#pragma once

// The injection half (rendering_integration_injection.hpp): the engine captures: scene colour, depth, the see-through
// targets and the composite depth snapshot. Never include it on its own.
namespace RenderIntegration {
inline void msaa_toggle_wipe(unsigned int tw_new);

// Capture core, parameterized on the source RTV: ensure_scene_capture calls it with the bound target (its one caller).
// The caller keeps ownership of the RTV reference. CAP_DEFER: copy false runs every check and (re)creation -
// the same verdict, the same target - and leaves the copy (a resolve, on an MSAA scene) to a later call.
inline std::string ensure_scene_capture_from(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11RenderTargetView* rtv, bool copy = true) {
    if (!rtv) return "no RTV";
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv->GetDesc(&rtv_desc);
    if (!res) return "RTV has no resource";
    ID3D11Texture2D* src_tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src_tex));
    res->Release();
    if (FAILED(hr) || !src_tex) return "RTV resource is not a Texture2D";
    D3D11_TEXTURE2D_DESC src_desc = {};
    src_tex->GetDesc(&src_desc);

    // Resolve target format must be non-typeless; the RTV view format is.
    DXGI_FORMAT capture_fmt = rtv_desc.Format;

    if (!g_res.scene_tex || g_res.scene_w != src_desc.Width ||
        g_res.scene_h != src_desc.Height || g_res.scene_fmt != capture_fmt) {
        g_res.release_scene_capture();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = src_desc.Width;
        td.Height = src_desc.Height;
        // Single-mip.
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = capture_fmt;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        hr = dev->CreateTexture2D(&td, nullptr, &g_res.scene_tex);
        if (FAILED(hr)) { src_tex->Release(); return "create scene capture tex " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.scene_tex, nullptr, &g_res.scene_srv);
        if (FAILED(hr)) {
            g_res.release_scene_capture();
            src_tex->Release();
            return "create scene capture SRV " + hr_str(hr);
        }
        g_res.scene_w = src_desc.Width;
        g_res.scene_h = src_desc.Height;
        g_res.scene_fmt = capture_fmt;
    }

    if (copy) {   // CAP_DEFER: false leaves the copy to a later call.
        if (src_desc.SampleDesc.Count > 1) {
            ctx->ResolveSubresource(g_res.scene_tex, 0, src_tex, 0, capture_fmt);
        } else {
            ctx->CopyResource(g_res.scene_tex, src_tex);
        }
    }

    src_tex->Release();
    return "";
}

inline std::string ensure_scene_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool copy = true) {
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return "no RTV bound";
    const std::string err = ensure_scene_capture_from(dev, ctx, rtv, copy);
    rtv->Release();
    return err;
}

// flush_locked's call sites and fx_chain_run's (only flush_locked calls it).
inline std::string scene_capture_timed(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_SCENE_CAPTURE);
    KH_GPU_SCOPE(ctx, GPU_SCENE_CAPTURE);   // PROF.
    return ensure_scene_capture(dev, ctx);
}

// BLEND_MINMAX - a lighten / darken mesh's own copy of the bound scene target in g_res.mm_tex (a resolve
// when the target is multisampled; ms says which): made at the first need, remade on a size or format
// change (ensure_scene_capture_from's rules), released with the device's other targets. scene_tex is not
// touched. Cost: one full-frame CopyResource / ResolveSubresource per such mesh per frame (flush_locked
// only), and one more frame-sized texture once any was drawn. False: no copy (the hardware forms stand).
inline bool mm_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool& ms) {
    KH_PROF_SCOPE(PROF_SCENE_CAPTURE);
    KH_GPU_SCOPE(ctx, GPU_SCENE_CAPTURE);   // PROF: counted with the scene captures.
    ms = false;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return false;
    D3D11_RENDER_TARGET_VIEW_DESC vd = {};
    rtv->GetDesc(&vd);
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    if (!res) return false;
    ID3D11Texture2D* src = nullptr;
    const HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src));
    res->Release();
    if (FAILED(hr) || !src) return false;
    D3D11_TEXTURE2D_DESC sd = {};
    src->GetDesc(&sd);
    const DXGI_FORMAT fmt = vd.Format;   // The view's typed format (a resolve needs one).
    if (!g_res.mm_tex || g_res.mm_w != sd.Width || g_res.mm_h != sd.Height || g_res.mm_fmt != fmt) {
        g_res.release_mm_capture();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = sd.Width;
        td.Height = sd.Height;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = fmt;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.mm_tex)) || !g_res.mm_tex ||
            FAILED(dev->CreateShaderResourceView(g_res.mm_tex, nullptr, &g_res.mm_srv)) || !g_res.mm_srv) {
            g_res.release_mm_capture();
            src->Release();
            return false;
        }
        g_res.mm_w = sd.Width;
        g_res.mm_h = sd.Height;
        g_res.mm_fmt = fmt;
    }
    ms = sd.SampleDesc.Count > 1;
    if (ms) ctx->ResolveSubresource(g_res.mm_tex, 0, src, 0, fmt);
    else         ctx->CopyResource(g_res.mm_tex, src);
    src->Release();
    return true;
}

// BLEND_MINMAX - one lighten / darken mesh's t3 (its own copy), put back as it was at the scope's end
// (GlowBind's rule).
struct MmBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    MmBind() = default;
    MmBind(const MmBind&) = delete;
    MmBind& operator=(const MmBind&) = delete;
    ~MmBind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(3, 1, &t3);
        KH_SAFE_RELEASE(t3);
    }
    bool bind(ID3D11DeviceContext* mb_ctx, ID3D11ShaderResourceView* srv) {
        if (ctx || !mb_ctx || !srv) return false;
        ctx = mb_ctx;
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSSetShaderResources(3, 1, &srv);
        return true;
    }
};

inline std::string ensure_depth_srv(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                    UINT* out_width, UINT* out_height) {
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    if (!dsv) return "no DSV bound";
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    dsv->Release();
    if (!res) return "DSV has no resource";
    // Dimensions are those of that same texture.
    if (g_res.depth_srv && static_cast<void*>(res) == g_eds_memo_res) {
        res->Release();
        if (out_width)  *out_width = g_eds_memo_w;
        if (out_height) *out_height = g_eds_memo_h;
        return "";
    }
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    if (FAILED(hr) || !tex) { res->Release(); return "DSV resource is not a Texture2D"; }
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    if (out_width)  *out_width = td.Width;
    if (out_height) *out_height = td.Height;

    if (g_res.depth_srv && g_res.depth_res_identity == static_cast<void*>(tex)) {
        g_eds_memo_res = static_cast<void*>(res);   // Key the fast path on the matched resource.
        g_eds_memo_w = td.Width;
        g_eds_memo_h = td.Height;
        res->Release();
        tex->Release();
        return "";
    }
    res->Release();

    // Shared dedup state (g_res.depth_sample_count, stored at the tail) keeps this and the flush-side trigger from
    // double-firing on one transition.
    msaa_toggle_wipe(td.SampleDesc.Count);

    g_res.release_depth_srv();

    if (!(td.BindFlags & D3D11_BIND_SHADER_RESOURCE)) {
        tex->Release();
        return "depth resource lacks BIND_SHADER_RESOURCE";
    }

    DXGI_FORMAT srv_fmt;

    switch (td.Format) {
        case DXGI_FORMAT_R24G8_TYPELESS:    srv_fmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case DXGI_FORMAT_R32_TYPELESS:      srv_fmt = DXGI_FORMAT_R32_FLOAT; break;
        case DXGI_FORMAT_R32G8X24_TYPELESS: srv_fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
        case DXGI_FORMAT_R16_TYPELESS:      srv_fmt = DXGI_FORMAT_R16_UNORM; break;
        default:
            tex->Release();
            return "unsupported depth format " + std::to_string(static_cast<int>(td.Format));
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = srv_fmt;

    if (td.SampleDesc.Count > 1) {
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    } else {
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
    }

    hr = dev->CreateShaderResourceView(tex, &sd, &g_res.depth_srv);

    if (FAILED(hr)) {
        tex->Release();
        return "create depth SRV " + hr_str(hr);
    }
    {   // SSAO_MARK: the stencil plane, where the format carries one (non-fatal).
        DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
        if (td.Format == DXGI_FORMAT_R24G8_TYPELESS) fmt = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        else if (td.Format == DXGI_FORMAT_R32G8X24_TYPELESS) fmt = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        if (fmt != DXGI_FORMAT_UNKNOWN) {
            D3D11_SHADER_RESOURCE_VIEW_DESC st_sd = sd;
            st_sd.Format = fmt;
            if (FAILED(dev->CreateShaderResourceView(tex, &st_sd, &g_res.depth_sten_srv))) g_res.depth_sten_srv = nullptr;
        }
    }

    // Read-only DSV over the same resource: D3D11 permits sampling the depth SRV while a READ_ONLY DSV is bound, so
    // depth testing keeps working during depth-sampling effects.
    {
        DXGI_FORMAT dsv_fmt = DXGI_FORMAT_UNKNOWN;
        UINT ro_flags = D3D11_DSV_READ_ONLY_DEPTH;

        switch (td.Format) {
            case DXGI_FORMAT_R24G8_TYPELESS:    dsv_fmt = DXGI_FORMAT_D24_UNORM_S8_UINT; ro_flags |= D3D11_DSV_READ_ONLY_STENCIL; break;
            case DXGI_FORMAT_R32_TYPELESS:      dsv_fmt = DXGI_FORMAT_D32_FLOAT; break;
            case DXGI_FORMAT_R32G8X24_TYPELESS: dsv_fmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; ro_flags |= D3D11_DSV_READ_ONLY_STENCIL; break;
            case DXGI_FORMAT_R16_TYPELESS:      dsv_fmt = DXGI_FORMAT_D16_UNORM; break;
            default: break;
        }

        if (dsv_fmt != DXGI_FORMAT_UNKNOWN) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
            dd.Format = dsv_fmt;
            dd.Flags = ro_flags;

            dd.ViewDimension = td.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                       : D3D11_DSV_DIMENSION_TEXTURE2D;

            if (FAILED(dev->CreateDepthStencilView(tex, &dd, &g_res.depth_dsv_ro))) {
                g_res.depth_dsv_ro = nullptr;   // depth-sampling effects will be skipped.
            }
        }
    }

    g_res.depth_res_identity = static_cast<void*>(tex);
    g_res.depth_sample_count = td.SampleDesc.Count;   // Dedup store (the wipe ran above).
    tex->Release();   // The SRV holds its own reference.
    return "";
}

// VM_SEE - the snapshot's two targets at the main depth's size. VMSEE_LAZY: made by the snapshot itself (the
// cycle's first view-model translucent draw of an armed cycle), so a scene with no view model - third person, a
// vehicle's 2D optics - makes none; nothing here compiles. Sized by the texture depth_srv views (the main depth,
// this cycle's: the snapshot binds it, having checked the bound depth is that texture) - read off the view, not
// through ensure_depth_srv, whose bound-view rebuild is the flush's own business. A size change remakes them.
inline bool vmsee_targets(ID3D11Device* dev) {
    if (!dev || !g_res.depth_srv) return false;
    UINT w = 0, h = 0;
    {
        ID3D11Resource* r = nullptr;
        g_res.depth_srv->GetResource(&r);
        ID3D11Texture2D* t = nullptr;
        if (r) {
            if (FAILED(r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t)))) {
                t = nullptr;
            }
            r->Release();
        }
        if (!t) return false;
        D3D11_TEXTURE2D_DESC dd = {};
        t->GetDesc(&dd);
        t->Release();
        w = dd.Width;
        h = dd.Height;
    }
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return false;
    if (!g_res.vmsee_s_tex || !g_res.vmsee_s_rtv || !g_res.vmsee_s_srv || !g_res.vmsee_t_tex || !g_res.vmsee_t_rtv ||
        !g_res.vmsee_t_srv || g_res.vmsee_w != w || g_res.vmsee_h != h) {
        g_res.release_vmsee_targets();
        if (g_res.vmsee_fail_w == w && g_res.vmsee_fail_h == h) return false;   // Refused at this size.
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.vmsee_s_tex)) &&
                  SUCCEEDED(dev->CreateRenderTargetView(g_res.vmsee_s_tex, nullptr, &g_res.vmsee_s_rtv)) &&
                  SUCCEEDED(dev->CreateShaderResourceView(g_res.vmsee_s_tex, nullptr, &g_res.vmsee_s_srv));
        td.Format = DXGI_FORMAT_R8_UNORM;
        ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.vmsee_t_tex)) &&
             SUCCEEDED(dev->CreateRenderTargetView(g_res.vmsee_t_tex, nullptr, &g_res.vmsee_t_rtv)) &&
             SUCCEEDED(dev->CreateShaderResourceView(g_res.vmsee_t_tex, nullptr, &g_res.vmsee_t_srv));
        if (!ok || !g_res.vmsee_s_srv || !g_res.vmsee_t_srv || !g_res.vmsee_s_rtv || !g_res.vmsee_t_rtv) {
            g_res.release_vmsee_targets();
            g_res.vmsee_fail_w = w;
            g_res.vmsee_fail_h = h;
            report_error_once_safe("KH RenderIntegration: view model: the see-through targets could not be made (the "
                                   "chain reads the live depth there)");
            return false;
        }
        g_res.vmsee_w = w;
        g_res.vmsee_h = h;
    }
    return true;
}

// VM_SEE - the snapshot's pass shader (PSDepthS0, compiled at the main depth's sample count as
// ensure_depth_resolve_shader's) and the transmittance's blend. Called by vmsee_arm at the scene flush, never
// from a draw hook: compile_shader may compile here or wait on the pool (the targets: vmsee_targets, above).
// Multisampled only (the chain runs past the view model only after an MSAA scene resolve). False = the cycle is
// not armed and the chain reads the live depth. wsee: WSEE's call (wsee_ensure) - the same shader and
// blend, a refusal reported under its own name (the first caller to meet a refusal reports it).
inline bool vmsee_ensure(ID3D11Device* dev, bool wsee = false) {
    const char* const who = wsee ? "KH RenderIntegration: world see-through"
                                            : "KH RenderIntegration: view model";
    if (!dev || !g_res.depth_srv || !g_res.depth_res_identity) return false;
    const UINT sc = g_res.depth_sample_count;
    if (sc < 2) return false;
    if (!g_res.ps_vmsee_s0 || g_res.ps_vmsee_samples != sc) {
        KH_SAFE_RELEASE(g_res.ps_vmsee_s0);
        g_res.ps_vmsee_samples = 0;
        if (g_res.ps_vmsee_fail == sc) return false;   // Refused at this count already.
        const std::string scs = std::to_string(sc);
        const D3D_SHADER_MACRO d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", scs.c_str() }, { nullptr, nullptr } };
        ID3DBlob* b = nullptr;
        const std::string err =
            compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSDepthS0", "ps_5_0", d, &b);
        HRESULT hr = E_FAIL;
        if (err.empty() && b) {
            hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                        &g_res.ps_vmsee_s0);
        }
        if (b) b->Release();
        if (FAILED(hr) || !g_res.ps_vmsee_s0) {
            KH_SAFE_RELEASE(g_res.ps_vmsee_s0);
            g_res.ps_vmsee_fail = sc;
            report_error_once_safe(std::string(who) + ": see-through snapshot PS (the chain reads the live depth "
                                   "there): " + (err.empty() ? hr_str(hr) : err));
            return false;
        }
        g_res.ps_vmsee_samples = sc;
    }
    if (!g_res.bs_vmsee_t) {
        if (g_res.bs_vmsee_failed) return false;
        D3D11_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable = FALSE;
        bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D11_BLEND_ZERO;   // T' = T (1 - a): the draw's own alpha, whatever it writes.
        rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
        rt.DestBlendAlpha = D3D11_BLEND_ONE;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_vmsee_t)) || !g_res.bs_vmsee_t) {
            KH_SAFE_RELEASE(g_res.bs_vmsee_t);
            g_res.bs_vmsee_failed = true;
            report_error_once_safe(std::string(who) + ": the see-through blend state could not be made");
            return false;
        }
    }
    return true;
}

// WSEE - the record's device objects, made once a device where the want is stamped (fx_chain_run, wsee_arm:
// compile_shader here, never inside a draw hook - the capture's makers in a hook create device states only). Every
// WSEE shader of depth_resolve.hlsl: PSWseeOne / OneL / OneZ / OneZL / D3Res (MSAA_DEPTH / SAMPLE_COUNT at the
// live count), PSPsbZb (PSB_ZB_DEFS) and the chain's copy (PS_WSEE_COPY_SRC); PSWseeAgree is an effect-unit
// build. States: dss_wsee_eq (EQUAL, no write), dss_wsee_nw (LESS, write), bs_wsee_tpre / tpost (ZERO / INV_SRC_ALPHA
// on T's B / A), bs_wsee_sort. The capture's pass and blend are VM_SEE's (PSDepthS0, bs_vmsee_t); the blended
// re-issue's depth state is dss_test (LESS_EQUAL, no write) over depth_dsv_ro. Required: PSWseeOne, the two depth
// states and the two T blend states (a refusal is reported once and latched until the device goes, wsee_v2_failed);
// the rest are non-fatal, each with its own latch below. False at FSAA 1x (a single-sample depth, vmsee_ensure):
// the hooks' render-thread gate does not take the world's draws there (fsaa_world_standdown), so nothing would
// capture.
inline bool wsee_ensure(ID3D11Device* dev) {
    if (!vmsee_ensure(dev, true) || !g_res.dss_test || !g_res.depth_dsv_ro) return false;
    if (!g_res.ps_wsee_one || !g_res.dss_wsee_nw || !g_res.dss_wsee_eq || !g_res.bs_wsee_tpre ||
        !g_res.bs_wsee_tpost) {
        if (g_res.wsee_v2_failed) return false;
        std::string err;
        if (!g_res.ps_wsee_one) {
            const std::string scs = std::to_string(g_res.depth_sample_count);
            const D3D_SHADER_MACRO d[] = {
                { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", scs.c_str() }, { nullptr, nullptr } };
            ID3DBlob* b = nullptr;
            err = compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSWseeOne", "ps_5_0", d,
                                 &b);
            HRESULT hr = E_FAIL;
            if (err.empty() && b) {
                hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                            &g_res.ps_wsee_one);
            }
            if (b) b->Release();
            if (err.empty() && (FAILED(hr) || !g_res.ps_wsee_one)) err = "PSWseeOne " + hr_str(hr);
        }
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = TRUE;
        dd.StencilEnable = FALSE;
        if (err.empty() && !g_res.dss_wsee_nw) {
            dd.DepthFunc = D3D11_COMPARISON_LESS;
            dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            const HRESULT hr = dev->CreateDepthStencilState(&dd, &g_res.dss_wsee_nw);
            if (FAILED(hr) || !g_res.dss_wsee_nw) err = "the no-write layers' depth state " + hr_str(hr);
        }
        if (err.empty() && !g_res.dss_wsee_eq) {
            dd.DepthFunc = D3D11_COMPARISON_EQUAL;
            dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            const HRESULT hr = dev->CreateDepthStencilState(&dd, &g_res.dss_wsee_eq);
            if (FAILED(hr) || !g_res.dss_wsee_eq) err = "the late-opaque depth state " + hr_str(hr);
        }
        // The particle-type T passes: Tpre into T's B, Tpost into T's A (ZERO / INV_SRC_ALPHA, alpha the same).
        D3D11_BLEND_DESC bd = {};
        D3D11_RENDER_TARGET_BLEND_DESC& br = bd.RenderTarget[0];
        br.BlendEnable = TRUE;
        br.SrcBlend = D3D11_BLEND_ZERO;
        br.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        br.BlendOp = D3D11_BLEND_OP_ADD;
        br.SrcBlendAlpha = D3D11_BLEND_ZERO;
        br.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        br.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        if (err.empty() && !g_res.bs_wsee_tpre) {
            br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
            const HRESULT hr = dev->CreateBlendState(&bd, &g_res.bs_wsee_tpre);
            if (FAILED(hr) || !g_res.bs_wsee_tpre) err = "the Tpre blend state " + hr_str(hr);
        }
        if (err.empty() && !g_res.bs_wsee_tpost) {
            br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
            const HRESULT hr = dev->CreateBlendState(&bd, &g_res.bs_wsee_tpost);
            if (FAILED(hr) || !g_res.bs_wsee_tpost) err = "the Tpost blend state " + hr_str(hr);
        }
        if (!err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one);
            KH_SAFE_RELEASE(g_res.dss_wsee_nw);
            KH_SAFE_RELEASE(g_res.dss_wsee_eq);
            KH_SAFE_RELEASE(g_res.bs_wsee_tpre);
            KH_SAFE_RELEASE(g_res.bs_wsee_tpost);
            g_res.wsee_v2_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: " + err +
                                   " (the chain reads the live depth through blended surfaces)");
            return false;
        }
    }
    // The layer-depth split's MRT twin (depth_resolve.hlsl PSWseeOneL: T and Tp at once); wsee_ld_ensure, inside a
    // draw hook, makes its states only. Non-fatal: a refusal latches the split off (wsee_ld_failed, as its states'
    // refusal does; particles keep one depth a phase), reported once.
    if (!g_res.ps_wsee_one_l && !g_res.wsee_ld_failed) {
        std::string err;
        const std::string scs = std::to_string(g_res.depth_sample_count);
        const D3D_SHADER_MACRO d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", scs.c_str() }, { nullptr, nullptr } };
        const char* const e[1] = { "PSWseeOneL" };
        ID3D11PixelShader** const p[1] = { &g_res.ps_wsee_one_l };
        for (int i = 0; i < 1 && err.empty(); ++i) {
            if (*p[i]) continue;
            ID3DBlob* b = nullptr;
            err = compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), e[i], "ps_5_0", d,
                                 &b);
            HRESULT hr = E_FAIL;
            if (err.empty() && b) {
                hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                            p[i]);
            }
            if (b) b->Release();
            if (err.empty() && (FAILED(hr) || !*p[i])) {
                err = std::string(e[i]) + " " + hr_str(hr);
            }
        }
        if (!err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one_l);
            g_res.wsee_ld_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth split could not be made "
                                   "(" + err + "; particles keep one depth a phase)");
        }
    }
    // WSEE_OQ2: the post twins that write D3m and D3's resolve (depth_resolve.hlsl PSWseeOneZ / PSWseeOneZL /
    // PSWseeD3Res). Non-fatal: a refusal latches every post replay onto the store path (ps_wsee_oq2_failed) until the
    // device goes, reported once.
    if ((!g_res.ps_wsee_one_z || !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) && !g_res.ps_wsee_oq2_failed) {
        std::string err;
        const std::string scs = std::to_string(g_res.depth_sample_count);
        const D3D_SHADER_MACRO d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", scs.c_str() }, { nullptr, nullptr } };
        const char* const e[3] = { "PSWseeOneZ", "PSWseeOneZL", "PSWseeD3Res" };
        ID3D11PixelShader** const p[3] = { &g_res.ps_wsee_one_z, &g_res.ps_wsee_one_zl, &g_res.ps_wsee_d3_res };
        for (int i = 0; i < 3 && err.empty(); ++i) {
            if (*p[i]) continue;
            ID3DBlob* b = nullptr;
            err = compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), e[i], "ps_5_0", d,
                                 &b);
            HRESULT hr = E_FAIL;
            if (err.empty() && b) {
                hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                            p[i]);
            }
            if (b) b->Release();
            if (err.empty() && (FAILED(hr) || !*p[i])) {
                err = std::string(e[i]) + " " + hr_str(hr);
            }
        }
        if (!err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one_z);
            KH_SAFE_RELEASE(g_res.ps_wsee_one_zl);
            KH_SAFE_RELEASE(g_res.ps_wsee_d3_res);
            g_res.ps_wsee_oq2_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the late opaques' depth target pass "
                                   "could not be made (" + err + "; every late opaque after the first "
                                   "fence-type layer stores its depth one by one, as before)");
        }
    }
    // PSB_ZB: the one pass's block bound (depth_resolve.hlsl PSPsbZb at log2 s = PSB_LS, PSB_ZB_DEFS; the
    // capture draws it inside a draw hook). Non-fatal: a refusal latches Zb off (psb_zb_failed: the one pass's loop
    // reads without its bound) until the device goes, reported once.
    if (!g_res.ps_psb_zb && !g_res.psb_zb_failed) {
        std::string err;
        {
            ID3DBlob* b = nullptr;
            err = compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSPsbZb", "ps_5_0",
                                 PSB_ZB_DEFS, &b);
            HRESULT hr = E_FAIL;
            if (err.empty() && b) {
                hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                            &g_res.ps_psb_zb);
            }
            if (b) b->Release();
            if (err.empty() && (FAILED(hr) || !g_res.ps_psb_zb)) {
                err = "PSPsbZb " + hr_str(hr);
            }
        }
        if (!err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_psb_zb);
            g_res.psb_zb_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the one-pass record's block bound PS "
                                   "could not be made (" + err + "; its depth reads as before)");
        }
    }
    if (!g_res.ps_wsee_copy && !g_res.ps_wsee_copy_failed) {
        ID3DBlob* cb = nullptr;
        const std::string cerr = compile_shader(PS_WSEE_COPY_SRC, "main", "ps_5_0", nullptr, &cb);
        HRESULT chr = E_FAIL;
        if (cerr.empty() && cb) {
            chr = dev->CreatePixelShader(cb->GetBufferPointer(), cb->GetBufferSize(), nullptr,
                                         &g_res.ps_wsee_copy);
        }
        if (cb) cb->Release();
        if (FAILED(chr) || !g_res.ps_wsee_copy) {
            KH_SAFE_RELEASE(g_res.ps_wsee_copy);
            g_res.ps_wsee_copy_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the chain's copy PS (a last pass that "
                                   "reads the scene's samples takes the approximation): " +
                                   (cerr.empty() ? hr_str(chr) : cerr));
        }
    }
    // The sorted tail's layer-aware composite: out.rgb = o0.rgb + dest.rgb x o1.rgb, out.a = o0.a + dest.a x o1.a
    // (dual source: PSMain's SV_Target0 / SV_Target1, cb.hlsl KhWseeMeshOut). With o0 = (a c, a) and o1 = 1 - a it is
    // the normal blend (blend_modes[0]) exactly. Non-fatal: absent, the sorted tail keeps the plain blend (mode 1)
    // and the injection defers nothing.
    if (!g_res.bs_wsee_sort && !g_res.bs_wsee_sort_failed) {
        D3D11_BLEND_DESC bd = {};
        bd.AlphaToCoverageEnable = FALSE;
        bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC& rt = bd.RenderTarget[0];
        rt.BlendEnable = TRUE;
        rt.SrcBlend = D3D11_BLEND_ONE;
        rt.DestBlend = D3D11_BLEND_SRC1_COLOR;
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ONE;
        rt.DestBlendAlpha = D3D11_BLEND_SRC1_ALPHA;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_wsee_sort)) || !g_res.bs_wsee_sort) {
            KH_SAFE_RELEASE(g_res.bs_wsee_sort);
            g_res.bs_wsee_sort_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the sorted tail's blend state could not "
                                   "be made (our translucent parts take the plain blend under the game's layers)");
        }
    }
    return true;
}

// WSEE - the capture's targets (Resources' note: S, S2, D3, T, B, B2, NW, NW2), keyed by the texture depth_srv
// views (the main depth; the capture has checked the bound depth is that texture) and the scene target's view format
// (fmt, B's). Made by the capture itself (VMSEE_LAZY's rule); a changed key remakes them. WSEE_FAIL_RETRY:
// a refused key is asked again once WSEE_HOLD_FAIL_RETRY cycles after the refusal (one Create* set per that many
// cycles while it keeps failing, reported once), a success at it clears the latch. The chain's own (E, C) have keys
// of their own and stay.
inline bool wsee_targets(ID3D11Device* dev, DXGI_FORMAT fmt) {
    if (!dev || !g_res.depth_srv || fmt == DXGI_FORMAT_UNKNOWN) return false;
    D3D11_TEXTURE2D_DESC dd = {};
    {
        ID3D11Resource* r = nullptr;
        g_res.depth_srv->GetResource(&r);
        ID3D11Texture2D* t = nullptr;
        if (r) {
            if (FAILED(r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t)))) {
                t = nullptr;
            }
            r->Release();
        }
        if (!t) return false;
        t->GetDesc(&dd);
        t->Release();
    }
    const UINT w = dd.Width, h = dd.Height;
    const UINT s = dd.SampleDesc.Count, q = dd.SampleDesc.Quality;
    if (w == 0 || h == 0 || w > 16384 || h > 16384 || s < 2) return false;
    if (g_res.wsee_s_tex && g_res.wsee_s_rtv && g_res.wsee_s_srv && g_res.wsee_t_tex && g_res.wsee_t_rtv &&
        g_res.wsee_t_srv && g_res.wsee_b_tex && g_res.wsee_b_srv && g_res.wsee_n_tex && g_res.wsee_n_dsv &&
        g_res.wsee_n_srv && g_res.wsee_b2_srv && g_res.wsee_n2_dsv && g_res.wsee_n2_srv &&
        g_res.wsee_s2_rtv && g_res.wsee_s2_srv &&
        g_res.wsee_d3_uav && g_res.wsee_d3_srv &&
        g_res.wsee_w == w && g_res.wsee_h == h && g_res.wsee_samples == s &&
        g_res.wsee_quality == q && g_res.wsee_b_fmt == fmt) return true;
    g_res.release_wsee_capture();
    const bool latched = g_res.wsee_fail_w == w && g_res.wsee_fail_h == h &&   // Refused at this key.
                         g_res.wsee_fail_s == s && g_res.wsee_fail_q == q &&
                         g_res.wsee_fail_f == fmt;
    if (latched && g_topo_cycles >= g_res.wsee_fail_cycle &&   // WSEE_FAIL_RETRY (a count that started
        g_topo_cycles - g_res.wsee_fail_cycle < WSEE_HOLD_FAIL_RETRY) return false;   // over: ask again).
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_s_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_s_tex, nullptr, &g_res.wsee_s_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_s_tex, nullptr, &g_res.wsee_s_srv));
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_s2_tex)) &&
         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_s2_tex, nullptr, &g_res.wsee_s2_rtv)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_s2_tex, nullptr, &g_res.wsee_s2_srv));
    // D3: S's desc with a UAV (the late opaque's store path writes it) and the RTV bind for D3's resolve from D3m
    // (WSEE_OQ2; the RTV itself is wsee_d3m_targets', made with D3m).
    td.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_d3_tex)) &&
         SUCCEEDED(dev->CreateUnorderedAccessView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_uav)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_srv));
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    // T: R Tw (fence-type), G the late-opaque flag, B Tpre and A Tpost (particle-type, before / after B2).
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = s;   // The depth's: bound with depth_dsv_ro (Texture2DMS views).
    td.SampleDesc.Quality = q;
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_t_tex)) &&
         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_t_tex, nullptr, &g_res.wsee_t_rtv)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_t_tex, nullptr, &g_res.wsee_t_srv));
    td.Format = fmt;
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;   // A resolve destination (ensure_scene_capture_from's).
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_b_tex)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_b_tex, nullptr, &g_res.wsee_b_srv));
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_b2_tex)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_b2_tex, nullptr, &g_res.wsee_b2_srv));
    // NW / NW2 - single sample (one depth a pixel; bound alone, without an RTV, beside the engine's multisampled
    // rasterizer state), a typeless texture for its two views.
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_n_tex)) &&
         SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_n2_tex));
    if (ok) {
        D3D11_DEPTH_STENCIL_VIEW_DESC nd = {};
        nd.Format = DXGI_FORMAT_D32_FLOAT;
        nd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        D3D11_SHADER_RESOURCE_VIEW_DESC ns = {};
        ns.Format = DXGI_FORMAT_R32_FLOAT;
        ns.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        ns.Texture2D.MipLevels = 1;
        ok = SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_n_tex, &nd, &g_res.wsee_n_dsv)) &&
             SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_n_tex, &ns, &g_res.wsee_n_srv)) &&
             SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_n2_tex, &nd, &g_res.wsee_n2_dsv)) &&
             SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_n2_tex, &ns, &g_res.wsee_n2_srv));
    }
    if (!ok || !g_res.wsee_s_rtv || !g_res.wsee_s_srv || !g_res.wsee_t_rtv || !g_res.wsee_t_srv ||
        !g_res.wsee_b_srv || !g_res.wsee_n_dsv || !g_res.wsee_n_srv || !g_res.wsee_b2_srv || !g_res.wsee_n2_dsv ||
        !g_res.wsee_n2_srv || !g_res.wsee_s2_rtv || !g_res.wsee_s2_srv ||
        !g_res.wsee_d3_uav || !g_res.wsee_d3_srv) {
        g_res.release_wsee_capture();
        g_res.wsee_fail_w = w;
        g_res.wsee_fail_h = h;
        g_res.wsee_fail_s = s;
        g_res.wsee_fail_q = q;
        g_res.wsee_fail_f = fmt;
        g_res.wsee_fail_cycle = g_topo_cycles;   // WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the capture targets could not be made (the "
                               "chain reads the live depth through blended surfaces)");
        return false;
    }
    if (latched) {   // WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_fail_w = g_res.wsee_fail_h = g_res.wsee_fail_s = g_res.wsee_fail_q = 0;
        g_res.wsee_fail_f = DXGI_FORMAT_UNKNOWN;
        g_res.wsee_fail_cycle = 0u;
    }
    g_res.wsee_w = w;
    g_res.wsee_h = h;
    g_res.wsee_samples = s;
    g_res.wsee_quality = q;
    g_res.wsee_b_fmt = fmt;
    return true;
}

// WSEE - the layer-depth split's states (Resources' note at wsee_tp_tex), inside a draw hook (device Create* only):
// bs_wsee_ldpre / ldpost (ZERO / INV_SRC_ALPHA on red / green: Tp's R / G), bs_wsee_ldpre2 / ldpost2 (blue / alpha:
// Tp's B / A), dss_wsee_far (GREATER, no write). Once a device; a refusal latches the split off (wsee_ld_failed) and
// is reported once. The MRT twin PSWseeOneL is wsee_ensure's: not yet made = no split this cycle, no latch.
inline bool wsee_ld_ensure(ID3D11Device* dev) {
    if (g_res.ps_wsee_one_l && g_res.bs_wsee_ldpre && g_res.bs_wsee_ldpost &&
        g_res.bs_wsee_ldpre2 && g_res.bs_wsee_ldpost2 && g_res.dss_wsee_far) return true;
    if (g_res.wsee_ld_failed || !dev) return false;
    if (!g_res.ps_wsee_one_l) return false;
    std::string err;
    D3D11_BLEND_DESC bd = {};
    D3D11_RENDER_TARGET_BLEND_DESC& br = bd.RenderTarget[0];
    br.BlendEnable = TRUE;
    br.SrcBlend = D3D11_BLEND_ZERO;
    br.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    br.BlendOp = D3D11_BLEND_OP_ADD;
    br.SrcBlendAlpha = D3D11_BLEND_ZERO;
    br.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    br.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    if (err.empty() && !g_res.bs_wsee_ldpre) {
        br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_wsee_ldpre)) || !g_res.bs_wsee_ldpre) {
            err = "the near / far pre blend state";
        }
    }
    if (err.empty() && !g_res.bs_wsee_ldpost) {
        br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_wsee_ldpost)) || !g_res.bs_wsee_ldpost) {
            err = "the near / far post blend state";
        }
    }
    if (err.empty() && !g_res.bs_wsee_ldpre2) {   // The 100 % band's pre channel (B).
        br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_wsee_ldpre2)) || !g_res.bs_wsee_ldpre2) {
            err = "the second near band's pre blend state";
        }
    }
    if (err.empty() && !g_res.bs_wsee_ldpost2) {   // Its post channel (A: the alpha factors).
        br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.bs_wsee_ldpost2)) || !g_res.bs_wsee_ldpost2) {
            err = "the second near band's post blend state";
        }
    }
    if (err.empty() && !g_res.dss_wsee_far) {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = TRUE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dd.DepthFunc = D3D11_COMPARISON_GREATER;
        dd.StencilEnable = FALSE;
        if (FAILED(dev->CreateDepthStencilState(&dd, &g_res.dss_wsee_far)) || !g_res.dss_wsee_far) {
            err = "the far pass's depth state";
        }
    }
    if (!err.empty()) {
        KH_SAFE_RELEASE(g_res.ps_wsee_one_l);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpre);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpost);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpre2);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpost2);
        KH_SAFE_RELEASE(g_res.dss_wsee_far);
        g_res.wsee_ld_failed = true;
        report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth split could not be made (" +
                               err + "; particles keep one depth a phase)");
        return false;
    }
    return true;
}

// WSEE - Tp, Tf and Tf's depth (Resources' note at wsee_tp_tex), at T's key (wsee_targets made it first);
// remade at a key change. A refusal is reported once and latched at its key (wsee_ld_fail_*: the capture targets'
// WSEE_FAIL_RETRY rule - another key tries at once, this one again WSEE_HOLD_FAIL_RETRY cycles later), the
// split off meanwhile (no Tp: wsee_ld_on).
inline bool wsee_ld_targets(ID3D11Device* dev) {
    if (!dev || g_res.wsee_ld_failed || !g_res.wsee_t_tex || g_res.wsee_w == 0 || g_res.wsee_h == 0) return false;
    if (g_res.wsee_tp_rtv && g_res.wsee_tp_srv && g_res.wsee_tf_rtv && g_res.wsee_tf_srv && g_res.wsee_tfd_dsv &&
        g_res.wsee_ld_w == g_res.wsee_w && g_res.wsee_ld_h == g_res.wsee_h && g_res.wsee_ld_s == g_res.wsee_samples &&
        g_res.wsee_ld_q == g_res.wsee_quality) return true;
    g_res.release_wsee_ld();
    const bool latched = g_res.wsee_ld_fail_w == g_res.wsee_w && g_res.wsee_ld_fail_h == g_res.wsee_h &&
                         g_res.wsee_ld_fail_s == g_res.wsee_samples &&
                         g_res.wsee_ld_fail_q == g_res.wsee_quality;   // Refused at this key.
    if (latched && g_topo_cycles >= g_res.wsee_ld_fail_cycle &&   // WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_ld_fail_cycle < WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_res.wsee_w;
    td.Height = g_res.wsee_h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    // Tp carries two bands: R / G the 25 % band's pre / post, B / A the 100 % band's; Tf is R8G8 (pre / post).
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = g_res.wsee_samples;   // T's: bound beside depth_dsv_ro (Texture2DMS views).
    td.SampleDesc.Quality = g_res.wsee_quality;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_tp_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_tp_tex, nullptr, &g_res.wsee_tp_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_tp_tex, nullptr, &g_res.wsee_tp_srv));
    td.Format = DXGI_FORMAT_R8G8_UNORM;
    td.SampleDesc.Count = 1;   // Tf: one value a pixel (its own depth target, single sample).
    td.SampleDesc.Quality = 0;
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_tf_tex)) &&
         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_tf_tex, nullptr, &g_res.wsee_tf_rtv)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_tf_tex, nullptr, &g_res.wsee_tf_srv));
    td.Format = DXGI_FORMAT_D32_FLOAT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_tfd_tex)) &&
         SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_tfd_tex, nullptr, &g_res.wsee_tfd_dsv));
    if (!ok || !g_res.wsee_tp_rtv || !g_res.wsee_tp_srv || !g_res.wsee_tf_rtv || !g_res.wsee_tf_srv ||
        !g_res.wsee_tfd_dsv) {
        g_res.release_wsee_ld();
        g_res.wsee_ld_fail_w = g_res.wsee_w;
        g_res.wsee_ld_fail_h = g_res.wsee_h;
        g_res.wsee_ld_fail_s = g_res.wsee_samples;
        g_res.wsee_ld_fail_q = g_res.wsee_quality;
        g_res.wsee_ld_fail_cycle = g_topo_cycles;   // WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth targets could not be made "
                               "(particles keep one depth a phase)");
        return false;
    }
    if (latched) {   // WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_ld_fail_w = g_res.wsee_ld_fail_h = g_res.wsee_ld_fail_s = g_res.wsee_ld_fail_q = 0;
        g_res.wsee_ld_fail_cycle = 0u;
    }
    g_res.wsee_ld_w = g_res.wsee_w;
    g_res.wsee_ld_h = g_res.wsee_h;
    g_res.wsee_ld_s = g_res.wsee_samples;
    g_res.wsee_ld_q = g_res.wsee_quality;
    return true;
}

// PSB: the one pass's states and constant buffers (Resources' PSB note), made once a device by the capture
// (device Create* alone, no compile); refused - N's format (R32_FLOAT) cannot blend (CheckFormatSupport's BLENDABLE)
// or a make fails - latches the one pass off until the device goes (psb_failed): every re-issue keeps today's passes.
inline bool psb_ensure(ID3D11Device* dev) {
    if (!dev || g_res.psb_failed) return false;
    if (g_res.bs_psb[1] && g_res.psb_cb[7]) return true;   // PSB: two states, eight buffers.
    UINT fs = 0u;
    bool ok = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_R32_FLOAT, &fs)) &&   // PSB_PHASE.
              (fs & D3D11_FORMAT_SUPPORT_BLENDABLE) != 0u &&
              (fs & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0u;
    for (int i = 0; i < 2 && ok; ++i) {
        if (g_res.bs_psb[i]) continue;
        // PSB_PHASE: the phase's own pair bound (psb_post), so no phase mask - [0] the layer-depth split
        // standing: P (RT0) takes all four channels as T's particle passes take theirs (dest *= 1 - src: ZERO /
        // INV_SRC_COLOR, the alpha's ZERO / INV_SRC_ALPHA); [1] the split off: R and G alone (T and the 25 % band; B
        // and A keep their clear); N (RT1) the phase's nearest by MIN. No N-alone state: a light-adding draw keeps
        // its NW pass.
        const bool ld = i == 0;
        D3D11_BLEND_DESC d = {};
        d.IndependentBlendEnable = TRUE;
        {
            D3D11_RENDER_TARGET_BLEND_DESC& t = d.RenderTarget[0];
            t.BlendEnable = TRUE;
            t.SrcBlend = D3D11_BLEND_ZERO;
            t.DestBlend = D3D11_BLEND_INV_SRC_COLOR;
            t.BlendOp = D3D11_BLEND_OP_ADD;
            t.SrcBlendAlpha = D3D11_BLEND_ZERO;
            t.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            t.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            t.RenderTargetWriteMask = static_cast<UINT8>(ld ?
                static_cast<UINT>(D3D11_COLOR_WRITE_ENABLE_ALL) :
                static_cast<UINT>(D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN));
        }
        D3D11_RENDER_TARGET_BLEND_DESC& n = d.RenderTarget[1];
        n.BlendEnable = TRUE;
        n.SrcBlend = D3D11_BLEND_ONE;
        n.DestBlend = D3D11_BLEND_ONE;
        n.BlendOp = D3D11_BLEND_OP_MIN;
        n.SrcBlendAlpha = D3D11_BLEND_ONE;
        n.DestBlendAlpha = D3D11_BLEND_ONE;
        n.BlendOpAlpha = D3D11_BLEND_OP_MIN;
        n.RenderTargetWriteMask = static_cast<UINT8>(D3D11_COLOR_WRITE_ENABLE_RED);
        ok = SUCCEEDED(dev->CreateBlendState(&d, &g_res.bs_psb[i])) && g_res.bs_psb[i];
    }
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = 64u;   // cb[0] (m_1, m_2, m_3, Dq), cb[1] (a_1, a_2, a_3, D), cb[2] the masks and the count,
                               // PSB_LORES: cb[3] the block's addressing and the scale (psb_post).
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    for (int i = 0; i < 8 && ok; ++i) {
        if (g_res.psb_cb[i]) continue;
        ok = SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_res.psb_cb[i])) && g_res.psb_cb[i];
        g_res.psb_cb_set[i] = false;
    }
    if (!ok) {
        for (ID3D11BlendState*& b : g_res.bs_psb) KH_SAFE_RELEASE(b);
        for (int i = 0; i < 8; ++i) {
            KH_SAFE_RELEASE(g_res.psb_cb[i]);
            g_res.psb_cb_set[i] = false;
        }
        g_res.psb_failed = true;
        report_error_once_safe("KH RenderIntegration: world see-through: the one-pass particle record could not be "
                               "set up (the particle re-draws keep their passes)");
    }
    return ok;
}
// PSB: P0, P1 (RGBA8) and N, N2 (R32_FLOAT, a phase's NW each), single sample at ceil(size / s) a side (s =
// 1 << PSB_LS; PSB_LORES), Zb with them - made by the capture, kept at their key (T's size); a refused make is
// latched at its key (wsee_pb_fail_*, the capture targets' WSEE_FAIL_RETRY rule).
inline bool psb_targets(ID3D11Device* dev) {
    if (!dev || !g_res.wsee_t_tex || g_res.wsee_w == 0 || g_res.wsee_h == 0 || !psb_ensure(dev)) return false;
    if (g_res.wsee_p0_rtv && g_res.wsee_p0_srv && g_res.wsee_p1_rtv && g_res.wsee_p1_srv && g_res.wsee_nb_rtv &&
        g_res.wsee_nb_srv && g_res.wsee_nb2_rtv && g_res.wsee_nb2_srv &&   // PSB_PHASE.
        g_res.wsee_pb_w == g_res.wsee_w && g_res.wsee_pb_h == g_res.wsee_h &&
        (g_res.wsee_zb_srv || g_res.wsee_zb_refused)) return true;   // PSB_ZB.
    g_res.release_wsee_pb();
    const bool latched = g_res.wsee_pb_fail_w == g_res.wsee_w && g_res.wsee_pb_fail_h == g_res.wsee_h;
    if (latched && g_topo_cycles >= g_res.wsee_pb_fail_cycle &&   // WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_pb_fail_cycle < WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = (g_res.wsee_w + (1u << PSB_LS) - 1u) >> PSB_LS;    // PSB_LORES: 1 / s, rounded up.
    td.Height = (g_res.wsee_h + (1u << PSB_LS) - 1u) >> PSB_LS;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_p0_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_p0_tex, nullptr, &g_res.wsee_p0_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_p0_tex, nullptr, &g_res.wsee_p0_srv)) &&
              SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_p1_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_p1_tex, nullptr, &g_res.wsee_p1_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_p1_tex, nullptr, &g_res.wsee_p1_srv));
    td.Format = DXGI_FORMAT_R32_FLOAT;   // PSB_PHASE: a phase's NW each.
    ok = ok && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_nb_tex)) &&
         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_nb_tex, nullptr, &g_res.wsee_nb_rtv)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_nb_tex, nullptr, &g_res.wsee_nb_srv)) &&
         SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_nb2_tex)) &&
         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_nb2_tex, nullptr, &g_res.wsee_nb2_rtv)) &&
         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_nb2_tex, nullptr, &g_res.wsee_nb2_srv));
    if (!ok || !g_res.wsee_p0_rtv || !g_res.wsee_p0_srv || !g_res.wsee_p1_rtv || !g_res.wsee_p1_srv ||
        !g_res.wsee_nb_rtv || !g_res.wsee_nb_srv || !g_res.wsee_nb2_rtv || !g_res.wsee_nb2_srv) {
        g_res.release_wsee_pb();
        g_res.wsee_pb_fail_w = g_res.wsee_w;
        g_res.wsee_pb_fail_h = g_res.wsee_h;
        g_res.wsee_pb_fail_cycle = g_topo_cycles;   // WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the one-pass particle record's targets "
                               "could not be made (the particle re-draws keep their passes)");
        return false;
    }
    if (latched) {   // WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_pb_fail_w = g_res.wsee_pb_fail_h = 0;
        g_res.wsee_pb_fail_cycle = 0u;
    }
    // PSB_ZB: Zb (R32G32_FLOAT, the record's size; a refused make leaves it absent at this key - the one pass reads
    // without its bound, no retry until the targets are released: wsee_zb_refused).
    {
        td.Format = DXGI_FORMAT_R32G32_FLOAT;
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_zb_tex)) ||
            FAILED(dev->CreateRenderTargetView(g_res.wsee_zb_tex, nullptr, &g_res.wsee_zb_rtv)) ||
            FAILED(dev->CreateShaderResourceView(g_res.wsee_zb_tex, nullptr, &g_res.wsee_zb_srv)) ||
            !g_res.wsee_zb_rtv || !g_res.wsee_zb_srv) {
            KH_SAFE_RELEASE(g_res.wsee_zb_srv);
            KH_SAFE_RELEASE(g_res.wsee_zb_rtv);
            KH_SAFE_RELEASE(g_res.wsee_zb_tex);
            g_res.wsee_zb_refused = true;   // PSB_ZB.
            report_error_once_safe("KH RenderIntegration: world see-through: the one-pass record's block bound could "
                                   "not be made (its depth reads as before)");
        }
    }
    g_res.wsee_pb_w = g_res.wsee_w;
    g_res.wsee_pb_h = g_res.wsee_h;
    return true;
}

// WSEE_OQ2 - D3m and D3's RTV (Resources' note at wsee_d3_rtv), at T's key plus the depth view's format (one
// GetDesc a capture); remade at a key change. Only for a 24- or 32-bit depth view (R24_UNORM_X8_TYPELESS, R32_FLOAT,
// R32_FLOAT_X8X24_TYPELESS: a 16-bit step exceeds the resolve's tolerance - the store path, unreported, no call).
// A refusal is reported once and latched at its key (wsee_d3m_fail_*, the capture targets' WSEE_FAIL_RETRY rule).
// Inside a draw hook (the capture): device Create* only, a one-time hitch at the first make; the shaders are
// wsee_ensure's, nothing is made before the three stand, nothing kept once they were refused.
inline bool wsee_d3m_targets(ID3D11Device* dev) {
    if (g_res.ps_wsee_oq2_failed || !g_res.ps_wsee_one_z || !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) {
        if (g_res.wsee_d3m_tex) g_res.release_wsee_d3m();
        return false;
    }
    if (!dev || !g_res.wsee_d3_tex || !g_res.wsee_t_tex || !g_res.depth_srv ||
        g_res.wsee_w == 0 || g_res.wsee_h == 0) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
    g_res.depth_srv->GetDesc(&vd);
    if (g_res.wsee_d3m_rtv && g_res.wsee_d3m_srv && g_res.wsee_d3_rtv && g_res.wsee_d3m_w == g_res.wsee_w &&
        g_res.wsee_d3m_h == g_res.wsee_h && g_res.wsee_d3m_s == g_res.wsee_samples &&
        g_res.wsee_d3m_q == g_res.wsee_quality && g_res.wsee_d3m_dfmt == vd.Format) return true;
    g_res.release_wsee_d3m();
    const bool latched = g_res.wsee_d3m_fail_w == g_res.wsee_w && g_res.wsee_d3m_fail_h == g_res.wsee_h &&
                         g_res.wsee_d3m_fail_s == g_res.wsee_samples &&
                         g_res.wsee_d3m_fail_q == g_res.wsee_quality &&
                         g_res.wsee_d3m_fail_f == vd.Format;   // Refused at this key.
    if (latched && g_topo_cycles >= g_res.wsee_d3m_fail_cycle &&   // WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_d3m_fail_cycle < WSEE_HOLD_FAIL_RETRY) return false;
    const bool fmt = vd.Format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS ||
                     vd.Format == DXGI_FORMAT_R32_FLOAT ||
                     vd.Format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_res.wsee_w;
    td.Height = g_res.wsee_h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;
    td.SampleDesc.Count = g_res.wsee_samples;   // T's: bound beside T and depth_dsv_ro (a Texture2DMS view).
    td.SampleDesc.Quality = g_res.wsee_quality;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    const bool ok = fmt && SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.wsee_d3m_tex)) &&
                    SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_d3m_tex, nullptr, &g_res.wsee_d3m_rtv)) &&
                    SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_d3m_tex, nullptr, &g_res.wsee_d3m_srv)) &&
                    SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_rtv));
    if (!ok || !g_res.wsee_d3m_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_d3_rtv) {
        g_res.release_wsee_d3m();
        g_res.wsee_d3m_fail_w = g_res.wsee_w;
        g_res.wsee_d3m_fail_h = g_res.wsee_h;
        g_res.wsee_d3m_fail_s = g_res.wsee_samples;
        g_res.wsee_d3m_fail_q = g_res.wsee_quality;
        g_res.wsee_d3m_fail_f = vd.Format;
        g_res.wsee_d3m_fail_cycle = g_topo_cycles;   // WSEE_FAIL_RETRY.
        if (fmt) {
            report_error_once_safe("KH RenderIntegration: world see-through: the late opaques' depth target could "
                                   "not be made (every late opaque after the first fence-type layer stores its depth "
                                   "one by one, as before)");
        }
        return false;
    }
    if (latched) {   // WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_d3m_fail_w = g_res.wsee_d3m_fail_h = g_res.wsee_d3m_fail_s = g_res.wsee_d3m_fail_q = 0;
        g_res.wsee_d3m_fail_f = DXGI_FORMAT_UNKNOWN;
        g_res.wsee_d3m_fail_cycle = 0u;
    }
    g_res.wsee_d3m_w = g_res.wsee_w;
    g_res.wsee_d3m_h = g_res.wsee_h;
    g_res.wsee_d3m_s = g_res.wsee_samples;
    g_res.wsee_d3m_q = g_res.wsee_quality;
    g_res.wsee_d3m_dfmt = vd.Format;
    return true;
}

// Called by the flush, with the main scene depth verified bound. "" on success (comp_depth_srv then holds this frame's
// scene depth); the whole target is resolved.
inline std::string snapshot_composite_depth(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    // Live depth as SRV (identity-cached; also feeds the compute queries).
    UINT dw = 0, dh = 0;
    std::string err = ensure_depth_srv(dev, ctx, &dw, &dh);
    if (!err.empty()) return err;
    err = ensure_depth_resolve_shader(dev);
    if (!err.empty()) return err;
    if (!g_res.vs_fullscreen || !g_res.dss_off || !g_res.rasterizer) return "core resources missing";

    const bool matches = g_res.comp_depth_tex && g_res.snap_rtv && g_res.comp_depth_srv &&
                         g_res.comp_depth_w == dw && g_res.comp_depth_h == dh &&
                         g_res.comp_depth_samples == 1;

    if (!matches) {
        KH_SAFE_RELEASE(g_res.snap_rtv);
        KH_SAFE_RELEASE(g_res.comp_depth_srv);
        KH_SAFE_RELEASE(g_res.comp_depth_tex);
        D3D11_TEXTURE2D_DESC cd = {};
        cd.Width = dw;
        cd.Height = dh;
        cd.MipLevels = 1;
        cd.ArraySize = 1;
        // Two-plane (.x farthest, .y nearest): every reader takes .x; the .y is written (PSDepthResolve) and read by
        // none (KhCastZPick reads the engine's depth). Always single-sample, whatever the main depth's count: the
        // resolve (PSDepthResolve, compiled at the live count) folds the samples to the farthest plane in .x, so a
        // guard discards only where every sample lies nearer than the fragment (a pixel any sample leaves uncovered
        // reads as clear and keeps it). The injection's composite draws keep a hardware depth test against the live
        // main depth (the injection never rebinds the OM: it draws into the DSV the trigger verified), so their
        // per-sample cut happens there. The flush's readers draw with dss_off - no hardware test: the guard of a
        // DepthMode::Off solid (PSMain, or PSComposite's ARB twin when the solid takes the near-gap route) and an
        // effect mesh's far-frame arbitration (khArbSnap, t2). On a pixel a nearer surface covers in part, .x reads the
        // farther sample and the fragment writes every sample it covers: such a draw paints over up to a 1 px rim along
        // the edges of what stands in front of it (MSAA only - world meshes stand down at 1x). Accepted as is: the
        // unread .y (nearest) would trade that rim for a 1 px gap in the draw instead.
        cd.Format = DXGI_FORMAT_R32G32_FLOAT;
        cd.SampleDesc.Count = 1;
        cd.Usage = D3D11_USAGE_DEFAULT;
        cd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = dev->CreateTexture2D(&cd, nullptr, &g_res.comp_depth_tex);
        if (FAILED(hr)) return "create snapshot tex " + hr_str(hr);
        hr = dev->CreateRenderTargetView(g_res.comp_depth_tex, nullptr, &g_res.snap_rtv);

        if (FAILED(hr)) {
            KH_SAFE_RELEASE(g_res.comp_depth_tex);
            return "create snapshot RTV " + hr_str(hr);
        }

        hr = dev->CreateShaderResourceView(g_res.comp_depth_tex, nullptr, &g_res.comp_depth_srv);

        if (FAILED(hr)) {
            KH_SAFE_RELEASE(g_res.snap_rtv);
            KH_SAFE_RELEASE(g_res.comp_depth_tex);
            return "create snapshot SRV " + hr_str(hr);
        }

        g_res.comp_depth_w = dw;
        g_res.comp_depth_h = dh;
        g_res.comp_depth_samples = 1;   // Guard shader compiles single-sample from here on.
    }

    // Replacing the OM targets first unbinds the live depth DSV, which makes its SRV legally bindable at t0.
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<FLOAT>(dw);
    vp.Height = static_cast<FLOAT>(dh);
    vp.MaxDepth = 1.0f;

    StateBackup backup;
    backup.capture(ctx);
    OmSave om;
    om.capture(ctx);
    UINT n_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&n_vp, saved_vp);

    ctx->OMSetRenderTargets(1, &g_res.snap_rtv, nullptr);
    ctx->RSSetViewports(1, &vp);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_depth_resolve, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ctx->Draw(3, 0);

    // Unbind t0 before the engine rebinds the live depth as a DSV (a lingering
    // SRV would be force-nulled with a debug-layer warning storm).
    ID3D11ShaderResourceView* null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &null_srv);

    om.restore(ctx);
    om.release();
    if (n_vp > 0) ctx->RSSetViewports(n_vp, saved_vp);
    backup.restore(ctx);

    g_res.comp_depth_time = effect_time_seconds();
    return "";
}
}