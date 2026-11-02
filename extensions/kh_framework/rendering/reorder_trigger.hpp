#pragma once

// The injection half (rendering_integration_injection.hpp): the reorder trigger: the world tests, kh_reorder_trigger
// and reorder_pre_draw. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/inject_meshes.hpp (declared here for the uses below).
inline void inject_composited_meshes(ID3D11DeviceContext* ctx);

// The injection trigger's two world tests (kh_reorder_trigger, the one authority; KH_WSEE's capture takes them too:
// kh_wsee_pre_draw). Mistiming rejection: a real scene issues a wall of opaque draws between its depth clear and its
// translucent phase; late re-clears and preview/overlay passes do not - without this, such passes could pull the
// injection into a completed frame and paint the meshes over everything. With camera evidence - a projection found
// in this cycle's uploads, or a plausible one kept from an upload within the last 250 ms - the floor is an eighth of
// the full count; without it, the full count.
inline uint32_t kh_reorder_min_opaques() {
    const bool khr_floor_keep = g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
                                steady_now_ms() - g_slot_keep_ms < 250;
    const uint32_t khr_floor_rel = KH_REORDER_MIN_OPAQUE_DRAWS / 8;
    return (g_ro.engine_proj_valid || khr_floor_keep) ? khr_floor_rel : KH_REORDER_MIN_OPAQUE_DRAWS;
}
// Only a partition spanning the broad middle of [0, 1] is the world pass: a range starting deep is a far partition
// (distant terrain, clouds); one that never reaches the far side is the weapon/hands slice (rendered after the world
// - injecting there paints the mesh over the completed frame).
inline bool kh_reorder_world_vp(const D3D11_VIEWPORT& khrw_vp) {
    return !(khrw_vp.MinDepth > 0.3f || khrw_vp.MaxDepth < 0.7f);
}
// Continuous at the near plane, so routing on/off never pops a fragment.
static constexpr float KH_NEARZ_GAP_FRAC = 0.92f;

// KH_SCENE_NAME - the scene texture is named where it is known to be the scene's: at the injection's accept (the target
// it draws into) and at the scene resolve kh_main_depth_readopt recognises. Everything at the scene resolve keys on it
// - the render-thread flush (whose absence parks every Draw3D: flush_frame's khnp_ready), the late chain and the
// dynamic-light world pass. The bootstrap park adopts the depth identity without naming it, and a park after a
// resolution or FSAA change leaves the old target's name. Do not name it at the accept alone: a view with too few
// opaque draws to accept (sea or sky at a mission start) would then park every frame until one does. The identity and
// its format (KH_PIP_FMT) go together, and the naming counts as seeing it. Render thread.
inline void kh_scene_tex_name(ID3D11Resource* khsn_res) {
    g_topo_scene_tex_id = static_cast<void*>(khsn_res);   // Identity only, never dereferenced.
    kh_pip_scene_fmt_note(khsn_res);   // KH_PIP_FMT.
    g_topo_scene_seen_cycle = g_topo_cycles;
}

// No twin, one authority.
inline void kh_reorder_trigger(ID3D11DeviceContext* self) {
    if (!g_ro.dsv_main) return;

    // The opaque floor and the world viewport: kh_reorder_min_opaques / kh_reorder_world_vp (their notes).
    const uint32_t min_opaques = kh_reorder_min_opaques();
    UINT n_vp = 1;
    D3D11_VIEWPORT vp = {};
    bool khr_world_vp = false;
    auto khr_read_vp = [&]() -> bool {
        self->RSGetViewports(&n_vp, &vp);
        if (n_vp < 1) return false;
        khr_world_vp = kh_reorder_world_vp(vp);
        return true;
    };

    if (g_ro.opaque_draws < min_opaques) return;

    {
        if (!khr_read_vp()) return;

        if (!khr_world_vp) {

            if (!kh_rescue_window_open()) return;
        }
        // A world range is recorded below, once the draw's depth binding has verified as the main
        // depth (KH_TRIG_VP_VERIFIED).
    }

    if (g_ro.inject_attempts >= KH_REORDER_MAX_INJECT_ATTEMPTS) return;
    ++g_ro.inject_attempts;

    // Verification at each attempt: confirm the live depth binding is the main scene's before committing. On mismatch,
    // correct the tracking and retry on a later qualifying draw - standing down entirely would blank the meshes for the
    // whole frame over a transient.
    {
        ID3D11DepthStencilView* live_dsv = nullptr;
        self->OMGetRenderTargets(0, nullptr, &live_dsv);
        void* id = nullptr;

        if (live_dsv) {
            id = reorder_dsv_identity(live_dsv);
            live_dsv->Release();
        }

        if (!id || id != g_main_depth_identity) {
            g_ro.dsv_main = false;
            return;
        }
    }

    // The passing trigger's range is the encode range: the world's translucents depth-test through it against the
    // opaque-written depth, so the meshes must encode through it too. KH_TRIG_VP_VERIFIED: taken here, past the check
    // above - recorded at the viewport read, a draw whose depth binding was not the main depth (refused just above) or
    // one past the attempt cap (never checked) would set the range every depth reader of the cycle decodes the scene
    // with (kh_fill_depth_range_cb) from a pass that is not the scene's.
    if (khr_world_vp) {
        g_ro.trig_vp_min = vp.MinDepth;
        g_ro.trig_vp_max = vp.MaxDepth;
        g_ro.trig_vp_valid = true;
        // KH_SCENE_VP_TRIG: the scene range (the fallback of every depth reader without this cycle's trigger range)
        // is learned here alone - a world-shaped viewport on the verified main depth - and only as a range the
        // sanitisers accept (they fall back to it, so it must be one; NaN fails every term).
        if (vp.MinDepth >= 0.0f && vp.MaxDepth <= 1.0f && vp.MinDepth < vp.MaxDepth) {
            g_scene_vp_min_d = vp.MinDepth;
            g_scene_vp_max_d = vp.MaxDepth;
        }
    }

    g_ro.injected = true;

    // Frame-topology census: stamp the accept and latch the scene colour RT0 identity (the injection draws into it -
    // the one moment it is known to be bound). The latched identity is never dereferenced; KH_PIP_FMT reads the format
    // through the reference held here.

    {
        ID3D11RenderTargetView* kht_rtv = nullptr;
        self->OMGetRenderTargets(1, &kht_rtv, nullptr);

        if (kht_rtv) {
            ID3D11Resource* kht_res = nullptr;
            kht_rtv->GetResource(&kht_res);

            if (kht_res) {
                kh_scene_tex_name(kht_res);   // KH_SCENE_NAME.
                kht_res->Release();
            }

            kht_rtv->Release();
        }
    }

    g_ro.in_injection = true;
    inject_composited_meshes(self);
    g_ro.in_injection = false;
}

inline void reorder_pre_draw(ID3D11DeviceContext* self) {
    KH_PROF_SCOPE(KHP_HOOK_DRAW);   // KH_PROF.
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed)) {
        return;
    }
    if (!reorder_on_render_thread()) {   // Our own game-thread draws must not be tracked.

        // Our own flush draws are excluded by g_kh_flush_active (the parks' flushes - kh_park_run's and
        // flush_ui_frame's lock path - the late chain's and the in-front tail's: KhFlushActiveScope), the clear's own
        // draw by the injecting guard, and the whole path by the render-thread identity existing - during the cold
        // window every render-thread draw lands in this bail.
        if (!g_kh_flush_active && !g_ui_mask_injecting &&
            g_reorder_render_tid.load(std::memory_order_relaxed) != 0 &&
            g_ui_mask_wanted.load(std::memory_order_relaxed)) {
            kh_ui_mask_thread_draw(self);
        }

        return;
    }
    KhProfDsDecision khpd_dec(g_ro.in_injection);   // KH_PROF_DRAWSTEP: this draw's measurement decision.
    if (g_prof_ds_on) kh_fun(KHF_DRAWS, 1u);

    // KH_UI_MASK_RT: the coverage mask machine sees THIS thread's draws too. Measured (KH_UI_DIAG): the engine issues
    // most of its UI-pass draws on the render thread and only a few, plus Present, on the game thread; a machine fed
    // from the game thread alone saw ~6 of ~50 widget draws a frame, cleared the alpha at an arbitrary point against
    // the render thread's own UI draws, and the coverage at Present was whatever fell after it (poisoned to 255
    // whenever the render thread's scene quad came later). Under the Draw3D park the game thread's draws always trailed
    // the render thread's - an ordering the park supplied, never a contract. The machine's state is plain and safe
    // here, as for the whole hook family: the engine never uses the immediate context from two threads at once
    // (measured: the runtime's multithread protection is off - the KH_UI_DIAG verdicts). Gated past the 3D pass (no
    // main depth bound) so the target query is paid on UI draws alone; our own draws are excluded by in_injection / the
    // injecting flag.
    if (g_ui_mask_wanted.load(std::memory_order_relaxed) && !g_ro.dsv_main &&
        !g_ro.in_injection && !g_ui_mask_injecting.load(std::memory_order_relaxed) &&
        !g_kh_flush_active.load(std::memory_order_relaxed)) {
        kh_ui_mask_thread_draw(self);
    }

    // Deferred seam injection, render thread only and before this draw - the first volume draw, so the buffer still
    // carries our depth when the stencil counting reads it.
    {
        KH_PROF_SUB_ALL(KHP_DS_SEAM, g_prof_ds_on);   // KH_PROF_DRAWSTEP: timed whole.
        kh_volume_seam_pump(self);
    }

    if (g_pip.on && !g_ro.dsv_main && !g_ro.in_injection) {   // KH_PIP.
        KH_PROF_SUB_ALL(KHP_DS_PIP, g_prof_ds_on);   // KH_PROF_DRAWSTEP: timed whole.
        kh_pip_pre_draw(self);
    }

    // The mirror-pass survey, after the pump, armed only.
    if (g_svs_vol_dsv_now && !g_ro.in_injection) {
        {
            const uint32_t khmr_f = static_cast<uint32_t>(g_svs_frame_seq);
            // Frame-seq alone cannot separate them - both sessions share the pre-resolve seq.
            const bool khmr_same = g_svs_vol_src && g_svs_vol_src == g_vmir_prepass_src;
            // Only the prepass session may claim the frame marker - a secondary-session-first frame must not burn the
            // once-per-frame patch slot with a refused attempt.
            if (khmr_same && g_vmir_mark2 != khmr_f) {
                g_vmir_mark2 = khmr_f;
                g_vmir_frame_ok = false;
                if (g_vmir_prepass_stamp == khmr_f && g_vmir_dsv && g_vmir_b2_vs && g_vmir_b2_ps && g_vmir_b2_patch && g_vmir_b2_src_srv && g_vmir_b2_out_rtv) {
                    g_vmir_frame_ok = kh_vmir_patch_b2(self);
                }
            }
            g_vmir_pending = g_vmir_frame_ok && khmr_same;
        }
    }
    // KH_INFRONT: the view-model seam - the first hands-range draw on the volume buffer this window (the engine's hands
    // prepass, render target bound), ahead of it.
    if (g_svs_vol_dsv_bound && !g_ro.in_injection && !g_vm_seam_done &&
        g_infront_wanted.load(std::memory_order_relaxed) && !kh_shadows_off()) {   // KH_SHADOW_OFF: the world's rule.
        UINT khvq_n = 1;
        D3D11_VIEWPORT khvq_vp = {};
        self->RSGetViewports(&khvq_n, &khvq_vp);
        if (khvq_n >= 1 && kh_infront_vp_is_slice(khvq_vp)) {
            g_vm_seam_done = true;
            kh_rp_front_note(khvq_vp);   // KH_VOL_REPLAY: where in the pass replay B draws the in-front footprint.
            if (!kh_rp_seam_withhold()) {   // KH_SEAM_WITHHOLD: the hands count, as the world's.
                // KH_VOL_REPLAY: the pass stays replayable. Replay A lacks this footprint, so it differs from the copy
                // only where the footprint changed the count, and the merge keeps the copy there (per-pixel trust). Do
                // not refuse the pass instead: the next cycle is then unmerged, so its hands seam writes and refuses
                // again - with an in-front mesh, first person never gets back to a merged cycle.
                kh_infront_seam_inject(self, khvq_vp);
            }
        }
    }

    // One relaxed load on the active hot path. The clear-hook boundary machinery is not gated (once per frame; keeps
    // the render tid + PV latch warm so a re-spawn does not re-open a cold window).
    if (!g_kh_track_wanted.load(std::memory_order_relaxed)) return;

    // A main-scene draw issued while the current lighting block is live. Not
    // g_ro.opaque_draws - that one stops at the injection.
    if (!g_ro.in_injection && g_ro.dsv_main && (!g_ro.blend_translucent || !g_ro.dss_nowrite)) ++g_blko_draws;

    if (!g_ro.in_injection) {
        kh_band_stage_commit(self);   // Land staged seals before the injection.
        {
            KH_PROF_SUB(KHP_DS_SHADOW, g_prof_ds_on, g_prof_ds_t);   // KH_PROF_DRAWSTEP.
            shadow_note_draw(self);
        }
        {
            KH_PROF_SUB(KHP_DS_MASK, g_prof_ds_on, g_prof_ds_t);   // KH_PROF_DRAWSTEP.
            mask_note_draw(self);
        }
        mask_cast_engine(self);

        if (kh_dl_cpu_wanted()) {   // KH_DLF_DEMAND: a lit mesh or a caster, or a dynamicLightFog pass.
            // Every draw's cb11, not every 16th: the pool follows the engine at ~50 ms instead of riding its TTL. The
            // per-window dedupe and the arena cap bound the copies.
            KH_PROF_SUB(KHP_DS_DL, g_prof_ds_on, g_prof_ds_t);   // KH_PROF_DRAWSTEP.
            dynlights_sample_draw(self);
        }
    }
    // Hoisted above the guard: phase_on_atlas structurally excludes our own draws (the injection never runs inside the
    // atlas phase); the every-16th sample plus the freshness idle keep healthy sessions at zero cost.
    if (!g_ro.in_injection && g_ls.phase_on_atlas && ((++g_cascharv_ctr & 15) == 0)) {
        KH_PROF_SUB_ALL(KHP_DS_CASCBIND, g_prof_ds_on);   // KH_PROF_DRAWSTEP: one draw in 16, timed whole.
        cascbind_step(self);
    }

    // KH_INFRONT: the view-model slice comes after the world's injection, so this sits ahead of the injected-cycle
    // return. KH_SHOW_INFRONT: the verdict's demand, kept through a cycle that began armed (its verdict is published as
    // measured).
    if (!g_ro.in_injection && g_ro.dsv_main &&
        (g_vm_watch_wanted.load(std::memory_order_relaxed) || g_vm_watch_cycle ||   // A watched cycle runs to its end.
         g_vm_pre_injected) &&   // KH_INFRONT_PRE_COL: a prepass that injected is owed its colour pass.
        !g_kh_flush_active.load(std::memory_order_relaxed)) {
        KH_PROF_SUB_ALL(KHP_DS_INFRONT, g_prof_ds_on);   // KH_PROF_DRAWSTEP: timed whole.
        kh_infront_pre_draw(self);
    }
    // KH_VM_SEE: the view model's translucent draws, in an armed cycle (after the world's injection, so ahead of the
    // injected-cycle return as KH_INFRONT's). The cycle compare first: one compare per draw when unarmed.
    if (g_vmsee_want_cycle == g_topo_cycles && !g_ro.in_injection && g_ro.dsv_main && g_ro.blend_translucent &&
        !g_kh_flush_active.load(std::memory_order_relaxed)) {
        kh_vmsee_pre_draw(self);
    }
    // KH_WSEE: the world's blended draws, the late opaque ones and the colour draws that would taint the capture, in
    // a wanted cycle (ahead of the injected-cycle return, as KH_VM_SEE's: they come after the injection). The cycle's
    // verdict first: one bool test per draw when unwanted. A blended no-write draw before this cycle's injection may
    // be the injection's trigger (kh_reorder_trigger, below) - its turn comes after the trigger, so a capture it takes
    // holds our meshes.
    const bool khws_on = g_wsee_on && !g_ro.in_injection && g_ro.dsv_main &&
                         !g_kh_flush_active.load(std::memory_order_relaxed);
    const bool khws_after = khws_on && !g_ro.injected && g_ro.blend_translucent && g_ro.dss_nowrite;
    if (khws_on && !khws_after) {
        KH_PROF_SUB(KHP_DS_WSEE, g_prof_ds_on, g_prof_ds_t);   // KH_PROF_DRAWSTEP: sampled.
        kh_wsee_pre_draw(self);
    }

    if (g_ro.in_injection || g_ro.injected) return;

    if (!g_ro.blend_translucent || !g_ro.dss_nowrite) {
        if (g_ro.dsv_main) {
            // Retry until the first success, capped at the first 8 opaque draws
            // - every latch consumer runs after this point in the frame.
            if (g_ro.cycle_pv_stale && g_ro.opaque_draws < 8) {
                KH_PROF_SUB_ALL(KHP_DS_LATCH, g_prof_ds_on);   // KH_PROF_DRAWSTEP: the bridge's retry, timed whole.
                RVExtBridge::ProjectionViewTransform khl_pv = {};

                // KH_LATCH_FOLD: no continuity gate (the clear has none either).
                if (RVExtBridge::get_projection_view_transform(khl_pv)) {
                    // The fresh view is what every consumer needs; the kept-previous projection stays.
                    if (g_ro.cycle_pv_valid) {
                        memcpy(&khl_pv.projection[0][0], &g_ro.cycle_pv.projection[0][0],
                               sizeof(khl_pv.projection));
                    }

                    g_ro.cycle_pv = khl_pv;
                    g_ro.cycle_pv_valid = true;
                    g_ro.cycle_pv_stale = false;
                }
            }

            ++g_ro.opaque_draws;
            ++g_ro.opaques_since_inject;   // flush_locked's repaint term reads it.

            if (g_ro.slot_near_live > 0.0f &&
                (!g_ro.world_pair_valid ||
                 g_ro.slot_m22 != g_ro.world_m22 ||
                 g_ro.slot_m32 != g_ro.world_m32) &&
                g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
                g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
                kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX) {
                g_ro.world_m22 = g_ro.slot_m22;
                g_ro.world_m32 = g_ro.slot_m32;
                g_ro.world_pair_valid = true;
            }

            // Sky binding probe, opaque-phase sampling: every 8th draw (the
            // fog-material draws later in the colour pass are never sampled).
            if ((g_ro.opaque_draws & 7) == 0 &&
                ((g_fog_valid && g_fog[0] > 1e-4f) ||
                 (!g_ls.view_src_valid && shadow_live_wanted()))) {
                KH_PROF_SUB_ALL(KHP_DS_SKYBIND, g_prof_ds_on);   // KH_PROF_DRAWSTEP: one opaque draw in 8, timed whole.
                skybind_step(self);
            }

            if ((g_ro.opaque_draws & 15) == 0) {
                UINT n_vp = 1;
                D3D11_VIEWPORT vp = {};
                self->RSGetViewports(&n_vp, &vp);

                if (n_vp >= 1) {
                    // An injected cycle returned above: a depth-range change renews only the attempt budget.
                    if (g_ro.cycle_vp_valid &&
                        (fabsf(vp.MinDepth - g_ro.cycle_vp_min) > 0.002f ||
                         fabsf(vp.MaxDepth - g_ro.cycle_vp_max) > 0.002f)) {
                        g_ro.inject_attempts = 0;
                    }

                    g_ro.cycle_vp_min = vp.MinDepth;
                    g_ro.cycle_vp_max = vp.MaxDepth;
                    g_ro.cycle_vp_valid = true;
                }
            }
        }

        return;
    }

    kh_reorder_trigger(self);
    // KH_WSEE: the trigger candidate's turn (khws_after, above), past the injection it may have drawn - the trigger
    // may also have found the bound depth not the main one (dsv_main down: nothing to take).
    if (khws_after && g_ro.dsv_main && !g_ro.in_injection) {
        KH_PROF_SUB(KHP_DS_WSEE, g_prof_ds_on, g_prof_ds_t);   // KH_PROF_DRAWSTEP: sampled.
        kh_wsee_pre_draw(self);
    }
}
}