// composite2.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.

struct VSOutC { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    float3 wrel : TEXCOORD4;   // KH_SELF_REL_INTERP (at VSOut).
    float4 icol : TEXCOORD5;   // KH_INSTANCING: the object colour interpolant (at VSOut).
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // World tangent + handedness.
#endif
};

// KH_INSTANCING: wrappers over KhVsCore (the shared prefix), the per-object one
// Over the CB lanes, the instanced one over the stream. The composite fills
// never write stenVol2.z non-zero (the seam prepass does, for VSMirror); the
// core carries the ladder verbatim. Twin:
// VSMain / VSMainInst in the static unit.
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
    // Tangents are covariant (transform like positions, not normals): per-axis
    // scale then the object rotation, renormalized. The handedness sign rides
    // untouched in w.
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

#if KH_ARB_DEPTH
float4 PSComposite(VSOutC i, out float khaODepth : SV_Depth) : SV_Target
#else
float4 PSComposite(VSOutC i) : SV_Target
#endif
{
    if (blendCtl.w != 0.0f) {
        float khlD = frac(52.9829189f * frac(dot(i.pos.xy, float2(0.06711056f, 0.00583715f))));
        if (blendCtl.w > 0.0f) { if (khlD >= blendCtl.w) discard; }
        else if (khlD < -blendCtl.w) discard;
    }
    ClipEdgeSliver(i.wpos, i.nrm);   // Degenerate edge-on fragments (fireflies).
    ClipOwnNear(i.pos.w);   // Our own near plane. Twin call.
    if (shadowMeta2.x < 0.5f && depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f) > 1.0f) discard;
    if (shadowMeta2.x < 0.5f && shadowMeta2.y > 0.0f && i.pos.w > shadowMeta2.y) discard;
    // Rejecting here - before the scene read, the analytic clamp, five shadow
    // tiers and PBR - is the early-Z this SV_Depth (late-Z) variant can never
    // get from the hardware. The injection's own owner map says whether another
    // admitted mesh already holds every sample of this pixel strictly nearer.
    // Lanes zero on every non-injection fill.
    if (shadowMeta2.w > 0.5f && shadowMeta2.z > 0.5f &&
        KhOwnerRejects(i.pos.xy, i.pos.z, shadowMeta2.z, shadowMeta2.w)) {
        // SV_Depth is mandatory on every path of the arb variant, so the raster
        // value is written before leaving.
#if KH_ARB_DEPTH
        khaODepth = i.pos.z;
#endif

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
    // footprint) is therefore tested unconditionally - four loads, every
    // distance, no gate - and the march handles the true behind-the-ridge cases
    // beyond the min-distance gate.
    float khtClear = 2.0e9f;   // 2e9 = lane inactive (distinct from no-data 1e9).

    if (thmParams.w >= 0.5f) {
        float khtHe = KhThmHeight(i.wpos.xz);
        if (khtHe > -1.0e5f) khtClear = i.wpos.y - khtHe;

        if (fragZ >= thmMeta.w) {
            // True ridges carry far more relief than 1.5 cells; only sub-cell
            // bumps lose their (false) vote.
            float khtMc = KhThmClearance(fxParams0.xyz, i.wpos) + 1.5f * thmParams.z;
            if (khtMc < khtClear) khtClear = khtMc;
        }

        if (khtClear < 1.5e9f && khtClear < -thmMeta.z) occ = true;
    }

#if KH_ARB_DEPTH
    {
        float khaD = i.pos.w;
        // It could never have shown a ramped write of ours sitting under a
        // clamped engine write at the same pixel.
        float khaNzN = abs(fxMeta.x);
        float khaNzD = (fxMeta.x < 0.0f) ? (1.0f / max(i.pos.w, 1.0e-8f))
                                         : khaD;
        bool  khaNzArm = (khaNzN > 0.0f && khaNzD < khaNzN);

        if (thmParams.w >= 0.5f && khtClear < 1.0e8f) {
            // Distance-proportional LOD margin (round 7): 0.06 is the
            // far-arbiter relative offset; fxParams1.z remains the absolute
            // cap.
            float khaCh = abs(fxParams1.w);
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
            // The only self-consistent decision is one that does not depend on
            // where our own last write landed: a competitor within the
            // possibly-self band (gap <= 0.25 + tie) is judged at the fragment,
            // and the pull is the 0.25 m margin itself.
            const bool khaFixed = (snapCam.w >= 0.5f);
            const float khaFixM = 0.10f;
            float khaOff = 0.0f;

            if (!khaSc && khaSz < khaRefD + khaTie) {

                float khaCap = min(fxParams1.z, i.pos.w * 0.06f) *
                    saturate((khaCh - khtClear) / max(0.3f * khaCh, 1.0f));
                bool khaTerr = true;
                float khaTerrW = 1.0f;   // The ramped form of khaTerr.

                if (fxParams1.w > 0.0f) {
                    float3 khaDir = (i.wpos - khaRayO) / max(khaRayW, 1.0e-4f);   // The snapshot's ray.
                    float khaIdZ = (khaFixed && (khaRefD - khaSz) <= 0.25f + khaTie) ? khaRefD : khaSz;
                    float3 khaSp = khaRayO + khaDir * khaIdZ;
                    float khaSh = KhThmHeight(khaSp.xz);
                    float khaTol = max(0.15f, thmParams.z * 0.02f);
                    khaTerr = (khaSh > -1.0e5f) && ((khaSp.y - khaSh) <= khaTol);
                    float khaTd = khaSp.y - khaSh;
                    float khaTrw = max(fwidth(khaTd), khaTol);
                    float khaTe = khaTol + khaTrw;   // The ramp reaches up past the tolerance.
                    khaTerrW = (khaSh > -1.0e5f)
                             ? saturate((khaTe - khaTd) / max(khaTrw, 1.0e-4f))
                             : 0.0f;
                 }
                if (khaTerr || khaTerrW > 0.0f) {
                    khaOff = khaFixed ? min(khaCap, khaFixM)   // The margin alone.
                                      : min(khaCap, max(khaRefD - khaSz, 0.0f) + 0.25f);   // Gap in the snapshot frame.
                    khaOff *= khaTerrW;   // Ramps.
                }
            }

            if (khaOff > 0.0f) {
                khaD = max(khaD - khaOff, 0.05f);
            }
        }

        // i.pos.z is the rasterizer's own interpolated, viewport-mapped depth -
        // byte-exact with what the hardware would have written for this
        // fragment had we never declared SV_Depth. Exact pass-through - the
        // depth banding.
        {
        const float khaNdcE = (i.pos.z - depthParams.z) /
                              max(depthParams.w - depthParams.z, 1.0e-6f);
        float khaNdc = khaNdcE + (depthParams.y / max(khaD, 0.01f) -
                                  depthParams.y / max(i.pos.w, 0.01f));
        if (khFarSplit.w > 0.5f) {
            float khaF = khFarSplit.x + khFarSplit.y / max(khaD, 0.01f);
            if (khaF <= 1.0f) {
                khaNdc = khaF;
            } else if (khFarSplit.w >= 1.5f) {
                khaNdc = 1.0f;
            } else if (fkVetoMeta.x > 0.5f &&
                       KhFkVetoHit(fxParams0.xyz, i.wpos, khFarSplit.z)) {
                clip(-1.0f);
            }
        }
        khaODepth = clamp(depthParams.z + (depthParams.w - depthParams.z) * khaNdc,
                          depthParams.z, depthParams.w);
 
        if (khaNdc >= 1.0f) khaODepth = depthParams.w - 1.0e-4f;   // Sliver, saturated only.
        // fxMeta.x carries the near estimate (> 0 arms; every other solid-mesh
        // fill leaves it zero - effect meshes never compile this shader),
        // fxMeta.y the widened floor the routed draw's viewport opened. The
        // ramp has been inverted since and never fired where it was needed.
        if (khaNzArm) {
            khaODepth = fxMeta.y + (depthParams.z - fxMeta.y) *
                        saturate(khaNzD / khaNzN);
        }

        }   // End recomputed path.
    }
#endif

     if (occ) discard;

    int bm = (int)sizeAxes.w;

#if KH_TEXTURED
    // KH_TEXTURED: sample below the far contract + guard blocks (the section-4
    // checkpoint - the textured twin adds no return/discard above them),
    // cutout-clip, then build the mapped shading normal. The geometric normal
    // keeps owning the receive gating below - shadow behavior stays in parity
    // with the untextured twin.
    KhMatSurf khtxS = KhSampleMat(i.uv);

    // matParams0.y = alpha mode
    if (matParams0.y >= 0.5f && matParams0.y < 1.5f) clip(khtxS.alpha - matParams0.z);   // Cutout kill.
    // Opaque alpha contract: sampled alpha never reaches the blend on the
    // opaque and cutout modes - survivors draw at alpha 1. KH_MAT_BLEND: a
    // blend material is split at alpha 0.996 - texels at or above it are solid
    // and draw in the opaque still occludes itself; texels below it draw in the
    // translucent tail alpha kept and fed to the same 'a' line as the object
    // colour's alpha below, hardware-blended without a depth write.
    if (matParams0.y >= 1.5f) {   // KH_MAT_SPLIT_TEXEL: one verdict per texel, both parts. Twin
                                  // Edit.
        // KH_MAT_SPLIT_TOL: the verdict tolerates compression. BC3/BC7 alpha in
        // a block that also holds transparent texels lands an opaque texel at
        // ~0.93-0.98 - the rectangular bites, one block each. Solid is >= 0.9;
        // a designed glass (0.3-0.6) still blends. Twin edit.
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
        } else khtxN = khtn;   // Degenerate tangent: geometric normal.

    }
#endif
    float smf = 1.0f;

    // An unguarded reference kills ps_composite and the box never draws;
    // shipped exactly that, in the wrong chunk. Taken here, outside the
    // divergent N.L branch, for the same quad-op legality as the pair above.
    // PSComposite declares khStenP in both branches of that conditional; this
    // block sat only in the #else, so the ARB variant never declared khBiasN
    // while the call site below the #endif used it. The bias slope wants the
    // facet, not the vertex normal.
    float3 khFacetN = cross(ddx(i.wpos), ddy(i.wpos));
    float khFacetL = length(khFacetN);
    float3 khBiasN = normalize(i.nrm);
    if (khFacetL > 1.0e-12f) {
        khFacetN /= khFacetL;
        if (dot(khFacetN, khBiasN) < 0.0f) khFacetN = -khFacetN;

    }

#if KH_TEXTURED
    float3 khShN = khtxN;
#else
    float3 khShN = normalize(i.nrm);
#endif
    if (lighting0.x >= 0.5f && dot(khShN, lighting1.xyz) > 0.01f) {
        {
            if (maskMeta.x >= 0.5f) smf = ShadowBandFactor(i.wrel + sunOrigin.xyz);
            else                    smf = ShadowMapFactor(i.wpos);   // Yzw re-lettered (were zero).
        }

        // Placed at a statement boundary between two independent receive terms,
        // so no expression is split. PSMain's twin segment is well under budget
        // and is deliberately not split - the twins differ in whitespace only.
 
        smf = min(smf, SunShadowFactorSelf(i.wpos, i.wrel, khBiasN));
        if (maskMeta.w >= 0.5f) {
            float khStenU = KhStenUnit(i.pos.xy);
            // On PSMain it is the effect ID, so the fade is not applied there
            // and must not be 'restored' to match.
#if KH_ARB_DEPTH
            // The magnitude is the near either way and the body already reads
            // abs.
            if (abs(fxMeta.x) > 0.0f) {
                float khStenF = saturate((i.pos.w / max(abs(fxMeta.x), 1.0e-4f) - 1.0f)
                                         / max(KH_STEN_FADE - 1.0f, 1.0e-4f));

                // KH_VOL_MIRROR: inside the fade the stencil verdict now comes
                // from the mirror counting pass instead of a flat 1.0 - shadow
                // scales through the last metre instead of washing out.
                float khMirF = 1.0f;
                if (mirMeta.x >= 0.5f) {
                    khMirF = KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z);
                }
                khStenU = lerp(khMirF, khStenU, khStenF);
            }
#endif
            // The volume term above starts from a witness compare - the engine
            // depth at this pixel must be this fragment's - and a translucent
            // texel wrote no depth: the pixel holds whatever is behind the
            // glass, the witness fails, the 7x7 search finds no matching plane
            // and the fallback answers with the background's stencil, which
            // changes with the camera. A translucent texel = the blend
            // material's translucent part (matParams0.y 2) or a whole
            // translucent object on normal blend (the interpolated colour alpha
            // Below 0.999 at blend id 0). TWIN: PSMain and PSComposite.
            if (mirMeta.x >= 0.5f && mirMeta.x < 1.5f &&
                ((matParams0.y >= 1.5f && matParams0.y < 2.5f) ||
                 (i.icol.a < 0.999f && bm == 0))) {
                khStenU = KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z);
            }
            // After the near-collapse mirror lerp: the fade thins the final
            // verdict.
            float khStRf = KhSunRangeFade(i.wpos);
            smf *= 1.0f - (1.0f - khStenU) * khStRf;
        }
    }

    // Slices surviving this are painted by the engine over our pixels, not by
    // any term of ours. Twin edit: PSMain and PSComposite identical.

#if KH_TEXTURED
    khtxS.albedo *= i.icol.rgb;   // The object colour tints the albedo lane only (KH_INSTANCING:
                                  // The interpolant).
#if KH_USER_MAT
    float3 lc = KhUserShade(khtxS, i.wpos, khtxN, smf);
#else
    float3 lc = KhApplyPBR(khtxS, i.wpos, khtxN, smf);
#endif
#else
    float3 lc = ApplyLighting(i.icol.rgb, i.wpos, i.nrm, smf);
#endif

    if (fogParams.w >= 0.5f || hazePars.w >= 0.5f || fogEngine.w >= 0.5f) {
        float distM = i.pos.w;
        float hgt = i.wpos.y;
        float camY = fogColor.w;
        float trans = 1.0f;
        float khaFbLay = fogSkyCol.w;
        bool  khaFbOn  = fogBelow.y >= 0.5f && camY < khaFbLay;
        float khaFbA   = distM;   // Path above the layer.
        float khaFbB   = 0.0f;   // Path below it.
        float khaFbRef = camY;   // The height reference.
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
                float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
                trans = exp(-distM * dens * 0.0153f);
            }
        }

        // The second extinction: the engine's distance haze, which multiplies
        // into this same transmittance and is armed whether or not fog is.
        // fogSkyCol.w carries the engine fog-layer altitude.
        trans *= KhHazeT(distM, hgt, camY, fogSkyCol.w);
        float khaAR = trans;
        float khaBt = khaFbOn ? exp(-khaFbB * fogBelow.x) : 1.0f;
        trans *= khaBt;
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
        if (khaFbOn && fogUw.w >= 0.5f) {
            float khaUwY = (hgt - camY) / max(distM, 1.0e-4f);
            float khaUwG;
            if (khaUwY < 0.0f) {
                float khaUwU = khaUwY + 1.0f;
                khaUwG = khaUwU * khaUwU * (fogUwGrad.y - fogUwGrad.x) + fogUwGrad.x;
            } else {
                khaUwG = khaUwY * (fogUwGrad.z - fogUwGrad.y) + fogUwGrad.y;
            }
            float khaWp = khaBt * (1.0f - khaAR);   // The PSC_FogColor weight.
            float khaWs = 1.0f - khaBt;   // The sky-colour weight.
            fog_target = (fog_target * khaWp + fogUw.rgb * khaUwG * khaWs) /
                         max(khaWp + khaWs, 1.0e-5f);
        }

        // At full optical depth our mesh paints exactly fog_target and nothing
        // else, so a silhouette can only exist if fog_target differs from what
        // the engine leaves on the pixels around it. Deliberately placed after
        // the fogSky gradient resolves, so it paints the target actually handed
        // to the lerp, not a stand-in.

        lc = lerp(fog_target, lc, trans);
    }
#if KH_TEXTURED
    float a = i.icol.a * khtxS.alpha * SolidMask(i.wpos);
#else
    float a = i.icol.a * SolidMask(i.wpos);
#endif
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
     if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);

    // Simple transparency per the spec is a display- space mix, so: sample the
    // pre-mesh scene capture at this pixel, blend in Reinhard space, invert,
    // write opaque - the destination can never dominate again.
    if (blendCtl.x >= 0.5f) {
        float khb_a = (sceneZ > blendCtl.y) ? 1.0f : a;
        float3 scn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float3 ts = scn / (1.0f + scn);
        float3 tl = lc / (1.0f + lc);
        float3 tm = lerp(ts, tl, khb_a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // Cap ~HDR 255.
    }

    return float4(lc, a);
}
