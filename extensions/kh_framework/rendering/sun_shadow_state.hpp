#pragma once

// The shared head (rendering_integration.hpp): our sun shadow maps' shared state (validity, biases, the band constants,
// the local casters). Never include it on its own.
namespace RenderIntegration {

static float g_shadow_map_strength = 1.0f;   // 0 disables the map term.
static float g_shadow_map_bias = 0.0015f;   // In the transform's depth units.
static float g_shadow_map_sign = 1.0f;   // +1 standard depth, -1 reversed.
// Private sun-depth map state (written by the render thread's pass, read by the flush). vp/bias/bounds mean
// something only while valid. The frame flag resets at the main depth clear (and wherever a pass re-opens the
// ladder: the cast fire's repicks, KH_SUN_RESTEP, KH_SUN_GEN, the resets); validity persists across cycles (the
// input-hash skip reuses the standing map) and drops only where render_sun_depth, kh_sun_ladder_forget or the
// session reset drops it.
static bool  g_sun_map_rendered_frame = false;   // This frame's pass already ran.
static bool  g_sun_map_valid = false;   // Pass produced a usable map.
static float g_sun_map_render_time = -1.0f;   // Not reset by sub-settle sun wobble.
static uint64_t g_sun_mat_chash = 0;   // Caster hash at the last maturity reset.
static float    g_sun_mat_sun[3] = {};   // Derived sun at the last maturity reset.
// When it was produced (the fire consumes last frame's map; 0.25 s staleness bounds fallback frames).
static float g_sun_map_time = -1.0f;
static bool  g_sun_map_no_local = false;   // Casters exist, none within the fit.
static float g_sun_anchor_now[3] = {};
static float g_sun_map_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the union's cast bias (0 = falls back to sun_meta[2]).
static float    g_sun2_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
static bool     g_sun2_map_valid = false;
static float    g_sun3_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
static bool     g_sun3_map_valid = false;
static float    g_sun4_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
static bool     g_sun4_map_valid = false;
static float    g_sun5_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = falls back to sun_meta*[2]).
static bool     g_sun5_map_valid = false;
static bool g_sun_pf_valid[4] = { false, false, false, false };   // KH_FAR_PF: [3] the far tier.
static constexpr float KH_BAND_BIAS_TEXELS = 2.0f;
static constexpr float KH_BAND_FWD_SLACK = 1.25f;
static constexpr float KH_BAND_FWD_DROP  = 1.60f;   // re-latch tighter past this.
static uint64_t g_sun_map_hash = 0;
static float g_sun_map_bounds[6] = {};   // Combined caster AABB: center xyz, half extents xyz

// In-map casters, 6 floats each: center xyz + half extents xyz (engine axes) - uncapped.
static std::vector<float> g_sun_locals;
static int   g_sun_local_count = 0;   // Valid entries above (render thread, like the rest).
// Bumped wherever g_sun_locals is rewritten: the occupancy memo's key is an FNV over the whole array, so with a
// serial the hash runs once per rewrite instead of once per fire.
static uint64_t g_sun_locals_serial = 0;
}