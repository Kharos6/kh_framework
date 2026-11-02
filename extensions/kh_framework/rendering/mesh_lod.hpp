#pragma once

// The feature half (rendering_integration_features.hpp): the LOD builder (khlod, kh_lod_build) and the LOD picker.
// Never include it on its own.
namespace RenderIntegration {
// KH_LOD_SIM_SLOT: whether an object draws level 0 alone - its lodLock, or a cloth's or chain's buffer in its slot
// (kh_mesh_vb_ref's own substitution test). clothSimulation and chainSimulation force the lock, but a script can clear
// it in the frame its simulation ends, and the slot keeps the simulation's buffer until the next flush's uploads hand
// it back: a coarser level drawn from it would show its own vertices at rest. Every per-object pick (injection, flush,
// PIP) and the caster records (sun, DLS, seam, dlsw mask) ask this; the instanced bucket admits no object whose slot
// holds a substitute, but for a skin pool chunk its bucket pulls from (kh_inst_eligible).
inline bool kh_lod_locked(const RenderObject& khll_o) {
    if (khll_o.lod_lock) return true;
    const uint32_t khll_s = khll_o.slot;
    return khll_s < g_cloth_sim_slot.size() && g_cloth_sim_slot[khll_s] != 0u && khll_s < g_cloth_vb_slot.size() &&
           g_cloth_vb_slot[khll_s] != nullptr && khll_s < g_cloth_mesh_slot.size() &&
           g_cloth_mesh_slot[khll_s] == mesh_id_clamp(khll_o.mesh);
}

// Automatic LOD for imported models. Decimation runs over the whole mesh at once (KH_LOD_JOINT), each triangle
// keeping its submesh, so a level's submesh table has level 0's cardinality and order.
static constexpr size_t KH_LOD_MIN_TRIS   = 32;
static constexpr size_t KH_LOD_FLOOR_TRIS = 12;
// KH_LOD_SCALE: the edge penalties in the face planes' units (area x distance squared) - a weight x the edge's
// length squared. Do not make them absolute (50 / 1000): they out-weigh a unit-box face ~10^4 times and pin every
// vertex on a seam (a UV-unwrapped cylinder with a seam on a third of its edges never got a level).
static constexpr double KH_LOD_SEAM_W = 10.0;    // A uv seam.
static constexpr double KH_LOD_OPEN_W = 100.0;   // An open edge.
static constexpr double KH_LOD_REFUSED = 1.0e300;   // KH_LOD_SEAM: a collapse no rule allows.

namespace khlod {

// Symmetric 4x4 quadric, upper triangle: a2 ab ac ad b2 bc bd c2 cd d2.
struct Q {
    double m[10];
};

inline void q_zero(Q& khq_q) {
    for (int khq_i = 0; khq_i < 10; ++khq_i) khq_q.m[khq_i] = 0.0;
}

inline void q_plane(Q& khq_q, double khq_a, double khq_b, double khq_c, double khq_d,
                    double khq_w) {
    khq_q.m[0] = khq_w * khq_a * khq_a; khq_q.m[1] = khq_w * khq_a * khq_b;
    khq_q.m[2] = khq_w * khq_a * khq_c; khq_q.m[3] = khq_w * khq_a * khq_d;
    khq_q.m[4] = khq_w * khq_b * khq_b; khq_q.m[5] = khq_w * khq_b * khq_c;
    khq_q.m[6] = khq_w * khq_b * khq_d; khq_q.m[7] = khq_w * khq_c * khq_c;
    khq_q.m[8] = khq_w * khq_c * khq_d; khq_q.m[9] = khq_w * khq_d * khq_d;
}

inline void q_add(Q& khq_r, const Q& khq_s) {
    for (int khq_i = 0; khq_i < 10; ++khq_i) khq_r.m[khq_i] += khq_s.m[khq_i];
}

inline double q_eval(const Q& khq_q, double khq_x, double khq_y, double khq_z) {
    return khq_q.m[0] * khq_x * khq_x + 2.0 * khq_q.m[1] * khq_x * khq_y +
           2.0 * khq_q.m[2] * khq_x * khq_z + 2.0 * khq_q.m[3] * khq_x +
           khq_q.m[4] * khq_y * khq_y + 2.0 * khq_q.m[5] * khq_y * khq_z +
           2.0 * khq_q.m[6] * khq_y + khq_q.m[7] * khq_z * khq_z +
           2.0 * khq_q.m[8] * khq_z + khq_q.m[9];
}

// Minimiser of the quadric: the 3x3 gradient system. False on a singular or ill-conditioned system; the caller then
// takes the best of the two endpoints and the midpoint.
inline bool q_min(const Q& khq_q, double khq_o[3]) {
    const double khq_a[3][3] = {
        { khq_q.m[0], khq_q.m[1], khq_q.m[2] },
        { khq_q.m[1], khq_q.m[4], khq_q.m[5] },
        { khq_q.m[2], khq_q.m[5], khq_q.m[7] },
    };
    const double khq_det =
        khq_a[0][0] * (khq_a[1][1] * khq_a[2][2] - khq_a[1][2] * khq_a[2][1]) -
        khq_a[0][1] * (khq_a[1][0] * khq_a[2][2] - khq_a[1][2] * khq_a[2][0]) +
        khq_a[0][2] * (khq_a[1][0] * khq_a[2][1] - khq_a[1][1] * khq_a[2][0]);
    // The singularity test is relative to the quadric's own scale: an absolute epsilon passes near-singular systems
    // on unit-size low-poly meshes.
    const double khq_s = (khq_a[0][0] + khq_a[1][1] + khq_a[2][2]) / 3.0;
    if (!(khq_s > 0.0) || !(fabs(khq_det) > 1.0e-6 * khq_s * khq_s * khq_s)) return false;
    const double khq_r[3] = { -khq_q.m[3], -khq_q.m[6], -khq_q.m[8] };
    const double khq_inv = 1.0 / khq_det;
    khq_o[0] = khq_inv * (khq_r[0] * (khq_a[1][1] * khq_a[2][2] - khq_a[1][2] * khq_a[2][1]) -
                          khq_a[0][1] * (khq_r[1] * khq_a[2][2] - khq_a[1][2] * khq_r[2]) +
                          khq_a[0][2] * (khq_r[1] * khq_a[2][1] - khq_a[1][1] * khq_r[2]));
    khq_o[1] = khq_inv * (khq_a[0][0] * (khq_r[1] * khq_a[2][2] - khq_a[1][2] * khq_r[2]) -
                          khq_r[0] * (khq_a[1][0] * khq_a[2][2] - khq_a[1][2] * khq_a[2][0]) +
                          khq_a[0][2] * (khq_a[1][0] * khq_r[2] - khq_r[1] * khq_a[2][0]));
    khq_o[2] = khq_inv * (khq_a[0][0] * (khq_a[1][1] * khq_r[2] - khq_r[1] * khq_a[2][1]) -
                          khq_a[0][1] * (khq_a[1][0] * khq_r[2] - khq_r[1] * khq_a[2][0]) +
                          khq_r[0] * (khq_a[1][0] * khq_a[2][1] - khq_a[1][1] * khq_a[2][0]));
    return true;
}

struct Tri { uint32_t v[3]; bool live; };
struct Cand { double cost; uint32_t a, b; uint64_t sa, sb; };

// Heap via push_heap/pop_heap with an inverted comparator so the cheapest collapse comes off the top (no
// std::priority_queue: this header declares no includes).
struct CandWorse {
    bool operator()(const Cand& khc_x, const Cand& khc_y) const { return khc_x.cost > khc_y.cost; }
};

inline void tri_normal(const double khn_p0[3], const double khn_p1[3], const double khn_p2[3],
                       double khn_o[3]) {
    const double khn_u[3] = { khn_p1[0] - khn_p0[0], khn_p1[1] - khn_p0[1], khn_p1[2] - khn_p0[2] };
    const double khn_v[3] = { khn_p2[0] - khn_p0[0], khn_p2[1] - khn_p0[1], khn_p2[2] - khn_p0[2] };
    khn_o[0] = khn_u[1] * khn_v[2] - khn_u[2] * khn_v[1];
    khn_o[1] = khn_u[2] * khn_v[0] - khn_u[0] * khn_v[2];
    khn_o[2] = khn_u[0] * khn_v[1] - khn_u[1] * khn_v[0];
}

// KH_LOD_METRIC: the frame the decimator judges normals in. A stored normal is the authored one times the mesh's
// extent per axis (KH_IMPORT_NRM), so an angle between stored normals is not the drawn one (two normals 35 deg apart
// on a flat sheet, KH_IMPORT_FLAT, compared as one). The frame's per-axis scale is the extent itself (n / s is the
// authored normal, up to length). The geometry - quadric error, budget, boundary planes, collapse point, flip test -
// and the blend of a collapse's normal and tangent stay in the stored frame on purpose: an error there counts
// relative to the extent along each axis, so a thin body keeps its thinness (measured isotropically a 1 x 0.05 x 1 m
// lens lost its rim first, and a 10 x 0.1 x 0.1 m box flip-tested as drawn reached its last level 4x further off its
// surface), and a stored-frame blend is exact on an ellipsoid (blended as drawn, the lens's mean normal error rose).
// Null, or a box that is not an import's (an import's native_ext is > 0 and under 1e7 on every axis): the identity.
inline void metric(const float* khm_ext, double khm_s[3]) {
    khm_s[0] = 1.0; khm_s[1] = 1.0; khm_s[2] = 1.0;
    if (!khm_ext) return;
    for (int khm_k = 0; khm_k < 3; ++khm_k) {
        if (!(khm_ext[khm_k] > 0.0f && khm_ext[khm_k] < 1.0e7f)) return;
    }
    for (int khm_k = 0; khm_k < 3; ++khm_k) khm_s[khm_k] = khm_ext[khm_k];
}

}

// KH_LOD_SKIN: the skin influences of a collapse's survivor - b folded into a where the new point lies the fraction t
// of the way from a to b (the uv's and the normal's fraction, KH_LOD_SEAM). Per bone, (1 - t) of a's weight and t of
// b's; the four strongest kept (KhSkinInf's four slots; a tie to the lower bone index) and scaled back to the blended
// sum, so what a and b left to the root (1 - the sum: kh_skin_verts' rest share) stays the root's, and the sum never
// passes the larger of the two (kh_skin_sup_prepare's convex-combination test). t at 0 or 1 is a's or b's own, exactly.
inline KhSkinInf kh_lod_inf_blend(const KhSkinInf& khlsk_a, const KhSkinInf& khlsk_b, double khlsk_t) {
    if (!(khlsk_t > 0.0)) return khlsk_a;
    if (khlsk_t >= 1.0) return khlsk_b;
    const float khlsk_s[2] = { static_cast<float>(1.0 - khlsk_t), static_cast<float>(khlsk_t) };
    uint16_t khlsk_bn[8] = {};
    float khlsk_w[8] = {};
    int khlsk_n = 0;
    float khlsk_sum = 0.0f;
    for (int khlsk_side = 0; khlsk_side < 2; ++khlsk_side) {
        const KhSkinInf& khlsk_f = khlsk_side == 0 ? khlsk_a : khlsk_b;
        for (int khlsk_k = 0; khlsk_k < 4; ++khlsk_k) {
            if (!(khlsk_f.w[khlsk_k] > 0.0f)) continue;   // kh_skin_verts' own skip.
            const float khlsk_v = khlsk_f.w[khlsk_k] * khlsk_s[khlsk_side];
            khlsk_sum += khlsk_v;
            int khlsk_j = 0;
            while (khlsk_j < khlsk_n && khlsk_bn[khlsk_j] != khlsk_f.b[khlsk_k]) ++khlsk_j;
            if (khlsk_j == khlsk_n) {
                khlsk_bn[khlsk_n] = khlsk_f.b[khlsk_k];
                khlsk_w[khlsk_n] = 0.0f;
                ++khlsk_n;
            }
            khlsk_w[khlsk_j] += khlsk_v;
        }
    }
    for (int khlsk_i = 0; khlsk_i < khlsk_n; ++khlsk_i) {   // Strongest first: a selection over eight at most.
        int khlsk_m = khlsk_i;
        for (int khlsk_j = khlsk_i + 1; khlsk_j < khlsk_n; ++khlsk_j) {
            if (khlsk_w[khlsk_j] > khlsk_w[khlsk_m] ||
                (khlsk_w[khlsk_j] == khlsk_w[khlsk_m] && khlsk_bn[khlsk_j] < khlsk_bn[khlsk_m])) khlsk_m = khlsk_j;
        }
        std::swap(khlsk_w[khlsk_i], khlsk_w[khlsk_m]);
        std::swap(khlsk_bn[khlsk_i], khlsk_bn[khlsk_m]);
    }
    const int khlsk_keep = khlsk_n < 4 ? khlsk_n : 4;
    float khlsk_kept = 0.0f;
    for (int khlsk_i = 0; khlsk_i < khlsk_keep; ++khlsk_i) khlsk_kept += khlsk_w[khlsk_i];
    const float khlsk_sc = khlsk_n > 4 && khlsk_kept > 0.0f ? khlsk_sum / khlsk_kept : 1.0f;
    KhSkinInf khlsk_o;
    for (int khlsk_i = 0; khlsk_i < khlsk_keep; ++khlsk_i) {
        khlsk_o.b[khlsk_i] = khlsk_bn[khlsk_i];
        khlsk_o.w[khlsk_i] = khlsk_w[khlsk_i] * khlsk_sc;
    }
    return khlsk_o;
}
// KH_LOD_SKIN: a baked vertex's key in kh_fbx_import's influence map (the squashed pre-bake position: bake's y / z
// swap is its own inverse).
inline KhClothPosKey kh_lod_skin_key(const MeshVertex& khlsk_v) {
    KhClothPosKey khlsk_k;
    khlsk_k.p[0] = khlsk_v.pos[0];
    khlsk_k.p[1] = khlsk_v.pos[2];
    khlsk_k.p[2] = khlsk_v.pos[1];
    return khlsk_k;
}

// KH_LOD_PROGRESSIVE: decimate one expanded triangle soup through khld_levels levels in ONE run, emitting each level
// into khld_outs as the run reaches it. khld_plan(l, prev, target, cap) gives level l's target (triangles or fewer)
// and cost cap (the largest collapse cost it may spend, quadric units; 0 = unbounded) from prev, the triangles of
// level l - 1 (of the input for l = 0). The heap is cheapest-first and a level only stops the run - its cap peeks at
// the top rather than popping it, and the pop bound carries on - so while the targets never rise and the caps never
// fall, every level is byte for byte what a run from the input to that level alone gives (KH_LOD_FROM0), at the cost
// of one run. A plan that breaks that order ends the run there: the levels left repeat the last one, which the
// ladder's 90 % check refuses. A level whose target is not below the input's count, and every level of a structural
// refusal, is a copy of the input.
// KH_LOD_TWO_PASS: only levels 0 .. khld_keep - 1 are materialised into khld_outs; every level's triangle count goes
// to khld_counts by the emission's own rule, so a counting run chooses what a keeping run emits.
// KH_LOD_JOINT: khld_tag, when given, is each source triangle's submesh (khld_src holds every submesh, one after
// another); an edge whose two triangles carry different tags is a material border, constrained as a uv seam is, so
// the run moves both sides together. khld_tag_outs takes each materialised level's tags, one per emitted triangle,
// in emission order.
// KH_LOD_SKIN: khld_inf, when given with khld_inf_outs, is each source vertex's skin influences (one per khld_src
// entry; the corners at one position agree, as the import keys them by position). A welded vertex takes its first
// corner's, and each collapse blends the folded vertex's into the survivor's (kh_lod_inf_blend, at the uv's
// fraction); khld_inf_outs takes each materialised level's, one per emitted vertex, in emission order - a level that
// copies the input or an earlier level copies those influences too. They feed no cost and no decision: every level
// is byte for byte the same with or without them.
// KH_LOD_METRIC: khld_ms, when given, is the angle frame's per-axis scale (khlod::metric of the mesh's native_ext):
// the hard-edge test and the corner's keep-its-normal test compare their normals there (n / s) - the angles as drawn.
// The geometry and the normal / tangent blend stay stored (see khlod::metric). Null: the stored frame.
template <class KhLdPlan>
inline void kh_lod_decimate(const std::vector<MeshVertex>& khld_src, int khld_levels, int khld_keep,
                            KhLdPlan&& khld_plan, std::vector<std::vector<MeshVertex>>& khld_outs,
                            std::vector<size_t>& khld_counts,
                            const std::vector<uint32_t>* khld_tag = nullptr,
                            std::vector<std::vector<uint32_t>>* khld_tag_outs = nullptr,
                            const std::vector<KhSkinInf>* khld_inf = nullptr,
                            std::vector<std::vector<KhSkinInf>>* khld_inf_outs = nullptr,
                            const double* khld_ms = nullptr) {
    if (khld_levels < 0) khld_levels = 0;
    if (khld_keep > khld_levels) khld_keep = khld_levels;
    if (khld_keep < 0) khld_keep = 0;
    // KH_LOD_METRIC: the frame's scale, and a stored normal into it (n / s, unit length; a zero one left zero).
    const double khld_m[3] = { khld_ms ? khld_ms[0] : 1.0, khld_ms ? khld_ms[1] : 1.0, khld_ms ? khld_ms[2] : 1.0 };
    auto khld_to_m = [&](const float* khv_s, double* khv_o) {
        for (int khv_k = 0; khv_k < 3; ++khv_k) khv_o[khv_k] = khv_s[khv_k] / khld_m[khv_k];
        const double khv_l = sqrt(khv_o[0] * khv_o[0] + khv_o[1] * khv_o[1] + khv_o[2] * khv_o[2]);
        if (khv_l > 0.0) for (int khv_k = 0; khv_k < 3; ++khv_k) khv_o[khv_k] /= khv_l;
    };
    khld_outs.clear();
    khld_outs.resize(static_cast<size_t>(khld_keep));
    const size_t khld_ntri = khld_src.size() / 3;
    khld_counts.assign(static_cast<size_t>(khld_levels), khld_ntri);
    // KH_LOD_JOINT: the tags (taken only when there is one per source triangle) and the input's own, which a level
    // that copies the input carries.
    const uint32_t* khld_tg = khld_tag && khld_tag->size() == khld_ntri ? khld_tag->data() : nullptr;
    std::vector<uint32_t> khld_src_tags;
    if (khld_tag_outs) {
        khld_tag_outs->clear();
        khld_tag_outs->resize(static_cast<size_t>(khld_keep));
        if (khld_tg) khld_src_tags.assign(khld_tg, khld_tg + khld_ntri);
        else khld_src_tags.assign(khld_ntri, 0u);
    }
    // KH_LOD_SKIN: the influences (only when asked for and one per source vertex) and the input's own (none where
    // none came).
    const KhSkinInf* khld_if =
        khld_inf_outs && khld_inf && khld_inf->size() == khld_src.size() ? khld_inf->data() : nullptr;
    std::vector<KhSkinInf> khld_src_inf;
    if (khld_inf_outs) {
        khld_inf_outs->clear();
        khld_inf_outs->resize(static_cast<size_t>(khld_keep));
        if (khld_if) khld_src_inf.assign(khld_if, khld_if + khld_src.size());
        else khld_src_inf.assign(khld_src.size(), KhSkinInf());
    }
    auto khld_refuse = [&]() {
        for (std::vector<MeshVertex>& khld_o : khld_outs) khld_o = khld_src;
        if (khld_tag_outs) for (std::vector<uint32_t>& khld_o : *khld_tag_outs) khld_o = khld_src_tags;
        if (khld_inf_outs) for (std::vector<KhSkinInf>& khld_o : *khld_inf_outs) khld_o = khld_src_inf;
    };
    if (khld_ntri < KH_LOD_MIN_TRIS) { khld_refuse(); return; }

    // Weld by position alone (KH_LOD_TOPO: see the note below).
    struct Key {
        int32_t k[3];   // KH_LOD_TOPO: position only - see the weld note.
        bool operator==(const Key& khk_o) const {
            for (int khk_i = 0; khk_i < 3; ++khk_i) if (k[khk_i] != khk_o.k[khk_i]) return false;
            return true;
        }
    };
    struct KeyHash {
        size_t operator()(const Key& khk_k) const {
            // Word-wise FNV over the quantised coordinates. Lookup only: the weld numbers its vertices in
            // first-sighting order, never in this map's.
            uint64_t khk_h = CryptoGenerator::FNV1A64_OFFSET;

            for (int khk_i = 0; khk_i < 3; ++khk_i) {
                khk_h ^= static_cast<uint64_t>(static_cast<uint32_t>(khk_k.k[khk_i]));
                khk_h *= CryptoGenerator::FNV1A64_PRIME;
            }

            return static_cast<size_t>(khk_h);
        }
    };
    auto khld_quant = [](float khq_v, float khq_step) {
        return static_cast<int32_t>(khq_v < 0.0f ? khq_v / khq_step - 0.5f
                                                 : khq_v / khq_step + 0.5f);
    };
    std::unordered_map<Key, uint32_t, KeyHash> khld_map;
    std::vector<double> khld_pos;   // 3 per welded vertex.
    // Attribute donor per welded vertex (kept for the weld; the output reads the wedges).
    std::vector<MeshVertex> khld_attr;
    std::vector<KhSkinInf> khld_vinf;   // KH_LOD_SKIN: per welded vertex, its influences as the run stands.
    // KH_LOD_TOPO: weld by position, keep the wedges - otherwise every hard edge and uv seam splits the mesh into
    // pieces whose split edges read as open boundaries.
    std::vector<MeshVertex> khld_wedge;   // 3 per accepted triangle: the corner's own attributes.
    std::vector<khlod::Tri> khld_tri;
    std::vector<uint32_t> khld_ttag;   // KH_LOD_JOINT: per accepted triangle, its submesh (0 untagged).
    khld_map.reserve(khld_src.size());
    khld_pos.reserve(khld_src.size() * 3);
    khld_attr.reserve(khld_src.size());
    if (khld_if) khld_vinf.reserve(khld_src.size());
    khld_tri.reserve(khld_ntri);

    for (size_t khld_t = 0; khld_t < khld_ntri; ++khld_t) {
        khlod::Tri khld_f = {};
        khld_f.live = true;

        for (int khld_c = 0; khld_c < 3; ++khld_c) {
            const MeshVertex& khld_v = khld_src[khld_t * 3 + khld_c];
            Key khld_k = {};
            khld_k.k[0] = khld_quant(khld_v.pos[0], 1.0e-5f);
            khld_k.k[1] = khld_quant(khld_v.pos[1], 1.0e-5f);
            khld_k.k[2] = khld_quant(khld_v.pos[2], 1.0e-5f);
            auto khld_it = khld_map.find(khld_k);
            uint32_t khld_ix;

            if (khld_it == khld_map.end()) {
                khld_ix = static_cast<uint32_t>(khld_attr.size());
                khld_map.emplace(khld_k, khld_ix);
                khld_pos.push_back(khld_v.pos[0]);
                khld_pos.push_back(khld_v.pos[1]);
                khld_pos.push_back(khld_v.pos[2]);
                khld_attr.push_back(khld_v);
                if (khld_if) khld_vinf.push_back(khld_if[khld_t * 3 + static_cast<size_t>(khld_c)]);   // KH_LOD_SKIN.
            } else khld_ix = khld_it->second;

            khld_f.v[khld_c] = khld_ix;
        }

        if (khld_f.v[0] == khld_f.v[1] || khld_f.v[1] == khld_f.v[2] ||
            khld_f.v[0] == khld_f.v[2]) continue;
        khld_tri.push_back(khld_f);
        khld_ttag.push_back(khld_tg ? khld_tg[khld_t] : 0u);
        for (int khld_c = 0; khld_c < 3; ++khld_c) khld_wedge.push_back(khld_src[khld_t * 3 + khld_c]);
    }

    const size_t khld_nv = khld_attr.size();
    if (khld_nv < 4 || khld_tri.size() < KH_LOD_MIN_TRIS) { khld_refuse(); return; }

    std::vector<std::vector<uint32_t>> khld_adj(khld_nv);
    std::vector<khlod::Q> khld_q(khld_nv);
    for (size_t khld_i = 0; khld_i < khld_nv; ++khld_i) khlod::q_zero(khld_q[khld_i]);
    std::unordered_map<uint64_t, uint32_t> khld_ecount;
    // KH_LOD_TOPO: first sighting's wedges.
    std::unordered_map<uint64_t, std::pair<uint32_t, std::pair<MeshVertex, MeshVertex>>> khld_efirst;
    // KH_LOD_TOPO: attribute-discontinuous edges - 2 = a uv seam, 1 = a hard edge alone (KH_LOD_CREASE).
    std::unordered_map<uint64_t, uint8_t> khld_eseam;
    std::unordered_map<uint64_t, uint32_t> khld_etag;   // KH_LOD_JOINT: the first sighting's submesh.
    auto khld_ekey = [](uint32_t khe_a, uint32_t khe_b) {
        const uint64_t khe_lo = khe_a < khe_b ? khe_a : khe_b;
        const uint64_t khe_hi = khe_a < khe_b ? khe_b : khe_a;
        return (khe_hi << 32) | khe_lo;
    };

    for (size_t khld_t = 0; khld_t < khld_tri.size(); ++khld_t) {
        const uint32_t* khld_v = khld_tri[khld_t].v;
        double khld_n[3];
        khlod::tri_normal(&khld_pos[khld_v[0] * 3], &khld_pos[khld_v[1] * 3],
                          &khld_pos[khld_v[2] * 3], khld_n);
        const double khld_len = sqrt(khld_n[0] * khld_n[0] + khld_n[1] * khld_n[1] +
                                     khld_n[2] * khld_n[2]);
        if (khld_len < 1.0e-18) continue;
        const double khld_area = 0.5 * khld_len;
        const double khld_a = khld_n[0] / khld_len, khld_b = khld_n[1] / khld_len,
                     khld_c = khld_n[2] / khld_len;
        const double khld_d = -(khld_a * khld_pos[khld_v[0] * 3] +
                                khld_b * khld_pos[khld_v[0] * 3 + 1] +
                                khld_c * khld_pos[khld_v[0] * 3 + 2]);
        khlod::Q khld_pq;
        khlod::q_plane(khld_pq, khld_a, khld_b, khld_c, khld_d, khld_area);

        for (int khld_k = 0; khld_k < 3; ++khld_k) {
            khlod::q_add(khld_q[khld_v[khld_k]], khld_pq);
            khld_adj[khld_v[khld_k]].push_back(static_cast<uint32_t>(khld_t));
            {   // KH_LOD_TOPO: edge census + attribute-seam detection
                // (the wedges disagree across the edge).
                const uint64_t khld_ek = khld_ekey(khld_v[khld_k], khld_v[(khld_k + 1) % 3]);
                khld_ecount[khld_ek]++;
                const MeshVertex& khld_w0 = khld_wedge[khld_t * 3 + khld_k];
                const MeshVertex& khld_w1 = khld_wedge[khld_t * 3 + (khld_k + 1) % 3];
                auto khld_ef = khld_efirst.find(khld_ek);
                if (khld_ef == khld_efirst.end()) {
                    khld_efirst.emplace(khld_ek, std::make_pair(khld_v[khld_k], std::make_pair(khld_w0, khld_w1)));
                    if (khld_tg) khld_etag.emplace(khld_ek, khld_ttag[khld_t]);
                } else {
                    // KH_LOD_JOINT: two submeshes meet here - a material border, a seam.
                    if (khld_tg && khld_etag[khld_ek] != khld_ttag[khld_t]) khld_eseam[khld_ek] = 2;
                    // Orient to the first sighting's corner order.
                    const MeshVertex& khld_pa = khld_ef->second.first == khld_v[khld_k] ? khld_w0 : khld_w1;
                    const MeshVertex& khld_pb = khld_ef->second.first == khld_v[khld_k] ? khld_w1 : khld_w0;
                    const MeshVertex& khld_qa = khld_ef->second.second.first;
                    const MeshVertex& khld_qb = khld_ef->second.second.second;
                    auto khld_diff = [&](const MeshVertex& x, const MeshVertex& y) -> uint8_t {
                        const float khld_du = fabsf(x.uv[0] - y.uv[0]) + fabsf(x.uv[1] - y.uv[1]);
                        double khld_xn[3], khld_yn[3];   // KH_LOD_METRIC: the angle as drawn.
                        khld_to_m(x.nrm, khld_xn);
                        khld_to_m(y.nrm, khld_yn);
                        const double khld_dn = khld_xn[0] * khld_yn[0] + khld_xn[1] * khld_yn[1] +
                                               khld_xn[2] * khld_yn[2];
                        // A uv seam (2), else a hard edge - normals over ~10 deg apart (1). KH_LOD_HAND: a
                        // tangent handedness that flips (a mirrored uv layout's centre line, where the uvs agree)
                        // is a seam too - a collapse across it handed one side the other side's tangent.
                        return (khld_du > 1.0e-4f || x.tan[3] != y.tan[3]) ? 2 : (khld_dn < 0.985f ? 1 : 0);
                    };
                    const uint8_t khld_ka = khld_diff(khld_pa, khld_qa), khld_kb = khld_diff(khld_pb, khld_qb);
                    const uint8_t khld_kk = khld_ka > khld_kb ? khld_ka : khld_kb;
                    if (khld_kk != 0) {
                        uint8_t& khld_ks = khld_eseam[khld_ek];
                        if (khld_kk > khld_ks) khld_ks = khld_kk;
                    }
                }
            }
        }
    }

    // Boundary constraint: a plane through an open edge or a uv seam (a material border among them, KH_LOD_JOINT),
    // perpendicular to its triangle, weighted in the face planes' units (KH_LOD_SCALE). Defends silhouettes and
    // texture seams without locking them (locked, seam-heavy models refuse to reach low levels). KH_LOD_CREASE: a
    // hard edge alone takes none - the face planes meeting there hold a real crease's shape, each corner keeps its
    // own normal, and its plane locked every vertex of a faceted mesh.
    for (size_t khld_t = 0; khld_t < khld_tri.size(); ++khld_t) {
        const uint32_t* khld_v = khld_tri[khld_t].v;
        double khld_n[3];
        khlod::tri_normal(&khld_pos[khld_v[0] * 3], &khld_pos[khld_v[1] * 3],
                          &khld_pos[khld_v[2] * 3], khld_n);
        const double khld_len = sqrt(khld_n[0] * khld_n[0] + khld_n[1] * khld_n[1] +
                                     khld_n[2] * khld_n[2]);
        if (khld_len < 1.0e-18) continue;
        khld_n[0] /= khld_len; khld_n[1] /= khld_len; khld_n[2] /= khld_len;

        for (int khld_k = 0; khld_k < 3; ++khld_k) {
            const uint32_t khld_i0 = khld_v[khld_k], khld_i1 = khld_v[(khld_k + 1) % 3];
            const uint32_t khld_ec = khld_ecount[khld_ekey(khld_i0, khld_i1)];
            const auto khld_se = khld_eseam.find(khld_ekey(khld_i0, khld_i1));
            const bool khld_is_seam = khld_ec == 2 && khld_se != khld_eseam.end() && khld_se->second == 2;
            if (khld_ec != 1 && !khld_is_seam) continue;
            const double khld_e[3] = { khld_pos[khld_i1 * 3] - khld_pos[khld_i0 * 3],
                                       khld_pos[khld_i1 * 3 + 1] - khld_pos[khld_i0 * 3 + 1],
                                       khld_pos[khld_i1 * 3 + 2] - khld_pos[khld_i0 * 3 + 2] };
            const double khld_bw = (khld_is_seam ? KH_LOD_SEAM_W : KH_LOD_OPEN_W) *   // KH_LOD_SCALE.
                                   (khld_e[0] * khld_e[0] + khld_e[1] * khld_e[1] + khld_e[2] * khld_e[2]);
            double khld_bn[3] = { khld_e[1] * khld_n[2] - khld_e[2] * khld_n[1],
                                  khld_e[2] * khld_n[0] - khld_e[0] * khld_n[2],
                                  khld_e[0] * khld_n[1] - khld_e[1] * khld_n[0] };
            const double khld_bl = sqrt(khld_bn[0] * khld_bn[0] + khld_bn[1] * khld_bn[1] +
                                        khld_bn[2] * khld_bn[2]);
            if (khld_bl < 1.0e-18) continue;
            khld_bn[0] /= khld_bl; khld_bn[1] /= khld_bl; khld_bn[2] /= khld_bl;
            const double khld_bd = -(khld_bn[0] * khld_pos[khld_i0 * 3] +
                                     khld_bn[1] * khld_pos[khld_i0 * 3 + 1] +
                                     khld_bn[2] * khld_pos[khld_i0 * 3 + 2]);
            khlod::Q khld_bq;
            khlod::q_plane(khld_bq, khld_bn[0], khld_bn[1], khld_bn[2], khld_bd, khld_bw);
            khlod::q_add(khld_q[khld_i0], khld_bq);
            khlod::q_add(khld_q[khld_i1], khld_bq);
        }
    }

    // KH_LOD_SEAM: an open edge or a uv seam (a material border among them, KH_LOD_JOINT) is a constrained edge. A
    // vertex on exactly two sits on one seam or border path and moves only along it, onto its neighbour there (kind
    // 1); a vertex on one or on three or more - a path's end or a junction - or on a non-manifold edge never moves
    // (kind 2); any other is interior (kind 0). The set follows the collapses: a moved vertex's constrained edges
    // become the survivor's.
    std::vector<std::vector<uint32_t>> khld_cnb(khld_nv);   // Neighbours along constrained edges.
    std::vector<uint8_t> khld_pin(khld_nv, 0);              // On a non-manifold edge.
    for (const auto& khld_kv : khld_ecount) {
        const uint32_t khld_e0 = static_cast<uint32_t>(khld_kv.first & 0xFFFFFFFFull);
        const uint32_t khld_e1 = static_cast<uint32_t>(khld_kv.first >> 32);
        if (khld_kv.second > 2) { khld_pin[khld_e0] = 1; khld_pin[khld_e1] = 1; continue; }
        bool khld_ce = khld_kv.second == 1;
        if (!khld_ce) {
            const auto khld_se = khld_eseam.find(khld_kv.first);
            khld_ce = khld_se != khld_eseam.end() && khld_se->second == 2;
        }
        if (khld_ce) { khld_cnb[khld_e0].push_back(khld_e1); khld_cnb[khld_e1].push_back(khld_e0); }
    }
    auto khld_kind = [&](uint32_t khk_v) -> int {
        if (khld_pin[khk_v]) return 2;
        const size_t khk_n = khld_cnb[khk_v].size();
        return khk_n == 0 ? 0 : (khk_n == 2 ? 1 : 2);
    };
    auto khld_cedge = [&](uint32_t khk_a, uint32_t khk_b) -> bool {
        for (const uint32_t khk_x : khld_cnb[khk_a]) if (khk_x == khk_b) return true;
        return false;
    };

    std::vector<uint64_t> khld_stamp(khld_nv, 0);
    std::vector<uint8_t> khld_dead(khld_nv, 0);
    std::vector<khlod::Cand> khld_pq2;
    const khlod::CandWorse khld_worse;

    // The optimal point is admitted only inside the edge's own reach (a sphere of radius |ab| about the midpoint); a
    // solve landing outside is a solve of the wrong system, and the endpoints / midpoint pick takes over.
    auto khld_near_edge = [&](uint32_t khn_a, uint32_t khn_b, const double khn_p[3]) -> bool {
        const double khn_e[3] = { khld_pos[khn_b * 3] - khld_pos[khn_a * 3],
                                  khld_pos[khn_b * 3 + 1] - khld_pos[khn_a * 3 + 1],
                                  khld_pos[khn_b * 3 + 2] - khld_pos[khn_a * 3 + 2] };
        const double khn_l2 = khn_e[0] * khn_e[0] + khn_e[1] * khn_e[1] + khn_e[2] * khn_e[2];
        const double khn_m[3] = { 0.5 * (khld_pos[khn_a * 3] + khld_pos[khn_b * 3]),
                                  0.5 * (khld_pos[khn_a * 3 + 1] + khld_pos[khn_b * 3 + 1]),
                                  0.5 * (khld_pos[khn_a * 3 + 2] + khld_pos[khn_b * 3 + 2]) };
        const double khn_d[3] = { khn_p[0] - khn_m[0], khn_p[1] - khn_m[1], khn_p[2] - khn_m[2] };
        const double khn_d2 = khn_d[0] * khn_d[0] + khn_d[1] * khn_d[1] + khn_d[2] * khn_d[2];
        return khn_d2 == khn_d2 && khn_d2 <= khn_l2 + 1.0e-18;
    };
    auto khld_place = [&](uint32_t khp_a, uint32_t khp_b, double khp_o[3]) {
        khlod::Q khp_q = khld_q[khp_a];
        khlod::q_add(khp_q, khld_q[khp_b]);
        double khp_best[3];

        if (khlod::q_min(khp_q, khp_best) && khld_near_edge(khp_a, khp_b, khp_best)) {
            khp_o[0] = khp_best[0]; khp_o[1] = khp_best[1]; khp_o[2] = khp_best[2];
            return khlod::q_eval(khp_q, khp_o[0], khp_o[1], khp_o[2]);
        }

        const double khp_c[3][3] = {
            { khld_pos[khp_a * 3], khld_pos[khp_a * 3 + 1], khld_pos[khp_a * 3 + 2] },
            { khld_pos[khp_b * 3], khld_pos[khp_b * 3 + 1], khld_pos[khp_b * 3 + 2] },
            { 0.5 * (khld_pos[khp_a * 3] + khld_pos[khp_b * 3]),
              0.5 * (khld_pos[khp_a * 3 + 1] + khld_pos[khp_b * 3 + 1]),
              0.5 * (khld_pos[khp_a * 3 + 2] + khld_pos[khp_b * 3 + 2]) },
        };
        double khp_bc = 1.0e300;
        int khp_bi = 2;

        for (int khp_i = 0; khp_i < 3; ++khp_i) {
            const double khp_e = khlod::q_eval(khp_q, khp_c[khp_i][0], khp_c[khp_i][1], khp_c[khp_i][2]);
            if (khp_e < khp_bc) { khp_bc = khp_e; khp_bi = khp_i; }
        }

        khp_o[0] = khp_c[khp_bi][0]; khp_o[1] = khp_c[khp_bi][1]; khp_o[2] = khp_c[khp_bi][2];
        return khp_bc;
    };
    // KH_LOD_SEAM: the collapse of edge (a, b) the kinds allow, and its cost. Two interior vertices meet at the
    // quadric's best point (khld_place); otherwise the vertex that may move goes onto the other where it stands - the
    // cheaper way when both may. khc_keep_b = b survives. KH_LOD_REFUSED when no way is.
    auto khld_choose = [&](uint32_t khc_a, uint32_t khc_b, double khc_o[3], bool& khc_keep_b) -> double {
        khc_keep_b = false;
        const int khc_ka = khld_kind(khc_a), khc_kb = khld_kind(khc_b);
        if (khc_ka == 0 && khc_kb == 0) return khld_place(khc_a, khc_b, khc_o);
        const bool khc_path = khld_cedge(khc_a, khc_b);
        const bool khc_b_moves = khc_kb == 0 || (khc_kb == 1 && khc_path);   // b onto a.
        const bool khc_a_moves = khc_ka == 0 || (khc_ka == 1 && khc_path);   // a onto b.
        khlod::Q khc_q = khld_q[khc_a];
        khlod::q_add(khc_q, khld_q[khc_b]);
        double khc_best = KH_LOD_REFUSED;
        if (khc_b_moves) {
            khc_best = khlod::q_eval(khc_q, khld_pos[khc_a * 3], khld_pos[khc_a * 3 + 1], khld_pos[khc_a * 3 + 2]);
            for (int khc_k = 0; khc_k < 3; ++khc_k) khc_o[khc_k] = khld_pos[khc_a * 3 + khc_k];
        }
        if (khc_a_moves) {
            const double khc_e = khlod::q_eval(khc_q, khld_pos[khc_b * 3], khld_pos[khc_b * 3 + 1],
                                               khld_pos[khc_b * 3 + 2]);
            if (khc_e < khc_best) {
                khc_best = khc_e;
                for (int khc_k = 0; khc_k < 3; ++khc_k) khc_o[khc_k] = khld_pos[khc_b * 3 + khc_k];
                khc_keep_b = true;
            }
        }
        return khc_best;
    };
    auto khld_push = [&](uint32_t khs_a, uint32_t khs_b) {
        if (khs_a == khs_b || khld_dead[khs_a] || khld_dead[khs_b]) return;
        double khs_p[3];
        bool khs_kb = false;
        khlod::Cand khs_c;
        khs_c.cost = khld_choose(khs_a, khs_b, khs_p, khs_kb);   // KH_LOD_SEAM.
        if (khs_c.cost >= KH_LOD_REFUSED) return;
        khs_c.a = khs_a; khs_c.b = khs_b;
        khs_c.sa = khld_stamp[khs_a]; khs_c.sb = khld_stamp[khs_b];
        khld_pq2.push_back(khs_c);
        std::push_heap(khld_pq2.begin(), khld_pq2.end(), khld_worse);
    };

    for (const auto& khld_kv : khld_ecount) {
        khld_push(static_cast<uint32_t>(khld_kv.first & 0xFFFFFFFFull),
                  static_cast<uint32_t>(khld_kv.first >> 32));
    }

    size_t khld_live = khld_tri.size();
    uint64_t khld_ver = 1;
    // The pop bound stops a pathological mesh spinning (generous by design). One count for the whole run, as a run
    // to the last level would keep.
    size_t khld_budget = khld_tri.size() * 32 + 4096;

    // KH_LOD_PROGRESSIVE: the level machine. khld_l is the level being run (its target and cap below); khld_open
    // says one is.
    int khld_l = -1;
    bool khld_open = false, khld_ran = false;
    size_t khld_prev = khld_ntri, khld_target = 0;
    double khld_cost_cap = 0.0;
    // A level's triangles as the run stands: every live face, each corner with its own attributes at its vertex's
    // current position. KH_LOD_JOINT: khld_otag, when given, takes each emitted triangle's submesh. KH_LOD_SKIN:
    // khld_oinf, when given, each emitted vertex's influences.
    auto khld_emit = [&](std::vector<uint32_t>* khld_otag,
                         std::vector<KhSkinInf>* khld_oinf) -> std::vector<MeshVertex> {
        std::vector<MeshVertex> khld_out;
        khld_out.reserve(khld_live * 3);
        if (khld_otag) khld_otag->clear();
        if (khld_oinf) khld_oinf->clear();

        for (size_t khld_ot = 0; khld_ot < khld_tri.size(); ++khld_ot) {
            const khlod::Tri& khld_f = khld_tri[khld_ot];
            if (!khld_f.live) continue;
            if (khld_f.v[0] == khld_f.v[1] || khld_f.v[1] == khld_f.v[2] ||
                khld_f.v[0] == khld_f.v[2]) continue;
            double khld_n[3];
            khlod::tri_normal(&khld_pos[khld_f.v[0] * 3], &khld_pos[khld_f.v[1] * 3],
                              &khld_pos[khld_f.v[2] * 3], khld_n);
            if (khld_n[0] * khld_n[0] + khld_n[1] * khld_n[1] + khld_n[2] * khld_n[2] < 1.0e-24) continue;

            for (int khld_k = 0; khld_k < 3; ++khld_k) {
                MeshVertex khld_v = khld_wedge[khld_ot * 3 + khld_k];   // KH_LOD_TOPO: the corner's own
                                                                        // attributes.
                khld_v.pos[0] = static_cast<float>(khld_pos[khld_f.v[khld_k] * 3]);
                khld_v.pos[1] = static_cast<float>(khld_pos[khld_f.v[khld_k] * 3 + 1]);
                khld_v.pos[2] = static_cast<float>(khld_pos[khld_f.v[khld_k] * 3 + 2]);
                khld_out.push_back(khld_v);
                if (khld_oinf) khld_oinf->push_back(khld_if ? khld_vinf[khld_f.v[khld_k]] : KhSkinInf());
            }
            if (khld_otag) khld_otag->push_back(khld_ttag[khld_ot]);
        }

        if (khld_out.size() < 9) {
            if (khld_otag) *khld_otag = khld_src_tags;
            if (khld_oinf) *khld_oinf = khld_src_inf;
            return khld_src;
        }
        return khld_out;
    };
    // KH_LOD_TWO_PASS: a level's triangle count is its emission's, taken and dropped at once (a single level held).
    auto khld_count = [&]() -> size_t { return khld_emit(nullptr, nullptr).size() / 3; };
    // A level closes: materialised if kept, counted either way.
    auto khld_close = [&](int khld_cl) {
        size_t khld_c;
        if (khld_cl < khld_keep) {
            khld_outs[static_cast<size_t>(khld_cl)] =
                khld_emit(khld_tag_outs ? &(*khld_tag_outs)[static_cast<size_t>(khld_cl)] : nullptr,
                          khld_inf_outs ? &(*khld_inf_outs)[static_cast<size_t>(khld_cl)] : nullptr);
            khld_c = khld_outs[static_cast<size_t>(khld_cl)].size() / 3;
        } else {
            khld_c = khld_count();
        }
        khld_counts[static_cast<size_t>(khld_cl)] = khld_c;
        khld_prev = khld_c;
    };

    auto khld_level_open = [&]() {   // Opens the next level that runs, filling the ones that do not.
        khld_open = false;
        for (++khld_l; khld_l < khld_levels; ++khld_l) {
            size_t khld_lt = 0;
            double khld_lc = 0.0;
            khld_plan(khld_l, khld_prev, khld_lt, khld_lc);
            if (khld_lt >= khld_ntri) {
                if (khld_l < khld_keep) {
                    khld_outs[static_cast<size_t>(khld_l)] = khld_src;
                    if (khld_tag_outs) (*khld_tag_outs)[static_cast<size_t>(khld_l)] = khld_src_tags;
                    if (khld_inf_outs) (*khld_inf_outs)[static_cast<size_t>(khld_l)] = khld_src_inf;
                }
                khld_counts[static_cast<size_t>(khld_l)] = khld_ntri;
                khld_prev = khld_ntri;
                continue;
            }
            const bool khld_order = !khld_ran ||
                (khld_lt <= khld_target &&
                 (khld_cost_cap == 0.0 ? khld_lc == 0.0 : (khld_lc == 0.0 || khld_lc >= khld_cost_cap)));
            if (!khld_order) {   // Not a continuation of the run: the levels left repeat the last one.
                for (int khld_r = khld_l; khld_r < khld_levels; ++khld_r) {
                    khld_counts[static_cast<size_t>(khld_r)] = khld_counts[static_cast<size_t>(khld_l - 1)];
                    if (khld_r < khld_keep) {
                        khld_outs[static_cast<size_t>(khld_r)] = khld_outs[static_cast<size_t>(khld_l - 1)];
                        if (khld_tag_outs) {
                            (*khld_tag_outs)[static_cast<size_t>(khld_r)] =
                                (*khld_tag_outs)[static_cast<size_t>(khld_l - 1)];
                        }
                        if (khld_inf_outs) {   // KH_LOD_SKIN.
                            (*khld_inf_outs)[static_cast<size_t>(khld_r)] =
                                (*khld_inf_outs)[static_cast<size_t>(khld_l - 1)];
                        }
                    }
                }
                khld_l = khld_levels;
                return;
            }
            khld_target = khld_lt;
            khld_cost_cap = khld_lc;
            khld_ran = true;
            khld_open = true;
            return;
        }
    };
    // Whether the loop pops again: the level closes at its target, an empty heap, the pop bound, or a top over its
    // cap (KH_LOD_TOPO: the error budget is spent) - peeked, not popped and not counted, so the next level pops it on
    // the same count. The top is the heap's cheapest, so every candidate left is over the cap too; a stale top the
    // run would have popped first changes nothing, and the next level pops it in the same place.
    auto khld_more = [&]() -> bool {
        while (khld_open) {
            // The bound is spent without wrapping: the count carries into the next level.
            if (khld_live > khld_target && !khld_pq2.empty() && khld_budget > 0) {
                if (!(khld_cost_cap > 0.0 && khld_pq2.front().cost > khld_cost_cap)) {
                    --khld_budget;
                    return true;
                }
            }
            khld_close(khld_l);
            khld_level_open();
        }
        return false;
    };
    khld_level_open();

    while (khld_more()) {
        std::pop_heap(khld_pq2.begin(), khld_pq2.end(), khld_worse);
        const khlod::Cand khld_c = khld_pq2.back();
        khld_pq2.pop_back();
        if (khld_dead[khld_c.a] || khld_dead[khld_c.b]) continue;
        if (khld_stamp[khld_c.a] != khld_c.sa || khld_stamp[khld_c.b] != khld_c.sb) continue;
        double khld_p[3];
        bool khld_keep_b = false;
        // KH_LOD_SEAM: re-asked here - a neighbour's collapse can change a kind without touching this pair's stamps.
        if (khld_choose(khld_c.a, khld_c.b, khld_p, khld_keep_b) >= KH_LOD_REFUSED) continue;
        const uint32_t khld_a = khld_keep_b ? khld_c.b : khld_c.a;   // The survivor.
        const uint32_t khld_b = khld_keep_b ? khld_c.a : khld_c.b;   // Folds into it.

        // Flip rejection: a collapse that turns a triangle inside out is a visible fold the quadric cost cannot see.
        bool khld_flip = false;

        for (int khld_s = 0; khld_s < 2 && !khld_flip; ++khld_s) {
            const uint32_t khld_from = khld_s == 0 ? khld_a : khld_b;

            for (uint32_t khld_ti : khld_adj[khld_from]) {
                if (!khld_tri[khld_ti].live) continue;
                const uint32_t* khld_v = khld_tri[khld_ti].v;
                bool khld_has_a = false, khld_has_b = false;

                for (int khld_k = 0; khld_k < 3; ++khld_k) {
                    if (khld_v[khld_k] == khld_a) khld_has_a = true;
                    if (khld_v[khld_k] == khld_b) khld_has_b = true;
                }

                if (khld_has_a && khld_has_b) continue;   // This face dies anyway.
                double khld_old[3], khld_new[3];
                double khld_pp[3][3];

                for (int khld_k = 0; khld_k < 3; ++khld_k) {
                    khld_pp[khld_k][0] = khld_pos[khld_v[khld_k] * 3];
                    khld_pp[khld_k][1] = khld_pos[khld_v[khld_k] * 3 + 1];
                    khld_pp[khld_k][2] = khld_pos[khld_v[khld_k] * 3 + 2];
                }

                khlod::tri_normal(khld_pp[0], khld_pp[1], khld_pp[2], khld_old);

                for (int khld_k = 0; khld_k < 3; ++khld_k) {
                    if (khld_v[khld_k] != khld_a && khld_v[khld_k] != khld_b) continue;
                    khld_pp[khld_k][0] = khld_p[0];
                    khld_pp[khld_k][1] = khld_p[1];
                    khld_pp[khld_k][2] = khld_p[2];
                }

                khlod::tri_normal(khld_pp[0], khld_pp[1], khld_pp[2], khld_new);
                const double khld_dot = khld_old[0] * khld_new[0] + khld_old[1] * khld_new[1] +
                                        khld_old[2] * khld_new[2];
                const double khld_ol = sqrt(khld_old[0] * khld_old[0] + khld_old[1] * khld_old[1] + khld_old[2] * khld_old[2]);
                const double khld_nl = sqrt(khld_new[0] * khld_new[0] + khld_new[1] * khld_new[1] + khld_new[2] * khld_new[2]);
                if (khld_nl < 1.0e-12 || khld_dot <= 0.2 * khld_ol * khld_nl) { khld_flip = true; break; }
            }
        }

        if (khld_flip) continue;
        // KH_LOD_SEAM: the corners' attributes follow the collapse. Each dying face pairs a's corner with b's on its
        // side of any seam; every surviving corner at a or b on that side (the same uv as the pair's) takes the
        // pair's attributes at the new point - a's own where b moved onto a - and a crease corner (its normal apart
        // from the pair's) keeps its normal. A corner of b no dying face pairs would carry its uv across a seam:
        // refused.
        struct KhLdPair { MeshVertex wa, wb, wn; };
        KhLdPair khld_pr[4];
        int khld_npr = 0;
        bool khld_bad = false;
        double khld_t = 0.0;   // Where p lies from a to b.
        {
            const double khld_ab[3] = { khld_pos[khld_b * 3] - khld_pos[khld_a * 3],
                                        khld_pos[khld_b * 3 + 1] - khld_pos[khld_a * 3 + 1],
                                        khld_pos[khld_b * 3 + 2] - khld_pos[khld_a * 3 + 2] };
            const double khld_l2 = khld_ab[0] * khld_ab[0] + khld_ab[1] * khld_ab[1] + khld_ab[2] * khld_ab[2];
            if (khld_l2 > 1.0e-30) {
                khld_t = ((khld_p[0] - khld_pos[khld_a * 3]) * khld_ab[0] +
                          (khld_p[1] - khld_pos[khld_a * 3 + 1]) * khld_ab[1] +
                          (khld_p[2] - khld_pos[khld_a * 3 + 2]) * khld_ab[2]) / khld_l2;
                khld_t = khld_t < 0.0 ? 0.0 : (khld_t > 1.0 ? 1.0 : khld_t);
            }
        }
        auto khld_uv_eq = [](const MeshVertex& x, const MeshVertex& y) {   // KH_LOD_HAND: and one handedness.
            return fabsf(x.uv[0] - y.uv[0]) + fabsf(x.uv[1] - y.uv[1]) <= 1.0e-4f && x.tan[3] == y.tan[3];
        };
        auto khld_corner = [&](uint32_t khq_t, uint32_t khq_v) -> int {
            for (int khq_k = 0; khq_k < 3; ++khq_k) if (khld_tri[khq_t].v[khq_k] == khq_v) return khq_k;
            return -1;
        };
        for (uint32_t khld_ti : khld_adj[khld_b]) {   // The dying faces: the pairs.
            if (!khld_tri[khld_ti].live) continue;
            const int khld_ka = khld_corner(khld_ti, khld_a);
            if (khld_ka < 0) continue;
            if (khld_npr == 4) { khld_bad = true; break; }
            const int khld_kb = khld_corner(khld_ti, khld_b);
            KhLdPair& khld_q2 = khld_pr[khld_npr++];
            khld_q2.wa = khld_wedge[khld_ti * 3 + static_cast<uint32_t>(khld_ka)];
            khld_q2.wb = khld_wedge[khld_ti * 3 + static_cast<uint32_t>(khld_kb)];
            khld_q2.wn = khld_q2.wa;
            const float khld_tf = static_cast<float>(khld_t);
            const MeshVertex& khld_xa = khld_q2.wa;
            const MeshVertex& khld_xb = khld_q2.wb;
            MeshVertex& khld_xn = khld_q2.wn;
            for (int khld_k = 0; khld_k < 2; ++khld_k) {
                khld_xn.uv[khld_k] = khld_xa.uv[khld_k] + (khld_xb.uv[khld_k] - khld_xa.uv[khld_k]) * khld_tf;
            }
            float khld_nl = 0.0f, khld_tl = 0.0f;
            for (int khld_k = 0; khld_k < 3; ++khld_k) {
                khld_xn.nrm[khld_k] = khld_xa.nrm[khld_k] + (khld_xb.nrm[khld_k] - khld_xa.nrm[khld_k]) * khld_tf;
                khld_xn.tan[khld_k] = khld_xa.tan[khld_k] + (khld_xb.tan[khld_k] - khld_xa.tan[khld_k]) * khld_tf;
                khld_nl += khld_q2.wn.nrm[khld_k] * khld_q2.wn.nrm[khld_k];
                khld_tl += khld_q2.wn.tan[khld_k] * khld_q2.wn.tan[khld_k];
            }
            if (khld_nl > 1.0e-12f) {
                const float khld_ni = 1.0f / sqrtf(khld_nl);
                for (int khld_k = 0; khld_k < 3; ++khld_k) khld_q2.wn.nrm[khld_k] *= khld_ni;
            } else {
                for (int khld_k = 0; khld_k < 3; ++khld_k) khld_q2.wn.nrm[khld_k] = khld_q2.wa.nrm[khld_k];
            }
            if (khld_tl > 1.0e-12f) {
                const float khld_ti2 = 1.0f / sqrtf(khld_tl);
                for (int khld_k = 0; khld_k < 3; ++khld_k) khld_q2.wn.tan[khld_k] *= khld_ti2;
            } else {
                for (int khld_k = 0; khld_k < 3; ++khld_k) khld_q2.wn.tan[khld_k] = khld_q2.wa.tan[khld_k];
            }
        }
        if (khld_bad || khld_npr == 0) continue;
        auto khld_pair_of = [&](const MeshVertex& khw, bool khw_b) -> int {
            for (int khld_i = 0; khld_i < khld_npr; ++khld_i) {
                if (khld_uv_eq(khw, khw_b ? khld_pr[khld_i].wb : khld_pr[khld_i].wa)) return khld_i;
            }
            return -1;
        };
        for (uint32_t khld_ti : khld_adj[khld_b]) {   // Every corner of b a face keeps must pair.
            if (!khld_tri[khld_ti].live || khld_corner(khld_ti, khld_a) >= 0) continue;
            const int khld_kb = khld_corner(khld_ti, khld_b);
            if (khld_pair_of(khld_wedge[khld_ti * 3 + static_cast<uint32_t>(khld_kb)], true) < 0) {
                khld_bad = true;
                break;
            }
        }
        if (khld_bad) continue;
        auto khld_take = [&](MeshVertex& khw, const KhLdPair& khpr, bool khw_b) {
            const MeshVertex& khref = khw_b ? khpr.wb : khpr.wa;
            double khsn_w[3], khsn_r[3];   // KH_LOD_METRIC: the angle as drawn.
            khld_to_m(khw.nrm, khsn_w);
            khld_to_m(khref.nrm, khsn_r);
            // KH_LOD_METRIC: the hard-edge test's literal (0.985f), so the two tests are exact complements.
            const bool khsame_n = khsn_w[0] * khsn_r[0] + khsn_w[1] * khsn_r[1] + khsn_w[2] * khsn_r[2] >= 0.985f;
            khw.uv[0] = khpr.wn.uv[0];
            khw.uv[1] = khpr.wn.uv[1];
            for (int khld_k = 0; khld_k < 4; ++khld_k) khw.tan[khld_k] = khpr.wn.tan[khld_k];
            if (khsame_n) for (int khld_k = 0; khld_k < 3; ++khld_k) khw.nrm[khld_k] = khpr.wn.nrm[khld_k];
        };
        for (int khld_s = 0; khld_s < 2; ++khld_s) {   // The corners that stay, at a then at b.
            const uint32_t khld_at = khld_s == 0 ? khld_a : khld_b;
            if (khld_s == 0 && khld_t <= 0.0) continue;   // b moved onto a: a's corners keep theirs.
            for (uint32_t khld_ti : khld_adj[khld_at]) {
                if (!khld_tri[khld_ti].live) continue;
                if (khld_corner(khld_ti, khld_s == 0 ? khld_b : khld_a) >= 0) continue;   // Dies.
                const int khld_k = khld_corner(khld_ti, khld_at);
                MeshVertex& khld_w = khld_wedge[khld_ti * 3 + static_cast<uint32_t>(khld_k)];
                const int khld_i = khld_pair_of(khld_w, khld_s == 1);
                if (khld_i >= 0) khld_take(khld_w, khld_pr[khld_i], khld_s == 1);
            }
        }
        // Commit: b folds into a at khld_p.
        khld_pos[khld_a * 3] = khld_p[0];
        khld_pos[khld_a * 3 + 1] = khld_p[1];
        khld_pos[khld_a * 3 + 2] = khld_p[2];
        khlod::q_add(khld_q[khld_a], khld_q[khld_b]);
        // KH_LOD_SKIN: b's influences fold into a's at the uv's fraction.
        if (khld_if) khld_vinf[khld_a] = kh_lod_inf_blend(khld_vinf[khld_a], khld_vinf[khld_b], khld_t);
        khld_dead[khld_b] = 1;
        khld_stamp[khld_a] = ++khld_ver;
        khld_stamp[khld_b] = ++khld_ver;

        for (uint32_t khld_ti : khld_adj[khld_b]) {
            if (!khld_tri[khld_ti].live) continue;
            uint32_t* khld_v = khld_tri[khld_ti].v;
            bool khld_has_a = false;
            for (int khld_k = 0; khld_k < 3; ++khld_k) if (khld_v[khld_k] == khld_a) khld_has_a = true;

            if (khld_has_a) {
                khld_tri[khld_ti].live = false;
                khld_live--;
                continue;
            }

            for (int khld_k = 0; khld_k < 3; ++khld_k) if (khld_v[khld_k] == khld_b) khld_v[khld_k] = khld_a;
            khld_adj[khld_a].push_back(khld_ti);
        }

        khld_adj[khld_b].clear();
        // KH_LOD_SEAM: b's constrained edges become a's (the edge a - b is gone).
        for (const uint32_t khld_x : khld_cnb[khld_b]) {
            std::vector<uint32_t>& khld_xn = khld_cnb[khld_x];
            khld_xn.erase(std::remove(khld_xn.begin(), khld_xn.end(), khld_b), khld_xn.end());
            if (khld_x == khld_a || khld_cedge(khld_a, khld_x)) continue;
            khld_cnb[khld_a].push_back(khld_x);
            khld_xn.push_back(khld_a);
        }
        khld_cnb[khld_b].clear();
        // Re-price every edge still touching the surviving vertex a.
        std::vector<uint32_t> khld_nb;

        for (uint32_t khld_ti : khld_adj[khld_a]) {
            if (!khld_tri[khld_ti].live) continue;
            const uint32_t* khld_v = khld_tri[khld_ti].v;
            for (int khld_k = 0; khld_k < 3; ++khld_k) {
                if (khld_v[khld_k] != khld_a && !khld_dead[khld_v[khld_k]]) khld_nb.push_back(khld_v[khld_k]);
            }
        }

        std::sort(khld_nb.begin(), khld_nb.end());
        khld_nb.erase(std::unique(khld_nb.begin(), khld_nb.end()), khld_nb.end());
        for (uint32_t khld_n2 : khld_nb) khld_push(khld_a, khld_n2);
    }
}

// Build levels 1..KH_LOD_MAX onto a baked MeshDef. Idempotent; any level that refuses to shrink ends the ladder and
// lod_n names what exists. KH_LOD_FROM0: every level is decimated from level 0, to half the previous level's count
// under that level's budget - all of the mesh's levels in one run (KH_LOD_PROGRESSIVE, kh_lod_decimate). Do not
// chain levels (decimate level l from level l - 1): a collapsed vertex's triangles keep their source corners'
// attributes, so a level's output reads as seams all over to the next (2 % -> 28 % -> 57 % of a 16k sphere's edges)
// and the ladder locks after a level or two.
// KH_LOD_SKIN: khlb_skin, when given with khlb_skin_out, is the import's influences by position (kh_fbx_import's
// khsk_map; kh_lod_skin_key): each source vertex's seeds the run, and khlb_skin_out takes the influences of every
// vertex a kept level holds at a point khlb_skin does not know - one the decimator placed anew, or a source point the
// import gave no entry (its seed: none) - keyed the same way; the first at a point stands.
inline void kh_lod_build(MeshDef& khlb_d,
                         const std::unordered_map<KhClothPosKey, KhSkinInf, KhClothPosHash>* khlb_skin = nullptr,
                         std::unordered_map<KhClothPosKey, KhSkinInf, KhClothPosHash>* khlb_skin_out = nullptr) {
    KH_PROF_SCOPE(KHP_LOD_BUILD);   // KH_PROF.
    if (khlb_d.lod_n) return;
    if (khlb_d.submeshes.empty() || khlb_d.verts.empty()) return;
    if (!khlb_d.indices.empty()) return;   // Expanded form only (kh_mesh_weld runs after).
    const uint32_t khlb_base = static_cast<uint32_t>(khlb_d.verts.size());
    // The ladder halves the triangle count per level, which is why the screen-size selector is a log2.
    // KH_LOD_JOINT: every submesh in ONE soup, one after another, each triangle tagged with its submesh. Decimated
    // apart, a border two submeshes share is thinned twice, differently - two polylines, and a crack between the
    // materials; in one run its two sides are one set of vertices, constrained as a uv seam is.
    std::vector<MeshVertex> khlb_cur;
    std::vector<uint32_t> khlb_tag;   // Per triangle of khlb_cur: its submesh.
    for (size_t khlb_s = 0; khlb_s < khlb_d.submeshes.size(); ++khlb_s) {
        const MeshSubmesh& khlb_sm = khlb_d.submeshes[khlb_s];
        const uint32_t khlb_n = khlb_sm.index_count - khlb_sm.index_count % 3u;   // Whole triangles.
        khlb_cur.insert(khlb_cur.end(), khlb_d.verts.begin() + khlb_sm.index_start,
                        khlb_d.verts.begin() + khlb_sm.index_start + khlb_n);
        khlb_tag.insert(khlb_tag.end(), khlb_n / 3u, static_cast<uint32_t>(khlb_s));
    }
    // KH_LOD_SKIN: each source vertex's influences, by its position (none where the import has none).
    const bool khlb_sk = khlb_skin != nullptr && khlb_skin_out != nullptr;
    std::vector<KhSkinInf> khlb_inf;
    if (khlb_sk) {
        khlb_inf.resize(khlb_cur.size());
        for (size_t khlb_i = 0; khlb_i < khlb_cur.size(); ++khlb_i) {
            const auto khlb_it = khlb_skin->find(kh_lod_skin_key(khlb_cur[khlb_i]));
            if (khlb_it != khlb_skin->end()) khlb_inf[khlb_i] = khlb_it->second;
        }
    }

    // The error budget: quadric cost is area x distance squared, so the budget lets a level move a vertex by tol_l
    // against six mean faces (the previous level's mean face: level 0's quadrics accumulate as its faces merge). A
    // level the budget stops short of 90 % of the previous one ends the ladder.
    double khlb_area = 0.0, khlb_diag = 0.0;
    {
        const std::vector<MeshVertex>& khlb_v = khlb_cur;
        double khlb_mn[3] = { 1.0e300, 1.0e300, 1.0e300 }, khlb_mx[3] = { -1.0e300, -1.0e300, -1.0e300 };
        for (const MeshVertex& khlb_p : khlb_v) {
            for (int khlb_k = 0; khlb_k < 3; ++khlb_k) {
                if (khlb_p.pos[khlb_k] < khlb_mn[khlb_k]) khlb_mn[khlb_k] = khlb_p.pos[khlb_k];
                if (khlb_p.pos[khlb_k] > khlb_mx[khlb_k]) khlb_mx[khlb_k] = khlb_p.pos[khlb_k];
            }
        }
        for (size_t khlb_t = 0; khlb_t + 2 < khlb_v.size(); khlb_t += 3) {
            const double khlb_p0[3] = { khlb_v[khlb_t].pos[0], khlb_v[khlb_t].pos[1], khlb_v[khlb_t].pos[2] };
            const double khlb_p1[3] = { khlb_v[khlb_t + 1].pos[0], khlb_v[khlb_t + 1].pos[1], khlb_v[khlb_t + 1].pos[2] };
            const double khlb_p2[3] = { khlb_v[khlb_t + 2].pos[0], khlb_v[khlb_t + 2].pos[1], khlb_v[khlb_t + 2].pos[2] };
            double khlb_n[3];
            khlod::tri_normal(khlb_p0, khlb_p1, khlb_p2, khlb_n);
            khlb_area += 0.5 * sqrt(khlb_n[0] * khlb_n[0] + khlb_n[1] * khlb_n[1] + khlb_n[2] * khlb_n[2]);
        }
        if (!khlb_v.empty()) {
            const double khlb_dx = khlb_mx[0] - khlb_mn[0], khlb_dy = khlb_mx[1] - khlb_mn[1], khlb_dz = khlb_mx[2] - khlb_mn[2];
            khlb_diag = sqrt(khlb_dx * khlb_dx + khlb_dy * khlb_dy + khlb_dz * khlb_dz);
        }
    }
    // KH_LOD_PROGRESSIVE: every level from level 0, in one run. Level l wants half of level l - 1 (the floor under
    // that) and spends at most its budget; both follow the previous level's count, so the targets fall and the caps
    // rise, which is what lets one run stand for a run per level. KH_LOD_TWO_PASS: pass 1 counts every level, the
    // 90 % check below picks how many are kept from the counts, and pass 2 materialises those alone - a stalling
    // mesh never holds a dozen copies of itself.
    std::vector<std::vector<MeshVertex>> khlb_lvls;
    std::vector<std::vector<uint32_t>> khlb_tags;
    std::vector<std::vector<KhSkinInf>> khlb_infs;   // KH_LOD_SKIN.
    std::vector<size_t> khlb_cnt;
    auto khlb_plan = [&](int khlb_l, size_t khlb_tris, size_t& khlb_want, double& khlb_cap) {
        khlb_want = khlb_tris / 2 < KH_LOD_FLOOR_TRIS ? KH_LOD_FLOOR_TRIS : khlb_tris / 2;
        khlb_cap = 0.0;
        if (khlb_tris > 0 && khlb_area > 0.0 && khlb_diag > 0.0) {
            const double khlb_mean_a = khlb_area / static_cast<double>(khlb_tris);
            const double khlb_edge = 1.52 * sqrt(khlb_mean_a);   // Equilateral side for that area.
            const double khlb_lv = static_cast<double>(1u << khlb_l);
            const double khlb_tol = std::min(0.5 * khlb_lv * khlb_edge, 0.02 * khlb_lv * khlb_diag);
            khlb_cap = khlb_tol * khlb_tol * khlb_mean_a * 6.0;
        }
    };
    double khlb_ms[3];   // KH_LOD_METRIC: the frame the run judges normals in.
    khlod::metric(khlb_d.native_ext, khlb_ms);
    kh_lod_decimate(khlb_cur, KH_LOD_MAX, 0, khlb_plan, khlb_lvls, khlb_cnt, &khlb_tag,   // Pass 1: counts alone.
                    nullptr, nullptr, nullptr, khlb_ms);
    // The levels kept, from the counts: a level that did not shrink below 90 % of the one before is not worth a draw
    // range, a cache entry or a transition, and ends the ladder.
    int khlb_keep = 0;
    for (; khlb_keep < KH_LOD_MAX; ++khlb_keep) {
        const size_t khlb_bt = khlb_keep == 0 ? khlb_cur.size() / 3 : khlb_cnt[static_cast<size_t>(khlb_keep - 1)];
        const size_t khlb_at = khlb_cnt[static_cast<size_t>(khlb_keep)];
        if (khlb_bt == 0 || khlb_at * 100 > khlb_bt * 90) break;
    }
    if (khlb_keep == 0) return;
    kh_lod_decimate(khlb_cur, khlb_keep, khlb_keep, khlb_plan, khlb_lvls, khlb_cnt, &khlb_tag, &khlb_tags,   // Pass 2.
                    khlb_sk ? &khlb_inf : nullptr, khlb_sk ? &khlb_infs : nullptr, khlb_ms);
    for (int khlb_l = 0; khlb_l < khlb_keep; ++khlb_l) {
        // The level back into its submeshes, in submesh order: a triangle keeps its submesh through every collapse,
        // and a submesh may end a level empty (a small material thinned away - its range draws nothing).
        std::vector<std::vector<MeshVertex>> khlb_next(khlb_d.submeshes.size());
        {
            const std::vector<MeshVertex>& khlb_lv = khlb_lvls[static_cast<size_t>(khlb_l)];
            const std::vector<uint32_t>& khlb_lt = khlb_tags[static_cast<size_t>(khlb_l)];
            for (size_t khlb_t = 0; khlb_t < khlb_lt.size() && khlb_t * 3 + 2 < khlb_lv.size(); ++khlb_t) {
                std::vector<MeshVertex>& khlb_o = khlb_next[khlb_lt[khlb_t]];
                khlb_o.insert(khlb_o.end(), khlb_lv.begin() + static_cast<std::ptrdiff_t>(khlb_t * 3),
                              khlb_lv.begin() + static_cast<std::ptrdiff_t>(khlb_t * 3 + 3));
            }
            if (khlb_sk) {   // KH_LOD_SKIN: the points this level placed anew.
                const std::vector<KhSkinInf>& khlb_li = khlb_infs[static_cast<size_t>(khlb_l)];
                for (size_t khlb_i = 0; khlb_i < khlb_lv.size() && khlb_i < khlb_li.size(); ++khlb_i) {
                    const KhClothPosKey khlb_k = kh_lod_skin_key(khlb_lv[khlb_i]);
                    if (khlb_skin->find(khlb_k) == khlb_skin->end()) khlb_skin_out->emplace(khlb_k, khlb_li[khlb_i]);
                }
                std::vector<KhSkinInf>().swap(khlb_infs[static_cast<size_t>(khlb_l)]);
            }
        }
        std::vector<MeshVertex>().swap(khlb_lvls[static_cast<size_t>(khlb_l)]);   // One level held at a time.
        const uint32_t khlb_start = static_cast<uint32_t>(khlb_d.verts.size());
        khlb_d.lod_istart[khlb_l] = khlb_start;
        khlb_d.lod_sub[khlb_l].resize(khlb_d.submeshes.size());

        for (size_t khlb_s = 0; khlb_s < khlb_next.size(); ++khlb_s) {
            MeshSubmesh khlb_sm;
            khlb_sm.name = khlb_d.submeshes[khlb_s].name;
            khlb_sm.index_start = static_cast<uint32_t>(khlb_d.verts.size());
            khlb_sm.index_count = static_cast<uint32_t>(khlb_next[khlb_s].size());
            khlb_d.verts.insert(khlb_d.verts.end(), khlb_next[khlb_s].begin(), khlb_next[khlb_s].end());
            khlb_d.lod_sub[khlb_l][khlb_s] = std::move(khlb_sm);
        }

        khlb_d.lod_icount[khlb_l] = static_cast<uint32_t>(khlb_d.verts.size()) - khlb_start;
        khlb_d.lod_n = static_cast<uint8_t>(khlb_l + 1);
    }

    // Level 0 runs from index 0 to level 1's lod_istart (mesh_base_icount); with nothing built, the whole stream.
    if (khlb_d.lod_n == 0) return;
    if (khlb_d.lod_istart[0] != khlb_base) khlb_d.lod_n = 0;   // Unreachable; a guard (nothing asserts it).
}

// LOD selection: screen size first, range second. khl_t comes back in the open
// interval - both endpoints mean "not fading" to the shader.
static constexpr float KH_LOD_REF_PX    = 320.0f;   // Half-height px at khl_f 0; level 0 ends at half this or sooner.
static constexpr float KH_LOD_DIST_BIAS = 0.35f;   // Levels per log2 of the range term.
static constexpr float KH_LOD_DIST_REF  = 100.0f;
static constexpr float KH_LOD_FADE      = 0.30f;   // Fraction of a level spent crossfading.
// KH_LOD_ZOOM: the unzoomed view's P11 (projection[1][1], the vertical half-FOV's cotangent) - the game's default
// fovTop of 0.75 - against which kh_lod_zoom_of measures a zoom. An error of a factor k here moves an unzoomed
// view's range term by at most KH_LOD_DIST_BIAS * log2(k) levels (0.09 at k = 1.2).
static constexpr float KH_LOD_P11_REF   = 1.0f / 0.75f;

// KH_LOD_ZOOM: a projection's zoom for the level pick's range term - P11 over the unzoomed view's, never below 1 (a
// view wider than the reference keeps the range term as it was), capped (an absurd P11 stays finite), 1 if unusable.
inline float kh_lod_zoom_of(float khlz_p11) {
    const float khlz_z = khlz_p11 / KH_LOD_P11_REF;
    if (!(khlz_z > 1.0f)) return 1.0f;   // NaN-safe.
    return khlz_z < 1.0e4f ? khlz_z : 1.0e4f;
}

// KH_LOD_ONE_SCALE: the main view's level-pick scale, the one definition both publishers (the injection, the flush)
// use: pixels per unit tangent = P11 * H / 2, H the height of the viewport the pass draws our meshes with - the
// viewport transform maps NDC y [-1, 1] onto Height pixels, so radius * that / distance is the mesh's projected
// radius in the pass's own pixels. Not the adopted main depth texture's height: the largest seen, it may exceed the
// scene viewport, and the two publishes of one frame then disagree (the shadow builds' kh_drawn_lod takes whichever
// came last). With it, g_lod_zoom (the same projection's). False, nothing written, for an unusable pair.
inline bool kh_lod_publish(float khlp_p11, float khlp_vp_h) {
    if (!(khlp_vp_h > 0.0f) || !(khlp_p11 > 0.0f)) return false;
    g_lod_proj_px = khlp_p11 * khlp_vp_h * 0.5f;
    g_lod_zoom = kh_lod_zoom_of(khlp_p11);
    return true;
}
// KH_LOD_PARK_KEEP: the injection's last successful publish - its pass viewport height (pixels) and the cycle
// (g_topo_cycles) it ran in; ~0 = none yet. Written by the injection, read by flush_locked (the render thread, or a
// park with it stalled). A park's flush runs under whatever viewport the stall left bound (the view model's slice,
// none), so it publishes only at the injection's height, or while the injection is not publishing (the park's
// draws are then the picture and it is the only publisher); otherwise the injection's value stands for the next
// cycle's maps, which pick their casters' levels before that cycle's injection publishes.
static float    g_lod_inj_h = 0.0f;
static uint64_t g_lod_inj_cycle = ~0ull;

// KH_PIP_LOD: khl_proj_px, when given (>= 0), is the pass's own pixels-per-tangent in place of g_lod_proj_px.
// KH_LOD_ZOOM: khl_zoom, when given (>= 0; a pass passing its own khl_proj_px passes it too), is the pass's own
// kh_lod_zoom_of in place of g_lod_zoom. The range term takes the distance divided by the zoom - the distance at
// which the mesh would look this size unzoomed - so a view through optics picks the level the same on-screen size
// picks unzoomed (it took the world distance: a 4x view at 400 m drew 0.46 levels coarser than the bare view at
// 100 m of the same picture, 10x at 1000 m 0.86); unzoomed (zoom 1) the pick is unchanged.
inline void kh_lod_pick(const MeshDef& khl_d, float khl_radius, float khl_dist,
                        int& khl_lvl, float& khl_t, float khl_proj_px = -1.0f, float khl_zoom = -1.0f) {
    khl_lvl = 0;
    khl_t = 0.0f;
    if (khl_d.lod_n == 0) return;
    const float khl_pp = khl_proj_px >= 0.0f ? khl_proj_px : g_lod_proj_px;
    if (!(khl_pp > 0.0f) || !(khl_radius > 0.0f)) return;
    const float khl_zr = khl_zoom >= 0.0f ? khl_zoom : g_lod_zoom;
    const float khl_z = khl_zr > 1.0f ? khl_zr : 1.0f;   // KH_LOD_ZOOM (NaN-safe).
    const float khl_px = khl_radius * khl_pp / (khl_dist > 0.01f ? khl_dist : 0.01f);
    float khl_f = log2f(KH_LOD_REF_PX / (khl_px > 0.01f ? khl_px : 0.01f));
    khl_f += KH_LOD_DIST_BIAS * log2f(1.0f + (khl_dist > 0.0f ? khl_dist : 0.0f) / (KH_LOD_DIST_REF * khl_z));
    if (!(khl_f > 0.0f)) return;   // NaN-safe: an insane pair reads as level 0.
    const float khl_top = static_cast<float>(khl_d.lod_n);

    if (khl_f >= khl_top) {
        khl_lvl = static_cast<int>(khl_d.lod_n);
        return;
    }

    const float khl_base = floorf(khl_f);
    khl_lvl = static_cast<int>(khl_base);
    const float khl_frac = khl_f - khl_base;
    if (khl_frac <= 1.0f - KH_LOD_FADE) return;
    float khl_p = (khl_frac - (1.0f - KH_LOD_FADE)) / KH_LOD_FADE;
    if (khl_p < 0.02f) return;   // Not yet worth a second draw.
    if (khl_p > 0.98f) khl_p = 0.98f;   // The coarse level never draws alone here.
    khl_t = khl_p;
}

// The dither value blendCtl.w takes for one half of a crossfade pair: khl_second false = the finer level fading
// out, true = the coarser fading in. Complementary at every pixel.
inline float kh_lod_dither(float khl_t, bool khl_second) {
    if (!(khl_t > 0.0f)) return 0.0f;
    const float khl_v = 1.0f - khl_t;
    return khl_second ? -khl_v : khl_v;
}

// The object's bounding-sphere radius in world metres, from the rotated half extents every draw path computes for
// its own culling.
inline float kh_lod_radius_of(const float* size, const float* rot_m, bool rotated) {
    const float khl_hl[3] = { size[0] * 0.5f, size[2] * 0.5f, size[1] * 0.5f };
    float khl_he[3];
    kh_rot_half_extents(khl_hl, rot_m, rotated, khl_he);
    return sqrtf(khl_he[0] * khl_he[0] + khl_he[1] * khl_he[1] + khl_he[2] * khl_he[2]);
}

// KH_SKEL_LOD_R: the LOD pick's radius. A skeletal object's size is its pose's box (the skin owns it), which
// breathes with the animation - arms out, arms in - so its pick takes the REST box, the mesh's native_size: the
// radius does not move with the pose (the distance is still to the pose's box, which a limb moves by its own reach
// alone). Every pick reads this - the colour loops and the PIP directly, the sun / DLS / seam records from their
// builds (kh_drawn_lod), the dlsw mask from its record's copy (kh_dlsw_mask_render) - so colour and shadows agree.
inline float kh_lod_radius(const RenderObject& khl_o) {
    if (khl_o.skel) {
        return kh_lod_radius_of(mesh_def(mesh_id_clamp(khl_o.mesh)).native_size, khl_o.rot_m, khl_o.rotated);
    }
    return kh_lod_radius_of(khl_o.size, khl_o.rot_m, khl_o.rotated);
}

// Every depth-only pass that re-draws the scene's meshes (sun tiers, alpha casters, DLS maps, seam footprint, mirror
// prepass, prime) draws the level the colour pass picked for this object at this camera - the finer half of a fade,
// 0 under a lock - so a map holds the silhouette the player sees and every run groups on (mesh, level).
// g_lod_proj_px unpublished reads as level 0. khdl_r is the record's kh_lod_radius, taken at its build (KH_SKEL_LOD_R).
inline int kh_drawn_lod(int khdl_mid, float khdl_r, const float* khdl_pos, const float* khdl_size,
                        const float* khdl_rot, bool khdl_rotated, bool khdl_lock, const float khdl_cam[3]) {
    if (khdl_lock) return 0;
    const MeshDef& khdl_md = mesh_def(khdl_mid);
    if (khdl_md.lod_n == 0) return 0;
    int   khdl_lvl = 0;
    float khdl_t = 0.0f;
    kh_lod_pick(khdl_md, khdl_r,
                sqrtf(kh_mesh_dist_sq_of(khdl_pos, khdl_size, khdl_rot, khdl_rotated, khdl_cam)),
                khdl_lvl, khdl_t);
    return mesh_lod_clamp(khdl_md, khdl_lvl);
}
inline void mesh_lod_range_of(int khlr_mid, int khlr_lvl, UINT& khlr_s, UINT& khlr_c) {
    mesh_lod_range(mesh_def(khlr_mid), khlr_lvl, khlr_s, khlr_c);
}
}