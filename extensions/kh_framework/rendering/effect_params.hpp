#pragma once

// The feature half (rendering_integration_features.hpp): the effect ids and parameters: the interned effect strings and
// texture sets, set_effect_params, the per-pass effect constants. Never include it on its own.
namespace RenderIntegration {

inline void kh_fill_user_obj_cb(ConstantData& cbd, const RenderObject& o);   // KH_USER_LANES, with the fx fill.

// KH_FX_INTERN: resolved user-shader / LUT paths and effect texture paths (KH_FX_TEX's user0 .. user5), interned for
// the process (a handful of strings; never released - a name -> pointer table only, which nothing resolves a path
// through, so no session reads another's).
static std::vector<std::unique_ptr<std::string>> g_fx_intern_pool;
static std::unordered_map<std::string, const std::string*> g_fx_intern_map;
static std::mutex g_fx_intern_mu;

inline const std::string* kh_intern_str(const std::string& khis_s) {
    if (khis_s.empty()) return nullptr;
    std::lock_guard<std::mutex> khis_l(g_fx_intern_mu);
    auto khis_it = g_fx_intern_map.find(khis_s);
    if (khis_it != g_fx_intern_map.end()) return khis_it->second;
    g_fx_intern_pool.emplace_back(new std::string(khis_s));
    const std::string* khis_p = g_fx_intern_pool.back().get();
    g_fx_intern_map.emplace(khis_s, khis_p);
    return khis_p;
}
static std::unordered_map<std::string, std::unique_ptr<KhFxTexSet>> g_fx_texset_pool;   // Key: the set's bytes.
static std::mutex g_fx_texset_mu;
inline const KhFxTexSet* kh_fx_tex_intern(const KhFxTexSet& khti_s) {
    std::string khti_k(reinterpret_cast<const char*>(khti_s.path), sizeof(khti_s.path));
    khti_k.push_back(static_cast<char>(khti_s.srgb));
    std::lock_guard<std::mutex> khti_l(g_fx_texset_mu);
    auto& khti_e = g_fx_texset_pool[khti_k];
    if (!khti_e) khti_e.reset(new KhFxTexSet(khti_s));
    return khti_e.get();
}

static constexpr int KH_MAX_EFFECT = 31;
// KH_DLF: dynamicLightFog's own passes, drawn by the chain alone (above KH_MAX_EFFECT: no script reaches them).
static constexpr int KH_DLF_GATHER = 32;
static constexpr int KH_DLF_FILTER = 33;
static float    g_rain_prev_cam[3] = {};
static bool     g_rain_prev_valid = false;
static uint64_t g_rain_prev_us = 0;

inline const std::string& kh_fx_shader_of(const RenderObject& o) {
    static const std::string khfs_none;
    return o.fx_shader ? *o.fx_shader : khfs_none;
}

// Writes effect parameters into obj.fx with per-effect defaults for omitted values; Pulse's position and the sun
// flare's direction arrive in SQF axes ([x, y, zASL] / [x, y, z]) and convert to engine space here. False, obj.fx
// unchanged, if a provided entry is not a finite number (nil keeps the default).
inline bool set_effect_params(RenderObject& obj, const auto_array<game_value>* params) {
    // Every ...Px lane below is in pixels of a 1080-row picture (KH_FX_PX_REF, cb.hlsl's KhFxPx).
    static const float defaults[KH_MAX_EFFECT + 1][12] = {
        {},
        {},
        { 1.0f, 1.0f, 1.0f, 1.0f },   // 2 colorgrade: sat, contrast, brightness, gamma.
        { 0.5f, 0.5f },   // 3 vignette: startRadius, softness.
        { 3.0f },
        { 0.06f, 24.0f, 1.5f, 0.7f, 0.3f },   // 5 grain: amount, fps, grainSizePx, lumaResponse,
                                              // Chroma.
        { 0.5f },
        { 2.0f },
        { 1.0f, 0.8f, 2.0f },   // 8 bloom: threshold, intensity, radiusPx.
        { 6.0f, 6.0f, 2.0f },   // 9 distortion: amplitudePx, frequency, speed.
        { 4.0f, 2.0f, 0.25f, 1.5f },   // 10 outline: depthEdge, lumEdge, sceneDarken, glowBoost.
        { 0.0f, 0.0f, 0.0f, 50.0f, 3.0f, 2.0f },   // 11 pulse: x, y, zASL, radius, bandWidth,
                                                   // Intensity.
        { 1.0f, 1.2f, 5.0f },   // 12 halation: threshold, intensity, radiusPx.
        { 200.0f, 1200.0f, 1.0f },   // 13 fog: startDist, endDist, skyAmount.
        { 1.1f, 0.7f, 5.0f, 0.35f, 0.45f, 0.6f, 3.0f },   // 14 lensflare: threshold, intensity,
                                                          // Ghosts, spacing, haloRadius,
                                                          // haloIntensity, chromaPx.
        { 1.2f, 1.2f, 220.0f, 2.0f, 0.0f },   // 15 anamorphic: threshold, intensity, lengthPx,
                                              // falloffPow, vertical.
        { 0.0f, 0.5f, 0.8f, 0.15f, 4.0f, 0.6f, 1.2f },   // 16 sunflare: dirX, dirY, dirZ (SQF
                                                         // Axes), size, ghostDots, ringIntensity,
                                                         // starburst.
        { 1.0f, 8.0f, 24.0f, 12.0f, 6.0f, 0.5f, 0.35f, 0.7f },
        { 0.35f, 22.0f },   // 18 clarity: strength, radiusPx.
        { 1.6f, 16.0f, 0.6f },   // 19 deband: threshold(1/255), rangePx, grain(1/255).
        { 0.6f, 1.0f, 0.35f, 1.0f },   // 20 rainlens: intensity, speed, condensation, refract
                                       // (fx[4..7] system: camera velocity xyz, rain clock w).
        // 21 crt: lane 10 = maskSizePx. KH_CRT_BANDLIMIT: lineCount 320 - 3.4 rows a line at the centre of a
        // 1080-row frame and 3.0 at its top and bottom edges at the default curvature (540 was two, which cannot be
        // drawn: the shader fades such lines out; 360 sat on the fade's start at the centre, so the warp's packing
        // faded every line off the centre row).
        { 0.12f, 0.45f, 320.0f, 0.35f, 1.5f, 0.25f, 0.3f, 0.06f, 4.0f, 1.5f, 3.0f },
        { 0.7f, 4.0f, 12.0f, 0.15f, 2.0f, 1.0f, 300.0f, 3.0f, 0.65f },
        { 0.5f, 0.0f, 12.0f },   // 23 fogscatter: intensity, radiusPx, taps.
        {}, {}, {}, {}, {}, {}, {},   // 24 - 30: internal side ids (no script's effect).
        { 1.0f, 1.0f, 0.0f, 0.6f, 16.0f, 250.0f, 2.0f, 24.0f },   // 31 dynamicLightFog (KH_DLF): intensity,
                                                                  // fogScale, baseDensity, anisotropy, steps,
                                                                  // maxDistance, shadows, maxLights.
    };

    // LUT default row: strength = 1 (a zero default would be a silent no-op; the shader lerps by fx[0]).
    static const float khlut_defaults[12] = { 1.0f };
    const int e = (obj.effect >= 0 && obj.effect <= KH_MAX_EFFECT) ? obj.effect : 0;
    // KH_FX_FINITE: a NaN / inf entry is refused like a non-number, and the row is built in a local and stored whole,
    // so a refused call leaves obj.fx as it was. KH_ARG_EXACT: more than 12 entries is an error, not a truncation.
    if (params && params->size() > 12) return false;
    float khfp_fx[12];
    memcpy(khfp_fx, obj.effect == KH_EFFECT_LUT ? khlut_defaults : defaults[e], sizeof(khfp_fx));

    if (params) {
        for (size_t i = 0; i < 12 && i < params->size(); ++i) {
            if ((*params)[i].is_nil()) continue;   // Nil keeps the default.
            if ((*params)[i].type_enum() != game_data_type::SCALAR) return false;   // Wrong type: hard error.
            khfp_fx[i] = static_cast<float>((*params)[i]);
            if (!std::isfinite(khfp_fx[i])) return false;
        }
    }
    memcpy(obj.fx, khfp_fx, sizeof(obj.fx));

    if (e == static_cast<int>(EffectId::Pulse) || e == static_cast<int>(EffectId::SunFlare)) {
        // SQF [x, y, zASL] -> engine [x, zASL, y].
        const float sx = obj.fx[0], sy = obj.fx[1], sz = obj.fx[2];
        obj.fx[0] = sx;
        obj.fx[1] = sz;
        obj.fx[2] = sy;
    }

    return true;
}

// The 0 - 1 intensity multiplier; sets 'expired' once the fade-out has completed.
inline float lifetime_envelope(const RenderObject& o, float now, bool& expired) {
    expired = false;
    if (!o.timed) return 1.0f;
    float age = now - o.birth_time;
    if (age < 0.0f) age = 0.0f;
    if (age < o.fade_in) return age / (o.fade_in > 0.001f ? o.fade_in : 0.001f);
    age -= o.fade_in;
    if (age < o.hold_dur) return 1.0f;
    age -= o.hold_dur;
    if (age < o.fade_out) return 1.0f - age / (o.fade_out > 0.001f ? o.fade_out : 0.001f);
    expired = true;
    return 0.0f;
}

// Colour + the twelve effect parameters, the rain lens's system lanes overriding fx1 (after KH_USER_LANES' lanes).
inline void kh_fill_fx_params_cb(ConstantData& cbd, const RenderObject& o) {
    kh_fill_user_obj_cb(cbd, o);   // KH_USER_LANES.
    memcpy(cbd.color, o.color, sizeof(cbd.color));
    memcpy(cbd.fx0, o.fx, sizeof(cbd.fx0));
    memcpy(cbd.fx1, o.fx + 4, sizeof(cbd.fx1));
    memcpy(cbd.fx2, o.fx + 8, sizeof(cbd.fx2));
    if (o.effect == static_cast<int>(EffectId::RainLens)) {
        cbd.fx1[0] = g_rain_vel[0];
        cbd.fx1[1] = g_rain_vel[1];
        cbd.fx1[2] = g_rain_vel[2];
        cbd.fx1[3] = static_cast<float>(g_rain_phase);   // The integrated rain clock.
    }
}
// KH_NEARZ_READER - the passes that read the depth (and through LoadDepthPS the marker): the localization and band
// masks (KhFxFinish: KhWorldPosRelFenced, LoadDepthPS), outline, pulse and distance fog (PSEffect), the sun flare (its
// pass and its visibility side draw, 29), SSGI (gather, a-trous, resolve), fog scatter (the pass and its side draw, 28)
// and dynamicLightFog (gather, filter, composite) with their linear-depth frames (PSFxLinZ), and a user shader, which
// may. Every other built-in - and the fused point stages (1, 2, 3, 5), the glow / anamorphic pyramids and the LUT -
// reads neither. The scene chain's depth predicate (flush_locked's needs_depth: the read-only depth swap, the depth
// pair) and the marker's reader test (kh_nzm_wanted) are this one rule. A reader that appears between the injection's
// test and the chain (a script's change landing at the flush's sync) gets its first frame without the marker (the mark
// is gone by the chain). At SSAO strength 0 the injection reaches ensure_depth_srv only through kh_ssao_pre, so with no
// depth reader an FSAA toggle's wipe (kh_msaa_toggle_wipe) waits for the flush's own trigger later in that frame.
// KH_VM_SEE arms these same readers (shadowMeta2.w) with the scene's depth behind the view model's translucent planes.
inline bool kh_fx_needs_depth(const RenderObject& o) {
    return o.localized || o.banded ||
           o.effect == static_cast<int>(EffectId::Outline) ||
           o.effect == static_cast<int>(EffectId::Pulse) ||
           o.effect == static_cast<int>(EffectId::Fog) ||
           o.effect == static_cast<int>(EffectId::SunFlare) ||
           o.effect == static_cast<int>(EffectId::Ssgi) ||   // Gather + resolve read live depth.
           o.effect == static_cast<int>(EffectId::Fogscatter) ||   // Live depth is the scatter's distance authority.
           o.effect == static_cast<int>(EffectId::DynLightFog) ||   // KH_DLF: the march ends at the depth.
           o.effect == KH_EFFECT_CUSTOM;   // User shaders may sample depth.
}
// KH_FX_DEPTH_DROP - the readers that stand down for a frame without the depth SRV (flush_locked's depth_fx_ready
// false): every kh_fx_needs_depth reader but an unbanded, unlocalized outline (its depth edges go to 0; its luma edges
// and darkening still draw) or user effect (it may read no depth). One rule for the scene chain's passes (dropped:
// effect 0) and for the effect meshes (skipped by the draw loop; a mesh caller adds effect > 0 -
// a banded solid is a needs_depth reader by its band, which SolidMask measures without the depth). The UI lane
// drops every reader without its depth SRV, those two included (flush_ui_locked's wants_depth).
inline bool kh_fx_depth_drop(const RenderObject& o) {
    const bool khfd_keep = (o.effect == KH_EFFECT_CUSTOM || o.effect == static_cast<int>(EffectId::Outline)) &&
                           !o.localized && !o.banded;
    return kh_fx_needs_depth(o) && !khfd_keep;
}
}