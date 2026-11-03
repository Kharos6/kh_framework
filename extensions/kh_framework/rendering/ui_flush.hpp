#pragma once

// The injection half (rendering_integration_injection.hpp): the UI flush, the UI driver and the Draw3D event handler.
// Never include it on its own.
namespace RenderIntegration {

// UI_BB_UNWIND: UnwindRelease (the guard flush_ui_locked holds the backbuffer in) is declared with the PIP
// chain (rendering/pip.hpp), its other holder (pip_run_fx's PIP colour texture), which comes first in the unit.

// SESSION_RESET: hoisted out of the function below so the session reset reaches it.
static uint64_t g_ui_prev_scene_frame = 0;
static bool     g_ui_prev_from_present = false;   // UI_ORDINAL_SRC: the clock g_ui_prev_scene_frame came from.
// UI_LEARN_OWN: present - the caller bound a view of the swap chain's buffer (present_cb); the lock path
// (false) draws into whatever the engine had bound at the grant, which need not be a backbuffer (UI_DIAG). Only
// the former is learned as one, except while no Present hook is installed - the lock path is then the only learner.
inline void flush_ui_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool present) {
    KH_PROF_SCOPE(PROF_FLUSH_UI);
    KH_GPU_SCOPE(ctx, GPU_FLUSH_UI);   // PROF.
    const float snapshot_now = effect_time_seconds();
    static std::vector<std::pair<uint64_t, RenderObject>> passes;   // Key = creation seq.
    passes.clear();

    LARGE_INTEGER t0 = {}, t1 = {};
    QueryPerformanceCounter(&t0);

    {
        scene_sync();   // SCENE.

        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& u_live = g_scene.objs[i];
            if (!(u_live.visible && u_live.fullscreen && u_live.affect_ui)) continue;
            bool expired = false;
            const float env = lifetime_envelope(u_live, snapshot_now, expired);
            if (expired) continue;   // The game thread erases it (flush_frame's census: NO_PARK_EXPIRY).
            RenderObject o = u_live;
            o.color[3] *= env;
            passes.emplace_back(o.seq, o);
        }
    }

    QueryPerformanceCounter(&t1);

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
    void* bb_id = static_cast<void*>(res);
    ID3D11Texture2D* bb = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb));
    res->Release();
    if (FAILED(hr) || !bb) {  return; }
    // UI_BB_UNWIND: a throw below (an allocation in an ensure, a load, a pass) reaches the Present callback's
    // or the command's catch, which cannot see bb - and a reference stranded on a swapchain buffer fails the
    // engine's next ResizeBuffers (PRESENT_RESIZE). Released on that unwind; every return below releases it.
    UnwindRelease<ID3D11Texture2D> ub_bb(bb);
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
        // UI_BB_REPORT: said only of the swap chain's buffer - the lock path's target may be any engine target.
        if (present) {   // EA_ERR_CLASS: the swap chain's configuration - a warning (logged, never thrown).
            log_once_safe("KH RenderIntegration: UI flush: UI passes need an 8-bit (RGBA8 / BGRA8) single-sample "
                          "backbuffer; none is drawn on this one");
        }
        bb->Release();
        return;
    }

    if (!ensure_ok("UI-flush resources", ensure_resources(dev))) { bb->Release(); return; }
    if (!ensure_ok("UI-flush effect shader", ensure_effect_shader(dev))) { bb->Release(); return; }

    // The mask is live when the machine's clear fired for this very buffer this frame; UI-mode passes skip otherwise
    // (one-frame learn latency at spawn - the designed fail-safe).
    if (present || !g_present.hook_active.load(std::memory_order_relaxed)) {   // UI_LEARN_OWN.
        ui_mask_learn(bb_id, td.Width, td.Height);
    }
    {
        bool from_present = false;
        const uint64_t scene_frame = ui_frame_ordinal(&from_present);   // PRESENT_SEQ: the
                                                                                     // dark-driver test's clock.
        // UI_ORDINAL_SRC: the previous reading counts only on the same clock - the Present count and the flush
        // ordinal count different events at different rates, so across a switch between them (a stale spell of the
        // Present path: a hitch, an unfocused stretch, the frames after alt-tab) one could pass the other by more than
        // one, read as a dark driver and drop that frame's UI effect. Zero is "no previous", as at a reset.
        if (from_present != g_ui_prev_from_present) g_ui_prev_scene_frame = 0;
        g_ui_prev_from_present = from_present;
        const bool driver_dark =
            g_ui_prev_scene_frame != 0 &&
            scene_frame > g_ui_prev_scene_frame + 1;
        g_ui_prev_scene_frame = scene_frame;

        if (driver_dark && g_ui_mask.compose_serial != 0 &&
            g_ui_mask.compose_serial != g_ui_mask.applied_serial) {
            g_ui_mask.mask_valid = false;   // Suspect coverage: skip, resync.
        }
    }

    const bool mask_live = g_ui_mask.mask_valid &&
                           g_ui_mask.phase_bb == bb_id &&
                           g_ui_poison_run < UI_POISON_DEAD;   // UI_POISON_RUN: a dead mask draws nothing.

    // UI_POISON_RUN's readback. A same-frame CPU verdict is unobtainable here (the copy is read at a later flush,
    // without waiting); the same-frame verdict is PSEffect's guard. This lagged one stands every pass down at the
    // flushes after the UI_POISON_DEAD-th full reading in a row, until a reading is not full.
    if (td.Width >= 512 && td.Height >= 512) {
        // UI_STAGE_FMT: the copies need a stage of the backbuffer's format, and the capture group that also
        // releases it (release_bb_capture) is remade only when a pass captures - a format change is caught here.
        if (g_res.ui_alpha_stage) {
            D3D11_TEXTURE2D_DESC sd = {};
            g_res.ui_alpha_stage->GetDesc(&sd);
            if (sd.Format != td.Format) KH_SAFE_RELEASE(g_res.ui_alpha_stage);
        }
        if (!g_res.ui_alpha_stage) {
            D3D11_TEXTURE2D_DESC as_td = {};
            as_td.Width = 64;
            as_td.Height = 64;
            as_td.MipLevels = 1;
            as_td.ArraySize = 1;
            as_td.Format = td.Format;
            as_td.SampleDesc.Count = 1;
            as_td.Usage = D3D11_USAGE_STAGING;
            as_td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            dev->CreateTexture2D(&as_td, nullptr, &g_res.ui_alpha_stage);
            g_ui_mask.alpha_copy_pending = false;
        }

        if (g_res.ui_alpha_stage) {
            if (g_ui_mask.alpha_copy_pending) {
                D3D11_MAPPED_SUBRESOURCE map = {};
                const HRESULT as_hr = ctx->Map(g_res.ui_alpha_stage, 0, D3D11_MAP_READ,
                                               D3D11_MAP_FLAG_DO_NOT_WAIT, &map);

                if (SUCCEEDED(as_hr)) {
                    int as_min = 255;   // The poison rule reads the minimum alone.

                    for (UINT y = 0; y < 64; ++y) {
                        const uint8_t* row =
                            static_cast<const uint8_t*>(map.pData) + y * map.RowPitch;

                        for (UINT x = 0; x < 64; ++x) {
                            const int as_a = row[x * 4 + 3];   // A is byte 3 in RGBA8 and
                                                                           // BGRA8.
                            if (as_a < as_min) as_min = as_a;
                        }
                    }

                    ctx->Unmap(g_res.ui_alpha_stage, 0);
                    g_ui_poison_run = (as_min == 255) ? g_ui_poison_run + 1 : 0;   // UI_POISON_RUN.
                    g_ui_mask.alpha_copy_pending = false;
                } else if (as_hr != DXGI_ERROR_WAS_STILL_DRAWING) {
                    g_ui_mask.alpha_copy_pending = false;
                }
            }

            if (!g_ui_mask.alpha_copy_pending) {
                static const float fx[8] = { 0.18f, 0.47f, 0.79f, 0.23f,
                                             0.68f, 0.31f, 0.58f, 0.86f };
                static const float fy[8] = { 0.21f, 0.16f, 0.24f, 0.52f,
                                             0.47f, 0.77f, 0.84f, 0.69f };
                PredAside pa;   // UI_STAGE_PRED: the copies run ahead of the flush's StateBackup.
                pa.begin(ctx);

                for (UINT s = 0; s < 8; ++s) {
                    D3D11_BOX box = {};
                    box.left = static_cast<UINT>(td.Width * fx[s]);
                    box.right = box.left + 64;
                    box.top = static_cast<UINT>(td.Height * fy[s]);
                    box.bottom = box.top + 8;
                    box.front = 0;
                    box.back = 1;
                    ctx->CopySubresourceRegion(g_res.ui_alpha_stage, 0,
                                               0, s * 8, 0, bb, 0, &box);
                }
                pa.end();

                g_ui_mask.alpha_copy_pending = true;
            }
        }
    }
    const uint64_t serial = g_ui_mask.compose_serial;

    if (serial != 0 && serial == g_ui_mask.applied_serial) {
        bb->Release();
        return;
    }

    // UI_CYCLE_CAM: the camera that rendered the frame this lane draws over - the cycle's boundary camera, as
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
    // FX_FRAME_ADOPT (UI_FX_PARITY): under the engine's frame rotation when fresh, as the scene chain takes
    // its camera (flush_locked adopts whichever source it took) - its meshes, its frame and its user lanes alike.
    adopt_frame_view(pv);
    // UI_SERIAL_LATE: applied once the camera is in hand - marked before it, a failed read skipped this
    // compose's passes instead of retrying them at the next Present.
    g_ui_mask.applied_serial = serial;
    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    float inv_view_proj[4][4] = {};
    bool need_inverse = false;

    for (const auto& f : passes) {
        if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse) ||
            f.second.effect == EFFECT_CUSTOM) { need_inverse = true; break; }
    }

    // UI_FX_PARITY: the scene chain's depth recipe (flush_locked: FX_ENGINE_PAIR, FX_CAM_REL) over this lane's
    // camera (UI_CYCLE_CAM, above). The depth is decoded with the pair the engine encoded it with when that was read
    // (the camera's own pair otherwise), and re-encoded through the same pair in the inverse; the inverse is
    // of the rotation-only view, the camera carried in fxCam - an absolute inverse cancels the camera's kilometres per
    // pixel in fp32, which shimmers a localized mask's edge.
    float m22 = pv.projection[2][2], m32 = pv.projection[3][2];
    if (g_ro.engine_proj_valid) {
        m22 = g_ro.engine_m22;
        m32 = g_ro.engine_m32;
    }
    {
        // FX_PAIR_HOLD: through the hold guard, as the scene chain takes it (fx_depth_pair_guard's note).
        // Where this cycle's scene flush decided (the memo's serial and cycle) its verdict is replayed, so this
        // lane reads with the chain's pair (FX_PAIR_KEY: an engine pair that latched after a fallback flush is
        // judged afresh unless that verdict held). FX_PAIR_ONE: on the fallback route this lane takes that
        // verdict's pair (g_fx_pair_guard.pair). None of this cycle's (no flush ran, or it left before its frame): a
        // verdict of this Present's own under a serial no flush can name, so no stale one is replayed.
        const uint64_t fs = g_cc_flush_serial.load(std::memory_order_relaxed);
        const bool same = fs == g_fx_pair_guard.verdict_serial &&
                          g_fx_pair_guard.verdict_cycle == g_topo_cycles_pub.load(std::memory_order_relaxed);
        if (same && !g_ro.engine_proj_valid) {
            m22 = g_fx_pair_guard.pair[0];
            m32 = g_fx_pair_guard.pair[1];
        } else {
            ConstantData pg = {};
            pg.depth_params[0] = m22;
            pg.depth_params[1] = m32;
            fill_depth_range_cb(pg);
            fx_depth_pair_guard(pg, same ? fs : ((1ull << 63) | ++g_fx_pair_guard.ui_serial));
            m22 = pg.depth_params[0];
            m32 = pg.depth_params[1];
        }
    }
    float fx_cam[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cam_of(pv.view, fx_cam);   // CAM_HONEST.
    float dvp[4][4];
    {
        float dproj[4][4], dview[4][4];
        memcpy(dproj, pv.projection, sizeof(dproj));
        dproj[2][2] = m22;
        dproj[3][2] = m32;
        memcpy(dview, pv.view, sizeof(dview));
        dview[3][0] = dview[3][1] = dview[3][2] = 0.0f;
        dview[3][3] = 1.0f;
        mul_4x4(dview, dproj, dvp);
    }

    const bool has_inverse = need_inverse && inverse_4x4(dvp, inv_view_proj);
    const double now_d = effect_time_seconds_d();
    const bool depth_ok = g_res.depth_srv != nullptr;
    StateBackup backup;
    backup.capture(ctx);
    // FX_TEX: finished background loads land here too - a scene of UI-lane passes alone never reaches the scene
    // flush's pump (its list keeps UI-only passes out), and a user pass's textures would stay pending. A placement
    // bumps the page generation; every material draw re-fixes its base against it (mat_gpu_ensure).
    tex_async_pump(ctx, dev);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Fail-dark: a failed frame upload stands every draw down through the frame_ok latch - never a draw against a
    // stale b1. b0 = per-object, b1 = per-frame.
    ID3D11Buffer* cbs[2] = { g_res.constant_buffer, g_res.frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    bool frame_ok = false;

    {
        ConstantData cbf = {};
        memcpy(cbf.view_proj, view_proj, sizeof(view_proj));
        memcpy(cbf.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        if (has_inverse) memcpy(cbf.fx_cam, fx_cam, sizeof(cbf.fx_cam));   // UI_FX_PARITY.
        {   // USER_LANES.
            float cam[3];
            cam_of(pv.view, cam);
            fill_user_cam_cb(cbf, cam, pv.view);
        }
        frame_ok = upload_frame_cb(ctx, g_res.frame_cb, cbf);
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
    bool chain = false;   // A chain is open; src holds the live source.
    int  src = 0;   // 0 = bb_tex, 1 = ui_chain_tex.
    ID3D11ShaderResourceView* up_srvs[2] = { nullptr, nullptr };
    ID3D11RenderTargetView*   rtvs[2] = { nullptr, nullptr };
    // The backbuffer RTV is bound at entry (the gate verified it); hold a reference for the final pass + custom
    // passes.
    ID3D11RenderTargetView* bb_rtv = nullptr;
    ID3D11DepthStencilView* bb_dsv = nullptr;   // nullptr by the gate; carried for symmetry.
    ctx->OMGetRenderTargets(1, &bb_rtv, &bb_dsv);
    // UI_BB_UNWIND: the view holds the buffer too. Ahead of om, so an unwind restores the OM first.
    UnwindRelease<ID3D11RenderTargetView> ub_rtv(bb_rtv);
    UnwindRelease<ID3D11DepthStencilView> ub_dsv(bb_dsv);
    // UI_VIEW_FMT: the lane reads the buffer in the texture's own format (its captures, chain and views), so a
    // bound view of another format - an _SRGB view on a UNORM flip-model buffer - would encode its result a second
    // time on the final write. Such a view is replaced, for this flush, by one of the texture's format (a TYPELESS
    // texture keeps the engine's: its captures cannot be read anyway). Released below, as the engine's would be. Only
    // on a live mask: every pass skips a dead one, so its view is never bound.
    if (mask_live && bb_rtv && td.Format != DXGI_FORMAT_R8G8B8A8_TYPELESS &&
        td.Format != DXGI_FORMAT_B8G8R8A8_TYPELESS) {
        D3D11_RENDER_TARGET_VIEW_DESC d = {};
        bb_rtv->GetDesc(&d);
        ID3D11RenderTargetView* own = nullptr;
        if (d.Format != td.Format && SUCCEEDED(dev->CreateRenderTargetView(bb, nullptr, &own)) &&
            own) {
            bb_rtv->Release();
            bb_rtv = own;
        }
    }
    // The gate verified slot 0 and the DSV only; the single-slot rebinds below null slots 1-7, so the full set is
    // captured here and restored at the tail.
    OmSave om;
    om.capture(ctx);

    bool                      uf_live = false;
    ConstantData              uf_cbd;
    ID3D11ShaderResourceView* uf_lut = nullptr;
    const RenderObject*       obj = nullptr;   // FX_TEX_ALL: the deferred pass's object (in passes).

    auto flush = [&](bool uf_final) {
        if (!uf_live) return;
        uf_live = false;
        GpuPassScope upass(ctx, 1u, static_cast<int>(uf_cbd.fx_meta[0] + 0.5f), false,   // GPU_PASS.
                           static_cast<uint32_t>(uf_cbd.fuse_meta[0] + 0.5f));
        // FX_TEX_ALL: a builtin pass's own textures - the lanes before the probe side draw takes uf_cbd, the
        // pages bound to the flush's end on every exit (the scene chain's rule).
        ID3D11ShaderResourceView* tex[MAT_USER_MAPS] = {};
        FxTexScope ts;
        if (obj && fx_tex_on(*obj)) {
            fx_tex_fill(dev, ctx, uf_cbd, *obj, tex);
            ts.bind(ctx, tex);
        }
        // FX_SIDE: the lane's coverage probe (side id 30, 1 x 1) of this pass's own source, drawn before the
        // pass's constants go up; they then arm matCtl.z, and t5 carries it.
        bool probe = false;
        if (frame_ok && uf_cbd.center_size[3] > 1.5f &&
            fx_side_draw(dev, ctx, 2, uf_cbd, 30.0f, 1u, 1u, up_srvs[src], g_res.constant_buffer)) {
            probe = true;
            uf_cbd.mat_ctl[2] = 1.0f;
        }
        // GLOW_PYR: the scene chain's rule, on this pass's own source (the spill lane's premultiply rides its w).
        bool on = false;
        const int e = static_cast<int>(uf_cbd.fx_meta[0] + 0.5f);
        if (frame_ok && glow_effect(e)) {
            on = glow_build(dev, ctx, g_res.glow_pyr[1], up_srvs[src], uf_cbd.fx_meta[2],
                            uf_cbd.fx_meta[3], uf_cbd.center_size[3], e, uf_cbd.fx0[0],
                            g_res.constant_buffer);
            glow_lanes(uf_cbd, on ? &g_res.glow_pyr[1] : nullptr);
        }
        // ANA_PYR: anamorphic's one-axis pyramid, the same rule (the spill lane's premultiply rides its w too).
        const AnaPyr* pyr = nullptr;
        if (frame_ok && e == static_cast<int>(EffectId::Anamorphic)) {
            const int ax = uf_cbd.fx1[0] > 0.5f ? 1 : 0;
            if (ana_build(dev, ctx, g_res.ana_pyr[1][ax], up_srvs[src], uf_cbd.fx_meta[2],
                          uf_cbd.fx_meta[3], uf_cbd.center_size[3], ax, uf_cbd.fx0[0],
                          g_res.constant_buffer))
                pyr = &g_res.ana_pyr[1][ax];
            ana_lanes(uf_cbd, pyr);
        }
        // Unbind the source slot before the destination's RTV turn.
        ID3D11ShaderResourceView* null = nullptr;
        ctx->PSSetShaderResources(0, 1, &null);
        if (uf_final) ctx->OMSetRenderTargets(1, &bb_rtv, bb_dsv);
        else            ctx->OMSetRenderTargets(1, &rtvs[src ^ 1], nullptr);
        ID3D11ShaderResourceView* uf_srvs[2] = { up_srvs[src],
                                                 depth_ok ? g_res.depth_srv : nullptr };
        ctx->PSSetShaderResources(0, 2, uf_srvs);
        if (uf_lut) ctx->PSSetShaderResources(19, 1, &uf_lut);

        if (!frame_ok ||
            !upload_obj_cb(ctx, g_res.constant_buffer, uf_cbd)) return;

        // FX_LEAN_PASS (the note at Resources::ps_effect_lean): the UI lane's passes are never armed.
        ctx->PSSetShader((g_res.ps_effect_lean && !(uf_cbd.fx_tex[1][2] > 0.5f)) ? g_res.ps_effect_lean
                                                                                   : g_res.ps_effect, nullptr, 0);
        // RGB-only replace blend on the final write, so the mask channel itself survives the view.
        ctx->OMSetBlendState(uf_final ? g_res.blend_ui_replace : nullptr,
                             bf, 0xFFFFFFFF);
        GlowBind ug_b;   // GLOW_PYR.
        if (on) ug_b.bind(ctx, g_res.glow_pyr[1]);
        AnaBind ua_b;   // ANA_PYR.
        if (pyr) ua_b.bind(ctx, *pyr);
        if (probe) ctx->PSSetShaderResources(5, 1, &g_res.fxs_srv[2]);   // FX_SIDE.
        ctx->Draw(3, 0);
        ua_b.unbind();
        ug_b.unbind();
        if (probe) {   // Unbound at once: the next pass's probe renders into it.
            ID3D11ShaderResourceView* pn = nullptr;
            ctx->PSSetShaderResources(5, 1, &pn);
        }
        if (!uf_final) src ^= 1;

    };


    // The single capture: the backbuffer as the engine's UI left it (rgb + the live coverage in alpha: UI_COV_LIVE),
    // into bb_tex = chain 0. False: refused (the loop ends).
    auto open = [&]() -> bool {
        std::string cap = ensure_backbuffer_capture(dev, ctx, bb, td);
        if (cap.empty()) cap = ensure_ui_chain(dev, td);
        // EA_UI_CAP_REPORT: reported once per text, as the user pass's capture (UI_USER_CAP) and the ensures
        // above are - a failure here ends the flush's pass loop (both callers break).
        if (!ensure_ok("UI-flush capture", cap)) return false;
        up_srvs[0] = g_res.bb_srv;
        rtvs[0] = g_res.bb_rtv;
        up_srvs[1] = g_res.ui_chain_srv;
        rtvs[1] = g_res.ui_chain_rtv;
        src = 0;
        chain = true;
        return true;
    };
    // A pass's constants (UI_BRIGHT_ID builds a held pass's at its own turn, with this).
    auto cbd_of = [&](const RenderObject& o, bool spill) {
        ConstantData cbd = {};
        memcpy(cbd.view_proj, view_proj, sizeof(view_proj));
        memcpy(cbd.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];
        cbd.center_size[2] = o.pos[1];
        cbd.center_size[3] =
            o.effect == EFFECT_CUSTOM ? 1.25f : (spill ? 3.0f : 2.0f);
        cbd.size_axes[3] = spill ? 0.0f : static_cast<float>(o.blend_mode);
        fill_fx_params_cb(cbd, o);

        cbd.fx_meta[0] = static_cast<float>(o.effect);
        cbd.fx_meta[1] = fx_time(o, now_d);
        cbd.fx_meta[2] = static_cast<float>(td.Width);
        cbd.fx_meta[3] = static_cast<float>(td.Height);
        cbd.depth_params[0] = m22;   // UI_FX_PARITY: the encode pair.
        cbd.depth_params[1] = m32;
        // UI_RANGE_ONE: the range the depth was written through, from the source the chain and every mesh
        // encode take it from (the cycle's verified trigger range, else the learned scene range). Not the learned
        // range alone: a cycle whose two differ would decode one depth two ways.
        fill_depth_range_cb(cbd);

        fill_local_band_cb(cbd, o);
        return cbd;
    };
    // UI_BRIGHT_ID: the pass held back (ui_bright_id) and its constants, or none.
    const RenderObject* held = nullptr;
    ConstantData        ub_cbd = {};

    for (const auto& f : passes) {
        const RenderObject& o = f.second;
        if (o.effect <= 0) continue;
        // No-op skip (write-window twin; FX_SKIP_DOC): an opacity of 0.001 or less is dropped - at 0 an exact
        // identity for the masked lane and the spill lane, up to 0.001 at most a thousandth of the pass - and skipping
        // before the capture saves the pass's copy too.
        if (o.color[3] <= 0.001f) continue;

        // Skip and count; the mask goes live one frame after the machinery spawns.
        if (!mask_live) {  continue; }

        ID3D11PixelShader* ufx = nullptr;
        bool upyr = false;   // FX_USER.

        if (o.effect == EFFECT_CUSTOM) {
            ufx = user_fx_ps(dev, fx_shader_of(o), &upyr);
            if (!ufx) continue;
        }

        // A LUT is not black-preserving (lifted blacks are the point of many grades), so it takes the masked builtin
        // lane (w = 2, in-shader coverage lerp) - never spill. Resolved here, bound at the pending flush: a later pass
        // in the deferral window must not steal t19 first.
        ID3D11ShaderResourceView* u_lut = nullptr;

        if (o.effect == EFFECT_LUT) {
            u_lut = user_lut_srv(dev, fx_shader_of(o));
            if (!u_lut) continue;
        }

        // The scene chain's one list (fx_needs_depth; user shaders may sample depth). Its three scene-only readers
        // (ssgi, fogScatter, dynamicLightFog) are skipped below either way.
        const bool wants_depth = fx_needs_depth(o);

        if (wants_depth && !depth_ok) continue;
        if ((o.localized || o.effect == static_cast<int>(EffectId::Pulse)) && !has_inverse) continue;
        if (o.effect == static_cast<int>(EffectId::Ssgi)) continue;
        if (o.effect == static_cast<int>(EffectId::Fogscatter)) continue;
        if (o.effect == static_cast<int>(EffectId::DynLightFog)) continue;   // DLF: the scene's fog, not the UI's.

        const bool spill =
            o.effect == static_cast<int>(EffectId::Bloom) ||
            o.effect == static_cast<int>(EffectId::Halation) ||
            o.effect == static_cast<int>(EffectId::LensFlare) ||
            o.effect == static_cast<int>(EffectId::Anamorphic) ||
            (o.effect == EFFECT_CUSTOM && o.ui_spill);

        // UI_BRIGHT_ID: a held pass is drawn only when this pass would fuse onto it - then exactly as at its own
        // turn (the pending pass drawn, the held one pending; this one fuses below) - and dropped otherwise: its
        // draw would have written the bits it read, and the pending pass, left pending, draws as the next turn or
        // the lane's end would have drawn the held one (its last write the RGB-only replace either way).
        // UI_HELD_OPEN: a closed chain opens here, not at the held pass's turn - every pass between returned
        // before drawing, so the capture is the bits its turn would have copied, and a dropped pass copies nothing.
        if (held) {
            if (!ufx && fuse_appendable(o)) {
                flush(false);
                if (!chain && !open()) {  break; }
                uf_cbd = ub_cbd;
                uf_lut = nullptr;
                obj = held;
                uf_live = true;
            }
            held = nullptr;
        }
        if (!ufx && ui_bright_id(o, td.Width, td.Height)) {
            ub_cbd = cbd_of(o, spill);
            held = &o;
            continue;
        }
        if (!ufx && uf_live && fuse_appendable(o) &&
            fuse_append(uf_cbd, o, now_d)) continue;   // FUSE_CLOCK: cbd_of's clock.

        // Then the bracket begins: end stamps are issued inside the flush, so begin/end stay strictly alternating.
        if (ufx) { flush(true); chain = false; }
        else          flush(false);

        if (ufx) {
            const std::string cap_err = spill
                ? capture_bb_premult(dev, ctx, bb, td)
                : ensure_backbuffer_capture(dev, ctx, bb, td);
            if (!cap_err.empty()) {   // UI_USER_CAP: this pass alone is lost (the chain is closed above).
                report_error_once_safe("KH RenderIntegration: UI flush: a .hlsl UI pass could not capture the "
                                       "backbuffer (" + cap_err + "); that pass is not drawn");
                continue;
            }
            ctx->OMSetRenderTargets(1, &bb_rtv, bb_dsv);
            ID3D11ShaderResourceView* srvs[2] = { g_res.bb_srv, depth_ok ? g_res.depth_srv : nullptr };
            ctx->PSSetShaderResources(0, 2, srvs);
        } else if (!chain) {
            if (!open()) {  break; }
        }

        ConstantData cbd = cbd_of(o, spill);
        if (ufx) {
            GpuPassScope uuser(ctx, 1u, o.effect, true, 0u);   // GPU_PASS: a UI-lane user effect.
            // FX_USER: the pyramid of the pass's own source (t0, the capture) when its file reads it - the UI
            // route's, glow_pyr[1] - built before the pass's constants go up (glow_build uploads its own).
            bool gl = false;
            if (upyr && frame_ok) {
                gl = glow_build(dev, ctx, g_res.glow_pyr[1], g_res.bb_srv, cbd.fx_meta[2], cbd.fx_meta[3],
                                cbd.center_size[3], o.effect, 0.0f, g_res.constant_buffer);
                glow_lanes(cbd, gl ? &g_res.glow_pyr[1] : nullptr);
            }
            ID3D11ShaderResourceView* tex[MAT_USER_MAPS] = {};   // FX_TEX.
            fx_tex_fill(dev, ctx, cbd, o, tex);
            // Object slice per pass; the frame slice went up once at the flush head (the latch gates this draw too).
            if (!frame_ok ||
                !upload_obj_cb(ctx, g_res.constant_buffer, cbd)) continue;
            ctx->PSSetShader(ufx, nullptr, 0);
            ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // FX_USER: s1 for every user effect draw.
            ctx->OMSetBlendState(spill ? g_res.blend_ui_spill
                                           : g_res.blend_ui_masked,
                                 bf, 0xFFFFFFFF);
            GlowBind gb;   // FX_USER: t3 / s2 for this draw.
            if (gl) gb.bind(ctx, g_res.glow_pyr[1]);
            fx_tex_bind(ctx, tex);   // FX_TEX.
            ctx->Draw(3, 0);
            fx_tex_unbind(ctx);
            gb.unbind();

        } else {
            uf_cbd = cbd;
            uf_lut = u_lut;
            obj = &o;   // FX_TEX_ALL.
            uf_live = true;
        }
    }

    flush(true);

    if (bb_rtv) bb_rtv->Release();
    if (bb_dsv) bb_dsv->Release();
    om.restore(ctx);
    om.release();
    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);
    backup.restore(ctx);
    bb->Release();
}
inline bool flush_ui_frame() {
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // RESET_THREAD.
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
    stat_inc(g_stats.ui_flushes);

    // PRESENT_UI: the Present hook draws the UI chain this frame; the EH only keeps the driver alive. Off that path
    // (no hook, or Present has not drawn within the stale window) the lock path below stands.
    if (present_ui_live()) return true;
    // PARK_WAIT: the UI park whole (its wait + flush_ui_locked). EA_UI_PARK_ZONE: below the Present path's
    // return, so the zone's entries count the lock path's calls alone.
    KH_PROF_SCOPE(PROF_FLUSH_UI_FRAME);

    // The lock path: the fallback while the Present hook is not serving (not installed, or no Present drew the chain
    // within the stale window). Its grant lands ahead of the engine's UI composition without the Draw3D park in front
    // of it (UI_DIAG, rendering/ui_mask_state.hpp). Never without the lock: see UI_DIAG.
    for (int attempt = 0; attempt < 3; ++attempt) {
        const int64_t wait_t0 = prof_now();   // PARK_WAIT.
        RVExtBridge::ScopedGraphicsLock lock;
        prof_add(PROF_FLUSH_UI_LOCK_WAIT, wait_t0);

        if (!lock.acquired()) {
            stat_inc(g_stats.lock_retries);
            continue;
        }
        ++g_ui_parks;   // PARK_COUNT.
        // FlushActiveScope keeps our draws out of the hooks (released on unwind). PARK_INJ: and the render-thread
        // branch on the thread the hooks take for the render thread - both as in park_run.
        {
            FlushActiveScope active;
            InjectionFlagScope inj;
            if (reorder_on_render_thread()) g_ro.in_injection = true;
            flush_ui_locked(dev, ctx, false);   // UI_LEARN_OWN.
        }
        return true;
    }

    stat_inc(g_stats.lock_failed_frames);
    return false;
}

// Internal UI-flush driver: a mission "Draw2D" event handler calling flushUIRender. Display 46 does not exist at
// pre_init, so an EachFrame poller waits for it, runs the compiled init once (which adds the handler if
// missionNamespace does not already hold its id), then idles. The init is idempotent, so the re-hoist below costs a
// namespace test. Under PRESENT_UI the driver does not draw - the Present hook does (flush_ui_frame returns at
// present_ui_live) - so the callsite only has to exist and be cheap.

// EXIT_STATICS: never destructed, as g_draw3d_eh (its note).
static intercept::client::EHIdentifierHandle& g_ui_poll_eh = *(new intercept::client::EHIdentifierHandle());
static bool g_ui_driver_registered = false;
static bool g_ui_ctrl_created = false;

inline void ensure_ui_driver() {
    if (!render_on()) return;   // PLAYER_ONLY.
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

inline void ui_driver_rehoist() {
    ensure_ui_driver();
    const float now = effect_time_seconds();
    if (now - g_ui_rehoist_last_s < 0.5f) return;
    g_ui_rehoist_last_s = now;
    g_ui_ctrl_created = false;   // The EachFrame poll re-runs the (idempotent) init next frame.
}

// C++ Draw3D event handler lifecycle. Mission EHs die with the mission: Intercept empties its C++ mission-EH callbacks
// at each mission start (before preInit) and at the mission display's unload (mission_ended), and main.cpp calls
// rendering_integration_reset at mission_ended and at pre_init outside the main menu, which clears g_draw3d_eh_active.
// A savegame load and leaving the editor for the main menu reach neither reset (accepted: a non-issue).
// ensure_draw_eh also lazily self-registers as a fallback, so retained objects added mid-mission always render.
// EXIT_STATICS: the handle is never destructed. A quit that skips mission_ended (Alt+F4, the window closed from
// Windows) leaves it armed - any object of ours arms it, a post effect alone - and an Intercept handle's destructor
// removes its mission event handler through an SQF command unless intercept::types::exiting is set, which the Intercept
// host does only at its own detach, after ours (it loaded us). Run at this DLL's static teardown, that command would
// reach an engine already shut down - the likely cause of the crash at Alt+F4, if that quit skips mission_ended (not
// confirmed in game). g_crew's per-character handles are kept the same way.

static intercept::client::EHIdentifierHandle& g_draw3d_eh = *(new intercept::client::EHIdentifierHandle());
static bool g_draw3d_eh_active = false;

inline void ensure_draw_eh() {
    if (!render_on()) return;   // PLAYER_ONLY.
    g_game_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);   // RESET_THREAD: SQF, game thread.
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