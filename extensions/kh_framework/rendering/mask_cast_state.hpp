#pragma once

// The injection half (rendering_integration_injection.hpp): the engine shadow mask cast's state (the fire lock, the
// cast readiness, the view drop, ShadowMaskState). Never include it on its own.
namespace RenderIntegration {
// Body in the seam suite; the wipe calls it with false (vmir is device-scoped).
inline void kh_svs_mask_release(bool khm_scrub_vmir = true);   // rendering/volume_seam_state.hpp.

static ID3D11ShaderResourceView* g_fire_lock = nullptr;
static float g_fire_fov2[2] = {};
static float g_fire_dims2[2] = {};
static float g_fire_view2[16] = {};
static float g_fire_sun2[3] = {};   // Live per re-fire while it re-publishes.
static bool  g_fire_lock_valid = false;
// KH_FIRE_NEXT_MASK - the cycle in which a shadow-atlas phase ended after this cycle's injection had landed. Those
// cascades and mask resolves belong to the next frame, and replaying this frame's fire into them left a ghost one frame
// behind. Replays are refused from such an exit to the next main-depth clear. Cost: a genuine replay after a
// post-injection atlas phase is refused too.
static uint64_t g_fire_post_scene_cycle = ~0ull;

// Fires this cycle (rolled at the main depth clear); a fire whose count reaches g_fire_clamp_target is the last.
static uint32_t g_fire_count_frame = 0;
static uint64_t g_lock_churn_ms_v = 0;   // Last lock-churn event (adoption / wipe / relock / forced
                                         // drop); 0 = never.
// The era guard's escape: consecutive refusals mean the incumbent is the stale
// one, so recovery is bounded.
static constexpr uint32_t KH_FIRE_ERA_RECOVER_N = 4;   // Consecutive refusals that earn adoption.
static uint32_t g_fire_era_reject_run = 0;
static constexpr uint64_t KH_CAST_READY_QUIET_MS = 1500;
static constexpr uint64_t KH_CAST_READY_DWELL_MS = 1500;   // Continuous-health dwell before the latch.
static bool     g_cast_ready = false;   // The latch: casts allowed.
static uint64_t g_cast_ready_since_ms = 0;   // Dwell start (0 = dwell not running).
static float g_cascbind_feed_t = -1.0f;   // Stamped at every harvest feed.
// The last fire cannot be identified while it is happening, so the index is learned from the previous frame: at each
// main depth clear a frame that fired takes its own final fire count as the target (painted or not); a frame with no
// fire leaves it (starts at 1).
static uint32_t g_fire_clamp_target = 1;   // Learned index of the frame's last fire.
static uint32_t g_fire_clamp_painted = 0;   // Paints carried this frame.
static bool     g_fire_mask_srv_now = false;   // This fire: engine mask SRV-bound at the save.

static uint32_t g_view_drop_path = 0;   // 0 none, 1 reject streak, 2 health miss.
static float    g_view_drop_bar = -1.0f;   // The bar it failed (0.02 + min(md, 0.3)).

static constexpr float    KH_PUB_DROP_HYST     = 1.5f;
// The valve is a clock, not a count: a count-based valve fires inside one frame and the hysteresis becomes a no-op. Two
// seconds of continuous soft rejection is a standing disagreement and drops.
static constexpr uint64_t KH_PUB_DROP_HOLD_MS  = 2000ull;
static uint64_t g_view_drop_hyst_since = 0;   // Stamp of the first hold of a run.
static uint64_t g_view_relock_wide_cycle = 0;   // Topo cycle of the last wide-bar agreement.

// Called from every site that sets view_src_valid true, after the new identity is in place.
inline void kh_view_lock_acquired() {
    if (g_view_drop_path == 0) return;   // No death to compare against.

    // Cleared on heal, or every lane gated on it keeps running.
    g_view_drop_path = 0;
    g_view_relock_wide_cycle = 0;
}

// A backstop paint lands at the end of frame N and is read early in frame N+1; if N+1 rebinds the mask as an RT before
// those reads, that paint never existed. The engine consumes its matrix camera-relatively; the fire is the one
// world-absolute inverse consumer (PSMaskCast reconstructs pw = q).
static float    g_fire_cast_inv[9] = {};   // R^{-1} laid out for castMat row-dots.
static bool     g_fire_cast_inv_valid = false;   // Freeze produced a sane inverse.

static bool g_mask_cast_arm = false;
// The frame whose casters the fire last built its map from. The fire re-arms on every depth resolve (~8 times a frame,
// measured), and the sun pass is idempotent by hash but not free to call - this holds it to once.
static uint64_t g_cast_map_frame = ~0ull;   // KH_CAST_FRAME64.
// Deferred mission destroy: armed by a mission end that could not take the lock (rendering_integration_reset),
// consumed by the next park (kh_park_run).
static std::atomic<bool> g_mission_destroy_pending{ false };

inline void kh_msaa_toggle_wipe(unsigned int khtw_new) {
    if (g_res.depth_sample_count == 0 || g_res.depth_sample_count == khtw_new) return;
    g_res.release_depth_srv();
    // KH_VMSEE_LAZY: one sample = no late chain, nothing reads them; an MSAA count's first snapshot makes them again.
    if (khtw_new < 2) g_res.release_vmsee_targets();
    if (khtw_new < 2) g_res.release_wsee_targets();   // KH_WSEE: likewise - 1x captures nothing (kh_wsee_ensure).
    // KH_WSEE_MSAA_FREE: at any other count change the record is dead (kh_wsee_valid and every re-issue's readiness
    // need its count to be the depth's; the capture's key remakes it) and C views the engine's old scene target - both
    // go here, not held beside the engine's new targets until the next capture or chain run rekeys them. E and the
    // hold (no sample count in their keys) stay. Every later reader tests the pointers or the count, as at 1x above.
    if (khtw_new >= 2) {
        g_res.release_wsee_capture();
        g_res.release_wsee_c();
    }
    kh_svs_mask_release(false);
    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); g_ls.atlas_tex = nullptr; }
    g_ls.atlas_identity = nullptr;
    g_ls.atlas_size = 0;
    g_ls.atlas_fmt = 0;
    g_ls.srv_failed = false;   // A stale true would starve ensure_srv on the new atlas.
    g_ls.cache_valid = false;
    g_ls.atlas_last_seen = -1.0f;
    g_ls.view_src_valid = false;
    g_ls.pub_fresh_ms = 0; g_ls.pub_exact_ms = 0;
    g_ls.count = 0; g_ls.newest = -1;
    for (uint32_t khtw_i = 0; khtw_i < 16; ++khtw_i) g_ls.cb_reg[khtw_i].buf = nullptr;
    g_ls.resolve_window = 0;
    g_ls.resolve_seen_since_cast = false;
    g_ls.phase_on_atlas = false;
}

// g_topo_cycles is defined with KhAttach in rendering/attachment_types.hpp (KH_SKEL_DRAW stamps on it).
static void*     g_topo_scene_tex_id = nullptr;   // Latched scene texture identity (the resolve
                                                  // hook's source compares; the PIP's exclusion).
// KH_SCENE_NAME (kh_scene_tex_name): the cycle the named scene texture was last seen - named, or resolved on the
// render thread. Render thread; reset with the identity.
static uint64_t  g_topo_scene_seen_cycle = 0;
static bool g_mask_cast_fired = false;   // One analytic pass per frame

inline bool shadow_live_wanted() {
    // g_sun_valid is deliberately not a term: the live capture is what produces the derived direction, so gating
    // capture on direction validity would deadlock the warm-up. Direction-dependent consumers gate individually.
    return g_ls.wanted.load(std::memory_order_relaxed) && g_shadow_map_strength > 0.0f;
}

// KH_MASK_REL_W: the width the engine's screen-space shadow resolve target must reach (beside its single-channel
// format, the shadow atlas bound at t15 and the cascade matrix): 1280 wherever the 3D render (the adopted main depth)
// is 1280 wide or more; half the render's width under 1280 (a low Sampling, a small window), where an absolute 1280
// stands down engine shadows on our meshes and our meshes' world cast. 1280 while the main depth is not known yet.
// The width is written by the resolve re-adoption and the flush, read here by the OM hook - a plain UINT, as its
// every other reader in the hooks.
inline uint32_t kh_mask_min_w() {
    const uint32_t khmw_d = static_cast<uint32_t>(g_main_depth_w);
    if (khmw_d == 0u || khmw_d >= 1280u) return 1280u;
    return khmw_d / 2u > 0u ? khmw_d / 2u : 1u;
}

struct ShadowMaskState {
    bool atlas_bound = false;   // Atlas seen at PS t15 since the last draw (mask_note_draw consumes it).
    bool  rt_is_resolve = false;   // Current RT0 is the screen-sized single-channel resolve target
                                   // (KH_MASK_REL_W: at least kh_mask_min_w wide).
    bool  rt_fmt_ok = false;   // Current RT0 has a mask-family format (any size).
    uint32_t rt_w = 0;
    ID3D11RenderTargetView* engine_mask_rtv = nullptr;
    double cold_t0 = -1.0;
    float  cold_first_inject = -1.0f;   // First successful injection.
    float  cold_first_cast = -1.0f;   // First analytic mask write.
    float  cold_first_stage = -1.0f;   // First injection pass with >= 1 eligible mesh.
    float  cold_first_trigger = -1.0f;   // First inject_composited_meshes entry (past the FSAA stand-down).
    ID3D11BlendState*       min_blend = nullptr;   // Out = min(dst, src).
    // Priming states. min_blend cannot be reused (out = min(dst, src), so a priming write would be a no-op); cast_dss
    // cannot either (depth-off and fullscreen, while priming must land exactly on our own footprint).
    ID3D11BlendState*       prime_blend = nullptr;
    ID3D11DepthStencilState* prime_dss = nullptr;   // Depth LESS_EQUAL, no write.
    bool                    prime_states_failed = false;
    // KH_CAST_SCISSOR: the fire's rasterizer with the scissor on (the default state otherwise); created on first use,
    // released with the others.
    ID3D11RasterizerState*  cast_rs_scissor = nullptr;
    bool                    cast_rs_scissor_failed = false;
    ID3D11Query*            prime_occl[3] = {};
    bool                    prime_occl_inflight[3] = {};
    int                     prime_occl_cursor = 0;
    bool                    prime_occl_failed = false;
    bool engine_mask_failed = false;
    ID3D11ShaderResourceView* cast_depth = nullptr;   // Scene linear depth (AddRef).
    float cast_dims[2] = {};
    float cast_fov[4] = {};
    bool  cast_fov_valid = false;
    // Shadow-distance re-engages, time skips and sun<->moon swaps interleave transitional passes whose captures
    // misregister the reconstruction.
    float    sweep_last_time = -1.0f;
    uint32_t sweep_settle = 0;   // Coherent sweeps since a discontinuity.
    uint32_t sweep_need = 3;   // Sweeps required before firing (5 after a gap, 3 after a fov change, 10 after a reset).
    ID3D11DepthStencilState* cast_dss = nullptr;
    bool  cast_states_failed = false;
};

static ShadowMaskState g_mask;
// g_mask.cast_depth is also the DLS world pass's receiver depth (its t0 in kh_dls_world_pass_body), and it arrives
// quantised: on a grazing surface near the camera many pixels share one plateau, so a normal from the 2x2 quad inside a
// plateau is the view axis, not the surface. The world pass measures the plane over KH_DLSW_NRM_R pixels at two
// baselines and carries their agreement into KhDlsWorldFactor (khw_nrel); where they disagree the normal may not drive
// N.L. KH_DLS_BIAS_M must exceed the receiver uncertainty or plateaus flip the compare. Texel: 2*z0/KH_DLS_MAP_PX at 90
// degrees, divided by sin(incidence).
}