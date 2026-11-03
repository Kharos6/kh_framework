#pragma once

// The injection half (rendering_integration_injection.hpp): the engine reset hook (on_engine_reset) and its releases.
// Never include it on its own.
namespace RenderIntegration {

// The device-death ladder shared by both reset branches: the shader batch ends with the device (no wait: the caller
// holds the graphics lock, is the render thread itself, or - on_engine_reset's best effort after three failed lock
// attempts - neither; a worker still inside D3DCompile, which holds no device object, is detached - SHADER_ORPHANS),
// then every device object and learned identity.
inline void engine_reset_release() {
    shader_mt_shutdown(false);
    g_res.release();   // THM_DEVRESET: also drops the heightfield's live flags.
    release_shadow_device_state();
    ui_mask_reset();   // Learned backbuffer identities die with the device.
    tex_cache_release_device();   // TEX_RESET_KEEP_PENDING: an entry still loading stays.
    user_shader_cache_release();
    user_lut_cache_release();
    // CLOTH: the buffers die with the device; the instances and their particle state do not, so a reset drops a
    // frame of cloth and not the drape. The workers keep running - they hold no device object.
    cloth_release_all();
    skin_release_all();   // SKEL: likewise; the next upload (the flush) re-uploads the pose.
    uvs_release_all();    // KH_USER_VS: likewise; the next flush makes them again.
}

static void __stdcall on_engine_reset() {
    // RESET_THREAD: not the game thread = the render thread itself, where taking the graphics lock would
    // deadlock. reorder_on_render_thread() cannot tell: its tid is 0 at the bootstrap and after the watchdog /
    // FSAA switch / session reset, and names the GAME thread at FSAA 1x.
    if (g_game_tid.load(std::memory_order_relaxed) != GetCurrentThreadId()) {
        engine_reset_release();
        return;
    }
    for (int attempt = 0; attempt < 3; ++attempt) {
        RVExtBridge::ScopedGraphicsLock reset_lock;
        if (!reset_lock.acquired()) continue;
        engine_reset_release();
        return;
    }
    // Best effort, as reset_render_stats_sqf: these objects belong to the dead device and cannot wait for a park.
    engine_reset_release();
}
}