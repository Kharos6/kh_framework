// effect2.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.
    else if (effect == 22)   // Ssgi
    {
        // Grain-style time seeds are forbidden in this branch - a static frame
        // renders bit-identical, so all shimmer sources reduce to real
        // scene/camera change;. Normals come from the smaller-delta side per
        // axis (the silhouette-safe reconstruction - a naive derivative cross
        // Paints edge pixels with cross-object normals);. Normalization is by
        // tap count, never by surviving weight: a sparse valid set dims instead
        // of amplifying lone taps (weight-sum division turns one surviving
        // bright tap into a full-strength sparkling pixel).
        float3 khg_b = float3(0.0f, 0.0f, 0.0f);
        // The resolve upsamples the result depth-guided at full res.
        float khg_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        int khg_st = max((int)(khg_inv + 0.5f), 1);
        int2 khg_fpx = int2(float2(px) * khg_inv);
        float khg_cd = LinDepth(LoadDepthPS(khg_fpx));
        float khg_rng = 1.0f;
        if (fxParams1.z > 1.0f)
            khg_rng = 1.0f - smoothstep(fxParams1.z * 0.7f, fxParams1.z, khg_cd);
        if (khg_cd < 1e8f && khg_rng > 0.001f && depthParams.y < -1.0e-3f)
        {
            float2 khg_res = float2(fxMeta.z, fxMeta.w);
            // Reconstruct in view space (KhgVpos: pixel + linear depth through
            // the recovered projection diagonal), which is camera-relative by
            // construction - magnitudes never exceed the far plane, so the
            // cancellation cannot occur.
            float khg_m00 = max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f);
            float khg_m11 = max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f);
            float3 khg_P = KhgVpos(float2(khg_fpx), khg_cd, khg_res, khg_m00, khg_m11);   // Full-res space.
            int2 khg_pl = int2(max(khg_fpx.x - khg_st, 0), khg_fpx.y);
            int2 khg_pr = int2(min(khg_fpx.x + khg_st, (int)fxMeta.z - 1), khg_fpx.y);
            int2 khg_pu = int2(khg_fpx.x, max(khg_fpx.y - khg_st, 0));
            int2 khg_pd = int2(khg_fpx.x, min(khg_fpx.y + khg_st, (int)fxMeta.w - 1));
            float khg_dl = LinDepth(LoadDepthPS(khg_pl));
            float khg_dr = LinDepth(LoadDepthPS(khg_pr));
            float khg_du = LinDepth(LoadDepthPS(khg_pu));
            float khg_dd = LinDepth(LoadDepthPS(khg_pd));
            bool khg_lx = abs(khg_dl - khg_cd) < abs(khg_dr - khg_cd);
            bool khg_ly = abs(khg_du - khg_cd) < abs(khg_dd - khg_cd);
            float3 khg_vx = KhgVpos(float2(khg_lx ? khg_pl : khg_pr),
                                    khg_lx ? khg_dl : khg_dr,
                                    khg_res, khg_m00, khg_m11) - khg_P;
            float3 khg_vy = KhgVpos(float2(khg_ly ? khg_pu : khg_pd),
                                    khg_ly ? khg_du : khg_dd,
                                    khg_res, khg_m00, khg_m11) - khg_P;
            float3 khg_N = cross(khg_vy, khg_vx);
            float khg_nl = length(khg_N);

            if (khg_nl > 1e-9f)
            {
                khg_N /= khg_nl;
                float3 khg_rd = khg_P / max(length(khg_P), 1e-4f);
                if (dot(khg_N, khg_rd) > 0.0f) khg_N = -khg_N;
                // Grazing confidence, per-pixel-ray form. Window lowered
                // (0.02..0.10 -> 0.008..0.05) - the bilateral resolve now eats
                // the reconstruction noise this fade guarded against, so
                // true-grazing distant ground keeps more of its bounce.
                float khg_conf = smoothstep(0.008f, 0.05f, abs(dot(khg_N, khg_rd)));
                float khg_rad = max(fxParams0.y, 0.1f);
                // 2048 is a sanity ceiling, not a design point.
                float khg_spx = clamp(khg_rad * khg_m11 * 0.5f * fxMeta.w / khg_cd, 3.0f, 2048.0f);
                int khg_n = clamp((int)fxParams0.z, 4, 32);
                float khg_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                                     + 0.00583715f * i.pos.y));
                float khg_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                      + 0.00583715f * (i.pos.y + 5.588238f)));
                float khg_rot = khg_ig * 6.2831853f;
                float khg_bias = clamp(fxParams0.w, 0.0f, 0.9f);
                float khg_fp = max(fxParams1.x, 0.25f);
                float khg_acne = fxParams2.z > 0.001f ? fxParams2.z : 1.0f;
                float khg_pfl = (0.01f + khg_cd * 0.0035f) * khg_acne;
                float3 khg_acc = float3(0.0f, 0.0f, 0.0f);
                float khg_ib = 0.0f;

                // Gap conviction (field: screen-anchored black gaps, "low res /
                // pixelated", camera-following): the jittered ladder let
                // stratum-0 radii dip inside the 2 px self-sample floor
                // whenever a pixel's ig2 rolled small, so each pixel randomly
                // lost its dominant nearby taps - per-pixel admission variance,
                // exactly the gap texture.
                float khg_hn = (float)((khg_n + 1) >> 1);
                [loop] for (int khg_k = 0; khg_k < khg_n; ++khg_k)
                {
                    int khg_kp = khg_k >> 1;
                    float khg_an = khg_kp * 2.3999632f + khg_rot + (khg_k & 1) * 3.14159265f;
                    // A golden-ratio offset per stratum decorrelates the bands:
                    // Still position-only (seedless doctrine), still one tap
                    // per annulus; the rings collapse into isotropic variance
                    // the smoothing chain already eats.
                    float khg_igk = frac(khg_ig2 + khg_kp * 0.61803399f);
                    float khg_sr = max(sqrt((khg_kp + khg_igk) / khg_hn) * khg_spx, 2.05f);   // Jitter; floor clamp;
                                                                                              // per-stratum.
                    float2 khg_off = float2(cos(khg_an), sin(khg_an)) * khg_sr;
                    if (dot(khg_off, khg_off) < 4.0f) continue;
                    int2 khg_sp = int2(float2(khg_fpx) + 0.5f + khg_off);   // Full-res space.
                    // Off-screen taps reject (clamping would smear the border
                    // pixels' radiance into the frame edge).
                    if (khg_sp.x < 0 || khg_sp.y < 0 ||
                        khg_sp.x >= (int)fxMeta.z || khg_sp.y >= (int)fxMeta.w) continue;
                    khg_ib += 1.0f;   // Counts information, not admission.
                    float khg_sd = LinDepth(LoadDepthPS(khg_sp));
                    if (khg_sd >= 1e8f) continue;   // Sky carries no bounce.
                    float3 khg_S = KhgVpos(float2(khg_sp), khg_sd, khg_res, khg_m00, khg_m11);
                    float3 khg_v = khg_S - khg_P;
                    float khg_d = length(khg_v);
                    if (khg_d < 1e-4f || khg_d > khg_rad) continue;
                    float khg_ph = dot(khg_N, khg_v);
                    // Smooth fade over [pfl, 2 pfl] instead of a hard cut -
                    // admission cliffs re-roll under 1 px content motion (a
                    // flicker term); the ramp is the same guard with a stable
                    // derivative.
                    float khg_pw = smoothstep(khg_pfl, khg_pfl * 2.0f, khg_ph);
                    if (khg_pw <= 0.0f) continue;
                    float khg_ndl = khg_ph / khg_d;
                    khg_ndl = saturate((khg_ndl - khg_bias) / max(1.0f - khg_bias, 1e-3f));
                    if (khg_ndl <= 0.0f) continue;

                    float khg_occ = 1.0f;

                    [unroll] for (int khg_o = 1; khg_o <= 2; ++khg_o)
                    {
                        float khg_ot = khg_o * 0.333f;
                        float2 khg_op = lerp(float2(khg_fpx), float2(khg_sp), khg_ot);
                        float khg_oz = LinDepth(LoadDepthPS(int2(khg_op)));
                        float khg_ez = khg_cd + khg_ot * (khg_sd - khg_cd);
                        float khg_pen = khg_ez - khg_oz;   // Blocker in front of the segment.
                        float khg_om = 0.05f + khg_ez * 0.006f;
                        khg_occ *= 1.0f - smoothstep(khg_om, khg_om * 3.0f, khg_pen);
                    }

                    if (khg_occ <= 0.001f) continue;
                    // Genuine lateral transport from deeper samples carries a
                    // positive dot and never needed it.
                    float khg_sl = 0.08f + min(khg_sd * 0.0005f, 0.24f);
                    float khg_ce = dot(khg_S, khg_v) / (max(length(khg_S), 1e-4f) * khg_d);   // Proxy (fallback).

                    {
                        int2 khg_nr = int2(min(khg_sp.x + khg_st, (int)fxMeta.z - 1), khg_sp.y);
                        int2 khg_nd = int2(khg_sp.x, min(khg_sp.y + khg_st, (int)fxMeta.w - 1));
                        float khg_dr2 = LinDepth(LoadDepthPS(khg_nr));
                        float khg_dd2 = LinDepth(LoadDepthPS(khg_nd));
                        float khg_xob = 0.15f * khg_sd + 0.5f;   // Cross-object bound.

                        if (khg_dr2 < 1e8f && khg_dd2 < 1e8f &&
                            abs(khg_dr2 - khg_sd) < khg_xob &&
                            abs(khg_dd2 - khg_sd) < khg_xob)
                        {
                            float3 khg_sx = KhgVpos(float2(khg_nr), khg_dr2, khg_res, khg_m00, khg_m11) - khg_S;
                            float3 khg_sy = KhgVpos(float2(khg_nd), khg_dd2, khg_res, khg_m00, khg_m11) - khg_S;
                            float3 khg_sn = cross(khg_sy, khg_sx);
                            float khg_snl = length(khg_sn);

                            if (khg_snl > 1e-9f)
                            {
                                khg_sn /= khg_snl;
                                if (dot(khg_sn, khg_S) > 0.0f) khg_sn = -khg_sn;   // Face the camera (the receiver
                                                                                   // Rule).
                                khg_ce = dot(khg_sn, -khg_v) / khg_d;   // True emission toward the
                                                                        // Receiver.
                            }
                        }
                    }

                    float khg_cs = saturate(khg_ce + khg_sl);
                    float khg_w = khg_ndl * khg_cs * khg_pw * khg_occ * pow(saturate(1.0f - khg_d / khg_rad), khg_fp);
                    float khg_gap = khg_spx / (2.0f * sqrt(max((float)khg_kp, 0.5f) * khg_hn));
                    float khg_ftp = max(khg_sr * 0.125f, khg_gap);
                    float khg_mip = clamp(log2(max(khg_ftp / khg_inv, 1.0f))
                                        + (khg_igk - 0.5f) * 0.5f, 0.0f, 6.0f);
                    float2 khg_uv2 = (float2(khg_sp) + 0.5f) / khg_res;
                    float2 khg_tx = (khg_inv * exp2(khg_mip) * 0.5f) / float2(fxMeta.z, fxMeta.w);
                    float3 khg_c = 0.25f * (khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_tx, khg_mip).rgb
                                 + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_tx, khg_mip).rgb
                                 + khsgTex.SampleLevel(khsgSamp, khg_uv2 + float2( khg_tx.x, -khg_tx.y), khg_mip).rgb
                                 + khsgTex.SampleLevel(khsgSamp, khg_uv2 + float2(-khg_tx.x,  khg_tx.y), khg_mip).rgb);

                    float khg_tg = frac(52.9829189f * frac(0.06711056f * (float)khg_sp.x
                                                         + 0.00583715f * (float)khg_sp.y));
                    float khg_tg2 = frac(52.9829189f * frac(0.06711056f * ((float)khg_sp.x + 5.588238f)
                                                          + 0.00583715f * ((float)khg_sp.y + 5.588238f)));
                    khg_c += (khg_tg - khg_tg2) * (1.0f / 255.0f)
                           * saturate(1.0f - khg_mip * 0.5f);
                    float khg_l = Luma(khg_c);
                    if (fxParams1.w > 0.01f && khg_l > fxParams1.w)
                        khg_c *= fxParams1.w / khg_l;   // Firefly clamp.
                    khg_acc += khg_c * khg_w;
                }

                float3 khg_gi = khg_acc * (2.0f / max(khg_ib, khg_n * 0.5f));
                float khg_gl = Luma(khg_gi);
                khg_gi = max(lerp(float3(khg_gl, khg_gl, khg_gl), khg_gi,
                                  max(fxParams1.y, 0.0f)), 0.0f);
                // Receiver-albedo proxy: bounce lands tinted by the surface it
                // lights (scene chroma over a luma floor) reload at full res -
                // the head's 'scene' sampled the half grid's coordinates (the
                // top- left quadrant), a wrong-texel tint on every receiver.
                float3 khg_scn = SampleScene(khg_fpx);
                float3 khg_alb = khg_scn / (Luma(khg_scn) + 0.3f);
                khg_b = khg_gi * lerp(float3(1.0f, 1.0f, 1.0f), khg_alb, saturate(fxParams2.x))
                      * color.rgb * max(fxParams0.x, 0.0f) * khg_rng * khg_conf;
            }
        }

        return float4(khg_b, 1.0f);
    }
    else if (effect == 24)
    {
        float3 khr_gi = float3(0.0f, 0.0f, 0.0f);
        float khr_cd = LinDepth(LoadDepthPS(px));
        if (khr_cd < 1e8f)
        {
            float khr_f = KhEncFence();   // Continuity belt below.
            float khr_sp = fxParams2.w > 0.5f ? clamp(fxParams2.w, 1.0f, 8.0f) : 3.0f;   // Auto widened (banding report).
            float khr_ws = 0.0f;
            [unroll] for (int khr_j = -2; khr_j <= 2; ++khr_j)
            [unroll] for (int khr_i = -2; khr_i <= 2; ++khr_i)
            {
                int2 khr_p = int2(i.pos.xy + float2(khr_i, khr_j) * khr_sp);
                if (khr_p.x < 0 || khr_p.y < 0 ||
                    khr_p.x >= (int)fxMeta.z || khr_p.y >= (int)fxMeta.w) continue;
                float khr_d = LinDepth(LoadDepthPS(khr_p));
                if (khr_d >= 1e8f) continue;
                float khr_dz = abs(khr_d - khr_cd) / (khr_cd * 0.06f + 0.05f);
                float khr_w = exp(-0.125f * (khr_i * khr_i + khr_j * khr_j))
                            * exp(-khr_dz * khr_dz);
                // Gather is half-res - full-res depth guiding half-res radiance
                // = joint bilateral upsample. Normalized coords are resolution-
                // independent, so the full-res uv addresses the half-res
                // texture exactly.
                khr_gi += khsgTex.SampleLevel(khsgSamp,
                              (float2(khr_p) + 0.5f) / float2(fxMeta.z, fxMeta.w), 0.0f).rgb * khr_w;
                khr_ws += khr_w;
            }
            khr_gi = khr_ws > 1e-4f ? khr_gi / khr_ws : float3(0.0f, 0.0f, 0.0f);
            // With maxDistM live (default 300) it is already zero out here;
            // with maxDistM = 0 the silhouette on/off cliff - resolved bounce
            // vs the sky branch's hard zero - had the verdict teeter to feed
            // on.
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

    else if (effect == 25)
    {
        int khat_sp = (int)clamp(localParams0.x >= 0.5f ? localParams0.x : 2.0f, 1.0f, 4.0f);
        // The scaled-grid -> full-grid factor (local0.y; the gather's twin
        // lane).
        float khat_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        float3 khat_c = khsgTex.Load(int3(px, 0)).rgb;
        int2 khat_fp = int2(float2(px) * khat_inv);
        float khat_cd = LinDepth(LoadDepthPS(khat_fp));

        if (khat_cd < 1e8f)
        {
            float3 khat_acc = khat_c;
            float khat_ws = 1.0f;
            int2 khat_hb = int2((int)(fxMeta.z / khat_inv), (int)(fxMeta.w / khat_inv));

            [unroll] for (int khat_j = -2; khat_j <= 2; ++khat_j)
            [unroll] for (int khat_i = -2; khat_i <= 2; ++khat_i)
            {
                if (khat_i == 0 && khat_j == 0) continue;
                int2 khat_p = px + int2(khat_i, khat_j) * khat_sp;
                if (khat_p.x < 0 || khat_p.y < 0 ||
                    khat_p.x >= khat_hb.x || khat_p.y >= khat_hb.y) continue;
                float khat_d = LinDepth(LoadDepthPS(int2(float2(khat_p) * khat_inv)));
                if (khat_d >= 1e8f) continue;
                float khat_dz = abs(khat_d - khat_cd) / (khat_cd * 0.06f + 0.05f);
                float khat_w = exp(-0.125f * (khat_i * khat_i + khat_j * khat_j))
                             * exp(-khat_dz * khat_dz);
                khat_acc += khsgTex.Load(int3(khat_p, 0)).rgb * khat_w;
                khat_ws += khat_w;
            }

            khat_c = khat_acc / khat_ws;
        }

        return float4(khat_c, 1.0f);   // Side-buffer draw: no tail (the resolve owns it).
    }
    else if (effect == 26)
    {
        // Radiance pyramid seed (internal id, the group's first draw: setPostFX
        // validates <= KH_MAX_EFFECT = 23, so 24/25/26 stay unreachable from
        // SQF; the flush synthesizes this ahead of the gather). i.pos spans the
        // scaled grid, so the full-frame uv rebuilds through the local0.y
        // factor (the gather's twin lane).
        float khrs_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        float2 khrs_uv = i.pos.xy * khrs_inv / float2(fxMeta.z, fxMeta.w);
        return float4(sceneColor.SampleLevel(khsgSamp, khrs_uv, 0.0f).rgb, 1.0f);
    }
    else if (effect == 27)
    {
        float2 khpd_sd = float2(max(localParams0.z, 1.0f), max(localParams0.w, 1.0f));
        float2 khpd_uv = i.pos.xy * 2.0f / khpd_sd;   // Dest px -> source-space uv.
        float2 khpd_tx = 0.75f / khpd_sd;
        float3 khpd_c = 0.25f * (khsgTex.SampleLevel(khsgSamp, khpd_uv + khpd_tx, 0.0f).rgb
                      + khsgTex.SampleLevel(khsgSamp, khpd_uv - khpd_tx, 0.0f).rgb
                      + khsgTex.SampleLevel(khsgSamp, khpd_uv + float2( khpd_tx.x, -khpd_tx.y), 0.0f).rgb
                      + khsgTex.SampleLevel(khsgSamp, khpd_uv + float2(-khpd_tx.x,  khpd_tx.y), 0.0f).rgb);
        return float4(khpd_c, 1.0f);
    }
