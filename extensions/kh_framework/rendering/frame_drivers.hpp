#pragma once

// The injection half (rendering_integration_injection.hpp): the game-thread drivers: the game thread id, video options
// and lighting staging, the park, flush_frame, the backbuffer capture and the UI chain. Never include it on its own.
namespace RenderIntegration {
inline void ensure_present_hook();   // Defined with the hook (rendering/present_hook.hpp).
inline int overlay_begin();       // OVERLAY_BRACKET, defined with the Present subscriber.
inline void overlay_end(int tok);

inline void ui_driver_rehoist();   // Defined with the driver lifecycle (the auto-hoist site,
                                      // rendering/ui_flush.hpp).
// RESET_THREAD: the game thread's id, stamped at the heads of flush_frame (Draw3D) and flush_ui_frame (their paths
// to ensure_resources install the reset hook) and in ensure_draw_eh (every object's add), so it is set before any
// object can feed a path to the hook - the render thread's, or the game thread's inside an engine call (fx_late_gt's
// chain-only flush at a context resolve, present_cb's flush_ui_locked at Present), each behind a demand flag that
// flush_frame or an object's add raises after its stamp (g_track_wanted for the render-thread flush that owes the
// late chain, g_ui_mask_wanted for the Present flush). on_engine_reset takes the graphics lock only on the game
// thread, like every other ScopedGraphicsLock site. Process-scoped (the game thread never changes): no session reset.
static std::atomic<uint32_t> g_game_tid{ 0 };

// Runs from flush_frame (stage_world_lighting) before the graphics lock is taken: it makes SQF/engine calls and must
// not extend the render-thread park. Its readings go straight to atomics either thread reads (g_obj_vis, g_sun_range,
// g_shadows_off); the fog stage_world_lighting stages beside it is published inside flush_locked
// (publish_world_lighting).

// VIDEO_OPT_CADENCE: both settings come from one engine call every sample - getObjectViewDistance, the video
// options' [object view distance, shadow view distance] - and the interval caps it at four a second. Do not read
// getVideoOptions (it builds a map of every video setting, ~1.2 ms a call) or getShadowDistance (it does not follow a
// change made in the video options at runtime, measured).
// SHADOW_VIS: the shadow view distance, read on every sample like the object view distance - no display gate, no
// pause rule: a sample is taken only when a Draw3D's flush_frame reaches the staging (some mesh or pass exists, the
// bridge is up), and each one reads the current setting. Envelope: a changed shadow or object view distance reaches
// g_shadows_off / g_sun_range / g_obj_vis up to VIDEO_OPT_INTERVAL_MS after the first sample that follows the
// change.
static constexpr uint64_t VIDEO_OPT_INTERVAL_MS = 250;
static uint64_t g_video_opt_ms = 0;   // Game thread: the last sample.
inline void stage_video_options() {
    try {
        const uint64_t now_ms = steady_now_ms();
        if (g_video_opt_ms != 0 && now_ms - g_video_opt_ms < VIDEO_OPT_INTERVAL_MS) return;
        g_video_opt_ms = now_ms;
        // SHADOW_VIS: both view distances, one call.
        const auto rd = sqf::get_object_view_distance();
        const float sh = rd.shadow_distance;
        // SHADOW_OFF: whoever owns the range below; SHADOW_OFF_M (5 m) or below reads as off, a NaN leaves the
        // switch as it stands.
        if (sh == sh) g_shadows_off.store(sh <= SHADOW_OFF_M, std::memory_order_relaxed);
        // The shadow range is not written while the engine's band table owns it (src == 2: the far border of the
        // engine's cascades, published by the render thread): the two would overwrite each other. A NaN or a
        // non-positive reading leaves the previous value standing.
        if (g_sun_setup.sun_range_src.load(std::memory_order_relaxed) != 2 && sh == sh && sh > 0.0f) {
            g_sun_range.store(sh, std::memory_order_relaxed);
            g_sun_setup.sun_range_src.store(1, std::memory_order_relaxed);
        }
        const float obj = rd.object_distance;
        if (obj == obj && obj > 0.0f) {
            g_obj_vis.store(obj, std::memory_order_relaxed);
        }
    } catch (...) {}
}
inline void stage_world_lighting() {
    stage_video_options();
    // DL_ORIGIN_GRID: the terrain's land-grid cell, once per session (an engine call, like the reads above).
    if (!(g_dlcap.cell.load(std::memory_order_relaxed) > 0.0f)) {
        try {
            const auto t = sqf::get_terrain_info();
            const float w = t.land_grid_width;
            if (w == w && w > 0.0f && w < 1.0e5f) {
                g_dlcap.cell.store(w, std::memory_order_relaxed);
            }
        } catch (...) {}
    }

    // Atmospheric occlusion applies to every world-space mesh; stage it whenever the flush runs.

    try {
        const auto fp = sqf::fog_params();   // The engine call outside the mutex.
        std::lock_guard<std::mutex> l(g_fog_stage_mu);   // NO_PARK.
        g_fog_staged[0] = fp.value;
        g_fog_staged[1] = fp.decay;
        g_fog_staged[2] = fp.base;
        g_fog_staged_valid = true;
    } catch (...) {
        { std::lock_guard<std::mutex> l(g_fog_stage_mu); g_fog_staged_valid = false; }
        // Fog unavailable this frame: the fog term stands down.
    }
}

// The park: bootstrap, watchdog and mission end.
inline void park_run(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int64_t wait_t0 = prof_now();   // PARK_WAIT.
        RVExtBridge::ScopedGraphicsLock lock;
        prof_add(PROF_FLUSH_LOCK_WAIT, wait_t0);

        if (!lock.acquired()) {
            stat_inc(g_stats.lock_retries);
            continue;
        }
        // Deferred mission destroy (armed only by a lock-exhausted mission end): consumed here because this lock is the
        // park invariant the teardown could not acquire.
        if (g_mission_destroy_pending.exchange(false, std::memory_order_relaxed)) {
            // The same teardown rendering_integration_reset runs under its own park (session_teardown), so a mission
            // whose edge could not take the lock tears down no differently.
            g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
            // Not clean: this park is the NEXT mission's (flush_frame parks only with a non-empty draw list), so its
            // objects' meshes, materials, scene slots, handles and clock stay; the old mission's were dropped by its
            // reset_retained_state, and the collectors take what they left.
            // TEARDOWN_KEEP_SETTINGS: the script settings are that mission's too (its handlers may have set
            // them before this park; rendering_integration_reset put the old mission's back to defaults), so the
            // teardown's session reset does not take them.
            const bool dls = g_dls_allowed.load(std::memory_order_relaxed);
            const float sg = g_ssgi_scale.load(std::memory_order_relaxed);
            const uint32_t aos = g_ao_strength_bits.load(std::memory_order_relaxed);
            const uint32_t aod = g_ao_dist_bits.load(std::memory_order_relaxed);
            const bool st = g_stats_armed.load(std::memory_order_relaxed);
            session_teardown(false);
            g_dls_allowed.store(dls, std::memory_order_relaxed);
            g_ssgi_scale.store(sg, std::memory_order_relaxed);
            g_ao_strength_bits.store(aos, std::memory_order_relaxed);
            g_ao_dist_bits.store(aod, std::memory_order_relaxed);
            g_stats_armed.store(st, std::memory_order_relaxed);
            // MESH_CACHE_LATER: the teardown stopped the mesh-cache writer but kept the imports this mission made
            // before it (not clean); their landing hands writes only to a running writer (EA_MW_INLINE), so it
            // starts again here, on the game thread.
            bool mcl = false;
            {
                std::lock_guard<std::mutex> g(g_mesh_cache_later_mu);
                mcl = !g_mesh_cache_later_q.empty();
            }
            if (mcl) mesh_cache_writer_start();
            ++g_parks;   // PARK_COUNT: after the teardown, whose stats reset would zero it - this session's park.
            return;
        }
        ++g_parks;   // PARK_COUNT.
        // PARK_STEP: a park's flush draws late and needs this cycle's poses; the render-thread step inside
        // flush_locked is rt only. Taken with the render thread stalled: a lane the render thread already stepped
        // this cycle is skipped (the cycle gate); others take the cycle's sample (gts_for_cycle), or hold.
        attach_step();
        // Keeps our draws out of the hooks; released on unwind too (a flag left
        // true would make every hook treat engine draws as ours).
        // PARK_INJ: on the thread the hooks take for the render thread (the game thread at FSAA 1x, which clears the
        // main depth) g_ro.in_injection is raised for the whole flush, as fx_late_gt and the Present subscriber
        // raise it - the hooks' render-thread branch reads that flag, not FlushActiveScope's. InjectionFlagScope
        // restores it.
        {
            FlushActiveScope active;
            InjectionFlagScope inj;
            if (reorder_on_render_thread()) g_ro.in_injection = true;
            g_flush_wrong_pass = false;
            flush_locked(dev, ctx, false);
        }
        // A wrong pass here differs from one at Draw3D: at this handover the engine may be in the view model's depth
        // partition, so flush_locked may refuse and this loop sleeps up to three times holding the lock inside the
        // engine's resolve call.
        if (!g_flush_wrong_pass) {  return; }
        // The park landed on a non-main depth (a PIP pass, a cascade) and drew nothing. The lock is released after this
        // Sleep(0) and the next attempt parks again; the frame is lost only when every attempt lands wrong.
        Sleep(0);
    }

    stat_inc(g_stats.lock_failed_frames);   // No park drew: every attempt failed the lock or landed wrong.
}

// SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static float g_ui_rehoist_auto_s = -1.0e9f;
inline void flush_frame() {
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // RESET_THREAD.
    KH_PROF_SCOPE(PROF_FLUSH_FRAME);   // PROF.
    const int64_t pre_t0 = prof_now();   // PROF: flushPre.
    // ATTACH_EDEN: this frame's flag, ahead of every read below (the crew table, the sampler, the chains); the
    // render thread's reads take the latest flag stored (this Draw3D's, or before it lands the previous one's).
    attach_eden_note();
    // ATTACH: no step here. The late draw is stepped by the render thread's flush or by park_run; a game-thread
    // STEP would write the poses at a different instant from the render thread's, which is the authority. A read is
    // another matter: ATTACH_GT_SNAP's sample is taken here, below, because Draw3D is the instant the engine's own
    // drawn state is coherent. The render thread's step consumes it and stays the authority on poses.
    // ATTACH_DEAD: drain what the step (either thread) found dead. Here because this is the game thread and the
    // erase releases game_values; before the work test below, so a frame whose only remaining mesh was following a
    // deleted object finds nothing left to draw.
    attach_reap();
    // ATTACH_BONE: after the reap, because the reap is what retires the proxies of the meshes it just erased, and
    // before the park, because this one makes SQF calls.
    attach_proxy_reap();
    // CREW_LAG: the crew table and this Draw3D's corrections - holding nothing (SQF calls), after the reaps
    // and before the sampler and the chain's end (chain_prepare), which apply them.
    crew_sync();
    // ATTACH_GT_SNAP: this frame's objects, as the engine will draw them. After the reaps (a reaped binding
    // takes no sample) and before the park, on the thread the SQF commands write the table from.
    const bool gts = attach_gt_snap();
    prof_add(PROF_FLUSH_PRE, pre_t0);
    const int64_t cen_t0 = prof_now();   // PROF: flushCensus.
    bool has_work;
    bool cloth = false;   // CLOTH: some object asks to simulate (cloth_sync's inert gate).

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        has_work = !g_draw_list.empty();

        // An empty list clears it for free.
        bool wanted = false;
        bool lit = false;   // A visible lit mesh, or any shadow-active one (casterOnly counts): g_ls.wanted.
        bool mesh = false;   // Any visible mesh at all (volume-copy demand).
        bool fogsc = false;   // FOG_PROBE: a visible fog-scatter pass.
        bool dlf = false;   // DLF_DEMAND: a visible scene-chain dynamicLightFog pass.
        bool front = false;   // INFRONT: any visible view-model mesh (the slice hook's demand).
        bool fpwatch = false;   // SHOW_INFRONT: a visible world mesh with showInFront false.
        g_cloth_proxy_want.clear();   // CLOTH_PROXY.
        const float now_s = effect_time_seconds();   // NO_PARK_EXPIRY: the expiry test's clock.
        // NO_PARK_EXPIRY - expired objects are erased here (no park comes for them), on the game thread under
        // g_draw_list_mutex, the same sequence remove_render_object and attach_reap run; render-thread walks skip an
        // expired object meanwhile.
        static std::vector<std::string> expired;
        expired.clear();
        ref_note_begin(g_ref_note_cen);   // REF_NOTE_ONCE.
        g_mat_note_cen.clear();   // MAT_NOTE_ONCE.

        for (const auto& um_kv : g_draw_list) {
            {   // NO_PARK_EXPIRY.
                bool x = false;
                lifetime_envelope(um_kv.second, now_s, x);
                if (x) { expired.push_back(um_kv.first); continue; }
            }
            if (!um_kv.second.fullscreen) ref_note_add(g_ref_note_cen, um_kv.second.mesh);   // MESH_FREE, REF_NOTE_ONCE.
            if (um_kv.second.materials) g_mat_note_cen.push_back(um_kv.second.materials);   // MAT_POOL,
                                                                                                // MAT_NOTE_ONCE.
            // Shadow-live demand: a visible lit object or any shadow-active object (casterOnly too).
            // MESH_SSGI: an "ssgi" effect mesh draws nothing (the flush skips it), so it demands nothing here.
            const bool nodraw = !um_kv.second.fullscreen &&
                                (um_kv.second.effect == static_cast<int>(EffectId::Ssgi) ||
                                 um_kv.second.effect == static_cast<int>(EffectId::DynLightFog));   // DLF.
            if ((obj_vis(um_kv.second) && um_kv.second.lit && !nodraw &&   // SHOW_INFRONT.
                 (!obj_in_front(um_kv.second) || g_vm_slice_live.load(std::memory_order_relaxed))) ||   // INFRONT.
                shadow_active(um_kv.second)) lit = true;
            if (cloth_obj_sim(um_kv.second) || chain_obj_sim(um_kv.second)) {   // CHAIN too.
                cloth = true;   // Visible or not, as the sync counts it.
                // CLOTH_PROXY: what the proxy step needs, taken under this lock.
                const float* p = um_kv.second.pos;
                g_cloth_proxy_want.push_back({ um_kv.first, um_kv.second.seq, { p[0], p[1], p[2] } });
            }
            if (!obj_vis(um_kv.second) || nodraw) continue;   // SHOW_INFRONT: obj_vis.

            if (um_kv.second.fullscreen) {
                if (um_kv.second.affect_ui) wanted = true;
                if (um_kv.second.effect == static_cast<int>(EffectId::Fogscatter)) fogsc = true;   // FOG_PROBE.
                // DLF: a scene-chain dynamicLightFog pass reads the engine fog (the probe) and the dynamic-light
                // pool - the light capture's demand alone (DLF_DEMAND), not a lit mesh's, which would also arm
                // the engine shadow capture the pass never reads (the FSAA 1x stand-down below clears it too).
                if (um_kv.second.effect == static_cast<int>(EffectId::DynLightFog) && !um_kv.second.ui_only) {
                    fogsc = true;
                    dlf = true;
                }
            } else {
                mesh = true;   // Every visible mesh reads the volume copy and the snapshots.
                // INFRONT: the slice hook's demand too. SHOW_INFRONT: an inFront mesh that is never shown
                // asks for nothing (obj_vis, above - as visible false); a world one with showInFront false asks
                // for the verdict.
                if (obj_in_front(um_kv.second)) {
                    if (um_kv.second.show_in_front) front = true;
                } else if (!um_kv.second.show_in_front) {
                    fpwatch = true;
                }
            }
            // No early-out: the two reference notes above must reach every object (an unnoted mesh or material set is
            // released by the GCs after their idle window while the object still carries it).
        }

        ref_note_end(g_ref_note_cen);   // REF_NOTE_ONCE: one stamp per id, one lock.
        material_note_end(g_mat_note_cen);   // MAT_NOTE_ONCE: likewise, one stamp per set.
        g_ref_note_cen_ms.store(GetTickCount64(), std::memory_order_release);   // GC_NOTE_SKIP.
        for (const std::string& h : expired) {   // As remove_render_object erases.
            auto it = g_draw_list.find(h);
            if (it == g_draw_list.end()) continue;
            scene_stage_remove(it->second);
            attach_drop(h);   // ATTACH.
            g_draw_list.erase(it);
        }
        if (!expired.empty()) has_work = !g_draw_list.empty();
        g_ui_mask_wanted.store(wanted, std::memory_order_relaxed);
        g_svs_mesh_wanted.store(mesh, std::memory_order_relaxed);
        g_fogsc_wanted.store(fogsc, std::memory_order_relaxed);   // FOG_PROBE.
        g_infront.wanted.store(front, std::memory_order_relaxed);   // INFRONT.
        g_infront.watch_wanted.store(front || fpwatch, std::memory_order_relaxed);   // SHOW_INFRONT.
        // FSAA 1x: the lit demand stands the whole shadow-live ecosystem down through the existing master-demand gates.
        if (lit && fsaa_world_standdown()) lit = false;
        g_ls.wanted.store(lit, std::memory_order_relaxed);
        if (dlf && fsaa_world_standdown()) dlf = false;   // DLF_DEMAND: as the lit demand.
        g_dlf_wanted.store(dlf, std::memory_order_relaxed);
    }

    prof_add(PROF_FLUSH_CENSUS, cen_t0);
    const int64_t prep_t0 = prof_now();   // PROF: flushPrep (closed at the park decision).
    // CLOTH_PROXY: holding nothing, and ahead of the empty-list return
    // below, so the last cloth mesh's removal still deletes its proxy.
    if (!g_cloth_proxy_want.empty() || !g_cloth_proxy.empty()) cloth_proxy_step();
    chain_prepare(gts);   // CHAIN: likewise holding nothing and ahead of it.

    g_track_wanted.store(has_work, std::memory_order_relaxed);

    if (!has_work) {
        // EA_EMPTY_RETIRE: the last object's removal still retires its skin / cloth / chain instances (the cloth
        // and chain buffers to their grave, every instance's CPU state freed; a skin instance's GPU buffer stays until
        // the next skin upload, a device reset or the session teardown); only the syncs retire, and nothing else would
        // run them.
        if (!g_skin.empty()) skin_sync();
        if (!g_cloth.empty()) cloth_sync(cloth_frame_dt(), false);
        return;
    }


    if (g_ui_mask_wanted.load(std::memory_order_relaxed)) {
        const float now_s = effect_time_seconds();

        if (now_s - g_ui_rehoist_auto_s >= 1.0f) {
            g_ui_rehoist_auto_s = now_s;
            ui_driver_rehoist();
        }
    }
    if (!RVExtBridge::is_initialized()) return;
    // cold_t0: first flush whose draw list holds a shadow-capable mesh (any
    // mesh, so a translucent-first session opens the window).
    if (g_mask.cold_t0 < 0.0) {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;

            if (o.visible && !o.fullscreen && o.effect == 0 &&
                o.mode != DepthMode::Off) {
                g_mask.cold_t0 = static_cast<double>(effect_time_seconds());
                break;
            }
        }
    }
    sun_size_latch();   // SUN_LADDER: the hook-less path latches here.
    stage_world_lighting();   // Game thread: the video options and the fog (pre-lock).
    thm_autobuild_step();   // Game thread: zero-setup terrain acquisition (pre-lock).
    // DESTROY_FIRST: a mission edge whose park failed left the old mission's teardown pending and the render
    // thread disarmed (no context). It is taken here, first, before ensure_reorder_hook re-arms that context:
    // armed first, at FSAA above 1x the render thread could flush once with the old mission's learned targets,
    // locators and shadow state under this mission's objects. The park's teardown releases this frame's work
    // anyway (park_run returns right after it); a park that fails again leaves the context unarmed.
    if (g_mission_destroy_pending.load(std::memory_order_relaxed)) {
        ID3D11Device* df_dev = RVExtBridge::get_d3d_device();
        ID3D11DeviceContext* df_ctx = RVExtBridge::get_d3d_device_context();
        if (df_dev && df_ctx) park_run(df_dev, df_ctx);
        return;
    }
    psv_hook_ensure();   // PSV: the CreatePixelShader hook, if no earlier point could install it.
    ensure_reorder_hook();   // Cheap early-out once installed; refreshes the tracked context.
    // OVERLAY_BRACKET: once the context hooks are live, another module's present-time draws are excluded from them
    // - with or without a UI pass of ours (ensure_present_hook, below, runs only with one). Two stores; idempotent.
    if (g_reorder_hook.active.load(std::memory_order_acquire)) {
        kh_present_overlay_register(&overlay_begin, &overlay_end);
    }
    if (g_ui_mask_wanted.load(std::memory_order_relaxed)) ensure_present_hook();   // PRESENT_UI: once, when a UI pass first exists.
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return;

    {   // FLUSH_MISS: did the injection land since the previous flush_frame (a park's verdict).
        const uint64_t serial = g_composite_inject_serial.load(std::memory_order_relaxed);
        g_flush_landed_this_frame = serial != g_flush_miss_seen_serial;
        g_flush_miss_seen_serial = serial;
    }
    prof_add(PROF_FLUSH_PREP, prep_t0);
    g_flush_serial.fetch_add(1, std::memory_order_relaxed);   // FLUSH_SERIAL: the graves' clock.
    stat_inc(g_stats.flushes);

    // CLOTH: queue this frame's simulation before the park. Off the park deliberately - the workers must be running
    // WHILE the render thread draws, not inside the window that stops it. The wind is sampled immediately ahead of it
    // because this is the game thread and sqf::wind is an engine call, which is the same reason the fog sample sits
    // where it does. EA_WIND_GATE: only with something simulating - the sync's instances are its one reader.
    if (cloth) cloth_wind_sample();
    // CLOTH_GROUND: the terrain's split, measured once per matrix and mission - engine calls, so here, holding
    // nothing, before the sync copies each cloth's ground.
    if (cloth) cloth_ground_prepare();
    // SKEL: the proxies are read and the poses skinned here for the same reason - on the game thread, off the park,
    // the upload inside it - and AHEAD of the cloth, which takes a simulating binding's pose from it.
    skin_sync();
    cloth_sync(cloth_frame_dt(), cloth);
    {   // INJ_FORK: the pool on demand for the render thread's per-object forks
        // (fork_each), which never start it themselves (that takes g_cloth_mu).
        // Idempotent once started; stopped with the session.
        size_t nobj = 0;
        { std::lock_guard<std::mutex> l(g_draw_list_mutex); nobj = g_draw_list.size(); }
        if (nobj >= FORK_EACH_MIN && cloth_fork_workers() < 2u) cloth_workers_start();
    }

    // NO_PARK: the contract below describes only the park (bootstrap, watchdog, mission end). Every other frame the
    // flush runs on the render thread inside its scene resolve (thread identity keeps the injection out), the scene
    // chain may run inside a later resolve (FX_LATE), and shared game-thread data is under g_draw_list_mutex,
    // g_skin_mu, g_cloth_mu, g_mesh_ref_mu or atomics. THREADING CONTRACT - DO NOT CHANGE. ScopedGraphicsLock is the
    // engine's RVExtensionGLock: while held the engine does not render, so the render thread cannot be inside any
    // hooked context method. That makes the flush's context use, ensure_mesh_vbs's vector growth, the cb_ring cursors
    // and g_res.release() safe against the injection. The injection never takes this lock (deadlock).
    // get_projection_view_transform is read from the render thread without the lock by design (a read; one frame stale
    // on hard cuts). The ordinal stays here: the UI flush's dark-driver test (ui_frame_ordinal, on the lock path)
    // expects one advance per frame on this thread and reads a jump of two as a dark driver.
    g_flush_frame++;
    g_flush_frame_pub.store(g_flush_frame, std::memory_order_relaxed);
    // NO_PARK BOOTSTRAP: g_main_depth_identity is adopted by flush_locked
    // (and by the scene resolve once the render thread is named:
    // DEPTH_READOPT), and the render-thread flush needs g_reorder_render_tid, which is set only once that identity
    // is known, and the scene texture's name, which a scene resolve on that thread gives it (SCENE_NAME: at once
    // when none is named, else once the old one has gone DEPTH_READOPT_RUN cycles unseen). So park until they are
    // known, then stop; the clear hook drops a stale identity (another size, or single-sample once FSAA is on:
    // FSAA_SWITCH), WATCH_STALL below a same-size one, and this parks again until it is re-adopted. WATCHDOG:
    // also park when the render thread has not flushed for a couple of cycles (no scene resolve, an unrecognised pass
    // shape, a stood-down hook).
    // WATCH_STALL: this sample against the last - the engine cleared depth, and the published cycle did not move.
    // Samples without a depth clear neither count nor reset (a screen with no 3D scene between two stalled ones).
    {
        const uint64_t pub = g_topo_cycles_pub.load(std::memory_order_relaxed);
        const uint64_t clr = g_watch.clears.load(std::memory_order_relaxed);
        if (pub != g_watch.cyc_seen) {
            g_watch.still = 0;
        } else if (clr != g_watch.clears_seen && ++g_watch.still >= WATCH_STALL_FRAMES) {
            g_watch.still = 0;
            g_main_depth_identity.store(nullptr, std::memory_order_relaxed);   // Back to the bootstrap: the park below.
            g_reorder_render_tid.store(0, std::memory_order_relaxed);
        }
        g_watch.cyc_seen = pub;
        g_watch.clears_seen = clr;
    }
    const uint64_t last = g_rtshadow_cycle.load(std::memory_order_relaxed);
    const bool ready = g_main_depth_identity != nullptr &&
                       g_reorder_render_tid.load(std::memory_order_relaxed) != 0 &&
                       last != ~0ull && g_topo_cycles_pub.load(std::memory_order_relaxed) - last <= 2ull;   // The published cycle counter, not the render thread's.
    // A pending deferred mission destroy never reaches here: DESTROY_FIRST above parks for it and returns, every
    // frame it is pending (rendering_integration_reset, its one setter, does not run inside flush_frame).
    if (!ready) park_run(dev, ctx);
}

inline std::string ensure_backbuffer_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11Texture2D* src_tex, const D3D11_TEXTURE2D_DESC& td,
                                             bool copy = true) {
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

    if (copy) ctx->CopyResource(g_res.bb_tex, src_tex);
    return "";
}

// Spill-lane capture: one systemic premultiplying copy in place of per-shader gather treatment.
inline std::string capture_bb_premult(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                      ID3D11Texture2D* src_tex, const D3D11_TEXTURE2D_DESC& td) {
    const std::string err = ensure_backbuffer_capture(dev, ctx, src_tex, td, false);
    if (!err.empty()) return err;   // Creation only - the draw below fills every texel.
    if (!g_res.ps_premult_copy || !g_res.bb_rtv || !g_res.vs_fullscreen) return "premult capture resources missing";

    ID3D11ShaderResourceView* live = nullptr;
    HRESULT hr = dev->CreateShaderResourceView(src_tex, nullptr, &live);
    if (FAILED(hr) || !live) return "create live bb SRV " + hr_str(hr);

    // The flush already owns IA/VS/dss/RS state (fullscreen configuration);
    // only the target, PS, SRV and blend differ for this one draw.
    ID3D11RenderTargetView* prev_rtv = nullptr;
    ID3D11DepthStencilView* prev_dsv = nullptr;
    ctx->OMGetRenderTargets(1, &prev_rtv, &prev_dsv);
    ID3D11RenderTargetView* tgt = g_res.bb_rtv;
    ID3D11ShaderResourceView* null = nullptr;
    // PREMULT_T0_OFF: t0 off before bb_rtv goes on - the lane's last draw may have left bb_srv there (a hazard
    // the runtime would resolve by nulling it); t0 takes the live view below either way.
    ctx->PSSetShaderResources(0, 1, &null);
    ctx->OMSetRenderTargets(1, &tgt, nullptr);
    ctx->PSSetShader(g_res.ps_premult_copy, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &live);
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);   // Full replace, alpha included.
    ctx->Draw(3, 0);
    ctx->PSSetShaderResources(0, 1, &null);
    ctx->OMSetRenderTargets(1, &prev_rtv, prev_dsv);
    if (prev_rtv) prev_rtv->Release();
    if (prev_dsv) prev_dsv->Release();
    live->Release();
    return "";
}

// Backbuffer format and dims. ensure_backbuffer_capture releases the whole capture group on any dims/format change
// before this runs, so an existing texture matches bb_tex by construction.
inline std::string ensure_ui_chain(ID3D11Device* dev, const D3D11_TEXTURE2D_DESC& td) {
    if (g_res.ui_chain_tex) return "";
    D3D11_TEXTURE2D_DESC uc_td = {};
    uc_td.Width = td.Width;
    uc_td.Height = td.Height;
    uc_td.MipLevels = 1;
    uc_td.ArraySize = 1;
    uc_td.Format = td.Format;
    uc_td.SampleDesc.Count = 1;
    uc_td.Usage = D3D11_USAGE_DEFAULT;
    uc_td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    HRESULT hr = dev->CreateTexture2D(&uc_td, nullptr, &g_res.ui_chain_tex);
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

// UI_BRIGHT_ID - whether a UI-lane pass of bloom, halation, lens flare or anamorphic writes back the very bits it
// read: the lane's values are 8-bit UNORM (sRGB-decoded or not), at most 1 after the spill lane's premultiply, so a
// threshold (fx[0]) of 1 or more takes every tap - direct, and through the pyramid seeds (PSGlowSeed / PSAnaSeed),
// whose every level is then 0 - to max(x - threshold, 0) = 0; the glow (0 times finite lanes) adds +-0, the opacity
// lerp keeps the scene, and the spill algebra returns raw x cov + raw x (1 - cov): raw within a few float ulps,
// which the 8-bit write rounds to raw's own code; alpha passes through; the probe's early exit returns raw. The
// lanes held finite and within 1e6 keep every tap position finite; a lens flare's ghostSpacing 0 or between 1e-3
// and 1e4 on a frame with an even side keeps its fringe's normalize off (0, 0) - NaN coordinates. Not localized,
// banded or textured (no mask; no page request). flush_ui_locked holds such a pass back instead of drawing it.
// TWIN: effect.hlsl's branches 8 / 12 / 14 / 15, PSGlowSeed / PSAnaSeed, cb.hlsl's KhFxFinish spill tail.
inline bool ui_bright_id(const RenderObject& o, UINT w, UINT h) {
    const bool e = o.effect == static_cast<int>(EffectId::Bloom) ||
                   o.effect == static_cast<int>(EffectId::Halation) ||
                   o.effect == static_cast<int>(EffectId::LensFlare) ||
                   o.effect == static_cast<int>(EffectId::Anamorphic);
    if (!e || !(o.fx[0] >= 1.0f) || o.localized || o.banded || fx_tex_on(o)) return false;
    for (int i = 0; i < 12; ++i)
        if (!(fabsf(o.fx[i]) <= 1.0e6f)) return false;   // NaN fails too.
    for (int i = 0; i < 4; ++i)
        if (!(fabsf(o.color[i]) <= 1.0e6f)) return false;
    if (o.effect == static_cast<int>(EffectId::LensFlare)) {
        const float s = fabsf(o.fx[3]);
        const bool odd = (w & 1u) != 0u && (h & 1u) != 0u;   // Both sides odd.
        if (s != 0.0f && (s < 1.0e-3f || s > 1.0e4f || odd)) return false;
    }
    return true;
}
}