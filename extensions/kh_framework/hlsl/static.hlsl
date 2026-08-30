// static.hlsl - plain HLSL, embedded in the DLL as RCDATA resource KH_STATIC_HLSL by
// kh_shaders.rc (next to rendering_integration.hpp) and loaded at first use
// by kh_hlsl_src, which strips CR before the source is hashed for the shader
// cache, so the cache key does not depend on the checkout's line endings.
// Units are assembled by C++ concatenation of these resources, exactly as the
// old raw-string splice did; there is no #include and no size cap. Any edit
// here changes this unit's shader cache key (one cold recompile per user).

// KH_INSTANCING (26762): both entry points are wrappers over KhVsCore (the
// shared prefix) - the per-object one hands in the CB lanes, the instanced
// one the stream's. TWIN: VSComposite / VSCompositeInst carry the identical
// pair of wrappers over the same core.
VSOut VSMain(VSIn i)
{
    VSOut o;
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

VSOut VSMainInst(VSIn i, VSInst n)
{
    VSOut o;
    KhVsCore(i.pos, i.nrm, n.ipos.xyz, n.irel.xyz, n.irel.w, n.isize.xyz,
             n.irot0.xyz, n.irot1.xyz, n.irot2.xyz, o.pos, o.wpos, o.wrel, o.nrm);
    o.icol = n.icol;
#if KH_TEXTURED
    o.uv = i.uv;
    o.tanw = float4(normalize(KhRotateR(i.tan.xyz * n.isize.xyz, n.irot0.xyz, n.irot1.xyz, n.irot2.xyz)), i.tan.w);
#endif
    return o;
}

// That is 's lesson applied to the one pass that never got it: measure the
// engine's own inputs, do not model them. The injection has always drawn
// depth-only with no pixel shader, so its stored depth is the rasterised NDC
// z clamped to the viewport range under DepthClipEnable FALSE.
void PSInjDepth(VSOut i, out float khiODepth : SV_Depth)
{
    float  khiW  = max(i.pos.w, 1.0e-4f);
    float3 khiC2 = float3(engBlk[0].z, engBlk[1].z, engBlk[2].z);
    float3 khiC3 = float3(engBlk[0].w, engBlk[1].w, engBlk[2].w);
    float  khiM22 = dot(khiC2, khiC3) / max(dot(khiC3, khiC3), 1.0e-12f);
    float  khiM32 = engBlk[3].z - engBlk[3].w * khiM22;

    float  khiLo = depthParams.z;
    float  khiHi = depthParams.w;
    float  khiZ  = khiLo + (khiHi - khiLo) * (khiM22 + khiM32 / khiW);

    // The near-gap band. n = -m32/m22 from the pair above; a fragment inside
    // it is spread across [fxMeta.y, khiLo) in view depth instead of being
    // pinned to khiLo. Monotonic in w, so ordering is preserved where the
    // clamp destroyed it.
    float khiN = (khiM22 > 1.0e-6f) ? (-khiM32 / khiM22) : 0.0f;
    float khiFloor = khiLo;

    if (fxMeta.x > 0.5f && fxMeta.y > 0.0f && fxMeta.y < khiLo) {
        khiFloor = fxMeta.y;
        if (khiN > 0.0f && khiW < khiN)
            khiZ = fxMeta.y + (khiLo - fxMeta.y) * saturate(khiW / khiN);
    }

    khiODepth = clamp(khiZ, khiFloor, khiHi);
}
// TWIN CONTRACT: every discard the colour pass applies up to and including
// the view-distance cut is applied here identically - a fragment the colour
// pass would discard must never own a sample (the LOD crossfade's
// complementary dither is the load-bearing one: without it the finer level
// owns the pixels it dithers OUT and the coarser level holes).
void PSOwner(float4 khow_pos : SV_Position, float3 khow_wpos : TEXCOORD0,
             float3 khow_nrm : TEXCOORD1, out uint khow_out : SV_Target)
{
    if (blendCtl.w != 0.0f) {
        float khlD = frac(52.9829189f * frac(dot(khow_pos.xy, float2(0.06711056f, 0.00583715f))));
        if (blendCtl.w > 0.0f) { if (khlD >= blendCtl.w) discard; }
        else if (khlD < -blendCtl.w) discard;
    }
    ClipEdgeSliver(khow_wpos, khow_nrm);
    ClipOwnNear(khow_pos.w);
    if (shadowMeta2.x < 0.5f && depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(khow_pos.w, 1.0e-4f) > 1.0f) discard;
    if (shadowMeta2.x < 0.5f && shadowMeta2.y > 0.0f && khow_pos.w > shadowMeta2.y) discard;
    khow_out = KhOwnerPack(khow_pos.z, shadowMeta2.z);
}

struct VSInSun {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float4 ipos : TEXCOORD4;
    float4 isize : TEXCOORD5;
    float4 irot0 : TEXCOORD6;
    float4 irot1 : TEXCOORD7;
    float4 irot2 : TEXCOORD8;
};

    // CHUNK BOUNDARY (precedent; chunks concatenate).
// x/y/w come from OUR viewProj through the SAME khvTp rebase path that draws
// the visible box - the one space that is correct for our meshes by
// construction ('s lesson: the engine's b2 position space is not
// reconstructible and positions must never go through it).
struct VSInMir { float3 pos : POSITION; };
float4 VSMirror(VSInMir i) : SV_Position
{
    float3 khmv_wp = centerSize.xyz + KhRotate(i.pos * sizeAxes.xyz);
    float3 khmv_tp = (centerRel.w > 0.5f)
                   ? (centerRel.xyz + KhRotate(i.pos * sizeAxes.xyz))
                   : khmv_wp;
    float4 khmv_c = mul(float4(khmv_tp, 1.0f), viewProj);
    float3 khmv_c2 = float3(engBlk[0].z, engBlk[1].z, engBlk[2].z);
    float3 khmv_c3 = float3(engBlk[0].w, engBlk[1].w, engBlk[2].w);
    float  khmv_d3 = dot(khmv_c3, khmv_c3);
    if (khmv_d3 > 1.0e-12f) {
        float khmv_m22 = dot(khmv_c2, khmv_c3) / khmv_d3;
        float khmv_m32 = engBlk[3].z - engBlk[3].w * khmv_m22;
        float khmv_n = (abs(khmv_m22) > 1.0e-9f) ? (-khmv_m32 / khmv_m22) : -1.0f;
        if (khmv_n > 0.0f && khmv_m22 > 1.0f + 1.0e-6f && khmv_m32 < -1.0e-6f) {
            float khmv_f = khmv_n * khmv_m22 / (khmv_m22 - 1.0f);
            if (khmv_f > 0.06f) {
                float khmv_l22 = khmv_f / (khmv_f - 0.05f);
                khmv_c.z = khmv_l22 * khmv_c.w + (-0.05f * khmv_l22);
            }
        }
    }
    return khmv_c;
}
float4 VSSunDepth(VSInSun i) : SV_Position
{
    float3 lp = i.pos * i.isize.xyz;
    float3 wp = i.ipos.xyz + lp.x * i.irot0.xyz + lp.y * i.irot1.xyz + lp.z * i.irot2.xyz;
    return mul(float4(wp - sunOrigin.xyz, 1.0f), viewProj);
}

#if KH_TEXTURED
// KH_CAST_ALPHA (26764) - ALPHA-AWARE CASTING, IN THE MAP. Both consumers of
// the private sun-depth maps (the self kernels and the world cast through
// PSMaskCast's SunShadowOcclusion) read the same texels, so alpha enters
// once, here, and both agree by construction. A depth map holds no
// intensity, so intensity is COVERAGE: a caster of alpha a writes depth into
// a fraction a of the map's texels through a 4x4 ordered dither (hashed /
// stippled shadows - the technique masked and translucent casters take in
// shipping engines), and the receive kernels this renderer already runs
// (the 3x3 receiver-plane taps, the soft compare, the moment pyramids)
// average that coverage into a partial shadow of intensity ~ a. Cutout
// clips at the material's cutoff and casts full; blend scales by the
// texel's alpha; the object's colour alpha scales the whole caster (the
// instance stream's isize.w). Texels at or above 0.996 cast solid (the
// blend split's own threshold), below 0.004 cast nothing. The transform is
// VSSunDepth's line for line; the uv rides along. No colour target: the
// pass binds a DSV alone and this PS only clips.
struct VSInSunA {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 ipos : TEXCOORD4;
    float4 isize : TEXCOORD5;   // w = the caster's colour alpha (envelope applied)
    float4 irot0 : TEXCOORD6;
    float4 irot1 : TEXCOORD7;
    float4 irot2 : TEXCOORD8;
};
struct VSOutSunA { float4 pos : SV_Position; float2 uv : TEXCOORD0; float alpha : TEXCOORD1; };

VSOutSunA VSSunDepthA(VSInSunA i)
{
    VSOutSunA o;
    float3 lp = i.pos * i.isize.xyz;
    float3 wp = i.ipos.xyz + lp.x * i.irot0.xyz + lp.y * i.irot1.xyz + lp.z * i.irot2.xyz;
    o.pos = mul(float4(wp - sunOrigin.xyz, 1.0f), viewProj);
    o.uv = i.uv;
    o.alpha = i.isize.w;
    return o;
}

// 4x4 Bayer thresholds, (index + 0.5) / 16: every 4x4 window of map texels
// covers exactly the requested fraction to a sixteenth, and a 3x3 kernel
// reads it to within a ninth. Anchored to the map's texel grid.
float KhSunDither(float2 khsd_px)
{
    static const float khsd_b[16] = { 0.0f, 8.0f, 2.0f, 10.0f, 12.0f, 4.0f, 14.0f, 6.0f,
                                      3.0f, 11.0f, 1.0f, 9.0f, 15.0f, 7.0f, 13.0f, 5.0f };
    int2 khsd_p = int2(khsd_px) & 3;
    return (khsd_b[khsd_p.y * 4 + khsd_p.x] + 0.5f) / 16.0f;
}

void PSSunDepthA(VSOutSunA i)
{
    float khsa_a = i.alpha;
    int khsa_mode = (int)matParams0.y;   // 0 opaque, 1 cutout, 2 blend (kh_bind_material)
    // The material's alpha by its own route (diffuse.a by default; 1 when no
    // map is bound), exactly the colour pass's sampling.
    float khsa_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khsa_mode == 1) clip(khsa_t - matParams0.z);   // cutout: the cutoff kills, survivors cast full
    else if (khsa_mode == 2 && KhMatRouteTexel(matParams3.y, 1.0f, i.uv) < 0.9f) khsa_a *= khsa_t;   // KH_MAT_SPLIT_TOL   // blend: a solid TEXEL casts solid (KH_MAT_SPLIT_TEXEL), the rest by the filtered alpha
    if (khsa_a >= 0.996f) return;                       // solid
    clip(khsa_a - 0.004f);                              // transparent: casts nothing
    clip(khsa_a - KhSunDither(i.pos.xy));               // partial: dithered coverage
}

// KH_FOOTPRINT_ALPHA (26766) - THE SEAM FOOTPRINT CARRIES DEPTH-WRITING
// TEXELS ONLY. The footprint puts our meshes into the engine's shadow-volume
// depth so its stencil count is measured at our surfaces. A translucent
// texel writes no depth in the colour pass, so the engine's count at its
// pixel must be measured at what is BEHIND it (the ground the player sees
// through the glass) - a whole footprint made the engine count at the glass
// and the ground behind inherited the glass's shadow (the shadow that
// 'followed the surface'). This PS only clips: a cutout texel at its cutoff,
// a blend texel below the colour draw's own 0.996 solid threshold; the depth
// stays the raster's, exactly the null-PS footprint's. Whole translucent
// objects never reach it (skipped at the list). The mirror prepass keeps
// every mesh whole: that count is the glass's OWN verdict (rule 1.58).
void PSInjDepthA(VSOut i)
{
    int khfa_mode = (int)matParams0.y;
    float khfa_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khfa_mode == 1) clip(khfa_t - matParams0.z);
    else if (khfa_mode == 2) clip(KhMatRouteTexel(matParams3.y, 1.0f, i.uv) - 0.9f);   // KH_MAT_SPLIT_TOL   // KH_MAT_SPLIT_TEXEL: the colour pass's own verdict
}
#endif

// Analytic mask cast: per-pixel ray-vs-AABB toward the sun, drawn into the
// engine's screen-space shadow mask with multiply blending. castMat[0..2] =
// view rows 0..2; castView[0] = view row 3 (translation); castView[1] =
// (fov.x, fov.y, maskW, maskH); castView[2] = sunDir.xyz + strength in w.
// Scene linear depth at t0.
Texture2D<float4> sceneDepthTex : register(t0);
float KhSceneLoad(int2 p) { return sceneDepthTex.Load(int3(p, 0)).x; }

// TWIN: the composite chunk carries KhWitLoad over depthTex and a
// byte-identical filter. TWIN: the composite chunk's KhWitLoad takes.y of its
// own depthTex.
float KhWitLoad(int2 khwl_p) { return sceneDepthTex.Load(int3(khwl_p, 0)).y; }

// Defined per terminal chunk over that chunk's own KhWitLoad, textually
// before its PS body - fxc requires it. TWIN EDIT: both chunk copies of this
// function must stay byte-identical.
float KhVolSoftScene(float2 khfs_r, float3 khfs_p, float khfs_tol, int khfs_m)
{
    int2 khfs_c = KhVolPx(khfs_r);
    if (abs(KhWitLoad(khfs_c) - khfs_p.x) <= khfs_tol)
        return KhVolShadowed(KhVolCount(khfs_c), khfs_m) ? 0.0f : 1.0f;

    float khfs_s = 0.0f;
    float khfs_w = 0.0f;
    // MODE 196 = SNAP, DO NOT BLUR. Tracks the single best-matching tap so
    // the crisp count can be taken from IT instead of from an average. Ledger
    // at
    float khfs_bk = 0.0f;
    int2  khfs_bq = khfs_c;

    for (int khfs_j = -3; khfs_j <= 3; ++khfs_j) {
        for (int khfs_i = -3; khfs_i <= 3; ++khfs_i) {
            int2  khfs_q = KhVolPx(khfs_r + float2(khfs_i, khfs_j));
            float khfs_e = khfs_p.x + khfs_p.y * khfs_i + khfs_p.z * khfs_j;
            float khfs_d = abs(KhWitLoad(khfs_q) - khfs_e);
            float khfs_k = saturate(1.0f - khfs_d / max(khfs_tol * 3.0f, 1.0e-9f));
            khfs_k *= khfs_k;
            khfs_s += khfs_k * (KhVolShadowed(KhVolCount(khfs_q), khfs_m) ? 0.0f : 1.0f);
            khfs_w += khfs_k;
            if (khfs_k > khfs_bk) { khfs_bk = khfs_k; khfs_bq = khfs_q; }
        }
    }

    if (dbgCtl.w >= 9.5f && dbgCtl.w < 10.5f && khfs_bk > 0.0f)
        return KhVolShadowed(KhVolCount(khfs_bq), khfs_m) ? 0.0f : 1.0f;

    if (khfs_w < 1.0e-4f) {
        if (dbgCtl.w >= 11.5f && dbgCtl.w < 12.5f)
            return KhVolShadowed(KhVolCount(khfs_c), khfs_m) ? 0.0f : 1.0f;   // 364: point read
        float khfs_mx = 0.0f;
        for (int khfs_v = -1; khfs_v <= 1; ++khfs_v)
            for (int khfs_u = -1; khfs_u <= 1; ++khfs_u)
                if (KhVolShadowed(KhVolCount(KhVolPx(khfs_r + float2(khfs_u, khfs_v))), khfs_m))
                    khfs_mx = 1.0f;
        return 1.0f - khfs_mx;   // any shadowed neighbor -> shadowed
    }
    return khfs_s / khfs_w;
}
// Pre-mesh scene COLOR capture (t3, single-sample) - the perceptual-
// composite blend source for the FLUSH mesh pass; see PSMain's packing.
Texture2D<float4> sceneColorTex : register(t3);
// Bound only at the mask cast fire; compiled out of every entry that never
// reads it. Extended per-caster locality list (localityMeta.y arms): 2 float4
// per caster, [2i] center / [2i+1] half extents, engine axes - the UNCAPPED
// twin of the 16-pair CB list below.
StructuredBuffer<float4> khrLocalityExt : register(t2);
// KH_CAST_OCC: the caster-occupancy grid. The reach test below used to loop
// EVERY caster per pixel - O(casters x pixels), the whole performance problem,
// while the shadow lookup itself (SunShadowOcclusion) was already O(1). This
// texture answers the same question in constant time: a world-XZ grid whose
// texel holds (minY, maxY) over every caster reach volume covering that
// column. castMat[0].w = 1 / cell size and armed IS > 0; castMat[1].w /
// castMat[2].w = grid origin XZ. castView[2].w stays the cast STRENGTH (26758
// briefly borrowed it for 1/cell, which overwrote the strength - 26759).
// Strict SUPERSET of the loop, so no caster's shadow can be lost. Mode 474
// disarms it.
Texture2D<float2> khrCastOcc : register(t35);

// KH_CAST_REACH_DROP (26784). THE REACH IS A SHADOW, NOT A SPHERE.
// The old gate asked "is the receiver within |he| * stretch of the caster's
// AABB" - an isotropic radius priced on the caster's OWN EXTENT. That is only
// right for a caster standing ON the receiver. A caster at altitude throws its
// shadow (base height above the receiver) * |sun.xz| / sun.y metres DOWN-SUN,
// a distance the caster's own size cannot express: a 0.5 m prop 100 m up lands
// its shadow 154 m away at a 33 deg sun, against a radius of 3.8 m. The far
// end of every such shadow failed the gate and was refused - on screen, a
// straight arbitrary slice through one mesh's shadow (mode 29, which bypasses
// this gate entirely, closes exactly those cuts and nothing else).
// Now: sweep the caster down-sun onto the RECEIVER's own height plane and test
// the segment. t0/t1 are where the caster's base and top land at pw.y, so the
// tested segment IS the shadow. lr survives as pure lateral slack.
// A caster sitting on the receiver gives t0 = 0 and t1 = the old shadow
// length, so that case is unchanged. The Y term of the old test is gone - the
// sweep replaces it - which makes this strictly MORE permissive, and a false
// positive here is free: SunShadowOcclusion answers 0 where no occluder
// stands, so the gate can only cost ALU, never a wrong verdict.
// sunCastBias2.y arms the legacy isotropic form (mode 497).
 
// CHUNK BOUNDARY - MSVC caps one string literal token at 16380 bytes (C2026).
// KH_CAST_REACH_DROP + KH_CAST_REPROJ took this chunk to 15746 and the rule is
// SPLIT, never trim, when a segment approaches the cap. Cut at a top-level
// statement boundary ahead of the cast helpers, so the whole cast-gate and
// reprojection family now has its own chunk with room to grow.
bool KhCastReach(float3 khcr_p, float3 khcr_c, float3 khcr_h, float khcr_lr)
{
    if (sunCastBias2.y >= 0.5f) {   // 497: the 26783 isotropic sphere, verbatim
        float3 khcr_ld = max(abs(khcr_p - khcr_c) - khcr_h, 0.0f);
        return dot(khcr_ld, khcr_ld) < khcr_lr * khcr_lr;
    }
    float3 khcr_sd = castView[2].xyz;   // frozen sun, toward the light
    float  khcr_hl = length(khcr_sd.xz);
    float  khcr_sy = max(abs(khcr_sd.y), 0.02f);
    float  khcr_k  = khcr_hl / khcr_sy;   // metres down-sun per metre of drop
    float2 khcr_u  = khcr_hl > 1.0e-4f ? (-khcr_sd.xz / khcr_hl) : float2(0.0f, 0.0f);
    // Where this caster's base and top land at the receiver's height.
    float  khcr_t0 = max((khcr_c.y - khcr_h.y) - khcr_p.y, 0.0f) * khcr_k;
    float  khcr_t1 = max((khcr_c.y + khcr_h.y) - khcr_p.y, 0.0f) * khcr_k;
    float2 khcr_q  = khcr_p.xz - khcr_c.xz;
    float  khcr_t  = clamp(dot(khcr_q, khcr_u), khcr_t0, khcr_t1);
    float2 khcr_r  = khcr_q - khcr_u * khcr_t;
    float2 khcr_d  = max(abs(khcr_r) - khcr_h.xz, 0.0f);
    if (dot(khcr_d, khcr_d) < khcr_lr * khcr_lr) return true;
    // SUPERSET CONTRACT. The sweep is the geometrically right test but it is
    // not a superset of the 26783 sphere on its own: a receiver directly under
    // a floating caster has t clamped to t0, so the residual is the whole
    // throw, while the sphere admitted it whenever the drop was under lr. The
    // sweep is RIGHT to refuse it - no shadow stands there at a low sun - but
    // this gate may only ever ADD reach, never remove it, or a fix for the cut
    // becomes a new cut somewhere nobody is looking. A false positive costs
    // one SunShadowOcclusion that answers 0, so the union is free. Verified
    // over 200k sampled configurations: zero admitted-then-refused.
    float3 khcr_ld = max(abs(khcr_p - khcr_c) - khcr_h, 0.0f);
    return dot(khcr_ld, khcr_ld) < khcr_lr * khcr_lr;
}

// KH_CAST_REPROJ (26785) - the three pieces the drift fix needs, hoisted out
// of PSMaskCast so the solve can call them per iteration. fxc has no linker:
// they sit above their caller deliberately.
// KH_CAST_ZPLANE (26790). THE LAST UNTESTED INPUT TO pw.
// The two-plane depth carries a far plane in .x and a near/witness plane in
// .y, and the live rule takes .y whenever it is non-zero and SILENTLY FALLS
// BACK TO .x otherwise - per pixel, per region, with nothing counting it. The
// note at the top of PSMaskCast records that .x carries a CONSTANT z BIAS
// ("drift growing as fragments near, fine at range - relative error c/z"),
// which is why .y was preferred in the first place; the fallback was never
// removed. A constant error in zl offsets pw ALONG ITS OWN VIEW RAY by a fixed
// number of metres - unchanged as the camera approaches, sliding as the camera
// turns or moves, and DIFFERENT PER SHADOW because every shadow sits on a
// different ray. That is the reported drift exactly, and it is the only input
// to pw not yet eliminated: the camera is exact to 0.19 mm (engCamDxMaxMm),
// the rotation is orthonormal to 8e-5, the pairing was refuted across a proven
// 1.94 m gap, and the range-scale and fp32 families are both refuted by field
// observation.
// sunCastBias2.w: 0 = the live rule, 1 = .x only (mode 499), 2 = .y only, no
// fallback (mode 500). ONE selector for ALL THREE read sites - the
// reconstruction (PSMaskCast's zl), the reprojection's per-iteration read
// (KhCastZl) and the near-floor whole-texture probe - so no two of them can
// disagree about which plane they are on. The probe was a max(.x, .y) third
// rule until 26793; it is bit-identical at mode 0 because .y is empty.
// TWIN COUNT (rule 1.5), re-measured at 26809. THE TWO UNITS DISAGREE AND
// BOTH ARE RIGHT: there are THREE read sites but SIX CALL EXPRESSIONS,
// because the near-floor probe is one site that spends four Loads. Rule
// 1.5 recorded 5 uses, which is neither. Count CALL EXPRESSIONS when
// auditing this helper: 1 def + 6 uses = 7 occurrences of the name, at
// KhCastZl, PSMaskCast's zl, and the four probe Loads.
float KhCastZPick(float4 khzp_t)
{
    if (sunCastBias2.w >= 1.5f) return khzp_t.y;                  // 500: .y, no fallback
    if (sunCastBias2.w >= 0.5f) return khzp_t.x;                  // 499: .x only
    return khzp_t.y > 0.0f ? khzp_t.y : khzp_t.x;                 // live rule
}
float KhCastZl(float2 khcz_s, float2 khcz_dims)
{
    uint khcz_w, khcz_h;
    sceneDepthTex.GetDimensions(khcz_w, khcz_h);
    int2 khcz_p = int2(khcz_s * float2(khcz_w, khcz_h) / max(khcz_dims, float2(1.0f, 1.0f)));
    float4 khcz_t = sceneDepthTex.Load(int3(khcz_p, 0));
    return KhCastZPick(khcz_t);   // KH_CAST_ZPLANE
}
// Screen pixel -> world, through the FROZEN view the depth was rendered with
// (castView[0] is its translation row, castMat its inverse rotation). This is
// the reconstruction PSMaskCast has always done, unchanged, just callable.
float3 KhCastWorld(float2 khcw_s, float2 khcw_dims, float khcw_zl)
{
    float2 khcw_n = float2(khcw_s.x / khcw_dims.x * 2.0f - 1.0f,
                           1.0f - khcw_s.y / khcw_dims.y * 2.0f);
    float3 khcw_v = float3(khcw_n.x * castView[1].x, khcw_n.y * castView[1].y, 1.0f) * khcw_zl;
    float3 khcw_q = khcw_v - castView[0].xyz;
    return float3(dot(khcw_q, castMat[0].xyz),
                  dot(khcw_q, castMat[1].xyz),
                  dot(khcw_q, castMat[2].xyz));
}
// World -> screen pixel through THIS frame's view. The inverse direction of
// the above, against a different matrix - which is the entire point.
float2 KhCastPixN(float3 khcp_w, float2 khcp_dims)
{
    float3 khcp_v = mul(float4(khcp_w, 1.0f), castViewN).xyz;
    khcp_v.z = max(khcp_v.z, 0.05f);   // behind the eye: clamp, never divide by 0
    float2 khcp_n = float2(khcp_v.x / (khcp_v.z * max(castView[1].x, 1.0e-6f)),
                           khcp_v.y / (khcp_v.z * max(castView[1].y, 1.0e-6f)));
    return float2((khcp_n.x * 0.5f + 0.5f) * khcp_dims.x,
                  (0.5f - khcp_n.y * 0.5f) * khcp_dims.y);
}

 
// That guard was discarding a genuine stencil verdict wherever the TERRAIN
// BEHIND our mesh was fully cascade-shadowed, because pre describes the
// background (we never write linear depth) while our band receive describes
// our surface. MASK PRIMING. Mode 78 painted it as a blue band: post 0, pre
// 0.
float4 PSMaskPrime(VSOut i) : SV_Target
{
    // RANGE-BOUNDED. This reader was open-ended (>= 0.5), so EVERY value
    // dbgCtl.w has ever carried tripped it - 58, 180, 181, 182 and now
    // 199/200 all silently armed mode 108's prime value as a side effect.
    float khpm_v = (dbgCtl.w >= 0.5f && dbgCtl.w < 1.5f) ? 1.0f : KH_PRIME_V;   // mode 108
    return float4(khpm_v, khpm_v, khpm_v, 1.0f);
}
 
// ---------------------------------------------------------------------------
// KH_DLSW_MASK (26847) - THE WORLD PASS'S OWN OCCLUSION MASK.
//
// Three builds tried to decide "is one of our meshes in front of the world
// surface at this pixel" by reading the mirror target's depth plane, and all
// three failed the same way. Mode 540 named the reason: VSMirror does not write
// the clip depth of the matrix it is handed, it OVERWRITES z with its own near
// plane (l22 = f/(f-0.05); z = l22*w - 0.05*l22, f out of the engine's b2
// block) and does so CONDITIONALLY, on four tests against engBlk. Its plane is
// another feature's private convention, it changes shape frame to frame, and
// nothing obliges it to keep doing either.
//
// So this pass stops borrowing and renders its own. The mask is a single
// R32_FLOAT target holding the LINEAR VIEW DISTANCE, in metres, of the nearest
// of our meshes at every pixel - the same quantity KhCastZl already returns for
// the world, so the comparison is metres against metres with no convention to
// get wrong, no near/far to recover, and no depth partition to track. It is
// drawn with OUR matrix, cleared to OUR sentinel, and read by nobody else.
//
// NEAREST WITHOUT A DEPTH BUFFER: the target clears to a huge sentinel and the
// draws blend with D3D11_BLEND_OP_MIN, so overlapping meshes resolve to the
// closest surface with no depth-stencil target, no depth state and no z-fight.
// The minimum of a set of distances is the nearest one, which is exactly the
// question, so the blend IS the depth test here.
struct VSOutDM { float4 pos : SV_Position; float dist : TEXCOORD0; };

VSOutDM VSDlsMask(VSIn i)
{
    // KH_DLSW_MASKREBASE (26876e). The same object transform every mesh draw in
    // this file uses - INCLUDING the fp32 rebase, which this shader did not
    // take until now. KhVsCore transforms centerRel + rotate(local) against a
    // viewProj with the camera folded into its translation row whenever
    // centerRel.w is armed; building the absolute position instead and
    // multiplying by the absolute matrix is the same geometry evaluated in a
    // different order, and at kilometre-scale world coordinates the two round
    // differently by a fraction of a pixel. That fraction is the faint rim.
    //
    // The zeroed default (centerRel.w = 0) keeps the absolute path, so any
    // caller that does not rebase is unaffected - the same contract KhVsCore
    // honours (the 264 precedent).
    const float3 khdm_l = KhRotate(i.pos * sizeAxes.xyz);
    float3 khdm_w = (centerRel.w > 0.5f) ? (centerRel.xyz + khdm_l)
                                         : (centerSize.xyz + khdm_l);
    float4 khdm_c = mul(float4(khdm_w, 1.0f), viewProj);
    VSOutDM o;
    o.pos = khdm_c;
    // w of a perspective clip position IS the view-space distance along the
    // forward axis - the same quantity KhCastZl returns for the world surface.
    o.dist = khdm_c.w;
    return o;
}

float4 PSDlsMask(VSOutDM i) : SV_Target
{
    return float4(i.dist, 0.0f, 0.0f, 0.0f);
}

// The mask, read back by the world pass. The sentinel means no mesh covers it.
Texture2D<float> khDlsMask : register(t37);
// khdw_zl is the world surface's distance at the same pixel. Both are metres.
// The margin is half a percent of range - 5 cm at 10 m - which absorbs the
// rasterisation difference between this mask and the engine's own depth
// without ever reaching the scale of a real occlusion.
// KH_DLSW_MASKDILATE (26876b) WAS TRIED HERE AND REVERTED AT 26876c.
//
// It refused a pixel if ANY texel of the 3x3 neighbourhood held one of our
// meshes in front, on the theory that the residual see-through was our
// rasterisation and the engine's disagreeing at the boundary texel. It was
// shipped without measuring that, and it cost a build: the see-through did NOT
// change, so the leak is not at the boundary, and the dilation added exactly
// the bright one-pixel ring around our meshes that its own note predicted as
// the trade. A trade that pays nothing is not a trade. RULE 1.83, broken by me,
// in the same campaign that recorded it twice.
//
// What it did prove is worth keeping: the leak is NOT a one-texel registration
// residue. Mode 539 (mask verdict) and mode 558 (what this pass actually
// writes) are the two instruments that can say where it IS, and neither has
// been read yet. Nothing further gets built here until one of them is.
// KH_DLSW_MASKOWN (26877) - THE MASK ANSWERS COVERAGE, NOT A DEPTH RACE.
//
// THE QUESTION THIS FUNCTION EXISTS TO ANSWER IS "DID THE ENGINE DRAW OUR MESH
// AT THIS PIXEL". A finite value in the mask answers that exactly and by
// construction: our own rasteriser wrote it, through the engine's own view and
// projection (KH_DLSW_MASKREG) and the engine's own rebase (KH_DLSW_MASKREBASE),
// with the sentinel meaning nothing of ours is here.
//
// The trailing compare answers a DIFFERENT question - "is our mesh in front of
// the world surface" - and it answers it by racing two independently produced
// quantities against each other with a half-percent margin: our mask's clip w,
// taken this frame under the engine's matrices, against the engine's linear
// depth, which at our hook is the PREVIOUS frame's, reached through KhCastZl's
// rescale and KhCastZPick's plane rule. Nothing binds those two together. Every
// way that race can go wrong has the same signature, and it is the reported
// artifact word for word: the pixels where it goes wrong are decided by the
// depth of the WORLD GEOMETRY behind our mesh, so the region it fails over has
// the SHAPE of that geometry, it PARALLAXES with that geometry rather than with
// our mesh, and what gets painted there is the world's own shadow factor -
// which at night, fully blocked, is BLACK.
//
// That is the see-through, whatever the specific disagreement turns out to be,
// and this is the reason to stop guessing between the candidates: they all
// travel through this one comparison, so removing the comparison removes the
// class. Rule 1.83 is not being broken here - the fix is not aimed at an
// unmeasured hypothesis, it deletes the input every live hypothesis needs.
//
// THE COST IS KNOWN AND WAS ACCEPTED ONCE ALREADY. Without the compare, a mesh
// that sinks into terrain masks the pixels where it is BURIED as well as the
// ones where it is visible, so the shadow gets a bite taken out of it at the
// intersection. 26846 shipped exactly that trade in exactly these words - "a
// visibly smaller fault than painting the world's shading across the whole
// mesh" - and the field confirmed it working at 26841. 26847 added the compare
// back on top of a mask that had just been made registration-exact, at which
// point it was buying a case that no longer needed buying.
//
// *** SHIPPED AS THE DEFAULT AT 26877 AND TAKEN BACK OUT AT 26877a. THE NOTE
// *** ABOVE IS KEPT BECAUSE THE REASONING IS STILL SOUND AND THE CONCLUSION WAS
// *** STILL WRONG - RULE 1.83, BROKEN BY ME, IN THE BUILD AFTER I WROTE IT UP.
//
// It did not fix the see-through, and it cost the thing its own note predicted:
// a mesh half underground masked its BURIED pixels too, so the shadow it casts
// on the terrain was clipped away along the ground line. That is a regression
// of a fault fixed long ago, and it is the 26876b lesson repeated - a trade
// that pays nothing is not a trade.
//
// What the failed trade DID buy is worth keeping: the depth race is not the
// carrier. Removing it entirely changed nothing about the artifact, which is a
// far stronger statement than any paint mode has made about this route, and it
// leaves the world pass's mask exonerated by construction rather than by a
// screenshot reading of 558.
//
// Default is the 26847 compare again, bit-identical. Mode 575 now ARMS the
// coverage-only rule rather than reverting it, so the measurement stays
// available and costs nothing.
#define KH_DLSW_MASKOWN (dbgCtl.w >= 574.5f && dbgCtl.w < 575.5f)
#define KH_DLSW_MASKDEPTH (!KH_DLSW_MASKOWN)
bool KhDlsMaskInFront(float2 khdw_px, float khdw_w, float khdw_h, float khdw_zl)
{
    int2 khdw_p = int2(clamp(khdw_px.x, 0.0f, khdw_w - 1.0f),
                       clamp(khdw_px.y, 0.0f, khdw_h - 1.0f));
    const float khdw_d = khDlsMask.Load(int3(khdw_p, 0));
    if (!(khdw_d > 0.0f) || khdw_d > 1.0e29f) return false;   // no mesh here
    if (KH_DLSW_MASKDEPTH) return khdw_d < khdw_zl * 0.995f;   // the 26847 rule
    return true;
}

// KH_DLSW_MASKCOV (26876f) - THE LAST OF THE RIM IS MULTISAMPLING, NOT
// REGISTRATION, AND THE DUMP SAYS SO OUTRIGHT.
//
// dlswRtDim reads 6 = TEXTURE2DMS and the scene runs 8 depth samples. So the
// target this pass multiplies into is MULTISAMPLED, our fullscreen triangle
// covers every sample of every pixel, and the pixel shader runs at PIXEL
// frequency - one factor written to all eight samples.
//
// At a silhouette pixel some of those samples belong to our mesh and some to
// the world behind it. A binary mask has to choose for all of them at once: it
// says "ours", the pass refuses, and the world samples in that pixel keep their
// full unshadowed brightness while every neighbouring pixel is darkened. That
// is the rim, and no amount of matching matrices can remove it - 26876a matched
// the view and projection, 26876e matched the fp32 rebase, and each took a bite
// out of it because each reduced the DISAGREEMENT, but the all-or-nothing
// decision is structural and survives perfect registration.
//
// The framebuffer pixel holds c = a*mesh + (1-a)*world for coverage a, and what
// we want is a*mesh + (1-a)*world*f. With one multiply the best available is
// g = a + (1-a)*f, which is exact when the mesh and the world behind it carry
// similar radiance and errs gently when they do not. So the mask returns
// COVERAGE rather than a verdict, and the factor is blended toward 1 by it.
//
// The coverage estimate is a 3x3 box over the binary mask, which is a real
// approximation and worth naming: it cannot see subpixel coverage, so it ramps
// over about three pixels instead of one. That trades a one-pixel step at FULL
// amplitude for a three-pixel gradient at a third of it, which is the direction
// the eye forgives. Exact elimination needs the mask itself to carry coverage -
// an MSAA or supersampled mask target - and that is memory this has not been
// shown to be worth yet.
//
// This is NOT the 26876b dilation returning. That refused whole pixels it was
// not sure about, which is what put a hard bright ring around our meshes. This
// refuses them PARTIALLY, in proportion, which is the thing dilation should
// have been all along.
//
// Mode 563 restores the binary verdict exactly: it returns 1 or 0, the
// interior early-out still fires on 1, and a 0 blends nothing.
 
// CHUNK BOUNDARY (26876f). SPLIT, never trim: the note above KhDlsMaskCov is
// the record of why three builds each removed part of the rim without removing
// it, and it is shorter than a fourth build spent rediscovering that.
float KhDlsMaskCov(float2 khmc_px, float khmc_w, float khmc_h, float khmc_zl)
{
    if (dbgCtl.w >= 562.5f && dbgCtl.w < 563.5f) {
        return KhDlsMaskInFront(khmc_px, khmc_w, khmc_h, khmc_zl) ? 1.0f : 0.0f;
    }

    const int2 khmc_c = int2(clamp(khmc_px.x, 0.0f, khmc_w - 1.0f),
                             clamp(khmc_px.y, 0.0f, khmc_h - 1.0f));
    const int2 khmc_mx = int2((int)khmc_w - 1, (int)khmc_h - 1);
    float khmc_n = 0.0f;

    [unroll] for (int khmc_j = -1; khmc_j <= 1; ++khmc_j) {
        [unroll] for (int khmc_i = -1; khmc_i <= 1; ++khmc_i) {
            const int2 khmc_p = clamp(khmc_c + int2(khmc_i, khmc_j),
                                      int2(0, 0), khmc_mx);
            const float khmc_d = khDlsMask.Load(int3(khmc_p, 0));
            if (!(khmc_d > 0.0f) || khmc_d > 1.0e29f) continue;
            // KH_DLSW_MASKOWN (26877, DEFAULT REVERTED AT 26877a). The centre
            // pixel's own world distance is the reference again, which is the
            // 26847 form verbatim; mode 575 arms the coverage-only rule. See
            // the note above KhDlsMaskInFront for why it was tried and what
            // trying it proved.
            if (!KH_DLSW_MASKDEPTH || khmc_d < khmc_zl * 0.995f) khmc_n += 1.0f;
        }
    }

    return khmc_n * (1.0f / 9.0f);
}

// KH_DLSW_NRM (26876). One plane tap at a chosen pixel radius, for the
// two-baseline normal in PSDlsWorld. Returns the two world-space central
// differences, or false when any of the four taps is not a surface.
//
// R IS IN PIXELS AND THAT IS THE POINT. The engine's linear depth is quantised
// with a step fixed in METRES, so the true depth difference across the stencil
// grows with R while the quantisation does not. Widening the stencil buys
// signal-to-quantisation directly, with no constant to guess.
//
// THE COST OF WIDENING is that a stencil straddling a real edge fits a plane
// across it. That is why the taps are REFUSED rather than clamped when one
// lands on sky or off the depth (rule 1.91) and why the caller keeps the 2x2
// quad derivative as the fallback: near a silhouette the narrow answer is the
// honest one, and the caller is told so through khw_nrel.
static const float KH_DLSW_NRM_R = 3.0f;        // base baseline, pixels
// KH_DLSW_NRM_FLAT (26876b): the SECOND-DIFFERENCE bound that decides whether
// the stencil is looking at ONE surface. For any plane, zl(+r) + zl(-r) - 2*zl
// is exactly zero whatever the plane's slope, so this test is blind to
// orientation and sensitive only to the neighbourhood breaking - which is the
// question. Quantisation leaves a residue of about one step (~100 mm measured),
// so the bound is relative to range and set well above that and well below a
// cliff: 2% of range is 0.6 m of curvature across the stencil at 30 m, which no
// continuous surface produces and every silhouette does.
static const float KH_DLSW_NRM_FLAT = 0.02f;
// The agreement floor for the two baselines. Dimensionless - it compares two
// unit normals, so it is not a bar standing in for a length (rule 1.84), and it
// needs no knowledge of the quantisation step. cos(8 deg): two baselines that
// disagree by more than that are not describing one plane.
static const float KH_DLSW_NRM_AGREE = 0.990f;

// KH_DLSW_ONESIDED (26876c) - THE NORMAL AT A DEPTH DISCONTINUITY.
//
// Every version of this pass so far has had NO valid normal at a silhouette,
// and has only ever chosen which way to be wrong. A 2x2 quad derivative there
// straddles the cliff and returns a plane belonging to neither surface; 26876a
// answered that by refusing and applying the facing prior, which over-darkened
// and drew a DARK outline; 26876b answered it by trusting the straddling
// derivative, which draws a BRIGHT one. The field has now reported both, on
// game geometry and on ours, and they are the same defect seen from two sides.
//
// The centre pixel is not ambiguous - it belongs to exactly one surface. Only
// the DIFFERENCE is, and only because it was taken across the cliff. So take it
// on the side that stays home: per axis, compare the two neighbours' depths
// against the centre's and keep the nearer one. On a continuous surface both
// sides agree and this is the central difference to within a quantisation step;
// at a cliff it is the one-sided difference along the surface the centre is on.
// No prior, no threshold, no choice about which way to be wrong.
//
// 26869 BUILT THIS AND 26870 DELETED IT, on the reading that mode 520 had
// refuted the normal. That reading was struck at 26876 (KH_DLSW_BANDING entry
// six): 520 only ever disarmed the normal inside KhDlsShadow, never inside the
// factor. The code was right and the reason for removing it was not.
bool KhDlswOneSided(float2 kho_px, float2 kho_dims, float kho_zc, float3 kho_w,
                    out float3 kho_dx, out float3 kho_dy)
{
    const float kho_zpx = KhCastZl(kho_px + float2(1.0f, 0.0f), kho_dims);
    const float kho_znx = KhCastZl(kho_px - float2(1.0f, 0.0f), kho_dims);
    const float kho_zpy = KhCastZl(kho_px + float2(0.0f, 1.0f), kho_dims);
    const float kho_zny = KhCastZl(kho_px - float2(0.0f, 1.0f), kho_dims);
    const bool kho_vpx = (kho_zpx > 0.05f) && (kho_zpx < 8000.0f);
    const bool kho_vnx = (kho_znx > 0.05f) && (kho_znx < 8000.0f);
    const bool kho_vpy = (kho_zpy > 0.05f) && (kho_zpy < 8000.0f);
    const bool kho_vny = (kho_zny > 0.05f) && (kho_zny < 8000.0f);
    if (!(kho_vpx || kho_vnx) || !(kho_vpy || kho_vny)) return false;

    // Keep the side whose depth is nearer the centre's - that is the side that
    // did not cross the cliff. Signs are chosen so both differences still point
    // along +x and +y, or the cross product would flip at every edge.
    const bool kho_usex = kho_vpx && (!kho_vnx ||
                          abs(kho_zpx - kho_zc) <= abs(kho_znx - kho_zc));
    const bool kho_usey = kho_vpy && (!kho_vny ||
                          abs(kho_zpy - kho_zc) <= abs(kho_zny - kho_zc));
    kho_dx = kho_usex
           ? KhCastWorld(kho_px + float2(1.0f, 0.0f), kho_dims, kho_zpx) - kho_w
           : kho_w - KhCastWorld(kho_px - float2(1.0f, 0.0f), kho_dims, kho_znx);
    kho_dy = kho_usey
           ? KhCastWorld(kho_px + float2(0.0f, 1.0f), kho_dims, kho_zpy) - kho_w
           : kho_w - KhCastWorld(kho_px - float2(0.0f, 1.0f), kho_dims, kho_zny);
    return true;
}

bool KhDlswPlane(float2 khp_px, float2 khp_dims, float khp_r, float khp_zc,
                 out float3 khp_dx, out float3 khp_dy)
{
    khp_dx = float3(0.0f, 0.0f, 0.0f);
    khp_dy = float3(0.0f, 0.0f, 0.0f);
    const float khp_zpx = KhCastZl(khp_px + float2(khp_r, 0.0f), khp_dims);
    const float khp_znx = KhCastZl(khp_px - float2(khp_r, 0.0f), khp_dims);
    const float khp_zpy = KhCastZl(khp_px + float2(0.0f, khp_r), khp_dims);
    const float khp_zny = KhCastZl(khp_px - float2(0.0f, khp_r), khp_dims);
    // The same window PSDlsWorld admits for its own centre pixel, so a tap can
    // never contribute a surface the pass itself would have refused.
    if (!(khp_zpx > 0.05f) || khp_zpx > 8000.0f) return false;
    if (!(khp_znx > 0.05f) || khp_znx > 8000.0f) return false;
    if (!(khp_zpy > 0.05f) || khp_zpy > 8000.0f) return false;
    if (!(khp_zny > 0.05f) || khp_zny > 8000.0f) return false;
    // ONE SURFACE, OR NONE. A stencil straddling a silhouette fits a plane
    // across the gap and returns a normal belonging to neither side, and the
    // caller must know that rather than be handed it. Refused, not clamped
    // (rule 1.91) - the caller falls back to the narrowest measurement it has.
    const float khp_cx = abs(khp_zpx + khp_znx - 2.0f * khp_zc);
    const float khp_cy = abs(khp_zpy + khp_zny - 2.0f * khp_zc);
    if (max(khp_cx, khp_cy) > KH_DLSW_NRM_FLAT * khp_zc) return false;
    khp_dx = KhCastWorld(khp_px + float2(khp_r, 0.0f), khp_dims, khp_zpx)
           - KhCastWorld(khp_px - float2(khp_r, 0.0f), khp_dims, khp_znx);
    khp_dy = KhCastWorld(khp_px + float2(0.0f, khp_r), khp_dims, khp_zpy)
           - KhCastWorld(khp_px - float2(0.0f, khp_r), khp_dims, khp_zny);
    return true;
}
 
// ---------------------------------------------------------------------------
// KH_DLS_WORLD (26834) - the world-receive pass for dynamic-light shadows.
//
// Fullscreen, fired from the scene-resolve hook, emitting a MULTIPLY FACTOR
// under a dest*src blend. All the lighting arithmetic lives in
// KhDlsWorldFactor beside DynLights; this shader's only job is to turn a
// screen pixel into a world position and a normal.
//
// DEPTH AND VIEW ARE BOTH THIS FRAME'S, and that is the one place this pass
// deliberately does NOT copy the sun's world cast. PSMaskCast reconstructs
// through a FROZEN view because it paints at draw 0, where - as the note at
// castViewN says outright - there IS no depth for this frame yet, so it must
// pair depth(N-1) with view(N-1) and then reproject. This pass fires at the
// resolve, two thirds of the way through the frame and after the world is
// drawn, so the live depth is complete and is snapshotted immediately before
// the draw. Pairing depth(N) with view(N) removes the whole reprojection
// class rather than compensating for it. Everything else - the caster set,
// the eight light maps, KhDlsShadow, its filter, its bias, its fail-to-lit
// rule - is shared with the mesh receive by construction.
//
// THE NORMAL IS RECONSTRUCTED FROM THE DEPTH FIELD, because the engine is
// forward and exposes no normal buffer (confirmed: 424 shaders in the export,
// one MRT pixel shader, an 8-way depth decimate). Screen-space derivatives of
// the reconstructed world position give the surface plane directly. This is
// noisy at silhouettes, where ddx/ddy straddle a depth discontinuity and the
// cross product swings wildly - so the result is only ever used for an N.L
// term and for KhDlsShadow's normal offset, both of which degrade to a
// slightly wrong shade rather than to a wrong occlusion verdict, and the
// factor is clamped to [0,1] regardless.
// KH_DLS_WORLD_STAGE (26835, mode 534) - WHY IS NOTHING HAPPENING?
//
// 26834 fired this pass on EVERY cycle with zero refusals - dlsWorldFires 551
// of 551 cycles, NoState/NoSlot/SnapFails/UploadFails/NoRt all 0 - and mode
// 531, which paints every pixel of the screen unconditionally, showed nothing
// at all. Those two facts cannot both be true unless something between the
// draw and the pixel is eating the result, and no lane can see inside a pixel
// shader. Every early exit here returns 1.0, which under a multiply blend is
// INVISIBLE, so a shader that bails on its first line and a shader that never
// ran look identical on screen. That is the 26830 trap again, one level down.
//
// Mode 534 makes each exit paint a DIFFERENT FLAT COLOUR instead of 1.0, so a
// single screenshot names the stage that is failing, per pixel:
//
//   RED     the arm lane is not reaching the shader (dbgCtl.z < 0.5) - the
//           constant buffer is not arriving, or not at b0
//   GREEN   the arm arrived but castView[1].zw carries no screen dimensions -
//           CBFrame is not arriving, or not at b1
//   BLUE    both buffers arrived and the DEPTH is unusable at this pixel -
//           the snapshot is empty, or the plane pick is wrong
//   YELLOW  everything decoded; this pixel reached the lighting kernel
//
// If the screen stays untinted even under 534, the pixel shader is not running
// or its output is not landing, and the fault is in the C++ pass rather than
// anywhere below this line. That is the one outcome no colour can express, and
// it is the most useful thing 534 can tell us.
#define KH_DLSW_STAGE (dbgCtl.w >= 533.5f && dbgCtl.w < 534.5f)
float4 PSDlsWorld(VSOut i) : SV_Target
{
    // dbgCtl.z arms this pass; a zeroed lane is a no-op multiply, so any site
    // that does not fill it behaves exactly as it did (the 264 precedent).
    if (dbgCtl.z < 0.5f) {
        // The stage view cannot read dbgCtl.w here either if the buffer is the
        // problem - so it is tested LAST, and a red screen with no mode set is
        // still the honest answer for "no constants".
        return KH_DLSW_STAGE ? float4(1.0f, 0.0f, 0.0f, 1.0f)
                             : float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    const float2 khw_dims = float2(castView[1].z, castView[1].w);
    if (khw_dims.x < 2.0f || khw_dims.y < 2.0f) {
        return KH_DLSW_STAGE ? float4(0.0f, 1.0f, 0.0f, 1.0f)
                             : float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    const float khw_zl = KhCastZl(i.pos.xy, khw_dims);
    // Sky and anything the depth pass never wrote: no surface, nothing to
    // shadow. The far plane reads as a huge or zero zl depending on the
    // encoding, so both ends are refused.
    if (!(khw_zl > 0.05f) || khw_zl > 8000.0f) {
        return KH_DLSW_STAGE ? float4(0.0f, 0.0f, 1.0f, 1.0f)
                             : float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    if (KH_DLSW_STAGE) return float4(1.0f, 1.0f, 0.0f, 1.0f);

    // KH_DLSW_ZL (26836, mode 535) - WHAT IS zl, NUMERICALLY?
    //
    // 534 came back yellow everywhere: both buffers arrive, the dimensions
    // arrive, and every pixel decodes a zl inside (0.05, 8000). But the 531
    // grid showed NO 1 m tiling on the ground - just a smooth gradient - and a
    // smooth gradient is what you get when the reconstructed world position
    // barely changes across the screen. That happens when zl is SMALL: with a
    // tiny view ray, q = v - camera is dominated by the camera term and every
    // pixel maps to nearly the same world point.
    //
    // The suspicion is therefore that the snapshot holds RAW DEPTH in 0..1
    // rather than the linear metres KhCastWorld's ray scaling requires. That
    // would also pass 534's range test, which is why 534 could not see it.
    //
    // This paints the MAGNITUDE of zl in three overlapping decades, so one
    // screenshot reads it off without a capture:
    //   mostly RED    zl is order 1        -> raw 0..1 depth, not metres
    //   mostly GREEN  zl is order 100      -> metres, as KhCastWorld expects
    //   mostly BLUE   zl is order 1000+    -> metres but far, or unbounded
    //   black         zl is order 0.01     -> effectively zero
    // The red channel is frac() so a metre-scale zl also gives it fine
    // structure; a raw-depth zl gives a smooth ramp instead. Shape as well as
    // hue, in case the magnitudes overlap.
    if (dbgCtl.w >= 534.5f && dbgCtl.w < 535.5f) {
        return float4(frac(khw_zl), saturate(khw_zl / 100.0f),
                      saturate(khw_zl / 1000.0f), 1.0f);
    }

    // KH_DLSW_SELFMASK (26838) - OUR OWN MESHES ARE NOT THE WORLD.
    //
    // The engine's linear depth does not contain our injected geometry, so at a
    // pixel our mesh covers, khw_zl is the distance to the world BEHIND it and
    // the world position below belongs to the ground, not to the mesh. Shading
    // that and painting it here put the vest's own shadow across the vest, laid
    // out as though it were on the ground behind - which is exactly what it
    // was, drawn in the wrong place.
    //
    // khMirSten is our meshes' stencil, the same mask the translucency path
    // reads, so the two cannot disagree about which pixels are ours.
    // KhMirUnit returns 0 where a mesh covers the pixel. Returning 1.0 leaves
    // those pixels to the mesh kernels, which shadow them properly through
    // KhDlsShadow with the mesh's own surface and normal.
    //
    // Mode 538 disarms this, which is the A/B: if the smear returns under 538
    // and not at default, the mask is what removed it.
    // KH_DLSW_MASKVIEW (26839, mode 539) - IS THE MASK MARKING ANYTHING?
    //
    // 26838 armed this mask on every frame (dlsWorldSelfMask 239 of 239) and it
    // changed NOTHING - mode 538, which disarms it, was indistinguishable from
    // default. A mask that is armed and has no effect is a mask that reports
    // "not ours" at every pixel, and there is a plain reason it might: the SRV
    // is X24_TYPELESS_G8_UINT, so KhMirUnit reads the STENCIL plane, while the
    // vmir prepass draws our meshes DEPTH-ONLY with a null pixel shader. If
    // nothing writes stencil there, the plane is zero everywhere and the mask
    // is inert.
    //
    // Rather than assume that, paint it: BLACK where the mask says a pixel is
    // ours, WHITE elsewhere. Under a multiply blend that is unmissable.
    //   a black silhouette of the meshes -> the mask works, and the shadow
    //     showing through the vest is the MESH'S OWN TRANSLUCENCY, not this
    //     pass painting on it - the ground behind is legitimately visible and
    //     there is nothing here to fix
    //   an all-white screen            -> the stencil plane is empty and this
    //     mask cannot isolate our meshes; the isolation has to come from
    //     somewhere else
    // Those two have opposite fixes, which is why guessing between them is
    // worth one build to avoid.
    // OUR OWN MESHES ARE NOT THE WORLD, and the mask now reads the plane the
    // prepass writes (see KhMirCovered). The engine's linear depth does not
    // contain our injected geometry, so at a pixel one of our meshes covers,
    // khw_zl is the distance to the world BEHIND it - and shading that put the
    // GROUND'S shadow onto the mesh, which is why the vest looked see-through
    // exactly where the world behind it would have been lit.
    //
    // Those pixels are left alone: the mesh kernels shadow them through
    // KhDlsShadow with the mesh's own surface and normal, which is the right
    // term for a surface we actually drew.
    const float3 khw_w = KhCastWorld(i.pos.xy, khw_dims, khw_zl);

    // Plane of the surface from the reconstructed field. The sign is fixed by
    // making the normal face the eye; castView[2].xyz is this pass's camera.
    //
    // 26869 REPLACED THIS WITH ONE-SIDED DIFFERENCES AND 26870 TOOK IT BACK
    // OUT, on the reading that "mode 520 refutes the receiver normal". THAT
    // READING WAS WRONG, AND IT COST THE WHOLE TASK-3 CAMPAIGN (rule 1.73).
    //
    // 520 disarms the normal inside KhDlsShadow ONLY - dlsFaceSlice[].y gates
    // khd_off_on, whose two readers are the offset at khd_p and the N.L floor
    // at khd_ndl. It does not reach KhDlsWorldFactor, which reads this same
    // normal in THREE more places 520 never touched: khw_ndl on every light's
    // diffuse, the same term inside khw_dyn, and the sun's N.L in the
    // denominator. So "the rings survive 520" says the shadow LOOKUP is
    // innocent. It says nothing at all about the normal, and the ledger's
    // hypothesis 5 must be struck.
    //
    // KH_DLSW_NRM (26876) - WHY THE NORMAL IS THE RING SOURCE.
    //
    // Mode 550 measured the engine's linear depth arriving in PLATEAUS beside
    // 100 mm CLIFFS. Feed a quantised field to a 2x2 quad derivative and the
    // reconstructed plane is not noisy - it is WRONG IN A SPECIFIC DIRECTION.
    // KhCastWorld builds v = (ndc.x*fovx, ndc.y*fovy, 1) * zl, so inside a
    // plateau zl is CONSTANT across the quad and the reconstructed surface is
    // the plane z = const in view space. Its normal is the VIEW AXIS. At every
    // cliff the depth jump dominates instead and the normal snaps back toward
    // the true surface. The normal therefore alternates between "pointing at
    // the camera" and "correct", switching exactly on the iso-depth contours of
    // the quantised field.
    //
    // THE SHAPE FOLLOWS. Iso-zl contours of a ground plane in screen space are
    // conics, nested, symmetric about the plane containing the view axis - so
    // they paint CONCENTRIC RINGS WITH ONE STRAIGHT SEAM ON THE SYMMETRY AXIS,
    // which is the field's report verbatim, seam included. Nothing in a shadow
    // MAP lookup can produce that shape; the map knows nothing about the
    // camera, and these contours are camera-locked.
    //
    // THE FADE FOLLOWS TOO, from the same mechanism. As the camera lies down
    // toward the surface, zl changes less per pixel, the plateaus widen, and a
    // larger and larger fraction of pixels sit in a plateau reading N = view
    // axis. A near-horizontal view axis against a lamp overhead gives
    // dot(N, L) -> 0, so khw_diff -> 0 and the blocked share collapses: the
    // shadow washes out precisely as the camera approaches. One mechanism, both
    // symptoms, and it explains why nine hypotheses aimed at the map all missed.
    //
    // THE FIX IS A BASELINE, NOT A FILTER. Quantisation is a fixed step in
    // metres; the true depth difference across the stencil grows with the
    // stencil. So sample the plane over KH_DLSW_NRM_R pixels instead of one and
    // the signal-to-quantisation ratio improves by that factor directly, with
    // no assumption about the step's size - which matters, because we have
    // never measured it and rule 1.84 says not to stand a bar where a
    // principle belongs.
    //
    // AND IT REPORTS ITS OWN CONFIDENCE. The plane is measured at TWO baselines
    // and their agreement is khw_nrel. On a genuinely resolved surface the two
    // agree closely whatever the surface is doing; on a staircase they do not,
    // because each baseline lands on a different tread. That is a dimensionless
    // self-calibrating witness - it needs no depth constant and no knowledge of
    // the quantisation step, which is exactly what hypothesis 9 lacked.
    // khw_nrel is carried into the factor, where a normal we cannot resolve
    // stops being allowed to drive N.L.
    //
    // Mode 553 reverts to the 2x2 derivative and an unweighted N.L - the A/B.
    // Mode 554 paints khw_nrel. Mode 520 is unchanged and still means what it
    // always meant, which is now correctly scoped to the lookup.
    const bool khw_nrev = (dbgCtl.w >= 552.5f && dbgCtl.w < 553.5f);
 
    // KH_DLSW_REACHCUT (26878) - THE FULLSCREEN PASS STOPS PAYING FOR PIXELS
    // NO LIGHT CAN REACH.
    //
    // Everything below this line - the two plane fits and the one-sided
    // fallback (8-12 depth Loads), the 3x3 coverage read (9 more) and the
    // per-light kernel - only matters where some casting light's map can
    // answer. KhDlsFaceUV refuses every receiver whose face-axis depth is at
    // or past that light's far plane, and the face-axis depth is the largest
    // absolute component of the light-relative position, which is at least
    // |p| / sqrt(3). So a pixel farther than sqrt(3) * far from EVERY casting
    // light cannot be shadowed by any of them, KhDlsWorldFactor returns 1
    // there by construction, and the multiply is a no-op. One reconstructed
    // position (already in hand) and at most eight distance compares decide
    // that, before the first extra Load. The 5% widening covers the 1.5-texel
    // receiver-normal offset KhDlsShadow applies before its own reach test
    // (at most 3 / size of the distance, i.e. ~1.2% at a 256 map).
    //
    // MODE 0 ONLY, by design: every instrument in this shader paints from
    // inputs computed below, and an instrument that goes white outside the
    // light radii would be a different instrument. Any set mode is therefore
    // the off switch (rule 1.82) and the paint modes keep their full-frame
    // view; the identity argument above is what makes this a pure cost cut.
    if (dbgCtl.w < 0.5f && dlCtl.x >= 2.5f) {
        // KH_DLS_RANGE (26879): the same identity one step earlier. Past
        // 99.5% of the shadow view distance KhDlsRangeFade returns 0 and
        // KhDlsShadow answers lit for every light, so the multiply is a
        // no-op there too; one length against the pass camera decides it
        // before the per-light loop. dlsRange.w 0 (lane unfilled, mode 581)
        // falls through to the reach test exactly as before.
        if (dlsRange.w > 0.0f) {
            const float3 khw_rr = khw_w - dlsRange.xyz;
            const float  khw_rl = 0.995f * dlsRange.w;
            if (dot(khw_rr, khw_rr) >= khw_rl * khw_rl) return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        const int khw_rn = (int)dlCtl.y + (int)dlCtl.z;
        bool khw_reach = false;
        [loop] for (int khw_ri = 0; khw_ri < khw_rn && !khw_reach; ++khw_ri) {
            const int khw_rs = (int)dlLights[khw_ri * 6 + 5].z - 1;
            if (khw_rs < 0) continue;
            const float khw_rf = dlsMeta[khw_rs].w * 1.05f;
            if (khw_rf <= 0.0f) continue;
            const float3 khw_rd = khw_w - dlsMeta[khw_rs].xyz;
            khw_reach = dot(khw_rd, khw_rd) < 3.0f * khw_rf * khw_rf;
        }
        if (!khw_reach) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float3 khw_dx = ddx(khw_w);
    float3 khw_dy = ddy(khw_w);
    float khw_nrel = 1.0f;

    if (!khw_nrev) {
        // Two baselines. The taps are central differences, so each spans 2R
        // pixels of depth signal against one quantisation step.
        // KH_DLSW_NRM_FALLBACK (26876b) - WHAT "UNRESOLVED" IS ALLOWED TO MEAN.
        //
        // 26876a set khw_nrel to 0 on EVERY failure of the wide fit, and the
        // field found the cost immediately: a thin dark outline wherever
        // shadowed world geometry meets the SKY. At such a pixel the wide taps
        // land on sky, the fit is refused, rel goes to 0, and the factor falls
        // back to the FACING PRIOR - N.L = 1, maximum darkening - on the one
        // class of pixel where the prior has no business being applied. The
        // prior was reasoned for grazing ground under a lamp, and a silhouette
        // is not that.
        //
        // The error was letting one number mean two different things. There are
        // two ways to fail to resolve a normal and they want OPPOSITE answers:
        //
        //   THE NEIGHBOURHOOD IS NOT ONE SURFACE (a sky tap, a silhouette, the
        //   frame edge). Nothing about the prior applies. Retreat to the
        //   narrowest measurement still available and TRUST IT - that is
        //   exactly the 26875 behaviour, which had no rim of its own here.
        //
        //   THE NEIGHBOURHOOD IS ONE SURFACE AND THE FIT STILL WOBBLES. The
        //   only thing left that can do that is quantisation, and that is the
        //   case the prior was built for.
        //
        // So rel now falls below 1 in exactly ONE circumstance: both stencils
        // agree the neighbourhood is planar, and their normals still disagree.
        // Every refusal retreats instead, at full confidence. KhDlswPlane's
        // second-difference test is what makes the distinction available - it
        // is blind to slope and sensitive only to the surface breaking.
        const float khw_r1 = KH_DLSW_NRM_R;
        const float khw_r2 = KH_DLSW_NRM_R * 2.0f;
        float3 khw_ax, khw_ay, khw_bx, khw_by;
        const bool khw_ok1 = KhDlswPlane(i.pos.xy, khw_dims, khw_r1, khw_zl, khw_ax, khw_ay);
        const bool khw_ok2 = KhDlswPlane(i.pos.xy, khw_dims, khw_r2, khw_zl, khw_bx, khw_by);

        if (khw_ok1 && khw_ok2) {
            // The WIDE pair shades: best signal-to-quantisation of the two.
            khw_dx = khw_bx;
            khw_dy = khw_by;
            const float3 khw_n1 = cross(khw_ax, khw_ay);
            const float3 khw_n2 = cross(khw_bx, khw_by);
            const float khw_l1 = length(khw_n1);
            const float khw_l2 = length(khw_n2);
            if (khw_l1 > 1e-12f && khw_l2 > 1e-12f) {
                // Unsigned: the two planes may be wound oppositely without
                // disagreeing about the surface they describe.
                const float khw_ag = abs(dot(khw_n1 / khw_l1, khw_n2 / khw_l2));
                khw_nrel = saturate((khw_ag - KH_DLSW_NRM_AGREE) /
                                    (1.0f - KH_DLSW_NRM_AGREE));
            } else {
                khw_nrel = 0.0f;
            }
        } else if (khw_ok1) {
            // Planar at R, broken by 2R: an edge sits between the two. The
            // narrow fit is a real measurement of a real surface, so it is
            // used AS MEASURED - no prior, no darkening it did not earn.
            khw_dx = khw_ax;
            khw_dy = khw_ay;
            khw_nrel = 1.0f;
        } else {
            // Broken inside R: a silhouette. The 2x2 quad derivative taken
            // above straddles it and belongs to neither surface - that is the
            // pixel-wide outline, dark under 26876a's prior and bright under
            // 26876b's trust. Take the difference on the side that stays on
            // the centre's own surface instead, and it is a real measurement
            // again. Only if even that fails (an isolated pixel with no valid
            // neighbour on some axis) does the quad derivative stand.
            float3 khw_ox, khw_oy;
            const bool khw_orev = (dbgCtl.w >= 558.5f && dbgCtl.w < 559.5f);
            if (!khw_orev &&
                KhDlswOneSided(i.pos.xy, khw_dims, khw_zl, khw_w, khw_ox, khw_oy)) {
                khw_dx = khw_ox;
                khw_dy = khw_oy;
            }
            khw_nrel = 1.0f;
        }
    }

    float3 khw_n = cross(khw_dx, khw_dy);
    const float khw_nl = length(khw_n);
    if (!(khw_nl > 1e-9f)) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    khw_n /= khw_nl;
    const float3 khw_eye = castView[2].xyz - khw_w;
    if (dot(khw_n, khw_eye) < 0.0f) khw_n = -khw_n;

    // KH_DLSW_NRMVIEW (26876, mode 554) - HOW MUCH OF THE SCREEN HAS A NORMAL?
    //
    // WHITE where the two baselines agree and N.L is trusted, BLACK where they
    // do not and the factor falls back to the facing prior. The prediction this
    // is here to check: the black regions must coincide with the rings, and
    // must SPREAD as the camera lies down. If they do not - if 554 is white
    // across the banded ground - the normal is resolved after all and this
    // whole note is wrong, which is worth one screenshot to find out.
    if (dbgCtl.w >= 553.5f && dbgCtl.w < 554.5f) {
        return float4(khw_nrel, khw_nrel, khw_nrel, 1.0f);
    }

    // KH_DLS_WORLD_SHOW (mode 531): the reconstruction itself, as colour, so a
    // wrong world position is visible as a wrong pattern instead of as a
    // subtly wrong shadow. Metre-scale fract on each axis - a correct
    // reconstruction paints a stable 1 m grid that stays welded to the ground
    // as the camera moves, and a broken one swims or shears.
    // OUR OWN MESHES ARE NOT THE WORLD - tested AFTER the reconstruction now,
    // because KhMirInFront projects the world position into the mirror's own
    // depth space (the KhVolZ idiom) and therefore needs khw_w.
    //
    // Moving it here also removes a hazard flagged at 26842: the mask used to
    // return BEFORE ddx/ddy were taken for the normal, so the derivative
    // neighbourhood straddled a divergent branch at every mask boundary. The
    // reconstruction is a handful of instructions and runs on pixels we may
    // then discard, which is the right trade for well-defined derivatives.
    if (dbgCtl.w >= 538.5f && dbgCtl.w < 539.5f) {
        if (mirMeta.x < 0.5f) return float4(1.0f, 0.0f, 0.0f, 1.0f);   // not armed at all
        // BLACK where a mesh is genuinely in front and the pixel is refused,
        // BLUE where a mesh merely projects here but sits BEHIND the visible
        // world - the buried and clipping parts, which must still receive.
        // BLACK where our mask says a mesh is genuinely in front and the pixel
        // is refused; BLUE where the mask holds a mesh distance but it is
        // BEHIND the visible world (the buried and clipping parts, which must
        // still receive); WHITE where no mesh covers the pixel at all.
        const float khv_md = khDlsMask.Load(int3(int2(clamp(i.pos.xy,
                                 float2(0.0f, 0.0f),
                                 float2(mirMeta.y - 1.0f, mirMeta.z - 1.0f))), 0));
        const bool khv_cov = (khv_md > 0.0f) && (khv_md < 1.0e29f);
        const bool khv_frt = KhDlsMaskInFront(i.pos.xy, mirMeta.y, mirMeta.z, khw_zl);
        if (khv_frt) return float4(0.0f, 0.0f, 0.0f, 1.0f);
        if (khv_cov) return float4(0.0f, 0.0f, 1.0f, 1.0f);
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // KH_DLSW_MIRZ (26845, mode 540) - THE TEST'S OWN OPERANDS.
    //
    // Three builds produced the identical picture from three different
    // comparisons, which means the comparison was never the thing to look at -
    // its inputs were. This paints them: RED is our MESH'S distance from the
    // mirror depth, GREEN is the WORLD SURFACE'S distance from the same
    // projection, both scaled over 50 m, BLUE marks a mesh pixel.
    //   red and green both present and DIFFERING over the mesh -> the operands
    //     are good and only the margin is in question
    //   red flat, black or pinned      -> the mirror depth is not what it seems
    //   green flat or pinned           -> castViewN is not the mirror's matrix
    //   magenta (red, no green)        -> the world projection failed entirely
    // Whichever it is, it is visible rather than inferred, and it ends the
    // guessing this mask has cost.
    if (dbgCtl.w >= 539.5f && dbgCtl.w < 540.5f) {
        if (mirMeta.x < 0.5f) return float4(1.0f, 0.0f, 0.0f, 1.0f);
        // RED our mesh's distance from OUR mask, GREEN the world surface's
        // distance, both over 50 m; BLUE marks a covered pixel. Red and green
        // both present and differing over the mesh is the healthy reading.
        const float khz_m = khDlsMask.Load(int3(int2(clamp(i.pos.xy,
                                float2(0.0f, 0.0f),
                                float2(mirMeta.y - 1.0f, mirMeta.z - 1.0f))), 0));
        const bool khz_cov = (khz_m > 0.0f) && (khz_m < 1.0e29f);
        return float4(khz_cov ? saturate(khz_m / 50.0f) : 0.0f,
                      saturate(khw_zl / 50.0f), khz_cov ? 1.0f : 0.0f, 1.0f);
    }

 
    // CHUNK BOUNDARY (26846). The note below is long because it records why
    // three builds failed against the mirror's depth plane, and that reasoning
    // is the thing that stops a fourth. SPLIT, never trim.
    // COVERAGE, NOT THE DEPTH TEST (26846) - AND THIS IS A RETREAT ON PURPOSE.
    //
    // Three builds tried to decide whether our mesh is IN FRONT of the world
    // surface by comparing against the mirror target's depth plane, and mode
    // 540 finally showed why none of them could: the world side read ZERO
    // (magenta everywhere - red present, green absent), and VSMirror explains
    // it. That shader does NOT write the clip depth of the matrix it is given.
    // It OVERWRITES z with its own near plane:
    //
    //     l22 = f / (f - 0.05);   z = l22 * w - 0.05 * l22
    //
    // where f is derived from the engine's b2 block - so the plane's A and B
    // terms are l22 and -0.05*l22, NOT projection[2][2] and [3][2], which is
    // what every attempt fed it. Worse, the remap is CONDITIONAL on four tests
    // against engBlk, so on some frames it is that convention and on others it
    // is the plain one, and a reader has to know which.
    //
    // That is not a plane to build a shipping test on: its semantics belong to
    // another feature, are conditional, and can change without this code being
    // told. So the default goes back to COVERAGE, which the field confirmed
    // working at 26841 - it over-masks where a mesh sinks into terrain, taking
    // a bite out of the shadow at the intersection, and that is a visibly
    // smaller fault than painting the world's shading across the whole mesh.
    //
    // MODE 541 DOES NOT EXIST (corrected 26876, rule 1.73). The sentence that
    // stood here said "mode 541 keeps the depth test for whoever finishes it".
    // It is wired nowhere in this shader, nowhere in the C++, and it is not in
    // set_render_debug_sqf's whitelist - so setting it would have silently done
    // nothing, which is process failure 7 of the last campaign repeated as a
    // comment. The mirror-depth route was ABANDONED, not parked: doing it would
    // mean replicating VSMirror's conditional remap exactly, and the pass now
    // has its OWN depth-only mask with semantics nobody else owns, which is the
    // better of the two answers anyway.
    // KH_DLSW_MASK (26847): our own mask, in metres, against the world
    // surface's distance in metres. Both sides are the same quantity produced
    // by machinery we own, so there is no convention, no partition and no
    // remap to be wrong about - which is what the three mirror-based attempts
    // each foundered on in a different way.
    // KH_DLSW_TOUCH (26876b, mode 558) - DOES THIS PASS TOUCH OUR MESH AT ALL?
    //
    // The see-through has two candidate sources that look identical on a lit
    // surface and have opposite fixes: this pass leaking past the self-mask, or
    // the MESH receive path (KhDlsShadow inside DynLights / KhDynLightsPBR)
    // shading the mesh oddly on its own. No lane can separate them, because the
    // question is per-pixel and the answer is a colour.
    //
    //   GREEN  the self-mask refused this pixel - the world pass did not write
    //          here, so anything visible on it belongs to the mesh kernels
    //   RED    the world pass darkened this pixel. Red ON THE MESH is a mask
    //          leak and this pass owns the artifact
    //   BLACK  the pass ran and left the pixel alone
    //
    // If the vest comes back solid green and the outline is still there, stage
    // 4 is exonerated and the next build looks at stage 3 - which would also
    // mean the operator's mode-533 reading has drifted again and is worth
    // re-taking. One screenshot decides it either way.
    const bool khw_touch = (dbgCtl.w >= 557.5f && dbgCtl.w < 558.5f);
    // KH_DLSW_MASKCOV: coverage, not a verdict. A pixel our mesh fully covers
    // still returns here untouched - the interior is bit-identical to every
    // build before this - and only the boundary, where coverage is partial,
    // reaches the kernel and is blended back toward 1 in proportion below.
    // KH_DLSW_MASKCOVVIEW (26877, mode 576) - PAINT THE COVERAGE THE PASS USED.
    //
    // The one number nothing has ever shown. 539 paints the BINARY verdict, 558
    // paints where the pass wrote, 563 reverts the coverage to a verdict - and
    // between them they can say "the mask covers the mesh" without ever saying
    // BY HOW MUCH, which is the quantity the shipping path actually multiplies
    // by. On our mesh the interior must be pure white and only the silhouette
    // may be grey. Any grey INSIDE the mesh is the leak, drawn at its own exact
    // footprint, and its shape names the cause: the outline of background
    // geometry is the depth race (fixed by default at 26877, reverted by 575),
    // a uniform wash is a mask that is scaled or offset, and holes that follow
    // our own geometry are a caster missing from the mask's set.
    float khw_cov = 0.0f;
    const bool khw_covview = (dbgCtl.w >= 575.5f && dbgCtl.w < 576.5f);
    if (mirMeta.x >= 0.5f) {
        khw_cov = KhDlsMaskCov(i.pos.xy, mirMeta.y, mirMeta.z, khw_zl);
        if (khw_covview) return float4(khw_cov, khw_cov, khw_cov, 1.0f);
        if (khw_cov >= 0.999f) {
            return khw_touch ? float4(0.0f, 1.0f, 0.0f, 1.0f)
                             : float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    } else if (khw_covview) {
        return float4(1.0f, 0.0f, 0.0f, 1.0f);   // mask not armed at all
    }

    if (dbgCtl.w >= 530.5f && dbgCtl.w < 531.5f) {
        return float4(frac(khw_w.x), frac(khw_w.y), frac(khw_w.z), 1.0f);
    }
    // KH_DLSW_COARSE (26836, mode 536): the same reconstruction at a 100 m
    // scale. A 1 m grid is invisible if the world position is wrong by orders
    // of magnitude, or if it barely varies - both look like "no grid". At 100 m
    // a CORRECT reconstruction still paints broad bands that stay welded to the
    // ground as the camera moves, and a collapsed one paints a smooth
    // screen-locked ramp. Comparing 531 against 536 separates "wrong scale"
    // from "wrong entirely".
    if (dbgCtl.w >= 535.5f && dbgCtl.w < 536.5f) {
        return float4(frac(khw_w.x * 0.01f), frac(khw_w.y * 0.01f),
                      frac(khw_w.z * 0.01f), 1.0f);
    }

    // KH_DLS_ZBIAS (26874) PASSED THE MEASURED DEPTH STEP HERE AND 26875 PUT
    // IT BACK TO ZERO. Mode 550 showed the engine depth arriving in plateaus
    // (identical neighbours, then 100 mm cliffs) against a 40 mm KH_DLS_BIAS_M,
    // so covering the step looked like arithmetic rather than a theory. The
    // field then reported mode 0 and mode 552 INDISTINGUISHABLE - the term
    // changes nothing visible, so whatever draws the rings is not this compare
    // being flipped by receiver-depth error. Hypothesis nine, refuted like the
    // eight before it (KH_DLSW_BANDING ledger).
    //
    // The two extra depth Loads are gone with it; the khd_zunc parameter is
    // kept only so the next attempt has the channel already plumbed, and mode
    // 552 is now a no-op that should be deleted alongside it.
    float4 khw_tx;
    float3 khw_f = KhDlsWorldFactor(khw_w, khw_n, 0.0f, khw_nrel, khw_tx);
    // KH_DLSW_MASKCOV: g = a + (1-a)*f. At coverage 0 this is f exactly, so
    // every pixel with none of our mesh in it is untouched; at the boundary it
    // withholds darkening in proportion to how much of the pixel is ours.
    khw_f = lerp(khw_f, float3(1.0f, 1.0f, 1.0f), khw_cov);

    // KH_DLSW_ZLSTEP (26873, instrument 550) - IS zl ITSELF QUANTISED?
    //
    // 548 left one bilinear tap: four binary compares of the receiver depth
    // against four CONSTANT texel depths. Inside a texel nothing varies but the
    // receiver depth, so a surviving sub-texel band means that depth is banded.
    // dlswDepthFmt 41 checked the CONTAINER (R32_FLOAT) and not the CONTENT - a
    // 32-bit float carries a quantised value perfectly well if whatever wrote
    // it rounded first.
    //
    // This paints the per-pixel STEP in zl, over three decades: red 0-1 mm,
    // green 0-1 cm, blue 0-10 cm. A continuous depth field paints a smooth dim
    // wash that brightens with distance. A quantised one paints BLACK PLATEAUS
    // separated by bright step lines, and if those lines are the rings, nine is
    // the answer. Uses the same zl the shadow compare uses, one pixel apart.
    if (dbgCtl.w >= 549.5f && dbgCtl.w < 550.5f) {
        const float khw_sx = abs(KhCastZl(i.pos.xy + float2(1.0f, 0.0f), khw_dims) - khw_zl);
        const float khw_sy = abs(KhCastZl(i.pos.xy + float2(0.0f, 1.0f), khw_dims) - khw_zl);
        const float khw_s = max(khw_sx, khw_sy);
        return float4(saturate(khw_s * 1000.0f), saturate(khw_s * 100.0f),
                      saturate(khw_s * 10.0f), 1.0f);
    }

    // KH_DLSW_TEXEL (26871, instruments 545 and 546). Does the ring pattern
    // coincide with the shadow map's own texel grid, and does its severity
    // track the texel's screen footprint? Ledger at KH_DLSW_BANDING in the C++.
    if (dbgCtl.w >= 544.5f && dbgCtl.w < 546.5f) {
        if (khw_tx.w < 0.5f) return float4(1.0f, 1.0f, 1.0f, 1.0f);   // no sample here
        uint khw_mw, khw_mh, khw_me;
        khDlsMaps.GetDimensions(khw_mw, khw_mh, khw_me);
        if (dbgCtl.w < 545.5f) {
            // 545: the texel grid itself, as a checkerboard in map space. If
            // the rings ARE texels they land on this checker exactly.
            const float2 khw_tc = khw_tx.xy * (float)khw_mw;
            const float khw_ck = fmod(floor(khw_tc.x) + floor(khw_tc.y), 2.0f);
            return float4(khw_ck, khw_ck, khw_ck, 1.0f);
        }
        // 546: how many SCREEN pixels one texel spans, over 0..8. The world
        // size of a pixel comes from the reconstruction's own derivatives, so
        // this is the ratio that grows without bound as the camera lies down
        // toward a surface while the map stays at KH_DLS_MAP_PX.
        const float khw_pw = max(max(length(ddx(khw_w)), length(ddy(khw_w))), 1.0e-6f);
        const float khw_r = khw_tx.z / khw_pw;
        return float4(saturate(khw_r / 8.0f), saturate(khw_r / 64.0f),
                      saturate(khw_r / 512.0f), 1.0f);
    }

    // KH_DLS_WORLD_AMP (mode 532): the same verdict, exaggerated. The honest
    // factor is often a gentle darkening that is hard to judge against a night
    // scene; this squares the departure from 1 so the shadow's SHAPE and
    // EDGES can be checked against the mesh's own shadow before the magnitude
    // is trusted. Never a shipping value.
    if (dbgCtl.w >= 531.5f && dbgCtl.w < 532.5f) {
        return float4(saturate(1.0f - (1.0f - khw_f) * 3.0f), 1.0f);
    }
    if (khw_touch) {
        const bool khw_dark = (khw_f.r < 0.999f || khw_f.g < 0.999f || khw_f.b < 0.999f);
        return khw_dark ? float4(1.0f, 0.0f, 0.0f, 1.0f)
                        : float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    return float4(khw_f, 1.0f);
}
 float4 PSMaskCast(VSOut i) : SV_Target
{
    // The field then produced the.x signature of a CONSTANT z BIAS (drift
    // growing as fragments near, fine at range - relative error c/z), the
    // mark of a near-offset encoding, while the engine's own resolve reads.y.
    float2 dimsM = float2(castView[1].z, castView[1].w);
    uint dw, dh;
    sceneDepthTex.GetDimensions(dw, dh);
    int2 px = int2(i.pos.xy * float2(dw, dh) / max(dimsM, float2(1.0f, 1.0f)));
    float4 zt = sceneDepthTex.Load(int3(px, 0));
    float zl = KhCastZPick(zt);   // KH_CAST_ZPLANE (twin of KhCastZl's pick)

    bool khcNearOk = zl > 1.2f || castView[0].w >= 0.5f;

    if (!khcNearOk && zl > 0.05f) {
        // KH_CAST_ZPLANE TWIN (26793). These four taps used max(.x, .y) - a
        // THIRD plane rule, distinct from both KhCastZPick modes - while the
        // note above KhCastZPick claims ONE selector serves every read site.
        // The claim was false and the four taps also Loaded each texel TWICE.
        // Routed through the selector: at mode 0 the .y plane is empty
        // field-wide, so KhCastZPick returns .x and max(.x, 0) returned .x -
        // BIT-IDENTICAL. Modes 499/500 now reach this block as documented.
        float khcM0 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 2, dh >> 1), 0)));
        float khcM1 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 2), 0)));
        float khcM2 = KhCastZPick(sceneDepthTex.Load(int3(int2((dw * 3) >> 2, (dh * 3) >> 2), 0)));
        float khcM3 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 4), 0)));
        float khcHi = max(max(khcM0, khcM1), max(khcM2, khcM3));
        float khcLo = min(min(khcM0, khcM1), min(khcM2, khcM3));
        khcNearOk = khcHi > 1.2f ||
                    (khcLo > 0.05f && khcHi > khcLo * 1.3f);   // perspective structure
    }

    // KH_CAST_REPROJ (26785; sunCastBias2.z arms it, mode 498). The shade this
    // pixel needs is the one belonging to the world point THIS frame shows
    // here - but the only depth available at draw 0 is the previous frame's,
    // so a straight reconstruction answers for the world point the PREVIOUS
    // frame showed here. Solve for the source pixel instead: find s such that
    // projecting the world point behind s through THIS frame's view lands on
    // i.pos. The map is near-identity so the fixed-point iteration converges
    // in two steps; a disoccluded pixel has no correct source and settles on
    // its neighbour, which is the standard and acceptable failure.
    float2 khrp_s = i.pos.xy;
    if (sunCastBias2.z >= 0.5f) {
        // THREE iterations, and the count is not a guess: measured against
        // ground truth over the dump's own worst frame-to-frame motion (6.8 m,
        // 25 deg yaw, 21 deg pitch), the mean world error runs 25.1 m with no
        // solve, 4.0 after one step, 3.8 after two and 0.81 after three. ONE
        // step is WORSE THAN NONE on a pitch-dominated frame (23.3 against
        // 16.9) because the map overshoots before it converges - so the loop
        // must not be allowed to stop early, and a step is CLAMPED rather than
        // abandoned.
        float2 khrp_c = i.pos.xy;
        const float khrp_lim = 0.25f * length(dimsM);
        [unroll] for (int khrp_i = 0; khrp_i < 3; ++khrp_i) {
            float3 khrp_w = KhCastWorld(khrp_c, dimsM, KhCastZl(khrp_c, dimsM));
            float2 khrp_e = i.pos.xy - KhCastPixN(khrp_w, dimsM);
            const float khrp_el = length(khrp_e);
            if (khrp_el > khrp_lim) khrp_e *= khrp_lim / max(khrp_el, 1.0e-6f);
            khrp_c = clamp(khrp_c + khrp_e, float2(0.0f, 0.0f), dimsM - 1.0f);
        }
        // ACCEPT ONLY A SOLVE THAT LANDED. Re-measure the residual and take the
        // result only if it projects back within 16 px of this pixel (256 =
        // 16 squared). 16 px is measured, not chosen: at 4 px the test rejects
        // genuine convergence (a 0.25 m world residual at 50 m already subtends
        // 5 px), and past 32 px it starts admitting disocclusions. A
        // disoccluded pixel has no source and will not converge; it keeps the
        // unreprojected sample, which is exactly today's behaviour. So this
        // path can improve a pixel or leave it alone, never worsen it.
        float3 khrp_wf = KhCastWorld(khrp_c, dimsM, KhCastZl(khrp_c, dimsM));
        float2 khrp_ef = i.pos.xy - KhCastPixN(khrp_wf, dimsM);
        if (dot(khrp_ef, khrp_ef) <= 256.0f) khrp_s = khrp_c;
        zl = KhCastZl(khrp_s, dimsM);
    }
    float3 pw = KhCastWorld(khrp_s, dimsM, zl);

    if (thmParams.w >= 0.5f) {
        float khtsH = KhThmHeight(pw.xz);
        if (khtsH > -1.0e5f && abs(pw.y - khtsH) < 0.35f) pw.y = khtsH;
    }

    float hit = 0.0f;

    if (sunMeta.x >= 0.5f) {
        bool near_ok = false;
        float stretch = 2.0f + 3.0f / max(abs(castView[2].y), 0.15f);

        // The test is now distance to the caster's AABB SURFACE (zero inside
        // the volume): interior and corners are in reach at ANY size, while
        // the margin keeps the same capped formula measured from the surface
        // - the far-plane-garbage damage bound the cap was built for (the
        // overcast lesson) is preserved.
        if (localityMeta.y >= 0.5f) {
            // EXTENDED list (t2, uncapped): same tube-tight test, the caster
            // count is no longer bounded by the CB layout.
            int lc = (int)localityMeta.x;

            if (castMat[0].w > 0.0f) {
                // KH_CAST_OCC: constant time in the caster count.
                int2 khoC = (int2)floor((pw.xz - float2(castMat[1].w, castMat[2].w)) * castMat[0].w);
                if (khoC.x >= 0 && khoC.y >= 0 && khoC.x < 256 && khoC.y < 256) {
                    float2 khoY = khrCastOcc.Load(int3(khoC, 0));
                    if (pw.y >= khoY.x && pw.y <= khoY.y) near_ok = true;
                }
            } else {
                [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                    float3 lce = khrLocalityExt[li * 2].xyz;
                    float3 lhe = khrLocalityExt[li * 2 + 1].xyz;
                    float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                    if (KhCastReach(pw, lce, lhe, lr)) near_ok = true;   // KH_CAST_REACH_DROP
                }
            }
        } else if (localityMeta.x >= 0.5f && localityMeta.x <= 16.5f) {
            int lc = (int)localityMeta.x;

            [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                float3 lce = locality[li * 2].xyz;
                float3 lhe = locality[li * 2 + 1].xyz;
                float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                if (KhCastReach(pw, lce, lhe, lr)) near_ok = true;   // KH_CAST_REACH_DROP (twin)
            }
        } else {
            float castR = length(sizeAxes.xyz) * 0.5f;
            float reach = min(castR * stretch, max(600.0f, castR * 24.0f));
            // KH_CAST_REACH_DROP (twin): the combined-bounds fallback sweeps too.
            near_ok = KhCastReach(pw, centerSize.xyz, sizeAxes.xyz * 0.5f, reach);
        }

        // zl floor 1.2 m: if the captured depth texture transiently holds
        // aliased non-depth content (normalized values <= 1), EVERY pixel
        // reconstructs to ~the camera position - inside reach whenever the
        // player stands near a caster, MIN-darkening the whole screen (the
        // era-independent overcast mode).
        if ((near_ok && khcNearOk) || localityMeta.w >= 0.5f) {
            hit = SunShadowOcclusion(pw);   // near floor: whole-texture verdict above
            hit *= KhSunRangeFade(pw);   // range fade (at the helper)
        }

        // What remains is either a wrong pw reaching a correct sun-map test,
        // or our paint never reaching the screen on those frames at all - and
        // the second is no longer a remote possibility, because
        // fireMaskSrvFires runs at 0.27 per cycle against fireClampPaints at
        // 0.51. FLOOD (localityMeta.w >= 1.5, setRenderDebug 34).
        if (localityMeta.w >= 1.5f) hit = 1.0f;
    } else {
        // AABB-shaped by construction.
        float3 sd = castView[2].xyz;
        float3 sds = float3(
            abs(sd.x) > 1e-6f ? sd.x : 1e-6f,
            abs(sd.y) > 1e-6f ? sd.y : 1e-6f,
             abs(sd.z) > 1e-6f ? sd.z : 1e-6f);
        float3 inv = 1.0f / sds;
        float3 bmin = centerSize.xyz - sizeAxes.xyz * 0.5f;
        float3 bmax = centerSize.xyz + sizeAxes.xyz * 0.5f;
        float3 ta = (bmin - pw) * inv;
        float3 tb = (bmax - pw) * inv;
        float3 tmn = min(ta, tb);
        float3 tmx = max(ta, tb);
        float tn = max(max(tmn.x, tmn.y), tmn.z);
        float tf = min(min(tmx.x, tmx.y), tmx.z);

        // Reach = a few diagonals stretched by sun elevation (low sun = long
        // shadows); beyond it, geometrically impossible - never darken.
        // LOCALITY CLAMP: a ground point can only be shadowed by a mesh
        // within plausible shadow reach of it.
        float castR = length(sizeAxes.xyz) * 0.5f;
        float reach = castR * (2.0f + 3.0f / max(abs(castView[2].y), 0.15f));
        float3 toCast = centerSize.xyz - pw;
        float near_enough = (dot(toCast, toCast) < reach * reach) ? 1.0f : 0.0f;
        hit = (tf >= max(tn, 0.05f) && khcNearOk && near_enough > 0.5f) ? 1.0f : 0.0f;   // near floor: whole-texture verdict above
    }

    // Full occlusion here; the APPLY colors the shadow by the ambient floor
    // (amb/(amb+sun)) - depth and hue both derive from the sky.
    float shade = 1.0f - hit * saturate(castView[2].w);
    return float4(shade, shade, shade, 1.0f);
}

// Fullscreen triangle from SV_VertexID: no vertex buffer, no input layout
VSOut VSFullscreen(uint vid : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    o.wpos = float3(0.0f, 0.0f, 0.0f);
    o.nrm = float3(0.0f, 1.0f, 0.0f);
    o.wrel = float3(0.0f, 0.0f, 0.0f);   // (fullscreen path: no self sampling)
    o.icol = color;   // KH_INSTANCING: every VSOut carries the colour lane (PSEffect reads the CB directly)
    return o;
}

float4 PSMain(VSOut i) : SV_Target
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
    // TWIN: PSMain / PSComposite / PSEffect / PSOwner.
    if (shadowMeta2.x < 0.5f && shadowMeta2.y > 0.0f && i.pos.w > shadowMeta2.y) discard;
    // The map was built every frame and nothing read it. Identical block;
    // inert on the flush path where the lanes are zero.
    if (shadowMeta2.w > 0.5f && shadowMeta2.z > 0.5f &&
        KhOwnerRejects(i.pos.xy, i.pos.z, shadowMeta2.z, shadowMeta2.w)) {
        // Visual 33 (mode 441) paints the rejection instead: the arming
        // proof. RETURN after the discard.
        if (dbgCtl.x >= 32.5f && dbgCtl.x < 33.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);
        discard;
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    // Punch-through / overlay-occlusion guard, flush-path edition: the same
    // contract as PSComposite's. The CPU arms tight margins only for
    // SINGLE-SAMPLE snapshots, matching this texture's declaration, so MSAA
    // frames stand down by construction.
    if (fxParams1.x < 1e8f) {
        int2 gpx = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        // Settled remap-decode linearization (see KhSceneMeters).
        float sceneZ = KhSceneMeters(KhSceneLoad(gpx));
        if (i.pos.w > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;
    }

    // ANALYTIC TERRAIN LANE (flush-path twin; see PSComposite's note,
    // endpoint test included - the half-buried correction).
    if (thmParams.w >= 0.5f) {
        float khtC = 2.0e9f;
        float khtHe = KhThmHeight(i.wpos.xz);
        if (khtHe > -1.0e5f) khtC = i.wpos.y - khtHe;

        if (i.pos.w >= thmMeta.w) {
            // 1.5-cell aliasing slack: see the PSComposite twin.
            float khtMc = KhThmClearance(fxParams0.xyz, i.wpos) + 1.5f * thmParams.z;
            if (khtMc < khtC) khtC = khtMc;
        }

        if (khtC < 1.5e9f && khtC < -thmMeta.z) discard;
    }

    // DEBUG VISUAL 15: PAINTER FORENSICS (see PSComposite's orange twin):
    // pixels owned by THIS path - the flush repaint / plain fallback - paint
    // CYAN. Cyan at the punch-through convicts an unguarded or mis-encoded
    // flush draw as the painter.
    if (dbgCtl.x >= 14.5f && dbgCtl.x < 15.5f) return float4(0.0f, 1.0f, 1.0f, 1.0f);

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

    if (dbgCtl.x >= 0.5f && dbgCtl.x < 3.5f) {
        if (dbgCtl.x < 1.5f) return float4(1.0f, 0.0f, 1.0f, 1.0f);

        if (dbgCtl.x < 2.5f) {
            int2 dpx = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
            float dz = KhSceneMeters(KhSceneLoad(dpx)) - i.pos.w;
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
    float smf = 1.0f;
    float  khStenZ = KhVolZ(i.wpos);
    float2 khStenG = float2(ddx(khStenZ), ddy(khStenZ));
    // Taken here, outside the divergent N.L branch, for the same quad-op
    // legality as the pair above. TWIN EDIT: PSMain and PSComposite carry the
    // identical line.
    // THE SLICE IS A UNITS MISMATCH BETWEEN WHAT WE WRITE AND WHAT WE
    // PSMain writes no depth: the raster tap reads i.pos.z. (PSComposite's
    // ARB variant declares SV_Depth and carries the khaODepth note - the one
    // place the twins legitimately differ.)
    float2 khStenR = i.pos.xy;
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
    float3 khStenP = float3(i.pos.z, ddx(i.pos.z), ddy(i.pos.z));
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
    // MODE 167 (lighting0.y == 3): cascade receive forced lit
        // the split's cascade arm. TWIN EDIT: PSMain and PSComposite
        // identical.
        if (lighting0.y < 2.5f || lighting0.y >= 3.5f) {
            // lighting0.y 59 (mode 381) restores the absolute interpolant:
            // the one-switch A/B (crawl and jitter return together). TWIN
            // EDIT: PSMain and PSComposite identical.
            if (maskMeta.x >= 0.5f) smf = ShadowBandFactor(
                (lighting0.y >= 58.5f && lighting0.y < 59.5f)
                    ? i.wpos : (i.wrel + sunOrigin.xyz));
            else                    smf = ShadowMapFactor(i.wpos);   // yzw re-lettered (were zero)
        }
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
                          ? KhVolSoftScene(khStenR, khStenP, KH_STEN_TOL_W(i.pos.w), 6)
                          : KhStenUnit(i.wpos, i.pos.xy, khStenG);
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
            float khStRf = (lighting0.y >= 42.5f && lighting0.y < 43.5f)
                         ? 1.0f : KhSunRangeFade(i.wpos);
            smf *= 1.0f - (1.0f - khStenU) * khStRf;
        }
    }

    // Slices surviving this are painted by the ENGINE over our pixels, not by
    // any term of ours. TWIN EDIT: PSMain and PSComposite identical.
    if (lighting0.y >= 1.5f && lighting0.y < 2.5f) smf = 1.0f;
    // KH_GEO_HORIZON (code 81 / mode 455): minted 26713, read identical to
    // 0, wiped 26714, code burned. A residual copy of the arm survived in
    // THIS twin (never in PSComposite; no ladder emitted 81, so it was dead)
    // until the 26719 twin-parity check removed it. TWIN EDIT: PSMain and
    // PSComposite identical from here to the shading call.
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

    if (maskMeta.y >= 0.5f) {   // mode 71 paint arm (function scope, not the 4.5-7.5 block above)
        float kmv = khShadowMask.Load(int3(KhMaskPx(i.pos.xy), 0)).r;
        return float4(kmv, kmv, kmv, 1.0f);
    }

    if (maskMeta.z >= 0.5f) return KhStenPaintU(i.wpos, i.pos.xy, maskMeta.z);

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
    // CHUNK BOUNDARY - the visual-25 ledger took PSMain's
    // segment 906 B past the 16380-byte MSVC token cap (C2026). Third catch
    // by the sweep gate in three builds; the visual ladders are prose-heavy
    // and this segment is now the one to watch.
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
    // CHUNK BOUNDARY - FIFTH C2026 CATCH OF THIS
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
                float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
                trans = exp(-distM * dens * 0.0153f);
            }
        }

        trans *= KhHazeT(distM, hgt, camY, fogSkyCol.w);
        float khaAR = trans;
        float khaBt = khaFbOn ? exp(-khaFbB * fogBelow.x) : 1.0f;
        trans *= khaBt;
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

    // CHUNK BOUNDARY - the visual-22 ledger took this segment
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
    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);

    // DEBUG VISUAL mode 8: the perceptual-composite transit probe (see
    // PSComposite's note - same channels, this path's own capture read).
    // Bounded: modes 9-13 are PSComposite guard forensics; here they fall
    // through to normal shading (the catch-all routing-bug shape, fixed a
    // third time).
    if (dbgCtl.x >= 7.5f && dbgCtl.x < 8.5f) {
        float3 pscn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float plum = dot(pscn / (1.0f + pscn), float3(0.333f, 0.333f, 0.334f));
        return float4(blendCtl.x, plum, a, 1.0f);
    }

    if (blendCtl.x >= 0.5f) {
        // BACKGROUND TRUST: RETIRED ON THIS PATH (the distant-tint fix).
        // PSComposite's rule stands untouched - mid-frame is still mid-frame
        // there.
        float3 scn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float3 ts = scn / (1.0f + scn);
        float3 tl = lc / (1.0f + lc);
        // KH_DLSW_MESHOPAQUE (26876g, mode 566): force the mix to full
        // opacity. At weight 1 the Reinhard round trip is an exact identity and
        // the scene capture cannot contribute, so if the see-through vanishes
        // under 566 this Load is the carrier and the fix is upstream, in
        // whichever factor of the alpha is short of 1. TWIN of PSComposite.
        float3 tm = lerp(ts, tl, (KhDlsMeshDbg() == 566) ? 1.0f : a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // cap ~HDR 255
    }

    return float4(lc, a);
}
