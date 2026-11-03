#pragma once

// The feature half (rendering_integration_features.hpp): KH_SKIN_GPU / KH_SKEL: skinning on the CPU and the GPU (stream
// output), the skin pools, the sync and upload. Never include it on its own.
namespace RenderIntegration {
inline void kh_skin_release_all();   // KH_SKEL: the skinned vertex buffers (defined after the cloth runtime).
inline void kh_uvs_src_forget(uint32_t khsg_slot);   // KH_UVS_SRC_FORGET: a slot's source buffer was replaced.
inline void kh_skin_drop_all();
// KH_SKEL_DRAW (defined with the skin): the consume-point skinning of one skeletal binding, from the stash
// kh_attach_step just took, in three parts (KH_SKIN_FORK): the maps (render thread), the vertices (any thread - the
// pool's), the upload and box (render thread).
inline bool kh_skin_draw_prep(KhAttach& khsd_a, const RenderObject& khsd_o, ID3D11DeviceContext* khsd_ctx);
inline void kh_skin_draw_verts(KhAttach& khsd_a, float khsd_bc[3], float khsd_bs[3]);
inline void kh_skin_draw_finish(const std::string& khsd_h, KhAttach& khsd_a, RenderObject& khsd_o,
                                ID3D11DeviceContext* khsd_ctx, const float khsd_bc[3], const float khsd_bs[3],
                                bool& khsd_moved, const KhVbRef& khsd_so_vb);
// Does something other than the draw read this instance's pose (its cloth's guide, its collider)? The step's stash
// gate for a hidden mesh (defined with the skin).
inline bool kh_skin_pose_read(const KhSkinInst& khpr_in);
// KH_SKIN_GPU (defined after the hook state it raises): the GPU skinning of every binding of a step that can take it.
inline void kh_skin_so_batch(ID3D11DeviceContext* khsb_ctx, std::vector<KhSkinDrawJob>& khsb_jobs);

// Defined in rendering/device_resources.hpp (declared here for the uses below).
inline bool kh_ensure_ok(const char* khe_what, const std::string& khe_err);

// KH_SKIN_CASTER: whether a skeletal binding's pose is drawn - it is visible, or a casterOnly binding a shadow map
// takes (kh_shadow_active: it casts and takes part - lit, no effect, depth on - so the sun / DLS maps draw it;
// kh_uvs_step's gate calls this). The step's stash and skin gates and kh_skin_sync's ask this, so a hidden
// caster does not cast a frozen pose. KH_SKIN_CASTER_VM: an inFront one in any view (kh_shadow_active_any_view: the
// slice term left out) - the slice verdict moves on the render thread at the main depth clear, after the game
// thread's step, so a gate that took it would give the maps a pose one or more frames old at a third -> first
// person switch.
inline bool kh_skin_pose_drawn(const RenderObject& khpd_o) {
    return kh_obj_vis(khpd_o) || (khpd_o.caster_only && kh_shadow_active_any_view(khpd_o));   // KH_SHOW_INFRONT.
}
inline void kh_skin_stat_note(uint32_t khsn_gpu, uint32_t khsn_cpu, uint32_t khsn_cull) {
    if (g_skin_stat_cycle != g_topo_cycles) {
        if (g_skin_stat_cycle != ~0ull) {
            g_skin_so_n.store(g_skin_stat_gpu, std::memory_order_relaxed);
            g_skin_cpu_n.store(g_skin_stat_cpu, std::memory_order_relaxed);
            g_skin_cull_n.store(g_skin_stat_cull, std::memory_order_relaxed);
        }
        g_skin_stat_cycle = g_topo_cycles;
        g_skin_stat_gpu = 0u;
        g_skin_stat_cpu = 0u;
        g_skin_stat_cull = 0u;
    }
    g_skin_stat_gpu += khsn_gpu;
    g_skin_stat_cpu += khsn_cpu;
    g_skin_stat_cull += khsn_cull;
}
inline void kh_skin_nrm_cof(const float khsn_r[3][3], const float khsn_n[3], float khsn_nn[3]) {
    const float* const r0 = khsn_r[0];
    const float* const r1 = khsn_r[1];
    const float* const r2 = khsn_r[2];
    const float* const n = khsn_n;
    khsn_nn[0] = n[0] * r0[0] + n[1] * r0[1] + n[2] * r0[2];   // n . B: dot(n, row c).
    khsn_nn[1] = n[0] * r1[0] + n[1] * r1[1] + n[2] * r1[2];
    khsn_nn[2] = n[0] * r2[0] + n[1] * r2[1] + n[2] * r2[2];
    // x0 / x1 / x2 = cross(r1, r2) / cross(r2, r0) / cross(r0, r1) in HLSL cross's terms; c = their dots with n.
    const float x0[3] = { r1[1] * r2[2] - r1[2] * r2[1], r1[2] * r2[0] - r1[0] * r2[2], r1[0] * r2[1] - r1[1] * r2[0] };
    const float x1[3] = { r2[1] * r0[2] - r2[2] * r0[1], r2[2] * r0[0] - r2[0] * r0[2], r2[0] * r0[1] - r2[1] * r0[0] };
    const float x2[3] = { r0[1] * r1[2] - r0[2] * r1[1], r0[2] * r1[0] - r0[0] * r1[2], r0[0] * r1[1] - r0[1] * r1[0] };
    const float khsn_c[3] = { n[0] * x0[0] + n[1] * x0[1] + n[2] * x0[2], n[0] * x1[0] + n[1] * x1[1] + n[2] * x1[2],
                              n[0] * x2[0] + n[1] * x2[1] + n[2] * x2[2] };
    const float khsn_lc = khsn_c[0] * khsn_c[0] + khsn_c[1] * khsn_c[1] + khsn_c[2] * khsn_c[2];
    const float khsn_la = khsn_nn[0] * khsn_nn[0] + khsn_nn[1] * khsn_nn[1] + khsn_nn[2] * khsn_nn[2];
    if (!(khsn_lc > KH_SKIN_NRM_LC_MIN && khsn_lc < KH_SKIN_NRM_L_MAX && khsn_la < KH_SKIN_NRM_L_MAX)) return;
    const float khsn_det = r0[0] * x0[0] + r0[1] * x0[1] + r0[2] * x0[2];
    const float khsn_g = (khsn_det >= 0.0f ? 1.0f : -1.0f) * (sqrtf(khsn_la) / sqrtf(khsn_lc));
    for (int k = 0; k < 3; ++k) khsn_nn[k] = khsn_c[k] * khsn_g;
}
// The upload's publish, under g_draw_list_mutex (the box writes' scope). The table is handed over whole and never
// written after, so the cloth sync shares it.
inline void kh_skin_col_publish(KhSkinColMap& khsp_col) {
    std::shared_ptr<const KhSkinColMap> khsp_t = std::make_shared<const KhSkinColMap>(std::move(khsp_col));
    std::lock_guard<std::mutex> khsp_l(g_skin_col_mu);
    g_skin_col.swap(khsp_t);
    g_skin_col_pub = g_skin_col_stamp;
}   // The previous table is released with khsp_t, outside the lock.
// The cloth sync's take, once per call: under g_draw_list_mutex beside the
// object copy when it simulates (khst_snap), alone on the inert path.
inline void kh_skin_col_take(bool khst_snap) {
    std::lock_guard<std::mutex> khst_l(g_skin_col_mu);
    g_skin_col_lo = g_skin_col_hi;
    g_skin_col_hi = g_skin_col_pub;
    if (khst_snap) g_skin_col_snap = g_skin_col;
}
inline bool kh_skin_col_fresh(uint64_t khsf_s) {
    return khsf_s > g_skin_col_lo && khsf_s <= g_skin_col_hi;
}
// One bone's tree over khsb_ix (indices into khsb_p, 3 floats a point; reordered in place), appended to khsb_s.
// Returns the root.
inline uint32_t kh_skin_sup_build(KhSkinSup& khsb_s, const std::vector<float>& khsb_p, std::vector<uint32_t>& khsb_ix) {
    struct KhSbTask { uint32_t node, lo, hi; };
    const uint32_t khsb_root = static_cast<uint32_t>(khsb_s.node.size());
    khsb_s.node.emplace_back();
    std::vector<KhSbTask> khsb_st;
    khsb_st.push_back(KhSbTask{ khsb_root, 0u, static_cast<uint32_t>(khsb_ix.size()) });
    while (!khsb_st.empty()) {
        const KhSbTask khsb_t = khsb_st.back();
        khsb_st.pop_back();
        float mn[3] = { 3.0e38f, 3.0e38f, 3.0e38f }, mx[3] = { -3.0e38f, -3.0e38f, -3.0e38f };
        for (uint32_t i = khsb_t.lo; i < khsb_t.hi; ++i) {
            const float* p = &khsb_p[static_cast<size_t>(khsb_ix[i]) * 3u];
            for (int k = 0; k < 3; ++k) { if (p[k] < mn[k]) mn[k] = p[k]; if (p[k] > mx[k]) mx[k] = p[k]; }
        }
        {
            KhSkinSupNode& n = khsb_s.node[khsb_t.node];
            memcpy(n.mn, mn, sizeof(mn));
            memcpy(n.mx, mx, sizeof(mx));
            n.left = 0u; n.first = 0u; n.count = 0u;
        }
        const uint32_t khsb_n = khsb_t.hi - khsb_t.lo;
        int khsb_ax = 0;
        if (mx[1] - mn[1] > mx[khsb_ax] - mn[khsb_ax]) khsb_ax = 1;
        if (mx[2] - mn[2] > mx[khsb_ax] - mn[khsb_ax]) khsb_ax = 2;
        if (khsb_n <= KH_SKIN_SUP_LEAF || !(mx[khsb_ax] - mn[khsb_ax] > 0.0f)) {   // Few points, or all at one place: a leaf.
            KhSkinSupNode& n = khsb_s.node[khsb_t.node];
            n.first = static_cast<uint32_t>(khsb_s.pt.size() / 3u);
            n.count = khsb_n;
            for (uint32_t i = khsb_t.lo; i < khsb_t.hi; ++i) {
                const float* p = &khsb_p[static_cast<size_t>(khsb_ix[i]) * 3u];
                khsb_s.pt.push_back(p[0]); khsb_s.pt.push_back(p[1]); khsb_s.pt.push_back(p[2]);
            }
            continue;
        }
        const uint32_t khsb_mid = khsb_t.lo + khsb_n / 2u;
        std::nth_element(khsb_ix.begin() + khsb_t.lo, khsb_ix.begin() + khsb_mid, khsb_ix.begin() + khsb_t.hi,
                         [&](uint32_t a, uint32_t b) { return khsb_p[static_cast<size_t>(a) * 3u + khsb_ax] < khsb_p[static_cast<size_t>(b) * 3u + khsb_ax]; });
        const uint32_t khsb_l = static_cast<uint32_t>(khsb_s.node.size());
        khsb_s.node.emplace_back();
        khsb_s.node.emplace_back();
        khsb_s.node[khsb_t.node].left = khsb_l;
        khsb_st.push_back(KhSbTask{ khsb_l,      khsb_t.lo,  khsb_mid });
        khsb_st.push_back(KhSbTask{ khsb_l + 1u, khsb_mid,   khsb_t.hi });
    }
    return khsb_root;
}
// A node's box support along u: no point under it has a larger p . u (each product and sum rounds monotonically;
// a fused multiply-add can exceed it by about an ulp - see kh_skin_so_bound).
// KH_SKIN_BOUND_PRUNE: kh_skin_sup_max's own test and kh_skin_so_bound's skip share it.
inline float kh_skin_sup_box(const KhSkinSupNode& n, const float khsx_u[3]) {
    return (khsx_u[0] > 0.0f ? khsx_u[0] * n.mx[0] : khsx_u[0] * n.mn[0]) +
           (khsx_u[1] > 0.0f ? khsx_u[1] * n.mx[1] : khsx_u[1] * n.mn[1]) +
           (khsx_u[2] > 0.0f ? khsx_u[2] * n.mx[2] : khsx_u[2] * n.mn[2]);
}
// The support of one bone's points along u: max over them of p . u. Exact.
inline float kh_skin_sup_max(const KhSkinSup& khsm_s, uint32_t khsm_root, const float khsm_u[3]) {
    uint32_t khsm_st[96];
    uint32_t khsm_n = 0u;
    float khsm_best = -3.0e38f;
    auto khsm_bound = [&](const KhSkinSupNode& n) { return kh_skin_sup_box(n, khsm_u); };
    khsm_st[khsm_n++] = khsm_root;
    while (khsm_n != 0u) {
        const KhSkinSupNode& n = khsm_s.node[khsm_st[--khsm_n]];
        // The box's own support, a hair up: its sum rounds apart from a point's.
        const float khsm_b = khsm_bound(n);
        if (!(khsm_b + fabsf(khsm_b) * 1.0e-6f > khsm_best)) continue;
        if (n.count != 0u) {
            const float* p = &khsm_s.pt[static_cast<size_t>(n.first) * 3u];
            for (uint32_t i = 0; i < n.count; ++i, p += 3) {
                const float d = p[0] * khsm_u[0] + p[1] * khsm_u[1] + p[2] * khsm_u[2];
                if (d > khsm_best) khsm_best = d;
            }
            continue;
        }
        if (khsm_n + 2u > 96u) {   // Never reached by a median tree (depth ~ log2 n); scan rather than drop.
            const float d = khsm_b;
            if (d > khsm_best) khsm_best = d;   // The box's support: conservative.
            continue;
        }
        const float khsm_bl = khsm_bound(khsm_s.node[n.left]), khsm_br = khsm_bound(khsm_s.node[n.left + 1u]);
        if (khsm_bl > khsm_br) { khsm_st[khsm_n++] = n.left + 1u; khsm_st[khsm_n++] = n.left; }   // The better child is popped first.
        else                   { khsm_st[khsm_n++] = n.left;      khsm_st[khsm_n++] = n.left + 1u; }
    }
    return khsm_best;
}
// KH_SKIN_GPU: the trees and boxes of a snapshot (verts / inf / drawn / ctr / ext set), for khsp_nb bones. Game
// thread, kh_skin_rest_of.
inline void kh_skin_sup_prepare(KhSkinRest& khsp_r, size_t khsp_nb) {
    KhSkinSup& khsp_s = khsp_r.sup;
    const size_t khsp_nv = khsp_r.verts.size();
    if (khsp_nb == 0u || khsp_r.inf.size() != khsp_nv) return;   // No influences: every vertex stands at rest; the CPU path keeps it.
    std::vector<float> khsp_p(khsp_nv * 3u);
    for (size_t i = 0; i < khsp_nv; ++i) {
        for (int k = 0; k < 3; ++k) khsp_p[i * 3u + k] = khsp_r.verts[i].pos[k] * khsp_r.ext[k] + khsp_r.ctr[k];   // kh_skin_verts' authored position.
    }
    const uint8_t* khsp_dr = khsp_r.drawn.size() == khsp_nv ? khsp_r.drawn.data() : nullptr;
    std::vector<std::vector<uint32_t>> khsp_set(khsp_nb);
    float id_mn[3] = { 3.0e38f, 3.0e38f, 3.0e38f }, id_mx[3] = { -3.0e38f, -3.0e38f, -3.0e38f };
    for (size_t i = 0; i < khsp_nv; ++i) {
        if (khsp_dr && khsp_dr[i] == 0u) continue;   // The box is the drawn vertices' (kh_skin_verts).
        const KhSkinInf& f = khsp_r.inf[i];
        float ws = 0.0f;
        for (int k = 0; k < 4; ++k) {
            if (!(f.w[k] > 0.0f) || f.b[k] >= khsp_nb) continue;   // kh_skin_verts' own skip.
            std::vector<uint32_t>& v = khsp_set[f.b[k]];
            if (v.empty() || v.back() != static_cast<uint32_t>(i)) v.push_back(static_cast<uint32_t>(i));   // A bone named twice by one vertex files it once.
            ws += f.w[k];
        }
        // The bound is a convex-combination argument and kh_skin_verts does not renormalise, so weights summing past
        // one (beyond the float slack the pad covers) would carry a vertex outside it: no trees, the mesh keeps the
        // CPU path with its exact box.
        if (ws > 1.0f + KH_SKIN_SO_ROOT_W) { khsp_s = KhSkinSup{}; khsp_r.bone_box.clear(); return; }
        if ((ws < 1.0f ? 1.0f - ws : 0.0f) > KH_SKIN_SO_ROOT_W) {
            for (int k = 0; k < 3; ++k) { const float a = khsp_p[i * 3u + k]; if (a < id_mn[k]) id_mn[k] = a; if (a > id_mx[k]) id_mx[k] = a; }
            khsp_s.id_any = true;
        }
    }
    memcpy(khsp_s.id_mn, id_mn, sizeof(id_mn));
    memcpy(khsp_s.id_mx, id_mx, sizeof(id_mx));
    khsp_s.root.assign(khsp_nb, 0xFFFFFFFFu);
    khsp_r.bone_box.assign(khsp_nb * 6u, 0.0f);
    for (size_t b = 0; b < khsp_nb; ++b) {
        float* bx = &khsp_r.bone_box[b * 6u];
        bx[0] = bx[1] = bx[2] = 1.0f; bx[3] = bx[4] = bx[5] = -1.0f;   // None.
        if (khsp_set[b].empty()) continue;
        khsp_s.root[b] = kh_skin_sup_build(khsp_s, khsp_p, khsp_set[b]);
        const KhSkinSupNode& n = khsp_s.node[khsp_s.root[b]];
        memcpy(bx, n.mn, sizeof(n.mn));
        memcpy(bx + 3, n.mx, sizeof(n.mx));
    }
    // KH_SKIN_SO_PAL: every bone a weight names - over every vertex, drawn or not, as the stream carries them all,
    // with kh_skin_verts' own skip (a weight not above zero, a bone past the skeleton). A bone no vertex carries (an
    // ancestor kh_skin_finish keeps for the chain) takes no palette row.
    {
        std::vector<uint8_t> khsp_on(khsp_nb, 0u);
        for (const KhSkinInf& f : khsp_r.inf) {
            for (int k = 0; k < 4; ++k) {
                if (f.w[k] > 0.0f && f.b[k] < khsp_nb) khsp_on[f.b[k]] = 1u;
            }
        }
        for (size_t b = 0; b < khsp_nb; ++b) {
            if (khsp_on[b] != 0u) khsp_s.pal.push_back(static_cast<uint32_t>(b));
        }
    }
    khsp_s.ok = true;
}
// KH_SKIN_GPU: the box of the pose khsb_a (kh_skin_affine_of's layout, the snapshot's bone count) - kh_skin_verts'
// bc / bs contract, from the trees. Contains the skinned drawn vertices - KH_SKIN_BOUND_PRUNE: those whose weight to
// the root is above KH_SKIN_SO_ROOT_W; one at or below it is stood in by its bones' images alone, so it can lie
// outside by up to that share of its bone's distance from rest (centimetres for a bone hundreds of metres out) - the
// box only, the drawn position is exact either way. Equals the exact box plus KH_SKIN_SO_PAD_M a side wherever the
// extreme vertex follows one bone.
// KH_SKIN_BOUND_PRUNE: a bone whose root box (kh_skin_sup_box, the tree's own first test) cannot move the running
// extreme along an axis skips that query - its exact value could not be taken (hi > mx / lo < mn are strict, and
// the box bounds every point: the tree prunes by the same test). The same box, bit for bit, as querying every bone
// wherever products and sums round separately (no contraction: MSVC x64 /fp:precise or /fp:strict); a build that
// fuses them (FMA under /fp:fast or /fp:contract) can differ by about an ulp of the terms, far inside
// KH_SKIN_SO_PAD_M, as the tree's own pruning already can. A NaN box never skips.
inline void kh_skin_so_bound(const KhSkinRest& khsb_r, const float* khsb_a, float khsb_bc[3], float khsb_bs[3]) {
    const KhSkinSup& s = khsb_r.sup;
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    if (s.id_any) { memcpy(mn, s.id_mn, sizeof(mn)); memcpy(mx, s.id_mx, sizeof(mx)); }
    for (size_t b = 0; b < s.root.size(); ++b) {
        if (s.root[b] == 0xFFFFFFFFu) continue;
        const float* m = khsb_a + b * 12u;
        const KhSkinSupNode& khsb_n = s.node[s.root[b]];
        for (int c = 0; c < 3; ++c) {   // v'[c] = v . (m[c], m[3 + c], m[6 + c]) + m[9 + c].
            const float u[3] = { m[c], m[3 + c], m[6 + c] };
            const float v[3] = { -u[0], -u[1], -u[2] };
            const float khsb_hb = kh_skin_sup_box(khsb_n, u), khsb_lb = kh_skin_sup_box(khsb_n, v);
            if (!(khsb_hb + fabsf(khsb_hb) * 1.0e-6f + m[9 + c] <= mx[c])) {
                const float hi = kh_skin_sup_max(s, s.root[b], u) + m[9 + c];
                if (hi > mx[c]) mx[c] = hi;
            }
            if (!(-(khsb_lb + fabsf(khsb_lb) * 1.0e-6f) + m[9 + c] >= mn[c])) {
                const float lo = -kh_skin_sup_max(s, s.root[b], v) + m[9 + c];
                if (lo < mn[c]) mn[c] = lo;
            }
        }
    }
    for (int k = 0; k < 3; ++k) {
        if (!(mn[k] <= mx[k])) { mn[k] = mx[k] = 0.0f; }   // Nothing drawn (kh_skin_verts).
        else { mn[k] -= KH_SKIN_SO_PAD_M; mx[k] += KH_SKIN_SO_PAD_M; }
        khsb_bc[k] = 0.5f * (mn[k] + mx[k]);
        const float sz = mx[k] - mn[k];
        khsb_bs[k] = sz > 1.0e-4f ? sz : 1.0e-4f;
    }
}
inline KhVbRef kh_skin_gpu_ref(const KhSkinGpu& khgr_g) {
    KhVbRef khgr_r;
    khgr_r.vb = khgr_g.vb;
    khgr_r.off = khgr_g.off;
    return khgr_r;
}
inline bool kh_skin_gpu_retry_open(uint64_t khgz_at) {   // ~0 = never refused; a counter that restarted opens it.
    return khgz_at == ~0ull || g_topo_cycles < khgz_at || g_topo_cycles - khgz_at >= KH_SKIN_GPU_RETRY;
}
// Whether kh_skin_so_ensure may be asked now: the objects never failed, or a refusal's retry is due.
inline bool kh_skin_so_askable() {
    return !g_skin_so_failed.load(std::memory_order_relaxed) ||
           (g_skin_so_retry_at != ~0ull && kh_skin_gpu_retry_open(g_skin_so_retry_at));
}
inline bool kh_skin_so_big_ready() {
    return g_res.skin_so_vs_l && g_res.skin_so_gs_l && g_res.skin_so_cb_l;
}
inline bool kh_skin_so_big_askable() {
    if (g_khsa_state != 2) return false;   // The prewarm is in flight (or not armed yet): not now.
    return !g_skin_so_big_failed || (g_skin_so_big_retry_at != ~0ull && kh_skin_gpu_retry_open(g_skin_so_big_retry_at));
}
// KH_SKIN_SO_LAZY: the snapshot's palette has a class whose objects stand - the small one always (its own objects
// are the callers' test), the large one once kh_skin_so_big_ensure made it. Every admission site asks this.
inline bool kh_skin_so_class_ok(const KhSkinRest& khsk_r) {
    const size_t khsk_n = khsk_r.sup.pal.size();
    return khsk_n <= KH_SKIN_SO_BONES_S || (khsk_n <= KH_SKIN_SO_BONES && kh_skin_so_big_ready());
}
inline uint32_t kh_skin_pool_maxcap(UINT khpm_bytes) {   // A chunk's most slices.
    return static_cast<uint32_t>(std::max<UINT>(1u, KH_SKIN_POOL_CHUNK_BYTES / khpm_bytes));
}
// A chunk's buffer and view for khpb_cap slices of khpb_bytes; false = neither made.
inline bool kh_skin_pool_buffer(ID3D11Device* khpb_dev, uint32_t khpb_cap, UINT khpb_bytes, ID3D11Buffer*& khpb_vb,
                                ID3D11ShaderResourceView*& khpb_srv) {
    khpb_vb = nullptr;
    khpb_srv = nullptr;
    D3D11_BUFFER_DESC khpb_bd = {};
    khpb_bd.ByteWidth = khpb_cap * khpb_bytes;   // At most max(64 MB, one slice).
    khpb_bd.Usage = D3D11_USAGE_DEFAULT;
    khpb_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(khpb_dev->CreateBuffer(&khpb_bd, nullptr, &khpb_vb)) || !khpb_vb) {
        khpb_vb = nullptr;
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC khpb_sd = {};
    khpb_sd.Format = DXGI_FORMAT_R32G32B32A32_UINT;
    khpb_sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    khpb_sd.Buffer.FirstElement = 0;
    khpb_sd.Buffer.NumElements = khpb_bd.ByteWidth / 16u;   // 48 = 3 x 16: a vertex is three elements.
    if (FAILED(khpb_dev->CreateShaderResourceView(khpb_vb, &khpb_sd, &khpb_srv)) || !khpb_srv) {
        khpb_srv = nullptr;
        KH_SAFE_RELEASE(khpb_vb);
        return false;
    }
    g_skin_pool_made = true;   // KH_SKIN_POOL_MADE: the device takes the kind.
    return true;
}
// A chunk's buffer changed (kh_skin_pool_grow): every binding in it, and every slot table element the walk
// installed for one, takes the new pointer. The offsets stand.
inline void kh_skin_pool_rebind(const KhSkinPoolChunk& khrb_c) {
    for (auto& khrb_kv : g_skin_gpu) {
        if (khrb_kv.second.chunk == &khrb_c) khrb_kv.second.vb = khrb_c.vb;
    }
    for (size_t khrb_s = 0; khrb_s < g_cloth_pool_slot.size() && khrb_s < g_cloth_vb_slot.size(); ++khrb_s) {
        if (g_cloth_pool_slot[khrb_s] == &khrb_c) g_cloth_vb_slot[khrb_s] = khrb_c.vb;
    }
}
// The chunk grown to khpg_to slices (more than it has) in place: a new buffer with the old one copied to its start,
// so every slice keeps its offset, bytes and base. False = refused (the chunk stands as it was).
inline bool kh_skin_pool_grow(ID3D11Device* khpg_dev, ID3D11DeviceContext* khpg_ctx, KhSkinPoolChunk& khpg_c,
                              UINT khpg_bytes, uint32_t khpg_to) {
    const uint32_t khpg_cap = static_cast<uint32_t>(khpg_c.used.size());
    if (khpg_to <= khpg_cap) return false;
    ID3D11Buffer* khpg_vb = nullptr;
    ID3D11ShaderResourceView* khpg_srv = nullptr;
    if (!kh_skin_pool_buffer(khpg_dev, khpg_to, khpg_bytes, khpg_vb, khpg_srv)) return false;
    khpg_ctx->CopySubresourceRegion(khpg_vb, 0, 0, 0, 0, khpg_c.vb, 0, nullptr);   // The whole old buffer, at 0.
    KH_SAFE_RELEASE(khpg_c.srv);
    KH_SAFE_RELEASE(khpg_c.vb);
    khpg_c.vb = khpg_vb;
    khpg_c.srv = khpg_srv;
    khpg_c.used.resize(khpg_to, 0u);
    kh_skin_pool_rebind(khpg_c);
    return true;
}
// A slice of the pool for (khpt_mesh, khpt_bytes) into khpt_g (vb / off / chunk / slice); false = none (the caller
// makes a buffer of the binding's own). khpt_g holds no buffer on entry. The first free slice in chunk order; else
// the last chunk doubles; else (it is at the most a chunk takes) a new chunk of one slice.
inline bool kh_skin_pool_take(ID3D11Device* khpt_dev, ID3D11DeviceContext* khpt_ctx, int khpt_mesh, UINT khpt_bytes,
                              KhSkinGpu& khpt_g) {
    if (g_skin_pool_failed || !khpt_dev || !khpt_ctx || khpt_mesh < 0 || khpt_bytes == 0u ||
        khpt_bytes % static_cast<UINT>(sizeof(MeshVertex)) != 0u) return false;
    const uint64_t khpt_key = (static_cast<uint64_t>(static_cast<uint32_t>(khpt_mesh)) << 32) | khpt_bytes;
    KhSkinPool& khpt_p = g_skin_pool[khpt_key];
    const uint32_t khpt_max = kh_skin_pool_maxcap(khpt_bytes);
    KhSkinPoolChunk* khpt_c = nullptr;
    uint32_t khpt_s = 0u;
    for (const std::unique_ptr<KhSkinPoolChunk>& khpt_up : khpt_p.chunks) {
        if (khpt_up->live >= khpt_up->used.size()) continue;
        for (uint32_t khpt_k = 0; khpt_k < static_cast<uint32_t>(khpt_up->used.size()); ++khpt_k) {
            if (!khpt_up->used[khpt_k]) { khpt_c = khpt_up.get(); khpt_s = khpt_k; break; }
        }
        if (khpt_c) break;
    }
    if (!khpt_c && !khpt_p.chunks.empty()) {
        KhSkinPoolChunk& khpt_l = *khpt_p.chunks.back();
        const uint32_t khpt_was = static_cast<uint32_t>(khpt_l.used.size());
        if (kh_skin_pool_grow(khpt_dev, khpt_ctx, khpt_l, khpt_bytes, std::min<uint32_t>(khpt_was * 2u, khpt_max))) {
            khpt_c = &khpt_l;
            khpt_s = khpt_was;
        } else if (khpt_was < khpt_max) {
            return false;   // A refused growth: this binding takes a buffer of its own; the pool stands.
        }
    }
    if (!khpt_c) {
        std::unique_ptr<KhSkinPoolChunk> khpt_n(new KhSkinPoolChunk());
        if (!kh_skin_pool_buffer(khpt_dev, 1u, khpt_bytes, khpt_n->vb, khpt_n->srv)) {
            // KH_SKIN_POOL_MADE: no pool buffer ever stood on this device - it refuses the kind itself: no more
            // pooling. After one stood the refusal is this take's alone (the binding takes a buffer of its own).
            if (!g_skin_pool_made) g_skin_pool_failed = true;
            if (khpt_p.chunks.empty()) g_skin_pool.erase(khpt_key);
            return false;
        }
        if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
        khpt_n->serial = g_skin_pool_serial;
        khpt_n->key = khpt_key;
        khpt_n->used.assign(1u, 0u);
        khpt_c = khpt_n.get();
        khpt_s = 0u;
        khpt_p.chunks.push_back(std::move(khpt_n));
    }
    khpt_c->used[khpt_s] = 1u;
    ++khpt_c->live;
    khpt_g.vb = khpt_c->vb;
    khpt_g.chunk = khpt_c;
    khpt_g.slice = khpt_s;
    khpt_g.off = khpt_s * khpt_bytes;
    return true;
}
// KH_SKIN_POOL: a pool about to take khpr_need more slices in this flush takes the room at once - its last chunk
// grown to fit them, at least doubled (to the cap), then whole chunks and one sized to the rest - so a mass spawn
// makes each buffer once and copies the old last chunk at most once, not a doubling ladder, and a trickle still
// grows by doubling. The takes then fill first-free. A refusal leaves the takes to grow as they go; a chunk nothing
// took goes at the next trim. kh_skin_upload's, before its walk, under g_skin_mu.
inline void kh_skin_pool_reserve(ID3D11Device* khpr_dev, ID3D11DeviceContext* khpr_ctx, uint64_t khpr_key,
                                 uint32_t khpr_need) {
    const UINT khpr_bytes = static_cast<UINT>(khpr_key & 0xFFFFFFFFull);
    if (g_skin_pool_failed || !khpr_dev || !khpr_ctx || khpr_need == 0u || khpr_bytes == 0u ||
        khpr_bytes % static_cast<UINT>(sizeof(MeshVertex)) != 0u) return;
    const uint32_t khpr_max = kh_skin_pool_maxcap(khpr_bytes);
    const auto khpr_it = g_skin_pool.find(khpr_key);
    uint64_t khpr_free = 0u;
    if (khpr_it != g_skin_pool.end()) {
        for (const std::unique_ptr<KhSkinPoolChunk>& khpr_c : khpr_it->second.chunks) {
            khpr_free += khpr_c->used.size() - khpr_c->live;
        }
    }
    if (khpr_free >= khpr_need) return;
    uint64_t khpr_want = khpr_need - khpr_free;
    if (khpr_it != g_skin_pool.end() && !khpr_it->second.chunks.empty()) {   // The last chunk first, to the cap.
        KhSkinPoolChunk& khpr_l = *khpr_it->second.chunks.back();
        const uint64_t khpr_was = khpr_l.used.size();
        if (khpr_was < khpr_max) {   // At least doubled (a trickle of spawns does not grow a slice a flush).
            const uint64_t khpr_to = std::min<uint64_t>(std::max<uint64_t>(khpr_was + khpr_want, khpr_was * 2u),
                                                        khpr_max);
            if (!kh_skin_pool_grow(khpr_dev, khpr_ctx, khpr_l, khpr_bytes, static_cast<uint32_t>(khpr_to))) return;
            khpr_want -= std::min<uint64_t>(khpr_want, khpr_to - khpr_was);
        }
    }
    while (khpr_want > 0u) {   // Then whole chunks, the last one sized to what is left.
        std::unique_ptr<KhSkinPoolChunk> khpr_n(new KhSkinPoolChunk());
        const uint32_t khpr_cap = static_cast<uint32_t>(std::min<uint64_t>(khpr_want, khpr_max));
        if (!kh_skin_pool_buffer(khpr_dev, khpr_cap, khpr_bytes, khpr_n->vb, khpr_n->srv)) return;   // The takes try.
        if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
        khpr_n->serial = g_skin_pool_serial;
        khpr_n->key = khpr_key;
        khpr_n->used.assign(khpr_cap, 0u);
        g_skin_pool[khpr_key].chunks.push_back(std::move(khpr_n));
        khpr_want -= khpr_cap;
    }
}
// The chunks with no live slice (a reservation nothing took) released; then every pool whose live slices fell to a
// quarter of its slices, repacked into half that: fresh chunks (as many full ones as needed, the last one twice its
// share), each binding's slice copied to its new place on the GPU and its fields re-pointed. Offsets change, so the
// bases go to the records with this flush's puts (kh_skin_upload), and until a record carries its new base the
// instanced routes' tests send that binding per object (kh_inst_pool_of, kh_cast_pool_of). Every new buffer is made
// before anything moves: a refused one leaves the pool as it was. Called by kh_skin_upload after its sweep, under
// g_skin_mu, with the slot tables empty (kh_cloth_upload) - no element names a released chunk.
inline void kh_skin_pool_trim(ID3D11Device* khtr_dev, ID3D11DeviceContext* khtr_ctx) {
    if (!khtr_dev || !khtr_ctx) return;
    for (auto khtr_pi = g_skin_pool.begin(); khtr_pi != g_skin_pool.end();) {   // Chunks with no live slice go.
        std::vector<std::unique_ptr<KhSkinPoolChunk>>& khtr_v = khtr_pi->second.chunks;
        for (size_t khtr_i = 0; khtr_i < khtr_v.size();) {
            if (khtr_v[khtr_i]->live != 0u) { ++khtr_i; continue; }
            KH_SAFE_RELEASE(khtr_v[khtr_i]->srv);
            KH_SAFE_RELEASE(khtr_v[khtr_i]->vb);
            khtr_v.erase(khtr_v.begin() + static_cast<std::ptrdiff_t>(khtr_i));
        }
        if (khtr_v.empty()) khtr_pi = g_skin_pool.erase(khtr_pi);
        else ++khtr_pi;
    }
    for (auto& khtr_kv : g_skin_pool) {
        KhSkinPool& khtr_p = khtr_kv.second;
        uint64_t khtr_live = 0u, khtr_cap = 0u;
        for (const std::unique_ptr<KhSkinPoolChunk>& khtr_c : khtr_p.chunks) {
            khtr_live += khtr_c->live;
            khtr_cap += khtr_c->used.size();
        }
        if (khtr_live == 0u || khtr_cap <= 1u || khtr_live * 4u > khtr_cap) continue;
        if (khtr_cap == khtr_p.trim_refused && g_topo_cycles - khtr_p.trim_refused_at < KH_SKIN_POOL_TRIM_RETRY) {
            continue;   // Refused at this size lately: wait for the pool to change, or a while.
        }
        const UINT khtr_bytes = static_cast<UINT>(khtr_kv.first & 0xFFFFFFFFull);
        const uint32_t khtr_max = kh_skin_pool_maxcap(khtr_bytes);
        std::vector<std::unique_ptr<KhSkinPoolChunk>> khtr_new;
        bool khtr_ok = true;
        for (uint64_t khtr_left = khtr_live; khtr_left > 0u && khtr_ok;) {
            const uint32_t khtr_n = khtr_left >= khtr_max ? khtr_max
                                  : static_cast<uint32_t>(std::min<uint64_t>(khtr_left * 2u, khtr_max));
            std::unique_ptr<KhSkinPoolChunk> khtr_c(new KhSkinPoolChunk());
            khtr_ok = kh_skin_pool_buffer(khtr_dev, khtr_n, khtr_bytes, khtr_c->vb, khtr_c->srv);
            if (!khtr_ok) break;
            khtr_c->key = khtr_kv.first;
            khtr_c->used.assign(khtr_n, 0u);
            khtr_left -= std::min<uint64_t>(khtr_left, khtr_n);
            khtr_new.push_back(std::move(khtr_c));
        }
        if (!khtr_ok) {
            for (std::unique_ptr<KhSkinPoolChunk>& khtr_c : khtr_new) {
                KH_SAFE_RELEASE(khtr_c->srv);
                KH_SAFE_RELEASE(khtr_c->vb);
            }
            khtr_p.trim_refused = khtr_cap;
            khtr_p.trim_refused_at = g_topo_cycles;
            continue;
        }
        size_t khtr_ci = 0;
        uint32_t khtr_si = 0u;
        for (auto& khtr_g : g_skin_gpu) {
            KhSkinGpu& khtr_m = khtr_g.second;
            if (!khtr_m.chunk || khtr_m.chunk->key != khtr_kv.first) continue;
            if (khtr_si >= khtr_new[khtr_ci]->used.size()) { ++khtr_ci; khtr_si = 0u; }
            KhSkinPoolChunk& khtr_d = *khtr_new[khtr_ci];
            D3D11_BOX khtr_b = {};
            khtr_b.left = khtr_m.off;
            khtr_b.right = khtr_m.off + khtr_bytes;
            khtr_b.top = 0;
            khtr_b.bottom = 1;
            khtr_b.front = 0;
            khtr_b.back = 1;
            khtr_ctx->CopySubresourceRegion(khtr_d.vb, 0, khtr_si * khtr_bytes, 0, 0, khtr_m.vb, 0, &khtr_b);
            khtr_d.used[khtr_si] = 1u;
            ++khtr_d.live;
            khtr_m.vb = khtr_d.vb;
            khtr_m.chunk = &khtr_d;
            khtr_m.slice = khtr_si;
            khtr_m.off = khtr_si * khtr_bytes;
            ++khtr_si;
        }
        for (std::unique_ptr<KhSkinPoolChunk>& khtr_c : khtr_p.chunks) {
            KH_SAFE_RELEASE(khtr_c->srv);
            KH_SAFE_RELEASE(khtr_c->vb);
        }
        for (std::unique_ptr<KhSkinPoolChunk>& khtr_c : khtr_new) {
            if (++g_skin_pool_serial == 0u) ++g_skin_pool_serial;
            khtr_c->serial = g_skin_pool_serial;
        }
        khtr_p.chunks.swap(khtr_new);   // The old chunk objects go with khtr_new.
    }
}
// The binding's buffer goes: its own released, or its slice handed back (and the chunk with its last slice).
inline void kh_skin_gpu_vb_drop(KhSkinGpu& khgd_g) {
    KhSkinPoolChunk* const khgd_c = khgd_g.chunk;
    if (khgd_c) {
        if (khgd_g.slice < khgd_c->used.size() && khgd_c->used[khgd_g.slice]) {
            khgd_c->used[khgd_g.slice] = 0u;
            --khgd_c->live;
        }
        if (khgd_c->live == 0u) {
            const auto khgd_p = g_skin_pool.find(khgd_c->key);
            if (khgd_p != g_skin_pool.end()) {
                std::vector<std::unique_ptr<KhSkinPoolChunk>>& khgd_v = khgd_p->second.chunks;
                for (size_t khgd_i = 0; khgd_i < khgd_v.size(); ++khgd_i) {
                    if (khgd_v[khgd_i].get() != khgd_c) continue;
                    KH_SAFE_RELEASE(khgd_v[khgd_i]->srv);
                    KH_SAFE_RELEASE(khgd_v[khgd_i]->vb);
                    khgd_v.erase(khgd_v.begin() + static_cast<std::ptrdiff_t>(khgd_i));
                    break;
                }
                if (khgd_v.empty()) g_skin_pool.erase(khgd_p);
            }
        }
        khgd_g.vb = nullptr;   // The chunk's: not this binding's to release.
    } else {
        KH_SAFE_RELEASE(khgd_g.vb);
    }
    khgd_g.chunk = nullptr;
    khgd_g.slice = 0u;
    khgd_g.off = 0u;
}
// A CPU write of a stream-output buffer's whole vertex array (khsw_bytes): the buffer of the binding's own, or
// (khsw_pooled) its slice of a chunk, by a box.
inline void kh_skin_so_write(ID3D11DeviceContext* khsw_ctx, const KhVbRef& khsw_r, UINT khsw_bytes, bool khsw_pooled,
                             const void* khsw_src) {
    if (!khsw_pooled) {
        khsw_ctx->UpdateSubresource(khsw_r.vb, 0, nullptr, khsw_src, 0, 0);
        return;
    }
    D3D11_BOX khsw_b = {};
    khsw_b.left = khsw_r.off;
    khsw_b.right = khsw_r.off + khsw_bytes;
    khsw_b.top = 0;
    khsw_b.bottom = 1;
    khsw_b.front = 0;
    khsw_b.back = 1;
    khsw_ctx->UpdateSubresource(khsw_r.vb, 0, &khsw_b, khsw_src, 0, 0);
}
// KH_SKIN_GPU_MOVE: the binding's DYNAMIC buffer handed over to stream output (kh_skin_gpu_movable said it may) - a
// slice of its mesh's pool, else a DEFAULT buffer of its own with the stream-output bind, as the walk makes a first
// one. Its bytes are copied on the GPU and the old buffer released: every other field (gen, key, box, collider,
// valid, the step's claim) stands with the bytes, so the binding draws the pose it held and its next step skins it
// on the GPU. The engine's predicate is set aside for the whole move and put back - a predicated copy would leave
// the new buffer unwritten, or a chunk the take grows without its older slices. A refusal keeps the DYNAMIC buffer
// and stamps so_refused. True = moved. Under g_skin_mu, in kh_skin_upload's walk.
inline bool kh_skin_gpu_move(ID3D11Device* khgw_dev, ID3D11DeviceContext* khgw_ctx, int khgw_mesh, KhSkinGpu& khgw_g) {
    ID3D11Predicate* khgw_pr = nullptr;
    BOOL khgw_pv = FALSE;
    khgw_ctx->GetPredication(&khgw_pr, &khgw_pv);
    if (khgw_pr) khgw_ctx->SetPredication(nullptr, FALSE);
    KhSkinGpu khgw_n;   // The new buffer: vb / chunk / slice / off.
    bool khgw_ok = kh_skin_pool_take(khgw_dev, khgw_ctx, khgw_mesh, khgw_g.bytes, khgw_n);
    if (!khgw_ok) {
        D3D11_BUFFER_DESC khgw_bd = {};
        khgw_bd.ByteWidth = khgw_g.bytes;
        khgw_bd.Usage = D3D11_USAGE_DEFAULT;
        khgw_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
        khgw_ok = SUCCEEDED(khgw_dev->CreateBuffer(&khgw_bd, nullptr, &khgw_n.vb)) && khgw_n.vb;
        if (!khgw_ok) khgw_n.vb = nullptr;
    }
    if (khgw_ok) {
        D3D11_BOX khgw_b = {};
        khgw_b.left = 0;
        khgw_b.right = khgw_g.bytes;
        khgw_b.top = 0;
        khgw_b.bottom = 1;
        khgw_b.front = 0;
        khgw_b.back = 1;
        khgw_ctx->CopySubresourceRegion(khgw_n.vb, 0, khgw_n.off, 0, 0, khgw_g.vb, 0, &khgw_b);
        KH_SAFE_RELEASE(khgw_g.vb);   // The DYNAMIC buffer: the binding's own (never pooled).
        khgw_g.vb = khgw_n.vb;
        khgw_g.chunk = khgw_n.chunk;
        khgw_g.slice = khgw_n.slice;
        khgw_g.off = khgw_n.off;
        khgw_g.so = true;
        khgw_g.so_refused = ~0ull;
    } else {
        khgw_g.so_refused = g_topo_cycles;
    }
    if (khgw_pr) {
        khgw_ctx->SetPredication(khgw_pr, khgw_pv);
        khgw_pr->Release();
    }
    return khgw_ok;
}
// KH_SKIN_SO_PAL: a weight's bone is stored as its palette row (KhSkinSup::pal - every bone the skip below lets
// through has one), so the shader reads row j where kh_skin_verts reads bone pal[j]: the same map, the same sums.
inline void kh_skin_so_stream(const KhSkinRest& khss_r, size_t khss_nb, std::vector<KhSkinSoVert>& khss_out) {
    const size_t khss_nv = khss_r.verts.size();
    std::vector<uint32_t> khss_row(khss_nb, 0u);   // KH_SKIN_SO_PAL: bone -> palette row.
    for (size_t j = 0; j < khss_r.sup.pal.size(); ++j) {
        if (khss_r.sup.pal[j] < khss_nb) khss_row[khss_r.sup.pal[j]] = static_cast<uint32_t>(j);
    }
    khss_out.resize(khss_nv);
    for (size_t i = 0; i < khss_nv; ++i) {
        const MeshVertex& r = khss_r.verts[i];
        KhSkinSoVert& o = khss_out[i];
        for (int k = 0; k < 3; ++k) {   // kh_skin_verts' first three lines, to the float.
            o.p[k] = r.pos[k] * khss_r.ext[k] + khss_r.ctr[k];
            o.n[k] = r.nrm[k] / khss_r.ext[k];
            o.t[k] = r.tan[k] * khss_r.ext[k];
        }
        o.t[3] = r.tan[3];
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
        const KhSkinInf& f = khss_r.inf[i];   // The caller checked inf.size() == nv.
        for (int k = 0; k < 4; ++k) {
            const bool on = f.w[k] > 0.0f && f.b[k] < khss_nb;   // kh_skin_verts' skip, made a zero weight.
            o.b[k] = on ? khss_row[f.b[k]] : 0u;   // KH_SKIN_SO_PAL.
            o.w[k] = on ? f.w[k] : 0.0f;
        }
    }
}
// CBSkin's image (skin.hlsl): the box's centre and size, then three float4 a bone - row c = (M[0 + c], M[3 + c],
// M[6 + c], M[9 + c]) of kh_skin_affine_of's map, so the shader's dot-and-add is kh_skin_verts' sum. khsc_dst holds
// (2 + khsc_np * 3) * 4 floats. KH_SKIN_SO_PAL: row j is bone khsc_pal[j]'s map (khsc_np rows; every entry is below
// the snapshot's bone count, so inside khsc_a).
inline void kh_skin_so_cb_fill(float* khsc_dst, const float khsc_bc[3], const float khsc_bs[3], const float* khsc_a,
                               const uint32_t* khsc_pal, size_t khsc_np) {
    khsc_dst[0] = khsc_bc[0]; khsc_dst[1] = khsc_bc[1]; khsc_dst[2] = khsc_bc[2]; khsc_dst[3] = 0.0f;
    khsc_dst[4] = khsc_bs[0]; khsc_dst[5] = khsc_bs[1]; khsc_dst[6] = khsc_bs[2]; khsc_dst[7] = 0.0f;
    float* khsc_o = khsc_dst + 8;
    for (size_t j = 0; j < khsc_np; ++j) {
        const float* m = khsc_a + static_cast<size_t>(khsc_pal[j]) * 12u;
        for (int c = 0; c < 3; ++c, khsc_o += 4) {
            khsc_o[0] = m[c]; khsc_o[1] = m[3 + c]; khsc_o[2] = m[6 + c]; khsc_o[3] = m[9 + c];
        }
    }
}
// A snapshot's rest stream, made once (g_skin_mu held; marks it seen for the upload's sweep). Null = this snapshot
// cannot be skinned on the GPU, or its create failed - an entry stands for a failure, so it is not retried on every
// pass: KH_SKIN_GPU_MOVE asks again KH_SKIN_GPU_RETRY cycles on (fail_at); the entry leaves with the snapshot's last
// binding.
inline ID3D11Buffer* kh_skin_rest_gpu_ensure(ID3D11Device* khre_dev, const std::shared_ptr<const KhSkinRest>& khre_rest, size_t khre_nb) {
    if (!khre_dev || !khre_rest || !khre_rest->sup.ok || khre_nb == 0u ||
        !kh_skin_so_class_ok(*khre_rest) ||   // KH_SKIN_SO_BIG / KH_SKIN_SO_LAZY: the palette's class stands.
        khre_rest->sup.root.size() != khre_nb || khre_rest->verts.empty() || khre_rest->inf.size() != khre_rest->verts.size()) return nullptr;
    const auto khre_it = g_skin_rest_gpu.find(khre_rest.get());
    if (khre_it != g_skin_rest_gpu.end()) {
        khre_it->second.seen = true;
        if (khre_it->second.vb || !kh_skin_gpu_retry_open(khre_it->second.fail_at)) return khre_it->second.vb;
    }
    KhSkinRestGpu& khre_rg = g_skin_rest_gpu[khre_rest.get()];
    khre_rg.seen = true;
    khre_rg.rest = khre_rest;
    try {
        std::vector<KhSkinSoVert> khre_sv;
        kh_skin_so_stream(*khre_rest, khre_nb, khre_sv);
        D3D11_BUFFER_DESC khre_rd = {};
        khre_rd.ByteWidth = static_cast<UINT>(khre_sv.size() * sizeof(KhSkinSoVert));
        khre_rd.Usage = D3D11_USAGE_IMMUTABLE;
        khre_rd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA khre_ri = {};
        khre_ri.pSysMem = khre_sv.data();
        if (FAILED(khre_dev->CreateBuffer(&khre_rd, &khre_ri, &khre_rg.vb))) khre_rg.vb = nullptr;
    } catch (...) { khre_rg.vb = nullptr; }
    khre_rg.nv = khre_rg.vb ? static_cast<UINT>(khre_rest->verts.size()) : 0u;
    khre_rg.fail_at = khre_rg.vb ? ~0ull : g_topo_cycles;   // KH_SKIN_GPU_MOVE.
    if (!khre_rg.vb) kh_stat(g_stats.skin_gpu_refused);   // KH_SKIN_GPU_MOVE: skinGpuRefused.
    return khre_rg.vb;
}
// KH_SKIN_GPU_MOVE: a binding's CPU-skinned (DYNAMIC) buffer that stream output can take over - its mesh's snapshot
// skins on the GPU (the bound's trees, a bone at least and a palette of at most KH_SKIN_SO_BONES (KH_SKIN_SO_BIG), an
// influence entry for every vertex: the walk's first-install test, item by item), the buffer is the rest stream's
// size, and no refusal of the device is younger than KH_SKIN_GPU_RETRY cycles. The rest stream itself is the
// caller's to have. Under g_skin_mu.
inline bool kh_skin_gpu_movable(const KhSkinGpu& khgq_g, const KhSkinRest& khgq_r, size_t khgq_nb) {
    return khgq_g.vb && !khgq_g.so && !khgq_g.chunk && khgq_r.sup.ok && khgq_nb != 0u &&
           kh_skin_so_class_ok(khgq_r) &&   // KH_SKIN_SO_BIG / _LAZY: a large palette moves once its class stands.
           khgq_r.sup.root.size() == khgq_nb && khgq_r.inf.size() == khgq_r.verts.size() && !khgq_r.verts.empty() &&
           static_cast<uint64_t>(khgq_g.bytes) == static_cast<uint64_t>(khgq_r.verts.size()) * sizeof(MeshVertex) &&
           kh_skin_gpu_retry_open(khgq_g.so_refused);
}
// KH_EA_SKIN_SO_DECL: the stream-output declaration of both classes (kh_skin_so_ensure, kh_skin_so_big_ensure):
// MeshVertex's four lanes in its order, 48 bytes (skin.hlsl's VSOutSkin); nothing is rasterised.
static const D3D11_SO_DECLARATION_ENTRY KH_SKIN_SO_DECL[] = {
    { 0, "POSITION", 0, 0, 3, 0 },
    { 0, "NORMAL",   0, 0, 3, 0 },
    { 0, "TEXCOORD", 0, 0, 2, 0 },
    { 0, "TANGENT",  0, 0, 4, 0 },
};
// The device objects, once. False = not available: no device, or not now (reported once; KH_SKIN_SO_RETRY: a
// memory refusal is asked again KH_SKIN_GPU_RETRY cycles on, any other failure stands until the device or the
// session resets). From kh_skin_upload, holding no lock: VSSkinSo is taken here from the arming prewarm
// (KH_PREWARM_ALL) - compile_shader waits for its worker, or compiles it, only if the batch has not reached it.
inline bool kh_skin_so_ensure(ID3D11Device* khse_dev) {
    if (!KH_SKIN_GPU_ON || !khse_dev || !kh_skin_so_askable()) return false;
    if (g_res.skin_so_vs && g_res.skin_so_gs && g_res.skin_so_layout && g_res.skin_so_cb) return true;
    // KH_SKIN_SO_RETRY: khse_retry = the device refused memory (asked again, and counted as skinGpuRefused).
    auto khse_fail = [](const std::string& khse_why, bool khse_retry) {
        KH_SAFE_RELEASE(g_res.skin_so_cb);
        KH_SAFE_RELEASE(g_res.skin_so_layout);
        KH_SAFE_RELEASE(g_res.skin_so_gs);
        KH_SAFE_RELEASE(g_res.skin_so_vs);
        g_skin_so_failed.store(true, std::memory_order_relaxed);
        g_skin_so_retry_at = khse_retry ? g_topo_cycles : ~0ull;
        if (khse_retry) kh_stat(g_stats.skin_gpu_refused);   // skinGpuRefused.
        kh_ensure_ok("GPU skinning (the CPU path stands)", khse_why);
        return false;
    };
    static const D3D_SHADER_MACRO khse_defs[] = { { nullptr, nullptr } };
    ID3DBlob* khse_vb = nullptr;
    const std::string khse_err = compile_shader(kh_hlsl_src(KH_HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", khse_defs, &khse_vb);
    if (!khse_err.empty() || !khse_vb) {
        if (khse_vb) khse_vb->Release();
        return khse_fail(khse_err.empty() ? std::string("VSSkinSo: no bytecode") : khse_err, false);
    }
    static const D3D11_INPUT_ELEMENT_DESC khse_il[] = {
        { "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    const UINT khse_stride = static_cast<UINT>(sizeof(MeshVertex));
    HRESULT khse_hr = khse_dev->CreateVertexShader(khse_vb->GetBufferPointer(), khse_vb->GetBufferSize(), nullptr, &g_res.skin_so_vs);
    if (SUCCEEDED(khse_hr)) {
        khse_hr = khse_dev->CreateGeometryShaderWithStreamOutput(khse_vb->GetBufferPointer(), khse_vb->GetBufferSize(),
                                                                 KH_SKIN_SO_DECL, 4u, &khse_stride, 1u,
                                                                 D3D11_SO_NO_RASTERIZED_STREAM, nullptr,
                                                                 &g_res.skin_so_gs);
    }
    if (SUCCEEDED(khse_hr)) {
        khse_hr = khse_dev->CreateInputLayout(khse_il, 6u, khse_vb->GetBufferPointer(), khse_vb->GetBufferSize(), &g_res.skin_so_layout);
    }
    khse_vb->Release();
    if (SUCCEEDED(khse_hr)) {
        D3D11_BUFFER_DESC khse_bd = {};
        khse_bd.ByteWidth = (2u + KH_SKIN_SO_BONES_S * 3u) * 16u;   // CBSkin, the small class (KH_SKIN_SO_LAZY).
        khse_bd.Usage = D3D11_USAGE_DYNAMIC;
        khse_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        khse_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        khse_hr = khse_dev->CreateBuffer(&khse_bd, nullptr, &g_res.skin_so_cb);
    }
    if (FAILED(khse_hr)) {
        return khse_fail(std::string("stream-output objects: ") + hr_str(khse_hr), khse_hr == E_OUTOFMEMORY);
    }
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // KH_SKIN_SO_RETRY: made (on a retry too).
    g_skin_so_retry_at = ~0ull;
    return true;
}
inline bool kh_skin_so_big_ensure(ID3D11Device* khsg_dev) {
    if (!KH_SKIN_GPU_ON || !khsg_dev) return false;
    if (kh_skin_so_big_ready()) return true;
    if (!g_res.skin_so_layout || !kh_skin_so_big_askable()) return false;   // The small class's layout serves both.
    auto khsg_fail = [](const std::string& khsg_why, bool khsg_retry) {
        KH_SAFE_RELEASE(g_res.skin_so_cb_l);
        KH_SAFE_RELEASE(g_res.skin_so_gs_l);
        KH_SAFE_RELEASE(g_res.skin_so_vs_l);
        g_skin_so_big_failed = true;
        g_skin_so_big_retry_at = khsg_retry ? g_topo_cycles : ~0ull;
        if (khsg_retry) kh_stat(g_stats.skin_gpu_refused);   // skinGpuRefused.
        kh_ensure_ok("GPU skinning of skeletons over 256 weighted bones (they stay on the CPU path)", khsg_why);
        return false;
    };
    ID3DBlob* khsg_vb = nullptr;
    const std::string khsg_err = compile_shader(kh_hlsl_src(KH_HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0",
                                                KH_SKIN_SO_DEFS_L, &khsg_vb);
    if (!khsg_err.empty() || !khsg_vb) {
        if (khsg_vb) khsg_vb->Release();
        return khsg_fail(khsg_err.empty() ? std::string("VSSkinSo (large): no bytecode") : khsg_err, false);
    }
    const UINT khsg_stride = static_cast<UINT>(sizeof(MeshVertex));
    HRESULT khsg_hr = khsg_dev->CreateVertexShader(khsg_vb->GetBufferPointer(), khsg_vb->GetBufferSize(), nullptr,
                                                   &g_res.skin_so_vs_l);
    if (SUCCEEDED(khsg_hr)) {
        khsg_hr = khsg_dev->CreateGeometryShaderWithStreamOutput(
            khsg_vb->GetBufferPointer(), khsg_vb->GetBufferSize(), KH_SKIN_SO_DECL, 4u, &khsg_stride, 1u,
            D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &g_res.skin_so_gs_l);
    }
    khsg_vb->Release();
    if (SUCCEEDED(khsg_hr)) {
        D3D11_BUFFER_DESC khsg_bd = {};
        khsg_bd.ByteWidth = (2u + KH_SKIN_SO_BONES * 3u) * 16u;   // CBSkin, the large class.
        khsg_bd.Usage = D3D11_USAGE_DYNAMIC;
        khsg_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        khsg_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        khsg_hr = khsg_dev->CreateBuffer(&khsg_bd, nullptr, &g_res.skin_so_cb_l);
    }
    if (FAILED(khsg_hr)) {
        return khsg_fail(std::string("stream-output objects (large): ") + hr_str(khsg_hr), khsg_hr == E_OUTOFMEMORY);
    }
    g_skin_so_big_failed = false;
    g_skin_so_big_retry_at = ~0ull;
    return true;
}

// Which proxy moves each bone, for this mesh and this set of memory points. Game thread.
inline void kh_skin_drive(KhSkinInst& khsd_in, const MeshDef& khsd_d, const std::vector<std::string>& khsd_mem) {
    const size_t khsd_nb = khsd_d.skin_bones.size();
    khsd_in.drive.assign(khsd_nb, -1);
    khsd_in.drive_px.assign(khsd_nb, -1);
    khsd_in.bones_driven = 0;
    std::unordered_map<std::string, int32_t> khsd_px;
    for (size_t khsd_j = 0; khsd_j < khsd_mem.size(); ++khsd_j) khsd_px.emplace(khsd_mem[khsd_j], static_cast<int32_t>(khsd_j));
    std::vector<int32_t> khsd_own(khsd_nb, -1);
    for (size_t khsd_b = 0; khsd_b < khsd_nb; ++khsd_b) {
        const auto khsd_it = khsd_px.find(khsd_d.skin_bones[khsd_b].name);
        if (khsd_it == khsd_px.end()) continue;
        khsd_own[khsd_b] = khsd_it->second;
        ++khsd_in.bones_driven;
    }
    for (size_t khsd_b = 0; khsd_b < khsd_nb; ++khsd_b) {
        // Up the chain to the nearest matched bone; bounded by the bone count so a malformed parent table cannot loop.
        int32_t khsd_c = static_cast<int32_t>(khsd_b);
        for (size_t khsd_s = 0; khsd_s <= khsd_nb && khsd_c >= 0 && static_cast<size_t>(khsd_c) < khsd_nb; ++khsd_s) {
            if (khsd_own[khsd_c] >= 0) {
                khsd_in.drive[khsd_b] = khsd_c;
                khsd_in.drive_px[khsd_b] = khsd_own[khsd_c];
                break;
            }
            khsd_c = khsd_d.skin_bones[khsd_c].parent;
        }
    }
}

// A proxy's transform against a parent's: the relative pose, false when the proxy is out of reach of it. Positions
// SQF order, bases rows aside / up / dir in engine axes, as the raw read hands them. khsx_xd: the proxy's
// KH_PXY_SMOOTH delta (SQF order), or null. The offset is taken in double and rounded once: with no delta that is
// the float difference exactly (IEEE subtraction is correctly rounded), and a delta finer than the world
// position's float step survives into the small offset.
inline bool kh_skin_rel(const float khsx_xp[3], const float khsx_xr[9], const float khsx_pp[3], const float khsx_rp[9],
                        KhSkinXf& khsx_x, const float* khsx_xd) {
    // Engine axes: SQF [x, y, zASL] -> [x, zASL, y].
    double khsx_w[3] = { static_cast<double>(khsx_xp[0]) - static_cast<double>(khsx_pp[0]),
                         static_cast<double>(khsx_xp[2]) - static_cast<double>(khsx_pp[2]),
                         static_cast<double>(khsx_xp[1]) - static_cast<double>(khsx_pp[1]) };
    if (khsx_xd) {
        khsx_w[0] += static_cast<double>(khsx_xd[0]);
        khsx_w[1] += static_cast<double>(khsx_xd[2]);
        khsx_w[2] += static_cast<double>(khsx_xd[1]);
    }
    const float khsx_d[3] = { static_cast<float>(khsx_w[0]), static_cast<float>(khsx_w[1]),
                              static_cast<float>(khsx_w[2]) };
    if (khsx_d[0] * khsx_d[0] + khsx_d[1] * khsx_d[1] + khsx_d[2] * khsx_d[2] > KH_SKEL_REACH_M * KH_SKEL_REACH_M) return false;
    for (int r = 0; r < 3; ++r) {
        // Row vectors: local = world * R^T, so each lane is a dot with one of the parent's rows.
        khsx_x.t[r] = khsx_d[0] * khsx_rp[r * 3 + 0] + khsx_d[1] * khsx_rp[r * 3 + 1] + khsx_d[2] * khsx_rp[r * 3 + 2];
        for (int c = 0; c < 3; ++c) {
            khsx_x.q[r * 3 + c] = khsx_xr[r * 3 + 0] * khsx_rp[c * 3 + 0] + khsx_xr[r * 3 + 1] * khsx_rp[c * 3 + 1] +
                                  khsx_xr[r * 3 + 2] * khsx_rp[c * 3 + 2];
        }
    }
    return true;
}
// The affine maps from an explicit pose set (the instance's drive tables).
inline void kh_skin_affine_of(const KhSkinInst& khsa_in, const MeshDef& khsa_d, const std::vector<KhSkinXf>& khsa_pose,
                              const std::vector<uint8_t>& khsa_ok, std::vector<float>& khsa_a) {
    const size_t khsa_nb = khsa_d.skin_bones.size();
    khsa_a.assign(khsa_nb * 12u, 0.0f);
    for (size_t khsa_b = 0; khsa_b < khsa_nb; ++khsa_b) {
        float* khsa_m = &khsa_a[khsa_b * 12u];
        const int32_t khsa_px = khsa_b < khsa_in.drive_px.size() ? khsa_in.drive_px[khsa_b] : -1;
        if (khsa_px < 0 || static_cast<size_t>(khsa_px) >= khsa_pose.size() || !khsa_ok[khsa_px]) {
            khsa_m[0] = khsa_m[4] = khsa_m[8] = 1.0f;   // The root: the identity.
            continue;
        }
        const KhSkinXf& khsa_x = khsa_pose[khsa_px];
        const float* khsa_h = khsa_d.skin_bones[khsa_in.drive[khsa_b]].head;
        memcpy(khsa_m, khsa_x.q, sizeof(khsa_x.q));
        for (int c = 0; c < 3; ++c) {
            khsa_m[9 + c] = khsa_x.t[c] - (khsa_h[0] * khsa_x.q[0 * 3 + c] + khsa_h[1] * khsa_x.q[1 * 3 + c] +
                                           khsa_h[2] * khsa_x.q[2 * 3 + c]);
        }
    }
}
// One affine map per bone from the current pose - the job's input (in_a), a few floats per bone; also the cloth's
// guide. Game thread; into in_a only on an instance that is not busy (the guide is written busy or not - no worker
// reads it).
inline void kh_skin_affine(const KhSkinInst& khsa_in, const MeshDef& khsa_d, std::vector<float>& khsa_a) {
    kh_skin_affine_of(khsa_in, khsa_d, khsa_in.pose, khsa_in.pose_ok, khsa_a);
}

// The vertices of one pose, from a rest snapshot and the per-bone affine maps: out (positions stored in the pose's
// own box, which bc / bs return) and met (the metric positions, the collider's input). No instance state: the worker
// (kh_skin_run) and the consume-point step (kh_skin_draw_verts) both call it.
// KH_SKIN_GPU: khsr_full false = the metric positions and the box alone (a collider's pose while the GPU draws the
// mesh): out is sized to the vertex count, as for a full pose, but not written.
inline void kh_skin_verts(const KhSkinRest& khsr_d, const float* khsr_a, size_t khsr_nb, std::vector<MeshVertex>& khsr_out,
                          std::vector<float>& khsr_met, float khsr_bc[3], float khsr_bs[3], bool khsr_full = true) {
    const size_t khsr_nv = khsr_d.verts.size();
    const bool khsr_inf = khsr_d.inf.size() == khsr_nv;
    const uint8_t* khsr_drawn = khsr_d.drawn.size() == khsr_nv ? khsr_d.drawn.data() : nullptr;   // KH_LOD_SKIN.
    khsr_out.resize(khsr_nv);
    khsr_met.resize(khsr_nv * 3u);
    const float* khsr_e = khsr_d.ext;
    const float* khsr_c0 = khsr_d.ctr;
    float khsr_mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, khsr_mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (size_t khsr_i = 0; khsr_i < khsr_nv; ++khsr_i) {
        const MeshVertex& khsr_r = khsr_d.verts[khsr_i];
        MeshVertex& khsr_o = khsr_out[khsr_i];
        float khsr_p[3], khsr_n[3], khsr_t[3];
        for (int k = 0; k < 3; ++k) {
            khsr_p[k] = khsr_r.pos[k] * khsr_e[k] + khsr_c0[k];   // Authored, metres.
            khsr_n[k] = khsr_full ? khsr_r.nrm[k] / khsr_e[k] : 0.0f;   // The authored normal (KH_IMPORT_NRM; NORMALS above).
            khsr_t[k] = khsr_full ? khsr_r.tan[k] * khsr_e[k] : 0.0f;   // Covariant: KhVsCore's tan * size.
        }
        float khsr_pp[3] = { 0.0f, 0.0f, 0.0f }, khsr_nn[3] = { 0.0f, 0.0f, 0.0f }, khsr_tt[3] = { 0.0f, 0.0f, 0.0f };
        float khsr_ws = 0.0f;
        float khsr_br[3][3] = {};   // KH_SKIN_NRM_COF: the blended map's rows.
        if (khsr_inf) {
            const KhSkinInf& khsr_f = khsr_d.inf[khsr_i];
            for (int khsr_k = 0; khsr_k < 4; ++khsr_k) {
                const float khsr_w = khsr_f.w[khsr_k];
                if (!(khsr_w > 0.0f) || khsr_f.b[khsr_k] >= khsr_nb) continue;
                const float* khsr_m = khsr_a + static_cast<size_t>(khsr_f.b[khsr_k]) * 12u;
                for (int c = 0; c < 3; ++c) {
                    khsr_pp[c] += khsr_w * (khsr_p[0] * khsr_m[c] + khsr_p[1] * khsr_m[3 + c] + khsr_p[2] * khsr_m[6 + c] + khsr_m[9 + c]);
                    if (!khsr_full) continue;   // KH_SKIN_GPU: positions alone.
                    khsr_tt[c] += khsr_w * (khsr_t[0] * khsr_m[c] + khsr_t[1] * khsr_m[3 + c] + khsr_t[2] * khsr_m[6 + c]);
                    khsr_br[c][0] += khsr_w * khsr_m[c];   // KH_SKIN_NRM_COF: row c of the blend (khsk_b0 - b2).
                    khsr_br[c][1] += khsr_w * khsr_m[3 + c];
                    khsr_br[c][2] += khsr_w * khsr_m[6 + c];
                }
                khsr_ws += khsr_w;
            }
        }
        const float khsr_rest = khsr_ws < 1.0f ? 1.0f - khsr_ws : 0.0f;   // The weight left to the root.
        const bool khsr_in_box = khsr_drawn == nullptr || khsr_drawn[khsr_i] != 0u;
        for (int k = 0; k < 3; ++k) {
            khsr_pp[k] += khsr_rest * khsr_p[k];
            khsr_tt[k] += khsr_rest * khsr_t[k];
            if (!khsr_in_box) continue;
            if (khsr_pp[k] < khsr_mn[k]) khsr_mn[k] = khsr_pp[k];
            if (khsr_pp[k] > khsr_mx[k]) khsr_mx[k] = khsr_pp[k];
        }
        memcpy(&khsr_met[khsr_i * 3u], khsr_pp, sizeof(khsr_pp));
        if (!khsr_full) continue;   // KH_SKIN_GPU: out is not written.
        for (int k = 0; k < 3; ++k) khsr_br[k][k] += khsr_rest;   // KH_SKIN_NRM_COF: the root's share, the identity.
        kh_skin_nrm_cof(khsr_br, khsr_n, khsr_nn);   // KH_SKIN_NRM_COF: the normal by the blend's inverse transpose.
        memcpy(khsr_o.nrm, khsr_nn, sizeof(khsr_nn));   // Metric for now; the box pass below scales them.
        memcpy(khsr_o.tan, khsr_tt, sizeof(khsr_tt));
        khsr_o.tan[3] = khsr_r.tan[3];   // The chart's handedness does not deform.
        khsr_o.uv[0] = khsr_r.uv[0];
        khsr_o.uv[1] = khsr_r.uv[1];
    }
    for (int k = 0; k < 3; ++k) {
        if (!(khsr_mn[k] <= khsr_mx[k])) { khsr_mn[k] = khsr_mx[k] = 0.0f; }   // Nothing drawn.
        khsr_bc[k] = 0.5f * (khsr_mn[k] + khsr_mx[k]);
        const float khsr_s = khsr_mx[k] - khsr_mn[k];
        khsr_bs[k] = khsr_s > 1.0e-4f ? khsr_s : 1.0e-4f;   // A flat axis, as native_size.
    }
    // Stored in the pose's own box: KhVsCore scales a position by size, divides a normal by it and multiplies a
    // tangent by it.
    if (!khsr_full) return;   // KH_SKIN_GPU.
    for (size_t khsr_i = 0; khsr_i < khsr_nv; ++khsr_i) {
        MeshVertex& khsr_o = khsr_out[khsr_i];
        const float* khsr_p = &khsr_met[khsr_i * 3u];
        for (int k = 0; k < 3; ++k) {
            khsr_o.pos[k] = (khsr_p[k] - khsr_bc[k]) / khsr_bs[k];
            khsr_o.nrm[k] *= khsr_bs[k];
            khsr_o.tan[k] /= khsr_bs[k];
        }
    }
}
// The skinning: ON A WORKER, from in_rest and in_a only, into the back buffer. Writes that buffer's box and shape
// key (a new key when some vertex has left the shape that took the current one by more than KH_CLOTH_SHAPE_M, the
// cloth's rule), then publishes.
inline void kh_skin_run(KhSkinInst& khsr_in) {
    KH_PROF_SCOPE(KHP_W_SKIN_JOB);   // KH_PROF.
    const KhSkinRest& khsr_d = *khsr_in.in_rest;
    const uint32_t khsr_back = 1u - (khsr_in.front.load(std::memory_order_relaxed) & 1u);
    std::vector<MeshVertex>& khsr_out = khsr_in.out[khsr_back];
    const size_t khsr_nv = khsr_d.verts.size();
    const float* khsr_e = khsr_d.ext;    // The collider's rest pose below scales the authored positions as the vertices did.
    const float* khsr_c0 = khsr_d.ctr;
    float* khsr_bc = khsr_in.box_ctr[khsr_back];
    float* khsr_bs = khsr_in.box_size[khsr_back];
    // KH_SKIN_GPU: the vertices for a buffer the upload will write, the positions for a collider, neither for a job
    // that only records a collider switched off, nor for one of a binding hidden by KH_SKIN_1X_HIDDEN alone (the box
    // and key are then the last result's).
    const bool khsr_pos = khsr_in.in_verts || khsr_in.in_collide;
    if (khsr_in.in_verts && kh_stats_on()) g_skin_pool_full.fetch_add(1u, std::memory_order_relaxed);   // skinPoolFull.
    if (khsr_pos) {
        kh_skin_verts(khsr_d, khsr_in.in_a.data(), khsr_in.in_a.size() / 12u, khsr_out, khsr_in.met, khsr_bc, khsr_bs, khsr_in.in_verts);
    } else {
        khsr_out.resize(khsr_nv);   // The upload sizes the buffer from it.
        memcpy(khsr_bc, khsr_in.box_ctr[1u - khsr_back], sizeof(float) * 3u);
        memcpy(khsr_bs, khsr_in.box_size[1u - khsr_back], sizeof(float) * 3u);
    }
    khsr_in.out_ok[khsr_back] = khsr_in.in_verts;
    if (khsr_pos && kh_stats_on() && khsr_d.sup.ok && khsr_d.sup.root.size() * 12u == khsr_in.in_a.size()) {   // skinBoundWorst.
        float khsr_gc[3], khsr_gs[3];
        kh_skin_so_bound(khsr_d, khsr_in.in_a.data(), khsr_gc, khsr_gs);
        float khsr_ratio = 0.0f;
        for (int k = 0; k < 3; ++k) {   // The pad off first; an axis the exact box barely has is not a ratio.
            if (!(khsr_bs[k] >= 0.05f)) continue;
            const float q = (khsr_gs[k] - 2.0f * KH_SKIN_SO_PAD_M) / khsr_bs[k];
            if (q > khsr_ratio) khsr_ratio = q;
        }
        uint32_t khsr_rb = 0u;
        memcpy(&khsr_rb, &khsr_ratio, sizeof(khsr_rb));
        uint32_t khsr_cur = g_skin_bound_worst_bits.load(std::memory_order_relaxed);
        while (khsr_ratio == khsr_ratio && khsr_rb > khsr_cur &&
               !g_skin_bound_worst_bits.compare_exchange_weak(khsr_cur, khsr_rb, std::memory_order_relaxed)) {}
    }
    // COLLIDER: this pose's BVH, over level 0's triangles, in metres about the box centre (the stored positions
    // times the box: the space a view with bake = size queries, kh_physics_gather_colliders). The topology never
    // changes between poses, so the rest pose's BVH is built once and each pose is a copy of it refitted
    // (kh_physics_bvh_refit, a tenth of a build), with a full build only for a pose the refit refuses.
    if (khsr_in.in_collide && khsr_d.idx0.size() >= 3u) {
        const uint32_t khsr_ni = static_cast<uint32_t>(khsr_d.idx0.size());
        if (!khsr_in.col_tpl) {
            std::vector<float> khsr_rp(khsr_nv * 3u);   // The rest pose, authored metres.
            for (size_t khsr_i = 0; khsr_i < khsr_nv; ++khsr_i) {
                for (int k = 0; k < 3; ++k) khsr_rp[khsr_i * 3u + k] = khsr_d.verts[khsr_i].pos[k] * khsr_e[k] + khsr_c0[k];
            }
            std::shared_ptr<KhPhysicsBvh> khsr_tb = std::make_shared<KhPhysicsBvh>();
            kh_physics_bvh_build(*khsr_tb, khsr_rp.data(), static_cast<uint32_t>(khsr_nv), khsr_d.idx0.data(), khsr_ni,
                               &khsr_in.col_topo);
            khsr_in.col_tpl = khsr_tb;
        }
        std::vector<float> khsr_cp(khsr_nv * 3u);
        for (size_t khsr_i = 0; khsr_i < khsr_nv; ++khsr_i) {
            for (int k = 0; k < 3; ++k) khsr_cp[khsr_i * 3u + k] = khsr_in.met[khsr_i * 3u + k] - khsr_bc[k];
        }
        // The motion since the last collider pose, in the same space (about THIS box's centre: the difference is
        // the limbs' own, root-relative motion), unless the game thread cut the chain (in_cut). A pose that is not
        // finite carries none either.
        std::vector<float> khsr_pp;
        if (!khsr_in.in_cut && khsr_in.col_prev.size() == khsr_in.met.size()) {
            bool khsr_bad = false;
            for (size_t khsr_i = 0; khsr_i < khsr_nv && !khsr_bad; ++khsr_i) {
                if (!kh_cloth_finite3(&khsr_in.met[khsr_i * 3u]) || !kh_cloth_finite3(&khsr_in.col_prev[khsr_i * 3u])) khsr_bad = true;
            }
            if (!khsr_bad) {
                khsr_pp.resize(khsr_nv * 3u);
                for (size_t khsr_i = 0; khsr_i < khsr_nv; ++khsr_i) {
                    for (int k = 0; k < 3; ++k) khsr_pp[khsr_i * 3u + k] = khsr_in.col_prev[khsr_i * 3u + k] - khsr_bc[k];
                }
            }
        }
        khsr_in.col_prev = khsr_in.met;
        // KH_COL_CLEAR: col_prev names THIS pose from here on, so the identity of the pose vd swept from is taken
        // now and col_pose_seq stands at none until this pose's BVH exists: a job that throws below (an allocation)
        // leaves the next pose a cut chain, never a link to a pose its vd was not measured from.
        const uint32_t khsr_from = khsr_in.col_pose_seq;
        khsr_in.col_pose_seq = 0u;
        std::shared_ptr<KhPhysicsBvh> khsr_bv = std::make_shared<KhPhysicsBvh>(*khsr_in.col_tpl);
        // KH_COL_CONVEX: a convex collider's pose is its planes: the refit leaves the nodes below the root and the
        // pseudonormals alone.
        if (!kh_physics_bvh_refit(*khsr_bv, khsr_in.col_topo, khsr_cp.data(), static_cast<uint32_t>(khsr_nv), khsr_in.col_scr,
                                khsr_pp.empty() ? nullptr : khsr_pp.data(), khsr_in.in_convex)) {
            kh_physics_bvh_build(*khsr_bv, khsr_cp.data(), static_cast<uint32_t>(khsr_nv), khsr_d.idx0.data(), khsr_ni);
        }
        // KH_COL_CLEAR: vd is this pose less the LAST collider job's (col_prev), so that job's pose is the one this
        // sweeps from; none without vd.
        khsr_bv->prev_seq = khsr_bv->vd.empty() ? 0u : khsr_from;
        khsr_in.col_pose_seq = khsr_bv->pose_seq;
        khsr_in.bvh_of[khsr_back] = khsr_bv;
    } else {
        khsr_in.bvh_of[khsr_back].reset();
        khsr_in.col_prev.clear();   // Switched off: the next collider pose starts still.
        khsr_in.col_pose_seq = 0u;   // KH_COL_CLEAR.
    }
    bool khsr_moved = khsr_pos && (khsr_in.shape_key == 0u || khsr_in.shape_ref.size() != khsr_in.met.size());   // KH_SKIN_GPU: no positions, no test - the key stands.
    for (size_t khsr_i = 0; khsr_pos && khsr_i < khsr_in.met.size() && !khsr_moved; ++khsr_i) {
        if (!(fabsf(khsr_in.met[khsr_i] - khsr_in.shape_ref[khsr_i]) <= KH_CLOTH_SHAPE_M)) khsr_moved = true;
    }
    if (khsr_moved) {
        khsr_in.shape_ref = khsr_in.met;
        uint32_t khsr_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (khsr_k == 0u) khsr_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        khsr_in.shape_key = khsr_k;
    }
    khsr_in.key_of[khsr_back] = khsr_in.shape_key;
    uint32_t khsr_g = g_skin_gen_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    if (khsr_g == 0u) khsr_g = g_skin_gen_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    khsr_in.gen_of[khsr_back] = khsr_g;   // KH_SKIN_GEN_OF: before front's release store, as the box and key.
    khsr_in.front.store(khsr_back, std::memory_order_release);
    khsr_in.gen.store(khsr_g, std::memory_order_release);
}

// The pool's entry point (declared ahead of kh_cloth_worker). busy is cleared on every path out, after the publish:
// the game thread may write this instance's inputs the instant it turns false.
inline void kh_skin_job(const std::shared_ptr<KhSkinInst>& khsj_in) {
    try { kh_skin_run(*khsj_in); } catch (...) {}
    khsj_in->busy.store(false, std::memory_order_release);
}

// The rest snapshot for a mesh, made on first use. Game thread.
inline std::shared_ptr<const KhSkinRest> kh_skin_rest_of(int khro_mesh) {
    const auto khro_it = g_skin_rest.find(khro_mesh);
    if (khro_it != g_skin_rest.end()) return khro_it->second;
    const MeshDef& khro_d = mesh_def(khro_mesh);
    std::shared_ptr<KhSkinRest> khro_r = std::make_shared<KhSkinRest>();
    khro_r->verts = khro_d.verts;
    if (khro_d.skin_inf.size() == khro_d.verts.size()) khro_r->inf = khro_d.skin_inf;
    const uint32_t khro_n0 = mesh_base_icount(khro_d);
    if (khro_n0 <= khro_d.indices.size()) {
        khro_r->idx0.assign(khro_d.indices.begin(), khro_d.indices.begin() + khro_n0);
    }
    // KH_LOD_SKIN: the box set, every vertex some level draws (all of them, as a rule: empty then).
    if (khro_d.lod_n != 0) {
        khro_r->drawn.assign(khro_d.verts.size(), 0u);
        for (const uint32_t khro_v : khro_d.indices) {
            if (khro_v < khro_r->drawn.size()) khro_r->drawn[khro_v] = 1u;
        }
        if (std::find(khro_r->drawn.begin(), khro_r->drawn.end(), static_cast<uint8_t>(0u)) == khro_r->drawn.end()) {
            khro_r->drawn.clear();
        }
    }
    memcpy(khro_r->ctr, khro_d.native_ctr, sizeof(khro_r->ctr));
    memcpy(khro_r->ext, khro_d.native_ext, sizeof(khro_r->ext));
    // KH_SKIN_GPU: the bound's trees. A failure (an allocation) leaves sup.ok false and this mesh on the CPU path.
    try { kh_skin_sup_prepare(*khro_r, khro_d.skin_bones.size()); } catch (...) { khro_r->sup = KhSkinSup{}; khro_r->bone_box.clear(); }
    g_skin_rest.emplace(khro_mesh, khro_r);
    return khro_r;
}

// KH_SKEL_DRAW - the consume-point skinning (declared with kh_skin_pose_drawn). From a render-thread drawer's
// kh_attach_step (usually the cast fire's, else the injection's when the skin waited for it, or the render-thread
// flush's when no injection ran), under g_draw_list_mutex, with the stash just taken: the relative poses and the
// per-bone maps through the instance's drive tables (render thread), the vertices from the rest snapshot (any pool
// thread, KH_SKIN_FORK; KH_SKIN_GPU: or skinned on the GPU by kh_skin_so_batch), and the result written into the
// buffer the upload installed for this handle (g_skin_gpu; kh_skin_upload, in the flush, keeps creating and
// installing it, and writes the worker's result into it only when no step wrote it within KH_SKIN_STEP_OWN cycles,
// KhSkinGpu::step_cycle). The box the pose was stored in goes to the object as the upload's put does, and the step's
// centre takes it from there.
// Nothing here touches the instance's job inputs or its published buffers: the worker path stays whole for the
// collider and the cloth guide.
inline bool kh_skin_pose_read(const KhSkinInst& khpr_in) {   // Declared with kh_skin_pose_drawn.
    return khpr_in.cloth || khpr_in.want_collide;
}
// The per-bone maps (kh_skin_affine_of's layout) of the stash this cycle's step took, into khsd_a.draw_a. False
// when the stash, instance or drive tables do not fit. Shared by KH_SKEL_DRAW and KH_CLOTH_PIN_STEP so both draw one
// pose.
inline bool kh_skin_step_maps(KhAttach& khsd_a, const KhSkinInst* khsd_in, const RenderObject& khsd_o) {
    if (!khsd_a.skel_smp_ok || khsd_a.skel_smp_gen != khsd_a.skel_gen) return false;
    if (khsd_in->seq != khsd_o.seq || khsd_in->mesh != khsd_o.mesh || !kh_mesh_alive(khsd_in->mesh)) return false;
    const MeshDef& khsd_d = mesh_def(khsd_in->mesh);
    if (khsd_in->drive_px.size() != khsd_d.skin_bones.size() || khsd_in->drive.size() != khsd_d.skin_bones.size()) return false;
    // KH_SKIN_DRIVE_GEN: the drive tables index the proxy list of the binding they were built for (kh_skin_sync,
    // which rebuilds them only for a binding whose pose is drawn - kh_skin_pose_drawn: visible, or a casterOnly one
    // a shadow map takes - or that is a cloth or a collider); a rebind's new list against the old tables drives bones
    // from the wrong proxies. Until the sync catches up, the last pose stands.
    if (khsd_in->drive_mesh != khsd_in->mesh || khsd_in->drive_gen != khsd_a.skel_gen) return false;
    const size_t khsd_np = khsd_a.skel_proxy.size();
    if (khsd_a.skel_smp.size() != khsd_np * 12u) return false;
    khsd_a.draw_pose.assign(khsd_np, KhSkinXf());
    khsd_a.draw_ok.assign(khsd_np, 0u);
    for (size_t khsd_j = 0; khsd_j < khsd_np; ++khsd_j) {
        const float* khsd_s = &khsd_a.skel_smp[khsd_j * 12u];
        const float* const khsd_xd = khsd_a.skel_smp_d.size() == khsd_np * 3u ? &khsd_a.skel_smp_d[khsd_j * 3u]
                                                                                : nullptr;   // KH_PXY_SMOOTH.
        if (kh_skin_rel(khsd_s, khsd_s + 3, khsd_a.skel_smp_par, khsd_a.skel_smp_par + 3, khsd_a.draw_pose[khsd_j],
                        khsd_xd)) {
            khsd_a.draw_ok[khsd_j] = 1u;
        }
    }
    kh_skin_affine_of(*khsd_in, khsd_d, khsd_a.draw_pose, khsd_a.draw_ok, khsd_a.draw_a);
    return true;
}
// KH_SKEL_SHAPE - the caching passes (sun ladder, point-light shadows, occlusion sets) hash a slot's shape key
// (g_cloth_gen_slot). The draw's bytes are the consume-point step's, so the step publishes a key per written pose by
// the cloth's rule (a new key when some vertex left the keyed shape by more than KH_CLOTH_SHAPE_M, on metric
// positions), from the process-wide serial; otherwise a mesh posing in place keeps its old shadow.
inline uint32_t kh_skin_draw_shape_note(KhAttach& khds_a) {
    const std::vector<float>& khds_m = khds_a.draw_met;
    bool khds_moved = khds_a.draw_shape_key == 0u || khds_a.draw_shape_ref.size() != khds_m.size();
    for (size_t khds_i = 0; khds_i < khds_m.size() && !khds_moved; ++khds_i) {
        if (!(fabsf(khds_m[khds_i] - khds_a.draw_shape_ref[khds_i]) <= KH_CLOTH_SHAPE_M)) khds_moved = true;
    }
    if (khds_a.draw_shape_gpu) khds_moved = true;   // KH_SKIN_GPU: the reference is a palette, not these positions.
    if (khds_moved) {
        khds_a.draw_shape_gpu = false;
        khds_a.draw_shape_ref = khds_m;
        uint32_t khds_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (khds_k == 0u) khds_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        khds_a.draw_shape_key = khds_k;
    }
    return khds_a.draw_shape_key;
}
// KH_SKIN_GPU: the same key from the palette, for a pose whose vertices the CPU never sees. A vertex moves by a
// convex combination of its bones' maps' changes at it, and a map's change over a bone's rest box is largest at a
// corner, so 'no bone moved any corner of its box by more than KH_CLOTH_SHAPE_M on any axis' implies the vertex
// test above would have passed: this re-keys whenever that would, and sometimes when it would not (never a stale
// shadow; at most a re-render the vertex test would have skipped).
inline uint32_t kh_skin_so_shape_note(KhAttach& khsn_a, const KhSkinRest& khsn_r) {
    const std::vector<float>& a = khsn_a.draw_a;
    bool khsn_moved = khsn_a.draw_shape_key == 0u || !khsn_a.draw_shape_gpu || khsn_a.draw_shape_ref.size() != a.size() ||
                      khsn_r.bone_box.size() * 2u != a.size();
    for (size_t b = 0; !khsn_moved && b * 12u < a.size(); ++b) {
        const float* bx = &khsn_r.bone_box[b * 6u];
        if (!(bx[0] <= bx[3])) continue;   // The bone carries no vertex.
        const float* m = &a[b * 12u];
        const float* r = &khsn_a.draw_shape_ref[b * 12u];
        for (int c = 0; c < 3 && !khsn_moved; ++c) {
            // |centre . dM + dT| + half . |dM|: the largest change over the box.
            float d = m[9 + c] - r[9 + c], h = 0.0f;
            for (int j = 0; j < 3; ++j) {
                const float dm = m[j * 3 + c] - r[j * 3 + c];
                d += 0.5f * (bx[j] + bx[3 + j]) * dm;
                h += 0.5f * (bx[3 + j] - bx[j]) * fabsf(dm);
            }
            if (!(fabsf(d) + h <= KH_CLOTH_SHAPE_M)) khsn_moved = true;
        }
    }
    if (khsn_moved) {
        khsn_a.draw_shape_ref = a;
        khsn_a.draw_shape_gpu = true;
        uint32_t khsn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (khsn_k == 0u) khsn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        khsn_a.draw_shape_key = khsn_k;
    }
    return khsn_a.draw_shape_key;
}
// KH_SKIN_FORK - the consume-point skin in three parts, so kh_attach_step can run the middle one (the vertices, all
// of the cost) for every binding of a step at once across the cloth pool. prep: the checks and the maps (draw_a);
// false = this binding does not skin this step. verts: the pose into draw_out / draw_met and the box; touches only
// this binding's scratch and reads its rest snapshot, both stable while the step holds g_draw_list_mutex
// (kh_skin_sync writes skin and rest_draw only under it), and takes no lock. finish: the upload, the shape key and
// the box, on the render thread's context.
inline bool kh_skin_draw_prep(KhAttach& khsd_a, const RenderObject& khsd_o, ID3D11DeviceContext* khsd_ctx) {
    KhSkinInst* khsd_in = khsd_a.skin.get();
    if (!khsd_ctx || !khsd_in || khsd_in->cloth || !khsd_in->rest_draw) return false;
    return kh_skin_step_maps(khsd_a, khsd_in, khsd_o);
}
inline void kh_skin_draw_verts(KhAttach& khsd_a, float khsd_bc[3], float khsd_bs[3]) {
    const KhSkinInst* const khsd_in = khsd_a.skin.get();
    kh_skin_verts(*khsd_in->rest_draw, khsd_a.draw_a.data(), khsd_a.draw_a.size() / 12u, khsd_a.draw_out, khsd_a.draw_met,
                  khsd_bc, khsd_bs);
}
// KH_SKIN_GPU: khsd_so_vb = the buffer kh_skin_so_batch already skinned this pose into (the bytes are there; bc / bs
// are its bound); null = the pool's vertices (draw_out), written here. KH_SKIN_POOL: the buffer is the pair (a
// chunk's slice, or the binding's own at 0).
inline void kh_skin_draw_finish(const std::string& khsd_h, KhAttach& khsd_a, RenderObject& khsd_o,
                                ID3D11DeviceContext* khsd_ctx, const float khsd_bc[3], const float khsd_bs[3],
                                bool& khsd_moved, const KhVbRef& khsd_so_vb) {
    KhVbRef khsd_vb = khsd_so_vb;
    if (!khsd_vb) {
        const UINT khsd_bytes = static_cast<UINT>(khsd_a.draw_out.size() * sizeof(MeshVertex));
        if (khsd_bytes == 0u) return;
        bool khsd_default = false;   // KH_SKIN_GPU: a stream-output buffer takes no Map.
        bool khsd_pooled = false;    // KH_SKIN_POOL: a chunk's slice.
        {   // g_draw_list_mutex -> g_skin_mu: the upload takes g_skin_mu on its own, never inside the list mutex.
            std::lock_guard<std::mutex> khsd_g(g_skin_mu);
            const auto khsd_it = g_skin_gpu.find(khsd_h);
            if (khsd_it != g_skin_gpu.end() && khsd_it->second.vb && khsd_it->second.bytes == khsd_bytes) {
                khsd_vb = kh_skin_gpu_ref(khsd_it->second);
                khsd_default = khsd_it->second.so;
                khsd_pooled = khsd_it->second.chunk != nullptr;
            }
        }
        if (!khsd_vb) return;   // Not installed yet (the first upload after the binding creates it).
        if (khsd_default) {
            // KH_EA_SKIN_PRED: UpdateSubresource is predicated and a step runs in the engine's hook with no StateBackup
            // (KH_PRED_ASIDE): a predicate the engine left bound would skip the pose while the stamps below claim it.
            KhPredAside khsd_pa;
            khsd_pa.begin(khsd_ctx);
            kh_skin_so_write(khsd_ctx, khsd_vb, khsd_bytes, khsd_pooled, khsd_a.draw_out.data());   // Its slice alone.
            khsd_pa.end();
        } else {   // A DYNAMIC buffer is the binding's own (never pooled).
            D3D11_MAPPED_SUBRESOURCE khsd_m = {};
            if (FAILED(khsd_ctx->Map(khsd_vb.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khsd_m)) || !khsd_m.pData) return;
            memcpy(khsd_m.pData, khsd_a.draw_out.data(), khsd_bytes);
            khsd_ctx->Unmap(khsd_vb.vb, 0);
        }
    }
    kh_uvs_src_wrote(khsd_o.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again (kh_uvs_step follows this step).
    // The key of the bytes just written: the palette's for a GPU pose (its snapshot stood under the batch's checks,
    // in this same step).
    const uint32_t khsd_key = (khsd_so_vb && khsd_a.skin && khsd_a.skin->rest_draw)
                            ? kh_skin_so_shape_note(khsd_a, *khsd_a.skin->rest_draw) : kh_skin_draw_shape_note(khsd_a);
    {   // The stamp, after the bytes landed (the same edge as above; the map cannot change under the render thread).
        std::lock_guard<std::mutex> khsd_g(g_skin_mu);
        const auto khsd_it = g_skin_gpu.find(khsd_h);
        if (khsd_it != g_skin_gpu.end() && kh_skin_gpu_ref(khsd_it->second) == khsd_vb) {   // KH_SKIN_POOL: the pair.
            // KH_SKIN_RESTEP_KEY: bytes of a new shape (KH_SKEL_SHAPE's key, against the key of the bytes the buffer
            // held - the step's or the worker's) move the mesh for the passes that cache on it (the scene record,
            // the sun map's reopen: KH_SUN_RESTEP, KH_CAST_REPICK) as a moved box does; otherwise a pose that changed
            // inside its box keeps the earlier step's self-shadow. The held key is g_skin_gpu's (step_key or key),
            // not the slot's, so a binding under a vertex stage (whose slot carries the stage's key) takes it too.
            const uint32_t khsd_held = khsd_it->second.by_step ? khsd_it->second.step_key : khsd_it->second.key;
            if (khsd_held != khsd_key) khsd_moved = true;
            khsd_it->second.step_cycle = g_topo_cycles;
            khsd_it->second.by_step = true;
            khsd_it->second.step_key = khsd_key;
            if (khsd_a.skin) khsd_a.skin->step_cycle_pub.store(g_topo_cycles, std::memory_order_relaxed);   // KH_SKIN_GPU: the game thread's in_verts.
            // The slot draws these bytes now, so it carries their key for every later pass.
            if (khsd_o.slot < g_cloth_vb_slot.size() && khsd_o.slot < g_cloth_gen_slot.size() &&
                khsd_o.slot < g_cloth_voff_slot.size() && g_cloth_vb_slot[khsd_o.slot] == khsd_vb.vb &&
                g_cloth_voff_slot[khsd_o.slot] == khsd_vb.off) {   // KH_SKIN_POOL: the pair.
                g_cloth_gen_slot[khsd_o.slot] = khsd_key;
            }
        }
    }
    // The pose's box, as kh_skin_upload's put writes it: size in SQF order, the centre in engine axes; the step's
    // kh_skel_centre_d (kh_attach_skel_place) re-derives pos.
    const float khsd_sz[3] = { khsd_bs[0], khsd_bs[2], khsd_bs[1] };
    // sizeof(khsd_o.skel_ctr), never sizeof(khsd_bc): khsd_bc is an array PARAMETER and decays to a pointer (8
    // bytes, two floats), so sizeof(khsd_bc) would compare and copy only two of the centre's three lanes and the
    // forward lane would stop following the bone.
    if (memcmp(khsd_o.skel_ctr, khsd_bc, sizeof(khsd_o.skel_ctr)) != 0 || memcmp(khsd_o.size, khsd_sz, sizeof(khsd_sz)) != 0) {
        memcpy(khsd_o.skel_ctr, khsd_bc, sizeof(khsd_o.skel_ctr));
        memcpy(khsd_o.size, khsd_sz, sizeof(khsd_sz));
        khsd_moved = true;
    }
}
// kh_skin_sync: GAME THREAD, from flush_frame before the park: builds every skeletal binding's poses from the
// consume-point stash (KH_SKEL_SAMPLE: its proxies and their parent, read together at Draw3D), then hands every
// instance whose pose changed to the pool. A stale stash gives no new pose (the last stands); a proxy out of reach
// of its parent - one the attach has not yet moved off its spawn point too - holds its last pose (the rest pose
// before its first), as an attachment lane holds its transform.
inline void kh_skin_sync() {
    KH_PROF_SCOPE(KHP_SKIN_SYNC);   // KH_PROF.
    if (g_attach_n.load(std::memory_order_relaxed) == 0 && g_skin.empty()) return;
    // KH_SKIN_REST_EARLY: a binding's missing rest snapshot (kh_skin_rest_of: copies and the bound's per-bone trees,
    // tens of ms for a large mesh) is built here, holding no lock, as kh_physics_bvh_for builds outside its own - in
    // the locked walk below the render thread would wait it out on g_draw_list_mutex / g_skin_mu. The list is taken
    // under g_draw_list_mutex with the walk's own filters up to its kh_skin_rest_of, so the walk finds each one
    // cached; the snapshot is a function of the published mesh def alone (immutable; the mesh stays referenced, so no
    // collector takes it), so it is the one the walk would have built. g_skin_rest: game thread only.
    {
        std::vector<int> khss_miss;
        {
            std::lock_guard<std::mutex> khss_pg(g_draw_list_mutex);
            for (const auto& khss_kv : g_attach) {
                if (!khss_kv.second.skel) continue;
                const auto khss_oit = g_draw_list.find(khss_kv.first);
                if (khss_oit == g_draw_list.end()) continue;
                const RenderObject& khss_o = khss_oit->second;
                if (!khss_o.skel || khss_o.fullscreen || khss_o.slot == 0xFFFFFFFFu) continue;
                if (!kh_mesh_alive(khss_o.mesh)) continue;
                if (g_skin_rest.find(khss_o.mesh) != g_skin_rest.end()) continue;
                if (std::find(khss_miss.begin(), khss_miss.end(), khss_o.mesh) == khss_miss.end()) {
                    khss_miss.push_back(khss_o.mesh);
                }
            }
        }
        for (const int khss_m : khss_miss) kh_skin_rest_of(khss_m);
    }
    std::vector<std::shared_ptr<KhSkinInst>> khss_jobs;
    {
        std::lock_guard<std::mutex> khss_g(g_draw_list_mutex);
        // KH_SKIN_MAP: the inserts and the field writes the render-thread upload reads. No call below takes a lock;
        // KH_SKIN_REST_EARLY built every rest snapshot it asks for.
        std::lock_guard<std::mutex> khss_sm(g_skin_mu);
        for (auto& khss_kv : g_skin) khss_kv.second->seen = false;
        for (auto& khss_kv : g_attach) {
            KhAttach& khss_a = khss_kv.second;
            if (!khss_a.skel) continue;
            const auto khss_oit = g_draw_list.find(khss_kv.first);
            if (khss_oit == g_draw_list.end()) continue;
            const RenderObject& khss_o = khss_oit->second;
            if (!khss_o.skel || khss_o.fullscreen || khss_o.slot == 0xFFFFFFFFu) continue;
            if (!kh_mesh_alive(khss_o.mesh)) continue;
            // A simulating mesh is the cloth's to draw (CLOTH above); an unpainted one does not simulate, so the skin
            // keeps it. KH_CHAIN: a simulating chain is drawn the same way, its bones guiding its start (and its end,
            // where it follows the skeleton). KH_CHAIN_REFUSED_SKIN: a chain whose build was refused does not
            // simulate either (no instance, nothing draws it), so the skin keeps it from the frame after the refusal
            // is filed.
            const bool khss_cloth = (kh_cloth_obj_sim(khss_o) && kh_cloth_mesh_ok(khss_o.mesh)) ||
                                    (kh_chain_obj_sim(khss_o) && kh_chain_mesh_ok(khss_o.mesh) &&
                                     !kh_chain_refused(khss_kv.first, khss_o));
            std::shared_ptr<KhSkinInst>& khss_sp = g_skin[khss_kv.first];
            if (!khss_sp || khss_sp->seq != khss_o.seq || khss_sp->mesh != khss_o.mesh) {
                // Another object or mesh: a fresh instance. One a worker still holds finishes into nothing and dies
                // with its job.
                khss_sp = std::make_shared<KhSkinInst>();
                khss_sp->seq = khss_o.seq;
                khss_sp->mesh = khss_o.mesh;
            }
            KhSkinInst& khss_in = *khss_sp;
            khss_in.seen = true;
            khss_a.skin = khss_sp;   // KH_SKEL_DRAW: the render-thread drawers' steps skin through this.
            khss_in.rest_draw = kh_skin_rest_of(khss_in.mesh);
            if (khss_in.skel_gen != khss_a.skel_gen || khss_in.pose.size() != khss_a.skel_proxy.size()) {
                // New proxies: their poses start over.
                khss_in.cut = true;
                khss_in.skel_gen = khss_a.skel_gen;
                khss_in.pose.assign(khss_a.skel_proxy.size(), KhSkinXf());
                khss_in.pose_ok.assign(khss_a.skel_proxy.size(), 0u);
            }
            khss_in.slot = khss_o.slot;
            khss_in.cloth = khss_cloth;
            khss_in.want_collide = kh_physics_obj_collider(khss_o);
            khss_in.want_convex = khss_o.physics_col_convex;   // KH_COL_CONVEX.
            // KH_SKIN_HIDDEN_COL; KH_SKIN_CASTER. KH_SKIN_1X_HIDDEN: at FSAA 1x no world mesh is drawn and no step
            // skins a buffer (kh_fsaa_world_standdown), so a visible binding is hidden too - no full CPU skin of
            // bytes nothing reads (a collider still gets its positions, a first result its vertices).
            khss_in.hidden.store(!kh_skin_pose_drawn(khss_o) || kh_fsaa_world_standdown(), std::memory_order_relaxed);
            // A hidden mesh keeps its instance and its last result, so the buffer the upload installs and the box
            // the object carries stay one pair while hidden and when shown again; one not drawn stops asking for
            // new poses (one hidden by KH_SKIN_1X_HIDDEN alone still asks - its job takes no vertices). A cloth
            // simulates hidden or not, so its guide does not wait; a hidden collider still collides, so its pose
            // does not wait either.
            if (!kh_skin_pose_drawn(khss_o) && !khss_cloth && !khss_in.want_collide) continue;   // KH_SKIN_CASTER.
            if (khss_in.drive_mesh != khss_in.mesh || khss_in.drive_gen != khss_in.skel_gen) {
                kh_skin_drive(khss_in, mesh_def(khss_in.mesh), khss_a.skel_mem);
                khss_in.drive_mesh = khss_in.mesh;
                khss_in.drive_gen = khss_in.skel_gen;
                khss_in.dirty = true;
                khss_in.cut = true;
            }
            // KH_SKEL_SAMPLE: the consume-point stash (kh_attach_step: the cycle's Draw3D sample), taken whole or not
            // at all - the proxies and their parent are one pair. Not fresh: no new pose this sync; the last stands.
            const bool khss_smp = khss_a.skel_smp_ok && khss_a.skel_smp_gen == khss_a.skel_gen &&
                                  khss_a.skel_smp.size() == khss_a.skel_proxy.size() * 12u &&
                                  steady_now_ms() - khss_a.skel_smp_ms < KH_SKEL_SAMPLE_KEEP_MS;
            for (size_t khss_j = 0; khss_smp && khss_j < khss_a.skel_proxy.size(); ++khss_j) {
                KhSkinXf khss_x;
                const float* const khss_xd = khss_a.skel_smp_d.size() == khss_a.skel_proxy.size() * 3u
                                           ? &khss_a.skel_smp_d[khss_j * 3u] : nullptr;   // KH_PXY_SMOOTH.
                if (!kh_skin_rel(&khss_a.skel_smp[khss_j * 12u], &khss_a.skel_smp[khss_j * 12u + 3u],
                                 khss_a.skel_smp_par, khss_a.skel_smp_par + 3, khss_x, khss_xd)) continue;
                khss_in.pose[khss_j] = khss_x;
                if (!khss_in.pose_ok[khss_j]) khss_in.cut = true;   // Rest to live: not a motion.
                khss_in.pose_ok[khss_j] = 1u;
            }
            if (khss_cloth) {
                // The cloth's guide, every frame (a few floats per bone). The skin's own job waits (dirty) until the
                // cloth lets go; a job still running from before finishes into a buffer the upload does not install.
                kh_skin_affine(khss_in, mesh_def(khss_in.mesh), khss_in.guide);
                khss_in.dirty = true;
                khss_in.cut = true;   // No collider pose is built meanwhile: the chain breaks here.
                continue;
            }
            khss_jobs.push_back(khss_sp);
        }
    }
    {   // KH_SKIN_MAP: the erase.
        std::lock_guard<std::mutex> khss_se(g_skin_mu);
        for (auto khss_it = g_skin.begin(); khss_it != g_skin.end();) {
            if (!khss_it->second->seen) khss_it = g_skin.erase(khss_it);
            else ++khss_it;
        }
    }
    // Rest snapshots no live instance's mesh uses. A job still running holds its own reference.
    for (auto khss_it = g_skin_rest.begin(); khss_it != g_skin_rest.end();) {
        bool khss_used = false;
        for (const auto& khss_kv : g_skin) if (khss_kv.second->mesh == khss_it->first) { khss_used = true; break; }
        if (!khss_used) khss_it = g_skin_rest.erase(khss_it);
        else ++khss_it;
    }
    bool khss_queued = false;
    for (const std::shared_ptr<KhSkinInst>& khss_sp : khss_jobs) {
        KhSkinInst& khss_in = *khss_sp;
        // A new job only when a pose some bone follows has changed since the last one was handed over, or the
        // upload asked for one. KH_SKIN_NEED_FULL: need_full is the upload's request for a result that carries
        // vertices, and is a reason on its own: a collider's jobs keep done current (a mesh that is no collider
        // leaves done behind through its skipped jobs, so its pose still differs), so were a pose change the only
        // reason, a collider whose pose stood bit-still from the moment the step stopped owning its buffer would never
        // hand the full job over - the upload would ask again every pass, take nothing, and the last positions-only
        // result (its BVH and box) would never be installed. The request makes the job below a full one (it clears
        // khss_owned) and that job clears it.
        bool khss_need = khss_in.dirty || khss_in.done.size() != khss_in.pose.size() ||
                         khss_in.done_collide != khss_in.want_collide ||   // COLLIDER: switched on or off.
                         khss_in.done_convex != khss_in.want_convex ||     // KH_COL_CONVEX: the pose's refit changes.
                         khss_in.need_full.load(std::memory_order_relaxed);   // KH_SKIN_NEED_FULL.
        for (size_t khss_b = 0; khss_b < khss_in.drive_px.size() && !khss_need; ++khss_b) {
            const int32_t khss_j = khss_in.drive_px[khss_b];
            if (khss_j < 0) continue;
            if (khss_in.done_ok[khss_j] != khss_in.pose_ok[khss_j] ||
                memcmp(&khss_in.done[khss_j], &khss_in.pose[khss_j], sizeof(KhSkinXf)) != 0) khss_need = true;
        }
        if (!khss_need) continue;
        // KH_SKIN_GPU: while the render thread's step owns the buffer (it wrote it within KH_SKIN_STEP_OWN cycles,
        // the span kh_skin_upload leaves the bytes and the box to it) the worker's vertices would be thrown away. A
        // collider's job then takes the positions alone; a mesh that is no collider takes no job at all, unless its
        // collider flags just changed (that one job records the switch: the BVH dropped, the motion chain cut).
        // dirty and done stand through a skipped job, so the moment the step stops owning (a hidden mesh, a session
        // the render thread does not flush in) the next sync hands over a full one.
        const uint64_t khss_sc = khss_in.step_cycle_pub.load(std::memory_order_relaxed);
        const uint64_t khss_now = g_topo_cycles_pub.load(std::memory_order_relaxed);
        const bool khss_owned = khss_sc != ~0ull && khss_now >= khss_sc && khss_now - khss_sc <= KH_SKIN_STEP_OWN &&
                                khss_in.gen.load(std::memory_order_relaxed) != 0u &&
                                !khss_in.need_full.load(std::memory_order_relaxed);
        if (khss_owned && !khss_in.want_collide && khss_in.done_collide == khss_in.want_collide &&
            khss_in.done_convex == khss_in.want_convex) continue;
        if (khss_in.busy.load(std::memory_order_acquire)) {
            // Still skinning the last pose: this one waits a frame (done is left alone, so the next sync sees it
            // again) and is counted.
            if (kh_stats_on()) g_skin_skipped.fetch_add(1u, std::memory_order_relaxed);
            continue;
        }
        const MeshDef& khss_d = mesh_def(khss_in.mesh);
        kh_skin_affine(khss_in, khss_d, khss_in.in_a);
        khss_in.in_rest = kh_skin_rest_of(khss_in.mesh);
        khss_in.done = khss_in.pose;
        khss_in.done_ok = khss_in.pose_ok;
        khss_in.dirty = false;
        khss_in.in_collide = khss_in.want_collide;
        khss_in.done_collide = khss_in.want_collide;
        khss_in.in_convex = khss_in.want_convex;   // KH_COL_CONVEX.
        khss_in.done_convex = khss_in.want_convex;
        // KH_SKIN_GPU: no vertices while the step owns the buffer. KH_SKIN_HIDDEN_COL: nor while the object is
        // hidden - unless the upload asked (need_full: a buffer that holds no pose yet) or this is the first result.
        khss_in.in_verts = !khss_owned && (!khss_in.hidden.load(std::memory_order_relaxed) ||
                                           khss_in.need_full.load(std::memory_order_relaxed) ||
                                           khss_in.gen.load(std::memory_order_relaxed) == 0u);
        if (khss_in.in_verts) khss_in.need_full.store(false, std::memory_order_relaxed);   // This job answers it.
        khss_in.in_cut = khss_in.cut;
        khss_in.cut = false;
        if (!khss_queued) { kh_cloth_workers_start(); khss_queued = true; }
        {
            std::lock_guard<std::mutex> khss_q(g_cloth_mu);
            g_skin_q.push_back(khss_sp);
            khss_in.busy.store(true, std::memory_order_release);   // KH_POOL_BUSY_AFTER_PUSH (kh_cloth_sync).
            g_skin_q_n.store(static_cast<uint32_t>(g_skin_q.size()), std::memory_order_relaxed);
        }
        g_skin_cv.notify_one();   // KH_POOL_SPLIT: the skin pool's own wake.
    }
}

// KH_SKEL - the pose a skeletal binding hands its cloth: one affine map per bone from this frame's kh_skin_sync,
// for this creation stamp and mesh. False when no binding of it is the cloth's, or its mesh has no bones. Called
// from both threads (kh_cloth_sync, and kh_cloth_upload via kh_cloth_pin_fresh), so it walks g_skin under g_skin_mu.
inline bool kh_skin_guide_of(uint64_t khgo_seq, int khgo_mesh, std::vector<float>& khgo_out) {
    std::lock_guard<std::mutex> khgo_g(g_skin_mu);
    for (const auto& khgo_kv : g_skin) {
        const KhSkinInst& khgo_in = *khgo_kv.second;
        if (!khgo_in.cloth || !khgo_in.seen || khgo_in.seq != khgo_seq || khgo_in.mesh != khgo_mesh) continue;
        khgo_out = khgo_in.guide;
        return !khgo_out.empty();
    }
    return false;
}

// Device side, in the flush after kh_cloth_upload (which empties the slot tables, installs the cloth's buffers and
// records their boxes). Uploads and installs each newly published skinned result, then hands every binding's object
// the box of what its slot draws, under g_draw_list_mutex: the skinned buffer's box, a cloth's installed buffer's
// box, or the rest box. A failed upload leaves the last installed result and box. Box, key and buffer all come from
// the same published result (KhSkinGpu).
inline void kh_skin_upload(ID3D11DeviceContext* khsu_ctx, ID3D11Device* khsu_dev) {
    KH_PROF_SCOPE(KHP_SKIN_UPLOAD);   // KH_PROF.
    if (!khsu_ctx || !khsu_dev) return;
    uint64_t khsu_meshes = 0, khsu_bones = 0, khsu_driven = 0, khsu_proxies = 0;
    uint64_t khsu_pooled = 0;   // KH_SKIN_POOL.
    // put = false: vb holds the render thread's step's bytes (KhSkinGpu::by_step - this cycle's or an earlier one's,
    // no worker write since), and the step wrote their box with them; the buffer's vertices are normalised to THAT
    // box (kh_skin_verts), so the object keeps it.
    // vbase: KH_SKIN_POOL, the slice's vertex base (0 = none), handed over whatever 'put' says.
    struct KhSkinPut { std::string h; uint64_t seq; int mesh; float ctr[3]; float size[3]; bool put; uint32_t vbase; };
    std::vector<KhSkinPut> khsu_put;
    KhSkinColMap khsu_col;   // This upload's collider table, published with the boxes.
    // A put for this binding, holding the rest box (kh_skel_rest_box's values) until something installed says
    // otherwise.
    auto khsu_start = [](const std::string& khsu_h, const KhSkinInst& khsu_in) {
        KhSkinPut khsu_p;
        khsu_p.h = khsu_h;
        khsu_p.seq = khsu_in.seq;
        khsu_p.mesh = khsu_in.mesh;
        khsu_p.put = true;
        khsu_p.vbase = 0u;
        const MeshDef& khsu_d = mesh_def(khsu_in.mesh);
        memcpy(khsu_p.ctr, khsu_d.native_ctr, sizeof(khsu_p.ctr));
        memcpy(khsu_p.size, khsu_d.native_size, sizeof(khsu_p.size));   // SQF order.
        return khsu_p;
    };
    // An installed buffer's box: engine axes in, the object's SQF order out.
    auto khsu_box = [](KhSkinPut& khsu_p, const float* khsu_c, const float* khsu_s) {
        memcpy(khsu_p.ctr, khsu_c, sizeof(khsu_p.ctr));
        khsu_p.size[0] = khsu_s[0];
        khsu_p.size[1] = khsu_s[2];
        khsu_p.size[2] = khsu_s[1];
    };
    {
        // The cloth's bindings, and the counts for every binding.
        std::lock_guard<std::mutex> khsu_c(g_cloth_mu);
        std::lock_guard<std::mutex> khsu_sk(g_skin_mu);   // Cloth -> skin order.
        for (auto& khsu_kv : g_skin) {
            KhSkinInst& khsu_in = *khsu_kv.second;
            khsu_proxies += khsu_in.pose.size();
            if (!kh_mesh_alive(khsu_in.mesh)) continue;
            khsu_bones += mesh_def(khsu_in.mesh).skin_bones.size();
            khsu_driven += khsu_in.bones_driven;
            if (!khsu_in.cloth) continue;
            KhSkinPut khsu_p = khsu_start(khsu_kv.first, khsu_in);
            const auto khsu_ci = g_cloth.find(khsu_in.slot);
            if (khsu_ci != g_cloth.end()) {
                const KhClothInst& khsu_cl = *khsu_ci->second;
                const uint32_t khsu_s = khsu_in.slot;
                const bool khsu_on = !khsu_cl.retired && khsu_cl.seq == khsu_in.seq && khsu_cl.mesh == khsu_in.mesh &&
                                     khsu_cl.vb && khsu_s < g_cloth_vb_slot.size() && g_cloth_vb_slot[khsu_s] == khsu_cl.vb;
                if (khsu_on && khsu_cl.uploaded_box_on) khsu_box(khsu_p, khsu_cl.uploaded_box_ctr, khsu_cl.uploaded_box_size);
            }
            khsu_put.push_back(khsu_p);
        }
    }
    // KH_SKIN_GPU: the device objects, before any lock (a first compile takes a moment), once some binding has a
    // result to install.
    bool khsu_so_ok = false;
    if (KH_SKIN_GPU_ON && kh_skin_so_askable()) {   // KH_SKIN_SO_RETRY.
        bool khsu_any = false;
        {
            std::lock_guard<std::mutex> khsu_g(g_skin_mu);
            for (const auto& khsu_kv : g_skin) {
                if (!khsu_kv.second->cloth && khsu_kv.second->gen.load(std::memory_order_acquire) != 0u) { khsu_any = true; break; }
            }
        }
        if (khsu_any) khsu_so_ok = kh_skin_so_ensure(khsu_dev);
    }
    // KH_SKIN_SO_LAZY: the large class, once some binding with a result has a palette only it takes - asked only
    // while it is missing and askable (after the prewarm, no standing refusal), so a made or refused class costs
    // nothing here; until then each flush walks the bindings once under g_skin_mu (a session without such a
    // skeleton: every flush, a few compares a binding).
    if (khsu_so_ok && !kh_skin_so_big_ready() && kh_skin_so_big_askable()) {
        bool khsu_big = false;
        {
            std::lock_guard<std::mutex> khsu_g(g_skin_mu);
            for (const auto& khsu_kv : g_skin) {
                const KhSkinInst& khsu_bi = *khsu_kv.second;
                if (khsu_bi.cloth || khsu_bi.gen.load(std::memory_order_acquire) == 0u || !khsu_bi.rest_draw) continue;
                const size_t khsu_bn = khsu_bi.rest_draw->sup.pal.size();
                if (khsu_bi.rest_draw->sup.ok && khsu_bn > KH_SKIN_SO_BONES_S && khsu_bn <= KH_SKIN_SO_BONES) {
                    khsu_big = true;
                    break;
                }
            }
        }
        if (khsu_big) kh_skin_so_big_ensure(khsu_dev);
    }
    {
        // g_cloth_mu first (cloth -> skin): every job leaves g_skin_q under it, so none can start writing out[front]
        // (the buffer this walk copies, and the box, key and BVH beside it) before the walk ends. A job already
        // running writes the other buffer. The cloth's upload holds it for the same reason.
        std::lock_guard<std::mutex> khsu_c2(g_cloth_mu);
        std::lock_guard<std::mutex> khsu_g(g_skin_mu);
        for (auto& khsu_rg : g_skin_rest_gpu) khsu_rg.second.seen = false;   // KH_SKIN_GPU: swept below.
        for (auto khsu_it = g_skin_gpu.begin(); khsu_it != g_skin_gpu.end();) {
            if (g_skin.find(khsu_it->first) == g_skin.end()) {   // Its instance went: released here, in the flush.
                kh_skin_gpu_vb_drop(khsu_it->second);   // KH_SKIN_POOL: or its slice handed back.
                khsu_it = g_skin_gpu.erase(khsu_it);
            } else ++khsu_it;
        }
        kh_skin_pool_trim(khsu_dev, khsu_ctx);   // KH_SKIN_POOL: a thinned pool repacked, before any install.
        if (khsu_so_ok && !g_skin_pool_failed) {   // KH_SKIN_POOL: this flush's new slices, reserved per pool.
            std::unordered_map<uint64_t, uint32_t> khsu_need;
            for (const auto& khsu_rk : g_skin) {
                const KhSkinInst& khsu_ri = *khsu_rk.second;
                if (khsu_ri.cloth || !kh_mesh_alive(khsu_ri.mesh) || khsu_ri.mesh < 0 ||
                    khsu_ri.gen.load(std::memory_order_acquire) == 0u) continue;
                const KhSkinRest* const khsu_rr = khsu_ri.rest_draw.get();
                const size_t khsu_rb = mesh_def(khsu_ri.mesh).skin_bones.size();
                if (!khsu_rr || !khsu_rr->sup.ok || khsu_rb == 0u ||
                    !kh_skin_so_class_ok(*khsu_rr) ||   // KH_SKIN_SO_BIG / KH_SKIN_SO_LAZY.
                    khsu_rr->sup.root.size() != khsu_rb || khsu_rr->inf.size() != khsu_rr->verts.size() ||
                    khsu_rr->verts.empty() || khsu_rr->verts.size() > 0xFFFFFFFFull / sizeof(MeshVertex)) continue;
                const UINT khsu_rv = static_cast<UINT>(khsu_rr->verts.size() * sizeof(MeshVertex));
                const uint64_t khsu_rkey = (static_cast<uint64_t>(static_cast<uint32_t>(khsu_ri.mesh)) << 32) | khsu_rv;
                const auto khsu_rg = g_skin_gpu.find(khsu_rk.first);
                // KH_SKIN_GPU_MOVE: a CPU-skinned buffer the walk moves to stream output takes a slice, new result or
                // not - when its rest stream stands (one the walk makes again this flush is taken by growing).
                const auto khsu_rs = g_skin_rest_gpu.find(khsu_rr);
                const bool khsu_rmv = khsu_rg != g_skin_gpu.end() && khsu_rs != g_skin_rest_gpu.end() &&
                                      khsu_rs->second.vb && kh_skin_gpu_movable(khsu_rg->second, *khsu_rr, khsu_rb);
                if (!khsu_rmv && khsu_rg != g_skin_gpu.end() && khsu_rg->second.valid &&   // The walk skips it.
                    khsu_rg->second.gen == khsu_ri.gen.load(std::memory_order_acquire)) continue;
                const uint32_t khsu_rf = khsu_ri.front.load(std::memory_order_acquire) & 1u;
                if (khsu_ri.out[khsu_rf].size() * sizeof(MeshVertex) != khsu_rv) continue;   // Not stream output.
                if (!khsu_rmv && khsu_rg != g_skin_gpu.end() && khsu_rg->second.vb &&
                    khsu_rg->second.bytes == khsu_rv &&
                    (!khsu_rg->second.chunk || khsu_rg->second.chunk->key == khsu_rkey)) continue;   // It stands.
                ++khsu_need[khsu_rkey];
            }
            for (const auto& khsu_nk : khsu_need) {
                kh_skin_pool_reserve(khsu_dev, khsu_ctx, khsu_nk.first, khsu_nk.second);
            }
        }
        for (auto& khsu_kv : g_skin) {
            KhSkinInst& khsu_in = *khsu_kv.second;
            if (khsu_in.cloth || !kh_mesh_alive(khsu_in.mesh)) continue;
            KhSkinPut khsu_p = khsu_start(khsu_kv.first, khsu_in);
            {   // KH_SKIN_GPU: this binding's rest stream stays, and is made here when its snapshot changed under a
                // buffer that stands (a mesh of the same byte size, a rebuilt snapshot): the batch would find none
                // and leave it to the CPU.
                const auto khsu_gq = g_skin_gpu.find(khsu_kv.first);
                if (khsu_so_ok && khsu_gq != g_skin_gpu.end() && khsu_gq->second.vb && khsu_gq->second.so) {
                    kh_skin_rest_gpu_ensure(khsu_dev, khsu_in.rest_draw, mesh_def(khsu_in.mesh).skin_bones.size());
                } else {
                    const auto khsu_rg = g_skin_rest_gpu.find(khsu_in.rest_draw.get());
                    if (khsu_rg != g_skin_rest_gpu.end()) khsu_rg->second.seen = true;
                }
            }
            const uint32_t khsu_gen = khsu_in.gen.load(std::memory_order_acquire);
            if (khsu_gen != 0u) {
                KhSkinGpu& khsu_gp = g_skin_gpu[khsu_kv.first];
                // KH_SKIN_GPU_MOVE: a CPU-skinned buffer whose binding the GPU can skin now (the device refused it a
                // stream-output buffer or its mesh's rest stream before) moves to stream output, its bytes and record
                // standing; the install below then takes it as a stream-output buffer that stood.
                {
                    const size_t khsu_mvb = mesh_def(khsu_in.mesh).skin_bones.size();
                    if (khsu_so_ok && khsu_in.rest_draw && kh_skin_gpu_movable(khsu_gp, *khsu_in.rest_draw, khsu_mvb) &&
                        kh_skin_rest_gpu_ensure(khsu_dev, khsu_in.rest_draw, khsu_mvb)) {
                        // KH_UVS_SRC_FORGET: a vertex stage over this binding primes again from the new buffer - its
                        // source is a weak pair, and a chunk a later take grows could land at the old one's address.
                        if (kh_skin_gpu_move(khsu_dev, khsu_ctx, khsu_in.mesh, khsu_gp)) {
                            kh_uvs_src_forget(khsu_in.slot);
                            kh_stat(g_stats.skin_gpu_moves);   // skinGpuMoves.
                        } else {
                            kh_stat(g_stats.skin_gpu_refused);   // skinGpuRefused.
                        }
                    }
                }
                if (!khsu_gp.valid || khsu_gp.gen != khsu_gen) {
                    const uint32_t khsu_f = khsu_in.front.load(std::memory_order_acquire) & 1u;
                    // KH_SKIN_GEN_OF: the buffer's own gen - khsu_gen, read first, can name the result before it
                    // (a job stores front, then gen), and installing under it would install this one twice.
                    const uint32_t khsu_gf = khsu_in.gen_of[khsu_f];
                    const std::vector<MeshVertex>& khsu_src = khsu_in.out[khsu_f];
                    const UINT khsu_bytes = static_cast<UINT>(khsu_src.size() * sizeof(MeshVertex));
                    if (khsu_bytes != 0u) {
                        // KH_SKIN_POOL: and a slice of another mesh's pool (the binding's mesh changed at the same
                        // byte size) is handed back, so a chunk holds one mesh's poses.
                        const uint64_t khsu_pm = static_cast<uint64_t>(static_cast<uint32_t>(khsu_in.mesh));
                        if (khsu_gp.vb && (khsu_gp.bytes != khsu_bytes ||
                                           (khsu_gp.chunk && (khsu_gp.chunk->key >> 32) != khsu_pm))) {
                            kh_skin_gpu_vb_drop(khsu_gp);   // KH_SKIN_POOL: or its slice handed back.
                            khsu_gp.valid = false;
                            khsu_gp.step_cycle = ~0ull;
                            khsu_gp.by_step = false;
                            khsu_gp.step_key = 0u;
                            // KH_UVS_SRC_FORGET: the buffer made below may land at the dropped one's address - a vertex
                            // stage over this binding primes again from it (a move's rule).
                            kh_uvs_src_forget(khsu_in.slot);
                        }
                        if (!khsu_gp.vb) {
                            // KH_SKIN_GPU: a buffer the GPU can skin into (DEFAULT with the stream-output bind) when
                            // the objects, this mesh's trees and its rest stream all exist and its palette fits; the
                            // CPU's DYNAMIC buffer otherwise, and if the device refuses the first. KH_SKIN_POOL: the
                            // first is a slice of the mesh's pool, or a buffer of the binding's own when the pool
                            // cannot give one. KH_SKIN_GPU_MOVE: a CPU buffer made for a refusal moves to stream
                            // output once the device gives one.
                            khsu_gp.so = false;
                            const std::shared_ptr<const KhSkinRest>& khsu_rest = khsu_in.rest_draw;
                            const size_t khsu_nb = mesh_def(khsu_in.mesh).skin_bones.size();
                            bool khsu_so = khsu_so_ok && khsu_rest && khsu_rest->sup.ok && khsu_nb != 0u &&
                                           kh_skin_so_class_ok(*khsu_rest) &&   // KH_SKIN_SO_BIG / KH_SKIN_SO_LAZY.
                                           khsu_rest->sup.root.size() == khsu_nb && khsu_rest->inf.size() == khsu_rest->verts.size() &&
                                           khsu_rest->verts.size() * sizeof(MeshVertex) == khsu_bytes;
                            if (khsu_so) khsu_so = kh_skin_rest_gpu_ensure(khsu_dev, khsu_rest, khsu_nb) != nullptr;   // The rest stream, once per snapshot.
                            const bool khsu_sow = khsu_so;   // KH_SKIN_GPU_MOVE: stream output wanted (and possible).
                            D3D11_BUFFER_DESC khsu_bd = {};
                            khsu_bd.ByteWidth = khsu_bytes;
                            if (khsu_so && !kh_skin_pool_take(khsu_dev, khsu_ctx, khsu_in.mesh, khsu_bytes, khsu_gp)) {
                                khsu_bd.Usage = D3D11_USAGE_DEFAULT;
                                khsu_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;
                                if (FAILED(khsu_dev->CreateBuffer(&khsu_bd, nullptr, &khsu_gp.vb))) { khsu_gp.vb = nullptr; khsu_so = false; }
                            }
                            if (!khsu_so) {
                                khsu_bd.Usage = D3D11_USAGE_DYNAMIC;
                                khsu_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                                khsu_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                                if (FAILED(khsu_dev->CreateBuffer(&khsu_bd, nullptr, &khsu_gp.vb))) khsu_gp.vb = nullptr;
                            }
                            // KH_SKIN_GPU_MOVE: stream output wanted and both its buffers refused - asked again (a
                            // move) KH_SKIN_GPU_RETRY cycles on, or by this install every flush while the DYNAMIC
                            // buffer is refused too; any other outcome leaves no refusal standing.
                            khsu_gp.so_refused = (khsu_sow && !khsu_so) ? g_topo_cycles : ~0ull;
                            if (khsu_sow && !khsu_so) kh_stat(g_stats.skin_gpu_refused);   // skinGpuRefused.
                            if (khsu_gp.vb) { khsu_gp.bytes = khsu_bytes; khsu_gp.valid = false; khsu_gp.so = khsu_so; }
                        }
                        // KH_SKEL_DRAW: a buffer the render thread skinned this cycle keeps those bytes; the record
                        // still takes the result below. KH_SKIN_STEP_OWN - a buffer the step skinned within
                        // KH_SKIN_STEP_OWN cycles is the step's: the worker's result (a pose sampled at Draw3D,
                        // unpaired) updates the record (gen, key, box, collider) but not the bytes or the box. Past
                        // that many cycles without a step (hidden mesh, stalled frame) the worker's bytes return.
                        const bool khsu_owned = khsu_gp.vb && khsu_gp.step_cycle != ~0ull &&
                                                khsu_gp.step_cycle != g_topo_cycles && g_topo_cycles > khsu_gp.step_cycle &&
                                                g_topo_cycles - khsu_gp.step_cycle <= KH_SKIN_STEP_OWN;
                        const bool khsu_stepped = khsu_gp.vb && (khsu_gp.step_cycle == g_topo_cycles ||
                                                  khsu_owned);
                        // KH_SKIN_GPU: the worker's bytes are written only when its result carries them
                        // (KhSkinInst::out_ok - a job handed over while the step owned the buffer skinned none). A
                        // buffer that needs them and has none asks for a full job and takes nothing this time: the
                        // record waits with the bytes, as it does for a Map that fails.
                        bool khsu_wrote = false;
                        // KH_SKIN_HIDDEN_COL: a hidden object's result without vertices, for a buffer that already
                        // holds a pose - the record is taken, the bytes stand, nothing is asked for.
                        const bool khsu_hid = khsu_gp.vb && khsu_gp.valid && !khsu_stepped && !khsu_in.out_ok[khsu_f] &&
                                              khsu_in.hidden.load(std::memory_order_relaxed);
                        if (khsu_gp.vb && !khsu_stepped && !khsu_hid) {
                            if (!khsu_in.out_ok[khsu_f]) {
                                khsu_in.need_full.store(true, std::memory_order_relaxed);
                            } else if (khsu_gp.so) {
                                // The whole array (KH_SKIN_POOL: its slice alone).
                                kh_skin_so_write(khsu_ctx, kh_skin_gpu_ref(khsu_gp), khsu_bytes,
                                                 khsu_gp.chunk != nullptr, khsu_src.data());
                                khsu_wrote = true;
                            } else {
                                D3D11_MAPPED_SUBRESOURCE khsu_m = {};
                                if (SUCCEEDED(khsu_ctx->Map(khsu_gp.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khsu_m)) && khsu_m.pData) {
                                    memcpy(khsu_m.pData, khsu_src.data(), khsu_bytes);   // The whole array: DISCARD leaves garbage.
                                    khsu_ctx->Unmap(khsu_gp.vb, 0);
                                    khsu_wrote = true;
                                }
                            }
                            if (khsu_wrote) {
                                khsu_gp.by_step = false;   // The worker's bytes again: the put may follow.
                                kh_uvs_src_wrote(khsu_in.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again.
                            }
                        }
                        if (khsu_gp.vb && (khsu_stepped || khsu_wrote || khsu_hid)) {
                            // COLLIDER: a pose this record has not held before takes this frame's stamp; the same
                            // pose uploaded again after a device reset keeps its own.
                            if (khsu_gp.gen != khsu_gf) khsu_gp.serial = ++g_skin_col_stamp;
                            khsu_gp.gen = khsu_gf;   // KH_SKIN_GEN_OF.
                            khsu_gp.key = khsu_in.key_of[khsu_f];
                            memcpy(khsu_gp.box_ctr, khsu_in.box_ctr[khsu_f], sizeof(khsu_gp.box_ctr));
                            memcpy(khsu_gp.box_size, khsu_in.box_size[khsu_f], sizeof(khsu_gp.box_size));
                            khsu_gp.bvh = khsu_in.bvh_of[khsu_f];
                            khsu_gp.valid = true;
                        }
                    }
                }
                if (khsu_gp.valid && khsu_gp.vb) {
                    khsu_p.put = !khsu_gp.by_step;   // KH_SKEL_DRAW: the step's box stands with its bytes (KhSkinGpu::by_step).
                    const uint32_t khsu_s = khsu_in.slot;
                    if (g_cloth_vb_slot.size() <= khsu_s) g_cloth_vb_slot.resize(khsu_s + 1u, nullptr);
                    if (g_cloth_gen_slot.size() <= khsu_s) g_cloth_gen_slot.resize(khsu_s + 1u, 0u);
                    if (g_cloth_mesh_slot.size() <= khsu_s) g_cloth_mesh_slot.resize(khsu_s + 1u, -1);
                    if (g_cloth_sim_slot.size() <= khsu_s) g_cloth_sim_slot.resize(khsu_s + 1u, 0u);
                    if (g_cloth_voff_slot.size() <= khsu_s) g_cloth_voff_slot.resize(khsu_s + 1u, 0u);   // POOL.
                    if (g_cloth_pool_slot.size() <= khsu_s) g_cloth_pool_slot.resize(khsu_s + 1u, nullptr);
                    g_cloth_sim_slot[khsu_s] = 0u;   // KH_LOD_SIM_SLOT: the skin's buffer, every level posed.
                    g_cloth_vb_slot[khsu_s] = khsu_gp.vb;
                    g_cloth_voff_slot[khsu_s] = khsu_gp.off;      // KH_SKIN_POOL: its slice, in this chunk.
                    g_cloth_pool_slot[khsu_s] = khsu_gp.chunk;
                    if (khsu_gp.chunk) {
                        khsu_p.vbase = khsu_gp.off / static_cast<UINT>(sizeof(MeshVertex));
                        ++khsu_pooled;
                    }
                    g_cloth_gen_slot[khsu_s] = khsu_gp.by_step ? khsu_gp.step_key : khsu_gp.key;   // The key follows the bytes.
                    g_cloth_mesh_slot[khsu_s] = khsu_in.mesh;
                    khsu_box(khsu_p, khsu_gp.box_ctr, khsu_gp.box_size);
                    ++khsu_meshes;
                    if (khsu_gp.bvh) {   // COLLIDER: this box's shape, for the gather.
                        KhSkinCol& khsu_sc = khsu_col[khsu_in.seq];
                        khsu_sc.mesh = khsu_in.mesh;
                        memcpy(khsu_sc.ctr, khsu_p.ctr, sizeof(khsu_sc.ctr));
                        memcpy(khsu_sc.size, khsu_p.size, sizeof(khsu_sc.size));
                        khsu_sc.bvh = khsu_gp.bvh;
                        khsu_sc.serial = khsu_gp.serial;
                    }
                }
            }
            khsu_put.push_back(khsu_p);
        }
        for (auto khsu_rg = g_skin_rest_gpu.begin(); khsu_rg != g_skin_rest_gpu.end();) {   // KH_SKIN_GPU: a rest stream no binding draws from.
            if (!khsu_rg->second.seen) { KH_SAFE_RELEASE(khsu_rg->second.vb); khsu_rg = g_skin_rest_gpu.erase(khsu_rg); }
            else ++khsu_rg;
        }
    }
    {
        // The box of what the slot draws. The position moves with the centre so that it stays root + skel_ctr *
        // rot_m for a late draw in this same flush, before kh_attach_step next recomputes it.
        std::lock_guard<std::mutex> khsu_g(g_draw_list_mutex);
        for (const KhSkinPut& khsu_p : khsu_put) {
            const auto khsu_oit = g_draw_list.find(khsu_p.h);
            if (khsu_oit == g_draw_list.end()) continue;
            RenderObject& khsu_o = khsu_oit->second;
            if (!khsu_o.skel || khsu_o.seq != khsu_p.seq || khsu_o.mesh != khsu_p.mesh) continue;
            // KH_SKIN_POOL: the base of the slice the tables now name, into the record at the next sync.
            if (khsu_o.skin_vbase != khsu_p.vbase) {
                khsu_o.skin_vbase = khsu_p.vbase;
                kh_scene_mark(khsu_o.slot);
            }
            if (!khsu_p.put) continue;
            if (memcmp(khsu_o.skel_ctr, khsu_p.ctr, sizeof(khsu_o.skel_ctr)) == 0 &&
                memcmp(khsu_o.size, khsu_p.size, sizeof(khsu_p.size)) == 0) continue;
            float khsu_d[3];
            for (int k = 0; k < 3; ++k) khsu_d[k] = khsu_p.ctr[k] - khsu_o.skel_ctr[k];
            float khsu_w[3] = { khsu_d[0], khsu_d[1], khsu_d[2] };
            if (khsu_o.rotated) {
                for (int k = 0; k < 3; ++k) {
                    khsu_w[k] = khsu_d[0] * khsu_o.rot_m[k] + khsu_d[1] * khsu_o.rot_m[3 + k] + khsu_d[2] * khsu_o.rot_m[6 + k];
                }
            }
            double khsu_x[3];   // KH_POS_RES: the move applied to the exact centre.
            kh_pos_exact(khsu_o, khsu_x);
            khsu_x[0] += static_cast<double>(khsu_w[0]);
            khsu_x[1] += static_cast<double>(khsu_w[2]);
            khsu_x[2] += static_cast<double>(khsu_w[1]);
            kh_pos_set_exact(khsu_o, khsu_x);
            memcpy(khsu_o.skel_ctr, khsu_p.ctr, sizeof(khsu_o.skel_ctr));
            memcpy(khsu_o.size, khsu_p.size, sizeof(khsu_p.size));
            kh_scene_mark(khsu_o.slot);
        }
        kh_skin_col_publish(khsu_col);   // With the boxes, in their scope.
    }
    g_stats.skin_meshes = khsu_meshes;
    g_stats.skin_pooled = khsu_pooled;   // KH_SKIN_POOL.
    g_skin_pool_any = khsu_pooled != 0u;   // This flush's tables (installed above, under the tables' rules).
    g_stats.skin_bones = khsu_bones;
    g_stats.skin_bones_driven = khsu_driven;
    g_stats.skin_proxies = khsu_proxies;
    g_stats.skin_skipped = g_skin_skipped.load(std::memory_order_relaxed);
    g_stats.skin_pool_full = g_skin_pool_full.load(std::memory_order_relaxed);   // skinPoolFull.
}


// Device teardown (kh_engine_reset_release: under the graphics lock, on the render thread itself, or its best effort
// holding neither). The CPU instances keep their pose and last result; the next upload (the flush) re-creates and
// re-uploads. The slot tables are emptied by
// kh_cloth_release_all beside it.
inline void kh_skin_release_all() {
    std::lock_guard<std::mutex> khsr_g(g_skin_mu);
    for (auto& khsr_kv : g_skin_gpu) {
        kh_skin_gpu_vb_drop(khsr_kv.second);   // KH_SKIN_POOL: a slice hands back; the chunk goes with its last.
        khsr_kv.second.bytes = 0;
        khsr_kv.second.valid = false;
        khsr_kv.second.step_cycle = ~0ull;
        khsr_kv.second.by_step = false;
        khsr_kv.second.so = false;   // KH_SKIN_GPU: decided again with the next buffer.
        khsr_kv.second.so_refused = ~0ull;   // KH_SKIN_GPU_MOVE: a new device is asked at once.
    }
    for (auto& khsr_rg : g_skin_rest_gpu) KH_SAFE_RELEASE(khsr_rg.second.vb);   // KH_SKIN_GPU: the rest streams.
    g_skin_rest_gpu.clear();
    for (auto& khsr_p : g_skin_pool) {   // KH_SKIN_POOL: none is left by the drops above; a stray one goes too.
        for (std::unique_ptr<KhSkinPoolChunk>& khsr_c : khsr_p.second.chunks) {
            KH_SAFE_RELEASE(khsr_c->srv);
            KH_SAFE_RELEASE(khsr_c->vb);
        }
    }
    g_skin_pool.clear();
    g_skin_pool_failed = false;   // A new device is asked again.
    g_skin_pool_made = false;     // KH_SKIN_POOL_MADE: likewise.
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // KH_SKIN_SO_RETRY: likewise (g_res is released).
    g_skin_so_retry_at = ~0ull;
    g_skin_so_big_failed = false;   // KH_SKIN_SO_LAZY: the large class's latch, likewise.
    g_skin_so_big_retry_at = ~0ull;
    g_skin_pool_any = false;      // kh_cloth_release_all emptied the tables beside this.
}

// Mission teardown, from the game thread under the park, beside kh_cloth_drop_all: the buffers and the instances
// both go. Its one caller (kh_session_teardown) has joined the workers first (kh_cloth_workers_stop), so no job
// runs here.
inline void kh_skin_drop_all() {
    kh_skin_release_all();
    {
        std::lock_guard<std::mutex> khsd_g(g_skin_mu);
        g_skin_gpu.clear();
    }
    {   // KH_SKIN_MAP.
        std::lock_guard<std::mutex> khsd_s(g_skin_mu);
        g_skin.clear();
    }
    g_skin_rest.clear();
    // Locked rather than relying on the callers holding a park.
    { std::lock_guard<std::mutex> khsd_c(g_skin_col_mu); g_skin_col.reset(); }
    g_skin_col_snap.reset();   // Game thread, as the one caller (kh_session_teardown, on both mission edges) is.
}
// The batch's per-binding rule: the buffer and stream a job is skinned with on the GPU, or none (the pool takes it).
// Under g_skin_mu. KH_SKIN_CULL decides only for a job this gives a target, so a culled job is one the GPU would skin.
inline KhSbTarget kh_skin_so_target(const KhSkinDrawJob& j) {
    const KhSbTarget khst_none = { KhVbRef{}, nullptr, 0u };
    if (!KH_SKIN_GPU_ON || !g_res.skin_so_vs || !g_res.skin_so_gs || !g_res.skin_so_layout || !g_res.skin_so_cb) {
        return khst_none;
    }
    const KhSkinInst* const in = j.a->skin.get();
    if (!in || !in->rest_draw) return khst_none;
    const KhSkinRest& r = *in->rest_draw;
    const size_t nb = j.a->draw_a.size() / 12u;
    if (!r.sup.ok || nb == 0u || !kh_skin_so_class_ok(r) || r.sup.root.size() != nb) {   // KH_SKIN_SO_BIG / _LAZY.
        return khst_none;
    }
    const auto gp = g_skin_gpu.find(*j.h);
    if (gp == g_skin_gpu.end() || !gp->second.vb || !gp->second.so ||
        gp->second.bytes != static_cast<UINT>(r.verts.size() * sizeof(MeshVertex))) return khst_none;
    const auto rg = g_skin_rest_gpu.find(&r);
    if (rg == g_skin_rest_gpu.end() || !rg->second.vb || rg->second.nv != static_cast<UINT>(r.verts.size()) ||
        rg->second.nv == 0u) return khst_none;
    // KH_SKIN_LVL0 (retired, KhSkinRest::drawn): every binding is skinned whole, a locked one too.
    return KhSbTarget{ kh_skin_gpu_ref(gp->second), rg->second.vb, rg->second.nv };
}
inline void kh_skin_so_batch(ID3D11DeviceContext* khsb_ctx, std::vector<KhSkinDrawJob>& khsb_jobs) {
    if (!KH_SKIN_GPU_ON || !khsb_ctx || khsb_jobs.empty()) return;
    if (!g_res.skin_so_vs || !g_res.skin_so_gs || !g_res.skin_so_layout || !g_res.skin_so_cb) return;
    static std::vector<KhSbTarget> khsb_tg;   // Scratch (the step's mutex).
    khsb_tg.assign(khsb_jobs.size(), KhSbTarget{ KhVbRef{}, nullptr, 0u });
    bool khsb_any = false;
    {   // g_draw_list_mutex -> g_skin_mu, as kh_skin_draw_finish. The map cannot change under the render thread.
        std::lock_guard<std::mutex> khsb_g(g_skin_mu);
        for (size_t i = 0; i < khsb_jobs.size(); ++i) {
            if (khsb_jobs[i].culled) continue;   // KH_SKIN_CULL: nothing to draw.
            khsb_tg[i] = kh_skin_so_target(khsb_jobs[i]);
            if (khsb_tg[i].vb) khsb_any = true;
        }
    }
    if (!khsb_any) return;
    {
        // The flag first and last: up before the capture, down after the restore (its scope unwinds after the
        // backup's), as every pass of ours orders them - the restore's own Set calls are ours too.
        KhInjectionFlagScope khsb_flag;
        g_ro.in_injection = true;
        StateBackup khsb_sb;
        khsb_sb.capture(khsb_ctx);
        ID3D11Buffer* khsb_so_old = nullptr;
        khsb_ctx->SOGetTargets(1, &khsb_so_old);
        khsb_ctx->IASetInputLayout(g_res.skin_so_layout);
        khsb_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        khsb_ctx->PSSetShader(nullptr, nullptr, 0);
        // A point list is not a valid input to a hull shader: were the engine ever to leave tessellation stages bound
        // at a step site, the draws below would be dropped and the pose never written. Cleared for the pass, as
        // kh_uvs_step clears them; the restore hands them back.
        khsb_ctx->HSSetShader(nullptr, nullptr, 0);
        khsb_ctx->DSSetShader(nullptr, nullptr, 0);
        // KH_SKIN_SO_BIG: each job's class by its palette's size (KH_SKIN_SO_PAL) - its VSSkinSo, stream-output object
        // and CBSkin, bound when the class changes (-1: none yet).
        int khsb_cls = -1;
        for (size_t i = 0; i < khsb_jobs.size(); ++i) {
            const KhSbTarget& tg = khsb_tg[i];
            if (!tg.vb) continue;
            KhSkinDrawJob& j = khsb_jobs[i];
            const std::vector<uint32_t>& khsb_pal = j.a->skin->rest_draw->sup.pal;   // The target's snapshot (checked).
            const int khsb_c = khsb_pal.size() > KH_SKIN_SO_BONES_S ? 1 : 0;
            ID3D11Buffer* const khsb_cb = khsb_c ? g_res.skin_so_cb_l : g_res.skin_so_cb;
            if (khsb_c != khsb_cls) {
                khsb_cls = khsb_c;
                khsb_ctx->VSSetShader(khsb_c ? g_res.skin_so_vs_l : g_res.skin_so_vs, nullptr, 0);
                khsb_ctx->GSSetShader(khsb_c ? g_res.skin_so_gs_l : g_res.skin_so_gs, nullptr, 0);
                khsb_ctx->VSSetConstantBuffers(0, 1, &khsb_cb);
            }
            float bc[3], bs[3];
            if (j.bnd) {   // KH_SKIN_CULL: the pass took the same function of the same inputs.
                memcpy(bc, j.bc, sizeof(bc));
                memcpy(bs, j.bs, sizeof(bs));
            } else {
                kh_skin_so_bound(*j.a->skin->rest_draw, j.a->draw_a.data(), bc, bs);
            }
            D3D11_MAPPED_SUBRESOURCE khsb_m = {};
            if (FAILED(khsb_ctx->Map(khsb_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khsb_m)) || !khsb_m.pData) continue;
            kh_skin_so_cb_fill(static_cast<float*>(khsb_m.pData), bc, bs, j.a->draw_a.data(), khsb_pal.data(),
                               khsb_pal.size());
            khsb_ctx->Unmap(khsb_cb, 0);
            const UINT khsb_stride = static_cast<UINT>(sizeof(KhSkinSoVert)), khsb_zero = 0u;
            khsb_ctx->IASetVertexBuffers(0, 1, &tg.rest, &khsb_stride, &khsb_zero);
            khsb_ctx->SOSetTargets(1, &tg.vb.vb, &tg.vb.off);   // KH_SKIN_POOL: at the slice's offset.
            if (g_orig_draw) g_orig_draw(khsb_ctx, tg.nv, 0u); else khsb_ctx->Draw(tg.nv, 0u);
            memcpy(j.bc, bc, sizeof(bc));
            memcpy(j.bs, bs, sizeof(bs));
            j.so_vb = tg.vb;
        }
        // The target off before anything binds the buffer as vertices.
        const UINT khsb_append = static_cast<UINT>(-1);
        khsb_ctx->SOSetTargets(1, &khsb_so_old, &khsb_append);
        if (khsb_so_old) khsb_so_old->Release();
        khsb_sb.restore(khsb_ctx);   // Under the flag.
    }   // The flag handed back last.
}
}