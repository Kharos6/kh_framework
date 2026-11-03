#pragma once

// The injection half (rendering_integration_injection.hpp): PIP (picture-in-picture). Never include it on its own.
namespace RenderIntegration {

// PIP - our meshes in the engine's PIP cameras. A PIP is a full scene pass on its own depth (D24S8) and HDR colour
// targets (the main scene's kind: RGBA16F at HDR quality Standard), both single-sample, rendered BEFORE the main pass.
// Engine per-view block (VS b2): floats 0..15 rebase origin (1,0,0,X / 0,1,0,0 / 0,0,1,Z / 0,0,0,0), 16..27 the 3x4
// view [R | -R.cam] on rebased positions, 28..31 rebased camera (w = 1), 56..59 absolute camera (w = 1). Projection: b0
// floats 0..15, row-vector (m00 0 0 0 / 0 m11 0 0 / 0 0 m22 1 / 0 0 m32 0). Both are re-uploaded per view ahead of its
// draws, so the last such uploads before a PIP draw are the PIP camera's. The pass is recognised at the OM hook
// (non-main depth + RGBA16F or the main scene's own format, PIP_FMT - single-sample colour that is not the scene
// texture); its depth clear opens a cycle, opaques are counted, and the first blended draw after PIP_MIN_OPAQUES
// fires one injection: the previous main pass's frame template with the view lanes replaced and every screen-space lane
// disarmed (depth witness, band table, volume stencil, mirror, snapshot); world-space products (sun ladder, DLS maps,
// terrain, lighting) are shared. PIP_FX: localized fullscreen passes run in the PIP when the engine COPIES THE PIP
// COLOUR TARGET OUT (hooked_copyresource), once, at the first copy of a cycle - it copies mid-pass for its translucents
// and after its last draw for its post pass, which overwrites the target from that copy, so anything drawn at the OM
// leave is lost; the leave stays as the fallback for a PIP that is never copied. They run through PSEffect over a copy
// of the PIP colour (t0: one copy per cycle, the chain ping-ponging between two buffers, its last pass writing the PIP
// target) and of its depth (t1), with the PIP camera in fxCam, under the PIP's own viewport captured during the pass,
// RGB only. The SSGI chain and dynamicLightFog stay main-view only; a user (.hlsl) pass runs once its single-sample
// build is ready (FX_USER), a LUT pass once the flush has loaded its file.
static constexpr uint32_t PIP_MIN_OPAQUES = 32;     // Opaque draws on the PIP depth before a fire.
static constexpr uint32_t PIP_TPL_MAX_CYCLES = 2;   // A template older than this is stale.

struct PipState {
    // The pass, from the OM hook.
    bool     on = false;               // The bound targets are a PIP pass.
    void*    cur_rtv = nullptr;        // PIP_FX: RT0 of the PIP pass currently bound (weak; a change is a leave).
    uint32_t w = 0, h = 0;             // Its colour target size.
    D3D11_VIEWPORT vp = {};            // PIP_FX_LEAVE_VP: the pass's viewport, read at its MIN_OPAQUES-th opaque draw.
    bool     vp_valid = false;
    bool     fx_done = false;          // PIP_FX: the passes were attempted this PIP cycle.
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
    // PIP_FMT: the main scene colour target's format (a DXGI_FORMAT; 0 = not seen yet), pip_scene_fmt_note.
    uint32_t scene_fmt = 0;
    // Classification cache: (dsv identity, rtv0) -> verdict, with the scene_fmt it was made under (fmt): a
    // verdict made before the format was learned, or under another, is not reused.
    struct Key { void* dsv; void* rtv; bool pip; uint32_t w, h; uint32_t fmt; };
    Key      cache[8] = {};
    uint32_t cache_n = 0;
};
static PipState g_pip;

inline void pip_reset() {
    g_pip = PipState();
    g_pip_seen = false;
}
// PIP_FMT - the main scene colour target's format, read wherever scene_tex_name names the scene texture: where
// the injection draws into it (its RT0 at the landing, the resource the census latches) and at a scene resolve
// main_depth_readopt recognises (its source). A PIP pass renders the scene into a target of the same kind, and
// that kind follows the video options' HDR quality: RGBA16F at Standard, another format at Low, where a fixed
// RGBA16F test recognises no PIP and none of our meshes appear in one. Render thread (both namers'); the
// reference is the caller's, live for the call.
inline void pip_scene_fmt_note(ID3D11Resource* res) {
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) ||
        !tex) return;
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    tex->Release();
    g_pip.scene_fmt = static_cast<uint32_t>(td.Format);
}

// The two engine blocks by signature (see the PIP note). A handful of exact compares before anything else.
inline void pip_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 64) return;
    if (res == static_cast<ID3D11Resource*>(g_res.composite_cb) ||
        res == static_cast<ID3D11Resource*>(g_res.constant_buffer) ||
        res == static_cast<ID3D11Resource*>(g_res.frame_cb) ||
        res == static_cast<ID3D11Resource*>(g_res.composite_frame_cb)) return;
    const float* f = static_cast<const float*>(data);
    const uint32_t nf = bytes / 4;
    upload_need(data, (nf < 60u ? nf : 60u) * 4u);   // UPLOAD_LAZY: floats 0..59.
    // The view block: rebase rows, then [R | t] with w-lanes 1 at 31 and 59.
    if (nf >= 60 && f[0] == 1.0f && f[1] == 0.0f && f[2] == 0.0f &&
        f[4] == 0.0f && f[5] == 1.0f && f[6] == 0.0f && f[7] == 0.0f &&
        f[8] == 0.0f && f[9] == 0.0f && f[10] == 1.0f &&
        f[12] == 0.0f && f[13] == 0.0f && f[14] == 0.0f && f[15] == 0.0f &&
        f[31] == 1.0f && f[59] == 1.0f) {
        bool ok = true;
        for (int r = 0; r < 3 && ok; ++r) {
            const float* w = f + 16 + r * 4;
            const float n = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
            if (n < 0.98f || n > 1.02f) ok = false;
        }
        if (ok) {
            // The absolute camera must be the rebased one plus the origin.
            const float dx = fabsf((f[28] + f[3]) - f[56]);
            const float dz = fabsf((f[30] + f[11]) - f[58]);
            if (dx < 0.5f && dz < 0.5f) {
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

// PIP_FX: PSEffect against a single-sample depth; a multisampled PIP depth stands the pass down rather than reading
// it through the wrong declaration. The prewarm table carries this exact define set. One attempt per device: a failed
// one is reported once by its caller's ensure_ok, after which "" with no shader stands the passes down quietly.
inline std::string pip_fx_shader(ID3D11Device* dev) {
    if (g_res.ps_effect_pip || g_res.ps_effect_pip_tried) return "";   // EA_PIP_ONCE.
    g_res.ps_effect_pip_tried = true;
    const D3D_SHADER_MACRO def[] = { { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    const std::string src = hlsl_src(HLSL_CB) + hlsl_src(HLSL_EFFECT).c_str();
    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(src.c_str(), "PSEffect", "ps_5_0", def, &blob);
    if (!err.empty()) return err;
    const HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_effect_pip);
    blob->Release();
    if (FAILED(hr)) { g_res.ps_effect_pip = nullptr; return "CreatePixelShader(effect, PIP) " + hr_str(hr); }
    return "";
}

// The PIP's depth, copied into a texture of ours the effect reads at t1. A copy rather than an SRV on the engine's
// texture: the PIP depth carries whatever bind flags the engine gave it, and the source stays bound as it was. The copy
// is the source's typeless kind (CopyResource takes one type group; a typed depth - D24_UNORM_S8_UINT, D32_FLOAT, ... -
// cannot carry an SRV, so it copies into its group's typeless format, PIP_DEPTH_TYPED), the SRV typed as
// ensure_depth_srv types the main one.
inline bool pip_fx_depth(ID3D11Device* dev, ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv) {
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return false;
    ID3D11Texture2D* tex = nullptr;
    if (FAILED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) || !tex) {
        res->Release();
        return false;
    }
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    bool ok = td.SampleDesc.Count == 1 && td.ArraySize == 1;
    DXGI_FORMAT sf = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT cf = DXGI_FORMAT_UNKNOWN;   // PIP_DEPTH_TYPED: the copy's format.
    switch (td.Format) {
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            sf = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; cf = DXGI_FORMAT_R24G8_TYPELESS; break;
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
            sf = DXGI_FORMAT_R32_FLOAT; cf = DXGI_FORMAT_R32_TYPELESS; break;
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            sf = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; cf = DXGI_FORMAT_R32G8X24_TYPELESS; break;
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_D16_UNORM:
            sf = DXGI_FORMAT_R16_UNORM; cf = DXGI_FORMAT_R16_TYPELESS; break;
        default:
            ok = false;
            // EA_ERR_CLASS: an engine PIP configuration we do not serve - a warning (logged, never thrown).
            log_once_safe("KH RenderIntegration: pip fx: the PIP depth's format (" +
                          std::to_string(static_cast<int>(td.Format)) +
                          ") has no read; no PIP post passes");
            break;
    }
    if (ok && (!g_res.pip_depth_tex || !g_res.pip_depth_srv ||
               g_res.pip_depth_w != td.Width || g_res.pip_depth_h != td.Height ||
               g_res.pip_depth_fmt != td.Format)) {
        KH_SAFE_RELEASE(g_res.pip_depth_srv);
        KH_SAFE_RELEASE(g_res.pip_depth_tex);
        g_res.pip_depth_w = g_res.pip_depth_h = 0;
        g_res.pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
        D3D11_TEXTURE2D_DESC cd = {};
        cd.Width = td.Width;
        cd.Height = td.Height;
        cd.MipLevels = 1;
        cd.ArraySize = 1;
        cd.Format = cf;
        cd.SampleDesc.Count = 1;
        cd.Usage = D3D11_USAGE_DEFAULT;
        cd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = sf;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
        if (FAILED(dev->CreateTexture2D(&cd, nullptr, &g_res.pip_depth_tex)) || !g_res.pip_depth_tex ||
            FAILED(dev->CreateShaderResourceView(g_res.pip_depth_tex, &sd, &g_res.pip_depth_srv)) || !g_res.pip_depth_srv) {
            KH_SAFE_RELEASE(g_res.pip_depth_srv);
            KH_SAFE_RELEASE(g_res.pip_depth_tex);
            report_error_once_safe("KH RenderIntegration: pip fx: copy of the PIP depth could not be created (format " +
                                   std::to_string(static_cast<int>(td.Format)) + "); no PIP post passes");
            ok = false;
        } else {
            g_res.pip_depth_w = td.Width;
            g_res.pip_depth_h = td.Height;
            g_res.pip_depth_fmt = td.Format;
        }
    }
    if (ok) {
        // PRED_ASIDE: the copy runs ahead of pip_run_fx's StateBackup, inside the hook that called it - a predicate
        // the engine left bound would skip it and the passes would read the last PIP cycle's depth.
        PredAside pa;
        pa.begin(ctx);
        ctx->CopyResource(g_res.pip_depth_tex, tex);
        pa.end();
    }
    tex->Release();
    res->Release();
    return ok;
}

// UI_BB_UNWIND: releases the COM pointer it watches if - and only if - its scope is left by an exception;
// every normal exit releases the pointer itself. It watches the variable, not its value at the guard.
// Holders: flush_ui_locked's backbuffer and its bound views (RTV, DSV), and pip_run_fx's PIP colour resource (declared
// here, ahead of both).
template <typename T>
struct UnwindRelease {
    T*& p;
    const int ex;
    explicit UnwindRelease(T*& v) : p(v), ex(std::uncaught_exceptions()) {}
    ~UnwindRelease() {
        if (std::uncaught_exceptions() > ex && p) {
            p->Release();
            p = nullptr;
        }
    }
    UnwindRelease(const UnwindRelease&) = delete;
    UnwindRelease& operator=(const UnwindRelease&) = delete;
};

// The passes, into the PIP's colour target while bound. Called from hooked_copyresource (before the engine's copy) and,
// for a PIP that is never copied, from pip_track_targets at the leave. One attempt per PIP cycle (fx_done set first:
// our own copies re-enter the copy hook). Gated on the pass itself (the opaque count), not the injection: a scene of
// fullscreen passes and no meshes never injects.
inline void pip_run_fx(ID3D11DeviceContext* ctx) {
    g_pip.fx_done = true;
    if (g_pip.opaques < PIP_MIN_OPAQUES || !g_pip.vp_valid) return;   // A real pass, its viewport known.
    if (!g_pip.tpl_valid || !g_pip.view_valid || !g_pip.proj_valid) return;
    if (g_topo_cycles < g_pip.tpl_cycle || g_topo_cycles - g_pip.tpl_cycle > PIP_TPL_MAX_CYCLES) return;
    if (!g_res.vs_fullscreen || !g_res.dss_off || !g_res.rasterizer ||
        !g_res.composite_cb || !g_res.composite_frame_cb) return;

    // The flush's fullscreen set narrowed to the localized ones (a global pass already reaches the mirror through the
    // main view), in creation order, the envelope folded into the alpha.
    static std::vector<std::pair<uint64_t, RenderObject>> list;
    list.clear();
    const float now = effect_time_seconds();
    const double now_d = effect_time_seconds_d();
    ID3D11Device* pf_dev = RVExtBridge::get_d3d_device();   // FX_USER: a user pass's PIP build, asked here.
    scene_sync();
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!o.visible || !o.fullscreen || !o.localized || o.ui_only || o.effect <= 0) continue;
        if (o.effect == static_cast<int>(EffectId::Ssgi)) continue;
        if (o.effect == static_cast<int>(EffectId::DynLightFog)) continue;   // DLF: the main view's lights.
        // FX_USER: a user pass joins once its single-sample build is ready (asking queues it).
        if (o.effect == EFFECT_CUSTOM &&
            (!pf_dev || !user_fx_ps(pf_dev, fx_shader_of(o), nullptr, 1u))) continue;
        bool exp = false;
        const float env = lifetime_envelope(o, now, exp);
        if (exp) continue;
        RenderObject pf_o = o;
        pf_o.color[3] *= env;
        if (pf_o.color[3] <= 0.001f) continue;
        if (pf_o.effect == EFFECT_LUT && !user_lut_cached(fx_shader_of(pf_o))) continue;   // Not loaded by the flush yet.
        list.emplace_back(pf_o.seq, pf_o);
    }
    if (list.empty()) return;
    std::sort(list.begin(), list.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    // PIP_ORIGIN: TopLeftX / TopLeftY must be 0 (held by every tested PIP): PSEffect takes SV_Position.xy as the
    // frame's pixel (clamping int2(SV_Position.xy) to [0, fxMeta.zw)) and its uv as SV_Position.xy / fxMeta.zw, so an
    // offset viewport's passes would read the scene, the depth and their masks at the wrong place. Such a PIP keeps the
    // engine's picture: refused here, with a pass to draw, ahead of the copies of its colour and depth (only these
    // passes read them) and of any state change, and reported once. Carrying the origin in a lane and subtracting it in
    // the shader would serve it.
    if (g_pip.vp.TopLeftX != 0.0f || g_pip.vp.TopLeftY != 0.0f) {
        // EA_ERR_CLASS: an engine PIP configuration we do not serve - a warning (logged, never thrown).
        log_once_safe("KH RenderIntegration: pip fx: a picture-in-picture viewport with an offset origin; no PIP "
                      "post passes");
        return;
    }

    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    if (!ensure_ok("pip effect shader", pip_fx_shader(dev)) || !g_res.ps_effect_pip) return;   // EA_PIP_ONCE.

    // Declared before the OM save so the unwind restores the OM before the SRVs (destructors run in reverse declaration
    // order); captured later, ahead of our draws.
    StateBackup bk;
    OmSave om;
    om.capture(ctx);
    ID3D11Resource* rt = nullptr;
    if (om.rtv[0]) om.rtv[0]->GetResource(&rt);
    // PIP_FX: a throw below (a pyramid build, a texture fill, an upload) reaches the hook's catch, which cannot
    // see this reference on the engine's PIP target - released on that unwind (UI_BB_UNWIND, the backbuffer's
    // guard); every return below releases it itself.
    UnwindRelease<ID3D11Resource> ub(rt);
    bool ok = rt != nullptr && om.dsv != nullptr;
    if (ok) {   // The PIP's colour target: our copy of it is what every pass reads.
        ID3D11Texture2D* tex = nullptr;
        if (FAILED(rt->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) || !tex) {
            ok = false;
        } else {
            D3D11_TEXTURE2D_DESC td = {};
            tex->GetDesc(&td);
            D3D11_RENDER_TARGET_VIEW_DESC rd = {};
            om.rtv[0]->GetDesc(&rd);
            ok = td.SampleDesc.Count == 1 && td.ArraySize == 1;
            if (ok && (!g_res.pip_scene_tex || !g_res.pip_scene_srv ||
                       !g_res.pip_scene_tex2 || !g_res.pip_scene_srv2 ||
                       !g_res.pip_scene_rtv[0] || !g_res.pip_scene_rtv[1] ||
                       g_res.pip_scene_w != td.Width || g_res.pip_scene_h != td.Height ||
                       g_res.pip_scene_fmt != rd.Format)) {
                KH_SAFE_RELEASE(g_res.pip_scene_srv);
                KH_SAFE_RELEASE(g_res.pip_scene_tex);
                KH_SAFE_RELEASE(g_res.pip_scene_srv2);
                KH_SAFE_RELEASE(g_res.pip_scene_tex2);
                KH_SAFE_RELEASE(g_res.pip_scene_rtv[0]);
                KH_SAFE_RELEASE(g_res.pip_scene_rtv[1]);
                g_res.pip_scene_w = g_res.pip_scene_h = 0;
                g_res.pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
                D3D11_TEXTURE2D_DESC cd = {};
                cd.Width = td.Width;
                cd.Height = td.Height;
                cd.MipLevels = 1;
                cd.ArraySize = 1;
                cd.Format = rd.Format;   // The view's typed format (ensure_scene_capture_from's rule).
                cd.SampleDesc.Count = 1;
                cd.Usage = D3D11_USAGE_DEFAULT;
                // Both buffers are read by one pass and written by the next.
                cd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
                if (FAILED(dev->CreateTexture2D(&cd, nullptr, &g_res.pip_scene_tex)) || !g_res.pip_scene_tex ||
                    FAILED(dev->CreateShaderResourceView(g_res.pip_scene_tex, nullptr, &g_res.pip_scene_srv)) || !g_res.pip_scene_srv ||
                    FAILED(dev->CreateRenderTargetView(g_res.pip_scene_tex, nullptr, &g_res.pip_scene_rtv[0])) || !g_res.pip_scene_rtv[0] ||
                    FAILED(dev->CreateTexture2D(&cd, nullptr, &g_res.pip_scene_tex2)) || !g_res.pip_scene_tex2 ||
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
                    ok = false;
                } else {
                    g_res.pip_scene_w = td.Width;
                    g_res.pip_scene_h = td.Height;
                    g_res.pip_scene_fmt = rd.Format;
                }
            }
            tex->Release();
        }
    }
    if (ok) ok = pip_fx_depth(dev, ctx, om.dsv);
    if (!ok) {
        if (rt) rt->Release();
        om.release();
        return;
    }

    // The PIP camera as pip_inject builds it: the rotation rows on camera-relative positions. The inverse is of
    // exactly that (small numbers, exact in fp32) and fxCam carries the PIP camera the shader adds back.
    float v[4][4] = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            v[r][c] = g_pip.view_rows[c * 4 + r];
    v[3][3] = 1.0f;
    float p[4][4];
    memcpy(p, g_pip.proj, sizeof(p));

    ConstantData cbf = g_pip.tpl;
    mul_4x4(v, p, cbf.view_proj);
    fill_user_cam_cb(cbf, g_pip.cam, v);   // USER_LANES: the PIP camera.
    if (!inverse_4x4(cbf.view_proj, cbf.inv_view_proj)) {
        rt->Release();
        om.release();
        return;
    }
    cbf.fx_cam[0] = g_pip.cam[0];
    cbf.fx_cam[1] = g_pip.cam[1];
    cbf.fx_cam[2] = g_pip.cam[2];
    cbf.fx_cam[3] = 1.0f;
    {   // LOCAL_CULL: the passes whose volume the PIP's view cannot reach (its own pair: p's, below).
        LocalCull lc;
        if (local_cull_make(lc, cbf.view_proj, cbf.fx_cam, false, 0.0f, 0.0f)) {
            list.erase(std::remove_if(list.begin(), list.end(),
                                      [&lc](const std::pair<uint64_t, RenderObject>& e) {
                                               return local_culled(lc, e.second);
                                      }),
                       list.end());
        }
        if (list.empty()) {   // Nothing reaches the view: the PIP stays as the engine drew it.
            rt->Release();
            om.release();
            return;
        }
    }
    cbf.fog_color[3] = g_pip.cam[1];                        // KhFsFog's camera altitude: the PIP's, not the
                                                                 // template's main camera (the flush's cam_y twin).
    cbf.snap_cam[3] = 0.0f;                                 // The same disarms as the mesh injection.
    memset(cbf.mask_meta, 0, sizeof(cbf.mask_meta));
    cbf.lighting2[3] = 0.0f;
    memset(cbf.sten_vol2, 0, sizeof(cbf.sten_vol2));
    memset(cbf.mir_meta, 0, sizeof(cbf.mir_meta));

    // The PIP's viewport: the screen size every pass's uv assumes and the depth range its depth was encoded through -
    // the pass's own, captured during it; its origin is 0 (PIP_ORIGIN, checked ahead of the copies).
    const D3D11_VIEWPORT vp = g_pip.vp;
    if (vp.Width < 2.0f || vp.Height < 2.0f) {
        rt->Release();
        om.release();
        return;
    }
    UINT nvp_cur = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT vp_cur[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&nvp_cur, vp_cur);   // Restored after our draws (StateBackup restores them too).

    const bool pinj = g_ro.in_injection;
    g_ro.in_injection = true;   // Our own draws: the hooks stand aside.
    bk.capture(ctx);
    uint32_t drawn = 0;
    if (upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_effect_pip, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
        ctx->VSSetConstantBuffers(0, 2, cbs);
        ctx->PSSetConstantBuffers(0, 2, cbs);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        const FLOAT bf[4] = { 0, 0, 0, 0 };
        // Compositing is in-shader (the flush's rule); RGB only - the PIP's
        // alpha is the engine's, read by its post pass.
        ctx->OMSetBlendState(g_res.blend_ui_replace ? g_res.blend_ui_replace : nullptr, bf, 0xFFFFFFFF);
        ctx->RSSetState(g_res.rasterizer);
        ctx->PSSetShaderResources(1, 1, &g_res.pip_depth_srv);
        ctx->RSSetViewports(1, &vp);   // PIP_FX_LEAVE_VP.
        bool lut_bound = false;
        // One copy of the PIP per cycle; the chain ping-pongs between the two buffers and its last pass writes the PIP
        // target itself. A pass that fails to upload ends the chain with the PIP untouched (the buffers hold the
        // partial result nobody reads).
        ctx->CopyResource(g_res.pip_scene_tex, rt);
        ID3D11ShaderResourceView* src = g_res.pip_scene_srv;
        int wr = 1;   // The next intermediate write buffer: [1] first, then alternating.
        for (size_t fi = 0; fi < list.size(); ++fi) {
            const RenderObject& o = list[fi].second;
            const bool last = fi + 1 == list.size();
            ID3D11ShaderResourceView* lut = nullptr;
            if (o.effect == EFFECT_LUT) {
                lut = user_lut_cached(fx_shader_of(o));
                if (!lut) break;   // Resolved at the gather; a release since ends the chain.
            }
            // FX_USER: a user pass's single-sample build (the PIP depth is a copy), ready at the gather.
            ID3D11PixelShader* ufx = nullptr;
            bool upyr = false;
            if (o.effect == EFFECT_CUSTOM) {
                ufx = user_fx_ps(dev, fx_shader_of(o), &upyr, 1u);
                if (!ufx) break;   // As a LUT released since.
            }
            ConstantData cbd = cbf;
            cbd.center_size[3] = 1.0f;   // A chain pass: the packing tail composites and writes opaque.
            cbd.size_axes[3] = static_cast<float>(o.blend_mode);   // The tail's blend mode (upload_cb's twin);
                                                                   // the template's zero read every pass as normal.
            fill_fx_params_cb(cbd, o);
            if (o.effect == static_cast<int>(EffectId::Fogscatter)) fogscatter_pack(cbd, list);
            cbd.fx_meta[0] = static_cast<float>(o.effect);
            cbd.fx_meta[1] = fx_time(o, now_d);
            cbd.fx_meta[2] = vp.Width;
            cbd.fx_meta[3] = vp.Height;
            cbd.depth_params[0] = p[2][2];   // The harvested projection: the pair the PIP depth was encoded with.
            cbd.depth_params[1] = p[3][2];
            cbd.depth_params[2] = vp.MinDepth;
            cbd.depth_params[3] = vp.MaxDepth;
            fill_local_band_cb(cbd, o);
            fill_lighting_obj_cb(ctx, cbd, o);
            // GLOW_PYR: the scene chain's rule, on this pass's own source.
            bool gl = false;
            if (glow_effect(o.effect) || upyr) {   // FX_USER: or a user pass's.
                gl = glow_build(dev, ctx, g_res.glow_pyr[2], src, cbd.fx_meta[2], cbd.fx_meta[3],
                                cbd.center_size[3], o.effect, cbd.fx0[0], g_res.composite_cb);
                glow_lanes(cbd, gl ? &g_res.glow_pyr[2] : nullptr);
            }
            // ANA_PYR: anamorphic's one-axis pyramid, the same rule.
            const AnaPyr* an = nullptr;
            if (o.effect == static_cast<int>(EffectId::Anamorphic)) {
                const int ax = cbd.fx1[0] > 0.5f ? 1 : 0;
                if (ana_build(dev, ctx, g_res.ana_pyr[2][ax], src, cbd.fx_meta[2], cbd.fx_meta[3],
                              cbd.center_size[3], ax, cbd.fx0[0], g_res.composite_cb))
                    an = &g_res.ana_pyr[2][ax];
                ana_lanes(cbd, an);
            }
            ID3D11ShaderResourceView* tex[MAT_USER_MAPS] = {};   // FX_TEX.
            const bool tx = fx_tex_on(o);   // FX_TEX_ALL: a user pass, or a builtin one with a set.
            if (tx) fx_tex_fill(dev, ctx, cbd, o, tex);
            if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            if (lut) { ctx->PSSetShaderResources(19, 1, &lut); lut_bound = true; }
            // Unbind the source before its sibling becomes the target (the scene chain's hazard pattern), then the last
            // pass takes the PIP's own targets back.
            ID3D11ShaderResourceView* null = nullptr;
            ctx->PSSetShaderResources(0, 1, &null);
            if (last) om.restore(ctx);
            else           ctx->OMSetRenderTargets(1, &g_res.pip_scene_rtv[wr], nullptr);
            ctx->PSSetShaderResources(0, 1, &src);
            GlowBind gb;   // GLOW_PYR.
            if (gl) gb.bind(ctx, g_res.glow_pyr[2]);
            AnaBind ab;   // ANA_PYR.
            if (an) ab.bind(ctx, *an);
            ctx->PSSetShader(ufx ? ufx : g_res.ps_effect_pip, nullptr, 0);   // FX_USER: per pass.
            if (ufx) ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // FX_USER: s1 for every user effect draw.
            if (tx) fx_tex_bind(ctx, tex);   // FX_TEX.
            ctx->Draw(3, 0);
            if (tx) fx_tex_unbind(ctx);
            ab.unbind();
            gb.unbind();
            ++drawn;
            if (!last) {
                src = wr == 1 ? g_res.pip_scene_srv2 : g_res.pip_scene_srv;
                wr ^= 1;
            }
        }
        if (lut_bound) { ID3D11ShaderResourceView* n19 = nullptr; ctx->PSSetShaderResources(19, 1, &n19); }
        {   // Never leave a chain buffer bound as t0 or as the target.
            ID3D11ShaderResourceView* n0 = nullptr;
            ctx->PSSetShaderResources(0, 1, &n0);
        }
    }
    om.restore(ctx);   // The intermediates were bound mid-chain; the PIP's targets come back.
    bk.restore(ctx);
    if (nvp_cur > 0) ctx->RSSetViewports(nvp_cur, vp_cur);   // PIP_FX_LEAVE_VP.
    g_ro.in_injection = pinj;
    rt->Release();
    om.release();
    if (drawn) stat_add(g_stats.pip_fx, drawn);
}

// At every OM set on the render thread, after g_ro.dsv_main is known.
inline void pip_track_targets(ID3D11DeviceContext* ctx, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv) {
    // The binding that leaves a PIP pass (off, or another PIP's RT0) draws the passes into it first - the PIP's targets
    // are still bound, this hook runs ahead of the engine's call.
    const bool was = g_pip.on;
    void* const was_rtv = g_pip.cur_rtv;
    if (!dsv || g_ro.dsv_main || !g_main_depth_identity || n < 1 || !rtvs || !rtvs[0]) {
        if (was && !g_pip.fx_done) pip_run_fx(ctx);
        g_pip.on = false;
        g_pip.cur_rtv = nullptr;
        return;
    }
    void* id = reorder_dsv_identity(dsv);
    void* rtv = static_cast<void*>(rtvs[0]);
    if (was && rtv != was_rtv && !g_pip.fx_done) pip_run_fx(ctx);
    for (uint32_t i = 0; i < g_pip.cache_n; ++i) {
        const PipState::Key& k = g_pip.cache[i];
        if (k.dsv == id && k.rtv == rtv && k.fmt == g_pip.scene_fmt) {   // PIP_FMT.
            g_pip.on = k.pip;
            g_pip.cur_rtv = k.pip ? rtv : nullptr;
            if (k.pip) { g_pip.w = k.w; g_pip.h = k.h; }
            return;
        }
    }
    bool pip = false;
    uint32_t w = 0, h = 0;
    ID3D11Resource* res = nullptr;
    rtvs[0]->GetResource(&res);
    if (res) {
        if (static_cast<void*>(res) != g_topo_scene_tex_id) {
            ID3D11Texture2D* tex = nullptr;
            if (SUCCEEDED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) && tex) {
                D3D11_TEXTURE2D_DESC td = {};
                tex->GetDesc(&td);
                // PIP_FMT: the HDR target kind - RGBA16F or the main scene's own format.
                const bool hdr = td.Format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
                                 (g_pip.scene_fmt != 0u && static_cast<uint32_t>(td.Format) == g_pip.scene_fmt);
                pip = hdr && td.SampleDesc.Count == 1 && td.Width >= 64 && td.Height >= 64;
                w = td.Width; h = td.Height;
                tex->Release();
            }
        }
        res->Release();
    }
    PipState::Key k = { id, rtv, pip, w, h, g_pip.scene_fmt };
    if (g_pip.cache_n < 8) g_pip.cache[g_pip.cache_n++] = k;
    else g_pip.cache[g_topo_cycles & 7] = k;   // A cold slot; the cache is a hint.
    g_pip.on = pip;
    g_pip.cur_rtv = pip ? rtv : nullptr;
    if (pip) { g_pip.w = w; g_pip.h = h; g_pip_seen = true; }
}

// A depth clear on a non-main target opens a PIP cycle (whether the target is a
// PIP one is decided at the OM hook, which precedes the draws).
inline void pip_on_clear() {
    g_pip.opaques = 0;
    g_pip.injected = false;
    g_pip.fx_done = false;   // PIP_FX.
    g_pip.vp_valid = false;  // PIP_FX_LEAVE_VP.
}

inline void pip_inject(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_PIP);
    KH_GPU_SCOPE(ctx, GPU_PIP);   // PROF.
    g_pip.injected = true;   // One attempt per cycle, whatever the gates say.
    if (!g_pip.tpl_valid || !g_pip.view_valid || !g_pip.proj_valid) return;
    if (g_topo_cycles < g_pip.tpl_cycle || g_topo_cycles - g_pip.tpl_cycle > PIP_TPL_MAX_CYCLES) return;
    if (!g_res.vs || !g_res.ps || !g_res.composite_cb || !g_res.composite_frame_cb ||
        !g_res.input_layout || !g_res.dss_test_write || !g_res.rasterizer || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    skin_cull_catchup(ctx);   // SKIN_CULL: no step precedes the PIP; its own bracket, ahead of this pass's.
    g_skin_cull_pip_cycle = g_topo_cycles;   // SKIN_CULL: while PIPs draw, the steps do not cull.

    const bool pinj = g_ro.in_injection;
    g_ro.in_injection = true;   // Our own draws: the hooks stand aside.
    StateBackup bk;
    bk.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSlot0 s0;   // MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    s0.open(ctx);

    // Camera-relative view (camera at the origin; center_rel carries the offset per object) times the harvested
    // projection: the rebased contract the two mesh passes draw under.
    float pi_v[4][4] = {};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            pi_v[r][c] = g_pip.view_rows[c * 4 + r];   // Row-vector: the transpose of the engine's rows.
    pi_v[3][3] = 1.0f;
    float p[4][4];
    memcpy(p, g_pip.proj, sizeof(p));
    const float* cam = g_pip.cam;
    // PIP_LOD: the pass's own pixels-per-tangent for the level pick - the PIP's projection and the height of the
    // viewport this pass draws our meshes with (LOD_ONE_SCALE's definition, EA_PIP_LOD_VP; the target's height
    // where none is bound), the main passes' formula (not g_lod.proj_px: the main view's, which a zoomed or smaller
    // PIP does not share - its meshes would draw at the main view's levels for the PIP camera's distance). An unknown
    // height reads as level 0, as an unpublished g_lod.proj_px does.
    UINT nvp = 1;
    D3D11_VIEWPORT dvp = {};
    ctx->RSGetViewports(&nvp, &dvp);
    const float lod_h = (nvp >= 1 && dvp.Height >= 1.0f) ? dvp.Height
                                                           : static_cast<float>(g_pip.h);
    const float lod_px = (lod_h > 0.0f && p[1][1] > 0.0f)
                       ? p[1][1] * lod_h * 0.5f : 0.0f;
    const float lod_zoom = lod_zoom_of(p[1][1]);   // LOD_ZOOM: the PIP camera's own.

    ConstantData cbf = g_pip.tpl;
    mul_4x4(pi_v, p, cbf.view_proj);
    fill_user_cam_cb(cbf, cam, pi_v);   // USER_LANES: the PIP camera.
    cbf.snap_cam[3] = 0.0f;                              // No scene snapshot here.
    memset(cbf.mask_meta, 0, sizeof(cbf.mask_meta));   // No band table, no mask stream.
    // With the band table off PSMain's received term falls to ShadowMapFactor - the engine atlas at t1 scaled by
    // lighting2.w. The atlas is not bound here (it is the main view's) and an unbound t1 reads as fully occluded, so
    // the strength is zeroed: the engine's own shadows on our meshes are not received in a PIP pass; ours (the sun
    // ladder, the DLS maps) are.
    cbf.lighting2[3] = 0.0f;
    memset(cbf.sten_vol2, 0, sizeof(cbf.sten_vol2));   // No volume transport; the plain vertex path.
    // No mirror: its lanes (xyz) go. PIP_SUN_FADE: mirMeta.w is the sun's range-fade radius, a world-space lane
    // about sunOrigin - the main camera the ladder's windows are fitted round, which the template carries - so it
    // stays: our meshes' sun shadows fade in a PIP where they fade in the main view (as the light maps' fade, dlsRange,
    // does), instead of ending at the far tier's square edge.
    cbf.mir_meta[0] = 0.0f;
    cbf.mir_meta[1] = 0.0f;
    cbf.mir_meta[2] = 0.0f;
    cbf.fx_meta[2] = static_cast<float>(g_pip.w);
    cbf.fx_meta[3] = static_cast<float>(g_pip.h);
    // Bucket pass lanes (the injection's twin): the rebase camera the instanced VS subtracts, armed exactly as
    // center_rel is per object below. The template carried the main pass's camera and arm, never this one's - left as
    // it was, every bucket instance would land a camera vector away.
    cbf.pass_cam[0] = cam[0]; cbf.pass_cam[1] = cam[1]; cbf.pass_cam[2] = cam[2];
    // The fog's camera altitude: the PIP's, not the template's main camera (pip_run_fx's twin). PSMain's fog measures
    // its height terms, its below-layer split and its elevation from it, and fxParams0 below is the PIP camera.
    cbf.fog_color[3] = cam[1];
    cbf.pass_cam[3] = 1.0f;
    scene_sync();   // PF_NEED: the scene this pass draws, synced ahead of its convert and binds.
    // PF_STAGES: the engine's GS / HS / DS are live in this hook and the convert sets none of them (a bound one
    // drops its draws, and the pyramid commits as valid); nulled first, as the injection and the flush do.
    // StateBackup restores them.
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    sun_pf_convert(ctx, &cbf);   // PF_NEED: a pyramid this pass's receivers read converts here.
    fill_sun_pf_lanes(cbf);   // PF_NEED: the arms say what the binds below bind.
    if (!upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
        s0.close(ctx);   // MESH_O1: under the hooks' exclusion.
        bk.restore(ctx);
        g_ro.in_injection = pinj;
        return;
    }

    const UINT stride = sizeof(MeshVertex);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(g_res.ps, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    // World-space products, as the injection binds them (no depth witness at t0, no bands: mask_meta says so).
    if (g_res.dls_srv) ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    if (g_sun_maps.sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    if (g_sun_maps.sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
    if (g_sun_maps.sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
    if (g_sun_maps.sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
    if (g_sun_maps.sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
    if (g_sun_maps.sun2_map_valid && g_sun_maps.sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29,
        1, &g_res.sun_pf_srv[0]);
    if (g_sun_maps.sun3_map_valid && g_sun_maps.sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30,
        1, &g_res.sun_pf_srv[1]);
    if (g_sun_maps.sun4_map_valid && g_sun_maps.sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31,
        1, &g_res.sun_pf_srv[2]);
    if (g_sun_maps.sun5_map_valid && g_sun_maps.sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20,
        1, &g_res.sun_pf_srv[3]);
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // SHADOW_GATHER: null = the default state, also clamp.
    if (g_thm.valid && g_res.thm_srv) ctx->PSSetShaderResources(10, 1, &g_res.thm_srv);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* bound_rs = g_res.rasterizer;
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    int bound_bm = -1;
    VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
    ID3D11VertexShader* bound_vs = g_res.vs;
    ID3D11InputLayout*  bound_il = g_res.input_layout;
    ID3D11ShaderResourceView* bound_sk = nullptr;   // SKIN_POOL: VS t52 (inst_sk_bind).
    ID3D11PixelShader*  bound_ps = g_res.ps;
    const float now = effect_time_seconds();
    uint32_t drawn = 0;

    objbuf_sync(ctx);   // OBJBUF: the records this pass's buckets index (scene synced above).
    objbuf_bind(ctx);   // VS t39 (StateBackup restores it).

    // The injection's structure on the PIP camera: the grid pre-cull against this pass's frustum (an invalid cull set
    // returns every cell), the eligible copies with the envelope folded, front to back, then buckets through the static
    // instanced twins (this pass draws the static pair: route 0 - no guard, arb or near-gap; a mesh beyond the PIP's
    // far draws clamped at its MaxDepth, unordered) and the per-object loop at each bucket's representative. Opaque
    // only, by design: the set is the injection's opaque one, and a blend material draws its solid part alone (part 1)
    // - this pass has no translucent tail, so glass texels are not drawn in a PIP.
    static std::vector<RenderObject> meshes;
    static std::vector<uint32_t> cand;
    meshes.clear();
    {
        CullSet cull;
        cull_build(cbf.view_proj, cam, cull);   // Rebased: the apex is the origin.
        scene_grid_query(cull, cand);
    }
    for (uint32_t s : cand) {
        if (!g_scene.alive[s]) continue;
        const RenderObject& o = g_scene.objs[s];
        if (!o.visible || !is_composite_eligible(o)) continue;
        bool expired = false;
        const float env = lifetime_envelope(o, now, expired);
        if (expired) continue;
        if (!(o.blend_mode == 0 && o.color[3] * env >= 0.999f)) continue;   // The injection's own set.
        const int mid = mesh_id_clamp(o.mesh);
        if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size()) continue;
        if (!mesh_vb_ref(mid, o.slot)) continue;   // CLOTH: no buffer, nothing to gather.
        meshes.push_back(o);
        meshes.back().color[3] *= env;
    }
    mesh_order(meshes, cam, g_dist_memo_inj);   // MESH_ORDER: opaque front to back for early-Z,
                                                        // the rest back to front; the memo begins here, seeded.
    g_vis_memo_inj.begin(meshes);

    // Bounding sphere against the PIP camera's frustum, in view space (rel = engine-order position minus the camera,
    // through the view rows), with the harvested projection's x / y scales as the side planes and the near plane at z =
    // 0. The pass's one visibility truth: the material ensure, the plan's emit and the loop all ask it, memoised per
    // object.
    const float m00 = p[0][0], m11 = p[1][1];
    const float nx = sqrtf(m00 * m00 + 1.0f);
    const float ny = sqrtf(m11 * m11 + 1.0f);
    auto vis_calc = [&](const RenderObject& pv_o) -> bool {
        const float rel[3] = { pv_o.pos[0] - cam[0], pv_o.pos[2] - cam[1], pv_o.pos[1] - cam[2] };
        float bs[3];
        bounds_size_of(pv_o, bs);   // CAST_REFIT.
        const float pc_r = lod_radius_of(bs, pv_o.rot_m, pv_o.rotated);
        float pc_v[3];
        for (int c = 0; c < 3; ++c) {
            const float* row = g_pip.view_rows + c * 4;
            pc_v[c] = row[0] * rel[0] + row[1] * rel[1] + row[2] * rel[2];
        }
        if (pc_v[2] + pc_r < 0.0f) return false;                                      // Behind the camera.
        if ((pc_v[2] - m00 * pc_v[0]) / nx < -pc_r) return false;        // Right of the frustum.
        if ((pc_v[2] + m00 * pc_v[0]) / nx < -pc_r) return false;        // Left.
        if ((pc_v[2] - m11 * pc_v[1]) / ny < -pc_r) return false;        // Above.
        if ((pc_v[2] + m11 * pc_v[1]) / ny < -pc_r) return false;        // Below.
        return true;
    };
    auto vis = [&](const RenderObject& pv_o) -> bool {
        return g_vis_memo_inj.get(pv_o, vis_calc);
    };

    // Buckets, PIP edition: the static instanced twins (all-or-nothing, the flush's gate), the plan keyed with route 0
    // and the light identity, the emission once against this frustum. Twin at the injection.
    const bool inst_on = inst_twins_on();
    static InstPlan plan;
    bool inst_vb_bound = false;
    if (inst_on) {
        mat_ensure_all(ctx, dev, meshes, vis);   // MAT_TABLE: bases fixed before any lane is written.
        inst_plan_build(plan, meshes, nullptr, meshes.size(),
                        [&](const RenderObject& pp_o, InstKey& k) {
                            k.route = 0u;
                            k.aux = dl_bucket_hash(pp_o);
                        });
        inst_plan_emit(ctx, dev, plan, meshes, cam, 1, vis, g_dist_memo_inj, lod_px,
                       lod_zoom);   // LOD_ZOOM.
    }

    for (size_t oi = 0; oi < meshes.size(); ++oi) {
        const RenderObject& o = meshes[oi];
        // Carried by its bucket's draw at the representative, or culled by the
        // emission's walk (only the representative stays undone).
        if (inst_on && plan.done[oi]) continue;
        if (!vis(o)) continue;   // The emit's twin: an emitted representative always draws.
        const int mid = mesh_id_clamp(o.mesh);

        ConstantData cbd = cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float res[3];   // POS_RES: the object's live residual.
        obj_pos_res(o, res);
        fill_center_rel(cbd, cam, res);    // Arms center_rel.w: the rebased contract.
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        fill_obj_rot(cbd, o.rot_m);
        cbd.blend_ctl[0] = 0.0f;
        cbd.blend_ctl[3] = 0.0f;
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        cbd.fx0[0] = cam[0]; cbd.fx0[1] = cam[1]; cbd.fx0[2] = cam[2];
        // PSMain's punch-through guard arms on fxParams1.x below 1e8 and reads the depth witness at t0 - the engine's
        // own binding here, never ours. Disarm it by value, as the injection does.
        cbd.fx1[0] = 1e9f;
        cbd.fx1[1] = 0.0f;
        fill_local_band_cb(cbd, o);
        fill_lighting_obj_cb(ctx, cbd, o);
        const MaterialSet* txm = obj_textured(o);
        const bool tx = txm && g_res.layout_tex && g_res.vs_tex && g_res.ps_tex && g_res.mat_sampler;
        // TX_ONE_UPLOAD: textured, the slice goes up per submesh in draw_textured. Twin: the injection.
        if (!tx && !upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;

        if (o.blend_mode != bound_bm) {
            ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
            bound_bm = o.blend_mode;
        }
        const MeshDef& md = mesh_def(mid);
        // The colour pass's own pick with its crossfade (the injection's twin;
        // a locked instance never picks), at the PIP's own scale (PIP_LOD).
        int lod = 0;
        float lodt = 0.0f;
        if (!lod_locked(o)) {   // LOD_SIM_SLOT.
            lod_pick(md, lod_radius(o), sqrtf(g_dist_memo_inj.get(o, cam)), lod, lodt, lod_px,
                     lod_zoom);   // LOD_ZOOM.
        }
        const bool lodx = lodt > 0.0f;
        {
            ID3D11PixelShader* want_ps = tx ? g_res.ps_tex : g_res.ps;
            if (want_ps != bound_ps) { ctx->PSSetShader(want_ps, nullptr, 0); bound_ps = want_ps; }
            ID3D11VertexShader* want_vs = tx ? g_res.vs_tex : g_res.vs;
            if (want_vs != bound_vs) { ctx->VSSetShader(want_vs, nullptr, 0); bound_vs = want_vs; }
            ID3D11InputLayout* want_il = tx ? g_res.layout_tex : g_res.input_layout;
            if (want_il != bound_il) { ctx->IASetInputLayout(want_il); bound_il = want_il; }
        }
        const VbRef vb = mesh_vb_ref(mid, o.slot);   // CLOTH.
        if (vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = vb;
        }

        // This object is its bucket's representative: swap in the static instanced VS + layout, bind the lane ring once
        // per pass, draw the emitted ranges (every member visible at emission, at its own level, crossfade included).
        // Every draw here is CullNone. Twin at the injection.
        int lod_iters = lodx ? 2 : 1;
        if (inst_on && plan.batch_of[oi] >= 0) {
            // SKIN_POOL: a skin pool chunk's bucket pulls (the injection's twin).
            const SkinPoolChunk* const ipc = inst_bucket_pool(plan, static_cast<uint32_t>(oi));
            ID3D11VertexShader* ivs = ipc ? (tx ? g_res.vs_inst_sk_tex : g_res.vs_inst_sk)
                                                    : (tx ? g_res.vs_inst_tex : g_res.vs_inst);
            ID3D11InputLayout*  iil = ipc ? g_res.layout_inst_sk
                                                    : (tx ? g_res.layout_inst_tex : g_res.layout_inst);
            if (ivs && iil) {
                if (ivs != bound_vs) { ctx->VSSetShader(ivs, nullptr, 0); bound_vs = ivs; }
                if (iil != bound_il) { ctx->IASetInputLayout(iil); bound_il = iil; }
                inst_sk_bind(ctx, ipc, bound_sk);
                if (!inst_vb_bound) {
                    const UINT is = INST_STRIDE, io = 0;
                    ctx->IASetVertexBuffers(1, 1, &g_res.inst_vb[1], &is, &io);
                    inst_vb_bound = true;
                }
                uint32_t in = 0;
                drawn += inst_draw(ctx, dev, plan, meshes, static_cast<uint32_t>(oi),
                                   cbd, g_res.composite_cb, tx ? txm : nullptr, false,
                                   bound_rs, 0, g_res.ps_tex, 1, in);
                if (ipc) stat_add(g_stats.skin_inst, in);   // SKIN_POOL.
                lod_iters = 0;   // The batch drew; no per-object level walk.
            } else {
                inst_unbatch(plan, static_cast<uint32_t>(oi));   // INST_UNBATCH.
            }
        }

        for (int li = 0; li < lod_iters; ++li) {
            const int lvl = lod + li;
            if (lodx) {
                cbd.blend_ctl[3] = lod_dither(lodt, li != 0);
                // TX_ONE_UPLOAD: textured, draw_textured uploads the dither with each submesh.
                if (!tx && !upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            }
            if (tx) {
                // The static twin (ctx 0), the opaque part (1): the injection's own call.
                drawn += draw_textured(ctx, dev, cbd, g_res.composite_cb, *txm, mid, false,
                                       bound_rs, 0, g_res.ps_tex, lvl, 1);
            } else {
                UINT ls = 0, lc = 0;
                mesh_lod_range(md, lvl, ls, lc);
                if (lc == 0) continue;
                ctx->DrawIndexed(lc, ls, 0);
                ++drawn;
            }
        }
        if (bound_rs != g_res.rasterizer) {
            ctx->RSSetState(g_res.rasterizer);
            bound_rs = g_res.rasterizer;
        }
    }
    if (drawn) {
        stat_inc(g_stats.pip_injections);
        stat_add(g_stats.pip_meshes, drawn);
    }
    s0.close(ctx);   // MESH_O1: under the hooks' exclusion.
    bk.restore(ctx);
    g_ro.in_injection = pinj;
}

// Every engine draw on a PIP pass (reorder_pre_draw, render thread).
inline void pip_pre_draw(ID3D11DeviceContext* ctx) {
    if (g_ro.blend_translucent) {
        if (!g_pip.injected && g_pip.opaques >= PIP_MIN_OPAQUES) pip_inject(ctx);
    } else if (!g_ro.dss_nowrite) {
        // The pass's viewport, read once at the draw that makes it a real pass;
        // pip_run_fx draws under it.
        if (++g_pip.opaques == PIP_MIN_OPAQUES) {
            UINT n = 1;
            D3D11_VIEWPORT vp = {};
            ctx->RSGetViewports(&n, &vp);
            g_pip.vp_valid = n >= 1 && vp.Width >= 2.0f && vp.Height >= 2.0f;
            if (g_pip.vp_valid) g_pip.vp = vp;
        }
    }
}
}