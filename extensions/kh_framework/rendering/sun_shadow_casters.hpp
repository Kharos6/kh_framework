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
    float res[3];   // KH_POS_RES (kh_pos_res at the build).
    float bsize[3];   // KH_CAST_REFIT: kh_bounds_size_of - every bounds test (rendering/sun_shadow_render.hpp,
                      // rendering/dls_render.hpp); size draws, the LOD's distance.
    uint32_t slot;   // KH_OBJBUF: the live-scene slot (the lane's index into the record buffer).
    float alpha;
    const KhMaterialSet* mats;   // Pool-owned (KH_MAT_POOL); the grave wall outlives this list.
    bool alpha_caster;
    bool lod_lock;   // KH_LOD_LOCK, carried so the dlsw mask can pick the drawn level.
    float lod_r;     // KH_SKEL_LOD_R: kh_lod_radius at the build - the pick's radius (the dlsw mask's too).
    int  lod;        // KH_SHADOW_LOD_DRAWN: the level this pass draws (kh_drawn_lod at the list build).
    bool visible;    // KH_DLSW_MASK_ALPHA: a casterOnly invisible object casts but owns no pixel.
    bool in_front;   // KH_INFRONT: drawn in the view-model slice (the dlsw mask draws it under the hands pair).
    // KH_FOOT_BLEND (the seam's rule, KhSvCaster::no_foot): a non-normal blend mode combines with what is behind it,
    // so the world shows through at every texel. It still casts (the maps); it owns no pixel (the dlsw mask).
    bool no_foot;
};

inline bool kh_cast_alpha_on() {
    return g_res.vs_sundepth_a && g_res.ps_sundepth_a && g_res.layout_sundepth_a;
}
// KH_CAST_ALPHA: the per-object depth route's alpha pair (VSSunDepthAObj + PSSunDepthA; KH_SUN_OBJ).
inline bool kh_cast_alpha_obj_on() {
    return g_res.vs_sundepth_ao && g_res.ps_sundepth_a && g_res.layout_sundepth_ao;
}

// KH_SKIN_POOL - the depth and seam runs over skin pool chunks. A skeletal caster whose slot draws a slice of a chunk
// joins a run with the casters of its mesh, level and chunk: the run draws through the route's Sk twin (the vertex
// pulled at the record's base - cb.hlsl's KhSkPull), every instance its own pose, where the IA route can give each
// binding only a run of one (one vertex buffer and offset a draw). kh_cast_pool_of: that chunk, when the route's twin
// (khcp_sk) and the layout exist and the record the route reads its lanes and box from (g_objbuf_cpu: the GPU's copy
// since the pass's kh_objbuf_sync) carries the base of the slice the tables name; null = the IA route, with the pair
// (KhVbRef). The tables' thread.
inline const KhSkinPoolChunk* kh_cast_pool_of(int khcp_mesh, uint32_t khcp_slot, ID3D11VertexShader* khcp_sk) {
    if (!g_skin_pool_any || !khcp_sk || !g_res.layout_inst_sk) return nullptr;
    const KhSkinPoolChunk* const khcp_c = kh_mesh_pool_of(khcp_mesh, khcp_slot);
    if (!khcp_c || khcp_slot >= g_cloth_voff_slot.size() || khcp_slot >= g_objbuf_cpu.size()) return nullptr;
    const float khcp_b = g_objbuf_cpu[khcp_slot].res[3];
    if (!(khcp_b >= 0.0f) ||
        static_cast<uint64_t>(khcp_b) * sizeof(MeshVertex) != static_cast<uint64_t>(g_cloth_voff_slot[khcp_slot])) {
        return nullptr;
    }
    return khcp_c;
}
// The sort's tie-break after (mesh, level): the chunk's serial (0 = none), so a chunk's casters stand together for
// the runs. Order only - each run's own test (kh_cast_run_joins) decides. Each sort takes it only while a chunk is
// installed (g_skin_pool_any, chosen once a sort), once a caster (kh_cast_key_put): without one, the sort is the
// plain (mesh, level) one, comparator and cost.
inline uint32_t kh_cast_pool_key(int khck_mesh, uint32_t khck_slot) {
    if (!g_skin_pool_any) return 0u;   // No chunk installed: every key 0, the order (mesh, level) alone.
    const KhSkinPoolChunk* const khck_c = kh_mesh_pool_of(khck_mesh, khck_slot);
    return khck_c ? khck_c->serial : 0u;
}
// The pooled sorts' tie-break keys, taken once a caster a sort into a scratch by slot (kh_cast_key_put), which the
// comparator reads (kh_cast_key_at). Only the casters' slots are written, and read; one sort at a time (the tables'
// thread).
static std::vector<uint32_t> g_cast_key_scr;
inline void kh_cast_key_put(int khkp_mesh, uint32_t khkp_slot) {
    if (khkp_slot >= (1u << 24)) return;   // Never a live slot; read as 0.
    if (khkp_slot >= g_cast_key_scr.size()) g_cast_key_scr.resize(static_cast<size_t>(khkp_slot) + 1u, 0u);
    g_cast_key_scr[khkp_slot] = kh_cast_pool_key(khkp_mesh, khkp_slot);
}
inline uint32_t kh_cast_key_at(uint32_t khka_slot) {
    return khka_slot < g_cast_key_scr.size() ? g_cast_key_scr[khka_slot] : 0u;
}
// Whether the caster (khrj_mesh, khrj_slot) of the run's mesh and level joins the run its first caster opened: the
// same buffer and offset (the IA route), or the same chunk through the twin.
inline bool kh_cast_run_joins(const KhVbRef& khrj_vb, const KhSkinPoolChunk* khrj_pc, int khrj_mesh,
                              uint32_t khrj_slot, ID3D11VertexShader* khrj_sk) {
    if (khrj_pc) return kh_cast_pool_of(khrj_mesh, khrj_slot, khrj_sk) == khrj_pc;
    return kh_mesh_vb_ref(khrj_mesh, khrj_slot) == khrj_vb && !kh_cast_pool_of(khrj_mesh, khrj_slot, khrj_sk);
}

// The opaque sun-depth state: the depth-only pair (no pixel shader). Every caster in a map draws whole; the
// receiver's own range fade (KhSunRangeFade, mirMeta.w on the receive fills) is the one distance rule.
inline void kh_sun_bind_opaque(ID3D11DeviceContext* khbo_ctx) {
    khbo_ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
    khbo_ctx->PSSetShader(nullptr, nullptr, 0);
}

// One caster record from a draw-list object, shared by render_sun_depth and kh_dls_frame. The eligibility and range
// tests stay with each caller (they differ on purpose).
inline SunCaster kh_sun_caster_of(const RenderObject& o, const float khsc_cam[3]) {
    SunCaster c;
    memcpy(c.pos, o.pos, sizeof(c.pos));
    kh_pos_res(o, c.res);   // KH_POS_RES.
    memcpy(c.size, o.size, sizeof(c.size));
    kh_bounds_size_of(o, c.bsize);   // KH_CAST_REFIT.
    memcpy(c.rot, o.rot_m, sizeof(c.rot));
    c.rotated = o.rotated;
    c.mesh = mesh_id_clamp(o.mesh);
    c.slot = o.slot;
    c.lod_lock = kh_lod_locked(o);   // KH_LOD_SIM_SLOT.
    c.visible = kh_obj_shown(o);   // KH_DLSW_MASK_ALPHA; KH_SHOW_INFRONT: owns pixels only where drawn.
    c.in_front = o.in_front;   // KH_INFRONT.
    c.no_foot = o.blend_mode != 0;   // KH_FOOT_BLEND (blend_mode is 0..5: sqf kh_rv_blend).
    // KH_SHADOW_LOD_DRAWN; KH_INFRONT_LOD: the slice draws an inFront mesh at level 0 only.
    c.lod_r = kh_lod_radius(o);   // KH_SKEL_LOD_R.
    c.lod = kh_in_front(o) ? 0 : kh_drawn_lod(c.mesh, c.lod_r, c.pos, c.size, c.rot, c.rotated, c.lod_lock, khsc_cam);
    // The caster's alpha (colour alpha x lifetime envelope; an expired object casts nothing) quantised to 1/64, its
    // alpha-carrying material set, and the verdict.
    {
        bool khsc_exp = false;
        const float khsc_env = lifetime_envelope(o, effect_time_seconds(), khsc_exp);
        float khsc_a = khsc_exp ? 0.0f : o.color[3] * khsc_env;
        khsc_a = khsc_a < 0.0f ? 0.0f : (khsc_a > 1.0f ? 1.0f : khsc_a);
        c.alpha = floorf(khsc_a * 64.0f + 0.5f) / 64.0f;
        const KhMaterialSet* khsc_ms = kh_obj_textured(o);
        c.mats = kh_mat_set_has_alpha(khsc_ms) ? o.materials : nullptr;
        c.alpha_caster = kh_cast_alpha_on() && (c.alpha < 0.999f || c.mats);
    }
    return c;
}

// Draws every alpha caster in khsa_set into the bound sun DSV: alpha twins bound, one instance each from the pass's
// stream (lane index = position in the set; the lane names the caster's record by slot, KH_OBJBUF), a caster with no
// alpha-carrying material set (mats null) whole with its colour alpha, one with such a set per submesh of the shadow
// LOD with its material bound. Depth state, rasterizer and viewport are the pass's; leaves the opaque sun state bound
// again. One template over the set's element type (the sun ladder hands a caster vector, the light maps a per-face
// pointer list). False when an upload failed, or with no instance stream or device (no caller reaches those): the map
// is incomplete (kh_sun_draw_obj_t's rule).
inline const SunCaster& kh_sa_at(const SunCaster& khsa_c) { return khsa_c; }
inline const SunCaster& kh_sa_at(const SunCaster* khsa_c) { return *khsa_c; }
template <class KhSaSet>
inline bool kh_sun_draw_alpha_t(ID3D11DeviceContext* ctx, const KhSaSet& khsa_set,
                                const ConstantData& khsa_tpl, ID3D11Buffer* khsa_inst_vb) {
    if (!kh_cast_alpha_on()) return true;   // No caster is an alpha caster (kh_sun_caster_of).
    if (!khsa_inst_vb) return false;
    ID3D11Device* khsa_dev = nullptr;
    ctx->GetDevice(&khsa_dev);
    if (!khsa_dev) return false;
    ctx->IASetInputLayout(g_res.layout_sundepth_a);
    ctx->VSSetShader(g_res.vs_sundepth_a, nullptr, 0);
    ctx->PSSetShader(g_res.ps_sundepth_a, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* khsa_vbs[2] = { nullptr, khsa_inst_vb };
    const UINT khsa_strides[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
    UINT khsa_offsets[2] = { 0, 0 };
    KhVbRef khsa_bound_vb;   // KH_CLOTH: keyed on the buffer, not the mesh id (KH_SKIN_POOL: the pair).
    bool khsa_all = true;   // Every alpha caster drawn whole.

    for (size_t khsa_i = 0; khsa_i < khsa_set.size(); ++khsa_i) {
        const SunCaster& c = kh_sa_at(khsa_set[khsa_i]);
        if (!c.alpha_caster) continue;

        const KhVbRef khsa_want = kh_mesh_vb_ref(c.mesh, c.slot);
        if (khsa_want != khsa_bound_vb) {
            khsa_vbs[0] = khsa_want.vb;
            khsa_offsets[0] = khsa_want.off;
            ctx->IASetVertexBuffers(0, 2, khsa_vbs, khsa_strides, khsa_offsets);
            ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
            khsa_bound_vb = khsa_want;
        }

        ConstantData khsa_cbd = khsa_tpl;

        if (!c.mats) {   // Colour alpha only: one draw, the whole shadow range, mode 0, no maps.
            kh_bind_material(ctx, khsa_dev, khsa_cbd, nullptr, 0);
            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khsa_cbd)) { khsa_all = false; break; }
            UINT khsa_ls = 0, khsa_lc = 0;
            mesh_lod_range_of(c.mesh, c.lod, khsa_ls, khsa_lc);   // KH_SHADOW_LOD_DRAWN.
            ctx->DrawIndexedInstanced(khsa_lc, 1, khsa_ls, 0, static_cast<UINT>(khsa_i));
        } else {   // Per submesh of the shadow level, the material bound.
            const MeshDef& khsa_md = mesh_def(c.mesh);
            const std::vector<MeshSubmesh>& khsa_tab = mesh_lod_submeshes(khsa_md, c.lod);   // KH_SHADOW_LOD_DRAWN.
            bool khsa_ok = true;

            for (size_t khsa_s = 0; khsa_s < khsa_tab.size() && khsa_ok; ++khsa_s) {
                const MeshSubmesh& khsa_sm = khsa_tab[khsa_s];
                if (khsa_sm.index_count == 0) continue;
                kh_bind_material(ctx, khsa_dev, khsa_cbd, c.mats, khsa_s);
                khsa_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khsa_cbd);
                if (!khsa_ok) break;
                ctx->DrawIndexedInstanced(khsa_sm.index_count, 1, khsa_sm.index_start, 0, static_cast<UINT>(khsa_i));
            }

            if (!khsa_ok) { khsa_all = false; break; }
        }
    }

    // The opaque sun state again, for whatever draws next at this site.
    ctx->IASetInputLayout(g_res.layout_sundepth);
    kh_sun_bind_opaque(ctx);
    khsa_dev->Release();
    return khsa_all;
}
inline bool kh_sun_draw_alpha(ID3D11DeviceContext* ctx, const std::vector<SunCaster>& khsa_set,
                              const ConstantData& khsa_tpl, ID3D11Buffer* khsa_inst_vb) {
    return kh_sun_draw_alpha_t(ctx, khsa_set, khsa_tpl, khsa_inst_vb);
}
// KH_SUN_OBJ - the instanced depth route's availability: the sun-depth pair, the record buffer and the shared
// stream. A pass without it (or whose set the stream cannot hold, or whose map failed) draws per object.
inline bool kh_sun_inst_on() {
    return g_res.vs_sundepth && g_res.layout_sundepth && g_res.sun_instance_vb && g_res.obj_srv;   // KH_OBJBUF.
}
// KH_SUN_INST_CAP - the depth maps' shared instance stream (sun_instance_vb, one KhInstLane per caster), grown and
// never shrunk to hold khsi_need lanes: by the union for its casters and by the light maps for theirs, so a scene
// with no sun map (no published sun: the union returns before it grows) still instances its light faces. False
// when the stream is absent (a failed create): the caller draws per object (kh_sun_draw_obj_t).
inline bool kh_sun_inst_ensure(ID3D11Device* khsi_dev, size_t khsi_need) {
    if (khsi_dev && khsi_need > static_cast<size_t>(g_res.sun_instance_cap)) {
        KH_SAFE_RELEASE(g_res.sun_instance_vb);
        UINT khsi_cap = g_res.sun_instance_cap > 64 ? g_res.sun_instance_cap : 64;
        while (static_cast<size_t>(khsi_cap) < khsi_need) khsi_cap *= 2;
        D3D11_BUFFER_DESC khsi_bd = {};
        khsi_bd.ByteWidth = khsi_cap * KH_INST_STRIDE;
        khsi_bd.Usage = D3D11_USAGE_DYNAMIC;
        khsi_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        khsi_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (SUCCEEDED(khsi_dev->CreateBuffer(&khsi_bd, nullptr, &g_res.sun_instance_vb))) {
            g_res.sun_instance_cap = khsi_cap;
        } else {
            g_res.sun_instance_vb = nullptr;
            g_res.sun_instance_cap = 0;
        }
    }
    return g_res.sun_instance_vb != nullptr;
}
// KH_SUN_OBJ - the per-object depth route, for the union, the four tiers and the light faces alike: every caster
// through VSMain's armed branch against the map's origin (khso_org, engine axes: the anchor for the sun maps, the
// light for a face - the frame CB's sunOrigin, which the matrix measures from; KH_POS_RES residual included), at
// its drawn level (KH_SHADOW_LOD_DRAWN, as the instanced runs - or the two disagree on silhouette), one constant
// upload + draw each; a caster whose alpha is under 1 or whose material set carries alpha through VSSunDepthAObj +
// PSSunDepthA by kh_sun_draw_alpha's rules (a cutout clips at its cutoff, a fade and a blend material's translucent
// texels dither their coverage), its alpha (colour x envelope, 1/64 steps) in color.a, per submesh of the drawn
// level with its material bound; without that pair the caster draws whole. The caller binds the target, viewport,
// depth and rasterizer states and uploads the frame CB; this binds its own IA / VS / PS state and leaves the
// per-object opaque pair (input_layout, VSMain, no PS). False when an upload failed: the map is incomplete.
template <class KhSaSet>
inline bool kh_sun_draw_obj_t(ID3D11DeviceContext* ctx, const KhSaSet& khso_set, const float khso_org[3]) {
    ID3D11Device* khso_dev = nullptr;   // kh_bind_material's.
    ctx->GetDevice(&khso_dev);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
    const UINT khso_stride = sizeof(MeshVertex);
    KhVbRef khso_bound_vb;   // KH_CLOTH: the buffer identifies the draw, not the mesh id (KH_SKIN_POOL: the pair).
    bool khso_ok = true;
    for (size_t khso_i = 0; khso_i < khso_set.size() && khso_ok; ++khso_i) {
        const SunCaster& c = kh_sa_at(khso_set[khso_i]);
        const KhVbRef khso_vb = kh_mesh_vb_ref(c.mesh, c.slot);   // KH_CLOTH.
        if (khso_vb != khso_bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khso_vb.vb, &khso_stride, &khso_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
            khso_bound_vb = khso_vb;
        }
        ConstantData khso_cbd = {};
        khso_cbd.center_size[0] = c.pos[0];
        khso_cbd.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        khso_cbd.center_size[2] = c.pos[1];
        khso_cbd.size_axes[0] = c.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        khso_cbd.size_axes[1] = c.size[2];
        khso_cbd.size_axes[2] = c.size[1];
        kh_fill_center_rel(khso_cbd, khso_org, c.res);
        kh_fill_obj_rot(khso_cbd, c.rot);   // Object rotation (VSMain path).
        if ((c.alpha < 0.999f || c.mats) && khso_dev && kh_cast_alpha_obj_on()) {
            ctx->IASetInputLayout(g_res.layout_sundepth_ao);
            ctx->VSSetShader(g_res.vs_sundepth_ao, nullptr, 0);
            ctx->PSSetShader(g_res.ps_sundepth_a, nullptr, 0);
            if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
            khso_cbd.color[3] = c.alpha;
            if (!c.mats) {   // Colour alpha only: one draw, the whole level, mode 0, no maps.
                kh_bind_material(ctx, khso_dev, khso_cbd, nullptr, 0);
                khso_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khso_cbd);
                if (khso_ok) {
                    UINT khso_als = 0, khso_alc = 0;
                    mesh_lod_range_of(c.mesh, c.lod, khso_als, khso_alc);
                    ctx->DrawIndexed(khso_alc, khso_als, 0);
                }
            } else {   // Per submesh of the level, its material bound.
                const std::vector<MeshSubmesh>& khso_tab = mesh_lod_submeshes(mesh_def(c.mesh), c.lod);
                for (size_t khso_s = 0; khso_s < khso_tab.size() && khso_ok; ++khso_s) {
                    const MeshSubmesh& khso_sm = khso_tab[khso_s];
                    if (khso_sm.index_count == 0) continue;
                    kh_bind_material(ctx, khso_dev, khso_cbd, c.mats, khso_s);
                    khso_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khso_cbd);
                    if (khso_ok) ctx->DrawIndexed(khso_sm.index_count, khso_sm.index_start, 0);
                }
            }
            ctx->IASetInputLayout(g_res.input_layout);   // The opaque pair again.
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
            continue;
        }
        khso_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khso_cbd);
        if (!khso_ok) break;
        UINT khso_ls = 0, khso_lc = 0;
        mesh_lod_range_of(c.mesh, c.lod, khso_ls, khso_lc);
        ctx->DrawIndexed(khso_lc, khso_ls, 0);
    }
    if (khso_dev) khso_dev->Release();
    return khso_ok;
}
}