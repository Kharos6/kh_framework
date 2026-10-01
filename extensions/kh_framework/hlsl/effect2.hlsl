// effect2.hlsl - continues effect.hlsl's effect chain in the effect unit (no
// #include). Any edit changes the unit's shader cache key.
    else if (effect == 22)   // Ssgi
    {
        // Grain-style time seeds are forbidden in this branch - a static frame
        // renders bit-identical, so all shimmer reduces to real scene/camera
        // change. Normals come from the smaller-delta side per axis (a naive
        // derivative cross paints edge pixels with cross-object normals).
        // Visibility is KH_SSGI_VB's bitmask (effect.hlsl): the result is the
        // slices' sector-weighted sum, normalized by the sector count, never
        // by surviving weight (weight-sum division turns one surviving bright
        // tap into a sparkling pixel).
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
            // Reconstruct in view space (KhgVpos), camera-relative by
            // construction, so no world-scale cancellation occurs.
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
                // Grazing confidence, per-pixel-ray form; the bilateral resolve
                // eats the reconstruction noise this fade guards against, so
                // the window is narrow.
                float khg_conf = smoothstep(0.008f, 0.05f, abs(dot(khg_N, khg_rd)));
                float khg_rad = max(fxParams0.y, 0.1f);
                // 2048 is a sanity ceiling, not a design point.
                float khg_spx = clamp(khg_rad * khg_m11 * 0.5f * fxMeta.w / khg_cd, 3.0f, 2048.0f);
                int khg_n = clamp((int)fxParams0.z, 4, 32);
                float khg_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                                     + 0.00583715f * i.pos.y));
                float khg_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                      + 0.00583715f * (i.pos.y + 5.588238f)));
                float khg_bias = clamp(fxParams0.w, 0.0f, 0.9f);
                float khg_fp = max(fxParams1.x, 0.25f);
                float khg_acne = fxParams2.z > 0.001f ? fxParams2.z : 1.0f;
                float khg_pfl = (0.01f + khg_cd * 0.0035f) * khg_acne;
                // KH_SSGI_VB: the receiver lifted off its own surface by the
                // acne floor, so its reconstruction noise holds no sector; the
                // bias narrows the admitted cone about the normal (acos bias).
                const float3 khg_V = -khg_rd;
                const float3 khg_Pr = khg_P + khg_N * khg_pfl;
                const float  khg_hw = acos(khg_bias);
                // Slices and steps per side from the sample budget (two sides a
                // slice): 12 -> 2 x 3, 16 -> 2 x 4, 32 -> 4 x 4. A budget between
                // those rounds down to 2 x slices x steps (20 takes 16, 5 takes 4).
                const int khg_ns = khg_n <= 8 ? 1 : (khg_n <= 16 ? 2 : 4);
                const int khg_m = max(khg_n / (2 * khg_ns), 1);
                float3 khg_acc = float3(0.0f, 0.0f, 0.0f);
                float khg_ws = 0.0f;

                [loop] for (int khg_si = 0; khg_si < khg_ns; ++khg_si)
                {
                    // The slice: a screen direction rotated per pixel (position
                    // only), and the plane through the view ray it spans - the
                    // view-space step of one pixel along it, at a unit depth
                    // (KhgVpos; pixels need not be square).
                    const float  khg_ph = ((float)khg_si + khg_ig) * (3.14159265f / (float)khg_ns);
                    const float2 khg_dir = float2(cos(khg_ph), sin(khg_ph));   // Screen, y down.
                    const float3 khg_dv = normalize(float3(khg_dir.x / (khg_m00 * khg_res.x),
                                                           -khg_dir.y / (khg_m11 * khg_res.y), 0.0f));
                    const float3 khg_o = normalize(khg_dv - khg_V * dot(khg_dv, khg_V));
                    const float3 khg_ax = cross(khg_o, khg_V);
                    const float3 khg_np = khg_N - khg_ax * dot(khg_N, khg_ax);   // The normal in the slice.
                    const float  khg_npl = length(khg_np);
                    if (khg_npl < 1.0e-4f) continue;
                    const float  khg_na = atan2(dot(khg_np, khg_o), dot(khg_np, khg_V));
                    const float  khg_c0 = KhSsgiCum(-khg_hw, khg_bias);
                    const float  khg_cr = KhSsgiCum(khg_hw, khg_bias) - khg_c0;
                    if (!(khg_cr > 1.0e-6f)) continue;
                    const float  khg_jk = frac(khg_ig2 + (float)khg_si * 0.61803399f);
                    uint   khg_mask = 0u;
                    float3 khg_sacc = float3(0.0f, 0.0f, 0.0f);

                    [loop] for (int khg_sd2 = 0; khg_sd2 < 2; ++khg_sd2)
                    {
                        const float  khg_sg = khg_sd2 == 0 ? 1.0f : -1.0f;
                        const float2 khg_sdir = khg_dir * khg_sg;
                        float3 khg_side = float3(0.0f, 0.0f, 0.0f);
                        int    khg_seen = khg_m;
                        [loop] for (int khg_k = 0; khg_k < khg_m; ++khg_k)
                        {
                            // Outward along the line, denser near the receiver;
                            // the 2 px self-sample floor as before. The sample
                            // stands for its stretch of the line: half the gap
                            // to each neighbour (khg_h px either side).
                            const float khg_u = ((float)khg_k + khg_jk) / (float)khg_m;
                            const float khg_sr = max(khg_u * khg_u * khg_spx, 2.05f);
                            const float khg_h = max(khg_u * khg_spx / (float)khg_m, 0.5f);
                            const int2  khg_sp = int2(float2(khg_fpx) + 0.5f + khg_sdir * khg_sr);   // Full-res.
                            // Off-screen: the rest of this side lies farther out;
                            // the side is scaled up by the share it saw (below).
                            if (khg_sp.x < 0 || khg_sp.y < 0 ||
                                khg_sp.x >= (int)fxMeta.z || khg_sp.y >= (int)fxMeta.w) {
                                khg_seen = khg_k;
                                break;
                            }
                            const float khg_sd = LinDepth(LoadDepthPS(khg_sp));
                            if (khg_sd >= 1e8f) continue;   // Sky: no surface, no bounce.
                            const float3 khg_S = KhgVpos(float2(khg_sp), khg_sd, khg_res, khg_m00, khg_m11);
                            // The sender's neighbours one resolve step right and down
                            // (its normal, below, and its plane): on one surface when
                            // both lie within the cross-object bound.
                            const int2  khg_nr = int2(min(khg_sp.x + khg_st, (int)fxMeta.z - 1), khg_sp.y);
                            const int2  khg_nd = int2(khg_sp.x, min(khg_sp.y + khg_st, (int)fxMeta.w - 1));
                            const float khg_dr2 = LinDepth(LoadDepthPS(khg_nr));
                            const float khg_dd2 = LinDepth(LoadDepthPS(khg_nd));
                            const float khg_xob = 0.15f * khg_sd + 0.5f;   // Cross-object bound.
                            const bool  khg_on = khg_dr2 < 1e8f && khg_dd2 < 1e8f &&
                                                 abs(khg_dr2 - khg_sd) < khg_xob && abs(khg_dd2 - khg_sd) < khg_xob;
                            // The footprint's plane takes a tight bound (a post's
                            // neighbour on the wall behind it is within the loose
                            // one) and a real step both ways (at the last row or
                            // column the step is clamped onto the sender itself).
                            const float khg_fob = 0.02f * khg_sd + 0.02f;
                            const bool  khg_fon = khg_nr.x > khg_sp.x && khg_nd.y > khg_sp.y &&
                                                  abs(khg_dr2 - khg_sd) < khg_fob && abs(khg_dd2 - khg_sd) < khg_fob;
                            // The footprint's ends on the sender's plane (1 / depth is
                            // affine across a plane in screen space; per pixel actually
                            // stepped); a sender on an edge is taken as a point.
                            const float khg_iz = 1.0f / khg_sd;
                            const float khg_izx = (1.0f / khg_dr2 - khg_iz) / max((float)(khg_nr.x - khg_sp.x), 1.0f);
                            const float khg_izy = (1.0f / khg_dd2 - khg_iz) / max((float)(khg_nd.y - khg_sp.y), 1.0f);
                            const float2 khg_ea = -khg_sdir * (khg_fon ? min(khg_h, khg_sr - 1.0f) : 0.0f);
                            const float2 khg_ez = khg_sdir * (khg_fon ? khg_h : 0.0f);
                            const float khg_iza = khg_iz + khg_izx * khg_ea.x + khg_izy * khg_ea.y;
                            const float khg_izz = khg_iz + khg_izx * khg_ez.x + khg_izy * khg_ez.y;
                            const float3 khg_Sa = KhgVpos(float2(khg_sp) + khg_ea, 1.0f / max(khg_iza, 0.5f * khg_iz),
                                                          khg_res, khg_m00, khg_m11);
                            const float3 khg_Sz = KhgVpos(float2(khg_sp) + khg_ez, 1.0f / max(khg_izz, 0.5f * khg_iz),
                                                          khg_res, khg_m00, khg_m11);
                            // Their backs, KH_SSGI_THICK behind along their own rays.
                            const float3 khg_Ba = khg_Sa + khg_Sa * (KH_SSGI_THICK / max(length(khg_Sa), 1e-4f));
                            const float3 khg_Bz = khg_Sz + khg_Sz * (KH_SSGI_THICK / max(length(khg_Sz), 1e-4f));
                            const float3 khg_qa = khg_Sa - khg_Pr, khg_qz = khg_Sz - khg_Pr;
                            const float3 khg_ra = khg_Ba - khg_Pr, khg_rz = khg_Bz - khg_Pr;
                            // Wholly below the receiver's (lifted) tangent plane:
                            // never above its horizon, whatever the rounding of
                            // the sample onto the line.
                            uint khg_bits = 0u;
                            if (max(max(dot(khg_qa, khg_N), dot(khg_qz, khg_N)),
                                    max(dot(khg_ra, khg_N), dot(khg_rz, khg_N))) > 0.0f) {
                                khg_bits = KhSsgiBits(float4(atan2(dot(khg_qa, khg_o), dot(khg_qa, khg_V)),
                                                             atan2(dot(khg_qz, khg_o), dot(khg_qz, khg_V)),
                                                             atan2(dot(khg_ra, khg_o), dot(khg_ra, khg_V)),
                                                             atan2(dot(khg_rz, khg_o), dot(khg_rz, khg_V))) - khg_na,
                                                      khg_bias, khg_hw, khg_c0, khg_cr);
                            }
                            const uint khg_new = khg_bits & ~khg_mask;
                            khg_mask = khg_mask | khg_bits;
                            if (khg_new == 0u) continue;
                            const float3 khg_v = khg_S - khg_P;
                            const float khg_d = length(khg_v);
                            if (khg_d < 1e-4f || khg_d > khg_rad) continue;
                            // Its light: the radiance its face sends the receiver's
                            // way, if that face turns toward it. A Lambertian face's
                            // radiance is the same every way, so the facing test only
                            // rejects a back face (the mask's sectors carry the
                            // geometry) - smoothly over the slack band.
                            float khg_sl = 0.08f + min(khg_sd * 0.0005f, 0.24f);
                            // The proxy (fallback) until the sender's normal is known.
                            float khg_ce = dot(khg_S, khg_v) / (max(length(khg_S), 1e-4f) * khg_d);

                            if (khg_on)
                            {
                                float3 khg_sx = KhgVpos(float2(khg_nr), khg_dr2, khg_res, khg_m00, khg_m11) - khg_S;
                                float3 khg_sy = KhgVpos(float2(khg_nd), khg_dd2, khg_res, khg_m00, khg_m11) - khg_S;
                                float3 khg_sn = cross(khg_sy, khg_sx);
                                float khg_snl = length(khg_sn);

                                if (khg_snl > 1e-9f)
                                {
                                    khg_sn /= khg_snl;
                                    if (dot(khg_sn, khg_S) > 0.0f) khg_sn = -khg_sn;   // Face the camera.
                                    khg_ce = dot(khg_sn, -khg_v) / khg_d;   // Emission toward the receiver.
                                }
                            }

                            const float khg_face = saturate((khg_ce + khg_sl) / (2.0f * khg_sl));
                            const float khg_fall = pow(saturate(1.0f - khg_d / khg_rad), khg_fp);
                            // Its radiance from the seed on the gather's grid (effect 26,
                            // KH_SSGI_SEED), four taps half a texel about the sample: a
                            // coarser level (the retired pyramid's) blurs a bright surface across an edge onto the one
                            // beside it - a wall's light read at the foot of the ground
                            // below it, or the ground's up the wall - and bleeds it back
                            // as bounce (the chain's a-trous and resolve carry the grain
                            // that reading the base level alone leaves).
                            float2 khg_uv2 = (float2(khg_sp) + 0.5f) / khg_res;
                            float2 khg_tx = (khg_inv * 0.5f) / float2(fxMeta.z, fxMeta.w);
                            const float2 khg_ty = float2(khg_tx.x, -khg_tx.y);
                            float3 khg_c = 0.25f * (khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_ty, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_ty, 0.0f).rgb);

                            float khg_tg = frac(52.9829189f * frac(0.06711056f * (float)khg_sp.x
                                                                 + 0.00583715f * (float)khg_sp.y));
                            float khg_tg2 = frac(52.9829189f * frac(0.06711056f * ((float)khg_sp.x + 5.588238f)
                                                                  + 0.00583715f * ((float)khg_sp.y + 5.588238f)));
                            khg_c += (khg_tg - khg_tg2) * (1.0f / 255.0f);
                            float khg_l = Luma(khg_c);
                            if (fxParams1.w > 0.01f && khg_l > fxParams1.w)
                                khg_c *= fxParams1.w / khg_l;   // Firefly clamp.
                            khg_side += khg_c * (khg_face * khg_fall * (float)countbits(khg_new) * (1.0f / 32.0f));
                        }
                        // A side cut short by the frame edge saw only part of its
                        // stretch: scaled by the share it saw, at most 2x (the
                        // former gather's rule for off-screen taps).
                        khg_sacc += khg_side * ((float)khg_m / max((float)khg_seen, 0.5f * (float)khg_m));
                    }
                    // GTAO's slice weight: the normal's projected length times the
                    // lobe's integral; the normalization takes the bias-free lobe (2).
                    khg_acc += khg_sacc * (khg_npl * khg_cr / (1.0f - khg_bias));
                    khg_ws += khg_npl * 2.0f;
                }

                // The budget's factor on the samples the slices take (2 ns m).
                float3 khg_gi = khg_acc * (KH_SSGI_VB_GAIN * pow(12.0f / (float)(2 * khg_ns * khg_m), KH_SSGI_VB_NEXP)
                                           / max(khg_ws, 1e-4f));
                float khg_gl = Luma(khg_gi);
                khg_gi = max(lerp(float3(khg_gl, khg_gl, khg_gl), khg_gi,
                                  max(fxParams1.y, 0.0f)), 0.0f);
                // Receiver-albedo proxy: bounce lands tinted by the surface it
                // lights (scene chroma over a luma floor), reloaded at full res
                // (the head's 'scene' sampled the half grid's coordinates).
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
            float khr_sp = fxParams2.w > 0.5f ? clamp(fxParams2.w, 1.0f, 8.0f) : 3.0f;   // Auto width.
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
        // The radiance seed (KH_SSGI_SEED; internal id: effect_id_from_gv refuses
        // 24 - 30 to a script, and KH_DLF's 32 / 33 lie past KH_MAX_EFFECT,
        // so none is reachable from SQF; the flush synthesizes this ahead of the
        // gather). i.pos spans the scaled grid,
        // so the full-frame uv rebuilds through the local0.y factor.
        float khrs_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        float2 khrs_uv = i.pos.xy * khrs_inv / float2(fxMeta.z, fxMeta.w);
        return float4(sceneColor.SampleLevel(khsgSamp, khrs_uv, 0.0f).rgb, 1.0f);
    }
