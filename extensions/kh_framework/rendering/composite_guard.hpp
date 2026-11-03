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

// FX_PAIR_HOLD: the depth pair guard's held pair, its candidate and the verdict the chain's passes replay.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets 8 of them.
struct FxPairGuard {
    float    pair[4] = {};   // Adopted m22 / m32 / minD / maxD.
    float    pair_near = -1.0f;   // Derived near of the adopted pair (-1 = cold).
    float    cand_near = -1.0f;   // Pending outlier candidate (-1 = none).
    uint64_t pair_ms = 0;
    uint64_t verdict_serial = ~0ull;   // Flush the verdict ran for.
    bool     verdict_held = false;
    // FX_PAIR_HOLD: the UI lane's own verdicts (flush_ui_locked) - a serial of their own, above every flush
    // serial (bit 63), one per Present that decides. Never reset: only its uniqueness is used.
    uint64_t ui_serial = 0;
    uint64_t verdict_cycle = ~0ull;   // The published cycle the verdict was made in (the UI lane's test).
    // FX_PAIR_KEY: the pair (m22, m32) the memoised verdict judged. A later call of the same serial with another
    // pair (the chain-only late run of a cycle whose engine pair latched after the scene flush decided on the
    // fallback route) is judged afresh instead of being handed a verdict made for a different pair; a held verdict
    // is still replayed whatever the pair (every reader of that flush keeps the one adopted pair).
    float    in_pair[2] = {};
};
static FxPairGuard g_fx_pair_guard;

// The hold guard over the pair a depth reader linearises with (depthParams.xy, and the range .zw it was read with):
// one verdict per flush serial (the first call decides, the rest replay it). A pair whose near is within 1.6 x of the
// adopted one's (or an adopted pair older than 500 ms) is adopted; one further off is adopted only when the bridge's
// camera agrees with it within 1.25 x, else the adopted pair stands in for it (held) and the newcomer becomes a
// candidate, adopted later only if it repeats (within 1.25 x) while the bridge cannot be read. FX_PAIR_HOLD: the
// engine's own pair goes through it too (flush_locked, fx_chain_run, flush_ui_locked), not only the fallback
// route's - the engine pair is latched by the upload matching the cycle's boundary camera (proj_try_window: m22
// within 2 %, m32 within 10 %, or 25 % of the last latched m32 off the known slot), and on a rare frame that camera is
// a foreign pass's (a partition, an aux camera), whose near would scale every reader's distance by n_w / n_t. One
// adopted pair serves both routes and the UI lane; the range lanes are adopted and replayed with it. A hold has no
// time bound: each one refreshes the adopted pair's age, so it lasts while the bridge's flush-time read disagrees
// with the pair presented. Holds are counted (getRenderStats fxPairHolds). FX_PAIR_KEY: the memo replays a verdict
// only to the pair it judged (or, held, to every caller of its serial).
inline void fx_depth_pair_guard(ConstantData& cbd, uint64_t serial) {
    const float m22 = cbd.depth_params[0];
    const float m32 = cbd.depth_params[1];
    // Degenerate pairs stand the effect down elsewhere (the stand-down + the
    // shader's m32 gate); never latch or hold against them.
    if (!(m32 < -1.0e-3f) || m22 <= 0.0f) return;
    const float n = -m32 / m22;
    if (n <= 0.0f) return;
    if (serial == g_fx_pair_guard.verdict_serial &&   // FX_PAIR_KEY.
        (g_fx_pair_guard.verdict_held ||
            (m22 == g_fx_pair_guard.in_pair[0] && m32 == g_fx_pair_guard.in_pair[1]))) {
        if (g_fx_pair_guard.verdict_held) {
            memcpy(cbd.depth_params, g_fx_pair_guard.pair, sizeof(g_fx_pair_guard.pair));
        }
        return;
    }
    g_fx_pair_guard.verdict_serial = serial;
    g_fx_pair_guard.in_pair[0] = m22;   // FX_PAIR_KEY.
    g_fx_pair_guard.in_pair[1] = m32;
    g_fx_pair_guard.verdict_cycle = g_topo_cycles_pub.load(std::memory_order_relaxed);   // FX_PAIR_HOLD.
    g_fx_pair_guard.verdict_held = false;
    const uint64_t now = steady_now_ms();
    const bool stale = g_fx_pair_guard.pair_near <= 0.0f ||
                       (now - g_fx_pair_guard.pair_ms) > 500;
    const float r = stale ? 1.0f
        : (n > g_fx_pair_guard.pair_near ? n / g_fx_pair_guard.pair_near
                                     : g_fx_pair_guard.pair_near / n);

    if (stale || r <= 1.6f) {
        memcpy(g_fx_pair_guard.pair, cbd.depth_params, sizeof(g_fx_pair_guard.pair));
        g_fx_pair_guard.pair_near = n;
        g_fx_pair_guard.cand_near = -1.0f;
        g_fx_pair_guard.pair_ms = now;
        return;
    }

    bool bok = false;

    {
        RVExtBridge::ProjectionViewTransform bpv = {};

        if (RVExtBridge::get_projection_view_transform(bpv)) {
            const float bm22 = bpv.projection[2][2];
            const float bm32 = bpv.projection[3][2];

            if (bm32 < -1.0e-3f && bm22 > 0.0f) {
                const float bn = -bm32 / bm22;
                const float br = bn > n ? bn / n
                                                       : n / bn;

                if (bn > 0.0f) {
                    bok = true;

                    if (br <= 1.25f) {
                        memcpy(g_fx_pair_guard.pair, cbd.depth_params, sizeof(g_fx_pair_guard.pair));
                        g_fx_pair_guard.pair_near = n;
                        g_fx_pair_guard.cand_near = -1.0f;
                        g_fx_pair_guard.pair_ms = now;
                        return;
                    }
                }
            }
        }
    }

    if (g_fx_pair_guard.cand_near > 0.0f) {
        const float cr = n > g_fx_pair_guard.cand_near
                       ? n / g_fx_pair_guard.cand_near
                       : g_fx_pair_guard.cand_near / n;
        if (cr <= 1.25f) {
            // A valid bridge read that fell through the band above contradicts the candidate; persistence proves
            // foreignness can repeat, not genuineness. Keep holding.
            if (bok) {
            } else {
                memcpy(g_fx_pair_guard.pair, cbd.depth_params, sizeof(g_fx_pair_guard.pair));
                g_fx_pair_guard.pair_near = n;
                g_fx_pair_guard.cand_near = -1.0f;
                g_fx_pair_guard.pair_ms = now;
                return;
            }
        }
    }

    g_fx_pair_guard.cand_near = n;
    g_fx_pair_guard.pair_ms = now;   // The latch stays fresh while holding.
    g_fx_pair_guard.verdict_held = true;   // Replayed by this flush's later draws.
    stat_inc(g_stats.fx_pair_holds);   // FX_PAIR_HOLD: one per verdict that held.
    memcpy(cbd.depth_params, g_fx_pair_guard.pair, sizeof(g_fx_pair_guard.pair));
}

// The composited path counts as healthy while injections are actually happening; outside that window eligible meshes
// revert to the post-scene flush path.
inline bool composite_path_healthy() {
    const uint64_t last = g_composite_last_inject_ms.load(std::memory_order_relaxed);
    return last != 0 && (steady_now_ms() - last) < 500;
}

inline bool is_composite_eligible(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off &&
           o.blend_mode == 0 && o.color[3] >= 0.999f && !o.in_front;   // INFRONT: its own slice.
}

// A genuine scene issues many opaque draws between its depth clear and its first translucent; anomaly passes (late
// re-clears, previews, UI blending) issue almost none. Requiring this many rejects those mistimings.
static constexpr uint32_t REORDER_MIN_OPAQUE_DRAWS = 128;
// The relative term must exceed the worst-case reconstruction error from near-plane uncertainty (d_est = d_true *
// n_used / n_true - relative, not absolute), so the guard never falsely clips a mesh genuinely in front.
static constexpr float COMPOSITE_GUARD_BASE = 0.5f;
static constexpr float COMPOSITE_GUARD_REL = 0.25f;
static constexpr float COMPOSITE_GUARD_BASE_MEASURED = 0.25f;
static constexpr float COMPOSITE_GUARD_REL_MEASURED = 0.02f;
static constexpr float FAR_ARB_GUARD_BASE = 15.0f;
// The hardware LESS_EQUAL test is the object authority; terrain is beaten analytically: the arb PS writes SV_Depth,
// pulling the fragment in front of the terrain-LOD band only where the analytic terrain says it is visible and only
// within the contact band below (clearance-ramped: zero offset above it).
static constexpr float FAR_ARB_CONTACT_H = 6.0f;   // Clearance (m) below which the LOD fight exists.
// The sign of this lane carries the occluder-identity gate to the composite pull: the shader reads abs for the ramp
// and runs the terrain probe only where it is positive. The value is always positive (the gate always armed: the pull
// lands only where the probe finds terrain); a negative one would keep the ramp and skip the probe (no identity test).
inline float far_arb_contact() {
    const float h = FAR_ARB_CONTACT_H;
    return h;
}
// Its one use: the effect-mesh far arbitration's relative margin (KhFxBegin, localParams1.w: a fragment beyond
// scene x (1 + REL) + FAR_ARB_GUARD_BASE discards - 15.6 m past a 30 m surface, 18.2 m past 158 m, 19.8 m past
// 240 m). The solid far-arb pull's cap is PSComposite's min(FAR_ARB_GUARD_BASE, 0.06 x w) (composite.hlsl): 1.8 m
// at 30 m, 9.5 m at 158 m, 14.4 m at 240 m.
static constexpr float FAR_ARB_GUARD_REL  = 0.02f;
static constexpr float FAR_ARB_NEAR_MIN = 5.0f;
static constexpr float FAR_ARB_NEAR_HOLD = 2.0f;

// Occlusion-guard snapshot state + runtime config (see snapshot_composite_depth).
static constexpr uint32_t SNAP_MAX_AGE_MS = 500;   // Injection freshness gate (health window).
// The occlusion-guard snapshot (snapshot_take): its serial and time, the pair, camera and viewport it was taken
// under.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct SnapState {
    uint64_t serial = 0;   // Completed snapshots (0 = never).
    uint64_t ms = 0;   // Steady stamp of the last snapshot.
    float    pair[2] = { 0.0f, 0.0f };   // m22/m32 the snapshot depth carries.
    int      pair_src = 0;   // 0 none, 1 engine pair, 2 last injection pair.
    float    cam[3] = { 0.0f, 0.0f, 0.0f };
    float    vp[4][4] = {};   // Rebased world->clip of the snapshot frame.
    float    vp_lo = 0.011f;   // Its viewport depth range.
    float    vp_hi = 0.999f;
    bool     vp_valid = false;
};
static SnapState g_snap;
// What the last proceeding injection published for the late readers (the flush, the seam, the PIP): the encode pair
// and near (enc_*), the view (view, view_ms), the depth lanes (dp).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member. session_scratch_reset resets 6 of them.
struct InjPublishState {
    float             dp[4] = {};
    bool              dp_valid = false;
    // The last proceeding injection attempt's view (after adoption) and its stamp: the flush and the DLS world pass
    // take it on a cycle that landed, within 100 ms. Rotation only (adopt_frame_view, below): the mesh passes adopt
    // fv's rotation whenever it is frame-fresh; the latch remains the authority when it is not. The supervisor's
    // truth/health reference stays the pre-adoption latch (a wrong lock must not self-certify through its own adopted
    // publishes).
    float    view[16] = {};
    uint64_t view_ms = 0;
    // The injection's depth encode (projection m22 / m32 and its near) and its stamp, re-published by every proceeding
    // attempt (inject_composited_meshes, below its skip returns): the flush and svs_take_pv read the last painter's.
    float    enc_m22 = 0.0f;
    float    enc_m32 = 0.0f;
    float    enc_near = -1.0f;
    uint64_t enc_ms = 0;
};
static InjPublishState g_inj_pub;
}