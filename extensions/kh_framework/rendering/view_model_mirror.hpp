#pragma once

// The injection half (rendering_integration_injection.hpp): the view-model mirror (VMIR): its targets, the b2 patch
// and the bracket. Never include it on its own.
namespace RenderIntegration {


inline void vmir_release() {
    for (int k = 0; k < 2; ++k) {
        KH_SAFE_RELEASE(g_vmir.rs_src[k]);   // VMIR_RS_KEY: the key's reference.
        KH_SAFE_RELEASE(g_vmir.rs_clamp[k]);
    }
    KH_SAFE_RELEASE(g_vmir.sv_rs);
    g_vmir.prepass_src = nullptr;   // Weak key into a dead device's textures.
    KH_SAFE_RELEASE(g_vmir.srv);
    KH_SAFE_RELEASE(g_vmir.dsv);
    KH_SAFE_RELEASE(g_vmir.tex);
    KH_SAFE_RELEASE(g_vmir.b2_src_srv);
    KH_SAFE_RELEASE(g_vmir.b2_src);
    KH_SAFE_RELEASE(g_vmir.b2_out_rtv);
    KH_SAFE_RELEASE(g_vmir.b2_out);
    KH_SAFE_RELEASE(g_vmir.b2_patch);
    KH_SAFE_RELEASE(g_vmir.b2_vs);
    KH_SAFE_RELEASE(g_vmir.b2_ps);
    KH_SAFE_RELEASE(g_vmir.vs);
    g_vmir.b2_failed = false;   // A new device gets a fresh compile attempt.
    g_vmir.w = 0; g_vmir.h = 0;
    g_vmir.pending = false; g_vmir.frame_ok = false;
    g_vmir.mask_time = -1.0f;
}

inline bool vmir_ensure_targets(ID3D11Device* dev) {
    if (!dev || !g_svs.vol_src) return false;
    D3D11_TEXTURE2D_DESC sd = {};
    g_svs.vol_src->GetDesc(&sd);
    if (g_vmir.tex && g_vmir.dsv && g_vmir.srv &&
        g_vmir.w == sd.Width && g_vmir.h == sd.Height) {
        return true;
    }
    KH_SAFE_RELEASE(g_vmir.srv);
    KH_SAFE_RELEASE(g_vmir.dsv);
    KH_SAFE_RELEASE(g_vmir.tex);
    g_vmir.w = 0; g_vmir.h = 0;
    if (sd.SampleDesc.Count != 1) return false;
    if (sd.ArraySize != 1 || sd.MipLevels != 1) return false;
    D3D11_TEXTURE2D_DESC td = sd;
    td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_vmir.tex)) || !g_vmir.tex) {
        g_vmir.tex = nullptr;
        return false;
    }
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(dev->CreateDepthStencilView(g_vmir.tex, &dd, &g_vmir.dsv))) {
        g_vmir.dsv = nullptr;
        KH_SAFE_RELEASE(g_vmir.tex);
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
    vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    vd.Texture2D.MipLevels = 1;
    if (FAILED(dev->CreateShaderResourceView(g_vmir.tex, &vd, &g_vmir.srv))) {
        g_vmir.srv = nullptr;
        KH_SAFE_RELEASE(g_vmir.dsv);
        KH_SAFE_RELEASE(g_vmir.tex);
        return false;
    }
    g_vmir.w = sd.Width;
    g_vmir.h = sd.Height;
    return true;
}

inline bool vmir_ensure_gpu(ID3D11Device* dev) {
    if (!dev) return false;
    if ((!g_vmir.b2_vs || !g_vmir.b2_ps) && !g_vmir.b2_failed) {
        ID3DBlob* vblob = nullptr;
        ID3DBlob* pblob = nullptr;
        std::string err = compile_shader(hlsl_src(HLSL_VMIR_CS).c_str(), "VSMirB2", "vs_5_0",
                                         nullptr, &vblob);
        if (err.empty()) {
            err = compile_shader(hlsl_src(HLSL_VMIR_CS).c_str(), "PSMirB2", "ps_5_0",
                                 nullptr, &pblob);
        }
        if (!err.empty() || !vblob || !pblob) {
            KH_SAFE_RELEASE(vblob);
            KH_SAFE_RELEASE(pblob);
            g_vmir.b2_failed = true;   // Rule: a compile failure names itself.
            report_error_once_safe("KH RenderIntegration: mirror: patch shader compile failed (the pass stands "
                                   "down): " + err);
            return false;
        }
        HRESULT hr = dev->CreateVertexShader(vblob->GetBufferPointer(),
                                             vblob->GetBufferSize(),
                                             nullptr, &g_vmir.b2_vs);
        if (SUCCEEDED(hr)) {
            hr = dev->CreatePixelShader(pblob->GetBufferPointer(),
                                        pblob->GetBufferSize(),
                                        nullptr, &g_vmir.b2_ps);
        }
        vblob->Release();
        pblob->Release();
        if (FAILED(hr) || !g_vmir.b2_vs || !g_vmir.b2_ps) {
            KH_SAFE_RELEASE(g_vmir.b2_vs);
            KH_SAFE_RELEASE(g_vmir.b2_ps);
            g_vmir.b2_failed = true;
            report_error_once_safe("KH RenderIntegration: mirror: patch shader could not be created (the pass stands "
                                   "down)");
            return false;
        }
    }
    if (g_vmir.b2_failed) return false;
    if (!g_vmir.b2_src) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = 64;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_vmir.b2_src)) || !g_vmir.b2_src) {
            g_vmir.b2_src = nullptr;
            return false;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_R32_TYPELESS;
        sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        sd.BufferEx.FirstElement = 0;
        sd.BufferEx.NumElements = 16;
        sd.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
        if (FAILED(dev->CreateShaderResourceView(g_vmir.b2_src, &sd, &g_vmir.b2_src_srv))) {
            g_vmir.b2_src_srv = nullptr;
            KH_SAFE_RELEASE(g_vmir.b2_src);
            return false;
        }
    }
    if (!g_vmir.b2_out) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = 64;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_RENDER_TARGET;
        if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_vmir.b2_out)) || !g_vmir.b2_out) {
            g_vmir.b2_out = nullptr;
            return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        rd.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
        rd.Buffer.FirstElement = 0;
        rd.Buffer.NumElements = 4;
        if (FAILED(dev->CreateRenderTargetView(g_vmir.b2_out, &rd, &g_vmir.b2_out_rtv))) {
            g_vmir.b2_out_rtv = nullptr;
            KH_SAFE_RELEASE(g_vmir.b2_out);
            return false;
        }
    }
    if (!g_vmir.b2_patch) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = 64;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        if (FAILED(dev->CreateBuffer(&bd, nullptr, &g_vmir.b2_patch)) || !g_vmir.b2_patch) {
            g_vmir.b2_patch = nullptr;
            return false;
        }
    }
    return true;
}

// MIR_CPU: the patch is a pure function of b2's first 64 bytes, so: learn which resource is bound at VS b2,
// snapshot it at its upload, patch on the CPU. Strict gate (same resource AND same frame) because a stale b2
// misplaces the mask; otherwise the GPU path runs. MIR_DRAW: that path is a draw (PSMirB2 into a buffer render
// target). Do not make it a Dispatch: a graphics -> compute transition on the immediate context drains the engine's
// in-flight work (measured at 148 ms on a dense frame).
struct B2Snap {
    void*    res = nullptr;
    uint64_t frame = 0;
    bool     valid = false;
    float    m[16] = {};
};
static B2Snap g_b2_snap;
static void*    g_b2_learn = nullptr;   // The resource last seen bound at VS b2.

inline void b2_snap_note(ID3D11Resource* res, const void* d, uint32_t n) {
    if (!g_b2_learn || static_cast<void*>(res) != g_b2_learn) return;
    if (!d || n < 64u) return;
    upload_need(d, 64u);   // UPLOAD_LAZY.
    memcpy(g_b2_snap.m, d, 64);
    g_b2_snap.res = g_b2_learn;
    g_b2_snap.frame = g_svs.frame_seq;
    g_b2_snap.valid = true;
}

// The C++ twin of PSMirB2 (vmir_cs.hlsl): same order of operations, same constants, same refusal conditions (where the
// shader passes z through untouched, so does this). Columns 0, 1 and 3 are copied verbatim; only column 2 is
// recombined.
inline void mir_patch_cpu(const float in[16], float out[16]) {
    float num = 0.0f, den = 0.0f;

    for (int r = 0; r < 3; ++r) {
        num += in[r * 4 + 2] * in[r * 4 + 3];
        den += in[r * 4 + 3] * in[r * 4 + 3];
    }

    const float w = sqrtf(den);
    bool  ok = den > 1.0e-12f && w > 0.90f && w < 1.10f;
    const float m22 = ok ? num / den : 0.0f;
    ok = ok && fabsf(m22) > 1.0e-9f;
    const float m32 = in[14] - in[15] * m22;
    const float n = ok ? (-m32 / m22) : -1.0f;
    ok = ok && n > 0.0f && m22 > 1.0f + 1.0e-6f && m32 < -1.0e-6f;
    const float f = ok ? (n * m22 / (m22 - 1.0f)) : 0.0f;
    ok = ok && f > 0.06f;
    const float l22 = ok ? (f / (f - 0.05f)) : 0.0f;
    const float l32 = -0.05f * l22;
    const float a = ok ? (l32 / m32) : 1.0f;
    const float b = ok ? (l22 - m22 * a) : 0.0f;

    for (int r2 = 0; r2 < 4; ++r2) {
        const float z = in[r2 * 4 + 2];
        const float ww = in[r2 * 4 + 3];
        out[r2 * 4 + 0] = in[r2 * 4 + 0];
        out[r2 * 4 + 1] = in[r2 * 4 + 1];
        out[r2 * 4 + 2] = ok ? (a * z + b * ww) : z;
        out[r2 * 4 + 3] = ww;
    }
}

// The GPU path draws under a StateBackup and a OmSave - the engine's state mid-frame is handed back whole. Both
// paths run with the engine's predicate set aside (it comes back at the return); the re-issued draw keeps it.
inline bool vmir_patch_b2(ID3D11DeviceContext* ctx) {
    ID3D11Buffer* b2 = nullptr;
    ctx->VSGetConstantBuffers(2, 1, &b2);
    if (!b2) return false;
    D3D11_BUFFER_DESC bd = {};
    b2->GetDesc(&bd);
    if (bd.ByteWidth < 64) { b2->Release(); return false; }
    // Learn which resource is bound at b2 so the upload funnel starts
    // snapshotting it; makes the fast path reachable from the second frame on.
    g_b2_learn = static_cast<void*>(b2);
    // EA_VMIR_PRED: the patch's upload and copies are GPU writes of ours outside a StateBackup (PRED_ASIDE).
    PredAside pa;
    pa.begin(ctx);

    // MIR_CPU: the fast path, only for a snapshot of THIS resource and THIS frame.
    if (g_b2_snap.valid && g_b2_snap.res == static_cast<void*>(b2) &&
        g_b2_snap.frame == g_svs.frame_seq) {
        float patched[16];
        mir_patch_cpu(g_b2_snap.m, patched);
        {
            // VMIR_FAST_OWN: our own buffer - under in_injection, so the upload funnel (the projection / shadow
            // / locator / PIP / camera scanners, the replay's CPU image) never reads the near-patched block as the
            // engine's. The GPU path below fills it by copies, which the funnel does not scan.
            InjectionFlagScope fl;
            g_ro.in_injection = true;
            ctx->UpdateSubresource(g_vmir.b2_patch, 0, nullptr, patched, 0, 0);
        }
        b2->Release();
        return true;
    }

    D3D11_BOX box = { 0, 0, 0, 64, 1, 1 };
    {
        ctx->CopySubresourceRegion(g_vmir.b2_src, 0, 0, 0, 0, b2, 0, &box);
    }
    b2->Release();

    // MIR_DRAW: PSMirB2 writes row x of the patched matrix at texel x of a four-texel buffer target; the copy below
    // moves it into the CB. Our own draw: the hooks stand aside (in_injection), and the re-issue flag is held down so
    // the post-draw hook cannot take this draw for a counting draw (the caller assigns it after this returns).
    const bool prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;
    g_vmir.pending = false;
    {
        StateBackup bk;
        bk.capture(ctx);
        OmSave om;
        om.capture(ctx);
        ctx->OMSetRenderTargets(1, &g_vmir.b2_out_rtv, nullptr);
        const D3D11_VIEWPORT vp = { 0.0f, 0.0f, 4.0f, 1.0f, 0.0f, 1.0f };
        ctx->RSSetViewports(1, &vp);
        // The default state: solid, no scissor, back-face culling (VSMirB2's
        // triangle is clockwise on screen - front-facing).
        ctx->RSSetState(nullptr);
        ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(nullptr, 0);   // No depth view bound.
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_vmir.b2_vs, nullptr, 0);
        ctx->PSSetShader(g_vmir.b2_ps, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->HSSetShader(nullptr, nullptr, 0);
        ctx->DSSetShader(nullptr, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &g_vmir.b2_src_srv);
        ctx->Draw(3, 0);
        ID3D11ShaderResourceView* null = nullptr;
        ctx->PSSetShaderResources(0, 1, &null);
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
    }
    g_ro.in_injection = prev_inj;

    ctx->CopyResource(g_vmir.b2_patch, g_vmir.b2_out);
    return true;
}


inline bool vmir_begin(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_VMIR);
    // PROF: the GPU zone (GPU_VMIR) is the draw hooks', around the re-issued draw this sets up and vmir_end.
    // Do not stamp it here: it would close on return, before that draw, and measure the state setup alone.
    ID3D11UnorderedAccessView* uavs[8] = {};
    ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmir.sv_rtvs, &g_vmir.sv_dsv);
    ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, uavs);   // UAV probe only.
    uint32_t un = 0;
    for (UINT i = 0; i < 8; ++i) {
        if (uavs[i]) un++;
        KH_SAFE_RELEASE(uavs[i]);
    }
    if (un > 0) {
        for (int r = 0; r < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
            ++r) KH_SAFE_RELEASE(g_vmir.sv_rtvs[r]);
        KH_SAFE_RELEASE(g_vmir.sv_dsv);
        return false;
    }
    g_vmir.sv_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    ctx->RSGetViewports(&g_vmir.sv_nvp, g_vmir.sv_vps);
    KH_SAFE_RELEASE(g_vmir.sv_rs);
    ctx->RSGetState(&g_vmir.sv_rs);
    ID3D11RasterizerState* clamp = nullptr;
    for (int k = 0; k < 2; ++k) {
        if (g_vmir.rs_clamp[k] && g_vmir.rs_src[k] == g_vmir.sv_rs) {
            clamp = g_vmir.rs_clamp[k];
            break;
        }
    }
    if (!clamp) {
        D3D11_RASTERIZER_DESC rd = {};
        if (g_vmir.sv_rs) {
            g_vmir.sv_rs->GetDesc(&rd);
        } else {   // Null RS = the API's default state.
            rd.FillMode = D3D11_FILL_SOLID;
            rd.CullMode = D3D11_CULL_BACK;
            rd.DepthClipEnable = TRUE;
        }
        rd.DepthClipEnable = FALSE;
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (dev) {
            const int s = g_vmir.rs_clamp[0] ? 1 : 0;   // 2-slot: evict the younger.
            KH_SAFE_RELEASE(g_vmir.rs_clamp[s]);
            KH_SAFE_RELEASE(g_vmir.rs_src[s]);   // VMIR_RS_KEY: the evicted key's reference.
            if (SUCCEEDED(dev->CreateRasterizerState(&rd,
                                                     &g_vmir.rs_clamp[s])) &&
                g_vmir.rs_clamp[s]) {
                g_vmir.rs_src[s] = g_vmir.sv_rs;
                if (g_vmir.sv_rs) g_vmir.sv_rs->AddRef();   // VMIR_RS_KEY.
                clamp = g_vmir.rs_clamp[s];
            } else {
                g_vmir.rs_clamp[s] = nullptr;
                g_vmir.rs_src[s] = nullptr;
            }
            dev->Release();
        }
    }
    if (clamp) {
        ctx->RSSetState(clamp);
    }
    g_vmir.sv_b2 = nullptr;
    g_vmir.sv_b2_first = 0;
    g_vmir.sv_b2_num = 0;
    // StateBackup's rule: a driver without offsetting drops Set1, which would leave our patch bound at b2.
    ID3D11DeviceContext1* c1 = g_cb_offsetting ? as_ctx1(ctx) : nullptr;
    if (c1) {
        c1->VSGetConstantBuffers1(2, 1, &g_vmir.sv_b2, &g_vmir.sv_b2_first, &g_vmir.sv_b2_num);
        g_vmir.sv_b2_off_ok = true;
    } else {
        ctx->VSGetConstantBuffers(2, 1, &g_vmir.sv_b2);
        g_vmir.sv_b2_off_ok = false;
    }
    {
        // VMIR_OM_OWN: our bind, under in_injection as every other own bind is - through the hooked OM it would read
        // as the engine's (the main-depth test, the PIP / shadow / seam trackers, and a re-armed seam every counting
        // draw once the prepass stamped the mirror without the footprint's own stamp).
        InjectionFlagScope fl;
        g_ro.in_injection = true;
        ctx->OMSetRenderTargets(0, nullptr, g_vmir.dsv);
    }
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(g_vmir.w);
    vp.Height = static_cast<float>(g_vmir.h);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);
    ctx->VSSetConstantBuffers(2, 1, &g_vmir.b2_patch);
    return true;
}

inline void vmir_end(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_VMIR);   // PROF.
    ctx->RSSetState(g_vmir.sv_rs);   // Engine clip state back first.
    KH_SAFE_RELEASE(g_vmir.sv_rs);
    {
        InjectionFlagScope fl;   // VMIR_OM_OWN: the engine's set back, which its trackers already saw.
        g_ro.in_injection = true;
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmir.sv_rtvs, g_vmir.sv_dsv);
    }
    for (int r = 0; r < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
        ++r) KH_SAFE_RELEASE(g_vmir.sv_rtvs[r]);
    KH_SAFE_RELEASE(g_vmir.sv_dsv);
    if (g_vmir.sv_nvp > 0) ctx->RSSetViewports(g_vmir.sv_nvp, g_vmir.sv_vps);
    ID3D11DeviceContext1* c1 = g_vmir.sv_b2_off_ok ? as_ctx1(ctx) : nullptr;
    if (g_vmir.sv_b2 && c1 && g_vmir.sv_b2_num > 0) {   // StateBackup's rule: the ranged set only with a range.
        c1->VSSetConstantBuffers1(2, 1, &g_vmir.sv_b2, &g_vmir.sv_b2_first, &g_vmir.sv_b2_num);
    } else {
        ctx->VSSetConstantBuffers(2, 1, &g_vmir.sv_b2);
    }
    KH_SAFE_RELEASE(g_vmir.sv_b2);
}
}