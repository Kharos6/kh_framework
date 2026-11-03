#pragma once

// The feature half (rendering_integration_features.hpp): DLS, the dynamic-light shadow maps: the maps, the DLF
// selection, the fills, the render and the frame. Never include it on its own.
namespace RenderIntegration {
static constexpr UINT DLS_MAP_PX = 1024;

// The light-shadow maps' render (Stage 2b): the face viewport, the face count, slice and mask, spot or point, the held
// faces, the map's far.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: facen, mapfar.
// Session reset: session_globals_reset resets every member.
struct DlsRenderState {
    // Stage 2b - the maps. Every property of the sun's depth pass (the shadow LOD, the mesh-run batching, the
    // alpha-coverage dither: the per-face keep list is partitioned opaque-first and its alpha tail goes through
    // sun_draw_alpha_t under the light's own frame CB - sunOrigin the light, viewProj the face, mirMeta.w zero)
    // applies here: it is the sun's version.
    float    vp[DLS_MAX][6][16] = {};   // Per slot, per face: world(light-rel) -> clip.
    uint32_t facen[DLS_MAX] = {};       // Slices this slot owns; 0 = no map.
    // The face -> slice map is not slot*6 + face. Stage 3 must read this table; 0xFFFF = the face has no map, which the
    // shader reads as 'nothing occludes in that direction'.
    uint16_t faceslice[DLS_MAX][6] = {};
    uint8_t  facemask[DLS_MAX] = {};    // Which faces got a slice.
    // Named, not inferred: a spot read as a cube would admit every caster in the scene into one face.
    uint8_t  isspot[DLS_MAX] = {};      // 1 = single spot frustum, not a cube.
    uint32_t face_hold[DLS_MAX][6] = {};
    float    mapfar[DLS_MAX] = {};      // The far plane its matrices were built to.
};
static DlsRenderState g_dls_render;
// DLSW_SPOT_REACH - the multiple of a slot's far within which PSDlsWorld can shade. A cube face refuses a receiver
// by its depth along the face axis, at least |p| / sqrt(3); a spot's single frustum refuses by the depth along the
// spot's axis, and its corners (|x|, |y| <= t z, t the tan of the half fov) reach out to far sqrt(1 + 2 t^2) - past
// sqrt(3) once the fov is wider than 90 degrees. From the slot's last rendered frustum (g_dls_render.vp's column x has
// length 1 / t). TWIN: PSDlsWorld's reach test (static.hlsl).
inline float dls_shade_k(uint32_t s) {
    float k2 = 3.0f;
    if (s < DLS_MAX && g_dls_render.isspot[s]) {
        const float* m = g_dls_render.vp[s][0];
        const float c2 = m[0] * m[0] + m[4] * m[4] + m[8] * m[8];
        if (c2 > 1.0e-6f) k2 = fmaxf(k2, 1.0f + 2.0f / c2);
    }
    return sqrtf(k2);
}
static const uint16_t DLS_FACE_NONE = 0xFFFFu;
// A face keeps its slice for DLS_FACE_HOLD frames after its last caster leaves - hysteresis like the distance margin
// DLS_HOLD_SLACK gives the slot table. An empty face costs one clear. DLS_FACE_HOLD_TEXT: the hold is a
// slice-layout latch, not a content hold - the caster's move changes the light's key, and the render pass then clears
// the held face and draws nothing into it (a lit face, as a dropped face reads), so no shadow outlives its caster
// there. What it keeps stable is the face count: a dropped face shifts every later light's slices and keys
// (DLS_KEY_SLICES), re-rendering their maps for a face that flickers in and out of the set mid-motion.
static constexpr uint32_t DLS_FACE_HOLD = 15;

// Grow the depth array to hold want slices. Recreated on growth (a Texture2DArray cannot be extended), so the
// count only climbs. The old array goes before the new one is made, so a create refused on a card with little VRAM
// (48 slices = 192 MB) leaves no DLS maps until one succeeds. DLS_SLICES_MAX is declared in
// rendering/dynamic_light_state.hpp, ahead of Resources, before DLS_MAX exists; EA_DLS_SHAPE checks they agree,
// with the slot bitmask (g_dls_map_valid) and the frame CB's per-face lanes.
static_assert(DLS_SLICES_MAX == static_cast<int>(DLS_MAX) * DLS_FACES && DLS_MAX <= 64u &&
              sizeof(ConstantData::dls_face_slice) ==
                  sizeof(float) * 4u * DLS_MAX * static_cast<size_t>(DLS_FACES) &&
              sizeof(ConstantData::dls_meta) == sizeof(float) * 4u * DLS_MAX,
              "EA_DLS_SHAPE: the slice budget, the slot mask and the frame CB's DLS lanes disagree");

inline bool dls_ensure_maps(ID3D11Device* dev, UINT want) {
    if (!dev || want == 0) return false;
    // Round up to a whole cube: stable across any mask change within a light,
    // and bounds the recreations to eight per session.
    want = ((want + 5u) / 6u) * 6u;
    if (want > static_cast<UINT>(DLS_SLICES_MAX)) want = DLS_SLICES_MAX;
    if (g_res.dls_tex && g_res.dls_slices >= want && g_res.dls_size == DLS_MAP_PX) return true;

    for (int i = 0; i < DLS_SLICES_MAX; ++i) KH_SAFE_RELEASE(g_res.dls_dsv[i]);
    KH_SAFE_RELEASE(g_res.dls_srv);
    KH_SAFE_RELEASE(g_res.dls_tex);
    g_res.dls_slices = 0;
    g_res.dls_size = 0;

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = DLS_MAP_PX;
    td.Height = DLS_MAP_PX;
    td.MipLevels = 1;
    td.ArraySize = want;
    td.Format = DXGI_FORMAT_R32_TYPELESS;   // d32 for the DSV, R32F for the SRV.
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.dls_tex))) return false;

    for (UINT s = 0; s < want; ++s) {
        D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
        dd.Format = DXGI_FORMAT_D32_FLOAT;
        dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dd.Texture2DArray.FirstArraySlice = s;
        dd.Texture2DArray.ArraySize = 1;
        if (FAILED(dev->CreateDepthStencilView(g_res.dls_tex, &dd, &g_res.dls_dsv[s]))) {
            for (UINT k = 0; k < s; ++k) KH_SAFE_RELEASE(g_res.dls_dsv[k]);
            KH_SAFE_RELEASE(g_res.dls_tex);
            return false;
        }
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_R32_FLOAT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    sd.Texture2DArray.MipLevels = 1;
    sd.Texture2DArray.ArraySize = want;
    if (FAILED(dev->CreateShaderResourceView(g_res.dls_tex, &sd, &g_res.dls_srv))) {
        for (UINT k = 0; k < want; ++k) KH_SAFE_RELEASE(g_res.dls_dsv[k]);
        KH_SAFE_RELEASE(g_res.dls_tex);
        return false;
    }

    g_res.dls_slices = want;
    g_res.dls_size = DLS_MAP_PX;
    // Every skip key dies with the texture: a key that outlived it would let a light skip and sample uninitialised
    // depth.
    for (uint32_t k = 0; k < DLS_MAX; ++k) g_dls_keys[k] = 0;
    return true;
}

// Publish the maps to the shader. Filled only through dls_fill_cb (below), from the mesh frame-CB sites, the world
// pass and the mask cast: the zeroed default dls_meta[i].w == 0 reads as 'no map, answer lit'. The face -> slice
// table is g_dls_render.faceslice, not slot*6+face: faces are allocated only where a caster stands, so the mapping is
// carried, not computed.
static constexpr float DLS_BIAS_M = 0.01f;      // Constant compare bias (m).
static constexpr float DLS_BIAS_SLOPE = 0.0001f;   // Plus this per metre of range.
// D3D11 silently unbinds a shader resource whose texture is bound as a depth target - which our own render pass does to
// this array. If the mesh bind site already ran this frame, t36 is left null: the readers' GetDimensions guard
// (KhDlsShadow, KhDlfDlsVis) then answers lit - the shadow is lost, never inverted.

// A slot the shader reads as a map: live, mapped this frame, with faces and a far plane - dls_fill_cb publishes
// dlsMeta.w = the far only for such a slot, and KhDlsShadow / PSDlsWorld's reach read w <= 0 as no map. A light whose
// slot fails this casts nothing the pass can see; it lights the surface like any other light and is packed as one.
// DLS_SRV_GATE: g_dls_map_valid outlives Resources::release (the maps go, the bits stay until the next
// dls_render); with no map view t36 is unbound (the shaders' GetDimensions guard answers lit), so no view = no
// map - the light is packed and lit as an ordinary one, not sent to a lookup that cannot run. Reads
// g_dls_map_valid (written by the render pass), so it must be declared above its readers.
inline bool dls_published(uint32_t s) {
    return s < DLS_MAX && g_res.dls_srv && g_dls[s].live &&
           (g_dls_map_valid & (1ull << s)) != 0ull &&
           g_dls_render.facen[s] > 0 && g_dls_render.mapfar[s] > 0.0f;
}
// The world pass's light lanes, filled inside its state bracket (the append binds PS t40), through the light ring
// (DL_RING) like the mesh fill. Records are the engine's own cb11 bytes copied verbatim; the slot lane is written
// after the copy. Points first, then spots. Two kinds: every casting light whose map is published this frame
// (dls_published; slot lane = slot + 1: what our meshes can block, KhDlsWorldFactor's numerator), and every other
// pool light that can reach a pixel the pass shades (slot lane 0: the denominator only). DLSW_DENOM: the factor is
// blocked / everything that lights the pixel; with the casting lights alone the denominator missed every non-casting
// light on the surface and the factor over-darkened, against its contract (every error fails toward
// under-darkening). "Can reach" follows the shader's own gates, widened only: PSDlsWorld shades a pixel only within
// dls_shade_k * 1.05 * far of a published map (its reach test: sqrt(3) for a cube, more for a spot wider than 90
// degrees), and a light adds an exact zero past its hard fade (dist * scale >= fade start + 1 / inverse width); a
// light with no fade (inverse width ~0) has no such bound and is always packed. No published casting light: nothing
// is packed (dl_ctl.x stays 0) and the fill returns false, as on every other stand-down: the factor is 1 everywhere,
// so the caller skips the draw.
inline bool dls_fill_dl_world(ID3D11DeviceContext* ctx, ConstantData& cb) {
    if (g_dl.pool_n == 0) return false;
    if (steady_now_ms() - g_dl.pool_stamp > DL_POOL_STALE_MS) return false;

    const float scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                      ? g_dl.main_scale : 1.0f;
    // The spheres the pass can shade in: every map dls_fill_cb publishes
    // (dlsMeta.w > 0), at PSDlsWorld's reach radius plus a hair.
    float anc[DLS_MAX][4];
    uint32_t anc_n = 0;
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        if (!dls_published(s)) continue;
        anc[anc_n][0] = g_dls[s].pos[0];
        anc[anc_n][1] = g_dls[s].pos[1];
        anc[anc_n][2] = g_dls[s].pos[2];
        // DLSW_SPOT_REACH.
        anc[anc_n][3] = dls_shade_k(s) * 1.05f * g_dls_render.mapfar[s] * 1.001f;
        anc_n++;
    }

    static thread_local std::vector<float> rec;   // Scratch.
    rec.clear();
    uint32_t out = 0, pn = 0, cast = 0;
    for (int pass = 0; pass < 2; ++pass) {
        for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
            const DlPoolLight& pl = g_dl.pool[i];
            if ((pl.spot != 0) != (pass == 1)) continue;
            const int slot = dls_slot_of(pl);
            const bool casts = slot >= 0 && dls_published(static_cast<uint32_t>(slot));
            if (!casts) {
                // Denominator only: packed if it can light a pixel the pass shades.
                const float inv = pl.rec[21];
                bool reach = !(inv > 1.0e-6f);   // No hard fade: unbounded.
                if (!reach) {
                    const float r = (pl.rec[20] + 1.0f / inv) / scale * 1.001f;
                    for (uint32_t a = 0; a < anc_n && !reach; ++a) {
                        const float dx = pl.rec[0] - anc[a][0];
                        const float dy = pl.rec[1] - anc[a][1];
                        const float dz = pl.rec[2] - anc[a][2];
                        const float rr = anc[a][3] + r;
                        reach = dx * dx + dy * dy + dz * dz < rr * rr;
                    }
                }
                if (!reach) continue;
            }
            rec.insert(rec.end(), pl.rec, pl.rec + 24);
            rec[static_cast<size_t>(out) * 24u + 22u] = casts ? static_cast<float>(slot + 1) : 0.0f;
            out++;
            if (pass == 0) pn++;
            if (casts) cast++;
        }
    }
    if (cast == 0) return false;   // Nothing can block: dl_ctl.x stays 0 and the kernel stands down.
    uint32_t first = 0;
    if (!dlr_append(ctx, rec.data(), out * 6u, first)) {
        stat_inc(g_stats.dl_ring_fails);   // DL_DIAG.
        return false;   // Lit: the kernel stands down.
    }
    memcpy(&cb.dl_first[0], &first, sizeof(first));
    cb.dl_first[1] = 0.0f;
    cb.dl_first[2] = 0.0f;
    cb.dl_first[3] = 0.0f;
    // The mesh fill's own control and global lanes (fill_dynlights_cb): mode 3, the split, the distance scale, the
    // main pass's diffuse multiplier and the global intensity - one writer, so the two cannot drift.
    dl_fill_ctl(cb, out, pn, scale);
    return true;
}
// DLF - dynamicLightFog's lights (effect.hlsl's DLF note). A fresh pool (dl_select's staleness rule)
// is the chain loop's skip test: without one the pass would add nothing.
inline bool dlf_pool_live() {
    return g_dl.pool_n != 0 && steady_now_ms() - g_dl.pool_stamp <= DL_POOL_STALE_MS;
}
// Every pool light whose reach comes within the pass's maxDistance of the camera, nearest reach first, at most
// maxLights, packed points first then spots as the shader's pointN split reads them. Each is the engine's cb11 record
// as dl_pack copies it, with [0..2] made relative to the pass's camera (fxCam: the frame the shader reconstructs in;
// the subtraction in double), [5].z = our light map's slot + 1 when the pass takes our meshes' shadows (shadows >= 1)
// and the map is published this frame (dls_published), else 0, and [5].w = the reach in metres: the nearest of the
// hard fade (the shader's fade term is zero past it), the distance where the light's brightness under its attenuation -
// and the pass's intensity and tint, which scale what it adds - falls to DLF_EPS, and the camera's distance plus
// maxDistance (the gather marches no farther from the camera). A record with a lane that is not finite is left out (one
// would blank every light's fog where its reach spans). Returns the count; pn = the points among them, scale
// = the distance scale their attenuation takes (dl_select's). Render thread or a park (g_dl's fill-site rule);
// FX_LATE's chain-only flush may also run it on the game thread, inside a resolve the engine issues there - under
// that route's premise that the engine never drives its context from both threads at once, which holds g_dl's writers
// (the harvests, inside the render thread's context calls and flush) off it. Empty: no fresh pool, an unarmed camera
// (fxCam.w), or no light bright and near enough. DLF_EPS bounds the light's colour x attenuation alone, not what a
// sample adds - that is also x sigma x pi p_HG, whose forward peak is (1 + g) / (1 - g)^2 times isotropic's: looking
// into a lamp the cut drops the fog between the camera and its reach sphere - up to 2 % of the halo at anisotropy 0,
// 3.5 - 5 % at the default 0.6, 13 - 19 % at 0.9 (lamps 50 - 150 m off), read as a smaller, dimmer halo, not an edge.
// Folding the peak into the reach recovers it but triples the reach of a quadratic-falloff lamp at 0.6 (up to x14 at
// 0.9): 2.4 - 11 times the light-texel pairs the gather marches at 0.6, 3 - 87 times at 0.9.
static constexpr float DLF_EPS = 2.0e-3f;   // Scene units: colour x attenuation below this ends a light's reach.
inline uint32_t dlf_select(const ConstantData& cb, std::vector<float>& out, uint32_t& pn,
                           float& scale) {
    out.clear();
    pn = 0;
    scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f) ? g_dl.main_scale : 1.0f;
    if (!dlf_pool_live() || !(cb.fx_cam[3] >= 0.5f)) return 0;
    // The pass's params, stored finite (set_effect_params), are clamped where read: maxDistance 10 - 5000 m and
    // maxLights 1 - 64 here; steps 4 - 64 (effect.hlsl's gather), intensity 0 or more (the gather), anisotropy
    // -0.9 - 0.9 (KhDlfPhase) and fogScale and baseDensity 0 or more (KhDlfSigma; effect.hlsl). DLF_COST: the work
    // grows as lights x steps x the grid's texels (setSsgiScale), shadows adding to every step. At the maxima together
    // (64 lights in reach, 64 steps, setSsgiScale 2, shadows 2) it is estimated - not measured - at a second or more a
    // frame on a mid-range GPU, in ONE draw; a draw of about two seconds makes Windows reset the driver, which the game
    // does not survive. Widen no range here without re-measuring that.
    const float maxd = fminf(fmaxf(cb.fx1[1], 10.0f), 5000.0f);
    const uint32_t dl_max = static_cast<uint32_t>(fminf(fmaxf(cb.fx1[3], 1.0f), 64.0f));
    const bool shadow = cb.fx1[2] >= 0.5f;
    // The brightness the shader gives a light (dl_fill_ctl's global tint and intensity).
    float gd[3] = { 1.0f, 1.0f, 1.0f };
    if (g_dl.main_ref_valid &&
        (g_dl.main_gdiff[0] > 0.0f || g_dl.main_gdiff[1] > 0.0f || g_dl.main_gdiff[2] > 0.0f)) {
        for (int c = 0; c < 3; ++c) gd[c] = g_dl.main_gdiff[c];
    }
    // The pass's own gain on a light: intensity (fx0.x) times the tint's strongest channel.
    const float gi = dl_intensity() * fmaxf(cb.fx0[0], 0.0f) *
                     fmaxf(fmaxf(cb.color[0], cb.color[1]), fmaxf(cb.color[2], 0.0f));
    struct DlfCand { float key; float reach; float rel[3]; uint32_t i; float dist; float cut; };
    static thread_local std::vector<DlfCand> cand;   // Scratch.
    cand.clear();
    for (uint32_t dl_i = 0; dl_i < g_dl.pool_n; ++dl_i) {
        const float* const r = g_dl.pool[dl_i].rec;
        bool fin = true;
        for (int a = 0; a < 22; ++a) fin = fin && std::isfinite(r[a]);
        if (!fin) continue;
        DlfCand e;
        double dd = 0.0;
        for (int a = 0; a < 3; ++a) {
            e.rel[a] = static_cast<float>(static_cast<double>(r[a]) -
                                          static_cast<double>(cb.fx_cam[a]));
            dd += static_cast<double>(e.rel[a]) * static_cast<double>(e.rel[a]);
        }
        const float dl_dist = static_cast<float>(std::sqrt(dd));
        if (!std::isfinite(dl_dist)) continue;
        // Its brightness at attenuation 1: DynLights' combine without the N.L, its brightest channel, times the
        // pass's gain.
        float peak = 0.0f;
        for (int c = 0; c < 3; ++c) {
            peak = fmaxf(peak, (gd[c] * r[8 + c] + r[12 + c]) * gi);
        }
        if (!(peak > DLF_EPS)) continue;
        const float open = dl_dist + maxd;   // DLF_TIE: the reach maxDistance alone allows.
        float dl_reach = open;
        if (r[21] > 1.0e-6f) {   // The hard fade (dl_select's reach).
            dl_reach = fminf(dl_reach, (r[20] + 1.0f / r[21]) / scale);
        }
        // The attenuation's reach: 1 / (c + l x + q x^2) falls to DLF_EPS / peak at x = d * scale - start. A light
        // whose constant term alone is past it never shows; negative terms (no engine light has them) leave the
        // reach to the fade and maxDistance.
        const float k = peak / DLF_EPS;
        const float c0 = r[17], l = r[18], q = r[19];
        if (!(k > c0)) continue;
        if (l >= 0.0f && q >= 0.0f) {
            // The positive root of q x^2 + l x - (k - c) in the form that keeps a tiny q from cancelling it away.
            const float den = l + std::sqrt(l * l + 4.0f * q * (k - c0));
            if (den > 1.0e-12f) {   // Else unbounded.
                const float x = 2.0f * (k - c0) / den;
                if (std::isfinite(x)) {
                    dl_reach = fminf(dl_reach, (fmaxf(r[16], 0.0f) + x) / scale);
                }
            }
        }
        if (!(dl_reach > 0.0f) || !std::isfinite(dl_reach)) continue;
        if (dl_dist - dl_reach > maxd) continue;
        e.key = dl_dist - dl_reach;
        e.reach = dl_reach;
        e.i = dl_i;
        e.dist = dl_dist;
        e.cut = dl_reach == open ? -maxd : e.key;   // DLF_TIE (below).
        cand.push_back(e);
    }
    std::stable_sort(cand.begin(), cand.end(),
                     [](const DlfCand& a, const DlfCand& b) { return a.key < b.key; });
    if (cand.size() > dl_max) {
        // DLF_TIE: every light whose reach runs past maxDistance has its reach clamped to dist + maxDistance, so
        // its key is -maxDistance give or take the add's rounding - a tie among all of them, which pool order and
        // that rounding broke: with more of them than maxLights the lamp beside the camera could lose its glow to a
        // farther one. The cut ranks such a light at exactly -maxDistance (cut) and a tie nearest first; the lights
        // kept go back in the order above (key, then pool order) for the draw.
        std::stable_sort(cand.begin(), cand.end(), [](const DlfCand& a, const DlfCand& b) {
            return a.cut < b.cut || (a.cut == b.cut && a.dist < b.dist);
        });
        cand.resize(dl_max);
        std::stable_sort(cand.begin(), cand.end(), [](const DlfCand& a, const DlfCand& b) {
            return a.key < b.key || (a.key == b.key && a.i < b.i);
        });
    }
    std::stable_partition(cand.begin(), cand.end(),
                          [](const DlfCand& a) { return g_dl.pool[a.i].spot == 0; });
    out.resize(cand.size() * 24u);
    for (size_t j = 0; j < cand.size(); ++j) {
        const DlfCand& e = cand[j];
        const DlPoolLight& pl = g_dl.pool[e.i];
        float* const o = &out[j * 24u];
        memcpy(o, pl.rec, DL_LIGHT_BYTES);
        o[0] = e.rel[0];
        o[1] = e.rel[1];
        o[2] = e.rel[2];
        const int s = shadow ? dls_slot_of(pl) : -1;
        o[22] = (s >= 0 && dls_published(static_cast<uint32_t>(s)))
              ? static_cast<float>(s + 1) : 0.0f;
        if (s >= 0 && o[22] == 0.0f) stat_inc(g_stats.dlf_unshadowed);   // DL_DIAG (CN-4).
        o[23] = e.reach;
        if (pl.spot == 0) ++pn;
    }
    return static_cast<uint32_t>(cand.size());
}
inline void dls_fill_cb(ConstantData& cb) {
    // One fill for every consumer (both mesh passes, the mask cast, the world receive). The clamp is every other
    // g_sun_range consumer's; the camera is the cycle's; the world pass overrides xyz with its own reconstruction
    // camera after this call.
    {
        float cam[3];
        if (dls_camera(cam)) {
            cb.dls_range[0] = cam[0];
            cb.dls_range[1] = cam[1];
            cb.dls_range[2] = cam[2];
            cb.dls_range[3] = fminf(fmaxf(
                g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
        } else {
            cb.dls_range[3] = 0.0f;
        }
    }
    // dlsFaceSlice[].y is our own lane and this fill is its only writer, so the mesh path's arm lives there.
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        // Publishable only if live and mapped this frame: a light selected but not rendered (slice budget, failed
        // upload) publishes w = 0 and reads as lit rather than sampling another light's slice. The world pass's and
        // the fog's own test (dls_published, DLS_SRV_GATE included); the mesh pack's lane reads it through w.
        if (!dls_published(s)) {
            cb.dls_meta[s][3] = 0.0f;
            for (int z = 0; z < 6; ++z) {
                cb.dls_face_slice[static_cast<int>(s) * 6 + z][0] = -1.0f;
            }
            continue;
        }

        cb.dls_meta[s][0] = g_dls[s].pos[0];
        cb.dls_meta[s][1] = g_dls[s].pos[1];
        cb.dls_meta[s][2] = g_dls[s].pos[2];
        cb.dls_meta[s][3] = g_dls_render.mapfar[s];

        cb.dls_ctl[s][0] = g_dls_render.isspot[s] ? 1.0f : 0.0f;
        cb.dls_ctl[s][1] = DLS_BIAS_M;
        cb.dls_ctl[s][2] = g_dls_render.isspot[s]
                         ? static_cast<float>(g_dls_render.faceslice[s][0]) : 0.0f;
        cb.dls_ctl[s][3] = DLS_BIAS_SLOPE;

        // slot * 6 + face, one float4 each. The arm is replicated across all six: the shader reads it before it
        // knows which face it wants.
        const float arm = 1.0f;
        for (int f = 0; f < 6; ++f) {
            const uint16_t sl = g_dls_render.faceslice[s][f];
            const int i = static_cast<int>(s) * 6 + f;
            cb.dls_face_slice[i][0] = (sl == DLS_FACE_NONE)
                                    ? -1.0f : static_cast<float>(sl);
            cb.dls_face_slice[i][1] = arm;
        }

        if (g_dls_render.isspot[s]) {
            memcpy(cb.dls_spot_vp[s], g_dls_render.vp[s][0],
                   sizeof(cb.dls_spot_vp[s]));
        }
    }
}

// owners (SHADOW_SWITCH): meshes that cast nothing but own pixels - the mask's alone, never a map's.
inline void dls_render(ID3D11DeviceContext* ctx,
                       const std::vector<SunCaster>& casters,
                       const std::vector<SunCaster>& owners) {
    g_dls_map_valid = 0;
    // The caster-set hash the per-light skip keys on.
    uint64_t chash = CryptoGenerator::FNV1A64_OFFSET;
    bool tex_alpha = false;   // TEX_LAND: a caster draws through its material's maps.
    bool uv_anim = false;   // UV_ANIM: an alpha caster's coverage moves with the clock.
    for (const SunCaster& c : casters) {
        tex_alpha |= c.alpha_caster && c.mats;
        uv_anim |= c.alpha_caster && c.mats && c.mats->uv_anim_alpha;
        chash = CryptoGenerator::fnv1a64_update(chash, c.pos, sizeof(c.pos));
        chash = CryptoGenerator::fnv1a64_update(chash, c.res, sizeof(c.res));   // POS_RES.
        chash = CryptoGenerator::fnv1a64_update(chash, c.size, sizeof(c.size));
        chash = CryptoGenerator::fnv1a64_update(chash, c.rot, sizeof(c.rot));
        chash = CryptoGenerator::fnv1a64_update(chash, &c.mesh, sizeof(c.mesh));
        chash = CryptoGenerator::fnv1a64_update(chash, &c.alpha, sizeof(c.alpha));
        chash = CryptoGenerator::fnv1a64_update(chash, &c.lod, sizeof(c.lod));   // SHADOW_LOD_DRAWN.
        // The map depends on the material and the verdict, as the union hash does.
        const uint64_t mh = c.mats ? c.mats->hash : 0ull;
        chash = CryptoGenerator::fnv1a64_update(chash, &mh, sizeof(mh));
        const uint8_t ac = c.alpha_caster ? 1 : 0;
        chash = CryptoGenerator::fnv1a64_update(chash, &ac, 1);
        // CLOTH: the shape key, as the sun maps hash it.
        const uint32_t cg = cloth_gen_of(c.slot);
        chash = CryptoGenerator::fnv1a64_update(chash, &cg, sizeof(cg));
    }
    if (tex_alpha) {   // TEX_LAND: a map landing redraws, as the union hash does.
        chash = CryptoGenerator::fnv1a64_update(chash, &g_tex_land_serial, sizeof(g_tex_land_serial));
    }
    if (uv_anim) {   // UV_ANIM: the union hash's rule.
        const uint64_t ac = mat_anim_cycle_key();
        chash = CryptoGenerator::fnv1a64_update(chash, &ac, sizeof(ac));
    }
    {
        g_dlsw_casters.clear();
        g_dlsw_casters.reserve(casters.size() + owners.size());
        for (const std::vector<SunCaster>* list : { &casters, &owners })   // SHADOW_SWITCH.
        for (const SunCaster& sc : *list) {
            g_dlsw_casters.emplace_back();
            DlswCaster& sd = g_dlsw_casters.back();
            memcpy(sd.pos, sc.pos, sizeof(sd.pos));
            memcpy(sd.res, sc.res, sizeof(sd.res));   // POS_RES.
            memcpy(sd.size, sc.size, sizeof(sd.size));
            memcpy(sd.rot, sc.rot, sizeof(sd.rot));
            sd.rotated = sc.rotated;
            sd.mesh = sc.mesh;
            sd.slot = sc.slot;   // CLOTH: carried so the mask binds the same buffer the colour pass does.
            sd.lod_lock = sc.lod_lock;
            sd.lod_r = sc.lod_r;   // SKEL_LOD_R.
            sd.visible = sc.visible;   // DLSW_MASK_ALPHA: the mask's own four lanes.
            sd.alpha = sc.alpha;
            sd.mats = sc.mats;
            sd.alpha_caster = sc.alpha_caster;
            sd.in_front = sc.in_front;   // INFRONT.
            sd.no_foot = sc.no_foot;   // FOOT_BLEND.
        }
    }
    if (!ctx || g_dls_n == 0) {  return; }

    // Resolve first, budget second: every light's face count and matrices up front, then allocate to the total.
    UINT need = 0;
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        g_dls_render.facen[s] = 0;
        g_dls_render.facemask[s] = 0;
        g_dls_render.isspot[s] = 0;
        for (int z = 0; z < DLS_FACES; ++z) {
            g_dls_render.faceslice[s][z] = DLS_FACE_NONE;
        }
        if (!g_dls[s].live) continue;
        const DlsSlot& l = g_dls[s];
        const float dr_far = dls_far(l);
        uint32_t fn = 0;

        if (l.spot) {
            // id first, then the nearest pool entry that is a spot.
            const DlPoolLight* src = nullptr;
            if (l.light_id != 0) {
                for (uint32_t p = 0; p < g_dl.pool_n; ++p) {
                    if (g_dl.pool[p].id == l.light_id) { src = &g_dl.pool[p]; break; }
                }
            }
            if (!src) {
                float sd = 0.0f;
                for (uint32_t p = 0; p < g_dl.pool_n; ++p) {
                    const DlPoolLight& c = g_dl.pool[p];
                    const float dx = c.rec[0] - l.pos[0];
                    const float dy = c.rec[1] - l.pos[1];
                    const float dz = c.rec[2] - l.pos[2];
                    const float d2 = dx * dx + dy * dy + dz * dz;
                    if (d2 > DLS_MATCH_M * DLS_MATCH_M) continue;
                    if (!c.spot) continue;   // A point record can never be a spot slot's source.
                    if (!src || d2 < sd) { src = &c; sd = d2; }
                }
            }
            if (src) {
                const float cc = src->rec[7];
                float fov = 0.0f;
                if (dls_spot_view_proj(src->rec + 4, cc, dr_far,
                                       g_dls_render.vp[s][0], &fov)) {
                    fn = 1;
                }
            }
        }
        if (fn == 1) {
            g_dls_render.facemask[s] = 1u;   // The spot's single frustum.
            g_dls_render.isspot[s] = 1;
        } else {
            // Cube: build all six matrices, then keep only the faces that contain a caster. The test is
            // dls_face_keep - the render pass's own - so allocation and draw cannot disagree.
            uint8_t mask = 0;
            for (int f = 0; f < DLS_FACES; ++f) {
                dls_face_vp(f, dr_far, g_dls_render.vp[s][f]);
                bool any = false;
                for (const SunCaster& c : casters) {
                    const float ctr[3] = { c.pos[0], c.pos[2], c.pos[1] };
                    const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f,
                                          c.bsize[1] * 0.5f };   // CAST_REFIT.
                    float he[3];
                    rot_half_extents(hl, c.rot, c.rotated, he);
                    if (dls_face_keep(f, g_dls[s].pos, ctr, he, dr_far)) {
                        any = true;
                        break;
                    }
                }
                if (any) {
                    mask |= static_cast<uint8_t>(1u << f);
                    g_dls_render.face_hold[s][f] = DLS_FACE_HOLD;
                } else if (g_dls_render.face_hold[s][f] > 0) {
                    // Keep the face in the mask so a caster crossing the face boundary does not shift the later
                    // lights' slices (DLS_FACE_HOLD_TEXT: the face itself is cleared on the key change the move
                    // causes - a lit face, not a kept shadow).
                    g_dls_render.face_hold[s][f]--;
                    mask |= static_cast<uint8_t>(1u << f);
                }
            }
            g_dls_render.facemask[s] = mask;
            fn = 0;
            for (int f = 0; f < DLS_FACES; ++f) {
                if (mask & (1u << f)) fn++;
            }
        }
        g_dls_render.facen[s] = fn;
        g_dls_render.mapfar[s] = dr_far;
        need += fn;
    }
    if (need == 0) {  return; }   // Before the bracket opens.

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return;
    const bool ok = dls_ensure_maps(dev, need);
    dev->Release();
    if (!ok) {  return; }

    // DLS_DEPTHCLIP: the sun's tiers bind DepthClipEnable = false twins on their orthographic maps (the union
    // clips). On a perspective map that flag is destructive: geometry nearer than the near plane clamps to depth 0
    // and occludes everything behind it.
    OmSave                om;   // Full OM set in, full OM set out (UAVs carried; self-restoring).
    D3D11_VIEWPORT          prev_vp[16] = {};
    UINT                    prev_nvp = 16;
    om.capture(ctx);
    ctx->RSGetViewports(&prev_nvp, prev_vp);
    ID3D11RasterizerState* prev_rs = nullptr;
    if (!g_res.dls_rast) {
        ID3D11Device* rd = nullptr;
        ctx->GetDevice(&rd);
        if (rd) {
            D3D11_RASTERIZER_DESC rsd = {};
            rsd.FillMode = D3D11_FILL_SOLID;
            rsd.CullMode = D3D11_CULL_NONE;   // As the sun: thin and alpha casters.
            rsd.DepthClipEnable = TRUE;
            rsd.DepthBias = 0;                // The bias is done in metres, in-shader.
            rsd.SlopeScaledDepthBias = 0.0f;
            rsd.DepthBiasClamp = 0.0f;
            rsd.MultisampleEnable = FALSE;
            if (FAILED(rd->CreateRasterizerState(&rsd, &g_res.dls_rast))) {
                g_res.dls_rast = nullptr;
            }
            rd->Release();
        }
    }
    if (g_res.dls_rast) {
        ctx->RSGetState(&prev_rs);   // The sun's, restored below.
        ctx->RSSetState(g_res.dls_rast);
    }

    UINT slice = 0;
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        // DLS_KEY_SLICES: a key names the slices its light rendered, which keep that content only while no other
        // light is laid onto them. A slot that renders nothing this pass (not live, no face, past the budget) gives
        // its slices up to the slots after it, which shift onto them, so its key goes: back, it renders afresh
        // rather than reusing a slice another light has drawn over.
        if (!g_dls[s].live) { g_dls_keys[s] = 0; continue; }
        const DlsSlot& l = g_dls[s];
        const uint32_t fn = g_dls_render.facen[s];
        const float dr_far = g_dls_render.mapfar[s];
        if (fn == 0) { g_dls_keys[s] = 0; continue; }
        if (slice + fn > g_res.dls_slices) {
            for (uint32_t r = s; r < DLS_MAX; ++r) g_dls_keys[r] = 0;
            break;
        }

        // Union-style skip: a caster or the light moving changes the key and forces a rebuild. Skipping reuses the
        // slices, so the matrices must be republished either way - they are state, not output.
        const uint64_t key = dls_light_hash(l, fn, slice,
                                            g_dls_render.facemask[s], chash,
                                            g_dls_render.isspot[s] ? g_dls_render.vp[s][0] : nullptr);
        const bool reuse = (key == g_dls_keys[s]);

        g_dls_map_valid |= (1ull << s);

        if (reuse) {
            // The maps are reused, but the face -> slice table is state every consumer reads: republished on the
            // skip path too.
            UINT rs = slice;
            for (uint32_t f = 0; f < DLS_FACES; ++f) {
                if (g_dls_render.facemask[s] & (1u << f)) {
                    g_dls_render.faceslice[s][f] = static_cast<uint16_t>(rs++);
                }
            }
            slice += fn;
            continue;
        }
        g_dls_keys[s] = key;

        UINT next = slice;
        bool fail = false;   // SUN_OBJ: a face left incomplete - the key is not committed.
        for (uint32_t f = 0; f < DLS_FACES; ++f) {
            if (!(g_dls_render.facemask[s] & (1u << f))) continue;
            const UINT sl = next++;
            g_dls_render.faceslice[s][f] = static_cast<uint16_t>(sl);
            ID3D11DepthStencilView* dsv = g_res.dls_dsv[sl];
            if (!dsv) { fail = true; continue; }   // EA_DLS_DSV: an undrawn face is an incomplete map.

            // Cull to this face. Partitioned opaque-first: the instanced runs cover [0, opq) and the alpha tail
            // [opq, n) draws per caster through the alpha twins; the instance stream is written in this order,
            // so an alpha draw's start-instance lands on its own lane.
            static std::vector<const SunCaster*> keep;
            keep.clear();
            size_t opq = 0;
            for (int part = 0; part < 2; ++part) {
                for (const SunCaster& c : casters) {
                    if ((part == 0) == c.alpha_caster) continue;
                    const float ctr[3] = { c.pos[0], c.pos[2], c.pos[1] };
                    const float hl[3] = { c.bsize[0] * 0.5f, c.bsize[2] * 0.5f,
                                          c.bsize[1] * 0.5f };   // CAST_REFIT.
                    float he[3];
                    rot_half_extents(hl, c.rot, c.rotated, he);
                    const bool in = g_dls_render.isspot[s]
                        ? true   // A spot is one frustum: every caster the reach walk admitted (any light's).
                        : dls_face_keep(static_cast<int>(f), l.pos,
                                        ctr, he, dr_far);
                    if (in) keep.push_back(&c);
                }
                if (part == 0) opq = keep.size();
            }
            // DLS_RUN_SORT: the opaque partition by (mesh, level), the sun tiers' order - the runs below merge
            // neighbours only, and the grid walk that built the list interleaves meshes, so unsorted, most runs would
            // be one caster (a vertex and index buffer bind and a draw each). The face is depth-only (no colour target,
            // a null pixel shader, a depth test that writes): each texel ends at its nearest fragment whatever the
            // order. Each lane is written at its caster's place in the sorted list, so a run's start instance still
            // names its own casters; the alpha tail [opq, n) keeps its place, order and lanes. SKIN_POOL: with
            // a chunk installed, a chunk's casters together within their (mesh, level), for its runs.
            if (g_skin_pool_any) {
                for (size_t k = 0; k < opq; ++k) cast_key_put(keep[k]->mesh,
                                                              keep[k]->slot);
                std::sort(keep.begin(), keep.begin() + static_cast<std::ptrdiff_t>(opq),
                          [](const SunCaster* a, const SunCaster* b) {
                              if (a->mesh != b->mesh) return a->mesh < b->mesh;
                              if (a->lod != b->lod) return a->lod < b->lod;
                              return cast_key_at(a->slot) < cast_key_at(b->slot);
                          });
            } else {
                std::sort(keep.begin(), keep.begin() + static_cast<std::ptrdiff_t>(opq),
                          [](const SunCaster* a, const SunCaster* b) {
                              return a->mesh != b->mesh ? a->mesh < b->mesh : a->lod < b->lod;
                          });
            }

            ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
            if (keep.empty()) {  continue; }
            // SUN_OBJ: instanced while the stream holds the face's casters; per object otherwise (no instancing
            // resources, more casters than the stream holds, a failed map) - never an empty face.
            const bool inst = sun_inst_on() && keep.size() <= static_cast<size_t>(g_res.sun_instance_cap);

            ConstantData obj = {};
            ConstantData cbf = {};
            memcpy(cbf.view_proj, g_dls_render.vp[s][f], sizeof(cbf.view_proj));
            cbf.sun_origin[0] = l.pos[0];
            cbf.sun_origin[1] = l.pos[1];
            cbf.sun_origin[2] = l.pos[2];
            if (!upload_frame_cb(ctx, g_res.composite_frame_cb, cbf) ||
                !upload_obj_cb(ctx, g_res.composite_cb, obj)) {
                fail = true;
                continue;
            }

            ctx->OMSetRenderTargets(0, nullptr, dsv);
            {
                D3D11_VIEWPORT vpt = {};
                vpt.Width = static_cast<FLOAT>(g_res.dls_size);
                vpt.Height = static_cast<FLOAT>(g_res.dls_size);
                vpt.MaxDepth = 1.0f;
                ctx->RSSetViewports(1, &vpt);
            }

            D3D11_MAPPED_SUBRESOURCE im = {};
            if (!inst || FAILED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) {
                if (!sun_draw_obj_t(ctx, keep, l.pos)) fail = true;   // SUN_OBJ.
                continue;
            }
            ctx->IASetInputLayout(g_res.layout_sundepth);   // SUN_OBJ: a face before may have drawn per object.
            ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
            InstLane* out = static_cast<InstLane*>(im.pData);   // OBJBUF: slot + alpha.
            for (const SunCaster* c : keep) {
                out->slot = c->slot;
                out->dither = 0.0f;
                out->alpha = c->alpha;
                out->mat_base = 0.0f;
                ++out;
            }
            ctx->Unmap(g_res.sun_instance_vb, 0);

            ID3D11Buffer* vbs[2] = { nullptr, g_res.sun_instance_vb };
            UINT str[2] = { sizeof(MeshVertex), INST_STRIDE };
            UINT off[2] = { 0, 0 };
            size_t first = 0;
            DepthSk sk = { g_res.vs_sundepth, g_res.layout_sundepth, g_res.vs_sundepth_sk, nullptr, nullptr };
            while (first < opq) {   // DLS_ALPHA: the opaque partition.
                const int mid = keep[first]->mesh;
                const int lod = keep[first]->lod;   // SHADOW_LOD_DRAWN.
                // CLOTH: a run may not span two buffers, so a cloth caster forms a run of one. SKIN_POOL: nor
                // two slices of one chunk (the pair) - unless the run pulls them through the twin (cast_run_joins).
                const VbRef vb = mesh_vb_ref(mid, keep[first]->slot);
                const SkinPoolChunk* const pc =
                    cast_pool_of(mid, keep[first]->slot, g_res.vs_sundepth_sk);
                size_t last = first + 1;
                while (last < opq && keep[last]->mesh == mid &&
                       keep[last]->lod == lod &&
                       cast_run_joins(vb, pc, keep[last]->mesh, keep[last]->slot,
                                      g_res.vs_sundepth_sk)) ++last;
                sk.use(ctx, pc);
                vbs[0] = vb.vb;
                off[0] = vb.off;
                ctx->IASetVertexBuffers(0, 2, vbs, str, off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                UINT ls = 0, lc = 0;
                mesh_lod_range_of(mid, lod, ls, lc);   // SHADOW_LOD_DRAWN.
                ctx->DrawIndexedInstanced(lc, static_cast<UINT>(last - first),
                                          ls, 0, static_cast<UINT>(first));
                first = last;
            }
            sk.end(ctx);   // SKIN_POOL: the face's own pair again.
            if (opq < keep.size()) {   // DLS_ALPHA: the alpha tail, the sun's own draw.
                if (!sun_draw_alpha_t(ctx, keep, obj, g_res.sun_instance_vb)) fail = true;
                // Back to this bracket's opaque state.
                ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
                ctx->PSSetShader(nullptr, nullptr, 0);
            }

        }
        if (fail) {   // SUN_OBJ: a face left incomplete - never published as valid, and rebuilt next frame.
            g_dls_keys[s] = 0;
            g_dls_map_valid &= ~(1ull << s);
        }

        slice += fn;
    }
    // Hand the sun back what it had (the rasteriser's Get AddRef'd: released either way; the OM save releases itself).
    if (g_res.dls_rast) {
        ctx->RSSetState(prev_rs);
        KH_SAFE_RELEASE(prev_rs);
    }
    om.restore(ctx);
    om.release();
    if (prev_nvp > 0) ctx->RSSetViewports(prev_nvp, prev_vp);
}

// DLSW_OWN_VIEW: can a ball of ours (centre m, radius h) stand, seen from the camera c, in front
// of a world surface inside a light's reach (centre l, radius r)? The world pass multiplies only surfaces
// in reach, and the mask keeps it off our pixels - so a mesh's place in the mask is decided by whether it can cover
// such a surface, not by whether the mesh itself is in reach. True when the two cones of view rays (through the ball,
// through the sphere) meet and the ball's nearest point is not beyond the sphere's far side; conservative (true)
// with the camera inside the ball, and inside the sphere once the far-side test has passed. The reach handed in is
// the pass's (dls_frame): a sphere round the cube a point light's faces shade, not the map's far distance. The
// light's terms (camera c, sphere l / r) are the same for every ball: dlsw_view_light takes them
// once, dlsw_view_covers each ball against them.
struct DlswLight {
    float b[3];   // The camera to the sphere's centre.
    float db;     // Its length.
    float r;      // The sphere's radius.
    float as;     // The half-angle it subtends (read only with the camera outside it).
};
inline void dlsw_view_light(const float c[3], const float l[3], float r, DlswLight& v) {
    v.b[0] = l[0] - c[0];
    v.b[1] = l[1] - c[1];
    v.b[2] = l[2] - c[2];
    v.db = sqrtf(v.b[0] * v.b[0] + v.b[1] * v.b[1] + v.b[2] * v.b[2]);
    v.r = r;
    v.as = v.db > r ? asinf(r / v.db) : 0.0f;
}
inline bool dlsw_view_covers(const float c[3], const float m[3], float h,
                             const DlswLight& v) {
    const float a[3] = { m[0] - c[0], m[1] - c[1], m[2] - c[2] };
    const float* const b = v.b;
    const float da = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    const float db = v.db;
    const float r = v.r;
    if (!(da > h)) return true;   // Inside the ball (or not a number).
    // DLSW_VIEW_FAR: ahead of the sphere test - it holds with the camera inside the sphere too (every surface in
    // reach is within db + r of it); behind it, a camera inside a light's reach made every visible mesh in the scene
    // an owner, however far.
    if (da - h > db + r) return false;   // Wholly beyond every surface in reach.
    if (!(db > r)) return true;   // Inside the sphere (or not a number).
    float cos = (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / (da * db);
    cos = cos < -1.0f ? -1.0f : (cos > 1.0f ? 1.0f : cos);
    return acosf(cos) <= asinf(h / da) + v.as;
}
inline void dls_frame(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_DLS_FRAME);
    if (!ctx) return;
    if (g_dls_frame_cycle == g_topo_cycles) return;
    g_dls_frame_cycle = g_topo_cycles;
    // PROF_ONCE: the GPU stamp past the once-per-cycle return, as render_sun_depth's. A zone reports its last run,
    // and a cycle can call this twice (an injection re-armed for the camera partition or by the rescue, a park flush
    // ahead of the injection): stamped at the head, gDlsFrame was the second call's empty return.
    KH_GPU_SCOPE(ctx, GPU_DLS_FRAME);

    static std::vector<SunCaster> casters;   // Scratch: both callers run on the render thread or in a park.
    casters.clear();
    static std::vector<SunCaster> owners;   // SHADOW_SWITCH: visible non-casters (the mask only).
    owners.clear();

    if (g_dls_n > 0) {
        scene_sync();   // SCENE: live walk by reference, no copy.
        // The reach test is a sphere per light, so the walk visits only the cells the union of those spheres can
        // touch (the grid pads by each cell's largest object radius).
        static std::vector<uint32_t> cand;
        {
            float mn[3] = { 0.0f, 0.0f, 0.0f }, mx[3] = { 0.0f, 0.0f, 0.0f };
            bool any = false;
            for (uint32_t s = 0; s < DLS_MAX; ++s) {
                if (!g_dls[s].live) continue;
                const float r = dls_far(g_dls[s]);
                for (int k = 0; k < 3; ++k) {
                    const float lo = g_dls[s].pos[k] - r;
                    const float hi = g_dls[s].pos[k] + r;
                    if (!any || lo < mn[k]) mn[k] = lo;
                    if (!any || hi > mx[k]) mx[k] = hi;
                }
                any = true;
            }
            cand.clear();
            if (any) scene_grid_query_box(mn, mx, cand);
        }
        objbuf_sync(ctx);   // OBJBUF.
        static std::vector<uint8_t> listed;   // DLSW_OWN_VIEW: the slots the reach walk placed.
        listed.assign(g_scene.objs.size(), 0u);
        for (uint32_t i : cand) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& o = g_scene.objs[i];
            // SHADOW_ACTIVE: casting is the sun's rule (shadow_part, then cast_on below). Owning pixels is
            // not: an unlit mesh neither casts nor receives, yet a visible one the injection drew is our surface at
            // its pixels - left out of the mask, the world pass multiplied the shadow of the surface behind it onto
            // it (DLSW_OWN). So a visible world mesh turned down only for being unlit still joins as an owner.
            const bool part = shadow_part(o);
            if (!part && !(obj_shown(o) && shadow_part_unlit(o))) continue;   // SHOW_INFRONT.
            const float ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
            float he[3];
            world_half_extents(o, he);
            const float hd = sqrtf(he[0] * he[0] + he[1] * he[1] +
                                   he[2] * he[2]);
            bool df_near = false;
            for (uint32_t s = 0; s < DLS_MAX && !df_near; ++s) {
                if (!g_dls[s].live) continue;
                const float lim = dls_far(g_dls[s]) + hd;
                const float dx = ce[0] - g_dls[s].pos[0];
                const float dy = ce[1] - g_dls[s].pos[1];
                const float dz = ce[2] - g_dls[s].pos[2];
                df_near = dx * dx + dy * dy + dz * dz <= lim * lim;
            }
            if (!df_near) {  continue; }
            listed[i] = 1u;
            // SHADOW_LOD_DRAWN: this frame's camera (DLS_LOD_CAM - with or without a published sun).
            // SHADOW_SWITCH: a mesh with castShadow off draws into no map, but a visible one still owns its pixels,
            // which the world pass must not paint (the mask).
            if (part && cast_on(o)) {   // SHADOW_OFF too.
                casters.push_back(sun_caster_of(o, g_sun_setup.sun_cam_now));
            } else if (obj_shown(o)) {   // SHOW_INFRONT.
                owners.push_back(sun_caster_of(o, g_sun_setup.sun_cam_now));
            }
        }
        // DLSW_OWN_VIEW: a visible mesh out of every light's reach casts nothing into these maps, but it still
        // owns its pixels - standing between the camera and a wall a light reaches, left out of the mask it would take
        // the wall's shadow from the world pass. Every visible world mesh the reach walk did not place
        // joins as an owner (the mask only) when it can cover a surface in some light's reach (dlsw_view_covers);
        // camera-less (DLS_LOD_CAM zero), every one joins. The reach is where the pass can shade, not the map's
        // far: a face refuses a receiver by its depth along the face axis (KhDlsFaceUV), so a point light shades the
        // cube of half-size far round it - out to sqrt(3) far at the corners - and PSDlsWorld's own early-out, the
        // anchors' radius (dls_fill_dl_world), is sqrt(3) x 1.05 far. A sphere of the far alone leaves a mesh in
        // front of a cube corner out of the mask. DLSW_SPOT_REACH: a spot wider than 90 degrees shades further out
        // (dls_shade_k), read from its last rendered frustum - this walk precedes the render, so a slot that
        // turns into a wide spot this frame takes the cube's reach for that one frame.
        const bool cam = g_sun_setup.sun_cam_now[0] != 0.0f || g_sun_setup.sun_cam_now[1] != 0.0f ||
            g_sun_setup.sun_cam_now[2] != 0.0f;
        DlswLight vl[DLS_MAX];   // The live slots' terms, once per frame (dlsw_view_light).
        for (uint32_t s = 0; s < DLS_MAX; ++s) {
            if (g_dls[s].live) {
                dlsw_view_light(g_sun_setup.sun_cam_now, g_dls[s].pos,
                                dls_shade_k(s) * 1.05f * dls_far(g_dls[s]), vl[s]);
            }
        }
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i] || listed[i]) continue;
            const RenderObject& o = g_scene.objs[i];
            if (!obj_shown(o) || !shadow_part_unlit(o)) continue;   // shadow_part implies it.
            const float ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
            float he[3];
            world_half_extents(o, he);
            const float hd = sqrtf(he[0] * he[0] + he[1] * he[1] +
                                   he[2] * he[2]);
            bool cov = !cam;
            for (uint32_t s = 0; s < DLS_MAX && !cov; ++s) {
                if (!g_dls[s].live) continue;
                cov = dlsw_view_covers(g_sun_setup.sun_cam_now, ce, hd, vl[s]);
            }
            if (cov) owners.push_back(sun_caster_of(o, g_sun_setup.sun_cam_now));
        }
    }

    // SUN_OBJ: the instancing resources are not required - a face draws per object without them.
    const bool state = g_res.vs && g_res.input_layout && g_res.dss_test_write && g_res.rast_sun &&
                       g_res.composite_cb && g_res.composite_frame_cb;
    if (!state || casters.empty()) {
        // Nothing to render into, or nothing that could cast. The per-light skip keys are untouched, so a caster
        // returning next frame reuses its slices.
        g_dls_map_valid = 0;
        for (uint32_t s = 0; s < DLS_MAX; ++s) g_dls_render.facen[s] = 0;
        g_dlsw_casters.clear();
        return;
    }

    // The standalone sun bracket, minus the tiers: own-draw exclusion, full state save, the depth-only instanced
    // pipeline. dls_render saves and restores its own targets, viewport and rasteriser; StateBackup restores both
    // vertex-buffer slots and t36.
    if (g_res.vs_sundepth && g_res.layout_sundepth && g_res.obj_srv) {   // SUN_INST_CAP: the faces' stream.
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (dev) {
            sun_inst_ensure(dev, casters.size());
            dev->Release();
        }
    }
    const bool pinj = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup bk;
    bk.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER: the faces' alpha casters.
    ctx->IASetInputLayout(g_res.layout_sundepth);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_sundepth, nullptr, 0);
    objbuf_bind(ctx);   // OBJBUF (StateBackup restores t39).
    ctx->PSSetShader(nullptr, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rast_sun);   // dls_rast supersedes it inside (DLS_DEPTHCLIP).

    dls_render(ctx, casters, owners);

    bk.restore(ctx);
    g_ro.in_injection = pinj;
}
}