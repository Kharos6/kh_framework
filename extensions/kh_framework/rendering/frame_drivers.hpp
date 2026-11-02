#pragma once

// The injection half (rendering_integration_injection.hpp): the game-thread drivers: the game thread id, video options
// and lighting staging, the park, flush_frame, the backbuffer capture and the UI chain. Never include it on its own.
namespace RenderIntegration {
inline void ensure_present_hook();   // Defined with the hook (rendering/present_hook.hpp).
inline int kh_overlay_begin();       // KH_OVERLAY_BRACKET, defined with the Present subscriber.
inline void kh_overlay_end(int khoe_tok);

inline void kh_ui_driver_rehoist();   // Defined with the driver lifecycle (the auto-hoist site,
                                      // rendering/ui_flush.hpp).
// KH_RESET_THREAD: the game thread's id, stamped at the heads of flush_frame (Draw3D) and flush_ui_frame (their paths
// to ensure_resources install the reset hook) and in ensure_draw_eh (every object's add), so it is set before any
// object can feed a path to the hook - the render thread's, or the game thread's inside an engine call (kh_fx_late_gt's
// chain-only flush at a context resolve, kh_present_cb's flush_ui_locked at Present), each behind a demand flag that
// flush_frame or an object's add raises after its stamp (g_kh_track_wanted for the render-thread flush that owes the
// late chain, g_ui_mask_wanted for the Present flush). on_engine_reset takes the graphics lock only on the game
// thread, like every other ScopedGraphicsLock site. Process-scoped (the game thread never changes): no session reset.
static std::atomic<uint32_t> g_game_tid{ 0 };

// Runs from flush_frame (stage_world_lighting) before the graphics lock is taken: it makes SQF/engine calls and must
// not extend the render-thread park. Its readings go straight to atomics either thread reads (g_obj_vis, g_sun_range,
// g_shadows_off); the fog stage_world_lighting stages beside it is published inside flush_locked
// (publish_world_lighting).

// KH_VIDEO_OPT_CADENCE: both settings come from one engine call every sample - getObjectViewDistance, the video
// options' [object view distance, shadow view distance] - and the interval caps it at four a second. Do not read
// getVideoOptions (it builds a map of every video setting, ~1.2 ms a call) or getShadowDistance (it does not follow a
// change made in the video options at runtime, measured).
// KH_SHADOW_VIS: the shadow view distance, read on every sample like the object view distance - no display gate, no
// pause rule: a sample is taken only when a Draw3D's flush_frame reaches the staging (some mesh or pass exists, the
// bridge is up), and each one reads the current setting. Envelope: a changed shadow or object view distance reaches
// g_shadows_off / g_sun_range / g_obj_vis up to KH_VIDEO_OPT_INTERVAL_MS after the first sample that follows the
// change.
static constexpr uint64_t KH_VIDEO_OPT_INTERVAL_MS = 250;
static uint64_t g_video_opt_ms = 0;   // Game thread: the last sample.
inline void stage_video_options() {
    try {
        const uint64_t khsr_now_ms = steady_now_ms();
        if (g_video_opt_ms != 0 && khsr_now_ms - g_video_opt_ms < KH_VIDEO_OPT_INTERVAL_MS) return;
        g_video_opt_ms = khsr_now_ms;
        // KH_SHADOW_VIS: both view distances, one call.
        const auto khsr_rd = sqf::get_object_view_distance();
        const float khsr_sh = khsr_rd.shadow_distance;
        // KH_SHADOW_OFF: whoever owns the range below; KH_SHADOW_OFF_M (5 m) or below reads as off, a NaN leaves the
        // switch as it stands.
        if (khsr_sh == khsr_sh) g_shadows_off.store(khsr_sh <= KH_SHADOW_OFF_M, std::memory_order_relaxed);
        // The shadow range is not written while the engine's band table owns it (src == 2: the far border of the
        // engine's cascades, published by the render thread): the two would overwrite each other. A NaN or a
        // non-positive reading leaves the previous value standing.
        if (g_sun_range_src.load(std::memory_order_relaxed) != 2 && khsr_sh == khsr_sh && khsr_sh > 0.0f) {
            g_sun_range.store(khsr_sh, std::memory_order_relaxed);
            g_sun_range_src.store(1, std::memory_order_relaxed);
        }
        const float khsr_obj = khsr_rd.object_distance;
        if (khsr_obj == khsr_obj && khsr_obj > 0.0f) {
            g_obj_vis.store(khsr_obj, std::memory_order_relaxed);
        }
    } catch (...) {}
}
inline void stage_world_lighting() {
    stage_video_options();
    // KH_DL_ORIGIN_GRID: the terrain's land-grid cell, once per session (an engine call, like the reads above).
    if (!(g_dl_cell.load(std::memory_order_relaxed) > 0.0f)) {
        try {
            const auto khdc_t = sqf::get_terrain_info();
            const float khdc_w = khdc_t.land_grid_width;
            if (khdc_w == khdc_w && khdc_w > 0.0f && khdc_w < 1.0e5f) {
                g_dl_cell.store(khdc_w, std::memory_order_relaxed);
            }
        } catch (...) {}
    }

    // Atmospheric occlusion applies to every world-space mesh; stage it whenever the flush runs.

    try {
        const auto fp = sqf::fog_params();   // The engine call outside the mutex.
        std::lock_guard<std::mutex> khsw_l(g_fog_stage_mu);   // KH_NO_PARK.
        g_fog_staged[0] = fp.value;
        g_fog_staged[1] = fp.decay;
        g_fog_staged[2] = fp.base;
        g_fog_staged_valid = true;
    } catch (...) {
        { std::lock_guard<std::mutex> khsw_l(g_fog_stage_mu); g_fog_staged_valid = false; }
        // Fog unavailable this frame: the fog term stands down.
    }
}

// The park: bootstrap, watchdog and mission end.
inline void kh_park_run(ID3D11Device* khpr_dev, ID3D11DeviceContext* khpr_ctx) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int64_t khff_wait_t0 = kh_prof_now();   // KH_PARK_WAIT.
        RVExtBridge::ScopedGraphicsLock lock;
        kh_prof_add(KHP_FLUSH_LOCK_WAIT, khff_wait_t0);

        if (!lock.acquired()) {
            kh_stat(g_stats.lock_retries);
            continue;
        }
        // Deferred mission destroy (armed only by a lock-exhausted mission end): consumed here because this lock is the
        // park invariant the teardown could not acquire.
        if (g_mission_destroy_pending.exchange(false, std::memory_order_relaxed)) {
            // The same teardown rendering_integration_reset runs under its own park (kh_session_teardown), so a mission
            // whose edge could not take the lock tears down no differently.
            g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
            // Not clean: this park is the NEXT mission's (flush_frame parks only with a non-empty draw list), so its
            // objects' meshes, materials, scene slots, handles and clock stay; the old mission's were dropped by its
            // reset_retained_state, and the collectors take what they left.
            // KH_TEARDOWN_KEEP_SETTINGS: the script settings are that mission's too (its handlers may have set
            // them before this park; rendering_integration_reset put the old mission's back to defaults), so the
            // teardown's session reset does not take them.
            const bool khpr_dls = g_dls_allowed.load(std::memory_order_relaxed);
            const float khpr_sg = g_khsg_scale.load(std::memory_order_relaxed);
            const uint32_t khpr_aos = g_ao_strength_bits.load(std::memory_order_relaxed);
            const uint32_t khpr_aod = g_ao_dist_bits.load(std::memory_order_relaxed);
            const bool khpr_st = g_stats_armed.load(std::memory_order_relaxed);
            kh_session_teardown(false);
            g_dls_allowed.store(khpr_dls, std::memory_order_relaxed);
            g_khsg_scale.store(khpr_sg, std::memory_order_relaxed);
            g_ao_strength_bits.store(khpr_aos, std::memory_order_relaxed);
            g_ao_dist_bits.store(khpr_aod, std::memory_order_relaxed);
            g_stats_armed.store(khpr_st, std::memory_order_relaxed);
            return;
        }
        // KH_PARK_STEP: a park's flush draws late and needs this cycle's poses; the render-thread step inside
        // flush_locked is khfl_rt only. Taken with the render thread stalled: a lane the render thread already stepped
        // this cycle is skipped (the cycle gate); others take the cycle's sample (kh_gts_for_cycle), or hold.
        kh_attach_step();
        // Keeps our draws out of the hooks; released on unwind too (a flag left
        // true would make every hook treat engine draws as ours).
        // KH_PARK_INJ: on the thread the hooks take for the render thread (the game thread at FSAA 1x, which clears the
        // main depth) g_ro.in_injection is raised for the whole flush, as kh_fx_late_gt and the Present subscriber
        // raise it - the hooks' render-thread branch reads that flag, not KhFlushActiveScope's. KhInjectionFlagScope
        // restores it.
        {
            KhFlushActiveScope khff_active;
            KhInjectionFlagScope khff_inj;
            if (reorder_on_render_thread()) g_ro.in_injection = true;
            g_flush_wrong_pass = false;
            flush_locked(khpr_dev, khpr_ctx, false);
        }
        // A wrong pass here differs from one at Draw3D: at this handover the engine may be in the view model's depth
        // partition, so flush_locked may refuse and this loop sleeps up to three times holding the lock inside the
        // engine's resolve call.
        if (!g_flush_wrong_pass) {  return; }
        // The park landed on a non-main depth (a PIP pass, a cascade) and drew nothing. The lock is released after this
        // Sleep(0) and the next attempt parks again; the frame is lost only when every attempt lands wrong.
        Sleep(0);
    }

    kh_stat(g_stats.lock_failed_frames);   // No park drew: every attempt failed the lock or landed wrong.
}

// KH_SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static float g_ui_rehoist_auto_s = -1.0e9f;
inline void flush_frame() {
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // KH_RESET_THREAD.
    KH_PROF_SCOPE(KHP_FLUSH_FRAME);   // KH_PROF.
    const int64_t khff_pre_t0 = kh_prof_now();   // KH_PROF: flushPre.
    // KH_ATTACH_EDEN: this frame's flag, ahead of every read below (the crew table, the sampler, the chains); the
    // render thread's reads take the latest flag stored (this Draw3D's, or before it lands the previous one's).
    kh_attach_eden_note();
    // KH_ATTACH: no step here. The late draw is stepped by the render thread's flush or by kh_park_run; a game-thread
    // STEP would write the poses at a different instant from the render thread's, which is the authority. A read is
    // another matter: KH_ATTACH_GT_SNAP's sample is taken here, below, because Draw3D is the instant the engine's own
    // drawn state is coherent. The render thread's step consumes it and stays the authority on poses.
    // KH_ATTACH_DEAD: drain what the step (either thread) found dead. Here because this is the game thread and the
    // erase releases game_values; before the work test below, so a frame whose only remaining mesh was following a
    // deleted object finds nothing left to draw.
    kh_attach_reap();
    // KH_ATTACH_BONE: after the reap, because the reap is what retires the proxies of the meshes it just erased, and
    // before the park, because this one makes SQF calls.
    kh_attach_proxy_reap();
    // KH_CREW_LAG: the crew table and this Draw3D's corrections - holding nothing (SQF calls), after the reaps
    // and before the sampler and the chain's end (kh_chain_prepare), which apply them.
    kh_crew_sync();
    // KH_ATTACH_GT_SNAP: this frame's objects, as the engine will draw them. After the reaps (a reaped binding
    // takes no sample) and before the park, on the thread the SQF commands write the table from.
    const bool khff_gts = kh_attach_gt_snap();
    kh_prof_add(KHP_FLUSH_PRE, khff_pre_t0);
    const int64_t khff_cen_t0 = kh_prof_now();   // KH_PROF: flushCensus.
    bool has_work;
    bool khff_cloth = false;   // KH_CLOTH: some object asks to simulate (kh_cloth_sync's inert gate).

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        has_work = !g_draw_list.empty();

        // An empty list clears it for free.
        bool khum_wanted = false;
        bool khum_lit = false;   // A visible lit mesh, or any shadow-active one (casterOnly counts): g_ls.wanted.
        bool khum_mesh = false;   // Any visible mesh at all (volume-copy demand).
        bool khum_fogsc = false;   // KH_FOG_PROBE: a visible fog-scatter pass.
        bool khum_dlf = false;   // KH_DLF_DEMAND: a visible scene-chain dynamicLightFog pass.
        bool khum_front = false;   // KH_INFRONT: any visible view-model mesh (the slice hook's demand).
        bool khum_fpwatch = false;   // KH_SHOW_INFRONT: a visible world mesh with showInFront false.
        g_cloth_proxy_want.clear();   // KH_CLOTH_PROXY.
        const float khum_now_s = effect_time_seconds();   // KH_NO_PARK_EXPIRY: the expiry test's clock.
        // KH_NO_PARK_EXPIRY - expired objects are erased here (no park comes for them), on the game thread under
        // g_draw_list_mutex, the same sequence remove_render_object and kh_attach_reap run; render-thread walks skip an
        // expired object meanwhile.
        static std::vector<std::string> khum_expired;
        khum_expired.clear();
        kh_ref_note_begin(g_ref_note_cen);   // KH_REF_NOTE_ONCE.
        g_mat_note_cen.clear();   // KH_MAT_NOTE_ONCE.

        for (const auto& khum_kv : g_draw_list) {
            {   // KH_NO_PARK_EXPIRY.
                bool khum_x = false;
                lifetime_envelope(khum_kv.second, khum_now_s, khum_x);
                if (khum_x) { khum_expired.push_back(khum_kv.first); continue; }
            }
            if (!khum_kv.second.fullscreen) kh_ref_note_add(g_ref_note_cen, khum_kv.second.mesh);   // KH_MESH_FREE, KH_REF_NOTE_ONCE.
            if (khum_kv.second.materials) g_mat_note_cen.push_back(khum_kv.second.materials);   // KH_MAT_POOL,
                                                                                                // KH_MAT_NOTE_ONCE.
            // Shadow-live demand: a visible lit object or any shadow-active object (casterOnly too).
            // KH_MESH_SSGI: an "ssgi" effect mesh draws nothing (the flush skips it), so it demands nothing here.
            const bool khum_nodraw = !khum_kv.second.fullscreen &&
                                     (khum_kv.second.effect == static_cast<int>(EffectId::Ssgi) ||
                                      khum_kv.second.effect == static_cast<int>(EffectId::DynLightFog));   // KH_DLF.
            if ((kh_obj_vis(khum_kv.second) && khum_kv.second.lit && !khum_nodraw &&   // KH_SHOW_INFRONT.
                 (!kh_in_front(khum_kv.second) || g_vm_slice_live.load(std::memory_order_relaxed))) ||   // KH_INFRONT.
                kh_shadow_active(khum_kv.second)) khum_lit = true;
            if (kh_cloth_obj_sim(khum_kv.second) || kh_chain_obj_sim(khum_kv.second)) {   // KH_CHAIN too.
                khff_cloth = true;   // Visible or not, as the sync counts it.
                // KH_CLOTH_PROXY: what the proxy step needs, taken under this lock.
                const float* khum_p = khum_kv.second.pos;
                g_cloth_proxy_want.push_back({ khum_kv.first, khum_kv.second.seq, { khum_p[0], khum_p[1], khum_p[2] } });
            }
            if (!kh_obj_vis(khum_kv.second) || khum_nodraw) continue;   // KH_SHOW_INFRONT: kh_obj_vis.

            if (khum_kv.second.fullscreen) {
                if (khum_kv.second.affect_ui) khum_wanted = true;
                if (khum_kv.second.effect == static_cast<int>(EffectId::Fogscatter)) khum_fogsc = true;   // KH_FOG_PROBE.
                // KH_DLF: a scene-chain dynamicLightFog pass reads the engine fog (the probe) and the dynamic-light
                // pool - the light capture's demand alone (KH_DLF_DEMAND), not a lit mesh's, which would also arm
                // the engine shadow capture the pass never reads (the FSAA 1x stand-down below clears it too).
                if (khum_kv.second.effect == static_cast<int>(EffectId::DynLightFog) && !khum_kv.second.ui_only) {
                    khum_fogsc = true;
                    khum_dlf = true;
                }
            } else {
                khum_mesh = true;   // Every visible mesh reads the volume copy and the snapshots.
                // KH_INFRONT: the slice hook's demand too. KH_SHOW_INFRONT: an inFront mesh that is never shown
                // asks for nothing (kh_obj_vis, above - as visible false); a world one with showInFront false asks
                // for the verdict.
                if (kh_in_front(khum_kv.second)) {
                    if (khum_kv.second.show_in_front) khum_front = true;
                } else if (!khum_kv.second.show_in_front) {
                    khum_fpwatch = true;
                }
            }
            // No early-out: the two reference notes above must reach every object (an unnoted mesh or material set is
            // released by the GCs after their idle window while the object still carries it).
        }

        kh_ref_note_end(g_ref_note_cen);   // KH_REF_NOTE_ONCE: one stamp per id, one lock.
        kh_material_note_end(g_mat_note_cen);   // KH_MAT_NOTE_ONCE: likewise, one stamp per set.
        g_ref_note_cen_ms.store(GetTickCount64(), std::memory_order_release);   // KH_GC_NOTE_SKIP.
        for (const std::string& khum_h : khum_expired) {   // As remove_render_object erases.
            auto khum_it = g_draw_list.find(khum_h);
            if (khum_it == g_draw_list.end()) continue;
            kh_scene_stage_remove(khum_it->second);
            kh_attach_drop(khum_h);   // KH_ATTACH.
            g_draw_list.erase(khum_it);
        }
        if (!khum_expired.empty()) has_work = !g_draw_list.empty();
        g_ui_mask_wanted.store(khum_wanted, std::memory_order_relaxed);
        g_svs_mesh_wanted.store(khum_mesh, std::memory_order_relaxed);
        g_fogsc_wanted.store(khum_fogsc, std::memory_order_relaxed);   // KH_FOG_PROBE.
        g_infront_wanted.store(khum_front, std::memory_order_relaxed);   // KH_INFRONT.
        g_vm_watch_wanted.store(khum_front || khum_fpwatch, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
        // FSAA 1x: the lit demand stands the whole shadow-live ecosystem down through the existing master-demand gates.
        if (khum_lit && kh_fsaa_world_standdown()) khum_lit = false;
        g_ls.wanted.store(khum_lit, std::memory_order_relaxed);
        if (khum_dlf && kh_fsaa_world_standdown()) khum_dlf = false;   // KH_DLF_DEMAND: as the lit demand.
        g_dlf_wanted.store(khum_dlf, std::memory_order_relaxed);
    }

    kh_prof_add(KHP_FLUSH_CENSUS, khff_cen_t0);
    const int64_t khff_prep_t0 = kh_prof_now();   // KH_PROF: flushPrep (closed at the park decision).
    // KH_CLOTH_PROXY: holding nothing, and ahead of the empty-list return
    // below, so the last cloth mesh's removal still deletes its proxy.
    if (!g_cloth_proxy_want.empty() || !g_cloth_proxy.empty()) kh_cloth_proxy_step();
    kh_chain_prepare(khff_gts);   // KH_CHAIN: likewise holding nothing and ahead of it.

    g_kh_track_wanted.store(has_work, std::memory_order_relaxed);

    if (!has_work) return;


    if (g_ui_mask_wanted.load(std::memory_order_relaxed)) {
        const float khrh_now_s = effect_time_seconds();

        if (khrh_now_s - g_ui_rehoist_auto_s >= 1.0f) {
            g_ui_rehoist_auto_s = khrh_now_s;
            kh_ui_driver_rehoist();
        }
    }
    if (!RVExtBridge::is_initialized()) return;
    // cold_t0: first flush whose draw list holds a shadow-capable mesh (any
    // mesh, so a translucent-first session opens the window).
    if (g_mask.cold_t0 < 0.0) {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const RenderObject& khc_o = kv.second;

            if (khc_o.visible && !khc_o.fullscreen && khc_o.effect == 0 &&
                khc_o.mode != DepthMode::Off) {
                g_mask.cold_t0 = static_cast<double>(effect_time_seconds());
                break;
            }
        }
    }
    kh_sun_size_latch();   // KH_SUN_LADDER: the hook-less path latches here.
    stage_world_lighting();   // Game thread: the video options and the fog (pre-lock).
    kh_thm_autobuild_step();   // Game thread: zero-setup terrain acquisition (pre-lock).
    // KH_DESTROY_FIRST: a mission edge whose park failed left the old mission's teardown pending and the render
    // thread disarmed (no context). It is taken here, first, before ensure_reorder_hook re-arms that context:
    // armed first, at FSAA above 1x the render thread could flush once with the old mission's learned targets,
    // locators and shadow state under this mission's objects. The park's teardown releases this frame's work
    // anyway (kh_park_run returns right after it); a park that fails again leaves the context unarmed.
    if (g_mission_destroy_pending.load(std::memory_order_relaxed)) {
        ID3D11Device* khdf_dev = RVExtBridge::get_d3d_device();
        ID3D11DeviceContext* khdf_ctx = RVExtBridge::get_d3d_device_context();
        if (khdf_dev && khdf_ctx) kh_park_run(khdf_dev, khdf_ctx);
        return;
    }
    kh_psv_hook_ensure();   // KH_PSV: the CreatePixelShader hook, if no earlier point could install it.
    ensure_reorder_hook();   // Cheap early-out once installed; refreshes the tracked context.
    // KH_OVERLAY_BRACKET: once the context hooks are live, another module's present-time draws are excluded from them
    // - with or without a UI pass of ours (ensure_present_hook, below, runs only with one). Two stores; idempotent.
    if (g_reorder_hook_active.load(std::memory_order_acquire)) {
        kh_present_overlay_register(&kh_overlay_begin, &kh_overlay_end);
    }
    if (g_ui_mask_wanted.load(std::memory_order_relaxed)) ensure_present_hook();   // KH_PRESENT_UI: once, when a UI pass first exists.
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return;

    {   // KH_FLUSH_MISS: did the injection land since the previous flush_frame (a park's verdict).
        const uint64_t khff_serial = g_composite_inject_serial.load(std::memory_order_relaxed);
        g_flush_landed_this_frame = khff_serial != g_flush_miss_seen_serial;
        g_flush_miss_seen_serial = khff_serial;
    }
    kh_prof_add(KHP_FLUSH_PREP, khff_prep_t0);
    g_flush_serial.fetch_add(1, std::memory_order_relaxed);   // KH_FLUSH_SERIAL: the graves' clock.
    kh_stat(g_stats.flushes);

    // KH_CLOTH: queue this frame's simulation before the park. Off the park deliberately - the workers must be running
    // WHILE the render thread draws, not inside the window that stops it. The wind is sampled immediately ahead of it
    // because this is the game thread and sqf::wind is an engine call, which is the same reason the fog sample sits
    // where it does.
    kh_cloth_wind_sample();
    // KH_CLOTH_GROUND: the terrain's split, measured once per matrix and mission - engine calls, so here, holding
    // nothing, before the sync copies each cloth's ground.
    if (khff_cloth) kh_cloth_ground_prepare();
    // KH_SKEL: the proxies are read and the poses skinned here for the same reason - on the game thread, off the park,
    // the upload inside it - and AHEAD of the cloth, which takes a simulating binding's pose from it.
    kh_skin_sync();
    kh_cloth_sync(kh_cloth_frame_dt(), khff_cloth);
    {   // KH_INJ_FORK: the pool on demand for the render thread's per-object forks
        // (kh_fork_each), which never start it themselves (that takes g_cloth_mu).
        // Idempotent once started; stopped with the session.
        size_t khff_nobj = 0;
        { std::lock_guard<std::mutex> khff_l(g_draw_list_mutex); khff_nobj = g_draw_list.size(); }
        if (khff_nobj >= KH_FORK_EACH_MIN && kh_cloth_fork_workers() < 2u) kh_cloth_workers_start();
    }

    // KH_NO_PARK: the contract below describes only the park (bootstrap, watchdog, mission end). Every other frame the
    // flush runs on the render thread inside its scene resolve (thread identity keeps the injection out), the scene
    // chain may run inside a later resolve (KH_FX_LATE), and shared game-thread data is under g_draw_list_mutex,
    // g_skin_mu, g_cloth_mu, g_mesh_ref_mu or atomics. THREADING CONTRACT - DO NOT CHANGE. ScopedGraphicsLock is the
    // engine's RVExtensionGLock: while held the engine does not render, so the render thread cannot be inside any
    // hooked context method. That makes the flush's context use, ensure_mesh_vbs's vector growth, the cb_ring cursors
    // and g_res.release() safe against the injection. The injection never takes this lock (deadlock).
    // get_projection_view_transform is read from the render thread without the lock by design (a read; one frame stale
    // on hard cuts). The ordinal stays here: the UI flush's dark-driver test (kh_ui_frame_ordinal, on the lock path)
    // expects one advance per frame on this thread and reads a jump of two as a dark driver.
    g_flush_frame++;
    g_flush_frame_pub.store(g_flush_frame, std::memory_order_relaxed);
    // KH_NO_PARK BOOTSTRAP: g_main_depth_identity is adopted by flush_locked
    // (and by the scene resolve once the render thread is named:
    // KH_DEPTH_READOPT), and the render-thread flush needs g_reorder_render_tid, which is set only once that identity
    // is known, and the scene texture's name, which a scene resolve on that thread gives it (KH_SCENE_NAME: at once
    // when none is named, else once the old one has gone KH_DEPTH_READOPT_RUN cycles unseen). So park until they are
    // known, then stop; the clear hook drops a stale identity (another size, or single-sample once FSAA is on:
    // KH_FSAA_SWITCH), KH_WATCH_STALL below a same-size one, and this parks again until it is re-adopted. WATCHDOG:
    // also park when the render thread has not flushed for a couple of cycles (no scene resolve, an unrecognised pass
    // shape, a stood-down hook).
    // KH_WATCH_STALL: this sample against the last - the engine cleared depth, and the published cycle did not move.
    // Samples without a depth clear neither count nor reset (a screen with no 3D scene between two stalled ones).
    {
        const uint64_t khnp_pub = g_topo_cycles_pub.load(std::memory_order_relaxed);
        const uint64_t khnp_clr = g_watch_clears.load(std::memory_order_relaxed);
        if (khnp_pub != g_watch_cyc_seen) {
            g_watch_still = 0;
        } else if (khnp_clr != g_watch_clears_seen && ++g_watch_still >= KH_WATCH_STALL_FRAMES) {
            g_watch_still = 0;
            g_main_depth_identity.store(nullptr, std::memory_order_relaxed);   // Back to the bootstrap: the park below.
            g_reorder_render_tid.store(0, std::memory_order_relaxed);
        }
        g_watch_cyc_seen = khnp_pub;
        g_watch_clears_seen = khnp_clr;
    }
    const uint64_t khnp_last = g_rtshadow_cycle.load(std::memory_order_relaxed);
    const bool khnp_ready = g_main_depth_identity != nullptr &&
                            g_reorder_render_tid.load(std::memory_order_relaxed) != 0 &&
                            khnp_last != ~0ull && g_topo_cycles_pub.load(std::memory_order_relaxed) - khnp_last <= 2ull;   // The published cycle counter, not the render thread's.
    // The deferred mission destroy releases device state and still needs the engine stopped: once per mission end
    // (kh_park_run consumes it before any flush).
    if (!khnp_ready || g_mission_destroy_pending.load(std::memory_order_relaxed)) {
        kh_park_run(dev, ctx);
    }
}

inline std::string ensure_backbuffer_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11Texture2D* src_tex, const D3D11_TEXTURE2D_DESC& td,
                                             bool khbc_copy = true) {
    if (!g_res.bb_tex || g_res.bb_w != td.Width || g_res.bb_h != td.Height || g_res.bb_fmt != td.Format) {
        g_res.release_bb_capture();
        D3D11_TEXTURE2D_DESC bd = {};
        bd.Width = td.Width;
        bd.Height = td.Height;
        bd.MipLevels = 1;
        bd.ArraySize = 1;
        bd.Format = td.Format;
        bd.SampleDesc.Count = 1;
        bd.Usage = D3D11_USAGE_DEFAULT;
        // RENDER_TARGET joins SHADER_RESOURCE: the spill lane's premultiplying capture draws into this texture.
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        HRESULT hr = dev->CreateTexture2D(&bd, nullptr, &g_res.bb_tex);
        if (FAILED(hr)) return "create bb tex " + hr_str(hr);
        hr = dev->CreateShaderResourceView(g_res.bb_tex, nullptr, &g_res.bb_srv);
        if (FAILED(hr)) { g_res.release_bb_capture(); return "create bb SRV " + hr_str(hr); }
        hr = dev->CreateRenderTargetView(g_res.bb_tex, nullptr, &g_res.bb_rtv);
        if (FAILED(hr)) { g_res.release_bb_capture(); return "create bb RTV " + hr_str(hr); }
        g_res.bb_w = td.Width;
        g_res.bb_h = td.Height;
        g_res.bb_fmt = td.Format;
    }

    if (khbc_copy) ctx->CopyResource(g_res.bb_tex, src_tex);
    return "";
}

// Spill-lane capture: one systemic premultiplying copy in place of per-shader gather treatment.
inline std::string kh_capture_bb_premult(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                         ID3D11Texture2D* src_tex, const D3D11_TEXTURE2D_DESC& td) {
    const std::string khpc_err = ensure_backbuffer_capture(dev, ctx, src_tex, td, false);
    if (!khpc_err.empty()) return khpc_err;   // Creation only - the draw below fills every texel.
    if (!g_res.ps_premult_copy || !g_res.bb_rtv || !g_res.vs_fullscreen) return "premult capture resources missing";

    ID3D11ShaderResourceView* khpc_live = nullptr;
    HRESULT hr = dev->CreateShaderResourceView(src_tex, nullptr, &khpc_live);
    if (FAILED(hr) || !khpc_live) return "create live bb SRV " + hr_str(hr);

    // The flush already owns IA/VS/dss/RS state (fullscreen configuration);
    // only the target, PS, SRV and blend differ for this one draw.
    ID3D11RenderTargetView* khpc_prev_rtv = nullptr;
    ID3D11DepthStencilView* khpc_prev_dsv = nullptr;
    ctx->OMGetRenderTargets(1, &khpc_prev_rtv, &khpc_prev_dsv);
    ID3D11RenderTargetView* khpc_tgt = g_res.bb_rtv;
    ID3D11ShaderResourceView* khpc_null = nullptr;
    // KH_PREMULT_T0_OFF: t0 off before bb_rtv goes on - the lane's last draw may have left bb_srv there (a hazard
    // the runtime would resolve by nulling it); t0 takes the live view below either way.
    ctx->PSSetShaderResources(0, 1, &khpc_null);
    ctx->OMSetRenderTargets(1, &khpc_tgt, nullptr);
    ctx->PSSetShader(g_res.ps_premult_copy, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &khpc_live);
    const FLOAT khpc_bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(nullptr, khpc_bf, 0xFFFFFFFF);   // Full replace, alpha included.
    ctx->Draw(3, 0);
    ctx->PSSetShaderResources(0, 1, &khpc_null);
    ctx->OMSetRenderTargets(1, &khpc_prev_rtv, khpc_prev_dsv);
    if (khpc_prev_rtv) khpc_prev_rtv->Release();
    if (khpc_prev_dsv) khpc_prev_dsv->Release();
    khpc_live->Release();
    return "";
}

// Backbuffer format and dims. ensure_backbuffer_capture releases the whole capture group on any dims/format change
// before this runs, so an existing texture matches bb_tex by construction.
inline std::string ensure_ui_chain(ID3D11Device* dev, const D3D11_TEXTURE2D_DESC& td) {
    if (g_res.ui_chain_tex) return "";
    D3D11_TEXTURE2D_DESC khuc_td = {};
    khuc_td.Width = td.Width;
    khuc_td.Height = td.Height;
    khuc_td.MipLevels = 1;
    khuc_td.ArraySize = 1;
    khuc_td.Format = td.Format;
    khuc_td.SampleDesc.Count = 1;
    khuc_td.Usage = D3D11_USAGE_DEFAULT;
    khuc_td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    HRESULT hr = dev->CreateTexture2D(&khuc_td, nullptr, &g_res.ui_chain_tex);
    if (FAILED(hr)) return "create ui chain tex " + hr_str(hr);
    hr = dev->CreateShaderResourceView(g_res.ui_chain_tex, nullptr, &g_res.ui_chain_srv);

    if (FAILED(hr)) {
        KH_SAFE_RELEASE(g_res.ui_chain_tex);
        return "create ui chain SRV " + hr_str(hr);
    }

    hr = dev->CreateRenderTargetView(g_res.ui_chain_tex, nullptr, &g_res.ui_chain_rtv);

    if (FAILED(hr)) {
        KH_SAFE_RELEASE(g_res.ui_chain_srv);
        KH_SAFE_RELEASE(g_res.ui_chain_tex);
        return "create ui chain RTV " + hr_str(hr);
    }

    return "";
}

// KH_UI_BRIGHT_ID - whether a UI-lane pass of bloom, halation, lens flare or anamorphic writes back the very bits it
// read: the lane's values are 8-bit UNORM (sRGB-decoded or not), at most 1 after the spill lane's premultiply, so a
// threshold (fx[0]) of 1 or more takes every tap - direct, and through the pyramid seeds (PSGlowSeed / PSAnaSeed),
// whose every level is then 0 - to max(x - threshold, 0) = 0; the glow (0 times finite lanes) adds +-0, the opacity
// lerp keeps the scene, and the spill algebra returns raw x cov + raw x (1 - cov): raw within a few float ulps,
// which the 8-bit write rounds to raw's own code; alpha passes through; the probe's early exit returns raw. The
// lanes held finite and within 1e6 keep every tap position finite; a lens flare's ghostSpacing 0 or between 1e-3
// and 1e4 on a frame with an even side keeps its fringe's normalize off (0, 0) - NaN coordinates. Not localized,
// banded or textured (no mask; no page request). flush_ui_locked holds such a pass back instead of drawing it.
// TWIN: effect.hlsl's branches 8 / 12 / 14 / 15, PSGlowSeed / PSAnaSeed, cb.hlsl's KhFxFinish spill tail.
inline bool kh_ui_bright_id(const RenderObject& o, UINT khub_w, UINT khub_h) {
    const bool khub_e = o.effect == static_cast<int>(EffectId::Bloom) ||
                        o.effect == static_cast<int>(EffectId::Halation) ||
                        o.effect == static_cast<int>(EffectId::LensFlare) ||
                        o.effect == static_cast<int>(EffectId::Anamorphic);
    if (!khub_e || !(o.fx[0] >= 1.0f) || o.localized || o.banded || kh_fx_tex_on(o)) return false;
    for (int khub_i = 0; khub_i < 12; ++khub_i)
        if (!(fabsf(o.fx[khub_i]) <= 1.0e6f)) return false;   // NaN fails too.
    for (int khub_i = 0; khub_i < 4; ++khub_i)
        if (!(fabsf(o.color[khub_i]) <= 1.0e6f)) return false;
    if (o.effect == static_cast<int>(EffectId::LensFlare)) {
        const float khub_s = fabsf(o.fx[3]);
        const bool khub_odd = (khub_w & 1u) != 0u && (khub_h & 1u) != 0u;   // Both sides odd.
        if (khub_s != 0.0f && (khub_s < 1.0e-3f || khub_s > 1.0e4f || khub_odd)) return false;
    }
    return true;
}
}