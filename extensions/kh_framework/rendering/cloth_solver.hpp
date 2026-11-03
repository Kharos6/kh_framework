#pragma once

// The feature half (rendering_integration_features.hpp): CLOTH, the simulation core: the physics primitives, the
// collider BVH, collision, tethers, self-collision, the worker fork, the affector evaluation and the cloth step. Never
// include it on its own.
namespace RenderIntegration {

// Hold one particle 'thick' metres above the ground patch, along the normal of
// the triangle it stands over (the exact distance to that plane). A push out of
// a valley can land the particle over the neighbouring triangle and still under
// it, so the projection repeats over wherever the last push landed, up to
// CLOTH_GROUND_PASSES; a pass that finds the particle clear ends it. True,
// with the last push's direction, when it moved the particle. Outside the patch
// there is no ground (the patch covers the cloth's reach).
inline bool cloth_ground_plane(const ClothGround& g, float thick, float* p, float* n_out);
inline uint32_t cloth_fork_workers();   // Both pools' thread count (defined with the pool).
inline void cloth_fork_wake();          // Wakes the idle workers (defined with the pool).
inline void cloth_fork_wake_try();      // The same without ever blocking (SKIN_FORK; defined with the pool).

// CLOTH - the simulation core: particles, distance constraints, the
// collider BVH and the XPBD solve. Pure arithmetic: every function here runs
// standalone on its inputs, and nothing in this block may touch D3D or SQF. Its
// only globals are g_physics_pose_serial (an atomic), the PROF scopes'
// counters (inert while the stats are off) and, through cloth_fork_run (the
// self-collision gather), the fork's state (CLOTH_FORK).

// Standard gravity (m/s^2). ENGINE AXES ARE (EAST, UP, NORTH): the up axis is
// index 1, not index 2 (applied to index 2 it pulls every cloth north instead
// of down). cloth_engine_pos is the authority: it puts zASL into index 1.
static constexpr float    CLOTH_G          = 9.80665f;
static constexpr int      CLOTH_UP         = 1;
static constexpr uint32_t CLOTH_MAX_SUBSTEP= 8u;         // The 'substeps' dial's top: substeps per 1/60 s.
// A longer frame is simulated as this long: the stall clamp (cloth_step),
// the one bound on how much time one step may simulate.
static constexpr float    CLOTH_DT_MAX     = 0.100f;
// The reference substep ('substeps' 2, the default, runs exactly this) and the
// interval the per-step dials (the cloth's damping; a chain's damping,
// linearDamping and angularDamping) are stated per, so changing 'substeps'
// changes the solve, not what they mean. The drag dials are forces
// (CLOTH_DRAG), stated per second.
static constexpr float    CLOTH_H          = 1.0f / 120.0f;
static constexpr uint32_t PHYSICS_BVH_LEAF   = 4u;         // Triangles per leaf.
// Gauss-Seidel passes over the per-edge stretch limit. The worst-edge ratio
// flattens past four passes; an exact cap would need many more on a long
// chain and buys nothing visible.
static constexpr int CLOTH_LIMIT_PASSES = 4;
// Attachment compliance at a paint weight of one half - the constant that sets
// where the pin/sim gradient crosses halfway. See cloth_build.
static constexpr float CLOTH_ATT_SOFT = 0.30f;
// CLOTH_DRAG - the drag of the air (and of the water) on cloth: one linear
// force per point against its velocity RELATIVE TO THE MEDIUM,
// F = -c (S I + (1 - S) M)(v - u). c is the cloth's 'drag' (or 'waterDrag') in
// kg/s, M the point's facing (the area-weighted mean of n n^T over its faces),
// S = CLOTH_SKIN the share left to motion along the surface. A flat plate's
// law: a face moving through the medium is pushed back hard (pressure), one
// sliding along its own plane barely (skin friction), so cloth edge-on to a
// wind, or lying flat in one, is barely moved. The acceleration is F / mass, so a heavier cloth answers the same air
// less.
static constexpr float CLOTH_SKIN = 0.02f;
// SEA LEVEL IS ZERO and the up axis is metres ASL, so a particle is underwater
// exactly when its altitude is negative (Arma has no caves: no negative
// altitude is dry). The solver runs in a frame of its own (ClothState::sea
// says where sea level lies in it - the floating origin in cloth_sync), so
// the test is against that. The transition is ramped over this depth, not
// switched at the surface: a hard switch makes a particle at the waterline
// alternate between full gravity and full buoyancy every frame, a visible buzz.
static constexpr float CLOTH_WATER_BAND = 0.10f;

// COL_CLEAR: a pose identity. Any thread (a worker builds and refits); an
// identity, so never reset (session_globals_reset's keep list).
static std::atomic<uint32_t> g_physics_pose_serial{ 0 };
inline uint32_t physics_pose_seq_next() {
    uint32_t s = g_physics_pose_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    if (s == 0u) s = g_physics_pose_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
    return s;
}
inline void cloth_v3_sub(const float* a, const float* b, float* o) {
    o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2];
}
inline float cloth_v3_dot(const float* a, const float* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
inline float cloth_v3_len(const float* a) { return sqrtf(cloth_v3_dot(a, a)); }
inline bool cloth_finite3(const float* a) {
    return a[0] == a[0] && a[1] == a[1] && a[2] == a[2] &&
           fabsf(a[0]) < 1.0e12f && fabsf(a[1]) < 1.0e12f && fabsf(a[2]) < 1.0e12f;
}

// COL_CONVEX: one triangle's plane from its corners: unit outward normal
// (the winding's, as the BVH's inside test reads it) and offset. False on a
// degenerate triangle.
inline bool physics_tri_plane(const float* v, float* n, float& d) {
    float e1[3], e2[3];
    cloth_v3_sub(v + 3, v, e1);
    cloth_v3_sub(v + 6, v, e2);
    n[0] = e1[1] * e2[2] - e1[2] * e2[1];
    n[1] = e1[2] * e2[0] - e1[0] * e2[2];
    n[2] = e1[0] * e2[1] - e1[1] * e2[0];
    const float l = cloth_v3_len(n);
    if (!(l > 1.0e-30f)) return false;
    for (int k = 0; k < 3; ++k) n[k] /= l;
    d = cloth_v3_dot(n, v);
    return true;
}
// COL_CONVEX: the pieces and their planes from a freshly built BVH (vp in
// stream order) and the build's position-welded corner ids (cid, 3 per
// kept triangle). A piece is a connected component of the welded corners; its
// planes are its triangles' planes with coplanar ones merged (the first
// triangle stands for the plane): a capsule of a few hundred triangles is a
// hundred-odd planes, a box six. physics_convex_refit then fills the sphere
// and the pose-dependent lanes.
// Convexity is the script's promise (physicsCollider's 'convex') and is not checked: a concave piece collides as the
// intersection of its planes' insides - a particle is pushed out only when behind every plane (physics_convex_one:
// the largest plane distance) - which is less than the piece, so cloth can pass into its concave parts.
inline void physics_convex_build(PhysicsBvh& o, const uint32_t* cid, uint32_t nt) {
    o.cvx_ptri.clear(); o.cvx_pstart.clear(); o.cvx_tri.clear();
    o.cvx_pl.clear(); o.cvx_pl0.clear(); o.cvx_pv.clear(); o.cvx_sph.clear();
    o.cvx_only = false;
    if (nt == 0u || !cid) return;
    uint32_t nid = 0u;
    for (uint32_t i = 0; i < nt * 3u; ++i) if (cid[i] + 1u > nid) nid = cid[i] + 1u;
    std::vector<uint32_t> par(nid);
    for (uint32_t i = 0; i < nid; ++i) par[i] = i;
    auto find = [&](uint32_t x) { while (par[x] != x) { par[x] = par[par[x]]; x = par[x]; } return x; };
    for (uint32_t t = 0; t < nt; ++t) {
        const uint32_t a = find(cid[t * 3u]), b = find(cid[t * 3u + 1u]), c = find(cid[t * 3u + 2u]);
        if (b != a) par[b] = a;
        const uint32_t a2 = find(a);
        if (find(c) != a2) par[find(c)] = a2;
    }
    // Pieces in first-seen order; every triangle filed under its piece.
    std::vector<uint32_t> root_piece(nid, 0xFFFFFFFFu);
    o.cvx_ptri.resize(nt);
    uint32_t np = 0u;
    for (uint32_t t = 0; t < nt; ++t) {
        const uint32_t r = find(cid[t * 3u]);
        if (root_piece[r] == 0xFFFFFFFFu) root_piece[r] = np++;
        o.cvx_ptri[t] = root_piece[r];
    }
    std::vector<uint32_t> head(np + 1u, 0u);
    for (uint32_t t = 0; t < nt; ++t) ++head[o.cvx_ptri[t] + 1u];
    for (uint32_t p = 0; p < np; ++p) head[p + 1u] += head[p];
    std::vector<uint32_t> by(nt);
    {
        std::vector<uint32_t> fill(head.begin(), head.end() - 1);
        for (uint32_t t = 0; t < nt; ++t) by[fill[o.cvx_ptri[t]]++] = t;
    }
    // COL_CONVEX_CELLS: the merge runs on every collider build, convex-marked or not. The kept planes are filed by
    // normal on a 0.01 grid and a triangle tests only the 27 cells around its own (a scan of every kept plane is
    // about n^2 / 2 tests on a smooth or non-convex piece, where almost nothing merges: a game-thread stall of
    // seconds for a 50k-triangle collider). The test's dot > 0.99999 between unit normals puts them under 0.0047
    // apart on every axis, so any plane it can match is in those cells, and the planes kept, and their order, are
    // exactly the full scan's. A normal that is not a number matches nothing either way (every comparison with it is
    // false) and is kept unfiled.
    std::unordered_map<uint64_t, std::vector<uint32_t>> cells;
    auto cell = [](const float* vc_n, int* vc_i) -> bool {
        for (int k = 0; k < 3; ++k) {
            if (!(vc_n[k] >= -2.0f && vc_n[k] <= 2.0f)) return false;   // Not a number (a unit normal is inside).
            vc_i[k] = static_cast<int>(floorf(vc_n[k] * 100.0f)) + 512;   // 312 .. 712: 10 bits each.
        }
        return true;
    };
    auto key = [](int x, int y, int z) -> uint64_t {
        return (static_cast<uint64_t>(x) << 20) | (static_cast<uint64_t>(y) << 10) | static_cast<uint64_t>(z);
    };
    o.cvx_pstart.push_back(0u);
    for (uint32_t p = 0; p < np; ++p) {
        cells.clear();
        for (uint32_t i = head[p]; i < head[p + 1u]; ++i) {
            const uint32_t t = by[i];
            float n[3], d;
            if (!physics_tri_plane(&o.vp[static_cast<size_t>(t) * 9u], n, d)) continue;
            bool dup = false;   // Coplanar with a plane this piece already has.
            int vx_c[3];
            const bool filed = cell(n, vx_c);
            if (filed) {
                for (int dx = -1; dx <= 1 && !dup; ++dx)
                for (int dy = -1; dy <= 1 && !dup; ++dy)
                for (int dz = -1; dz <= 1 && !dup; ++dz) {
                    const auto it = cells.find(key(vx_c[0] + dx, vx_c[1] + dy, vx_c[2] + dz));
                    if (it == cells.end()) continue;
                    for (uint32_t q : it->second) {
                        const float* m = &o.cvx_pl[static_cast<size_t>(q) * 4u];
                        if (cloth_v3_dot(n, m) > 0.99999f && fabsf(d - m[3]) <= 1.0e-4f * (1.0f + fabsf(d))) {
                            dup = true;
                            break;
                        }
                    }
                }
            }
            if (dup) continue;
            if (filed) {
                cells[key(vx_c[0], vx_c[1], vx_c[2])].push_back(
                    static_cast<uint32_t>(o.cvx_tri.size()));
            }
            o.cvx_tri.push_back(t);
            o.cvx_pl.push_back(n[0]); o.cvx_pl.push_back(n[1]); o.cvx_pl.push_back(n[2]); o.cvx_pl.push_back(d);
        }
        o.cvx_pstart.push_back(static_cast<uint32_t>(o.cvx_tri.size()));
    }
    o.cvx_sph.assign(static_cast<size_t>(np) * 4u, 0.0f);
}
// COL_CONVEX: the planes and spheres for the corners now in vp (and, with
// vd, the previous pose's planes and each plane's mean corner motion). After
// every refit and build; nothing else reads a pose's convex lanes.
inline void physics_convex_refit(PhysicsBvh& o) {
    const size_t np = o.cvx_pstart.empty() ? 0u : o.cvx_pstart.size() - 1u;
    const size_t nl = o.cvx_tri.size();
    if (np == 0u || nl == 0u || o.cvx_ptri.size() != o.tri.size()) {
        o.cvx_pl0.clear(); o.cvx_pv.clear();
        return;
    }
    const bool sw = !o.vd.empty() && o.vd.size() == o.vp.size();
    o.cvx_pl.resize(nl * 4u);
    if (sw) { o.cvx_pl0.resize(nl * 4u); o.cvx_pv.resize(nl * 3u); }
    else { o.cvx_pl0.clear(); o.cvx_pv.clear(); }
    for (size_t q = 0; q < nl; ++q) {
        const size_t ti = static_cast<size_t>(o.cvx_tri[q]) * 9u;
        float* pl = &o.cvx_pl[q * 4u];
        float n[3], d;
        if (physics_tri_plane(&o.vp[ti], n, d)) { pl[0] = n[0]; pl[1] = n[1]; pl[2] = n[2]; pl[3] = d; }
        // A triangle gone flat keeps its last plane (the lane holds the template's or the last pose's).
        if (sw) {
            float v0[9];
            for (int j = 0; j < 9; ++j) v0[j] = o.vp[ti + j] - o.vd[ti + j];
            float* pl0 = &o.cvx_pl0[q * 4u];
            if (physics_tri_plane(v0, n, d)) { pl0[0] = n[0]; pl0[1] = n[1]; pl0[2] = n[2]; pl0[3] = d; }
            else memcpy(pl0, pl, sizeof(float) * 4u);
            float* pv = &o.cvx_pv[q * 3u];
            for (int k = 0; k < 3; ++k) {
                pv[k] = (o.vd[ti + k] + o.vd[ti + 3 + k] + o.vd[ti + 6 + k]) * (1.0f / 3.0f);
            }
        }
    }
    // The spheres: the box of every corner of the piece at both poses, then
    // the farthest corner from its centre.
    std::vector<float> mn(np * 3u, 1.0e30f), mx(np * 3u, -1.0e30f);
    for (size_t t = 0; t < o.tri.size(); ++t) {
        const uint32_t p = o.cvx_ptri[t];
        if (p >= np) continue;
        const float* v = &o.vp[t * 9u];
        for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
            const float a = v[c * 3 + k];
            const float bq = sw ? a - o.vd[t * 9u + c * 3 + k] : a;
            const float lo = a < bq ? a : bq, hi = a < bq ? bq : a;
            if (lo < mn[p * 3u + k]) mn[p * 3u + k] = lo;
            if (hi > mx[p * 3u + k]) mx[p * 3u + k] = hi;
        }
    }
    o.cvx_sph.assign(np * 4u, 0.0f);
    for (size_t p = 0; p < np; ++p) {
        float* sp = &o.cvx_sph[p * 4u];
        for (int k = 0; k < 3; ++k) sp[k] = mn[p * 3u + k] <= mx[p * 3u + k] ? 0.5f * (mn[p * 3u + k] + mx[p * 3u + k]) : 0.0f;
    }
    for (size_t t = 0; t < o.tri.size(); ++t) {
        const uint32_t p = o.cvx_ptri[t];
        if (p >= np) continue;
        float* sp = &o.cvx_sph[p * 4u];
        const float* v = &o.vp[t * 9u];
        for (int c = 0; c < 3; ++c) {
            float r2 = 0.0f, r2p = 0.0f;
            for (int k = 0; k < 3; ++k) {
                const float a = v[c * 3 + k] - sp[k];
                r2 += a * a;
                if (sw) { const float bq = a - o.vd[t * 9u + c * 3 + k]; r2p += bq * bq; }
            }
            if (r2 > sp[3]) sp[3] = r2;
            if (r2p > sp[3]) sp[3] = r2p;
        }
    }
    for (size_t p = 0; p < np; ++p) o.cvx_sph[p * 4u + 3u] = sqrtf(o.cvx_sph[p * 4u + 3u]);
}

// Closest point on triangle abc to p (Ericson 5.1.5), and the feature it lies
// on: 0 the face, 1..3 corner a / b / c, 4..6 edge ab / bc / ca. The feature
// is what physics_collide_one's inside test needs.
inline void cloth_closest_tri(const float* p, const float* a, const float* b, const float* c, float* o,
                              int& r) {
    float ab[3], ac[3], ap[3];
    cloth_v3_sub(b, a, ab); cloth_v3_sub(c, a, ac); cloth_v3_sub(p, a, ap);
    const float d1 = cloth_v3_dot(ab, ap), d2 = cloth_v3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) { o[0] = a[0]; o[1] = a[1]; o[2] = a[2]; r = 1; return; }
    float bp[3]; cloth_v3_sub(p, b, bp);
    const float d3 = cloth_v3_dot(ab, bp), d4 = cloth_v3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) { o[0] = b[0]; o[1] = b[1]; o[2] = b[2]; r = 2; return; }
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        const float v = d1 / (d1 - d3);
        for (int k = 0; k < 3; ++k) o[k] = a[k] + ab[k] * v;
        r = 4;
        return;
    }
    float cp[3]; cloth_v3_sub(p, c, cp);
    const float d5 = cloth_v3_dot(ab, cp), d6 = cloth_v3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) { o[0] = c[0]; o[1] = c[1]; o[2] = c[2]; r = 3; return; }
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        const float w = d2 / (d2 - d6);
        for (int k = 0; k < 3; ++k) o[k] = a[k] + ac[k] * w;
        r = 6;
        return;
    }
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        for (int k = 0; k < 3; ++k) o[k] = b[k] + (c[k] - b[k]) * w;
        r = 5;
        return;
    }
    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom, w = vc * denom;
    for (int k = 0; k < 3; ++k) o[k] = a[k] + ab[k] * v + ac[k] * w;
    r = 0;
}

// SKEL: what a build records for physics_bvh_refit - per kept triangle, in
// vp order, its three source vertices, and per corner its position-welded
// corner id and the id of the edge (k, k + 1) that starts there.
struct PhysicsBvhTopo {
    std::vector<uint32_t> src;
    std::vector<uint32_t> cid;
    std::vector<uint32_t> eid;
    uint32_t ncid = 0;
    uint32_t neid = 0;
};

// Median-split build over triangle centroids. Iterative, with an explicit
// stack: a deep recursive build on a pathological mesh is a stack overflow on
// a worker thread, and the worker has no handler.
inline void physics_bvh_build(PhysicsBvh& o, const float* pos, uint32_t nv,
                               const uint32_t* ix, uint32_t ni, PhysicsBvhTopo* topo = nullptr) {
    KH_PROF_SCOPE(PROF_W_BVH_BUILD);   // PROF.
    o.nodes.clear(); o.tri.clear(); o.vp.clear(); o.vd.clear();
    o.pose_seq = physics_pose_seq_next(); o.prev_seq = 0u; o.vd_max = 0.0f;   // COL_CLEAR: a new pose, sweeping from none.
    if (topo) { topo->src.clear(); topo->cid.clear(); topo->eid.clear(); topo->ncid = topo->neid = 0u; }
    if (!pos || !ix || ni < 3u) return;
    const uint32_t nt = ni / 3u;
    o.vp.resize(static_cast<size_t>(nt) * 9u);
    o.tri.resize(nt);
    uint32_t live = 0;
    for (uint32_t t = 0; t < nt; ++t) {
        const uint32_t i0 = ix[t * 3u], i1 = ix[t * 3u + 1u], i2 = ix[t * 3u + 2u];
        if (i0 >= nv || i1 >= nv || i2 >= nv) continue;   // A bad stream drops its triangle.
        float* d = &o.vp[static_cast<size_t>(live) * 9u];
        for (int k = 0; k < 3; ++k) {
            d[k]     = pos[i0 * 3u + k];
            d[3 + k] = pos[i1 * 3u + k];
            d[6 + k] = pos[i2 * 3u + k];
        }
        if (!cloth_finite3(d) || !cloth_finite3(d + 3) || !cloth_finite3(d + 6)) continue;
        // A triangle collinear to float precision has no surface: it can win
        // the nearest search along its neighbours' edges and then has no face
        // normal to push by, so it is dropped at build.
        {
            float cb_e1[3], cb_e2[3];
            cloth_v3_sub(d + 3, d, cb_e1);
            cloth_v3_sub(d + 6, d, cb_e2);
            const float cb_n[3] = { cb_e1[1] * cb_e2[2] - cb_e1[2] * cb_e2[1],
                                    cb_e1[2] * cb_e2[0] - cb_e1[0] * cb_e2[2],
                                    cb_e1[0] * cb_e2[1] - cb_e1[1] * cb_e2[0] };
            if (!(cloth_v3_len(cb_n) > 1.0e-6f * cloth_v3_len(cb_e1) * cloth_v3_len(cb_e2))) continue;
        }
        o.tri[live] = live;
        ++live;
        if (topo) { topo->src.push_back(i0); topo->src.push_back(i1); topo->src.push_back(i2); }
    }
    if (live == 0) { o.nodes.clear(); o.tri.clear(); o.vp.clear(); return; }
    o.tri.resize(live);
    o.vp.resize(static_cast<size_t>(live) * 9u);

    // PSEUDONORMALS (Baerentzen and Aanaes): the angle-weighted normal of every
    // corner and the summed normal of every edge, over all the faces that meet
    // there BY POSITION (a collider's hard edges and UV seams split its
    // vertices; a split corner would see only its own face). The sign of
    // (p - closest) against the pseudonormal of the feature the closest point
    // lies on is an inside test that holds everywhere on a closed mesh. A single
    // face's normal does not: past a sharp convex edge (faces turned more than
    // 90 degrees apart - the base of the builtin cone and pyramid among them) it
    // calls a point outside inside, and the push throws the cloth round the edge.
    o.pn.assign(static_cast<size_t>(live) * 18u, 0.0f);
    {
        const uint32_t nc = live * 3u;
        auto pc = [&](uint32_t i) { return &o.vp[static_cast<size_t>(i) * 3u]; };
        std::vector<uint32_t> ord(nc);
        for (uint32_t i = 0; i < nc; ++i) ord[i] = i;
        std::sort(ord.begin(), ord.end(), [&](uint32_t x, uint32_t y) {
            const float* a = pc(x);
            const float* b = pc(y);
            if (a[0] != b[0]) return a[0] < b[0];
            if (a[1] != b[1]) return a[1] < b[1];
            return a[2] < b[2];
        });
        std::vector<uint32_t> id(nc);
        uint32_t nid = 0;
        for (uint32_t i = 0; i < nc; ++i) {
            if (i > 0) {
                const float* a = pc(ord[i - 1u]);
                const float* b = pc(ord[i]);
                if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) ++nid;
            }
            id[ord[i]] = nid;
        }
        ++nid;
        std::vector<float> vpn(static_cast<size_t>(nid) * 3u, 0.0f);
        std::unordered_map<uint64_t, uint32_t> eix;
        eix.reserve(static_cast<size_t>(nc));
        std::vector<float> epn;
        std::vector<uint32_t> te(nc);   // Corner k of a triangle -> its edge (k, k + 1).
        for (uint32_t t = 0; t < live; ++t) {
            const float* v = &o.vp[static_cast<size_t>(t) * 9u];
            float e1[3], e2[3], n[3];
            cloth_v3_sub(v + 3, v, e1);
            cloth_v3_sub(v + 6, v, e2);
            n[0] = e1[1] * e2[2] - e1[2] * e2[1];
            n[1] = e1[2] * e2[0] - e1[0] * e2[2];
            n[2] = e1[0] * e2[1] - e1[1] * e2[0];
            const float nl = cloth_v3_len(n);
            if (nl > 1.0e-30f) { n[0] /= nl; n[1] /= nl; n[2] /= nl; } else { n[0] = n[1] = n[2] = 0.0f; }
            for (int k = 0; k < 3; ++k) {
                const float* p0 = v + k * 3;
                const float* p1 = v + ((k + 1) % 3) * 3;
                const float* p2 = v + ((k + 2) % 3) * 3;
                float u[3], w[3];
                cloth_v3_sub(p1, p0, u);
                cloth_v3_sub(p2, p0, w);
                const float ul = cloth_v3_len(u), wl = cloth_v3_len(w);
                float cos = (ul > 1.0e-30f && wl > 1.0e-30f) ? cloth_v3_dot(u, w) / (ul * wl) : 1.0f;
                if (cos > 1.0f) cos = 1.0f;
                if (cos < -1.0f) cos = -1.0f;
                const float ang = acosf(cos);
                float* vn = &vpn[static_cast<size_t>(id[t * 3u + k]) * 3u];
                vn[0] += n[0] * ang; vn[1] += n[1] * ang; vn[2] += n[2] * ang;
                const uint32_t ia = id[t * 3u + k], ib = id[t * 3u + (k + 1) % 3];
                const uint64_t ek = ia < ib ? (static_cast<uint64_t>(ia) << 32) | ib
                                                 : (static_cast<uint64_t>(ib) << 32) | ia;
                const auto it = eix.emplace(ek, static_cast<uint32_t>(epn.size() / 3u));
                if (it.second) { epn.push_back(0.0f); epn.push_back(0.0f); epn.push_back(0.0f); }
                float* en = &epn[static_cast<size_t>(it.first->second) * 3u];
                en[0] += n[0]; en[1] += n[1]; en[2] += n[2];
                te[t * 3u + k] = it.first->second;
            }
        }
        for (uint32_t t = 0; t < live; ++t) {
            float* pn = &o.pn[static_cast<size_t>(t) * 18u];
            for (int k = 0; k < 3; ++k) {
                memcpy(pn + k * 3, &vpn[static_cast<size_t>(id[t * 3u + k]) * 3u], sizeof(float) * 3u);
                memcpy(pn + 9 + k * 3, &epn[static_cast<size_t>(te[t * 3u + k]) * 3u], sizeof(float) * 3u);
            }
        }
        if (topo) {
            topo->cid = id;
            topo->eid = te;
            topo->ncid = nid;
            topo->neid = static_cast<uint32_t>(epn.size() / 3u);
        }
        physics_convex_build(o, id.data(), live);   // COL_CONVEX: the pieces, from the same welding.
        // COL_CONVEX_BUILD: ...and their spheres. physics_convex_build
        // leaves every piece's sphere at radius 0 about the origin; without this
        // refit a BVH that is built and never refitted (every rigid collider's,
        // physics_bvh_for, and a skinned pose's when its refit is refused)
        // culls every piece in physics_convex_one and a rigid collider marked
        // convex collides with nothing. vd is empty here: the rigid lanes.
        physics_convex_refit(o);
    }

    std::vector<float> ctr(static_cast<size_t>(live) * 3u);
    for (uint32_t t = 0; t < live; ++t) {
        const float* v = &o.vp[static_cast<size_t>(t) * 9u];
        for (int k = 0; k < 3; ++k) ctr[t * 3u + k] = (v[k] + v[3 + k] + v[6 + k]) * (1.0f / 3.0f);
    }
    o.nodes.reserve(static_cast<size_t>(live) * 2u + 1u);
    o.nodes.push_back(PhysicsBvhNode());
    struct CbTask { uint32_t node, first, count; };
    std::vector<CbTask> st;
    st.push_back(CbTask{ 0u, 0u, live });

    while (!st.empty()) {
        const CbTask cb_t = st.back();
        st.pop_back();
        PhysicsBvhNode& cb_n = o.nodes[cb_t.node];
        for (int k = 0; k < 3; ++k) { cb_n.mn[k] = 1.0e30f; cb_n.mx[k] = -1.0e30f; }
        for (uint32_t i = 0; i < cb_t.count; ++i) {
            const float* v = &o.vp[static_cast<size_t>(o.tri[cb_t.first + i]) * 9u];
            for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                if (v[c * 3 + k] < cb_n.mn[k]) cb_n.mn[k] = v[c * 3 + k];
                if (v[c * 3 + k] > cb_n.mx[k]) cb_n.mx[k] = v[c * 3 + k];
            }
        }
        if (cb_t.count <= PHYSICS_BVH_LEAF) {
            cb_n.left = cb_t.first; cb_n.count = cb_t.count;
            continue;
        }
        int ax = 0;
        {
            float ext[3];
            for (int k = 0; k < 3; ++k) ext[k] = cb_n.mx[k] - cb_n.mn[k];
            if (ext[1] > ext[ax]) ax = 1;
            if (ext[2] > ext[ax]) ax = 2;
        }
        const uint32_t mid = cb_t.count / 2u;
        std::nth_element(o.tri.begin() + cb_t.first,
                         o.tri.begin() + cb_t.first + mid,
                         o.tri.begin() + cb_t.first + cb_t.count,
                         [&](uint32_t x, uint32_t y) {
                             return ctr[x * 3u + ax] < ctr[y * 3u + ax];
                         });
        const uint32_t l = static_cast<uint32_t>(o.nodes.size());
        o.nodes.push_back(PhysicsBvhNode());
        o.nodes.push_back(PhysicsBvhNode());
        // The reference above is dangling after the push_backs - re-take it.
        o.nodes[cb_t.node].left = l;
        o.nodes[cb_t.node].count = 0u;
        st.push_back(CbTask{ l,      cb_t.first,             mid });
        st.push_back(CbTask{ l + 1u, cb_t.first + mid,  cb_t.count - mid });
    }
}

// SKEL - a BVH built with a topology record (the template), moved to new
// positions of the same vertices: the corners, the pseudonormals (the build's
// arithmetic over the build's corner and edge ids) and every node's box,
// bottom-up (a node's children always come after it). The tree keeps the
// template's splits, so a pose far from it queries slower, never wrongly. A
// triangle the template dropped stays dropped and one it kept is kept. False,
// with o not to be used, when the record does not fit o or a position is not
// finite: the caller builds instead. scr is scratch. prev, when
// given, is the same vertices at the previous pose, in the same space: o then
// carries each corner's displacement (vd) and every box holds both poses, so a
// query at any fraction between them prunes correctly.
// COL_CONVEX: planes = the pose is for the convex path alone: the
// corners, their motion, the root box and the planes are refitted, the nodes
// below the root and the pseudonormals stay the template's, and the pose is
// marked cvx_only.
inline bool physics_bvh_refit(PhysicsBvh& o, const PhysicsBvhTopo& tp, const float* pos,
                               uint32_t nv, std::vector<float>& scr, const float* prev = nullptr,
                               bool planes = false) {
    KH_PROF_SCOPE(PROF_W_BVH_REFIT);   // PROF.
    const size_t nt = o.tri.size();
    if (nt == 0u || tp.src.size() != nt * 3u || tp.cid.size() != nt * 3u ||
        tp.eid.size() != nt * 3u || o.vp.size() != nt * 9u || o.pn.size() != nt * 18u) return false;
    for (size_t t = 0; t < nt; ++t) {
        for (int c = 0; c < 3; ++c) {
            const uint32_t s = tp.src[t * 3u + c];
            if (s >= nv) return false;
            float* d = &o.vp[t * 9u + c * 3u];
            memcpy(d, pos + static_cast<size_t>(s) * 3u, sizeof(float) * 3u);
            if (!cloth_finite3(d)) return false;
        }
    }
    if (prev) {
        o.vd.resize(nt * 9u);
        for (size_t t = 0; t < nt; ++t) {
            for (int c = 0; c < 3; ++c) {
                const float* q = prev + static_cast<size_t>(tp.src[t * 3u + c]) * 3u;
                if (!cloth_finite3(q)) return false;
                for (int k = 0; k < 3; ++k) o.vd[t * 9u + c * 3u + k] = o.vp[t * 9u + c * 3u + k] - q[k];
            }
        }
    } else {
        o.vd.clear();
    }
    {   // COL_CLEAR: a refitted copy is a new pose; its largest corner sweep. The caller names the pose it sweeps from.
        o.pose_seq = physics_pose_seq_next();
        o.prev_seq = 0u;
        float m2 = 0.0f;
        for (size_t q = 0; q + 3u <= o.vd.size(); q += 3u) {
            const float* d = &o.vd[q];
            const float d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
            if (d2 > m2) m2 = d2;
        }
        o.vd_max = sqrtf(m2);
    }
    const bool sw = !o.vd.empty();
    if (planes && !o.nodes.empty()) {   // COL_CONVEX: the root box over both poses, then the planes.
        PhysicsBvhNode& rn = o.nodes[0];
        for (int k = 0; k < 3; ++k) { rn.mn[k] = 1.0e30f; rn.mx[k] = -1.0e30f; }
        for (size_t t = 0; t < nt; ++t) {
            const float* v = &o.vp[t * 9u];
            for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                const float a = v[c * 3 + k];
                const float b = sw ? a - o.vd[t * 9u + c * 3 + k] : a;
                const float lo = a < b ? a : b, hi = a < b ? b : a;
                if (lo < rn.mn[k]) rn.mn[k] = lo;
                if (hi > rn.mx[k]) rn.mx[k] = hi;
            }
        }
        physics_convex_refit(o);
        o.cvx_only = true;
        return true;
    }
    scr.assign((static_cast<size_t>(tp.ncid) + tp.neid) * 3u, 0.0f);
    float* vpn = scr.data();
    float* epn = vpn + static_cast<size_t>(tp.ncid) * 3u;
    for (size_t t = 0; t < nt; ++t) {
        const float* v = &o.vp[t * 9u];
        float e1[3], e2[3], n[3];
        cloth_v3_sub(v + 3, v, e1);
        cloth_v3_sub(v + 6, v, e2);
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        const float nl = cloth_v3_len(n);
        if (nl > 1.0e-30f) { n[0] /= nl; n[1] /= nl; n[2] /= nl; } else { n[0] = n[1] = n[2] = 0.0f; }
        for (int k = 0; k < 3; ++k) {
            const uint32_t ci = tp.cid[t * 3u + k], ei = tp.eid[t * 3u + k];
            if (ci >= tp.ncid || ei >= tp.neid) return false;
            const float* p0 = v + k * 3;
            const float* p1 = v + ((k + 1) % 3) * 3;
            const float* p2 = v + ((k + 2) % 3) * 3;
            float u[3], w[3];
            cloth_v3_sub(p1, p0, u);
            cloth_v3_sub(p2, p0, w);
            const float ul = cloth_v3_len(u), wl = cloth_v3_len(w);
            float cos = (ul > 1.0e-30f && wl > 1.0e-30f) ? cloth_v3_dot(u, w) / (ul * wl) : 1.0f;
            if (cos > 1.0f) cos = 1.0f;
            if (cos < -1.0f) cos = -1.0f;
            const float ang = acosf(cos);
            float* vn = vpn + static_cast<size_t>(ci) * 3u;
            vn[0] += n[0] * ang; vn[1] += n[1] * ang; vn[2] += n[2] * ang;
            float* en = epn + static_cast<size_t>(ei) * 3u;
            en[0] += n[0]; en[1] += n[1]; en[2] += n[2];
        }
    }
    for (size_t t = 0; t < nt; ++t) {
        float* pn = &o.pn[t * 18u];
        for (int k = 0; k < 3; ++k) {
            memcpy(pn + k * 3, vpn + static_cast<size_t>(tp.cid[t * 3u + k]) * 3u, sizeof(float) * 3u);
            memcpy(pn + 9 + k * 3, epn + static_cast<size_t>(tp.eid[t * 3u + k]) * 3u, sizeof(float) * 3u);
        }
    }
    for (size_t i = o.nodes.size(); i-- > 0u;) {
        PhysicsBvhNode& rf_n = o.nodes[i];
        for (int k = 0; k < 3; ++k) { rf_n.mn[k] = 1.0e30f; rf_n.mx[k] = -1.0e30f; }
        if (rf_n.count != 0u) {
            for (uint32_t j = 0; j < rf_n.count; ++j) {
                const size_t ti = static_cast<size_t>(o.tri[rf_n.left + j]) * 9u;
                const float* v = &o.vp[ti];
                for (int c = 0; c < 3; ++c) for (int k = 0; k < 3; ++k) {
                    const float a = v[c * 3 + k];
                    const float b = sw ? a - o.vd[ti + c * 3 + k] : a;   // The previous pose.
                    const float lo = a < b ? a : b, hi = a < b ? b : a;
                    if (lo < rf_n.mn[k]) rf_n.mn[k] = lo;
                    if (hi > rf_n.mx[k]) rf_n.mx[k] = hi;
                }
            }
        } else {
            for (uint32_t rf_c = 0; rf_c < 2u; ++rf_c) {
                const PhysicsBvhNode& rf_k = o.nodes[rf_n.left + rf_c];
                for (int k = 0; k < 3; ++k) {
                    if (rf_k.mn[k] < rf_n.mn[k]) rf_n.mn[k] = rf_k.mn[k];
                    if (rf_k.mx[k] > rf_n.mx[k]) rf_n.mx[k] = rf_k.mx[k];
                }
            }
        }
    }
    physics_convex_refit(o);   // COL_CONVEX: the planes of this pose too.
    o.cvx_only = false;
    return true;
}

// Nearest point on the collider surface to p within radius r; false when
// nothing is in reach. Iterative descent with an explicit stack (as the build).
// SKEL: vd / s, when given, place each triangle's corners at
// vp + s * vd (a deforming collider part-way through its frame).
// CLOTH_WARM: warm, when given, is the triangle this query hit last
// time (CLOTH_WARM_NONE = none). It is tested FIRST and its distance seeds
// the bound, so the walk skips every node further than it: cloth resting on a
// body hits the same triangles for hundreds of substeps, and the walk then
// visits almost nothing. The answer is the same nearest point (a seed is only
// a candidate; anything strictly nearer replaces it; at exactly equal distance
// the seed wins). On a hit the slot takes the triangle found; on a miss it
// keeps what it held.
static constexpr uint32_t CLOTH_WARM_NONE = 0xFFFFFFFFu;
inline bool physics_bvh_nearest(const PhysicsBvh& b, const float* p, float r,
                                 float* out, float& d, uint32_t& tri, int& reg,
                                 const float* vd = nullptr, float s = 0.0f, uint32_t* warm = nullptr) {
    if (b.empty()) return false;
    float best2 = r * r;
    bool  hit = false;
    if (warm && *warm != CLOTH_WARM_NONE &&
        (static_cast<size_t>(*warm) + 1u) * 9u <= b.vp.size()) {
        const uint32_t wt = *warm;
        const float* v = &b.vp[static_cast<size_t>(wt) * 9u];
        float vv[9];
        if (vd) {
            const float* dv = vd + static_cast<size_t>(wt) * 9u;
            for (int j = 0; j < 9; ++j) vv[j] = v[j] + s * dv[j];
            v = vv;
        }
        float c[3];
        int r0 = 0;
        cloth_closest_tri(p, v, v + 3, v + 6, c, r0);
        float dv[3];
        cloth_v3_sub(c, p, dv);
        const float d2 = cloth_v3_dot(dv, dv);
        if (d2 < best2) {
            best2 = d2;
            out[0] = c[0]; out[1] = c[1]; out[2] = c[2];
            tri = wt;
            reg = r0;
            hit = true;
        }
    }
    uint32_t st[64];
    int sp = 0;
    st[sp++] = 0u;
    while (sp > 0) {
        const uint32_t ni = st[--sp];
        const PhysicsBvhNode& n = b.nodes[ni];
        // Squared distance from p to the node box; a box further than the
        // current best cannot hold a nearer triangle.
        float bd = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float v = p[k] < n.mn[k] ? n.mn[k] - p[k]
                          : (p[k] > n.mx[k] ? p[k] - n.mx[k] : 0.0f);
            bd += v * v;
        }
        if (bd > best2) continue;
        if (n.count != 0u) {
            for (uint32_t i = 0; i < n.count; ++i) {
                const float* v = &b.vp[static_cast<size_t>(b.tri[n.left + i]) * 9u];
                float vv[9];
                if (vd) {
                    const float* dv = vd + static_cast<size_t>(b.tri[n.left + i]) * 9u;
                    for (int j = 0; j < 9; ++j) vv[j] = v[j] + s * dv[j];
                    v = vv;
                }
                float c[3];
                int r = 0;
                cloth_closest_tri(p, v, v + 3, v + 6, c, r);
                float dv[3];
                cloth_v3_sub(c, p, dv);
                const float d2 = cloth_v3_dot(dv, dv);
                if (d2 < best2) {
                    best2 = d2;
                    out[0] = c[0]; out[1] = c[1]; out[2] = c[2];
                    tri = b.tri[n.left + i];
                    reg = r;
                    hit = true;
                }
            }
            continue;
        }
        if (sp + 2 <= 64) {
            st[sp++] = n.left;
            st[sp++] = n.left + 1u;
        }
    }
    if (hit) d = sqrtf(best2);
    if (hit && warm) *warm = tri;
    return hit;
}

// The proportions a collider's BVH is built at. A nearest-point search is only
// METRIC when the BVH's space differs from metres by one uniform scale; the
// importer normalises every model per axis, so a non-cubic collider queried in
// normalized space finds the wrong nearest point, feature or triangle (the side
// is unaffected: the inside test is scale-invariant). The BVH is built with its
// normalized positions times these proportions - size over its largest axis,
// quantized in log2 to 1/PHYSICS_BVH_SHAPE_STEPS of an octave - leaving the
// query a scale uniform to within half a step. Returns the shape's cache key
// (10 bits per axis).
static constexpr float PHYSICS_BVH_SHAPE_STEPS = 64.0f;
inline uint32_t physics_bvh_shape(const float* size, float* bake) {
    float m = fabsf(size[0]);
    if (fabsf(size[1]) > m) m = fabsf(size[1]);
    if (fabsf(size[2]) > m) m = fabsf(size[2]);
    uint32_t key = 0;
    for (int k = 0; k < 3; ++k) {
        const float r = m > 1.0e-6f ? fabsf(size[k]) / m : 1.0f;
        float q = -log2f(r > 1.0e-3f ? r : 1.0e-3f) * PHYSICS_BVH_SHAPE_STEPS;
        q = floorf(q + 0.5f);
        if (q < 0.0f) q = 0.0f;
        if (q > 1023.0f) q = 1023.0f;
        bake[k] = exp2f(-q / PHYSICS_BVH_SHAPE_STEPS);
        key |= static_cast<uint32_t>(q) << (k * 10);
    }
    return key;
}

// The frame a fraction t of the way from a to b, exact at both ends. Centre
// and size are linear; the rotation rows are blended and re-orthonormalized
// (Gram-Schmidt, handedness taken from the blend).
inline void cloth_frame_lerp(const ClothFrame& a, const ClothFrame& b, float t, ClothFrame& o) {
    if (t <= 0.0f) { o = a; return; }
    if (t >= 1.0f) { o = b; return; }
    for (int k = 0; k < 3; ++k) {
        o.centre[k] = a.centre[k] + (b.centre[k] - a.centre[k]) * t;
        o.size[k] = a.size[k] + (b.size[k] - a.size[k]) * t;
    }
    static const float id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    if (!a.rotated && !b.rotated) { memcpy(o.rot, id, sizeof(o.rot)); o.rotated = false; return; }
    const float* ra = a.rotated ? a.rot : id;
    const float* rb = b.rotated ? b.rot : id;
    float m[9];
    for (int k = 0; k < 9; ++k) m[k] = ra[k] + (rb[k] - ra[k]) * t;
    float r0[3] = { m[0], m[1], m[2] };
    const float l0 = cloth_v3_len(r0);
    const float d01 = l0 > 1.0e-6f ? cloth_v3_dot(m + 3, r0) / (l0 * l0) : 0.0f;
    float r1[3] = { m[3] - r0[0] * d01, m[4] - r0[1] * d01, m[5] - r0[2] * d01 };
    const float l1 = cloth_v3_len(r1);
    if (l0 < 1.0e-6f || l1 < 1.0e-6f) {   // A half turn blends through zero: take the nearer end's rows.
        memcpy(o.rot, t < 0.5f ? ra : rb, sizeof(o.rot));
        o.rotated = true;
        return;
    }
    for (int k = 0; k < 3; ++k) { r0[k] /= l0; r1[k] /= l1; }
    float r2[3] = { r0[1] * r1[2] - r0[2] * r1[1], r0[2] * r1[0] - r0[0] * r1[2], r0[0] * r1[1] - r0[1] * r1[0] };
    if (cloth_v3_dot(r2, m + 6) < 0.0f) { r2[0] = -r2[0]; r2[1] = -r2[1]; r2[2] = -r2[2]; }
    memcpy(o.rot, r0, sizeof(r0)); memcpy(o.rot + 3, r1, sizeof(r1)); memcpy(o.rot + 6, r2, sizeof(r2));
    o.rotated = true;
}

// Field by field - the struct has padding, and a whole-struct compare reads it.
inline bool cloth_frame_same(const ClothFrame& a, const ClothFrame& b) {
    if (a.rotated != b.rotated) return false;
    for (int k = 0; k < 3; ++k) if (a.centre[k] != b.centre[k] || a.size[k] != b.size[k]) return false;
    if (a.rotated) for (int k = 0; k < 9; ++k) if (a.rot[k] != b.rot[k]) return false;
    return true;
}

// A frame that moved further than 'teleport' metres, or turned more than 60
// degrees, between two samples did not move: it jumped, and nothing may be
// swept across the gap. trace(Ra^T Rb) is 3 for no turn and 2 at 60 degrees.
// Each caller's first test hands in 'a' carried forward by its own velocity
// where one is known (the carrier's carrier_v in cloth_step and
// chain_step, a collider view's vel in cloth_sync), so the distance is
// the miss from where steady motion would have put it; with none known yet it
// is the plain distance moved. TELEPORT_ONCE's second test in the two steps
// hands in the unpredicted last frame's end: the plain distance moved.
inline bool cloth_frame_jumped(const ClothFrame& a, const ClothFrame& b, float teleport) {
    float d[3];
    cloth_v3_sub(b.centre, a.centre, d);
    if (cloth_v3_len(d) > (teleport > 0.0f ? teleport : 2.0f)) return true;
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
// displacement at p between two samples. ratio is f1.size / f0.size per
// axis. Formed from offsets against the centres, never from two absolute
// positions, so it keeps its precision at altitude.
inline void cloth_frame_delta(const float* p, const ClothFrame& f0, const float* ratio,
                              const ClothFrame& f1, float* o) {
    float d[3];
    cloth_v3_sub(p, f0.centre, d);
    float r[3];
    if (f0.rotated) {
        for (int k = 0; k < 3; ++k) r[k] = f0.rot[k * 3] * d[0] + f0.rot[k * 3 + 1] * d[1] + f0.rot[k * 3 + 2] * d[2];
    } else { r[0] = d[0]; r[1] = d[1]; r[2] = d[2]; }
    for (int k = 0; k < 3; ++k) r[k] *= ratio[k];
    float q[3];
    if (f1.rotated) {
        for (int k = 0; k < 3; ++k) q[k] = f1.rot[k] * r[0] + f1.rot[3 + k] * r[1] + f1.rot[6 + k] * r[2];
    } else { q[0] = r[0]; q[1] = r[1]; q[2] = r[2]; }
    for (int k = 0; k < 3; ++k) o[k] = (f1.centre[k] - f0.centre[k]) + (q[k] - d[k]);
}

// CLOTH_GROUND - the terrain under one cloth for one frame: the engine's own
// terrain heights at its grid nodes (the framework's terrain matrix), copied by
// cloth_sync into the instance's frame, and how the engine splits each grid
// cell into its two flat triangles (cloth_ground_prepare measured it).
// Between the nodes the surface is those triangles, as the engine's own is;
// split 2 is a smooth bilinear fit, used only where the split was not
// recognised. A heightfield has no far side: a particle below it always comes
// back up, it cannot tunnel.
// Terrain only: buildings, floors, bridges and rocks are not ground - cloth and chains need a physicsCollider for
// them, and a cloth or chain on an upper floor falls through to the terrain.
static constexpr int CLOTH_GROUND_PASSES = 4;   // Projections per contact; see cloth_ground_contact.
inline bool cloth_ground_contact(const ClothGround& g, float thick, float* p, float* n_out) {
    if (!g.on || g.nx < 2 || g.nz < 2 || !(g.cell > 0.0f) || g.h.size() < static_cast<size_t>(g.nx) * g.nz) return false;
    bool moved = false;
    for (int pass = 0; pass < CLOTH_GROUND_PASSES; ++pass) {
        if (!cloth_ground_plane(g, thick, p, n_out)) break;
        moved = true;
    }
    return moved;
}
// One projection onto the triangle under p (cloth_ground_contact's pass).
inline bool cloth_ground_plane(const ClothGround& g, float thick, float* p, float* n_out) {
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

inline ClothParams cloth_defaults() {
    ClothParams p;
    // drag: a hinged cape, as a flat plate (Cd 1.2) at 0.2-0.4 kg/m^2, sweeps
    // back 19-32 degrees at a 1.4 m/s walk and 48-60 at a 3 m/s jog.
    // CLOTH_DRAG: 0.735 kg/s per point at the default 0.2 kg is 3.67 /s
    // against a face's motion, which lands inside both ranges; waterDrag 12.26 is 61.3 /s (CLOTH_SKIN of
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
inline float cloth_compliance(float dial, float soft) {
    if (dial >= 0.999f) return 0.0f;              // Exactly rigid.
    if (dial <= 0.0f) return soft;
    return soft * powf(1.0e-4f, dial);
}

// Mesh-local rest position -> world, through the object's frame: the mesh is
// authored normalized to [-0.5, 0.5] and drawn at 'size', exactly the vertex
// shader's transform, so the cloth's rest pose and its drawn rest pose are the
// same points.
inline void cloth_l2w(const float* local, const float* centre, const float* rot,
                      bool rotated, const float* size, float* out) {
    const float s[3] = { local[0] * size[0],
                         local[1] * size[1],
                         local[2] * size[2] };
    if (rotated) {
        for (int k = 0; k < 3; ++k) {
            out[k] = centre[k] + rot[k] * s[0] + rot[3 + k] * s[1] + rot[6 + k] * s[2];
        }
    } else {
        for (int k = 0; k < 3; ++k) out[k] = centre[k] + s[k];
    }
}

// World -> mesh-local, the inverse of cloth_l2w. rot_m is orthonormal, so
// the inverse rotation is its transpose; a zero size axis (a non-invertible
// frame) is floored, not divided by.
inline void cloth_w2l(const float* world, const float* centre, const float* rot,
                      bool rotated, const float* inv_size, float* out) {
    const float d[3] = { world[0] - centre[0],
                         world[1] - centre[1],
                         world[2] - centre[2] };
    float r[3];
    if (rotated) {
        // Transposed here, in one place: cloth_l2w reads the rows as basis
        // vectors, so the inverse reads the same nine floats as columns.
        for (int k = 0; k < 3; ++k) {
            r[k] = rot[k * 3] * d[0] + rot[k * 3 + 1] * d[1] + rot[k * 3 + 2] * d[2];
        }
    } else { r[0] = d[0]; r[1] = d[1]; r[2] = d[2]; }
    for (int k = 0; k < 3; ++k) out[k] = r[k] * inv_size[k];
}

// COL_CONVEX - one convex collider pass over one particle: the twin of
// physics_collide_one below (same frames, outputs, standoff and friction
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
inline bool physics_convex_one(const PhysicsColliderView& c, float thick,
                               const float* prev, float* p, float* n_out, float* sv_out,
                               uint32_t* warm) {
    const PhysicsBvh& b = *c.bvh;
    float mv[3];
    cloth_v3_sub(p, prev, mv);
    const float motion = cloth_v3_len(mv);
    const float reach = thick + c.margin + motion;
    if (!c.inside) {
        float d[3]; cloth_v3_sub(p, c.bs_c, d);
        const float rr = c.bs_r + reach;
        if (cloth_v3_dot(d, d) > rr * rr) return false;
    }
    float es[3], ies[3], pes[3];
    for (int k = 0; k < 3; ++k) {
        const float bk = c.bake[k] > 0.0f ? c.bake[k] : 1.0f;
        es[k] = c.size[k] / bk;
        ies[k] = c.inv_size[k] * bk;
        pes[k] = c.prev.size[k] / bk;
    }
    float smin = es[0];
    if (es[1] < smin) smin = es[1];
    if (es[2] < smin) smin = es[2];
    if (smin < 1.0e-6f) smin = 1.0e-6f;
    const float want = thick + c.margin;
    const float rl = reach / smin;   // The reach in the planes' space, conservative.
    const bool def = c.deform && b.cvx_pl0.size() == b.cvx_pl.size() && b.cvx_pv.size() == b.cvx_tri.size() * 3u;
    const float s1 = def ? c.def1 : 0.0f;
    const float w = 1.0f + s1;   // Of the way from the previous pose to this one, at the substep's end.
    const uint32_t np = static_cast<uint32_t>(b.cvx_pstart.size() - 1u);
    const bool prev_ok = c.prev.size[0] != 0.0f || c.prev.size[1] != 0.0f || c.prev.size[2] != 0.0f;
    // One plane at the pose: unit normal and offset in the planes' space.
    auto plane = [&](uint32_t q, float* n, float& d) {
        const float* p1 = &b.cvx_pl[static_cast<size_t>(q) * 4u];
        if (!def) { n[0] = p1[0]; n[1] = p1[1]; n[2] = p1[2]; d = p1[3]; return; }
        const float* p0 = &b.cvx_pl0[static_cast<size_t>(q) * 4u];
        for (int k = 0; k < 3; ++k) n[k] = p0[k] + w * (p1[k] - p0[k]);
        d = p0[3] + w * (p1[3] - p0[3]);
        const float nl = cloth_v3_len(n);
        if (nl > 1.0e-6f) { for (int k = 0; k < 3; ++k) n[k] /= nl; d /= nl; }
        else { n[0] = p1[0]; n[1] = p1[1]; n[2] = p1[2]; d = p1[3]; }
    };
    float l[3];
    cloth_w2l(p, c.centre, c.rot, c.rotated, ies, l);
    // Resolve the particle against plane q (local n, d; its signed distance
    // sd there), in world: to the standoff on the side the collider keeps
    // cloth. False when the world distance already clears it.
    auto resolve = [&](uint32_t q, const float* nl, float sd) -> bool {
        float cl[3];
        for (int k = 0; k < 3; ++k) cl[k] = l[k] - nl[k] * sd;   // The contact point, on the plane.
        float cw[3];
        cloth_l2w(cl, c.centre, c.rot, c.rotated, es, cw);
        float nn[3] = { nl[0] * ies[0], nl[1] * ies[1], nl[2] * ies[2] };   // The normal, as the BVH path carries it.
        float nw[3];
        if (c.rotated) {
            for (int k = 0; k < 3; ++k) nw[k] = c.rot[k] * nn[0] + c.rot[3 + k] * nn[1] + c.rot[6 + k] * nn[2];
        } else { nw[0] = nn[0]; nw[1] = nn[1]; nw[2] = nn[2]; }
        const float nlen = cloth_v3_len(nw);
        if (nlen < 1.0e-12f) return false;
        for (int k = 0; k < 3; ++k) nw[k] /= nlen;
        float dv[3];
        cloth_v3_sub(p, cw, dv);
        const float sdw = cloth_v3_dot(dv, nw);   // Signed, outward.
        const float target = c.inside ? -want : want;
        if (c.inside ? sdw <= target : sdw >= target) return false;
        for (int k = 0; k < 3; ++k) p[k] += nw[k] * (target - sdw);
        for (int k = 0; k < 3; ++k) n_out[k] = c.inside ? -nw[k] : nw[k];
        sv_out[0] = sv_out[1] = sv_out[2] = 0.0f;
        if (prev_ok) {
            float c0[3], cp[3];
            if (def) {
                const float* pv = &b.cvx_pv[static_cast<size_t>(q) * 3u];
                const float cx_ds = c.def0 - s1;   // <= 0: back to the substep's start.
                for (int k = 0; k < 3; ++k) c0[k] = cl[k] + cx_ds * pv[k];
            } else { memcpy(c0, cl, sizeof(c0)); }
            cloth_l2w(c0, c.prev.centre, c.prev.rot, c.prev.rotated, pes, cp);
            for (int k = 0; k < 3; ++k) sv_out[k] = cw[k] - cp[k];
        }
        cloth_w2l(p, c.centre, c.rot, c.rotated, ies, l);   // For the next piece.
        return true;
    };
    bool hit = false;
    const uint32_t wq = warm ? *warm : CLOTH_WARM_NONE;
    // 'inside': the piece the particle is least outside of, unless one holds it.
    float in_best = 3.0e38f; uint32_t in_q = CLOTH_WARM_NONE; float in_n[3] = { 0.0f, 0.0f, 0.0f };
    for (uint32_t pc = 0; pc < np; ++pc) {
        const uint32_t q0 = b.cvx_pstart[pc], q1 = b.cvx_pstart[pc + 1u];
        if (q1 <= q0) continue;
        if (!c.inside) {
            const float* sp = &b.cvx_sph[static_cast<size_t>(pc) * 4u];
            float ds[3]; cloth_v3_sub(l, sp, ds);
            const float rr = sp[3] + rl;
            if (cloth_v3_dot(ds, ds) > rr * rr) continue;
        }
        float n[3], d;
        if (!c.inside && wq >= q0 && wq < q1) {   // The plane that separated last time.
            plane(wq, n, d);
            if ((cloth_v3_dot(n, l) - d) * smin >= want) continue;
        }
        float best = -3.0e38f, bn[3] = { 0.0f, 0.0f, 0.0f };
        uint32_t bq = CLOTH_WARM_NONE;
        bool sep = false;
        for (uint32_t q = q0; q < q1; ++q) {
            plane(q, n, d);
            const float sd = cloth_v3_dot(n, l) - d;
            if (!c.inside && sd * smin >= want) {   // A separating plane: clear of this piece.
                if (warm) *warm = q;
                sep = true;
                break;
            }
            if (sd > best) { best = sd; bq = q; bn[0] = n[0]; bn[1] = n[1]; bn[2] = n[2]; }
        }
        if (sep || bq == CLOTH_WARM_NONE) continue;
        if (c.inside) {
            if (best * smin <= -want) return hit;   // Held by this piece.
            if (best < in_best) { in_best = best; in_q = bq; memcpy(in_n, bn, sizeof(in_n)); }
            continue;
        }
        if (warm) *warm = bq;
        if (resolve(bq, bn, best)) hit = true;
    }
    if (c.inside && in_q != CLOTH_WARM_NONE) {
        if (resolve(in_q, in_n, in_best)) hit = true;
    }
    return hit;
}

// COL_CLEAR - the collision pass's clearance cache. A cloth hanging from a
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
// corner sweep times the sweep fraction covered (PhysicsBvh::vd_max), summed
// per collider column as its poses chain (col_clear_advance): within a pose
// by the fraction, from a pose to the one that sweeps from it (prev_seq) by
// what was left of the old and what is covered of the new; a pose that does
// not chain from the last one used (a cut, a rebuilt BVH, a skipped pose)
// empties the column. Everything is a lower bound in the BVH's units, carried
// to metres by the smallest residual scale as the query's reach is, with
// COL_CLEAR_PAD_M and COL_CLEAR_SLACK over the standoff for the residual
// scale's anisotropy and float rounding. A skipped call is one the queries
// would have answered 'false' with the particle untouched; only the warm slot
// is not refreshed (a seed, not an answer). A rebase, a teleport or the
// non-finite rescue move the particle far in the model frame and spend the
// record; an origin shift moves the particles and the collider views together
// (cloth_origin_shift), so the record stands.
// Not for a containing ('inside') collider nor the convex path.
// COL_CLEAR_ON false = every call queries.
static constexpr bool  COL_CLEAR_ON = true;
static constexpr float COL_CLEAR_PAD_M = 0.005f;
static constexpr float COL_CLEAR_SLACK = 0.98f;
static constexpr float COL_CLEAR_ACC_MAX = 1000.0f;   // The column's sum restarts here: a float's differences stay exact to 0.1 mm.
// One collider pass over one particle. Returns true when the particle was
// moved, so the caller can apply friction against the same contact:
// n_out is the push direction and sv_out how far the surface under
// the contact moved this substep (the view's prev pose to its current one).
// prev is the particle's position at the start of the substep. The query
// radius covers the distance travelled as well as the thickness, or a
// particle moving faster than its thickness per substep tunnels through.
inline bool physics_collide_one(const PhysicsColliderView& c, float thick,
                                 const float* prev, float* p, float* n_out, float* sv_out,
                                 uint32_t* warm = nullptr,   // CLOTH_WARM: the particle's last-hit slot for this collider.
                                 ColClear* cc = nullptr, float acc = 0.0f) {   // COL_CLEAR: the pair's record and the column's sum.
    if (!c.bvh || c.bvh->empty()) return false;
    // COL_CONVEX: a collider marked convex, or a pose refitted for the
    // planes alone (whose nodes below the root are stale), takes the planes.
    if ((c.convex || c.bvh->cvx_only) && c.bvh->cvx_valid()) {
        return physics_convex_one(c, thick, prev, p, n_out, sv_out, warm);
    }
    float mv[3];
    cloth_v3_sub(p, prev, mv);
    const float motion = cloth_v3_len(mv);
    const float reach = thick + c.margin + motion;
    // Broadphase: the collider's world bounding sphere against the particle.
    // Only in the ordinary OUTSIDE sense - a containing collider has to reach
    // particles that have escaped it, which are by definition the ones this
    // test would reject.
    if (!c.inside) {
        float d[3]; cloth_v3_sub(p, c.bs_c, d);
        const float rr = c.bs_r + reach;
        if (cloth_v3_dot(d, d) > rr * rr) return false;
    }
    // What carries the BVH's space to metres: the size less the proportions
    // already baked into it - uniform to within half a quantization step, which
    // is what makes the nearest search below a metric one.
    float es[3], ies[3], pes[3];
    for (int k = 0; k < 3; ++k) {
        const float b = c.bake[k] > 0.0f ? c.bake[k] : 1.0f;
        es[k] = c.size[k] / b;
        ies[k] = c.inv_size[k] * b;
        pes[k] = c.prev.size[k] / b;
    }
    float l[3];
    cloth_w2l(p, c.centre, c.rot, c.rotated, ies, l);
    // The reach is metric on entry, so it converts through the SMALLEST axis
    // of what is left - conservative, never a missed contact.
    float smin = es[0];
    if (es[1] < smin) smin = es[1];
    if (es[2] < smin) smin = es[2];
    if (smin < 1.0e-6f) smin = 1.0e-6f;
    const float r = reach / smin;
    // COL_CLEAR: the particle in the collider's model frame; a record that
    // still clears the standoff answers the call.
    const float lm[3] = { l[0] + c.skin_ctr[0], l[1] + c.skin_ctr[1], l[2] + c.skin_ctr[2] };
    if (c.inside) cc = nullptr;
    if (cc && cc->d > 0.0f) {
        const float m[3] = { lm[0] - cc->l[0], lm[1] - cc->l[1], lm[2] - cc->l[2] };
        const float left = (cc->d - sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]) -
                            (acc - cc->acc)) * smin;
        if (left * COL_CLEAR_SLACK > thick + c.margin + COL_CLEAR_PAD_M) return false;
        cc->d = 0.0f;   // Spent (a NaN lands here too): query.
    }
    // SKEL: a deforming collider is queried at the substep's end pose.
    const bool def = c.deform && !c.bvh->vd.empty() && c.bvh->vd.size() == c.bvh->vp.size();
    const float* vd = def ? c.bvh->vd.data() : nullptr;
    const float s1 = def ? c.def1 : 0.0f;
    float cl[3]; float co_d = 0.0f; uint32_t t = 0u; int reg = 0;
    if (!physics_bvh_nearest(*c.bvh, l, r, cl, co_d, t, reg, vd, s1, warm)) {
        // Nothing within reach of the surface: usually 'far outside', but also
        // what a DEEPLY EMBEDDED particle looks like (spawned inside, or carried
        // in by a teleport), and leaving that one alone strands it inside the
        // collider forever. The collider's root box tells the two apart, so the
        // wider query is paid for only by the particles inside it.
        const PhysicsBvhNode& root = c.bvh->nodes[0];
        float out2 = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float e = l[k] < root.mn[k] ? root.mn[k] - l[k]
                          : (l[k] > root.mx[k] ? l[k] - root.mx[k] : 0.0f);
            out2 += e * e;
        }
        // Outside the root box and not a container: genuinely nowhere near.
        if (out2 > 0.0f && !c.inside) return false;
        float diag = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float e = root.mx[k] - root.mn[k];
            diag += e * e;
        }
        diag = sqrtf(diag) + sqrtf(out2);
        if (!physics_bvh_nearest(*c.bvh, l, diag, cl, co_d, t, reg, vd, s1, warm)) return false;
    }

    // Back to world, and do the whole decision there: a BVH-space distance is
    // a depth only to within the residual scale.
    float cw[3];
    cloth_l2w(cl, c.centre, c.rot, c.rotated, es, cw);
    // The hit triangle's OUTWARD normal, carried to world through the same
    // frame. Inside and outside come from the winding alone: a collider must be
    // a closed, consistently wound mesh (meshgen::bake winds every triangle to
    // its authored normal, so an outward-normalled model arrives outward-wound;
    // an open or inside-out one pushes cloth the wrong way, undetected). A
    // non-uniform size takes the inverse transpose: for a diagonal scale, divide
    // by size where the point multiplied by it.
    const float* v = &c.bvh->vp[static_cast<size_t>(t) * 9u];
    float vv[9];
    if (def) {   // SKEL: the hit triangle where the query found it.
        const float* dv = vd + static_cast<size_t>(t) * 9u;
        for (int j = 0; j < 9; ++j) vv[j] = v[j] + s1 * dv[j];
        v = vv;
    }
    float e1[3], e2[3], nl[3];
    cloth_v3_sub(v + 3, v, e1);
    cloth_v3_sub(v + 6, v, e2);
    nl[0] = e1[1] * e2[2] - e1[2] * e2[1];
    nl[1] = e1[2] * e2[0] - e1[0] * e2[2];
    nl[2] = e1[0] * e2[1] - e1[1] * e2[0];
    for (int k = 0; k < 3; ++k) nl[k] *= ies[k];
    float nw[3];
    if (c.rotated) {
        for (int k = 0; k < 3; ++k) {
            nw[k] = c.rot[k] * nl[0] + c.rot[3 + k] * nl[1] + c.rot[6 + k] * nl[2];
        }
    } else { nw[0] = nl[0]; nw[1] = nl[1]; nw[2] = nl[2]; }
    const float nlen = cloth_v3_len(nw);
    if (nlen < 1.0e-12f) return false;   // Degenerate triangle: no side to be on.
    for (int k = 0; k < 3; ++k) nw[k] /= nlen;
    if (c.inside) for (int k = 0; k < 3; ++k) nw[k] = -nw[k];

    const float want = thick + c.margin;
    float dv[3];
    cloth_v3_sub(p, cw, dv);
    if (reg == 0) {
        // In a face's interior the face normal IS the gradient of the
        // distance, so the contact is the plane: signed depth along it.
        const float sd = cloth_v3_dot(dv, nw);
        if (sd >= want) {                       // Already clear on the outside.
            if (cc) { memcpy(cc->l, lm, sizeof(cc->l)); cc->d = co_d; cc->acc = acc; }   // COL_CLEAR.
            return false;
        }
        const float push = want - sd;
        for (int k = 0; k < 3; ++k) p[k] += nw[k] * push;
        n_out[0] = nw[0]; n_out[1] = nw[1]; n_out[2] = nw[2];
    } else {
        // On an edge or a corner the face normal is neither the side nor the
        // direction. The side is the pseudonormal's (physics_bvh_build),
        // tested in local space where the closest point was found (the sign
        // survives the size's positive scale). The direction is from the surface
        // to the particle, the distance's gradient there, so a contact rounds the
        // edge at exactly the standoff instead of being pushed along one face's
        // normal.
        const float* pn = (c.bvh->pn.size() >= (static_cast<size_t>(t) + 1u) * 18u)
                        ? &c.bvh->pn[static_cast<size_t>(t) * 18u +
                                     (reg <= 3 ? (reg - 1) * 3 : 9 + (reg - 4) * 3)]
                        : nl;
        float lv[3];
        cloth_v3_sub(l, cl, lv);
        const bool out = cloth_v3_dot(lv, pn) >= 0.0f;   // Outside the mesh.
        const bool ok = c.inside ? !out : out;          // On the side cloth belongs.
        const float dl = cloth_v3_len(dv);
        float u[3];
        float sd;
        if (dl > 1.0e-7f) {
            const float s = ok ? 1.0f / dl : -1.0f / dl;
            for (int k = 0; k < 3; ++k) u[k] = dv[k] * s;
            sd = ok ? dl : -dl;
        } else {
            // On the surface itself: no direction to read, so the
            // pseudonormal's, carried to world the way the face normal is.
            float pl[3] = { pn[0] * ies[0], pn[1] * ies[1],
                            pn[2] * ies[2] };
            if (c.rotated) {
                for (int k = 0; k < 3; ++k) {
                    u[k] = c.rot[k] * pl[0] + c.rot[3 + k] * pl[1] + c.rot[6 + k] * pl[2];
                }
            } else { u[0] = pl[0]; u[1] = pl[1]; u[2] = pl[2]; }
            const float ul = cloth_v3_len(u);
            if (ul < 1.0e-12f) { u[0] = nw[0]; u[1] = nw[1]; u[2] = nw[2]; }
            else {
                const float s = c.inside ? -1.0f / ul : 1.0f / ul;
                for (int k = 0; k < 3; ++k) u[k] *= s;
            }
            sd = 0.0f;
        }
        if (sd >= want) {   // Outside (sd is positive on the cloth's side alone).
            if (cc) { memcpy(cc->l, lm, sizeof(cc->l)); cc->d = co_d; cc->acc = acc; }   // COL_CLEAR.
            return false;
        }
        for (int k = 0; k < 3; ++k) p[k] = cw[k] + u[k] * want;
        n_out[0] = u[0]; n_out[1] = u[1]; n_out[2] = u[2];
    }
    // Where the contact point was at the start of the substep. Zero for a
    // still collider, and for a view whose prev was never filled.
    sv_out[0] = sv_out[1] = sv_out[2] = 0.0f;
    if (c.prev.size[0] != 0.0f || c.prev.size[1] != 0.0f || c.prev.size[2] != 0.0f) {
        float cp[3];
        if (def) {
            // SKEL: the SAME point of the surface at the substep's start (the
            // contact's barycentric place on the hit triangle, carried back by
            // those corners' own motion), so the surface's motion under the
            // contact is the limb's as well as the root's, and friction carries
            // cloth with a swing.
            float b[3] = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };
            {
                float q[3];
                cloth_v3_sub(cl, v, q);
                const float d00 = cloth_v3_dot(e1, e1), d01 = cloth_v3_dot(e1, e2);
                const float d11 = cloth_v3_dot(e2, e2);
                const float d20 = cloth_v3_dot(q, e1), d21 = cloth_v3_dot(q, e2);
                const float den = d00 * d11 - d01 * d01;
                if (den > 1.0e-20f) {
                    float bv = (d11 * d20 - d01 * d21) / den;
                    float bw = (d00 * d21 - d01 * d20) / den;
                    float bu = 1.0f - bv - bw;
                    bu = bu > 0.0f ? bu : 0.0f;
                    bv = bv > 0.0f ? bv : 0.0f;
                    bw = bw > 0.0f ? bw : 0.0f;
                    const float bs = bu + bv + bw;
                    if (bs > 1.0e-12f) { b[0] = bu / bs; b[1] = bv / bs; b[2] = bw / bs; }
                }
            }
            const float* dv = vd + static_cast<size_t>(t) * 9u;
            const float ds = c.def0 - s1;   // <= 0: back to the substep's start.
            float c0[3];
            for (int k = 0; k < 3; ++k) {
                c0[k] = cl[k] + ds * (b[0] * dv[k] + b[1] * dv[3 + k] + b[2] * dv[6 + k]);
            }
            cloth_l2w(c0, c.prev.centre, c.prev.rot, c.prev.rotated, pes, cp);
        } else {
            cloth_l2w(cl, c.prev.centre, c.prev.rot, c.prev.rotated, pes, cp);
        }
        for (int k = 0; k < 3; ++k) sv_out[k] = cw[k] - cp[k];
    }
    return true;
}

// COL_CLEAR: one collider column at one substep - its surface-motion sum
// advanced along the pose chain. False = the column's records are void (the
// caller empties them): not a cacheable collider, a pose that does not chain
// from the last one used, or the sum at its restart.
inline bool col_clear_advance(ColClearCol& c, const PhysicsColliderView& v) {
    const PhysicsBvh* const b = v.bvh;
    const bool ok = COL_CLEAR_ON && b && !b->empty() && b->pose_seq != 0u && !v.inside &&
                    !((v.convex || b->cvx_only) && b->cvx_valid());
    // The sweep fraction the query reads the surface at (physics_collide_one's def).
    const bool def = ok && v.deform && !b->vd.empty() && b->vd.size() == b->vp.size();
    const float s = def ? v.def1 : 0.0f;
    bool chain = ok && c.on;
    if (chain) {
        if (c.seq == b->pose_seq) c.acc += fabsf(s - c.s) * b->vd_max;
        else if (b->prev_seq != 0u && b->prev_seq == c.seq) c.acc += fabsf(c.s) * c.vd_max + (1.0f + s) * b->vd_max;
        else chain = false;
        if (chain && !(c.acc < COL_CLEAR_ACC_MAX)) chain = false;
    }
    if (!chain) c.acc = 0.0f;
    c.on = ok;
    c.seq = ok ? b->pose_seq : 0u;
    c.s = s;
    c.vd_max = ok ? b->vd_max : 0.0f;
    return chain;
}
// Nearest pinned particle along the mesh, for every particle, plus that path's
// rest length. A multi-source Dijkstra over the stretch edges - not a BFS,
// because edge rest lengths differ and a hop count is not a distance.
inline void cloth_build_tethers(std::vector<ClothPart>& part,
                                const std::vector<ClothDist>& edge) {
    const size_t n = part.size();
    if (n == 0) return;
    std::vector<uint32_t> head(n + 1u, 0u);
    for (size_t e = 0; e < edge.size(); ++e) {
        if (edge[e].a >= n || edge[e].b >= n) continue;
        ++head[edge[e].a + 1u];
        ++head[edge[e].b + 1u];
    }
    for (size_t i = 0; i < n; ++i) head[i + 1u] += head[i];
    std::vector<uint32_t> adj(head[n]);
    std::vector<float>    len(head[n]);
    {
        std::vector<uint32_t> fill(head.begin(), head.end() - 1);
        for (size_t e = 0; e < edge.size(); ++e) {
            const uint32_t a = edge[e].a, b = edge[e].b;
            if (a >= n || b >= n) continue;
            adj[fill[a]] = b; len[fill[a]] = edge[e].rest; ++fill[a];
            adj[fill[b]] = a; len[fill[b]] = edge[e].rest; ++fill[b];
        }
    }
    std::vector<float> ct_d(n, 1.0e30f);
    struct CtNode { float d; uint32_t v; };
    struct CtWorse { bool operator()(const CtNode& x, const CtNode& y) const { return x.d > y.d; } };
    std::vector<CtNode> q;
    for (size_t i = 0; i < n; ++i) {
        part[i].anchor = static_cast<uint32_t>(i);
        part[i].tether = -1.0f;
        if (part[i].w <= 0.0f) {
            ct_d[i] = 0.0f;
            q.push_back(CtNode{ 0.0f, static_cast<uint32_t>(i) });
        }
    }
    std::make_heap(q.begin(), q.end(), CtWorse());
    while (!q.empty()) {
        std::pop_heap(q.begin(), q.end(), CtWorse());
        const CtNode c = q.back();
        q.pop_back();
        if (c.d > ct_d[c.v] + 1.0e-9f) continue;   // A stale heap entry.
        for (uint32_t e = head[c.v]; e < head[c.v + 1u]; ++e) {
            const uint32_t to = adj[e];
            const float nd = c.d + len[e];
            if (nd + 1.0e-9f >= ct_d[to]) continue;
            ct_d[to] = nd;
            part[to].anchor = part[c.v].anchor;
            q.push_back(CtNode{ nd, to });
            std::push_heap(q.begin(), q.end(), CtWorse());
        }
    }
    for (size_t i = 0; i < n; ++i) {
        if (part[i].w > 0.0f && ct_d[i] < 1.0e29f) part[i].tether = ct_d[i];
    }
}

// SKEL - particle i's rigid-follow target, mesh-local, at fraction t of the
// step's span: the skeleton's guide from where the last substep ended (guide_a,
// SUB_CLOCK) to the frame's end (guide_b), or the rest position when there
// is none. tmp receives an interpolated target; read the pointer returned.
inline const float* cloth_target(const ClothState& s, size_t i, float t, float* tmp) {
    if (s.guide_b.size() != s.part.size() * 3u || s.guide_a.size() != s.guide_b.size()) {
        return s.part[i].rest;
    }
    const float* b = &s.guide_b[i * 3u];
    if (t >= 1.0f) return b;
    const float* a = &s.guide_a[i * 3u];
    for (int k = 0; k < 3; ++k) tmp[k] = a[k] + (b[k] - a[k]) * t;
    return tmp;
}

// Place every particle at its rigid-follow target in the current frame - its
// rest pose, or where a skeleton's guide puts it (cloth_target) - and clear
// all motion. The cold start, and the recovery from every pathology.
inline void cloth_reset(ClothState& s) {
    for (size_t i = 0; i < s.part.size(); ++i) {
        ClothPart& p = s.part[i];
        float g[3];
        cloth_l2w(cloth_target(s, i, s.guide_t1, g), s.centre, s.rot, s.rotated,
                  s.size, p.p);
        for (int k = 0; k < 3; ++k) p.pp[k] = p.p[k];
    }
    s.accum = 0.0f;
    s.h_last = 0.0f;   // SIM_LOD: no displacement stands, so no length was read.
    s.primed = true;
    s.carrier_vok = false;   // What moved the cloth here is not a velocity.
    s.carrier_vreb = false;   // TELEPORT_ONCE.
}

// Carry the whole particle set rigidly from one frame to another. A teleport is
// not motion: the cloth arrives with the object instead of being whipped across
// the map by a one-frame velocity, and p AND pp move together, so its internal
// shape is preserved.
inline void cloth_rebase(ClothState& s,
                         const float* c0, const float* r0, bool rot0, const float* is0,
                         const float* c1, const float* r1, bool rot1, const float* sz1) {
    for (size_t i = 0; i < s.part.size(); ++i) {
        ClothPart& p = s.part[i];
        float l[3], lp[3];
        cloth_w2l(p.p,  c0, r0, rot0, is0, l);
        cloth_w2l(p.pp, c0, r0, rot0, is0, lp);
        cloth_l2w(l,  c1, r1, rot1, sz1, p.p);
        cloth_l2w(lp, c1, r1, rot1, sz1, p.pp);
    }
}

// How far past the separation a self-collision pair is gathered, as a multiple
// of it (cloth_self_gather).
static constexpr float CLOTH_SELF_REACH = 1.5f;
// The most relative motion per substep the reach allows for, in mean rest
// edges (cloth_self_gather).
static constexpr float CLOTH_SELF_MOTION = 2.0f;
// CLOTH_SELF_WIDE: the most hash cells a wide face or edge is filed into
// (cloth_self_gather); one covering more takes no part that substep.
static constexpr uint32_t CLOTH_SELF_WIDE_CELLS = 1024u;
// A crossing deeper than this in one substep, in mean rest edges, is not
// pushed back (cloth_self_solve).
static constexpr float CLOTH_SELF_DEEP = 0.5f;
// The jolt stand-down (cloth_step): a change of velocity at any corner of
// the carrier's box above this in one frame (m/s; no physical carrier comes
// near it - a 30 g crash is 5 m/s a frame at 60 fps) turns self-collision off
// for this long (s).
static constexpr float CLOTH_SELF_JOLT = 10.0f;
static constexpr float CLOTH_SELF_HOLD = 1.0f;

// Closest points of segments p0-p1 and q0-q1 (Ericson 5.1.9): the parameters
// along each, s and t in [0, 1], and the points. Parallel segments take s = 0
// and the matching t, which is one of the nearest pairs.
inline void cloth_closest_seg(const float* p0, const float* p1, const float* q0, const float* q1,
                              float& s, float& t, float* cp, float* cq) {
    float d1[3], d2[3], r[3];
    cloth_v3_sub(p1, p0, d1);
    cloth_v3_sub(q1, q0, d2);
    cloth_v3_sub(p0, q0, r);
    const float a = cloth_v3_dot(d1, d1), e = cloth_v3_dot(d2, d2), f = cloth_v3_dot(d2, r);
    auto clamp01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };
    if (a <= 1.0e-20f && e <= 1.0e-20f) {
        s = 0.0f; t = 0.0f;
    } else if (a <= 1.0e-20f) {
        s = 0.0f; t = clamp01(f / e);
    } else {
        const float c = cloth_v3_dot(d1, r);
        if (e <= 1.0e-20f) {
            t = 0.0f; s = clamp01(-c / a);
        } else {
            const float b = cloth_v3_dot(d1, d2);
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
inline bool cloth_seg_tri(const float* p, const float* q, const float* a, const float* b, const float* c) {
    float e1[3], e2[3], d[3], s[3], h[3], k[3];
    cloth_v3_sub(b, a, e1); cloth_v3_sub(c, a, e2); cloth_v3_sub(q, p, d);
    h[0] = d[1] * e2[2] - d[2] * e2[1]; h[1] = d[2] * e2[0] - d[0] * e2[2]; h[2] = d[0] * e2[1] - d[1] * e2[0];
    const float det = cloth_v3_dot(e1, h);
    if (!(fabsf(det) > 1.0e-20f)) return false;   // Parallel, or no area.
    const float inv = 1.0f / det;
    cloth_v3_sub(p, a, s);
    const float u = cloth_v3_dot(s, h) * inv;
    if (!(u >= 0.0f && u <= 1.0f)) return false;
    k[0] = s[1] * e1[2] - s[2] * e1[1]; k[1] = s[2] * e1[0] - s[0] * e1[2]; k[2] = s[0] * e1[1] - s[1] * e1[0];
    const float v = cloth_v3_dot(d, k) * inv;
    if (!(v >= 0.0f && u + v <= 1.0f)) return false;
    const float t = cloth_v3_dot(e2, k) * inv;
    return t >= 0.0f && t <= 1.0f;
}

// SELF-COLLISION, the gather. Every pair of the same cloth that can touch this
// substep, found once after the prediction and solved with everything else in
// the iterations (cloth_self_solve). Two kinds (Bridson et al. 2002): a
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
// CLOTH_FORK - a fork-join on the cloth pool's own workers. The posters: a
// cloth job's self-collision gather (cloth_self_gather), attach_step's
// skin fork (SKIN_FORK) and the render thread's per-object loops
// (fork_each, INJ_FORK). A poster with a loop of independent chunks posts
// it here; every idle pool worker (a solver worker waiting on g_cloth_cv, or a
// skin worker on g_skin_cv - POOL_SPLIT - with no job of its own queue to
// take) takes chunks until none is left, and the poster's own thread takes
// chunks too, then waits for the last helper's to finish. A worker with a job
// of its own never helps (its own queue's jobs come first in cloth_worker):
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
// and the pool size are the runtime's (cloth_fork_wake,
// cloth_fork_workers); the core only posts.
// ONE FORK AT A TIME. The record is one: two jobs posting at once would write
// each other's count and counter, and one would wait for ever on a total reset
// under it, its instance's busy flag never cleared. A job takes g_cloth_fork_mu
// for the whole of its fork (cloth_fork_run); a job that finds it taken runs
// its chunks serially on its own thread instead of waiting (no worker idles on another's fork; the result is the
// same: a chunk's output does not depend on who ran it). Helpers never take the
// mutex. The only lock taken inside it is g_cloth_mu, by the wake, momentarily;
// nothing blocks on g_cloth_fork_mu while holding another lock (attach_step's
// skin fork only try-locks it, under g_draw_list_mutex).
struct ClothFork {
    std::atomic<uint64_t> next{ 0 };        // gen << 40 | n << 20 | the next chunk to hand out; 0 between forks.
    std::atomic<uint32_t> gen_posted{ 0 };  // The generation posted; 0 = none.
    std::atomic<uint32_t> done{ 0 };        // Chunks finished (a chunk that threw counts: it is over).
    std::atomic<uint32_t> failed{ 0 };      // Chunks that threw; the initiator rethrows for them after the join.
    std::atomic<uint32_t> pids{ 1 };        // The next participant id (0 is the initiator's).
    // Participant ids with scratch. Atomic: a helper that read the previous
    // fork's generation may read it while the next fork's initiator writes it
    // (a stale value only admits or refuses a pid it never runs a chunk with).
    std::atomic<uint32_t> cap{ 0 };
    uint32_t gen = 0;
    void (*fn)(void*, uint32_t, uint32_t) = nullptr;   // (ctx, chunk, participant).
    void* ctx = nullptr;
    // SKIN_FORK: a helper leaves this fork between chunks for a queued job of
    // its own pool's queue - a cloth step, or a skin job for a skin worker (the
    // initiator finishes what is left).
    std::atomic<bool> yield_cloth{ false };
    // FORK_PRIO: the initiator's thread priority. A helper runs its chunks
    // at it when that is higher than its own, so the render thread never
    // waits on a chunk whose worker a same-priority thread preempted
    // (priority inversion); restored when the helper leaves the fork.
    std::atomic<int> prio{ THREAD_PRIORITY_NORMAL };
};
static ClothFork g_cloth_fork;
// Cloth steps queued and not yet taken (g_cloth_q's size, kept beside it under
// g_cloth_mu): read lock-free by a yielding helper.
static std::atomic<uint32_t> g_cloth_q_n{ 0 };
static std::atomic<uint32_t> g_skin_q_n{ 0 };    // POOL_SPLIT: g_skin_q's size, the same way, for a skin helper's yield.
static std::mutex g_cloth_fork_mu;   // Held by the one job whose fork is posted.
static constexpr uint32_t CLOTH_FORK_MAXN = (1u << 20) - 1u;   // Chunks a handout word can name.
inline uint32_t cloth_fork_gen_of(uint64_t v) { return static_cast<uint32_t>(v >> 40); }
inline uint32_t cloth_fork_n_of(uint64_t v)   { return static_cast<uint32_t>(v >> 20) & CLOTH_FORK_MAXN; }
inline uint32_t cloth_fork_i_of(uint64_t v)   { return static_cast<uint32_t>(v) & CLOTH_FORK_MAXN; }
// An idle worker's helping: chunks until none is left. Returns the generation
// it served (0 = none was posted): a worker serves a generation ONCE and then
// waits for the next, never re-entering a fork it has left (a re-entry would
// take a fresh participant id for no chunk, and a worker looping on a
// still-posted fork would spin for the fork's whole length).
// POOL_SPLIT: skin = the helper is a skin worker; it yields to a queued
// skin job as a solver worker yields to a queued cloth step.
inline uint32_t cloth_fork_help(uint32_t served, bool skin = false) {
    const uint32_t g = g_cloth_fork.gen_posted.load(std::memory_order_acquire);
    if (g == 0u || g == served) return served;
    const uint32_t pid = g_cloth_fork.pids.fetch_add(1u, std::memory_order_acq_rel);
    if (pid >= g_cloth_fork.cap.load(std::memory_order_relaxed)) return g;   // No scratch for one more: the initiator finishes.
    // FORK_PRIO: raised for the fork, restored on every way out below.
    const HANDLE fh_th = GetCurrentThread();
    const int fh_own = GetThreadPriority(fh_th);
    const int want = g_cloth_fork.prio.load(std::memory_order_relaxed);
    const bool boost = fh_own != THREAD_PRIORITY_ERROR_RETURN && want != THREAD_PRIORITY_ERROR_RETURN &&
                       want > fh_own && SetThreadPriority(fh_th, want) != 0;
    struct FhPrio {
        HANDLE th; int own; bool on;
        ~FhPrio() { if (on) SetThreadPriority(th, own); }
    } restore{ fh_th, fh_own, boost };
    for (;;) {
        if (g_cloth_fork.yield_cloth.load(std::memory_order_acquire) &&
            (skin ? g_skin_q_n : g_cloth_q_n).load(std::memory_order_relaxed) != 0u) break;   // SKIN_FORK: a job of this worker's own queue waits.
        // FORK_CAS: a chunk is claimed only from this generation's word. Do
        // not use a plain fetch_add here: a helper preempted between its last
        // chunk and the add (the previous initiator sees every chunk done,
        // returns, a new fork posts) would take the NEXT fork's chunk 0 and
        // leave on the generation test without running it, and the new
        // initiator waits for ever.
        uint64_t v = g_cloth_fork.next.load(std::memory_order_acquire);
        if (cloth_fork_gen_of(v) != g) break;   // Another fork's word, or none: nothing of this one is left.
        const uint32_t i = cloth_fork_i_of(v);
        if (i >= cloth_fork_n_of(v)) break;   // The count of THIS word, never the record's.
        if (!g_cloth_fork.next.compare_exchange_weak(v, v + 1u, std::memory_order_acq_rel,
                                                     std::memory_order_acquire)) continue;   // Raced: look again.
        // A chunk that throws (an allocation) must still count as finished, or
        // the initiator waits for ever; cloth_fork_run rethrows on the
        // initiator's thread - for the gather, a cloth job's worker, where
        // cloth_worker's catch clears the instance's busy flag.
        try { g_cloth_fork.fn(g_cloth_fork.ctx, i, pid); }
        catch (...) { g_cloth_fork.failed.fetch_add(1u, std::memory_order_acq_rel); }
        g_cloth_fork.done.fetch_add(1u, std::memory_order_acq_rel);
    }
    return g;
}
// The initiator's side: chunks 0..n-1 of fn, in parallel when the pool can
// help, serially (in order, as participant 0) when it cannot. Returns once
// every chunk has finished.
inline void cloth_fork_run(uint32_t n, uint32_t cap, void (*fn)(void*, uint32_t, uint32_t), void* ctx,
                           bool fr_yield = false) {
    // n must stay below CLOTH_FORK_MAXN: the initiator's last add leaves the
    // index at n + 1, which at n == MAXN would carry into the word's count and
    // hand chunk 0 out again.
    if (n <= 1u || n >= CLOTH_FORK_MAXN || cap < 2u || !g_cloth_fork_mu.try_lock()) {   // Small, alone, or another job's fork is up: serial.
        for (uint32_t i = 0; i < n; ++i) fn(ctx, i, 0u);
        return;
    }
    std::lock_guard<std::mutex> hold(g_cloth_fork_mu, std::adopt_lock);   // Released when the fork is done.
    ClothFork& f = g_cloth_fork;
    f.fn = fn;
    f.ctx = ctx;
    f.cap.store(cap, std::memory_order_relaxed);   // Published by gen_posted's release.
    f.done.store(0u, std::memory_order_relaxed);
    f.failed.store(0u, std::memory_order_relaxed);
    f.pids.store(1u, std::memory_order_relaxed);
    f.yield_cloth.store(fr_yield, std::memory_order_relaxed);   // Published by gen_posted's release.
    f.prio.store(GetThreadPriority(GetCurrentThread()), std::memory_order_relaxed);   // FORK_PRIO.
    f.gen = (f.gen + 1u) & 0xFFFFFFu;   // 24 bits, never 0 (0 is 'no fork').
    if (f.gen == 0u) f.gen = 1u;
    const uint32_t g = f.gen;
    f.next.store((static_cast<uint64_t>(g) << 40) | (static_cast<uint64_t>(n) << 20), std::memory_order_release);
    f.gen_posted.store(g, std::memory_order_release);
    if (fr_yield) cloth_fork_wake_try(); else cloth_fork_wake();
    for (;;) {
        const uint64_t v = f.next.fetch_add(1u, std::memory_order_acq_rel);
        const uint32_t i = cloth_fork_i_of(v);
        if (cloth_fork_gen_of(v) != g || i >= n) break;
        try { fn(ctx, i, 0u); }
        catch (...) { f.failed.fetch_add(1u, std::memory_order_acq_rel); }
        f.done.fetch_add(1u, std::memory_order_acq_rel);
    }
    while (f.done.load(std::memory_order_acquire) < n) std::this_thread::yield();
    f.next.store(0u, std::memory_order_release);   // Every later handout reads 'no fork'.
    f.gen_posted.store(0u, std::memory_order_release);
    if (f.failed.load(std::memory_order_acquire) != 0u) throw std::bad_alloc();   // The one thing a chunk can throw (a list growing); <new> is in every unit.
}
// A chunk lambda through the fork's plain function pointer.
template <class FcF> inline void cloth_fork_call(void* c, uint32_t i, uint32_t p) {
    (*static_cast<FcF*>(c))(i, p);
}
static constexpr uint32_t FORK_EACH_CHUNK = 64u;
template <class FeF> inline void fork_each(uint32_t n, FeF&& fn) {
    const uint32_t w = cloth_fork_workers();
    if (n < FORK_EACH_MIN || w < 2u) {
        for (uint32_t i = 0; i < n; ++i) fn(i);
        return;
    }
    const uint32_t chunks = (n + FORK_EACH_CHUNK - 1u) / FORK_EACH_CHUNK;
    auto body = [&](uint32_t c, uint32_t) {
        const uint32_t a = c * FORK_EACH_CHUNK;
        const uint32_t b = a + FORK_EACH_CHUNK < n ? a + FORK_EACH_CHUNK : n;
        for (uint32_t i = a; i < b; ++i) fn(i);
    };
    // yield: the wake is the non-blocking one (it never waits on g_cloth_mu,
    // which the game thread holds across a chain build) and a helper leaves for a
    // queued job of its own pool's queue (a cloth step, or a skin job for a skin
    // worker); either way the caller runs what is left.
    // EA_FORK_EACH_CAP: the caller (the render thread, or the game thread in a parked flush) is no pool worker, so
    // every worker may help: workers + 1 participants, as attach_step's fork (the body has no per-participant
    // scratch).
    cloth_fork_run(chunks, w + 1u, &cloth_fork_call<decltype(body)>, &body, true);
}
// Elements (faces + edges) from which the gather forks; below it the chunking
// costs more than it saves. Queries per chunk.
static constexpr size_t   CLOTH_FORK_MIN   = 4096u;
static constexpr uint32_t CLOTH_FORK_CHUNK = 256u;

// A pair is taken if its two ends started within CLOTH_SELF_REACH times the
// separation plus their predicted relative motion. The reach exceeds one
// separation because the constraint solve carries particles further than the
// prediction: a pair pushed inside the separation while unlisted is pushed out
// next substep, a bounce that never sleeps. The gather's cost grows with the
// square of the reach. The motion term is capped at CLOTH_SELF_MOTION mean
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
// made while self-collision is stood down - cloth_step) is taken as the
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
// mean edges whose box reaches past an ordinary element's (CLOTH_SELF_WIDE)
// spans as many as its box covers, up to CLOTH_SELF_WIDE_CELLS.
inline void cloth_self_gather(ClothState& sg_s, float thick) {
    sg_s.self_c.clear();
    sg_s.self_e.clear();
    const size_t n = sg_s.part.size();
    const size_t nt = sg_s.tri.size();
    if (n == 0 || nt == 0 || !(thick > 0.0f)) return;

    // The surface's edges, each with a face it borders: topology, so once.
    if (sg_s.self_edges_nt != nt) {
        sg_s.self_edges.clear();
        std::unordered_map<uint64_t, uint8_t> have;
        have.reserve(nt * 2u);
        for (size_t t = 0; t < nt; ++t) {
            const uint32_t v[3] = { sg_s.tri[t].a, sg_s.tri[t].b, sg_s.tri[t].c };
            if (v[0] >= n || v[1] >= n || v[2] >= n) continue;
            for (int e = 0; e < 3; ++e) {
                const uint32_t x = v[e], y = v[(e + 1) % 3];
                const uint64_t sg_k = x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
                if (!have.emplace(sg_k, 1u).second) continue;
                ClothSelfEdge ed;
                ed.a = x; ed.b = y; ed.f = static_cast<uint32_t>(t);
                sg_s.self_edges.push_back(ed);
            }
        }
        sg_s.self_edges_nt = static_cast<uint32_t>(nt);
    }
    const size_t ne = sg_s.self_edges.size();

    // Metric rest positions, rebuilt only when the size moves.
    if (sg_s.self_rest.size() != n * 3u || sg_s.self_rest_size[0] != sg_s.size[0] ||
        sg_s.self_rest_size[1] != sg_s.size[1] || sg_s.self_rest_size[2] != sg_s.size[2]) {
        sg_s.self_rest.resize(n * 3u);
        for (size_t i = 0; i < n; ++i) {
            for (int k = 0; k < 3; ++k) sg_s.self_rest[i * 3u + k] = sg_s.part[i].rest[k] * sg_s.size[k];
        }
        for (int k = 0; k < 3; ++k) sg_s.self_rest_size[k] = sg_s.size[k];
        double sum = 0.0;
        for (size_t e = 0; e < ne; ++e) {
            float d[3];
            cloth_v3_sub(&sg_s.self_rest[sg_s.self_edges[e].b * 3u], &sg_s.self_rest[sg_s.self_edges[e].a * 3u], d);
            sum += cloth_v3_len(d);
        }
        sg_s.self_edge = ne ? static_cast<float>(sum / static_cast<double>(ne)) : 0.0f;
    }
    auto rp = [&sg_s](uint32_t i) { return &sg_s.self_rest[i * 3u]; };
    auto cross = [](const float* x, const float* y, float* o) {
        o[0] = x[1] * y[2] - x[2] * y[1]; o[1] = x[2] * y[0] - x[0] * y[2]; o[2] = x[0] * y[1] - x[1] * y[0];
    };
    auto fnormal = [&](const ClothTri& sg_t, bool rest, float* o) {
        const float* a = rest ? rp(sg_t.a) : sg_s.part[sg_t.a].pp;
        const float* b = rest ? rp(sg_t.b) : sg_s.part[sg_t.b].pp;
        const float* c = rest ? rp(sg_t.c) : sg_s.part[sg_t.c].pp;
        float e1[3], e2[3];
        cloth_v3_sub(b, a, e1); cloth_v3_sub(c, a, e2);
        cross(e1, e2, o);
    };
    // The separation a pair holds, from its rest offset and a normal of the
    // surface there; negative = not a pair.
    auto sep = [thick](const float* rd, float rl, const float* fn) -> float {
        if (rl >= 2.0f * thick) return thick;
        if (rl < 1.0e-4f) return -1.0f;   // Coincident at rest: no direction to hold.
        const float fl = cloth_v3_len(fn);
        const bool layer = fl > 1.0e-12f &&
                           fabsf(cloth_v3_dot(rd, fn)) >= 0.7071f * rl * fl;
        if (!layer) return 0.5f * rl;   // A neighbour along the surface.
        return rl < thick ? rl : thick;
    };
    const float mvcap = CLOTH_SELF_MOTION * fmaxf(sg_s.self_edge, thick);
    auto dv = [&sg_s](uint32_t x, uint32_t y, float* o) {   // Relative motion of x against y this substep.
        for (int k = 0; k < 3; ++k) o[k] = (sg_s.part[x].p[k] - sg_s.part[x].pp[k]) - (sg_s.part[y].p[k] - sg_s.part[y].pp[k]);
    };

    // The cloth's mean motion this substep, taken out of every box below.
    double md[3] = { 0.0, 0.0, 0.0 };
    for (size_t i = 0; i < n; ++i) {
        for (int k = 0; k < 3; ++k) md[k] += sg_s.part[i].p[k] - sg_s.part[i].pp[k];
    }
    const float d[3] = { static_cast<float>(md[0] / static_cast<double>(n)),
                         static_cast<float>(md[1] / static_cast<double>(n)),
                         static_cast<float>(md[2] / static_cast<double>(n)) };
    auto boxof = [&](const uint32_t* v, int nv, float grow, float* sg_b) {
        for (int k = 0; k < 3; ++k) { sg_b[k] = 3.0e38f; sg_b[3 + k] = -3.0e38f; }
        for (int j = 0; j < nv; ++j) {
            const ClothPart& p = sg_s.part[v[j]];
            for (int k = 0; k < 3; ++k) {
                const float x0 = p.pp[k], x1 = p.p[k] - d[k];
                sg_b[k] = fminf(sg_b[k], fminf(x0, x1));
                sg_b[3 + k] = fmaxf(sg_b[3 + k], fmaxf(x0, x1));
            }
        }
        for (int k = 0; k < 3; ++k) { sg_b[k] -= grow; sg_b[3 + k] += grow; }
    };

    // Boxes: the faces, then the edges - their own sweeps, not grown. The reach
    // is added to the QUERY's box instead (the same overlap test; each element
    // sits in as few cells as it covers). An element sweeping further than the
    // motion the reach allows for, beyond its own size, is left out of this
    // substep: the reach could not list a pair for it anyway.
    // CLOTH_SELF_WIDE: the own size is the larger of the element's two ends'
    // extents on each axis, never taken below two mean edges. Do not take two
    // mean edges for every element: a face larger than that standing still is
    // then left out, and another layer passes through it. A WIDE element (a box
    // past what an ordinary one can reach) is filed into every cell its box
    // covers, at the cell the ordinary elements set: it adds entries of its own
    // and grows the cell for no query. One covering more than
    // CLOTH_SELF_WIDE_CELLS cells is left out.
    const float fast = mvcap + 2.0f * sg_s.self_edge;   // The ordinary element's reach.
    auto toofast = [&](const float* sg_b, const float* own) {
        for (int k = 0; k < 3; ++k) {
            if (sg_b[3 + k] - sg_b[k] > mvcap + fmaxf(own[k], 2.0f * sg_s.self_edge)) return true;
        }
        return false;
    };
    auto ownof = [&](const uint32_t* v, int nv, float* sg_o) {
        for (int k = 0; k < 3; ++k) {
            float a0 = 3.0e38f, a1 = -3.0e38f, b0 = 3.0e38f, b1 = -3.0e38f;
            for (int j = 0; j < nv; ++j) {
                const ClothPart& p = sg_s.part[v[j]];
                const float x0 = p.pp[k], x1 = p.p[k] - d[k];
                a0 = fminf(a0, x0); a1 = fmaxf(a1, x0);
                b0 = fminf(b0, x1); b1 = fmaxf(b1, x1);
            }
            sg_o[k] = fmaxf(a1 - a0, b1 - b0);
        }
    };
    const float own0[3] = { 0.0f, 0.0f, 0.0f };   // A particle's.
    const size_t nel = nt + ne;
    sg_s.self_box.resize(nel * 6u);
    sg_s.self_wide.clear();
    float ext = 0.0f;
    auto place = [&](size_t sg_el, const uint32_t* v, int nv) {
        float* sg_b = &sg_s.self_box[sg_el * 6u];
        float own[3];
        boxof(v, nv, 0.0f, sg_b);
        if (sg_b[3] - sg_b[0] <= fast && sg_b[4] - sg_b[1] <= fast &&
            sg_b[5] - sg_b[2] <= fast) {   // The ordinary element (the tests below would pass it).
            for (int k = 0; k < 3; ++k) ext = fmaxf(ext, sg_b[3 + k] - sg_b[k]);
            return;
        }
        ownof(v, nv, own);
        if (toofast(sg_b, own)) {
            for (int k = 0; k < 3; ++k) { sg_b[k] = 1.0f; sg_b[3 + k] = -1.0f; }
            return;
        }
        if (sg_b[3] - sg_b[0] > fast || sg_b[4] - sg_b[1] > fast ||
            sg_b[5] - sg_b[2] > fast) {
            sg_s.self_wide.push_back(static_cast<uint32_t>(sg_el));   // Filed once the cell is known.
            return;
        }
        for (int k = 0; k < 3; ++k) ext = fmaxf(ext, sg_b[3 + k] - sg_b[k]);
    };
    for (size_t t = 0; t < nt; ++t) {
        float* sg_b = &sg_s.self_box[t * 6u];
        const uint32_t v[3] = { sg_s.tri[t].a, sg_s.tri[t].b, sg_s.tri[t].c };
        if (v[0] >= n || v[1] >= n || v[2] >= n) {
            for (int k = 0; k < 3; ++k) { sg_b[k] = 1.0f; sg_b[3 + k] = -1.0f; }   // Empty: never overlaps.
            continue;
        }
        place(t, v, 3);
    }
    for (size_t e = 0; e < ne; ++e) {
        const uint32_t v[2] = { sg_s.self_edges[e].a, sg_s.self_edges[e].b };
        place(nt + e, v, 2);
    }
    float cell = fmaxf(sg_s.self_edge, 2.0f * thick);
    cell = fmaxf(cell, 0.25f * ext);
    if (!(cell > 1.0e-4f)) cell = 1.0e-4f;
    const float ic = 1.0f / cell;
    auto cells = [ic](const float* sg_b, int* sg_lo, int* sg_hi) {
        for (int k = 0; k < 3; ++k) {
            sg_lo[k] = static_cast<int>(floorf(sg_b[k] * ic));
            sg_hi[k] = static_cast<int>(floorf(sg_b[3 + k] * ic));
        }
    };
    // CLOTH_SELF_WIDE: the wide elements' cells at that cell; over the cap, left out. The table grows by what
    // they add, so the ordinary elements' buckets stay as sparse as without them.
    // CLOTH_SELF_WIDE_BOUND: a wide element stands still at any size, so a runaway solve short of the non-finite
    // rescue can hand one a box past int range in cells (the conversion is UB) or cells whose product wraps int64
    // back under the cap (the table grows to its limit and the fill walks billions of cells). Each axis's cells are
    // held within +-2e9 (short of int range, so the fill's increments stay defined) and against the cap before the
    // product - the same verdict for every box inside that bound whose cells are in order; a box outside that bound is
    // left out as one over the cap is.
    size_t wide_n = 0u;
    for (uint32_t w : sg_s.self_wide) {
        float* sg_b = &sg_s.self_box[static_cast<size_t>(w) * 6u];
        int64_t wc = 1;
        for (int k = 0; k < 3 && wc > 0; ++k) {
            const float l = floorf(sg_b[k] * ic), h = floorf(sg_b[3 + k] * ic);
            if (!(l > -2.0e9f && l < 2.0e9f && h > -2.0e9f && h < 2.0e9f)) {   // NaN too.
                wc = 0;
                break;
            }
            const int64_t n = static_cast<int64_t>(static_cast<int>(h)) - static_cast<int>(l) + 1;
            wc = n > static_cast<int64_t>(CLOTH_SELF_WIDE_CELLS) ? 0 : wc * n;   // n < 1: out.
        }
        if (wc <= 0 || wc > static_cast<int64_t>(CLOTH_SELF_WIDE_CELLS)) {
            for (int k = 0; k < 3; ++k) { sg_b[k] = 1.0f; sg_b[3 + k] = -1.0f; }
            continue;
        }
        wide_n += static_cast<size_t>(wc);
    }

    uint32_t tn = 256u;
    while (tn < (nel + wide_n) * 2u && tn < (1u << 24)) tn <<= 1;
    const uint32_t mask = tn - 1u;
    auto hash = [mask](int x, int y, int z) -> uint32_t {
        return ((static_cast<uint32_t>(x) * 73856093u) ^ (static_cast<uint32_t>(y) * 19349663u) ^
                (static_cast<uint32_t>(z) * 83492791u)) & mask;
    };

    // Counting sort of (element, cell) into the table: count, prefix, fill.
    // After the fill each head is its bucket's start and the next its end.
    sg_s.self_head.assign(static_cast<size_t>(tn) + 1u, 0u);
    for (int pass = 0; pass < 2; ++pass) {
        if (pass == 1) {
            for (uint32_t b = 1; b <= tn; ++b) sg_s.self_head[b] += sg_s.self_head[b - 1u];
            sg_s.self_ent.resize(sg_s.self_head[tn]);
        }
        for (size_t el = 0; el < nel; ++el) {
            const float* sg_b = &sg_s.self_box[el * 6u];
            if (sg_b[0] > sg_b[3]) continue;
            int lo[3], hi[3];
            cells(sg_b, lo, hi);
            for (int x = lo[0]; x <= hi[0]; ++x) for (int y = lo[1]; y <= hi[1]; ++y) for (int z = lo[2]; z <= hi[2]; ++z) {
                const uint32_t bk = hash(x, y, z);
                if (pass == 0) ++sg_s.self_head[bk];
                else sg_s.self_ent[--sg_s.self_head[bk]] = static_cast<uint32_t>(el);
            }
        }
    }
    // An element in several cells, or two cells on one bucket, is met more than
    // once by a query; the stamp (the querying particle, n + the querying edge,
    // or n + edges + the tangle test's edge, plus one) takes it once.
    // CLOTH_FORK: the three query loops below run in chunks of
    // CLOTH_FORK_CHUNK queries, across the pool when the element count earns
    // it; each participant stamps its own copy of the table (stamps are unique
    // per query, so the copies never disagree). A chunk's pairs go to its own
    // list and the lists are joined in chunk order, so the pair lists - and the
    // solve's sweep order - are exactly the serial loop's. self_tang is the
    // participants' marks joined.
    const uint32_t cap0 = cloth_fork_workers();
    const bool     par = cap0 >= 2u && nel >= CLOTH_FORK_MIN;
    const uint32_t cap = par ? cap0 : 1u;
    if (sg_s.self_seen_par.size() < cap) sg_s.self_seen_par.resize(cap);
    if (sg_s.self_tang_par.size() < cap) sg_s.self_tang_par.resize(cap);
    for (uint32_t pi = 0; pi < cap; ++pi) {
        sg_s.self_seen_par[pi].assign(nel, 0u);
        sg_s.self_tang_par[pi].assign(n, 0u);
    }
    auto chunks = [par](size_t nq) -> uint32_t {
        if (!par || nq == 0u) return 1u;
        return static_cast<uint32_t>((nq + CLOTH_FORK_CHUNK - 1u) / CLOTH_FORK_CHUNK);
    };
    auto range = [par](size_t nq, uint32_t sg_c, uint32_t& sg_lo, uint32_t& sg_hi) {
        if (!par) { sg_lo = 0u; sg_hi = static_cast<uint32_t>(nq); return; }
        sg_lo = sg_c * CLOTH_FORK_CHUNK;
        const size_t sg_e = static_cast<size_t>(sg_lo) + CLOTH_FORK_CHUNK;
        sg_hi = static_cast<uint32_t>(sg_e < nq ? sg_e : nq);
    };
    auto overlap = [](const float* x, const float* y) {
        return !(x[0] > y[3] || x[3] < y[0] || x[1] > y[4] || x[4] < y[1] || x[2] > y[5] || x[5] < y[2]);
    };
    // Every element numbered in [el0, el1) in the cells of box
    // qb, once, handed to fn. The range is tested first: a query
    // for faces walks past the edges in its cells, and an edge past the
    // edges it has already been paired with, for one integer compare each.
    auto query = [&](uint32_t* seen, const float* qb, uint32_t stamp, uint32_t el0, uint32_t el1, auto&& fn) {
        int lo[3], hi[3];
        cells(qb, lo, hi);
        for (int x = lo[0]; x <= hi[0]; ++x) for (int y = lo[1]; y <= hi[1]; ++y) for (int z = lo[2]; z <= hi[2]; ++z) {
            const uint32_t bk = hash(x, y, z);
            for (uint32_t e = sg_s.self_head[bk]; e < sg_s.self_head[bk + 1u]; ++e) {
                const uint32_t el = sg_s.self_ent[e];
                if (el < el0 || el >= el1) continue;
                if (seen[el] == stamp) continue;
                seen[el] = stamp;
                if (overlap(qb, &sg_s.self_box[el * 6u])) fn(el);
            }
        }
    };

    // Tangles (above): the particles of every edge through a face at the start.
    {
        auto tangle = [&](uint32_t sg_c, uint32_t pid) {
            uint32_t* const seen = sg_s.self_seen_par[pid].data();
            uint8_t* const tang = sg_s.self_tang_par[pid].data();
            uint32_t sg_lo, sg_hi;
            range(ne, sg_c, sg_lo, sg_hi);
            for (uint32_t e = sg_lo; e < sg_hi; ++e) {
                const float* const eb = &sg_s.self_box[(nt + e) * 6u];
                if (eb[0] > eb[3]) continue;
                const ClothSelfEdge& E = sg_s.self_edges[e];
                query(seen, eb, static_cast<uint32_t>(n + ne) + e + 1u, 0u, static_cast<uint32_t>(nt), [&](uint32_t t) {
                    const ClothTri& T = sg_s.tri[t];
                    if (E.a == T.a || E.a == T.b || E.a == T.c || E.b == T.a || E.b == T.b || E.b == T.c) return;
                    if (!cloth_seg_tri(sg_s.part[E.a].pp, sg_s.part[E.b].pp,
                                       sg_s.part[T.a].pp, sg_s.part[T.b].pp, sg_s.part[T.c].pp)) return;
                    tang[E.a] = tang[E.b] = 1u;
                    tang[T.a] = tang[T.b] = tang[T.c] = 1u;
                });
            }
        };
        cloth_fork_run(chunks(ne), cap, &cloth_fork_call<decltype(tangle)>, &tangle);
        // The marks, joined: participant 0's array is the one every test reads.
        sg_s.self_tang.swap(sg_s.self_tang_par[0]);
        for (uint32_t pi = 1; pi < cap; ++pi) {
            const std::vector<uint8_t>& sg_t = sg_s.self_tang_par[pi];
            for (size_t i = 0; i < n; ++i) sg_s.self_tang[i] |= sg_t[i];
        }
    }
    auto tg = [&sg_s](uint32_t x) { return sg_s.self_tang[x] != 0u; };

    // Particle against face.
    {
    const uint32_t nc = chunks(n);
    if (sg_s.self_c_par.size() < nc) sg_s.self_c_par.resize(nc);
    auto pf = [&](uint32_t sg_c, uint32_t pid) {
    uint32_t* const seen = sg_s.self_seen_par[pid].data();
    std::vector<ClothSelfC>& out = sg_s.self_c_par[sg_c];
    out.clear();
    uint32_t sg_lo, sg_hi;
    range(n, sg_c, sg_lo, sg_hi);
    for (uint32_t q = sg_lo; q < sg_hi; ++q) {
        const ClothPart& Q = sg_s.part[q];
        float qb[6];
        boxof(&q, 1, 0.0f, qb);
        if (toofast(qb, own0)) continue;
        for (int k = 0; k < 3; ++k) { qb[k] -= CLOTH_SELF_REACH * thick; qb[3 + k] += CLOTH_SELF_REACH * thick; }
        query(seen, qb, q + 1u, 0u, static_cast<uint32_t>(nt), [&](uint32_t t) {
            const ClothTri& sg_t = sg_s.tri[t];
            if (q == sg_t.a || q == sg_t.b || q == sg_t.c) return;
            const ClothPart& A = sg_s.part[sg_t.a];
            const ClothPart& B = sg_s.part[sg_t.b];
            const ClothPart& C = sg_s.part[sg_t.c];
            if (Q.w + A.w + B.w + C.w <= 0.0f) return;   // Nothing here can move.
            if (tg(q) && (tg(sg_t.a) || tg(sg_t.b) || tg(sg_t.c))) return;   // In a tangle.
            float n0[3];
            fnormal(sg_t, false, n0);
            const float nl = cloth_v3_len(n0);
            if (!(nl > 1.0e-12f)) return;   // No area at the start: no plane, no side.
            float mv = 0.0f;
            const uint32_t tv[3] = { sg_t.a, sg_t.b, sg_t.c };
            for (int j = 0; j < 3; ++j) { float r[3]; dv(q, tv[j], r); mv = fmaxf(mv, cloth_v3_len(r)); }
            float cp[3], qd[3];
            int reg = 0;
            cloth_closest_tri(Q.pp, A.pp, B.pp, C.pp, cp, reg);
            cloth_v3_sub(Q.pp, cp, qd);
            if (cloth_v3_len(qd) > CLOTH_SELF_REACH * thick + fminf(mv, mvcap)) return;
            float sp = thick;
            {
                float fn[3];
                fnormal(sg_t, true, fn);
                if (cloth_v3_len(fn) > 1.0e-12f) {
                    float rcp[3], rd[3];
                    int rr = 0;
                    cloth_closest_tri(rp(q), rp(sg_t.a), rp(sg_t.b), rp(sg_t.c), rcp, rr);
                    cloth_v3_sub(rp(q), rcp, rd);
                    sp = sep(rd, cloth_v3_len(rd), fn);
                    if (sp < 0.0f) return;
                }
            }
            float ap[3];
            cloth_v3_sub(Q.pp, A.pp, ap);
            ClothSelfC pc;
            pc.q = q;
            pc.t = t;
            const float s0 = cloth_v3_dot(ap, n0) / nl;
            pc.side = s0 < 0.0f ? -1.0f : 1.0f;
            pc.sep = sp;
            pc.dep = 0.0f;
            pc.s0 = fabsf(s0);
            out.push_back(pc);
        });
    }
    };
    cloth_fork_run(nc, cap, &cloth_fork_call<decltype(pf)>, &pf);
    if (nc == 1u) sg_s.self_c.swap(sg_s.self_c_par[0]);
    else for (uint32_t sg_c = 0; sg_c < nc; ++sg_c) {
        sg_s.self_c.insert(sg_s.self_c.end(), sg_s.self_c_par[sg_c].begin(), sg_s.self_c_par[sg_c].end());
    }
    }

    // Edge against edge, each pair once (the second index the larger).
    {
    const uint32_t nc = chunks(ne);
    if (sg_s.self_e_par.size() < nc) sg_s.self_e_par.resize(nc);
    auto ee = [&](uint32_t sg_c, uint32_t pid) {
    uint32_t* const seen = sg_s.self_seen_par[pid].data();
    std::vector<ClothSelfE>& out = sg_s.self_e_par[sg_c];
    out.clear();
    uint32_t sg_lo, sg_hi;
    range(ne, sg_c, sg_lo, sg_hi);
    for (uint32_t e0 = sg_lo; e0 < sg_hi; ++e0) {
        const ClothSelfEdge& E0 = sg_s.self_edges[e0];
        float* const qb = &sg_s.self_box[(nt + e0) * 6u];   // Its own box; empty when too fast.
        if (qb[0] > qb[3]) continue;
        float qg[6];
        for (int k = 0; k < 3; ++k) { qg[k] = qb[k] - CLOTH_SELF_REACH * thick; qg[3 + k] = qb[3 + k] + CLOTH_SELF_REACH * thick; }
        query(seen, qg, static_cast<uint32_t>(n) + e0 + 1u, static_cast<uint32_t>(nt) + e0 + 1u,
              static_cast<uint32_t>(nel), [&](uint32_t el) {
            const uint32_t e1 = el - static_cast<uint32_t>(nt);
            const ClothSelfEdge& E1 = sg_s.self_edges[e1];
            if (E1.a == E0.a || E1.a == E0.b || E1.b == E0.a || E1.b == E0.b) return;
            const ClothPart& A = sg_s.part[E0.a];
            const ClothPart& B = sg_s.part[E0.b];
            const ClothPart& C = sg_s.part[E1.a];
            const ClothPart& D = sg_s.part[E1.b];
            if (A.w + B.w + C.w + D.w <= 0.0f) return;
            if ((tg(E0.a) || tg(E0.b)) && (tg(E1.a) || tg(E1.b))) return;   // In a tangle.
            float mv = 0.0f;
            const uint32_t sg_x[2] = { E0.a, E0.b }, sg_y[2] = { E1.a, E1.b };
            for (int i = 0; i < 2; ++i) for (int j = 0; j < 2; ++j) {
                float r[3]; dv(sg_x[i], sg_y[j], r); mv = fmaxf(mv, cloth_v3_len(r));
            }
            float s, t, c0[3], c1[3], n0[3];
            cloth_closest_seg(A.pp, B.pp, C.pp, D.pp, s, t, c0, c1);
            cloth_v3_sub(c0, c1, n0);
            const float nl = cloth_v3_len(n0);
            if (nl > CLOTH_SELF_REACH * thick + fminf(mv, mvcap)) return;
            if (!(nl > 1.0e-7f)) return;   // Touching at the start: no direction to keep.
            float sp = thick;
            {
                float r0[3], r1[3], rd[3], fn[3];
                cloth_closest_seg(rp(E0.a), rp(E0.b), rp(E1.a), rp(E1.b), s, t, r0, r1);
                cloth_v3_sub(r0, r1, rd);
                fnormal(sg_s.tri[E0.f], true, fn);
                sp = sep(rd, cloth_v3_len(rd), fn);
                if (sp < 0.0f) return;
            }
            ClothSelfE pe;
            pe.e0 = e0;
            pe.e1 = e1;
            for (int k = 0; k < 3; ++k) pe.n[k] = n0[k] / nl;
            pe.sep = sp;
            pe.dep = 0.0f;
            pe.d0 = nl;
            out.push_back(pe);
        });
    }
    };
    cloth_fork_run(nc, cap, &cloth_fork_call<decltype(ee)>, &ee);
    if (nc == 1u) sg_s.self_e.swap(sg_s.self_e_par[0]);
    else for (uint32_t sg_c = 0; sg_c < nc; ++sg_c) {
        sg_s.self_e.insert(sg_s.self_e.end(), sg_s.self_e_par[sg_c].begin(), sg_s.self_e_par[sg_c].end());
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
// crossing deeper than CLOTH_SELF_DEEP mean edges in one substep is left
// where it is: only a yank drives one layer that far through another, and
// pushing it back would fight the tethers pulling it through.
// The correction (Bridson et al. 2002, as a PBD constraint): the direction held
// fixed for the step, each side's corners moving by their barycentric shares of
// the nearest point, all by inverse mass, so momentum is kept and a pinned side
// does not move - and no corner moves further than the correction itself (the
// bare shares divide by the barycentric weight, which flings the free corner of
// a nearly pinned face); what such a face cannot take stays unresolved. With
// mu > 0 the sweep also applies each contact's friction once, from the
// separation it restored over the whole substep: Coulomb, as the collider pass,
// on the relative motion of the two nearest points.
inline void cloth_self_solve(ClothState& ss_s, float mu) {
    // One contact, whatever its kind: points x (weights wx) against points y
    // (weights wy) along the unit direction g, at separation h.
    auto apply = [mu](ClothPart* const* x, const float* wx, int nx, ClothPart* const* y,
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
        if (mu <= 0.0f || !(dep > 0.0f)) return;
        float m[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < nx; ++i) for (int k = 0; k < 3; ++k) m[k] += wx[i] * (x[i]->p[k] - x[i]->pp[k]);
        for (int i = 0; i < ny; ++i) for (int k = 0; k < 3; ++k) m[k] -= wy[i] * (y[i]->p[k] - y[i]->pp[k]);
        const float mn = cloth_v3_dot(m, g);
        float tn[3];
        for (int k = 0; k < 3; ++k) tn[k] = m[k] - g[k] * mn;
        const float tl = cloth_v3_len(tn);
        if (tl < 1.0e-12f) return;
        const float cone = mu * dep;
        const float stop = (tl <= cone ? 1.0f : cone / tl) / Wd;   // Share of it removed, per Wd.
        for (int i = 0; i < nx; ++i) for (int k = 0; k < 3; ++k) x[i]->p[k] -= tn[k] * stop * wx[i] * x[i]->w;
        for (int i = 0; i < ny; ++i) for (int k = 0; k < 3; ++k) y[i]->p[k] += tn[k] * stop * wy[i] * y[i]->w;
    };

    for (size_t i = 0; i < ss_s.self_c.size(); ++i) {
        ClothSelfC& c = ss_s.self_c[i];
        const ClothTri& ss_t = ss_s.tri[c.t];
        ClothPart* const q[1] = { &ss_s.part[c.q] };
        ClothPart* const f[3] = { &ss_s.part[ss_t.a], &ss_s.part[ss_t.b], &ss_s.part[ss_t.c] };
        float e1[3], e2[3], n[3];
        cloth_v3_sub(f[1]->p, f[0]->p, e1);
        cloth_v3_sub(f[2]->p, f[0]->p, e2);
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        const float nl = cloth_v3_len(n);
        if (!(nl > 1.0e-12f)) continue;
        float cp[3];
        int reg = 0;
        cloth_closest_tri(q[0]->p, f[0]->p, f[1]->p, f[2]->p, cp, reg);
        // The nearest point's barycentric shares, which weight the corners.
        float d[3];
        cloth_v3_sub(cp, f[0]->p, d);
        const float d00 = cloth_v3_dot(e1, e1), d01 = cloth_v3_dot(e1, e2);
        const float d11 = cloth_v3_dot(e2, e2);
        const float d20 = cloth_v3_dot(d, e1), d21 = cloth_v3_dot(d, e2);
        const float den = d00 * d11 - d01 * d01;
        if (!(den > 0.0f)) continue;
        float v = (d11 * d20 - d01 * d21) / den;
        float w = (d00 * d21 - d01 * d20) / den;
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        w = w < 0.0f ? 0.0f : (w > 1.0f - v ? 1.0f - v : w);
        const float bw[3] = { 1.0f - v - w, v, w };
        const float one[1] = { 1.0f };
        float ss_g[3], ss_h, qd[3];
        cloth_v3_sub(q[0]->p, cp, qd);
        if (reg == 0) {
            for (int k = 0; k < 3; ++k) ss_g[k] = n[k] / nl * c.side;
            ss_h = cloth_v3_dot(qd, ss_g);
            if (ss_h < 0.0f) {   // The far side: through the face, or round it?
                const float tau = c.s0 / (c.s0 - ss_h);
                float pt[4][3];
                ClothPart* const all[4] = { q[0], f[0], f[1], f[2] };
                for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) {
                    pt[j][k] = all[j]->pp[k] + (all[j]->p[k] - all[j]->pp[k]) * tau;
                }
                float ct[3], dt[3];
                int rt = 0;
                cloth_closest_tri(pt[0], pt[1], pt[2], pt[3], ct, rt);
                cloth_v3_sub(pt[0], ct, dt);
                if (rt != 0 || !(cloth_v3_len(dt) <= c.sep) ||
                    -ss_h > CLOTH_SELF_DEEP * ss_s.self_edge) {
                    // Round it - or through so far that only a yank did it: its side is where it is now.
                    c.side = -c.side;
                    for (int k = 0; k < 3; ++k) ss_g[k] = -ss_g[k];
                    ss_h = -ss_h;
                }
            }
        } else {
            ss_h = cloth_v3_len(qd);
            if (!(ss_h > 1.0e-7f)) continue;   // On the edge itself: no direction to push.
            for (int k = 0; k < 3; ++k) ss_g[k] = qd[k] / ss_h;
        }
        apply(q, one, 1, f, bw, 3, ss_g, ss_h, c.sep, c.dep);
    }

    for (size_t i = 0; i < ss_s.self_e.size(); ++i) {
        ClothSelfE& c = ss_s.self_e[i];
        const ClothSelfEdge& E0 = ss_s.self_edges[c.e0];
        const ClothSelfEdge& E1 = ss_s.self_edges[c.e1];
        ClothPart* const ss_x[2] = { &ss_s.part[E0.a], &ss_s.part[E0.b] };
        ClothPart* const ss_y[2] = { &ss_s.part[E1.a], &ss_s.part[E1.b] };
        float s, t, c0[3], c1[3], dv[3], ss_g[3], ss_h;
        cloth_closest_seg(ss_x[0]->p, ss_x[1]->p, ss_y[0]->p, ss_y[1]->p, s, t, c0, c1);
        cloth_v3_sub(c0, c1, dv);
        const float dl = cloth_v3_len(dv);
        const float d1 = cloth_v3_dot(dv, c.n);
        bool through = false;
        if (!(dl > 1.0e-7f && d1 > 0.0f)) {   // Turned against the start: through each other, or past?
            const float tau = c.d0 / fmaxf(c.d0 - d1, 1.0e-12f);
            float pt[4][3];
            ClothPart* const all[4] = { ss_x[0], ss_x[1], ss_y[0], ss_y[1] };
            for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) {
                pt[j][k] = all[j]->pp[k] + (all[j]->p[k] - all[j]->pp[k]) * tau;
            }
            float st, tt, a[3], b[3], ab[3];
            cloth_closest_seg(pt[0], pt[1], pt[2], pt[3], st, tt, a, b);
            cloth_v3_sub(a, b, ab);
            through = st > 0.0f && st < 1.0f && tt > 0.0f && tt < 1.0f &&
                      cloth_v3_len(ab) <= c.sep && -d1 <= CLOTH_SELF_DEEP * ss_s.self_edge;
            if (!through) {   // Past each other: the direction is where they are now.
                if (!(dl > 1.0e-7f)) continue;
                for (int k = 0; k < 3; ++k) c.n[k] = dv[k] / dl;
            }
        }
        if (through) {   // Back the way they came.
            for (int k = 0; k < 3; ++k) ss_g[k] = c.n[k];
            ss_h = d1;
        } else {
            for (int k = 0; k < 3; ++k) ss_g[k] = dv[k] / dl;
            ss_h = dl;
        }
        const float ss_wx[2] = { 1.0f - s, s }, ss_wy[2] = { 1.0f - t, t };
        apply(ss_x, ss_wx, 2, ss_y, ss_wy, 2, ss_g, ss_h, c.sep, c.dep);
    }
}
static constexpr float    AFF_CORE_M = 0.10f;   // A radial force fades to zero inside this (m).
static constexpr float    AFF_DRAG_MAX = 1000.0f;   // CLOTH_DRAG: a drag affector's most thickening.

enum AffType : uint8_t {
    AFF_NONE = 0, AFF_WIND, AFF_TURBULENCE, AFF_VORTEX, AFF_FORCE, AFF_RADIAL, AFF_DRAG,
};

// The affectors at one particle position: air gains their air velocity
// (m/s), acc their acceleration (m/s^2), and drag their thickening
// of the air (CLOTH_DRAG: the air's density is 1 + drag times its
// own). tback is how long before the frame's end this substep ends (s),
// turbulence's clock.
inline void cloth_aff_eval(const ClothAffView* ae_v, size_t n, const float* p,
                           float tback, float* air, float* acc, float& drag) {
    for (size_t i = 0; i < n; ++i) {
        const ClothAffView& v = ae_v[i];
        if (!(v.gain > 0.0f)) continue;
        float d[3];
        cloth_v3_sub(p, v.c, d);
        const float l[3] = { cloth_v3_dot(d, v.ax), cloth_v3_dot(d, v.ax + 3), cloth_v3_dot(d, v.ax + 6) };
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
        case AFF_WIND:
            for (int k = 0; k < 3; ++k) air[k] += f[k] * v.p[0] * g;
            break;
        case AFF_FORCE:
            for (int k = 0; k < 3; ++k) acc[k] += f[k] * v.p[0] * g;
            break;
        case AFF_RADIAL: {
            // Away from the centre for a positive strength. Inside AFF_CORE_M
            // it fades to zero: a particle on the centre is not flung about by a
            // direction that is undefined there.
            const float r = cloth_v3_len(d);
            if (r > 1.0e-6f) {
                const float s = v.p[0] * g * (r < AFF_CORE_M ? r / AFF_CORE_M : 1.0f) / r;
                for (int k = 0; k < 3; ++k) acc[k] += d[k] * s;
            }
            break;
        }
        case AFF_DRAG:   // CLOTH_DRAG: thicker air - densities add.
            drag += (v.p[0] < 0.0f ? 0.0f : (v.p[0] > AFF_DRAG_MAX ? AFF_DRAG_MAX : v.p[0])) * g;
            break;
        case AFF_VORTEX: {
            // A Rankine vortex about the forward axis: the swirl grows with the
            // distance from the axis out to the core radius and falls as
            // 1 / distance beyond it (still at the axis, peak at the core). The
            // inflow ramps in over the same core. rho x f is the right-hand
            // sense about vectorDir in WORLD axes: the engine axes are a
            // reflection of them, so the order flips.
            float rho[3];
            for (int k = 0; k < 3; ++k) rho[k] = d[k] - f[k] * l[1];
            const float rl = cloth_v3_len(rho);
            if (rl > 1.0e-6f) {
                const float core = v.p[3];
                const float vt = v.p[0] * (rl < core ? rl / core : core / rl);
                const float vr = -v.p[1] * (rl < core ? rl / core : 1.0f);
                const float t[3] = { rho[1] * f[2] - rho[2] * f[1],
                                     rho[2] * f[0] - rho[0] * f[2],
                                     rho[0] * f[1] - rho[1] * f[0] };
                for (int k = 0; k < 3; ++k) air[k] += (t[k] * vt + rho[k] * vr) / rl * g;
            }
            for (int k = 0; k < 3; ++k) air[k] += f[k] * v.p[2] * g;
            break;
        }
        case AFF_TURBULENCE:
            // A sum of travelling plane waves, each with its amplitude
            // perpendicular to its wave vector (divergence-free), so the sum is
            // an incompressible swirl. Measured from the affector's own centre:
            // one field in the world whichever cloth, and whichever floating
            // origin, reads it.
            for (uint32_t m = 0; m < AFF_MODES; ++m) {
                const float s = sinf(cloth_v3_dot(v.mk[m], d) + v.mph[m] - v.mw[m] * tback) * g;
                for (int k = 0; k < 3; ++k) air[k] += v.ma[m][k] * s;
            }
            break;
        default:
            break;
        }
    }
}

// One XPBD substep. h is the substep length; the caller owns the accumulator.
// The state's frame (centre / rot / size) is where the carrier stands at the
// END of this substep and f0 where it stood at the start; each collider
// view is posed the same way, its prev being the start. cloth_step
// interpolates both across the step's span (SUB_CLOCK).
inline void cloth_substep(ClothState& s, const ClothParams& pr,
                          const PhysicsColliderView* col, size_t ncol, float h,
                          const float* wind, const ClothFrame& f0,
                          const ClothAffView* aff = nullptr, size_t naff = 0,
                          float tback = 0.0f, const ClothGround* ground = nullptr) {
    const size_t n = s.part.size();
    if (n == 0 || h <= 0.0f) return;
    const float g = -CLOTH_G * pr.gravity;   // Down the engine up axis (index 1).
    // The per-step dials are stated per CLOTH_H, so a different substep
    // raises what they keep to the matching power: 'substeps' changes how finely
    // the solve runs, not how much motion a second of damping removes. At the
    // default substep the ratio is exactly 1 and this is the dial, bit for bit.
    const float rate = h / CLOTH_H;
    auto keep = [rate](float dial) {
        const float cs_k = 1.0f - (dial < 0.0f ? 0.0f : (dial > 1.0f ? 1.0f : dial));
        return rate == 1.0f ? cs_k : powf(cs_k, rate);
    };
    // Along the constraints, before the predict (MATERIAL DAMPING).
    const float damp = keep(pr.damping);
    const float vmax = pr.max_speed > 0.0f ? pr.max_speed : 1.0e6f;
    const float dmax = vmax * h;   // A substep may never move a particle further than this...
    // ...relative to the carrier. This substep's carrier runs from f0 to
    // the state's frame; cloth_frame_delta gives its motion at a point.
    ClothFrame f1;
    for (int k = 0; k < 3; ++k) { f1.centre[k] = s.centre[k]; f1.size[k] = s.size[k]; }
    memcpy(f1.rot, s.rot, sizeof(f1.rot));
    f1.rotated = s.rotated;
    const bool cmove = !cloth_frame_same(f0, f1);
    float cratio[3];
    for (int k = 0; k < 3; ++k) {
        const float c0 = fabsf(f0.size[k]) < 1.0e-6f ? 1.0e-6f : f0.size[k];
        cratio[k] = f1.size[k] / c0;
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
    const float wmul = pr.wind;
    const bool windy = !s.tri.empty();
    if (windy) {
        if (s.nrm.size() != n * 7u) s.nrm.assign(n * 7u, 0.0f);
        else std::fill(s.nrm.begin(), s.nrm.end(), 0.0f);
        for (size_t t = 0; t < s.tri.size(); ++t) {
            const ClothTri& cs_t = s.tri[t];
            if (cs_t.a >= n || cs_t.b >= n || cs_t.c >= n) continue;
            float e1[3], e2[3], nr[3];
            cloth_v3_sub(s.part[cs_t.b].p, s.part[cs_t.a].p, e1);
            cloth_v3_sub(s.part[cs_t.c].p, s.part[cs_t.a].p, e2);
            nr[0] = e1[1] * e2[2] - e1[2] * e2[1];
            nr[1] = e1[2] * e2[0] - e1[0] * e2[2];
            nr[2] = e1[0] * e2[1] - e1[1] * e2[0];
            // |n| is twice the area and n n^T / |n| is |n| nhat nhat^T.
            const float nl = cloth_v3_len(nr);
            if (nl < 1.0e-20f) continue;
            const float il = 1.0f / nl;
            const float q[7] = { nr[0] * nr[0] * il, nr[0] * nr[1] * il,
                                 nr[0] * nr[2] * il, nr[1] * nr[1] * il,
                                 nr[1] * nr[2] * il, nr[2] * nr[2] * il,
                                 nl };
            const uint32_t iv[3] = { cs_t.a, cs_t.b, cs_t.c };
            for (int cs_k = 0; cs_k < 3; ++cs_k) {
                float* acc = &s.nrm[static_cast<size_t>(iv[cs_k]) * 7u];
                for (int j = 0; j < 7; ++j) acc[j] += q[j];
            }
        }
    }

    const float mass = pr.mass > 1.0e-4f ? pr.mass : 1.0e-4f;
    const float buoy = pr.buoyancy < 0.0f ? 0.0f : pr.buoyancy;
    // CLOTH_DRAG: the media's drag per unit mass (1/s), against a face's motion.
    const float cair = (pr.drag > 0.0f ? pr.drag : 0.0f) / mass;
    const float cwat = (pr.water_drag > 0.0f ? pr.water_drag : 0.0f) / mass;
    // ...and its two factors in plain air (no water, no drag affector), the common case, taken once.
    const float es0 = expf(-CLOTH_SKIN * cair * h);
    const float ef0 = 1.0f - expf(-(1.0f - CLOTH_SKIN) * cair * h);

    // MATERIAL DAMPING. Each distance constraint loses 1 - damp of the rate
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
    if (damp < 1.0f) {
        const float take = 1.0f - damp;
        for (int pass = 0; pass < 2; ++pass) {
            const std::vector<ClothDist>& set = pass == 0 ? s.stretch : s.bend;
            for (size_t c = 0; c < set.size(); ++c) {
                ClothPart& A = s.part[set[c].a];
                ClothPart& B = s.part[set[c].b];
                const float wsum = A.w + B.w;
                if (wsum <= 0.0f) continue;
                float dv[3];
                cloth_v3_sub(B.p, A.p, dv);
                const float len = cloth_v3_len(dv);
                if (len < 1.0e-9f) continue;
                float rv = 0.0f;   // Separation over the last substep, along the constraint.
                for (int k = 0; k < 3; ++k) rv += ((B.p[k] - B.pp[k]) - (A.p[k] - A.pp[k])) * dv[k];
                const float sc = rv / len * take / (len * wsum);
                for (int k = 0; k < 3; ++k) {
                    A.pp[k] -= dv[k] * sc * A.w;
                    B.pp[k] += dv[k] * sc * B.w;
                }
            }
        }
    }

    // Predict. Velocity is implicit in (p - pp), which is what keeps a
    // position correction from being re-applied as momentum next substep.
    for (size_t i = 0; i < n; ++i) {
        ClothPart& p = s.part[i];
        // How submerged, ramped over the band (the up axis is metres ASL: no water query).
        float sub = (s.sea - p.p[CLOTH_UP]) * (1.0f / CLOTH_WATER_BAND);
        if (sub < 0.0f) sub = 0.0f; else if (sub > 1.0f) sub = 1.0f;
        // AFFECTOR: the scripted fields at this particle, where it stands
        // before the prediction (the point the wind below is taken at). A pinned
        // particle does not move, so it is not asked.
        float aair[3] = { 0.0f, 0.0f, 0.0f };
        float aacc[3] = { 0.0f, 0.0f, 0.0f };
        float adrag = 0.0f;
        const bool aff_on = naff != 0 && p.w > 0.0f;
        if (aff_on) cloth_aff_eval(aff, naff, p.p, tback, aair, aacc, adrag);
        // CLOTH_DRAG: the air above the water and the still water below, each in its share across the band,
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
        const float ra = (1.0f - sub) * (1.0f + adrag) * cair;   // The air's pull (1/s).
        const float rn = ra + sub * cwat;                         // ...and with the water's.
        float ue[3] = { 0.0f, 0.0f, 0.0f };   // The media's velocity over one substep (m), as they pull.
        if (ra > 0.0f) {
            const float sh = ra / rn * h;
            for (int k = 0; k < 3; ++k) {
                ue[k] = ((wind ? wind[k] * wmul : 0.0f) + aair[k]) * sh;
            }
        }
        const bool dry = sub <= 0.0f && adrag <= 0.0f;   // Then rn IS cair: the pair above.
        const float es = dry ? es0 : expf(-CLOTH_SKIN * rn * h);   // Along: kept.
        const float ef = dry ? ef0 : 1.0f - expf(-(1.0f - CLOTH_SKIN) * rn * h);   // Lost.
        float v[3];
        for (int k = 0; k < 3; ++k) v[k] = ue[k] + (p.p[k] - p.pp[k] - ue[k]) * es;
        const float* m = windy ? &s.nrm[i * 7u] : nullptr;
        if (m && m[6] > 1.0e-20f) {
            const float rel[3] = { ue[0] - v[0], ue[1] - v[1], ue[2] - v[2] };
            const float im = ef / m[6];
            v[0] += (m[0] * rel[0] + m[1] * rel[1] + m[2] * rel[2]) * im;
            v[1] += (m[1] * rel[0] + m[3] * rel[1] + m[4] * rel[2]) * im;
            v[2] += (m[2] * rel[0] + m[4] * rel[1] + m[5] * rel[2]) * im;
        } else {
            for (int k = 0; k < 3; ++k) v[k] += (ue[k] - v[k]) * ef;
        }
        // Gravity, less the buoyancy the submerged part of it displaces.
        v[CLOTH_UP] += (g + sub * buoy * CLOTH_G) * h * h;
        if (aff_on) for (int k = 0; k < 3; ++k) v[k] += aacc[k] * h * h;
        // The clamp guards against a particle flung loose, so it measures against
        // the carrier's own motion at the particle: a cloth riding a vehicle at
        // 100 m/s is not flung, and a clamp on WORLD speed would stream it out
        // behind its pins. Only a particle fast in the world is tested, so one
        // the carrier leaves standing is never pulled along by the clamp; on a
        // still carrier this is the plain world-speed clamp, bit for bit.
        const float vl = cloth_v3_len(v);
        if (vl > dmax) {
            float cd[3] = { 0.0f, 0.0f, 0.0f };
            if (cmove) cloth_frame_delta(p.p, f0, cratio, f1, cd);
            float rel[3];
            for (int k = 0; k < 3; ++k) rel[k] = v[k] - cd[k];
            const float rl = cmove ? cloth_v3_len(rel) : vl;
            if (rl > dmax) {
                const float sc = dmax / rl;
                for (int k = 0; k < 3; ++k) v[k] = cmove ? cd[k] + rel[k] * sc : v[k] * sc;
            }
        }
        for (int k = 0; k < 3; ++k) {
            p.pp[k] = p.p[k];
            p.p[k] += p.w > 0.0f ? v[k] : 0.0f;
        }
    }

    // Pinned particles ARE the frame: re-placed every substep so a moving object
    // drags its cloth instead of leaving it behind (the consume-point rule the
    // draw passes follow). Their pp is where the carrier held them at the
    // substep's start, so a pin's velocity is the carrier's there and the
    // material damping compares its neighbours against that motion, not against
    // standing still in the world. Under a skeletal binding the targets move too
    // (cloth_target): pp takes the skeleton's pose at the substep's start and
    // p its pose at the end.
    for (size_t i = 0; i < n; ++i) {
        ClothPart& p = s.part[i];
        if (p.w > 0.0f) continue;
        float g1[3], g0[3];
        cloth_l2w(cloth_target(s, i, s.guide_t1, g1), s.centre, s.rot, s.rotated,
                  s.size, p.p);
        cloth_l2w(cloth_target(s, i, s.guide_t0, g0), f0.centre, f0.rot,
                  f0.rotated, f0.size, p.pp);
    }

    // Self-collision's pairs, found once the prediction and the pins say where
    // everything is headed this substep, then solved inside the iterations.
    const bool self = pr.self_collide && !(s.self_hold > 0.0f) && !s.tri.empty();
    if (self) cloth_self_gather(s, pr.self_thickness);
    else { s.self_c.clear(); s.self_e.clear(); }

    // XPBD accumulates a multiplier per constraint per substep; it is what makes
    // compliance a MATERIAL. Without it alpha only damps each correction, every
    // extra iteration walks the constraint toward rigid, and the converged
    // answer is the inextensible one whatever the dial says. The multiplier
    // resets per SUBSTEP, not per iteration: each substep is its own implicit
    // solve.
    if (s.lam_stretch.size() != s.stretch.size()) s.lam_stretch.assign(s.stretch.size(), 0.0f);
    if (s.lam_bend.size() != s.bend.size()) s.lam_bend.assign(s.bend.size(), 0.0f);
    std::fill(s.lam_stretch.begin(), s.lam_stretch.end(), 0.0f);
    std::fill(s.lam_bend.begin(), s.lam_bend.end(), 0.0f);
    if (s.lam_att.size() != n) s.lam_att.assign(n, 0.0f);
    std::fill(s.lam_att.begin(), s.lam_att.end(), 0.0f);
    const float h2 = h * h;
    const float a_str = cloth_compliance(pr.stretch, 1.0e-2f) / h2;
    const float a_bnd = cloth_compliance(pr.bend,    1.0e+0f) / h2;
    const int   it = pr.iterations < 1 ? 1 : (pr.iterations > 64 ? 64 : pr.iterations);

    for (int cs_k = 0; cs_k < it; ++cs_k) {
        // Distance constraints, stretch then bend. Gauss-Seidel in place: each
        // solve sees the previous one's correction, which lets eight iterations
        // stand in for a global solve.
        for (int pass = 0; pass < 2; ++pass) {
            const std::vector<ClothDist>& set = pass == 0 ? s.stretch : s.bend;
            std::vector<float>& lamv = pass == 0 ? s.lam_stretch : s.lam_bend;
            const float alpha = pass == 0 ? a_str : a_bnd;
            for (size_t c = 0; c < set.size(); ++c) {
                const ClothDist& d = set[c];
                ClothPart& A = s.part[d.a];
                ClothPart& B = s.part[d.b];
                const float wsum = A.w + B.w;
                if (wsum <= 0.0f) continue;
                float dv[3];
                cloth_v3_sub(B.p, A.p, dv);
                const float len = cloth_v3_len(dv);
                if (len < 1.0e-9f) continue;
                const float cval = len - d.rest;
                // dLambda = (-C - alpha * lambda) / (sum w |grad C|^2 + alpha).
                // |grad C| is 1 for a distance constraint, so the sum is w_a + w_b.
                const float lam = (-cval - alpha * lamv[c]) / (wsum + alpha);
                lamv[c] += lam;
                const float sc = lam / len;
                for (int k = 0; k < 3; ++k) {
                    A.p[k] -= dv[k] * sc * A.w;
                    B.p[k] += dv[k] * sc * B.w;
                }
            }
        }

        // The pin/sim gradient, solved as an XPBD constraint holding the particle
        // at the position the rigid mesh - or the skeleton (cloth_target) -
        // would give it. A CONSTRAINT, not a lerp toward the target: a lerp has
        // no force balance, so it converges on the target itself, and applied
        // once per iteration per substep even a very weak one erases the whole
        // deviation. A compliance has an equilibrium: the particle settles where
        // the attachment balances gravity.
        for (size_t i = 0; i < n; ++i) {
            ClothPart& p = s.part[i];
            if (p.att <= 0.0f || p.w <= 0.0f) continue;
            float cs_t[3], g[3];
            cloth_l2w(cloth_target(s, i, s.guide_t1, g), s.centre, s.rot,
                      s.rotated, s.size, cs_t);
            float av[3];
            cloth_v3_sub(p.p, cs_t, av);
            const float ac = cloth_v3_len(av);
            if (ac < 1.0e-9f) continue;
            const float aa = p.att / h2;
            const float al = (-ac - aa * s.lam_att[i]) / (p.w + aa);
            s.lam_att[i] += al;
            const float as = al * p.w / ac;
            for (int k = 0; k < 3; ++k) p.p[k] += av[k] * as;
        }

        // Self-contact, in the same Gauss-Seidel sweep, so the constraints
        // and the contacts settle against each other rather than in turn.
        if (self) cloth_self_solve(s, -1.0f);
    }

    // Self-contact's last sweep, the one that carries its friction, AHEAD of the
    // two limits, which have the last word over the cloth's own layers: under a
    // yank the tethers drag free cloth straight toward its pins, through the
    // cape if that is where it hangs, and a sweep after them would push that
    // cloth back every substep while they drag it through again. The colliders
    // come later still: a solid outranks both.
    if (self) {
        const float smu = pr.self_friction < 0.0f ? 0.0f
                        : (pr.self_friction > 1.0f ? 1.0f : pr.self_friction);
        cloth_self_solve(s, smu);
    }

    // Per-edge limit, first of the two. The tether bounds distance from the
    // anchor but says nothing about a pair of particles that both drifted the
    // same way, and nothing at all for a cloth with no pins; this catches both.
    // It BOUNDS the worst edge, it does not cap it: a fixed number of
    // Gauss-Seidel passes cannot satisfy every edge of a chain at once, and
    // under a violent yank the worst ratio settles above max_stretch for a few
    // frames before the solver walks it back. The excursion is bounded and
    // transient, which is what stops permanent spaghetti.
    if (pr.max_stretch >= 1.0f) {
        for (int lp = 0; lp < CLOTH_LIMIT_PASSES; ++lp) {
        for (size_t c = 0; c < s.stretch.size(); ++c) {
            const ClothDist& d = s.stretch[c];
            ClothPart& A = s.part[d.a];
            ClothPart& B = s.part[d.b];
            const float wsum = A.w + B.w;
            if (wsum <= 0.0f) continue;
            float dv[3];
            cloth_v3_sub(B.p, A.p, dv);
            const float len = cloth_v3_len(dv);
            const float cap = d.rest * pr.max_stretch;
            if (len <= cap || len < 1.0e-9f) continue;
            const float sc = (len - cap) / len;
            for (int k = 0; k < 3; ++k) {
                A.p[k] += dv[k] * sc * (A.w / wsum);
                B.p[k] -= dv[k] * sc * (B.w / wsum);
            }
        }
        }
    }

    // Long-range attachment, the global bound and the one that IS exact. It runs
    // AFTER the per-edge sweep: whichever of the two goes last is the one that
    // holds, and this is the one worth holding. Only the collider pass may move
    // a particle after this, by its contact depth and the friction that depth
    // allows.
    if (pr.max_stretch >= 1.0f) {
        for (size_t i = 0; i < n; ++i) {
            ClothPart& p = s.part[i];
            if (p.w <= 0.0f || p.tether <= 0.0f) continue;
            const ClothPart& an = s.part[p.anchor];
            float dv[3];
            cloth_v3_sub(p.p, an.p, dv);
            const float len = cloth_v3_len(dv);
            const float cap = p.tether * pr.max_stretch;
            if (len <= cap || len < 1.0e-9f) continue;
            const float sc = cap / len;
            for (int k = 0; k < 3; ++k) p.p[k] = an.p[k] + dv[k] * sc;
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
    const float fr = pr.friction < 0.0f ? 0.0f : (pr.friction > 1.0f ? 1.0f : pr.friction);
    // CLOTH_WARM: the per-collider columns of last-hit triangles, kept across
    // substeps and frames while the collider at that index is the same object
    // (slot and mesh); a column for a collider that changed starts over, and a
    // new column count re-lays them all out.
    if (ncol != 0u) {
        const size_t wn = n * ncol;
        if (s.col_warm.size() != wn || s.col_warm_key.size() != ncol * 2u) {
            s.col_warm.assign(wn, CLOTH_WARM_NONE);
            s.col_warm_key.assign(ncol * 2u, CLOTH_WARM_NONE);
        }
        if (s.col_clear.size() != wn || s.col_clear_col.size() != ncol) {   // COL_CLEAR: laid out with the warm columns.
            s.col_clear.assign(wn, ColClear{});
            s.col_clear_col.assign(ncol, ColClearCol{});
        }
        for (size_t c = 0; c < ncol; ++c) {
            const uint32_t km = static_cast<uint32_t>(col[c].mesh);
            if (s.col_warm_key[c * 2u] != col[c].slot || s.col_warm_key[c * 2u + 1u] != km) {
                s.col_warm_key[c * 2u] = col[c].slot;
                s.col_warm_key[c * 2u + 1u] = km;
                std::fill(s.col_warm.begin() + static_cast<ptrdiff_t>(c * n),
                          s.col_warm.begin() + static_cast<ptrdiff_t>((c + 1u) * n), CLOTH_WARM_NONE);
                s.col_clear_col[c] = ColClearCol{};   // COL_CLEAR: another collider's column; emptied just below.
            }
            // COL_CLEAR: the column's chain to this substep's pose; records of a broken chain are void.
            const bool was = s.col_clear_col[c].on;
            if (!col_clear_advance(s.col_clear_col[c], col[c]) && (was || s.col_clear_col[c].on)) {
                std::fill(s.col_clear.begin() + static_cast<ptrdiff_t>(c * n),
                          s.col_clear.begin() + static_cast<ptrdiff_t>((c + 1u) * n), ColClear{});
            }
        }
    }
    for (size_t i = 0; i < n; ++i) {
        ClothPart& p = s.part[i];
        if (p.w <= 0.0f) continue;
        for (size_t c = 0; c < ncol; ++c) {
            float n_hit[3], sv[3];
            const float p0[3] = { p.p[0], p.p[1], p.p[2] };
            if (!physics_collide_one(col[c], pr.thickness, p.pp, p.p, n_hit, sv,
                                     &s.col_warm[c * n + i],
                                     s.col_clear_col[c].on ? &s.col_clear[c * n + i] : nullptr,   // COL_CLEAR.
                                     s.col_clear_col[c].acc)) continue;
            float f = fr * col[c].friction;
            if (f <= 0.0f) continue;
            if (f > 1.0f) f = 1.0f;
            float push[3];
            cloth_v3_sub(p.p, p0, push);
            const float depth = cloth_v3_dot(push, n_hit);
            if (!(depth > 0.0f)) continue;
            float mv[3];
            cloth_v3_sub(p.p, p.pp, mv);
            for (int k = 0; k < 3; ++k) mv[k] -= sv[k];
            const float nd = cloth_v3_dot(mv, n_hit);
            float tan[3];
            for (int k = 0; k < 3; ++k) tan[k] = mv[k] - n_hit[k] * nd;
            const float tl = cloth_v3_len(tan);
            if (tl < 1.0e-12f) continue;
            const float cone = f * depth;
            const float stop = tl <= cone ? 1.0f : cone / tl;   // Share of it removed.
            for (int k = 0; k < 3; ++k) p.p[k] -= tan[k] * stop;
        }
        // CLOTH_GROUND: the terrain, after every collider, so what lies between
        // a collider and the ground ends the substep on the ground's side (a cape
        // under a prone body shows against the body's collider, never under the
        // visible terrain). The same Coulomb form as a collider; the ground is
        // still and its half of the coefficient is 1 (a collider's default), so
        // the contact's is the cloth's 'friction'.
        if (ground) {
            float gn[3];
            const float g0[3] = { p.p[0], p.p[1], p.p[2] };
            if (cloth_ground_contact(*ground, pr.thickness, p.p, gn)) {
                float push[3];
                cloth_v3_sub(p.p, g0, push);
                const float depth = cloth_v3_dot(push, gn);
                float mv[3];
                cloth_v3_sub(p.p, p.pp, mv);
                const float nd = cloth_v3_dot(mv, gn);
                float tan[3];
                for (int k = 0; k < 3; ++k) tan[k] = mv[k] - gn[k] * nd;
                const float tl = cloth_v3_len(tan);
                if (fr > 0.0f && depth > 0.0f && tl >= 1.0e-12f) {
                    const float cone = fr * depth;
                    const float stop = tl <= cone ? 1.0f : cone / tl;
                    for (int k = 0; k < 3; ++k) p.p[k] -= tan[k] * stop;
                }
            }
        }
    }
    // Non-finite rescue. One bad particle poisons every constraint it touches
    // within an iteration, so the test is on the whole set and the answer is
    // the rest pose - a visible snap beats a permanent NaN cloud.
    for (size_t i = 0; i < n; ++i) {
        if (!cloth_finite3(s.part[i].p) || !cloth_finite3(s.part[i].pp)) {
            cloth_reset(s);
            return;
        }
    }
}

// One frame. Owns every decision that depends on how the object MOVED since
// last frame: teleports, stalls, resizes and the first frame of all.
inline void cloth_step(ClothState& s, const ClothParams& pr,
                       const PhysicsColliderView* col, size_t ncol,
                       const float* centre, const float* rot, bool rotated,
                       const float* size, float dt, const float* wind,
                       const ClothAffView* aff = nullptr, size_t naff = 0,
                       const ClothGround* ground = nullptr) {
    if (s.part.empty()) return;

    // A frame that arrives after a load, an alt-tab or a hitch is CLAMPED and
    // simulated like any other (a rest-pose reset would snap every drape back on
    // each hitch, and below 10 fps every frame is a 'stall'). What jumped while
    // the game hung (the object moved or turned past the teleport test) is
    // caught below whatever the frame's length; a dt that is not a positive
    // number advances nothing.
    if (!(dt > 0.0f)) dt = 0.0f;
    // DT_RAW: the frame's own length, for the velocities taken below (the carrier's and the jolt test's corners).
    // The clamp is the simulation's budget; do not take velocities over the clamp: a hitch then reads as a burst of
    // speed (a 0.5 s hitch at 150 m/s reads 750 m/s, the next frame's prediction misses by 10 m: a spurious rebase,
    // nothing swept - TELEPORT_ONCE covers it only while a frame's own travel is under the teleport distance -
    // and a jolt where none was).
    const float dt_raw = dt;
    if (dt > CLOTH_DT_MAX) dt = CLOTH_DT_MAX;
    // SKEL: every placement outside the substeps is at the frame's end.
    s.guide_t0 = 1.0f;
    s.guide_t1 = 1.0f;

    // SUB_CLOCK: the frame's motion is measured from the last frame's END
    // (fend); the substeps run from where the simulation stands - the state's
    // frame, at most a substep short of it - and the particles stand against
    // that frame, so it is also what a rebase carries them from.
    ClothFrame old = s.fend;
    ClothFrame stood;
    for (int k = 0; k < 3; ++k) { stood.centre[k] = s.centre[k]; stood.size[k] = s.size[k]; }
    memcpy(stood.rot, s.rot, sizeof(stood.rot));
    stood.rotated = s.rotated;
    ClothFrame cf_new;
    for (int k = 0; k < 3; ++k) { cf_new.centre[k] = centre[k]; cf_new.size[k] = size[k]; }
    memcpy(cf_new.rot, rot, sizeof(cf_new.rot));
    cf_new.rotated = rotated;

    float is_old[3];
    for (int k = 0; k < 3; ++k) {
        const float v = fabsf(s.size[k]) < 1.0e-6f ? 1.0e-6f : s.size[k];
        is_old[k] = 1.0f / v;
    }

    bool reset = !s.primed;
    bool rebase = false;
    if (!reset) {
        // A centre that landed further than the threshold from where its own
        // velocity would have taken it, or a basis that turned over, is a
        // teleport. Measured against the prediction, not last frame's centre, so
        // a steady speed (2 m a frame is 60 m/s at 30 fps) never trips it.
        ClothFrame pred = old;
        if (s.carrier_vok) for (int k = 0; k < 3; ++k) pred.centre[k] += s.carrier_v[k] * dt;
        rebase = cloth_frame_jumped(pred, cf_new, pr.teleport);
        // TELEPORT_ONCE: after a rebase the prediction carries the jump on; a carrier that moved less than the
        // teleport distance from where it stood did not jump again.
        if (rebase && s.carrier_vok && s.carrier_vreb &&
            !cloth_frame_jumped(old, cf_new, pr.teleport)) rebase = false;
        if (dt > 0.0f) {   // DT_RAW: the travel over the frame it took.
            for (int k = 0; k < 3; ++k) s.carrier_v[k] = (cf_new.centre[k] - old.centre[k]) / dt_raw;
            s.carrier_vok = true;
            s.carrier_vreb = rebase;   // TELEPORT_ONCE.
        }
        // A resized mesh has a different rest pose in world units; rebasing
        // through the old inverse would scale the cloth by the ratio.
        for (int k = 0; k < 3; ++k) {
            const float r = size[k] * is_old[k];
            if (r < 0.5f || r > 2.0f) reset = true;
        }
    }

    if (rebase && !reset) {
        cloth_rebase(s, s.centre, s.rot, s.rotated, is_old,
                     centre, rot, rotated, size);
    }

    if (reset) {
        // SUB_CLOCK: placed at the frame's end, with nothing left over.
        for (int k = 0; k < 3; ++k) { s.centre[k] = centre[k]; s.size[k] = size[k]; }
        memcpy(s.rot, rot, sizeof(s.rot));
        s.rotated = rotated;
        s.fend = cf_new;
        s.sub_t = 1.0f;
        cloth_reset(s);
        s.speed_mean = 0.0f;
        s.reach = 0.0f;
        return;
    }
    // A teleported cloth already stands in the new frame: nothing is swept.
    if (rebase) { old = cf_new; stood = cf_new; }

    // SELF-COLLISION STANDS DOWN AFTER A JOLT. A carrier that moves under the teleport test but faster than any cloth
    // could follow drags the free cloth after its pins through itself; with self-collision on it is driven through in
    // places too deep to push back and then held there; with it off, the cloth passes through and falls open again. So
    // a jolt (a change of velocity at any corner of the carrier's box past CLOTH_SELF_JOLT in one frame) turns
    // self-collision off for CLOTH_SELF_HOLD seconds (a further jolt restarts the hold), long enough for the cloth
    // to fall open; then it is back on. Falling or flapping cloth is untouched: the test is the carrier's motion, not
    // the cloth's.
    if (pr.self_collide) {
        float jolt = 0.0f;
        const bool vnow = dt > 0.0f && !rebase;
        for (int c = 0; c < 8; ++c) {
            const float cl[3] = { (c & 1) ? 0.5f : -0.5f, (c & 2) ? 0.5f : -0.5f, (c & 4) ? 0.5f : -0.5f };
            float c0[3], c1[3], v[3];
            cloth_l2w(cl, old.centre, old.rot, old.rotated, old.size, c0);
            cloth_l2w(cl, cf_new.centre, cf_new.rot, cf_new.rotated, cf_new.size, c1);
            for (int k = 0; k < 3; ++k) {   // DT_RAW.
                v[k] = vnow ? (c1[k] - c0[k]) / dt_raw : 0.0f;
            }
            if (vnow && s.self_vok) {
                float dv[3];
                cloth_v3_sub(v, s.self_v[c], dv);
                jolt = fmaxf(jolt, cloth_v3_len(dv));
            }
            if (vnow) for (int k = 0; k < 3; ++k) s.self_v[c][k] = v[k];
        }
        if (vnow) s.self_vok = true;
        else if (rebase) s.self_vok = false;   // A teleport is not a velocity.
        if (jolt > CLOTH_SELF_JOLT) s.self_hold = CLOTH_SELF_HOLD;
        else if (s.self_hold > 0.0f) s.self_hold -= dt;
    } else {
        s.self_hold = 0.0f;
        s.self_vok = false;
    }

    // Fixed substep with a bounded backlog. The accumulator is CAPPED, not
    // carried: a machine that cannot keep up must run the cloth slow, not spiral
    // into ever more substeps per frame. The cap is a TIME (the stall clamp's),
    // so the frame rate at which the cloth starts to run slow does not move with
    // the dial below.
    const int want = pr.substeps < 1 ? 1
                   : (pr.substeps > CLOTH_MAX_SUBSTEP ? static_cast<int>(CLOTH_MAX_SUBSTEP)
                                                                   : static_cast<int>(pr.substeps));
    // THE SUBSTEP IS FIXED. XPBD divides compliance by h squared and this
    // integrator carries velocity as the displacement (p - pp) over ONE substep,
    // so a substep that moved with the frame rate would read the same
    // displacement back as a different velocity every frame: frame-time
    // variance as a per-frame kick, a visible shimmer at rest. 'substeps'
    // therefore counts substeps per 1/60 s, not per frame, so h stays a
    // constant: the default 2 is exactly CLOTH_H. A slow frame runs more of
    // them, a fast one fewer, and the leftover carries in the accumulator.
    const float h = 1.0f / (60.0f * static_cast<float>(want));
    // SIM_LOD: the substep length changed (the LOD share moved, or the dial
    // did). The displacement p - pp IS the velocity over one substep, so read
    // over the new length it would be a different velocity: carry the velocity,
    // not the displacement - scale it by h_new / h_old. The rebase above moved p
    // and pp together, so the displacement is already in this frame. At an
    // unchanged length this is bit-identical (no write).
    if (s.h_last > 0.0f && s.h_last != h) {
        const float hr = h / s.h_last;
        for (size_t i = 0; i < s.part.size(); ++i) {
            ClothPart& p = s.part[i];
            for (int k = 0; k < 3; ++k) p.pp[k] = p.p[k] - (p.p[k] - p.pp[k]) * hr;
        }
    }
    s.h_last = h;
    s.accum += dt;
    if (s.accum > CLOTH_DT_MAX) s.accum = CLOTH_DT_MAX;
    const uint32_t nmax = static_cast<uint32_t>(CLOTH_DT_MAX / h) + 1u;   // A guard; the cap binds first.
    uint32_t n = 0;
    float left = s.accum;
    for (; left >= h && n < nmax; left -= h) ++n;
    // SUB_CLOCK: the step's span runs from where the simulation stands to
    // the frame's end - the whole accumulator - and the substeps cover this
    // share of it; what they leave (left, under one substep) is carried.
    // Exactly 1 whenever nothing is left (the float walk may leave a few ulp).
    const float cov = n == 0u ? 0.0f : 1.0f - left / s.accum;

    // THE CARRIER AND THE COLLIDERS ARE INTERPOLATED ACROSS THE SUBSTEPS.
    // Substep i sees the pose (i + 1) / n of the way through what the substeps
    // cover of the span, from where the simulation stands to the frame's end,
    // for the carrier and for each collider from its own pose there
    // (cloth_sync). Handing every substep the frame's END pose would move the
    // pins, the attachment targets and every collider the whole frame's distance
    // in the first substep and not at all in the rest (a jerk once a frame, a
    // shape that depends on the frame rate).
    // SUB_CLOCK: (i + 1) / n of the covered share, not of the FRAME, so each
    // substep moves the carrier one substep's worth of travel; on the frame
    // clock a frame that ran one substep would move it a whole frame's travel in
    // h, and the leftover would never place it at the time actually simulated
    // (the pins' velocity off the carrier's).
    if (s.col_sub.size() < ncol) s.col_sub.resize(ncol);
    PhysicsColliderView* const cv = s.col_sub.data();
    ClothFrame f0 = stood;
    uint32_t ran = 0;
    while (ran < n) {
        const float t0 = static_cast<float>(ran) / static_cast<float>(n) * cov;
        const float t1 = static_cast<float>(ran + 1u) / static_cast<float>(n) * cov;
        ClothFrame f1;
        cloth_frame_lerp(stood, cf_new, t1, f1);
        for (int k = 0; k < 3; ++k) { s.centre[k] = f1.centre[k]; s.size[k] = f1.size[k]; }
        memcpy(s.rot, f1.rot, sizeof(s.rot));
        s.rotated = f1.rotated;
        for (size_t c = 0; c < ncol; ++c) {
            const PhysicsColliderView& src = col[c];
            ClothFrame cur;
            for (int k = 0; k < 3; ++k) { cur.centre[k] = src.centre[k]; cur.size[k] = src.size[k]; }
            memcpy(cur.rot, src.rot, sizeof(cur.rot));
            cur.rotated = src.rotated;
            // A prev that was never filled is all zeros: a still collider.
            const bool pv = src.prev.size[0] != 0.0f || src.prev.size[1] != 0.0f ||
                            src.prev.size[2] != 0.0f;
            const ClothFrame& p0 = pv ? src.prev : cur;
            ClothFrame a, b;
            cloth_frame_lerp(p0, cur, t0, a);
            cloth_frame_lerp(p0, cur, t1, b);
            PhysicsColliderView& v = cv[c];
            v = src;
            float bs = 0.0f;
            for (int k = 0; k < 3; ++k) {
                v.centre[k] = b.centre[k];
                v.bs_c[k] = b.centre[k];
                const float e = fabsf(b.size[k]) < 1.0e-6f ? 1.0e-6f : b.size[k];
                v.size[k] = e;
                v.inv_size[k] = 1.0f / e;
                bs += 0.25f * e * e;
            }
            memcpy(v.rot, b.rot, sizeof(v.rot));
            v.rotated = b.rotated;
            v.bs_r = sqrtf(bs);
            v.prev = a;
            // SKEL: a deforming collider's SHAPE is interpolated like its
            // frame - the corners stand (1 - t) of the frame's motion back
            // from its pose - so a fast limb crosses the cloth in substep
            // steps, not in one.
            v.def0 = src.deform ? t0 - 1.0f : 0.0f;
            v.def1 = src.deform ? t1 - 1.0f : 0.0f;
            // Its bound is the gather's, which covers both poses about the
            // same centre; the box's half-diagonal covers only this one.
            if (src.deform && src.bs_r > v.bs_r) v.bs_r = src.bs_r;
        }
        // A skeleton's targets are interpolated like the carrier.
        s.guide_t0 = t0;
        s.guide_t1 = t1;
        // The affectors hold still across the frame; only turbulence's clock
        // runs, and it runs by the substep.
        // EA_TURB_CLOCK: this substep's end before the frame's end, leftover included (SUB_CLOCK).
        cloth_substep(s, pr, cv, ncol, h, wind, f0, aff, naff,
                      static_cast<float>(n - 1u - ran) * h + left, ground);
        s.accum -= h;
        f0 = f1;
        ++ran;
    }
    // A substep's non-finite rescue (cloth_reset) zeroes the budget inside this loop and the substeps after it
    // take it below zero, which would idle the cloth until frames repaid it. Otherwise this is a no-op.
    if (s.accum < 0.0f) s.accum = 0.0f;
    // The same rescue zeroes h_last (no displacement stood then), but every substep after it ran at h: the length
    // the displacement now stands for. After a rescue in the last substep it is zero, which any length reads alike.
    if (ran > 0u) s.h_last = h;
    // SUB_CLOCK: the state's frame is where the substeps ended (the frame's
    // end only when nothing is left over) and the frame's end stands beside it.
    // The writeback is relative to the state's frame, so the draw's own
    // transform carries the whole cloth, pins and free particles alike, rigidly
    // across the remainder; relative to the frame's end, a frame that ran no
    // substep would draw the free cloth a frame of travel behind the object.
    for (int k = 0; k < 3; ++k) { s.centre[k] = f0.centre[k]; s.size[k] = f0.size[k]; }
    memcpy(s.rot, f0.rot, sizeof(s.rot));
    s.rotated = f0.rotated;
    s.fend = cf_new;
    s.sub_t = cov;
    s.guide_t0 = 1.0f;
    s.guide_t1 = 1.0f;

    // A frame shorter than a substep advances nothing, and nothing moves: the
    // particles, the pins among them, stand against the state's frame, which
    // stood still, and the draw carries them all with the object.
    if (ran == 0) return;

    // The sleep test: the mean speed of the SIMULATED particles relative to the
    // carrier over the last substep. A pinned particle never moves in the
    // solver's terms, and averaging it in would scale the threshold by the
    // painted fraction; measured absolutely, a cloth on anything in motion
    // could never settle.
    ClothFrame fl0;   // The last substep's start; the state's frame (f0) is its end.
    cloth_frame_lerp(stood, cf_new,
                     static_cast<float>(ran - 1u) / static_cast<float>(ran) * cov, fl0);
    float ratio[3];
    for (int k = 0; k < 3; ++k) {
        const float s0 = fabsf(fl0.size[k]) < 1.0e-6f ? 1.0e-6f : fl0.size[k];
        ratio[k] = f0.size[k] / s0;
    }
    const bool moving = !cloth_frame_same(fl0, f0);
    double sum = 0.0;
    uint32_t sim = 0;
    float reach2 = 0.0f;
    const float inv = 1.0f / h;
    for (size_t i = 0; i < s.part.size(); ++i) {
        const ClothPart& p = s.part[i];
        float r[3];
        cloth_v3_sub(p.p, s.centre, r);
        const float r2 = cloth_v3_dot(r, r);
        if (r2 > reach2) reach2 = r2;
        if (p.w <= 0.0f) continue;
        float df[3] = { 0.0f, 0.0f, 0.0f };
        if (moving) cloth_frame_delta(p.pp, fl0, ratio, f0, df);
        float v[3];
        for (int k = 0; k < 3; ++k) v[k] = (p.p[k] - p.pp[k]) - df[k];
        sum += cloth_v3_len(v) * inv;
        ++sim;
    }
    s.speed_mean = sim ? static_cast<float>(sum / static_cast<double>(sim)) : 0.0f;
    s.reach = sqrtf(reach2);

    // The park ('sleep'). A cloth whose mean particle speed is under the
    // threshold has finished moving as far as anyone can see, and carrying its
    // residual velocity forward turns a converged drape into a shimmer. Killing
    // the velocity (not the position) leaves gravity and every constraint
    // acting, so it wakes the instant anything pushes it. It is the velocity
    // RELATIVE TO THE CARRIER that is killed: what is left is the carrier's own.
    if (pr.sleep > 0.0f && s.speed_mean < pr.sleep) {
        for (size_t i = 0; i < s.part.size(); ++i) {
            ClothPart& p = s.part[i];
            if (p.w <= 0.0f) continue;
            float df[3] = { 0.0f, 0.0f, 0.0f };
            if (moving) cloth_frame_delta(p.pp, fl0, ratio, f0, df);
            for (int k = 0; k < 3; ++k) p.pp[k] = p.p[k] - df[k];
        }
        s.speed_mean = 0.0f;
    }
}
}