#pragma once

// The feature half (rendering_integration_features.hpp): SSAO and the near-z marker. Never include it on its own.
namespace RenderIntegration {

// SSAO - screen-space ambient occlusion for our meshes (ssao.hlsl; the settings are setRenderAmbientOcclusion's).
// After a drawer's depth-writing draws (the injection's, or the flush's late ones) the term is computed from the live
// main depth, which then holds the world's opaques and ours, smoothed by three separable blur levels (SSAO_SEP), and
// multiplied into the scene colour at OUR samples (ssao_post), over the meshes' pixel rectangle (snapshot_rect,
// padded).
//
// WHICH PIXELS ARE OURS is the stencil's answer (SSAO_MARK). The main depth is D24S8 and the engine's world draws
// never write its stencil (measured: the hands' Replace 0 there is a no-op). Every depth-writing draw of ours binds
// dss_test_write_mark, ref 1, so a pixel it wins carries a 1 (3 for a see-through blend mode, which the term skips:
// SSAO_SEE_THROUGH); the AO pass applies where the mark is and then ERASES it (dss_mark_clear), so each pixel takes
// the term exactly once from whichever drawer painted it last - whatever the order of the flush's park and the
// injection's landing, however many times a cycle injects, whether a moving mesh's two draws agree. (A depth copy taken
// before the draws could not tell one drawer's pixels from another's, and every race between them flickered.) The mark
// lives only between our draws and our pass, one contiguous span on one thread; the engine never sees it set.
//
// The engine's pixels are left as they are (the engine has its own AO); the engine's depth occludes ours, so a mesh
// darkens where it meets the ground or a wall. Skinned, cloth and chain meshes take the term from the depth they were
// drawn with, not a shape baked at import. Off when the strength is 0. The PIP passes and the view-model slice draw no
// term; a depth format without a stencil plane draws none either.
//
// Replaced the distance-field AO (a SDF_N^3 field per mesh in the mesh cache, a Texture3D atlas, occluder records at
// t34 / t40 / t41, a six-cone trace); three CBFrame lanes and the cache chunk went with it (the mesh cache version was
// bumped).
struct SsaoCb {   // HLSL twin CBSsao (ssao.hlsl, b0), 5 float4.
    float proj[4];   // x = m00, y = m11, z = m22, w = m32 (the drawer's projection: the injection's or the flush's).
    float vp[4];     // x = viewport MinDepth, y = MaxDepth, z = target width, w = target height.
    float ctl[4];    // x = radius (m), y = strength (the exponent), z = the draw's blur stride
                     // multiplier (1, 2, 3 by level); w = the blur's axis and rule (SSAO_SEP: 0 / 1 = x / y,
                     // + 2 = the third level's stride rule). Only the blurs read z / w: the gather's and the
                     // near marker's uploads carry z = 1, w = 0; the apply draws under the last blur's.
    float rect[4];   // The pass rectangle, left / top / right / bottom (px, exclusive): the
                     // blurs read no term texel outside it (nothing wrote there this frame).
    float half[4];   // x, y = the half-resolution grid's width and height; z = the near-z route's near (0 = no route
                     // this pass), w = its gap floor (SSAO_NEARZ).
};
static_assert(sizeof(SsaoCb) == 80, "SsaoCb is 5 float4 (HLSL twin CBSsao)");
// The pass rectangle is the meshes' (snapshot_rect, its SNAP_RECT_MARGIN included) padded by the passes' reach
// beyond a marked pixel. One reach binds the pad: PSSsaoMain's wide normal, four half-grid depth taps at a stride of at
// most 24 half pixels = 48 full, which take no rectangle test. The blurs' and the apply's taps (three separable levels
// at strides of at most 3, 6 and 4 half pixels, SSAO_SEP: 52 full pixels, and the upsample's half texel past them,
// 54) are rectangle-tested and read only texels of ours (SSAO_OURS), which lie inside the unpadded rectangle, so
// they never reach into the pad. A marked pixel lies inside the unpadded rectangle (the mark is a depth write of a mesh
// whose box the rectangle bounds), so the wide normal stays on half-grid texels PSSsaoDepth wrote this frame; the
// whole-target fallback writes every texel. The gather's samples (up to 256 half pixels out) are rejected outside the
// rectangle, so its search ends 16 + 72 = 88 full pixels past the meshes' box: where the radius spans more on screen (a
// mesh up close), the occluders beyond go unsampled and the contact term is weaker than the radius says.
static constexpr LONG SSAO_RECT_PAD = 72;
inline float ao_strength() {
    const uint32_t b = g_ao_strength_bits.load(std::memory_order_relaxed);
    float f = 0.0f;
    memcpy(&f, &b, sizeof(f));
    return f;
}
inline float ao_dist() {
    const uint32_t b = g_ao_dist_bits.load(std::memory_order_relaxed);
    float f = 0.5f;
    memcpy(&f, &b, sizeof(f));
    return f;
}
// NEARZ_MARK - the effect chain runs late (after the view model: FX_LATE), when the SSAO mark is erased; the
// engine's hands would hold the same depths as our near-z gap if their slice reached it (measured, it ends at 0.01,
// below the gap's floor 0.92 x 0.011 = 0.01012), so the depth alone is not trusted to say a gap pixel is ours. Each
// drawer's ssao_post, while its mark still stands, writes (raw, true distance) at its near-z gap pixels into
// this marker (PSSsaoNearMark, KhSaMetersAt's test and decode); the chain arms it (shadowMeta2.x, t37) and
// cb.hlsl's LoadDepthPS takes the true distance wherever the marker's raw is still the live raw - a pixel
// drawn over since (the hands) keeps its own. Valid for the cycle that wrote it (g_nzm_cycle).
// EA_NZM_XT: written by the drawers (render thread, or a park), read by the chain - also the game thread's late
// chain (FX_LATE) - against the published cycle, as vmsee_valid / the WSEE stamps are (XTHREAD_ATOMIC).
static std::atomic<uint64_t> g_nzm_cycle{ ~0ull };   // The cycle the marker was cleared and written in (~0 = none).
inline bool nzm_valid() {
    return g_nzm_cycle.load(std::memory_order_relaxed) == g_topo_cycles_pub.load(std::memory_order_relaxed) &&
           g_res.nzm_srv != nullptr;
}
// The marker may have a reader this cycle: a visible fullscreen or local pass the scene chain runs, which arms it
// (the flush's rule: every fullscreen pass but a UI-mode one - ui_only - which the UI chain runs with no marker
// armed), and that reads the depth (NEARZ_READER: a pass that never calls LoadDepthPS never reads the marker). An
// effect mesh is not one - no effect mesh is armed (NEARZ_MARK's limit). Without a reader the marker is not
// written; with SSAO off as well the drawers do not mark (ssao_pre returns on neither term nor marker) and the
// SSAO pass does not run at all.
inline bool nzm_wanted() {
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (o.visible && (o.fullscreen || o.localized) && !o.ui_only && fx_needs_depth(o)) return true;
    }
    return false;
}
// The five pixel shaders, compiled at the live depth's sample count (the depth-resolve recipe). The define table is
// byte for byte the speculation batch's.
inline std::string ensure_ssao_shaders(ID3D11Device* dev) {
    const UINT sc = g_res.depth_sample_count > 0 ? g_res.depth_sample_count : 1;
    if (g_res.ps_ssao_depth && g_res.ps_ssao_main && g_res.ps_ssao_blur && g_res.ps_ssao_apply &&
        g_res.ps_ssao_nzmark && g_res.ps_ssao_samples == sc) return "";   // NEARZ_MARK: five.
    auto drop = []() {   // All five or none (a failed compile or create below drops the ones made).
        KH_SAFE_RELEASE(g_res.ps_ssao_depth);
        KH_SAFE_RELEASE(g_res.ps_ssao_main);
        KH_SAFE_RELEASE(g_res.ps_ssao_blur);
        KH_SAFE_RELEASE(g_res.ps_ssao_apply);
        KH_SAFE_RELEASE(g_res.ps_ssao_nzmark);   // NEARZ_MARK.
    };
    drop();
    const std::string scs = std::to_string(sc);
    const D3D_SHADER_MACRO defs[] = {
        { "MSAA_DEPTH", sc > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", scs.c_str() },
        { nullptr, nullptr },
    };
    const char* src = hlsl_src(HLSL_SSAO).c_str();
    struct SsaoEntry { const char* entry; ID3D11PixelShader** out; };
    const SsaoEntry e[5] = {
        { "PSSsaoDepth", &g_res.ps_ssao_depth },
        { "PSSsaoMain",  &g_res.ps_ssao_main },
        { "PSSsaoBlur",  &g_res.ps_ssao_blur },
        { "PSSsaoApply", &g_res.ps_ssao_apply },
        { "PSSsaoNearMark", &g_res.ps_ssao_nzmark },   // NEARZ_MARK.
    };
    for (const SsaoEntry& x : e) {
        ID3DBlob* b = nullptr;
        const std::string err = compile_shader(src, x.entry, "ps_5_0", defs, &b);
        if (!err.empty() || !b) {
            drop();
            return std::string(x.entry) + ": " + err;
        }
        const HRESULT hr = dev->CreatePixelShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, x.out);
        b->Release();
        if (FAILED(hr)) {
            drop();
            return std::string("create ") + x.entry + " " + hr_str(hr);
        }
    }
    g_res.ps_ssao_samples = sc;
    return "";
}
inline void ssao_targets_release() {
    KH_SAFE_RELEASE(g_res.ssao_dh_srv);
    KH_SAFE_RELEASE(g_res.ssao_dh_rtv);
    KH_SAFE_RELEASE(g_res.ssao_dh_tex);
    KH_SAFE_RELEASE(g_res.ssao_ao_srv);
    KH_SAFE_RELEASE(g_res.ssao_ao_rtv);
    KH_SAFE_RELEASE(g_res.ssao_ao_tex);
    KH_SAFE_RELEASE(g_res.ssao_ao2_srv);
    KH_SAFE_RELEASE(g_res.ssao_ao2_rtv);
    KH_SAFE_RELEASE(g_res.ssao_ao2_tex);
    KH_SAFE_RELEASE(g_res.nzm_srv);   // NEARZ_MARK: follows the main depth's size too.
    KH_SAFE_RELEASE(g_res.nzm_rtv);
    KH_SAFE_RELEASE(g_res.nzm_tex);
    g_res.nzm_w = g_res.nzm_h = 0;
    g_res.ssao_w = 0;
    g_res.ssao_h = 0;
    g_res.ssao_hw = 0;
    g_res.ssao_hh = 0;
}
// The half grid (the main depth's size rounded up to even, halved): its sample-0 depth in metres (R32_FLOAT), the
// term and its ping-pong twin (R8_UNORM); and the 80-byte b0. "" = all stand; else what was refused (the caller
// reports it once - EA_SSAO_REPORT: the other effect targets' rule).
inline std::string ensure_ssao_targets(ID3D11Device* dev, UINT w, UINT h) {
    if (!g_res.ssao_cb) {
        D3D11_BUFFER_DESC cd = {};
        cd.ByteWidth = sizeof(SsaoCb);
        cd.Usage = D3D11_USAGE_DYNAMIC;
        cd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        const HRESULT chr = dev->CreateBuffer(&cd, nullptr, &g_res.ssao_cb);
        if (FAILED(chr)) { g_res.ssao_cb = nullptr; return "create ssao CB " + hr_str(chr); }
    }
    if (g_res.ssao_dh_tex && g_res.ssao_dh_rtv && g_res.ssao_dh_srv &&
        g_res.ssao_ao_tex && g_res.ssao_ao_rtv && g_res.ssao_ao_srv &&
        g_res.ssao_ao2_tex && g_res.ssao_ao2_rtv && g_res.ssao_ao2_srv &&
        g_res.ssao_w == w && g_res.ssao_h == h) return "";
    ssao_targets_release();
    const UINT hw = (w + 1u) / 2u, hh = (h + 1u) / 2u;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = hw;
    td.Height = hh;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    // One target: its texture, RTV and SRV at td, or the first refusal's HRESULT.
    auto make = [&](ID3D11Texture2D** t, ID3D11RenderTargetView** r,
                    ID3D11ShaderResourceView** s) -> HRESULT {
        HRESULT sm_hr = dev->CreateTexture2D(&td, nullptr, t);
        if (SUCCEEDED(sm_hr)) sm_hr = dev->CreateRenderTargetView(*t, nullptr, r);
        if (SUCCEEDED(sm_hr)) sm_hr = dev->CreateShaderResourceView(*t, nullptr, s);
        return sm_hr;
    };
    td.Format = DXGI_FORMAT_R32_FLOAT;
    HRESULT st_hr = make(&g_res.ssao_dh_tex, &g_res.ssao_dh_rtv, &g_res.ssao_dh_srv);
    td.Format = DXGI_FORMAT_R8_UNORM;
    if (SUCCEEDED(st_hr)) st_hr = make(&g_res.ssao_ao_tex, &g_res.ssao_ao_rtv, &g_res.ssao_ao_srv);
    if (SUCCEEDED(st_hr)) st_hr = make(&g_res.ssao_ao2_tex, &g_res.ssao_ao2_rtv, &g_res.ssao_ao2_srv);
    if (FAILED(st_hr)) {
        ssao_targets_release();
        return "create targets " + hr_str(st_hr);
    }
    g_res.ssao_w = w;
    g_res.ssao_h = h;
    g_res.ssao_hw = hw;
    g_res.ssao_hh = hh;
    return "";
}
// The rectangle's half-grid viewport: edges rounded outward (the shader's KhSaInRectHalf takes the same edges), so a
// non-empty rectangle gives at least one texel each way.
inline void ssao_viewport_half(const D3D11_VIEWPORT& full, D3D11_VIEWPORT& vp) {
    const LONG l = static_cast<LONG>(full.TopLeftX) / 2;
    const LONG t = static_cast<LONG>(full.TopLeftY) / 2;
    const LONG r = (static_cast<LONG>(full.TopLeftX + full.Width) + 1) / 2;
    const LONG b = (static_cast<LONG>(full.TopLeftY + full.Height) + 1) / 2;
    vp = D3D11_VIEWPORT{};
    vp.TopLeftX = static_cast<FLOAT>(l);
    vp.TopLeftY = static_cast<FLOAT>(t);
    vp.Width = static_cast<FLOAT>(r - l);
    vp.Height = static_cast<FLOAT>(b - t);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
}
// The rectangle as a viewport, clamped to the target; false = nothing to draw. The clamp is ssao.hlsl's grid bound:
// KhSaInRectHalf is the passes' only on-grid test.
inline bool ssao_viewport(const SsaoPass& p, D3D11_VIEWPORT& vp) {
    const LONG l = p.rect.left < 0 ? 0 : p.rect.left;
    const LONG t = p.rect.top < 0 ? 0 : p.rect.top;
    const LONG r = p.rect.right > static_cast<LONG>(p.w) ? static_cast<LONG>(p.w) : p.rect.right;
    const LONG b = p.rect.bottom > static_cast<LONG>(p.h) ? static_cast<LONG>(p.h) : p.rect.bottom;
    if (r <= l || b <= t) return false;
    vp = D3D11_VIEWPORT{};
    vp.TopLeftX = static_cast<FLOAT>(l);
    vp.TopLeftY = static_cast<FLOAT>(t);
    vp.Width = static_cast<FLOAT>(r - l);
    vp.Height = static_cast<FLOAT>(b - t);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    return true;
}
// The fullscreen-triangle state the three draws share; everything it touches sits inside the caller's StateBackup.
inline void ssao_bind_fs(ID3D11DeviceContext* ctx, ID3D11PixelShader* ps, ID3D11BlendState* blend) {
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(blend, bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
}
// Before the first of a drawer's draws (the main depth bound as the DSV): the resources, and sp from the drawer's
// projection and viewport range. sp.rect is the caller's (snapshot_rect at the pv it draws with, padded; whole
// when that fails) and bounds the post passes.
inline void ssao_pre(ID3D11Device* dev, ID3D11DeviceContext* ctx, SsaoPass& sp,
                     const float proj[4][4], float vp_lo, float vp_hi) {
    KH_PROF_SCOPE(PROF_SSAO);   // PROF.
    sp.on = false;
    sp.term = ao_strength() > 0.0f;   // NEARZ_MARK: or the marker alone.
    sp.nzm = nzm_wanted();
    if ((!sp.term && !sp.nzm) || !dev || !ctx) return;
    UINT dw = 0, dh = 0;
    if (!ensure_ok("ssao depth", ensure_depth_srv(dev, ctx, &dw, &dh))) return;
    if (!g_res.depth_srv || !g_res.depth_sten_srv || dw < 2 || dh < 2) return;   // No stencil plane: no mark, no term.
    if (!ensure_ok("ssao shader", ensure_ssao_shaders(dev))) return;
    if (!ensure_ok("ssao targets", ensure_ssao_targets(dev, dw, dh))) return;
    if (!g_res.vs_fullscreen || !g_res.dss_off || !g_res.dss_mark_clear || !g_res.dss_test_write_mark ||
        !g_res.rasterizer || !g_res.dls_world_blend) return;
    sp.w = dw;
    sp.h = dh;
    sp.proj[0] = proj[0][0];
    sp.proj[1] = proj[1][1];
    sp.proj[2] = proj[2][2];
    sp.proj[3] = proj[3][2];
    sp.vp_lo = vp_lo;
    sp.vp_hi = vp_hi;
    if (fabsf(sp.proj[0]) < 1.0e-6f || fabsf(sp.proj[1]) < 1.0e-6f || !(sp.proj[3] < -1.0e-6f) ||
        !(vp_hi > vp_lo)) return;
    D3D11_VIEWPORT vp = {};
    if (!ssao_viewport(sp, vp)) return;
    sp.on = true;
}
inline void ssao_rect(const RVExtBridge::ProjectionViewTransform& pv, D3D11_RECT& out) {
    if (!snapshot_rect(pv, g_main_depth_w, g_main_depth_h, out)) {
        out = D3D11_RECT{ 0, 0, static_cast<LONG>(g_main_depth_w), static_cast<LONG>(g_main_depth_h) };
    } else if (out.right > out.left) {   // An empty rect stays empty.
        out.left -= SSAO_RECT_PAD;
        out.top -= SSAO_RECT_PAD;
        out.right += SSAO_RECT_PAD;
        out.bottom += SSAO_RECT_PAD;
    }
}
// After a drawer's last draw, scene target bound as RT0: the term over sp.rect, the apply into the scene at our
// samples, then the eraser. The eraser is the only thing that clears the marks, so it runs on every path once sp.on
// holds and the drawer's DSV is known, even when the term stands down; otherwise a mark would reach the next drawer's
// apply. Its own needs are the only early returns.
inline void ssao_post(ID3D11DeviceContext* ctx, const SsaoPass& sp) {
    KH_PROF_SCOPE(PROF_SSAO);
    KH_GPU_SCOPE(ctx, GPU_SSAO_POST);   // PROF.
    if (!sp.on || !ctx) return;
    D3D11_VIEWPORT vp = {};
    if (!ssao_viewport(sp, vp)) return;
    if (!g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_mark_clear) return;
    StateBackup bk;
    bk.capture(ctx);
    OmSave om;
    om.capture(ctx);
    ID3D11RenderTargetView* scene = om.rtv[0];
    D3D11_VIEWPORT hvp = {};
    ssao_viewport_half(vp, hvp);
    const bool term = sp.term && g_res.ssao_cb && g_res.ssao_ao_rtv && g_res.ssao_ao_srv && g_res.ssao_ao2_rtv &&
                      g_res.ssao_ao2_srv && g_res.ssao_dh_rtv && g_res.ssao_dh_srv && g_res.depth_srv &&
                      g_res.depth_sten_srv &&
                      g_res.ps_ssao_depth && g_res.ps_ssao_main && g_res.ps_ssao_blur && g_res.ps_ssao_apply;
    // The lanes, re-uploaded per draw with that draw's stride multiplier.
    SsaoCb cb = {};
    cb.proj[0] = sp.proj[0]; cb.proj[1] = sp.proj[1];
    cb.proj[2] = sp.proj[2]; cb.proj[3] = sp.proj[3];
    cb.vp[0] = sp.vp_lo;
    cb.vp[1] = sp.vp_hi;
    cb.vp[2] = static_cast<float>(sp.w);
    cb.vp[3] = static_cast<float>(sp.h);
    cb.ctl[0] = ao_dist();
    cb.ctl[1] = ao_strength();
    cb.rect[0] = vp.TopLeftX;
    cb.rect[1] = vp.TopLeftY;
    cb.rect[2] = vp.TopLeftX + vp.Width;
    cb.rect[3] = vp.TopLeftY + vp.Height;
    cb.half[0] = static_cast<float>(g_res.ssao_hw);
    cb.half[1] = static_cast<float>(g_res.ssao_hh);
    cb.half[2] = sp.nz_near;   // SSAO_NEARZ.
    cb.half[3] = sp.nz_gap;
    auto upload = [&](float stride) -> bool {
        cb.ctl[2] = stride;
        D3D11_MAPPED_SUBRESOURCE m = {};
        if (FAILED(ctx->Map(g_res.ssao_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return false;
        memcpy(m.pData, &cb, sizeof(cb));
        ctx->Unmap(g_res.ssao_cb, 0);
        return true;
    };
    bool sp_ok = term && upload(1.0f);
    if (sp_ok) {
    // The half grid's depth (unbinds the DSV; the live depth reads at t0, its stencil - the mark - at t1), then the
    // term on it, from t3.
    ctx->OMSetRenderTargets(1, &g_res.ssao_dh_rtv, nullptr);
    ctx->RSSetViewports(1, &hvp);
    ssao_bind_fs(ctx, g_res.ps_ssao_depth, nullptr);
    ctx->PSSetConstantBuffers(0, 1, &g_res.ssao_cb);
    ID3D11ShaderResourceView* srvs[4] = { g_res.depth_srv, g_res.depth_sten_srv, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 4, srvs);
    ctx->Draw(3, 0);
    ctx->OMSetRenderTargets(1, &g_res.ssao_ao_rtv, nullptr);
    ctx->RSSetViewports(1, &hvp);
    ctx->PSSetShader(g_res.ps_ssao_main, nullptr, 0);
    srvs[3] = g_res.ssao_dh_srv;
    ctx->PSSetShaderResources(3, 1, &srvs[3]);
    ctx->Draw(3, 0);
    // The blur levels below ping-pong ao -> ao2 -> ao; a target is never read and written by one draw.
    ctx->PSSetShader(g_res.ps_ssao_blur, nullptr, 0);
    ID3D11ShaderResourceView* none = nullptr;
    // SSAO_SEP: three levels of the separable blur, each an x pass (ao -> ao2) then a y pass (ao2 -> ao), so
    // the term is back in ao for the apply. ctl.z = the level's stride multiplier; ctl.w = the axis, + 2 at the
    // third level (its own stride rule).
    static const float lv[3][2] = { { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 3.0f, 2.0f } };
    for (int l = 0; l < 3 && sp_ok; ++l) {
        for (int a = 0; a < 2 && sp_ok; ++a) {
            cb.ctl[3] = lv[l][1] + static_cast<float>(a);
            if (!upload(lv[l][0])) { sp_ok = false; break; }
            ctx->PSSetShaderResources(2, 1, &none);   // This pass's target may be the last one's source.
            ctx->OMSetRenderTargets(1, a == 0 ? &g_res.ssao_ao2_rtv : &g_res.ssao_ao_rtv, nullptr);
            ctx->RSSetViewports(1, &hvp);
            srvs[2] = a == 0 ? g_res.ssao_ao_srv : g_res.ssao_ao2_srv;
            ctx->PSSetShaderResources(2, 1, &srvs[2]);
            ctx->Draw(3, 0);
        }
    }
    ctx->PSSetShaderResources(2, 1, &none);   // ao becomes the apply's source below.
    cb.ctl[3] = 0.0f;
    // The multiply over the read-only view of the drawer's depth (both planes read-only, so t0 / t1 stay bound) with
    // the mark test (stencil EQUAL 1, no writes): unmarked pixels skip the shader and only marked samples are
    // multiplied. Without the view or the state: the shader's sample-0 mark test alone.
    if (sp_ok && scene) {
        const bool mask = g_res.depth_dsv_ro && g_res.dss_mark_test;
        ctx->OMSetRenderTargets(1, &scene, mask ? g_res.depth_dsv_ro : nullptr);
        ctx->RSSetViewports(1, &vp);
        ctx->PSSetShader(g_res.ps_ssao_apply, nullptr, 0);
        if (mask) ctx->OMSetDepthStencilState(g_res.dss_mark_test, 1);
        const FLOAT bf[4] = { 0, 0, 0, 0 };
        ctx->OMSetBlendState(g_res.dls_world_blend, bf, 0xFFFFFFFF);   // dest.rgb *= src.rgb.
        srvs[2] = g_res.ssao_ao_srv;
        ctx->PSSetShaderResources(2, 1, &srvs[2]);
        ctx->Draw(3, 0);
    }
    ID3D11ShaderResourceView* null[4] = { nullptr, nullptr, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 4, null);   // Before the DSV comes back.
    }   // End of the term pass; the eraser below does not depend on it.
    // NEARZ_MARK: this drawer's near-z gap pixels into the marker while the mark stands. A cycle's first write
    // clears it; a later drawer adds its own pixels (the pass discards every other). Only for a reader (sp.nzm):
    // with no depth-reading fullscreen or local pass (nzm_wanted) the target is never made and the pass never draws.
    if (sp.nzm && sp.nz_near > 0.0f && g_res.ps_ssao_nzmark && g_res.depth_srv && g_res.depth_sten_srv &&
        g_res.ssao_cb) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        bool nm_ok = dev != nullptr;
        if (nm_ok && (!g_res.nzm_tex || !g_res.nzm_rtv || !g_res.nzm_srv ||
                      g_res.nzm_w != sp.w || g_res.nzm_h != sp.h)) {
            KH_SAFE_RELEASE(g_res.nzm_srv); KH_SAFE_RELEASE(g_res.nzm_rtv); KH_SAFE_RELEASE(g_res.nzm_tex);
            g_res.nzm_w = g_res.nzm_h = 0;
            D3D11_TEXTURE2D_DESC td = {};
            td.Width = sp.w; td.Height = sp.h; td.MipLevels = 1; td.ArraySize = 1;
            td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
            td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            td.Format = DXGI_FORMAT_R32G32_FLOAT;
            nm_ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.nzm_tex)) &&
                    SUCCEEDED(dev->CreateRenderTargetView(g_res.nzm_tex, nullptr, &g_res.nzm_rtv)) &&
                    SUCCEEDED(dev->CreateShaderResourceView(g_res.nzm_tex, nullptr, &g_res.nzm_srv));
            if (nm_ok) { g_res.nzm_w = sp.w; g_res.nzm_h = sp.h; }
            else { KH_SAFE_RELEASE(g_res.nzm_srv); KH_SAFE_RELEASE(g_res.nzm_rtv); KH_SAFE_RELEASE(g_res.nzm_tex); }
            g_nzm_cycle.store(~0ull, std::memory_order_relaxed);   // A new target holds nothing yet.
        }
        if (dev) dev->Release();
        if (nm_ok && upload(1.0f)) {
            if (g_nzm_cycle.load(std::memory_order_relaxed) != g_topo_cycles) {
                const FLOAT z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                ctx->ClearRenderTargetView(g_res.nzm_rtv, z);
                g_nzm_cycle.store(g_topo_cycles, std::memory_order_relaxed);
            }
            ctx->OMSetRenderTargets(1, &g_res.nzm_rtv, nullptr);   // The DSV off: its planes are read.
            ctx->RSSetViewports(1, &vp);
            ssao_bind_fs(ctx, g_res.ps_ssao_nzmark, nullptr);
            ctx->PSSetConstantBuffers(0, 1, &g_res.ssao_cb);
            ID3D11ShaderResourceView* srvs[2] = { g_res.depth_srv, g_res.depth_sten_srv };
            ctx->PSSetShaderResources(0, 2, srvs);
            ctx->Draw(3, 0);
            ID3D11ShaderResourceView* null[2] = { nullptr, nullptr };
            ctx->PSSetShaderResources(0, 2, null);   // Before the DSV comes back.
        }
    }
    // The eraser: the mark to 0 over the rectangle, so the next drawer's pass sees only what it painted. No target,
    // the drawer's own DSV (writable), no pixel shader - the stencil op alone. Unconditional on the term's outcome,
    // so it binds its whole state itself.
    if (om.dsv) {
        ctx->OMSetRenderTargets(0, nullptr, om.dsv);
        ctx->RSSetViewports(1, &vp);
        ssao_bind_fs(ctx, nullptr, nullptr);
        ctx->OMSetDepthStencilState(g_res.dss_mark_clear, 1);
        ctx->Draw(3, 0);
    }
    om.restore(ctx);
    om.release();
    bk.restore(ctx);
}
}