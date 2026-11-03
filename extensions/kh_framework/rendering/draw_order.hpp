#pragma once

// The feature half (rendering_integration_features.hpp): the draw ordering: the visibility and distance memos, the
// order keys and the fusion. Never include it on its own.
namespace RenderIntegration {

inline float fx_time(const RenderObject& o, double now_d);   // Defined with the effect clock
                                                                           // (rendering/effect_clock.hpp).

// Append rules: followers must be builtin point ops from {1 invert, 2 colorgrade, 3 vignette, 5 grain}, never
// localized or banded, and carry no textures (FX_TEX_ALL, fuse_appendable) - point ops that are never
// spill-classified, never depth consumers, never t19 binders.
static constexpr int FUSE_MAX = 3;   // Extra stages riding one draw.

// The pass visibility predicate is evaluated three times per object per pass (mat_ensure_all, inst_plan_emit,
// the per-object loop) with identical arguments; memoised for the pass. An object's index is its offset in the
// pass's contiguous mesh vector; out-of-range falls through to a live evaluation, so the memo is only ever a cache.
struct VisMemo {
    const RenderObject* base = nullptr;
    size_t n = 0;
    std::vector<uint8_t> mark;   // 0 unknown, 1 visible, 2 hidden.
    void begin(const std::vector<RenderObject>& v) {
        base = v.data();
        n = v.size();
        mark.assign(n, 0u);
    }
    // Returns the cached verdict, or computes it through f and caches it.
    template <class VisFn>
    bool get(const RenderObject& o, VisFn f) {
        // The null test precedes the subtraction: &o - nullptr is undefined.
        if (!base) return f(o);
        const size_t i = static_cast<size_t>(&o - base);
        if (i >= n) return f(o);   // Not a member: evaluate live.
        const uint8_t m = mark[i];
        if (m != 0u) return m == 1u;
        const bool r = f(o);
        mark[i] = r ? 1u : 2u;
        return r;
    }
};
static VisMemo g_vis_memo_inj;    // Render thread (the injection, the PIP inject).
static VisMemo g_vis_memo_flush;  // The flush (the render thread's scene flush, or the game thread under a park).

// The distance twin: mesh_dist_sq (to the world-axis box enclosing the object's rotated size box - not an OBB
// distance) is asked per object by the route verdicts, the emit, the LOD pick and the translucent sort with the same
// camera for the whole pass. Same membership rule and cache-only contract as VisMemo. mesh_order begins it
// after its permutation (a sort moves the objects) and seeds it with the sort's own distances. NEARZ_BOUNDS:
// get_drawn is bounds_dist_sq under the same rule (the near-z route's).
struct DistMemo {
    const RenderObject* base = nullptr;
    size_t n = 0;
    std::vector<float> d2;   // < 0 = unknown.
    std::vector<float> d2b;  // NEARZ_BOUNDS: the drawn box's; < 0 = unknown.
    void begin(const std::vector<RenderObject>& v) {
        base = v.data();
        n = v.size();
        d2.assign(n, -1.0f);
        d2b.assign(n, -1.0f);
    }
    float get_drawn(const RenderObject& o, const float cam[3]) {
        if (!base) return bounds_dist_sq(o, cam);
        const size_t i = static_cast<size_t>(&o - base);
        if (i >= n) return bounds_dist_sq(o, cam);   // Not a member: evaluate live.
        if (d2b[i] >= 0.0f) return d2b[i];
        d2b[i] = bounds_dist_sq(o, cam);
        return d2b[i];
    }
    float get(const RenderObject& o, const float cam[3]) {
        if (!base) return mesh_dist_sq(o, cam);
        const size_t i = static_cast<size_t>(&o - base);
        if (i >= n) return mesh_dist_sq(o, cam);   // Not a member: evaluate live.
        if (d2[i] >= 0.0f) return d2[i];
        d2[i] = mesh_dist_sq(o, cam);
        return d2[i];
    }
};
static DistMemo g_dist_memo_inj;    // Render thread (the injection, the PIP inject).
static DistMemo g_dist_memo_flush;  // The flush (the render thread's scene flush, or the game thread under a park).

// The order the colour passes draw in (the injection, the PIP inject, the flush): by group (opaque solids, then
// translucent and effect meshes, then depth-Off overlays), the opaque solids front to back (early depth rejection),
// the rest back to front. Sorts a 12-byte key (one distance per object) and permutes once, rather than sorting whole
// (~500-byte) objects with the distance recomputed in the comparator.
struct OrdKey {
    float    d;   // mesh_dist_sq to the pass camera.
    uint32_t g;   // 0 opaque solid, 1 translucent/effect, 2 depth-Off.
    uint32_t i;   // Index into the mesh vector.
};
inline uint32_t ord_group(const RenderObject& o) {
    return o.mode == DepthMode::Off ? 2u
         : (!o.fullscreen && o.effect == 0 && o.blend_mode == 0 &&
            o.color[3] >= 0.999f) ? 0u : 1u;
}
// ORD_TIE: the distance as the sort compares it - a non-finite one (inf, NaN: a non-finite position or size)
// as the largest finite float, so every key is ordered. By the bits: a NaN test may be folded by fast-math.
inline float ord_dist(float d) {
    uint32_t u;
    memcpy(&u, &d, sizeof(u));
    return (u & 0x7F800000u) == 0x7F800000u ? FLT_MAX : d;
}
// Scratch, reused across frames and all three passes (never concurrently).
static std::vector<OrdKey>      g_ord_keys;
static std::vector<RenderObject>  g_ord_tmp;
// dm is the pass's distance memo: begun here, after the permutation (the memo keys on addresses inside the
// vector), and seeded with the sort's own distances in the permuted order, so no consumer pays the distance twice.
inline void mesh_order(std::vector<RenderObject>& m, const float cam[3], DistMemo& dm) {
    const size_t n = m.size();
    if (n < 2) {
        dm.begin(m);
        if (n == 1) dm.d2[0] = mesh_dist_sq(m[0], cam);
        return;
    }
    g_ord_keys.clear();
    g_ord_keys.reserve(n);

    for (uint32_t i = 0; i < static_cast<uint32_t>(n); ++i) {
        OrdKey k;
        k.d = mesh_dist_sq(m[i], cam);   // Once per object, not per compare.
        k.g = ord_group(m[i]);
        k.i = i;
        g_ord_keys.push_back(k);
    }

    // ORD_TIE: a total order. Equal distances (every box that holds the camera reads 0, so a translucent dome and
    // the glass or haze inside it tie) break on creation order (seq): std::sort is not stable, and past its
    // insertion-sort size a tied pair's order follows the pivots, i.e. every other object's distance, so without the
    // seq tie the two layers would swap from frame to frame. The translucent tails' stable_sort keeps the order given
    // here. A non-finite distance compares as the largest (ord_dist), so the comparator stays a strict weak order
    // on any input; the memo keeps the distance as computed.
    std::sort(g_ord_keys.begin(), g_ord_keys.end(),
              [&m](const OrdKey& a, const OrdKey& b) {
                  if (a.g != b.g) return a.g < b.g;
                  const float da = ord_dist(a.d), db = ord_dist(b.d);
                  if (da != db) {
                      if (a.g != 0u) return da > db;   // Translucent: back to front.
                      return da < db;                      // Opaque: front to back.
                  }
                  const RenderObject& oa = m[a.i];
                  const RenderObject& ob = m[b.i];
                  if (oa.seq != ob.seq) return oa.seq < ob.seq;   // ORD_TIE.
                  // PART_ORDER: the flush's two part copies of one object tie on all of the above; its solid
                  // texels (draw_part 2) draw before its glass (draw_part 1) where both sort into the tail.
                  return oa.draw_part != 1 && ob.draw_part == 1;
              });
    g_ord_tmp.clear();
    g_ord_tmp.reserve(n);
    for (const OrdKey& k : g_ord_keys) g_ord_tmp.push_back(std::move(m[k.i]));
    m.swap(g_ord_tmp);
    dm.begin(m);
    for (size_t i = 0; i < n; ++i) dm.d2[i] = g_ord_keys[i].d;
}

// FX_TEX_ALL: a pass with textures is never appended - a fused stage rides the head's draw, whose six texture
// slots are the head's.
inline bool fuse_appendable(const RenderObject& o) {
    return (o.effect == 1 || o.effect == 2 || o.effect == 3 ||
            o.effect == 5) && !o.localized && !o.banded && !o.fx_tex;
}
// FUSE_CLOCK: now_d is the flush's clock, the one its fills form fxMeta.y from - the stage's age in [4n].z
// is then the very float its pass takes unfused (KhFuseTail reads it for the grain's frame).
inline bool fuse_append(ConstantData& cbd, const RenderObject& o, double now_d) {
    const int n = static_cast<int>(cbd.fuse_meta[0] + 0.5f);
    if (n >= FUSE_MAX) return false;
    float* st = cbd.fuse_stage[n * 4];
    st[0] = static_cast<float>(o.effect);
    st[1] = static_cast<float>(o.blend_mode);
    st[2] = fx_time(o, now_d);
    st[3] = 0.0f;
    memcpy(cbd.fuse_stage[n * 4 + 1], o.fx,     4 * sizeof(float));
    memcpy(cbd.fuse_stage[n * 4 + 2], o.fx + 4, 4 * sizeof(float));
    memcpy(cbd.fuse_stage[n * 4 + 3], o.color,  4 * sizeof(float));
    cbd.fuse_meta[0] = static_cast<float>(n + 1);
    return true;
}
}