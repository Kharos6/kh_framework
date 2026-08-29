// g_hlsl_composite2 - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
struct VSOutC { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    float3 wrel : TEXCOORD4;   // KH_SELF_REL_INTERP (at VSOut)
    float4 icol : TEXCOORD5;   // KH_INSTANCING: the object colour interpolant (at VSOut)
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // world tangent + handedness
#endif
};

// KH_INSTANCING (26762): wrappers over KhVsCore (the shared prefix), the
// per-object one over the CB lanes, the instanced one over the stream. The
// historic transform is still the only one this pass takes - stenVol2.z is
// never written non-zero here - and the core carries the ladder verbatim.
// TWIN: VSMain / VSMainInst in the static unit.
VSOutC VSComposite(VSIn i)
{
    VSOutC o;
    float3 khvR0, khvR1, khvR2;
    KhObjRows(khvR0, khvR1, khvR2);
    KhVsCore(i.pos, i.nrm, centerSize.xyz, centerRel.xyz, centerRel.w, sizeAxes.xyz,
             khvR0, khvR1, khvR2, o.pos, o.wpos, o.wrel, o.nrm);
    o.icol = color;
#if KH_TEXTURED
    o.uv = i.uv;
    // Tangents are COVARIANT (transform like positions, not normals):
    // per-axis scale then the object rotation, renormalized. The handedness
    // sign rides untouched in w.
    o.tanw = float4(normalize(KhRotateR(i.tan.xyz * sizeAxes.xyz, khvR0, khvR1, khvR2)), i.tan.w);
#endif
    return o;
}

VSOutC VSCompositeInst(VSIn i, VSInst n)
{
    VSOutC o;
    KhVsCore(i.pos, i.nrm, n.ipos.xyz, n.irel.xyz, n.irel.w, n.isize.xyz,
             n.irot0.xyz, n.irot1.xyz, n.irot2.xyz, o.pos, o.wpos, o.wrel, o.nrm);
    o.icol = n.icol;
#if KH_TEXTURED
    o.uv = i.uv;
    o.tanw = float4(normalize(KhRotateR(i.tan.xyz * n.isize.xyz, n.irot0.xyz, n.irot1.xyz, n.irot2.xyz)), i.tan.w);
#endif
    return o;
}

)HLSL" R"HLSL(
#if KH_ARB_DEPTH
float4 PSComposite(VSOutC i, out float khaODepth : SV_Depth) : SV_Target
#else
float4 PSComposite(VSOutC i) : SV_Target
#endif
{
    // KH_DLSW_MESHFIRST (26876k, mode 570) - IS OUR FRAGMENT EVEN GETTING THERE?
    //
    // THE FIRST STATEMENT IN THE SHADER, ABOVE EVERY DISCARD, CLIP AND GUARD,
    // AND THAT PLACEMENT IS THE WHOLE POINT. Every probe before this one - 560,
    // 561, 562, 564, 565, 566, 567, 568, 569 - sits AFTER the LOD dither
    // discard, ClipEdgeSliver, ClipOwnNear, the far-contract discards, the
    // owner-map reject and the punch-through guard. Not one of them could
    // report a pixel that never reached the shading code, so the entire
    // elimination so far has been blind to exactly the failure the field is now
    // describing: not a faint tint but the background objects appearing THROUGH
    // the mesh as filled black shapes, parallaxing with those objects.
    //
    // A filled silhouette in the shape of background geometry is what you get
    // when our fragments STOP BEING WRITTEN there - discarded above, or losing
    // the depth test to that geometry - and what shows instead is the world
    // behind, which at night in shadow is black. That is a coverage failure
    // wearing the costume of a shading bug, and no amount of paint applied
    // after the guards can distinguish the two.
    //
    // Solid magenta = our fragment reached the target at every pixel of the
    // mesh, so the black shapes are written by something DOWNSTREAM and the
    // hunt moves to the world pass, the blend or the engine's own post.
    // Black shapes inside the magenta = our fragments are not arriving, and the
    // cause is above this line: one of those discards, or the depth test.
    // Either answer eliminates half of everything left, which is more than the
    // last five builds managed between them.
    if (KhDlsMeshDbg() == 570) {
#if KH_ARB_DEPTH
        // The depth output is mandatory on this variant: leaving it unwritten
        // makes the fragment's depth undefined, which would itself change what
        // survives the test and corrupt the very thing being measured.
        khaODepth = i.pos.z;
#endif
        return float4(1.0f, 0.0f, 1.0f, 1.0f);
    }
    // blendCtl.w carries this fragment's dither threshold and the ZEROED
    // DEFAULT IS OFF, so every fill site that never heard of LODs keeps
    // drawing whole. KH_MESH_LOD CROSSFADE
    if (blendCtl.w != 0.0f) {
        float khlD = frac(52.9829189f * frac(dot(i.pos.xy, float2(0.06711056f, 0.00583715f))));
        if (blendCtl.w > 0.0f) { if (khlD >= blendCtl.w) discard; }
        else if (khlD < -blendCtl.w) discard;
    }
    ClipEdgeSliver(i.wpos, i.nrm);   // degenerate edge-on fragments (fireflies)
    ClipOwnNear(i.pos.w);   // our own near plane. TWIN CALL.
    if (shadowMeta2.x < 0.5f && depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f) > 1.0f) discard;
    if (shadowMeta2.x < 0.5f && shadowMeta2.y > 0.0f && i.pos.w > shadowMeta2.y) discard;
    // Rejecting HERE - before the scene read, the analytic clamp, five shadow
    // tiers and PBR - is the early-Z this SV_Depth (late-Z) variant can never
    // get from the hardware. The injection's own owner map says whether
    // another admitted mesh already holds every sample of this pixel strictly
    // nearer. Lanes zero on every non-injection fill.
    if (shadowMeta2.w > 0.5f && shadowMeta2.z > 0.5f &&
        KhOwnerRejects(i.pos.xy, i.pos.z, shadowMeta2.z, shadowMeta2.w)) {
        // SV_Depth is mandatory on every path of the arb variant, so the
        // raster value is written before leaving.
#if KH_ARB_DEPTH
        khaODepth = i.pos.z;
#endif
        if (dbgCtl.x >= 32.5f && dbgCtl.x < 33.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
        discard;
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    int2 px = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float rawS = GuardSceneRaw(px);
    bool sceneClear = (rawS <= 0.000001f || rawS >= 0.999999f);
    float sceneZ = sceneClear ? 1.0e9f : KhSceneMeters(rawS);
    float fragZ = i.pos.w;
    bool occ = fragZ > sceneZ * (1.0f + fxParams1.y) + fxParams1.x;

    // The fragment's own clearance (its height above the terrain at its own
    // footprint) is therefore tested UNCONDITIONALLY - four loads, every
    // distance, no gate - and the march handles the true behind-the-ridge
    // cases beyond the min-distance gate.
    float khtClear = 2.0e9f;   // 2e9 = lane inactive (distinct from no-data 1e9)

    if (thmParams.w >= 0.5f) {
        float khtHe = KhThmHeight(i.wpos.xz);
        if (khtHe > -1.0e5f) khtClear = i.wpos.y - khtHe;

        if (fragZ >= thmMeta.w) {
            // 1.5 cells (was 0.75): the 96-sample march paces ~1.6 cells at
            // long range, and near-graze rays flipped verdicts as the sample
            // comb slid with camera motion - the moving-camera part-vanish.
            // True ridges carry far more relief than 1.5 cells; only sub-cell
            // bumps lose their (false) vote.
            float khtMc = KhThmClearance(fxParams0.xyz, i.wpos) + 1.5f * thmParams.z;
            if (khtMc < khtClear) khtClear = khtMc;
        }

        if (khtClear < 1.5e9f && khtClear < -thmMeta.z) occ = true;
    }

    // carried out of the clamp's scope for DEBUG VISUAL 10.
    float khaPullDbg = -1.0f;   // < 0 = the clamp did not run at this pixel
    float khaTerrDbg = -1.0f;   // 0 = pull SUPPRESSED by the identity gate
    float khaNoneDbg = 0.0f;   // 1 = clamp ran, nothing to overtake
    // Computed OUTSIDE the KH_ARB_DEPTH block on purpose: it is the crossover
    // model's 'g', and it must be readable on stock frames too, where the
    // clamp never runs. CENSUS: the competing gap in METRES - what the pull
    // would have to spend to overtake whatever is in front of this pixel. 1 =
    // nothing in front (or no scene depth).
    float khaGapDbg = (sceneClear || sceneZ >= fragZ) ? -1.0f : (fragZ - sceneZ);
#if KH_ARB_DEPTH
    // Only fragments the analytic terrain calls VISIBLE, and only inside the
    // contact band (clearance ramp), are pulled forward by the LOD margin -
    // so the rendered terrain LOD (which sits within that margin of the TRUE
    // surface) loses exactly where it used to eat the band, and nothing else
    // in the frame feels the offset.
    {
        float khaD = i.pos.w;
        // It could never have shown a ramped write of ours sitting under a
        // clamped engine write at the same pixel.
        float khaNzN = abs(fxMeta.x);
        float khaNzD = (fxMeta.x < 0.0f) ? (1.0f / max(i.pos.w, 1.0e-8f))
                                         : khaD;
        bool  khaNzArm = (khaNzN > 0.0f && khaNzD < khaNzN);
        bool  khaMod = false;   // did anything actually move the depth?

        if (thmParams.w >= 0.5f && khtClear < 1.0e8f) {
            // Distance-proportional LOD margin (round 7): 0.06 is the
            // far-arbiter relative offset; fxParams1.z remains the absolute
            // cap.
            float khaCh = abs(fxParams1.w);
)HLSL" R"HLSL(   // KH_SNAP_REPROJECT (snapCam.w arms; mode 330 = 0 = the)
            bool   khaSc   = sceneClear;
            float  khaSz   = sceneZ;
            float  khaRefD = khaD;
            float3 khaRayO = fxParams0.xyz;
            float  khaRayW = i.pos.w;
            if (snapCam.w >= 0.5f) {
                float4 khaSC = mul(float4(i.wpos - snapCam.xyz, 1.0f), snapVp);
                if (khaSC.w > 0.01f) {
                    float2 khaSU = khaSC.xy / khaSC.w * float2(0.5f, -0.5f) + 0.5f;
                    if (khaSU.x >= 0.0f && khaSU.x < 1.0f && khaSU.y >= 0.0f && khaSU.y < 1.0f) {
                        int2 khaSPx = clamp(int2(khaSU * fxMeta.zw), int2(0, 0),
                                            int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
                        float khaSR = GuardSceneRaw(khaSPx);
                        khaSc = (khaSR <= 0.000001f || khaSR >= 0.999999f);
                        float khaSN = (khaSR - snapMeta.z) / max(snapMeta.w - snapMeta.z, 1.0e-6f);
                        float khaSDen = khaSN - snapMeta.x;
                        khaSz = (khaSc || khaSDen > -1.0e-7f) ? 1.0e9f : (snapMeta.y / khaSDen);
                        if (!(khaSz > 0.0f)) khaSz = 1.0e9f;
                        khaRefD = khaSC.w;
                        khaRayO = snapCam.xyz;
                        khaRayW = khaSC.w;
                    }
                }
            }
            const float khaTie = (snapCam.w >= 0.5f) ? (0.05f + 1.0e-4f * khaRefD) : 0.0f;
            // The only self-consistent decision is one that does not depend
            // on where our own last write landed: a competitor within the
            // possibly-self band (gap <= 0.25 + tie) is judged at the
            // FRAGMENT, and the pull is the 0.25 m margin itself.
            const bool khaFixed = (snapCam.w >= 0.5f && snapCam.w < 1.5f) || (snapCam.w >= 2.5f && snapCam.w < 3.5f);
            // the fixed pull at a 0.10 m margin (DEFAULT; snapCam.w 1 = mode
            // 333 = the 0.25 m form). A real object base 0.10-0.25 m in front
            // of a ground fragment stops being overtaken; a terrain LOD
            // deficit over 0.10 m is left bitten. Same fixed point, smaller
            // lie.
            const float khaFixM = (snapCam.w >= 2.5f && snapCam.w < 3.5f) ? 0.10f : 0.25f;
            float khaOff = 0.0f;
            bool  khaGated = false;   // gate said 'real occluder'
            bool  khaNone  = true;   // nothing competing at this pixel

            if (!khaSc && khaSz < khaRefD + khaTie) {
                khaNone = false;
                float khaCap = min(fxParams1.z, i.pos.w * 0.06f) *
                    saturate((khaCh - khtClear) / max(0.3f * khaCh, 1.0f));
                bool khaTerr = true;
                float khaTerrW = 1.0f;   // the ramped form of khaTerr

                if (fxParams1.w > 0.0f) {
                    float3 khaDir = (i.wpos - khaRayO) / max(khaRayW, 1.0e-4f);   // the snapshot's ray
                    float khaIdZ = (khaFixed && (khaRefD - khaSz) <= 0.25f + khaTie) ? khaRefD : khaSz;
                    float3 khaSp = khaRayO + khaDir * khaIdZ;
                    float khaSh = KhThmHeight(khaSp.xz);
                    float khaTol = (dbgCtl.w >= 1.5f && dbgCtl.w < 2.5f)
                                 ? max(1.0f, thmParams.z * 0.08f)
                                 : (dbgCtl.w >= 0.5f && dbgCtl.w < 1.5f)
                                 ? max(2.0f, thmParams.z * 0.25f)
                                 : max(0.15f, thmParams.z * 0.02f);
                    // The ramp width is the tested quantity's own screen
                    // derivative, floored at half the tolerance - no new
                    // constant, and the weight reaches 0 exactly at today's
                    // threshold, so 206 can only ever pull LESS than mode 0,
                    // never more, and cannot open a new punch-through. THE
                    // FAIL-OPEN IS THE EXTREME-RANGE RESIDUAL.
                    khaTerr = (khaSh > -1.0e5f)
                            ? ((khaSp.y - khaSh) <= khaTol)
                            : (dbgCtl.w >= 2.5f && dbgCtl.w < 3.5f);
                    // 206 ramps DOWN from the threshold, 207 ramps UP from
                    // it. THE UPWARD RAMP IS THE DEFAULT. dbgCtl.z: 0 default
                    // (up), 1 = 52, 2 = 206 (down - DEFECTIVE, kept as
                    // forensics), 3 = 208 (THE REVERT: the hard gate).
                    float khaTd = khaSp.y - khaSh;
                    bool  khaHard = (dbgCtl.z >= 2.5f && dbgCtl.z < 3.5f);
                    bool  khaDown = (dbgCtl.z >= 1.5f && dbgCtl.z < 2.5f);
                    bool  khaUp   = !khaHard && !khaDown;
                    float khaTrw = khaDown ? max(fwidth(khaTd), khaTol * 0.5f)
                                           : max(fwidth(khaTd), khaTol);
                    float khaTe = khaUp ? (khaTol + khaTrw) : khaTol;
                    khaTerrW = (khaSh > -1.0e5f)
                             ? saturate((khaTe - khaTd) / max(khaTrw, 1.0e-4f))
                             : (khaTerr ? 1.0f : 0.0f);
)HLSL" R"HLSL(                }

                bool khaHardC = (dbgCtl.z >= 2.5f && dbgCtl.z < 3.5f);
                bool khaUpC   = !khaHardC && !(dbgCtl.z >= 1.5f && dbgCtl.z < 2.5f);
                if (khaTerr || (khaUpC && khaTerrW > 0.0f)) {
                    khaOff = (dbgCtl.z >= 0.5f && dbgCtl.z < 1.5f)
                           ? khaCap
                           : (khaFixed ? min(khaCap, khaFixM)   // the margin alone (sized by mode 332)
                                       : min(khaCap, max(khaRefD - khaSz, 0.0f) + 0.25f));   // gap in the snapshot frame
                    if (!khaHardC) khaOff *= khaTerrW;   // default ramps
                    if (khaOff <= 0.0f) khaGated = true;
                } else {
                    khaGated = true;
                }
            }

            khaTerrDbg = khaGated ? 0.0f : 1.0f;
            khaNoneDbg = khaNone ? 1.0f : 0.0f;
            khaPullDbg = khaOff;

            if (khaOff > 0.0f) {
                khaD = max(khaD - khaOff, 0.05f);
                khaMod = true;
            }
        }

        // i.pos.z is the rasterizer's own interpolated, viewport-mapped depth
        // - byte-exact with what the hardware would have written for this
        // fragment had we never declared SV_Depth. EXACT PASS-THROUGH - the
        // depth banding.
        if (!khaMod && khFarSplit.w <= 0.5f &&
            !khaNzArm &&   // the hoisted predicate, not a second copy
            (dbgCtl.y >= 1.5f && dbgCtl.y < 3.5f)) {   // 202/203 revert it
            khaODepth = i.pos.z;
        } else {

        // Even on the path that DOES move the depth, recomputing the absolute
        // NDC re-runs m22 + m32/z, and the m22 term is ~1.0 while the m32
        // term is ~7e-4: the sum discards most of the second operand's
        // mantissa before the offset is even visible.
        float khaNdc;

        if ((dbgCtl.y >= 0.5f && dbgCtl.y < 1.5f) ||
            (dbgCtl.y >= 2.5f && dbgCtl.y < 3.5f)) {   // 51 or 203
            khaD *= (1.0f - 1.0e-4f);   // and earlier
            khaNdc = depthParams.x + depthParams.y / max(khaD, 0.01f);
        } else {
            const float khaNdcE = (i.pos.z - depthParams.z) /
                                  max(depthParams.w - depthParams.z, 1.0e-6f);
            khaNdc = khaNdcE + (depthParams.y / max(khaD, 0.01f) -
                                depthParams.y / max(i.pos.w, 0.01f));
        }
        if (khFarSplit.w > 0.5f) {
            float khaF = khFarSplit.x + khFarSplit.y / max(khaD, 0.01f);
            if (khaF <= 1.0f) {
                khaNdc = khaF;
            } else if (khFarSplit.w >= 1.5f) {
)HLSL" R"HLSL(   // CHUNK BOUNDARY - the KH_FRAME_CLAMP ledger took this
                // KH_FRAME_CLAMP - (mode 314 arms; catalog ledger). khaF > 1,
                // and the solid orange/red regions are khaF <= 1.
                khaNdc = 1.0f;
            } else if (fkVetoMeta.x > 0.5f &&
                       KhFkVetoHit(fxParams0.xyz, i.wpos, khFarSplit.z)) {
                clip(-1.0f);
            }
        }
        khaODepth = clamp(depthParams.z + (depthParams.w - depthParams.z) * khaNdc,
                          depthParams.z, depthParams.w);
)HLSL" R"HLSL(
        // KH_FAR_TIE_BREAK - (mode 316 reverts). Beyond-far fragments do not
        // encode wrongly; they SATURATE. VISUAL 27 ENDED THE SEARCH.
        if (khaNdc >= 1.0f && !(dbgCtl.z >= 3.5f && dbgCtl.z < 4.5f))
            khaODepth = depthParams.w - 1.0e-4f;   // sliver, saturated only
        if (dbgCtl.x >= 27.5f && dbgCtl.x < 28.5f)   // visual 28: the tie-break itself
            return (khaNdc >= 1.0f && !(dbgCtl.z >= 3.5f && dbgCtl.z < 4.5f))
                 ? float4(1.0f, 0.0f, 1.0f, 1.0f) : float4(0.0f, 0.6f, 0.0f, 1.0f);
        // fxMeta.x carries the near estimate (> 0 arms; every other
        // solid-mesh fill leaves it zero - effect meshes never compile this
        // shader), fxMeta.y the widened floor the routed draw's viewport
        // opened. THE RAMP HAS BEEN INVERTED SINCE AND NEVER FIRED WHERE IT
        // WAS NEEDED.
        if (khaNzArm) {
            khaODepth = fxMeta.y + (depthParams.z - fxMeta.y) *
                        saturate(khaNzD / khaNzN);
        }

        }   // end recomputed path
    }
#endif

    // DEBUG VISUAL 9 (setRenderDebug; PRE-discard - the verdict is painted,
    // never enforced, so occluded regions stay inspectable): GREEN = occluded
    // (depth or terrain lane would discard), BLUE = scene clear (depth
    // sentinel: nothing in the snapshot to occlude with), RED = pass but
    // within 4x the depth margins (the marginal band), dim gray = clear pass.
    if (dbgCtl.x >= 8.5f) {
        if (dbgCtl.x < 9.5f) {
            if (occ) return float4(0.0f, 1.0f, 0.0f, 1.0f);
            if (sceneClear) return float4(0.0f, 0.0f, 1.0f, 1.0f);
            // Fall back to a plain metric band when the guard is standing
            // down.
            float khaMx = (fxParams1.x > 1.0e8f)
                        ? (0.5f + sceneZ * 0.02f)
                        : 4.0f * (sceneZ * fxParams1.y + fxParams1.x);
            bool marginal = fragZ > sceneZ - khaMx;
            if (marginal) return float4(1.0f, 0.0f, 0.0f, 1.0f);
            return float4(0.2f, 0.2f, 0.2f, 1.0f);
        }

        if (dbgCtl.x >= 9.5f && dbgCtl.x < 10.5f) {
            if (khaPullDbg < 0.0f) return float4(0.25f, 0.25f, 0.25f, 1.0f);
            if (khaNoneDbg >= 0.5f) return float4(0.0f, 0.15f, 0.7f, 1.0f);
            if (khaTerrDbg < 0.5f) return float4(0.0f, 0.9f, 1.0f, 1.0f);
            float khaR = max(saturate(khaPullDbg / max(fxParams1.z, 1.0f)), 0.15f);
            return float4(khaR, 0.0f, khaR, 1.0f);
        }

        // 12 = the competing gap, the metres that WOULD be needed. METRIC
        // LADDERS, reclaiming two of the retired 11-14 slots.
        if (dbgCtl.x >= 10.5f && dbgCtl.x < 12.5f) {
            float khaM = (dbgCtl.x < 11.5f) ? khaPullDbg : khaGapDbg;
            if (khaM <  0.0f) return float4(0.25f, 0.25f, 0.25f, 1.0f);
            if (khaM <= 0.0f) return float4(0.0f, 0.0f, 0.0f, 1.0f);
            if (khaM <= 0.5f) return float4(0.0f, 0.0f, 0.45f, 1.0f);
            if (khaM <= 1.0f) return float4(0.0f, 0.0f, 1.0f, 1.0f);
            if (khaM <= 2.0f) return float4(0.0f, 0.9f, 1.0f, 1.0f);
            if (khaM <= 4.0f) return float4(0.0f, 0.9f, 0.0f, 1.0f);
            if (khaM <= 8.0f) return float4(1.0f, 1.0f, 0.0f, 1.0f);
            if (khaM <= 15.0f) return float4(1.0f, 0.45f, 0.0f, 1.0f);
            return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }

    }

)HLSL" R"HLSL(    if (occ) discard;

    // DEBUG VISUAL 15: PAINTER FORENSICS - post-guard, post-everything:
    // whichever path actually OWNS a pixel declares itself. PSComposite (the
    // guarded injection) = ORANGE; PSMain (flush / plain fallback) = CYAN.
    if (dbgCtl.x >= 14.5f && dbgCtl.x < 15.5f) return float4(1.0f, 0.35f, 0.0f, 1.0f);

    // DEBUG VISUAL 16: HEIGHTFIELD FORENSICS (pre-verdict paint: the thm
    // lane's clearance, PRE-discard would hide it - so this mode never
    // reaches here for buried pixels; buried fragments were discarded above
    // unless the lane is off.
    if (dbgCtl.x >= 15.5f && dbgCtl.x < 16.5f) {
        if (khtClear > 1.9e9f) return float4(0.4f, 0.4f, 0.4f, 1.0f);
        if (khtClear > 0.9e9f) return float4(0.0f, 0.0f, 1.0f, 1.0f);
        float khtR = saturate(khtClear / 200.0f);
        return float4(0.0f, max(khtR, 0.08f), 0.0f, 1.0f);
    }

    if (dbgCtl.x >= 18.5f && dbgCtl.x < 19.5f) return SunSelfProbe(i.wpos, i.nrm);   // visual 19
    if (dbgCtl.x >= 31.5f && dbgCtl.x < 32.5f) return KhSelfCertProbe(i.wpos, i.wrel, i.nrm);
    // DEBUG VISUAL 21 (setRenderDebug 269): the self-tier probe.
    if (dbgCtl.x >= 20.5f && dbgCtl.x < 21.5f) return KhSelfTierProbe(i.wpos, i.wrel, i.nrm);
    if (dbgCtl.x >= 19.5f && dbgCtl.x < 20.5f) {
        if (mirMeta.x < 0.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
        return KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z) < 0.5f
             ? float4(0.0f, 0.2f, 1.0f, 1.0f)
             : float4(0.0f, 1.0f, 0.2f, 1.0f);
    }
    if (dbgCtl.x >= 17.5f && dbgCtl.x < 18.5f) return ShadowBandContent(i.wpos);
    if (dbgCtl.x >= 16.5f && dbgCtl.x < 17.5f) {
        if (lighting0.x < 0.5f) return float4(0.25f, 0.25f, 0.25f, 1.0f);
        if (lighting1.w < 0.5f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
        if (lightAmb.w < 0.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
        float khlAmbT = max(lightAmb.r, max(lightAmb.g, lightAmb.b)) * lighting0.z;
        float khlSunT = max(lighting2.r, max(lighting2.g, lighting2.b)) * lighting0.w;
        return float4(0.0f, saturate(khlAmbT), saturate(khlSunT / 8.0f), 1.0f);
    }

    // DEBUG VISUAL (dbgCtl.x, setRenderDebug): see PSMain's note - same
    // modes, same colors, this path's own scene distance. Modes 1-3 ONLY: 4
    // falls through to normal shading (its fog-kill gate lives at the fog
    // blocks) and 5-7 to the isolation branch after lc.
    if (dbgCtl.x >= 0.5f && dbgCtl.x < 3.5f) {
        if (dbgCtl.x < 1.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);

        if (dbgCtl.x < 2.5f) {
            float dz = sceneZ - fragZ;
            if (abs(dz) < 0.25f) return float4(0.0f, 1.0f, 0.0f, 1.0f);
            if (dz < 0.0f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
            return float4(0.0f, 0.0f, 0.2f + 0.8f * saturate(dz * 0.02f), 1.0f);
        }

        return float4(1.0f, 1.0f, 1.0f, 0.15f);
    }

    int bm = (int)sizeAxes.w;

#if KH_TEXTURED
    // KH_TEXTURED: sample BELOW the FAR CONTRACT + guard blocks (the
    // section-4 checkpoint - the textured twin adds no return/discard above
    // them), cutout-clip, then build the mapped shading normal. The GEOMETRIC
    // normal keeps owning the receive gating below - shadow behavior stays in
    // parity with the untextured twin.
    KhMatSurf khtxS = KhSampleMat(i.uv);
    // KH_DLSW_MESHALPHAMODE (26876l, mode 571) - THE TEXTURED PATH'S OWN CLIP,
    // READ BEFORE IT FIRES. TWIN EDIT: PSMain and PSComposite.
    //
    // The field found the artifact happens ONLY on PBR meshes - an untextured
    // primitive in the same scene is clean. That is the sharpest fact in this
    // whole investigation, because the textured path contains a discard the
    // untextured path does not run at all: the cutout clip on the line below,
    // and the blend-split clips after it. A clip punches HOLES, holes show the
    // world behind, and the world behind at night in shadow is BLACK - filled
    // black shapes of the background geometry, parallaxing with it, exactly as
    // reported and nothing like the faint tint I spent five builds chasing.
    //
    // Mode 565 could not see this: it paints khtxS.alpha AFTER the opaque
    // contract has forced it to 1, so it reported white on a surface whose raw
    // texture alpha may be full of holes. This paints the raw values, above
    // every clip.
    //
    //   RED   = matParams0.y / 4, the alpha MODE. 0 black = opaque and no clip
    //           runs; 0.25 = CUTOUT and the clip below is live; 0.5 / 0.75 =
    //           the two halves of a blend material, both of which clip.
    //   GREEN = the raw sampled alpha, before the opaque contract forces it.
    //   BLUE  = matParams0.z, the cutout threshold. Green darker than blue
    //           anywhere is a texel the clip is about to kill.
    //
    // Red black everywhere means the material is opaque, no clip runs, and this
    // hypothesis is dead on one screenshot.
    if (KhDlsMeshDbg() == 571) {
#if KH_ARB_DEPTH
        // Mandatory on this variant - an unwritten SV_Depth is undefined and
        // would change what survives the test, corrupting the measurement.
        khaODepth = i.pos.z;
#endif
        return float4(matParams0.y * 0.25f, khtxS.alpha, matParams0.z, 1.0f);
    }
    // matParams0.y = alpha mode: 0 opaque, 1 cutout, 2 = a blend material's
    // TRANSLUCENT texels (the flush's post-scene part), 3 = the SAME blend
    // material's OPAQUE texels (the ordinary depth-writing draw). KH_MAT_BLEND.
    if (matParams0.y >= 0.5f && matParams0.y < 1.5f) clip(khtxS.alpha - matParams0.z);   // cutout kill
    // OPAQUE ALPHA CONTRACT (the whitish-see-through fix): sampled alpha
    // NEVER reaches the blend on the opaque and cutout modes - survivors
    // draw at alpha 1. KH_MAT_BLEND (26760): a blend material is split at
    // alpha 0.996 - texels at or above it are solid and draw in the opaque
    // phase WITH depth (mode 3: the rest clipped, alpha forced 1), so a hull
    // still occludes itself; texels below it draw in the translucent tail
    // (mode 2: solid texels and fully transparent ones clipped, sampled
    // alpha kept and fed to the SAME 'a' line as the object colour's alpha
    // below, hardware-blended without a depth write).
    if (matParams0.y >= 1.5f) {   // KH_MAT_SPLIT_TEXEL (26768): one verdict per TEXEL, both parts. TWIN EDIT.
        // KH_MAT_SPLIT_TOL (26769): the verdict tolerates compression. BC3/BC7
        // alpha in a block that also holds transparent texels lands an opaque
        // texel at ~0.93-0.98 - the rectangular bites, one block each. Solid
        // is >= 0.9; a designed glass (0.3-0.6) still blends. TWIN EDIT.
        const float khtxCls = KhMatRouteTexel(matParams3.y, 1.0f, i.uv);
        if (matParams0.y >= 2.5f) {
            clip(khtxCls - 0.9f);
            khtxS.alpha = 1.0f;
        } else {
            if (khtxCls >= 0.9f) discard;
            clip(khtxS.alpha - 0.004f);
        }
    } else {
        khtxS.alpha = 1.0f;
    }
    float3 khtxN;
    {
        float3 khtn = normalize(i.nrm);
        float3 khtt = i.tanw.xyz - khtn * dot(khtn, i.tanw.xyz);
        float khttl = length(khtt);
        if (khttl > 1.0e-5f) {
            khtt /= khttl;
            float3 khtb = cross(khtn, khtt) * i.tanw.w;
            khtxN = normalize(khtt * khtxS.nrmT.x + khtb * khtxS.nrmT.y + khtn * khtxS.nrmT.z);
        } else khtxN = khtn;   // degenerate tangent: geometric normal
        // KH_DLSW_MESHGEOM (26877a, mode 577) - THE BISECT THE OPERATOR'S OWN
        // OBSERVATION ASKS FOR. TWIN EDIT: PSMain and PSComposite carry this
        // identically.
        //
        // The field reports the see-through is visible ONLY where the NORMAL
        // MAP darkens the surface at that viewing angle - not where the mesh is
        // directly lit, and not at all on an untextured primitive. That is the
        // first clue anyone has had that separates the textured path from the
        // untextured one by something other than "it has a texture", and the
        // previous handoff already named this probe as the obvious next bisect
        // and recorded that it did not exist.
        //
        // This drops the MAPPED normal and shades the textured path with the
        // GEOMETRIC one, changing nothing else - same material, same albedo,
        // same roughness, same alpha, same shadow, same lighting. It splits the
        // two readings of the clue, which have opposite fixes:
        //
        // *** FIELD RESULT: ARTIFACT GONE. Recorded here because the two
        // *** FOLLOW-UPS THAT ANSWER WERE READ INTO BOTH FAILED, and the next
        // *** reader needs the failures more than the hypothesis.
        //
        // 578 gave the dynamic-light shadow QUERY the geometric normal while
        // the shading kept the mapped one: INDISTINGUISHABLE FROM MODE 0. So
        // KhDlsShadow's receiver-normal offset is not the carrier, and that
        // whole branch - the 8.8 cm per-texel displacement of the query point -
        // is refuted, not merely untested. Reverted.
        //
        // 579 gave the smf SHADOW GATE (dot(khShN, lighting1) > 0.01) the
        // geometric normal: DID NOT FIX THE SEE-THROUGH AND BROKE THE SUN.
        // With the gate uniform over a face, every pixel of a lit mesh reached
        // the screen-space world-shadow term instead of only the texels past
        // the terminator, so the world's cascade shadow printed across our
        // meshes at full strength in daylight. The gate was MASKING the carrier,
        // not being it. Reverted, and sun/moon was out of scope to begin with.
        //
        // What survives: 577 cures it, so the mapped normal IS in the chain,
        // and neither of its two geometric consumers is the route. That leaves
        // the SHADING consumers - N.L, the GGX lobe, the per-light ambient -
        // and whatever the value they scale is carrying.
        if (KhDlsMeshDbg() == 577) khtxN = normalize(i.nrm);
    }
#endif
)HLSL" R"HLSL(    float smf = 1.0f;
    float  khStenZ = KhVolZ(i.wpos);
    float2 khStenG = float2(ddx(khStenZ), ddy(khStenZ));
    // An unguarded reference kills ps_composite and the box never draws;
    // shipped exactly that, in the wrong chunk. Taken here, outside the
    // divergent N.L branch, for the same quad-op legality as the pair above.
    // THE WITNESS COMPARES WHAT WE WROTE ('s fix, in its correct chunk this
    // time).
    float2 khStenR = i.pos.xy;
#if KH_ARB_DEPTH
    khStenR = KhStenTapComp(i.pos.xy, i.wpos, i.pos.w);
    // Open-ended at >= 3.5, this armed mode 182's witness revert for any
    // HIGHER value too, so 199 and 200 were never one-thing arms: they also
    // flipped khStenP's source. RANGE-BOUNDED - see the mode-108 reader.
    float3 khStenP = float3((dbgCtl.w >= 3.5f && dbgCtl.w < 4.5f) ? i.pos.z : khaODepth,
                            ddx(i.pos.z), ddy(i.pos.z));
#else
    float3 khStenP = float3(i.pos.z, ddx(i.pos.z), ddy(i.pos.z));
#endif
    // At steady motion the two agree to sub-pixel (0.6 px measured across 18
    // m/s rows) and the 2 px DEADBAND keeps the historic raster tap
    // BIT-EXACTLY - so the halo regime that killed (a standing ~30 px
    // displacement on every moving frame) cannot form.
    {
        float4 khcy_c = mul(float4(i.wpos, 1.0f), stenCycVp);
        if (khcy_c.w > 1.0e-4f) {
            float2 khcy_px = float2((khcy_c.x / khcy_c.w * 0.5f + 0.5f) * stenVol.z,
                                    (0.5f - khcy_c.y / khcy_c.w * 0.5f) * stenVol.w);
            float2 khcy_d = khcy_px - khStenR;
            if (dot(khcy_d, khcy_d) > 4.0f &&
                khcy_px.x >= 0.0f && khcy_px.y >= 0.0f &&
                khcy_px.x <= stenVol.z - 1.0f && khcy_px.y <= stenVol.w - 1.0f)
                khStenR = khcy_px;
        }
    }
    // PSComposite declares khStenP in BOTH branches of that conditional; this
    // block sat only in the #else, so the ARB variant never declared khBiasN
    // while the call site below the #endif used it. THE BIAS SLOPE WANTS THE
    // FACET, NOT THE VERTEX NORMAL. Ledger at KH_FACET_BIAS_NORMAL.
    float3 khFacetN = cross(ddx(i.wpos), ddy(i.wpos));
    float khFacetL = length(khFacetN);
    float3 khBiasN = normalize(i.nrm);
    if (khFacetL > 1.0e-12f) {
        khFacetN /= khFacetL;
        if (dot(khFacetN, khBiasN) < 0.0f) khFacetN = -khFacetN;
        if ((lighting0.y >= 52.5f && lighting0.y < 53.5f) &&
            dot(khFacetN, khBiasN) > 0.5f &&
            abs(dot(khFacetN, lighting1.xyz)) < abs(dot(khBiasN, lighting1.xyz)))
            khBiasN = khFacetN;   // opt-in (369)
    }
    if (lighting0.y >= 18.5f && lighting0.y < 19.5f) khBiasN = normalize(i.nrm);   // 233 (alias of default)

#if KH_TEXTURED
    float3 khShN = (lighting0.y >= 14.5f && lighting0.y < 15.5f) ? normalize(i.nrm) : khtxN;
#else
    float3 khShN = normalize(i.nrm);
#endif
    if (lighting0.x >= 0.5f && dot(khShN, lighting1.xyz) > 0.01f) {
        // MODE 167 (lighting0.y == 3): cascade receive forced lit - the
        // split's cascade arm. TWIN EDIT: PSMain and PSComposite identical.
        if (lighting0.y < 2.5f || lighting0.y >= 3.5f) {
            // lighting0.y 59 (mode 381) restores the absolute interpolant:
            // the one-switch A/B (crawl and jitter return together). TWIN
            // EDIT: PSMain and PSComposite identical.
            if (maskMeta.x >= 0.5f) smf = ShadowBandFactor(
                (lighting0.y >= 58.5f && lighting0.y < 59.5f)
                    ? i.wpos : (i.wrel + sunOrigin.xyz));
            else                    smf = ShadowMapFactor(i.wpos);   // yzw re-lettered (were zero)
        }

        // Placed at a statement boundary between two independent receive
        // terms, so no expression is split. PSMain's twin segment is well
        // under budget and is deliberately NOT split - the twins differ in
        // whitespace only.
)HLSL" R"HLSL(
        if (dbgCtl.x >= 30.5f && dbgCtl.x < 31.5f)   // VISUAL 31
            return KhPfProbe2(i.wpos, i.wrel, khBiasN);
        if (dbgCtl.x >= 29.5f && dbgCtl.x < 30.5f)   // VISUAL 30
            return KhPfProbe(i.wpos, i.wrel, khBiasN);
        if (dbgCtl.x >= 28.5f && dbgCtl.x < 29.5f) {
            float khv29_s = 1.0f - SunShadowFactorSelf(i.wpos, i.wrel, khBiasN);
            float khv29_u = 0.0f;
            if (maskMeta.w >= 0.5f)
                khv29_u = 1.0f - ((stenVol2.x >= 0.5f && KhVolMode() == 6)
                        ? KhVolSoftScene(khStenR, khStenP, KH_STEN_TOL_W(i.pos.w), 6)
                        : KhStenUnit(i.wpos, i.pos.xy, khStenG));
            return float4(1.0f - smf, khv29_s, khv29_u, 1.0f);
        }
        // TWIN EDIT: PSMain and PSComposite identical.
        if (lighting0.y < 4.5f || lighting0.y >= 5.5f)
            smf = min(smf, SunShadowFactorSelf(i.wpos, i.wrel, khBiasN));
        if (maskMeta.w >= 0.5f &&
            (lighting0.y < 3.5f || lighting0.y >= 4.5f)) {
            float khStenU = (stenVol2.x >= 0.5f && KhVolMode() == 6)
                          ? KhVolSoftScene(khStenR, khStenP,
                                           KH_STEN_TOL_W(i.pos.w), 6)
                          : KhStenUnit(i.wpos, i.pos.xy, khStenG);
            // On PSMain it is the EFFECT ID, so the fade is NOT applied there
            // and must not be 'restored' to match.
#if KH_ARB_DEPTH
            // SIGN-AGNOSTIC GATE, and it is a defect repair, not a tuning.
            // fxMeta.x's SIGN is mode 188's carrier for the near-gap ramp ,
            // so the signed test made 188 silently revert this fade as well -
            // the 1.8 class on a different lane. The magnitude is the near
            // either way and the body already reads abs.
            if (abs(fxMeta.x) > 0.0f) {
                float khStenF = saturate((i.pos.w / max(abs(fxMeta.x), 1.0e-4f) - 1.0f)
                                         / max(KH_STEN_FADE - 1.0f, 1.0e-4f));
                if (dbgCtl.w >= 6.5f && dbgCtl.w < 7.5f) khStenF = 1.0f;   // 201 reverts
                // KH_VOL_MIRROR: inside the fade the stencil verdict now
                // comes from the mirror counting pass instead of a flat 1.0 -
                // shadow scales through the last metre instead of washing
                // out.
                float khMirF = 1.0f;
                if (mirMeta.x >= 0.5f && !(lighting0.y >= 23.5f && lighting0.y < 24.5f)) {
                    khMirF = KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z);
                }
                khStenU = lerp(khMirF, khStenU, khStenF);
            }
#endif
            // KH_TRANSL_STEN_MIRROR (26765): A TRANSLUCENT TEXEL TAKES THE
            // MIRROR VERDICT AT EVERY DISTANCE. The volume term above starts
            // from a witness compare - the engine depth at this pixel must be
            // this fragment's - and a translucent texel wrote no depth: the
            // pixel holds whatever is BEHIND the glass, the witness fails,
            // the 7x7 search finds no matching plane and the fallback answers
            // with the background's stencil, which changes with the camera
            // (the oscillation; a nearby engine volume pinning the background
            // is why a shot 'fixed' it). The mirror counts the engine's
            // volumes against OUR depth, glass included whole by the prepass,
            // so its count at this pixel is the one measured AT the glass -
            // the only such count there is. Solid texels keep the engine's
            // exact count (and the ARB near fade above). A translucent texel
            // = the blend material's translucent part (matParams0.y 2) or a
            // whole translucent object on normal blend (the interpolated
            // colour alpha below 0.999 at blend id 0). mirMeta.x 2 = mode
            // 485, the A/B (the mirror still valid for the fade above).
            // TWIN EDIT: PSMain and PSComposite carry the identical block.
            if (mirMeta.x >= 0.5f && mirMeta.x < 1.5f &&
                !(lighting0.y >= 23.5f && lighting0.y < 24.5f) &&
                ((matParams0.y >= 1.5f && matParams0.y < 2.5f) ||
                 (i.icol.a < 0.999f && bm == 0))) {
                khStenU = KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z);
            }
            // After the near-collapse mirror lerp: the fade thins the final
            // verdict.
            float khStRf = (lighting0.y >= 42.5f && lighting0.y < 43.5f)
                         ? 1.0f : KhSunRangeFade(i.wpos);
            smf *= 1.0f - (1.0f - khStenU) * khStRf;
        }
    }

    // Slices surviving this are painted by the ENGINE over our pixels, not by
    // any term of ours. TWIN EDIT: PSMain and PSComposite identical.
    if (lighting0.y >= 1.5f && lighting0.y < 2.5f) smf = 1.0f;
#if KH_TEXTURED
    khtxS.albedo *= i.icol.rgb;   // the object colour tints the albedo lane only (KH_INSTANCING: the interpolant)
#if KH_USER_MAT
    float3 lc = KhUserShade(khtxS, i.wpos, khtxN, smf);
#else
    float3 lc = KhApplyPBR(khtxS, i.wpos, khtxN, smf);
#endif
#else
    float3 lc = ApplyLighting(i.icol.rgb, i.wpos, i.nrm, smf);
#endif

    if (dbgCtl.x >= 4.5f && dbgCtl.x < 7.5f) {
        if (dbgCtl.x < 5.5f) return float4(i.icol.rgb, 1.0f);
        if (dbgCtl.x < 6.5f) return float4(lc, 1.0f);
        return float4(i.icol.a, SolidMask(i.wpos), smf, 1.0f);
    }

        if (maskMeta.y >= 0.5f) {
            float kmv = khShadowMask.Load(int3(KhMaskPx(i.pos.xy), 0)).r;
            return float4(kmv, kmv, kmv, 1.0f);
        }

        if (maskMeta.z >= 0.5f) return KhStenPaintU(i.wpos, i.pos.xy, maskMeta.z);

)HLSL" R"HLSL(   // CHUNK BOUNDARY - the visual-23 ledger took PSComposite's
    // segment 500 B past the 16380-byte MSVC token cap (C2026), caught by the
    // sweep gate exactly as 's did on PSMain's twin one build earlier. It
    // returns FLAT MAGENTA with alpha 1 from BEFORE the atmospheric block -
    // no fog, no haze, no ramp, no target, no transmittance of any kind
    // reaches the output.
    if (dbgCtl.x >= 22.5f && dbgCtl.x < 23.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
    // The census settled the partition question: partVpLo/Hi 0.011/0.999 are
    // IDENTICAL to partSceneVpLo/Hi and partTrigAccLo/Hi,
    // partTrigRej/partRejLoMax never updated and partSkySpans is 0 - the
    // engine gives the far content no viewport sub-range and our routed draw
    // shares one range with the whole scene.
    if (dbgCtl.x >= 26.5f && dbgCtl.x < 27.5f) {
        float khod = i.pos.z;
        if (khod >= 0.9990f) return float4(1.0f, 1.0f, 1.0f, 1.0f);   // at the viewport max
        if (khod >= 0.9975f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
        if (khod >= 0.9950f) return float4(1.0f, 0.45f, 0.0f, 1.0f);
        if (khod >= 0.9900f) return float4(1.0f, 1.0f, 0.0f, 1.0f);
        if (khod >= 0.9800f) return float4(0.0f, 0.9f, 0.0f, 1.0f);
        return float4(0.0f, 0.4f, 1.0f, 1.0f);
    }
    // Both previous ladders painted OUR pair and were blind by construction:
    // with fog on the mesh is far-keep routed (farKeepMeshDraws 2027/2028),
    // so depthParams carries the 20 km pair and its ndc tops out near 0.993 -
    // visual 24 could never show white.
    if (dbgCtl.x >= 25.5f && dbgCtl.x < 26.5f) {
        if (khFarSplit.w <= 0.5f) return float4(0.5f, 0.5f, 0.5f, 1.0f);
        float khef = khFarSplit.x + khFarSplit.y / max(i.pos.w, 1.0e-4f);
        if (khef >= 1.0f)   return float4(1.0f, 1.0f, 1.0f, 1.0f);
        if (khef >= 0.999f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
        if (khef >= 0.99f)  return float4(1.0f, 0.45f, 0.0f, 1.0f);
        if (khef >= 0.95f)  return float4(1.0f, 1.0f, 0.0f, 1.0f);
        return float4(0.0f, 0.4f, 1.0f, 1.0f);
    }
    if (dbgCtl.x >= 24.5f && dbgCtl.x < 25.5f) {
        float khdw = i.pos.w;
        if (khdw <  500.0f) return float4(0.0f, 0.35f, 1.0f, 1.0f);   // blue <500
        if (khdw < 1000.0f) return float4(0.0f, 0.9f,  0.0f, 1.0f);   // green 500-1000
        if (khdw < 1400.0f) return float4(1.0f, 1.0f,  0.0f, 1.0f);   // yellow 1000-1400
        if (khdw < 1531.0f) return float4(1.0f, 0.45f, 0.0f, 1.0f);   // orange 1400-1531 (fog ramp end)
        if (khdw < 1547.0f) return float4(1.0f, 0.0f,  0.0f, 1.0f);   // red 1531-1547 (engine far)
        if (khdw < 2500.0f) return float4(0.6f, 0.0f,  0.9f, 1.0f);   // purple 1547-2500
        return float4(1.0f, 1.0f, 1.0f, 1.0f);   // white >2500
    }
    if (dbgCtl.x >= 23.5f && dbgCtl.x < 24.5f) {
        float khdz = depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f);
        if (khdz >= 1.0f)   return float4(1.0f, 1.0f, 1.0f, 1.0f);   // clamped at far
        if (khdz >= 0.999f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
        if (khdz >= 0.99f)  return float4(1.0f, 0.45f, 0.0f, 1.0f);
        if (khdz >= 0.95f)  return float4(1.0f, 1.0f, 0.0f, 1.0f);
        if (khdz >= 0.90f)  return float4(0.0f, 0.9f, 0.0f, 1.0f);
        return float4(0.0f, 0.4f, 1.0f, 1.0f);
    }
    // Mode 4 and up kill the block, so the new visual has to be admitted
    // explicitly exactly as visual 13 was.
    if ((fogParams.w >= 0.5f || hazePars.w >= 0.5f || fogEngine.w >= 0.5f) &&
        (dbgCtl.x < 3.5f || (dbgCtl.x >= 12.5f && dbgCtl.x < 13.5f)
                         || (dbgCtl.x >= 21.5f && dbgCtl.x < 22.5f))) {   // mode 4 kills both (see g_dbg_mode)
        float distM = i.pos.w;
        float hgt = i.wpos.y;
        float camY = fogColor.w;
        // The exponential-lambda era and its spectral fog approximation are
        // retired: the engine's spectral vectors extinguish LIGHTING, not
        // fog.
)HLSL" R"HLSL(   // CHUNK BOUNDARY - FIFTH C2026 CATCH OF THIS
        // CAMPAIGN, and the first outside the shared block.
        float trans = 1.0f;
        // KH_FARVIS_NO_VDIST - (mode 322 reverts; catalog ledger).
        float khaFbLay = fogSkyCol.w;
        bool  khaFbOn  = fogBelow.y >= 0.5f && camY < khaFbLay;
        float khaFbA   = distM;   // path above the layer
        float khaFbB   = 0.0f;   // path below it
        float khaFbRef = camY;   // the height reference
        if (khaFbOn) {
            float khaFbF = saturate((khaFbLay - camY) /
                                    (max(hgt - camY, 0.0f) + 1.0e-5f));
            khaFbB   = distM * khaFbF;
            khaFbA   = distM - khaFbB;
            khaFbRef = khaFbLay;
        }
        if (fogEngine.w >= 0.5f && fogEngine.w < 1.5f && blendCtl.z < 0.5f)
            trans = saturate((fogEngine.y - khaFbA) * fogEngine.z);

        if (fogParams.w >= 0.5f) {
            if (fogEngine.w >= 0.5f) {
                // Below it they carry the export's above-path and layer
                // reference, and a ray wholly below the layer (khaFbA == 0)
                // takes no height fog.
                float dh = abs(hgt - khaFbRef);
                float k = fogParams.y * dh / max(khaFbA, 1.0e-4f);
                float integ = k < 1.0e-6f ? khaFbA : (1.0f - exp(-khaFbA * k)) / k;
                float minY = khaFbOn ? min(khaFbLay, hgt) : min(hgt, camY);
                trans *= exp(-integ * fogEngine.x * exp(-fogParams.y * max(minY, 0.0f)));
            } else {
                // block not locked (sub-second cold): the legacy exponential
                float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
                trans = exp(-distM * dens * 0.0153f);
            }
        }

        // THE SECOND EXTINCTION: the engine's distance haze, which multiplies
        // into this same transmittance and is armed whether or not fog is.
        // fogSkyCol.w carries the engine fog-layer altitude.
        trans *= KhHazeT(distM, hgt, camY, fogSkyCol.w);
        float khaAR = trans;
        float khaBt = khaFbOn ? exp(-khaFbB * fogBelow.x) : 1.0f;
        trans *= khaBt;
        // DEBUG VISUAL 13: TRANSMITTANCE METRIC LADDER. The fog reports have
        // all been adjectives - 'over-fogged', 'barely affected' - and two
        // builds were shipped and reverted on them. trans 1 = fully clear.
        // BLACK <=0.01, navy <=0.05, blue <=0.1, cyan <=0.25, green <=0.5,
        // yellow <=0.75, orange <=0.9, red <=0.99, WHITE > 0.99.
)HLSL" R"HLSL(   // CHUNK BOUNDARY - the visual-14 pass legend took this
        // MSVC caps one string literal token at 16380 bytes; splitting at a
        // statement boundary costs nothing at runtime and buys the next edit
        // room. segment to 98.5%% with 241 B free. PSMain's twin segment is
        // well under budget and is deliberately NOT split.
        if (dbgCtl.x >= 13.5f && dbgCtl.x < 14.5f) {
            float khbi = ShadowBandIndex(i.wpos);
            if (khbi < 0.0f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
            // BLACK = no slab contained it and the coverage fallback supplied
            // the band. Black IS the fold doing work; magenta under 217 is
            // the same fragment shading unshadowed.
            if (khbi > 7.5f) return float4(0.0f, 0.0f, 0.0f, 1.0f);
            float khbf = khbi * 0.125f;
            return float4(saturate(1.5f - khbf * 4.0f),
                          saturate(khbf * 4.0f) * saturate(2.5f - khbf * 4.0f),
                          saturate(khbf * 4.0f - 1.5f), 1.0f);
        }
        if (dbgCtl.x >= 12.5f && dbgCtl.x < 13.5f) {
            if (trans <= 0.01f) return float4(0.0f, 0.0f, 0.0f, 1.0f);
            if (trans <= 0.05f) return float4(0.0f, 0.0f, 0.45f, 1.0f);
            if (trans <= 0.10f) return float4(0.0f, 0.0f, 1.0f, 1.0f);
            if (trans <= 0.25f) return float4(0.0f, 0.9f, 1.0f, 1.0f);
            if (trans <= 0.50f) return float4(0.0f, 0.9f, 0.0f, 1.0f);
            if (trans <= 0.75f) return float4(1.0f, 1.0f, 0.0f, 1.0f);
            if (trans <= 0.90f) return float4(1.0f, 0.45f, 0.0f, 1.0f);
            if (trans <= 0.99f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
            return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        // Field values (0.25, 0.75, 3.5): dim looking down, bright toward
        // zenith - the warm-over-sand / blue-against-sky behavior no constant
        // could produce. The convergence capture and its isolation weighting
        // retire here: scaffolding that served until the real source was
        // located.
        float3 fog_target = fogColor.rgb;

        if (fogSky.w >= 0.5f) {
            float dirY = (hgt - camY) / max(distM, 1.0e-4f);
            float g;

            if (dirY < 0.0f) {
                float u = dirY + 1.0f;
                g = u * u * (fogSky.y - fogSky.x) + fogSky.x;
            } else {
                g = dirY * (fogSky.z - fogSky.y) + fogSky.y;
            }

            fog_target = fogSkyCol.rgb * g;
        }
        // KH_FOG_UW_TARGET - (mode 325 reverts; catalog ledger). BELOW THE
        // LAYER THE ENGINE CONVERGES ON A DIFFERENT COLOUR, AND THAT - NOT
        // the extinction - IS THE WHITE BOX.
        if (khaFbOn && fogUw.w >= 0.5f) {
            float khaUwY = (hgt - camY) / max(distM, 1.0e-4f);
            float khaUwG;
            if (khaUwY < 0.0f) {
                float khaUwU = khaUwY + 1.0f;
                khaUwG = khaUwU * khaUwU * (fogUwGrad.y - fogUwGrad.x) + fogUwGrad.x;
            } else {
                khaUwG = khaUwY * (fogUwGrad.z - fogUwGrad.y) + fogUwGrad.y;
            }
            float khaWp = khaBt * (1.0f - khaAR);   // the PSC_FogColor weight
            float khaWs = 1.0f - khaBt;   // the sky-colour weight
            fog_target = (fog_target * khaWp + fogUw.rgb * khaUwG * khaWs) /
                         max(khaWp + khaWs, 1.0e-5f);
        }

        // At full optical depth our mesh paints exactly fog_target and
        // nothing else, so a silhouette can only exist if fog_target differs
        // from what the engine leaves on the pixels around it. Deliberately
        // placed AFTER the fogSky gradient resolves, so it paints the target
        // actually handed to the lerp, not a stand-in.
        if (dbgCtl.x >= 21.5f && dbgCtl.x < 22.5f) return float4(fog_target, 1.0f);
        lc = lerp(fog_target, lc, trans);
    }
#if KH_TEXTURED
    float a = i.icol.a * khtxS.alpha * SolidMask(i.wpos);
#else
    float a = i.icol.a * SolidMask(i.wpos);
#endif
    // KH_DLSW_MESHALPHA (26876g, mode 565) - THE OPACITY, SPLIT INTO ITS THREE
    // FACTORS. TWIN EDIT: PSMain and PSComposite carry this identically.
    //
    // 562 (flat white), 564 (smf), 519 (no dynamic-light shadow) and 558/539
    // (the world pass) between them exclude blending, the screen-space shadow
    // chain, the whole shadow feature and stage 4. What survives that is a hard
    // constraint rather than a hunch: background structure can only reach our
    // mesh's colour through a SCREEN-SPACE READ OF A BUFFER CONTAINING THE
    // BACKGROUND, and there is exactly one left in this shader - the perceptual
    // composite below, which Loads sceneColorTex at our own pixel and lerps by
    // this alpha. At a = 1 that lerp is an exact identity round trip and leaks
    // nothing. Below 1 it mixes the pre-mesh scene capture into our surface in
    // proportion, which is the reported artifact word for word.
    //
    // It also explains why 562 looked clean without clearing the route: at
    // flat white, lc is 1.0 and a small scene contribution is swamped, while in
    // mode 0 the shadowed parts of the mesh are DARK and the same contribution
    // is large in relative terms - "faint, and worse where the surface is
    // darker" is exactly what a fixed-fraction mix of a night scene looks like.
    //
    // RED = the object colour alpha, GREEN = the material alpha, BLUE =
    // SolidMask. Their product is the lerp weight. Any channel below white
    // names its own culprit, which is why this paints the factors and not the
    // product (the handoff records what checking the container instead of the
    // content cost last time).
    if (KhDlsMeshDbg() == 565) {
#if KH_TEXTURED
        return float4(i.icol.a, khtxS.alpha, SolidMask(i.wpos), 1.0f);
#else
        return float4(i.icol.a, 1.0f, SolidMask(i.wpos), 1.0f);
#endif
    }
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
)HLSL" R"HLSL(    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);

    // Simple transparency per the spec is a DISPLAY- space mix, so: sample
    // the pre-mesh scene capture at this pixel, blend in Reinhard space,
    // invert, write OPAQUE - the destination can never dominate again.
    if (dbgCtl.x >= 7.5f && dbgCtl.x < 8.5f) {
        float3 pscn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float plum = dot(pscn / (1.0f + pscn), float3(0.333f, 0.333f, 0.334f));
        return float4(blendCtl.x, plum, a, 1.0f);
    }

    if (blendCtl.x >= 0.5f) {
        // KH_DLSW_MESHOPAQUE twin (mode 566) - see the PSMain site.
        float khb_a = (KhDlsMeshDbg() == 566 || sceneZ > blendCtl.y) ? 1.0f : a;
        float3 scn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float3 ts = scn / (1.0f + scn);
        float3 tl = lc / (1.0f + lc);
        float3 tm = lerp(ts, tl, khb_a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // cap ~HDR 255
    }

    return float4(lc, a);
}
)HLSL"
