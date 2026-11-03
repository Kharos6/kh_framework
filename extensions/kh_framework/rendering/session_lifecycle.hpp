#pragma once

// The shared tail (rendering_integration.hpp): the session lifecycle: the stats and session resets, the teardown and
// the process detach. Never include it on its own.
namespace RenderIntegration {

// The stats reset touches statistics only - the counters and the shader-cache hit / miss pair - and disarms
// collection. Called from the game thread by resetRenderStats (under the graphics lock when its best-effort park
// gets it, without it when not), by rendering_integration_reset's lock-failure arm (unparked, the same case) and by
// the parked session reset, so nothing the render thread depends on may be written here; per-session live state is
// kh_session_scratch_reset below, which only the parked session reset reaches.
inline void reset_stat_counters() {
    g_stats_armed.store(false, std::memory_order_relaxed);
    g_stats = RenderStats{};
    g_parks = 0;   // KH_PARK_COUNT.
    g_ui_parks = 0;
    g_attach_repaired.store(0, std::memory_order_relaxed);   // KH_ATTACH_DIAG.
    g_rp_c = KhRpCounters();   // KH_VOL_REPLAY.
    kh_wsee_gput_zero_pub();   // KH_WSEE_GPUT: "wseeGpu" (frames in flight stay the clear's to read or drop).
    g_gid_breaks = 0;   // KH_GTS_IDENTITY.
    g_rtp_c = KhRtpCounters();   // KH_RT_PAINT_READ.
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    kh_prof_reset_pub();   // KH_PROF.
    g_prof_epoch.fetch_add(1u, std::memory_order_release);   // KH_PROF_DISARM: nothing measured before this is read.
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);   // KH_SKIN_GPU: skinBoundWorst.
    g_skin_so_n.store(0, std::memory_order_relaxed);   // skinGpu / skinCpu: republished by the next cycle's skinning.
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // KH_SKIN_CULL: skinCulled.
    g_cloth_skipped.store(0, std::memory_order_relaxed);   // clothSkipped / chainSkipped / skinSkipped.
    g_chain_skipped.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
}

// Per-session scratch that is not a device object; the replay, paint-read and identity counters it zeroes with the
// state they count are statistics reset_stat_counters zeroes too. Must run with the render thread parked.
inline void kh_session_scratch_reset() {
    g_castocc_set.clear(); g_castocc_slack = 0.0f;
    g_castocc_pre.active = false; g_castocc_pre_noreach = false;   // KH_CASTOCC_PRE.
    g_cam_step_m = -1.0f;
    g_fire_count_frame = 0;
    g_fire_clamp_painted = 0;
    g_svs_post_made = 0;
    g_sun5_renders = 0;   // (validity clears at the pass gates).
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    // A cycle-stamped latch restarts with the counter above: a stamp left by the previous session matches once,
    // when the new session's counter reaches it, and that cycle's once-per-cycle work is skipped.
    g_mat_anim_done = ~0ull;   // KH_UV_ANIM: the rewrite's and the time's stamps (the deferred edge keeps the list).
    g_mat_anim_tcyc = ~0ull;
    // KH_ATTACH_GT_SNAP: g_gts_seq and its publication are reset with the objects (kh_session_objects_reset).
    kh_rp_release_records();   // KH_VOL_REPLAY (the targets and copies go with the device).
    kh_rp_img_clear();   // KH_REPLAY_CPUCB.
    g_rp_pass = ~0ull;
    g_rp_ok = g_rp_ended = g_rp_replayed = false;
    g_rp_merge_ok = false;
    g_rp_mir_ok = false;   // KH_MIR_REPLAY.
    g_rp_mir_want = false;
    g_rp_mirb_drawn = false;
    g_rp_mir_pend = false;
    g_rp_mir_pend_cyc = g_rp_mir_pend_pass = ~0ull;
    g_rp_sc_all = KhRpRect();   // KH_REPLAY_SCISSOR.
    g_rp_sc_mir = KhRpRect();
    g_rp_sc_gen = ~0ull;
    g_rp_pass_vol = g_rp_pass_mir = false;
    g_rp_rast_on = false;   // KH_REPLAY_RASTER.
    g_vmir_want_cycle = ~0ull;   // KH_MIR_GATE: a cycle stamp.
    g_nzm_cycle.store(~0ull, std::memory_order_relaxed);   // KH_NEARZ_MARK: likewise.
    g_vmsee_want_cycle = ~0ull;   // KH_VM_SEE: likewise.
    g_vmsee_snap_cycle = ~0ull;
    g_vmsee_ok_cycle.store(~0ull, std::memory_order_relaxed);
    g_vmsee_pending = false;
    g_wsee_want_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE: likewise.
    g_wsee_col_want_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE_BWANT.
    g_wsee_col_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_ok_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_taint_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_used_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_snap_cycle = ~0ull;
    g_wsee_on = false;
    g_wsee_pending = KH_WSEE_RE_NONE;
    g_wsee_pending_post = false;
    g_wsee_pd_vp_ok = false;   // The pre-draw's kept viewports are no draw's.
    g_wsee_b2_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_b2_try = ~0ull;
    g_wsee_waste_n = 0;   // KH_WSEE: the back-off restarts with the cycle counter.
    g_wsee_backoff_end = 0;
    g_wsee_flap_prev = -1;   // KH_WSEE_FLAP.
    g_wsee_flap_win_start = 0;
    g_wsee_flap_n = 0;
    g_wsee_flushw_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE_FLUSHW_PRED.
    g_wsee_flushw_off.store(~0ull, std::memory_order_relaxed);
    g_wsee_flushw_probe = 0u;
    g_wsee_flushw_hold = false;
    g_wsee_opq_n = 0;   // The late-opaque cap's count.
    g_wsee_inj_elig = false;   // The capture gate's eligibility.
    g_wsee_cyc_layer = false;   // The cycle's layer-depth verdict.
    g_wsee_ld_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_ld_far_ok = false;
    g_wsee_ld_far_lo = 0.0f;   // The range D's raw was computed in (read under g_wsee_ld_far_ok only).
    g_wsee_ld_far_hi = 1.0f;
    g_psb_cycle.store(~0ull, std::memory_order_relaxed);   // KH_PSB: the one pass's record and the old one's stamp.
    g_psb_drew_cycle.store(~0ull, std::memory_order_relaxed);
    g_psb_zb_cycle.store(~0ull, std::memory_order_relaxed);   // KH_PSB_ZB: no Zb, its guard's state unknown.
    g_psb_zb_gen = 0u;
    g_psb_zb_far = true;
    g_psa_cycle.store(~0ull, std::memory_order_relaxed);
    g_psa_tp_cycle.store(~0ull, std::memory_order_relaxed);   // KH_PSB_TPA.
    g_psb_far_raw = 1.0f;
    g_wsee_od_cycle.store(~0ull, std::memory_order_relaxed);   // The cycle whose capture filled D3.
    g_wsee_d3_sync_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE_OQ.
    g_wsee_d3m_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE_OQ2.
    g_wsee_nopre_cycle.store(~0ull, std::memory_order_relaxed);   // KH_WSEE_NOPRE: the skipped and the S2 cycles.
    g_wsee_s2_cycle.store(~0ull, std::memory_order_relaxed);
    g_wsee_defer_cycle = ~0ull;   // The deferral record and its back-off.
    g_wsee_defer_objs.clear();
    g_wsee_hold.clear();
    // The per-mesh run cache (kh_wsee_hold_drop clears it too, at the next session's first clear) and the plan scratch
    // (kh_wsee_defer_group clears it before each plan); the reset leaves nothing to that frame.
    g_wsee_spans.clear();
    g_wsee_run_plan.clear();
    g_wsee_hold_used = 0u;
    g_wsee_hold_cycle = ~0ull;
    g_wsee_hold_copy_cycle = ~0ull;   // KH_WSEE_HOLD_IDLE_COPY.
    g_wsee_defer_taken = ~0ull;
    g_wsee_defer_drawn = ~0ull;
    g_wsee_defer_back = 0;
    g_wsee_opq_cycle = ~0ull;
    g_rp_withheld_seq = ~0ull;
    g_rp_front_seq = ~0ull;
    g_rp_eng_seq = ~0ull;
    g_rp_vol_clear_seq = ~0ull;
    g_rp_seam_b = false;
    g_rp_c = KhRpCounters();
    g_gid_ok = false;   // KH_GTS_IDENTITY: relearned in the new session.
    g_gid_k = 0;
    g_gid_breaks = 0;
    g_rtp_cyc = ~0ull;   // KH_RT_PAINT_READ.
    g_rtp_read_ser = 0;   // KH_RT_READ_SER.
    g_rtp_c = KhRtpCounters();
    g_gts_clear_qpc = 0;
    g_gts_clear_prev = 0;
    g_gts_half = 0;
    g_gts_final_cyc = ~0ull;
    g_gts_learn_cyc = ~0ull;
    g_gts_first_rd_cyc = ~0ull;
    g_gts_crew_n.store(0, std::memory_order_relaxed);   // KH_CREW_LAG.
    g_gts_first_cyc = ~0ull;      // KH_CAST_REPICK.
    g_gts_first_done = 0;
    g_attach_repick_step = false;
    g_attach_repick_moved = false;
    g_sun_restep_cycle = ~0ull;
    g_rtshadow_cycle.store(~0ull, std::memory_order_relaxed);
    g_dls_frame_cycle = ~0ull;
    g_dls_world_cycle = 0;
    g_fire_post_scene_cycle = ~0ull;
    g_ring_best_cycle = 0;
    g_ring_best_valid = false;
    g_view_relock_wide_cycle = 0;
    g_inj_dp_valid = false;
    g_inj_dp[0] = g_inj_dp[1] = g_inj_dp[2] = g_inj_dp[3] = 0.0f;
    for (int khbt_r = 0; khbt_r < 8; ++khbt_r) g_band_tab_far[khbt_r] = -1.0f;
    g_inj_enc_m22 = 0.0f; g_inj_enc_m32 = 0.0f; g_inj_enc_near = -1.0f; g_inj_enc_ms = 0;
    g_khpe_p1 = -1.0f; g_khpe_p2 = -1.0f; g_khpe_m22 = 0.0f;
    g_khpe_ms = 0; g_khpe_upd_ms = 0; g_khpe_skip1 = -1.0f;
    g_khbl_stable = -1.0f; g_khbl_prev = -1.0f;
    g_khlt_prev_ok = false; g_khlt_prev_step = 0.0f;
    g_khlt_step_ring[0] = g_khlt_step_ring[1] = 0.0f;
    g_khlt_step_ring[2] = g_khlt_step_ring[3] = 0.0f;
    g_khlt_step_ring_i = 0;
    g_khlt_lp_ok = false;
    g_khlt_ref_ok = false;
}

// KH_VOL_REPLAY, KH_MIR_REPLAY, KH_SEAM_WITHHOLD, KH_GTS_IDENTITY, KH_RT_PAINT_READ: every namespace-scope global of
// these features back to a fresh process's value (SESSION CLEAN SLATE). Idempotent: the GPU objects are released
// first (normally already done by release_shadow_device_state), so it is right in either teardown order.
inline void kh_rp_globals_reset() {
    kh_rp_release_all();
    g_rp_merge_cyc = ~0ull;
    g_rp_replay_cyc = ~0ull;   // KH_REPLAY_ONCE.
    g_rp_mir_cyc = ~0ull;   // KH_MIR_REPLAY (its targets and g_rp_mir_ok go with kh_rp_release_all).
    g_rp_mir_want = false;
    g_rp_mirb_drawn = false;
    g_rp_mir_pend = false;
    g_rp_mir_pend_cyc = g_rp_mir_pend_pass = ~0ull;
    g_rp_sc_all = KhRpRect();   // KH_REPLAY_SCISSOR.
    g_rp_sc_mir = KhRpRect();
    g_rp_sc_gen = ~0ull;
    g_rp_pass_vol = g_rp_pass_mir = false;
    g_rp_rast_on = false;   // KH_REPLAY_RASTER.
    kh_reinit(g_rp_rast);
    g_rp_seam_b = false;
    g_rp_withheld_seq = ~0ull;
    g_rp_front_idx = 0;
    g_rp_front_vp = D3D11_VIEWPORT{};
    g_rp_c = KhRpCounters();
    g_gid_k = 0;
    g_gid_ok = false;
    g_gid_breaks = 0;
    g_rtp_c = KhRtpCounters();
    g_rtp_cyc = ~0ull;
    g_rtp_read_ser = 0;
    kh_psv_device_lost();   // KH_PSV: the variants (g_res.release ran it already: idempotent).
}
// KH_SESSION_RESET - every namespace-scope global of the renderer's three headers back to its declared initial
// value, i.e. to what a fresh process holds: scalars and arrays to their initialisers, atomics stored, containers
// and structs rebuilt in place (kh_reinit, which also frees their memory). Generated from the declarations, one line
// each in the undivided header's declaration order; a global added to any of them gets a line here, or in
// kh_session_objects_reset below when the objects of a live mission hold what it names. Except the cycle stamps and
// per-session state kh_session_scratch_reset sets (g_wsee_*, g_psb_* / g_psa_*, g_vmsee_*, g_vmir_want_cycle,
// g_nzm_cycle, the g_gts_* clock and learning state, g_attach_repick_*): they have no line here - that reset runs
// after this one in reset_session_state - and a global added to those families gets its line there. Not reset, by
// design:
// synchronisation objects and thread records; the hook trampolines and install records (the code patches stay in
// place for the process, pass-through while disarmed); the shader pool (g_khsm_* / g_khsa_*: a detached worker may
// still read them - its own shutdown owns them) and its failure latch (g_khsc_fail: kh_shader_mt_shutdown clears
// it); the Draw3D / UI event-handler handles and the draw list (their own teardown, first); the attachment table and
// the proxy pool with its key index (g_attach, g_pxy_pool, g_pxy_pool_key: never destructed, emptied with the draw
// list at both mission edges by kh_attach_drop_all); the crew table (g_crew, g_crew_notman and its counters
// g_crew_serial / g_crew_frame: never destructed, emptied at both mission edges by kh_crew_clear_all);
// g_mesh_publish_pending and g_mesh_cache_later_q (kh_mesh_release_session); the material pool
// (kh_material_pool_reset keeps the pinned default); the cloth pool's records (kh_cloth_workers_stop); the read-memo
// epoch (bumped, never reset); the presence memo's epoch (g_dl_pres_epoch, KH_DL_PRES_MEMO: bumped, never reset -
// the session reset empties its table); the UI lane's verdict serial (g_khfx_ui_serial, KH_FX_PAIR_HOLD: only its
// uniqueness is used) and the shader fail latch's epoch (g_khsc_fail_epoch, KH_SHADER_FAIL_LATCH: bumped, never
// reset); the map redirect (g_redir / g_redir_live / g_rbuf, KH_MAP_REDIRECT: a mapping in flight must reach its
// Unmap, which frees its record; the cached buffers are the process's; g_wc_cached_src is raised and lowered around
// each capture); the profiler's TSC calibration (g_prof_cal_q / g_prof_cal_t / g_prof_tsc_per_us, KH_PROF_SUB: the
// machine's rate - the next armed fold measures it again over the gap) and its sampling state (g_prof_up_ctr / _on /
// _t, g_prof_ds_ctr / _on / _t: call counters, the bools taken again before every use - g_prof_up_cur, the step in
// flight, is reset); the GPU pass list's entries (g_gpu_pass_pub, KH_GPU_PASS: read only below g_gpu_pass_pub_n,
// which the stats reset zeroes); the fast clock (g_fast_clock, KH_FAST_CLOCK: a new clock origin re-anchors it); the
// want table's role bits (g_dl_keep_kind: written afresh with each new entry); the Eden flag's copy (g_attach_eden,
// KH_ATTACH_EDEN: stored again at the next Draw3D or command read); the game thread's id (g_game_tid,
// KH_RESET_THREAD: the process's, stamped by flush_frame, flush_ui_frame and ensure_draw_eh); the collider pose
// serial (g_physics_pose_serial: an identity - 32-bit, skipping 0, so it repeats only 2^32 poses on, and every
// reader compares a stamp it took from it with ==); the vertex-stage hint (g_uvs_sets, KH_USER_VS: the deferred
// teardown runs under the NEXT mission's objects, whose sets the parse has already counted - zeroed here it would
// switch their vertex stages off; left standing it costs a later mission without one a walk of the live scene a
// flush); the interned path strings (a name -> pointer table nothing resolves through) and effect texture sets
// (g_fx_texset_pool, KH_FX_TEX: immutable records objects point at); the compiled proxy handlers; the per-thread
// draw handshakes (set and consumed inside one draw); the bind record (t_kh_bc, KH_BIND_FILTER: per thread,
// believed only inside a pass's scope, forgotten at both its ends); the upload scratch (written before every read);
// the hook install's constant-buffer reuse verdict (g_rp_cbreuse_on: set where the hooks are installed, as the
// install records); KH_VM_SEE's re-issue saves (g_vmsee_sv_*: null outside one draw's kh_vmsee_t_begin / _end pair)
// and KH_WSEE's (g_wsee_sv_*: likewise, its kh_wsee_re_save / _restore pairs; g_wsee_pd_vp / _vpn: read only under
// g_wsee_pd_vp_ok, which the scratch reset clears); the KH_PSV blob store, its books and
// serial (g_psv_store, g_psv_books, g_psv_ser: the engine's shaders outlive every mission - each blob goes at its
// shader's release, KH_PSV_TOMB); g_ui_frame_rtv (kh_ui_mask_reset, below, releases it). Runs first in
// reset_session_state, after every device object is released, so no COM pointer is dropped unreleased; the calls
// after it there (stats, scratch, UI mask, in-front frame, error dedupe, PIP, sun ring, dynamic lights) set the rest.
inline void kh_session_globals_reset() {
    kh_rp_globals_reset();   // KH_VOL_REPLAY and its companions.
    g_stats_armed.store(false, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_ticks) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_count) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_us_pub) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_n_pub) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_sample_ctr) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_SAMPLE.
    for (auto& khsg_a : g_prof_tsc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_SUB.
    for (auto& khsg_a : g_prof_xb_acc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_UPLOAD.
    for (auto& khsg_a : g_fun_acc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_FUNNEL.
    g_prof_up_cur = KHP_ZONE_N;
    g_prof_prev_on = false;   // KH_PROF_WINDOW.
    g_prof_epoch.store(0, std::memory_order_relaxed);   // KH_PROF_DISARM.
    g_prof_epoch_seen = 0;
    kh_reinit(g_gpu_prof);
    for (auto& khsg_a : g_gpu_us_pub) khsg_a.store(0, std::memory_order_relaxed);
    g_cloth_q_n.store(0, std::memory_order_relaxed);
    g_skin_q_n.store(0, std::memory_order_relaxed);   // KH_POOL_SPLIT.
    kh_reinit(g_mat_grave);
    g_eds_memo_res = nullptr;
    g_eds_memo_w = 0;
    g_eds_memo_h = 0;
    kh_reinit(g_res);
    kh_reinit(g_ui_mask);
    g_ui_poison_run = 0;
    g_ui_mask_wanted.store(false, std::memory_order_relaxed);
    g_present_ui_ms.store(0, std::memory_order_relaxed);
    g_present_seq.store(0, std::memory_order_relaxed);
    kh_reinit(g_present_rtv);
    kh_reinit(g_present_rtv_id);
    g_present_rtv_next = 0;
    g_svs_mesh_wanted.store(false, std::memory_order_relaxed);
    g_fogsc_wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // KH_DLF_DEMAND.
    g_kh_flush_active.store(false, std::memory_order_relaxed);
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);
    g_ui_frame_rtv_seq = 0;
    kh_reinit(g_rain_vel);
    g_rain_phase = 0.0;
    kh_reinit(g_rain_prev_cam);
    g_rain_prev_valid = false;
    g_rain_prev_us = 0;
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    g_attach_repaired.store(0, std::memory_order_relaxed);
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_attach_final_step = false;
    g_sun_restep_cycle = ~0ull;
    g_flush_frame = 0;
    g_flush_frame_pub.store(0, std::memory_order_relaxed);
    kh_reinit(g_stats);
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
    g_kh_ctx1_null_first = false;
    g_cb_offsetting = false;
    g_scene_hold = false;   // KH_SCENE_HOLD (scoped; false outside a flush).
    kh_reinit(g_vis_memo_inj);
    kh_reinit(g_vis_memo_flush);
    kh_reinit(g_dist_memo_inj);
    kh_reinit(g_dist_memo_flush);
    kh_reinit(g_ord_keys);
    kh_reinit(g_ord_tmp);
    kh_reinit(g_cast_key_scr);   // KH_SKIN_POOL: the pooled sorts' tie-break scratch.
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    g_khsc_trim_debt.store(KH_CACHE_SHADER_TRIM_SLACK, std::memory_order_relaxed);   // KH_CACHE_TRIM_THROTTLE.
    kh_reinit(g_physics_bvh);
    g_physics_bvh_frame = 0;
    kh_reinit(g_cloth);
    kh_reinit(g_cloth_q);
    kh_reinit(g_cloth_grave);
    g_cloth_skipped.store(0, std::memory_order_relaxed);
    g_simlod_seq.store(0, std::memory_order_relaxed);   // KH_SIM_LOD: no view until the next boundary.
    for (auto& khsg_c : g_simlod_cam) khsg_c.store(0.0f, std::memory_order_relaxed);
    g_simlod_m11.store(0.0f, std::memory_order_relaxed);
    g_simlod_t.store(-1.0e30f, std::memory_order_relaxed);
    g_simlod_work.store(0, std::memory_order_relaxed);
    g_simlod_full.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_warned);
    g_cloth_shape_serial.store(0, std::memory_order_relaxed);
    kh_reinit(g_skin_q);
    kh_reinit(g_skin_col);
    g_skin_col_stamp = 0;
    g_skin_col_pub = 0;
    g_skin_col_lo = 0;
    g_skin_col_hi = 0;
    kh_reinit(g_skin_col_snap);
    kh_reinit(g_cloth_vb_slot);
    kh_reinit(g_cloth_mesh_slot);
    kh_reinit(g_cloth_gen_slot);
    kh_reinit(g_cloth_sim_slot);   // KH_LOD_SIM_SLOT.
    kh_reinit(g_cloth_voff_slot);   // KH_SKIN_POOL.
    kh_reinit(g_cloth_pool_slot);
    kh_reinit(g_cloth_objs_h);
    g_chain_skipped.store(0, std::memory_order_relaxed);
    kh_reinit(g_physics_col);
    g_physics_col_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_objs);
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_cloth_wind, khsg_v, sizeof(g_cloth_wind)); }
    g_cloth_dt_last = 0.0;
    kh_reinit(g_skin);
    kh_reinit(g_skin_rest);
    g_skin_gen_serial.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
    kh_reinit(g_skin_gpu);
    kh_reinit(g_skin_pool);   // KH_SKIN_POOL.
    g_skin_pool_serial = 0u;
    g_skin_pool_failed = false;
    g_skin_pool_made = false;   // KH_SKIN_POOL_MADE.
    g_skin_pool_any = false;
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // KH_SKIN_GPU.
    g_skin_so_retry_at = ~0ull;   // KH_SKIN_SO_RETRY.
    g_skin_so_big_failed = false;   // KH_SKIN_SO_LAZY: the large class's latch.
    g_skin_so_big_retry_at = ~0ull;
    g_skin_so_n.store(0, std::memory_order_relaxed);
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // KH_SKIN_CULL.
    g_skin_stat_cycle = ~0ull;
    g_skin_stat_gpu = 0u;
    g_skin_stat_cpu = 0u;
    g_skin_stat_cull = 0u;
    g_skin_cull_any = false;   // KH_SKIN_CULL: the attachments' flags went with them.
    g_skin_cull_rad_lo = 0.0f;
    g_skin_cull_rad_hi = 0.0f;
    g_skin_cull_pip_cycle = ~0ull;
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);
    kh_reinit(g_skin_rest_gpu);   // Its buffers went with kh_skin_release_all.
    kh_reinit(g_tex_pages);
    kh_reinit(g_tex_cache);
    g_tex_warned.store(0, std::memory_order_relaxed);   // KH_TEX_WARN_CAP.
    kh_reinit(g_user_ps_cache);
    kh_reinit(g_user_req);
    kh_reinit(g_user_vtx_scan);   // KH_USER_VS.
    kh_reinit(g_uvs);             // Its buffers went with kh_uvs_release_all.
    g_uvs_key_serial = 0u;
    g_user_req_seen = 0;
    g_user_req_settle = 0;
    g_user_req_waits = 0;
    g_user_pump_n = 0;   // KH_USER_REQ_IDLE: the stamps it is compared with went with the cache above.
    g_user_batch_own = false;
    kh_reinit(g_user_lut_cache);
    g_lod_proj_px = 0.0f;
    g_lod_zoom = 1.0f;   // KH_LOD_ZOOM.
    g_lod_inj_h = 0.0f;          // KH_LOD_PARK_KEEP.
    g_lod_inj_cycle = ~0ull;
    kh_reinit(g_khmw_q);
    g_khmw_running = false;
    g_khmw_abort.store(false, std::memory_order_relaxed);
    g_khmw_busy_id.store(-1, std::memory_order_relaxed);
    kh_reinit(g_khtl_q);
    kh_reinit(g_khtl_done);
    g_khtl_running = false;
    g_khtl_abort.store(false, std::memory_order_relaxed);
    g_vmir_vs = nullptr;
    g_khsg_scale.store(0.5f, std::memory_order_relaxed);
    g_khsg_w = 0;
    g_khsg_h = 0;
    g_comp_fail_streak = 0;
    g_comp_next_retry = 0.0f;
    kh_reinit(g_comp_last_err);
    g_sun_size_div.store(1, std::memory_order_relaxed);
    g_sun_map_size = KH_SUN_DEPTH_BASE;
    g_sun2_map_size = KH_SUN_HERO_BASE;
    g_sun3_map_size = KH_SUN_MID_BASE;
    g_sun4_map_size = KH_SUN_OUT_BASE;
    g_sun5_map_size = KH_SUN_FAR_BASE;
    g_sun_range.store(200.0f, std::memory_order_relaxed);
    g_obj_vis.store(0.0f, std::memory_order_relaxed);
    g_sun_range_src.store(0, std::memory_order_relaxed);
    g_shadows_off.store(false, std::memory_order_relaxed);   // KH_SHADOW_OFF.
    g_sun_pf_autogen = false;
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_composite_last_inject_ms.store(0, std::memory_order_relaxed);
    g_composite_inject_serial.store(0, std::memory_order_relaxed);
    g_composite_land_cycle = ~0ull;
    kh_reinit(g_last_pv);
    g_last_pv_ms = 0;
    g_proj_locator_ever = false;
    kh_reinit(g_khfx_pair);
    g_khfx_pair_near = -1.0f;
    g_khfx_cand_near = -1.0f;
    g_khfx_pair_ms = 0;
    g_khfx_verdict_serial = ~0ull;
    g_khfx_verdict_cycle = ~0ull;   // KH_FX_PAIR_HOLD.
    g_khfx_verdict_held = false;
    kh_reinit(g_khfx_in_pair);   // KH_FX_PAIR_KEY.
    g_vm_slice_live.store(false, std::memory_order_relaxed);
    g_vm_slice_known.store(false, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
    g_vm_watch_cycle = false;
    g_snap_serial = 0;
    g_snap_ms = 0;
    kh_reinit(g_inj_dp);
    g_inj_dp_valid = false;
    { const float khsg_v[2] = { 0.0f, 0.0f }; memcpy(g_snap_pair, khsg_v, sizeof(g_snap_pair)); }
    g_snap_pair_src = 0;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_snap_cam, khsg_v, sizeof(g_snap_cam)); }
    kh_reinit(g_snap_vp);
    g_snap_vp_lo = 0.011f;
    g_snap_vp_hi = 0.999f;
    g_snap_vp_valid = false;
    kh_reinit(g_thm_data);
    g_thm_w = 0;
    g_thm_h = 0;
    kh_reinit(g_thm_origin);
    g_thm_cell = 0.0f;
    g_thm_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thm_dirty.store(false, std::memory_order_relaxed);
    g_thm_valid.store(false, std::memory_order_relaxed);
    g_thm_reupload.store(false, std::memory_order_relaxed);   // KH_THM_DEVRESET.
    kh_reinit(g_thml_origin);
    g_thml_cell = 0.0f;
    g_thml_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thml_w = 0;
    g_thml_h = 0;
    g_thm_auto_state = 0;
    g_thm_auto_cursor = 0;
    g_thm_auto_retry = 0;
    g_thmf_dim = 0;
    g_thmf_cell = 0.0f;
    kh_reinit(g_voc_thm_max);
    kh_reinit(g_voc_thm_fine);
    g_voc_thm_w = 0;
    g_voc_thm_fw = 0;
    g_voc_thm_fh = 0;
    kh_reinit(g_voc_thm_origin);
    g_voc_thm_cell = 0.0f;
    g_voc_thm_valid = false;
    g_reorder_render_tid.store(0, std::memory_order_relaxed);
    g_watch_clears.store(0, std::memory_order_relaxed);   // KH_WATCH_STALL (and the three below).
    g_watch_cyc_seen = ~0ull;
    g_watch_clears_seen = 0;
    g_watch_still = 0;
    kh_reinit(g_ro);
    g_hook_exceptions.store(0, std::memory_order_relaxed);
    g_slot_keep_m22 = 0.0f;
    g_slot_keep_m32 = 0.0f;
    g_slot_keep_near = -1.0f;
    g_slot_keep_ms = 0;
    kh_reinit(g_cbc);
    kh_reinit(g_latch_cam);
    g_latch_cam_valid = false;
    g_cam_step_m = -1.0f;
    g_boundary_pv_valid = false;
    g_boundary_t = -1.0f;
    kh_reinit(g_proj_locator);
    kh_reinit(g_proj_pending);
    g_proj_last_m32 = 0.0f;
    kh_reinit(g_upc);   // KH_UPLOAD_LAZY.
    kh_reinit(g_uwc);
    g_sun_valid = false;
    { const float khsg_v[3] = { 0.0f, 1.0f, 0.0f }; memcpy(g_sun_dir_engine, khsg_v, sizeof(g_sun_dir_engine)); }
    g_fog_staged_valid = false;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog_staged, khsg_v, sizeof(g_fog_staged)); }
    g_fog_valid = false;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog, khsg_v, sizeof(g_fog)); }
    g_sun_jump_pending = false;
    { const float khsg_v[3] = { 0, 0, 0 }; memcpy(g_skysun_ref, khsg_v, sizeof(g_skysun_ref)); }
    g_skysun_ref_valid = false;
    g_skysun_ref_ms = 0;
    kh_reinit(g_skysun_mot_ref);
    g_skysun_mot_ref_ms = 0;
    g_skysun_mot_deg = -1.0f;
    g_atlas_frame_cur = false;
    g_atlas_frame_prev = false;
    g_pub_valid = false;
    kh_reinit(g_pub_dir);
    g_pub_block_valid = false;
    { const float khsg_v[8] = { -1,-1,-1,-1,-1,-1,-1,-1 }; memcpy(g_band_tab_far, khsg_v, sizeof(g_band_tab_far)); }
    kh_reinit(g_pub_block_amb);
    kh_reinit(g_pub_block_sun);
    g_lite_cand_n = 0;
    g_lite_cand_sl = 0.0f;
    g_lite_cand_al = 0.0f;
    g_blko_draws = 0;
    kh_reinit(g_blko_cur);
    g_blko_n = 0;
    g_blko_open_draws = 0;
    g_blko_open = false;
    g_blko_open_i = -1;
    kh_reinit(g_blko_open_amb);
    kh_reinit(g_blko_open_sun);
    kh_reinit(g_blko_gen_amb);   // KH_DLSW_GEN_AMB.
    kh_reinit(g_blko_gen_sun);
    g_blko_gen_valid = false;
    kh_reinit(g_sraw_cyc);
    g_sraw_cyc_n = 0;
    g_sraw_last_sample_ms = 0;
    g_sraw_last_src = 0;
    kh_reinit(g_sraw_sky_prev);
    g_sraw_sky_prev_valid = false;
    g_sraw_sky_step_deg = -1.0f;
    g_sraw_pub_gap_deg = -1.0f;
    g_sraw_sky_gap_deg = -1.0f;
    kh_reinit(g_sraw_ring);
    g_sraw_ring_n = 0;
    g_sraw_ring_i = 0;
    kh_reinit(g_sraw_med_prev);
    g_sraw_med_prev_valid = false;
    g_sraw_hold_until_ms = 0;
    g_sraw_cons_moved_ms = 0;
    g_sraw_unsettled_ms = 0;
    kh_reinit(g_ls);
    g_ls_cam_cycle = ~0ull;
    kh_reinit(g_inj_view);
    g_inj_view_ms = 0;
    kh_reinit(g_light_probe);
    kh_reinit(g_sky_probe);
    kh_reinit(g_skybind);
    g_ring_best_valid = false;
    g_acq_hold.store(false, std::memory_order_relaxed);
    g_ring_best_er = 0.0f;
    g_ring_best_res = nullptr;
    g_ring_best_off = 0;
    g_ring_best_orient = 1;
    g_ring_best_cycle = 0;
    kh_reinit(g_ring_best_m);
    g_pub_first = -1.0f;
    g_prewarm_cand_res = nullptr;
    g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr;
    g_coldlock_cand_off = 0;
    g_cc_flush_serial.store(0, std::memory_order_relaxed);
    g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_inj_enc_m22 = 0.0f;
    g_inj_enc_m32 = 0.0f;
    g_inj_enc_near = -1.0f;
    g_inj_enc_ms = 0;
    g_ms_flush_serial = 0;
    g_carry_pending_serial.store(0, std::memory_order_relaxed);
    g_rescue_frame_count = 0;
    g_kh_ctx1 = nullptr;
    g_kh_ctx1_for = nullptr;
    kh_reinit(g_rt0_desc);
    g_rt0_desc_n = 0;
    g_rt0_desc_next = 0;
    g_shadow_map_strength = 1.0f;
    g_shadow_map_bias = 0.0015f;
    g_shadow_map_sign = 1.0f;
    g_sun_map_rendered_frame = false;
    g_sun_map_gen = 0;   // KH_SUN_GEN.
    g_sun_gen_win_start = 0;   // KH_SUN_GEN_CAP.
    g_sun_gen_win_n = 0;
    g_sun_gen_hold_until = 0;
    g_sun_map_valid = false;
    g_sun_map_cam_anchor = false;
    g_sun_map_render_time = -1.0f;
    g_sun_mat_chash = 0;
    kh_reinit(g_sun_mat_sun);
    g_sun_map_time = -1.0f;
    kh_reinit(g_sun_map_vp);
    kh_reinit(g_sun_anchor_now);
    kh_reinit(g_sun_cam_now);   // KH_DLS_LOD_CAM.
    kh_reinit(g_sun_map_anchor);
    kh_reinit(g_sun_tier_anchor);
    kh_reinit(g_sun_tier_lat);   // KH_PCSS_CELL.
    g_vmir_w = 0;
    g_vmir_h = 0;
    g_vmir_mask_time = -1.0f;
    g_sun_map_bias = 0.0f;
    g_sun_map_cast_bias = 0.0f;
    kh_reinit(g_sun2_map_vp);
    g_sun2_map_bias = 0.0f;
    g_sun2_cast_bias = 0.0f;
    g_sun2_half_diag = 0.0f;
    g_sun2_map_valid = false;
    g_sun2_renders = 0;
    kh_reinit(g_sun3_map_vp);
    g_sun3_map_bias = 0.0f;
    g_sun3_cast_bias = 0.0f;
    g_sun3_half_diag = 0.0f;
    g_sun3_map_valid = false;
    g_sun3_renders = 0;
    kh_reinit(g_sun4_map_vp);
    g_sun4_map_bias = 0.0f;
    g_sun4_cast_bias = 0.0f;
    g_sun4_half_diag = 0.0f;
    g_sun4_map_valid = false;
    g_sun4_renders = 0;
    kh_reinit(g_sun5_map_vp);
    g_sun5_map_bias = 0.0f;
    g_sun5_cast_bias = 0.0f;
    g_sun5_half_diag = 0.0f;
    g_sun5_map_valid = false;
    g_sun5_renders = 0;
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_valid, khsg_v, sizeof(g_sun_pf_valid)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_fresh, khsg_v, sizeof(g_sun_pf_fresh)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_held, khsg_v, sizeof(g_sun_pf_held)); }
    { const uint64_t khsg_v[4] = { 0, 0, 0, 0 }; memcpy(g_sun_tier_key, khsg_v, sizeof(g_sun_tier_key)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_tier_key_ok, khsg_v, sizeof(g_sun_tier_key_ok)); }
    g_sun_pf_done = ~0ull;
    { const float khsg_v[4] = { -1.0f, -1.0f, -1.0f, -1.0f }; memcpy(g_sun_fwd_held, khsg_v, sizeof(g_sun_fwd_held)); }
    g_sun_union_rad_held = -1.0f;
    g_sun_union_lat_held = -1.0f;
    g_sun_map_hash = 0;
    kh_reinit(g_sun_map_bounds);
    g_fire_lock = nullptr;
    kh_reinit(g_fire_fov2);
    kh_reinit(g_fire_dims2);
    kh_reinit(g_fire_view2);
    kh_reinit(g_fire_sun2);
    g_fire_lock_valid = false;
    g_fire_post_scene_cycle = ~0ull;
    g_fire_count_frame = 0;
    g_lock_churn_ms_v = 0;
    g_fire_era_reject_run = 0;
    g_cast_ready = false;
    g_cast_ready_since_ms = 0;
    g_cascbind_feed_t = -1.0f;
    g_fire_clamp_target = 1;
    g_fire_clamp_painted = 0;
    g_fire_mask_srv_now = false;
    g_view_drop_path = 0;
    g_view_drop_bar = -1.0f;
    g_view_drop_hyst_since = 0;
    g_view_relock_wide_cycle = 0;
    kh_reinit(g_band_pool);
    g_band_fall_flush = ~0ull;
    g_recv_health_ok = false;
    g_recv_health_since_ms = 0;
    kh_reinit(g_fire_cast_inv);
    g_fire_cast_inv_valid = false;
    kh_reinit(g_sun_locals);
    g_sun_local_count = 0;
    g_sun_locals_serial = 0;
    g_mask_cast_arm = false;
    g_cast_map_frame = ~0ull;   // KH_CAST_FRAME64.
    g_mission_destroy_pending.store(false, std::memory_order_relaxed);
    g_topo_scene_tex_id = nullptr;
    g_topo_scene_seen_cycle = 0;   // KH_SCENE_NAME.
    g_mask_cast_fired = false;
    g_dl_pool_next_id = 1;
    g_dl_intensity_bits.store(0x3F800000u, std::memory_order_relaxed);
    kh_reinit(g_cbs);
    g_cbs_gen = 0;
    g_cbs_wseq = 0;   // KH_DL_REC_ORDER: with the pool (kh_reinit(g_dl), next), whose records it orders.
    kh_reinit(g_dl);
    g_dl_harvest_cycle = ~0ull;
    g_dl_harvest_flush_last = false;
    kh_reinit(g_dl_phix);   // KH_DL_PRES_HIX: the pool's height index (with g_dl, above).
    kh_reinit(g_dls);
    g_dls_n = 0;
    g_dls_frame = 0;
    g_dls_sel_ms = 0;   // KH_DLS_POOL_AGE.
    g_dls_allowed.store(false, std::memory_order_relaxed);   // KH_DLS_ALLOW: off.
    kh_reinit(g_dls_keys);
    g_dlr_buf = nullptr;   // KH_DL_RING: released by release_shadow_device_state first.
    g_dlr_srv = nullptr;
    g_dlr_cap = 0;
    g_dlr_cursor = 0;
    g_dlr_fresh = true;
    g_dlr_nooverwrite = -1;
    g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
    g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
    kh_reinit(g_sr);
    kh_reinit(g_cascbind);
    g_cascharv_ctr = 0;
    kh_reinit(g_mask);
    kh_reinit(g_dls_vp);
    kh_reinit(g_dls_facen);
    kh_reinit(g_dls_faceslice);
    kh_reinit(g_dls_facemask);
    kh_reinit(g_dls_isspot);
    kh_reinit(g_dls_face_hold);
    kh_reinit(g_dls_mapfar);
    g_dls_map_valid = 0;
    kh_reinit(g_dlsw_casters);
    g_dls_frame_cycle = ~0ull;
    g_castocc_key = 0;
    kh_reinit(g_castocc_lo);
    g_castocc_inv = 0.0f;
    kh_reinit(g_castocc_grid);
    kh_reinit(g_castocc_set);
    kh_reinit(g_castocc_addi);
    g_castocc_cell = 0.0f;
    kh_reinit(g_castocc_sunb);
    kh_reinit(g_castocc_dom);
    g_castocc_slack = 0.0f;
    g_castocc_rdom = 0.0f;
    g_castocc_rng = 0.0f;   // KH_CASTOCC_RANGE.
    g_castocc_floor = 0.0f;
    kh_reinit(g_castocc_pre);
    g_castocc_pre_noreach = false;
    kh_reinit(g_castocc_pre_noreach_dom);
    g_castocc_kc = 0;
    g_castocc_kc_ser = ~0ull;
    g_castocc_kc_rng = -1.0f;
    g_castocc_kc_n = -1;
    kh_reinit(g_svs_rtv_seen);
    g_svs_rtv_seen_n = 0;
    kh_reinit(g_svs_cls_id);
    kh_reinit(g_svs_cls_srv);
    g_svs_cls_n = 0;
    g_svs_pending = false;
    g_svs_pend_w = 0;
    g_svs_pend_h = 0;
    g_svs_mask_id = nullptr;
    g_svs_mask_srv = nullptr;
    g_svs_mask_w = 0;
    g_svs_mask_h = 0;
    g_svs_mask_fmt = 0;
    g_svs_mask_cand = nullptr;
    g_svs_pre_tex = nullptr;
    g_svs_pre_srv = nullptr;
    g_svs_pre_w = 0;
    g_svs_pre_h = 0;
    g_svs_pre_fmt = 0;
    g_svs_post_tex = nullptr;
    g_svs_post_srv = nullptr;
    g_svs_post_made = 0;
    g_depth_clear_serial = 0;
    g_svs_skip_sig = 0;
    g_svs_skip_valid = false;
    g_svs_frame_seq = 0;
    g_svs_bracket = 0;
    kh_reinit(g_svs_caster_list);
    kh_reinit(g_svs_inst_ord);
    kh_reinit(g_svs_prime_vp);
    g_khpe_p1 = -1.0f;
    g_khpe_p2 = -1.0f;
    g_khpe_m22 = 0.0f;
    g_khpe_ms = 0;
    g_khpe_upd_ms = 0;
    g_khpe_skip1 = -1.0f;
    g_khbl_stable = -1.0f;
    g_khbl_prev = -1.0f;
    kh_reinit(g_khlt_prev);
    g_khlt_prev_ok = false;
    g_khlt_prev_step = 0.0f;
    kh_reinit(g_khlt_step_ring);
    g_khlt_step_ring_i = 0;
    kh_reinit(g_khlt_ref_cam);
    g_khlt_ref_ok = false;
    kh_reinit(g_khlt_lp);
    g_khlt_lp_ok = false;
    kh_reinit(g_svs_prime_cam);
    g_svs_prime_rebase = false;
    g_svs_prime_vp_lo = 0.0f;
    g_svs_prime_vp_hi = 1.0f;
    g_svs_vol_src = nullptr;
    g_svs_vol_src_id = nullptr;
    g_svs_vol_tex = nullptr;
    g_svs_vol_sten_srv = nullptr;
    g_svs_vol_depth_srv = nullptr;   // KH_VOL_WITNESS: released by kh_svs_vol_release first.
    g_svs_vol_w = 0;
    g_svs_vol_h = 0;
    g_svs_vol_primed = false;
    g_svs_vol_seq = 0;
    kh_reinit(g_svs_seam_enc);   // KH_VOL_WITNESS.
    g_svs_seam_enc_seq = ~0ull;
    kh_reinit(g_svs_vol_enc);
    g_svs_vol_enc_ok = false;
    kh_reinit(g_svs_seam_proj);   // KH_VOL_ZOOM.
    g_svs_seam_proj_ok = false;
    kh_reinit(g_svs_vol_proj);
    g_svs_vol_proj_ok = false;
    // KH_VOL_FOOT: released by kh_svs_vol_release (kh_svs_foot_release) first.
    kh_reinit(g_svs_foot_tex);
    kh_reinit(g_svs_foot_rtv);
    kh_reinit(g_svs_foot_srv);
    g_svs_foot_w = 0;
    g_svs_foot_h = 0;
    g_svs_foot_pub = -1;
    g_svs_foot_seam_ix = -1;
    g_svs_foot_seam_seq = ~0ull;
    g_svs_vol_foot_ok = false;
    g_svs_prime_ready = false;
    g_svs_vol_dsv_now = false;
    g_svs_vol_dsv_bound = false;
    g_svs_injected_frame = false;
    kh_reinit(g_engcam_cand);
    g_engcam_cand_n = 0;
    g_engcam_cand_seq = ~0ull;
    g_engcam_res = nullptr;
    g_engcam_off = -1;
    kh_reinit(g_engcam_vals);
    g_engcam_vals_n = 0;
    g_engcam_val_seq = ~0ull;
    kh_reinit(g_engcam_frame_val);
    g_engcam_frame_seq = ~0ull;
    g_engcam_frame_taken = false;
    kh_reinit(g_engcam_ref);
    g_engcam_ref_ok = false;
    g_vmir_tex = nullptr;
    g_vmir_dsv = nullptr;
    g_vmir_srv = nullptr;
    g_vmir_b2_src = nullptr;
    g_vmir_b2_src_srv = nullptr;
    g_vmir_b2_out = nullptr;
    g_vmir_b2_out_rtv = nullptr;
    g_vmir_b2_patch = nullptr;
    g_vmir_b2_vs = nullptr;
    g_vmir_b2_ps = nullptr;
    g_vmir_b2_failed = false;
    g_vmir_pending = false;
    g_vmir_frame_ok = false;
    g_vmir_mark2 = 0;
    g_vmir_prepass_stamp = 0;
    kh_reinit(g_vmir_rs_src);
    kh_reinit(g_vmir_rs_clamp);
    g_vmir_sv_rs = nullptr;
    g_vmir_prepass_src = nullptr;
    kh_reinit(g_b2_snap);
    g_b2_learn = nullptr;
    kh_reinit(g_vmir_sv_rtvs);
    g_vmir_sv_dsv = nullptr;
    kh_reinit(g_vmir_sv_vps);
    g_vmir_sv_nvp = 0;
    g_vmir_sv_b2 = nullptr;
    g_vmir_sv_b2_first = 0;
    g_vmir_sv_b2_num = 0;
    g_vmir_sv_b2_off_ok = false;
    g_pip_seen = false;
    kh_reinit(g_pip);
    g_infront_wanted.store(false, std::memory_order_relaxed);
    g_vm_watch_wanted.store(false, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
    g_vm_prev_near = false;
    g_vm_pre_done = false;
    g_vm_col_done = false;
    g_vm_pre_injected = false;   // KH_INFRONT_PRE_COL.
    g_vm_seam_done = false;
    g_vm_frame_valid = false;
    g_vm_frame_cycle = 0;
    kh_reinit(g_vm_view_proj);
    kh_reinit(g_vm_proj);   // KH_MIR_ZOOM.
    kh_reinit(g_vm_cam);
    g_vm_rebase = 0.0f;
    g_vm_m22 = 0.0f;
    g_vm_m32 = 0.0f;
    g_vm_keep_m22 = 0.0f;
    g_vm_keep_m32 = 0.0f;
    g_vm_keep_ms = 0;
    kh_reinit(g_vm_meshes);
    kh_infront_tail_release();   // KH_INFRONT: the deferred tail's held pair (released; the viewport is scratch).
    kh_reinit(g_vm_tail_vp);
    g_inj_arb_prev_armed = false;
    g_dls_world_cycle = 0;
    g_fx_late_owed_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_ran_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_dry_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_streak.store(0, std::memory_order_relaxed);
    g_fx_late_backoff.store(0, std::memory_order_relaxed);
    g_fx_late_miss_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_miss_cycle2.store(~0ull, std::memory_order_relaxed);
    g_depth_readopt_run = 0;
    kh_reinit(g_hdc_blend);
    kh_reinit(g_hdc_dss);
    g_flush_fx_arb_prev = false;
    g_video_opt_ms = 0;
    g_dl_cell.store(0.0f, std::memory_order_relaxed);   // KH_DL_ORIGIN_GRID: the next session reads its terrain.
    g_dl_q_cells = 0;
    g_dl_stall_a = 0;   // KH_DL_STALL_GAP.
    g_dl_stall_b = 0;
    g_ui_rehoist_auto_s = -1.0e9f;
    g_ui_prev_scene_frame = 0;
    g_ui_prev_from_present = false;   // KH_UI_ORDINAL_SRC.
    g_ui_rehoist_last_s = -1.0e9f;
}

// KH_SESSION_RESET - the globals the objects of a mission hold on to (scene slots, handles and sequence numbers,
// attachment and helper-pool books, affectors, cloth proxies and chain settings, the event-handler flags, the mesh
// path keys and reference times, the material table, the effect clock), to their declared start the same way. Only
// when no object exists (kh_session_teardown's clean path): the deferred destroy runs under the next mission's
// objects, which these belong to.
inline void kh_session_objects_reset() {
    // KH_CACHE_SESSION_KEEP: the disk cache's files read or written from here on are the next session's own.
    g_khmc_session_t0.store(std::filesystem::file_time_type::clock::now().time_since_epoch().count(),
                            std::memory_order_relaxed);
    kh_reinit(g_scene);
    g_scene_unpub_min = 0xFFFFFFFFu;   // KH_LOD_BOUNDS.
    kh_reinit(g_scene_stage);
    kh_reinit(g_scene_handle);
    kh_reinit(g_scene_dirty_mark);
    kh_reinit(g_scene_dirty);
    g_scene_dirty_n.store(0u, std::memory_order_relaxed);   // KH_SYNC_IDLE.
    kh_reinit(g_scene_free);
    kh_reinit(g_objbuf_cpu);
    kh_reinit(g_objbuf_dirty);
    kh_reinit(g_objbuf_dirty_mark);
    g_objbuf_shoff = false;   // KH_SHADOW_OFF.
    g_gts_seq = 0;   // KH_ATTACH_GT_SNAP: the bindings' sample numbers (their rings, picks and helper stamps).
    g_skel_gen_serial = 0;
    g_attach_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_attach_dead);
    g_attach_dead_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_attach_proxy_dead);
    g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
    g_pxy_pool_n.store(0, std::memory_order_relaxed);
    g_gts_done_pub.store(0, std::memory_order_relaxed);   // Names a g_gts_seq value: restarts with it.
    g_gts_done_defer = 0;   // KH_CHAIN_END_DRAW.
    for (auto& khsg_st : g_gts_stamp_pub) khsg_st.store(0, std::memory_order_relaxed);
    g_next_seq = 0;
    kh_reinit(g_chain_cfg);
    kh_reinit(g_chain_end);   // KH_CHAIN_END_DRAW (reset_retained_state has emptied it).
    g_chain_end_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_chain_warned);
    kh_reinit(g_chain_nobuild);
    g_ground_ready = false;
    g_ground_tried = false;
    g_ground_cell = 0.0f;
    g_ground_n = 0;
    { const uint8_t khsg_v[4] = { 2, 2, 2, 2 }; memcpy(g_ground_split, khsg_v, sizeof(g_ground_split)); }
    kh_reinit(g_affectors);
    g_affectors_n.store(0, std::memory_order_relaxed);
    g_aff_next_seq = 0;
    kh_reinit(g_aff_frame);
    g_tex_page_gen = 0;
    g_tex_land_serial = 0;   // KH_TEX_LAND.
    kh_reinit(g_fbx_cache);
    kh_reinit(g_mesh_gen_cache);   // KH_MESH_GEN_SHARE.
    kh_reinit(g_mesh_last_ref);
    kh_reinit(g_ref_note_cen);   // KH_REF_NOTE_ONCE.
    kh_reinit(g_mat_note_cen);   // KH_MAT_NOTE_ONCE.
    g_ref_note_cen_ms.store(0, std::memory_order_relaxed);   // KH_GC_NOTE_SKIP.
    kh_reinit(g_ref_note_gc);
    kh_reinit(g_mat_gpu_cpu);
    g_mat_gpu_dirty = false;
    g_mat_gpu_page_gen = 0;
    kh_reinit(g_mat_anim);   // KH_UV_ANIM: the list, its two cycle stamps and the cycle's time.
    g_mat_anim_done = ~0ull;
    g_mat_anim_tcyc = ~0ull;
    g_mat_anim_t = 0.0;
    g_effect_t0.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_proxy);
    kh_reinit(g_cloth_proxy_want);
    g_cloth_proxy_frame = 0;
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_draw3d_eh_active = false;
}

// Device objects and locator/probe state go through release_shadow_device_state + g_res.release (the caller runs
// those first, under the graphics lock); this resets what those don't. Must run with the render thread parked:
// g_ro, the mask/live structs and the hoisted memories are render-thread state.
inline void reset_session_state() {
    kh_session_globals_reset();   // KH_SESSION_RESET: everything to its declared start; the calls below add the rest.
    reset_stat_counters();
    kh_session_scratch_reset();
    kh_ui_mask_reset();   // Learned backbuffers, phase machine, demand flag.
    kh_infront_frame_reset();

    {   // Error dedupe the session (a fresh session re-reports).
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        g_err_once.clear();
        g_err_once_capped = false;   // KH_ERR_ONCE_CAP.
    }
    {   // KH_EA_LOG_ONCE: the warnings' dedupe, likewise.
        std::lock_guard<std::mutex> g(g_kh_log_once_mx);
        g_kh_log_once.clear();
        g_kh_log_once_capped = false;
    }

    kh_pip_reset();   // KH_PIP.
    kh_sraw_ring_reset();   // KH_SRAW_SETTLE.
    dynlights_reset_session();
}

inline void reset_retained_state() {
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& kv : g_draw_list) kh_scene_stage_remove(kv.second);   // KH_SCENE: deaths sync later.
        kh_attach_drop_all(false);   // KH_ATTACH: a mission edge must not hold an object to process exit.
        g_draw_list.clear();
        g_next_seq = 0;   // Creation order restarts with the list it orders.
    }
    kh_crew_clear_all();   // KH_CREW_LAG: outside the mutex (its entries' event handlers go).
    kh_aff_clear();   // KH_AFFECTOR: game thread, like the table itself.
    // KH_CLOTH_PROXY: released, not deleted - the mission's end takes its own objects, and a mission edge must not
    // hold one (kh_attach_drop_all).
    g_cloth_proxy.clear();
    g_cloth_proxy_want.clear();
    g_chain_cfg.clear();   // KH_CHAIN: its proxies too are the mission's, released the same way.
    {   // KH_CHAIN_END_DRAW: the render thread reads the table under the mutex.
        std::lock_guard<std::mutex> khce_g(g_draw_list_mutex);
        g_chain_end.clear();
        g_chain_end_n.store(0, std::memory_order_relaxed);
    }
    g_chain_warned.clear();
    g_chain_nobuild.clear();
    g_aff_frame.clear();
    g_aff_next_seq = 0;
    // KH_CLOTH_GROUND: measured again on the next mission's matrix.
    g_ground_tried = false;
    g_ground_ready = false;
}

// KH_SESSION_RESET - the whole teardown, under the park, for both mission-edge paths: the joinable workers first
// (texture loader, mesh cache writer, the cloth / skin pool - nothing may run on after its data is gone), then every
// device object and learned identity, the meshes (into the grave the resource release frees), the instances, the
// caches (textures, user .hlsl, user .cube, materials, asset paths), the shader pool (bounded wait - none for a
// user-shader batch, KH_USER_END_NOWAIT - then detach: a job inside D3DCompile cannot be killed), and last the
// session state, which puts every other global back to what a fresh process holds. khst_clean: no object exists
// (rendering_integration_reset, after kh_mission_handles_reset). False for kh_park_run's deferred destroy, which
// runs under the next mission's objects: what they hold (meshes, materials, scene slots, handles, the effect clock)
// is left to them.
inline void kh_session_teardown(bool khst_clean) {
    kh_texldr_stop();   // KH_TEX_ASYNC: joined, results dropped.
    kh_mesh_cache_writer_stop();   // Joined, queued-but-unwritten entries dropped.
    kh_cloth_workers_stop();   // Joined; its queues dropped.
    release_shadow_device_state();
    if (khst_clean) kh_mesh_release_session();   // Ahead of g_res.release, which frees the grave it fills.
    g_res.release();
    kh_cloth_drop_all();   // KH_CLOTH: the instances and their buffers.
    kh_skin_drop_all();    // KH_SKEL: likewise.
    kh_uvs_release_all();  // KH_USER_VS: likewise (the records hold nothing a mission outlives).
    kh_tex_cache_release();
    // KH_USER_END_NOWAIT: a user-shader batch in flight (read before the release below clears g_user_batch_own; only
    // one batch is ever live, so its workers are all the batch's) is not waited for: detached at once. Its units are
    // this mission's requests, which the release drops, and a material's unit is the builtin mesh shader with the
    // file - tens of seconds of fxc - so the bounded wait would freeze the game for all of its 5 s at the mission's
    // end and the compile would detach anyway (an effect's unit, about a second, would end inside the wait; detached,
    // it ends during the next mission's start). A unit it finishes still lands on disk (its bytecode in the memo too),
    // and the next mission's first ensure holds every mesh on the white placeholder until it ends, as after a wait that
    // ran out.
    const bool khst_user_batch = g_user_batch_own && g_khsm_active;
    kh_user_shader_cache_release();
    kh_user_lut_cache_release();
    if (khst_clean) kh_material_pool_reset();
    RenderAssetDiscovery::clear_cache();
    // KH_SHADER_ORPHANS (g_khsm_orphans): a detached worker runs out its unit after the mission and exits.
    kh_shader_mt_shutdown(!khst_user_batch);
    g_vs_memo_epoch.fetch_add(1u, std::memory_order_relaxed);   // Every thread's read memo empties at its next read.
    if (khst_clean) {
        kh_session_objects_reset();
    } else {
        // KH_ATTACH_CYCLE_RESTAMP: the next mission's bindings and chain ends stay (kh_session_objects_reset is the
        // clean edge's), but reset_session_state below restarts g_topo_cycles - a kept stamp would match once, when
        // the new counter reached it, and hand that cycle a pick, a read or a stash from months before (the rule the
        // scratch reset keeps for the global latches). Under the park, as the step that writes them runs.
        std::lock_guard<std::mutex> khst_g(g_draw_list_mutex);
        for (auto& khst_e : g_attach) {
            khst_e.second.gts_cyc = ~0ull;
            khst_e.second.rt_cyc = ~0ull;
            khst_e.second.rt_chk_cyc = ~0ull;
            khst_e.second.skel_smp_cycle = ~0ull;
            khst_e.second.smp_cycle = ~0ull;
            khst_e.second.skin_culled = false;   // KH_SKIN_CULL: the next step decides afresh.
            khst_e.second.cull_rest.reset();
            khst_e.second.cull_a.clear();
        }
        g_skin_cull_any = false;
        for (auto& khst_c : g_chain_end) khst_c.second.rt_cyc = ~0ull;
    }
    reset_session_state();
}

// Both mission edges: the event-handler handles and the per-mission retained state go first.
inline void kh_mission_handles_reset() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();
}

// The whole-system reset, both mission edges: the handles and the retained state, then kh_session_teardown under
// the park (the prewarm refuses while any detached shader worker lives).
inline void rendering_integration_reset() {
    if (!kh_render_on()) return;   // KH_PLAYER_ONLY: nothing of the renderer ever started.
    kh_psv_hook_ensure();   // KH_PSV: the CreatePixelShader hook, if the extension's load could not install it.
    bool khme_done = false;
    kh_mission_handles_reset();

    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);

    for (int attempt = 0; attempt < 8; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) continue;
        kh_session_teardown(true);   // Clean: kh_mission_handles_reset emptied the draw list above.
        khme_done = true;
        break;
    }

    // A mission that never uses the extension holds the state disarmed.
    if (!khme_done) {
        // KH_TEARDOWN_KEEP_SETTINGS: the deferred teardown keeps the script settings it finds (the next mission's),
        // so this mission's go back to their declared defaults here, on the game thread (atomics). The stats take
        // reset_stat_counters: disarmed and zeroed, the unlocked best effort reset_render_stats_sqf also falls back to.
        g_dls_allowed.store(false, std::memory_order_relaxed);
        g_khsg_scale.store(0.5f, std::memory_order_relaxed);
        g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
        g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
        reset_stat_counters();
        g_mission_destroy_pending.store(true, std::memory_order_relaxed);
    }
}

// Waits up to khts_ms for a joinable thread to exit, terminates it if it has not, then joins it (detaching it if the
// join throws).
inline void kh_thread_stop_bounded(std::thread& khts_t, DWORD khts_ms) {
    if (!khts_t.joinable()) return;
    HANDLE khts_h = static_cast<HANDLE>(khts_t.native_handle());
    if (khts_h && WaitForSingleObject(khts_h, khts_ms) == WAIT_TIMEOUT) TerminateThread(khts_h, 0);
    try { khts_t.join(); } catch (...) { try { khts_t.detach(); } catch (...) {} }
}

// True when anything of the render integration is running or exists: script objects (meshes, post FX, live or
// staged), the per-frame drivers, an installed hook or the reset hook, device resources, a shader compile in any
// form (the pool, the async batch, a user .hlsl request), a worker thread or its queued / finished jobs, a pending
// mesh publish or deferred destroy, the caches, the shadow and PIP state, the terrain heightfield. A session that
// never touched the extension reads false on every term and DllMain skips the detach, as it does for the other
// frameworks. No lock may be taken from DllMain, so every read is lock-free: the script-side containers' writer is
// the game thread, the caller; the worker queues (g_khtl_q / g_khtl_done, g_khmw_q, g_khsm_map) and g_tex_cache
// have other writers and are read racily. Each worker's has a term that reads true while that writer can run
// (g_khtl_running, g_khmw_running, g_khsm_live); the render thread's writes (g_tex_cache, g_khsm_map's erases - the
// hooks' draws or the shared Present's UI flush) need an object or a UI effect, so g_draw_list / g_res.initialized
// read true. The verdict does not hang on the racy reads.
inline bool rendering_integration_is_initialized() {
    return !g_draw_list.empty() || g_scene.alive_n > 0 || !g_affectors.empty() || !g_cloth_proxy.empty() ||
           g_draw3d_eh_active || g_ui_driver_registered ||
           g_reorder_hook_active.load(std::memory_order_acquire) || g_reset_hook_installed ||
           g_res.initialized ||
           g_khsm_active || g_khsa_state != 0 || !g_khsm_thr.empty() ||
           g_khsm_live.load(std::memory_order_acquire) > 0 || !g_khsm_map.empty() ||
           g_user_batch_own || !g_user_req.empty() ||
           g_khtl_running || !g_khtl_q.empty() || !g_khtl_done.empty() ||
           g_khmw_running || !g_khmw_q.empty() ||
           !g_cloth_thr.empty() || !g_skin_thr.empty() ||   // KH_POOL_SPLIT: either pool's threads.
           g_mesh_publish_pending.load(std::memory_order_relaxed) ||
           g_mission_destroy_pending.load(std::memory_order_relaxed) ||
           !g_tex_cache.empty() || !g_user_ps_cache.empty() || !g_user_lut_cache.empty() ||
           !g_mat_pool.empty() ||
           g_ls.wanted.load(std::memory_order_relaxed) || g_ls.atlas_tex != nullptr ||
           g_dlf_wanted.load(std::memory_order_relaxed) ||   // KH_DLF_DEMAND.
           g_pip_seen || g_thm_valid || g_thm_dirty || g_thm_auto_state != 0 ||
           g_stats_armed.load(std::memory_order_relaxed);
}

// Process detach (DllMain, the FreeLibrary path only - lpReserved == nullptr, like every other framework's
// teardown). On the terminating path the OS has already killed our threads and nothing here may run: the five
// thread objects (loader, cache writer, shader pool, cloth pool, skin pool) are heap-leaked so no static destructor
// can std::terminate on a still-joinable thread; and the CRT still runs this DLL's static destructors after DllMain
// on that path, so every Intercept event-handler handle (whose destructor runs an SQF command) and every container
// of object references a quit without mission_ended leaves full is heap-leaked too (KH_EXIT_STATICS). The plain
// code / game_value statics left (as framework.hpp's) only drop a reference there, as on every exit. That is what
// makes that path safe without a call.
//
// Unloading: our threads are alive and would return into unmapped code once the DLL goes. Each is asked to stop,
// waited for briefly and terminated - the shader pool's workers an earlier shutdown detached too, by the handles
// kept for them (g_khsm_orphans). Under the loader lock a worker that obeys cannot finish exiting (its thread detach
// needs that lock), so every wait runs its budget out and every worker is terminated; the stop request is what gets
// it out of our code first (the loader, the cache writer and the shader pool return before their next job,
// KH_KHMW_ABORT_STOP; the cloth and skin pools after the jobs already queued, below; a job inside D3DCompile or a
// large cache write cannot be interrupted). The framework's watchdog bounds the whole detach regardless. No graphics
// lock (the render thread may never park again) and no device release (a leaked COM reference at unload costs
// nothing; a context call could hang).
inline void rendering_integration_process_detach() {
    // Every hook is a pass-through from here and every demand gate reads off.
    g_psv_cap_on.store(false, std::memory_order_relaxed);   // KH_PSV: the CreatePixelShader detour copies nothing.
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);
    g_ls.wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // KH_DLF_DEMAND.
    g_dl_idle.store(0, std::memory_order_relaxed);   // KH_DL_IDLE.
    g_khsa_abort.store(true, std::memory_order_relaxed);
    g_khmw_abort.store(true, std::memory_order_relaxed);
    g_khtl_abort.store(true, std::memory_order_relaxed);
    g_cloth_stop.store(true, std::memory_order_relaxed);
    // The reset hook is a pointer into this DLL held by kh_rv_extension, a module the engine loaded on its own and
    // which outlives an unload of ours; cleared so a later device reset cannot call into unmapped code. The setter
    // forwards a bare function pointer, so null is a plain store.
    if (g_reset_hook_installed) {
        RVExtBridge::set_reset_hook(nullptr);
        g_reset_hook_installed = false;
    }
    g_khtl_cv.notify_all();
    g_khmw_cv.notify_all();
    g_khsm_cv.notify_all();
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // KH_POOL_SPLIT.
    kh_thread_stop_bounded(g_khtl_thr, 200);
    kh_thread_stop_bounded(g_khmw_thr, 200);
    // The running flags without their mutexes: a worker terminated inside its critical section leaves the mutex
    // locked forever, and nothing starts a worker after this point.
    g_khtl_running = false;
    g_khmw_running = false;
    // The pool: one budget for every worker, not one per worker.
    const uint64_t khpd_t0 = steady_now_ms();
    for (std::thread& khpd_t : g_khsm_thr) {
        const uint64_t khpd_el = steady_now_ms() - khpd_t0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_khsm_thr.clear();
    // The workers an earlier shutdown detached, on what is left of the same budget (one inside D3DCompile cannot be
    // told to stop).
    for (const HANDLE khpd_h : g_khsm_orphans) {
        const uint64_t khpd_el = steady_now_ms() - khpd_t0;
        if (WaitForSingleObject(khpd_h, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u) == WAIT_TIMEOUT) {
            TerminateThread(khpd_h, 0);
        }
        CloseHandle(khpd_h);
    }
    g_khsm_orphans.clear();
    g_khsm_active = false;
    g_khsa_state = 0;
    // The cloth pool, the same way. A worker woken with the stop set drains its own pool's queue - a solver worker one
    // step per queued cloth, a skin worker one skinning per queued skeletal mesh (KH_SKEL, KH_POOL_SPLIT) - and
    // returns; nothing queues after this point.
    const uint64_t khpd_c0 = steady_now_ms();
    for (std::thread& khpd_t : g_cloth_thr) {
        const uint64_t khpd_el = steady_now_ms() - khpd_c0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_cloth_thr.clear();
    g_cloth_thr_n.store(0u, std::memory_order_relaxed);   // KH_CLOTH_FORK.
    for (std::thread& khpd_t : g_skin_thr) {   // KH_POOL_SPLIT: the skin pool, on the same budget.
        const uint64_t khpd_el = steady_now_ms() - khpd_c0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_skin_thr.clear();
    g_skin_thr_n.store(0u, std::memory_order_relaxed);
}
}