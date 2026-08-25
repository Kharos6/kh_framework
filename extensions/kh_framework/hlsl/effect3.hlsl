// g_hlsl_effect3 - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(    else if (effect == 23)   // fogscatter: [intensity, maxRadiusPx (0 = auto), samples 4..24];
    {
        float2 khfs_res = float2(fxMeta.z, fxMeta.w);
        float khfs_m00 = max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f);
        float khfs_m11 = max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f);
        float khfs_in = max(fxParams0.x, 0.0f);
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f)
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        float khfs_cd = LinDepth(LoadDepthPS(px));
        float khfs_sc = saturate(KhFsFog(float2(px), khfs_cd, khfs_res, khfs_m00, khfs_m11) * khfs_in);
        float khfs_c1 = 3.0f * khfs_rm * khfs_rm / (float)khfs_n;
        float khfs_rc = max(khfs_rm * khfs_sc, 1.0f);
        float khfs_ws = (1.0f - khfs_sc) + khfs_sc * khfs_c1 / (khfs_rc * khfs_rc);
        float3 khfs_acc = scene * khfs_ws;
        float khfs_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                              + 0.00583715f * i.pos.y));
        float khfs_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                               + 0.00583715f * (i.pos.y + 5.588238f)));
        float khfs_rot = khfs_ig * 6.2831853f;
        float khfs_hn = (float)((khfs_n + 1) >> 1);

        [loop] for (int khfs_k = 0; khfs_k < khfs_n; ++khfs_k)
        {
            int khfs_kp = khfs_k >> 1;
            float khfs_an = khfs_kp * 2.3999632f + khfs_rot + (khfs_k & 1) * 3.14159265f;
            float khfs_sr = max(sqrt((khfs_kp + khfs_ig2) / khfs_hn) * khfs_rm, 1.0f);
            int2 khfs_sp = int2(float2(px) + 0.5f + float2(cos(khfs_an), sin(khfs_an)) * khfs_sr);
            // off-screen taps are absent INFORMATION (the ssgi normalization
            // finding): skipping them leaves the closing sum-normalization to
            // renormalize, so edge receivers lean on their surviving weights
            // instead of dimming.
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            float khfs_sd = LinDepth(LoadDepthPS(khfs_sp));
            float khfs_ss = saturate(KhFsFog(float2(khfs_sp), khfs_sd, khfs_res, khfs_m00, khfs_m11) * khfs_in);
            if (khfs_sd > khfs_cd) khfs_ss = min(khfs_ss, khfs_sc);   // deflection gate
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // this source's disc does not reach
            float khfs_w = khfs_ss * khfs_c1 * (1.0f - khfs_sr / khfs_rk) / max(khfs_rk * khfs_rk, 1.0f);
            khfs_acc += SampleScene(khfs_sp) * khfs_w;
            khfs_ws += khfs_w;
        }

        outc = khfs_acc / max(khfs_ws, 1e-4f);
    }
)HLSL" R"HLSL(    else if (effect == 101)   // 3D LUT grade (.cube, effect KH_EFFECT_LUT): [strength]
    {
        // Input is clamped to the LUT's [0,1] domain (display-referred.cube
        // semantics; the loader resamples non-identity domains onto [0,1], so
        // no domain math lives here). GetDimensions keeps the branch CB-free.
        uint khlW, khlH, khlD;
        khLut.GetDimensions(khlW, khlH, khlD);

        if (khlW >= 2)
        {
            // The WRITE WINDOW consumes post-tonemap LDR already - direct
            // lookup is correct there - so the sandwich keys on the phase
            // discriminator (scene lanes w <= 1.0; the LUT never rides the
            // custom 1.25 or spill 3 lanes). Encode -> LUT -> decode puts the
            // lookup where the artist designed it and hands the tonemap a
            // linear result.
            bool khlSand = fxParams0.y < 0.5f ? (centerSize.w < 1.5f)
                                              : (fxParams0.y >= 1.5f);
            float3 khlIn = saturate(scene);
            if (khlSand) khlIn = pow(khlIn, 1.0f / 2.2f);
            float3 khlC = khlIn * (float)(khlW - 1);
            int khlHi = (int)khlW - 2;
            int3 khlI0 = clamp(int3(khlC), int3(0, 0, 0), int3(khlHi, khlHi, khlHi));
            float3 khlF = khlC - (float3)khlI0;
            float khlR = khlF.x, khlG = khlF.y, khlB = khlF.z;
            float3 khlV000 = KhLutV(khlI0);
            float3 khlV111 = KhLutV(khlI0 + int3(1, 1, 1));
            float3 khlOut;

            if (khlR >= khlG && khlG >= khlB)
                khlOut = (1.0f - khlR) * khlV000 + (khlR - khlG) * KhLutV(khlI0 + int3(1, 0, 0))
                       + (khlG - khlB) * KhLutV(khlI0 + int3(1, 1, 0)) + khlB * khlV111;
            else if (khlR >= khlB && khlB >= khlG)
                khlOut = (1.0f - khlR) * khlV000 + (khlR - khlB) * KhLutV(khlI0 + int3(1, 0, 0))
                       + (khlB - khlG) * KhLutV(khlI0 + int3(1, 0, 1)) + khlG * khlV111;
            else if (khlB >= khlR && khlR >= khlG)
                khlOut = (1.0f - khlB) * khlV000 + (khlB - khlR) * KhLutV(khlI0 + int3(0, 0, 1))
                       + (khlR - khlG) * KhLutV(khlI0 + int3(1, 0, 1)) + khlG * khlV111;
            else if (khlG >= khlR && khlR >= khlB)
                khlOut = (1.0f - khlG) * khlV000 + (khlG - khlR) * KhLutV(khlI0 + int3(0, 1, 0))
                       + (khlR - khlB) * KhLutV(khlI0 + int3(1, 1, 0)) + khlB * khlV111;
            else if (khlG >= khlB && khlB >= khlR)
                khlOut = (1.0f - khlG) * khlV000 + (khlG - khlB) * KhLutV(khlI0 + int3(0, 1, 0))
                       + (khlB - khlR) * KhLutV(khlI0 + int3(0, 1, 1)) + khlR * khlV111;
            else
                khlOut = (1.0f - khlB) * khlV000 + (khlB - khlG) * KhLutV(khlI0 + int3(0, 0, 1))
                       + (khlG - khlR) * KhLutV(khlI0 + int3(0, 1, 1)) + khlR * khlV111;

            // Decode the graded result back to linear when sandwiched
            // (negative lattice values - legal in.cube - clamp before the
            // pow; the strength lerp below then mixes in the scene's own
            // space either way).
            if (khlSand) khlOut = pow(max(khlOut, 0.0f), 2.2f);

            // color.rgb tints AFTER the grade (parity with the builtin
            // family's tint role); strength lerps against the untouched scene
            // BEFORE the localization/band masks, which then multiply in as
            // everywhere else.
            outc = lerp(scene, khlOut * color.rgb, saturate(fxParams0.x));
        }
    }

    if (localParams1.y > 0.5f)
    {
        float khlm_d;
        float3 nd3 = abs(KhWorldPosFenced(px, uv, khlm_d) - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        // normalized distance: 1.0 = the mask surface (ellipsoid or mesh)
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // cube (Chebyshev)
                 : length(nd3);   // sphere/ellipsoid
        float mask = 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
        // KH_LOCAL_INVERSE (26720): localRadii.w >= 0.5 complements the
        // mask - the effect reaches everything EXCEPT the volume, the
        // falloff band included, the sky (fenced depth, nd >> 1) included.
        // C++ twin local_radii[3] (addLocalPostFX 'inverse').
        if (localRadii.w >= 0.5f) mask = 1.0f - mask;
        outc = lerp(scene, outc, mask);
    }

    // Camera-distance band mask: full strength for scene distances within
    // [min, max], fading over 'falloff' meters at both edges. max <= 0 means
    // unbounded far (sky included). Combines multiplicatively with the
    // world-space localization mask above.
    if (bandParams.w > 0.5f)
    {
        float d = LinDepth(LoadDepthPS(px));
        float fall = max(bandParams.z, 0.01f);
        float mask = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            mask *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        outc = lerp(scene, outc, mask);
    }

    // UI-coverage destination mask (write-window MASKED lane, centerSize.w =
    // 2): the effect vanishes smoothly off the UI.
    if (centerSize.w > 1.5f && centerSize.w < 2.5f)
        outc = lerp(scene, outc, KhUiCov(px));   // spill = w 3, excluded

    int bm = (int)sizeAxes.w;
    if (centerSize.w > 0.5f)
    {
        float a = color.a;
        float3 mixed = lerp(scene, outc, a);
        float3 comp;
        if (bm == 1)      comp = scene + outc * a;   // additive
        else if (bm == 2) comp = scene * lerp(float3(1.0f, 1.0f, 1.0f), outc, a);   // multiply
        else if (bm == 3) comp = scene + outc * a - scene * outc * a;   // screen
        else if (bm == 4) comp = max(scene, mixed);   // lighten
        else if (bm == 5) comp = min(scene, mixed);   // darken
)HLSL" R"HLSL(        else              comp = mixed;   // normal

        if (centerSize.w > 1.5f) {
            float4 khuRaw = sceneColor.Load(int3(clamp(px, int2(0, 0),
                int2((int)fxMeta.z - 1, (int)fxMeta.w - 1)), 0));
            if (centerSize.w > 2.5f)
                comp += khuRaw.rgb * (1.0f - khuRaw.a);
            // fused point-op stages ride the SAME draw - applied here, after
            // the stage-0 composite (and the spill term) completed, exactly
            // where the follower passes used to sample. Coverage rides
            // through unchanged.
            comp = KhFuseTail(comp, khuRaw.a, true, uv, i.pos.xy, t);
            return float4(comp, khuRaw.a);   // coverage passthrough
        }

        comp = KhFuseTail(comp, 1.0f, false, uv, i.pos.xy, t);   // (scene chain lane)
        return float4(comp, 1.0f);
    }

    // Blend-mode output packing (meshes: hardware blend against the live
    // framebuffer; intensity pre-applied where blend factors cannot express
    // it)
    if (bm == 1 || bm == 3) return float4(outc * color.a, 1.0f);   // additive, screen
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), outc, color.a), 1.0f);   // multiply
    if (bm == 4 || bm == 5) return float4(lerp(scene, outc, color.a), 1.0f);   // lighten, darken (MAX/MIN op)
    return float4(outc, color.a);   // normal (alpha lerp)
}
)HLSL"
