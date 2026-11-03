#pragma once

// The shared tail (rendering_integration.hpp): the session lifecycle: the stats and session resets, the teardown and
// the process detach. Never include it on its own.
namespace RenderIntegration {

// The stats reset touches statistics only - the counters and the shader-cache hit / miss pair - and disarms
// collection. Called from the game thread by resetRenderStats (under the graphics lock when its best-effort park
// gets it, without it when not), by rendering_integration_reset's lock-failure arm (unparked, the same case) and by
// the parked session reset, so nothing the render thread depends on may be written here; per-session live state is
// session_scratch_reset below, which only the parked session reset reaches.
inline void reset_stat_counters() {
    g_stats_armed.store(false, std::memory_order_relaxed);
    g_stats = RenderStats{};
    g_parks = 0;   // PARK_COUNT.
    g_ui_parks = 0;
    g_attach_repaired.store(0, std::memory_order_relaxed);   // ATTACH_DIAG.
    g_rp_rec.c = RpCounters();   // VOL_REPLAY.
    wsee_gput_zero_pub();   // WSEE_GPUT: "wseeGpu" (frames in flight stay the clear's to read or drop).
    g_gid_breaks = 0;   // GTS_IDENTITY.
    g_rtp_c = RtpCounters();   // RT_PAINT_READ.
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    prof_reset_pub();   // PROF.
    g_prof.epoch.fetch_add(1u, std::memory_order_release);   // PROF_DISARM: nothing measured before this is read.
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);   // SKIN_GPU: skinBoundWorst.
    g_skin_so_n.store(0, std::memory_order_relaxed);   // skinGpu / skinCpu: republished by the next cycle's skinning.
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // SKIN_CULL: skinCulled.
    g_cloth_skipped.store(0, std::memory_order_relaxed);   // clothSkipped / chainSkipped / skinSkipped.
    g_chain_skipped.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
}

// Per-session scratch that is not a device object; the replay, paint-read and identity counters it zeroes with the
// state they count are statistics reset_stat_counters zeroes too. Must run with the render thread parked.
inline void session_scratch_reset() {
    g_castocc.set.clear(); g_castocc.slack = 0.0f;
    g_castocc.pre.active = false; g_castocc.pre_noreach = false;   // CASTOCC_PRE.
    g_cam_step_m = -1.0f;
    g_fire.count_frame = 0;
    g_fire.clamp_painted = 0;
    g_svs.post_made = 0;
    g_sun_setup.sun5_renders = 0;   // (validity clears at the pass gates).
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    // A cycle-stamped latch restarts with the counter above: a stamp left by the previous session matches once,
    // when the new session's counter reaches it, and that cycle's once-per-cycle work is skipped.
    g_mat_anim_done = ~0ull;   // UV_ANIM: the rewrite's and the time's stamps (the deferred edge keeps the list).
    g_mat_anim_tcyc = ~0ull;
    // ATTACH_GT_SNAP: g_gts_seq and its publication are reset with the objects (session_objects_reset).
    rp_release_records();   // VOL_REPLAY (the targets and copies go with the device).
    rp_img_clear();   // REPLAY_CPUCB.
    g_rp_rec.pass = ~0ull;
    g_rp_rec.ok = g_rp_rec.ended = g_rp_rec.replayed = false;
    g_rp.merge_ok = false;
    g_rp.mir_ok = false;   // MIR_REPLAY.
    g_rp.mir_want = false;
    g_rp.mirb_drawn = false;
    g_rp.mir_pend = false;
    g_rp.mir_pend_cyc = g_rp.mir_pend_pass = ~0ull;
    g_rp.sc_all = RpRect();   // REPLAY_SCISSOR.
    g_rp.sc_mir = RpRect();
    g_rp.sc_gen = ~0ull;
    g_rp.pass_vol = g_rp.pass_mir = false;
    g_rp.rast_on = false;   // REPLAY_RASTER.
    g_vmir.want_cycle = ~0ull;   // MIR_GATE: a cycle stamp.
    g_nzm_cycle.store(~0ull, std::memory_order_relaxed);   // NEARZ_MARK: likewise.
    g_vmsee.want_cycle = ~0ull;   // VM_SEE: likewise.
    g_vmsee.snap_cycle = ~0ull;
    g_vmsee.ok_cycle.store(~0ull, std::memory_order_relaxed);
    g_vmsee.pending = false;
    g_wsee.want_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE: likewise.
    g_wsee.col_want_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE_BWANT.
    g_wsee.col_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.ok_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.taint_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.used_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.snap_cycle = ~0ull;
    g_wsee.on = false;
    g_wsee.pending = WSEE_RE_NONE;
    g_wsee.pending_post = false;
    g_wsee_pd_vp_ok = false;   // The pre-draw's kept viewports are no draw's.
    g_wsee.b2_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.b2_try = ~0ull;
    g_wsee_waste_n = 0;   // WSEE: the back-off restarts with the cycle counter.
    g_wsee_backoff_end = 0;
    g_wsee_flap_prev = -1;   // WSEE_FLAP.
    g_wsee_flap_win_start = 0;
    g_wsee_flap_n = 0;
    g_wsee_flushw_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE_FLUSHW_PRED.
    g_wsee_flushw_off.store(~0ull, std::memory_order_relaxed);
    g_wsee_flushw_probe = 0u;
    g_wsee_flushw_hold = false;
    g_wsee_opq_n = 0;   // The late-opaque cap's count.
    g_wsee_inj_elig = false;   // The capture gate's eligibility.
    g_wsee.cyc_layer = false;   // The cycle's layer-depth verdict.
    g_wsee.ld_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.ld_far_ok = false;
    g_wsee.ld_far_lo = 0.0f;   // The range D's raw was computed in (read under g_wsee.ld_far_ok only).
    g_wsee.ld_far_hi = 1.0f;
    g_psb.cycle.store(~0ull, std::memory_order_relaxed);   // PSB: the one pass's record and the old one's stamp.
    g_psb.drew_cycle.store(~0ull, std::memory_order_relaxed);
    g_psb.zb_cycle.store(~0ull, std::memory_order_relaxed);   // PSB_ZB: no Zb, its guard's state unknown.
    g_psb.zb_gen = 0u;
    g_psb.zb_far = true;
    g_psb.psa_cycle.store(~0ull, std::memory_order_relaxed);
    g_psb.psa_tp_cycle.store(~0ull, std::memory_order_relaxed);   // PSB_TPA.
    g_psb.far_raw = 1.0f;
    g_wsee.od_cycle.store(~0ull, std::memory_order_relaxed);   // The cycle whose capture filled D3.
    g_wsee.d3_sync_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE_OQ.
    g_wsee.d3m_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE_OQ2.
    g_wsee.nopre_cycle.store(~0ull, std::memory_order_relaxed);   // WSEE_NOPRE: the skipped and the S2 cycles.
    g_wsee.s2_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee.defer_cycle = ~0ull;   // The deferral record and its back-off.
    g_wsee.defer_objs.clear();
    g_wsee.hold.clear();
    // The per-mesh run cache (wsee_hold_drop clears it too, at the next session's first clear) and the plan scratch
    // (wsee_defer_group clears it before each plan); the reset leaves nothing to that frame.
    g_wsee_spans.clear();
    g_wsee_run_plan.clear();
    g_wsee.hold_used = 0u;
    g_wsee.hold_cycle = ~0ull;
    g_wsee.hold_copy_cycle = ~0ull;   // WSEE_HOLD_IDLE_COPY.
    g_wsee.defer_taken = ~0ull;
    g_wsee.defer_drawn = ~0ull;
    g_wsee.defer_back = 0;
    g_wsee_opq_cycle = ~0ull;
    g_rp.withheld_seq = ~0ull;
    g_rp_rec.front_seq = ~0ull;
    g_rp_rec.eng_seq = ~0ull;
    g_rp_rec.vol_clear_seq = ~0ull;
    g_rp.seam_b = false;
    g_rp_rec.c = RpCounters();
    g_gid_ok = false;   // GTS_IDENTITY: relearned in the new session.
    g_gid_k = 0;
    g_gid_breaks = 0;
    g_rtp_cyc = ~0ull;   // RT_PAINT_READ.
    g_rtp_read_ser = 0;   // RT_READ_SER.
    g_rtp_c = RtpCounters();
    g_gts.clear_qpc = 0;
    g_gts.clear_prev = 0;
    g_gts.half = 0;
    g_gts.final_cyc = ~0ull;
    g_gts.learn_cyc = ~0ull;
    g_gts.first_rd_cyc = ~0ull;
    g_gts.crew_n.store(0, std::memory_order_relaxed);   // CREW_LAG.
    g_gts.first_cyc = ~0ull;      // CAST_REPICK.
    g_gts.first_done = 0;
    g_attach_repick_step = false;
    g_attach_repick_moved = false;
    g_sun_restep_cycle = ~0ull;
    g_rtshadow_cycle.store(~0ull, std::memory_order_relaxed);
    g_dls_frame_cycle = ~0ull;
    g_dls_world_cycle = 0;
    g_fire.post_scene_cycle = ~0ull;
    g_ring_best.cycle = 0;
    g_ring_best.valid = false;
    g_view_drop.relock_wide_cycle = 0;
    g_inj_pub.dp_valid = false;
    g_inj_pub.dp[0] = g_inj_pub.dp[1] = g_inj_pub.dp[2] = g_inj_pub.dp[3] = 0.0f;
    for (int r = 0; r < 8; ++r) g_band_tab_far[r] = -1.0f;
    g_inj_pub.enc_m22 = 0.0f; g_inj_pub.enc_m32 = 0.0f; g_inj_pub.enc_near = -1.0f; g_inj_pub.enc_ms = 0;
    g_seam_take.pred_p1 = -1.0f; g_seam_take.pred_p2 = -1.0f; g_seam_take.pred_m22 = 0.0f;
    g_seam_take.pred_ms = 0; g_seam_take.pred_upd_ms = 0; g_seam_take.pred_skip1 = -1.0f;
    g_seam_take.base_stable = -1.0f; g_seam_take.base_prev = -1.0f;
    g_seam_take.live_prev_ok = false; g_seam_take.live_prev_step = 0.0f;
    g_seam_take.live_step_ring[0] = g_seam_take.live_step_ring[1] = 0.0f;
    g_seam_take.live_step_ring[2] = g_seam_take.live_step_ring[3] = 0.0f;
    g_seam_take.live_step_ring_i = 0;
    g_seam_take.live_lp_ok = false;
    g_seam_take.live_ref_ok = false;
}

// VOL_REPLAY, MIR_REPLAY, SEAM_WITHHOLD, GTS_IDENTITY, RT_PAINT_READ: every namespace-scope global of
// these features back to a fresh process's value (SESSION CLEAN SLATE). Idempotent: the GPU objects are released
// first (normally already done by release_shadow_device_state), so it is right in either teardown order.
inline void rp_globals_reset() {
    rp_release_all();
    g_rp.merge_cyc = ~0ull;
    g_rp.replay_cyc = ~0ull;   // REPLAY_ONCE.
    g_rp.mir_cyc = ~0ull;   // MIR_REPLAY (its targets and g_rp.mir_ok go with rp_release_all).
    g_rp.mir_want = false;
    g_rp.mirb_drawn = false;
    g_rp.mir_pend = false;
    g_rp.mir_pend_cyc = g_rp.mir_pend_pass = ~0ull;
    g_rp.sc_all = RpRect();   // REPLAY_SCISSOR.
    g_rp.sc_mir = RpRect();
    g_rp.sc_gen = ~0ull;
    g_rp.pass_vol = g_rp.pass_mir = false;
    g_rp.rast_on = false;   // REPLAY_RASTER.
    reinit(g_rp.rast);
    g_rp.seam_b = false;
    g_rp.withheld_seq = ~0ull;
    g_rp_rec.front_idx = 0;
    g_rp_rec.front_vp = D3D11_VIEWPORT{};
    g_rp_rec.c = RpCounters();
    g_gid_k = 0;
    g_gid_ok = false;
    g_gid_breaks = 0;
    g_rtp_c = RtpCounters();
    g_rtp_cyc = ~0ull;
    g_rtp_read_ser = 0;
    psv_device_lost();   // PSV: the variants (g_res.release ran it already: idempotent).
}
// SESSION_RESET - every namespace-scope global of the renderer's three headers back to its declared initial value, i.e.
// to what a fresh process holds: scalars and arrays to their initialisers, atomics stored, containers and structs
// rebuilt in place (reinit, which also frees their memory). Generated from the declarations, one line each (or one per
// feature struct, below) in the undivided header's declaration order; a global added to any of them gets a line here,
// or in session_objects_reset below when the objects of a live mission hold what it names. Except the cycle stamps and
// per-session state session_scratch_reset sets (g_wsee.* and the other g_wsee_* stamps, g_psb.* with its psa_* cycles,
// g_vmsee.*, g_vmir.want_cycle, g_nzm_cycle, the g_gts.* clock and learning state, g_attach_repick_*): they have no
// line here - that reset runs after this one in reset_session_state - and a global added to those families gets its
// line there. A feature struct (g_svs, g_fire, g_castocc, ...) whose every member started over here at its declared
// value is one line, reinit(<instance>): a member added to it is reset with no line of its own (one with another reset
// - an atomic's store, a keep - stays a per-member line, as in the structs reset member by member). Not reset, by
// design: synchronisation objects and thread records; the hook trampolines and install records (the code patches stay
// in place for the process, pass-through while disarmed); the shader pool (g_shpool_* / g_shbatch_*: a detached worker
// may still read them - its own shutdown owns them) and its failure latch (g_shcache_fail: shader_mt_shutdown clears
// it); the Draw3D / UI event-handler handles and the draw list (their own teardown, first); the attachment table and
// the proxy pool with its key index (g_attach, g_pxy_pool, g_pxy_pool_key: never destructed, emptied with the draw list
// at both mission edges by attach_drop_all); the crew table (g_crew, g_crew_notman and its counters g_crew_serial /
// g_crew_frame: never destructed, emptied at both mission edges by crew_clear_all); g_mesh_publish_pending and
// g_mesh_cache_later_q (mesh_release_session); the material pool (material_pool_reset keeps the pinned default); the
// cloth pool's records (cloth_workers_stop); the read-memo epoch (bumped, never reset); the presence memo's epoch
// (g_dlcap.pres_epoch, DL_PRES_MEMO: bumped, never reset - the session reset empties its table); the UI lane's verdict
// serial (g_fx_pair_guard.ui_serial, FX_PAIR_HOLD: only its uniqueness is used) and the shader fail latch's epoch
// (g_shcache_fail_epoch, SHADER_FAIL_LATCH: bumped, never reset); the map redirect (g_redir / g_redir_live / g_rbuf,
// MAP_REDIRECT: a mapping in flight must reach its Unmap, which frees its record; the cached buffers are the process's;
// g_wc_cached_src is raised and lowered around each capture); the profiler's TSC calibration (g_prof.cal_q /
// g_prof.cal_t / g_prof.tsc_per_us, KH_PROF_SUB: the machine's rate - the next armed fold measures it again over the
// gap) and its sampling state (g_prof.up_ctr / _on / _t, g_prof.ds_ctr / _on / _t: call counters, the bools taken again
// before every use - g_prof.up_cur, the step in flight, is reset); the GPU pass list's entries (g_gpu_pass_pub,
// GPU_PASS: read only below g_gpu_pass_pub_n, which the stats reset zeroes); the fast clock (g_fast_clock, FAST_CLOCK:
// a new clock origin re-anchors it); the want table's role bits (g_dlcap.keep_kind: written afresh with each new
// entry); the Eden flag's copy (g_attach_eden, ATTACH_EDEN: stored again at the next Draw3D or command read); the game
// thread's id (g_game_tid, RESET_THREAD: the process's, stamped by flush_frame, flush_ui_frame and ensure_draw_eh); the
// collider pose serial (g_physics_pose_serial: an identity - 32-bit, skipping 0, so it repeats only 2^32 poses on, and
// every reader compares a stamp it took from it with ==); the vertex-stage hint (g_uvs_sets, KH_USER_VS: the deferred
// teardown runs under the NEXT mission's objects, whose sets the parse has already counted - zeroed here it would
// switch their vertex stages off; left standing it costs a later mission without one a walk of the live scene a flush);
// the interned path strings (a name -> pointer table nothing resolves through) and effect texture sets
// (g_fx_texset_pool, FX_TEX: immutable records objects point at); the compiled proxy handlers; the per-thread draw
// handshakes (set and consumed inside one draw); the bind record (t_kh_bc, BIND_FILTER: per thread, believed only
// inside a pass's scope, forgotten at both its ends); the upload scratch (written before every read); the hook
// install's constant-buffer reuse verdict (g_rp_rec.cbreuse_on: set where the hooks are installed, as the install
// records); VM_SEE's re-issue saves (g_vmsee.sv_*: null outside one draw's vmsee_t_begin / _end pair) and WSEE's
// (g_wsee_sv.*: likewise, its wsee_re_save / _restore pairs; g_wsee_pd_vp / _vpn: read only under g_wsee_pd_vp_ok,
// which the scratch reset clears); the PSV blob store, its books and serial (g_psv_store, g_psv_books, g_psv_ser: the
// engine's shaders outlive every mission - each blob goes at its shader's release, PSV_TOMB); g_ui_frame_rtv
// (ui_mask_reset, below, releases it). Runs first in reset_session_state, after every device object is released, so no
// COM pointer is dropped unreleased; the calls after it there (stats, scratch, UI mask, in-front frame, error dedupe,
// PIP, sun ring, dynamic lights) set the rest.
inline void session_globals_reset() {
    rp_globals_reset();   // VOL_REPLAY and its companions.
    g_stats_armed.store(false, std::memory_order_relaxed);
    for (auto& a : g_prof.ticks) a.store(0, std::memory_order_relaxed);
    for (auto& a : g_prof.count) a.store(0, std::memory_order_relaxed);
    for (auto& a : g_prof.us_pub) a.store(0, std::memory_order_relaxed);
    for (auto& a : g_prof.n_pub) a.store(0, std::memory_order_relaxed);
    for (auto& a : g_prof.sample_ctr) a.store(0, std::memory_order_relaxed);   // PROF_SAMPLE.
    for (auto& a : g_prof.tsc) a.store(0, std::memory_order_relaxed);   // KH_PROF_SUB.
    for (auto& a : g_prof.xb_acc) a.store(0, std::memory_order_relaxed);   // PROF_UPLOAD.
    for (auto& a : g_fun_acc) a.store(0, std::memory_order_relaxed);   // PROF_FUNNEL.
    g_prof.up_cur = PROF_ZONE_N;
    g_prof.prev_on = false;   // PROF_WINDOW.
    g_prof.epoch.store(0, std::memory_order_relaxed);   // PROF_DISARM.
    g_prof.epoch_seen = 0;
    reinit(g_gpu_prof);
    for (auto& a : g_gpu_us_pub) a.store(0, std::memory_order_relaxed);
    g_cloth_q_n.store(0, std::memory_order_relaxed);
    g_skin_q_n.store(0, std::memory_order_relaxed);   // POOL_SPLIT.
    reinit(g_mat_grave);
    g_eds_memo_res = nullptr;
    g_eds_memo_w = 0;
    g_eds_memo_h = 0;
    reinit(g_res);
    reinit(g_ui_mask);
    g_ui_poison_run = 0;
    g_ui_mask_wanted.store(false, std::memory_order_relaxed);
    g_present.ui_ms.store(0, std::memory_order_relaxed);
    g_present.seq.store(0, std::memory_order_relaxed);
    reinit(g_present.rtv);
    reinit(g_present.rtv_id);
    g_present.rtv_next = 0;
    g_svs_mesh_wanted.store(false, std::memory_order_relaxed);
    g_fogsc_wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // DLF_DEMAND.
    g_flush_active.store(false, std::memory_order_relaxed);
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
    g_track_wanted.store(false, std::memory_order_relaxed);
    g_ui_frame_rtv_seq = 0;
    reinit(g_rain);   // RainLensState: every member to its declared start.
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    g_attach_repaired.store(0, std::memory_order_relaxed);
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_attach_final_step = false;
    g_sun_restep_cycle = ~0ull;
    g_flush_frame = 0;
    g_flush_frame_pub.store(0, std::memory_order_relaxed);
    reinit(g_stats);
    g_flush_serial.store(0, std::memory_order_relaxed);
    g_scene_depth_samples.store(0, std::memory_order_relaxed);
    g_main_depth_identity.store(nullptr, std::memory_order_relaxed);
    g_main_depth_w = 0;
    g_main_depth_h = 0;
    g_wrong_pass_streak = 0;
    g_flush_wrong_pass = false;
    g_rtshadow_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_park_cycle = ~0ull;
    g_flush_miss_seen_serial = 0;
    g_flush_landed_this_frame = false;
    g_scene_vp_min_d = 0.011f;
    g_scene_vp_max_d = 0.999f;
    g_cbr_supported = false;
    g_ctx1_null_first = false;
    g_cb_offsetting = false;
    g_scene_hold = false;   // SCENE_HOLD (scoped; false outside a flush).
    reinit(g_vis_memo_inj);
    reinit(g_vis_memo_flush);
    reinit(g_dist_memo_inj);
    reinit(g_dist_memo_flush);
    reinit(g_ord_keys);
    reinit(g_ord_tmp);
    reinit(g_cast_key_scr);   // SKIN_POOL: the pooled sorts' tie-break scratch.
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    g_shcache_trim_debt.store(CACHE_SHADER_TRIM_SLACK, std::memory_order_relaxed);   // CACHE_TRIM_THROTTLE.
    reinit(g_physics_bvh);
    g_physics_bvh_frame = 0;
    reinit(g_cloth);
    reinit(g_cloth_q);
    reinit(g_cloth_grave);
    g_cloth_skipped.store(0, std::memory_order_relaxed);
    g_simlod.seq.store(0, std::memory_order_relaxed);   // SIM_LOD: no view until the next boundary.
    for (auto& c : g_simlod.cam) c.store(0.0f, std::memory_order_relaxed);
    g_simlod.m11.store(0.0f, std::memory_order_relaxed);
    g_simlod.t.store(-1.0e30f, std::memory_order_relaxed);
    g_simlod.work.store(0, std::memory_order_relaxed);
    g_simlod.full.store(0, std::memory_order_relaxed);
    reinit(g_cloth_warned);
    g_cloth_shape_serial.store(0, std::memory_order_relaxed);
    reinit(g_skin_q);
    reinit(g_skin_col);
    g_skin_col_stamp = 0;
    g_skin_col_pub = 0;
    g_skin_col_lo = 0;
    g_skin_col_hi = 0;
    reinit(g_skin_col_snap);
    reinit(g_cloth_vb_slot);
    reinit(g_cloth_mesh_slot);
    reinit(g_cloth_gen_slot);
    reinit(g_cloth_sim_slot);   // LOD_SIM_SLOT.
    reinit(g_cloth_voff_slot);   // SKIN_POOL.
    reinit(g_cloth_pool_slot);
    reinit(g_cloth_objs_h);
    g_chain_skipped.store(0, std::memory_order_relaxed);
    reinit(g_physics_col);
    g_physics_col_n.store(0, std::memory_order_relaxed);
    reinit(g_cloth_objs);
    { const float v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_cloth_wind, v, sizeof(g_cloth_wind)); }
    g_cloth_dt_last = 0.0;
    reinit(g_skin);
    reinit(g_skin_rest);
    g_skin_gen_serial.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
    reinit(g_skin_gpu);
    reinit(g_skin_pool);   // SKIN_POOL.
    g_skin_pool_serial = 0u;
    g_skin_pool_failed = false;
    g_skin_pool_made = false;   // SKIN_POOL_MADE.
    g_skin_pool_any = false;
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // SKIN_GPU.
    g_skin_so_retry_at = ~0ull;   // SKIN_SO_RETRY.
    g_skin_so_big_failed = false;   // SKIN_SO_LAZY: the large class's latch.
    g_skin_so_big_retry_at = ~0ull;
    g_skin_so_n.store(0, std::memory_order_relaxed);
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // SKIN_CULL.
    g_skin_stat_cycle = ~0ull;
    g_skin_stat_gpu = 0u;
    g_skin_stat_cpu = 0u;
    g_skin_stat_cull = 0u;
    g_skin_cull_any = false;   // SKIN_CULL: the attachments' flags went with them.
    g_skin_cull_rad_lo = 0.0f;
    g_skin_cull_rad_hi = 0.0f;
    g_skin_cull_pip_cycle = ~0ull;
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);
    reinit(g_skin_rest_gpu);   // Its buffers went with skin_release_all.
    reinit(g_tex_pages);
    reinit(g_tex_cache);
    g_tex_warned.store(0, std::memory_order_relaxed);   // TEX_WARN_CAP.
    reinit(g_user_shaders);   // UserShaderState: every member to its declared start (KH_USER_VS, USER_REQ_IDLE).
    reinit(g_uvs);             // Its buffers went with uvs_release_all.
    g_uvs_key_serial = 0u;
    reinit(g_user_lut_cache);
    reinit(g_lod);   // LodScaleState: every member to its declared start (LOD_ZOOM, LOD_PARK_KEEP).
    reinit(g_meshwriter_q);
    g_meshwriter_running = false;
    g_meshwriter_abort.store(false, std::memory_order_relaxed);
    g_meshwriter_busy_id.store(-1, std::memory_order_relaxed);
    reinit(g_texldr_q);
    reinit(g_texldr_done);
    g_texldr_running = false;
    g_texldr_abort.store(false, std::memory_order_relaxed);
    g_vmir.vs = nullptr;
    g_ssgi_scale.store(0.5f, std::memory_order_relaxed);
    g_ssgi_w = 0;
    g_ssgi_h = 0;
    g_comp_fail_streak = 0;
    g_comp_next_retry = 0.0f;
    reinit(g_comp_last_err);
    g_sun_setup.sun_size_div.store(1, std::memory_order_relaxed);
    g_sun_setup.sun_map_size = SUN_DEPTH_BASE;
    g_sun_setup.sun2_map_size = SUN_HERO_BASE;
    g_sun_setup.sun3_map_size = SUN_MID_BASE;
    g_sun_setup.sun4_map_size = SUN_OUT_BASE;
    g_sun_setup.sun5_map_size = SUN_FAR_BASE;
    g_sun_range.store(200.0f, std::memory_order_relaxed);
    g_obj_vis.store(0.0f, std::memory_order_relaxed);
    g_sun_setup.sun_range_src.store(0, std::memory_order_relaxed);
    g_shadows_off.store(false, std::memory_order_relaxed);   // SHADOW_OFF.
    g_sun_setup.sun_pf_autogen = false;
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_composite_last_inject_ms.store(0, std::memory_order_relaxed);
    g_composite_inject_serial.store(0, std::memory_order_relaxed);
    g_composite_land_cycle = ~0ull;
    reinit(g_last_pv);
    g_last_pv_ms = 0;
    g_proj_locator_ever = false;
    reinit(g_fx_pair_guard.pair);
    g_fx_pair_guard.pair_near = -1.0f;
    g_fx_pair_guard.cand_near = -1.0f;
    g_fx_pair_guard.pair_ms = 0;
    g_fx_pair_guard.verdict_serial = ~0ull;
    g_fx_pair_guard.verdict_cycle = ~0ull;   // FX_PAIR_HOLD.
    g_fx_pair_guard.verdict_held = false;
    reinit(g_fx_pair_guard.in_pair);   // FX_PAIR_KEY.
    g_vm_slice_live.store(false, std::memory_order_relaxed);
    g_vm_slice_known.store(false, std::memory_order_relaxed);   // SHOW_INFRONT.
    g_infront.watch_cycle = false;
    g_snap.serial = 0;
    g_snap.ms = 0;
    reinit(g_inj_pub);   // InjPublishState: every member to its declared start.
    { const float v[2] = { 0.0f, 0.0f }; memcpy(g_snap.pair, v, sizeof(g_snap.pair)); }
    g_snap.pair_src = 0;
    { const float v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_snap.cam, v, sizeof(g_snap.cam)); }
    reinit(g_snap.vp);
    g_snap.vp_lo = 0.011f;
    g_snap.vp_hi = 0.999f;
    g_snap.vp_valid = false;
    reinit(g_thm.data);
    g_thm.w = 0;
    g_thm.h = 0;
    reinit(g_thm.origin);
    g_thm.cell = 0.0f;
    g_thm.split = THM_SPLIT_SMOOTH;   // THM_EXACT.
    g_thm.dirty.store(false, std::memory_order_relaxed);
    g_thm.valid.store(false, std::memory_order_relaxed);
    g_thm.reupload.store(false, std::memory_order_relaxed);   // THM_DEVRESET.
    reinit(g_thml);   // TerrainLayoutState: every member to its declared start (THM_EXACT).
    reinit(g_thm_auto);   // TerrainAutoState: every member to its declared start.
    g_thmf_dim = 0;
    g_thmf_cell = 0.0f;
    reinit(g_voc_thm);   // VocTerrainState: every member to its declared start.
    g_reorder_render_tid.store(0, std::memory_order_relaxed);
    g_watch.clears.store(0, std::memory_order_relaxed);   // WATCH_STALL (and the three below).
    g_watch.cyc_seen = ~0ull;
    g_watch.clears_seen = 0;
    g_watch.still = 0;
    reinit(g_ro);
    g_hook_exceptions.store(0, std::memory_order_relaxed);
    reinit(g_slot_keep);   // SlotKeepState: every member to its declared start.
    reinit(g_cbc);
    reinit(g_latch_cam);
    g_latch_cam_valid = false;
    g_cam_step_m = -1.0f;
    g_boundary_pv_valid = false;
    g_boundary_t = -1.0f;
    reinit(g_proj_locator);
    reinit(g_proj_pending);
    g_proj_last_m32 = 0.0f;
    reinit(g_upc);   // UPLOAD_LAZY.
    reinit(g_uwc);
    g_sun_valid = false;
    { const float v[3] = { 0.0f, 1.0f, 0.0f }; memcpy(g_sun_dir_engine, v, sizeof(g_sun_dir_engine)); }
    g_fog_staged_valid = false;
    { const float v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog_staged, v, sizeof(g_fog_staged)); }
    g_fog_valid = false;
    { const float v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog, v, sizeof(g_fog)); }
    g_sun_jump_pending = false;
    { const float v[3] = { 0, 0, 0 }; memcpy(g_skysun.ref, v, sizeof(g_skysun.ref)); }
    g_skysun.ref_valid = false;
    g_skysun.ref_ms = 0;
    reinit(g_skysun.mot_ref);
    g_skysun.mot_ref_ms = 0;
    g_skysun.mot_deg = -1.0f;
    g_atlas_frame_cur = false;
    g_atlas_frame_prev = false;
    g_pub_valid = false;
    reinit(g_pub_dir);
    g_pub_block_valid = false;
    { const float v[8] = { -1,-1,-1,-1,-1,-1,-1,-1 }; memcpy(g_band_tab_far, v, sizeof(g_band_tab_far)); }
    reinit(g_pub_block_amb);
    reinit(g_pub_block_sun);
    reinit(g_lite_cand);   // LiteCandState: every member to its declared start.
    reinit(g_blko);   // BlockOwnerState: every member to its declared start (DLSW_GEN_AMB).
    reinit(g_sraw);   // SunRawState: every member to its declared start.
    reinit(g_ls);
    g_ls_cam_cycle = ~0ull;
    reinit(g_light_probe);
    reinit(g_sky_probe);
    reinit(g_skybind);
    reinit(g_ring_best);   // RingBestState: every member to its declared start.
    g_acq_hold.store(false, std::memory_order_relaxed);
    g_pub_first = -1.0f;
    g_prewarm_cand_res = nullptr;
    g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr;
    g_coldlock_cand_off = 0;
    g_cc_flush_serial.store(0, std::memory_order_relaxed);
    g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_ms_flush_serial = 0;
    g_carry_pending_serial.store(0, std::memory_order_relaxed);
    g_rescue_frame_count = 0;
    g_ctx1 = nullptr;
    g_ctx1_for = nullptr;
    reinit(g_rt0_desc);
    g_rt0_desc_n = 0;
    g_rt0_desc_next = 0;
    g_shadow_map_strength = 1.0f;
    g_shadow_map_bias = 0.0015f;
    g_shadow_map_sign = 1.0f;
    g_sun_maps.sun_map_rendered_frame = false;
    reinit(g_sun_gen);   // SunGenState: every member to its declared start (SUN_GEN, SUN_GEN_CAP).
    g_sun_maps.sun_map_valid = false;
    g_sun_setup.sun_map_cam_anchor = false;
    g_sun_maps.sun_map_render_time = -1.0f;
    g_sun_maps.sun_mat_chash = 0;
    reinit(g_sun_maps.sun_mat_sun);
    g_sun_maps.sun_map_time = -1.0f;
    reinit(g_sun_setup.sun_map_vp);
    reinit(g_sun_maps.sun_anchor_now);
    reinit(g_sun_setup.sun_cam_now);   // DLS_LOD_CAM.
    reinit(g_sun_setup.sun_map_anchor);
    reinit(g_sun_setup.sun_tier_anchor);
    reinit(g_sun_setup.sun_tier_lat);   // PCSS_CELL.
    g_vmir.w = 0;
    g_vmir.h = 0;
    g_vmir.mask_time = -1.0f;
    g_sun_setup.sun_map_bias = 0.0f;
    g_sun_maps.sun_map_cast_bias = 0.0f;
    reinit(g_sun_setup.sun2_map_vp);
    g_sun_setup.sun2_map_bias = 0.0f;
    g_sun_maps.sun2_cast_bias = 0.0f;
    g_sun_setup.sun2_half_diag = 0.0f;
    g_sun_maps.sun2_map_valid = false;
    g_sun_setup.sun2_renders = 0;
    reinit(g_sun_setup.sun3_map_vp);
    g_sun_setup.sun3_map_bias = 0.0f;
    g_sun_maps.sun3_cast_bias = 0.0f;
    g_sun_setup.sun3_half_diag = 0.0f;
    g_sun_maps.sun3_map_valid = false;
    g_sun_setup.sun3_renders = 0;
    reinit(g_sun_setup.sun4_map_vp);
    g_sun_setup.sun4_map_bias = 0.0f;
    g_sun_maps.sun4_cast_bias = 0.0f;
    g_sun_setup.sun4_half_diag = 0.0f;
    g_sun_maps.sun4_map_valid = false;
    g_sun_setup.sun4_renders = 0;
    reinit(g_sun_setup.sun5_map_vp);
    g_sun_setup.sun5_map_bias = 0.0f;
    g_sun_maps.sun5_cast_bias = 0.0f;
    g_sun_setup.sun5_half_diag = 0.0f;
    g_sun_maps.sun5_map_valid = false;
    g_sun_setup.sun5_renders = 0;
    { const bool v[4] = { false, false, false, false };
        memcpy(g_sun_maps.sun_pf_valid, v, sizeof(g_sun_maps.sun_pf_valid)); }
    { const bool v[4] = { false, false, false, false };
        memcpy(g_sun_setup.sun_pf_fresh, v, sizeof(g_sun_setup.sun_pf_fresh)); }
    { const bool v[4] = { false, false, false, false };
        memcpy(g_sun_setup.sun_pf_held, v, sizeof(g_sun_setup.sun_pf_held)); }
    { const uint64_t v[4] = { 0, 0, 0, 0 };
        memcpy(g_sun_setup.sun_tier_key, v, sizeof(g_sun_setup.sun_tier_key)); }
    { const bool v[4] = { false, false, false, false };
        memcpy(g_sun_setup.sun_tier_key_ok, v, sizeof(g_sun_setup.sun_tier_key_ok)); }
    g_sun_setup.sun_pf_done = ~0ull;
    { const float v[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
        memcpy(g_sun_setup.sun_fwd_held, v, sizeof(g_sun_setup.sun_fwd_held)); }
    g_sun_setup.sun_union_rad_held = -1.0f;
    g_sun_setup.sun_union_lat_held = -1.0f;
    g_sun_maps.sun_map_hash = 0;
    reinit(g_sun_maps.sun_map_bounds);
    reinit(g_fire);   // FireState: every member to its declared start.
    g_lock_churn_ms_v = 0;
    g_cast_ready = false;
    g_cast_ready_since_ms = 0;
    g_cascbind_feed_t = -1.0f;
    reinit(g_view_drop);   // ViewDropState: every member to its declared start.
    reinit(g_band_pool);
    g_band_fall_flush = ~0ull;
    g_recv_health_ok = false;
    g_recv_health_since_ms = 0;
    reinit(g_sun_maps.sun_locals);
    g_sun_maps.sun_local_count = 0;
    g_sun_maps.sun_locals_serial = 0;
    g_mask_cast_arm = false;
    g_cast_map_frame = ~0ull;   // CAST_FRAME64.
    g_mission_destroy_pending.store(false, std::memory_order_relaxed);
    g_topo_scene_tex_id = nullptr;
    g_topo_scene_seen_cycle = 0;   // SCENE_NAME.
    g_mask_cast_fired = false;
    g_dlcap.pool_next_id = 1;
    g_dl_intensity_bits.store(0x3F800000u, std::memory_order_relaxed);
    reinit(g_cbs);
    g_dlcap.cbs_gen = 0;
    g_dlcap.cbs_wseq = 0;   // DL_REC_ORDER: with the pool (reinit(g_dl), next), whose records it orders.
    reinit(g_dl);
    g_dlcap.harvest_cycle = ~0ull;
    g_dlcap.harvest_flush_last = false;
    reinit(g_dlcap.phix);   // DL_PRES_HIX: the pool's height index (with g_dl, above).
    reinit(g_dls);
    g_dls_n = 0;
    g_dls_frame = 0;
    g_dls_sel_ms = 0;   // DLS_POOL_AGE.
    g_dls_allowed.store(false, std::memory_order_relaxed);   // DLS_ALLOW: off.
    reinit(g_dls_keys);
    reinit(g_dlr);   // DlRingState: every member to its declared start (DL_RING).
    g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
    g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
    reinit(g_sr);
    reinit(g_cascbind);
    g_cascharv_ctr = 0;
    reinit(g_mask);
    reinit(g_dls_render);   // DlsRenderState: every member to its declared start.
    g_dls_map_valid = 0;
    reinit(g_dlsw_casters);
    g_dls_frame_cycle = ~0ull;
    reinit(g_castocc);   // CastOccState: every member to its declared start (CASTOCC_RANGE).
    reinit(g_svs);   // SeamState: every member to its declared start (VOL_WITNESS, VOL_ZOOM, VOL_FOOT).
    g_depth_clear_serial = 0;
    reinit(g_seam_take);   // SeamTakeState: every member to its declared start.
    reinit(g_engcam);   // EngCamState: every member to its declared start.
    g_vmir.tex = nullptr;
    g_vmir.dsv = nullptr;
    g_vmir.srv = nullptr;
    g_vmir.b2_src = nullptr;
    g_vmir.b2_src_srv = nullptr;
    g_vmir.b2_out = nullptr;
    g_vmir.b2_out_rtv = nullptr;
    g_vmir.b2_patch = nullptr;
    g_vmir.b2_vs = nullptr;
    g_vmir.b2_ps = nullptr;
    g_vmir.b2_failed = false;
    g_vmir.pending = false;
    g_vmir.frame_ok = false;
    g_vmir.mark2 = 0;
    g_vmir.prepass_stamp = 0;
    reinit(g_vmir.rs_src);
    reinit(g_vmir.rs_clamp);
    g_vmir.sv_rs = nullptr;
    g_vmir.prepass_src = nullptr;
    reinit(g_b2_snap);
    g_b2_learn = nullptr;
    reinit(g_vmir.sv_rtvs);
    g_vmir.sv_dsv = nullptr;
    reinit(g_vmir.sv_vps);
    g_vmir.sv_nvp = 0;
    g_vmir.sv_b2 = nullptr;
    g_vmir.sv_b2_first = 0;
    g_vmir.sv_b2_num = 0;
    g_vmir.sv_b2_off_ok = false;
    g_pip_seen = false;
    reinit(g_pip);
    g_infront.wanted.store(false, std::memory_order_relaxed);
    g_infront.watch_wanted.store(false, std::memory_order_relaxed);   // SHOW_INFRONT.
    g_infront.prev_near = false;
    g_infront.pre_done = false;
    g_infront.col_done = false;
    g_infront.pre_injected = false;   // INFRONT_PRE_COL.
    g_infront.seam_done = false;
    g_infront.frame_valid = false;
    g_infront.frame_cycle = 0;
    reinit(g_infront.view_proj);
    reinit(g_infront.proj);   // MIR_ZOOM.
    reinit(g_infront.cam);
    g_infront.rebase = 0.0f;
    g_infront.m22 = 0.0f;
    g_infront.m32 = 0.0f;
    g_infront.keep_m22 = 0.0f;
    g_infront.keep_m32 = 0.0f;
    g_infront.keep_ms = 0;
    reinit(g_infront.meshes);
    infront_tail_release();   // INFRONT: the deferred tail's held pair (released; the viewport is scratch).
    reinit(g_infront.tail_vp);
    g_inj_arb_prev_armed = false;
    g_dls_world_cycle = 0;
    g_fx_late.owed_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late.ran_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late.dry_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late.streak.store(0, std::memory_order_relaxed);
    g_fx_late.backoff.store(0, std::memory_order_relaxed);
    g_fx_late.miss_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late.miss_cycle2.store(~0ull, std::memory_order_relaxed);
    g_depth_readopt_run = 0;
    reinit(g_hdc_blend);
    reinit(g_hdc_dss);
    g_flush_fx_arb_prev = false;
    g_video_opt_ms = 0;
    g_dlcap.cell.store(0.0f, std::memory_order_relaxed);   // DL_ORIGIN_GRID: the next session reads its terrain.
    g_dlcap.q_cells = 0;
    g_dlcap.stall_a = 0;   // DL_STALL_GAP.
    g_dlcap.stall_b = 0;
    g_ui_rehoist_auto_s = -1.0e9f;
    g_ui_prev_scene_frame = 0;
    g_ui_prev_from_present = false;   // UI_ORDINAL_SRC.
    g_ui_rehoist_last_s = -1.0e9f;
}

// SESSION_RESET - the globals the objects of a mission hold on to (scene slots, handles and sequence numbers,
// attachment and helper-pool books, affectors, cloth proxies and chain settings, the event-handler flags, the mesh
// path keys and reference times, the material table, the effect clock), to their declared start the same way. Only
// when no object exists (session_teardown's clean path): the deferred destroy runs under the next mission's
// objects, which these belong to.
inline void session_objects_reset() {
    // CACHE_SESSION_KEEP: the disk cache's files read or written from here on are the next session's own.
    g_meshcache_session_t0.store(std::filesystem::file_time_type::clock::now().time_since_epoch().count(),
                                 std::memory_order_relaxed);
    reinit(g_scene);
    g_scene_unpub_min = 0xFFFFFFFFu;   // LOD_BOUNDS.
    reinit(g_scene_stage);
    reinit(g_scene_handle);
    reinit(g_scene_dirty_mark);
    reinit(g_scene_dirty);
    g_scene_dirty_n.store(0u, std::memory_order_relaxed);   // SYNC_IDLE.
    reinit(g_scene_free);
    reinit(g_objbuf_cpu);
    reinit(g_objbuf_dirty);
    reinit(g_objbuf_dirty_mark);
    g_objbuf_shoff = false;   // SHADOW_OFF.
    g_gts_seq = 0;   // ATTACH_GT_SNAP: the bindings' sample numbers (their rings, picks and helper stamps).
    g_skel_gen_serial = 0;
    g_attach_n.store(0, std::memory_order_relaxed);
    reinit(g_attach_dead);
    g_attach_dead_n.store(0, std::memory_order_relaxed);
    reinit(g_attach_proxy_dead);
    g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
    g_pxy_pool_n.store(0, std::memory_order_relaxed);
    g_gts.done_pub.store(0, std::memory_order_relaxed);   // Names a g_gts_seq value: restarts with it.
    g_gts.done_defer = 0;   // CHAIN_END_DRAW.
    for (auto& st : g_gts.stamp_pub) st.store(0, std::memory_order_relaxed);
    g_next_seq = 0;
    reinit(g_chain_cfg);
    reinit(g_chain_end);   // CHAIN_END_DRAW (reset_retained_state has emptied it).
    g_chain_end_n.store(0, std::memory_order_relaxed);
    reinit(g_chain_warned);
    reinit(g_chain_nobuild);
    g_ground_ready = false;
    g_ground_tried = false;
    g_ground_cell = 0.0f;
    g_ground_n = 0;
    { const uint8_t v[4] = { 2, 2, 2, 2 }; memcpy(g_ground_split, v, sizeof(g_ground_split)); }
    reinit(g_affectors);
    g_affectors_n.store(0, std::memory_order_relaxed);
    g_aff_next_seq = 0;
    reinit(g_aff_frame);
    g_tex_page_gen = 0;
    g_tex_land_serial = 0;   // TEX_LAND.
    reinit(g_fbx_cache);
    reinit(g_mesh_gen_cache);   // MESH_GEN_SHARE.
    reinit(g_mesh_last_ref);
    reinit(g_ref_note_cen);   // REF_NOTE_ONCE.
    reinit(g_mat_note_cen);   // MAT_NOTE_ONCE.
    g_ref_note_cen_ms.store(0, std::memory_order_relaxed);   // GC_NOTE_SKIP.
    reinit(g_ref_note_gc);
    reinit(g_mat_gpu.cpu);
    g_mat_gpu.dirty = false;
    g_mat_gpu.page_gen = 0;
    reinit(g_mat_anim);   // UV_ANIM: the list, its two cycle stamps and the cycle's time.
    g_mat_anim_done = ~0ull;
    g_mat_anim_tcyc = ~0ull;
    g_mat_anim_t = 0.0;
    g_effect_t0.store(0, std::memory_order_relaxed);
    reinit(g_cloth_proxy);
    reinit(g_cloth_proxy_want);
    g_cloth_proxy_frame = 0;
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_draw3d_eh_active = false;
}

// Device objects and locator/probe state go through release_shadow_device_state + g_res.release (the caller runs
// those first, under the graphics lock); this resets what those don't. Must run with the render thread parked:
// g_ro, the mask/live structs and the hoisted memories are render-thread state.
inline void reset_session_state() {
    session_globals_reset();   // SESSION_RESET: everything to its declared start; the calls below add the rest.
    reset_stat_counters();
    session_scratch_reset();
    ui_mask_reset();   // Learned backbuffers, phase machine, demand flag.
    infront_frame_reset();

    {   // Error dedupe the session (a fresh session re-reports).
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        g_err_once.clear();
        g_err_once_capped = false;   // ERR_ONCE_CAP.
    }
    {   // EA_LOG_ONCE: the warnings' dedupe, likewise.
        std::lock_guard<std::mutex> g(g_log_once_mx);
        g_log_once.clear();
        g_log_once_capped = false;
    }

    pip_reset();   // PIP.
    sraw_ring_reset();   // SRAW_SETTLE.
    dynlights_reset_session();
}

inline void reset_retained_state() {
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& kv : g_draw_list) scene_stage_remove(kv.second);   // SCENE: deaths sync later.
        attach_drop_all(false);   // ATTACH: a mission edge must not hold an object to process exit.
        g_draw_list.clear();
        g_next_seq = 0;   // Creation order restarts with the list it orders.
    }
    crew_clear_all();   // CREW_LAG: outside the mutex (its entries' event handlers go).
    aff_clear();   // AFFECTOR: game thread, like the table itself.
    // CLOTH_PROXY: released, not deleted - the mission's end takes its own objects, and a mission edge must not
    // hold one (attach_drop_all).
    g_cloth_proxy.clear();
    g_cloth_proxy_want.clear();
    g_chain_cfg.clear();   // CHAIN: its proxies too are the mission's, released the same way.
    {   // CHAIN_END_DRAW: the render thread reads the table under the mutex.
        std::lock_guard<std::mutex> ce_g(g_draw_list_mutex);
        g_chain_end.clear();
        g_chain_end_n.store(0, std::memory_order_relaxed);
    }
    g_chain_warned.clear();
    g_chain_nobuild.clear();
    g_aff_frame.clear();
    g_aff_next_seq = 0;
    // CLOTH_GROUND: measured again on the next mission's matrix.
    g_ground_tried = false;
    g_ground_ready = false;
}

// SESSION_RESET - the whole teardown, under the park, for both mission-edge paths: the joinable workers first
// (texture loader, mesh cache writer, the cloth / skin pool - nothing may run on after its data is gone), then every
// device object and learned identity, the meshes (into the grave the resource release frees), the instances, the
// caches (textures, user .hlsl, user .cube, materials, asset paths), the shader pool (bounded wait - none for a
// user-shader batch, USER_END_NOWAIT - then detach: a job inside D3DCompile cannot be killed), and last the
// session state, which puts every other global back to what a fresh process holds. clean: no object exists
// (rendering_integration_reset, after mission_handles_reset). False for park_run's deferred destroy, which
// runs under the next mission's objects: what they hold (meshes, materials, scene slots, handles, the effect clock)
// is left to them.
inline void session_teardown(bool clean) {
    texldr_stop();   // TEX_ASYNC: joined, results dropped.
    mesh_cache_writer_stop();   // Joined, queued-but-unwritten entries dropped.
    cloth_workers_stop();   // Joined; its queues dropped.
    release_shadow_device_state();
    if (clean) mesh_release_session();   // Ahead of g_res.release, which frees the grave it fills.
    g_res.release();
    cloth_drop_all();   // CLOTH: the instances and their buffers.
    skin_drop_all();    // SKEL: likewise.
    uvs_release_all();  // KH_USER_VS: likewise (the records hold nothing a mission outlives).
    tex_cache_release();
    // USER_END_NOWAIT: a user-shader batch in flight (read before the release below clears g_user_shaders.batch_own;
    // only one batch is ever live, so its workers are all the batch's) is not waited for: detached at once. Its units
    // are this mission's requests, which the release drops, and a material's unit is the builtin mesh shader with the
    // file - tens of seconds of fxc - so the bounded wait would freeze the game for all of its 5 s at the mission's end
    // and the compile would detach anyway (an effect's unit, about a second, would end inside the wait; detached, it
    // ends during the next mission's start). A unit it finishes still lands on disk (its bytecode in the memo too), and
    // the next mission's first ensure holds every mesh on the white placeholder until it ends, as after a wait that ran
    // out.
    const bool user_batch = g_user_shaders.batch_own && g_shpool_active;
    user_shader_cache_release();
    user_lut_cache_release();
    if (clean) material_pool_reset();
    RenderAssetDiscovery::clear_cache();
    // SHADER_ORPHANS (g_shpool_orphans): a detached worker runs out its unit after the mission and exits.
    shader_mt_shutdown(!user_batch);
    g_vs_memo_epoch.fetch_add(1u, std::memory_order_relaxed);   // Every thread's read memo empties at its next read.
    if (clean) {
        session_objects_reset();
    } else {
        // ATTACH_CYCLE_RESTAMP: the next mission's bindings and chain ends stay (session_objects_reset is the
        // clean edge's), but reset_session_state below restarts g_topo_cycles - a kept stamp would match once, when
        // the new counter reached it, and hand that cycle a pick, a read or a stash from months before (the rule the
        // scratch reset keeps for the global latches). Under the park, as the step that writes them runs.
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& e : g_attach) {
            e.second.gts_cyc = ~0ull;
            e.second.rt_cyc = ~0ull;
            e.second.rt_chk_cyc = ~0ull;
            e.second.skel_smp_cycle = ~0ull;
            e.second.smp_cycle = ~0ull;
            e.second.skin_culled = false;   // SKIN_CULL: the next step decides afresh.
            e.second.cull_rest.reset();
            e.second.cull_a.clear();
        }
        g_skin_cull_any = false;
        for (auto& c : g_chain_end) c.second.rt_cyc = ~0ull;
    }
    reset_session_state();
}

// Both mission edges: the event-handler handles and the per-mission retained state go first.
inline void mission_handles_reset() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();
}

// The whole-system reset, both mission edges: the handles and the retained state, then session_teardown under
// the park (the prewarm refuses while any detached shader worker lives).
inline void rendering_integration_reset() {
    if (!render_on()) return;   // PLAYER_ONLY: nothing of the renderer ever started.
    psv_hook_ensure();   // PSV: the CreatePixelShader hook, if the extension's load could not install it.
    bool done = false;
    mission_handles_reset();

    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_track_wanted.store(false, std::memory_order_relaxed);

    for (int attempt = 0; attempt < 8; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) continue;
        session_teardown(true);   // Clean: mission_handles_reset emptied the draw list above.
        done = true;
        break;
    }

    // A mission that never uses the extension holds the state disarmed.
    if (!done) {
        // TEARDOWN_KEEP_SETTINGS: the deferred teardown keeps the script settings it finds (the next mission's),
        // so this mission's go back to their declared defaults here, on the game thread (atomics). The stats take
        // reset_stat_counters: disarmed and zeroed, the unlocked best effort reset_render_stats_sqf also falls back to.
        g_dls_allowed.store(false, std::memory_order_relaxed);
        g_ssgi_scale.store(0.5f, std::memory_order_relaxed);
        g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
        g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
        reset_stat_counters();
        g_mission_destroy_pending.store(true, std::memory_order_relaxed);
    }
}

// Waits up to ms for a joinable thread to exit, terminates it if it has not, then joins it (detaching it if the
// join throws).
inline void thread_stop_bounded(std::thread& t, DWORD ms) {
    if (!t.joinable()) return;
    HANDLE h = static_cast<HANDLE>(t.native_handle());
    if (h && WaitForSingleObject(h, ms) == WAIT_TIMEOUT) TerminateThread(h, 0);
    try { t.join(); } catch (...) { try { t.detach(); } catch (...) {} }
}

// True when anything of the render integration is running or exists: script objects (meshes, post FX, live or staged),
// the per-frame drivers, an installed hook or the reset hook, device resources, a shader compile in any form (the pool,
// the async batch, a user .hlsl request), a worker thread or its queued / finished jobs, a pending mesh publish or
// deferred destroy, the caches, the shadow and PIP state, the terrain heightfield. A session that never touched the
// extension reads false on every term and DllMain skips the detach, as it does for the other frameworks. No lock may be
// taken from DllMain, so every read is lock-free: the script-side containers' writer is the game thread, the caller;
// the worker queues (g_texldr_q / g_texldr_done, g_meshwriter_q, g_shpool_map) and g_tex_cache have other writers and
// are read racily. Each worker's has a term that reads true while that writer can run (g_texldr_running,
// g_meshwriter_running, g_shpool_live); the render thread's writes (g_tex_cache, g_shpool_map's erases - the hooks'
// draws or the shared Present's UI flush) need an object or a UI effect, so g_draw_list / g_res.initialized read true.
// The verdict does not hang on the racy reads.
inline bool rendering_integration_is_initialized() {
    return !g_draw_list.empty() || g_scene.alive_n > 0 || !g_affectors.empty() || !g_cloth_proxy.empty() ||
           g_draw3d_eh_active || g_ui_driver_registered ||
           g_reorder_hook.active.load(std::memory_order_acquire) || g_reset_hook_installed ||
           g_res.initialized ||
           g_shpool_active || g_shbatch_state != 0 || !g_shpool_thr.empty() ||
           g_shpool_live.load(std::memory_order_acquire) > 0 || !g_shpool_map.empty() ||
           g_user_shaders.batch_own || !g_user_shaders.req.empty() ||
           g_texldr_running || !g_texldr_q.empty() || !g_texldr_done.empty() ||
           g_meshwriter_running || !g_meshwriter_q.empty() ||
           !g_cloth_thr.empty() || !g_skin_thr.empty() ||   // POOL_SPLIT: either pool's threads.
           g_mesh_publish_pending.load(std::memory_order_relaxed) ||
           g_mission_destroy_pending.load(std::memory_order_relaxed) ||
           !g_tex_cache.empty() || !g_user_shaders.ps_cache.empty() || !g_user_lut_cache.empty() ||
           !g_mat_pool.empty() ||
           g_ls.wanted.load(std::memory_order_relaxed) || g_ls.atlas_tex != nullptr ||
           g_dlf_wanted.load(std::memory_order_relaxed) ||   // DLF_DEMAND.
           g_pip_seen || g_thm.valid || g_thm.dirty || g_thm_auto.state != 0 ||
           g_stats_armed.load(std::memory_order_relaxed);
}

// Process detach (DllMain, the FreeLibrary path only - lpReserved == nullptr, like every other framework's
// teardown). On the terminating path the OS has already killed our threads and nothing here may run: the five
// thread objects (loader, cache writer, shader pool, cloth pool, skin pool) are heap-leaked so no static destructor
// can std::terminate on a still-joinable thread; and the CRT still runs this DLL's static destructors after DllMain
// on that path, so every Intercept event-handler handle (whose destructor runs an SQF command) and every container
// of object references a quit without mission_ended leaves full is heap-leaked too (EXIT_STATICS). The plain
// code / game_value statics left (as framework.hpp's) only drop a reference there, as on every exit. That is what
// makes that path safe without a call.
//
// Unloading: our threads are alive and would return into unmapped code once the DLL goes. Each is asked to stop,
// waited for briefly and terminated - the shader pool's workers an earlier shutdown detached too, by the handles
// kept for them (g_shpool_orphans). Under the loader lock a worker that obeys cannot finish exiting (its thread detach
// needs that lock), so every wait runs its budget out and every worker is terminated; the stop request is what gets
// it out of our code first (the loader, the cache writer and the shader pool return before their next job,
// MESHWRITER_ABORT_STOP; the cloth and skin pools after the jobs already queued, below; a job inside D3DCompile or a
// large cache write cannot be interrupted). The framework's watchdog bounds the whole detach regardless. No graphics
// lock (the render thread may never park again) and no device release (a leaked COM reference at unload costs
// nothing; a context call could hang).
inline void rendering_integration_process_detach() {
    // Every hook is a pass-through from here and every demand gate reads off.
    g_psv_cap_on.store(false, std::memory_order_relaxed);   // PSV: the CreatePixelShader detour copies nothing.
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_track_wanted.store(false, std::memory_order_relaxed);
    g_ls.wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // DLF_DEMAND.
    g_dlcap.idle.store(0, std::memory_order_relaxed);   // DL_IDLE.
    g_shbatch_abort.store(true, std::memory_order_relaxed);
    g_meshwriter_abort.store(true, std::memory_order_relaxed);
    g_texldr_abort.store(true, std::memory_order_relaxed);
    g_cloth_stop.store(true, std::memory_order_relaxed);
    // The reset hook is a pointer into this DLL held by kh_rv_extension, a module the engine loaded on its own and
    // which outlives an unload of ours; cleared so a later device reset cannot call into unmapped code. The setter
    // forwards a bare function pointer, so null is a plain store.
    if (g_reset_hook_installed) {
        RVExtBridge::set_reset_hook(nullptr);
        g_reset_hook_installed = false;
    }
    g_texldr_cv.notify_all();
    g_meshwriter_cv.notify_all();
    g_shpool_cv.notify_all();
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // POOL_SPLIT.
    thread_stop_bounded(g_texldr_thr, 200);
    thread_stop_bounded(g_meshwriter_thr, 200);
    // The running flags without their mutexes: a worker terminated inside its critical section leaves the mutex
    // locked forever, and nothing starts a worker after this point.
    g_texldr_running = false;
    g_meshwriter_running = false;
    // The pool: one budget for every worker, not one per worker.
    const uint64_t t0 = steady_now_ms();
    for (std::thread& t : g_shpool_thr) {
        const uint64_t el = steady_now_ms() - t0;
        thread_stop_bounded(t, el < 200u ? static_cast<DWORD>(200u - el) : 0u);
    }
    g_shpool_thr.clear();
    // The workers an earlier shutdown detached, on what is left of the same budget (one inside D3DCompile cannot be
    // told to stop).
    for (const HANDLE h : g_shpool_orphans) {
        const uint64_t el = steady_now_ms() - t0;
        if (WaitForSingleObject(h, el < 200u ? static_cast<DWORD>(200u - el) : 0u) == WAIT_TIMEOUT) {
            TerminateThread(h, 0);
        }
        CloseHandle(h);
    }
    g_shpool_orphans.clear();
    g_shpool_active = false;
    g_shbatch_state = 0;
    // The cloth pool, the same way. A worker woken with the stop set drains its own pool's queue - a solver worker one
    // step per queued cloth, a skin worker one skinning per queued skeletal mesh (SKEL, POOL_SPLIT) - and
    // returns; nothing queues after this point.
    const uint64_t c0 = steady_now_ms();
    for (std::thread& t : g_cloth_thr) {
        const uint64_t el = steady_now_ms() - c0;
        thread_stop_bounded(t, el < 200u ? static_cast<DWORD>(200u - el) : 0u);
    }
    g_cloth_thr.clear();
    g_cloth_thr_n.store(0u, std::memory_order_relaxed);   // CLOTH_FORK.
    for (std::thread& t : g_skin_thr) {   // POOL_SPLIT: the skin pool, on the same budget.
        const uint64_t el = steady_now_ms() - c0;
        thread_stop_bounded(t, el < 200u ? static_cast<DWORD>(200u - el) : 0u);
    }
    g_skin_thr.clear();
    g_skin_thr_n.store(0u, std::memory_order_relaxed);
}
}