#pragma once

// The feature half (rendering_integration_features.hpp): our sun shadow maps' setup: the ladder's sizes and targets,
// the anchors and tiers, the prefilter and the sun lanes. Never include it on its own.
namespace RenderIntegration {
// Defined with the DLS map state it reads.
inline void dls_fill_cb(ConstantData& cb);

inline bool sun_map_ensure(ID3D11Device* dev, UINT size,
                           ID3D11Texture2D*& tex,
                           ID3D11DepthStencilView*& dsv,
                           ID3D11ShaderResourceView*& srv);   // Defined with the bands.
inline void sun_pf_forget(int t);   // PF_WHOLE: defined with the tier flags (below).

static constexpr UINT SUN_DEPTH_BASE = 4096;   // The union map (SUN_LADDER).
// Bounding the fit bounds the texel size, the locality reach and the normal-offset at once. Casters beyond this
// camera radius stay out of the sun-depth fit (no self-shadow, no cast), like the engine's own object shadows. The
// depth window extends 12 m sun-ward so out-of-window occluders still cast in. Do not lower the ladder without
// re-pricing the offset floor.
static constexpr UINT  SUN_HERO_BASE = 4096;
static constexpr float SUN_HERO_HALF = 2.0f;
static constexpr float SUN_HERO_DEPTH = 12.0f;
static constexpr UINT  SUN_MID_BASE  = 4096;
static constexpr float SUN_MID_HALF  = 8.0f;
static constexpr float SUN_MID_DEPTH = 48.0f;
static constexpr UINT  SUN_OUT_BASE  = 4096;
static constexpr float SUN_OUT_HALF  = 32.0f;
static constexpr float SUN_OUT_DEPTH = 192.0f;
// The union remains behind it for the beyond-far presence test and anything outside the range window.
static constexpr UINT SUN_FAR_BASE = 4096;   // 12 cm texels over the 500 m fit; not on the ladder.
// Our sun shadow maps' setup: the ladder's sizes and viewports, the anchors and tiers, the biases and half diagonals,
// the prefilter's freshness and the held lanes.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: sun_size_div, sun_range_src.
// Session reset: session_globals_reset resets every member. session_scratch_reset resets 1 of them.
struct SunMapSetup {
    // Sizes are runtime (g_sun_setup.sun_size_div), the far band's excepted: sun_far_size is fixed (not on the
    // ladder).
    std::atomic<uint32_t> sun_size_div{1};
    // The size each map was last rendered at - what sunMeta.y must carry.
    UINT sun_map_size  = SUN_DEPTH_BASE;
    UINT sun2_map_size = SUN_HERO_BASE;
    UINT sun3_map_size = SUN_MID_BASE;
    UINT sun4_map_size = SUN_OUT_BASE;
    UINT sun5_map_size = SUN_FAR_BASE;
    std::atomic<int> sun_range_src{0};
    // Non-fatal: a failure returns false where it happens, and the caller ignores the result. What was made before it
    // stands (a tier whose pyramid is whole can still arm); the next ensure makes the rest, and sun_pf_convert
    // commits a whole chain only (PF_WHOLE).
    // PF_AUTOGEN: the moment pyramids are built by GenerateMips when the device auto-generates R32G32_FLOAT mips.
    // PSPfMip is the 2x2 mean of a level whose values are in [0, 1] (the saturate is a no-op there), which is the box
    // filter the hardware applies, so the pyramid is the same and the per-level scratch copy + draw pair (11 of each
    // per tier, per rebuild) collapses to one call. Without support the shader path stands.
    bool sun_pf_autogen = false;
    // Union fit was camera-anchored (default) vs caster-anchored (camera-less).
    bool  sun_map_cam_anchor = false;
    float sun_map_vp[16] = {};   // row-vector world -> sun clip.
    // DLS_LOD_CAM - the camera the light maps pick their casters' levels at (dls_frame, SHADOW_LOD_DRAWN): the
    // sun ladder's camera this cycle (engine axes; zero when camera-less), written by render_sun_depth ahead of its
    // no-sun returns. g_sun_maps.sun_anchor_now is the same camera once a sun is published; before a session's first
    // publish it stays at the origin while the light maps still render (they need no sun).
    float sun_cam_now[3] = {};
    float sun_map_anchor[3] = {};
    float sun_tier_anchor[4][3] = {};   // Maps 2 - 5 (hero to far), stamped only when that tier rendered.
    // PCSS_CELL: each camera tier's texel lattice offset (ConstantData::sun_lat), set with the render that made the
    // map.
    uint32_t sun_tier_lat[4][2] = {};
    float sun_map_bias = 0.0f;   // Compare bias, normalized depth units.
    float    sun2_map_vp[4][4] = {};
    float    sun2_map_bias = 0.0f;
    float    sun2_half_diag = 0.0f;
    uint64_t sun2_renders = 0;
    float    sun3_map_vp[4][4] = {};
    float    sun3_map_bias = 0.0f;
    float    sun3_half_diag = 0.0f;
    uint64_t sun3_renders = 0;
    float    sun4_map_vp[4][4] = {};
    float    sun4_map_bias = 0.0f;
    float    sun4_half_diag = 0.0f;
    uint64_t sun4_renders = 0;
    // SUN_FAR_BAND state - the hero pattern once more (band 4).
    float    sun5_map_vp[4][4] = {};
    float    sun5_map_bias = 0.0f;
    float    sun5_half_diag = 0.0f;
    uint64_t sun5_renders = 0;
    // Each camera tier's render is a function of its inputs (caster hash with the TEX_LAND serial while a textured
    // alpha caster is in the set, sun, snapped centre with the sun-axis component unsnapped, window, size, the anchor
    // g_sun_maps.sun_anchor_now, reach latch, instance capacity). A tier whose key matches its last completed render
    // keeps every static it left and renders nothing. SUN_TIER_KEY_STILL: the anchor is the camera and the centre's
    // sun-axis component is the camera's unsnapped one, so the key matches only while the camera stands bit-still - any
    // camera motion re-renders every tier. Neither can leave the key and stay exact: a moved anchor re-rounds every
    // anchor-relative vertex and the translation row (an ulp-level map change), a moved sun-axis centre moves the eye
    // and shifts every stored depth. A skipped tier is not a fresh map: the convert reads pf_fresh, not pf_valid, and
    // the anchor stamp follows the render.
    bool     sun_pf_fresh[4] = { false, false, false, false };   // Rendered since the last convert.
    uint64_t sun_tier_key[4] = { 0, 0, 0, 0 };
    bool     sun_tier_key_ok[4] = { false, false, false, false };
    // PF_NEED: a fresh map whose pyramid no receiver can read waits here, its pf_valid down (its lanes and its bind
    // off) until a convert finds a reader; a render meanwhile re-arms pf_valid and the wait puts it down again.
    bool sun_pf_held[4] = { false, false, false, false };
    // A failed step clears the band flag; the kernel stays classic there.
    // SESSION_RESET: a namespace static (not the function's) so the session reset reaches it.
    uint64_t sun_pf_done = ~0ull;
    float    sun_fwd_held[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
    float    sun_union_rad_held = -1.0f;
    // -1 = the latch has never taken.
    float    sun_union_lat_held = -1.0f;
};
static SunMapSetup g_sun_setup;
inline UINT sun_depth_size() { return SUN_DEPTH_BASE / g_sun_setup.sun_size_div.load(std::memory_order_relaxed); }
inline UINT sun_hero_size()  { return SUN_HERO_BASE  / g_sun_setup.sun_size_div.load(std::memory_order_relaxed); }
inline UINT sun_mid_size()   { return SUN_MID_BASE   / g_sun_setup.sun_size_div.load(std::memory_order_relaxed); }
inline UINT sun_out_size()   { return SUN_OUT_BASE   / g_sun_setup.sun_size_div.load(std::memory_order_relaxed); }
inline UINT sun_far_size()   { return SUN_FAR_BASE; }
inline void sun_size_latch() {
    g_sun_setup.sun_size_div.store(1u,
                                   std::memory_order_relaxed);
}
// The tiers bind the clamp twins (rast_sun*_pk); the union keeps the clip forms. clamp selects the clamp twin,
// falling through to the clip form when the twin is null.
inline ID3D11RasterizerState* sun_rs_pick(bool clamp) {
    ID3D11RasterizerState* clip;
    ID3D11RasterizerState* pk;
    if (g_res.rast_sun_lo) { clip = g_res.rast_sun_lo; pk = g_res.rast_sun_lo_pk; }
    else                   { clip = g_res.rast_sun;    pk = g_res.rast_sun_pk; }
    return (clamp && pk) ? pk : clip;
}

// The union takes the same width check as the tiers, so a size flip recreates it too (an unrecreated map under a
// larger viewport reads out of bounds = occluded).
inline bool ensure_sun_depth(ID3D11Device* dev) {
    return sun_map_ensure(dev, sun_depth_size(), g_res.sun_tex, g_res.sun_dsv, g_res.sun_srv);
}

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
        static const D3D_SHADER_MACRO defs[] = { { nullptr, nullptr } };
        // SUNPF_VS_ONCE: only a missing one is made - a pixel shader that failed leaves the vertex shader
        // standing, and re-creating it over the live pointer on every later call leaks it.
        if (!g_res.vs_sunpf) {
            ID3DBlob* vb = nullptr;
            if (!compile_shader(hlsl_src(HLSL_SUNPF).c_str(), "VSPf", "vs_5_0", defs, &vb).empty() ||
                !vb) return false;
            const HRESULT vh = dev->CreateVertexShader(vb->GetBufferPointer(),
                                                       vb->GetBufferSize(),
                                                       nullptr, &g_res.vs_sunpf);
            vb->Release();
            if (FAILED(vh)) { g_res.vs_sunpf = nullptr; return false; }
        }
        ID3DBlob* pb = nullptr;
        if (!compile_shader(hlsl_src(HLSL_SUNPF).c_str(), "PSPf", "ps_5_0", defs, &pb).empty() ||
            !pb) return false;
        const HRESULT ph = dev->CreatePixelShader(pb->GetBufferPointer(),
                                                  pb->GetBufferSize(),
                                                  nullptr, &g_res.ps_sunpf);
        pb->Release();
        if (FAILED(ph)) return false;
    }
    if (!g_res.ps_sunpfm) {
        static const D3D_SHADER_MACRO defs[] = { { nullptr, nullptr } };
        ID3DBlob* pb = nullptr;
        if (!compile_shader(hlsl_src(HLSL_SUNPF).c_str(), "PSPfMip", "ps_5_0", defs, &pb).empty() ||
            !pb) return false;
        const HRESULT ph = dev->CreatePixelShader(pb->GetBufferPointer(),
                                                  pb->GetBufferSize(),
                                                  nullptr, &g_res.ps_sunpfm);
        pb->Release();
        if (FAILED(ph)) return false;
    }
    const UINT sz[4] = {   // SUN_LADDER: runtime, half the map. FAR_PF: [3] the far tier.
        sun_hero_size() / 2u, sun_mid_size() / 2u, sun_out_size() / 2u, sun_far_size() / 2u };
    {   // PF_AUTOGEN: a device property, read on every ensure (cheap, device-constant).
        UINT fs = 0;
        g_sun_setup.sun_pf_autogen = SUCCEEDED(dev->CheckFormatSupport(DXGI_FORMAT_R32G32_FLOAT, &fs)) &&
                                     (fs & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN) != 0;
    }
    for (int i = 0; i < 4; ++i) {
        if (g_res.sun_pf_tex[i] && g_res.sun_pf_rtv[i] &&
            g_res.sun_pf_srv[i] && g_res.sun_pf_rtvm[i][1]) {
            D3D11_TEXTURE2D_DESC have = {};
            g_res.sun_pf_tex[i]->GetDesc(&have);
            // PF_WHOLE: every level's view, not the first alone - a view create that fails part-way leaves the
            // chain short, and a test of level 1 alone would never make the rest.
            bool whole = true;
            for (UINT m = 1; (sz[i] >> m) >= 1u && m < 12u; ++m)
                if (!g_res.sun_pf_rtvm[i][m]) whole = false;
            if (have.Width == sz[i] && whole) continue;
            for (UINT m = 1; m < 12u; ++m)   // The mip views die with the texture.
                KH_SAFE_RELEASE(g_res.sun_pf_rtvm[i][m]);
        }
        sun_pf_forget(i);   // PF_WHOLE: what the flags vouched for goes with the texture.
        KH_SAFE_RELEASE(g_res.sun_pf_srv[i]);
        KH_SAFE_RELEASE(g_res.sun_pf_rtv[i]);
        KH_SAFE_RELEASE(g_res.sun_pf_tex[i]);
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = sz[i];
        td.Height = sz[i];
        td.MipLevels = 0;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32G32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (g_sun_setup.sun_pf_autogen) td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;   // PF_AUTOGEN.
        if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.sun_pf_tex[i])))
            return false;
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = DXGI_FORMAT_R32G32_FLOAT;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        if (FAILED(dev->CreateRenderTargetView(g_res.sun_pf_tex[i], &rd,
                                               &g_res.sun_pf_rtv[i]))) {
            KH_SAFE_RELEASE(g_res.sun_pf_tex[i]);
            return false;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_R32G32_FLOAT;
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = static_cast<UINT>(-1);
        if (FAILED(dev->CreateShaderResourceView(g_res.sun_pf_tex[i], &sd,
                                                 &g_res.sun_pf_srv[i]))) {
            KH_SAFE_RELEASE(g_res.sun_pf_rtv[i]);
            KH_SAFE_RELEASE(g_res.sun_pf_tex[i]);
            return false;
        }
        // Per-level RTVs for the manual chain (level 0 = the RTV above; the loop stops at 1x1 or the array bound).
        for (UINT m = 1; (sz[i] >> m) >= 1u && m < 12u; ++m) {
            KH_SAFE_RELEASE(g_res.sun_pf_rtvm[i][m]);
            D3D11_RENDER_TARGET_VIEW_DESC rm = {};
            rm.Format = DXGI_FORMAT_R32G32_FLOAT;
            rm.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rm.Texture2D.MipSlice = m;
            if (FAILED(dev->CreateRenderTargetView(g_res.sun_pf_tex[i], &rm,
                                                   &g_res.sun_pf_rtvm[i][m])))
                return false;
        }
    }
    // PF_SCR_MAX: the scratch takes each tier's level m - 1 whole (sun_pf_convert's CopySubresourceRegion), so it
    // is the largest tier's size - the far tier is off the size ladder (sun_far_size), and a copy larger than the
    // scratch's level is outside what D3D11 defines. Every tier is one size while g_sun_setup.sun_size_div is 1 (today,
    // always).
    UINT scr_sz = sz[0];
    for (int k = 1; k < 4; ++k) {
        if (sz[k] > scr_sz) scr_sz = sz[k];
    }
    bool scr_stale = false;   // SUN_LADDER: the scratch pyramid follows the tiers' sizes.
    if (g_res.sun_pf_scr) {
        D3D11_TEXTURE2D_DESC shave = {};
        g_res.sun_pf_scr->GetDesc(&shave);
        scr_stale = shave.Width != scr_sz;
    }
    for (UINT m = 0; (scr_sz >> m) >= 1u && m < 12u; ++m)   // PF_WHOLE, the scratch's.
        if (!g_res.sun_pf_scr_srvm[m]) scr_stale = true;
    if (!g_res.sun_pf_scr || !g_res.sun_pf_scr_srvm[0] || scr_stale) {
        KH_SAFE_RELEASE(g_res.sun_pf_scr);
        for (UINT m = 0; m < 12u; ++m)
            KH_SAFE_RELEASE(g_res.sun_pf_scr_srvm[m]);
        D3D11_TEXTURE2D_DESC ts = {};
        ts.Width = scr_sz;
        ts.Height = scr_sz;
        ts.MipLevels = 0;
        ts.ArraySize = 1;
        ts.Format = DXGI_FORMAT_R32G32_FLOAT;
        ts.SampleDesc.Count = 1;
        ts.Usage = D3D11_USAGE_DEFAULT;
        ts.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(dev->CreateTexture2D(&ts, nullptr, &g_res.sun_pf_scr))) return false;
        for (UINT m = 0; (scr_sz >> m) >= 1u && m < 12u; ++m) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sm = {};
            sm.Format = DXGI_FORMAT_R32G32_FLOAT;
            sm.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            sm.Texture2D.MostDetailedMip = m;
            sm.Texture2D.MipLevels = 1;
            if (FAILED(dev->CreateShaderResourceView(g_res.sun_pf_scr, &sm,
                                                     &g_res.sun_pf_scr_srvm[m])))
                return false;
        }
    }
    return true;
}

// One map ensure for every private sun map: R32_TYPELESS texture, D32 DSV, R32 SRV, square, single-sample.
// Non-fatal: a failed band stands itself down (meta stays 0).
inline bool sun_map_ensure(ID3D11Device* dev, UINT size,
                           ID3D11Texture2D*& tex,
                           ID3D11DepthStencilView*& dsv,
                           ID3D11ShaderResourceView*& srv) {
    if (tex && dsv && srv) {
        D3D11_TEXTURE2D_DESC have = {};
        tex->GetDesc(&have);
        if (have.Width == size) return true;   // SUN_LADDER: else recreate at the
                                                         // wanted size.
    }
    KH_SAFE_RELEASE(srv);
    KH_SAFE_RELEASE(dsv);
    KH_SAFE_RELEASE(tex);
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = size;
    td.Height = size;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &tex))) return false;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D32_FLOAT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(dev->CreateDepthStencilView(tex, &dd, &dsv))) {
        KH_SAFE_RELEASE(tex);
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_R32_FLOAT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    if (FAILED(dev->CreateShaderResourceView(tex, &sd, &srv))) {
        KH_SAFE_RELEASE(dsv);
        KH_SAFE_RELEASE(tex);
        return false;
    }
    return true;
}

inline bool ensure_sun_depth2(ID3D11Device* dev) {
    return sun_map_ensure(dev, sun_hero_size(), g_res.sun2_tex, g_res.sun2_dsv, g_res.sun2_srv);
}

inline bool ensure_sun_depth3(ID3D11Device* dev) {
    return sun_map_ensure(dev, sun_mid_size(), g_res.sun3_tex, g_res.sun3_dsv, g_res.sun3_srv);
}

inline bool ensure_sun_depth5(ID3D11Device* dev) {   // Far band.
    return sun_map_ensure(dev, sun_far_size(), g_res.sun5_tex, g_res.sun5_dsv, g_res.sun5_srv);
}

inline bool ensure_sun_depth4(ID3D11Device* dev) {
    return sun_map_ensure(dev, sun_out_size(), g_res.sun4_tex, g_res.sun4_dsv, g_res.sun4_srv);
}
// Self-term staleness ceiling (fill_lighting_frame_cb's age gate; the cast fire has its own 0.25 s window). 1 s rides
// out miss bursts without flicker; beyond it the self term stands down.
static constexpr float SUN_MAP_SELF_MAX_AGE_S = 1.0f;
// The lit flag is structural: lighting1.w = g_sun_valid (published this session; never falls). The shading does not
// depend on the view lock: direction and colours publish at the depth clear.
// The 'fresh' publication bar (pub_fresh_ms; read by adopt_frame_view - the cast's and the mesh passes' view
// adoption - and by recv_health and band_capture).
static constexpr float    LIT_FRESH_ERR = 0.20f;
// Rebase src (built about src_anchor) so it is relative to dst_anchor.
inline void sun_rebase_vp(float dst[16], const float* src,
                          const float* src_anchor, const float* dst_anchor) {
    memcpy(dst, src, sizeof(float) * 16);
    const double d[3] = {
        static_cast<double>(dst_anchor[0]) - static_cast<double>(src_anchor[0]),
        static_cast<double>(dst_anchor[1]) - static_cast<double>(src_anchor[1]),
        static_cast<double>(dst_anchor[2]) - static_cast<double>(src_anchor[2]),
    };
    if (d[0] == 0.0 && d[1] == 0.0 && d[2] == 0.0) return;
    for (int j = 0; j < 4; ++j) {
        double t = static_cast<double>(src[12 + j]);
        t += d[0] * static_cast<double>(src[0 * 4 + j]);
        t += d[1] * static_cast<double>(src[1 * 4 + j]);
        t += d[2] * static_cast<double>(src[2 * 4 + j]);
        dst[12 + j] = static_cast<float>(t);
    }
}

// The four camera-anchored bands fill CBFrame identically (rebased VP + meta: valid, size, bias, half-diag); one helper
// keeps the flush and injection fills from drifting. A stood-down band leaves its lanes zeroed, which the shader reads
// as 'no map'.
inline void fill_sun_tier_cb(const float* anchor,   // SUN_TIER_ANCHOR: the anchor this
                                                            // tier rendered with.
                             float vp[4][4], float meta[4], bool valid,
                             float map_vp[4][4], float bias,
                             float half_diag, UINT size) {
    if (!valid) return;
    const float* src = anchor;
    sun_rebase_vp(&vp[0][0], &map_vp[0][0], src, g_sun_maps.sun_anchor_now);
    meta[0] = 1.0f;
    meta[1] = static_cast<float>(size);
    meta[2] = bias;
    meta[3] = half_diag;
}

// PF_NEED - which tiers' moment pyramids a receiver of ours can read (bit k: tier k = hero, mid, outer, far).
// KhSelfTier reads tier k's pyramid only for a sample point that reached tier k - no finer tier answered it - and
// lies inside tier k's window. A finer tier j answers every point inside its window whose edge weight KhTbW is at
// least 0.9999 (the shader's end test: e = max |clip.xy| at or under 0.7513, 0 < clip.z < 1): with a carry it
// blends and ends, without one it ends.
// The sample point is the receiver plus the normal offset, at most 2 of tier j's texels (khno_k <= 2 times the
// texel). So an object whose drawn box (world_half_extents: everything it may draw), grown by that offset,
// projects inside |x|, |y| <= PF_NEED_E and PF_NEED_Z .. 1 - PF_NEED_Z of tier j never reaches a tier
// past j. A tier up to j is needed only where the box's image meets its window: an image wholly outside it (past
// |x| or |y| = 1 or z outside 0 .. 1, by PF_NEED_Z) holds no point its window test admits. The clip is affine
// (the sun projections are ortho), so a box's image is its centre's plus the sums of the rows' absolute values.
// A pass that shades under a copied frame template (the PIP, the view-model slice) passes it: the tiers and matrices
// its shader will read, and its own objects when they are a snapshot (the slice's). Otherwise the tiers are the maps as
// they stand - the fill after the convert writes those same lanes - and the objects are the scene: every colour-drawing
// object in it, a superset of what any pass draws from it. Render thread, or the game thread under the park.
static constexpr double PF_NEED_E = 0.74;    // Under the 0.7513 KhTbW needs, by a margin for float.
static constexpr double PF_NEED_Z = 1.0e-3;
inline uint32_t sun_pf_need(const ConstantData* tpl, const std::vector<RenderObject>* objs) {
    struct PfTier { bool on; double vp[4][4]; double a[3]; double pad; };
    PfTier pn_t[4];
    const bool gv[4] = { g_sun_maps.sun2_map_valid, g_sun_maps.sun3_map_valid, g_sun_maps.sun4_map_valid,
        g_sun_maps.sun5_map_valid };
    const float (*const gm[4])[4] = { g_sun_setup.sun2_map_vp, g_sun_setup.sun3_map_vp, g_sun_setup.sun4_map_vp,
        g_sun_setup.sun5_map_vp };
    const UINT gs[4] = { g_sun_setup.sun2_map_size, g_sun_setup.sun3_map_size, g_sun_setup.sun4_map_size,
        g_sun_setup.sun5_map_size };
    uint32_t all = 0;
    for (int k = 0; k < 4; ++k) {
        PfTier& t = pn_t[k];
        const float (*m)[4] = gm[k];
        float sz = static_cast<float>(gs[k]);
        if (tpl) {   // The template's lanes: what that pass's shader reads.
            const float (*const tm[4])[4] = { tpl->sun_vp2, tpl->sun_vp3, tpl->sun_vp4,
                                                    tpl->sun_vp5 };
            const float* const tt[4] = { tpl->sun_meta2, tpl->sun_meta3, tpl->sun_meta4,
                                               tpl->sun_meta5 };
            t.on = tpl->sun_meta[0] >= 0.5f && tt[k][0] >= 0.5f;
            m = tm[k];
            sz = tt[k][1];
            for (int i = 0; i < 3; ++i) t.a[i] = static_cast<double>(tpl->sun_origin[i]);
        } else {
            t.on = gv[k];
            for (int i = 0; i < 3; ++i) t.a[i] = static_cast<double>(g_sun_setup.sun_tier_anchor[k][i]);
        }
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) t.vp[r][c] = static_cast<double>(m[r][c]);
        const double ir = sqrt(t.vp[0][0] * t.vp[0][0] + t.vp[1][0] * t.vp[1][0] + t.vp[2][0] * t.vp[2][0]);
        const double tw = 2.0 / (fmax(static_cast<double>(sz), 1.0) * fmax(ir, 1.0e-6));
        t.pad = 2.0 * tw * 1.01 + 1.0e-4;   // The normal offset's reach (KhSelfTier's khT_no).
        if (t.on) all |= 1u << k;
    }
    if (all == 0u) return 0u;
    uint32_t need = 0u;
    auto one = [&](const RenderObject& o) {
        if (!obj_vis(o) || o.fullscreen) return;   // SHOW_INFRONT.
        const double pn_c[3] = { o.pos[0], o.pos[2], o.pos[1] };   // Engine axes.
        float hf[3];
        world_half_extents(o, hf);
        uint32_t reach = 0u;
        for (int j = 0; j < 4; ++j) {   // Finest first; the far tier is the last (nothing lies past it).
            const PfTier& t = pn_t[j];
            if (!t.on) continue;
            double cl[3], ex[3];
            for (int i = 0; i < 3; ++i) {
                cl[i] = t.vp[3][i];
                ex[i] = 0.0;
                for (int r = 0; r < 3; ++r) {
                    cl[i] += (pn_c[r] - t.a[r]) * t.vp[r][i];
                    ex[i] += fabs(t.vp[r][i]) * (static_cast<double>(hf[r]) + t.pad);
                }
            }
            const bool out = fabs(cl[0]) - ex[0] >= 1.0 + PF_NEED_Z ||
                             fabs(cl[1]) - ex[1] >= 1.0 + PF_NEED_Z ||
                             cl[2] + ex[2] <= -PF_NEED_Z ||
                             cl[2] - ex[2] >= 1.0 + PF_NEED_Z;   // NaN: not outside.
            if (out) continue;   // No point of it passes this window: past it, as if the tier were off.
            reach |= 1u << j;
            const bool in = fabs(cl[0]) + ex[0] <= PF_NEED_E &&
                            fabs(cl[1]) + ex[1] <= PF_NEED_E &&
                            cl[2] - ex[2] >= PF_NEED_Z &&
                            cl[2] + ex[2] <= 1.0 - PF_NEED_Z;   // NaN: not held.
            if (in) break;   // Every point ends here: no tier past it is read.
        }
        need |= reach;
    };
    if (objs) {
        for (const RenderObject& o : *objs) { one(o); if (need == all) break; }
    } else {
        for (uint32_t s = 0; s < g_scene.objs.size() && need != all; ++s) {
            if (g_scene.alive[s]) one(g_scene.objs[s]);
        }
    }
    return need;
}
// PF_WHOLE: a tier's pyramid re-made by ensure_sun_pf holds nothing yet. Its flags drop - a tier converted under
// PF_AUTOGEN needs no level views, so a pyramid re-made for a missing one would otherwise stay valid, and a
// matching key keep the tier from rendering the fresh map the convert waits for - and its key with them, so the tier
// renders again and the next convert fills the new pyramid.
inline void sun_pf_forget(int t) {
    g_sun_maps.sun_pf_valid[t] = false;
    g_sun_setup.sun_pf_fresh[t] = false;
    g_sun_setup.sun_pf_held[t] = false;
    g_sun_setup.sun_tier_key_ok[t] = false;
}

// tpl / objs: a template pass's lanes and snapshot (PF_NEED); the injection and the flush pass neither.
inline void sun_pf_convert(ID3D11DeviceContext* ctx, const ConstantData* tpl = nullptr,
                           const std::vector<RenderObject>* objs = nullptr) {
    const uint64_t mark = g_sun_setup.sun2_renders + g_sun_setup.sun3_renders + g_sun_setup.sun4_renders +
                          g_sun_setup.sun5_renders;   // FAR_PF.
    const bool held = g_sun_setup.sun_pf_held[0] || g_sun_setup.sun_pf_held[1] || g_sun_setup.sun_pf_held[2] ||
                      g_sun_setup.sun_pf_held[3];
    // Once per fresh map set unless a tier is held (4 sites).
    if (g_sun_setup.sun_pf_done == mark && !held) return;
    g_sun_setup.sun_pf_done = mark;
    if (!g_res.vs_sunpf || !g_res.ps_sunpf || !g_res.ps_sunpfm ||
        !g_res.sun_pf_scr) {
        g_sun_maps.sun_pf_valid[0] = false; g_sun_maps.sun_pf_valid[1] = false; g_sun_maps.sun_pf_valid[2] = false;
        g_sun_maps.sun_pf_valid[3] = false;
        for (int k = 0; k < 4; ++k) g_sun_setup.sun_pf_held[k] = false;   // PF_NEED.
        return;
    }
    // PF_NEED: decided before any state is touched - a held tier a receiver now reads is due again; a fresh one
    // none reads waits. Nothing due: no state capture, no draw.
    const uint32_t need = sun_pf_need(tpl, objs);
    bool due = false;
    for (int k = 0; k < 4; ++k) {
        const bool read = (need & (1u << k)) != 0u;
        if (g_sun_setup.sun_pf_held[k]) {
            if (!read) { g_sun_maps.sun_pf_valid[k] = false; continue; }
            g_sun_setup.sun_pf_held[k] = false;   // A held tier stays fresh (the wait never clears fresh):
            g_sun_maps.sun_pf_valid[k] = true;    // re-armed, it converts below.
        }
        if (!g_sun_maps.sun_pf_valid[k] || !g_sun_setup.sun_pf_fresh[k]) continue;
        if (!read) {
            g_sun_setup.sun_pf_held[k] = true;
            g_sun_maps.sun_pf_valid[k] = false;
            continue;
        }
        due = true;
    }
    if (!due) return;
    OmSave om;   // Full OM set in, full OM set out (rule: never a slot-0/4 save).
    om.capture(ctx);
    UINT nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&nvp, ovp);
    D3D11_PRIMITIVE_TOPOLOGY topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ctx->IAGetPrimitiveTopology(&topo);
    ID3D11InputLayout* il = nullptr;
    ctx->IAGetInputLayout(&il);
    ID3D11VertexShader* vs = nullptr;
    ctx->VSGetShader(&vs, nullptr, nullptr);
    ID3D11PixelShader* ps = nullptr;
    ctx->PSGetShader(&ps, nullptr, nullptr);
    ID3D11ShaderResourceView* sr[2] = {};
    ctx->PSGetShaderResources(0, 2, sr);
    // The engine blend summed every convert into the target (hence the saved and replaced blend state); the rasterizer
    // joins for scissor / cull completeness.
    ID3D11BlendState* bl = nullptr;
    FLOAT bf[4] = {};
    UINT bm = 0xFFFFFFFF;
    ctx->OMGetBlendState(&bl, bf, &bm);
    ID3D11RasterizerState* rs = nullptr;
    ctx->RSGetState(&rs);
    ctx->OMSetBlendState(g_res.blend_pf, nullptr, 0xFFFFFFFF);
    if (g_res.rast_sun) ctx->RSSetState(g_res.rast_sun);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetInputLayout(nullptr);
    ctx->VSSetShader(g_res.vs_sunpf, nullptr, 0);
    ID3D11ShaderResourceView* null = nullptr;

    const bool bv[4] = { g_sun_maps.sun2_map_valid, g_sun_maps.sun3_map_valid, g_sun_maps.sun4_map_valid,
        g_sun_maps.sun5_map_valid };
    ID3D11ShaderResourceView* const dep[4] = { g_res.sun2_srv, g_res.sun3_srv, g_res.sun4_srv, g_res.sun5_srv };
    UINT sz[4] = { 0, 0, 0, 0 };   // SUN_LADDER: the pyramid's own size, never the wanted one.
    for (int k = 0; k < 4; ++k) {
        if (!g_res.sun_pf_tex[k]) continue;
        D3D11_TEXTURE2D_DESC td = {};
        g_res.sun_pf_tex[k]->GetDesc(&td);
        sz[k] = td.Width;
    }

    for (int i = 0; i < 4; ++i) {
        if (!g_sun_maps.sun_pf_valid[i]) continue;   // Band did not render fresh.
        if (!g_sun_setup.sun_pf_fresh[i]) continue;   // SUN_TIER_KEY: the standing pyramid stands.
        g_sun_setup.sun_pf_fresh[i] = false;
        if (sz[i] == 0) { g_sun_maps.sun_pf_valid[i] = false; continue; }
        g_sun_maps.sun_pf_valid[i] = false;   // Proven again by completion below.
        if (!bv[i] || !dep[i] ||
            !g_res.sun_pf_rtv[i] || !g_res.sun_pf_srv[i]) continue;
        ctx->OMSetRenderTargets(1, &g_res.sun_pf_rtv[i], nullptr);
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<FLOAT>(sz[i]);
        vp.Height = static_cast<FLOAT>(sz[i]);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        ctx->PSSetShader(g_res.ps_sunpf, nullptr, 0);
        ctx->PSSetShaderResources(0, 1, &dep[i]);
        ctx->Draw(3, 0);
        ctx->PSSetShaderResources(0, 1, &null);
        if (g_sun_setup.sun_pf_autogen) {   // PF_AUTOGEN: the whole chain in one call, RTV unbound first.
            ctx->OMSetRenderTargets(0, nullptr, nullptr);
            ctx->GenerateMips(g_res.sun_pf_srv[i]);
            g_sun_maps.sun_pf_valid[i] = true;
            continue;
        }
        ctx->PSSetShader(g_res.ps_sunpfm, nullptr, 0);
        bool whole = true;   // PF_WHOLE: a chain a missing view cut short is not committed.
        for (UINT m = 1; (sz[i] >> m) >= 1u && m < 12u; ++m) {
            ID3D11RenderTargetView* mr = g_res.sun_pf_rtvm[i][m];
            ID3D11ShaderResourceView* ms = g_res.sun_pf_scr_srvm[m - 1];
            if (!mr || !ms) { whole = false; break; }
            ctx->OMSetRenderTargets(0, nullptr, nullptr);
            ctx->CopySubresourceRegion(g_res.sun_pf_scr, m - 1, 0, 0, 0,
                                       g_res.sun_pf_tex[i], m - 1, nullptr);
            ctx->OMSetRenderTargets(1, &mr, nullptr);
            D3D11_VIEWPORT mvp = {};
            mvp.Width = static_cast<FLOAT>(sz[i] >> m);
            mvp.Height = static_cast<FLOAT>(sz[i] >> m);
            mvp.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &mvp);
            ctx->PSSetShaderResources(1, 1, &ms);
            ctx->Draw(3, 0);
        }
        ctx->PSSetShaderResources(1, 1, &null);
        g_sun_maps.sun_pf_valid[i] = whole;
        // PF_WHOLE: a short chain converts again once the missing view is made (the next ensure) - forgetting the
        // tier drops its key, so it renders a fresh map for the convert even when nothing moved.
        if (!whole) sun_pf_forget(i);
    }

    om.restore(ctx);
    om.release();
    if (nvp > 0) ctx->RSSetViewports(nvp, ovp);
    ctx->IASetPrimitiveTopology(topo);
    ctx->IASetInputLayout(il);
    KH_SAFE_RELEASE(il);
    ctx->VSSetShader(vs, nullptr, 0);
    KH_SAFE_RELEASE(vs);
    ctx->PSSetShader(ps, nullptr, 0);
    KH_SAFE_RELEASE(ps);
    ctx->OMSetBlendState(bl, bf, bm);
    KH_SAFE_RELEASE(bl);
    ctx->RSSetState(rs);
    KH_SAFE_RELEASE(rs);
    // The caller's t0/t1 pair.
    ctx->PSSetShaderResources(0, 2, sr);
    KH_SAFE_RELEASE(sr[0]);
    KH_SAFE_RELEASE(sr[1]);
}
// Texel-priced like BAND_BIAS_TEXELS (ladder texels run 1 / 4 / 16 / 100 mm hero..far). The cast cannot acne (the
// receiver is never in our map), so this bias only absorbs the caster's own stored depth error.
static constexpr float CAST_BIAS_CAP_M = 0.04f;

inline void fill_sun_tiers_cb(ConstantData& cbd) {
    sun_rebase_vp(&cbd.sun_vp[0][0], g_sun_setup.sun_map_vp, g_sun_setup.sun_map_anchor, g_sun_maps.sun_anchor_now);
    cbd.sun_meta[0] = g_sun_setup.sun_map_cam_anchor ? 2.0f : 1.0f;
    cbd.sun_meta[1] = static_cast<float>(g_sun_setup.sun_map_size);   // Rendered size (SUN_LADDER).
    cbd.sun_meta[2] = g_sun_setup.sun_map_bias;
    cbd.sun_meta[3] = g_shadow_map_strength;
    // The mid and outer bands ride the same clock as the hero map (SUN_CASCADE);
    // a downed/failed band leaves its meta zeroed and the shader chain falls through.
    fill_sun_tier_cb(g_sun_setup.sun_tier_anchor[0], cbd.sun_vp2, cbd.sun_meta2, g_sun_maps.sun2_map_valid,
                     g_sun_setup.sun2_map_vp, g_sun_setup.sun2_map_bias, g_sun_setup.sun2_half_diag,
                            g_sun_setup.sun2_map_size);
    fill_sun_tier_cb(g_sun_setup.sun_tier_anchor[1], cbd.sun_vp3, cbd.sun_meta3, g_sun_maps.sun3_map_valid,
                     g_sun_setup.sun3_map_vp, g_sun_setup.sun3_map_bias, g_sun_setup.sun3_half_diag,
                            g_sun_setup.sun3_map_size);
    fill_sun_tier_cb(g_sun_setup.sun_tier_anchor[2], cbd.sun_vp4, cbd.sun_meta4, g_sun_maps.sun4_map_valid,
                     g_sun_setup.sun4_map_vp, g_sun_setup.sun4_map_bias, g_sun_setup.sun4_half_diag,
                            g_sun_setup.sun4_map_size);
    fill_sun_tier_cb(g_sun_setup.sun_tier_anchor[3], cbd.sun_vp5, cbd.sun_meta5, g_sun_maps.sun5_map_valid,
                     // Far band.
                     g_sun_setup.sun5_map_vp, g_sun_setup.sun5_map_bias, g_sun_setup.sun5_half_diag,
                            g_sun_setup.sun5_map_size);
    // PCSS_CELL: the lattice offsets, as bits.
    memcpy(&cbd.sun_lat[0][0], g_sun_setup.sun_tier_lat[0], 8);
    memcpy(&cbd.sun_lat[0][2], g_sun_setup.sun_tier_lat[1], 8);
    memcpy(&cbd.sun_lat[1][0], g_sun_setup.sun_tier_lat[2], 8);
    memcpy(&cbd.sun_lat[1][2], g_sun_setup.sun_tier_lat[3], 8);
    dls_fill_cb(cbd);   // DL_SHADOW: beside the sun tiers, same frame CB.
}

// The prefilter arms, as the passes bind the pyramids (a map and its converted pyramid). The frame fill writes
// them; a pass shading under a copied template (the PIP, the view-model slice) writes them again after its own
// convert (PF_NEED), so they say what its binds bind.
inline void fill_sun_pf_lanes(ConstantData& cbd) {
    cbd.sun_pf[0] = g_sun_maps.sun2_map_valid && g_sun_maps.sun_pf_valid[0] ? 1.0f : 0.0f;
    cbd.sun_pf[1] = g_sun_maps.sun3_map_valid && g_sun_maps.sun_pf_valid[1] ? 1.0f : 0.0f;
    cbd.sun_pf[2] = g_sun_maps.sun4_map_valid && g_sun_maps.sun_pf_valid[2] ? 1.0f : 0.0f;
    // FAR_PF: the far arm.
    cbd.sun_origin[3] = g_sun_maps.sun5_map_valid && g_sun_maps.sun_pf_valid[3] ? 1.0f : 0.0f;
}

// SUN_LADDER: every flag that claims a sun map, a prefilter pyramid or a tier's completed render, plus the input
// hash render_sun_depth commits on success. All their subjects are Resources-owned, so this is the one authority for
// dropping them; both release paths go through it.
inline void sun_ladder_forget() {
    g_sun_maps.sun_map_valid = false;
    g_sun_maps.sun2_map_valid = false;
    g_sun_maps.sun3_map_valid = false;
    g_sun_maps.sun4_map_valid = false;
    g_sun_maps.sun5_map_valid = false;
    g_sun_maps.sun_pf_valid[0] = false;
    g_sun_maps.sun_pf_valid[1] = false;
    g_sun_maps.sun_pf_valid[2] = false;   // Band validity flags above.
    g_sun_maps.sun_pf_valid[3] = false;   // FAR_PF.
    for (int i = 0; i < 4; ++i) {   // SUN_TIER_KEY.
        g_sun_setup.sun_pf_fresh[i] = false;
        g_sun_setup.sun_pf_held[i] = false;   // PF_NEED.
        g_sun_setup.sun_tier_key_ok[i] = false;
    }
    g_sun_maps.sun_map_hash = 0;
}
}