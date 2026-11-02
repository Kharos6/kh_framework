#pragma once

// The feature half (rendering_integration_features.hpp): the glow and anamorphic pyramids: their sizes, targets and
// builds. Never include it on its own.
namespace RenderIntegration {

// KH_ANA_PYR - anamorphic's one-axis pyramid (effect.hlsl's KH_ANA_PYR note: PSAnaSeed / PSAnaDown make it, effect 15
// reads it; kh_ana_build). Level k keeps the frame's full size across the streak's axis and has kh_ana_n texels along
// it (one per 2^(k+1) frame pixels); the levels are strips of two atlases - [0] the even ones, [1] the odd, each at
// kh_ana_off along the axis - so a level is always built from the other atlas. R16G16B16A16_FLOAT, as the glow
// pyramid. Resources holds one per glow_pyr route and axis (ana_pyr[route][axis]), made at first use, remade when
// its frame changes size, released with the device (Resources::release).
static constexpr int KH_ANA_LEVELS = 8;   // HLSL twin: texels of 2 .. 256 frame pixels along the axis.
static_assert(KH_ANA_LEVELS >= 2 && KH_ANA_LEVELS % 2 == 0, "KH_ANA_LEVELS: the top two levels end the two atlases");
// HLSL twin KhAnaLevel (n, o): level k's texels along a frame khan_len pixels long, and its offset in its atlas.
inline UINT kh_ana_n(UINT khan_len, int khan_k) { return (khan_len + (2u << khan_k) - 1u) >> (khan_k + 1); }
inline UINT kh_ana_off(UINT khao_len, int khao_k) {
    UINT khao_o = 0u;
    for (int khao_j = khao_k & 1; khao_j < khao_k; khao_j += 2) khao_o += kh_ana_n(khao_len, khao_j);
    return khao_o;
}

// KH_GLOW_PYR - the glows and blurs read a pre-filtered picture (cb.hlsl's KH_GLOW_PYR note): the effects whose
// taps read it, and those among them that take it through their threshold (fxParams0.x, the bright pass).
// Anamorphic is not among them: its streak is a pixel thin, which an isotropic level cannot give without
// thickening it; it reads a pyramid of its own, halved along the streak alone (KH_ANA_PYR, kh_ana_build).
inline bool kh_glow_effect(int khge_e) {
    return khge_e == static_cast<int>(EffectId::Blur) || khge_e == static_cast<int>(EffectId::Bloom) ||
           khge_e == static_cast<int>(EffectId::Halation) || khge_e == static_cast<int>(EffectId::LensFlare) ||
           khge_e == static_cast<int>(EffectId::Clarity) ||
           khge_e == static_cast<int>(EffectId::RainLens) || khge_e == static_cast<int>(EffectId::Fogscatter);
}
inline bool kh_glow_bright(int khgb_e) {
    return khgb_e == static_cast<int>(EffectId::Bloom) || khgb_e == static_cast<int>(EffectId::Halation) ||
           khgb_e == static_cast<int>(EffectId::LensFlare);
}

// KH_GLOW_PYR - the pyramid for a frame of khpe_fw x khpe_fh (and the shared sampler), made or remade as needed.
// False (reported once) when anything cannot be made; the pass then takes its direct taps.
inline bool kh_glow_pyr_ensure(ID3D11Device* dev, KhGlowPyr& khpe_p, UINT khpe_fw, UINT khpe_fh) {
    if (khpe_fw == 0u || khpe_fh == 0u) return false;
    if (!g_res.glow_sampler) {
        D3D11_SAMPLER_DESC khpe_sd = {};
        khpe_sd.Filter = D3D11_FILTER_ANISOTROPIC;
        khpe_sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        khpe_sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        khpe_sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        khpe_sd.MaxAnisotropy = 16;
        khpe_sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        khpe_sd.MinLOD = 0.0f;
        khpe_sd.MaxLOD = D3D11_FLOAT32_MAX;
        const HRESULT khpe_shr = dev->CreateSamplerState(&khpe_sd, &g_res.glow_sampler);
        if (FAILED(khpe_shr)) {
            g_res.glow_sampler = nullptr;
            report_error_once_safe("KH glow pyramid: CreateSamplerState " + hr_str(khpe_shr));
            return false;
        }
    }
    const UINT khpe_q = 1u << (KH_GLOW_LEVELS - 1);
    const UINT khpe_w = ((khpe_fw + 1u) / 2u + khpe_q - 1u) / khpe_q * khpe_q;
    const UINT khpe_h = ((khpe_fh + 1u) / 2u + khpe_q - 1u) / khpe_q * khpe_q;
    if (khpe_p.tex && khpe_p.w == khpe_w && khpe_p.h == khpe_h) return true;
    khpe_p.release();
    D3D11_TEXTURE2D_DESC khpe_td = {};
    khpe_td.Width = khpe_w;
    khpe_td.Height = khpe_h;
    khpe_td.MipLevels = KH_GLOW_LEVELS;
    khpe_td.ArraySize = 1;
    khpe_td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    khpe_td.SampleDesc.Count = 1;
    khpe_td.Usage = D3D11_USAGE_DEFAULT;
    khpe_td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    HRESULT khpe_hr = dev->CreateTexture2D(&khpe_td, nullptr, &khpe_p.tex);
    if (SUCCEEDED(khpe_hr)) khpe_hr = dev->CreateShaderResourceView(khpe_p.tex, nullptr, &khpe_p.srv);
    for (int khpe_m = 0; SUCCEEDED(khpe_hr) && khpe_m < KH_GLOW_LEVELS; ++khpe_m) {
        D3D11_RENDER_TARGET_VIEW_DESC khpe_rd = {};
        khpe_rd.Format = khpe_td.Format;
        khpe_rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        khpe_rd.Texture2D.MipSlice = static_cast<UINT>(khpe_m);
        khpe_hr = dev->CreateRenderTargetView(khpe_p.tex, &khpe_rd, &khpe_p.rtv[khpe_m]);
        if (FAILED(khpe_hr)) break;
        D3D11_SHADER_RESOURCE_VIEW_DESC khpe_vd = {};
        khpe_vd.Format = khpe_td.Format;
        khpe_vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        khpe_vd.Texture2D.MostDetailedMip = static_cast<UINT>(khpe_m);
        khpe_vd.Texture2D.MipLevels = 1;
        khpe_hr = dev->CreateShaderResourceView(khpe_p.tex, &khpe_vd, &khpe_p.lvl_srv[khpe_m]);
    }
    if (FAILED(khpe_hr)) {
        khpe_p.release();
        report_error_once_safe("KH glow pyramid: " + hr_str(khpe_hr) + " (glows take their direct taps)");
        return false;
    }
    khpe_p.w = khpe_w;
    khpe_p.h = khpe_h;
    return true;
}

// KH_GLOW_PYR - the pass's lanes: armed with the pyramid's full-resolution extent, or disarmed (null).
inline void kh_glow_lanes(ConstantData& khgl_cbd, const KhGlowPyr* khgl_p) {
    khgl_cbd.fuse_meta[1] = khgl_p ? 1.0f : 0.0f;
    khgl_cbd.fuse_meta[2] = khgl_p ? 2.0f * static_cast<float>(khgl_p->w) : 0.0f;
    khgl_cbd.fuse_meta[3] = khgl_p ? 2.0f * static_cast<float>(khgl_p->h) : 0.0f;
}

// KH_GLOW_PYR - kh_glow_build's save of everything it binds, put back exactly as it was (the kh_sun_pf_convert
// pattern): targets and viewports, the input assembler's layout and topology, both shaders, the rasterizer,
// depth-stencil and blend states, and PS t0 / t3. b0 is NOT kept: the build uploads its own slice, so a caller
// uploads its pass's constants after it.
struct KhGlowSave {
    KhOmSave                  om;
    D3D11_VIEWPORT            vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT                      vp_n = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ID3D11InputLayout*        il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY  topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11VertexShader*       vs = nullptr;
    ID3D11PixelShader*        ps = nullptr;
    ID3D11RasterizerState*    rs = nullptr;
    ID3D11DepthStencilState*  dss = nullptr;
    UINT                      dss_ref = 0;
    ID3D11BlendState*         bs = nullptr;
    FLOAT                     bf[4] = {};
    UINT                      bmask = 0xFFFFFFFFu;
    ID3D11ShaderResourceView* t0 = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    void capture(ID3D11DeviceContext* ctx) {
        om.capture(ctx);
        ctx->RSGetViewports(&vp_n, vp);
        ctx->IAGetInputLayout(&il);
        ctx->IAGetPrimitiveTopology(&topo);
        ctx->VSGetShader(&vs, nullptr, nullptr);
        ctx->PSGetShader(&ps, nullptr, nullptr);
        ctx->RSGetState(&rs);
        ctx->OMGetDepthStencilState(&dss, &dss_ref);
        ctx->OMGetBlendState(&bs, bf, &bmask);
        ctx->PSGetShaderResources(0, 1, &t0);
        ctx->PSGetShaderResources(3, 1, &t3);
    }
    void restore(ID3D11DeviceContext* ctx) {
        // The pyramid's views off every slot first, so no target coming back meets one of ours bound.
        ID3D11ShaderResourceView* khgv_null = nullptr;
        ctx->PSSetShaderResources(0, 1, &khgv_null);
        ctx->PSSetShaderResources(3, 1, &khgv_null);
        om.restore(ctx);
        om.release();
        if (vp_n > 0) ctx->RSSetViewports(vp_n, vp);
        ctx->IASetInputLayout(il);
        ctx->IASetPrimitiveTopology(topo);
        // KH_CLASS_INST: the shaders come back without class instances (the capture takes none). Every user (the
        // chain, the scene flush's own glow build, the PIP's and the UI flush's builds) runs under a StateBackup, so
        // the shaders here are ours, bound without any; the engine's come back with theirs at that backup's restore.
        ctx->VSSetShader(vs, nullptr, 0);
        ctx->PSSetShader(ps, nullptr, 0);
        ctx->RSSetState(rs);
        ctx->OMSetDepthStencilState(dss, dss_ref);
        ctx->OMSetBlendState(bs, bf, bmask);
        ctx->PSSetShaderResources(0, 1, &t0);
        ctx->PSSetShaderResources(3, 1, &t3);
        KH_SAFE_RELEASE(il);
        KH_SAFE_RELEASE(vs);
        KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(rs);
        KH_SAFE_RELEASE(dss);
        KH_SAFE_RELEASE(bs);
        KH_SAFE_RELEASE(t0);
        KH_SAFE_RELEASE(t3);
    }
};

// KH_GLOW_PYR - build khgb_p from khgb_src, the pass's own source (its t0), for a pass of effect khgb_e over a frame
// of khgb_fw x khgb_fh (its fxMeta.zw): the seed through the pass's threshold (khgb_thr, fxParams0.x) when the
// effect is a bright pass and through SampleScene's premultiply when khgb_csw (its centerSize.w) is the UI spill
// lane's, then every level from the one above. Every binding it makes is put back (KhGlowSave) except b0, which
// carries its own slice (khgb_cb, the route's object buffer): call it BEFORE the pass's own upload. False = no
// pyramid (resources or shaders missing): the pass is left disarmed and takes its direct taps. Our draws, inside a
// pass that already stands the hooks aside.
inline bool kh_glow_build(ID3D11Device* dev, ID3D11DeviceContext* ctx, KhGlowPyr& khgb_p,
                          ID3D11ShaderResourceView* khgb_src, float khgb_fw, float khgb_fh, float khgb_csw,
                          int khgb_e, float khgb_thr, ID3D11Buffer* khgb_cb) {
    if (!dev || !ctx || !khgb_src || !khgb_cb || !g_res.ps_glow_seed || !g_res.ps_glow_down ||
        !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!(khgb_fw >= 1.0f && khgb_fh >= 1.0f && khgb_fw < 65536.0f && khgb_fh < 65536.0f)) return false;
    if (!kh_glow_pyr_ensure(dev, khgb_p, static_cast<UINT>(khgb_fw + 0.5f), static_cast<UINT>(khgb_fh + 0.5f))) return false;
    ConstantData khgb_cbd = {};
    khgb_cbd.fx_meta[2] = khgb_fw;   // SampleScene's clamp: the pass's frame.
    khgb_cbd.fx_meta[3] = khgb_fh;
    khgb_cbd.center_size[3] = khgb_csw;   // SampleScene's premultiply (w > 2.5, the UI spill lane).
    const bool khgb_bright = kh_glow_bright(khgb_e);
    khgb_cbd.local0[0] = khgb_bright ? khgb_thr : 0.0f;
    khgb_cbd.local0[1] = khgb_bright ? 1.0f : 0.0f;
    KhGlowSave khgb_sv;
    khgb_sv.capture(ctx);
    const bool khgb_ok = kh_upload_obj_cb(ctx, khgb_cb, khgb_cbd);
    if (khgb_ok) {
        ID3D11ShaderResourceView* khgb_null = nullptr;
        ctx->PSSetShaderResources(3, 1, &khgb_null);   // No view of the pyramid bound while its levels are targets.
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT khgb_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, khgb_bf, 0xFFFFFFFF);
        D3D11_VIEWPORT khgb_vp = {};
        khgb_vp.MaxDepth = 1.0f;
        ctx->OMSetRenderTargets(1, &khgb_p.rtv[0], nullptr);
        ctx->PSSetShaderResources(0, 1, &khgb_src);
        ctx->PSSetShader(g_res.ps_glow_seed, nullptr, 0);
        khgb_vp.Width = static_cast<FLOAT>(khgb_p.w);
        khgb_vp.Height = static_cast<FLOAT>(khgb_p.h);
        ctx->RSSetViewports(1, &khgb_vp);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 1, &khgb_null);
        ctx->PSSetShader(g_res.ps_glow_down, nullptr, 0);
        for (int khgb_m = 1; khgb_m < KH_GLOW_LEVELS; ++khgb_m) {
            ctx->OMSetRenderTargets(1, &khgb_p.rtv[khgb_m], nullptr);   // Level m - 1's view stays off it.
            ctx->PSSetShaderResources(3, 1, &khgb_p.lvl_srv[khgb_m - 1]);
            khgb_vp.Width = static_cast<FLOAT>(khgb_p.w >> khgb_m);
            khgb_vp.Height = static_cast<FLOAT>(khgb_p.h >> khgb_m);
            ctx->RSSetViewports(1, &khgb_vp);
            ctx->Draw(3, 0);
        }
    }
    khgb_sv.restore(ctx);
    return khgb_ok;
}

// KH_ANA_PYR - the pyramid for a frame of khpe_fw x khpe_fh along axis khpe_ax (0 x, 1 y), made or remade as needed.
// Each atlas is the frame's size across the axis and, along it, as long as its last level's strip ends. False
// (reported once) when anything cannot be made; the pass then takes its direct taps.
inline bool kh_ana_pyr_ensure(ID3D11Device* dev, KhAnaPyr& khpe_p, UINT khpe_fw, UINT khpe_fh, int khpe_ax) {
    if (khpe_fw == 0u || khpe_fh == 0u) return false;
    if (khpe_p.tex[0] && khpe_p.tex[1] && khpe_p.fw == khpe_fw && khpe_p.fh == khpe_fh) return true;
    khpe_p.release();
    const UINT khpe_len = khpe_ax ? khpe_fh : khpe_fw;
    HRESULT khpe_hr = S_OK;
    for (int khpe_a = 0; SUCCEEDED(khpe_hr) && khpe_a < 2; ++khpe_a) {
        const int khpe_k = KH_ANA_LEVELS - 2 + khpe_a;   // This atlas's top level (KH_ANA_LEVELS is even).
        const UINT khpe_ext = kh_ana_off(khpe_len, khpe_k) + kh_ana_n(khpe_len, khpe_k);
        D3D11_TEXTURE2D_DESC khpe_td = {};
        khpe_td.Width = khpe_ax ? khpe_fw : khpe_ext;
        khpe_td.Height = khpe_ax ? khpe_ext : khpe_fh;
        khpe_td.MipLevels = 1;
        khpe_td.ArraySize = 1;
        khpe_td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        khpe_td.SampleDesc.Count = 1;
        khpe_td.Usage = D3D11_USAGE_DEFAULT;
        khpe_td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        khpe_hr = dev->CreateTexture2D(&khpe_td, nullptr, &khpe_p.tex[khpe_a]);
        if (SUCCEEDED(khpe_hr))
            khpe_hr = dev->CreateShaderResourceView(khpe_p.tex[khpe_a], nullptr, &khpe_p.srv[khpe_a]);
        if (SUCCEEDED(khpe_hr))
            khpe_hr = dev->CreateRenderTargetView(khpe_p.tex[khpe_a], nullptr, &khpe_p.rtv[khpe_a]);
    }
    if (FAILED(khpe_hr)) {
        khpe_p.release();
        report_error_once_safe("KH anamorphic pyramid: " + hr_str(khpe_hr) + " (anamorphic takes its direct taps)");
        return false;
    }
    khpe_p.fw = khpe_fw;
    khpe_p.fh = khpe_fh;
    return true;
}

// KH_ANA_PYR - an anamorphic pass's lanes: armed with the frame its pyramid was made for, or disarmed (null). The
// glows' lanes (kh_glow_lanes), which an anamorphic pass never takes.
inline void kh_ana_lanes(ConstantData& khal_cbd, const KhAnaPyr* khal_p) {
    khal_cbd.fuse_meta[1] = khal_p ? 1.0f : 0.0f;
    khal_cbd.fuse_meta[2] = khal_p ? static_cast<float>(khal_p->fw) : 0.0f;
    khal_cbd.fuse_meta[3] = khal_p ? static_cast<float>(khal_p->fh) : 0.0f;
}

// KH_ANA_PYR - build khab_p from khab_src, the pass's own source (its t0), for an anamorphic pass over a frame of
// khab_fw x khab_fh (its fxMeta.zw) along axis khab_ax (its fxParams1.x > 0.5): level 0 through the pass's threshold
// (khab_thr, fxParams0.x) and SampleScene's premultiply when khab_csw (its centerSize.w) is the UI spill lane's,
// then every level from the one below, each into its strip. kh_glow_build's contract: every binding it makes is put
// back (KhGlowSave) except b0, which carries its own slices (khab_cb, the route's object buffer), so call it BEFORE
// the pass's own upload; false = no pyramid (resources or shaders missing): the pass is left disarmed and takes
// its direct taps. Our draws, inside a pass that already stands the hooks aside.
inline bool kh_ana_build(ID3D11Device* dev, ID3D11DeviceContext* ctx, KhAnaPyr& khab_p,
                         ID3D11ShaderResourceView* khab_src, float khab_fw, float khab_fh, float khab_csw,
                         int khab_ax, float khab_thr, ID3D11Buffer* khab_cb) {
    if (!dev || !ctx || !khab_src || !khab_cb || !g_res.ps_ana_seed || !g_res.ps_ana_down ||
        !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!(khab_fw >= 1.0f && khab_fh >= 1.0f && khab_fw <= 16384.0f && khab_fh <= 16384.0f)) return false;
    const UINT khab_w = static_cast<UINT>(khab_fw + 0.5f);
    const UINT khab_h = static_cast<UINT>(khab_fh + 0.5f);
    if (!kh_ana_pyr_ensure(dev, khab_p, khab_w, khab_h, khab_ax)) return false;
    const UINT khab_len = khab_ax ? khab_h : khab_w;
    ConstantData khab_cbd = {};
    khab_cbd.fx_meta[2] = khab_fw;   // SampleScene's clamp: the pass's frame (and the levels' length along).
    khab_cbd.fx_meta[3] = khab_fh;
    khab_cbd.center_size[3] = khab_csw;   // SampleScene's premultiply (w > 2.5, the UI spill lane).
    khab_cbd.local0[0] = khab_thr;
    khab_cbd.local0[2] = khab_ax ? 1.0f : 0.0f;
    KhGlowSave khab_sv;
    khab_sv.capture(ctx);
    ID3D11ShaderResourceView* khab_null = nullptr;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->RSSetState(g_res.rasterizer);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT khab_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(nullptr, khab_bf, 0xFFFFFFFF);
    bool khab_ok = true;
    for (int khab_k = 0; khab_ok && khab_k < KH_ANA_LEVELS; ++khab_k) {
        khab_cbd.local0[3] = static_cast<float>(khab_k);   // The level (PSAnaDown).
        khab_ok = kh_upload_obj_cb(ctx, khab_cb, khab_cbd);
        if (!khab_ok) break;
        ctx->PSSetShaderResources(3, 1, &khab_null);   // The atlas about to be a target is off every slot first.
        ctx->OMSetRenderTargets(1, &khab_p.rtv[khab_k & 1], nullptr);
        D3D11_VIEWPORT khab_vp = {};   // The level's strip.
        const FLOAT khab_o = static_cast<FLOAT>(kh_ana_off(khab_len, khab_k));
        const FLOAT khab_n = static_cast<FLOAT>(kh_ana_n(khab_len, khab_k));
        khab_vp.TopLeftX = khab_ax ? 0.0f : khab_o;
        khab_vp.TopLeftY = khab_ax ? khab_o : 0.0f;
        khab_vp.Width = khab_ax ? static_cast<FLOAT>(khab_w) : khab_n;
        khab_vp.Height = khab_ax ? khab_n : static_cast<FLOAT>(khab_h);
        khab_vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &khab_vp);
        if (khab_k == 0) {
            ctx->PSSetShader(g_res.ps_ana_seed, nullptr, 0);
            ctx->PSSetShaderResources(0, 1, &khab_src);
        } else {
            if (khab_k == 1) {
                ctx->PSSetShaderResources(0, 1, &khab_null);
                ctx->PSSetShader(g_res.ps_ana_down, nullptr, 0);
            }
            ctx->PSSetShaderResources(3, 1, &khab_p.srv[(khab_k - 1) & 1]);   // The level below, the other atlas.
        }
        ctx->Draw(3, 0);
    }
    khab_sv.restore(ctx);
    return khab_ok;
}
}