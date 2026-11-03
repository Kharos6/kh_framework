#pragma once

// The feature half (rendering_integration_features.hpp): the fullscreen post-processing chain (kh_fx_chain_run), its
// side passes (KH_FX_SIDE, KH_FX_LINZ) and the dynamicLightFog split stencil. Never include it on its own.
namespace RenderIntegration {

// KH_FX_SIDE - draw side value k (side id khsd_id: 28 fog scatter's per-pixel fog, 29 the sun flare's visibility,
// 30 the UI lane's coverage probe) with PSEffect (28: PSFsSide where made) into its own target, from the pass's own
// constants, before the pass: the glow build's contract - it uploads its own slice to khsd_cb (the route's object
// buffer, bound at b0), so the pass uploads its constants after it, and every other binding it makes is put back
// (KhGlowSave). The fog target is the frame's size and is drawn through the viewport the pass draws through; the
// two 1 x 1 targets through a 1 x 1 one. khsd_t0: the source bound at t0 for the draw (null keeps t0 as it is).
// False = no side value: the pass stays unarmed and computes the value in place.
// KH_WSEE_EDGE_COST: khsd_st - the run's E stencil where E's draw marked it (else null): fog scatter's value (k 0) is
// then drawn split by it (kh_wsee_split_draw) where its target is E's size; a size that differs draws once.
inline bool kh_fx_side_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx, int khsd_k, ConstantData& khsd_cbd,
                            float khsd_id, UINT khsd_w, UINT khsd_h, ID3D11ShaderResourceView* khsd_t0,
                            ID3D11Buffer* khsd_cb, ID3D11DepthStencilView* khsd_st = nullptr) {
    if (!dev || !ctx || !khsd_cb || khsd_k < 0 || khsd_k > 2) return false;
    if (khsd_w == 0 || khsd_h == 0 || khsd_w > 16384 || khsd_h > 16384) return false;
    if (!g_res.ps_effect || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!g_res.fxs_tex[khsd_k] || !g_res.fxs_rtv[khsd_k] || !g_res.fxs_srv[khsd_k] ||
        g_res.fxs_w[khsd_k] != khsd_w || g_res.fxs_h[khsd_k] != khsd_h) {
        KH_SAFE_RELEASE(g_res.fxs_srv[khsd_k]);
        KH_SAFE_RELEASE(g_res.fxs_rtv[khsd_k]);
        KH_SAFE_RELEASE(g_res.fxs_tex[khsd_k]);
        g_res.fxs_w[khsd_k] = g_res.fxs_h[khsd_k] = 0;
        D3D11_TEXTURE2D_DESC khsd_td = {};
        khsd_td.Width = khsd_w;
        khsd_td.Height = khsd_h;
        khsd_td.MipLevels = 1;
        khsd_td.ArraySize = 1;
        // The fog fraction (k 0, in [0, 1]) is stored at half precision: the armed pass reads it within 2^-12 of
        // the fp32 value the unarmed routes (effect meshes, the PIP) compute in place - far below an 8-bit step.
        // The two 1 x 1 values are exact (fp32).
        khsd_td.Format = khsd_k == 0 ? DXGI_FORMAT_R16_FLOAT : DXGI_FORMAT_R32_FLOAT;
        khsd_td.SampleDesc.Count = 1;
        khsd_td.Usage = D3D11_USAGE_DEFAULT;
        khsd_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&khsd_td, nullptr, &g_res.fxs_tex[khsd_k])) ||
            FAILED(dev->CreateRenderTargetView(g_res.fxs_tex[khsd_k], nullptr, &g_res.fxs_rtv[khsd_k])) ||
            FAILED(dev->CreateShaderResourceView(g_res.fxs_tex[khsd_k], nullptr, &g_res.fxs_srv[khsd_k]))) {
            KH_SAFE_RELEASE(g_res.fxs_srv[khsd_k]);
            KH_SAFE_RELEASE(g_res.fxs_rtv[khsd_k]);
            KH_SAFE_RELEASE(g_res.fxs_tex[khsd_k]);
            report_error_once_safe("KH RenderIntegration: effects: side target could not be made (the pass computes "
                                   "it in place)");
            return false;
        }
        g_res.fxs_w[khsd_k] = khsd_w;
        g_res.fxs_h[khsd_k] = khsd_h;
    }
    KhGlowSave khsd_sv;
    khsd_sv.capture(ctx);
    const float khsd_id0 = khsd_cbd.fx_meta[0];
    khsd_cbd.fx_meta[0] = khsd_id;
    const bool khsd_ok = kh_upload_obj_cb(ctx, khsd_cb, khsd_cbd);
    khsd_cbd.fx_meta[0] = khsd_id0;
    if (khsd_ok) {
        ctx->OMSetRenderTargets(1, &g_res.fxs_rtv[khsd_k], nullptr);
        if (khsd_k != 0) {
            D3D11_VIEWPORT khsd_vp = {};
            khsd_vp.Width = 1.0f;
            khsd_vp.Height = 1.0f;
            khsd_vp.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &khsd_vp);
        }
        const FLOAT khsd_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        // KH_WSEE_EDGE_COST: fog scatter's value (k 0) with its own entry where made (PSFsSide: PSEffect's branch 28,
        // bit for bit, at its own register count); every other side value, and k 0 without it, with PSEffect.
        ctx->PSSetShader((khsd_k == 0 && khsd_id == 28.0f && g_res.ps_fs_side) ? g_res.ps_fs_side : g_res.ps_effect,
                         nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, khsd_bf, 0xFFFFFFFF);
        if (khsd_t0) ctx->PSSetShaderResources(0, 1, &khsd_t0);
        const bool khsd_sp = khsd_st && khsd_k == 0 && g_res.dss_wsee_ssel;   // KH_WSEE_EDGE_COST.
        if (khsd_sp && khsd_w == g_res.wsee_e_w && khsd_h == g_res.wsee_e_h) {
            ConstantData khsd_up = khsd_cbd;   // KH_WSEE_EPIX: the slice uploaded above.
            khsd_up.fx_meta[0] = khsd_id;
            kh_wsee_split_draw(ctx, g_res.fxs_rtv[khsd_k], khsd_st, khsd_cb, &khsd_up);
        } else {
            ctx->Draw(3, 0);
        }
    }
    khsd_sv.restore(ctx);
    return khsd_ok;
}

// KH_FX_LINZ - draw linear-depth target k (effect.hlsl's note: 0 the frame, fxMeta.x 34; 1 the SSGI grid's copy, 35;
// 2 dynamicLightFog's, 36) with PSFxLinZ, khlz_w x khlz_h, one texel per pixel, from khlz_cbd - the reading pass's
// own constants: its depth lanes, and for a grid its mapping in local0 and its matCtl.x arm, so the copy reads the
// frame's. kh_fx_side_draw's contract: it uploads its own slice to khlz_cb (the pass uploads after it) and every
// other binding it makes is put back (KhGlowSave); the target's own slot (t7 + k) is nulled first and left null - the
// caller binds the result. The target is made at first use and remade on a size change. False = none drawn: the
// readers stay unarmed and convert in place.
inline bool kh_fx_linz_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx, int khlz_k, ConstantData& khlz_cbd,
                            UINT khlz_w, UINT khlz_h, ID3D11Buffer* khlz_cb) {
    if (!dev || !ctx || !khlz_cb || khlz_k < 0 || khlz_k > 2) return false;
    if (khlz_w == 0 || khlz_h == 0 || khlz_w > 16384 || khlz_h > 16384) return false;
    if (!g_res.ps_fx_linz || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    ID3D11ShaderResourceView* khlz_null = nullptr;
    ctx->PSSetShaderResources(7 + static_cast<UINT>(khlz_k), 1, &khlz_null);   // Off its slot before it is a target.
    if (!g_res.linz_tex[khlz_k] || !g_res.linz_rtv[khlz_k] || !g_res.linz_srv[khlz_k] ||
        g_res.linz_w[khlz_k] != khlz_w || g_res.linz_h[khlz_k] != khlz_h) {
        KH_SAFE_RELEASE(g_res.linz_srv[khlz_k]);
        KH_SAFE_RELEASE(g_res.linz_rtv[khlz_k]);
        KH_SAFE_RELEASE(g_res.linz_tex[khlz_k]);
        g_res.linz_w[khlz_k] = g_res.linz_h[khlz_k] = 0;
        D3D11_TEXTURE2D_DESC khlz_td = {};
        khlz_td.Width = khlz_w;
        khlz_td.Height = khlz_h;
        khlz_td.MipLevels = 1;
        khlz_td.ArraySize = 1;
        khlz_td.Format = DXGI_FORMAT_R32_FLOAT;
        khlz_td.SampleDesc.Count = 1;
        khlz_td.Usage = D3D11_USAGE_DEFAULT;
        khlz_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&khlz_td, nullptr, &g_res.linz_tex[khlz_k])) ||
            FAILED(dev->CreateRenderTargetView(g_res.linz_tex[khlz_k], nullptr, &g_res.linz_rtv[khlz_k])) ||
            FAILED(dev->CreateShaderResourceView(g_res.linz_tex[khlz_k], nullptr, &g_res.linz_srv[khlz_k]))) {
            KH_SAFE_RELEASE(g_res.linz_srv[khlz_k]);
            KH_SAFE_RELEASE(g_res.linz_rtv[khlz_k]);
            KH_SAFE_RELEASE(g_res.linz_tex[khlz_k]);
            report_error_once_safe("KH RenderIntegration: effects: linear-depth target could not be made (the depth "
                                   "is read in place)");
            return false;
        }
        g_res.linz_w[khlz_k] = khlz_w;
        g_res.linz_h[khlz_k] = khlz_h;
    }
    KhGlowSave khlz_sv;
    khlz_sv.capture(ctx);
    const float khlz_id0 = khlz_cbd.fx_meta[0];
    khlz_cbd.fx_meta[0] = 34.0f + static_cast<float>(khlz_k);
    const bool khlz_ok = kh_upload_obj_cb(ctx, khlz_cb, khlz_cbd);
    khlz_cbd.fx_meta[0] = khlz_id0;
    if (khlz_ok) {
        ctx->OMSetRenderTargets(1, &g_res.linz_rtv[khlz_k], nullptr);
        D3D11_VIEWPORT khlz_vp = {};
        khlz_vp.Width = static_cast<float>(khlz_w);
        khlz_vp.Height = static_cast<float>(khlz_h);
        khlz_vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &khlz_vp);
        const FLOAT khlz_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_fx_linz, nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, khlz_bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
    }
    khlz_sv.restore(ctx);
    return khlz_ok;
}

// KH_DLF_SPLIT - the stencil dynamicLightFog's gather draws under (the note at Resources::ps_dlf_gather_lean):
// D24_UNORM_S8_UINT at khds_t's size (the grid target; a render target and a depth-stencil view bound together must
// agree in size), made at first use and remade on a size change, with the two states (no depth; ALWAYS / REPLACE to
// mark, EQUAL / KEEP with no write to select), made once with the device. Null = no split this time: either entry
// absent, a multisampled target, or a refused make (reported once; latched at that size, another size tries again).
// The chain's thread.
inline ID3D11DepthStencilView* kh_dlf_split_ensure(ID3D11Device* dev, ID3D11Texture2D* khds_t) {
    if (!dev || !khds_t || !g_res.ps_dlf_gather_lean || !g_res.ps_dlf_class) return nullptr;
    D3D11_TEXTURE2D_DESC khds_td = {};
    khds_t->GetDesc(&khds_td);
    if (khds_td.SampleDesc.Count != 1 || khds_td.Width == 0 || khds_td.Height == 0) return nullptr;
    if (g_res.dlf_st_dsv && g_res.dss_dlf_mark && g_res.dss_dlf_sel && g_res.dlf_st_w == khds_td.Width &&
        g_res.dlf_st_h == khds_td.Height) {
        return g_res.dlf_st_dsv;
    }
    if (g_res.dlf_st_fail_w == khds_td.Width && g_res.dlf_st_fail_h == khds_td.Height) return nullptr;
    KH_SAFE_RELEASE(g_res.dlf_st_dsv);
    KH_SAFE_RELEASE(g_res.dlf_st_tex);
    g_res.dlf_st_w = g_res.dlf_st_h = 0;
    D3D11_TEXTURE2D_DESC khds_sd = {};
    khds_sd.Width = khds_td.Width;
    khds_sd.Height = khds_td.Height;
    khds_sd.MipLevels = 1;
    khds_sd.ArraySize = 1;
    khds_sd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    khds_sd.SampleDesc.Count = 1;
    khds_sd.Usage = D3D11_USAGE_DEFAULT;
    khds_sd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    D3D11_DEPTH_STENCIL_DESC khds_dd = {};
    khds_dd.DepthEnable = FALSE;
    khds_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    khds_dd.DepthFunc = D3D11_COMPARISON_ALWAYS;
    khds_dd.StencilEnable = TRUE;
    khds_dd.StencilReadMask = 0xFF;
    khds_dd.StencilWriteMask = 0xFF;
    khds_dd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    khds_dd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    khds_dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    khds_dd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    khds_dd.BackFace = khds_dd.FrontFace;
    bool khds_ok = SUCCEEDED(dev->CreateTexture2D(&khds_sd, nullptr, &g_res.dlf_st_tex)) &&
                   SUCCEEDED(dev->CreateDepthStencilView(g_res.dlf_st_tex, nullptr, &g_res.dlf_st_dsv));
    if (khds_ok && !g_res.dss_dlf_mark) {
        khds_ok = SUCCEEDED(dev->CreateDepthStencilState(&khds_dd, &g_res.dss_dlf_mark));
    }
    if (khds_ok && !g_res.dss_dlf_sel) {
        khds_dd.StencilWriteMask = 0x00;
        khds_dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        khds_dd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        khds_dd.BackFace = khds_dd.FrontFace;
        khds_ok = SUCCEEDED(dev->CreateDepthStencilState(&khds_dd, &g_res.dss_dlf_sel));
    }
    if (!khds_ok) {
        KH_SAFE_RELEASE(g_res.dlf_st_dsv);
        KH_SAFE_RELEASE(g_res.dlf_st_tex);
        g_res.dlf_st_fail_w = khds_td.Width;
        g_res.dlf_st_fail_h = khds_td.Height;
        report_error_once_safe("KH RenderIntegration: dynamicLightFog: the gather's stencil could not be made (the "
                               "full gather draws alone)");
        return nullptr;
    }
    g_res.dlf_st_w = khds_td.Width;
    g_res.dlf_st_h = khds_td.Height;
    return g_res.dlf_st_dsv;
}

// Retained-mode flush: geometry then post-processing, with the scene captured at most once per frame. flush_frame runs
// once per frame from the C++ Draw3D EH (game thread); the flush body runs on the render thread at its scene resolve,
// or under a park (KH_NO_PARK); KH_FX_LATE's chain-only run may be the game thread's, inside a resolve the engine
// issues there. Fully dormant when there is nothing to do.

// The per-frame work, run from flush_locked. KH_RENDER_FLUSH: the fullscreen chain (the ping-pong through chain_tex,
// the SSGI seed / gather / resolve, the LUT and user-shader passes, the fusion, the final write-back into the saved
// OM). What the flush supplies from its locals arrives as parameters: the depth SRV it had at t1 (null when the depth
// is not ready), whether a scene capture is still owed (when meshes drew, or the flush's setup left its copy here -
// KH_CAP_DEFER), its clock (KH_FUSE_CLOCK), and its two lambdas - the per-pass constant-buffer fill (upload_cb,
// chain_pass = true, deferred) and the depth-demand predicate - as template parameters, so the call sites inside are
// the flush's own text. The caller performs the read-only DSV swap before calling and keeps its OM save; this
// function's own KhOmSave is what the final pass restores into. flush_locked is the one caller: its scene flush and its
// chain-only run (KH_FX_LATE) alike.
template <class KhFxUploadCb, class KhFxNeedsDepth>
inline void kh_fx_chain_run(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                            std::vector<std::pair<uint64_t, RenderObject>>& fullscreen,
                            const FLOAT bf[4], ID3D11ShaderResourceView* khfc_depth_srv,
                            bool khfc_capture, KhFxUploadCb&& upload_cb, KhFxNeedsDepth&& needs_depth,
                            double khfc_now_d,   // KH_FUSE_CLOCK: the flush's clock (upload_cb's now_d).
                            const KhLocalCull* khfc_lc = nullptr) {   // KH_LOCAL_CULL: null = none culled.
    // The chain's depth must be AT t1: the mesh loop leaves t1 as the last mesh needed it, and a localized / banded /
    // depth-reading pass reading 0 there reconstructs the camera position everywhere. Null when the depth is not ready,
    // as ps_srvs already says.
    ctx->PSSetShaderResources(1, 1, &khfc_depth_srv);
    // KH_NEARZ_MARK: this cycle's marker at t37 (inside StateBackup's range), or none; read once, so the bind and
    // every pass's arm (shadowMeta2.x, below) take one verdict (KH_EA_NZM_XT).
    const bool khfc_nzv = kh_nzm_valid();
    {
        ID3D11ShaderResourceView* khfc_nzm = khfc_nzv ? g_res.nzm_srv : nullptr;
        ctx->PSSetShaderResources(37, 1, &khfc_nzm);
    }
    // KH_VM_SEE: this cycle's snapshot (t34) and transmittance (t41), or none - inside the flush's StateBackup range,
    // so handed back with it; read only where a pass is armed (shadowMeta2.w, below).
    const bool khfc_vmsee = kh_vmsee_valid();
    {
        ID3D11ShaderResourceView* khfc_vss = khfc_vmsee ? g_res.vmsee_s_srv : nullptr;
        ID3D11ShaderResourceView* khfc_vst = khfc_vmsee ? g_res.vmsee_t_srv : nullptr;
        ctx->PSSetShaderResources(34, 1, &khfc_vss);
        ctx->PSSetShaderResources(41, 1, &khfc_vst);
    }
    std::string chain_err = khfc_capture ? kh_scene_capture_timed(dev, ctx) : "";
    if (chain_err.empty()) chain_err = ensure_fx_chain(dev);

    if (!chain_err.empty()) {
        // The mesh-side twin of this pair reports as "effect setup"; this is the chain's own report.
        kh_ensure_ok("fullscreen chain", chain_err);
    } else {
        KhOmSave khfp_om;
        khfp_om.capture(ctx);
        // KH_WSEE: this cycle's record at t53 - t69 (the KH_WSEE note's REGISTERS; t56 is the chain's input capture,
        // past this run's capture above, which may remake it), or none: inside the flush's StateBackup range
        // (t0..t69), so handed back with it; read only where a pass is armed (fx_tex[1].z, below). t58 (the run's
        // agreement mask E) is bound once drawn, at the run's first sited pass, and t59 (the scene's sample colours C,
        // khfc_wsc) around a C reader's own draw alone (khfp_flush: the hazard rule there) - null until then;
        // fx_tex[1].w tells the pass which stand. Built out (KH_WSEE_ON false): none of it (kh_wsee_valid), no want.
        const bool khfc_wsee = kh_wsee_valid() && g_res.scene_srv != nullptr;
        bool khfc_wsee_want = false;   // Some pass took a mode (valid or not): the want is stamped after the loop.
        ID3D11ShaderResourceView* khfc_wsc = nullptr;   // C (kh_wsee_scene_ms; held by g_res), or none.
        bool khfc_we_tried = false;   // E attempted (the first sited pass), and drawn.
        bool khfc_we_ok = false;
        bool khfc_we_st = false;      // KH_WSEE_EDGE_COST: E's stencil marked with it (the split draws may use it).
        bool khfc_b2 = false;         // This cycle's B2 moment stands (t60 with a colour want, and S2 t62; bit 3).
        bool khfc_col = false;        // KH_WSEE_BWANT: this cycle's capture resolved B (and B2 with it, where taken).
        bool khfc_col_want = false;   // KH_WSEE_BWANT: an armed colour reader ran (the want, stamped after the loop).
        bool khfc_ld = false;         // This cycle's layer-depth split bound (Tp t63, Tf t64; bit 4).
        bool khfc_od = false;         // This cycle's D3 bound (t65; bit 5).
        bool khfc_pb = false;         // KH_PSB: this cycle's one-pass record bound (t66 - t69 - PHASE; bit 6).
        bool khfc_pe = false;         // KH_PSB: the old record holds no particle-type layer this cycle (bit 7).
        bool khfc_np = false;         // KH_WSEE_NOPRE: a pre late opaque skipped this cycle (bit 8).
        bool khfc_s2 = false;         // KH_WSEE_NOPRE: this cycle's S2 bound (t62, B2 or not; bit 9).
        {
            ID3D11ShaderResourceView* khfc_ws[13] = {};
            if (khfc_wsee) {
                khfc_ws[0] = g_res.wsee_s_srv;
                khfc_ws[1] = g_res.wsee_t_srv;
                // KH_WSEE_BWANT: B (t55) and B2 (t60) only where this cycle's capture resolved them (no colour reader
                // wanted them: not written this cycle - null, and a colour reader runs unarmed below).
                khfc_col = g_wsee_col_cycle.load(std::memory_order_relaxed) ==
                           g_topo_cycles_pub.load(std::memory_order_relaxed);
                khfc_ws[2] = khfc_col ? g_res.wsee_b_srv : nullptr;
                khfc_ws[3] = g_res.scene_srv;
                khfc_ws[4] = g_res.wsee_n_srv;
                // B2 (t60) where this cycle took it (bit 3 of fx_tex[1].w says so), NWpost (t61).
                khfc_b2 = g_res.wsee_b2_srv != nullptr && g_res.wsee_s2_srv != nullptr &&
                          g_wsee_b2_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                khfc_ws[7] = (khfc_b2 && khfc_col) ? g_res.wsee_b2_srv : nullptr;   // Bit 3 stays B2's moment / S2.
                khfc_ws[8] = g_res.wsee_n2_srv;
                // KH_WSEE_NOPRE: S2 wherever this cycle took it (the derivation's reference, bit 9 - B2 or not); bit 8
                // where it skipped a pre late opaque's replay (cb.hlsl's note at khWsS2).
                khfc_s2 = g_res.wsee_s2_srv != nullptr && g_wsee_s2_cycle.load(std::memory_order_relaxed) ==
                                                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                khfc_np = g_wsee_nopre_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                khfc_ws[9] = (khfc_b2 || khfc_s2) ? g_res.wsee_s2_srv : nullptr;   // S2 (t62), B2's twin.
                // Tp (t63) and Tf (t64) where this cycle's capture cleared them; bit 4 of fx_tex[1].w (KhWseeAvail)
                // says so.
                khfc_ld = g_res.wsee_tp_srv != nullptr && g_res.wsee_tf_srv != nullptr &&
                          g_wsee_ld_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                khfc_ws[10] = khfc_ld ? g_res.wsee_tp_srv : nullptr;
                khfc_ws[11] = khfc_ld ? g_res.wsee_tf_srv : nullptr;
                // D3 (t65) where this cycle's capture stood it up; bit 5 of fx_tex[1].w (KhWseeAvail) says so.
                khfc_od = g_res.wsee_d3_srv != nullptr &&
                          g_wsee_od_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                // KH_WSEE_OQ: and complete - a skipped pre opaque with no S2 taken after it leaves it unbound
                // (kh_wsee_d3_ok's note: the live depth is the behind then).
                if (khfc_od && !kh_wsee_d3_ok(g_topo_cycles_pub.load(std::memory_order_relaxed))) khfc_od = false;
                // KH_WSEE_OQ2: and the post opaques' D3m resolved into it first (kh_wsee_d3m_resolve; not drawable -
                // unbound). Before the binds below: the pass restores this run's state.
                if (khfc_od && !kh_wsee_d3m_resolve(ctx, g_topo_cycles_pub.load(std::memory_order_relaxed))) {
                    khfc_od = false;
                }
                khfc_ws[12] = khfc_od ? g_res.wsee_d3_srv : nullptr;
                // KH_PSB: P0, P1, N, N2 (t66 - t69) where a re-issue drew into them this cycle (bit 6 of fx_tex[1].w);
                // bit 7 where no particle-type layer went into the old record (its NW / Tp / Tf hold their clears).
                khfc_pb = g_res.wsee_p0_srv != nullptr && g_res.wsee_p1_srv != nullptr &&
                          g_res.wsee_nb_srv != nullptr && g_res.wsee_nb2_srv != nullptr &&   // KH_PSB_PHASE.
                          g_psb_drew_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                // Bit 7 only while the one pass's record stands (else the old record is read whole).
                khfc_pe = g_psb_cycle.load(std::memory_order_relaxed) ==
                              g_topo_cycles_pub.load(std::memory_order_relaxed) &&
                          g_psa_cycle.load(std::memory_order_relaxed) !=
                              g_topo_cycles_pub.load(std::memory_order_relaxed);
                g_wsee_used_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed), std::memory_order_relaxed);
                // The target bound at the run's start is the scene target the capture above resolved, and the one
                // the final pass writes (khfp_om).
                khfc_wsc = kh_wsee_scene_ms(dev, khfp_om.rtv[0]);
                // KH_WSEE_BIND_ARMED: the two binds only with the record valid - without it every pass's [1].z is 0
                // and no shader loads t53 - t69 (every load is under KhWseeArmed / KhWseeMeshMode), nothing of ours
                // stands there (the sort scope nulls its own at its end) and the flush's StateBackup puts the
                // engine's views back either way: two calls fewer per unarmed run, the 0 build's every run.
                ctx->PSSetShaderResources(53, 13, khfc_ws);   // t58 / t59 null: bound later, or never.
                ID3D11ShaderResourceView* const khfc_pbv[4] = { khfc_pb ? g_res.wsee_p0_srv : nullptr,
                                                                khfc_pb ? g_res.wsee_p1_srv : nullptr,
                                                                khfc_pb ? g_res.wsee_nb_srv : nullptr,
                                                                khfc_pb ? g_res.wsee_nb2_srv : nullptr };   // PHASE.
                ctx->PSSetShaderResources(66, 4, khfc_pbv);   // KH_PSB (StateBackup covers t0 - t69; PHASE: N2).
            }
        }
        ctx->IASetInputLayout(nullptr);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);   // Opaque: compositing is in-shader.

        ID3D11ShaderResourceView* src_srv = g_res.scene_srv;
        int write_idx = 0;

        bool                      khfp_live = false;
        ConstantData              khfp_cbd;
        int                       khfp_effect = 0;
        ID3D11PixelShader*        khfp_ps = nullptr;
        bool                      khfp_upyr = false;   // KH_FX_USER: the user pass reads the pyramid.
        const RenderObject*       khfp_obj = nullptr;   // KH_FX_TEX: the pass's object (its textures).
        ID3D11ShaderResourceView* khfp_lut = nullptr;
        // KH_FX_LINZ (effect.hlsl's note): this run's linear-depth targets. The frame's is drawn at the run's first
        // pass set to read it (khlz_frame: fog scatter at its flush, SSGI past its drop-whole gate, dynamicLightFog
        // once its lights are up), from its constants; its key is that pass's depth lanes (the depth pair, the viewport
        // range, the marker arm, KH_VM_SEE's substitution) and frame size, and a pass is armed (mat_ctl x = 1) only
        // when its own are the key - t1, t37 and t34, the conversion's textures, are the run's own (bound above). A
        // grid's copy is drawn for an armed pass by its block, with its own mapping (the SSGI factor; the DLF grid),
        // and arms mat_ctl w. t7 - t9 are taken at the first draw and given back at the run's end.
        bool                      khlz_saved = false;
        ID3D11ShaderResourceView* khlz_prev[3] = {};
        bool                      khlz_tried = false;
        bool                      khlz_ok = false;
        float                     khlz_key[8] = {};
        bool                      khlz_gs_tried = false;
        bool                      khlz_gs_ok = false;
        float                     khlz_gs_inv = 0.0f;
        bool                      khlz_gd_tried = false;
        bool                      khlz_gd_ok = false;
        ID3D11ShaderResourceView* khlz_none = nullptr;   // t8 / t9 for an unarmed pass.
        auto khlz_save = [&]() {
            if (khlz_saved) return;
            ctx->PSGetShaderResources(7, 3, khlz_prev);
            khlz_saved = true;
        };
        auto khlz_keyof = [](const ConstantData& khlz_c, float khlz_out[8]) {
            khlz_out[0] = khlz_c.depth_params[0];
            khlz_out[1] = khlz_c.depth_params[1];
            khlz_out[2] = khlz_c.depth_params[2];
            khlz_out[3] = khlz_c.depth_params[3];
            khlz_out[4] = khlz_c.shadow_meta2[0];
            khlz_out[5] = khlz_c.fx_meta[2];
            khlz_out[6] = khlz_c.fx_meta[3];
            khlz_out[7] = khlz_c.shadow_meta2[3] > 0.5f ? 1.0f : 0.0f;   // KH_VM_SEE: substitutes (1 and 2 alike).
        };
        // The frame's linear depth for a builtin SSGI / dynamicLightFog / fog-scatter pass that will read it - drawn
        // at the run's first such call, from that pass's constants; then this pass armed when its lanes are the key
        // (any other converts in place).
        auto khlz_frame = [&](ConstantData& khlz_pc) {
            float khlz_k[8];
            khlz_keyof(khlz_pc, khlz_k);
            if (!khlz_tried) {
                khlz_tried = true;
                const bool khlz_in = khlz_k[5] >= 1.0f && khlz_k[5] <= 16384.0f &&
                                     khlz_k[6] >= 1.0f && khlz_k[6] <= 16384.0f;
                const UINT khlz_w = khlz_in ? static_cast<UINT>(khlz_k[5]) : 0u;
                const UINT khlz_h = khlz_in ? static_cast<UINT>(khlz_k[6]) : 0u;
                if (khlz_w > 0 && khlz_h > 0 && static_cast<float>(khlz_w) == khlz_k[5] &&
                    static_cast<float>(khlz_h) == khlz_k[6]) {   // A whole frame: the shader's fxMeta.zw bound.
                    khlz_save();
                    ConstantData khlz_c = khlz_pc;
                    khlz_ok = kh_fx_linz_draw(dev, ctx, 0, khlz_c, khlz_w, khlz_h, g_res.constant_buffer);
                    if (khlz_ok) {
                        memcpy(khlz_key, khlz_k, sizeof(khlz_key));
                        ctx->PSSetShaderResources(7, 1, &g_res.linz_srv[0]);
                    }
                }
            }
            if (khlz_ok && memcmp(khlz_key, khlz_k, sizeof(khlz_key)) == 0) khlz_pc.mat_ctl[0] = 1.0f;
        };

        // KH_FX_FUSE: a return below past this point (the SSGI block's missing target, sampler or entry point; a
        // failed constant upload) leaves the pass undrawn with every stage fused onto it (kh_fuse_append), and as the
        // chain's final pass its write into the scene too - the scene then keeps the picture the run found, without
        // any of the chain's passes, for that frame (the passes before it drew into a ping-pong buffer nothing reads
        // after; KH_FX_BAIL_DOC). The loop skips the known causes before it fuses (SSGI without its entry points,
        // dynamicLightFog with nothing to add); the rest ensure_fx_chain makes all-or-nothing, so none is met in
        // practice. No fall-through copy: the bail sites leave the OM, viewport and s2 in differing states, and the
        // only copy shader (ps_wsee_copy) stands with KH_WSEE alone.
        auto khfp_flush = [&](bool khfp_final) {
            if (!khfp_live) return;
            khfp_live = false;
            // KH_GPU_PASS: the pass whole - its side draws, pyramids and linear-depth builds included.
            KhGpuPassScope khgq_pass(ctx, 0u, khfp_effect, khfp_ps != nullptr,
                                     static_cast<uint32_t>(khfp_cbd.fuse_meta[0] + 0.5f));
            // KH_FX_TEX: the pass's own textures - a user effect's, or (KH_FX_TEX_ALL) a builtin pass's that names
            // some. The lanes go into khfp_cbd before any side draw below takes it or a copy, and the pages stay
            // bound from here to the pass's end (khfp_ts unbinds on every exit), so the side draws and the pass read
            // one set; nothing else in the chain binds t43 - t48 or s0.
            ID3D11ShaderResourceView* khfp_tex[KH_MAT_USER_MAPS] = {};
            KhFxTexScope khfp_ts;
            if (khfp_obj && kh_fx_tex_on(*khfp_obj)) {
                kh_fx_tex_fill(dev, ctx, khfp_cbd, *khfp_obj, khfp_tex);
                khfp_ts.bind(ctx, khfp_tex);
            }
            // KH_WSEE: an armed pass (its mode: the loop below) - the run's agreement mask E at the first sited one,
            // from its constants and before its side draws (they read t58 too; the frame's size, KH_FX_LINZ's rule),
            // then bit 0 of fx_tex[1].w on every armed pass while E stands; bit 1 is the pass's own draw's (below),
            // never its side draws'; bit 2 on every armed pass (both E and the inline per-sample test need it).
            const bool khfp_wsa = khfp_cbd.fx_tex[1][2] > 0.5f;
            // KH_WSEE_EDGE_COST: E at the run's first armed pass with a per-sample site (kh_wsee_ps_site), not
            // the first armed one - a pass without a site reads neither E nor bit 0, so the value every reader gets
            // is the same (E's inputs, t1 and t54, are the run's); a run without a site draws no E.
            if (khfp_wsa && !khfc_we_tried && khfp_obj && kh_wsee_ps_site(*khfp_obj)) {
                khfc_we_tried = true;
                const float khwe_fw = khfp_cbd.fx_meta[2], khwe_fh = khfp_cbd.fx_meta[3];
                const bool khwe_in = khwe_fw >= 1.0f && khwe_fw <= 16384.0f && khwe_fh >= 1.0f && khwe_fh <= 16384.0f;
                const UINT khwe_w = khwe_in ? static_cast<UINT>(khwe_fw) : 0u;
                const UINT khwe_h = khwe_in ? static_cast<UINT>(khwe_fh) : 0u;
                if (khwe_w > 0 && khwe_h > 0 && static_cast<float>(khwe_w) == khwe_fw &&
                    static_cast<float>(khwe_h) == khwe_fh) {
                    khfc_we_ok = kh_wsee_agree_draw(dev, ctx, khfp_cbd, khwe_w, khwe_h, g_res.constant_buffer,
                                                    khfc_we_st);
                }
                if (khfc_we_ok) ctx->PSSetShaderResources(58, 1, &g_res.wsee_e_srv);
            }
            khfp_cbd.fx_tex[1][3] = (khfp_wsa && khfc_we_ok) ? 1.0f : 0.0f;
            if (khfp_wsa) khfp_cbd.fx_tex[1][3] += 4.0f;   // Bit 2, per sample allowed.
            if (khfp_wsa && khfc_b2) khfp_cbd.fx_tex[1][3] += 8.0f;   // Bit 3, B2's moment (t60; S2, t62).
            if (khfp_wsa && khfc_ld) khfp_cbd.fx_tex[1][3] += 16.0f;   // Bit 4, Tp / Tf (t63 / t64).
            if (khfp_wsa && khfc_od) khfp_cbd.fx_tex[1][3] += 32.0f;   // Bit 5, D3 (t65).
            if (khfp_wsa && khfc_pb) khfp_cbd.fx_tex[1][3] += 64.0f;   // KH_PSB: bit 6, P0 / P1 / N / N2 (t66 - t69).
            if (khfp_wsa && khfc_pe) khfp_cbd.fx_tex[1][3] += 128.0f;   // KH_PSB: bit 7, the old record unread.
            if (khfp_wsa && khfc_np) khfp_cbd.fx_tex[1][3] += 256.0f;   // KH_WSEE_NOPRE: bit 8, pre derived;
            if (khfp_wsa && khfc_s2) khfp_cbd.fx_tex[1][3] += 512.0f;   // bit 9, S2 bound (t62).
            // KH_FX_LINZ: fog scatter reads the depth whenever it runs (its side draw below, then the pass).
            if (!khfp_ps && khfp_effect == static_cast<int>(EffectId::Fogscatter)) khlz_frame(khfp_cbd);
            bool khsg_pair = false;
            // The resolve reads whichever half-res buffer holds the final smoothed field (three a-trous iterations,
            // steps 1 / 2 / 4, ping-pong [2]->[3]->[2]->[3]; a failed iteration leaves the pointer at the last good
            // stage).
            ID3D11ShaderResourceView* khsg_fin = nullptr;
            ID3D11SamplerState* khsg_s2old = nullptr;   // KH_FX_SAMP_S2: the saved s2.
            if (khfp_effect == static_cast<int>(EffectId::Ssgi) && !khfp_ps) {
                if (!g_res.chain_srv[2] || !g_res.chain_srv[4] ||
                    !g_res.chain_rtv[4] || !g_res.khsg_sampler ||
                    !g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous) return;   // The seed and the gather's and
                                                                               // a-trous' entry points (KH_FX_SPLIT)
                                                                               // join the drop-whole gate.
                ctx->PSGetSamplers(2, 1, &khsg_s2old);   // KH_FX_SAMP_S2: slot 2.
                const float khsg_l0sv = khfp_cbd.local0[0];
                const float khsg_l1sv = khfp_cbd.local0[1];
                const float khsg_l2sv = khfp_cbd.local0[2];
                const float khsg_l3sv = khfp_cbd.local0[3];
                khfp_cbd.local0[1] = (float)g_res.scene_w / (float)(g_khsg_w > 0 ? g_khsg_w : 1);
                // KH_SSGI_CENTRE: the grid's own size for the side draws (seed, gather, a-trous, its depth copy) -
                // each axis's factor and the a-trous' bound (effect.hlsl KhSsgiGrid); put back with local0.xy.
                khfp_cbd.local0[2] = (float)(g_khsg_w > 0 ? g_khsg_w : 1);
                khfp_cbd.local0[3] = (float)(g_khsg_h > 0 ? g_khsg_h : 1);
                // KH_FX_LINZ: the frame's (past the drop-whole gate: the pass reads it from here on), then the grid's
                // copy at this factor, for the a-trous (KhSsgiGridZ) and the resolve (KH_SSGI_JBU, effect 24's Gather)
                // - an armed pass's, drawn once a run per factor; mat_ctl w = 1 arms it. t8 holds this pass's copy, or
                // nothing when unarmed (the helper's size query and load meet no other route's view).
                khlz_frame(khfp_cbd);
                if (khfp_cbd.mat_ctl[0] > 0.5f && g_khsg_w > 0 && g_khsg_h > 0) {
                    if (!khlz_gs_tried || khlz_gs_inv != khfp_cbd.local0[1]) {
                        khlz_gs_tried = true;
                        khlz_save();
                        ConstantData khlz_c = khfp_cbd;
                        khlz_gs_ok = kh_fx_linz_draw(dev, ctx, 1, khlz_c, static_cast<UINT>(g_khsg_w),
                                                     static_cast<UINT>(g_khsg_h), g_res.constant_buffer);
                        khlz_gs_inv = khfp_cbd.local0[1];
                    }
                    if (khlz_gs_ok) khfp_cbd.mat_ctl[3] = 1.0f;
                }
                khlz_save();
                ctx->PSSetShaderResources(8, 1, khfp_cbd.mat_ctl[3] > 0.5f ? &g_res.linz_srv[1] : &khlz_none);
                ctx->PSSetSamplers(2, 1, &g_res.khsg_sampler);
                // Every exit puts s2 back and releases the saved ref: the three failed uploads below, the pass's own
                // failed upload (KH_FX_SSGI_BAIL) and the pass's end. s1 itself is untouched at each.
                if (!kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                    ctx->PSSetSamplers(2, 1, &khsg_s2old);
                    if (khsg_s2old) khsg_s2old->Release();
                    return;
                }
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
                ID3D11ShaderResourceView* khsg_null = nullptr;
                ctx->PSSetShaderResources(0, 1, &khsg_null);
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                // The gather draws on the half viewport; the saved one is restored before the resolve body.
                D3D11_VIEWPORT khsg_vps; UINT khsg_vpn = 1;
                ctx->RSGetViewports(&khsg_vpn, &khsg_vps);
                D3D11_VIEWPORT khsg_vph = {};
                khsg_vph.Width    = (float)(g_khsg_w > 0 ? g_khsg_w : 1);
                khsg_vph.Height   = (float)(g_khsg_h > 0 ? g_khsg_h : 1);
                khsg_vph.MaxDepth = 1.0f;
                ctx->RSSetViewports(1, &khsg_vph);
                khfp_cbd.fx_meta[0] = 26.0f;
                if (!kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                    if (khsg_vpn) ctx->RSSetViewports(1, &khsg_vps);
                    ctx->PSSetSamplers(2, 1, &khsg_s2old);
                    if (khsg_s2old) khsg_s2old->Release();
                    return;
                }
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[4], nullptr);
                ctx->PSSetShaderResources(0, 1, &src_srv);
                ctx->Draw(3, 0);   // The radiance seed (effect 26), the gather's one level (KH_SSGI_SEED).
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                khfp_cbd.fx_meta[0] = 22.0f;
                if (!kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                    if (khsg_vpn) ctx->RSSetViewports(1, &khsg_vps);
                    ctx->PSSetSamplers(2, 1, &khsg_s2old);
                    if (khsg_s2old) khsg_s2old->Release();
                    return;
                }
                ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[4]);
                ctx->PSSetShader(g_res.ps_ssgi_gather, nullptr, 0);   // KH_FX_SPLIT: its own entry point.
                ctx->Draw(3, 0);   // The gather (t0 = source for albedo, t3 = the seed its taps read).
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
                khsg_fin = g_res.chain_srv[2];   // Raw gather until smoothing lands.
                // Missing buffer or a failed upload skips the draw whole - the resolve reads the raw gather instead
                // (one noisier frame, never a lost pass).
                if (g_res.chain_rtv[3] && g_res.chain_srv[3]) {
                    khfp_cbd.fx_meta[0] = 25.0f;
                    khfp_cbd.local0[0] = 1.0f;
                    ctx->PSSetShader(g_res.ps_ssgi_atrous, nullptr, 0);   // KH_FX_SPLIT: the three iterations'.

                    if (kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                        ctx->OMSetRenderTargets(1, &g_res.chain_rtv[3], nullptr);
                        ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[2]);
                        ctx->Draw(3, 0);
                        ID3D11ShaderResourceView* khsg_n3 = nullptr;
                        ctx->PSSetShaderResources(3, 1, &khsg_n3);
                        khsg_fin = g_res.chain_srv[3];

                        khfp_cbd.local0[0] = 2.0f;   // Iteration B: sp, back into [2].
                        if (kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                            ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[3]);
                            ctx->Draw(3, 0);
                            ctx->PSSetShaderResources(3, 1, &khsg_n3);
                            khsg_fin = g_res.chain_srv[2];

                            khfp_cbd.local0[0] = 4.0f;
                            if (kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[3], nullptr);
                                ctx->PSSetShaderResources(3, 1, &g_res.chain_srv[2]);
                                ctx->Draw(3, 0);
                                ctx->PSSetShaderResources(3, 1, &khsg_n3);
                                khsg_fin = g_res.chain_srv[3];
                            }
                        }
                    }

                }
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);   // KH_FX_SPLIT: the pass's own again.
                khfp_cbd.local0[0] = khsg_l0sv;
                khfp_cbd.local0[1] = khsg_l1sv;
                khfp_cbd.local0[2] = khsg_l2sv;
                khfp_cbd.local0[3] = khsg_l3sv;
                if (khsg_vpn) {
                    ctx->RSSetViewports(1, &khsg_vps);
                } else {
                    khsg_vph.Width  = (float)g_res.scene_w;
                    khsg_vph.Height = (float)g_res.scene_h;
                    ctx->RSSetViewports(1, &khsg_vph);
                }
                // When the spread param is auto (< 0.5) the flush writes the scale-matched value 1.5 x inv; explicit
                // user values pass through untouched. khfp_cbd is per-pass, so no restore is owed.
                if (khfp_cbd.fx2[3] < 0.5f)
                    khfp_cbd.fx2[3] = 1.5f * ((float)g_res.scene_w
                                    / (float)(g_khsg_w > 0 ? g_khsg_w : 1));
                khfp_cbd.fx_meta[0] = 24.0f;
                khsg_pair = true;
            }
            // KH_DLF: dynamicLightFog - its lights into the ring (t40, kh_dlf_select), our meshes' light maps at
            // t36 (shadows >= 1; their lanes are the frame's, kh_fx_frame_template's), the gather (32) onto the SSGI
            // grid's [2], two filter iterations (33: [2] -> [3] at step 1, [3] -> [2] at step 2; a failed upload
            // keeps the last good buffer, the SSGI's rule), then the pass itself (31) composites the final buffer
            // from t3, bound below once the OM has moved off it.
            // dl_ctl.x arms the composite; unarmed (no light selected, the ring, the grid or the gather's upload
            // failed, an unarmed camera, no medium - KH_DLF_MEDIUM0) it passes the picture through. local0 carries
            // the grid to the side draws (x = the filter's step, y = the grid factor, zw = the grid's size) and is put
            // back before the composite, whose localization mask reads it; so is the viewport.
            ID3D11ShaderResourceView* khdlf_fin = nullptr;
            if (khfp_effect == static_cast<int>(EffectId::DynLightFog) && !khfp_ps) {
                KH_GPU_SCOPE(ctx, KHG_DLF);   // KH_PROF.
                khfp_cbd.dl_ctl[0] = 0.0f;
                static thread_local std::vector<float> khdlf_rec;   // Scratch.
                uint32_t khdlf_pn = 0, khdlf_first = 0;
                float khdlf_scale = 1.0f;
                // KH_DLF_MEDIUM0: no medium anywhere - every sample of the gather continues, the grid holds 0 (or a
                // NaN of a non-finite tint), and the armed composite adds max(that, 0) = 0: the unarmed composite's
                // picture, so the pass runs as with no light selected (no ring append, gather or filter, and no frame
                // linear depth - a later reader draws it, by the same key; the composite still writes, so the chain's
                // write-back stands). A channel holding -0.0 may come out +0.0 or the reverse under additive /
                // screen - zero either way. kh_dlf_select is not called, so its dlf_unshadowed stat (KH_DL_DIAG)
                // counts only lights of passes that have a medium.
                const uint32_t khdlf_n = (g_res.chain_rtv[2] && g_res.chain_srv[2] && g_khsg_w > 0 && g_khsg_h > 0 &&
                                          g_res.ps_dlf_gather && g_res.ps_dlf_filter &&   // KH_FX_SPLIT.
                                          !kh_dlf_medium_zero(khfp_cbd))
                                       ? kh_dlf_select(khfp_cbd, khdlf_rec, khdlf_pn, khdlf_scale) : 0u;
                const bool khdlf_up = khdlf_n > 0 && kh_dlr_append(ctx, khdlf_rec.data(), khdlf_n * 6u, khdlf_first);
                if (khdlf_n > 0 && !khdlf_up) kh_stat(g_stats.dl_ring_fails);   // KH_DL_DIAG.
                if (khdlf_up) {
                    memcpy(&khfp_cbd.dl_first[0], &khdlf_first, sizeof(khdlf_first));
                    khfp_cbd.dl_first[1] = 0.0f;
                    khfp_cbd.dl_first[2] = 0.0f;
                    khfp_cbd.dl_first[3] = 0.0f;
                    kh_dl_fill_ctl(khfp_cbd, khdlf_n, khdlf_pn, khdlf_scale);   // dl_ctl.x = 3: armed.
                    float khdlf_l0[4];
                    memcpy(khdlf_l0, khfp_cbd.local0, sizeof(khdlf_l0));
                    khfp_cbd.local0[0] = 1.0f;
                    khfp_cbd.local0[1] = static_cast<float>(g_res.scene_w) / static_cast<float>(g_khsg_w);
                    khfp_cbd.local0[2] = static_cast<float>(g_khsg_w);
                    khfp_cbd.local0[3] = static_cast<float>(g_khsg_h);
                    // KH_FX_LINZ: the frame's (the lights are up: the gather, the filter and the composite read
                    // it), then the grid's copy (KhDlfGridZ: KhDlfFull at this grid) for the filter and the
                    // composite - an armed pass's, drawn once a run; mat_ctl w = 2 arms it. t9 holds this pass's
                    // copy, or nothing when unarmed (the helper's size query and load meet no other route's view).
                    khlz_frame(khfp_cbd);
                    if (khfp_cbd.mat_ctl[0] > 0.5f) {
                        if (!khlz_gd_tried) {
                            khlz_gd_tried = true;
                            khlz_save();
                            ConstantData khlz_c = khfp_cbd;
                            khlz_gd_ok = kh_fx_linz_draw(dev, ctx, 2, khlz_c, static_cast<UINT>(g_khsg_w),
                                                         static_cast<UINT>(g_khsg_h), g_res.constant_buffer);
                        }
                        if (khlz_gd_ok) khfp_cbd.mat_ctl[3] = 2.0f;
                    }
                    khlz_save();
                    ctx->PSSetShaderResources(9, 1, khfp_cbd.mat_ctl[3] > 1.5f ? &g_res.linz_srv[2] : &khlz_none);
                    D3D11_VIEWPORT khdlf_vps; UINT khdlf_vpn = 1;
                    ctx->RSGetViewports(&khdlf_vpn, &khdlf_vps);
                    D3D11_VIEWPORT khdlf_vpg = {};
                    khdlf_vpg.Width    = static_cast<float>(g_khsg_w);
                    khdlf_vpg.Height   = static_cast<float>(g_khsg_h);
                    khdlf_vpg.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &khdlf_vpg);
                    ctx->PSSetShader(g_res.ps_dlf_gather, nullptr, 0);   // KH_FX_SPLIT: its own entry point.
                    ID3D11ShaderResourceView* khdlf_null = nullptr;
                    ctx->PSSetShaderResources(3, 1, &khdlf_null);   // Off t3 before [2] becomes a target.
                    khfp_cbd.fx_meta[0] = static_cast<float>(KH_DLF_GATHER);
                    if (kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                        ID3D11ShaderResourceView* khdlf_dls = (khfp_cbd.fx1[2] >= 0.5f) ? g_res.dls_srv : nullptr;
                        // KH_DLF_SPLIT (the note at Resources::ps_dlf_gather_lean): an armed pass marks the texels a
                        // layer may cover (PSDlfClass, stencil 1) and draws the full gather there, the lean build on
                        // the rest; an unarmed pass draws the lean build alone (the full build's record code does not
                        // run there); an armed pass without the marker or the stencil, or any pass without the lean
                        // build, the full build alone. The pass's constants serve all three draws (the
                        // marker reads the gather's own lanes).
                        ID3D11DepthStencilView* const khds_dsv =
                            khfp_wsa ? kh_dlf_split_ensure(dev, g_res.chain_tex[2]) : nullptr;
                        if (khds_dsv) {
                            ID3D11DepthStencilState* khds_old = nullptr;
                            UINT khds_oref = 0;
                            ctx->OMGetDepthStencilState(&khds_old, &khds_oref);
                            // KH_STEN_CLEAR_BOTH (E's stencil's note): the depth plane is unread.
                            ctx->ClearDepthStencilView(khds_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                            ctx->OMSetRenderTargets(0, nullptr, khds_dsv);   // The marker writes the stencil alone.
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_mark, 1);
                            ctx->PSSetShader(g_res.ps_dlf_class, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], khds_dsv);
                            ctx->PSSetShaderResources(0, 1, &src_srv);
                            ctx->PSSetShaderResources(36, 1, &khdlf_dls);
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_sel, 0);
                            ctx->PSSetShader(g_res.ps_dlf_gather_lean, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetDepthStencilState(g_res.dss_dlf_sel, 1);
                            ctx->PSSetShader(g_res.ps_dlf_gather, nullptr, 0);
                            ctx->Draw(3, 0);
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);   // The stencil off the OM.
                            ctx->OMSetDepthStencilState(khds_old, khds_oref);
                            KH_SAFE_RELEASE(khds_old);
                        } else {
                            ctx->OMSetRenderTargets(1, &g_res.chain_rtv[2], nullptr);
                            ctx->PSSetShaderResources(0, 1, &src_srv);
                            ctx->PSSetShaderResources(36, 1, &khdlf_dls);
                            if (!khfp_wsa && g_res.ps_dlf_gather_lean) {
                                ctx->PSSetShader(g_res.ps_dlf_gather_lean, nullptr, 0);
                            }
                            ctx->Draw(3, 0);
                        }
                        ctx->PSSetShaderResources(36, 1, &khdlf_null);
                        khdlf_fin = g_res.chain_srv[2];
                        if (g_res.chain_rtv[3] && g_res.chain_srv[3]) {
                            khfp_cbd.fx_meta[0] = static_cast<float>(KH_DLF_FILTER);
                            ctx->PSSetShader(g_res.ps_dlf_filter, nullptr, 0);   // KH_FX_SPLIT: both iterations'.
                            for (int khdlf_it = 0; khdlf_it < 2; ++khdlf_it) {
                                khfp_cbd.local0[0] = khdlf_it == 0 ? 1.0f : 2.0f;
                                if (!kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) break;
                                const int khdlf_to = khdlf_it == 0 ? 3 : 2;
                                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[khdlf_to], nullptr);
                                ctx->PSSetShaderResources(3, 1, &khdlf_fin);
                                ctx->Draw(3, 0);
                                ctx->PSSetShaderResources(3, 1, &khdlf_null);
                                khdlf_fin = g_res.chain_srv[khdlf_to];
                            }
                        }
                    }
                    ctx->PSSetShader(g_res.ps_effect, nullptr, 0);   // KH_FX_SPLIT: the pass's own again.
                    memcpy(khfp_cbd.local0, khdlf_l0, sizeof(khdlf_l0));
                    if (khdlf_vpn) {
                        ctx->RSSetViewports(1, &khdlf_vps);
                    } else {
                        khdlf_vpg.Width  = static_cast<float>(g_res.scene_w);
                        khdlf_vpg.Height = static_cast<float>(g_res.scene_h);
                        ctx->RSSetViewports(1, &khdlf_vpg);
                    }
                    khfp_cbd.fx_meta[0] = static_cast<float>(EffectId::DynLightFog);
                    if (!khdlf_fin) khfp_cbd.dl_ctl[0] = 0.0f;
                }
            }
            // KH_FX_SIDE: fog scatter's per-pixel fog (side id 28, the frame's size) or the sun flare's visibility
            // (29, 1 x 1), drawn before the pass's constants go up; they then arm matCtl.y, and t4 carries it.
            int khfx_side = -1;
            if (!khfp_ps && (khfp_effect == static_cast<int>(EffectId::Fogscatter) ||
                             khfp_effect == static_cast<int>(EffectId::SunFlare))) {
                const bool khfx_fog = khfp_effect == static_cast<int>(EffectId::Fogscatter);
                const UINT khfx_w = khfx_fog ? static_cast<UINT>((khfp_cbd.fx_meta[2] > 0.0f ? khfp_cbd.fx_meta[2] : 0.0f) + 0.5f) : 1u;
                const UINT khfx_h = khfx_fog ? static_cast<UINT>((khfp_cbd.fx_meta[3] > 0.0f ? khfp_cbd.fx_meta[3] : 0.0f) + 0.5f) : 1u;
                // KH_WSEE_EDGE_COST: fog scatter's value split by E's stencil where it stands (an armed pass).
                ID3D11DepthStencilView* const khfx_st = (khfx_fog && khfp_wsa && khfc_we_st) ? g_res.wsee_es_dsv
                                                                                             : nullptr;
                if (kh_fx_side_draw(dev, ctx, khfx_fog ? 0 : 1, khfp_cbd, khfx_fog ? 28.0f : 29.0f, khfx_w, khfx_h,
                                    nullptr, g_res.constant_buffer, khfx_st)) {
                    khfx_side = khfx_fog ? 0 : 1;
                    khfp_cbd.mat_ctl[1] = 1.0f;
                }
            }
            // KH_GLOW_PYR: a glow pass's pyramid, of this pass's own source, built before its constants go up.
            bool khgl_on = false;
            if ((!khfp_ps && kh_glow_effect(khfp_effect)) || (khfp_ps && khfp_upyr)) {   // KH_FX_USER: or a user's.
                khgl_on = kh_glow_build(dev, ctx, g_res.glow_pyr[0], src_srv, khfp_cbd.fx_meta[2], khfp_cbd.fx_meta[3],
                                        khfp_cbd.center_size[3], khfp_effect, khfp_cbd.fx0[0], g_res.constant_buffer);
                kh_glow_lanes(khfp_cbd, khgl_on ? &g_res.glow_pyr[0] : nullptr);
            }
            // KH_ANA_PYR: anamorphic's one-axis pyramid (per axis), likewise before its constants go up.
            const KhAnaPyr* khan_pyr = nullptr;
            if (!khfp_ps && khfp_effect == static_cast<int>(EffectId::Anamorphic)) {
                const int khan_ax = khfp_cbd.fx1[0] > 0.5f ? 1 : 0;
                if (kh_ana_build(dev, ctx, g_res.ana_pyr[0][khan_ax], src_srv, khfp_cbd.fx_meta[2], khfp_cbd.fx_meta[3],
                                 khfp_cbd.center_size[3], khan_ax, khfp_cbd.fx0[0], g_res.constant_buffer))
                    khan_pyr = &g_res.ana_pyr[0][khan_ax];
                kh_ana_lanes(khfp_cbd, khan_pyr);
            }
            // KH_WSEE: C (t59) for an armed pass's own draw - the scene target's samples, which the run's final
            // write overwrites. The runtime refuses a view of a resource bound as an output (it nulls the view; the
            // reads give 0), so C is bound only while the OM holds a ping-pong target - after the target switch below,
            // nulled right after the draw - and never across a side draw (each puts back the OM it found, the scene's
            // at the run's first pass). A last pass that may read it (the distance fog; a user effect, whose helpers
            // may) draws into the ping-pong target as a middle pass does, and the scene target takes a copy of that
            // (ps_wsee_copy, below: the chain's format is the capture's, so the same values); any other last pass
            // draws straight into the scene, without C. Bit 1 of fx_tex[1].w goes up with this draw alone.
            // KH_WSEE_EDGE_COST: C goes to a reader's draw alone (the distance fog, a user effect) - a middle pass
            // that never loads t59 draws without it (bit 1 is read by effect 13 and a user effect alone).
            const bool khfp_wsc_reader = khfp_effect == static_cast<int>(EffectId::Fog) || khfp_ps != nullptr;
            const bool khfp_reroute = khfp_final && khfp_wsa && khfc_wsc && khfp_wsc_reader && g_res.ps_wsee_copy;
            const bool khfp_wsc = khfp_wsa && khfc_wsc && khfp_wsc_reader && (!khfp_final || khfp_reroute);
            if (khfp_wsc) khfp_cbd.fx_tex[1][3] += 2.0f;
            if (!kh_upload_obj_cb(ctx, g_res.constant_buffer, khfp_cbd)) {
                if (khsg_pair) ctx->PSSetSamplers(2, 1, &khsg_s2old);   // KH_FX_SSGI_BAIL: s2 back on
                                                                        // the pair path.
                if (khsg_s2old) khsg_s2old->Release();   // The pair path's ref (twin above; null on
                                                         // every non-pair path).
                return;
            }
            if (khfp_lut) ctx->PSSetShaderResources(19, 1, &khfp_lut);
            // KH_FS_SPLIT: fog scatter's own entry where its side draw is armed (khfx_side 0: mat_ctl y up, t4 bound
            // below) - PSFsSplitS reads t4 unconditionally; a failed side draw or the entry absent: PSEffect's branch.
            const bool khfs_split = khfx_side == 0 && g_res.ps_fs_split != nullptr;
            // KH_FX_LEAN: an unlocalized, unbanded pass (the shaders' own tests of localParams1.y / bandParams.w)
            // takes the lean build - fog scatter's entry, dynamicLightFog's composite (PSEffect's branch 31) - which
            // runs no record maths: such a pass reaches none (cb.hlsl's KH_FX_LEAN note), so the same values.
            const bool khfp_lean = !(khfp_cbd.local1[1] > 0.5f) && !(khfp_cbd.band0[3] > 0.5f);
            ID3D11PixelShader* const khfs_ps = (khfp_lean && g_res.ps_fs_split_lean) ? g_res.ps_fs_split_lean
                                                                                     : g_res.ps_fs_split;
            const bool khdc_lean = !khfp_ps && khfp_lean && g_res.ps_dlf_comp != nullptr &&
                                   khfp_effect == static_cast<int>(EffectId::DynLightFog);
            // KH_FX_LEAN_PASS (the note at Resources::ps_effect_lean): a builtin pass the constants just uploaded
            // leave unarmed draws PSEffect's lean build.
            const bool khfp_plean = !khfp_ps && !khfs_split && !khdc_lean && g_res.ps_effect_lean != nullptr &&
                                    !(khfp_cbd.fx_tex[1][2] > 0.5f);
            ctx->PSSetShader(khfp_ps ? khfp_ps : (khfs_split ? khfs_ps : (khdc_lean ? g_res.ps_dlf_comp
                                                                         : (khfp_plean ? g_res.ps_effect_lean
                                                                                       : g_res.ps_effect))),
                             nullptr, 0);
            if (khfp_ps) ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_FX_USER: s1 for every user effect draw.
            // Unbind the source slot before binding it as RTV next round.
            ID3D11ShaderResourceView* khfp_null = nullptr;
            ctx->PSSetShaderResources(0, 1, &khfp_null);
            if (khfp_final && !khfp_reroute) khfp_om.restore(ctx);
            else                             ctx->OMSetRenderTargets(1, &g_res.chain_rtv[write_idx], nullptr);
            // KH_WSEE_EDGE_COST: an armed builtin pass with a per-sample site drawing PSEffect into the chain's
            // ping-pong target (a middle pass, or a rerouted last one) is split by E's stencil where the target is
            // E's size (one GetDesc); the final write into the multisampled scene target is never split (no stencil
            // of its sample count), nor a user effect or another entry (PSFsSplitS).
            bool khfp_split = false;
            if ((!khfp_final || khfp_reroute) && khfp_wsa && khfc_we_st && !khfp_ps && !khfs_split && !khdc_lean &&
                khfp_obj && kh_wsee_ps_site(*khfp_obj) && g_res.chain_tex[write_idx] && g_res.dss_wsee_ssel) {
                D3D11_TEXTURE2D_DESC khfp_ctd = {};
                g_res.chain_tex[write_idx]->GetDesc(&khfp_ctd);
                khfp_split = khfp_ctd.Width == g_res.wsee_e_w && khfp_ctd.Height == g_res.wsee_e_h &&
                             khfp_ctd.SampleDesc.Count == 1;
            }
            ctx->PSSetShaderResources(0, 1, &src_srv);
            if (khfp_wsc) ctx->PSSetShaderResources(59, 1, &khfc_wsc);   // KH_WSEE: the OM is off the scene.
            // Gather at t3 for the resolve draw (bound after the OM above moved the RTV off buffer 2 - binding it
            // earlier would be a read-write hazard the runtime resolves by silently nulling the SRV); unbound right
            // after the draw.
            if (khsg_pair) {
                ctx->PSSetShaderResources(3, 1, &khsg_fin);
            }
            if (khdlf_fin) ctx->PSSetShaderResources(3, 1, &khdlf_fin);   // KH_DLF: the fog, for the composite.
            KhGlowBind khgl_b;   // KH_GLOW_PYR: the pyramid at t3 and its sampler at s2, for this draw.
            if (khgl_on) khgl_b.bind(ctx, g_res.glow_pyr[0]);
            KhAnaBind khan_b;   // KH_ANA_PYR: its atlases at t3 / t6, for this draw.
            if (khan_pyr) khan_b.bind(ctx, *khan_pyr);
            if (khfx_side >= 0) ctx->PSSetShaderResources(4, 1, &g_res.fxs_srv[khfx_side]);   // KH_FX_SIDE.
            // KH_WSEE_EDGE_COST.
            if (khfp_split) kh_wsee_split_draw(ctx, g_res.chain_rtv[write_idx], g_res.wsee_es_dsv,
                                               g_res.constant_buffer, &khfp_cbd);   // KH_WSEE_EPIX: its slice.
            else ctx->Draw(3, 0);   // KH_FX_TEX: the pages bound at the top (khfp_ts) come off at the pass's end.
            if (khfp_wsc) {   // KH_WSEE: C off before any target can be the scene again.
                ID3D11ShaderResourceView* khfp_wsn = nullptr;
                ctx->PSSetShaderResources(59, 1, &khfp_wsn);
            }
            khan_b.unbind();
            khgl_b.unbind();
            if (khfs_split || khdc_lean || khfp_plean) {   // KH_FS_SPLIT, KH_FX_LEAN, KH_FX_LEAN_PASS.
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
            }
            if (khfx_side >= 0) {   // Unbound at once: the next side draw renders into it.
                ID3D11ShaderResourceView* khfx_n = nullptr;
                ctx->PSSetShaderResources(4, 1, &khfx_n);
            }
            if (khsg_pair) {
                ID3D11ShaderResourceView* khsg_n2 = nullptr;
                ctx->PSSetShaderResources(3, 1, &khsg_n2);
                ctx->PSSetSamplers(2, 1, &khsg_s2old);   // KH_FX_SAMP_S2: exact prior binding
                                                         // back.
                if (khsg_s2old) { khsg_s2old->Release(); khsg_s2old = nullptr; }
            }
            if (khdlf_fin) {   // KH_DLF: off t3 before the next pass's draws.
                ID3D11ShaderResourceView* khdlf_n3 = nullptr;
                ctx->PSSetShaderResources(3, 1, &khdlf_n3);
            }
            if (khfp_reroute) {   // KH_WSEE: the last pass's result into the scene target (the final write).
                ID3D11ShaderResourceView* khfp_wcn = nullptr;
                ctx->PSSetShaderResources(0, 1, &khfp_wcn);
                khfp_om.restore(ctx);
                ctx->PSSetShaderResources(0, 1, &g_res.chain_srv[write_idx]);
                ctx->PSSetShader(g_res.ps_wsee_copy, nullptr, 0);
                ctx->Draw(3, 0);
                ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
            }

            if (!khfp_final) {
                src_srv = g_res.chain_srv[write_idx];
                write_idx ^= 1;
            }
        };

        for (const auto& f : fullscreen) {
            if (f.second.effect <= 0) continue;
            // No-op skip (KH_FX_SKIP_DOC): an opacity of 0.001 or less is dropped, not drawn, so a (near-)zero-opacity
            // pass costs nothing. At 0 it is an exact identity under the chain packing for every blend mode; up to
            // 0.001 at most a thousandth of its change (invisible but for an additive colour in the hundreds).
            if (f.second.color[3] <= 0.001f) continue;
            // KH_LOCAL_CULL: a localized pass whose volume no pixel's point can reach - skipped as above, here
            // for the reason the next note gives.
            if (khfc_lc && kh_local_culled(*khfc_lc, f.second)) continue;
            // KH_DLF: no fresh light in the pool, or no intensity - the pass would add nothing, so it is skipped
            // whole as a zero-opacity pass is. Here, ahead of the pending pass's flush: a skip after it would leave
            // the chain's last pass unflushed as the final one, and its write-back into the scene lost. Only under a
            // blend its unarmed composite leaves as it was (normal, lighten, darken): additive, multiply and screen
            // blend the unchanged picture into itself, so there the pass runs and blends as it does armed.
            if (f.second.effect == static_cast<int>(EffectId::DynLightFog) &&
                (f.second.blend_mode == 0 || f.second.blend_mode == 4 || f.second.blend_mode == 5) &&
                (!(f.second.fx[0] > 0.0f) || !kh_dlf_pool_live())) continue;
            // KH_FX_SPLIT: SSGI without its gather / a-trous entry points (failed to build, reported) is skipped
            // here for the same reason - the flush's drop-whole gate would otherwise lose the chain's write-back
            // were it the last pass.
            if (f.second.effect == static_cast<int>(EffectId::Ssgi) &&
                (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) continue;

            // Custom effect: the pass draws with the user PS; absent or failed compiles skip the pass (reported once).
            ID3D11PixelShader* khc_ufx = nullptr;
            bool khc_upyr = false;   // KH_FX_USER.

            if (f.second.effect == KH_EFFECT_CUSTOM) {
                khc_ufx = kh_user_fx_ps(dev, kh_fx_shader_of(f.second), &khc_upyr);
                if (!khc_ufx) continue;
            }

            ID3D11ShaderResourceView* khc_lut = nullptr;

            if (f.second.effect == KH_EFFECT_LUT) {
                khc_lut = kh_user_lut_srv(dev, kh_fx_shader_of(f.second));
                if (!khc_lut) continue;
            }

            if (khfp_live && !khfp_ps && kh_fuse_appendable(f.second) &&
                kh_fuse_append(khfp_cbd, f.second, khfc_now_d)) continue;

            khfp_flush(false);
            if (!upload_cb(f.second, true, &khfp_cbd)) continue;
            if (f.second.effect == static_cast<int>(EffectId::Fogscatter))
                kh_fogscatter_pack(khfp_cbd, fullscreen);
            else if (f.second.effect == KH_EFFECT_CUSTOM)   // KH_FX_FOGREC: a user effect's view of the fog passes.
                kh_fog_records_pack(khfp_cbd.fx_fog[0], khfp_cbd.fx_fog[1], fullscreen);
            else if (f.second.effect == static_cast<int>(EffectId::DynLightFog))   // KH_DLF_FOGREC: and the DLF's.
                kh_dlf_fog_pack(khfp_cbd, fullscreen, &f.second);
            if (needs_depth(f.second)) {
                // The pair the engine encoded this cycle's depth with, read off its own upload when valid - the recipe
                // every stable depth reader here uses; the flush-selected pair is the fallback. KH_FX_PAIR_HOLD: either
                // goes through the hold guard, whose verdict for this flush its frame made first (flush_locked,
                // KH_FX_ENGINE_PAIR: the engine pair read, the cycle's camera latched) - the inverse and every reader
                // take one pair. The range lanes are this pass's own, put back after a hold. Effect MESHES keep the
                // flush-selected pair from upload_cb: KhFxBegin's depth gate tests their own fragments' view depth with
                // it (their far-frame arbitration takes a pair of its own, local0). One pair and one range per pass,
                // for the whole depth: a partitioned frame (KH_PART_NEAR), whose depth holds more than one encode (the
                // flush's KH_LATE_WORLD_PAIR note), has the pixels of the partition this pair did not encode decoded
                // with it too - not handled here.
                if (g_ro.engine_proj_valid) {
                    khfp_cbd.depth_params[0] = g_ro.engine_m22;
                    khfp_cbd.depth_params[1] = g_ro.engine_m32;
                }
                const float khfp_rz = khfp_cbd.depth_params[2], khfp_rw = khfp_cbd.depth_params[3];
                kh_fx_depth_pair_guard(khfp_cbd, g_cc_flush_serial.load(std::memory_order_relaxed));
                khfp_cbd.depth_params[2] = khfp_rz;
                khfp_cbd.depth_params[3] = khfp_rw;
            }
            khfp_cbd.shadow_meta2[0] = khfc_nzv ? 1.0f : 0.0f;   // KH_NEARZ_MARK: LoadDepthPS reads t37.
            // KH_VM_SEE: 1 = the depth readers take the scene's depth behind the view model's translucent planes
            // (LoadDepthPS, t34); 2 = the builtin passes that put the world's medium or light into the picture also
            // fade by their transmittance (KhFxFinish, t41). A user effect is 1: it may be a screen filter, which the
            // fade would weaken under the planes - one that is a medium fades itself (KhVmSeeFade, the contract). Fused
            // stages ride the live pass's arm. Only with the world's range above 0 (depthParams.z): the view model's
            // slice lies below it, and the tests are against it.
            khfp_cbd.shadow_meta2[3] = 0.0f;
            if (khfc_vmsee && needs_depth(f.second) && khfp_cbd.depth_params[2] > 0.0f) {
                const int khvs_e = f.second.effect;
                const bool khvs_t = khvs_e == static_cast<int>(EffectId::Fog) ||
                                    khvs_e == static_cast<int>(EffectId::Fogscatter) ||
                                    khvs_e == static_cast<int>(EffectId::DynLightFog) ||
                                    khvs_e == static_cast<int>(EffectId::Ssgi);
                khfp_cbd.shadow_meta2[3] = khvs_t ? 2.0f : 1.0f;
            }
            // KH_WSEE: the pass's mode (kh_wsee_mode_of) - 1 blends what the pass derives from the depth by the
            // transmittance between the depth behind the world's blended layers and the front one (cb.hlsl /
            // effect.hlsl's KH_WSEE; the depth itself is never substituted, so the linear-depth copies' key is
            // unchanged) - only with the world's range above 0 and this cycle's capture valid; 0 otherwise, as on
            // every draw outside this loop. upload_cb's copy of the frame template zeroed the lane (and [1].w, set by
            // khfp_flush); the pass's side draws and linear-depth copies take it with khfp_cbd (kh_fx_tex_fill leaves
            // both). A pass that would take a mode wants the capture whether or not this one is valid - with the
            // system built in (KH_WSEE_ON false: no mode, no want).
            if constexpr (KH_WSEE_ON) {
                if (needs_depth(f.second) && khfp_cbd.depth_params[2] > 0.0f) {
                    const int khws_m = kh_wsee_mode_of(f.second);
                    if (khws_m > 0) {
                        khfc_wsee_want = true;
                        // KH_WSEE_BWANT: a colour reader wants B / B2 from the next clear; in a cycle whose capture
                        // took none it is drawn unarmed (the picture of a frame before the record's first capture: the
                        // record's own want lag).
                        const bool khws_cr = kh_wsee_col_reader(f.second);
                        if (khws_cr) khfc_col_want = true;
                        if (khfc_wsee && (khfc_col || !khws_cr)) khfp_cbd.fx_tex[1][2] = static_cast<float>(khws_m);
                    }
                }
            }
            khfp_effect = f.second.effect;
            khfp_ps = khc_ufx;
            khfp_upyr = khc_upyr;
            khfp_obj = &f.second;
            khfp_lut = khc_lut;
            khfp_live = true;
        }

        if (khfp_live) khfp_flush(true);
        // KH_WSEE: the want, with the cycle this run draws (the published one: this may be the game thread's late
        // chain), once the capture's and the re-issues' shaders and states stand (kh_wsee_ensure - its first call may
        // compile).
        if (khfc_wsee_want && kh_wsee_ensure(dev)) {
            g_wsee_want_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed), std::memory_order_relaxed);
            if (khfc_col_want) {   // KH_WSEE_BWANT: and the colours with it.
                g_wsee_col_want_cycle.store(g_topo_cycles_pub.load(std::memory_order_relaxed),
                                            std::memory_order_relaxed);
            }
        }
        // Unconditional: a bail out of the final flush returns above the restore and would otherwise leave a chain RTV
        // bound for the rest of the frame.
        khfp_om.restore(ctx);
        khfp_om.release();
        if (khlz_saved) {   // KH_FX_LINZ: t7 - t9 back as the run found them.
            ctx->PSSetShaderResources(7, 3, khlz_prev);
            for (ID3D11ShaderResourceView*& khlz_p : khlz_prev) KH_SAFE_RELEASE(khlz_p);
        }
    }
}

}