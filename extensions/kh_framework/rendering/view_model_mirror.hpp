#pragma once

// The injection half (rendering_integration_injection.hpp): the view-model mirror (KH_VMIR): its targets, the b2 patch
// and the bracket. Never include it on its own.
namespace RenderIntegration {

static ID3D11Texture2D*          g_vmir_tex = nullptr;
static ID3D11DepthStencilView*   g_vmir_dsv = nullptr;
static ID3D11ShaderResourceView* g_vmir_srv = nullptr;
static ID3D11Buffer* g_vmir_b2_src = nullptr;   // 64B raw SRV: engine b2 copy.
static ID3D11ShaderResourceView* g_vmir_b2_src_srv = nullptr;
// KH_MIR_DRAW: the patch's draw target, four R32G32B32A32_FLOAT elements (one
// per matrix row), copied whole into g_vmir_b2_patch.
static ID3D11Buffer* g_vmir_b2_out = nullptr;
static ID3D11RenderTargetView* g_vmir_b2_out_rtv = nullptr;
static ID3D11Buffer* g_vmir_b2_patch = nullptr;   // 64B CB: what the re-issue binds.
static ID3D11VertexShader* g_vmir_b2_vs = nullptr;   // VSMirB2 (vmir_cs.hlsl).
static ID3D11PixelShader*  g_vmir_b2_ps = nullptr;   // PSMirB2.
static bool     g_vmir_b2_failed = false;   // Compile once; a failure stands the pass down.
static bool     g_vmir_pending = false;   // Set per counting draw, consumed by the hook.
static bool     g_vmir_frame_ok = false;   // This frame's patch + prepass both landed.
static uint32_t g_vmir_mark2 = 0;
static uint32_t g_vmir_prepass_stamp = 0;   // Frame seq of the last prepass.
// Depth clamp closes the near-plane hole: with DepthClipEnable false the sub-near cap fragments clamp to z = 0, fail
// the GREATER_EQUAL test and the balance is exact - the textbook z-fail companion.
// KH_VMIR_RS_KEY: each key holds a reference (AddRef at caching, Release at eviction and in kh_vmir_release), as
// kh_rp_rs_mir's does, so the engine's state cannot die and a new state of another desc take its address while the
// clone is cached. Null = the default state (no reference).
static ID3D11RasterizerState* g_vmir_rs_src[2] = {};   // Engine RS the clone shadows.
static ID3D11RasterizerState* g_vmir_rs_clamp[2] = {};   // Same desc, DepthClipEnable false.
static ID3D11RasterizerState* g_vmir_sv_rs = nullptr;
// The secondary session (a volume source other than g_vmir_prepass_src) then counts for the engine exactly as it would
// with no mirror pass, and never touches the mirror.
static ID3D11Texture2D* g_vmir_prepass_src = nullptr;   // Weak identity, compare only.

inline void kh_vmir_release() {
    for (int khvr_k = 0; khvr_k < 2; ++khvr_k) {
        KH_SAFE_RELEASE(g_vmir_rs_src[khvr_k]);   // KH_VMIR_RS_KEY: the key's reference.
        KH_SAFE_RELEASE(g_vmir_rs_clamp[khvr_k]);
    }
    KH_SAFE_RELEASE(g_vmir_sv_rs);
    g_vmir_prepass_src = nullptr;   // Weak key into a dead device's textures.
    KH_SAFE_RELEASE(g_vmir_srv);
    KH_SAFE_RELEASE(g_vmir_dsv);
    KH_SAFE_RELEASE(g_vmir_tex);
    KH_SAFE_RELEASE(g_vmir_b2_src_srv);
    KH_SAFE_RELEASE(g_vmir_b2_src);
    KH_SAFE_RELEASE(g_vmir_b2_out_rtv);
    KH_SAFE_RELEASE(g_vmir_b2_out);
    KH_SAFE_RELEASE(g_vmir_b2_patch);
    KH_SAFE_RELEASE(g_vmir_b2_vs);
    KH_SAFE_RELEASE(g_vmir_b2_ps);
    KH_SAFE_RELEASE(g_vmir_vs);
    g_vmir_b2_failed = false;   // A new device gets a fresh compile attempt.
    g_vmir_w = 0; g_vmir_h = 0;
    g_vmir_pending = false; g_vmir_frame_ok = false;
    g_vmir_mask_time = -1.0f;
}

inline bool kh_vmir_ensure_targets(ID3D11Device* khve_dev) {
    if (!khve_dev || !g_svs_vol_src) return false;
    D3D11_TEXTURE2D_DESC khve_sd = {};
    g_svs_vol_src->GetDesc(&khve_sd);
    if (g_vmir_tex && g_vmir_dsv && g_vmir_srv &&
        g_vmir_w == khve_sd.Width && g_vmir_h == khve_sd.Height) {
        return true;
    }
    KH_SAFE_RELEASE(g_vmir_srv);
    KH_SAFE_RELEASE(g_vmir_dsv);
    KH_SAFE_RELEASE(g_vmir_tex);
    g_vmir_w = 0; g_vmir_h = 0;
    if (khve_sd.SampleDesc.Count != 1) return false;
    if (khve_sd.ArraySize != 1 || khve_sd.MipLevels != 1) return false;
    D3D11_TEXTURE2D_DESC khve_td = khve_sd;
    khve_td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    khve_td.Usage = D3D11_USAGE_DEFAULT;
    khve_td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    khve_td.CPUAccessFlags = 0;
    khve_td.MiscFlags = 0;
    if (FAILED(khve_dev->CreateTexture2D(&khve_td, nullptr, &g_vmir_tex)) || !g_vmir_tex) {
        g_vmir_tex = nullptr;
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC khve_dd = {};
    khve_dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    khve_dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(khve_dev->CreateDepthStencilView(g_vmir_tex, &khve_dd, &g_vmir_dsv))) {
        g_vmir_dsv = nullptr;
        KH_SAFE_RELEASE(g_vmir_tex);
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC khve_vd = {};
    khve_vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    khve_vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    khve_vd.Texture2D.MipLevels = 1;
    if (FAILED(khve_dev->CreateShaderResourceView(g_vmir_tex, &khve_vd, &g_vmir_srv))) {
        g_vmir_srv = nullptr;
        KH_SAFE_RELEASE(g_vmir_dsv);
        KH_SAFE_RELEASE(g_vmir_tex);
        return false;
    }
    g_vmir_w = khve_sd.Width;
    g_vmir_h = khve_sd.Height;
    return true;
}

inline bool kh_vmir_ensure_gpu(ID3D11Device* khvg_dev) {
    if (!khvg_dev) return false;
    if ((!g_vmir_b2_vs || !g_vmir_b2_ps) && !g_vmir_b2_failed) {
        ID3DBlob* khvg_vblob = nullptr;
        ID3DBlob* khvg_pblob = nullptr;
        std::string khvg_err = compile_shader(kh_hlsl_src(KH_HLSL_VMIR_CS).c_str(), "VSMirB2", "vs_5_0",
                                              nullptr, &khvg_vblob);
        if (khvg_err.empty()) {
            khvg_err = compile_shader(kh_hlsl_src(KH_HLSL_VMIR_CS).c_str(), "PSMirB2", "ps_5_0",
                                      nullptr, &khvg_pblob);
        }
        if (!khvg_err.empty() || !khvg_vblob || !khvg_pblob) {
            KH_SAFE_RELEASE(khvg_vblob);
            KH_SAFE_RELEASE(khvg_pblob);
            g_vmir_b2_failed = true;   // Rule: a compile failure names itself.
            report_error_once_safe("KH mirror: patch shader compile failed (the pass stands down): " + khvg_err);
            return false;
        }
        HRESULT khvg_hr = khvg_dev->CreateVertexShader(khvg_vblob->GetBufferPointer(),
                                                       khvg_vblob->GetBufferSize(),
                                                       nullptr, &g_vmir_b2_vs);
        if (SUCCEEDED(khvg_hr)) {
            khvg_hr = khvg_dev->CreatePixelShader(khvg_pblob->GetBufferPointer(),
                                                  khvg_pblob->GetBufferSize(),
                                                  nullptr, &g_vmir_b2_ps);
        }
        khvg_vblob->Release();
        khvg_pblob->Release();
        if (FAILED(khvg_hr) || !g_vmir_b2_vs || !g_vmir_b2_ps) {
            KH_SAFE_RELEASE(g_vmir_b2_vs);
            KH_SAFE_RELEASE(g_vmir_b2_ps);
            g_vmir_b2_failed = true;
            report_error_once_safe("KH mirror: patch shader could not be created (the pass stands down)");
            return false;
        }
    }
    if (g_vmir_b2_failed) return false;
    if (!g_vmir_b2_src) {
        D3D11_BUFFER_DESC khvg_bd = {};
        khvg_bd.ByteWidth = 64;
        khvg_bd.Usage = D3D11_USAGE_DEFAULT;
        khvg_bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        khvg_bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        if (FAILED(khvg_dev->CreateBuffer(&khvg_bd, nullptr, &g_vmir_b2_src)) || !g_vmir_b2_src) {
            g_vmir_b2_src = nullptr;
            return false;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC khvg_sd = {};
        khvg_sd.Format = DXGI_FORMAT_R32_TYPELESS;
        khvg_sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        khvg_sd.BufferEx.FirstElement = 0;
        khvg_sd.BufferEx.NumElements = 16;
        khvg_sd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        if (FAILED(khvg_dev->CreateShaderResourceView(g_vmir_b2_src, &khvg_sd, &g_vmir_b2_src_srv))) {
            g_vmir_b2_src_srv = nullptr;
            KH_SAFE_RELEASE(g_vmir_b2_src);
            return false;
        }
    }
    if (!g_vmir_b2_out) {
        D3D11_BUFFER_DESC khvg_bd = {};
        khvg_bd.ByteWidth = 64;
        khvg_bd.Usage = D3D11_USAGE_DEFAULT;
        khvg_bd.BindFlags = D3D11_BIND_RENDER_TARGET;
        if (FAILED(khvg_dev->CreateBuffer(&khvg_bd, nullptr, &g_vmir_b2_out)) || !g_vmir_b2_out) {
            g_vmir_b2_out = nullptr;
            return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC khvg_rd = {};
        khvg_rd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        khvg_rd.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
        khvg_rd.Buffer.FirstElement = 0;
        khvg_rd.Buffer.NumElements = 4;
        if (FAILED(khvg_dev->CreateRenderTargetView(g_vmir_b2_out, &khvg_rd, &g_vmir_b2_out_rtv))) {
            g_vmir_b2_out_rtv = nullptr;
            KH_SAFE_RELEASE(g_vmir_b2_out);
            return false;
        }
    }
    if (!g_vmir_b2_patch) {
        D3D11_BUFFER_DESC khvg_bd = {};
        khvg_bd.ByteWidth = 64;
        khvg_bd.Usage = D3D11_USAGE_DEFAULT;
        khvg_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(khvg_dev->CreateBuffer(&khvg_bd, nullptr, &g_vmir_b2_patch)) || !g_vmir_b2_patch) {
            g_vmir_b2_patch = nullptr;
            return false;
        }
    }
    return true;
}

// KH_MIR_CPU: the patch is a pure function of b2's first 64 bytes, so: learn which resource is bound at VS b2,
// snapshot it at its upload, patch on the CPU. Strict gate (same resource AND same frame) because a stale b2
// misplaces the mask; otherwise the GPU path runs. KH_MIR_DRAW: that path is a draw (PSMirB2 into a buffer render
// target). Do not make it a Dispatch: a graphics -> compute transition on the immediate context drains the engine's
// in-flight work (measured at 148 ms on a dense frame).
struct KhB2Snap {
    void*    res = nullptr;
    uint64_t frame = 0;
    bool     valid = false;
    float    m[16] = {};
};
static KhB2Snap g_b2_snap;
static void*    g_b2_learn = nullptr;   // The resource last seen bound at VS b2.

inline void kh_b2_snap_note(ID3D11Resource* khb_res, const void* khb_d, uint32_t khb_n) {
    if (!g_b2_learn || static_cast<void*>(khb_res) != g_b2_learn) return;
    if (!khb_d || khb_n < 64u) return;
    kh_upload_need(khb_d, 64u);   // KH_UPLOAD_LAZY.
    memcpy(g_b2_snap.m, khb_d, 64);
    g_b2_snap.res = g_b2_learn;
    g_b2_snap.frame = g_svs_frame_seq;
    g_b2_snap.valid = true;
}

// The C++ twin of PSMirB2 (vmir_cs.hlsl): same order of operations, same constants, same refusal conditions (where the
// shader passes z through untouched, so does this). Columns 0, 1 and 3 are copied verbatim; only column 2 is
// recombined.
inline void kh_mir_patch_cpu(const float khm_in[16], float khm_out[16]) {
    float khm_num = 0.0f, khm_den = 0.0f;

    for (int khm_r = 0; khm_r < 3; ++khm_r) {
        khm_num += khm_in[khm_r * 4 + 2] * khm_in[khm_r * 4 + 3];
        khm_den += khm_in[khm_r * 4 + 3] * khm_in[khm_r * 4 + 3];
    }

    const float khm_w = sqrtf(khm_den);
    bool  khm_ok = khm_den > 1.0e-12f && khm_w > 0.90f && khm_w < 1.10f;
    const float khm_m22 = khm_ok ? khm_num / khm_den : 0.0f;
    khm_ok = khm_ok && fabsf(khm_m22) > 1.0e-9f;
    const float khm_m32 = khm_in[14] - khm_in[15] * khm_m22;
    const float khm_n = khm_ok ? (-khm_m32 / khm_m22) : -1.0f;
    khm_ok = khm_ok && khm_n > 0.0f && khm_m22 > 1.0f + 1.0e-6f && khm_m32 < -1.0e-6f;
    const float khm_f = khm_ok ? (khm_n * khm_m22 / (khm_m22 - 1.0f)) : 0.0f;
    khm_ok = khm_ok && khm_f > 0.06f;
    const float khm_l22 = khm_ok ? (khm_f / (khm_f - 0.05f)) : 0.0f;
    const float khm_l32 = -0.05f * khm_l22;
    const float khm_a = khm_ok ? (khm_l32 / khm_m32) : 1.0f;
    const float khm_b = khm_ok ? (khm_l22 - khm_m22 * khm_a) : 0.0f;

    for (int khm_r2 = 0; khm_r2 < 4; ++khm_r2) {
        const float khm_z = khm_in[khm_r2 * 4 + 2];
        const float khm_ww = khm_in[khm_r2 * 4 + 3];
        khm_out[khm_r2 * 4 + 0] = khm_in[khm_r2 * 4 + 0];
        khm_out[khm_r2 * 4 + 1] = khm_in[khm_r2 * 4 + 1];
        khm_out[khm_r2 * 4 + 2] = khm_ok ? (khm_a * khm_z + khm_b * khm_ww) : khm_z;
        khm_out[khm_r2 * 4 + 3] = khm_ww;
    }
}

// The GPU path draws under a StateBackup and a KhOmSave - the engine's state mid-frame is handed back whole.
inline bool kh_vmir_patch_b2(ID3D11DeviceContext* khvp_ctx) {
    ID3D11Buffer* khvp_b2 = nullptr;
    khvp_ctx->VSGetConstantBuffers(2, 1, &khvp_b2);
    if (!khvp_b2) return false;
    D3D11_BUFFER_DESC khvp_bd = {};
    khvp_b2->GetDesc(&khvp_bd);
    if (khvp_bd.ByteWidth < 64) { khvp_b2->Release(); return false; }
    // Learn which resource is bound at b2 so the upload funnel starts
    // snapshotting it; makes the fast path reachable from the second frame on.
    g_b2_learn = static_cast<void*>(khvp_b2);

    // KH_MIR_CPU: the fast path, only for a snapshot of THIS resource and THIS frame.
    if (g_b2_snap.valid && g_b2_snap.res == static_cast<void*>(khvp_b2) &&
        g_b2_snap.frame == g_svs_frame_seq) {
        float khvp_patched[16];
        kh_mir_patch_cpu(g_b2_snap.m, khvp_patched);
        {
            // KH_VMIR_FAST_OWN: our own buffer - under in_injection, so the upload funnel (the projection / shadow
            // / locator / PIP / camera scanners, the replay's CPU image) never reads the near-patched block as the
            // engine's. The GPU path below fills it by copies, which the funnel does not scan.
            KhInjectionFlagScope khvp_fl;
            g_ro.in_injection = true;
            khvp_ctx->UpdateSubresource(g_vmir_b2_patch, 0, nullptr, khvp_patched, 0, 0);
        }
        khvp_b2->Release();
        return true;
    }

    D3D11_BOX khvp_box = { 0, 0, 0, 64, 1, 1 };
    {
        khvp_ctx->CopySubresourceRegion(g_vmir_b2_src, 0, 0, 0, 0, khvp_b2, 0, &khvp_box);
    }
    khvp_b2->Release();

    // KH_MIR_DRAW: PSMirB2 writes row x of the patched matrix at texel x of a four-texel buffer target; the copy below
    // moves it into the CB. Our own draw: the hooks stand aside (in_injection), and the re-issue flag is held down so
    // the post-draw hook cannot take this draw for a counting draw (the caller assigns it after this returns).
    const bool khvp_prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;
    g_vmir_pending = false;
    {
        StateBackup khvp_bk;
        khvp_bk.capture(khvp_ctx);
        KhOmSave khvp_om;
        khvp_om.capture(khvp_ctx);
        khvp_ctx->OMSetRenderTargets(1, &g_vmir_b2_out_rtv, nullptr);
        const D3D11_VIEWPORT khvp_vp = { 0.0f, 0.0f, 4.0f, 1.0f, 0.0f, 1.0f };
        khvp_ctx->RSSetViewports(1, &khvp_vp);
        // The default state: solid, no scissor, back-face culling (VSMirB2's
        // triangle is clockwise on screen - front-facing).
        khvp_ctx->RSSetState(nullptr);
        khvp_ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        khvp_ctx->OMSetDepthStencilState(nullptr, 0);   // No depth view bound.
        khvp_ctx->IASetInputLayout(nullptr);
        khvp_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        khvp_ctx->VSSetShader(g_vmir_b2_vs, nullptr, 0);
        khvp_ctx->PSSetShader(g_vmir_b2_ps, nullptr, 0);
        khvp_ctx->GSSetShader(nullptr, nullptr, 0);
        khvp_ctx->HSSetShader(nullptr, nullptr, 0);
        khvp_ctx->DSSetShader(nullptr, nullptr, 0);
        khvp_ctx->PSSetShaderResources(0, 1, &g_vmir_b2_src_srv);
        khvp_ctx->Draw(3, 0);
        ID3D11ShaderResourceView* khvp_null = nullptr;
        khvp_ctx->PSSetShaderResources(0, 1, &khvp_null);
        khvp_om.restore(khvp_ctx);
        khvp_om.release();
        khvp_bk.restore(khvp_ctx);
    }
    g_ro.in_injection = khvp_prev_inj;

    khvp_ctx->CopyResource(g_vmir_b2_patch, g_vmir_b2_out);
    return true;
}

// Engine dss / shaders / IA are never touched - the stencil arithmetic stays the engine's own; the rasterizer is
// swapped for a DepthClipEnable = FALSE clone of the engine's own (kh_vmir_begin) and handed back at the end.
static ID3D11RenderTargetView* g_vmir_sv_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
static ID3D11DepthStencilView* g_vmir_sv_dsv = nullptr;
static D3D11_VIEWPORT g_vmir_sv_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
static UINT g_vmir_sv_nvp = 0;
static ID3D11Buffer* g_vmir_sv_b2 = nullptr;
static UINT g_vmir_sv_b2_first = 0;
static UINT g_vmir_sv_b2_num = 0;
static bool g_vmir_sv_b2_off_ok = false;

inline bool kh_vmir_begin(ID3D11DeviceContext* khvb_ctx) {
    KH_PROF_SCOPE(KHP_VMIR);
    // KH_PROF: the GPU zone (KHG_VMIR) is the draw hooks', around the re-issued draw this sets up and kh_vmir_end.
    // Do not stamp it here: it would close on return, before that draw, and measure the state setup alone.
    ID3D11UnorderedAccessView* khvb_uavs[8] = {};
    khvb_ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmir_sv_rtvs, &g_vmir_sv_dsv);
    khvb_ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, khvb_uavs);   // UAV probe only.
    uint32_t khvb_un = 0;
    for (UINT khvb_i = 0; khvb_i < 8; ++khvb_i) {
        if (khvb_uavs[khvb_i]) khvb_un++;
        KH_SAFE_RELEASE(khvb_uavs[khvb_i]);
    }
    if (khvb_un > 0) {
        for (int khvb_r = 0; khvb_r < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khvb_r) KH_SAFE_RELEASE(g_vmir_sv_rtvs[khvb_r]);
        KH_SAFE_RELEASE(g_vmir_sv_dsv);
        return false;
    }
    g_vmir_sv_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    khvb_ctx->RSGetViewports(&g_vmir_sv_nvp, g_vmir_sv_vps);
    KH_SAFE_RELEASE(g_vmir_sv_rs);
    khvb_ctx->RSGetState(&g_vmir_sv_rs);
    ID3D11RasterizerState* khvb_clamp = nullptr;
    for (int khvb_k = 0; khvb_k < 2; ++khvb_k) {
        if (g_vmir_rs_clamp[khvb_k] && g_vmir_rs_src[khvb_k] == g_vmir_sv_rs) {
            khvb_clamp = g_vmir_rs_clamp[khvb_k];
            break;
        }
    }
    if (!khvb_clamp) {
        D3D11_RASTERIZER_DESC khvb_rd = {};
        if (g_vmir_sv_rs) {
            g_vmir_sv_rs->GetDesc(&khvb_rd);
        } else {   // Null RS = the API's default state.
            khvb_rd.FillMode = D3D11_FILL_SOLID;
            khvb_rd.CullMode = D3D11_CULL_BACK;
            khvb_rd.DepthClipEnable = TRUE;
        }
        khvb_rd.DepthClipEnable = FALSE;
        ID3D11Device* khvb_dev = nullptr;
        khvb_ctx->GetDevice(&khvb_dev);
        if (khvb_dev) {
            const int khvb_s = g_vmir_rs_clamp[0] ? 1 : 0;   // 2-slot: evict the younger.
            KH_SAFE_RELEASE(g_vmir_rs_clamp[khvb_s]);
            KH_SAFE_RELEASE(g_vmir_rs_src[khvb_s]);   // KH_VMIR_RS_KEY: the evicted key's reference.
            if (SUCCEEDED(khvb_dev->CreateRasterizerState(&khvb_rd,
                                                          &g_vmir_rs_clamp[khvb_s])) &&
                g_vmir_rs_clamp[khvb_s]) {
                g_vmir_rs_src[khvb_s] = g_vmir_sv_rs;
                if (g_vmir_sv_rs) g_vmir_sv_rs->AddRef();   // KH_VMIR_RS_KEY.
                khvb_clamp = g_vmir_rs_clamp[khvb_s];
            } else {
                g_vmir_rs_clamp[khvb_s] = nullptr;
                g_vmir_rs_src[khvb_s] = nullptr;
            }
            khvb_dev->Release();
        }
    }
    if (khvb_clamp) {
        khvb_ctx->RSSetState(khvb_clamp);
    }
    g_vmir_sv_b2 = nullptr;
    g_vmir_sv_b2_first = 0;
    g_vmir_sv_b2_num = 0;
    // StateBackup's rule: a driver without offsetting drops Set1, which would leave our patch bound at b2.
    ID3D11DeviceContext1* khvb_c1 = g_cb_offsetting ? kh_ctx1(khvb_ctx) : nullptr;
    if (khvb_c1) {
        khvb_c1->VSGetConstantBuffers1(2, 1, &g_vmir_sv_b2, &g_vmir_sv_b2_first, &g_vmir_sv_b2_num);
        g_vmir_sv_b2_off_ok = true;
    } else {
        khvb_ctx->VSGetConstantBuffers(2, 1, &g_vmir_sv_b2);
        g_vmir_sv_b2_off_ok = false;
    }
    {
        // KH_VMIR_OM_OWN: our bind, under in_injection as every other own bind is - through the hooked OM it would read
        // as the engine's (the main-depth test, the PIP / shadow / seam trackers, and a re-armed seam every counting
        // draw once the prepass stamped the mirror without the footprint's own stamp).
        KhInjectionFlagScope khvb_fl;
        g_ro.in_injection = true;
        khvb_ctx->OMSetRenderTargets(0, nullptr, g_vmir_dsv);
    }
    D3D11_VIEWPORT khvb_vp = {};
    khvb_vp.Width = static_cast<float>(g_vmir_w);
    khvb_vp.Height = static_cast<float>(g_vmir_h);
    khvb_vp.MinDepth = 0.0f;
    khvb_vp.MaxDepth = 1.0f;
    khvb_ctx->RSSetViewports(1, &khvb_vp);
    khvb_ctx->VSSetConstantBuffers(2, 1, &g_vmir_b2_patch);
    return true;
}

inline void kh_vmir_end(ID3D11DeviceContext* khvn_ctx) {
    KH_PROF_SCOPE(KHP_VMIR);   // KH_PROF.
    khvn_ctx->RSSetState(g_vmir_sv_rs);   // Engine clip state back first.
    KH_SAFE_RELEASE(g_vmir_sv_rs);
    {
        KhInjectionFlagScope khvn_fl;   // KH_VMIR_OM_OWN: the engine's set back, which its trackers already saw.
        g_ro.in_injection = true;
        khvn_ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmir_sv_rtvs, g_vmir_sv_dsv);
    }
    for (int khvn_r = 0; khvn_r < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khvn_r) KH_SAFE_RELEASE(g_vmir_sv_rtvs[khvn_r]);
    KH_SAFE_RELEASE(g_vmir_sv_dsv);
    if (g_vmir_sv_nvp > 0) khvn_ctx->RSSetViewports(g_vmir_sv_nvp, g_vmir_sv_vps);
    ID3D11DeviceContext1* khvn_c1 = g_vmir_sv_b2_off_ok ? kh_ctx1(khvn_ctx) : nullptr;
    if (g_vmir_sv_b2 && khvn_c1 && g_vmir_sv_b2_num > 0) {   // StateBackup's rule: the ranged set only with a range.
        khvn_c1->VSSetConstantBuffers1(2, 1, &g_vmir_sv_b2, &g_vmir_sv_b2_first, &g_vmir_sv_b2_num);
    } else {
        khvn_ctx->VSSetConstantBuffers(2, 1, &g_vmir_sv_b2);
    }
    KH_SAFE_RELEASE(g_vmir_sv_b2);
}
}