#pragma once

// The shared head (rendering_integration.hpp): PROF, the stats gate and the profiler (CPU zones, funnels, the
// see-through and GPU pass timers). Never include it on its own.
namespace RenderIntegration {

// STATS_ARMED (declared ahead of the stats block for PROF's scopes): collection is off until the first
// getRenderStats.
static std::atomic<bool> g_stats_armed{ false };
inline bool stats_on() { return g_stats_armed.load(std::memory_order_relaxed); }
// PROF: opt-in frame-time instrumentation, CPU and GPU, per main-depth cycle. Nothing here runs unless the stats
// are armed (a getRenderStats): a disarmed session pays one relaxed load per scope, no clock read, no query (the first
// fold after a disarm closes, unread, the bracket the armed cycle opened, and zeroes the armed-only accumulators once -
// PROF_DISARM).
//
// CPU zones: ProfScope takes two TSC reads (__rdtsc) around a region and adds the ticks and one entry to the zone's
// accumulators - relaxed atomics: zones run on the render thread, the game thread under the park, the UI-phase thread
// and the worker pool. The fold at the main depth clear (prof_fold) converts the accumulators to microseconds (the
// TSC's rate measured against QPC between armed folds, KH_PROF_SUB; prof_now / prof_add intervals stay on QPC)
// and publishes them with the entry counts as the ended cycle's values. PROF_TSC_CLOCK: a QPC call (~40 ns) is a
// zone's floor per timed call, as large as the work of the small hooks and x16 on a sampled zone; a TSC read is a few
// ns. The TSC zones publish whole armed cycles only (the cycle the arming splits reads 0; by the next one the rate is
// measured). Worker-pool zones (cloth, chain, skin, BVH) sum across workers - worker-microseconds, which can exceed
// the frame: divide by the count. The BVH build's zone also takes the game thread's builds (a rigid collider's first
// use, physics_bvh_for), a stall rather than worker time.
//
// GPU zones: one TIMESTAMP_DISJOINT query brackets each cycle (begun and ended at the main depth clear, on the render
// thread); each zone owns a begin / end TIMESTAMP pair per ring slot. GpuScope issues End(begin) and End(end) on the
// context it is given - the immediate context, whether the render thread or the parked game thread holds it. The fold
// reads the slot GPU_LAG cycles back with DONOTFLUSH and keeps the last value when a result is not ready, so no
// readback stalls the pipeline. Queries are created on the first armed use and released with the resources. A zone
// that ran more than once in a cycle reports its last run (a pair holds one interval); the CPU count tells how many.
enum ProfZone : uint32_t {
    // Render-thread hooks.
    PROF_HOOK_CLEAR, PROF_HOOK_DRAW, PROF_HOOK_MAP, PROF_UPLOAD_SCAN,
    // Injection and its systems.
    PROF_INJECT, PROF_ATTACH_STEP, PROF_SCENE_SYNC, PROF_OBJBUF_SYNC, PROF_SUN_LADDER,
    PROF_DLS_FRAME, PROF_DLS_WORLD, PROF_MASK_CAST, PROF_SVS_PRIME, PROF_SVS_VOL_COPY,
    PROF_VMIR, PROF_SSAO, PROF_CULL_BUILD, PROF_VIS_OCC, PROF_INST_PLAN, PROF_SNAPSHOT,
    PROF_PIP, PROF_BAND_COMMIT, PROF_THM_AUTOBUILD, PROF_DL_ACQUIRE,
    // The flushes and their systems.
    PROF_FLUSH, PROF_FLUSH_FRAME, PROF_FLUSH_UI, PROF_CLOTH_UPLOAD, PROF_SKIN_UPLOAD, PROF_SKIN_SYNC,
    // PARK_WAIT: the graphics-lock acquire alone, per park (the game thread stalled until the render thread reaches
    // the engine's grant point), for the Draw3D park and the UI driver's park; flushUiFrame is the UI park whole.
    PROF_FLUSH_LOCK_WAIT, PROF_FLUSH_UI_LOCK_WAIT, PROF_FLUSH_UI_FRAME,
    // flush_frame's pre-lock work in three pieces: the attachment reaps and the Draw3D sampler; the demand census
    // under g_draw_list_mutex; the rest up to the park decision (proxy step, chain prepare, the driver re-hoist, the
    // hook ensure).
    PROF_FLUSH_PRE, PROF_FLUSH_CENSUS, PROF_FLUSH_PREP,
    PROF_CHAIN_PREPARE, PROF_CLOTH_PROXY, PROF_PHYSICS_GATHER, PROF_MESH_GC, PROF_SCENE_CAPTURE,
    PROF_UI_MASK_DRAW,
    // Worker pool (worker-microseconds).
    PROF_W_CLOTH_SIM, PROF_W_CHAIN_SIM, PROF_W_SKIN_JOB, PROF_W_BVH_BUILD, PROF_W_BVH_REFIT,
    // Builders: lod_build (import time) and ensure_resources, called from the injection, the flushes, the seam and
    // the sun-depth paths: a call before g_res.initialized builds (or defers while shaders compile); every later one
    // takes its fast path, the mesh VB top-up.
    PROF_LOD_BUILD, PROF_ENSURE_RES,
    // PROF_HOOKS: every other installed context hook's own work (the swap chain's rare ResizeBuffers has no
    // zone), the driver's call left outside (hookUpdSub alone wraps its driver call: drvUpdSub below takes it out).
    // hookUpdSub / hookPsSrv / hookOmBlend / hookOmDss / drawRecord / hookCopyRegion / hookMisc are sampled
    // (PROF_SAMPLE); hookOmRt / hookOmRtUav / hookCopyRes / hookResolve are called far less, some calls with heavy
    // work (a PIP leave, the flush at the resolve), and are timed whole, as is drawPost (KH_PROF_TSC: its work - the
    // view-model mirror's re-issued draw, the UI quad, the bracket - lands on one draw or two a frame). The zones nest
    // and are not additive: they time every call through the hook, ours too, so work our own code issues inside a
    // zone (the flush inside hookResolve, the PIP leave inside hookOmRt / hookCopyRes, our draws inside a draw step)
    // also counts in the zones of the hooks it calls.
    PROF_HOOK_UPDSUB, PROF_HOOK_PSSRV, PROF_HOOK_OMRT, PROF_HOOK_OMRTUAV, PROF_HOOK_OMBLEND, PROF_HOOK_OMDSS,
    PROF_HOOK_COPYRES, PROF_HOOK_RESOLVE, PROF_HOOK_COPYREGION, PROF_HOOK_MISC, PROF_DRAW_RECORD, PROF_DRAW_POST,
    // PROF_DRV: the driver's own Map / Unmap (inside hookMap) and UpdateSubresource (inside hookUpdSub), timed on
    // one call in PROF_SAMPLE_EVERY of their own (prof_drv_t0) with the TSC and scaled; their N is those calls
    // x PROF_SAMPLE_EVERY (an estimate). hookMap - drvMap - drvUnmap estimates our share of hookMap only while
    // the driver's calls cost alike: the scope and the driver are sampled on different calls, so a Map that stalls in
    // the driver (waiting on the GPU) lands scaled in one estimate and not the other - a large difference, or a drvMap
    // max above hookMap's, then reads as a driver stall, not our work.
    PROF_DRV_MAP, PROF_DRV_UNMAP, PROF_DRV_UPDSUB,
    // PROF_UPLOAD: the funnel's steps for each captured upload (ProfSub; one decision per upload, timed one in
    // PROF_SAMPLE_EVERY): the capture (the copy), then each scanner, then the replay's image note.
    PROF_UP_CAPTURE, PROF_SC_PROJ, PROF_SC_LIVE, PROF_SC_LOCATOR, PROF_SC_PIP, PROF_SC_ENGCAM, PROF_SC_REG,
    PROF_SC_VIEW, PROF_SC_B2, PROF_SC_RPIMG,
    // PROF_DRAWSTEP: reorder_pre_draw's steps for the engine's draws (ProfSub; one decision per draw, our own
    // draws take none): dsShadowNote / dsMaskNote / dlSample sampled; timed whole the ones whose work lands on a few
    // draws a frame (dsSeamPump, dsPip, dsInfront: the seam / svs, PIP and in-front injections), or one draw in 8 or
    // 16, or a retry (dsCascbind, dsSkybind, dsLatch). mask cast, band commit, the UI mask and the injection keep
    // their own zones above.
    PROF_DS_SEAM, PROF_DS_PIP, PROF_DS_SHADOW, PROF_DS_MASK, PROF_DS_DL, PROF_DS_INFRONT, PROF_DS_CASCBIND,
    PROF_DS_SKYBIND, PROF_DS_LATCH,
    // WSEE - the see-through record's CPU zones. dsWsee: the pre-draw's step for the engine's draws in a wanted
    // cycle (wsee_pre_draw: the state reads and the capture; sampled, as dsShadowNote). wseePost: the re-issues
    // (wsee_post, inside the draw hooks' post: the saves, the passes' draws and the restore), timed whole with the
    // TSC as drawPost is, which it nests in.
    PROF_DS_WSEE, PROF_WSEE_POST,
    // wseeClear: the see-through work at each main-depth clear (wsee_cycle_note, psv_cycle_head's variant
    // compiles - PSV_MAKE_N entries at most, up to three driver compiles each -, wsee_gput_head), nested in
    // hookClear. wseeCapture: the capture and the B2 / S2 moment (wsee_snapshot, wsee_b2_take: once a cycle
    // each, refusals counted too), nested in dsWsee or in inject (the injection tail's capture). Both KH_PROF_TSC.
    PROF_WSEE_CLEAR, PROF_WSEE_CAP,
    PROF_ZONE_N
};
static const char* const g_prof_zone_name[] = {
    "hookClear", "hookDraw", "hookMap", "uploadScan",
    "inject", "attachStep", "sceneSync", "objbufSync", "sunLadder",
    "dlsFrame", "dlsWorld", "maskCast", "svsPrime", "svsVolCopy",
    "vmir", "ssao", "cullBuild", "visOcc", "instPlan", "snapshot",
    "pip", "bandCommit", "thmAutobuild", "dlAcquire",
    "flush", "flushFrame", "flushUi", "clothUpload", "skinUpload", "skinSync",
    "flushLockWait", "flushUiLockWait", "flushUiFrame",
    "flushPre", "flushCensus", "flushPrep",
    "chainPrepare", "clothProxy", "physicsGather", "meshGc", "sceneCapture",
    "uiMaskDraw",
    "wClothSim", "wChainSim", "wSkinJob", "wBvhBuild", "wBvhRefit",
    "lodBuild", "ensureRes",
    "hookUpdSub", "hookPsSrv", "hookOmRt", "hookOmRtUav", "hookOmBlend", "hookOmDss",
    "hookCopyRes", "hookResolve", "hookCopyRegion", "hookMisc", "drawRecord", "drawPost",
    "drvMap", "drvUnmap", "drvUpdSub",
    "upCapture", "scProj", "scShadowLive", "scLocator", "scPip", "scEngcam", "scShadowReg", "scShadowView",
    "scB2", "scRpImg",
    "dsSeamPump", "dsPip", "dsShadowNote", "dsMaskNote", "dlSample", "dsInfront", "dsCascbind",
    "dsSkybind", "dsLatch",
    "dsWsee", "wseePost", "wseeClear", "wseeCapture",   // WSEE.
};
static_assert(sizeof(g_prof_zone_name) / sizeof(g_prof_zone_name[0]) == PROF_ZONE_N, "PROF: a name per zone");
// The CPU profiler (PROF): the zones' ticks and counts, the published values, the sampling counters, the upload and
// draw sub-steps, the window and the TSC calibration.
// Threads: the zone accumulators (ticks, count, sample_ctr, tsc, xb_acc) are atomics bumped wherever a PROF scope
// runs (any thread); prof_fold (the render thread) folds them into the published values and the window, which
// getRenderStats reads and resetRenderStats zeroes on the game thread; the upload / draw sub-step samplers (up_*,
// ds_*) and the calibration (cal_*) are the render thread's.
// Session reset: session_globals_reset resets 11 of them. reset_stat_counters resets 1 of them.
struct ProfState {
    std::atomic<uint64_t> ticks[PROF_ZONE_N];
    std::atomic<uint64_t> count[PROF_ZONE_N];
    std::atomic<uint64_t> us_pub[PROF_ZONE_N];
    std::atomic<uint64_t> n_pub[PROF_ZONE_N];
    // PROF_DISARM: resetRenderStats' count (reset_stat_counters bumps it) and the one the last fold saw (render
    // thread). A fold that finds collection off, or a reset since the fold before it, publishes zeros - CPU zones and
    // GPU times alike - and reads no GPU slot: the accumulators and the query ring hold pre-reset measurements, and the
    // first getRenderStats after a reset reports zero. The fold itself zeroes the GPU times, not only the reset: a
    // harvest that raced a reset made without the graphics lock (resetRenderStats' fallback) can store pre-reset times
    // after the reset zeroed them, and no disarmed fold harvests again.
    std::atomic<uint32_t> epoch{ 0 };
    uint32_t epoch_seen = 0;
    std::atomic<uint32_t> sample_ctr[PROF_ZONE_N];   // hooked_map's scope opens before its thread gate: any thread.
    // PROF_TSC_CLOCK: the zones' TSC ticks (ProfScope, KH_PROF_TSC, PROF_DRV with atomic adds, any thread;
    // ProfSub with plain adds, render thread - no zone is written both ways), converted by the fold (KH_PROF_SUB).
    std::atomic<uint64_t> tsc[PROF_ZONE_N];
    // KH_PROF_SUB - the steps of one captured upload (PROF_UPLOAD) and of one draw (PROF_DRAWSTEP). Tens of
    // thousands of entries a frame, so a QPC pair on each, or an atomic read-modify-write, would itself move the frame:
    // one decision per upload / per draw (prof_up_begin / ProfDsDecision: armed, and one in PROF_SAMPLE_EVERY
    // timed), then each step counts with a plain add and, on a timed one, reads the TSC (__rdtsc) twice - scaled by its
    // rate (scale: the sample rate, or 1 for a step timed whole). The fold converts the ticks with the TSC's rate
    // it measured against QPC since the last armed fold (an invariant TSC, as QPC itself uses where it has one -
    // without one, a CPU older than invariant TSCs, a zone is scaled by the interval's average rate: a measurement
    // error only; a read that runs backwards - cores whose TSCs disagree - drops that sample). Written only by the
    // render-thread identity (the hooks past reorder_on_render_thread, the funnel behind upload_hook_wanted) and
    // folded on it: relaxed loads and stores, no read-modify-write - a race would miscount, not corrupt.
    ProfZone up_cur = PROF_ZONE_N;   // The step in flight (scanExtBytes' owner); PROF_ZONE_N = none.
    // The decisions (render-thread identity; PROF_SAMPLE_HASH). prof_up_begin: at the head of upload_capture,
    // the funnel's first step for every captured upload (Unmap and UpdateSubresource alike); ProfDsDecision: past
    // reorder_pre_draw's render-thread gate, for the rest of the draw. Disarmed: one relaxed load and two plain stores
    // each (the bools), and every step (rendering_integration_injection.hpp) reads the bool.
    uint32_t up_ctr = 0;
    bool     up_on = false;
    bool     up_t = false;
    uint32_t ds_ctr = 0;
    bool     ds_on = false;
    bool     ds_t = false;
    // scanExtBytes: the extension bytes each step made the funnel copy (the step in flight, g_prof.up_cur).
    std::atomic<uint64_t> xb_acc[PROF_ZONE_N];
    std::atomic<uint64_t> xb_pub[PROF_ZONE_N];
    std::atomic<uint64_t> xb_win[PROF_ZONE_N];
    // PROF_WINDOW - one cycle is one sample (a 1-in-16 zone multiplies one interrupted call by 16; a step that runs
    // once a frame lands on the cycle or not), so every whole armed cycle (a published fold, PROF_DISARM, whose
    // previous fold was armed too) also adds to these: each zone's mean and worst cycle since the arming or the last
    // resetRenderStats, for getRenderStats.
    std::atomic<uint64_t> win_cycles{ 0 };
    std::atomic<uint64_t> win_us[PROF_ZONE_N];
    std::atomic<uint64_t> win_n[PROF_ZONE_N];
    std::atomic<uint64_t> win_max[PROF_ZONE_N];
    // KH_PROF_SUB's rate: TSC ticks per microsecond, measured at each armed fold over the interval since the previous
    // one - the conversion of every TSC-timed zone (every CPU zone but prof_add's QPC intervals). 0 until two armed
    // folds have run in this process (those zones publish 0 until then); kept across resets (the machine's rate,
    // measured again by every armed fold). Render-thread identity writes.
    uint64_t cal_q = 0;
    uint64_t cal_t = 0;
    std::atomic<double> tsc_per_us{ 0.0 };
    // PROF_WINDOW: the last fold found collection on (render thread). The window takes whole armed cycles only: the
    // cycle the arming split stays out of the means (its fold publishes the counts and prof_add's QPC zones -
    // nothing at all when it is also the first fold after a reset, PROF_DISARM; the TSC zones, scanExtBytes and the
    // funnel read 0 there).
    bool prev_on = false;
};
static ProfState g_prof;
inline uint64_t prof_freq() {
    static const uint64_t pf = [] { LARGE_INTEGER f = {}; QueryPerformanceFrequency(&f); return f.QuadPart > 0 ? static_cast<uint64_t>(f.QuadPart) : 1ull; }();
    return pf;
}
// PROF_SAMPLE: the zones entered tens of thousands of times a frame (PROF_HOOK_MAP on every Map and Unmap,
// PROF_UPLOAD_SCAN on every scanned upload, and the hooks PROF_HOOKS adds: prof_sampled's set) take one clock
// pair in PROF_SAMPLE_EVERY entries and scale the ticks; the count stays exact. Two QPC calls on ~50,000 entries a
// frame were ~4 ms of the armed hook figures and most of the armed-vs-unarmed frame gap.
static constexpr uint32_t PROF_SAMPLE_EVERY = 16u;
static_assert((PROF_SAMPLE_EVERY & (PROF_SAMPLE_EVERY - 1u)) == 0u, "PROF_SAMPLE: a power of two");
// PROF_HOOKS: the hooks the engine calls thousands of times a frame join the sampled set.
inline bool prof_sampled(ProfZone z) {
    switch (z) {
    case PROF_HOOK_MAP: case PROF_UPLOAD_SCAN: case PROF_HOOK_UPDSUB: case PROF_HOOK_PSSRV: case PROF_HOOK_OMBLEND:
    case PROF_HOOK_OMDSS: case PROF_HOOK_COPYREGION: case PROF_HOOK_MISC: case PROF_DRAW_RECORD:
        return true;
    default:
        return false;
    }
}
// PROF_SAMPLE_HASH: which calls are timed - a mix of the zone's call counter, not the counter's own residue. Do not
// use the residue: it locks onto call patterns (hookMap's Map and Unmap strictly alternate on one counter, so every
// timed call is an Unmap, or every one a Map). salt draws further decisions from the same count (the driver's
// timing, PROF_DRV) independent of the first, so one's cost lands in the other's samples at its true rate.
inline uint32_t prof_mix(uint32_t x, uint32_t salt) {
    x ^= salt * 0x9E3779B9u;
    x ^= x >> 16; x *= 0x7FEB352Du;
    x ^= x >> 15; x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}
inline bool prof_pick(uint32_t c, uint32_t salt) {
    return (prof_mix(c, salt) & (PROF_SAMPLE_EVERY - 1u)) == 0u;
}
struct ProfScope {
    ProfZone z;
    uint64_t   t0;   // PROF_TSC_CLOCK: the TSC at the entry.
    bool       on;
    bool       timed;
    uint32_t   c;   // PROF_SAMPLE_HASH: a sampled zone's call count (0 = none drawn).
    explicit ProfScope(ProfZone zone)
        : z(zone), t0(0), on(stats_on()), timed(false), c(0) {
        if (!on) return;
        if (prof_sampled(zone)) {
            c = g_prof.sample_ctr[zone].fetch_add(1, std::memory_order_relaxed) + 1u;
            timed = prof_pick(c, 0u);
        } else {
            timed = true;
        }
        if (timed) t0 = __rdtsc();
    }
    ~ProfScope() {
        if (!on) return;
        g_prof.count[z].fetch_add(1, std::memory_order_relaxed);
        if (!timed) return;
        const uint64_t t1 = __rdtsc();
        if (t1 <= t0) return;   // PROF_TSC_CLOCK: a backwards read (unsynchronised cores) is dropped.
        const uint64_t dt = t1 - t0;
        g_prof.tsc[z].fetch_add(prof_sampled(z) ? dt * PROF_SAMPLE_EVERY : dt,
                                std::memory_order_relaxed);
    }
    ProfScope(const ProfScope&) = delete;
    ProfScope& operator=(const ProfScope&) = delete;
};
#define KH_PROF_SCOPE(z) ProfScope prof_scope_##z(z)
// An explicit interval for a region a scope cannot bracket (a constructor's own body: the graphics lock's acquire).
// Same accounting, armed only.
inline int64_t prof_now() { if (!stats_on()) return 0; LARGE_INTEGER q = {}; QueryPerformanceCounter(&q); return q.QuadPart; }
inline void prof_add(ProfZone z, int64_t t0) {
    if (!stats_on() || t0 == 0) return;
    LARGE_INTEGER q = {}; QueryPerformanceCounter(&q);
    g_prof.ticks[z].fetch_add(static_cast<uint64_t>(q.QuadPart - t0), std::memory_order_relaxed);
    g_prof.count[z].fetch_add(1, std::memory_order_relaxed);
}
inline void prof_plain_add(std::atomic<uint64_t>& a, uint64_t n) {
    a.store(a.load(std::memory_order_relaxed) + n, std::memory_order_relaxed);
}
// PROF_DRV: the driver's own call inside a sampled hook scope (s), timed on one call in
// PROF_SAMPLE_EVERY of its own (a second decision from the scope's count) with the TSC, and scaled. Its own
// decision, not the scope's: timed on the scope's timed calls, its cost would sit in every call the scope extrapolates.
// Any thread (the hooks'), so atomics; 0 = not timed (nothing recorded).
inline uint64_t prof_drv_t0(const ProfScope& s) {
    if (s.c == 0u || !prof_pick(s.c, 1u)) return 0u;
    return __rdtsc();
}
inline void prof_drv(ProfZone z, uint64_t t0) {
    if (t0 == 0u) return;
    const uint64_t t1 = __rdtsc();
    if (t1 <= t0) return;   // PROF_TSC_CLOCK: a backwards read is dropped.
    g_prof.tsc[z].fetch_add((t1 - t0) * PROF_SAMPLE_EVERY, std::memory_order_relaxed);
    g_prof.count[z].fetch_add(PROF_SAMPLE_EVERY, std::memory_order_relaxed);
}
// KH_PROF_TSC: a zone timed whole with the TSC, any thread - for a hook called on every draw whose work lands on a
// few of them (a 1-in-16 sample would read 0 or 16x a frame): two TSC reads and two atomic adds per armed call.
struct ProfTscScope {
    ProfZone z;
    uint64_t   t0;
    bool       on;
    explicit ProfTscScope(ProfZone zone) : z(zone), t0(0), on(stats_on()) {
        if (on) t0 = __rdtsc();
    }
    ~ProfTscScope() {
        if (!on) return;
        g_prof.count[z].fetch_add(1, std::memory_order_relaxed);
        const uint64_t t1 = __rdtsc();
        if (t1 > t0) g_prof.tsc[z].fetch_add(t1 - t0, std::memory_order_relaxed);
    }
    ProfTscScope(const ProfTscScope&) = delete;
    ProfTscScope& operator=(const ProfTscScope&) = delete;
};
#define KH_PROF_TSC(z) ProfTscScope tsc_scope_##z(z)
struct ProfSub {
    ProfZone z;
    ProfZone prev;
    uint64_t   t0;
    uint64_t   scale;
    bool       on;
    bool       timed;
    ProfSub(ProfZone zone, bool armed, bool pb_time, uint64_t rate)
        : z(zone), prev(PROF_ZONE_N), t0(0), scale(rate), on(armed),
          timed(armed && pb_time) {
        if (!on) return;
        prof_plain_add(g_prof.count[z], 1u);
        prev = g_prof.up_cur;
        g_prof.up_cur = z;
        if (timed) t0 = __rdtsc();
    }
    ~ProfSub() {
        if (!on) return;
        if (timed) {
            const uint64_t t1 = __rdtsc();
            if (t1 > t0) prof_plain_add(g_prof.tsc[z], (t1 - t0) * scale);
        }
        g_prof.up_cur = prev;
    }
    ProfSub(const ProfSub&) = delete;
    ProfSub& operator=(const ProfSub&) = delete;
};
// A sampled step (timed on the upload's / draw's decision), and a step timed whole whenever it runs armed.
#define KH_PROF_SUB(z, on, timed) ProfSub prof_sub_##z((z), (on), (timed), PROF_SAMPLE_EVERY)
#define KH_PROF_SUB_ALL(z, on) ProfSub prof_sub_##z((z), (on), true, 1u)
inline void prof_up_begin() {
    g_prof.up_on = stats_on();
    g_prof.up_t = g_prof.up_on && prof_pick(++g_prof.up_ctr, 2u);
}
// own: one of our own draws (g_ro.in_injection) - not one of the engine's: no decision of its own, nothing
// counted, and the engine draw it is issued inside (a seam / PIP / in-front injection) gets its decision back.
struct ProfDsDecision {
    bool own;
    bool on;
    bool t;
    explicit ProfDsDecision(bool own_draw) : own(own_draw), on(false), t(false) {
        const bool armed = stats_on();
        if (own && armed) {
            on = g_prof.ds_on;
            t = g_prof.ds_t;
            g_prof.ds_on = false;
            g_prof.ds_t = false;
            return;
        }
        own = false;
        g_prof.ds_on = armed;
        g_prof.ds_t = armed && prof_pick(++g_prof.ds_ctr, 3u);
    }
    ~ProfDsDecision() {
        if (!own) return;
        g_prof.ds_on = on;
        g_prof.ds_t = t;
    }
    ProfDsDecision(const ProfDsDecision&) = delete;
    ProfDsDecision& operator=(const ProfDsDecision&) = delete;
};
// PROF_FUNNEL - counts and bytes per cycle (render-thread identity, armed only; plain adds as ProfSub's). The
// upload funnel: captured uploads (UpdateSubresource's among them), the eager copies and bytes, the write-combined
// fences, the extensions past the eager copy (upload_need / upload_copy_range) and their bytes, the light
// capture's whole copies (cbs_write) and bytes, in all and by what the sampler named the buffer (the want table's
// kind, tagged while armed: cb11, cb10, a VS slot, none yet), uploads the want table refused (a shadow, if any,
// marked stale), captures the idle cadence skipped, and the want table's index slots probed. The light sampler: the
// engine's draws past the render-thread gate, draws with no cb11 bound, uploads already processed this span, shadow
// misses (an incomplete side), twins of an authored window, new lists hashed and the bytes hashed, matches run and
// replayed (DL_PRES_MEMO) and the record x pool-light pairs the runs stood against, windows authored and the VS
// bytes they copied; then the pairs the runs visited (DL_PRES_HIX: the lights a record's height can match, or the
// whole pool), the height index's rebuilds and its in-place moves (a presence rewrite that moved a light's height).
// MAP_MIX: the captured maps (UpdateSubresource's not among them) by kind - whole-buffer (WRITE_DISCARD) and
// partial (WRITE_NO_OVERWRITE) - each with the buffers' full ByteWidth summed (what copying the buffer whole at Unmap
// would move), and the whole-buffer maps by ByteWidth: <= 256, <= 1 KiB, <= 4 KiB, <= 16 KiB, larger.
// MAP_REDIRECT: the whole-buffer maps handed a cached buffer and their ByteWidth (the bytes the Unmap copies to the
// driver's pointer), the whole-buffer maps with a pending slot that were not (no free record, no cached buffer to
// give, wider than the cap, the hooks not all installed, or MAP_REDIRECT_ON false), and the cached buffers
// (re)assigned to a buffer (a first map, a new width, or the table's churn). MAP_BRANCH: the Map and Unmap hooks'
// per-call branches - Maps through the hook, those whose forget (REPLAY_CBREUSE) took g_rp_cbkeep_mx, those
// REPLAY_SPLIT looked up in its set and those it split, those that scanned the redirect records (redir_drop),
// those the upload gate admitted, and of these the ones given a pending slot and the ones finding every slot held;
// Unmaps through the hook, those that walked the pending slots, those that found theirs, those that scanned the
// redirect records (redir_close); and the forgets that took the mutex from any hook. Counted on the hook scope's
// armed snapshot (on) - the forgets' own count excepted: rp_cb_forget has callers with no such scope and
// loads stats_on() on its locked path, so on the call that straddles an arm or a disarm it and mapReplayLocks can
// differ by one. Behind the render-thread gate, plain adds as above; elsewhere any thread, so atomic adds (fun_any).
enum Funnel : uint32_t {
    FUNNEL_UPLOADS, FUNNEL_UPDSUB, FUNNEL_EAGER_N, FUNNEL_EAGER_B, FUNNEL_FENCES, FUNNEL_EXT_N, FUNNEL_EXT_B,
    FUNNEL_SHADOW_N, FUNNEL_SHADOW_B, FUNNEL_SH11_N, FUNNEL_SH11_B, FUNNEL_SH10_N, FUNNEL_SH10_B, FUNNEL_SHVS_N,
    FUNNEL_SHVS_B, FUNNEL_SHOTHER_N, FUNNEL_SHOTHER_B, FUNNEL_WANT_REFUSED, FUNNEL_IDLE_SKIP, FUNNEL_GATE_PROBES,
    FUNNEL_DRAWS, FUNNEL_DL_NOLIST, FUNNEL_DL_SAME, FUNNEL_DL_MISS, FUNNEL_DL_TWIN, FUNNEL_DL_LISTS, FUNNEL_DL_HASH_B,
    FUNNEL_DL_RUNS, FUNNEL_DL_REPLAYS, FUNNEL_DL_PAIRS, FUNNEL_DL_WINDOWS, FUNNEL_DL_WIN_VS_B,
    FUNNEL_DL_VISITS, FUNNEL_DL_HIX_BUILDS, FUNNEL_DL_HIX_MOVES,   // DL_PRES_HIX.
    FUNNEL_MAP_DISC_N, FUNNEL_MAP_DISC_B, FUNNEL_MAP_NOOV_N, FUNNEL_MAP_NOOV_B,   // MAP_MIX.
    FUNNEL_MAP_W256, FUNNEL_MAP_W1K, FUNNEL_MAP_W4K, FUNNEL_MAP_W16K, FUNNEL_MAP_WBIG,
    FUNNEL_REDIR_N, FUNNEL_REDIR_B, FUNNEL_REDIR_REFUSED, FUNNEL_REDIR_BUFS,   // MAP_REDIRECT.
    FUNNEL_HK_MAP_N, FUNNEL_HK_RP_LOCK, FUNNEL_HK_RP_CHECK, FUNNEL_HK_RP_SPLIT, FUNNEL_HK_RD_DROP,   // MAP_BRANCH.
    FUNNEL_HK_ADMIT, FUNNEL_HK_PENDING, FUNNEL_HK_NOSLOT,
    FUNNEL_HK_UNMAP_N, FUNNEL_HK_UN_WALK, FUNNEL_HK_UN_HIT, FUNNEL_HK_RD_CLOSE, FUNNEL_RP_LOCKS,
    FUNNEL_N
};
static const char* const g_fun_name[] = {
    "uploads", "updSubUploads", "eagerCopies", "eagerBytes", "wcFences", "extCopies", "extBytes",
    "shadowCopies", "shadowBytes", "shadowCb11Copies", "shadowCb11Bytes", "shadowCb10Copies", "shadowCb10Bytes",
    "shadowVsCopies", "shadowVsBytes", "shadowOtherCopies", "shadowOtherBytes", "wantRefused", "idleSkips",
    "gateProbes",
    "draws", "dlNoList", "dlSameUpload", "dlShadowMiss", "dlTwin", "dlNewLists", "dlHashBytes",
    "dlMatchRuns", "dlMatchReplays", "dlMatchPairs", "dlWindows", "dlWindowVsBytes",
    "dlMatchVisited", "dlHeightIxBuilds", "dlHeightIxMoves",
    "mapDiscard", "mapDiscardBytes", "mapNoOverwrite", "mapNoOverwriteBytes",
    "mapW256", "mapW1k", "mapW4k", "mapW16k", "mapWBig",
    "mapRedirected", "mapRedirectedBytes", "mapRedirectRefused", "mapRedirectBuffers",
    "mapCalls", "mapReplayLocks", "mapReplayChecks", "mapReplaySplits", "mapRedirectDrops",
    "mapAdmitted", "mapPending", "mapNoSlot",
    "unmapCalls", "unmapSlotWalks", "unmapSlotHits", "unmapRedirectCloses", "replayLocks",
};
static_assert(sizeof(g_fun_name) / sizeof(g_fun_name[0]) == FUNNEL_N, "PROF_FUNNEL: a name per counter");
static std::atomic<uint64_t> g_fun_acc[FUNNEL_N];   // This cycle's (render-thread identity).
static std::atomic<uint64_t> g_fun_pub[FUNNEL_N];   // The ended cycle's (the fold).
static std::atomic<uint64_t> g_fun_win[FUNNEL_N];   // Summed over the window (PROF_WINDOW).
inline void fun_add(Funnel k, uint64_t n) { prof_plain_add(g_fun_acc[k], n); }
// MAP_BRANCH: a count made off the render-thread gate (any thread): an atomic add, armed callers only, and only to
// a counter fun_add never adds to.
inline void fun_any(Funnel k, uint64_t n) {
    g_fun_acc[k].fetch_add(n, std::memory_order_relaxed);
}
// The want table's kinds (dl_want_note's tag; a buffer named in more than one role carries each bit).
static constexpr uint8_t DL_KIND_CB11 = 1u;
static constexpr uint8_t DL_KIND_CB10 = 2u;
static constexpr uint8_t DL_KIND_VS = 4u;
inline void fun_shadow(uint8_t kind, uint64_t b) {
    fun_add(FUNNEL_SHADOW_N, 1u);
    fun_add(FUNNEL_SHADOW_B, b);
    if (kind & DL_KIND_CB11)     { fun_add(FUNNEL_SH11_N, 1u); fun_add(FUNNEL_SH11_B, b); }
    else if (kind & DL_KIND_CB10) { fun_add(FUNNEL_SH10_N, 1u); fun_add(FUNNEL_SH10_B, b); }
    else if (kind & DL_KIND_VS)   { fun_add(FUNNEL_SHVS_N, 1u); fun_add(FUNNEL_SHVS_B, b); }
    else                                  { fun_add(FUNNEL_SHOTHER_N, 1u); fun_add(FUNNEL_SHOTHER_B, b); }
}
// MAP_MIX: one captured map - its kind and its buffer's full ByteWidth (armed callers only).
inline void fun_map_mix(bool discard, uint32_t w) {
    if (!discard) {
        fun_add(FUNNEL_MAP_NOOV_N, 1u);
        fun_add(FUNNEL_MAP_NOOV_B, w);
        return;
    }
    fun_add(FUNNEL_MAP_DISC_N, 1u);
    fun_add(FUNNEL_MAP_DISC_B, w);
    if (w <= 256u)        fun_add(FUNNEL_MAP_W256, 1u);
    else if (w <= 1024u)  fun_add(FUNNEL_MAP_W1K, 1u);
    else if (w <= 4096u)  fun_add(FUNNEL_MAP_W4K, 1u);
    else if (w <= 16384u) fun_add(FUNNEL_MAP_W16K, 1u);
    else                       fun_add(FUNNEL_MAP_WBIG, 1u);
}
inline void fun_ext(uint64_t b) {
    fun_add(FUNNEL_EXT_N, 1u);
    fun_add(FUNNEL_EXT_B, b);
    if (g_prof.up_cur < PROF_ZONE_N) prof_plain_add(g_prof.xb_acc[g_prof.up_cur], b);
}

// WSEE_GPUT - GPU timing of the see-through record's own work (stats alone: every write under stats_on; nothing
// drawn or decided reads it). GPU_WSEE / GPU_WSEE_B2 time the capture and B2 alone - the re-issues run inside the
// engine's draw stream and no zone brackets them. One frame in WSEE_GPUT_EVERY while armed and capturing (cycle %
// 16 == WSEE_GPUT_AT) ends a timestamp at each re-issue's save and restore (wsee_re_save / wsee_re_restore),
// before each particle-type pass (wsee_re_set) and around the capture, B2, our tail, the chain's E draw and D3's
// resolve (WseeGputScope); the frame is one disjoint query from its clear to the next. Read back at later clears
// without a flush or a wait (at most WSEE_GPUT_READ stamps a clear; a frame not read whole within WSEE_GPUT_WAIT
// clears is dropped; a ring of WSEE_GPUT_RING frames). Per kind: the sum of its intervals in the frame and their
// count (getRenderStats "wseeGpu"). An interval is elapsed GPU time - the pipeline's drain and refill around the draws
// included, never less than their own time: a sum over many short intervals over-states their marginal cost, and the
// probe frame runs slower by its stamps. A block that would not fit (WSEE_GPUT_N stamps a frame, eight kept for a
// block's own) or that opens inside another goes untimed (counted). The chain's E draw and the resolve stamp on the
// chain's thread (the render thread, a park that stalls it, or the game thread's late chain inside a resolve the
// engine issues there: the engine never drives the immediate context from two threads at once). Released with the
// PROF queries (gpu_prof_release: the device, the session); a refused make latches until then. The first probe
// frame makes 8,194 queries inside the main depth's clear hook (a render-thread hitch of a few ms, once per session
// and device reset); the clears after a probe frame issue up to WSEE_GPUT_READ GetData calls each. WSEE_GPU_P_OTHER: a
// pass without a kind of its own: wsee_gput_pk's catch-all (none today: every particle-type pass has its own,
// and the one-pass kinds - a late opaque, a fence-type layer - take no pass stamp).
enum WseeGputKind : uint32_t {
    WSEE_GPU_OPQ_POST, WSEE_GPU_FENCE, WSEE_GPU_PART, WSEE_GPU_P_T, WSEE_GPU_P_NW, WSEE_GPU_P_TP, WSEE_GPU_P_TP2,
    WSEE_GPU_P_TF, WSEE_GPU_P_OTHER, WSEE_GPU_TAIL, WSEE_GPU_CAPTURE, WSEE_GPU_B2, WSEE_GPU_CHAIN_E, WSEE_GPU_D3RES,
    WSEE_GPU_P_ONE,   // PSB: the one pass (a particle-type re-issue drawn once).
    WSEE_GPU_N
};
static const char* const g_wsee_gput_name[WSEE_GPU_N] = {
    "opqPost", "fence", "particle", "pT", "pNW", "pTp", "pTp2", "pTf",
    "pOther", "tail", "capture", "b2", "chainE", "d3Resolve",
    "pOne",
};
static constexpr uint32_t WSEE_GPUT_N = 4096u;
static constexpr uint32_t WSEE_GPUT_RING = 2u;
static constexpr uint32_t WSEE_GPUT_READ = 1024u;
static constexpr uint64_t WSEE_GPUT_EVERY = 16u;
static constexpr uint64_t WSEE_GPUT_AT = 8u;
static constexpr uint64_t WSEE_GPUT_WAIT = 14u;
// A stamp's event: the kind in the low six bits, the role above (a block's begin, a pass's start, a block's end).
static constexpr uint8_t WSEE_GPU_EV_BEGIN = 0x00u, WSEE_GPU_EV_PASS = 0x40u, WSEE_GPU_EV_END = 0x80u;
struct WseeGput {
    ID3D11Query* dj[WSEE_GPUT_RING] = {};
    ID3D11Query* ts[WSEE_GPUT_RING][WSEE_GPUT_N] = {};
    uint8_t      ev[WSEE_GPUT_RING][WSEE_GPUT_N] = {};
    uint64_t     tv[WSEE_GPUT_RING][WSEE_GPUT_N] = {};   // The values read back.
    uint32_t     n[WSEE_GPUT_RING] = {};      // Stamps ended in the slot's frame.
    uint32_t     rd[WSEE_GPUT_RING] = {};     // Of them read back.
    uint64_t     cyc[WSEE_GPUT_RING] = {};
    uint64_t     freq[WSEE_GPUT_RING] = {};   // The disjoint query's (0 = not read yet).
    bool         wait[WSEE_GPUT_RING] = {};
    int          open = -1;    // The slot recording (this frame), -1 = none.
    uint32_t     next = 0;     // The slot the next probe frame takes.
    int          blk = -1;     // The open block's begin stamp in the open slot, -1 = none.
    bool         ready = false;
    bool         failed = false;
};
static WseeGput g_wsee_gput;
// Published (the harvest writes, getRenderStats reads): per kind the last frame read's ns and intervals, and the sums
// over every frame read since the arming / resetRenderStats; the frames read, dropped, and the blocks left untimed.
static std::atomic<uint64_t> g_wsee_gput_ns_last[WSEE_GPU_N];
static std::atomic<uint64_t> g_wsee_gput_n_last[WSEE_GPU_N];
static std::atomic<uint64_t> g_wsee_gput_ns_sum[WSEE_GPU_N];
static std::atomic<uint64_t> g_wsee_gput_n_sum[WSEE_GPU_N];
static std::atomic<uint64_t> g_wsee_gput_read{ 0 };
static std::atomic<uint64_t> g_wsee_gput_drop{ 0 };
static std::atomic<uint64_t> g_wsee_gput_untimed{ 0 };
inline void wsee_gput_zero_pub() {   // resetRenderStats (reset_stat_counters).
    for (uint32_t k = 0; k < WSEE_GPU_N; ++k) {
        g_wsee_gput_ns_last[k].store(0, std::memory_order_relaxed);
        g_wsee_gput_n_last[k].store(0, std::memory_order_relaxed);
        g_wsee_gput_ns_sum[k].store(0, std::memory_order_relaxed);
        g_wsee_gput_n_sum[k].store(0, std::memory_order_relaxed);
    }
    g_wsee_gput_read.store(0, std::memory_order_relaxed);
    g_wsee_gput_drop.store(0, std::memory_order_relaxed);
    g_wsee_gput_untimed.store(0, std::memory_order_relaxed);
}
inline void wsee_gput_release() {
    for (uint32_t s = 0; s < WSEE_GPUT_RING; ++s) {
        KH_SAFE_RELEASE(g_wsee_gput.dj[s]);
        for (ID3D11Query*& q : g_wsee_gput.ts[s]) KH_SAFE_RELEASE(q);
        g_wsee_gput.n[s] = g_wsee_gput.rd[s] = 0;
        g_wsee_gput.freq[s] = 0;
        g_wsee_gput.wait[s] = false;
    }
    g_wsee_gput.open = -1;
    g_wsee_gput.next = 0;
    g_wsee_gput.blk = -1;
    g_wsee_gput.ready = false;
    g_wsee_gput.failed = false;
}
inline bool wsee_gput_ensure(ID3D11DeviceContext* ctx) {
    if (g_wsee_gput.ready) return true;
    if (g_wsee_gput.failed || !ctx) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    bool ok = dev != nullptr;
    D3D11_QUERY_DESC dd = {};
    dd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    D3D11_QUERY_DESC td = {};
    td.Query = D3D11_QUERY_TIMESTAMP;
    for (uint32_t s = 0; s < WSEE_GPUT_RING && ok; ++s) {
        ok = SUCCEEDED(dev->CreateQuery(&dd, &g_wsee_gput.dj[s]));
        for (uint32_t i = 0; i < WSEE_GPUT_N && ok; ++i) {
            ok = SUCCEEDED(dev->CreateQuery(&td, &g_wsee_gput.ts[s][i]));
        }
    }
    if (dev) dev->Release();
    if (!ok) {
        wsee_gput_release();
        g_wsee_gput.failed = true;
        return false;
    }
    g_wsee_gput.ready = true;
    return true;
}
// A block's begin (k its kind, or WSEE_GPU_N: the first pass's, wsee_gput_pass).
inline void wsee_gput_begin(ID3D11DeviceContext* ctx, uint32_t k) {
    if (g_wsee_gput.open < 0 || !stats_on()) return;
    const uint32_t s = static_cast<uint32_t>(g_wsee_gput.open);
    if (g_wsee_gput.blk >= 0 || g_wsee_gput.n[s] + 8u > WSEE_GPUT_N) {
        g_wsee_gput_untimed.fetch_add(1u, std::memory_order_relaxed);
        return;
    }
    const uint32_t i = g_wsee_gput.n[s]++;
    g_wsee_gput.ev[s][i] = static_cast<uint8_t>(WSEE_GPU_EV_BEGIN | (k & 0x3Fu));
    ctx->End(g_wsee_gput.ts[s][i]);
    g_wsee_gput.blk = static_cast<int>(i);
}
// A pass of the open block: the block's kind where its begin left it open (bk), and a pass stamp of kind pk
// (WSEE_GPU_N: none - the one-pass kinds take the block's interval alone). One stamp is always left for the end.
inline void wsee_gput_pass(ID3D11DeviceContext* ctx, uint32_t bk, uint32_t pk) {
    if (g_wsee_gput.blk < 0 || g_wsee_gput.open < 0) return;
    const uint32_t s = static_cast<uint32_t>(g_wsee_gput.open);
    uint8_t& b = g_wsee_gput.ev[s][static_cast<uint32_t>(g_wsee_gput.blk)];
    if ((b & 0x3Fu) == WSEE_GPU_N) b = static_cast<uint8_t>(WSEE_GPU_EV_BEGIN | (bk & 0x3Fu));
    if (pk >= WSEE_GPU_N || g_wsee_gput.n[s] + 1u >= WSEE_GPUT_N) return;
    const uint32_t i = g_wsee_gput.n[s]++;
    g_wsee_gput.ev[s][i] = static_cast<uint8_t>(WSEE_GPU_EV_PASS | (pk & 0x3Fu));
    ctx->End(g_wsee_gput.ts[s][i]);
}
inline void wsee_gput_end(ID3D11DeviceContext* ctx) {
    if (g_wsee_gput.blk < 0 || g_wsee_gput.open < 0) return;
    const uint32_t s = static_cast<uint32_t>(g_wsee_gput.open);
    const uint32_t i = g_wsee_gput.n[s]++;   // Room: the begin kept eight, a pass leaves one.
    g_wsee_gput.ev[s][i] = WSEE_GPU_EV_END;
    ctx->End(g_wsee_gput.ts[s][i]);
    g_wsee_gput.blk = -1;
}
// A whole-kind block (the capture, B2, our tail, E, the resolve): its begin at construction, its end at destruction
// where this scope opened it. Nothing in it may set a slot (only the clear does: wsee_gput_head).
struct WseeGputScope {
    ID3D11DeviceContext* ctx;
    bool                 on;
    WseeGputScope(ID3D11DeviceContext* c, uint32_t k) : ctx(c), on(false) {
        if (g_wsee_gput.open < 0 || g_wsee_gput.blk >= 0 || !ctx) {
            if (g_wsee_gput.open >= 0 && g_wsee_gput.blk >= 0 && stats_on()) {
                g_wsee_gput_untimed.fetch_add(1u, std::memory_order_relaxed);
            }
            return;
        }
        wsee_gput_begin(ctx, k);
        on = g_wsee_gput.blk >= 0;
    }
    ~WseeGputScope() {
        if (on) wsee_gput_end(ctx);
    }
    WseeGputScope(const WseeGputScope&) = delete;
    WseeGputScope& operator=(const WseeGputScope&) = delete;
};
// A read slot's frame folded: each block's interval to its kind (a block no pass named - the re-issue's stencil
// backstop, which draws nothing - to the particle kind), each pass's to the pass kind, to its block's end or the next
// pass; published as the last frame and added to the sums.
inline void wsee_gput_fold(uint32_t s) {
    uint64_t ns[WSEE_GPU_N] = {};
    uint64_t n[WSEE_GPU_N] = {};
    const double f = static_cast<double>(g_wsee_gput.freq[s]);
    const auto add = [&](uint32_t k, uint64_t a, uint64_t b) {
        if (k >= WSEE_GPU_N || b < a) return;
        ns[k] += static_cast<uint64_t>(static_cast<double>(b - a) * 1.0e9 / f);
        ++n[k];
    };
    int bi = -1, pi = -1;
    uint32_t bk = WSEE_GPU_N, pk = WSEE_GPU_N;
    for (uint32_t i = 0; i < g_wsee_gput.n[s]; ++i) {
        const uint8_t e = g_wsee_gput.ev[s][i];
        const uint64_t t = g_wsee_gput.tv[s][i];
        if ((e & 0xC0u) == WSEE_GPU_EV_BEGIN) {
            bi = static_cast<int>(i);
            bk = e & 0x3Fu;
            pi = -1;
        } else if (bi >= 0 && (e & 0xC0u) == WSEE_GPU_EV_PASS) {
            if (pi >= 0) add(pk, g_wsee_gput.tv[s][static_cast<uint32_t>(pi)], t);
            pi = static_cast<int>(i);
            pk = e & 0x3Fu;
        } else if (bi >= 0 && (e & 0xC0u) == WSEE_GPU_EV_END) {
            if (pi >= 0) add(pk, g_wsee_gput.tv[s][static_cast<uint32_t>(pi)], t);
            add(bk >= WSEE_GPU_N ? static_cast<uint32_t>(WSEE_GPU_PART) : bk,
                g_wsee_gput.tv[s][static_cast<uint32_t>(bi)], t);
            bi = pi = -1;
        }
    }
    for (uint32_t k = 0; k < WSEE_GPU_N; ++k) {
        g_wsee_gput_ns_last[k].store(ns[k], std::memory_order_relaxed);
        g_wsee_gput_n_last[k].store(n[k], std::memory_order_relaxed);
        g_wsee_gput_ns_sum[k].fetch_add(ns[k], std::memory_order_relaxed);
        g_wsee_gput_n_sum[k].fetch_add(n[k], std::memory_order_relaxed);
    }
    g_wsee_gput_read.fetch_add(1u, std::memory_order_relaxed);
}
// The main depth's clear (render thread; c the new cycle, cap whether it captures - g_wsee.on): the frame
// recorded ends (its disjoint query; a block still open there is dropped with what followed it), the frames awaited
// are read as far as they are done, and a probe frame opens. Disarmed: nothing recorded, nothing awaited.
inline void wsee_gput_head(ID3D11DeviceContext* ctx, uint64_t c, bool cap) {
    if (g_wsee_gput.open >= 0) {
        const uint32_t o = static_cast<uint32_t>(g_wsee_gput.open);
        if (g_wsee_gput.blk >= 0) g_wsee_gput.n[o] = static_cast<uint32_t>(g_wsee_gput.blk);
        ctx->End(g_wsee_gput.dj[o]);
        g_wsee_gput.wait[o] = true;
        g_wsee_gput.open = -1;
        g_wsee_gput.blk = -1;
    }
    if (!stats_on()) {
        for (bool& w : g_wsee_gput.wait) w = false;
        return;
    }
    uint32_t budget = WSEE_GPUT_READ;
    for (uint32_t s = 0; s < WSEE_GPUT_RING; ++s) {
        if (!g_wsee_gput.wait[s]) continue;
        if (c - g_wsee_gput.cyc[s] > WSEE_GPUT_WAIT) {
            g_wsee_gput.wait[s] = false;
            g_wsee_gput_drop.fetch_add(1u, std::memory_order_relaxed);
            continue;
        }
        if (g_wsee_gput.freq[s] == 0) {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj = {};
            if (ctx->GetData(g_wsee_gput.dj[s], &dj, sizeof(dj),
                             D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) continue;
            if (dj.Disjoint || dj.Frequency == 0) {
                g_wsee_gput.wait[s] = false;
                g_wsee_gput_drop.fetch_add(1u, std::memory_order_relaxed);
                continue;
            }
            g_wsee_gput.freq[s] = dj.Frequency;
        }
        while (budget > 0 && g_wsee_gput.rd[s] < g_wsee_gput.n[s]) {
            UINT64 t = 0;
            if (ctx->GetData(g_wsee_gput.ts[s][g_wsee_gput.rd[s]], &t, sizeof(t),
                             D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) break;
            g_wsee_gput.tv[s][g_wsee_gput.rd[s]++] = t;
            --budget;
        }
        if (g_wsee_gput.rd[s] == g_wsee_gput.n[s]) {
            wsee_gput_fold(s);
            g_wsee_gput.wait[s] = false;
        }
    }
    if (cap && c % WSEE_GPUT_EVERY == WSEE_GPUT_AT && !g_wsee_gput.wait[g_wsee_gput.next] &&
        wsee_gput_ensure(ctx)) {
        const uint32_t s = g_wsee_gput.next;
        g_wsee_gput.n[s] = g_wsee_gput.rd[s] = 0;
        g_wsee_gput.freq[s] = 0;
        g_wsee_gput.cyc[s] = c;
        ctx->Begin(g_wsee_gput.dj[s]);
        g_wsee_gput.open = static_cast<int>(s);
        g_wsee_gput.next = (s + 1u) % WSEE_GPUT_RING;
    }
}

enum GpuZone : uint32_t {
    GPU_INJECT, GPU_SUN_LADDER, GPU_DLS_FRAME, GPU_DLS_WORLD, GPU_MASK_CAST, GPU_SVS_PRIME,
    GPU_SVS_VOL_COPY, GPU_VMIR, GPU_SSAO_POST, GPU_SNAPSHOT, GPU_PIP,
    GPU_FLUSH, GPU_FLUSH_UI, GPU_SCENE_CAPTURE, GPU_DLF,
    GPU_WSEE,   // WSEE: the capture (wsee_snapshot) alone - one pair a zone a cycle, so no re-issue is timed.
    GPU_WSEE_B2,   // B2 and S2 (wsee_b2_take), once a cycle.
    GPU_ZONE_N
};
static const char* const g_gpu_zone_name[GPU_ZONE_N] = {
    "gInject", "gSunLadder", "gDlsFrame", "gDlsWorld", "gMaskCast", "gSvsPrime",
    "gSvsVolCopy", "gVmir", "gSsaoPost", "gSnapshot", "gPip",
    "gFlush", "gFlushUi", "gSceneCapture", "gDlFog",
    "gWsee", "gWseeB2",
};
static constexpr uint32_t GPU_RING = 8u;
// The cycles back a slot is read, never waiting (< GPU_RING): the GPU was measured running 5 cycles behind the CPU
// under load at 4K, and a slot read sooner is never done. The ring reopens it GPU_RING - GPU_LAG folds later.
static constexpr uint32_t GPU_LAG = 6u;
// GPU_PASS: the effect passes a cycle times (the scene chain's and the UI chain's), each its own timestamp pair in
// the cycle's bracket, keyed by gpu_pass_key; past this many in a cycle the rest go untimed.
static constexpr uint32_t GPU_PASS_N = 32u;
struct GpuProf {
    ID3D11Query* disjoint[GPU_RING] = {};
    ID3D11Query* ts[GPU_ZONE_N][GPU_RING][2] = {};
    bool         used[GPU_ZONE_N][GPU_RING] = {};
    ID3D11Query* pts[GPU_RING][GPU_PASS_N][2] = {};   // GPU_PASS.
    uint32_t     pass_n[GPU_RING] = {};
    uint32_t     pass_key[GPU_RING][GPU_PASS_N] = {};
    bool         pass_used[GPU_RING][GPU_PASS_N] = {};
    bool         ready = false;    // Queries exist.
    bool         failed = false;   // A create failed: the GPU side stands down for the session.
    int          open_slot = -1;   // Slot whose disjoint is begun, -1 = none.
    uint64_t     cycle = 0;        // Cycles folded (ring index source).
};
static GpuProf g_gpu_prof;
static std::atomic<uint64_t> g_gpu_us_pub[GPU_ZONE_N];
// PROF_WINDOW: every GPU value the fold harvested since the arming / reset (sum, count, worst).
static std::atomic<uint64_t> g_gpu_win_us[GPU_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_n[GPU_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_max[GPU_ZONE_N];
// GPU_PASS: a pass's key - the effect id (0 - 255), the chain (bit 8: 0 the scene's, 1 the UI's), a user shader
// (bit 9), the stages fused into it (bits 10 - 15, at most 63).
inline uint32_t gpu_pass_key(uint32_t chain, int effect, bool user, uint32_t fused) {
    const uint32_t e = effect < 0 ? 0u : (effect > 255 ? 255u : static_cast<uint32_t>(effect));
    const uint32_t f = fused > 63u ? 63u : fused;
    return e | ((chain & 1u) << 8) | ((user ? 1u : 0u) << 9) | (f << 10);
}
// The harvested cycle's passes (key << 32 | microseconds), in draw order, and how many; and the window per (chain,
// user, effect) - the key's low ten bits (key & (GPU_PASS_W - 1)): sum, count, worst.
static constexpr uint32_t GPU_PASS_W = 1024u;
static std::atomic<uint64_t> g_gpu_pass_pub[GPU_PASS_N];
static std::atomic<uint32_t> g_gpu_pass_pub_n{ 0 };
static std::atomic<uint64_t> g_gpu_passw_us[GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_n[GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_max[GPU_PASS_W];
// PROF_WINDOW / PROF_FUNNEL / GPU_PASS: the window starts again and the published values go to zero - by
// resetRenderStats (prof_reset_pub) and by the first fold after it (PROF_DISARM).
inline void prof_window_zero() {
    g_prof.win_cycles.store(0, std::memory_order_relaxed);
    for (uint32_t z = 0; z < PROF_ZONE_N; ++z) {
        g_prof.win_us[z].store(0, std::memory_order_relaxed);
        g_prof.win_n[z].store(0, std::memory_order_relaxed);
        g_prof.win_max[z].store(0, std::memory_order_relaxed);
        g_prof.xb_pub[z].store(0, std::memory_order_relaxed);
        g_prof.xb_win[z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t z = 0; z < GPU_ZONE_N; ++z) {
        g_gpu_win_us[z].store(0, std::memory_order_relaxed);
        g_gpu_win_n[z].store(0, std::memory_order_relaxed);
        g_gpu_win_max[z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t k = 0; k < FUNNEL_N; ++k) {
        g_fun_pub[k].store(0, std::memory_order_relaxed);
        g_fun_win[k].store(0, std::memory_order_relaxed);
    }
    g_gpu_pass_pub_n.store(0, std::memory_order_relaxed);
    for (uint32_t w = 0; w < GPU_PASS_W; ++w) {
        g_gpu_passw_us[w].store(0, std::memory_order_relaxed);
        g_gpu_passw_n[w].store(0, std::memory_order_relaxed);
        g_gpu_passw_max[w].store(0, std::memory_order_relaxed);
    }
}
inline uint32_t gpu_slot() { return static_cast<uint32_t>(g_gpu_prof.cycle % GPU_RING); }
inline bool gpu_prof_ensure(ID3D11DeviceContext* ctx) {
    if (g_gpu_prof.ready) return true;
    if (g_gpu_prof.failed || !ctx) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) { g_gpu_prof.failed = true; return false; }
    bool ok = true;
    for (uint32_t r = 0; r < GPU_RING && ok; ++r) {
        D3D11_QUERY_DESC dd = {}; dd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (FAILED(dev->CreateQuery(&dd, &g_gpu_prof.disjoint[r]))) ok = false;
        for (uint32_t z = 0; z < GPU_ZONE_N && ok; ++z) {
            D3D11_QUERY_DESC td = {}; td.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(dev->CreateQuery(&td, &g_gpu_prof.ts[z][r][0])) ||
                FAILED(dev->CreateQuery(&td, &g_gpu_prof.ts[z][r][1]))) ok = false;
        }
        for (uint32_t p = 0; p < GPU_PASS_N && ok; ++p) {   // GPU_PASS.
            D3D11_QUERY_DESC pd = {}; pd.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(dev->CreateQuery(&pd, &g_gpu_prof.pts[r][p][0])) ||
                FAILED(dev->CreateQuery(&pd, &g_gpu_prof.pts[r][p][1]))) ok = false;
        }
    }
    dev->Release();
    if (!ok) {
        for (uint32_t r = 0; r < GPU_RING; ++r) {
            KH_SAFE_RELEASE(g_gpu_prof.disjoint[r]);
            for (uint32_t z = 0; z < GPU_ZONE_N; ++z) { KH_SAFE_RELEASE(g_gpu_prof.ts[z][r][0]); KH_SAFE_RELEASE(g_gpu_prof.ts[z][r][1]); }
            for (uint32_t p = 0; p < GPU_PASS_N; ++p) {   // GPU_PASS.
                KH_SAFE_RELEASE(g_gpu_prof.pts[r][p][0]);
                KH_SAFE_RELEASE(g_gpu_prof.pts[r][p][1]);
            }
        }
        g_gpu_prof.failed = true;
        return false;
    }
    g_gpu_prof.ready = true;
    return true;
}
inline void gpu_prof_release() {
    wsee_gput_release();   // WSEE_GPUT: its queries share the PROF set's lifetime.
    for (uint32_t r = 0; r < GPU_RING; ++r) {
        KH_SAFE_RELEASE(g_gpu_prof.disjoint[r]);
        for (uint32_t z = 0; z < GPU_ZONE_N; ++z) {
            KH_SAFE_RELEASE(g_gpu_prof.ts[z][r][0]);
            KH_SAFE_RELEASE(g_gpu_prof.ts[z][r][1]);
            g_gpu_prof.used[z][r] = false;
        }
        for (uint32_t p = 0; p < GPU_PASS_N; ++p) {   // GPU_PASS.
            KH_SAFE_RELEASE(g_gpu_prof.pts[r][p][0]);
            KH_SAFE_RELEASE(g_gpu_prof.pts[r][p][1]);
            g_gpu_prof.pass_used[r][p] = false;
        }
        g_gpu_prof.pass_n[r] = 0;
    }
    g_gpu_prof.ready = false;
    g_gpu_prof.failed = false;
    g_gpu_prof.open_slot = -1;
}
struct GpuScope {
    ID3D11DeviceContext* ctx;
    GpuZone            z;
    int                  slot;   // The bracket the begin stamp went to; -1 = none.
    GpuScope(ID3D11DeviceContext* c, GpuZone zone) : ctx(c), z(zone), slot(-1) {
        // Only inside an open cycle bracket, on a ready query set.
        if (!stats_on() || !ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        slot = g_gpu_prof.open_slot;
        ctx->End(g_gpu_prof.ts[z][static_cast<uint32_t>(slot)][0]);
        g_gpu_prof.used[z][static_cast<uint32_t>(slot)] = true;
    }
    ~GpuScope() {
        // The end stamp goes to the begin's bracket; if that bracket closed
        // meanwhile the pair is dropped rather than straddling two.
        if (slot < 0) return;
        if (g_gpu_prof.open_slot != slot) { g_gpu_prof.used[z][static_cast<uint32_t>(slot)] = false; return; }
        ctx->End(g_gpu_prof.ts[z][static_cast<uint32_t>(slot)][1]);
    }
    GpuScope(const GpuScope&) = delete;
    GpuScope& operator=(const GpuScope&) = delete;
};
#define KH_GPU_SCOPE(ctx, z) GpuScope gpu_scope_##z((ctx), z)
// GPU_PASS: one effect pass's GPU interval - GpuScope's rules (only inside an open cycle bracket, on a ready
// query set; an end that finds its bracket closed drops the pair), one of the cycle's GPU_PASS_N pairs.
struct GpuPassScope {
    ID3D11DeviceContext* ctx;
    int                  slot;   // The bracket the begin stamp went to; -1 = none.
    uint32_t             i;
    GpuPassScope(ID3D11DeviceContext* c, uint32_t chain, int effect, bool user,
                 uint32_t fused) : ctx(c), slot(-1), i(0) {
        if (!stats_on() || !ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        const uint32_t s = static_cast<uint32_t>(g_gpu_prof.open_slot);
        if (g_gpu_prof.pass_n[s] >= GPU_PASS_N) return;
        i = g_gpu_prof.pass_n[s]++;
        g_gpu_prof.pass_key[s][i] = gpu_pass_key(chain, effect, user, fused);
        g_gpu_prof.pass_used[s][i] = true;
        ctx->End(g_gpu_prof.pts[s][i][0]);
        slot = static_cast<int>(s);
    }
    ~GpuPassScope() {
        if (slot < 0) return;
        if (g_gpu_prof.open_slot != slot) {
            g_gpu_prof.pass_used[static_cast<uint32_t>(slot)][i] = false;
            return;
        }
        ctx->End(g_gpu_prof.pts[static_cast<uint32_t>(slot)][i][1]);
    }
    GpuPassScope(const GpuPassScope&) = delete;
    GpuPassScope& operator=(const GpuPassScope&) = delete;
};

// The cycle fold (main depth clear, render thread): publish the CPU accumulators, close this cycle's GPU bracket,
// harvest the slot GPU_LAG cycles back, open the next bracket while the stats stay armed.
inline void prof_fold(ID3D11DeviceContext* ctx) {
    const uint64_t f = prof_freq();
    const bool on = stats_on();
    // PROF_DISARM: publish only what was measured armed and since the last reset.
    const uint32_t ep = g_prof.epoch.load(std::memory_order_acquire);
    const bool pub = on && ep == g_prof.epoch_seen;
    const bool reset = ep != g_prof.epoch_seen;   // A resetRenderStats since the last fold.
    g_prof.epoch_seen = ep;
    // PROF_WINDOW: a whole armed cycle. The TSC zones, scanExtBytes and the funnel publish these only: a scope
    // still open at a disarm adds after it, and nothing drains them disarmed - the arming fold drops what they hold.
    const bool win = pub && g_prof.prev_on;
    g_prof.prev_on = on;
    // PROF_DISARM for the window, the funnel and the passes: the first fold after a reset, armed or not, drops the
    // armed-only accumulators and zeroes the window and what is published, on this thread (a fold that raced a reset
    // made without the graphics lock may have added pre-reset values after the reset zeroed them). Once a reset.
    if (reset) {
        for (uint32_t z = 0; z < PROF_ZONE_N; ++z) {
            g_prof.tsc[z].store(0, std::memory_order_relaxed);
            g_prof.xb_acc[z].store(0, std::memory_order_relaxed);
        }
        for (uint32_t k = 0; k < FUNNEL_N; ++k) g_fun_acc[k].store(0, std::memory_order_relaxed);
        prof_window_zero();
    }
    // KH_PROF_SUB: the TSC's rate over the interval since the last armed fold. Disarmed folds skip the new work below
    // (nothing armed-only accumulates while disarmed: disarming is a reset, whose leftovers the block above dropped).
    double tpu = 0.0;
    if (on) {
        LARGE_INTEGER q = {};
        QueryPerformanceCounter(&q);
        const uint64_t qn = static_cast<uint64_t>(q.QuadPart);
        const uint64_t tn = __rdtsc();
        if (g_prof.cal_q != 0 && qn > g_prof.cal_q && tn > g_prof.cal_t) {
            g_prof.tsc_per_us.store(static_cast<double>(tn - g_prof.cal_t) * static_cast<double>(f) /
                                    (static_cast<double>(qn - g_prof.cal_q) * 1.0e6), std::memory_order_relaxed);
        }
        g_prof.cal_q = qn;
        g_prof.cal_t = tn;
        tpu = g_prof.tsc_per_us.load(std::memory_order_relaxed);
        if (win) g_prof.win_cycles.fetch_add(1u, std::memory_order_relaxed);   // PROF_WINDOW.
    }
    for (uint32_t z = 0; z < PROF_ZONE_N; ++z) {
        const uint64_t t = g_prof.ticks[z].exchange(0, std::memory_order_relaxed);
        const uint64_t n = g_prof.count[z].exchange(0, std::memory_order_relaxed);
        uint64_t us = pub ? t * 1000000ull / f : 0ull;
        if (on) {   // KH_PROF_SUB / PROF_UPLOAD / PROF_WINDOW.
            const uint64_t s = g_prof.tsc[z].exchange(0, std::memory_order_relaxed);
            const uint64_t x = g_prof.xb_acc[z].exchange(0, std::memory_order_relaxed);
            if (win && s != 0 && tpu > 0.0) {
                us += static_cast<uint64_t>(static_cast<double>(s) / tpu);
            }
            g_prof.xb_pub[z].store(win ? x : 0ull, std::memory_order_relaxed);
            if (win) {
                g_prof.win_us[z].fetch_add(us, std::memory_order_relaxed);
                g_prof.win_n[z].fetch_add(n, std::memory_order_relaxed);
                g_prof.xb_win[z].fetch_add(x, std::memory_order_relaxed);
                if (us > g_prof.win_max[z].load(std::memory_order_relaxed)) {
                    g_prof.win_max[z].store(us, std::memory_order_relaxed);
                }
            }
        }
        g_prof.us_pub[z].store(pub ? us : 0ull, std::memory_order_relaxed);
        g_prof.n_pub[z].store(pub ? n : 0ull, std::memory_order_relaxed);
    }
    if (on) {   // PROF_FUNNEL.
        for (uint32_t k = 0; k < FUNNEL_N; ++k) {
            const uint64_t v = g_fun_acc[k].exchange(0, std::memory_order_relaxed);
            g_fun_pub[k].store(win ? v : 0ull, std::memory_order_relaxed);
            if (win) g_fun_win[k].fetch_add(v, std::memory_order_relaxed);
        }
    }
    if (!pub) {   // PROF_DISARM: the GPU times too (the note above g_prof.epoch).
        for (uint32_t z = 0; z < GPU_ZONE_N; ++z) {
            g_gpu_us_pub[z].store(0, std::memory_order_relaxed);
        }
    }
    if (g_gpu_prof.open_slot >= 0 && g_gpu_prof.ready && ctx) {
        ctx->End(g_gpu_prof.disjoint[static_cast<uint32_t>(g_gpu_prof.open_slot)]);
        g_gpu_prof.open_slot = -1;
        g_gpu_prof.cycle++;
        if (!pub) {   // PROF_DISARM: no slot's stamps outlive it to be read after the next arming.
            for (uint32_t z = 0; z < GPU_ZONE_N; ++z) {
                for (uint32_t r = 0; r < GPU_RING; ++r) g_gpu_prof.used[z][r] = false;
            }
            for (uint32_t r = 0; r < GPU_RING; ++r) g_gpu_prof.pass_n[r] = 0;   // GPU_PASS.
        }
        // Harvest: the slot GPU_LAG cycles back, never waiting.
        if (pub && g_gpu_prof.cycle >= GPU_LAG) {
            const uint32_t s = static_cast<uint32_t>((g_gpu_prof.cycle - GPU_LAG) % GPU_RING);
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT dj = {};
            if (ctx->GetData(g_gpu_prof.disjoint[s], &dj, sizeof(dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                !dj.Disjoint && dj.Frequency > 0) {
                for (uint32_t z = 0; z < GPU_ZONE_N; ++z) {
                    if (!g_gpu_prof.used[z][s]) { g_gpu_us_pub[z].store(0, std::memory_order_relaxed); continue; }
                    UINT64 a = 0, b = 0;
                    if (ctx->GetData(g_gpu_prof.ts[z][s][0], &a, sizeof(a), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        ctx->GetData(g_gpu_prof.ts[z][s][1], &b, sizeof(b), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        b >= a) {
                        const uint64_t g = (b - a) * 1000000ull / dj.Frequency;
                        g_gpu_us_pub[z].store(g, std::memory_order_relaxed);
                        g_gpu_win_us[z].fetch_add(g, std::memory_order_relaxed);   // PROF_WINDOW.
                        g_gpu_win_n[z].fetch_add(1u, std::memory_order_relaxed);
                        if (g > g_gpu_win_max[z].load(std::memory_order_relaxed)) {
                            g_gpu_win_max[z].store(g, std::memory_order_relaxed);
                        }
                    }
                    g_gpu_prof.used[z][s] = false;
                }
                // GPU_PASS: the slot's passes, in draw order; the published list is this cycle's.
                uint32_t pk = 0;
                for (uint32_t i = 0; i < g_gpu_prof.pass_n[s]; ++i) {
                    if (!g_gpu_prof.pass_used[s][i]) continue;
                    g_gpu_prof.pass_used[s][i] = false;
                    UINT64 a = 0, b = 0;
                    ID3D11Query* const* const q = g_gpu_prof.pts[s][i];
                    const UINT fl = D3D11_ASYNC_GETDATA_DONOTFLUSH;
                    if (ctx->GetData(q[0], &a, sizeof(a), fl) != S_OK ||
                        ctx->GetData(q[1], &b, sizeof(b), fl) != S_OK ||
                        b < a) continue;
                    const uint64_t g = (b - a) * 1000000ull / dj.Frequency;
                    const uint32_t key = g_gpu_prof.pass_key[s][i];
                    g_gpu_pass_pub[pk++].store((static_cast<uint64_t>(key) << 32) | (g & 0xFFFFFFFFull),
                                               std::memory_order_relaxed);
                    const uint32_t w = key & (GPU_PASS_W - 1u);
                    g_gpu_passw_us[w].fetch_add(g, std::memory_order_relaxed);
                    g_gpu_passw_n[w].fetch_add(1u, std::memory_order_relaxed);
                    if (g > g_gpu_passw_max[w].load(std::memory_order_relaxed)) {
                        g_gpu_passw_max[w].store(g, std::memory_order_relaxed);
                    }
                }
                g_gpu_pass_pub_n.store(pk, std::memory_order_relaxed);
            }
        }
    }
    if (on && ctx && gpu_prof_ensure(ctx)) {
        const uint32_t s = gpu_slot();
        for (uint32_t z = 0; z < GPU_ZONE_N; ++z) g_gpu_prof.used[z][s] = false;
        for (uint32_t i = 0; i < GPU_PASS_N; ++i) g_gpu_prof.pass_used[s][i] = false;
        g_gpu_prof.pass_n[s] = 0;   // GPU_PASS.
        ctx->Begin(g_gpu_prof.disjoint[s]);
        g_gpu_prof.open_slot = static_cast<int>(s);
    }
}
inline void prof_reset_pub() {
    for (uint32_t z = 0; z < PROF_ZONE_N;
        ++z) { g_prof.us_pub[z].store(0, std::memory_order_relaxed);
        g_prof.n_pub[z].store(0, std::memory_order_relaxed); }
    for (uint32_t z = 0; z < GPU_ZONE_N; ++z) g_gpu_us_pub[z].store(0, std::memory_order_relaxed);
    prof_window_zero();   // PROF_WINDOW (the cycle in flight's accumulators are the fold's to drop).
}
}