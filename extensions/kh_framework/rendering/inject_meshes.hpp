#pragma once

// The injection half (rendering_integration_injection.hpp): the injection site (inject_composited_meshes) and its haze
// lanes. Never include it on its own.
namespace RenderIntegration {

// Engine distance haze fill (one truth; the injection, the flush's kh_fx_frame_template and the world pass's
// kh_dlsw_fog_lanes call it). Modes 1 and 3 share the case; any other mode is one this code has never seen and it
// stands down.
inline void kh_fill_haze(ConstantData& khz_cb) {
    khz_cb.haze_pars[0] = 0.0f;
    khz_cb.haze_pars[1] = 0.0f;
    khz_cb.haze_pars[2] = 0.0f;
    khz_cb.haze_pars[3] = 0.0f;
    khz_cb.fog_sky_col[3] = 0.0f;

    // KH_PROBE_META40: the light lanes below (nb[44], nb[50], nb[53]) sit at those indices only in a mirror whose
    // lock has the full 40-float head (meta == 40), the test the fog colour and engine terms take.
    if (g_sky_probe.hits == 0 || g_light_probe.hits == 0 || g_light_probe.meta != 40) return;

    // fog-model selector: the lane holds an integer, reinterpreted.
    uint32_t khz_mode = 0;
    memcpy(&khz_mode, &g_light_probe.nb[44], sizeof(khz_mode));
    if (khz_mode != 1u && khz_mode != 3u) {  return; }

    const float khz_ref = g_sky_probe.nb[56];
    const float khz_den = g_sky_probe.nb[57];
    const float khz_fal = g_sky_probe.nb[58];

    // Finite, positive, and inside the range an atmosphere can occupy. Absent beats wrong.
    if (!(khz_den > 0.0f && khz_den < 1.0f) ||
        !(khz_fal > 1.0e-6f && khz_fal < 1.0e-1f) ||
        !(khz_ref > -1.0e4f && khz_ref < 1.0e4f)) {  return; }

    khz_cb.haze_pars[0] = khz_ref;
    khz_cb.haze_pars[1] = khz_den;
    khz_cb.haze_pars[2] = khz_fal;
    khz_cb.haze_pars[3] = 1.0f;
    // The layer the integral branches on, carried in the spare colour lane.
    khz_cb.fog_sky_col[3] = g_light_probe.nb[50] + g_light_probe.nb[53];
}

// No owner prepass: the KH_ARB_DEPTH variant writes SV_Depth, which disables early-Z, so LESS_EQUAL cannot order two of
// our meshes against each other on far-arb frames. The software depth prepass was retired: its rasterization into a
// clone of the MSAA engine DSV cost more than any overdraw it saved (KH_VIS_OCC already culls the fully hidden
// majority). Do not restore it unless it is armed on measured overlap, never on a mesh count. Conservative depth output
// cannot help (the clamp pulls NEARER). What remains: ordinary overdraw, and on far-arb frames a mesh whose clamp
// pulled it further than a neighbour's can win a pixel it is behind (both within KH_FAR_ARB_CONTACT_H of the terrain,
// both behind it, overlapping). If it shows, bound the clamp so our meshes cannot reorder each other.
// KH_SESSION_RESET: at file scope so the session reset reaches it.
static bool g_inj_arb_prev_armed = false;   // Injection site is render-thread-only.
inline void inject_composited_meshes(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_INJECT);
    KH_GPU_SCOPE(ctx, KHG_INJECT);   // KH_PROF.
    if (kh_fsaa_world_standdown()) {  return; }

    const float snapshot_now = effect_time_seconds();

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) {
        g_mask.cold_first_trigger = static_cast<float>(snapshot_now - g_mask.cold_t0);
    }
    static std::vector<RenderObject> meshes;
    meshes.clear();
    static std::vector<RenderObject> khr_occx;   // KH_OCC_FORCE: declared occluders this pass does not draw.
    khr_occx.clear();
    static std::vector<uint32_t> khr_cand;   // KH_SCENE: grid pre-cull candidates.
    static std::vector<uint32_t> khr_elig;   // KH_SCENE: every eligible slot, pre-cull independent.
    khr_elig.clear();
    // The eligible set is walked twice - once whole (the "anything eligible" verdicts, the near-clearance minimum -
    // that list taken again after the replay's late syncs when the scene moved, KH_OBJBUF_LATE), once through the grid
    // so the copies below are only of objects whose cell can meet this cycle's frustum. An injection with every
    // eligible mesh off-screen still lands (serial + stats), which the flush's stand-down keys on.
    uint32_t khr_elig_n = 0;
    bool     khc_any_caster = false;
    {
        // KH_ATTACH: placed HERE, immediately before the sync that consumes it, on the thread that is about to draw. Do
        // not move it earlier: measured, the game thread wrote a newer scene mid-frame on 51.4% of injections (752 of
        // 1,464, never more than one generation) - a coin flip between this frame's transform and the next one's, which
        // is jitter. At the consume point there is no window: the injection draws what this step just placed, from the
        // cycle's Draw3D sample of the frame the engine is drawing (kh_gts_for_cycle).
        {
            KhFinalStepScope khfs_final;   // KH_ATTACH_FINAL: the last step that can still change this cycle's draw.
            kh_attach_step(ctx);   // KH_SKEL_DRAW: this step skins the skeletal bindings too.
        }
        kh_uvs_step(ctx);   // KH_USER_VS: a no-op for what the cast fire's step evaluated, but for a source this step just re-posed.
        if (g_sun_restep_cycle == g_topo_cycles) g_sun_map_rendered_frame = false;   // KH_SUN_RESTEP: rebuild from this step's poses.
        // KH_MESH_GEN_PUB: a pending publish (a generated mesh, an import - KH_EA_FBX_NO_PARK) lands here, before the
        // sync that would hold the objects naming it (KH_SCENE_UNPUB) - so a new or rebuilt mesh draws this cycle,
        // with the pose this step just placed. The render thread, between our draws: the flush's own window (its
        // housekeeping lands the same way), and ensure_resources below tops up the same buffers here on every
        // injection that draws.
        kh_mesh_publish_land(RVExtBridge::get_d3d_device());
        kh_scene_sync();
        kh_objbuf_sync(ctx);   // KH_OBJBUF: the records this pass's buckets index.
    }
    // KH_OBJBUF_LATE: the scene generation this list was taken at, and the test it was taken by - the list is taken
    // again after the replay's late syncs when the scene moved on.
    const auto khr_elig_gen = g_scene.gen;
    auto khr_eligible = [&](const RenderObject& o) {
        if (!kh_obj_shown(o) || !is_composite_eligible(o)) return false;   // KH_SHOW_INFRONT.
        bool expired = false;
        const float env = lifetime_envelope(o, snapshot_now, expired);
        if (expired) return false;   // The Draw3D flush owns the erasure.
        // Translucents never take the mid-frame slot. Measure before extending the SV_Depth clamp to PSMain.
        return o.blend_mode == 0 && o.color[3] * env >= 0.999f;
    };

    {
        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& o = g_scene.objs[khsc_i];

            if (kh_shadow_active(o)) khc_any_caster = true;   // KH_SHADOW_ACTIVE: casterOnly counts.
            if (!khr_eligible(o)) continue;
            ++khr_elig_n;
            khr_elig.push_back(khsc_i);
        }

        if (khr_elig_n > 0 && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_stage < 0.0f) {
            g_mask.cold_first_stage = static_cast<float>(snapshot_now - g_mask.cold_t0);
        }
    }

    // The path is alive even when the eligible set is momentarily empty: stamping here keeps the flush from flapping
    // over.
    g_composite_last_inject_ms.store(steady_now_ms(), std::memory_order_relaxed);
    g_wsee_inj_elig = khr_elig_n > 0;   // KH_WSEE: the capture gate's eligibility (this walk's verdict).
    if (khr_elig_n == 0) {
        if (!khc_any_caster) {
            // KH_DL_TRIGGER_HARVEST: nothing to draw, but the dynamic-light pool is still harvested at this trigger,
            // inside the main pass, as an injection harvests it (dynlights_acquire, below). Do not harvest it at the
            // scene flush (KH_DL_FALLBACK_HARVEST) instead: that span starts after the frame's resolve, its routine
            // windows (the span's first uploads) are other passes' lists than the main pass's after the trigger, and a
            // light the latter alone carries is re-sighted and not rewritten - a flashlight froze at some angles until
            // one of our meshes was in the scene (the main pass's variant was all but never placed). An anomalous cycle
            // is the injection's (khr_anomalous below): re-armed for a later partition while the rescue window is shut,
            // a rescue harvests nothing. Unlike an injection with a mesh near the camera (KH_PART_NEAR), a far
            // partition's trigger is not refused - as with our meshes farther off. The view is the cycle's
            // (dynlights_acquire keeps it for the trigger mirror only).
            if (g_proj_locator_ever && g_ro.slot_near_live <= 0.0f) {
                if (!kh_rescue_window_open()) {
                    g_ro.injected = false;
                    g_ro.inject_attempts = 0;
                }
                return;
            }
            if (g_ro.cycle_pv_valid) dynlights_acquire(ctx, g_ro.cycle_pv.view);
            return;
        }
    }
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) {  return; }
    if (!kh_ensure_ok("injection resources", ensure_resources(dev))) {  return; }

    kh_dls_select();   // KH_DL_SHADOW twin 1/2 - ahead of the sun, never inside it.
    // KH_SUN_GEN: the syncs above (this step's, kh_dls_select's) took a scene the cast fire's ladder was not built
    // from - rebuild it from what this pass draws (over standing maps only: none, nothing to redraw).
    if (g_sun_map_valid && g_scene.gen_cast != g_sun_map_gen && kh_sun_gen_reopen_ok()) {   // KH_SUN_GEN_CAP.
        g_sun_map_rendered_frame = false;
    }
    render_sun_depth(ctx);
    kh_dls_frame(ctx);   // KH_DLS_FRAME twin 1/2 - after the sun, never inside it.
    kh_rp_replay(ctx);   // KH_VOL_REPLAY: before the meshes bind t24 / t33.
    // KH_OBJBUF_LATE: the replay re-syncs the scene (KH_RP_PRESYNC and kh_infront_seam_inject, which upload no
    // records; kh_volume_seam_inject), so a stage written since the sync above is in g_scene but not yet in the
    // records the bucket draws index (t39) - one frame at the old pose, or a zero / a dead object's record in a new
    // or reused slot. O(dirty); nothing when nothing moved. The eligible list the near-clearance test walks is taken
    // again with them: an object the late sync added, removed or moved into a freed slot is measured as it is drawn.
    kh_objbuf_sync(ctx);
    if (g_scene.gen != khr_elig_gen) {
        khr_elig.clear();
        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (g_scene.alive[khsc_i] && khr_eligible(g_scene.objs[khsc_i])) khr_elig.push_back(khsc_i);
        }
        // The count the landing stamp reads (below): the set this pass draws from, as the list now is.
        khr_elig_n = static_cast<uint32_t>(khr_elig.size());
    }

    RVExtBridge::ProjectionViewTransform pv = {};

    if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;

        RVExtBridge::ProjectionViewTransform live = {};

        if (RVExtBridge::get_projection_view_transform(live)) {
            // Render thread throughout: the boundary camera is written on this same thread.
            float khlp_lc[3], khlp_vc[3];
            kh_cam_of(pv.view, khlp_lc);
            kh_cam_of(live.view, khlp_vc);
            const float khlp_d2 = latch_cam_dist_sq(khlp_lc, khlp_vc);

            // A cycle whose clear-time fetch succeeded keeps its own sample (pv already); the mid-frame live fetch is
            // only the fallback for a cycle whose boundary fetch failed.
            if (khlp_d2 > KH_LATCH_JUMP_M * KH_LATCH_JUMP_M && !g_boundary_pv_valid) {
                memcpy(&pv.view[0][0], &live.view[0][0], sizeof(pv.view));
            }
            const float m22_latch = pv.projection[2][2];
            const float m32_latch = pv.projection[3][2];
            const float m22_live = live.projection[2][2];
            const float m32_live = live.projection[3][2];
            const float n_latch = (fabsf(m22_latch) > 1e-9f) ? (-m32_latch / m22_latch) : 0.0f;
            const float n_live  = (fabsf(m22_live)  > 1e-9f) ? (-m32_live  / m22_live)  : 0.0f;

            // On disagreement the latch is the suspect: on sky-heavy frames it belongs to a partition/aux pass whose
            // huge near plane slices the injected mesh.
            if (fabsf(n_latch - n_live) > 1e-4f ||
                (kh_enc_far(m22_latch, m32_latch) > KH_ENC_FAR_MAX &&
                 kh_enc_far(m22_live, m32_live) <= KH_ENC_FAR_MAX)) {
                pv.projection[2][2] = m22_live;
                pv.projection[3][2] = m32_live;
            }

            // The live fetch's scale terms are frame k+1's and the cycle latch's are frame k's; the seam has taken the
            // live pair, so the footprint and the visible box must rasterise at the same zoom or they diverge radially
            // by the per-frame FOV delta.
            const float khcf_lx = g_ro.cycle_pv.projection[0][0];
            const float khcf_ly = g_ro.cycle_pv.projection[1][1];
            const float khcf_sx = live.projection[0][0];
            const float khcf_sy = live.projection[1][1];
            bool  khcf_ok = khcf_sx == khcf_sx && khcf_sy == khcf_sy &&
                            khcf_lx > 1.0e-6f && khcf_ly > 1.0e-6f &&
                            khcf_sx > 1.0e-6f && khcf_sy > 1.0e-6f;

            if (khcf_ok) {
                const float khcf_rx = khcf_sx / khcf_lx;
                const float khcf_ry = khcf_sy / khcf_ly;
                const float khcf_hi = khcf_rx > khcf_ry ? khcf_rx : khcf_ry;
                // Aspect preserved, to 1 pct of the larger ratio.
                if (fabsf(khcf_rx - khcf_ry) > 0.01f * khcf_hi) khcf_ok = false;
                // And inside the foreign-window line, both ways.
                if (khcf_hi > 4.0f || khcf_hi < 0.25f) khcf_ok = false;
            }

            if (khcf_ok) {
                pv.projection[0][0] = khcf_sx;
                pv.projection[1][1] = khcf_sy;
            }
        }
    } else if (RVExtBridge::get_projection_view_transform(pv)) {
        g_last_pv = pv;
        g_last_pv_ms = steady_now_ms();
    } else if (g_last_pv_ms != 0 && steady_now_ms() - g_last_pv_ms < 250) {
        pv = g_last_pv;
    } else {
        return;   // No usable matrices anywhere this frame.
    }

    float khav_latch_view[16];
    memcpy(khav_latch_view, &pv.view[0][0], sizeof(khav_latch_view));
    // Do not adopt the seam's rotation rows here: at altitude the seam pass runs before the cycle's clear and its view
    // is the previous frame's, and adopting it hands every mesh a one-frame rotation error (measured 4.8 deg at mouse
    // speed, distance-scaled). The pass draws with the cycle's own view; the seam's mask may trail a frame at its edge.
    kh_adopt_frame_view(pv);

    // The meshes then rasterize with the true dynamic near plane of the frame being rendered - the one scalar the
    // bridge can never pin down - and the guard reconstructs scene distance with the same truth.
    const bool measured = g_ro.engine_proj_valid;

    // State snapshot: injection decisions read the telltale once. The render thread's upload and pre-draw hooks write
    // this state; the snapshot keeps every decision below self-consistent.
    const float snap_slot_near = g_ro.slot_near_live;
    const float snap_slot_m22 = g_ro.slot_m22;
    const float snap_slot_m32 = g_ro.slot_m32;
    const bool  snap_world_valid = g_ro.world_pair_valid;
    const float snap_world_m22 = g_ro.world_m22;
    const float snap_world_m32 = g_ro.world_m32;

    // KH_PART_NEAR: the camera's own near - the cycle's camera (the clear's latch, its pair checked against the
    // live one above), before any pair below replaces it: the partition our meshes belong in.
    const float khr_cam_near = fabsf(pv.projection[2][2]) > 1e-9f
                             ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;

    if (measured) {
        pv.projection[2][2] = g_ro.engine_m22;
        pv.projection[3][2] = g_ro.engine_m32;
    }

    const bool khr_keep_fresh = g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
                                steady_now_ms() - g_slot_keep_ms < 250;

    const bool khr_slot_band = snap_slot_near >= KH_INJ_SLOT_NEAR_MIN;
    const bool khr_slot_sane_far = kh_enc_far(snap_slot_m22, snap_slot_m32) <= KH_ENC_FAR_MAX;
    bool khr_far_phase = false;   // Far pair is this phase's truth (set at both far branches below).

    if (snap_world_valid) {
        // Parity by construction: the pair the world opaques we depth-test against last rendered under.
        pv.projection[2][2] = snap_world_m22;
        pv.projection[3][2] = snap_world_m32;
    } else if (snap_slot_near > 0.0f && khr_slot_band && khr_slot_sane_far) {
        pv.projection[2][2] = snap_slot_m22;
        pv.projection[3][2] = snap_slot_m32;
    } else if (khr_keep_fresh) {
        const float khr_pv_near = fabsf(pv.projection[2][2]) > 1e-9f
                                ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool khr_pv_in_band = khr_pv_near >= KH_CAM_NEAR_MIN && khr_pv_near <= KH_CAM_NEAR_MAX;
        const bool khr_keep_agrees = khr_pv_in_band &&
            fabsf(khr_pv_near - g_slot_keep_near) <= 0.25f * khr_pv_near;

        if (khr_keep_agrees || !khr_pv_in_band) {
            pv.projection[2][2] = g_slot_keep_m22;
            pv.projection[3][2] = g_slot_keep_m32;
        }
    }

    // A following camera partition still gets its own injection via the partition-change re-arm.
    // KH_PART_NEAR: judged on the near this injection would draw with - the pair just taken - against the camera's. Do
    // not judge it on the slot: the world-phase latch keeps the last opaque draw's pair, which can be a far partition's
    // (a view with no object near the camera: 10 m, measured) while the slot has already moved on to the camera's;
    // judged on the slot, the rule passed and the injection drew with the far near, a near mesh wholly in the near-gap
    // in front of everything.
    const float khr_eff_near = fabsf(pv.projection[2][2]) > 1e-9f
                             ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    if (snap_slot_near > 0.0f && khr_cam_near > 1e-4f &&
        kh_enc_far(pv.projection[2][2], pv.projection[3][2]) <= KH_ENC_FAR_MAX &&   // Celestial-class: never.
        khr_eff_near > 1.5f * khr_cam_near) {
        float khr_cam[3];
        kh_cam_of(pv.view, khr_cam);
        float khr_min_clear = 1e9f;

        for (uint32_t khr_slot : khr_elig) {   // KH_SCENE: every eligible mesh, pre-cull independent.
            // KH_NEARZ_BOUNDS: the nearest the box around everything the mesh may draw comes to the camera
            // (kh_bounds_size_of: a cloth's reach, a decimated level, a vertex stage's reach, the residual; a
            // scaled rotation) - the script's size alone passed such a mesh as clear, drawn in the near-gap.
            const float d = sqrtf(kh_bounds_dist_sq(g_scene.objs[khr_slot], khr_cam));

            if (d < khr_min_clear) khr_min_clear = d;
        }

        if (khr_min_clear < 1.5f * khr_eff_near) {
            // A repaint-invoked call landing here gets its one-shot refunded,
            // so a genuine repaint later the same cycle still fires.
            g_ro.injected = false;   // Stay armed for the camera partition.
            g_ro.inject_attempts = 0;
            return;
        }

        khr_far_phase = true;   // Arms the analytic arbitration below.
    }

    // Anomalous cycle: a proven locator with a silent slot means the engine is
    // rendering a pass that never published a camera-class projection.
    const bool khr_anomalous = g_proj_locator_ever && snap_slot_near <= 0.0f;

    if (khr_anomalous) g_ro.anomaly_seen = true;   // Flush carries this frame.

    if (khr_anomalous) {
        // Failure direction: an even-later partition erases it again - never a new artifact on healthy frames.
        if (!kh_rescue_window_open()) {
            g_ro.injected = false;
            g_ro.inject_attempts = 0;
            return;
        }
        // Rescue proceeds: khr_anomalous stays true, so the view-source supervisor quarantine below holds and the
        // end-of-draw bookkeeping counts a rescue, not an injection.
    }

    const float inject_near = fabsf(pv.projection[2][2]) > 1e-9f
                            ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;

    // Below the far-phase and anomalous skip returns, so an aborted attempt never re-publishes over a real landing's
    // pair; re-published by any later proceeding attempt this frame, so the flush carries the last painter's encode.
    g_inj_enc_m22 = pv.projection[2][2];
    g_inj_enc_m32 = pv.projection[3][2];
    g_inj_enc_near = inject_near;
    g_inj_enc_ms = steady_now_ms();
    memcpy(g_inj_view, &pv.view[0][0], sizeof(g_inj_view));
    g_inj_view_ms = g_inj_enc_ms;


    if (inject_near >= KH_FAR_ARB_NEAR_MIN ||
        (g_inj_arb_prev_armed && inject_near >= KH_FAR_ARB_NEAR_HOLD)) {
        khr_far_phase = true;
    }

    // Accepted injections only. Rescue cycles are quarantined: a foreign pass's
    // slot bindings are exactly the poison the CPU validation keeps out.
    if (!khr_anomalous) dynlights_acquire(ctx, pv.view);

    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    // Supervise the view source: lock the upload location whose contents equal this frame's pre-adoption latch
    // (khav_latch_view - not the adopted view the meshes draw with: a wrong lock must not self-certify through its own
    // publishes). Never on anomalous cycles - a supervisor taught by them mis-locks (world-sliding shadows).
    if (!khr_anomalous) {
        const float* truth = khav_latch_view;
        const uint32_t vcn = g_ls.vc_n < 16 ? g_ls.vc_n : 16;
        // Age-bounded so a matrix harvested before the camera moved cannot sit here winning.
        const bool khrb_ok = kh_ring_best_fresh();   // KH_RING_BEST_AGE.

        if (!g_ls.view_src_valid && (vcn > 0 || khrb_ok)) {
            // Argmin, not first-passing: pick the closest candidate (by its rotation's error) and report its error.
            int best_i = -1, best_k = 0;
            float best_rot = 1e9f;   // KH_DDX_BEST_TR: the rotation's error alone.

            for (uint32_t i = 0; i < vcn; ++i) {
                const float* m = g_ls.vc_ring[i].m;

                for (int orient = 1; orient <= 2; ++orient) {
                    float e_rot = 0.0f;

                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            const float mv = orient == 1 ? m[r * 4 + c] : m[c * 4 + r];
                            e_rot += fabsf(mv - truth[r * 4 + c]);
                        }
                    }

                    if (e_rot < best_rot) {
                        best_rot = e_rot;
                        best_i = static_cast<int>(i);
                        best_k = orient;
                    }
                }
            }

            // Copied into ring slot 0 so every downstream consumer (convention classification, tmag gate, cold confirm,
            // adoption) runs on the identical code path.
            if (khrb_ok) {
                float khrb_e = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        const float khrb_v = (g_ring_best_orient == 1)
                            ? g_ring_best_m[r * 4 + c] : g_ring_best_m[c * 4 + r];
                        khrb_e += fabsf(khrb_v - truth[r * 4 + c]);
                    }
                }

                if (best_i < 0 || khrb_e < best_rot) {
                    g_ls.vc_ring[0].res =
                        static_cast<ID3D11Resource*>(const_cast<void*>(g_ring_best_res));
                    g_ls.vc_ring[0].off = g_ring_best_off;
                    memcpy(g_ls.vc_ring[0].m, g_ring_best_m, 64);
                    best_i = 0;
                    best_k = static_cast<int>(g_ring_best_orient);
                    best_rot = khrb_e;
                }
            }

            // Lock bar: the cold lock has no reference and accepts 0.05; re-locks must beat 0.02.
            const float lock_bar = g_ls.view_locks == 0 ? 0.05f : 0.02f;

            const bool exact_lock = best_rot < 1e-3f;

            if (best_i >= 0 && best_rot < lock_bar && !exact_lock &&
                g_ls.view_wait_since <= 0.0f) {
                g_ls.view_wait_since = effect_time_seconds();
            }

            const bool wait_expired = g_ls.view_wait_since > 0.0f &&
                effect_time_seconds() - g_ls.view_wait_since > 2.0f;

            if (best_i >= 0 && best_rot < lock_bar && (exact_lock || wait_expired)) {
                g_ls.view_wait_since = 0.0f;
                // Convention classification.
                const float* m = g_ls.vc_ring[best_i].m;
                float tmag = 0.0f;

                for (int c = 0; c < 3; ++c) {
                    const float tv = best_k == 1 ? m[12 + c] : m[c * 4 + 3];
                    tmag += fabsf(tv);
                }

                const bool relative = tmag < 10.0f;

                // Re-locks keep the tighter bar and skip the cold double-confirmation (a lock dropped mid-flight must
                // not wait an extra frame); the exact-or-2 s wait above still applies to them.
                bool cold_confirmed = true;

                if (g_ls.view_locks == 0) {
                    cold_confirmed = g_ls.vc_ring[best_i].res == g_coldlock_cand_res &&
                                     g_ls.vc_ring[best_i].off == g_coldlock_cand_off;
                    g_coldlock_cand_res = g_ls.vc_ring[best_i].res;
                    g_coldlock_cand_off = g_ls.vc_ring[best_i].off;
                }

                if (relative && cold_confirmed) {
                    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
                    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
                    g_ls.view_src_orient = best_k;
                    g_ls.view_src_relative = true;
                    g_ls.view_src_valid = true;
                    kh_view_lock_acquired();
                    g_ring_best_valid = false;

                    g_ls.view_src_miss = 0;
                    g_ls.pub_ok_since_lock = false;
                    g_ls.view_locks++;
                    g_lock_churn_ms_v = steady_now_ms();   // lock-settle: fresh lock, hold the cast a beat.
                    g_ls.wipe_deferred = true;
                }
            }
        }

        g_ls.vc_n = 0;   // Fresh ring per injection: stale entries can't win.

        if (g_ls.view_src_valid) {
            // Health: rotation agreement is the signal (the hybrid's translation is bridge-derived by construction).
            bool healthy = false;

            if (g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f) {
                float e_rot = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        e_rot += fabsf(g_ls.frame_view[r * 4 + c] - truth[r * 4 + c]);
                    }
                }

                // The publish-side family filter still guards at 0.35.
                healthy = e_rot < 0.20f;
            }

            if (healthy) {
                g_ls.view_src_miss = 0;
            } else if (++g_ls.view_src_miss > 90) {
                g_view_drop_path = 2;
                g_view_relock_wide_cycle = 0;
                g_ring_best_valid = false;
                g_ls.view_src_valid = false;
                g_ls.vc_n = 0;
            }
        }
    }
    float cam[3];

    // The camera through the honest inverse of the view the pass draws with, never the transpose.
    kh_cam_of(pv.view, cam);
    const bool khcs_engcam = kh_engcam_consume(pv.view, cam);
    g_ls.cam[0] = cam[0];
    g_ls.cam[1] = cam[1];
    g_ls.cam[2] = cam[2];
    g_ls_cam_cycle = g_topo_cycles;   // KH_FOG_PROBE.

    // The grid pre-cull, against the view this pass will draw with (pv.view is final here) and the same cam the precise
    // test below is built from. The eligibility predicate is the staging walk's, against the same snapshot_now. An
    // invalid cull set returns every cell.
    {
        KhCullSet khr_pre;
        float khr_pre_vp[4][4];
        mul_4x4(pv.view, pv.projection, khr_pre_vp);
        kh_cull_build(khr_pre_vp, cam, khr_pre);
        kh_scene_grid_query(khr_pre, khr_cand);

        for (uint32_t khr_slot : khr_cand) {
            if (!g_scene.alive[khr_slot]) continue;
            const RenderObject& o = g_scene.objs[khr_slot];
            bool expired = false;
            // KH_OCC_FORCE: a declared occluder this pass will not draw still
            // hides (the cull's second list) while it lives. KH_SHOW_INFRONT: khr_eligible's test.
            if (!kh_obj_shown(o) || !is_composite_eligible(o)) {
                if (o.occluder && !o.fullscreen) {
                    (void)lifetime_envelope(o, snapshot_now, expired);
                    if (!expired) khr_occx.push_back(o);
                }
                continue;
            }
            const float env = lifetime_envelope(o, snapshot_now, expired);
            if (expired) continue;
            if (!(o.blend_mode == 0 && o.color[3] * env >= 0.999f)) {
                if (o.occluder) khr_occx.push_back(o);
                continue;
            }
            meshes.push_back(o);
            meshes.back().color[3] *= env;
        }
    }

    // Our meshes in kh_mesh_order's order by the camera's distance to each
    // object's world bounds: opaque solids front to back, the rest back to front.
    {   // KH_MESH_ORDER: key sort, same order, one distance per object.
        kh_mesh_order(meshes, cam, g_dist_memo_inj);   // The memo begins here, seeded; the route verdicts,
                                                       // the emit, the pick and the parts read it.
    }
    // KH_REPLAY_SCISSOR: the merged textures are this pass's where the replay drew for these meshes (the replay's own
    // world seam re-syncs the scene).
    if (g_rp_mir_pend) {   // KH_MIR_REPLAY: this pass reads the mirror if a mesh of it does, under its own near.
        for (const RenderObject& o : meshes) {
            if (kh_rp_mir_reads(o, pv.projection, cam)) { kh_rp_mir_finish(ctx); break; }
        }
    }
    bool khr_rp_vol = false, khr_rp_mir = false;
    kh_rp_covered(meshes, khr_rp_vol, khr_rp_mir);
    KhRpPassScope khr_rp_scope(khr_rp_vol, khr_rp_mir);

    StateBackup backup;
    backup.capture(ctx);
    KhBindScope khr_bc(ctx);   // KH_BIND_FILTER.
    KhOmSlot0 khr_s0;   // KH_MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    khr_s0.open(ctx);
    kh_objbuf_bind(ctx);   // KH_OBJBUF: VS t39 for every bucket draw (restored with the backup).

    // The meshes' stored depth must be encoded through the same range the surrounding scene geometry used, or every
    // comparison skews - worst for near fragments, where a min-depth offset is a huge world-space error.
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&n_saved_vp, saved_vp);

    D3D11_VIEWPORT khr_pass_vp = {};   // near-gap: filled below.
    {
        D3D11_VIEWPORT vp = {};

        if (n_saved_vp >= 1) {
            vp = saved_vp[0];
        } else {
            vp.Width = static_cast<FLOAT>(g_main_depth_w);
            vp.Height = static_cast<FLOAT>(g_main_depth_h);
        }

        vp.MinDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
        vp.MaxDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
        ctx->RSSetViewports(1, &vp);
        khr_pass_vp = vp;   // near-gap: the world-range viewport.
    }   // pass-scope for the per-draw gap swap.

    // KH_SSAO: the pass's resources and lanes, before the first of our draws; the post pass runs after the last
    // depth-writing one (ahead of the parts tail, which writes no depth and must not take the solids' term). pv and
    // khr_pass_vp are what the solids below draw with.
    KhSsaoPass khr_ssao;
    if (kh_ao_strength() > 0.0f || kh_nzm_wanted()) {   // KH_NEARZ_MARK: the marker needs the mark too.
        kh_ssao_rect(pv, khr_ssao.rect);
        kh_ssao_pre(dev, ctx, khr_ssao, pv.projection, khr_pass_vp.MinDepth, khr_pass_vp.MaxDepth);
    }

    const uint64_t khs_inj_now = steady_now_ms();
    const bool khs_snap_ok = g_res.comp_depth_srv && g_res.comp_depth_samples == 1 &&
                             g_snap_serial != 0 && g_snap_ms != 0 &&
                             (khs_inj_now - g_snap_ms) < KH_SNAP_MAX_AGE_MS;
    const bool guard = khs_snap_ok && kh_ensure_ok("composite shader", ensure_composite_shader(dev));

    // Fail-safe direction: heightfield or arb shader down => stock LESS_EQUAL path (the known bite, never a new
    // artifact).
    // KH_FAR_ARB_SNAP: and no usable snapshot (khs_snap_ok) - the arb PS reads t0, which is the engine's texture when
    // no snapshot binds there. The flush's twin requires khf_arb_snap_ok.
    const bool khr_far_arb = khr_far_phase && g_thm_valid && khs_snap_ok &&
                             kh_ensure_ok("composite shader", ensure_composite_shader(dev)) &&
                             g_res.ps_composite_arb != nullptr;

    g_inj_arb_prev_armed = khr_far_arb;

    UINT stride = sizeof(MeshVertex), offset = 0;
    KhVbRef bound_vb;   // KH_CLOTH: seeded on the bind below (KH_SKIN_POOL: the pair).
    bound_vb.vb = g_res.mesh_vb.empty() ? nullptr : g_res.mesh_vb[0];
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[0], &stride, &offset);
    ctx->IASetIndexBuffer(g_res.mesh_ib[0], DXGI_FORMAT_R32_UINT, 0);
    ctx->VSSetShader((guard || khr_far_arb) ? g_res.vs_composite : g_res.vs, nullptr, 0);
    ctx->PSSetShader(khr_far_arb ? g_res.ps_composite_arb
                     : guard     ? g_res.ps_composite
                                 : g_res.ps, nullptr, 0);
    // SV_Depth replaces the rasterized depth entirely, so the shared per-frame viewProj stays untouched: the override
    // is two per-object scalars plus the PS pick.
    ID3D11PixelShader* khr_bound_ps = khr_far_arb ? g_res.ps_composite_arb
                                    : guard      ? g_res.ps_composite
                                                 : g_res.ps;
    ID3D11VertexShader* khr_bound_vs = (guard || khr_far_arb) ? g_res.vs_composite : g_res.vs;
    ID3D11InputLayout* khr_bound_il = g_res.input_layout;
    ID3D11ShaderResourceView* khr_bound_sk = nullptr;   // KH_SKIN_POOL: VS t52 (kh_inst_sk_bind).
    // s0 for the textured per-submesh path; StateBackup's sampler slot restores.
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    const float khr_acc_far = kh_enc_far(pv.projection[2][2], pv.projection[3][2]);
    // Fail direction: disarmed = the stock path. Arms only with the arb shader compiled, a sane near, and a real
    // sub-floor gap to open (MinDepth 0 has none).
    const float khr_nz_A = pv.projection[2][2];
    const float khr_nz_B = pv.projection[3][2];
    const float khr_nz_near = (khr_nz_A > 1.0e-6f) ? (-khr_nz_B / khr_nz_A) : 0.0f;
    const bool khr_nz_on = g_res.ps_composite_arb != nullptr && khr_nz_near > KH_CAM_NEAR_MIN && khr_nz_near < 50.0f && khr_pass_vp.MinDepth > 1.0e-5f && khr_pass_vp.MinDepth < khr_pass_vp.MaxDepth && khr_pass_vp.MaxDepth <= 1.0f;
    D3D11_VIEWPORT khr_nz_vp = khr_pass_vp;
    const float khr_nz_gap_lo = khr_pass_vp.MinDepth * KH_NEARZ_GAP_FRAC;
    if (khr_nz_on) khr_nz_vp.MinDepth = khr_nz_gap_lo;
    khr_ssao.nz_near = khr_nz_on ? khr_nz_near : 0.0f;   // KH_SSAO_NEARZ: the gap this pass's routed draws fill.
    khr_ssao.nz_gap = khr_nz_gap_lo;
    // The per-draw viewport swap state (khr_bound_rs pattern): 0 = the pass's, 1 = the near-gap.
    int khr_vp_bound = 0;
    auto khr_vp_select = [&](int khrv_want) {
        if (khrv_want == khr_vp_bound) return;
        ctx->RSSetViewports(1, khrv_want == 1 ? &khr_nz_vp : &khr_pass_vp);
        khr_vp_bound = khrv_want;
    };
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Binding probe: sky rows (fog) and the view harvest (lock survival). Runs
    // when either wants; idles when both are satisfied.
    if ((g_fog_valid && g_fog[0] > 1e-4f) || !g_ls.view_src_valid) skybind_step(ctx);

    // Twin: the flush path carries the identical narrowing.
    ID3D11Buffer* khr_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khr_cbs);
    ctx->PSSetConstantBuffers(0, 2, khr_cbs);
    // KH_INJ_T0: the depth snapshot or null at t0, every pass (inside StateBackup's saved range). PSComposite Loads t0
    // with no lane gating it, and the near-gap route draws the arb twin whether or not a snapshot exists
    // (KH_FAR_ARB_SNAP gates the far route alone) - left unbound, the first frame of a session or device and every
    // frame after a failed snapshot create would read whatever view the engine had at t0. Such a draw has its guard
    // stood down (fx1[0] = 1e9, fx1.zw = 0, snapCam.w = 0), so the value is discarded; a null view reads 0, the guard's
    // "no scene". Twin: the flush binds the snapshot or null per solid (KH_FLUSH_NOSNAP).
    {
        ID3D11ShaderResourceView* const khr_t0 = g_res.comp_depth_srv;
        ctx->PSSetShaderResources(0, 1, &khr_t0);
    }
    // Bound together whenever any consumer is armed: the ratio needs both textures, and a half-bound pair is the
    // silent-wrong-answer shape this feature avoids.
    if (kh_svs_sten_on()) {
        if (g_svs_pre_srv) {
            ctx->PSSetShaderResources(21, 1, &g_svs_pre_srv);
        }

        if (g_svs_post_srv) {
            ctx->PSSetShaderResources(22, 1, &g_svs_post_srv);
        }
    }
    // The volume copy's stencil plane (t24), on its own arm; inside
    // StateBackup's saved range. Twin: the flush carries the identical block.
    if (kh_svs_vol_ready()) {
        ID3D11ShaderResourceView* const khr_sten = kh_rp_use_merged() ? g_rp_msten_srv : g_svs_vol_sten_srv;   // KH_VOL_REPLAY.
        ctx->PSSetShaderResources(24, 1, &khr_sten);
        // KH_VOL_WITNESS: its depth plane (t23; inside StateBackup's range).
        if (g_svs_vol_depth_srv) ctx->PSSetShaderResources(23, 1, &g_svs_vol_depth_srv);
        // KH_VOL_FOOT: its footprint mask (t33; inside StateBackup's range).
        ID3D11ShaderResourceView* const khr_foot = kh_rp_use_merged() ? g_rp_mfoot_srv : kh_svs_foot_bound();
        if (khr_foot) ctx->PSSetShaderResources(33, 1, &khr_foot);
    }
    // The mirror stencil at t28 (inside StateBackup's saved range). mirMeta.x = 0 short-circuits the shader, so an
    // absent mask never reads a stale bind. Twin: the flush carries the identical block.
    if (g_vmir_srv && g_vmir_mask_time >= 0.0f) {
        ID3D11ShaderResourceView* const khr_mir = kh_rp_mir_srv();   // KH_MIR_REPLAY.
        ctx->PSSetShaderResources(28, 1, &khr_mir);
    }
    // Shadow atlas for the per-pixel map compare (shadowMeta.x = 0 short-circuits the shader when the table is empty).
    // t1 is inside StateBackup's saved range. KH_INJ_ATLAS: the atlas or null, every pass - the table
    // (fill_lighting_frame_cb) is armed whether or not this bind lands, and left unbound the receive would read
    // whatever the engine had at t1 (a frame without a fresh snapshot, or the composite shader down). Twin: the flush
    // and the in-front slice bind the same way.
    {
        ID3D11ShaderResourceView* const khr_atlas = shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
        ctx->PSSetShaderResources(1, 1, &khr_atlas);
    }
    for (UINT bslot = 0; bslot < 8; ++bslot) {
        if (g_ls.band[bslot].valid && g_ls.band[bslot].srv) {
            const UINT treg = bslot < 6 ? 4 + bslot : 12 + (bslot - 6);
            ctx->PSSetShaderResources(treg, 1, &g_ls.band[bslot].srv);
        }
    }
    // Private sun-depth map for the self term (t11, inside the saved range).
    kh_sun_pf_convert(ctx);   // Pyramids convert here (proven binds).
    if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    if (g_sun2_map_valid && g_sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29, 1, &g_res.sun_pf_srv[0]);
    if (g_sun3_map_valid && g_sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30, 1, &g_res.sun_pf_srv[1]);
    if (g_sun4_map_valid && g_sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31, 1, &g_res.sun_pf_srv[2]);
    if (g_sun5_map_valid && g_sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20, 1, &g_res.sun_pf_srv[3]);   // KH_FAR_PF.
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER: null = the default state, also clamp.
    // Analytic terrain heightfield (t10, saved range; see thmParams).
    if (g_thm_valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    // The light depth array at t36, bound by the pass that reads it (twin: the flush). Bound whenever it exists -
    // per-light validity lives in dls_meta[i].w, so an unbound slot would be a second way to say 'no shadow' and the
    // two could disagree.
    if (g_res.dls_srv) {
        ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    }

    // The injection fills blendCtl.x = 0 unconditionally; PSComposite's blendCtl.x branch stays cold. The engine's
    // translucents must be able to depth-reject against these meshes.

    // LESS_EQUAL + write for both arms: the honest raw-space test is the object authority; the arb variant beats
    // terrain LOD via its SV_Depth clamp.
    // KH_SSAO_MARK: our depth writes mark stencil 1 only when this pass's post (kh_ssao_post, its eraser) runs after
    // them; a mark no eraser follows would stand for the next drawer's pass.
    ID3D11DepthStencilState* const khr_dss_w = khr_ssao.on ? g_res.dss_test_write_mark : g_res.dss_test_write;
    ctx->OMSetDepthStencilState(khr_dss_w, khr_ssao.on ? 1u : 0u);
    // KH_WSEE: our solids write the main depth from here (late: past the capture).
    kh_wsee_taint(g_topo_cycles);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* khr_bound_rs = g_res.rasterizer;
    int khr_bound_bm = -1;
    const FLOAT bf[4] = { 0, 0, 0, 0 };

    // Per-pass frame build (injection edition). Each object starts from this template; only the object slice uploads
    // per draw. Twin of the flush build (khf_cbf); this copy also interleaves the g_band_tab_* and sun-range
    // publications.
    ConstantData khr_cbf = {};
    memcpy(khr_cbf.view_proj, view_proj, sizeof(khr_cbf.view_proj));

    // cam is deliberately not always pv.view's own camera (KH_ENGCAM: kh_engcam_consume may hand back the engine's
    // own camera - the frame's latch, the seam's take where kh_volume_seam_inject consumed first), and the fold
    // cancels against kh_fill_center_rel for any origin.
    const bool khr_rebase_on = khcs_engcam
                             ? kh_rebase_vp_engcam(khr_cbf, pv.view, pv.projection)
                             : kh_rebase_vp(khr_cbf, cam);   // KH_DDX_REBASE.
    fill_lighting_frame_cb(khr_cbf);
    // Snapshot reprojection frame fields (arb PS consumer; every other pass ignores them).
    if (g_snap_vp_valid && khr_far_arb) {
        memcpy(khr_cbf.snap_vp, g_snap_vp, sizeof(khr_cbf.snap_vp));
        khr_cbf.snap_meta[0] = g_snap_pair[0];
        khr_cbf.snap_meta[1] = g_snap_pair[1];
        khr_cbf.snap_meta[2] = g_snap_vp_lo;
        khr_cbf.snap_meta[3] = g_snap_vp_hi;
        khr_cbf.snap_cam[0] = g_snap_cam[0];
        khr_cbf.snap_cam[1] = g_snap_cam[1];
        khr_cbf.snap_cam[2] = g_snap_cam[2];
        khr_cbf.snap_cam[3] = 1.0f;
    } else {
        khr_cbf.snap_cam[3] = 0.0f;   // The zeroed template already says so; explicit.
    }
    {
        bool band_any = false;
        // Finest-first: order slots by near edge ascending, so the shader's first containing band holds the finest
        // content in the engine's overlap zones. bandBorder.w encodes valid + which physical texture slot to sample (1
        // + index).
        int order[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

        for (int a = 0; a < 7; ++a) {
            for (int b2 = a + 1; b2 < 8; ++b2) {
                const float na = g_ls.band[order[a]].valid ? g_ls.band[order[a]].border[0] : 1e9f;
                const float nb = g_ls.band[order[b2]].valid ? g_ls.band[order[b2]].border[0] : 1e9f;

                if (nb < na) {
                    const int t = order[a];
                    order[a] = order[b2];
                    order[b2] = t;
                }
            }
        }

        float prev_far = -1e9f;

        for (int b = 0; b < 8; ++b) {
            const auto& bs = g_ls.band[order[b]];
            khr_cbf.band_border[b][3] = 0.0f;
            // KH_BAND_TAB_FILL: the table is this fill's, not the session's. Do not carry it across fills: a position
            // written once and then skipped keeps its old far, and the max below (src 2, which shuts the video option
            // out) can then only grow - a lowered shadow distance never reaches our range while a dedupe twin or a
            // cascade the engine stopped sealing holds a slot. The far is the engine's cascade border, so it is taken
            // from every sane valid band, committed to the lanes or not: our own receive gating (warm-up, health, a
            // pending view, the twin dedupe) must not flap the range. A fill with no sane band leaves the previous
            // value and src standing (the max stays -1 and fails the guard).
            g_band_tab_far[b] = -1.0f;
            if (!bs.valid || !bs.srv) { continue; }
            if (bs.border[1] > bs.border[0] && bs.border[0] >= 0.0f && bs.border[1] <= 4000.0f) {
                g_band_tab_far[b] = bs.border[1];   // The sanity test below, in the positive.
            }

            // Absent beats offset: the receive term stands down for KH_RECEIVE_WARMUP_S after the first capture.
            if (!g_ls.receive_warmed) {
                if (g_ls.first_capture_time < 0.0f ||
                    effect_time_seconds() - g_ls.first_capture_time < KH_RECEIVE_WARMUP_S) {
                    continue;
                }
                g_ls.receive_warmed = true;
            }

            if (!kh_recv_health()) { continue; }

            // Spawn-window guard: a pending band whose provisional view is the bridge fallback pairs cross-convention;
            // the band re-enters the moment its seal completes.
            if (bs.pending_view && bs.vcol_bridge) {  continue; }

            // A pending band's vcol belongs to the epoch before the sm sitting next to it.
            if (bs.pending_view) {
                continue;
            }

            // Sanity first (a band failing it is dropped until its next resolve); then the twin dedupe: near inside the
            // previous band's range and far not meaningfully beyond it = a partition duplicate of the same range; two
            // twins alternate seal content and the shader's winner flip-flops.
            if (!(bs.border[1] > bs.border[0]) || bs.border[0] < 0.0f ||
                bs.border[1] > 4000.0f) {
                g_ls.band[order[b]].valid = false;   // The next resolve reseals sanely.
                continue;
            }

            if (bs.border[0] < prev_far - 0.5f && bs.border[1] < prev_far * 1.25f + 1.0f) { continue; }
            prev_far = bs.border[1];
            memcpy(khr_cbf.band_mat[b * 3 + 0], bs.sm + 0, 16);
            memcpy(khr_cbf.band_mat[b * 3 + 1], bs.sm + 4, 16);
            memcpy(khr_cbf.band_mat[b * 3 + 2], bs.sm + 8, 16);
            memcpy(khr_cbf.band_view[b * 3 + 0], bs.vcol + 0, 16);
            memcpy(khr_cbf.band_view[b * 3 + 1], bs.vcol + 4, 16);
            memcpy(khr_cbf.band_view[b * 3 + 2], bs.vcol + 8, 16);
            khr_cbf.band_border[b][0] = bs.border[0];
            khr_cbf.band_border[b][1] = bs.border[1];
            khr_cbf.band_border[b][2] = bs.border[2];
            khr_cbf.band_border[b][3] = 1.0f + static_cast<float>(order[b]);
            band_any = true;
        }

        {   // The far border of the engine's cascades (KH_BAND_TAB_FILL): the max over this fill's table.
            float khsrb_mf = -1.0f;
            for (int khsrb_i = 0; khsrb_i < 8; ++khsrb_i) {
                if (g_band_tab_far[khsrb_i] > khsrb_mf) khsrb_mf = g_band_tab_far[khsrb_i];
            }
            if (khsrb_mf > 8.0f && khsrb_mf < 1.0e5f && khsrb_mf == khsrb_mf) {
                const float khsrb_prev = g_sun_range.load(std::memory_order_relaxed);
                if (!(fabsf(khsrb_mf - khsrb_prev) <= 0.005f * khsrb_prev)) {
                    g_sun_range.store(khsrb_mf, std::memory_order_relaxed);
                }
                g_sun_range_src.store(2, std::memory_order_relaxed);
            }
        }

        if (band_any && g_sun_valid &&
            (g_atlas_frame_cur || g_atlas_frame_prev)) {   // Published sun + live atlas.
            khr_cbf.mask_meta[0] = 1.0f;   // Stream liveness: the receive.
        }
        khr_cbf.mask_meta[3] = kh_svs_unit_on() ? 1.0f : 0.0f;
        // The volume transport arm and the copy's dimensions, for the visible mesh.
        kh_fill_sten_reproj(khr_cbf, pv.projection);

        // The fog-lane fill below: TWIN of the flush's (kh_fx_frame_template)
        // and of kh_dlsw_fog_lanes (KH_DLSW_FOG_FILL).
        const float khr_cam_y =
            (g_ls.cam[0] * g_ls.cam[0] + g_ls.cam[1] * g_ls.cam[1] +
             g_ls.cam[2] * g_ls.cam[2] >= 1.0f) ? g_ls.cam[1]
          : (cam[0] * cam[0] + cam[1] * cam[1] + cam[2] * cam[2] >= 1.0f)
             ? cam[1] : 0.0f;
        // The TRUE camera altitude, unclamped. The shaders arm their below-layer branch on camY < fogSkyCol.w
        // (static.hlsl, composite.hlsl), so a lane clamped at the layer would arm that branch nowhere: the ray is
        // split shader-side, not here.
        const float khr_fog_cam_y = khr_cam_y;
        // fogBelow: x = the engine's below-layer extinction (block nb[51]), y = armed. z is written 0 here: it is the
        // cast fire's lane (KH_CAST_SNAP_FACING, read by PSMaskCast alone), which the mesh shaders never read; w is
        // never written.
        khr_cbf.fog_below[0] = (g_light_probe.hits > 0 && g_light_probe.meta == 40)   // KH_PROBE_META40.
                             ? g_light_probe.nb[51] : 0.0f;
        khr_cbf.fog_below[1] = 1.0f;
        khr_cbf.fog_below[2] = 0.0f;
        const bool khr_fog_on = g_fog_valid && g_fog[0] > 1e-4f;
        const bool khr_eng_ok = g_light_probe.hits > 0 && g_light_probe.meta == 40;
        kh_fill_haze(khr_cbf);

        if (khr_fog_on || khr_cbf.haze_pars[3] >= 0.5f || khr_eng_ok) {
            khr_cbf.fog_params[0] = khr_fog_on ? g_fog[0] : 0.0f;
            khr_cbf.fog_params[1] = g_fog[1];
            khr_cbf.fog_params[2] = g_fog[2];
            khr_cbf.fog_params[3] = khr_fog_on ? 1.0f : 0.0f;

            // Fog is decoupled from the block lock, so a degenerate staged decay or the pre-lock cold window does not
            // leave the mesh unfogged in a fogged world.
            if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
                const float* e = g_light_probe.nb + 36;

                for (int c = 0; c < 3; ++c) khr_cbf.fog_color[c] = e[c];

                // Engine transmittance terms from the block mirror: the density
                // scale, the linear ramp's end + inverse range.
                khr_cbf.fog_engine[0] = g_light_probe.nb[41];
                khr_cbf.fog_engine[1] = g_light_probe.nb[48];
                khr_cbf.fog_engine[2] = g_light_probe.nb[49];
                const bool khr_dec_fresh =
                    g_light_probe.hits > 0 &&
                    (effect_time_seconds() - g_light_probe.mirror_confirm) < 0.5f;   // KH_DEC_MIRROR_CLOCK.
                const bool khr_dec_eng = khr_dec_fresh && g_light_probe.nb[40] >= 0.0f && g_light_probe.nb[40] < 1.0f;
                if (khr_dec_eng) {
                    khr_cbf.fog_params[1] = g_light_probe.nb[40];
                }
                const bool khr_terms = g_light_probe.nb[48] > 1.0f &&
                                       g_light_probe.nb[49] > 0.0f &&
                                       g_light_probe.nb[41] >= 0.0f;
                const float khr_rp_end = g_light_probe.nb[48];
                const float khr_rp_inv = g_light_probe.nb[49];
                const float khr_rp_prod = khr_rp_end * khr_rp_inv;
                const bool khr_rp_incoh = khr_terms && khr_rp_prod < 1.0f;
                const bool khr_ramp_off = khr_rp_incoh;
                khr_cbf.fog_engine[3] = !khr_terms ? 0.0f : (khr_ramp_off ? 2.0f : 1.0f);
            } else {
                khr_cbf.fog_color[0] = 1.0f;
                khr_cbf.fog_color[1] = 1.0f;
                khr_cbf.fog_color[2] = 1.0f;
            }

            if (g_sky_probe.hits > 0) {
                khr_cbf.fog_sky[0] = 1.0f;
                khr_cbf.fog_sky[1] = 1.0f;
                khr_cbf.fog_sky[2] = 1.0f;
                khr_cbf.fog_sky[3] = 1.0f;
                khr_cbf.fog_sky_col[0] = g_sky_probe.nb[4];
                khr_cbf.fog_sky_col[1] = g_sky_probe.nb[5];
                khr_cbf.fog_sky_col[2] = g_sky_probe.nb[6];
                // The below-layer convergence colour: nb[28..30] = the sky fog colour, nb[68..70] = its elevation
                // gradient. Pure captures, inert unless the shader is below the layer.
                khr_cbf.fog_uw[0] = g_sky_probe.nb[28];
                khr_cbf.fog_uw[1] = g_sky_probe.nb[29];
                khr_cbf.fog_uw[2] = g_sky_probe.nb[30];
                khr_cbf.fog_uw[3] =
                    1.0f;
                khr_cbf.fog_uw_grad[0] = g_sky_probe.nb[68];
                khr_cbf.fog_uw_grad[1] = g_sky_probe.nb[69];
                khr_cbf.fog_uw_grad[2] = g_sky_probe.nb[70];
            }

            // Camera altitude (engine Y-up), unclamped - see khr_fog_cam_y.
            khr_cbf.fog_color[3] = khr_fog_cam_y;
        }
    }
    // Heightfield lanes, both arms - the terrain lane never depends on the depth lane's arming state.
    kh_fill_occ(khr_cbf);
    // Pass lanes: the rebase camera the bucket VS subtracts (armed exactly when
    // the per-object path arms centerRel) and the object view-distance cut.
    khr_cbf.kh_pass[0] = cam[0]; khr_cbf.kh_pass[1] = cam[1]; khr_cbf.kh_pass[2] = cam[2];
    khr_cbf.kh_pass[3] = khr_rebase_on ? 1.0f : 0.0f;
    khr_cbf.kh_pass_obj[0] = g_obj_vis.load(std::memory_order_relaxed);
    kh_fill_user_cam_cb(khr_cbf, cam, pv.view);   // KH_USER_LANES.
    const bool khr_frame_ok = kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khr_cbf);
    if (khr_frame_ok) {   // KH_PIP: the next frame's PIP passes draw under this template.
        g_pip.tpl = khr_cbf;
        g_pip.tpl_valid = true;
        g_pip.tpl_cycle = g_topo_cycles;
    }

    // Publish the pass's pixels-per-tangent here too: on a frame the injection draws and the flush never reaches its
    // own publish, g_lod_proj_px would stay 0 and every object would draw at level 0. KH_LOD_ONE_SCALE: the flush's
    // definition - this pass's own viewport height (khr_pass_vp, what its meshes draw with), not the adopted depth
    // texture's; an unusable pair keeps the last value.
    if (kh_lod_publish(pv.projection[1][1], khr_pass_vp.Height)) {   // KH_LOD_PARK_KEEP: the flush's park test.
        g_lod_inj_h = khr_pass_vp.Height;
        g_lod_inj_cycle = g_topo_cycles;
    }

    KhCullSet khr_cull;
    kh_cull_build(khr_cbf.view_proj, cam, khr_cull);
    khr_cull.back_on = true;   // KH_CULL_BACK: the colour pass never draws what is behind the camera.
    // Per-mesh route verdict (the near-z gap; the far route is the pass's, khr_far_arb), factored out so the bucket
    // plan keys every member through the same code the representative is judged by (route is part of the key).
    struct KhInjRoute { bool nearz; };
    auto khr_route_of = [&](const RenderObject& khrv_o) -> KhInjRoute {
        KhInjRoute khrv_r;
        // For z >= near the encode equals the world mapping to a few D24 steps (float32 re-map; KH_ARB_VP_FLOOR),
        // so routing on/off pops nothing but a fragment coplanar with the world's. KH_NEARZ_BOUNDS: the drawn box
        // decides - what a fragment nearer than the near plane can come from (the script's box can hold less).
        khrv_r.nearz = khr_nz_on &&
            g_dist_memo_inj.get_drawn(khrv_o, cam) < (khr_nz_near + 2.0f) * (khr_nz_near + 2.0f);
        return khrv_r;
    };
    // Buckets, injection edition: the pass verdict, the plan + emission (natural order) and the slot-1 bind latch. This
    // pass routes buckets through the composite instanced twins, which compile non-fatally; absent either twin the
    // whole pass draws per object (a missing twin would strand a bucket's members). The flush's gate stays
    // kh_inst_twins_on(): the static twins are all-or-nothing.
    const bool khr_inst_on = kh_inst_twins_on() &&
                             g_res.vs_comp_inst != nullptr && g_res.vs_comp_inst_tex != nullptr;
    static KhInstPlan khr_inst_plan;
    bool khr_inst_vb_bound = false;
    // Finished background loads land here, before any base is fixed or lane written (a placement bumps the page
    // generation and the ensure below re-fixes every base).
    kh_tex_async_pump(ctx, dev);
    // Occlusion cull, injection edition: this pass depth-writes every solid, so a hidden object provably contributed
    // zero pixels (from KH_THM_MIN_DIST_M, none the heightfield shows: KH_VOC_BURIAL). SV_Depth routes are excluded on
    // both sides; on far-arb frames the terrain clamp may pull any fragment up to the guard nearer, so that is a
    // margin. The cap is metres of VIEW DEPTH while the grid compares EUCLIDEAN distances, so the margin carries the
    // frustum-corner secant sqrt(1 + tan^2(fovx/2) + tan^2(fovy/2)) from the pass projection; unreadable scales read as
    // 'never cull'.
    float khr_occ_margin = 0.0f;
    if (khr_far_arb) {
        const float khr_opx = fabsf(pv.projection[0][0]);
        const float khr_opy = fabsf(pv.projection[1][1]);
        if (khr_opx > 1.0e-6f && khr_opy > 1.0e-6f) {
            const float khr_otx = 1.0f / khr_opx;
            const float khr_oty = 1.0f / khr_opy;
            khr_occ_margin = (KH_FAR_ARB_GUARD_BASE + 1.0f) *
                             sqrtf(1.0f + khr_otx * khr_otx + khr_oty * khr_oty);
        } else {
            khr_occ_margin = 1.0e18f;
        }
    }
    static KhVisOcc khr_occ;
    kh_vis_occ_build(khr_occ, meshes, khr_occx, khr_cbf.view_proj, khr_cull, cam,
                     khr_occ_margin,
                     khr_acc_far, g_obj_vis.load(std::memory_order_relaxed),
                     // KH_VOC_FLOOR_FLOAT: the floor is a height (kh_vis_occ_build reads a float); a bool return would
                     // make REFUSE, NONE and every floor 1.0f.
                     [&](const RenderObject& khvs_o) -> float {
                         const KhInjRoute khvs_r = khr_route_of(khvs_o);
                         return khvs_r.nearz ? KH_VOC_FLOOR_REFUSE : kh_voc_floor(khvs_o);
                     });
    // The pass's one visibility truth: the material ensure, the plan's emit and the per-object loop all call this same
    // lambda, so a bucket's representative can never be admitted by one test and dropped by another, and every emitted
    // lane's set was ensured. Memoised per object per pass.
    g_vis_memo_inj.begin(meshes);
    // Why an object is not drawn: 0 it is, 1 outside the view, 2 hidden by the
    // occlusion cull. The verdict is 'is it 0'; KH_OCC_STATS counts the 2s.
    auto khr_vis_code = [&](const RenderObject& khrv_o) -> uint8_t {
        float khrv_cc[3];
        kh_obj_center_engine(khrv_o, khrv_cc);
        float khrv_bs[3];
        kh_bounds_size_of(khrv_o, khrv_bs);   // KH_CAST_REFIT: the cull's size; the LOD pick keeps size.
        if (!kh_mesh_visible(khr_cull, khrv_cc, kh_lod_radius_of(khrv_bs, khrv_o.rot_m, khrv_o.rotated))) return 1u;
        if (khr_occ.on) {
            const KhInjRoute khrv_r = khr_route_of(khrv_o);
            if (!khrv_r.nearz && kh_vis_occ_hidden(khr_occ, khrv_o)) return 2u;
        }
        return 0u;
    };
    auto khr_vis_calc = [&](const RenderObject& khrv_o) -> bool { return khr_vis_code(khrv_o) == 0u; };
    auto khr_vis = [&](const RenderObject& khrv_o) -> bool {
        return g_vis_memo_inj.get(khrv_o, khr_vis_calc);
    };
    // KH_INJ_FORK: every object's verdict now, across the pool, into the memo's own marks (the same calc every consumer
    // below would run through get). The calc reads the cull set, the occupancy grid and the distance memo - all built
    // and seeded above - and writes its own index only.
    // KH_OCC_STATS: armed, each index also records whether the occlusion cull hid it - its own slot, as the marks are -
    // summed after the join.
    static std::vector<uint8_t> khr_occ_hid;
    const bool khr_occ_count = kh_stats_on();
    if (khr_occ_count) khr_occ_hid.assign(meshes.size(), 0u);
    kh_fork_each(static_cast<uint32_t>(meshes.size()), [&](uint32_t khrv_i) {
        const uint8_t khrv_c = khr_vis_code(meshes[khrv_i]);
        g_vis_memo_inj.mark[khrv_i] = khrv_c == 0u ? 1u : 2u;
        if (khr_occ_count) khr_occ_hid[khrv_i] = khrv_c == 2u ? 1u : 0u;
    });
    if (khr_occ_count) {
        uint64_t khr_occ_n = 0;
        for (const uint8_t khr_h : khr_occ_hid) khr_occ_n += khr_h;
        kh_stat_add(g_stats.vis_occluded, khr_occ_n);
    }
    if (khr_inst_on) {
        kh_mat_ensure_all(ctx, dev, meshes, khr_vis);   // KH_MAT_TABLE: bases fixed before any lane is written.
        kh_inst_plan_build(khr_inst_plan, meshes, nullptr, meshes.size(),
                           [&](const RenderObject& khrp_o, KhInstKey& khrp_k) {
                               const KhInjRoute khrp_r = khr_route_of(khrp_o);
                               khrp_k.route = khrp_r.nearz ? 2u : 0u;
                               khrp_k.aux = kh_dl_bucket_hash(khrp_o);
                           });
        // The cull walk, once, against the pass's own frustum; the whole stream lands in the ring here.
        kh_inst_plan_emit(ctx, dev, khr_inst_plan, meshes, cam, 1, khr_vis, g_dist_memo_inj);
    }
    // KH_BLEND_PART_INJ - the translucent part draws where the solid draws.
    struct KhInjPart {
        uint32_t oi;
        ConstantData cbd;   // The object's slice as its solid drew; mat lanes refilled per submesh by
                            // the draw, light lanes re-appended at the tail (KH_DL_RING).
        ID3D11PixelShader* ps;
        ID3D11VertexShader* vs;
        ID3D11InputLayout* il;
        const KhSkinPoolChunk* pool = nullptr;   // KH_SKIN_POOL: a bucket part's chunk (its Sk twin's t52).
        int ctx;   // kh_draw_textured's pipeline variant (user-shader twin selection).
        int lod;
        float lodt;
        bool lodx;
        int vpsel;   // The viewport this object drew under (khr_vp_select's code).
        bool ts_ordered;
        bool inst;   // The part is a bucket's: kh_inst_draw part 2, one tail entry per lane (KH_INST_PART_LANE).
        float dist2;
        bool ws_defer = false;   // KH_WSEE: left to the flush's tail (its objects noted; not drawn here).
    };
    static std::vector<KhInjPart> khr_parts;
    // The far-to-near order, sorted as entries naming a record (a stable_sort on the records themselves wants an
    // aligned_storage temporary MSVC refuses; the records never move). KH_INST_PART_LANE: a bucket's part is one entry
    // per lane (lane = its ring lane, lvl its level, d2 its member's distance), so each lane takes its own place among
    // the other parts - the bucket drew all of them at its representative's place; a per-object part is one entry (lane
    // ~0u, d2 = its dist2).
    struct KhInjPartEnt { uint32_t part; uint32_t lane; int lvl; float d2; };
    static std::vector<KhInjPartEnt> khr_parts_order;
    static std::vector<KhInstPartLane> khr_part_lanes;   // Scratch: one bucket's lanes (kh_inst_part_lanes).
    khr_parts.clear();
    for (size_t khr_oi = 0; khr_oi < meshes.size(); ++khr_oi) {
        const RenderObject& o = meshes[khr_oi];
        // Carried by its bucket's draw at the representative, or culled by the
        // emission's walk (only the representative stays undone).
        if (khr_inst_on && khr_inst_plan.done[khr_oi]) continue;
        ConstantData cbd = khr_cbf;
        cbd.fx_meta[2] = static_cast<float>(g_main_depth_w);
        cbd.fx_meta[3] = static_cast<float>(g_main_depth_h);
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        // Per-object rebase arm mirrors the pass verdict (one without the other is the wrong-space landmine).
        float khr_res[3];   // KH_POS_RES: the object's live residual.
        kh_pos_res(o, khr_res);
        if (khr_rebase_on) kh_fill_center_rel(cbd, cam, khr_res);
        cbd.size_axes[0] = o.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y].
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        kh_fill_obj_rot(cbd, o.rot_m);
        // blendCtl.x: never on this pass (see the pass head).
        cbd.blend_ctl[0] = 0.0f;
        // blend_ctl[3] is the crossfade dither; written here so it is never inherited from the previous object's fade.
        cbd.blend_ctl[3] = 0.0f;
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        // The camera (engine space) serves both the guard's true fragment
        // distance and the solid band mask, so it is filled regardless.
        cbd.fx0[0] = cam[0];
        cbd.fx0[1] = cam[1];
        cbd.fx0[2] = cam[2];
        // per-mesh route verdicts (khr_route_of, above the loop).
        const KhInjRoute khr_rt = khr_route_of(o);
        const bool khr_mesh_nearz = khr_rt.nearz;
        // Textured routing: a material-carrying solid takes the textured twin of exactly the variant this draw would
        // have used (plain / guard / arb).
        const KhMaterialSet* khr_txm = kh_obj_textured(o);
        const bool khr_tx_arb = khr_far_arb || khr_mesh_nearz;
        const bool khr_comp_vs = guard || khr_tx_arb;
        const bool khr_tx_on = khr_txm != nullptr &&
            g_res.layout_tex && g_res.mat_sampler &&
            (khr_tx_arb ? g_res.ps_comp_arb_tex != nullptr :
             guard      ? g_res.ps_comp_tex != nullptr :
                          g_res.ps_tex != nullptr) &&
            (khr_comp_vs ? g_res.vs_comp_tex != nullptr
                         : g_res.vs_tex != nullptr);
        {
            ID3D11PixelShader* khr_want_ps =
                khr_tx_on ? (khr_tx_arb ? g_res.ps_comp_arb_tex
                             : guard    ? g_res.ps_comp_tex
                                        : g_res.ps_tex)
                : khr_tx_arb ? g_res.ps_composite_arb
                : guard                             ? g_res.ps_composite
                                                    : g_res.ps;
            if (khr_want_ps != khr_bound_ps) {
                ctx->PSSetShader(khr_want_ps, nullptr, 0);
                khr_bound_ps = khr_want_ps;
            }
            ID3D11VertexShader* khr_want_vs =
                khr_tx_on ? (khr_comp_vs ? g_res.vs_comp_tex : g_res.vs_tex)
                          : (khr_comp_vs ? g_res.vs_composite : g_res.vs);
            if (khr_want_vs != khr_bound_vs) {
                ctx->VSSetShader(khr_want_vs, nullptr, 0);
                khr_bound_vs = khr_want_vs;
            }
            ID3D11InputLayout* khr_want_il = khr_tx_on ? g_res.layout_tex : g_res.input_layout;
            if (khr_want_il != khr_bound_il) {
                ctx->IASetInputLayout(khr_want_il);
                khr_bound_il = khr_want_il;
            }
        }
        // Band / local-volume mask inputs (same conversion as the flush).
        kh_fill_local_band_cb(cbd, o);
        fill_lighting_obj_cb(ctx, cbd, o);
        if (guard || khr_tx_arb) {   // The pair and range, for the cut and the lane.
            // Guard inputs: reconstruction coefficients + the encode range of
            // the copied depth, the copy's pixel dimensions, and the margins.
            cbd.depth_params[0] = pv.projection[2][2];
            cbd.depth_params[1] = pv.projection[3][2];
            kh_fill_depth_range_cb(cbd);
            // Twin: both CB editions carry it. The copy's dimensions only when the copy is what the draw reads; a
            // routed draw without one keeps the scene's (the mask pixel clamps read these lanes too).
            if (guard) {
                cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
                cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
            }

            const bool exact_encode = measured || snap_slot_near > 0.0f;
            // The classic tiers return only as the no-heightfield terrain backstop.
            if (khr_far_arb) {
                // fxParams1.zw carry the analytic depth clamp: offset cap (the LOD margin) + the contact-band height
                // its ramp closes over; thm owns terrain in the same pass.
                cbd.fx1[0] = 1e9f;
                cbd.fx1[1] = 0.0f;
                cbd.fx1[2] = KH_FAR_ARB_GUARD_BASE;
                cbd.fx1[3] = kh_far_arb_contact();
            } else if (g_thm_valid || !guard) {
                // A live heightfield (it owns terrain), or a near-gap routed draw without the guard (no fresh snapshot,
                // or no composite shader; a far-routed draw takes the branch above): the guard stands down - the same
                // contract as the no-copy branch below (zero margins against an absent or stale depth is the
                // invisible-geometry landmine).
                cbd.fx1[0] = 1e9f;
                cbd.fx1[1] = 0.0f;
            } else {
                cbd.fx1[0] = exact_encode ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
                cbd.fx1[1] = exact_encode ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
            }
            if (khr_mesh_nearz) {
                cbd.fx_meta[0] =
                    khr_nz_near;
                cbd.fx_meta[1] = khr_nz_gap_lo;
            }
            g_inj_dp[0] = cbd.depth_params[0];
            g_inj_dp[1] = cbd.depth_params[1];
            g_inj_dp[2] = cbd.depth_params[2];
            g_inj_dp[3] = cbd.depth_params[3];
            g_inj_dp_valid = true;
        } else {
            // No depth copy this frame: the fallback PS carries the same guard contract, so stand it down explicitly -
            // zero margins with invalid depth is the invisible-geometry landmine.
            cbd.fx1[0] = 1e9f;
        }
        if (!o.fullscreen) {
            // The same khr_vis the plan's emit ran, so a bucket representative that was emitted always draws here.
            if (!khr_vis(o)) {  continue; }
        }

        // KH_TX_ONE_UPLOAD: a textured object's slice goes up per submesh inside kh_draw_textured (with that submesh's
        // material lanes, before its draw) on every route it draws by - per object and by bucket alike - and no draw
        // reads b0 between here and there, so an object-level upload here would never be read. Untextured draws read
        // this one. Twins: the flush and the PIP.
        // A failed pass-level frame upload (khr_frame_ok) stands every draw down - fail dark, never stale-frame draws.
        // Object slice only (the frame slice went up before the loop).
        if (!khr_frame_ok || (!khr_tx_on && !kh_upload_obj_cb(ctx, g_res.composite_cb, cbd))) continue;
        // twoSided semantics must survive arb frames: a flat CULL_BACK force erases interior faces there. Any shader
        // kind (a scene-read effect alpha-lerps on normal blend just like PSMain).
        const bool khr_ts_ordered = o.two_sided &&
                                    o.blend_mode == 0 && o.color[3] < 0.999f &&
                                    g_res.rasterizer_front != nullptr;
        ID3D11RasterizerState* khr_want_rs =
            khr_ts_ordered ? g_res.rasterizer_front :
            o.two_sided    ? g_res.rasterizer : g_res.rasterizer_cull;

        if (khr_want_rs != khr_bound_rs) {
            ctx->RSSetState(khr_want_rs);
            khr_bound_rs = khr_want_rs;
        }

        khr_vp_select(khr_mesh_nearz ? 1 : 0);   // The near-gap swap.

        const int mid = mesh_id_clamp(o.mesh);

        const KhVbRef khb_vb = kh_mesh_vb_ref(mid, o.slot);   // KH_CLOTH.
        if (khb_vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khb_vb.vb, &stride, &khb_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = khb_vb;
        }

        if (o.blend_mode != khr_bound_bm) {
            ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
            khr_bound_bm = o.blend_mode;
        }

        // The pick reads g_lod_proj_px (kh_lod_publish above, else the last value kept); only while it is unpublished
        // (0) does every object select level 0 rather than guess a projection scale.
        const MeshDef& khr_md = mesh_def(mid);
        int khr_lod = 0;
        float khr_lodt = 0.0f;
        // A locked instance never picks - level 0, no fade. Twin at the flush.
        if (!kh_lod_locked(o)) {   // KH_LOD_SIM_SLOT.
            kh_lod_pick(khr_md, kh_lod_radius(o), sqrtf(g_dist_memo_inj.get(o, cam)), khr_lod, khr_lodt);
        }
        const bool khr_lodx = khr_lodt > 0.0f;

        // This object is its bucket's representative (see the flush twin). The VS is the composite instanced pair when
        // this draw took the composite VS, the static pair otherwise. The bucket's ranges were emitted before the loop,
        // so the per-object LOD walk runs zero iterations. Members share the route verdict by key.
        int khr_lod_iters = khr_lodx ? 2 : 1;

        if (khr_inst_on && khr_inst_plan.batch_of[khr_oi] >= 0) {
            // KH_SKIN_POOL: a skin pool chunk's bucket pulls each member's own pose (the Sk twins: no slot-0
            // vertices, the chunk at t52).
            const KhSkinPoolChunk* const khr_ipc = kh_inst_bucket_pool(khr_inst_plan, static_cast<uint32_t>(khr_oi));
            ID3D11VertexShader* khr_ivs = khr_ipc
                ? (khr_tx_on ? (khr_comp_vs ? g_res.vs_comp_inst_sk_tex : g_res.vs_inst_sk_tex)
                             : (khr_comp_vs ? g_res.vs_comp_inst_sk     : g_res.vs_inst_sk))
                : (khr_tx_on ? (khr_comp_vs ? g_res.vs_comp_inst_tex : g_res.vs_inst_tex)
                             : (khr_comp_vs ? g_res.vs_comp_inst     : g_res.vs_inst));
            ID3D11InputLayout*  khr_iil = khr_ipc ? g_res.layout_inst_sk
                                                  : (khr_tx_on ? g_res.layout_inst_tex : g_res.layout_inst);

            if (khr_ivs && khr_iil) {
                if (khr_ivs != khr_bound_vs) { ctx->VSSetShader(khr_ivs, nullptr, 0); khr_bound_vs = khr_ivs; }
                if (khr_iil != khr_bound_il) { ctx->IASetInputLayout(khr_iil); khr_bound_il = khr_iil; }
                kh_inst_sk_bind(ctx, khr_ipc, khr_bound_sk);

                if (!khr_inst_vb_bound) {
                    const UINT khr_is = KH_INST_STRIDE, khr_io = 0;
                    ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[1], &khr_is, &khr_io);
                    khr_inst_vb_bound = true;
                }

                if (o.two_sided && !khr_ts_ordered && khr_bound_rs != g_res.rasterizer) {
                    ctx->RSSetState(g_res.rasterizer);
                    khr_bound_rs = g_res.rasterizer;
                }

                // The bucket's ranges were emitted before the loop (cull, LOD
                // and route verdicts are in the plan), so this draws them.
                uint32_t khr_in = 0;
                const uint32_t khr_id = kh_inst_draw(ctx, dev, khr_inst_plan, meshes, static_cast<uint32_t>(khr_oi),
                    cbd, g_res.composite_cb, khr_tx_on ? khr_txm : nullptr, khr_ts_ordered,
                    khr_bound_rs, khr_tx_arb ? 2 : (guard ? 1 : 0), khr_bound_ps, 1, khr_in);
                kh_stat_add(g_stats.composite_meshes, khr_id);
                if (khr_tx_on) kh_stat_add(g_stats.textured_draws, khr_id);
                if (khr_ipc) kh_stat_add(g_stats.skin_inst, khr_in);   // KH_SKIN_POOL.
                // The batch's translucent part: the same gathered set, part 2, staged for the tail like a per-object
                // part - each lane its own tail entry, far-to-near among every other part (KH_INST_PART_LANE; dist2 is
                // unused for it). It must not draw here: the AO term (kh_ssao_post) multiplies every marked pixel, and
                // a blend part drawn ahead of it sits on its own solid's mark and takes the solid's term.
                if (khr_tx_on && khr_txm && khr_in > 0 && kh_obj_has_blend(o)) {
                    KhInjPart khp;
                    khp.oi = static_cast<uint32_t>(khr_oi);
                    khp.cbd = cbd;
                    khp.ps = khr_bound_ps;
                    khp.vs = khr_ivs;
                    khp.il = khr_iil;
                    khp.pool = khr_ipc;   // KH_SKIN_POOL.
                    khp.ctx = khr_tx_arb ? 2 : (guard ? 1 : 0);
                    khp.lod = 0;
                    khp.lodt = 0.0f;
                    khp.lodx = false;   // The plan's ranges carry their own levels.
                    khp.vpsel = khr_mesh_nearz ? 1 : 0;
                    khp.ts_ordered = o.two_sided && g_res.rasterizer_front != nullptr;
                    khp.inst = true;
                    khp.dist2 = g_dist_memo_inj.get(o, cam);
                    khr_parts.push_back(khp);
                }
                khr_lod_iters = 0;   // The batch drew; no per-object level walk.
            } else {
                kh_inst_unbatch(khr_inst_plan, static_cast<uint32_t>(khr_oi));   // KH_INST_UNBATCH.
            }
        }

        for (int khr_li = 0; khr_li < khr_lod_iters; ++khr_li) {
            const int khr_lvl = khr_lod + khr_li;

            if (khr_lodx) {
                cbd.blend_ctl[3] = kh_lod_dither(khr_lodt, khr_li != 0);
                // KH_TX_ONE_UPLOAD: textured, kh_draw_textured uploads the dither with each submesh.
                if (!khr_tx_on && !kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            }

            if (khr_tx_on) {
                // Per-submesh textured draws. Part 1 - the solid texels; the object's translucent part is staged below
                // and drawn in the parts tail (KH_BLEND_PART_INJ).
                const uint32_t khr_txd = kh_draw_textured(ctx, dev, cbd, g_res.composite_cb,
                                                          *khr_txm, mid, khr_ts_ordered, khr_bound_rs,
                                                          khr_tx_arb ? 2 : (guard ? 1 : 0), khr_bound_ps,
                                                          khr_lvl, 1);
                kh_stat_add(g_stats.composite_meshes, khr_txd);
                kh_stat_add(g_stats.textured_draws, khr_txd);
            } else {
                UINT khr_ls = 0, khr_lc = 0;
                mesh_lod_range(khr_md, khr_lvl, khr_ls, khr_lc);

                if (khr_ts_ordered && khr_bound_rs != g_res.rasterizer_front) {
                    ctx->RSSetState(g_res.rasterizer_front);
                    khr_bound_rs = g_res.rasterizer_front;
                }

                ctx->DrawIndexed(khr_lc, khr_ls, 0);
                kh_stat(g_stats.composite_meshes);

                if (khr_ts_ordered) {   // Pass 2: front faces over the interior.
                    ctx->RSSetState(g_res.rasterizer_cull);
                    khr_bound_rs = g_res.rasterizer_cull;
                    ctx->DrawIndexed(khr_lc, khr_ls, 0);
                    kh_stat(g_stats.composite_meshes);
                }
            }
        }
        // The solid draw's set, for the parts tail.
        ID3D11PixelShader*  khr_if_ps = khr_bound_ps;
        ID3D11VertexShader* khr_if_vs = khr_bound_vs;
        ID3D11InputLayout*  khr_if_il = khr_bound_il;

        cbd.blend_ctl[3] = 0.0f;   // Never leaks into the next object.

        // Stage this object's translucent part for the tail. The CB is the slice the solid draw uploaded; the dither
        // lane is cleared (the light lanes are re-appended at the tail).
        if (khr_lod_iters > 0 && khr_tx_on && khr_txm && kh_obj_has_blend(o)) {
            KhInjPart khp;
            khp.oi = static_cast<uint32_t>(khr_oi);
            khp.cbd = cbd;
            khp.ps = khr_if_ps;
            khp.vs = khr_if_vs;
            khp.il = khr_if_il;
            khp.ctx = khr_tx_arb ? 2 : (guard ? 1 : 0);
            khp.lod = khr_lod;
            khp.lodt = khr_lodt;
            khp.lodx = khr_lodx;
            khp.vpsel = khr_mesh_nearz ? 1 : 0;
            khp.ts_ordered = o.two_sided && g_res.rasterizer_front != nullptr;
            khp.inst = false;
            khp.dist2 = g_dist_memo_inj.get(o, cam);
            khr_parts.push_back(khp);
        }
    }

    // KH_SSAO: the term and its multiply, at our pixels, before any translucent part blends over them (its own
    // StateBackup / KhOmSave hand the loop's state back, so the tail's trackers stay true).
    kh_ssao_post(ctx, khr_ssao);

    // The tail: every staged part, far to near, under the exact state its solid drew with, dss_test, the object's blend
    // state, interiors-then-exteriors when twoSided, else back-face culled. The LOD crossfade is honoured. dss goes
    // back to the plain write state afterwards - never the marking one: the eraser has run (KH_SSAO_MARK), so a mark
    // left past it would reach the next drawer's apply.
    // KH_WSEE: the capture here (kh_wsee_inj_capture: after our solids and their AO, before our first translucent
    // part), in the world's own injection alone (khr_inj_world: not an anomalous rescue, the engine's viewport the
    // world's). Then, where the cycle sorts (kh_wsee_defer_ok; not a far phase - KH_PART_NEAR: the camera partition
    // draws after us under another pair - and no partitioned pair, kh_wsee_pair_split), the parts whose geometry the
    // flush can draw as the injection drew it (kh_wsee_defer_group: the mesh buffer named, own, or the vertices held;
    // a bucket's part only with every visible member so, a skin pool bucket too) are noted as copies and not drawn
    // here - only a part the flush can composite (normal blend, the mesh viewport, no effect, not depth-Off:
    // khf_ws_sort_this's terms).
    const bool khr_inj_world = !khr_anomalous && n_saved_vp >= 1 && kh_reorder_world_vp(saved_vp[0]);
    kh_wsee_inj_capture(ctx, khr_inj_world);
    auto khp_dok = [&](const RenderObject& khp_do, int khp_dvp) {
        return khp_do.blend_mode == 0 && khp_dvp == 0 && khp_do.effect == 0 && !khp_do.fullscreen &&
               khp_do.mode != DepthMode::Off;
    };
    if (!khr_parts.empty() && khr_inj_world && !khr_far_phase &&
        !kh_wsee_pair_split(pv.projection[2][2], pv.projection[3][2]) && kh_wsee_defer_ok(dev, ctx)) {
        static std::vector<const RenderObject*> khp_dms;   // One part's objects (kh_wsee_defer_group),
        static std::vector<int> khp_dlv;                     // their levels and crossfades.
        static std::vector<float> khp_dtv;
        for (KhInjPart& khp_d : khr_parts) {
            if (!khp_dok(meshes[khp_d.oi], khp_d.vpsel)) continue;
            if (!khp_d.inst) {
                // The object's geometry held, or its mesh buffer named, at the levels this part would have drawn
                // here; refused, the part stays here.
                const RenderObject* const khp_d1 = &meshes[khp_d.oi];
                const int khp_dl = khp_d.lod;
                const float khp_dt = khp_d.lodx ? khp_d.lodt : 0.0f;
                khp_d.ws_defer = kh_wsee_defer_group(dev, ctx, &khp_d1, &khp_dl, &khp_dt, 1u);
                continue;
            }
            // A skin pool bucket too (khp_d.pool): its members are drawn from their own slices, which are held.
            if (khp_d.oi >= khr_inst_plan.batch_of.size() || khr_inst_plan.batch_of[khp_d.oi] < 0) continue;
            const uint32_t khp_db = static_cast<uint32_t>(khr_inst_plan.batch_of[khp_d.oi]);
            if (khp_db >= khr_inst_plan.batch_begin.size() || khp_db >= khr_inst_plan.batch_n.size()) continue;
            const uint32_t khp_d0 = khr_inst_plan.batch_begin[khp_db], khp_dn = khr_inst_plan.batch_n[khp_db];
            if (static_cast<size_t>(khp_d0) + khp_dn > khr_inst_plan.members.size()) continue;
            bool khp_dall = true;
            khp_dms.clear();
            khp_dlv.clear();
            khp_dtv.clear();
            for (uint32_t khp_dk = 0; khp_dk < khp_dn && khp_dall; ++khp_dk) {
                const uint32_t khp_dm = khr_inst_plan.members[khp_d0 + khp_dk];
                if (khp_dm >= meshes.size()) { khp_dall = false; break; }
                const bool khp_dvis = khp_dm < g_vis_memo_inj.mark.size() && g_vis_memo_inj.mark[khp_dm] == 1u;
                if (!khp_dvis) continue;
                if (!khp_dok(meshes[khp_dm], khp_d.vpsel)) {
                    khp_dall = false;
                    continue;
                }
                khp_dms.push_back(&meshes[khp_dm]);
                // The member's level and crossfade as the bucket's emission picked and drew them
                // (kh_inst_plan_emit's lod_l / lod_t - one source, no second copy of its rule).
                const int khp_dl = khp_dm < khr_inst_plan.lod_l.size() ? khr_inst_plan.lod_l[khp_dm] : 0;
                const float khp_dt = khp_dm < khr_inst_plan.lod_t.size() ? khr_inst_plan.lod_t[khp_dm] : 0.0f;
                khp_dlv.push_back(khp_dl);
                khp_dtv.push_back(khp_dt);
            }
            if (!khp_dall) continue;
            // The visible members all or none, each with its entry.
            khp_d.ws_defer = kh_wsee_defer_group(dev, ctx, khp_dms.data(), khp_dlv.data(), khp_dtv.data(),
                                                 khp_dms.size());
        }
    }
    if (!khr_parts.empty()) {
        khr_parts_order.clear();
        for (uint32_t khp_i = 0; khp_i < static_cast<uint32_t>(khr_parts.size()); ++khp_i) {
            if (khr_parts[khp_i].ws_defer) continue;   // KH_WSEE: the flush's tail draws it.
            if (!khr_parts[khp_i].inst) {
                khr_parts_order.push_back({ khp_i, ~0u, 0, khr_parts[khp_i].dist2 });
                continue;
            }
            kh_inst_part_lanes(khr_inst_plan, khr_parts[khp_i].oi, khr_part_lanes);   // KH_INST_PART_LANE.
            for (const KhInstPartLane& khp_l : khr_part_lanes) {
                khr_parts_order.push_back({ khp_i, khp_l.lane, khp_l.lvl, khp_l.d2 });
            }
        }
        std::stable_sort(khr_parts_order.begin(), khr_parts_order.end(),
                         [](const KhInjPartEnt& a, const KhInjPartEnt& b) { return a.d2 > b.d2; });
        ctx->OMSetDepthStencilState(g_res.dss_test, 0);
        uint32_t khp_lights = ~0u;   // KH_INST_PART_LANE: the part whose lights the ring holds last (below).

        for (const KhInjPartEnt& khp_e : khr_parts_order) {
            KhInjPart& khp = khr_parts[khp_e.part];
            const RenderObject& khp_o = meshes[khp.oi];
            const KhMaterialSet* khp_txm = kh_obj_textured(khp_o);
            if (!khp_txm) continue;
            if (khp.ps != khr_bound_ps) { ctx->PSSetShader(khp.ps, nullptr, 0); khr_bound_ps = khp.ps; }
            if (khp.vs != khr_bound_vs) { ctx->VSSetShader(khp.vs, nullptr, 0); khr_bound_vs = khp.vs; }
            if (khp.il != khr_bound_il) { ctx->IASetInputLayout(khp.il); khr_bound_il = khp.il; }

            khr_vp_select(khp.vpsel);

            if (khp_o.blend_mode != khr_bound_bm) {
                ctx->OMSetBlendState(g_res.blend_modes[khp_o.blend_mode], bf, 0xFFFFFFFF);
                khr_bound_bm = khp_o.blend_mode;
            }

            if (!khp.ts_ordered && khr_bound_rs != g_res.rasterizer_cull) {
                ctx->RSSetState(g_res.rasterizer_cull);
                khr_bound_rs = g_res.rasterizer_cull;
            }

            const int khp_mid = mesh_id_clamp(khp_o.mesh);

            const KhVbRef khp_vb = kh_mesh_vb_ref(khp_mid, khp_o.slot);   // KH_CLOTH.
            if (khp_vb != bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &khp_vb.vb, &stride, &khp_vb.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[khp_mid], DXGI_FORMAT_R32_UINT, 0);
                bound_vb = khp_vb;
            }

            // KH_DL_RING: a ring region is this object's only until the next append, and every later object
            // appended after its solid - a device without NO_OVERWRITE reuses offset 0 on every append, and a wrap or
            // growth renames the ring. So the part appends its lights again here, next to its own draws: the same pool
            // as the solid's fill (this pass), so the same set; the lanes cleared first, as a fresh template carries
            // them, so a failed append draws unlit. A bucket's part takes its representative's, as its solid did.
            // KH_INST_PART_LANE: a bucket's lanes drawn one after another append once - nothing appends between them; a
            // lane drawn after another part's entry appends its part's lights again (one selection per run of the
            // part's lanes).
            if (khp_e.part != khp_lights) {
                memset(khp.cbd.dl_ctl, 0, sizeof(khp.cbd.dl_ctl));
                memset(khp.cbd.dl_first, 0, sizeof(khp.cbd.dl_first));
                if (khp_o.lit) fill_dynlights_cb(ctx, khp.cbd, khp_o);
                khp_lights = khp_e.part;
            }

            // KH_WSEE: the part's draw as one call, so the re-draws into T and NW (kh_wsee_tail, below) are this same
            // draw - khp_rep: such a re-draw, which counts nothing.
            auto khp_geo = [&](bool khp_rep) {
                if (khp.inst) {
                    // This lane of the bucket (the instance VB at slot 1 is still bound: nothing in this pass unbinds
                    // it, and kh_ssao_post's StateBackup put back what it found).
                    kh_inst_sk_bind(ctx, khp.pool, khr_bound_sk);   // KH_SKIN_POOL: its chunk (t52 is in the backup).
                    uint32_t khp_in = 0;
                    const KhInstPartLane khp_one = { khp_e.d2, khp_e.lane, khp_e.lvl };   // KH_INST_PART_LANE.
                    const uint32_t khp_n = kh_inst_draw(ctx, dev, khr_inst_plan, meshes, khp.oi, khp.cbd,
                                                        g_res.composite_cb, khp_txm, khp.ts_ordered, khr_bound_rs,
                                                        khp.ctx, khp.ps, 2, khp_in, &khp_one);
                    if (khp_rep) return;
                    kh_stat_add(g_stats.composite_meshes, khp_n);
                    kh_stat_add(g_stats.textured_draws, khp_n);
                    // KH_SKIN_POOL: instanced lanes (the flush draws its parts per object: none counted there).
                    if (khp.pool) kh_stat_add(g_stats.skin_inst, khp_in);
                    return;
                }
                for (int khp_li = 0; khp_li < (khp.lodx ? 2 : 1); ++khp_li) {
                    const int khp_lvl = khp.lod + khp_li;
                    khp.cbd.blend_ctl[3] = khp.lodx ? kh_lod_dither(khp.lodt, khp_li != 0) : 0.0f;
                    // kh_draw_textured uploads the slice per submesh (the dither rides along).
                    const uint32_t khp_n = kh_draw_textured(ctx, dev, khp.cbd, g_res.composite_cb, *khp_txm, khp_mid,
                                                            khp.ts_ordered, khr_bound_rs, khp.ctx, khp.ps, khp_lvl, 2);
                    if (khp_rep) continue;
                    kh_stat_add(g_stats.composite_meshes, khp_n);
                    kh_stat_add(g_stats.textured_draws, khp_n);
                }
            };
            khp_geo(false);
            // KH_WSEE: a part drawn here while this cycle's capture stands goes into T and NW as the flush's tail
            // goes (kh_wsee_tail): normal blend, the mesh viewport (the near-gap route lies below the world's range).
            // Its pixel shader writes the coverage as straight alpha here (blendCtl.x is 0 on this pass).
            // A refused re-draw (the bound depth not the capture's) taints, as the flush's does.
            if (khp_o.blend_mode == 0 && khp.vpsel == 0 && kh_wsee_tail_owed()) {
                if (!kh_wsee_tail(ctx, [&](bool) { khp_geo(true); })) {
                    kh_wsee_taint(g_topo_cycles);   // T and NW would miss our layer.
                }
            }
        }

        ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);   // Plain write: the eraser is behind us.
    }

    backup.restore(ctx);   // The engine's viewports too (saved_vp is the same capture, taken after it).
    if (khr_anomalous) {
        // Not an injection for the hybrid guarantee's serial (the next flush must judge the next frame on its own
        // injection, not this frame's rescue) and not a cold-start metric.

        g_rescue_frame_count++;

        if (g_rescue_frame_count < KH_RESCUE_MAX_PER_FRAME) {
            g_ro.injected = false;
            g_ro.inject_attempts = 0;
        }
    } else if (khr_elig_n > 0) {   // KH_SCENE: landed = eligible set non-empty, pre-cull aside.
        kh_stat(g_stats.composite_injections);
        g_composite_inject_serial.fetch_add(1, std::memory_order_relaxed);
        g_composite_land_cycle = g_topo_cycles;   // KH_NO_PARK_LANDED.
    }
    g_ro.opaques_since_inject = 0;   // The flush's repaint check counts from this landing.

    if (!khr_anomalous && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_inject < 0.0f) {
        g_mask.cold_first_inject = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }

}
}