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
            float khC_o = KhSunSoftT(khC_map, khC_meta.y, khC_u, khC_c.z - khC_meta.z);   // filtered
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
        khc_v = KhCastTier(khSunDepth2, sunVP2, sunMeta2, khc_r, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth3, sunVP3, sunMeta3, khc_r, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth4, sunVP4, sunMeta4, khc_r, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth5, sunVP5, sunMeta5, khc_r, khlf_on, khtb_on, khtb_occ, khtb_w, khc_done);
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

    if (c.z >= 1.0f) return localityMeta.z >= 0.5f
                          ? SunShadowCompareSoft(uv, 1.0f - sunMeta.z) : 0.0f;   // filtered

    // KH_TIER_BLEND: the union answer resolves any carried band edge.
    float khtb_un = SunShadowCompareSoft(uv, z_bias(c.z));   // filtered
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

        float ndl = max(dot(n, L), 0.0f);
        acc += (dlGlobal.xyz * dlLights[b + 2].xyz * ndl + dlLights[b + 3].xyz) * att;
    }

    return acc * dlGlobal.w;
}

// smf: per-pixel shadow factor from the caller (received world shadows and
// the private sun-depth self term, min-combined - they answer the same
// question at different granularities and must not stack). Sun/moon shading
// for solid meshes (PSMain and PSComposite), opt-in per object via
// lighting0.x.
float3 ApplyLighting(float3 base, float3 wpos, float3 nrm, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float shadow = smf;   // per-pixel receive + self term (min-combined upstream)
    float3 direct = lighting2.rgb * (ndl * lighting0.w * shadow);
    return base * (lightAmb.rgb * lighting0.z + direct + DynLights(wpos, nrm));
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

        float ndl = max(dot(n, L), 0.0f);
        float3 diffI = dlGlobal.xyz * dlLights[b + 2].xyz * ndl;
        float3 lit = albedo * (diffI * kdM + dlLights[b + 3].xyz);

        if (specOn >= 0.5f) {   // uniform branch (mode verdict, not per-light)
            float3 khsF;
            lit += KhGGXSpec(n, v, L, rough, F0, khsF) * diffI;
        }

        acc += lit * att;
    }

    return acc * dlGlobal.w;
}

)HLSL" R"HLSL(   // Compact GGX (Cook-Torrance specular + Lambert diffuse) fed IDENTICAL
float3 KhApplyPBR(KhMatSurf m, float3 wpos, float3 n, float smf)
{
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
    float3 amb = lightAmb.rgb * lighting0.z * m.occ;
    // Dynamic lights: full radiance from KhDynLightsPBR (specular round) -
    // the Lambert lane inside is numerically the retired m.albedo *
    // DynLights(wpos, n) term at metal 0 (the parity anchor).
    return m.albedo * amb + KhDynLightsPBR(wpos, n, m.albedo, F0, rough, metal) + direct + m.emissive;
}
#endif

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