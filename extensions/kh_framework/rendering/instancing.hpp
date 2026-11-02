#pragma once

// The feature half (rendering_integration_features.hpp): instancing: eligibility, keys, the plan and kh_inst_draw.
// Never include it on its own.
namespace RenderIntegration {
static constexpr uint32_t KH_INST_CAP = 65536u;   // Lanes per ring (1 MB).

// KH_SKIN_POOL: the colour buckets' Sk twins - both units' plain and textured pairs and their layout. Any one absent
// keeps every skeletal binding out of the buckets (kh_inst_pool_of), whichever pass plans.
inline bool kh_inst_sk_on() {
    return g_res.vs_inst_sk && g_res.vs_inst_sk_tex && g_res.vs_comp_inst_sk && g_res.vs_comp_inst_sk_tex &&
           g_res.layout_inst_sk;
}
// KH_SKIN_POOL: the skin pool chunk a skeletal binding's bucket draw pulls its pose from, or null (it draws per
// object, its slice bound by the IA offset). Non-null requires: its slot draws a slice of a chunk (the tables,
// kh_mesh_pool_of), the twins exist, and the record the bucket's VS reads (g_objbuf_cpu: what kh_objbuf_sync put on
// the GPU before any bucket draw of the pass) carries this copy's every lane that VS reads - the base (res.w)
// naming the slice the tables name, and the box that slice's bytes are normalised to. A copy the record has not
// caught up with (a base the next sync carries, a box a flush's catch-up moved under KH_SCENE_HOLD - KH_SKIN_CULL)
// draws per object that pass. The tables' thread; also from kh_inst_plan_build's fork (reads only).
inline const KhSkinPoolChunk* kh_inst_pool_of(const RenderObject& o) {
    if (!o.skel || !kh_inst_sk_on()) return nullptr;
    const KhSkinPoolChunk* const khip_c = kh_mesh_pool_of(mesh_id_clamp(o.mesh), o.slot);
    if (!khip_c || o.slot >= g_cloth_voff_slot.size() || o.slot >= g_objbuf_cpu.size()) return nullptr;
    if (static_cast<uint64_t>(g_cloth_voff_slot[o.slot]) !=
        static_cast<uint64_t>(o.skin_vbase) * sizeof(MeshVertex)) return nullptr;
    KhObjRec khip_r;
    kh_objrec_fill(khip_r, o);
    const KhObjRec& khip_m = g_objbuf_cpu[o.slot];
    // Every lane but the colour's alpha (the copy's carries the envelope; the bucket's alpha is the lane's).
    if (memcmp(&khip_r, &khip_m, offsetof(KhObjRec, col)) != 0 ||
        memcmp(khip_r.col, khip_m.col, 3u * sizeof(float)) != 0 ||
        memcmp(khip_r.res, khip_m.res, sizeof(khip_r.res)) != 0) return nullptr;
    return khip_c;
}

// Bucket eligibility: a mesh drawn from its mesh's shared vertex buffer. Kept per object: a normal-blend whole
// translucent, an effect mesh, a fullscreen pass, depth-Off, a localized / banded mesh (it reads its own mask
// lanes), an exact lighten / darken (below), a simulated or vertex-staged mesh, a slot drawing a substitute and a
// blend material's translucent part copy (KH_INST_PART_LANE); the other blend modes are eligible at any alpha, as
// are the solid part copies. KH_SKIN_POOL: khel_pc (eligible only) = the skin pool chunk the object's bucket pulls
// from, null for any other.
// KH_BLEND_MINMAX: a lighten / darken mesh - not an effect mesh (KhFxFinish blends those exactly), not a fullscreen
// pass, not a blend material's translucent part (hardware-blended like every part) - whose opacity can fall below
// 1: colour alpha below 1 (the envelope is folded in by the flush's collection), or banded / localized (SolidMask).
// Those are the twins' whole a here: the flush draws a blend material's solid texels only, at alpha 1
// (kh_draw_textured's part 1). The MIN / MAX blend ops take no factors, so the hardware forms are exact at a = 1
// only: lighten max(scene, colour x a), darken min(scene, lerp(65504, colour, a)) - the scene itself for any a
// short of 1 (a darken mesh at opacity 0.8 darkened nothing). The flush draws such a mesh on its own (not instanced)
// against its own copy of the scene taken just before it (kh_mm_capture), and the twins write the exact lerp(scene,
// max / min(scene, colour), a) (blendCtl.x), so each one reads the scene the meshes before it left. The hardware
// forms stand at a = 1 (exact; instanced, no copy), when the copy cannot be made, and for an inFront mesh (the
// hands slice draws it).
inline bool kh_blend_minmax_exact(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.draw_part != 1 && (o.blend_mode == 4 || o.blend_mode == 5) &&
           (o.color[3] < 1.0f || o.localized || o.banded);   // Strict: darken at a = 0.9995 adds ~33.
}

inline bool kh_inst_eligible(const RenderObject& o, const KhSkinPoolChunk*& khel_pc) {
    khel_pc = nullptr;
    // KH_CLOTH: a bucket binds ONE vertex buffer for every member, and a simulated mesh has a buffer of its own -
    // two cloths sharing a mesh would otherwise both draw whichever one's buffer the bucket bound. The per-object
    // path costs a draw call, which translucents and effect meshes already pay.
    if (o.cloth_sim) return false;
    if (o.chain_sim) return false;   // KH_CHAIN: the same reason.
    // KH_INST_PART_LANE: the flush's copy of a blend material's translucent part draws per object, in the tail's
    // far-to-near order among every other translucent (a bucket drew all its members' parts at the farthest member's
    // place: a nearer copy's glass went down before the translucents between them). The draws are the same either
    // way (one per lane, KH_INST_PART_ORDER); per object it pays its own constant-buffer fill and, lit, its own
    // light-ring append, as every other translucent of the tail does.
    if (o.draw_part == 1) return false;
    if (kh_blend_minmax_exact(o)) return false;   // KH_BLEND_MINMAX: each against its own capture.
    // KH_SKEL: the same reason - its skinned buffer is its own. KH_SKIN_POOL: unless it is a slice of a skin pool
    // chunk, which its bucket's Sk twin pulls each member's own pose from (the chunk keys the bucket: KhInstKey::pool).
    if (o.skel) khel_pc = kh_inst_pool_of(o);
    const bool khel_pool = khel_pc != nullptr;
    if (o.skel && !khel_pool) return false;
    if (o.materials && o.materials->vertex_any) return false;   // KH_USER_VS: the same reason - its deformed buffer is its own.
    // KH_LOD_SIM_SLOT: nor while its slot still draws a substitute - a simulation's or skin's buffer stays until
    // the next flush after the flag that set it ends (and a bucket's level could read a cloth's rest-pose tail).
    // KH_SKIN_POOL: a pooled binding's substitute is the chunk its bucket pulls from.
    if (!khel_pool) {
        const int khel_mid = mesh_id_clamp(o.mesh);
        if (khel_mid >= 0 && static_cast<size_t>(khel_mid) < g_res.mesh_vb.size() &&
            kh_mesh_vb_ref(khel_mid, o.slot).vb != g_res.mesh_vb[static_cast<size_t>(khel_mid)]) return false;
    }
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           !(o.blend_mode == 0 && o.color[3] < 0.999f) && !o.localized && !o.banded;
}

inline bool kh_inst_twins_on() {
    return g_res.vs_inst && g_res.layout_inst && g_res.inst_vb[0] && g_res.inst_vb[1] && g_res.obj_srv;
}

inline void kh_inst_lane_fill(KhInstLane& l, const RenderObject& o, float khil_dither, float khil_mat_base) {
    l.slot = o.slot;
    l.dither = khil_dither;
    l.alpha = o.color[3];   // The staging copy's alpha - envelope already folded (fold first, then plan).
    l.mat_base = khil_mat_base;
}

// Writes up to KH_INST_CAP lanes; returns the count (0 on a failed map, which re-DISCARDs the ring next time) and
// the first lane's index. A full ring wraps with a discard: every earlier draw of the pass already consumed its lanes.
inline uint32_t kh_inst_ring_write(ID3D11DeviceContext* ctx, int khiw_ri, const KhInstLane* khiw_recs,
                                   uint32_t khiw_n, uint32_t& khiw_first) {
    ID3D11Buffer* khiw_buf = g_res.inst_vb[khiw_ri];
    uint32_t& khiw_cur = g_res.inst_cursor[khiw_ri];
    bool& khiw_fresh = g_res.inst_fresh[khiw_ri];
    khiw_first = 0;
    if (!khiw_buf || khiw_n == 0) return 0;
    if (khiw_n > KH_INST_CAP) khiw_n = KH_INST_CAP;
    if (khiw_cur + khiw_n > KH_INST_CAP) { khiw_cur = 0; khiw_fresh = true; }
    D3D11_MAPPED_SUBRESOURCE khiw_m = {};
    const D3D11_MAP khiw_mode = khiw_fresh ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

    if (FAILED(ctx->Map(khiw_buf, 0, khiw_mode, 0, &khiw_m))) {
        khiw_fresh = true;
        return 0;
    }

    khiw_fresh = false;
    memcpy(static_cast<uint8_t*>(khiw_m.pData) + static_cast<size_t>(khiw_cur) * KH_INST_STRIDE,
           khiw_recs, static_cast<size_t>(khiw_n) * KH_INST_STRIDE);
    ctx->Unmap(khiw_buf, 0);
    khiw_first = khiw_cur;
    khiw_cur += khiw_n;
    return khiw_n;
}

// route / aux are filled by kh_inst_plan_build's functor. A set not yet uploaded keys on its content hash (fallback
// only; the passes ensure every set before planning).
inline KhInstKey kh_inst_key(const RenderObject& o) {
    KhInstKey k;
    k.mesh = mesh_id_clamp(o.mesh);
    k.pool = 0u;   // KH_SKIN_POOL: kh_inst_plan_build's (the chunk kh_inst_eligible found).
    {
        const KhMaterialSet* khik_ms = kh_obj_textured(o);
        // |1 keeps a zero hash distinct from "untextured".
        k.mats = khik_ms ? ((khik_ms->gpu.base >= 0 ? khik_ms->gpu.page_sig : khik_ms->hash) | 1u) : 0u;
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

inline int kh_inst_key_cmp(const KhInstKey& a, const KhInstKey& b) {
    if (a.mesh != b.mesh) return a.mesh < b.mesh ? -1 : 1;
    if (a.pool != b.pool) return a.pool < b.pool ? -1 : 1;   // KH_SKIN_POOL (0 everywhere but a skeletal bucket).
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

// khip_order = the loop's visiting order (nullptr = natural). Groups by key; members keep their visiting order
// inside a bucket. khip_route(o, key) fills the pass's per-object key lanes.
template <class KhInstRoute>
inline void kh_inst_plan_build(KhInstPlan& p, const std::vector<RenderObject>& meshes,
                               const uint32_t* khip_order, size_t khip_n, KhInstRoute khip_route) {
    KH_PROF_SCOPE(KHP_INST_PLAN);   // KH_PROF.
    p.batch_of.assign(meshes.size(), -1);
    p.done.assign(meshes.size(), 0);
    p.batch_begin.clear();
    p.batch_n.clear();
    p.batch_pool.clear();   // KH_SKIN_POOL.
    p.members.clear();
    p.ents.clear();
    p.rep.clear();
    p.range_begin.clear();
    p.range_n.clear();
    p.ranges.clear();
    p.lanes.clear();
    p.lane_d2.clear();   // KH_INST_PART_ORDER.

    {   // KH_INJ_FORK: every rank's key in parallel (its own slot; a hole where not eligible), then the compaction in rank order.
        static std::vector<KhInstPlan::Ent> khip_scr;
        static std::vector<uint8_t> khip_on;
        khip_scr.resize(khip_n);
        khip_on.assign(khip_n, 0u);
        kh_fork_each(static_cast<uint32_t>(khip_n), [&](uint32_t khip_r) {
            const uint32_t khip_i = khip_order ? khip_order[khip_r] : khip_r;
            if (khip_i >= meshes.size()) return;
            const RenderObject& o = meshes[khip_i];
            const KhSkinPoolChunk* khip_pc = nullptr;   // KH_SKIN_POOL.
            if (!kh_inst_eligible(o, khip_pc)) return;
            KhInstPlan::Ent& e = khip_scr[khip_r];
            e.key = kh_inst_key(o);
            e.key.pool = khip_pc ? khip_pc->serial : 0u;   // KH_SKIN_POOL: the bucket is the chunk's.
            e.pool_c = khip_pc;
            khip_route(o, e.key);
            e.rank = khip_r;
            e.idx = khip_i;
            khip_on[khip_r] = 1u;
        });
        for (size_t khip_r = 0; khip_r < khip_n; ++khip_r) if (khip_on[khip_r]) p.ents.push_back(khip_scr[khip_r]);
    }

    if (p.ents.empty()) return;
    std::stable_sort(p.ents.begin(), p.ents.end(), [](const KhInstPlan::Ent& a, const KhInstPlan::Ent& b) {
        const int c = kh_inst_key_cmp(a.key, b.key);
        return c != 0 ? c < 0 : a.rank < b.rank;
    });

    for (size_t khip_e = 0; khip_e < p.ents.size(); ++khip_e) {
        const bool khip_new = khip_e == 0 || kh_inst_key_cmp(p.ents[khip_e - 1].key, p.ents[khip_e].key) != 0;

        if (khip_new) {
            p.batch_begin.push_back(static_cast<uint32_t>(p.members.size()));
            p.batch_n.push_back(0);
            p.batch_pool.push_back(p.ents[khip_e].pool_c);   // KH_SKIN_POOL: one chunk a bucket (the key's).
        }

        p.batch_of[p.ents[khip_e].idx] = static_cast<int32_t>(p.batch_n.size() - 1);
        p.batch_n.back()++;
        p.members.push_back(p.ents[khip_e].idx);
    }
}

// The cull walk: per bucket, per member - the pass's visibility verdict, the level it would pick alone with its
// crossfade (a fading object is emitted at both levels, complementary dither), sorted by level then distance
// (near-to-far for a normal-blend bucket so early-Z rejects hidden instances; far-to-near for the other blend
// modes). The stream is written to the ring once; each bucket records its (level, first, n) ranges. Members are
// marked done except the representative. Preconditions: material sets ensured, envelope folded into every colour
// alpha, the pass's distance memo begun on this vector (kh_mesh_order). khie_proj_px: the pass's own level-pick
// scale (KH_PIP_LOD; < 0 = the main view's g_lod_proj_px). khie_zoom: its zoom (KH_LOD_ZOOM; < 0 = g_lod_zoom).
template <class KhInstVisible>
inline void kh_inst_plan_emit(ID3D11DeviceContext* ctx, ID3D11Device* dev, KhInstPlan& p,
                              const std::vector<RenderObject>& meshes, const float* cam, int khie_ri,
                              KhInstVisible khie_visible, KhDistMemo& khie_dm, float khie_proj_px = -1.0f,
                              float khie_zoom = -1.0f) {
    p.rep.assign(p.batch_n.size(), KH_SCENE_NONE);
    p.range_begin.assign(p.batch_n.size(), 0);
    p.range_n.assign(p.batch_n.size(), 0);
    p.ranges.clear();
    p.lanes.clear();
    p.lane_d2.clear();   // KH_INST_PART_ORDER.
    p.lod_t.assign(meshes.size(), 0.0f);
    p.lod_l.assign(meshes.size(), 0);   // KH_WSEE: the deferral's level pick.

    for (uint32_t khie_b = 0; khie_b < static_cast<uint32_t>(p.batch_n.size()); ++khie_b) {
        p.picked.clear();
        const uint32_t khie_end = p.batch_begin[khie_b] + p.batch_n[khie_b];
        const MeshDef* khie_md = nullptr;

        for (uint32_t khie_m = p.batch_begin[khie_b]; khie_m < khie_end; ++khie_m) {
            const uint32_t khie_i = p.members[khie_m];
            const RenderObject& o = meshes[khie_i];
            p.done[khie_i] = 1;
            if (!khie_visible(o)) continue;
            if (p.rep[khie_b] == KH_SCENE_NONE) { p.rep[khie_b] = khie_i; p.done[khie_i] = 0; }
            if (!khie_md) khie_md = &mesh_def(mesh_id_clamp(o.mesh));
            int khie_lvl = 0;
            float khie_t = 0.0f;
            const float khie_d2 = khie_dm.get(o, cam);   // The pass's memo (seeded by its sort).
            if (!o.lod_lock) {
                kh_lod_pick(*khie_md, kh_lod_radius(o), sqrtf(khie_d2), khie_lvl, khie_t, khie_proj_px,
                            khie_zoom);   // KH_LOD_ZOOM.
            }
            khie_lvl = mesh_lod_clamp(*khie_md, khie_lvl);
            const bool khie_x = khie_t > 0.0f && khie_lvl < static_cast<int>(khie_md->lod_n);
            p.lod_t[khie_i] = khie_x ? khie_t : 0.0f;   // The lane fill below reads it back.
            p.lod_l[khie_i] = khie_lvl;   // KH_WSEE: the deferral reads the pick back too.
            // Distance rank in the low 31 bits (ascending for a normal-blend bucket, descending otherwise); bit 31 =
            // the coarser half of a crossfade pair (complementary per pixel, so the order between halves is free).
            const uint32_t khie_dq = static_cast<uint32_t>(fminf(khie_d2, 2.0e9f)) & 0x7FFFFFFFu;
            const uint32_t khie_far = (o.blend_mode == 0) ? khie_dq : (0x7FFFFFFFu - khie_dq);
            p.picked.emplace_back((static_cast<uint64_t>(khie_lvl) << 32) | khie_far, khie_i);
            if (khie_x) p.picked.emplace_back((static_cast<uint64_t>(khie_lvl + 1) << 32) | khie_far | 0x80000000u, khie_i);
        }

        if (p.picked.empty()) continue;
        // KH_INST_OVERFLOW: the pass's stream is written to the ring in one piece of at most KH_INST_CAP lanes
        // (kh_inst_ring_write); a bucket that would carry it past that draws per object instead - its members
        // leave the bucket and are not done, so the per-object loop culls, picks and draws each. Do not cut the
        // stream instead: the members past the cut, marked done, draw nothing.
        if (p.lanes.size() + p.picked.size() > static_cast<size_t>(KH_INST_CAP)) {
            for (uint32_t khie_m = p.batch_begin[khie_b]; khie_m < khie_end; ++khie_m) {
                p.done[p.members[khie_m]] = 0;
                p.batch_of[p.members[khie_m]] = -1;
            }
            p.rep[khie_b] = KH_SCENE_NONE;
            continue;
        }
        std::stable_sort(p.picked.begin(), p.picked.end(),
                         [](const std::pair<uint64_t, uint32_t>& a, const std::pair<uint64_t, uint32_t>& b) {
                             return a.first < b.first;
                         });
        p.range_begin[khie_b] = static_cast<uint32_t>(p.ranges.size());
        size_t khie_at = 0;

        while (khie_at < p.picked.size()) {
            const int khie_lvl = static_cast<int>(p.picked[khie_at].first >> 32);
            KhInstRange khie_r;
            khie_r.lvl = khie_lvl;
            khie_r.first = static_cast<uint32_t>(p.lanes.size());
            khie_r.src = khie_r.first;   // KH_INST_PART_ORDER.
            khie_r.n = 0;

            for (; khie_at < p.picked.size() && static_cast<int>(p.picked[khie_at].first >> 32) == khie_lvl; ++khie_at) {
                const RenderObject& o = meshes[p.picked[khie_at].second];
                const bool khie_second = (p.picked[khie_at].first & 0x80000000u) != 0;
                // The crossfade the object would apply alone, from the pick above.
                const float khie_lt = p.lod_t[p.picked[khie_at].second];
                const float khie_dither = khie_lt > 0.0f ? kh_lod_dither(khie_lt, khie_second) : 0.0f;
                KhInstLane khie_l;
                kh_inst_lane_fill(khie_l, o, khie_dither, kh_mat_inst_base(ctx, dev, kh_obj_textured(o)));
                p.lanes.push_back(khie_l);
                p.lane_d2.push_back(khie_dm.get(o, cam));   // KH_INST_PART_ORDER: the memo's value (the sort's).
                khie_r.n++;
            }

            p.ranges.push_back(khie_r);
            p.range_n[khie_b]++;
        }
    }

    if (p.lanes.empty()) return;
    uint32_t khie_first = 0;
    const uint32_t khie_w = kh_inst_ring_write(ctx, khie_ri, p.lanes.data(),
                                               static_cast<uint32_t>(p.lanes.size()), khie_first);
    // Ring-absolute lane indices; a range past the written prefix (a lost map or
    // a stream longer than the ring) draws nothing for the pass.
    for (KhInstRange& khie_r : p.ranges) {
        if (khie_r.first + khie_r.n > khie_w) khie_r.n = khie_r.first < khie_w ? khie_w - khie_r.first : 0;
        khie_r.first += khie_first;
    }
}

// KH_SKIN_POOL: the skin pool chunk the bucket khib_rep represents pulls from (null = the mesh's own buffer).
inline const KhSkinPoolChunk* kh_inst_bucket_pool(const KhInstPlan& p, uint32_t khib_rep) {
    if (khib_rep >= p.batch_of.size() || p.batch_of[khib_rep] < 0) return nullptr;
    const size_t khib_b = static_cast<size_t>(p.batch_of[khib_rep]);
    return khib_b < p.batch_pool.size() ? p.batch_pool[khib_b] : nullptr;
}
// KH_INST_UNBATCH: hands the bucket khiu_rep represents back to the per-object loop, as KH_INST_OVERFLOW does at
// the emit (every member out of the bucket and not done), for a site that finds no instanced VS / layout pair for
// it at the representative (unreachable while the pass gates hold: kh_inst_twins_on, whose vs_inst comes with
// vs_inst_tex and both layouts or not at all; kh_inst_sk_on behind every pooled bucket; the injection's composite
// pair). The representative is the first visible member in the loop's visiting order, so every other member is
// still ahead of the loop, which culls, picks and draws each per object; the representative goes on to its own.
inline void kh_inst_unbatch(KhInstPlan& p, uint32_t khiu_rep) {
    if (khiu_rep >= p.batch_of.size() || p.batch_of[khiu_rep] < 0) return;
    const size_t khiu_b = static_cast<size_t>(p.batch_of[khiu_rep]);
    if (khiu_b >= p.batch_n.size()) return;
    for (uint32_t khiu_m = p.batch_begin[khiu_b]; khiu_m < p.batch_begin[khiu_b] + p.batch_n[khiu_b]; ++khiu_m) {
        p.done[p.members[khiu_m]] = 0;
        p.batch_of[p.members[khiu_m]] = -1;
    }
}
// KH_SKIN_POOL: binds khsb_c's view (a skin pool chunk, kh_inst_bucket_pool's) at VS t52 when the pass's tracker
// (khsb_bound) does not already hold it.
// StateBackup hands t52 back.
inline void kh_inst_sk_bind(ID3D11DeviceContext* ctx, const KhSkinPoolChunk* khsb_c,
                            ID3D11ShaderResourceView*& khsb_bound) {
    ID3D11ShaderResourceView* const khsb_v = khsb_c ? khsb_c->srv : nullptr;
    if (!khsb_v || khsb_v == khsb_bound) return;
    ctx->VSSetShaderResources(52, 1, &khsb_v);
    khsb_bound = khsb_v;
}

// KH_INST_PART_ORDER: the lanes of the bucket khipl_rep represents, farthest first by the pass's distance memo (a
// stable sort of the ranges' level-major, near-to-far order) - kh_inst_draw's part-2 walk, and KH_INST_PART_LANE's
// tail entries in the injection (one per lane, sorted there among every other translucent part).
inline void kh_inst_part_lanes(const KhInstPlan& p, uint32_t khipl_rep, std::vector<KhInstPartLane>& khipl_o) {
    khipl_o.clear();
    if (khipl_rep >= p.batch_of.size() || p.batch_of[khipl_rep] < 0) return;
    const uint32_t khipl_b = static_cast<uint32_t>(p.batch_of[khipl_rep]);
    if (khipl_b >= p.range_n.size()) return;
    for (uint32_t khipl_k = 0; khipl_k < p.range_n[khipl_b]; ++khipl_k) {
        const KhInstRange& khipl_r = p.ranges[p.range_begin[khipl_b] + khipl_k];
        for (uint32_t khipl_i = 0; khipl_i < khipl_r.n; ++khipl_i) {
            const uint32_t khipl_s = khipl_r.src + khipl_i;
            khipl_o.push_back({ khipl_s < p.lane_d2.size() ? p.lane_d2[khipl_s] : 0.0f,
                                khipl_r.first + khipl_i, khipl_r.lvl });
        }
    }
    std::stable_sort(khipl_o.begin(), khipl_o.end(), [](const KhInstPartLane& a, const KhInstPartLane& b) {
        return a.d2 > b.d2;
    });
}

// Draws the bucket khid_rep belongs to: one DrawIndexedInstanced per emitted range (per LOD level), the textured
// per-submesh path when khid_txm is set. KH_INST_PART_ORDER: part 2 (a blend material's translucent texels) instead
// draws its lanes one at a time, farthest first by the pass's distance memo - the translucent tail's order. The
// ranges run near-to-far within a level (a normal-blend bucket's early-Z order, which the solid part keeps) and
// level-major, and the instances of one draw blend in instance order, so one draw lands a far member's translucent
// texels over a near member's. One draw per lane on part 2 (a per-object translucent part's cost); part 1 is
// unchanged. KH_INST_PART_LANE: khid_one (part 2 only) draws that one lane of the walk (kh_inst_part_lanes) - the
// injection's tail interleaves a bucket's lanes with the other parts. Every part-2 caller passes it (the flush
// draws its parts per object), so the whole walk without it is unused.
// Preconditions (kh_draw_textured's): the representative's CB uploaded (the untextured route; textured,
// kh_draw_textured uploads it - KH_TX_ONE_UPLOAD), its PS bound, the instanced VS + layout bound, the lane ring on
// IA slot 1, blend + dss set. Returns the draw count; khid_inst_out the lanes drawn.
inline uint32_t kh_inst_draw(ID3D11DeviceContext* ctx, ID3D11Device* dev, const KhInstPlan& p,
                             const std::vector<RenderObject>& meshes, uint32_t khid_rep,
                             ConstantData& cbd, ID3D11Buffer* obj_cb,
                             const KhMaterialSet* khid_txm, bool khid_ts_ordered,
                             ID3D11RasterizerState*& khid_bound_rs,
                             int khid_ctx, ID3D11PixelShader* khid_builtin_ps, int khid_part,
                             uint32_t& khid_inst_out, const KhInstPartLane* khid_one = nullptr) {
    khid_inst_out = 0;
    if (khid_rep >= p.batch_of.size() || p.batch_of[khid_rep] < 0) return 0;
    const uint32_t khid_b = static_cast<uint32_t>(p.batch_of[khid_rep]);
    if (khid_b >= p.range_n.size()) return 0;
    const RenderObject& khid_ro = meshes[khid_rep];
    const int khid_mid = mesh_id_clamp(khid_ro.mesh);
    const MeshDef& khid_md = mesh_def(khid_mid);
    uint32_t khid_draws = 0;

    // One run of lanes [khid_first, +khid_n) at level khid_lvl.
    auto khid_run = [&](int khid_lvl, uint32_t khid_first, uint32_t khid_n) {
        if (khid_txm) {
            khid_draws += kh_draw_textured(ctx, dev, cbd, obj_cb, *khid_txm, khid_mid, khid_ts_ordered,
                                           khid_bound_rs, khid_ctx, khid_builtin_ps, khid_lvl, khid_part,
                                           khid_n, khid_first);
        } else {
            UINT khid_ls = 0, khid_lc = 0;
            mesh_lod_range(khid_md, khid_lvl, khid_ls, khid_lc);

            if (khid_ts_ordered) {   // Interiors of every instance, then exteriors over them.
                if (khid_bound_rs != g_res.rasterizer_front) {
                    ctx->RSSetState(g_res.rasterizer_front);
                    khid_bound_rs = g_res.rasterizer_front;
                }
                ctx->DrawIndexedInstanced(khid_lc, khid_n, khid_ls, 0, khid_first);
                ctx->RSSetState(g_res.rasterizer_cull);
                khid_bound_rs = g_res.rasterizer_cull;
                ctx->DrawIndexedInstanced(khid_lc, khid_n, khid_ls, 0, khid_first);
                khid_draws += 2;
            } else {
                ctx->DrawIndexedInstanced(khid_lc, khid_n, khid_ls, 0, khid_first);
                khid_draws += 1;
            }
        }

        khid_inst_out += khid_n;
    };

    if (khid_part == 2) {   // KH_INST_PART_ORDER: every lane of the bucket, farthest first.
        if (khid_one) {   // KH_INST_PART_LANE: one lane of that walk.
            khid_run(khid_one->lvl, khid_one->lane, 1u);
            return khid_draws;
        }
        kh_inst_part_lanes(p, khid_rep, p.part_order);
        for (const KhInstPartLane& khid_l : p.part_order) khid_run(khid_l.lvl, khid_l.lane, 1u);
        return khid_draws;
    }

    for (uint32_t khid_k = 0; khid_k < p.range_n[khid_b]; ++khid_k) {
        const KhInstRange& khid_r = p.ranges[p.range_begin[khid_b] + khid_k];
        if (khid_r.n == 0) continue;
        khid_run(khid_r.lvl, khid_r.first, khid_r.n);
    }

    return khid_draws;
}
}