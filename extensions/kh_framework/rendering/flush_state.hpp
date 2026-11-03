#pragma once

// The injection half (rendering_integration_injection.hpp): the flush's shared state: the demand flags, the active
// scope, the frame counters, the watchdog and the depth identity. Never include it on its own.
namespace RenderIntegration {
// Defined after g_flush_frame_pub, which it reads on the lock path. UI_ORDINAL_SRC: present, when given,
// says which clock answered (true = the Present count).
inline uint64_t ui_frame_ordinal(bool* present = nullptr);

// Defined in rendering/present_hook.hpp (declared here for the uses below).
inline bool present_ui_live();

// SVS_VOL_DEMAND: any visible non-fullscreen mesh that draws (an ssgi or dynamicLightFog mesh does not) exists.
// Recomputed in flush_frame's demand loop beside the UI and lit flags.
static std::atomic<bool> g_svs_mesh_wanted{false};
// FOG_PROBE: a visible fog-scatter pass or scene-chain dynamicLightFog pass (DLF) - the engine fog block's
// consumers besides the meshes (KhFsFog reads the probe-fed lanes). A visible mesh already opens the upload funnel to
// every map (cbc_on); a session of effects alone needs this term to feed the probes. Written by flush_frame's census
// (game thread), read by the upload hooks (render thread).
static std::atomic<bool> g_fogsc_wanted{false};
// DLF_DEMAND: a visible scene-chain dynamicLightFog pass - the dynamic lights' capture (dl_cpu_wanted) without
// the lit meshes' engine shadow capture (g_ls.wanted, shadow_live_wanted), which the pass never reads. Written by
// flush_frame's census (game thread; FSAA 1x stands it down as it does g_ls.wanted), read by the capture's hooks and
// the flush's fallback harvest (render thread).
static std::atomic<bool> g_dlf_wanted{false};

// Our draws are in flight: the hooks stand aside on it (process-wide, so on either thread). Raised only through
// FlushActiveScope: under the graphics lock by the two parks (park_run's flush and flush_ui_frame's lock-path
// flush_ui_locked, the render thread stalled) and, with no lock, by fx_late_gt's chain-only flush and
// infront_tail_take - inside the engine's own resolve / OM call on the thread that issued it, the render thread's
// or the game thread's (the engine never drives the immediate context from two threads at once). Atomic so a hook
// body cannot cache it.
static std::atomic<bool> g_flush_active{false};
// The flag's scope for those four raisers: cleared on every exit, exception included.
struct FlushActiveScope {
    FlushActiveScope() { g_flush_active.store(true, std::memory_order_relaxed); }
    ~FlushActiveScope() { g_flush_active.store(false, std::memory_order_relaxed); }
    FlushActiveScope(const FlushActiveScope&) = delete;
    FlushActiveScope& operator=(const FlushActiveScope&) = delete;
};
// Recursion guard for our own UI draws (the alpha clear's own Draw re-enters the hooks): the UI-thread branch, the
// mask machine and the OM hooks stand aside on it. Separate from g_ro.in_injection, which the other render-thread
// hooks key on; the alpha clear, which also runs on the render thread, raises only this flag. Also raised around the
// Present subscriber's UI flush and, by OVERLAY_BRACKET, around another module's present-time draws.
static std::atomic<bool> g_ui_mask_injecting{false};
static std::atomic<bool> g_track_wanted{false};

static uint64_t g_flush_frame = 0;
static std::atomic<uint64_t> g_flush_frame_pub{0};
// PRESENT_SEQ: the UI flush's frame clock (see g_present.seq).
inline uint64_t ui_frame_ordinal(bool* present) {
    const bool live = present_ui_live();
    if (present) *present = live;   // UI_ORDINAL_SRC.
    return live ? g_present.seq.load(std::memory_order_relaxed)
                     : g_flush_frame_pub.load(std::memory_order_relaxed);
}

// Atomic: written by the clear hook's drops (either thread: at FSAA 1x the game thread clears the main depth), the
// render thread's resolve re-adoption, the flush and the game thread's watchdog (WATCH_STALL) - each of which also
// reads it - and cleared by the session and device-state resets; read too by the named render thread's hooks (the
// OM, PIP and injection identity tests), by the late chain test (either thread) and by getRenderStats (game thread:
// "mainDepthKnown").
static std::atomic<void*> g_main_depth_identity{ nullptr };
// WATCH_STALL - the watchdog's lag test (flush_frame) compares two counters that stop together when the main
// depth is not recognised - a same-size stale identity after FSAA changes to 1x at runtime, where no scene resolve
// re-adopts it (DEPTH_READOPT) - so it reads ready while nothing of ours runs (measured: one 68 s cycle over
// ~5,500 Draw3D samples and ~44,000 engine depth clears, no park; the real 1x scene then passed for a PIP and the PIP
// injection drew the meshes into it). A watchdog sample at which the engine cleared depth since the last
// one while the published cycle did not move counts; WATCH_STALL_FRAMES of them in a row drop the identity and
// the render thread, the bootstrap state, and the park re-adopts at once (the 1x mission start's path). A screen
// that draws no 3D scene clears no depth and runs no Draw3D (measured: no sample in 26-36 s menu stretches), so
// the rule stays idle there.
static constexpr uint32_t WATCH_STALL_FRAMES = 8u;
// WATCH_STALL: the depth clears counted against the published cycle, and the stall's run.
// Threads: clears is counted by the depth-clear hook (render thread); cyc_seen, clears_seen and still are
// flush_frame's (game thread, outside any park).
// Session reset: session_globals_reset resets every member.
struct WatchState {
    std::atomic<uint64_t> clears{ 0 };   // Engine depth clears on the tracked context (the clear hook).
    uint64_t cyc_seen = ~0ull;   // Game thread: the published cycle at the watchdog's last sample.
    uint64_t clears_seen = 0;    // Game thread: g_watch.clears at that sample.
    uint32_t still = 0;          // Game thread: samples in a row with depth cleared and the cycle still.
};
static WatchState g_watch;
static UINT  g_wrong_pass_streak = 0;
static bool  g_flush_wrong_pass = false;   // WRONG_PASS_REPARK: flush_locked parked on a non-main depth; re-park.
// NO_PARK - the game thread does not take the graphics lock every frame; the render thread owns the flush (at its
// scene resolve, with the main depth bound). The park's guarantee (no hooked context call during the flush) holds
// because the injection and the flush share the render thread. Game-thread work that needs only g_draw_list_mutex
// (reaps, census) stays there, and the scene-flush ordinal stays the game thread's clock (the UI flush reads it).
// The game thread parks (park_run) only to bootstrap - until the main depth identity and the render thread are
// known - as a watchdog when the render thread has not flushed for a couple of cycles, and once for a deferred
// mission destroy, which releases device state.
static std::atomic<uint64_t> g_rtshadow_cycle{ ~0ull };   // Written render thread, read by the game thread's watchdog.

// The cycle in which a park last drew the post chain (stamped under the park just before fx_chain_run): the
// render-thread flush draws no chain in that cycle (chain_owed).
static uint64_t               g_fx_park_cycle = ~0ull;
static std::mutex             g_fog_stage_mu;   // The staged fog: written unparked on the game thread, read by either thread's publish.
// FLUSH_MISS - a park's verdict (game thread): did the injection land since the PREVIOUS flush_frame call. The
// landing serial is bumped only by a real landing, and flush_frame compares it once per call, so this is per call and
// not per park. A park's flush reads it as injected_since_last_flush; the render-thread flush takes its own witness
// (NO_PARK_LANDED).
static uint64_t g_flush_miss_seen_serial = 0;   // Game thread only.
static bool     g_flush_landed_this_frame = false;
static constexpr UINT WRONG_PASS_READOPT = 120;
}