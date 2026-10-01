// effect3.hlsl - the tail of the effect unit (no #include). Any edit changes
// the unit's shader cache key.
    else if (effect == 23)   // Fogscatter
    {
        float2 khfs_res = float2(fxMeta.z, fxMeta.w);
        float khfs_m00 = max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f);
        float khfs_m11 = max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f);
        float khfs_in = max(fxParams0.x, 0.0f);
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * KhFxPx()   // KH_FX_PX_REF.
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        float khfs_cd = LinDepth(LoadDepthPS(px));
        // KH_FX_SIDE: every tap needs its source pixel's fog (KhFsFog: a matrix transform and two or three
        // exponentials), which each receiver the source reaches used to recompute. The scene chain draws every
        // pixel's once before the pass (side id 28, the same call on the same depth) and arms matCtl.y; the pass
        // reads it. Unarmed (effect meshes, the PIP) each tap computes it, as before.
        const bool khfs_side = matCtl.y > 0.5f;
        float khfs_f0;
        [branch] if (khfs_side) khfs_f0 = khFxSide.Load(int3(px, 0));
        else                    khfs_f0 = KhFsFog(float2(px), khfs_cd, khfs_res, khfs_m00, khfs_m11);
        float khfs_sc = saturate(khfs_f0 * khfs_in);
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
        // KH_GLOW_PYR: each tap's share of the disc (pi rm^2 / n) is a square rm sqrt(pi / n) on a side; a tap's
        // colour is the picture averaged over it, so the per-pixel rotation no longer shows as grain.
        // (KH_FS_MIX below leaves the footprint the even disc's: it sets the look - a tap stands for the picture
        // around it as it did - while the mixture only decides where the taps land and what each one weighs.)
        const float khfs_gs = khfs_rm * sqrt(3.14159265f / (float)khfs_n);
        const float khfs_gm = KhGlowMix(khfs_gs);
        // KH_FS_MIX: a source scatters over a disc of rm x its scatter (khfs_rk below). In thin fog that disc is a
        // few pixels while the taps spread evenly over the full radius rm, so only the pixels whose first tap
        // happened to land inside it took a share, each at the full disc's weight: sparse bright points around every
        // light. Most sources that reach this receiver scatter about as much as it does or less - a farther one is
        // held to the receiver's own by the deflection gate, and a nearer one, on nearly the same view ray, has
        // crossed less fog - so the radii now come from a mixture: a quarter of the distribution over the
        // receiver's own disc ri (rm x its scatter, at least 2 px), three quarters over rm as before, still one
        // stratum per tap pair. Each tap is weighted by the mixture's density where it lands (the balance
        // heuristic) - pi / (n qi) of the disc's area inside ri, pi / (n qo) outside, in place of pi rm^2 / n - so
        // it estimates the same sum. Where a nearer source does scatter more (a sky whose fog is below a nearer
        // object's: a fog pass's skyAmount under its ramp, the far fence's feather, the engine's fog-end ramp
        // against its sky; or, in the fallback fog mode, a higher farther surface), the even three quarters still
        // find it, with up to about a half more grain than before there (none where the two fogs are close); the
        // quarter is that trade's balance (the harness: in thin fog the speckles' peaks about 0.4 - 0.65 of the
        // former's at 12 taps, a third at 24, and the grain about half - finer, if not less, in the very thinnest).
        // At ri = rm the mixture is the even disc and the taps and weights are the former ones.
        const float khfs_ri = min(max(khfs_rm * khfs_sc, 2.0f), khfs_rm);
        const float khfs_qo = 0.75f / (khfs_rm * khfs_rm);   // The density outside ri, x pi.
        const float khfs_qi = 0.25f / (khfs_ri * khfs_ri) + khfs_qo;   // ...and inside it.
        const float khfs_fi = khfs_qi * khfs_ri * khfs_ri;   // The share of the distribution inside ri.
        const float khfs_cwi = 3.0f / ((float)khfs_n * khfs_qi);   // khfs_c1's per-tap twins (3 / pi x the area).
        const float khfs_cwo = 3.0f / ((float)khfs_n * khfs_qo);

        [loop] for (int khfs_k = 0; khfs_k < khfs_n; ++khfs_k)
        {
            int khfs_kp = khfs_k >> 1;
            float khfs_an = khfs_kp * 2.3999632f + khfs_rot + (khfs_k & 1) * 3.14159265f;
            // KH_FS_MIX: the stratum's point of the mixture's distribution, inverted to a radius (both arms are
            // finite; the select comes before the root).
            const float khfs_u = (khfs_kp + khfs_ig2) / khfs_hn;
            const bool  khfs_inr = khfs_u < khfs_fi;
            float khfs_sr = max(sqrt(khfs_inr ? khfs_u / khfs_qi
                                              : khfs_ri * khfs_ri + (khfs_u - khfs_fi) / khfs_qo), 1.0f);
            const float2 khfs_pc = float2(px) + 0.5f + float2(cos(khfs_an), sin(khfs_an)) * khfs_sr;
            int2 khfs_sp = int2(khfs_pc);
            // Off-screen taps are absent information: skipping them leaves the
            // closing sum-normalization to renormalize, so edge receivers lean
            // on their surviving weights instead of dimming.
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            float khfs_sd = LinDepth(LoadDepthPS(khfs_sp));
            float khfs_fs;
            [branch] if (khfs_side) khfs_fs = khFxSide.Load(int3(khfs_sp, 0));   // KH_FX_SIDE.
            else                    khfs_fs = KhFsFog(float2(khfs_sp), khfs_sd, khfs_res, khfs_m00, khfs_m11);
            float khfs_ss = saturate(khfs_fs * khfs_in);
            if (khfs_sd > khfs_cd) khfs_ss = min(khfs_ss, khfs_sc);   // Deflection gate.
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // This source's disc does not reach.
            float khfs_w = khfs_ss * (khfs_inr ? khfs_cwi : khfs_cwo) * (1.0f - khfs_sr / khfs_rk)
                         / max(khfs_rk * khfs_rk, 1.0f);   // KH_FS_MIX: the tap's own share.
            float3 khfs_c;
            if (khfs_gm >= 1.0f)      khfs_c = KhGlowTap(khfs_pc, khfs_gs);
            else if (khfs_gm <= 0.0f) khfs_c = SampleScene(khfs_sp);
            else                      khfs_c = lerp(SampleScene(khfs_sp), KhGlowTap(khfs_pc, khfs_gs), khfs_gm);
            khfs_acc += khfs_c * khfs_w;
            khfs_ws += khfs_w;
        }

        outc = khfs_acc / max(khfs_ws, 1e-4f);
    }
    else if (effect == 28)   // KH_FX_SIDE: fog scatter's per-pixel fog (KhFsFog), drawn before the pass.
    {
        // Fog scatter's own inputs, the same expressions (its lanes are this pass's).
        const float2 khfa_res = float2(fxMeta.z, fxMeta.w);
        const float khfa_m00 = max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f);
        const float khfa_m11 = max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f);
        return float4(KhFsFog(float2(px), LinDepth(LoadDepthPS(px)), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);
    }
    else if (effect == 32)   // KH_DLF: dynamicLightFog's gather, on the SSGI grid (local0.zw = its size).
    {
        // Every selected light's in-scattering along this texel's view ray (the KH_DLF note, effect.hlsl), times
        // intensity (fxParams0.x) and the tint (color.rgb). Side-buffer draw: no tail (the composite, 31, owns it).
        float3 khdg_acc = float3(0.0f, 0.0f, 0.0f);
        [branch] if (dlCtl.x >= 0.5f && fxCam.w >= 0.5f) {
            const int2   khdg_fp = KhDlfFull(px, localParams0.zw);
            const float  khdg_sd = LinDepth(LoadDepthPS(khdg_fp));   // The visible surface's view depth.
            const float  khdg_dz = min(khdg_sd, KhDlfMaxD());   // KhDlfKey: the march ends there, the sky past it.
            const float2 khdg_uv = (float2(khdg_fp) + 0.5f) / float2(fxMeta.z, fxMeta.w);
            const float3 khdg_d1 = KhDlfRel(khdg_uv, 1.0f);   // P(s) = khdg_d1 * s at view depth s.
            const float  khdg_rl = max(length(khdg_d1), 1.0e-6f);   // Metres of ray per unit of s.
            const float3 khdg_rd = khdg_d1 / khdg_rl;
            // The march's end in s: the surface, or maxDistance metres from the camera (a straight line, as
            // kh_dlf_select takes the lights: a light whose reach lies past it lights nothing marched).
            const float  khdg_end = min(khdg_dz, KhDlfMaxD() / khdg_rl);
            // The screen basis the occlusion taps project with (KhDlfSsVis).
            const float3 khdg_f = KhDlfRel(float2(0.5f, 0.5f), 1.0f);
            const float3 khdg_r = KhDlfRel(float2(1.0f, 0.5f), 1.0f) - khdg_f;
            const float3 khdg_u = KhDlfRel(float2(0.5f, 0.0f), 1.0f) - khdg_f;
            const float  khdg_cy = fxCam.y;   // The camera's absolute height: the fog's reference.
            const int    khdg_ns = clamp((int)fxParams1.x, 4, 64);
            const bool   khdg_ss = fxParams1.z >= 1.5f;
            // Position-only jitter (a static frame renders bit-identical): the SSGI gather's gradient noise.
            const float  khdg_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x + 0.00583715f * i.pos.y));
            const float  khdg_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                           + 0.00583715f * (i.pos.y + 5.588238f)));
            const int    khdg_pn = (int)dlCtl.y;
            const int    khdg_n = khdg_pn + (int)dlCtl.z;
            [loop] for (int khdg_l = 0; khdg_l < khdg_n; ++khdg_l) {
                const int    khdg_b = khdg_l * 6;
                const float4 khdg_r0 = KhDlRec(khdg_b + 0);
                const float4 khdg_r1 = KhDlRec(khdg_b + 1);
                const float4 khdg_r2 = KhDlRec(khdg_b + 2);
                const float4 khdg_r3 = KhDlRec(khdg_b + 3);
                const float4 khdg_r4 = KhDlRec(khdg_b + 4);
                const float4 khdg_r5 = KhDlRec(khdg_b + 5);
                // The ray against the sphere of the light's reach ([5].w), in s.
                const float  khdg_qa = dot(khdg_d1, khdg_d1);
                const float  khdg_qb = -2.0f * dot(khdg_d1, khdg_r0.xyz);
                const float  khdg_qc = dot(khdg_r0.xyz, khdg_r0.xyz) - khdg_r5.w * khdg_r5.w;
                const float  khdg_disc = khdg_qb * khdg_qb - 4.0f * khdg_qa * khdg_qc;
                if (!(khdg_disc > 0.0f)) continue;
                const float  khdg_sq = sqrt(khdg_disc);
                float khdg_s0 = max((-khdg_qb - khdg_sq) / (2.0f * khdg_qa), 0.0f);
                float khdg_s1 = min((-khdg_qb + khdg_sq) / (2.0f * khdg_qa), khdg_end);
                if (!(khdg_s1 > khdg_s0)) continue;
                const bool khdg_spot = khdg_l >= khdg_pn;
                // A spot marches its cone alone (a cut below 0.05 - wider than ~87 degrees - is not one cone). A
                // branch, not &&: fxc does not short-circuit, and the clip writes s0 / s1 - a point's too, were it run.
                bool khdg_off = false;
                [branch] if (khdg_spot && khdg_r1.w > 0.05f && khdg_r2.w > 0.0f) {
                    khdg_off = !KhDlfCone(khdg_d1, khdg_r0.xyz, khdg_r1.xyz, khdg_r1.w, khdg_s0, khdg_s1);
                }
                if (khdg_off) continue;
                // The samples, in metres along the ray (KhDlfMisStep): half equiangular about the ray's point nearest
                // the light, so the glow's core, where the light is brightest, gets them; half evenly spaced. The
                // equiangular width is the miss distance, or the light's core (KhDlfCore) when wider - no wider than
                // the marched span, where the even half serves.
                const float khdg_tc = dot(khdg_r0.xyz, khdg_rd);
                const float khdg_mb = length(khdg_r0.xyz - khdg_rd * khdg_tc);
                const float khdg_w = max(max(khdg_mb, min(KhDlfCore(khdg_r4), (khdg_s1 - khdg_s0) * khdg_rl)), 0.05f);
                const float khdg_t0 = khdg_s0 * khdg_rl;
                const float khdg_t1 = khdg_s1 * khdg_rl;
                const float4 khdg_eq = KhDlfEquiInit(khdg_t0, khdg_t1, khdg_tc, khdg_w);
                const int   khdg_slot = (int)khdg_r5.z - 1;   // Our map's slot, or -1 (none, or shadows 0).
                float khdg_sum = 0.0f;
                [loop] for (int khdg_k = 0; khdg_k < khdg_ns; ++khdg_k) {
                    const float2 khdg_tw = KhDlfMisStep(khdg_eq, khdg_t0, khdg_t1, khdg_k, khdg_ns, khdg_ig);
                    const float3 khdg_p = khdg_rd * khdg_tw.x;
                    float3 khdg_lv = khdg_r0.xyz - khdg_p;
                    const float khdg_dist = length(khdg_lv);
                    khdg_lv /= khdg_dist + 1e-4f;
                    const float khdg_at = KhDlfAtt(khdg_r1, khdg_r2, khdg_r3, khdg_r4, khdg_r5, khdg_dist, khdg_lv,
                                                   khdg_spot);
                    if (khdg_at <= 0.0f) continue;
                    const float khdg_py = khdg_p.y + khdg_cy;
                    const float khdg_sg = KhDlfSigma(khdg_py);
                    if (khdg_sg <= 0.0f) continue;
                    float khdg_v = 1.0f;
                    [branch] if (khdg_slot >= 0) khdg_v = KhDlfDlsVis(khdg_slot, khdg_p + fxCam.xyz);
                    [branch] if (khdg_ss && khdg_v > 0.0f) {
                        khdg_v *= KhDlfSsVis(khdg_p, khdg_r0.xyz, khdg_f, khdg_r, khdg_u,
                                             frac(khdg_ig2 + (float)khdg_k * 0.61803399f), khdg_sd);
                    }
                    if (khdg_v <= 0.0f) continue;
                    // Extinction on both legs: the camera's (the height fog's closed form, and the engine's fog-end
                    // ramp as its fog applies it) and the light's (the local density over the distance to it).
                    const float khdg_t = KhDlfTau(khdg_tw.x, khdg_cy, khdg_py) + khdg_sg * khdg_dist;
                    khdg_sum += khdg_tw.y * khdg_at * khdg_sg * KhDlfPhase(dot(khdg_lv, khdg_rd)) * khdg_v
                              * exp(-khdg_t) * KhDlfEndRamp(khdg_tw.x);
                }
                // DynLights' combine without the N.L: the diffuse under the global tint, plus the per-light
                // ambient, times the global intensity.
                const float3 khdg_col = (dlGlobal.xyz * khdg_r2.xyz + khdg_r3.xyz) * dlGlobal.w;
                khdg_acc += khdg_col * khdg_sum;
            }
            khdg_acc *= max(fxParams0.x, 0.0f) * color.rgb;
        }
        return float4(khdg_acc, 1.0f);
    }
    else if (effect == 33)   // KH_DLF: dynamicLightFog's grid filter (local0.x = the step, zw = the grid).
    {
        // The SSGI a-trous (effect 25) on the fog: 5 x 5 at the step, Gaussian in the offset and in relative
        // agreement of the depth key (KhDlfKey) - the sky and everything past maxDistance, which marched to the same
        // end, blend with one another and with nothing nearer. Side-buffer draw: no tail.
        const float2 khdt_gd = localParams0.zw;
        const int    khdt_st = clamp((int)(localParams0.x + 0.5f), 1, 8);
        const float  khdt_cd = KhDlfKey(KhDlfFull(px, khdt_gd));
        float3 khdt_acc = khsgTex.Load(int3(px, 0)).rgb;
        float  khdt_ws = 1.0f;
        [unroll] for (int khdt_j = -2; khdt_j <= 2; ++khdt_j)
        [unroll] for (int khdt_i = -2; khdt_i <= 2; ++khdt_i)
        {
            if (khdt_i == 0 && khdt_j == 0) continue;
            const int2 khdt_p = px + int2(khdt_i, khdt_j) * khdt_st;
            if (khdt_p.x < 0 || khdt_p.y < 0 || khdt_p.x >= (int)khdt_gd.x || khdt_p.y >= (int)khdt_gd.y) continue;
            const float khdt_z = abs(KhDlfKey(KhDlfFull(khdt_p, khdt_gd)) - khdt_cd) / (khdt_cd * 0.06f + 0.05f);
            const float khdt_w = exp(-0.125f * (float)(khdt_i * khdt_i + khdt_j * khdt_j)) * exp(-khdt_z * khdt_z);
            khdt_acc += khsgTex.Load(int3(khdt_p, 0)).rgb * khdt_w;
            khdt_ws += khdt_w;
        }
        return float4(khdt_acc / khdt_ws, 1.0f);
    }
    else if (effect == 31)   // KH_DLF: dynamicLightFog - the grid's fog (t3) upsampled and added to the scene.
    {
        // Joint bilateral upsample (KH_SSAO_SEP's recipe): the four grid texels around the pixel, bilinear weights
        // times depth-key agreement; none agreeing, the nearest in depth of the 3 x 3. Unarmed (dlCtl.x: no light
        // selected, the ring or the grid missing, an unarmed camera) the pass leaves the picture as it was.
        [branch] if (dlCtl.x >= 0.5f) {
            uint khdc_w, khdc_h;
            khsgTex.GetDimensions(khdc_w, khdc_h);
            const float2 khdc_gd = float2((float)max(khdc_w, 1u), (float)max(khdc_h, 1u));
            const int2   khdc_hi = int2(khdc_gd) - 1;
            const float  khdc_cd = KhDlfKey(px);
            const float2 khdc_g = (float2(px) + 0.5f) * khdc_gd / float2(fxMeta.z, fxMeta.w) - 0.5f;
            const int2   khdc_t0 = int2(floor(khdc_g));
            const float2 khdc_f = khdc_g - floor(khdc_g);
            float3 khdc_acc = float3(0.0f, 0.0f, 0.0f);
            float  khdc_ws = 0.0f;
            [unroll] for (int khdc_j = 0; khdc_j <= 1; ++khdc_j)
            [unroll] for (int khdc_i = 0; khdc_i <= 1; ++khdc_i)
            {
                const int2  khdc_t = clamp(khdc_t0 + int2(khdc_i, khdc_j), int2(0, 0), khdc_hi);
                const float khdc_bw = (khdc_i ? khdc_f.x : 1.0f - khdc_f.x) * (khdc_j ? khdc_f.y : 1.0f - khdc_f.y);
                const float khdc_z = abs(KhDlfKey(KhDlfFull(khdc_t, khdc_gd)) - khdc_cd) / (khdc_cd * 0.06f + 0.05f);
                const float khdc_wt = khdc_bw * exp(-khdc_z * khdc_z);
                khdc_acc += khsgTex.Load(int3(khdc_t, 0)).rgb * khdc_wt;
                khdc_ws += khdc_wt;
            }
            if (!(khdc_ws > 1.0e-4f)) {
                const int2 khdc_c = int2(floor(khdc_g + 0.5f));
                float khdc_best = 3.0e38f;
                khdc_acc = float3(0.0f, 0.0f, 0.0f);
                khdc_ws = 1.0f;
                [unroll] for (int khdc_n = -1; khdc_n <= 1; ++khdc_n)
                [unroll] for (int khdc_m = -1; khdc_m <= 1; ++khdc_m)
                {
                    const int2  khdc_t = clamp(khdc_c + int2(khdc_m, khdc_n), int2(0, 0), khdc_hi);
                    const float khdc_e = abs(KhDlfKey(KhDlfFull(khdc_t, khdc_gd)) - khdc_cd);
                    [flatten] if (khdc_e < khdc_best) {
                        khdc_best = khdc_e;
                        khdc_acc = khsgTex.Load(int3(khdc_t, 0)).rgb;
                    }
                }
            }
            outc = scene + max(khdc_acc / khdc_ws, 0.0f);
        }
    }
     else if (effect == 101)   // 3D LUT grade (.cube, effect KH_EFFECT_LUT): [strength].
    {
        // Input is clamped to the LUT's [0,1] domain (the loader resamples
        // non-identity domains onto [0,1], so no domain math lives here).
        // GetDimensions keeps the branch CB-free.
        uint khlW, khlH, khlD;
        khLut.GetDimensions(khlW, khlH, khlD);

        if (khlW >= 2)
        {
            // The write window consumes post-tonemap LDR already (direct lookup
            // is correct there), so the sandwich keys on the phase
            // discriminator (scene lanes w <= 1.0). Encode -> LUT -> decode
            // puts the lookup where the artist designed it and hands the
            // tonemap a linear result.
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

            // Decode the graded result back to linear when sandwiched (negative
            // lattice values are legal in .cube; clamp before the pow).
            if (khlSand) khlOut = pow(max(khlOut, 0.0f), 2.2f);

            // color.rgb tints after the grade; strength lerps against the
            // untouched scene before the localization/band masks, which
            // multiply in as everywhere else.
            outc = lerp(scene, khlOut * color.rgb, saturate(fxParams0.x));
        }
    }

    // KH_FX_USER: the finish - the localization and band masks, the UI lane's
    // coverage, opacity and blend mode, the fused stages and the packing - is
    // KhFxFinish (cb.hlsl's KH_FX_UNIT section), one body with the user effects'.
    return KhFxFinish(scene, outc, i.pos.xy);
}
