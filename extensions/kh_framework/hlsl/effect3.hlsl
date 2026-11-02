// effect3.hlsl - the tail of the effect unit (no #include). Any edit changes the unit's shader cache key.
    else if (effect == 23)   // Fogscatter
    {
        // KH_FS_SPLIT: the scene chain draws PSFsSplitS (the unit's tail) where side 28 is armed - this branch's TWIN,
        // its code line for line (five precise marks and khfs_rm's MSAA_DEPTH 0 form added there, the two side tests
        // resolved armed and the four lanes only KhFsFogW reads dropped): an edit here is made there too.
        float2 khfs_res = float2(fxMeta.z, fxMeta.w);
        const float2 khfs_ms = KhFxProjScale();
        float khfs_m00 = khfs_ms.x;
        float khfs_m11 = khfs_ms.y;
        float khfs_in = max(fxParams0.x, 0.0f);
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * KhFxPx()   // KH_FX_PX_REF.
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        float khfs_cd = KhLinZ(px);   // KH_FX_LINZ.
        // KH_FX_SIDE: every tap needs its source pixel's fog (KhFsFog: a matrix transform and two or three
        // exponentials), which each receiver the source reaches would recompute. The scene chain draws every
        // pixel's once before the pass (side id 28, the same call on the same depth) and arms matCtl.y; the pass
        // reads it. Unarmed (effect meshes, the PIP, a failed side draw) each tap computes it.
        const bool khfs_side = matCtl.y > 0.5f;
        float khfs_f0;
        [branch] if (khfs_side) khfs_f0 = khFxSide.Load(int3(px, 0));
        else                    khfs_f0 = KhFsFogW(px, khfs_cd, khfs_res, khfs_m00, khfs_m11);   // KH_WSEE.
        float khfs_sc = saturate(khfs_f0 * khfs_in);
        // KH_FS_SELF (cb.hlsl's KhFsOwnW): the receiver's own term - its unscattered share and its own pixel's share of
        // its own scatter cone. Do not weight it as a whole tap (3 rm^2 / n / rc^2, about 38x at the defaults): every
        // receiver then keeps an extra 3 / (n sc) of its own sharp value - a light's core through thick fog, a
        // one-pixel step at a silhouette where everything is fogged, and a look that changes with the taps. (A near
        // tap's pyramid footprint, KH_GLOW_PYR, still covers the pixel - about 0.7 of its share again.)
        const float khfs_w0 = KhFsOwnW(khfs_sc, khfs_rm);
        float khfs_ws = khfs_w0;
        float3 khfs_acc = scene * khfs_ws;
        // KH_FS_SKY: the most fog a NEARER source that reaches this receiver puts on its ray (R, below the loop).
        float khfs_fx = khfs_sc;
        float khfs_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                              + 0.00583715f * i.pos.y));
        float khfs_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                               + 0.00583715f * (i.pos.y + 5.588238f)));
        float khfs_rot = khfs_ig * 6.2831853f;
        float khfs_hn = (float)((khfs_n + 1) >> 1);
        // KH_GLOW_PYR: each tap's share of the disc (pi rm^2 / n) is a square rm sqrt(pi / n) on a side; a tap's colour
        // is the picture averaged over it, so the per-pixel rotation does not show as grain. (KH_FS_MIX below leaves
        // the footprint the even disc's: it sets the look - a tap stands for the picture around it - while the mixture
        // only decides where the taps land and what each one weighs.)
        const float khfs_gs = khfs_rm * sqrt(3.14159265f / (float)khfs_n);
        const float khfs_gm = KhGlowMix(khfs_gs);
        // KH_FS_MIX: a source scatters over a disc of rm x its scatter (khfs_rk below). In thin fog that disc is a few
        // pixels; with the taps spread evenly over the full radius rm only the pixels whose first tap happens to land
        // inside it take a share, each at the full disc's weight: sparse bright points around every light. Most sources
        // that reach this receiver scatter about as much as it does or less - a farther one takes the receiver's own by
        // the deflection gate (KH_FS_SKY), and a nearer one, on nearly the same view ray, has crossed less fog - so the
        // radii come from a mixture: a quarter of the distribution over the receiver's own disc ri (rm x its scatter,
        // at least 2 px), three quarters evenly over rm, still one stratum per tap pair. Each tap is weighted by the
        // mixture's density where it lands (the balance heuristic) - pi / (2 hn qi) of the disc's area inside ri,
        // pi / (2 hn qo) outside (2 hn = n for an even count; KH_FS_ODD below), in place of pi rm^2 / n - so it
        // estimates the same sum. Where a nearer source does scatter more (a sky whose fog is below a nearer object's:
        // a fog pass's skyAmount under its ramp, the far fence's feather, the engine's fog-end ramp against its sky;
        // or, in the fallback fog mode, a higher farther surface), the even three quarters still find it, with up to
        // about a half more grain there than the even disc (none where the two fogs are close); the quarter is that
        // trade's balance. At ri = rm the mixture is the even disc.
        const float khfs_ri = min(max(khfs_rm * khfs_sc, 2.0f), khfs_rm);
        const float khfs_qo = 0.75f / (khfs_rm * khfs_rm);   // The density outside ri, x pi.
        const float khfs_qi = 0.25f / (khfs_ri * khfs_ri) + khfs_qo;   // ...and inside it.
        const float khfs_fi = khfs_qi * khfs_ri * khfs_ri;   // The share of the distribution inside ri.
        // KH_FS_ODD: a stratum (a tap pair) holds 1 / hn of the distribution, so a tap of a pair stands for half of it,
        // 1 / (2 hn) - 1 / n for an even count - and an odd count's lone last tap for all of it (doubled below).
        // Weighted 1 / n each, an odd count's full strata would take (n + 1) / n of their share and the lone one
        // (n + 1) / 2n (5 taps: the inner rings 1.2x, the outer 0.6x - a tighter halo). The footprint (khfs_gs)
        // keeps 1 / n: a look.
        const float khfs_nt = 2.0f * khfs_hn;
        const float khfs_cwi = 3.0f / (khfs_nt * khfs_qi);   // The cone's 3 / pi x the tap's share of the area.
        const float khfs_cwo = 3.0f / (khfs_nt * khfs_qo);

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
            // Off-screen taps are absent information: skipping them leaves the closing sum-normalization to
            // renormalize, so edge receivers lean on their surviving weights instead of dimming.
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            float khfs_sd = KhLinZ(khfs_sp);
            float khfs_fs;
            [branch] if (khfs_side) khfs_fs = khFxSide.Load(int3(khfs_sp, 0));   // KH_FX_SIDE.
            else                    khfs_fs = KhFsFogW(khfs_sp, khfs_sd, khfs_res, khfs_m00, khfs_m11);   // KH_WSEE.
            float khfs_ss = saturate(khfs_fs * khfs_in);
            // Deflection gate (KH_FS_SKY - G): a farther source's light reaches this receiver through this receiver's
            // own column, so it scatters with this receiver's fog - also where the farther source reads less (a sky
            // whose fog is below a ramp-fogged ridge in front of it), which min(ss, sc) would keep off the ridge.
            khfs_ss = KhFsSourceFog(khfs_ss, khfs_sc, khfs_sd, khfs_cd);
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // This source's disc does not reach.
            // KH_FS_SKY - R: a nearer source that scatters more - this ray runs beside its column and crossed that fog
            // too. Tapered over the source's disc, as its weight is, so the floor fades out with its reach.
            khfs_fx = KhFsNearFloor(khfs_fx, khfs_ss, khfs_sr, khfs_rk, khfs_sd, khfs_cd);
            float khfs_w = khfs_ss * (khfs_inr ? khfs_cwi : khfs_cwo) * (1.0f - khfs_sr / khfs_rk)
                         / max(khfs_rk * khfs_rk, 1.0f);   // KH_FS_MIX: the tap's own share.
            if ((khfs_k | 1) >= khfs_n) khfs_w *= 2.0f;   // KH_FS_ODD: an odd count's lone last tap.
            float3 khfs_c;
            if (khfs_gm >= 1.0f)      khfs_c = KhGlowTap(khfs_pc, khfs_gs);
            else if (khfs_gm <= 0.0f) khfs_c = SampleScene(khfs_sp);
            else                      khfs_c = lerp(SampleScene(khfs_sp), KhGlowTap(khfs_pc, khfs_gs), khfs_gm);
            khfs_acc += khfs_c * khfs_w;
            khfs_ws += khfs_w;
        }

        // KH_FS_SKY - R: beside a nearer source that scatters more (a sky or past-the-fog-end receiver beside a ridge
        // the ramp has fogged), the receiver's own term at that fog - smaller, so more of the neighbours' scatter shows
        // and the step at the silhouette softens. The own term (KH_FS_SELF) falls with the fog everywhere - 1 - 0.045 f
        // while the cone is under a pixel, 1 - f + 0.955 / (rm^2 f) after - so dw < 0 whenever fx > sc; the min is a
        // guard (never less bleed than without R). fx > sc only beside such a source; elsewhere this is skipped.
        [branch] if (khfs_fx > khfs_sc) {
            const float khfs_dw = min(KhFsOwnW(khfs_fx, khfs_rm) - khfs_w0, 0.0f);
            khfs_acc += scene * khfs_dw;
            khfs_ws += khfs_dw;
        }
        // KH_FS_FLOOR: ws never falls below the own weight at full fog, KhFsOwnW(1, rm) = 0.955 / max(rm, 1)^2 - the
        // least own weight at any fog; the taps only add, and R takes the own term down to KhFsOwnW(fx) at most - so
        // the guard is half of it, which only rounding could reach. Do not use a fixed 1e-4 bound: under KH_FS_SELF,
        // once rm passes 98 px (a radius param past ~49 at 2160 rows), a fully fogged receiver no tap reaches darkens
        // to w0 / 1e-4.
        outc = khfs_acc / max(khfs_ws, 0.5f * KhFsOwnW(1.0f, khfs_rm));
    }
    else if (effect == 28)   // KH_FX_SIDE: fog scatter's per-pixel fog (KhFsFogW), drawn before the pass.
    {
        // KH_WSEE_EDGE_COST: the scene chain draws PSFsSide (the unit's tail) - this branch's TWIN; an edit here is
        // made there too.
        // Fog scatter's own inputs, the same expressions (its lanes are this pass's).
        const float2 khfa_res = float2(fxMeta.z, fxMeta.w);
        const float2 khfa_ms = KhFxProjScale();
        const float khfa_m00 = khfa_ms.x;
        const float khfa_m11 = khfa_ms.y;
        return float4(KhFsFogW(px, KhLinZ(px), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);   // KH_WSEE.
    }
    else if (effect == 31)   // KH_DLF: dynamicLightFog - the grid's fog (t3) upsampled and added to the scene.
    {
        // KH_FX_LEAN: the scene chain draws PSDlfComp (below PSFsSplitS) on an unlocalized, unbanded pass - this
        // branch's TWIN, its code line for line: an edit here is made there too.
        // Joint bilateral upsample (KH_SSAO_SEP's recipe): the four grid texels around the pixel, bilinear weights
        // times depth-key agreement; none agreeing, the nearest in depth of the 3 x 3. Unarmed (dlCtl.x: no light
        // selected, the ring, the grid or the gather / filter entry missing, an unarmed camera, no medium -
        // KH_DLF_MEDIUM0) outc stays the scene: under blend 0 / 4 / 5 the pass leaves the picture as it was; additive,
        // multiply and screen blend it into itself (KhFxFinish), as the C++ chain's skip rule says.
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
                float khdc_best2 = 3.0e38f;   // KH_DLF_FOGREC: the unclamped distance, the tie-break.
                khdc_acc = float3(0.0f, 0.0f, 0.0f);
                khdc_ws = 1.0f;
                [unroll] for (int khdc_n = -1; khdc_n <= 1; ++khdc_n)
                [unroll] for (int khdc_m = -1; khdc_m <= 1; ++khdc_m)
                {
                    const int2  khdc_t = clamp(khdc_c + int2(khdc_m, khdc_n), int2(0, 0), khdc_hi);
                    // KH_DLF_FOGREC: the nearest key clamped to maxDistance (the sky and everything
                    // past maxDistance compare equal, the feather's band too while maxDistance lies below it), ties
                    // broken by the unclamped key - a pixel of far terrain among sky texels takes theirs, not a nearer
                    // surface's, and a sky pixel keeps a sky texel where one is in reach. Without a fog pass drawn
                    // after this one the keys are already clamped (KhDlfKeyZ), the two distances equal, and the choice
                    // is the plain nearest key's, bit for bit.
                    const float khdc_k = KhDlfKeyG(khdc_t, khdc_gd);
                    const float khdc_e = abs(min(khdc_k, KhDlfMaxD()) - min(khdc_cd, KhDlfMaxD()));
                    const float khdc_e2 = abs(khdc_k - khdc_cd);
                    [flatten] if (khdc_e < khdc_best || (khdc_e == khdc_best && khdc_e2 < khdc_best2)) {
                        khdc_best = khdc_e;
                        khdc_best2 = khdc_e2;
                        khdc_acc = khsgTex.Load(int3(khdc_t, 0)).rgb;
                    }
                }
            }
            outc = scene + max(khdc_acc / khdc_ws, 0.0f);
        }
    }
     else if (effect == 101)   // 3D LUT grade (.cube, effect KH_EFFECT_LUT): [strength, encode].
    {
        // Input is clamped to the LUT's [0,1] domain (the loader resamples non-identity domains onto [0,1], so no
        // domain math lives here). GetDimensions gives the LUT's size, so no lane carries it.
        uint khlW, khlH, khlD;
        khLut.GetDimensions(khlW, khlH, khlD);

        if (khlW >= 2)
        {
            // The write window consumes post-tonemap LDR already (direct lookup is correct there), so under encode 0
            // (fxParams0.y; 1 never, 2 always) the sandwich keys on the phase discriminator (scene lanes w <= 1.0).
            // Encode -> LUT -> decode puts the lookup where the artist designed it and hands the tonemap a linear
            // result.
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

            // color.rgb tints after the grade; strength lerps against the untouched scene before the localization/band
            // masks, which multiply in as everywhere else.
            outc = lerp(scene, khlOut * color.rgb, saturate(fxParams0.x));
        }
    }

    // KH_FX_USER: the finish - the localization and band masks, the UI lane's coverage, opacity and blend mode, the
    // fused stages and the packing - is KhFxFinish (cb.hlsl's KH_FX_UNIT section), one body with the user effects'.
    return KhFxFinish(scene, outc, i.pos.xy);
}

// ---- KH_FX_SPLIT - the effect chain's heavy side draws, each its own entry point. --------------------------------
// The SSGI gather (22) and a-trous (25), dynamicLightFog's gather (32) and grid filter (33), not branches of PSEffect:
// one pixel shader's register allocation is that of its heaviest branch, so every effect would pay for these loops.
// Each takes PSEffect's KhFxBegin and px as its prologue (no scene sample, no UI coverage probe: no body reads the
// sample, and every chain pass has centerSize.w = 1). C++ draws them where the chain draws those ids
// (kh_fx_chain_run); no other route draws them (the PIP, the UI lane and effect meshes skip SSGI and
// dynamicLightFog). KH_FS_SPLIT: fog scatter's own entry for the scene chain (PSFsSplitS, below PSFxLinZ), PSEffect's
// branch 23 kept for every other route.

// Effect 22 - the SSGI gather (KH_SSGI_VB).
float4 PSSsgiGather(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // Grain-style time seeds are forbidden in this entry - a static frame renders bit-identical, so all shimmer
        // reduces to real scene/camera change. Normals come from the smaller-delta side per axis (a naive derivative
        // cross paints edge pixels with cross-object normals). Visibility is KH_SSGI_VB's bitmask (effect.hlsl): the
        // result is the slices' sector-weighted sum, normalized by the sector count, never by surviving weight
        // (weight-sum division turns one surviving bright tap into a sparkling pixel).
        float3 khg_b = float3(0.0f, 0.0f, 0.0f);
        // The resolve upsamples the result depth-guided at full res.
        float khg_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        int khg_st = max((int)(khg_inv + 0.5f), 1);
        int2 khg_fpx = KhSsgiFull(px);   // KH_SSGI_CENTRE: the block's centre pixel.
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
            // KH_SSGI_EDGE_N: a neighbour clamped onto the receiver itself (the frame's last column / row at a grid
            // factor of 2 or less - setSsgiScale 0.5 and above; the first below 2 - above 0.5) is absent - its zero
            // depth difference would win the nearer test and its zero vector make the cross 0: no bounce on that whole
            // column / row.
            bool khg_lx = khg_pr.x == khg_fpx.x ||
                          (khg_pl.x != khg_fpx.x && abs(khg_dl - khg_cd) < abs(khg_dr - khg_cd));
            bool khg_ly = khg_pd.y == khg_fpx.y ||
                          (khg_pu.y != khg_fpx.y && abs(khg_du - khg_cd) < abs(khg_dd - khg_cd));
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
                // Grazing confidence, per-pixel-ray form; the bilateral resolve eats the reconstruction noise this fade
                // guards against, so the window is narrow.
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
                // KH_SSGI_VB: the receiver lifted off its own surface by the acne floor, so its reconstruction noise
                // holds no sector; the bias narrows the admitted cone about the normal (acos bias).
                const float3 khg_V = -khg_rd;
                const float3 khg_Pr = khg_P + khg_N * khg_pfl;
                const float  khg_hw = acos(khg_bias);
                // Slices and steps per side from the sample budget (two sides a slice): 12 -> 2 x 3, 16 -> 2 x 4, 32 ->
                // 4 x 4. A budget between those rounds down to 2 x slices x steps (20 takes 16, 5 takes 4).
                const int khg_ns = khg_n <= 8 ? 1 : (khg_n <= 16 ? 2 : 4);
                // KH_UDIV: as uint (both > 0: khg_n 4 - 32, khg_ns 1 - 4) - fxc's X3556; the same quotient.
                const int khg_m = max((int)((uint)khg_n / (uint)(2 * khg_ns)), 1);
                float3 khg_acc = float3(0.0f, 0.0f, 0.0f);
                float khg_ws = 0.0f;

                [loop] for (int khg_si = 0; khg_si < khg_ns; ++khg_si)
                {
                    // The slice: a screen direction rotated per pixel (position only), and the plane through the view
                    // ray it spans - the view-space step of one pixel along it, at a unit depth (KhgVpos; pixels need
                    // not be square).
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
                            // Outward along the line, denser near the receiver, above a 2 px self-sample floor. The
                            // sample stands for its stretch of the line: half the gap to each neighbour (khg_h px
                            // either side).
                            const float khg_u = ((float)khg_k + khg_jk) / (float)khg_m;
                            const float khg_sr = max(khg_u * khg_u * khg_spx, 2.05f);
                            const float khg_h = max(khg_u * khg_spx / (float)khg_m, 0.5f);
                            const int2  khg_sp = int2(float2(khg_fpx) + 0.5f + khg_sdir * khg_sr);   // Full-res.
                            // Off-screen: the rest of this side lies farther out; the side is scaled up by the share it
                            // saw (below). int() truncates toward zero: a sample under one pixel left of (above) the
                            // frame reads the first column (row) at that pixel's own position, as every sample is
                            // rounded onto a pixel - on screen, not off.
                            if (khg_sp.x < 0 || khg_sp.y < 0 ||
                                khg_sp.x >= (int)fxMeta.z || khg_sp.y >= (int)fxMeta.w) {
                                khg_seen = khg_k;
                                break;
                            }
                            const float khg_sd = KhLinZ(khg_sp);
                            if (khg_sd >= 1e8f) continue;   // Sky: no surface, no bounce.
                            const float3 khg_S = KhgVpos(float2(khg_sp), khg_sd, khg_res, khg_m00, khg_m11);
                            // The sender's neighbours one resolve step right and down (its normal, below, and its
                            // plane): on one surface when both lie within the cross-object bound.
                            const int2  khg_nr = int2(min(khg_sp.x + khg_st, (int)fxMeta.z - 1), khg_sp.y);
                            const int2  khg_nd = int2(khg_sp.x, min(khg_sp.y + khg_st, (int)fxMeta.w - 1));
                            const float khg_dr2 = KhLinZ(khg_nr);
                            const float khg_dd2 = KhLinZ(khg_nd);
                            const float khg_xob = 0.15f * khg_sd + 0.5f;   // Cross-object bound.
                            const bool  khg_on = khg_dr2 < 1e8f && khg_dd2 < 1e8f &&
                                                 abs(khg_dr2 - khg_sd) < khg_xob && abs(khg_dd2 - khg_sd) < khg_xob;
                            // The footprint's plane takes a tight bound (a post's neighbour on the wall behind it is
                            // within the loose one) and a real step both ways (at the last row or column the step is
                            // clamped onto the sender itself).
                            const float khg_fob = 0.02f * khg_sd + 0.02f;
                            const bool  khg_fon = khg_nr.x > khg_sp.x && khg_nd.y > khg_sp.y &&
                                                  abs(khg_dr2 - khg_sd) < khg_fob && abs(khg_dd2 - khg_sd) < khg_fob;
                            // The footprint's ends on the sender's plane (1 / depth is affine across a plane in screen
                            // space; per pixel actually stepped); a sender on an edge is taken as a point.
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
                            // Wholly below the receiver's (lifted) tangent plane: never above its horizon, whatever the
                            // rounding of the sample onto the line.
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
                            // Its light: the radiance its face sends the receiver's way, if that face turns toward it.
                            // A Lambertian face's radiance is the same every way, so the facing test only rejects a
                            // back face (the mask's sectors carry the geometry) - smoothly over the slack band.
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
                            // Its radiance from the seed on the gather's grid (effect 26, KH_SSGI_SEED), four taps half
                            // a texel about the sample. Do not read a coarser level (a pyramid's): it blurs a bright
                            // surface across an edge onto the one beside it - a wall's light read at the foot of the
                            // ground below it, or the ground's up the wall - and bleeds it back as bounce; the chain's
                            // a-trous and resolve carry the grain that reading the base level alone leaves. The base
                            // level keeps a smaller share of that bleed: a sample 2 - 3 px from the receiver reads
                            // texels holding the receiver's own block, so up to 23 % of its radiance comes from pixels
                            // at or behind the receiver (on its side of the line through it square to the slice; the
                            // worst slice direction) at setSsgiScale 0.5, 36 % at 0.25. Up to 6 % at 0.5 is the
                            // receiver's own pixel, which an even factor puts at its block's lower right - nearest the
                            // samples up and to the left (3.5 % at most for samples down and to the right). The grid's
                            // resolution: narrower taps do not remove it (a point read keeps 16 % at 0.5).
                            float2 khg_uv2 = (float2(khg_sp) + 0.5f) / khg_res;
                            float2 khg_tx = (khg_inv * 0.5f) / float2(fxMeta.z, fxMeta.w);
                            const float2 khg_ty = float2(khg_tx.x, -khg_tx.y);
                            float3 khg_c = 0.25f * (khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_ty, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_ty, 0.0f).rgb);

                            // KH_SSGI_NODITHER: do not add a +-1/255 dither to each sample's radiance here - a display
                            // step in the linear HDR scene, with no quantizer to serve (the seed, this grid and the
                            // a-trous are 16F; the resolve dithers its own result, gated off the dark). In the dark its
                            // zero-mean sum comes out of the final max(.., 0) as a positive floor and a static pattern;
                            // in the light the a-trous averages it away.
                            float khg_l = Luma(khg_c);
                            if (fxParams1.w > 0.01f && khg_l > fxParams1.w)
                                khg_c *= fxParams1.w / khg_l;   // Firefly clamp.
                            khg_side += khg_c * (khg_face * khg_fall * (float)countbits(khg_new) * (1.0f / 32.0f));
                        }
                        // A side cut short by the frame edge saw only part of its
                        // stretch: scaled by the share it saw, at most 2x.
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
                // Receiver-albedo proxy: bounce lands tinted by the surface it lights (scene chroma over a luma floor),
                // read at the full-res pixel this grid texel stands for (i.pos spans the grid).
                // KH_GI_FINITE: held to [0, 65504] as the seed is - unclamped, a NaN pixel here makes the texel NaN
                // (0 x NaN) and the a-trous passes spread it; a negative one divides by a luma near -0.3. A finite
                // non-negative scene passes unchanged.
                float3 khg_scn = min(max(SampleScene(khg_fpx), 0.0f), 65504.0f);
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
        float3 khat_c = khsgTex.Load(int3(px, 0)).rgb;
        float khat_cd = KhSsgiGridZ(px);   // KH_FX_LINZ.

        if (khat_cd < 1e8f)
        {
            float3 khat_acc = khat_c;
            float khat_ws = 1.0f;
            // KH_SSGI_CENTRE: the grid's own size, not fxMeta.zw over the width's factor (that comes out a column short
            // for some widths and scales, and a row short wherever the sides round apart).
            const int2 khat_hb = int2(KhSsgiGrid());

            [unroll] for (int khat_j = -2; khat_j <= 2; ++khat_j)
            [unroll] for (int khat_i = -2; khat_i <= 2; ++khat_i)
            {
                if (khat_i == 0 && khat_j == 0) continue;
                int2 khat_p = px + int2(khat_i, khat_j) * khat_sp;
                if (khat_p.x < 0 || khat_p.y < 0 ||
                    khat_p.x >= khat_hb.x || khat_p.y >= khat_hb.y) continue;
                float khat_d = KhSsgiGridZ(khat_p);
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

// Effect 32 - dynamicLightFog's gather (KH_DLF). KH_DLF_SPLIT: [earlydepthstencil] - the stencil test before the
// shader runs, so the texels the other build takes cost nothing here though KhFxBegin may discard (no depth or stencil
// is written by either gather draw; with no depth-stencil view bound it changes nothing).
[earlydepthstencil]
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
            // KH_WSEE: where the world's blended layers cover the texel's pixel (KH_DLF_WS_FAINT: where the split
            // stands, the marker sends a texel whose layers pass KH_DLF_WS_FAINT_T or more with the depth buffer at the
            // behind to the lean build; drawn here, such a texel is covered), one march runs to the depth behind them
            // (khdg_sb) and partial sums are kept at each layer group's depth (khdg_gm: the cut in metres) and at the
            // front (khdg_sf: the nearest layer, which may lie before the depth buffer's khdg_sd); the texel takes them
            // composed front to back -
            // (1 - T1) sum(z1) + T1 (1 - T2) sum(z2) + ... + prod T sum(behind); one group: Tr sum(behind) + (1 - Tr)
            // sum(the group's own depth, KhWseeOneFront). Per pixel (sample 0). Elsewhere and unarmed
            // khdg_sb = khdg_sf = khdg_sd, khdg_wc false: the plain march and sum - the KH_FX_LEAN build's, which
            // KH_DLF_SPLIT draws on those texels (PSDlfClass's note): past the verdict the uncovered path's result
            // depends on nothing of the record.
            // KH_DLF_TW: the sums telescoped into ONE - a sample at t weighs W(t) = the behind's weight + the weight of
            // every group (or the front) whose cut lies at or past t, so the march keeps one accumulator in place of
            // eight: the same sum reassociated (float rounding alone).
            float khdg_wt = 1.0f;
            float khdg_sb = khdg_sd;
            float khdg_sf = khdg_sd;
            bool  khdg_wc = false;
            bool  khdg_gs = false;   // The group split.
            float3 khdg_gz = float3(0.0f, 0.0f, 0.0f);   // The groups' view depths (khWsGz), their weights and
            float3 khdg_gw = float3(0.0f, 0.0f, 0.0f);   // the behind's weight (the product of the groups' T).
            float  khdg_gp = 1.0f;
            float4 khdg_gz2 = float4(0.0f, 0.0f, 0.0f, 0.0f);   // Groups 3 - 6's view depths and
            float4 khdg_gw2 = float4(0.0f, 0.0f, 0.0f, 0.0f);   // weights (the layer-depth split; 0 without it);
            bool   khdg_gs2 = false;   // The split's groups stand (khWsSp): groups 3 - 6.
            [branch] if (KhWseeArmed()) {
                float3 khdg_wo;
                khdg_wt = KhWseeAt(khdg_fp, LoadDepthPS(khdg_fp), khdg_wo);
                if (khdg_wo.y > khdg_wo.x) {
                    khdg_wc = true;
                    khdg_sb = LinDepth(khdg_wo.y);
                    khdg_sf = LinDepth(KhWseeOneFront(khdg_wo));
                    [branch] if (khWsGn >= 2u) {
                        khdg_gs = true;
                        khdg_gz = float3(LinDepth(khWsGz.x), LinDepth(khWsGz.y), LinDepth(khWsGz.z));
                        khdg_gw = float3(1.0f - khWsGt.x, khWsGt.x * (1.0f - khWsGt.y),
                                         khWsGt.x * khWsGt.y * (1.0f - khWsGt.z));
                        khdg_gp = khWsGt.x * khWsGt.y * khWsGt.z;
                        // The split's farther groups (absent: T 1, weight 0 - the sums above unchanged).
                        [branch] if (khWsSp) {   // Four (5 and 6 absent, T 1, without a 2nd band).
                            khdg_gs2 = true;
                            khdg_gz2 = float4(LinDepth(khWsGz2.x), LinDepth(khWsGz2.y), LinDepth(khWsGz2.z),
                                              LinDepth(khWsGz2.w));
                            khdg_gw2.xy = float2(khdg_gp * (1.0f - khWsGt2.x),
                                                 khdg_gp * khWsGt2.x * (1.0f - khWsGt2.y));
                            khdg_gp = khdg_gp * khWsGt2.x * khWsGt2.y;
                            khdg_gw2.zw = float2(khdg_gp * (1.0f - khWsGt2.z),
                                                 khdg_gp * khWsGt2.z * (1.0f - khWsGt2.w));
                            khdg_gp = khdg_gp * khWsGt2.z * khWsGt2.w;
                        }
                    }
                }
            }
            const float  khdg_dz = min(khdg_sb, KhDlfMaxD());   // The march ends there, the sky past it.
            const float2 khdg_uv = (float2(khdg_fp) + 0.5f) / float2(fxMeta.z, fxMeta.w);
            const float3 khdg_d1 = KhDlfRel(khdg_uv, 1.0f);   // P(s) = khdg_d1 * s at view depth s.
            const float  khdg_rl = max(length(khdg_d1), 1.0e-6f);   // Metres of ray per unit of s.
            const float3 khdg_rd = khdg_d1 / khdg_rl;
            // The march's end in s: the surface, or maxDistance metres from the camera (a straight line, as
            // kh_dlf_select takes the lights: a light whose reach lies past it lights nothing marched).
            const float  khdg_end = min(khdg_dz, KhDlfMaxD() / khdg_rl);
            // KH_WSEE: the front surface's end of the march in metres along the ray (khdg_end's rule at khdg_sf).
            const float  khdg_fm = min(min(khdg_sf, KhDlfMaxD()), KhDlfMaxD() / khdg_rl) * khdg_rl;
            // The groups' cuts (khdg_fm's rule at each group's depth).
            const float  khdg_gx = KhDlfMaxD();
            const float3 khdg_gm = min(min(khdg_gz, float3(khdg_gx, khdg_gx, khdg_gx)),
                                       float3(khdg_gx, khdg_gx, khdg_gx) / khdg_rl) * khdg_rl;
            const float4 khdg_gm2 = min(min(khdg_gz2, khdg_gx.xxxx),   // Groups 3 - 6.
                                        khdg_gx.xxxx / khdg_rl) * khdg_rl;
            // KH_DLF_TW: W(t)'s terms - the behind's weight khdg_tb and up to seven cuts (khdg_tc0 / tc1, metres)
            // with their weights (khdg_tw0 / tw1): the groups' (gp; gw at gm, gw2 at gm2), or the one-group mix's
            // (Tr; 1 - Tr at the front, khdg_fm: its second sum took the samples not past it), or W = 1 (uncovered:
            // c x 1, the plain sum bit for bit). An unused cut is -1 with weight 0 (no sample lies before 0).
            float  khdg_tb = 1.0f;
            float4 khdg_tc0 = float4(-1.0f, -1.0f, -1.0f, -1.0f), khdg_tw0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
            float4 khdg_tc1 = float4(-1.0f, -1.0f, -1.0f, -1.0f), khdg_tw1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if (khdg_gs) {
                khdg_tb = khdg_gp;
                khdg_tc0 = float4(khdg_gm, -1.0f);
                khdg_tw0 = float4(khdg_gw, 0.0f);
                if (khdg_gs2) {
                    khdg_tc1 = khdg_gm2;
                    khdg_tw1 = khdg_gw2;
                }
            } else if (khdg_wc) {
                khdg_tb = khdg_wt;
                khdg_tc0.x = khdg_fm;
                khdg_tw0.x = 1.0f - khdg_wt;
            }
            // The screen basis the occlusion taps project with (KhDlfSsVis).
            const float3 khdg_f = KhDlfRel(float2(0.5f, 0.5f), 1.0f);
            const float3 khdg_r = KhDlfRel(float2(1.0f, 0.5f), 1.0f) - khdg_f;
            const float3 khdg_u = KhDlfRel(float2(0.5f, 0.0f), 1.0f) - khdg_f;
            const float  khdg_cy = fxCam.y;   // The camera's absolute height: the fog's reference.
            const int    khdg_ns = clamp((int)fxParams1.x, 4, 64);
            const bool   khdg_fr = khFxFog[0].w > 0.001f || khFxFog[1].w > 0.001f;   // KH_DLF_FOGREC: a fog pass.
            // KH_DLF_FOGREC: T at the surface the texel shows - the behind where layers cover it (effect 13's factor
            // for the background; under the layers it blends T at the behind and at the front by their transmittance,
            // so a glow behind glass, a fence or smoke against the sky stays too bright there, by up to 1 / T at the
            // sky) - per record drawn after this pass; 1 for one drawn before or none (KhDlfPassMedium's khdm_tz).
            // The filter and the upsample key the sky apart from the surfaces (KhDlfKeyZ), so a texel's glow blends
            // with those whose factors are close.
            float2 khdg_tz = float2(1.0f, 1.0f);
            [branch] if (khdg_fr) {
                float khdg_dq;
                [unroll] for (int khdg_q = 0; khdg_q < 2; ++khdg_q) {
                    const float4 khdg_e = khdg_q == 0 ? khFxFog[0] : khFxFog[1];
                    const bool   khdg_af = khdg_e.w > 0.001f && (khdg_q == 0 ? fxParams2.x : fxParams2.y) < 0.5f;
                    const float  khdg_tq = saturate(1.0f - saturate(khdg_e.w) * KhDlfPassF(khdg_sb, khdg_e, khdg_dq));
                    khdg_tz[khdg_q] = khdg_af ? khdg_tq : 1.0f;
                }
            }
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
                // KH_DLF_SS: whether the light itself is hidden behind a nearer surface - the gate of
                // KhDlfSsVis's plane continuation - once per light per texel (one probe), not per sample; nothing
                // without the geometry test (shadows < 2).
                bool khdg_cov = false;
                [branch] if (khdg_ss) khdg_cov = KhDlfSsCovered(khdg_r0.xyz, khdg_f, khdg_r, khdg_u);
                float khdg_sum = 0.0f;   // KH_DLF_TW: each sample at its W(t).
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
                    // KH_DLF_FOGREC (effect.hlsl KhDlfPassMedium): the chain's fog passes as a medium too - (1, 0, 0)
                    // without one, the expressions below then the game's fog's alone, bit for bit.
                    float3 khdg_pm = float3(1.0f, 0.0f, 0.0f);
                    [branch] if (khdg_fr) khdg_pm = KhDlfPassMedium(khdg_tw.x / khdg_rl, khdg_rl, khdg_tz);
                    // Nothing to scatter (also behind a pass drawn before that is opaque there - its x 0). TWIN: C++
                    // kh_dlf_medium_zero (KH_DLF_MEDIUM0) draws no gather or filter where this holds at every sample
                    // (the composite runs unarmed).
                    if (!(khdg_sg * khdg_pm.x > 0.0f) && !(khdg_pm.z > 0.0f)) continue;
                    float khdg_v = 1.0f;
                    [branch] if (khdg_slot >= 0) khdg_v = KhDlfDlsVis(khdg_slot, khdg_p + fxCam.xyz);
                    // KH_WSEE: a sample past the front surface (covered texels only) lies behind the layers, which
                    // its own pixel shows at the depth buffer's depth; against that depth KhDlfSsVis would read the
                    // layers as a continuous surface it went behind and shadow it, so its own surface is the behind
                    // one (where a late opaque surface is behind, G = 1, that is khdg_sd itself).
                    const bool khdg_bk = khdg_wc && khdg_tw.x > khdg_fm;
                    [branch] if (khdg_ss && khdg_v > 0.0f) {
                        khdg_v *= KhDlfSsVis(khdg_p, khdg_r0.xyz, khdg_f, khdg_r, khdg_u,
                                             frac(khdg_ig2 + (float)khdg_k * 0.61803399f),
                                             khdg_bk ? khdg_sb : khdg_sd, khdg_cov);   // KH_DLF_SS.
                    }
                    if (khdg_v <= 0.0f) continue;
                    // Extinction on both legs: the camera's (the height fog's closed form, and the engine's fog-end
                    // ramp as its fog applies it) and the light's (the local density over the distance to it).
                    // KH_FOG_EXP_CAP: the density capped (finite), so an optical depth that overflows (a huge fogScale
                    // under a negative decay) gives 1e30 x 0 = 0 and a sample exactly at the light a finite optical
                    // depth, not inf x 0 = NaN - a NaN here would take the light's whole glow out of the texel and,
                    // through the filter, its neighbours'.
                    const float khdg_sgc = min(khdg_sg, 1.0e30f);
                    // KH_DLF_FOGREC: the passes' extinction on the light leg, their transmittance on the camera's
                    // (x, for the game's fog's scattering) and their own scattering (z).
                    const float khdg_t = KhDlfTau(khdg_tw.x, khdg_cy, khdg_py)
                                       + min(khdg_sgc + khdg_pm.y, 1.0e30f) * khdg_dist;
                    const float khdg_c = khdg_tw.y * khdg_at * (khdg_sgc * khdg_pm.x + khdg_pm.z)
                                       * KhDlfPhase(dot(khdg_lv, khdg_rd)) * khdg_v
                                       * exp(-khdg_t) * KhDlfEndRamp(khdg_tw.x);
                    float khdg_ww = khdg_tb;   // KH_DLF_TW: W(t).
                    [branch] if (khdg_wc) {
                        khdg_ww += dot(khdg_tw0, float4(khdg_tw.x <= khdg_tc0.x ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.y ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.z ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.w ? 1.0f : 0.0f))
                                 + dot(khdg_tw1, float4(khdg_tw.x <= khdg_tc1.x ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.y ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.z ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.w ? 1.0f : 0.0f));
                    }
                    khdg_sum += khdg_c * khdg_ww;
                }
                // DynLights' combine without the N.L: the diffuse under the global tint, plus the per-light
                // ambient, times the global intensity. The shadow (khdg_v: our light maps' and the screen-space
                // test's) takes both lanes whole in the sum, where DynLights keeps KH_DLS_AMB_KEEP of the ambient
                // under its map's shadow: a shaft's shadowed side is dark in the fog where a wall in that shadow
                // keeps 35 % of the lamp's ambient.
                const float3 khdg_col = (dlGlobal.xyz * khdg_r2.xyz + khdg_r3.xyz) * dlGlobal.w;
                khdg_acc += khdg_col * khdg_sum;   // KH_DLF_TW: the groups already composed in the weights.
            }
            khdg_acc *= max(fxParams0.x, 0.0f) * color.rgb;
        }
        return float4(khdg_acc, 1.0f);
    }
}

// KH_DLF_SPLIT - which gather a grid texel takes. The same entry built with KH_FX_LEAN 1 (ps_dlf_gather_lean) has no
// record code: where no layer covers the texel's pixel the full gather's march and sum are the lean one's (the weight
// W(t) is 1, the march ends at the depth buffer's depth), so C++ draws the gather twice under a stencil this entry
// marks - the lean build where it stays 0, the full build where it reads 1 - and a texel no layer covers runs without
// the record's registers. PSDlfClass marks 1 wherever KhWseeRule COULD find the pixel covered (KhDlfWsMaybe), faint
// layers alone excepted (KH_DLF_WS_FAINT, below), at any pixel KhDlfFull may give the texel (the coordinate is
// truncated: within KH_DLF_SPLIT_TOL of a pixel edge both sides are tested, so another compile's rounding of the same
// expression cannot pick a pixel left untested - at an even grid factor, setSsgiScale 0.5 on an even frame, the
// coordinate lies ON an edge, so most texels test four pixels: a few loads a texel, against the gather's hundreds), so
// a covered texel takes the full build unless faint layers alone cover it (then the lean one); an uncovered one marked
// 1 takes it too, with the same result. No colour output: the stencil alone (C++ binds no target). KhFxBegin as the
// gathers run it: where it discards, neither gather writes the texel, and this entry marks nothing.
static const float KH_DLF_SPLIT_TOL = 0.01f;   // Pixels: ~40 float ulps at 3840, any compile's rounding inside it.
// KH_DLF_WS_FAINT - a covered pixel whose layers pass KH_DLF_WS_FAINT_T or more of the light behind them, with no
// depth-writing layer among them (the behind raw is the depth buffer's own, o.y == r: the lean march ends where the
// full one does), takes the lean gather: the full build's composition there is T sum(behind) + (1 - T) sum(front),
// the front sum a truncation of the behind sum along the same ray, so the two builds differ by at most (1 - T) of
// their difference - under 2 % of a term that is itself small - for the full gather's whole cost (the record's loads
// and registers, W(t) per sample; the march ends at the same depth). Thin rain, drizzle, a faint haze sprite over the
// frame pay nothing; smoke, foliage and glass (T small, or a depth written) keep the full build. The verdict is
// KhWseeAt's at the pixel, the pair the full gather itself takes (sample 0, PSDlfGather's khdg_wt / khdg_wo), so the
// texel is gated on exactly the inputs it would have used. KhDlfWsMaybe's superset stands for every other pixel (its
// rounding note).
static const float KH_DLF_WS_FAINT_T = KH_WSEE_FAINT_T;   // One constant for every reader (cb.hlsl KH_WSEE_FAINT).
// KhWseeRule's head at p (sample 0, khwr_own, khwr_r0 = r, as KhWseeAt calls it): the range test and the entry test
// (n < r or s > r) on the same loads and the same r (LoadDepthPS) - a superset of its covered set, since KhWseeRule
// covers only inside both. Those compare loaded values alone, except LoadDepthPS's near-z mark, whose r is computed
// (re-encoded) and could round apart in another build: such a pixel counts as maybe covered. Callers hold the arm.
// TWIN: KhWseeRule's NW loads and its two outer tests (cb.hlsl), LoadDepthPS's mark test.
bool KhDlfWsMaybe(int2 p)
{
    bool khwm_c = false;
    const float khwm_r = LoadDepthPS(p);
    bool khwm_z = false;   // LoadDepthPS took the near-z mark's computed value.
    [branch] if (shadowMeta2.x > 0.5f) {
        const float  khwm_raw = LoadDepthRaw(p);
        const float2 khwm_m = khNzMark.Load(int3(p, 0));
        khwm_z = khwm_m.x > 0.0f && khwm_m.x == khwm_raw && khwm_m.y > 0.0f;
    }
    [branch] if (khwm_z) {
        khwm_c = true;
    } else {
        float2 khwm_nn = float2(1.0f, 1.0f);
        [branch] if ((KhWseeAvail() & 128u) == 0u) {
            khwm_nn = float2(khWsN.Load(int3(p, 0)), khWsN2.Load(int3(p, 0)));
        }
        [branch] if ((KhWseeAvail() & 64u) != 0u) {
            const int2 khwm_bp = KhWseePsbAt(p);
            khwm_nn = min(khwm_nn, float2(khWsNb.Load(int3(khwm_bp, 0)), khWsNb2.Load(int3(khwm_bp, 0))));
        }
        const float khwm_n = min(khwm_nn.x, khwm_nn.y);
        [branch] if (min(khwm_r, khwm_n) >= depthParams.z) {
            const float khwm_s = khWsS.Load(int3(p, 0));
            khwm_c = khwm_n < khwm_r || khwm_s > khwm_r;
        }
    }
    // KH_DLF_WS_FAINT: a pixel the rule finds covered by faint layers alone, the depth buffer already at the behind.
    [branch] if (khwm_c) {
        float3 khwm_o;
        const float khwm_t = KhWseeAt(p, khwm_r, khwm_o);
        if (khwm_o.y > khwm_o.x && khwm_t >= KH_DLF_WS_FAINT_T && khwm_o.y == khwm_r) khwm_c = false;
    }
    return khwm_c;
}
void PSDlfClass(VSOut i)
{
    KhFxBegin(i);   // The gathers' prologue.
    const int2 px = int2(i.pos.xy);
    bool khdl_c = false;
    [branch] if (KhWseeArmed()) {
        // KhDlfFull's coordinate before its truncation, and the pixels a rounding of it within the tolerance gives.
        const float2 khdl_f = (float2(px) + 0.5f) * float2(fxMeta.z, fxMeta.w) / max(localParams0.zw, 1.0f);
        const int2   khdl_hi = int2((int)fxMeta.z - 1, (int)fxMeta.w - 1);
        const int2   khdl_a = clamp(int2(khdl_f - KH_DLF_SPLIT_TOL), int2(0, 0), khdl_hi);
        const int2   khdl_b = clamp(int2(khdl_f + KH_DLF_SPLIT_TOL), int2(0, 0), khdl_hi);
        khdl_c = KhDlfWsMaybe(khdl_a);
        [branch] if (!khdl_c && khdl_b.x != khdl_a.x) khdl_c = KhDlfWsMaybe(int2(khdl_b.x, khdl_a.y));
        [branch] if (!khdl_c && khdl_b.y != khdl_a.y) khdl_c = KhDlfWsMaybe(int2(khdl_a.x, khdl_b.y));
        [branch] if (!khdl_c && khdl_b.x != khdl_a.x && khdl_b.y != khdl_a.y) khdl_c = KhDlfWsMaybe(khdl_b);
    }
    if (!khdl_c) discard;
}

// Effect 33 - dynamicLightFog's grid filter (KH_DLF).
float4 PSDlfFilter(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // The SSGI a-trous (effect 25) on the fog: 5 x 5 at the step, Gaussian in the offset and in relative
        // agreement of the depth key (KhDlfKeyG) - the sky and everything past maxDistance, which marched to the same
        // end, blend with one another and with nothing nearer (KH_DLF_FOGREC: with a fog pass drawn after
        // this one the sky keys apart from them - KhDlfKeyZ). Side-buffer draw: no tail.
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
// frame (the conversion itself), 35: the SSGI grid (texel t = KhLinZ at KhSsgiFull(t), the a-trous' mapping), 36:
// dynamicLightFog's (KhLinZ at KhDlfFull(t, local0.zw)). C++ kh_fx_linz_draw draws them with the
// reading passes' own lanes; 35 / 36 read the frame's through KhLinZ.
float PSFxLinZ(VSOut i) : SV_Target
{
    const int2 khlb_p = int2(i.pos.xy);
    const int  khlb_m = (int)(fxMeta.x + 0.5f);
    float khlb_z;
    [branch] if (khlb_m == 35) khlb_z = KhLinZ(KhSsgiFull(khlb_p));   // KH_SSGI_CENTRE.
    else if (khlb_m == 36) khlb_z = KhLinZ(KhDlfFull(khlb_p, localParams0.zw));
    else khlb_z = LinDepth(LoadDepthPS(khlb_p));
    return khlb_z;
}

// KH_FS_SPLIT - fog scatter (effect 23) for the scene chain: PSEffect's branch with its side draw (28) mandatory -
// every pixel's and every tap's fog read from khFxSide (t4), which the chain draws before the pass and arms (matCtl.y)
// - so the in-place KhFsFogW arms (KhWseeMix inlined in the tap loop) are not compiled in and the entry's register
// count stays low. C++ (kh_fx_chain_run) draws it where kh_fx_side_draw armed side 28 and the entry stands; PSEffect's
// branch everywhere else (a failed side draw, an effect mesh, the PIP). The branch's code line for line - its two side
// tests resolved as the armed route resolves them, the four lanes only KhFsFogW read dropped -, PSEffect's prologue
// without the UI lane's probe and KhFxFinish as its tail, so the output is the branch's bit for bit. precise on the
// tap geometry and the two depths (khfs_cd, khfs_an, khfs_sr, khfs_pc, khfs_sd) keeps every operation feeding them
// unfused, as PSEffect's build leaves them through its KhFsFogW arms (PSEffect itself marks none; a GPU fusing mad
// would part from PSEffect by an ulp); khfs_rm at MSAA_DEPTH 0 is written in that build's product form. TWIN:
// PSEffect's branch - an edit there is made here, and both blobs compared again (an edit can move fxc's choices); its
// notes (KH_FX_SIDE, KH_FS_SELF, KH_FS_SKY, KH_GLOW_PYR, KH_FS_MIX, KH_FS_ODD, KH_FS_FLOOR) stand for both.
float4 PSFsSplitS(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue.
    int2  px = int2(i.pos.xy);
    float3 scene = SampleScene(px);
    float3 outc = scene;
    {
        float khfs_in = max(fxParams0.x, 0.0f);
#if MSAA_DEPTH
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * KhFxPx()   // KH_FX_PX_REF.
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
#else   // KH_FS_SPLIT: PSEffect's MSAA_DEPTH 0 build folds both divisions into products (its fxc output).
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * (max(fxMeta.w, 1.0f) * (1.0f / 1080.0f))
                                           : clamp(fxMeta.w * (1.0f / 90.0f), 4.0f, 64.0f);
#endif
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        precise float khfs_cd = KhLinZ(px);   // KH_FX_LINZ.
        float khfs_f0 = khFxSide.Load(int3(px, 0));   // KH_FS_SPLIT: armed (the C++ selection).
        float khfs_sc = saturate(khfs_f0 * khfs_in);
        const float khfs_w0 = KhFsOwnW(khfs_sc, khfs_rm);
        float khfs_ws = khfs_w0;
        float3 khfs_acc = scene * khfs_ws;
        float khfs_fx = khfs_sc;
        float khfs_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                              + 0.00583715f * i.pos.y));
        float khfs_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                               + 0.00583715f * (i.pos.y + 5.588238f)));
        float khfs_rot = khfs_ig * 6.2831853f;
        float khfs_hn = (float)((khfs_n + 1) >> 1);
        const float khfs_gs = khfs_rm * sqrt(3.14159265f / (float)khfs_n);
        const float khfs_gm = KhGlowMix(khfs_gs);
        const float khfs_ri = min(max(khfs_rm * khfs_sc, 2.0f), khfs_rm);
        const float khfs_qo = 0.75f / (khfs_rm * khfs_rm);   // The density outside ri, x pi.
        const float khfs_qi = 0.25f / (khfs_ri * khfs_ri) + khfs_qo;   // ...and inside it.
        const float khfs_fi = khfs_qi * khfs_ri * khfs_ri;   // The share of the distribution inside ri.
        const float khfs_nt = 2.0f * khfs_hn;
        const float khfs_cwi = 3.0f / (khfs_nt * khfs_qi);   // The cone's 3 / pi x the tap's share of the area.
        const float khfs_cwo = 3.0f / (khfs_nt * khfs_qo);

        [loop] for (int khfs_k = 0; khfs_k < khfs_n; ++khfs_k)
        {
            int khfs_kp = khfs_k >> 1;
            precise float khfs_an = khfs_kp * 2.3999632f + khfs_rot + (khfs_k & 1) * 3.14159265f;
            const float khfs_u = (khfs_kp + khfs_ig2) / khfs_hn;
            const bool  khfs_inr = khfs_u < khfs_fi;
            precise float khfs_sr = max(sqrt(khfs_inr ? khfs_u / khfs_qi
                                                      : khfs_ri * khfs_ri + (khfs_u - khfs_fi) / khfs_qo), 1.0f);
            precise float2 khfs_pc = float2(px) + 0.5f + float2(cos(khfs_an), sin(khfs_an)) * khfs_sr;
            int2 khfs_sp = int2(khfs_pc);
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            precise float khfs_sd = KhLinZ(khfs_sp);
            float khfs_fs = khFxSide.Load(int3(khfs_sp, 0));   // KH_FS_SPLIT (KH_FX_SIDE).
            float khfs_ss = saturate(khfs_fs * khfs_in);
            khfs_ss = KhFsSourceFog(khfs_ss, khfs_sc, khfs_sd, khfs_cd);
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // This source's disc does not reach.
            khfs_fx = KhFsNearFloor(khfs_fx, khfs_ss, khfs_sr, khfs_rk, khfs_sd, khfs_cd);
            float khfs_w = khfs_ss * (khfs_inr ? khfs_cwi : khfs_cwo) * (1.0f - khfs_sr / khfs_rk)
                         / max(khfs_rk * khfs_rk, 1.0f);   // KH_FS_MIX: the tap's own share.
            if ((khfs_k | 1) >= khfs_n) khfs_w *= 2.0f;   // KH_FS_ODD: an odd count's lone last tap.
            float3 khfs_c;
            if (khfs_gm >= 1.0f)      khfs_c = KhGlowTap(khfs_pc, khfs_gs);
            else if (khfs_gm <= 0.0f) khfs_c = SampleScene(khfs_sp);
            else                      khfs_c = lerp(SampleScene(khfs_sp), KhGlowTap(khfs_pc, khfs_gs), khfs_gm);
            khfs_acc += khfs_c * khfs_w;
            khfs_ws += khfs_w;
        }

        [branch] if (khfs_fx > khfs_sc) {
            const float khfs_dw = min(KhFsOwnW(khfs_fx, khfs_rm) - khfs_w0, 0.0f);
            khfs_acc += scene * khfs_dw;
            khfs_ws += khfs_dw;
        }
        outc = khfs_acc / max(khfs_ws, 0.5f * KhFsOwnW(1.0f, khfs_rm));
    }
    return KhFxFinish(scene, outc, i.pos.xy);
}

// KH_FX_LEAN - dynamicLightFog's composite (effect 31) for the scene chain: PSEffect's branch 31 line for line, with
// PSEffect's prologue (KhFxBegin, the scene sample; the UI lane's probe left out - a chain pass has centerSize.w 1) and
// KhFxFinish as its tail. C++ (kh_fx_chain_run) draws it, compiled with KH_FX_LEAN (cb.hlsl's note: no record
// maths, ~17 temps where PSEffect holds ~83), where the pass is neither localized nor banded and the build stands;
// PSEffect's branch everywhere else. The same operations on the same inputs (a DXBC emulation of both blobs, mad
// fused and unfused). TWIN: PSEffect's branch 31 - an edit there is made here, and the emulation run again.
float4 PSDlfComp(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue.
    int2  px = int2(i.pos.xy);
    float3 scene = SampleScene(px);
    float3 outc = scene;
    {
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
                float khdc_best2 = 3.0e38f;   // KH_DLF_FOGREC: the unclamped distance, the tie-break.
                khdc_acc = float3(0.0f, 0.0f, 0.0f);
                khdc_ws = 1.0f;
                [unroll] for (int khdc_n = -1; khdc_n <= 1; ++khdc_n)
                [unroll] for (int khdc_m = -1; khdc_m <= 1; ++khdc_m)
                {
                    const int2  khdc_t = clamp(khdc_c + int2(khdc_m, khdc_n), int2(0, 0), khdc_hi);
                    // KH_DLF_FOGREC: the nearest key - PSEffect's branch 31 rule (its note there), line for line.
                    const float khdc_k = KhDlfKeyG(khdc_t, khdc_gd);
                    const float khdc_e = abs(min(khdc_k, KhDlfMaxD()) - min(khdc_cd, KhDlfMaxD()));
                    const float khdc_e2 = abs(khdc_k - khdc_cd);
                    [flatten] if (khdc_e < khdc_best || (khdc_e == khdc_best && khdc_e2 < khdc_best2)) {
                        khdc_best = khdc_e;
                        khdc_best2 = khdc_e2;
                        khdc_acc = khsgTex.Load(int3(khdc_t, 0)).rgb;
                    }
                }
            }
            outc = scene + max(khdc_acc / khdc_ws, 0.0f);
        }
    }
    return KhFxFinish(scene, outc, i.pos.xy);
}

// KH_WSEE_EDGE_COST - fog scatter's side value (side id 28) as its own entry: PSEffect's branch 28 line for line
// (KhFxBegin, then KhFsFogW at the pixel; the prologue's scene load and the UI lane's probe are dead on a side draw).
// Its own entry because PSEffect's register count starves the per-sample loop (KhWseeMix) of warps to hide its loads.
// kh_fx_side_draw draws it for k 0 where made, else PSEffect. TWIN: PSEffect's branch 28 - an edit there is made here.
float4 PSFsSide(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    const int2 px = int2(i.pos.xy);
    const float2 khfa_res = float2(fxMeta.z, fxMeta.w);
    const float2 khfa_ms = KhFxProjScale();
    const float khfa_m00 = khfa_ms.x;
    const float khfa_m11 = khfa_ms.y;
    return float4(KhFsFogW(px, KhLinZ(px), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);   // KH_WSEE.
}

// KH_WSEE: the chain run's class word (khWsE, t58; cb.hlsl KhWseeClassWord) in an R32_UINT target C++ clears to 0; 0
// everywhere without MSAA. Drawn once a run at its first sited pass, before that pass's side draws, with its lanes,
// so every armed pass's fast path (KhWseeCount) reads one texel in place of 2n. A pixel whose word is 0 is discarded,
// so neither the word nor the stencil C++ binds beside it (marked 1 where drawn) is written there.
uint PSWseeAgree(VSOut i) : SV_Target
{
    uint khwg_w = 0u;
#if MSAA_DEPTH
    khwg_w = KhWseeClassWord(int2(i.pos.xy));
#endif
    if (khwg_w == 0u) discard;
    return khwg_w;
}
