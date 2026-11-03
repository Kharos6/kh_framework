#pragma once

// The shared head (rendering_integration.hpp): our sun shadow maps' shared state (validity, biases, the band constants,
// the local casters). Never include it on its own.
namespace RenderIntegration {

static float g_shadow_map_strength = 1.0f;   // 0 disables the map term.
static float g_shadow_map_bias = 0.0015f;   // In the transform's depth units.
static float g_shadow_map_sign = 1.0f;   // +1 standard depth, -1 reversed.
// Our sun shadow maps' shared state: each map's validity and render stamps, the cast biases, the prefilter's validity,
// the map hash and bounds, the local casters.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct SunMapState {
    // Private sun-depth map state (written by the render thread's pass, read by the flush). vp/bias/bounds mean
    // something only while valid. The frame flag resets at the main depth clear (and wherever a pass re-opens the
    // ladder: the cast fire's repicks, SUN_RESTEP, SUN_GEN, the resets); validity persists across cycles (the
    // input-hash skip reuses the standing map) and drops only where render_sun_depth, sun_ladder_forget or the
    // session reset drops it.
    bool  sun_map_rendered_frame = false;   // This frame's pass already ran.
    bool  sun_map_valid = false;   // Pass produced a usable map.
    float sun_map_render_time = -1.0f;   // Not reset by sub-settle sun wobble.
    uint64_t sun_mat_chash = 0;   // Caster hash at the last maturity reset.
    float    sun_mat_sun[3] = {};   // Derived sun at the last maturity reset.
    // When it was produced (effect clock); the cast fire refuses a map 0.25 s old or more (map_fresh).
    float sun_map_time = -1.0f;
    float sun_anchor_now[3] = {};
    float sun_map_cast_bias = 0.0f;   // CAST_BIAS_CAP: the union's cast bias (0 = falls back to sun_meta[2]).
    float    sun2_cast_bias = 0.0f;   // CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
    bool     sun2_map_valid = false;
    float    sun3_cast_bias = 0.0f;   // CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
    bool     sun3_map_valid = false;
    float    sun4_cast_bias = 0.0f;   // CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
    bool     sun4_map_valid = false;
    float    sun5_cast_bias = 0.0f;   // CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
    bool     sun5_map_valid = false;
    bool sun_pf_valid[4] = { false, false, false, false };   // FAR_PF: [3] the far tier.
    uint64_t sun_map_hash = 0;
    float sun_map_bounds[6] = {};   // Combined caster AABB: center xyz, half extents xyz
    // In-map casters, 6 floats each: center xyz + half extents xyz (engine axes) - uncapped.
    std::vector<float> sun_locals;
    int   sun_local_count = 0;   // Valid entries above (render thread, like the rest).
    // Bumped wherever g_sun_maps.sun_locals is rewritten: the occupancy memo's key is an FNV over the whole array, so
    // with a serial the hash runs once per rewrite instead of once per fire.
    uint64_t sun_locals_serial = 0;
};
static SunMapState g_sun_maps;
static constexpr float BAND_BIAS_TEXELS = 2.0f;
static constexpr float BAND_FWD_SLACK = 1.25f;
static constexpr float BAND_FWD_DROP  = 1.60f;   // re-latch tighter past this.

}