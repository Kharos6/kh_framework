// effect2.hlsl - continues effect.hlsl's effect chain in the effect unit (no
// #include). Any edit changes the unit's shader cache key.
// KH_FX_SPLIT: the SSGI gather (22) and a-trous (25) are their own entry points at the unit's tail (effect3.hlsl:
// PSSsgiGather, PSSsgiAtrous), as are dynamicLightFog's gather and filter (32, 33).
    else if (effect == 24)
    {
        float3 khr_gi = float3(0.0f, 0.0f, 0.0f);
        float khr_cd = KhLinZ(px);   // KH_FX_LINZ.
        if (khr_cd < 1e8f)
        {
            float khr_f = KhEncFence();   // Continuity belt below.
            float khr_sp = fxParams2.w > 0.5f ? clamp(fxParams2.w, 1.0f, 8.0f) : 3.0f;   // Auto width.
            float khr_ws = 0.0f;
            [unroll] for (int khr_j = -2; khr_j <= 2; ++khr_j)
            [unroll] for (int khr_i = -2; khr_i <= 2; ++khr_i)
            {
                int2 khr_p = int2(i.pos.xy + float2(khr_i, khr_j) * khr_sp);
                if (khr_p.x < 0 || khr_p.y < 0 ||
                    khr_p.x >= (int)fxMeta.z || khr_p.y >= (int)fxMeta.w) continue;
                float khr_d = KhLinZ(khr_p);
                if (khr_d >= 1e8f) continue;
                float khr_dz = abs(khr_d - khr_cd) / (khr_cd * 0.06f + 0.05f);
                float khr_w = exp(-0.125f * (khr_i * khr_i + khr_j * khr_j))
                            * exp(-khr_dz * khr_dz);
                // Gather is half-res: full-res depth guiding half-res radiance
                // = joint bilateral upsample. Normalized coords are
                // resolution-independent, so the full-res uv addresses the
                // half-res texture exactly.
                khr_gi += khsgTex.SampleLevel(khsgSamp,
                              (float2(khr_p) + 0.5f) / float2(fxMeta.z, fxMeta.w), 0.0f).rgb * khr_w;
                khr_ws += khr_w;
            }
            khr_gi = khr_ws > 1e-4f ? khr_gi / khr_ws : float3(0.0f, 0.0f, 0.0f);
            // Fade before the fence so the silhouette against the sky branch's
            // hard zero has nothing to teeter on.
            khr_gi *= 1.0f - saturate((khr_cd - khr_f * 0.98f)
                                      / max(khr_f * 0.019f, 1.0f));
            float khr_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                                 + 0.00583715f * i.pos.y));
            float khr_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                  + 0.00583715f * (i.pos.y + 5.588238f)));
            khr_gi += (khr_ig - khr_ig2) * (1.0f / 255.0f)
                    * smoothstep(0.0f, 1.5f / 255.0f, Luma(khr_gi));
        }
        outc = fxParams2.y > 0.5f ? khr_gi : scene + khr_gi;
    }

    else if (effect == 26)
    {
        // The radiance seed (KH_SSGI_SEED; internal id: effect_id_from_gv refuses
        // 24 - 30 to a script, and KH_DLF's 32 / 33 lie past KH_MAX_EFFECT,
        // so none is reachable from SQF; the flush synthesizes this ahead of the
        // gather). i.pos spans the scaled grid,
        // so the full-frame uv rebuilds through the local0.y factor.
        float khrs_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        float2 khrs_uv = i.pos.xy * khrs_inv / float2(fxMeta.z, fxMeta.w);
        // KH_GI_FINITE: held to [0, 65504] (the 16F range; min / max take the non-NaN operand, so a NaN reads 0)
        // - a NaN pixel (a broken user shader's) seeded every gather sample that reached it, and the a-trous
        // passes spread it ~35 px round. A finite non-negative scene passes unchanged.
        return float4(min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv, 0.0f).rgb, 0.0f), 65504.0f), 1.0f);
    }
