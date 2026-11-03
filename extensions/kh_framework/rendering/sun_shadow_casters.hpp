#pragma once

// The feature half (rendering_integration_features.hpp): our sun shadow casters: the caster records, the alpha sets and
// the instanced caster draws. Never include it on its own.
namespace RenderIntegration {

// Entirely our pipeline, which is what makes mesh-shaped cast shadows and concave self-shadowing possible: our
// geometry never enters the engine's atlas.

// The caster record, at namespace scope so the alpha draw helper can take it. alpha = colour alpha x lifetime
// envelope, quantised to 1/64 (the input hash reads it: a fade re-renders the map, a sub-quantum wobble does not);
// mats = the material set when any used slot is cutout or blend, else null; alpha_caster = alpha below 1 or mats
// present, and the alpha twins exist.
struct SunCaster {
    float pos[3]; float size[3]; float rot[9]; bool rotated; int mesh;
    float res[3];   // POS_RES (obj_pos_res at the build).
    float bsize[3];   // CAST_REFIT: bounds_size_of - every bounds test (rendering/sun_shadow_render.hpp,
                      // rendering/dls_render.hpp); size draws, the LOD's distance.
    uint32_t slot;   // OBJBUF: the live-scene slot (the lane's index into the record buffer).
    float alpha;
    const MaterialSet* mats;   // Pool-owned (MAT_POOL); the grave wall outlives this list.
    bool alpha_caster;
    bool lod_lock;   // LOD_LOCK, carried so the dlsw mask can pick the drawn level.
    float lod_r;     // SKEL_LOD_R: lod_radius at the build - the pick's radius (the dlsw mask's too).
    int  lod;        // SHADOW_LOD_DRAWN: the level this pass draws (drawn_lod at the list build).
    bool visible;    // DLSW_MASK_ALPHA: a casterOnly invisible object casts but owns no pixel.
    bool in_front;   // INFRONT: drawn in the view-model slice (the dlsw mask draws it under the hands pair).
    // FOOT_BLEND (the seam's rule, SvCaster::no_foot): a non-normal blend mode combines with what is behind it,
    // so the world shows through at every texel. It still casts (the maps); it owns no pixel (the dlsw mask).
    bool no_foot;
};

inline bool cast_alpha_on() {
    return g_res.vs_sundepth_a && g_res.ps_sundepth_a && g_res.layout_sundepth_a;
}
// CAST_ALPHA: the per-object depth route's alpha pair (VSSunDepthAObj + PSSunDepthA; SUN_OBJ).
inline bool cast_alpha_obj_on() {
    return g_res.vs_sundepth_ao && g_res.ps_sundepth_a && g_res.layout_sundepth_ao;
}

// SKIN_POOL - the depth and seam runs over skin pool chunks. A skeletal caster whose slot draws a slice of a chunk
// joins a run with the casters of its mesh, level and chunk: the run draws through the route's Sk twin (the vertex
// pulled at the record's base - cb.hlsl's KhSkPull), every instance its own pose, where the IA route can give each
// binding only a run of one (one vertex buffer and offset a draw). cast_pool_of: that chunk, when the route's twin
// (sk) and the layout exist and the record the route reads its lanes and box from (g_objbuf_cpu: the GPU's copy
// since the pass's objbuf_sync) carries the base of the slice the tables name; null = the IA route, with the pair
// (VbRef). The tables' thread.
inline const SkinPoolChunk* cast_pool_of(int mesh, uint32_t slot, ID3D11VertexShader* sk) {
    if (!g_skin_pool_any || !sk || !g_res.layout_inst_sk) return nullptr;
    const SkinPoolChunk* const c = mesh_pool_of(mesh, slot);
    if (!c || slot >= g_cloth_voff_slot.size() || slot >= g_objbuf_cpu.size()) return nullptr;
    const float b = g_objbuf_cpu[slot].res[3];
    if (!(b >= 0.0f) ||
        static_cast<uint64_t>(b) * sizeof(MeshVertex) != static_cast<uint64_t>(g_cloth_voff_slot[slot])) {
        return nullptr;
    }
    return c;
}
// The sort's tie-break after (mesh, level): the chunk's serial (0 = none), so a chunk's casters stand together for
// the runs. Order only - each run's own test (cast_run_joins) decides. Each sort takes it only while a chunk is
// installed (g_skin_pool_any, chosen once a sort), once a caster (cast_key_put): without one, the sort is the
// plain (mesh, level) one, comparator and cost.
inline uint32_t cast_pool_key(int mesh, uint32_t slot) {
    if (!g_skin_pool_any) return 0u;   // No chunk installed: every key 0, the order (mesh, level) alone.
    const SkinPoolChunk* const c = mesh_pool_of(mesh, slot);
    return c ? c->serial : 0u;
}
// The pooled sorts' tie-break keys, taken once a caster a sort into a scratch by slot (cast_key_put), which the
// comparator reads (cast_key_at). Only the casters' slots are written, and read; one sort at a time (the tables'
// thread).
static std::vector<uint32_t> g_cast_key_scr;
inline void cast_key_put(int mesh, uint32_t slot) {
    if (slot >= (1u << 24)) return;   // Never a live slot; read as 0.
    if (slot >= g_cast_key_scr.size()) g_cast_key_scr.resize(static_cast<size_t>(slot) + 1u, 0u);
    g_cast_key_scr[slot] = cast_pool_key(mesh, slot);
}
inline uint32_t cast_key_at(uint32_t slot) {
    return slot < g_cast_key_scr.size() ? g_cast_key_scr[slot] : 0u;
}
// Whether the caster (mesh, slot) of the run's mesh and level joins the run its first caster opened: the
// same buffer and offset (the IA route), or the same chunk through the twin.
inline bool cast_run_joins(const VbRef& vb, const SkinPoolChunk* pc, int mesh,
                           uint32_t slot, ID3D11VertexShader* sk) {
    if (pc) return cast_pool_of(mesh, slot, sk) == pc;
    return mesh_vb_ref(mesh, slot) == vb && !cast_pool_of(mesh, slot, sk);
}

// The opaque sun-depth state: the depth-only pair (no pixel shader). Every caster in a map draws whole; the
// receiver's own range fade (KhSunRangeFade, mirMeta.w on the receive fills) is the one distance rule.
inline void sun_bind_opaque(ID3D11DeviceContext* ctx) {
    ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
}

// One caster record from a draw-list object, shared by render_sun_depth and dls_frame. The eligibility and range
// tests stay with each caller (they differ on purpose).
inline SunCaster sun_caster_of(const RenderObject& o, const float cam[3]) {
    SunCaster c;
    memcpy(c.pos, o.pos, sizeof(c.pos));
    obj_pos_res(o, c.res);   // POS_RES.
    memcpy(c.size, o.size, sizeof(c.size));
    bounds_size_of(o, c.bsize);   // CAST_REFIT.
    memcpy(c.rot, o.rot_m, sizeof(c.rot));
    c.rotated = o.rotated;
    c.mesh = mesh_id_clamp(o.mesh);
    c.slot = o.slot;
    c.lod_lock = lod_locked(o);   // LOD_SIM_SLOT.
    c.visible = obj_shown(o);   // DLSW_MASK_ALPHA; SHOW_INFRONT: owns pixels only where drawn.
    c.in_front = o.in_front;   // INFRONT.
    c.no_foot = o.blend_mode != 0;   // FOOT_BLEND (blend_mode is 0..5: sqf kh_rv_blend).
    // SHADOW_LOD_DRAWN; INFRONT_LOD: the slice draws an inFront mesh at level 0 only.
    c.lod_r = lod_radius(o);   // SKEL_LOD_R.
    c.lod = obj_in_front(o) ? 0 : drawn_lod(c.mesh, c.lod_r, c.pos, c.size, c.rot, c.rotated, c.lod_lock, cam);
    // The caster's alpha (colour alpha x lifetime envelope; an expired object casts nothing) quantised to 1/64, its
    // alpha-carrying material set, and the verdict.
    {
        bool exp = false;
        const float env = lifetime_envelope(o, effect_time_seconds(), exp);
        float a = exp ? 0.0f : o.color[3] * env;
        a = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
        c.alpha = floorf(a * 64.0f + 0.5f) / 64.0f;
        const MaterialSet* ms = obj_textured(o);
        c.mats = mat_set_has_alpha(ms) ? o.materials : nullptr;
        c.alpha_caster = cast_alpha_on() && (c.alpha < 0.999f || c.mats);
    }
    return c;
}

// Draws every alpha caster in set into the bound sun DSV: alpha twins bound, one instance each from the pass's
// stream (lane index = position in the set; the lane names the caster's record by slot, OBJBUF), a caster with no
// alpha-carrying material set (mats null) whole with its colour alpha, one with such a set per submesh of the shadow
// LOD with its material bound. Depth state, rasterizer and viewport are the pass's; leaves the opaque sun state bound
// again. One template over the set's element type (the sun ladder hands a caster vector, the light maps a per-face
// pointer list). False when an upload failed, or with no instance stream or device (no caller reaches those): the map
// is incomplete (sun_draw_obj_t's rule).
inline const SunCaster& sa_at(const SunCaster& c) { return c; }
inline const SunCaster& sa_at(const SunCaster* c) { return *c; }
template <class SaSet>
inline bool sun_draw_alpha_t(ID3D11DeviceContext* ctx, const SaSet& set,
                             const ConstantData& tpl, ID3D11Buffer* inst_vb) {
    if (!cast_alpha_on()) return true;   // No caster is an alpha caster (sun_caster_of).
    if (!inst_vb) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    ctx->IASetInputLayout(g_res.layout_sundepth_a);
    ctx->VSSetShader(g_res.vs_sundepth_a, nullptr, 0);
    ctx->PSSetShader(g_res.ps_sundepth_a, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* vbs[2] = { nullptr, inst_vb };
    const UINT strides[2] = { sizeof(MeshVertex), INST_STRIDE };
    UINT offsets[2] = { 0, 0 };
    VbRef bound_vb;   // CLOTH: keyed on the buffer, not the mesh id (SKIN_POOL: the pair).
    bool all = true;   // Every alpha caster drawn whole.

    for (size_t i = 0; i < set.size(); ++i) {
        const SunCaster& c = sa_at(set[i]);
        if (!c.alpha_caster) continue;

        const VbRef want = mesh_vb_ref(c.mesh, c.slot);
        if (want != bound_vb) {
            vbs[0] = want.vb;
            offsets[0] = want.off;
            ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);
            ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = want;
        }

        ConstantData cbd = tpl;

        if (!c.mats) {   // Colour alpha only: one draw, the whole shadow range, mode 0, no maps.
            bind_material(ctx, dev, cbd, nullptr, 0);
            if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) { all = false; break; }
            UINT ls = 0, lc = 0;
            mesh_lod_range_of(c.mesh, c.lod, ls, lc);   // SHADOW_LOD_DRAWN.
            ctx->DrawIndexedInstanced(lc, 1, ls, 0, static_cast<UINT>(i));
        } else {   // Per submesh of the shadow level, the material bound.
            const MeshDef& md = mesh_def(c.mesh);
            const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, c.lod);   // SHADOW_LOD_DRAWN.
            bool ok = true;

            for (size_t s = 0; s < tab.size() && ok; ++s) {
                const MeshSubmesh& sm = tab[s];
                if (sm.index_count == 0) continue;
                bind_material(ctx, dev, cbd, c.mats, s);
                ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
                if (!ok) break;
                ctx->DrawIndexedInstanced(sm.index_count, 1, sm.index_start, 0, static_cast<UINT>(i));
            }

            if (!ok) { all = false; break; }
        }
    }

    // The opaque sun state again, for whatever draws next at this site.
    ctx->IASetInputLayout(g_res.layout_sundepth);
    sun_bind_opaque(ctx);
    dev->Release();
    return all;
}
inline bool sun_draw_alpha(ID3D11DeviceContext* ctx, const std::vector<SunCaster>& set,
                           const ConstantData& tpl, ID3D11Buffer* inst_vb) {
    return sun_draw_alpha_t(ctx, set, tpl, inst_vb);
}
// SUN_OBJ - the instanced depth route's availability: the sun-depth pair, the record buffer and the shared
// stream. A pass without it (or whose set the stream cannot hold, or whose map failed) draws per object.
inline bool sun_inst_on() {
    return g_res.vs_sundepth && g_res.layout_sundepth && g_res.sun_instance_vb && g_res.obj_srv;   // OBJBUF.
}
// SUN_INST_CAP - the depth maps' shared instance stream (sun_instance_vb, one InstLane per caster), grown and
// never shrunk to hold need lanes: by the union for its casters and by the light maps for theirs, so a scene
// with no sun map (no published sun: the union returns before it grows) still instances its light faces. False
// when the stream is absent (a failed create): the caller draws per object (sun_draw_obj_t).
inline bool sun_inst_ensure(ID3D11Device* dev, size_t need) {
    if (dev && need > static_cast<size_t>(g_res.sun_instance_cap)) {
        KH_SAFE_RELEASE(g_res.sun_instance_vb);
        UINT cap = g_res.sun_instance_cap > 64 ? g_res.sun_instance_cap : 64;
        while (static_cast<size_t>(cap) < need) cap *= 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = cap * INST_STRIDE;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_res.sun_instance_vb))) {
            g_res.sun_instance_cap = cap;
        } else {
            g_res.sun_instance_vb = nullptr;
            g_res.sun_instance_cap = 0;
        }
    }
    return g_res.sun_instance_vb != nullptr;
}
// SUN_OBJ - the per-object depth route, for the union, the four tiers and the light faces alike: every caster
// through VSMain's armed branch against the map's origin (org, engine axes: the anchor for the sun maps, the
// light for a face - the frame CB's sunOrigin, which the matrix measures from; POS_RES residual included), at
// its drawn level (SHADOW_LOD_DRAWN, as the instanced runs - or the two disagree on silhouette), one constant
// upload + draw each; a caster whose alpha is under 1 or whose material set carries alpha through VSSunDepthAObj +
// PSSunDepthA by sun_draw_alpha's rules (a cutout clips at its cutoff, a fade and a blend material's translucent
// texels dither their coverage), its alpha (colour x envelope, 1/64 steps) in color.a, per submesh of the drawn
// level with its material bound; without that pair the caster draws whole. The caller binds the target, viewport,
// depth and rasterizer states and uploads the frame CB; this binds its own IA / VS / PS state and leaves the
// per-object opaque pair (input_layout, VSMain, no PS). False when an upload failed: the map is incomplete.
template <class SaSet>
inline bool sun_draw_obj_t(ID3D11DeviceContext* ctx, const SaSet& set, const float org[3]) {
    ID3D11Device* dev = nullptr;   // bind_material's.
    ctx->GetDevice(&dev);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
    const UINT stride = sizeof(MeshVertex);
    VbRef bound_vb;   // CLOTH: the buffer identifies the draw, not the mesh id (SKIN_POOL: the pair).
    bool ok = true;
    for (size_t i = 0; i < set.size() && ok; ++i) {
        const SunCaster& c = sa_at(set[i]);
        const VbRef vb = mesh_vb_ref(c.mesh, c.slot);   // CLOTH.
        if (vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = vb;
        }
        ConstantData cbd = {};
        cbd.center_size[0] = c.pos[0];
        cbd.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = c.pos[1];
        cbd.size_axes[0] = c.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        cbd.size_axes[1] = c.size[2];
        cbd.size_axes[2] = c.size[1];
        fill_center_rel(cbd, org, c.res);
        fill_obj_rot(cbd, c.rot);   // Object rotation (VSMain path).
        if ((c.alpha < 0.999f || c.mats) && dev && cast_alpha_obj_on()) {
            ctx->IASetInputLayout(g_res.layout_sundepth_ao);
            ctx->VSSetShader(g_res.vs_sundepth_ao, nullptr, 0);
            ctx->PSSetShader(g_res.ps_sundepth_a, nullptr, 0);
            if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
            cbd.color[3] = c.alpha;
            if (!c.mats) {   // Colour alpha only: one draw, the whole level, mode 0, no maps.
                bind_material(ctx, dev, cbd, nullptr, 0);
                ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
                if (ok) {
                    UINT als = 0, alc = 0;
                    mesh_lod_range_of(c.mesh, c.lod, als, alc);
                    ctx->DrawIndexed(alc, als, 0);
                }
            } else {   // Per submesh of the level, its material bound.
                const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(mesh_def(c.mesh), c.lod);
                for (size_t s = 0; s < tab.size() && ok; ++s) {
                    const MeshSubmesh& sm = tab[s];
                    if (sm.index_count == 0) continue;
                    bind_material(ctx, dev, cbd, c.mats, s);
                    ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
                    if (ok) ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                }
            }
            ctx->IASetInputLayout(g_res.input_layout);   // The opaque pair again.
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
            continue;
        }
        ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
        if (!ok) break;
        UINT ls = 0, lc = 0;
        mesh_lod_range_of(c.mesh, c.lod, ls, lc);
        ctx->DrawIndexed(lc, ls, 0);
    }
    if (dev) dev->Release();
    return ok;
}
}