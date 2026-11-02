#pragma once

// The feature half (rendering_integration_features.hpp): KH_SSAO and the near-z marker. Never include it on its own.
namespace RenderIntegration {

// KH_SSAO - screen-space ambient occlusion for our meshes (ssao.hlsl; the settings are setRenderAmbientOcclusion's).
// After a drawer's depth-writing draws (the injection's, or the flush's late ones) the term is computed from the live
// main depth, which then holds the world's opaques and ours, smoothed by three separable blur levels (KH_SSAO_SEP), and
// multiplied into the scene colour at OUR samples (kh_ssao_post), over the meshes' pixel rectangle (kh_snapshot_rect,
// padded).
//
// WHICH PIXELS ARE OURS is the stencil's answer (KH_SSAO_MARK). The main depth is D24S8 and the engine's world draws
// never write its stencil (measured: the hands' Replace 0 there is a no-op). Every depth-writing draw of ours binds
// dss_test_write_mark, ref 1, so a pixel it wins carries a 1 (3 for a see-through blend mode, which the term skips:
// KH_SSAO_SEE_THROUGH); the AO pass applies where the mark is and then ERASES it (dss_mark_clear), so each pixel takes
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
// Replaced the distance-field AO (a KH_SDF_N^3 field per mesh in the mesh cache, a Texture3D atlas, occluder records at
// t34 / t40 / t41, a six-cone trace); three CBFrame lanes and the cache chunk went with it (the mesh cache version was
// bumped).
struct KhSsaoCb {   // HLSL twin CBSsao (ssao.hlsl, b0), 5 float4.
    float proj[4];   // x = m00, y = m11, z = m22, w = m32 (the drawer's projection: the injection's or the flush's).
    float vp[4];     // x = viewport MinDepth, y = MaxDepth, z = target width, w = target height.
    float ctl[4];    // x = radius (m), y = strength (the exponent), z = the draw's blur stride
                     // multiplier (1, 2, 3 by level); w = the blur's axis and rule (KH_SSAO_SEP: 0 / 1 = x / y,
                     // + 2 = the third level's stride rule). Only the blurs read z / w: the gather's and the
                     // near marker's uploads carry z = 1, w = 0; the apply draws under the last blur's.
    float rect[4];   // The pass rectangle, left / top / right / bottom (px, exclusive): the
                     // blurs read no term texel outside it (nothing wrote there this frame).
    float half[4];   // x, y = the half-resolution grid's width and height; z = the near-z route's near (0 = no route
                     // this pass), w = its gap floor (KH_SSAO_NEARZ).
};
static_assert(sizeof(KhSsaoCb) == 80, "KhSsaoCb is 5 float4 (HLSL twin CBSsao)");
// The pass rectangle is the meshes' (kh_snapshot_rect, its KH_SNAP_RECT_MARGIN included) padded by the passes' reach
// beyond a marked pixel. One reach binds the pad: PSSsaoMain's wide normal, four half-grid depth taps at a stride of at
// most 24 half pixels = 48 full, which take no rectangle test. The blurs' and the apply's taps (three separable levels
// at strides of at most 3, 6 and 4 half pixels, KH_SSAO_SEP: 52 full pixels, and the upsample's half texel past them,
// 54) are rectangle-tested and read only texels of ours (KH_SSAO_OURS), which lie inside the unpadded rectangle, so
// they never reach into the pad. A marked pixel lies inside the unpadded rectangle (the mark is a depth write of a mesh
// whose box the rectangle bounds), so the wide normal stays on half-grid texels PSSsaoDepth wrote this frame; the
// whole-target fallback writes every texel. The gather's samples (up to 256 half pixels out) are rejected outside the
// rectangle, so its search ends 16 + 72 = 88 full pixels past the meshes' box: where the radius spans more on screen (a
// mesh up close), the occluders beyond go unsampled and the contact term is weaker than the radius says.
static constexpr LONG KH_SSAO_RECT_PAD = 72;
inline float kh_ao_strength() {
    const uint32_t khas_b = g_ao_strength_bits.load(std::memory_order_relaxed);
    float khas_f = 0.0f;
    memcpy(&khas_f, &khas_b, sizeof(khas_f));
    return khas_f;
}
inline float kh_ao_dist() {
    const uint32_t khad_b = g_ao_dist_bits.load(std::memory_order_relaxed);
    float khad_f = 0.5f;
    memcpy(&khad_f, &khad_b, sizeof(khad_f));
    return khad_f;
}
// KH_NEARZ_MARK - the effect chain runs late (after the view model: KH_FX_LATE), when the SSAO mark is erased; the
// engine's hands would hold the same depths as our near-z gap if their slice reached it (measured, it ends at 0.01,
// below the gap's floor 0.92 x 0.011 = 0.01012), so the depth alone is not trusted to say a gap pixel is ours. Each
// drawer's kh_ssao_post, while its mark still stands, writes (raw, true distance) at its near-z gap pixels into
// this marker (PSSsaoNearMark, KhSaMetersAt's test and decode); the chain arms it (shadowMeta2.x, t37) and
// cb.hlsl's LoadDepthPS takes the true distance wherever the marker's raw is still the live raw - a pixel
// drawn over since (the hands) keeps its own. Valid for the cycle that wrote it (g_nzm_cycle).
static uint64_t g_nzm_cycle = ~0ull;   // The cycle the marker was cleared and written in (~0 = none).
inline bool kh_nzm_valid() { return g_nzm_cycle == g_topo_cycles && g_res.nzm_srv != nullptr; }
// The marker may have a reader this cycle: a visible fullscreen or local pass the scene chain runs, which arms it
// (the flush's rule: every fullscreen pass but a UI-mode one - ui_only - which the UI chain runs with no marker
// armed), and that reads the depth (KH_NEARZ_READER: a pass that never calls LoadDepthPS never reads the marker). An
// effect mesh is not one - no effect mesh is armed (KH_NEARZ_MARK's limit). Without a reader the marker is not
// written; with SSAO off as well the drawers do not mark (kh_ssao_pre returns on neither term nor marker) and the
// SSAO pass does not run at all.
inline bool kh_nzm_wanted() {
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (o.visible && (o.fullscreen || o.localized) && !o.ui_only && kh_fx_needs_depth(o)) return true;
    }
    return false;
}
// The five pixel shaders, compiled at the live depth's sample count (the depth-resolve recipe). The define table is
// byte for byte the speculation batch's.
inline std::string ensure_ssao_shaders(ID3D11Device* dev) {
    const UINT khss_sc = g_res.depth_sample_count > 0 ? g_res.depth_sample_count : 1;
    if (g_res.ps_ssao_depth && g_res.ps_ssao_main && g_res.ps_ssao_blur && g_res.ps_ssao_apply &&
        g_res.ps_ssao_nzmark && g_res.ps_ssao_samples == khss_sc) return "";   // KH_NEARZ_MARK: five.
    KH_SAFE_RELEASE(g_res.ps_ssao_depth);
    KH_SAFE_RELEASE(g_res.ps_ssao_main);
    KH_SAFE_RELEASE(g_res.ps_ssao_blur);
    KH_SAFE_RELEASE(g_res.ps_ssao_apply);
    KH_SAFE_RELEASE(g_res.ps_ssao_nzmark);   // KH_NEARZ_MARK.
    const std::string khss_scs = std::to_string(khss_sc);
    const D3D_SHADER_MACRO khss_defs[] = {
        { "MSAA_DEPTH", khss_sc > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", khss_scs.c_str() },
        { nullptr, nullptr },
    };
    const char* khss_src = kh_hlsl_src(KH_HLSL_SSAO).c_str();
    struct KhSsaoEntry { const char* entry; ID3D11PixelShader** out; };
    const KhSsaoEntry khss_e[5] = {
        { "PSSsaoDepth", &g_res.ps_ssao_depth },
        { "PSSsaoMain",  &g_res.ps_ssao_main },
        { "PSSsaoBlur",  &g_res.ps_ssao_blur },
        { "PSSsaoApply", &g_res.ps_ssao_apply },
        { "PSSsaoNearMark", &g_res.ps_ssao_nzmark },   // KH_NEARZ_MARK.
    };
    for (const KhSsaoEntry& khss_x : khss_e) {
        ID3DBlob* khss_b = nullptr;
        const std::string khss_err = compile_shader(khss_src, khss_x.entry, "ps_5_0", khss_defs, &khss_b);
        if (!khss_err.empty() || !khss_b) {
            KH_SAFE_RELEASE(g_res.ps_ssao_depth);
            KH_SAFE_RELEASE(g_res.ps_ssao_main);
            KH_SAFE_RELEASE(g_res.ps_ssao_blur);
            KH_SAFE_RELEASE(g_res.ps_ssao_apply);
            KH_SAFE_RELEASE(g_res.ps_ssao_nzmark);
            return std::string(khss_x.entry) + ": " + khss_err;
        }
        const HRESULT khss_hr = dev->CreatePixelShader(khss_b->GetBufferPointer(), khss_b->GetBufferSize(), nullptr, khss_x.out);
        khss_b->Release();
        if (FAILED(khss_hr)) {
            KH_SAFE_RELEASE(g_res.ps_ssao_depth);
            KH_SAFE_RELEASE(g_res.ps_ssao_main);
            KH_SAFE_RELEASE(g_res.ps_ssao_blur);
            KH_SAFE_RELEASE(g_res.ps_ssao_apply);
            KH_SAFE_RELEASE(g_res.ps_ssao_nzmark);
            return std::string("create ") + khss_x.entry + " " + hr_str(khss_hr);
        }
    }
    g_res.ps_ssao_samples = khss_sc;
    return "";
}
inline void kh_ssao_targets_release() {
    KH_SAFE_RELEASE(g_res.ssao_dh_srv);
    KH_SAFE_RELEASE(g_res.ssao_dh_rtv);
    KH_SAFE_RELEASE(g_res.ssao_dh_tex);
    KH_SAFE_RELEASE(g_res.ssao_ao_srv);
    KH_SAFE_RELEASE(g_res.ssao_ao_rtv);
    KH_SAFE_RELEASE(g_res.ssao_ao_tex);
    KH_SAFE_RELEASE(g_res.ssao_ao2_srv);
    KH_SAFE_RELEASE(g_res.ssao_ao2_rtv);
    KH_SAFE_RELEASE(g_res.ssao_ao2_tex);
    KH_SAFE_RELEASE(g_res.nzm_srv);   // KH_NEARZ_MARK: follows the main depth's size too.
    KH_SAFE_RELEASE(g_res.nzm_rtv);
    KH_SAFE_RELEASE(g_res.nzm_tex);
    g_res.nzm_w = g_res.nzm_h = 0;
    g_res.ssao_w = 0;
    g_res.ssao_h = 0;
    g_res.ssao_hw = 0;
    g_res.ssao_hh = 0;
}
// The half grid (the main depth's size rounded up to even, halved): its sample-0 depth in metres (R32_FLOAT), the
// term and its ping-pong twin (R8_UNORM); and the 80-byte b0.
inline bool ensure_ssao_targets(ID3D11Device* dev, UINT khst_w, UINT khst_h) {
    if (!g_res.ssao_cb) {
        D3D11_BUFFER_DESC khst_cd = {};
        khst_cd.ByteWidth = sizeof(KhSsaoCb);
        khst_cd.Usage = D3D11_USAGE_DYNAMIC;
        khst_cd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        khst_cd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(dev->CreateBuffer(&khst_cd, nullptr, &g_res.ssao_cb))) { g_res.ssao_cb = nullptr; return false; }
    }
    if (g_res.ssao_dh_tex && g_res.ssao_dh_rtv && g_res.ssao_dh_srv &&
        g_res.ssao_ao_tex && g_res.ssao_ao_rtv && g_res.ssao_ao_srv &&
        g_res.ssao_ao2_tex && g_res.ssao_ao2_rtv && g_res.ssao_ao2_srv &&
        g_res.ssao_w == khst_w && g_res.ssao_h == khst_h) return true;
    kh_ssao_targets_release();
    const UINT khst_hw = (khst_w + 1u) / 2u, khst_hh = (khst_h + 1u) / 2u;
    D3D11_TEXTURE2D_DESC khst_td = {};
    khst_td.Width = khst_hw;
    khst_td.Height = khst_hh;
    khst_td.MipLevels = 1;
    khst_td.ArraySize = 1;
    khst_td.SampleDesc.Count = 1;
    khst_td.Usage = D3D11_USAGE_DEFAULT;
    khst_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    khst_td.Format = DXGI_FORMAT_R32_FLOAT;
    if (FAILED(dev->CreateTexture2D(&khst_td, nullptr, &g_res.ssao_dh_tex)) ||
        FAILED(dev->CreateRenderTargetView(g_res.ssao_dh_tex, nullptr, &g_res.ssao_dh_rtv)) ||
        FAILED(dev->CreateShaderResourceView(g_res.ssao_dh_tex, nullptr, &g_res.ssao_dh_srv))) {
        kh_ssao_targets_release();
        return false;
    }
    khst_td.Format = DXGI_FORMAT_R8_UNORM;
    if (FAILED(dev->CreateTexture2D(&khst_td, nullptr, &g_res.ssao_ao_tex)) ||
        FAILED(dev->CreateRenderTargetView(g_res.ssao_ao_tex, nullptr, &g_res.ssao_ao_rtv)) ||
        FAILED(dev->CreateShaderResourceView(g_res.ssao_ao_tex, nullptr, &g_res.ssao_ao_srv))) {
        kh_ssao_targets_release();
        return false;
    }
    if (FAILED(dev->CreateTexture2D(&khst_td, nullptr, &g_res.ssao_ao2_tex)) ||
        FAILED(dev->CreateRenderTargetView(g_res.ssao_ao2_tex, nullptr, &g_res.ssao_ao2_rtv)) ||
        FAILED(dev->CreateShaderResourceView(g_res.ssao_ao2_tex, nullptr, &g_res.ssao_ao2_srv))) {
        kh_ssao_targets_release();
        return false;
    }
    g_res.ssao_w = khst_w;
    g_res.ssao_h = khst_h;
    g_res.ssao_hw = khst_hw;
    g_res.ssao_hh = khst_hh;
    return true;
}
// The rectangle's half-grid viewport: edges rounded outward (the shader's KhSaInRectHalf takes the same edges), so a
// non-empty rectangle gives at least one texel each way.
inline void kh_ssao_viewport_half(const D3D11_VIEWPORT& khsh_full, D3D11_VIEWPORT& khsh_vp) {
    const LONG khsh_l = static_cast<LONG>(khsh_full.TopLeftX) / 2;
    const LONG khsh_t = static_cast<LONG>(khsh_full.TopLeftY) / 2;
    const LONG khsh_r = (static_cast<LONG>(khsh_full.TopLeftX + khsh_full.Width) + 1) / 2;
    const LONG khsh_b = (static_cast<LONG>(khsh_full.TopLeftY + khsh_full.Height) + 1) / 2;
    khsh_vp = D3D11_VIEWPORT{};
    khsh_vp.TopLeftX = static_cast<FLOAT>(khsh_l);
    khsh_vp.TopLeftY = static_cast<FLOAT>(khsh_t);
    khsh_vp.Width = static_cast<FLOAT>(khsh_r - khsh_l);
    khsh_vp.Height = static_cast<FLOAT>(khsh_b - khsh_t);
    khsh_vp.MinDepth = 0.0f;
    khsh_vp.MaxDepth = 1.0f;
}
// The rectangle as a viewport, clamped to the target; false = nothing to draw. The clamp is ssao.hlsl's grid bound:
// KhSaInRectHalf is the passes' only on-grid test.
inline bool kh_ssao_viewport(const KhSsaoPass& khsv_p, D3D11_VIEWPORT& khsv_vp) {
    const LONG khsv_l = khsv_p.rect.left < 0 ? 0 : khsv_p.rect.left;
    const LONG khsv_t = khsv_p.rect.top < 0 ? 0 : khsv_p.rect.top;
    const LONG khsv_r = khsv_p.rect.right > static_cast<LONG>(khsv_p.w) ? static_cast<LONG>(khsv_p.w) : khsv_p.rect.right;
    const LONG khsv_b = khsv_p.rect.bottom > static_cast<LONG>(khsv_p.h) ? static_cast<LONG>(khsv_p.h) : khsv_p.rect.bottom;
    if (khsv_r <= khsv_l || khsv_b <= khsv_t) return false;
    khsv_vp = D3D11_VIEWPORT{};
    khsv_vp.TopLeftX = static_cast<FLOAT>(khsv_l);
    khsv_vp.TopLeftY = static_cast<FLOAT>(khsv_t);
    khsv_vp.Width = static_cast<FLOAT>(khsv_r - khsv_l);
    khsv_vp.Height = static_cast<FLOAT>(khsv_b - khsv_t);
    khsv_vp.MinDepth = 0.0f;
    khsv_vp.MaxDepth = 1.0f;
    return true;
}
// The fullscreen-triangle state the three draws share; everything it touches sits inside the caller's StateBackup.
inline void kh_ssao_bind_fs(ID3D11DeviceContext* ctx, ID3D11PixelShader* khsb_ps, ID3D11BlendState* khsb_blend) {
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(khsb_ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT khsb_bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(khsb_blend, khsb_bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
}
// Before the first of a drawer's draws (the main depth bound as the DSV): the resources, and khsp from the drawer's
// projection and viewport range. khsp.rect is the caller's (kh_snapshot_rect at the pv it draws with, padded; whole
// when that fails) and bounds the post passes.
inline void kh_ssao_pre(ID3D11Device* dev, ID3D11DeviceContext* ctx, KhSsaoPass& khsp,
                        const float khsp_proj[4][4], float khsp_vp_lo, float khsp_vp_hi) {
    KH_PROF_SCOPE(KHP_SSAO);   // KH_PROF.
    khsp.on = false;
    khsp.term = kh_ao_strength() > 0.0f;   // KH_NEARZ_MARK: or the marker alone.
    khsp.nzm = kh_nzm_wanted();
    if ((!khsp.term && !khsp.nzm) || !dev || !ctx) return;
    UINT khsp_dw = 0, khsp_dh = 0;
    if (!kh_ensure_ok("ssao depth", ensure_depth_srv(dev, ctx, &khsp_dw, &khsp_dh))) return;
    if (!g_res.depth_srv || !g_res.depth_sten_srv || khsp_dw < 2 || khsp_dh < 2) return;   // No stencil plane: no mark, no term.
    if (!kh_ensure_ok("ssao shader", ensure_ssao_shaders(dev))) return;
    if (!ensure_ssao_targets(dev, khsp_dw, khsp_dh)) return;
    if (!g_res.vs_fullscreen || !g_res.dss_off || !g_res.dss_mark_clear || !g_res.dss_test_write_mark ||
        !g_res.rasterizer || !g_res.dls_world_blend) return;
    khsp.w = khsp_dw;
    khsp.h = khsp_dh;
    khsp.proj[0] = khsp_proj[0][0];
    khsp.proj[1] = khsp_proj[1][1];
    khsp.proj[2] = khsp_proj[2][2];
    khsp.proj[3] = khsp_proj[3][2];
    khsp.vp_lo = khsp_vp_lo;
    khsp.vp_hi = khsp_vp_hi;
    if (fabsf(khsp.proj[0]) < 1.0e-6f || fabsf(khsp.proj[1]) < 1.0e-6f || !(khsp.proj[3] < -1.0e-6f) ||
        !(khsp_vp_hi > khsp_vp_lo)) return;
    D3D11_VIEWPORT khsp_vp = {};
    if (!kh_ssao_viewport(khsp, khsp_vp)) return;
    khsp.on = true;
}
inline void kh_ssao_rect(const RVExtBridge::ProjectionViewTransform& khsr_pv, D3D11_RECT& khsr_out) {
    if (!kh_snapshot_rect(khsr_pv, g_main_depth_w, g_main_depth_h, khsr_out)) {
        khsr_out = D3D11_RECT{ 0, 0, static_cast<LONG>(g_main_depth_w), static_cast<LONG>(g_main_depth_h) };
    } else if (khsr_out.right > khsr_out.left) {   // An empty rect stays empty.
        khsr_out.left -= KH_SSAO_RECT_PAD;
        khsr_out.top -= KH_SSAO_RECT_PAD;
        khsr_out.right += KH_SSAO_RECT_PAD;
        khsr_out.bottom += KH_SSAO_RECT_PAD;
    }
}
// After a drawer's last draw, scene target bound as RT0: the term over khsp.rect, the apply into the scene at our
// samples, then the eraser. The eraser is the only thing that clears the marks, so it runs on every path once khsp.on
// holds and the drawer's DSV is known, even when the term stands down; otherwise a mark would reach the next drawer's
// apply. Its own needs are the only early returns.
inline void kh_ssao_post(ID3D11DeviceContext* ctx, const KhSsaoPass& khsp) {
    KH_PROF_SCOPE(KHP_SSAO);
    KH_GPU_SCOPE(ctx, KHG_SSAO_POST);   // KH_PROF.
    if (!khsp.on || !ctx) return;
    D3D11_VIEWPORT khsp_vp = {};
    if (!kh_ssao_viewport(khsp, khsp_vp)) return;
    if (!g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_mark_clear) return;
    StateBackup khsp_bk;
    khsp_bk.capture(ctx);
    KhOmSave khsp_om;
    khsp_om.capture(ctx);
    ID3D11RenderTargetView* khsp_scene = khsp_om.rtv[0];
    D3D11_VIEWPORT khsp_hvp = {};
    kh_ssao_viewport_half(khsp_vp, khsp_hvp);
    const bool khsp_term = khsp.term && g_res.ssao_cb && g_res.ssao_ao_rtv && g_res.ssao_ao_srv && g_res.ssao_ao2_rtv &&
                           g_res.ssao_ao2_srv && g_res.ssao_dh_rtv && g_res.ssao_dh_srv && g_res.depth_srv &&
                           g_res.depth_sten_srv &&
                           g_res.ps_ssao_depth && g_res.ps_ssao_main && g_res.ps_ssao_blur && g_res.ps_ssao_apply;
    // The lanes, re-uploaded per draw with that draw's stride multiplier.
    KhSsaoCb khsp_cb = {};
    khsp_cb.proj[0] = khsp.proj[0]; khsp_cb.proj[1] = khsp.proj[1];
    khsp_cb.proj[2] = khsp.proj[2]; khsp_cb.proj[3] = khsp.proj[3];
    khsp_cb.vp[0] = khsp.vp_lo;
    khsp_cb.vp[1] = khsp.vp_hi;
    khsp_cb.vp[2] = static_cast<float>(khsp.w);
    khsp_cb.vp[3] = static_cast<float>(khsp.h);
    khsp_cb.ctl[0] = kh_ao_dist();
    khsp_cb.ctl[1] = kh_ao_strength();
    khsp_cb.rect[0] = khsp_vp.TopLeftX;
    khsp_cb.rect[1] = khsp_vp.TopLeftY;
    khsp_cb.rect[2] = khsp_vp.TopLeftX + khsp_vp.Width;
    khsp_cb.rect[3] = khsp_vp.TopLeftY + khsp_vp.Height;
    khsp_cb.half[0] = static_cast<float>(g_res.ssao_hw);
    khsp_cb.half[1] = static_cast<float>(g_res.ssao_hh);
    khsp_cb.half[2] = khsp.nz_near;   // KH_SSAO_NEARZ.
    khsp_cb.half[3] = khsp.nz_gap;
    auto khsp_upload = [&](float khsu_stride) -> bool {
        khsp_cb.ctl[2] = khsu_stride;
        D3D11_MAPPED_SUBRESOURCE khsu_m = {};
        if (FAILED(ctx->Map(g_res.ssao_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khsu_m))) return false;
        memcpy(khsu_m.pData, &khsp_cb, sizeof(khsp_cb));
        ctx->Unmap(g_res.ssao_cb, 0);
        return true;
    };
    bool khsp_ok = khsp_term && khsp_upload(1.0f);
    if (khsp_ok) {
    // The half grid's depth (unbinds the DSV; the live depth reads at t0, its stencil - the mark - at t1), then the
    // term on it, from t3.
    ctx->OMSetRenderTargets(1, &g_res.ssao_dh_rtv, nullptr);
    ctx->RSSetViewports(1, &khsp_hvp);
    kh_ssao_bind_fs(ctx, g_res.ps_ssao_depth, nullptr);
    ctx->PSSetConstantBuffers(0, 1, &g_res.ssao_cb);
    ID3D11ShaderResourceView* khsp_srvs[4] = { g_res.depth_srv, g_res.depth_sten_srv, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 4, khsp_srvs);
    ctx->Draw(3, 0);
    ctx->OMSetRenderTargets(1, &g_res.ssao_ao_rtv, nullptr);
    ctx->RSSetViewports(1, &khsp_hvp);
    ctx->PSSetShader(g_res.ps_ssao_main, nullptr, 0);
    khsp_srvs[3] = g_res.ssao_dh_srv;
    ctx->PSSetShaderResources(3, 1, &khsp_srvs[3]);
    ctx->Draw(3, 0);
    // The blur levels below ping-pong ao -> ao2 -> ao; a target is never read and written by one draw.
    ctx->PSSetShader(g_res.ps_ssao_blur, nullptr, 0);
    ID3D11ShaderResourceView* khsp_none = nullptr;
    // KH_SSAO_SEP: three levels of the separable blur, each an x pass (ao -> ao2) then a y pass (ao2 -> ao), so
    // the term is back in ao for the apply. ctl.z = the level's stride multiplier; ctl.w = the axis, + 2 at the
    // third level (its own stride rule).
    static const float khsp_lv[3][2] = { { 1.0f, 0.0f }, { 2.0f, 0.0f }, { 3.0f, 2.0f } };
    for (int khsp_l = 0; khsp_l < 3 && khsp_ok; ++khsp_l) {
        for (int khsp_a = 0; khsp_a < 2 && khsp_ok; ++khsp_a) {
            khsp_cb.ctl[3] = khsp_lv[khsp_l][1] + static_cast<float>(khsp_a);
            if (!khsp_upload(khsp_lv[khsp_l][0])) { khsp_ok = false; break; }
            ctx->PSSetShaderResources(2, 1, &khsp_none);   // This pass's target may be the last one's source.
            ctx->OMSetRenderTargets(1, khsp_a == 0 ? &g_res.ssao_ao2_rtv : &g_res.ssao_ao_rtv, nullptr);
            ctx->RSSetViewports(1, &khsp_hvp);
            khsp_srvs[2] = khsp_a == 0 ? g_res.ssao_ao_srv : g_res.ssao_ao2_srv;
            ctx->PSSetShaderResources(2, 1, &khsp_srvs[2]);
            ctx->Draw(3, 0);
        }
    }
    ctx->PSSetShaderResources(2, 1, &khsp_none);   // ao becomes the apply's source below.
    khsp_cb.ctl[3] = 0.0f;
    // The multiply over the read-only view of the drawer's depth (both planes read-only, so t0 / t1 stay bound) with
    // the mark test (stencil EQUAL 1, no writes): unmarked pixels skip the shader and only marked samples are
    // multiplied. Without the view or the state: the shader's sample-0 mark test alone.
    if (khsp_ok && khsp_scene) {
        const bool khsp_mask = g_res.depth_dsv_ro && g_res.dss_mark_test;
        ctx->OMSetRenderTargets(1, &khsp_scene, khsp_mask ? g_res.depth_dsv_ro : nullptr);
        ctx->RSSetViewports(1, &khsp_vp);
        ctx->PSSetShader(g_res.ps_ssao_apply, nullptr, 0);
        if (khsp_mask) ctx->OMSetDepthStencilState(g_res.dss_mark_test, 1);
        const FLOAT khsp_bf[4] = { 0, 0, 0, 0 };
        ctx->OMSetBlendState(g_res.dls_world_blend, khsp_bf, 0xFFFFFFFF);   // dest.rgb *= src.rgb.
        khsp_srvs[2] = g_res.ssao_ao_srv;
        ctx->PSSetShaderResources(2, 1, &khsp_srvs[2]);
        ctx->Draw(3, 0);
    }
    ID3D11ShaderResourceView* khsp_null[4] = { nullptr, nullptr, nullptr, nullptr };
    ctx->PSSetShaderResources(0, 4, khsp_null);   // Before the DSV comes back.
    }   // End of the term pass; the eraser below does not depend on it.
    // KH_NEARZ_MARK: this drawer's near-z gap pixels into the marker while the mark stands. A cycle's first write
    // clears it; a later drawer adds its own pixels (the pass discards every other). Only for a reader (khsp.nzm):
    // with no depth-reading fullscreen or local pass (kh_nzm_wanted) the target is never made and the pass never draws.
    if (khsp.nzm && khsp.nz_near > 0.0f && g_res.ps_ssao_nzmark && g_res.depth_srv && g_res.depth_sten_srv &&
        g_res.ssao_cb) {
        ID3D11Device* khnm_dev = nullptr;
        ctx->GetDevice(&khnm_dev);
        bool khnm_ok = khnm_dev != nullptr;
        if (khnm_ok && (!g_res.nzm_tex || !g_res.nzm_rtv || !g_res.nzm_srv ||
                        g_res.nzm_w != khsp.w || g_res.nzm_h != khsp.h)) {
            KH_SAFE_RELEASE(g_res.nzm_srv); KH_SAFE_RELEASE(g_res.nzm_rtv); KH_SAFE_RELEASE(g_res.nzm_tex);
            g_res.nzm_w = g_res.nzm_h = 0;
            D3D11_TEXTURE2D_DESC khnm_td = {};
            khnm_td.Width = khsp.w; khnm_td.Height = khsp.h; khnm_td.MipLevels = 1; khnm_td.ArraySize = 1;
            khnm_td.SampleDesc.Count = 1; khnm_td.Usage = D3D11_USAGE_DEFAULT;
            khnm_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            khnm_td.Format = DXGI_FORMAT_R32G32_FLOAT;
            khnm_ok = SUCCEEDED(khnm_dev->CreateTexture2D(&khnm_td, nullptr, &g_res.nzm_tex)) &&
                      SUCCEEDED(khnm_dev->CreateRenderTargetView(g_res.nzm_tex, nullptr, &g_res.nzm_rtv)) &&
                      SUCCEEDED(khnm_dev->CreateShaderResourceView(g_res.nzm_tex, nullptr, &g_res.nzm_srv));
            if (khnm_ok) { g_res.nzm_w = khsp.w; g_res.nzm_h = khsp.h; }
            else { KH_SAFE_RELEASE(g_res.nzm_srv); KH_SAFE_RELEASE(g_res.nzm_rtv); KH_SAFE_RELEASE(g_res.nzm_tex); }
            g_nzm_cycle = ~0ull;   // A new target holds nothing yet.
        }
        if (khnm_dev) khnm_dev->Release();
        if (khnm_ok && khsp_upload(1.0f)) {
            if (g_nzm_cycle != g_topo_cycles) {
                const FLOAT khnm_z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                ctx->ClearRenderTargetView(g_res.nzm_rtv, khnm_z);
                g_nzm_cycle = g_topo_cycles;
            }
            ctx->OMSetRenderTargets(1, &g_res.nzm_rtv, nullptr);   // The DSV off: its planes are read.
            ctx->RSSetViewports(1, &khsp_vp);
            kh_ssao_bind_fs(ctx, g_res.ps_ssao_nzmark, nullptr);
            ctx->PSSetConstantBuffers(0, 1, &g_res.ssao_cb);
            ID3D11ShaderResourceView* khnm_srvs[2] = { g_res.depth_srv, g_res.depth_sten_srv };
            ctx->PSSetShaderResources(0, 2, khnm_srvs);
            ctx->Draw(3, 0);
            ID3D11ShaderResourceView* khnm_null[2] = { nullptr, nullptr };
            ctx->PSSetShaderResources(0, 2, khnm_null);   // Before the DSV comes back.
        }
    }
    // The eraser: the mark to 0 over the rectangle, so the next drawer's pass sees only what it painted. No target,
    // the drawer's own DSV (writable), no pixel shader - the stencil op alone. Unconditional on the term's outcome,
    // so it binds its whole state itself.
    if (khsp_om.dsv) {
        ctx->OMSetRenderTargets(0, nullptr, khsp_om.dsv);
        ctx->RSSetViewports(1, &khsp_vp);
        kh_ssao_bind_fs(ctx, nullptr, nullptr);
        ctx->OMSetDepthStencilState(g_res.dss_mark_clear, 1);
        ctx->Draw(3, 0);
    }
    khsp_om.restore(ctx);
    khsp_om.release();
    khsp_bk.restore(ctx);
}
}