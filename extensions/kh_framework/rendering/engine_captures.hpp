#pragma once

// The injection half (rendering_integration_injection.hpp): the engine captures: scene colour, depth, the see-through
// targets and the composite depth snapshot. Never include it on its own.
namespace RenderIntegration {
inline void kh_msaa_toggle_wipe(unsigned int khtw_new);

// Capture core, parameterized on the source RTV: ensure_scene_capture calls it with the bound target (its one caller).
// The caller keeps ownership of the RTV reference. KH_CAP_DEFER: khcap_copy false runs every check and (re)creation -
// the same verdict, the same target - and leaves the copy (a resolve, on an MSAA scene) to a later call.
inline std::string ensure_scene_capture_from(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11RenderTargetView* rtv, bool khcap_copy = true) {
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

    if (khcap_copy) {   // KH_CAP_DEFER: false leaves the copy to a later call.
        if (src_desc.SampleDesc.Count > 1) {
            ctx->ResolveSubresource(g_res.scene_tex, 0, src_tex, 0, capture_fmt);
        } else {
            ctx->CopyResource(g_res.scene_tex, src_tex);
        }
    }

    src_tex->Release();
    return "";
}

inline std::string ensure_scene_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool khcap_copy = true) {
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return "no RTV bound";
    const std::string err = ensure_scene_capture_from(dev, ctx, rtv, khcap_copy);
    rtv->Release();
    return err;
}

// flush_locked's call sites and kh_fx_chain_run's (only flush_locked calls it).
inline std::string kh_scene_capture_timed(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_SCENE_CAPTURE);
    KH_GPU_SCOPE(ctx, KHG_SCENE_CAPTURE);   // KH_PROF.
    return ensure_scene_capture(dev, ctx);
}

// KH_BLEND_MINMAX - a lighten / darken mesh's own copy of the bound scene target in g_res.mm_tex (a resolve
// when the target is multisampled; khmc_ms says which): made at the first need, remade on a size or format
// change (ensure_scene_capture_from's rules), released with the device's other targets. scene_tex is not
// touched. Cost: one full-frame CopyResource / ResolveSubresource per such mesh per frame (flush_locked
// only), and one more frame-sized texture once any was drawn. False: no copy (the hardware forms stand).
inline bool kh_mm_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool& khmc_ms) {
    KH_PROF_SCOPE(KHP_SCENE_CAPTURE);
    KH_GPU_SCOPE(ctx, KHG_SCENE_CAPTURE);   // KH_PROF: counted with the scene captures.
    khmc_ms = false;
    ID3D11RenderTargetView* khmc_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khmc_rtv, nullptr);
    if (!khmc_rtv) return false;
    D3D11_RENDER_TARGET_VIEW_DESC khmc_vd = {};
    khmc_rtv->GetDesc(&khmc_vd);
    ID3D11Resource* khmc_res = nullptr;
    khmc_rtv->GetResource(&khmc_res);
    khmc_rtv->Release();
    if (!khmc_res) return false;
    ID3D11Texture2D* khmc_src = nullptr;
    const HRESULT khmc_hr = khmc_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khmc_src));
    khmc_res->Release();
    if (FAILED(khmc_hr) || !khmc_src) return false;
    D3D11_TEXTURE2D_DESC khmc_sd = {};
    khmc_src->GetDesc(&khmc_sd);
    const DXGI_FORMAT khmc_fmt = khmc_vd.Format;   // The view's typed format (a resolve needs one).
    if (!g_res.mm_tex || g_res.mm_w != khmc_sd.Width || g_res.mm_h != khmc_sd.Height || g_res.mm_fmt != khmc_fmt) {
        g_res.release_mm_capture();
        D3D11_TEXTURE2D_DESC khmc_td = {};
        khmc_td.Width = khmc_sd.Width;
        khmc_td.Height = khmc_sd.Height;
        khmc_td.MipLevels = 1;
        khmc_td.ArraySize = 1;
        khmc_td.Format = khmc_fmt;
        khmc_td.SampleDesc.Count = 1;
        khmc_td.Usage = D3D11_USAGE_DEFAULT;
        khmc_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&khmc_td, nullptr, &g_res.mm_tex)) || !g_res.mm_tex ||
            FAILED(dev->CreateShaderResourceView(g_res.mm_tex, nullptr, &g_res.mm_srv)) || !g_res.mm_srv) {
            g_res.release_mm_capture();
            khmc_src->Release();
            return false;
        }
        g_res.mm_w = khmc_sd.Width;
        g_res.mm_h = khmc_sd.Height;
        g_res.mm_fmt = khmc_fmt;
    }
    khmc_ms = khmc_sd.SampleDesc.Count > 1;
    if (khmc_ms) ctx->ResolveSubresource(g_res.mm_tex, 0, khmc_src, 0, khmc_fmt);
    else         ctx->CopyResource(g_res.mm_tex, khmc_src);
    khmc_src->Release();
    return true;
}

// KH_BLEND_MINMAX - one lighten / darken mesh's t3 (its own copy), put back as it was at the scope's end
// (KhGlowBind's rule).
struct KhMmBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    KhMmBind() = default;
    KhMmBind(const KhMmBind&) = delete;
    KhMmBind& operator=(const KhMmBind&) = delete;
    ~KhMmBind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(3, 1, &t3);
        KH_SAFE_RELEASE(t3);
    }
    bool bind(ID3D11DeviceContext* khmb_ctx, ID3D11ShaderResourceView* khmb_srv) {
        if (ctx || !khmb_ctx || !khmb_srv) return false;
        ctx = khmb_ctx;
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSSetShaderResources(3, 1, &khmb_srv);
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
    kh_msaa_toggle_wipe(td.SampleDesc.Count);

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
    {   // KH_SSAO_MARK: the stencil plane, where the format carries one (non-fatal).
        DXGI_FORMAT khst_fmt = DXGI_FORMAT_UNKNOWN;
        if (td.Format == DXGI_FORMAT_R24G8_TYPELESS) khst_fmt = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        else if (td.Format == DXGI_FORMAT_R32G8X24_TYPELESS) khst_fmt = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
        if (khst_fmt != DXGI_FORMAT_UNKNOWN) {
            D3D11_SHADER_RESOURCE_VIEW_DESC khst_sd = sd;
            khst_sd.Format = khst_fmt;
            if (FAILED(dev->CreateShaderResourceView(tex, &khst_sd, &g_res.depth_sten_srv))) g_res.depth_sten_srv = nullptr;
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

// KH_VM_SEE - the snapshot's two targets at the main depth's size. KH_VMSEE_LAZY: made by the snapshot itself (the
// cycle's first view-model translucent draw of an armed cycle), so a scene with no view model - third person, a
// vehicle's 2D optics - makes none; nothing here compiles. Sized by the texture depth_srv views (the main depth,
// this cycle's: the snapshot binds it, having checked the bound depth is that texture) - read off the view, not
// through ensure_depth_srv, whose bound-view rebuild is the flush's own business. A size change remakes them.
inline bool kh_vmsee_targets(ID3D11Device* dev) {
    if (!dev || !g_res.depth_srv) return false;
    UINT khve_w = 0, khve_h = 0;
    {
        ID3D11Resource* khve_r = nullptr;
        g_res.depth_srv->GetResource(&khve_r);
        ID3D11Texture2D* khve_t = nullptr;
        if (khve_r) {
            if (FAILED(khve_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khve_t)))) {
                khve_t = nullptr;
            }
            khve_r->Release();
        }
        if (!khve_t) return false;
        D3D11_TEXTURE2D_DESC khve_dd = {};
        khve_t->GetDesc(&khve_dd);
        khve_t->Release();
        khve_w = khve_dd.Width;
        khve_h = khve_dd.Height;
    }
    if (khve_w == 0 || khve_h == 0 || khve_w > 16384 || khve_h > 16384) return false;
    if (!g_res.vmsee_s_tex || !g_res.vmsee_s_rtv || !g_res.vmsee_s_srv || !g_res.vmsee_t_tex || !g_res.vmsee_t_rtv ||
        !g_res.vmsee_t_srv || g_res.vmsee_w != khve_w || g_res.vmsee_h != khve_h) {
        g_res.release_vmsee_targets();
        if (g_res.vmsee_fail_w == khve_w && g_res.vmsee_fail_h == khve_h) return false;   // Refused at this size.
        D3D11_TEXTURE2D_DESC khve_td = {};
        khve_td.Width = khve_w;
        khve_td.Height = khve_h;
        khve_td.MipLevels = 1;
        khve_td.ArraySize = 1;
        khve_td.Format = DXGI_FORMAT_R32_FLOAT;
        khve_td.SampleDesc.Count = 1;
        khve_td.Usage = D3D11_USAGE_DEFAULT;
        khve_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bool khve_ok = SUCCEEDED(dev->CreateTexture2D(&khve_td, nullptr, &g_res.vmsee_s_tex)) &&
                       SUCCEEDED(dev->CreateRenderTargetView(g_res.vmsee_s_tex, nullptr, &g_res.vmsee_s_rtv)) &&
                       SUCCEEDED(dev->CreateShaderResourceView(g_res.vmsee_s_tex, nullptr, &g_res.vmsee_s_srv));
        khve_td.Format = DXGI_FORMAT_R8_UNORM;
        khve_ok = khve_ok && SUCCEEDED(dev->CreateTexture2D(&khve_td, nullptr, &g_res.vmsee_t_tex)) &&
                  SUCCEEDED(dev->CreateRenderTargetView(g_res.vmsee_t_tex, nullptr, &g_res.vmsee_t_rtv)) &&
                  SUCCEEDED(dev->CreateShaderResourceView(g_res.vmsee_t_tex, nullptr, &g_res.vmsee_t_srv));
        if (!khve_ok || !g_res.vmsee_s_srv || !g_res.vmsee_t_srv || !g_res.vmsee_s_rtv || !g_res.vmsee_t_rtv) {
            g_res.release_vmsee_targets();
            g_res.vmsee_fail_w = khve_w;
            g_res.vmsee_fail_h = khve_h;
            report_error_once_safe("KH RenderIntegration: view model: the see-through targets could not be made (the "
                                   "chain reads the live depth there)");
            return false;
        }
        g_res.vmsee_w = khve_w;
        g_res.vmsee_h = khve_h;
    }
    return true;
}

// KH_VM_SEE - the snapshot's pass shader (PSDepthS0, compiled at the main depth's sample count as
// ensure_depth_resolve_shader's) and the transmittance's blend. Called by kh_vmsee_arm at the scene flush, never
// from a draw hook: compile_shader may compile here or wait on the pool (the targets: kh_vmsee_targets, above).
// Multisampled only (the chain runs past the view model only after an MSAA scene resolve). False = the cycle is
// not armed and the chain reads the live depth. khve_wsee: KH_WSEE's call (kh_wsee_ensure) - the same shader and
// blend, a refusal reported under its own name (the first caller to meet a refusal reports it).
inline bool kh_vmsee_ensure(ID3D11Device* dev, bool khve_wsee = false) {
    const char* const khve_who = khve_wsee ? "KH RenderIntegration: world see-through"
                                            : "KH RenderIntegration: view model";
    if (!dev || !g_res.depth_srv || !g_res.depth_res_identity) return false;
    const UINT khve_sc = g_res.depth_sample_count;
    if (khve_sc < 2) return false;
    if (!g_res.ps_vmsee_s0 || g_res.ps_vmsee_samples != khve_sc) {
        KH_SAFE_RELEASE(g_res.ps_vmsee_s0);
        g_res.ps_vmsee_samples = 0;
        if (g_res.ps_vmsee_fail == khve_sc) return false;   // Refused at this count already.
        const std::string khve_scs = std::to_string(khve_sc);
        const D3D_SHADER_MACRO khve_d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", khve_scs.c_str() }, { nullptr, nullptr } };
        ID3DBlob* khve_b = nullptr;
        const std::string khve_err =
            compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSDepthS0", "ps_5_0", khve_d, &khve_b);
        HRESULT khve_hr = E_FAIL;
        if (khve_err.empty() && khve_b) {
            khve_hr = dev->CreatePixelShader(khve_b->GetBufferPointer(), khve_b->GetBufferSize(), nullptr,
                                             &g_res.ps_vmsee_s0);
        }
        if (khve_b) khve_b->Release();
        if (FAILED(khve_hr) || !g_res.ps_vmsee_s0) {
            KH_SAFE_RELEASE(g_res.ps_vmsee_s0);
            g_res.ps_vmsee_fail = khve_sc;
            report_error_once_safe(std::string(khve_who) + ": see-through snapshot PS (the chain reads the live depth "
                                   "there): " + (khve_err.empty() ? hr_str(khve_hr) : khve_err));
            return false;
        }
        g_res.ps_vmsee_samples = khve_sc;
    }
    if (!g_res.bs_vmsee_t) {
        if (g_res.bs_vmsee_failed) return false;
        D3D11_BLEND_DESC khve_bd = {};
        khve_bd.AlphaToCoverageEnable = FALSE;
        khve_bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC& khve_rt = khve_bd.RenderTarget[0];
        khve_rt.BlendEnable = TRUE;
        khve_rt.SrcBlend = D3D11_BLEND_ZERO;   // T' = T (1 - a): the draw's own alpha, whatever it writes.
        khve_rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        khve_rt.BlendOp = D3D11_BLEND_OP_ADD;
        khve_rt.SrcBlendAlpha = D3D11_BLEND_ZERO;
        khve_rt.DestBlendAlpha = D3D11_BLEND_ONE;
        khve_rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        khve_rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
        if (FAILED(dev->CreateBlendState(&khve_bd, &g_res.bs_vmsee_t)) || !g_res.bs_vmsee_t) {
            KH_SAFE_RELEASE(g_res.bs_vmsee_t);
            g_res.bs_vmsee_failed = true;
            report_error_once_safe(std::string(khve_who) + ": the see-through blend state could not be made");
            return false;
        }
    }
    return true;
}

// KH_WSEE - the record's device objects, made once a device where the want is stamped (kh_fx_chain_run, kh_wsee_arm:
// compile_shader here, never inside a draw hook - the capture's makers in a hook create device states only). Every
// KH_WSEE shader of depth_resolve.hlsl: PSWseeOne / OneL / OneZ / OneZL / D3Res (MSAA_DEPTH / SAMPLE_COUNT at the
// live count), PSPsbZb (KH_PSB_ZB_DEFS) and the chain's copy (KH_PS_WSEE_COPY_SRC); PSWseeAgree is an effect-unit
// build. States: dss_wsee_eq (EQUAL, no write), dss_wsee_nw (LESS, write), bs_wsee_tpre / tpost (ZERO / INV_SRC_ALPHA
// on T's B / A), bs_wsee_sort. The capture's pass and blend are KH_VM_SEE's (PSDepthS0, bs_vmsee_t); the blended
// re-issue's depth state is dss_test (LESS_EQUAL, no write) over depth_dsv_ro. Required: PSWseeOne, the two depth
// states and the two T blend states (a refusal is reported once and latched until the device goes, wsee_v2_failed);
// the rest are non-fatal, each with its own latch below. False at FSAA 1x (a single-sample depth, kh_vmsee_ensure):
// the hooks' render-thread gate does not take the world's draws there (kh_fsaa_world_standdown), so nothing would
// capture.
inline bool kh_wsee_ensure(ID3D11Device* dev) {
    if (!kh_vmsee_ensure(dev, true) || !g_res.dss_test || !g_res.depth_dsv_ro) return false;
    if (!g_res.ps_wsee_one || !g_res.dss_wsee_nw || !g_res.dss_wsee_eq || !g_res.bs_wsee_tpre ||
        !g_res.bs_wsee_tpost) {
        if (g_res.wsee_v2_failed) return false;
        std::string khwe_err;
        if (!g_res.ps_wsee_one) {
            const std::string khwe_scs = std::to_string(g_res.depth_sample_count);
            const D3D_SHADER_MACRO khwe_d[] = {
                { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", khwe_scs.c_str() }, { nullptr, nullptr } };
            ID3DBlob* khwe_b = nullptr;
            khwe_err = compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSWseeOne", "ps_5_0", khwe_d,
                                      &khwe_b);
            HRESULT khwe_hr = E_FAIL;
            if (khwe_err.empty() && khwe_b) {
                khwe_hr = dev->CreatePixelShader(khwe_b->GetBufferPointer(), khwe_b->GetBufferSize(), nullptr,
                                                 &g_res.ps_wsee_one);
            }
            if (khwe_b) khwe_b->Release();
            if (khwe_err.empty() && (FAILED(khwe_hr) || !g_res.ps_wsee_one)) khwe_err = "PSWseeOne " + hr_str(khwe_hr);
        }
        D3D11_DEPTH_STENCIL_DESC khwe_dd = {};
        khwe_dd.DepthEnable = TRUE;
        khwe_dd.StencilEnable = FALSE;
        if (khwe_err.empty() && !g_res.dss_wsee_nw) {
            khwe_dd.DepthFunc = D3D11_COMPARISON_LESS;
            khwe_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
            const HRESULT khwe_hr = dev->CreateDepthStencilState(&khwe_dd, &g_res.dss_wsee_nw);
            if (FAILED(khwe_hr) || !g_res.dss_wsee_nw) khwe_err = "the no-write layers' depth state " + hr_str(khwe_hr);
        }
        if (khwe_err.empty() && !g_res.dss_wsee_eq) {
            khwe_dd.DepthFunc = D3D11_COMPARISON_EQUAL;
            khwe_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
            const HRESULT khwe_hr = dev->CreateDepthStencilState(&khwe_dd, &g_res.dss_wsee_eq);
            if (FAILED(khwe_hr) || !g_res.dss_wsee_eq) khwe_err = "the late-opaque depth state " + hr_str(khwe_hr);
        }
        // The particle-type T passes: Tpre into T's B, Tpost into T's A (ZERO / INV_SRC_ALPHA, alpha the same).
        D3D11_BLEND_DESC khwe_bd = {};
        D3D11_RENDER_TARGET_BLEND_DESC& khwe_br = khwe_bd.RenderTarget[0];
        khwe_br.BlendEnable = TRUE;
        khwe_br.SrcBlend = D3D11_BLEND_ZERO;
        khwe_br.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        khwe_br.BlendOp = D3D11_BLEND_OP_ADD;
        khwe_br.SrcBlendAlpha = D3D11_BLEND_ZERO;
        khwe_br.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        khwe_br.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        if (khwe_err.empty() && !g_res.bs_wsee_tpre) {
            khwe_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
            const HRESULT khwe_hr = dev->CreateBlendState(&khwe_bd, &g_res.bs_wsee_tpre);
            if (FAILED(khwe_hr) || !g_res.bs_wsee_tpre) khwe_err = "the Tpre blend state " + hr_str(khwe_hr);
        }
        if (khwe_err.empty() && !g_res.bs_wsee_tpost) {
            khwe_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
            const HRESULT khwe_hr = dev->CreateBlendState(&khwe_bd, &g_res.bs_wsee_tpost);
            if (FAILED(khwe_hr) || !g_res.bs_wsee_tpost) khwe_err = "the Tpost blend state " + hr_str(khwe_hr);
        }
        if (!khwe_err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one);
            KH_SAFE_RELEASE(g_res.dss_wsee_nw);
            KH_SAFE_RELEASE(g_res.dss_wsee_eq);
            KH_SAFE_RELEASE(g_res.bs_wsee_tpre);
            KH_SAFE_RELEASE(g_res.bs_wsee_tpost);
            g_res.wsee_v2_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: " + khwe_err +
                                   " (the chain reads the live depth through blended surfaces)");
            return false;
        }
    }
    // The layer-depth split's MRT twin (depth_resolve.hlsl PSWseeOneL: T and Tp at once); kh_wsee_ld_ensure, inside a
    // draw hook, makes its states only. Non-fatal: a refusal latches the split off (wsee_ld_failed, as its states'
    // refusal does; particles keep one depth a phase), reported once.
    if (!g_res.ps_wsee_one_l && !g_res.wsee_ld_failed) {
        std::string khle_err;
        const std::string khle_scs = std::to_string(g_res.depth_sample_count);
        const D3D_SHADER_MACRO khle_d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", khle_scs.c_str() }, { nullptr, nullptr } };
        const char* const khle_e[1] = { "PSWseeOneL" };
        ID3D11PixelShader** const khle_p[1] = { &g_res.ps_wsee_one_l };
        for (int khle_i = 0; khle_i < 1 && khle_err.empty(); ++khle_i) {
            if (*khle_p[khle_i]) continue;
            ID3DBlob* khle_b = nullptr;
            khle_err = compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), khle_e[khle_i], "ps_5_0", khle_d,
                                      &khle_b);
            HRESULT khle_hr = E_FAIL;
            if (khle_err.empty() && khle_b) {
                khle_hr = dev->CreatePixelShader(khle_b->GetBufferPointer(), khle_b->GetBufferSize(), nullptr,
                                                 khle_p[khle_i]);
            }
            if (khle_b) khle_b->Release();
            if (khle_err.empty() && (FAILED(khle_hr) || !*khle_p[khle_i])) {
                khle_err = std::string(khle_e[khle_i]) + " " + hr_str(khle_hr);
            }
        }
        if (!khle_err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one_l);
            g_res.wsee_ld_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth split could not be made "
                                   "(" + khle_err + "; particles keep one depth a phase)");
        }
    }
    // KH_WSEE_OQ2: the post twins that write D3m and D3's resolve (depth_resolve.hlsl PSWseeOneZ / PSWseeOneZL /
    // PSWseeD3Res). Non-fatal: a refusal latches every post replay onto the store path (ps_wsee_oq2_failed) until the
    // device goes, reported once.
    if ((!g_res.ps_wsee_one_z || !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) && !g_res.ps_wsee_oq2_failed) {
        std::string khoz_err;
        const std::string khoz_scs = std::to_string(g_res.depth_sample_count);
        const D3D_SHADER_MACRO khoz_d[] = {
            { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", khoz_scs.c_str() }, { nullptr, nullptr } };
        const char* const khoz_e[3] = { "PSWseeOneZ", "PSWseeOneZL", "PSWseeD3Res" };
        ID3D11PixelShader** const khoz_p[3] = { &g_res.ps_wsee_one_z, &g_res.ps_wsee_one_zl, &g_res.ps_wsee_d3_res };
        for (int khoz_i = 0; khoz_i < 3 && khoz_err.empty(); ++khoz_i) {
            if (*khoz_p[khoz_i]) continue;
            ID3DBlob* khoz_b = nullptr;
            khoz_err = compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), khoz_e[khoz_i], "ps_5_0", khoz_d,
                                      &khoz_b);
            HRESULT khoz_hr = E_FAIL;
            if (khoz_err.empty() && khoz_b) {
                khoz_hr = dev->CreatePixelShader(khoz_b->GetBufferPointer(), khoz_b->GetBufferSize(), nullptr,
                                                 khoz_p[khoz_i]);
            }
            if (khoz_b) khoz_b->Release();
            if (khoz_err.empty() && (FAILED(khoz_hr) || !*khoz_p[khoz_i])) {
                khoz_err = std::string(khoz_e[khoz_i]) + " " + hr_str(khoz_hr);
            }
        }
        if (!khoz_err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_wsee_one_z);
            KH_SAFE_RELEASE(g_res.ps_wsee_one_zl);
            KH_SAFE_RELEASE(g_res.ps_wsee_d3_res);
            g_res.ps_wsee_oq2_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the late opaques' depth target pass "
                                   "could not be made (" + khoz_err + "; every late opaque after the first "
                                   "fence-type layer stores its depth one by one, as before)");
        }
    }
    // KH_PSB_ZB: the one pass's block bound (depth_resolve.hlsl PSPsbZb at log2 s = KH_PSB_LS, KH_PSB_ZB_DEFS; the
    // capture draws it inside a draw hook). Non-fatal: a refusal latches Zb off (psb_zb_failed: the one pass's loop
    // reads without its bound) until the device goes, reported once.
    if (!g_res.ps_psb_zb && !g_res.psb_zb_failed) {
        std::string khzb_err;
        {
            ID3DBlob* khzb_b = nullptr;
            khzb_err = compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSPsbZb", "ps_5_0",
                                      KH_PSB_ZB_DEFS, &khzb_b);
            HRESULT khzb_hr = E_FAIL;
            if (khzb_err.empty() && khzb_b) {
                khzb_hr = dev->CreatePixelShader(khzb_b->GetBufferPointer(), khzb_b->GetBufferSize(), nullptr,
                                                 &g_res.ps_psb_zb);
            }
            if (khzb_b) khzb_b->Release();
            if (khzb_err.empty() && (FAILED(khzb_hr) || !g_res.ps_psb_zb)) {
                khzb_err = "PSPsbZb " + hr_str(khzb_hr);
            }
        }
        if (!khzb_err.empty()) {
            KH_SAFE_RELEASE(g_res.ps_psb_zb);
            g_res.psb_zb_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the one-pass record's block bound PS "
                                   "could not be made (" + khzb_err + "; its depth reads as before)");
        }
    }
    if (!g_res.ps_wsee_copy && !g_res.ps_wsee_copy_failed) {
        ID3DBlob* khwe_cb = nullptr;
        const std::string khwe_cerr = compile_shader(KH_PS_WSEE_COPY_SRC, "main", "ps_5_0", nullptr, &khwe_cb);
        HRESULT khwe_chr = E_FAIL;
        if (khwe_cerr.empty() && khwe_cb) {
            khwe_chr = dev->CreatePixelShader(khwe_cb->GetBufferPointer(), khwe_cb->GetBufferSize(), nullptr,
                                              &g_res.ps_wsee_copy);
        }
        if (khwe_cb) khwe_cb->Release();
        if (FAILED(khwe_chr) || !g_res.ps_wsee_copy) {
            KH_SAFE_RELEASE(g_res.ps_wsee_copy);
            g_res.ps_wsee_copy_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the chain's copy PS (a last pass that "
                                   "reads the scene's samples takes the approximation): " +
                                   (khwe_cerr.empty() ? hr_str(khwe_chr) : khwe_cerr));
        }
    }
    // The sorted tail's layer-aware composite: out.rgb = o0.rgb + dest.rgb x o1.rgb, out.a = o0.a + dest.a x o1.a
    // (dual source: PSMain's SV_Target0 / SV_Target1, cb.hlsl KhWseeMeshOut). With o0 = (a c, a) and o1 = 1 - a it is
    // the normal blend (blend_modes[0]) exactly. Non-fatal: absent, the sorted tail keeps the plain blend (mode 1)
    // and the injection defers nothing.
    if (!g_res.bs_wsee_sort && !g_res.bs_wsee_sort_failed) {
        D3D11_BLEND_DESC khwe_bd = {};
        khwe_bd.AlphaToCoverageEnable = FALSE;
        khwe_bd.IndependentBlendEnable = FALSE;
        D3D11_RENDER_TARGET_BLEND_DESC& khwe_rt = khwe_bd.RenderTarget[0];
        khwe_rt.BlendEnable = TRUE;
        khwe_rt.SrcBlend = D3D11_BLEND_ONE;
        khwe_rt.DestBlend = D3D11_BLEND_SRC1_COLOR;
        khwe_rt.BlendOp = D3D11_BLEND_OP_ADD;
        khwe_rt.SrcBlendAlpha = D3D11_BLEND_ONE;
        khwe_rt.DestBlendAlpha = D3D11_BLEND_SRC1_ALPHA;
        khwe_rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        khwe_rt.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&khwe_bd, &g_res.bs_wsee_sort)) || !g_res.bs_wsee_sort) {
            KH_SAFE_RELEASE(g_res.bs_wsee_sort);
            g_res.bs_wsee_sort_failed = true;
            report_error_once_safe("KH RenderIntegration: world see-through: the sorted tail's blend state could not "
                                   "be made (our translucent parts take the plain blend under the game's layers)");
        }
    }
    return true;
}

// KH_WSEE - the capture's targets (Resources' note: S, S2, D3, T, B, B2, NW, NW2), keyed by the texture depth_srv
// views (the main depth; the capture has checked the bound depth is that texture) and the scene target's view format
// (khwg_fmt, B's). Made by the capture itself (KH_VMSEE_LAZY's rule); a changed key remakes them. KH_WSEE_FAIL_RETRY:
// a refused key is asked again once KH_WSEE_HOLD_FAIL_RETRY cycles after the refusal (one Create* set per that many
// cycles while it keeps failing, reported once), a success at it clears the latch. The chain's own (E, C) have keys
// of their own and stay.
inline bool kh_wsee_targets(ID3D11Device* dev, DXGI_FORMAT khwg_fmt) {
    if (!dev || !g_res.depth_srv || khwg_fmt == DXGI_FORMAT_UNKNOWN) return false;
    D3D11_TEXTURE2D_DESC khwg_dd = {};
    {
        ID3D11Resource* khwg_r = nullptr;
        g_res.depth_srv->GetResource(&khwg_r);
        ID3D11Texture2D* khwg_t = nullptr;
        if (khwg_r) {
            if (FAILED(khwg_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khwg_t)))) {
                khwg_t = nullptr;
            }
            khwg_r->Release();
        }
        if (!khwg_t) return false;
        khwg_t->GetDesc(&khwg_dd);
        khwg_t->Release();
    }
    const UINT khwg_w = khwg_dd.Width, khwg_h = khwg_dd.Height;
    const UINT khwg_s = khwg_dd.SampleDesc.Count, khwg_q = khwg_dd.SampleDesc.Quality;
    if (khwg_w == 0 || khwg_h == 0 || khwg_w > 16384 || khwg_h > 16384 || khwg_s < 2) return false;
    if (g_res.wsee_s_tex && g_res.wsee_s_rtv && g_res.wsee_s_srv && g_res.wsee_t_tex && g_res.wsee_t_rtv &&
        g_res.wsee_t_srv && g_res.wsee_b_tex && g_res.wsee_b_srv && g_res.wsee_n_tex && g_res.wsee_n_dsv &&
        g_res.wsee_n_srv && g_res.wsee_b2_srv && g_res.wsee_n2_dsv && g_res.wsee_n2_srv &&
        g_res.wsee_s2_rtv && g_res.wsee_s2_srv &&
        g_res.wsee_d3_uav && g_res.wsee_d3_srv &&
        g_res.wsee_w == khwg_w && g_res.wsee_h == khwg_h && g_res.wsee_samples == khwg_s &&
        g_res.wsee_quality == khwg_q && g_res.wsee_b_fmt == khwg_fmt) return true;
    g_res.release_wsee_capture();
    const bool khwg_latched = g_res.wsee_fail_w == khwg_w && g_res.wsee_fail_h == khwg_h &&   // Refused at this key.
                              g_res.wsee_fail_s == khwg_s && g_res.wsee_fail_q == khwg_q &&
                              g_res.wsee_fail_f == khwg_fmt;
    if (khwg_latched && g_topo_cycles >= g_res.wsee_fail_cycle &&   // KH_WSEE_FAIL_RETRY (a count that started
        g_topo_cycles - g_res.wsee_fail_cycle < KH_WSEE_HOLD_FAIL_RETRY) return false;   // over: ask again).
    D3D11_TEXTURE2D_DESC khwg_td = {};
    khwg_td.Width = khwg_w;
    khwg_td.Height = khwg_h;
    khwg_td.MipLevels = 1;
    khwg_td.ArraySize = 1;
    khwg_td.Format = DXGI_FORMAT_R32_FLOAT;
    khwg_td.SampleDesc.Count = 1;
    khwg_td.Usage = D3D11_USAGE_DEFAULT;
    khwg_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool khwg_ok = SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_s_tex)) &&
                   SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_s_tex, nullptr, &g_res.wsee_s_rtv)) &&
                   SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_s_tex, nullptr, &g_res.wsee_s_srv));
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_s2_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_s2_tex, nullptr, &g_res.wsee_s2_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_s2_tex, nullptr, &g_res.wsee_s2_srv));
    // D3: S's desc with a UAV (the late opaque's store path writes it) and the RTV bind for D3's resolve from D3m
    // (KH_WSEE_OQ2; the RTV itself is kh_wsee_d3m_targets', made with D3m).
    khwg_td.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_d3_tex)) &&
              SUCCEEDED(dev->CreateUnorderedAccessView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_uav)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_srv));
    khwg_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    // T: R Tw (fence-type), G the late-opaque flag, B Tpre and A Tpost (particle-type, before / after B2).
    khwg_td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    khwg_td.SampleDesc.Count = khwg_s;   // The depth's: bound with depth_dsv_ro (Texture2DMS views).
    khwg_td.SampleDesc.Quality = khwg_q;
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_t_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_t_tex, nullptr, &g_res.wsee_t_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_t_tex, nullptr, &g_res.wsee_t_srv));
    khwg_td.Format = khwg_fmt;
    khwg_td.SampleDesc.Count = 1;
    khwg_td.SampleDesc.Quality = 0;
    khwg_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;   // A resolve destination (ensure_scene_capture_from's).
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_b_tex)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_b_tex, nullptr, &g_res.wsee_b_srv));
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_b2_tex)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_b2_tex, nullptr, &g_res.wsee_b2_srv));
    // NW / NW2 - single sample (one depth a pixel; bound alone, without an RTV, beside the engine's multisampled
    // rasterizer state), a typeless texture for its two views.
    khwg_td.Format = DXGI_FORMAT_R32_TYPELESS;
    khwg_td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    khwg_ok = khwg_ok && SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_n_tex)) &&
              SUCCEEDED(dev->CreateTexture2D(&khwg_td, nullptr, &g_res.wsee_n2_tex));
    if (khwg_ok) {
        D3D11_DEPTH_STENCIL_VIEW_DESC khwg_nd = {};
        khwg_nd.Format = DXGI_FORMAT_D32_FLOAT;
        khwg_nd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        D3D11_SHADER_RESOURCE_VIEW_DESC khwg_ns = {};
        khwg_ns.Format = DXGI_FORMAT_R32_FLOAT;
        khwg_ns.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        khwg_ns.Texture2D.MipLevels = 1;
        khwg_ok = SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_n_tex, &khwg_nd, &g_res.wsee_n_dsv)) &&
                  SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_n_tex, &khwg_ns, &g_res.wsee_n_srv)) &&
                  SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_n2_tex, &khwg_nd, &g_res.wsee_n2_dsv)) &&
                  SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_n2_tex, &khwg_ns, &g_res.wsee_n2_srv));
    }
    if (!khwg_ok || !g_res.wsee_s_rtv || !g_res.wsee_s_srv || !g_res.wsee_t_rtv || !g_res.wsee_t_srv ||
        !g_res.wsee_b_srv || !g_res.wsee_n_dsv || !g_res.wsee_n_srv || !g_res.wsee_b2_srv || !g_res.wsee_n2_dsv ||
        !g_res.wsee_n2_srv || !g_res.wsee_s2_rtv || !g_res.wsee_s2_srv ||
        !g_res.wsee_d3_uav || !g_res.wsee_d3_srv) {
        g_res.release_wsee_capture();
        g_res.wsee_fail_w = khwg_w;
        g_res.wsee_fail_h = khwg_h;
        g_res.wsee_fail_s = khwg_s;
        g_res.wsee_fail_q = khwg_q;
        g_res.wsee_fail_f = khwg_fmt;
        g_res.wsee_fail_cycle = g_topo_cycles;   // KH_WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the capture targets could not be made (the "
                               "chain reads the live depth through blended surfaces)");
        return false;
    }
    if (khwg_latched) {   // KH_WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_fail_w = g_res.wsee_fail_h = g_res.wsee_fail_s = g_res.wsee_fail_q = 0;
        g_res.wsee_fail_f = DXGI_FORMAT_UNKNOWN;
        g_res.wsee_fail_cycle = 0u;
    }
    g_res.wsee_w = khwg_w;
    g_res.wsee_h = khwg_h;
    g_res.wsee_samples = khwg_s;
    g_res.wsee_quality = khwg_q;
    g_res.wsee_b_fmt = khwg_fmt;
    return true;
}

// KH_WSEE - the layer-depth split's states (Resources' note at wsee_tp_tex), inside a draw hook (device Create* only):
// bs_wsee_ldpre / ldpost (ZERO / INV_SRC_ALPHA on red / green: Tp's R / G), bs_wsee_ldpre2 / ldpost2 (blue / alpha:
// Tp's B / A), dss_wsee_far (GREATER, no write). Once a device; a refusal latches the split off (wsee_ld_failed) and
// is reported once. The MRT twin PSWseeOneL is kh_wsee_ensure's: not yet made = no split this cycle, no latch.
inline bool kh_wsee_ld_ensure(ID3D11Device* dev) {
    if (g_res.ps_wsee_one_l && g_res.bs_wsee_ldpre && g_res.bs_wsee_ldpost &&
        g_res.bs_wsee_ldpre2 && g_res.bs_wsee_ldpost2 && g_res.dss_wsee_far) return true;
    if (g_res.wsee_ld_failed || !dev) return false;
    if (!g_res.ps_wsee_one_l) return false;
    std::string khle_err;
    D3D11_BLEND_DESC khle_bd = {};
    D3D11_RENDER_TARGET_BLEND_DESC& khle_br = khle_bd.RenderTarget[0];
    khle_br.BlendEnable = TRUE;
    khle_br.SrcBlend = D3D11_BLEND_ZERO;
    khle_br.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    khle_br.BlendOp = D3D11_BLEND_OP_ADD;
    khle_br.SrcBlendAlpha = D3D11_BLEND_ZERO;
    khle_br.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    khle_br.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    if (khle_err.empty() && !g_res.bs_wsee_ldpre) {
        khle_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
        if (FAILED(dev->CreateBlendState(&khle_bd, &g_res.bs_wsee_ldpre)) || !g_res.bs_wsee_ldpre) {
            khle_err = "the near / far pre blend state";
        }
    }
    if (khle_err.empty() && !g_res.bs_wsee_ldpost) {
        khle_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_GREEN;
        if (FAILED(dev->CreateBlendState(&khle_bd, &g_res.bs_wsee_ldpost)) || !g_res.bs_wsee_ldpost) {
            khle_err = "the near / far post blend state";
        }
    }
    if (khle_err.empty() && !g_res.bs_wsee_ldpre2) {   // The 100 % band's pre channel (B).
        khle_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_BLUE;
        if (FAILED(dev->CreateBlendState(&khle_bd, &g_res.bs_wsee_ldpre2)) || !g_res.bs_wsee_ldpre2) {
            khle_err = "the second near band's pre blend state";
        }
    }
    if (khle_err.empty() && !g_res.bs_wsee_ldpost2) {   // Its post channel (A: the alpha factors).
        khle_br.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
        if (FAILED(dev->CreateBlendState(&khle_bd, &g_res.bs_wsee_ldpost2)) || !g_res.bs_wsee_ldpost2) {
            khle_err = "the second near band's post blend state";
        }
    }
    if (khle_err.empty() && !g_res.dss_wsee_far) {
        D3D11_DEPTH_STENCIL_DESC khle_dd = {};
        khle_dd.DepthEnable = TRUE;
        khle_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        khle_dd.DepthFunc = D3D11_COMPARISON_GREATER;
        khle_dd.StencilEnable = FALSE;
        if (FAILED(dev->CreateDepthStencilState(&khle_dd, &g_res.dss_wsee_far)) || !g_res.dss_wsee_far) {
            khle_err = "the far pass's depth state";
        }
    }
    if (!khle_err.empty()) {
        KH_SAFE_RELEASE(g_res.ps_wsee_one_l);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpre);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpost);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpre2);
        KH_SAFE_RELEASE(g_res.bs_wsee_ldpost2);
        KH_SAFE_RELEASE(g_res.dss_wsee_far);
        g_res.wsee_ld_failed = true;
        report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth split could not be made (" +
                               khle_err + "; particles keep one depth a phase)");
        return false;
    }
    return true;
}

// KH_WSEE - Tp, Tf and Tf's depth (Resources' note at wsee_tp_tex), at T's key (kh_wsee_targets made it first);
// remade at a key change. A refusal is reported once and latched at its key (wsee_ld_fail_*: the capture targets'
// KH_WSEE_FAIL_RETRY rule - another key tries at once, this one again KH_WSEE_HOLD_FAIL_RETRY cycles later), the
// split off meanwhile (no Tp: kh_wsee_ld_on).
inline bool kh_wsee_ld_targets(ID3D11Device* dev) {
    if (!dev || g_res.wsee_ld_failed || !g_res.wsee_t_tex || g_res.wsee_w == 0 || g_res.wsee_h == 0) return false;
    if (g_res.wsee_tp_rtv && g_res.wsee_tp_srv && g_res.wsee_tf_rtv && g_res.wsee_tf_srv && g_res.wsee_tfd_dsv &&
        g_res.wsee_ld_w == g_res.wsee_w && g_res.wsee_ld_h == g_res.wsee_h && g_res.wsee_ld_s == g_res.wsee_samples &&
        g_res.wsee_ld_q == g_res.wsee_quality) return true;
    g_res.release_wsee_ld();
    const bool khlt_latched = g_res.wsee_ld_fail_w == g_res.wsee_w && g_res.wsee_ld_fail_h == g_res.wsee_h &&
                              g_res.wsee_ld_fail_s == g_res.wsee_samples &&
                              g_res.wsee_ld_fail_q == g_res.wsee_quality;   // Refused at this key.
    if (khlt_latched && g_topo_cycles >= g_res.wsee_ld_fail_cycle &&   // KH_WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_ld_fail_cycle < KH_WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_TEXTURE2D_DESC khlt_td = {};
    khlt_td.Width = g_res.wsee_w;
    khlt_td.Height = g_res.wsee_h;
    khlt_td.MipLevels = 1;
    khlt_td.ArraySize = 1;
    // Tp carries two bands: R / G the 25 % band's pre / post, B / A the 100 % band's; Tf is R8G8 (pre / post).
    khlt_td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    khlt_td.SampleDesc.Count = g_res.wsee_samples;   // T's: bound beside depth_dsv_ro (Texture2DMS views).
    khlt_td.SampleDesc.Quality = g_res.wsee_quality;
    khlt_td.Usage = D3D11_USAGE_DEFAULT;
    khlt_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool khlt_ok = SUCCEEDED(dev->CreateTexture2D(&khlt_td, nullptr, &g_res.wsee_tp_tex)) &&
                   SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_tp_tex, nullptr, &g_res.wsee_tp_rtv)) &&
                   SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_tp_tex, nullptr, &g_res.wsee_tp_srv));
    khlt_td.Format = DXGI_FORMAT_R8G8_UNORM;
    khlt_td.SampleDesc.Count = 1;   // Tf: one value a pixel (its own depth target, single sample).
    khlt_td.SampleDesc.Quality = 0;
    khlt_ok = khlt_ok && SUCCEEDED(dev->CreateTexture2D(&khlt_td, nullptr, &g_res.wsee_tf_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_tf_tex, nullptr, &g_res.wsee_tf_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_tf_tex, nullptr, &g_res.wsee_tf_srv));
    khlt_td.Format = DXGI_FORMAT_D32_FLOAT;
    khlt_td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    khlt_ok = khlt_ok && SUCCEEDED(dev->CreateTexture2D(&khlt_td, nullptr, &g_res.wsee_tfd_tex)) &&
              SUCCEEDED(dev->CreateDepthStencilView(g_res.wsee_tfd_tex, nullptr, &g_res.wsee_tfd_dsv));
    if (!khlt_ok || !g_res.wsee_tp_rtv || !g_res.wsee_tp_srv || !g_res.wsee_tf_rtv || !g_res.wsee_tf_srv ||
        !g_res.wsee_tfd_dsv) {
        g_res.release_wsee_ld();
        g_res.wsee_ld_fail_w = g_res.wsee_w;
        g_res.wsee_ld_fail_h = g_res.wsee_h;
        g_res.wsee_ld_fail_s = g_res.wsee_samples;
        g_res.wsee_ld_fail_q = g_res.wsee_quality;
        g_res.wsee_ld_fail_cycle = g_topo_cycles;   // KH_WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the layer-depth targets could not be made "
                               "(particles keep one depth a phase)");
        return false;
    }
    if (khlt_latched) {   // KH_WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_ld_fail_w = g_res.wsee_ld_fail_h = g_res.wsee_ld_fail_s = g_res.wsee_ld_fail_q = 0;
        g_res.wsee_ld_fail_cycle = 0u;
    }
    g_res.wsee_ld_w = g_res.wsee_w;
    g_res.wsee_ld_h = g_res.wsee_h;
    g_res.wsee_ld_s = g_res.wsee_samples;
    g_res.wsee_ld_q = g_res.wsee_quality;
    return true;
}

// KH_PSB: the one pass's states and constant buffers (Resources' KH_PSB note), made once a device by the capture
// (device Create* alone, no compile); refused - N's format (R32_FLOAT) cannot blend (CheckFormatSupport's BLENDABLE)
// or a make fails - latches the one pass off until the device goes (psb_failed): every re-issue keeps today's passes.
inline bool kh_psb_ensure(ID3D11Device* dev) {
    if (!dev || g_res.psb_failed) return false;
    if (g_res.bs_psb[1] && g_res.psb_cb[7]) return true;   // KH_PSB: two states, eight buffers.
    UINT khpe_fs = 0u;
    bool khpe_ok = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_R32_FLOAT, &khpe_fs)) &&   // KH_PSB_PHASE.
                   (khpe_fs & D3D11_FORMAT_SUPPORT_BLENDABLE) != 0u &&
                   (khpe_fs & D3D11_FORMAT_SUPPORT_RENDER_TARGET) != 0u;
    for (int khpe_i = 0; khpe_i < 2 && khpe_ok; ++khpe_i) {
        if (g_res.bs_psb[khpe_i]) continue;
        // KH_PSB_PHASE: the phase's own pair bound (kh_psb_post), so no phase mask - [0] the layer-depth split
        // standing: P (RT0) takes all four channels as T's particle passes take theirs (dest *= 1 - src: ZERO /
        // INV_SRC_COLOR, the alpha's ZERO / INV_SRC_ALPHA); [1] the split off: R and G alone (T and the 25 % band; B
        // and A keep their clear); N (RT1) the phase's nearest by MIN. No N-alone state: a light-adding draw keeps
        // its NW pass.
        const bool khpe_ld = khpe_i == 0;
        D3D11_BLEND_DESC khpe_d = {};
        khpe_d.IndependentBlendEnable = TRUE;
        {
            D3D11_RENDER_TARGET_BLEND_DESC& khpe_t = khpe_d.RenderTarget[0];
            khpe_t.BlendEnable = TRUE;
            khpe_t.SrcBlend = D3D11_BLEND_ZERO;
            khpe_t.DestBlend = D3D11_BLEND_INV_SRC_COLOR;
            khpe_t.BlendOp = D3D11_BLEND_OP_ADD;
            khpe_t.SrcBlendAlpha = D3D11_BLEND_ZERO;
            khpe_t.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            khpe_t.BlendOpAlpha = D3D11_BLEND_OP_ADD;
            khpe_t.RenderTargetWriteMask = static_cast<UINT8>(khpe_ld ?
                static_cast<UINT>(D3D11_COLOR_WRITE_ENABLE_ALL) :
                static_cast<UINT>(D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN));
        }
        D3D11_RENDER_TARGET_BLEND_DESC& khpe_n = khpe_d.RenderTarget[1];
        khpe_n.BlendEnable = TRUE;
        khpe_n.SrcBlend = D3D11_BLEND_ONE;
        khpe_n.DestBlend = D3D11_BLEND_ONE;
        khpe_n.BlendOp = D3D11_BLEND_OP_MIN;
        khpe_n.SrcBlendAlpha = D3D11_BLEND_ONE;
        khpe_n.DestBlendAlpha = D3D11_BLEND_ONE;
        khpe_n.BlendOpAlpha = D3D11_BLEND_OP_MIN;
        khpe_n.RenderTargetWriteMask = static_cast<UINT8>(D3D11_COLOR_WRITE_ENABLE_RED);
        khpe_ok = SUCCEEDED(dev->CreateBlendState(&khpe_d, &g_res.bs_psb[khpe_i])) && g_res.bs_psb[khpe_i];
    }
    D3D11_BUFFER_DESC khpe_bd = {};
    khpe_bd.ByteWidth = 64u;   // cb[0] (m_1, m_2, m_3, Dq), cb[1] (a_1, a_2, a_3, D), cb[2] the masks and the count,
                               // KH_PSB_LORES: cb[3] the block's addressing and the scale (kh_psb_post).
    khpe_bd.Usage = D3D11_USAGE_DEFAULT;
    khpe_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    for (int khpe_i = 0; khpe_i < 8 && khpe_ok; ++khpe_i) {
        if (g_res.psb_cb[khpe_i]) continue;
        khpe_ok = SUCCEEDED(dev->CreateBuffer(&khpe_bd, nullptr, &g_res.psb_cb[khpe_i])) && g_res.psb_cb[khpe_i];
        g_res.psb_cb_set[khpe_i] = false;
    }
    if (!khpe_ok) {
        for (ID3D11BlendState*& khpe_b : g_res.bs_psb) KH_SAFE_RELEASE(khpe_b);
        for (int khpe_i = 0; khpe_i < 8; ++khpe_i) {
            KH_SAFE_RELEASE(g_res.psb_cb[khpe_i]);
            g_res.psb_cb_set[khpe_i] = false;
        }
        g_res.psb_failed = true;
        report_error_once_safe("KH RenderIntegration: world see-through: the one-pass particle record could not be "
                               "set up (the particle re-draws keep their passes)");
    }
    return khpe_ok;
}
// KH_PSB: P0, P1 (RGBA8) and N, N2 (R32_FLOAT, a phase's NW each), single sample at ceil(size / s) a side (s =
// 1 << KH_PSB_LS; KH_PSB_LORES), Zb with them - made by the capture, kept at their key (T's size); a refused make is
// latched at its key (wsee_pb_fail_*, the capture targets' KH_WSEE_FAIL_RETRY rule).
inline bool kh_psb_targets(ID3D11Device* dev) {
    if (!dev || !g_res.wsee_t_tex || g_res.wsee_w == 0 || g_res.wsee_h == 0 || !kh_psb_ensure(dev)) return false;
    if (g_res.wsee_p0_rtv && g_res.wsee_p0_srv && g_res.wsee_p1_rtv && g_res.wsee_p1_srv && g_res.wsee_nb_rtv &&
        g_res.wsee_nb_srv && g_res.wsee_nb2_rtv && g_res.wsee_nb2_srv &&   // KH_PSB_PHASE.
        g_res.wsee_pb_w == g_res.wsee_w && g_res.wsee_pb_h == g_res.wsee_h &&
        (g_res.wsee_zb_srv || g_res.wsee_zb_refused)) return true;   // KH_PSB_ZB.
    g_res.release_wsee_pb();
    const bool khpt_latched = g_res.wsee_pb_fail_w == g_res.wsee_w && g_res.wsee_pb_fail_h == g_res.wsee_h;
    if (khpt_latched && g_topo_cycles >= g_res.wsee_pb_fail_cycle &&   // KH_WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_pb_fail_cycle < KH_WSEE_HOLD_FAIL_RETRY) return false;
    D3D11_TEXTURE2D_DESC khpt_td = {};
    khpt_td.Width = (g_res.wsee_w + (1u << KH_PSB_LS) - 1u) >> KH_PSB_LS;    // KH_PSB_LORES: 1 / s, rounded up.
    khpt_td.Height = (g_res.wsee_h + (1u << KH_PSB_LS) - 1u) >> KH_PSB_LS;
    khpt_td.MipLevels = 1;
    khpt_td.ArraySize = 1;
    khpt_td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    khpt_td.SampleDesc.Count = 1;
    khpt_td.Usage = D3D11_USAGE_DEFAULT;
    khpt_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool khpt_ok = SUCCEEDED(dev->CreateTexture2D(&khpt_td, nullptr, &g_res.wsee_p0_tex)) &&
                   SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_p0_tex, nullptr, &g_res.wsee_p0_rtv)) &&
                   SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_p0_tex, nullptr, &g_res.wsee_p0_srv)) &&
                   SUCCEEDED(dev->CreateTexture2D(&khpt_td, nullptr, &g_res.wsee_p1_tex)) &&
                   SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_p1_tex, nullptr, &g_res.wsee_p1_rtv)) &&
                   SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_p1_tex, nullptr, &g_res.wsee_p1_srv));
    khpt_td.Format = DXGI_FORMAT_R32_FLOAT;   // KH_PSB_PHASE: a phase's NW each.
    khpt_ok = khpt_ok && SUCCEEDED(dev->CreateTexture2D(&khpt_td, nullptr, &g_res.wsee_nb_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_nb_tex, nullptr, &g_res.wsee_nb_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_nb_tex, nullptr, &g_res.wsee_nb_srv)) &&
              SUCCEEDED(dev->CreateTexture2D(&khpt_td, nullptr, &g_res.wsee_nb2_tex)) &&
              SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_nb2_tex, nullptr, &g_res.wsee_nb2_rtv)) &&
              SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_nb2_tex, nullptr, &g_res.wsee_nb2_srv));
    if (!khpt_ok || !g_res.wsee_p0_rtv || !g_res.wsee_p0_srv || !g_res.wsee_p1_rtv || !g_res.wsee_p1_srv ||
        !g_res.wsee_nb_rtv || !g_res.wsee_nb_srv || !g_res.wsee_nb2_rtv || !g_res.wsee_nb2_srv) {
        g_res.release_wsee_pb();
        g_res.wsee_pb_fail_w = g_res.wsee_w;
        g_res.wsee_pb_fail_h = g_res.wsee_h;
        g_res.wsee_pb_fail_cycle = g_topo_cycles;   // KH_WSEE_FAIL_RETRY.
        report_error_once_safe("KH RenderIntegration: world see-through: the one-pass particle record's targets "
                               "could not be made (the particle re-draws keep their passes)");
        return false;
    }
    if (khpt_latched) {   // KH_WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_pb_fail_w = g_res.wsee_pb_fail_h = 0;
        g_res.wsee_pb_fail_cycle = 0u;
    }
    // KH_PSB_ZB: Zb (R32G32_FLOAT, the record's size; a refused make leaves it absent at this key - the one pass reads
    // without its bound, no retry until the targets are released: wsee_zb_refused).
    {
        khpt_td.Format = DXGI_FORMAT_R32G32_FLOAT;
        if (FAILED(dev->CreateTexture2D(&khpt_td, nullptr, &g_res.wsee_zb_tex)) ||
            FAILED(dev->CreateRenderTargetView(g_res.wsee_zb_tex, nullptr, &g_res.wsee_zb_rtv)) ||
            FAILED(dev->CreateShaderResourceView(g_res.wsee_zb_tex, nullptr, &g_res.wsee_zb_srv)) ||
            !g_res.wsee_zb_rtv || !g_res.wsee_zb_srv) {
            KH_SAFE_RELEASE(g_res.wsee_zb_srv);
            KH_SAFE_RELEASE(g_res.wsee_zb_rtv);
            KH_SAFE_RELEASE(g_res.wsee_zb_tex);
            g_res.wsee_zb_refused = true;   // KH_PSB_ZB.
            report_error_once_safe("KH RenderIntegration: world see-through: the one-pass record's block bound could "
                                   "not be made (its depth reads as before)");
        }
    }
    g_res.wsee_pb_w = g_res.wsee_w;
    g_res.wsee_pb_h = g_res.wsee_h;
    return true;
}

// KH_WSEE_OQ2 - D3m and D3's RTV (Resources' note at wsee_d3_rtv), at T's key plus the depth view's format (one
// GetDesc a capture); remade at a key change. Only for a 24- or 32-bit depth view (R24_UNORM_X8_TYPELESS, R32_FLOAT,
// R32_FLOAT_X8X24_TYPELESS: a 16-bit step exceeds the resolve's tolerance - the store path, unreported, no call).
// A refusal is reported once and latched at its key (wsee_d3m_fail_*, the capture targets' KH_WSEE_FAIL_RETRY rule).
// Inside a draw hook (the capture): device Create* only, a one-time hitch at the first make; the shaders are
// kh_wsee_ensure's, nothing is made before the three stand, nothing kept once they were refused.
inline bool kh_wsee_d3m_targets(ID3D11Device* dev) {
    if (g_res.ps_wsee_oq2_failed || !g_res.ps_wsee_one_z || !g_res.ps_wsee_one_zl || !g_res.ps_wsee_d3_res) {
        if (g_res.wsee_d3m_tex) g_res.release_wsee_d3m();
        return false;
    }
    if (!dev || !g_res.wsee_d3_tex || !g_res.wsee_t_tex || !g_res.depth_srv ||
        g_res.wsee_w == 0 || g_res.wsee_h == 0) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC khdm_vd = {};
    g_res.depth_srv->GetDesc(&khdm_vd);
    if (g_res.wsee_d3m_rtv && g_res.wsee_d3m_srv && g_res.wsee_d3_rtv && g_res.wsee_d3m_w == g_res.wsee_w &&
        g_res.wsee_d3m_h == g_res.wsee_h && g_res.wsee_d3m_s == g_res.wsee_samples &&
        g_res.wsee_d3m_q == g_res.wsee_quality && g_res.wsee_d3m_dfmt == khdm_vd.Format) return true;
    g_res.release_wsee_d3m();
    const bool khdm_latched = g_res.wsee_d3m_fail_w == g_res.wsee_w && g_res.wsee_d3m_fail_h == g_res.wsee_h &&
                              g_res.wsee_d3m_fail_s == g_res.wsee_samples &&
                              g_res.wsee_d3m_fail_q == g_res.wsee_quality &&
                              g_res.wsee_d3m_fail_f == khdm_vd.Format;   // Refused at this key.
    if (khdm_latched && g_topo_cycles >= g_res.wsee_d3m_fail_cycle &&   // KH_WSEE_FAIL_RETRY.
        g_topo_cycles - g_res.wsee_d3m_fail_cycle < KH_WSEE_HOLD_FAIL_RETRY) return false;
    const bool khdm_fmt = khdm_vd.Format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS ||
                          khdm_vd.Format == DXGI_FORMAT_R32_FLOAT ||
                          khdm_vd.Format == DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    D3D11_TEXTURE2D_DESC khdm_td = {};
    khdm_td.Width = g_res.wsee_w;
    khdm_td.Height = g_res.wsee_h;
    khdm_td.MipLevels = 1;
    khdm_td.ArraySize = 1;
    khdm_td.Format = DXGI_FORMAT_R32_FLOAT;
    khdm_td.SampleDesc.Count = g_res.wsee_samples;   // T's: bound beside T and depth_dsv_ro (a Texture2DMS view).
    khdm_td.SampleDesc.Quality = g_res.wsee_quality;
    khdm_td.Usage = D3D11_USAGE_DEFAULT;
    khdm_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    const bool khdm_ok = khdm_fmt && SUCCEEDED(dev->CreateTexture2D(&khdm_td, nullptr, &g_res.wsee_d3m_tex)) &&
                         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_d3m_tex, nullptr, &g_res.wsee_d3m_rtv)) &&
                         SUCCEEDED(dev->CreateShaderResourceView(g_res.wsee_d3m_tex, nullptr, &g_res.wsee_d3m_srv)) &&
                         SUCCEEDED(dev->CreateRenderTargetView(g_res.wsee_d3_tex, nullptr, &g_res.wsee_d3_rtv));
    if (!khdm_ok || !g_res.wsee_d3m_rtv || !g_res.wsee_d3m_srv || !g_res.wsee_d3_rtv) {
        g_res.release_wsee_d3m();
        g_res.wsee_d3m_fail_w = g_res.wsee_w;
        g_res.wsee_d3m_fail_h = g_res.wsee_h;
        g_res.wsee_d3m_fail_s = g_res.wsee_samples;
        g_res.wsee_d3m_fail_q = g_res.wsee_quality;
        g_res.wsee_d3m_fail_f = khdm_vd.Format;
        g_res.wsee_d3m_fail_cycle = g_topo_cycles;   // KH_WSEE_FAIL_RETRY.
        if (khdm_fmt) {
            report_error_once_safe("KH RenderIntegration: world see-through: the late opaques' depth target could "
                                   "not be made (every late opaque after the first fence-type layer stores its depth "
                                   "one by one, as before)");
        }
        return false;
    }
    if (khdm_latched) {   // KH_WSEE_FAIL_RETRY: a retry at the refused key succeeded.
        g_res.wsee_d3m_fail_w = g_res.wsee_d3m_fail_h = g_res.wsee_d3m_fail_s = g_res.wsee_d3m_fail_q = 0;
        g_res.wsee_d3m_fail_f = DXGI_FORMAT_UNKNOWN;
        g_res.wsee_d3m_fail_cycle = 0u;
    }
    g_res.wsee_d3m_w = g_res.wsee_w;
    g_res.wsee_d3m_h = g_res.wsee_h;
    g_res.wsee_d3m_s = g_res.wsee_samples;
    g_res.wsee_d3m_q = g_res.wsee_quality;
    g_res.wsee_d3m_dfmt = khdm_vd.Format;
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
    KhOmSave khs_om;
    khs_om.capture(ctx);
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
    const FLOAT khs_bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(nullptr, khs_bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ctx->Draw(3, 0);

    // Unbind t0 before the engine rebinds the live depth as a DSV (a lingering
    // SRV would be force-nulled with a debug-layer warning storm).
    ID3D11ShaderResourceView* khs_null_srv = nullptr;
    ctx->PSSetShaderResources(0, 1, &khs_null_srv);

    khs_om.restore(ctx);
    khs_om.release();
    if (n_vp > 0) ctx->RSSetViewports(n_vp, saved_vp);
    backup.restore(ctx);

    g_res.comp_depth_time = effect_time_seconds();
    return "";
}
}