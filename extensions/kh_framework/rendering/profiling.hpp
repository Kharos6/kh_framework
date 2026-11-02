#pragma once

// The shared head (rendering_integration.hpp): KH_PROF, the stats gate and the profiler (CPU zones, funnels, the
// see-through and GPU pass timers). Never include it on its own.
namespace RenderIntegration {
inline bool kh_stats_on();   // Defined with KH_PROF at the top of the namespace.

// KH_STATS_ARMED (declared ahead of the stats block for KH_PROF's scopes): collection is off until the first
// getRenderStats.
static std::atomic<bool> g_stats_armed{ false };
inline bool kh_stats_on() { return g_stats_armed.load(std::memory_order_relaxed); }
// KH_PROF: opt-in frame-time instrumentation, CPU and GPU, per main-depth cycle. Nothing here runs unless the stats
// are armed (a getRenderStats): a disarmed session pays one relaxed load per scope, no clock read, no query (the first
// fold after a disarm closes, unread, the bracket the armed cycle opened, and zeroes the armed-only accumulators once -
// KH_PROF_DISARM).
//
// CPU zones: KhProfScope takes two TSC reads (__rdtsc) around a region and adds the ticks and one entry to the zone's
// accumulators - relaxed atomics: zones run on the render thread, the game thread under the park, the UI-phase thread
// and the worker pool. The fold at the main depth clear (kh_prof_fold) converts the accumulators to microseconds (the
// TSC's rate measured against QPC between armed folds, KH_PROF_SUB; kh_prof_now / kh_prof_add intervals stay on QPC)
// and publishes them with the entry counts as the ended cycle's values. KH_PROF_TSC_CLOCK: a QPC call (~40 ns) is a
// zone's floor per timed call, as large as the work of the small hooks and x16 on a sampled zone; a TSC read is a few
// ns. The TSC zones publish whole armed cycles only (the cycle the arming splits reads 0; by the next one the rate is
// measured). Worker-pool zones (cloth, chain, skin, BVH) sum across workers - worker-microseconds, which can exceed
// the frame: divide by the count. The BVH build's zone also takes the game thread's builds (a rigid collider's first
// use, kh_physics_bvh_for), a stall rather than worker time.
//
// GPU zones: one TIMESTAMP_DISJOINT query brackets each cycle (begun and ended at the main depth clear, on the render
// thread); each zone owns a begin / end TIMESTAMP pair per ring slot. KhGpuScope issues End(begin) and End(end) on the
// context it is given - the immediate context, whether the render thread or the parked game thread holds it. The fold
// reads the slot KH_GPU_LAG cycles back with DONOTFLUSH and keeps the last value when a result is not ready, so no
// readback stalls the pipeline. Queries are created on the first armed use and released with the resources. A zone
// that ran more than once in a cycle reports its last run (a pair holds one interval); the CPU count tells how many.
enum KhProfZone : uint32_t {
    // Render-thread hooks.
    KHP_HOOK_CLEAR, KHP_HOOK_DRAW, KHP_HOOK_MAP, KHP_UPLOAD_SCAN,
    // Injection and its systems.
    KHP_INJECT, KHP_ATTACH_STEP, KHP_SCENE_SYNC, KHP_OBJBUF_SYNC, KHP_SUN_LADDER,
    KHP_DLS_FRAME, KHP_DLS_WORLD, KHP_MASK_CAST, KHP_SVS_PRIME, KHP_SVS_VOL_COPY,
    KHP_VMIR, KHP_SSAO, KHP_CULL_BUILD, KHP_VIS_OCC, KHP_INST_PLAN, KHP_SNAPSHOT,
    KHP_PIP, KHP_BAND_COMMIT, KHP_THM_AUTOBUILD, KHP_DL_ACQUIRE,
    // The flushes and their systems.
    KHP_FLUSH, KHP_FLUSH_FRAME, KHP_FLUSH_UI, KHP_CLOTH_UPLOAD, KHP_SKIN_UPLOAD, KHP_SKIN_SYNC,
    // KH_PARK_WAIT: the graphics-lock acquire alone, per park (the game thread stalled until the render thread reaches
    // the engine's grant point), for the Draw3D park and the UI driver's park; flushUiFrame is the UI park whole.
    KHP_FLUSH_LOCK_WAIT, KHP_FLUSH_UI_LOCK_WAIT, KHP_FLUSH_UI_FRAME,
    // flush_frame's pre-lock work in three pieces: the attachment reaps and the Draw3D sampler; the demand census
    // under g_draw_list_mutex; the rest up to the park decision (proxy step, chain prepare, the driver re-hoist, the
    // hook ensure).
    KHP_FLUSH_PRE, KHP_FLUSH_CENSUS, KHP_FLUSH_PREP,
    KHP_CHAIN_PREPARE, KHP_CLOTH_PROXY, KHP_PHYSICS_GATHER, KHP_MESH_GC, KHP_SCENE_CAPTURE,
    KHP_UI_MASK_DRAW,
    // Worker pool (worker-microseconds).
    KHP_W_CLOTH_SIM, KHP_W_CHAIN_SIM, KHP_W_SKIN_JOB, KHP_W_BVH_BUILD, KHP_W_BVH_REFIT,
    // Builders: kh_lod_build (import time) and ensure_resources, called from the injection, the flushes, the seam and
    // the sun-depth paths: a call before g_res.initialized builds (or defers while shaders compile); every later one
    // takes its fast path, the mesh VB top-up.
    KHP_LOD_BUILD, KHP_ENSURE_RES,
    // KH_PROF_HOOKS: every other installed context hook's own work (the swap chain's rare ResizeBuffers has no
    // zone), the driver's call left outside (hookUpdSub alone wraps its driver call: drvUpdSub below takes it out).
    // hookUpdSub / hookPsSrv / hookOmBlend / hookOmDss / drawRecord / hookCopyRegion / hookMisc are sampled
    // (KH_PROF_SAMPLE); hookOmRt / hookOmRtUav / hookCopyRes / hookResolve are called far less, some calls with heavy
    // work (a PIP leave, the flush at the resolve), and are timed whole, as is drawPost (KH_PROF_TSC: its work - the
    // view-model mirror's re-issued draw, the UI quad, the bracket - lands on one draw or two a frame). The zones nest
    // and are not additive: they time every call through the hook, ours too, so work our own code issues inside a
    // zone (the flush inside hookResolve, the PIP leave inside hookOmRt / hookCopyRes, our draws inside a draw step)
    // also counts in the zones of the hooks it calls.
    KHP_HOOK_UPDSUB, KHP_HOOK_PSSRV, KHP_HOOK_OMRT, KHP_HOOK_OMRTUAV, KHP_HOOK_OMBLEND, KHP_HOOK_OMDSS,
    KHP_HOOK_COPYRES, KHP_HOOK_RESOLVE, KHP_HOOK_COPYREGION, KHP_HOOK_MISC, KHP_DRAW_RECORD, KHP_DRAW_POST,
    // KH_PROF_DRV: the driver's own Map / Unmap (inside hookMap) and UpdateSubresource (inside hookUpdSub), timed on
    // one call in KH_PROF_SAMPLE_EVERY of their own (kh_prof_drv_t0) with the TSC and scaled; their N is those calls
    // x KH_PROF_SAMPLE_EVERY (an estimate). hookMap - drvMap - drvUnmap estimates our share of hookMap only while
    // the driver's calls cost alike: the scope and the driver are sampled on different calls, so a Map that stalls in
    // the driver (waiting on the GPU) lands scaled in one estimate and not the other - a large difference, or a drvMap
    // max above hookMap's, then reads as a driver stall, not our work.
    KHP_DRV_MAP, KHP_DRV_UNMAP, KHP_DRV_UPDSUB,
    // KH_PROF_UPLOAD: the funnel's steps for each captured upload (KhProfSub; one decision per upload, timed one in
    // KH_PROF_SAMPLE_EVERY): the capture (the copy), then each scanner, then the replay's image note.
    KHP_UP_CAPTURE, KHP_SC_PROJ, KHP_SC_LIVE, KHP_SC_LOCATOR, KHP_SC_PIP, KHP_SC_ENGCAM, KHP_SC_REG, KHP_SC_VIEW,
    KHP_SC_B2, KHP_SC_RPIMG,
    // KH_PROF_DRAWSTEP: reorder_pre_draw's steps for the engine's draws (KhProfSub; one decision per draw, our own
    // draws take none): dsShadowNote / dsMaskNote / dlSample sampled; timed whole the ones whose work lands on a few
    // draws a frame (dsSeamPump, dsPip, dsInfront: the seam / svs, PIP and in-front injections), or one draw in 8 or
    // 16, or a retry (dsCascbind, dsSkybind, dsLatch). mask cast, band commit, the UI mask and the injection keep
    // their own zones above.
    KHP_DS_SEAM, KHP_DS_PIP, KHP_DS_SHADOW, KHP_DS_MASK, KHP_DS_DL, KHP_DS_INFRONT, KHP_DS_CASCBIND,
    KHP_DS_SKYBIND, KHP_DS_LATCH,
    // KH_WSEE - the see-through record's CPU zones. dsWsee: the pre-draw's step for the engine's draws in a wanted
    // cycle (kh_wsee_pre_draw: the state reads and the capture; sampled, as dsShadowNote). wseePost: the re-issues
    // (kh_wsee_post, inside the draw hooks' post: the saves, the passes' draws and the restore), timed whole with the
    // TSC as drawPost is, which it nests in.
    KHP_DS_WSEE, KHP_WSEE_POST,
    // wseeClear: the see-through work at each main-depth clear (kh_wsee_cycle_note, kh_psv_cycle_head's variant
    // compiles - KH_PSV_MAKE_N entries at most, up to three driver compiles each -, kh_wsee_gput_head), nested in
    // hookClear. wseeCapture: the capture and the B2 / S2 moment (kh_wsee_snapshot, kh_wsee_b2_take: once a cycle
    // each, refusals counted too), nested in dsWsee or in inject (the injection tail's capture). Both KH_PROF_TSC.
    KHP_WSEE_CLEAR, KHP_WSEE_CAP,
    KHP_ZONE_N
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
    "dsWsee", "wseePost", "wseeClear", "wseeCapture",   // KH_WSEE.
};
static_assert(sizeof(g_prof_zone_name) / sizeof(g_prof_zone_name[0]) == KHP_ZONE_N, "KH_PROF: a name per zone");
static std::atomic<uint64_t> g_prof_ticks[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_count[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_us_pub[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_n_pub[KHP_ZONE_N];
// KH_PROF_DISARM: resetRenderStats' count (reset_stat_counters bumps it) and the one the last fold saw (render
// thread). A fold that finds collection off, or a reset since the fold before it, publishes zeros - CPU zones and GPU
// times alike - and reads no GPU slot: the accumulators and the query ring hold pre-reset measurements, and the first
// getRenderStats after a reset reports zero. The fold itself zeroes the GPU times, not only the reset: a harvest that
// raced a reset made without the graphics lock (resetRenderStats' fallback) can store pre-reset times after the reset
// zeroed them, and no disarmed fold harvests again.
static std::atomic<uint32_t> g_prof_epoch{ 0 };
static uint32_t g_prof_epoch_seen = 0;
inline uint64_t kh_prof_freq() {
    static const uint64_t khpf = [] { LARGE_INTEGER khpf_f = {}; QueryPerformanceFrequency(&khpf_f); return khpf_f.QuadPart > 0 ? static_cast<uint64_t>(khpf_f.QuadPart) : 1ull; }();
    return khpf;
}
// KH_PROF_SAMPLE: the zones entered tens of thousands of times a frame (KHP_HOOK_MAP on every Map and Unmap,
// KHP_UPLOAD_SCAN on every scanned upload, and the hooks KH_PROF_HOOKS adds: kh_prof_sampled's set) take one clock
// pair in KH_PROF_SAMPLE_EVERY entries and scale the ticks; the count stays exact. Two QPC calls on ~50,000 entries a
// frame were ~4 ms of the armed hook figures and most of the armed-vs-unarmed frame gap.
static constexpr uint32_t KH_PROF_SAMPLE_EVERY = 16u;
static_assert((KH_PROF_SAMPLE_EVERY & (KH_PROF_SAMPLE_EVERY - 1u)) == 0u, "KH_PROF_SAMPLE: a power of two");
static std::atomic<uint32_t> g_prof_sample_ctr[KHP_ZONE_N];   // hooked_map's scope opens before its thread gate: any thread.
// KH_PROF_TSC_CLOCK: the zones' TSC ticks (KhProfScope, KH_PROF_TSC, KH_PROF_DRV with atomic adds, any thread;
// KhProfSub with plain adds, render thread - no zone is written both ways), converted by the fold (KH_PROF_SUB).
static std::atomic<uint64_t> g_prof_tsc[KHP_ZONE_N];
// KH_PROF_HOOKS: the hooks the engine calls thousands of times a frame join the sampled set.
inline bool kh_prof_sampled(KhProfZone khpz_z) {
    switch (khpz_z) {
    case KHP_HOOK_MAP: case KHP_UPLOAD_SCAN: case KHP_HOOK_UPDSUB: case KHP_HOOK_PSSRV: case KHP_HOOK_OMBLEND:
    case KHP_HOOK_OMDSS: case KHP_HOOK_COPYREGION: case KHP_HOOK_MISC: case KHP_DRAW_RECORD:
        return true;
    default:
        return false;
    }
}
// KH_PROF_SAMPLE_HASH: which calls are timed - a mix of the zone's call counter, not the counter's own residue. Do not
// use the residue: it locks onto call patterns (hookMap's Map and Unmap strictly alternate on one counter, so every
// timed call is an Unmap, or every one a Map). khpm_salt draws further decisions from the same count (the driver's
// timing, KH_PROF_DRV) independent of the first, so one's cost lands in the other's samples at its true rate.
inline uint32_t kh_prof_mix(uint32_t khpm_x, uint32_t khpm_salt) {
    khpm_x ^= khpm_salt * 0x9E3779B9u;
    khpm_x ^= khpm_x >> 16; khpm_x *= 0x7FEB352Du;
    khpm_x ^= khpm_x >> 15; khpm_x *= 0x846CA68Bu;
    khpm_x ^= khpm_x >> 16;
    return khpm_x;
}
inline bool kh_prof_pick(uint32_t khpk_c, uint32_t khpk_salt) {
    return (kh_prof_mix(khpk_c, khpk_salt) & (KH_PROF_SAMPLE_EVERY - 1u)) == 0u;
}
struct KhProfScope {
    KhProfZone khps_z;
    uint64_t   khps_t0;   // KH_PROF_TSC_CLOCK: the TSC at the entry.
    bool       khps_on;
    bool       khps_timed;
    uint32_t   khps_c;   // KH_PROF_SAMPLE_HASH: a sampled zone's call count (0 = none drawn).
    explicit KhProfScope(KhProfZone khps_zone)
        : khps_z(khps_zone), khps_t0(0), khps_on(kh_stats_on()), khps_timed(false), khps_c(0) {
        if (!khps_on) return;
        if (kh_prof_sampled(khps_zone)) {
            khps_c = g_prof_sample_ctr[khps_zone].fetch_add(1, std::memory_order_relaxed) + 1u;
            khps_timed = kh_prof_pick(khps_c, 0u);
        } else {
            khps_timed = true;
        }
        if (khps_timed) khps_t0 = __rdtsc();
    }
    ~KhProfScope() {
        if (!khps_on) return;
        g_prof_count[khps_z].fetch_add(1, std::memory_order_relaxed);
        if (!khps_timed) return;
        const uint64_t khps_t1 = __rdtsc();
        if (khps_t1 <= khps_t0) return;   // KH_PROF_TSC_CLOCK: a backwards read (unsynchronised cores) is dropped.
        const uint64_t khps_dt = khps_t1 - khps_t0;
        g_prof_tsc[khps_z].fetch_add(kh_prof_sampled(khps_z) ? khps_dt * KH_PROF_SAMPLE_EVERY : khps_dt,
                                     std::memory_order_relaxed);
    }
    KhProfScope(const KhProfScope&) = delete;
    KhProfScope& operator=(const KhProfScope&) = delete;
};
#define KH_PROF_SCOPE(z) KhProfScope khps_scope_##z(z)
// An explicit interval for a region a scope cannot bracket (a constructor's own body: the graphics lock's acquire).
// Same accounting, armed only.
inline int64_t kh_prof_now() { if (!kh_stats_on()) return 0; LARGE_INTEGER khpn_q = {}; QueryPerformanceCounter(&khpn_q); return khpn_q.QuadPart; }
inline void kh_prof_add(KhProfZone khpa_z, int64_t khpa_t0) {
    if (!kh_stats_on() || khpa_t0 == 0) return;
    LARGE_INTEGER khpa_q = {}; QueryPerformanceCounter(&khpa_q);
    g_prof_ticks[khpa_z].fetch_add(static_cast<uint64_t>(khpa_q.QuadPart - khpa_t0), std::memory_order_relaxed);
    g_prof_count[khpa_z].fetch_add(1, std::memory_order_relaxed);
}
// KH_PROF_SUB - the steps of one captured upload (KH_PROF_UPLOAD) and of one draw (KH_PROF_DRAWSTEP). Tens of
// thousands of entries a frame, so a QPC pair on each, or an atomic read-modify-write, would itself move the frame:
// one decision per upload / per draw (kh_prof_up_begin / KhProfDsDecision: armed, and one in KH_PROF_SAMPLE_EVERY
// timed), then each step counts with a plain add and, on a timed one, reads the TSC (__rdtsc) twice - scaled by its
// rate (khpb_scale: the sample rate, or 1 for a step timed whole). The fold converts the ticks with the TSC's rate it
// measured against QPC since the last armed fold (an invariant TSC, as QPC itself uses where it has one - without one,
// a CPU older than invariant TSCs, a zone is scaled by the interval's average rate: a measurement error only; a read
// that runs backwards - cores whose TSCs disagree - drops that sample). Written only by the
// render-thread identity (the hooks past reorder_on_render_thread, the funnel behind kh_upload_hook_wanted) and folded
// on it: relaxed loads and stores, no read-modify-write - a race would miscount, not corrupt.
static KhProfZone g_prof_up_cur = KHP_ZONE_N;   // The step in flight (scanExtBytes' owner); KHP_ZONE_N = none.
inline void kh_prof_plain_add(std::atomic<uint64_t>& khpa_a, uint64_t khpa_n) {
    khpa_a.store(khpa_a.load(std::memory_order_relaxed) + khpa_n, std::memory_order_relaxed);
}
// KH_PROF_DRV: the driver's own call inside a sampled hook scope (khpd_s), timed on one call in
// KH_PROF_SAMPLE_EVERY of its own (a second decision from the scope's count) with the TSC, and scaled. Its own
// decision, not the scope's: timed on the scope's timed calls, its cost would sit in every call the scope extrapolates.
// Any thread (the hooks'), so atomics; 0 = not timed (nothing recorded).
inline uint64_t kh_prof_drv_t0(const KhProfScope& khpd_s) {
    if (khpd_s.khps_c == 0u || !kh_prof_pick(khpd_s.khps_c, 1u)) return 0u;
    return __rdtsc();
}
inline void kh_prof_drv(KhProfZone khpd_z, uint64_t khpd_t0) {
    if (khpd_t0 == 0u) return;
    const uint64_t khpd_t1 = __rdtsc();
    if (khpd_t1 <= khpd_t0) return;   // KH_PROF_TSC_CLOCK: a backwards read is dropped.
    g_prof_tsc[khpd_z].fetch_add((khpd_t1 - khpd_t0) * KH_PROF_SAMPLE_EVERY, std::memory_order_relaxed);
    g_prof_count[khpd_z].fetch_add(KH_PROF_SAMPLE_EVERY, std::memory_order_relaxed);
}
// KH_PROF_TSC: a zone timed whole with the TSC, any thread - for a hook called on every draw whose work lands on a
// few of them (a 1-in-16 sample would read 0 or 16x a frame): two TSC reads and two atomic adds per armed call.
struct KhProfTscScope {
    KhProfZone khpt_z;
    uint64_t   khpt_t0;
    bool       khpt_on;
    explicit KhProfTscScope(KhProfZone khpt_zone) : khpt_z(khpt_zone), khpt_t0(0), khpt_on(kh_stats_on()) {
        if (khpt_on) khpt_t0 = __rdtsc();
    }
    ~KhProfTscScope() {
        if (!khpt_on) return;
        g_prof_count[khpt_z].fetch_add(1, std::memory_order_relaxed);
        const uint64_t khpt_t1 = __rdtsc();
        if (khpt_t1 > khpt_t0) g_prof_tsc[khpt_z].fetch_add(khpt_t1 - khpt_t0, std::memory_order_relaxed);
    }
    KhProfTscScope(const KhProfTscScope&) = delete;
    KhProfTscScope& operator=(const KhProfTscScope&) = delete;
};
#define KH_PROF_TSC(z) KhProfTscScope khpt_scope_##z(z)
struct KhProfSub {
    KhProfZone khpb_z;
    KhProfZone khpb_prev;
    uint64_t   khpb_t0;
    uint64_t   khpb_scale;
    bool       khpb_on;
    bool       khpb_timed;
    KhProfSub(KhProfZone khpb_zone, bool khpb_armed, bool khpb_time, uint64_t khpb_rate)
        : khpb_z(khpb_zone), khpb_prev(KHP_ZONE_N), khpb_t0(0), khpb_scale(khpb_rate), khpb_on(khpb_armed),
          khpb_timed(khpb_armed && khpb_time) {
        if (!khpb_on) return;
        kh_prof_plain_add(g_prof_count[khpb_z], 1u);
        khpb_prev = g_prof_up_cur;
        g_prof_up_cur = khpb_z;
        if (khpb_timed) khpb_t0 = __rdtsc();
    }
    ~KhProfSub() {
        if (!khpb_on) return;
        if (khpb_timed) {
            const uint64_t khpb_t1 = __rdtsc();
            if (khpb_t1 > khpb_t0) kh_prof_plain_add(g_prof_tsc[khpb_z], (khpb_t1 - khpb_t0) * khpb_scale);
        }
        g_prof_up_cur = khpb_prev;
    }
    KhProfSub(const KhProfSub&) = delete;
    KhProfSub& operator=(const KhProfSub&) = delete;
};
// A sampled step (timed on the upload's / draw's decision), and a step timed whole whenever it runs armed.
#define KH_PROF_SUB(z, on, timed) KhProfSub khpb_sub_##z((z), (on), (timed), KH_PROF_SAMPLE_EVERY)
#define KH_PROF_SUB_ALL(z, on) KhProfSub khpb_sub_##z((z), (on), true, 1u)
// The decisions (render-thread identity; KH_PROF_SAMPLE_HASH). kh_prof_up_begin: at the head of kh_upload_capture,
// the funnel's first step for every captured upload (Unmap and UpdateSubresource alike); KhProfDsDecision: past
// reorder_pre_draw's render-thread gate, for the rest of the draw. Disarmed: one relaxed load and two plain stores
// each (the bools), and every step (rendering_integration_injection.hpp) reads the bool.
static uint32_t g_prof_up_ctr = 0;
static bool     g_prof_up_on = false;
static bool     g_prof_up_t = false;
inline void kh_prof_up_begin() {
    g_prof_up_on = kh_stats_on();
    g_prof_up_t = g_prof_up_on && kh_prof_pick(++g_prof_up_ctr, 2u);
}
static uint32_t g_prof_ds_ctr = 0;
static bool     g_prof_ds_on = false;
static bool     g_prof_ds_t = false;
// khdd_own: one of our own draws (g_ro.in_injection) - not one of the engine's: no decision of its own, nothing
// counted, and the engine draw it is issued inside (a seam / PIP / in-front injection) gets its decision back.
struct KhProfDsDecision {
    bool khdd_own;
    bool khdd_on;
    bool khdd_t;
    explicit KhProfDsDecision(bool khdd_own_draw) : khdd_own(khdd_own_draw), khdd_on(false), khdd_t(false) {
        const bool khdd_armed = kh_stats_on();
        if (khdd_own && khdd_armed) {
            khdd_on = g_prof_ds_on;
            khdd_t = g_prof_ds_t;
            g_prof_ds_on = false;
            g_prof_ds_t = false;
            return;
        }
        khdd_own = false;
        g_prof_ds_on = khdd_armed;
        g_prof_ds_t = khdd_armed && kh_prof_pick(++g_prof_ds_ctr, 3u);
    }
    ~KhProfDsDecision() {
        if (!khdd_own) return;
        g_prof_ds_on = khdd_on;
        g_prof_ds_t = khdd_t;
    }
    KhProfDsDecision(const KhProfDsDecision&) = delete;
    KhProfDsDecision& operator=(const KhProfDsDecision&) = delete;
};
// KH_PROF_FUNNEL - counts and bytes per cycle (render-thread identity, armed only; plain adds as KhProfSub's). The
// upload funnel: captured uploads (UpdateSubresource's among them), the eager copies and bytes, the write-combined
// fences, the extensions past the eager copy (kh_upload_need / kh_upload_copy_range) and their bytes, the light
// capture's whole copies (kh_cbs_write) and bytes, in all and by what the sampler named the buffer (the want table's
// kind, tagged while armed: cb11, cb10, a VS slot, none yet), uploads the want table refused (a shadow, if any,
// marked stale), captures the idle cadence skipped, and the want table's index slots probed. The light sampler: the
// engine's draws past the render-thread gate, draws with no cb11 bound, uploads already processed this span, shadow
// misses (an incomplete side), twins of an authored window, new lists hashed and the bytes hashed, matches run and
// replayed (KH_DL_PRES_MEMO) and the record x pool-light pairs the runs stood against, windows authored and the VS
// bytes they copied; then the pairs the runs visited (KH_DL_PRES_HIX: the lights a record's height can match, or the
// whole pool), the height index's rebuilds and its in-place moves (a presence rewrite that moved a light's height).
// KH_MAP_MIX: the captured maps (UpdateSubresource's not among them) by kind - whole-buffer (WRITE_DISCARD) and
// partial (WRITE_NO_OVERWRITE) - each with the buffers' full ByteWidth summed (what copying the buffer whole at Unmap
// would move), and the whole-buffer maps by ByteWidth: <= 256, <= 1 KiB, <= 4 KiB, <= 16 KiB, larger.
// KH_MAP_REDIRECT: the whole-buffer maps handed a cached buffer and their ByteWidth (the bytes the Unmap copies to the
// driver's pointer), the whole-buffer maps with a pending slot that were not (no free record, no cached buffer to
// give, wider than the cap, the hooks not all installed, or KH_MAP_REDIRECT_ON false), and the cached buffers
// (re)assigned to a buffer (a first map, a new width, or the table's churn). KH_MAP_BRANCH: the Map and Unmap hooks'
// per-call branches - Maps through the hook, those whose forget (KH_REPLAY_CBREUSE) took g_rp_cbkeep_mx, those
// KH_REPLAY_SPLIT looked up in its set and those it split, those that scanned the redirect records (kh_redir_drop),
// those the upload gate admitted, and of these the ones given a pending slot and the ones finding every slot held;
// Unmaps through the hook, those that walked the pending slots, those that found theirs, those that scanned the
// redirect records (kh_redir_close); and the forgets that took the mutex from any hook. Counted on the hook scope's
// armed snapshot (khps_on) - the forgets' own count excepted: kh_rp_cb_forget has callers with no such scope and
// loads kh_stats_on() on its locked path, so on the call that straddles an arm or a disarm it and mapReplayLocks can
// differ by one. Behind the render-thread gate, plain adds as above; elsewhere any thread, so atomic adds (kh_fun_any).
enum KhFunnel : uint32_t {
    KHF_UPLOADS, KHF_UPDSUB, KHF_EAGER_N, KHF_EAGER_B, KHF_FENCES, KHF_EXT_N, KHF_EXT_B,
    KHF_SHADOW_N, KHF_SHADOW_B, KHF_SH11_N, KHF_SH11_B, KHF_SH10_N, KHF_SH10_B, KHF_SHVS_N, KHF_SHVS_B,
    KHF_SHOTHER_N, KHF_SHOTHER_B, KHF_WANT_REFUSED, KHF_IDLE_SKIP, KHF_GATE_PROBES,
    KHF_DRAWS, KHF_DL_NOLIST, KHF_DL_SAME, KHF_DL_MISS, KHF_DL_TWIN, KHF_DL_LISTS, KHF_DL_HASH_B,
    KHF_DL_RUNS, KHF_DL_REPLAYS, KHF_DL_PAIRS, KHF_DL_WINDOWS, KHF_DL_WIN_VS_B,
    KHF_DL_VISITS, KHF_DL_HIX_BUILDS, KHF_DL_HIX_MOVES,   // KH_DL_PRES_HIX.
    KHF_MAP_DISC_N, KHF_MAP_DISC_B, KHF_MAP_NOOV_N, KHF_MAP_NOOV_B,   // KH_MAP_MIX.
    KHF_MAP_W256, KHF_MAP_W1K, KHF_MAP_W4K, KHF_MAP_W16K, KHF_MAP_WBIG,
    KHF_REDIR_N, KHF_REDIR_B, KHF_REDIR_REFUSED, KHF_REDIR_BUFS,   // KH_MAP_REDIRECT.
    KHF_HK_MAP_N, KHF_HK_RP_LOCK, KHF_HK_RP_CHECK, KHF_HK_RP_SPLIT, KHF_HK_RD_DROP,   // KH_MAP_BRANCH.
    KHF_HK_ADMIT, KHF_HK_PENDING, KHF_HK_NOSLOT,
    KHF_HK_UNMAP_N, KHF_HK_UN_WALK, KHF_HK_UN_HIT, KHF_HK_RD_CLOSE, KHF_RP_LOCKS,
    KHF_N
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
static_assert(sizeof(g_fun_name) / sizeof(g_fun_name[0]) == KHF_N, "KH_PROF_FUNNEL: a name per counter");
static std::atomic<uint64_t> g_fun_acc[KHF_N];   // This cycle's (render-thread identity).
static std::atomic<uint64_t> g_fun_pub[KHF_N];   // The ended cycle's (the fold).
static std::atomic<uint64_t> g_fun_win[KHF_N];   // Summed over the window (KH_PROF_WINDOW).
inline void kh_fun(KhFunnel khfn_k, uint64_t khfn_n) { kh_prof_plain_add(g_fun_acc[khfn_k], khfn_n); }
// KH_MAP_BRANCH: a count made off the render-thread gate (any thread): an atomic add, armed callers only, and only to
// a counter kh_fun never adds to.
inline void kh_fun_any(KhFunnel khfa_k, uint64_t khfa_n) {
    g_fun_acc[khfa_k].fetch_add(khfa_n, std::memory_order_relaxed);
}
// The want table's kinds (kh_dl_want_note's tag; a buffer named in more than one role carries each bit).
static constexpr uint8_t KH_DL_KIND_CB11 = 1u;
static constexpr uint8_t KH_DL_KIND_CB10 = 2u;
static constexpr uint8_t KH_DL_KIND_VS = 4u;
inline void kh_fun_shadow(uint8_t khfs_kind, uint64_t khfs_b) {
    kh_fun(KHF_SHADOW_N, 1u);
    kh_fun(KHF_SHADOW_B, khfs_b);
    if (khfs_kind & KH_DL_KIND_CB11)     { kh_fun(KHF_SH11_N, 1u); kh_fun(KHF_SH11_B, khfs_b); }
    else if (khfs_kind & KH_DL_KIND_CB10) { kh_fun(KHF_SH10_N, 1u); kh_fun(KHF_SH10_B, khfs_b); }
    else if (khfs_kind & KH_DL_KIND_VS)   { kh_fun(KHF_SHVS_N, 1u); kh_fun(KHF_SHVS_B, khfs_b); }
    else                                  { kh_fun(KHF_SHOTHER_N, 1u); kh_fun(KHF_SHOTHER_B, khfs_b); }
}
// KH_MAP_MIX: one captured map - its kind and its buffer's full ByteWidth (armed callers only).
inline void kh_fun_map_mix(bool khmm_discard, uint32_t khmm_w) {
    if (!khmm_discard) {
        kh_fun(KHF_MAP_NOOV_N, 1u);
        kh_fun(KHF_MAP_NOOV_B, khmm_w);
        return;
    }
    kh_fun(KHF_MAP_DISC_N, 1u);
    kh_fun(KHF_MAP_DISC_B, khmm_w);
    if (khmm_w <= 256u)        kh_fun(KHF_MAP_W256, 1u);
    else if (khmm_w <= 1024u)  kh_fun(KHF_MAP_W1K, 1u);
    else if (khmm_w <= 4096u)  kh_fun(KHF_MAP_W4K, 1u);
    else if (khmm_w <= 16384u) kh_fun(KHF_MAP_W16K, 1u);
    else                       kh_fun(KHF_MAP_WBIG, 1u);
}
// scanExtBytes: the extension bytes each step made the funnel copy (the step in flight, g_prof_up_cur).
static std::atomic<uint64_t> g_prof_xb_acc[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_xb_pub[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_xb_win[KHP_ZONE_N];
inline void kh_fun_ext(uint64_t khfe_b) {
    kh_fun(KHF_EXT_N, 1u);
    kh_fun(KHF_EXT_B, khfe_b);
    if (g_prof_up_cur < KHP_ZONE_N) kh_prof_plain_add(g_prof_xb_acc[g_prof_up_cur], khfe_b);
}
// KH_PROF_WINDOW - one cycle is one sample (a 1-in-16 zone multiplies one interrupted call by 16; a step that runs
// once a frame lands on the cycle or not), so every whole armed cycle (a published fold, KH_PROF_DISARM, whose
// previous fold was armed too) also adds to these: each zone's mean and worst cycle since the arming or the last
// resetRenderStats, for getRenderStats.
static std::atomic<uint64_t> g_prof_win_cycles{ 0 };
static std::atomic<uint64_t> g_prof_win_us[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_win_n[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_win_max[KHP_ZONE_N];
// KH_PROF_SUB's rate: TSC ticks per microsecond, measured at each armed fold over the interval since the previous one
// - the conversion of every TSC-timed zone (every CPU zone but kh_prof_add's QPC intervals). 0 until two armed folds
// have run in this process (those zones publish 0 until then); kept across resets (the machine's rate, measured again
// by every armed fold). Render-thread identity writes.
static uint64_t g_prof_cal_q = 0;
static uint64_t g_prof_cal_t = 0;
static std::atomic<double> g_prof_tsc_per_us{ 0.0 };
// KH_PROF_WINDOW: the last fold found collection on (render thread). The window takes whole armed cycles only: the
// cycle the arming split stays out of the means (its fold publishes the counts and kh_prof_add's QPC zones - nothing
// at all when it is also the first fold after a reset, KH_PROF_DISARM; the TSC zones, scanExtBytes and the funnel
// read 0 there).
static bool g_prof_prev_on = false;

// KH_WSEE_GPUT - GPU timing of the see-through record's own work (stats alone: every write under kh_stats_on; nothing
// drawn or decided reads it). KHG_WSEE / KHG_WSEE_B2 time the capture and B2 alone - the re-issues run inside the
// engine's draw stream and no zone brackets them. One frame in KH_WSEE_GPUT_EVERY while armed and capturing (cycle %
// 16 == KH_WSEE_GPUT_AT) ends a timestamp at each re-issue's save and restore (kh_wsee_re_save / kh_wsee_re_restore),
// before each particle-type pass (kh_wsee_re_set) and around the capture, B2, our tail, the chain's E draw and D3's
// resolve (KhWseeGputScope); the frame is one disjoint query from its clear to the next. Read back at later clears
// without a flush or a wait (at most KH_WSEE_GPUT_READ stamps a clear; a frame not read whole within KH_WSEE_GPUT_WAIT
// clears is dropped; a ring of KH_WSEE_GPUT_RING frames). Per kind: the sum of its intervals in the frame and their
// count (getRenderStats "wseeGpu"). An interval is elapsed GPU time - the pipeline's drain and refill around the draws
// included, never less than their own time: a sum over many short intervals over-states their marginal cost, and the
// probe frame runs slower by its stamps. A block that would not fit (KH_WSEE_GPUT_N stamps a frame, eight kept for a
// block's own) or that opens inside another goes untimed (counted). The chain's E draw and the resolve stamp on the
// chain's thread (the render thread, a park that stalls it, or the game thread's late chain inside a resolve the
// engine issues there: the engine never drives the immediate context from two threads at once). Released with the
// KH_PROF queries (kh_gpu_prof_release: the device, the session); a refused make latches until then. The first probe
// frame makes 8,194 queries inside the main depth's clear hook (a render-thread hitch of a few ms, once per session
// and device reset); the clears after a probe frame issue up to KH_WSEE_GPUT_READ GetData calls each. KHWG_P_OTHER: a
// pass without a kind of its own: kh_wsee_gput_pk's catch-all (none today: every particle-type pass has its own,
// and the one-pass kinds - a late opaque, a fence-type layer - take no pass stamp).
enum KhWseeGputKind : uint32_t {
    KHWG_OPQ_POST, KHWG_FENCE, KHWG_PART, KHWG_P_T, KHWG_P_NW, KHWG_P_TP, KHWG_P_TP2, KHWG_P_TF,
    KHWG_P_OTHER, KHWG_TAIL, KHWG_CAPTURE, KHWG_B2, KHWG_CHAIN_E, KHWG_D3RES,
    KHWG_P_ONE,   // KH_PSB: the one pass (a particle-type re-issue drawn once).
    KHWG_N
};
static const char* const g_wsee_gput_name[KHWG_N] = {
    "opqPost", "fence", "particle", "pT", "pNW", "pTp", "pTp2", "pTf",
    "pOther", "tail", "capture", "b2", "chainE", "d3Resolve",
    "pOne",
};
static constexpr uint32_t KH_WSEE_GPUT_N = 4096u;
static constexpr uint32_t KH_WSEE_GPUT_RING = 2u;
static constexpr uint32_t KH_WSEE_GPUT_READ = 1024u;
static constexpr uint64_t KH_WSEE_GPUT_EVERY = 16u;
static constexpr uint64_t KH_WSEE_GPUT_AT = 8u;
static constexpr uint64_t KH_WSEE_GPUT_WAIT = 14u;
// A stamp's event: the kind in the low six bits, the role above (a block's begin, a pass's start, a block's end).
static constexpr uint8_t KHWG_EV_BEGIN = 0x00u, KHWG_EV_PASS = 0x40u, KHWG_EV_END = 0x80u;
struct KhWseeGput {
    ID3D11Query* dj[KH_WSEE_GPUT_RING] = {};
    ID3D11Query* ts[KH_WSEE_GPUT_RING][KH_WSEE_GPUT_N] = {};
    uint8_t      ev[KH_WSEE_GPUT_RING][KH_WSEE_GPUT_N] = {};
    uint64_t     tv[KH_WSEE_GPUT_RING][KH_WSEE_GPUT_N] = {};   // The values read back.
    uint32_t     n[KH_WSEE_GPUT_RING] = {};      // Stamps ended in the slot's frame.
    uint32_t     rd[KH_WSEE_GPUT_RING] = {};     // Of them read back.
    uint64_t     cyc[KH_WSEE_GPUT_RING] = {};
    uint64_t     freq[KH_WSEE_GPUT_RING] = {};   // The disjoint query's (0 = not read yet).
    bool         wait[KH_WSEE_GPUT_RING] = {};
    int          open = -1;    // The slot recording (this frame), -1 = none.
    uint32_t     next = 0;     // The slot the next probe frame takes.
    int          blk = -1;     // The open block's begin stamp in the open slot, -1 = none.
    bool         ready = false;
    bool         failed = false;
};
static KhWseeGput g_wsee_gput;
// Published (the harvest writes, getRenderStats reads): per kind the last frame read's ns and intervals, and the sums
// over every frame read since the arming / resetRenderStats; the frames read, dropped, and the blocks left untimed.
static std::atomic<uint64_t> g_wsee_gput_ns_last[KHWG_N];
static std::atomic<uint64_t> g_wsee_gput_n_last[KHWG_N];
static std::atomic<uint64_t> g_wsee_gput_ns_sum[KHWG_N];
static std::atomic<uint64_t> g_wsee_gput_n_sum[KHWG_N];
static std::atomic<uint64_t> g_wsee_gput_read{ 0 };
static std::atomic<uint64_t> g_wsee_gput_drop{ 0 };
static std::atomic<uint64_t> g_wsee_gput_untimed{ 0 };
inline void kh_wsee_gput_zero_pub() {   // resetRenderStats (reset_stat_counters).
    for (uint32_t khgz_k = 0; khgz_k < KHWG_N; ++khgz_k) {
        g_wsee_gput_ns_last[khgz_k].store(0, std::memory_order_relaxed);
        g_wsee_gput_n_last[khgz_k].store(0, std::memory_order_relaxed);
        g_wsee_gput_ns_sum[khgz_k].store(0, std::memory_order_relaxed);
        g_wsee_gput_n_sum[khgz_k].store(0, std::memory_order_relaxed);
    }
    g_wsee_gput_read.store(0, std::memory_order_relaxed);
    g_wsee_gput_drop.store(0, std::memory_order_relaxed);
    g_wsee_gput_untimed.store(0, std::memory_order_relaxed);
}
inline void kh_wsee_gput_release() {
    for (uint32_t khgr_s = 0; khgr_s < KH_WSEE_GPUT_RING; ++khgr_s) {
        KH_SAFE_RELEASE(g_wsee_gput.dj[khgr_s]);
        for (ID3D11Query*& khgr_q : g_wsee_gput.ts[khgr_s]) KH_SAFE_RELEASE(khgr_q);
        g_wsee_gput.n[khgr_s] = g_wsee_gput.rd[khgr_s] = 0;
        g_wsee_gput.freq[khgr_s] = 0;
        g_wsee_gput.wait[khgr_s] = false;
    }
    g_wsee_gput.open = -1;
    g_wsee_gput.next = 0;
    g_wsee_gput.blk = -1;
    g_wsee_gput.ready = false;
    g_wsee_gput.failed = false;
}
inline bool kh_wsee_gput_ensure(ID3D11DeviceContext* khge_ctx) {
    if (g_wsee_gput.ready) return true;
    if (g_wsee_gput.failed || !khge_ctx) return false;
    ID3D11Device* khge_dev = nullptr;
    khge_ctx->GetDevice(&khge_dev);
    bool khge_ok = khge_dev != nullptr;
    D3D11_QUERY_DESC khge_dd = {};
    khge_dd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    D3D11_QUERY_DESC khge_td = {};
    khge_td.Query = D3D11_QUERY_TIMESTAMP;
    for (uint32_t khge_s = 0; khge_s < KH_WSEE_GPUT_RING && khge_ok; ++khge_s) {
        khge_ok = SUCCEEDED(khge_dev->CreateQuery(&khge_dd, &g_wsee_gput.dj[khge_s]));
        for (uint32_t khge_i = 0; khge_i < KH_WSEE_GPUT_N && khge_ok; ++khge_i) {
            khge_ok = SUCCEEDED(khge_dev->CreateQuery(&khge_td, &g_wsee_gput.ts[khge_s][khge_i]));
        }
    }
    if (khge_dev) khge_dev->Release();
    if (!khge_ok) {
        kh_wsee_gput_release();
        g_wsee_gput.failed = true;
        return false;
    }
    g_wsee_gput.ready = true;
    return true;
}
// A block's begin (khgb_k its kind, or KHWG_N: the first pass's, kh_wsee_gput_pass).
inline void kh_wsee_gput_begin(ID3D11DeviceContext* khgb_ctx, uint32_t khgb_k) {
    if (g_wsee_gput.open < 0 || !kh_stats_on()) return;
    const uint32_t khgb_s = static_cast<uint32_t>(g_wsee_gput.open);
    if (g_wsee_gput.blk >= 0 || g_wsee_gput.n[khgb_s] + 8u > KH_WSEE_GPUT_N) {
        g_wsee_gput_untimed.fetch_add(1u, std::memory_order_relaxed);
        return;
    }
    const uint32_t khgb_i = g_wsee_gput.n[khgb_s]++;
    g_wsee_gput.ev[khgb_s][khgb_i] = static_cast<uint8_t>(KHWG_EV_BEGIN | (khgb_k & 0x3Fu));
    khgb_ctx->End(g_wsee_gput.ts[khgb_s][khgb_i]);
    g_wsee_gput.blk = static_cast<int>(khgb_i);
}
// A pass of the open block: the block's kind where its begin left it open (khgp_bk), and a pass stamp of kind khgp_pk
// (KHWG_N: none - the one-pass kinds take the block's interval alone). One stamp is always left for the end.
inline void kh_wsee_gput_pass(ID3D11DeviceContext* khgp_ctx, uint32_t khgp_bk, uint32_t khgp_pk) {
    if (g_wsee_gput.blk < 0 || g_wsee_gput.open < 0) return;
    const uint32_t khgp_s = static_cast<uint32_t>(g_wsee_gput.open);
    uint8_t& khgp_b = g_wsee_gput.ev[khgp_s][static_cast<uint32_t>(g_wsee_gput.blk)];
    if ((khgp_b & 0x3Fu) == KHWG_N) khgp_b = static_cast<uint8_t>(KHWG_EV_BEGIN | (khgp_bk & 0x3Fu));
    if (khgp_pk >= KHWG_N || g_wsee_gput.n[khgp_s] + 1u >= KH_WSEE_GPUT_N) return;
    const uint32_t khgp_i = g_wsee_gput.n[khgp_s]++;
    g_wsee_gput.ev[khgp_s][khgp_i] = static_cast<uint8_t>(KHWG_EV_PASS | (khgp_pk & 0x3Fu));
    khgp_ctx->End(g_wsee_gput.ts[khgp_s][khgp_i]);
}
inline void kh_wsee_gput_end(ID3D11DeviceContext* khgn_ctx) {
    if (g_wsee_gput.blk < 0 || g_wsee_gput.open < 0) return;
    const uint32_t khgn_s = static_cast<uint32_t>(g_wsee_gput.open);
    const uint32_t khgn_i = g_wsee_gput.n[khgn_s]++;   // Room: the begin kept eight, a pass leaves one.
    g_wsee_gput.ev[khgn_s][khgn_i] = KHWG_EV_END;
    khgn_ctx->End(g_wsee_gput.ts[khgn_s][khgn_i]);
    g_wsee_gput.blk = -1;
}
// A whole-kind block (the capture, B2, our tail, E, the resolve): its begin at construction, its end at destruction
// where this scope opened it. Nothing in it may set a slot (only the clear does: kh_wsee_gput_head).
struct KhWseeGputScope {
    ID3D11DeviceContext* khgs_ctx;
    bool                 khgs_on;
    KhWseeGputScope(ID3D11DeviceContext* khgs_c, uint32_t khgs_k) : khgs_ctx(khgs_c), khgs_on(false) {
        if (g_wsee_gput.open < 0 || g_wsee_gput.blk >= 0 || !khgs_ctx) {
            if (g_wsee_gput.open >= 0 && g_wsee_gput.blk >= 0 && kh_stats_on()) {
                g_wsee_gput_untimed.fetch_add(1u, std::memory_order_relaxed);
            }
            return;
        }
        kh_wsee_gput_begin(khgs_ctx, khgs_k);
        khgs_on = g_wsee_gput.blk >= 0;
    }
    ~KhWseeGputScope() {
        if (khgs_on) kh_wsee_gput_end(khgs_ctx);
    }
    KhWseeGputScope(const KhWseeGputScope&) = delete;
    KhWseeGputScope& operator=(const KhWseeGputScope&) = delete;
};
// A read slot's frame folded: each block's interval to its kind (a block no pass named - the re-issue's stencil
// backstop, which draws nothing - to the particle kind), each pass's to the pass kind, to its block's end or the next
// pass; published as the last frame and added to the sums.
inline void kh_wsee_gput_fold(uint32_t khgf_s) {
    uint64_t khgf_ns[KHWG_N] = {};
    uint64_t khgf_n[KHWG_N] = {};
    const double khgf_f = static_cast<double>(g_wsee_gput.freq[khgf_s]);
    const auto khgf_add = [&](uint32_t khgf_k, uint64_t khgf_a, uint64_t khgf_b) {
        if (khgf_k >= KHWG_N || khgf_b < khgf_a) return;
        khgf_ns[khgf_k] += static_cast<uint64_t>(static_cast<double>(khgf_b - khgf_a) * 1.0e9 / khgf_f);
        ++khgf_n[khgf_k];
    };
    int khgf_bi = -1, khgf_pi = -1;
    uint32_t khgf_bk = KHWG_N, khgf_pk = KHWG_N;
    for (uint32_t khgf_i = 0; khgf_i < g_wsee_gput.n[khgf_s]; ++khgf_i) {
        const uint8_t khgf_e = g_wsee_gput.ev[khgf_s][khgf_i];
        const uint64_t khgf_t = g_wsee_gput.tv[khgf_s][khgf_i];
        if ((khgf_e & 0xC0u) == KHWG_EV_BEGIN) {
            khgf_bi = static_cast<int>(khgf_i);
            khgf_bk = khgf_e & 0x3Fu;
            khgf_pi = -1;
        } else if (khgf_bi >= 0 && (khgf_e & 0xC0u) == KHWG_EV_PASS) {
            if (khgf_pi >= 0) khgf_add(khgf_pk, g_wsee_gput.tv[khgf_s][static_cast<uint32_t>(khgf_pi)], khgf_t);
            khgf_pi = static_cast<int>(khgf_i);
            khgf_pk = khgf_e & 0x3Fu;
        } else if (khgf_bi >= 0 && (khgf_e & 0xC0u) == KHWG_EV_END) {
            if (khgf_pi >= 0) khgf_add(khgf_pk, g_wsee_gput.tv[khgf_s][static_cast<uint32_t>(khgf_pi)], khgf_t);
            khgf_add(khgf_bk >= KHWG_N ? static_cast<uint32_t>(KHWG_PART) : khgf_bk,
                     g_wsee_gput.tv[khgf_s][static_cast<uint32_t>(khgf_bi)], khgf_t);
            khgf_bi = khgf_pi = -1;
        }
    }
    for (uint32_t khgf_k = 0; khgf_k < KHWG_N; ++khgf_k) {
        g_wsee_gput_ns_last[khgf_k].store(khgf_ns[khgf_k], std::memory_order_relaxed);
        g_wsee_gput_n_last[khgf_k].store(khgf_n[khgf_k], std::memory_order_relaxed);
        g_wsee_gput_ns_sum[khgf_k].fetch_add(khgf_ns[khgf_k], std::memory_order_relaxed);
        g_wsee_gput_n_sum[khgf_k].fetch_add(khgf_n[khgf_k], std::memory_order_relaxed);
    }
    g_wsee_gput_read.fetch_add(1u, std::memory_order_relaxed);
}
// The main depth's clear (render thread; khgh_c the new cycle, khgh_cap whether it captures - g_wsee_on): the frame
// recorded ends (its disjoint query; a block still open there is dropped with what followed it), the frames awaited
// are read as far as they are done, and a probe frame opens. Disarmed: nothing recorded, nothing awaited.
inline void kh_wsee_gput_head(ID3D11DeviceContext* khgh_ctx, uint64_t khgh_c, bool khgh_cap) {
    if (g_wsee_gput.open >= 0) {
        const uint32_t khgh_o = static_cast<uint32_t>(g_wsee_gput.open);
        if (g_wsee_gput.blk >= 0) g_wsee_gput.n[khgh_o] = static_cast<uint32_t>(g_wsee_gput.blk);
        khgh_ctx->End(g_wsee_gput.dj[khgh_o]);
        g_wsee_gput.wait[khgh_o] = true;
        g_wsee_gput.open = -1;
        g_wsee_gput.blk = -1;
    }
    if (!kh_stats_on()) {
        for (bool& khgh_w : g_wsee_gput.wait) khgh_w = false;
        return;
    }
    uint32_t khgh_budget = KH_WSEE_GPUT_READ;
    for (uint32_t khgh_s = 0; khgh_s < KH_WSEE_GPUT_RING; ++khgh_s) {
        if (!g_wsee_gput.wait[khgh_s]) continue;
        if (khgh_c - g_wsee_gput.cyc[khgh_s] > KH_WSEE_GPUT_WAIT) {
            g_wsee_gput.wait[khgh_s] = false;
            g_wsee_gput_drop.fetch_add(1u, std::memory_order_relaxed);
            continue;
        }
        if (g_wsee_gput.freq[khgh_s] == 0) {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT khgh_dj = {};
            if (khgh_ctx->GetData(g_wsee_gput.dj[khgh_s], &khgh_dj, sizeof(khgh_dj),
                                  D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) continue;
            if (khgh_dj.Disjoint || khgh_dj.Frequency == 0) {
                g_wsee_gput.wait[khgh_s] = false;
                g_wsee_gput_drop.fetch_add(1u, std::memory_order_relaxed);
                continue;
            }
            g_wsee_gput.freq[khgh_s] = khgh_dj.Frequency;
        }
        while (khgh_budget > 0 && g_wsee_gput.rd[khgh_s] < g_wsee_gput.n[khgh_s]) {
            UINT64 khgh_t = 0;
            if (khgh_ctx->GetData(g_wsee_gput.ts[khgh_s][g_wsee_gput.rd[khgh_s]], &khgh_t, sizeof(khgh_t),
                                  D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) break;
            g_wsee_gput.tv[khgh_s][g_wsee_gput.rd[khgh_s]++] = khgh_t;
            --khgh_budget;
        }
        if (g_wsee_gput.rd[khgh_s] == g_wsee_gput.n[khgh_s]) {
            kh_wsee_gput_fold(khgh_s);
            g_wsee_gput.wait[khgh_s] = false;
        }
    }
    if (khgh_cap && khgh_c % KH_WSEE_GPUT_EVERY == KH_WSEE_GPUT_AT && !g_wsee_gput.wait[g_wsee_gput.next] &&
        kh_wsee_gput_ensure(khgh_ctx)) {
        const uint32_t khgh_s = g_wsee_gput.next;
        g_wsee_gput.n[khgh_s] = g_wsee_gput.rd[khgh_s] = 0;
        g_wsee_gput.freq[khgh_s] = 0;
        g_wsee_gput.cyc[khgh_s] = khgh_c;
        khgh_ctx->Begin(g_wsee_gput.dj[khgh_s]);
        g_wsee_gput.open = static_cast<int>(khgh_s);
        g_wsee_gput.next = (khgh_s + 1u) % KH_WSEE_GPUT_RING;
    }
}

enum KhGpuZone : uint32_t {
    KHG_INJECT, KHG_SUN_LADDER, KHG_DLS_FRAME, KHG_DLS_WORLD, KHG_MASK_CAST, KHG_SVS_PRIME,
    KHG_SVS_VOL_COPY, KHG_VMIR, KHG_SSAO_POST, KHG_SNAPSHOT, KHG_PIP,
    KHG_FLUSH, KHG_FLUSH_UI, KHG_SCENE_CAPTURE, KHG_DLF,
    KHG_WSEE,   // KH_WSEE: the capture (kh_wsee_snapshot) alone - one pair a zone a cycle, so no re-issue is timed.
    KHG_WSEE_B2,   // B2 and S2 (kh_wsee_b2_take), once a cycle.
    KHG_ZONE_N
};
static const char* const g_gpu_zone_name[KHG_ZONE_N] = {
    "gInject", "gSunLadder", "gDlsFrame", "gDlsWorld", "gMaskCast", "gSvsPrime",
    "gSvsVolCopy", "gVmir", "gSsaoPost", "gSnapshot", "gPip",
    "gFlush", "gFlushUi", "gSceneCapture", "gDlFog",
    "gWsee", "gWseeB2",
};
static constexpr uint32_t KH_GPU_RING = 8u;
// The cycles back a slot is read, never waiting (< KH_GPU_RING): the GPU was measured running 5 cycles behind the CPU
// under load at 4K, and a slot read sooner is never done. The ring reopens it KH_GPU_RING - KH_GPU_LAG folds later.
static constexpr uint32_t KH_GPU_LAG = 6u;
// KH_GPU_PASS: the effect passes a cycle times (the scene chain's and the UI chain's), each its own timestamp pair in
// the cycle's bracket, keyed by kh_gpu_pass_key; past this many in a cycle the rest go untimed.
static constexpr uint32_t KH_GPU_PASS_N = 32u;
struct KhGpuProf {
    ID3D11Query* disjoint[KH_GPU_RING] = {};
    ID3D11Query* ts[KHG_ZONE_N][KH_GPU_RING][2] = {};
    bool         used[KHG_ZONE_N][KH_GPU_RING] = {};
    ID3D11Query* pts[KH_GPU_RING][KH_GPU_PASS_N][2] = {};   // KH_GPU_PASS.
    uint32_t     pass_n[KH_GPU_RING] = {};
    uint32_t     pass_key[KH_GPU_RING][KH_GPU_PASS_N] = {};
    bool         pass_used[KH_GPU_RING][KH_GPU_PASS_N] = {};
    bool         ready = false;    // Queries exist.
    bool         failed = false;   // A create failed: the GPU side stands down for the session.
    int          open_slot = -1;   // Slot whose disjoint is begun, -1 = none.
    uint64_t     cycle = 0;        // Cycles folded (ring index source).
};
static KhGpuProf g_gpu_prof;
static std::atomic<uint64_t> g_gpu_us_pub[KHG_ZONE_N];
// KH_PROF_WINDOW: every GPU value the fold harvested since the arming / reset (sum, count, worst).
static std::atomic<uint64_t> g_gpu_win_us[KHG_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_n[KHG_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_max[KHG_ZONE_N];
// KH_GPU_PASS: a pass's key - the effect id (0 - 255), the chain (bit 8: 0 the scene's, 1 the UI's), a user shader
// (bit 9), the stages fused into it (bits 10 - 15, at most 63).
inline uint32_t kh_gpu_pass_key(uint32_t khgk_chain, int khgk_effect, bool khgk_user, uint32_t khgk_fused) {
    const uint32_t khgk_e = khgk_effect < 0 ? 0u : (khgk_effect > 255 ? 255u : static_cast<uint32_t>(khgk_effect));
    const uint32_t khgk_f = khgk_fused > 63u ? 63u : khgk_fused;
    return khgk_e | ((khgk_chain & 1u) << 8) | ((khgk_user ? 1u : 0u) << 9) | (khgk_f << 10);
}
// The harvested cycle's passes (key << 32 | microseconds), in draw order, and how many; and the window per (chain,
// user, effect) - the key's low ten bits (key & (KH_GPU_PASS_W - 1)): sum, count, worst.
static constexpr uint32_t KH_GPU_PASS_W = 1024u;
static std::atomic<uint64_t> g_gpu_pass_pub[KH_GPU_PASS_N];
static std::atomic<uint32_t> g_gpu_pass_pub_n{ 0 };
static std::atomic<uint64_t> g_gpu_passw_us[KH_GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_n[KH_GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_max[KH_GPU_PASS_W];
// KH_PROF_WINDOW / KH_PROF_FUNNEL / KH_GPU_PASS: the window starts again and the published values go to zero - by
// resetRenderStats (kh_prof_reset_pub) and by the first fold after it (KH_PROF_DISARM).
inline void kh_prof_window_zero() {
    g_prof_win_cycles.store(0, std::memory_order_relaxed);
    for (uint32_t khwz_z = 0; khwz_z < KHP_ZONE_N; ++khwz_z) {
        g_prof_win_us[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_win_n[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_win_max[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_xb_pub[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_xb_win[khwz_z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t khwz_z = 0; khwz_z < KHG_ZONE_N; ++khwz_z) {
        g_gpu_win_us[khwz_z].store(0, std::memory_order_relaxed);
        g_gpu_win_n[khwz_z].store(0, std::memory_order_relaxed);
        g_gpu_win_max[khwz_z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t khwz_k = 0; khwz_k < KHF_N; ++khwz_k) {
        g_fun_pub[khwz_k].store(0, std::memory_order_relaxed);
        g_fun_win[khwz_k].store(0, std::memory_order_relaxed);
    }
    g_gpu_pass_pub_n.store(0, std::memory_order_relaxed);
    for (uint32_t khwz_w = 0; khwz_w < KH_GPU_PASS_W; ++khwz_w) {
        g_gpu_passw_us[khwz_w].store(0, std::memory_order_relaxed);
        g_gpu_passw_n[khwz_w].store(0, std::memory_order_relaxed);
        g_gpu_passw_max[khwz_w].store(0, std::memory_order_relaxed);
    }
}
inline uint32_t kh_gpu_slot() { return static_cast<uint32_t>(g_gpu_prof.cycle % KH_GPU_RING); }
inline bool kh_gpu_prof_ensure(ID3D11DeviceContext* khgp_ctx) {
    if (g_gpu_prof.ready) return true;
    if (g_gpu_prof.failed || !khgp_ctx) return false;
    ID3D11Device* khgp_dev = nullptr;
    khgp_ctx->GetDevice(&khgp_dev);
    if (!khgp_dev) { g_gpu_prof.failed = true; return false; }
    bool khgp_ok = true;
    for (uint32_t khgp_r = 0; khgp_r < KH_GPU_RING && khgp_ok; ++khgp_r) {
        D3D11_QUERY_DESC khgp_dd = {}; khgp_dd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (FAILED(khgp_dev->CreateQuery(&khgp_dd, &g_gpu_prof.disjoint[khgp_r]))) khgp_ok = false;
        for (uint32_t khgp_z = 0; khgp_z < KHG_ZONE_N && khgp_ok; ++khgp_z) {
            D3D11_QUERY_DESC khgp_td = {}; khgp_td.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(khgp_dev->CreateQuery(&khgp_td, &g_gpu_prof.ts[khgp_z][khgp_r][0])) ||
                FAILED(khgp_dev->CreateQuery(&khgp_td, &g_gpu_prof.ts[khgp_z][khgp_r][1]))) khgp_ok = false;
        }
        for (uint32_t khgp_p = 0; khgp_p < KH_GPU_PASS_N && khgp_ok; ++khgp_p) {   // KH_GPU_PASS.
            D3D11_QUERY_DESC khgp_pd = {}; khgp_pd.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(khgp_dev->CreateQuery(&khgp_pd, &g_gpu_prof.pts[khgp_r][khgp_p][0])) ||
                FAILED(khgp_dev->CreateQuery(&khgp_pd, &g_gpu_prof.pts[khgp_r][khgp_p][1]))) khgp_ok = false;
        }
    }
    khgp_dev->Release();
    if (!khgp_ok) {
        for (uint32_t khgp_r = 0; khgp_r < KH_GPU_RING; ++khgp_r) {
            KH_SAFE_RELEASE(g_gpu_prof.disjoint[khgp_r]);
            for (uint32_t khgp_z = 0; khgp_z < KHG_ZONE_N; ++khgp_z) { KH_SAFE_RELEASE(g_gpu_prof.ts[khgp_z][khgp_r][0]); KH_SAFE_RELEASE(g_gpu_prof.ts[khgp_z][khgp_r][1]); }
            for (uint32_t khgp_p = 0; khgp_p < KH_GPU_PASS_N; ++khgp_p) {   // KH_GPU_PASS.
                KH_SAFE_RELEASE(g_gpu_prof.pts[khgp_r][khgp_p][0]);
                KH_SAFE_RELEASE(g_gpu_prof.pts[khgp_r][khgp_p][1]);
            }
        }
        g_gpu_prof.failed = true;
        return false;
    }
    g_gpu_prof.ready = true;
    return true;
}
inline void kh_gpu_prof_release() {
    kh_wsee_gput_release();   // KH_WSEE_GPUT: its queries share the KH_PROF set's lifetime.
    for (uint32_t khgr_r = 0; khgr_r < KH_GPU_RING; ++khgr_r) {
        KH_SAFE_RELEASE(g_gpu_prof.disjoint[khgr_r]);
        for (uint32_t khgr_z = 0; khgr_z < KHG_ZONE_N; ++khgr_z) {
            KH_SAFE_RELEASE(g_gpu_prof.ts[khgr_z][khgr_r][0]);
            KH_SAFE_RELEASE(g_gpu_prof.ts[khgr_z][khgr_r][1]);
            g_gpu_prof.used[khgr_z][khgr_r] = false;
        }
        for (uint32_t khgr_p = 0; khgr_p < KH_GPU_PASS_N; ++khgr_p) {   // KH_GPU_PASS.
            KH_SAFE_RELEASE(g_gpu_prof.pts[khgr_r][khgr_p][0]);
            KH_SAFE_RELEASE(g_gpu_prof.pts[khgr_r][khgr_p][1]);
            g_gpu_prof.pass_used[khgr_r][khgr_p] = false;
        }
        g_gpu_prof.pass_n[khgr_r] = 0;
    }
    g_gpu_prof.ready = false;
    g_gpu_prof.failed = false;
    g_gpu_prof.open_slot = -1;
}
struct KhGpuScope {
    ID3D11DeviceContext* khgs_ctx;
    KhGpuZone            khgs_z;
    int                  khgs_slot;   // The bracket the begin stamp went to; -1 = none.
    KhGpuScope(ID3D11DeviceContext* khgs_c, KhGpuZone khgs_zone) : khgs_ctx(khgs_c), khgs_z(khgs_zone), khgs_slot(-1) {
        // Only inside an open cycle bracket, on a ready query set.
        if (!kh_stats_on() || !khgs_ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        khgs_slot = g_gpu_prof.open_slot;
        khgs_ctx->End(g_gpu_prof.ts[khgs_z][static_cast<uint32_t>(khgs_slot)][0]);
        g_gpu_prof.used[khgs_z][static_cast<uint32_t>(khgs_slot)] = true;
    }
    ~KhGpuScope() {
        // The end stamp goes to the begin's bracket; if that bracket closed
        // meanwhile the pair is dropped rather than straddling two.
        if (khgs_slot < 0) return;
        if (g_gpu_prof.open_slot != khgs_slot) { g_gpu_prof.used[khgs_z][static_cast<uint32_t>(khgs_slot)] = false; return; }
        khgs_ctx->End(g_gpu_prof.ts[khgs_z][static_cast<uint32_t>(khgs_slot)][1]);
    }
    KhGpuScope(const KhGpuScope&) = delete;
    KhGpuScope& operator=(const KhGpuScope&) = delete;
};
#define KH_GPU_SCOPE(ctx, z) KhGpuScope khgs_scope_##z((ctx), z)
// KH_GPU_PASS: one effect pass's GPU interval - KhGpuScope's rules (only inside an open cycle bracket, on a ready
// query set; an end that finds its bracket closed drops the pair), one of the cycle's KH_GPU_PASS_N pairs.
struct KhGpuPassScope {
    ID3D11DeviceContext* khgq_ctx;
    int                  khgq_slot;   // The bracket the begin stamp went to; -1 = none.
    uint32_t             khgq_i;
    KhGpuPassScope(ID3D11DeviceContext* khgq_c, uint32_t khgq_chain, int khgq_effect, bool khgq_user,
                   uint32_t khgq_fused) : khgq_ctx(khgq_c), khgq_slot(-1), khgq_i(0) {
        if (!kh_stats_on() || !khgq_ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        const uint32_t khgq_s = static_cast<uint32_t>(g_gpu_prof.open_slot);
        if (g_gpu_prof.pass_n[khgq_s] >= KH_GPU_PASS_N) return;
        khgq_i = g_gpu_prof.pass_n[khgq_s]++;
        g_gpu_prof.pass_key[khgq_s][khgq_i] = kh_gpu_pass_key(khgq_chain, khgq_effect, khgq_user, khgq_fused);
        g_gpu_prof.pass_used[khgq_s][khgq_i] = true;
        khgq_ctx->End(g_gpu_prof.pts[khgq_s][khgq_i][0]);
        khgq_slot = static_cast<int>(khgq_s);
    }
    ~KhGpuPassScope() {
        if (khgq_slot < 0) return;
        if (g_gpu_prof.open_slot != khgq_slot) {
            g_gpu_prof.pass_used[static_cast<uint32_t>(khgq_slot)][khgq_i] = false;
            return;
        }
        khgq_ctx->End(g_gpu_prof.pts[static_cast<uint32_t>(khgq_slot)][khgq_i][1]);
    }
    KhGpuPassScope(const KhGpuPassScope&) = delete;
    KhGpuPassScope& operator=(const KhGpuPassScope&) = delete;
};

// The cycle fold (main depth clear, render thread): publish the CPU accumulators, close this cycle's GPU bracket,
// harvest the slot KH_GPU_LAG cycles back, open the next bracket while the stats stay armed.
inline void kh_prof_fold(ID3D11DeviceContext* khpf_ctx) {
    const uint64_t khpf_f = kh_prof_freq();
    const bool khpf_on = kh_stats_on();
    // KH_PROF_DISARM: publish only what was measured armed and since the last reset.
    const uint32_t khpf_ep = g_prof_epoch.load(std::memory_order_acquire);
    const bool khpf_pub = khpf_on && khpf_ep == g_prof_epoch_seen;
    const bool khpf_reset = khpf_ep != g_prof_epoch_seen;   // A resetRenderStats since the last fold.
    g_prof_epoch_seen = khpf_ep;
    // KH_PROF_WINDOW: a whole armed cycle. The TSC zones, scanExtBytes and the funnel publish these only: a scope
    // still open at a disarm adds after it, and nothing drains them disarmed - the arming fold drops what they hold.
    const bool khpf_win = khpf_pub && g_prof_prev_on;
    g_prof_prev_on = khpf_on;
    // KH_PROF_DISARM for the window, the funnel and the passes: the first fold after a reset, armed or not, drops the
    // armed-only accumulators and zeroes the window and what is published, on this thread (a fold that raced a reset
    // made without the graphics lock may have added pre-reset values after the reset zeroed them). Once a reset.
    if (khpf_reset) {
        for (uint32_t khpf_z = 0; khpf_z < KHP_ZONE_N; ++khpf_z) {
            g_prof_tsc[khpf_z].store(0, std::memory_order_relaxed);
            g_prof_xb_acc[khpf_z].store(0, std::memory_order_relaxed);
        }
        for (uint32_t khpf_k = 0; khpf_k < KHF_N; ++khpf_k) g_fun_acc[khpf_k].store(0, std::memory_order_relaxed);
        kh_prof_window_zero();
    }
    // KH_PROF_SUB: the TSC's rate over the interval since the last armed fold. Disarmed folds skip the new work below
    // (nothing armed-only accumulates while disarmed: disarming is a reset, whose leftovers the block above dropped).
    double khpf_tpu = 0.0;
    if (khpf_on) {
        LARGE_INTEGER khpf_q = {};
        QueryPerformanceCounter(&khpf_q);
        const uint64_t khpf_qn = static_cast<uint64_t>(khpf_q.QuadPart);
        const uint64_t khpf_tn = __rdtsc();
        if (g_prof_cal_q != 0 && khpf_qn > g_prof_cal_q && khpf_tn > g_prof_cal_t) {
            g_prof_tsc_per_us.store(static_cast<double>(khpf_tn - g_prof_cal_t) * static_cast<double>(khpf_f) /
                                    (static_cast<double>(khpf_qn - g_prof_cal_q) * 1.0e6), std::memory_order_relaxed);
        }
        g_prof_cal_q = khpf_qn;
        g_prof_cal_t = khpf_tn;
        khpf_tpu = g_prof_tsc_per_us.load(std::memory_order_relaxed);
        if (khpf_win) g_prof_win_cycles.fetch_add(1u, std::memory_order_relaxed);   // KH_PROF_WINDOW.
    }
    for (uint32_t khpf_z = 0; khpf_z < KHP_ZONE_N; ++khpf_z) {
        const uint64_t khpf_t = g_prof_ticks[khpf_z].exchange(0, std::memory_order_relaxed);
        const uint64_t khpf_n = g_prof_count[khpf_z].exchange(0, std::memory_order_relaxed);
        uint64_t khpf_us = khpf_pub ? khpf_t * 1000000ull / khpf_f : 0ull;
        if (khpf_on) {   // KH_PROF_SUB / KH_PROF_UPLOAD / KH_PROF_WINDOW.
            const uint64_t khpf_s = g_prof_tsc[khpf_z].exchange(0, std::memory_order_relaxed);
            const uint64_t khpf_x = g_prof_xb_acc[khpf_z].exchange(0, std::memory_order_relaxed);
            if (khpf_win && khpf_s != 0 && khpf_tpu > 0.0) {
                khpf_us += static_cast<uint64_t>(static_cast<double>(khpf_s) / khpf_tpu);
            }
            g_prof_xb_pub[khpf_z].store(khpf_win ? khpf_x : 0ull, std::memory_order_relaxed);
            if (khpf_win) {
                g_prof_win_us[khpf_z].fetch_add(khpf_us, std::memory_order_relaxed);
                g_prof_win_n[khpf_z].fetch_add(khpf_n, std::memory_order_relaxed);
                g_prof_xb_win[khpf_z].fetch_add(khpf_x, std::memory_order_relaxed);
                if (khpf_us > g_prof_win_max[khpf_z].load(std::memory_order_relaxed)) {
                    g_prof_win_max[khpf_z].store(khpf_us, std::memory_order_relaxed);
                }
            }
        }
        g_prof_us_pub[khpf_z].store(khpf_pub ? khpf_us : 0ull, std::memory_order_relaxed);
        g_prof_n_pub[khpf_z].store(khpf_pub ? khpf_n : 0ull, std::memory_order_relaxed);
    }
    if (khpf_on) {   // KH_PROF_FUNNEL.
        for (uint32_t khpf_k = 0; khpf_k < KHF_N; ++khpf_k) {
            const uint64_t khpf_v = g_fun_acc[khpf_k].exchange(0, std::memory_order_relaxed);
            g_fun_pub[khpf_k].store(khpf_win ? khpf_v : 0ull, std::memory_order_relaxed);
            if (khpf_win) g_fun_win[khpf_k].fetch_add(khpf_v, std::memory_order_relaxed);
        }
    }
    if (!khpf_pub) {   // KH_PROF_DISARM: the GPU times too (the note above g_prof_epoch).
        for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
            g_gpu_us_pub[khpf_z].store(0, std::memory_order_relaxed);
        }
    }
    if (g_gpu_prof.open_slot >= 0 && g_gpu_prof.ready && khpf_ctx) {
        khpf_ctx->End(g_gpu_prof.disjoint[static_cast<uint32_t>(g_gpu_prof.open_slot)]);
        g_gpu_prof.open_slot = -1;
        g_gpu_prof.cycle++;
        if (!khpf_pub) {   // KH_PROF_DISARM: no slot's stamps outlive it to be read after the next arming.
            for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
                for (uint32_t khpf_r = 0; khpf_r < KH_GPU_RING; ++khpf_r) g_gpu_prof.used[khpf_z][khpf_r] = false;
            }
            for (uint32_t khpf_r = 0; khpf_r < KH_GPU_RING; ++khpf_r) g_gpu_prof.pass_n[khpf_r] = 0;   // KH_GPU_PASS.
        }
        // Harvest: the slot KH_GPU_LAG cycles back, never waiting.
        if (khpf_pub && g_gpu_prof.cycle >= KH_GPU_LAG) {
            const uint32_t khpf_s = static_cast<uint32_t>((g_gpu_prof.cycle - KH_GPU_LAG) % KH_GPU_RING);
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT khpf_dj = {};
            if (khpf_ctx->GetData(g_gpu_prof.disjoint[khpf_s], &khpf_dj, sizeof(khpf_dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                !khpf_dj.Disjoint && khpf_dj.Frequency > 0) {
                for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
                    if (!g_gpu_prof.used[khpf_z][khpf_s]) { g_gpu_us_pub[khpf_z].store(0, std::memory_order_relaxed); continue; }
                    UINT64 khpf_a = 0, khpf_b = 0;
                    if (khpf_ctx->GetData(g_gpu_prof.ts[khpf_z][khpf_s][0], &khpf_a, sizeof(khpf_a), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        khpf_ctx->GetData(g_gpu_prof.ts[khpf_z][khpf_s][1], &khpf_b, sizeof(khpf_b), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        khpf_b >= khpf_a) {
                        const uint64_t khpf_g = (khpf_b - khpf_a) * 1000000ull / khpf_dj.Frequency;
                        g_gpu_us_pub[khpf_z].store(khpf_g, std::memory_order_relaxed);
                        g_gpu_win_us[khpf_z].fetch_add(khpf_g, std::memory_order_relaxed);   // KH_PROF_WINDOW.
                        g_gpu_win_n[khpf_z].fetch_add(1u, std::memory_order_relaxed);
                        if (khpf_g > g_gpu_win_max[khpf_z].load(std::memory_order_relaxed)) {
                            g_gpu_win_max[khpf_z].store(khpf_g, std::memory_order_relaxed);
                        }
                    }
                    g_gpu_prof.used[khpf_z][khpf_s] = false;
                }
                // KH_GPU_PASS: the slot's passes, in draw order; the published list is this cycle's.
                uint32_t khpf_pk = 0;
                for (uint32_t khpf_i = 0; khpf_i < g_gpu_prof.pass_n[khpf_s]; ++khpf_i) {
                    if (!g_gpu_prof.pass_used[khpf_s][khpf_i]) continue;
                    g_gpu_prof.pass_used[khpf_s][khpf_i] = false;
                    UINT64 khpf_a = 0, khpf_b = 0;
                    ID3D11Query* const* const khpf_q = g_gpu_prof.pts[khpf_s][khpf_i];
                    const UINT khpf_fl = D3D11_ASYNC_GETDATA_DONOTFLUSH;
                    if (khpf_ctx->GetData(khpf_q[0], &khpf_a, sizeof(khpf_a), khpf_fl) != S_OK ||
                        khpf_ctx->GetData(khpf_q[1], &khpf_b, sizeof(khpf_b), khpf_fl) != S_OK ||
                        khpf_b < khpf_a) continue;
                    const uint64_t khpf_g = (khpf_b - khpf_a) * 1000000ull / khpf_dj.Frequency;
                    const uint32_t khpf_key = g_gpu_prof.pass_key[khpf_s][khpf_i];
                    g_gpu_pass_pub[khpf_pk++].store((static_cast<uint64_t>(khpf_key) << 32) | (khpf_g & 0xFFFFFFFFull),
                                                    std::memory_order_relaxed);
                    const uint32_t khpf_w = khpf_key & (KH_GPU_PASS_W - 1u);
                    g_gpu_passw_us[khpf_w].fetch_add(khpf_g, std::memory_order_relaxed);
                    g_gpu_passw_n[khpf_w].fetch_add(1u, std::memory_order_relaxed);
                    if (khpf_g > g_gpu_passw_max[khpf_w].load(std::memory_order_relaxed)) {
                        g_gpu_passw_max[khpf_w].store(khpf_g, std::memory_order_relaxed);
                    }
                }
                g_gpu_pass_pub_n.store(khpf_pk, std::memory_order_relaxed);
            }
        }
    }
    if (khpf_on && khpf_ctx && kh_gpu_prof_ensure(khpf_ctx)) {
        const uint32_t khpf_s = kh_gpu_slot();
        for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) g_gpu_prof.used[khpf_z][khpf_s] = false;
        for (uint32_t khpf_i = 0; khpf_i < KH_GPU_PASS_N; ++khpf_i) g_gpu_prof.pass_used[khpf_s][khpf_i] = false;
        g_gpu_prof.pass_n[khpf_s] = 0;   // KH_GPU_PASS.
        khpf_ctx->Begin(g_gpu_prof.disjoint[khpf_s]);
        g_gpu_prof.open_slot = static_cast<int>(khpf_s);
    }
}
inline void kh_prof_reset_pub() {
    for (uint32_t khpr_z = 0; khpr_z < KHP_ZONE_N; ++khpr_z) { g_prof_us_pub[khpr_z].store(0, std::memory_order_relaxed); g_prof_n_pub[khpr_z].store(0, std::memory_order_relaxed); }
    for (uint32_t khpr_z = 0; khpr_z < KHG_ZONE_N; ++khpr_z) g_gpu_us_pub[khpr_z].store(0, std::memory_order_relaxed);
    kh_prof_window_zero();   // KH_PROF_WINDOW (the cycle in flight's accumulators are the fold's to drop).
}
}