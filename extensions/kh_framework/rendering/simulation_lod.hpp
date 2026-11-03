#pragma once

// The feature half (rendering_integration_features.hpp): the simulation LOD (SIMLOD) of cloth and chains. Never
// include it on its own.
namespace RenderIntegration {

// SIM_LOD - SOLVER LEVEL OF DETAIL. A cloth or chain that is both small on screen AND far from the camera runs a
// share of the iterations and substeps the object asked for; one that is either close or large on screen runs them
// whole. The share q is floor + (1 - floor) * max(qScreen, qDist): qScreen ramps (smoothstep) from 0 at min_frac to
// 1 at full_frac of the screen height the instance's bounding sphere spans (r * m11 / d, so a narrower field of view
// - a scope - keeps the quality), qDist from 0 at min_dist to 1 at full_dist metres from the camera to the sphere's
// nearest point. Iterations run at q of the dial; substeps at min(1, 2q) of it (the iterations give first; the
// substep length changes only below half share, and cloth_step carries the velocity across the change). Both
// count floors are 1. floor = 1 turns it off. The camera is the render thread's boundary camera (simlod_publish
// at the main depth clear), read here through a seqlock; a view older than SIMLOD_STALE_S (no scene boundary -
// the map, a menu) reads as 'unknown', which is full quality. The dials are the object's (RenderObject::sim_lod, the
// simulationLod key of updateRender3D), so a hero cloth can be exempt.
static constexpr float SIMLOD_STALE_S = 1.0f;
// SIM_LOD's boundary view: written by the clear hook (render thread), read by cloth_sync (game thread) - a
// seqlock over atomics (an odd sequence is a write in progress).
// Threads: the clear hook (render thread) writes the view (seq, cam, m11, t) under the seqlock and cloth_sync (game
// thread) reads it; cloth_sync writes work and full, which the flush reads for the stats.
// Session reset: session_globals_reset resets every member.
struct SimLodView {
    // The boundary view: written by the clear hook (render thread), read by cloth_sync (game thread). A seqlock over
    // atomics: an odd sequence is a write in progress; a reader whose two sequence reads differ tries again.
    std::atomic<uint32_t> seq{ 0 };
    std::atomic<float>    cam[3] = { {0.0f}, {0.0f}, {0.0f} };   // Engine axes.
    std::atomic<float>    m11{ 0.0f };   // The projection's vertical scale.
    std::atomic<float>    t{ -1.0e30f };   // Effect clock (s) of the publish.
    // Stats: the solver work (substeps x iterations, summed over the instances stepped this frame) at the chosen counts
    // and at the dials. Game thread (cloth_sync), published by cloth_stats_publish.
    std::atomic<uint64_t> work{ 0 };
    std::atomic<uint64_t> full{ 0 };
};
static SimLodView g_simlod;
inline void simlod_publish(const float cam[3], float m11, float t) {
    const uint32_t s = g_simlod.seq.load(std::memory_order_relaxed);
    g_simlod.seq.store(s + 1u, std::memory_order_relaxed);   // Odd: in progress.
    // Each field's release orders the odd sequence before it; the even sequence's release orders every field before
    // that. No fences (TSan).
    for (int k = 0; k < 3; ++k) g_simlod.cam[k].store(cam[k], std::memory_order_release);
    g_simlod.m11.store(m11, std::memory_order_release);
    g_simlod.t.store(t, std::memory_order_release);
    g_simlod.seq.store(s + 2u, std::memory_order_release);   // Even: stable.
}
// False = no consistent view (never published, or a write in flight past eight tries): the caller takes full quality.
inline bool simlod_read(float cam[3], float& m11, float& t) {
    for (int sr_try = 0; sr_try < 8; ++sr_try) {
        const uint32_t s0 = g_simlod.seq.load(std::memory_order_acquire);
        if (s0 == 0u || (s0 & 1u)) { if (s0 == 0u) return false; continue; }
        // Acquire loads: a field read from a write in flight carries that write's odd sequence, which the re-read
        // below then sees.
        for (int k = 0; k < 3; ++k) cam[k] = g_simlod.cam[k].load(std::memory_order_acquire);
        m11 = g_simlod.m11.load(std::memory_order_acquire);
        t = g_simlod.t.load(std::memory_order_acquire);
        if (g_simlod.seq.load(std::memory_order_relaxed) == s0) return true;
    }
    return false;
}
inline float simlod_smooth(float x) {
    if (!(x > 0.0f)) return 0.0f;   // NaN reads as 0 (the ramp's floor end).
    if (x >= 1.0f) return 1.0f;
    return x * x * (3.0f - 2.0f * x);
}
// The share (floor..1) for a sphere of radius r at c (engine axes) seen from the view; 1 when the view is
// unknown or the switch is off. Every NaN input reads as full quality; an infinite one need not (a centre or camera
// at infinity, the rest finite, reads as the floor). dl = the object's five dials (RenderObject::sim_lod: floor,
// full distance, min distance, full screen share, min screen share), on its switch.
inline float simlod_share(bool view, const float cam[3], float m11, const float c[3], float r,
                          const float dl[5], bool on) {
    float floor = dl[0];
    if (!on || !(floor < 0.999f) || !view) return 1.0f;
    if (floor < 0.05f) floor = 0.05f;
    const float dfull = dl[1], dmin = dl[2];
    const float ffull = dl[3], fmin = dl[4];
    if (!(dmin > dfull) || !(dfull >= 0.0f) || !(ffull > fmin) || !(fmin >= 0.0f)) return 1.0f;
    if (!(r >= 0.0f) || !(m11 > 0.0f)) return 1.0f;
    float d2 = 0.0f;
    for (int k = 0; k < 3; ++k) { const float e = c[k] - cam[k]; d2 += e * e; }
    if (!(d2 >= 0.0f)) return 1.0f;
    const float d = sqrtf(d2);
    const float dn = d > r ? d - r : 0.0f;   // To the sphere's nearest point.
    const float f = d > 1.0e-3f ? r * m11 / d : 1.0e9f;   // Of the screen height.
    const float qf = simlod_smooth((f - fmin) / (ffull - fmin));
    const float qd = simlod_smooth((dmin - dn) / (dmin - dfull));
    const float q = qf > qd ? qf : qd;
    return floor + (1.0f - floor) * q;
}
// A dial's count at share q, with hysteresis: the last count stands until the scaled dial has moved a whole count
// past it (a half count beyond the rounding boundary), so a cloth drifting about a boundary holds. Never above the
// dial, never below 1; a last count past the dial (the dial was lowered) is dropped.
inline uint16_t simlod_count(uint16_t dial, float q, uint16_t last) {
    if (dial < 1u) dial = 1u;
    if (!(q < 1.0f)) return dial;
    const float t = static_cast<float>(dial) * (q > 0.0f ? q : 0.0f);
    int c = static_cast<int>(t + 0.5f);
    if (c < 1) c = 1;
    if (c > static_cast<int>(dial)) c = static_cast<int>(dial);
    if (last >= 1u && last <= dial && c != static_cast<int>(last) &&
        fabsf(t - static_cast<float>(last)) < 1.0f) return last;
    return static_cast<uint16_t>(c);
}
// CHAIN_LOD_FLOOR - the least a CHAIN runs at under the simulation LOD: two passes and three substeps, or the
// script's own dial where that is lower. A chain's joints are one linearised direct solve per pass
// (chain_joint_solve), and the LOD's lowest counts from the default dials (1 x 2 from 2 x 4) leave residuals that
// come back as velocity: a chain of ten bones or more can then diverge, its links flying apart for as long as the share
// stays low; two passes at two substeps still open a forty-bone chain under a carrier's steady acceleration. The
// counts simlod_count keeps (in_iter / in_sub, its hysteresis) are not changed; only what the step runs and the
// work the stats count are raised, so a chain at or above the floor is untouched.
static constexpr uint16_t CHAIN_LOD_ITER = 2u;
static constexpr uint16_t CHAIN_LOD_SUB = 3u;
inline uint16_t chain_lod_floor(uint16_t dial, uint16_t count, uint16_t least) {
    const uint16_t lf_min = dial < least ? dial : least;
    return count < lf_min ? lf_min : count;
}
}