#pragma once

// The feature half (rendering_integration_features.hpp): the simulation LOD (KH_SIMLOD) of cloth and chains. Never
// include it on its own.
namespace RenderIntegration {

// KH_SIM_LOD - SOLVER LEVEL OF DETAIL. A cloth or chain that is both small on screen AND far from the camera runs a
// share of the iterations and substeps the object asked for; one that is either close or large on screen runs them
// whole. The share q is floor + (1 - floor) * max(qScreen, qDist): qScreen ramps (smoothstep) from 0 at min_frac to
// 1 at full_frac of the screen height the instance's bounding sphere spans (r * m11 / d, so a narrower field of view
// - a scope - keeps the quality), qDist from 0 at min_dist to 1 at full_dist metres from the camera to the sphere's
// nearest point. Iterations run at q of the dial; substeps at min(1, 2q) of it (the iterations give first; the
// substep length changes only below half share, and kh_cloth_step carries the velocity across the change). Both
// count floors are 1. floor = 1 turns it off. The camera is the render thread's boundary camera (kh_simlod_publish
// at the main depth clear), read here through a seqlock; a view older than KH_SIMLOD_STALE_S (no scene boundary -
// the map, a menu) reads as 'unknown', which is full quality. The dials are the object's (RenderObject::sim_lod, the
// simulationLod key of updateRender3D), so a hero cloth can be exempt.
static constexpr float KH_SIMLOD_STALE_S = 1.0f;
// The boundary view: written by the clear hook (render thread), read by kh_cloth_sync (game thread). A seqlock over
// atomics: an odd sequence is a write in progress; a reader whose two sequence reads differ tries again.
static std::atomic<uint32_t> g_simlod_seq{ 0 };
static std::atomic<float>    g_simlod_cam[3] = { {0.0f}, {0.0f}, {0.0f} };   // Engine axes.
static std::atomic<float>    g_simlod_m11{ 0.0f };   // The projection's vertical scale.
static std::atomic<float>    g_simlod_t{ -1.0e30f };   // Effect clock (s) of the publish.
inline void kh_simlod_publish(const float khsp_cam[3], float khsp_m11, float khsp_t) {
    const uint32_t khsp_s = g_simlod_seq.load(std::memory_order_relaxed);
    g_simlod_seq.store(khsp_s + 1u, std::memory_order_relaxed);   // Odd: in progress.
    // Each field's release orders the odd sequence before it; the even sequence's release orders every field before
    // that. No fences (TSan).
    for (int k = 0; k < 3; ++k) g_simlod_cam[k].store(khsp_cam[k], std::memory_order_release);
    g_simlod_m11.store(khsp_m11, std::memory_order_release);
    g_simlod_t.store(khsp_t, std::memory_order_release);
    g_simlod_seq.store(khsp_s + 2u, std::memory_order_release);   // Even: stable.
}
// False = no consistent view (never published, or a write in flight past eight tries): the caller takes full quality.
inline bool kh_simlod_read(float khsr_cam[3], float& khsr_m11, float& khsr_t) {
    for (int khsr_try = 0; khsr_try < 8; ++khsr_try) {
        const uint32_t khsr_s0 = g_simlod_seq.load(std::memory_order_acquire);
        if (khsr_s0 == 0u || (khsr_s0 & 1u)) { if (khsr_s0 == 0u) return false; continue; }
        // Acquire loads: a field read from a write in flight carries that write's odd sequence, which the re-read
        // below then sees.
        for (int k = 0; k < 3; ++k) khsr_cam[k] = g_simlod_cam[k].load(std::memory_order_acquire);
        khsr_m11 = g_simlod_m11.load(std::memory_order_acquire);
        khsr_t = g_simlod_t.load(std::memory_order_acquire);
        if (g_simlod_seq.load(std::memory_order_relaxed) == khsr_s0) return true;
    }
    return false;
}
inline float kh_simlod_smooth(float khss_x) {
    if (!(khss_x > 0.0f)) return 0.0f;   // NaN reads as 0 (the ramp's floor end).
    if (khss_x >= 1.0f) return 1.0f;
    return khss_x * khss_x * (3.0f - 2.0f * khss_x);
}
// The share (floor..1) for a sphere of radius khss_r at khss_c (engine axes) seen from the view; 1 when the view is
// unknown or the switch is off. Every NaN input reads as full quality; an infinite one need not (a centre or camera
// at infinity, the rest finite, reads as the floor). khss_dl = the object's five dials (RenderObject::sim_lod: floor,
// full distance, min distance, full screen share, min screen share), khss_on its switch.
inline float kh_simlod_share(bool khss_view, const float khss_cam[3], float khss_m11, const float khss_c[3], float khss_r,
                             const float khss_dl[5], bool khss_on) {
    float khss_floor = khss_dl[0];
    if (!khss_on || !(khss_floor < 0.999f) || !khss_view) return 1.0f;
    if (khss_floor < 0.05f) khss_floor = 0.05f;
    const float khss_dfull = khss_dl[1], khss_dmin = khss_dl[2];
    const float khss_ffull = khss_dl[3], khss_fmin = khss_dl[4];
    if (!(khss_dmin > khss_dfull) || !(khss_dfull >= 0.0f) || !(khss_ffull > khss_fmin) || !(khss_fmin >= 0.0f)) return 1.0f;
    if (!(khss_r >= 0.0f) || !(khss_m11 > 0.0f)) return 1.0f;
    float khss_d2 = 0.0f;
    for (int k = 0; k < 3; ++k) { const float khss_e = khss_c[k] - khss_cam[k]; khss_d2 += khss_e * khss_e; }
    if (!(khss_d2 >= 0.0f)) return 1.0f;
    const float khss_d = sqrtf(khss_d2);
    const float khss_dn = khss_d > khss_r ? khss_d - khss_r : 0.0f;   // To the sphere's nearest point.
    const float khss_f = khss_d > 1.0e-3f ? khss_r * khss_m11 / khss_d : 1.0e9f;   // Of the screen height.
    const float khss_qf = kh_simlod_smooth((khss_f - khss_fmin) / (khss_ffull - khss_fmin));
    const float khss_qd = kh_simlod_smooth((khss_dmin - khss_dn) / (khss_dmin - khss_dfull));
    const float khss_q = khss_qf > khss_qd ? khss_qf : khss_qd;
    return khss_floor + (1.0f - khss_floor) * khss_q;
}
// A dial's count at share q, with hysteresis: the last count stands until the scaled dial has moved a whole count
// past it (a half count beyond the rounding boundary), so a cloth drifting about a boundary holds. Never above the
// dial, never below 1; a last count past the dial (the dial was lowered) is dropped.
inline uint16_t kh_simlod_count(uint16_t khsc_dial, float khsc_q, uint16_t khsc_last) {
    if (khsc_dial < 1u) khsc_dial = 1u;
    if (!(khsc_q < 1.0f)) return khsc_dial;
    const float khsc_t = static_cast<float>(khsc_dial) * (khsc_q > 0.0f ? khsc_q : 0.0f);
    int khsc_c = static_cast<int>(khsc_t + 0.5f);
    if (khsc_c < 1) khsc_c = 1;
    if (khsc_c > static_cast<int>(khsc_dial)) khsc_c = static_cast<int>(khsc_dial);
    if (khsc_last >= 1u && khsc_last <= khsc_dial && khsc_c != static_cast<int>(khsc_last) &&
        fabsf(khsc_t - static_cast<float>(khsc_last)) < 1.0f) return khsc_last;
    return static_cast<uint16_t>(khsc_c);
}
// KH_CHAIN_LOD_FLOOR - the least a CHAIN runs at under the simulation LOD: two passes and three substeps, or the
// script's own dial where that is lower. A chain's joints are one linearised direct solve per pass
// (kh_chain_joint_solve), and the LOD's lowest counts from the default dials (1 x 2 from 2 x 4) leave residuals that
// come back as velocity: a chain of ten bones or more can then diverge, its links flying apart for as long as the share
// stays low; two passes at two substeps still open a forty-bone chain under a carrier's steady acceleration. The
// counts kh_simlod_count keeps (in_iter / in_sub, its hysteresis) are not changed; only what the step runs and the
// work the stats count are raised, so a chain at or above the floor is untouched.
static constexpr uint16_t KH_CHAIN_LOD_ITER = 2u;
static constexpr uint16_t KH_CHAIN_LOD_SUB = 3u;
inline uint16_t kh_chain_lod_floor(uint16_t khlf_dial, uint16_t khlf_count, uint16_t khlf_least) {
    const uint16_t khlf_min = khlf_dial < khlf_least ? khlf_dial : khlf_least;
    return khlf_count < khlf_min ? khlf_min : khlf_count;
}
// Stats: the solver work (substeps x iterations, summed over the instances stepped this frame) at the chosen counts
// and at the dials. Game thread (kh_cloth_sync), published by kh_cloth_stats_publish.
static std::atomic<uint64_t> g_simlod_work{ 0 };
static std::atomic<uint64_t> g_simlod_full{ 0 };
}