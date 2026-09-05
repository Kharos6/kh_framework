// cb.hlsl - the shared prefix, concatenated into every unit by C++ (no
// #include). Any edit changes every unit's shader cache key.

cbuffer CBObj : register(b0)
{
    float4 centerSize;   // xyz = world center (engine space), w = edge length.
    // FP32 jitter rebase: xyz = center minus the pass's rebase origin (the
    // camera); w = 1 arms the rebased vertex transform, whose viewProj is then
    // the rebased matrix. w = 0 keeps the absolute path.
    float4 centerRel;
    float4 color;
    float4 fxParams0;   // Effect parameters [0..3].
    float4 fxParams1;   // Effect parameters [4..7].
    float4 fxMeta;   // x = effect id, y = time (s), z = screen width, w = screen height.
    float4 depthParams;   // x = proj m22, y = proj m32, z = viewport MinDepth, w = MaxDepth.
    float4 sizeAxes;   // xyz = mesh edge lengths (engine axes), w = blend mode id.
    float4 localParams0;   // xyz = mask center (engine space), w = shape (0 sphere, 1 cube).
    float4 localParams1;   // x = falloff (normalized to mean radius), y = localized flag.
    float4 localRadii;   // xyz = mask radii (engine axes).
    float4 bandParams;   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w =
                         // Banded flag.
    // x = lit flag, z = ambient fraction, w = diffuse fraction (read through
    // KhObjLanesCb / KhObjLoad); y unread.
    float4 lighting0;
    float4 shadowMeta2;   // x = far-visibility clamp flag, y = object view-distance cut; zw unread.
    // Engine-axes rotation rows (row-vector): world = center + local.x*R0 +
    // local.y*R1 + local.z*R2.
    float4 objRot0;
    float4 objRot1;
    float4 objRot2;   // objRot0.w = 1 marks a filled matrix; 0 (the zeroed default) reads as identity.
    // x = 1: normal-blend translucent mesh with the scene capture bound this
    // inject - the packing composites in Reinhard space against t3 and writes
    // opaque.
    float4 blendCtl;
    // x = this draw's material table index (base + submesh slot; non-instanced
    // VS), y = the submesh slot (the instanced VS adds it to the instance's own
    // base), z = 1 when x is uniform for every instance, w = per-draw
    // alpha-mode override (>= 0 replaces the table's mode; 3 = the opaque part
    // of a blend split). Zeroed and unread on every untextured fill.
    float4 matCtl;
    float4 khFarSplit;   // Far-keep split: xy = the frame pair.
    float4 fuseMeta;
    float4 fuseStage[12];
    float4 fxParams2;   // Effect parameters [8..11] (C++ twin fx2).
    // x = mode (0 off, 1 camera-relative world, 2 view space, 3 absolute pool),
    // y = point count, z = spot count, w = global distance scale (cb10[2].x).
    float4 dlCtl;
    // xyz = global diffuse multiplier (cb10[3]); w = the script intensity,
    // applied to the whole DynLights sum.
    float4 dlGlobal;
    // View matrix columns (world->view rotation) for mode 2, captured with the
    // light list.
    float4 dlView[3];
    float4 dlLights[192];
};

 cbuffer CBFrame : register(b1)
{
    row_major float4x4 viewProj;   // Rebased on the two mesh passes (see centerRel); absolute elsewhere.
    row_major float4x4 invViewProj;   // Clip -> world (row-vector convention). Camera-relative
                                      // when fxCam.w = 1 (every reader adds fxCam.xyz back:
                                      // KhWorldPosFenced, KhFsFog); absolute when fxCam.w = 0.
    float4 lighting1;   // xyz = unit vector toward the sun/moon (engine axes), w = lighting valid flag.
    // rgb = the engine's sun colour in HDR scene units ((1,1,1) before the
    // first publish), w = shadow-map strength.
    float4 lighting2;
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size
                         // (px).
    float4 shadowTiles[8];   // Per-cascade atlas rect in UV: x0, y0, x1, y1.
    float4 shadowMats[24];   // Per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t].
    // View-paired bands: the engine's decoded receiver contract. bandView rows
    // are the view matrix columns (world->view, row-vector convention):
    // viewPos.k = dot(float4(wpos,1), bandView[slot*3+k]). bandMat rows are
    // PSC_ShadowmapMatrix rows: uvz.k = dot(float4(vp,1), row).
    float4 bandMat[24];
    float4 bandView[24];
    float4 bandBorder[8];   // x = near, y = far, z = fade, w = 0 invalid / 1+texIndex.
    // Cast pass: the meshes are drawn depth-only into the engine's shadow atlas
    // at each cascade pass end, so the world receives their shadows.
    float4 castMat[3];   // This cascade's sampling matrix rows.
    float4 castView[3];   // Matching view columns.
    float4 maskMeta;   // x = view-paired band table valid.
    float4 fogParams;   // x = fogValue, y = fogDecay, z = fogBase, w = enable.
    float4 fogColor;   // rgb = fog color, w = camera altitude ASL.
    row_major float4x4 sunVP;   // World -> private sun-depth clip (row-vector).
    float4 sunMeta;   // x = valid, y = map size (px), z = compare bias, w = strength.
    // x = pair count, y = t2 list armed, z = sun map older than 0.5 s (the
    // filtered-compare gate), w = KH_OCC_N, the cast-occupancy grid's edge in
    // texels (0 reads as 256).
    float4 localityMeta;
    float4 locality[32];   // [2i] = center.xyz (engine), [2i+1] = half extents.xyz.
    // rgb = engine ambient colour (HDR scene units) from the located lighting
    // block, or (1,1,1) before the first lock; w = engine-mode flag.
    float4 lightAmb;
    // The engine's own fog terms from the located block: x = density scale
    // (lane 41), y = fog end distance (lane 48), z = inverse ramp range (lane
    // 49), w = terms valid.
    float4 fogEngine;
    float4 fogSky;   // The sky CB's view-elevation gradient control.
    float4 fogSkyCol;   // The sky CB's fog base colour (row 7).
    float4 hazePars;   // Engine distance haze, the sky CB's row 14.
    // x = origin world X, y = origin world Z (SQF y), z = cell size (m), w =
    // enabled and texture valid.
    float4 thmParams;
    float4 thmMeta;   // x = width (cells), y = height (cells).
    float4 fkVetoMeta;   // x = OBB count (0 = veto dark).
    float4 fkVeto[40];   // 8 OBBs x 5 float4: [center.xyz, 1+slot].
    float4 stenVol;   // xy = the injection's viewport depth range for this epoch; zw = the volume
                      // copy's own dims.
    float4 stenVol2;   // x = transport arm; z = KhVsCore vertex path selector (3 = the seam
                       // prepass); y/w never written.
    row_major float4x4 stenProj;   // The engine's projection (C++ twin sten_proj), paired with engRot.
    row_major float4x4 sunVP2;   // World -> hero sun-depth clip.
    float4 sunMeta2;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP3;   // World -> mid-band sun-depth clip (t26).
    float4 sunMeta3;   // x = valid, y = size, z = bias, w = half-diag.
    row_major float4x4 sunVP4;   // World -> outer-band sun-depth clip (t27).
    float4 sunMeta4;   // x = valid, y = size, z = bias, w = half-diag.
    // mirMeta: x = mirror mask valid, yz = mask dims. sunOrigin: the anchor
    // every sunVP* is relative to (subtract from wpos before transforming; zero
    // = world-absolute); w = the far tier's prefilter arm.
    float4 mirMeta;
    float4 sunOrigin;
    float4 fogBelow;
    // Below the fog layer the engine converges on a different colour: fogUw.rgb
    // = cb0[7], the sky fog colour, shaped by the cb0[17] elevation gradient in
    // fogUwGrad.xyz.
    float4 fogUw;
    float4 fogUwGrad;
    row_major float4x4 snapVp;
    float4 snapMeta;
    float4 snapCam;
    // xyz = the per-tier prefilter arms (hero / mid / outer); the far tier's
    // arm is sunOrigin.w. w unread.
    float4 sunPf;
    // World -> far-band sun-depth clip (t32) + x = valid, y = size, z = bias, w
    // = half-diag.
    row_major float4x4 sunVP5;
    float4 sunMeta5;
    // The world cast's own compare bias per tier (x/y/z/w = hero / mid / outer
    // / far, normalized by that tier's own d2v), separate from sunMeta*.z,
    // which the self kernel also reads and which may not move. Zero = fall back
    // to that tier's sunMeta*.z, which every mesh fill site leaves.
    float4 sunCastBias;
    // x = the union's cast bias (normalized by its own D); 0 = fall back to
    // sunMeta.z.
    float4 sunCastBias2;
    // C++ twins dls_meta / dls_ctl / dls_face_slice / dls_spot_vp.
    float4 dlsMeta[8];     // xyz = light world position, w = far plane (0 = no map).
    float4 dlsCtl[8];      // x = spot flag, y = bias const (m), z = spot slice, w = bias slope.
    // slot * 6 + face. x = slice (-1 = none), y = normal-offset arm (replicated
    // per light).
    float4 dlsFaceSlice[48];
    row_major float4x4 dlsSpotVP[8];
    // The dynamic-light shadows obey the shadow view distance like the sun's.
    // xyz = the pass camera (engine axes), w = clamp(shadowVisibility, 8,
    // 1000); 0 = no fade. Written by kh_dls_fill_cb.
    float4 dlsRange;
    // khPass.xyz = the pass's rebase camera (engine axes), w = 1 arms the
    // bucket vertex path's rebase (twin of centerRel.w). khPassObj.x = the
    // engine object view distance (the farVis-off cut for bucket instances);
    // yzw free. Zero wherever no bucket draws.
    float4 khPass;
    float4 khPassObj;
    // xyz = the camera invViewProj is relative to, w = 1 arms it (every reader
    // adds xyz after the reconstruction); w = 0 = invViewProj is absolute.
    float4 fxCam;
};
 
cbuffer CBEngView : register(b2)
{
    float4 engBlk[15];
};

cbuffer CBEngView1 : register(b3)
{
    float4 engBlk1[15];
};

cbuffer CBEngView2 : register(b4)
{
    float4 engBlk2[15];
};

// The object record buffer (C++ twin KhObjRec, 6 float4), one per live-scene
// slot, read by every bucket vertex shader through the lane's slot
// (VSInst.islot). Engine axes. size.w = farVis flag, rot0.w = 1 (filled),
// rot1.w = lit ambient fraction, rot2.w = lit diffuse fraction; col carries no
// lifetime envelope (the lane's alpha does).
struct KhObjRec { float4 pos; float4 size; float4 rot0; float4 rot1; float4 rot2; float4 col; };
StructuredBuffer<KhObjRec> khObjs : register(t39);

// The per-object lanes a bucket varies per instance and the CB carries per
// draw. Filled by the vertex shader into two flat interpolants (VSOut.iobj0/1)
// and loaded by every mesh pixel shader at entry (KhObjLoad); the lighting /
// far contract / dither reads use these, never lighting0.zw, shadowMeta2.xyz or
// blendCtl.zw directly.
static float khObjAmb = 0.0f;      // lighting0.z twin: base-colour fraction kept in shadow.
static float khObjDif = 0.0f;      // lighting0.w twin: n.L-scaled fraction.
static float khObjFarVis = 0.0f;   // shadowMeta2.x / blendCtl.z twin: far-visibility clamp flag.
static float khObjCut = 0.0f;      // shadowMeta2.y twin: object view-distance cut (m, 0 = off).
static float khObjDither = 0.0f;   // blendCtl.w twin: the LOD crossfade dither for this draw.
void KhObjLoad(float4 khol_a, float4 khol_b)
{
    khObjAmb = khol_a.x;
    khObjDif = khol_a.y;
    khObjFarVis = khol_a.z;
    khObjCut = khol_a.w;
    khObjDither = khol_b.y;
}
// The vertex side: the CB lanes (per-object draws)...
void KhObjLanesCb(out float4 khoc_a, out float4 khoc_b)
{
    khoc_a = float4(lighting0.z, lighting0.w, shadowMeta2.x, shadowMeta2.y);
    khoc_b = float4(0.0f, blendCtl.w, 0.0f, 0.0f);
}
// ...or the record + lane (bucket draws); the cut is the pass's object view
// distance for a farVis-off instance.
void KhObjLanesRec(KhObjRec khor_r, float khor_dither, out float4 khor_a, out float4 khor_b)
{
    khor_a = float4(khor_r.rot1.w, khor_r.rot2.w, khor_r.size.w,
                    (khor_r.size.w > 0.5f) ? 0.0f : khPassObj.x);
    khor_b = float4(0.0f, khor_dither, 0.0f, 0.0f);
}

#define KH_RPDB_GC_M 0.008f
#define KH_HERO_TEXEL_M 0.001f
#define KH_SLOPE_TW(khtw) (min((khtw), KH_HERO_TEXEL_M))
#define KH_RPDB_GC(khtw) (min(8.0f * (khtw), KH_RPDB_GC_M))

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
 
// Heightfield occlusion is marched camera->fragment: per-pixel, temporally
// stable, altitude- and LOD-independent. Do not swap in a screen-space variant.
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
Texture2D<float> khSunDepth2 : register(t25);   // Hero sun map (KH_SUN_HERO_MAP).
Texture2D<float> khSunDepth3 : register(t26);   // Mid cascade band (KH_SUN_CASCADE).
Texture2D<float> khSunDepth4 : register(t27);   // Outer cascade band (KH_SUN_CASCADE).
Texture2D<float> khSunDepth5 : register(t32);   // Far band (KH_SUN_FAR_BAND; t28-t31 taken).
// KH_SELF_PREFILTER: per-band moment pyramids (mu, E[z^2]) at half resolution +
// mips; the self kernel samples them under minification.
Texture2D<float2> khSunPf2 : register(t29);   // Hero.
Texture2D<float2> khSunPf3 : register(t30);
Texture2D<float2> khSunPf4 : register(t31);   // Outer.
Texture2D<float2> khSunPf5 : register(t20);   // Far (KH_FAR_PF; t20 was free in every unit).
SamplerState khPfSamp : register(s1);   // Linear-clamp.
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
Texture2D<float4> khShadowPre : register(t21);
Texture2D<float4> khShadowPost : register(t22);

#define KH_PRIME_V   0.99607843f
#define KH_PRIME_EPS 0.00196078f

int2 KhMaskPx(float2 khsp_xy)
{
    return clamp(int2(khsp_xy), int2(0, 0),
                 int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}

// Guard the denominator, do not clamp it.
float KhStenRatio(float khsp_post, float khsp_pre)
{
    if (khsp_pre <= 0.02f) return 1.0f;
    return saturate(khsp_post / khsp_pre);
}

float KhStenTerm(float2 khsp_xy)
{
    int2 khsp_p = KhMaskPx(khsp_xy);
    float khsp_post = khShadowPost.Load(int3(khsp_p, 0)).r;
    float khsp_pre  = khShadowPre.Load(int3(khsp_p, 0)).r;
    return KhStenRatio(khsp_post, khsp_pre);
}

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
// The fade starts at KH_SUN_FADE_START of the range and completes at 99.5%, so
// every contract built on the 0.995 R rim (tier windows, grid domain,
// eligibility) holds; a short band at the rim pops long shadow tips in and out
// under camera motion.
#define KH_SUN_FADE_START 0.72f
float KhSunRangeFadeAt(float3 khrf_p, float3 khrf_o)
{
    if (mirMeta.w < 0.5f) return 1.0f;
    float3 khrf_v = khrf_p - khrf_o;
    float  khrf_fh = 1.0f - smoothstep(KH_SUN_FADE_START * mirMeta.w, 0.995f * mirMeta.w,
                                       length(khrf_v.xz));
    float  khrf_fy = 1.0f - smoothstep(KH_SUN_FADE_START * mirMeta.w, 0.995f * mirMeta.w,
                                       abs(khrf_v.y));
    return min(khrf_fh, khrf_fy);
}
float KhSunRangeFade(float3 khrf_p)
{
    if (mirMeta.w < 0.5f) return 1.0f;
    // The domain is a vertical cylinder around the camera, not a sphere: from
    // altitude a sphere spends its reach vertically. Horizontal and vertical
    // fade separately over the same radius; the min keeps the rim soft on both.
    // Full until KH_SUN_FADE_START, gone at 99.5% (the 0.5% guard completes
    // before the per-caster cliff at R).
    return KhSunRangeFadeAt(khrf_p, sunOrigin.xyz);
}

int2 KhVolPx(float2 khvp_xy)
{
    return clamp(int2(khvp_xy), int2(0, 0),
                 int2((int)stenVol.z - 1, (int)stenVol.w - 1));
}

// The stencil count at a texel. x24_TYPELESS_G8_UINT puts stencil in .g.
uint KhVolCount(int2 khvc_p)
{
    return khVolSten.Load(int3(khvc_p, 0)).g;
}

bool KhVolShadowed(uint khvd_c)
{
    return khvd_c != 0u && khvd_c < 128u;
}

float KhVolTerm(float2 khvt_raster)
{
    return KhVolShadowed(KhVolCount(KhVolPx(khvt_raster))) ? 0.0f : 1.0f;
}

float KhStenUnit(float2 khsu_raster)
{
    if (stenVol2.x >= 0.5f) return KhVolTerm(khsu_raster);
    return KhStenTerm(khsu_raster);
}

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
    float khcs_o = 0.75f / max(khcs_sz, 1.0f);
    return (KhSunBilinT(khcs_m, khcs_sz, uv, z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2( khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(-khcs_o, 0.0f), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f,  khcs_o), z)
          + KhSunBilinT(khcs_m, khcs_sz, uv + float2(0.0f, -khcs_o), z)) * 0.2f;
}

float SunShadowCompareSoft(float2 uv, float z)  { return KhSunSoftT(khSunDepth, sunMeta.y, uv, z); }

// The cast chain's compare: a 3 x 3 ring of bilinear compares at +-khcw_sp
// texels. The spread follows the receiver footprint (clamped 1..4), so the edge
// is averaged over the texels it covers and a one-texel re-rasterization shift
// under a moving sun is a fraction of the transition.
float KhSunSoftWT(Texture2D<float> khcw_m, float khcw_sz, float2 uv, float z, float khcw_sp)
{
    float khcw_o = khcw_sp / max(khcw_sz, 1.0f);
    float khcw_a = KhSunBilinT(khcw_m, khcw_sz, uv, z);
    [unroll] for (int khcw_j = -1; khcw_j <= 1; ++khcw_j) {
        [unroll] for (int khcw_i = -1; khcw_i <= 1; ++khcw_i) {
            if (khcw_i == 0 && khcw_j == 0) continue;
            khcw_a += KhSunBilinT(khcw_m, khcw_sz, uv + float2((float)khcw_i, (float)khcw_j) * khcw_o, z);
        }
    }
    return khcw_a / 9.0f;
}

// The tier-blend weight over the outer window edge. One curve for every blend
// site (cast chain, self kernel, contact carries); KhJw shares it.
float KhTbW(float khtw_e)
{
    return 1.0f - smoothstep(0.75f, 0.98f, khtw_e);
}
// KH_TIER_BLEND: a finer tier that carried a partial verdict blends with the
// coarser tier resolving it by the finer tier's edge weight.
float KhTbBlend(float khtd_c, float khtd_f, float khtd_w)
{
    return lerp(khtd_c, khtd_f, khtd_w);
}

float KhJw(float khjw_e)
{
    return KhTbW(khjw_e);
}

// One cast tier for the four camera-anchored bands (hero/mid/outer/far). false
// = fall through with the carry state updated in place. Twin contract with
// KhSelfTier's ladder.
float KhCastTier(Texture2D<float> khC_map, float4x4 khC_vp, float4 khC_meta, float3 khC_r,
                 float khC_cb,   // This tier's cast bias; 0 = meta.z.
                 bool khC_last,   // Last camera tier - no edge carry.
                 inout float khtb_occ, inout float khtb_w, out bool khC_done)
{
    khC_done = false;
    if (khC_meta.x >= 0.5f) {
        float4 khC_c = mul(float4(khC_r, 1.0f), khC_vp);
        float2 khC_u = float2(0.5f + 0.5f * khC_c.x, 0.5f - 0.5f * khC_c.y);
        // The receiver footprint in this tier's texels, taken here where
        // control flow is still uniform.
        float2 khC_fw = fwidth(khC_u * khC_meta.y);
        float  khC_sp = clamp(0.5f * max(khC_fw.x, khC_fw.y), 1.0f, 4.0f);
        if (khC_u.x > 0.002f && khC_u.x < 0.998f &&
            khC_u.y > 0.002f && khC_u.y < 0.998f &&
            khC_c.z > 0.0f && khC_c.z < 1.0f) {
            // khC_meta.z is texel-priced (1 mm .. 100 mm across the ladder) and
            // on the cast chain this line is the whole bias, so the far tier
            // would push every world shadow bias / tan(elevation) sun-ward; the
            // capped khC_cb is what stops that.
            float khC_b = khC_cb > 0.0f ? khC_cb : khC_meta.z;
            float khC_o = KhSunSoftWT(khC_map, khC_meta.y, khC_u, khC_c.z - khC_b, khC_sp);
            // Lit authoritative: the tier map is complete.
            if (khtb_occ >= 0.0f) { khC_done = true; return KhTbBlend(khC_o, khtb_occ, khtb_w); }
            if (khC_last) { khC_done = true; return khC_o; }   // The window holds the whole domain.
            float khC_e = max(abs(khC_u.x - 0.5f), abs(khC_u.y - 0.5f)) * 2.0f;
            float khC_w = KhTbW(khC_e);
            if (khC_w >= 0.9999f) { khC_done = true; return khC_o; }
            khtb_occ = khC_o; khtb_w = khC_w;   // Carry into the next tier.
        }
    }
    return 0.0f;
}

float SunShadowOcclusion(float3 wpos)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    // Twin contract: the cast chain here and the self chain carry the identical
    // shape.
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    // The admission test is a hard threshold on occlusion: one ulp below it the
    // band is skipped entirely - no return, no carry, no weight - and the next
    // tier answers at full strength.
    {
        const float3 khc_r = wpos - sunOrigin.xyz;
        bool khc_done;
        float khc_v;
        khc_v = KhCastTier(khSunDepth2, sunVP2, sunMeta2, khc_r, sunCastBias.x, false, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth3, sunVP3, sunMeta3, khc_r, sunCastBias.y, false, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth4, sunVP4, sunMeta4, khc_r, sunCastBias.z, false, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
        khc_v = KhCastTier(khSunDepth5, sunVP5, sunMeta5, khc_r, sunCastBias.w, true, khtb_occ, khtb_w, khc_done);
        if (khc_done) return khc_v;
    }
    float4 c = mul(float4(wpos - sunOrigin.xyz, 1.0f), sunVP);   // Ortho: w = 1 (KH_SUN_ANCHOR).
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    // KH_TIER_BLEND: a carried band verdict resolves against whatever the union
    // answers - including its rejections (0 = lit).
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (c.z <= 0.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;

    const float khcu_b = sunCastBias2.x > 0.0f ? sunCastBias2.x : sunMeta.z;

    if (c.z >= 1.0f) return localityMeta.z >= 0.5f
                          ? SunShadowCompareSoft(uv, 1.0f - khcu_b) : 0.0f;   // Filtered.

    // KH_TIER_BLEND: the union answer resolves any carried band edge.
    float khtb_un = SunShadowCompareSoft(uv, c.z - khcu_b);
    return (khtb_occ >= 0.0f) ? KhTbBlend(khtb_un, khtb_occ, khtb_w) : khtb_un;
}

// Soft compare for the self term: five bilinear taps in a +/-0.75-texel diamond
// (~2.5-texel penumtra). One tap body for all five maps (fxc resolves a
// resource parameter at inlining). The texel is clamped on BOTH sides to the
// map (khst_sz = the map edge in texels): a footprint ring past the far edge
// would feed Load an out-of-range texel, which returns 0 = the nearest depth =
// occluded; the edge texel answers instead, the same rule KhDlsBilin applies.
float KhSelfTapT(Texture2D<float> khst_m, float khst_sz, float2 khst_t, float2 khst_g, float khst_z, float khst_b, float khst_w, float2 khst_o)
{
    float2 khst_tc = khst_t + khst_o - 0.5f;   // Fractional offsets land on the corners.
    float2 khst_f0 = floor(khst_tc);
    float2 khst_fr = khst_tc - khst_f0;
    int2   khst_p0 = int2(khst_f0);
    int2   khst_mx = int2((int)khst_sz - 1, (int)khst_sz - 1);
    float4 khst_c;
    [unroll] for (int khst_k = 0; khst_k < 4; ++khst_k) {
        int2 khst_q = clamp(khst_p0 + int2(khst_k & 1, khst_k >> 1), int2(0, 0), khst_mx);
        float2 khst_d = (float2(khst_q) + 0.5f) - khst_t;
        float khst_e = khst_z + khst_d.x * khst_g.x + khst_d.y * khst_g.y - khst_b;
        float khst_s = khst_m.Load(int3(khst_q, 0));
        khst_c[khst_k] = saturate((khst_e - khst_s) / max(khst_w, 1.0e-9f) + 0.5f);
    }
    return lerp(lerp(khst_c.x, khst_c.y, khst_fr.x),
                lerp(khst_c.z, khst_c.w, khst_fr.x), khst_fr.y);
}

// One cascade of the self ladder: receiver-plane gradient (damped by
// fwidth(n)), normal-offset sampling, a tier-proportional bias floor plus the
// hero-priced slope term, a 3x3 footprint-spread pcf ring, the prefilter
// (mean/variance) blend where the footprint exceeds a texel, and the tier-blend
// carry at the window edge. The window test IS the selection: a point inside
// this tier's window takes this tier's verdict, lit included - letting a lit
// verdict fall through to a coarser tier paints the coarse tier's fatter shadow
// rim outside the fine tier's correct one.
float KhSelfTier(Texture2D<float> khT_map, Texture2D<float2> khT_pf, float4x4 khT_vp, float4 khT_meta,
                 float khT_pfArm,
                 bool khT_last,   // Last camera tier - no edge carry.
                 float3 khwr, float3 n, float ndl, float khno_k, float khgs,
                 inout float khtb_occ, inout float khtb_w, out bool khT_done)
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
            float3 khT_cr = float3(khT_vp[0].x, khT_vp[1].x, khT_vp[2].x);
            float3 khT_cu = float3(khT_vp[0].y, khT_vp[1].y, khT_vp[2].y);
            float khT_iR = length(khT_cr);
            float khT_iD = length(float3(khT_vp[0].z, khT_vp[1].z, khT_vp[2].z));
            float khT_tw = 2.0f / (max(khT_meta.y, 1.0f) * max(khT_iR, 1e-6f));
            float khT_k = khT_tw * khT_iD / max(ndl, 0.02f)
                        * saturate(1.0f - 3.0f * length(fwidth(n)));   // Damped gradient.
            float khT_gc = KH_RPDB_GC(khT_tw) * khT_iD;
            float2 khT_g = clamp(
                float2( dot(n, khT_cr / max(khT_iR, 1e-9f)) * khT_k,
                       -dot(n, khT_cu / max(khT_iR, 1e-9f)) * khT_k),
                -khT_gc, khT_gc);
            float2 khT_t = khT_uv * khT_meta.y;
            float khT_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f), 1.0f, 1.0e4f);
            float khT_b = max(khT_meta.z, 1.5f * khT_tw * khT_iD)   // Tier-proportional floor.
                        + 0.35f * khT_tan * KH_SLOPE_TW(khT_tw) * khT_iD;
            float khT_gs = min(khgs, 4.0f * khT_tw);
            khT_b += khT_gs * khT_iD;
            float2 khT_fw = fwidth(khT_t);
            float khT_sp = (khgs > 4.0f * khT_tw)
                         ? 1.0f
                         : clamp(0.5f * max(khT_fw.x, khT_fw.y), 1.0f, 8.0f);
            float khT_sw = max(2.0f * khT_gs, khT_tw) * khT_iD;   // Clamped slack.
            // The centre and the four edge neighbours first; where those five
            // agree the footprint is uniform and the diagonals cannot move the
            // nine-tap mean, so they are skipped. That holds only while the
            // taps overlap: the cross reaches sp texels out and the diagonals
            // sp * sqrt(2), so past sp = 1 a diagonal edge can leave every
            // cross tap lit while a corner is shadowed - hence the gate: the
            // early-out only at sp = 1, all nine taps on grazing and distant
            // footprints.
            float khT_ctr = KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  0));
            float khT_cr4 = KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0, -1) * khT_sp)
                          + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  0) * khT_sp)
                          + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  0) * khT_sp)
                          + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 0,  1) * khT_sp);
            float khT_un = khT_ctr + khT_cr4;
            float khT_res;
            [branch] if (khT_sp <= 1.5f &&
                         (khT_un >= 4.9995f || khT_un <= 0.0005f)) {
                khT_res = khT_ctr;   // Unanimous: the mean IS the centre.
            } else {
                float khT_rng = khT_cr4
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1, -1) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1, -1) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2(-1,  1) * khT_sp)
                              + KhSelfTapT(khT_map, khT_meta.y, khT_t, khT_g, khT_c.z, khT_b, khT_sw, float2( 1,  1) * khT_sp);
                khT_res = (khT_ctr + khT_rng) / 9.0f;
            }
            if (khT_pfArm >= 0.5f) {
                float khT_ft = max(khT_fw.x, khT_fw.y);
                float khT_pw = (khgs > 4.0f * khT_tw)
                             ? 0.0f : smoothstep(1.0f, 2.0f, khT_ft);
                if (khT_pw > 0.001f) {
                    float khT_lod = log2(max(khT_ft * 0.5f, 1.0f));
                    float2 khT_mv = KhPfMu(khT_pf, khT_uv, khT_meta.y * 0.5f, khT_lod);
                    float khT_vd = (khT_c.z - khT_b) - khT_mv.x;
                    float khT_ww = max(khT_ft, 2.0f) * khT_tw * khT_iD;
                    float khT_var = khT_mv.y - khT_mv.x * khT_mv.x;
                    float khT_s2 = (khT_var >= 2.5e-7f ? max(khT_var, khT_ww * khT_ww)
                                                       : khT_ww * khT_ww);   // Ramp.
                    float khT_vv = khT_vd <= 0.0f ? 1.0f
                                 : saturate(khT_s2 / (khT_s2 + khT_vd * khT_vd));
                    float khT_pfo = 1.0f - saturate((khT_vv - 0.4f) / 0.6f);
                    // khT_ga is the 'both read lit, do not invent shadow'
                    // floor, as a weight (a hard test steps the blend across an
                    // iso-contour - a halo tracking the penumbra).
                    float khT_ga = max(smoothstep(0.0f, 0.002f, khT_res),
                                       smoothstep(0.10f, 0.30f, khT_pfo));
                    // The prefilter may not brighten the taps beyond the taps'
                    // own uncertainty. Chebyshev's p_max is an UPPER bound on
                    // the lit fraction: tight for a caster edge against open
                    // sky, loose where two casters at different depths share
                    // the footprint (their depth spread is variance with no lit
                    // texel behind it - VSM light bleed, a bright line along
                    // every shadow-meets-shadow boundary). Where the nine taps
                    // are unanimous the prefilter has nothing brighter to add;
                    // where they disagree it keeps its weight (4 r (1 - r),
                    // continuous). Darkening is untouched (a thin caster the
                    // taps straddle). Past the spread clamp the bound relaxes.
                    float khT_wt = khT_pw * khT_ga;
                    if (khT_pfo < khT_res) {
                        float khT_unc = max(4.0f * khT_res * (1.0f - khT_res),
                                            smoothstep(16.0f, 32.0f, khT_ft));
                        khT_wt *= khT_unc;
                    }
                    khT_res = lerp(khT_res, khT_pfo, khT_wt);
                }
            }
            // Unconditional: the point is inside this tier's window, so this
            // tier's verdict stands whatever it is. Only the window EDGE
            // consults the next tier, through the same carry the cast chain
            // uses.
            if (khtb_occ >= 0.0f) { khT_done = true; return KhTbBlend(khT_res, khtb_occ, khtb_w); }
            if (khT_last) { khT_done = true; return khT_res; }   // The window holds the whole domain.
            float khT_e = max(abs(khT_uv.x - 0.5f), abs(khT_uv.y - 0.5f)) * 2.0f;
            float khT_bw = KhTbW(khT_e);
            if (khT_bw >= 0.9999f) { khT_done = true; return khT_res; }
            khtb_occ = khT_res; khtb_w = khT_bw;   // Edge band: blend with the coarser tier.
        }
    }
    return 0.0f;
}

// The self term: hero, mid, outer, far tiers, then the union map. Twin contract
// with SunShadowOcclusion (the cast chain): identical carry shape.
float SunShadowOcclusionSelf(float3 wrel, float3 nrm)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    float3 khwr = wrel;
    float khgs = 0.5f * length(fwidth(khwr));
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float khno_k = min(1.0f + 2.0f * sqrt(saturate(1.0f - ndl * ndl))
                              / max(ndl, 0.15f), 2.0f)
                 * saturate(1.0f - 5.0f * length(fwidth(n)));
    float khtb_occ = -1.0f;
    float khtb_w = 0.0f;
    bool  khT_done = false;
    float khT_v;
    khT_v = KhSelfTier(khSunDepth2, khSunPf2, sunVP2, sunMeta2, sunPf.x, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    khT_v = KhSelfTier(khSunDepth3, khSunPf3, sunVP3, sunMeta3, sunPf.y, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    khT_v = KhSelfTier(khSunDepth4, khSunPf4, sunVP4, sunMeta4, sunPf.z, false,
                       khwr, n, ndl, khno_k, khgs,
                       khtb_occ, khtb_w, khT_done);
    if (khT_done) return khT_v;
    if (sunMeta5.x >= 0.5f) {
        khT_v = KhSelfTier(khSunDepth5, khSunPf5, sunVP5, sunMeta5, sunOrigin.w, true,   // KH_FAR_PF.
                           khwr, n, ndl, khno_k, khgs,
                           khtb_occ, khtb_w, khT_done);
        if (khT_done) return khT_v;
    }
    float khsr_iR0 = length(float3(sunVP[0].x, sunVP[1].x, sunVP[2].x));
    float khsr_no = khno_k * 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR0, 1e-6f));
    float4 khsr_c = mul(float4(khwr + n * khsr_no, 1.0f), sunVP);   // Ortho: w = 1.
    float2 khsr_uv = float2(0.5f + 0.5f * khsr_c.x, 0.5f - 0.5f * khsr_c.y);
    if (khsr_uv.x <= 0.002f || khsr_uv.x >= 0.998f ||
        khsr_uv.y <= 0.002f || khsr_uv.y >= 0.998f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    if (khsr_c.z <= 0.0f || khsr_c.z >= 1.0f)
        return (khtb_occ >= 0.0f) ? KhTbBlend(0.0f, khtb_occ, khtb_w) : 0.0f;
    float3 khsr_cr = float3(sunVP[0].x, sunVP[1].x, sunVP[2].x);
    float3 khsr_cu = float3(sunVP[0].y, sunVP[1].y, sunVP[2].y);
    float khsr_iR = length(khsr_cr);
    float khsr_iD = length(float3(sunVP[0].z, sunVP[1].z, sunVP[2].z));
    float khsr_tw = 2.0f / (max(sunMeta.y, 1.0f) * max(khsr_iR, 1e-6f));
    float khsr_k = khsr_tw * khsr_iD / max(ndl, 0.02f)
                 * saturate(1.0f - 3.0f * length(fwidth(n)));
    float khsr_gc = KH_RPDB_GC(khsr_tw) * khsr_iD;   // KH_RPDB_WORLD_CLAMP twin.
    float2 khsr_g = clamp(
        float2( dot(n, khsr_cr / max(khsr_iR, 1e-9f)) * khsr_k,
               -dot(n, khsr_cu / max(khsr_iR, 1e-9f)) * khsr_k),
        -khsr_gc, khsr_gc);
    float2 khsr_t = khsr_uv * sunMeta.y;
    float khsr_tan = clamp(sqrt(saturate(1.0f - ndl * ndl)) / max(ndl, 0.02f),
                           1.0f, 1.0e4f);
    float khsr_b = max(sunMeta.z, 1.5f * khsr_tw * khsr_iD)   // Tier-proportional floor.
                 + 0.35f * khsr_tan * KH_SLOPE_TW(khsr_tw) * khsr_iD;   // KH_SLOPE_WORLD twin.
    float khsr_gs = min(khgs, 4.0f * khsr_tw);
    khsr_b += khsr_gs * khsr_iD;
    float khsr_w = saturate(0.001f / max(khsr_tw, 1e-6f));
    float2 khsr_fw = fwidth(khsr_t);
    float khsr_sp = (khgs > 4.0f * khsr_tw)
                ? 1.0f
                : clamp(0.5f * max(khsr_fw.x, khsr_fw.y), 1.0f, 8.0f);
    float khsr_sw = max(2.0f * khsr_gs, khsr_tw) * khsr_iD;   // Clamped slack.
    float khsr_ctr = KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  0));
    float khsr_rng = KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1, -1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  0) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  0) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2(-1,  1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 0,  1) * khsr_sp)
                   + KhSelfTapT(khSunDepth, sunMeta.y, khsr_t, khsr_g, khsr_c.z, khsr_b, khsr_sw, float2( 1,  1) * khsr_sp);
    float khsr_fd = 1.0f;
    if (sunMeta.x >= 1.5f) {
        float khsr_e = max(abs(khsr_uv.x - 0.5f), abs(khsr_uv.y - 0.5f)) * 2.0f;
        khsr_fd = 1.0f - smoothstep(0.85f, 0.98f, khsr_e);
    }
    float khsr_res = (khsr_ctr + khsr_w * khsr_rng)
                   * (1.0f / (1.0f + 8.0f * khsr_w)) * khsr_fd;
    // The union is reached only when no tier window held the point; its verdict
    // resolves any carried tier-edge blend and stands.
    return (khtb_occ >= 0.0f) ? KhTbBlend(khsr_res, khtb_occ, khtb_w) : khsr_res;
}

float SunShadowFactorSelf(float3 wpos, float3 wrel, float3 nrm)
{
    return 1.0f - SunShadowOcclusionSelf(wrel, nrm) * saturate(sunMeta.w)
                * KhSunRangeFade(wpos);
}

// Our own near clip: 0.05 m sits an order of magnitude below the engine's floor
// of 0.07, so it can never be the binding constraint.
static const float KH_OWN_NEAR = 0.05f;

static const float KH_STEN_FADE = 1.35f;

void ClipOwnNear(float khon_w)
{
    if (khon_w < KH_OWN_NEAR) discard;
}

void ClipEdgeSliver(float3 wpos, float3 nrm)
{
    // Fail-safe: this clip trusts fxParams0.xyz as the camera. The failure
    // direction is 'fireflies possible', never 'geometry lost'.
    if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return;
    float3 khes_dx = ddx(wpos);
    float3 khes_dy = ddy(wpos);
    // Same failure-direction rule: stand the clip down when either derivative
    // sits within ~4 ulps of the coordinate magnitude (2^-23 = 1.2e-7; 16 = 4
    // ulps squared).
    float khes_ulp = max(max(abs(wpos.x), abs(wpos.y)), abs(wpos.z)) * 1.2e-7f;
    float khes_q2 = khes_ulp * khes_ulp * 16.0f;
    if (dot(khes_dx, khes_dx) < khes_q2 || dot(khes_dy, khes_dy) < khes_q2) return;
    // Close-range stand-down: standing near a face's own plane puts a large
    // real face at true grazing incidence, and a firefly test built for distant
    // degenerate slivers must not eat close-up geometry that merely shares the
    // sliver's angle.
    float3 khes_v = fxParams0.xyz - wpos;
    float khes_vl = length(khes_v);
    if (khes_vl < 10.0f) return;
    float3 khes_fn = cross(khes_dx, khes_dy);
    float khes_nv = abs(dot(khes_fn, khes_v / max(khes_vl, 1.0e-6f)))
                  / max(length(khes_fn), 1.0e-12f);
    float khes_deg = length(khes_fn)
                   / max(length(khes_dx) * length(khes_dy), 1.0e-20f);
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
                 ? max(nd3.x, max(nd3.y, nd3.z))   // Cube (Chebyshev).
                 : length(nd3);   // Sphere/ellipsoid.
        m *= 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
    }

    return m;
}

// KH_DL_SHADOW - the dynamic-light shadow term. The cube needs no matrix:
// KH_DLS_BASIS is an axis permutation with signs, so for a light-relative point
// the face is the dominant axis, the depth is that component, and the other two
// are the uv. Every failure path returns 1.0: the worst a bug can do is fail to
// darken.
 
static const float KH_DLS_AMB_KEEP = 0.35f;
// The slope term is priced in texels, not metres, so it scales with range as
// the depth error it covers does. 1.5 texels is the filter's own reach (one
// texel for the receiver's, half for the bilinear footprint), the same 1.5 the
// receiver-normal offset uses. KH_DLS_GRAD_TEXELS clamps the gradient, in
// texel-widths of depth per texel of lateral step.
static const float KH_DLS_GRAD_TEXELS = 8.0f;
// The HLSL twin of kh_dls_near, row for row: the receiver's (a, c) must be
// built to the same near the map was rasterised with. The three constants are
// the C++ ones and change with them.
static const float KH_DLS_NEAR_M     = 0.05f;
static const float KH_DLS_NEAR_RATIO = 300.0f;
static const float KH_DLS_NEAR_CAP_M = 0.25f;
float KhDlsNear(float khn_far)
{
    return min(max(khn_far / KH_DLS_NEAR_RATIO, KH_DLS_NEAR_M), KH_DLS_NEAR_CAP_M);
}
// Receiver-distance fade at the shadow view distance, the sun's own rule
// (KhSunRangeFade) on the light maps; without it a lamp's shadow on the ground
// draws at any range while the sun's stops at shadowVisibility.
float KhDlsRangeFade(float3 khrf_p)
{
    if (dlsRange.w <= 0.0f) return 1.0f;
    const float khrf_d = length(khrf_p - dlsRange.xyz);
    return 1.0f - smoothstep(0.94f * dlsRange.w, 0.995f * dlsRange.w, khrf_d);
}
// Bias derivation, so the next reader can check it: the maps are D32_FLOAT
// (kh_dls_ensure_maps), stored as a + c/z with a = f/(f-n) and c = -nf/(f-n),
// so d(stored)/dz = nf/((f-n) z^2) and one float ulp near stored ~ 1 is 2^-24.

Texture2DArray<float> khDlsMaps : register(t36);

// Parity with the sun filter: KhSunBilinT / KhSunSoftT with the texture swapped
// for an array slice - same bilinear weights, same 0.75-texel cross offset,
// same 5-tap average, same rule that a depth map is compared BEFORE it is
// filtered. The compare is built PER TAP: a single projected reference z is
// only correct for a receiver parallel to the map plane; on a tilted one the
// receiver's true depth differs at every texel of the footprint, so (as
// KhSelfTapT does) the reference is corrected per tap by d.x*g.x + d.y*g.y.
// khb_zb = the receiver's BIASED face-axis depth in metres; khb_tc = the
// RECEIVER's own uv in texels (never the filter's offset tap - the gradient
// origin must stay put while the footprint moves); khb_g = metres of face-axis
// depth per texel along +u / +v; a/c/near = the face projection, applied per
// tap.
float KhDlsBilin(float khb_sz, float2 uv, float khb_slice,
                 float khb_zb, float2 khb_tc, float2 khb_g,
                 float khb_a, float khb_c, float khb_near)
{
    float2 tx = uv * khb_sz - 0.5f;
    float2 f = frac(tx);
    // Floor, not truncate: int2 rounds toward zero while frac is x - floor(x),
    // so they disagree wherever tx < 0, and the outermost texel of every cube
    // face would sample its neighbour instead of itself.
    int2 p0 = int2(floor(tx));
    int khb_s = (int)khb_slice;
    int khb_mx = (int)khb_sz - 1;
    // Tap order 0..3 = (0,0) (1,0) (0,1) (1,1), the o00/o10/o01/o11 the lerp
    // pair below consumes.
    float4 khb_o;
    [unroll] for (int khb_k = 0; khb_k < 4; ++khb_k) {
        int2 khb_q = clamp(p0 + int2(khb_k & 1, khb_k >> 1),
                           int2(0, 0), int2(khb_mx, khb_mx));
        // Texel centres sit at integer + 0.5 in this convention, and khb_tc is
        // in the same one, so the difference is a signed count of texels.
        float2 khb_d = (float2(khb_q) + 0.5f) - khb_tc;
        float khb_zq = khb_zb + khb_d.x * khb_g.x + khb_d.y * khb_g.y;
        float khb_ref = khb_a + khb_c / max(khb_zq, khb_near);
        khb_o[khb_k] = (khb_ref > khDlsMaps.Load(int4(khb_q, khb_s, 0))) ? 1.0f : 0.0f;
    }
    return lerp(lerp(khb_o.x, khb_o.y, f.x), lerp(khb_o.z, khb_o.w, f.x), f.y);
}
 
// The 3 x 3 footprint ring (the sun cast's KhSunSoftWT shape). khs_zb / khs_g /
// a / c / near replace a single pre-projected z (the compare is built per tap);
// khs_tc is computed ONCE here from the unoffset receiver uv and handed to
// every tap - the ring moves the FOOTPRINT and must not move the gradient's
// origin. khs_sp is the receiver footprint in map texels, clamped 1..4, priced
// by the caller from fwidth(wpos) before any per-pixel branch.
float KhDlsSoft(float khs_sz, float2 uv, float khs_slice,
                float khs_zb, float2 khs_g,
                float khs_a, float khs_c, float khs_near, float khs_sp)
{
    const float2 khs_tc = uv * khs_sz;
    float khs_o = khs_sp / max(khs_sz, 1.0f);
    float khs_acc = KhDlsBilin(khs_sz, uv, khs_slice, khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near);
    [unroll] for (int khs_j = -1; khs_j <= 1; ++khs_j) {
        [unroll] for (int khs_i = -1; khs_i <= 1; ++khs_i) {
            if (khs_i == 0 && khs_j == 0) continue;
            khs_acc += KhDlsBilin(khs_sz, uv + float2((float)khs_i, (float)khs_j) * khs_o, khs_slice,
                                  khs_zb, khs_tc, khs_g, khs_a, khs_c, khs_near);
        }
    }
    return khs_acc / 9.0f;
}

// khd_nrm is the receiver's world normal. A mesh shadowing itself is the common
// case for a lamp beside a prop, and a self-compare with only a constant bias
// acnes wherever the surface slants away; this kernel takes the sun kernel's
// three defences (receiver-normal offset, texel-priced slope term,
// receiver-plane gradient).
 
// The face, slice and uv selection, shared so any second reader asks the SAME
// question the shadow lookup asks. Returns false where the lookup would answer
// "lit" without sampling. Also returns the projection axes the receiver-plane
// gradient needs, from the selection itself so they cannot drift from it: khf_r
// / khf_u = the projection's right / up axes, light-relative (world-parallel:
// dlsMeta.xyz is a translation); khf_sx = the lateral scale (1 for a cube face,
// 1/tan(fov/2) for a spot; one texel spans 2*z/(size*sx) metres). khf_sel is
// the UNOFFSET light-relative position and the only input to the face choice;
// khf_p (offset) is what gets projected - choosing the face from the offset
// position lets a receiver within 1.5 texels of an edge flip to the
// neighbouring face and land at its rim (a lit seam along every face boundary).
// The spot branch never selects and ignores khf_sel.
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
        if (khf_cl.w <= 1.0e-6f) return false;          // Behind the spot.
        khf_uv = float2(0.5f + 0.5f * khf_cl.x / khf_cl.w,
                        0.5f - 0.5f * khf_cl.y / khf_cl.w);
        khf_z = khf_cl.w;
        khf_slice = dlsCtl[khf_slot].z;
        // Row-vector convention: clip.x = dot(p, column x), and kh_dls_spot_vp
        // builds column x as right * s, so the column's direction is the right
        // axis and its length the lateral scale.
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
        // Dominant axis = the cube face; st is that face's (right, up) pair
        // from KH_DLS_BASIS (the C++ table's twin). The face is chosen from
        // khf_sel; z, st and the axes are read off khf_p on that face.
        const float3 khf_ap = abs(khf_sel);
        float2 khf_st;
        int khf_face;
        // khf_r / khf_u are the same permutation khf_st applies, written as
        // vectors, reproducing KH_DLS_BASIS row for row.
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
        if (khf_z <= khf_near) return false;            // Inside the near plane.
        khf_uv = float2(0.5f + 0.5f * khf_st.x / khf_z,
                        0.5f - 0.5f * khf_st.y / khf_z);
        khf_slice = dlsFaceSlice[khf_slot * 6 + khf_face].x;
    }

    if (khf_slice < 0.0f) return false;                 // This face has no slice.
    if (khf_z >= khf_far) return false;                 // Past the light's reach.
    // The cube must not refuse the outermost texel of a face (it has no guard
    // band, and the boundary would answer lit one texel wide on every edge).
    // The spot keeps the refusal: outside its one frustum there is no map, and
    // lit is right.
    if (dlsCtl[khf_slot].x >= 0.5f) {
        if (khf_uv.x <= 0.001f || khf_uv.x >= 0.999f ||
            khf_uv.y <= 0.001f || khf_uv.y >= 0.999f) return false;
    } else {
        khf_uv = saturate(khf_uv);
    }
    return true;
}

// The receiver-plane depth gradient: the bias has to cover how much the
// receiver's depth CHANGES ACROSS THE FILTER FOOTPRINT, and on a plane depth is
// an affine function of map position, so it is corrected exactly instead of
// covered. The cube face maps u = 0.5 + 0.5*sx*(p.r)/z, v = 0.5 -
// 0.5*sx*(p.u)/z, and the receiver plane is n.p = d; solving for z gives dz/du
// = -2 (n.r) z^2 / (sx d), dz/dv = +2 (n.u) z^2 / (sx d), per texel the same
// over the map size. Clamped, not floored: d goes to zero as the receiver turns
// edge-on and the plane genuinely spans unbounded depth inside one texel, so
// the gradient is capped at KH_DLS_GRAD_TEXELS (the KH_RPDB_GC idiom) and that
// sliver is allowed to acne. The divide is guarded by MAGNITUDE, keeping d's
// sign (negative facing the light, positive facing away; flipping it would tilt
// the correction the wrong way on back faces).
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
 
// khd_zunc is the receiver's own depth uncertainty in metres, supplied by the
// caller: a mesh passes 0 (interpolated geometry is exact); the world pass
// passes its quantised-depth plateau step, which exceeds the constant bias.
// khd_fwp: the receiver's world footprint per screen pixel,
// length(fwidth(wpos)), priced by the caller BEFORE its light loop (a gradient
// inside a loop with a break does not compile).
float KhDlsShadow(int khd_slot, float3 khd_wpos, float3 khd_nrm, float khd_zunc, float khd_fwp)
{
    if (khd_slot < 0 || khd_slot > 7) return 1.0f;
    float4 khd_meta = dlsMeta[khd_slot];
    if (khd_meta.w <= 0.0f) return 1.0f;               // No map for this light.
    const float khd_rf = KhDlsRangeFade(khd_wpos);
    if (khd_rf <= 0.0f) return 1.0f;

    // Face-axis distance before the offset, to price the texel: one texel of a
    // 90 degree face at distance z is 2z / size.
    const float3 khd_p0 = khd_wpos - khd_meta.xyz;
    const float  khd_z0 = max(max(abs(khd_p0.x), abs(khd_p0.y)), abs(khd_p0.z));
    uint khd_mw, khd_mh, khd_me;
    khDlsMaps.GetDimensions(khd_mw, khd_mh, khd_me);
    // An unbound array reports zero dimensions; the texel price would then
    // divide by max(0, 1) and displace the receiver twice its own distance from
    // the light. Answer lit.
    if (khd_mw < 2u) return 1.0f;
    const float  khd_texel = 2.0f * khd_z0 / max((float)khd_mw, 1.0f);
    const int    khd_fbase = khd_slot * 6;
    const bool   khd_off_on = dlsFaceSlice[khd_fbase].y >= 0.5f;
    const float3 khd_n = normalize(khd_nrm);
    // 1.5 texels: one to clear the texel the receiver sits in, half for the
    // bilinear footprint.
    const float3 khd_p = khd_off_on ? (khd_p0 + khd_n * (khd_texel * 1.5f)) : khd_p0;
    const float  khd_far = khd_meta.w;
    const float  khd_near = KhDlsNear(khd_far);   // KhDlsNear twin of kh_dls_near.
    const float  khd_a = khd_far / (khd_far - khd_near);
    const float  khd_c = -khd_near * khd_far / (khd_far - khd_near);

    float2 khd_uv;
    float  khd_z;      // Receiver distance along the face axis, in metres.
    float  khd_slice;
    float3 khd_fr, khd_fu;   // The projection's axes and lateral scale, for the receiver-plane gradient.
    float  khd_fsx;
    if (!KhDlsFaceUV(khd_slot, khd_p, khd_p0, khd_near, khd_far, khd_uv, khd_z, khd_slice,
                     khd_fr, khd_fu, khd_fsx)) {
        return 1.0f;
    }

    const float  khd_b = dlsCtl[khd_slot].y + dlsCtl[khd_slot].w * khd_z
                       + (khd_off_on ? khd_texel : 0.0f) + khd_zunc;
    const float2 khd_g = KhDlsGrad(khd_p, khd_n, khd_fr, khd_fu, khd_z,
                                   (float)khd_mw, khd_fsx, khd_texel);

    // The sun's own 5-tap soft compare on an array slice. Returns occlusion in
    // the sun's convention (1 = blocked); this kernel's contract is lit, so it
    // is inverted once here. Footprint in map texels at this receiver: a face
    // texel at distance z spans 2z / size, scaled by the projection's lateral.
    const float khd_sp = clamp(0.5f * khd_fwp / max(khd_texel / max(khd_fsx, 1.0e-3f), 1.0e-6f), 1.0f, 4.0f);
    const float khd_occ = KhDlsSoft((float)khd_mw, khd_uv, khd_slice,
                                    khd_z - khd_b, khd_g,
                                    khd_a, khd_c, khd_near, khd_sp);
    return saturate(1.0f - khd_occ * khd_rf);   // Thinned by the range fade, not cut.
}
 
// The strongest dynamic-light occlusion any casting light claims at this point
// (1 lit, 0 fully blocked), for ApplyLighting and KhApplyPBR.
float3 DynLights(float3 wpos, float3 nrm)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // Bounded to the declared array (192 / 6): the C++ fill sites cap at
    // KH_DL_MAX_LIGHTS = 32 already, so this is a hard floor under them, not a
    // second truth.
    int totalN = min(pointN + (int)dlCtl.z, 32);
    float3 n = normalize(nrm);
    float3 p;

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes) - no camera dependency
        // at all; the CPU merged the pool in world space.
        p = wpos;
    } else {
        // Modes 1/2: camera-origin decodes. fxParams0.xyz carries the camera on
        // the solid-mesh paths (the ClipEdgeSliver contract); a zeroed camera
        // cannot decode camera-origin lights - stand down rather than shade
        // garbage.
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }
    }

    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = dlLights[b + 0].xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - dlLights[b + 4].x, 0.0f);
        float att = saturate(1.0f / (dot(dlLights[b + 4].yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - dlLights[b + 5].x) * dlLights[b + 5].y);

        if (i >= pointN) {
            // Spot cone: the engine's log/mul/exp pow; the (c > 0) guard stands
            // in for log(0) = -inf -> exp -> 0, and dodges the pow(0, 0) NaN a
            // degenerate exponent would mint.
            float c = saturate((dot(-dlLights[b + 1].xyz, L) - dlLights[b + 1].w) * dlLights[b + 2].w);
            att *= (c > 0.0f) ? pow(c, dlLights[b + 3].w) : 0.0f;
        }

        // The shadow scales the directional term only. dlLights[b + 3] is the
        // per-light ambient - the away-facing glow that makes A3 lights read on
        // surfaces facing away - and a surface in shadow is still inside that
        // glow. A dim light casts a faint shadow and a bright one a hard shadow
        // for free.
        const float khs_sh = KhDlsShadow((int)dlLights[b + 5].z - 1, wpos, nrm, 0.0f, khs_fwp);
        // A pure-ambient light must still cast: a fraction of the per-light
        // ambient follows the shadow (KH_DLS_AMB_KEEP kept).
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        acc += (dlGlobal.xyz * dlLights[b + 2].xyz * ndl * khs_sh
              + dlLights[b + 3].xyz * khs_amb) * att;
    }

    return acc * dlGlobal.w;
}

// smf: per-pixel shadow factor from the caller (received world shadows and the
// private sun-depth self term, min-combined - they answer the same question at
// different granularities and must not stack).
 
// Reinhard first, then the lift: x/(1+x) maps the whole HDR range into 0..1
// without discarding anything above 1 (a saturate before the lift flattens
// every flashlit surface to 1).
float3 ApplyLighting(float3 base, float3 wpos, float3 nrm, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float3 direct = lighting2.rgb * (ndl * khObjDif * smf);   // Per-pixel receive + self term
                                                              // (min-combined upstream).
    return base * (lightAmb.rgb * khObjAmb + direct + DynLights(wpos, nrm));
}

// The material lanes every reader names as matParams0..3. In the textured
// variants KhMatLoad fills them from the table entry; the untextured variants
// never load and read them at zero (the mirror-mask gate in PSMain /
// PSComposite reads matParams0.y in both).
static float4 matParams0 = 0.0f, matParams1 = 0.0f, matParams2 = 0.0f, matParams3 = 0.0f;

#if KH_TEXTURED
// Every material map is a layer of a texture page (a Texture2DArray of textures
// sharing width, height, format and mip count); the page is bound per draw at
// t14-t18, the layer comes from the material table entry. Materials whose maps
// live in the same pages batch across one instanced draw.
Texture2DArray<float4> matDiffuse  : register(t14);
Texture2DArray<float4> matNormal   : register(t15);
Texture2DArray<float4> matOrm      : register(t16);
Texture2DArray<float4> matEmissive : register(t17);
Texture2DArray<float4> matSpecular : register(t18);
SamplerState matSamp : register(s0);

// One entry per material-set slot, C++ twin KhGpuMat (6 float4). p0..p3 = the
// matParams0..3 lanes (map-bound flags, alpha mode, cutoff, normal strength /
// base colour, roughness / metalness, emissive intensity, occ route, rough
// route / metal route, alpha route, gloss route, spec workflow); lay0 =
// diffuse/normal/orm/emissive layers, lay1.x = specular.
struct KhGpuMat { float4 p0; float4 p1; float4 p2; float4 p3; float4 lay0; float4 lay1; };
StructuredBuffer<KhGpuMat> khMats : register(t38);

// The per-pixel material lanes. KhMatLoad fills them from the table entry once
// per pixel (the index rides the VS interpolant, flat per draw or per
// instance).
static float4 khMatLay0 = 0.0f, khMatLay1 = 0.0f;
void KhMatLoad(uint khml_ix)
{
    KhGpuMat khml_m = khMats[khml_ix];
    matParams0 = khml_m.p0;
    matParams1 = khml_m.p1;
    matParams2 = khml_m.p2;
    matParams3 = khml_m.p3;
    khMatLay0 = khml_m.lay0;
    khMatLay1 = khml_m.lay1;
    if (matCtl.w >= 0.0f) matParams0.y = matCtl.w;   // The draw's alpha-mode override.
}
float KhMatLayer(int slot)
{
    if (slot == 0) return khMatLay0.x;
    if (slot == 1) return khMatLay0.y;
    if (slot == 2) return khMatLay0.z;
    if (slot == 3) return khMatLay0.w;
    return khMatLay1.x;
}

float4 KhMatFetch(int slot, float2 uv)
{
    float3 khmf_c = float3(uv, KhMatLayer(slot));
    if (slot == 0) return matDiffuse.Sample(matSamp, khmf_c);
    if (slot == 1) return matNormal.Sample(matSamp, khmf_c);
    if (slot == 2) return matOrm.Sample(matSamp, khmf_c);
    if (slot == 3) return matEmissive.Sample(matSamp, khmf_c);
    return matSpecular.Sample(matSamp, khmf_c);
}

float KhMatRoute(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetch(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

// The blend split classifies by the texel's own alpha: a point fetch at mip 0
// through the same route, never the filtered sample. The filtered alpha
// averages a transparent neighbour into an opaque texel wherever the footprint
// touches the window (the rim at distance, a uv seam where the hardware picks
// the coarsest mip), so such texels would leave the opaque pass and the part
// pass could not cover them - bites through a solid hull.
float4 KhMatFetchTexel(int slot, float2 uv)
{
    uint kmt_w, kmt_h, kmt_n;
    float2 kmt_t = frac(uv);   // The sampler wraps; so does this.
    int kmt_l = (int)KhMatLayer(slot);
    if (slot == 0) { matDiffuse.GetDimensions(kmt_w, kmt_h, kmt_n);  return matDiffuse.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 1) { matNormal.GetDimensions(kmt_w, kmt_h, kmt_n);   return matNormal.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 2) { matOrm.GetDimensions(kmt_w, kmt_h, kmt_n);      return matOrm.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    if (slot == 3) { matEmissive.GetDimensions(kmt_w, kmt_h, kmt_n); return matEmissive.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0)); }
    matSpecular.GetDimensions(kmt_w, kmt_h, kmt_n);
    return matSpecular.Load(int4(int2(kmt_t * float2(kmt_w, kmt_h)), kmt_l, 0));
}

float KhMatRouteTexel(float route, float fallback, float2 uv)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatFetchTexel(r >> 2, uv);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

// Why the surface taps every map, always, and once. The matParams / layer lanes
// are per-pixel (KhMatLoad fills them from khMats[matIx], an interpolant the
// compiler cannot prove uniform), so every branch on them is divergent flow. A
// FILTERED sample needs implicit derivatives, which are unavailable in
// divergent flow, so fxc hoists every arm of a slot chain at compile time: five
// fetches per KhMatFetch call, ~29 per textured pixel. One tap set selected by
// ALU is bounded at five fetches per pixel. Tapping a slot whose map is absent
// costs nothing (a null SRV reads zero; the flag and route tests still decide
// what the value means). This applies to the FILTERED path only:
// KhMatFetchTexel's chain is Loads, which carry no derivative, so it is left as
// is, as are KhMatFetch / KhMatRoute (their callers fetch once).
struct KhMatTaps { float4 t0; float4 t1; float4 t2; float4 t3; float4 t4; };

KhMatTaps KhMatTapAll(float2 uv)
{
    KhMatTaps khmt;
    khmt.t0 = matDiffuse.Sample(matSamp, float3(uv, khMatLay0.x));
    khmt.t1 = matNormal.Sample(matSamp, float3(uv, khMatLay0.y));
    khmt.t2 = matOrm.Sample(matSamp, float3(uv, khMatLay0.z));
    khmt.t3 = matEmissive.Sample(matSamp, float3(uv, khMatLay0.w));
    khmt.t4 = matSpecular.Sample(matSamp, float3(uv, khMatLay1.x));
    return khmt;
}

// Slot select over a taken tap set - a chain of movc, no fetch, no flow. Slot
// numbering is KhMatFetch's.
float4 KhMatPick(KhMatTaps khmp, int slot)
{
    return slot == 0 ? khmp.t0
         : slot == 1 ? khmp.t1
         : slot == 2 ? khmp.t2
         : slot == 3 ? khmp.t3
                     : khmp.t4;
}

// KhMatRoute's decode over an already-taken tap set: same lane encoding (slot =
// r >> 2, channel = r & 3), same negative-is-unrouted fallback, no fetch.
float KhMatRouteTap(KhMatTaps khmr, float route, float fallback)
{
    int r = (int)route;
    if (r < 0) return fallback;
    float4 s = KhMatPick(khmr, r >> 2);
    int c = r & 3;
    return c == 0 ? s.r : c == 1 ? s.g : c == 2 ? s.b : s.a;
}

struct KhMatSurf {
    float3 albedo; float alpha; float3 nrmT; float occ; float rough;
    float metal; float3 emissive; float3 specF0; float gloss; float specOn;
};

// One tap set for the whole surface; the flag guards and the five routes are
// selects over it. The guards still decide what an absent map means (white
// diffuse, flat normal, no emissive, no specular), not whether a fetch happens.
KhMatSurf KhSampleMat(float2 uv)
{
    KhMatSurf s;
    KhMatTaps khsm_m = KhMatTapAll(uv);
    int flags = (int)matParams0.x;
    float4 dif = (flags & 1) ? khsm_m.t0 : float4(1.0f, 1.0f, 1.0f, 1.0f);
    s.albedo = dif.rgb * matParams1.xyz;
    s.alpha = KhMatRouteTap(khsm_m, matParams3.y, 1.0f);
    s.nrmT = (flags & 2) ? (khsm_m.t1.xyz * 2.0f - 1.0f) : float3(0.0f, 0.0f, 1.0f);
    s.nrmT.xy *= matParams0.w;
    s.occ = KhMatRouteTap(khsm_m, matParams2.z, 1.0f);
    s.rough = KhMatRouteTap(khsm_m, matParams2.w, matParams1.w);
    s.metal = KhMatRouteTap(khsm_m, matParams3.x, matParams2.x);
    s.emissive = ((flags & 8) ? khsm_m.t3.rgb : float3(0.0f, 0.0f, 0.0f)) * matParams2.y;
    s.specOn = matParams3.w;
    float4 spc = (flags & 16) ? khsm_m.t4 : float4(0.0f, 0.0f, 0.0f, 0.0f);
    s.specF0 = spc.rgb;
    s.gloss = KhMatRouteTap(khsm_m, matParams3.z, spc.a);
    return s;
}

// Shared compact GGX core (Cook-Torrance D * G * f / (4 ndv ndl)) for the sun
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

// Engine dynamic lights on the PBR path: modes 1 camera-relative world / 2 view
// space / 3 absolute, offset + a0/a1/a2 attenuation on the scaled distance, the
// spot cone log/mul/exp pow with its degenerate-exponent guard, and the hard
// range fade.
float3 KhDynLightsPBR(float3 wpos, float3 nrm, float3 albedo, float3 F0, float rough, float metal)
{
    if (dlCtl.x < 0.5f) return float3(0.0f, 0.0f, 0.0f);
    int pointN = (int)dlCtl.y;
    // Bounded to the declared array (192 / 6): the C++ fill sites cap at
    // KH_DL_MAX_LIGHTS = 32 already, so this is a hard floor under them, not a
    // second truth.
    int totalN = min(pointN + (int)dlCtl.z, 32);
    float3 n = normalize(nrm);
    float3 p;
    float specOn = 1.0f;
    float3 v = float3(0.0f, 0.0f, 1.0f);

    if (dlCtl.x >= 2.5f) {
        // Mode 3: absolute world positions (engine axes); the view vector alone
        // consumes fxParams0 (zeroed camera = diffuse-only).
        p = wpos;
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) specOn = 0.0f;
        else v = normalize(fxParams0.xyz - wpos);
    } else {
        if (dot(fxParams0.xyz, fxParams0.xyz) < 1.0f) return float3(0.0f, 0.0f, 0.0f);
        p = wpos - fxParams0.xyz;

        if (dlCtl.x >= 1.5f) {
            p = float3(dot(p, dlView[0].xyz), dot(p, dlView[1].xyz), dot(p, dlView[2].xyz));
            n = float3(dot(n, dlView[0].xyz), dot(n, dlView[1].xyz), dot(n, dlView[2].xyz));
        }

        // Camera-at-origin: the fragment sits at p, the camera at 0.
        v = -p / max(length(p), 1.0e-4f);
    }

    float kdM = 1.0f - saturate(metal);
    float3 acc = float3(0.0f, 0.0f, 0.0f);
    const float khs_fwp = length(fwidth(wpos));   // Footprint priced outside the loop.

    [loop] for (int i = 0; i < totalN; ++i) {
        int b = i * 6;
        float3 L = dlLights[b + 0].xyz - p;
        float dist = length(L);
        L /= dist + 1e-4f;
        float d = max(dist * dlCtl.w - dlLights[b + 4].x, 0.0f);
        float att = saturate(1.0f / (dot(dlLights[b + 4].yzw, float3(1.0f, d, d * d)) + 1e-4f));
        att *= 1.0f - saturate((dist * dlCtl.w - dlLights[b + 5].x) * dlLights[b + 5].y);

        if (i >= pointN) {
            // Spot cone: the engine's log/mul/exp pow; the (c > 0) guard stands
            // in for log(0) = -inf -> exp -> 0, and dodges the pow(0, 0) NaN a
            // degenerate exponent would mint.
            float c = saturate((dot(-dlLights[b + 1].xyz, L) - dlLights[b + 1].w) * dlLights[b + 2].w);
            att *= (c > 0.0f) ? pow(c, dlLights[b + 3].w) : 0.0f;
        }

        // Twin of the DynLights site. Folding the term into diffI shadows the
        // specular lobe with it (khGGXSpec is scaled by diffI): a highlight
        // from a blocked light goes with the light. The per-light ambient stays
        // outside.
        const float khs_sh = KhDlsShadow((int)dlLights[b + 5].z - 1, wpos, nrm, 0.0f, khs_fwp);
        // Twin of the DynLights site.
        const float khs_amb = lerp(KH_DLS_AMB_KEEP, 1.0f, khs_sh);
        float ndl = max(dot(n, L), 0.0f);
        float3 diffI = dlGlobal.xyz * dlLights[b + 2].xyz * ndl * khs_sh;
        float3 lit = albedo * (diffI * kdM + dlLights[b + 3].xyz * khs_amb);

        if (specOn >= 0.5f) {   // Uniform branch (mode verdict, not per-light).
            float3 khsF;
            lit += KhGGXSpec(n, v, L, rough, F0, khsF) * diffI;
        }

        acc += lit * att;
    }

    return acc * dlGlobal.w;
}

// The split-sum ambient the direct term implies: ambient specular from the
// dome, (1 - metal) on the diffuse - a metal has no diffuse albedo, so without
// this it goes flat grey-brown the moment the direct term dies (shadow,
// overcast, dusk). There is no reflection target; the dome is analytic.
static const float KH_PBR_GROUND = 0.35f;
float3 KhPbrAmbient(float3 khpa_n, float3 khpa_v, bool khpa_vOk, float khpa_rough,
                    float3 khpa_F0, float khpa_metal, float3 khpa_albedo,
                    float3 khpa_amb, float khpa_occ)
{
    const float3 khpa_ambo = khpa_amb * khpa_occ;
    if (!khpa_vOk) return khpa_albedo * khpa_ambo;

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

    // Roughness-aware Schlick at N.V: what the surface reflects of the sky is
    // what the diffuse does not get.
    const float3 khpa_Fr = max(float3(1.0f - khpa_rough, 1.0f - khpa_rough, 1.0f - khpa_rough), khpa_F0);
    const float3 khpa_Famb = khpa_F0 + (khpa_Fr - khpa_F0) * pow(1.0f - khpa_ndv, 5.0f);

    const float3 khpa_diff = khpa_albedo * khpa_ambo * (1.0f - khpa_metal) * (1.0f - khpa_Famb);
    const float3 khpa_spec = khpa_env * khpa_envBRDF * khpa_occ;
    return khpa_diff + khpa_spec;
}

    // Compact GGX (Cook-Torrance specular + Lambert diffuse).
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
    // Both sun terms (direct here, the overcast lobe below) carry ndl * smf;
    // when it is 0 the GGX lobes that feed only them are not evaluated. Exact.
    const bool khsd_lit = ndl * smf > 0.0f;
    float3 direct = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit) {
        float3 F;
        float3 spec = KhGGXSpec(n, v, l, rough, F0, F);
        float3 kd = (1.0f - F) * (1.0f - metal);
        direct = lighting2.rgb * (khObjDif * ndl * smf) * (kd * m.albedo + spec);
    }
    // The sun through cloud is a wide highlight: under overcast the sun colour
    // falls to a fraction of the ambient and the GGX lobe above is gone, yet
    // the bright patch of cloud around the sun still puts one broad, soft
    // highlight on a metal. This is that patch: the same GGX toward lighting1,
    // roughness floored (an area light), radiance the ambient's, weighted by
    // how dim the sun is against the ambient.
    const bool  khov_vOk = dot(fxParams0.xyz, fxParams0.xyz) >= 1.0f;
    const float khov_sun = max(lighting2.r, max(lighting2.g, lighting2.b));
    const float khov_amb = max(lightAmb.r, max(lightAmb.g, lightAmb.b));
    const float khov_w = (lightAmb.w >= 0.5f && khov_vOk)
                       ? saturate(1.0f - khov_sun / max(khov_amb, 1.0e-4f)) : 0.0f;
    float3 khov = float3(0.0f, 0.0f, 0.0f);
    if (khsd_lit && khov_w > 0.0f) {   // KH_SUN_SKIP_DARK.
        float3 khov_F;
        khov = KhGGXSpec(n, v, l, max(rough, 0.6f), F0, khov_F)
             * (lightAmb.rgb * khObjAmb) * (khov_w * ndl * smf);
    }
    // The split-sum ambient replaces albedo * amb. smf is not passed - the sky
    // is not shadowed by our casters.
    const float3 amb = KhPbrAmbient(n, v, khov_vOk, rough, F0, metal, m.albedo,
                                    lightAmb.rgb * khObjAmb, m.occ);
    return amb + KhDynLightsPBR(wpos, n, m.albedo, F0, rough, metal) + direct + khov + m.emissive;
}
#endif

// KH_DLS_WORLD: the world receives our meshes' dynamic-light shadows. Returns a
// per-channel MULTIPLY FACTOR for an already-shaded world pixel (1 = leave
// alone, < 1 = this much of its light was blocked by one of our meshes); the
// caller emits it under a dest*src blend at the scene resolve. Why a factor:
// the engine's lit shaders are forward and end with out = albedo *
// (skyAmb*shadowMask + sun*NdL + Ldyn), and there is no albedo target to read;
// in ratio form out' = out * (1 - Lblocked/Ltotal) the albedo divides out. THE
// NUMERATOR IS EXACT: the per-light loop below is the engine's own dynamic
// light function instruction for instruction (cb11 stride 6, the same offset
// table, the same 1e-4 guards, the same log/mul/exp cone pow, ambient
// accumulated without N.L). This is the third copy of that loop in this file
// (beside DynLights and KhDynLightsPBR, plus kh_dls_atten in the C++); they
// differ in what they ACCUMULATE, not in how they attenuate, so any edit to the
// attenuation or the cone in one is an edit to all four. Offsets: [b+0]
// position, [b+1] spot axis + cone threshold, [b+2] diffuse + cone scale, [b+3]
// ambient + cone exponent, [b+4] offset + (a0,a1,a2), [b+5] fade start +
// inverse width + OUR SHADOW SLOT. THE DENOMINATOR IS GENEROUS: Ltotal adds the
// engine's sky ambient and sun with the sun's own shadow assumed fully lit, so
// every error fails toward UNDER-darkening; in daylight the factor correctly
// goes to 1. EVERY FAILURE PATH RETURNS 1.0. khw_nrel is the caller's
// confidence in khw_nrm, 0..1 (a mesh passes 1; the world pass passes the
// agreement between two baselines of its depth-derived plane, which collapses
// to the view axis inside quantisation plateaus). Where the normal is not
// resolved N.L falls back to 1 - the facing prior - on BOTH the numerator's
// diffuse and the denominator's sun.
float3 KhDlsWorldFactor(float3 khw_wpos, float3 khw_nrm, float khw_zunc,
                        float khw_nrel)
{
    if (dlCtl.x < 0.5f) return float3(1.0f, 1.0f, 1.0f);
    // Mode 3 only: modes 1/2 decode light positions relative to the camera
    // through lanes this pass does not fill.
    if (dlCtl.x < 2.5f) return float3(1.0f, 1.0f, 1.0f);

    const int khw_pointN = (int)dlCtl.y;
    const int khw_totalN = min(khw_pointN + (int)dlCtl.z, 32);   // Same floor as DynLights.
    const float3 khw_n = normalize(khw_nrm);

    float3 khw_dyn = float3(0.0f, 0.0f, 0.0f);       // What the lights add here.
    float3 khw_blocked = float3(0.0f, 0.0f, 0.0f);   // And how much we take back.
    const float khw_fwp = length(fwidth(khw_wpos));   // Footprint priced outside the loop.

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

        const float  khw_ndl = lerp(1.0f, max(dot(khw_n, khw_L), 0.0f), khw_nrel);
        const float3 khw_diff = dlGlobal.xyz * dlLights[khw_b + 2].xyz * khw_ndl;
        const float3 khw_amb = dlLights[khw_b + 3].xyz;
        khw_dyn += (khw_diff + khw_amb) * khw_att;

        // The slot lane, written by kh_dls_fill_cb's twin in the C++. 0 = this
        // light casts no shadow (the zeroed default).
        const int khw_slot = (int)dlLights[khw_b + 5].z - 1;
        if (khw_slot < 0) continue;
        const float khw_sh = KhDlsShadow(khw_slot, khw_wpos, khw_n, khw_zunc, khw_fwp);

        if (khw_sh >= 1.0f) continue;   // Fully lit: nothing blocked; skip the arithmetic.
        // Exactly the shading DynLights would have produced with the shadow
        // applied, including the partial ambient (KH_DLS_AMB_KEEP). The
        // difference is the blocked share.
        const float3 khw_shaded = (khw_diff * khw_sh
                                 + khw_amb * lerp(KH_DLS_AMB_KEEP, 1.0f, khw_sh)) * khw_att;
        khw_blocked += max((khw_diff + khw_amb) * khw_att - khw_shaded, 0.0f);
    }

    // DynLights' trailing global scale on both sides, so the ratio is
    // scale-invariant.
    khw_dyn *= dlGlobal.w;
    khw_blocked *= dlGlobal.w;
    if (dot(khw_blocked, float3(1.0f, 1.0f, 1.0f)) <= 0.0f) return float3(1.0f, 1.0f, 1.0f);

    // Ltotal: the engine's own combine, sun shadow assumed lit (see above).
    const float3 khw_sky = lightAmb.rgb * lighting0.z;
    // Same N.L weighting as the numerator: at khw_nrel 0 this reads full sun,
    // which overstates Ltotal and fails toward under-darkening.
    const float3 khw_sun = lighting2.rgb *
        (lerp(1.0f, max(dot(khw_n, lighting1.xyz), 0.0f), khw_nrel) * lighting0.w);
    const float3 khw_total = khw_dyn + khw_sky + khw_sun;

    return saturate(1.0f - khw_blocked / max(khw_total, 1e-4f));
}
 
struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL;
#if KH_TEXTURED
    float2 uv : TEXCOORD0; float4 tan : TANGENT;   // 48-byte lanes (layout_tex).
#endif
};
struct VSOut { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1;
    // Anchor-relative position, metres-scale through the interpolators
    // (world-absolute wpos at ~7.5 km rounds at +-0.49 mm per frame under
    // camera motion - half a hero texel). The self chain samples with this;
    // wpos stays for every absolute consumer.
    float3 wrel : TEXCOORD4;
    // The object colour rides the interpolators so the pixel twins read one
    // source whether the draw was per object (the VS copies the CB lane) or a
    // batch (the instance lane). Flat across a triangle by construction.
    float4 icol : TEXCOORD5;
    // The per-object lanes (KhObjLoad at every mesh PS entry). Flat per draw or
    // per instance.
    nointerpolation float4 iobj0 : TEXCOORD7;   // amb, dif, farVis, cut.
    nointerpolation float4 iobj1 : TEXCOORD8;   // 0, dither, 0, 0.
#if KH_TEXTURED
    float2 uv : TEXCOORD2; float4 tanw : TEXCOORD3;   // World tangent + handedness.
    nointerpolation uint matIx : TEXCOORD6;   // KH_MAT_TABLE: this draw's / instance's entry.
#endif
};

// The one vertex transform. The per-object lanes (centre, rebase-relative
// centre + arm, edge lengths, rotation rows) come from the CB on a per-object
// draw and from the instance stream on a batch draw; everything below the lane
// read is identical, so the four entry points (VSMain / VSMainInst, VSComposite
// / VSCompositeInst) are wrappers that only choose the source. VSInst = the
// 16-byte lane (C++ twin KhInstLane): record slot, LOD dither, final colour
// alpha, material table base.
struct VSInst {
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;    // x = dither, y = alpha, z = material base.
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
    // FP32 jitter rebase (see centerRel): when armed, the camera-relative
    // position goes through the rebased viewProj and the world-absolute fp32
    // cancellation never enters the position path.
    float3 khvTp = (khvc_relArm > 0.5f)
                 ? (khvc_rel + KhRotateR(khvc_lp * khvc_size, khvc_r0, khvc_r1, khvc_r2))
                 : wp;
    // stenVol2.z selects the vertex path: 1 = the absolute position through the
    // engine VP, 2 = the rebased one, 3 = the seam prepass (the engine-view z
    // remap). Only 3 is written today; 1/2 are arms, not the default, because a
    // camera-relative engine cb2 would displace an absolute wp by the whole
    // camera vector.
    float4x4 khEngVP = float4x4(engBlk[0], engBlk[1], engBlk[2], engBlk[3]);
    float3   khEngP  = (stenVol2.z >= 1.5f) ? khvTp : wp;
    float4   khClip  = mul(float4(khvTp, 1.0f), viewProj);

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
    // PS (far contract block) instead of here.
    khvc_owpos = wp;
    // KH_SELF_REL_INTERP: subtract the same fp32 anchor the sun matrices
    // subtract - the quantised anchor cancels exactly, and the interpolant
    // leaves at metres scale.
    khvc_owrel = wp - sunOrigin.xyz;
    // Per-axis scale is non-uniform: normals take the inverse scale, then the
    // object rotation (the inverse-transpose of scale-then-rotate for
    // orthonormal R - see kh_set_rotation).
    khvc_onrm = normalize(KhRotateR(khvc_ln / max(khvc_size, float3(1e-4f, 1e-4f, 1e-4f)), khvc_r0, khvc_r1, khvc_r2));
}

// The effect unit's depthTex owns t1: only the static and composite compiles
// pass KH_RECEIVE_TEX.
#ifdef KH_RECEIVE_TEX
// The engine's shadow atlas (the depth its cascade passes render into), sampled
// with the engine's own per-cascade world->atlasUV+depth transforms harvested
// from its constant uploads - the same compare the engine performs for its own
// geometry.
Texture2D<float> shadowAtlas : register(t1);
Texture2D<float> shadowBand0 : register(t4);
Texture2D<float> shadowBand1 : register(t5);
Texture2D<float> shadowBand2 : register(t6);
Texture2D<float> shadowBand3 : register(t7);
Texture2D<float> shadowBand4 : register(t8);
Texture2D<float> shadowBand5 : register(t9);
Texture2D<float> shadowBand6 : register(t12);   // Slots 6-7 (t10 is the terrain heightfield).
Texture2D<float> shadowBand7 : register(t13);

// Takes ABSOLUTE world positions: the engine renders camera-relative and its
// sampling transforms consume that space, so fill_lighting_frame_cb folds each
// entry's own camera origin into its translation (t' = t - M.cam) before the
// table lands here. A camera-relative position would be folded twice.
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
        // Tiles are sorted finest-first; the first cascade whose tile and depth
        // range contain the point decides.
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

// Bilinear compare per tap (the engine's sample_c equivalent): the 2x2 weighted
// compare resolves every dither cell of the atlas's dithered foliage to its
// coverage fraction. KhBandTap4 is the compare against ONE named band texture;
// the texture selection lives in the single branch below.
float KhBandTap4(Texture2D<float> khbt_m, float2 pos, float z)
{
    float2 f = pos - 0.5f;
    int2 p0 = int2(floor(f));
    float2 fr = frac(f);
    float b00 = ((z - khbt_m.Load(int3(p0, 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b10 = ((z - khbt_m.Load(int3(p0 + int2(1, 0), 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b01 = ((z - khbt_m.Load(int3(p0 + int2(0, 1), 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b11 = ((z - khbt_m.Load(int3(p0 + int2(1, 1), 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    return lerp(lerp(b00, b10, fr.x), lerp(b01, b11, fr.x), fr.y);
}

// The selection is a BRANCH, not a select: fxc flattens `?:` over the eight
// band textures into eight Load instructions per tap (it cannot index a texture
// object). [branch] on a value uniform across the band costs one taken arm.
float BandCmpBilin(int t, float2 pos, float z)
{
    [branch] if (t == 0) return KhBandTap4(shadowBand0, pos, z);
    [branch] if (t == 1) return KhBandTap4(shadowBand1, pos, z);
    [branch] if (t == 2) return KhBandTap4(shadowBand2, pos, z);
    [branch] if (t == 3) return KhBandTap4(shadowBand3, pos, z);
    [branch] if (t == 4) return KhBandTap4(shadowBand4, pos, z);
    [branch] if (t == 5) return KhBandTap4(shadowBand5, pos, z);
    [branch] if (t == 6) return KhBandTap4(shadowBand6, pos, z);
    return KhBandTap4(shadowBand7, pos, z);
}

float ShadowBandFactor(float3 wpos)
{
    // No [unroll] on these loops (X4575 in this unit); the 'done' flag plus a
    // plain break is the accepted early-out shape - no continue. Slots arrive
    // finest-first; the first containing band wins; bandBorder.w-1 names the
    // physical texture.
    float4 p = float4(wpos, 1.0f);
    float occ = -1.0f;
    float khbf_vz = 0.0f;   // The winning band's view-z, for the far fade.
    int done = 0;

    for (int s = 0; s < 8; ++s) {
        if (done != 0) break;
        if (bandBorder[s].w < 0.5f) { done = 0; }
        else {
            float vz = dot(p, bandView[s * 3 + 2]);

            if (vz >= bandBorder[s].x && vz < bandBorder[s].y) {
                float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
                float u = dot(vp4, bandMat[s * 3 + 0]);
                float v = dot(vp4, bandMat[s * 3 + 1]);
                float z = dot(vp4, bandMat[s * 3 + 2]);

                if (u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f && z > 0.001f && z < 0.999f) {
                    int t = (int)(bandBorder[s].w + 0.5f) - 1;   // w = 1 + texIndex.
                    float2 khbp_hz = frac(wpos.xz * 0.015625f) * 64.0f;
                    float ang = frac(sin(dot(khbp_hz, float2(12.9898f, 78.233f))) * 43758.5469f) * 6.2831853f;
                    float ca = cos(ang);
                    float sa = sin(ang);
                    const float r = 1.3f;
                    float2 base = float2(u, v) * shadowMeta.w;
                    float acc = 0.0f;

                    for (int k = 0; k < 4; ++k) {
                        float2 d0 =
                            (k == 0) ? float2( 0.974844f, 0.756484f)
                          : (k == 1) ? float2(-0.814100f, 0.914376f)
                          : (k == 2) ? float2( 0.945586f,-0.768907f)
                                     : float2(-0.815442f,-0.879125f);
                        // The 12 refinement taps below keep the rotation where
                        // it belongs.
                        acc += BandCmpBilin(t, base + d0 * r, z);
                    }

                    if (acc >= 3.999f || acc <= 0.001f) {
                        occ = acc >= 3.999f ? 1.0f : 0.0f;   // Decisive: engine's early-out.
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

                        occ = acc * 0.0625f;   // /16, the engine's own weight.
                    }

                    khbf_vz = vz;
                    done = 1;
                }
            }
        }
    }

    if (occ < 0.0f) return 1.0f;   // No band covers this depth: lit.
    // Fade at the shadow view distance (without it the shadow snaps into
    // existence at the cascade's far edge under motion).
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
    if (khbf_mf > 0.0f) {
        occ *= saturate((khbf_mf - khbf_d) / max(0.164f * (khbf_mf - khbf_mn), 1.0f));
    }
    return 1.0f - occ * saturate(lighting2.w);
}
#endif

// Engine distance haze (PSC_HazePars, sky CB row 14): the blue distance wash
// world geometry carries.
float KhHazeT(float khaz_d, float khaz_wposY, float khaz_camY, float khaz_layerY)
{
    if (hazePars.w < 0.5f) return 1.0f;
    if (khaz_camY < khaz_layerY) {
        if (fogBelow.y < 0.5f) return 1.0f;   // Stand-down below the layer.
        float khaz_bf = saturate((khaz_layerY - khaz_camY) /
                                 (max(khaz_wposY - khaz_camY, 0.0f) + 1.0e-5f));
        float khaz_sa = khaz_d * (1.0f - khaz_bf);   // Path above the layer.
        if (khaz_sa <= 0.0f) return 1.0f;
        float khaz_dn = khaz_sa + 1.0e-5f;
        float khaz_a2 = (abs(khaz_wposY - khaz_layerY) / khaz_dn) * hazePars.z;
        float khaz_I2 = abs(khaz_a2) < 1.0e-12f
                      ? khaz_sa
                      : (1.0f - exp(-khaz_sa * khaz_a2)) / khaz_a2;
        // minY = min(layerY, fragY) - the export's "min r3.z, r1.y, v8.y".
        float khaz_b2 = exp(-(min(khaz_layerY, khaz_wposY) - hazePars.x) *
                            hazePars.z) * hazePars.y;
        return min(exp(-khaz_I2 * khaz_b2), 1.0f);
    }

    float khaz_t   = saturate((khaz_camY - khaz_layerY) /
                              (max(khaz_camY - khaz_wposY, 0.0f) + 1.0e-5f));
    float khaz_s   = khaz_d * khaz_t;   // Path above the layer.
    float khaz_den = khaz_s + 1.0e-5f;
    float khaz_hi  = max(khaz_layerY, khaz_wposY);
    float khaz_a   = ((khaz_hi - khaz_camY) / khaz_den) * hazePars.z;   // Signed.
    // Integral of exp(-a x) over [0, s]; the a -> 0 limit is s itself.
    float khaz_I   = abs(khaz_a) < 1.0e-12f
                   ? khaz_s
                   : (1.0f - exp(-khaz_s * khaz_a)) / khaz_a;
    float khaz_b   = exp(-(khaz_camY - hazePars.x) * hazePars.z) * hazePars.y;

    return min(exp(-khaz_I * khaz_b), 1.0f);
}
