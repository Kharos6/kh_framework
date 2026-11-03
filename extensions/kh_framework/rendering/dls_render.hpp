#pragma once

// The feature half (rendering_integration_features.hpp): KH_DLS, the dynamic-light shadow maps: the maps, the DLF
// selection, the fills, the render and the frame. Never include it on its own.
namespace RenderIntegration {
static constexpr UINT KH_DLS_MAP_PX = 1024;

// Stage 2b - the maps. Every property of the sun's depth pass (the shadow LOD, the mesh-run batching, the
// alpha-coverage dither: the per-face keep list is partitioned opaque-first and its alpha tail goes through
// kh_sun_draw_alpha_t under the light's own frame CB - sunOrigin the light, viewProj the face, mirMeta.w zero)
// applies here: it is the sun's version.
static float    g_dls_vp[KH_DLS_MAX][6][16] = {};   // Per slot, per face: world(light-rel) -> clip.
static uint32_t g_dls_facen[KH_DLS_MAX] = {};       // Slices this slot owns; 0 = no map.
// The face -> slice map is not slot*6 + face. Stage 3 must read this table; 0xFFFF = the face has no map, which the
// shader reads as 'nothing occludes in that direction'.
static uint16_t g_dls_faceslice[KH_DLS_MAX][6] = {};
static uint8_t  g_dls_facemask[KH_DLS_MAX] = {};    // Which faces got a slice.
// Named, not inferred: a spot read as a cube would admit every caster in the scene into one face.
static uint8_t  g_dls_isspot[KH_DLS_MAX] = {};      // 1 = single spot frustum, not a cube.
// KH_DLSW_SPOT_REACH - the multiple of a slot's far within which PSDlsWorld can shade. A cube face refuses a receiver
// by its depth along the face axis, at least |p| / sqrt(3); a spot's single frustum refuses by the depth along the
// spot's axis, and its corners (|x|, |y| <= t z, t the tan of the half fov) reach out to far sqrt(1 + 2 t^2) - past
// sqrt(3) once the fov is wider than 90 degrees. From the slot's last rendered frustum (g_dls_vp's column x has
// length 1 / t). TWIN: PSDlsWorld's reach test (static.hlsl).
inline float kh_dls_shade_k(uint32_t khsk_s) {
    float khsk_k2 = 3.0f;
    if (khsk_s < KH_DLS_MAX && g_dls_isspot[khsk_s]) {
        const float* khsk_m = g_dls_vp[khsk_s][0];
        const float khsk_c2 = khsk_m[0] * khsk_m[0] + khsk_m[4] * khsk_m[4] + khsk_m[8] * khsk_m[8];
        if (khsk_c2 > 1.0e-6f) khsk_k2 = fmaxf(khsk_k2, 1.0f + 2.0f / khsk_c2);
    }
    return sqrtf(khsk_k2);
}
static const uint16_t KH_DLS_FACE_NONE = 0xFFFFu;
// A face keeps its slice for KH_DLS_FACE_HOLD frames after its last caster leaves - hysteresis like the distance margin
// KH_DLS_HOLD_SLACK gives the slot table. An empty face costs one clear. KH_DLS_FACE_HOLD_TEXT: the hold is a
// slice-layout latch, not a content hold - the caster's move changes the light's key, and the render pass then clears
// the held face and draws nothing into it (a lit face, as a dropped face reads), so no shadow outlives its caster
// there. What it keeps stable is the face count: a dropped face shifts every later light's slices and keys
// (KH_DLS_KEY_SLICES), re-rendering their maps for a face that flickers in and out of the set mid-motion.
static constexpr uint32_t KH_DLS_FACE_HOLD = 15;
static uint32_t g_dls_face_hold[KH_DLS_MAX][6] = {};
static float    g_dls_mapfar[KH_DLS_MAX] = {};      // The far plane its matrices were built to.

// Grow the depth array to hold khde_want slices. Recreated on growth (a Texture2DArray cannot be extended), so the
// count only climbs. The old array goes before the new one is made, so a create refused on a card with little VRAM
// (48 slices = 192 MB) leaves no DLS maps until one succeeds. KH_DLS_SLICES_MAX is declared in
// rendering/dynamic_light_state.hpp, ahead of Resources, before KH_DLS_MAX exists; KH_EA_DLS_SHAPE checks they agree,
// with the slot bitmask (g_dls_map_valid) and the frame CB's per-face lanes.
static_assert(KH_DLS_SLICES_MAX == static_cast<int>(KH_DLS_MAX) * KH_DLS_FACES && KH_DLS_MAX <= 64u &&
              sizeof(ConstantData::dls_face_slice) ==
                  sizeof(float) * 4u * KH_DLS_MAX * static_cast<size_t>(KH_DLS_FACES) &&
              sizeof(ConstantData::dls_meta) == sizeof(float) * 4u * KH_DLS_MAX,
              "KH_EA_DLS_SHAPE: the slice budget, the slot mask and the frame CB's DLS lanes disagree");

inline bool kh_dls_ensure_maps(ID3D11Device* khde_dev, UINT khde_want) {
    if (!khde_dev || khde_want == 0) return false;
    // Round up to a whole cube: stable across any mask change within a light,
    // and bounds the recreations to eight per session.
    khde_want = ((khde_want + 5u) / 6u) * 6u;
    if (khde_want > static_cast<UINT>(KH_DLS_SLICES_MAX)) khde_want = KH_DLS_SLICES_MAX;
    if (g_res.dls_tex && g_res.dls_slices >= khde_want && g_res.dls_size == KH_DLS_MAP_PX) return true;

    for (int khde_i = 0; khde_i < KH_DLS_SLICES_MAX; ++khde_i) KH_SAFE_RELEASE(g_res.dls_dsv[khde_i]);
    KH_SAFE_RELEASE(g_res.dls_srv);
    KH_SAFE_RELEASE(g_res.dls_tex);
    g_res.dls_slices = 0;
    g_res.dls_size = 0;

    D3D11_TEXTURE2D_DESC khde_td = {};
    khde_td.Width = KH_DLS_MAP_PX;
    khde_td.Height = KH_DLS_MAP_PX;
    khde_td.MipLevels = 1;
    khde_td.ArraySize = khde_want;
    khde_td.Format = DXGI_FORMAT_R32_TYPELESS;   // d32 for the DSV, R32F for the SRV.
    khde_td.SampleDesc.Count = 1;
    khde_td.Usage = D3D11_USAGE_DEFAULT;
    khde_td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(khde_dev->CreateTexture2D(&khde_td, nullptr, &g_res.dls_tex))) return false;

    for (UINT khde_s = 0; khde_s < khde_want; ++khde_s) {
        D3D11_DEPTH_STENCIL_VIEW_DESC khde_dd = {};
        khde_dd.Format = DXGI_FORMAT_D32_FLOAT;
        khde_dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        khde_dd.Texture2DArray.FirstArraySlice = khde_s;
        khde_dd.Texture2DArray.ArraySize = 1;
        if (FAILED(khde_dev->CreateDepthStencilView(g_res.dls_tex, &khde_dd, &g_res.dls_dsv[khde_s]))) {
            for (UINT khde_k = 0; khde_k < khde_s; ++khde_k) KH_SAFE_RELEASE(g_res.dls_dsv[khde_k]);
            KH_SAFE_RELEASE(g_res.dls_tex);
            return false;
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC khde_sd = {};
    khde_sd.Format = DXGI_FORMAT_R32_FLOAT;
    khde_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    khde_sd.Texture2DArray.MipLevels = 1;
    khde_sd.Texture2DArray.ArraySize = khde_want;
    if (FAILED(khde_dev->CreateShaderResourceView(g_res.dls_tex, &khde_sd, &g_res.dls_srv))) {
        for (UINT khde_k = 0; khde_k < khde_want; ++khde_k) KH_SAFE_RELEASE(g_res.dls_dsv[khde_k]);
        KH_SAFE_RELEASE(g_res.dls_tex);
        return false;
    }

    g_res.dls_slices = khde_want;
    g_res.dls_size = KH_DLS_MAP_PX;
    // Every skip key dies with the texture: a key that outlived it would let a light skip and sample uninitialised
    // depth.
    for (uint32_t khde_k = 0; khde_k < KH_DLS_MAX; ++khde_k) g_dls_keys[khde_k] = 0;
    return true;
}

// Publish the maps to the shader. Filled only through kh_dls_fill_cb (below), from the mesh frame-CB sites, the world
// pass and the mask cast: the zeroed default dls_meta[i].w == 0 reads as 'no map, answer lit'. The face -> slice
// table is g_dls_faceslice, not slot*6+face: faces are allocated only where a caster stands, so the mapping is
// carried, not computed.
static constexpr float KH_DLS_BIAS_M = 0.01f;      // Constant compare bias (m).
static constexpr float KH_DLS_BIAS_SLOPE = 0.0001f;   // Plus this per metre of range.
// D3D11 silently unbinds a shader resource whose texture is bound as a depth target - which our own render pass does to
// this array. If the mesh bind site already ran this frame, t36 is left null: the readers' GetDimensions guard
// (KhDlsShadow, KhDlfDlsVis) then answers lit - the shadow is lost, never inverted.

// A slot the shader reads as a map: live, mapped this frame, with faces and a far plane - kh_dls_fill_cb publishes
// dlsMeta.w = the far only for such a slot, and KhDlsShadow / PSDlsWorld's reach read w <= 0 as no map. A light whose
// slot fails this casts nothing the pass can see; it lights the surface like any other light and is packed as one.
// KH_DLS_SRV_GATE: g_dls_map_valid outlives Resources::release (the maps go, the bits stay until the next
// kh_dls_render); with no map view t36 is unbound (the shaders' GetDimensions guard answers lit), so no view = no
// map - the light is packed and lit as an ordinary one, not sent to a lookup that cannot run. Reads
// g_dls_map_valid (written by the render pass), so it must be declared above its readers.
inline bool kh_dls_published(uint32_t khdp_s) {
    return khdp_s < KH_DLS_MAX && g_res.dls_srv && g_dls[khdp_s].live &&
           (g_dls_map_valid & (1ull << khdp_s)) != 0ull &&
           g_dls_facen[khdp_s] > 0 && g_dls_mapfar[khdp_s] > 0.0f;
}
// The world pass's light lanes, filled inside its state bracket (the append binds PS t40), through the light ring
// (KH_DL_RING) like the mesh fill. Records are the engine's own cb11 bytes copied verbatim; the slot lane is written
// after the copy. Points first, then spots. Two kinds: every casting light whose map is published this frame
// (kh_dls_published; slot lane = slot + 1: what our meshes can block, KhDlsWorldFactor's numerator), and every other
// pool light that can reach a pixel the pass shades (slot lane 0: the denominator only). KH_DLSW_DENOM: the factor is
// blocked / everything that lights the pixel; with the casting lights alone the denominator missed every non-casting
// light on the surface and the factor over-darkened, against its contract (every error fails toward
// under-darkening). "Can reach" follows the shader's own gates, widened only: PSDlsWorld shades a pixel only within
// kh_dls_shade_k * 1.05 * far of a published map (its reach test: sqrt(3) for a cube, more for a spot wider than 90
// degrees), and a light adds an exact zero past its hard fade (dist * scale >= fade start + 1 / inverse width); a
// light with no fade (inverse width ~0) has no such bound and is always packed. No published casting light: nothing
// is packed (dl_ctl.x stays 0) and the fill returns false, as on every other stand-down: the factor is 1 everywhere,
// so the caller skips the draw.
inline bool kh_dls_fill_dl_world(ID3D11DeviceContext* khd_ctx, ConstantData& khd_cb) {
    if (g_dl.pool_n == 0) return false;
    if (steady_now_ms() - g_dl.pool_stamp > KH_DL_POOL_STALE_MS) return false;

    const float khd_scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                          ? g_dl.main_scale : 1.0f;
    // The spheres the pass can shade in: every map kh_dls_fill_cb publishes
    // (dlsMeta.w > 0), at PSDlsWorld's reach radius plus a hair.
    float khd_anc[KH_DLS_MAX][4];
    uint32_t khd_anc_n = 0;
    for (uint32_t khd_s = 0; khd_s < KH_DLS_MAX; ++khd_s) {
        if (!kh_dls_published(khd_s)) continue;
        khd_anc[khd_anc_n][0] = g_dls[khd_s].pos[0];
        khd_anc[khd_anc_n][1] = g_dls[khd_s].pos[1];
        khd_anc[khd_anc_n][2] = g_dls[khd_s].pos[2];
        khd_anc[khd_anc_n][3] = kh_dls_shade_k(khd_s) * 1.05f * g_dls_mapfar[khd_s] * 1.001f;   // KH_DLSW_SPOT_REACH.
        khd_anc_n++;
    }

    static thread_local std::vector<float> khd_rec;   // Scratch.
    khd_rec.clear();
    uint32_t khd_out = 0, khd_pn = 0, khd_cast = 0;
    for (int khd_pass = 0; khd_pass < 2; ++khd_pass) {
        for (uint32_t khd_i = 0; khd_i < g_dl.pool_n; ++khd_i) {
            const DlPoolLight& khd_pl = g_dl.pool[khd_i];
            if ((khd_pl.spot != 0) != (khd_pass == 1)) continue;
            const int khd_slot = kh_dls_slot_of(khd_pl);
            const bool khd_casts = khd_slot >= 0 && kh_dls_published(static_cast<uint32_t>(khd_slot));
            if (!khd_casts) {
                // Denominator only: packed if it can light a pixel the pass shades.
                const float khd_inv = khd_pl.rec[21];
                bool khd_reach = !(khd_inv > 1.0e-6f);   // No hard fade: unbounded.
                if (!khd_reach) {
                    const float khd_r = (khd_pl.rec[20] + 1.0f / khd_inv) / khd_scale * 1.001f;
                    for (uint32_t khd_a = 0; khd_a < khd_anc_n && !khd_reach; ++khd_a) {
                        const float khd_dx = khd_pl.rec[0] - khd_anc[khd_a][0];
                        const float khd_dy = khd_pl.rec[1] - khd_anc[khd_a][1];
                        const float khd_dz = khd_pl.rec[2] - khd_anc[khd_a][2];
                        const float khd_rr = khd_anc[khd_a][3] + khd_r;
                        khd_reach = khd_dx * khd_dx + khd_dy * khd_dy + khd_dz * khd_dz < khd_rr * khd_rr;
                    }
                }
                if (!khd_reach) continue;
            }
            khd_rec.insert(khd_rec.end(), khd_pl.rec, khd_pl.rec + 24);
            khd_rec[static_cast<size_t>(khd_out) * 24u + 22u] = khd_casts ? static_cast<float>(khd_slot + 1) : 0.0f;
            khd_out++;
            if (khd_pass == 0) khd_pn++;
            if (khd_casts) khd_cast++;
        }
    }
    if (khd_cast == 0) return false;   // Nothing can block: dl_ctl.x stays 0 and the kernel stands down.
    uint32_t khd_first = 0;
    if (!kh_dlr_append(khd_ctx, khd_rec.data(), khd_out * 6u, khd_first)) {
        kh_stat(g_stats.dl_ring_fails);   // KH_DL_DIAG.
        return false;   // Lit: the kernel stands down.
    }
    memcpy(&khd_cb.dl_first[0], &khd_first, sizeof(khd_first));
    khd_cb.dl_first[1] = 0.0f;
    khd_cb.dl_first[2] = 0.0f;
    khd_cb.dl_first[3] = 0.0f;
    // The mesh fill's own control and global lanes (fill_dynlights_cb): mode 3, the split, the distance scale, the
    // main pass's diffuse multiplier and the global intensity - one writer, so the two cannot drift.
    kh_dl_fill_ctl(khd_cb, khd_out, khd_pn, khd_scale);
    return true;
}
// KH_DLF - dynamicLightFog's lights (effect.hlsl's KH_DLF note). A fresh pool (kh_dl_select's staleness rule)
// is the chain loop's skip test: without one the pass would add nothing.
inline bool kh_dlf_pool_live() {
    return g_dl.pool_n != 0 && steady_now_ms() - g_dl.pool_stamp <= KH_DL_POOL_STALE_MS;
}
// Every pool light whose reach comes within the pass's maxDistance of the camera, nearest reach first, at most
// maxLights, packed points first then spots as the shader's pointN split reads them. Each is the engine's cb11 record
// as kh_dl_pack copies it, with [0..2] made relative to the pass's camera (fxCam: the frame the shader reconstructs in;
// the subtraction in double), [5].z = our light map's slot + 1 when the pass takes our meshes' shadows (shadows >= 1)
// and the map is published this frame (kh_dls_published), else 0, and [5].w = the reach in metres: the nearest of the
// hard fade (the shader's fade term is zero past it), the distance where the light's brightness under its attenuation -
// and the pass's intensity and tint, which scale what it adds - falls to KH_DLF_EPS, and the camera's distance plus
// maxDistance (the gather marches no farther from the camera). A record with a lane that is not finite is left out (one
// would blank every light's fog where its reach spans). Returns the count; khdl_pn = the points among them, khdl_scale
// = the distance scale their attenuation takes (kh_dl_select's). Render thread or a park (g_dl's fill-site rule);
// KH_FX_LATE's chain-only flush may also run it on the game thread, inside a resolve the engine issues there - under
// that route's premise that the engine never drives its context from both threads at once, which holds g_dl's writers
// (the harvests, inside the render thread's context calls and flush) off it. Empty: no fresh pool, an unarmed camera
// (fxCam.w), or no light bright and near enough. KH_DLF_EPS bounds the light's colour x attenuation alone, not what a
// sample adds - that is also x sigma x pi p_HG, whose forward peak is (1 + g) / (1 - g)^2 times isotropic's: looking
// into a lamp the cut drops the fog between the camera and its reach sphere - up to 2 % of the halo at anisotropy 0,
// 3.5 - 5 % at the default 0.6, 13 - 19 % at 0.9 (lamps 50 - 150 m off), read as a smaller, dimmer halo, not an edge.
// Folding the peak into the reach recovers it but triples the reach of a quadratic-falloff lamp at 0.6 (up to x14 at
// 0.9): 2.4 - 11 times the light-texel pairs the gather marches at 0.6, 3 - 87 times at 0.9.
static constexpr float KH_DLF_EPS = 2.0e-3f;   // Scene units: colour x attenuation below this ends a light's reach.
inline uint32_t kh_dlf_select(const ConstantData& khdl_cb, std::vector<float>& khdl_out, uint32_t& khdl_pn,
                              float& khdl_scale) {
    khdl_out.clear();
    khdl_pn = 0;
    khdl_scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f) ? g_dl.main_scale : 1.0f;
    if (!kh_dlf_pool_live() || !(khdl_cb.fx_cam[3] >= 0.5f)) return 0;
    // The pass's params, stored finite (set_effect_params), are clamped where read: maxDistance 10 - 5000 m and
    // maxLights 1 - 64 here; steps 4 - 64 (effect.hlsl's gather), intensity 0 or more (the gather), anisotropy
    // -0.9 - 0.9 (KhDlfPhase) and fogScale and baseDensity 0 or more (KhDlfSigma; effect.hlsl). KH_DLF_COST: the work
    // grows as lights x steps x the grid's texels (setSsgiScale), shadows adding to every step. At the maxima together
    // (64 lights in reach, 64 steps, setSsgiScale 2, shadows 2) it is estimated - not measured - at a second or more a
    // frame on a mid-range GPU, in ONE draw; a draw of about two seconds makes Windows reset the driver, which the game
    // does not survive. Widen no range here without re-measuring that.
    const float khdl_maxd = fminf(fmaxf(khdl_cb.fx1[1], 10.0f), 5000.0f);
    const uint32_t khdl_max = static_cast<uint32_t>(fminf(fmaxf(khdl_cb.fx1[3], 1.0f), 64.0f));
    const bool khdl_shadow = khdl_cb.fx1[2] >= 0.5f;
    // The brightness the shader gives a light (kh_dl_fill_ctl's global tint and intensity).
    float khdl_gd[3] = { 1.0f, 1.0f, 1.0f };
    if (g_dl.main_ref_valid &&
        (g_dl.main_gdiff[0] > 0.0f || g_dl.main_gdiff[1] > 0.0f || g_dl.main_gdiff[2] > 0.0f)) {
        for (int khdl_c = 0; khdl_c < 3; ++khdl_c) khdl_gd[khdl_c] = g_dl.main_gdiff[khdl_c];
    }
    // The pass's own gain on a light: intensity (fx0.x) times the tint's strongest channel.
    const float khdl_gi = kh_dl_intensity() * fmaxf(khdl_cb.fx0[0], 0.0f) *
                          fmaxf(fmaxf(khdl_cb.color[0], khdl_cb.color[1]), fmaxf(khdl_cb.color[2], 0.0f));
    struct KhDlfCand { float key; float reach; float rel[3]; uint32_t i; float dist; float cut; };
    static thread_local std::vector<KhDlfCand> khdl_cand;   // Scratch.
    khdl_cand.clear();
    for (uint32_t khdl_i = 0; khdl_i < g_dl.pool_n; ++khdl_i) {
        const float* const khdl_r = g_dl.pool[khdl_i].rec;
        bool khdl_fin = true;
        for (int khdl_a = 0; khdl_a < 22; ++khdl_a) khdl_fin = khdl_fin && std::isfinite(khdl_r[khdl_a]);
        if (!khdl_fin) continue;
        KhDlfCand khdl_e;
        double khdl_dd = 0.0;
        for (int khdl_a = 0; khdl_a < 3; ++khdl_a) {
            khdl_e.rel[khdl_a] = static_cast<float>(static_cast<double>(khdl_r[khdl_a]) -
                                                    static_cast<double>(khdl_cb.fx_cam[khdl_a]));
            khdl_dd += static_cast<double>(khdl_e.rel[khdl_a]) * static_cast<double>(khdl_e.rel[khdl_a]);
        }
        const float khdl_dist = static_cast<float>(std::sqrt(khdl_dd));
        if (!std::isfinite(khdl_dist)) continue;
        // Its brightness at attenuation 1: DynLights' combine without the N.L, its brightest channel, times the
        // pass's gain.
        float khdl_peak = 0.0f;
        for (int khdl_c = 0; khdl_c < 3; ++khdl_c) {
            khdl_peak = fmaxf(khdl_peak, (khdl_gd[khdl_c] * khdl_r[8 + khdl_c] + khdl_r[12 + khdl_c]) * khdl_gi);
        }
        if (!(khdl_peak > KH_DLF_EPS)) continue;
        const float khdl_open = khdl_dist + khdl_maxd;   // KH_DLF_TIE: the reach maxDistance alone allows.
        float khdl_reach = khdl_open;
        if (khdl_r[21] > 1.0e-6f) {   // The hard fade (kh_dl_select's reach).
            khdl_reach = fminf(khdl_reach, (khdl_r[20] + 1.0f / khdl_r[21]) / khdl_scale);
        }
        // The attenuation's reach: 1 / (c + l x + q x^2) falls to KH_DLF_EPS / peak at x = d * scale - start. A light
        // whose constant term alone is past it never shows; negative terms (no engine light has them) leave the
        // reach to the fade and maxDistance.
        const float khdl_k = khdl_peak / KH_DLF_EPS;
        const float khdl_c0 = khdl_r[17], khdl_l = khdl_r[18], khdl_q = khdl_r[19];
        if (!(khdl_k > khdl_c0)) continue;
        if (khdl_l >= 0.0f && khdl_q >= 0.0f) {
            // The positive root of q x^2 + l x - (k - c) in the form that keeps a tiny q from cancelling it away.
            const float khdl_den = khdl_l + std::sqrt(khdl_l * khdl_l + 4.0f * khdl_q * (khdl_k - khdl_c0));
            if (khdl_den > 1.0e-12f) {   // Else unbounded.
                const float khdl_x = 2.0f * (khdl_k - khdl_c0) / khdl_den;
                if (std::isfinite(khdl_x)) {
                    khdl_reach = fminf(khdl_reach, (fmaxf(khdl_r[16], 0.0f) + khdl_x) / khdl_scale);
                }
            }
        }
        if (!(khdl_reach > 0.0f) || !std::isfinite(khdl_reach)) continue;
        if (khdl_dist - khdl_reach > khdl_maxd) continue;
        khdl_e.key = khdl_dist - khdl_reach;
        khdl_e.reach = khdl_reach;
        khdl_e.i = khdl_i;
        khdl_e.dist = khdl_dist;
        khdl_e.cut = khdl_reach == khdl_open ? -khdl_maxd : khdl_e.key;   // KH_DLF_TIE (below).
        khdl_cand.push_back(khdl_e);
    }
    std::stable_sort(khdl_cand.begin(), khdl_cand.end(),
                     [](const KhDlfCand& khdl_a, const KhDlfCand& khdl_b) { return khdl_a.key < khdl_b.key; });
    if (khdl_cand.size() > khdl_max) {
        // KH_DLF_TIE: every light whose reach runs past maxDistance has its reach clamped to dist + maxDistance, so
        // its key is -maxDistance give or take the add's rounding - a tie among all of them, which pool order and
        // that rounding broke: with more of them than maxLights the lamp beside the camera could lose its glow to a
        // farther one. The cut ranks such a light at exactly -maxDistance (cut) and a tie nearest first; the lights
        // kept go back in the order above (key, then pool order) for the draw.
        std::stable_sort(khdl_cand.begin(), khdl_cand.end(), [](const KhDlfCand& khdl_a, const KhDlfCand& khdl_b) {
            return khdl_a.cut < khdl_b.cut || (khdl_a.cut == khdl_b.cut && khdl_a.dist < khdl_b.dist);
        });
        khdl_cand.resize(khdl_max);
        std::stable_sort(khdl_cand.begin(), khdl_cand.end(), [](const KhDlfCand& khdl_a, const KhDlfCand& khdl_b) {
            return khdl_a.key < khdl_b.key || (khdl_a.key == khdl_b.key && khdl_a.i < khdl_b.i);
        });
    }
    std::stable_partition(khdl_cand.begin(), khdl_cand.end(),
                          [](const KhDlfCand& khdl_a) { return g_dl.pool[khdl_a.i].spot == 0; });
    khdl_out.resize(khdl_cand.size() * 24u);
    for (size_t khdl_j = 0; khdl_j < khdl_cand.size(); ++khdl_j) {
        const KhDlfCand& khdl_e = khdl_cand[khdl_j];
        const DlPoolLight& khdl_pl = g_dl.pool[khdl_e.i];
        float* const khdl_o = &khdl_out[khdl_j * 24u];
        memcpy(khdl_o, khdl_pl.rec, KH_DL_LIGHT_BYTES);
        khdl_o[0] = khdl_e.rel[0];
        khdl_o[1] = khdl_e.rel[1];
        khdl_o[2] = khdl_e.rel[2];
        const int khdl_s = khdl_shadow ? kh_dls_slot_of(khdl_pl) : -1;
        khdl_o[22] = (khdl_s >= 0 && kh_dls_published(static_cast<uint32_t>(khdl_s)))
                   ? static_cast<float>(khdl_s + 1) : 0.0f;
        if (khdl_s >= 0 && khdl_o[22] == 0.0f) kh_stat(g_stats.dlf_unshadowed);   // KH_DL_DIAG (CN-4).
        khdl_o[23] = khdl_e.reach;
        if (khdl_pl.spot == 0) ++khdl_pn;
    }
    return static_cast<uint32_t>(khdl_cand.size());
}
inline void kh_dls_fill_cb(ConstantData& khf_cb) {
    // One fill for every consumer (both mesh passes, the mask cast, the world receive). The clamp is every other
    // g_sun_range consumer's; the camera is the cycle's; the world pass overrides xyz with its own reconstruction
    // camera after this call.
    {
        float khf_cam[3];
        if (kh_dls_camera(khf_cam)) {
            khf_cb.dls_range[0] = khf_cam[0];
            khf_cb.dls_range[1] = khf_cam[1];
            khf_cb.dls_range[2] = khf_cam[2];
            khf_cb.dls_range[3] = fminf(fmaxf(
                g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
        } else {
            khf_cb.dls_range[3] = 0.0f;
        }
    }
    // dlsFaceSlice[].y is our own lane and this fill is its only writer, so the mesh path's arm lives there.
    for (uint32_t khf_s = 0; khf_s < KH_DLS_MAX; ++khf_s) {
        // Publishable only if live and mapped this frame: a light selected but not rendered (slice budget, failed
        // upload) publishes w = 0 and reads as lit rather than sampling another light's slice. The world pass's and
        // the fog's own test (kh_dls_published, KH_DLS_SRV_GATE included); the mesh pack's lane reads it through w.
        if (!kh_dls_published(khf_s)) {
            khf_cb.dls_meta[khf_s][3] = 0.0f;
            for (int khf_z = 0; khf_z < 6; ++khf_z) {
                khf_cb.dls_face_slice[static_cast<int>(khf_s) * 6 + khf_z][0] = -1.0f;
            }
            continue;
        }

        khf_cb.dls_meta[khf_s][0] = g_dls[khf_s].pos[0];
        khf_cb.dls_meta[khf_s][1] = g_dls[khf_s].pos[1];
        khf_cb.dls_meta[khf_s][2] = g_dls[khf_s].pos[2];
        khf_cb.dls_meta[khf_s][3] = g_dls_mapfar[khf_s];

        khf_cb.dls_ctl[khf_s][0] = g_dls_isspot[khf_s] ? 1.0f : 0.0f;
        khf_cb.dls_ctl[khf_s][1] = KH_DLS_BIAS_M;
        khf_cb.dls_ctl[khf_s][2] = g_dls_isspot[khf_s]
                                 ? static_cast<float>(g_dls_faceslice[khf_s][0]) : 0.0f;
        khf_cb.dls_ctl[khf_s][3] = KH_DLS_BIAS_SLOPE;

        // slot * 6 + face, one float4 each. The arm is replicated across all six: the shader reads it before it
        // knows which face it wants.
        const float khf_arm = 1.0f;
        for (int khf_f = 0; khf_f < 6; ++khf_f) {
            const uint16_t khf_sl = g_dls_faceslice[khf_s][khf_f];
            const int khf_i = static_cast<int>(khf_s) * 6 + khf_f;
            khf_cb.dls_face_slice[khf_i][0] = (khf_sl == KH_DLS_FACE_NONE)
                                            ? -1.0f : static_cast<float>(khf_sl);
            khf_cb.dls_face_slice[khf_i][1] = khf_arm;
        }

        if (g_dls_isspot[khf_s]) {
            memcpy(khf_cb.dls_spot_vp[khf_s], g_dls_vp[khf_s][0],
                   sizeof(khf_cb.dls_spot_vp[khf_s]));
        }
    }
}

// khdr_owners (KH_SHADOW_SWITCH): meshes that cast nothing but own pixels - the mask's alone, never a map's.
inline void kh_dls_render(ID3D11DeviceContext* khdr_ctx,
                          const std::vector<SunCaster>& khdr_casters,
                          const std::vector<SunCaster>& khdr_owners) {
    g_dls_map_valid = 0;
    // The caster-set hash the per-light skip keys on.
    uint64_t khdr_chash = CryptoGenerator::FNV1A64_OFFSET;
    bool khdr_tex_alpha = false;   // KH_TEX_LAND: a caster draws through its material's maps.
    bool khdr_uv_anim = false;   // KH_UV_ANIM: an alpha caster's coverage moves with the clock.
    for (const SunCaster& khdl_c : khdr_casters) {
        khdr_tex_alpha |= khdl_c.alpha_caster && khdl_c.mats;
        khdr_uv_anim |= khdl_c.alpha_caster && khdl_c.mats && khdl_c.mats->uv_anim_alpha;
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, khdl_c.pos, sizeof(khdl_c.pos));
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, khdl_c.res, sizeof(khdl_c.res));   // KH_POS_RES.
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, khdl_c.size, sizeof(khdl_c.size));
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, khdl_c.rot, sizeof(khdl_c.rot));
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_c.mesh, sizeof(khdl_c.mesh));
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_c.alpha, sizeof(khdl_c.alpha));
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_c.lod, sizeof(khdl_c.lod));   // KH_SHADOW_LOD_DRAWN.
        // The map depends on the material and the verdict, as the union hash does.
        const uint64_t khdl_mh = khdl_c.mats ? khdl_c.mats->hash : 0ull;
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_mh, sizeof(khdl_mh));
        const uint8_t khdl_ac = khdl_c.alpha_caster ? 1 : 0;
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_ac, 1);
        // KH_CLOTH: the shape key, as the sun maps hash it.
        const uint32_t khdl_cg = kh_cloth_gen_of(khdl_c.slot);
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdl_cg, sizeof(khdl_cg));
    }
    if (khdr_tex_alpha) {   // KH_TEX_LAND: a map landing redraws, as the union hash does.
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &g_tex_land_serial, sizeof(g_tex_land_serial));
    }
    if (khdr_uv_anim) {   // KH_UV_ANIM: the union hash's rule.
        const uint64_t khdr_ac = kh_mat_anim_cycle_key();
        khdr_chash = CryptoGenerator::fnv1a64_update(khdr_chash, &khdr_ac, sizeof(khdr_ac));
    }
    {
        g_dlsw_casters.clear();
        g_dlsw_casters.reserve(khdr_casters.size() + khdr_owners.size());
        for (const std::vector<SunCaster>* khdr_list : { &khdr_casters, &khdr_owners })   // KH_SHADOW_SWITCH.
        for (const SunCaster& khsc : *khdr_list) {
            g_dlsw_casters.emplace_back();
            KhDlswCaster& khsd = g_dlsw_casters.back();
            memcpy(khsd.pos, khsc.pos, sizeof(khsd.pos));
            memcpy(khsd.res, khsc.res, sizeof(khsd.res));   // KH_POS_RES.
            memcpy(khsd.size, khsc.size, sizeof(khsd.size));
            memcpy(khsd.rot, khsc.rot, sizeof(khsd.rot));
            khsd.rotated = khsc.rotated;
            khsd.mesh = khsc.mesh;
            khsd.slot = khsc.slot;   // KH_CLOTH: carried so the mask binds the same buffer the colour pass does.
            khsd.lod_lock = khsc.lod_lock;
            khsd.lod_r = khsc.lod_r;   // KH_SKEL_LOD_R.
            khsd.visible = khsc.visible;   // KH_DLSW_MASK_ALPHA: the mask's own four lanes.
            khsd.alpha = khsc.alpha;
            khsd.mats = khsc.mats;
            khsd.alpha_caster = khsc.alpha_caster;
            khsd.in_front = khsc.in_front;   // KH_INFRONT.
            khsd.no_foot = khsc.no_foot;   // KH_FOOT_BLEND.
        }
    }
    if (!khdr_ctx || g_dls_n == 0) {  return; }

    // Resolve first, budget second: every light's face count and matrices up front, then allocate to the total.
    UINT khdr_need = 0;
    for (uint32_t khdr_s = 0; khdr_s < KH_DLS_MAX; ++khdr_s) {
        g_dls_facen[khdr_s] = 0;
        g_dls_facemask[khdr_s] = 0;
        g_dls_isspot[khdr_s] = 0;
        for (int khdr_z = 0; khdr_z < KH_DLS_FACES; ++khdr_z) {
            g_dls_faceslice[khdr_s][khdr_z] = KH_DLS_FACE_NONE;
        }
        if (!g_dls[khdr_s].live) continue;
        const KhDlsSlot& khdr_l = g_dls[khdr_s];
        const float khdr_far = kh_dls_far(khdr_l);
        uint32_t khdr_fn = 0;

        if (khdr_l.spot) {
            // id first, then the nearest pool entry that is a spot.
            const DlPoolLight* khdr_src = nullptr;
            if (khdr_l.light_id != 0) {
                for (uint32_t khdr_p = 0; khdr_p < g_dl.pool_n; ++khdr_p) {
                    if (g_dl.pool[khdr_p].id == khdr_l.light_id) { khdr_src = &g_dl.pool[khdr_p]; break; }
                }
            }
            if (!khdr_src) {
                float khdr_sd = 0.0f;
                for (uint32_t khdr_p = 0; khdr_p < g_dl.pool_n; ++khdr_p) {
                    const DlPoolLight& khdr_c = g_dl.pool[khdr_p];
                    const float khdr_dx = khdr_c.rec[0] - khdr_l.pos[0];
                    const float khdr_dy = khdr_c.rec[1] - khdr_l.pos[1];
                    const float khdr_dz = khdr_c.rec[2] - khdr_l.pos[2];
                    const float khdr_d2 = khdr_dx * khdr_dx + khdr_dy * khdr_dy + khdr_dz * khdr_dz;
                    if (khdr_d2 > KH_DLS_MATCH_M * KH_DLS_MATCH_M) continue;
                    if (!khdr_c.spot) continue;   // A point record can never be a spot slot's source.
                    if (!khdr_src || khdr_d2 < khdr_sd) { khdr_src = &khdr_c; khdr_sd = khdr_d2; }
                }
            }
            if (khdr_src) {
                const float khdr_cc = khdr_src->rec[7];
                float khdr_fov = 0.0f;
                if (kh_dls_spot_vp(khdr_src->rec + 4, khdr_cc, khdr_far,
                                   g_dls_vp[khdr_s][0], &khdr_fov)) {
                    khdr_fn = 1;
                }
            }
        }
        if (khdr_fn == 1) {
            g_dls_facemask[khdr_s] = 1u;   // The spot's single frustum.
            g_dls_isspot[khdr_s] = 1;
        } else {
            // Cube: build all six matrices, then keep only the faces that contain a caster. The test is
            // kh_dls_face_keep - the render pass's own - so allocation and draw cannot disagree.
            uint8_t khdr_mask = 0;
            for (int khdr_f = 0; khdr_f < KH_DLS_FACES; ++khdr_f) {
                kh_dls_face_vp(khdr_f, khdr_far, g_dls_vp[khdr_s][khdr_f]);
                bool khdr_any = false;
                for (const SunCaster& khdr_c : khdr_casters) {
                    const float khdr_ctr[3] = { khdr_c.pos[0], khdr_c.pos[2], khdr_c.pos[1] };
                    const float khdr_hl[3] = { khdr_c.bsize[0] * 0.5f, khdr_c.bsize[2] * 0.5f,
                                               khdr_c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
                    float khdr_he[3];
                    kh_rot_half_extents(khdr_hl, khdr_c.rot, khdr_c.rotated, khdr_he);
                    if (kh_dls_face_keep(khdr_f, g_dls[khdr_s].pos, khdr_ctr, khdr_he, khdr_far)) {
                        khdr_any = true;
                        break;
                    }
                }
                if (khdr_any) {
                    khdr_mask |= static_cast<uint8_t>(1u << khdr_f);
                    g_dls_face_hold[khdr_s][khdr_f] = KH_DLS_FACE_HOLD;
                } else if (g_dls_face_hold[khdr_s][khdr_f] > 0) {
                    // Keep the face in the mask so a caster crossing the face boundary does not shift the later
                    // lights' slices (KH_DLS_FACE_HOLD_TEXT: the face itself is cleared on the key change the move
                    // causes - a lit face, not a kept shadow).
                    g_dls_face_hold[khdr_s][khdr_f]--;
                    khdr_mask |= static_cast<uint8_t>(1u << khdr_f);
                }
            }
            g_dls_facemask[khdr_s] = khdr_mask;
            khdr_fn = 0;
            for (int khdr_f = 0; khdr_f < KH_DLS_FACES; ++khdr_f) {
                if (khdr_mask & (1u << khdr_f)) khdr_fn++;
            }
        }
        g_dls_facen[khdr_s] = khdr_fn;
        g_dls_mapfar[khdr_s] = khdr_far;
        khdr_need += khdr_fn;
    }
    if (khdr_need == 0) {  return; }   // Before the bracket opens.

    ID3D11Device* khdr_dev = nullptr;
    khdr_ctx->GetDevice(&khdr_dev);
    if (!khdr_dev) return;
    const bool khdr_ok = kh_dls_ensure_maps(khdr_dev, khdr_need);
    khdr_dev->Release();
    if (!khdr_ok) {  return; }

    // KH_DLS_DEPTHCLIP: the sun's tiers bind DepthClipEnable = false twins on their orthographic maps (the union
    // clips). On a perspective map that flag is destructive: geometry nearer than the near plane clamps to depth 0
    // and occludes everything behind it.
    KhOmSave                khdr_om;   // Full OM set in, full OM set out (UAVs carried; self-restoring).
    D3D11_VIEWPORT          khdr_prev_vp[16] = {};
    UINT                    khdr_prev_nvp = 16;
    khdr_om.capture(khdr_ctx);
    khdr_ctx->RSGetViewports(&khdr_prev_nvp, khdr_prev_vp);
    ID3D11RasterizerState* khdr_prev_rs = nullptr;
    if (!g_res.dls_rast) {
        ID3D11Device* khdr_rd = nullptr;
        khdr_ctx->GetDevice(&khdr_rd);
        if (khdr_rd) {
            D3D11_RASTERIZER_DESC khdr_rsd = {};
            khdr_rsd.FillMode = D3D11_FILL_SOLID;
            khdr_rsd.CullMode = D3D11_CULL_NONE;   // As the sun: thin and alpha casters.
            khdr_rsd.DepthClipEnable = TRUE;
            khdr_rsd.DepthBias = 0;                // The bias is done in metres, in-shader.
            khdr_rsd.SlopeScaledDepthBias = 0.0f;
            khdr_rsd.DepthBiasClamp = 0.0f;
            khdr_rsd.MultisampleEnable = FALSE;
            if (FAILED(khdr_rd->CreateRasterizerState(&khdr_rsd, &g_res.dls_rast))) {
                g_res.dls_rast = nullptr;
            }
            khdr_rd->Release();
        }
    }
    if (g_res.dls_rast) {
        khdr_ctx->RSGetState(&khdr_prev_rs);   // The sun's, restored below.
        khdr_ctx->RSSetState(g_res.dls_rast);
    }

    UINT khdr_slice = 0;
    for (uint32_t khdr_s = 0; khdr_s < KH_DLS_MAX; ++khdr_s) {
        // KH_DLS_KEY_SLICES: a key names the slices its light rendered, which keep that content only while no other
        // light is laid onto them. A slot that renders nothing this pass (not live, no face, past the budget) gives
        // its slices up to the slots after it, which shift onto them, so its key goes: back, it renders afresh
        // rather than reusing a slice another light has drawn over.
        if (!g_dls[khdr_s].live) { g_dls_keys[khdr_s] = 0; continue; }
        const KhDlsSlot& khdr_l = g_dls[khdr_s];
        const uint32_t khdr_fn = g_dls_facen[khdr_s];
        const float khdr_far = g_dls_mapfar[khdr_s];
        if (khdr_fn == 0) { g_dls_keys[khdr_s] = 0; continue; }
        if (khdr_slice + khdr_fn > g_res.dls_slices) {
            for (uint32_t khdr_r = khdr_s; khdr_r < KH_DLS_MAX; ++khdr_r) g_dls_keys[khdr_r] = 0;
            break;
        }

        // Union-style skip: a caster or the light moving changes the key and forces a rebuild. Skipping reuses the
        // slices, so the matrices must be republished either way - they are state, not output.
        const uint64_t khdr_key = kh_dls_light_hash(khdr_l, khdr_fn, khdr_slice,
                                                    g_dls_facemask[khdr_s], khdr_chash,
                                                    g_dls_isspot[khdr_s] ? g_dls_vp[khdr_s][0] : nullptr);
        const bool khdr_reuse = (khdr_key == g_dls_keys[khdr_s]);

        g_dls_map_valid |= (1ull << khdr_s);

        if (khdr_reuse) {
            // The maps are reused, but the face -> slice table is state every consumer reads: republished on the
            // skip path too.
            UINT khdr_rs = khdr_slice;
            for (uint32_t khdr_f = 0; khdr_f < KH_DLS_FACES; ++khdr_f) {
                if (g_dls_facemask[khdr_s] & (1u << khdr_f)) {
                    g_dls_faceslice[khdr_s][khdr_f] = static_cast<uint16_t>(khdr_rs++);
                }
            }
            khdr_slice += khdr_fn;
            continue;
        }
        g_dls_keys[khdr_s] = khdr_key;

        UINT khdr_next = khdr_slice;
        bool khdr_fail = false;   // KH_SUN_OBJ: a face left incomplete - the key is not committed.
        for (uint32_t khdr_f = 0; khdr_f < KH_DLS_FACES; ++khdr_f) {
            if (!(g_dls_facemask[khdr_s] & (1u << khdr_f))) continue;
            const UINT khdr_sl = khdr_next++;
            g_dls_faceslice[khdr_s][khdr_f] = static_cast<uint16_t>(khdr_sl);
            ID3D11DepthStencilView* khdr_dsv = g_res.dls_dsv[khdr_sl];
            if (!khdr_dsv) { khdr_fail = true; continue; }   // KH_EA_DLS_DSV: an undrawn face is an incomplete map.

            // Cull to this face. Partitioned opaque-first: the instanced runs cover [0, khdr_opq) and the alpha tail
            // [khdr_opq, n) draws per caster through the alpha twins; the instance stream is written in this order,
            // so an alpha draw's start-instance lands on its own lane.
            static std::vector<const SunCaster*> khdr_keep;
            khdr_keep.clear();
            size_t khdr_opq = 0;
            for (int khdr_part = 0; khdr_part < 2; ++khdr_part) {
                for (const SunCaster& khdr_c : khdr_casters) {
                    if ((khdr_part == 0) == khdr_c.alpha_caster) continue;
                    const float khdr_ctr[3] = { khdr_c.pos[0], khdr_c.pos[2], khdr_c.pos[1] };
                    const float khdr_hl[3] = { khdr_c.bsize[0] * 0.5f, khdr_c.bsize[2] * 0.5f,
                                               khdr_c.bsize[1] * 0.5f };   // KH_CAST_REFIT.
                    float khdr_he[3];
                    kh_rot_half_extents(khdr_hl, khdr_c.rot, khdr_c.rotated, khdr_he);
                    const bool khdr_in = g_dls_isspot[khdr_s]
                        ? true   // A spot is one frustum: every caster the reach walk admitted (any light's).
                        : kh_dls_face_keep(static_cast<int>(khdr_f), khdr_l.pos,
                                           khdr_ctr, khdr_he, khdr_far);
                    if (khdr_in) khdr_keep.push_back(&khdr_c);
                }
                if (khdr_part == 0) khdr_opq = khdr_keep.size();
            }
            // KH_DLS_RUN_SORT: the opaque partition by (mesh, level), the sun tiers' order - the runs below merge
            // neighbours only, and the grid walk that built the list interleaves meshes, so unsorted, most runs would
            // be one caster (a vertex and index buffer bind and a draw each). The face is depth-only (no colour target,
            // a null pixel shader, a depth test that writes): each texel ends at its nearest fragment whatever the
            // order. Each lane is written at its caster's place in the sorted list, so a run's start instance still
            // names its own casters; the alpha tail [khdr_opq, n) keeps its place, order and lanes. KH_SKIN_POOL: with
            // a chunk installed, a chunk's casters together within their (mesh, level), for its runs.
            if (g_skin_pool_any) {
                for (size_t khdr_k = 0; khdr_k < khdr_opq; ++khdr_k) kh_cast_key_put(khdr_keep[khdr_k]->mesh,
                                                                                     khdr_keep[khdr_k]->slot);
                std::sort(khdr_keep.begin(), khdr_keep.begin() + static_cast<std::ptrdiff_t>(khdr_opq),
                          [](const SunCaster* a, const SunCaster* b) {
                              if (a->mesh != b->mesh) return a->mesh < b->mesh;
                              if (a->lod != b->lod) return a->lod < b->lod;
                              return kh_cast_key_at(a->slot) < kh_cast_key_at(b->slot);
                          });
            } else {
                std::sort(khdr_keep.begin(), khdr_keep.begin() + static_cast<std::ptrdiff_t>(khdr_opq),
                          [](const SunCaster* a, const SunCaster* b) {
                              return a->mesh != b->mesh ? a->mesh < b->mesh : a->lod < b->lod;
                          });
            }

            khdr_ctx->ClearDepthStencilView(khdr_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
            if (khdr_keep.empty()) {  continue; }
            // KH_SUN_OBJ: instanced while the stream holds the face's casters; per object otherwise (no instancing
            // resources, more casters than the stream holds, a failed map) - never an empty face.
            const bool khdr_inst = kh_sun_inst_on() && khdr_keep.size() <= static_cast<size_t>(g_res.sun_instance_cap);

            ConstantData khdr_obj = {};
            ConstantData khdr_cbf = {};
            memcpy(khdr_cbf.view_proj, g_dls_vp[khdr_s][khdr_f], sizeof(khdr_cbf.view_proj));
            khdr_cbf.sun_origin[0] = khdr_l.pos[0];
            khdr_cbf.sun_origin[1] = khdr_l.pos[1];
            khdr_cbf.sun_origin[2] = khdr_l.pos[2];
            if (!kh_upload_frame_cb(khdr_ctx, g_res.composite_frame_cb, khdr_cbf) ||
                !kh_upload_obj_cb(khdr_ctx, g_res.composite_cb, khdr_obj)) {
                khdr_fail = true;
                continue;
            }

            khdr_ctx->OMSetRenderTargets(0, nullptr, khdr_dsv);
            {
                D3D11_VIEWPORT khdr_vpt = {};
                khdr_vpt.Width = static_cast<FLOAT>(g_res.dls_size);
                khdr_vpt.Height = static_cast<FLOAT>(g_res.dls_size);
                khdr_vpt.MaxDepth = 1.0f;
                khdr_ctx->RSSetViewports(1, &khdr_vpt);
            }

            D3D11_MAPPED_SUBRESOURCE khdr_im = {};
            if (!khdr_inst || FAILED(khdr_ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khdr_im))) {
                if (!kh_sun_draw_obj_t(khdr_ctx, khdr_keep, khdr_l.pos)) khdr_fail = true;   // KH_SUN_OBJ.
                continue;
            }
            khdr_ctx->IASetInputLayout(g_res.layout_sundepth);   // KH_SUN_OBJ: a face before may have drawn per object.
            khdr_ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
            KhInstLane* khdr_out = static_cast<KhInstLane*>(khdr_im.pData);   // KH_OBJBUF: slot + alpha.
            for (const SunCaster* khdr_c : khdr_keep) {
                khdr_out->slot = khdr_c->slot;
                khdr_out->dither = 0.0f;
                khdr_out->alpha = khdr_c->alpha;
                khdr_out->mat_base = 0.0f;
                ++khdr_out;
            }
            khdr_ctx->Unmap(g_res.sun_instance_vb, 0);

            ID3D11Buffer* khdr_vbs[2] = { nullptr, g_res.sun_instance_vb };
            UINT khdr_str[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
            UINT khdr_off[2] = { 0, 0 };
            size_t khdr_first = 0;
            KhDepthSk khdr_sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr, nullptr };
            while (khdr_first < khdr_opq) {   // KH_DLS_ALPHA: the opaque partition.
                const int khdr_mid = khdr_keep[khdr_first]->mesh;
                const int khdr_lod = khdr_keep[khdr_first]->lod;   // KH_SHADOW_LOD_DRAWN.
                // KH_CLOTH: a run may not span two buffers, so a cloth caster forms a run of one. KH_SKIN_POOL: nor
                // two slices of one chunk (the pair) - unless the run pulls them through the twin (kh_cast_run_joins).
                const KhVbRef khdr_vb = kh_mesh_vb_ref(khdr_mid, khdr_keep[khdr_first]->slot);
                const KhSkinPoolChunk* const khdr_pc =
                    kh_cast_pool_of(khdr_mid, khdr_keep[khdr_first]->slot, g_res.vs_sundepth_sk);
                size_t khdr_last = khdr_first + 1;
                while (khdr_last < khdr_opq && khdr_keep[khdr_last]->mesh == khdr_mid &&
                       khdr_keep[khdr_last]->lod == khdr_lod &&
                       kh_cast_run_joins(khdr_vb, khdr_pc, khdr_keep[khdr_last]->mesh, khdr_keep[khdr_last]->slot,
                                         g_res.vs_sundepth_sk)) ++khdr_last;
                khdr_sk.use(khdr_ctx, khdr_pc);
                khdr_vbs[0] = khdr_vb.vb;
                khdr_off[0] = khdr_vb.off;
                khdr_ctx->IASetVertexBuffers(0, 2, khdr_vbs, khdr_str, khdr_off);
                khdr_ctx->IASetIndexBuffer(g_res.mesh_ib[khdr_mid], DXGI_FORMAT_R32_UINT, 0);
                UINT khdr_ls = 0, khdr_lc = 0;
                mesh_lod_range_of(khdr_mid, khdr_lod, khdr_ls, khdr_lc);   // KH_SHADOW_LOD_DRAWN.
                khdr_ctx->DrawIndexedInstanced(khdr_lc, static_cast<UINT>(khdr_last - khdr_first),
                                               khdr_ls, 0, static_cast<UINT>(khdr_first));
                khdr_first = khdr_last;
            }
            khdr_sk.end(khdr_ctx);   // KH_SKIN_POOL: the face's own pair again.
            if (khdr_opq < khdr_keep.size()) {   // KH_DLS_ALPHA: the alpha tail, the sun's own draw.
                if (!kh_sun_draw_alpha_t(khdr_ctx, khdr_keep, khdr_obj, g_res.sun_instance_vb)) khdr_fail = true;
                // Back to this bracket's opaque state.
                khdr_ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
                khdr_ctx->PSSetShader(nullptr, nullptr, 0);
            }

        }
        if (khdr_fail) {   // KH_SUN_OBJ: a face left incomplete - never published as valid, and rebuilt next frame.
            g_dls_keys[khdr_s] = 0;
            g_dls_map_valid &= ~(1ull << khdr_s);
        }

        khdr_slice += khdr_fn;
    }
    // Hand the sun back what it had (the rasteriser's Get AddRef'd: released either way; the OM save releases itself).
    if (g_res.dls_rast) {
        khdr_ctx->RSSetState(khdr_prev_rs);
        KH_SAFE_RELEASE(khdr_prev_rs);
    }
    khdr_om.restore(khdr_ctx);
    khdr_om.release();
    if (khdr_prev_nvp > 0) khdr_ctx->RSSetViewports(khdr_prev_nvp, khdr_prev_vp);
}

// KH_DLSW_OWN_VIEW: can a ball of ours (centre khdo_m, radius khdo_h) stand, seen from the camera khdo_c, in front
// of a world surface inside a light's reach (centre khdo_l, radius khdo_r)? The world pass multiplies only surfaces
// in reach, and the mask keeps it off our pixels - so a mesh's place in the mask is decided by whether it can cover
// such a surface, not by whether the mesh itself is in reach. True when the two cones of view rays (through the ball,
// through the sphere) meet and the ball's nearest point is not beyond the sphere's far side; conservative (true)
// with the camera inside the ball, and inside the sphere once the far-side test has passed. The reach handed in is
// the pass's (kh_dls_frame): a sphere round the cube a point light's faces shade, not the map's far distance. The
// light's terms (camera khdo_c, sphere khdo_l / khdo_r) are the same for every ball: kh_dlsw_view_light takes them
// once, kh_dlsw_view_covers each ball against them.
struct KhDlswLight {
    float b[3];   // The camera to the sphere's centre.
    float db;     // Its length.
    float r;      // The sphere's radius.
    float as;     // The half-angle it subtends (read only with the camera outside it).
};
inline void kh_dlsw_view_light(const float khdo_c[3], const float khdo_l[3], float khdo_r, KhDlswLight& khdo_v) {
    khdo_v.b[0] = khdo_l[0] - khdo_c[0];
    khdo_v.b[1] = khdo_l[1] - khdo_c[1];
    khdo_v.b[2] = khdo_l[2] - khdo_c[2];
    khdo_v.db = sqrtf(khdo_v.b[0] * khdo_v.b[0] + khdo_v.b[1] * khdo_v.b[1] + khdo_v.b[2] * khdo_v.b[2]);
    khdo_v.r = khdo_r;
    khdo_v.as = khdo_v.db > khdo_r ? asinf(khdo_r / khdo_v.db) : 0.0f;
}
inline bool kh_dlsw_view_covers(const float khdo_c[3], const float khdo_m[3], float khdo_h,
                                const KhDlswLight& khdo_v) {
    const float khdo_a[3] = { khdo_m[0] - khdo_c[0], khdo_m[1] - khdo_c[1], khdo_m[2] - khdo_c[2] };
    const float* const khdo_b = khdo_v.b;
    const float khdo_da = sqrtf(khdo_a[0] * khdo_a[0] + khdo_a[1] * khdo_a[1] + khdo_a[2] * khdo_a[2]);
    const float khdo_db = khdo_v.db;
    const float khdo_r = khdo_v.r;
    if (!(khdo_da > khdo_h)) return true;   // Inside the ball (or not a number).
    // KH_DLSW_VIEW_FAR: ahead of the sphere test - it holds with the camera inside the sphere too (every surface in
    // reach is within db + r of it); behind it, a camera inside a light's reach made every visible mesh in the scene
    // an owner, however far.
    if (khdo_da - khdo_h > khdo_db + khdo_r) return false;   // Wholly beyond every surface in reach.
    if (!(khdo_db > khdo_r)) return true;   // Inside the sphere (or not a number).
    float khdo_cos = (khdo_a[0] * khdo_b[0] + khdo_a[1] * khdo_b[1] + khdo_a[2] * khdo_b[2]) / (khdo_da * khdo_db);
    khdo_cos = khdo_cos < -1.0f ? -1.0f : (khdo_cos > 1.0f ? 1.0f : khdo_cos);
    return acosf(khdo_cos) <= asinf(khdo_h / khdo_da) + khdo_v.as;
}
inline void kh_dls_frame(ID3D11DeviceContext* khdf_ctx) {
    KH_PROF_SCOPE(KHP_DLS_FRAME);
    if (!khdf_ctx) return;
    if (g_dls_frame_cycle == g_topo_cycles) return;
    g_dls_frame_cycle = g_topo_cycles;
    // KH_PROF_ONCE: the GPU stamp past the once-per-cycle return, as render_sun_depth's. A zone reports its last run,
    // and a cycle can call this twice (an injection re-armed for the camera partition or by the rescue, a park flush
    // ahead of the injection): stamped at the head, gDlsFrame was the second call's empty return.
    KH_GPU_SCOPE(khdf_ctx, KHG_DLS_FRAME);

    static std::vector<SunCaster> khdf_casters;   // Scratch: both callers run on the render thread or in a park.
    khdf_casters.clear();
    static std::vector<SunCaster> khdf_owners;   // KH_SHADOW_SWITCH: visible non-casters (the mask only).
    khdf_owners.clear();

    if (g_dls_n > 0) {
        kh_scene_sync();   // KH_SCENE: live walk by reference, no copy.
        // The reach test is a sphere per light, so the walk visits only the cells the union of those spheres can
        // touch (the grid pads by each cell's largest object radius).
        static std::vector<uint32_t> khdf_cand;
        {
            float khdf_mn[3] = { 0.0f, 0.0f, 0.0f }, khdf_mx[3] = { 0.0f, 0.0f, 0.0f };
            bool khdf_any = false;
            for (uint32_t khdf_s = 0; khdf_s < KH_DLS_MAX; ++khdf_s) {
                if (!g_dls[khdf_s].live) continue;
                const float khdf_r = kh_dls_far(g_dls[khdf_s]);
                for (int khdf_k = 0; khdf_k < 3; ++khdf_k) {
                    const float khdf_lo = g_dls[khdf_s].pos[khdf_k] - khdf_r;
                    const float khdf_hi = g_dls[khdf_s].pos[khdf_k] + khdf_r;
                    if (!khdf_any || khdf_lo < khdf_mn[khdf_k]) khdf_mn[khdf_k] = khdf_lo;
                    if (!khdf_any || khdf_hi > khdf_mx[khdf_k]) khdf_mx[khdf_k] = khdf_hi;
                }
                khdf_any = true;
            }
            khdf_cand.clear();
            if (khdf_any) kh_scene_grid_query_box(khdf_mn, khdf_mx, khdf_cand);
        }
        kh_objbuf_sync(khdf_ctx);   // KH_OBJBUF.
        static std::vector<uint8_t> khdf_listed;   // KH_DLSW_OWN_VIEW: the slots the reach walk placed.
        khdf_listed.assign(g_scene.objs.size(), 0u);
        for (uint32_t khsc_i : khdf_cand) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& o = g_scene.objs[khsc_i];
            // KH_SHADOW_ACTIVE: casting is the sun's rule (kh_shadow_part, then kh_cast_on below). Owning pixels is
            // not: an unlit mesh neither casts nor receives, yet a visible one the injection drew is our surface at
            // its pixels - left out of the mask, the world pass multiplied the shadow of the surface behind it onto
            // it (KH_DLSW_OWN). So a visible world mesh turned down only for being unlit still joins as an owner.
            const bool khdf_part = kh_shadow_part(o);
            if (!khdf_part && !(kh_obj_shown(o) && kh_shadow_part_unlit(o))) continue;   // KH_SHOW_INFRONT.
            const float khdf_ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
            float khdf_he[3];
            kh_world_half_extents(o, khdf_he);
            const float khdf_hd = sqrtf(khdf_he[0] * khdf_he[0] + khdf_he[1] * khdf_he[1] +
                                        khdf_he[2] * khdf_he[2]);
            bool khdf_near = false;
            for (uint32_t khdf_s = 0; khdf_s < KH_DLS_MAX && !khdf_near; ++khdf_s) {
                if (!g_dls[khdf_s].live) continue;
                const float khdf_lim = kh_dls_far(g_dls[khdf_s]) + khdf_hd;
                const float khdf_dx = khdf_ce[0] - g_dls[khdf_s].pos[0];
                const float khdf_dy = khdf_ce[1] - g_dls[khdf_s].pos[1];
                const float khdf_dz = khdf_ce[2] - g_dls[khdf_s].pos[2];
                khdf_near = khdf_dx * khdf_dx + khdf_dy * khdf_dy + khdf_dz * khdf_dz <= khdf_lim * khdf_lim;
            }
            if (!khdf_near) {  continue; }
            khdf_listed[khsc_i] = 1u;
            // KH_SHADOW_LOD_DRAWN: this frame's camera (KH_DLS_LOD_CAM - with or without a published sun).
            // KH_SHADOW_SWITCH: a mesh with castShadow off draws into no map, but a visible one still owns its pixels,
            // which the world pass must not paint (the mask).
            if (khdf_part && kh_cast_on(o)) {   // KH_SHADOW_OFF too.
                khdf_casters.push_back(kh_sun_caster_of(o, g_sun_cam_now));
            } else if (kh_obj_shown(o)) {   // KH_SHOW_INFRONT.
                khdf_owners.push_back(kh_sun_caster_of(o, g_sun_cam_now));
            }
        }
        // KH_DLSW_OWN_VIEW: a visible mesh out of every light's reach casts nothing into these maps, but it still
        // owns its pixels - standing between the camera and a wall a light reaches, left out of the mask it would take
        // the wall's shadow from the world pass. Every visible world mesh the reach walk did not place
        // joins as an owner (the mask only) when it can cover a surface in some light's reach (kh_dlsw_view_covers);
        // camera-less (KH_DLS_LOD_CAM zero), every one joins. The reach is where the pass can shade, not the map's
        // far: a face refuses a receiver by its depth along the face axis (KhDlsFaceUV), so a point light shades the
        // cube of half-size far round it - out to sqrt(3) far at the corners - and PSDlsWorld's own early-out, the
        // anchors' radius (kh_dls_fill_dl_world), is sqrt(3) x 1.05 far. A sphere of the far alone leaves a mesh in
        // front of a cube corner out of the mask. KH_DLSW_SPOT_REACH: a spot wider than 90 degrees shades further out
        // (kh_dls_shade_k), read from its last rendered frustum - this walk precedes the render, so a slot that
        // turns into a wide spot this frame takes the cube's reach for that one frame.
        const bool khdf_cam = g_sun_cam_now[0] != 0.0f || g_sun_cam_now[1] != 0.0f || g_sun_cam_now[2] != 0.0f;
        KhDlswLight khdf_vl[KH_DLS_MAX];   // The live slots' terms, once per frame (kh_dlsw_view_light).
        for (uint32_t khdf_s = 0; khdf_s < KH_DLS_MAX; ++khdf_s) {
            if (g_dls[khdf_s].live) {
                kh_dlsw_view_light(g_sun_cam_now, g_dls[khdf_s].pos,
                                   kh_dls_shade_k(khdf_s) * 1.05f * kh_dls_far(g_dls[khdf_s]), khdf_vl[khdf_s]);
            }
        }
        for (uint32_t khdf_i = 0; khdf_i < g_scene.objs.size(); ++khdf_i) {
            if (!g_scene.alive[khdf_i] || khdf_listed[khdf_i]) continue;
            const RenderObject& o = g_scene.objs[khdf_i];
            if (!kh_obj_shown(o) || !kh_shadow_part_unlit(o)) continue;   // kh_shadow_part implies it.
            const float khdf_ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
            float khdf_he[3];
            kh_world_half_extents(o, khdf_he);
            const float khdf_hd = sqrtf(khdf_he[0] * khdf_he[0] + khdf_he[1] * khdf_he[1] +
                                        khdf_he[2] * khdf_he[2]);
            bool khdf_cov = !khdf_cam;
            for (uint32_t khdf_s = 0; khdf_s < KH_DLS_MAX && !khdf_cov; ++khdf_s) {
                if (!g_dls[khdf_s].live) continue;
                khdf_cov = kh_dlsw_view_covers(g_sun_cam_now, khdf_ce, khdf_hd, khdf_vl[khdf_s]);
            }
            if (khdf_cov) khdf_owners.push_back(kh_sun_caster_of(o, g_sun_cam_now));
        }
    }

    // KH_SUN_OBJ: the instancing resources are not required - a face draws per object without them.
    const bool khdf_state = g_res.vs && g_res.input_layout && g_res.dss_test_write && g_res.rast_sun &&
                            g_res.composite_cb && g_res.composite_frame_cb;
    if (!khdf_state || khdf_casters.empty()) {
        // Nothing to render into, or nothing that could cast. The per-light skip keys are untouched, so a caster
        // returning next frame reuses its slices.
        g_dls_map_valid = 0;
        for (uint32_t khdf_s = 0; khdf_s < KH_DLS_MAX; ++khdf_s) g_dls_facen[khdf_s] = 0;
        g_dlsw_casters.clear();
        return;
    }

    // The standalone sun bracket, minus the tiers: own-draw exclusion, full state save, the depth-only instanced
    // pipeline. kh_dls_render saves and restores its own targets, viewport and rasteriser; StateBackup restores both
    // vertex-buffer slots and t36.
    if (g_res.vs_sundepth && g_res.layout_sundepth && g_res.obj_srv) {   // KH_SUN_INST_CAP: the faces' stream.
        ID3D11Device* khdf_dev = nullptr;
        khdf_ctx->GetDevice(&khdf_dev);
        if (khdf_dev) {
            kh_sun_inst_ensure(khdf_dev, khdf_casters.size());
            khdf_dev->Release();
        }
    }
    const bool khdf_pinj = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup khdf_bk;
    khdf_bk.capture(khdf_ctx);
    KhBindScope khdf_bc(khdf_ctx);   // KH_BIND_FILTER: the faces' alpha casters.
    khdf_ctx->IASetInputLayout(g_res.layout_sundepth);
    khdf_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    khdf_ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
    kh_objbuf_bind(khdf_ctx);   // KH_OBJBUF (StateBackup restores t39).
    khdf_ctx->PSSetShader(nullptr, nullptr, 0);
    khdf_ctx->GSSetShader(nullptr, nullptr, 0);
    khdf_ctx->HSSetShader(nullptr, nullptr, 0);
    khdf_ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* khdf_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    khdf_ctx->VSSetConstantBuffers(0, 2, khdf_cbs);
    khdf_ctx->PSSetConstantBuffers(0, 2, khdf_cbs);
    khdf_ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    khdf_ctx->RSSetState(g_res.rast_sun);   // dls_rast supersedes it inside (KH_DLS_DEPTHCLIP).

    kh_dls_render(khdf_ctx, khdf_casters, khdf_owners);

    khdf_bk.restore(khdf_ctx);
    g_ro.in_injection = khdf_pinj;
}
}