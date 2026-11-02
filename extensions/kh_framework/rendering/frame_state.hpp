#pragma once

// The shared head (rendering_integration.hpp): the frame and camera state both halves read (the flush serial, the main
// depth, the viewport range, the camera latch). Never include it on its own.
namespace RenderIntegration {
// The live flush count the graves and the band dedupe key on (reset only by the session teardown, which first empties
// every grave and latch that holds one). Advanced by flush_frame on the game thread and read by the render thread's
// flush, so atomic; a reader one step behind holds a grave one flush longer.
static std::atomic<uint64_t> g_flush_serial{ 0 };

// sampleDesc.Count of the adopted main depth. KH_XTHREAD_ATOMIC: written by flush_locked (render thread or a
// park), read on both threads through kh_fsaa_world_standdown (the injection, the clear hook, flush_locked /
// kh_white_draw; flush_frame and kh_skin_sync on the game thread) and by getRenderStats on the game thread.
static std::atomic<uint32_t> g_scene_depth_samples{ 0 };
// By design: at FSAA 1x (single-sample main depth) every world mesh stands down - flush and injection - while
// fullscreen passes keep running; the mesh path is validated against the multisampled scene only. At 1x the game
// thread clears the main depth, so the clear hook names it the render thread, and the engine's render thread (which
// draws the 1x scene) is outside every hook's render-thread gate: the injection, PIP, in-front and seam routes,
// keyed on its scene passes, never run (measured: no injection or PIP injection at 1x), and it never flushes, so the
// watchdog parks every 1x frame; that flush draws no world mesh (the white placeholder included; its sun and
// dynamic-light maps can still render, unread). A runtime FSAA change returns to the bootstrap (KH_WATCH_STALL,
// KH_FSAA_SWITCH).
inline bool kh_fsaa_world_standdown() {
    return g_scene_depth_samples.load(std::memory_order_relaxed) == 1;
}

static const char* const KH_SHADERS_COMPILING = "shaders compiling";
static UINT g_main_depth_w = 0;
static UINT g_main_depth_h = 0;

// Depth values were written through the scene viewport depth range; UI-phase shaders must linearize with the same
// range. The fallback of every reader of g_ro.trig_vp_*; learned at the injection trigger alone (KH_SCENE_VP_TRIG);
// initialized to the measured world range (KH_INFRONT's RenderDoc note).
static float g_scene_vp_min_d = 0.011f;
static float g_scene_vp_max_d = 0.999f;
static std::atomic<float> g_sun_range{200.0f};
static std::atomic<float> g_obj_vis{0.0f};
// KH_NO_PARK_LANDED: the cycle of the last landing, stamped beside the serial. Render thread only: read by
// flush_locked's injected_since_last_flush, the DLS world pass (kh_dls_world_pass_body) and the skin cull
// (kh_skin_cull_pass).
static uint64_t g_composite_land_cycle = ~0ull;

// The clear adopts the bridge PV every cycle; no teleport debounce (a held camera puts the wrong camera into the
// cascade consensus). KH_LATCH_JUMP_M is the distance class the injection, the fire and the seam classify against.
static constexpr float    KH_LATCH_JUMP_M   = 50.0f;
static bool     g_boundary_pv_valid = false;   // The clear-time bridge fetch succeeded this cycle (it IS g_ro.cycle_pv then).
inline float latch_cam_dist_sq(const float a[3], const float b[3]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}
static float    g_khlt_lp[3] = {};
static bool     g_khlt_lp_ok = false;
}