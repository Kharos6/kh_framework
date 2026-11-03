#pragma once

// The feature half (rendering_integration_features.hpp): the LOD builder (lod_qem, lod_build) and the LOD picker.
// Never include it on its own.
namespace RenderIntegration {
// LOD_SIM_SLOT: whether an object draws level 0 alone - its lodLock, or a cloth's or chain's buffer in its slot
// (mesh_vb_ref's own substitution test). clothSimulation and chainSimulation force the lock, but a script can clear
// it in the frame its simulation ends, and the slot keeps the simulation's buffer until the next flush's uploads hand
// it back: a coarser level drawn from it would show its own vertices at rest. Every per-object pick (injection, flush,
// PIP) and the caster records (sun, DLS, seam, dlsw mask) ask this; the instanced bucket admits no object whose slot
// holds a substitute, but for a skin pool chunk its bucket pulls from (inst_eligible).
inline bool lod_locked(const RenderObject& o) {
    if (o.lod_lock) return true;
    const uint32_t s = o.slot;
    return s < g_cloth_sim_slot.size() && g_cloth_sim_slot[s] != 0u && s < g_cloth_vb_slot.size() &&
           g_cloth_vb_slot[s] != nullptr && s < g_cloth_mesh_slot.size() &&
           g_cloth_mesh_slot[s] == mesh_id_clamp(o.mesh);
}

// Automatic LOD for imported models. Decimation runs over the whole mesh at once (LOD_JOINT), each triangle
// keeping its submesh, so a level's submesh table has level 0's cardinality and order.
static constexpr size_t LOD_MIN_TRIS   = 32;
static constexpr size_t LOD_FLOOR_TRIS = 12;
// LOD_SCALE: the edge penalties in the face planes' units (area x distance squared) - a weight x the edge's
// length squared. Do not make them absolute (50 / 1000): they out-weigh a unit-box face ~10^4 times and pin every
// vertex on a seam (a UV-unwrapped cylinder with a seam on a third of its edges never got a level).
static constexpr double LOD_SEAM_W = 10.0;    // A uv seam.
static constexpr double LOD_OPEN_W = 100.0;   // An open edge.
static constexpr double LOD_REFUSED = 1.0e300;   // LOD_SEAM: a collapse no rule allows.

namespace lod_qem {

// Symmetric 4x4 quadric, upper triangle: a2 ab ac ad b2 bc bd c2 cd d2.
struct Q {
    double m[10];
};

inline void q_zero(Q& q) {
    for (int i = 0; i < 10; ++i) q.m[i] = 0.0;
}

inline void q_plane(Q& q, double a, double b, double c, double d,
                    double w) {
    q.m[0] = w * a * a; q.m[1] = w * a * b;
    q.m[2] = w * a * c; q.m[3] = w * a * d;
    q.m[4] = w * b * b; q.m[5] = w * b * c;
    q.m[6] = w * b * d; q.m[7] = w * c * c;
    q.m[8] = w * c * d; q.m[9] = w * d * d;
}

inline void q_add(Q& r, const Q& s) {
    for (int i = 0; i < 10; ++i) r.m[i] += s.m[i];
}

inline double q_eval(const Q& q, double x, double y, double z) {
    return q.m[0] * x * x + 2.0 * q.m[1] * x * y +
           2.0 * q.m[2] * x * z + 2.0 * q.m[3] * x +
           q.m[4] * y * y + 2.0 * q.m[5] * y * z +
           2.0 * q.m[6] * y + q.m[7] * z * z +
           2.0 * q.m[8] * z + q.m[9];
}

// Minimiser of the quadric: the 3x3 gradient system. False on a singular or ill-conditioned system; the caller then
// takes the best of the two endpoints and the midpoint.
inline bool q_min(const Q& q, double o[3]) {
    const double a[3][3] = {
        { q.m[0], q.m[1], q.m[2] },
        { q.m[1], q.m[4], q.m[5] },
        { q.m[2], q.m[5], q.m[7] },
    };
    const double det =
        a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
        a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
        a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    // The singularity test is relative to the quadric's own scale: an absolute epsilon passes near-singular systems
    // on unit-size low-poly meshes.
    const double s = (a[0][0] + a[1][1] + a[2][2]) / 3.0;
    if (!(s > 0.0) || !(fabs(det) > 1.0e-6 * s * s * s)) return false;
    const double r[3] = { -q.m[3], -q.m[6], -q.m[8] };
    const double inv = 1.0 / det;
    o[0] = inv * (r[0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
                  a[0][1] * (r[1] * a[2][2] - a[1][2] * r[2]) +
                  a[0][2] * (r[1] * a[2][1] - a[1][1] * r[2]));
    o[1] = inv * (a[0][0] * (r[1] * a[2][2] - a[1][2] * r[2]) -
                  r[0] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
                  a[0][2] * (a[1][0] * r[2] - r[1] * a[2][0]));
    o[2] = inv * (a[0][0] * (a[1][1] * r[2] - r[1] * a[2][1]) -
                  a[0][1] * (a[1][0] * r[2] - r[1] * a[2][0]) +
                  r[0] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]));
    return true;
}

struct Tri { uint32_t v[3]; bool live; };
struct Cand { double cost; uint32_t a, b; uint64_t sa, sb; };

// Heap via push_heap/pop_heap with an inverted comparator so the cheapest collapse comes off the top (no
// std::priority_queue: this header declares no includes).
struct CandWorse {
    bool operator()(const Cand& x, const Cand& y) const { return x.cost > y.cost; }
};

inline void tri_normal(const double p0[3], const double p1[3], const double p2[3],
                       double o[3]) {
    const double u[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
    const double v[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
    o[0] = u[1] * v[2] - u[2] * v[1];
    o[1] = u[2] * v[0] - u[0] * v[2];
    o[2] = u[0] * v[1] - u[1] * v[0];
}

// LOD_METRIC: the frame the decimator judges normals in. A stored normal is the authored one times the mesh's
// extent per axis (IMPORT_NRM), so an angle between stored normals is not the drawn one (two normals 35 deg apart
// on a flat sheet, IMPORT_FLAT, compared as one). The frame's per-axis scale is the extent itself (n / s is the
// authored normal, up to length). The geometry - quadric error, budget, boundary planes, collapse point, flip test -
// and the blend of a collapse's normal and tangent stay in the stored frame on purpose: an error there counts
// relative to the extent along each axis, so a thin body keeps its thinness (measured isotropically a 1 x 0.05 x 1 m
// lens lost its rim first, and a 10 x 0.1 x 0.1 m box flip-tested as drawn reached its last level 4x further off its
// surface), and a stored-frame blend is exact on an ellipsoid (blended as drawn, the lens's mean normal error rose).
// Null, or a box that is not an import's (an import's native_ext is > 0 and under 1e7 on every axis): the identity.
inline void metric(const float* ext, double s[3]) {
    s[0] = 1.0; s[1] = 1.0; s[2] = 1.0;
    if (!ext) return;
    for (int k = 0; k < 3; ++k) {
        if (!(ext[k] > 0.0f && ext[k] < 1.0e7f)) return;
    }
    for (int k = 0; k < 3; ++k) s[k] = ext[k];
}

}

// LOD_SKIN: the skin influences of a collapse's survivor - b folded into a where the new point lies the fraction t
// of the way from a to b (the uv's and the normal's fraction, LOD_SEAM). Per bone, (1 - t) of a's weight and t of
// b's; the four strongest kept (SkinInf's four slots; a tie to the lower bone index) and scaled back to the blended
// sum, so what a and b left to the root (1 - the sum: skin_verts' rest share) stays the root's, and the sum never
// passes the larger of the two (skin_sup_prepare's convex-combination test). t at 0 or 1 is a's or b's own, exactly.
inline SkinInf lod_inf_blend(const SkinInf& a, const SkinInf& b, double t) {
    if (!(t > 0.0)) return a;
    if (t >= 1.0) return b;
    const float s[2] = { static_cast<float>(1.0 - t), static_cast<float>(t) };
    uint16_t bn[8] = {};
    float w[8] = {};
    int n = 0;
    float sum = 0.0f;
    for (int side = 0; side < 2; ++side) {
        const SkinInf& f = side == 0 ? a : b;
        for (int k = 0; k < 4; ++k) {
            if (!(f.w[k] > 0.0f)) continue;   // skin_verts' own skip.
            const float v = f.w[k] * s[side];
            sum += v;
            int j = 0;
            while (j < n && bn[j] != f.b[k]) ++j;
            if (j == n) {
                bn[n] = f.b[k];
                w[n] = 0.0f;
                ++n;
            }
            w[j] += v;
        }
    }
    for (int i = 0; i < n; ++i) {   // Strongest first: a selection over eight at most.
        int m = i;
        for (int j = i + 1; j < n; ++j) {
            if (w[j] > w[m] ||
                (w[j] == w[m] && bn[j] < bn[m])) m = j;
        }
        std::swap(w[i], w[m]);
        std::swap(bn[i], bn[m]);
    }
    const int keep = n < 4 ? n : 4;
    float kept = 0.0f;
    for (int i = 0; i < keep; ++i) kept += w[i];
    const float sc = n > 4 && kept > 0.0f ? sum / kept : 1.0f;
    SkinInf o;
    for (int i = 0; i < keep; ++i) {
        o.b[i] = bn[i];
        o.w[i] = w[i] * sc;
    }
    return o;
}
// LOD_SKIN: a baked vertex's key in fbx_import's influence map (the squashed pre-bake position: bake's y / z
// swap is its own inverse).
inline ClothPosKey lod_skin_key(const MeshVertex& v) {
    ClothPosKey k;
    k.p[0] = v.pos[0];
    k.p[1] = v.pos[2];
    k.p[2] = v.pos[1];
    return k;
}

// LOD_PROGRESSIVE: decimate one expanded triangle soup through levels levels in ONE run, emitting each level
// into outs as the run reaches it. plan(l, prev, target, cap) gives level l's target (triangles or fewer)
// and cost cap (the largest collapse cost it may spend, quadric units; 0 = unbounded) from prev, the triangles of
// level l - 1 (of the input for l = 0). The heap is cheapest-first and a level only stops the run - its cap peeks at
// the top rather than popping it, and the pop bound carries on - so while the targets never rise and the caps never
// fall, every level is byte for byte what a run from the input to that level alone gives (LOD_FROM0), at the cost
// of one run. A plan that breaks that order ends the run there: the levels left repeat the last one, which the
// ladder's 90 % check refuses. A level whose target is not below the input's count, and every level of a structural
// refusal, is a copy of the input.
// LOD_TWO_PASS: only levels 0 .. keep - 1 are materialised into outs; every level's triangle count goes
// to counts by the emission's own rule, so a counting run chooses what a keeping run emits.
// LOD_JOINT: tag, when given, is each source triangle's submesh (src holds every submesh, one after
// another); an edge whose two triangles carry different tags is a material border, constrained as a uv seam is, so
// the run moves both sides together. tag_outs takes each materialised level's tags, one per emitted triangle,
// in emission order.
// LOD_SKIN: inf, when given with inf_outs, is each source vertex's skin influences (one per src
// entry; the corners at one position agree, as the import keys them by position). A welded vertex takes its first
// corner's, and each collapse blends the folded vertex's into the survivor's (lod_inf_blend, at the uv's
// fraction); inf_outs takes each materialised level's, one per emitted vertex, in emission order - a level that
// copies the input or an earlier level copies those influences too. They feed no cost and no decision: every level
// is byte for byte the same with or without them.
// LOD_METRIC: ms, when given, is the angle frame's per-axis scale (lod_qem::metric of the mesh's native_ext):
// the hard-edge test and the corner's keep-its-normal test compare their normals there (n / s) - the angles as drawn.
// The geometry and the normal / tangent blend stay stored (see lod_qem::metric). Null: the stored frame.
template <class LdPlan>
inline void lod_decimate(const std::vector<MeshVertex>& src, int levels, int keep,
                         LdPlan&& plan, std::vector<std::vector<MeshVertex>>& outs,
                         std::vector<size_t>& counts,
                         const std::vector<uint32_t>* tag = nullptr,
                         std::vector<std::vector<uint32_t>>* tag_outs = nullptr,
                         const std::vector<SkinInf>* inf = nullptr,
                         std::vector<std::vector<SkinInf>>* inf_outs = nullptr,
                         const double* ms = nullptr) {
    if (levels < 0) levels = 0;
    if (keep > levels) keep = levels;
    if (keep < 0) keep = 0;
    // LOD_METRIC: the frame's scale, and a stored normal into it (n / s, unit length; a zero one left zero).
    const double ld_m[3] = { ms ? ms[0] : 1.0, ms ? ms[1] : 1.0, ms ? ms[2] : 1.0 };
    auto to_m = [&](const float* s, double* o) {
        for (int v_k = 0; v_k < 3; ++v_k) o[v_k] = s[v_k] / ld_m[v_k];
        const double v_l = sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2]);
        if (v_l > 0.0) for (int v_k = 0; v_k < 3; ++v_k) o[v_k] /= v_l;
    };
    outs.clear();
    outs.resize(static_cast<size_t>(keep));
    const size_t ntri = src.size() / 3;
    counts.assign(static_cast<size_t>(levels), ntri);
    // LOD_JOINT: the tags (taken only when there is one per source triangle) and the input's own, which a level
    // that copies the input carries.
    const uint32_t* tg = tag && tag->size() == ntri ? tag->data() : nullptr;
    std::vector<uint32_t> src_tags;
    if (tag_outs) {
        tag_outs->clear();
        tag_outs->resize(static_cast<size_t>(keep));
        if (tg) src_tags.assign(tg, tg + ntri);
        else src_tags.assign(ntri, 0u);
    }
    // LOD_SKIN: the influences (only when asked for and one per source vertex) and the input's own (none where
    // none came).
    const SkinInf* ld_if =
        inf_outs && inf && inf->size() == src.size() ? inf->data() : nullptr;
    std::vector<SkinInf> src_inf;
    if (inf_outs) {
        inf_outs->clear();
        inf_outs->resize(static_cast<size_t>(keep));
        if (ld_if) src_inf.assign(ld_if, ld_if + src.size());
        else src_inf.assign(src.size(), SkinInf());
    }
    auto refuse = [&]() {
        for (std::vector<MeshVertex>& o : outs) o = src;
        if (tag_outs) for (std::vector<uint32_t>& o : *tag_outs) o = src_tags;
        if (inf_outs) for (std::vector<SkinInf>& o : *inf_outs) o = src_inf;
    };
    if (ntri < LOD_MIN_TRIS) { refuse(); return; }

    // Weld by position alone (LOD_TOPO: see the note below).
    struct Key {
        int32_t k[3];   // LOD_TOPO: position only - see the weld note.
        bool operator==(const Key& o) const {
            for (int i = 0; i < 3; ++i) if (k[i] != o.k[i]) return false;
            return true;
        }
    };
    struct KeyHash {
        size_t operator()(const Key& k_k) const {
            // Word-wise FNV over the quantised coordinates. Lookup only: the weld numbers its vertices in
            // first-sighting order, never in this map's.
            uint64_t h = CryptoGenerator::FNV1A64_OFFSET;

            for (int i = 0; i < 3; ++i) {
                h ^= static_cast<uint64_t>(static_cast<uint32_t>(k_k.k[i]));
                h *= CryptoGenerator::FNV1A64_PRIME;
            }

            return static_cast<size_t>(h);
        }
    };
    auto quant = [](float v, float step) {
        return static_cast<int32_t>(v < 0.0f ? v / step - 0.5f
                                                 : v / step + 0.5f);
    };
    std::unordered_map<Key, uint32_t, KeyHash> map;
    std::vector<double> pos;   // 3 per welded vertex.
    // Attribute donor per welded vertex (kept for the weld; the output reads the wedges).
    std::vector<MeshVertex> attr;
    std::vector<SkinInf> vinf;   // LOD_SKIN: per welded vertex, its influences as the run stands.
    // LOD_TOPO: weld by position, keep the wedges - otherwise every hard edge and uv seam splits the mesh into
    // pieces whose split edges read as open boundaries.
    std::vector<MeshVertex> wedge;   // 3 per accepted triangle: the corner's own attributes.
    std::vector<lod_qem::Tri> tri;
    std::vector<uint32_t> ttag;   // LOD_JOINT: per accepted triangle, its submesh (0 untagged).
    map.reserve(src.size());
    pos.reserve(src.size() * 3);
    attr.reserve(src.size());
    if (ld_if) vinf.reserve(src.size());
    tri.reserve(ntri);

    for (size_t ld_t = 0; ld_t < ntri; ++ld_t) {
        lod_qem::Tri f = {};
        f.live = true;

        for (int c = 0; c < 3; ++c) {
            const MeshVertex& v = src[ld_t * 3 + c];
            Key ld_k = {};
            ld_k.k[0] = quant(v.pos[0], 1.0e-5f);
            ld_k.k[1] = quant(v.pos[1], 1.0e-5f);
            ld_k.k[2] = quant(v.pos[2], 1.0e-5f);
            auto it = map.find(ld_k);
            uint32_t ix;

            if (it == map.end()) {
                ix = static_cast<uint32_t>(attr.size());
                map.emplace(ld_k, ix);
                pos.push_back(v.pos[0]);
                pos.push_back(v.pos[1]);
                pos.push_back(v.pos[2]);
                attr.push_back(v);
                if (ld_if) vinf.push_back(ld_if[ld_t * 3 + static_cast<size_t>(c)]);   // LOD_SKIN.
            } else ix = it->second;

            f.v[c] = ix;
        }

        if (f.v[0] == f.v[1] || f.v[1] == f.v[2] ||
            f.v[0] == f.v[2]) continue;
        tri.push_back(f);
        ttag.push_back(tg ? tg[ld_t] : 0u);
        for (int c = 0; c < 3; ++c) wedge.push_back(src[ld_t * 3 + c]);
    }

    const size_t nv = attr.size();
    if (nv < 4 || tri.size() < LOD_MIN_TRIS) { refuse(); return; }

    std::vector<std::vector<uint32_t>> adj(nv);
    std::vector<lod_qem::Q> ld_q(nv);
    for (size_t i = 0; i < nv; ++i) lod_qem::q_zero(ld_q[i]);
    std::unordered_map<uint64_t, uint32_t> ecount;
    // LOD_TOPO: first sighting's wedges.
    std::unordered_map<uint64_t, std::pair<uint32_t, std::pair<MeshVertex, MeshVertex>>> efirst;
    // LOD_TOPO: attribute-discontinuous edges - 2 = a uv seam, 1 = a hard edge alone (LOD_CREASE).
    std::unordered_map<uint64_t, uint8_t> eseam;
    std::unordered_map<uint64_t, uint32_t> etag;   // LOD_JOINT: the first sighting's submesh.
    auto ekey = [](uint32_t a, uint32_t b) {
        const uint64_t lo = a < b ? a : b;
        const uint64_t hi = a < b ? b : a;
        return (hi << 32) | lo;
    };

    for (size_t ld_t = 0; ld_t < tri.size(); ++ld_t) {
        const uint32_t* v = tri[ld_t].v;
        double n[3];
        lod_qem::tri_normal(&pos[v[0] * 3], &pos[v[1] * 3],
                            &pos[v[2] * 3], n);
        const double len = sqrt(n[0] * n[0] + n[1] * n[1] +
                                n[2] * n[2]);
        if (len < 1.0e-18) continue;
        const double area = 0.5 * len;
        const double a = n[0] / len, ld_b = n[1] / len,
                     c = n[2] / len;
        const double d = -(a * pos[v[0] * 3] +
                           ld_b * pos[v[0] * 3 + 1] +
                           c * pos[v[0] * 3 + 2]);
        lod_qem::Q pq;
        lod_qem::q_plane(pq, a, ld_b, c, d, area);

        for (int ld_k = 0; ld_k < 3; ++ld_k) {
            lod_qem::q_add(ld_q[v[ld_k]], pq);
            adj[v[ld_k]].push_back(static_cast<uint32_t>(ld_t));
            {   // LOD_TOPO: edge census + attribute-seam detection
                // (the wedges disagree across the edge).
                const uint64_t ek = ekey(v[ld_k], v[(ld_k + 1) % 3]);
                ecount[ek]++;
                const MeshVertex& w0 = wedge[ld_t * 3 + ld_k];
                const MeshVertex& w1 = wedge[ld_t * 3 + (ld_k + 1) % 3];
                auto ef = efirst.find(ek);
                if (ef == efirst.end()) {
                    efirst.emplace(ek, std::make_pair(v[ld_k], std::make_pair(w0, w1)));
                    if (tg) etag.emplace(ek, ttag[ld_t]);
                } else {
                    // LOD_JOINT: two submeshes meet here - a material border, a seam.
                    if (tg && etag[ek] != ttag[ld_t]) eseam[ek] = 2;
                    // Orient to the first sighting's corner order.
                    const MeshVertex& pa = ef->second.first == v[ld_k] ? w0 : w1;
                    const MeshVertex& pb = ef->second.first == v[ld_k] ? w1 : w0;
                    const MeshVertex& qa = ef->second.second.first;
                    const MeshVertex& qb = ef->second.second.second;
                    auto diff = [&](const MeshVertex& x, const MeshVertex& y) -> uint8_t {
                        const float du = fabsf(x.uv[0] - y.uv[0]) + fabsf(x.uv[1] - y.uv[1]);
                        double xn[3], yn[3];   // LOD_METRIC: the angle as drawn.
                        to_m(x.nrm, xn);
                        to_m(y.nrm, yn);
                        const double dn = xn[0] * yn[0] + xn[1] * yn[1] +
                                          xn[2] * yn[2];
                        // A uv seam (2), else a hard edge - normals over ~10 deg apart (1). LOD_HAND: a
                        // tangent handedness that flips (a mirrored uv layout's centre line, where the uvs agree)
                        // is a seam too - a collapse across it handed one side the other side's tangent.
                        return (du > 1.0e-4f || x.tan[3] != y.tan[3]) ? 2 : (dn < 0.985f ? 1 : 0);
                    };
                    const uint8_t ka = diff(pa, qa), kb = diff(pb, qb);
                    const uint8_t kk = ka > kb ? ka : kb;
                    if (kk != 0) {
                        uint8_t& ks = eseam[ek];
                        if (kk > ks) ks = kk;
                    }
                }
            }
        }
    }

    // Boundary constraint: a plane through an open edge or a uv seam (a material border among them, LOD_JOINT),
    // perpendicular to its triangle, weighted in the face planes' units (LOD_SCALE). Defends silhouettes and
    // texture seams without locking them (locked, seam-heavy models refuse to reach low levels). LOD_CREASE: a
    // hard edge alone takes none - the face planes meeting there hold a real crease's shape, each corner keeps its
    // own normal, and its plane locked every vertex of a faceted mesh.
    for (size_t ld_t = 0; ld_t < tri.size(); ++ld_t) {
        const uint32_t* v = tri[ld_t].v;
        double n[3];
        lod_qem::tri_normal(&pos[v[0] * 3], &pos[v[1] * 3],
                            &pos[v[2] * 3], n);
        const double len = sqrt(n[0] * n[0] + n[1] * n[1] +
                                n[2] * n[2]);
        if (len < 1.0e-18) continue;
        n[0] /= len; n[1] /= len; n[2] /= len;

        for (int ld_k = 0; ld_k < 3; ++ld_k) {
            const uint32_t i0 = v[ld_k], i1 = v[(ld_k + 1) % 3];
            const uint32_t ec = ecount[ekey(i0, i1)];
            const auto se = eseam.find(ekey(i0, i1));
            const bool is_seam = ec == 2 && se != eseam.end() && se->second == 2;
            if (ec != 1 && !is_seam) continue;
            const double e[3] = { pos[i1 * 3] - pos[i0 * 3],
                                  pos[i1 * 3 + 1] - pos[i0 * 3 + 1],
                                  pos[i1 * 3 + 2] - pos[i0 * 3 + 2] };
            const double bw = (is_seam ? LOD_SEAM_W : LOD_OPEN_W) *   // LOD_SCALE.
                              (e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
            double bn[3] = { e[1] * n[2] - e[2] * n[1],
                             e[2] * n[0] - e[0] * n[2],
                             e[0] * n[1] - e[1] * n[0] };
            const double bl = sqrt(bn[0] * bn[0] + bn[1] * bn[1] +
                                   bn[2] * bn[2]);
            if (bl < 1.0e-18) continue;
            bn[0] /= bl; bn[1] /= bl; bn[2] /= bl;
            const double bd = -(bn[0] * pos[i0 * 3] +
                                bn[1] * pos[i0 * 3 + 1] +
                                bn[2] * pos[i0 * 3 + 2]);
            lod_qem::Q bq;
            lod_qem::q_plane(bq, bn[0], bn[1], bn[2], bd, bw);
            lod_qem::q_add(ld_q[i0], bq);
            lod_qem::q_add(ld_q[i1], bq);
        }
    }

    // LOD_SEAM: an open edge or a uv seam (a material border among them, LOD_JOINT) is a constrained edge. A
    // vertex on exactly two sits on one seam or border path and moves only along it, onto its neighbour there (kind
    // 1); a vertex on one or on three or more - a path's end or a junction - or on a non-manifold edge never moves
    // (kind 2); any other is interior (kind 0). The set follows the collapses: a moved vertex's constrained edges
    // become the survivor's.
    std::vector<std::vector<uint32_t>> cnb(nv);   // Neighbours along constrained edges.
    std::vector<uint8_t> pin(nv, 0);              // On a non-manifold edge.
    for (const auto& kv : ecount) {
        const uint32_t e0 = static_cast<uint32_t>(kv.first & 0xFFFFFFFFull);
        const uint32_t e1 = static_cast<uint32_t>(kv.first >> 32);
        if (kv.second > 2) { pin[e0] = 1; pin[e1] = 1; continue; }
        bool ce = kv.second == 1;
        if (!ce) {
            const auto se = eseam.find(kv.first);
            ce = se != eseam.end() && se->second == 2;
        }
        if (ce) { cnb[e0].push_back(e1); cnb[e1].push_back(e0); }
    }
    auto kind = [&](uint32_t v) -> int {
        if (pin[v]) return 2;
        const size_t n = cnb[v].size();
        return n == 0 ? 0 : (n == 2 ? 1 : 2);
    };
    auto cedge = [&](uint32_t a, uint32_t b) -> bool {
        for (const uint32_t x_k : cnb[a]) if (x_k == b) return true;
        return false;
    };

    std::vector<uint64_t> stamp(nv, 0);
    std::vector<uint8_t> dead(nv, 0);
    std::vector<lod_qem::Cand> pq2;
    const lod_qem::CandWorse worse;

    // The optimal point is admitted only inside the edge's own reach (a sphere of radius |ab| about the midpoint); a
    // solve landing outside is a solve of the wrong system, and the endpoints / midpoint pick takes over.
    auto near_edge = [&](uint32_t a, uint32_t b, const double p[3]) -> bool {
        const double e[3] = { pos[b * 3] - pos[a * 3],
                              pos[b * 3 + 1] - pos[a * 3 + 1],
                              pos[b * 3 + 2] - pos[a * 3 + 2] };
        const double l2 = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
        const double n_m[3] = { 0.5 * (pos[a * 3] + pos[b * 3]),
                                0.5 * (pos[a * 3 + 1] + pos[b * 3 + 1]),
                                0.5 * (pos[a * 3 + 2] + pos[b * 3 + 2]) };
        const double d[3] = { p[0] - n_m[0], p[1] - n_m[1], p[2] - n_m[2] };
        const double d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        return d2 == d2 && d2 <= l2 + 1.0e-18;
    };
    auto place = [&](uint32_t a, uint32_t b, double o[3]) {
        lod_qem::Q q_p = ld_q[a];
        lod_qem::q_add(q_p, ld_q[b]);
        double best[3];

        if (lod_qem::q_min(q_p, best) && near_edge(a, b, best)) {
            o[0] = best[0]; o[1] = best[1]; o[2] = best[2];
            return lod_qem::q_eval(q_p, o[0], o[1], o[2]);
        }

        const double c[3][3] = {
            { pos[a * 3], pos[a * 3 + 1], pos[a * 3 + 2] },
            { pos[b * 3], pos[b * 3 + 1], pos[b * 3 + 2] },
            { 0.5 * (pos[a * 3] + pos[b * 3]),
              0.5 * (pos[a * 3 + 1] + pos[b * 3 + 1]),
              0.5 * (pos[a * 3 + 2] + pos[b * 3 + 2]) },
        };
        double bc = 1.0e300;
        int bi = 2;

        for (int i = 0; i < 3; ++i) {
            const double e = lod_qem::q_eval(q_p, c[i][0], c[i][1], c[i][2]);
            if (e < bc) { bc = e; bi = i; }
        }

        o[0] = c[bi][0]; o[1] = c[bi][1]; o[2] = c[bi][2];
        return bc;
    };
    // LOD_SEAM: the collapse of edge (a, b) the kinds allow, and its cost. Two interior vertices meet at the
    // quadric's best point (place); otherwise the vertex that may move goes onto the other where it stands - the
    // cheaper way when both may. keep_b = b survives. LOD_REFUSED when no way is.
    auto choose = [&](uint32_t a, uint32_t b, double o[3], bool& keep_b) -> double {
        keep_b = false;
        const int ka = kind(a), kb = kind(b);
        if (ka == 0 && kb == 0) return place(a, b, o);
        const bool path = cedge(a, b);
        const bool b_moves = kb == 0 || (kb == 1 && path);   // b onto a.
        const bool a_moves = ka == 0 || (ka == 1 && path);   // a onto b.
        lod_qem::Q c_q = ld_q[a];
        lod_qem::q_add(c_q, ld_q[b]);
        double best = LOD_REFUSED;
        if (b_moves) {
            best = lod_qem::q_eval(c_q, pos[a * 3], pos[a * 3 + 1], pos[a * 3 + 2]);
            for (int c_k = 0; c_k < 3; ++c_k) o[c_k] = pos[a * 3 + c_k];
        }
        if (a_moves) {
            const double e = lod_qem::q_eval(c_q, pos[b * 3], pos[b * 3 + 1],
                                             pos[b * 3 + 2]);
            if (e < best) {
                best = e;
                for (int c_k = 0; c_k < 3; ++c_k) o[c_k] = pos[b * 3 + c_k];
                keep_b = true;
            }
        }
        return best;
    };
    auto push = [&](uint32_t a, uint32_t b) {
        if (a == b || dead[a] || dead[b]) return;
        double p[3];
        bool kb = false;
        lod_qem::Cand c;
        c.cost = choose(a, b, p, kb);   // LOD_SEAM.
        if (c.cost >= LOD_REFUSED) return;
        c.a = a; c.b = b;
        c.sa = stamp[a]; c.sb = stamp[b];
        pq2.push_back(c);
        std::push_heap(pq2.begin(), pq2.end(), worse);
    };

    for (const auto& kv : ecount) {
        push(static_cast<uint32_t>(kv.first & 0xFFFFFFFFull),
             static_cast<uint32_t>(kv.first >> 32));
    }

    size_t live = tri.size();
    uint64_t ver = 1;
    // The pop bound stops a pathological mesh spinning (generous by design). One count for the whole run, as a run
    // to the last level would keep.
    size_t budget = tri.size() * 32 + 4096;

    // LOD_PROGRESSIVE: the level machine. ld_l is the level being run (its target and cap below); open
    // says one is.
    int ld_l = -1;
    bool open = false, ran = false;
    size_t prev = ntri, target = 0;
    double cost_cap = 0.0;
    // A level's triangles as the run stands: every live face, each corner with its own attributes at its vertex's
    // current position. LOD_JOINT: otag, when given, takes each emitted triangle's submesh. LOD_SKIN:
    // oinf, when given, each emitted vertex's influences.
    auto emit = [&](std::vector<uint32_t>* otag,
                    std::vector<SkinInf>* oinf) -> std::vector<MeshVertex> {
        std::vector<MeshVertex> out;
        out.reserve(live * 3);
        if (otag) otag->clear();
        if (oinf) oinf->clear();

        for (size_t ot = 0; ot < tri.size(); ++ot) {
            const lod_qem::Tri& f = tri[ot];
            if (!f.live) continue;
            if (f.v[0] == f.v[1] || f.v[1] == f.v[2] ||
                f.v[0] == f.v[2]) continue;
            double n[3];
            lod_qem::tri_normal(&pos[f.v[0] * 3], &pos[f.v[1] * 3],
                                &pos[f.v[2] * 3], n);
            if (n[0] * n[0] + n[1] * n[1] + n[2] * n[2] < 1.0e-24) continue;

            for (int ld_k = 0; ld_k < 3; ++ld_k) {
                MeshVertex v = wedge[ot * 3 + ld_k];   // LOD_TOPO: the corner's own
                                                                        // attributes.
                v.pos[0] = static_cast<float>(pos[f.v[ld_k] * 3]);
                v.pos[1] = static_cast<float>(pos[f.v[ld_k] * 3 + 1]);
                v.pos[2] = static_cast<float>(pos[f.v[ld_k] * 3 + 2]);
                out.push_back(v);
                if (oinf) oinf->push_back(ld_if ? vinf[f.v[ld_k]] : SkinInf());
            }
            if (otag) otag->push_back(ttag[ot]);
        }

        if (out.size() < 9) {
            if (otag) *otag = src_tags;
            if (oinf) *oinf = src_inf;
            return src;
        }
        return out;
    };
    // LOD_TWO_PASS: a level's triangle count is its emission's, taken and dropped at once (a single level held).
    auto count = [&]() -> size_t { return emit(nullptr, nullptr).size() / 3; };
    // A level closes: materialised if kept, counted either way.
    auto close = [&](int cl) {
        size_t c;
        if (cl < keep) {
            outs[static_cast<size_t>(cl)] =
                emit(tag_outs ? &(*tag_outs)[static_cast<size_t>(cl)] : nullptr,
                     inf_outs ? &(*inf_outs)[static_cast<size_t>(cl)] : nullptr);
            c = outs[static_cast<size_t>(cl)].size() / 3;
        } else {
            c = count();
        }
        counts[static_cast<size_t>(cl)] = c;
        prev = c;
    };

    auto level_open = [&]() {   // Opens the next level that runs, filling the ones that do not.
        open = false;
        for (++ld_l; ld_l < levels; ++ld_l) {
            size_t lt = 0;
            double lc = 0.0;
            plan(ld_l, prev, lt, lc);
            if (lt >= ntri) {
                if (ld_l < keep) {
                    outs[static_cast<size_t>(ld_l)] = src;
                    if (tag_outs) (*tag_outs)[static_cast<size_t>(ld_l)] = src_tags;
                    if (inf_outs) (*inf_outs)[static_cast<size_t>(ld_l)] = src_inf;
                }
                counts[static_cast<size_t>(ld_l)] = ntri;
                prev = ntri;
                continue;
            }
            const bool order = !ran ||
                (lt <= target &&
                 (cost_cap == 0.0 ? lc == 0.0 : (lc == 0.0 || lc >= cost_cap)));
            if (!order) {   // Not a continuation of the run: the levels left repeat the last one.
                for (int r = ld_l; r < levels; ++r) {
                    counts[static_cast<size_t>(r)] = counts[static_cast<size_t>(ld_l - 1)];
                    if (r < keep) {
                        outs[static_cast<size_t>(r)] = outs[static_cast<size_t>(ld_l - 1)];
                        if (tag_outs) {
                            (*tag_outs)[static_cast<size_t>(r)] =
                                (*tag_outs)[static_cast<size_t>(ld_l - 1)];
                        }
                        if (inf_outs) {   // LOD_SKIN.
                            (*inf_outs)[static_cast<size_t>(r)] =
                                (*inf_outs)[static_cast<size_t>(ld_l - 1)];
                        }
                    }
                }
                ld_l = levels;
                return;
            }
            target = lt;
            cost_cap = lc;
            ran = true;
            open = true;
            return;
        }
    };
    // Whether the loop pops again: the level closes at its target, an empty heap, the pop bound, or a top over its
    // cap (LOD_TOPO: the error budget is spent) - peeked, not popped and not counted, so the next level pops it on
    // the same count. The top is the heap's cheapest, so every candidate left is over the cap too; a stale top the
    // run would have popped first changes nothing, and the next level pops it in the same place.
    auto more = [&]() -> bool {
        while (open) {
            // The bound is spent without wrapping: the count carries into the next level.
            if (live > target && !pq2.empty() && budget > 0) {
                if (!(cost_cap > 0.0 && pq2.front().cost > cost_cap)) {
                    --budget;
                    return true;
                }
            }
            close(ld_l);
            level_open();
        }
        return false;
    };
    level_open();

    while (more()) {
        std::pop_heap(pq2.begin(), pq2.end(), worse);
        const lod_qem::Cand c = pq2.back();
        pq2.pop_back();
        if (dead[c.a] || dead[c.b]) continue;
        if (stamp[c.a] != c.sa || stamp[c.b] != c.sb) continue;
        double p[3];
        bool keep_b = false;
        // LOD_SEAM: re-asked here - a neighbour's collapse can change a kind without touching this pair's stamps.
        if (choose(c.a, c.b, p, keep_b) >= LOD_REFUSED) continue;
        const uint32_t a = keep_b ? c.b : c.a;   // The survivor.
        const uint32_t ld_b = keep_b ? c.a : c.b;   // Folds into it.

        // Flip rejection: a collapse that turns a triangle inside out is a visible fold the quadric cost cannot see.
        bool flip = false;

        for (int s = 0; s < 2 && !flip; ++s) {
            const uint32_t from = s == 0 ? a : ld_b;

            for (uint32_t ti : adj[from]) {
                if (!tri[ti].live) continue;
                const uint32_t* v = tri[ti].v;
                bool has_a = false, has_b = false;

                for (int ld_k = 0; ld_k < 3; ++ld_k) {
                    if (v[ld_k] == a) has_a = true;
                    if (v[ld_k] == ld_b) has_b = true;
                }

                if (has_a && has_b) continue;   // This face dies anyway.
                double old[3], ld_new[3];
                double pp[3][3];

                for (int ld_k = 0; ld_k < 3; ++ld_k) {
                    pp[ld_k][0] = pos[v[ld_k] * 3];
                    pp[ld_k][1] = pos[v[ld_k] * 3 + 1];
                    pp[ld_k][2] = pos[v[ld_k] * 3 + 2];
                }

                lod_qem::tri_normal(pp[0], pp[1], pp[2], old);

                for (int ld_k = 0; ld_k < 3; ++ld_k) {
                    if (v[ld_k] != a && v[ld_k] != ld_b) continue;
                    pp[ld_k][0] = p[0];
                    pp[ld_k][1] = p[1];
                    pp[ld_k][2] = p[2];
                }

                lod_qem::tri_normal(pp[0], pp[1], pp[2], ld_new);
                const double dot = old[0] * ld_new[0] + old[1] * ld_new[1] +
                                   old[2] * ld_new[2];
                const double ol = sqrt(old[0] * old[0] + old[1] * old[1] + old[2] * old[2]);
                const double nl = sqrt(ld_new[0] * ld_new[0] + ld_new[1] * ld_new[1] + ld_new[2] * ld_new[2]);
                if (nl < 1.0e-12 || dot <= 0.2 * ol * nl) { flip = true; break; }
            }
        }

        if (flip) continue;
        // LOD_SEAM: the corners' attributes follow the collapse. Each dying face pairs a's corner with b's on its
        // side of any seam; every surviving corner at a or b on that side (the same uv as the pair's) takes the
        // pair's attributes at the new point - a's own where b moved onto a - and a crease corner (its normal apart
        // from the pair's) keeps its normal. A corner of b no dying face pairs would carry its uv across a seam:
        // refused.
        struct LdPair { MeshVertex wa, wb, wn; };
        LdPair pr[4];
        int npr = 0;
        bool bad = false;
        double ld_t = 0.0;   // Where p lies from a to b.
        {
            const double ab[3] = { pos[ld_b * 3] - pos[a * 3],
                                   pos[ld_b * 3 + 1] - pos[a * 3 + 1],
                                   pos[ld_b * 3 + 2] - pos[a * 3 + 2] };
            const double l2 = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
            if (l2 > 1.0e-30) {
                ld_t = ((p[0] - pos[a * 3]) * ab[0] +
                        (p[1] - pos[a * 3 + 1]) * ab[1] +
                        (p[2] - pos[a * 3 + 2]) * ab[2]) / l2;
                ld_t = ld_t < 0.0 ? 0.0 : (ld_t > 1.0 ? 1.0 : ld_t);
            }
        }
        auto uv_eq = [](const MeshVertex& x, const MeshVertex& y) {   // LOD_HAND: and one handedness.
            return fabsf(x.uv[0] - y.uv[0]) + fabsf(x.uv[1] - y.uv[1]) <= 1.0e-4f && x.tan[3] == y.tan[3];
        };
        auto corner = [&](uint32_t q_t, uint32_t v) -> int {
            for (int q_k = 0; q_k < 3; ++q_k) if (tri[q_t].v[q_k] == v) return q_k;
            return -1;
        };
        for (uint32_t ti : adj[ld_b]) {   // The dying faces: the pairs.
            if (!tri[ti].live) continue;
            const int ka = corner(ti, a);
            if (ka < 0) continue;
            if (npr == 4) { bad = true; break; }
            const int kb = corner(ti, ld_b);
            LdPair& q2 = pr[npr++];
            q2.wa = wedge[ti * 3 + static_cast<uint32_t>(ka)];
            q2.wb = wedge[ti * 3 + static_cast<uint32_t>(kb)];
            q2.wn = q2.wa;
            const float tf = static_cast<float>(ld_t);
            const MeshVertex& xa = q2.wa;
            const MeshVertex& xb = q2.wb;
            MeshVertex& xn = q2.wn;
            for (int ld_k = 0; ld_k < 2; ++ld_k) {
                xn.uv[ld_k] = xa.uv[ld_k] + (xb.uv[ld_k] - xa.uv[ld_k]) * tf;
            }
            float nl = 0.0f, tl = 0.0f;
            for (int ld_k = 0; ld_k < 3; ++ld_k) {
                xn.nrm[ld_k] = xa.nrm[ld_k] + (xb.nrm[ld_k] - xa.nrm[ld_k]) * tf;
                xn.tan[ld_k] = xa.tan[ld_k] + (xb.tan[ld_k] - xa.tan[ld_k]) * tf;
                nl += q2.wn.nrm[ld_k] * q2.wn.nrm[ld_k];
                tl += q2.wn.tan[ld_k] * q2.wn.tan[ld_k];
            }
            if (nl > 1.0e-12f) {
                const float ni = 1.0f / sqrtf(nl);
                for (int ld_k = 0; ld_k < 3; ++ld_k) q2.wn.nrm[ld_k] *= ni;
            } else {
                for (int ld_k = 0; ld_k < 3; ++ld_k) q2.wn.nrm[ld_k] = q2.wa.nrm[ld_k];
            }
            if (tl > 1.0e-12f) {
                const float ti2 = 1.0f / sqrtf(tl);
                for (int ld_k = 0; ld_k < 3; ++ld_k) q2.wn.tan[ld_k] *= ti2;
            } else {
                for (int ld_k = 0; ld_k < 3; ++ld_k) q2.wn.tan[ld_k] = q2.wa.tan[ld_k];
            }
        }
        if (bad || npr == 0) continue;
        auto pair_of = [&](const MeshVertex& vtx, bool w_b) -> int {
            for (int i = 0; i < npr; ++i) {
                if (uv_eq(vtx, w_b ? pr[i].wb : pr[i].wa)) return i;
            }
            return -1;
        };
        for (uint32_t ti : adj[ld_b]) {   // Every corner of b a face keeps must pair.
            if (!tri[ti].live || corner(ti, a) >= 0) continue;
            const int kb = corner(ti, ld_b);
            if (pair_of(wedge[ti * 3 + static_cast<uint32_t>(kb)], true) < 0) {
                bad = true;
                break;
            }
        }
        if (bad) continue;
        auto take = [&](MeshVertex& vtx, const LdPair& pair_in, bool w_b) {
            const MeshVertex& ref = w_b ? pair_in.wb : pair_in.wa;
            double w[3], r[3];   // LOD_METRIC: the angle as drawn.
            to_m(vtx.nrm, w);
            to_m(ref.nrm, r);
            // LOD_METRIC: the hard-edge test's literal (0.985f), so the two tests are exact complements.
            const bool n = w[0] * r[0] + w[1] * r[1] + w[2] * r[2] >= 0.985f;
            vtx.uv[0] = pair_in.wn.uv[0];
            vtx.uv[1] = pair_in.wn.uv[1];
            for (int ld_k = 0; ld_k < 4; ++ld_k) vtx.tan[ld_k] = pair_in.wn.tan[ld_k];
            if (n) for (int ld_k = 0; ld_k < 3; ++ld_k) vtx.nrm[ld_k] = pair_in.wn.nrm[ld_k];
        };
        for (int s = 0; s < 2; ++s) {   // The corners that stay, at a then at b.
            const uint32_t at = s == 0 ? a : ld_b;
            if (s == 0 && ld_t <= 0.0) continue;   // b moved onto a: a's corners keep theirs.
            for (uint32_t ti : adj[at]) {
                if (!tri[ti].live) continue;
                if (corner(ti, s == 0 ? ld_b : a) >= 0) continue;   // Dies.
                const int ld_k = corner(ti, at);
                MeshVertex& w = wedge[ti * 3 + static_cast<uint32_t>(ld_k)];
                const int i = pair_of(w, s == 1);
                if (i >= 0) take(w, pr[i], s == 1);
            }
        }
        // Commit: b folds into a at p.
        pos[a * 3] = p[0];
        pos[a * 3 + 1] = p[1];
        pos[a * 3 + 2] = p[2];
        lod_qem::q_add(ld_q[a], ld_q[ld_b]);
        // LOD_SKIN: b's influences fold into a's at the uv's fraction.
        if (ld_if) vinf[a] = lod_inf_blend(vinf[a], vinf[ld_b], ld_t);
        dead[ld_b] = 1;
        stamp[a] = ++ver;
        stamp[ld_b] = ++ver;

        for (uint32_t ti : adj[ld_b]) {
            if (!tri[ti].live) continue;
            uint32_t* v = tri[ti].v;
            bool has_a = false;
            for (int ld_k = 0; ld_k < 3; ++ld_k) if (v[ld_k] == a) has_a = true;

            if (has_a) {
                tri[ti].live = false;
                live--;
                continue;
            }

            for (int ld_k = 0; ld_k < 3; ++ld_k) if (v[ld_k] == ld_b) v[ld_k] = a;
            adj[a].push_back(ti);
        }

        adj[ld_b].clear();
        // LOD_SEAM: b's constrained edges become a's (the edge a - b is gone).
        for (const uint32_t ld_x : cnb[ld_b]) {
            std::vector<uint32_t>& xn = cnb[ld_x];
            xn.erase(std::remove(xn.begin(), xn.end(), ld_b), xn.end());
            if (ld_x == a || cedge(a, ld_x)) continue;
            cnb[a].push_back(ld_x);
            xn.push_back(a);
        }
        cnb[ld_b].clear();
        // Re-price every edge still touching the surviving vertex a.
        std::vector<uint32_t> nb;

        for (uint32_t ti : adj[a]) {
            if (!tri[ti].live) continue;
            const uint32_t* v = tri[ti].v;
            for (int ld_k = 0; ld_k < 3; ++ld_k) {
                if (v[ld_k] != a && !dead[v[ld_k]]) nb.push_back(v[ld_k]);
            }
        }

        std::sort(nb.begin(), nb.end());
        nb.erase(std::unique(nb.begin(), nb.end()), nb.end());
        for (uint32_t n2 : nb) push(a, n2);
    }
}

// Build levels 1..LOD_MAX onto a baked MeshDef. Idempotent; any level that refuses to shrink ends the ladder and
// lod_n names what exists. LOD_FROM0: every level is decimated from level 0, to half the previous level's count
// under that level's budget - all of the mesh's levels in one run (LOD_PROGRESSIVE, lod_decimate). Do not
// chain levels (decimate level l from level l - 1): a collapsed vertex's triangles keep their source corners'
// attributes, so a level's output reads as seams all over to the next (2 % -> 28 % -> 57 % of a 16k sphere's edges)
// and the ladder locks after a level or two.
// LOD_SKIN: skin, when given with skin_out, is the import's influences by position (fbx_import's
// sk_map; lod_skin_key): each source vertex's seeds the run, and skin_out takes the influences of every
// vertex a kept level holds at a point skin does not know - one the decimator placed anew, or a source point the
// import gave no entry (its seed: none) - keyed the same way; the first at a point stands.
inline void lod_build(MeshDef& d,
                      const std::unordered_map<ClothPosKey, SkinInf, ClothPosHash>* skin = nullptr,
                      std::unordered_map<ClothPosKey, SkinInf, ClothPosHash>* skin_out = nullptr) {
    KH_PROF_SCOPE(PROF_LOD_BUILD);   // PROF.
    if (d.lod_n) return;
    if (d.submeshes.empty() || d.verts.empty()) return;
    if (!d.indices.empty()) return;   // Expanded form only (mesh_weld runs after).
    const uint32_t base = static_cast<uint32_t>(d.verts.size());
    // The ladder halves the triangle count per level, which is why the screen-size selector is a log2.
    // LOD_JOINT: every submesh in ONE soup, one after another, each triangle tagged with its submesh. Decimated
    // apart, a border two submeshes share is thinned twice, differently - two polylines, and a crack between the
    // materials; in one run its two sides are one set of vertices, constrained as a uv seam is.
    std::vector<MeshVertex> cur;
    std::vector<uint32_t> tag;   // Per triangle of cur: its submesh.
    for (size_t s = 0; s < d.submeshes.size(); ++s) {
        const MeshSubmesh& sm = d.submeshes[s];
        const uint32_t n = sm.index_count - sm.index_count % 3u;   // Whole triangles.
        cur.insert(cur.end(), d.verts.begin() + sm.index_start,
                   d.verts.begin() + sm.index_start + n);
        tag.insert(tag.end(), n / 3u, static_cast<uint32_t>(s));
    }
    // LOD_SKIN: each source vertex's influences, by its position (none where the import has none).
    const bool sk = skin != nullptr && skin_out != nullptr;
    std::vector<SkinInf> inf;
    if (sk) {
        inf.resize(cur.size());
        for (size_t i = 0; i < cur.size(); ++i) {
            const auto it = skin->find(lod_skin_key(cur[i]));
            if (it != skin->end()) inf[i] = it->second;
        }
    }

    // The error budget: quadric cost is area x distance squared, so the budget lets a level move a vertex by tol_l
    // against six mean faces (the previous level's mean face: level 0's quadrics accumulate as its faces merge). A
    // level the budget stops short of 90 % of the previous one ends the ladder.
    double area = 0.0, diag = 0.0;
    {
        const std::vector<MeshVertex>& v = cur;
        double mn[3] = { 1.0e300, 1.0e300, 1.0e300 }, mx[3] = { -1.0e300, -1.0e300, -1.0e300 };
        for (const MeshVertex& p : v) {
            for (int k = 0; k < 3; ++k) {
                if (p.pos[k] < mn[k]) mn[k] = p.pos[k];
                if (p.pos[k] > mx[k]) mx[k] = p.pos[k];
            }
        }
        for (size_t t = 0; t + 2 < v.size(); t += 3) {
            const double p0[3] = { v[t].pos[0], v[t].pos[1], v[t].pos[2] };
            const double p1[3] = { v[t + 1].pos[0], v[t + 1].pos[1], v[t + 1].pos[2] };
            const double p2[3] = { v[t + 2].pos[0], v[t + 2].pos[1], v[t + 2].pos[2] };
            double n[3];
            lod_qem::tri_normal(p0, p1, p2, n);
            area += 0.5 * sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        }
        if (!v.empty()) {
            const double dx = mx[0] - mn[0], dy = mx[1] - mn[1], dz = mx[2] - mn[2];
            diag = sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    // LOD_PROGRESSIVE: every level from level 0, in one run. Level l wants half of level l - 1 (the floor under
    // that) and spends at most its budget; both follow the previous level's count, so the targets fall and the caps
    // rise, which is what lets one run stand for a run per level. LOD_TWO_PASS: pass 1 counts every level, the
    // 90 % check below picks how many are kept from the counts, and pass 2 materialises those alone - a stalling
    // mesh never holds a dozen copies of itself.
    std::vector<std::vector<MeshVertex>> lvls;
    std::vector<std::vector<uint32_t>> tags;
    std::vector<std::vector<SkinInf>> infs;   // LOD_SKIN.
    std::vector<size_t> cnt;
    auto plan = [&](int l, size_t tris, size_t& want, double& cap) {
        want = tris / 2 < LOD_FLOOR_TRIS ? LOD_FLOOR_TRIS : tris / 2;
        cap = 0.0;
        if (tris > 0 && area > 0.0 && diag > 0.0) {
            const double mean_a = area / static_cast<double>(tris);
            const double edge = 1.52 * sqrt(mean_a);   // Equilateral side for that area.
            const double lv = static_cast<double>(1u << l);
            const double tol = std::min(0.5 * lv * edge, 0.02 * lv * diag);
            cap = tol * tol * mean_a * 6.0;
        }
    };
    double ms[3];   // LOD_METRIC: the frame the run judges normals in.
    lod_qem::metric(d.native_ext, ms);
    lod_decimate(cur, LOD_MAX, 0, plan, lvls, cnt, &tag,   // Pass 1: counts alone.
                 nullptr, nullptr, nullptr, ms);
    // The levels kept, from the counts: a level that did not shrink below 90 % of the one before is not worth a draw
    // range, a cache entry or a transition, and ends the ladder.
    int keep = 0;
    for (; keep < LOD_MAX; ++keep) {
        const size_t bt = keep == 0 ? cur.size() / 3 : cnt[static_cast<size_t>(keep - 1)];
        const size_t at = cnt[static_cast<size_t>(keep)];
        if (bt == 0 || at * 100 > bt * 90) break;
    }
    if (keep == 0) return;
    lod_decimate(cur, keep, keep, plan, lvls, cnt, &tag, &tags,   // Pass 2.
                 sk ? &inf : nullptr, sk ? &infs : nullptr, ms);
    for (int l = 0; l < keep; ++l) {
        // The level back into its submeshes, in submesh order: a triangle keeps its submesh through every collapse,
        // and a submesh may end a level empty (a small material thinned away - its range draws nothing).
        std::vector<std::vector<MeshVertex>> next(d.submeshes.size());
        {
            const std::vector<MeshVertex>& lv = lvls[static_cast<size_t>(l)];
            const std::vector<uint32_t>& lt = tags[static_cast<size_t>(l)];
            for (size_t t = 0; t < lt.size() && t * 3 + 2 < lv.size(); ++t) {
                std::vector<MeshVertex>& o = next[lt[t]];
                o.insert(o.end(), lv.begin() + static_cast<std::ptrdiff_t>(t * 3),
                         lv.begin() + static_cast<std::ptrdiff_t>(t * 3 + 3));
            }
            if (sk) {   // LOD_SKIN: the points this level placed anew.
                const std::vector<SkinInf>& li = infs[static_cast<size_t>(l)];
                for (size_t i = 0; i < lv.size() && i < li.size(); ++i) {
                    const ClothPosKey k = lod_skin_key(lv[i]);
                    if (skin->find(k) == skin->end()) skin_out->emplace(k, li[i]);
                }
                std::vector<SkinInf>().swap(infs[static_cast<size_t>(l)]);
            }
        }
        std::vector<MeshVertex>().swap(lvls[static_cast<size_t>(l)]);   // One level held at a time.
        const uint32_t start = static_cast<uint32_t>(d.verts.size());
        d.lod_istart[l] = start;
        d.lod_sub[l].resize(d.submeshes.size());

        for (size_t s = 0; s < next.size(); ++s) {
            MeshSubmesh sm;
            sm.name = d.submeshes[s].name;
            sm.index_start = static_cast<uint32_t>(d.verts.size());
            sm.index_count = static_cast<uint32_t>(next[s].size());
            d.verts.insert(d.verts.end(), next[s].begin(), next[s].end());
            d.lod_sub[l][s] = std::move(sm);
        }

        d.lod_icount[l] = static_cast<uint32_t>(d.verts.size()) - start;
        d.lod_n = static_cast<uint8_t>(l + 1);
    }

    // Level 0 runs from index 0 to level 1's lod_istart (mesh_base_icount); with nothing built, the whole stream.
    if (d.lod_n == 0) return;
    if (d.lod_istart[0] != base) d.lod_n = 0;   // Unreachable; a guard (nothing asserts it).
}

// LOD selection: screen size first, range second. The fade t comes back in the open
// interval - both endpoints mean "not fading" to the shader.
static constexpr float LOD_REF_PX    = 320.0f;   // Half-height px at f 0; level 0 ends at half this or sooner.
static constexpr float LOD_DIST_BIAS = 0.35f;   // Levels per log2 of the range term.
static constexpr float LOD_DIST_REF  = 100.0f;
static constexpr float LOD_FADE      = 0.30f;   // Fraction of a level spent crossfading.
// LOD_ZOOM: the unzoomed view's P11 (projection[1][1], the vertical half-FOV's cotangent) - the game's default
// fovTop of 0.75 - against which lod_zoom_of measures a zoom. An error of a factor k here moves an unzoomed
// view's range term by at most LOD_DIST_BIAS * log2(k) levels (0.09 at k = 1.2).
static constexpr float LOD_P11_REF   = 1.0f / 0.75f;

// LOD_ZOOM: a projection's zoom for the level pick's range term - P11 over the unzoomed view's, never below 1 (a
// view wider than the reference keeps the range term as it was), capped (an absurd P11 stays finite), 1 if unusable.
inline float lod_zoom_of(float p11) {
    const float z = p11 / LOD_P11_REF;
    if (!(z > 1.0f)) return 1.0f;   // NaN-safe.
    return z < 1.0e4f ? z : 1.0e4f;
}

// LOD_ONE_SCALE: the main view's level-pick scale, the one definition both publishers (the injection, the flush)
// use: pixels per unit tangent = P11 * H / 2, H the height of the viewport the pass draws our meshes with - the
// viewport transform maps NDC y [-1, 1] onto Height pixels, so radius * that / distance is the mesh's projected
// radius in the pass's own pixels. Not the adopted main depth texture's height: the largest seen, it may exceed the
// scene viewport, and the two publishes of one frame then disagree (the shadow builds' drawn_lod takes whichever
// came last). With it, g_lod.zoom (the same projection's). False, nothing written, for an unusable pair.
inline bool lod_publish(float p11, float vp_h) {
    if (!(vp_h > 0.0f) || !(p11 > 0.0f)) return false;
    g_lod.proj_px = p11 * vp_h * 0.5f;
    g_lod.zoom = lod_zoom_of(p11);
    return true;
}

// PIP_LOD: proj_px, when given (>= 0), is the pass's own pixels-per-tangent in place of g_lod.proj_px.
// LOD_ZOOM: zoom, when given (>= 0; a pass passing its own proj_px passes it too), is the pass's own
// lod_zoom_of in place of g_lod.zoom. The range term takes the distance divided by the zoom - the distance at
// which the mesh would look this size unzoomed - so a view through optics picks the level the same on-screen size
// picks unzoomed (it took the world distance: a 4x view at 400 m drew 0.46 levels coarser than the bare view at
// 100 m of the same picture, 10x at 1000 m 0.86); unzoomed (zoom 1) the pick is unchanged.
inline void lod_pick(const MeshDef& d, float radius, float dist,
                     int& lvl, float& t, float proj_px = -1.0f, float zoom = -1.0f) {
    lvl = 0;
    t = 0.0f;
    if (d.lod_n == 0) return;
    const float pp = proj_px >= 0.0f ? proj_px : g_lod.proj_px;
    if (!(pp > 0.0f) || !(radius > 0.0f)) return;
    const float zr = zoom >= 0.0f ? zoom : g_lod.zoom;
    const float z = zr > 1.0f ? zr : 1.0f;   // LOD_ZOOM (NaN-safe).
    const float px = radius * pp / (dist > 0.01f ? dist : 0.01f);
    float f = log2f(LOD_REF_PX / (px > 0.01f ? px : 0.01f));
    f += LOD_DIST_BIAS * log2f(1.0f + (dist > 0.0f ? dist : 0.0f) / (LOD_DIST_REF * z));
    if (!(f > 0.0f)) return;   // NaN-safe: an insane pair reads as level 0.
    const float top = static_cast<float>(d.lod_n);

    if (f >= top) {
        lvl = static_cast<int>(d.lod_n);
        return;
    }

    const float base = floorf(f);
    lvl = static_cast<int>(base);
    const float frac = f - base;
    if (frac <= 1.0f - LOD_FADE) return;
    float p = (frac - (1.0f - LOD_FADE)) / LOD_FADE;
    if (p < 0.02f) return;   // Not yet worth a second draw.
    if (p > 0.98f) p = 0.98f;   // The coarse level never draws alone here.
    t = p;
}

// The dither value blendCtl.w takes for one half of a crossfade pair: second false = the finer level fading
// out, true = the coarser fading in. Complementary at every pixel.
inline float lod_dither(float t, bool second) {
    if (!(t > 0.0f)) return 0.0f;
    const float v = 1.0f - t;
    return second ? -v : v;
}

// The object's bounding-sphere radius in world metres, from the rotated half extents every draw path computes for
// its own culling.
inline float lod_radius_of(const float* size, const float* rot_m, bool rotated) {
    const float hl[3] = { size[0] * 0.5f, size[2] * 0.5f, size[1] * 0.5f };
    float he[3];
    rot_half_extents(hl, rot_m, rotated, he);
    return sqrtf(he[0] * he[0] + he[1] * he[1] + he[2] * he[2]);
}

// SKEL_LOD_R: the LOD pick's radius. A skeletal object's size is its pose's box (the skin owns it), which
// breathes with the animation - arms out, arms in - so its pick takes the REST box, the mesh's native_size: the
// radius does not move with the pose (the distance is still to the pose's box, which a limb moves by its own reach
// alone). Every pick reads this - the colour loops and the PIP directly, the sun / DLS / seam records from their
// builds (drawn_lod), the dlsw mask from its record's copy (dlsw_mask_render) - so colour and shadows agree.
inline float lod_radius(const RenderObject& o) {
    if (o.skel) {
        return lod_radius_of(mesh_def(mesh_id_clamp(o.mesh)).native_size, o.rot_m, o.rotated);
    }
    return lod_radius_of(o.size, o.rot_m, o.rotated);
}

// Every depth-only pass that re-draws the scene's meshes (sun tiers, alpha casters, DLS maps, seam footprint, mirror
// prepass, prime) draws the level the colour pass picked for this object at this camera - the finer half of a fade,
// 0 under a lock - so a map holds the silhouette the player sees and every run groups on (mesh, level).
// g_lod.proj_px unpublished reads as level 0. r is the record's lod_radius, taken at its build (SKEL_LOD_R).
inline int drawn_lod(int mid, float r, const float* pos, const float* size,
                     const float* rot, bool rotated, bool lock, const float cam[3]) {
    if (lock) return 0;
    const MeshDef& md = mesh_def(mid);
    if (md.lod_n == 0) return 0;
    int   lvl = 0;
    float t = 0.0f;
    lod_pick(md, r,
             sqrtf(mesh_dist_sq_of(pos, size, rot, rotated, cam)),
             lvl, t);
    return mesh_lod_clamp(md, lvl);
}
inline void mesh_lod_range_of(int mid, int lvl, UINT& s, UINT& c) {
    mesh_lod_range(mesh_def(mid), lvl, s, c);
}
}