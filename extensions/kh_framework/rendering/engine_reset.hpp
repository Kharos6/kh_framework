#pragma once

// The injection half (rendering_integration_injection.hpp): the engine reset hook (on_engine_reset) and its releases.
// Never include it on its own.
namespace RenderIntegration {

// The device-death ladder shared by both reset branches: no shader worker may outlive the device (no wait: the caller
// holds the graphics lock, is the render thread itself, or - on_engine_reset's best effort after three failed lock
// attempts - neither), then every device object and learned identity.
inline void kh_engine_reset_release() {
    kh_shader_mt_shutdown(false);
    g_res.release();   // KH_THM_DEVRESET: also drops the heightfield's live flags.
    release_shadow_device_state();
    kh_ui_mask_reset();   // Learned backbuffer identities die with the device.
    kh_tex_cache_release_device();   // KH_TEX_RESET_KEEP_PENDING: an entry still loading stays.
    kh_user_shader_cache_release();
    kh_user_lut_cache_release();
    // KH_CLOTH: the buffers die with the device; the instances and their particle state do not, so a reset drops a
    // frame of cloth and not the drape. The workers keep running - they hold no device object.
    kh_cloth_release_all();
    kh_skin_release_all();   // KH_SKEL: likewise; the next park re-uploads the pose.
    kh_uvs_release_all();    // KH_USER_VS: likewise; the next flush makes them again.
}

static void __stdcall on_engine_reset() {
    // KH_RESET_THREAD: not the game thread = the render thread itself, where taking the graphics lock would
    // deadlock. reorder_on_render_thread() cannot tell: its tid is 0 at the bootstrap and after the watchdog /
    // FSAA switch / session reset, and names the GAME thread at FSAA 1x.
    if (g_game_tid.load(std::memory_order_relaxed) != GetCurrentThreadId()) {
        kh_engine_reset_release();
        return;
    }
    for (int attempt = 0; attempt < 3; ++attempt) {
        RVExtBridge::ScopedGraphicsLock khr_reset_lock;
        if (!khr_reset_lock.acquired()) continue;
        kh_engine_reset_release();
        return;
    }
    // Best effort, as reset_render_stats_sqf: these objects belong to the dead device and cannot wait for a park.
    kh_engine_reset_release();
}
}