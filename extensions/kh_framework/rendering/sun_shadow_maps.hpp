#pragma once

// The feature half (rendering_integration_features.hpp): our sun shadow maps' setup: the ladder's sizes and targets,
// the anchors and tiers, the prefilter and the sun lanes. Never include it on its own.
namespace RenderIntegration {
// Defined with the DLS map state it reads.
inline void kh_dls_fill_cb(ConstantData& khf_cb);

inline bool kh_sun_map_ensure(ID3D11Device* dev, UINT khsm_size,
                              ID3D11Texture2D*& khsm_tex,
                              ID3D11DepthStencilView*& khsm_dsv,
                              ID3D11ShaderResourceView*& khsm_srv);   // Defined with the bands.
inline void kh_sun_pf_forget(int khpf_t);   // KH_PF_WHOLE: defined with the tier flags (below).

static constexpr UINT KH_SUN_DEPTH_BASE = 4096;   // The union map (KH_SUN_LADDER).
// Bounding the fit bounds the texel size, the locality reach and the normal-offset at once. Casters beyond this
// camera radius stay out of the sun-depth fit (no self-shadow, no cast), like the engine's own object shadows. The
// depth window extends 12 m sun-ward so out-of-window occluders still cast in. Do not lower the ladder without
// re-pricing the offset floor.
static constexpr UINT  KH_SUN_HERO_BASE = 4096;
static constexpr float KH_SUN_HERO_HALF = 2.0f;
static constexpr float KH_SUN_HERO_DEPTH = 12.0f;
static constexpr UINT  KH_SUN_MID_BASE  = 4096;
static constexpr float KH_SUN_MID_HALF  = 8.0f;
static constexpr float KH_SUN_MID_DEPTH = 48.0f;
static constexpr UINT  KH_SUN_OUT_BASE  = 4096;
static constexpr float KH_SUN_OUT_HALF  = 32.0f;
static constexpr float KH_SUN_OUT_DEPTH = 192.0f;
// The union remains behind it for the beyond-far presence test and anything outside the range window.
static constexpr UINT KH_SUN_FAR_BASE = 4096;   // 12 cm texels over the 500 m fit; not on the ladder.
// Sizes are runtime (g_sun_size_div), the far band's excepted: kh_sun_far_size is fixed (not on the ladder).
static std::atomic<uint32_t> g_sun_size_div{1};
inline UINT kh_sun_depth_size() { return KH_SUN_DEPTH_BASE / g_sun_size_div.load(std::memory_order_relaxed); }
inline UINT kh_sun_hero_size()  { return KH_SUN_HERO_BASE  / g_sun_size_div.load(std::memory_order_relaxed); }
inline UINT kh_sun_mid_size()   { return KH_SUN_MID_BASE   / g_sun_size_div.load(std::memory_order_relaxed); }
inline UINT kh_sun_out_size()   { return KH_SUN_OUT_BASE   / g_sun_size_div.load(std::memory_order_relaxed); }
inline UINT kh_sun_far_size()   { return KH_SUN_FAR_BASE; }
inline void kh_sun_size_latch() {
    g_sun_size_div.store(1u,
                         std::memory_order_relaxed);
}
// The size each map was last rendered at - what sunMeta.y must carry.
static UINT g_sun_map_size  = KH_SUN_DEPTH_BASE;
static UINT g_sun2_map_size = KH_SUN_HERO_BASE;
static UINT g_sun3_map_size = KH_SUN_MID_BASE;
static UINT g_sun4_map_size = KH_SUN_OUT_BASE;
static UINT g_sun5_map_size = KH_SUN_FAR_BASE;
// The tiers bind the clamp twins (rast_sun*_pk); the union keeps the clip forms. khrs_clamp selects the clamp twin,
// falling through to the clip form when the twin is null.
inline ID3D11RasterizerState* kh_sun_rs_pick(bool khrs_clamp) {
    ID3D11RasterizerState* khrs_clip;
    ID3D11RasterizerState* khrs_pk;
    if (g_res.rast_sun_lo) { khrs_clip = g_res.rast_sun_lo; khrs_pk = g_res.rast_sun_lo_pk; }
    else                   { khrs_clip = g_res.rast_sun;    khrs_pk = g_res.rast_sun_pk; }
    return (khrs_clamp && khrs_pk) ? khrs_pk : khrs_clip;
}
static std::atomic<int> g_sun_range_src{0};

// The union takes the same width check as the tiers, so a size flip recreates it too (an unrecreated map under a
// larger viewport reads out of bounds = occluded).
inline bool ensure_sun_depth(ID3D11Device* dev) {
    return kh_sun_map_ensure(dev, kh_sun_depth_size(), g_res.sun_tex, g_res.sun_dsv, g_res.sun_srv);
}

// Non-fatal: a failure returns false where it happens, and the caller ignores the result. What was made before it
// stands (a tier whose pyramid is whole can still arm); the next ensure makes the rest, and kh_sun_pf_convert
// commits a whole chain only (KH_PF_WHOLE).
// KH_PF_AUTOGEN: the moment pyramids are built by GenerateMips when the device auto-generates R32G32_FLOAT mips.
// PSPfMip is the 2x2 mean of a level whose values are in [0, 1] (the saturate is a no-op there), which is the box
// filter the hardware applies, so the pyramid is the same and the per-level scratch copy + draw pair (11 of each
// per tier, per rebuild) collapses to one call. Without support the shader path stands.
static bool g_sun_pf_autogen = false;
inline bool ensure_sun_pf(ID3D11Device* dev) {
    if (!g_res.samp_pf) {
        D3D11_SAMPLER_DESC sp = {};
        sp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sp.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(dev->CreateSamplerState(&sp, &g_res.samp_pf))) return false;
    }
    if (!g_res.blend_pf) {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = FALSE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(dev->CreateBlendState(&bd, &g_res.blend_pf))) return false;
    }
    if (!g_res.vs_sunpf || !g_res.ps_sunpf) {
        static const D3D_SHADER_MACRO khpf_defs[] = { { nullptr, nullptr } };
        // KH_SUNPF_VS_ONCE: only a missing one is made - a pixel shader that failed leaves the vertex shader
        // standing, and re-creating it over the live pointer on every later call leaks it.
        if (!g_res.vs_sunpf) {
            ID3DBlob* khpf_vb = nullptr;
            if (!compile_shader(kh_hlsl_src(KH_HLSL_SUNPF).c_str(), "VSPf", "vs_5_0", khpf_defs, &khpf_vb).empty() ||
                !khpf_vb) return false;
            const HRESULT khpf_vh = dev->CreateVertexShader(khpf_vb->GetBufferPointer(),
                                                            khpf_vb->GetBufferSize(),
                                                            nullptr, &g_res.vs_sunpf);
            khpf_vb->Release();
            if (FAILED(khpf_vh)) { g_res.vs_sunpf = nullptr; return false; }
        }
        ID3DBlob* khpf_pb = nullptr;
        if (!compile_shader(kh_hlsl_src(KH_HLSL_SUNPF).c_str(), "PSPf", "ps_5_0", khpf_defs, &khpf_pb).empty() ||
            !khpf_pb) return false;
        const HRESULT khpf_ph = dev->CreatePixelShader(khpf_pb->GetBufferPointer(),
                                                       khpf_pb->GetBufferSize(),
                                                       nullptr, &g_res.ps_sunpf);
        khpf_pb->Release();
        if (FAILED(khpf_ph)) return false;
    }
    if (!g_res.ps_sunpfm) {
        static const D3D_SHADER_MACRO khpfm_defs[] = { { nullptr, nullptr } };
        ID3DBlob* khpfm_pb = nullptr;
        if (!compile_shader(kh_hlsl_src(KH_HLSL_SUNPF).c_str(), "PSPfMip", "ps_5_0", khpfm_defs, &khpfm_pb).empty() ||
            !khpfm_pb) return false;
        const HRESULT khpfm_ph = dev->CreatePixelShader(khpfm_pb->GetBufferPointer(),
                                                        khpfm_pb->GetBufferSize(),
                                                        nullptr, &g_res.ps_sunpfm);
        khpfm_pb->Release();
        if (FAILED(khpfm_ph)) return false;
    }
    const UINT khpf_sz[4] = {   // KH_SUN_LADDER: runtime, half the map. KH_FAR_PF: [3] the far tier.
        kh_sun_hero_size() / 2u, kh_sun_mid_size() / 2u, kh_sun_out_size() / 2u, kh_sun_far_size() / 2u };
    {   // KH_PF_AUTOGEN: a device property, read on every ensure (cheap, device-constant).
        UINT khpf_fs = 0;
        g_sun_pf_autogen = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_R32G32_FLOAT, &khpf_fs)) &&
                           (khpf_fs & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) != 0;
    }
    for (int khpf_i = 0; khpf_i < 4; ++khpf_i) {
        if (g_res.sun_pf_tex[khpf_i] && g_res.sun_pf_rtv[khpf_i] &&
            g_res.sun_pf_srv[khpf_i] && g_res.sun_pf_rtvm[khpf_i][1]) {
            D3D11_TEXTURE2D_DESC khpf_have = {};
            g_res.sun_pf_tex[khpf_i]->GetDesc(&khpf_have);
            // KH_PF_WHOLE: every level's view, not the first alone - a view create that fails part-way leaves the
            // chain short, and a test of level 1 alone would never make the rest.
            bool khpf_whole = true;
            for (UINT khpf_m = 1; (khpf_sz[khpf_i] >> khpf_m) >= 1u && khpf_m < 12u; ++khpf_m)
                if (!g_res.sun_pf_rtvm[khpf_i][khpf_m]) khpf_whole = false;
            if (khpf_have.Width == khpf_sz[khpf_i] && khpf_whole) continue;
            for (UINT khpf_m = 1; khpf_m < 12u; ++khpf_m)   // The mip views die with the texture.
                KH_SAFE_RELEASE(g_res.sun_pf_rtvm[khpf_i][khpf_m]);
        }
        kh_sun_pf_forget(khpf_i);   // KH_PF_WHOLE: what the flags vouched for goes with the texture.
        KH_SAFE_RELEASE(g_res.sun_pf_srv[khpf_i]);
        KH_SAFE_RELEASE(g_res.sun_pf_rtv[khpf_i]);
        KH_SAFE_RELEASE(g_res.sun_pf_tex[khpf_i]);
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = khpf_sz[khpf_i];
        td.Height = khpf_sz[khpf_i];
        td.MipLevels = 0;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32G32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (g_sun_pf_autogen) td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;   // KH_PF_AUTOGEN.
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.sun_pf_tex[khpf_i])))
            return false;
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = DXGI_FORMAT_R32G32_FLOAT;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(dev->CreateRenderTargetView(g_res.sun_pf_tex[khpf_i], &rd,
                                               &g_res.sun_pf_rtv[khpf_i]))) {
            KH_SAFE_RELEASE(g_res.sun_pf_tex[khpf_i]);
            return false;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_R32G32_FLOAT;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = static_cast<UINT>(-1);
        if (FAILED(dev->CreateShaderResourceView(g_res.sun_pf_tex[khpf_i], &sd,
                                                 &g_res.sun_pf_srv[khpf_i]))) {
            KH_SAFE_RELEASE(g_res.sun_pf_rtv[khpf_i]);
            KH_SAFE_RELEASE(g_res.sun_pf_tex[khpf_i]);
            return false;
        }
        // Per-level RTVs for the manual chain (level 0 = the RTV above; the loop stops at 1x1 or the array bound).
        for (UINT khpf_m = 1; (khpf_sz[khpf_i] >> khpf_m) >= 1u && khpf_m < 12u; ++khpf_m) {
            KH_SAFE_RELEASE(g_res.sun_pf_rtvm[khpf_i][khpf_m]);
            D3D11_RENDER_TARGET_VIEW_DESC rm = {};
            rm.Format = DXGI_FORMAT_R32G32_FLOAT;
            rm.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rm.Texture2D.MipSlice = khpf_m;
            if (FAILED(dev->CreateRenderTargetView(g_res.sun_pf_tex[khpf_i], &rm,
                                                   &g_res.sun_pf_rtvm[khpf_i][khpf_m])))
                return false;
        }
    }
    // KH_PF_SCR_MAX: the scratch takes each tier's level m - 1 whole (kh_sun_pf_convert's CopySubresourceRegion),
    // so it is the largest tier's size - the far tier is off the size ladder (kh_sun_far_size), and a copy larger
    // than the scratch's level is outside what D3D11 defines. Every tier is one size while g_sun_size_div is 1
    // (today, always).
    UINT khpf_scr_sz = khpf_sz[0];
    for (int khpf_k = 1; khpf_k < 4; ++khpf_k) {
        if (khpf_sz[khpf_k] > khpf_scr_sz) khpf_scr_sz = khpf_sz[khpf_k];
    }
    bool khpf_scr_stale = false;   // KH_SUN_LADDER: the scratch pyramid follows the tiers' sizes.
    if (g_res.sun_pf_scr) {
        D3D11_TEXTURE2D_DESC khpf_shave = {};
        g_res.sun_pf_scr->GetDesc(&khpf_shave);
        khpf_scr_stale = khpf_shave.Width != khpf_scr_sz;
    }
    for (UINT khpf_m = 0; (khpf_scr_sz >> khpf_m) >= 1u && khpf_m < 12u; ++khpf_m)   // KH_PF_WHOLE, the scratch's.
        if (!g_res.sun_pf_scr_srvm[khpf_m]) khpf_scr_stale = true;
    if (!g_res.sun_pf_scr || !g_res.sun_pf_scr_srvm[0] || khpf_scr_stale) {
        KH_SAFE_RELEASE(g_res.sun_pf_scr);
        for (UINT khpf_m = 0; khpf_m < 12u; ++khpf_m)
            KH_SAFE_RELEASE(g_res.sun_pf_scr_srvm[khpf_m]);
        D3D11_TEXTURE2D_DESC ts = {};
        ts.Width = khpf_scr_sz;
        ts.Height = khpf_scr_sz;
        ts.MipLevels = 0;
        ts.ArraySize = 1;
        ts.Format = DXGI_FORMAT_R32G32_FLOAT;
        ts.SampleDesc.Count = 1;
        ts.Usage = D3D11_USAGE_DEFAULT;
        ts.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&ts, nullptr, &g_res.sun_pf_scr))) return false;
        for (UINT khpf_m = 0; (khpf_scr_sz >> khpf_m) >= 1u && khpf_m < 12u; ++khpf_m) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sm = {};
            sm.Format = DXGI_FORMAT_R32G32_FLOAT;
            sm.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            sm.Texture2D.MostDetailedMip = khpf_m;
            sm.Texture2D.MipLevels = 1;
            if (FAILED(dev->CreateShaderResourceView(g_res.sun_pf_scr, &sm,
                                                     &g_res.sun_pf_scr_srvm[khpf_m])))
                return false;
        }
    }
    return true;
}

// One map ensure for every private sun map: R32_TYPELESS texture, D32 DSV, R32 SRV, square, single-sample.
// Non-fatal: a failed band stands itself down (meta stays 0).
inline bool kh_sun_map_ensure(ID3D11Device* dev, UINT khsm_size,
                              ID3D11Texture2D*& khsm_tex,
                              ID3D11DepthStencilView*& khsm_dsv,
                              ID3D11ShaderResourceView*& khsm_srv) {
    if (khsm_tex && khsm_dsv && khsm_srv) {
        D3D11_TEXTURE2D_DESC khsm_have = {};
        khsm_tex->GetDesc(&khsm_have);
        if (khsm_have.Width == khsm_size) return true;   // KH_SUN_LADDER: else recreate at the
                                                         // wanted size.
    }
    KH_SAFE_RELEASE(khsm_srv);
    KH_SAFE_RELEASE(khsm_dsv);
    KH_SAFE_RELEASE(khsm_tex);
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = khsm_size;
    td.Height = khsm_size;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &khsm_tex))) return false;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D32_FLOAT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(dev->CreateDepthStencilView(khsm_tex, &dd, &khsm_dsv))) {
        KH_SAFE_RELEASE(khsm_tex);
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_R32_FLOAT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    if (FAILED(dev->CreateShaderResourceView(khsm_tex, &sd, &khsm_srv))) {
        KH_SAFE_RELEASE(khsm_dsv);
        KH_SAFE_RELEASE(khsm_tex);
        return false;
    }
    return true;
}

inline bool ensure_sun_depth2(ID3D11Device* dev) {
    return kh_sun_map_ensure(dev, kh_sun_hero_size(), g_res.sun2_tex, g_res.sun2_dsv, g_res.sun2_srv);
}

inline bool ensure_sun_depth3(ID3D11Device* dev) {
    return kh_sun_map_ensure(dev, kh_sun_mid_size(), g_res.sun3_tex, g_res.sun3_dsv, g_res.sun3_srv);
}

inline bool ensure_sun_depth5(ID3D11Device* dev) {   // Far band.
    return kh_sun_map_ensure(dev, kh_sun_far_size(), g_res.sun5_tex, g_res.sun5_dsv, g_res.sun5_srv);
}

inline bool ensure_sun_depth4(ID3D11Device* dev) {
    return kh_sun_map_ensure(dev, kh_sun_out_size(), g_res.sun4_tex, g_res.sun4_dsv, g_res.sun4_srv);
}
// Self-term staleness ceiling (fill_lighting_frame_cb's age gate; the cast fire has its own 0.25 s window). 1 s rides
// out miss bursts without flicker; beyond it the self term stands down.
static constexpr float KH_SUN_MAP_SELF_MAX_AGE_S = 1.0f;
// The lit flag is structural: lighting1.w = g_sun_valid (published this session; never falls). The shading does not
// depend on the view lock: direction and colours publish at the depth clear.
// The 'fresh' publication bar (pub_fresh_ms; read by kh_adopt_frame_view - the cast's and the mesh passes' view
// adoption - and by kh_recv_health and band_capture).
static constexpr float    KH_LIT_FRESH_ERR = 0.20f;
// Union fit was camera-anchored (default) vs caster-anchored (camera-less).
static bool  g_sun_map_cam_anchor = false;
static float g_sun_map_vp[16] = {};   // row-vector world -> sun clip.
// KH_DLS_LOD_CAM - the camera the light maps pick their casters' levels at (kh_dls_frame, KH_SHADOW_LOD_DRAWN):
// the sun ladder's camera this cycle (engine axes; zero when camera-less), written by render_sun_depth ahead of
// its no-sun returns. g_sun_anchor_now is the same camera once a sun is published; before a session's first
// publish it stays at the origin while the light maps still render (they need no sun).
static float g_sun_cam_now[3] = {};
static float g_sun_map_anchor[3] = {};
static float g_sun_tier_anchor[4][3] = {};   // Maps 2 - 5 (hero to far), stamped only when that tier rendered.
// KH_PCSS_CELL: each camera tier's texel lattice offset (ConstantData::sun_lat), set with the render that made the map.
static uint32_t g_sun_tier_lat[4][2] = {};
// Rebase src (built about src_anchor) so it is relative to dst_anchor.
inline void kh_sun_rebase_vp(float khsr_dst[16], const float* khsr_src,
                             const float* khsr_src_anchor, const float* khsr_dst_anchor) {
    memcpy(khsr_dst, khsr_src, sizeof(float) * 16);
    const double khsr_d[3] = {
        static_cast<double>(khsr_dst_anchor[0]) - static_cast<double>(khsr_src_anchor[0]),
        static_cast<double>(khsr_dst_anchor[1]) - static_cast<double>(khsr_src_anchor[1]),
        static_cast<double>(khsr_dst_anchor[2]) - static_cast<double>(khsr_src_anchor[2]),
    };
    if (khsr_d[0] == 0.0 && khsr_d[1] == 0.0 && khsr_d[2] == 0.0) return;
    for (int khsr_j = 0; khsr_j < 4; ++khsr_j) {
        double khsr_t = static_cast<double>(khsr_src[12 + khsr_j]);
        khsr_t += khsr_d[0] * static_cast<double>(khsr_src[0 * 4 + khsr_j]);
        khsr_t += khsr_d[1] * static_cast<double>(khsr_src[1 * 4 + khsr_j]);
        khsr_t += khsr_d[2] * static_cast<double>(khsr_src[2 * 4 + khsr_j]);
        khsr_dst[12 + khsr_j] = static_cast<float>(khsr_t);
    }
}
static float g_sun_map_bias = 0.0f;   // Compare bias, normalized depth units.
static float    g_sun2_map_vp[4][4] = {};
static float    g_sun2_map_bias = 0.0f;
static float    g_sun2_half_diag = 0.0f;
static uint64_t g_sun2_renders = 0;
static float    g_sun3_map_vp[4][4] = {};
static float    g_sun3_map_bias = 0.0f;
static float    g_sun3_half_diag = 0.0f;
static uint64_t g_sun3_renders = 0;
static float    g_sun4_map_vp[4][4] = {};
static float    g_sun4_map_bias = 0.0f;
static float    g_sun4_half_diag = 0.0f;
static uint64_t g_sun4_renders = 0;
// KH_SUN_FAR_BAND state - the hero pattern once more (band 4).
static float    g_sun5_map_vp[4][4] = {};
static float    g_sun5_map_bias = 0.0f;
static float    g_sun5_half_diag = 0.0f;
static uint64_t g_sun5_renders = 0;

// The four camera-anchored bands fill CBFrame identically (rebased VP + meta: valid, size, bias, half-diag); one helper
// keeps the flush and injection fills from drifting. A stood-down band leaves its lanes zeroed, which the shader reads
// as 'no map'.
inline void kh_fill_sun_tier_cb(const float* khft_anchor,   // KH_SUN_TIER_ANCHOR: the anchor this
                                                            // tier rendered with.
                                float khft_vp[4][4], float khft_meta[4], bool khft_valid,
                                float khft_map_vp[4][4], float khft_bias,
                                float khft_half_diag, UINT khft_size) {
    if (!khft_valid) return;
    const float* khft_src = khft_anchor;
    kh_sun_rebase_vp(&khft_vp[0][0], &khft_map_vp[0][0], khft_src, g_sun_anchor_now);
    khft_meta[0] = 1.0f;
    khft_meta[1] = static_cast<float>(khft_size);
    khft_meta[2] = khft_bias;
    khft_meta[3] = khft_half_diag;
}
// Each camera tier's render is a function of its inputs (caster hash with the KH_TEX_LAND serial while a textured alpha
// caster is in the set, sun, snapped centre with the sun-axis component unsnapped, window, size, the anchor
// g_sun_anchor_now, reach latch, instance capacity). A tier whose key matches its last completed render keeps every
// static it left and renders nothing. KH_SUN_TIER_KEY_STILL: the anchor is the camera and the centre's sun-axis
// component is the camera's unsnapped one, so the key matches only while the camera stands bit-still - any camera
// motion re-renders every tier. Neither can leave the key and stay exact: a moved anchor re-rounds every
// anchor-relative vertex and the translation row (an ulp-level map change), a moved sun-axis centre moves the eye and
// shifts every stored depth. A skipped tier is not a fresh map: the convert reads pf_fresh, not pf_valid, and the
// anchor stamp follows the render.
static bool     g_sun_pf_fresh[4] = { false, false, false, false };   // Rendered since the last convert.
static uint64_t g_sun_tier_key[4] = { 0, 0, 0, 0 };
static bool     g_sun_tier_key_ok[4] = { false, false, false, false };

// KH_PF_NEED - which tiers' moment pyramids a receiver of ours can read (bit k: tier k = hero, mid, outer, far).
// KhSelfTier reads tier k's pyramid only for a sample point that reached tier k - no finer tier answered it - and
// lies inside tier k's window. A finer tier j answers every point inside its window whose edge weight KhTbW is at
// least 0.9999 (the shader's end test: e = max |clip.xy| at or under 0.7513, 0 < clip.z < 1): with a carry it
// blends and ends, without one it ends.
// The sample point is the receiver plus the normal offset, at most 2 of tier j's texels (khno_k <= 2 times the
// texel). So an object whose drawn box (kh_world_half_extents: everything it may draw), grown by that offset,
// projects inside |x|, |y| <= KH_PF_NEED_E and KH_PF_NEED_Z .. 1 - KH_PF_NEED_Z of tier j never reaches a tier
// past j. A tier up to j is needed only where the box's image meets its window: an image wholly outside it (past
// |x| or |y| = 1 or z outside 0 .. 1, by KH_PF_NEED_Z) holds no point its window test admits. The clip is affine
// (the sun projections are ortho), so a box's image is its centre's plus the sums of the rows' absolute values.
// A pass that shades under a copied frame template (the PIP, the view-model slice) passes it: the tiers and matrices
// its shader will read, and its own objects when they are a snapshot (the slice's). Otherwise the tiers are the maps as
// they stand - the fill after the convert writes those same lanes - and the objects are the scene: every colour-drawing
// object in it, a superset of what any pass draws from it. Render thread, or the game thread under the park.
static constexpr double KH_PF_NEED_E = 0.74;    // Under the 0.7513 KhTbW needs, by a margin for float.
static constexpr double KH_PF_NEED_Z = 1.0e-3;
inline uint32_t kh_sun_pf_need(const ConstantData* khpn_tpl, const std::vector<RenderObject>* khpn_objs) {
    struct KhPfTier { bool on; double vp[4][4]; double a[3]; double pad; };
    KhPfTier khpn_t[4];
    const bool khpn_gv[4] = { g_sun2_map_valid, g_sun3_map_valid, g_sun4_map_valid, g_sun5_map_valid };
    const float (*const khpn_gm[4])[4] = { g_sun2_map_vp, g_sun3_map_vp, g_sun4_map_vp, g_sun5_map_vp };
    const UINT khpn_gs[4] = { g_sun2_map_size, g_sun3_map_size, g_sun4_map_size, g_sun5_map_size };
    uint32_t khpn_all = 0;
    for (int k = 0; k < 4; ++k) {
        KhPfTier& t = khpn_t[k];
        const float (*m)[4] = khpn_gm[k];
        float khpn_sz = static_cast<float>(khpn_gs[k]);
        if (khpn_tpl) {   // The template's lanes: what that pass's shader reads.
            const float (*const khpn_tm[4])[4] = { khpn_tpl->sun_vp2, khpn_tpl->sun_vp3, khpn_tpl->sun_vp4,
                                                    khpn_tpl->sun_vp5 };
            const float* const khpn_tt[4] = { khpn_tpl->sun_meta2, khpn_tpl->sun_meta3, khpn_tpl->sun_meta4,
                                               khpn_tpl->sun_meta5 };
            t.on = khpn_tpl->sun_meta[0] >= 0.5f && khpn_tt[k][0] >= 0.5f;
            m = khpn_tm[k];
            khpn_sz = khpn_tt[k][1];
            for (int i = 0; i < 3; ++i) t.a[i] = static_cast<double>(khpn_tpl->sun_origin[i]);
        } else {
            t.on = khpn_gv[k];
            for (int i = 0; i < 3; ++i) t.a[i] = static_cast<double>(g_sun_tier_anchor[k][i]);
        }
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) t.vp[r][c] = static_cast<double>(m[r][c]);
        const double khpn_ir = sqrt(t.vp[0][0] * t.vp[0][0] + t.vp[1][0] * t.vp[1][0] + t.vp[2][0] * t.vp[2][0]);
        const double khpn_tw = 2.0 / (fmax(static_cast<double>(khpn_sz), 1.0) * fmax(khpn_ir, 1.0e-6));
        t.pad = 2.0 * khpn_tw * 1.01 + 1.0e-4;   // The normal offset's reach (KhSelfTier's khT_no).
        if (t.on) khpn_all |= 1u << k;
    }
    if (khpn_all == 0u) return 0u;
    uint32_t khpn_need = 0u;
    auto khpn_one = [&](const RenderObject& o) {
        if (!kh_obj_vis(o) || o.fullscreen) return;   // KH_SHOW_INFRONT.
        const double khpn_c[3] = { o.pos[0], o.pos[2], o.pos[1] };   // Engine axes.
        float khpn_hf[3];
        kh_world_half_extents(o, khpn_hf);
        uint32_t khpn_reach = 0u;
        for (int j = 0; j < 4; ++j) {   // Finest first; the far tier is the last (nothing lies past it).
            const KhPfTier& t = khpn_t[j];
            if (!t.on) continue;
            double khpn_cl[3], khpn_ex[3];
            for (int i = 0; i < 3; ++i) {
                khpn_cl[i] = t.vp[3][i];
                khpn_ex[i] = 0.0;
                for (int r = 0; r < 3; ++r) {
                    khpn_cl[i] += (khpn_c[r] - t.a[r]) * t.vp[r][i];
                    khpn_ex[i] += fabs(t.vp[r][i]) * (static_cast<double>(khpn_hf[r]) + t.pad);
                }
            }
            const bool khpn_out = fabs(khpn_cl[0]) - khpn_ex[0] >= 1.0 + KH_PF_NEED_Z ||
                                  fabs(khpn_cl[1]) - khpn_ex[1] >= 1.0 + KH_PF_NEED_Z ||
                                  khpn_cl[2] + khpn_ex[2] <= -KH_PF_NEED_Z ||
                                  khpn_cl[2] - khpn_ex[2] >= 1.0 + KH_PF_NEED_Z;   // NaN: not outside.
            if (khpn_out) continue;   // No point of it passes this window: past it, as if the tier were off.
            khpn_reach |= 1u << j;
            const bool khpn_in = fabs(khpn_cl[0]) + khpn_ex[0] <= KH_PF_NEED_E &&
                                 fabs(khpn_cl[1]) + khpn_ex[1] <= KH_PF_NEED_E &&
                                 khpn_cl[2] - khpn_ex[2] >= KH_PF_NEED_Z &&
                                 khpn_cl[2] + khpn_ex[2] <= 1.0 - KH_PF_NEED_Z;   // NaN: not held.
            if (khpn_in) break;   // Every point ends here: no tier past it is read.
        }
        khpn_need |= khpn_reach;
    };
    if (khpn_objs) {
        for (const RenderObject& o : *khpn_objs) { khpn_one(o); if (khpn_need == khpn_all) break; }
    } else {
        for (uint32_t s = 0; s < g_scene.objs.size() && khpn_need != khpn_all; ++s) {
            if (g_scene.alive[s]) khpn_one(g_scene.objs[s]);
        }
    }
    return khpn_need;
}
// KH_PF_NEED: a fresh map whose pyramid no receiver can read waits here, its pf_valid down (its lanes and its bind
// off) until a convert finds a reader; a render meanwhile re-arms pf_valid and the wait puts it down again.
static bool g_sun_pf_held[4] = { false, false, false, false };
// KH_PF_WHOLE: a tier's pyramid re-made by ensure_sun_pf holds nothing yet. Its flags drop - a tier converted under
// KH_PF_AUTOGEN needs no level views, so a pyramid re-made for a missing one would otherwise stay valid, and a
// matching key keep the tier from rendering the fresh map the convert waits for - and its key with them, so the tier
// renders again and the next convert fills the new pyramid.
inline void kh_sun_pf_forget(int khpf_t) {
    g_sun_pf_valid[khpf_t] = false;
    g_sun_pf_fresh[khpf_t] = false;
    g_sun_pf_held[khpf_t] = false;
    g_sun_tier_key_ok[khpf_t] = false;
}

// A failed step clears the band flag; the kernel stays classic there.
// KH_SESSION_RESET: a namespace static (not the function's) so the session reset reaches it.
static uint64_t g_sun_pf_done = ~0ull;
// khpc_tpl / khpc_objs: a template pass's lanes and snapshot (KH_PF_NEED); the injection and the flush pass neither.
inline void kh_sun_pf_convert(ID3D11DeviceContext* ctx, const ConstantData* khpc_tpl = nullptr,
                              const std::vector<RenderObject>* khpc_objs = nullptr) {
    const uint64_t khpc_mark = g_sun2_renders + g_sun3_renders + g_sun4_renders + g_sun5_renders;   // KH_FAR_PF.
    const bool khpc_held = g_sun_pf_held[0] || g_sun_pf_held[1] || g_sun_pf_held[2] || g_sun_pf_held[3];
    if (g_sun_pf_done == khpc_mark && !khpc_held) return;   // Once per fresh map set unless a tier is held (4 sites).
    g_sun_pf_done = khpc_mark;
    if (!g_res.vs_sunpf || !g_res.ps_sunpf || !g_res.ps_sunpfm ||
        !g_res.sun_pf_scr) {
        g_sun_pf_valid[0] = false; g_sun_pf_valid[1] = false; g_sun_pf_valid[2] = false; g_sun_pf_valid[3] = false;
        for (int khpc_k = 0; khpc_k < 4; ++khpc_k) g_sun_pf_held[khpc_k] = false;   // KH_PF_NEED.
        return;
    }
    // KH_PF_NEED: decided before any state is touched - a held tier a receiver now reads is due again; a fresh one
    // none reads waits. Nothing due: no state capture, no draw.
    const uint32_t khpc_need = kh_sun_pf_need(khpc_tpl, khpc_objs);
    bool khpc_due = false;
    for (int khpc_k = 0; khpc_k < 4; ++khpc_k) {
        const bool khpc_read = (khpc_need & (1u << khpc_k)) != 0u;
        if (g_sun_pf_held[khpc_k]) {
            if (!khpc_read) { g_sun_pf_valid[khpc_k] = false; continue; }
            g_sun_pf_held[khpc_k] = false;   // A held tier stays fresh (the wait never clears fresh):
            g_sun_pf_valid[khpc_k] = true;    // re-armed, it converts below.
        }
        if (!g_sun_pf_valid[khpc_k] || !g_sun_pf_fresh[khpc_k]) continue;
        if (!khpc_read) {
            g_sun_pf_held[khpc_k] = true;
            g_sun_pf_valid[khpc_k] = false;
            continue;
        }
        khpc_due = true;
    }
    if (!khpc_due) return;
    KhOmSave khpc_om;   // Full OM set in, full OM set out (rule: never a slot-0/4 save).
    khpc_om.capture(ctx);
    UINT khpc_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khpc_ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&khpc_nvp, khpc_ovp);
    D3D11_PRIMITIVE_TOPOLOGY khpc_topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&khpc_topo);
    ID3D11InputLayout* khpc_il = nullptr;
    ctx->IAGetInputLayout(&khpc_il);
    ID3D11VertexShader* khpc_vs = nullptr;
    ctx->VSGetShader(&khpc_vs, nullptr, nullptr);
    ID3D11PixelShader* khpc_ps = nullptr;
    ctx->PSGetShader(&khpc_ps, nullptr, nullptr);
    ID3D11ShaderResourceView* khpc_sr[2] = {};
    ctx->PSGetShaderResources(0, 2, khpc_sr);
    // The engine blend summed every convert into the target (hence the saved and replaced blend state); the rasterizer
    // joins for scissor / cull completeness.
    ID3D11BlendState* khpc_bl = nullptr;
    FLOAT khpc_bf[4] = {};
    UINT khpc_bm = 0xFFFFFFFF;
    ctx->OMGetBlendState(&khpc_bl, khpc_bf, &khpc_bm);
    ID3D11RasterizerState* khpc_rs = nullptr;
    ctx->RSGetState(&khpc_rs);
    ctx->OMSetBlendState(g_res.blend_pf, nullptr, 0xFFFFFFFF);
    if (g_res.rast_sun) ctx->RSSetState(g_res.rast_sun);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->VSSetShader(g_res.vs_sunpf, nullptr, 0);
    ID3D11ShaderResourceView* khpc_null = nullptr;

    const bool khpc_bv[4] = { g_sun2_map_valid, g_sun3_map_valid, g_sun4_map_valid, g_sun5_map_valid };
    ID3D11ShaderResourceView* const khpc_dep[4] = { g_res.sun2_srv, g_res.sun3_srv, g_res.sun4_srv, g_res.sun5_srv };
    UINT khpc_sz[4] = { 0, 0, 0, 0 };   // KH_SUN_LADDER: the pyramid's own size, never the wanted one.
    for (int khpc_k = 0; khpc_k < 4; ++khpc_k) {
        if (!g_res.sun_pf_tex[khpc_k]) continue;
        D3D11_TEXTURE2D_DESC khpc_td = {};
        g_res.sun_pf_tex[khpc_k]->GetDesc(&khpc_td);
        khpc_sz[khpc_k] = khpc_td.Width;
    }

    for (int khpc_i = 0; khpc_i < 4; ++khpc_i) {
        if (!g_sun_pf_valid[khpc_i]) continue;   // Band did not render fresh.
        if (!g_sun_pf_fresh[khpc_i]) continue;   // KH_SUN_TIER_KEY: the standing pyramid stands.
        g_sun_pf_fresh[khpc_i] = false;
        if (khpc_sz[khpc_i] == 0) { g_sun_pf_valid[khpc_i] = false; continue; }
        g_sun_pf_valid[khpc_i] = false;   // Proven again by completion below.
        if (!khpc_bv[khpc_i] || !khpc_dep[khpc_i] ||
            !g_res.sun_pf_rtv[khpc_i] || !g_res.sun_pf_srv[khpc_i]) continue;
        ctx->OMSetRenderTargets(1, &g_res.sun_pf_rtv[khpc_i], nullptr);
        D3D11_VIEWPORT khpc_vp = {};
        khpc_vp.Width = static_cast<FLOAT>(khpc_sz[khpc_i]);
        khpc_vp.Height = static_cast<FLOAT>(khpc_sz[khpc_i]);
        khpc_vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &khpc_vp);
        ctx->PSSetShader(g_res.ps_sunpf, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &khpc_dep[khpc_i]);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 1, &khpc_null);
        if (g_sun_pf_autogen) {   // KH_PF_AUTOGEN: the whole chain in one call, RTV unbound first.
            ctx->OMSetRenderTargets(0, nullptr, nullptr);
            ctx->GenerateMips(g_res.sun_pf_srv[khpc_i]);
            g_sun_pf_valid[khpc_i] = true;
            continue;
        }
        ctx->PSSetShader(g_res.ps_sunpfm, nullptr, 0);
        bool khpc_whole = true;   // KH_PF_WHOLE: a chain a missing view cut short is not committed.
        for (UINT khpc_m = 1; (khpc_sz[khpc_i] >> khpc_m) >= 1u && khpc_m < 12u; ++khpc_m) {
            ID3D11RenderTargetView* khpc_mr = g_res.sun_pf_rtvm[khpc_i][khpc_m];
            ID3D11ShaderResourceView* khpc_ms = g_res.sun_pf_scr_srvm[khpc_m - 1];
            if (!khpc_mr || !khpc_ms) { khpc_whole = false; break; }
            ctx->OMSetRenderTargets(0, nullptr, nullptr);
            ctx->CopySubresourceRegion(g_res.sun_pf_scr, khpc_m - 1, 0, 0, 0,
                                       g_res.sun_pf_tex[khpc_i], khpc_m - 1, nullptr);
            ctx->OMSetRenderTargets(1, &khpc_mr, nullptr);
            D3D11_VIEWPORT khpc_mvp = {};
            khpc_mvp.Width = static_cast<FLOAT>(khpc_sz[khpc_i] >> khpc_m);
            khpc_mvp.Height = static_cast<FLOAT>(khpc_sz[khpc_i] >> khpc_m);
            khpc_mvp.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &khpc_mvp);
            ctx->PSSetShaderResources(1, 1, &khpc_ms);
            ctx->Draw(3, 0);
        }
        ctx->PSSetShaderResources(1, 1, &khpc_null);
        g_sun_pf_valid[khpc_i] = khpc_whole;
        // KH_PF_WHOLE: a short chain converts again once the missing view is made (the next ensure) - forgetting the
        // tier drops its key, so it renders a fresh map for the convert even when nothing moved.
        if (!khpc_whole) kh_sun_pf_forget(khpc_i);
    }

    khpc_om.restore(ctx);
    khpc_om.release();
    if (khpc_nvp > 0) ctx->RSSetViewports(khpc_nvp, khpc_ovp);
    ctx->IASetPrimitiveTopology(khpc_topo);
    ctx->IASetInputLayout(khpc_il);
    KH_SAFE_RELEASE(khpc_il);
    ctx->VSSetShader(khpc_vs, nullptr, 0);
    KH_SAFE_RELEASE(khpc_vs);
    ctx->PSSetShader(khpc_ps, nullptr, 0);
    KH_SAFE_RELEASE(khpc_ps);
    ctx->OMSetBlendState(khpc_bl, khpc_bf, khpc_bm);
    KH_SAFE_RELEASE(khpc_bl);
    ctx->RSSetState(khpc_rs);
    KH_SAFE_RELEASE(khpc_rs);
    // The caller's t0/t1 pair.
    ctx->PSSetShaderResources(0, 2, khpc_sr);
    KH_SAFE_RELEASE(khpc_sr[0]);
    KH_SAFE_RELEASE(khpc_sr[1]);
}
// Texel-priced like KH_BAND_BIAS_TEXELS (ladder texels run 1 / 4 / 16 / 100 mm hero..far). The cast cannot acne (the
// receiver is never in our map), so this bias only absorbs the caster's own stored depth error.
static constexpr float KH_CAST_BIAS_CAP_M = 0.04f;
static float    g_sun_fwd_held[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
static float    g_sun_union_rad_held = -1.0f;
// -1 = the latch has never taken.
static float    g_sun_union_lat_held = -1.0f;

inline void kh_fill_sun_tiers_cb(ConstantData& cbd) {
    kh_sun_rebase_vp(&cbd.sun_vp[0][0], g_sun_map_vp, g_sun_map_anchor, g_sun_anchor_now);
    cbd.sun_meta[0] = g_sun_map_cam_anchor ? 2.0f : 1.0f;
    cbd.sun_meta[1] = static_cast<float>(g_sun_map_size);   // Rendered size (KH_SUN_LADDER).
    cbd.sun_meta[2] = g_sun_map_bias;
    cbd.sun_meta[3] = g_shadow_map_strength;
    // The mid and outer bands ride the same clock as the hero map (KH_SUN_CASCADE);
    // a downed/failed band leaves its meta zeroed and the shader chain falls through.
    kh_fill_sun_tier_cb(g_sun_tier_anchor[0], cbd.sun_vp2, cbd.sun_meta2, g_sun2_map_valid,
                        g_sun2_map_vp, g_sun2_map_bias, g_sun2_half_diag, g_sun2_map_size);
    kh_fill_sun_tier_cb(g_sun_tier_anchor[1], cbd.sun_vp3, cbd.sun_meta3, g_sun3_map_valid,
                        g_sun3_map_vp, g_sun3_map_bias, g_sun3_half_diag, g_sun3_map_size);
    kh_fill_sun_tier_cb(g_sun_tier_anchor[2], cbd.sun_vp4, cbd.sun_meta4, g_sun4_map_valid,
                        g_sun4_map_vp, g_sun4_map_bias, g_sun4_half_diag, g_sun4_map_size);
    kh_fill_sun_tier_cb(g_sun_tier_anchor[3], cbd.sun_vp5, cbd.sun_meta5, g_sun5_map_valid,
                        g_sun5_map_vp, g_sun5_map_bias, g_sun5_half_diag, g_sun5_map_size);   // Far band.
    // KH_PCSS_CELL: the lattice offsets, as bits.
    memcpy(&cbd.sun_lat[0][0], g_sun_tier_lat[0], 8); memcpy(&cbd.sun_lat[0][2], g_sun_tier_lat[1], 8);
    memcpy(&cbd.sun_lat[1][0], g_sun_tier_lat[2], 8); memcpy(&cbd.sun_lat[1][2], g_sun_tier_lat[3], 8);
    kh_dls_fill_cb(cbd);   // KH_DL_SHADOW: beside the sun tiers, same frame CB.
}

// The prefilter arms, as the passes bind the pyramids (a map and its converted pyramid). The frame fill writes
// them; a pass shading under a copied template (the PIP, the view-model slice) writes them again after its own
// convert (KH_PF_NEED), so they say what its binds bind.
inline void kh_fill_sun_pf_lanes(ConstantData& cbd) {
    cbd.sun_pf[0] = g_sun2_map_valid && g_sun_pf_valid[0] ? 1.0f : 0.0f;
    cbd.sun_pf[1] = g_sun3_map_valid && g_sun_pf_valid[1] ? 1.0f : 0.0f;
    cbd.sun_pf[2] = g_sun4_map_valid && g_sun_pf_valid[2] ? 1.0f : 0.0f;
    cbd.sun_origin[3] = g_sun5_map_valid && g_sun_pf_valid[3] ? 1.0f : 0.0f;   // KH_FAR_PF: the far arm.
}

// KH_SUN_LADDER: every flag that claims a sun map, a prefilter pyramid or a tier's completed render, plus the input
// hash render_sun_depth commits on success. All their subjects are Resources-owned, so this is the one authority for
// dropping them; both release paths go through it.
inline void kh_sun_ladder_forget() {
    g_sun_map_valid = false;
    g_sun2_map_valid = false;
    g_sun3_map_valid = false;
    g_sun4_map_valid = false;
    g_sun5_map_valid = false;
    g_sun_pf_valid[0] = false;
    g_sun_pf_valid[1] = false;
    g_sun_pf_valid[2] = false;   // Band validity flags above.
    g_sun_pf_valid[3] = false;   // KH_FAR_PF.
    for (int khtk_i = 0; khtk_i < 4; ++khtk_i) {   // KH_SUN_TIER_KEY.
        g_sun_pf_fresh[khtk_i] = false;
        g_sun_pf_held[khtk_i] = false;   // KH_PF_NEED.
        g_sun_tier_key_ok[khtk_i] = false;
    }
    g_sun_map_hash = 0;
}
}