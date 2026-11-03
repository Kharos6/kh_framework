#pragma once

// The feature half (rendering_integration_features.hpp): the fullscreen post-processing chain (fx_chain_run), its
// side passes (FX_SIDE, FX_LINZ) and the dynamicLightFog split stencil. Never include it on its own.
namespace RenderIntegration {

// FX_SIDE - draw side value k (side id id: 28 fog scatter's per-pixel fog, 29 the sun flare's visibility,
// 30 the UI lane's coverage probe) with PSEffect (28: PSFsSide where made) into its own target, from the pass's own
// constants, before the pass: the glow build's contract - it uploads its own slice to cb (the route's object
// buffer, bound at b0), so the pass uploads its constants after it, and every other binding it makes is put back
// (GlowSave). The fog target is the frame's size and is drawn through the viewport the pass draws through; the
// two 1 x 1 targets through a 1 x 1 one. t0: the source bound at t0 for the draw (null keeps t0 as it is).
// False = no side value: the pass stays unarmed and computes the value in place.
// WSEE_EDGE_COST: st - the run's E stencil where E's draw marked it (else null): fog scatter's value (k 0) is
// then drawn split by it (wsee_split_draw) where its target is E's size; a size that differs draws once.
inline bool fx_side_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx, int k, ConstantData& cbd,
                         float id, UINT w, UINT h, ID3D11ShaderResourceView* t0,
                         ID3D11Buffer* cb, ID3D11DepthStencilView* st = nullptr) {
    if (!dev || !ctx || !cb || k < 0 || k > 2) return false;
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return false;
    if (!g_res.ps_effect || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!g_res.fxs_tex[k] || !g_res.fxs_rtv[k] || !g_res.fxs_srv[k] ||
        g_res.fxs_w[k] != w || g_res.fxs_h[k] != h) {
        KH_SAFE_RELEASE(g_res.fxs_srv[k]);
        KH_SAFE_RELEASE(g_res.fxs_rtv[k]);
        KH_SAFE_RELEASE(g_res.fxs_tex[k]);
        g_res.fxs_w[k] = g_res.fxs_h[k] = 0;
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        // The fog fraction (k 0, in [0, 1]) is stored at half precision: the armed pass reads it within 2^-12 of
        // the fp32 value the unarmed routes (effect meshes, the PIP) compute in place - far below an 8-bit step.
        // The two 1 x 1 values are exact (fp32).
        td.Format = k == 0 ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.fxs_tex[k])) ||
            FAILED(dev->CreateRenderTargetView(g_res.fxs_tex[k], nullptr, &g_res.fxs_rtv[k])) ||
            FAILED(dev->CreateShaderResourceView(g_res.fxs_tex[k], nullptr, &g_res.fxs_srv[k]))) {
            KH_SAFE_RELEASE(g_res.fxs_srv[k]);
            KH_SAFE_RELEASE(g_res.fxs_rtv[k]);
            KH_SAFE_RELEASE(g_res.fxs_tex[k]);
            report_error_once_safe("KH RenderIntegration: effects: side target could not be made (the pass computes "
                                   "it in place)");
            return false;
        }
        g_res.fxs_w[k] = w;
        g_res.fxs_h[k] = h;
    }
    GlowSave sv;
    sv.capture(ctx);
    const float id0 = cbd.fx_meta[0];
    cbd.fx_meta[0] = id;
    const bool ok = upload_obj_cb(ctx, cb, cbd);
    cbd.fx_meta[0] = id0;
    if (ok) {
        ctx->OMSetRenderTargets(1, &g_res.fxs_rtv[k], nullptr);
        if (k != 0) {
            D3D11_VIEWPORT vp = {};
            vp.Width = 1.0f;
            vp.Height = 1.0f;
            vp.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &vp);
        }
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        // WSEE_EDGE_COST: fog scatter's value (k 0) with its own entry where made (PSFsSide: PSEffect's branch 28,
        // bit for bit, at its own register count); every other side value, and k 0 without it, with PSEffect.
        ctx->PSSetShader((k == 0 && id == 28.0f && g_res.ps_fs_side) ? g_res.ps_fs_side : g_res.ps_effect,
                         nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        if (t0) ctx->PSSetShaderResources(0, 1, &t0);
        const bool sp = st && k == 0 && g_res.dss_wsee_ssel;   // WSEE_EDGE_COST.
        if (sp && w == g_res.wsee_e_w && h == g_res.wsee_e_h) {
            ConstantData up = cbd;   // WSEE_EPIX: the slice uploaded above.
            up.fx_meta[0] = id;
            wsee_split_draw(ctx, g_res.fxs_rtv[k], st, cb, &up);
        } else {
            ctx->Draw(3, 0);
        }
    }
    sv.restore(ctx);
    return ok;
}

// FX_LINZ - draw linear-depth target k (effect.hlsl's note: 0 the frame, fxMeta.x 34; 1 the SSGI grid's copy, 35;
// 2 dynamicLightFog's, 36) with PSFxLinZ, w x h, one texel per pixel, from cbd - the reading pass's
// own constants: its depth lanes, and for a grid its mapping in local0 and its matCtl.x arm, so the copy reads the
// frame's. fx_side_draw's contract: it uploads its own slice to cb (the pass uploads after it) and every
// other binding it makes is put back (GlowSave); the target's own slot (t7 + k) is nulled first and left null - the
// caller binds the result. The target is made at first use and remade on a size change. False = none drawn: the
// readers stay unarmed and convert in place.
inline bool fx_linz_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx, int k, ConstantData& cbd,
                         UINT w, UINT h, ID3D11Buffer* cb) {
    if (!dev || !ctx || !cb || k < 0 || k > 2) return false;
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return false;
    if (!g_res.ps_fx_linz || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    ID3D11ShaderResourceView* null = nullptr;
    ctx->PSSetShaderResources(7 + static_cast<UINT>(k), 1, &null);   // Off its slot before it is a target.
    if (!g_res.linz_tex[k] || !g_res.linz_rtv[k] || !g_res.linz_srv[k] ||
        g_res.linz_w[k] != w || g_res.linz_h[k] != h) {
        KH_SAFE_RELEASE(g_res.linz_srv[k]);
        KH_SAFE_RELEASE(g_res.linz_rtv[k]);
        KH_SAFE_RELEASE(g_res.linz_tex[k]);
        g_res.linz_w[k] = g_res.linz_h[k] = 0;
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.linz_tex[k])) ||
            FAILED(dev->CreateRenderTargetView(g_res.linz_tex[k], nullptr, &g_res.linz_rtv[k])) ||
            FAILED(dev->CreateShaderResourceView(g_res.linz_tex[k], nullptr, &g_res.linz_srv[k]))) {
            KH_SAFE_RELEASE(g_res.linz_srv[k]);
            KH_SAFE_RELEASE(g_res.linz_rtv[k]);
            KH_SAFE_RELEASE(g_res.linz_tex[k]);
            report_error_once_safe("KH RenderIntegration: effects: linear-depth target could not be made (the depth "
                                   "is read in place)");
            return false;
        }
        g_res.linz_w[k] = w;
        g_res.linz_h[k] = h;
    }
    GlowSave sv;
    sv.capture(ctx);
    const float id0 = cbd.fx_meta[0];
    cbd.fx_meta[0] = 34.0f + static_cast<float>(k);
    const bool ok = upload_obj_cb(ctx, cb, cbd);
    cbd.fx_meta[0] = id0;
    if (ok) {
        ctx->OMSetRenderTargets(1, &g_res.linz_rtv[k], nullptr);
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(w);
        vp.Height = static_cast<float>(h);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_fx_linz, nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
    }
    sv.restore(ctx);
    return ok;
}

// DLF_SPLIT - the stencil dynamicLightFog's gather draws under (the note at Resources::ps_dlf_gather_lean):
// D24_UNORM_S8_UINT at t's size (the grid target; a render target and a depth-stencil view bound together must
// agree in size), made at first use and remade on a size change, with the two states (no depth; ALWAYS / REPLACE to
// mark, EQUAL / KEEP with no write to select), made once with the device. Null = no split this time: either entry
// absent, a multisampled target, or a refused make (reported once; latched at that size, another size tries again).
// The chain's thread.
inline ID3D11DepthStencilView* dlf_split_ensure(ID3D11Device* dev, ID3D11Texture2D* t) {
    if (!dev || !t || !g_res.ps_dlf_gather_lean || !g_res.ps_dlf_class) return nullptr;
    D3D11_TEXTURE2D_DESC td = {};
    t->GetDesc(&td);
    if (td.SampleDesc.Count != 1 || td.Width == 0 || td.Height == 0) return nullptr;
    if (g_res.dlf_st_dsv && g_res.dss_dlf_mark && g_res.dss_dlf_sel && g_res.dlf_st_w == td.Width &&
        g_res.dlf_st_h == td.Height) {
        return g_res.dlf_st_dsv;
    }
    if (g_res.dlf_st_fail_w == td.Width && g_res.dlf_st_fail_h == td.Height) return nullptr;
    KH_SAFE_RELEASE(g_res.dlf_st_dsv);
    KH_SAFE_RELEASE(g_res.dlf_st_tex);
    g_res.dlf_st_w = g_res.dlf_st_h = 0;
    D3D11_TEXTURE2D_DESC sd = {};
    sd.Width = td.Width;
    sd.Height = td.Height;
    sd.MipLevels = 1;
    sd.ArraySize = 1;
    sd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    sd.SampleDesc.Count = 1;
    sd.Usage = D3D11_USAGE_DEFAULT;
    sd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = FALSE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    dd.StencilEnable = TRUE;
    dd.StencilReadMask = 0xFF;
    dd.StencilWriteMask = 0xFF;
    dd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dd.BackFace = dd.FrontFace;
    bool ok = SUCCEEDED(dev->CreateTexture2D(&sd, nullptr, &g_res.dlf_st_tex)) &&
              SUCCEEDED(dev->CreateDepthStencilView(g_res.dlf_st_tex, nullptr, &g_res.dlf_st_dsv));
    if (ok && !g_res.dss_dlf_mark) {
        ok = SUCCEEDED(dev->CreateDepthStencilState(&dd, &g_res.dss_dlf_mark));
    }
    if (ok && !g_res.dss_dlf_sel) {
        dd.StencilWriteMask = 0x00;
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        dd.BackFace = dd.FrontFace;
        ok = SUCCEEDED(dev->CreateDepthStencilState(&dd, &g_res.dss_dlf_sel));
    }
    if (!ok) {
        KH_SAFE_RELEASE(g_res.dlf_st_dsv);
        KH_SAFE_RELEASE(g_res.dlf_st_tex);
        g_res.dlf_st_fail_w = td.Width;
        g_res.dlf_st_fail_h = td.Height;
        report_error_once_safe("KH RenderIntegration: dynamicLightFog: the gather's stencil could not be made (the "
                               "full gather draws alone)");
        return nullptr;
    }
    g_res.dlf_st_w = td.Width;
    g_res.dlf_st_h = td.Height;
    return g_res.dlf_st_dsv;
}

// Retained-mode flush: geometry then post-processing, with the scene captured at most once per frame. flush_frame runs
// once per frame from the C++ Draw3D EH (game thread); the flush body runs on the render thread at its scene resolve,
// or under a park (NO_PARK); FX_LATE's chain-only run may be the game thread's, inside a resolve the engine
// issues there. Fully dormant when there is nothing to do.

// The per-frame work, run from flush_locked. RENDER_FLUSH: the fullscreen chain (the ping-pong through chain_tex,
// the SSGI seed / gather / resolve, the LUT and user-shader passes, the fusion, the final write-back into the saved
// OM). What the flush supplies from its state arrives as parameters: the depth SRV it had at t1 (null when the depth
// is not ready), whether a scene capture is still owed (when meshes drew, or the flush's setup left its copy here -
// CAP_DEFER), its clock (FUSE_CLOCK), and two callables - the per-pass constant-buffer fill (FlushPass::upload_cb,
// chain_pass = true, deferred) and the depth-demand predicate - as template parameters, so the call sites inside are
// the flush's own text. The caller performs the read-only DSV swap before calling and keeps its OM save; this
// function's own OmSave is what the final pass restores into. flush_locked is the one caller (FlushPass::run_chain):
// its scene flush and its chain-only run (FX_LATE) alike.
template <class FxUploadCb, class FxNeedsDepth>
inline void fx_chain_run(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                         std::vector<std::pair<uint64_t, RenderObject>>& fullscreen,
                         const FLOAT bf[4], ID3D11ShaderResourceView* depth_srv,
                         bool capture, FxUploadCb&& upload_cb, FxNeedsDepth&& needs_depth,
                         double now_d,   // FUSE_CLOCK: the flush's clock (upload_cb's now_d).
                         const LocalCull* lc = nullptr) {   // LOCAL_CULL: null = none culled.
    // The chain's depth must be AT t1: the mesh loop leaves t1 as the last mesh needed it, and a localized / banded /
    // depth-reading pass reading 0 there reconstructs the camera position everywhere. Null when the depth is not ready,
    // as ps_srvs already says.
    ctx->PSSetShaderResources(1, 1, &depth_srv);
    // NEARZ_MARK: this cycle's marker at t37 (inside StateBackup's range), or none; read once, so the bind and
    // every pass's arm (shadowMeta2.x, below) take one verdict (EA_NZM_XT).
    const bool nzv = nzm_valid();
    {
        ID3D11ShaderResourceView* nzm = nzv ? g_res.nzm_srv : nullptr;
        ctx->PSSetShaderResources(37, 1, &nzm);
    }
    // VM_SEE: this cycle's snapshot (t34) and transmittance (t41), or none - inside the flush's StateBackup range,
    // so handed back with it; read only where a pass is armed (shadowMeta2.w, below).
    const bool vmsee = vmsee_valid();
    {
        ID3D11ShaderResourceView* vss = vmsee ? g_res.vmsee_s_srv : nullptr;
        ID3D11ShaderResourceView* vst = vmsee ? g_res.vmsee_t_srv : nullptr;
        ctx->PSSetShaderResources(34, 1, &vss);
        ctx->PSSetShaderResources(41, 1, &vst);
    }
    std::string chain_err = capture ? scene_capture_timed(dev, ctx) : "";
    if (chain_err.empty()) chain_err = ensure_fx_chain(dev);

    if (!chain_err.empty()) {
        // The mesh-side twin of this pair reports as "effect setup"; this is the chain's own report.
        ensure_ok("fullscreen chain", chain_err);
    } else {
        OmSave om;
        om.capture(ctx);
        // WSEE: this cycle's record at t53 - t69 (the WSEE note's REGISTERS; t56 is the chain's input capture,
        // past this run's capture above, which may remake it), or none: inside the flush's StateBackup range
        // (t0..t69), so handed back with it; read only where a pass is armed (fx_tex[1].z, below). t58 (the run's
        // agreement mask E) is bound once drawn, at the run's first sited pass, and t59 (the scene's sample colours C,
        // fc_wsc) around a C reader's own draw alone (flush: the hazard rule there) - null until then;
        // fx_tex[1].w tells the pass which stand. Built out (WSEE_ON false): none of it (wsee_valid), no want.
        const bool wsee = wsee_valid() && g_res.scene_srv != nullptr;
        bool wsee_want = false;   // Some pass took a mode (valid or not): the want is stamped after the loop.
        ID3D11ShaderResourceView* fc_wsc = nullptr;   // C (wsee_scene_ms; held by g_res), or none.
        bool we_tried = false;   // E attempted (the first sited pass), and drawn.
        bool we_ok = false;
        bool we_st = false;      // WSEE_EDGE_COST: E's stencil marked with it (the split draws may use it).
        bool b2 = false;         // This cycle's B2 moment stands (t60 with a colour want, and S2 t62; bit 3).
        bool col = false;        // WSEE_BWANT: this cycle's capture resolved B (and B2 with it, where taken).
        bool col_want = false;   // WSEE_BWANT: an armed colour reader ran (the want, stamped after the loop).
        bool ld = false;         // This cycle's layer-depth split bound (Tp t63, Tf t64; bit 4).
        bool od = false;         // This cycle's D3 bound (t65; bit 5).
        bool pb = false;         // PSB: this cycle's one-pass record bound (t66 - t69 - PHASE; bit 6).
        bool pe = false;         // PSB: the old record holds no particle-type layer this cycle (bit 7).
        bool np = false;         // WSEE_NOPRE: a pre late opaque skipped this cycle (bit 8).
        bool s2 = false;         // WSEE_NOPRE: this cycle's S2 bound (t62, B2 or not; bit 9).
        {
            ID3D11ShaderResourceView* ws[13] = {};
            if (wsee) {
                ws[0] = g_res.wsee_s_srv;
                ws[1] = g_res.wsee_t_srv;
                // WSEE_BWANT: B (t55) and B2 (t60) only where this cycle's capture resolved them (no colour reader
                // wanted them: not written this cycle - null, and a colour reader runs unarmed below).
                col = g_wsee.col_cycle.load(std::memory_order_relaxed) ==
                      g_topo_cycles_pub.load(std::memory_order_relaxed);
                ws[2] = col ? g_res.wsee_b_srv : nullptr;
                ws[3] = g_res.scene_srv;
                ws[4] = g_res.wsee_n_srv;
                // B2 (t60) where this cycle took it (bit 3 of fx_tex[1].w says so), NWpost (t61).
                b2 = g_res.wsee_b2_srv != nullptr && g_res.wsee_s2_srv != nullptr &&
                     g_wsee.b2_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                ws[7] = (b2 && col) ? g_res.wsee_b2_srv : nullptr;   // Bit 3 stays B2's moment / S2.
                ws[8] = g_res.wsee_n2_srv;
                // WSEE_NOPRE: S2 wherever this cycle took it (the derivation's reference, bit 9 - B2 or not); bit 8
                // where it skipped a pre late opaque's replay (cb.hlsl's note at khWsS2).
                s2 = g_res.wsee_s2_srv != nullptr && g_wsee.s2_cycle.load(std::memory_order_relaxed) ==
                                                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                np = g_wsee.nopre_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                ws[9] = (b2 || s2) ? g_res.wsee_s2_srv : nullptr;   // S2 (t62), B2's twin.
                // Tp (t63) and Tf (t64) where this cycle's capture cleared them; bit 4 of fx_tex[1].w (KhWseeAvail)
                // says so.
                ld = g_res.wsee_tp_srv != nullptr && g_res.wsee_tf_srv != nullptr &&
                     g_wsee.ld_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                ws[10] = ld ? g_res.wsee_tp_srv : nullptr;
                ws[11] = ld ? g_res.wsee_tf_srv : nullptr;
                // D3 (t65) where this cycle's capture stood it up; bit 5 of fx_tex[1].w (KhWseeAvail) says so.
                od = g_res.wsee_d3_srv != nullptr &&
                     g_wsee.od_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                // WSEE_OQ: and complete - a skipped pre opaque with no S2 taken after it leaves it unbound
                // (wsee_d3_ok's note: the live depth is the behind then).
                if (od && !wsee_d3_ok(g_topo_cycles_pub.load(std::memory_order_relaxed))) od = false;
                // WSEE_OQ2: and the post opaques' D3m resolved into it first (wsee_d3m_resolve; not drawable -
                // unbound). Before the binds below: the pass restores this run's state.
                if (od && !wsee_d3m_resolve(ctx, g_topo_cycles_pub.load(std::memory_order_relaxed))) {
                    od = false;
                }
                ws[12] = od ? g_res.wsee_d3_srv : nullptr;
                // PSB: P0, P1, N, N2 (t66 - t69) where a re-issue drew into them this cycle (bit 6 of fx_tex[1].w);
                // bit 7 where no particle-type layer went into the old record (its NW / Tp / Tf hold their clears).
                pb = g_res.wsee_p0_srv != nullptr && g_res.wsee_p1_srv != nullptr &&
                     g_res.wsee_nb_srv != nullptr && g_res.wsee_nb2_srv != nullptr &&   // PSB_PHASE.
                     g_psb.drew_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                // Bit 7 only while the one pass's record stands (else the old record is read whole).
                pe = g_psb.cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed) &&
                     g_psb.psa_cycle.load(std::memory_order_relaxed) !=
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                g_wsee.used_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed), std::memory_order_relaxed);
                // The target bound at the run's start is the scene target the capture above resolved, and the one
                // the final pass writes (om).
                fc_wsc = wsee_scene_ms(dev, om.rtv[0]);
                // WSEE_BIND_ARMED: the two binds only with the record valid - without it every pass's [1].z is 0
                // and no shader loads t53 - t69 (every load is under KhWseeArmed / KhWseeMeshMode), nothing of ours
                // stands there (the sort scope nulls its own at its end) and the flush's StateBackup puts the
                // engine's views back either way: two calls fewer per unarmed run, the 0 build's every run.
                ctx->PSSetShaderResources(53, 13, ws);   // t58 / t59 null: bound later, or never.
                ID3D11ShaderResourceView* const pbv[4] = { pb ? g_res.wsee_p0_srv : nullptr,
                                                           pb ? g_res.wsee_p1_srv : nullptr,
                                                           pb ? g_res.wsee_nb_srv : nullptr,
                                                           pb ? g_res.wsee_nb2_srv : nullptr };   // PHASE.
                ctx->PSSetShaderResources(66, 4, pbv);   // PSB (StateBackup covers t0 - t69; PHASE: N2).
            }
        }
        ctx->IASetInputLayout(nullptr);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);   // Opaque: compositing is in-shader.

        ID3D11ShaderResourceView* src_srv = g_res.scene_srv;
        int write_idx = 0;

        bool                      live = false;
        ConstantData              cbd;
        int                       effect = 0;
        ID3D11PixelShader*        fp_ps = nullptr;
        bool                      fp_upyr = false;   // FX_USER: the user pass reads the pyramid.
        const RenderObject*       obj = nullptr;   // FX_TEX: the pass's object (its textures).
        ID3D11ShaderResourceView* fp_lut = nullptr;
        // FX_LINZ (effect.hlsl's note): this run's linear-depth targets. The frame's is drawn at the run's first
        // pass set to read it (frame: fog scatter at its flush, SSGI past its drop-whole gate, dynamicLightFog
        // once its lights are up), from its constants; its key is that pass's depth lanes (the depth pair, the viewport
        // range, the marker arm, VM_SEE's substitution) and frame size, and a pass is armed (mat_ctl x = 1) only
        // when its own are the key - t1, t37 and t34, the conversion's textures, are the run's own (bound above). A
        // grid's copy is drawn for an armed pass by its block, with its own mapping (the SSGI factor; the DLF grid),
        // and arms mat_ctl w. t7 - t9 are taken at the first draw and given back at the run's end.
        bool                      saved = false;
        ID3D11ShaderResourceView* prev[3] = {};
        bool                      tried = false;
        bool                      ok = false;
        float                     key[8] = {};
        bool                      gs_tried = false;
        bool                      gs_ok = false;
        float                     gs_inv = 0.0f;
        bool                      gd_tried = false;
        bool                      gd_ok = false;
        ID3D11ShaderResourceView* none = nullptr;   // t8 / t9 for an unarmed pass.
        auto save = [&]() {
            if (saved) return;
            ctx->PSGetShaderResources(7, 3, prev);
            saved = true;
        };
        auto keyof = [](const ConstantData& c, float out[8]) {
            out[0] = c.depth_params[0];
            out[1] = c.depth_params[1];
            out[2] = c.depth_params[2];
            out[3] = c.depth_params[3];
            out[4] = c.shadow_meta2[0];
            out[5] = c.fx_meta[2];
            out[6] = c.fx_meta[3];
            out[7] = c.shadow_meta2[3] > 0.5f ? 1.0f : 0.0f;   // VM_SEE: substitutes (1 and 2 alike).
        };
        // The frame's linear depth for a builtin SSGI / dynamicLightFog / fog-scatter pass that will read it - drawn
        // at the run's first such call, from that pass's constants; then this pass armed when its lanes are the key
        // (any other converts in place).
        auto frame = [&](ConstantData& pc) {
            float k[8];
            keyof(pc, k);
            if (!tried) {
                tried = true;
                const bool in = k[5] >= 1.0f && k[5] <= 16384.0f &&
                                k[6] >= 1.0f && k[6] <= 16384.0f;
                const UINT w = in ? static_cast<UINT>(k[5]) : 0u;
                const UINT h = in ? static_cast<UINT>(k[6]) : 0u;
                if (w > 0 && h > 0 && static_cast<float>(w) == k[5] &&
                    static_cast<float>(h) == k[6]) {   // A whole frame: the shader's fxMeta.zw bound.
                    save();
                    ConstantData c = pc;
                    ok = fx_linz_draw(dev, ctx, 0, c, w, h, g_res.constant_buffer);
                    if (ok) {
                        memcpy(key, k, sizeof(key));
                        ctx->PSSetShaderResources(7, 1, &g_res.linz_srv[0]);
                    }
                }
            }
            if (ok && memcmp(key, k, sizeof(key)) == 0) pc.mat_ctl[0] = 1.0f;
        };

        // FX_FUSE: a return below past this point (the SSGI block's missing target, sampler or entry point; a
        // failed constant upload) leaves the pass undrawn with every stage fused onto it (fuse_append), and as the
        // chain's final pass its write into the scene too - the scene then keeps the picture the run found, without
        // any of the chain's passes, for that frame (the passes before it drew into a ping-pong buffer nothing reads
        // after; FX_BAIL_DOC). The loop skips the known causes before it fuses (SSGI without its entry points,
        // dynamicLightFog with nothing to add); the rest ensure_fx_chain makes all-or-nothing, so none is met in
        // practice. No fall-through copy: the bail sites leave the OM, viewport and s2 in differing states, and the
        // only copy shader (ps_wsee_copy) stands with WSEE alone.
        auto flush = [&](bool fp_final) {
            if (!live) return;
            live = false;
            // GPU_PASS: the pass whole - its side draws, pyramids and linear-depth builds included.
            GpuPassScope pass(ctx, 0u, effect, fp_ps != nullptr,
                              static_cast<uint32_t>(cbd.fuse_meta[0] + 0.5f));
            // FX_TEX: the pass's own textures - a user effect's, or (FX_TEX_ALL) a builtin pass's that names
            // some. The lanes go into cbd before any side draw below takes it or a copy, and the pages stay
            // bound from here to the pass's end (ts unbinds on every exit), so the side draws and the pass read
            // one set; nothing else in the chain binds t43 - t48 or s0.
            ID3D11ShaderResourceView* tex[MAT_USER_MAPS] = {};
            FxTexScope ts;
            if (obj && fx_tex_on(*obj)) {
                fx_tex_fill(dev, ctx, cbd, *obj, tex);
                ts.bind(ctx, tex);
            }
            // WSEE: an armed pass (its mode: the loop below) - the run's agreement mask E at the first sited one,
            // from its constants and before its side draws (they read t58 too; the frame's size, FX_LINZ's rule),
            // then bit 0 of fx_tex[1].w on every armed pass while E stands; bit 1 is the pass's own draw's (below),
            // never its side draws'; bit 2 on every armed pass (both E and the inline per-sample test need it).
            const bool wsa = cbd.fx_tex[1][2] > 0.5f;
            // WSEE_EDGE_COST: E at the run's first armed pass with a per-sample site (wsee_ps_site), not
            // the first armed one - a pass without a site reads neither E nor bit 0, so the value every reader gets
            // is the same (E's inputs, t1 and t54, are the run's); a run without a site draws no E.
            if (wsa && !we_tried && obj && wsee_ps_site(*obj)) {
                we_tried = true;
                const float fw = cbd.fx_meta[2], fh = cbd.fx_meta[3];
                const bool in = fw >= 1.0f && fw <= 16384.0f && fh >= 1.0f && fh <= 16384.0f;
                const UINT w = in ? static_cast<UINT>(fw) : 0u;
                const UINT h = in ? static_cast<UINT>(fh) : 0u;
                if (w > 0 && h > 0 && static_cast<float>(w) == fw &&
                    static_cast<float>(h) == fh) {
                    we_ok = wsee_agree_draw(dev, ctx, cbd, w, h, g_res.constant_buffer,
                                            we_st);
                }
                if (we_ok) ctx->PSSetShaderResources(58, 1, &g_res.wsee_e_srv);
            }
            cbd.fx_tex[1][3] = (wsa && we_ok) ? 1.0f : 0.0f;
            if (wsa) cbd.fx_tex[1][3] += 4.0f;   // Bit 2, per sample allowed.
            if (wsa && b2) cbd.fx_tex[1][3] += 8.0f;   // Bit 3, B2's moment (t60; S2, t62).
            if (wsa && ld) cbd.fx_tex[1][3] += 16.0f;   // Bit 4, Tp / Tf (t63 / t64).
            if (wsa && od) cbd.fx_tex[1][3] += 32.0f;   // Bit 5, D3 (t65).
            if (wsa && pb) cbd.fx_tex[1][3] += 64.0f;   // PSB: bit 6, P0 / P1 / N / N2 (t66 - t69).
            if (wsa && pe) cbd.fx_tex[1][3] += 128.0f;   // PSB: bit 7, the old record unread.
            if (wsa && np) cbd.fx_tex[1][3] += 256.0f;   // WSEE_NOPRE: bit 8, pre derived;
            if (wsa && s2) cbd.fx_tex[1][3] += 512.0f;   // bit 9, S2 bound (t62).
            // FX_LINZ: fog scatter reads the depth whenever it runs (its side draw below, then the pass).
            if (!fp_ps && effect == static_cast<int>(EffectId::Fogscatter)) frame(cbd);
            bool sg_pair = false;
            // The resolve reads whichever half-res buffer holds the final smoothed field (three a-trous iterations,
            // steps 1 / 2 / 4, ping-pong [2]->[3]->[2]->[3]; a failed iteration leaves the pointer at the last good
            // stage).
            ID3D11ShaderResourceView* sg_fin = nullptr;
            ID3D11SamplerState* s2old = nullptr;   // FX_SAMP_S2: the saved s2.
            if (effect == static_cast<int>(EffectId::Ssgi) && !fp_ps) {
                if (!g_res.chain_srv[2] || !g_res.chain_srv[4] ||
                    !g_res.chain_rtv[4] || !g_res.sampler ||
                    !g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous) return;   // The seed and the gather's and
                                                                               // a-trous' entry points (FX_SPLIT)
                                                                               // join the drop-whole gate.
                ctx->PSGetSamplers(2, 1, &s2old);   // FX_SAMP_S2: slot 2.
                const float l0sv = cbd.local0[0];
                const float l1sv = cbd.local0[1];
                const float l2sv = cbd.local0[2];
                const float l3sv = cbd.local0[3];
                cbd.local0[1] = (float)g_res.scene_w / (float)(g_ssgi_w > 0 ? g_ssgi_w : 1);
                // SSGI_CENTRE: the grid's own size for the side draws (seed, gather, a-trous, its depth copy) -
                // each axis's factor and the a-trous' bound (effect.hlsl KhSsgiGrid); put back with local0.xy.
                cbd.local0[2] = (float)(g_ssgi_w > 0 ? g_ssgi_w : 1);
                cbd.local0[3] = (float)(g_ssgi_h > 0 ? g_ssgi_h : 1);
                // FX_LINZ: the frame's (past the drop-whole gate: the pass reads it from here on), then the grid's
                // copy at this factor, for the a-trous (KhSsgiGridZ) and the resolve (SSGI_JBU, effect 24's Gather)
                // - an armed pass's, drawn once a run per factor; mat_ctl w = 1 arms it. t8 holds this pass's copy, or
                // nothing when unarmed (the helper's size query and load meet no other route's view).
                frame(cbd);
                if (cbd.mat_ctl[0] > 0.5f && g_ssgi_w > 0 && g_ssgi_h > 0) {
                    if (!gs_tried || gs_inv != cbd.local0[1]) {
                        gs_tried = true;
                        save();
                        ConstantData c = cbd;
                        gs_ok = fx_linz_draw(dev, ctx, 1, c, static_cast<UINT>(g_ssgi_w),
                                             static_cast<UINT>(g_ssgi_h), g_res.constant_buffer);
                        gs_inv = cbd.local0[1];
                    }
                    if (gs_ok) cbd.mat_ctl[3] = 1.0f;
                }
                save();
                ctx->PSSetShaderResources(8, 1, cbd.mat_ctl[3] > 0.5f ? &g_res.linz_srv[1] : &none);
                ctx->PSSetSamplers(2, 1, &g_res.sampler);
                // Every exit puts s2 back and releases the saved ref: the three failed uploads below, the pass's own
                // failed upload (FX_SSGI_BAIL) and the pass's end. s1 itself is untouched at each.
                if (!upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                    ctx->PSSetSamplers(2, 1, &s2old);
                    if (s2old) s2old->Release();
                    return;
                }
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
                ID3D11ShaderResourceView* sg_null = nullptr;
                ctx->PSSetShaderResources(0, 1, &sg_null);
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                // The gather draws on the half viewport; the saved one is restored before the resolve body.
                D3D11_VIEWPORT vps; UINT vpn = 1;
                ctx->RSGetViewports(&vpn, &vps);
                D3D11_VIEWPORT vph = {};
                vph.Width    = (float)(g_ssgi_w > 0 ? g_ssgi_w : 1);
                vph.Height   = (float)(g_ssgi_h > 0 ? g_ssgi_h : 1);
                vph.MaxDepth = 1.0f;
                ctx->RSSetViewports(1, &vph);
                cbd.fx_meta[0] = 26.0f;
                if (!upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                    if (vpn) ctx->RSSetViewports(1, &vps);
                    ctx->PSSetSamplers(2, 1, &s2old);
                    if (s2old) s2old->Release();
                    return;
                }
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[4], nullptr);
                ctx->PSSetShaderResources(0, 1, &src_srv);
                ctx->Draw(3, 0);   // The radiance seed (effect 26), the gather's one level (SSGI_SEED).
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                cbd.fx_meta[0] = 22.0f;
                if (!upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                    if (vpn) ctx->RSSetViewports(1, &vps);
                    ctx->PSSetSamplers(2, 1, &s2old);
                    if (s2old) s2old->Release();
                    return;
                }
                ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[4]);
                ctx->PSSetShader(g_res.ps_ssgi_gather, nullptr, 0);   // FX_SPLIT: its own entry point.
                ctx->Draw(3, 0);   // The gather (t0 = source for albedo, t3 = the seed its taps read).
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
                sg_fin = g_res.chain_srv[2];   // Raw gather until smoothing lands.
                // Missing buffer or a failed upload skips the draw whole - the resolve reads the raw gather instead
                // (one noisier frame, never a lost pass).
                if (g_res.chain_rtv[3] && g_res.chain_srv[3]) {
                    cbd.fx_meta[0] = 25.0f;
                    cbd.local0[0] = 1.0f;
                    ctx->PSSetShader(g_res.ps_ssgi_atrous, nullptr, 0);   // FX_SPLIT: the three iterations'.

                    if (upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                        ctx->OMSetRenderTargets(1, &g_res.chain_rtv[3], nullptr);
                        ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[2]);
                        ctx->Draw(3, 0);
                        ID3D11ShaderResourceView* n3 = nullptr;
                        ctx->PSSetShaderResources(3, 1, &n3);
                        sg_fin = g_res.chain_srv[3];

                        cbd.local0[0] = 2.0f;   // Iteration B: sp, back into [2].
                        if (upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                            ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[3]);
                            ctx->Draw(3, 0);
                            ctx->PSSetShaderResources(3, 1, &n3);
                            sg_fin = g_res.chain_srv[2];

                            cbd.local0[0] = 4.0f;
                            if (upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[3], nullptr);
                                ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[2]);
                                ctx->Draw(3, 0);
                                ctx->PSSetShaderResources(3, 1, &n3);
                                sg_fin = g_res.chain_srv[3];
                            }
                        }
                    }

                }
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);   // FX_SPLIT: the pass's own again.
                cbd.local0[0] = l0sv;
                cbd.local0[1] = l1sv;
                cbd.local0[2] = l2sv;
                cbd.local0[3] = l3sv;
                if (vpn) {
                    ctx->RSSetViewports(1, &vps);
                } else {
                    vph.Width  = (float)g_res.scene_w;
                    vph.Height = (float)g_res.scene_h;
                    ctx->RSSetViewports(1, &vph);
                }
                // When the spread param is auto (< 0.5) the flush writes the scale-matched value 1.5 x inv; explicit
                // user values pass through untouched. cbd is per-pass, so no restore is owed.
                if (cbd.fx2[3] < 0.5f)
                    cbd.fx2[3] = 1.5f * ((float)g_res.scene_w
                               / (float)(g_ssgi_w > 0 ? g_ssgi_w : 1));
                cbd.fx_meta[0] = 24.0f;
                sg_pair = true;
            }
            // DLF: dynamicLightFog - its lights into the ring (t40, dlf_select), our meshes' light maps at
            // t36 (shadows >= 1; their lanes are the frame's, fx_frame_template's), the gather (32) onto the SSGI
            // grid's [2], two filter iterations (33: [2] -> [3] at step 1, [3] -> [2] at step 2; a failed upload
            // keeps the last good buffer, the SSGI's rule), then the pass itself (31) composites the final buffer
            // from t3, bound below once the OM has moved off it.
            // dl_ctl.x arms the composite; unarmed (no light selected, the ring, the grid or the gather's upload
            // failed, an unarmed camera, no medium - DLF_MEDIUM0) it passes the picture through. local0 carries
            // the grid to the side draws (x = the filter's step, y = the grid factor, zw = the grid's size) and is put
            // back before the composite, whose localization mask reads it; so is the viewport.
            ID3D11ShaderResourceView* dlf_fin = nullptr;
            if (effect == static_cast<int>(EffectId::DynLightFog) && !fp_ps) {
                KH_GPU_SCOPE(ctx, GPU_DLF);   // PROF.
                cbd.dl_ctl[0] = 0.0f;
                static thread_local std::vector<float> rec;   // Scratch.
                uint32_t pn = 0, first = 0;
                float scale = 1.0f;
                // DLF_MEDIUM0: no medium anywhere - every sample of the gather continues, the grid holds 0 (or a
                // NaN of a non-finite tint), and the armed composite adds max(that, 0) = 0: the unarmed composite's
                // picture, so the pass runs as with no light selected (no ring append, gather or filter, and no frame
                // linear depth - a later reader draws it, by the same key; the composite still writes, so the chain's
                // write-back stands). A channel holding -0.0 may come out +0.0 or the reverse under additive /
                // screen - zero either way. dlf_select is not called, so its dlf_unshadowed stat (DL_DIAG)
                // counts only lights of passes that have a medium.
                const uint32_t n = (g_res.chain_rtv[2] && g_res.chain_srv[2] && g_ssgi_w > 0 && g_ssgi_h > 0 &&
                                    g_res.ps_dlf_gather && g_res.ps_dlf_filter &&   // FX_SPLIT.
                                    !dlf_medium_zero(cbd))
                                 ? dlf_select(cbd, rec, pn, scale) : 0u;
                const bool up = n > 0 && dlr_append(ctx, rec.data(), n * 6u, first);
                if (n > 0 && !up) stat_inc(g_stats.dl_ring_fails);   // DL_DIAG.
                if (up) {
                    memcpy(&cbd.dl_first[0], &first, sizeof(first));
                    cbd.dl_first[1] = 0.0f;
                    cbd.dl_first[2] = 0.0f;
                    cbd.dl_first[3] = 0.0f;
                    dl_fill_ctl(cbd, n, pn, scale);   // dl_ctl.x = 3: armed.
                    float l0[4];
                    memcpy(l0, cbd.local0, sizeof(l0));
                    cbd.local0[0] = 1.0f;
                    cbd.local0[1] = static_cast<float>(g_res.scene_w) / static_cast<float>(g_ssgi_w);
                    cbd.local0[2] = static_cast<float>(g_ssgi_w);
                    cbd.local0[3] = static_cast<float>(g_ssgi_h);
                    // FX_LINZ: the frame's (the lights are up: the gather, the filter and the composite read
                    // it), then the grid's copy (KhDlfGridZ: KhDlfFull at this grid) for the filter and the
                    // composite - an armed pass's, drawn once a run; mat_ctl w = 2 arms it. t9 holds this pass's
                    // copy, or nothing when unarmed (the helper's size query and load meet no other route's view).
                    frame(cbd);
                    if (cbd.mat_ctl[0] > 0.5f) {
                        if (!gd_tried) {
                            gd_tried = true;
                            save();
                            ConstantData c = cbd;
                            gd_ok = fx_linz_draw(dev, ctx, 2, c, static_cast<UINT>(g_ssgi_w),
                                                 static_cast<UINT>(g_ssgi_h), g_res.constant_buffer);
                        }
                        if (gd_ok) cbd.mat_ctl[3] = 2.0f;
                    }
                    save();
                    ctx->PSSetShaderResources(9, 1, cbd.mat_ctl[3] > 1.5f ? &g_res.linz_srv[2] : &none);
                    D3D11_VIEWPORT vps; UINT vpn = 1;
                    ctx->RSGetViewports(&vpn, &vps);
                    D3D11_VIEWPORT vpg = {};
                    vpg.Width    = static_cast<float>(g_ssgi_w);
                    vpg.Height   = static_cast<float>(g_ssgi_h);
                    vpg.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &vpg);
                    ctx->PSSetShader(g_res.ps_dlf_gather, nullptr, 0);   // FX_SPLIT: its own entry point.
                    ID3D11ShaderResourceView* dlf_null = nullptr;
                    ctx->PSSetShaderResources(3, 1, &dlf_null);   // Off t3 before [2] becomes a target.
                    cbd.fx_meta[0] = static_cast<float>(DLF_GATHER);
                    if (upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                        ID3D11ShaderResourceView* dls = (cbd.fx1[2] >= 0.5f) ? g_res.dls_srv : nullptr;
                        // DLF_SPLIT (the note at Resources::ps_dlf_gather_lean): an armed pass marks the texels a
                        // layer may cover (PSDlfClass, stencil 1) and draws the full gather there, the lean build on
                        // the rest; an unarmed pass draws the lean build alone (the full build's record code does not
                        // run there); an armed pass without the marker or the stencil, or any pass without the lean
                        // build, the full build alone. The pass's constants serve all three draws (the
                        // marker reads the gather's own lanes).
                        ID3D11DepthStencilView* const dsv =
                            wsa ? dlf_split_ensure(dev, g_res.chain_tex[2]) : nullptr;
                        if (dsv) {
                            ID3D11DepthStencilState* old = nullptr;
                            UINT oref = 0;
                            ctx->OMGetDepthStencilState(&old, &oref);
                            // STEN_CLEAR_BOTH (E's stencil's note): the depth plane is unread.
                            ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                            ctx->OMSetRenderTargets(0, nullptr, dsv);   // The marker writes the stencil alone.
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_mark, 1);
                            ctx->PSSetShader(g_res.ps_dlf_class, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], dsv);
                            ctx->PSSetShaderResources(0, 1, &src_srv);
                            ctx->PSSetShaderResources(36, 1, &dls);
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_sel, 0);
                            ctx->PSSetShader(g_res.ps_dlf_gather_lean, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_sel, 1);
                            ctx->PSSetShader(g_res.ps_dlf_gather, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);   // The stencil off the OM.
                            ctx->OMSetDepthStencilState(old, oref);
                            KH_SAFE_RELEASE(old);
                        } else {
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                            ctx->PSSetShaderResources(0, 1, &src_srv);
                            ctx->PSSetShaderResources(36, 1, &dls);
                            if (!wsa && g_res.ps_dlf_gather_lean) {
                                ctx->PSSetShader(g_res.ps_dlf_gather_lean, nullptr, 0);
                            }
                            ctx->Draw(3, 0);
                        }
                        ctx->PSSetShaderResources(36, 1, &dlf_null);
                        dlf_fin = g_res.chain_srv[2];
                        if (g_res.chain_rtv[3] && g_res.chain_srv[3]) {
                            cbd.fx_meta[0] = static_cast<float>(DLF_FILTER);
                            ctx->PSSetShader(g_res.ps_dlf_filter, nullptr, 0);   // FX_SPLIT: both iterations'.
                            for (int it = 0; it < 2; ++it) {
                                cbd.local0[0] = it == 0 ? 1.0f : 2.0f;
                                if (!upload_obj_cb(ctx, g_res.constant_buffer, cbd)) break;
                                const int to = it == 0 ? 3 : 2;
                                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[to], nullptr);
                                ctx->PSSetShaderResources(3, 1, &dlf_fin);
                                ctx->Draw(3, 0);
                                ctx->PSSetShaderResources(3, 1, &dlf_null);
                                dlf_fin = g_res.chain_srv[to];
                            }
                        }
                    }
                    ctx->PSSetShader(g_res.ps_effect, nullptr, 0);   // FX_SPLIT: the pass's own again.
                    memcpy(cbd.local0, l0, sizeof(l0));
                    if (vpn) {
                        ctx->RSSetViewports(1, &vps);
                    } else {
                        vpg.Width  = static_cast<float>(g_res.scene_w);
                        vpg.Height = static_cast<float>(g_res.scene_h);
                        ctx->RSSetViewports(1, &vpg);
                    }
                    cbd.fx_meta[0] = static_cast<float>(EffectId::DynLightFog);
                    if (!dlf_fin) cbd.dl_ctl[0] = 0.0f;
                }
            }
            // FX_SIDE: fog scatter's per-pixel fog (side id 28, the frame's size) or the sun flare's visibility
            // (29, 1 x 1), drawn before the pass's constants go up; they then arm matCtl.y, and t4 carries it.
            int side = -1;
            if (!fp_ps && (effect == static_cast<int>(EffectId::Fogscatter) ||
                           effect == static_cast<int>(EffectId::SunFlare))) {
                const bool fog = effect == static_cast<int>(EffectId::Fogscatter);
                const UINT w = fog ? static_cast<UINT>((cbd.fx_meta[2] > 0.0f ? cbd.fx_meta[2] : 0.0f) + 0.5f) : 1u;
                const UINT h = fog ? static_cast<UINT>((cbd.fx_meta[3] > 0.0f ? cbd.fx_meta[3] : 0.0f) + 0.5f) : 1u;
                // WSEE_EDGE_COST: fog scatter's value split by E's stencil where it stands (an armed pass).
                ID3D11DepthStencilView* const st = (fog && wsa && we_st) ? g_res.wsee_es_dsv
                                                                                             : nullptr;
                if (fx_side_draw(dev, ctx, fog ? 0 : 1, cbd, fog ? 28.0f : 29.0f, w, h,
                                 nullptr, g_res.constant_buffer, st)) {
                    side = fog ? 0 : 1;
                    cbd.mat_ctl[1] = 1.0f;
                }
            }
            // GLOW_PYR: a glow pass's pyramid, of this pass's own source, built before its constants go up.
            bool on = false;
            if ((!fp_ps && glow_effect(effect)) || (fp_ps && fp_upyr)) {   // FX_USER: or a user's.
                on = glow_build(dev, ctx, g_res.glow_pyr[0], src_srv, cbd.fx_meta[2], cbd.fx_meta[3],
                                cbd.center_size[3], effect, cbd.fx0[0], g_res.constant_buffer);
                glow_lanes(cbd, on ? &g_res.glow_pyr[0] : nullptr);
            }
            // ANA_PYR: anamorphic's one-axis pyramid (per axis), likewise before its constants go up.
            const AnaPyr* pyr = nullptr;
            if (!fp_ps && effect == static_cast<int>(EffectId::Anamorphic)) {
                const int ax = cbd.fx1[0] > 0.5f ? 1 : 0;
                if (ana_build(dev, ctx, g_res.ana_pyr[0][ax], src_srv, cbd.fx_meta[2], cbd.fx_meta[3],
                              cbd.center_size[3], ax, cbd.fx0[0], g_res.constant_buffer))
                    pyr = &g_res.ana_pyr[0][ax];
                ana_lanes(cbd, pyr);
            }
            // WSEE: C (t59) for an armed pass's own draw - the scene target's samples, which the run's final
            // write overwrites. The runtime refuses a view of a resource bound as an output (it nulls the view; the
            // reads give 0), so C is bound only while the OM holds a ping-pong target - after the target switch below,
            // nulled right after the draw - and never across a side draw (each puts back the OM it found, the scene's
            // at the run's first pass). A last pass that may read it (the distance fog; a user effect, whose helpers
            // may) draws into the ping-pong target as a middle pass does, and the scene target takes a copy of that
            // (ps_wsee_copy, below: the chain's format is the capture's, so the same values); any other last pass
            // draws straight into the scene, without C. Bit 1 of fx_tex[1].w goes up with this draw alone.
            // WSEE_EDGE_COST: C goes to a reader's draw alone (the distance fog, a user effect) - a middle pass
            // that never loads t59 draws without it (bit 1 is read by effect 13 and a user effect alone).
            const bool wsc_reader = effect == static_cast<int>(EffectId::Fog) || fp_ps != nullptr;
            const bool reroute = fp_final && wsa && fc_wsc && wsc_reader && g_res.ps_wsee_copy;
            const bool fp_wsc = wsa && fc_wsc && wsc_reader && (!fp_final || reroute);
            if (fp_wsc) cbd.fx_tex[1][3] += 2.0f;
            if (!upload_obj_cb(ctx, g_res.constant_buffer, cbd)) {
                if (sg_pair) ctx->PSSetSamplers(2, 1, &s2old);   // FX_SSGI_BAIL: s2 back on
                                                                        // the pair path.
                if (s2old) s2old->Release();   // The pair path's ref (twin above; null on
                                                         // every non-pair path).
                return;
            }
            if (fp_lut) ctx->PSSetShaderResources(19, 1, &fp_lut);
            // FS_SPLIT: fog scatter's own entry where its side draw is armed (side 0: mat_ctl y up, t4 bound
            // below) - PSFsSplitS reads t4 unconditionally; a failed side draw or the entry absent: PSEffect's branch.
            const bool fs_split = side == 0 && g_res.ps_fs_split != nullptr;
            // KH_FX_LEAN: an unlocalized, unbanded pass (the shaders' own tests of localParams1.y / bandParams.w)
            // takes the lean build - fog scatter's entry, dynamicLightFog's composite (PSEffect's branch 31) - which
            // runs no record maths: such a pass reaches none (cb.hlsl's KH_FX_LEAN note), so the same values.
            const bool fp_lean = !(cbd.local1[1] > 0.5f) && !(cbd.band0[3] > 0.5f);
            ID3D11PixelShader* const fs_ps = (fp_lean && g_res.ps_fs_split_lean) ? g_res.ps_fs_split_lean
                                                                                     : g_res.ps_fs_split;
            const bool dc_lean = !fp_ps && fp_lean && g_res.ps_dlf_comp != nullptr &&
                                 effect == static_cast<int>(EffectId::DynLightFog);
            // FX_LEAN_PASS (the note at Resources::ps_effect_lean): a builtin pass the constants just uploaded
            // leave unarmed draws PSEffect's lean build.
            const bool plean = !fp_ps && !fs_split && !dc_lean && g_res.ps_effect_lean != nullptr &&
                               !(cbd.fx_tex[1][2] > 0.5f);
            ctx->PSSetShader(fp_ps ? fp_ps : (fs_split ? fs_ps : (dc_lean ? g_res.ps_dlf_comp
                                                                 : (plean ? g_res.ps_effect_lean
                                                                                       : g_res.ps_effect))),
                             nullptr, 0);
            if (fp_ps) ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // FX_USER: s1 for every user effect draw.
            // Unbind the source slot before binding it as RTV next round.
            ID3D11ShaderResourceView* fp_null = nullptr;
            ctx->PSSetShaderResources(0, 1, &fp_null);
            if (fp_final && !reroute) om.restore(ctx);
            else                             ctx->OMSetRenderTargets(1, &g_res.chain_rtv[write_idx], nullptr);
            // WSEE_EDGE_COST: an armed builtin pass with a per-sample site drawing PSEffect into the chain's
            // ping-pong target (a middle pass, or a rerouted last one) is split by E's stencil where the target is
            // E's size (one GetDesc); the final write into the multisampled scene target is never split (no stencil
            // of its sample count), nor a user effect or another entry (PSFsSplitS).
            bool fp_split = false;
            if ((!fp_final || reroute) && wsa && we_st && !fp_ps && !fs_split && !dc_lean &&
                obj && wsee_ps_site(*obj) && g_res.chain_tex[write_idx] && g_res.dss_wsee_ssel) {
                D3D11_TEXTURE2D_DESC ctd = {};
                g_res.chain_tex[write_idx]->GetDesc(&ctd);
                fp_split = ctd.Width == g_res.wsee_e_w && ctd.Height == g_res.wsee_e_h &&
                           ctd.SampleDesc.Count == 1;
            }
            ctx->PSSetShaderResources(0, 1, &src_srv);
            if (fp_wsc) ctx->PSSetShaderResources(59, 1, &fc_wsc);   // WSEE: the OM is off the scene.
            // Gather at t3 for the resolve draw (bound after the OM above moved the RTV off buffer 2 - binding it
            // earlier would be a read-write hazard the runtime resolves by silently nulling the SRV); unbound right
            // after the draw.
            if (sg_pair) {
                ctx->PSSetShaderResources(3, 1, &sg_fin);
            }
            if (dlf_fin) ctx->PSSetShaderResources(3, 1, &dlf_fin);   // DLF: the fog, for the composite.
            GlowBind gl_b;   // GLOW_PYR: the pyramid at t3 and its sampler at s2, for this draw.
            if (on) gl_b.bind(ctx, g_res.glow_pyr[0]);
            AnaBind an_b;   // ANA_PYR: its atlases at t3 / t6, for this draw.
            if (pyr) an_b.bind(ctx, *pyr);
            if (side >= 0) ctx->PSSetShaderResources(4, 1, &g_res.fxs_srv[side]);   // FX_SIDE.
            // WSEE_EDGE_COST.
            if (fp_split) wsee_split_draw(ctx, g_res.chain_rtv[write_idx], g_res.wsee_es_dsv,
                                          g_res.constant_buffer, &cbd);   // WSEE_EPIX: its slice.
            else ctx->Draw(3, 0);   // FX_TEX: the pages bound at the top (ts) come off at the pass's end.
            if (fp_wsc) {   // WSEE: C off before any target can be the scene again.
                ID3D11ShaderResourceView* wsn = nullptr;
                ctx->PSSetShaderResources(59, 1, &wsn);
            }
            an_b.unbind();
            gl_b.unbind();
            if (fs_split || dc_lean || plean) {   // FS_SPLIT, KH_FX_LEAN, FX_LEAN_PASS.
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
            }
            if (side >= 0) {   // Unbound at once: the next side draw renders into it.
                ID3D11ShaderResourceView* n = nullptr;
                ctx->PSSetShaderResources(4, 1, &n);
            }
            if (sg_pair) {
                ID3D11ShaderResourceView* n2 = nullptr;
                ctx->PSSetShaderResources(3, 1, &n2);
                ctx->PSSetSamplers(2, 1, &s2old);   // FX_SAMP_S2: exact prior binding
                                                         // back.
                if (s2old) { s2old->Release(); s2old = nullptr; }
            }
            if (dlf_fin) {   // DLF: off t3 before the next pass's draws.
                ID3D11ShaderResourceView* n3 = nullptr;
                ctx->PSSetShaderResources(3, 1, &n3);
            }
            if (reroute) {   // WSEE: the last pass's result into the scene target (the final write).
                ID3D11ShaderResourceView* wcn = nullptr;
                ctx->PSSetShaderResources(0, 1, &wcn);
                om.restore(ctx);
                ctx->PSSetShaderResources(0, 1, &g_res.chain_srv[write_idx]);
                ctx->PSSetShader(g_res.ps_wsee_copy, nullptr, 0);
                ctx->Draw(3, 0);
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
            }

            if (!fp_final) {
                src_srv = g_res.chain_srv[write_idx];
                write_idx ^= 1;
            }
        };

        for (const auto& f : fullscreen) {
            if (f.second.effect <= 0) continue;
            // No-op skip (FX_SKIP_DOC): an opacity of 0.001 or less is dropped, not drawn, so a (near-)zero-opacity
            // pass costs nothing. At 0 it is an exact identity under the chain packing for every blend mode; up to
            // 0.001 at most a thousandth of its change (invisible but for an additive colour in the hundreds).
            if (f.second.color[3] <= 0.001f) continue;
            // LOCAL_CULL: a localized pass whose volume no pixel's point can reach - skipped as above, here
            // for the reason the next note gives.
            if (lc && local_culled(*lc, f.second)) continue;
            // DLF: no fresh light in the pool, or no intensity - the pass would add nothing, so it is skipped
            // whole as a zero-opacity pass is. Here, ahead of the pending pass's flush: a skip after it would leave
            // the chain's last pass unflushed as the final one, and its write-back into the scene lost. Only under a
            // blend its unarmed composite leaves as it was (normal, lighten, darken): additive, multiply and screen
            // blend the unchanged picture into itself, so there the pass runs and blends as it does armed.
            if (f.second.effect == static_cast<int>(EffectId::DynLightFog) &&
                (f.second.blend_mode == 0 || f.second.blend_mode == 4 || f.second.blend_mode == 5) &&
                (!(f.second.fx[0] > 0.0f) || !dlf_pool_live())) continue;
            // FX_SPLIT: SSGI without its gather / a-trous entry points (failed to build, reported) is skipped
            // here for the same reason - the flush's drop-whole gate would otherwise lose the chain's write-back
            // were it the last pass.
            if (f.second.effect == static_cast<int>(EffectId::Ssgi) &&
                (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) continue;

            // Custom effect: the pass draws with the user PS; absent or failed compiles skip the pass (reported once).
            ID3D11PixelShader* ufx = nullptr;
            bool c_upyr = false;   // FX_USER.

            if (f.second.effect == EFFECT_CUSTOM) {
                ufx = user_fx_ps(dev, fx_shader_of(f.second), &c_upyr);
                if (!ufx) continue;
            }

            ID3D11ShaderResourceView* c_lut = nullptr;

            if (f.second.effect == EFFECT_LUT) {
                c_lut = user_lut_srv(dev, fx_shader_of(f.second));
                if (!c_lut) continue;
            }

            if (live && !fp_ps && fuse_appendable(f.second) &&
                fuse_append(cbd, f.second, now_d)) continue;

            flush(false);
            if (!upload_cb(f.second, true, &cbd)) continue;
            if (f.second.effect == static_cast<int>(EffectId::Fogscatter))
                fogscatter_pack(cbd, fullscreen);
            else if (f.second.effect == EFFECT_CUSTOM)   // FX_FOGREC: a user effect's view of the fog passes.
                fog_records_pack(cbd.fx_fog[0], cbd.fx_fog[1], fullscreen);
            else if (f.second.effect == static_cast<int>(EffectId::DynLightFog))   // DLF_FOGREC: and the DLF's.
                dlf_fog_pack(cbd, fullscreen, &f.second);
            if (needs_depth(f.second)) {
                // The pair the engine encoded this cycle's depth with, read off its own upload when valid - the recipe
                // every stable depth reader here uses; the flush-selected pair is the fallback. FX_PAIR_HOLD: either
                // goes through the hold guard, whose verdict for this flush its frame made first (flush_locked,
                // FX_ENGINE_PAIR: the engine pair read, the cycle's camera latched) - the inverse and every reader
                // take one pair. The range lanes are this pass's own, put back after a hold. Effect MESHES keep the
                // flush-selected pair from upload_cb: KhFxBegin's depth gate tests their own fragments' view depth with
                // it (their far-frame arbitration takes a pair of its own, local0). One pair and one range per pass,
                // for the whole depth: a partitioned frame (PART_NEAR), whose depth holds more than one encode (the
                // flush's LATE_WORLD_PAIR note), has the pixels of the partition this pair did not encode decoded
                // with it too - not handled here.
                if (g_ro.engine_proj_valid) {
                    cbd.depth_params[0] = g_ro.engine_m22;
                    cbd.depth_params[1] = g_ro.engine_m32;
                }
                const float rz = cbd.depth_params[2], rw = cbd.depth_params[3];
                fx_depth_pair_guard(cbd, g_cc_flush_serial.load(std::memory_order_relaxed));
                cbd.depth_params[2] = rz;
                cbd.depth_params[3] = rw;
            }
            cbd.shadow_meta2[0] = nzv ? 1.0f : 0.0f;   // NEARZ_MARK: LoadDepthPS reads t37.
            // VM_SEE: 1 = the depth readers take the scene's depth behind the view model's translucent planes
            // (LoadDepthPS, t34); 2 = the builtin passes that put the world's medium or light into the picture also
            // fade by their transmittance (KhFxFinish, t41). A user effect is 1: it may be a screen filter, which the
            // fade would weaken under the planes - one that is a medium fades itself (KhVmSeeFade, the contract). Fused
            // stages ride the live pass's arm. Only with the world's range above 0 (depthParams.z): the view model's
            // slice lies below it, and the tests are against it.
            cbd.shadow_meta2[3] = 0.0f;
            if (vmsee && needs_depth(f.second) && cbd.depth_params[2] > 0.0f) {
                const int e = f.second.effect;
                const bool t = e == static_cast<int>(EffectId::Fog) ||
                               e == static_cast<int>(EffectId::Fogscatter) ||
                               e == static_cast<int>(EffectId::DynLightFog) ||
                               e == static_cast<int>(EffectId::Ssgi);
                cbd.shadow_meta2[3] = t ? 2.0f : 1.0f;
            }
            // WSEE: the pass's mode (wsee_mode_of) - 1 blends what the pass derives from the depth by the
            // transmittance between the depth behind the world's blended layers and the front one (cb.hlsl /
            // effect.hlsl's WSEE; the depth itself is never substituted, so the linear-depth copies' key is
            // unchanged) - only with the world's range above 0 and this cycle's capture valid; 0 otherwise, as on
            // every draw outside this loop. upload_cb's copy of the frame template zeroed the lane (and [1].w, set by
            // flush); the pass's side draws and linear-depth copies take it with cbd (fx_tex_fill leaves
            // both). A pass that would take a mode wants the capture whether or not this one is valid - with the
            // system built in (WSEE_ON false: no mode, no want).
            if constexpr (WSEE_ON) {
                if (needs_depth(f.second) && cbd.depth_params[2] > 0.0f) {
                    const int m = wsee_mode_of(f.second);
                    if (m > 0) {
                        wsee_want = true;
                        // WSEE_BWANT: a colour reader wants B / B2 from the next clear; in a cycle whose capture
                        // took none it is drawn unarmed (the picture of a frame before the record's first capture: the
                        // record's own want lag).
                        const bool cr = wsee_col_reader(f.second);
                        if (cr) col_want = true;
                        if (wsee && (col || !cr)) cbd.fx_tex[1][2] = static_cast<float>(m);
                    }
                }
            }
            effect = f.second.effect;
            fp_ps = ufx;
            fp_upyr = c_upyr;
            obj = &f.second;
            fp_lut = c_lut;
            live = true;
        }

        if (live) flush(true);
        // WSEE: the want, with the cycle this run draws (the published one: this may be the game thread's late
        // chain), once the capture's and the re-issues' shaders and states stand (wsee_ensure - its first call may
        // compile).
        if (wsee_want && wsee_ensure(dev)) {
            g_wsee.want_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed), std::memory_order_relaxed);
            if (col_want) {   // WSEE_BWANT: and the colours with it.
                g_wsee.col_want_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed),
                                            std::memory_order_relaxed);
            }
        }
        // Unconditional: a bail out of the final flush returns above the restore and would otherwise leave a chain RTV
        // bound for the rest of the frame.
        om.restore(ctx);
        om.release();
        if (saved) {   // FX_LINZ: t7 - t9 back as the run found them.
            ctx->PSSetShaderResources(7, 3, prev);
            for (ID3D11ShaderResourceView*& p : prev) KH_SAFE_RELEASE(p);
        }
    }
}

}