#pragma once

// The feature half (rendering_integration_features.hpp): the glow and anamorphic pyramids: their sizes, targets and
// builds. Never include it on its own.
namespace RenderIntegration {

// ANA_PYR - anamorphic's one-axis pyramid (effect.hlsl's ANA_PYR note: PSAnaSeed / PSAnaDown make it, effect 15
// reads it; ana_build). Level k keeps the frame's full size across the streak's axis and has ana_n texels along
// it (one per 2^(k+1) frame pixels); the levels are strips of two atlases - [0] the even ones, [1] the odd, each at
// ana_off along the axis - so a level is always built from the other atlas. R16G16B16A16_FLOAT, as the glow
// pyramid. Resources holds one per glow_pyr route and axis (ana_pyr[route][axis]), made at first use, remade when
// its frame changes size, released with the device (Resources::release).
static constexpr int KH_ANA_LEVELS = 8;   // HLSL twin: texels of 2 .. 256 frame pixels along the axis.
static_assert(KH_ANA_LEVELS >= 2 && KH_ANA_LEVELS % 2 == 0, "KH_ANA_LEVELS: the top two levels end the two atlases");
// HLSL twin KhAnaLevel (n, o): level k's texels along a frame len pixels long, and its offset in its atlas.
inline UINT ana_n(UINT len, int k) { return (len + (2u << k) - 1u) >> (k + 1); }
inline UINT ana_off(UINT len, int k) {
    UINT o = 0u;
    for (int j = k & 1; j < k; j += 2) o += ana_n(len, j);
    return o;
}

// GLOW_PYR - the glows and blurs read a pre-filtered picture (cb.hlsl's GLOW_PYR note): the effects whose
// taps read it, and those among them that take it through their threshold (fxParams0.x, the bright pass).
// Anamorphic is not among them: its streak is a pixel thin, which an isotropic level cannot give without
// thickening it; it reads a pyramid of its own, halved along the streak alone (ANA_PYR, ana_build).
inline bool glow_effect(int e) {
    return e == static_cast<int>(EffectId::Blur) || e == static_cast<int>(EffectId::Bloom) ||
           e == static_cast<int>(EffectId::Halation) || e == static_cast<int>(EffectId::LensFlare) ||
           e == static_cast<int>(EffectId::Clarity) ||
           e == static_cast<int>(EffectId::RainLens) || e == static_cast<int>(EffectId::Fogscatter);
}
inline bool glow_bright(int e) {
    return e == static_cast<int>(EffectId::Bloom) || e == static_cast<int>(EffectId::Halation) ||
           e == static_cast<int>(EffectId::LensFlare);
}

// GLOW_PYR - the pyramid for a frame of fw x fh (and the shared sampler), made or remade as needed.
// False (reported once) when anything cannot be made; the pass then takes its direct taps.
inline bool glow_pyr_ensure(ID3D11Device* dev, GlowPyr& p, UINT fw, UINT fh) {
    if (fw == 0u || fh == 0u) return false;
    if (!g_res.glow_sampler) {
        D3D11_SAMPLER_DESC sd = {};
        sd.Filter = D3D11_FILTER_ANISOTROPIC;
        sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sd.MaxAnisotropy = 16;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0.0f;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        const HRESULT shr = dev->CreateSamplerState(&sd, &g_res.glow_sampler);
        if (FAILED(shr)) {
            g_res.glow_sampler = nullptr;
            report_error_once_safe("KH RenderIntegration: glow pyramid: CreateSamplerState " + hr_str(shr));
            return false;
        }
    }
    const UINT q = 1u << (GLOW_LEVELS - 1);
    const UINT w = ((fw + 1u) / 2u + q - 1u) / q * q;
    const UINT h = ((fh + 1u) / 2u + q - 1u) / q * q;
    if (p.tex && p.w == w && p.h == h) return true;
    p.release();
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = GLOW_LEVELS;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    HRESULT hr = dev->CreateTexture2D(&td, nullptr, &p.tex);
    if (SUCCEEDED(hr)) hr = dev->CreateShaderResourceView(p.tex, nullptr, &p.srv);
    for (int m = 0; SUCCEEDED(hr) && m < GLOW_LEVELS; ++m) {
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = td.Format;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rd.Texture2D.MipSlice = static_cast<UINT>(m);
        hr = dev->CreateRenderTargetView(p.tex, &rd, &p.rtv[m]);
        if (FAILED(hr)) break;
        D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
        vd.Format = td.Format;
        vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        vd.Texture2D.MostDetailedMip = static_cast<UINT>(m);
        vd.Texture2D.MipLevels = 1;
        hr = dev->CreateShaderResourceView(p.tex, &vd, &p.lvl_srv[m]);
    }
    if (FAILED(hr)) {
        p.release();
        report_error_once_safe("KH RenderIntegration: glow pyramid: " + hr_str(hr) +
                               " (glows take their direct taps)");
        return false;
    }
    p.w = w;
    p.h = h;
    return true;
}

// GLOW_PYR - the pass's lanes: armed with the pyramid's full-resolution extent, or disarmed (null).
inline void glow_lanes(ConstantData& cbd, const GlowPyr* p) {
    cbd.fuse_meta[1] = p ? 1.0f : 0.0f;
    cbd.fuse_meta[2] = p ? 2.0f * static_cast<float>(p->w) : 0.0f;
    cbd.fuse_meta[3] = p ? 2.0f * static_cast<float>(p->h) : 0.0f;
}

// GLOW_PYR - glow_build's save of everything it binds, put back exactly as it was (the sun_pf_convert
// pattern): targets and viewports, the input assembler's layout and topology, both shaders, the rasterizer,
// depth-stencil and blend states, and PS t0 / t3. b0 is NOT kept: the build uploads its own slice, so a caller
// uploads its pass's constants after it.
struct GlowSave {
    OmSave                  om;
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
        ID3D11ShaderResourceView* null = nullptr;
        ctx->PSSetShaderResources(0, 1, &null);
        ctx->PSSetShaderResources(3, 1, &null);
        om.restore(ctx);
        om.release();
        if (vp_n > 0) ctx->RSSetViewports(vp_n, vp);
        ctx->IASetInputLayout(il);
        ctx->IASetPrimitiveTopology(topo);
        // CLASS_INST: the shaders come back without class instances (the capture takes none). Every user (the
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

// GLOW_PYR - build p from src, the pass's own source (its t0), for a pass of effect e over a frame
// of fw x fh (its fxMeta.zw): the seed through the pass's threshold (thr, fxParams0.x) when the
// effect is a bright pass and through SampleScene's premultiply when csw (its centerSize.w) is the UI spill
// lane's, then every level from the one above. Every binding it makes is put back (GlowSave) except b0, which
// carries its own slice (cb, the route's object buffer): call it BEFORE the pass's own upload. False = no
// pyramid (resources or shaders missing): the pass is left disarmed and takes its direct taps. Our draws, inside a
// pass that already stands the hooks aside.
inline bool glow_build(ID3D11Device* dev, ID3D11DeviceContext* ctx, GlowPyr& p,
                       ID3D11ShaderResourceView* src, float fw, float fh, float csw,
                       int e, float thr, ID3D11Buffer* cb) {
    if (!dev || !ctx || !src || !cb || !g_res.ps_glow_seed || !g_res.ps_glow_down ||
        !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!(fw >= 1.0f && fh >= 1.0f && fw < 65536.0f && fh < 65536.0f)) return false;
    if (!glow_pyr_ensure(dev, p, static_cast<UINT>(fw + 0.5f), static_cast<UINT>(fh + 0.5f))) return false;
    ConstantData cbd = {};
    cbd.fx_meta[2] = fw;   // SampleScene's clamp: the pass's frame.
    cbd.fx_meta[3] = fh;
    cbd.center_size[3] = csw;   // SampleScene's premultiply (w > 2.5, the UI spill lane).
    const bool bright = glow_bright(e);
    cbd.local0[0] = bright ? thr : 0.0f;
    cbd.local0[1] = bright ? 1.0f : 0.0f;
    GlowSave sv;
    sv.capture(ctx);
    const bool ok = upload_obj_cb(ctx, cb, cbd);
    if (ok) {
        ID3D11ShaderResourceView* null = nullptr;
        ctx->PSSetShaderResources(3, 1, &null);   // No view of the pyramid bound while its levels are targets.
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->RSSetState(g_res.rasterizer);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
        D3D11_VIEWPORT vp = {};
        vp.MaxDepth = 1.0f;
        ctx->OMSetRenderTargets(1, &p.rtv[0], nullptr);
        ctx->PSSetShaderResources(0, 1, &src);
        ctx->PSSetShader(g_res.ps_glow_seed, nullptr, 0);
        vp.Width = static_cast<FLOAT>(p.w);
        vp.Height = static_cast<FLOAT>(p.h);
        ctx->RSSetViewports(1, &vp);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 1, &null);
        ctx->PSSetShader(g_res.ps_glow_down, nullptr, 0);
        for (int m = 1; m < GLOW_LEVELS; ++m) {
            ctx->OMSetRenderTargets(1, &p.rtv[m], nullptr);   // Level m - 1's view stays off it.
            ctx->PSSetShaderResources(3, 1, &p.lvl_srv[m - 1]);
            vp.Width = static_cast<FLOAT>(p.w >> m);
            vp.Height = static_cast<FLOAT>(p.h >> m);
            ctx->RSSetViewports(1, &vp);
            ctx->Draw(3, 0);
        }
    }
    sv.restore(ctx);
    return ok;
}

// ANA_PYR - the pyramid for a frame of fw x fh along axis ax (0 x, 1 y), made or remade as needed.
// Each atlas is the frame's size across the axis and, along it, as long as its last level's strip ends. False
// (reported once) when anything cannot be made; the pass then takes its direct taps.
inline bool ana_pyr_ensure(ID3D11Device* dev, AnaPyr& p, UINT fw, UINT fh, int ax) {
    if (fw == 0u || fh == 0u) return false;
    if (p.tex[0] && p.tex[1] && p.fw == fw && p.fh == fh) return true;
    p.release();
    const UINT len = ax ? fh : fw;
    HRESULT hr = S_OK;
    for (int a = 0; SUCCEEDED(hr) && a < 2; ++a) {
        const int k = KH_ANA_LEVELS - 2 + a;   // This atlas's top level (KH_ANA_LEVELS is even).
        const UINT ext = ana_off(len, k) + ana_n(len, k);
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = ax ? fw : ext;
        td.Height = ax ? ext : fh;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        hr = dev->CreateTexture2D(&td, nullptr, &p.tex[a]);
        if (SUCCEEDED(hr))
            hr = dev->CreateShaderResourceView(p.tex[a], nullptr, &p.srv[a]);
        if (SUCCEEDED(hr))
            hr = dev->CreateRenderTargetView(p.tex[a], nullptr, &p.rtv[a]);
    }
    if (FAILED(hr)) {
        p.release();
        report_error_once_safe("KH RenderIntegration: anamorphic pyramid: " + hr_str(hr) +
                               " (anamorphic takes its direct taps)");
        return false;
    }
    p.fw = fw;
    p.fh = fh;
    return true;
}

// ANA_PYR - an anamorphic pass's lanes: armed with the frame its pyramid was made for, or disarmed (null). The
// glows' lanes (glow_lanes), which an anamorphic pass never takes.
inline void ana_lanes(ConstantData& cbd, const AnaPyr* p) {
    cbd.fuse_meta[1] = p ? 1.0f : 0.0f;
    cbd.fuse_meta[2] = p ? static_cast<float>(p->fw) : 0.0f;
    cbd.fuse_meta[3] = p ? static_cast<float>(p->fh) : 0.0f;
}

// ANA_PYR - build p from src, the pass's own source (its t0), for an anamorphic pass over a frame of
// fw x fh (its fxMeta.zw) along axis ax (its fxParams1.x > 0.5): level 0 through the pass's threshold
// (thr, fxParams0.x) and SampleScene's premultiply when csw (its centerSize.w) is the UI spill lane's,
// then every level from the one below, each into its strip. glow_build's contract: every binding it makes is put
// back (GlowSave) except b0, which carries its own slices (cb, the route's object buffer), so call it BEFORE
// the pass's own upload; false = no pyramid (resources or shaders missing): the pass is left disarmed and takes
// its direct taps. Our draws, inside a pass that already stands the hooks aside.
inline bool ana_build(ID3D11Device* dev, ID3D11DeviceContext* ctx, AnaPyr& p,
                      ID3D11ShaderResourceView* src, float fw, float fh, float csw,
                      int ax, float thr, ID3D11Buffer* cb) {
    if (!dev || !ctx || !src || !cb || !g_res.ps_ana_seed || !g_res.ps_ana_down ||
        !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) return false;
    if (!(fw >= 1.0f && fh >= 1.0f && fw <= 16384.0f && fh <= 16384.0f)) return false;
    const UINT w = static_cast<UINT>(fw + 0.5f);
    const UINT h = static_cast<UINT>(fh + 0.5f);
    if (!ana_pyr_ensure(dev, p, w, h, ax)) return false;
    const UINT len = ax ? h : w;
    ConstantData cbd = {};
    cbd.fx_meta[2] = fw;   // SampleScene's clamp: the pass's frame (and the levels' length along).
    cbd.fx_meta[3] = fh;
    cbd.center_size[3] = csw;   // SampleScene's premultiply (w > 2.5, the UI spill lane).
    cbd.local0[0] = thr;
    cbd.local0[2] = ax ? 1.0f : 0.0f;
    GlowSave sv;
    sv.capture(ctx);
    ID3D11ShaderResourceView* null = nullptr;
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->RSSetState(g_res.rasterizer);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
    bool ok = true;
    for (int k = 0; ok && k < KH_ANA_LEVELS; ++k) {
        cbd.local0[3] = static_cast<float>(k);   // The level (PSAnaDown).
        ok = upload_obj_cb(ctx, cb, cbd);
        if (!ok) break;
        ctx->PSSetShaderResources(3, 1, &null);   // The atlas about to be a target is off every slot first.
        ctx->OMSetRenderTargets(1, &p.rtv[k & 1], nullptr);
        D3D11_VIEWPORT vp = {};   // The level's strip.
        const FLOAT o = static_cast<FLOAT>(ana_off(len, k));
        const FLOAT n = static_cast<FLOAT>(ana_n(len, k));
        vp.TopLeftX = ax ? 0.0f : o;
        vp.TopLeftY = ax ? o : 0.0f;
        vp.Width = ax ? static_cast<FLOAT>(w) : n;
        vp.Height = ax ? n : static_cast<FLOAT>(h);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        if (k == 0) {
            ctx->PSSetShader(g_res.ps_ana_seed, nullptr, 0);
            ctx->PSSetShaderResources(0, 1, &src);
        } else {
            if (k == 1) {
                ctx->PSSetShaderResources(0, 1, &null);
                ctx->PSSetShader(g_res.ps_ana_down, nullptr, 0);
            }
            ctx->PSSetShaderResources(3, 1, &p.srv[(k - 1) & 1]);   // The level below, the other atlas.
        }
        ctx->Draw(3, 0);
    }
    sv.restore(ctx);
    return ok;
}
}