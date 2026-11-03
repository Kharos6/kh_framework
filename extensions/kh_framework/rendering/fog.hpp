#pragma once

// The feature half (rendering_integration_features.hpp): the fog records' packing (fog, fog scatter, dynamicLightFog's
// medium). Never include it on its own.
namespace RenderIntegration {

// FX_FOGREC: the scene's KH fog passes as fog scatter takes them (HLSL twin: cb.hlsl's KhFxFogPasses) - up to two
// fullscreen distance-fog passes, neither localized nor banded nor on an additive, multiply or screen blend
// (FOGREC_BLEND), past the chain's own opacity skip, the most opaque first - into out0 / out1 as
// [startDist, endDist, skyAmount, opacity] (both zeroed first; a record not filled stays zero = none). Consumers: the
// builtin fog scatter's system lanes fx1 / fx2 (fogscatter_pack), a user effect's fx_fog (the scene chain) and
// dynamicLightFog's (dlf_fog_pack). DLF_FOGREC: pick, when given, receives the pass behind each record
// (nullptr where none).
inline void fog_records_pack(float out0[4], float out1[4],
                             const std::vector<std::pair<uint64_t, RenderObject>>& list,
                             const RenderObject** pick = nullptr) {
    memset(out0, 0, 4 * sizeof(float));
    memset(out1, 0, 4 * sizeof(float));
    const RenderObject* top[2] = { nullptr, nullptr };

    for (const auto& f : list) {
        const RenderObject& o = f.second;
        if (o.effect != static_cast<int>(EffectId::Fog)) continue;
        if (o.localized || o.banded) continue;
        // FOGREC_BLEND: a record is a medium of transmittance 1 - a f (a = opacity; effect 13's lerp(scene, colour,
        // f) under KhFxFinish's normal blend). Additive and screen add a term to the picture, and multiply scales it by
        // 1 - a + a x lerp(scene, colour, f) - a factor of the scene's own brightness, there at f = 0 too: none of the
        // three is a medium that dims by distance, so none is a record. Lighten and darken give normal's lerp wherever
        // they change a pixel (the fog brighter / darker than the scene there) and leave the rest: taken - a medium
        // that over-states them where they leave the scene.
        if (o.blend_mode >= 1 && o.blend_mode <= 3) continue;
        if (o.color[3] <= 0.001f) continue;   // The chain's own no-op skip, mirrored.

        if (!top[0] || o.color[3] > top[0]->color[3]) {
            top[1] = top[0];
            top[0] = &o;
        } else if (!top[1] || o.color[3] > top[1]->color[3]) {
            top[1] = &o;
        }
    }

    if (pick) {
        pick[0] = top[0];
        pick[1] = top[1];
    }
    for (int i = 0; i < 2; ++i) {
        if (!top[i]) break;
        float* dst = i == 0 ? out0 : out1;
        dst[0] = top[i]->fx[0];
        dst[1] = top[i]->fx[1];
        dst[2] = top[i]->fx[2];
        dst[3] = top[i]->color[3];
    }
}
// Fog scattering pack, CPU side (HLSL twin: KhFsFog's fog-pass lanes, fxParams1 / 2). Mesh-borne fogscatter never
// routes here: its lanes stay zeroed, so a mesh footprint scatters by the engine fog alone.
inline void fogscatter_pack(ConstantData& cbd,
                            const std::vector<std::pair<uint64_t, RenderObject>>& list) {
    fog_records_pack(cbd.fx1, cbd.fx2, list);
}

// DLF_FOGREC (HLSL twin: effect.hlsl KhDlfPassMedium): dynamicLightFog's view of the fog passes - fog scatter's
// two records (fog_records_pack) into fx_fog, and fx2[0] / fx2[1] = 1 where that record's pass draws BEFORE this
// one (self, the dynamicLightFog pass's own entry in the same list: the chain draws the list in order), 0 where
// it draws after or there is none. The pass's fx2 is otherwise unread (its params end at fx1).
inline void dlf_fog_pack(ConstantData& cbd, const std::vector<std::pair<uint64_t, RenderObject>>& list,
                         const RenderObject* self) {
    const RenderObject* pick[2] = { nullptr, nullptr };
    fog_records_pack(cbd.fx_fog[0], cbd.fx_fog[1], list, pick);
    for (int i = 0; i < 2; ++i) {
        bool before = false;
        if (pick[i]) {
            for (const auto& f : list) {   // The first of the two met is the earlier.
                if (&f.second == self) break;
                if (&f.second == pick[i]) { before = true; break; }
            }
        }
        cbd.fx2[i] = before ? 1.0f : 0.0f;
    }
}
// DLF_MEDIUM0 - whether dynamicLightFog's medium is zero everywhere, from the lanes its gather reads, as filled
// for the pass (cb: the chain's cbd - its fog_params / fog_engine the frame template's, which the flush
// uploaded as b1; fx0 the pass's; fx_fog after dlf_fog_pack). Then every sample of PSDlfGather (effect.hlsl)
// continues at `!(sg * pm.x > 0) && !(pm.z > 0)`: KhDlfSigma (effect.hlsl) is max(s, 0) x max(fogScale, 0) +
// max(baseDensity, 0), with s 0 when the fog is off (fogParams.w) and otherwise the density coefficient (fogEngine.x
// with the engine terms, fogEngine.w, else fogParams.x x 0.0153) times a finite exp (capped at e^60) - so sg is
// +-0 or NaN at every height; KhDlfPassMedium is (1, 0, 0) with no record (khFxFog[i].w > 0.001), and with fogScale
// at or below 0 its z is 0 x finite or NaN and x lies in [0, 1]. `!(x > 0)` is the shader's reading of a NaN lane
// (max(NaN, 0) = 0); a denormal the GPU flushes reads positive here: the test errs toward the work. TWIN:
// KhDlfSigma, KhDlfPassMedium and the gather's per-sample test - an edit there is checked here.
inline bool dlf_medium_zero(const ConstantData& cb) {
    const bool rec = cb.fx_fog[0][3] > 0.001f || cb.fx_fog[1][3] > 0.001f;
    const bool fs0 = !(cb.fx0[1] > 0.0f);   // fogScale.
    const float k = cb.fog_engine[3] >= 0.5f ? cb.fog_engine[0] : cb.fog_params[0];
    const bool s0 = !(cb.fog_params[3] >= 0.5f) || fs0 || !(k > 0.0f);
    return !(cb.fx0[2] > 0.0f) && s0 && (!rec || fs0);   // baseDensity; the game's fog; records.
}
}