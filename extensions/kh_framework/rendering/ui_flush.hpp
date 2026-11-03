#pragma once

// The injection half (rendering_integration_injection.hpp): the UI flush, the UI driver and the Draw3D event handler.
// Never include it on its own.
namespace RenderIntegration {

// KH_UI_BB_UNWIND: KhUnwindRelease (the guard flush_ui_locked holds the backbuffer in) is declared with the PIP
// chain (rendering/pip.hpp), its other holder (kh_pip_fx's PIP colour texture), which comes first in the unit.

// KH_SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static uint64_t g_ui_prev_scene_frame = 0;
static bool     g_ui_prev_from_present = false;   // KH_UI_ORDINAL_SRC: the clock g_ui_prev_scene_frame came from.
// KH_UI_LEARN_OWN: khul_present - the caller bound a view of the swap chain's buffer (kh_present_cb); the lock path
// (false) draws into whatever the engine had bound at the grant, which need not be a backbuffer (KH_UI_DIAG). Only
// the former is learned as one, except while no Present hook is installed - the lock path is then the only learner.
inline void flush_ui_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool khul_present) {
    KH_PROF_SCOPE(KHP_FLUSH_UI);
    KH_GPU_SCOPE(ctx, KHG_FLUSH_UI);   // KH_PROF.
    const float snapshot_now = effect_time_seconds();
    static std::vector<std::pair<uint64_t, RenderObject>> passes;   // Key = creation seq.
    passes.clear();

    LARGE_INTEGER khusn_t0 = {}, khusn_t1 = {};
    QueryPerformanceCounter(&khusn_t0);

    {
        kh_scene_sync();   // KH_SCENE.

        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& khu_live = g_scene.objs[khsc_i];
            if (!(khu_live.visible && khu_live.fullscreen && khu_live.affect_ui)) continue;
            bool expired = false;
            const float env = lifetime_envelope(khu_live, snapshot_now, expired);
            if (expired) continue;   // The game thread erases it (flush_frame's census: KH_NO_PARK_EXPIRY).
            RenderObject o = khu_live;
            o.color[3] *= env;
            passes.emplace_back(o.seq, o);
        }
    }

    QueryPerformanceCounter(&khusn_t1);

    if (passes.empty()) return;

    std::sort(passes.begin(), passes.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);

    if (dsv) {
        dsv->Release();
        if (rtv) rtv->Release();
        return;
    }

    if (!rtv) {  return; }
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    if (!res) {  return; }
    // The backbuffer identity the mask machinery learns is the GetResource pointer - the same token the OMSet hook's
    // classify resolves (weak identity).
    void* khum_bb_id = static_cast<void*>(res);
    ID3D11Texture2D* bb = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb));
    res->Release();
    if (FAILED(hr) || !bb) {  return; }
    // KH_UI_BB_UNWIND: a throw below (an allocation in an ensure, a load, a pass) reaches the Present callback's
    // or the command's catch, which cannot see bb - and a reference stranded on a swapchain buffer fails the
    // engine's next ResizeBuffers (KH_PRESENT_RESIZE). Released on that unwind; every return below releases it.
    KhUnwindRelease<ID3D11Texture2D> khub_bb(bb);
    D3D11_TEXTURE2D_DESC td = {};
    bb->GetDesc(&td);
    bool fmt_ok;

    switch (td.Format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            fmt_ok = true; break;
        default:
            fmt_ok = false; break;
    }

    if (!fmt_ok || td.SampleDesc.Count > 1) {
        // KH_UI_BB_REPORT: said only of the swap chain's buffer - the lock path's target may be any engine target.
        if (khul_present) {   // KH_EA_ERR_CLASS: the swap chain's configuration - a warning (logged, never thrown).
            kh_log_once_safe("KH RenderIntegration: UI flush: UI passes need an 8-bit (RGBA8 / BGRA8) single-sample "
                             "backbuffer; none is drawn on this one");
        }
        bb->Release();
        return;
    }

    if (!kh_ensure_ok("UI-flush resources", ensure_resources(dev))) { bb->Release(); return; }
    if (!kh_ensure_ok("UI-flush effect shader", ensure_effect_shader(dev))) { bb->Release(); return; }

    // The mask is live when the machine's clear fired for this very buffer this frame; UI-mode passes skip otherwise
    // (one-frame learn latency at spawn - the designed fail-safe).
    if (khul_present || !g_present_hook_active.load(std::memory_order_relaxed)) {   // KH_UI_LEARN_OWN.
        kh_ui_mask_learn(khum_bb_id, td.Width, td.Height);
    }
    {
        bool khum_from_present = false;
        const uint64_t khum_scene_frame = kh_ui_frame_ordinal(&khum_from_present);   // KH_PRESENT_SEQ: the
                                                                                     // dark-driver test's clock.
        // KH_UI_ORDINAL_SRC: the previous reading counts only on the same clock - the Present count and the flush
        // ordinal count different events at different rates, so across a switch between them (a stale spell of the
        // Present path: a hitch, an unfocused stretch, the frames after alt-tab) one could pass the other by more than
        // one, read as a dark driver and drop that frame's UI effect. Zero is "no previous", as at a reset.
        if (khum_from_present != g_ui_prev_from_present) g_ui_prev_scene_frame = 0;
        g_ui_prev_from_present = khum_from_present;
        const bool khum_driver_dark =
            g_ui_prev_scene_frame != 0 &&
            khum_scene_frame > g_ui_prev_scene_frame + 1;
        g_ui_prev_scene_frame = khum_scene_frame;

        if (khum_driver_dark && g_ui_mask.compose_serial != 0 &&
            g_ui_mask.compose_serial != g_ui_mask.applied_serial) {
            g_ui_mask.mask_valid = false;   // Suspect coverage: skip, resync.
        }
    }

    const bool khum_mask_live = g_ui_mask.mask_valid &&
                                g_ui_mask.phase_bb == khum_bb_id &&
                                g_ui_poison_run < KH_UI_POISON_DEAD;   // KH_UI_POISON_RUN: a dead mask draws nothing.

    // KH_UI_POISON_RUN's readback. A same-frame CPU verdict is unobtainable here (the copy is read at a later flush,
    // without waiting); the same-frame verdict is PSEffect's guard. This lagged one stands every pass down at the
    // flushes after the KH_UI_POISON_DEADth full reading in a row, until a reading is not full.
    if (td.Width >= 512 && td.Height >= 512) {
        // KH_UI_STAGE_FMT: the copies need a stage of the backbuffer's format, and the capture group that also
        // releases it (release_bb_capture) is remade only when a pass captures - a format change is caught here.
        if (g_res.ui_alpha_stage) {
            D3D11_TEXTURE2D_DESC khas_sd = {};
            g_res.ui_alpha_stage->GetDesc(&khas_sd);
            if (khas_sd.Format != td.Format) KH_SAFE_RELEASE(g_res.ui_alpha_stage);
        }
        if (!g_res.ui_alpha_stage) {
            D3D11_TEXTURE2D_DESC khas_td = {};
            khas_td.Width = 64;
            khas_td.Height = 64;
            khas_td.MipLevels = 1;
            khas_td.ArraySize = 1;
            khas_td.Format = td.Format;
            khas_td.SampleDesc.Count = 1;
            khas_td.Usage = D3D11_USAGE_STAGING;
            khas_td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            dev->CreateTexture2D(&khas_td, nullptr, &g_res.ui_alpha_stage);
            g_ui_mask.alpha_copy_pending = false;
        }

        if (g_res.ui_alpha_stage) {
            if (g_ui_mask.alpha_copy_pending) {
                D3D11_MAPPED_SUBRESOURCE khas_map = {};
                const HRESULT khas_hr = ctx->Map(g_res.ui_alpha_stage, 0, D3D11_MAP_READ,
                                                 D3D11_MAP_FLAG_DO_NOT_WAIT, &khas_map);

                if (SUCCEEDED(khas_hr)) {
                    int khas_min = 255;   // The poison rule reads the minimum alone.

                    for (UINT khas_y = 0; khas_y < 64; ++khas_y) {
                        const uint8_t* khas_row =
                            static_cast<const uint8_t*>(khas_map.pData) + khas_y * khas_map.RowPitch;

                        for (UINT khas_x = 0; khas_x < 64; ++khas_x) {
                            const int khas_a = khas_row[khas_x * 4 + 3];   // A is byte 3 in RGBA8 and
                                                                           // BGRA8.
                            if (khas_a < khas_min) khas_min = khas_a;
                        }
                    }

                    ctx->Unmap(g_res.ui_alpha_stage, 0);
                    g_ui_poison_run = (khas_min == 255) ? g_ui_poison_run + 1 : 0;   // KH_UI_POISON_RUN.
                    g_ui_mask.alpha_copy_pending = false;
                } else if (khas_hr != DXGI_ERROR_WAS_STILL_DRAWING) {
                    g_ui_mask.alpha_copy_pending = false;
                }
            }

            if (!g_ui_mask.alpha_copy_pending) {
                static const float khas_fx[8] = { 0.18f, 0.47f, 0.79f, 0.23f,
                                                  0.68f, 0.31f, 0.58f, 0.86f };
                static const float khas_fy[8] = { 0.21f, 0.16f, 0.24f, 0.52f,
                                                  0.47f, 0.77f, 0.84f, 0.69f };
                KhPredAside khas_pa;   // KH_UI_STAGE_PRED: the copies run ahead of the flush's StateBackup.
                khas_pa.begin(ctx);

                for (UINT khas_s = 0; khas_s < 8; ++khas_s) {
                    D3D11_BOX khas_box = {};
                    khas_box.left = static_cast<UINT>(td.Width * khas_fx[khas_s]);
                    khas_box.right = khas_box.left + 64;
                    khas_box.top = static_cast<UINT>(td.Height * khas_fy[khas_s]);
                    khas_box.bottom = khas_box.top + 8;
                    khas_box.front = 0;
                    khas_box.back = 1;
                    ctx->CopySubresourceRegion(g_res.ui_alpha_stage, 0,
                                               0, khas_s * 8, 0, bb, 0, &khas_box);
                }
                khas_pa.end();

                g_ui_mask.alpha_copy_pending = true;
            }
        }
    }
    const uint64_t khum_serial = g_ui_mask.compose_serial;

    if (khum_serial != 0 && khum_serial == g_ui_mask.applied_serial) {
        bb->Release();
        return;
    }

    // KH_UI_CYCLE_CAM: the camera that rendered the frame this lane draws over - the cycle's boundary camera, as
    // the scene chain's inverse takes it (flush_locked) - so a pass that reads the scene depth (a localized mask,
    // Pulse, a user effect) reconstructs the world from the eye that wrote it. The bridge's camera at this call can
    // be a frame ahead of it on a moving camera, which slid a localized mask's edge off the scene. No usable latch
    // (none yet, this cycle's fetch failed, a degenerate projection): the bridge's. Written by the depth
    // clear hook on this same context, which the engine never drives beside the Present (or under the park).
    RVExtBridge::ProjectionViewTransform pv = {};
    if (g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && fabsf(g_ro.cycle_pv.projection[2][2]) > 1.0e-6f) {
        pv = g_ro.cycle_pv;
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        bb->Release();
        return;
    }
    // KH_FX_FRAME_ADOPT (KH_UI_FX_PARITY): under the engine's frame rotation when fresh, as the scene chain takes
    // its camera (flush_locked adopts whichever source it took) - its meshes, its frame and its user lanes alike.
    kh_adopt_frame_view(pv);
    // KH_UI_SERIAL_LATE: applied once the camera is in hand - marked before it, a failed read skipped this
    // compose's passes instead of retrying them at the next Present.
    g_ui_mask.applied_serial = khum_serial;
    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    float inv_view_proj[4][4] = {};
    bool need_inverse = false;

    for (const auto& f : passes) {
        if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse) ||
            f.second.effect == KH_EFFECT_CUSTOM) { need_inverse = true; break; }
    }

    // KH_UI_FX_PARITY: the scene chain's depth recipe (flush_locked: KH_FX_ENGINE_PAIR, KH_FX_CAM_REL) over this lane's
    // camera (KH_UI_CYCLE_CAM, above). The depth is decoded with the pair the engine encoded it with when that was read
    // (the camera's own pair otherwise), and re-encoded through the same pair in the inverse; the inverse is
    // of the rotation-only view, the camera carried in fxCam - an absolute inverse cancels the camera's kilometres per
    // pixel in fp32, which shimmers a localized mask's edge.
    float khui_m22 = pv.projection[2][2], khui_m32 = pv.projection[3][2];
    if (g_ro.engine_proj_valid) {
        khui_m22 = g_ro.engine_m22;
        khui_m32 = g_ro.engine_m32;
    }
    {
        // KH_FX_PAIR_HOLD: through the hold guard, as the scene chain takes it (kh_fx_depth_pair_guard's note).
        // Where this cycle's scene flush decided (the memo's serial and cycle) its verdict is replayed, so this
        // lane reads with the chain's pair (KH_FX_PAIR_KEY: an engine pair that latched after a fallback flush is
        // judged afresh unless that verdict held). KH_FX_PAIR_ONE: on the fallback route this lane takes that
        // verdict's pair (g_khfx_pair). None of this cycle's (no flush ran, or it left before its frame): a
        // verdict of this Present's own under a serial no flush can name, so no stale one is replayed.
        const uint64_t khui_fs = g_cc_flush_serial.load(std::memory_order_relaxed);
        const bool khui_same = khui_fs == g_khfx_verdict_serial &&
                               g_khfx_verdict_cycle == g_topo_cycles_pub.load(std::memory_order_relaxed);
        if (khui_same && !g_ro.engine_proj_valid) {
            khui_m22 = g_khfx_pair[0];
            khui_m32 = g_khfx_pair[1];
        } else {
            ConstantData khui_pg = {};
            khui_pg.depth_params[0] = khui_m22;
            khui_pg.depth_params[1] = khui_m32;
            kh_fill_depth_range_cb(khui_pg);
            kh_fx_depth_pair_guard(khui_pg, khui_same ? khui_fs : ((1ull << 63) | ++g_khfx_ui_serial));
            khui_m22 = khui_pg.depth_params[0];
            khui_m32 = khui_pg.depth_params[1];
        }
    }
    float khui_fx_cam[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    kh_cam_of(pv.view, khui_fx_cam);   // KH_CAM_HONEST.
    float khui_dvp[4][4];
    {
        float khui_dproj[4][4], khui_dview[4][4];
        memcpy(khui_dproj, pv.projection, sizeof(khui_dproj));
        khui_dproj[2][2] = khui_m22;
        khui_dproj[3][2] = khui_m32;
        memcpy(khui_dview, pv.view, sizeof(khui_dview));
        khui_dview[3][0] = khui_dview[3][1] = khui_dview[3][2] = 0.0f;
        khui_dview[3][3] = 1.0f;
        mul_4x4(khui_dview, khui_dproj, khui_dvp);
    }

    const bool has_inverse = need_inverse && inverse_4x4(khui_dvp, inv_view_proj);
    const double now_d = effect_time_seconds_d();
    const bool depth_ok = g_res.depth_srv != nullptr;
    StateBackup backup;
    backup.capture(ctx);
    // KH_FX_TEX: finished background loads land here too - a scene of UI-lane passes alone never reaches the scene
    // flush's pump (its list keeps UI-only passes out), and a user pass's textures would stay pending. A placement
    // bumps the page generation; every material draw re-fixes its base against it (kh_mat_gpu_ensure).
    kh_tex_async_pump(ctx, dev);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Fail-dark: a failed frame upload stands every draw down through the khuf_frame_ok latch - never a draw against a
    // stale b1. b0 = per-object, b1 = per-frame.
    ID3D11Buffer* khui_cbs[2] = { g_res.constant_buffer, g_res.frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khui_cbs);
    ctx->PSSetConstantBuffers(0, 2, khui_cbs);
    bool khuf_frame_ok = false;

    {
        ConstantData khuf_cbf = {};
        memcpy(khuf_cbf.view_proj, view_proj, sizeof(view_proj));
        memcpy(khuf_cbf.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        if (has_inverse) memcpy(khuf_cbf.fx_cam, khui_fx_cam, sizeof(khuf_cbf.fx_cam));   // KH_UI_FX_PARITY.
        {   // KH_USER_LANES.
            float khuf_cam[3];
            kh_cam_of(pv.view, khuf_cam);
            kh_fill_user_cam_cb(khuf_cbf, khuf_cam, pv.view);
        }
        khuf_frame_ok = kh_upload_frame_cb(ctx, g_res.frame_cb, khuf_cbf);
    }
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->RSSetState(g_res.rasterizer);
    const FLOAT bf[4] = { 0, 0, 0, 0 };

    // The viewport at entry is whatever the engine last set - at the lock path's grant (flushUIRender: the Draw2D
    // driver's call, or a script's Draw EH) or at the Present - not necessarily the full target, so a fullscreen
    // triangle through it could be clipped. Own the viewport explicitly: full target for our passes, engine state
    // restored after.
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&n_saved_vp, saved_vp);

    {
        D3D11_VIEWPORT full = {};
        full.Width = static_cast<FLOAT>(td.Width);
        full.Height = static_cast<FLOAT>(td.Height);
        full.MinDepth = 0.0f;
        full.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &full);
    }

    // 1 capture + N draws (the last one straight into the backbuffer), replacing N captures + N draws.
    bool khup_chain = false;   // A chain is open; khup_src holds the live source.
    int  khup_src = 0;   // 0 = bb_tex, 1 = ui_chain_tex.
    ID3D11ShaderResourceView* khup_srvs[2] = { nullptr, nullptr };
    ID3D11RenderTargetView*   khup_rtvs[2] = { nullptr, nullptr };
    // The backbuffer RTV is bound at entry (the gate verified it); hold a reference for the final pass + custom
    // passes.
    ID3D11RenderTargetView* khup_bb_rtv = nullptr;
    ID3D11DepthStencilView* khup_bb_dsv = nullptr;   // nullptr by the gate; carried for symmetry.
    ctx->OMGetRenderTargets(1, &khup_bb_rtv, &khup_bb_dsv);
    // KH_UI_BB_UNWIND: the view holds the buffer too. Ahead of khup_om, so an unwind restores the OM first.
    KhUnwindRelease<ID3D11RenderTargetView> khub_rtv(khup_bb_rtv);
    KhUnwindRelease<ID3D11DepthStencilView> khub_dsv(khup_bb_dsv);
    // KH_UI_VIEW_FMT: the lane reads the buffer in the texture's own format (its captures, chain and views), so a
    // bound view of another format - an _SRGB view on a UNORM flip-model buffer - would encode its result a second
    // time on the final write. Such a view is replaced, for this flush, by one of the texture's format (a TYPELESS
    // texture keeps the engine's: its captures cannot be read anyway). Released below, as the engine's would be. Only
    // on a live mask: every pass skips a dead one, so its view is never bound.
    if (khum_mask_live && khup_bb_rtv && td.Format != DXGI_FORMAT_R8G8B8A8_TYPELESS &&
        td.Format != DXGI_FORMAT_B8G8R8A8_TYPELESS) {
        D3D11_RENDER_TARGET_VIEW_DESC khvf_d = {};
        khup_bb_rtv->GetDesc(&khvf_d);
        ID3D11RenderTargetView* khvf_own = nullptr;
        if (khvf_d.Format != td.Format && SUCCEEDED(dev->CreateRenderTargetView(bb, nullptr, &khvf_own)) &&
            khvf_own) {
            khup_bb_rtv->Release();
            khup_bb_rtv = khvf_own;
        }
    }
    // The gate verified slot 0 and the DSV only; the single-slot rebinds below null slots 1-7, so the full set is
    // captured here and restored at the tail.
    KhOmSave khup_om;
    khup_om.capture(ctx);

    bool                      khuf_live = false;
    ConstantData              khuf_cbd;
    ID3D11ShaderResourceView* khuf_lut = nullptr;
    const RenderObject*       khuf_obj = nullptr;   // KH_FX_TEX_ALL: the deferred pass's object (in passes).

    auto khuf_flush = [&](bool khuf_final) {
        if (!khuf_live) return;
        khuf_live = false;
        KhGpuPassScope khgq_upass(ctx, 1u, static_cast<int>(khuf_cbd.fx_meta[0] + 0.5f), false,   // KH_GPU_PASS.
                                  static_cast<uint32_t>(khuf_cbd.fuse_meta[0] + 0.5f));
        // KH_FX_TEX_ALL: a builtin pass's own textures - the lanes before the probe side draw takes khuf_cbd, the
        // pages bound to the flush's end on every exit (the scene chain's rule).
        ID3D11ShaderResourceView* khuf_tex[KH_MAT_USER_MAPS] = {};
        KhFxTexScope khuf_ts;
        if (khuf_obj && kh_fx_tex_on(*khuf_obj)) {
            kh_fx_tex_fill(dev, ctx, khuf_cbd, *khuf_obj, khuf_tex);
            khuf_ts.bind(ctx, khuf_tex);
        }
        // KH_FX_SIDE: the lane's coverage probe (side id 30, 1 x 1) of this pass's own source, drawn before the
        // pass's constants go up; they then arm matCtl.z, and t5 carries it.
        bool khuf_probe = false;
        if (khuf_frame_ok && khuf_cbd.center_size[3] > 1.5f &&
            kh_fx_side_draw(dev, ctx, 2, khuf_cbd, 30.0f, 1u, 1u, khup_srvs[khup_src], g_res.constant_buffer)) {
            khuf_probe = true;
            khuf_cbd.mat_ctl[2] = 1.0f;
        }
        // KH_GLOW_PYR: the scene chain's rule, on this pass's own source (the spill lane's premultiply rides its w).
        bool khug_on = false;
        const int khug_e = static_cast<int>(khuf_cbd.fx_meta[0] + 0.5f);
        if (khuf_frame_ok && kh_glow_effect(khug_e)) {
            khug_on = kh_glow_build(dev, ctx, g_res.glow_pyr[1], khup_srvs[khup_src], khuf_cbd.fx_meta[2],
                                    khuf_cbd.fx_meta[3], khuf_cbd.center_size[3], khug_e, khuf_cbd.fx0[0],
                                    g_res.constant_buffer);
            kh_glow_lanes(khuf_cbd, khug_on ? &g_res.glow_pyr[1] : nullptr);
        }
        // KH_ANA_PYR: anamorphic's one-axis pyramid, the same rule (the spill lane's premultiply rides its w too).
        const KhAnaPyr* khua_pyr = nullptr;
        if (khuf_frame_ok && khug_e == static_cast<int>(EffectId::Anamorphic)) {
            const int khua_ax = khuf_cbd.fx1[0] > 0.5f ? 1 : 0;
            if (kh_ana_build(dev, ctx, g_res.ana_pyr[1][khua_ax], khup_srvs[khup_src], khuf_cbd.fx_meta[2],
                             khuf_cbd.fx_meta[3], khuf_cbd.center_size[3], khua_ax, khuf_cbd.fx0[0],
                             g_res.constant_buffer))
                khua_pyr = &g_res.ana_pyr[1][khua_ax];
            kh_ana_lanes(khuf_cbd, khua_pyr);
        }
        // Unbind the source slot before the destination's RTV turn.
        ID3D11ShaderResourceView* khuf_null = nullptr;
        ctx->PSSetShaderResources(0, 1, &khuf_null);
        if (khuf_final) ctx->OMSetRenderTargets(1, &khup_bb_rtv, khup_bb_dsv);
        else            ctx->OMSetRenderTargets(1, &khup_rtvs[khup_src ^ 1], nullptr);
        ID3D11ShaderResourceView* khuf_srvs[2] = { khup_srvs[khup_src],
                                                   depth_ok ? g_res.depth_srv : nullptr };
        ctx->PSSetShaderResources(0, 2, khuf_srvs);
        if (khuf_lut) ctx->PSSetShaderResources(19, 1, &khuf_lut);

        if (!khuf_frame_ok ||
            !kh_upload_obj_cb(ctx, g_res.constant_buffer, khuf_cbd)) return;

        // KH_FX_LEAN_PASS (the note at Resources::ps_effect_lean): the UI lane's passes are never armed.
        ctx->PSSetShader((g_res.ps_effect_lean && !(khuf_cbd.fx_tex[1][2] > 0.5f)) ? g_res.ps_effect_lean
                                                                                   : g_res.ps_effect, nullptr, 0);
        // RGB-only replace blend on the final write, so the mask channel itself survives the view.
        ctx->OMSetBlendState(khuf_final ? g_res.blend_ui_replace : nullptr,
                             bf, 0xFFFFFFFF);
        KhGlowBind khug_b;   // KH_GLOW_PYR.
        if (khug_on) khug_b.bind(ctx, g_res.glow_pyr[1]);
        KhAnaBind khua_b;   // KH_ANA_PYR.
        if (khua_pyr) khua_b.bind(ctx, *khua_pyr);
        if (khuf_probe) ctx->PSSetShaderResources(5, 1, &g_res.fxs_srv[2]);   // KH_FX_SIDE.
        ctx->Draw(3, 0);
        khua_b.unbind();
        khug_b.unbind();
        if (khuf_probe) {   // Unbound at once: the next pass's probe renders into it.
            ID3D11ShaderResourceView* khuf_pn = nullptr;
            ctx->PSSetShaderResources(5, 1, &khuf_pn);
        }
        if (!khuf_final) khup_src ^= 1;

    };


    // The single capture: the backbuffer as the engine's UI left it (rgb + the live coverage in alpha: KH_UI_COV_LIVE),
    // into bb_tex = chain 0. False: refused (the loop ends).
    auto khup_open = [&]() -> bool {
        std::string khup_cap = ensure_backbuffer_capture(dev, ctx, bb, td);
        if (khup_cap.empty()) khup_cap = ensure_ui_chain(dev, td);
        // KH_EA_UI_CAP_REPORT: reported once per text, as the user pass's capture (KH_UI_USER_CAP) and the ensures
        // above are - a failure here ends the flush's pass loop (both callers break).
        if (!kh_ensure_ok("UI-flush capture", khup_cap)) return false;
        khup_srvs[0] = g_res.bb_srv;
        khup_rtvs[0] = g_res.bb_rtv;
        khup_srvs[1] = g_res.ui_chain_srv;
        khup_rtvs[1] = g_res.ui_chain_rtv;
        khup_src = 0;
        khup_chain = true;
        return true;
    };
    // A pass's constants (KH_UI_BRIGHT_ID builds a held pass's at its own turn, with this).
    auto khu_cbd_of = [&](const RenderObject& o, bool khu_spill) {
        ConstantData cbd = {};
        memcpy(cbd.view_proj, view_proj, sizeof(view_proj));
        memcpy(cbd.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];
        cbd.center_size[2] = o.pos[1];
        cbd.center_size[3] =
            o.effect == KH_EFFECT_CUSTOM ? 1.25f : (khu_spill ? 3.0f : 2.0f);
        cbd.size_axes[3] = khu_spill ? 0.0f : static_cast<float>(o.blend_mode);
        kh_fill_fx_params_cb(cbd, o);

        cbd.fx_meta[0] = static_cast<float>(o.effect);
        cbd.fx_meta[1] = kh_fx_time(o, now_d);
        cbd.fx_meta[2] = static_cast<float>(td.Width);
        cbd.fx_meta[3] = static_cast<float>(td.Height);
        cbd.depth_params[0] = khui_m22;   // KH_UI_FX_PARITY: the encode pair.
        cbd.depth_params[1] = khui_m32;
        // KH_UI_RANGE_ONE: the range the depth was written through, from the source the chain and every mesh
        // encode take it from (the cycle's verified trigger range, else the learned scene range). Not the learned
        // range alone: a cycle whose two differ would decode one depth two ways.
        kh_fill_depth_range_cb(cbd);

        kh_fill_local_band_cb(cbd, o);
        return cbd;
    };
    // KH_UI_BRIGHT_ID: the pass held back (kh_ui_bright_id) and its constants, or none.
    const RenderObject* khub_held = nullptr;
    ConstantData        khub_cbd = {};

    for (const auto& f : passes) {
        const RenderObject& o = f.second;
        if (o.effect <= 0) continue;
        // No-op skip (write-window twin; KH_FX_SKIP_DOC): an opacity of 0.001 or less is dropped - at 0 an exact
        // identity for the masked lane and the spill lane, up to 0.001 at most a thousandth of the pass - and skipping
        // before the capture saves the pass's copy too.
        if (o.color[3] <= 0.001f) continue;

        // Skip and count; the mask goes live one frame after the machinery spawns.
        if (!khum_mask_live) {  continue; }

        ID3D11PixelShader* khu_ufx = nullptr;
        bool khu_upyr = false;   // KH_FX_USER.

        if (o.effect == KH_EFFECT_CUSTOM) {
            khu_ufx = kh_user_fx_ps(dev, kh_fx_shader_of(o), &khu_upyr);
            if (!khu_ufx) continue;
        }

        // A LUT is not black-preserving (lifted blacks are the point of many grades), so it takes the masked builtin
        // lane (w = 2, in-shader coverage lerp) - never spill. Resolved here, bound at the pending flush: a later pass
        // in the deferral window must not steal t19 first.
        ID3D11ShaderResourceView* khu_lut = nullptr;

        if (o.effect == KH_EFFECT_LUT) {
            khu_lut = kh_user_lut_srv(dev, kh_fx_shader_of(o));
            if (!khu_lut) continue;
        }

        // The scene chain's one list (kh_fx_needs_depth; user shaders may sample depth). Its three scene-only readers
        // (ssgi, fogScatter, dynamicLightFog) are skipped below either way.
        const bool wants_depth = kh_fx_needs_depth(o);

        if (wants_depth && !depth_ok) continue;
        if ((o.localized || o.effect == static_cast<int>(EffectId::Pulse)) && !has_inverse) continue;
        if (o.effect == static_cast<int>(EffectId::Ssgi)) continue;
        if (o.effect == static_cast<int>(EffectId::Fogscatter)) continue;
        if (o.effect == static_cast<int>(EffectId::DynLightFog)) continue;   // KH_DLF: the scene's fog, not the UI's.

        const bool khu_spill =
            o.effect == static_cast<int>(EffectId::Bloom) ||
            o.effect == static_cast<int>(EffectId::Halation) ||
            o.effect == static_cast<int>(EffectId::LensFlare) ||
            o.effect == static_cast<int>(EffectId::Anamorphic) ||
            (o.effect == KH_EFFECT_CUSTOM && o.ui_spill);

        // KH_UI_BRIGHT_ID: a held pass is drawn only when this pass would fuse onto it - then exactly as at its own
        // turn (the pending pass drawn, the held one pending; this one fuses below) - and dropped otherwise: its
        // draw would have written the bits it read, and the pending pass, left pending, draws as the next turn or
        // the lane's end would have drawn the held one (its last write the RGB-only replace either way).
        // KH_UI_HELD_OPEN: a closed chain opens here, not at the held pass's turn - every pass between returned
        // before drawing, so the capture is the bits its turn would have copied, and a dropped pass copies nothing.
        if (khub_held) {
            if (!khu_ufx && kh_fuse_appendable(o)) {
                khuf_flush(false);
                if (!khup_chain && !khup_open()) {  break; }
                khuf_cbd = khub_cbd;
                khuf_lut = nullptr;
                khuf_obj = khub_held;
                khuf_live = true;
            }
            khub_held = nullptr;
        }
        if (!khu_ufx && kh_ui_bright_id(o, td.Width, td.Height)) {
            khub_cbd = khu_cbd_of(o, khu_spill);
            khub_held = &o;
            continue;
        }
        if (!khu_ufx && khuf_live && kh_fuse_appendable(o) &&
            kh_fuse_append(khuf_cbd, o, now_d)) continue;   // KH_FUSE_CLOCK: khu_cbd_of's clock.

        // Then the bracket begins: end stamps are issued inside the flush, so begin/end stay strictly alternating.
        if (khu_ufx) { khuf_flush(true); khup_chain = false; }
        else          khuf_flush(false);

        if (khu_ufx) {
            const std::string khu_cap_err = khu_spill
                ? kh_capture_bb_premult(dev, ctx, bb, td)
                : ensure_backbuffer_capture(dev, ctx, bb, td);
            if (!khu_cap_err.empty()) {   // KH_UI_USER_CAP: this pass alone is lost (the chain is closed above).
                report_error_once_safe("KH RenderIntegration: UI flush: a .hlsl UI pass could not capture the "
                                       "backbuffer (" + khu_cap_err + "); that pass is not drawn");
                continue;
            }
            ctx->OMSetRenderTargets(1, &khup_bb_rtv, khup_bb_dsv);
            ID3D11ShaderResourceView* srvs[2] = { g_res.bb_srv, depth_ok ? g_res.depth_srv : nullptr };
            ctx->PSSetShaderResources(0, 2, srvs);
        } else if (!khup_chain) {
            if (!khup_open()) {  break; }
        }

        ConstantData cbd = khu_cbd_of(o, khu_spill);
        if (khu_ufx) {
            KhGpuPassScope khgq_uuser(ctx, 1u, o.effect, true, 0u);   // KH_GPU_PASS: a UI-lane user effect.
            // KH_FX_USER: the pyramid of the pass's own source (t0, the capture) when its file reads it - the UI
            // route's, glow_pyr[1] - built before the pass's constants go up (kh_glow_build uploads its own).
            bool khu_gl = false;
            if (khu_upyr && khuf_frame_ok) {
                khu_gl = kh_glow_build(dev, ctx, g_res.glow_pyr[1], g_res.bb_srv, cbd.fx_meta[2], cbd.fx_meta[3],
                                       cbd.center_size[3], o.effect, 0.0f, g_res.constant_buffer);
                kh_glow_lanes(cbd, khu_gl ? &g_res.glow_pyr[1] : nullptr);
            }
            ID3D11ShaderResourceView* khu_tex[KH_MAT_USER_MAPS] = {};   // KH_FX_TEX.
            kh_fx_tex_fill(dev, ctx, cbd, o, khu_tex);
            // Object slice per pass; the frame slice went up once at the flush head (the latch gates this draw too).
            if (!khuf_frame_ok ||
                !kh_upload_obj_cb(ctx, g_res.constant_buffer, cbd)) continue;
            ctx->PSSetShader(khu_ufx, nullptr, 0);
            ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_FX_USER: s1 for every user effect draw.
            ctx->OMSetBlendState(khu_spill ? g_res.blend_ui_spill
                                           : g_res.blend_ui_masked,
                                 bf, 0xFFFFFFFF);
            KhGlowBind khu_gb;   // KH_FX_USER: t3 / s2 for this draw.
            if (khu_gl) khu_gb.bind(ctx, g_res.glow_pyr[1]);
            kh_fx_tex_bind(ctx, khu_tex);   // KH_FX_TEX.
            ctx->Draw(3, 0);
            kh_fx_tex_unbind(ctx);
            khu_gb.unbind();

        } else {
            khuf_cbd = cbd;
            khuf_lut = khu_lut;
            khuf_obj = &o;   // KH_FX_TEX_ALL.
            khuf_live = true;
        }
    }

    khuf_flush(true);

    if (khup_bb_rtv) khup_bb_rtv->Release();
    if (khup_bb_dsv) khup_bb_dsv->Release();
    khup_om.restore(ctx);
    khup_om.release();
    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);
    backup.restore(ctx);
    bb->Release();
}
inline bool flush_ui_frame() {
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // KH_RESET_THREAD.
    bool has_work = false;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (const auto& kv : g_draw_list) {
            if (kv.second.visible && kv.second.fullscreen && kv.second.affect_ui) { has_work = true; break; }
        }
    }

    if (!has_work) return false;
    if (!RVExtBridge::is_initialized()) return false;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return false;
    kh_stat(g_stats.ui_flushes);

    // KH_PRESENT_UI: the Present hook draws the UI chain this frame; the EH only keeps the driver alive. Off that path
    // (no hook, or Present has not drawn within the stale window) the lock path below stands.
    if (kh_present_ui_live()) return true;
    // KH_PARK_WAIT: the UI park whole (its wait + flush_ui_locked). KH_EA_UI_PARK_ZONE: below the Present path's
    // return, so the zone's entries count the lock path's calls alone.
    KH_PROF_SCOPE(KHP_FLUSH_UI_FRAME);

    // The lock path: the fallback while the Present hook is not serving (not installed, or no Present drew the chain
    // within the stale window). Its grant lands ahead of the engine's UI composition without the Draw3D park in front
    // of it (KH_UI_DIAG, rendering/ui_mask_state.hpp). Never without the lock: see KH_UI_DIAG.
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int64_t khfu_wait_t0 = kh_prof_now();   // KH_PARK_WAIT.
        RVExtBridge::ScopedGraphicsLock lock;
        kh_prof_add(KHP_FLUSH_UI_LOCK_WAIT, khfu_wait_t0);

        if (!lock.acquired()) {
            kh_stat(g_stats.lock_retries);
            continue;
        }
        ++g_ui_parks;   // KH_PARK_COUNT.
        // KhFlushActiveScope keeps our draws out of the hooks (released on unwind). KH_PARK_INJ: and the render-thread
        // branch on the thread the hooks take for the render thread - both as in kh_park_run.
        {
            KhFlushActiveScope khfu_active;
            KhInjectionFlagScope khfu_inj;
            if (reorder_on_render_thread()) g_ro.in_injection = true;
            flush_ui_locked(dev, ctx, false);   // KH_UI_LEARN_OWN.
        }
        return true;
    }

    kh_stat(g_stats.lock_failed_frames);
    return false;
}

// Internal UI-flush driver: a mission "Draw2D" event handler calling flushUIRender. Display 46 does not exist at
// pre_init, so an EachFrame poller waits for it, runs the compiled init once (which adds the handler if
// missionNamespace does not already hold its id), then idles. The init is idempotent, so the re-hoist below costs a
// namespace test. Under KH_PRESENT_UI the driver does not draw - the Present hook does (flush_ui_frame returns at
// kh_present_ui_live) - so the callsite only has to exist and be cheap.

// KH_EXIT_STATICS: never destructed, as g_draw3d_eh (its note).
static intercept::client::EHIdentifierHandle& g_ui_poll_eh = *(new intercept::client::EHIdentifierHandle());
static bool g_ui_driver_registered = false;
static bool g_ui_ctrl_created = false;

inline void ensure_ui_driver() {
    if (!kh_render_on()) return;   // KH_PLAYER_ONLY.
    if (g_ui_driver_registered) return;
    g_ui_driver_registered = true;
    g_ui_ctrl_created = false;

    g_ui_poll_eh = intercept::client::addMissionEventHandler<
        intercept::client::eventhandlers_mission::EachFrame>([]() {
        if (g_ui_ctrl_created) return;

        try {
            game_value result = raw_call_sqf_native(g_compiled_kh_ui_render_init);

            if (result.type_enum() == game_data_type::BOOL && static_cast<bool>(result)) {
                g_ui_ctrl_created = true;
            }
        } catch (...) {
            // Display transitioning or call context unavailable - retry next frame.
        }
    });
}

static float g_ui_rehoist_last_s = -1.0e9f;

inline void kh_ui_driver_rehoist() {
    ensure_ui_driver();
    const float khrh_now = effect_time_seconds();
    if (khrh_now - g_ui_rehoist_last_s < 0.5f) return;
    g_ui_rehoist_last_s = khrh_now;
    g_ui_ctrl_created = false;   // The EachFrame poll re-runs the (idempotent) init next frame.
}

// C++ Draw3D event handler lifecycle. Mission EHs die with the mission: Intercept empties its C++ mission-EH callbacks
// at each mission start (before preInit) and at the mission display's unload (mission_ended), and main.cpp calls
// rendering_integration_reset at mission_ended and at pre_init outside the main menu, which clears g_draw3d_eh_active.
// A savegame load and leaving the editor for the main menu reach neither reset (accepted: a non-issue).
// ensure_draw_eh also lazily self-registers as a fallback, so retained objects added mid-mission always render.
// KH_EXIT_STATICS: the handle is never destructed. A quit that skips mission_ended (Alt+F4, the window closed from
// Windows) leaves it armed - any object of ours arms it, a post effect alone - and an Intercept handle's destructor
// removes its mission event handler through an SQF command unless intercept::types::exiting is set, which the Intercept
// host does only at its own detach, after ours (it loaded us). Run at this DLL's static teardown, that command would
// reach an engine already shut down - the likely cause of the crash at Alt+F4, if that quit skips mission_ended (not
// confirmed in game). g_crew's per-character handles are kept the same way.

static intercept::client::EHIdentifierHandle& g_draw3d_eh = *(new intercept::client::EHIdentifierHandle());
static bool g_draw3d_eh_active = false;

inline void ensure_draw_eh() {
    if (!kh_render_on()) return;   // KH_PLAYER_ONLY.
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // KH_RESET_THREAD: SQF, game thread.
    if (g_draw3d_eh_active) return;

    g_draw3d_eh = intercept::client::addMissionEventHandler<
        intercept::client::eventhandlers_mission::Draw3D>([]() {
        try {
            flush_frame();
        } catch (const std::exception& e) {
            // Once per text (the handler runs every frame; a throw that recurs would otherwise raise an
            // SQF exception per frame) - same thread, so the report lands at the next on_frame.
            report_error_once_safe(std::string("KH RenderIntegration: Draw3D flush: ") + e.what());
        } catch (...) {
            report_error_once_safe("KH RenderIntegration: Draw3D flush: unknown exception");
        }
    });

    g_draw3d_eh_active = true;
}

}