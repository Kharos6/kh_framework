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
        float khfs_cd = KhLinZ(px);   // KH_FX_LINZ.
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
        // quarter is that trade's balance (the harness: in thin fog the speckles' peaks 0.39 - 0.63 of the
        // former's at 12 taps, 0.33 - 0.43 at 24, and the grain about half - finer, if not less, in the very thinnest).
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
            float khfs_sd = KhLinZ(khfs_sp);
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
        return float4(KhFsFog(float2(px), KhLinZ(px), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);
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
                const float khdc_z = abs(KhDlfKeyG(khdc_t, khdc_gd) - khdc_cd) / (khdc_cd * 0.06f + 0.05f);
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
                    const float khdc_e = abs(KhDlfKeyG(khdc_t, khdc_gd) - khdc_cd);
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

// ---- KH_FX_SPLIT - the effect chain's heavy side draws, each its own entry point. --------------------------------
// The SSGI gather (22) and a-trous (25), dynamicLightFog's gather (32) and grid filter (33), formerly branches of
// PSEffect: one pixel shader's register allocation is that of its heaviest branch, so every effect paid for these
// loops. Each body is the former branch verbatim (its own notes above it in PSEffect's history: effect2.hlsl /
// effect3.hlsl), its prologue PSEffect's KhFxBegin and px (its scene sample and the UI lane's coverage probe are left
// out: no body reads the sample, and the probe never ran here - every chain pass has centerSize.w = 1); only the
// depth reads (KH_FX_LINZ) and the march's record order (KH_DLF_REC_LAZY) changed. C++ draws them where the chain
// drew those ids (kh_fx_chain_run); no other route
// ever drew them (the PIP, the UI lane and effect meshes skip SSGI and dynamicLightFog).

// Effect 22 - the SSGI gather (KH_SSGI_VB).
float4 PSSsgiGather(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
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
        float khg_cd = KhLinZ(khg_fpx);
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
            float khg_dl = KhLinZ(khg_pl);
            float khg_dr = KhLinZ(khg_pr);
            float khg_du = KhLinZ(khg_pu);
            float khg_dd = KhLinZ(khg_pd);
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
                            const float khg_sd = KhLinZ(khg_sp);
                            if (khg_sd >= 1e8f) continue;   // Sky: no surface, no bounce.
                            const float3 khg_S = KhgVpos(float2(khg_sp), khg_sd, khg_res, khg_m00, khg_m11);
                            // The sender's neighbours one resolve step right and down
                            // (its normal, below, and its plane): on one surface when
                            // both lie within the cross-object bound.
                            const int2  khg_nr = int2(min(khg_sp.x + khg_st, (int)fxMeta.z - 1), khg_sp.y);
                            const int2  khg_nd = int2(khg_sp.x, min(khg_sp.y + khg_st, (int)fxMeta.w - 1));
                            const float khg_dr2 = KhLinZ(khg_nr);
                            const float khg_dd2 = KhLinZ(khg_nd);
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
                // lights (scene chroma over a luma floor), read at the full-res
                // pixel this grid texel stands for (i.pos spans the grid).
                float3 khg_scn = SampleScene(khg_fpx);
                float3 khg_alb = khg_scn / (Luma(khg_scn) + 0.3f);
                khg_b = khg_gi * lerp(float3(1.0f, 1.0f, 1.0f), khg_alb, saturate(fxParams2.x))
                      * color.rgb * max(fxParams0.x, 0.0f) * khg_rng * khg_conf;
            }
        }

        return float4(khg_b, 1.0f);
    }
}

// Effect 25 - the SSGI a-trous (step local0.x).
float4 PSSsgiAtrous(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        int khat_sp = (int)clamp(localParams0.x >= 0.5f ? localParams0.x : 2.0f, 1.0f, 4.0f);
        // The scaled-grid -> full-grid factor (local0.y; the gather's twin
        // lane).
        float khat_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        float3 khat_c = khsgTex.Load(int3(px, 0)).rgb;
        float khat_cd = KhSsgiGridZ(px, khat_inv);   // KH_FX_LINZ.

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
                float khat_d = KhSsgiGridZ(khat_p, khat_inv);
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
}

// Effect 32 - dynamicLightFog's gather (KH_DLF).
float4 PSDlfGather(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // Every selected light's in-scattering along this texel's view ray (the KH_DLF note, effect.hlsl), times
        // intensity (fxParams0.x) and the tint (color.rgb). Side-buffer draw: no tail (the composite, 31, owns it).
        float3 khdg_acc = float3(0.0f, 0.0f, 0.0f);
        [branch] if (dlCtl.x >= 0.5f && fxCam.w >= 0.5f) {
            const int2   khdg_fp = KhDlfFull(px, localParams0.zw);
            const float  khdg_sd = KhLinZ(khdg_fp);   // The visible surface's view depth.
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
                // KH_DLF_REC_LAZY: the light's position ([0]) and reach ([5]) first; [1] - [4] only for a ray
                // that meets its reach (most lights miss most texels).
                const float4 khdg_r0 = KhDlRec(khdg_b + 0);
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
                const float4 khdg_r1 = KhDlRec(khdg_b + 1);
                const float4 khdg_r2 = KhDlRec(khdg_b + 2);
                const float4 khdg_r3 = KhDlRec(khdg_b + 3);
                const float4 khdg_r4 = KhDlRec(khdg_b + 4);
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
}

// Effect 33 - dynamicLightFog's grid filter (KH_DLF).
float4 PSDlfFilter(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // The SSGI a-trous (effect 25) on the fog: 5 x 5 at the step, Gaussian in the offset and in relative
        // agreement of the depth key (KhDlfKey) - the sky and everything past maxDistance, which marched to the same
        // end, blend with one another and with nothing nearer. Side-buffer draw: no tail.
        const float2 khdt_gd = localParams0.zw;
        const int    khdt_st = clamp((int)(localParams0.x + 0.5f), 1, 8);
        const float  khdt_cd = KhDlfKeyG(px, khdt_gd);
        float3 khdt_acc = khsgTex.Load(int3(px, 0)).rgb;
        float  khdt_ws = 1.0f;
        [unroll] for (int khdt_j = -2; khdt_j <= 2; ++khdt_j)
        [unroll] for (int khdt_i = -2; khdt_i <= 2; ++khdt_i)
        {
            if (khdt_i == 0 && khdt_j == 0) continue;
            const int2 khdt_p = px + int2(khdt_i, khdt_j) * khdt_st;
            if (khdt_p.x < 0 || khdt_p.y < 0 || khdt_p.x >= (int)khdt_gd.x || khdt_p.y >= (int)khdt_gd.y) continue;
            const float khdt_z = abs(KhDlfKeyG(khdt_p, khdt_gd) - khdt_cd) / (khdt_cd * 0.06f + 0.05f);
            const float khdt_w = exp(-0.125f * (float)(khdt_i * khdt_i + khdt_j * khdt_j)) * exp(-khdt_z * khdt_z);
            khdt_acc += khsgTex.Load(int3(khdt_p, 0)).rgb * khdt_w;
            khdt_ws += khdt_w;
        }
        return float4(khdt_acc / khdt_ws, 1.0f);
    }
}

// KH_FX_LINZ: the targets KhLinZ / KhSsgiGridZ / KhDlfGridZ read, one texel per pixel of the draw - fxMeta.x 34: the
// frame (the conversion itself), 35: the SSGI grid (texel t = KhLinZ at int2(t x the factor), the a-trous' rule on
// local0.y), 36: dynamicLightFog's (KhLinZ at KhDlfFull(t, local0.zw)). C++ kh_fx_linz_draw draws them with the
// reading passes' own lanes; 35 / 36 read the frame's through KhLinZ.
float PSFxLinZ(VSOut i) : SV_Target
{
    const int2 khlb_p = int2(i.pos.xy);
    const int  khlb_m = (int)(fxMeta.x + 0.5f);
    const float khlb_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;   // The a-trous' factor rule.
    float khlb_z;
    [branch] if (khlb_m == 35) khlb_z = KhLinZ(int2(float2(khlb_p) * khlb_inv));
    else if (khlb_m == 36) khlb_z = KhLinZ(KhDlfFull(khlb_p, localParams0.zw));
    else khlb_z = LinDepth(LoadDepthPS(khlb_p));
    return khlb_z;
}
