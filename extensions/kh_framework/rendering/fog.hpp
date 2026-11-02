#pragma once

// The feature half (rendering_integration_features.hpp): the fog records' packing (fog, fog scatter, dynamicLightFog's
// medium). Never include it on its own.
namespace RenderIntegration {

// KH_FX_FOGREC: the scene's KH fog passes as fog scatter takes them (HLSL twin: cb.hlsl's KhFxFogPasses) - up to two
// fullscreen distance-fog passes, neither localized nor banded nor on an additive, multiply or screen blend
// (KH_FOGREC_BLEND), past the chain's own opacity skip, the most opaque first - into khfr_out0 / khfr_out1 as
// [startDist, endDist, skyAmount, opacity] (both zeroed first; a record not filled stays zero = none). Consumers: the
// builtin fog scatter's system lanes fx1 / fx2 (kh_fogscatter_pack), a user effect's fx_fog (the scene chain) and
// dynamicLightFog's (kh_dlf_fog_pack). KH_DLF_FOGREC: khfr_pick, when given, receives the pass behind each record
// (nullptr where none).
inline void kh_fog_records_pack(float khfr_out0[4], float khfr_out1[4],
                                const std::vector<std::pair<uint64_t, RenderObject>>& khfs_list,
                                const RenderObject** khfr_pick = nullptr) {
    memset(khfr_out0, 0, 4 * sizeof(float));
    memset(khfr_out1, 0, 4 * sizeof(float));
    const RenderObject* khfs_top[2] = { nullptr, nullptr };

    for (const auto& khfs_f : khfs_list) {
        const RenderObject& khfs_o = khfs_f.second;
        if (khfs_o.effect != static_cast<int>(EffectId::Fog)) continue;
        if (khfs_o.localized || khfs_o.banded) continue;
        // KH_FOGREC_BLEND: a record is a medium of transmittance 1 - a f (a = opacity; effect 13's lerp(scene, colour,
        // f) under KhFxFinish's normal blend). Additive and screen add a term to the picture, and multiply scales it by
        // 1 - a + a x lerp(scene, colour, f) - a factor of the scene's own brightness, there at f = 0 too: none of the
        // three is a medium that dims by distance, so none is a record. Lighten and darken give normal's lerp wherever
        // they change a pixel (the fog brighter / darker than the scene there) and leave the rest: taken - a medium
        // that over-states them where they leave the scene.
        if (khfs_o.blend_mode >= 1 && khfs_o.blend_mode <= 3) continue;
        if (khfs_o.color[3] <= 0.001f) continue;   // The chain's own no-op skip, mirrored.

        if (!khfs_top[0] || khfs_o.color[3] > khfs_top[0]->color[3]) {
            khfs_top[1] = khfs_top[0];
            khfs_top[0] = &khfs_o;
        } else if (!khfs_top[1] || khfs_o.color[3] > khfs_top[1]->color[3]) {
            khfs_top[1] = &khfs_o;
        }
    }

    if (khfr_pick) {
        khfr_pick[0] = khfs_top[0];
        khfr_pick[1] = khfs_top[1];
    }
    for (int khfs_i = 0; khfs_i < 2; ++khfs_i) {
        if (!khfs_top[khfs_i]) break;
        float* khfs_dst = khfs_i == 0 ? khfr_out0 : khfr_out1;
        khfs_dst[0] = khfs_top[khfs_i]->fx[0];
        khfs_dst[1] = khfs_top[khfs_i]->fx[1];
        khfs_dst[2] = khfs_top[khfs_i]->fx[2];
        khfs_dst[3] = khfs_top[khfs_i]->color[3];
    }
}
// Fog scattering pack, CPU side (HLSL twin: KhFsFog's fog-pass lanes, fxParams1 / 2). Mesh-borne fogscatter never
// routes here: its lanes stay zeroed, so a mesh footprint scatters by the engine fog alone.
inline void kh_fogscatter_pack(ConstantData& cbd,
                               const std::vector<std::pair<uint64_t, RenderObject>>& khfs_list) {
    kh_fog_records_pack(cbd.fx1, cbd.fx2, khfs_list);
}

// KH_DLF_FOGREC (HLSL twin: effect.hlsl KhDlfPassMedium): dynamicLightFog's view of the fog passes - fog scatter's
// two records (kh_fog_records_pack) into fx_fog, and fx2[0] / fx2[1] = 1 where that record's pass draws BEFORE this
// one (khdf_self, the dynamicLightFog pass's own entry in the same list: the chain draws the list in order), 0 where
// it draws after or there is none. The pass's fx2 is otherwise unread (its params end at fx1).
inline void kh_dlf_fog_pack(ConstantData& cbd, const std::vector<std::pair<uint64_t, RenderObject>>& khdf_list,
                            const RenderObject* khdf_self) {
    const RenderObject* khdf_pick[2] = { nullptr, nullptr };
    kh_fog_records_pack(cbd.fx_fog[0], cbd.fx_fog[1], khdf_list, khdf_pick);
    for (int khdf_i = 0; khdf_i < 2; ++khdf_i) {
        bool khdf_before = false;
        if (khdf_pick[khdf_i]) {
            for (const auto& khdf_f : khdf_list) {   // The first of the two met is the earlier.
                if (&khdf_f.second == khdf_self) break;
                if (&khdf_f.second == khdf_pick[khdf_i]) { khdf_before = true; break; }
            }
        }
        cbd.fx2[khdf_i] = khdf_before ? 1.0f : 0.0f;
    }
}
// KH_DLF_MEDIUM0 - whether dynamicLightFog's medium is zero everywhere, from the lanes its gather reads, as filled
// for the pass (khdm_cb: the chain's khfp_cbd - its fog_params / fog_engine the frame template's, which the flush
// uploaded as b1; fx0 the pass's; fx_fog after kh_dlf_fog_pack). Then every sample of PSDlfGather (effect.hlsl)
// continues at `!(sg * pm.x > 0) && !(pm.z > 0)`: KhDlfSigma (effect.hlsl) is max(s, 0) x max(fogScale, 0) +
// max(baseDensity, 0), with s 0 when the fog is off (fogParams.w) and otherwise the density coefficient (fogEngine.x
// with the engine terms, fogEngine.w, else fogParams.x x 0.0153) times a finite exp (capped at e^60) - so sg is
// +-0 or NaN at every height; KhDlfPassMedium is (1, 0, 0) with no record (khFxFog[i].w > 0.001), and with fogScale
// at or below 0 its z is 0 x finite or NaN and x lies in [0, 1]. `!(x > 0)` is the shader's reading of a NaN lane
// (max(NaN, 0) = 0); a denormal the GPU flushes reads positive here: the test errs toward the work. TWIN:
// KhDlfSigma, KhDlfPassMedium and the gather's per-sample test - an edit there is checked here.
inline bool kh_dlf_medium_zero(const ConstantData& khdm_cb) {
    const bool khdm_rec = khdm_cb.fx_fog[0][3] > 0.001f || khdm_cb.fx_fog[1][3] > 0.001f;
    const bool khdm_fs0 = !(khdm_cb.fx0[1] > 0.0f);   // fogScale.
    const float khdm_k = khdm_cb.fog_engine[3] >= 0.5f ? khdm_cb.fog_engine[0] : khdm_cb.fog_params[0];
    const bool khdm_s0 = !(khdm_cb.fog_params[3] >= 0.5f) || khdm_fs0 || !(khdm_k > 0.0f);
    return !(khdm_cb.fx0[2] > 0.0f) && khdm_s0 && (!khdm_rec || khdm_fs0);   // baseDensity; the game's fog; records.
}
}