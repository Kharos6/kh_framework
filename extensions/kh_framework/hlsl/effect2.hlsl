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
            // KH_SSGI_JBU: the grid's radiance guided by the GRID's own depth (Kopf et al.'s joint bilateral rule:
            // each low-resolution texel weighed by the guide at its own position). Each tap is a bilinear read of
            // the grid at full pixel q (normalized coords address the grid by its texel centres - the block centre
            // the seed and the gather take, KH_SSGI_CENTRE); its depth weight is that of the least agreeing of the
            // four texels the read mixes (t8's copy: texel t = the depth at KhSsgiFull(t), whose bounce the gather
            // computed), Gathered at the same coords, so a texel across a depth edge never enters the pixel's
            // mean - weighing a tap by the full-res depth at q alone would take a background texel at the
            // foreground's full weight within a texel of a silhouette (a dip of the bounce there, and a leak the
            // other way, each up to 30 %). Where the four depths equal q's (a surface square to the view) this is
            // the 5 x 5 blur at the resolve width bit for bit; on a slope a tap takes its least agreeing texel's
            // weight, its farthest by up to a texel (ground at 4K: within 0.0013 of q's weighting on a smooth
            // field, its noise up to 11 % higher at 50 m). A read on a texel centre (one in b wherever grid / frame
            // reduces to a / b with both odd - a third where the grid is exactly a third of the frame, setSsgiScale
            // 1 / 3 on sides that are multiples of 3) also tests its zero-weight neighbour: near an edge such a tap
            // may drop where it could count. The full-res depth at q is the guide (the old weighting, bit for bit)
            // when unarmed (matCtl.w: no copy this run) and when the grid is the frame's size (setSsgiScale 1: every
            // read is one texel, its depth q's - that weighting is the joint bilateral one there).
            // KH_SSGI_JBU_FB: no tap agreeing (the weight sum at most 1e-4) is one of two cases. Where the pixel's
            // depth lies within the centre read's four texel depths (widened by the tolerance) the surface runs on
            // through the pixel - ground seen at a grazing angle, its depth past the tolerance from texel to texel -
            // and there is no edge to keep: the taps run again under the old weighting (the old image there; the
            // nearest texel alone showed the grid's F x F blocks). Elsewhere the pixel shows what no texel about it
            // holds (a wire, a post thinner than a texel, its every read straddling): the centre read's texel
            // nearest the pixel's depth (dynamicLightFog's composite falls back so over its 3 x 3).
            // Cost against the old resolve, per full-res pixel: the same 25 + 25 fetches (Gathers of t8, a quarter
            // of t7's texels, for the loads of t7), about +150 ALU (four differences and three max a tap for one
            // difference), a size query; where no tap agrees, the old 25 taps once more (grazing ground) or three
            // Gathers (the nearest texel).
            uint khr_gw, khr_gh;
            khsgTex.GetDimensions(khr_gw, khr_gh);
            const bool khr_ga = matCtl.w > 0.5f && matCtl.w < 1.5f &&
                                ((float)khr_gw != fxMeta.z || (float)khr_gh != fxMeta.w);
            bool khr_bw = !khr_ga;   // The old weighting: the full-res depth at q.
            float4 khr_z0 = float4(khr_cd, khr_cd, khr_cd, khr_cd);   // The centre read's four depths.
            [loop] for (int khr_k = 0; khr_k < 2; ++khr_k)
            {
                khr_gi = float3(0.0f, 0.0f, 0.0f);
                khr_ws = 0.0f;
                [unroll] for (int khr_j = -2; khr_j <= 2; ++khr_j)
                [unroll] for (int khr_i = -2; khr_i <= 2; ++khr_i)
                {
                    int2 khr_p = int2(i.pos.xy + float2(khr_i, khr_j) * khr_sp);
                    if (khr_p.x < 0 || khr_p.y < 0 ||
                        khr_p.x >= (int)fxMeta.z || khr_p.y >= (int)fxMeta.w) continue;
                    const float2 khr_uv = (float2(khr_p) + 0.5f) / float2(fxMeta.z, fxMeta.w);
                    float4 khr_d;
                    [branch] if (!khr_bw) {
                        khr_d = khGridZS.Gather(khsgSamp, khr_uv);
                    } else {
                        const float khr_lz = KhLinZ(khr_p);
                        khr_d = float4(khr_lz, khr_lz, khr_lz, khr_lz);
                    }
                    if (khr_i == 0 && khr_j == 0 && !khr_bw) khr_z0 = khr_d;
                    const float4 khr_e = abs(khr_d - khr_cd);   // A sky texel (1e9) weighs exp(-huge) = 0.
                    float khr_dz = max(max(khr_e.x, khr_e.y), max(khr_e.z, khr_e.w)) / (khr_cd * 0.06f + 0.05f);
                    float khr_w = exp(-0.125f * (khr_i * khr_i + khr_j * khr_j))
                                * exp(-khr_dz * khr_dz);
                    khr_gi += khsgTex.SampleLevel(khsgSamp, khr_uv, 0.0f).rgb * khr_w;
                    khr_ws += khr_w;
                }
                if (khr_ws > 1e-4f || khr_bw) break;   // The old weighting's centre tap weighs 1: no third pass.
                const float khr_tl = khr_cd * 0.06f + 0.05f;   // KH_SSGI_JBU_FB.
                const float khr_lo = min(min(khr_z0.x, khr_z0.y), min(khr_z0.z, khr_z0.w)) - khr_tl;
                const float khr_hi = max(max(khr_z0.x, khr_z0.y), max(khr_z0.z, khr_z0.w)) + khr_tl;
                if (!(khr_cd >= khr_lo && khr_cd <= khr_hi)) break;
                khr_bw = true;
            }
            [branch] if (!(khr_ws > 1e-4f)) {
                // Gather's components: w = the read's (lo.x, lo.y), z = (lo.x + 1, lo.y), x = (lo.x, lo.y + 1),
                // y = (lo.x + 1, lo.y + 1) - the same four texels for khr_z0 and the colours, one coordinate.
                const float2 khr_uv0 = (float2(px) + 0.5f) / float2(fxMeta.z, fxMeta.w);
                const float4 khr_cr = khsgTex.GatherRed(khsgSamp, khr_uv0);
                const float4 khr_cg = khsgTex.GatherGreen(khsgSamp, khr_uv0);
                const float4 khr_cb = khsgTex.GatherBlue(khsgSamp, khr_uv0);
                const float4 khr_e0 = abs(khr_z0 - khr_cd);
                float khr_be = khr_e0.w;
                khr_gi = float3(khr_cr.w, khr_cg.w, khr_cb.w);
                if (khr_e0.z < khr_be) { khr_be = khr_e0.z; khr_gi = float3(khr_cr.z, khr_cg.z, khr_cb.z); }
                if (khr_e0.x < khr_be) { khr_be = khr_e0.x; khr_gi = float3(khr_cr.x, khr_cg.x, khr_cb.x); }
                if (khr_e0.y < khr_be) { khr_be = khr_e0.y; khr_gi = float3(khr_cr.y, khr_cg.y, khr_cb.y); }
                khr_ws = 1.0f;
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
            // KH_SSGI_DITHER_CH: gated per channel - one offset gated by the luma would reach a zero channel of a
            // saturated bounce and take it (and scene + it, giOnly's output) below 0. For c >= 0 and a gate s(c)
            // = smoothstep(0, 1.5 / 255, c): c - s(c) / 255 = t (2 t^2 - 3 t + 1.5) / 255 >= 0 (t = 170 c below
            // the knee; the quadratic has no real root), so no channel goes negative; still zero-mean.
            khr_gi += (khr_ig - khr_ig2) * (1.0f / 255.0f)
                    * smoothstep(float3(0.0f, 0.0f, 0.0f), float3(1.5f, 1.5f, 1.5f) / 255.0f, khr_gi);
            // KH_WSEE: SSGI keeps the front depth - the bounce lands on the layers, and on what lies
            // behind them as on the layers where that is geometry (both surfaces lit, assumed alike). Where the sky
            // shows behind them only the layers' share (1 - Tr) of the pixel is a surface: the bounce takes that
            // share (none through a fully transparent layer). An approximation (one Tr for the whole bounce; no
            // receiver of its own behind the layers); per pixel (sample 0), not per sample.
            [branch] if (KhWseeArmed()) {
                float3 khr_wo;   // (front raw, behind raw, G) - the depth behind the layers in .y.
                const float khr_wt = KhWseeAt(px, LoadDepthPS(px), khr_wo);
                if (khr_wo.y > khr_wo.x) {
                    if (LinDepth(khr_wo.y) > 1e8f) khr_gi *= 1.0f - khr_wt;
                }
            }
        }
        outc = fxParams2.y > 0.5f ? khr_gi : scene + khr_gi;
    }

    else if (effect == 26)
    {
        // The radiance seed (KH_SSGI_SEED; internal id: effect_id_from_gv refuses
        // 24 - 30 to a script, and KH_DLF's 32 / 33 lie past KH_MAX_EFFECT,
        // so none is reachable from SQF; the flush synthesizes this ahead of the
        // gather). i.pos spans the scaled grid,
        // so the full-frame uv is its texel's normalized centre on the grid (KH_SSGI_CENTRE: the block's centre,
        // each axis by its own factor, as the resolve reads the grid).
        const float2 khrs_g = KhSsgiGrid();
        const float2 khrs_uv = i.pos.xy / khrs_g;
        // KH_GI_FINITE: held to [0, 65504] (the 16F range; min / max take the non-NaN operand, so a NaN reads 0)
        // - a NaN pixel (a broken user shader's) seeded every gather sample that reached it, and the a-trous
        // passes spread it ~35 px round. A finite non-negative scene passes unchanged.
        float3 khrs_c = min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv, 0.0f).rgb, 0.0f), 65504.0f);
        // KH_SSGI_SEED_BOX: one bilinear tap at the centre averages the middle 2 x 2 full-res pixels - the whole
        // block at a factor of 2 (setSsgiScale 0.5), but 4 of 9 at 3 and 4 of 16 at 4, so a small bright detail
        // left the seed and came back as the camera moved, the bounce flickering with it. Past a factor of 2 on
        // either axis, four bilinear taps a quarter block about the centre cover the block: a box exactly at 4,
        // every pixel at 3 (the middle row and column at two thirds the weight); at 2 and below the single tap is
        // the whole block, as before. Non-default scales only; there the seed draws on 1 / 9 - 1 / 16 of the pixels.
        const float2 khrs_f = float2(fxMeta.z, fxMeta.w) / khrs_g;
        [branch] if (max(khrs_f.x, khrs_f.y) > 2.01f)
        {
            const float2 khrs_q = 0.25f / khrs_g;
            const float2 khrs_r = float2(khrs_q.x, -khrs_q.y);
            khrs_c = 0.25f * (min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv + khrs_q, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv - khrs_q, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv + khrs_r, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv - khrs_r, 0.0f).rgb, 0.0f), 65504.0f));
        }
        return float4(khrs_c, 1.0f);
    }
