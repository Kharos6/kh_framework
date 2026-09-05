// composite2.hlsl - joined after composite.hlsl into the composite unit by C++
// (no #include). Any edit changes the unit's shader cache key.

struct VSOutC { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    float3 wrel : TEXCOORD4;   // Anchor-relative position (at VSOut).
    float4 icol : TEXCOORD5;   // The object colour interpolant (at VSOut).
    nointerpolation float4 iobj0 : TEXCOORD7;   // Per-object lanes (at VSOut).
    nointerpolation float4 iobj1 : TEXCOORD8;
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // World tangent + handedness.
    nointerpolation uint matIx : TEXCOORD6;   // Material table entry (at VSOut).
#endif
};

// Wrappers over KhVsCore, the per-object one over the CB lanes, the instanced
// one over the stream. The composite fills never write stenVol2.z non-zero.
// Twin: VSMain / VSMainInst in the static unit.
VSOutC VSComposite(VSIn i)
{
    VSOutC o;
    float3 khvR0, khvR1, khvR2;
    KhObjRows(khvR0, khvR1, khvR2);
    KhVsCore(i.pos, i.nrm, centerSize.xyz, centerRel.xyz, centerRel.w, sizeAxes.xyz,
             khvR0, khvR1, khvR2, o.pos, o.wpos, o.wrel, o.nrm);
    o.icol = color;
    KhObjLanesCb(o.iobj0, o.iobj1);   // KH_OBJBUF: the CB's per-object lanes.
#if KH_TEXTURED
    o.uv = i.uv;
    // Tangents are covariant (transform like positions, not normals): per-axis
    // scale then the object rotation, renormalized. The handedness sign rides
    // untouched in w.
    o.tanw = float4(normalize(KhRotateR(i.tan.xyz * sizeAxes.xyz, khvR0, khvR1, khvR2)), i.tan.w);
    o.matIx = (uint)matCtl.x;   // KH_MAT_TABLE: the draw's entry.
#endif
    return o;
}

// Twin of VSMainInst (static unit) - the record at the lane's slot.
VSOutC VSCompositeInst(VSIn i, VSInst n)
{
    VSOutC o;
    KhObjRec r = khObjs[n.islot];
    precise float3 khvRel = r.pos.xyz - khPass.xyz;
    KhVsCore(i.pos, i.nrm, r.pos.xyz, khvRel, khPass.w, r.size.xyz,
             r.rot0.xyz, r.rot1.xyz, r.rot2.xyz, o.pos, o.wpos, o.wrel, o.nrm);
    o.icol = float4(r.col.rgb, n.ilane.y);
    KhObjLanesRec(r, n.ilane.x, o.iobj0, o.iobj1);
#if KH_TEXTURED
    o.uv = i.uv;
    o.tanw = float4(normalize(KhRotateR(i.tan.xyz * r.size.xyz, r.rot0.xyz, r.rot1.xyz, r.rot2.xyz)), i.tan.w);
    o.matIx = (matCtl.z >= 0.5f) ? (uint)matCtl.x : (uint)(n.ilane.z + matCtl.y);   // KH_MAT_TABLE.
#endif
    return o;
}

#if KH_ARB_DEPTH
float4 PSComposite(VSOutC i, out float khaODepth : SV_Depth) : SV_Target
#else
float4 PSComposite(VSOutC i) : SV_Target
#endif
{
    KhObjLoad(i.iobj0, i.iobj1);   // KH_OBJBUF: the per-object lanes, per draw or per instance.
    if (khObjDither != 0.0f) {
        float khlD = frac(52.9829189f * frac(dot(i.pos.xy, float2(0.06711056f, 0.00583715f))));
        if (khObjDither > 0.0f) { if (khlD >= khObjDither) discard; }
        else if (khlD < -khObjDither) discard;
    }
    ClipEdgeSliver(i.wpos, i.nrm);   // Degenerate edge-on fragments (fireflies).
    ClipOwnNear(i.pos.w);   // Our own near plane. Twin call.
    if (khObjFarVis < 0.5f && depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f) > 1.0f) discard;
    if (khObjFarVis < 0.5f && khObjCut > 0.0f && i.pos.w > khObjCut) discard;
    int2 px = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float rawS = GuardSceneRaw(px);
    bool sceneClear = (rawS <= 0.000001f || rawS >= 0.999999f);
    float sceneZ = sceneClear ? 1.0e9f : KhSceneMeters(rawS);
    float fragZ = i.pos.w;
    bool occ = fragZ > sceneZ * (1.0f + fxParams1.y) + fxParams1.x;

    // The fragment's own clearance (its height above the terrain at its own
    // footprint) is tested unconditionally - four loads, every distance, no
    // gate; the march handles the true behind-the-ridge cases beyond the
    // min-distance gate.
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
        float khaNzN = abs(fxMeta.x);
        float khaNzD = (fxMeta.x < 0.0f) ? (1.0f / max(i.pos.w, 1.0e-8f))
                                         : khaD;
        bool  khaNzArm = (khaNzN > 0.0f && khaNzD < khaNzN);

        if (thmParams.w >= 0.5f && khtClear < 1.0e8f) {
            // Distance-proportional LOD margin: 0.06 is the far-arbiter
            // relative offset; fxParams1.z remains the absolute cap.
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
            // The decision must not depend on where our own last write landed:
            // a competitor within the possibly-self band (gap <= 0.25 + tie) is
            // judged at the fragment, and the pull is the 0.25 m margin itself.
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
        // byte-exact with what the hardware would have written had we never
        // declared SV_Depth. Exact pass-through.
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
        // fxMeta.y the widened floor the routed draw's viewport opened.
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
    // Textured: sample below the far contract + guard blocks (the textured twin
    // adds no return/discard above them), cutout-clip, then build the mapped
    // shading normal. The geometric normal keeps owning the receive gating
    // below.
    KhMatLoad(i.matIx);   // KH_MAT_TABLE: the lanes below read from the entry.
    KhMatSurf khtxS = KhSampleMat(i.uv);

    if (matParams0.y >= 0.5f && matParams0.y < 1.5f) clip(khtxS.alpha - matParams0.z);   // Cutout kill.
    // Opaque alpha contract: sampled alpha never reaches the blend on the
    // opaque and cutout modes - survivors draw at alpha 1. A blend material is
    // split at the texel: solid texels draw in the opaque part and still
    // occlude; texels below it draw in the translucent tail, alpha kept and
    // hardware-blended without a depth write.
    if (matParams0.y >= 1.5f) {   // One verdict per texel, both parts. Twin edit.
        // The verdict tolerates compression: BC3/BC7 alpha in a block that also
        // holds transparent texels lands an opaque texel at ~0.93-0.98. Solid
        // is >= 0.9; a designed glass (0.3-0.6) still blends. Twin edit.
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

    // Taken outside the divergent N.L branch for quad-op legality, and declared
    // in both branches of the KH_ARB_DEPTH conditional (the call site below the
    // #endif reads it). The bias slope wants the facet, not the vertex normal.
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
            else                    smf = ShadowMapFactor(i.wpos);
        }

        // A pixel the received term already darkens to 0 cannot get darker -
        // min(0, x) = 0 - so the self ladder (the costliest term in this
        // shader) is not consulted for it. A plain if, so fxc's gradient
        // hoisting applies.
        if (smf > 0.0f) smf = min(smf, SunShadowFactorSelf(i.wpos, i.wrel, khBiasN));
        if (maskMeta.w >= 0.5f) {
            float khStenU = KhStenUnit(i.pos.xy);
            // On PSMain fxMeta.x is the effect id, so the fade is not applied
            // there.
#if KH_ARB_DEPTH
            // The magnitude is the near either way.
            if (abs(fxMeta.x) > 0.0f) {
                float khStenF = saturate((i.pos.w / max(abs(fxMeta.x), 1.0e-4f) - 1.0f)
                                         / max(KH_STEN_FADE - 1.0f, 1.0e-4f));

                // Inside the fade the stencil verdict comes from the mirror
                // counting pass instead of a flat 1.0 - shadow scales through
                // the last metre instead of washing out.
                float khMirF = 1.0f;
                if (mirMeta.x >= 0.5f) {
                    khMirF = KhMirUnit(i.pos.xy, mirMeta.y, mirMeta.z);
                }
                khStenU = lerp(khMirF, khStenU, khStenF);
            }
#endif
            // The volume term starts from a witness compare - the engine depth
            // at this pixel must be this fragment's - and a translucent texel
            // wrote no depth, so the witness fails and the fallback answers
            // with the background's stencil. A translucent texel = the blend
            // material's translucent part or a whole translucent object on
            // normal blend. TWIN: PSMain and PSComposite.
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

    // Twin edit: PSMain and PSComposite identical from here to the shading
    // call.

#if KH_TEXTURED
    khtxS.albedo *= i.icol.rgb;   // The object colour tints the albedo lane only.
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
        if (fogEngine.w >= 0.5f && fogEngine.w < 1.5f && khObjFarVis < 0.5f)
            trans = saturate((fogEngine.y - khaFbA) * fogEngine.z);

        if (fogParams.w >= 0.5f) {
            if (fogEngine.w >= 0.5f) {
                // Below the layer they carry the export's above-path and layer
                // reference; a ray wholly below the layer (khaFbA == 0) takes
                // no height fog.
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

        // At full optical depth our mesh paints exactly fog_target, so a
        // silhouette can only exist if fog_target differs from what the engine
        // leaves around it. Placed after the fogSky gradient resolves, so it
        // paints the target actually handed to the lerp.

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

    // Simple transparency per the spec is a display-space mix: sample the
    // pre-mesh scene capture at this pixel, blend in Reinhard space, invert,
    // write opaque.
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
