#pragma once

// The feature half (rendering_integration_features.hpp): instancing: eligibility, keys, the plan and inst_draw.
// Never include it on its own.
namespace RenderIntegration {
static constexpr uint32_t INST_CAP = 65536u;   // Lanes per ring (1 MB).

// SKIN_POOL: the colour buckets' Sk twins - both units' plain and textured pairs and their layout. Any one absent
// keeps every skeletal binding out of the buckets (inst_pool_of), whichever pass plans.
inline bool inst_sk_on() {
    return g_res.vs_inst_sk && g_res.vs_inst_sk_tex && g_res.vs_comp_inst_sk && g_res.vs_comp_inst_sk_tex &&
           g_res.layout_inst_sk;
}
// SKIN_POOL: the skin pool chunk a skeletal binding's bucket draw pulls its pose from, or null (it draws per
// object, its slice bound by the IA offset). Non-null requires: its slot draws a slice of a chunk (the tables,
// mesh_pool_of), the twins exist, and the record the bucket's VS reads (g_objbuf_cpu: what objbuf_sync put on
// the GPU before any bucket draw of the pass) carries this copy's every lane that VS reads - the base (res.w)
// naming the slice the tables name, and the box that slice's bytes are normalised to. A copy the record has not
// caught up with (a base the next sync carries, a box a flush's catch-up moved under SCENE_HOLD - SKIN_CULL)
// draws per object that pass. The tables' thread; also from inst_plan_build's fork (reads only).
inline const SkinPoolChunk* inst_pool_of(const RenderObject& o) {
    if (!o.skel || !inst_sk_on()) return nullptr;
    const SkinPoolChunk* const c = mesh_pool_of(mesh_id_clamp(o.mesh), o.slot);
    if (!c || o.slot >= g_cloth_voff_slot.size() || o.slot >= g_objbuf_cpu.size()) return nullptr;
    if (static_cast<uint64_t>(g_cloth_voff_slot[o.slot]) !=
        static_cast<uint64_t>(o.skin_vbase) * sizeof(MeshVertex)) return nullptr;
    KhObjRec r;
    objrec_fill(r, o);
    const KhObjRec& m = g_objbuf_cpu[o.slot];
    // Every lane but the colour's alpha (the copy's carries the envelope; the bucket's alpha is the lane's).
    if (memcmp(&r, &m, offsetof(KhObjRec, col)) != 0 ||
        memcmp(r.col, m.col, 3u * sizeof(float)) != 0 ||
        memcmp(r.res, m.res, sizeof(r.res)) != 0) return nullptr;
    return c;
}

// Bucket eligibility: a mesh drawn from its mesh's shared vertex buffer. Kept per object: a normal-blend whole
// translucent, an effect mesh, a fullscreen pass, depth-Off, a localized / banded mesh (it reads its own mask
// lanes), an exact lighten / darken (below), a simulated or vertex-staged mesh, a slot drawing a substitute and a
// blend material's translucent part copy (INST_PART_LANE); the other blend modes are eligible at any alpha, as
// are the solid part copies. SKIN_POOL: pc (eligible only) = the skin pool chunk the object's bucket pulls
// from, null for any other.
// BLEND_MINMAX: a lighten / darken mesh - not an effect mesh (KhFxFinish blends those exactly), not a fullscreen
// pass, not a blend material's translucent part (hardware-blended like every part) - whose opacity can fall below
// 1: colour alpha below 1 (the envelope is folded in by the flush's collection), or banded / localized (SolidMask).
// Those are the twins' whole a here: the flush draws a blend material's solid texels only, at alpha 1
// (draw_textured's part 1). The MIN / MAX blend ops take no factors, so the hardware forms are exact at a = 1
// only: lighten max(scene, colour x a), darken min(scene, lerp(65504, colour, a)) - the scene itself for any a
// short of 1 (a darken mesh at opacity 0.8 darkened nothing). The flush draws such a mesh on its own (not instanced)
// against its own copy of the scene taken just before it (mm_capture), and the twins write the exact lerp(scene,
// max / min(scene, colour), a) (blendCtl.x), so each one reads the scene the meshes before it left. The hardware
// forms stand at a = 1 (exact; instanced, no copy), when the copy cannot be made, and for an inFront mesh (the
// hands slice draws it).
inline bool blend_minmax_exact(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.draw_part != 1 && (o.blend_mode == 4 || o.blend_mode == 5) &&
           (o.color[3] < 1.0f || o.localized || o.banded);   // Strict: darken at a = 0.9995 adds ~33.
}

inline bool inst_eligible(const RenderObject& o, const SkinPoolChunk*& pc) {
    pc = nullptr;
    // CLOTH: a bucket binds ONE vertex buffer for every member, and a simulated mesh has a buffer of its own -
    // two cloths sharing a mesh would otherwise both draw whichever one's buffer the bucket bound. The per-object
    // path costs a draw call, which translucents and effect meshes already pay.
    if (o.cloth_sim) return false;
    if (o.chain_sim) return false;   // CHAIN: the same reason.
    // INST_PART_LANE: the flush's copy of a blend material's translucent part draws per object, in the tail's
    // far-to-near order among every other translucent (a bucket drew all its members' parts at the farthest member's
    // place: a nearer copy's glass went down before the translucents between them). The draws are the same either
    // way (one per lane, INST_PART_ORDER); per object it pays its own constant-buffer fill and, lit, its own
    // light-ring append, as every other translucent of the tail does.
    if (o.draw_part == 1) return false;
    if (blend_minmax_exact(o)) return false;   // BLEND_MINMAX: each against its own capture.
    // SKEL: the same reason - its skinned buffer is its own. SKIN_POOL: unless it is a slice of a skin pool
    // chunk, which its bucket's Sk twin pulls each member's own pose from (the chunk keys the bucket: InstKey::pool).
    if (o.skel) pc = inst_pool_of(o);
    const bool pool = pc != nullptr;
    if (o.skel && !pool) return false;
    if (o.materials && o.materials->vertex_any) return false;   // KH_USER_VS: the same reason - its deformed buffer is its own.
    // LOD_SIM_SLOT: nor while its slot still draws a substitute - a simulation's or skin's buffer stays until
    // the next flush after the flag that set it ends (and a bucket's level could read a cloth's rest-pose tail).
    // SKIN_POOL: a pooled binding's substitute is the chunk its bucket pulls from.
    if (!pool) {
        const int mid = mesh_id_clamp(o.mesh);
        if (mid >= 0 && static_cast<size_t>(mid) < g_res.mesh_vb.size() &&
            mesh_vb_ref(mid, o.slot).vb != g_res.mesh_vb[static_cast<size_t>(mid)]) return false;
    }
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           !(o.blend_mode == 0 && o.color[3] < 0.999f) && !o.localized && !o.banded;
}

inline bool inst_twins_on() {
    return g_res.vs_inst && g_res.layout_inst && g_res.inst_vb[0] && g_res.inst_vb[1] && g_res.obj_srv;
}

inline void inst_lane_fill(InstLane& l, const RenderObject& o, float dither, float mat_base) {
    l.slot = o.slot;
    l.dither = dither;
    l.alpha = o.color[3];   // The staging copy's alpha - envelope already folded (fold first, then plan).
    l.mat_base = mat_base;
}

// Writes up to INST_CAP lanes; returns the count (0 on a failed map, which re-DISCARDs the ring next time) and
// the first lane's index. A full ring wraps with a discard: every earlier draw of the pass already consumed its lanes.
inline uint32_t inst_ring_write(ID3D11DeviceContext* ctx, int ri, const InstLane* recs,
                                uint32_t n, uint32_t& first) {
    ID3D11Buffer* buf = g_res.inst_vb[ri];
    uint32_t& cur = g_res.inst_cursor[ri];
    bool& fresh = g_res.inst_fresh[ri];
    first = 0;
    if (!buf || n == 0) return 0;
    if (n > INST_CAP) n = INST_CAP;
    if (cur + n > INST_CAP) { cur = 0; fresh = true; }
    D3D11_MAPPED_SUBRESOURCE m = {};
    const D3D11_MAP mode = fresh ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

    if (FAILED(ctx->Map(buf, 0, mode, 0, &m))) {
        fresh = true;
        return 0;
    }

    fresh = false;
    memcpy(static_cast<uint8_t*>(m.pData) + static_cast<size_t>(cur) * INST_STRIDE,
           recs, static_cast<size_t>(n) * INST_STRIDE);
    ctx->Unmap(buf, 0);
    first = cur;
    cur += n;
    return n;
}

// route / aux are filled by inst_plan_build's functor. A set not yet uploaded keys on its content hash (fallback
// only; the passes ensure every set before planning).
inline InstKey inst_key(const RenderObject& o) {
    InstKey k;
    k.mesh = mesh_id_clamp(o.mesh);
    k.pool = 0u;   // SKIN_POOL: inst_plan_build's (the chunk inst_eligible found).
    {
        const MaterialSet* ms = obj_textured(o);
        // |1 keeps a zero hash distinct from "untextured".
        k.mats = ms ? ((ms->gpu.base >= 0 ? ms->gpu.page_sig : ms->hash) | 1u) : 0u;
    }
    k.blend = o.blend_mode;
    k.mode = static_cast<int>(o.mode);
    k.lit = o.lit ? 1 : 0;
    k.two_sided = o.two_sided ? 1 : 0;
    k.part = o.draw_part;
    k.route = 0;
    k.aux = 0;
    return k;
}

inline int inst_key_cmp(const InstKey& a, const InstKey& b) {
    if (a.mesh != b.mesh) return a.mesh < b.mesh ? -1 : 1;
    if (a.pool != b.pool) return a.pool < b.pool ? -1 : 1;   // SKIN_POOL (0 everywhere but a skeletal bucket).
    if (a.mats != b.mats) return a.mats < b.mats ? -1 : 1;
    if (a.blend != b.blend) return a.blend < b.blend ? -1 : 1;
    if (a.mode != b.mode) return a.mode < b.mode ? -1 : 1;
    if (a.lit != b.lit) return a.lit < b.lit ? -1 : 1;
    if (a.two_sided != b.two_sided) return a.two_sided < b.two_sided ? -1 : 1;
    if (a.part != b.part) return a.part < b.part ? -1 : 1;
    if (a.route != b.route) return a.route < b.route ? -1 : 1;
    if (a.aux != b.aux) return a.aux < b.aux ? -1 : 1;
    return 0;
}

// order = the loop's visiting order (nullptr = natural). Groups by key; members keep their visiting order
// inside a bucket. route(o, key) fills the pass's per-object key lanes.
template <class InstRoute>
inline void inst_plan_build(InstPlan& p, const std::vector<RenderObject>& meshes,
                            const uint32_t* order, size_t n, InstRoute route) {
    KH_PROF_SCOPE(PROF_INST_PLAN);   // PROF.
    p.batch_of.assign(meshes.size(), -1);
    p.done.assign(meshes.size(), 0);
    p.batch_begin.clear();
    p.batch_n.clear();
    p.batch_pool.clear();   // SKIN_POOL.
    p.members.clear();
    p.ents.clear();
    p.rep.clear();
    p.range_begin.clear();
    p.range_n.clear();
    p.ranges.clear();
    p.lanes.clear();
    p.lane_d2.clear();   // INST_PART_ORDER.

    {   // INJ_FORK: every rank's key in parallel (its own slot; a hole where not eligible), then the compaction in rank order.
        static std::vector<InstPlan::Ent> scr;
        static std::vector<uint8_t> on;
        scr.resize(n);
        on.assign(n, 0u);
        fork_each(static_cast<uint32_t>(n), [&](uint32_t r) {
            const uint32_t i = order ? order[r] : r;
            if (i >= meshes.size()) return;
            const RenderObject& o = meshes[i];
            const SkinPoolChunk* pc = nullptr;   // SKIN_POOL.
            if (!inst_eligible(o, pc)) return;
            InstPlan::Ent& e = scr[r];
            e.key = inst_key(o);
            e.key.pool = pc ? pc->serial : 0u;   // SKIN_POOL: the bucket is the chunk's.
            e.pool_c = pc;
            route(o, e.key);
            e.rank = r;
            e.idx = i;
            on[r] = 1u;
        });
        for (size_t r = 0; r < n; ++r) if (on[r]) p.ents.push_back(scr[r]);
    }

    if (p.ents.empty()) return;
    std::stable_sort(p.ents.begin(), p.ents.end(), [](const InstPlan::Ent& a, const InstPlan::Ent& b) {
        const int c = inst_key_cmp(a.key, b.key);
        return c != 0 ? c < 0 : a.rank < b.rank;
    });

    for (size_t ip_e = 0; ip_e < p.ents.size(); ++ip_e) {
        const bool ip_new = ip_e == 0 || inst_key_cmp(p.ents[ip_e - 1].key, p.ents[ip_e].key) != 0;

        if (ip_new) {
            p.batch_begin.push_back(static_cast<uint32_t>(p.members.size()));
            p.batch_n.push_back(0);
            p.batch_pool.push_back(p.ents[ip_e].pool_c);   // SKIN_POOL: one chunk a bucket (the key's).
        }

        p.batch_of[p.ents[ip_e].idx] = static_cast<int32_t>(p.batch_n.size() - 1);
        p.batch_n.back()++;
        p.members.push_back(p.ents[ip_e].idx);
    }
}

// The cull walk: per bucket, per member - the pass's visibility verdict, the level it would pick alone with its
// crossfade (a fading object is emitted at both levels, complementary dither), sorted by level then distance
// (near-to-far for a normal-blend bucket so early-Z rejects hidden instances; far-to-near for the other blend
// modes). The stream is written to the ring once; each bucket records its (level, first, n) ranges. Members are
// marked done except the representative. Preconditions: material sets ensured, envelope folded into every colour
// alpha, the pass's distance memo begun on this vector (mesh_order). proj_px: the pass's own level-pick
// scale (PIP_LOD; < 0 = the main view's g_lod.proj_px). zoom: its zoom (LOD_ZOOM; < 0 = g_lod.zoom).
template <class InstVisible>
inline void inst_plan_emit(ID3D11DeviceContext* ctx, ID3D11Device* dev, InstPlan& p,
                           const std::vector<RenderObject>& meshes, const float* cam, int ri,
                           InstVisible visible, DistMemo& dm, float proj_px = -1.0f,
                           float zoom = -1.0f) {
    p.rep.assign(p.batch_n.size(), SCENE_NONE);
    p.range_begin.assign(p.batch_n.size(), 0);
    p.range_n.assign(p.batch_n.size(), 0);
    p.ranges.clear();
    p.lanes.clear();
    p.lane_d2.clear();   // INST_PART_ORDER.
    p.lod_t.assign(meshes.size(), 0.0f);
    p.lod_l.assign(meshes.size(), 0);   // WSEE: the deferral's level pick.

    for (uint32_t ie_b = 0; ie_b < static_cast<uint32_t>(p.batch_n.size()); ++ie_b) {
        p.picked.clear();
        const uint32_t end = p.batch_begin[ie_b] + p.batch_n[ie_b];
        const MeshDef* md = nullptr;

        for (uint32_t m = p.batch_begin[ie_b]; m < end; ++m) {
            const uint32_t i = p.members[m];
            const RenderObject& o = meshes[i];
            p.done[i] = 1;
            if (!visible(o)) continue;
            if (p.rep[ie_b] == SCENE_NONE) { p.rep[ie_b] = i; p.done[i] = 0; }
            if (!md) md = &mesh_def(mesh_id_clamp(o.mesh));
            int lvl = 0;
            float t = 0.0f;
            const float d2 = dm.get(o, cam);   // The pass's memo (seeded by its sort).
            if (!o.lod_lock) {
                lod_pick(*md, lod_radius(o), sqrtf(d2), lvl, t, proj_px,
                         zoom);   // LOD_ZOOM.
            }
            lvl = mesh_lod_clamp(*md, lvl);
            const bool x = t > 0.0f && lvl < static_cast<int>(md->lod_n);
            p.lod_t[i] = x ? t : 0.0f;   // The lane fill below reads it back.
            p.lod_l[i] = lvl;   // WSEE: the deferral reads the pick back too.
            // Distance rank in the low 31 bits (ascending for a normal-blend bucket, descending otherwise); bit 31 =
            // the coarser half of a crossfade pair (complementary per pixel, so the order between halves is free).
            const uint32_t dq = static_cast<uint32_t>(fminf(d2, 2.0e9f)) & 0x7FFFFFFFu;
            const uint32_t ie_far = (o.blend_mode == 0) ? dq : (0x7FFFFFFFu - dq);
            p.picked.emplace_back((static_cast<uint64_t>(lvl) << 32) | ie_far, i);
            if (x) p.picked.emplace_back((static_cast<uint64_t>(lvl + 1) << 32) | ie_far | 0x80000000u, i);
        }

        if (p.picked.empty()) continue;
        // INST_OVERFLOW: the pass's stream is written to the ring in one piece of at most INST_CAP lanes
        // (inst_ring_write); a bucket that would carry it past that draws per object instead - its members
        // leave the bucket and are not done, so the per-object loop culls, picks and draws each. Do not cut the
        // stream instead: the members past the cut, marked done, draw nothing.
        if (p.lanes.size() + p.picked.size() > static_cast<size_t>(INST_CAP)) {
            for (uint32_t m = p.batch_begin[ie_b]; m < end; ++m) {
                p.done[p.members[m]] = 0;
                p.batch_of[p.members[m]] = -1;
            }
            p.rep[ie_b] = SCENE_NONE;
            continue;
        }
        std::stable_sort(p.picked.begin(), p.picked.end(),
                         [](const std::pair<uint64_t, uint32_t>& a, const std::pair<uint64_t, uint32_t>& b) {
                             return a.first < b.first;
                         });
        p.range_begin[ie_b] = static_cast<uint32_t>(p.ranges.size());
        size_t at = 0;

        while (at < p.picked.size()) {
            const int lvl = static_cast<int>(p.picked[at].first >> 32);
            InstRange r;
            r.lvl = lvl;
            r.first = static_cast<uint32_t>(p.lanes.size());
            r.src = r.first;   // INST_PART_ORDER.
            r.n = 0;

            for (; at < p.picked.size() && static_cast<int>(p.picked[at].first >> 32) == lvl; ++at) {
                const RenderObject& o = meshes[p.picked[at].second];
                const bool second = (p.picked[at].first & 0x80000000u) != 0;
                // The crossfade the object would apply alone, from the pick above.
                const float lt = p.lod_t[p.picked[at].second];
                const float dither = lt > 0.0f ? lod_dither(lt, second) : 0.0f;
                InstLane l;
                inst_lane_fill(l, o, dither, mat_inst_base(ctx, dev, obj_textured(o)));
                p.lanes.push_back(l);
                p.lane_d2.push_back(dm.get(o, cam));   // INST_PART_ORDER: the memo's value (the sort's).
                r.n++;
            }

            p.ranges.push_back(r);
            p.range_n[ie_b]++;
        }
    }

    if (p.lanes.empty()) return;
    uint32_t first = 0;
    const uint32_t w = inst_ring_write(ctx, ri, p.lanes.data(),
                                       static_cast<uint32_t>(p.lanes.size()), first);
    // Ring-absolute lane indices; a range past the written prefix (a lost map or
    // a stream longer than the ring) draws nothing for the pass.
    for (InstRange& r : p.ranges) {
        if (r.first + r.n > w) r.n = r.first < w ? w - r.first : 0;
        r.first += first;
    }
}

// SKIN_POOL: the skin pool chunk the bucket rep represents pulls from (null = the mesh's own buffer).
inline const SkinPoolChunk* inst_bucket_pool(const InstPlan& p, uint32_t rep) {
    if (rep >= p.batch_of.size() || p.batch_of[rep] < 0) return nullptr;
    const size_t b = static_cast<size_t>(p.batch_of[rep]);
    return b < p.batch_pool.size() ? p.batch_pool[b] : nullptr;
}
// INST_UNBATCH: hands the bucket rep represents back to the per-object loop, as INST_OVERFLOW does at
// the emit (every member out of the bucket and not done), for a site that finds no instanced VS / layout pair for
// it at the representative (unreachable while the pass gates hold: inst_twins_on, whose vs_inst comes with
// vs_inst_tex and both layouts or not at all; inst_sk_on behind every pooled bucket; the injection's composite
// pair). The representative is the first visible member in the loop's visiting order, so every other member is
// still ahead of the loop, which culls, picks and draws each per object; the representative goes on to its own.
inline void inst_unbatch(InstPlan& p, uint32_t rep) {
    if (rep >= p.batch_of.size() || p.batch_of[rep] < 0) return;
    const size_t b = static_cast<size_t>(p.batch_of[rep]);
    if (b >= p.batch_n.size()) return;
    for (uint32_t m = p.batch_begin[b]; m < p.batch_begin[b] + p.batch_n[b]; ++m) {
        p.done[p.members[m]] = 0;
        p.batch_of[p.members[m]] = -1;
    }
}
// SKIN_POOL: binds c's view (a skin pool chunk, inst_bucket_pool's) at VS t52 when the pass's tracker
// (bound) does not already hold it.
// StateBackup hands t52 back.
inline void inst_sk_bind(ID3D11DeviceContext* ctx, const SkinPoolChunk* c,
                         ID3D11ShaderResourceView*& bound) {
    ID3D11ShaderResourceView* const v = c ? c->srv : nullptr;
    if (!v || v == bound) return;
    ctx->VSSetShaderResources(52, 1, &v);
    bound = v;
}

// INST_PART_ORDER: the lanes of the bucket rep represents, farthest first by the pass's distance memo (a
// stable sort of the ranges' level-major, near-to-far order) - INST_PART_LANE's tail entries in the injection (one
// per lane, sorted there among every other translucent part; inst_draw draws each, part 2).
inline void inst_part_lanes(const InstPlan& p, uint32_t rep, std::vector<InstPartLane>& o) {
    o.clear();
    if (rep >= p.batch_of.size() || p.batch_of[rep] < 0) return;
    const uint32_t ipl_b = static_cast<uint32_t>(p.batch_of[rep]);
    if (ipl_b >= p.range_n.size()) return;
    for (uint32_t k = 0; k < p.range_n[ipl_b]; ++k) {
        const InstRange& r = p.ranges[p.range_begin[ipl_b] + k];
        for (uint32_t i = 0; i < r.n; ++i) {
            const uint32_t s = r.src + i;
            o.push_back({ s < p.lane_d2.size() ? p.lane_d2[s] : 0.0f,
                          r.first + i, r.lvl });
        }
    }
    std::stable_sort(o.begin(), o.end(), [](const InstPartLane& a, const InstPartLane& b) {
        return a.d2 > b.d2;
    });
}

// Draws the bucket rep belongs to: one DrawIndexedInstanced per emitted range (per LOD level), the textured
// per-submesh path when txm is set. INST_PART_ORDER: part 2 (a blend material's translucent texels) instead
// draws its lanes one at a time, farthest first by the pass's distance memo - the translucent tail's order. The
// ranges run near-to-far within a level (a normal-blend bucket's early-Z order, which the solid part keeps) and
// level-major, and the instances of one draw blend in instance order, so one draw lands a far member's translucent
// texels over a near member's. One draw per lane on part 2 (a per-object translucent part's cost); part 1 is
// unchanged. INST_PART_LANE: one (part 2 only) draws that one lane of the walk (inst_part_lanes) - the
// injection's tail interleaves a bucket's lanes with the other parts. Part 2 draws only that lane: the injection's
// tail is the one part-2 caller (the flush's translucent part copies never bucket, inst_eligible), and a part-2
// call without it draws nothing.
// Preconditions (draw_textured's): the representative's CB uploaded (the untextured route; textured,
// draw_textured uploads it - TX_ONE_UPLOAD), its PS bound, the instanced VS + layout bound, the lane ring on
// IA slot 1, blend + dss set. Returns the draw count; inst_out the lanes drawn.
inline uint32_t inst_draw(ID3D11DeviceContext* ctx, ID3D11Device* dev, const InstPlan& p,
                          const std::vector<RenderObject>& meshes, uint32_t rep,
                          ConstantData& cbd, ID3D11Buffer* obj_cb,
                          const MaterialSet* txm, bool ts_ordered,
                          ID3D11RasterizerState*& bound_rs,
                          int id_ctx, ID3D11PixelShader* builtin_ps, int part,
                          uint32_t& inst_out, const InstPartLane* one = nullptr) {
    inst_out = 0;
    if (rep >= p.batch_of.size() || p.batch_of[rep] < 0) return 0;
    const uint32_t b = static_cast<uint32_t>(p.batch_of[rep]);
    if (b >= p.range_n.size()) return 0;
    const RenderObject& ro = meshes[rep];
    const int mid = mesh_id_clamp(ro.mesh);
    const MeshDef& md = mesh_def(mid);
    uint32_t draws = 0;

    // One run of lanes [first, +n) at level lvl.
    auto run = [&](int lvl, uint32_t first, uint32_t n) {
        if (txm) {
            draws += draw_textured(ctx, dev, cbd, obj_cb, *txm, mid, ts_ordered,
                                   bound_rs, id_ctx, builtin_ps, lvl, part,
                                   n, first);
        } else {
            UINT ls = 0, lc = 0;
            mesh_lod_range(md, lvl, ls, lc);

            if (ts_ordered) {   // Interiors of every instance, then exteriors over them.
                if (bound_rs != g_res.rasterizer_front) {
                    ctx->RSSetState(g_res.rasterizer_front);
                    bound_rs = g_res.rasterizer_front;
                }
                ctx->DrawIndexedInstanced(lc, n, ls, 0, first);
                ctx->RSSetState(g_res.rasterizer_cull);
                bound_rs = g_res.rasterizer_cull;
                ctx->DrawIndexedInstanced(lc, n, ls, 0, first);
                draws += 2;
            } else {
                ctx->DrawIndexedInstanced(lc, n, ls, 0, first);
                draws += 1;
            }
        }

        inst_out += n;
    };

    if (part == 2) {   // INST_PART_LANE: one lane of the INST_PART_ORDER walk.
        if (one) run(one->lvl, one->lane, 1u);
        return draws;
    }

    for (uint32_t k = 0; k < p.range_n[b]; ++k) {
        const InstRange& r = p.ranges[p.range_begin[b] + k];
        if (r.n == 0) continue;
        run(r.lvl, r.first, r.n);
    }

    return draws;
}
}