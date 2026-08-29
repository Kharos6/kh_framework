// g_cb_hlsl - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
cbuffer CBObj : register(b0)
{
    float4 centerSize;   // xyz = world center (engine space), w = edge length
    // FP32 JITTER REBASE: xyz = center MINUS the pass's rebase origin (the
    // camera), double-subtracted on the CPU; w = 1 arms the rebased vertex
    // transform in VSMain/VSComposite, whose viewProj is then the REBASED
    // matrix. The zeroed default (w = 0) keeps every non-rebasing fill site
    // on the absolute path.
    float4 centerRel;
    float4 color;
    float4 fxParams0;   // effect parameters [0..3]
    float4 fxParams1;   // effect parameters [4..7]
    float4 fxMeta;   // x = effect id, y = time (s), z = screen width, w = screen height
    float4 depthParams;   // x = proj m22, y = proj m32, z = viewport MinDepth, w = MaxDepth
                         // (per-object: the flush's mode-Off solids carry the
                         // snapshot's encode pair, effects the live pair)
    float4 sizeAxes;   // xyz = mesh edge lengths (engine axes), w = blend mode id
    float4 localParams0;   // xyz = mask center (engine space), w = shape (0 sphere, 1 cube)
    float4 localParams1;   // x = falloff (normalized to mean radius), y = localized flag
    float4 localRadii;   // xyz = mask radii (engine axes)
    float4 bandParams;   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w = banded flag
    float4 lighting0;   // x = lit flag, y = THE RECEIVE-ARM CODE LANE (
    float4 shadowMeta2;   // x = far-visibility clamp flag (VSMain/VSComposite; PER OBJECT)
    float4 objRot0;   // engine-axes rotation rows (row-vector: world =
    float4 objRot1;   // center + local.x*R0 + local.y*R1 + local.z*R2).
    float4 objRot2;   // objRot0.w = 1 marks a filled matrix; 0 (the)
                          // zeroed-CB default) reads as identity, so fill
                          // sites that never rotate stay correct untouched.
    float4 dbgCtl;   // x = debug visual mode (setRenderDebug): 0 off
                          // (uniform branch, free), 1 coverage, 2 scene-
                          // vs-mesh depth, 3 blend layers. y: HISTORICAL
                          // (pyramid arm; retired the whole quarter-res
                          // family - permanently 0 now, no fill site
                          // remains); zw padding. x: mesh fill sites only -
                          // everywhere else the zeroed default keeps branches
                          // cold.
    float4 blendCtl;   // x = 1: normal-blend translucent mesh with the
                          // scene capture bound this inject - the packing
                          // composites in Reinhard space against t3 and
                          // writes opaque. 0 (zeroed default: capture miss,
                          // alpha 1, non-normal blends) = legacy hardware
                          // blend.
    float4 matParams0;   // KH_TEXTURED material head: map-bound flags
    float4 matParams1;   // alpha mode, cutoff, normal strength / base
    float4 matParams2;   // color + roughness / metalness, emissive
    float4 matParams3;   // intensity + channel routes (see the material)
                          // block after ApplyLighting). Zeroed and unread on
                          // every untextured fill site.
    float4 khFarSplit;   // FAR-KEEP SPLIT : xy = the FRAME pair
    float4 fuseMeta;
    float4 fuseStage[12];
    float4 fxParams2;   // effect parameters [8..11] (C++ twin)
                          // fx2 - the mirror contract; declared here in the
                          // append region, not beside fxParams0/1). PER
                          // OBJECT: mode 3 culls the pool to this mesh's
                          // nearest set.
    float4 dlCtl;   // x = mode (0 off, 1 camera-relative world)
                          // 2 view space), y = point count, z = spot count, w
                          // = global distance scale (cb10[2].x)
    float4 dlGlobal;   // xyz = global diffuse multiplier (cb10[3]); w = the
                          // script intensity, applied to the WHOLE DynLights
                          // sum (C++ twin dl_global)
    float4 dlView[3];   // view matrix COLUMNS (world->view rotation)
                          // for mode 2, captured WITH the light list
    float4 dlLights[192];   // 32 lights x 6 float4, cb11 layout verbatim:
                          // +0 position.xyz; +1 spot dir.xyz + cone
                          // cos-threshold.w; +2 diffuse.rgb + cone
                          // inv-width.w; +3 per-light ambient.rgb (no N.L) +
                          // cone exponent.w; +4 distance offset.x +
                          // attenuation a0/a1/a2.yzw; +5 range-fade start.x +
                          // inv-width.y
};

)HLSL" R"HLSL(cbuffer CBFrame : register(b1)
{
    row_major float4x4 viewProj;   // REBASED on the two mesh passes
                                      // (see centerRel); absolute elsewhere
    row_major float4x4 invViewProj;   // clip -> world (row-vector convention)
                                      // ALWAYS the absolute live-encode inverse
    float4 lighting1;   // xyz = unit vector TOWARD the sun/moon (engine axes), w = lighting valid flag
    float4 lighting2;   // rgb = light color (max-component normalized), w = shadow-map strength
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size (px)
    float4 shadowTiles[8];   // per-cascade atlas rect in UV: x0, y0, x1, y1
    float4 shadowMats[24];   // per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t]
    // View-paired bands: the engine's decoded receiver contract. bandView
    // rows are the view matrix COLUMNS (world->view, row-vector convention):
    // viewPos.k = dot(float4(wpos,1), bandView[slot*3+k]). bandMat rows are
    // PSC_ShadowmapMatrix rows: uvz.k = dot(float4(vp,1), row).
    float4 bandMat[24];
    float4 bandView[24];
    float4 bandBorder[8];   // x = near, y = far, z = fade, w = 0 invalid / 1+texIndex
    // Cast pass: draw the meshes depth-only INTO the engine's shadow atlas at
    // each cascade pass end, so the world receives THEIR shadows.
    float4 castMat[3];   // this cascade's sampling matrix rows
    float4 castView[3];   // matching view columns
    float4 maskMeta;   // x = view-paired band table valid
                             // y = engine-mask PAINT arm (mode 71); z =
                             // stencil-term paint selector (0 off, 1 = sten,
                             // 2 = the pre snapshot); w = stencil-term
                             // MULTIPLY arm (mode 75)
    float4 fogParams;   // x = fogValue, y = fogDecay, z = fogBase, w = enable
    float4 fogColor;   // rgb = fog color, w = camera altitude ASL
    row_major float4x4 sunVP;   // world -> private sun-depth clip (row-vector)
    float4 sunMeta;   // x = valid, y = map size (px), z = compare bias
    float4 localityMeta;
    float4 locality[32];   // [2i] = center.xyz (engine), [2i+1] = half extents.xyz
    float4 lightAmb;   // rgb = engine ambient color (HDR scene units) from the
                                // located lighting block (last-known lanes
                                // between confirmations), or (1,1,1) in the
                                // sub-second cold before the first lock; w =
                                // engine-mode flag
    float4 fogEngine;   // THE ENGINE'S OWN FOG TERMS (read from its)
                                // disassembly, values from the located
                                // block): x = density scale (lane 41), y =
                                // fog end distance (lane 48), z = inverse
                                // ramp range (lane 49), w = terms valid
    float4 fogSky;   // the sky CB's VIEW-ELEVATION GRADIENT control
    float4 fogSkyCol;   // the sky CB's fog base color (row 7; tracks)
    float4 hazePars;   // ENGINE DISTANCE HAZE, the sky CB's row 14
    float4 thmParams;   // x = origin world X, y = origin world Z
                          // (SQF y), z = cell size (m), w = enabled AND
                          // texture valid
    float4 thmMeta;   // x = width (cells), y = height (cells)
    float4 fkVetoMeta;   // x = OBB count (0 = veto dark), yzw unused
    float4 fkVeto[40];   // 8 OBBs x 5 float4: [center.xyz, 1+slot]
    row_major float4x4 stenReproj;   // the injection's view-proj (post-rebase)
    float4 stenReprojCam;   // xyz = its rebase camera; w: 1 rebased
    float4 stenVol;   // xy = the injection's viewport depth range for THIS
                       // epoch; zw = the volume copy's own dims
    float4 stenVol2;   // x = transport arm, y = footprint tolerance (0 = off)
                       // z  = ENGINE-VIEW ARM
    row_major float4x4 stenProj;   // the engine's projection (C++ twin)
                                   // sten_proj), paired with engRot at b2
    row_major float4x4 sunVP2;   // world -> HERO sun-depth clip
    float4 sunMeta2;   // x = valid, y = size, z = bias, w = half-diag
    // APPENDED AT THE TAIL like every existing offset in both blocks stays
    // byte-identical and 10 float4 keeps KH_CBFRAME_BYTES % 16 == 0.
    row_major float4x4 sunVP3;   // world -> MID-band sun-depth clip (t26)
    float4 sunMeta3;   // x = valid, y = size, z = bias, w = half-diag
    row_major float4x4 sunVP4;   // world -> OUTER-band sun-depth clip (t27)
    float4 sunMeta4;   // x = valid, y = size, z = bias, w = half-diag
    // (C++ twins mir_meta / sun_origin; appended at the tail, both blocks in
    // step). mirMeta: x = mirror mask valid, yz = mask dims. sunOrigin: the
    // anchor every sunVP* above is relative to - subtract from wpos before
    // transforming (zero = pre-world-absolute).
    float4 mirMeta;
    float4 sunOrigin;
    float4 fogBelow;
    // KH_FOG_UW_TARGET (C++ twins fog_uw / fog_uw_grad). Below the layer the
    // engine converges on a DIFFERENT colour: fogUw.rgb = cb0[7], the sky fog
    // colour, shaped by the cb0[17] elevation gradient in fogUwGrad.xyz.
    float4 fogUw;
    float4 fogUwGrad;
    row_major float4x4 snapVp;
    float4 snapMeta;
    float4 snapCam;
    // KH_SELF_PREFILTER (C++ twin sun_pf; appended at the tail, both blocks
    // in step). x/y/z = hero/mid/outer moment pyramid armed this frame (0 =
    // classic taps; mode 357 zeroes all three); w free.
    float4 sunPf;
    // Appended at the tail so every offset above is byte-identical to
    // KH_SUN_FAR_BAND: world -> FAR-band sun-depth clip (t32) + x = valid, y
    // = size, z = bias, w = half-diag.
    row_major float4x4 sunVP5;
    float4 sunMeta5;
    // Appended at the tail so every offset above is byte-identical to
    // KH_STEN_CYCLE_TAP - the cycle-basis view-projection of the frame the
    // consumed volume copy was captured in. All-zero = stand down (the
    // consumer's w gate refuses to the raster tap).
    row_major float4x4 stenCycVp;
    // KH_CAST_BIAS_CAP (26783). Appended at the tail: every existing offset
    // in both blocks stays byte-identical and 2 float4 keeps
    // KH_CBFRAME_BYTES % 16 == 0. THE WORLD CAST CARRIES ITS OWN COMPARE
    // BIAS, SEPARATE FROM sunMeta*.z. sunMeta*.z is read by BOTH verdict
    // chains; the self kernel floors on it and is healthy, so it may not
    // move. The cast chain needs a SMALLER, capped bias for the coarse
    // tiers and gets it here. x/y/z/w = hero / mid / outer / far, already
    // normalized by that tier's own d2v. ZERO = fall back to that tier's
    // sunMeta*.z, so any fill site that does not write these lanes (every
    // mesh fill: the cast chain has no mesh-side caller - the 264
    // precedent) behaves exactly as it did.
    float4 sunCastBias;
    // x = the UNION's cast bias (normalized by its own D); 0 = fall back to
    // sunMeta.z. yzw free.
    float4 sunCastBias2;
    // KH_CAST_REPROJ (26785). THIS FRAME's world->view, paired with the
    // FROZEN one the cast reconstructs through. The mask is painted at draw 0
    // from the depth resolved at draw 465 of the PREVIOUS frame - the only
    // depth that exists that early - so its shade is correct for the previous
    // frame's screen and is then read by THIS frame's lit pass at draws
    // 0..468, at pixels that no longer show the same world points. That is the
    // whole of the drift, and it is why any camera motion triggers it. Pairing
    // depth(N) with view(N) cannot fix it here: at draw 0 there IS no depth(N).
    // This matrix lets the pass solve the reprojection instead. Zero = stand
    // down (sunCastBias2.z is the arm).
    row_major float4x4 castViewN;
    // KH_DL_SHADOW (26822). Appended at the tail; C++ twins dls_meta /
    // dls_ctl / dls_face_slice / dls_spot_vp, same relative slots.
    // The cube faces carry NO matrix: their basis is a fixed axis permutation
    // and KhDlsShadow reconstructs face, uv and depth from the light-relative
    // position directly.
    float4 dlsMeta[8];     // xyz = light world position, w = far plane (0 = no map)
    float4 dlsCtl[8];      // x = spot flag, y = bias const (m), z = spot slice, w = bias slope
    // KH_DLS_FACE_FLAT (26826): slot * 6 + face. x = slice (-1 = none),
    // y = normal-offset arm (replicated per light). C++ twin dls_face_slice.
    float4 dlsFaceSlice[48];
    row_major float4x4 dlsSpotVP[8];
    // KH_DLS_RANGE (26879). Appended at the tail: every existing offset in
    // both blocks stays byte-identical and one float4 keeps KH_CBFRAME_BYTES
    // % 16 == 0. THE DYNAMIC-LIGHT SHADOWS OBEY THE SHADOW VIEW DISTANCE
    // like the sun's do. xyz = the pass camera (engine axes), w =
    // clamp(shadowVisibility, 8, 1000); 0 = no fade, which is the zeroed
    // default, so any fill site that never writes this lane behaves exactly
    // as it did (the 264 precedent). mirMeta.w could not carry it: the
    // world-receive pass fills mirMeta.xyz with its own mask and has no
    // sunOrigin. C++ twin dls_range, written by kh_dls_fill_cb.
    float4 dlsRange;
};
)HLSL" R"HLSL(
// THE CHUNK BOUNDARY IS HERE BECAUSE MSVC CAPS ONE STRING LITERAL TOKEN AT
// 16380 BYTES (C2026: "string too big, trailing characters truncated"). The
// rotation lands and nothing improves, because the rotation was never the
// whole transform. KhEngRotUsable took this chunk to 17568.
cbuffer CBEngView : register(b2)
{
    float4 engBlk[15];
};

// two more candidates, filled at DIFFERENT draws in the frame. Ledger at
// KhEngTry. b3/b4 were free; StateBackup's save range widened with them.
cbuffer CBEngView1 : register(b3)
{
    float4 engBlk1[15];
};

cbuffer CBEngView2 : register(b4)
{
    float4 engBlk2[15];
};

// mode 117 - the visible mesh leaves the frustum entirely: THE BOX
// DISAPPEARS.
// KH_RPDB_WORLD_CLAMP (26712): the receiver-plane depth gradient (khT_g /
// khsr_g) is clamped per texel; the clamp used to be 8 TEXELS at every tier,
// so the world distance a kernel tap may extrapolate the receiver plane grew
// 4x per tier - 8 mm at hero, 32 mm at mid, 128 mm at outer, metres at the
// union. On a receiver at grazing incidence the slope saturates the clamp,
// and the up-slope taps of the 3x3 kernel then predict the receiver far
// enough sun-ward to pass a thin occluder standing a few centimetres above
// it: the strap read part-lit the moment it left hero's window (269: on
// entering yellow), uniformly over the region (a fixed fraction of the
// taps), and only while the curvature damper let the gradient through
// (close range) - the third instance of a texel-priced world quantity
// (KH_SUN_HERO_BASE's normal offset, the 2/2 offset cap). The clamp is now
// priced in METRES at the hero calibration - 8 hero texels = 8 mm - so no
// tier may look further around an occluder than hero can, and the seams
// between tiers stop disagreeing about thin geometry. Hero itself is
// unchanged at the 4096 ladder (under 448 its 16 mm becomes 8). Coarse
// tiers lose gradient reach on steep slopes; the slope-scaled bias covers
// that to ~79 deg at mid, ~72 at outer, ~66 at the union (grazing light,
// where direct light is small and the soft compare halves the residual).
// Code 80 / mode 454 restores the texel-priced clamp (the 26711 form).
// TWIN: the tier body and the union tail carry the identical form.
#define KH_RPDB_GC_M 0.008f
// KH_SLOPE_WORLD (DEFAULT since 26714; minted opt-in at 26713 as code 82 /
// mode 456, now an alias of the default): the slope-scaled bias term
// (khT_slope * tan * texel) was the second texel-priced world quantity in
// the self kernel - 0.35 mm * tan at hero, 1.4 mm * tan at mid, tan ceiling
// 1e4 (code 17 / mode 232 restores 8). It is now priced at the HERO texel on
// every tier, so no tier biases a receiver further sun-ward than hero would
// at the same incidence and the hero->mid seam stops disagreeing about thin
// geometry (field: the last of the strap leak, 26713 read). The floor stays
// texel-priced: it covers the coarse map's own quantisation, which really is
// per texel. Code 83 / mode 457 restores the texel-priced term (the 26713
// form). Falsifier: acne on grazing receivers beyond hero - 457 is the
// stand-down and the floor is the knob, not this term.
#define KH_HERO_TEXEL_M 0.001f
#define KH_SLOPE_TW(khtw) ((lighting0.y >= 82.5f && lighting0.y < 83.5f) ? (khtw) : min((khtw), KH_HERO_TEXEL_M))
#define KH_RPDB_GC(khtw) ((lighting0.y >= 79.5f && lighting0.y < 80.5f) ? (8.0f * (khtw)) : min(8.0f * (khtw), KH_RPDB_GC_M))
#define KH_STEN_TOL_M 0.5f
// MODE 194 = THE CRISP PATH, AND IT EXISTS TO PROTECT MODE 131.
#define KH_STEN_TOL_W(khw) ((dbgCtl.w >= 8.5f && dbgCtl.w < 9.5f) ? 1.0e9f : (dbgCtl.w >= 7.5f && dbgCtl.w < 8.5f) ? clamp(KH_STEN_TOL_M * (depthParams.w - depthParams.z) * abs(depthParams.y) / max((khw) * (khw), 1.0e-6f), 4.0e-7f, 1.0e-2f) : stenVol2.y)

// svReprojPxMean has never exceeded ~125 px (0.065 NDC at 1920 wide) and
// svReprojPxMax reaches ~500 px (0.26 NDC) on wild frames, so a legitimate
// sub-frame lag has 2x headroom while a recoil-rotated basis puts the centre
// off screen entirely. THE BAR IS DELIBERATELY WIDE - stenVol2.w, filled at
// 0.5 NDC, which is a quarter of the screen.
)HLSL" R"HLSL(
// CHUNK BOUNDARY - MSVC caps one string literal token at 16380 bytes and
// KhEngTry took this one to 16616. THE ROWS ARE PASSED AS float4, NEVER AS A
// MATRIX. verify_hlsl_size.py caught it before the compiler did, which is the
// whole reason that check exists. ONE TEST THAT VALIDATES AND TRANSFORMS,
// OVER THREE CANDIDATES.
bool KhEngTry(float4 khe0, float4 khe1, float4 khe2, float4 khe3, float3 kheCam,
              float3 kheRel, float3 kheCtr, out float4 kheOut)
{
    kheOut = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float3 kheL = float3(dot(khe0.xyz, khe0.xyz), dot(khe1.xyz, khe1.xyz),
                         dot(khe2.xyz, khe2.xyz));
    if (max(max(abs(kheL.x - 1.0f), abs(kheL.y - 1.0f)),
            abs(kheL.z - 1.0f)) > 0.02f) return false;
    if (max(max(abs(khe0.w), abs(khe1.w)), abs(khe2.w)) > 1.0e-3f) return false;
    if (max(max(abs(khe3.x), abs(khe3.y)),
            max(abs(khe3.z), abs(khe3.w - 1.0f))) > 1.0e-3f) return false;

    float3 kheD = (centerSize.xyz - centerRel.xyz) - kheCam;
    if (dot(kheD, kheD) > 625.0f) return false;
    float3 kheR = kheRel + kheD;

    // Rows are the camera axes, so view space is three dot products - the
    // column-vector transpose, written out.
    kheOut = mul(float4(dot(khe0.xyz, kheR), dot(khe1.xyz, kheR),
                        dot(khe2.xyz, kheR), 1.0f), stenProj);

    if (stenVol2.w > 0.0f) {
        float3 kheC = kheCtr + kheD;
        float4 kheP = mul(float4(dot(khe0.xyz, kheC), dot(khe1.xyz, kheC),
                                 dot(khe2.xyz, kheC), 1.0f), stenProj);
        float4 kheH = mul(float4(kheCtr, 1.0f), viewProj);
        if (kheP.w < 1.0e-3f || kheH.w < 1.0e-3f) return false;
        float2 kheD = abs(kheP.xy / kheP.w - kheH.xy / kheH.w);
        if (max(kheD.x, kheD.y) > stenVol2.w) return false;
    }

    return true;
}

// Raw depth -> view-axis meters (the guard's linearization), SETTLED: the
// buffer's stored values are viewport-remapped, so the decode applies
// depthParams.z/.w before inverting the projection. Full autopsy in the notes
// doc.)
float KhSceneMeters(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

// Slab test in each OBB's local frame (the rows transform world offsets to
// local - the same row-vector convention as KhRotate).
bool KhFkVetoHit(float3 cam, float3 wpos, float selfId)
{
    float3 kfvV = wpos - cam;
    float kfvTf = length(kfvV);
    if (kfvTf < 1.0f) return false;
    float3 kfvRd = kfvV / kfvTf;
    int kfvN = (int)(fkVetoMeta.x + 0.5f);
    [loop] for (int kfvI = 0; kfvI < 8; ++kfvI) {
        if (kfvI >= kfvN) break;
        float4 kfvC = fkVeto[kfvI * 5 + 0];
        if (abs(kfvC.w - selfId) < 0.5f) continue;
        float3 kfvH = fkVeto[kfvI * 5 + 1].xyz;
        float3 kfvA0 = fkVeto[kfvI * 5 + 2].xyz;
        float3 kfvA1 = fkVeto[kfvI * 5 + 3].xyz;
        float3 kfvA2 = fkVeto[kfvI * 5 + 4].xyz;
        float3 kfvRo = cam - kfvC.xyz;
        float3 kfvO = float3(dot(kfvRo, kfvA0), dot(kfvRo, kfvA1), dot(kfvRo, kfvA2));
        float3 kfvD = float3(dot(kfvRd, kfvA0), dot(kfvRd, kfvA1), dot(kfvRd, kfvA2));
        float3 kfvSd = float3(kfvD.x >= 0.0f ? 1.0f : -1.0f,
                              kfvD.y >= 0.0f ? 1.0f : -1.0f,
                              kfvD.z >= 0.0f ? 1.0f : -1.0f);
        float3 kfvInv = kfvSd / max(abs(kfvD), 1.0e-6f);
        float3 kfvT0 = (-kfvH - kfvO) * kfvInv;
        float3 kfvT1 = ( kfvH - kfvO) * kfvInv;
        float3 kfvMn = min(kfvT0, kfvT1);
        float3 kfvMx = max(kfvT0, kfvT1);
        float kfvIn  = max(max(kfvMn.x, kfvMn.y), kfvMn.z);
        float kfvOut = min(min(kfvMx.x, kfvMx.y), kfvMx.z);
        if (kfvIn <= kfvOut && kfvOut > 0.0f && kfvIn < kfvTf - 0.5f) return true;
    }
    return false;
}
)HLSL" R"HLSL(
// Heightfield occlusion is marched CAMERA->FRAGMENT: per-pixel, temporally
// stable (the flicker has no input to feed on), altitude- and LOD-
// independent. Do not swap in a screen-space variant.
Texture2D<float> terrainHeightTex : register(t10);

// Bilinear terrain height (ASL meters) at engine-space (x, z). Returns -1e6
// outside the heightfield (= no occlusion evidence).
float KhThmHeight(float2 xz)
{
    float2 g = (xz - thmParams.xy) / max(thmParams.z, 1e-3f);
    if (g.x < 0.0f || g.y < 0.0f ||
        g.x > thmMeta.x - 1.001f || g.y > thmMeta.y - 1.001f) return -1.0e6f;
    int2 c0 = int2(g);
    float2 f = g - float2(c0);
    float h00 = terrainHeightTex.Load(int3(c0, 0));
    float h10 = terrainHeightTex.Load(int3(c0 + int2(1, 0), 0));
    float h01 = terrainHeightTex.Load(int3(c0 + int2(0, 1), 0));
    float h11 = terrainHeightTex.Load(int3(c0 + int2(1, 1), 0));
    return lerp(lerp(h00, h10, f.x), lerp(h01, h11, f.x), f.y);
}

float KhThmClearance(float3 cam, float3 wp)
{
    float mc = 1.0e9f;
    float len = distance(cam, wp);
    if (len < 1.0f) return mc;
    float skip = max(2.0f * thmParams.z, 25.0f);
    float t0 = saturate(skip / len);
    float t1 = 1.0f - saturate(skip / len);
    if (t1 <= t0) return mc;

    [loop] for (int s = 0; s < 96; ++s) {
        float t = lerp(t0, t1, ((float)s + 0.5f) / 96.0f);
        float3 p = lerp(cam, wp, t);
        float h = KhThmHeight(p.xz);
        if (h > -1.0e5f) mc = min(mc, p.y - h);
    }

    return mc;
}

// Shared object rotation (see objRot0): identity while unfilled, so only the
// mesh-transform fill sites carry the matrix.
float3 KhRotate(float3 p)
{
    if (objRot0.w < 0.5f) return p;
    return p.x * objRot0.xyz + p.y * objRot1.xyz + p.z * objRot2.xyz;
}

Texture2D<float> khSunDepth : register(t11);
Texture2D<float> khSunDepth2 : register(t25);   // HERO sun map (KH_SUN_HERO_MAP)
Texture2D<float> khSunDepth3 : register(t26);   // MID cascade band (KH_SUN_CASCADE)
Texture2D<float> khSunDepth4 : register(t27);   // OUTER cascade band (KH_SUN_CASCADE)
Texture2D<float> khSunDepth5 : register(t32);   // FAR band (KH_SUN_FAR_BAND; t28-t31 taken)
// KH_SELF_PREFILTER: per-band moment pyramids (mu, E[z^2]) at half resolution
// + mips; the self kernel samples them under minification.
Texture2D<float2> khSunPf2 : register(t29);   // hero
Texture2D<float2> khSunPf3 : register(t30);
Texture2D<float2> khSunPf4 : register(t31);   // outer
SamplerState khPfSamp : register(s1);   // linear-clamp (; gauges only)
// Linear filtering of RG32F is OPTIONAL support and this GPU point- samples
// it (the 358 blocky/jitter read; visual 31 G==B). Loads are guaranteed
// everywhere: bilinear by hand at two integer mips, lerped by the fractional
// lod. khpb_base = the pyramid mip-0 size in texels.
float2 KhPfMu(Texture2D<float2> khpb_t, float2 khpb_uv, float khpb_base, float khpb_lod)
{
    float khpb_l0 = floor(khpb_lod);
    float khpb_lw = khpb_lod - khpb_l0;
    int khpb_m0 = (int)khpb_l0;
    float2 khpb_r = float2(0.0f, 0.0f);
    [unroll] for (int khpb_i = 0; khpb_i < 2; ++khpb_i) {
        int khpb_m = khpb_m0 + khpb_i;
        float khpb_sz = max(khpb_base / exp2((float)khpb_m), 1.0f);
        float2 khpb_p = khpb_uv * khpb_sz - 0.5f;
        float2 khpb_f = frac(khpb_p);
        int2 khpb_i0 = int2(floor(khpb_p));
        int2 khpb_mx = int2((int)khpb_sz - 1, (int)khpb_sz - 1);
        int2 khpb_a = clamp(khpb_i0,             int2(0, 0), khpb_mx);
        int2 khpb_b = clamp(khpb_i0 + int2(1, 0), int2(0, 0), khpb_mx);
        int2 khpb_c = clamp(khpb_i0 + int2(0, 1), int2(0, 0), khpb_mx);
        int2 khpb_d = clamp(khpb_i0 + int2(1, 1), int2(0, 0), khpb_mx);
        float2 khpb_v = lerp(lerp(khpb_t.Load(int3(khpb_a, khpb_m)).xy,
                                  khpb_t.Load(int3(khpb_b, khpb_m)).xy, khpb_f.x),
                             lerp(khpb_t.Load(int3(khpb_c, khpb_m)).xy,
                                  khpb_t.Load(int3(khpb_d, khpb_m)).xy, khpb_f.x),
                             khpb_f.y);
        khpb_r = (khpb_i == 0) ? khpb_v : lerp(khpb_r, khpb_v, khpb_lw);
    }
    return khpb_r;
}
Texture2D<float4> khShadowMask : register(t20);
// Mode 74 paints pre on its own so a wrong ratio can never be confused with a
// copy that did not land. THE PRE-RESOLVE SNAPSHOT, and why the ratio is the
// whole feature.
Texture2D<float4> khShadowPre : register(t21);
// THE POST SNAPSHOT, and why one snapshot was never enough. It survived with
// the soldier walked out of range, which is what proved it was never a
// stencil term.
Texture2D<float4> khShadowPost : register(t22);

#define KH_PRIME_V   0.99607843f
#define KH_PRIME_EPS 0.00196078f

int2 KhMaskPx(float2 khsp_xy)
{
    return clamp(int2(khsp_xy), int2(0, 0),
                 int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}

// GUARD THE DENOMINATOR, DO NOT CLAMP IT. The cascade residual the whole
// feature was built to discard came straight back.
float KhStenRatio(float khsp_post, float khsp_pre)
{
    if (khsp_pre <= 0.02f) return 1.0f;
    return saturate(khsp_post / khsp_pre);
}

float KhStenRatioSub(float khsp_post, float khsp_pre)
{
    return 1.0f - saturate(khsp_pre - khsp_post);
}

)HLSL" R"HLSL(
// Under rotation they do not, and the mesh shader was Loading the mask at its
// OWN raster position, so at every silhouette edge a strip of the mesh read
// the verdict belonging to the background behind it. THE REGISTRATION FIX.
// THE FAULT.
float2 KhStenSel(float3 khsp_w, float2 khsp_raster)
{
    if (stenReprojCam.w < 0.5f) return khsp_raster;
    float4 khsp_c = mul(float4(khsp_w, 1.0f), stenReproj);
    if (khsp_c.w <= 1.0e-6f) return khsp_raster;
    float2 khsp_n = khsp_c.xy / khsp_c.w;
    float2 khsp_q = float2((khsp_n.x * 0.5f + 0.5f) * fxMeta.z,
                           (0.5f - khsp_n.y * 0.5f) * fxMeta.w);
    // Reprojecting by svReprojPxMean ~37 px walks a band of that width into
    // the hole and reads a verdict that was never ours. THE DISOCCLUSION
    // GUARD, and naming the halo is what earned it.
    if (stenReprojCam.w < 1.5f) return khsp_q;   // guard off (mode 108)
    // THE FOOTPRINT WITNESS.
    int2 khsp_qi = clamp(int2(khsp_q), int2(0, 0),
                         int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float khsp_pv = khShadowPre.Load(int3(khsp_qi, 0)).r;
    if (abs(khsp_pv - KH_PRIME_V) > KH_PRIME_EPS) return khsp_raster;
    return khsp_q;
}

float KhStenTerm(float2 khsp_xy)
{
    int2 khsp_p = KhMaskPx(khsp_xy);
    float khsp_post = khShadowPost.Load(int3(khsp_p, 0)).r;
    float khsp_pre  = khShadowPre.Load(int3(khsp_p, 0)).r;
    return KhStenRatio(khsp_post, khsp_pre);
}

// maskMeta.z: 1 = the isolated stencil term, 2 = the raw pre snapshot, 3 =
// the ratio's inputs side by side (R = post, G = pre, B = sten), 4 = the
// stencil term via the SUBTRACTIVE form (mode 79). Yellow = cancelling
// correctly, black = both inputs at zero, blue = both equal and non-zero.
float4 KhStenPaint4(float2 khsp_xy, float khsp_sel)
{
    int2 khsp_p = KhMaskPx(khsp_xy);
    // Under 76 our box's own cast shadow comes back; under every other
    // selector it cannot.
    float khsp_post = khsp_sel >= 4.5f ? khShadowMask.Load(int3(khsp_p, 0)).r
                                       : khShadowPost.Load(int3(khsp_p, 0)).r;
    float khsp_pre  = khShadowPre.Load(int3(khsp_p, 0)).r;
    if (khsp_sel >= 4.5f) {
        float khsp_l = KhStenRatio(khsp_post, khsp_pre);
        return float4(khsp_l, khsp_l, khsp_l, 1.0f);
    }
    if (khsp_sel >= 3.5f) {
        float khsp_b = KhStenRatioSub(khsp_post, khsp_pre);
        return float4(khsp_b, khsp_b, khsp_b, 1.0f);
    }
    float khsp_s = KhStenRatio(khsp_post, khsp_pre);
    if (khsp_sel >= 2.5f) return float4(khsp_post, khsp_pre, khsp_s, 1.0f);
    if (khsp_sel >= 1.5f) return float4(khsp_pre, khsp_pre, khsp_pre, 1.0f);
    return float4(khsp_s, khsp_s, khsp_s, 1.0f);
}

)HLSL" R"HLSL(
Texture2D<float> khVolDepth : register(t23);
Texture2D<uint2> khVolSten  : register(t24);
// Same shadowed semantics as KhVolShadowed's default arm (count != 0).
Texture2D<uint2> khMirSten  : register(t28);
float KhMirUnit(float2 khmu_px, float khmu_w, float khmu_h)
{
    int2 khmu_p = int2(clamp(khmu_px.x, 0.0f, khmu_w - 1.0f),
                       clamp(khmu_px.y, 0.0f, khmu_h - 1.0f));
    return (khMirSten.Load(int3(khmu_p, 0)).g != 0u) ? 0.0f : 1.0f;
}
// KH_DLS_WORLD_SELFDEPTH / _INFRONT / _VOLZ WERE REMOVED HERE (26849).
//
// Three helpers lived here between 26841 and 26845 - KhMirCovered, KhMirInFront
// and KhMirZPair - all trying to answer "is one of our meshes in front of the
// world surface at this pixel" by reading the mirror target that the vmir
// prepass fills. None of them worked, and the reason was never the arithmetic:
//
//   26841  read the STENCIL plane, which the prepass clears to 0 and never
//          writes. Inert on every pixel.
//   26842  read the DEPTH plane and inverted it with projection[2][2]/[3][2].
//          Wrong terms: the engine renders in depth PARTITIONS and the
//          projection at the resolve is not the one the mask was drawn under.
//   26844  carried the prepass's own matrix and compared in its depth space,
//          which is partition-proof - but compared raw device depths with a
//          fixed epsilon far larger than the values it had to separate.
//   26845  compared in metres, and mode 540 showed the world side reading
//          zero.
//
// The wall behind all four is VSMirror: it does not write the clip depth of the
// matrix it is handed, it OVERWRITES z with its own near plane
// (l22 = f/(f-0.05); z = l22*w - 0.05*l22, f from the engine's b2 block), and
// does so CONDITIONALLY on four tests against engBlk. That plane is another
// feature's private, frame-varying convention. It is not something a second
// consumer can read safely, and the four builds spent proving that are the
// reason KH_DLSW_MASK renders its own.
//
// KhMirUnit above is untouched - it has its own consumers and its own meaning.
//
// Receiver-distance fade at the shadow view distance, the engine-standard
// semantics: the FRAGMENT's own distance from the camera (sunOrigin IS the
// pass camera whenever mirMeta.w is armed - the fill guarantees the pairing)
// thins every cast and self term across the last ~15% of
// clamp(shadowVisibility, 8, 1000) and reaches zero before the per-caster
// eligibility cliff can pop.
float KhSunRangeFade(float3 khrf_p)
{
    if (mirMeta.w < 0.5f) return 1.0f;
    float khrf_d = length(khrf_p - sunOrigin.xyz);
    // The mirror (g_sun_range <- the game's shadowVisibility config) is
    // healthy - the CURVE was the author. New curve hugs the eligibility
    // cliff: full until 94%, gone at 99.5% (the 0.5% guard still completes
    // before the per-caster cliff at R, preserving the anti-pop contract
    // shipped for).
    return 1.0f - ((lighting0.y >= 53.5f && lighting0.y < 54.5f)
                   ? smoothstep(0.85f * mirMeta.w, 0.98f * mirMeta.w, khrf_d)
                   : smoothstep(0.94f * mirMeta.w, 0.995f * mirMeta.w, khrf_d));
}

int2 KhVolPx(float2 khvp_xy)
{
    return clamp(int2(khvp_xy), int2(0, 0),
                 int2((int)stenVol.z - 1, (int)stenVol.w - 1));
}

// The stencil COUNT at a texel. X24_TYPELESS_G8_UINT puts stencil in.g.
uint KhVolCount(int2 khvc_p)
{
    return khVolSten.Load(int3(khvc_p, 0)).g;
}

// No CB layout change. THE LOOKUP MODE, CARRIED IN stenReprojCam.w.
int KhVolMode()
{
    return (stenReprojCam.w >= 0.5f) ? 0 : (int)(0.5f - stenReprojCam.w);
}

// Ledger at

// In third person the camera is outside every volume and the wrap never fires
// - which is why this has never once been reproduced outside first person. It
// was never the missing evidence - it was never run. THE COUNT IS SIGNED, AND
// THAT IS THE FIRST-PERSON "GETS BIGGER". Counting is IncWrap on front faces
// and DecWrap on back faces.
bool KhVolShadowed(uint khvd_c, int khvd_m)
{
    if (khvd_m == 2 || khvd_m == 4 || khvd_m == 6) return khvd_c != 0u && khvd_c < 128u;
    return khvd_c != 0u;
}

// They are the two ENDS of a segment, and the answer is the FIRST texel along
// it that is ours: correct footprint at the minimum possible displacement.
float KhVolZ(float3 khvz_w)
{
    if (stenVol.y <= stenVol.x) return 0.0f;
    float4 khvz_c = mul(float4(khvz_w, 1.0f), stenReproj);
    if (khvz_c.w <= 1.0e-6f) return 0.0f;
    return stenVol.x + saturate(khvz_c.z / khvz_c.w) * (stenVol.y - stenVol.x);
}

// Kept as mode 130; NOT on the default path.
// The default form is STRAIGHT-LINE on purpose: no [unroll], break, continue
// or early return - X4575 fires on divergent flow downstream of ddx/ddy
// (ShadowBandFactor carries the same discipline).

float KhVolSoft(float2 khf_r, float khf_z, float khf_gx, float khf_gy, float khf_tol, int khf_m)
{
    int2  khf_p = KhVolPx(khf_r);
    if (abs(khVolDepth.Load(int3(khf_p, 0)) - khf_z) <= khf_tol)
        return KhVolShadowed(KhVolCount(khf_p), khf_m) ? 0.0f : 1.0f;

    float khf_s = 0.0f;
    float khf_w = 0.0f;

    for (int khf_j = -3; khf_j <= 3; ++khf_j) {
        for (int khf_i = -3; khf_i <= 3; ++khf_i) {
            int2  khf_q = KhVolPx(khf_r + float2(khf_i, khf_j));
            float khf_e = khf_z + khf_gx * khf_i + khf_gy * khf_j;
            float khf_d = abs(khVolDepth.Load(int3(khf_q, 0)) - khf_e);
            // soft membership, not a threshold: no hard per-pixel choice, so
            // no interlacing. Falls smoothly to 0 as a tap stops being our
            // surface.
            float khf_k = saturate(1.0f - khf_d / max(khf_tol * 3.0f, 1.0e-9f));
            khf_k *= khf_k;
            khf_s += khf_k * (KhVolShadowed(KhVolCount(khf_q), khf_m) ? 0.0f : 1.0f);
            khf_w += khf_k;
        }
    }

    if (khf_w < 1.0e-4f)
        return KhVolShadowed(KhVolCount(khf_p), khf_m) ? 0.0f : 1.0f;   // nothing was us
    return khf_s / khf_w;
}

)HLSL" R"HLSL(   // KH_MESH_OWNER_PREPASS helpers (shared header; see the block's own comment)
// MOVED into the shared header g_cb_hlsl - put this in g_hlsl_static, which
// the composite and effect units never see; every composite compile failed on
// X3004 'KhOwnerRejects' for two builds (compFailStreak 8/8) and the
// injection fell back to PSMain.
Texture2DMS<uint> khOwnMs : register(t33);
Texture2D<uint>   khOwnSs : register(t34);
uint KhOwnerPack(float khop_z, float khop_id)
{
    uint khop_q = (uint)(saturate(khop_z) * 1048575.0f + 0.5f);
    return (khop_q << 12) | ((uint)khop_id & 4095u);
}
bool KhOwnerRejects(float2 khot_px, float khot_z, float khot_id, float khot_samp)
{
    int2  khot_p  = int2(khot_px);
    uint  khot_me = (uint)khot_id & 4095u;
    float khot_q  = floor(saturate(khot_z) * 1048575.0f + 0.5f);
    float khot_dir = (depthParams.w < depthParams.z) ? -1.0f : 1.0f;   // toward-far positive
    int   khot_n  = (int)khot_samp;
    [loop] for (int khot_s = 0; khot_s < khot_n; ++khot_s) {
        uint khot_o = (khot_n > 1) ? khOwnMs.Load(khot_p, khot_s)
                                   : khOwnSs.Load(int3(khot_p, 0));
        if (khot_o == 0u) return false;
        if ((khot_o & 4095u) == khot_me) return false;
        if (((float)(khot_o >> 12) - khot_q) * khot_dir > 1.0f) return false;   // owner farther: keep
    }
    return true;
}
int2 KhVolWalk(float2 khw_r, float2 khw_j, float khw_z,
               float khw_gx, float khw_gy, float khw_tol)
{
    float2 khw_d = khw_j - khw_r;
    int2   khw_hit = KhVolPx(khw_j);   // walk fails -> the reprojected texel
    int    khw_done = 0;

    for (int khw_k = 0; khw_k <= 8; ++khw_k) {
        float2 khw_o = khw_d * ((float)khw_k * 0.125f);
        int2   khw_p = KhVolPx(khw_r + khw_o);
        // Anchored at the reproj end: at khw_o == khw_d this is khw_z
        // exactly.
        float  khw_e = khw_z + khw_gx * (khw_o.x - khw_d.x)
                             + khw_gy * (khw_o.y - khw_d.y);

        if (khw_done == 0 && abs(khVolDepth.Load(int3(khw_p, 0)) - khw_e) <= khw_tol) {
            khw_hit = khw_p;
            khw_done = 1;
        }
    }

    return khw_hit;
}

float4 KhVolSel(float3 khvs_w, float2 khvs_raster)
{
    if (stenVol.y <= stenVol.x) return float4(khvs_raster, -1.0f, 0.0f);
    float4 khvs_c = mul(float4(khvs_w, 1.0f), stenReproj);
    if (khvs_c.w <= 1.0e-6f) return float4(khvs_raster, -1.0f, 0.0f);
    float2 khvs_n = khvs_c.xy / khvs_c.w;
    float khvs_z = stenVol.x + saturate(khvs_c.z / khvs_c.w) * (stenVol.y - stenVol.x);
    return float4((khvs_n.x * 0.5f + 0.5f) * stenVol.z,
                  (0.5f - khvs_n.y * 0.5f) * stenVol.w,
                  khvs_z, 1.0f);
}

// |copied depth - reconstructed window z|, or -1 where nothing was
// reprojected.
float KhVolResidual(float4 khvr_s)
{
    if (khvr_s.w < 0.5f) return -1.0f;
    return abs(khVolDepth.Load(int3(KhVolPx(khvr_s.xy), 0)) - khvr_s.z);
}

)HLSL" R"HLSL(
// WHAT A MAGENTA READING MEANS: != 0 is the wrong test, the count needs a
// signed reading (or the engine resolve's own comparison replicated), and
// this transport must not become the default until it does. THE FOOTPRINT
// TEST FALLS BACK TO THE RASTER POSITION - never to a clamped texel 0, never
// to a neutral 1.0.
float KhVolTerm(float3 khvt_w, float2 khvt_raster, float2 khvt_g)
{
    float4 khvt_s = KhVolSel(khvt_w, khvt_raster);
    int    khvt_m = KhVolMode();
    int2   khvt_p;

    if (khvt_s.w < 0.5f || stenVol2.y <= 0.0f) {
        khvt_p = KhVolPx(khvt_raster);   // no witness: behaviour
    } else if (khvt_m == 0) {
        khvt_p = KhVolPx(khvt_s.xy);   // mode 126: pure reprojection
        if (abs(khVolDepth.Load(int3(khvt_p, 0)) - khvt_s.z) > stenVol2.y)
            khvt_p = KhVolPx(khvt_raster);
    } else if (khvt_m == 5) {
        return KhVolSoft(khvt_raster, khvt_s.z, khvt_g.x, khvt_g.y, stenVol2.y, khvt_m);
    } else {
        khvt_p = KhVolWalk(khvt_raster, khvt_s.xy, khvt_s.z,
                           khvt_g.x, khvt_g.y, stenVol2.y);
    }

    return KhVolShadowed(KhVolCount(khvt_p), khvt_m) ? 0.0f : 1.0f;
}

// khsu_g is the walk's screen-space depth gradient, taken by the CALLER
// outside the N.L branch (ddx/ddy are quad ops - see KhVolZ). The mask
// transport ignores it, so the one call shape still serves both.
// TWIN EDIT: the KhStenUnit call sites in PSMain (static unit) and
// PSComposite must stay byte-identical.
float KhStenUnit(float3 khsu_w, float2 khsu_raster, float2 khsu_g)
{
    if (stenVol2.x >= 0.5f) return KhVolTerm(khsu_w, khsu_raster, khsu_g);
    return KhStenTerm(KhStenSel(khsu_w, khsu_raster));
}

float4 KhVolPaint4(float3 khvq_w, float2 khvq_raster, float khvq_sel)
{
    float4 khvq_s = KhVolSel(khvq_w, khvq_raster);

    // Gray = the copy's depth plane at the raster texel (upper-half stretch,
    // where scene depth lives); ORANGE = counted (1..127); MAGENTA = wrapped
    // (>= 128).
    if (khvq_sel >= 9.5f && khvq_sel < 10.5f) {
        int2  khvq_dp = KhVolPx(khvq_raster);
        float khvq_dz = khVolDepth.Load(int3(khvq_dp, 0));
        uint  khvq_dc = KhVolCount(khvq_dp);
        float khvq_dg = saturate((khvq_dz - 0.5f) * 2.0f);
        if (khvq_dc >= 128u) return float4(1.0f, khvq_dg, 1.0f, 1.0f);
        if (khvq_dc != 0u)   return float4(1.0f, 0.35f + khvq_dg * 0.4f, 0.0f, 1.0f);
        return float4(khvq_dg, khvq_dg, khvq_dg, 1.0f);
    }

    if (khvq_sel >= 6.5f && khvq_sel < 7.5f) {
        float khvq_d = KhVolResidual(khvq_s);
        if (khvq_d <  0.0f)    return float4(0.5f, 0.5f, 0.5f, 1.0f);
        if (khvq_d <= 1.0e-6f) return float4(0.0f, 0.0f, 0.0f, 1.0f);
        if (khvq_d <= 1.0e-5f) return float4(0.0f, 0.0f, 0.5f, 1.0f);
        if (khvq_d <= 1.0e-4f) return float4(0.0f, 0.0f, 1.0f, 1.0f);
        if (khvq_d <= 1.0e-3f) return float4(0.0f, 1.0f, 1.0f, 1.0f);
        if (khvq_d <= 1.0e-2f) return float4(0.0f, 1.0f, 0.0f, 1.0f);
        if (khvq_d <= 1.0e-1f) return float4(1.0f, 1.0f, 0.0f, 1.0f);
        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    }

    // Sel 8 (mode 113) keeps the OLD unguarded behaviour on purpose: 110
    // against 113 is a one-variable A/B that shows exactly what the footprint
    // witness removes, and it is the only view in which the disocclusion band
    // is directly visible.
    int2 khvq_p = KhVolPx(khvq_s.xy);
    bool khvq_fell = false;

    if (khvq_sel < 7.5f && khvq_s.w >= 0.5f && stenVol2.y > 0.0f) {
        float khvq_dz = abs(khVolDepth.Load(int3(khvq_p, 0)) - khvq_s.z);

        if (khvq_dz > stenVol2.y) {
            khvq_p = KhVolPx(khvq_raster);
            khvq_fell = true;
        }
    }

    if (khvq_sel >= 8.5f) {
        int2 khvq_pr = KhVolPx(khvq_s.xy);
        int2 khvq_pw = KhVolPx(khvq_raster);
        bool khvq_sr = KhVolCount(khvq_pr) != 0u;
        bool khvq_sw = KhVolCount(khvq_pw) != 0u;
        bool khvq_g = false;

        if (khvq_s.w >= 0.5f && stenVol2.y > 0.0f) {
            khvq_g = abs(khVolDepth.Load(int3(khvq_pr, 0)) - khvq_s.z) > stenVol2.y;
        }

        if (khvq_sr == khvq_sw) {
            float khvq_b = khvq_sr ? 1.0f : 0.0f;
            return float4(khvq_b, khvq_b, khvq_b, 1.0f);
        }

        if (khvq_sr) return khvq_g ? float4(1.0f, 0.5f, 0.0f, 1.0f)   // orange
                                   : float4(0.45f, 0.0f, 0.0f, 1.0f);   // dark red
        return khvq_g ? float4(0.0f, 1.0f, 1.0f, 1.0f)   // cyan
                      : float4(0.0f, 0.0f, 0.45f, 1.0f);   // dark blue
    }

    uint khvq_c = KhVolCount(khvq_p);
    if (khvq_c >= 128u) return float4(1.0f, 0.0f, 1.0f, 1.0f);   // WRAPPED

    if (khvq_fell) {
        return khvq_c != 0u ? float4(1.0f, 0.5f, 0.0f, 1.0f)
                            : float4(0.35f, 0.2f, 0.0f, 1.0f);
    }

    if (khvq_c == 0u) return float4(0.0f, 0.0f, 0.0f, 1.0f);
    return float4(0.0f, saturate((float)khvq_c * 0.125f), 1.0f, 1.0f);
}

float4 KhStenPaintU(float3 khpu_w, float2 khpu_raster, float khpu_sel)
{
    if (khpu_sel >= 5.5f) return KhVolPaint4(khpu_w, khpu_raster, khpu_sel);
    return KhStenPaint4(KhStenSel(khpu_w, khpu_raster), khpu_sel);
}

)HLSL" R"HLSL(   // Raw occlusion (0 lit.. 1 occluded), pre-strength. Bilinear 4-tap PCF:
// one-texel-soft edges, and the acne band averages instead of flipping.
// KH_CAST_BIAS_CAP (26783): the union tail now applies its own capped bias
// inline, so this helper has no caller left. Body kept - fxc drops an
// unreferenced function and the resolver harness gates on UNRESOLVED calls,
// not unused ones - so that a revert is a one-line change.
float z_bias(float z) { return z - sunMeta.z; }

// ONE bilinear compare and ONE 5-tap soft compare for all five maps: the
// texture and its size are parameters (per-map twins retired; fxc resolves
// resource parameters at inlining). lighting0.y 22 (mode 245, fire CB)
// restores the hard single tap. Band maps never contain the ground, so a
// receiver never self-compares here: widening cannot acne, no slope term.
float KhSunBilinT(Texture2D<float> khcb_m, float khcb_sz, float2 uv, float z)
{
    float2 tx = uv * khcb_sz - 0.5f;
    float2 f = frac(tx);
    int2 p0 = int2(tx);
    float o00 = (z > khcb_m.Load(int3(p0 + int2(0, 0), 0))) ? 1.0f : 0.0f;
    float o10 = (z > khcb_m.Load(int3(p0 + int2(1, 0), 0))) ? 1.0f : 0.0f;
    float o01 = (z > khcb_m.Load(int3(p0 + int2(0, 1), 0))) ? 1.0f : 0.0f;
    float o11 = (z > khcb_m.Load(int3(p0 + int2(1, 1), 0))) ? 1.0f : 0.0f;
    return lerp(lerp(o00, o10, f.x), lerp(o01, o11, f.x), f.y);
}

float KhSunSoftT(Texture2D<float> khcs_m, float khcs_sz, float2 uv, float z)
{
    if (lighting0.y >= 21.5f && lighting0.y < 22.5f) return KhSunBilinT(khcs_m, khcs_sz, uv, z);
    float khcs_o = 0.75f / max(khcs_sz, 1.0f);
    return (KhSunBilinT(khcs_m, khcs_sz, uv, z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2( khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(-khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f,  khcs_o), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f, -khcs_o), z)) * 0.2f;
}

// Union-map forms keep their names: the cast tail, the probes and PSMaskCast
// read them.
float SunShadowCompareBilin(float2 uv, float z) { return KhSunBilinT(khSunDepth, sunMeta.y, uv, z); }
float SunShadowCompareSoft(float2 uv, float z)  { return KhSunSoftT(khSunDepth, sunMeta.y, uv, z); }

)HLSL" R"HLSL(
// The cross-fade now spans the outer 40% of every window - hero 80 cm, mid
// 3.2 m, outer 12.8 m - one helper, all nine blend sites (cast chain, self
// kernel, contact carries). KhJw shares this curve: continuity holds.
// KH_FADE_TO_GUARD (26717): the fade ends at the window's 0.998 uv guard,
// not 0.98. The 0.98 end left a DEAD ZONE - the last 0.9% of every window
// (4 cm of hero) where a finer tier's carry exists with weight exactly 0,
// so the coarser tier answered alone and no hold ramp expressed in that
// weight could reach it (the seam stripe's last residual, handoff 6.4).
// With the end at the guard the hold's 0.10 ramp covers ~1.6 cm of hero
// instead of ~9. The symmetric tail lengthens by the same 0.9% at near-
// zero weight. The kernel's OOB margin at the 0.998 guard is 8.2 texels
// while KhSelfTapT reaches up to 9 (spread 8 + bilinear 1, no high-side
// clamp): under 462 alone a one-texel out-of-bounds load (reads 0 =
// occluded) is possible at >= 16 texels/px minification; at the default
// the 0.98 end zeroes that band, so it is a boundary, not a defect.
// REVERTED at 26718 (operator: the shared fade shape must stay equal at
// every tier boundary; a shorter fade reads as a snap). Default = the
// 0.98 end again; code 88 / mode 462 = the guard end, OPT-IN; code 87 /
// mode 461 is an alias of the default. Codes 55 / 62 keep their own
// end points.
float KhTbW(float khtw_e)
{
    return (lighting0.y >= 54.5f && lighting0.y < 55.5f)
         ? 1.0f - smoothstep(0.90f, 0.99f, khtw_e)
         : (lighting0.y >= 61.5f && lighting0.y < 62.5f)
         ? 1.0f - smoothstep(0.60f, 0.98f, khtw_e)
         : (lighting0.y >= 87.5f && lighting0.y < 88.5f)
         ? 1.0f - smoothstep(0.75f, 0.998f, khtw_e)   // 462: fade to the guard
         : 1.0f - smoothstep(0.75f, 0.98f, khtw_e);
}
// Code 73 (mode 443) is the PARTNER PAINT: the partner alone inside the band,
// the arming read for which tier authors a fade (read with 257 and 0 at the
// same pose; 269 names the tier).
// KH_TIER_FADE_DIR - the HOLD is the DEFAULT since 26715 (minted opt-in as
// code 75 / mode 445, now an alias). A finer tier that carried a MORE
// shadowed verdict than the tier resolving the blend keeps it: the finer
// map resolved something the coarser one is on its ambiguity ramp over
// (field: a contact-scale fold at a seam - hero full shadow, mid a soft-
// compare partial - read as a lit stripe on entering hero's window, 269
// orange / 395 green = certified by hero, served by mid; 257 removed it,
// 445 removed most of it). The lighter direction still fades symmetrically
// (a lit finer verdict against a shadowed coarser one is the penumbra case
// the fade was built for). Continuity at the window edge holds: h -> 0 as
// w -> 0. The remaining band is KhTbW's dead zone (w == 0 for e in
// 0.98..0.998, the last 4 cm of hero) plus the hold's own ramp.
// REVERTED at 26718: the hold shortens the darker direction of the fade
// (46 cm of hero -> 11 cm at a 0.28 ramp, 5 cm at 0.10) and the operator
// judged an equal fade at every tier boundary worth more than the seam
// stripe (handoff 6.4 records the stripe as accepted). Default = the
// symmetric lerp (the 26714 form). Code 75 / mode 445 = hold, 0.28 ramp
// (its original meaning); code 85 / mode 459 = hold, 0.10 ramp; codes
// 84 / 86 (modes 458 / 460) are aliases - of the default and of 445
// respectively. Nine blend sites, both chains: this is the only body.
float KhTbBlend(float khtd_c, float khtd_f, float khtd_w)
{
    if (lighting0.y >= 72.5f && lighting0.y < 73.5f) return khtd_c;   // 443: partner alone
    const bool khtd_h28 = (lighting0.y >= 74.5f && lighting0.y < 75.5f) ||
                          (lighting0.y >= 85.5f && lighting0.y < 86.5f);   // 445 / 460: hold, 0.28
    const bool khtd_h10 = (lighting0.y >= 84.5f && lighting0.y < 85.5f);   // 459: hold, 0.10
    float khtd_h = (khtd_f > khtd_c && (khtd_h28 || khtd_h10))
                 ? smoothstep(0.0f, khtd_h10 ? 0.10f : 0.28f, khtd_w) : khtd_w;
    return lerp(khtd_c, khtd_f, khtd_h);
}
// KH_ABSENCE_WITNESS - RETIRED (26719; off by default since 26711). The
// union-map witness (KhUnionClear, 26694; tier-scoped at 26707) vetoed a
// tier's certification and the cast's lit-authority for an occluder the
// tier could not have RENDERED: a caster sun-ward of its near plane, which
// DepthClipEnable clipped out of the tier map. KH_SUN_PANCAKE (26710)
// removed that case - such a caster lands on z = 0 and still occludes - so
// every tier map is complete by construction and a clear texel IS absence.
// Body deleted, both khuw_off gates deleted, KhCastTier's khC_iD (the
// witness's depth scale) deleted. Modes 450 / 452 retired in the whitelist;
// codes 76 / 77 / 78 BURNED (never reassign). Restore = the 26718 upload's
// KhUnionClear, verbatim. Relabeling (6.1 of the 26718 handoff): the 26718
// default already evaluated khuw_off = true for every code but 77 / 78, so
// the default path is unchanged bit-for-bit.

// PAST the cut no finer tier contains the fragment, so khla_g never leaves 0
// and far's rejection cannot pass its own khla_g > 0.5 gate. Visual 32 paints
// certifier IDENTITY and never reads khcf.w, which is where this WEIGHT is
// carried - different quantities, no contradiction.
float KhJw(float khjw_e)
{
    return ((lighting0.y >= 59.5f && lighting0.y < 60.5f) ||
            (lighting0.y >= 54.5f && lighting0.y < 55.5f) ||
            (lighting0.y >= 25.5f && lighting0.y < 26.5f))
         ? 1.0f : KhTbW(khjw_e);
}

// ONE cast tier for the four camera-anchored bands (hero/mid/outer/far). The
// per-band twins differed only in map/matrix/meta; the hero copy also lacked
// the carried-verdict resolve, which is dead at tier one (khtb_occ is -1
// there). khC_done = a verdict was returned; false = fall through with the
// carry state updated in place. TWIN CONTRACT with KhSelfTier's ladder.
float KhCastTier(Texture2D<float> khC_map, float4x4 khC_vp, float4 khC_meta, float3 khC_r,
                 float khC_cb,   // KH_CAST_BIAS_CAP (26783): this tier's CAST bias; 0 = meta.z
                 bool khlf_on, bool khtb_on,
                 inout float khtb_occ, inout float khtb_w, out bool khC_done)
{
    khC_done = false;
    if (khC_meta.x >= 0.5f) {
        float4 khC_c = mul(float4(khC_r, 1.0f), khC_vp);   // KH_SUN_ANCHOR
        float2 khC_u = float2(0.5f + 0.5f * khC_c.x, 0.5f - 0.5f * khC_c.y);
        if (khC_u.x > 0.002f && khC_u.x < 0.998f &&
            khC_u.y > 0.002f && khC_u.y < 0.998f &&
            khC_c.z > 0.0f && khC_c.z < 1.0f) {
            // KH_CAST_BIAS_CAP (26783). khC_meta.z is TEXEL-PRICED and the tier
            // texels run 1 mm / 4 mm / 16 mm / 100 mm, so the compare bias ran
            // 2 mm / 8 mm / 32 mm / 200 mm across the ladder. On the SELF chain
            // that is covered by the receiver-normal offset, the hero-priced
            // slope term and the metre-clamped receiver-plane gradient. THE CAST
            // CHAIN HAS NONE OF THOSE - this line is its whole bias - so the far
            // tier pushed every world shadow 200 mm sun-ward, which is
            // bias / tan(elevation) of GROUND displacement: 0.2 m at 45 deg,
            // 0.55 m at 20, 1.13 m at 10. The 32 m outer/far handoff therefore
            // stepped the displacement 6x across a straight line in the sun
            // plane. khC_cb is the same quantity capped in METRES (the
            // KH_SLOPE_TW / KH_RPDB_GC idiom, fourth instance of a texel-priced
            // world quantity); zero falls back to the legacy lane.
            float khC_b = khC_cb > 0.0f ? khC_cb : khC_meta.z;
            float khC_o = KhSunSoftT(khC_map, khC_meta.y, khC_u, khC_c.z - khC_b);   // filtered
            if (khC_o > 0.0001f || !khlf_on) {   // lit authoritative (the tier map is complete: KH_ABSENCE_WITNESS retired); code 42 falls through
                if (khtb_occ >= 0.0f) { khC_done = true; return KhTbBlend(khC_o, khtb_occ, khtb_w); }
                float khC_e = max(abs(khC_u.x - 0.5f), abs(khC_u.y - 0.5f)) * 2.0f;
                float khC_w = khtb_on ? KhTbW(khC_e) : 1.0f;
                if (khC_w >= 0.9999f) { khC_done = true; return khC_o; }
                khtb_occ = khC_o; khtb_w = khC_w;   // carry into the next tier
            }
        }
    }
    return 0.0f;
}

float SunShadowOcclusion(float3 wpos)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    // lighting0.y 21 (mode 243, set only by the fire's cast fill) reverts to
    // the union-only compare. TWIN CONTRACT: the cast chain here and the self
    // chain carry the identical shape.
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    const bool khtb_on = !(lighting0.y >= 25.5f && lighting0.y < 26.5f);
    // The admission test is a HARD THRESHOLD on occlusion: one ulp below it
    // the band is skipped entirely - no return, no carry, no weight - and
    // the next tier answers at FULL strength.
    // LADDER: code 42 (mode 328) RE-ARMS fallthrough for A/B. Code 40 (mode
    // 295) is now an accepted ALIAS of the default and stays whitelisted -
    // the mode-64 precedent.
    const bool khlf_on = (lighting0.y >= 41.5f && lighting0.y < 42.5f);
    // KH_ABSENCE_WITNESS: retired (26719) - see the note above KhJw. TWIN:
    // the self chain dropped its gate in the same build.
    if (lighting0.y < 20.5f || lighting0.y >= 21.5f) {
        const float3 khc_r = wpos - sunOrigin.xyz;   // KH_SUN_ANCHOR
        bool khc_done;
        float khc_v;
        khc_v = KhCastTier(khSunDepth2, sunVP2, sunMeta2, khc_r, sunCastBias.x, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth3, sunVP3, sunMeta3, khc_r, sunCastBias.y, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth4, sunVP4, sunMeta4, khc_r, sunCastBias.z, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth5, sunVP5, sunMeta5, khc_r, sunCastBias.w, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
    }
)HLSL" R"HLSL(   // CHUNK BOUNDARY - SIXTH C2026 CATCH OF THE CAMPAIGN
    float4 c = mul(float4(wpos - sunOrigin.xyz, 1.0f), sunVP);   // ortho: w = 1 (KH_SUN_ANCHOR)
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    // KH_TIER_BLEND: a carried band verdict resolves against WHATEVER the
    // union answers - including its rejections (0 = lit).
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (c.z <= 0.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;

    // KH_CAST_BIAS_CAP (26783): the union's CAST bias, capped in metres like
    // the four tiers above. Zero falls back to sunMeta.z, which is what
    // z_bias has always applied and what every un-filled site still gets.
    const float khcu_b = sunCastBias2.x > 0.0f ? sunCastBias2.x : sunMeta.z;

    if (c.z >= 1.0f) return localityMeta.z >= 0.5f
                          ? SunShadowCompareSoft(uv, 1.0f - khcu_b) : 0.0f;   // filtered

    // KH_TIER_BLEND: the union answer resolves any carried band edge.
    float khtb_un = SunShadowCompareSoft(uv, c.z - khcu_b);   // filtered (z_bias, capped)
    return (khtb_occ >= 0.0f) ? KhTbBlend(khtb_un, khtb_occ, khtb_w) : khtb_un;
}

// Soft variant for the SELF term: five bilinear taps in a +/-0.75-texel
// diamond - a ~2.5-texel penumbra, 'very slightly smoothed' rather than the
// bilinear's hard 1-texel ramp. The world CAST keeps the single tap:
)HLSL" R"HLSL(   // its edges land on engine-lit ground where the engine's own shadows are
// It was the five-tap soft kernel the old offset/fade/escape self term
// called, and KH_SELF_RPDB replaced that term - so it had no caller left
// anywhere. SunShadowCompareBilin stays - the world CAST (SunShadowOcclusion
// / PSMaskCast) still uses it, and that path never compares a surface against
// itself so it cannot acne and needs none of this.
// ONE tap body for all five maps - the texture is the only thing that ever
// differed between the per-map twins, and fxc resolves a resource parameter
// at inlining, so this costs nothing and cannot drift.
float KhSelfTapT(Texture2D<float> khst_m, float2 khst_t, float2 khst_g, float khst_z, float khst_b, float khst_w, float2 khst_o)
{
    if (lighting0.y >= 30.5f && lighting0.y < 31.5f) {
        int2  khsth_p = int2(khst_t) + int2(round(khst_o));   // code-31 arm, rounded back
        float2 khsth_d = (float2(khsth_p) + 0.5f) - khst_t;
        float khsth_e = khst_z + khsth_d.x * khst_g.x + khsth_d.y * khst_g.y - khst_b;
        return (khsth_e > khst_m.Load(int3(khsth_p, 0))) ? 1.0f : 0.0f;
    }
    float2 khst_tc = khst_t + khst_o - 0.5f;   // fractional offsets land on the corners
    float2 khst_f0 = floor(khst_tc);
    float2 khst_fr = khst_tc - khst_f0;
    int2   khst_p0 = int2(khst_f0);
    float4 khst_c;
    [unroll] for (int khst_k = 0; khst_k < 4; ++khst_k) {
        int2 khst_q = max(khst_p0 + int2(khst_k & 1, khst_k >> 1), int2(0, 0));
        float2 khst_d = (float2(khst_q) + 0.5f) - khst_t;
        float khst_e = khst_z + khst_d.x * khst_g.x + khst_d.y * khst_g.y - khst_b;
        float khst_s = khst_m.Load(int3(khst_q, 0));
        khst_c[khst_k] = (lighting0.y >= 43.5f && lighting0.y < 44.5f)
                       ? ((khst_e > khst_s) ? 1.0f : 0.0f)
                       : saturate((khst_e - khst_s) / max(khst_w, 1.0e-9f) + 0.5f);
    }
    return lerp(lerp(khst_c.x, khst_c.y, khst_fr.x),
                lerp(khst_c.z, khst_c.w, khst_fr.x), khst_fr.y);
}
// the HERO-map twin (t25). Identical arithmetic, different texture.

)HLSL" R"HLSL(   // CHUNK BOUNDARY (C2026) - the shared self tier gets its own segment
// ONE self tier for the four camera-anchored bands. Derived from the mid
// tier, which carries every branch; the real per-tier differences are the
// parameters: map / moment pyramid / matrix / meta, the pf arm (far has no
// pyramid: 0), the code-61 fixed guard (mid 48, outer 192, 0 = no arm), the
// code-69 admission-bound guard (far only), the code-22 ring collapse (hero
// never had it), and the certifier id written to khcf. The two branches the
// hero copy lacked - the carried-verdict resolve and the jurisdiction
// rejection - are dead at tier one by construction (khtb_occ is -1 and
// khla_g is 0 there). khT_in / khT_cert are the outer tier's stamps for the
// far gate; the other tiers receive scratch. khT_done = a verdict was
// returned; false = fall through with the carry state updated in place.
// TWIN CONTRACT with KhCastTier's ladder: same tier order, same KhTbBlend
// carry shape.
float KhSelfTier(Texture2D<float> khT_map, Texture2D<float2> khT_pf, float4x4 khT_vp, float4 khT_meta,
                 float khT_pfArm, float khT_cg61, bool khT_cg69, bool khT_wArm, float khT_id,
                 float3 khwr, float3 n, float ndl, float khno_k, float khgs,
                 bool khcl, bool khcc_on, bool khcc_clr, bool khtb_on,
                 bool khlfs_off, bool khct_on,
                 inout float khtb_occ, inout float khtb_w, inout float khla_g, inout float khla_w,
                 inout float4 khcf, inout bool khT_in, inout bool khT_cert, out bool khT_done)
{
    khT_done = false;
    if (khT_meta.x >= 0.5f) {
        float khT_iR0 = length(float3(khT_vp[0].x, khT_vp[1].x, khT_vp[2].x));
        float khT_no = khno_k * 2.0f / (max(khT_meta.y, 1.0f) * max(khT_iR0, 1e-6f));
        float4 khT_c = mul(float4(khwr + n * khT_no, 1.0f), khT_vp);
        float2 khT_uv = float2(0.5f + 0.5f * khT_c.x, 0.5f - 0.5f * khT_c.y);

        if (khT_uv.x > 0.002f && khT_uv.x < 0.998f &&
            khT_uv.y > 0.002f && khT_uv.y < 0.998f &&
            khT_c.z > 0.0f && khT_c.z < 1.0f) {
            khT_in = true;   // outer: the far gate reads it
            float3 khT_cr = float3(khT_vp[0].x, khT_vp[1].x, khT_vp[2].x);
            float3 khT_cu = float3(khT_vp[0].y, khT_vp[1].y, khT_vp[2].y);
            float khT_iR = length(khT_cr);
            float khT_iD = length(float3(khT_vp[0].z, khT_vp[1].z, khT_vp[2].z));
            float khT_tw = 2.0f / (max(khT_meta.y, 1.0f) * max(khT_iR, 1e-6f));
            float khT_k = (khcl || (lighting0.y >= 31.5f && lighting0.y < 33.5f))
                        ? 0.0f : khT_tw * khT_iD / max(ndl, 0.02f)
                          * ((lighting0.y >= 38.5f && lighting0.y < 39.5f)
                             ? 1.0f : saturate(1.0f - 3.0f * length(fwidth(n))));   // damper DEFAULT (280's form); off under the 39 arm
            float khT_gc = KH_RPDB_GC(khT_tw) * khT_iD;   // KH_RPDB_WORLD_CLAMP (454 = texel-priced)
            float2 khT_g = clamp(
                float2( dot(n, khT_cr / max(khT_iR, 1e-9f)) * khT_k,
                       -dot(n, khT_cu / max(khT_iR, 1e-9f)) * khT_k),
                -khT_gc, khT_gc);
            float2 khT_t = khT_uv * khT_meta.y;
            float khT_ceil = (lighting0.y >= 16.5f && lighting0.y < 17.5f) ? 8.0f : 1.0e4f;
            float khT_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f), 1.0f, khT_ceil);
            float khT_slope = (lighting0.y >= 17.5f && lighting0.y < 18.5f) ? 3.0f
                            : (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 0.8f : 0.35f;
            float khT_fb = (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 1.25f : 1.0f;
            float khT_b = (lighting0.y >= 12.5f && lighting0.y < 13.5f)
                        ? khT_meta.z
                        : max(khT_meta.z * khT_fb,   // tier-proportional floor (code 37 reverts; at khsr_slope)
                              ((lighting0.y >= 36.5f && lighting0.y < 37.5f) ? 0.0f : 1.5f) * khT_tw * khT_iD)
                          + khT_slope * khT_tan * KH_SLOPE_TW(khT_tw) * khT_iD;   // KH_SLOPE_WORLD (457 = texel-priced)
            float khT_gs = (lighting0.y >= 47.5f && lighting0.y < 48.5f)
                        ? khgs : min(khgs, 4.0f * khT_tw);
            khT_b += khT_gs * khT_iD;
            float4 khTc_c = mul(float4(khwr, 1.0f), khT_vp);
            float2 khTc_t = float2(0.5f + 0.5f * khTc_c.x,
                                       0.5f - 0.5f * khTc_c.y) * khT_meta.y;
            float khTc_s = khT_map.Load(int3(int2(khTc_t), 0));
            if (khct_on && khTc_c.z > 0.0f && khTc_c.z < 1.0f &&
                khTc_c.z - khTc_s >
                    (3.0f + 1.5f * khT_tan) * khT_tw * khT_iD) {
                float khT_cres = 1.0f;   // distinct occluder provably above
                float khT_ce = max(abs(khT_uv.x - 0.5f), abs(khT_uv.y - 0.5f)) * 2.0f;
                float khT_cbw = khtb_on ? KhTbW(khT_ce) : 1.0f;
                if (khtb_occ >= 0.0f) { khT_done = true; return KhTbBlend(khT_cres, khtb_occ, khtb_w); }
                if (khT_cbw >= 0.9999f) { khT_done = true; return khT_cres; }
                khtb_occ = khT_cres; khtb_w = khT_cbw;
            } else {
            // lighting0.y 22 (mode 245) restores the collapse on the bands
            // for the A/B.
            float khT_w = (khT_wArm && (lighting0.y >= 21.5f && lighting0.y < 22.5f))
                        ? saturate(0.001f / max(khT_tw, 1e-6f)) : 1.0f;
            float2 khT_fw = fwidth(khT_t);
            float khT_sp = (khcl || (lighting0.y >= 27.5f && lighting0.y < 28.5f)
                            || (khgs > 4.0f * khT_tw && !(lighting0.y >= 51.5f && lighting0.y < 52.5f)))
                          ? 1.0f
                          : (lighting0.y >= 44.5f && lighting0.y < 45.5f)
                          ? float(clamp(int(round(0.5f * max(khT_fw.x, khT_fw.y))), 1, 8))
                          : clamp(0.5f * max(khT_fw.x, khT_fw.y), 1.0f, 8.0f);
            float khT_sw = max(2.0f * khT_gs, khT_tw) * khT_iD;   // clamped slack
            float khT_ctr = KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  0));
            float khT_rng = KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0, -1) * khT_sp)
                          + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  0) * khT_sp)
                          + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  0) * khT_sp)
                          + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  1) * khT_sp);
            float khT_cnt = 4.0f;
            if (!(lighting0.y >= 37.5f && lighting0.y < 38.5f)) {   // 3x3 DEFAULT again (field: close-range worse under the diamond; 38 arms the diamond; 36 now an alias)
                khT_rng += KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1, -1) * khT_sp)
                         + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1, -1) * khT_sp)
                         + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  1) * khT_sp)
                         + KhSelfTapT(khT_map, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  1) * khT_sp);
                khT_cnt = 8.0f;
            }
            float khT_res = (khT_ctr + khT_w * khT_rng) / (1.0f + khT_cnt * khT_w);
            // Centre texel of the map (the jurisdiction test reads it):
            // hoisted above the pf block in 26711 for an arm since wiped;
            // left here - same load, same value, no reason to move it back.
            float khT_js = khT_map.Load(int3(int2(khT_t), 0));
            if (khT_pfArm >= 0.5f && !khcl) {
                float khT_ft = max(khT_fw.x, khT_fw.y);
                // engagement covers the moire band (ft 1-2, where the old 1.5
                // floor left the worst aliasing unserved).
                float khT_pw = (khgs > 4.0f * khT_tw && !(lighting0.y >= 51.5f && lighting0.y < 52.5f))
                             ? 0.0f : smoothstep(1.0f, 2.0f, khT_ft);
                // lighting0.y 47 (mode 358): PF-EXCLUSIVE instrument arm.
                bool khT_px = (lighting0.y >= 46.5f && lighting0.y < 47.5f);
                if (khT_pw > 0.001f || khT_px) {
                    float khT_lod = log2(max(khT_ft * 0.5f, 1.0f));
                    // The filtered MEAN is the precision-sound channel: soft
                    // compare over the footprint depth ramp. Interiors cannot
                    // bleed - the filtered depth there IS the occluder.
                    float2 khT_mv = KhPfMu(khT_pf, khT_uv, khT_meta.y * 0.5f, khT_lod);
                    float khT_vd = (khT_c.z - khT_b) - khT_mv.x;
                    float khT_ww = max(khT_ft, 2.0f) * khT_tw * khT_iD;
                    float khT_var = khT_mv.y - khT_mv.x * khT_mv.x;
                    float khT_s2 = (lighting0.y >= 73.5f && lighting0.y < 74.5f)
                                 ? max(khT_var, max(2.5e-7f, khT_ww * khT_ww))   // 444: floor
                                 : (khT_var >= 2.5e-7f ? max(khT_var, khT_ww * khT_ww)
                                                          : khT_ww * khT_ww);   // ramp
                    float khT_vv = khT_vd <= 0.0f ? 1.0f
                                 : saturate(khT_s2 / (khT_s2 + khT_vd * khT_vd));
                    float khT_pfo = 1.0f - saturate((khT_vv - 0.4f) / 0.6f);
                    if (khT_px) khT_res = khT_pfo;
                    // honest content may CREATE standing - absent content
                    // reads pfo ~ 0 and still falls through. (26711's
                    // KH_PF_DEEP_GATE arm, code 79 / mode 453, stood here;
                    // it read null against the strap and was wiped at
                    // 26714. The strap was the texel-priced gradient clamp
                    // and slope bias, not the pyramid.)
                    else if ((khT_res > 0.0001f || khT_pfo > 0.2f) &&
                             !(khT_res > 0.8f && khT_pfo < 0.2f))
                        khT_res = lerp(khT_res, khT_pfo, khT_pw);
                }
            }
            if (khla_g > 0.5f && khT_res > 0.0001f &&
                (khT_c.z - khT_js) < khla_g * khT_iD &&
                (khtb_occ < 0.0f || (lighting0.y >= 54.5f && lighting0.y < 55.5f)) &&
                !(lighting0.y >= 50.5f && lighting0.y < 51.5f))
                khT_res *= 1.0f - khla_w;   // rejection at the certifier's edge weight
            // the radius is hoisted so the evidence test can ask about the
            // same number the latch is about to publish.
            float khT_cg = (khT_cg61 > 0.0f && (lighting0.y >= 60.5f && lighting0.y < 61.5f))
                         ? khT_cg61   // code 61: fixed guard (mid 48, outer 192)
                         : (khT_cg69 && (lighting0.y >= 68.5f && lighting0.y < 69.5f))
                         ? khT_meta.w   // 398: the admission bound (far)
                         : 6.0f * khT_meta.w;   // default = form
            bool khT_cok = (khcc_on   // the tier's own map certifies (KH_ABSENCE_WITNESS retired, 26719)
                         ? ((khcc_clr || khT_js < 1.0f) &&
                            (khT_c.z - khT_js) < khT_cg * khT_iD)
                         : (abs(khT_c.z - khT_js) < 3.0f * khT_b));
            if (khcf.x < 0.5f) khcf.x = khT_cok ? 3.0f
                                     : (khT_js >= 1.0f ? 1.0f : 2.0f);
            if (khT_cok && khcf.y < 0.5f) khcf.y = khT_id;
            khcf.z = khT_id;
            if (khT_cok) {
                // khla_g takes the MAX across certifiers while khla_w takes
                // the LAST certifier's value - coherent only while guards
                // are monotonic in tier; inserting a tier out of order
                // breaks this silently.
                khla_g = max(khla_g, khT_cg);
                khla_w = KhJw(max(abs(khT_uv.x - 0.5f), abs(khT_uv.y - 0.5f)) * 2.0f);
                khcf.w = khla_w;
                khT_cert = true;   // outer: the far gate reads it
            }
            if (khT_res > 0.0001f || khlfs_off) {   // lit falls through
                if (khtb_occ >= 0.0f) { khT_done = true; return KhTbBlend(khT_res, khtb_occ, khtb_w); }
                float khT_e = max(abs(khT_uv.x - 0.5f), abs(khT_uv.y - 0.5f)) * 2.0f;
                float khT_bw = khtb_on ? KhTbW(khT_e) : 1.0f;
                if (khT_bw >= 0.9999f) { khT_done = true; return khT_res; }
                khtb_occ = khT_res; khtb_w = khT_bw;   // carry onward
            }
            }
        }
    }
    return 0.0f;
}

)HLSL" R"HLSL(   // CHUNK BOUNDARY (C2026)
float SunShadowOcclusionSelfEx(float3 wpos, float3 wrel, float3 nrm,
                               out float4 khcf)
{
    // KH_CERT_PROBE forensic (full comment at KhSelfCertProbe). x = innermost
    // containing tier outcome, y = innermost certifier, z = serving tier, w =
    // khla_w. Set before every return.
    khcf = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (sunMeta.x < 0.5f) return 0.0f;
    float3 khwr = (lighting0.y >= 45.5f && lighting0.y < 46.5f)
                ? (wpos - sunOrigin.xyz) : wrel;
    const bool khcl = (lighting0.y >= 49.5f && lighting0.y < 50.5f);   // KH_SELF_CLASSIC (365/366)
    float khgs = (khcl || (lighting0.y >= 24.5f && lighting0.y < 25.5f))
               ? 0.0f : 0.5f * length(fwidth(khwr));
    // NORMAL-OFFSET SAMPLING (mode 259 / lighting0.y 27 reverts) - the second
    // half of the industry-standard pair: the SAMPLE POINT shifts one
    // texel-world along the geometric normal before projecting, so the lookup
    // lands beside the surface instead of on it and the terminator band (the
    // 239-visible edge-line locus) narrows to the offset's own width.
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    // Declared AFTER ndl on purpose: fxc requires textual declaration order
    // (X3004) and the sweep checks it.
    // Code 33 (mode 268) restores the 6/8 form wholesale; code 32 (mode 267)
    // keeps this default (its role is the gradient isolator).
    float khno_k = (lighting0.y >= 26.5f && lighting0.y < 27.5f)
                 ? 0.0f
                 : (lighting0.y >= 32.5f && lighting0.y < 33.5f)
                 ? min(1.0f + 6.0f * sqrt(saturate(1.0f - ndl * ndl))
                              / max(ndl, 0.15f), 8.0f)
                   * saturate(1.0f - 5.0f * length(fwidth(n)))
                 : min(1.0f + 2.0f * sqrt(saturate(1.0f - ndl * ndl))
                              / max(ndl, 0.15f), 2.0f)
                   * (khcl ? 1.0f : saturate(1.0f - 5.0f * length(fwidth(n))));
    if (ndl <= 0.0f && (lighting0.y >= 15.5f && lighting0.y < 16.5f)) return 0.0f;
    float khtb_occ = -1.0f;
    float khla_g = 0.0f;   // jurisdiction radius, metres (at the union block)
    float khla_w = 0.0f;   // KhJw protection weight
    const bool khcc_on = !(lighting0.y >= 65.5f && lighting0.y < 66.5f);
    // KH_CERT_CLEAR (mode 396 / lighting0.y 67 restores 's content-only
    // form). A CLEAR texel is evidence of ABSENCE, not absence of evidence:
    // the tier rasterized its whole admitted caster set and nothing landed on
    // this ray.
    const bool khcc_clr = !(lighting0.y >= 66.5f && lighting0.y < 67.5f);
    // KH_ABSENCE_WITNESS: retired (26719) - see the note above KhJw. TWIN:
    // the cast chain dropped its gate in the same build.
    float khtb_w = 0.0f;
    const bool khtb_on = !(lighting0.y >= 25.5f && lighting0.y < 26.5f);
    // the self chain's TWIN of KH_BAND_LIT_FALLTHROUGH (mode 296 =
    // lighting0.y 41, mesh fill only). Band depth windows are camera- local,
    // so a lit KERNEL verdict falls through to the union self path.
    const bool khlfs_off = (lighting0.y >= 40.5f && lighting0.y < 41.5f);
    // A re-arm needs margin > 2.5 and accepts losing sub-centimetre contacts.
    const bool khct_on = (lighting0.y >= 29.5f && lighting0.y < 30.5f);
    // HERO -> MID -> OUTER -> FAR -> union, one helper per tier. The outer
    // tier's in-window / certified stamps gate the far tier (KH_SUN_FAR_BAND).
    bool  khT_done = false;
    bool  khT_scr_in = false, khT_scr_cert = false;   // scratch for the non-outer tiers
    float khT_v;
    khT_v = KhSelfTier(khSunDepth2, khSunPf2, sunVP2, sunMeta2, sunPf.x, 0.0f,   false, false, 1.0f,
                       khwr, n, ndl, khno_k, khgs, khcl, khcc_on, khcc_clr, khtb_on, khlfs_off, khct_on,
                       khtb_occ, khtb_w, khla_g, khla_w, khcf, khT_scr_in, khT_scr_cert, khT_done);
    if (khT_done) return khT_v;
    khT_v = KhSelfTier(khSunDepth3, khSunPf3, sunVP3, sunMeta3, sunPf.y, 48.0f,  false, true,  2.0f,
                       khwr, n, ndl, khno_k, khgs, khcl, khcc_on, khcc_clr, khtb_on, khlfs_off, khct_on,
                       khtb_occ, khtb_w, khla_g, khla_w, khcf, khT_scr_in, khT_scr_cert, khT_done);
    if (khT_done) return khT_v;
    bool kh4_in = false;
    bool kh4_cert = false;
    khT_v = KhSelfTier(khSunDepth4, khSunPf4, sunVP4, sunMeta4, sunPf.z, 192.0f, false, true,  3.0f,
                       khwr, n, ndl, khno_k, khgs, khcl, khcc_on, khcc_clr, khtb_on, khlfs_off, khct_on,
                       khtb_occ, khtb_w, khla_g, khla_w, khcf, kh4_in, kh4_cert, khT_done);
    if (khT_done) return khT_v;
)HLSL" R"HLSL(   // KH_SUN_FAR_BAND self tier (own segment)
    // KH_FAR_SELF_GATE - RETIRED (26719). The far tier used to run only
    // when outer did not contain the fragment, or carried a verdict, or
    // certified it: the gate existed because an outer map could be
    // INCOMPLETE (a 30-50 m caster clipped out sun-ward of outer's near
    // plane read lit there, and the gate handed such a fragment to the
    // union). Under KH_SUN_PANCAKE (26710) outer's lit verdict is
    // trustworthy, so far now runs whenever it is reached - the code-65
    // form (mode 392, now an ALIAS of the default). Code 89 / mode 463 =
    // KH_FAR_SELF_GATE ON, the 26718 default verbatim (the restore arm);
    // code 71 / mode 401 (the harder cut without the kh4_cert clause) is
    // retired, code 71 BURNED. kh4_in / kh4_cert survive for the arm.
    // No moment pyramid at the FAR band - its texel is already the
    // minified scale the pyramids exist to reach (pf arm 0; khSunPf4 is a
    // placeholder the arm never reads).
    if (sunMeta5.x >= 0.5f &&
        (!(lighting0.y >= 88.5f && lighting0.y < 89.5f) ||   // 463: the 26718 gate
         !kh4_in || khtb_occ >= 0.0f || kh4_cert)) {
        khT_v = KhSelfTier(khSunDepth5, khSunPf4, sunVP5, sunMeta5, 0.0f,    0.0f,   true,  true,  4.0f,
                           khwr, n, ndl, khno_k, khgs, khcl, khcc_on, khcc_clr, khtb_on, khlfs_off, khct_on,
                           khtb_occ, khtb_w, khla_g, khla_w, khcf, khT_scr_in, khT_scr_cert, khT_done);
        if (khT_done) return khT_v;
    }
)HLSL" R"HLSL(   // KH_SELF_RPDB - receiver-plane depth bias; in C++.
    float khsr_iR0 = length(float3(sunVP[0].x, sunVP[1].x, sunVP[2].x));
    float khsr_no = khno_k * 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR0, 1e-6f));
    float4 khsr_c = mul(float4(khwr + n * khsr_no, 1.0f), sunVP);   // ortho: w = 1
    float2 khsr_uv = float2(0.5f + 0.5f * khsr_c.x, 0.5f - 0.5f * khsr_c.y);
    if (khsr_uv.x <= 0.002f || khsr_uv.x >= 0.998f ||
        khsr_uv.y <= 0.002f || khsr_uv.y >= 0.998f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (khsr_c.z <= 0.0f || khsr_c.z >= 1.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    // Sun-space basis straight out of sunVP: column 0 is r3/R, column 1 is
    // u3/R, column 2 is f/D. texelWorld = 2R / mapSize.
    float3 khsr_cr = float3(sunVP[0].x, sunVP[1].x, sunVP[2].x);
    float3 khsr_cu = float3(sunVP[0].y, sunVP[1].y, sunVP[2].y);
    float khsr_iR = length(khsr_cr);
    float khsr_iD = length(float3(sunVP[0].z, sunVP[1].z, sunVP[2].z));
    float khsr_tw = 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR, 1e-6f));
    // What ALSO changed: the offset no longer displaces samples up to 8
    // texels into foreign surfaces (the 2/2 cap ships with this), so the
    // gradient predicts the surface the tap actually lands on. Codes 32/33
    // zero the gradient (267 = capped offset WITHOUT gradient - the isolator;
    // 268 = the whole default back).
    float khsr_k = (khcl || (lighting0.y >= 31.5f && lighting0.y < 33.5f))
                 ? 0.0f : khsr_tw * khsr_iD / max(ndl, 0.02f)
                   * ((lighting0.y >= 38.5f && lighting0.y < 39.5f)
                      ? 1.0f : saturate(1.0f - 3.0f * length(fwidth(n))));   // (at khsr_slope)
    float khsr_gc = KH_RPDB_GC(khsr_tw) * khsr_iD;   // KH_RPDB_WORLD_CLAMP twin (454 = texel-priced)
    float2 khsr_g = clamp(
        float2( dot(n, khsr_cr / max(khsr_iR, 1e-9f)) * khsr_k,
               -dot(n, khsr_cu / max(khsr_iR, 1e-9f)) * khsr_k),
        -khsr_gc, khsr_gc);
    float2 khsr_t = khsr_uv * sunMeta.y;
    // 's KH_SELF_SLOPE_MAX capped an offset that MOVED the sample; this is a
    // pure DEPTH bias and the two have opposite cost curves. The map holds
    // our own FRONT faces, so a receiver compares against ITSELF and RPDB
    // cannot close it alone (shading vs geometric normal). 227 stores BACK
    // faces and the bias drops to quantisation.
    float khsr_ceil = (lighting0.y >= 16.5f && lighting0.y < 17.5f) ? 8.0f : 1.0e4f;
    // The tier-proportional floor stays default - 280 never disabled it, so
    // the ranked winner includes it. Computing it here from a normal made it
    // vary fragment to fragment, which was the noise.
    float khsr_slope = (lighting0.y >= 17.5f && lighting0.y < 18.5f) ? 3.0f
                     : (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 0.8f : 0.35f;
    float khsr_fb = (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 1.25f : 1.0f;
    float khsr_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f),
                           1.0f, khsr_ceil);
    float khsr_b = (lighting0.y >= 12.5f && lighting0.y < 13.5f)
                 ? sunMeta.z
                 : max(sunMeta.z * khsr_fb,   // tier-proportional floor (code 37 reverts)
                       ((lighting0.y >= 36.5f && lighting0.y < 37.5f) ? 0.0f : 1.5f) * khsr_tw * khsr_iD)
                   + khsr_slope * khsr_tan * KH_SLOPE_TW(khsr_tw) * khsr_iD;   // KH_SLOPE_WORLD twin (457 = texel-priced)
            float khsr_gs = (lighting0.y >= 47.5f && lighting0.y < 48.5f)
                        ? khgs : min(khgs, 4.0f * khsr_tw);
            khsr_b += khsr_gs * khsr_iD;
    // lighting0.y 20 (mode 238) restores the flat ring.
    float khsr_w = (lighting0.y >= 19.5f && lighting0.y < 20.5f)
                 ? 1.0f : saturate(0.001f / max(khsr_tw, 1e-6f));
    float2 khsr_fw = fwidth(khsr_t);
    float khsr_sp = (khcl || (lighting0.y >= 27.5f && lighting0.y < 28.5f)
                     || (khgs > 4.0f * khsr_tw && !(lighting0.y >= 51.5f && lighting0.y < 52.5f)))
                ? 1.0f
                : (lighting0.y >= 44.5f && lighting0.y < 45.5f)
                ? float(clamp(int(round(0.5f * max(khsr_fw.x, khsr_fw.y))), 1, 8))
                : clamp(0.5f * max(khsr_fw.x, khsr_fw.y), 1.0f, 8.0f);
    float khsr_sw = max(2.0f * khsr_gs, khsr_tw) * khsr_iD;   // clamped slack
    float khsr_ctr = KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  0));
    float khsr_rng = KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  0) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  0) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  1) * khsr_sp);
    // Camera-anchored fits only (sunMeta.x 2.0); code 23 (mode 246) restores
    // the hard edge.
    float khsr_fd = 1.0f;
    if (sunMeta.x >= 1.5f && !(lighting0.y >= 22.5f && lighting0.y < 23.5f)) {
        float khsr_e = max(abs(khsr_uv.x - 0.5f), abs(khsr_uv.y - 0.5f)) * 2.0f;
        khsr_fd = 1.0f - smoothstep(0.85f, 0.98f, khsr_e);
    }
    float khsr_res = (khsr_ctr + khsr_w * khsr_rng)
                   * (1.0f / (1.0f + 8.0f * khsr_w)) * khsr_fd;
    khcf.z = 5.0f;   // the union is the server if we reached here
    if (khla_g > 0.5f && khsr_res > 0.0001f &&
        (khsr_c.z - khSunDepth.Load(int3(int2(khsr_t), 0))) < khla_g * khsr_iD &&
        (khtb_occ < 0.0f || (lighting0.y >= 54.5f && lighting0.y < 55.5f)) &&
        !(lighting0.y >= 50.5f && lighting0.y < 51.5f))
        khsr_res *= 1.0f - khla_w;
    return (khtb_occ >= 0.0f) ? KhTbBlend(khsr_res, khtb_occ, khtb_w) : khsr_res;
}

// forwarder - no call site changes, no twin risk.
float SunShadowOcclusionSelf(float3 wpos, float3 wrel, float3 nrm)
{
    float4 khcf_d;
    return SunShadowOcclusionSelfEx(wpos, wrel, nrm, khcf_d);
}

float SunShadowFactorSelf(float3 wpos, float3 wrel, float3 nrm)
{
    return 1.0f - SunShadowOcclusionSelf(wpos, wrel, nrm) * saturate(sunMeta.w)
                * KhSunRangeFade(wpos);
}

)HLSL" R"HLSL(   // CHUNK BOUNDARY - visual 19 took this segment 719 B past the
// 16380-byte MSVC token cap (C2026). Splitting here costs nothing at runtime.
// The LAW still holds across the split: chunks concatenate, so SunSelfProbe's
// body still precedes its call sites in every assembly.
float4 SunSelfProbe(float3 wpos, float3 nrm)
{
    if (sunMeta.x < 0.5f) return float4(0.3f, 0.3f, 0.3f, 1.0f);   // no map
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float4 c = mul(float4(wpos - sunOrigin.xyz, 1.0f), sunVP);   // KH_SUN_ANCHOR
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    if (uv.x <= 0.002f || uv.x >= 0.998f ||
        uv.y <= 0.002f || uv.y >= 0.998f ||
        c.z <= 0.0f || c.z >= 1.0f) return float4(1.0f, 0.0f, 1.0f, 1.0f);   // MAGENTA
    float st = khSunDepth.Load(int3(int2(uv * sunMeta.y), 0));
    if (st >= 0.999f) return float4(1.0f, 0.0f, 0.0f, 1.0f);   // RED: cleared
    float iR = length(float3(sunVP[0].x, sunVP[1].x, sunVP[2].x));
    float iD = length(float3(sunVP[0].z, sunVP[1].z, sunVP[2].z));
    float tw = 2.0f / (max(sunMeta.y, 1.0f) * max(iR, 1e-6f));
    // the probe's BIAS tracks the term's (same formula, same ceiling arm) so
    // ORANGE/GREEN keeps meaning what it says. Its useful divergence is
    // STRUCTURAL - one tap, no N.L veto - not this constant.
    float qceil = (lighting0.y >= 16.5f && lighting0.y < 17.5f) ? 8.0f : 1.0e4f;
    float tanq = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f), 1.0f, qceil);
    // probe follows the term - the slope moved to the rasterizer.
    float qslope = (lighting0.y >= 17.5f && lighting0.y < 18.5f) ? 3.0f
                 : (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 0.8f : 0.35f;   // (probe follows the term)
    float qfb = (lighting0.y >= 38.5f && lighting0.y < 39.5f) ? 1.25f : 1.0f;
    float bias = max(sunMeta.z * qfb,
                     ((lighting0.y >= 36.5f && lighting0.y < 37.5f) ? 0.0f : 1.5f) * tw * iD)
               + qslope * tanq * tw * iD;   // (probe follows the term)
    float gap = c.z - st;   // >0 = something in front
    float gmm = gap / max(iD, 1e-9f) * 1000.0f;   // metres -> mm along the sun
    float bmm = bias / max(iD, 1e-9f) * 1000.0f;
    if (gap <= 0.0f) return float4(0.0f, 0.0f, 1.0f, 1.0f);   // BLUE: nothing in front
    if (gap <= bias) return float4(1.0f, 0.45f, 0.0f, 1.0f);   // ORANGE: eaten by the bias
    // GREEN ramp: a real occluder the bias does NOT eat, brightness = how far
    // past the bias it sits (full green at 10x).
    return float4(0.0f, saturate(gmm / max(bmm * 10.0f, 1e-6f)), 0.0f, 1.0f);
}

float4 KhSelfTierProbe(float3 wpos, float3 wrel, float3 nrm)
{
    float khtp_v = 1.0f - 0.65f * saturate(SunShadowOcclusionSelf(wpos, wrel, nrm));
    // the probe windows follow the term (same khwr arm).
    float3 khtp_wr = (lighting0.y >= 45.5f && lighting0.y < 46.5f)
                   ? (wpos - sunOrigin.xyz) : wrel;
    if (sunMeta2.x >= 0.5f) {
        float4 khtp_c = mul(float4(khtp_wr, 1.0f), sunVP2);
        float2 khtp_uv = float2(0.5f + 0.5f * khtp_c.x, 0.5f - 0.5f * khtp_c.y);
        if (khtp_uv.x > 0.002f && khtp_uv.x < 0.998f &&
            khtp_uv.y > 0.002f && khtp_uv.y < 0.998f &&
            khtp_c.z > 0.0f && khtp_c.z < 1.0f)
            return float4(khtp_v, 0.25f * khtp_v, 0.0f, 1.0f);
    }
    if (sunMeta3.x >= 0.5f) {
        float4 khtp_c = mul(float4(khtp_wr, 1.0f), sunVP3);
        float2 khtp_uv = float2(0.5f + 0.5f * khtp_c.x, 0.5f - 0.5f * khtp_c.y);
        if (khtp_uv.x > 0.002f && khtp_uv.x < 0.998f &&
            khtp_uv.y > 0.002f && khtp_uv.y < 0.998f &&
            khtp_c.z > 0.0f && khtp_c.z < 1.0f)
            return float4(khtp_v, khtp_v, 0.0f, 1.0f);
    }
    if (sunMeta4.x >= 0.5f) {
        float4 khtp_c = mul(float4(khtp_wr, 1.0f), sunVP4);
        float2 khtp_uv = float2(0.5f + 0.5f * khtp_c.x, 0.5f - 0.5f * khtp_c.y);
        if (khtp_uv.x > 0.002f && khtp_uv.x < 0.998f &&
            khtp_uv.y > 0.002f && khtp_uv.y < 0.998f &&
            khtp_c.z > 0.0f && khtp_c.z < 1.0f)
            return float4(0.0f, khtp_v, khtp_v, 1.0f);
    }
    if (sunMeta5.x >= 0.5f) {   // FAR band paint (magenta)
        float4 khtp_c = mul(float4(khtp_wr, 1.0f), sunVP5);
        float2 khtp_uv = float2(0.5f + 0.5f * khtp_c.x, 0.5f - 0.5f * khtp_c.y);
        if (khtp_uv.x > 0.002f && khtp_uv.x < 0.998f &&
            khtp_uv.y > 0.002f && khtp_uv.y < 0.998f &&
            khtp_c.z > 0.0f && khtp_c.z < 1.0f)
            return float4(khtp_v, 0.0f, khtp_v, 1.0f);
    }
    if (sunMeta.x >= 0.5f) {
        float4 khtp_c = mul(float4(khtp_wr, 1.0f), sunVP);
        float2 khtp_uv = float2(0.5f + 0.5f * khtp_c.x, 0.5f - 0.5f * khtp_c.y);
        if (khtp_uv.x > 0.002f && khtp_uv.x < 0.998f &&
            khtp_uv.y > 0.002f && khtp_uv.y < 0.998f &&
            khtp_c.z > 0.0f && khtp_c.z < 1.0f)
            return float4(0.1f * khtp_v, 0.25f * khtp_v, khtp_v, 1.0f);
    }
    return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

// DEBUG VISUAL 32 (setRenderDebug 395; dbgCtl.x 32) - WHY THIS PIXEL IS OR IS
// NOT CERTIFIED. Pure gauge.
float4 KhSelfCertProbe(float3 wpos, float3 wrel, float3 nrm)
{
    float4 khcp;
    float khcp_v = saturate(SunShadowOcclusionSelfEx(wpos, wrel, nrm, khcp));
    float3 khcp_c;
    if      (khcp.x < 0.5f) khcp_c = float3(0.35f, 0.35f, 0.35f);
    else if (khcp.x < 1.5f) khcp_c = float3(1.00f, 0.12f, 0.12f);
    else if (khcp.x < 2.5f) khcp_c = float3(1.00f, 0.55f, 0.00f);
    else if (khcp.y >= khcp.z - 0.5f) khcp_c = float3(0.20f, 0.45f, 1.00f);
    else                    khcp_c = float3(0.15f, 0.90f, 0.25f);
    float khcp_t = saturate(1.15f - 0.13f * khcp.z);
    return float4(khcp_c * (1.0f - 0.55f * khcp_v) * khcp_t, 1.0f);
}

float4 KhPfProbe(float3 wpos, float3 wrel, float3 nrm)
{
    if (sunMeta2.x < 0.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
    float3 khpp_wr = (lighting0.y >= 45.5f && lighting0.y < 46.5f)
                   ? (wpos - sunOrigin.xyz) : wrel;
    float4 khpp_c = mul(float4(khpp_wr, 1.0f), sunVP2);
    float2 khpp_uv = float2(0.5f + 0.5f * khpp_c.x, 0.5f - 0.5f * khpp_c.y);
    if (khpp_uv.x <= 0.002f || khpp_uv.x >= 0.998f ||
        khpp_uv.y <= 0.002f || khpp_uv.y >= 0.998f ||
        khpp_c.z <= 0.0f || khpp_c.z >= 1.0f)
        return float4(1.0f, 0.0f, 1.0f, 1.0f);   // MAGENTA: outside hero
    float khpp_st = khSunDepth2.Load(int3(int2(khpp_uv * sunMeta2.y), 0));
    float khpp_m0 = khSunPf2.SampleLevel(khPfSamp, khpp_uv, 0.0f).x;
    float khpp_m2 = khSunPf2.SampleLevel(khPfSamp, khpp_uv, 2.0f).x;
    return float4(saturate((khpp_c.z - khpp_st) * 200.0f),
                  saturate((khpp_c.z - khpp_m0) * 200.0f),
                  saturate((khpp_c.z - khpp_m2) * 200.0f), 1.0f);
}

float4 KhPfProbe2(float3 wpos, float3 wrel, float3 nrm)
{
    if (sunMeta2.x < 0.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
    float3 khpq_wr = (lighting0.y >= 45.5f && lighting0.y < 46.5f)
                   ? (wpos - sunOrigin.xyz) : wrel;
    float4 khpq_c = mul(float4(khpq_wr, 1.0f), sunVP2);
    float2 khpq_uv = float2(0.5f + 0.5f * khpq_c.x, 0.5f - 0.5f * khpq_c.y);
    if (khpq_uv.x <= 0.002f || khpq_uv.x >= 0.998f ||
        khpq_uv.y <= 0.002f || khpq_uv.y >= 0.998f ||
        khpq_c.z <= 0.0f || khpq_c.z >= 1.0f)
        return float4(1.0f, 0.0f, 1.0f, 1.0f);   // MAGENTA: outside hero
    float khpq_st = khSunDepth2.Load(int3(int2(khpq_uv * sunMeta2.y), 0));
    float khpq_gs = khSunPf2.SampleLevel(khPfSamp, khpq_uv, 0.0f).x;
    float khpq_gl = khSunPf2.Load(int3(int2(khpq_uv * sunMeta2.y * 0.5f), 0)).x;
    return float4(saturate((1.0f - khpq_st) * 12.0f),
                  saturate((1.0f - khpq_gs) * 12.0f),
                  saturate((1.0f - khpq_gl) * 12.0f), 1.0f);
}
)HLSL" R"HLSL(   // CHUNK BOUNDARY (precedent; chunks concatenate)
// the tier-probe visual took this segment past the 16380-byte MSVC token
// cap).

// This extension's meshes have never near-clipped. 0.05 m is a normal near
// plane for a first-person game, sits an order of magnitude below the
// engine's floor of 0.07 so it can never be the binding constraint, and is
// close enough that you have to press the camera into a surface to see it act
// at all.
static const float KH_OWN_NEAR = 0.05f;

static const float KH_STEN_FADE = 1.35f;

float2 KhStenTapComp(float2 khct_r, float3 khct_wp, float khct_wt)
{
    // Mode 199 rides dbgCtl.w alongside 58/180/181/182 - one at a time.
    if (!(dbgCtl.w >= 4.5f && dbgCtl.w < 5.5f)) return khct_r;
    if (lighting1.w < 0.5f) return khct_r;   // no valid sun vector

    float khct_n = abs(fxMeta.x);   // the engine's near
    if (!(khct_n > 0.0f) || khct_wt >= khct_n) return khct_r;   // no collapse

    float3 khct_L  = lighting1.xyz;   // unit, toward the sun
    float3 khct_wc = float3(viewProj[0].w, viewProj[1].w, viewProj[2].w);
    float  khct_dw = dot(khct_L, khct_wc);
    if (abs(khct_dw) < 1.0e-4f) return khct_r;   // light across the view

    // Both bounds below were picked by eye, and the geometry says they bite:
    // the walk must change VIEW DEPTH by (near - w) at a rate of dot(L,
    // viewProj w-column) - the sun's component along the view axis - which is
    // small whenever the sun is near perpendicular to where the camera looks,
    // so t blows through 25 m, and where it does not the projected tap blows
    // through a quarter screen.
    const bool khct_wide = (dbgCtl.w >= 5.5f && dbgCtl.w < 6.5f);

    float khct_t = (khct_n - khct_wt) / khct_dw;
    if (!khct_wide && abs(khct_t) > 25.0f) return khct_r;   // 's eye-picked line
    if (abs(khct_t) > 400.0f) return khct_r;   // never a receiver offset

    float4 khct_q = mul(float4(khct_wp + khct_t * khct_L, 1.0f), viewProj);
    if (khct_q.w < 1.0e-4f) return khct_r;   // behind the camera

    float2 khct_ndc = khct_q.xy / khct_q.w;
    float2 khct_px  = float2((khct_ndc.x * 0.5f + 0.5f) * fxMeta.z,
                             (0.5f - khct_ndc.y * 0.5f) * fxMeta.w);
    if (khct_wide) {
        if (khct_px.x < 0.0f || khct_px.y < 0.0f ||
            khct_px.x > fxMeta.z - 1.0f || khct_px.y > fxMeta.w - 1.0f) return khct_r;
        return khct_px;
    }
    float2 khct_d = khct_px - khct_r;
    float  khct_m = max(fxMeta.z, fxMeta.w) * 0.25f;
    if (dot(khct_d, khct_d) > khct_m * khct_m) return khct_r;
    return khct_px;
}

// TWIN CALL: PSMain and PSComposite both take this immediately after
// ClipEdgeSliver.
void ClipOwnNear(float khon_w)
{
    if (khon_w < KH_OWN_NEAR) discard;
}

void ClipEdgeSliver(float3 wpos, float3 nrm)
{
    // FAIL-SAFE: this clip trusts fxParams0.xyz as the camera. If any path
    // leaves it zeroed, nv computes against garbage and whole face regions
    // can be discarded - the partial-object flicker class, which outranks
    // fireflies absolutely.
    // The failure direction is now 'fireflies possible', never 'geometry
    // lost'.
    if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return;
    float3 khes_dx = ddx(wpos);
    float3 khes_dy = ddy(wpos);
    // Follow this block's own failure-direction rule (fail toward 'fireflies
    // possible', never 'geometry lost'): stand the clip down when either
    // derivative sits within ~4 ulps of the coordinate magnitude (2^-23 =
    // 1.2e-7; 16 = 4 ulps squared).
    float khes_ulp = max(max(abs(wpos.x), abs(wpos.y)), abs(wpos.z)) * 1.2e-7f;
    float khes_q2 = khes_ulp * khes_ulp * 16.0f;
    if (dot(khes_dx, khes_dx) < khes_q2 || dot(khes_dy, khes_dy) < khes_q2) return;
)HLSL" R"HLSL(   // CLOSE-RANGE STAND-DOWN (renderstats18 + the corner discriminator:
    // 'looking up clips at a corner edge, not at a side' - face-on nv~1 never
    // clips, but standing near a face's own PLANE, the natural corner
    // posture, puts a LARGE REAL face at true grazing incidence, and this
    // clip carried no distance qualification: a firefly test built for
    // DISTANT degenerate slivers ate close-up geometry that merely shares the
    // sliver's angle.
    float3 khes_v = fxParams0.xyz - wpos;
    float khes_vl = length(khes_v);
    if (khes_vl < 10.0f) return;
    float3 khes_fn = cross(khes_dx, khes_dy);
    float khes_nv = abs(dot(khes_fn, khes_v / max(khes_vl, 1.0e-6f)))
                  / max(length(khes_fn), 1.0e-12f);
    // The 10 m stand-down below was added for the same class (renderstats18:
    // "a firefly test built for DISTANT degenerate slivers ate close-up
    // geometry that merely shares the sliver's angle") and it covers the near
    // side of that surface only; the far side of the same wall was never
    // covered and is the wedge.
    float khes_deg = length(khes_fn)
                   / max(length(khes_dx) * length(khes_dy), 1.0e-20f);
    if (dbgCtl.w >= 10.5f && dbgCtl.w < 11.5f) khes_deg = 0.0f;   // 212: angle only
    if (khes_deg < 0.05f) clip(khes_nv - 0.005f);
}

float SolidMask(float3 wpos)
{
    float m = 1.0f;

    if (bandParams.w > 0.5f)
    {
        float d = distance(wpos, fxParams0.xyz);
        float fall = max(bandParams.z, 0.01f);
        float bm = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            bm *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        m *= bm;
    }

    if (localParams1.y > 0.5f)
    {
        float3 nd3 = abs(wpos - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // cube (Chebyshev)
                 : length(nd3);   // sphere/ellipsoid
        m *= 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
    }

    return m;
}

// ===========================================================================
// KH_DL_SHADOW (26822) - THE DYNAMIC-LIGHT SHADOW TERM.
//
// THE CUBE NEEDS NO MATRIX. KH_DLS_BASIS is an axis permutation with signs, so
// for a light-relative point the face is the dominant axis, the depth is that
// component, and the other two are the uv - all recoverable directly. Storing
// 48 matrices would restate the permutation in 3 KB and add a way for the CPU
// and the shader to disagree about it. Only the spot frustum is arbitrary, and
// only the spot frustum is stored.
//
// THE BIAS IS APPLIED IN METRES, NOT IN DEPTH. These maps are perspective, so
// a constant normalised bias is a different physical distance at every range -
// tight at the near plane, useless at the far. Offsetting the RECEIVER's
// distance before projecting keeps the bias a fixed number of centimetres
// wherever the receiver stands, which is the KH_SLOPE_TW idiom applied to a
// perspective map.
//
// A LIGHT WITH NO MAP ANSWERS LIT (1.0), and so does a face with no slice.
// Faces are allocated only where a caster stands, so 'no slice' is the common
// case and it means nothing occludes in that direction - a correct answer, not
// a missing one. Every failure path in here returns 1.0 for the same reason:
// the worst a bug can then do is fail to darken, never to darken wrongly.
// ===========================================================================
)HLSL" R"HLSL(
// CHUNK BOUNDARY - MSVC caps one string literal token at 16380 bytes
// (C2026). KH_DL_SHADOW took this chunk to 16342, leaving 38 bytes, and
// the rule is SPLIT, never trim, when a segment approaches the cap. Cut at
// a top-level statement boundary ahead of the light-shadow family so the
// whole kernel - filter, reconstruction and both call sites - has its own
// chunk with room to grow.
// How much of a light's per-light AMBIENT survives full shadow. 1 = the 26822
// behaviour (ambient never shadowed, so a pure-ambient light cannot cast);
// 0 = the shadowed side goes fully black, darker than the light being absent.
// Tune HERE, not per light: it is a look constant, not a per-light property,
// and there is deliberately no debug mode for it - 514 already stands the
// whole feature down and 519 the receive half, which is what an A/B needs.
static const float KH_DLS_AMB_KEEP = 0.35f;
// KH_DLS_BIAS_SLOPE_TEXEL (26876m). The slope term is priced in TEXELS, not
// metres, so it scales with range exactly as the depth error it covers does.
// 1.5 texels is the filter's own reach: one texel for the one the receiver sits
// in, half for the bilinear footprint the soft filter reaches into - the same
// 1.5 the receiver-normal offset above already uses, and deliberately the same
// number rather than a second one to drift.
static const float KH_DLS_SLOPE_TEXELS = 1.5f;
// ~76 degrees. A clamp and not a floor: past this a receiver spans effectively
// unbounded depth within one texel and NO finite bias is correct, so the right
// answer is to let that sliver acne rather than lift the whole shadow off its
// caster to prevent it.
static const float KH_DLS_SLOPE_TAN_MAX = 4.0f;
// KH_DLS_RPDB (26877). The gradient's clamp, in TEXEL-WIDTHS of depth per texel
// of lateral step. It is the KH_RPDB_GC idiom (the sun's tiers clamp their own
// gradient at 8 texels, metre-capped) and it is the only number in the new
// path that is not derived - so it is a CLAMP on a computed quantity rather
// than a bar standing in for one (rule 1.84). At 8, a receiver may be predicted
// to fall away by eight texel-widths of depth across one texel, which is
// tan(theta) = 8 or about 83 degrees; past that the sliver acnes.
static const float KH_DLS_GRAD_TEXELS = 8.0f;
// KH_DLS_NEAR_CAP (26878) - THE HLSL TWIN OF kh_dls_near, WHICH HAD DRIFTED.
//
// The C++ has clamped the near plane at KH_DLS_NEAR_CAP_M since 26876b
// (far / KH_DLS_NEAR_RATIO, floored at KH_DLS_NEAR_M, capped at 25 cm) and
// builds every face and spot matrix to that near. This file kept the uncapped
// 26822 form - max(far / 300, 0.05) - at both of its readers, so for any light
// whose map far exceeded 75 m the receiver's (a, c) were built to a DIFFERENT
// near than the map was rasterised with. At far = 100 m a receiver at 30 m was
// tested as if it stood at 24 m; at far = 200 m, at 12 m. Every reach up to
// 75 m was bit-identical, which is why it hid. The 26877 bias derivation above
// priced the depth format at n = 0.25, the C++ value the shader was not using.
//
// One reader for the rule, twin of kh_dls_near row for row; the three
// constants are the C++ ones and change with them.
static const float KH_DLS_NEAR_M     = 0.05f;
static const float KH_DLS_NEAR_RATIO = 300.0f;
static const float KH_DLS_NEAR_CAP_M = 0.25f;
float KhDlsNear(float khn_far)
{
    return min(max(khn_far / KH_DLS_NEAR_RATIO, KH_DLS_NEAR_M), KH_DLS_NEAR_CAP_M);
}
// KH_DLS_RANGE (26879) - RECEIVER-DISTANCE FADE AT THE SHADOW VIEW DISTANCE,
// the sun's own rule (KhSunRangeFade) on the light maps. The maps were never
// bounded by the camera: kh_dls_select scores by camera distance but culls by
// nothing, and the reach ceiling bounds the LIGHT's far plane, not the
// receiver's distance from the eye - so a lamp's shadow on the ground drew at
// any range while the sun's stopped at shadowVisibility. Same curve as the
// sun's default (full until 94%, gone at 99.5%), applied ONCE inside
// KhDlsShadow so every consumer - DynLights, KhDynLightsPBR, KhDlsWorldFactor,
// the mesh probe, both mesh passes and the world receive - fades identically
// (rule 1.5). Returns the SURVIVING fraction; 0 past the distance, 1 with the
// lane unfilled. Mode 581 zeroes the lane at the fill (the A/B).
float KhDlsRangeFade(float3 khrf_p)
{
    if (dlsRange.w <= 0.0f) return 1.0f;
    const float khrf_d = length(khrf_p - dlsRange.xyz);
    return 1.0f - smoothstep(0.94f * dlsRange.w, 0.995f * dlsRange.w, khrf_d);
}
// THE BIAS CONSTANTS WERE RE-DERIVED AT 26877, and the previous note asked for
// exactly that. It said KH_DLS_BIAS_SLOPE (0.006/m) was compensating for the
// missing slope term rather than for quantisation, and that it should be
// re-derived from the map's actual depth resolution. Here is that derivation,
// so the next reader can check it rather than trust it:
//
//   the maps are D32_FLOAT (kh_dls_ensure_maps), stored as a + c/z with
//   a = f/(f-n) and c = -nf/(f-n), so d(stored)/dz = nf/((f-n) z^2) and one
//   float ulp near stored ~ 1 is 2^-24. The metre-equivalent of one quantum is
//   therefore  dz = 2^-24 (f-n) z^2 / (n f).
//
//   at f = 100, n = 0.25 (the KH_DLS_NEAR_CAP value): 0.21 mm at 30 m and
//   2.4 mm at 100 m. The session worst case is f = 200, n = 0.25, z = 200:
//   9.5 mm.
//
// 0.006/m priced 180 mm at 30 m against a requirement of 0.21 mm - a factor of
// 850. The requirement is QUADRATIC in z and the term is linear, so the honest
// linear cover is set by the far end: 1e-4/m gives 20 mm at 200 m against
// 9.5 mm needed, and 3 mm at 30 m against 0.21 mm. Two-times margin where it is
// tight, fourteen where it is not.
//
// The constant term is not about the depth format at all - it covers the
// CASTER side, whose own surface tilts within a map texel and which no receiver
// plane can model. One centimetre, which is a quarter of what it was and still
// an order of magnitude above the format.
//
// BOTH ARE REVERTED TOGETHER BY MODE 573, and mode 574 arms the gradient while
// KEEPING the old constants - so the build can say which of the two changes did
// the work rather than shipping them as one indivisible move.

Texture2DArray<float> khDlsMaps : register(t36);

// PARITY WITH THE SUN FILTER (26823). The 26822 kernel took four POINT taps
// and averaged the verdicts, which is a 2x2 box over one texel - the blockiness
// reported from the field. These are KhSunBilinT / KhSunSoftT with the texture
// swapped for an array slice and NOTHING ELSE CHANGED: same bilinear weights,
// same 0.75-texel cross offset, same 5-tap average, same rule that a depth map
// is compared BEFORE it is filtered (the average of two depths is not the depth
// of the average). The sun and the lights therefore soften identically, which
// is the whole point of reusing the form rather than writing a second one.
// KH_DLS_MESHMODE (26876d, moved above KhDlsShadow at 26876m). The debug mode
// as the MESH path sees it. dbgCtl.w does not carry it there - every mesh fill
// writes an enumeration into that lane - so the mode rides dlsFaceSlice[].z,
// which kh_dls_fill_cb writes across all 48 entries and nothing else writes at
// all. Slot 0 is read because the value is replicated; it is not a statement
// about slot 0 being live. Declared HERE rather than beside its other readers
// because KhDlsShadow now reads it too and fxc has no linker pass.
int KhDlsMeshDbg() { return (int)dlsFaceSlice[0].z; }

// KH_DLS_RPDB (26877) - THE COMPARE IS BUILT PER TAP NOW, NOT ONCE.
//
// This took a single already-projected reference z and compared all four texels
// against it. That is only correct for a receiver PARALLEL to the map plane;
// on a tilted one the receiver's true depth differs at every texel of the
// footprint, and the constant bias had to be large enough to cover the whole
// spread. That is what the N.L divide (26822) and then the texel*tan term
// (26876m) were each standing in for, and it is why neither could stop
// peter-panning without over-biasing everything else.
//
// The sun does not do this: KhSelfTapT corrects the reference PER TAP by
// d.x*g.x + d.y*g.y - the depth the receiver's own plane would have at that
// texel - so its constant bias never has to cover the surface's tilt. This is
// that arithmetic, on an array slice, in metres.
//
//   khb_zb  the receiver's BIASED face-axis depth in METRES (khd_z - khd_b)
//   khb_tc  the RECEIVER's own uv in TEXELS. NEVER the filter's offset tap -
//           the gradient origin has to stay put while the footprint moves, or
//           the correction cancels the offset it is meant to accompany.
//   khb_g   metres of face-axis depth per texel along +u / +v
//   a/c/near  the face projection, applied per tap instead of once
float KhDlsBilin(float khb_sz, float2 uv, float khb_slice,
                 float khb_zb, float2 khb_tc, float2 khb_g,
                 float khb_a, float khb_c, float khb_near)
{
    float2 tx = uv * khb_sz - 0.5f;
    float2 f = frac(tx);
    // KH_DLS_FLOOR (26876). floor, NOT truncate. int2() rounds toward zero
    // while frac() is x - floor(x), so the two disagree wherever tx < 0 - and
    // tx DOES go negative here: KhDlsFaceUV admits uv > 0.001, which at 1024 px
    // is tx > 0.524, and KhDlsSoft then offsets the tap by -0.75 texel, reaching
    // tx = -0.226. Across that sliver p0 read 0 while f read 0.77, so the lerp
    // weighted texel 1 where it should have weighted texel 0 - the outermost
    // texel of every cube face sampling its neighbour instead of itself.
    //
    // KhSunBilinT carries the identical line and is NOT touched: the sun path
    // is out of scope by the operator's own scoping and works as intended. Its
    // uv guard differs, so the sliver may not even be reachable there. Flagged,
    // not fixed - see the handoff.
    int2 p0 = int2(floor(tx));
    int khb_s = (int)khb_slice;
    int khb_mx = (int)khb_sz - 1;
    // Tap order 0..3 = (0,0) (1,0) (0,1) (1,1), which is the o00/o10/o01/o11
    // the lerp pair below has always consumed - the loop is a rewrite of four
    // hand-written taps, not a reordering of them.
    float4 khb_o;
    [unroll] for (int khb_k = 0; khb_k < 4; ++khb_k) {
        int2 khb_q = clamp(p0 + int2(khb_k & 1, khb_k >> 1),
                           int2(0, 0), int2(khb_mx, khb_mx));
        // Texel CENTRES sit at integer + 0.5 in this convention, and khb_tc is
        // in the same one, so the difference is a signed count of texels.
        float2 khb_d = (float2(khb_q) + 0.5f) - khb_tc;
        float khb_zq = khb_zb + khb_d.x * khb_g.x + khb_d.y * khb_g.y;
        float khb_ref = khb_a + khb_c / max(khb_zq, khb_near);
        khb_o[khb_k] = (khb_ref > khDlsMaps.Load(int4(khb_q, khb_s, 0))) ? 1.0f : 0.0f;
    }
    return lerp(lerp(khb_o.x, khb_o.y, f.x), lerp(khb_o.z, khb_o.w, f.x), f.y);
}
)HLSL" R"HLSL(
// CHUNK BOUNDARY (26877). SPLIT, never trim: the per-tap rewrite of KhDlsBilin
// and the parameter list KhDlsSoft now threads took this segment to 14752, and
// the receiver-plane note above KhDlsBilin is the record of why two builds each
// replaced one stand-in with another. Cut at a top-level statement boundary
// between the bilinear tap and the filter that calls it.
// KH_DLS_PCF (26872). The 5-tap cross and two alternatives, selectable live.
//
// THE DEFAULT IS UNCHANGED AT MODE 0. 548 and 549 exist because the 545 capture
// showed the ground rings are FINER than the projected texel trapezoids, and
// nothing in the map lookup can have a sub-texel period - inside a texel the
// stored depth is constant. This filter can: KhDlsBilin compares BINARY per
// texel and then bilerps the four verdicts, so each tap is a ramp pinned to 0/1
// at the corners, and five of those are summed at +/-0.75 TEXEL. An 0.75 offset
// against a 1.0 grid beats with a period of exactly 3 texels - sub-texel
// structure, sweeping through phase as the projected texel size varies across a
// grazing surface, which draws concentric rings. Up close the trapezoids are
// huge, the ramps stretch over many pixels, and the binary corners read as flat
// bands with a soft fade instead of an edge. One mechanism, both symptoms.
//
//   548 SINGLE TAP - the diagnosis. No 5-tap sum, so no beat. If the rings
//       change character or vanish (leaving hard aliased edges), the filter is
//       the source. If they survive unchanged, it is not and this whole note
//       is wrong.
//   549 3x3 AT ONE TEXEL - the candidate cure. Nine taps on the grid's own
//       period cannot beat with it, and nine verdicts give a genuinely graded
//       penumbra instead of five ramps between binary corners.
// HAZARD (recorded 26876, NOT fixed): dbgCtl.w DOES NOT CARRY THE DEBUG MODE ON
// THE MESH PATH. kh_dls_world_pass writes dbg_ctl[3] = the raw setRenderDebug
// mode, but every MESH fill site writes an ENUMERATION there instead (0..~13,
// the mode-58/180/191/194 ladder). So 548, 549 and KhDlsShadow's 552 are
// world-receive-only: on a mesh they can never compare true and the default
// five-tap always runs.
//
// That is harmless for the ground rings, which are a world-pass artifact, and
// it is why "548 leaves the rings unchanged" was still a valid reading. It is
// NOT harmless as a habit - it is process failure 7 of the last campaign in a
// quieter form, a mode that reaches half the consumers of a shared helper while
// reading as though it reached both. Anything routed through dbgCtl.w in this
// file must be checked against BOTH fill sites before its result is trusted.
// The fix is a lane of our own, as KH_DLS_FACE_FLAT already did for mode 520's
// arm; it is not taken here because these three modes are cures and reverts for
// refuted hypotheses and are scheduled for deletion, and widening a mode's
// reach on the way to deleting it is work in the wrong direction.
// KH_DLS_RPDB (26877): khs_zb / khs_g / a / c / near replace the single
// pre-projected z. khs_tc is computed ONCE here, from the unoffset receiver uv,
// and handed to every tap - the five-tap cross and the 3x3 both move the
// FOOTPRINT and must not move the gradient's origin with it.
float KhDlsSoft(float khs_sz, float2 uv, float khs_slice,
                float khs_zb, float2 khs_g,
                float khs_a, float khs_c, float khs_near)
{
    const int khs_m = (int)dbgCtl.w;
    const float2 khs_tc = uv * khs_sz;

    if (khs_m == 548) return KhDlsBilin(khs_sz, uv, khs_slice, khs_zb, khs_tc,
                                        khs_g, khs_a, khs_c, khs_near);

    if (khs_m == 549) {
        const float khs_t = 1.0f / max(khs_sz, 1.0f);
        float khs_acc = 0.0f;
        [unroll] for (int khs_y = -1; khs_y <= 1; ++khs_y) {
            [unroll] for (int khs_x = -1; khs_x <= 1; ++khs_x) {
                khs_acc += KhDlsBilin(khs_sz, uv + float2(khs_x, khs_y) * khs_t,
                                      khs_slice, khs_zb, khs_tc, khs_g,
                                      khs_a, khs_c, khs_near);
            }
        }
        return khs_acc * (1.0f / 9.0f);
    }

    float khs_o = 0.75f / max(khs_sz, 1.0f);
    return (KhDlsBilin(khs_sz, uv, khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near)
          + KhDlsBilin(khs_sz, uv + float2( khs_o, 0.0f), khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near)
          + KhDlsBilin(khs_sz, uv + float2(-khs_o, 0.0f), khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near)
          + KhDlsBilin(khs_sz, uv + float2(0.0f,  khs_o), khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near)
          + KhDlsBilin(khs_sz, uv + float2(0.0f, -khs_o), khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near)) * 0.2f;
}

// khd_nrm is the receiver's WORLD normal. 26822 took none, and that is the
// strobe: the mesh shadowing ITSELF is the common case for a lamp beside a
// prop, and a self-compare with only a constant bias ACNES - the receiver's own
// depth crosses the map's by more than the bias wherever the surface slants
// away, and every acne band crawls as anything moves. On screen that reads as
// strobing, not as acne.
//
// The sun's self kernel has three defences here (KH_SUN_HERO_BASE's receiver-
// normal offset, the hero-priced slope term, and the metre-clamped receiver-
// plane gradient) and the cast chain's own note says it has NONE of them. This
// kernel had none either. It now takes the first two, which are the ones that
// matter for a self-compare:
//
//   NORMAL OFFSET - move the receiver off its own surface by the world size of
//   a map texel at that range, so the sample lands on the surface rather than
//   inside it. Priced in texels, not metres, so it scales with range exactly
//   as the error it cancels does (the KH_SLOPE_TW idiom).
//
//   SLOPE-SCALED BIAS - a surface at a grazing angle to the light spans more
//   depth per texel, so the bias divides by N.L. Clamped, or a silhouette
//   grazing to zero would demand infinite bias and peter-pan the whole shadow.
//
// Mode 520 disarms both, which is the A/B: if the strobe returns under 520 and
// not at default, it was acne.
)HLSL" R"HLSL(
// KH_DLS_FACEUV (26871). The face, slice and uv selection, lifted verbatim out
// of KhDlsShadow so the banding instrument asks the SAME question the shadow
// lookup asks instead of keeping a second copy to drift (rule 1.5). Returns
// false where the lookup would have returned "lit" without sampling. Given its
// own segment because KhDlsShadow's had 968 bytes left - SPLIT, never trim.
// KH_DLS_RPDB (26877) ADDS THREE OUTPUTS, AND THEY ARE NOT DECORATION.
//
// The receiver-plane gradient needs the axes the uv is measured along, and this
// is the only function that knows them - the cube branch below picks a face and
// its (right, up) pair by inspecting the dominant component, and the spot
// branch's pair lives inside dlsSpotVP's columns. Deriving them a second time
// somewhere else is a copy that can drift from the face selection it has to
// agree with (rule 1.5), so they come out of the selection itself.
//
//   khf_r / khf_u  the projection's right / up axes, LIGHT-RELATIVE (which is
//                  world-parallel: dlsMeta.xyz is a translation, no rotation)
//   khf_sx         the projection's lateral scale. 1 for a cube face, which is
//                  exactly 90 degrees; 1/tan(fov/2) for a spot. One texel spans
//                  2*z/(size*sx) metres laterally, so this is the lane that
//                  stops a narrow spot being priced as a 90 degree face.
// KH_DLS_FACE_SEAM (26878) - khf_sel. The handoff recorded a lit seam along
// every cube face boundary a shadow crossed, with two independent
// contributors. The first is a bias-ORDERING fault: the receiver-normal offset
// displaced the position BEFORE the face was chosen, so a receiver within 1.5
// texels of an edge could flip to the neighbouring face and land at uv ~0.0005
// there. khf_sel is the UNOFFSET light-relative position and is now the only
// input to the face choice; khf_p (offset) is what gets projected. The spot
// branch has one frustum and never selected, so it ignores khf_sel. Callers
// with no offset pass the same vector twice.
bool KhDlsFaceUV(int khf_slot, float3 khf_p, float3 khf_sel, float khf_near, float khf_far,
                 out float2 khf_uv, out float khf_z, out float khf_slice,
                 out float3 khf_r, out float3 khf_u, out float khf_sx)
{
    khf_uv = float2(0.0f, 0.0f);
    khf_z = 0.0f;
    khf_slice = -1.0f;
    khf_r = float3(1.0f, 0.0f, 0.0f);
    khf_u = float3(0.0f, 1.0f, 0.0f);
    khf_sx = 1.0f;

    if (dlsCtl[khf_slot].x >= 0.5f) {
        float4 khf_cl = mul(float4(khf_p, 1.0f), dlsSpotVP[khf_slot]);
        if (khf_cl.w <= 1.0e-6f) return false;          // behind the spot
        khf_uv = float2(0.5f + 0.5f * khf_cl.x / khf_cl.w,
                        0.5f - 0.5f * khf_cl.y / khf_cl.w);
        khf_z = khf_cl.w;
        khf_slice = dlsCtl[khf_slot].z;
        // Row-vector convention: clip.x = dot(p, column x), and kh_dls_spot_vp
        // builds column x as right * s. So the column's DIRECTION is the right
        // axis and its LENGTH is the lateral scale - one read gives both.
        const float3 khf_cx = float3(dlsSpotVP[khf_slot][0].x,
                                     dlsSpotVP[khf_slot][1].x,
                                     dlsSpotVP[khf_slot][2].x);
        const float3 khf_cy = float3(dlsSpotVP[khf_slot][0].y,
                                     dlsSpotVP[khf_slot][1].y,
                                     dlsSpotVP[khf_slot][2].y);
        khf_sx = max(length(khf_cx), 1.0e-6f);
        khf_r = khf_cx / khf_sx;
        khf_u = khf_cy / max(length(khf_cy), 1.0e-6f);
    } else {
        // Dominant axis = the cube face. st is that face's (right, up) pair,
        // read straight off KH_DLS_BASIS - the C++ table's twin.
        // The face is chosen from khf_sel; z, st and the axes are read off
        // khf_p on THAT face (KH_DLS_FACE_SEAM). khf_z is the signed depth
        // along the chosen face's forward axis, which was abs(khf_p.*) while
        // the same vector chose the face and is identical whenever it still
        // does.
        const float3 khf_ap = abs(khf_sel);
        float2 khf_st;
        int khf_face;
        // khf_r / khf_u are the SAME permutation khf_st applies, written as
        // vectors: st.x = dot(p, khf_r) and st.y = dot(p, khf_u) hold on every
        // branch, and they reproduce KH_DLS_BASIS row for row (the C++ twin).
        if (khf_ap.x >= khf_ap.y && khf_ap.x >= khf_ap.z) {
            const bool khf_pos = khf_sel.x > 0.0f;
            khf_z = khf_pos ? khf_p.x : -khf_p.x;
            khf_face = khf_pos ? 0 : 1;
            khf_st = khf_pos ? float2(-khf_p.z, khf_p.y)
                             : float2( khf_p.z, khf_p.y);
            khf_r = float3(0.0f, 0.0f, khf_pos ? -1.0f : 1.0f);
            khf_u = float3(0.0f, 1.0f, 0.0f);
        } else if (khf_ap.y >= khf_ap.z) {
            const bool khf_pos = khf_sel.y > 0.0f;
            khf_z = khf_pos ? khf_p.y : -khf_p.y;
            khf_face = khf_pos ? 2 : 3;
            khf_st = khf_pos ? float2(khf_p.x, -khf_p.z)
                             : float2(khf_p.x,  khf_p.z);
            khf_r = float3(1.0f, 0.0f, 0.0f);
            khf_u = float3(0.0f, 0.0f, khf_pos ? -1.0f : 1.0f);
        } else {
            const bool khf_pos = khf_sel.z > 0.0f;
            khf_z = khf_pos ? khf_p.z : -khf_p.z;
            khf_face = khf_pos ? 4 : 5;
            khf_st = khf_pos ? float2( khf_p.x, khf_p.y)
                             : float2(-khf_p.x, khf_p.y);
            khf_r = float3(khf_pos ? 1.0f : -1.0f, 0.0f, 0.0f);
            khf_u = float3(0.0f, 1.0f, 0.0f);
        }
        if (khf_z <= khf_near) return false;            // inside the near plane
        khf_uv = float2(0.5f + 0.5f * khf_st.x / khf_z,
                        0.5f - 0.5f * khf_st.y / khf_z);
        khf_slice = dlsFaceSlice[khf_slot * 6 + khf_face].x;
    }

    if (khf_slice < 0.0f) return false;                 // this face has no slice
    if (khf_z >= khf_far) return false;                 // past the light's reach
    // KH_DLS_FACE_SEAM (26878), the second contributor: this guard refused the
    // outermost texel of every face, and the cube has no guard band to make
    // up for it - so a receiver at the boundary answered LIT, one texel wide,
    // six faces times four edges. The spot keeps the refusal: outside its one
    // frustum there is genuinely no map, and lit is the right answer. The cube
    // CLAMPS instead: six 90-degree faces tile the sphere, so the texel column
    // at a face's edge is rasterised right up to the clip plane and is the
    // correct sample for a receiver at that boundary to within one texel
    // (KhDlsBilin already clamps its taps to the map for the same reason).
    // Mode 580 restores the refusal on the cube - the A/B; the seam returns.
    if (dlsCtl[khf_slot].x >= 0.5f || KhDlsMeshDbg() == 580) {
        if (khf_uv.x <= 0.001f || khf_uv.x >= 0.999f ||
            khf_uv.y <= 0.001f || khf_uv.y >= 0.999f) return false;
    } else {
        khf_uv = saturate(khf_uv);
    }
    return true;
}

// KH_DLS_RPDB (26877) - THE RECEIVER-PLANE DEPTH GRADIENT. THE SUN'S THIRD
// DEFENCE, WHICH THIS KERNEL HAS BEEN STANDING IN FOR SINCE 26822.
//
// The note above KhDlsShadow has recorded since 26822 that the sun has three
// defences against a self-compare and this kernel took two. 26822 stood in for
// the missing one by dividing the whole bias by N.L; 26876m replaced that with
// khd_texel * tan(theta), which is a better-shaped stand-in for the same
// quantity. Both are the same admission: the bias has to cover how much the
// receiver's depth CHANGES ACROSS THE FILTER FOOTPRINT, and neither knew that
// number, so both bought it with peter-panning.
//
// The number is not hard to have. On a plane, depth is an affine function of
// map position, so it can be corrected exactly instead of covered:
//
//   the cube face maps u = 0.5 + 0.5*sx*(p.r)/z, v = 0.5 - 0.5*sx*(p.u)/z, and
//   the receiver plane is n.p = d. Substituting s = (2u-1)z/sx and
//   t = (1-2v)z/sx and solving for z gives z = d / (n.r(2u-1)/sx +
//   n.u(1-2v)/sx + n.f), so
//
//       dz/du = -2 (n.r) z^2 / (sx d)      dz/dv = +2 (n.u) z^2 / (sx d)
//
//   and per TEXEL that is the same over the map size. No N.L, no tangent, no
//   clamp shaping the answer - the two dot products and z are the whole of it.
//
// WHY IT IS CLAMPED AND NOT FLOORED, which is the one judgement call here.
// d = n.p goes to zero as the receiver turns edge-on to the light, and there
// the plane genuinely spans unbounded depth inside one texel - no correction is
// right, exactly as no bias was right for the 26876m tangent. So the gradient
// is capped at KH_DLS_GRAD_TEXELS texel-widths of depth per texel of lateral
// step, which is the KH_RPDB_GC idiom the sun's tiers already use, and that
// sliver is allowed to acne rather than the whole shadow being lifted.
//
// The divide is guarded by MAGNITUDE, keeping d's sign: d is negative for a
// receiver facing the light and positive for one facing away, and flipping it
// would tilt the correction the wrong way on back faces rather than merely
// scale it.
float2 KhDlsGrad(float3 khg_p, float3 khg_n, float3 khg_r, float3 khg_u,
                 float khg_z, float khg_sz, float khg_sx, float khg_texel)
{
    const float khg_d = dot(khg_n, khg_p);
    const float khg_ad = max(abs(khg_d), 1.0e-4f);
    const float khg_sd = (khg_d < 0.0f) ? -khg_ad : khg_ad;
    const float khg_k = (khg_z * khg_z)
                      / (max(khg_sz, 1.0f) * max(khg_sx, 1.0e-6f) * khg_sd);
    const float2 khg_g = float2(-2.0f * dot(khg_n, khg_r),
                                 2.0f * dot(khg_n, khg_u)) * khg_k;
    const float khg_c = KH_DLS_GRAD_TEXELS * khg_texel;
    return clamp(khg_g, -khg_c, khg_c);
}
)HLSL" R"HLSL(
// KH_DLS_ZBIAS (26874). khd_zunc is the receiver's own depth UNCERTAINTY in
// metres, supplied by the CALLER because only the caller knows how its position
// was obtained. A mesh passes 0: its position is interpolated geometry and is
// exact. The world-receive pass passes the measured per-pixel step in the
// engine's linear depth, because its position is unprojected FROM that depth
// and inherits whatever quantisation it carries.
//
// WHY: mode 550 painted that step and the field found BLACK PLATEAUS beside
// WHITE CLIFFS - adjacent pixels decoding the identical depth, then jumping
// 100 mm or more. KH_DLS_BIAS_M is 0.04, so the compare bias was 40 mm against
// a receiver uncertainty of 100 mm+, and inside every plateau the test flips on
// quantisation alone. Eight earlier hypotheses are refuted in the
// KH_DLSW_BANDING ledger; this one is not a theory about the artifact, it is
// two measured numbers in the wrong order.
float KhDlsShadow(int khd_slot, float3 khd_wpos, float3 khd_nrm, float khd_zunc)
{
    if (khd_slot < 0 || khd_slot > 7) return 1.0f;
    float4 khd_meta = dlsMeta[khd_slot];
    if (khd_meta.w <= 0.0f) return 1.0f;               // no map for this light
    // KH_DLS_RANGE (26879): past the shadow view distance the answer is lit
    // and no texel is worth reading; inside the band the verdict is thinned
    // at the return below, so the kernel itself is unchanged.
    const float khd_rf = KhDlsRangeFade(khd_wpos);
    if (khd_rf <= 0.0f) return 1.0f;

    // Face-axis distance BEFORE the offset, only to price the texel: the
    // dominant component is the face's depth axis, and one texel of a 90 degree
    // face at distance z is 2z / size in world units.
    const float3 khd_p0 = khd_wpos - khd_meta.xyz;
    const float  khd_z0 = max(max(abs(khd_p0.x), abs(khd_p0.y)), abs(khd_p0.z));
    uint khd_mw, khd_mh, khd_me;
    khDlsMaps.GetDimensions(khd_mw, khd_mh, khd_me);
    // AN UNBOUND ARRAY REPORTS ZERO DIMENSIONS. Without this the texel price
    // below divides by max(0, 1) and returns 2z, displacing the receiver twice
    // its own distance from the light - which is how a missing BINDING became a
    // missing SHADOW in 26828. Answer lit and say nothing else; the bind is
    // repaired at its source (KH_DLS_SRV_REBIND), this only refuses to compute
    // nonsense from a resource that is not there.
    if (khd_mw < 2u) return 1.0f;
    const float  khd_texel = 2.0f * khd_z0 / max((float)khd_mw, 1.0f);
    // THE ARM IS OUR OWN LANE, NOT lighting0.y. Code 83 there is already
    // KH_SLOPE_TW (mode 457) and the ladder is contested across the whole
    // renderer; dlsFaceSlice[].y is our own lane. A mode collision is
    // silent and costs a build to find.
    const int    khd_fbase = khd_slot * 6;
    const bool   khd_off_on = dlsFaceSlice[khd_fbase].y >= 0.5f;
    const float3 khd_n = normalize(khd_nrm);
    // 1.5 texels: one to clear the texel the receiver sits in, half for the
    // bilinear footprint the soft filter reaches into.
    const float3 khd_p = khd_off_on ? (khd_p0 + khd_n * (khd_texel * 1.5f)) : khd_p0;
    const float  khd_far = khd_meta.w;
    const float  khd_near = KhDlsNear(khd_far);   // KH_DLS_NEAR_CAP twin (26878)
    const float  khd_a = khd_far / (khd_far - khd_near);
    const float  khd_c = -khd_near * khd_far / (khd_far - khd_near);

    float2 khd_uv;
    float  khd_z;      // receiver distance along the face axis, in METRES
    float  khd_slice;
    float3 khd_fr, khd_fu;   // KH_DLS_RPDB: the projection's axes and lateral
    float  khd_fsx;          // scale, for the receiver-plane gradient
    if (!KhDlsFaceUV(khd_slot, khd_p, khd_p0, khd_near, khd_far, khd_uv, khd_z, khd_slice,
                     khd_fr, khd_fu, khd_fsx)) {
        return 1.0f;
    }

    // Bias in metres, then project: constant centimetres at every range, and
    // SLOPE-SCALED so a grazing surface gets the depth span its texel covers.
    // The clamp at 0.25 stops a silhouette from demanding an unbounded bias,
    // which would lift the shadow off its caster entirely.
    // KH_DLS_BIAS_SLOPE_TEXEL (26876m) - THE LIGHT BLEED ALONG AN EDGE.
    //
    // 26822 divided the WHOLE metre bias by N.L with a 0.25 floor. That inflates
    // every term, including the two that have nothing to do with slope, by up to
    // 4x on a face turned edge-on to the lamp: at 15 m the compare bias reached
    // 55 cm and at 30 m it reached 94 cm. A compare bias is a distance the
    // receiver is pushed TOWARD the light before it is tested, so half a metre
    // of it is half a metre of peter-panning - the shadow detaches from its own
    // caster and light lands in a band hugging the silhouette. On a box lit from
    // behind that band is a bright strip down the edge, which is the report.
    //
    // THE SUN DOES NOT DO THIS AND THAT IS THE REFERENCE. KhSelfTapT carries a
    // receiver-plane gradient - per tap it corrects the reference depth by
    // d.x*g.x + d.y*g.y, the depth the receiver's own plane would have at that
    // texel - so its constant bias never has to cover the surface's tilt and
    // never grows. The note above this kernel already recorded that the sun has
    // three defences here and that this one took only two; the gradient is the
    // one it skipped, and dividing by N.L was standing in for it.
    //
    // The stand-in is replaced with the quantity it was approximating. What a
    // bias must actually cover is how much the receiver's depth CHANGES ACROSS
    // THE FILTER FOOTPRINT, which is one texel in world metres times the surface
    // tilt: khd_texel * tan(theta), with tan from the same N.L. That is priced
    // in texels like every other term in this kernel (the KH_SLOPE_TW idiom), it
    // scales with range exactly as the error it covers does, and it leaves the
    // constant and range terms UNDIVIDED because neither is a slope.
    //
    // The tangent is clamped rather than floored, and the clamp is the honest
    // part: at 90 degrees a receiver spans infinite depth in one texel and no
    // finite bias is correct, so the surface is allowed to acne there rather
    // than the whole shadow being lifted off its caster to prevent it.
    //
    // Mode 572 restores the 26822 divide.
    // KH_DLS_RPDB (26877). The stand-in is GONE, not re-shaped: the filter now
    // corrects its reference per tap by the receiver's own plane (KhDlsGrad,
    // whose derivation sits with it), so the bias no longer has to cover the
    // surface's tilt and the slope term has nothing left to do. What remains is
    // the constant, the range term - both re-derived at their declarations - and
    // one texel for the caster's own rasterisation.
    //
    //   572  the 26822 N.L divide (unchanged)
    //   573  the 26876m texel*tan stand-in, gradient off. With the C++ twin
    //        restoring the old constants this is bit-identical to 26876m.
    //   574  gradient ON, old constants (C++ only - it does not reach here).
    const int    khd_dbg = KhDlsMeshDbg();
    const float3 khd_ldir = normalize(-khd_p);
    const float  khd_ndl = khd_off_on ? max(dot(khd_n, khd_ldir), 0.05f) : 1.0f;
    const float  khd_zu = (khd_dbg == 552) ? 0.0f : khd_zunc;
    float khd_b;
    float2 khd_g = float2(0.0f, 0.0f);

    if (khd_dbg == 572) {
        khd_b = (dlsCtl[khd_slot].y + dlsCtl[khd_slot].w * khd_z) / max(khd_ndl, 0.25f)
              + (khd_off_on ? khd_texel : 0.0f) + khd_zu;
    } else if (khd_dbg == 573) {
        // tan(theta) between the surface and the light, clamped at ~76 degrees.
        const float khd_tan = min(sqrt(max(1.0f - khd_ndl * khd_ndl, 0.0f))
                                / max(khd_ndl, 1.0e-3f), KH_DLS_SLOPE_TAN_MAX);
        khd_b = dlsCtl[khd_slot].y + dlsCtl[khd_slot].w * khd_z
              + (khd_off_on ? khd_texel * (1.0f + KH_DLS_SLOPE_TEXELS * khd_tan) : 0.0f)
              + khd_zu;
    } else {
        khd_b = dlsCtl[khd_slot].y + dlsCtl[khd_slot].w * khd_z
              + (khd_off_on ? khd_texel : 0.0f) + khd_zu;
        khd_g = KhDlsGrad(khd_p, khd_n, khd_fr, khd_fu, khd_z,
                          (float)khd_mw, khd_fsx, khd_texel);
    }

    // The sun's own 5-tap soft compare, on an array slice. Returns OCCLUSION
    // in the sun's convention (1 = blocked); this kernel's contract is LIT, so
    // it is inverted once, here, rather than the filter being rewritten to a
    // second convention that could drift from the sun's.
    // The second GetDimensions this used to spend is gone: khd_mw is the same
    // query, already made above to price the texel, and two reads of one
    // resource are two things that can disagree (rule 1.5).
    const float khd_occ = KhDlsSoft((float)khd_mw, khd_uv, khd_slice,
                                    khd_z - khd_b, khd_g,
                                    khd_a, khd_c, khd_near);
    return saturate(1.0f - khd_occ * khd_rf);   // KH_DLS_RANGE: thinned, not cut
}
)HLSL" R"HLSL(
// CHUNK BOUNDARY (26877). SPLIT, never trim: the receiver-plane gradient and
// its two reverts took this segment to 15723, leaving 657 bytes, and this
// chunk still carries DynLights and both mesh-side probes. Cut at a top-level
// statement boundary immediately after KhDlsShadow, so the kernel keeps its own
// chunk and the probes and the light loop keep theirs.
// KH_DLSW_MESHPROBE (26876d, mode 560) - WHAT DOES THE MESH RECEIVE?
//
// Mode 558 exonerated the world pass for the see-through: on our mesh it
// returns 1.0 at every pixel, either because the self-mask refused it (green)
// or because the surface behind is sky and the depth test refused it first
// (unchanged) - and a multiply by 1.0 writes nothing. Mode 539 agreed, painting
// the mesh silhouette solid black. So whatever draws background structure onto
// the vest is downstream of KhDlsShadow in the MESH kernels, and no lane can
// see inside a pixel shader.
//
// This returns the strongest dynamic-light occlusion any casting light claims
// at this point - 1 lit, 0 fully blocked - so ApplyLighting and KhApplyPBR can
// paint it directly as greyscale. If the reported outline appears here, it is
// the shadow lookup and the map or its bias owns it. If the surface comes back
// FLAT WHITE while the outline is still visible in mode 0, the lookup claims
// nothing at those pixels and the artifact is not the dynamic-light shadow at
// all - which would also mean the mode-533 baseline needs re-taking.
float KhDlsMeshProbe(float3 khpm_wpos, float3 khpm_nrm)
{
    if (dlCtl.x < 0.5f) return 1.0f;
    const int khpm_n = (int)dlCtl.y + (int)dlCtl.z;
    float khpm_m = 1.0f;

    [loop] for (int khpm_i = 0; khpm_i < khpm_n; ++khpm_i) {
        // The same slot lane the shading loops read, so this cannot disagree
        // with them about which light casts (rule 1.5).
        const int khpm_s = (int)dlLights[khpm_i * 6 + 5].z - 1;
        if (khpm_s < 0) continue;
        khpm_m = min(khpm_m, KhDlsShadow(khpm_s, khpm_wpos, khpm_nrm, 0.0f));
    }

    return khpm_m;
}

// Math is the lit-material disassembly VERBATIM - distance attenuation 1/(a0
// + a1*d + a2*d^2) on the offset, scaled distance, the hard range fade, the
// spot cone pow, and the per-light AMBIENT term accumulated without N.L (the
// away-facing glow that makes A3 lights read on surfaces).
float3 DynLights(float3 wpos, float3 nrm)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;

    if (dlCtl.x >= 2.5f) {
        // mode 3: ABSOLUTE world positions (engine axes) - no camera
        // dependency at all; the CPU merged the pool in world space.
        p = wpos;
    } else {
        // modes 1/2 (diagnostics): camera-origin decodes. fxParams0.xyz
        // carries the camera on the solid-mesh paths (the ClipEdgeSliver
        // contract); a zeroed camera cannot decode camera-origin lights -
        // stand down rather than shade garbage.
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }
    }

    float3 acc = float3(0.0f, 0.0f, 0.0f);

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = dlLights[b + 0].xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - dlLights[b + 4].x, 0.0f);
        float att = saturate(1.0f / (dot(dlLights[b + 4].yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - dlLights[b + 5].x) * dlLights[b + 5].y);

        if (i >= pointN) {
            // spot cone: the engine's log/mul/exp pow; the (c > 0) guard
            // stands in for log(0) = -inf -> exp -> 0, and dodges the pow(0,
            // 0) NaN a degenerate exponent would mint.
            float c = saturate((dot(-dlLights[b + 1].xyz, L) - dlLights[b + 1].w) * dlLights[b + 2].w);
            att *= (c > 0.0f) ? pow(c, dlLights[b + 3].w) : 0.0f;
        }

        // KH_DL_SHADOW (26822). The slot rides in dlLights[b + 5].z, a lane
        // NEITHER kernel has ever read (0 = no shadow, 1 + slot otherwise), so
        // an unfilled site behaves exactly as it did - the 264 precedent.
        // wpos is WORLD in every dlCtl mode, and dlsMeta is world, so the
        // lookup does not care which decode the loop above used.
        //
        // THE SHADOW SCALES THE DIRECTIONAL TERM ONLY. dlLights[b + 3] is the
        // per-light AMBIENT - the away-facing glow that makes A3 lights read on
        // surfaces facing away from them - and a surface in shadow is still
        // inside that glow. Multiplying it would black out the shadowed side
        // completely, which is darker than the light being absent at all.
        // Opacity therefore needs no separate control: what a shadow removes is
        // exactly the contribution being blocked, so a dim light casts a faint
        // shadow and a bright one a hard shadow, for free.
        const float khs_sh = KhDlsShadow((int)dlLights[b + 5].z - 1, wpos, nrm, 0.0f);
        // KH_DLS_AMBIENT_SHADOW (26827) - A PURE-AMBIENT LIGHT MUST STILL CAST.
        //
        // 26822 shadowed the DIRECTIONAL term only and left dlLights[b + 3],
        // the per-light ambient, untouched. The reasoning held for a light with
        // both: a surface in shadow is still inside the glow, and multiplying it
        // blacks the shadowed side out darker than the light being absent.
        //
        // It does not hold for a light authored as ambient ONLY. The field
        // found exactly that - setLightColor [0,0,0] with setLightAmbient
        // [1,0.8,0.25] - where the diffuse the shadow scales is ZERO, so the
        // shadow multiplies nothing and the light cannot cast at all. Gunfire
        // lights carry a real diffuse and cast normally, which is why the two
        // behaved differently despite both being points.
        //
        // So the ambient takes a PARTIAL shadow: in full shadow it keeps
        // KH_DLS_AMB_KEEP of its value. A pure-ambient light now casts a
        // visible, soft shadow; a light with both keeps the away-facing glow it
        // is there to provide. Neither extreme is right, and the constant is the
        // only honest way to say so.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        acc += (dlGlobal.xyz * dlLights[b + 2].xyz * ndl * khs_sh
              + dlLights[b + 3].xyz * khs_amb) * att;
    }

    return acc * dlGlobal.w;
}

// smf: per-pixel shadow factor from the caller (received world shadows and
// the private sun-depth self term, min-combined - they answer the same
// question at different granularities and must not stack). Sun/moon shading
// for solid meshes (PSMain and PSComposite), opt-in per object via
// lighting0.x.
)HLSL" R"HLSL(
// CHUNK BOUNDARY (26876f). Segment 22 reached 753 bytes of headroom once the
// mesh-side probes landed in it. SPLIT, never trim - the probes are the only
// instruments that have ever reached this path and their notes are why the
// next reader will not re-derive which modes reach which consumer.
// KH_DLSW_MESHLIFT (26876j) - THE PAINT HELPERS, WITH THE CLAMP TAKEN OUT.
//
// 568 shipped as pow(saturate(lc), 0.25) and that saturate voided the result.
// A flashlit surface is HDR - lc runs well above 1 - so saturate clamped every
// lit pixel to exactly 1 and the quarter power returned 1, which made 568 the
// THIRD flat white paint in a row after 562 and 565. It could not have shown
// faint structure whether or not it was there, and its negative means nothing.
//
// Reinhard first, THEN the lift: x/(1+x) maps the whole HDR range into 0..1
// without discarding anything above 1, so a bright surface keeps its internal
// variation instead of being flattened against the ceiling.
float3 KhDlsLift(float3 khlf_c)
{
    return pow(max(khlf_c, 0.0f) / (1.0f + max(khlf_c, 0.0f)), 0.45f);
}

// KH_DLSW_MESHEDGE (26876j, mode 569). The artifact is described as an OUTLINE,
// and an outline is a gradient. This high-passes the tonemapped result with the
// 2x2 quad derivative and amplifies it, so any coherent edge in the mesh's
// shaded colour is drawn as a bright curve on black - no clamp, no flattening,
// and no dependence on the surface's absolute brightness, which is what defeated
// every paint before it. The vest's weave will register too, as fine uniform
// texture; a background silhouette registers as a long coherent line through it
// and the two do not look alike.
float3 KhDlsEdge(float3 khed_c)
{
    const float khed_t = dot(KhDlsLift(khed_c), float3(0.333f, 0.333f, 0.334f));
    const float khed_e = (abs(ddx(khed_t)) + abs(ddy(khed_t))) * 24.0f;
    return float3(saturate(khed_e), saturate(khed_e), saturate(khed_e));
}

float3 ApplyLighting(float3 base, float3 wpos, float3 nrm, float smf)
{
    // KH_DLSW_MESHPROBE twin 1/2 (mode 560). Above the lit gate deliberately:
    // an unlit object still has to be able to show that it claims nothing.
    if (KhDlsMeshDbg() == 560) {
        const float khml_p = KhDlsMeshProbe(wpos, nrm);
        return float3(khml_p, khml_p, khml_p);
    }
    // KH_DLSW_MESHLIGHT (26876e, mode 561) twin 1/2. 560 shows the SHADOW TERM
    // alone; this shows the whole dynamic-light contribution - attenuation,
    // cone, the per-light ambient and KH_DLS_AMB_KEEP included - with the
    // material removed. Together they bracket the artifact: structure visible
    // in 561 but not in 560 is in the dynamic-light SHADING rather than in the
    // shadow lookup, and structure in neither is not in this system at all.
    if (KhDlsMeshDbg() == 561) return DynLights(wpos, nrm);
    // KH_DLSW_MESHFLAT (26876e, mode 562) twin 1/2 - THE TEST THE WORD ASKS FOR.
    //
    // The mesh returns FLAT WHITE. No lighting, no shadow, no normal, no
    // texture, no albedo - one constant, from both shading entry points. Every
    // input that could carry structure is gone, so if the world behind is still
    // discernible through the mesh, nothing being COMPUTED is responsible: the
    // mesh's pixels are being blended with what is behind them, and the fault
    // is coverage or alpha rather than shading. That is what "see-through"
    // literally means, and it has never been tested directly.
    //
    // Mode 519 already showed the artifact survives with every dynamic-light
    // shadow switched off, so the shadow campaign's whole surface area is
    // already excluded. This says whether ANY shading is involved.
    if (KhDlsMeshDbg() == 562) return float3(1.0f, 1.0f, 1.0f);
    // KH_DLSW_MESHSMF (26876f, mode 564) twin 1/2 - THE ONE SCREEN-SPACE INPUT
    // THE MESH KERNELS STILL TAKE.
    //
    // 562 came back a clean flat white silhouette, so the mesh is not being
    // blended with anything and the artifact is in SHADING. 519 already
    // excluded every dynamic-light shadow. What is left in the shading path
    // that could carry the shape of things BEHIND the mesh is smf, and it is
    // the obvious candidate the moment you look at where it comes from: the
    // caller builds it from the engine's received world shadow (khShadowMask
    // and friends, t20-t22) and the mirror stencil, all read at THIS PIXEL in
    // SCREEN SPACE - and the engine's mask does not contain our injected
    // geometry, so at a pixel our mesh covers it describes the world behind it.
    // That is the same class of fault KH_DLSW_SELFMASK fixed for the world
    // pass, on the other side of the fence, and nothing has ever checked it
    // from here.
    //
    // Painted greyscale: WHITE fully lit, BLACK fully shadowed. If the reported
    // outline appears in this, smf is carrying the background and the fix is a
    // self-mask on the mesh receive exactly as stage 4 has. Note this and 561
    // are mutually exclusive - KhDynLightsPBR never reads smf - so whichever of
    // the two shows the structure settles it outright.
    if (KhDlsMeshDbg() == 564) return float3(smf, smf, smf);
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float shadow = smf;   // per-pixel receive + self term (min-combined upstream)
    float3 direct = lighting2.rgb * (ndl * lighting0.w * shadow);
    // KH_DLSW_MESHNODL (26876h, mode 567) twin 1/2 - THE COMPLEMENT OF 561.
    //
    // 561 showed the dynamic-light contribution alone and the field read
    // structure in it. This shows everything EXCEPT it: sun, sky ambient,
    // emissive, the material, the normal map, at full contrast. The two
    // partition the shaded colour exactly, so between them the artifact has
    // nowhere to hide. Structure here means the dynamic lights are innocent and
    // 561's reading was the vest's own weave under a moving light; structure in
    // 561 and NOT here confirms it.
    //
    // This is a real paint and not a flat one, which matters: 562 and 565 came
    // back clean partly because there was nothing in them to be structured. A
    // flat image is a weak negative and this is not one.
    const float3 khal_dl = (KhDlsMeshDbg() == 567) ? float3(0.0f, 0.0f, 0.0f)
                                                   : DynLights(wpos, nrm);
    const float3 khal_out = base * (lightAmb.rgb * lighting0.z + direct + khal_dl);
    // KH_DLSW_MESHLIFT (26876i, mode 568) twin 1/2 - THE SHIPPING PICTURE, LIFTED.
    //
    // A METHODOLOGICAL FIX, NOT A NEW HYPOTHESIS. 562 (flat white), 565 (flat
    // white) and 567 (black at night) all came back "clean", and I read those
    // as eliminations. They are not: a flat or black image has nothing in it
    // that COULD be structured, so it cannot report faint structure either way.
    // Only 564 was a real contrast test, and only that negative should carry
    // weight. The artifact has been visible in exactly the modes where the mesh
    // has tonal range - 0, 519, 561 - which is what you would expect of
    // something faint whatever its cause.
    //
    // So this changes nothing about the shading and only tone-maps and lifts
    // the result, stretching the range where a faint artifact lives while
    // leaving the geometry, the lighting and every input exactly as they ship.
    //
    // 26876i SHIPPED THIS WITH saturate() IN FRONT OF THE LIFT AND THAT VOIDED
    // IT. A flashlit surface is HDR, saturate clamped every lit pixel to 1, and
    // the lift returned 1 - a third flat white paint after 562 and 565, from
    // the exact mistake the note above was written to warn about. Corrected at
    // 26876j to tone-map first (see KhDlsLift). The 26876i reading of 568 is
    // VOID and must not be counted as an elimination.
    if (KhDlsMeshDbg() == 568) return KhDlsLift(khal_out);
    if (KhDlsMeshDbg() == 569) return KhDlsEdge(khal_out);
    return khal_out;
}

)HLSL" R"HLSL(
#if KH_TEXTURED
Texture2D<float4> matDiffuse  : register(t14);
Texture2D<float4> matNormal   : register(t15);
Texture2D<float4> matOrm      : register(t16);
Texture2D<float4> matEmissive : register(t17);
Texture2D<float4> matSpecular : register(t18);
SamplerState matSamp : register(s0);

float4 KhMatFetch(int slot, float2 uv)
{
    if (slot == 0) return matDiffuse.Sample(matSamp, uv);
    if (slot == 1) return matNormal.Sample(matSamp, uv);
    if (slot == 2) return matOrm.Sample(matSamp, uv);
    if (slot == 3) return matEmissive.Sample(matSamp, uv);
    return matSpecular.Sample(matSamp, uv);
}

float KhMatRoute(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetch(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

// KH_MAT_SPLIT_TEXEL (26768) - THE BLEND SPLIT CLASSIFIES BY THE TEXEL'S
// OWN ALPHA: a point fetch at mip 0 through the same route, never the
// filtered sample. The filtered alpha averages a transparent neighbour into
// an opaque texel wherever the footprint touches the window - the window's
// rim at distance (the coarse mip is mostly glass) and along a uv seam
// (the derivative jumps across it, so the hardware picks the coarsest mip
// on exactly that line) - so such texels read below 0.996, left the opaque
// pass, and the part pass, which blends by that same alpha and tests
// against a depth the opaque pass never wrote there, could not cover them:
// the 'mouse bites' through a solid hull. The blend keeps the filtered
// alpha; only the VERDICT is per texel, and both passes (and the sun map,
// and the footprint) reach the same verdict for the same uv by construction.
float4 KhMatFetchTexel(int slot, float2 uv)
{
    uint kmt_w, kmt_h;
    float2 kmt_t = frac(uv);   // the sampler wraps; so does this
    if (slot == 0) { matDiffuse.GetDimensions(kmt_w, kmt_h);  return matDiffuse.Load(int3(int2(kmt_t * float2(kmt_w, kmt_h)), 0)); }
    if (slot == 1) { matNormal.GetDimensions(kmt_w, kmt_h);   return matNormal.Load(int3(int2(kmt_t * float2(kmt_w, kmt_h)), 0)); }
    if (slot == 2) { matOrm.GetDimensions(kmt_w, kmt_h);      return matOrm.Load(int3(int2(kmt_t * float2(kmt_w, kmt_h)), 0)); }
    if (slot == 3) { matEmissive.GetDimensions(kmt_w, kmt_h); return matEmissive.Load(int3(int2(kmt_t * float2(kmt_w, kmt_h)), 0)); }
    matSpecular.GetDimensions(kmt_w, kmt_h);
    return matSpecular.Load(int3(int2(kmt_t * float2(kmt_w, kmt_h)), 0));
}

float KhMatRouteTexel(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetchTexel(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

struct KhMatSurf {
    float3 albedo; float alpha; float3 nrmT; float occ; float rough;
    float metal; float3 emissive; float3 specF0; float gloss; float specOn;
};

KhMatSurf KhSampleMat(float2 uv)
{
    KhMatSurf s;
    int flags = (int)matParams0.x;
    float4 dif = (flags & 1) ? matDiffuse.Sample(matSamp, uv) : float4(1.0f, 1.0f, 1.0f, 1.0f);
    s.albedo = dif.rgb * matParams1.xyz;
    s.alpha = KhMatRoute(matParams3.y, 1.0f, uv);
    s.nrmT = (flags & 2) ? (matNormal.Sample(matSamp, uv).xyz * 2.0f - 1.0f) : float3(0.0f, 0.0f, 1.0f);
    s.nrmT.xy *= matParams0.w;
    s.occ = KhMatRoute(matParams2.z, 1.0f, uv);
    s.rough = KhMatRoute(matParams2.w, matParams1.w, uv);
    s.metal = KhMatRoute(matParams3.x, matParams2.x, uv);
    s.emissive = ((flags & 8) ? matEmissive.Sample(matSamp, uv).rgb : float3(0.0f, 0.0f, 0.0f)) * matParams2.y;
    s.specOn = matParams3.w;
    float4 spc = (flags & 16) ? matSpecular.Sample(matSamp, uv) : float4(0.0f, 0.0f, 0.0f, 0.0f);
    s.specF0 = spc.rgb;
    s.gloss = KhMatRoute(matParams3.z, spc.a, uv);
    return s;
}

// Arithmetic and clamps are the sun path's original lines VERBATIM -
// factoring must not drift the sun result. Shared compact GGX core
// (Cook-Torrance D * G * F / (4 ndv ndl)): ONE implementation for the sun
// term (KhApplyPBR) and the engine dynamic lights (KhDynLightsPBR).
float3 KhGGXSpec(float3 n, float3 v, float3 l, float rough, float3 F0, out float3 outF)
{
    float3 h = normalize(l + v);
    float ndl = saturate(dot(n, l));
    float ndv = max(dot(n, v), 1.0e-4f);
    float ndh = saturate(dot(n, h));
    float vdh = saturate(dot(v, h));
    float aa = rough * rough;
    float a2 = aa * aa;
    float dd = ndh * ndh * (a2 - 1.0f) + 1.0f;
    float D = a2 / max(3.14159265f * dd * dd, 1.0e-6f);
    float kk = (rough + 1.0f) * (rough + 1.0f) * 0.125f;
    float gl = max(ndl, 1.0e-4f);
    float G = (ndv / (ndv * (1.0f - kk) + kk)) * (gl / (gl * (1.0f - kk) + kk));
    outF = F0 + (1.0f - F0) * pow(1.0f - vdh, 5.0f);
    return D * G * outF / max(4.0f * ndv * gl, 1.0e-4f);
}

// Engine dynamic lights on the PBR path (the specular-glints round):
// DynLights' per-light loop VERBATIM - position decode (mode 3 absolute pool
// / 1 camera-relative world / 2 view space), offset + a0/a1/a2 attenuation on
// the scaled distance, the spot cone log/mul/exp pow with its
// degenerate-exponent guard, and the hard range fade - any drift here is a
// wrong-direction glint at night.
float3 KhDynLightsPBR(float3 wpos, float3 nrm, float3 albedo, float3 F0, float rough, float metal)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    int totalN = pointN + (int)dlCtl.z;
    float3 n = normalize(nrm);
    float3 p;
    float specOn = 1.0f;
    float3 v = float3(0.0f, 0.0f, 1.0f);

    if (dlCtl.x >= 2.5f) {
        // mode 3: ABSOLUTE world positions (engine axes) - no camera
        // dependency for the diffuse decode; the view vector alone consumes
        // fxParams0 (zeroed camera = diffuse-only, see above).
        p = wpos;
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) specOn = 0.0f;
        else v = normalize(fxParams0.xyz - wpos);
    } else {
        // modes 1/2 (diagnostics): camera-origin decodes; a zeroed camera
        // cannot decode camera-origin lights - stand down rather than shade
        // garbage (DynLights' own rule).
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }

        // camera-at-origin: the fragment sits at p, the camera at 0.
        v = -p / max(length(p), 1.0e-4f);
    }

    float kdM = 1.0f - saturate(metal);
    float3 acc = float3(0.0f, 0.0f, 0.0f);

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = dlLights[b + 0].xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - dlLights[b + 4].x, 0.0f);
        float att = saturate(1.0f / (dot(dlLights[b + 4].yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - dlLights[b + 5].x) * dlLights[b + 5].y);

        if (i >= pointN) {
            // spot cone: the engine's log/mul/exp pow; the (c > 0) guard
            // stands in for log(0) = -inf -> exp -> 0, and dodges the pow(0,
            // 0) NaN a degenerate exponent would mint.
            float c = saturate((dot(-dlLights[b + 1].xyz, L) - dlLights[b + 1].w) * dlLights[b + 2].w);
            att *= (c > 0.0f) ? pow(c, dlLights[b + 3].w) : 0.0f;
        }

        // KH_DL_SHADOW (26822) - TWIN of the DynLights site above, rule 1.5.
        // Folding the term into diffI shadows the specular lobe with it, since
        // KhGGXSpec is scaled by diffI: a highlight from a blocked light must
        // go with the light. The per-light ambient stays outside, as there.
        const float khs_sh = KhDlsShadow((int)dlLights[b + 5].z - 1, wpos, nrm, 0.0f);
        // KH_DLS_AMBIENT_SHADOW twin (rule 1.5) - see the DynLights site.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        float3 diffI = dlGlobal.xyz * dlLights[b + 2].xyz * ndl * khs_sh;
        float3 lit = albedo * (diffI * kdM + dlLights[b + 3].xyz * khs_amb);

        if (specOn >= 0.5f) {   // uniform branch (mode verdict, not per-light)
            float3 khsF;
            lit += KhGGXSpec(n, v, L, rough, F0, khsF) * diffI;
        }

        acc += lit * att;
    }

    return acc * dlGlobal.w;
}

// KH_PBR_AMBIENT (26879) - WHAT A METAL SEES WHEN THE SUN IS NOT ON IT.
//
// KhApplyPBR's ambient was albedo * lightAmb, nothing else: no ambient
// specular, and no (1 - metal) on the diffuse. A metal has no diffuse albedo,
// so the moment the direct term died (smf 0, overcast, dusk) it went flat
// grey-brown - painted plastic - while the dynamic lights (KhDynLightsPBR)
// already had GGX and looked right. This is the split-sum ambient the direct
// term already implies, in one helper reached by both pixel twins through
// KhApplyPBR (rule 1.5).
//
//   THE DOME. There is no reflection target and this binds none. The only
//   ambient radiance lane whose units are PROVEN against the direct term is
//   lightAmb (the lit block's own ambient, the same block lighting2 comes
//   from). fogSky is (1,1,1,1) at every mesh fill - a validity flag, not
//   gradient points - and fogSkyCol is the fog target, which the 302 probe
//   recorded as arriving unscaled against the lit block; neither is used.
//   So the dome is the ambient over the sky hemisphere and KH_PBR_GROUND of
//   it below the horizon (terrain returns a fraction of the sky's light;
//   0.35 is a look constant like KH_DLS_AMB_KEEP, stated here rather than
//   dressed as a measurement), a short smoothstep across the horizon, and
//   a blur toward the dome MEAN by roughness^2 (a rough surface integrates
//   the dome; a mirror sees one direction). Evaluated along R = reflect(-V,
//   N). No fill site changes and no lane is added.
//
//   THE BRDF. Karis's analytic split-sum fit (UE4 mobile): F0*A + B from
//   roughness and N.V. Cheap, and its known failure is the one the field
//   would report as 'everything looks slightly wet' - that is a wrong A/B
//   or a missing (1 - metal), not a tuning problem.
//
//   ENERGY. Ambient diffuse becomes albedo * amb * (1 - metal) * (1 - F_amb),
//   F_amb the roughness-aware Schlick at N.V, so the two ambient terms share
//   the light instead of stacking. m.occ scales both; smf touches NEITHER -
//   a shadow blocks the sun, not the sky.
//
// vOk is KhDynLightsPBR's rule: fxParams0 carries the camera on the solid
// mesh paths and a zeroed camera cannot give N.V, so the ambient falls back
// to the 26878 form (bit-identical: amb * occ first, then albedo) rather
// than shading from a made-up view vector. Mode 582 forces that fallback -
// the A/B. Definition here, in KhDynLightsPBR's segment, because KhApplyPBR
// calls it and fxc has no linker.
static const float KH_PBR_GROUND = 0.35f;
float3 KhPbrAmbient(float3 khpa_n, float3 khpa_v, bool khpa_vOk, float khpa_rough,
                    float3 khpa_F0, float khpa_metal, float3 khpa_albedo,
                    float3 khpa_amb, float khpa_occ)
{
    const float3 khpa_ambo = khpa_amb * khpa_occ;
    if (!khpa_vOk || KhDlsMeshDbg() == 582) return khpa_albedo * khpa_ambo;

    const float khpa_ndv = saturate(dot(khpa_n, khpa_v));
    // Karis env BRDF (F0 * A + B).
    const float4 khpa_c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 khpa_c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    const float4 khpa_r = khpa_rough * khpa_c0 + khpa_c1;
    const float  khpa_a004 = min(khpa_r.x * khpa_r.x, exp2(-9.28f * khpa_ndv)) * khpa_r.x + khpa_r.y;
    const float2 khpa_AB = float2(-1.04f, 1.04f) * khpa_a004 + khpa_r.zw;
    const float3 khpa_envBRDF = khpa_F0 * khpa_AB.x + khpa_AB.y;

    // The dome along the reflection vector, blurred toward its mean.
    const float3 khpa_R = reflect(-khpa_v, khpa_n);
    const float  khpa_sky = smoothstep(-0.15f, 0.15f, khpa_R.y);
    const float3 khpa_dome = khpa_amb * lerp(KH_PBR_GROUND, 1.0f, khpa_sky);
    const float3 khpa_mean = khpa_amb * (0.5f * (1.0f + KH_PBR_GROUND));
    const float3 khpa_env = lerp(khpa_dome, khpa_mean, khpa_rough * khpa_rough);

    // Roughness-aware Schlick at N.V: what the surface reflects of the sky
    // is what the diffuse does not get.
    const float3 khpa_Fr = max(float3(1.0f - khpa_rough, 1.0f - khpa_rough, 1.0f - khpa_rough), khpa_F0);
    const float3 khpa_Famb = khpa_F0 + (khpa_Fr - khpa_F0) * pow(1.0f - khpa_ndv, 5.0f);

    const float3 khpa_diff = khpa_albedo * khpa_ambo * (1.0f - khpa_metal) * (1.0f - khpa_Famb);
    const float3 khpa_spec = khpa_env * khpa_envBRDF * khpa_occ;
    return khpa_diff + khpa_spec;
}

)HLSL" R"HLSL(   // Compact GGX (Cook-Torrance specular + Lambert diffuse) fed IDENTICAL
float3 KhApplyPBR(KhMatSurf m, float3 wpos, float3 n, float smf)
{
    // KH_DLSW_MESHPROBE twin 2/2 (mode 560) - see the ApplyLighting site.
    if (KhDlsMeshDbg() == 560) {
        const float khmp_p = KhDlsMeshProbe(wpos, n);
        return float3(khmp_p, khmp_p, khmp_p);
    }
    // KH_DLSW_MESHLIGHT twin 2/2 (mode 561). WHITE albedo and a default F0
    // deliberately: the question is whether the reported structure is in the
    // LIGHT or in the MATERIAL, so the material must not be in the picture.
    if (KhDlsMeshDbg() == 561) {
        return KhDynLightsPBR(wpos, n, float3(1.0f, 1.0f, 1.0f),
                              float3(0.04f, 0.04f, 0.04f), 0.5f, 0.0f);
    }
    // KH_DLSW_MESHFLAT twin 2/2 (mode 562) - see the ApplyLighting site. This
    // is the twin that matters: the textured path is the one the vest takes,
    // and it is the one whose normal map and albedo are still in the picture
    // under 561.
    if (KhDlsMeshDbg() == 562) return float3(1.0f, 1.0f, 1.0f);
    // KH_DLSW_MESHSMF twin 2/2 (mode 564) - see the ApplyLighting site. This is
    // the twin the vest takes.
    if (KhDlsMeshDbg() == 564) return float3(smf, smf, smf);
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return m.albedo * m.occ + m.emissive;
    float rough = m.specOn >= 0.5f ? saturate(1.0f - m.gloss) : saturate(m.rough);
    rough = max(rough, 0.045f);
    float3 F0 = m.specOn >= 0.5f ? m.specF0
              : lerp(float3(0.04f, 0.04f, 0.04f), m.albedo, saturate(m.metal));
    float metal = m.specOn >= 0.5f ? 0.0f : saturate(m.metal);
    float3 l = lighting1.xyz;
    float3 v = normalize(fxParams0.xyz - wpos);
    float ndl = saturate(dot(n, l));
    float3 F;
    float3 spec = KhGGXSpec(n, v, l, rough, F0, F);
    float3 kd = (1.0f - F) * (1.0f - metal);
    float3 direct = lighting2.rgb * (lighting0.w * ndl * smf) * (kd * m.albedo + spec);
    // KH_PBR_OVERCAST (26879) - THE SUN THROUGH CLOUD IS A WIDE HIGHLIGHT.
    //
    // Under overcast the lit block's sun colour falls to a fraction of its
    // ambient, so the GGX lobe above - which rides lighting2 - is gone, yet
    // the bright patch of cloud around the sun still puts one broad, soft
    // highlight on a metal. This is that patch: the same GGX toward
    // lighting1, with the roughness floored (an area light, not a point),
    // radiance the AMBIENT's (the cloud is sky, and cannot be brighter than
    // the sky lane says), weighted by how dim the sun is against the
    // ambient. The weight is DERIVED, not a bar (rule 1.84): 1 - sun/amb,
    // saturated, from the two lanes of the one block - it is exactly zero
    // whenever the sun outshines its ambient, so a clear day is bit-for-bit
    // unchanged, and it grows continuously as cloud thickens. Attenuated by
    // ndl and smf like every other direct term: our own casters block the
    // cloud-sun as they block the sun. Refused while the block is cold
    // (lightAmb.w 0: both lanes read 1, the ratio means nothing) and by the
    // zeroed-camera rule below. Mode 583 is the A/B.
    const bool  khov_vOk = dot(fxParams0.xyz, fxParams0.xyz) >= 1.0f;
    const float khov_sun = max(lighting2.r, max(lighting2.g, lighting2.b));
    const float khov_amb = max(lightAmb.r, max(lightAmb.g, lightAmb.b));
    const float khov_w = (lightAmb.w >= 0.5f && khov_vOk && KhDlsMeshDbg() != 583)
                       ? saturate(1.0f - khov_sun / max(khov_amb, 1.0e-4f)) : 0.0f;
    float3 khov = float3(0.0f, 0.0f, 0.0f);
    if (khov_w > 0.0f) {
        float3 khov_F;
        khov = KhGGXSpec(n, v, l, max(rough, 0.6f), F0, khov_F)
             * (lightAmb.rgb * lighting0.z) * (khov_w * ndl * smf);
    }
    // KH_PBR_AMBIENT (26879): the split-sum ambient (diffuse with (1 - metal)
    // and Fresnel, plus the dome's specular) replaces albedo * amb; see the
    // helper. smf is not passed - the sky is not shadowed by our casters.
    const float3 amb = KhPbrAmbient(n, v, khov_vOk, rough, F0, metal, m.albedo,
                                    lightAmb.rgb * lighting0.z, m.occ);
    // Dynamic lights: full radiance from KhDynLightsPBR (specular round) -
    // the Lambert lane inside is numerically the retired m.albedo *
    // DynLights(wpos, n) term at metal 0 (the parity anchor).
    // KH_DLSW_MESHNODL twin 2/2 (mode 567) - see the ApplyLighting site. This
    // is the twin the vest takes.
    const float3 khap_dl = (KhDlsMeshDbg() == 567)
                         ? float3(0.0f, 0.0f, 0.0f)
                         : KhDynLightsPBR(wpos, n, m.albedo, F0, rough, metal);
    const float3 khap_out = amb + khap_dl + direct + khov + m.emissive;
    // KH_DLSW_MESHLIFT twin 2/2 (mode 568) - see the ApplyLighting site. This
    // is the twin the vest takes.
    if (KhDlsMeshDbg() == 568) return KhDlsLift(khap_out);
    if (KhDlsMeshDbg() == 569) return KhDlsEdge(khap_out);
    return khap_out;
}
#endif

)HLSL" R"HLSL(
// CHUNK BOUNDARY (26834). KH_DLS_WORLD's kernel and its rationale are ~10 KB,
// and the segment that used to end here was 8.7 KB of a 16,380-byte MSVC token
// - the two together overflow it (the gate caught this on the first attempt,
// at 18,798). SPLIT, never trim: the boundary goes here rather than the
// explanation being cut down, because the explanation is why the next person
// does not re-derive the albedo-cancellation from scratch.
//
// Segment 20 - DynLights, KhDlsShadow and ApplyLighting - is still the largest
// in this file at 968 bytes of headroom, and still has to be split before
// anything is added to IT.
// ---------------------------------------------------------------------------
// KH_DLS_WORLD (26834) - THE WORLD RECEIVES OUR MESHES' DYNAMIC-LIGHT SHADOWS.
//
// Returns a per-channel MULTIPLY FACTOR for an already-shaded world pixel:
// 1.0 = leave it alone, < 1 = this much of its light was blocked by one of our
// meshes. The caller (PSDlsWorld) emits it under a dest*src blend at the scene
// resolve, which is the one point in the frame where such a write both lands
// after the world is drawn and still reaches the presented image (the whole of
// the 26830-26833 probe campaign; see KH_DLS_PROBE_ATRESOLVE).
//
// WHY A FACTOR AND NOT A SUBTRACTION - THE ALBEDO CANCELS.
//
// The engine's lit shaders are forward and end with, verbatim from the shader
// export (mul r0.xyz, r0.xyzx, r1.yzwy):
//
//     out = albedo * ( skyAmb*shadowMask + sun*NdL + Ldyn )
//
// so subtracting a blocked light's share needs albedo * Lblocked, and there is
// no albedo target to read - the renderer is forward and the 424-shader export
// contains exactly one MRT pixel shader, an 8-way depth decimate. That is what
// the 26829 handoff section 8.2 called the blocker on option B. It dissolves in
// ratio form:
//
//     out' = albedo * (Ltotal - Lblocked) = out * (1 - Lblocked/Ltotal)
//
// The albedo divides out. We never need it, and we never read the scene colour
// either - the factor goes out as the pixel and the blend does the rest, which
// is also why this dodges the "writes into the scene tail are invisible" class
// entirely: we are not reading, we are blending into a target that the probe
// proved composites.
//
// THE NUMERATOR IS EXACT. The per-light loop below is the engine's own dynamic
// light function - confirmed instruction for instruction against the daytime
// disassembly of its lit object shaders (cb11 stride 6, the same offset table,
// the same 1e-4 guards, the same log/mul/exp cone pow, ambient accumulated
// without N.L) - so the contribution we remove is the contribution the engine
// added, and it cancels rather than approximating.
//
// TWIN (rule 1.5). This is the THIRD copy of that loop in this file, beside
// DynLights and KhDynLightsPBR, and the fourth counting kh_dls_atten in the
// C++. It is a copy and not a shared helper because the three differ in what
// they ACCUMULATE, not in how they attenuate: DynLights sums shaded radiance,
// the PBR kernel sums it through a BRDF, and this one needs the shaded AND
// unshaded values side by side to difference them. Any edit to the attenuation
// or the cone in one is an edit to all four. The offsets are the contract:
// [b+0] position, [b+1] spot axis + cone threshold, [b+2] diffuse + cone
// scale, [b+3] ambient + cone exponent, [b+4] offset + (a0,a1,a2),
// [b+5] fade start + inverse width + OUR SHADOW SLOT.
//
// THE DENOMINATOR IS DELIBERATELY GENEROUS. Ltotal adds the engine's sky
// ambient and sun terms to the dynamic total, with the SUN'S OWN SHADOW
// ASSUMED FULLY LIT. We do not know the engine's per-pixel sun shadow here and
// guessing it low would shrink the denominator and over-darken. Overstating it
// can only shrink the factor's departure from 1, so every error in Ltotal
// fails toward UNDER-darkening - the same rule KhDlsShadow already holds, where
// the worst a bug can do is fail to darken. At night, which is the regime this
// feature exists for, the dynamic term dominates and the approximation is
// small; in daylight the sun term swamps it and the factor correctly goes to 1,
// so a lamp's shadow fades out across the day cycle for free rather than
// needing a gate.
//
// EVERY FAILURE PATH RETURNS 1.0, exactly as KhDlsShadow does.
// khw_dbg (KH_DLSW_TEXEL, 26871) carries the first shadowing slot's texel
// geometry out for the instrument in PSDlsWorld: xy = the shadow map uv this
// pixel samples, z = one texel's edge in METRES at this receiver, w = 1 when
// the pixel actually reaches a sample. Census only; it does not touch the
// returned factor.
// khw_nrel (KH_DLSW_NRM, 26876) is the CALLER'S CONFIDENCE IN khw_nrm, 0..1,
// supplied for the same reason khw_zunc is: only the caller knows how its
// normal was obtained. A mesh passes 1 - its normal is interpolated geometry
// and is exact. The world pass passes the agreement between two baselines of
// its depth-derived plane, because a plane fitted to a QUANTISED depth field
// is not merely noisy, it collapses to the view axis inside every quantisation
// plateau (the full mechanism is at KH_DLSW_NRM in PSDlsWorld).
//
// WHERE THE NORMAL IS NOT RESOLVED, N.L IS NOT ALLOWED TO DRIVE ANYTHING. The
// term falls back to 1.0 - the facing prior - rather than to a measured-looking
// number we do not have. That is the honest degenerate answer here: the pixels
// that lose their normal are grazing, low-gradient ground under a lamp, and
// "this surface faces the light" is overwhelmingly the truth there. It is also
// applied to BOTH the numerator's diffuse and the denominator's sun, so the
// ratio moves to the ambient-free limit rather than being pulled one way.
float3 KhDlsWorldFactor(float3 khw_wpos, float3 khw_nrm, float khw_zunc,
                        float khw_nrel, out float4 khw_dbg)
{
    khw_dbg = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if (dlCtl.x < 0.5f) return float3(1.0f, 1.0f, 1.0f);
    // MODE 3 ONLY. Modes 1/2 decode light positions relative to the camera
    // through fxParams0/dlView, lanes this pass does not fill and must not
    // pretend to - a camera-origin decode against a world position would place
    // every light at the wrong end of the scene. The pool is merged in world
    // space for mode 3, which is what the shipping path uses.
    if (dlCtl.x < 2.5f) return float3(1.0f, 1.0f, 1.0f);

    const int khw_pointN = (int)dlCtl.y;
    const int khw_totalN = khw_pointN + (int)dlCtl.z;
    const float3 khw_n = normalize(khw_nrm);

    float3 khw_dyn = float3(0.0f, 0.0f, 0.0f);       // what the lights add here
    float3 khw_blocked = float3(0.0f, 0.0f, 0.0f);   // ... and how much we take back

    [loop] for (int khw_i = 0; khw_i < khw_totalN; ++khw_i) {
        const int khw_b = khw_i * 6;
        float3 khw_L = dlLights[khw_b + 0].xyz - khw_wpos;
        const float khw_dist = length(khw_L);
        khw_L /= khw_dist + 1e-4f;
        const float khw_d = max(khw_dist * dlCtl.w - dlLights[khw_b + 4].x, 0.0f);
        float khw_att = saturate(1.0f / (dot(dlLights[khw_b + 4].yzw,
                                             float3(1.0f, khw_d, khw_d * khw_d)) + 1e-4f));
        khw_att *= 1.0f - saturate((khw_dist * dlCtl.w - dlLights[khw_b + 5].x)
                                   * dlLights[khw_b + 5].y);
        if (khw_i >= khw_pointN) {
            const float khw_c = saturate((dot(-dlLights[khw_b + 1].xyz, khw_L)
                                          - dlLights[khw_b + 1].w) * dlLights[khw_b + 2].w);
            khw_att *= (khw_c > 0.0f) ? pow(khw_c, dlLights[khw_b + 3].w) : 0.0f;
        }
        if (khw_att <= 0.0f) continue;

        // KH_DLSW_NRM: the facing prior at khw_nrel 0, the measured N.L at 1.
        const float  khw_ndl = lerp(1.0f, max(dot(khw_n, khw_L), 0.0f), khw_nrel);
        const float3 khw_diff = dlGlobal.xyz * dlLights[khw_b + 2].xyz * khw_ndl;
        const float3 khw_amb = dlLights[khw_b + 3].xyz;
        khw_dyn += (khw_diff + khw_amb) * khw_att;

        // The slot lane, written by kh_dls_fill_cb's twin in the C++ and read
        // by nothing else. 0 = this light casts no shadow, which is the zeroed
        // default and therefore the safe one.
        const int khw_slot = (int)dlLights[khw_b + 5].z - 1;
        if (khw_slot < 0) continue;
        const float khw_sh = KhDlsShadow(khw_slot, khw_wpos, khw_n, khw_zunc);

        // Gated on the instrument's own modes (26875): at mode 0 this block is
        // pure cost - a KhDlsFaceUV per pixel per light for a value nothing
        // reads. An instrument that bills the shipping path is a regression.
        if (dbgCtl.w >= 544.5f && dbgCtl.w < 546.5f && khw_dbg.w < 0.5f) {
            const float4 khw_dm = dlsMeta[khw_slot];
            if (khw_dm.w > 0.0f) {
                const float3 khw_dp = khw_wpos - khw_dm.xyz;
                const float  khw_dz0 = max(max(abs(khw_dp.x), abs(khw_dp.y)), abs(khw_dp.z));
                uint khw_mw, khw_mh, khw_me;
                khDlsMaps.GetDimensions(khw_mw, khw_mh, khw_me);
                float2 khw_duv; float khw_dz, khw_dsl;
                // KH_DLS_RPDB (26877) widened KhDlsFaceUV. This is the census
                // twin and wants none of the frame - scratch, discarded.
                float3 khw_dfr, khw_dfu; float khw_dfsx;
                const float khw_dfar = khw_dm.w;
                const float khw_dnear = KhDlsNear(khw_dfar);   // KH_DLS_NEAR_CAP twin (26878)
                if (khw_mw >= 2u &&
                    KhDlsFaceUV(khw_slot, khw_dp, khw_dp, khw_dnear, khw_dfar,
                                khw_duv, khw_dz, khw_dsl,
                                khw_dfr, khw_dfu, khw_dfsx)) {
                    khw_dbg = float4(khw_duv.x, khw_duv.y,
                                     2.0f * khw_dz0 / max((float)khw_mw, 1.0f), 1.0f);
                }
            }
        }
        if (khw_sh >= 1.0f) continue;   // fully lit: nothing blocked, and the
                                        // common case - skip the arithmetic
        // Exactly the shading DynLights would have produced with the shadow
        // applied, including the partial ambient a pure-ambient light needs to
        // cast at all (KH_DLS_AMB_KEEP). The difference IS the blocked share.
        const float3 khw_shaded = (khw_diff * khw_sh
                                 + khw_amb * lerp(KH_DLS_AMB_KEEP, 1.0f, khw_sh)) * khw_att;
        khw_blocked += max((khw_diff + khw_amb) * khw_att - khw_shaded, 0.0f);
    }

    khw_dyn *= dlGlobal.w;       // DynLights' trailing global scale, on both
    khw_blocked *= dlGlobal.w;   // sides, so the ratio is scale-invariant
    if (dot(khw_blocked, float3(1.0f, 1.0f, 1.0f)) <= 0.0f) return float3(1.0f, 1.0f, 1.0f);

    // Ltotal: the engine's own combine, sun shadow assumed lit (see above).
    const float3 khw_sky = lightAmb.rgb * lighting0.z;
    // Same weighting as the numerator's N.L, and for the same reason: a normal
    // the caller could not resolve may not band the denominator either. At
    // khw_nrel 0 this reads full sun, which OVERSTATES Ltotal - and overstating
    // Ltotal can only shrink the factor's departure from 1, which is the
    // direction this kernel already fails in everywhere else.
    const float3 khw_sun = lighting2.rgb *
        (lerp(1.0f, max(dot(khw_n, lighting1.xyz), 0.0f), khw_nrel) * lighting0.w);
    const float3 khw_total = khw_dyn + khw_sky + khw_sun;

    return saturate(1.0f - khw_blocked / max(khw_total, 1e-4f));
}
)HLSL" R"HLSL(
struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL;
#if KH_TEXTURED
    float2 uv : TEXCOORD0; float4 tan : TANGENT;   // 48-byte lanes (layout_tex)
#endif
};
struct VSOut { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    // KH_SELF_REL_INTERP: anchor-relative position, metres-scale through the
    // interpolators (world-absolute wpos rides at ~7.5 km where fp32
    // interpolation rounds at +-0.49 mm PER FRAME under camera motion - half
    // a hero texel). The self chain samples with this; wpos stays for
    // fog/range/every absolute consumer.
    float3 wrel : TEXCOORD4;
    // KH_INSTANCING (26762): the object colour rides the interpolators, so
    // the pixel twins read ONE source whether the draw was per object (the
    // VS copies the CB's color lane in) or a batch (the instance stream's
    // own lane). Flat across a triangle by construction - every vertex of
    // a draw carries the same value - so the interpolation is exact.
    float4 icol : TEXCOORD5;
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // world tangent + handedness
#endif
};

// KH_INSTANCING (26762) - THE ONE VERTEX TRANSFORM. The per-object lanes
// the vertex path reads (centre, rebase-relative centre + arm, edge lengths,
// rotation rows) come from the CB on a per-object draw and from the instance
// stream on a batch draw; everything below the lane read is identical, so it
// lives here once and the four entry points (VSMain / VSMainInst in the
// static unit, VSComposite / VSCompositeInst in the composite unit) are
// wrappers that only choose the source. KhRotateR is KhRotate's rotated
// branch over explicit rows; KhObjRows supplies the CB rows with KhRotate's
// unfilled-default rule (objRot0.w = 0 reads as identity), and identity rows
// through KhRotateR are bit-exact with the old early return (x*1 + y*0 + z*0
// is x). The body is VSMain's, expression for expression - the stenVol2.z
// ladder included - so a per-object draw through the wrapper produces the
// same clip position, wpos, wrel and normal as before.
struct VSInst {
    float4 ipos  : TEXCOORD4;    // xyz = world centre (engine axes)
    float4 irel  : TEXCOORD5;    // xyz = centre MINUS the pass's rebase camera (double-subtracted on the CPU); w = 1 armed
    float4 isize : TEXCOORD6;    // xyz = edge lengths (engine axes)
    float4 irot0 : TEXCOORD7;    // engine-axes rotation rows (row-vector), ALWAYS filled (identity when unrotated)
    float4 irot1 : TEXCOORD8;
    float4 irot2 : TEXCOORD9;
    float4 icol  : TEXCOORD10;   // object colour, lifetime envelope applied
};

float3 KhRotateR(float3 p, float3 r0, float3 r1, float3 r2)
{
    return p.x * r0 + p.y * r1 + p.z * r2;
}

void KhObjRows(out float3 r0, out float3 r1, out float3 r2)
{
    if (objRot0.w < 0.5f) {
        r0 = float3(1.0f, 0.0f, 0.0f);
        r1 = float3(0.0f, 1.0f, 0.0f);
        r2 = float3(0.0f, 0.0f, 1.0f);
    } else {
        r0 = objRot0.xyz;
        r1 = objRot1.xyz;
        r2 = objRot2.xyz;
    }
}

void KhVsCore(float3 khvc_lp, float3 khvc_ln, float3 khvc_ctr, float3 khvc_rel, float khvc_relArm,
              float3 khvc_size, float3 khvc_r0, float3 khvc_r1, float3 khvc_r2,
              out float4 khvc_opos, out float3 khvc_owpos, out float3 khvc_owrel, out float3 khvc_onrm)
{
    float3 wp = khvc_ctr + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2);
    // FP32 JITTER REBASE (see centerRel): when armed, transform the
    // CAMERA-RELATIVE position through the REBASED viewProj - the
    // world-absolute fp32 cancellation (the stationary micro-jitter's
    // reducible term) never enters the position path.
    float3 khvTp = (khvc_relArm > 0.5f)
                 ? (khvc_rel + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2))
                 : wp;
    // stenVol2.z selects the vertex path: 0 = the historic viewProj transform
    // (every flush/injection mesh fill at mode 0); 3 = viewProj position with
    // the ENGINE'S depth mapping (the seam-inject fill's mode-0 value, 176
    // alias); 1/2 = the engine's whole cb2 transform (modes 174/175 and the
    // 117/118 engine-view arm). A large-world engine that feeds cb4[4..6]
    // before cb2 may well be handing cb2 CAMERA-RELATIVE world, in which case
    // absolute wp is displaced by the whole camera vector and never wins the
    // depth test - that is why 1/2 are arms, not the default.
    float4x4 khEngVP = float4x4(engBlk[0], engBlk[1], engBlk[2], engBlk[3]);
    float3   khEngP  = (stenVol2.z >= 1.5f) ? khvTp : wp;
    float4   khClip  = mul(float4(khvTp, 1.0f), viewProj);

    // This is SPACE-AGNOSTIC: it cannot be wrong about a frame of reference
    // because it never uses one. TAKE THE ENGINE'S DEPTH MAPPING, NOT ITS
    // POSITION (mode 176).
    if (stenVol2.z >= 2.5f) {
        float3 khC2 = float3(engBlk[0].z, engBlk[1].z, engBlk[2].z);
        float3 khC3 = float3(engBlk[0].w, engBlk[1].w, engBlk[2].w);
        float  khD3 = dot(khC3, khC3);

        if (khD3 > 1.0e-12f) {
            float khM22 = dot(khC2, khC3) / khD3;
            float khM32 = engBlk[3].z - engBlk[3].w * khM22;
            khClip.z = khM22 * khClip.w + khM32;
        }
        khvc_opos = khClip;
    } else {
        khvc_opos = (stenVol2.z >= 0.5f) ? mul(float4(khEngP, 1.0f), khEngVP)
                                         : khClip;
    }
    // The farVis-off pop at max view distance is enforced per fragment in the
    // PS (FAR CONTRACT block) instead of here.
    khvc_owpos = wp;
    // KH_SELF_REL_INTERP: subtract the SAME fp32 anchor the sun matrices
    // subtract - the quantised anchor cancels exactly, and the interpolant
    // leaves at metres scale.
    khvc_owrel = wp - sunOrigin.xyz;
    // Per-axis scale is non-uniform: normals take the inverse scale, then the
    // object rotation (the inverse-transpose of scale-then-rotate for
    // orthonormal R - see kh_set_rotation).
    khvc_onrm = normalize(KhRotateR(khvc_ln / max(khvc_size, float3(1e-4f, 1e-4f, 1e-4f)), khvc_r0, khvc_r1, khvc_r2));
}

// Guarded so the effect unit - whose depthTex owns register t1 - never sees
// the atlas declaration: only the static and composite compiles pass
// KH_RECEIVE_TEX.
#ifdef KH_RECEIVE_TEX
// The engine's shadow atlas (the depth texture its cascade passes render
// into), sampled with the engine's OWN per-cascade world->atlasUV+depth
// transforms harvested from its constant uploads - so this compare is the
// same one the engine performs for its own geometry.
Texture2D<float> shadowAtlas : register(t1);
Texture2D<float> shadowBand0 : register(t4);
Texture2D<float> shadowBand1 : register(t5);
Texture2D<float> shadowBand2 : register(t6);
Texture2D<float> shadowBand3 : register(t7);
Texture2D<float> shadowBand4 : register(t8);
Texture2D<float> shadowBand5 : register(t9);
Texture2D<float> shadowBand6 : register(t12);   // slots 6-7; t10 is free
Texture2D<float> shadowBand7 : register(t13);   // (convergence retired)

// The engine renders CAMERA-RELATIVE and its shadow sampling transforms
// consume that same space: pass camera-relative positions only - absolute
// world coordinates land UVs tens of atlas widths off the map.
void ShadowMapSample(float3 rel, out int cascade, out float occluded)
{
    cascade = -1;
    occluded = 0.0f;
    int n = (int)shadowMeta.x;
    if (n <= 0) return;

    for (int c = 0; c < n; ++c) {
        float4 r0 = shadowMats[c * 3 + 0];
        float4 r1 = shadowMats[c * 3 + 1];
        float4 r2 = shadowMats[c * 3 + 2];
        float u = dot(r0.xyz, rel) + r0.w;
        float v = dot(r1.xyz, rel) + r1.w;
        float z = dot(r2.xyz, rel) + r2.w;

        float4 t = shadowTiles[c];
        // Tiles are sorted finest-first; the first cascade whose tile and
        // depth range contain the point decides.
        if (u < t.x || u > t.z || v < t.y || v > t.w) continue;
        if (z <= 0.001f || z >= 0.999f) continue;

        float px1 = 1.0f / shadowMeta.w;
        float2 uv = clamp(float2(u, v), t.xy + px1, t.zw - px1);
        float2 fpx = uv * shadowMeta.w - 0.5f;
        int2 p0 = int2(floor(fpx));
        float2 fr = frac(fpx);
        float d00 = shadowAtlas.Load(int3(p0, 0));
        float d10 = shadowAtlas.Load(int3(p0 + int2(1, 0), 0));
        float d01 = shadowAtlas.Load(int3(p0 + int2(0, 1), 0));
        float d11 = shadowAtlas.Load(int3(p0 + int2(1, 1), 0));
        float o00 = ((z - d00) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o10 = ((z - d10) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o01 = ((z - d01) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o11 = ((z - d11) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        cascade = c;
        occluded = lerp(lerp(o00, o10, fr.x), lerp(o01, o11, fr.x), fr.y);
        return;
    }
}

float ShadowMapFactor(float3 rel)
{
    int cascade;
    float occluded;
    ShadowMapSample(rel, cascade, occluded);
    return 1.0f - occluded * saturate(lighting2.w);
}

// Each band's view matrix was frozen WITH its matrix and content, so rotation
// is exact.
float BandLoad(int t, int2 px)
{
    return (t == 0) ? shadowBand0.Load(int3(px, 0))
         : (t == 1) ? shadowBand1.Load(int3(px, 0))
         : (t == 2) ? shadowBand2.Load(int3(px, 0))
         : (t == 3) ? shadowBand3.Load(int3(px, 0))
         : (t == 4) ? shadowBand4.Load(int3(px, 0))
         : (t == 5) ? shadowBand5.Load(int3(px, 0))
         : (t == 6) ? shadowBand6.Load(int3(px, 0))
                    : shadowBand7.Load(int3(px, 0));
}

// BILINEAR COMPARE per tap - the engine's sample_c equivalent (the quality
// verdict from the path tint: the band path owns 100% of the shading, and its
// POINT taps printed the shadow map's dithered foliage raw - the stipple, and
// each dither cluster as a circle-with-dot. A)
)HLSL" R"HLSL(   // 2x2 weighted compare resolves every dither cell to its smooth
// coverage fraction, exactly like the engine's 16-tap sample_c tier).
float BandCmpBilin(int t, float2 pos, float z)
{
    float2 f = pos - 0.5f;
    int2 p0 = int2(floor(f));
    float2 fr = frac(f);
    float b00 = ((z - BandLoad(t, p0)) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b10 = ((z - BandLoad(t, p0 + int2(1, 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b01 = ((z - BandLoad(t, p0 + int2(0, 1))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b11 = ((z - BandLoad(t, p0 + int2(1, 1))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    return lerp(lerp(b00, b10, fr.x), lerp(b01, b11, fr.x), fr.y);
}

// The receiver's band, for visual 14. Ledger at the MODE CATALOG.
float ShadowBandIndex(float3 wpos)
{
    float4 p = float4(wpos, 1.0f);
    // q 0 = the slab-gated selection, q 1 = the coverage fallback; a fallback
    // hit returns 8 + slot so the paint can separate recovered from selected.
    // Ledger at KH_BAND_COVER_FALLBACK.
    for (int q = 0; q < 2; ++q) {
    for (int s = 0; s < 8; ++s) {
        if (bandBorder[s].w < 0.5f) continue;
        float vz = dot(p, bandView[s * 3 + 2]);
        if (q == 0 && !(vz >= bandBorder[s].x && vz < bandBorder[s].y)) continue;
        float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
        float u = dot(vp4, bandMat[s * 3 + 0]);
        float v = dot(vp4, bandMat[s * 3 + 1]);
        float z = dot(vp4, bandMat[s * 3 + 2]);
        if (u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f &&
            z > 0.001f && z < 0.999f) return (float)(s + q * 8);
    }
    }
    return -1.0f;
}

)HLSL" R"HLSL(   // CHUNK BOUNDARY - the content probe took this segment to 105%%.
// MSVC caps one string literal token at 16380 bytes; splitting at a function
// boundary costs nothing at runtime. VISUAL 18 - WHAT THE SEALED TILE
// ACTUALLY HOLDS. Debug-only; nothing on the shading path calls this.
float4 ShadowBandContent(float3 wpos)
{
    float4 p = float4(wpos, 1.0f);
    for (int q = 0; q < 2; ++q) {
    for (int s = 0; s < 8; ++s) {
        if (bandBorder[s].w < 0.5f) continue;
        float vz = dot(p, bandView[s * 3 + 2]);
        if (q == 0 && !(vz >= bandBorder[s].x && vz < bandBorder[s].y)) continue;
        float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
        float u = dot(vp4, bandMat[s * 3 + 0]);
        float v = dot(vp4, bandMat[s * 3 + 1]);
        float z = dot(vp4, bandMat[s * 3 + 2]);
        if (!(u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f &&
              z > 0.001f && z < 0.999f)) continue;
        int t = (int)(bandBorder[s].w + 0.5f) - 1;
        int2 px = int2(float2(u, v) * shadowMeta.w);
        // EMPTY = the clear value: nothing was ever rasterised into this
        // texel. shadowMeta.y is the compare sign and names which end that
        // is.
        float khbc_e = 0.0f, khbc_o = 0.0f;
        for (int k = 0; k < 4; ++k) {
            float d = BandLoad(t, px + int2(k & 1, k >> 1));
            bool khbc_c = (shadowMeta.y > 0.0f) ? (d >= 0.999f) : (d <= 0.001f);
            if (khbc_c) khbc_e += 0.25f;
            if ((z - d) * shadowMeta.y > 0.0f) khbc_o += 0.25f;
        }
)HLSL" R"HLSL(   // CHUNK BOUNDARY - the hue table took this segment past
        // 16380 B. Split, do not trim. Statement boundary, costs nothing at
        // runtime. NINE SATURATED HUES, NO BRIGHTNESS ENCODING.
        if (q == 0) {
            if (khbc_e >= 0.999f) return float4(1.0f, 0.0f, 0.0f, 1.0f);   // RED clear texels
            if (khbc_e >  0.001f) return float4(1.0f, 0.5f, 0.0f, 1.0f);   // ORANGE partly clear
            if (khbc_o >  0.5f)   return float4(0.0f, 0.0f, 1.0f, 1.0f);   // BLUE shadowed
            return float4(0.0f, 1.0f, 0.0f, 1.0f);   // GREEN real, lit
        }
        if (khbc_e >= 0.999f) return float4(1.0f, 1.0f, 0.0f, 1.0f);   // YELLOW clear texels
        if (khbc_e >  0.001f) return float4(0.0f, 1.0f, 1.0f, 1.0f);   // CYAN partly clear
        if (khbc_o >  0.5f)   return float4(1.0f, 1.0f, 1.0f, 1.0f);   // WHITE shadowed
        return float4(0.0f, 0.0f, 0.0f, 1.0f);   // BLACK real, lit
    }
    }
    return float4(1.0f, 0.0f, 1.0f, 1.0f);   // MAGENTA no tile at all
}

float ShadowBandFactor(float3 wpos)
{
    // NO [unroll] on these loops (X4575 in this unit); the 'done' flag plus
    // a plain break is the accepted early-out shape - no continue. Slots
    // arrive finest-first; the first containing band wins; bandBorder.w-1
    // names the physical texture.
    float4 p = float4(wpos, 1.0f);
    float occ = -1.0f;
    float khbf_vz = 0.0f;   // the winning band's view-z, for the far fade
    float khbp_w = max(length(ddx(wpos)), length(ddy(wpos)));
    int done = 0;
    int khbf_np = (lighting0.y >= 8.5f && lighting0.y < 9.5f) ? 2 : 1;

    for (int khbf_ps = 0; khbf_ps < khbf_np; ++khbf_ps) {
    if (done != 0) break;
    for (int s = 0; s < 8; ++s) {
        if (done != 0) break;
        if (bandBorder[s].w < 0.5f) { done = 0; }
        else {
            float vz = dot(p, bandView[s * 3 + 2]);

            if (khbf_ps != 0 || (vz >= bandBorder[s].x && vz < bandBorder[s].y)) {
                float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
                float u = dot(vp4, bandMat[s * 3 + 0]);
                float v = dot(vp4, bandMat[s * 3 + 1]);
                float z = dot(vp4, bandMat[s * 3 + 2]);

                if (u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f && z > 0.001f && z < 0.999f) {
                    int t = (int)(bandBorder[s].w + 0.5f) - 1;   // w = 1 + texIndex
                    // Rotated-poisson PCF, the engine's own recipe: a
                    // per-pixel random disk rotation turns fixed-offset
                    // banding into fine noise.
                    bool khbp_old = (lighting0.y >= 48.5f && lighting0.y < 49.5f);
                    float2 khbp_hz = khbp_old ? wpos.xz
                                              : (frac(wpos.xz * 0.015625f) * 64.0f);
                    float ang = frac(sin(dot(khbp_hz, float2(12.9898f, 78.233f))) * 43758.5469f) * 6.2831853f;
                    float ca = cos(ang);
                    float sa = sin(ang);
                    float khbr_tpm = length(bandMat[s * 3 + 0].xyz) * shadowMeta.w;
                    float khbr_tpp = khbp_w * khbr_tpm;
                    float r = 1.3f;
                    if (lighting0.y >= 6.5f && lighting0.y < 7.5f) {   // 215 selects
                        r = clamp(khbr_tpp * 0.5f, 1.3f, 8.0f);
                    }
                    float2 base = float2(u, v) * shadowMeta.w;
                    float acc = 0.0f;

                    for (int k = 0; k < 4; ++k) {
                        float2 d0 =
                            (k == 0) ? float2( 0.974844f, 0.756484f)
                          : (k == 1) ? float2(-0.814100f, 0.914376f)
                          : (k == 2) ? float2( 0.945586f,-0.768907f)
                                     : float2(-0.815442f,-0.879125f);
                        // the DECISION ring is rotation-free - with a
                        // per-pixel rotation, whether the early-out fires at
                        // all was itself random near edges, and the early-out
                        // promoted that dither to full-contrast dots (the
                        // fireflies + the halo specks). The 12 refinement
                        // taps below keep the rotation where it belongs.
                        float2 off = khbp_old
                                   ? float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca)
                                   : d0;
                        acc += BandCmpBilin(t, base + off * r, z);
                    }

                    if (acc >= 3.999f || acc <= 0.001f) {
                        occ = acc >= 3.999f ? 1.0f : 0.0f;   // decisive: engine's early-out
                    } else {
                        for (int k2 = 0; k2 < 12; ++k2) {
                            float2 d0 =
                                (k2 ==  0) ? float2( 0.443233f,-0.975116f)
                              : (k2 ==  1) ? float2(-0.241888f, 0.997065f)
                              : (k2 ==  2) ? float2(-0.915886f, 0.457714f)
                              : (k2 ==  3) ? float2(-0.942016f,-0.399062f)
                              : (k2 ==  4) ? float2(-0.094184f,-0.929389f)
                              : (k2 ==  5) ? float2( 0.791975f, 0.190902f)
                              : (k2 ==  6) ? float2( 0.199841f, 0.786414f)
                              : (k2 ==  7) ? float2( 0.537430f,-0.473734f)
                              : (k2 ==  8) ? float2(-0.264969f,-0.418930f)
                              : (k2 ==  9) ? float2(-0.382775f, 0.276768f)
                              : (k2 == 10) ? float2( 0.344959f, 0.293878f)
                                           : float2( 0.143832f,-0.141008f);
                            float2 off = float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca);
                            acc += BandCmpBilin(t, base + off * r, z);
                        }

                        occ = acc * 0.0625f;   // /16, the engine's own weight
                    }

                    khbf_vz = vz;
                    done = 1;
                }
            }
        }
    }
    }

    // MODE 85 IS REMOVED. A fallback with no fault left to catch is exactly
    // what refuses to keep: "harmless" is not a reason to carry a branch that
    // buys nothing. Ledger at KH_BAND_TELEPORT_TRANSIENT, which was its last
    // stated hope and now records the null.
    if (occ < 0.0f) return 1.0f;   // no band covers this depth: lit
    // There was never a fade to soften it - see the border[2] note at the
    // band struct. FADE AT THE SHADOW VIEW DISTANCE. Operator report: walking
    // slowly toward the cascade's far edge shows the shadow banding in
    // correctly, but at any normal speed it SNAPS into existence, and the
    // same edge is where a fast dolly makes it flicker out.
)HLSL" R"HLSL(   // CHUNK BOUNDARY - the fade-anchor ledger took this segment
    // past 16380 B. Split, do not trim. Arithmetically this barely moves for
    // the measured table - 0.10 x 248.567 = 24.86 m against 0.164 x 120.380 =
    // 19.74 m - which is itself the finding: the estimate was close, and it
    // is now close FOR A REASON and will track a different cascade layout
    // instead of holding a number that happened to suit this one.
    float khbf_mf = 0.0f;
    float khbf_mn = 0.0f;
    for (int khbf_s = 0; khbf_s < 8; ++khbf_s) {
        if (bandBorder[khbf_s].w >= 0.5f && bandBorder[khbf_s].y > khbf_mf) {
            khbf_mf = bandBorder[khbf_s].y;
            khbf_mn = bandBorder[khbf_s].x;
        }
    }
    float khbf_d = khbf_vz;
    if (dot(fxParams0.xyz, fxParams0.xyz) > 1.0f) khbf_d = distance(wpos, fxParams0.xyz);
    if (khbf_mf > 0.0f && !(lighting0.y >= 5.5f && lighting0.y < 6.5f)) {   // 211 reverts
        occ *= saturate((khbf_mf - khbf_d) / max(0.164f * (khbf_mf - khbf_mn), 1.0f));
    }
    return 1.0f - occ * saturate(lighting2.w);
}
#endif

// THE FAULT THIS CLOSES: world geometry carries a blue distance wash our
// meshes never received - visible as a crisp red box against hazed hills.
// ENGINE DISTANCE HAZE (PSC_HazePars, sky CB row 14).
float KhHazeT(float khaz_d, float khaz_wposY, float khaz_camY, float khaz_layerY)
{
    if (hazePars.w < 0.5f) return 1.0f;
    // 's clamp (mode 324) and 's stand-down (mode 323) both remain reachable
    // beneath it.
    if (khaz_camY < khaz_layerY) {
        if (fogBelow.y < 0.5f) return 1.0f;   // 323: stand-down
        float khaz_bf = saturate((khaz_layerY - khaz_camY) /
                                 (max(khaz_wposY - khaz_camY, 0.0f) + 1.0e-5f));
        float khaz_sa = khaz_d * (1.0f - khaz_bf);   // path ABOVE the layer
        if (khaz_sa <= 0.0f) return 1.0f;
        float khaz_dn = khaz_sa + 1.0e-5f;
        float khaz_a2 = (abs(khaz_wposY - khaz_layerY) / khaz_dn) * hazePars.z;
        float khaz_I2 = abs(khaz_a2) < 1.0e-12f
                      ? khaz_sa
                      : (1.0f - exp(-khaz_sa * khaz_a2)) / khaz_a2;
        // minY = min(layerY, fragY) - the export's "min r3.z, r1.y, v8.y"
        float khaz_b2 = exp(-(min(khaz_layerY, khaz_wposY) - hazePars.x) *
                            hazePars.z) * hazePars.y;
        return min(exp(-khaz_I2 * khaz_b2), 1.0f);
    }

    float khaz_t   = saturate((khaz_camY - khaz_layerY) /
                              (max(khaz_camY - khaz_wposY, 0.0f) + 1.0e-5f));
    float khaz_s   = khaz_d * khaz_t;   // path above the layer
    float khaz_den = khaz_s + 1.0e-5f;
    float khaz_hi  = max(khaz_layerY, khaz_wposY);
    float khaz_a   = ((khaz_hi - khaz_camY) / khaz_den) * hazePars.z;   // SIGNED
    // integral of exp(-a x) over [0, s]; the a -> 0 limit is s itself
    float khaz_I   = abs(khaz_a) < 1.0e-12f
                   ? khaz_s
                   : (1.0f - exp(-khaz_s * khaz_a)) / khaz_a;
    float khaz_b   = exp(-(khaz_camY - hazePars.x) * hazePars.z) * hazePars.y;

    return min(exp(-khaz_I * khaz_b), 1.0f);
}

)HLSL"