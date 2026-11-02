#pragma once

// The shared head (rendering_integration.hpp): RenderStats (getRenderStats' counters) and kh_stat / kh_stat_add. Never
// include it on its own.
namespace RenderIntegration {

struct RenderStats {
    uint64_t flushes = 0;   // Flush attempts with work queued.
    uint64_t lock_retries = 0;   // Individual failed lock acquisitions.
    uint64_t lock_failed_frames = 0;   // Frames dropped after all retries failed.
    uint64_t ui_flushes = 0;   // UI-phase flush attempts with work queued.
    uint64_t composite_injections = 0;   // pre-translucent injection events (once per scene frame).
    uint64_t pip_injections = 0;   // KH_PIP: injections into PIP passes.
    uint64_t pip_meshes = 0;       // KH_PIP: draws they made.
    uint64_t pip_fx = 0;           // KH_PIP_FX: localized fullscreen passes drawn into PIP passes.
    // KH_CLOTH / KH_CHAIN, published by kh_cloth_upload: instances simulating right now, the frames one was still busy
    // from the previous one (since the stats were armed - a steady rise means the workers cannot keep up) and the
    // collider meshes gathered last frame.
    uint64_t cloth_instances = 0;
    uint64_t cloth_skipped = 0;
    uint64_t physics_colliders = 0;
    // KH_SKEL, published by the upload like the cloth's: skeletal meshes with a vertex buffer, the bones of their
    // skeletons, the bones among those whose own name matched a memory point (0 with skinBones above 0: no name
    // matched), and the memory-point proxies following the parents.
    uint64_t skin_meshes = 0;
    // KH_SKIN_POOL: of those, the ones whose pose lives in a skin pool chunk (published by the upload); and, since the
    // stats were armed, the skeletal instances the colour buckets drew from a chunk (one per instance per draw - a
    // crossfading one twice).
    uint64_t skin_pooled = 0;
    uint64_t skin_inst = 0;
    uint64_t skin_bones = 0;
    uint64_t skin_bones_driven = 0;
    uint64_t skin_proxies = 0;
    // Changed poses that waited a frame because that mesh's last job was still running on the pool (the skin's
    // clothSkipped). Since the stats were armed.
    uint64_t skin_skipped = 0;
    // Since the stats were armed, full CPU skins by the skin pool's workers (kh_skin_run with vertices: a binding's
    // first result, an unhide, a device reset, a span no render-thread step covers) - not in skinCpu, the steps' count.
    uint64_t skin_pool_full = 0;
    // KH_SKIN_GPU_MOVE, since the stats were armed: bindings moved from a CPU-skinned buffer to GPU skinning
    // (kh_skin_gpu_move), and the device's refusals of a GPU-skinning buffer (the pool slice and one of the binding's
    // own, at a first install or a move) or of a mesh's rest stream - each leaves a binding on the CPU path until the
    // retry - or of memory for the stream-output objects (KH_SKIN_SO_RETRY: every binding, likewise).
    uint64_t skin_gpu_moves = 0;
    uint64_t skin_gpu_refused = 0;
    uint64_t chain_instances = 0;
    uint64_t chain_skipped = 0;
    // KH_SIM_LOD, published with the cloth's: the solver work (substeps x iterations over the instances stepped last
    // frame) at the counts the LOD chose and at the objects' own dials; their ratio is the share run.
    uint64_t simlod_work = 0;
    uint64_t simlod_full = 0;
    uint64_t composite_meshes = 0;   // Meshes drawn through the composited path.
    // KH_OCC_STATS, since the stats were armed: the occluder boxes the injection's occlusion cull drew into its grid
    // (kh_vis_occ_build; a box that covered no cell is not counted), and the objects inside the view it then hid (what
    // the frustum rejects is not counted).
    uint64_t vis_occluders = 0;
    uint64_t vis_occluded = 0;
    uint64_t textured_draws = 0;   // per-submesh textured draws issued (KH_TEXTURED).
    uint64_t meshes_released = 0;   // KH_MESH_FREE: registry meshes released by the idle GC.
    uint64_t textures_released = 0;   // KH_TEX_GC: cached texture views released idle.
    // FBX files parsed + registered (misses of both the id map and the binary mesh cache).
    uint64_t fbx_imports = 0;
    // KH_DL_DIAG - the dynamic-light pool's upkeep. Harvests taken by the injection and by the render-thread flush's
    // fallback (a cycle with no accepted injection); harvests the sampler knew were incomplete (the TTL sweep ran
    // instead of presence); author windows refused at the per-span cap; lit mesh fills that found the pool stale (older
    // than KH_DL_POOL_STALE_MS: those draws got no dynamic light); light-ring appends that failed; and the most lights
    // one draw received.
    uint64_t dl_harvests_inject = 0;
    uint64_t dl_harvests_flush = 0;
    uint64_t dl_harvests_incomplete = 0;
    uint64_t dl_windows_refused = 0;
    uint64_t dl_stale_fills = 0;
    uint64_t dl_ring_fails = 0;
    uint64_t dl_mesh_lights_max = 0;
    // KH_DL_DIAG (CN-4) - what can change the dynamic lights for one frame. Pool lights a complete harvest's presence
    // sweep dropped with no sighting after the harvest that created them (a light drawn in one span only, a copy placed
    // at a wrong origin, the placements of a light moving past the match cube that no later upload re-sighted).
    // Harvests whose brightest candidate - the main-pass reference's evidence (ref_hist, KH_DL_REF_MEDIAN: a placed
    // window or an upload re-sighted through its buffer's origin) - was brighter / dimmer than the last such
    // harvest's (kh_dl_ref_brighter), and whose previous such was a move this one took back (a one-harvest excursion
    // - the median keeps it from the reference). New pool entries repeating an older entry's record a nonzero whole
    // number of land-grid cells away at its height (kh_dl_copy_of: another origin's placement of that light, or a lamp
    // of the same model grid-aligned with it). And dynamicLightFog lights, per pass and frame, whose light-map slot is
    // not published (unshadowed there).
    uint64_t dl_blinks = 0;
    uint64_t dl_ref_up = 0;
    uint64_t dl_ref_down = 0;
    uint64_t dl_ref_spike_up = 0;
    uint64_t dl_ref_spike_down = 0;
    uint64_t dl_copies = 0;
    uint64_t dlf_unshadowed = 0;
    // KH_FX_PAIR_HOLD, since the stats were armed: depth-pair verdicts that held the adopted pair against the frame's
    // (kh_fx_depth_pair_guard) - one per verdict that held: a flush's (the scene chain's, a park's) or the UI lane's
    // own. A count stepping on the frame of a one-frame depth glitch (fog or light through a near surface) names the
    // foreign latch the guard was built for.
    uint64_t fx_pair_holds = 0;
    // KH_DL_PRES_MEMO - the light lists presence matched against the pool (uploads past the count check), and how
    // many of those replayed a remembered match instead of running it.
    uint64_t dl_pres_calls = 0;
    uint64_t dl_pres_memo_hits = 0;
};
static RenderStats g_stats;
// KH_STATS_ARMED: collection is off until the first getRenderStats; a resetRenderStats zeroes the counters and
// disarms it again. Every counter write is gated on kh_stats_on (kh_stat / kh_stat_add, or a test at the site), so a
// disarmed session pays one relaxed load per site and accumulates nothing. g_stats_armed / kh_stats_on: defined with
// KH_PROF (rendering/profiling.hpp).
inline void kh_stat(uint64_t& khst_c) { if (kh_stats_on()) ++khst_c; }
inline void kh_stat_add(uint64_t& khst_c, uint64_t khst_n) { if (kh_stats_on()) khst_c += khst_n; }
}