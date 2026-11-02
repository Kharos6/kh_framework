#pragma once

// The feature half (rendering_integration_features.hpp): KH_CLOTH, the simulation core: the physics primitives, the
// collider BVH, collision, tethers, self-collision, the worker fork, the affector evaluation and the cloth step. Never
// include it on its own.
namespace RenderIntegration {

// Hold one particle 'thick' metres above the ground patch, along the normal of
// the triangle it stands over (the exact distance to that plane). A push out of
// a valley can land the particle over the neighbouring triangle and still under
// it, so the projection repeats over wherever the last push landed, up to
// KH_CLOTH_GROUND_PASSES; a pass that finds the particle clear ends it. True,
// with the last push's direction, when it moved the particle. Outside the patch
// there is no ground (the patch covers the cloth's reach).
inline bool kh_cloth_ground_plane(const KhClothGround& g, float thick, float* p, float* n_out);
inline uint32_t kh_cloth_fork_workers();   // Both pools' thread count (defined with the pool).
inline void kh_cloth_fork_wake();          // Wakes the idle workers (defined with the pool).
inline void kh_cloth_fork_wake_try();      // The same without ever blocking (KH_SKIN_FORK; defined with the pool).

// KH_CLOTH - the simulation core: particles, distance constraints, the
// collider BVH and the XPBD solve. Pure arithmetic: every function here runs
// standalone on its inputs, and nothing in this block may touch D3D or SQF. Its
// only globals are g_physics_pose_serial (an atomic), the KH_PROF scopes'
// counters (inert while the stats are off) and, through kh_cloth_fork_run (the
// self-collision gather), the fork's state (KH_CLOTH_FORK).

// Standard gravity (m/s^2). ENGINE AXES ARE (EAST, UP, NORTH): the up axis is
// index 1, not index 2 (applied to index 2 it pulls every cloth north instead
// of down). kh_cloth_engine_pos is the authority: it puts zASL into index 1.
static constexpr float    KH_CLOTH_G          = 9.80665f;
static constexpr int      KH_CLOTH_UP         = 1;
static constexpr uint32_t KH_CLOTH_MAX_SUBSTEP= 8u;         // The 'substeps' dial's top: substeps per 1/60 s.
// A longer frame is simulated as this long: the stall clamp (kh_cloth_step),
// the one bound on how much time one step may simulate.
static constexpr float    KH_CLOTH_DT_MAX     = 0.100f;
// The reference substep ('substeps' 2, the default, runs exactly this) and the
// interval the per-step dials (the cloth's damping; a chain's damping,
// linearDamping and angularDamping) are stated per, so changing 'substeps'
// changes the solve, not what they mean. The drag dials are forces
// (KH_CLOTH_DRAG), stated per second.
static constexpr float    KH_CLOTH_H          = 1.0f / 120.0f;
static constexpr uint32_t KH_PHYSICS_BVH_LEAF   = 4u;         // Triangles per leaf.
// Gauss-Seidel passes over the per-edge stretch limit. The worst-edge ratio
// flattens past four passes; an exact cap would need many more on a long
// chain and buys nothing visible.
static constexpr int KH_CLOTH_LIMIT_PASSES = 4;
// Attachment compliance at a paint weight of one half - the constant that sets
// where the pin/sim gradient crosses halfway. See kh_cloth_build.
static constexpr float KH_CLOTH_ATT_SOFT = 0.30f;
// KH_CLOTH_DRAG - the drag of the air (and of the water) on cloth: one linear
// force per point against its velocity RELATIVE TO THE MEDIUM,
// F = -c (S I + (1 - S) M)(v - u). c is the cloth's 'drag' (or 'waterDrag') in
// kg/s, M the point's facing (the area-weighted mean of n n^T over its faces),
// S = KH_CLOTH_SKIN the share left to motion along the surface. A flat plate's
// law: a face moving through the medium is pushed back hard (pressure), one
// sliding along its own plane barely (skin friction), so cloth edge-on to a
// wind, or lying flat in one, is barely moved. The acceleration is F / mass, so a heavier cloth answers the same air
// less.
static constexpr float KH_CLOTH_SKIN = 0.02f;
// SEA LEVEL IS ZERO and the up axis is metres ASL, so a particle is underwater
// exactly when its altitude is negative (Arma has no caves: no negative
// altitude is dry). The solver runs in a frame of its own (KhClothState::sea
// says where sea level lies in it - the floating origin in kh_cloth_sync), so
// the test is against that. The transition is ramped over this depth, not
// switched at the surface: a hard switch makes a particle at the waterline
// alternate between full gravity and full buoyancy every frame, a visible buzz.
static constexpr float KH_CLOTH_WATER_BAND = 0.10f;

// KH_COL_CLEAR: a pose identity. Any thread (a worker builds and refits); an
// identity, so never reset (kh_session_globals_reset's keep list).
static std::atomic<uint32_t> g_physics_pose_serial{ 0 };
inline uint32_t kh_physics_pose_seq_next() {
    uint32_t khps_s = g_physics_pose_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    if (khps_s == 0u) khps_s = g_physics_pose_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    return khps_s;
}
inline void kh_cloth_v3_sub(const float* a, const float* b, float* o) {
    o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2];
}
inline float kh_cloth_v3_dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline float kh_cloth_v3_len(const float* a) { return sqrtf(kh_cloth_v3_dot(a, a)); }
inline bool kh_cloth_finite3(const float* a) {
    return a[0] == a[0] && a[1] == a[1] && a[2] == a[2] &&
           fabsf(a[0]) < 1.0e12f && fabsf(a[1]) < 1.0e12f && fabsf(a[2]) < 1.0e12f;
}

// KH_COL_CONVEX: one triangle's plane from its corners: unit outward normal
// (the winding's, as the BVH's inside test reads it) and offset. False on a
// degenerate triangle.
inline bool kh_physics_tri_plane(const float* khtp_v, float* khtp_n, float& khtp_d) {
    float e1[3], e2[3];
    kh_cloth_v3_sub(khtp_v + 3, khtp_v, e1);
    kh_cloth_v3_sub(khtp_v + 6, khtp_v, e2);
    khtp_n[0] = e1[1] * e2[2] - e1[2] * e2[1];
    khtp_n[1] = e1[2] * e2[0] - e1[0] * e2[2];
    khtp_n[2] = e1[0] * e2[1] - e1[1] * e2[0];
    const float khtp_l = kh_cloth_v3_len(khtp_n);
    if (!(khtp_l > 1.0e-30f)) return false;
    for (int k = 0; k < 3; ++k) khtp_n[k] /= khtp_l;
    khtp_d = kh_cloth_v3_dot(khtp_n, khtp_v);
    return true;
}
// KH_COL_CONVEX: the pieces and their planes from a freshly built BVH (vp in
// stream order) and the build's position-welded corner ids (khvx_cid, 3 per
// kept triangle). A piece is a connected component of the welded corners; its
// planes are its triangles' planes with coplanar ones merged (the first
// triangle stands for the plane): a capsule of a few hundred triangles is a
// hundred-odd planes, a box six. kh_physics_convex_refit then fills the sphere
// and the pose-dependent lanes.
// Convexity is the script's promise (physicsCollider's 'convex') and is not checked: a concave piece collides as the
// intersection of its planes' insides - a particle is pushed out only when behind every plane (kh_physics_convex_one:
// the largest plane distance) - which is less than the piece, so cloth can pass into its concave parts.
inline void kh_physics_convex_build(KhPhysicsBvh& khvx_o, const uint32_t* khvx_cid, uint32_t khvx_nt) {
    khvx_o.cvx_ptri.clear(); khvx_o.cvx_pstart.clear(); khvx_o.cvx_tri.clear();
    khvx_o.cvx_pl.clear(); khvx_o.cvx_pl0.clear(); khvx_o.cvx_pv.clear(); khvx_o.cvx_sph.clear();
    khvx_o.cvx_only = false;
    if (khvx_nt == 0u || !khvx_cid) return;
    uint32_t khvx_nid = 0u;
    for (uint32_t i = 0; i < khvx_nt * 3u; ++i) if (khvx_cid[i] + 1u > khvx_nid) khvx_nid = khvx_cid[i] + 1u;
    std::vector<uint32_t> khvx_par(khvx_nid);
    for (uint32_t i = 0; i < khvx_nid; ++i) khvx_par[i] = i;
    auto khvx_find = [&](uint32_t x) { while (khvx_par[x] != x) { khvx_par[x] = khvx_par[khvx_par[x]]; x = khvx_par[x]; } return x; };
    for (uint32_t t = 0; t < khvx_nt; ++t) {
        const uint32_t a = khvx_find(khvx_cid[t * 3u]), b = khvx_find(khvx_cid[t * 3u + 1u]), c = khvx_find(khvx_cid[t * 3u + 2u]);
        if (b != a) khvx_par[b] = a;
        const uint32_t a2 = khvx_find(a);
        if (khvx_find(c) != a2) khvx_par[khvx_find(c)] = a2;
    }
    // Pieces in first-seen order; every triangle filed under its piece.
    std::vector<uint32_t> khvx_root_piece(khvx_nid, 0xFFFFFFFFu);
    khvx_o.cvx_ptri.resize(khvx_nt);
    uint32_t khvx_np = 0u;
    for (uint32_t t = 0; t < khvx_nt; ++t) {
        const uint32_t r = khvx_find(khvx_cid[t * 3u]);
        if (khvx_root_piece[r] == 0xFFFFFFFFu) khvx_root_piece[r] = khvx_np++;
        khvx_o.cvx_ptri[t] = khvx_root_piece[r];
    }
    std::vector<uint32_t> khvx_head(khvx_np + 1u, 0u);
    for (uint32_t t = 0; t < khvx_nt; ++t) ++khvx_head[khvx_o.cvx_ptri[t] + 1u];
    for (uint32_t p = 0; p < khvx_np; ++p) khvx_head[p + 1u] += khvx_head[p];
    std::vector<uint32_t> khvx_by(khvx_nt);
    {
        std::vector<uint32_t> khvx_fill(khvx_head.begin(), khvx_head.end() - 1);
        for (uint32_t t = 0; t < khvx_nt; ++t) khvx_by[khvx_fill[khvx_o.cvx_ptri[t]]++] = t;
    }
    // KH_COL_CONVEX_CELLS: the merge runs on every collider build, convex-marked or not. The kept planes are filed by
    // normal on a 0.01 grid and a triangle tests only the 27 cells around its own (a scan of every kept plane is
    // about n^2 / 2 tests on a smooth or non-convex piece, where almost nothing merges: a game-thread stall of
    // seconds for a 50k-triangle collider). The test's dot > 0.99999 between unit normals puts them under 0.0047
    // apart on every axis, so any plane it can match is in those cells, and the planes kept, and their order, are
    // exactly the full scan's. A normal that is not a number matches nothing either way (every comparison with it is
    // false) and is kept unfiled.
    std::unordered_map<uint64_t, std::vector<uint32_t>> khvx_cells;
    auto khvx_cell = [](const float* khvc_n, int* khvc_i) -> bool {
        for (int k = 0; k < 3; ++k) {
            if (!(khvc_n[k] >= -2.0f && khvc_n[k] <= 2.0f)) return false;   // Not a number (a unit normal is inside).
            khvc_i[k] = static_cast<int>(floorf(khvc_n[k] * 100.0f)) + 512;   // 312 .. 712: 10 bits each.
        }
        return true;
    };
    auto khvx_key = [](int x, int y, int z) -> uint64_t {
        return (static_cast<uint64_t>(x) << 20) | (static_cast<uint64_t>(y) << 10) | static_cast<uint64_t>(z);
    };
    khvx_o.cvx_pstart.push_back(0u);
    for (uint32_t p = 0; p < khvx_np; ++p) {
        khvx_cells.clear();
        for (uint32_t i = khvx_head[p]; i < khvx_head[p + 1u]; ++i) {
            const uint32_t t = khvx_by[i];
            float n[3], d;
            if (!kh_physics_tri_plane(&khvx_o.vp[static_cast<size_t>(t) * 9u], n, d)) continue;
            bool khvx_dup = false;   // Coplanar with a plane this piece already has.
            int khvx_c[3];
            const bool khvx_filed = khvx_cell(n, khvx_c);
            if (khvx_filed) {
                for (int dx = -1; dx <= 1 && !khvx_dup; ++dx)
                for (int dy = -1; dy <= 1 && !khvx_dup; ++dy)
                for (int dz = -1; dz <= 1 && !khvx_dup; ++dz) {
                    const auto khvx_it = khvx_cells.find(khvx_key(khvx_c[0] + dx, khvx_c[1] + dy, khvx_c[2] + dz));
                    if (khvx_it == khvx_cells.end()) continue;
                    for (uint32_t q : khvx_it->second) {
                        const float* m = &khvx_o.cvx_pl[static_cast<size_t>(q) * 4u];
                        if (kh_cloth_v3_dot(n, m) > 0.99999f && fabsf(d - m[3]) <= 1.0e-4f * (1.0f + fabsf(d))) {
                            khvx_dup = true;
                            break;
                        }
                    }
                }
            }
            if (khvx_dup) continue;
            if (khvx_filed) {
                khvx_cells[khvx_key(khvx_c[0], khvx_c[1], khvx_c[2])].push_back(
                    static_cast<uint32_t>(khvx_o.cvx_tri.size()));
            }
            khvx_o.cvx_tri.push_back(t);
            khvx_o.cvx_pl.push_back(n[0]); khvx_o.cvx_pl.push_back(n[1]); khvx_o.cvx_pl.push_back(n[2]); khvx_o.cvx_pl.push_back(d);
        }
        khvx_o.cvx_pstart.push_back(static_cast<uint32_t>(khvx_o.cvx_tri.size()));
    }
    khvx_o.cvx_sph.assign(static_cast<size_t>(khvx_np) * 4u, 0.0f);
}
// KH_COL_CONVEX: the planes and spheres for the corners now in vp (and, with
// vd, the previous pose's planes and each plane's mean corner motion). After
// every refit and build; nothing else reads a pose's convex lanes.
inline void kh_physics_convex_refit(KhPhysicsBvh& khvr_o) {
    const size_t khvr_np = khvr_o.cvx_pstart.empty() ? 0u : khvr_o.cvx_pstart.size() - 1u;
    const size_t khvr_nl = khvr_o.cvx_tri.size();
    if (khvr_np == 0u || khvr_nl == 0u || khvr_o.cvx_ptri.size() != khvr_o.tri.size()) {
        khvr_o.cvx_pl0.clear(); khvr_o.cvx_pv.clear();
        return;
    }
    const bool khvr_sw = !khvr_o.vd.empty() && khvr_o.vd.size() == khvr_o.vp.size();
    khvr_o.cvx_pl.resize(khvr_nl * 4u);
    if (khvr_sw) { khvr_o.cvx_pl0.resize(khvr_nl * 4u); khvr_o.cvx_pv.resize(khvr_nl * 3u); }
    else { khvr_o.cvx_pl0.clear(); khvr_o.cvx_pv.clear(); }
    for (size_t q = 0; q < khvr_nl; ++q) {
        const size_t khvr_ti = static_cast<size_t>(khvr_o.cvx_tri[q]) * 9u;
        float* pl = &khvr_o.cvx_pl[q * 4u];
        float n[3], d;
        if (kh_physics_tri_plane(&khvr_o.vp[khvr_ti], n, d)) { pl[0] = n[0]; pl[1] = n[1]; pl[2] = n[2]; pl[3] = d; }
        // A triangle gone flat keeps its last plane (the lane holds the template's or the last pose's).
        if (khvr_sw) {
            float v0[9];
            for (int j = 0; j < 9; ++j) v0[j] = khvr_o.vp[khvr_ti + j] - khvr_o.vd[khvr_ti + j];
            float* pl0 = &khvr_o.cvx_pl0[q * 4u];
            if (kh_physics_tri_plane(v0, n, d)) { pl0[0] = n[0]; pl0[1] = n[1]; pl0[2] = n[2]; pl0[3] = d; }
            else memcpy(pl0, pl, sizeof(float) * 4u);
            float* pv = &khvr_o.cvx_pv[q * 3u];
            for (int k = 0; k < 3; ++k) {
                pv[k] = (khvr_o.vd[khvr_ti + k] + khvr_o.vd[khvr_ti + 3 + k] + khvr_o.vd[khvr_ti + 6 + k]) * (1.0f / 3.0f);
            }
        }
    }
    // The spheres: the box of every corner of the piece at both poses, then
    // the farthest corner from its centre.
    std::vector<float> khvr_mn(khvr_np * 3u, 1.0e30f), khvr_mx(khvr_np * 3u, -1.0e30f);
    for (size_t t = 0; t < khvr_o.tri.size(); ++t) {
        const uint32_t p = khvr_o.cvx_ptri[t];
        if (p >= khvr_np) continue;
        const float* v = &khvr_o.vp[t * 9u];
        for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
            const float a = v[c * 3 + k];
            const float bq = khvr_sw ? a - khvr_o.vd[t * 9u + c * 3 + k] : a;
            const float lo = a < bq ? a : bq, hi = a < bq ? bq : a;
            if (lo < khvr_mn[p * 3u + k]) khvr_mn[p * 3u + k] = lo;
            if (hi > khvr_mx[p * 3u + k]) khvr_mx[p * 3u + k] = hi;
        }
    }
    khvr_o.cvx_sph.assign(khvr_np * 4u, 0.0f);
    for (size_t p = 0; p < khvr_np; ++p) {
        float* sp = &khvr_o.cvx_sph[p * 4u];
        for (int k = 0; k < 3; ++k) sp[k] = khvr_mn[p * 3u + k] <= khvr_mx[p * 3u + k] ? 0.5f * (khvr_mn[p * 3u + k] + khvr_mx[p * 3u + k]) : 0.0f;
    }
    for (size_t t = 0; t < khvr_o.tri.size(); ++t) {
        const uint32_t p = khvr_o.cvx_ptri[t];
        if (p >= khvr_np) continue;
        float* sp = &khvr_o.cvx_sph[p * 4u];
        const float* v = &khvr_o.vp[t * 9u];
        for (int c = 0; c < 3; ++c) {
            float r2 = 0.0f, r2p = 0.0f;
            for (int k = 0; k < 3; ++k) {
                const float a = v[c * 3 + k] - sp[k];
                r2 += a * a;
                if (khvr_sw) { const float bq = a - khvr_o.vd[t * 9u + c * 3 + k]; r2p += bq * bq; }
            }
            if (r2 > sp[3]) sp[3] = r2;
            if (r2p > sp[3]) sp[3] = r2p;
        }
    }
    for (size_t p = 0; p < khvr_np; ++p) khvr_o.cvx_sph[p * 4u + 3u] = sqrtf(khvr_o.cvx_sph[p * 4u + 3u]);
}

// Closest point on triangle abc to p (Ericson 5.1.5), and the feature it lies
// on: 0 the face, 1..3 corner a / b / c, 4..6 edge ab / bc / ca. The feature
// is what kh_physics_collide_one's inside test needs.
inline void kh_cloth_closest_tri(const float* p, const float* a, const float* b, const float* c, float* o,
                                 int& khct_r) {
    float ab[3], ac[3], ap[3];
    kh_cloth_v3_sub(b, a, ab); kh_cloth_v3_sub(c, a, ac); kh_cloth_v3_sub(p, a, ap);
    const float d1 = kh_cloth_v3_dot(ab, ap), d2 = kh_cloth_v3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) { o[0] = a[0]; o[1] = a[1]; o[2] = a[2]; khct_r = 1; return; }
    float bp[3]; kh_cloth_v3_sub(p, b, bp);
    const float d3 = kh_cloth_v3_dot(ab, bp), d4 = kh_cloth_v3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) { o[0] = b[0]; o[1] = b[1]; o[2] = b[2]; khct_r = 2; return; }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        for (int k = 0; k < 3; ++k) o[k] = a[k] + ab[k] * v;
        khct_r = 4;
        return;
    }
    float cp[3]; kh_cloth_v3_sub(p, c, cp);
    const float d5 = kh_cloth_v3_dot(ab, cp), d6 = kh_cloth_v3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) { o[0] = c[0]; o[1] = c[1]; o[2] = c[2]; khct_r = 3; return; }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        for (int k = 0; k < 3; ++k) o[k] = a[k] + ac[k] * w;
        khct_r = 6;
        return;
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        for (int k = 0; k < 3; ++k) o[k] = b[k] + (c[k] - b[k]) * w;
        khct_r = 5;
        return;
    }
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom, w = vc * denom;
    for (int k = 0; k < 3; ++k) o[k] = a[k] + ab[k] * v + ac[k] * w;
    khct_r = 0;
}

// KH_SKEL: what a build records for kh_physics_bvh_refit - per kept triangle, in
// vp order, its three source vertices, and per corner its position-welded
// corner id and the id of the edge (k, k + 1) that starts there.
struct KhPhysicsBvhTopo {
    std::vector<uint32_t> src;
    std::vector<uint32_t> cid;
    std::vector<uint32_t> eid;
    uint32_t ncid = 0;
    uint32_t neid = 0;
};

// Median-split build over triangle centroids. Iterative, with an explicit
// stack: a deep recursive build on a pathological mesh is a stack overflow on
// a worker thread, and the worker has no handler.
inline void kh_physics_bvh_build(KhPhysicsBvh& khcb_o, const float* khcb_pos, uint32_t khcb_nv,
                               const uint32_t* khcb_ix, uint32_t khcb_ni, KhPhysicsBvhTopo* khcb_topo = nullptr) {
    KH_PROF_SCOPE(KHP_W_BVH_BUILD);   // KH_PROF.
    khcb_o.nodes.clear(); khcb_o.tri.clear(); khcb_o.vp.clear(); khcb_o.vd.clear();
    khcb_o.pose_seq = kh_physics_pose_seq_next(); khcb_o.prev_seq = 0u; khcb_o.vd_max = 0.0f;   // KH_COL_CLEAR: a new pose, sweeping from none.
    if (khcb_topo) { khcb_topo->src.clear(); khcb_topo->cid.clear(); khcb_topo->eid.clear(); khcb_topo->ncid = khcb_topo->neid = 0u; }
    if (!khcb_pos || !khcb_ix || khcb_ni < 3u) return;
    const uint32_t khcb_nt = khcb_ni / 3u;
    khcb_o.vp.resize(static_cast<size_t>(khcb_nt) * 9u);
    khcb_o.tri.resize(khcb_nt);
    uint32_t khcb_live = 0;
    for (uint32_t t = 0; t < khcb_nt; ++t) {
        const uint32_t i0 = khcb_ix[t * 3u], i1 = khcb_ix[t * 3u + 1u], i2 = khcb_ix[t * 3u + 2u];
        if (i0 >= khcb_nv || i1 >= khcb_nv || i2 >= khcb_nv) continue;   // A bad stream drops its triangle.
        float* d = &khcb_o.vp[static_cast<size_t>(khcb_live) * 9u];
        for (int k = 0; k < 3; ++k) {
            d[k]     = khcb_pos[i0 * 3u + k];
            d[3 + k] = khcb_pos[i1 * 3u + k];
            d[6 + k] = khcb_pos[i2 * 3u + k];
        }
        if (!kh_cloth_finite3(d) || !kh_cloth_finite3(d + 3) || !kh_cloth_finite3(d + 6)) continue;
        // A triangle collinear to float precision has no surface: it can win
        // the nearest search along its neighbours' edges and then has no face
        // normal to push by, so it is dropped at build.
        {
            float khcb_e1[3], khcb_e2[3];
            kh_cloth_v3_sub(d + 3, d, khcb_e1);
            kh_cloth_v3_sub(d + 6, d, khcb_e2);
            const float khcb_n[3] = { khcb_e1[1] * khcb_e2[2] - khcb_e1[2] * khcb_e2[1],
                                      khcb_e1[2] * khcb_e2[0] - khcb_e1[0] * khcb_e2[2],
                                      khcb_e1[0] * khcb_e2[1] - khcb_e1[1] * khcb_e2[0] };
            if (!(kh_cloth_v3_len(khcb_n) > 1.0e-6f * kh_cloth_v3_len(khcb_e1) * kh_cloth_v3_len(khcb_e2))) continue;
        }
        khcb_o.tri[khcb_live] = khcb_live;
        ++khcb_live;
        if (khcb_topo) { khcb_topo->src.push_back(i0); khcb_topo->src.push_back(i1); khcb_topo->src.push_back(i2); }
    }
    if (khcb_live == 0) { khcb_o.nodes.clear(); khcb_o.tri.clear(); khcb_o.vp.clear(); return; }
    khcb_o.tri.resize(khcb_live);
    khcb_o.vp.resize(static_cast<size_t>(khcb_live) * 9u);

    // PSEUDONORMALS (Baerentzen and Aanaes): the angle-weighted normal of every
    // corner and the summed normal of every edge, over all the faces that meet
    // there BY POSITION (a collider's hard edges and UV seams split its
    // vertices; a split corner would see only its own face). The sign of
    // (p - closest) against the pseudonormal of the feature the closest point
    // lies on is an inside test that holds everywhere on a closed mesh. A single
    // face's normal does not: past a sharp convex edge (faces turned more than
    // 90 degrees apart - the base of the builtin cone and pyramid among them) it
    // calls a point outside inside, and the push throws the cloth round the edge.
    khcb_o.pn.assign(static_cast<size_t>(khcb_live) * 18u, 0.0f);
    {
        const uint32_t khcb_nc = khcb_live * 3u;
        auto khcb_pc = [&](uint32_t i) { return &khcb_o.vp[static_cast<size_t>(i) * 3u]; };
        std::vector<uint32_t> khcb_ord(khcb_nc);
        for (uint32_t i = 0; i < khcb_nc; ++i) khcb_ord[i] = i;
        std::sort(khcb_ord.begin(), khcb_ord.end(), [&](uint32_t x, uint32_t y) {
            const float* a = khcb_pc(x);
            const float* b = khcb_pc(y);
            if (a[0] != b[0]) return a[0] < b[0];
            if (a[1] != b[1]) return a[1] < b[1];
            return a[2] < b[2];
        });
        std::vector<uint32_t> khcb_id(khcb_nc);
        uint32_t khcb_nid = 0;
        for (uint32_t i = 0; i < khcb_nc; ++i) {
            if (i > 0) {
                const float* a = khcb_pc(khcb_ord[i - 1u]);
                const float* b = khcb_pc(khcb_ord[i]);
                if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) ++khcb_nid;
            }
            khcb_id[khcb_ord[i]] = khcb_nid;
        }
        ++khcb_nid;
        std::vector<float> khcb_vpn(static_cast<size_t>(khcb_nid) * 3u, 0.0f);
        std::unordered_map<uint64_t, uint32_t> khcb_eix;
        khcb_eix.reserve(static_cast<size_t>(khcb_nc));
        std::vector<float> khcb_epn;
        std::vector<uint32_t> khcb_te(khcb_nc);   // Corner k of a triangle -> its edge (k, k + 1).
        for (uint32_t t = 0; t < khcb_live; ++t) {
            const float* v = &khcb_o.vp[static_cast<size_t>(t) * 9u];
            float e1[3], e2[3], n[3];
            kh_cloth_v3_sub(v + 3, v, e1);
            kh_cloth_v3_sub(v + 6, v, e2);
            n[0] = e1[1] * e2[2] - e1[2] * e2[1];
            n[1] = e1[2] * e2[0] - e1[0] * e2[2];
            n[2] = e1[0] * e2[1] - e1[1] * e2[0];
            const float nl = kh_cloth_v3_len(n);
            if (nl > 1.0e-30f) { n[0] /= nl; n[1] /= nl; n[2] /= nl; } else { n[0] = n[1] = n[2] = 0.0f; }
            for (int k = 0; k < 3; ++k) {
                const float* p0 = v + k * 3;
                const float* p1 = v + ((k + 1) % 3) * 3;
                const float* p2 = v + ((k + 2) % 3) * 3;
                float u[3], w[3];
                kh_cloth_v3_sub(p1, p0, u);
                kh_cloth_v3_sub(p2, p0, w);
                const float ul = kh_cloth_v3_len(u), wl = kh_cloth_v3_len(w);
                float khcb_cos = (ul > 1.0e-30f && wl > 1.0e-30f) ? kh_cloth_v3_dot(u, w) / (ul * wl) : 1.0f;
                if (khcb_cos > 1.0f) khcb_cos = 1.0f;
                if (khcb_cos < -1.0f) khcb_cos = -1.0f;
                const float khcb_ang = acosf(khcb_cos);
                float* vn = &khcb_vpn[static_cast<size_t>(khcb_id[t * 3u + k]) * 3u];
                vn[0] += n[0] * khcb_ang; vn[1] += n[1] * khcb_ang; vn[2] += n[2] * khcb_ang;
                const uint32_t ia = khcb_id[t * 3u + k], ib = khcb_id[t * 3u + (k + 1) % 3];
                const uint64_t khcb_ek = ia < ib ? (static_cast<uint64_t>(ia) << 32) | ib
                                                 : (static_cast<uint64_t>(ib) << 32) | ia;
                const auto khcb_it = khcb_eix.emplace(khcb_ek, static_cast<uint32_t>(khcb_epn.size() / 3u));
                if (khcb_it.second) { khcb_epn.push_back(0.0f); khcb_epn.push_back(0.0f); khcb_epn.push_back(0.0f); }
                float* en = &khcb_epn[static_cast<size_t>(khcb_it.first->second) * 3u];
                en[0] += n[0]; en[1] += n[1]; en[2] += n[2];
                khcb_te[t * 3u + k] = khcb_it.first->second;
            }
        }
        for (uint32_t t = 0; t < khcb_live; ++t) {
            float* pn = &khcb_o.pn[static_cast<size_t>(t) * 18u];
            for (int k = 0; k < 3; ++k) {
                memcpy(pn + k * 3, &khcb_vpn[static_cast<size_t>(khcb_id[t * 3u + k]) * 3u], sizeof(float) * 3u);
                memcpy(pn + 9 + k * 3, &khcb_epn[static_cast<size_t>(khcb_te[t * 3u + k]) * 3u], sizeof(float) * 3u);
            }
        }
        if (khcb_topo) {
            khcb_topo->cid = khcb_id;
            khcb_topo->eid = khcb_te;
            khcb_topo->ncid = khcb_nid;
            khcb_topo->neid = static_cast<uint32_t>(khcb_epn.size() / 3u);
        }
        kh_physics_convex_build(khcb_o, khcb_id.data(), khcb_live);   // KH_COL_CONVEX: the pieces, from the same welding.
        // KH_COL_CONVEX_BUILD: ...and their spheres. kh_physics_convex_build
        // leaves every piece's sphere at radius 0 about the origin; without this
        // refit a BVH that is built and never refitted (every rigid collider's,
        // kh_physics_bvh_for, and a skinned pose's when its refit is refused)
        // culls every piece in kh_physics_convex_one and a rigid collider marked
        // convex collides with nothing. vd is empty here: the rigid lanes.
        kh_physics_convex_refit(khcb_o);
    }

    std::vector<float> khcb_ctr(static_cast<size_t>(khcb_live) * 3u);
    for (uint32_t t = 0; t < khcb_live; ++t) {
        const float* v = &khcb_o.vp[static_cast<size_t>(t) * 9u];
        for (int k = 0; k < 3; ++k) khcb_ctr[t * 3u + k] = (v[k] + v[3 + k] + v[6 + k]) * (1.0f / 3.0f);
    }
    khcb_o.nodes.reserve(static_cast<size_t>(khcb_live) * 2u + 1u);
    khcb_o.nodes.push_back(KhPhysicsBvhNode());
    struct KhCbTask { uint32_t node, first, count; };
    std::vector<KhCbTask> khcb_st;
    khcb_st.push_back(KhCbTask{ 0u, 0u, khcb_live });

    while (!khcb_st.empty()) {
        const KhCbTask khcb_t = khcb_st.back();
        khcb_st.pop_back();
        KhPhysicsBvhNode& khcb_n = khcb_o.nodes[khcb_t.node];
        for (int k = 0; k < 3; ++k) { khcb_n.mn[k] = 1.0e30f; khcb_n.mx[k] = -1.0e30f; }
        for (uint32_t i = 0; i < khcb_t.count; ++i) {
            const float* v = &khcb_o.vp[static_cast<size_t>(khcb_o.tri[khcb_t.first + i]) * 9u];
            for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                if (v[c * 3 + k] < khcb_n.mn[k]) khcb_n.mn[k] = v[c * 3 + k];
                if (v[c * 3 + k] > khcb_n.mx[k]) khcb_n.mx[k] = v[c * 3 + k];
            }
        }
        if (khcb_t.count <= KH_PHYSICS_BVH_LEAF) {
            khcb_n.left = khcb_t.first; khcb_n.count = khcb_t.count;
            continue;
        }
        int khcb_ax = 0;
        {
            float khcb_ext[3];
            for (int k = 0; k < 3; ++k) khcb_ext[k] = khcb_n.mx[k] - khcb_n.mn[k];
            if (khcb_ext[1] > khcb_ext[khcb_ax]) khcb_ax = 1;
            if (khcb_ext[2] > khcb_ext[khcb_ax]) khcb_ax = 2;
        }
        const uint32_t khcb_mid = khcb_t.count / 2u;
        std::nth_element(khcb_o.tri.begin() + khcb_t.first,
                         khcb_o.tri.begin() + khcb_t.first + khcb_mid,
                         khcb_o.tri.begin() + khcb_t.first + khcb_t.count,
                         [&](uint32_t x, uint32_t y) {
                             return khcb_ctr[x * 3u + khcb_ax] < khcb_ctr[y * 3u + khcb_ax];
                         });
        const uint32_t khcb_l = static_cast<uint32_t>(khcb_o.nodes.size());
        khcb_o.nodes.push_back(KhPhysicsBvhNode());
        khcb_o.nodes.push_back(KhPhysicsBvhNode());
        // The reference above is dangling after the push_backs - re-take it.
        khcb_o.nodes[khcb_t.node].left = khcb_l;
        khcb_o.nodes[khcb_t.node].count = 0u;
        khcb_st.push_back(KhCbTask{ khcb_l,      khcb_t.first,             khcb_mid });
        khcb_st.push_back(KhCbTask{ khcb_l + 1u, khcb_t.first + khcb_mid,  khcb_t.count - khcb_mid });
    }
}

// KH_SKEL - a BVH built with a topology record (the template), moved to new
// positions of the same vertices: the corners, the pseudonormals (the build's
// arithmetic over the build's corner and edge ids) and every node's box,
// bottom-up (a node's children always come after it). The tree keeps the
// template's splits, so a pose far from it queries slower, never wrongly. A
// triangle the template dropped stays dropped and one it kept is kept. False,
// with o not to be used, when the record does not fit o or a position is not
// finite: the caller builds instead. khrf_scr is scratch. khrf_prev, when
// given, is the same vertices at the previous pose, in the same space: o then
// carries each corner's displacement (vd) and every box holds both poses, so a
// query at any fraction between them prunes correctly.
// KH_COL_CONVEX: khrf_planes = the pose is for the convex path alone: the
// corners, their motion, the root box and the planes are refitted, the nodes
// below the root and the pseudonormals stay the template's, and the pose is
// marked cvx_only.
inline bool kh_physics_bvh_refit(KhPhysicsBvh& khrf_o, const KhPhysicsBvhTopo& khrf_tp, const float* khrf_pos,
                               uint32_t khrf_nv, std::vector<float>& khrf_scr, const float* khrf_prev = nullptr,
                               bool khrf_planes = false) {
    KH_PROF_SCOPE(KHP_W_BVH_REFIT);   // KH_PROF.
    const size_t khrf_nt = khrf_o.tri.size();
    if (khrf_nt == 0u || khrf_tp.src.size() != khrf_nt * 3u || khrf_tp.cid.size() != khrf_nt * 3u ||
        khrf_tp.eid.size() != khrf_nt * 3u || khrf_o.vp.size() != khrf_nt * 9u || khrf_o.pn.size() != khrf_nt * 18u) return false;
    for (size_t t = 0; t < khrf_nt; ++t) {
        for (int c = 0; c < 3; ++c) {
            const uint32_t khrf_s = khrf_tp.src[t * 3u + c];
            if (khrf_s >= khrf_nv) return false;
            float* d = &khrf_o.vp[t * 9u + c * 3u];
            memcpy(d, khrf_pos + static_cast<size_t>(khrf_s) * 3u, sizeof(float) * 3u);
            if (!kh_cloth_finite3(d)) return false;
        }
    }
    if (khrf_prev) {
        khrf_o.vd.resize(khrf_nt * 9u);
        for (size_t t = 0; t < khrf_nt; ++t) {
            for (int c = 0; c < 3; ++c) {
                const float* khrf_q = khrf_prev + static_cast<size_t>(khrf_tp.src[t * 3u + c]) * 3u;
                if (!kh_cloth_finite3(khrf_q)) return false;
                for (int k = 0; k < 3; ++k) khrf_o.vd[t * 9u + c * 3u + k] = khrf_o.vp[t * 9u + c * 3u + k] - khrf_q[k];
            }
        }
    } else {
        khrf_o.vd.clear();
    }
    {   // KH_COL_CLEAR: a refitted copy is a new pose; its largest corner sweep. The caller names the pose it sweeps from.
        khrf_o.pose_seq = kh_physics_pose_seq_next();
        khrf_o.prev_seq = 0u;
        float khrf_m2 = 0.0f;
        for (size_t khrf_q = 0; khrf_q + 3u <= khrf_o.vd.size(); khrf_q += 3u) {
            const float* d = &khrf_o.vd[khrf_q];
            const float khrf_d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
            if (khrf_d2 > khrf_m2) khrf_m2 = khrf_d2;
        }
        khrf_o.vd_max = sqrtf(khrf_m2);
    }
    const bool khrf_sw = !khrf_o.vd.empty();
    if (khrf_planes && !khrf_o.nodes.empty()) {   // KH_COL_CONVEX: the root box over both poses, then the planes.
        KhPhysicsBvhNode& khrf_rn = khrf_o.nodes[0];
        for (int k = 0; k < 3; ++k) { khrf_rn.mn[k] = 1.0e30f; khrf_rn.mx[k] = -1.0e30f; }
        for (size_t t = 0; t < khrf_nt; ++t) {
            const float* v = &khrf_o.vp[t * 9u];
            for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                const float khrf_a = v[c * 3 + k];
                const float khrf_b = khrf_sw ? khrf_a - khrf_o.vd[t * 9u + c * 3 + k] : khrf_a;
                const float khrf_lo = khrf_a < khrf_b ? khrf_a : khrf_b, khrf_hi = khrf_a < khrf_b ? khrf_b : khrf_a;
                if (khrf_lo < khrf_rn.mn[k]) khrf_rn.mn[k] = khrf_lo;
                if (khrf_hi > khrf_rn.mx[k]) khrf_rn.mx[k] = khrf_hi;
            }
        }
        kh_physics_convex_refit(khrf_o);
        khrf_o.cvx_only = true;
        return true;
    }
    khrf_scr.assign((static_cast<size_t>(khrf_tp.ncid) + khrf_tp.neid) * 3u, 0.0f);
    float* khrf_vpn = khrf_scr.data();
    float* khrf_epn = khrf_vpn + static_cast<size_t>(khrf_tp.ncid) * 3u;
    for (size_t t = 0; t < khrf_nt; ++t) {
        const float* v = &khrf_o.vp[t * 9u];
        float e1[3], e2[3], n[3];
        kh_cloth_v3_sub(v + 3, v, e1);
        kh_cloth_v3_sub(v + 6, v, e2);
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        const float nl = kh_cloth_v3_len(n);
        if (nl > 1.0e-30f) { n[0] /= nl; n[1] /= nl; n[2] /= nl; } else { n[0] = n[1] = n[2] = 0.0f; }
        for (int k = 0; k < 3; ++k) {
            const uint32_t khrf_ci = khrf_tp.cid[t * 3u + k], khrf_ei = khrf_tp.eid[t * 3u + k];
            if (khrf_ci >= khrf_tp.ncid || khrf_ei >= khrf_tp.neid) return false;
            const float* p0 = v + k * 3;
            const float* p1 = v + ((k + 1) % 3) * 3;
            const float* p2 = v + ((k + 2) % 3) * 3;
            float u[3], w[3];
            kh_cloth_v3_sub(p1, p0, u);
            kh_cloth_v3_sub(p2, p0, w);
            const float ul = kh_cloth_v3_len(u), wl = kh_cloth_v3_len(w);
            float khrf_cos = (ul > 1.0e-30f && wl > 1.0e-30f) ? kh_cloth_v3_dot(u, w) / (ul * wl) : 1.0f;
            if (khrf_cos > 1.0f) khrf_cos = 1.0f;
            if (khrf_cos < -1.0f) khrf_cos = -1.0f;
            const float khrf_ang = acosf(khrf_cos);
            float* vn = khrf_vpn + static_cast<size_t>(khrf_ci) * 3u;
            vn[0] += n[0] * khrf_ang; vn[1] += n[1] * khrf_ang; vn[2] += n[2] * khrf_ang;
            float* en = khrf_epn + static_cast<size_t>(khrf_ei) * 3u;
            en[0] += n[0]; en[1] += n[1]; en[2] += n[2];
        }
    }
    for (size_t t = 0; t < khrf_nt; ++t) {
        float* pn = &khrf_o.pn[t * 18u];
        for (int k = 0; k < 3; ++k) {
            memcpy(pn + k * 3, khrf_vpn + static_cast<size_t>(khrf_tp.cid[t * 3u + k]) * 3u, sizeof(float) * 3u);
            memcpy(pn + 9 + k * 3, khrf_epn + static_cast<size_t>(khrf_tp.eid[t * 3u + k]) * 3u, sizeof(float) * 3u);
        }
    }
    for (size_t i = khrf_o.nodes.size(); i-- > 0u;) {
        KhPhysicsBvhNode& khrf_n = khrf_o.nodes[i];
        for (int k = 0; k < 3; ++k) { khrf_n.mn[k] = 1.0e30f; khrf_n.mx[k] = -1.0e30f; }
        if (khrf_n.count != 0u) {
            for (uint32_t j = 0; j < khrf_n.count; ++j) {
                const size_t khrf_ti = static_cast<size_t>(khrf_o.tri[khrf_n.left + j]) * 9u;
                const float* v = &khrf_o.vp[khrf_ti];
                for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                    const float khrf_a = v[c * 3 + k];
                    const float khrf_b = khrf_sw ? khrf_a - khrf_o.vd[khrf_ti + c * 3 + k] : khrf_a;   // The previous pose.
                    const float khrf_lo = khrf_a < khrf_b ? khrf_a : khrf_b, khrf_hi = khrf_a < khrf_b ? khrf_b : khrf_a;
                    if (khrf_lo < khrf_n.mn[k]) khrf_n.mn[k] = khrf_lo;
                    if (khrf_hi > khrf_n.mx[k]) khrf_n.mx[k] = khrf_hi;
                }
            }
        } else {
            for (uint32_t khrf_c = 0; khrf_c < 2u; ++khrf_c) {
                const KhPhysicsBvhNode& khrf_k = khrf_o.nodes[khrf_n.left + khrf_c];
                for (int k = 0; k < 3; ++k) {
                    if (khrf_k.mn[k] < khrf_n.mn[k]) khrf_n.mn[k] = khrf_k.mn[k];
                    if (khrf_k.mx[k] > khrf_n.mx[k]) khrf_n.mx[k] = khrf_k.mx[k];
                }
            }
        }
    }
    kh_physics_convex_refit(khrf_o);   // KH_COL_CONVEX: the planes of this pose too.
    khrf_o.cvx_only = false;
    return true;
}

// Nearest point on the collider surface to p within radius r; false when
// nothing is in reach. Iterative descent with an explicit stack (as the build).
// KH_SKEL: khcq_vd / khcq_s, when given, place each triangle's corners at
// vp + s * vd (a deforming collider part-way through its frame).
// KH_CLOTH_WARM: khcq_warm, when given, is the triangle this query hit last
// time (KH_CLOTH_WARM_NONE = none). It is tested FIRST and its distance seeds
// the bound, so the walk skips every node further than it: cloth resting on a
// body hits the same triangles for hundreds of substeps, and the walk then
// visits almost nothing. The answer is the same nearest point (a seed is only
// a candidate; anything strictly nearer replaces it; at exactly equal distance
// the seed wins). On a hit the slot takes the triangle found; on a miss it
// keeps what it held.
static constexpr uint32_t KH_CLOTH_WARM_NONE = 0xFFFFFFFFu;
inline bool kh_physics_bvh_nearest(const KhPhysicsBvh& khcq_b, const float* khcq_p, float khcq_r,
                                 float* khcq_out, float& khcq_d, uint32_t& khcq_tri, int& khcq_reg,
                                 const float* khcq_vd = nullptr, float khcq_s = 0.0f, uint32_t* khcq_warm = nullptr) {
    if (khcq_b.empty()) return false;
    float khcq_best2 = khcq_r * khcq_r;
    bool  khcq_hit = false;
    if (khcq_warm && *khcq_warm != KH_CLOTH_WARM_NONE &&
        (static_cast<size_t>(*khcq_warm) + 1u) * 9u <= khcq_b.vp.size()) {
        const uint32_t khcq_wt = *khcq_warm;
        const float* v = &khcq_b.vp[static_cast<size_t>(khcq_wt) * 9u];
        float khcq_vv[9];
        if (khcq_vd) {
            const float* khcq_dv = khcq_vd + static_cast<size_t>(khcq_wt) * 9u;
            for (int j = 0; j < 9; ++j) khcq_vv[j] = v[j] + khcq_s * khcq_dv[j];
            v = khcq_vv;
        }
        float khcq_c[3];
        int khcq_r0 = 0;
        kh_cloth_closest_tri(khcq_p, v, v + 3, v + 6, khcq_c, khcq_r0);
        float khcq_dv[3];
        kh_cloth_v3_sub(khcq_c, khcq_p, khcq_dv);
        const float khcq_d2 = kh_cloth_v3_dot(khcq_dv, khcq_dv);
        if (khcq_d2 < khcq_best2) {
            khcq_best2 = khcq_d2;
            khcq_out[0] = khcq_c[0]; khcq_out[1] = khcq_c[1]; khcq_out[2] = khcq_c[2];
            khcq_tri = khcq_wt;
            khcq_reg = khcq_r0;
            khcq_hit = true;
        }
    }
    uint32_t khcq_st[64];
    int khcq_sp = 0;
    khcq_st[khcq_sp++] = 0u;
    while (khcq_sp > 0) {
        const uint32_t khcq_ni = khcq_st[--khcq_sp];
        const KhPhysicsBvhNode& khcq_n = khcq_b.nodes[khcq_ni];
        // Squared distance from p to the node box; a box further than the
        // current best cannot hold a nearer triangle.
        float khcq_bd = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float v = khcq_p[k] < khcq_n.mn[k] ? khcq_n.mn[k] - khcq_p[k]
                          : (khcq_p[k] > khcq_n.mx[k] ? khcq_p[k] - khcq_n.mx[k] : 0.0f);
            khcq_bd += v * v;
        }
        if (khcq_bd > khcq_best2) continue;
        if (khcq_n.count != 0u) {
            for (uint32_t i = 0; i < khcq_n.count; ++i) {
                const float* v = &khcq_b.vp[static_cast<size_t>(khcq_b.tri[khcq_n.left + i]) * 9u];
                float khcq_vv[9];
                if (khcq_vd) {
                    const float* khcq_dv = khcq_vd + static_cast<size_t>(khcq_b.tri[khcq_n.left + i]) * 9u;
                    for (int j = 0; j < 9; ++j) khcq_vv[j] = v[j] + khcq_s * khcq_dv[j];
                    v = khcq_vv;
                }
                float khcq_c[3];
                int khcq_r = 0;
                kh_cloth_closest_tri(khcq_p, v, v + 3, v + 6, khcq_c, khcq_r);
                float khcq_dv[3];
                kh_cloth_v3_sub(khcq_c, khcq_p, khcq_dv);
                const float khcq_d2 = kh_cloth_v3_dot(khcq_dv, khcq_dv);
                if (khcq_d2 < khcq_best2) {
                    khcq_best2 = khcq_d2;
                    khcq_out[0] = khcq_c[0]; khcq_out[1] = khcq_c[1]; khcq_out[2] = khcq_c[2];
                    khcq_tri = khcq_b.tri[khcq_n.left + i];
                    khcq_reg = khcq_r;
                    khcq_hit = true;
                }
            }
            continue;
        }
        if (khcq_sp + 2 <= 64) {
            khcq_st[khcq_sp++] = khcq_n.left;
            khcq_st[khcq_sp++] = khcq_n.left + 1u;
        }
    }
    if (khcq_hit) khcq_d = sqrtf(khcq_best2);
    if (khcq_hit && khcq_warm) *khcq_warm = khcq_tri;
    return khcq_hit;
}

// The proportions a collider's BVH is built at. A nearest-point search is only
// METRIC when the BVH's space differs from metres by one uniform scale; the
// importer normalises every model per axis, so a non-cubic collider queried in
// normalized space finds the wrong nearest point, feature or triangle (the side
// is unaffected: the inside test is scale-invariant). The BVH is built with its
// normalized positions times these proportions - size over its largest axis,
// quantized in log2 to 1/KH_PHYSICS_BVH_SHAPE_STEPS of an octave - leaving the
// query a scale uniform to within half a step. Returns the shape's cache key
// (10 bits per axis).
static constexpr float KH_PHYSICS_BVH_SHAPE_STEPS = 64.0f;
inline uint32_t kh_physics_bvh_shape(const float* khbs_size, float* khbs_bake) {
    float khbs_m = fabsf(khbs_size[0]);
    if (fabsf(khbs_size[1]) > khbs_m) khbs_m = fabsf(khbs_size[1]);
    if (fabsf(khbs_size[2]) > khbs_m) khbs_m = fabsf(khbs_size[2]);
    uint32_t khbs_key = 0;
    for (int k = 0; k < 3; ++k) {
        const float khbs_r = khbs_m > 1.0e-6f ? fabsf(khbs_size[k]) / khbs_m : 1.0f;
        float khbs_q = -log2f(khbs_r > 1.0e-3f ? khbs_r : 1.0e-3f) * KH_PHYSICS_BVH_SHAPE_STEPS;
        khbs_q = floorf(khbs_q + 0.5f);
        if (khbs_q < 0.0f) khbs_q = 0.0f;
        if (khbs_q > 1023.0f) khbs_q = 1023.0f;
        khbs_bake[k] = exp2f(-khbs_q / KH_PHYSICS_BVH_SHAPE_STEPS);
        khbs_key |= static_cast<uint32_t>(khbs_q) << (k * 10);
    }
    return khbs_key;
}

// The frame a fraction t of the way from a to b, exact at both ends. Centre
// and size are linear; the rotation rows are blended and re-orthonormalized
// (Gram-Schmidt, handedness taken from the blend).
inline void kh_cloth_frame_lerp(const KhClothFrame& a, const KhClothFrame& b, float t, KhClothFrame& o) {
    if (t <= 0.0f) { o = a; return; }
    if (t >= 1.0f) { o = b; return; }
    for (int k = 0; k < 3; ++k) {
        o.centre[k] = a.centre[k] + (b.centre[k] - a.centre[k]) * t;
        o.size[k] = a.size[k] + (b.size[k] - a.size[k]) * t;
    }
    static const float khfl_id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    if (!a.rotated && !b.rotated) { memcpy(o.rot, khfl_id, sizeof(o.rot)); o.rotated = false; return; }
    const float* ra = a.rotated ? a.rot : khfl_id;
    const float* rb = b.rotated ? b.rot : khfl_id;
    float m[9];
    for (int k = 0; k < 9; ++k) m[k] = ra[k] + (rb[k] - ra[k]) * t;
    float r0[3] = { m[0], m[1], m[2] };
    const float l0 = kh_cloth_v3_len(r0);
    const float d01 = l0 > 1.0e-6f ? kh_cloth_v3_dot(m + 3, r0) / (l0 * l0) : 0.0f;
    float r1[3] = { m[3] - r0[0] * d01, m[4] - r0[1] * d01, m[5] - r0[2] * d01 };
    const float l1 = kh_cloth_v3_len(r1);
    if (l0 < 1.0e-6f || l1 < 1.0e-6f) {   // A half turn blends through zero: take the nearer end's rows.
        memcpy(o.rot, t < 0.5f ? ra : rb, sizeof(o.rot));
        o.rotated = true;
        return;
    }
    for (int k = 0; k < 3; ++k) { r0[k] /= l0; r1[k] /= l1; }
    float r2[3] = { r0[1] * r1[2] - r0[2] * r1[1], r0[2] * r1[0] - r0[0] * r1[2], r0[0] * r1[1] - r0[1] * r1[0] };
    if (kh_cloth_v3_dot(r2, m + 6) < 0.0f) { r2[0] = -r2[0]; r2[1] = -r2[1]; r2[2] = -r2[2]; }
    memcpy(o.rot, r0, sizeof(r0)); memcpy(o.rot + 3, r1, sizeof(r1)); memcpy(o.rot + 6, r2, sizeof(r2));
    o.rotated = true;
}

// Field by field - the struct has padding, and a whole-struct compare reads it.
inline bool kh_cloth_frame_same(const KhClothFrame& a, const KhClothFrame& b) {
    if (a.rotated != b.rotated) return false;
    for (int k = 0; k < 3; ++k) if (a.centre[k] != b.centre[k] || a.size[k] != b.size[k]) return false;
    if (a.rotated) for (int k = 0; k < 9; ++k) if (a.rot[k] != b.rot[k]) return false;
    return true;
}

// A frame that moved further than 'teleport' metres, or turned more than 60
// degrees, between two samples did not move: it jumped, and nothing may be
// swept across the gap. trace(Ra^T Rb) is 3 for no turn and 2 at 60 degrees.
// Each caller's first test hands in 'a' carried forward by its own velocity
// where one is known (the carrier's carrier_v in kh_cloth_step and
// kh_chain_step, a collider view's vel in kh_cloth_sync), so the distance is
// the miss from where steady motion would have put it; with none known yet it
// is the plain distance moved. KH_TELEPORT_ONCE's second test in the two steps
// hands in the unpredicted last frame's end: the plain distance moved.
inline bool kh_cloth_frame_jumped(const KhClothFrame& a, const KhClothFrame& b, float khfj_teleport) {
    float d[3];
    kh_cloth_v3_sub(b.centre, a.centre, d);
    if (kh_cloth_v3_len(d) > (khfj_teleport > 0.0f ? khfj_teleport : 2.0f)) return true;
    if (a.rotated || b.rotated) {
        float tr = 0.0f;
        for (int r = 0; r < 3; ++r) for (int c = 0; c < 3; ++c) {
            const float ea = a.rotated ? a.rot[r * 3 + c] : (r == c ? 1.0f : 0.0f);
            const float eb = b.rotated ? b.rot[r * 3 + c] : (r == c ? 1.0f : 0.0f);
            tr += ea * eb;
        }
        if (tr < 2.0f) return true;
    }
    return false;
}

// How far f1 carries a point that f0 held at p - the carrier's own
// displacement at p between two samples. khfd_ratio is f1.size / f0.size per
// axis. Formed from offsets against the centres, never from two absolute
// positions, so it keeps its precision at altitude.
inline void kh_cloth_frame_delta(const float* p, const KhClothFrame& f0, const float* khfd_ratio,
                                 const KhClothFrame& f1, float* o) {
    float d[3];
    kh_cloth_v3_sub(p, f0.centre, d);
    float r[3];
    if (f0.rotated) {
        for (int k = 0; k < 3; ++k) r[k] = f0.rot[k * 3] * d[0] + f0.rot[k * 3 + 1] * d[1] + f0.rot[k * 3 + 2] * d[2];
    } else { r[0] = d[0]; r[1] = d[1]; r[2] = d[2]; }
    for (int k = 0; k < 3; ++k) r[k] *= khfd_ratio[k];
    float q[3];
    if (f1.rotated) {
        for (int k = 0; k < 3; ++k) q[k] = f1.rot[k] * r[0] + f1.rot[3 + k] * r[1] + f1.rot[6 + k] * r[2];
    } else { q[0] = r[0]; q[1] = r[1]; q[2] = r[2]; }
    for (int k = 0; k < 3; ++k) o[k] = (f1.centre[k] - f0.centre[k]) + (q[k] - d[k]);
}

// KH_CLOTH_GROUND - the terrain under one cloth for one frame: the engine's own
// terrain heights at its grid nodes (the framework's terrain matrix), copied by
// kh_cloth_sync into the instance's frame, and how the engine splits each grid
// cell into its two flat triangles (kh_cloth_ground_prepare measured it).
// Between the nodes the surface is those triangles, as the engine's own is;
// split 2 is a smooth bilinear fit, used only where the split was not
// recognised. A heightfield has no far side: a particle below it always comes
// back up, it cannot tunnel.
// Terrain only: buildings, floors, bridges and rocks are not ground - cloth and chains need a physicsCollider for
// them, and a cloth or chain on an upper floor falls through to the terrain.
static constexpr int KH_CLOTH_GROUND_PASSES = 4;   // Projections per contact; see kh_cloth_ground_contact.
inline bool kh_cloth_ground_contact(const KhClothGround& g, float thick, float* p, float* n_out) {
    if (!g.on || g.nx < 2 || g.nz < 2 || !(g.cell > 0.0f) || g.h.size() < static_cast<size_t>(g.nx) * g.nz) return false;
    bool moved = false;
    for (int pass = 0; pass < KH_CLOTH_GROUND_PASSES; ++pass) {
        if (!kh_cloth_ground_plane(g, thick, p, n_out)) break;
        moved = true;
    }
    return moved;
}
// One projection onto the triangle under p (kh_cloth_ground_contact's pass).
inline bool kh_cloth_ground_plane(const KhClothGround& g, float thick, float* p, float* n_out) {
    const float gx = (p[0] - g.x0) / g.cell;
    const float gz = (p[2] - g.z0) / g.cell;
    if (!(gx >= 0.0f) || !(gz >= 0.0f)) return false;   // Outside, or not a number.
    const float lx = static_cast<float>(g.nx - 1u), lz = static_cast<float>(g.nz - 1u);
    if (gx > lx || gz > lz) return false;
    uint32_t i = static_cast<uint32_t>(gx), j = static_cast<uint32_t>(gz);
    if (i > g.nx - 2u) i = g.nx - 2u;   // On the far node line itself.
    if (j > g.nz - 2u) j = g.nz - 2u;
    const float fx = gx - static_cast<float>(i), fz = gz - static_cast<float>(j);
    const float* r0 = &g.h[static_cast<size_t>(j) * g.nx + i];
    const float* r1 = r0 + g.nx;
    const float h00 = r0[0], h10 = r0[1], h01 = r1[0], h11 = r1[1];
    const uint8_t sp = g.split[((i + g.px) & 1u) | (((j + g.pz) & 1u) << 1)];
    float h, sx, sz;   // The height under the particle and its rise per cell east / north.
    if (sp == 0u) {
        if (fx >= fz) { sx = h10 - h00; sz = h11 - h10; } else { sx = h11 - h01; sz = h01 - h00; }
        h = h00 + sx * fx + sz * fz;
    } else if (sp == 1u) {
        if (fx + fz <= 1.0f) { sx = h10 - h00; sz = h01 - h00; h = h00 + sx * fx + sz * fz; }
        else { sx = h11 - h01; sz = h11 - h10; h = h11 + sx * (fx - 1.0f) + sz * (fz - 1.0f); }
    } else {
        sx = (h10 - h00) * (1.0f - fz) + (h11 - h01) * fz;
        sz = (h01 - h00) * (1.0f - fx) + (h11 - h10) * fx;
        h = (h00 * (1.0f - fx) + h10 * fx) * (1.0f - fz) + (h01 * (1.0f - fx) + h11 * fx) * fz;
    }
    const float kx = sx / g.cell, kz = sz / g.cell;
    const float il = 1.0f / sqrtf(1.0f + kx * kx + kz * kz);
    const float s = (p[1] - h) * il;   // Signed distance to the triangle's plane.
    if (s >= thick) return false;
    n_out[0] = -kx * il; n_out[1] = il; n_out[2] = -kz * il;
    const float push = thick - s;
    for (int k = 0; k < 3; ++k) p[k] += n_out[k] * push;
    return true;
}

inline KhClothParams kh_cloth_defaults() {
    KhClothParams p;
    // drag: a hinged cape, as a flat plate (Cd 1.2) at 0.2-0.4 kg/m^2, sweeps
    // back 19-32 degrees at a 1.4 m/s walk and 48-60 at a 3 m/s jog.
    // KH_CLOTH_DRAG: 0.735 kg/s per point at the default 0.2 kg is 3.67 /s
    // against a face's motion, which lands inside both ranges; waterDrag 12.26 is 61.3 /s (KH_CLOTH_SKIN of
    // that along the surface).
    p.stretch = 0.90f; p.bend = 0.20f; p.damping = 0.02f; p.drag = 0.735f;
    p.mass = 0.20f; p.gravity = 1.0f; p.max_stretch = 1.05f; p.thickness = 0.01f;
    p.friction = 0.30f; p.max_speed = 40.0f; p.teleport = 2.0f; p.range = 1.0f;
    p.sleep = 0.005f; p.shell = 0.0f;
    p.wind = 1.0f; p.buoyancy = 1.0f; p.water_drag = 12.26f;
    // self_friction matches the contact a cloth makes with a collider at the
    // defaults (0.3 x 1.0).
    p.self_collide = false; p.self_thickness = 0.01f; p.self_friction = 0.30f;
    p.iterations = 8; p.substeps = 2;
    return p;
}

// A 0..1 stiffness dial to XPBD compliance (1/stiffness, N^-1 m), mapped
// exponentially: the interesting range of cloth spans several decades, and a
// linear dial spends nine tenths of its travel on values that all look
// inextensible.
inline float kh_cloth_compliance(float khcc_dial, float khcc_soft) {
    if (khcc_dial >= 0.999f) return 0.0f;              // Exactly rigid.
    if (khcc_dial <= 0.0f) return khcc_soft;
    return khcc_soft * powf(1.0e-4f, khcc_dial);
}

// Mesh-local rest position -> world, through the object's frame: the mesh is
// authored normalized to [-0.5, 0.5] and drawn at 'size', exactly the vertex
// shader's transform, so the cloth's rest pose and its drawn rest pose are the
// same points.
inline void kh_cloth_l2w(const float* khcl_local, const float* khcl_centre, const float* khcl_rot,
                         bool khcl_rotated, const float* khcl_size, float* khcl_out) {
    const float s[3] = { khcl_local[0] * khcl_size[0],
                         khcl_local[1] * khcl_size[1],
                         khcl_local[2] * khcl_size[2] };
    if (khcl_rotated) {
        for (int k = 0; k < 3; ++k) {
            khcl_out[k] = khcl_centre[k] + khcl_rot[k] * s[0] + khcl_rot[3 + k] * s[1] + khcl_rot[6 + k] * s[2];
        }
    } else {
        for (int k = 0; k < 3; ++k) khcl_out[k] = khcl_centre[k] + s[k];
    }
}

// World -> mesh-local, the inverse of kh_cloth_l2w. rot_m is orthonormal, so
// the inverse rotation is its transpose; a zero size axis (a non-invertible
// frame) is floored, not divided by.
inline void kh_cloth_w2l(const float* khcw_world, const float* khcw_centre, const float* khcw_rot,
                         bool khcw_rotated, const float* khcw_inv_size, float* khcw_out) {
    const float d[3] = { khcw_world[0] - khcw_centre[0],
                         khcw_world[1] - khcw_centre[1],
                         khcw_world[2] - khcw_centre[2] };
    float r[3];
    if (khcw_rotated) {
        // Transposed here, in one place: kh_cloth_l2w reads the rows as basis
        // vectors, so the inverse reads the same nine floats as columns.
        for (int k = 0; k < 3; ++k) {
            r[k] = khcw_rot[k * 3] * d[0] + khcw_rot[k * 3 + 1] * d[1] + khcw_rot[k * 3 + 2] * d[2];
        }
    } else { r[0] = d[0]; r[1] = d[1]; r[2] = d[2]; }
    for (int k = 0; k < 3; ++k) khcw_out[k] = r[k] * khcw_inv_size[k];
}

// KH_COL_CONVEX - one convex collider pass over one particle: the twin of
// kh_physics_collide_one below (same frames, outputs, standoff and friction
// contract), for a collider whose pose carries pieces. Per piece whose sphere
// the particle is within reach of, the signed distance is the largest of the
// piece's plane distances: exact on a face; past an edge or a corner less than
// the true distance, so the standoff surface keeps the piece's corners where
// the triangle path rounds them (a box's standoff is a bigger box) - a particle
// there is held out by up to the standoff more, never less; deep inside it is
// the nearest face out, so an embedded particle leaves through the closest
// wall with no wider search. The common case is one plane test: the plane that separated this
// particle last time (its warm slot) is tested first, and a particle it still
// clears skips the piece; otherwise the planes are scanned until one separates
// (it becomes the warm slot), and a particle inside the standoff, which none
// separates, scans them all. An 'inside' collider skips the warm test and scans
// every plane.
// The planes are tested in the BVH's space with the world standoff shrunk by
// the smallest residual scale (conservative, as the BVH query's reach), and the
// decision is made in world on the deciding plane. 'inside' keeps the particle
// inside some piece by the standoff, pushing it into the piece it is least
// outside of. A deforming pose interpolates each plane between the previous and
// this pose at the substep's end, and carries the contact back to the substep's
// start by the plane's mean motion, as the BVH path does by the hit triangle's
// corners.
inline bool kh_physics_convex_one(const KhPhysicsColliderView& khcx_c, float khcx_thick,
                                  const float* khcx_prev, float* khcx_p, float* khcx_n_out, float* khcx_sv_out,
                                  uint32_t* khcx_warm) {
    const KhPhysicsBvh& khcx_b = *khcx_c.bvh;
    float khcx_mv[3];
    kh_cloth_v3_sub(khcx_p, khcx_prev, khcx_mv);
    const float khcx_motion = kh_cloth_v3_len(khcx_mv);
    const float khcx_reach = khcx_thick + khcx_c.margin + khcx_motion;
    if (!khcx_c.inside) {
        float d[3]; kh_cloth_v3_sub(khcx_p, khcx_c.bs_c, d);
        const float rr = khcx_c.bs_r + khcx_reach;
        if (kh_cloth_v3_dot(d, d) > rr * rr) return false;
    }
    float khcx_es[3], khcx_ies[3], khcx_pes[3];
    for (int k = 0; k < 3; ++k) {
        const float khcx_bk = khcx_c.bake[k] > 0.0f ? khcx_c.bake[k] : 1.0f;
        khcx_es[k] = khcx_c.size[k] / khcx_bk;
        khcx_ies[k] = khcx_c.inv_size[k] * khcx_bk;
        khcx_pes[k] = khcx_c.prev.size[k] / khcx_bk;
    }
    float khcx_smin = khcx_es[0];
    if (khcx_es[1] < khcx_smin) khcx_smin = khcx_es[1];
    if (khcx_es[2] < khcx_smin) khcx_smin = khcx_es[2];
    if (khcx_smin < 1.0e-6f) khcx_smin = 1.0e-6f;
    const float khcx_want = khcx_thick + khcx_c.margin;
    const float khcx_rl = khcx_reach / khcx_smin;   // The reach in the planes' space, conservative.
    const bool khcx_def = khcx_c.deform && khcx_b.cvx_pl0.size() == khcx_b.cvx_pl.size() && khcx_b.cvx_pv.size() == khcx_b.cvx_tri.size() * 3u;
    const float khcx_s1 = khcx_def ? khcx_c.def1 : 0.0f;
    const float khcx_w = 1.0f + khcx_s1;   // Of the way from the previous pose to this one, at the substep's end.
    const uint32_t khcx_np = static_cast<uint32_t>(khcx_b.cvx_pstart.size() - 1u);
    const bool khcx_prev_ok = khcx_c.prev.size[0] != 0.0f || khcx_c.prev.size[1] != 0.0f || khcx_c.prev.size[2] != 0.0f;
    // One plane at the pose: unit normal and offset in the planes' space.
    auto khcx_plane = [&](uint32_t q, float* n, float& d) {
        const float* p1 = &khcx_b.cvx_pl[static_cast<size_t>(q) * 4u];
        if (!khcx_def) { n[0] = p1[0]; n[1] = p1[1]; n[2] = p1[2]; d = p1[3]; return; }
        const float* p0 = &khcx_b.cvx_pl0[static_cast<size_t>(q) * 4u];
        for (int k = 0; k < 3; ++k) n[k] = p0[k] + khcx_w * (p1[k] - p0[k]);
        d = p0[3] + khcx_w * (p1[3] - p0[3]);
        const float nl = kh_cloth_v3_len(n);
        if (nl > 1.0e-6f) { for (int k = 0; k < 3; ++k) n[k] /= nl; d /= nl; }
        else { n[0] = p1[0]; n[1] = p1[1]; n[2] = p1[2]; d = p1[3]; }
    };
    float khcx_l[3];
    kh_cloth_w2l(khcx_p, khcx_c.centre, khcx_c.rot, khcx_c.rotated, khcx_ies, khcx_l);
    // Resolve the particle against plane q (local n, d; its signed distance
    // sd there), in world: to the standoff on the side the collider keeps
    // cloth. False when the world distance already clears it.
    auto khcx_resolve = [&](uint32_t q, const float* nl, float sd) -> bool {
        float khcx_cl[3];
        for (int k = 0; k < 3; ++k) khcx_cl[k] = khcx_l[k] - nl[k] * sd;   // The contact point, on the plane.
        float khcx_cw[3];
        kh_cloth_l2w(khcx_cl, khcx_c.centre, khcx_c.rot, khcx_c.rotated, khcx_es, khcx_cw);
        float khcx_nn[3] = { nl[0] * khcx_ies[0], nl[1] * khcx_ies[1], nl[2] * khcx_ies[2] };   // The normal, as the BVH path carries it.
        float nw[3];
        if (khcx_c.rotated) {
            for (int k = 0; k < 3; ++k) nw[k] = khcx_c.rot[k] * khcx_nn[0] + khcx_c.rot[3 + k] * khcx_nn[1] + khcx_c.rot[6 + k] * khcx_nn[2];
        } else { nw[0] = khcx_nn[0]; nw[1] = khcx_nn[1]; nw[2] = khcx_nn[2]; }
        const float nlen = kh_cloth_v3_len(nw);
        if (nlen < 1.0e-12f) return false;
        for (int k = 0; k < 3; ++k) nw[k] /= nlen;
        float khcx_dv[3];
        kh_cloth_v3_sub(khcx_p, khcx_cw, khcx_dv);
        const float khcx_sdw = kh_cloth_v3_dot(khcx_dv, nw);   // Signed, outward.
        const float khcx_target = khcx_c.inside ? -khcx_want : khcx_want;
        if (khcx_c.inside ? khcx_sdw <= khcx_target : khcx_sdw >= khcx_target) return false;
        for (int k = 0; k < 3; ++k) khcx_p[k] += nw[k] * (khcx_target - khcx_sdw);
        for (int k = 0; k < 3; ++k) khcx_n_out[k] = khcx_c.inside ? -nw[k] : nw[k];
        khcx_sv_out[0] = khcx_sv_out[1] = khcx_sv_out[2] = 0.0f;
        if (khcx_prev_ok) {
            float khcx_c0[3], khcx_cp[3];
            if (khcx_def) {
                const float* pv = &khcx_b.cvx_pv[static_cast<size_t>(q) * 3u];
                const float khcx_ds = khcx_c.def0 - khcx_s1;   // <= 0: back to the substep's start.
                for (int k = 0; k < 3; ++k) khcx_c0[k] = khcx_cl[k] + khcx_ds * pv[k];
            } else { memcpy(khcx_c0, khcx_cl, sizeof(khcx_c0)); }
            kh_cloth_l2w(khcx_c0, khcx_c.prev.centre, khcx_c.prev.rot, khcx_c.prev.rotated, khcx_pes, khcx_cp);
            for (int k = 0; k < 3; ++k) khcx_sv_out[k] = khcx_cw[k] - khcx_cp[k];
        }
        kh_cloth_w2l(khcx_p, khcx_c.centre, khcx_c.rot, khcx_c.rotated, khcx_ies, khcx_l);   // For the next piece.
        return true;
    };
    bool khcx_hit = false;
    const uint32_t khcx_wq = khcx_warm ? *khcx_warm : KH_CLOTH_WARM_NONE;
    // 'inside': the piece the particle is least outside of, unless one holds it.
    float khcx_in_best = 3.0e38f; uint32_t khcx_in_q = KH_CLOTH_WARM_NONE; float khcx_in_n[3] = { 0.0f, 0.0f, 0.0f };
    for (uint32_t pc = 0; pc < khcx_np; ++pc) {
        const uint32_t q0 = khcx_b.cvx_pstart[pc], q1 = khcx_b.cvx_pstart[pc + 1u];
        if (q1 <= q0) continue;
        if (!khcx_c.inside) {
            const float* sp = &khcx_b.cvx_sph[static_cast<size_t>(pc) * 4u];
            float ds[3]; kh_cloth_v3_sub(khcx_l, sp, ds);
            const float rr = sp[3] + khcx_rl;
            if (kh_cloth_v3_dot(ds, ds) > rr * rr) continue;
        }
        float n[3], d;
        if (!khcx_c.inside && khcx_wq >= q0 && khcx_wq < q1) {   // The plane that separated last time.
            khcx_plane(khcx_wq, n, d);
            if ((kh_cloth_v3_dot(n, khcx_l) - d) * khcx_smin >= khcx_want) continue;
        }
        float khcx_best = -3.0e38f, khcx_bn[3] = { 0.0f, 0.0f, 0.0f };
        uint32_t khcx_bq = KH_CLOTH_WARM_NONE;
        bool khcx_sep = false;
        for (uint32_t q = q0; q < q1; ++q) {
            khcx_plane(q, n, d);
            const float sd = kh_cloth_v3_dot(n, khcx_l) - d;
            if (!khcx_c.inside && sd * khcx_smin >= khcx_want) {   // A separating plane: clear of this piece.
                if (khcx_warm) *khcx_warm = q;
                khcx_sep = true;
                break;
            }
            if (sd > khcx_best) { khcx_best = sd; khcx_bq = q; khcx_bn[0] = n[0]; khcx_bn[1] = n[1]; khcx_bn[2] = n[2]; }
        }
        if (khcx_sep || khcx_bq == KH_CLOTH_WARM_NONE) continue;
        if (khcx_c.inside) {
            if (khcx_best * khcx_smin <= -khcx_want) return khcx_hit;   // Held by this piece.
            if (khcx_best < khcx_in_best) { khcx_in_best = khcx_best; khcx_in_q = khcx_bq; memcpy(khcx_in_n, khcx_bn, sizeof(khcx_in_n)); }
            continue;
        }
        if (khcx_warm) *khcx_warm = khcx_bq;
        if (khcx_resolve(khcx_bq, khcx_bn, khcx_best)) khcx_hit = true;
    }
    if (khcx_c.inside && khcx_in_q != KH_CLOTH_WARM_NONE) {
        if (khcx_resolve(khcx_in_q, khcx_in_n, khcx_in_best)) khcx_hit = true;
    }
    return khcx_hit;
}

// KH_COL_CLEAR - the collision pass's clearance cache. A cloth hanging from a
// body sits INSIDE the body's root box and BEYOND reach of its surface, so
// without it nearly every free particle, every substep, falls through the
// reach-limited query to the embedded-particle rescue's whole-body nearest
// search only to learn that it is outside (measured: 61 - 77 % of the near
// queries went wide, one call in 70 - 150 ended in a contact). A particle
// found OUTSIDE at distance d of the surface cannot touch or enter the collider
// until it and the surface have together closed d less the standoff, so the
// pair remembers d and where the particle stood, and the pass answers 'no
// contact' without a query while what is left of d clears the standoff. The
// particle's travel is measured in the COLLIDER'S MODEL FRAME (the BVH's space
// plus the pose's own box centre), where the collider's rigid motion, however
// fast, is no motion and a rigid collider's surface stands still exactly. A
// deforming (skinned) collider's surface moves by at most the pose's largest
// corner sweep times the sweep fraction covered (KhPhysicsBvh::vd_max), summed
// per collider column as its poses chain (kh_col_clear_advance): within a pose
// by the fraction, from a pose to the one that sweeps from it (prev_seq) by
// what was left of the old and what is covered of the new; a pose that does
// not chain from the last one used (a cut, a rebuilt BVH, a skipped pose)
// empties the column. Everything is a lower bound in the BVH's units, carried
// to metres by the smallest residual scale as the query's reach is, with
// KH_COL_CLEAR_PAD_M and KH_COL_CLEAR_SLACK over the standoff for the residual
// scale's anisotropy and float rounding. A skipped call is one the queries
// would have answered 'false' with the particle untouched; only the warm slot
// is not refreshed (a seed, not an answer). A rebase, a teleport or the
// non-finite rescue move the particle far in the model frame and spend the
// record; an origin shift moves the particles and the collider views together
// (kh_cloth_origin_shift), so the record stands.
// Not for a containing ('inside') collider nor the convex path.
// KH_COL_CLEAR_ON false = every call queries.
static constexpr bool  KH_COL_CLEAR_ON = true;
static constexpr float KH_COL_CLEAR_PAD_M = 0.005f;
static constexpr float KH_COL_CLEAR_SLACK = 0.98f;
static constexpr float KH_COL_CLEAR_ACC_MAX = 1000.0f;   // The column's sum restarts here: a float's differences stay exact to 0.1 mm.
// One collider pass over one particle. Returns true when the particle was
// moved, so the caller can apply friction against the same contact:
// khco_n_out is the push direction and khco_sv_out how far the surface under
// the contact moved this substep (the view's prev pose to its current one).
// khco_prev is the particle's position at the start of the substep. The query
// radius covers the distance travelled as well as the thickness, or a
// particle moving faster than its thickness per substep tunnels through.
inline bool kh_physics_collide_one(const KhPhysicsColliderView& khco_c, float khco_thick,
                                 const float* khco_prev, float* khco_p, float* khco_n_out, float* khco_sv_out,
                                 uint32_t* khco_warm = nullptr,   // KH_CLOTH_WARM: the particle's last-hit slot for this collider.
                                 KhColClear* khco_cc = nullptr, float khco_acc = 0.0f) {   // KH_COL_CLEAR: the pair's record and the column's sum.
    if (!khco_c.bvh || khco_c.bvh->empty()) return false;
    // KH_COL_CONVEX: a collider marked convex, or a pose refitted for the
    // planes alone (whose nodes below the root are stale), takes the planes.
    if ((khco_c.convex || khco_c.bvh->cvx_only) && khco_c.bvh->cvx_valid()) {
        return kh_physics_convex_one(khco_c, khco_thick, khco_prev, khco_p, khco_n_out, khco_sv_out, khco_warm);
    }
    float khco_mv[3];
    kh_cloth_v3_sub(khco_p, khco_prev, khco_mv);
    const float khco_motion = kh_cloth_v3_len(khco_mv);
    const float khco_reach = khco_thick + khco_c.margin + khco_motion;
    // Broadphase: the collider's world bounding sphere against the particle.
    // Only in the ordinary OUTSIDE sense - a containing collider has to reach
    // particles that have escaped it, which are by definition the ones this
    // test would reject.
    if (!khco_c.inside) {
        float d[3]; kh_cloth_v3_sub(khco_p, khco_c.bs_c, d);
        const float rr = khco_c.bs_r + khco_reach;
        if (kh_cloth_v3_dot(d, d) > rr * rr) return false;
    }
    // What carries the BVH's space to metres: the size less the proportions
    // already baked into it - uniform to within half a quantization step, which
    // is what makes the nearest search below a metric one.
    float khco_es[3], khco_ies[3], khco_pes[3];
    for (int k = 0; k < 3; ++k) {
        const float khco_b = khco_c.bake[k] > 0.0f ? khco_c.bake[k] : 1.0f;
        khco_es[k] = khco_c.size[k] / khco_b;
        khco_ies[k] = khco_c.inv_size[k] * khco_b;
        khco_pes[k] = khco_c.prev.size[k] / khco_b;
    }
    float khco_l[3];
    kh_cloth_w2l(khco_p, khco_c.centre, khco_c.rot, khco_c.rotated, khco_ies, khco_l);
    // The reach is metric on entry, so it converts through the SMALLEST axis
    // of what is left - conservative, never a missed contact.
    float khco_smin = khco_es[0];
    if (khco_es[1] < khco_smin) khco_smin = khco_es[1];
    if (khco_es[2] < khco_smin) khco_smin = khco_es[2];
    if (khco_smin < 1.0e-6f) khco_smin = 1.0e-6f;
    const float khco_r = khco_reach / khco_smin;
    // KH_COL_CLEAR: the particle in the collider's model frame; a record that
    // still clears the standoff answers the call.
    const float khco_lm[3] = { khco_l[0] + khco_c.skin_ctr[0], khco_l[1] + khco_c.skin_ctr[1], khco_l[2] + khco_c.skin_ctr[2] };
    if (khco_c.inside) khco_cc = nullptr;
    if (khco_cc && khco_cc->d > 0.0f) {
        const float khco_m[3] = { khco_lm[0] - khco_cc->l[0], khco_lm[1] - khco_cc->l[1], khco_lm[2] - khco_cc->l[2] };
        const float khco_left = (khco_cc->d - sqrtf(khco_m[0] * khco_m[0] + khco_m[1] * khco_m[1] + khco_m[2] * khco_m[2]) -
                                 (khco_acc - khco_cc->acc)) * khco_smin;
        if (khco_left * KH_COL_CLEAR_SLACK > khco_thick + khco_c.margin + KH_COL_CLEAR_PAD_M) return false;
        khco_cc->d = 0.0f;   // Spent (a NaN lands here too): query.
    }
    // KH_SKEL: a deforming collider is queried at the substep's end pose.
    const bool khco_def = khco_c.deform && !khco_c.bvh->vd.empty() && khco_c.bvh->vd.size() == khco_c.bvh->vp.size();
    const float* khco_vd = khco_def ? khco_c.bvh->vd.data() : nullptr;
    const float khco_s1 = khco_def ? khco_c.def1 : 0.0f;
    float khco_cl[3]; float khco_d = 0.0f; uint32_t khco_t = 0u; int khco_reg = 0;
    if (!kh_physics_bvh_nearest(*khco_c.bvh, khco_l, khco_r, khco_cl, khco_d, khco_t, khco_reg, khco_vd, khco_s1, khco_warm)) {
        // Nothing within reach of the surface: usually 'far outside', but also
        // what a DEEPLY EMBEDDED particle looks like (spawned inside, or carried
        // in by a teleport), and leaving that one alone strands it inside the
        // collider forever. The collider's root box tells the two apart, so the
        // wider query is paid for only by the particles inside it.
        const KhPhysicsBvhNode& khco_root = khco_c.bvh->nodes[0];
        float khco_out2 = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float e = khco_l[k] < khco_root.mn[k] ? khco_root.mn[k] - khco_l[k]
                          : (khco_l[k] > khco_root.mx[k] ? khco_l[k] - khco_root.mx[k] : 0.0f);
            khco_out2 += e * e;
        }
        // Outside the root box and not a container: genuinely nowhere near.
        if (khco_out2 > 0.0f && !khco_c.inside) return false;
        float khco_diag = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float e = khco_root.mx[k] - khco_root.mn[k];
            khco_diag += e * e;
        }
        khco_diag = sqrtf(khco_diag) + sqrtf(khco_out2);
        if (!kh_physics_bvh_nearest(*khco_c.bvh, khco_l, khco_diag, khco_cl, khco_d, khco_t, khco_reg, khco_vd, khco_s1, khco_warm)) return false;
    }

    // Back to world, and do the whole decision there: a BVH-space distance is
    // a depth only to within the residual scale.
    float khco_cw[3];
    kh_cloth_l2w(khco_cl, khco_c.centre, khco_c.rot, khco_c.rotated, khco_es, khco_cw);
    // The hit triangle's OUTWARD normal, carried to world through the same
    // frame. Inside and outside come from the winding alone: a collider must be
    // a closed, consistently wound mesh (meshgen::bake winds every triangle to
    // its authored normal, so an outward-normalled model arrives outward-wound;
    // an open or inside-out one pushes cloth the wrong way, undetected). A
    // non-uniform size takes the inverse transpose: for a diagonal scale, divide
    // by size where the point multiplied by it.
    const float* v = &khco_c.bvh->vp[static_cast<size_t>(khco_t) * 9u];
    float khco_vv[9];
    if (khco_def) {   // KH_SKEL: the hit triangle where the query found it.
        const float* khco_dv = khco_vd + static_cast<size_t>(khco_t) * 9u;
        for (int j = 0; j < 9; ++j) khco_vv[j] = v[j] + khco_s1 * khco_dv[j];
        v = khco_vv;
    }
    float e1[3], e2[3], nl[3];
    kh_cloth_v3_sub(v + 3, v, e1);
    kh_cloth_v3_sub(v + 6, v, e2);
    nl[0] = e1[1] * e2[2] - e1[2] * e2[1];
    nl[1] = e1[2] * e2[0] - e1[0] * e2[2];
    nl[2] = e1[0] * e2[1] - e1[1] * e2[0];
    for (int k = 0; k < 3; ++k) nl[k] *= khco_ies[k];
    float nw[3];
    if (khco_c.rotated) {
        for (int k = 0; k < 3; ++k) {
            nw[k] = khco_c.rot[k] * nl[0] + khco_c.rot[3 + k] * nl[1] + khco_c.rot[6 + k] * nl[2];
        }
    } else { nw[0] = nl[0]; nw[1] = nl[1]; nw[2] = nl[2]; }
    const float nlen = kh_cloth_v3_len(nw);
    if (nlen < 1.0e-12f) return false;   // Degenerate triangle: no side to be on.
    for (int k = 0; k < 3; ++k) nw[k] /= nlen;
    if (khco_c.inside) for (int k = 0; k < 3; ++k) nw[k] = -nw[k];

    const float khco_want = khco_thick + khco_c.margin;
    float khco_dv[3];
    kh_cloth_v3_sub(khco_p, khco_cw, khco_dv);
    if (khco_reg == 0) {
        // In a face's interior the face normal IS the gradient of the
        // distance, so the contact is the plane: signed depth along it.
        const float khco_sd = kh_cloth_v3_dot(khco_dv, nw);
        if (khco_sd >= khco_want) {                       // Already clear on the outside.
            if (khco_cc) { memcpy(khco_cc->l, khco_lm, sizeof(khco_cc->l)); khco_cc->d = khco_d; khco_cc->acc = khco_acc; }   // KH_COL_CLEAR.
            return false;
        }
        const float khco_push = khco_want - khco_sd;
        for (int k = 0; k < 3; ++k) khco_p[k] += nw[k] * khco_push;
        khco_n_out[0] = nw[0]; khco_n_out[1] = nw[1]; khco_n_out[2] = nw[2];
    } else {
        // On an edge or a corner the face normal is neither the side nor the
        // direction. The side is the pseudonormal's (kh_physics_bvh_build),
        // tested in local space where the closest point was found (the sign
        // survives the size's positive scale). The direction is from the surface
        // to the particle, the distance's gradient there, so a contact rounds the
        // edge at exactly the standoff instead of being pushed along one face's
        // normal.
        const float* khco_pn = (khco_c.bvh->pn.size() >= (static_cast<size_t>(khco_t) + 1u) * 18u)
                             ? &khco_c.bvh->pn[static_cast<size_t>(khco_t) * 18u +
                                               (khco_reg <= 3 ? (khco_reg - 1) * 3 : 9 + (khco_reg - 4) * 3)]
                             : nl;
        float khco_lv[3];
        kh_cloth_v3_sub(khco_l, khco_cl, khco_lv);
        const bool khco_out = kh_cloth_v3_dot(khco_lv, khco_pn) >= 0.0f;   // Outside the mesh.
        const bool khco_ok = khco_c.inside ? !khco_out : khco_out;          // On the side cloth belongs.
        const float khco_dl = kh_cloth_v3_len(khco_dv);
        float khco_u[3];
        float khco_sd;
        if (khco_dl > 1.0e-7f) {
            const float khco_s = khco_ok ? 1.0f / khco_dl : -1.0f / khco_dl;
            for (int k = 0; k < 3; ++k) khco_u[k] = khco_dv[k] * khco_s;
            khco_sd = khco_ok ? khco_dl : -khco_dl;
        } else {
            // On the surface itself: no direction to read, so the
            // pseudonormal's, carried to world the way the face normal is.
            float khco_pl[3] = { khco_pn[0] * khco_ies[0], khco_pn[1] * khco_ies[1],
                                 khco_pn[2] * khco_ies[2] };
            if (khco_c.rotated) {
                for (int k = 0; k < 3; ++k) {
                    khco_u[k] = khco_c.rot[k] * khco_pl[0] + khco_c.rot[3 + k] * khco_pl[1] + khco_c.rot[6 + k] * khco_pl[2];
                }
            } else { khco_u[0] = khco_pl[0]; khco_u[1] = khco_pl[1]; khco_u[2] = khco_pl[2]; }
            const float khco_ul = kh_cloth_v3_len(khco_u);
            if (khco_ul < 1.0e-12f) { khco_u[0] = nw[0]; khco_u[1] = nw[1]; khco_u[2] = nw[2]; }
            else {
                const float khco_s = khco_c.inside ? -1.0f / khco_ul : 1.0f / khco_ul;
                for (int k = 0; k < 3; ++k) khco_u[k] *= khco_s;
            }
            khco_sd = 0.0f;
        }
        if (khco_sd >= khco_want) {   // Outside (khco_sd is positive on the cloth's side alone).
            if (khco_cc) { memcpy(khco_cc->l, khco_lm, sizeof(khco_cc->l)); khco_cc->d = khco_d; khco_cc->acc = khco_acc; }   // KH_COL_CLEAR.
            return false;
        }
        for (int k = 0; k < 3; ++k) khco_p[k] = khco_cw[k] + khco_u[k] * khco_want;
        khco_n_out[0] = khco_u[0]; khco_n_out[1] = khco_u[1]; khco_n_out[2] = khco_u[2];
    }
    // Where the contact point was at the start of the substep. Zero for a
    // still collider, and for a view whose prev was never filled.
    khco_sv_out[0] = khco_sv_out[1] = khco_sv_out[2] = 0.0f;
    if (khco_c.prev.size[0] != 0.0f || khco_c.prev.size[1] != 0.0f || khco_c.prev.size[2] != 0.0f) {
        float khco_cp[3];
        if (khco_def) {
            // KH_SKEL: the SAME point of the surface at the substep's start (the
            // contact's barycentric place on the hit triangle, carried back by
            // those corners' own motion), so the surface's motion under the
            // contact is the limb's as well as the root's, and friction carries
            // cloth with a swing.
            float khco_b[3] = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };
            {
                float khco_q[3];
                kh_cloth_v3_sub(khco_cl, v, khco_q);
                const float khco_d00 = kh_cloth_v3_dot(e1, e1), khco_d01 = kh_cloth_v3_dot(e1, e2);
                const float khco_d11 = kh_cloth_v3_dot(e2, e2);
                const float khco_d20 = kh_cloth_v3_dot(khco_q, e1), khco_d21 = kh_cloth_v3_dot(khco_q, e2);
                const float khco_den = khco_d00 * khco_d11 - khco_d01 * khco_d01;
                if (khco_den > 1.0e-20f) {
                    float khco_bv = (khco_d11 * khco_d20 - khco_d01 * khco_d21) / khco_den;
                    float khco_bw = (khco_d00 * khco_d21 - khco_d01 * khco_d20) / khco_den;
                    float khco_bu = 1.0f - khco_bv - khco_bw;
                    khco_bu = khco_bu > 0.0f ? khco_bu : 0.0f;
                    khco_bv = khco_bv > 0.0f ? khco_bv : 0.0f;
                    khco_bw = khco_bw > 0.0f ? khco_bw : 0.0f;
                    const float khco_bs = khco_bu + khco_bv + khco_bw;
                    if (khco_bs > 1.0e-12f) { khco_b[0] = khco_bu / khco_bs; khco_b[1] = khco_bv / khco_bs; khco_b[2] = khco_bw / khco_bs; }
                }
            }
            const float* khco_dv = khco_vd + static_cast<size_t>(khco_t) * 9u;
            const float khco_ds = khco_c.def0 - khco_s1;   // <= 0: back to the substep's start.
            float khco_c0[3];
            for (int k = 0; k < 3; ++k) {
                khco_c0[k] = khco_cl[k] + khco_ds * (khco_b[0] * khco_dv[k] + khco_b[1] * khco_dv[3 + k] + khco_b[2] * khco_dv[6 + k]);
            }
            kh_cloth_l2w(khco_c0, khco_c.prev.centre, khco_c.prev.rot, khco_c.prev.rotated, khco_pes, khco_cp);
        } else {
            kh_cloth_l2w(khco_cl, khco_c.prev.centre, khco_c.prev.rot, khco_c.prev.rotated, khco_pes, khco_cp);
        }
        for (int k = 0; k < 3; ++k) khco_sv_out[k] = khco_cw[k] - khco_cp[k];
    }
    return true;
}

// KH_COL_CLEAR: one collider column at one substep - its surface-motion sum
// advanced along the pose chain. False = the column's records are void (the
// caller empties them): not a cacheable collider, a pose that does not chain
// from the last one used, or the sum at its restart.
inline bool kh_col_clear_advance(KhColClearCol& khca_c, const KhPhysicsColliderView& khca_v) {
    const KhPhysicsBvh* const b = khca_v.bvh;
    const bool khca_ok = KH_COL_CLEAR_ON && b && !b->empty() && b->pose_seq != 0u && !khca_v.inside &&
                         !((khca_v.convex || b->cvx_only) && b->cvx_valid());
    // The sweep fraction the query reads the surface at (kh_physics_collide_one's khco_def).
    const bool khca_def = khca_ok && khca_v.deform && !b->vd.empty() && b->vd.size() == b->vp.size();
    const float khca_s = khca_def ? khca_v.def1 : 0.0f;
    bool khca_chain = khca_ok && khca_c.on;
    if (khca_chain) {
        if (khca_c.seq == b->pose_seq) khca_c.acc += fabsf(khca_s - khca_c.s) * b->vd_max;
        else if (b->prev_seq != 0u && b->prev_seq == khca_c.seq) khca_c.acc += fabsf(khca_c.s) * khca_c.vd_max + (1.0f + khca_s) * b->vd_max;
        else khca_chain = false;
        if (khca_chain && !(khca_c.acc < KH_COL_CLEAR_ACC_MAX)) khca_chain = false;
    }
    if (!khca_chain) khca_c.acc = 0.0f;
    khca_c.on = khca_ok;
    khca_c.seq = khca_ok ? b->pose_seq : 0u;
    khca_c.s = khca_s;
    khca_c.vd_max = khca_ok ? b->vd_max : 0.0f;
    return khca_chain;
}
// Nearest pinned particle along the mesh, for every particle, plus that path's
// rest length. A multi-source Dijkstra over the stretch edges - not a BFS,
// because edge rest lengths differ and a hop count is not a distance.
inline void kh_cloth_build_tethers(std::vector<KhClothPart>& khct_part,
                                   const std::vector<KhClothDist>& khct_edge) {
    const size_t khct_n = khct_part.size();
    if (khct_n == 0) return;
    std::vector<uint32_t> khct_head(khct_n + 1u, 0u);
    for (size_t e = 0; e < khct_edge.size(); ++e) {
        if (khct_edge[e].a >= khct_n || khct_edge[e].b >= khct_n) continue;
        ++khct_head[khct_edge[e].a + 1u];
        ++khct_head[khct_edge[e].b + 1u];
    }
    for (size_t i = 0; i < khct_n; ++i) khct_head[i + 1u] += khct_head[i];
    std::vector<uint32_t> khct_adj(khct_head[khct_n]);
    std::vector<float>    khct_len(khct_head[khct_n]);
    {
        std::vector<uint32_t> khct_fill(khct_head.begin(), khct_head.end() - 1);
        for (size_t e = 0; e < khct_edge.size(); ++e) {
            const uint32_t a = khct_edge[e].a, b = khct_edge[e].b;
            if (a >= khct_n || b >= khct_n) continue;
            khct_adj[khct_fill[a]] = b; khct_len[khct_fill[a]] = khct_edge[e].rest; ++khct_fill[a];
            khct_adj[khct_fill[b]] = a; khct_len[khct_fill[b]] = khct_edge[e].rest; ++khct_fill[b];
        }
    }
    std::vector<float> khct_d(khct_n, 1.0e30f);
    struct KhCtNode { float d; uint32_t v; };
    struct KhCtWorse { bool operator()(const KhCtNode& x, const KhCtNode& y) const { return x.d > y.d; } };
    std::vector<KhCtNode> khct_q;
    for (size_t i = 0; i < khct_n; ++i) {
        khct_part[i].anchor = static_cast<uint32_t>(i);
        khct_part[i].tether = -1.0f;
        if (khct_part[i].w <= 0.0f) {
            khct_d[i] = 0.0f;
            khct_q.push_back(KhCtNode{ 0.0f, static_cast<uint32_t>(i) });
        }
    }
    std::make_heap(khct_q.begin(), khct_q.end(), KhCtWorse());
    while (!khct_q.empty()) {
        std::pop_heap(khct_q.begin(), khct_q.end(), KhCtWorse());
        const KhCtNode khct_c = khct_q.back();
        khct_q.pop_back();
        if (khct_c.d > khct_d[khct_c.v] + 1.0e-9f) continue;   // A stale heap entry.
        for (uint32_t e = khct_head[khct_c.v]; e < khct_head[khct_c.v + 1u]; ++e) {
            const uint32_t khct_to = khct_adj[e];
            const float khct_nd = khct_c.d + khct_len[e];
            if (khct_nd + 1.0e-9f >= khct_d[khct_to]) continue;
            khct_d[khct_to] = khct_nd;
            khct_part[khct_to].anchor = khct_part[khct_c.v].anchor;
            khct_q.push_back(KhCtNode{ khct_nd, khct_to });
            std::push_heap(khct_q.begin(), khct_q.end(), KhCtWorse());
        }
    }
    for (size_t i = 0; i < khct_n; ++i) {
        if (khct_part[i].w > 0.0f && khct_d[i] < 1.0e29f) khct_part[i].tether = khct_d[i];
    }
}

// KH_SKEL - particle i's rigid-follow target, mesh-local, at fraction t of the
// step's span: the skeleton's guide from where the last substep ended (guide_a,
// KH_SUB_CLOCK) to the frame's end (guide_b), or the rest position when there
// is none. khct_tmp receives an interpolated target; read the pointer returned.
inline const float* kh_cloth_target(const KhClothState& khct_s, size_t khct_i, float khct_t, float* khct_tmp) {
    if (khct_s.guide_b.size() != khct_s.part.size() * 3u || khct_s.guide_a.size() != khct_s.guide_b.size()) {
        return khct_s.part[khct_i].rest;
    }
    const float* khct_b = &khct_s.guide_b[khct_i * 3u];
    if (khct_t >= 1.0f) return khct_b;
    const float* khct_a = &khct_s.guide_a[khct_i * 3u];
    for (int k = 0; k < 3; ++k) khct_tmp[k] = khct_a[k] + (khct_b[k] - khct_a[k]) * khct_t;
    return khct_tmp;
}

// Place every particle at its rigid-follow target in the current frame - its
// rest pose, or where a skeleton's guide puts it (kh_cloth_target) - and clear
// all motion. The cold start, and the recovery from every pathology.
inline void kh_cloth_reset(KhClothState& khcr_s) {
    for (size_t i = 0; i < khcr_s.part.size(); ++i) {
        KhClothPart& khcr_p = khcr_s.part[i];
        float khcr_g[3];
        kh_cloth_l2w(kh_cloth_target(khcr_s, i, khcr_s.guide_t1, khcr_g), khcr_s.centre, khcr_s.rot, khcr_s.rotated,
                     khcr_s.size, khcr_p.p);
        for (int k = 0; k < 3; ++k) khcr_p.pp[k] = khcr_p.p[k];
    }
    khcr_s.accum = 0.0f;
    khcr_s.h_last = 0.0f;   // KH_SIM_LOD: no displacement stands, so no length was read.
    khcr_s.primed = true;
    khcr_s.carrier_vok = false;   // What moved the cloth here is not a velocity.
    khcr_s.carrier_vreb = false;   // KH_TELEPORT_ONCE.
}

// Carry the whole particle set rigidly from one frame to another. A teleport is
// not motion: the cloth arrives with the object instead of being whipped across
// the map by a one-frame velocity, and p AND pp move together, so its internal
// shape is preserved.
inline void kh_cloth_rebase(KhClothState& khcb_s,
                            const float* khcb_c0, const float* khcb_r0, bool khcb_rot0, const float* khcb_is0,
                            const float* khcb_c1, const float* khcb_r1, bool khcb_rot1, const float* khcb_sz1) {
    for (size_t i = 0; i < khcb_s.part.size(); ++i) {
        KhClothPart& khcb_p = khcb_s.part[i];
        float khcb_l[3], khcb_lp[3];
        kh_cloth_w2l(khcb_p.p,  khcb_c0, khcb_r0, khcb_rot0, khcb_is0, khcb_l);
        kh_cloth_w2l(khcb_p.pp, khcb_c0, khcb_r0, khcb_rot0, khcb_is0, khcb_lp);
        kh_cloth_l2w(khcb_l,  khcb_c1, khcb_r1, khcb_rot1, khcb_sz1, khcb_p.p);
        kh_cloth_l2w(khcb_lp, khcb_c1, khcb_r1, khcb_rot1, khcb_sz1, khcb_p.pp);
    }
}

// How far past the separation a self-collision pair is gathered, as a multiple
// of it (kh_cloth_self_gather).
static constexpr float KH_CLOTH_SELF_REACH = 1.5f;
// The most relative motion per substep the reach allows for, in mean rest
// edges (kh_cloth_self_gather).
static constexpr float KH_CLOTH_SELF_MOTION = 2.0f;
// KH_CLOTH_SELF_WIDE: the most hash cells a wide face or edge is filed into
// (kh_cloth_self_gather); one covering more takes no part that substep.
static constexpr uint32_t KH_CLOTH_SELF_WIDE_CELLS = 1024u;
// A crossing deeper than this in one substep, in mean rest edges, is not
// pushed back (kh_cloth_self_solve).
static constexpr float KH_CLOTH_SELF_DEEP = 0.5f;
// The jolt stand-down (kh_cloth_step): a change of velocity at any corner of
// the carrier's box above this in one frame (m/s; no physical carrier comes
// near it - a 30 g crash is 5 m/s a frame at 60 fps) turns self-collision off
// for this long (s).
static constexpr float KH_CLOTH_SELF_JOLT = 10.0f;
static constexpr float KH_CLOTH_SELF_HOLD = 1.0f;

// Closest points of segments p0-p1 and q0-q1 (Ericson 5.1.9): the parameters
// along each, s and t in [0, 1], and the points. Parallel segments take s = 0
// and the matching t, which is one of the nearest pairs.
inline void kh_cloth_closest_seg(const float* p0, const float* p1, const float* q0, const float* q1,
                                 float& s, float& t, float* cp, float* cq) {
    float d1[3], d2[3], r[3];
    kh_cloth_v3_sub(p1, p0, d1);
    kh_cloth_v3_sub(q1, q0, d2);
    kh_cloth_v3_sub(p0, q0, r);
    const float a = kh_cloth_v3_dot(d1, d1), e = kh_cloth_v3_dot(d2, d2), f = kh_cloth_v3_dot(d2, r);
    auto clamp01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
    if (a <= 1.0e-20f && e <= 1.0e-20f) {
        s = 0.0f; t = 0.0f;
    } else if (a <= 1.0e-20f) {
        s = 0.0f; t = clamp01(f / e);
    } else {
        const float c = kh_cloth_v3_dot(d1, r);
        if (e <= 1.0e-20f) {
            t = 0.0f; s = clamp01(-c / a);
        } else {
            const float b = kh_cloth_v3_dot(d1, d2);
            const float den = a * e - b * b;
            s = den > 1.0e-10f * a * e ? clamp01((b * f - c * e) / den) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f)      { t = 0.0f; s = clamp01(-c / a); }
            else if (t > 1.0f) { t = 1.0f; s = clamp01((b - c) / a); }
        }
    }
    for (int k = 0; k < 3; ++k) { cp[k] = p0[k] + d1[k] * s; cq[k] = q0[k] + d2[k] * t; }
}

// Whether segment p-q passes through triangle a-b-c (Moller-Trumbore), ends
// and rim included.
inline bool kh_cloth_seg_tri(const float* p, const float* q, const float* a, const float* b, const float* c) {
    float e1[3], e2[3], d[3], s[3], h[3], k[3];
    kh_cloth_v3_sub(b, a, e1); kh_cloth_v3_sub(c, a, e2); kh_cloth_v3_sub(q, p, d);
    h[0] = d[1] * e2[2] - d[2] * e2[1]; h[1] = d[2] * e2[0] - d[0] * e2[2]; h[2] = d[0] * e2[1] - d[1] * e2[0];
    const float det = kh_cloth_v3_dot(e1, h);
    if (!(fabsf(det) > 1.0e-20f)) return false;   // Parallel, or no area.
    const float inv = 1.0f / det;
    kh_cloth_v3_sub(p, a, s);
    const float u = kh_cloth_v3_dot(s, h) * inv;
    if (!(u >= 0.0f && u <= 1.0f)) return false;
    k[0] = s[1] * e1[2] - s[2] * e1[1]; k[1] = s[2] * e1[0] - s[0] * e1[2]; k[2] = s[0] * e1[1] - s[1] * e1[0];
    const float v = kh_cloth_v3_dot(d, k) * inv;
    if (!(v >= 0.0f && u + v <= 1.0f)) return false;
    const float t = kh_cloth_v3_dot(e2, k) * inv;
    return t >= 0.0f && t <= 1.0f;
}

// SELF-COLLISION, the gather. Every pair of the same cloth that can touch this
// substep, found once after the prediction and solved with everything else in
// the iterations (kh_cloth_self_solve). Two kinds (Bridson et al. 2002): a
// particle against a face, and an edge against an edge - the second stops the
// straight edge BETWEEN two held particles cutting through another layer.
// Only a pair that shares a particle is excluded (a particle and a face it is
// a corner of, two edges with an end in common): a wider topological exclusion
// leaves the small triangles of an irregular mesh unguarded. Neighbours along
// one surface are handled from the rest pose instead (below), which scales with the mesh.
// A particle / face pair's SIDE is the side of the face's plane the particle
// started the substep on (pp): a single sheet has no inside, so only its
// history says which side it belongs on. The side is consulted only while the
// particle stands over the face; off it, the solve pushes away from the nearest
// edge or corner, which needs no side. An edge pair keeps the direction between
// its nearest points at the start in the same role.
//
// KH_CLOTH_FORK - a fork-join on the cloth pool's own workers. The posters: a
// cloth job's self-collision gather (kh_cloth_self_gather), kh_attach_step's
// skin fork (KH_SKIN_FORK) and the render thread's per-object loops
// (kh_fork_each, KH_INJ_FORK). A poster with a loop of independent chunks posts
// it here; every idle pool worker (a solver worker waiting on g_cloth_cv, or a
// skin worker on g_skin_cv - KH_POOL_SPLIT - with no job of its own queue to
// take) takes chunks until none is left, and the poster's own thread takes
// chunks too, then waits for the last helper's to finish. A worker with a job
// of its own never helps (its own queue's jobs come first in kh_cloth_worker):
// a heavy cloth borrows only the threads that would otherwise sleep.
// A chunk may touch the initiator's instance, read-only, plus scratch indexed
// by the CHUNK (its own output) or by the PARTICIPANT id it was handed (its own
// stamps) - never anything another chunk writes. The fork state is one global
// record; every handout is one atomic word carrying the generation, the chunk
// count AND the chunk (gen << 40 | n << 20 | chunk), so what a helper compares
// its chunk against came from the same word: a helper that wakes late and takes
// a word from a finished fork sees a foreign generation (or the zero the fork's
// end leaves) and takes nothing, and a valid handout of generation g proves
// fork g still in flight (its count cannot complete without this chunk), so the
// fn, ctx and cap it then reads are g's (the next post needs g's join first).
// No chunk runs twice, and nothing of a finished fork is read after its
// initiator returned.
// Participant ids past the scratch capacity take no chunk (the initiator runs
// what is left); the initiator is always participant 0. Chunks never block,
// take a lock or wait on anything: that keeps helping deadlock-free. The wake
// and the pool size are the runtime's (kh_cloth_fork_wake,
// kh_cloth_fork_workers); the core only posts.
// ONE FORK AT A TIME. The record is one: two jobs posting at once would write
// each other's count and counter, and one would wait for ever on a total reset
// under it, its instance's busy flag never cleared. A job takes g_cloth_fork_mu
// for the whole of its fork (kh_cloth_fork_run); a job that finds it taken runs
// its chunks serially on its own thread instead of waiting (no worker idles on another's fork; the result is the
// same: a chunk's output does not depend on who ran it). Helpers never take the
// mutex. The only lock taken inside it is g_cloth_mu, by the wake, momentarily;
// nothing blocks on g_cloth_fork_mu while holding another lock (kh_attach_step's
// skin fork only try-locks it, under g_draw_list_mutex).
struct KhClothFork {
    std::atomic<uint64_t> next{ 0 };        // gen << 40 | n << 20 | the next chunk to hand out; 0 between forks.
    std::atomic<uint32_t> gen_posted{ 0 };  // The generation posted; 0 = none.
    std::atomic<uint32_t> done{ 0 };        // Chunks finished (a chunk that threw counts: it is over).
    std::atomic<uint32_t> failed{ 0 };      // Chunks that threw; the initiator rethrows for them after the join.
    std::atomic<uint32_t> pids{ 1 };        // The next participant id (0 is the initiator's).
    uint32_t n = 0;                         // Chunks (also in every handout word).
    // Participant ids with scratch. Atomic: a helper that read the previous
    // fork's generation may read it while the next fork's initiator writes it
    // (a stale value only admits or refuses a pid it never runs a chunk with).
    std::atomic<uint32_t> cap{ 0 };
    uint32_t gen = 0;
    void (*fn)(void*, uint32_t, uint32_t) = nullptr;   // (ctx, chunk, participant).
    void* ctx = nullptr;
    // KH_SKIN_FORK: a helper leaves this fork between chunks for a queued job of
    // its own pool's queue - a cloth step, or a skin job for a skin worker (the
    // initiator finishes what is left).
    std::atomic<bool> yield_cloth{ false };
    // KH_FORK_PRIO: the initiator's thread priority. A helper runs its chunks
    // at it when that is higher than its own, so the render thread never
    // waits on a chunk whose worker a same-priority thread preempted
    // (priority inversion); restored when the helper leaves the fork.
    std::atomic<int> prio{ THREAD_PRIORITY_NORMAL };
};
static KhClothFork g_cloth_fork;
// Cloth steps queued and not yet taken (g_cloth_q's size, kept beside it under
// g_cloth_mu): read lock-free by a yielding helper.
static std::atomic<uint32_t> g_cloth_q_n{ 0 };
static std::atomic<uint32_t> g_skin_q_n{ 0 };    // KH_POOL_SPLIT: g_skin_q's size, the same way, for a skin helper's yield.
static std::mutex g_cloth_fork_mu;   // Held by the one job whose fork is posted.
static constexpr uint32_t KH_CLOTH_FORK_MAXN = (1u << 20) - 1u;   // Chunks a handout word can name.
inline uint32_t kh_cloth_fork_gen_of(uint64_t khfg_v) { return static_cast<uint32_t>(khfg_v >> 40); }
inline uint32_t kh_cloth_fork_n_of(uint64_t khfg_v)   { return static_cast<uint32_t>(khfg_v >> 20) & KH_CLOTH_FORK_MAXN; }
inline uint32_t kh_cloth_fork_i_of(uint64_t khfg_v)   { return static_cast<uint32_t>(khfg_v) & KH_CLOTH_FORK_MAXN; }
// An idle worker's helping: chunks until none is left. Returns the generation
// it served (0 = none was posted): a worker serves a generation ONCE and then
// waits for the next, never re-entering a fork it has left (a re-entry would
// take a fresh participant id for no chunk, and a worker looping on a
// still-posted fork would spin for the fork's whole length).
// KH_POOL_SPLIT: khfh_skin = the helper is a skin worker; it yields to a queued
// skin job as a solver worker yields to a queued cloth step.
inline uint32_t kh_cloth_fork_help(uint32_t khfh_served, bool khfh_skin = false) {
    const uint32_t khfh_g = g_cloth_fork.gen_posted.load(std::memory_order_acquire);
    if (khfh_g == 0u || khfh_g == khfh_served) return khfh_served;
    const uint32_t khfh_pid = g_cloth_fork.pids.fetch_add(1u, std::memory_order_acq_rel);
    if (khfh_pid >= g_cloth_fork.cap.load(std::memory_order_relaxed)) return khfh_g;   // No scratch for one more: the initiator finishes.
    // KH_FORK_PRIO: raised for the fork, restored on every way out below.
    const HANDLE khfh_th = GetCurrentThread();
    const int khfh_own = GetThreadPriority(khfh_th);
    const int khfh_want = g_cloth_fork.prio.load(std::memory_order_relaxed);
    const bool khfh_boost = khfh_own != THREAD_PRIORITY_ERROR_RETURN && khfh_want != THREAD_PRIORITY_ERROR_RETURN &&
                            khfh_want > khfh_own && SetThreadPriority(khfh_th, khfh_want) != 0;
    struct KhFhPrio {
        HANDLE th; int own; bool on;
        ~KhFhPrio() { if (on) SetThreadPriority(th, own); }
    } khfh_restore{ khfh_th, khfh_own, khfh_boost };
    for (;;) {
        if (g_cloth_fork.yield_cloth.load(std::memory_order_acquire) &&
            (khfh_skin ? g_skin_q_n : g_cloth_q_n).load(std::memory_order_relaxed) != 0u) break;   // KH_SKIN_FORK: a job of this worker's own queue waits.
        // KH_FORK_CAS: a chunk is claimed only from this generation's word. Do
        // not use a plain fetch_add here: a helper preempted between its last
        // chunk and the add (the previous initiator sees every chunk done,
        // returns, a new fork posts) would take the NEXT fork's chunk 0 and
        // leave on the generation test without running it, and the new
        // initiator waits for ever.
        uint64_t khfh_v = g_cloth_fork.next.load(std::memory_order_acquire);
        if (kh_cloth_fork_gen_of(khfh_v) != khfh_g) break;   // Another fork's word, or none: nothing of this one is left.
        const uint32_t khfh_i = kh_cloth_fork_i_of(khfh_v);
        if (khfh_i >= kh_cloth_fork_n_of(khfh_v)) break;   // The count of THIS word, never the record's.
        if (!g_cloth_fork.next.compare_exchange_weak(khfh_v, khfh_v + 1u, std::memory_order_acq_rel,
                                                     std::memory_order_acquire)) continue;   // Raced: look again.
        // A chunk that throws (an allocation) must still count as finished, or
        // the initiator waits for ever; kh_cloth_fork_run rethrows on the
        // initiator's thread - for the gather, a cloth job's worker, where
        // kh_cloth_worker's catch clears the instance's busy flag.
        try { g_cloth_fork.fn(g_cloth_fork.ctx, khfh_i, khfh_pid); }
        catch (...) { g_cloth_fork.failed.fetch_add(1u, std::memory_order_acq_rel); }
        g_cloth_fork.done.fetch_add(1u, std::memory_order_acq_rel);
    }
    return khfh_g;
}
// The initiator's side: chunks 0..n-1 of fn, in parallel when the pool can
// help, serially (in order, as participant 0) when it cannot. Returns once
// every chunk has finished.
inline void kh_cloth_fork_run(uint32_t khfr_n, uint32_t khfr_cap, void (*khfr_fn)(void*, uint32_t, uint32_t), void* khfr_ctx,
                              bool khfr_yield = false) {
    // n must stay below KH_CLOTH_FORK_MAXN: the initiator's last add leaves the
    // index at n + 1, which at n == MAXN would carry into the word's count and
    // hand chunk 0 out again.
    if (khfr_n <= 1u || khfr_n >= KH_CLOTH_FORK_MAXN || khfr_cap < 2u || !g_cloth_fork_mu.try_lock()) {   // Small, alone, or another job's fork is up: serial.
        for (uint32_t khfr_i = 0; khfr_i < khfr_n; ++khfr_i) khfr_fn(khfr_ctx, khfr_i, 0u);
        return;
    }
    std::lock_guard<std::mutex> khfr_hold(g_cloth_fork_mu, std::adopt_lock);   // Released when the fork is done.
    KhClothFork& khfr_f = g_cloth_fork;
    khfr_f.fn = khfr_fn;
    khfr_f.ctx = khfr_ctx;
    khfr_f.n = khfr_n;
    khfr_f.cap.store(khfr_cap, std::memory_order_relaxed);   // Published by gen_posted's release.
    khfr_f.done.store(0u, std::memory_order_relaxed);
    khfr_f.failed.store(0u, std::memory_order_relaxed);
    khfr_f.pids.store(1u, std::memory_order_relaxed);
    khfr_f.yield_cloth.store(khfr_yield, std::memory_order_relaxed);   // Published by gen_posted's release.
    khfr_f.prio.store(GetThreadPriority(GetCurrentThread()), std::memory_order_relaxed);   // KH_FORK_PRIO.
    khfr_f.gen = (khfr_f.gen + 1u) & 0xFFFFFFu;   // 24 bits, never 0 (0 is 'no fork').
    if (khfr_f.gen == 0u) khfr_f.gen = 1u;
    const uint32_t khfr_g = khfr_f.gen;
    khfr_f.next.store((static_cast<uint64_t>(khfr_g) << 40) | (static_cast<uint64_t>(khfr_n) << 20), std::memory_order_release);
    khfr_f.gen_posted.store(khfr_g, std::memory_order_release);
    if (khfr_yield) kh_cloth_fork_wake_try(); else kh_cloth_fork_wake();
    for (;;) {
        const uint64_t khfr_v = khfr_f.next.fetch_add(1u, std::memory_order_acq_rel);
        const uint32_t khfr_i = kh_cloth_fork_i_of(khfr_v);
        if (kh_cloth_fork_gen_of(khfr_v) != khfr_g || khfr_i >= khfr_n) break;
        try { khfr_fn(khfr_ctx, khfr_i, 0u); }
        catch (...) { khfr_f.failed.fetch_add(1u, std::memory_order_acq_rel); }
        khfr_f.done.fetch_add(1u, std::memory_order_acq_rel);
    }
    while (khfr_f.done.load(std::memory_order_acquire) < khfr_n) std::this_thread::yield();
    khfr_f.next.store(0u, std::memory_order_release);   // Every later handout reads 'no fork'.
    khfr_f.gen_posted.store(0u, std::memory_order_release);
    if (khfr_f.failed.load(std::memory_order_acquire) != 0u) throw std::bad_alloc();   // The one thing a chunk can throw (a list growing); <new> is in every unit.
}
// A chunk lambda through the fork's plain function pointer.
template <class KhFcF> inline void kh_cloth_fork_call(void* khfc_c, uint32_t khfc_i, uint32_t khfc_p) {
    (*static_cast<KhFcF*>(khfc_c))(khfc_i, khfc_p);
}
static constexpr uint32_t KH_FORK_EACH_CHUNK = 64u;
template <class KhFeF> inline void kh_fork_each(uint32_t khfe_n, KhFeF&& khfe_fn) {
    const uint32_t khfe_w = kh_cloth_fork_workers();
    if (khfe_n < KH_FORK_EACH_MIN || khfe_w < 2u) {
        for (uint32_t khfe_i = 0; khfe_i < khfe_n; ++khfe_i) khfe_fn(khfe_i);
        return;
    }
    const uint32_t khfe_chunks = (khfe_n + KH_FORK_EACH_CHUNK - 1u) / KH_FORK_EACH_CHUNK;
    auto khfe_body = [&](uint32_t khfe_c, uint32_t) {
        const uint32_t khfe_a = khfe_c * KH_FORK_EACH_CHUNK;
        const uint32_t khfe_b = khfe_a + KH_FORK_EACH_CHUNK < khfe_n ? khfe_a + KH_FORK_EACH_CHUNK : khfe_n;
        for (uint32_t khfe_i = khfe_a; khfe_i < khfe_b; ++khfe_i) khfe_fn(khfe_i);
    };
    // yield: the wake is the non-blocking one (it never waits on g_cloth_mu,
    // which the game thread holds across a chain build) and a helper leaves for a
    // queued job of its own pool's queue (a cloth step, or a skin job for a skin
    // worker); either way the caller runs what is left.
    kh_cloth_fork_run(khfe_chunks, khfe_w, &kh_cloth_fork_call<decltype(khfe_body)>, &khfe_body, true);
}
// Elements (faces + edges) from which the gather forks; below it the chunking
// costs more than it saves. Queries per chunk.
static constexpr size_t   KH_CLOTH_FORK_MIN   = 4096u;
static constexpr uint32_t KH_CLOTH_FORK_CHUNK = 256u;

// A pair is taken if its two ends started within KH_CLOTH_SELF_REACH times the
// separation plus their predicted relative motion. The reach exceeds one
// separation because the constraint solve carries particles further than the
// prediction: a pair pushed inside the separation while unlisted is pushed out
// next substep, a bounce that never sleeps. The gather's cost grows with the
// square of the reach. The motion term is capped at KH_CLOTH_SELF_MOTION mean
// edges a substep: it is unbounded when the carrier jumps (pins moving far
// against free cloth that has not) and would list every pair within that of
// each other; a crossing faster than the cap is not caught, and nothing near
// it is a fold.
// A pair already near in the REST pose (within twice the separation) is one of
// two things. A rest offset mostly along the face's normal is a layer the mesh
// was made with (an authored fold, a shell's other surface): it holds its rest
// distance if that is nearer than the dial. An offset mostly in the face's
// plane is a neighbour along the same surface: it holds half its rest distance,
// so at rest it stands at twice its separation, out of contact (so it never resists a wrinkle), cannot
// be folded through, and is continuous with the pairs beyond it. The offset's
// direction decides, not where the foot lands: a layer whose vertices sit
// exactly over the other's puts every foot on a corner.
// TANGLES. Anything that gets through (a crossing faster than the cap, or one
// made while self-collision is stood down - kh_cloth_step) is taken as the
// cloth's shape by the next substep's sides and held by every pair around it.
// So each substep first finds every edge that passes through a face at its
// start and marks the particles of both; a pair whose two elements are both
// marked is not a pair, so the cloth can come apart where it passes through
// itself and is held everywhere else. This lets a tangle go, it does not pull
// it out (Baraff et al. 2003 do both).
// The broadphase is one uniform hash over the faces' and edges' boxes. The
// boxes span the substep's motion LESS the cloth's mean motion (a pair only
// meets through relative motion). The cell is the mean edge or twice the
// separation, grown to a quarter of the largest ordinary box, so no ordinary
// element spans more than a handful of cells; a face or edge larger than two
// mean edges whose box reaches past an ordinary element's (KH_CLOTH_SELF_WIDE)
// spans as many as its box covers, up to KH_CLOTH_SELF_WIDE_CELLS.
inline void kh_cloth_self_gather(KhClothState& khsg_s, float khsg_thick) {
    khsg_s.self_c.clear();
    khsg_s.self_e.clear();
    const size_t khsg_n = khsg_s.part.size();
    const size_t khsg_nt = khsg_s.tri.size();
    if (khsg_n == 0 || khsg_nt == 0 || !(khsg_thick > 0.0f)) return;

    // The surface's edges, each with a face it borders: topology, so once.
    if (khsg_s.self_edges_nt != khsg_nt) {
        khsg_s.self_edges.clear();
        std::unordered_map<uint64_t, uint8_t> khsg_have;
        khsg_have.reserve(khsg_nt * 2u);
        for (size_t t = 0; t < khsg_nt; ++t) {
            const uint32_t khsg_v[3] = { khsg_s.tri[t].a, khsg_s.tri[t].b, khsg_s.tri[t].c };
            if (khsg_v[0] >= khsg_n || khsg_v[1] >= khsg_n || khsg_v[2] >= khsg_n) continue;
            for (int e = 0; e < 3; ++e) {
                const uint32_t x = khsg_v[e], y = khsg_v[(e + 1) % 3];
                const uint64_t khsg_k = x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
                if (!khsg_have.emplace(khsg_k, 1u).second) continue;
                KhClothSelfEdge khsg_ed;
                khsg_ed.a = x; khsg_ed.b = y; khsg_ed.f = static_cast<uint32_t>(t);
                khsg_s.self_edges.push_back(khsg_ed);
            }
        }
        khsg_s.self_edges_nt = static_cast<uint32_t>(khsg_nt);
    }
    const size_t khsg_ne = khsg_s.self_edges.size();

    // Metric rest positions, rebuilt only when the size moves.
    if (khsg_s.self_rest.size() != khsg_n * 3u || khsg_s.self_rest_size[0] != khsg_s.size[0] ||
        khsg_s.self_rest_size[1] != khsg_s.size[1] || khsg_s.self_rest_size[2] != khsg_s.size[2]) {
        khsg_s.self_rest.resize(khsg_n * 3u);
        for (size_t i = 0; i < khsg_n; ++i) {
            for (int k = 0; k < 3; ++k) khsg_s.self_rest[i * 3u + k] = khsg_s.part[i].rest[k] * khsg_s.size[k];
        }
        for (int k = 0; k < 3; ++k) khsg_s.self_rest_size[k] = khsg_s.size[k];
        double khsg_sum = 0.0;
        for (size_t e = 0; e < khsg_ne; ++e) {
            float khsg_d[3];
            kh_cloth_v3_sub(&khsg_s.self_rest[khsg_s.self_edges[e].b * 3u], &khsg_s.self_rest[khsg_s.self_edges[e].a * 3u], khsg_d);
            khsg_sum += kh_cloth_v3_len(khsg_d);
        }
        khsg_s.self_edge = khsg_ne ? static_cast<float>(khsg_sum / static_cast<double>(khsg_ne)) : 0.0f;
    }
    auto khsg_rp = [&khsg_s](uint32_t i) { return &khsg_s.self_rest[i * 3u]; };
    auto khsg_cross = [](const float* x, const float* y, float* o) {
        o[0] = x[1] * y[2] - x[2] * y[1]; o[1] = x[2] * y[0] - x[0] * y[2]; o[2] = x[0] * y[1] - x[1] * y[0];
    };
    auto khsg_fnormal = [&](const KhClothTri& khsg_t, bool khsg_rest, float* o) {
        const float* a = khsg_rest ? khsg_rp(khsg_t.a) : khsg_s.part[khsg_t.a].pp;
        const float* b = khsg_rest ? khsg_rp(khsg_t.b) : khsg_s.part[khsg_t.b].pp;
        const float* c = khsg_rest ? khsg_rp(khsg_t.c) : khsg_s.part[khsg_t.c].pp;
        float e1[3], e2[3];
        kh_cloth_v3_sub(b, a, e1); kh_cloth_v3_sub(c, a, e2);
        khsg_cross(e1, e2, o);
    };
    // The separation a pair holds, from its rest offset and a normal of the
    // surface there; negative = not a pair.
    auto khsg_sep = [khsg_thick](const float* khsg_rd, float khsg_rl, const float* khsg_fn) -> float {
        if (khsg_rl >= 2.0f * khsg_thick) return khsg_thick;
        if (khsg_rl < 1.0e-4f) return -1.0f;   // Coincident at rest: no direction to hold.
        const float khsg_fl = kh_cloth_v3_len(khsg_fn);
        const bool khsg_layer = khsg_fl > 1.0e-12f &&
                                fabsf(kh_cloth_v3_dot(khsg_rd, khsg_fn)) >= 0.7071f * khsg_rl * khsg_fl;
        if (!khsg_layer) return 0.5f * khsg_rl;   // A neighbour along the surface.
        return khsg_rl < khsg_thick ? khsg_rl : khsg_thick;
    };
    const float khsg_mvcap = KH_CLOTH_SELF_MOTION * fmaxf(khsg_s.self_edge, khsg_thick);
    auto khsg_dv = [&khsg_s](uint32_t x, uint32_t y, float* o) {   // Relative motion of x against y this substep.
        for (int k = 0; k < 3; ++k) o[k] = (khsg_s.part[x].p[k] - khsg_s.part[x].pp[k]) - (khsg_s.part[y].p[k] - khsg_s.part[y].pp[k]);
    };

    // The cloth's mean motion this substep, taken out of every box below.
    double khsg_md[3] = { 0.0, 0.0, 0.0 };
    for (size_t i = 0; i < khsg_n; ++i) {
        for (int k = 0; k < 3; ++k) khsg_md[k] += khsg_s.part[i].p[k] - khsg_s.part[i].pp[k];
    }
    const float khsg_d[3] = { static_cast<float>(khsg_md[0] / static_cast<double>(khsg_n)),
                              static_cast<float>(khsg_md[1] / static_cast<double>(khsg_n)),
                              static_cast<float>(khsg_md[2] / static_cast<double>(khsg_n)) };
    auto khsg_boxof = [&](const uint32_t* khsg_v, int khsg_nv, float khsg_grow, float* khsg_b) {
        for (int k = 0; k < 3; ++k) { khsg_b[k] = 3.0e38f; khsg_b[3 + k] = -3.0e38f; }
        for (int j = 0; j < khsg_nv; ++j) {
            const KhClothPart& khsg_p = khsg_s.part[khsg_v[j]];
            for (int k = 0; k < 3; ++k) {
                const float khsg_x0 = khsg_p.pp[k], khsg_x1 = khsg_p.p[k] - khsg_d[k];
                khsg_b[k] = fminf(khsg_b[k], fminf(khsg_x0, khsg_x1));
                khsg_b[3 + k] = fmaxf(khsg_b[3 + k], fmaxf(khsg_x0, khsg_x1));
            }
        }
        for (int k = 0; k < 3; ++k) { khsg_b[k] -= khsg_grow; khsg_b[3 + k] += khsg_grow; }
    };

    // Boxes: the faces, then the edges - their own sweeps, not grown. The reach
    // is added to the QUERY's box instead (the same overlap test; each element
    // sits in as few cells as it covers). An element sweeping further than the
    // motion the reach allows for, beyond its own size, is left out of this
    // substep: the reach could not list a pair for it anyway.
    // KH_CLOTH_SELF_WIDE: the own size is the larger of the element's two ends'
    // extents on each axis, never taken below two mean edges. Do not take two
    // mean edges for every element: a face larger than that standing still is
    // then left out, and another layer passes through it. A WIDE element (a box
    // past what an ordinary one can reach) is filed into every cell its box
    // covers, at the cell the ordinary elements set: it adds entries of its own
    // and grows the cell for no query. One covering more than
    // KH_CLOTH_SELF_WIDE_CELLS cells is left out.
    const float khsg_fast = khsg_mvcap + 2.0f * khsg_s.self_edge;   // The ordinary element's reach.
    auto khsg_toofast = [&](const float* khsg_b, const float* khsg_own) {
        for (int k = 0; k < 3; ++k) {
            if (khsg_b[3 + k] - khsg_b[k] > khsg_mvcap + fmaxf(khsg_own[k], 2.0f * khsg_s.self_edge)) return true;
        }
        return false;
    };
    auto khsg_ownof = [&](const uint32_t* khsg_v, int khsg_nv, float* khsg_o) {
        for (int k = 0; k < 3; ++k) {
            float khsg_a0 = 3.0e38f, khsg_a1 = -3.0e38f, khsg_b0 = 3.0e38f, khsg_b1 = -3.0e38f;
            for (int j = 0; j < khsg_nv; ++j) {
                const KhClothPart& khsg_p = khsg_s.part[khsg_v[j]];
                const float khsg_x0 = khsg_p.pp[k], khsg_x1 = khsg_p.p[k] - khsg_d[k];
                khsg_a0 = fminf(khsg_a0, khsg_x0); khsg_a1 = fmaxf(khsg_a1, khsg_x0);
                khsg_b0 = fminf(khsg_b0, khsg_x1); khsg_b1 = fmaxf(khsg_b1, khsg_x1);
            }
            khsg_o[k] = fmaxf(khsg_a1 - khsg_a0, khsg_b1 - khsg_b0);
        }
    };
    const float khsg_own0[3] = { 0.0f, 0.0f, 0.0f };   // A particle's.
    const size_t khsg_nel = khsg_nt + khsg_ne;
    khsg_s.self_box.resize(khsg_nel * 6u);
    khsg_s.self_wide.clear();
    float khsg_ext = 0.0f;
    auto khsg_place = [&](size_t khsg_el, const uint32_t* khsg_v, int khsg_nv) {
        float* khsg_b = &khsg_s.self_box[khsg_el * 6u];
        float khsg_own[3];
        khsg_boxof(khsg_v, khsg_nv, 0.0f, khsg_b);
        if (khsg_b[3] - khsg_b[0] <= khsg_fast && khsg_b[4] - khsg_b[1] <= khsg_fast &&
            khsg_b[5] - khsg_b[2] <= khsg_fast) {   // The ordinary element (the tests below would pass it).
            for (int k = 0; k < 3; ++k) khsg_ext = fmaxf(khsg_ext, khsg_b[3 + k] - khsg_b[k]);
            return;
        }
        khsg_ownof(khsg_v, khsg_nv, khsg_own);
        if (khsg_toofast(khsg_b, khsg_own)) {
            for (int k = 0; k < 3; ++k) { khsg_b[k] = 1.0f; khsg_b[3 + k] = -1.0f; }
            return;
        }
        if (khsg_b[3] - khsg_b[0] > khsg_fast || khsg_b[4] - khsg_b[1] > khsg_fast ||
            khsg_b[5] - khsg_b[2] > khsg_fast) {
            khsg_s.self_wide.push_back(static_cast<uint32_t>(khsg_el));   // Filed once the cell is known.
            return;
        }
        for (int k = 0; k < 3; ++k) khsg_ext = fmaxf(khsg_ext, khsg_b[3 + k] - khsg_b[k]);
    };
    for (size_t t = 0; t < khsg_nt; ++t) {
        float* khsg_b = &khsg_s.self_box[t * 6u];
        const uint32_t khsg_v[3] = { khsg_s.tri[t].a, khsg_s.tri[t].b, khsg_s.tri[t].c };
        if (khsg_v[0] >= khsg_n || khsg_v[1] >= khsg_n || khsg_v[2] >= khsg_n) {
            for (int k = 0; k < 3; ++k) { khsg_b[k] = 1.0f; khsg_b[3 + k] = -1.0f; }   // Empty: never overlaps.
            continue;
        }
        khsg_place(t, khsg_v, 3);
    }
    for (size_t e = 0; e < khsg_ne; ++e) {
        const uint32_t khsg_v[2] = { khsg_s.self_edges[e].a, khsg_s.self_edges[e].b };
        khsg_place(khsg_nt + e, khsg_v, 2);
    }
    float khsg_cell = fmaxf(khsg_s.self_edge, 2.0f * khsg_thick);
    khsg_cell = fmaxf(khsg_cell, 0.25f * khsg_ext);
    if (!(khsg_cell > 1.0e-4f)) khsg_cell = 1.0e-4f;
    const float khsg_ic = 1.0f / khsg_cell;
    auto khsg_cells = [khsg_ic](const float* khsg_b, int* khsg_lo, int* khsg_hi) {
        for (int k = 0; k < 3; ++k) {
            khsg_lo[k] = static_cast<int>(floorf(khsg_b[k] * khsg_ic));
            khsg_hi[k] = static_cast<int>(floorf(khsg_b[3 + k] * khsg_ic));
        }
    };
    // KH_CLOTH_SELF_WIDE: the wide elements' cells at that cell; over the cap, left out. The table grows by what
    // they add, so the ordinary elements' buckets stay as sparse as without them.
    // KH_CLOTH_SELF_WIDE_BOUND: a wide element stands still at any size, so a runaway solve short of the non-finite
    // rescue can hand one a box past int range in cells (the conversion is UB) or cells whose product wraps int64
    // back under the cap (the table grows to its limit and the fill walks billions of cells). Each axis's cells are
    // held within +-2e9 (short of int range, so the fill's increments stay defined) and against the cap before the
    // product - the same verdict for every box inside that bound whose cells are in order; a box outside that bound is
    // left out as one over the cap is.
    size_t khsg_wide_n = 0u;
    for (uint32_t khsg_w : khsg_s.self_wide) {
        float* khsg_b = &khsg_s.self_box[static_cast<size_t>(khsg_w) * 6u];
        int64_t khsg_wc = 1;
        for (int k = 0; k < 3 && khsg_wc > 0; ++k) {
            const float khsg_l = floorf(khsg_b[k] * khsg_ic), khsg_h = floorf(khsg_b[3 + k] * khsg_ic);
            if (!(khsg_l > -2.0e9f && khsg_l < 2.0e9f && khsg_h > -2.0e9f && khsg_h < 2.0e9f)) {   // NaN too.
                khsg_wc = 0;
                break;
            }
            const int64_t khsg_n = static_cast<int64_t>(static_cast<int>(khsg_h)) - static_cast<int>(khsg_l) + 1;
            khsg_wc = khsg_n > static_cast<int64_t>(KH_CLOTH_SELF_WIDE_CELLS) ? 0 : khsg_wc * khsg_n;   // n < 1: out.
        }
        if (khsg_wc <= 0 || khsg_wc > static_cast<int64_t>(KH_CLOTH_SELF_WIDE_CELLS)) {
            for (int k = 0; k < 3; ++k) { khsg_b[k] = 1.0f; khsg_b[3 + k] = -1.0f; }
            continue;
        }
        khsg_wide_n += static_cast<size_t>(khsg_wc);
    }

    uint32_t khsg_tn = 256u;
    while (khsg_tn < (khsg_nel + khsg_wide_n) * 2u && khsg_tn < (1u << 24)) khsg_tn <<= 1;
    const uint32_t khsg_mask = khsg_tn - 1u;
    auto khsg_hash = [khsg_mask](int x, int y, int z) -> uint32_t {
        return ((static_cast<uint32_t>(x) * 73856093u) ^ (static_cast<uint32_t>(y) * 19349663u) ^
                (static_cast<uint32_t>(z) * 83492791u)) & khsg_mask;
    };

    // Counting sort of (element, cell) into the table: count, prefix, fill.
    // After the fill each head is its bucket's start and the next its end.
    khsg_s.self_head.assign(static_cast<size_t>(khsg_tn) + 1u, 0u);
    for (int khsg_pass = 0; khsg_pass < 2; ++khsg_pass) {
        if (khsg_pass == 1) {
            for (uint32_t b = 1; b <= khsg_tn; ++b) khsg_s.self_head[b] += khsg_s.self_head[b - 1u];
            khsg_s.self_ent.resize(khsg_s.self_head[khsg_tn]);
        }
        for (size_t el = 0; el < khsg_nel; ++el) {
            const float* khsg_b = &khsg_s.self_box[el * 6u];
            if (khsg_b[0] > khsg_b[3]) continue;
            int lo[3], hi[3];
            khsg_cells(khsg_b, lo, hi);
            for (int x = lo[0]; x <= hi[0]; ++x) for (int y = lo[1]; y <= hi[1]; ++y) for (int z = lo[2]; z <= hi[2]; ++z) {
                const uint32_t khsg_bk = khsg_hash(x, y, z);
                if (khsg_pass == 0) ++khsg_s.self_head[khsg_bk];
                else khsg_s.self_ent[--khsg_s.self_head[khsg_bk]] = static_cast<uint32_t>(el);
            }
        }
    }
    // An element in several cells, or two cells on one bucket, is met more than
    // once by a query; the stamp (the querying particle, n + the querying edge,
    // or n + edges + the tangle test's edge, plus one) takes it once.
    // KH_CLOTH_FORK: the three query loops below run in chunks of
    // KH_CLOTH_FORK_CHUNK queries, across the pool when the element count earns
    // it; each participant stamps its own copy of the table (stamps are unique
    // per query, so the copies never disagree). A chunk's pairs go to its own
    // list and the lists are joined in chunk order, so the pair lists - and the
    // solve's sweep order - are exactly the serial loop's. self_tang is the
    // participants' marks joined.
    const uint32_t khsg_cap0 = kh_cloth_fork_workers();
    const bool     khsg_par = khsg_cap0 >= 2u && khsg_nel >= KH_CLOTH_FORK_MIN;
    const uint32_t khsg_cap = khsg_par ? khsg_cap0 : 1u;
    if (khsg_s.self_seen_par.size() < khsg_cap) khsg_s.self_seen_par.resize(khsg_cap);
    if (khsg_s.self_tang_par.size() < khsg_cap) khsg_s.self_tang_par.resize(khsg_cap);
    for (uint32_t khsg_pi = 0; khsg_pi < khsg_cap; ++khsg_pi) {
        khsg_s.self_seen_par[khsg_pi].assign(khsg_nel, 0u);
        khsg_s.self_tang_par[khsg_pi].assign(khsg_n, 0u);
    }
    auto khsg_chunks = [khsg_par](size_t khsg_nq) -> uint32_t {
        if (!khsg_par || khsg_nq == 0u) return 1u;
        return static_cast<uint32_t>((khsg_nq + KH_CLOTH_FORK_CHUNK - 1u) / KH_CLOTH_FORK_CHUNK);
    };
    auto khsg_range = [khsg_par](size_t khsg_nq, uint32_t khsg_c, uint32_t& khsg_lo, uint32_t& khsg_hi) {
        if (!khsg_par) { khsg_lo = 0u; khsg_hi = static_cast<uint32_t>(khsg_nq); return; }
        khsg_lo = khsg_c * KH_CLOTH_FORK_CHUNK;
        const size_t khsg_e = static_cast<size_t>(khsg_lo) + KH_CLOTH_FORK_CHUNK;
        khsg_hi = static_cast<uint32_t>(khsg_e < khsg_nq ? khsg_e : khsg_nq);
    };
    auto khsg_overlap = [](const float* x, const float* y) {
        return !(x[0] > y[3] || x[3] < y[0] || x[1] > y[4] || x[4] < y[1] || x[2] > y[5] || x[5] < y[2]);
    };
    // Every element numbered in [khsg_el0, khsg_el1) in the cells of box
    // khsg_qb, once, handed to khsg_fn. The range is tested first: a query
    // for faces walks past the edges in its cells, and an edge past the
    // edges it has already been paired with, for one integer compare each.
    auto khsg_query = [&](uint32_t* khsg_seen, const float* khsg_qb, uint32_t khsg_stamp, uint32_t khsg_el0, uint32_t khsg_el1, auto&& khsg_fn) {
        int lo[3], hi[3];
        khsg_cells(khsg_qb, lo, hi);
        for (int x = lo[0]; x <= hi[0]; ++x) for (int y = lo[1]; y <= hi[1]; ++y) for (int z = lo[2]; z <= hi[2]; ++z) {
            const uint32_t khsg_bk = khsg_hash(x, y, z);
            for (uint32_t e = khsg_s.self_head[khsg_bk]; e < khsg_s.self_head[khsg_bk + 1u]; ++e) {
                const uint32_t el = khsg_s.self_ent[e];
                if (el < khsg_el0 || el >= khsg_el1) continue;
                if (khsg_seen[el] == khsg_stamp) continue;
                khsg_seen[el] = khsg_stamp;
                if (khsg_overlap(khsg_qb, &khsg_s.self_box[el * 6u])) khsg_fn(el);
            }
        }
    };

    // Tangles (above): the particles of every edge through a face at the start.
    {
        auto khsg_tangle = [&](uint32_t khsg_c, uint32_t khsg_pid) {
            uint32_t* const khsg_seen = khsg_s.self_seen_par[khsg_pid].data();
            uint8_t* const khsg_tang = khsg_s.self_tang_par[khsg_pid].data();
            uint32_t khsg_lo, khsg_hi;
            khsg_range(khsg_ne, khsg_c, khsg_lo, khsg_hi);
            for (uint32_t e = khsg_lo; e < khsg_hi; ++e) {
                const float* const khsg_eb = &khsg_s.self_box[(khsg_nt + e) * 6u];
                if (khsg_eb[0] > khsg_eb[3]) continue;
                const KhClothSelfEdge& E = khsg_s.self_edges[e];
                khsg_query(khsg_seen, khsg_eb, static_cast<uint32_t>(khsg_n + khsg_ne) + e + 1u, 0u, static_cast<uint32_t>(khsg_nt), [&](uint32_t t) {
                    const KhClothTri& T = khsg_s.tri[t];
                    if (E.a == T.a || E.a == T.b || E.a == T.c || E.b == T.a || E.b == T.b || E.b == T.c) return;
                    if (!kh_cloth_seg_tri(khsg_s.part[E.a].pp, khsg_s.part[E.b].pp,
                                          khsg_s.part[T.a].pp, khsg_s.part[T.b].pp, khsg_s.part[T.c].pp)) return;
                    khsg_tang[E.a] = khsg_tang[E.b] = 1u;
                    khsg_tang[T.a] = khsg_tang[T.b] = khsg_tang[T.c] = 1u;
                });
            }
        };
        kh_cloth_fork_run(khsg_chunks(khsg_ne), khsg_cap, &kh_cloth_fork_call<decltype(khsg_tangle)>, &khsg_tangle);
        // The marks, joined: participant 0's array is the one every test reads.
        khsg_s.self_tang.swap(khsg_s.self_tang_par[0]);
        for (uint32_t khsg_pi = 1; khsg_pi < khsg_cap; ++khsg_pi) {
            const std::vector<uint8_t>& khsg_t = khsg_s.self_tang_par[khsg_pi];
            for (size_t i = 0; i < khsg_n; ++i) khsg_s.self_tang[i] |= khsg_t[i];
        }
    }
    auto khsg_tg = [&khsg_s](uint32_t x) { return khsg_s.self_tang[x] != 0u; };

    // Particle against face.
    {
    const uint32_t khsg_nc = khsg_chunks(khsg_n);
    if (khsg_s.self_c_par.size() < khsg_nc) khsg_s.self_c_par.resize(khsg_nc);
    auto khsg_pf = [&](uint32_t khsg_c, uint32_t khsg_pid) {
    uint32_t* const khsg_seen = khsg_s.self_seen_par[khsg_pid].data();
    std::vector<KhClothSelfC>& khsg_out = khsg_s.self_c_par[khsg_c];
    khsg_out.clear();
    uint32_t khsg_lo, khsg_hi;
    khsg_range(khsg_n, khsg_c, khsg_lo, khsg_hi);
    for (uint32_t q = khsg_lo; q < khsg_hi; ++q) {
        const KhClothPart& Q = khsg_s.part[q];
        float khsg_qb[6];
        khsg_boxof(&q, 1, 0.0f, khsg_qb);
        if (khsg_toofast(khsg_qb, khsg_own0)) continue;
        for (int k = 0; k < 3; ++k) { khsg_qb[k] -= KH_CLOTH_SELF_REACH * khsg_thick; khsg_qb[3 + k] += KH_CLOTH_SELF_REACH * khsg_thick; }
        khsg_query(khsg_seen, khsg_qb, q + 1u, 0u, static_cast<uint32_t>(khsg_nt), [&](uint32_t t) {
            const KhClothTri& khsg_t = khsg_s.tri[t];
            if (q == khsg_t.a || q == khsg_t.b || q == khsg_t.c) return;
            const KhClothPart& A = khsg_s.part[khsg_t.a];
            const KhClothPart& B = khsg_s.part[khsg_t.b];
            const KhClothPart& C = khsg_s.part[khsg_t.c];
            if (Q.w + A.w + B.w + C.w <= 0.0f) return;   // Nothing here can move.
            if (khsg_tg(q) && (khsg_tg(khsg_t.a) || khsg_tg(khsg_t.b) || khsg_tg(khsg_t.c))) return;   // In a tangle.
            float khsg_n0[3];
            khsg_fnormal(khsg_t, false, khsg_n0);
            const float khsg_nl = kh_cloth_v3_len(khsg_n0);
            if (!(khsg_nl > 1.0e-12f)) return;   // No area at the start: no plane, no side.
            float khsg_mv = 0.0f;
            const uint32_t khsg_tv[3] = { khsg_t.a, khsg_t.b, khsg_t.c };
            for (int j = 0; j < 3; ++j) { float r[3]; khsg_dv(q, khsg_tv[j], r); khsg_mv = fmaxf(khsg_mv, kh_cloth_v3_len(r)); }
            float khsg_cp[3], khsg_qd[3];
            int khsg_reg = 0;
            kh_cloth_closest_tri(Q.pp, A.pp, B.pp, C.pp, khsg_cp, khsg_reg);
            kh_cloth_v3_sub(Q.pp, khsg_cp, khsg_qd);
            if (kh_cloth_v3_len(khsg_qd) > KH_CLOTH_SELF_REACH * khsg_thick + fminf(khsg_mv, khsg_mvcap)) return;
            float khsg_sp = khsg_thick;
            {
                float khsg_fn[3];
                khsg_fnormal(khsg_t, true, khsg_fn);
                if (kh_cloth_v3_len(khsg_fn) > 1.0e-12f) {
                    float khsg_rcp[3], khsg_rd[3];
                    int khsg_rr = 0;
                    kh_cloth_closest_tri(khsg_rp(q), khsg_rp(khsg_t.a), khsg_rp(khsg_t.b), khsg_rp(khsg_t.c), khsg_rcp, khsg_rr);
                    kh_cloth_v3_sub(khsg_rp(q), khsg_rcp, khsg_rd);
                    khsg_sp = khsg_sep(khsg_rd, kh_cloth_v3_len(khsg_rd), khsg_fn);
                    if (khsg_sp < 0.0f) return;
                }
            }
            float khsg_ap[3];
            kh_cloth_v3_sub(Q.pp, A.pp, khsg_ap);
            KhClothSelfC khsg_pc;
            khsg_pc.q = q;
            khsg_pc.t = t;
            const float khsg_s0 = kh_cloth_v3_dot(khsg_ap, khsg_n0) / khsg_nl;
            khsg_pc.side = khsg_s0 < 0.0f ? -1.0f : 1.0f;
            khsg_pc.sep = khsg_sp;
            khsg_pc.dep = 0.0f;
            khsg_pc.s0 = fabsf(khsg_s0);
            khsg_out.push_back(khsg_pc);
        });
    }
    };
    kh_cloth_fork_run(khsg_nc, khsg_cap, &kh_cloth_fork_call<decltype(khsg_pf)>, &khsg_pf);
    if (khsg_nc == 1u) khsg_s.self_c.swap(khsg_s.self_c_par[0]);
    else for (uint32_t khsg_c = 0; khsg_c < khsg_nc; ++khsg_c) {
        khsg_s.self_c.insert(khsg_s.self_c.end(), khsg_s.self_c_par[khsg_c].begin(), khsg_s.self_c_par[khsg_c].end());
    }
    }

    // Edge against edge, each pair once (the second index the larger).
    {
    const uint32_t khsg_nc = khsg_chunks(khsg_ne);
    if (khsg_s.self_e_par.size() < khsg_nc) khsg_s.self_e_par.resize(khsg_nc);
    auto khsg_ee = [&](uint32_t khsg_c, uint32_t khsg_pid) {
    uint32_t* const khsg_seen = khsg_s.self_seen_par[khsg_pid].data();
    std::vector<KhClothSelfE>& khsg_out = khsg_s.self_e_par[khsg_c];
    khsg_out.clear();
    uint32_t khsg_lo, khsg_hi;
    khsg_range(khsg_ne, khsg_c, khsg_lo, khsg_hi);
    for (uint32_t e0 = khsg_lo; e0 < khsg_hi; ++e0) {
        const KhClothSelfEdge& E0 = khsg_s.self_edges[e0];
        float* const khsg_qb = &khsg_s.self_box[(khsg_nt + e0) * 6u];   // Its own box; empty when too fast.
        if (khsg_qb[0] > khsg_qb[3]) continue;
        float khsg_qg[6];
        for (int k = 0; k < 3; ++k) { khsg_qg[k] = khsg_qb[k] - KH_CLOTH_SELF_REACH * khsg_thick; khsg_qg[3 + k] = khsg_qb[3 + k] + KH_CLOTH_SELF_REACH * khsg_thick; }
        khsg_query(khsg_seen, khsg_qg, static_cast<uint32_t>(khsg_n) + e0 + 1u, static_cast<uint32_t>(khsg_nt) + e0 + 1u,
                   static_cast<uint32_t>(khsg_nel), [&](uint32_t el) {
            const uint32_t e1 = el - static_cast<uint32_t>(khsg_nt);
            const KhClothSelfEdge& E1 = khsg_s.self_edges[e1];
            if (E1.a == E0.a || E1.a == E0.b || E1.b == E0.a || E1.b == E0.b) return;
            const KhClothPart& A = khsg_s.part[E0.a];
            const KhClothPart& B = khsg_s.part[E0.b];
            const KhClothPart& C = khsg_s.part[E1.a];
            const KhClothPart& D = khsg_s.part[E1.b];
            if (A.w + B.w + C.w + D.w <= 0.0f) return;
            if ((khsg_tg(E0.a) || khsg_tg(E0.b)) && (khsg_tg(E1.a) || khsg_tg(E1.b))) return;   // In a tangle.
            float khsg_mv = 0.0f;
            const uint32_t khsg_x[2] = { E0.a, E0.b }, khsg_y[2] = { E1.a, E1.b };
            for (int i = 0; i < 2; ++i) for (int j = 0; j < 2; ++j) {
                float r[3]; khsg_dv(khsg_x[i], khsg_y[j], r); khsg_mv = fmaxf(khsg_mv, kh_cloth_v3_len(r));
            }
            float s, t, khsg_c0[3], khsg_c1[3], khsg_n0[3];
            kh_cloth_closest_seg(A.pp, B.pp, C.pp, D.pp, s, t, khsg_c0, khsg_c1);
            kh_cloth_v3_sub(khsg_c0, khsg_c1, khsg_n0);
            const float khsg_nl = kh_cloth_v3_len(khsg_n0);
            if (khsg_nl > KH_CLOTH_SELF_REACH * khsg_thick + fminf(khsg_mv, khsg_mvcap)) return;
            if (!(khsg_nl > 1.0e-7f)) return;   // Touching at the start: no direction to keep.
            float khsg_sp = khsg_thick;
            {
                float khsg_r0[3], khsg_r1[3], khsg_rd[3], khsg_fn[3];
                kh_cloth_closest_seg(khsg_rp(E0.a), khsg_rp(E0.b), khsg_rp(E1.a), khsg_rp(E1.b), s, t, khsg_r0, khsg_r1);
                kh_cloth_v3_sub(khsg_r0, khsg_r1, khsg_rd);
                khsg_fnormal(khsg_s.tri[E0.f], true, khsg_fn);
                khsg_sp = khsg_sep(khsg_rd, kh_cloth_v3_len(khsg_rd), khsg_fn);
                if (khsg_sp < 0.0f) return;
            }
            KhClothSelfE khsg_pe;
            khsg_pe.e0 = e0;
            khsg_pe.e1 = e1;
            for (int k = 0; k < 3; ++k) khsg_pe.n[k] = khsg_n0[k] / khsg_nl;
            khsg_pe.sep = khsg_sp;
            khsg_pe.dep = 0.0f;
            khsg_pe.d0 = khsg_nl;
            khsg_out.push_back(khsg_pe);
        });
    }
    };
    kh_cloth_fork_run(khsg_nc, khsg_cap, &kh_cloth_fork_call<decltype(khsg_ee)>, &khsg_ee);
    if (khsg_nc == 1u) khsg_s.self_e.swap(khsg_s.self_e_par[0]);
    else for (uint32_t khsg_c = 0; khsg_c < khsg_nc; ++khsg_c) {
        khsg_s.self_e.insert(khsg_s.self_e.end(), khsg_s.self_e_par[khsg_c].begin(), khsg_s.self_e_par[khsg_c].end());
    }
    }
}

// SELF-COLLISION, one Gauss-Seidel sweep over this substep's pairs.
// A particle / face pair is in contact when the particle is nearer the face
// than 'sep'. Over the face, 'nearer' is measured along the face's normal on
// the particle's own side, so one that crossed this substep is pushed back; off
// the face it is the distance to the nearest edge or corner and the push is
// straight away from it, so cloth folded over another layer's open edge rounds
// it at the separation instead of sagging through the corner. The two agree
// where they meet. An edge pair is in contact when its nearest points are
// nearer than 'sep' and is pushed apart along the line between them - or, once
// that line has turned against its starting direction because the edges passed
// through each other, back along that starting direction.
// Ending up on the other side is not proof of passing THROUGH: a particle can
// go round a face's edge, or two edges past each other's ends. A pair found on
// the far side is interpolated back to the moment it crossed (positions linear
// over the substep from pp) and is a crossing only if the particle was over the
// face there (for two edges: both nearest points inside both edges), within
// 'sep'; otherwise it went round, and its present side becomes its side. A
// crossing deeper than KH_CLOTH_SELF_DEEP mean edges in one substep is left
// where it is: only a yank drives one layer that far through another, and
// pushing it back would fight the tethers pulling it through.
// The correction (Bridson et al. 2002, as a PBD constraint): the direction held
// fixed for the step, each side's corners moving by their barycentric shares of
// the nearest point, all by inverse mass, so momentum is kept and a pinned side
// does not move - and no corner moves further than the correction itself (the
// bare shares divide by the barycentric weight, which flings the free corner of
// a nearly pinned face); what such a face cannot take stays unresolved. With
// khss_mu > 0 the sweep also applies each contact's friction once, from the
// separation it restored over the whole substep: Coulomb, as the collider pass,
// on the relative motion of the two nearest points.
inline void kh_cloth_self_solve(KhClothState& khss_s, float khss_mu) {
    // One contact, whatever its kind: points x (weights wx) against points y
    // (weights wy) along the unit direction g, at separation h.
    auto khss_apply = [khss_mu](KhClothPart* const* x, const float* wx, int nx, KhClothPart* const* y,
                                const float* wy, int ny, const float* g, float h, float sep, float& dep) {
        float W = 0.0f, Wd = 0.0f;
        for (int i = 0; i < nx; ++i) { W += wx[i] * wx[i] * x[i]->w; Wd = fmaxf(Wd, wx[i] * x[i]->w); }
        for (int i = 0; i < ny; ++i) { W += wy[i] * wy[i] * y[i]->w; Wd = fmaxf(Wd, wy[i] * y[i]->w); }
        if (!(W > 0.0f)) return;
        Wd = fmaxf(Wd, W);   // Divides every move: no corner goes further than the correction.
        if (h < sep) {
            const float corr = sep - h;
            const float l = corr / Wd;
            for (int i = 0; i < nx; ++i) for (int k = 0; k < 3; ++k) x[i]->p[k] += g[k] * l * wx[i] * x[i]->w;
            for (int i = 0; i < ny; ++i) for (int k = 0; k < 3; ++k) y[i]->p[k] -= g[k] * l * wy[i] * y[i]->w;
            dep += corr * (W / Wd);   // What the pair actually separated.
        }
        if (khss_mu <= 0.0f || !(dep > 0.0f)) return;
        float m[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < nx; ++i) for (int k = 0; k < 3; ++k) m[k] += wx[i] * (x[i]->p[k] - x[i]->pp[k]);
        for (int i = 0; i < ny; ++i) for (int k = 0; k < 3; ++k) m[k] -= wy[i] * (y[i]->p[k] - y[i]->pp[k]);
        const float mn = kh_cloth_v3_dot(m, g);
        float tn[3];
        for (int k = 0; k < 3; ++k) tn[k] = m[k] - g[k] * mn;
        const float tl = kh_cloth_v3_len(tn);
        if (tl < 1.0e-12f) return;
        const float cone = khss_mu * dep;
        const float stop = (tl <= cone ? 1.0f : cone / tl) / Wd;   // Share of it removed, per Wd.
        for (int i = 0; i < nx; ++i) for (int k = 0; k < 3; ++k) x[i]->p[k] -= tn[k] * stop * wx[i] * x[i]->w;
        for (int i = 0; i < ny; ++i) for (int k = 0; k < 3; ++k) y[i]->p[k] += tn[k] * stop * wy[i] * y[i]->w;
    };

    for (size_t i = 0; i < khss_s.self_c.size(); ++i) {
        KhClothSelfC& khss_c = khss_s.self_c[i];
        const KhClothTri& khss_t = khss_s.tri[khss_c.t];
        KhClothPart* const khss_q[1] = { &khss_s.part[khss_c.q] };
        KhClothPart* const khss_f[3] = { &khss_s.part[khss_t.a], &khss_s.part[khss_t.b], &khss_s.part[khss_t.c] };
        float khss_e1[3], khss_e2[3], khss_n[3];
        kh_cloth_v3_sub(khss_f[1]->p, khss_f[0]->p, khss_e1);
        kh_cloth_v3_sub(khss_f[2]->p, khss_f[0]->p, khss_e2);
        khss_n[0] = khss_e1[1] * khss_e2[2] - khss_e1[2] * khss_e2[1];
        khss_n[1] = khss_e1[2] * khss_e2[0] - khss_e1[0] * khss_e2[2];
        khss_n[2] = khss_e1[0] * khss_e2[1] - khss_e1[1] * khss_e2[0];
        const float khss_nl = kh_cloth_v3_len(khss_n);
        if (!(khss_nl > 1.0e-12f)) continue;
        float khss_cp[3];
        int khss_reg = 0;
        kh_cloth_closest_tri(khss_q[0]->p, khss_f[0]->p, khss_f[1]->p, khss_f[2]->p, khss_cp, khss_reg);
        // The nearest point's barycentric shares, which weight the corners.
        float khss_d[3];
        kh_cloth_v3_sub(khss_cp, khss_f[0]->p, khss_d);
        const float d00 = kh_cloth_v3_dot(khss_e1, khss_e1), d01 = kh_cloth_v3_dot(khss_e1, khss_e2);
        const float d11 = kh_cloth_v3_dot(khss_e2, khss_e2);
        const float d20 = kh_cloth_v3_dot(khss_d, khss_e1), d21 = kh_cloth_v3_dot(khss_d, khss_e2);
        const float khss_den = d00 * d11 - d01 * d01;
        if (!(khss_den > 0.0f)) continue;
        float v = (d11 * d20 - d01 * d21) / khss_den;
        float w = (d00 * d21 - d01 * d20) / khss_den;
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        w = w < 0.0f ? 0.0f : (w > 1.0f - v ? 1.0f - v : w);
        const float khss_bw[3] = { 1.0f - v - w, v, w };
        const float khss_one[1] = { 1.0f };
        float khss_g[3], khss_h, khss_qd[3];
        kh_cloth_v3_sub(khss_q[0]->p, khss_cp, khss_qd);
        if (khss_reg == 0) {
            for (int k = 0; k < 3; ++k) khss_g[k] = khss_n[k] / khss_nl * khss_c.side;
            khss_h = kh_cloth_v3_dot(khss_qd, khss_g);
            if (khss_h < 0.0f) {   // The far side: through the face, or round it?
                const float khss_tau = khss_c.s0 / (khss_c.s0 - khss_h);
                float khss_pt[4][3];
                KhClothPart* const khss_all[4] = { khss_q[0], khss_f[0], khss_f[1], khss_f[2] };
                for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) {
                    khss_pt[j][k] = khss_all[j]->pp[k] + (khss_all[j]->p[k] - khss_all[j]->pp[k]) * khss_tau;
                }
                float khss_ct[3], khss_dt[3];
                int khss_rt = 0;
                kh_cloth_closest_tri(khss_pt[0], khss_pt[1], khss_pt[2], khss_pt[3], khss_ct, khss_rt);
                kh_cloth_v3_sub(khss_pt[0], khss_ct, khss_dt);
                if (khss_rt != 0 || !(kh_cloth_v3_len(khss_dt) <= khss_c.sep) ||
                    -khss_h > KH_CLOTH_SELF_DEEP * khss_s.self_edge) {
                    // Round it - or through so far that only a yank did it: its side is where it is now.
                    khss_c.side = -khss_c.side;
                    for (int k = 0; k < 3; ++k) khss_g[k] = -khss_g[k];
                    khss_h = -khss_h;
                }
            }
        } else {
            khss_h = kh_cloth_v3_len(khss_qd);
            if (!(khss_h > 1.0e-7f)) continue;   // On the edge itself: no direction to push.
            for (int k = 0; k < 3; ++k) khss_g[k] = khss_qd[k] / khss_h;
        }
        khss_apply(khss_q, khss_one, 1, khss_f, khss_bw, 3, khss_g, khss_h, khss_c.sep, khss_c.dep);
    }

    for (size_t i = 0; i < khss_s.self_e.size(); ++i) {
        KhClothSelfE& khss_c = khss_s.self_e[i];
        const KhClothSelfEdge& E0 = khss_s.self_edges[khss_c.e0];
        const KhClothSelfEdge& E1 = khss_s.self_edges[khss_c.e1];
        KhClothPart* const khss_x[2] = { &khss_s.part[E0.a], &khss_s.part[E0.b] };
        KhClothPart* const khss_y[2] = { &khss_s.part[E1.a], &khss_s.part[E1.b] };
        float s, t, khss_c0[3], khss_c1[3], khss_dv[3], khss_g[3], khss_h;
        kh_cloth_closest_seg(khss_x[0]->p, khss_x[1]->p, khss_y[0]->p, khss_y[1]->p, s, t, khss_c0, khss_c1);
        kh_cloth_v3_sub(khss_c0, khss_c1, khss_dv);
        const float khss_dl = kh_cloth_v3_len(khss_dv);
        const float khss_d1 = kh_cloth_v3_dot(khss_dv, khss_c.n);
        bool khss_through = false;
        if (!(khss_dl > 1.0e-7f && khss_d1 > 0.0f)) {   // Turned against the start: through each other, or past?
            const float khss_tau = khss_c.d0 / fmaxf(khss_c.d0 - khss_d1, 1.0e-12f);
            float khss_pt[4][3];
            KhClothPart* const khss_all[4] = { khss_x[0], khss_x[1], khss_y[0], khss_y[1] };
            for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) {
                khss_pt[j][k] = khss_all[j]->pp[k] + (khss_all[j]->p[k] - khss_all[j]->pp[k]) * khss_tau;
            }
            float st, tt, khss_a[3], khss_b[3], khss_ab[3];
            kh_cloth_closest_seg(khss_pt[0], khss_pt[1], khss_pt[2], khss_pt[3], st, tt, khss_a, khss_b);
            kh_cloth_v3_sub(khss_a, khss_b, khss_ab);
            khss_through = st > 0.0f && st < 1.0f && tt > 0.0f && tt < 1.0f &&
                           kh_cloth_v3_len(khss_ab) <= khss_c.sep && -khss_d1 <= KH_CLOTH_SELF_DEEP * khss_s.self_edge;
            if (!khss_through) {   // Past each other: the direction is where they are now.
                if (!(khss_dl > 1.0e-7f)) continue;
                for (int k = 0; k < 3; ++k) khss_c.n[k] = khss_dv[k] / khss_dl;
            }
        }
        if (khss_through) {   // Back the way they came.
            for (int k = 0; k < 3; ++k) khss_g[k] = khss_c.n[k];
            khss_h = khss_d1;
        } else {
            for (int k = 0; k < 3; ++k) khss_g[k] = khss_dv[k] / khss_dl;
            khss_h = khss_dl;
        }
        const float khss_wx[2] = { 1.0f - s, s }, khss_wy[2] = { 1.0f - t, t };
        khss_apply(khss_x, khss_wx, 2, khss_y, khss_wy, 2, khss_g, khss_h, khss_c.sep, khss_c.dep);
    }
}
static constexpr float    KH_AFF_CORE_M = 0.10f;   // A radial force fades to zero inside this (m).
static constexpr float    KH_AFF_DRAG_MAX = 1000.0f;   // KH_CLOTH_DRAG: a drag affector's most thickening.

enum KhAffType : uint8_t {
    KH_AFF_NONE = 0, KH_AFF_WIND, KH_AFF_TURBULENCE, KH_AFF_VORTEX, KH_AFF_FORCE, KH_AFF_RADIAL, KH_AFF_DRAG,
};

// The affectors at one particle position: khae_air gains their air velocity
// (m/s), khae_acc their acceleration (m/s^2), and khae_drag their thickening
// of the air (KH_CLOTH_DRAG: the air's density is 1 + khae_drag times its
// own). khae_tback is how long before the frame's end this substep ends (s),
// turbulence's clock.
inline void kh_cloth_aff_eval(const KhClothAffView* khae_v, size_t khae_n, const float* khae_p,
                              float khae_tback, float* khae_air, float* khae_acc, float& khae_drag) {
    for (size_t khae_i = 0; khae_i < khae_n; ++khae_i) {
        const KhClothAffView& v = khae_v[khae_i];
        if (!(v.gain > 0.0f)) continue;
        float d[3];
        kh_cloth_v3_sub(khae_p, v.c, d);
        const float l[3] = { kh_cloth_v3_dot(d, v.ax), kh_cloth_v3_dot(d, v.ax + 3), kh_cloth_v3_dot(d, v.ax + 6) };
        float g = v.gain;
        if (v.bounded) {
            float q2 = 0.0f;
            for (int k = 0; k < 3; ++k) { const float e = l[k] * v.inv_r[k]; q2 += e * e; }
            if (q2 >= 1.0f) continue;
            if (v.edge > 0.0f) {
                const float u = (sqrtf(q2) - (1.0f - v.edge)) / v.edge;
                if (u > 0.0f) g *= 1.0f - u * u * (3.0f - 2.0f * u);
            }
        }
        const float* f = v.ax + 3;   // Forward: the script's vectorDir.
        switch (v.type) {
        case KH_AFF_WIND:
            for (int k = 0; k < 3; ++k) khae_air[k] += f[k] * v.p[0] * g;
            break;
        case KH_AFF_FORCE:
            for (int k = 0; k < 3; ++k) khae_acc[k] += f[k] * v.p[0] * g;
            break;
        case KH_AFF_RADIAL: {
            // Away from the centre for a positive strength. Inside KH_AFF_CORE_M
            // it fades to zero: a particle on the centre is not flung about by a
            // direction that is undefined there.
            const float r = kh_cloth_v3_len(d);
            if (r > 1.0e-6f) {
                const float s = v.p[0] * g * (r < KH_AFF_CORE_M ? r / KH_AFF_CORE_M : 1.0f) / r;
                for (int k = 0; k < 3; ++k) khae_acc[k] += d[k] * s;
            }
            break;
        }
        case KH_AFF_DRAG:   // KH_CLOTH_DRAG: thicker air - densities add.
            khae_drag += (v.p[0] < 0.0f ? 0.0f : (v.p[0] > KH_AFF_DRAG_MAX ? KH_AFF_DRAG_MAX : v.p[0])) * g;
            break;
        case KH_AFF_VORTEX: {
            // A Rankine vortex about the forward axis: the swirl grows with the
            // distance from the axis out to the core radius and falls as
            // 1 / distance beyond it (still at the axis, peak at the core). The
            // inflow ramps in over the same core. rho x f is the right-hand
            // sense about vectorDir in WORLD axes: the engine axes are a
            // reflection of them, so the order flips.
            float rho[3];
            for (int k = 0; k < 3; ++k) rho[k] = d[k] - f[k] * l[1];
            const float rl = kh_cloth_v3_len(rho);
            if (rl > 1.0e-6f) {
                const float core = v.p[3];
                const float vt = v.p[0] * (rl < core ? rl / core : core / rl);
                const float vr = -v.p[1] * (rl < core ? rl / core : 1.0f);
                const float t[3] = { rho[1] * f[2] - rho[2] * f[1],
                                     rho[2] * f[0] - rho[0] * f[2],
                                     rho[0] * f[1] - rho[1] * f[0] };
                for (int k = 0; k < 3; ++k) khae_air[k] += (t[k] * vt + rho[k] * vr) / rl * g;
            }
            for (int k = 0; k < 3; ++k) khae_air[k] += f[k] * v.p[2] * g;
            break;
        }
        case KH_AFF_TURBULENCE:
            // A sum of travelling plane waves, each with its amplitude
            // perpendicular to its wave vector (divergence-free), so the sum is
            // an incompressible swirl. Measured from the affector's own centre:
            // one field in the world whichever cloth, and whichever floating
            // origin, reads it.
            for (uint32_t m = 0; m < KH_AFF_MODES; ++m) {
                const float s = sinf(kh_cloth_v3_dot(v.mk[m], d) + v.mph[m] - v.mw[m] * khae_tback) * g;
                for (int k = 0; k < 3; ++k) khae_air[k] += v.ma[m][k] * s;
            }
            break;
        default:
            break;
        }
    }
}

// One XPBD substep. h is the substep length; the caller owns the accumulator.
// The state's frame (centre / rot / size) is where the carrier stands at the
// END of this substep and khcs_f0 where it stood at the start; each collider
// view is posed the same way, its prev being the start. kh_cloth_step
// interpolates both across the step's span (KH_SUB_CLOCK).
inline void kh_cloth_substep(KhClothState& khcs_s, const KhClothParams& khcs_pr,
                             const KhPhysicsColliderView* khcs_col, size_t khcs_ncol, float khcs_h,
                             const float* khcs_wind, const KhClothFrame& khcs_f0,
                             const KhClothAffView* khcs_aff = nullptr, size_t khcs_naff = 0,
                             float khcs_tback = 0.0f, const KhClothGround* khcs_ground = nullptr) {
    const size_t khcs_n = khcs_s.part.size();
    if (khcs_n == 0 || khcs_h <= 0.0f) return;
    const float khcs_g = -KH_CLOTH_G * khcs_pr.gravity;   // Down the engine up axis (index 1).
    // The per-step dials are stated per KH_CLOTH_H, so a different substep
    // raises what they keep to the matching power: 'substeps' changes how finely
    // the solve runs, not how much motion a second of damping removes. At the
    // default substep the ratio is exactly 1 and this is the dial, bit for bit.
    const float khcs_rate = khcs_h / KH_CLOTH_H;
    auto khcs_keep = [khcs_rate](float khcs_dial) {
        const float khcs_k = 1.0f - (khcs_dial < 0.0f ? 0.0f : (khcs_dial > 1.0f ? 1.0f : khcs_dial));
        return khcs_rate == 1.0f ? khcs_k : powf(khcs_k, khcs_rate);
    };
    // Along the constraints, before the predict (MATERIAL DAMPING).
    const float khcs_damp = khcs_keep(khcs_pr.damping);
    const float khcs_vmax = khcs_pr.max_speed > 0.0f ? khcs_pr.max_speed : 1.0e6f;
    const float khcs_dmax = khcs_vmax * khcs_h;   // A substep may never move a particle further than this...
    // ...relative to the carrier. This substep's carrier runs from khcs_f0 to
    // the state's frame; kh_cloth_frame_delta gives its motion at a point.
    KhClothFrame khcs_f1;
    for (int k = 0; k < 3; ++k) { khcs_f1.centre[k] = khcs_s.centre[k]; khcs_f1.size[k] = khcs_s.size[k]; }
    memcpy(khcs_f1.rot, khcs_s.rot, sizeof(khcs_f1.rot));
    khcs_f1.rotated = khcs_s.rotated;
    const bool khcs_cmove = !kh_cloth_frame_same(khcs_f0, khcs_f1);
    float khcs_cratio[3];
    for (int k = 0; k < 3; ++k) {
        const float khcs_c0 = fabsf(khcs_f0.size[k]) < 1.0e-6f ? 1.0e-6f : khcs_f0.size[k];
        khcs_cratio[k] = khcs_f1.size[k] / khcs_c0;
    }

    // Wind's facing. Pressure on a face acts along its normal whichever way the
    // face is wound, so a face contributes (rel . nhat) nhat, and weighting the
    // per-face terms by area gives each particle the mean of nhat nhat^T over
    // its faces: the per-triangle aerodynamic force spread onto the particles
    // as an acceleration, independent of the tessellation. Flat cloth gets
    // nhat nhat^T exactly; a fold gets what its faces present. Do not average
    // the NORMALS first and normalise: that hands the edge of a fold a
    // full-strength push along whatever direction the nearly cancelling faces
    // leave, including a fold lying edge-on to the wind.
    // It runs whether or not any wind blows: the relative velocity is the air's
    // less the particle's, and still air resists motion through a face exactly
    // as moving air pushes on one. Do not gate it on a nonzero world wind: the
    // face term then vanishes in a calm and returns at full strength at
    // 0.01 m/s, and an edge-on breeze, which pushes nothing, swings a jogging
    // cape 17 degrees further back. 'wind' scales only how much of the world's
    // wind reaches the object.
    const float khcs_wmul = khcs_pr.wind;
    const bool khcs_windy = !khcs_s.tri.empty();
    if (khcs_windy) {
        if (khcs_s.nrm.size() != khcs_n * 7u) khcs_s.nrm.assign(khcs_n * 7u, 0.0f);
        else std::fill(khcs_s.nrm.begin(), khcs_s.nrm.end(), 0.0f);
        for (size_t t = 0; t < khcs_s.tri.size(); ++t) {
            const KhClothTri& khcs_t = khcs_s.tri[t];
            if (khcs_t.a >= khcs_n || khcs_t.b >= khcs_n || khcs_t.c >= khcs_n) continue;
            float khcs_e1[3], khcs_e2[3], khcs_nr[3];
            kh_cloth_v3_sub(khcs_s.part[khcs_t.b].p, khcs_s.part[khcs_t.a].p, khcs_e1);
            kh_cloth_v3_sub(khcs_s.part[khcs_t.c].p, khcs_s.part[khcs_t.a].p, khcs_e2);
            khcs_nr[0] = khcs_e1[1] * khcs_e2[2] - khcs_e1[2] * khcs_e2[1];
            khcs_nr[1] = khcs_e1[2] * khcs_e2[0] - khcs_e1[0] * khcs_e2[2];
            khcs_nr[2] = khcs_e1[0] * khcs_e2[1] - khcs_e1[1] * khcs_e2[0];
            // |n| is twice the area and n n^T / |n| is |n| nhat nhat^T.
            const float khcs_nl = kh_cloth_v3_len(khcs_nr);
            if (khcs_nl < 1.0e-20f) continue;
            const float khcs_il = 1.0f / khcs_nl;
            const float khcs_q[7] = { khcs_nr[0] * khcs_nr[0] * khcs_il, khcs_nr[0] * khcs_nr[1] * khcs_il,
                                      khcs_nr[0] * khcs_nr[2] * khcs_il, khcs_nr[1] * khcs_nr[1] * khcs_il,
                                      khcs_nr[1] * khcs_nr[2] * khcs_il, khcs_nr[2] * khcs_nr[2] * khcs_il,
                                      khcs_nl };
            const uint32_t khcs_iv[3] = { khcs_t.a, khcs_t.b, khcs_t.c };
            for (int khcs_k = 0; khcs_k < 3; ++khcs_k) {
                float* khcs_acc = &khcs_s.nrm[static_cast<size_t>(khcs_iv[khcs_k]) * 7u];
                for (int j = 0; j < 7; ++j) khcs_acc[j] += khcs_q[j];
            }
        }
    }

    const float khcs_mass = khcs_pr.mass > 1.0e-4f ? khcs_pr.mass : 1.0e-4f;
    const float khcs_buoy = khcs_pr.buoyancy < 0.0f ? 0.0f : khcs_pr.buoyancy;
    // KH_CLOTH_DRAG: the media's drag per unit mass (1/s), against a face's motion.
    const float khcs_cair = (khcs_pr.drag > 0.0f ? khcs_pr.drag : 0.0f) / khcs_mass;
    const float khcs_cwat = (khcs_pr.water_drag > 0.0f ? khcs_pr.water_drag : 0.0f) / khcs_mass;
    // ...and its two factors in plain air (no water, no drag affector), the common case, taken once.
    const float khcs_es0 = expf(-KH_CLOTH_SKIN * khcs_cair * khcs_h);
    const float khcs_ef0 = 1.0f - expf(-(1.0f - KH_CLOTH_SKIN) * khcs_cair * khcs_h);

    // MATERIAL DAMPING. Each distance constraint loses 1 - khcs_damp of the rate
    // at which its two ends separate (the strain rate) and nothing else: the
    // velocity change runs along the constraint and is shared by inverse mass,
    // so a rigid motion, which separates no pair, is untouched, and so is the
    // carrier's (a relaxation of the whole velocity toward the carrier's is
    // 'drag' by another name on a still carrier).
    // It acts HERE, on the velocity the last substep finished with (p - pp,
    // pins carrying the carrier's own), and takes it out of pp: damping removes
    // motion, it does not move anything. Do not apply it after the solve, where
    // the velocity still holds what the projections are about to undo: pp is
    // then left displaced after a push has put the particle back, a phantom
    // velocity that never sleeps.
    if (khcs_damp < 1.0f) {
        const float khcs_take = 1.0f - khcs_damp;
        for (int khcs_pass = 0; khcs_pass < 2; ++khcs_pass) {
            const std::vector<KhClothDist>& khcs_set = khcs_pass == 0 ? khcs_s.stretch : khcs_s.bend;
            for (size_t c = 0; c < khcs_set.size(); ++c) {
                KhClothPart& A = khcs_s.part[khcs_set[c].a];
                KhClothPart& B = khcs_s.part[khcs_set[c].b];
                const float khcs_wsum = A.w + B.w;
                if (khcs_wsum <= 0.0f) continue;
                float khcs_dv[3];
                kh_cloth_v3_sub(B.p, A.p, khcs_dv);
                const float khcs_len = kh_cloth_v3_len(khcs_dv);
                if (khcs_len < 1.0e-9f) continue;
                float khcs_rv = 0.0f;   // Separation over the last substep, along the constraint.
                for (int k = 0; k < 3; ++k) khcs_rv += ((B.p[k] - B.pp[k]) - (A.p[k] - A.pp[k])) * khcs_dv[k];
                const float khcs_sc = khcs_rv / khcs_len * khcs_take / (khcs_len * khcs_wsum);
                for (int k = 0; k < 3; ++k) {
                    A.pp[k] -= khcs_dv[k] * khcs_sc * A.w;
                    B.pp[k] += khcs_dv[k] * khcs_sc * B.w;
                }
            }
        }
    }

    // Predict. Velocity is implicit in (p - pp), which is what keeps a
    // position correction from being re-applied as momentum next substep.
    for (size_t i = 0; i < khcs_n; ++i) {
        KhClothPart& khcs_p = khcs_s.part[i];
        // How submerged, ramped over the band (the up axis is metres ASL: no water query).
        float khcs_sub = (khcs_s.sea - khcs_p.p[KH_CLOTH_UP]) * (1.0f / KH_CLOTH_WATER_BAND);
        if (khcs_sub < 0.0f) khcs_sub = 0.0f; else if (khcs_sub > 1.0f) khcs_sub = 1.0f;
        // KH_AFFECTOR: the scripted fields at this particle, where it stands
        // before the prediction (the point the wind below is taken at). A pinned
        // particle does not move, so it is not asked.
        float khcs_aair[3] = { 0.0f, 0.0f, 0.0f };
        float khcs_aacc[3] = { 0.0f, 0.0f, 0.0f };
        float khcs_adrag = 0.0f;
        const bool khcs_aff_on = khcs_naff != 0 && khcs_p.w > 0.0f;
        if (khcs_aff_on) kh_cloth_aff_eval(khcs_aff, khcs_naff, khcs_p.p, khcs_tback, khcs_aair, khcs_aacc, khcs_adrag);
        // KH_CLOTH_DRAG: the air above the water and the still water below, each in its share across the band,
        // drag the point toward the medium's velocity (the world wind times 'wind' plus the affectors' air) as
        // one linear force F = -c (S I + (1 - S) M)(v - u), c the two media's drag in that share (a drag
        // affector thickens the air), at F / mass. The two media pull together toward ue, the air's velocity
        // times its share of the pull, and the step is exact: the part along the surface (S) and the part
        // through it share the axes I and M, so their exponentials are taken one after the other, each a
        // contraction toward ue whatever the dials and the substep: nothing overshoots, and the motion is the
        // same at every substep length. Through the surface it is exact for a flat patch (M a projector); a
        // fold's mixed facing is taken the same way, and still no further than the projector would. A point with
        // no facing (its faces have no area) takes all of it as drag along every axis. The material's own
        // damping is not here: it acts on the constraints, above.
        const float khcs_ra = (1.0f - khcs_sub) * (1.0f + khcs_adrag) * khcs_cair;   // The air's pull (1/s).
        const float khcs_rn = khcs_ra + khcs_sub * khcs_cwat;                         // ...and with the water's.
        float khcs_ue[3] = { 0.0f, 0.0f, 0.0f };   // The media's velocity over one substep (m), as they pull.
        if (khcs_ra > 0.0f) {
            const float khcs_sh = khcs_ra / khcs_rn * khcs_h;
            for (int k = 0; k < 3; ++k) {
                khcs_ue[k] = ((khcs_wind ? khcs_wind[k] * khcs_wmul : 0.0f) + khcs_aair[k]) * khcs_sh;
            }
        }
        const bool khcs_dry = khcs_sub <= 0.0f && khcs_adrag <= 0.0f;   // Then khcs_rn IS khcs_cair: the pair above.
        const float khcs_es = khcs_dry ? khcs_es0 : expf(-KH_CLOTH_SKIN * khcs_rn * khcs_h);   // Along: kept.
        const float khcs_ef = khcs_dry ? khcs_ef0 : 1.0f - expf(-(1.0f - KH_CLOTH_SKIN) * khcs_rn * khcs_h);   // Lost.
        float khcs_v[3];
        for (int k = 0; k < 3; ++k) khcs_v[k] = khcs_ue[k] + (khcs_p.p[k] - khcs_p.pp[k] - khcs_ue[k]) * khcs_es;
        const float* khcs_m = khcs_windy ? &khcs_s.nrm[i * 7u] : nullptr;
        if (khcs_m && khcs_m[6] > 1.0e-20f) {
            const float khcs_rel[3] = { khcs_ue[0] - khcs_v[0], khcs_ue[1] - khcs_v[1], khcs_ue[2] - khcs_v[2] };
            const float khcs_im = khcs_ef / khcs_m[6];
            khcs_v[0] += (khcs_m[0] * khcs_rel[0] + khcs_m[1] * khcs_rel[1] + khcs_m[2] * khcs_rel[2]) * khcs_im;
            khcs_v[1] += (khcs_m[1] * khcs_rel[0] + khcs_m[3] * khcs_rel[1] + khcs_m[4] * khcs_rel[2]) * khcs_im;
            khcs_v[2] += (khcs_m[2] * khcs_rel[0] + khcs_m[4] * khcs_rel[1] + khcs_m[5] * khcs_rel[2]) * khcs_im;
        } else {
            for (int k = 0; k < 3; ++k) khcs_v[k] += (khcs_ue[k] - khcs_v[k]) * khcs_ef;
        }
        // Gravity, less the buoyancy the submerged part of it displaces.
        khcs_v[KH_CLOTH_UP] += (khcs_g + khcs_sub * khcs_buoy * KH_CLOTH_G) * khcs_h * khcs_h;
        if (khcs_aff_on) for (int k = 0; k < 3; ++k) khcs_v[k] += khcs_aacc[k] * khcs_h * khcs_h;
        // The clamp guards against a particle flung loose, so it measures against
        // the carrier's own motion at the particle: a cloth riding a vehicle at
        // 100 m/s is not flung, and a clamp on WORLD speed would stream it out
        // behind its pins. Only a particle fast in the world is tested, so one
        // the carrier leaves standing is never pulled along by the clamp; on a
        // still carrier this is the plain world-speed clamp, bit for bit.
        const float khcs_vl = kh_cloth_v3_len(khcs_v);
        if (khcs_vl > khcs_dmax) {
            float khcs_cd[3] = { 0.0f, 0.0f, 0.0f };
            if (khcs_cmove) kh_cloth_frame_delta(khcs_p.p, khcs_f0, khcs_cratio, khcs_f1, khcs_cd);
            float khcs_rel[3];
            for (int k = 0; k < 3; ++k) khcs_rel[k] = khcs_v[k] - khcs_cd[k];
            const float khcs_rl = khcs_cmove ? kh_cloth_v3_len(khcs_rel) : khcs_vl;
            if (khcs_rl > khcs_dmax) {
                const float khcs_sc = khcs_dmax / khcs_rl;
                for (int k = 0; k < 3; ++k) khcs_v[k] = khcs_cmove ? khcs_cd[k] + khcs_rel[k] * khcs_sc : khcs_v[k] * khcs_sc;
            }
        }
        for (int k = 0; k < 3; ++k) {
            khcs_p.pp[k] = khcs_p.p[k];
            khcs_p.p[k] += khcs_p.w > 0.0f ? khcs_v[k] : 0.0f;
        }
    }

    // Pinned particles ARE the frame: re-placed every substep so a moving object
    // drags its cloth instead of leaving it behind (the consume-point rule the
    // draw passes follow). Their pp is where the carrier held them at the
    // substep's start, so a pin's velocity is the carrier's there and the
    // material damping compares its neighbours against that motion, not against
    // standing still in the world. Under a skeletal binding the targets move too
    // (kh_cloth_target): pp takes the skeleton's pose at the substep's start and
    // p its pose at the end.
    for (size_t i = 0; i < khcs_n; ++i) {
        KhClothPart& khcs_p = khcs_s.part[i];
        if (khcs_p.w > 0.0f) continue;
        float khcs_g1[3], khcs_g0[3];
        kh_cloth_l2w(kh_cloth_target(khcs_s, i, khcs_s.guide_t1, khcs_g1), khcs_s.centre, khcs_s.rot, khcs_s.rotated,
                     khcs_s.size, khcs_p.p);
        kh_cloth_l2w(kh_cloth_target(khcs_s, i, khcs_s.guide_t0, khcs_g0), khcs_f0.centre, khcs_f0.rot,
                     khcs_f0.rotated, khcs_f0.size, khcs_p.pp);
    }

    // Self-collision's pairs, found once the prediction and the pins say where
    // everything is headed this substep, then solved inside the iterations.
    const bool khcs_self = khcs_pr.self_collide && !(khcs_s.self_hold > 0.0f) && !khcs_s.tri.empty();
    if (khcs_self) kh_cloth_self_gather(khcs_s, khcs_pr.self_thickness);
    else { khcs_s.self_c.clear(); khcs_s.self_e.clear(); }

    // XPBD accumulates a multiplier per constraint per substep; it is what makes
    // compliance a MATERIAL. Without it alpha only damps each correction, every
    // extra iteration walks the constraint toward rigid, and the converged
    // answer is the inextensible one whatever the dial says. The multiplier
    // resets per SUBSTEP, not per iteration: each substep is its own implicit
    // solve.
    if (khcs_s.lam_stretch.size() != khcs_s.stretch.size()) khcs_s.lam_stretch.assign(khcs_s.stretch.size(), 0.0f);
    if (khcs_s.lam_bend.size() != khcs_s.bend.size()) khcs_s.lam_bend.assign(khcs_s.bend.size(), 0.0f);
    std::fill(khcs_s.lam_stretch.begin(), khcs_s.lam_stretch.end(), 0.0f);
    std::fill(khcs_s.lam_bend.begin(), khcs_s.lam_bend.end(), 0.0f);
    if (khcs_s.lam_att.size() != khcs_n) khcs_s.lam_att.assign(khcs_n, 0.0f);
    std::fill(khcs_s.lam_att.begin(), khcs_s.lam_att.end(), 0.0f);
    const float khcs_h2 = khcs_h * khcs_h;
    const float khcs_a_str = kh_cloth_compliance(khcs_pr.stretch, 1.0e-2f) / khcs_h2;
    const float khcs_a_bnd = kh_cloth_compliance(khcs_pr.bend,    1.0e+0f) / khcs_h2;
    const int   khcs_it = khcs_pr.iterations < 1 ? 1 : (khcs_pr.iterations > 64 ? 64 : khcs_pr.iterations);

    for (int khcs_k = 0; khcs_k < khcs_it; ++khcs_k) {
        // Distance constraints, stretch then bend. Gauss-Seidel in place: each
        // solve sees the previous one's correction, which lets eight iterations
        // stand in for a global solve.
        for (int khcs_pass = 0; khcs_pass < 2; ++khcs_pass) {
            const std::vector<KhClothDist>& khcs_set = khcs_pass == 0 ? khcs_s.stretch : khcs_s.bend;
            std::vector<float>& khcs_lamv = khcs_pass == 0 ? khcs_s.lam_stretch : khcs_s.lam_bend;
            const float khcs_alpha = khcs_pass == 0 ? khcs_a_str : khcs_a_bnd;
            for (size_t c = 0; c < khcs_set.size(); ++c) {
                const KhClothDist& khcs_d = khcs_set[c];
                KhClothPart& A = khcs_s.part[khcs_d.a];
                KhClothPart& B = khcs_s.part[khcs_d.b];
                const float khcs_wsum = A.w + B.w;
                if (khcs_wsum <= 0.0f) continue;
                float khcs_dv[3];
                kh_cloth_v3_sub(B.p, A.p, khcs_dv);
                const float khcs_len = kh_cloth_v3_len(khcs_dv);
                if (khcs_len < 1.0e-9f) continue;
                const float khcs_cval = khcs_len - khcs_d.rest;
                // dLambda = (-C - alpha * lambda) / (sum w |grad C|^2 + alpha).
                // |grad C| is 1 for a distance constraint, so the sum is w_a + w_b.
                const float khcs_lam = (-khcs_cval - khcs_alpha * khcs_lamv[c]) / (khcs_wsum + khcs_alpha);
                khcs_lamv[c] += khcs_lam;
                const float khcs_sc = khcs_lam / khcs_len;
                for (int k = 0; k < 3; ++k) {
                    A.p[k] -= khcs_dv[k] * khcs_sc * A.w;
                    B.p[k] += khcs_dv[k] * khcs_sc * B.w;
                }
            }
        }

        // The pin/sim gradient, solved as an XPBD constraint holding the particle
        // at the position the rigid mesh - or the skeleton (kh_cloth_target) -
        // would give it. A CONSTRAINT, not a lerp toward the target: a lerp has
        // no force balance, so it converges on the target itself, and applied
        // once per iteration per substep even a very weak one erases the whole
        // deviation. A compliance has an equilibrium: the particle settles where
        // the attachment balances gravity.
        for (size_t i = 0; i < khcs_n; ++i) {
            KhClothPart& khcs_p = khcs_s.part[i];
            if (khcs_p.att <= 0.0f || khcs_p.w <= 0.0f) continue;
            float khcs_t[3], khcs_g[3];
            kh_cloth_l2w(kh_cloth_target(khcs_s, i, khcs_s.guide_t1, khcs_g), khcs_s.centre, khcs_s.rot,
                         khcs_s.rotated, khcs_s.size, khcs_t);
            float khcs_av[3];
            kh_cloth_v3_sub(khcs_p.p, khcs_t, khcs_av);
            const float khcs_ac = kh_cloth_v3_len(khcs_av);
            if (khcs_ac < 1.0e-9f) continue;
            const float khcs_aa = khcs_p.att / khcs_h2;
            const float khcs_al = (-khcs_ac - khcs_aa * khcs_s.lam_att[i]) / (khcs_p.w + khcs_aa);
            khcs_s.lam_att[i] += khcs_al;
            const float khcs_as = khcs_al * khcs_p.w / khcs_ac;
            for (int k = 0; k < 3; ++k) khcs_p.p[k] += khcs_av[k] * khcs_as;
        }

        // Self-contact, in the same Gauss-Seidel sweep, so the constraints
        // and the contacts settle against each other rather than in turn.
        if (khcs_self) kh_cloth_self_solve(khcs_s, -1.0f);
    }

    // Self-contact's last sweep, the one that carries its friction, AHEAD of the
    // two limits, which have the last word over the cloth's own layers: under a
    // yank the tethers drag free cloth straight toward its pins, through the
    // cape if that is where it hangs, and a sweep after them would push that
    // cloth back every substep while they drag it through again. The colliders
    // come later still: a solid outranks both.
    if (khcs_self) {
        const float khcs_smu = khcs_pr.self_friction < 0.0f ? 0.0f
                             : (khcs_pr.self_friction > 1.0f ? 1.0f : khcs_pr.self_friction);
        kh_cloth_self_solve(khcs_s, khcs_smu);
    }

    // Per-edge limit, first of the two. The tether bounds distance from the
    // anchor but says nothing about a pair of particles that both drifted the
    // same way, and nothing at all for a cloth with no pins; this catches both.
    // It BOUNDS the worst edge, it does not cap it: a fixed number of
    // Gauss-Seidel passes cannot satisfy every edge of a chain at once, and
    // under a violent yank the worst ratio settles above max_stretch for a few
    // frames before the solver walks it back. The excursion is bounded and
    // transient, which is what stops permanent spaghetti.
    if (khcs_pr.max_stretch >= 1.0f) {
        for (int khcs_lp = 0; khcs_lp < KH_CLOTH_LIMIT_PASSES; ++khcs_lp) {
        for (size_t c = 0; c < khcs_s.stretch.size(); ++c) {
            const KhClothDist& khcs_d = khcs_s.stretch[c];
            KhClothPart& A = khcs_s.part[khcs_d.a];
            KhClothPart& B = khcs_s.part[khcs_d.b];
            const float khcs_wsum = A.w + B.w;
            if (khcs_wsum <= 0.0f) continue;
            float khcs_dv[3];
            kh_cloth_v3_sub(B.p, A.p, khcs_dv);
            const float khcs_len = kh_cloth_v3_len(khcs_dv);
            const float khcs_cap = khcs_d.rest * khcs_pr.max_stretch;
            if (khcs_len <= khcs_cap || khcs_len < 1.0e-9f) continue;
            const float khcs_sc = (khcs_len - khcs_cap) / khcs_len;
            for (int k = 0; k < 3; ++k) {
                A.p[k] += khcs_dv[k] * khcs_sc * (A.w / khcs_wsum);
                B.p[k] -= khcs_dv[k] * khcs_sc * (B.w / khcs_wsum);
            }
        }
        }
    }

    // Long-range attachment, the global bound and the one that IS exact. It runs
    // AFTER the per-edge sweep: whichever of the two goes last is the one that
    // holds, and this is the one worth holding. Only the collider pass may move
    // a particle after this, by its contact depth and the friction that depth
    // allows.
    if (khcs_pr.max_stretch >= 1.0f) {
        for (size_t i = 0; i < khcs_n; ++i) {
            KhClothPart& khcs_p = khcs_s.part[i];
            if (khcs_p.w <= 0.0f || khcs_p.tether <= 0.0f) continue;
            const KhClothPart& khcs_an = khcs_s.part[khcs_p.anchor];
            float khcs_dv[3];
            kh_cloth_v3_sub(khcs_p.p, khcs_an.p, khcs_dv);
            const float khcs_len = kh_cloth_v3_len(khcs_dv);
            const float khcs_cap = khcs_p.tether * khcs_pr.max_stretch;
            if (khcs_len <= khcs_cap || khcs_len < 1.0e-9f) continue;
            const float khcs_sc = khcs_cap / khcs_len;
            for (int k = 0; k < 3; ++k) khcs_p.p[k] = khcs_an.p[k] + khcs_dv[k] * khcs_sc;
        }
    }

    // Collision, last, so nothing after it can push a particle back inside.
    // Friction is COULOMB, position-based (Macklin et al. 2014): the contact's
    // coefficient is the cloth's dial times the collider's, the normal force is
    // the depth this contact just resolved, and the motion it acts on is the
    // tangential part of the substep's displacement RELATIVE TO THE SURFACE
    // (since pp, less how far the surface under the contact moved), so cloth
    // rides a moving collider. Inside the cone the contact holds (static);
    // outside it the tangential motion loses mu times the depth (kinetic). A
    // fixed fraction of the tangential velocity would have no static regime and
    // creep down every slope.
    const float khcs_fr = khcs_pr.friction < 0.0f ? 0.0f : (khcs_pr.friction > 1.0f ? 1.0f : khcs_pr.friction);
    // KH_CLOTH_WARM: the per-collider columns of last-hit triangles, kept across
    // substeps and frames while the collider at that index is the same object
    // (slot and mesh); a column for a collider that changed starts over, and a
    // new column count re-lays them all out.
    if (khcs_ncol != 0u) {
        const size_t khcs_wn = khcs_n * khcs_ncol;
        if (khcs_s.col_warm.size() != khcs_wn || khcs_s.col_warm_key.size() != khcs_ncol * 2u) {
            khcs_s.col_warm.assign(khcs_wn, KH_CLOTH_WARM_NONE);
            khcs_s.col_warm_key.assign(khcs_ncol * 2u, KH_CLOTH_WARM_NONE);
        }
        if (khcs_s.col_clear.size() != khcs_wn || khcs_s.col_clear_col.size() != khcs_ncol) {   // KH_COL_CLEAR: laid out with the warm columns.
            khcs_s.col_clear.assign(khcs_wn, KhColClear{});
            khcs_s.col_clear_col.assign(khcs_ncol, KhColClearCol{});
        }
        for (size_t c = 0; c < khcs_ncol; ++c) {
            const uint32_t khcs_km = static_cast<uint32_t>(khcs_col[c].mesh);
            if (khcs_s.col_warm_key[c * 2u] != khcs_col[c].slot || khcs_s.col_warm_key[c * 2u + 1u] != khcs_km) {
                khcs_s.col_warm_key[c * 2u] = khcs_col[c].slot;
                khcs_s.col_warm_key[c * 2u + 1u] = khcs_km;
                std::fill(khcs_s.col_warm.begin() + static_cast<ptrdiff_t>(c * khcs_n),
                          khcs_s.col_warm.begin() + static_cast<ptrdiff_t>((c + 1u) * khcs_n), KH_CLOTH_WARM_NONE);
                khcs_s.col_clear_col[c] = KhColClearCol{};   // KH_COL_CLEAR: another collider's column; emptied just below.
            }
            // KH_COL_CLEAR: the column's chain to this substep's pose; records of a broken chain are void.
            const bool khcs_was = khcs_s.col_clear_col[c].on;
            if (!kh_col_clear_advance(khcs_s.col_clear_col[c], khcs_col[c]) && (khcs_was || khcs_s.col_clear_col[c].on)) {
                std::fill(khcs_s.col_clear.begin() + static_cast<ptrdiff_t>(c * khcs_n),
                          khcs_s.col_clear.begin() + static_cast<ptrdiff_t>((c + 1u) * khcs_n), KhColClear{});
            }
        }
    }
    for (size_t i = 0; i < khcs_n; ++i) {
        KhClothPart& khcs_p = khcs_s.part[i];
        if (khcs_p.w <= 0.0f) continue;
        for (size_t c = 0; c < khcs_ncol; ++c) {
            float khcs_n_hit[3], khcs_sv[3];
            const float khcs_p0[3] = { khcs_p.p[0], khcs_p.p[1], khcs_p.p[2] };
            if (!kh_physics_collide_one(khcs_col[c], khcs_pr.thickness, khcs_p.pp, khcs_p.p, khcs_n_hit, khcs_sv,
                                        &khcs_s.col_warm[c * khcs_n + i],
                                        khcs_s.col_clear_col[c].on ? &khcs_s.col_clear[c * khcs_n + i] : nullptr,   // KH_COL_CLEAR.
                                        khcs_s.col_clear_col[c].acc)) continue;
            float khcs_f = khcs_fr * khcs_col[c].friction;
            if (khcs_f <= 0.0f) continue;
            if (khcs_f > 1.0f) khcs_f = 1.0f;
            float khcs_push[3];
            kh_cloth_v3_sub(khcs_p.p, khcs_p0, khcs_push);
            const float khcs_depth = kh_cloth_v3_dot(khcs_push, khcs_n_hit);
            if (!(khcs_depth > 0.0f)) continue;
            float khcs_mv[3];
            kh_cloth_v3_sub(khcs_p.p, khcs_p.pp, khcs_mv);
            for (int k = 0; k < 3; ++k) khcs_mv[k] -= khcs_sv[k];
            const float khcs_nd = kh_cloth_v3_dot(khcs_mv, khcs_n_hit);
            float khcs_tan[3];
            for (int k = 0; k < 3; ++k) khcs_tan[k] = khcs_mv[k] - khcs_n_hit[k] * khcs_nd;
            const float khcs_tl = kh_cloth_v3_len(khcs_tan);
            if (khcs_tl < 1.0e-12f) continue;
            const float khcs_cone = khcs_f * khcs_depth;
            const float khcs_stop = khcs_tl <= khcs_cone ? 1.0f : khcs_cone / khcs_tl;   // Share of it removed.
            for (int k = 0; k < 3; ++k) khcs_p.p[k] -= khcs_tan[k] * khcs_stop;
        }
        // KH_CLOTH_GROUND: the terrain, after every collider, so what lies between
        // a collider and the ground ends the substep on the ground's side (a cape
        // under a prone body shows against the body's collider, never under the
        // visible terrain). The same Coulomb form as a collider; the ground is
        // still and its half of the coefficient is 1 (a collider's default), so
        // the contact's is the cloth's 'friction'.
        if (khcs_ground) {
            float khcs_gn[3];
            const float khcs_g0[3] = { khcs_p.p[0], khcs_p.p[1], khcs_p.p[2] };
            if (kh_cloth_ground_contact(*khcs_ground, khcs_pr.thickness, khcs_p.p, khcs_gn)) {
                float khcs_push[3];
                kh_cloth_v3_sub(khcs_p.p, khcs_g0, khcs_push);
                const float khcs_depth = kh_cloth_v3_dot(khcs_push, khcs_gn);
                float khcs_mv[3];
                kh_cloth_v3_sub(khcs_p.p, khcs_p.pp, khcs_mv);
                const float khcs_nd = kh_cloth_v3_dot(khcs_mv, khcs_gn);
                float khcs_tan[3];
                for (int k = 0; k < 3; ++k) khcs_tan[k] = khcs_mv[k] - khcs_gn[k] * khcs_nd;
                const float khcs_tl = kh_cloth_v3_len(khcs_tan);
                if (khcs_fr > 0.0f && khcs_depth > 0.0f && khcs_tl >= 1.0e-12f) {
                    const float khcs_cone = khcs_fr * khcs_depth;
                    const float khcs_stop = khcs_tl <= khcs_cone ? 1.0f : khcs_cone / khcs_tl;
                    for (int k = 0; k < 3; ++k) khcs_p.p[k] -= khcs_tan[k] * khcs_stop;
                }
            }
        }
    }
    // Non-finite rescue. One bad particle poisons every constraint it touches
    // within an iteration, so the test is on the whole set and the answer is
    // the rest pose - a visible snap beats a permanent NaN cloud.
    for (size_t i = 0; i < khcs_n; ++i) {
        if (!kh_cloth_finite3(khcs_s.part[i].p) || !kh_cloth_finite3(khcs_s.part[i].pp)) {
            kh_cloth_reset(khcs_s);
            return;
        }
    }
}

// One frame. Owns every decision that depends on how the object MOVED since
// last frame: teleports, stalls, resizes and the first frame of all.
inline void kh_cloth_step(KhClothState& khcf_s, const KhClothParams& khcf_pr,
                          const KhPhysicsColliderView* khcf_col, size_t khcf_ncol,
                          const float* khcf_centre, const float* khcf_rot, bool khcf_rotated,
                          const float* khcf_size, float khcf_dt, const float* khcf_wind,
                          const KhClothAffView* khcf_aff = nullptr, size_t khcf_naff = 0,
                          const KhClothGround* khcf_ground = nullptr) {
    if (khcf_s.part.empty()) return;

    // A frame that arrives after a load, an alt-tab or a hitch is CLAMPED and
    // simulated like any other (a rest-pose reset would snap every drape back on
    // each hitch, and below 10 fps every frame is a 'stall'). What jumped while
    // the game hung (the object moved or turned past the teleport test) is
    // caught below whatever the frame's length; a dt that is not a positive
    // number advances nothing.
    if (!(khcf_dt > 0.0f)) khcf_dt = 0.0f;
    // KH_DT_RAW: the frame's own length, for the velocities taken below (the carrier's and the jolt test's corners).
    // The clamp is the simulation's budget; do not take velocities over the clamp: a hitch then reads as a burst of
    // speed (a 0.5 s hitch at 150 m/s reads 750 m/s, the next frame's prediction misses by 10 m: a spurious rebase,
    // nothing swept - KH_TELEPORT_ONCE covers it only while a frame's own travel is under the teleport distance -
    // and a jolt where none was).
    const float khcf_dt_raw = khcf_dt;
    if (khcf_dt > KH_CLOTH_DT_MAX) khcf_dt = KH_CLOTH_DT_MAX;
    // KH_SKEL: every placement outside the substeps is at the frame's end.
    khcf_s.guide_t0 = 1.0f;
    khcf_s.guide_t1 = 1.0f;

    // KH_SUB_CLOCK: the frame's motion is measured from the last frame's END
    // (fend); the substeps run from where the simulation stands - the state's
    // frame, at most a substep short of it - and the particles stand against
    // that frame, so it is also what a rebase carries them from.
    KhClothFrame khcf_old = khcf_s.fend;
    KhClothFrame khcf_stood;
    for (int k = 0; k < 3; ++k) { khcf_stood.centre[k] = khcf_s.centre[k]; khcf_stood.size[k] = khcf_s.size[k]; }
    memcpy(khcf_stood.rot, khcf_s.rot, sizeof(khcf_stood.rot));
    khcf_stood.rotated = khcf_s.rotated;
    KhClothFrame khcf_new;
    for (int k = 0; k < 3; ++k) { khcf_new.centre[k] = khcf_centre[k]; khcf_new.size[k] = khcf_size[k]; }
    memcpy(khcf_new.rot, khcf_rot, sizeof(khcf_new.rot));
    khcf_new.rotated = khcf_rotated;

    float khcf_is_old[3];
    for (int k = 0; k < 3; ++k) {
        const float khcf_v = fabsf(khcf_s.size[k]) < 1.0e-6f ? 1.0e-6f : khcf_s.size[k];
        khcf_is_old[k] = 1.0f / khcf_v;
    }

    bool khcf_reset = !khcf_s.primed;
    bool khcf_rebase = false;
    if (!khcf_reset) {
        // A centre that landed further than the threshold from where its own
        // velocity would have taken it, or a basis that turned over, is a
        // teleport. Measured against the prediction, not last frame's centre, so
        // a steady speed (2 m a frame is 60 m/s at 30 fps) never trips it.
        KhClothFrame khcf_pred = khcf_old;
        if (khcf_s.carrier_vok) for (int k = 0; k < 3; ++k) khcf_pred.centre[k] += khcf_s.carrier_v[k] * khcf_dt;
        khcf_rebase = kh_cloth_frame_jumped(khcf_pred, khcf_new, khcf_pr.teleport);
        // KH_TELEPORT_ONCE: after a rebase the prediction carries the jump on; a carrier that moved less than the
        // teleport distance from where it stood did not jump again.
        if (khcf_rebase && khcf_s.carrier_vok && khcf_s.carrier_vreb &&
            !kh_cloth_frame_jumped(khcf_old, khcf_new, khcf_pr.teleport)) khcf_rebase = false;
        if (khcf_dt > 0.0f) {   // KH_DT_RAW: the travel over the frame it took.
            for (int k = 0; k < 3; ++k) khcf_s.carrier_v[k] = (khcf_new.centre[k] - khcf_old.centre[k]) / khcf_dt_raw;
            khcf_s.carrier_vok = true;
            khcf_s.carrier_vreb = khcf_rebase;   // KH_TELEPORT_ONCE.
        }
        // A resized mesh has a different rest pose in world units; rebasing
        // through the old inverse would scale the cloth by the ratio.
        for (int k = 0; k < 3; ++k) {
            const float khcf_r = khcf_size[k] * khcf_is_old[k];
            if (khcf_r < 0.5f || khcf_r > 2.0f) khcf_reset = true;
        }
    }

    if (khcf_rebase && !khcf_reset) {
        kh_cloth_rebase(khcf_s, khcf_s.centre, khcf_s.rot, khcf_s.rotated, khcf_is_old,
                        khcf_centre, khcf_rot, khcf_rotated, khcf_size);
    }

    if (khcf_reset) {
        // KH_SUB_CLOCK: placed at the frame's end, with nothing left over.
        for (int k = 0; k < 3; ++k) { khcf_s.centre[k] = khcf_centre[k]; khcf_s.size[k] = khcf_size[k]; }
        memcpy(khcf_s.rot, khcf_rot, sizeof(khcf_s.rot));
        khcf_s.rotated = khcf_rotated;
        khcf_s.fend = khcf_new;
        khcf_s.sub_t = 1.0f;
        kh_cloth_reset(khcf_s);
        khcf_s.speed_mean = 0.0f;
        khcf_s.reach = 0.0f;
        return;
    }
    // A teleported cloth already stands in the new frame: nothing is swept.
    if (khcf_rebase) { khcf_old = khcf_new; khcf_stood = khcf_new; }

    // SELF-COLLISION STANDS DOWN AFTER A JOLT. A carrier that moves under the teleport test but faster than any cloth
    // could follow drags the free cloth after its pins through itself; with self-collision on it is driven through in
    // places too deep to push back and then held there; with it off, the cloth passes through and falls open again. So
    // a jolt (a change of velocity at any corner of the carrier's box past KH_CLOTH_SELF_JOLT in one frame) turns
    // self-collision off for KH_CLOTH_SELF_HOLD seconds (a further jolt restarts the hold), long enough for the cloth
    // to fall open; then it is back on. Falling or flapping cloth is untouched: the test is the carrier's motion, not
    // the cloth's.
    if (khcf_pr.self_collide) {
        float khcf_jolt = 0.0f;
        const bool khcf_vnow = khcf_dt > 0.0f && !khcf_rebase;
        for (int c = 0; c < 8; ++c) {
            const float khcf_cl[3] = { (c & 1) ? 0.5f : -0.5f, (c & 2) ? 0.5f : -0.5f, (c & 4) ? 0.5f : -0.5f };
            float khcf_c0[3], khcf_c1[3], khcf_v[3];
            kh_cloth_l2w(khcf_cl, khcf_old.centre, khcf_old.rot, khcf_old.rotated, khcf_old.size, khcf_c0);
            kh_cloth_l2w(khcf_cl, khcf_new.centre, khcf_new.rot, khcf_new.rotated, khcf_new.size, khcf_c1);
            for (int k = 0; k < 3; ++k) {   // KH_DT_RAW.
                khcf_v[k] = khcf_vnow ? (khcf_c1[k] - khcf_c0[k]) / khcf_dt_raw : 0.0f;
            }
            if (khcf_vnow && khcf_s.self_vok) {
                float khcf_dv[3];
                kh_cloth_v3_sub(khcf_v, khcf_s.self_v[c], khcf_dv);
                khcf_jolt = fmaxf(khcf_jolt, kh_cloth_v3_len(khcf_dv));
            }
            if (khcf_vnow) for (int k = 0; k < 3; ++k) khcf_s.self_v[c][k] = khcf_v[k];
        }
        if (khcf_vnow) khcf_s.self_vok = true;
        else if (khcf_rebase) khcf_s.self_vok = false;   // A teleport is not a velocity.
        if (khcf_jolt > KH_CLOTH_SELF_JOLT) khcf_s.self_hold = KH_CLOTH_SELF_HOLD;
        else if (khcf_s.self_hold > 0.0f) khcf_s.self_hold -= khcf_dt;
    } else {
        khcf_s.self_hold = 0.0f;
        khcf_s.self_vok = false;
    }

    // Fixed substep with a bounded backlog. The accumulator is CAPPED, not
    // carried: a machine that cannot keep up must run the cloth slow, not spiral
    // into ever more substeps per frame. The cap is a TIME (the stall clamp's),
    // so the frame rate at which the cloth starts to run slow does not move with
    // the dial below.
    const int khcf_want = khcf_pr.substeps < 1 ? 1
                        : (khcf_pr.substeps > KH_CLOTH_MAX_SUBSTEP ? static_cast<int>(KH_CLOTH_MAX_SUBSTEP)
                                                                   : static_cast<int>(khcf_pr.substeps));
    // THE SUBSTEP IS FIXED. XPBD divides compliance by h squared and this
    // integrator carries velocity as the displacement (p - pp) over ONE substep,
    // so a substep that moved with the frame rate would read the same
    // displacement back as a different velocity every frame: frame-time
    // variance as a per-frame kick, a visible shimmer at rest. 'substeps'
    // therefore counts substeps per 1/60 s, not per frame, so h stays a
    // constant: the default 2 is exactly KH_CLOTH_H. A slow frame runs more of
    // them, a fast one fewer, and the leftover carries in the accumulator.
    const float khcf_h = 1.0f / (60.0f * static_cast<float>(khcf_want));
    // KH_SIM_LOD: the substep length changed (the LOD share moved, or the dial
    // did). The displacement p - pp IS the velocity over one substep, so read
    // over the new length it would be a different velocity: carry the velocity,
    // not the displacement - scale it by h_new / h_old. The rebase above moved p
    // and pp together, so the displacement is already in this frame. At an
    // unchanged length this is bit-identical (no write).
    if (khcf_s.h_last > 0.0f && khcf_s.h_last != khcf_h) {
        const float khcf_hr = khcf_h / khcf_s.h_last;
        for (size_t i = 0; i < khcf_s.part.size(); ++i) {
            KhClothPart& khcf_p = khcf_s.part[i];
            for (int k = 0; k < 3; ++k) khcf_p.pp[k] = khcf_p.p[k] - (khcf_p.p[k] - khcf_p.pp[k]) * khcf_hr;
        }
    }
    khcf_s.h_last = khcf_h;
    khcf_s.accum += khcf_dt;
    if (khcf_s.accum > KH_CLOTH_DT_MAX) khcf_s.accum = KH_CLOTH_DT_MAX;
    const uint32_t khcf_nmax = static_cast<uint32_t>(KH_CLOTH_DT_MAX / khcf_h) + 1u;   // A guard; the cap binds first.
    uint32_t khcf_n = 0;
    float khcf_left = khcf_s.accum;
    for (; khcf_left >= khcf_h && khcf_n < khcf_nmax; khcf_left -= khcf_h) ++khcf_n;
    // KH_SUB_CLOCK: the step's span runs from where the simulation stands to
    // the frame's end - the whole accumulator - and the substeps cover this
    // share of it; what they leave (khcf_left, under one substep) is carried.
    // Exactly 1 whenever nothing is left (the float walk may leave a few ulp).
    const float khcf_cov = khcf_n == 0u ? 0.0f : 1.0f - khcf_left / khcf_s.accum;

    // THE CARRIER AND THE COLLIDERS ARE INTERPOLATED ACROSS THE SUBSTEPS.
    // Substep i sees the pose (i + 1) / n of the way through what the substeps
    // cover of the span, from where the simulation stands to the frame's end,
    // for the carrier and for each collider from its own pose there
    // (kh_cloth_sync). Handing every substep the frame's END pose would move the
    // pins, the attachment targets and every collider the whole frame's distance
    // in the first substep and not at all in the rest (a jerk once a frame, a
    // shape that depends on the frame rate).
    // KH_SUB_CLOCK: (i + 1) / n of the covered share, not of the FRAME, so each
    // substep moves the carrier one substep's worth of travel; on the frame
    // clock a frame that ran one substep would move it a whole frame's travel in
    // h, and the leftover would never place it at the time actually simulated
    // (the pins' velocity off the carrier's).
    if (khcf_s.col_sub.size() < khcf_ncol) khcf_s.col_sub.resize(khcf_ncol);
    KhPhysicsColliderView* const khcf_cv = khcf_s.col_sub.data();
    KhClothFrame khcf_f0 = khcf_stood;
    uint32_t khcf_ran = 0;
    while (khcf_ran < khcf_n) {
        const float khcf_t0 = static_cast<float>(khcf_ran) / static_cast<float>(khcf_n) * khcf_cov;
        const float khcf_t1 = static_cast<float>(khcf_ran + 1u) / static_cast<float>(khcf_n) * khcf_cov;
        KhClothFrame khcf_f1;
        kh_cloth_frame_lerp(khcf_stood, khcf_new, khcf_t1, khcf_f1);
        for (int k = 0; k < 3; ++k) { khcf_s.centre[k] = khcf_f1.centre[k]; khcf_s.size[k] = khcf_f1.size[k]; }
        memcpy(khcf_s.rot, khcf_f1.rot, sizeof(khcf_s.rot));
        khcf_s.rotated = khcf_f1.rotated;
        for (size_t c = 0; c < khcf_ncol; ++c) {
            const KhPhysicsColliderView& khcf_src = khcf_col[c];
            KhClothFrame khcf_cur;
            for (int k = 0; k < 3; ++k) { khcf_cur.centre[k] = khcf_src.centre[k]; khcf_cur.size[k] = khcf_src.size[k]; }
            memcpy(khcf_cur.rot, khcf_src.rot, sizeof(khcf_cur.rot));
            khcf_cur.rotated = khcf_src.rotated;
            // A prev that was never filled is all zeros: a still collider.
            const bool khcf_pv = khcf_src.prev.size[0] != 0.0f || khcf_src.prev.size[1] != 0.0f ||
                                 khcf_src.prev.size[2] != 0.0f;
            const KhClothFrame& khcf_p0 = khcf_pv ? khcf_src.prev : khcf_cur;
            KhClothFrame khcf_a, khcf_b;
            kh_cloth_frame_lerp(khcf_p0, khcf_cur, khcf_t0, khcf_a);
            kh_cloth_frame_lerp(khcf_p0, khcf_cur, khcf_t1, khcf_b);
            KhPhysicsColliderView& khcf_v = khcf_cv[c];
            khcf_v = khcf_src;
            float khcf_bs = 0.0f;
            for (int k = 0; k < 3; ++k) {
                khcf_v.centre[k] = khcf_b.centre[k];
                khcf_v.bs_c[k] = khcf_b.centre[k];
                const float khcf_e = fabsf(khcf_b.size[k]) < 1.0e-6f ? 1.0e-6f : khcf_b.size[k];
                khcf_v.size[k] = khcf_e;
                khcf_v.inv_size[k] = 1.0f / khcf_e;
                khcf_bs += 0.25f * khcf_e * khcf_e;
            }
            memcpy(khcf_v.rot, khcf_b.rot, sizeof(khcf_v.rot));
            khcf_v.rotated = khcf_b.rotated;
            khcf_v.bs_r = sqrtf(khcf_bs);
            khcf_v.prev = khcf_a;
            // KH_SKEL: a deforming collider's SHAPE is interpolated like its
            // frame - the corners stand (1 - t) of the frame's motion back
            // from its pose - so a fast limb crosses the cloth in substep
            // steps, not in one.
            khcf_v.def0 = khcf_src.deform ? khcf_t0 - 1.0f : 0.0f;
            khcf_v.def1 = khcf_src.deform ? khcf_t1 - 1.0f : 0.0f;
            // Its bound is the gather's, which covers both poses about the
            // same centre; the box's half-diagonal covers only this one.
            if (khcf_src.deform && khcf_src.bs_r > khcf_v.bs_r) khcf_v.bs_r = khcf_src.bs_r;
        }
        // A skeleton's targets are interpolated like the carrier.
        khcf_s.guide_t0 = khcf_t0;
        khcf_s.guide_t1 = khcf_t1;
        // The affectors hold still across the frame; only turbulence's clock
        // runs, and it runs by the substep.
        kh_cloth_substep(khcf_s, khcf_pr, khcf_cv, khcf_ncol, khcf_h, khcf_wind, khcf_f0,
                         khcf_aff, khcf_naff, static_cast<float>(khcf_n - 1u - khcf_ran) * khcf_h, khcf_ground);
        khcf_s.accum -= khcf_h;
        khcf_f0 = khcf_f1;
        ++khcf_ran;
    }
    // A substep's non-finite rescue (kh_cloth_reset) zeroes the budget inside this loop and the substeps after it
    // take it below zero, which would idle the cloth until frames repaid it. Otherwise this is a no-op.
    if (khcf_s.accum < 0.0f) khcf_s.accum = 0.0f;
    // The same rescue zeroes h_last (no displacement stood then), but every substep after it ran at khcf_h: the length
    // the displacement now stands for. After a rescue in the last substep it is zero, which any length reads alike.
    if (khcf_ran > 0u) khcf_s.h_last = khcf_h;
    // KH_SUB_CLOCK: the state's frame is where the substeps ended (the frame's
    // end only when nothing is left over) and the frame's end stands beside it.
    // The writeback is relative to the state's frame, so the draw's own
    // transform carries the whole cloth, pins and free particles alike, rigidly
    // across the remainder; relative to the frame's end, a frame that ran no
    // substep would draw the free cloth a frame of travel behind the object.
    for (int k = 0; k < 3; ++k) { khcf_s.centre[k] = khcf_f0.centre[k]; khcf_s.size[k] = khcf_f0.size[k]; }
    memcpy(khcf_s.rot, khcf_f0.rot, sizeof(khcf_s.rot));
    khcf_s.rotated = khcf_f0.rotated;
    khcf_s.fend = khcf_new;
    khcf_s.sub_t = khcf_cov;
    khcf_s.guide_t0 = 1.0f;
    khcf_s.guide_t1 = 1.0f;

    // A frame shorter than a substep advances nothing, and nothing moves: the
    // particles, the pins among them, stand against the state's frame, which
    // stood still, and the draw carries them all with the object.
    if (khcf_ran == 0) return;

    // The sleep test: the mean speed of the SIMULATED particles relative to the
    // carrier over the last substep. A pinned particle never moves in the
    // solver's terms, and averaging it in would scale the threshold by the
    // painted fraction; measured absolutely, a cloth on anything in motion
    // could never settle.
    KhClothFrame khcf_fl0;   // The last substep's start; the state's frame (khcf_f0) is its end.
    kh_cloth_frame_lerp(khcf_stood, khcf_new,
                        static_cast<float>(khcf_ran - 1u) / static_cast<float>(khcf_ran) * khcf_cov, khcf_fl0);
    float khcf_ratio[3];
    for (int k = 0; k < 3; ++k) {
        const float khcf_s0 = fabsf(khcf_fl0.size[k]) < 1.0e-6f ? 1.0e-6f : khcf_fl0.size[k];
        khcf_ratio[k] = khcf_f0.size[k] / khcf_s0;
    }
    const bool khcf_moving = !kh_cloth_frame_same(khcf_fl0, khcf_f0);
    double khcf_sum = 0.0;
    uint32_t khcf_sim = 0;
    float khcf_reach2 = 0.0f;
    const float khcf_inv = 1.0f / khcf_h;
    for (size_t i = 0; i < khcf_s.part.size(); ++i) {
        const KhClothPart& khcf_p = khcf_s.part[i];
        float khcf_r[3];
        kh_cloth_v3_sub(khcf_p.p, khcf_s.centre, khcf_r);
        const float khcf_r2 = kh_cloth_v3_dot(khcf_r, khcf_r);
        if (khcf_r2 > khcf_reach2) khcf_reach2 = khcf_r2;
        if (khcf_p.w <= 0.0f) continue;
        float khcf_df[3] = { 0.0f, 0.0f, 0.0f };
        if (khcf_moving) kh_cloth_frame_delta(khcf_p.pp, khcf_fl0, khcf_ratio, khcf_f0, khcf_df);
        float khcf_v[3];
        for (int k = 0; k < 3; ++k) khcf_v[k] = (khcf_p.p[k] - khcf_p.pp[k]) - khcf_df[k];
        khcf_sum += kh_cloth_v3_len(khcf_v) * khcf_inv;
        ++khcf_sim;
    }
    khcf_s.speed_mean = khcf_sim ? static_cast<float>(khcf_sum / static_cast<double>(khcf_sim)) : 0.0f;
    khcf_s.reach = sqrtf(khcf_reach2);

    // The park ('sleep'). A cloth whose mean particle speed is under the
    // threshold has finished moving as far as anyone can see, and carrying its
    // residual velocity forward turns a converged drape into a shimmer. Killing
    // the velocity (not the position) leaves gravity and every constraint
    // acting, so it wakes the instant anything pushes it. It is the velocity
    // RELATIVE TO THE CARRIER that is killed: what is left is the carrier's own.
    if (khcf_pr.sleep > 0.0f && khcf_s.speed_mean < khcf_pr.sleep) {
        for (size_t i = 0; i < khcf_s.part.size(); ++i) {
            KhClothPart& khcf_p = khcf_s.part[i];
            if (khcf_p.w <= 0.0f) continue;
            float khcf_df[3] = { 0.0f, 0.0f, 0.0f };
            if (khcf_moving) kh_cloth_frame_delta(khcf_p.pp, khcf_fl0, khcf_ratio, khcf_f0, khcf_df);
            for (int k = 0; k < 3; ++k) khcf_p.pp[k] = khcf_p.p[k] - khcf_df[k];
        }
        khcf_s.speed_mean = 0.0f;
    }
}
}