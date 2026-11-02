#pragma once

// The feature half (rendering_integration_features.hpp): the draw ordering: the visibility and distance memos, the
// order keys and the fusion. Never include it on its own.
namespace RenderIntegration {

inline float kh_fx_time(const RenderObject& khft_o, double khft_now_d);   // Defined with the effect clock
                                                                           // (rendering/effect_clock.hpp).

// Append rules: followers must be builtin point ops from {1 invert, 2 colorgrade, 3 vignette, 5 grain}, never
// localized or banded, and carry no textures (KH_FX_TEX_ALL, kh_fuse_appendable) - point ops that are never
// spill-classified, never depth consumers, never t19 binders.
static constexpr int KH_FUSE_MAX = 3;   // Extra stages riding one draw.

// The pass visibility predicate is evaluated three times per object per pass (kh_mat_ensure_all, kh_inst_plan_emit,
// the per-object loop) with identical arguments; memoised for the pass. An object's index is its offset in the
// pass's contiguous mesh vector; out-of-range falls through to a live evaluation, so the memo is only ever a cache.
struct KhVisMemo {
    const RenderObject* base = nullptr;
    size_t n = 0;
    std::vector<uint8_t> mark;   // 0 unknown, 1 visible, 2 hidden.
    void begin(const std::vector<RenderObject>& khm_v) {
        base = khm_v.data();
        n = khm_v.size();
        mark.assign(n, 0u);
    }
    // Returns the cached verdict, or computes it through khm_f and caches it.
    template <class KhVisFn>
    bool get(const RenderObject& khm_o, KhVisFn khm_f) {
        // The null test precedes the subtraction: &khm_o - nullptr is undefined.
        if (!base) return khm_f(khm_o);
        const size_t khm_i = static_cast<size_t>(&khm_o - base);
        if (khm_i >= n) return khm_f(khm_o);   // Not a member: evaluate live.
        const uint8_t khm_m = mark[khm_i];
        if (khm_m != 0u) return khm_m == 1u;
        const bool khm_r = khm_f(khm_o);
        mark[khm_i] = khm_r ? 1u : 2u;
        return khm_r;
    }
};
static KhVisMemo g_vis_memo_inj;    // Render thread (the injection, the PIP inject).
static KhVisMemo g_vis_memo_flush;  // The flush (the render thread's scene flush, or the game thread under a park).

// The distance twin: kh_mesh_dist_sq (to the world-axis box enclosing the object's rotated size box - not an OBB
// distance) is asked per object by the route verdicts, the emit, the LOD pick and the translucent sort with the same
// camera for the whole pass. Same membership rule and cache-only contract as KhVisMemo. kh_mesh_order begins it
// after its permutation (a sort moves the objects) and seeds it with the sort's own distances. KH_NEARZ_BOUNDS:
// get_drawn is kh_bounds_dist_sq under the same rule (the near-z route's).
struct KhDistMemo {
    const RenderObject* base = nullptr;
    size_t n = 0;
    std::vector<float> d2;   // < 0 = unknown.
    std::vector<float> d2b;  // KH_NEARZ_BOUNDS: the drawn box's; < 0 = unknown.
    void begin(const std::vector<RenderObject>& khm_v) {
        base = khm_v.data();
        n = khm_v.size();
        d2.assign(n, -1.0f);
        d2b.assign(n, -1.0f);
    }
    float get_drawn(const RenderObject& khm_o, const float khm_cam[3]) {
        if (!base) return kh_bounds_dist_sq(khm_o, khm_cam);
        const size_t khm_i = static_cast<size_t>(&khm_o - base);
        if (khm_i >= n) return kh_bounds_dist_sq(khm_o, khm_cam);   // Not a member: evaluate live.
        if (d2b[khm_i] >= 0.0f) return d2b[khm_i];
        d2b[khm_i] = kh_bounds_dist_sq(khm_o, khm_cam);
        return d2b[khm_i];
    }
    float get(const RenderObject& khm_o, const float khm_cam[3]) {
        if (!base) return kh_mesh_dist_sq(khm_o, khm_cam);
        const size_t khm_i = static_cast<size_t>(&khm_o - base);
        if (khm_i >= n) return kh_mesh_dist_sq(khm_o, khm_cam);   // Not a member: evaluate live.
        if (d2[khm_i] >= 0.0f) return d2[khm_i];
        d2[khm_i] = kh_mesh_dist_sq(khm_o, khm_cam);
        return d2[khm_i];
    }
};
static KhDistMemo g_dist_memo_inj;    // Render thread (the injection, the PIP inject).
static KhDistMemo g_dist_memo_flush;  // The flush (the render thread's scene flush, or the game thread under a park).

// The order the colour passes draw in (the injection, the PIP inject, the flush): by group (opaque solids, then
// translucent and effect meshes, then depth-Off overlays), the opaque solids front to back (early depth rejection),
// the rest back to front. Sorts a 12-byte key (one distance per object) and permutes once, rather than sorting whole
// (~500-byte) objects with the distance recomputed in the comparator.
struct KhOrdKey {
    float    d;   // kh_mesh_dist_sq to the pass camera.
    uint32_t g;   // 0 opaque solid, 1 translucent/effect, 2 depth-Off.
    uint32_t i;   // Index into the mesh vector.
};
inline uint32_t kh_ord_group(const RenderObject& kho_o) {
    return kho_o.mode == DepthMode::Off ? 2u
         : (!kho_o.fullscreen && kho_o.effect == 0 && kho_o.blend_mode == 0 &&
            kho_o.color[3] >= 0.999f) ? 0u : 1u;
}
// KH_ORD_TIE: the distance as the sort compares it - a non-finite one (inf, NaN: a non-finite position or size)
// as the largest finite float, so every key is ordered. By the bits: a NaN test may be folded by fast-math.
inline float kh_ord_dist(float khod_d) {
    uint32_t khod_u;
    memcpy(&khod_u, &khod_d, sizeof(khod_u));
    return (khod_u & 0x7F800000u) == 0x7F800000u ? FLT_MAX : khod_d;
}
// Scratch, reused across frames and all three passes (never concurrently).
static std::vector<KhOrdKey>      g_ord_keys;
static std::vector<RenderObject>  g_ord_tmp;
// kho_dm is the pass's distance memo: begun here, after the permutation (the memo keys on addresses inside the
// vector), and seeded with the sort's own distances in the permuted order, so no consumer pays the distance twice.
inline void kh_mesh_order(std::vector<RenderObject>& kho_m, const float kho_cam[3], KhDistMemo& kho_dm) {
    const size_t kho_n = kho_m.size();
    if (kho_n < 2) {
        kho_dm.begin(kho_m);
        if (kho_n == 1) kho_dm.d2[0] = kh_mesh_dist_sq(kho_m[0], kho_cam);
        return;
    }
    g_ord_keys.clear();
    g_ord_keys.reserve(kho_n);

    for (uint32_t kho_i = 0; kho_i < static_cast<uint32_t>(kho_n); ++kho_i) {
        KhOrdKey kho_k;
        kho_k.d = kh_mesh_dist_sq(kho_m[kho_i], kho_cam);   // Once per object, not per compare.
        kho_k.g = kh_ord_group(kho_m[kho_i]);
        kho_k.i = kho_i;
        g_ord_keys.push_back(kho_k);
    }

    // KH_ORD_TIE: a total order. Equal distances (every box that holds the camera reads 0, so a translucent dome and
    // the glass or haze inside it tie) break on creation order (seq): std::sort is not stable, and past its
    // insertion-sort size a tied pair's order follows the pivots, i.e. every other object's distance, so without the
    // seq tie the two layers would swap from frame to frame. The translucent tails' stable_sort keeps the order given
    // here. A non-finite distance compares as the largest (kh_ord_dist), so the comparator stays a strict weak order
    // on any input; the memo keeps the distance as computed.
    std::sort(g_ord_keys.begin(), g_ord_keys.end(),
              [&kho_m](const KhOrdKey& kho_a, const KhOrdKey& kho_b) {
                  if (kho_a.g != kho_b.g) return kho_a.g < kho_b.g;
                  const float kho_da = kh_ord_dist(kho_a.d), kho_db = kh_ord_dist(kho_b.d);
                  if (kho_da != kho_db) {
                      if (kho_a.g != 0u) return kho_da > kho_db;   // Translucent: back to front.
                      return kho_da < kho_db;                      // Opaque: front to back.
                  }
                  const RenderObject& kho_oa = kho_m[kho_a.i];
                  const RenderObject& kho_ob = kho_m[kho_b.i];
                  if (kho_oa.seq != kho_ob.seq) return kho_oa.seq < kho_ob.seq;   // KH_ORD_TIE.
                  // KH_PART_ORDER: the flush's two part copies of one object tie on all of the above; its solid
                  // texels (draw_part 2) draw before its glass (draw_part 1) where both sort into the tail.
                  return kho_oa.draw_part != 1 && kho_ob.draw_part == 1;
              });
    g_ord_tmp.clear();
    g_ord_tmp.reserve(kho_n);
    for (const KhOrdKey& kho_k : g_ord_keys) g_ord_tmp.push_back(std::move(kho_m[kho_k.i]));
    kho_m.swap(g_ord_tmp);
    kho_dm.begin(kho_m);
    for (size_t kho_i = 0; kho_i < kho_n; ++kho_i) kho_dm.d2[kho_i] = g_ord_keys[kho_i].d;
}

// KH_FX_TEX_ALL: a pass with textures is never appended - a fused stage rides the head's draw, whose six texture
// slots are the head's.
inline bool kh_fuse_appendable(const RenderObject& o) {
    return (o.effect == 1 || o.effect == 2 || o.effect == 3 ||
            o.effect == 5) && !o.localized && !o.banded && !o.fx_tex;
}
// KH_FUSE_CLOCK: khfu_now_d is the flush's clock, the one its fills form fxMeta.y from - the stage's age in [4n].z
// is then the very float its pass takes unfused (KhFuseTail reads it for the grain's frame).
inline bool kh_fuse_append(ConstantData& cbd, const RenderObject& o, double khfu_now_d) {
    const int khfu_n = static_cast<int>(cbd.fuse_meta[0] + 0.5f);
    if (khfu_n >= KH_FUSE_MAX) return false;
    float* khfu_st = cbd.fuse_stage[khfu_n * 4];
    khfu_st[0] = static_cast<float>(o.effect);
    khfu_st[1] = static_cast<float>(o.blend_mode);
    khfu_st[2] = kh_fx_time(o, khfu_now_d);
    khfu_st[3] = 0.0f;
    memcpy(cbd.fuse_stage[khfu_n * 4 + 1], o.fx,     4 * sizeof(float));
    memcpy(cbd.fuse_stage[khfu_n * 4 + 2], o.fx + 4, 4 * sizeof(float));
    memcpy(cbd.fuse_stage[khfu_n * 4 + 3], o.color,  4 * sizeof(float));
    cbd.fuse_meta[0] = static_cast<float>(khfu_n + 1);
    return true;
}
}