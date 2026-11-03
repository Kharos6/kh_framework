#pragma once

// The feature half (rendering_integration_features.hpp): WSEE's side draws for the effect chain (split, agreement,
// MS scene) and the readers' modes. Never include it on its own.
namespace RenderIntegration {

// WSEE_EDGE_COST - one draw of a frame-size pass with a per-sample site, split by E's stencil (dsv: the
// run's, marked where E's class word is nonzero): the OM's targets as bound (rtv, E's size) with the stencil,
// drawn under dss_wsee_ssel at ref 0 (the agreeing pixels) and then at ref 1 (the edge pixels) - the same shader,
// constants and bindings, so each pixel is drawn exactly once and the value is the same whether or not the driver
// runs a quad's stencil-failed pixels as helper lanes. The saving is not: the edge pixels' per-sample loop runs in
// warps of its own only where the driver does not run those helpers in the other draw; the passes split read no
// quad derivative (no deriv_* / implicit-LOD sample in PSEffect's or PSFsSide's DXBC), so a driver may drop them. A
// user effect is never split. The depth state and the targets come back as dss_off over rtv alone (the
// chain's own). Cost: one more draw call and its state sets per split pass.
// WSEE_EPIX: with the pass's constants given (cb / cbd: the slice the caller uploaded), the edge
// pixels are drawn first with that slice, then the agreeing ones with lane bits 0 and 2 clear - stencil 0 is word 0
// there, and with bit 2 clear KhWseeCountW (the only reader of bits 0 and 2 a split shader has) gives what word 0
// gives (one pair, no gradient, word 0) without loading E: the same values, the draws' order free (disjoint pixels);
// then the slice goes up again as given (the pass's constants after the call are as before it). A failed upload leaves
// the slice bound as it was (upload_obj_cb rebinds only what it wrote): that half then takes the pass's own lanes.
// Cost: two constant uploads (the ring's) per split pass.
inline void wsee_split_draw(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv,
                            ID3D11DepthStencilView* dsv, ID3D11Buffer* cb = nullptr,
                            const ConstantData* cbd = nullptr) {
    ctx->OMSetRenderTargets(1, &rtv, dsv);
    const bool lean = cb && cbd;   // WSEE_EPIX: the edge pixels first.
    if (!lean) {
        ctx->OMSetDepthStencilState(g_res.dss_wsee_ssel, 0);
        ctx->Draw(3, 0);
    }
    ctx->OMSetDepthStencilState(g_res.dss_wsee_ssel, 1);
    ctx->Draw(3, 0);
    if (lean) {   // WSEE_EPIX: the agreeing pixels without E, then the pass's own slice again.
        ConstantData c = *cbd;
        const uint32_t av = static_cast<uint32_t>(c.fx_tex[1][3] + 0.5f) & ~5u;
        c.fx_tex[1][3] = static_cast<float>(av);
        const bool up = upload_obj_cb(ctx, cb, c);
        ctx->OMSetDepthStencilState(g_res.dss_wsee_ssel, 0);
        ctx->Draw(3, 0);
        if (up) upload_obj_cb(ctx, cb, *cbd);
    }
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
}

// WSEE - the run's sample-agreement mask E (t58): PSWseeAgree (effect.hlsl; the class word - nonzero where a
// pixel's depth samples or T samples disagree, else 0) into an R32_UINT target of w x h (the chain's frame),
// from cbd - the run's first sited pass's own constants (its lanes: the shader arms as that pass does).
// fx_linz_draw's contract: its own upload to cb (the pass uploads after it), every other binding put back
// (GlowSave), t58 nulled first and left null - the caller binds the result. It reads t1 (the depth) and t54 (T),
// both the run's. The target is made at first use and remade on a size change, released by release_wsee_targets (1x,
// a long idle, the device - a capture rekey leaves it); a refused make is reported once and not retried at its size
// (wsee_e_fail_*; another size tries again, the latch goes with the device). False = none drawn: the passes test
// inline (fx_tex[1].w bit 0 stays clear).
// WSEE_EDGE_COST: the target is R32_UINT - the class word (cb.hlsl KhWseeClassWord), drawn at the run's first
// sited pass - cleared to 0 before the draw (PSWseeAgree discards where the word is 0), with E's stencil
// (Resources::wsee_es_*: made with E, at its size) cleared to 0 and marked 1 where the word is nonzero; st says
// the stencil stands for this run (false where it was refused: E alone). Per run: two clears (E's 33 MB at
// 3840x2160, the stencil's) besides the draw; none of it without a site.
inline bool wsee_agree_draw(ID3D11Device* dev, ID3D11DeviceContext* ctx, const ConstantData& cbd, UINT w,
                            UINT h, ID3D11Buffer* cb, bool& st) {
    st = false;
    if (!dev || !ctx || !cb || !g_res.ps_wsee_agree || !g_res.vs_fullscreen || !g_res.rasterizer ||
        !g_res.dss_off) return false;
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return false;
    ID3D11ShaderResourceView* null = nullptr;
    ctx->PSSetShaderResources(58, 1, &null);   // Off its slot before it is a target.
    if (!g_res.wsee_e_tex || !g_res.wsee_e_rtv || !g_res.wsee_e_srv || g_res.wsee_e_w != w ||
        g_res.wsee_e_h != h) {
        KH_SAFE_RELEASE(g_res.wsee_e_srv);
        KH_SAFE_RELEASE(g_res.wsee_e_rtv);
        KH_SAFE_RELEASE(g_res.wsee_e_tex);
        g_res.wsee_e_w = g_res.wsee_e_h = 0;
        KH_SAFE_RELEASE(g_res.wsee_es_dsv);   // WSEE_EDGE_COST: the stencil at E's size, made below.
        KH_SAFE_RELEASE(g_res.wsee_es_tex);
        // WSEE_EFAIL_KEY: refused at this size alone (another size tries again).
        if (g_res.wsee_e_failed && g_res.wsee_e_fail_w == w && g_res.wsee_e_fail_h == h) return false;
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_UINT;   // WSEE_EDGE_COST: the class word.
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_e_tex)) ||
            FAILED(dev->CreateRenderTargetView(g_res.wsee_e_tex, nullptr, &g_res.wsee_e_rtv)) ||
            FAILED(dev->CreateShaderResourceView(g_res.wsee_e_tex, nullptr, &g_res.wsee_e_srv))) {
            KH_SAFE_RELEASE(g_res.wsee_e_srv);
            KH_SAFE_RELEASE(g_res.wsee_e_rtv);
            KH_SAFE_RELEASE(g_res.wsee_e_tex);
            g_res.wsee_e_failed = true;
            g_res.wsee_e_fail_w = w;   // WSEE_EFAIL_KEY.
            g_res.wsee_e_fail_h = h;
            report_error_once_safe("KH RenderIntegration: world see-through: the agreement mask could not be made "
                                   "(the passes test the samples inline)");
            return false;
        }
        g_res.wsee_e_w = w;
        g_res.wsee_e_h = h;
    }
    // WSEE_EDGE_COST: E's stencil and its two states, made once at E's size (the states with the device); refused
    // at E's size - not retried at that size, reported once (WSEE_EFAIL_KEY; the latch goes with the device).
    const bool ref = g_res.wsee_es_failed && g_res.wsee_es_fail_w == w && g_res.wsee_es_fail_h == h;
    if (!ref && (!g_res.wsee_es_dsv || !g_res.dss_wsee_smark || !g_res.dss_wsee_ssel)) {
        KH_SAFE_RELEASE(g_res.wsee_es_dsv);
        KH_SAFE_RELEASE(g_res.wsee_es_tex);
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R24G8_TYPELESS;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        D3D11_DEPTH_STENCIL_VIEW_DESC vd = {};
        vd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        vd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
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
        bool es_ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_es_tex)) &&
                     SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_es_tex, &vd, &g_res.wsee_es_dsv));
        if (es_ok && !g_res.dss_wsee_smark) {
            es_ok = SUCCEEDED(dev->CreateDepthStencilState(&dd, &g_res.dss_wsee_smark));
        }
        if (es_ok && !g_res.dss_wsee_ssel) {
            dd.StencilWriteMask = 0x00;
            dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
            dd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
            dd.BackFace = dd.FrontFace;
            es_ok = SUCCEEDED(dev->CreateDepthStencilState(&dd, &g_res.dss_wsee_ssel));
        }
        if (!es_ok) {
            KH_SAFE_RELEASE(g_res.wsee_es_dsv);
            KH_SAFE_RELEASE(g_res.wsee_es_tex);
            g_res.wsee_es_failed = true;
            g_res.wsee_es_fail_w = w;   // WSEE_EFAIL_KEY.
            g_res.wsee_es_fail_h = h;
            report_error_once_safe("KH RenderIntegration: world see-through: the edge stencil could not be made (the "
                                   "per-sample draws are not split)");
        }
    }
    const bool sm = g_res.wsee_es_dsv && g_res.dss_wsee_smark && g_res.dss_wsee_ssel;
    WseeGputScope gt(ctx, WSEE_GPU_CHAIN_E);   // WSEE_GPUT: E's clears and draw.
    GlowSave sv;
    sv.capture(ctx);
    const bool we_ok = upload_obj_cb(ctx, cb, cbd);
    if (we_ok) {
        const FLOAT zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   // WSEE_EDGE_COST: no word where discarded.
        ctx->ClearRenderTargetView(g_res.wsee_e_rtv, zero);
        // STEN_CLEAR_BOTH: depth and stencil together - the depth plane is unread (no SRV, every state on the view
        // has DepthEnable FALSE), and a stencil-only clear of D24S8 must keep it (on many GPUs a read-modify-write).
        if (sm) ctx->ClearDepthStencilView(g_res.wsee_es_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
        ctx->OMSetRenderTargets(1, &g_res.wsee_e_rtv, sm ? g_res.wsee_es_dsv : nullptr);
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<float>(w);
        vp.Height = static_cast<float>(h);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_wsee_agree, nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(sm ? g_res.dss_wsee_smark : g_res.dss_off, 1);   // WSEE_EDGE_COST.
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
        st = sm;
    }
    sv.restore(ctx);
    return we_ok;
}

// WSEE - C (t59): a Texture2DMS view, in its RTV's format, of the scene target the chain captures from
// (rtv: the target bound at the chain's start, the resolve source of ensure_scene_capture_from - so its samples
// are the very colours the run's input capture R averaged), when that texture is multisampled at the capture's count
// and size and was made with D3D11_BIND_SHADER_RESOURCE; null otherwise (the shader's fallback: the per-sample
// factors averaged over the resolved colour). Kept while the key (the texture's identity and the format, and the
// capture's size and count it was checked against) holds - the view holds a reference to the engine's texture, so the
// identity cannot be another's meanwhile -, released by release_wsee_targets (1x, a long idle, the device), at a
// sample-count change and at the swapchain's resize (release_wsee_c; a capture rekey leaves it - the key above
// remakes it at its next call); a refused view is not retried for that texture and format until C is released
// (release_wsee_c clears the refusal with the view: the texture's address may be another's after it) or the device
// goes. The chain's thread (the render thread, a park that stalls it, or the game thread's late chain inside a
// resolve the engine issues there: the engine never drives the immediate context from two threads at once).
inline ID3D11ShaderResourceView* wsee_scene_ms(ID3D11Device* dev, ID3D11RenderTargetView* rtv) {
    if (!dev || !rtv || g_res.wsee_samples < 2) return nullptr;
    D3D11_RENDER_TARGET_VIEW_DESC vd = {};
    rtv->GetDesc(&vd);
    if (vd.ViewDimension != D3D11_RTV_DIMENSION_TEXTURE2DMS || vd.Format == DXGI_FORMAT_UNKNOWN) {
        return nullptr;
    }
    ID3D11Resource* r = nullptr;
    rtv->GetResource(&r);
    if (!r) return nullptr;
    void* const id = static_cast<void*>(r);
    if (g_res.wsee_c_srv && g_res.wsee_c_res == id && g_res.wsee_c_fmt == vd.Format &&
        g_res.wsee_c_w == g_res.wsee_w && g_res.wsee_c_h == g_res.wsee_h && g_res.wsee_c_s == g_res.wsee_samples) {
        r->Release();
        return g_res.wsee_c_srv;
    }
    KH_SAFE_RELEASE(g_res.wsee_c_srv);
    g_res.wsee_c_res = nullptr;
    g_res.wsee_c_fmt = DXGI_FORMAT_UNKNOWN;
    g_res.wsee_c_w = g_res.wsee_c_h = g_res.wsee_c_s = 0;
    ID3D11Texture2D* t = nullptr;
    const HRESULT qh = r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t));
    r->Release();
    if (FAILED(qh) || !t) return nullptr;
    D3D11_TEXTURE2D_DESC td = {};
    t->GetDesc(&td);
    const bool fit = (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
                     td.SampleDesc.Count == g_res.wsee_samples && td.Width == g_res.wsee_w &&
                     td.Height == g_res.wsee_h && td.ArraySize == 1;
    if (!fit || (g_res.wsee_c_fail_res == id && g_res.wsee_c_fail_fmt == vd.Format)) {
        t->Release();
        return nullptr;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = vd.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    const HRESULT hr = dev->CreateShaderResourceView(t, &sd, &g_res.wsee_c_srv);
    t->Release();
    if (FAILED(hr) || !g_res.wsee_c_srv) {
        KH_SAFE_RELEASE(g_res.wsee_c_srv);
        g_res.wsee_c_fail_res = id;
        g_res.wsee_c_fail_fmt = vd.Format;
        report_error_once_safe("KH RenderIntegration: world see-through: the scene's sample-colour view could not be "
                               "made (the fog takes the per-sample approximation) " + hr_str(hr));
        return nullptr;
    }
    g_res.wsee_c_res = id;
    g_res.wsee_c_fmt = vd.Format;
    g_res.wsee_c_w = g_res.wsee_w;
    g_res.wsee_c_h = g_res.wsee_h;
    g_res.wsee_c_s = g_res.wsee_samples;
    return g_res.wsee_c_srv;
}
// WSEE - the mode a scene-chain pass takes in ConstantData fx_tex[1].z (the WSEE note at g_wsee_sv.rtvs, in
// rendering/see_through_shaders.hpp): 1 = two-depth, for the passes that use the capture - distance fog, fog
// scatter, dynamicLightFog, Pulse, the sun flare (each blends its own front / behind results by T), SSGI (over the
// sky its bounce takes the layers' share) and any localized / banded pass (its masks blend); Outline only when
// localized / banded (its edges stay on the front depth); a user effect when localized / banded or when its file
// names WSEE's helpers or views or KhSunFlareVis (user_fx_uses_wsee); 0 = none - a pass with no use for the
// capture neither arms nor wants it. No mode substitutes the depth (a layer's opaque texels keep their own). The
// chain sets it only with the world's range above 0 (depthParams.z) and a valid capture (fx_chain_run); 0 on
// every other draw. Every armed pass is particle-aware (it reads every layer, fence-type and particle-type alike).
inline int wsee_mode_of(const RenderObject& o) {
    if (!fx_needs_depth(o)) return 0;
    if (o.localized || o.banded) return 1;
    if (o.effect == static_cast<int>(EffectId::Outline)) return 0;
    if (o.effect == EFFECT_CUSTOM && !user_fx_uses_wsee(fx_shader_of(o))) return 0;
    return 1;
}
// WSEE_EDGE_COST - whether an ARMED pass (wsee_mode_of > 0) has a per-sample site: a reader of the
// agreement mask E (cb.hlsl KhWseeCount's callers - KhWseeMix and effect 13): the distance fog, fog scatter (its side
// draw 28 and taps), Pulse, the sun flare (its side draw 29's taps), any localized / banded pass (its masks) and a user
// effect (armed only when it names WSEE's helpers, or is localized / banded). The other armed passes - an
// unlocalized, unbanded SSGI or dynamicLightFog - read the record at sample 0 alone (KhWseeAt) and never E. Fused
// stages (fuse_appendable: never localized / banded) add no site. fx_chain_run draws E at the run's first
// armed pass with a site, so a run without one draws none.
inline bool wsee_ps_site(const RenderObject& o) {
    return o.localized || o.banded || o.effect == EFFECT_CUSTOM ||
           o.effect == static_cast<int>(EffectId::Fog) || o.effect == static_cast<int>(EffectId::Fogscatter) ||
           o.effect == static_cast<int>(EffectId::Pulse) || o.effect == static_cast<int>(EffectId::SunFlare);
}
// WSEE_BWANT - whether an ARMED pass (wsee_mode_of > 0) may read the scene colours B / B2 (t55 / t60): the
// distance fog (effect.hlsl's exact stages, its only reads) and a user effect whose file names the record (the contract
// gives it khWsB / khWsB2; user_fx_uses_wsee - a KhWsee / khWs identifier or KhSunFlareVis, read from the compile
// cache's entry: one key and one map find a call, as wsee_mode_of's own test - a localized or banded one naming none
// is armed for its masks alone and cannot read them). No other builtin, mask or side draw reads them; the flush's
// sorted parts do (KhWseeMeshCut, modes 2 / 3 - their own want, flush_locked). Fused stages never are one.
inline bool wsee_col_reader(const RenderObject& o) {
    return o.effect == static_cast<int>(EffectId::Fog) ||
           (o.effect == EFFECT_CUSTOM && user_fx_uses_wsee(fx_shader_of(o)));
}
}