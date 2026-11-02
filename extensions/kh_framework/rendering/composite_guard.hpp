#pragma once

// The injection half (rendering_integration_injection.hpp): the composite guard: the depth pair guard, composite
// eligibility, the far arbiter and the snapshot state. Never include it on its own.
namespace RenderIntegration {
static std::atomic<uint64_t> g_composite_last_inject_ms{0};
static std::atomic<uint64_t> g_composite_inject_serial{0};

static RVExtBridge::ProjectionViewTransform g_last_pv = {};
static uint64_t g_last_pv_ms = 0;

// The locator's validity blinks on foreign-pass frames, so the anomaly gate
// keys on ever-proven-this-session. Cleared only with the session.
static bool g_proj_locator_ever = false;

static float    g_khfx_pair[4] = {};   // Adopted m22 / m32 / minD / maxD.
static float    g_khfx_pair_near = -1.0f;   // Derived near of the adopted pair (-1 = cold).
static float    g_khfx_cand_near = -1.0f;   // Pending outlier candidate (-1 = none).
static uint64_t g_khfx_pair_ms = 0;

static uint64_t g_khfx_verdict_serial = ~0ull;   // Flush the verdict ran for.
static bool     g_khfx_verdict_held = false;
// KH_FX_PAIR_HOLD: the UI lane's own verdicts (flush_ui_locked) - a serial of their own, above every flush
// serial (bit 63), one per Present that decides. Never reset: only its uniqueness is used.
static uint64_t g_khfx_ui_serial = 0;
static uint64_t g_khfx_verdict_cycle = ~0ull;   // The published cycle the verdict was made in (the UI lane's test).
// KH_FX_PAIR_KEY: the pair (m22, m32) the memoised verdict judged. A later call of the same serial with another
// pair (the chain-only late run of a cycle whose engine pair latched after the scene flush decided on the
// fallback route) is judged afresh instead of being handed a verdict made for a different pair; a held verdict
// is still replayed whatever the pair (every reader of that flush keeps the one adopted pair).
static float    g_khfx_in_pair[2] = {};

// The hold guard over the pair a depth reader linearises with (depthParams.xy, and the range .zw it was read with):
// one verdict per flush serial (the first call decides, the rest replay it). A pair whose near is within 1.6 x of the
// adopted one's (or an adopted pair older than 500 ms) is adopted; one further off is adopted only when the bridge's
// camera agrees with it within 1.25 x, else the adopted pair stands in for it (held) and the newcomer becomes a
// candidate, adopted later only if it repeats (within 1.25 x) while the bridge cannot be read. KH_FX_PAIR_HOLD: the
// engine's own pair goes through it too (flush_locked, kh_fx_chain_run, flush_ui_locked), not only the fallback
// route's - the engine pair is latched by the upload matching the cycle's boundary camera (proj_try_window: m22
// within 2 %, m32 within 10 %, or 25 % of the last latched m32 off the known slot), and on a rare frame that camera is
// a foreign pass's (a partition, an aux camera), whose near would scale every reader's distance by n_w / n_t. One
// adopted pair serves both routes and the UI lane; the range lanes are adopted and replayed with it. A hold has no
// time bound: each one refreshes the adopted pair's age, so it lasts while the bridge's flush-time read disagrees
// with the pair presented. Holds are counted (getRenderStats fxPairHolds). KH_FX_PAIR_KEY: the memo replays a verdict
// only to the pair it judged (or, held, to every caller of its serial).
inline void kh_fx_depth_pair_guard(ConstantData& cbd, uint64_t khfx_serial) {
    const float khfx_m22 = cbd.depth_params[0];
    const float khfx_m32 = cbd.depth_params[1];
    // Degenerate pairs stand the effect down elsewhere (the stand-down + the
    // shader's m32 gate); never latch or hold against them.
    if (!(khfx_m32 < -1.0e-3f) || khfx_m22 <= 0.0f) return;
    const float khfx_n = -khfx_m32 / khfx_m22;
    if (khfx_n <= 0.0f) return;
    if (khfx_serial == g_khfx_verdict_serial &&   // KH_FX_PAIR_KEY.
        (g_khfx_verdict_held || (khfx_m22 == g_khfx_in_pair[0] && khfx_m32 == g_khfx_in_pair[1]))) {
        if (g_khfx_verdict_held) {
            memcpy(cbd.depth_params, g_khfx_pair, sizeof(g_khfx_pair));
        }
        return;
    }
    g_khfx_verdict_serial = khfx_serial;
    g_khfx_in_pair[0] = khfx_m22;   // KH_FX_PAIR_KEY.
    g_khfx_in_pair[1] = khfx_m32;
    g_khfx_verdict_cycle = g_topo_cycles_pub.load(std::memory_order_relaxed);   // KH_FX_PAIR_HOLD.
    g_khfx_verdict_held = false;
    const uint64_t khfx_now = steady_now_ms();
    const bool khfx_stale = g_khfx_pair_near <= 0.0f ||
                            (khfx_now - g_khfx_pair_ms) > 500;
    const float khfx_r = khfx_stale ? 1.0f
        : (khfx_n > g_khfx_pair_near ? khfx_n / g_khfx_pair_near
                                     : g_khfx_pair_near / khfx_n);

    if (khfx_stale || khfx_r <= 1.6f) {
        memcpy(g_khfx_pair, cbd.depth_params, sizeof(g_khfx_pair));
        g_khfx_pair_near = khfx_n;
        g_khfx_cand_near = -1.0f;
        g_khfx_pair_ms = khfx_now;
        return;
    }

    bool khfx_bok = false;

    {
        RVExtBridge::ProjectionViewTransform khfx_bpv = {};

        if (RVExtBridge::get_projection_view_transform(khfx_bpv)) {
            const float khfx_bm22 = khfx_bpv.projection[2][2];
            const float khfx_bm32 = khfx_bpv.projection[3][2];

            if (khfx_bm32 < -1.0e-3f && khfx_bm22 > 0.0f) {
                const float khfx_bn = -khfx_bm32 / khfx_bm22;
                const float khfx_br = khfx_bn > khfx_n ? khfx_bn / khfx_n
                                                       : khfx_n / khfx_bn;

                if (khfx_bn > 0.0f) {
                    khfx_bok = true;

                    if (khfx_br <= 1.25f) {
                        memcpy(g_khfx_pair, cbd.depth_params, sizeof(g_khfx_pair));
                        g_khfx_pair_near = khfx_n;
                        g_khfx_cand_near = -1.0f;
                        g_khfx_pair_ms = khfx_now;
                        return;
                    }
                }
            }
        }
    }

    if (g_khfx_cand_near > 0.0f) {
        const float khfx_cr = khfx_n > g_khfx_cand_near
                            ? khfx_n / g_khfx_cand_near
                            : g_khfx_cand_near / khfx_n;
        if (khfx_cr <= 1.25f) {
            // A valid bridge read that fell through the band above contradicts the candidate; persistence proves
            // foreignness can repeat, not genuineness. Keep holding.
            if (khfx_bok) {
            } else {
                memcpy(g_khfx_pair, cbd.depth_params, sizeof(g_khfx_pair));
                g_khfx_pair_near = khfx_n;
                g_khfx_cand_near = -1.0f;
                g_khfx_pair_ms = khfx_now;
                return;
            }
        }
    }

    g_khfx_cand_near = khfx_n;
    g_khfx_pair_ms = khfx_now;   // The latch stays fresh while holding.
    g_khfx_verdict_held = true;   // Replayed by this flush's later draws.
    kh_stat(g_stats.fx_pair_holds);   // KH_FX_PAIR_HOLD: one per verdict that held.
    memcpy(cbd.depth_params, g_khfx_pair, sizeof(g_khfx_pair));
}

// The composited path counts as healthy while injections are actually happening; outside that window eligible meshes
// revert to the post-scene flush path.
inline bool composite_path_healthy() {
    const uint64_t last = g_composite_last_inject_ms.load(std::memory_order_relaxed);
    return last != 0 && (steady_now_ms() - last) < 500;
}

inline bool is_composite_eligible(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           o.blend_mode == 0 && o.color[3] >= 0.999f && !o.in_front;   // KH_INFRONT: its own slice.
}

// A genuine scene issues many opaque draws between its depth clear and its first translucent; anomaly passes (late
// re-clears, previews, UI blending) issue almost none. Requiring this many rejects those mistimings.
static constexpr uint32_t KH_REORDER_MIN_OPAQUE_DRAWS = 128;
// The relative term must exceed the worst-case reconstruction error from near-plane uncertainty (d_est = d_true *
// n_used / n_true - relative, not absolute), so the guard never falsely clips a mesh genuinely in front.
static constexpr float KH_COMPOSITE_GUARD_BASE = 0.5f;
static constexpr float KH_COMPOSITE_GUARD_REL = 0.25f;
static constexpr float KH_COMPOSITE_GUARD_BASE_MEASURED = 0.25f;
static constexpr float KH_COMPOSITE_GUARD_REL_MEASURED = 0.02f;
static constexpr float KH_FAR_ARB_GUARD_BASE = 15.0f;
// The hardware LESS_EQUAL test is the object authority; terrain is beaten analytically: the arb PS writes SV_Depth,
// pulling the fragment in front of the terrain-LOD band only where the analytic terrain says it is visible and only
// within the contact band below (clearance-ramped: zero offset above it).
static constexpr float KH_FAR_ARB_CONTACT_H = 6.0f;   // Clearance (m) below which the LOD fight exists.
// The sign of this lane carries the occluder-identity gate to the composite pull: the shader reads abs for the ramp
// and runs the terrain probe only where it is positive. The value is always positive (the gate always armed: the pull
// lands only where the probe finds terrain); a negative one would keep the ramp and skip the probe (no identity test).
inline float kh_far_arb_contact() {
    const float khc_h = KH_FAR_ARB_CONTACT_H;
    return khc_h;
}
// Its one use: the effect-mesh far arbitration's relative margin (KhFxBegin, localParams1.w: a fragment beyond
// scene x (1 + REL) + KH_FAR_ARB_GUARD_BASE discards - 15.6 m past a 30 m surface, 18.2 m past 158 m, 19.8 m past
// 240 m). The solid far-arb pull's cap is PSComposite's min(KH_FAR_ARB_GUARD_BASE, 0.06 x w) (composite.hlsl): 1.8 m
// at 30 m, 9.5 m at 158 m, 14.4 m at 240 m.
static constexpr float KH_FAR_ARB_GUARD_REL  = 0.02f;
static constexpr float KH_FAR_ARB_NEAR_MIN = 5.0f;
static constexpr float KH_FAR_ARB_NEAR_HOLD = 2.0f;

// Occlusion-guard snapshot state + runtime config (see snapshot_composite_depth).
static constexpr uint32_t KH_SNAP_MAX_AGE_MS = 500;   // Injection freshness gate (health window).
static uint64_t g_snap_serial = 0;   // Completed snapshots (0 = never).
static uint64_t g_snap_ms = 0;   // Steady stamp of the last snapshot.
static float             g_inj_dp[4] = {};
static bool              g_inj_dp_valid = false;
static float    g_snap_pair[2] = { 0.0f, 0.0f };   // m22/m32 the snapshot depth carries.
static int      g_snap_pair_src = 0;   // 0 none, 1 engine pair, 2 last injection pair.
static float    g_snap_cam[3] = { 0.0f, 0.0f, 0.0f };
static float    g_snap_vp[4][4] = {};   // Rebased world->clip of the snapshot frame.
static float    g_snap_vp_lo = 0.011f;   // Its viewport depth range.
static float    g_snap_vp_hi = 0.999f;
static bool     g_snap_vp_valid = false;
}