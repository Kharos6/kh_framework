#pragma once

// The injection half (rendering_integration_injection.hpp): mask_cast_engine and the fire's mask-bound test. Never
// include it on its own.
namespace RenderIntegration {

// Is the engine's shadow mask bound as a shader resource right now (PS t0-t32 or VS t0-t32)? The fire's clamp paint
// keys on it: our paint must land before the engine reads the mask. Read at the fire's entry, before any state moves.
inline bool kh_fire_mask_srv_bound(ID3D11DeviceContext* khfb_ctx) {
    if (!g_mask.engine_mask_rtv) return false;
    ID3D11Resource* khfb_mask = nullptr;
    g_mask.engine_mask_rtv->GetResource(&khfb_mask);
    if (!khfb_mask) return false;
    ID3D11ShaderResourceView* khfb_ps[33] = {};
    ID3D11ShaderResourceView* khfb_vs[33] = {};
    khfb_ctx->PSGetShaderResources(0, _countof(khfb_ps), khfb_ps);
    khfb_ctx->VSGetShaderResources(0, _countof(khfb_vs), khfb_vs);
    bool khfb_hit = false;

    for (int si = 0; si < 66 && !khfb_hit; ++si) {   // 33 slots per stage.
        ID3D11ShaderResourceView* sv = si < static_cast<int>(_countof(khfb_ps)) ? khfb_ps[si]
                                                                                : khfb_vs[si - 33];
        if (!sv) continue;
        ID3D11Resource* sr = nullptr;
        sv->GetResource(&sr);
        if (sr == khfb_mask) khfb_hit = true;
        if (sr) sr->Release();
    }

    for (int si = 0; si < static_cast<int>(_countof(khfb_ps)); ++si) {
        KH_SAFE_RELEASE(khfb_ps[si]);
        KH_SAFE_RELEASE(khfb_vs[si]);
    }

    khfb_mask->Release();
    return khfb_hit;
}

inline void mask_cast_engine(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_MASK_CAST);
    // The GPU stamp (KHG_MASK_CAST) is NOT taken here: this runs on every tracked engine draw and nearly always returns
    // at one of the gates below. A stamp pair at the head costs two context End() calls per draw while armed (~3k a
    // frame, most of maskCastUs) and reports the last run - an early return - so gMaskCastUs never measures the fire.
    // It is taken below, once the fire has passed the paint gate: every fire of a frame passes the arm and freshness
    // gates, and each one after the frame's paint returns at that gate, so a stamp taken at the fire's start reports
    // the last of those no-op fires. From the fire's start to the gate nothing issues GPU work (state creation and SRV
    // queries only); the once-per-frame build above the fire (the attach and user-vertex steps and the ladder, which
    // has its own zone) is outside this zone at either place. A repick after the gate (KH_CAST_REPICK,
    // KH_RT_PAINT_READ) re-runs the attach step inside it, and the user-vertex step and the ladder when the repick
    // moved anything.

    // Resolve-keyed re-fires stay ungated: gating this cadence starved the fire on look-down frames.

    if (!ctx) return;

    // Placed at the top of the per-draw fire so it cannot run at the frame boundary (run there, it caused the
    // partitioned-frame overcast). Only removes the wait for a publish: a freeze without one keeps the bridge's
    // rotation (KH_FIRE_ADOPT declines on a frame view older than the cycle).
    if (!g_mask_cast_fired && !g_mask_cast_arm && g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && !g_ls.phase_on_atlas) {
        g_mask_cast_arm = true;
    }
    if (g_mask_cast_fired && g_ls.resolve_seen_since_cast && g_ls.frame_view_valid) {
        // Evidence predating a cascade phase is cleared at atlas entry; a mid-frame atlas re-entry re-earns its
        // evidence at its own resolves.
        if (g_ls.phase_on_atlas) {
        } else if (g_fire_post_scene_cycle == g_topo_cycles) {   // The next frame's mask.
            g_ls.resolve_seen_since_cast = false;
        } else {
            g_mask_cast_arm = true;
            g_mask_cast_fired = false;
        }
    }

    if (!g_mask_cast_arm || g_mask_cast_fired) return;
    g_ls.resolve_seen_since_cast = false;
    if (!g_res.initialized || !g_res.ps_maskcast || !g_res.vs_fullscreen || !g_res.composite_cb) {
        return;
    }
    if (!g_mask.cast_depth) { return; }
    if (!g_mask.cast_fov_valid) { return; }
    if (g_mask.engine_mask_failed) { return; }
    if (!g_ls.frame_view_valid) { return; }

    // No timestamp epoch gate: the view CB re-uploads ~164x/frame, so a timestamp can never carry epoch identity, and a
    // hold would starve cold arms.

    if (g_ls.pub_exact_ms == 0 ||
        steady_now_ms() - g_ls.pub_exact_ms > 250) {
        return;
    }
    // Steady state saturates this within one frame.
    if (g_mask.sweep_settle < g_mask.sweep_need) {
        return;
    }
    if (!g_sun_valid && !kh_sun_axis_witnessed()) {
        g_cast_ready = false;
        g_cast_ready_since_ms = 0;   // Readiness dwell restarts after the sun re-settles.
        return;
    }
    if (!kh_shadow_sun()) {
        g_cast_ready = false;
        g_cast_ready_since_ms = 0;
        return;
    }
    // Absent beats swinging: at a healthy cold this adds at most 1 s to a first cast already seconds out, and a settled
    // steady state never stamps.
    if (g_lock_churn_ms_v != 0 && steady_now_ms() - g_lock_churn_ms_v < 1000) {
        return;
    }

    const uint64_t khcr_now = steady_now_ms();
    const bool khcr_q_churn =
        (g_lock_churn_ms_v == 0 || khcr_now - g_lock_churn_ms_v >= KH_CAST_READY_QUIET_MS);
    // No travel / jump quiet term: a raw publish moves only when the engine's own axis moved, and the cast must move
    // with it.
    const bool khcr_stable = khcr_q_churn;

    if (!khcr_stable) {
        g_cast_ready = false;
        g_cast_ready_since_ms = 0;
        return;
    }

    if (!g_cast_ready) {
        if (g_cast_ready_since_ms == 0) g_cast_ready_since_ms = khcr_now;

        if (khcr_now - g_cast_ready_since_ms < KH_CAST_READY_DWELL_MS) {
            return;
        }

        g_cast_ready = true;
    }

    // Build this frame's map here, before consuming it: render_sun_depth is otherwise reached only later in the frame,
    // so the fire would draw a moved caster at its previous position. Once per frame: its once-per-frame flag returns
    // the injection's second call at once unless KH_SUN_GEN / KH_SUN_RESTEP re-open it (the hash early-outs then keep
    // it cheap). The pass saves its own state and raises g_ro.in_injection, so its draws cannot re-enter here.
    if (g_cast_map_frame != g_ro.frame_cycles) {
        g_cast_map_frame = g_ro.frame_cycles;
        {
            // KH_STEP_OWN_FLAG: the step's buffer writes (a skeletal binding's skin, a cloth's pinned part, a
            // chain's end) are ours: under the flag the hooks stand aside, as at every other step with a context.
            KhInjectionFlagScope khmf_flag;
            g_ro.in_injection = true;
            // The followed transforms first: the map is built from them (KH_SKEL_DRAW: the skinned pose too).
            kh_attach_step(ctx);
        }
        kh_uvs_step(ctx);      // KH_USER_VS: and the deformed meshes - the map casts what the colour passes will draw.
        render_sun_depth(ctx);
    }

    // Fire only through a fresh private sun map (the fire body's freshness bar). Absent beats a rectangle; the arm
    // survives and the next fresh map fires.
    const bool khcr_map_fresh = g_sun_map_valid &&
                                g_sun_map_time >= 0.0f &&
                                effect_time_seconds() - g_sun_map_time < 0.25f;

    if (!khcr_map_fresh) return;
    if (!g_mask.engine_mask_rtv) {  return; }

    g_mask_cast_arm = false;
    g_mask_cast_fired = true;

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_cast < 0.0f) {
        g_mask.cold_first_cast = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }

    // The map this fire consumes is THIS frame's: the once-per-frame render_sun_depth above builds it from this frame's
    // casters before the fire reads it. The freshness bar (khcr_map_fresh) is a 250 ms wall-clock one, which at 60 fps
    // would admit a map fifteen cycles old: it measures liveness, not currency, which is why the build above it and not
    // this bar is what keeps the shadow with its caster. KH_EA_SLAB_DEAD: the fire paints through the sun map alone (a
    // fire without a fresh one returned above), so every fill carries sunMeta.x (kh_fill_sun_tiers_cb).

    if (!g_mask.cast_dss && !g_mask.cast_states_failed) {
        ID3D11Device* dv2 = nullptr;
        ctx->GetDevice(&dv2);

        if (dv2) {
            D3D11_DEPTH_STENCIL_DESC dd2 = {};
            dd2.DepthEnable = FALSE;
            if (FAILED(dv2->CreateDepthStencilState(&dd2, &g_mask.cast_dss))) g_mask.cast_states_failed = true;
            dv2->Release();
        }
    }

    if (!g_mask.min_blend) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) { g_mask.engine_mask_failed = true;  return; }
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        if (FAILED(dev->CreateBlendState(&bd, &g_mask.min_blend))) {
            g_mask.engine_mask_failed = true;
            dev->Release();
            return;
        }

        dev->Release();
    }

    if (!g_fire_lock_valid) {   // frame's first fire: freeze the inputs.
        g_fire_lock = g_mask.cast_depth;   // The live cast depth is the frozen input.
        g_fire_lock->AddRef();
        g_fire_fov2[0] = g_mask.cast_fov[0];
        g_fire_fov2[1] = g_mask.cast_fov[1];
        g_fire_dims2[0] = g_mask.cast_dims[0];
        g_fire_dims2[1] = g_mask.cast_dims[1];
        // Seed the seam view from the carried frame_view: a consistent pair with only a uniform screen-space lag.
        memcpy(g_fire_view2, g_ls.frame_view, sizeof(g_fire_view2));

        // The freeze below replaces it with the cycle's camera (the bridge's clear-time latch, the draw-step retry,
        // or this fire's retry when those failed) under the engine's frame rotation (KH_FIRE_ADOPT); the seed stands
        // where no cycle camera is usable or the era guard refuses it. KH_EA_FIRE_LATCH: never a live mid-frame read
        // beside a usable latch - a later camera than the one the engine's depth was drawn with (the DLS world pass's
        // KH_DLSW_LATCH rule; that pass reconstructs through this freeze).

        if (g_ro.cycle_pv_stale) {
            RVExtBridge::ProjectionViewTransform khfe_pv = {};
            if (RVExtBridge::get_projection_view_transform(khfe_pv)) {   // KH_LATCH_FOLD: no continuity gate.
                if (g_ro.cycle_pv_valid) {
                    memcpy(&khfe_pv.projection[0][0], &g_ro.cycle_pv.projection[0][0],
                           sizeof(khfe_pv.projection));
                }

                g_ro.cycle_pv = khfe_pv;
                g_ro.cycle_pv_valid = true;
                g_ro.cycle_pv_stale = false;
            }
        }

        if (g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale) {
            // The freeze.
            float khfe_bc[3];
            float khfe_pc[3];
            kh_cam_of(g_ro.cycle_pv.view, khfe_bc);
            // frame_view[r*4+c] == cycle_pv.view[r][c] elementwise.
            kh_cam_of(reinterpret_cast<const float(*)[4]>(g_fire_view2), khfe_pc);
            const float khfe_dx = khfe_pc[0] - khfe_bc[0];
            const float khfe_dy = khfe_pc[1] - khfe_bc[1];
            const float khfe_dz = khfe_pc[2] - khfe_bc[2];
            const float khfe_d = sqrtf(khfe_dx * khfe_dx + khfe_dy * khfe_dy + khfe_dz * khfe_dz);

            // KH_FIRE_ADOPT (KH_FX_FRAME_ADOPT): the frozen view is the meshes' - this camera under the engine's
            // frame rotation when that is fresh, as the injection, the flush and the DLS world pass's unfrozen
            // branch take theirs. PSMaskCast and the DLS world pass's frozen branch reconstruct the engine's depth
            // through it (castMat below), and that depth was drawn under the engine's rotation: the bridge's own
            // trails it by up to a frame in a fast turn, and the meshes' shadows on the world trail with it.
            // Adoption keeps the camera (kh_adopt_frame_view rebuilds the translation from the honest one), so
            // the era guard's distance (khfe_bc, above) is the same. A fire armed before this cycle's view publish
            // (the head arm in mask_cast_engine needs none) finds the frame view older than the cycle's clear:
            // adoption declines and the freeze keeps the bridge's rotation. The injection's own adoption decides again
            // at its own time, so in such a cycle the freeze may hold the bridge's rotation while the meshes take the
            // frame's.
            RVExtBridge::ProjectionViewTransform khfe_apv = g_ro.cycle_pv;
            kh_adopt_frame_view(khfe_apv);

            // The shadow freeze. khfe_d is the distance between the incumbent frozen camera and the cycle latch's.
            // The era guard refuses the bridge past 50 m so a foreign bridge never steers the fire, but refusing
            // leaves the incumbent in place; the escape (KH_FIRE_ERA_RECOVER_N consecutive refusals) lets a genuine
            // jump re-converge.
            if (khfe_d <= KH_LATCH_JUMP_M || ++g_fire_era_reject_run >= KH_FIRE_ERA_RECOVER_N) {
                g_fire_era_reject_run = 0;
                memcpy(g_fire_view2, &khfe_apv.view[0][0], sizeof(g_fire_view2));
            }
        }

        const float* fsun = kh_shadow_sun();
        if (!fsun) fsun = g_sun_dir_engine;
        g_fire_sun2[0] = fsun[0];
        g_fire_sun2[1] = fsun[1];
        g_fire_sun2[2] = fsun[2];

        // The engine erases the mask every frame, so a ghost is never stale paint. Once per freeze; re-fires replay it.
        // KH_CAM_HONEST: the frozen view's cofactor inverse rotation in double, laid out for KhCastWorld's row dots
        // (castMat[j][c] = cofactor[j][c] / det); refused (the fill keeps the rows, the transpose) for a determinant
        // 0.1 or more off 1. KH_EA_FIRE_INV: through kh_view_inv_rows, the DLS world pass's twin - the same arithmetic.
        {
            float khoi_inv[3][3];
            g_fire_cast_inv_valid = kh_view_inv_rows(reinterpret_cast<const float(*)[4]>(g_fire_view2), khoi_inv);
            if (g_fire_cast_inv_valid) memcpy(g_fire_cast_inv, khoi_inv, sizeof(g_fire_cast_inv));
        }
        g_fire_lock_valid = true;

    }

    // KH_MASKCAST_NOPAINT: the fire replays per engine resolve, but the clamp paint hands the strength lane
    // (castView[2].w) to exactly ONE fire per frame (the first with the mask SRV-bound, else the frame's last); any
    // other fire would carry a zero lane, which paints 1.0 into a MIN blend, i.e. nothing at full pixel cost, so such a
    // fire skips the draw, the state save and the occupancy build. Once the frame's paint has landed no later fire can
    // paint, so the probe (66 SRV gets per call) is skipped with the draw.
    g_fire_count_frame++;
    g_fire_mask_srv_now = g_fire_clamp_painted == 0 && kh_fire_mask_srv_bound(ctx);   // per-fire (at the statics).
    {
        const bool khnp_last = (g_fire_count_frame >= g_fire_clamp_target);
        const bool khnp_paint = g_fire_clamp_painted == 0 && (g_fire_mask_srv_now || khnp_last);

        if (!khnp_paint) {
            return;
        }
    }
    KH_GPU_SCOPE(ctx, KHG_MASK_CAST);   // KH_PROF: the painting fire (see the head).
    // KH_CAST_REPICK: this paint is the frame's one; it reads the sun map built at the frame's first step, from the
    // samples landed by then. When a newer sample has been published since, a step that picks again moves what it
    // changes, and the map is rebuilt from it before the paint. Nothing waits here (KH_ATTACH_GT_SNAP: "Wait").
    if (g_gts_first_cyc == g_topo_cycles && g_gts_done_pub.load(std::memory_order_acquire) > g_gts_first_done) {
        g_attach_repick_moved = false;
        {
            KhRepickStepScope khrp_scope;
            KhInjectionFlagScope khrp_flag;   // KH_STEP_OWN_FLAG: the step's buffer writes are ours.
            g_ro.in_injection = true;
            kh_attach_step(ctx);
        }
        if (g_attach_repick_moved) {
            kh_uvs_step(ctx);
            g_sun_map_rendered_frame = false;
            render_sun_depth(ctx);
        }
        g_gts_first_done = g_gts_done_pub.load(std::memory_order_acquire);   // Picked against these.
    }
    // KH_RT_PAINT_READ: the own sample still absent - the render thread's read, if both passes agree.
    if (reorder_on_render_thread() && g_attach_n.load(std::memory_order_relaxed) != 0u && g_rtp_cyc != g_topo_cycles &&
        g_gts_first_cyc == g_topo_cycles && kh_rt_own_absent()) {
        g_rtp_cyc = g_topo_cycles;
        { if (kh_stats_on()) ++g_rtp_c.paints; }
        if (kh_rt_paint_read() > 0u) {   // Reads taken, or the own sample arrived meanwhile: the paint's repick either way.
            g_attach_repick_moved = false;
            {
                KhRepickStepScope khrt_scope;
                KhInjectionFlagScope khrt_flag;   // KH_STEP_OWN_FLAG: the step's buffer writes are ours.
                g_ro.in_injection = true;
                kh_attach_step(ctx);
            }
            if (g_attach_repick_moved) {
                kh_uvs_step(ctx);
                g_sun_map_rendered_frame = false;
                render_sun_depth(ctx);
            }
        }
    }
    // KH_SUN_OBJ: a repick's render above can leave no valid map (no caster left in range, a failed upload after the
    // clear). The paint reads a map whole or not at all, so this frame paints nothing; the next fresh map fires.
    if (!g_sun_map_valid) return;
    g_ro.in_injection = true;
    // KH_PRED_ASIDE: the fire's draws run in the draw hook with no StateBackup - a predicate the engine left bound
    // would skip the paint while the fire's bookkeeping (g_fire_clamp_painted, kh_rt_paint_read's reads) advanced
    // past it: a missed paint without a retry. Handed back with the rest of the engine's state below.
    KhPredAside khmc_pa;
    khmc_pa.begin(ctx);
    ID3D11RenderTargetView* old_rtvs[8] = {};
    ID3D11DepthStencilView* old_dsv = nullptr;
    ID3D11UnorderedAccessView* old_uavs[8] = {};
    ctx->OMGetRenderTargets(8, old_rtvs, &old_dsv);
    ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, old_uavs);
    int khf_uav_n = 0, khf_uav_lo = 8;

    for (int ui = 0; ui < 8; ++ui) {
        if (old_uavs[ui]) {
            khf_uav_n++;
            if (ui < khf_uav_lo) khf_uav_lo = ui;
        }
    }

    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);
    ID3D11VertexShader* old_vs = nullptr;
    ID3D11PixelShader* old_ps = nullptr;
    ID3D11InputLayout* old_il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY old_topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* old_cb = nullptr;
    ID3D11BlendState* old_blend = nullptr;
    FLOAT old_bf[4] = {};
    UINT old_bmask = 0xFFFFFFFF;
    ID3D11DepthStencilState* old_dss = nullptr;
    UINT old_sref = 0;
    ctx->VSGetShader(&old_vs, nullptr, nullptr);
    ctx->PSGetShader(&old_ps, nullptr, nullptr);
    ctx->IAGetInputLayout(&old_il);
    ctx->IAGetPrimitiveTopology(&old_topo);
    UINT old_cb_first = 0, old_cb_num = 0;
    UINT old_vs_cb_first = 0, old_vs_cb_num = 0;
    ID3D11Buffer* old_cb1 = nullptr;
    ID3D11Buffer* old_vs_cb1 = nullptr;
    UINT old_cb1_first = 0, old_cb1_num = 0;
    UINT old_vs_cb1_first = 0, old_vs_cb1_num = 0;
    // The cached ID3D11DeviceContext1, and only where the driver honours the offset setters (StateBackup's rule: a Set1
    // the runtime drops leaves our buffer bound).
    ID3D11DeviceContext1* khf_ctx1 = g_cb_offsetting ? kh_ctx1(ctx) : nullptr;
    ID3D11Buffer* old_vs_cb = nullptr;

    if (khf_ctx1) {
        khf_ctx1->PSGetConstantBuffers1(0, 1, &old_cb, &old_cb_first, &old_cb_num);
        khf_ctx1->VSGetConstantBuffers1(0, 1, &old_vs_cb, &old_vs_cb_first, &old_vs_cb_num);
        khf_ctx1->PSGetConstantBuffers1(1, 1, &old_cb1, &old_cb1_first, &old_cb1_num);
        khf_ctx1->VSGetConstantBuffers1(1, 1, &old_vs_cb1, &old_vs_cb1_first, &old_vs_cb1_num);

    } else {
        ctx->PSGetConstantBuffers(0, 1, &old_cb);
        ctx->VSGetConstantBuffers(0, 1, &old_vs_cb);   // The fire sets VS b0.
        ctx->PSGetConstantBuffers(1, 1, &old_cb1);
        ctx->VSGetConstantBuffers(1, 1, &old_vs_cb1);
    }
    // Restored below: every engine vertex shader after the fire would otherwise read our mesh cbd as its constants (the
    // fire runs past the engine's last natural VS-b0 rebind).
    ctx->OMGetBlendState(&old_blend, old_bf, &old_bmask);
    ctx->OMGetDepthStencilState(&old_dss, &old_sref);
    // Array size is the save range: capture, restore, scan and release all follow it.
    // t0..t32: t32 (the far band) is the highest slot of the range the fire binds (t35 is saved apart, below); t28-31
    // are unbound on this path but saved for width parity.
    ID3D11ShaderResourceView* old_ps_srvs[33] = {};
    ctx->PSGetShaderResources(0, _countof(old_ps_srvs), old_ps_srvs);   // Width, not a literal.
    // No VS SRV save: the fire binds none (vs_fullscreen reads nothing).

    // Saved here, nulled below, restored with the rest.
    ID3D11GeometryShader* old_gs = nullptr;
    ID3D11HullShader* old_hs = nullptr;
    ID3D11DomainShader* old_ds2 = nullptr;
    ID3D11RasterizerState* old_rs = nullptr;
    ID3D11ShaderResourceView* khco_old_t35 = nullptr;   // KH_CAST_OCC: t35 sits past
                                                        // old_ps_srvs[33].
    ctx->PSGetShaderResources(35, 1, &khco_old_t35);
    ID3D11SamplerState* khgm_old_s1 = nullptr;   // KH_SHADOW_GATHER: s1, put back with t35.
    ctx->PSGetSamplers(1, 1, &khgm_old_s1);
    ctx->GSGetShader(&old_gs, nullptr, nullptr);
    ctx->HSGetShader(&old_hs, nullptr, nullptr);
    ctx->DSGetShader(&old_ds2, nullptr, nullptr);
    ctx->RSGetState(&old_rs);
    // KH_CAST_SCISSOR: the engine's scissor rects, put back if the fire sets one.
    D3D11_RECT khsr_old[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT khsr_old_n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetScissorRects(&khsr_old_n, khsr_old);
    bool khsr_set = false;
    ctx->OMSetRenderTargets(1, &g_mask.engine_mask_rtv, nullptr);
    D3D11_VIEWPORT vpn = {};
    vpn.Width = g_mask.cast_dims[0];
    vpn.Height = g_mask.cast_dims[1];
    vpn.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpn);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskcast, nullptr, 0);   // Engine linear depth: single-sample.
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // The SV_VertexID fullscreen triangle is clockwise in render-target space, so CullBack never rejects it; DepthClip
    // is moot at z = 0. Default rasterizer: solid fill, scissor off.
    ctx->RSSetState(nullptr);
    // b0 = per-object, b1 = per-frame; both slices upload per fire below (the ad-hoc backup saves/restores both slots).
    ID3D11Buffer* khmc_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khmc_cbs);
    ctx->PSSetConstantBuffers(0, 2, khmc_cbs);
    const FLOAT bfm[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.min_blend, bfm, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
    // The fire's viewport must come from the frozen dims (dimsM = castView[1].zw), never the live g_mask.cast_dims:
    // whenever the live pair diverges mid-frame, later repaints rasterize on one grid and reconstruct on another -
    // scaled darkening and ghosting under motion.
    {

        if (g_fire_dims2[0] <= 0.0f || g_fire_dims2[1] <= 0.0f) {
        } else {
            D3D11_VIEWPORT vpf = {};
            vpf.Width = g_fire_dims2[0];
            vpf.Height = g_fire_dims2[1];
            vpf.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &vpf);
        }
    }

    ctx->PSSetShaderResources(0, 1, &g_fire_lock);   // Frozen: identical replay per batch.
    // KH_CAST_SNAP_FACING: one ulp of the frozen depth's format, relative (fogBelow.z): R32_FLOAT's for an R32_FLOAT
    // view, R16_FLOAT's for every other format the adoption admits.
    float khfs_ulp = 0.0009765625f;
    if (g_fire_lock) {
        D3D11_SHADER_RESOURCE_VIEW_DESC khfs_d = {};
        g_fire_lock->GetDesc(&khfs_d);
        if (khfs_d.Format == DXGI_FORMAT_R32_FLOAT) khfs_ulp = 1.1920929e-7f;
    }
    if (g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER: the cast chain's (null = the default, clamp).
    // t36 is not bound here: PSMaskCast never reads it. This fire binds only what it reads.
    if (g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);   // Terrain snap (full-table
                                                                           // restore covers it).

    auto fill_frame = [&](ConstantData& cbd) {
        for (int r = 0; r < 3; ++r) {
            memcpy(cbd.cast_mat[r], g_fire_view2 + r * 4, 16);
        }

        // Lane hygiene: the memcpy above writes four floats per row, so cast_mat[r][3] lands the view matrix's fourth
        // column - the occupancy grid's arm and origin lanes (castMat[0].w > 0 arms the shader's O(1) path, with
        // localityMeta.w; [1].w / [2].w are the grid origin XZ). Both are enforced here.
        cbd.cast_mat[0][3] = 0.0f;
        cbd.cast_mat[1][3] = 0.0f;
        cbd.cast_mat[2][3] = 0.0f;

        if (g_fire_cast_inv_valid) {
            for (int r = 0; r < 3; ++r) {
                cbd.cast_mat[r][0] = g_fire_cast_inv[r * 3 + 0];
                cbd.cast_mat[r][1] = g_fire_cast_inv[r * 3 + 1];
                cbd.cast_mat[r][2] = g_fire_cast_inv[r * 3 + 2];
            }
        }

        memcpy(cbd.cast_view[0], g_fire_view2 + 12, 16);
        cbd.cast_view[0][3] = (g_mask.sweep_settle >= g_mask.sweep_need) ? 1.0f : 0.0f;
        cbd.cast_view[1][0] = g_fire_fov2[0];
        cbd.cast_view[1][1] = g_fire_fov2[1];
        cbd.cast_view[1][2] = g_fire_dims2[0];
        cbd.cast_view[1][3] = g_fire_dims2[1];
        cbd.cast_view[2][0] = g_fire_sun2[0];   // Frozen sun (see the latch).
        cbd.cast_view[2][1] = g_fire_sun2[1];
        cbd.cast_view[2][2] = g_fire_sun2[2];
        cbd.cast_view[2][3] = g_shadow_map_strength;
        // KH_MASKCAST_NOPAINT: only the frame's painting fire gets here (the gate above; nothing between moves its
        // terms), so this fill carries the frame's one paint.
        g_fire_clamp_painted++;
        kh_fill_occ(cbd);   // terrain-snap lanes (zeroed = shader no-op).
        cbd.fog_below[2] = khfs_ulp;   // KH_CAST_SNAP_FACING: the snap's depth ulp.
        // One origin for every sun matrix this fill carries; the matrices are rebased to it at their own fills below.
        cbd.sun_origin[0] = g_sun_anchor_now[0];
        cbd.sun_origin[1] = g_sun_anchor_now[1];
        cbd.sun_origin[2] = g_sun_anchor_now[2];
        if (g_sun_anchor_now[0] != 0.0f || g_sun_anchor_now[1] != 0.0f ||
             g_sun_anchor_now[2] != 0.0f) {
            cbd.mir_meta[3] = fminf(fmaxf(
                g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
        }
    };

    // Mesh-shaped cast: one fullscreen test against the sun-depth map covers every caster at once.
    // centerSize/sizeAxes carry the combined caster bounds (engine axes) for the shader's locality clamp.
    ConstantData cbd = {};
    fill_frame(cbd);
    cbd.center_size[0] = g_sun_map_bounds[0];
    cbd.center_size[1] = g_sun_map_bounds[1];
    cbd.center_size[2] = g_sun_map_bounds[2];
    cbd.size_axes[0] = g_sun_map_bounds[3] * 2.0f;
    cbd.size_axes[1] = g_sun_map_bounds[4] * 2.0f;
    cbd.size_axes[2] = g_sun_map_bounds[5] * 2.0f;
    kh_fill_sun_tiers_cb(cbd);
    // The cast chain's own compare bias, one lane per tier plus the union.
    cbd.sun_cast_bias[0]  = g_sun2_map_valid ? g_sun2_cast_bias : 0.0f;
    cbd.sun_cast_bias[1]  = g_sun3_map_valid ? g_sun3_cast_bias : 0.0f;
    cbd.sun_cast_bias[2]  = g_sun4_map_valid ? g_sun4_cast_bias : 0.0f;
    cbd.sun_cast_bias[3]  = g_sun5_map_valid ? g_sun5_cast_bias : 0.0f;
    cbd.sun_cast_bias2[0] = g_sun_map_cast_bias;   // The union (sunMeta.x gates its use).
    cbd.locality_meta[0] = static_cast<float>(g_sun_local_count);
    cbd.locality_meta[3] = static_cast<float>(KH_OCC_N);   // KH_OCC_N_LANE.
    // See the shader gate.
    cbd.locality_meta[2] = (g_sun_map_render_time >= 0.0f &&
                            effect_time_seconds() - g_sun_map_render_time > 0.5f)
                         ? 1.0f : 0.0f;

    if (g_sun_local_count > 0 && g_sun_local_count <= 16) {
        for (int li = 0; li < g_sun_local_count; ++li) {
            const float* lb = g_sun_locals.data() + static_cast<size_t>(li) * 6;
            cbd.locality[li * 2][0] = lb[0];
            cbd.locality[li * 2][1] = lb[1];
            cbd.locality[li * 2][2] = lb[2];
            cbd.locality[li * 2 + 1][0] = lb[3];
            cbd.locality[li * 2 + 1][1] = lb[4];
            cbd.locality[li * 2 + 1][2] = lb[5];
        }
    } else if (g_sun_local_count > 16 && kh_upload_locality_ext(ctx)) {
        // Uncapped t2 twin; failure leaves meta.y = 0 and the shader takes the combined-bounds fallback.
        cbd.locality_meta[1] = 1.0f;
        ctx->PSSetShaderResources(2, 1, &g_res.locality_srv);
        // Build and bind the occupancy grid; the shader then gates in O(1). Any failure leaves the per-pixel
        // scan.
        if (kh_cast_occ_build(ctx, cbd)) {
            ctx->PSSetShaderResources(35, 1, &g_res.castocc_srv);
        } else {
            cbd.cast_mat[0][3] = 0.0f;
        }
    }

    // Both slices (centerSize/sizeAxes ride b0; the cast matrices, sun map + locality list ride b1).
    if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd) ||
        !kh_upload_frame_cb(ctx, g_res.composite_frame_cb, cbd)) {
    } else {
        // KH_CAST_SCISSOR: only where a caster's shadow can land; the rest of the screen would write 1 into a
        // MIN (no change).
        bool khsr_unorm = false;
        {
            D3D11_RENDER_TARGET_VIEW_DESC khsr_rd = {};
            g_mask.engine_mask_rtv->GetDesc(&khsr_rd);
            khsr_unorm = khsr_rd.Format == DXGI_FORMAT_R8_UNORM || khsr_rd.Format == DXGI_FORMAT_R16_UNORM;
        }
        D3D11_RECT khsr_rect = {};
        const int khsr_v = kh_cast_scissor(cbd, khsr_unorm, khsr_rect);
        if (khsr_v == 1 && kh_cast_scissor_rs(ctx)) {
            ctx->RSSetState(g_mask.cast_rs_scissor);
            ctx->RSSetScissorRects(1, &khsr_rect);
            khsr_set = true;
            ctx->Draw(3, 0);
            ctx->RSSetState(nullptr);   // The fire's own state again.
        } else if (khsr_v != 2) {
            ctx->Draw(3, 0);
        }
    }

    // The engine's own layout satisfies the API's slot rule (UAV start >= RTV count); a driver-rejected call degrades
    // to no UAV restore, never worse.
    if (khf_uav_n > 0) {
        int khf_uav_hi = 0;

        for (int ui = 7; ui >= 0; --ui) {
            if (old_uavs[ui]) { khf_uav_hi = ui; break; }
        }

        UINT khf_rt_n = 0;

        for (int ri = 7; ri >= 0; --ri) {
            if (old_rtvs[ri]) { khf_rt_n = static_cast<UINT>(ri) + 1; break; }
        }

        static const UINT khf_keep[8] = {
            0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
            0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
        };
        ctx->OMSetRenderTargetsAndUnorderedAccessViews(
            khf_rt_n, old_rtvs, old_dsv,
            static_cast<UINT>(khf_uav_lo),
            static_cast<UINT>(khf_uav_hi - khf_uav_lo + 1),
            old_uavs + khf_uav_lo, khf_keep);
    } else {
        ctx->OMSetRenderTargets(8, old_rtvs, old_dsv);
    }

    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    // KH_CLASS_INST: the engine's VS / PS come back without class instances (the save above takes none, as for the
    // GS / HS / DS below; the fire runs in the draw hook outside any StateBackup). Accepted as StateBackup's GS / HS /
    // DS are: the engine is not known to use dynamic linkage.
    ctx->VSSetShader(old_vs, nullptr, 0);
    ctx->PSSetShader(old_ps, nullptr, 0);
    ctx->IASetInputLayout(old_il);
    ctx->IASetPrimitiveTopology(old_topo);
    // Our composite_cb is bound here, so the buffer always changes at this call.
    if (khf_ctx1) {
        if (old_cb && old_cb_num > 0) {
            khf_ctx1->PSSetConstantBuffers1(0, 1, &old_cb, &old_cb_first, &old_cb_num);
        } else {
            ctx->PSSetConstantBuffers(0, 1, &old_cb);
        }

        if (old_vs_cb && old_vs_cb_num > 0) {
            khf_ctx1->VSSetConstantBuffers1(0, 1, &old_vs_cb, &old_vs_cb_first, &old_vs_cb_num);
        } else {
            ctx->VSSetConstantBuffers(0, 1, &old_vs_cb);
        }

        // Slot 1 (CB split): same offset-aware restore discipline.
        if (old_cb1 && old_cb1_num > 0) {
            khf_ctx1->PSSetConstantBuffers1(1, 1, &old_cb1, &old_cb1_first, &old_cb1_num);
        } else {
            ctx->PSSetConstantBuffers(1, 1, &old_cb1);
        }

        if (old_vs_cb1 && old_vs_cb1_num > 0) {
            khf_ctx1->VSSetConstantBuffers1(1, 1, &old_vs_cb1, &old_vs_cb1_first, &old_vs_cb1_num);
        } else {
            ctx->VSSetConstantBuffers(1, 1, &old_vs_cb1);
        }
    } else {
        ctx->PSSetConstantBuffers(0, 1, &old_cb);
        ctx->VSSetConstantBuffers(0, 1, &old_vs_cb);
        ctx->PSSetConstantBuffers(1, 1, &old_cb1);
        ctx->VSSetConstantBuffers(1, 1, &old_vs_cb1);
    }

    ctx->OMSetBlendState(old_blend, old_bf, old_bmask);
    ctx->OMSetDepthStencilState(old_dss, old_sref);
    // Covers t0/t11 by construction.
    ctx->PSSetShaderResources(0, _countof(old_ps_srvs), old_ps_srvs);   // t0..t32 (t35 below); width, not a literal.
    kh_bc_forget();   // KH_BIND_FILTER: t14 - t18.
    ctx->GSSetShader(old_gs, nullptr, 0);
    ctx->HSSetShader(old_hs, nullptr, 0);
    ctx->DSSetShader(old_ds2, nullptr, 0);
    ctx->RSSetState(old_rs);
    if (khsr_set) ctx->RSSetScissorRects(khsr_old_n, khsr_old_n ? khsr_old : nullptr);   // KH_CAST_SCISSOR.
    ctx->PSSetShaderResources(35, 1, &khco_old_t35);
    KH_SAFE_RELEASE(khco_old_t35);
    ctx->PSSetSamplers(1, 1, &khgm_old_s1);   // KH_SHADOW_GATHER.
    KH_SAFE_RELEASE(khgm_old_s1);
    khmc_pa.end();   // KH_PRED_ASIDE.
    g_ro.in_injection = false;
    KH_SAFE_RELEASE(old_vs_cb);
    KH_SAFE_RELEASE(old_vs_cb1);

    for (int r = 0; r < 8; ++r) KH_SAFE_RELEASE(old_rtvs[r]);

    for (int u = 0; u < 8; ++u) KH_SAFE_RELEASE(old_uavs[u]);

    KH_SAFE_RELEASE(old_dsv);
    KH_SAFE_RELEASE(old_vs);
    KH_SAFE_RELEASE(old_ps);
    KH_SAFE_RELEASE(old_il);
    KH_SAFE_RELEASE(old_cb);
    KH_SAFE_RELEASE(old_cb1);
    KH_SAFE_RELEASE(old_blend);
    KH_SAFE_RELEASE(old_dss);

    for (int s16 = 0; s16 < static_cast<int>(_countof(old_ps_srvs)); ++s16) {
        KH_SAFE_RELEASE(old_ps_srvs[s16]);
    }
    KH_SAFE_RELEASE(old_gs);
    KH_SAFE_RELEASE(old_hs);
    KH_SAFE_RELEASE(old_ds2);
    KH_SAFE_RELEASE(old_rs);
}

inline void mask_note_draw(ID3D11DeviceContext* ctx) {
    if (!g_mask.atlas_bound || !ctx) return;
    g_mask.atlas_bound = false;   // One record per pass segment is plenty.
    g_ls.atlas_last_seen = effect_time_seconds();   // The engine consumed the held atlas:
                                                    // it is alive.
    if (g_mask.rt_is_resolve) {
        resolve_pair_capture(ctx);
    }
}

// Lazy SRV over the atlas; typeless depth formats map to their colour-read equivalents. Callers: the injection, the
// in-front slice (kh_infront_inject) and the flush (flush_locked - a park's flush on the game thread included).
inline bool shadow_live_ensure_srv() {
    if (g_ls.atlas_srv) return true;
    if (!g_ls.atlas_tex || g_ls.srv_failed) return false;

    const DXGI_FORMAT fmt = kh_atlas_view_fmt(g_ls.atlas_fmt);

    if (fmt == DXGI_FORMAT_UNKNOWN) { g_ls.srv_failed = true;  return false; }

    ID3D11Device* dev = nullptr;
    g_ls.atlas_tex->GetDevice(&dev);
    if (!dev) { g_ls.srv_failed = true;  return false; }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    const HRESULT hr = dev->CreateShaderResourceView(g_ls.atlas_tex, &sd, &g_ls.atlas_srv);
    dev->Release();

    if (FAILED(hr) || !g_ls.atlas_srv) {
        g_ls.atlas_srv = nullptr;
        g_ls.srv_failed = true;
        return false;
    }

    return true;
}
}