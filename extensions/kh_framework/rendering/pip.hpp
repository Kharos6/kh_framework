#pragma once

// The injection half (rendering_integration_injection.hpp): KH_PIP (picture-in-picture). Never include it on its own.
namespace RenderIntegration {

// KH_PIP - our meshes in the engine's PIP cameras. A PIP is a full scene pass on its own depth (D24S8) and HDR colour
// targets (the main scene's kind: RGBA16F at HDR quality Standard), both single-sample, rendered BEFORE the main pass.
// Engine per-view block (VS b2): floats 0..15 rebase origin (1,0,0,X / 0,1,0,0 / 0,0,1,Z / 0,0,0,0), 16..27 the 3x4
// view [R | -R.cam] on rebased positions, 28..31 rebased camera (w = 1), 56..59 absolute camera (w = 1). Projection: b0
// floats 0..15, row-vector (m00 0 0 0 / 0 m11 0 0 / 0 0 m22 1 / 0 0 m32 0). Both are re-uploaded per view ahead of its
// draws, so the last such uploads before a PIP draw are the PIP camera's. The pass is recognised at the OM hook
// (non-main depth + RGBA16F or the main scene's own format, KH_PIP_FMT - single-sample colour that is not the scene
// texture); its depth clear opens a cycle, opaques are counted, and the first blended draw after KH_PIP_MIN_OPAQUES
// fires one injection: the previous main pass's frame template with the view lanes replaced and every screen-space lane
// disarmed (depth witness, band table, volume stencil, mirror, snapshot); world-space products (sun ladder, DLS maps,
// terrain, lighting) are shared. KH_PIP_FX: localized fullscreen passes run in the PIP when the engine COPIES THE PIP
// COLOUR TARGET OUT (hooked_copyresource), once, at the first copy of a cycle - it copies mid-pass for its translucents
// and after its last draw for its post pass, which overwrites the target from that copy, so anything drawn at the OM
// leave is lost; the leave stays as the fallback for a PIP that is never copied. They run through PSEffect over a copy
// of the PIP colour (t0: one copy per cycle, the chain ping-ponging between two buffers, its last pass writing the PIP
// target) and of its depth (t1), with the PIP camera in fxCam, under the PIP's own viewport captured during the pass,
// RGB only. The SSGI chain and dynamicLightFog stay main-view only; a user (.hlsl) pass runs once its single-sample
// build is ready (KH_FX_USER), a LUT pass once the flush has loaded its file.
static constexpr uint32_t KH_PIP_MIN_OPAQUES = 32;     // Opaque draws on the PIP depth before a fire.
static constexpr uint32_t KH_PIP_TPL_MAX_CYCLES = 2;   // A template older than this is stale.

struct KhPipState {
    // The pass, from the OM hook.
    bool     on = false;               // The bound targets are a PIP pass.
    void*    cur_rtv = nullptr;        // KH_PIP_FX: RT0 of the PIP pass currently bound (weak; a change is a leave).
    uint32_t w = 0, h = 0;             // Its colour target size.
    D3D11_VIEWPORT vp = {};            // KH_PIP_FX_LEAVE_VP: the pass's viewport, read at its MIN_OPAQUES-th opaque draw.
    bool     vp_valid = false;
    bool     fx_done = false;          // KH_PIP_FX: the passes were attempted this PIP cycle.
    // The cycle, from the clear hook and the draws.
    uint32_t opaques = 0;
    bool     injected = false;
    // The harvest, from the upload hooks (the latest of each block, any pass).
    float    view_rows[12] = {};       // 3x4 [R | t], engine layout.
    float    cam[3] = {};              // Absolute camera.
    bool     view_valid = false;
    float    proj[16] = {};            // Row-vector projection.
    bool     proj_valid = false;
    // The main view's finished frame template: the injection's when it published this cycle, else the flush's.
    ConstantData tpl = {};
    bool     tpl_valid = false;
    uint64_t tpl_cycle = 0;
    // KH_PIP_FMT: the main scene colour target's format (a DXGI_FORMAT; 0 = not seen yet), kh_pip_scene_fmt_note.
    uint32_t scene_fmt = 0;
    // Classification cache: (dsv identity, rtv0) -> verdict, with the scene_fmt it was made under (fmt): a
    // verdict made before the format was learned, or under another, is not reused.
    struct Key { void* dsv; void* rtv; bool pip; uint32_t w, h; uint32_t fmt; };
    Key      cache[8] = {};
    uint32_t cache_n = 0;
};
static KhPipState g_pip;

inline void kh_pip_reset() {
    g_pip = KhPipState();
    g_pip_seen = false;
}
// KH_PIP_FMT - the main scene colour target's format, read wherever kh_scene_tex_name names the scene texture: where
// the injection draws into it (its RT0 at the landing, the resource the census latches) and at a scene resolve
// kh_main_depth_readopt recognises (its source). A PIP pass renders the scene into a target of the same kind, and
// that kind follows the video options' HDR quality: RGBA16F at Standard, another format at Low, where a fixed
// RGBA16F test recognises no PIP and none of our meshes appear in one. Render thread (both namers'); the
// reference is the caller's, live for the call.
inline void kh_pip_scene_fmt_note(ID3D11Resource* khpsf_res) {
    ID3D11Texture2D* khpsf_tex = nullptr;
    if (FAILED(khpsf_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khpsf_tex))) ||
        !khpsf_tex) return;
    D3D11_TEXTURE2D_DESC khpsf_td = {};
    khpsf_tex->GetDesc(&khpsf_td);
    khpsf_tex->Release();
    g_pip.scene_fmt = static_cast<uint32_t>(khpsf_td.Format);
}

// The two engine blocks by signature (see the KH_PIP note). A handful of exact compares before anything else.
inline void kh_pip_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 64) return;
    if (res == static_cast<ID3D11Resource*>(g_res.composite_cb) ||
        res == static_cast<ID3D11Resource*>(g_res.constant_buffer) ||
        res == static_cast<ID3D11Resource*>(g_res.frame_cb) ||
        res == static_cast<ID3D11Resource*>(g_res.composite_frame_cb)) return;
    const float* f = static_cast<const float*>(data);
    const uint32_t nf = bytes / 4;
    kh_upload_need(data, (nf < 60u ? nf : 60u) * 4u);   // KH_UPLOAD_LAZY: floats 0..59.
    // The view block: rebase rows, then [R | t] with w-lanes 1 at 31 and 59.
    if (nf >= 60 && f[0] == 1.0f && f[1] == 0.0f && f[2] == 0.0f &&
        f[4] == 0.0f && f[5] == 1.0f && f[6] == 0.0f && f[7] == 0.0f &&
        f[8] == 0.0f && f[9] == 0.0f && f[10] == 1.0f &&
        f[12] == 0.0f && f[13] == 0.0f && f[14] == 0.0f && f[15] == 0.0f &&
        f[31] == 1.0f && f[59] == 1.0f) {
        bool khpu_ok = true;
        for (int r = 0; r < 3 && khpu_ok; ++r) {
            const float* w = f + 16 + r * 4;
            const float n = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
            if (n < 0.98f || n > 1.02f) khpu_ok = false;
        }
        if (khpu_ok) {
            // The absolute camera must be the rebased one plus the origin.
            const float khpu_dx = fabsf((f[28] + f[3]) - f[56]);
            const float khpu_dz = fabsf((f[30] + f[11]) - f[58]);
            if (khpu_dx < 0.5f && khpu_dz < 0.5f) {
                memcpy(g_pip.view_rows, f + 16, sizeof(g_pip.view_rows));
                g_pip.cam[0] = f[56]; g_pip.cam[1] = f[57]; g_pip.cam[2] = f[58];
                g_pip.view_valid = true;
            }
        }
        return;
    }
    // The projection block: the row-vector perspective shape.
    if (nf >= 16 && f[1] == 0.0f && f[2] == 0.0f && f[3] == 0.0f &&
        f[4] == 0.0f && f[6] == 0.0f && f[7] == 0.0f &&
        f[8] == 0.0f && f[9] == 0.0f && f[11] == 1.0f &&
        f[12] == 0.0f && f[13] == 0.0f && f[15] == 0.0f &&
        f[0] > 0.01f && f[5] > 0.01f && f[10] > 0.9f && f[10] < 1.1f && f[14] < 0.0f) {
        memcpy(g_pip.proj, f, sizeof(g_pip.proj));
        g_pip.proj_valid = true;
    }
}

// KH_PIP_FX: PSEffect against a single-sample depth; a multisampled PIP depth stands the pass down rather than reading
// it through the wrong declaration. The prewarm table carries this exact define set. One attempt per device: a failed
// one is reported once by its caller's kh_ensure_ok, after which "" with no shader stands the passes down quietly.
inline std::string kh_pip_fx_shader(ID3D11Device* dev) {
    if (g_res.ps_effect_pip || g_res.ps_effect_pip_tried) return "";   // KH_EA_PIP_ONCE.
    g_res.ps_effect_pip_tried = true;
    const D3D_SHADER_MACRO khpx_def[] = { { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    const std::string khpx_src = kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_EFFECT).c_str();
    ID3DBlob* khpx_blob = nullptr;
    std::string khpx_err = compile_shader(khpx_src.c_str(), "PSEffect", "ps_5_0", khpx_def, &khpx_blob);
    if (!khpx_err.empty()) return khpx_err;
    const HRESULT khpx_hr = dev->CreatePixelShader(khpx_blob->GetBufferPointer(), khpx_blob->GetBufferSize(), nullptr, &g_res.ps_effect_pip);
    khpx_blob->Release();
    if (FAILED(khpx_hr)) { g_res.ps_effect_pip = nullptr; return "CreatePixelShader(effect, PIP) " + hr_str(khpx_hr); }
    return "";
}

// The PIP's depth, copied into a texture of ours the effect reads at t1. A copy rather than an SRV on the engine's
// texture: the PIP depth carries whatever bind flags the engine gave it, and the source stays bound as it was. The copy
// is the source's typeless kind (CopyResource takes one type group; a typed depth - D24_UNORM_S8_UINT, D32_FLOAT, ... -
// cannot carry an SRV, so it copies into its group's typeless format, KH_PIP_DEPTH_TYPED), the SRV typed as
// ensure_depth_srv types the main one.
inline bool kh_pip_fx_depth(ID3D11Device* khpd_dev, ID3D11DeviceContext* khpd_ctx, ID3D11DepthStencilView* khpd_dsv) {
    ID3D11Resource* khpd_res = nullptr;
    khpd_dsv->GetResource(&khpd_res);
    if (!khpd_res) return false;
    ID3D11Texture2D* khpd_tex = nullptr;
    if (FAILED(khpd_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khpd_tex))) || !khpd_tex) {
        khpd_res->Release();
        return false;
    }
    D3D11_TEXTURE2D_DESC khpd_td = {};
    khpd_tex->GetDesc(&khpd_td);
    bool khpd_ok = khpd_td.SampleDesc.Count == 1 && khpd_td.ArraySize == 1;
    DXGI_FORMAT khpd_sf = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT khpd_cf = DXGI_FORMAT_UNKNOWN;   // KH_PIP_DEPTH_TYPED: the copy's format.
    switch (khpd_td.Format) {
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            khpd_sf = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; khpd_cf = DXGI_FORMAT_R24G8_TYPELESS; break;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            khpd_sf = DXGI_FORMAT_R32_FLOAT; khpd_cf = DXGI_FORMAT_R32_TYPELESS; break;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            khpd_sf = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; khpd_cf = DXGI_FORMAT_R32G8X24_TYPELESS; break;
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
            khpd_sf = DXGI_FORMAT_R16_UNORM; khpd_cf = DXGI_FORMAT_R16_TYPELESS; break;
        default:
            khpd_ok = false;
            // KH_EA_ERR_CLASS: an engine PIP configuration we do not serve - a warning (logged, never thrown).
            kh_log_once_safe("KH RenderIntegration: pip fx: the PIP depth's format (" +
                             std::to_string(static_cast<int>(khpd_td.Format)) +
                             ") has no read; no PIP post passes");
            break;
    }
    if (khpd_ok && (!g_res.pip_depth_tex || !g_res.pip_depth_srv ||
                    g_res.pip_depth_w != khpd_td.Width || g_res.pip_depth_h != khpd_td.Height ||
                    g_res.pip_depth_fmt != khpd_td.Format)) {
        KH_SAFE_RELEASE(g_res.pip_depth_srv);
        KH_SAFE_RELEASE(g_res.pip_depth_tex);
        g_res.pip_depth_w = g_res.pip_depth_h = 0;
        g_res.pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
        D3D11_TEXTURE2D_DESC khpd_cd = {};
        khpd_cd.Width = khpd_td.Width;
        khpd_cd.Height = khpd_td.Height;
        khpd_cd.MipLevels = 1;
        khpd_cd.ArraySize = 1;
        khpd_cd.Format = khpd_cf;
        khpd_cd.SampleDesc.Count = 1;
        khpd_cd.Usage = D3D11_USAGE_DEFAULT;
        khpd_cd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SHADER_RESOURCE_VIEW_DESC khpd_sd = {};
        khpd_sd.Format = khpd_sf;
        khpd_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        khpd_sd.Texture2D.MipLevels = 1;
        if (FAILED(khpd_dev->CreateTexture2D(&khpd_cd, nullptr, &g_res.pip_depth_tex)) || !g_res.pip_depth_tex ||
            FAILED(khpd_dev->CreateShaderResourceView(g_res.pip_depth_tex, &khpd_sd, &g_res.pip_depth_srv)) || !g_res.pip_depth_srv) {
            KH_SAFE_RELEASE(g_res.pip_depth_srv);
            KH_SAFE_RELEASE(g_res.pip_depth_tex);
            report_error_once_safe("KH RenderIntegration: pip fx: copy of the PIP depth could not be created (format " +
                                   std::to_string(static_cast<int>(khpd_td.Format)) + "); no PIP post passes");
            khpd_ok = false;
        } else {
            g_res.pip_depth_w = khpd_td.Width;
            g_res.pip_depth_h = khpd_td.Height;
            g_res.pip_depth_fmt = khpd_td.Format;
        }
    }
    if (khpd_ok) {
        // KH_PRED_ASIDE: the copy runs ahead of kh_pip_fx's StateBackup, inside the hook that called it - a predicate
        // the engine left bound would skip it and the passes would read the last PIP cycle's depth.
        KhPredAside khpd_pa;
        khpd_pa.begin(khpd_ctx);
        khpd_ctx->CopyResource(g_res.pip_depth_tex, khpd_tex);
        khpd_pa.end();
    }
    khpd_tex->Release();
    khpd_res->Release();
    return khpd_ok;
}

// KH_UI_BB_UNWIND: releases the COM pointer it watches if - and only if - its scope is left by an exception;
// every normal exit releases the pointer itself. It watches the variable, not its value at the guard.
// Holders: flush_ui_locked's backbuffer and its bound views (RTV, DSV), and kh_pip_fx's PIP colour resource (declared
// here, ahead of both).
template <typename KhT>
struct KhUnwindRelease {
    KhT*& khur_p;
    const int khur_ex;
    explicit KhUnwindRelease(KhT*& khur_v) : khur_p(khur_v), khur_ex(std::uncaught_exceptions()) {}
    ~KhUnwindRelease() {
        if (std::uncaught_exceptions() > khur_ex && khur_p) {
            khur_p->Release();
            khur_p = nullptr;
        }
    }
    KhUnwindRelease(const KhUnwindRelease&) = delete;
    KhUnwindRelease& operator=(const KhUnwindRelease&) = delete;
};

// The passes, into the PIP's colour target while bound. Called from hooked_copyresource (before the engine's copy) and,
// for a PIP that is never copied, from kh_pip_track_targets at the leave. One attempt per PIP cycle (fx_done set first:
// our own copies re-enter the copy hook). Gated on the pass itself (the opaque count), not the injection: a scene of
// fullscreen passes and no meshes never injects.
inline void kh_pip_fx(ID3D11DeviceContext* ctx) {
    g_pip.fx_done = true;
    if (g_pip.opaques < KH_PIP_MIN_OPAQUES || !g_pip.vp_valid) return;   // A real pass, its viewport known.
    if (!g_pip.tpl_valid || !g_pip.view_valid || !g_pip.proj_valid) return;
    if (g_topo_cycles < g_pip.tpl_cycle || g_topo_cycles - g_pip.tpl_cycle > KH_PIP_TPL_MAX_CYCLES) return;
    if (!g_res.vs_fullscreen || !g_res.dss_off || !g_res.rasterizer ||
        !g_res.composite_cb || !g_res.composite_frame_cb) return;

    // The flush's fullscreen set narrowed to the localized ones (a global pass already reaches the mirror through the
    // main view), in creation order, the envelope folded into the alpha.
    static std::vector<std::pair<uint64_t, RenderObject>> khpf_list;
    khpf_list.clear();
    const float khpf_now = effect_time_seconds();
    const double khpf_now_d = effect_time_seconds_d();
    ID3D11Device* khpf_dev = RVExtBridge::get_d3d_device();   // KH_FX_USER: a user pass's PIP build, asked here.
    kh_scene_sync();
    for (uint32_t khpf_i = 0; khpf_i < g_scene.objs.size(); ++khpf_i) {
        if (!g_scene.alive[khpf_i]) continue;
        const RenderObject& o = g_scene.objs[khpf_i];
        if (!o.visible || !o.fullscreen || !o.localized || o.ui_only || o.effect <= 0) continue;
        if (o.effect == static_cast<int>(EffectId::Ssgi)) continue;
        if (o.effect == static_cast<int>(EffectId::DynLightFog)) continue;   // KH_DLF: the main view's lights.
        // KH_FX_USER: a user pass joins once its single-sample build is ready (asking queues it).
        if (o.effect == KH_EFFECT_CUSTOM &&
            (!khpf_dev || !kh_user_fx_ps(khpf_dev, kh_fx_shader_of(o), nullptr, 1u))) continue;
        bool khpf_exp = false;
        const float khpf_env = lifetime_envelope(o, khpf_now, khpf_exp);
        if (khpf_exp) continue;
        RenderObject khpf_o = o;
        khpf_o.color[3] *= khpf_env;
        if (khpf_o.color[3] <= 0.001f) continue;
        if (khpf_o.effect == KH_EFFECT_LUT && !kh_user_lut_cached(kh_fx_shader_of(khpf_o))) continue;   // Not loaded by the flush yet.
        khpf_list.emplace_back(khpf_o.seq, khpf_o);
    }
    if (khpf_list.empty()) return;
    std::sort(khpf_list.begin(), khpf_list.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    // KH_PIP_ORIGIN: TopLeftX / TopLeftY must be 0 (held by every tested PIP): PSEffect takes SV_Position.xy as the
    // frame's pixel (clamping int2(SV_Position.xy) to [0, fxMeta.zw)) and its uv as SV_Position.xy / fxMeta.zw, so an
    // offset viewport's passes would read the scene, the depth and their masks at the wrong place. Such a PIP keeps the
    // engine's picture: refused here, with a pass to draw, ahead of the copies of its colour and depth (only these
    // passes read them) and of any state change, and reported once. Carrying the origin in a lane and subtracting it in
    // the shader would serve it.
    if (g_pip.vp.TopLeftX != 0.0f || g_pip.vp.TopLeftY != 0.0f) {
        // KH_EA_ERR_CLASS: an engine PIP configuration we do not serve - a warning (logged, never thrown).
        kh_log_once_safe("KH RenderIntegration: pip fx: a picture-in-picture viewport with an offset origin; no PIP "
                         "post passes");
        return;
    }

    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    if (!kh_ensure_ok("pip effect shader", kh_pip_fx_shader(dev)) || !g_res.ps_effect_pip) return;   // KH_EA_PIP_ONCE.

    // Declared before the OM save so the unwind restores the OM before the SRVs (destructors run in reverse declaration
    // order); captured later, ahead of our draws.
    StateBackup khpf_bk;
    KhOmSave khpf_om;
    khpf_om.capture(ctx);
    ID3D11Resource* khpf_rt = nullptr;
    if (khpf_om.rtv[0]) khpf_om.rtv[0]->GetResource(&khpf_rt);
    // KH_PIP_FX: a throw below (a pyramid build, a texture fill, an upload) reaches the hook's catch, which cannot
    // see this reference on the engine's PIP target - released on that unwind (KH_UI_BB_UNWIND, the backbuffer's
    // guard); every return below releases it itself.
    KhUnwindRelease<ID3D11Resource> khpf_ub(khpf_rt);
    bool khpf_ok = khpf_rt != nullptr && khpf_om.dsv != nullptr;
    if (khpf_ok) {   // The PIP's colour target: our copy of it is what every pass reads.
        ID3D11Texture2D* khpf_tex = nullptr;
        if (FAILED(khpf_rt->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khpf_tex))) || !khpf_tex) {
            khpf_ok = false;
        } else {
            D3D11_TEXTURE2D_DESC khpf_td = {};
            khpf_tex->GetDesc(&khpf_td);
            D3D11_RENDER_TARGET_VIEW_DESC khpf_rd = {};
            khpf_om.rtv[0]->GetDesc(&khpf_rd);
            khpf_ok = khpf_td.SampleDesc.Count == 1 && khpf_td.ArraySize == 1;
            if (khpf_ok && (!g_res.pip_scene_tex || !g_res.pip_scene_srv ||
                            !g_res.pip_scene_tex2 || !g_res.pip_scene_srv2 ||
                            !g_res.pip_scene_rtv[0] || !g_res.pip_scene_rtv[1] ||
                            g_res.pip_scene_w != khpf_td.Width || g_res.pip_scene_h != khpf_td.Height ||
                            g_res.pip_scene_fmt != khpf_rd.Format)) {
                KH_SAFE_RELEASE(g_res.pip_scene_srv);
                KH_SAFE_RELEASE(g_res.pip_scene_tex);
                KH_SAFE_RELEASE(g_res.pip_scene_srv2);
                KH_SAFE_RELEASE(g_res.pip_scene_tex2);
                KH_SAFE_RELEASE(g_res.pip_scene_rtv[0]);
                KH_SAFE_RELEASE(g_res.pip_scene_rtv[1]);
                g_res.pip_scene_w = g_res.pip_scene_h = 0;
                g_res.pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
                D3D11_TEXTURE2D_DESC khpf_cd = {};
                khpf_cd.Width = khpf_td.Width;
                khpf_cd.Height = khpf_td.Height;
                khpf_cd.MipLevels = 1;
                khpf_cd.ArraySize = 1;
                khpf_cd.Format = khpf_rd.Format;   // The view's typed format (ensure_scene_capture_from's rule).
                khpf_cd.SampleDesc.Count = 1;
                khpf_cd.Usage = D3D11_USAGE_DEFAULT;
                // Both buffers are read by one pass and written by the next.
                khpf_cd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                if (FAILED(dev->CreateTexture2D(&khpf_cd, nullptr, &g_res.pip_scene_tex)) || !g_res.pip_scene_tex ||
                    FAILED(dev->CreateShaderResourceView(g_res.pip_scene_tex, nullptr, &g_res.pip_scene_srv)) || !g_res.pip_scene_srv ||
                    FAILED(dev->CreateRenderTargetView(g_res.pip_scene_tex, nullptr, &g_res.pip_scene_rtv[0])) || !g_res.pip_scene_rtv[0] ||
                    FAILED(dev->CreateTexture2D(&khpf_cd, nullptr, &g_res.pip_scene_tex2)) || !g_res.pip_scene_tex2 ||
                    FAILED(dev->CreateShaderResourceView(g_res.pip_scene_tex2, nullptr, &g_res.pip_scene_srv2)) || !g_res.pip_scene_srv2 ||
                    FAILED(dev->CreateRenderTargetView(g_res.pip_scene_tex2, nullptr, &g_res.pip_scene_rtv[1])) || !g_res.pip_scene_rtv[1]) {
                    KH_SAFE_RELEASE(g_res.pip_scene_srv);
                    KH_SAFE_RELEASE(g_res.pip_scene_tex);
                    KH_SAFE_RELEASE(g_res.pip_scene_srv2);
                    KH_SAFE_RELEASE(g_res.pip_scene_tex2);
                    KH_SAFE_RELEASE(g_res.pip_scene_rtv[0]);
                    KH_SAFE_RELEASE(g_res.pip_scene_rtv[1]);
                    report_error_once_safe("KH RenderIntegration: pip fx: copy of the PIP colour target could not be "
                                           "created; no PIP post passes");
                    khpf_ok = false;
                } else {
                    g_res.pip_scene_w = khpf_td.Width;
                    g_res.pip_scene_h = khpf_td.Height;
                    g_res.pip_scene_fmt = khpf_rd.Format;
                }
            }
            khpf_tex->Release();
        }
    }
    if (khpf_ok) khpf_ok = kh_pip_fx_depth(dev, ctx, khpf_om.dsv);
    if (!khpf_ok) {
        if (khpf_rt) khpf_rt->Release();
        khpf_om.release();
        return;
    }

    // The PIP camera as kh_pip_inject builds it: the rotation rows on camera-relative positions. The inverse is of
    // exactly that (small numbers, exact in fp32) and fxCam carries the PIP camera the shader adds back.
    float khpf_v[4][4] = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            khpf_v[r][c] = g_pip.view_rows[c * 4 + r];
    khpf_v[3][3] = 1.0f;
    float khpf_p[4][4];
    memcpy(khpf_p, g_pip.proj, sizeof(khpf_p));

    ConstantData khpf_cbf = g_pip.tpl;
    mul_4x4(khpf_v, khpf_p, khpf_cbf.view_proj);
    kh_fill_user_cam_cb(khpf_cbf, g_pip.cam, khpf_v);   // KH_USER_LANES: the PIP camera.
    if (!inverse_4x4(khpf_cbf.view_proj, khpf_cbf.inv_view_proj)) {
        khpf_rt->Release();
        khpf_om.release();
        return;
    }
    khpf_cbf.fx_cam[0] = g_pip.cam[0];
    khpf_cbf.fx_cam[1] = g_pip.cam[1];
    khpf_cbf.fx_cam[2] = g_pip.cam[2];
    khpf_cbf.fx_cam[3] = 1.0f;
    {   // KH_LOCAL_CULL: the passes whose volume the PIP's view cannot reach (its own pair: khpf_p's, below).
        KhLocalCull khpf_lc;
        if (kh_local_cull_make(khpf_lc, khpf_cbf.view_proj, khpf_cbf.fx_cam, false, 0.0f, 0.0f)) {
            khpf_list.erase(std::remove_if(khpf_list.begin(), khpf_list.end(),
                                           [&khpf_lc](const std::pair<uint64_t, RenderObject>& khpf_e) {
                                               return kh_local_culled(khpf_lc, khpf_e.second);
                                           }),
                            khpf_list.end());
        }
        if (khpf_list.empty()) {   // Nothing reaches the view: the PIP stays as the engine drew it.
            khpf_rt->Release();
            khpf_om.release();
            return;
        }
    }
    khpf_cbf.fog_color[3] = g_pip.cam[1];                        // KhFsFog's camera altitude: the PIP's, not the
                                                                 // template's main camera (the flush's khf_cam_y twin).
    khpf_cbf.snap_cam[3] = 0.0f;                                 // The same disarms as the mesh injection.
    memset(khpf_cbf.mask_meta, 0, sizeof(khpf_cbf.mask_meta));
    khpf_cbf.lighting2[3] = 0.0f;
    memset(khpf_cbf.sten_vol2, 0, sizeof(khpf_cbf.sten_vol2));
    memset(khpf_cbf.mir_meta, 0, sizeof(khpf_cbf.mir_meta));

    // The PIP's viewport: the screen size every pass's uv assumes and the depth range its depth was encoded through -
    // the pass's own, captured during it; its origin is 0 (KH_PIP_ORIGIN, checked ahead of the copies).
    const D3D11_VIEWPORT khpf_vp = g_pip.vp;
    if (khpf_vp.Width < 2.0f || khpf_vp.Height < 2.0f) {
        khpf_rt->Release();
        khpf_om.release();
        return;
    }
    UINT khpf_nvp_cur = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khpf_vp_cur[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&khpf_nvp_cur, khpf_vp_cur);   // Restored after our draws (StateBackup restores them too).

    const bool khpf_pinj = g_ro.in_injection;
    g_ro.in_injection = true;   // Our own draws: the hooks stand aside.
    khpf_bk.capture(ctx);
    uint32_t khpf_drawn = 0;
    if (kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khpf_cbf)) {
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_effect_pip, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ID3D11Buffer* khpf_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
        ctx->VSSetConstantBuffers(0, 2, khpf_cbs);
        ctx->PSSetConstantBuffers(0, 2, khpf_cbs);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT khpf_bf[4] = { 0, 0, 0, 0 };
        // Compositing is in-shader (the flush's rule); RGB only - the PIP's
        // alpha is the engine's, read by its post pass.
        ctx->OMSetBlendState(g_res.blend_ui_replace ? g_res.blend_ui_replace : nullptr, khpf_bf, 0xFFFFFFFF);
        ctx->RSSetState(g_res.rasterizer);
        ctx->PSSetShaderResources(1, 1, &g_res.pip_depth_srv);
        ctx->RSSetViewports(1, &khpf_vp);   // KH_PIP_FX_LEAVE_VP.
        bool khpf_lut_bound = false;
        // One copy of the PIP per cycle; the chain ping-pongs between the two buffers and its last pass writes the PIP
        // target itself. A pass that fails to upload ends the chain with the PIP untouched (the buffers hold the
        // partial result nobody reads).
        ctx->CopyResource(g_res.pip_scene_tex, khpf_rt);
        ID3D11ShaderResourceView* khpf_src = g_res.pip_scene_srv;
        int khpf_wr = 1;   // The next intermediate write buffer: [1] first, then alternating.
        for (size_t khpf_fi = 0; khpf_fi < khpf_list.size(); ++khpf_fi) {
            const RenderObject& o = khpf_list[khpf_fi].second;
            const bool khpf_last = khpf_fi + 1 == khpf_list.size();
            ID3D11ShaderResourceView* khpf_lut = nullptr;
            if (o.effect == KH_EFFECT_LUT) {
                khpf_lut = kh_user_lut_cached(kh_fx_shader_of(o));
                if (!khpf_lut) break;   // Resolved at the gather; a release since ends the chain.
            }
            // KH_FX_USER: a user pass's single-sample build (the PIP depth is a copy), ready at the gather.
            ID3D11PixelShader* khpf_ufx = nullptr;
            bool khpf_upyr = false;
            if (o.effect == KH_EFFECT_CUSTOM) {
                khpf_ufx = kh_user_fx_ps(dev, kh_fx_shader_of(o), &khpf_upyr, 1u);
                if (!khpf_ufx) break;   // As a LUT released since.
            }
            ConstantData cbd = khpf_cbf;
            cbd.center_size[3] = 1.0f;   // A chain pass: the packing tail composites and writes opaque.
            cbd.size_axes[3] = static_cast<float>(o.blend_mode);   // The tail's blend mode (upload_cb's twin);
                                                                   // the template's zero read every pass as normal.
            kh_fill_fx_params_cb(cbd, o);
            if (o.effect == static_cast<int>(EffectId::Fogscatter)) kh_fogscatter_pack(cbd, khpf_list);
            cbd.fx_meta[0] = static_cast<float>(o.effect);
            cbd.fx_meta[1] = kh_fx_time(o, khpf_now_d);
            cbd.fx_meta[2] = khpf_vp.Width;
            cbd.fx_meta[3] = khpf_vp.Height;
            cbd.depth_params[0] = khpf_p[2][2];   // The harvested projection: the pair the PIP depth was encoded with.
            cbd.depth_params[1] = khpf_p[3][2];
            cbd.depth_params[2] = khpf_vp.MinDepth;
            cbd.depth_params[3] = khpf_vp.MaxDepth;
            kh_fill_local_band_cb(cbd, o);
            fill_lighting_obj_cb(ctx, cbd, o);
            // KH_GLOW_PYR: the scene chain's rule, on this pass's own source.
            bool khpf_gl = false;
            if (kh_glow_effect(o.effect) || khpf_upyr) {   // KH_FX_USER: or a user pass's.
                khpf_gl = kh_glow_build(dev, ctx, g_res.glow_pyr[2], khpf_src, cbd.fx_meta[2], cbd.fx_meta[3],
                                        cbd.center_size[3], o.effect, cbd.fx0[0], g_res.composite_cb);
                kh_glow_lanes(cbd, khpf_gl ? &g_res.glow_pyr[2] : nullptr);
            }
            // KH_ANA_PYR: anamorphic's one-axis pyramid, the same rule.
            const KhAnaPyr* khpf_an = nullptr;
            if (o.effect == static_cast<int>(EffectId::Anamorphic)) {
                const int khpf_ax = cbd.fx1[0] > 0.5f ? 1 : 0;
                if (kh_ana_build(dev, ctx, g_res.ana_pyr[2][khpf_ax], khpf_src, cbd.fx_meta[2], cbd.fx_meta[3],
                                 cbd.center_size[3], khpf_ax, cbd.fx0[0], g_res.composite_cb))
                    khpf_an = &g_res.ana_pyr[2][khpf_ax];
                kh_ana_lanes(cbd, khpf_an);
            }
            ID3D11ShaderResourceView* khpf_tex[KH_MAT_USER_MAPS] = {};   // KH_FX_TEX.
            const bool khpf_tx = kh_fx_tex_on(o);   // KH_FX_TEX_ALL: a user pass, or a builtin one with a set.
            if (khpf_tx) kh_fx_tex_fill(dev, ctx, cbd, o, khpf_tex);
            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            if (khpf_lut) { ctx->PSSetShaderResources(19, 1, &khpf_lut); khpf_lut_bound = true; }
            // Unbind the source before its sibling becomes the target (the scene chain's hazard pattern), then the last
            // pass takes the PIP's own targets back.
            ID3D11ShaderResourceView* khpf_null = nullptr;
            ctx->PSSetShaderResources(0, 1, &khpf_null);
            if (khpf_last) khpf_om.restore(ctx);
            else           ctx->OMSetRenderTargets(1, &g_res.pip_scene_rtv[khpf_wr], nullptr);
            ctx->PSSetShaderResources(0, 1, &khpf_src);
            KhGlowBind khpf_gb;   // KH_GLOW_PYR.
            if (khpf_gl) khpf_gb.bind(ctx, g_res.glow_pyr[2]);
            KhAnaBind khpf_ab;   // KH_ANA_PYR.
            if (khpf_an) khpf_ab.bind(ctx, *khpf_an);
            ctx->PSSetShader(khpf_ufx ? khpf_ufx : g_res.ps_effect_pip, nullptr, 0);   // KH_FX_USER: per pass.
            if (khpf_ufx) ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_FX_USER: s1 for every user effect draw.
            if (khpf_tx) kh_fx_tex_bind(ctx, khpf_tex);   // KH_FX_TEX.
            ctx->Draw(3, 0);
            if (khpf_tx) kh_fx_tex_unbind(ctx);
            khpf_ab.unbind();
            khpf_gb.unbind();
            ++khpf_drawn;
            if (!khpf_last) {
                khpf_src = khpf_wr == 1 ? g_res.pip_scene_srv2 : g_res.pip_scene_srv;
                khpf_wr ^= 1;
            }
        }
        if (khpf_lut_bound) { ID3D11ShaderResourceView* khpf_n19 = nullptr; ctx->PSSetShaderResources(19, 1, &khpf_n19); }
        {   // Never leave a chain buffer bound as t0 or as the target.
            ID3D11ShaderResourceView* khpf_n0 = nullptr;
            ctx->PSSetShaderResources(0, 1, &khpf_n0);
        }
    }
    khpf_om.restore(ctx);   // The intermediates were bound mid-chain; the PIP's targets come back.
    khpf_bk.restore(ctx);
    if (khpf_nvp_cur > 0) ctx->RSSetViewports(khpf_nvp_cur, khpf_vp_cur);   // KH_PIP_FX_LEAVE_VP.
    g_ro.in_injection = khpf_pinj;
    khpf_rt->Release();
    khpf_om.release();
    if (khpf_drawn) kh_stat_add(g_stats.pip_fx, khpf_drawn);
}

// At every OM set on the render thread, after g_ro.dsv_main is known.
inline void kh_pip_track_targets(ID3D11DeviceContext* ctx, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv) {
    // The binding that leaves a PIP pass (off, or another PIP's RT0) draws the passes into it first - the PIP's targets
    // are still bound, this hook runs ahead of the engine's call.
    const bool khpt_was = g_pip.on;
    void* const khpt_was_rtv = g_pip.cur_rtv;
    if (!dsv || g_ro.dsv_main || !g_main_depth_identity || n < 1 || !rtvs || !rtvs[0]) {
        if (khpt_was && !g_pip.fx_done) kh_pip_fx(ctx);
        g_pip.on = false;
        g_pip.cur_rtv = nullptr;
        return;
    }
    void* khpt_id = reorder_dsv_identity(dsv);
    void* khpt_rtv = static_cast<void*>(rtvs[0]);
    if (khpt_was && khpt_rtv != khpt_was_rtv && !g_pip.fx_done) kh_pip_fx(ctx);
    for (uint32_t i = 0; i < g_pip.cache_n; ++i) {
        const KhPipState::Key& k = g_pip.cache[i];
        if (k.dsv == khpt_id && k.rtv == khpt_rtv && k.fmt == g_pip.scene_fmt) {   // KH_PIP_FMT.
            g_pip.on = k.pip;
            g_pip.cur_rtv = k.pip ? khpt_rtv : nullptr;
            if (k.pip) { g_pip.w = k.w; g_pip.h = k.h; }
            return;
        }
    }
    bool khpt_pip = false;
    uint32_t khpt_w = 0, khpt_h = 0;
    ID3D11Resource* khpt_res = nullptr;
    rtvs[0]->GetResource(&khpt_res);
    if (khpt_res) {
        if (static_cast<void*>(khpt_res) != g_topo_scene_tex_id) {
            ID3D11Texture2D* khpt_tex = nullptr;
            if (SUCCEEDED(khpt_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khpt_tex))) && khpt_tex) {
                D3D11_TEXTURE2D_DESC td = {};
                khpt_tex->GetDesc(&td);
                // KH_PIP_FMT: the HDR target kind - RGBA16F or the main scene's own format.
                const bool khpt_hdr = td.Format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
                                      (g_pip.scene_fmt != 0u && static_cast<uint32_t>(td.Format) == g_pip.scene_fmt);
                khpt_pip = khpt_hdr && td.SampleDesc.Count == 1 && td.Width >= 64 && td.Height >= 64;
                khpt_w = td.Width; khpt_h = td.Height;
                khpt_tex->Release();
            }
        }
        khpt_res->Release();
    }
    KhPipState::Key k = { khpt_id, khpt_rtv, khpt_pip, khpt_w, khpt_h, g_pip.scene_fmt };
    if (g_pip.cache_n < 8) g_pip.cache[g_pip.cache_n++] = k;
    else g_pip.cache[g_topo_cycles & 7] = k;   // A cold slot; the cache is a hint.
    g_pip.on = khpt_pip;
    g_pip.cur_rtv = khpt_pip ? khpt_rtv : nullptr;
    if (khpt_pip) { g_pip.w = khpt_w; g_pip.h = khpt_h; g_pip_seen = true; }
}

// A depth clear on a non-main target opens a PIP cycle (whether the target is a
// PIP one is decided at the OM hook, which precedes the draws).
inline void kh_pip_on_clear() {
    g_pip.opaques = 0;
    g_pip.injected = false;
    g_pip.fx_done = false;   // KH_PIP_FX.
    g_pip.vp_valid = false;  // KH_PIP_FX_LEAVE_VP.
}

inline void kh_pip_inject(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_PIP);
    KH_GPU_SCOPE(ctx, KHG_PIP);   // KH_PROF.
    g_pip.injected = true;   // One attempt per cycle, whatever the gates say.
    if (!g_pip.tpl_valid || !g_pip.view_valid || !g_pip.proj_valid) return;
    if (g_topo_cycles < g_pip.tpl_cycle || g_topo_cycles - g_pip.tpl_cycle > KH_PIP_TPL_MAX_CYCLES) return;
    if (!g_res.vs || !g_res.ps || !g_res.composite_cb || !g_res.composite_frame_cb ||
        !g_res.input_layout || !g_res.dss_test_write || !g_res.rasterizer || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    kh_skin_cull_catchup(ctx);   // KH_SKIN_CULL: no step precedes the PIP; its own bracket, ahead of this pass's.
    g_skin_cull_pip_cycle = g_topo_cycles;   // KH_SKIN_CULL: while PIPs draw, the steps do not cull.

    const bool khpi_pinj = g_ro.in_injection;
    g_ro.in_injection = true;   // Our own draws: the hooks stand aside.
    StateBackup khpi_bk;
    khpi_bk.capture(ctx);
    KhBindScope khpi_bc(ctx);   // KH_BIND_FILTER.
    KhOmSlot0 khpi_s0;   // KH_MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    khpi_s0.open(ctx);

    // Camera-relative view (camera at the origin; center_rel carries the offset per object) times the harvested
    // projection: the rebased contract the two mesh passes draw under.
    float khpi_v[4][4] = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            khpi_v[r][c] = g_pip.view_rows[c * 4 + r];   // Row-vector: the transpose of the engine's rows.
    khpi_v[3][3] = 1.0f;
    float khpi_p[4][4];
    memcpy(khpi_p, g_pip.proj, sizeof(khpi_p));
    const float* cam = g_pip.cam;
    // KH_PIP_LOD: the pass's own pixels-per-tangent for the level pick - the PIP's projection and the height of the
    // viewport this pass draws our meshes with (KH_LOD_ONE_SCALE's definition, KH_EA_PIP_LOD_VP; the target's height
    // where none is bound), the main passes' formula (not g_lod_proj_px: the main view's, which a zoomed or smaller
    // PIP does not share - its meshes would draw at the main view's levels for the PIP camera's distance). An unknown
    // height reads as level 0, as an unpublished g_lod_proj_px does.
    UINT khpi_nvp = 1;
    D3D11_VIEWPORT khpi_dvp = {};
    ctx->RSGetViewports(&khpi_nvp, &khpi_dvp);
    const float khpi_lod_h = (khpi_nvp >= 1 && khpi_dvp.Height >= 1.0f) ? khpi_dvp.Height
                                                                          : static_cast<float>(g_pip.h);
    const float khpi_lod_px = (khpi_lod_h > 0.0f && khpi_p[1][1] > 0.0f)
                            ? khpi_p[1][1] * khpi_lod_h * 0.5f : 0.0f;
    const float khpi_lod_zoom = kh_lod_zoom_of(khpi_p[1][1]);   // KH_LOD_ZOOM: the PIP camera's own.

    ConstantData khpi_cbf = g_pip.tpl;
    mul_4x4(khpi_v, khpi_p, khpi_cbf.view_proj);
    kh_fill_user_cam_cb(khpi_cbf, cam, khpi_v);   // KH_USER_LANES: the PIP camera.
    khpi_cbf.snap_cam[3] = 0.0f;                              // No scene snapshot here.
    memset(khpi_cbf.mask_meta, 0, sizeof(khpi_cbf.mask_meta));   // No band table, no mask stream.
    // With the band table off PSMain's received term falls to ShadowMapFactor - the engine atlas at t1 scaled by
    // lighting2.w. The atlas is not bound here (it is the main view's) and an unbound t1 reads as fully occluded, so
    // the strength is zeroed: the engine's own shadows on our meshes are not received in a PIP pass; ours (the sun
    // ladder, the DLS maps) are.
    khpi_cbf.lighting2[3] = 0.0f;
    memset(khpi_cbf.sten_vol2, 0, sizeof(khpi_cbf.sten_vol2));   // No volume transport; the plain vertex path.
    // No mirror: its lanes (xyz) go. KH_PIP_SUN_FADE: mirMeta.w is the sun's range-fade radius, a world-space lane
    // about sunOrigin - the main camera the ladder's windows are fitted round, which the template carries - so it
    // stays: our meshes' sun shadows fade in a PIP where they fade in the main view (as the light maps' fade, dlsRange,
    // does), instead of ending at the far tier's square edge.
    khpi_cbf.mir_meta[0] = 0.0f;
    khpi_cbf.mir_meta[1] = 0.0f;
    khpi_cbf.mir_meta[2] = 0.0f;
    khpi_cbf.fx_meta[2] = static_cast<float>(g_pip.w);
    khpi_cbf.fx_meta[3] = static_cast<float>(g_pip.h);
    // Bucket pass lanes (the injection's twin): the rebase camera the instanced VS subtracts, armed exactly as
    // center_rel is per object below. The template carried the main pass's camera and arm, never this one's - left as
    // it was, every bucket instance would land a camera vector away.
    khpi_cbf.kh_pass[0] = cam[0]; khpi_cbf.kh_pass[1] = cam[1]; khpi_cbf.kh_pass[2] = cam[2];
    // The fog's camera altitude: the PIP's, not the template's main camera (kh_pip_fx's twin). PSMain's fog measures
    // its height terms, its below-layer split and its elevation from it, and fxParams0 below is the PIP camera.
    khpi_cbf.fog_color[3] = cam[1];
    khpi_cbf.kh_pass[3] = 1.0f;
    kh_scene_sync();   // KH_PF_NEED: the scene this pass draws, synced ahead of its convert and binds.
    // KH_PF_STAGES: the engine's GS / HS / DS are live in this hook and the convert sets none of them (a bound one
    // drops its draws, and the pyramid commits as valid); nulled first, as the injection and the flush do.
    // StateBackup restores them.
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    kh_sun_pf_convert(ctx, &khpi_cbf);   // KH_PF_NEED: a pyramid this pass's receivers read converts here.
    kh_fill_sun_pf_lanes(khpi_cbf);   // KH_PF_NEED: the arms say what the binds below bind.
    if (!kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khpi_cbf)) {
        khpi_s0.close(ctx);   // KH_MESH_O1: under the hooks' exclusion.
        khpi_bk.restore(ctx);
        g_ro.in_injection = khpi_pinj;
        return;
    }

    const UINT stride = sizeof(MeshVertex);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(g_res.ps, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* khpi_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khpi_cbs);
    ctx->PSSetConstantBuffers(0, 2, khpi_cbs);
    // World-space products, as the injection binds them (no depth witness at t0, no bands: mask_meta says so).
    if (g_res.dls_srv) ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    if (g_sun2_map_valid && g_sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29, 1, &g_res.sun_pf_srv[0]);
    if (g_sun3_map_valid && g_sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30, 1, &g_res.sun_pf_srv[1]);
    if (g_sun4_map_valid && g_sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31, 1, &g_res.sun_pf_srv[2]);
    if (g_sun5_map_valid && g_sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20, 1, &g_res.sun_pf_srv[3]);
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER: null = the default state, also clamp.
    if (g_thm_valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* khpi_bound_rs = g_res.rasterizer;
    const FLOAT khpi_bf[4] = { 0, 0, 0, 0 };
    int khpi_bound_bm = -1;
    KhVbRef khpi_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
    ID3D11VertexShader* khpi_bound_vs = g_res.vs;
    ID3D11InputLayout*  khpi_bound_il = g_res.input_layout;
    ID3D11ShaderResourceView* khpi_bound_sk = nullptr;   // KH_SKIN_POOL: VS t52 (kh_inst_sk_bind).
    ID3D11PixelShader*  khpi_bound_ps = g_res.ps;
    const float khpi_now = effect_time_seconds();
    uint32_t khpi_drawn = 0;

    kh_objbuf_sync(ctx);   // KH_OBJBUF: the records this pass's buckets index (scene synced above).
    kh_objbuf_bind(ctx);   // VS t39 (StateBackup restores it).

    // The injection's structure on the PIP camera: the grid pre-cull against this pass's frustum (an invalid cull set
    // returns every cell), the eligible copies with the envelope folded, front to back, then buckets through the static
    // instanced twins (this pass draws the static pair: route 0 - no guard, arb or near-gap; a mesh beyond the PIP's
    // far draws clamped at its MaxDepth, unordered) and the per-object loop at each bucket's representative. Opaque
    // only, by design: the set is the injection's opaque one, and a blend material draws its solid part alone (part 1)
    // - this pass has no translucent tail, so glass texels are not drawn in a PIP.
    static std::vector<RenderObject> khpi_meshes;
    static std::vector<uint32_t> khpi_cand;
    khpi_meshes.clear();
    {
        KhCullSet khpi_cull;
        kh_cull_build(khpi_cbf.view_proj, cam, khpi_cull);   // Rebased: the apex is the origin.
        kh_scene_grid_query(khpi_cull, khpi_cand);
    }
    for (uint32_t khpi_s : khpi_cand) {
        if (!g_scene.alive[khpi_s]) continue;
        const RenderObject& o = g_scene.objs[khpi_s];
        if (!o.visible || !is_composite_eligible(o)) continue;
        bool khpi_expired = false;
        const float khpi_env = lifetime_envelope(o, khpi_now, khpi_expired);
        if (khpi_expired) continue;
        if (!(o.blend_mode == 0 && o.color[3] * khpi_env >= 0.999f)) continue;   // The injection's own set.
        const int mid = mesh_id_clamp(o.mesh);
        if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size()) continue;
        if (!kh_mesh_vb_ref(mid, o.slot)) continue;   // KH_CLOTH: no buffer, nothing to gather.
        khpi_meshes.push_back(o);
        khpi_meshes.back().color[3] *= khpi_env;
    }
    kh_mesh_order(khpi_meshes, cam, g_dist_memo_inj);   // KH_MESH_ORDER: opaque front to back for early-Z,
                                                        // the rest back to front; the memo begins here, seeded.
    g_vis_memo_inj.begin(khpi_meshes);

    // Bounding sphere against the PIP camera's frustum, in view space (rel = engine-order position minus the camera,
    // through the view rows), with the harvested projection's x / y scales as the side planes and the near plane at z =
    // 0. The pass's one visibility truth: the material ensure, the plan's emit and the loop all ask it, memoised per
    // object.
    const float khpc_m00 = khpi_p[0][0], khpc_m11 = khpi_p[1][1];
    const float khpc_nx = sqrtf(khpc_m00 * khpc_m00 + 1.0f);
    const float khpc_ny = sqrtf(khpc_m11 * khpc_m11 + 1.0f);
    auto khpi_vis_calc = [&](const RenderObject& khpv_o) -> bool {
        const float khpc_rel[3] = { khpv_o.pos[0] - cam[0], khpv_o.pos[2] - cam[1], khpv_o.pos[1] - cam[2] };
        float khpc_bs[3];
        kh_bounds_size_of(khpv_o, khpc_bs);   // KH_CAST_REFIT.
        const float khpc_r = kh_lod_radius_of(khpc_bs, khpv_o.rot_m, khpv_o.rotated);
        float khpc_v[3];
        for (int c = 0; c < 3; ++c) {
            const float* row = g_pip.view_rows + c * 4;
            khpc_v[c] = row[0] * khpc_rel[0] + row[1] * khpc_rel[1] + row[2] * khpc_rel[2];
        }
        if (khpc_v[2] + khpc_r < 0.0f) return false;                                      // Behind the camera.
        if ((khpc_v[2] - khpc_m00 * khpc_v[0]) / khpc_nx < -khpc_r) return false;        // Right of the frustum.
        if ((khpc_v[2] + khpc_m00 * khpc_v[0]) / khpc_nx < -khpc_r) return false;        // Left.
        if ((khpc_v[2] - khpc_m11 * khpc_v[1]) / khpc_ny < -khpc_r) return false;        // Above.
        if ((khpc_v[2] + khpc_m11 * khpc_v[1]) / khpc_ny < -khpc_r) return false;        // Below.
        return true;
    };
    auto khpi_vis = [&](const RenderObject& khpv_o) -> bool {
        return g_vis_memo_inj.get(khpv_o, khpi_vis_calc);
    };

    // Buckets, PIP edition: the static instanced twins (all-or-nothing, the flush's gate), the plan keyed with route 0
    // and the light identity, the emission once against this frustum. Twin at the injection.
    const bool khpi_inst_on = kh_inst_twins_on();
    static KhInstPlan khpi_plan;
    bool khpi_inst_vb_bound = false;
    if (khpi_inst_on) {
        kh_mat_ensure_all(ctx, dev, khpi_meshes, khpi_vis);   // KH_MAT_TABLE: bases fixed before any lane is written.
        kh_inst_plan_build(khpi_plan, khpi_meshes, nullptr, khpi_meshes.size(),
                           [&](const RenderObject& khpp_o, KhInstKey& khpp_k) {
                               khpp_k.route = 0u;
                               khpp_k.aux = kh_dl_bucket_hash(khpp_o);
                           });
        kh_inst_plan_emit(ctx, dev, khpi_plan, khpi_meshes, cam, 1, khpi_vis, g_dist_memo_inj, khpi_lod_px,
                          khpi_lod_zoom);   // KH_LOD_ZOOM.
    }

    for (size_t khpi_oi = 0; khpi_oi < khpi_meshes.size(); ++khpi_oi) {
        const RenderObject& o = khpi_meshes[khpi_oi];
        // Carried by its bucket's draw at the representative, or culled by the
        // emission's walk (only the representative stays undone).
        if (khpi_inst_on && khpi_plan.done[khpi_oi]) continue;
        if (!khpi_vis(o)) continue;   // The emit's twin: an emitted representative always draws.
        const int mid = mesh_id_clamp(o.mesh);

        ConstantData cbd = khpi_cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float khpi_res[3];   // KH_POS_RES: the object's live residual.
        kh_pos_res(o, khpi_res);
        kh_fill_center_rel(cbd, cam, khpi_res);    // Arms center_rel.w: the rebased contract.
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        kh_fill_obj_rot(cbd, o.rot_m);
        cbd.blend_ctl[0] = 0.0f;
        cbd.blend_ctl[3] = 0.0f;
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        cbd.fx0[0] = cam[0]; cbd.fx0[1] = cam[1]; cbd.fx0[2] = cam[2];
        // PSMain's punch-through guard arms on fxParams1.x below 1e8 and reads the depth witness at t0 - the engine's
        // own binding here, never ours. Disarm it by value, as the injection does.
        cbd.fx1[0] = 1e9f;
        cbd.fx1[1] = 0.0f;
        kh_fill_local_band_cb(cbd, o);
        fill_lighting_obj_cb(ctx, cbd, o);
        const KhMaterialSet* khpi_txm = kh_obj_textured(o);
        const bool khpi_tx = khpi_txm && g_res.layout_tex && g_res.vs_tex && g_res.ps_tex && g_res.mat_sampler;
        // KH_TX_ONE_UPLOAD: textured, the slice goes up per submesh in kh_draw_textured. Twin: the injection.
        if (!khpi_tx && !kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;

        if (o.blend_mode != khpi_bound_bm) {
            ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], khpi_bf, 0xFFFFFFFF);
            khpi_bound_bm = o.blend_mode;
        }
        const MeshDef& md = mesh_def(mid);
        // The colour pass's own pick with its crossfade (the injection's twin;
        // a locked instance never picks), at the PIP's own scale (KH_PIP_LOD).
        int khpi_lod = 0;
        float khpi_lodt = 0.0f;
        if (!kh_lod_locked(o)) {   // KH_LOD_SIM_SLOT.
            kh_lod_pick(md, kh_lod_radius(o), sqrtf(g_dist_memo_inj.get(o, cam)), khpi_lod, khpi_lodt, khpi_lod_px,
                        khpi_lod_zoom);   // KH_LOD_ZOOM.
        }
        const bool khpi_lodx = khpi_lodt > 0.0f;
        {
            ID3D11PixelShader* khpi_want_ps = khpi_tx ? g_res.ps_tex : g_res.ps;
            if (khpi_want_ps != khpi_bound_ps) { ctx->PSSetShader(khpi_want_ps, nullptr, 0); khpi_bound_ps = khpi_want_ps; }
            ID3D11VertexShader* khpi_want_vs = khpi_tx ? g_res.vs_tex : g_res.vs;
            if (khpi_want_vs != khpi_bound_vs) { ctx->VSSetShader(khpi_want_vs, nullptr, 0); khpi_bound_vs = khpi_want_vs; }
            ID3D11InputLayout* khpi_want_il = khpi_tx ? g_res.layout_tex : g_res.input_layout;
            if (khpi_want_il != khpi_bound_il) { ctx->IASetInputLayout(khpi_want_il); khpi_bound_il = khpi_want_il; }
        }
        const KhVbRef khpi_vb = kh_mesh_vb_ref(mid, o.slot);   // KH_CLOTH.
        if (khpi_vb != khpi_bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khpi_vb.vb, &stride, &khpi_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            khpi_bound_vb = khpi_vb;
        }

        // This object is its bucket's representative: swap in the static instanced VS + layout, bind the lane ring once
        // per pass, draw the emitted ranges (every member visible at emission, at its own level, crossfade included).
        // Every draw here is CullNone. Twin at the injection.
        int khpi_lod_iters = khpi_lodx ? 2 : 1;
        if (khpi_inst_on && khpi_plan.batch_of[khpi_oi] >= 0) {
            // KH_SKIN_POOL: a skin pool chunk's bucket pulls (the injection's twin).
            const KhSkinPoolChunk* const khpi_ipc = kh_inst_bucket_pool(khpi_plan, static_cast<uint32_t>(khpi_oi));
            ID3D11VertexShader* khpi_ivs = khpi_ipc ? (khpi_tx ? g_res.vs_inst_sk_tex : g_res.vs_inst_sk)
                                                    : (khpi_tx ? g_res.vs_inst_tex : g_res.vs_inst);
            ID3D11InputLayout*  khpi_iil = khpi_ipc ? g_res.layout_inst_sk
                                                    : (khpi_tx ? g_res.layout_inst_tex : g_res.layout_inst);
            if (khpi_ivs && khpi_iil) {
                if (khpi_ivs != khpi_bound_vs) { ctx->VSSetShader(khpi_ivs, nullptr, 0); khpi_bound_vs = khpi_ivs; }
                if (khpi_iil != khpi_bound_il) { ctx->IASetInputLayout(khpi_iil); khpi_bound_il = khpi_iil; }
                kh_inst_sk_bind(ctx, khpi_ipc, khpi_bound_sk);
                if (!khpi_inst_vb_bound) {
                    const UINT khpi_is = KH_INST_STRIDE, khpi_io = 0;
                    ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[1], &khpi_is, &khpi_io);
                    khpi_inst_vb_bound = true;
                }
                uint32_t khpi_in = 0;
                khpi_drawn += kh_inst_draw(ctx, dev, khpi_plan, khpi_meshes, static_cast<uint32_t>(khpi_oi),
                                           cbd, g_res.composite_cb, khpi_tx ? khpi_txm : nullptr, false,
                                           khpi_bound_rs, 0, g_res.ps_tex, 1, khpi_in);
                if (khpi_ipc) kh_stat_add(g_stats.skin_inst, khpi_in);   // KH_SKIN_POOL.
                khpi_lod_iters = 0;   // The batch drew; no per-object level walk.
            } else {
                kh_inst_unbatch(khpi_plan, static_cast<uint32_t>(khpi_oi));   // KH_INST_UNBATCH.
            }
        }

        for (int khpi_li = 0; khpi_li < khpi_lod_iters; ++khpi_li) {
            const int khpi_lvl = khpi_lod + khpi_li;
            if (khpi_lodx) {
                cbd.blend_ctl[3] = kh_lod_dither(khpi_lodt, khpi_li != 0);
                // KH_TX_ONE_UPLOAD: textured, kh_draw_textured uploads the dither with each submesh.
                if (!khpi_tx && !kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            }
            if (khpi_tx) {
                // The static twin (ctx 0), the opaque part (1): the injection's own call.
                khpi_drawn += kh_draw_textured(ctx, dev, cbd, g_res.composite_cb, *khpi_txm, mid, false,
                                               khpi_bound_rs, 0, g_res.ps_tex, khpi_lvl, 1);
            } else {
                UINT khpi_ls = 0, khpi_lc = 0;
                mesh_lod_range(md, khpi_lvl, khpi_ls, khpi_lc);
                if (khpi_lc == 0) continue;
                ctx->DrawIndexed(khpi_lc, khpi_ls, 0);
                ++khpi_drawn;
            }
        }
        if (khpi_bound_rs != g_res.rasterizer) {
            ctx->RSSetState(g_res.rasterizer);
            khpi_bound_rs = g_res.rasterizer;
        }
    }
    if (khpi_drawn) {
        kh_stat(g_stats.pip_injections);
        kh_stat_add(g_stats.pip_meshes, khpi_drawn);
    }
    khpi_s0.close(ctx);   // KH_MESH_O1: under the hooks' exclusion.
    khpi_bk.restore(ctx);
    g_ro.in_injection = khpi_pinj;
}

// Every engine draw on a PIP pass (reorder_pre_draw, render thread).
inline void kh_pip_pre_draw(ID3D11DeviceContext* ctx) {
    if (g_ro.blend_translucent) {
        if (!g_pip.injected && g_pip.opaques >= KH_PIP_MIN_OPAQUES) kh_pip_inject(ctx);
    } else if (!g_ro.dss_nowrite) {
        // The pass's viewport, read once at the draw that makes it a real pass;
        // kh_pip_fx draws under it.
        if (++g_pip.opaques == KH_PIP_MIN_OPAQUES) {
            UINT khpp_n = 1;
            D3D11_VIEWPORT khpp_vp = {};
            ctx->RSGetViewports(&khpp_n, &khpp_vp);
            g_pip.vp_valid = khpp_n >= 1 && khpp_vp.Width >= 2.0f && khpp_vp.Height >= 2.0f;
            if (g_pip.vp_valid) g_pip.vp = khpp_vp;
        }
    }
}
}