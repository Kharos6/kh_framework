// static.hlsl - concatenated after cb.hlsl into the static unit by C++ (no
// #include). Any edit changes the unit's shader cache key.

// Both entry points are wrappers over KhVsCore - the per-object one hands in
// the CB lanes, the instanced one the stream's. Twin: VSComposite /
// VSCompositeInst.
VSOut VSMain(VSIn i)
{
    VSOut o;
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

// The bucket wrapper: the record at the lane's slot supplies what the CB
// supplies above; the rebase-relative centre is the record centre minus the
// pass camera (khPass), a correctly rounded float difference.
VSOut VSMainInst(VSIn i, VSInst n)
{
    VSOut o;
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

// The stencil-volume seam transport's instance stream: the sun ladder's own
// lane (KhInstLane / VSInSun), so layout_sundepth binds both entry points
// below. NORMAL is declared to keep that layout's slot-0 shape and is unread by
// the mirror twin.
struct VSInSeam {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;
};

// Loop A's twin - the footprint into the engine's volume buffer. Same KhVsCore
// and the same stenVol2.z = 3 remap as VSMain; only the lane source differs.
// The rebase arm is khPass.w, NOT centerRel.w (never filled on an instanced
// draw), so the C++ side must fill khPass for this pass or the absolute branch
// is taken against a rebased viewProj.
float4 VSSeamInst(VSInSeam i) : SV_Position
{
    KhObjRec r = khObjs[i.islot];
    precise float3 khsi_rel = r.pos.xyz - khPass.xyz;
    float4 khsi_pos;
    float3 khsi_wpos, khsi_wrel, khsi_nrm;
    KhVsCore(i.pos, i.nrm, r.pos.xyz, khsi_rel, khPass.w, r.size.xyz,
             r.rot0.xyz, r.rot1.xyz, r.rot2.xyz, khsi_pos, khsi_wpos, khsi_wrel, khsi_nrm);
    return khsi_pos;
}

// The sun-depth stream is the same lane the colour buckets use; the record
// supplies centre, size and rotation.
struct VSInSun {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;   // y = the caster's alpha (envelope applied, KH_CAST_ALPHA).
};

struct VSInMir { float3 pos : POSITION; };
// The mirror prepass's clip transform, factored so the per-object entry and its
// instanced twin cannot drift. Takes the position in the visible box's own
// space, then rebuilds the projection with the near plane at 0.05 from the live
// b2 low pair - deliberately NOT KhVsCore's stenVol2.z = 3 remap, which
// recovers the engine's own z.
float4 KhMirClip(float3 khmv_tp)
{
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
float4 VSMirror(VSInMir i) : SV_Position
{
    float3 khmv_l = KhRotate(i.pos * sizeAxes.xyz);
    float3 khmv_wp = centerSize.xyz + khmv_l;
    return KhMirClip((centerRel.w > 0.5f) ? (centerRel.xyz + khmv_l) : khmv_wp);
}

// Loop B's twin. KhRotate's rotated branch over the record's own rows
// (kh_objrec_fill always writes rot0.w = 1 and identity rows for an unrotated
// object).
float4 VSMirrorInst(VSInSeam i) : SV_Position
{
    KhObjRec r = khObjs[i.islot];
    float3 khmi_l = KhRotateR(i.pos * r.size.xyz, r.rot0.xyz, r.rot1.xyz, r.rot2.xyz);
    float3 khmi_wp = r.pos.xyz + khmi_l;
    precise float3 khmi_rel = r.pos.xyz - khPass.xyz;
    return KhMirClip((khPass.w > 0.5f) ? (khmi_rel + khmi_l) : khmi_wp);
}
float4 VSSunDepth(VSInSun i) : SV_Position
{
    KhObjRec r = khObjs[i.islot];
    float3 lp = i.pos * r.size.xyz;
    float3 wp = r.pos.xyz + lp.x * r.rot0.xyz + lp.y * r.rot1.xyz + lp.z * r.rot2.xyz;
    return mul(float4(wp - sunOrigin.xyz, 1.0f), viewProj);
}

// KH_CAST_DISSOLVE - the engine's rule for far casters, in the map: each CASTER
// FRAGMENT is dithered out by its own distance from the camera (sunOrigin) over
// the same band as the range fade (KH_SUN_FADE_START .. 0.995 of mirMeta.w).
// Per fragment, so the far corner of a mesh dissolves before its near one; a
// nearer occluder still writes its depth, so unlike a depth-gap fade this
// cannot lighten a doubly-shadowed spot. mirMeta.w = 0 (a caster-anchored map,
// no camera) leaves every fragment.
struct VSOutSunD { float4 pos : SV_Position; float3 wp : TEXCOORD0; };

VSOutSunD VSSunDepthD(VSInSun i)
{
    VSOutSunD o;
    KhObjRec r = khObjs[i.islot];
    float3 lp = i.pos * r.size.xyz;
    float3 wp = r.pos.xyz + lp.x * r.rot0.xyz + lp.y * r.rot1.xyz + lp.z * r.rot2.xyz;
    o.pos = mul(float4(wp - sunOrigin.xyz, 1.0f), viewProj);
    o.wp = wp;
    return o;
}

// 4x4 Bayer, 16 levels - the SAME period as the alpha twin's: the receiver's
// soft filter spans a ~4x4 texel footprint, so a period-4 pattern is averaged
// whole and reads as uniform coverage (an 8x8 pattern speckles). The map
// windows move in whole texels, so the pattern is fixed in the world.
float KhSunDitherMap(float2 khdm_px)
{
    static const float khdm_b[16] = { 0.0f, 8.0f, 2.0f, 10.0f, 12.0f, 4.0f, 14.0f, 6.0f,
                                      3.0f, 11.0f, 1.0f, 9.0f, 15.0f, 7.0f, 13.0f, 5.0f };
    int2 khdm_p = int2(khdm_px) & 3;
    return (khdm_b[khdm_p.y * 4 + khdm_p.x] + 0.5f) / 16.0f;
}

void PSSunDepthD(VSOutSunD i)
{
    if (mirMeta.w < 0.5f) return;
    float khsd_f = KhSunRangeFadeAt(i.wp, sunOrigin.xyz);
    clip(khsd_f - KhSunDitherMap(i.pos.xy));
}

#if KH_TEXTURED
// KH_CAST_ALPHA - alpha-aware casting, in the map. Both consumers of the
// private sun-depth maps (the self kernels and PSMaskCast's world cast) read
// the same texels, so alpha enters once, here.
struct VSInSunA {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    uint   islot : TEXCOORD4;   // KH_OBJBUF (the plain twin's lane).
    float3 ilane : TEXCOORD5;   // y = the caster's colour alpha (envelope applied).
};
struct VSOutSunA { float4 pos : SV_Position; float2 uv : TEXCOORD0; float alpha : TEXCOORD1;
                   float3 wp : TEXCOORD2; };   // wp: for the dissolve.

VSOutSunA VSSunDepthA(VSInSunA i)
{
    VSOutSunA o;
    KhObjRec r = khObjs[i.islot];
    float3 lp = i.pos * r.size.xyz;
    float3 wp = r.pos.xyz + lp.x * r.rot0.xyz + lp.y * r.rot1.xyz + lp.z * r.rot2.xyz;
    o.pos = mul(float4(wp - sunOrigin.xyz, 1.0f), viewProj);
    o.uv = i.uv;
    o.alpha = i.ilane.y;
    o.wp = wp;
    return o;
}

// 4x4 Bayer thresholds, (index + 0.5) / 16: every 4x4 window of map texels
// covers the requested fraction to a sixteenth. Anchored to the map's texel
// grid.
float KhSunDither(float2 khsd_px)
{
    static const float khsd_b[16] = { 0.0f, 8.0f, 2.0f, 10.0f, 12.0f, 4.0f, 14.0f, 6.0f,
                                      3.0f, 11.0f, 1.0f, 9.0f, 15.0f, 7.0f, 13.0f, 5.0f };
    int2 khsd_p = int2(khsd_px) & 3;
    return (khsd_b[khsd_p.y * 4 + khsd_p.x] + 0.5f) / 16.0f;
}

void PSSunDepthA(VSOutSunA i)
{
    KhMatLoad((uint)matCtl.x);   // KH_MAT_TABLE: per-submesh draw, the CB lane.
    float khsa_a = i.alpha;
    int khsa_mode = (int)matParams0.y;   // 0 opaque, 1 cutout, 2 blend (kh_bind_material).
    // The material's alpha by its own route (diffuse.a by default; 1 when no
    // map is bound), exactly the colour pass's sampling.
    float khsa_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khsa_mode == 1) clip(khsa_t - matParams0.z);   // Cutout: the cutoff kills, survivors cast full.
    else if (khsa_mode == 2 && KhMatRouteTexel(matParams3.y, 1.0f, i.uv) < 0.9f) khsa_a *= khsa_t;
    // The distance band multiplies INTO the coverage and one dither decides, so
    // a translucent caster dissolves with distance exactly as an opaque one
    // does (two clips on the same pattern would compose as a min, not a
    // product).
    float khsa_f = (mirMeta.w >= 0.5f) ? KhSunRangeFadeAt(i.wp, sunOrigin.xyz) : 1.0f;
    if (khsa_f < 0.9999f) {
        float khsa_c = khsa_a * khsa_f;
        clip(khsa_c - 0.004f);
        clip(khsa_c - KhSunDitherMap(i.pos.xy));
        return;
    }
    if (khsa_a >= 0.996f) return;                       // Solid.
    clip(khsa_a - 0.004f);                              // Transparent: casts nothing.
    clip(khsa_a - KhSunDither(i.pos.xy));               // Partial: dithered coverage.
}

// The seam footprint carries depth-writing texels only: a translucent texel
// writes no depth in the colour pass, so the engine's stencil count at its
// pixel must be measured at what is behind it, or the ground behind the glass
// inherits the glass's shadow. Whole translucent objects never reach it.
void PSInjDepthA(VSOut i)
{
    KhMatLoad(i.matIx);   // KH_MAT_TABLE.
    int khfa_mode = (int)matParams0.y;
    float khfa_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khfa_mode == 1) clip(khfa_t - matParams0.z);
    // The colour pass's own verdict.
    else if (khfa_mode == 2) clip(KhMatRouteTexel(matParams3.y, 1.0f, i.uv) - 0.9f);
}

// The world pass's self-mask carries depth-writing texels only (the seam
// footprint's rule): a cutout hole or a blend material's translucent texel
// writes no depth, so the surface at its pixel is whatever is behind it and
// must receive the dynamic-light shadow like the ground beside it. Survivors
// write the view distance PSDlsMask writes (SV_Position.w is the clip w, the
// quantity VSDlsMask stores).
float4 PSDlsMaskA(VSOut i) : SV_Target
{
    KhMatLoad(i.matIx);   // KH_MAT_TABLE.
    int khma_mode = (int)matParams0.y;
    float khma_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khma_mode == 1) clip(khma_t - matParams0.z);
    // The colour pass's own verdict.
    else if (khma_mode == 2) clip(KhMatRouteTexel(matParams3.y, 1.0f, i.uv) - 0.9f);
    return float4(i.pos.w, 0.0f, 0.0f, 0.0f);
}
#endif

// Analytic mask cast: per-pixel ray-vs-AABB toward the sun, drawn into the
// engine's screen-space shadow mask with multiply blending. castMat[0..2] =
// view rows 0..2; castView[0] = view row 3 (translation); castView[1] = (fov.x,
// fov.y, maskW, maskH); castView[2] = sunDir.xyz + strength in w. Scene linear
// depth at t0.
Texture2D<float4> sceneDepthTex : register(t0);
float KhSceneLoad(int2 p) { return sceneDepthTex.Load(int3(p, 0)).x; }

// Pre-mesh scene colour capture (t3, single-sample) - the perceptual-composite
// blend source for the flush mesh pass.
Texture2D<float4> sceneColorTex : register(t3);
// Extended per-caster locality list (localityMeta.y arms): 2 float4 per caster,
// [2i] center / [2i+1] half extents, engine axes - the uncapped twin of the
// 16-pair CB list. Bound only at the mask cast fire.
StructuredBuffer<float4> khrLocalityExt : register(t2);
// The caster-occupancy grid: a strict superset of the loop, so no caster's
// shadow can be lost.
Texture2D<float2> khrCastOcc : register(t35);

// The reach is a shadow, not a sphere: sweep the caster down-sun onto the
// receiver's own height plane and test the segment (t0/t1 are where the
// caster's base and top land at pw.y). lr is pure lateral slack.
// SunShadowOcclusion answers 0 where no occluder stands, so this gate can only
// cost ALU, never a wrong verdict.
 
bool KhCastReach(float3 khcr_p, float3 khcr_c, float3 khcr_h, float khcr_lr)
{

    float3 khcr_sd = castView[2].xyz;   // Frozen sun, toward the light.
    float  khcr_hl = length(khcr_sd.xz);
    float  khcr_sy = max(abs(khcr_sd.y), 0.02f);
    float  khcr_k  = khcr_hl / khcr_sy;   // Metres down-sun per metre of drop.
    float2 khcr_u  = khcr_hl > 1.0e-4f ? (-khcr_sd.xz / khcr_hl) : float2(0.0f, 0.0f);
    // Where this caster's base and top land at the receiver's height.
    float  khcr_t0 = max((khcr_c.y - khcr_h.y) - khcr_p.y, 0.0f) * khcr_k;
    float  khcr_t1 = max((khcr_c.y + khcr_h.y) - khcr_p.y, 0.0f) * khcr_k;
    float2 khcr_q  = khcr_p.xz - khcr_c.xz;
    float  khcr_t  = clamp(dot(khcr_q, khcr_u), khcr_t0, khcr_t1);
    float2 khcr_r  = khcr_q - khcr_u * khcr_t;
    float2 khcr_d  = max(abs(khcr_r) - khcr_h.xz, 0.0f);
    if (dot(khcr_d, khcr_d) < khcr_lr * khcr_lr) return true;
    // Superset contract: the sweep is not a superset of the sphere on its own
    // (a receiver directly under a floating caster has t clamped to t0), and
    // this gate may only ever add reach, never remove it. A false positive
    // costs one SunShadowOcclusion that answers 0.
    float3 khcr_ld = max(abs(khcr_p - khcr_c) - khcr_h, 0.0f);
    return dot(khcr_ld, khcr_ld) < khcr_lr * khcr_lr;
}

// The two-plane depth carries a far plane in .x and a near/witness plane in .y;
// .x carries a constant z bias (relative error c/z), so .y is taken whenever it
// is non-zero. One selector for every read site.
float KhCastZPick(float4 khzp_t)
{

    return khzp_t.y > 0.0f ? khzp_t.y : khzp_t.x;                 // Live rule.
}
float KhCastZl(float2 khcz_s, float2 khcz_dims)
{
    uint khcz_w, khcz_h;
    sceneDepthTex.GetDimensions(khcz_w, khcz_h);
    int2 khcz_p = int2(khcz_s * float2(khcz_w, khcz_h) / max(khcz_dims, float2(1.0f, 1.0f)));
    float4 khcz_t = sceneDepthTex.Load(int3(khcz_p, 0));
    return KhCastZPick(khcz_t);
}
// Screen pixel -> world, through the frozen view the depth was rendered with
// (castView[0] is its translation row, castMat its inverse rotation).
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
// World -> screen pixel through this frame's view: the inverse direction of the
// above, against a different matrix - the point.
float4 PSMaskPrime(VSOut i) : SV_Target
{
    const float khpm_v = KH_PRIME_V;
    return float4(khpm_v, khpm_v, khpm_v, 1.0f);
}
 
// VSMirror overwrites z with its own conditional near plane, so its depth
// cannot be read back. The mask is a single R32_FLOAT target holding the linear
// view distance, in metres, of the nearest of our meshes at every pixel - the
// same quantity KhCastZl returns for the world, so the comparison is metres
// against metres.
struct VSOutDM { float4 pos : SV_Position; float dist : TEXCOORD0; };

VSOutDM VSDlsMask(VSIn i)
{
    // The same object transform every mesh draw uses, fp32 rebase included;
    // centerRel.w = 0 keeps the absolute path.
    const float3 khdm_l = KhRotate(i.pos * sizeAxes.xyz);
    float3 khdm_w = (centerRel.w > 0.5f) ? (centerRel.xyz + khdm_l)
                                         : (centerSize.xyz + khdm_l);
    float4 khdm_c = mul(float4(khdm_w, 1.0f), viewProj);
    VSOutDM o;
    o.pos = khdm_c;
    // w of a perspective clip position is the view-space distance along the
    // forward axis - the quantity KhCastZl returns for the world surface.
    o.dist = khdm_c.w;
    return o;
}

float4 PSDlsMask(VSOutDM i) : SV_Target
{
    return float4(i.dist, 0.0f, 0.0f, 0.0f);
}

// The mask, read back by the world pass; the sentinel means no mesh covers it.
// It carries our depth-writing texels only (an alpha caster is drawn through
// PSDlsMaskA; an invisible or whole-translucent object is not drawn), so
// 'nearer than the world' means 'our surface is what the eye sees here'.
Texture2D<float> khDlsMask : register(t37);
// khdw_zl is the world surface's distance at the same pixel. Both are metres.
float KhDlsMaskCov(float2 khmc_px, float khmc_w, float khmc_h, float khmc_zl)
{
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
            if (khmc_d < khmc_zl * 0.995f) khmc_n += 1.0f;   // The rule.
        }
    }

    return khmc_n * (1.0f / 9.0f);
}

// One plane tap at a chosen pixel radius, for the two-baseline normal in
// PSDlsWorld. Returns the two world-space central differences, or false when
// any of the four taps is not a surface. Widening the stencil buys
// signal-to-quantisation directly; the taps are refused rather than clamped
// when one lands on sky or off the depth, and the caller keeps the 2x2 quad
// derivative as the fallback (near a silhouette the narrow answer is the honest
// one, told through khw_nrel).
static const float KH_DLSW_NRM_R = 3.0f;        // Base baseline, pixels.
// The second-difference bound that decides whether the stencil is looking at
// one surface: for any plane zl(+r) + zl(-r) - 2*zl is exactly zero whatever
// its slope, so this is blind to orientation and sensitive only to the
// neighbourhood breaking.
static const float KH_DLSW_NRM_FLAT = 0.02f;
// The agreement floor for the two baselines: cos(8 deg) between two unit
// normals; dimensionless, no knowledge of the quantisation step.
static const float KH_DLSW_NRM_AGREE = 0.990f;

// The normal at a depth discontinuity: a 2x2 quad derivative straddles the
// cliff and returns a plane belonging to neither surface (refusing it
// over-darkens a dark outline; trusting it draws a bright one). The centre
// pixel belongs to exactly one surface; only the difference taken across the
// cliff is ambiguous.
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

    // Keep the side whose depth is nearer the centre's - the side that did not
    // cross the cliff. Signs are chosen so both differences still point along
    // +x and +y.
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
    // never contribute a surface the pass would have refused.
    if (!(khp_zpx > 0.05f) || khp_zpx > 8000.0f) return false;
    if (!(khp_znx > 0.05f) || khp_znx > 8000.0f) return false;
    if (!(khp_zpy > 0.05f) || khp_zpy > 8000.0f) return false;
    if (!(khp_zny > 0.05f) || khp_zny > 8000.0f) return false;
    // One surface, or none. A stencil straddling a silhouette fits a plane
    // across the gap; refused, not clamped - the caller falls back to the
    // narrowest measurement it has.
    const float khp_cx = abs(khp_zpx + khp_znx - 2.0f * khp_zc);
    const float khp_cy = abs(khp_zpy + khp_zny - 2.0f * khp_zc);
    if (max(khp_cx, khp_cy) > KH_DLSW_NRM_FLAT * khp_zc) return false;
    khp_dx = KhCastWorld(khp_px + float2(khp_r, 0.0f), khp_dims, khp_zpx)
           - KhCastWorld(khp_px - float2(khp_r, 0.0f), khp_dims, khp_znx);
    khp_dy = KhCastWorld(khp_px + float2(0.0f, khp_r), khp_dims, khp_zpy)
           - KhCastWorld(khp_px - float2(0.0f, khp_r), khp_dims, khp_zny);
    return true;
}
 
// KH_DLS_WORLD - the world-receive pass for dynamic-light shadows. Fullscreen,
// fired from the scene-resolve hook, emitting a MULTIPLY FACTOR under a
// dest*src blend; the lighting arithmetic lives in KhDlsWorldFactor, this
// shader turns a screen pixel into a world position and a normal. The depth is
// the engine's own resolved linear depth (g_mask.cast_depth) and the view is
// the frame's - the one place this pass deliberately does NOT copy PSMaskCast,
// which reconstructs through a FROZEN view because it paints at draw 0 where
// there is no depth for this frame yet. Pairing the engine's depth with the
// view it was drawn under removes the reprojection class. The normal is
// reconstructed from the depth field (the engine is forward and exposes no
// normal buffer); it is only ever used for an N.L term and KhDlsShadow's normal
// offset, which degrade to a slightly wrong shade, not a wrong verdict, and the
// factor is clamped to [0,1]. Every early exit returns 1.0, invisible under the
// multiply.
float4 PSDlsWorld(VSOut i) : SV_Target
{
    const float2 khw_dims = float2(castView[1].z, castView[1].w);
    if (khw_dims.x < 2.0f || khw_dims.y < 2.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    const float khw_zl = KhCastZl(i.pos.xy, khw_dims);
    if (!(khw_zl > 0.05f) || khw_zl > 8000.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    const float3 khw_w = KhCastWorld(i.pos.xy, khw_dims, khw_zl);

    // KhCastWorld builds v = (ndc.x*fovx, ndc.y*fovy, 1) * zl, so inside a
    // quantisation plateau zl is constant across the quad and the reconstructed
    // surface is the plane z = const in view space. Sampling the plane over
    // KH_DLSW_NRM_R pixels instead of one improves the signal-to-quantisation
    // ratio by that factor with no assumption about the step's size. On a
    // resolved surface the two baselines agree; on a staircase they do not.
 
    // The fullscreen pass stops paying for pixels no light can reach:
    // KhDlsFaceUV refuses every receiver whose face-axis depth is at or past
    // that light's far plane, and the face-axis depth is at least |p| /
    // sqrt(3).
    if (dlCtl.x >= 2.5f) {
        // The range fade, one step earlier.
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

    {
        // Two baselines: the taps are central differences, so each spans 2R
        // pixels of depth signal against one quantisation step. A failed wide
        // fit must not set khw_nrel to 0 outright: at a silhouette the facing
        // prior (reasoned for grazing ground under a lamp) draws a dark
        // outline.
        const float khw_r1 = KH_DLSW_NRM_R;
        const float khw_r2 = KH_DLSW_NRM_R * 2.0f;
        float3 khw_ax, khw_ay, khw_bx, khw_by;
        const bool khw_ok1 = KhDlswPlane(i.pos.xy, khw_dims, khw_r1, khw_zl, khw_ax, khw_ay);
        const bool khw_ok2 = KhDlswPlane(i.pos.xy, khw_dims, khw_r2, khw_zl, khw_bx, khw_by);

        if (khw_ok1 && khw_ok2) {
            // The wide pair shades: best signal-to-quantisation of the two.
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
            // Planar at R, broken by 2R: an edge sits between the two.
            khw_dx = khw_ax;
            khw_dy = khw_ay;
            khw_nrel = 1.0f;
        } else {
            // Broken inside R: a silhouette. The 2x2 quad derivative straddles
            // it and belongs to neither surface (the pixel-wide outline); take
            // the difference on the side that stays on the centre's own surface
            // instead. Only if that fails too does the quad derivative stand.
            float3 khw_ox, khw_oy;
            if (KhDlswOneSided(i.pos.xy, khw_dims, khw_zl, khw_w, khw_ox, khw_oy)) {
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

    float khw_cov = 0.0f;
    if (mirMeta.x >= 0.5f) {
        khw_cov = KhDlsMaskCov(i.pos.xy, mirMeta.y, mirMeta.z, khw_zl);
        if (khw_cov >= 0.999f) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    float3 khw_f = KhDlsWorldFactor(khw_w, khw_n, 0.0f, khw_nrel);
    khw_f = lerp(khw_f, float3(1.0f, 1.0f, 1.0f), khw_cov);
    return float4(khw_f, 1.0f);
}
 float4 PSMaskCast(VSOut i) : SV_Target
{
    float2 dimsM = float2(castView[1].z, castView[1].w);
    uint dw, dh;
    sceneDepthTex.GetDimensions(dw, dh);
    int2 px = int2(i.pos.xy * float2(dw, dh) / max(dimsM, float2(1.0f, 1.0f)));
    float4 zt = sceneDepthTex.Load(int3(px, 0));
    float zl = KhCastZPick(zt);   // Twin of KhCastZl's pick.

    bool khcNearOk = zl > 1.2f || castView[0].w >= 0.5f;

    if (!khcNearOk && zl > 0.05f) {
        // Routed through the one selector (max(.x, .y) was a third plane rule;
        // at mode 0 the .y plane is empty field-wide and the two coincide).
        float khcM0 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 2, dh >> 1), 0)));
        float khcM1 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 2), 0)));
        float khcM2 = KhCastZPick(sceneDepthTex.Load(int3(int2((dw * 3) >> 2, (dh * 3) >> 2), 0)));
        float khcM3 = KhCastZPick(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 4), 0)));
        float khcHi = max(max(khcM0, khcM1), max(khcM2, khcM3));
        float khcLo = min(min(khcM0, khcM1), min(khcM2, khcM3));
        khcNearOk = khcHi > 1.2f ||
                    (khcLo > 0.05f && khcHi > khcLo * 1.3f);   // Perspective structure.
    }

    float2 khrp_s = i.pos.xy;
    float3 pw = KhCastWorld(khrp_s, dimsM, zl);

    if (thmParams.w >= 0.5f) {
        float khtsH = KhThmHeight(pw.xz);
        if (khtsH > -1.0e5f && abs(pw.y - khtsH) < 0.35f) pw.y = khtsH;
    }

    float hit = 0.0f;
    if (sunMeta.x >= 0.5f) {
        bool near_ok = false;
        float stretch = 2.0f + 3.0f / max(abs(castView[2].y), 0.15f);

        if (localityMeta.y >= 0.5f) {
            int lc = (int)localityMeta.x;

            if (castMat[0].w > 0.0f) {
                // Constant time in the caster count. The bound comes from the
                // CB, not a literal, so it cannot drift from the C++ that sizes
                // the texture; zero (an unwritten lane) reads as 256.
                int khoN = (int)localityMeta.w;
                if (khoN <= 0) khoN = 256;
                int2 khoC = (int2)floor((pw.xz - float2(castMat[1].w, castMat[2].w)) * castMat[0].w);
                if (khoC.x >= 0 && khoC.y >= 0 && khoC.x < khoN && khoC.y < khoN) {
                    float2 khoY = khrCastOcc.Load(int3(khoC, 0));
                    if (pw.y >= khoY.x && pw.y <= khoY.y) near_ok = true;
                }
            } else {
                [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                    float3 lce = khrLocalityExt[li * 2].xyz;
                    float3 lhe = khrLocalityExt[li * 2 + 1].xyz;
                    float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                    if (KhCastReach(pw, lce, lhe, lr)) near_ok = true;
                }
            }
        } else if (localityMeta.x >= 0.5f && localityMeta.x <= 16.5f) {
            int lc = (int)localityMeta.x;

            [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                float3 lce = locality[li * 2].xyz;
                float3 lhe = locality[li * 2 + 1].xyz;
                float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                if (KhCastReach(pw, lce, lhe, lr)) near_ok = true;   // Twin of the reach drop.
            }
        } else {
            float castR = length(sizeAxes.xyz) * 0.5f;
            float reach = min(castR * stretch, max(600.0f, castR * 24.0f));
            // The combined-bounds fallback sweeps too.
            near_ok = KhCastReach(pw, centerSize.xyz, sizeAxes.xyz * 0.5f, reach);
        }

        // zl floor 1.2 m: if the captured depth texture transiently holds
        // aliased non-depth content (normalized values <= 1), every pixel
        // reconstructs to ~the camera position and MIN-darkens the whole screen
        // whenever the player stands near a caster.
        if (near_ok && khcNearOk) {
            hit = SunShadowOcclusion(pw);   // Near floor: whole-texture verdict above.
            // The fade is measured from the FROZEN fire camera (castMat *
            // -castView[0]), the same view pw came from, not sunOrigin (this
            // frame's camera): a frame of motion apart, the band moves under
            // the shadows every frame.
            float3 khfc = float3(dot(-castView[0].xyz, castMat[0].xyz),
                                 dot(-castView[0].xyz, castMat[1].xyz),
                                 dot(-castView[0].xyz, castMat[2].xyz));
            hit *= KhSunRangeFadeAt(pw, khfc);   // Range fade (at the helper).
        }

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

        // Locality clamp: a ground point can only be shadowed by a mesh within
        // plausible shadow reach of it (a few diagonals stretched by sun
        // elevation).
        float castR = length(sizeAxes.xyz) * 0.5f;
        float reach = castR * (2.0f + 3.0f / max(abs(castView[2].y), 0.15f));
        float3 toCast = centerSize.xyz - pw;
        float near_enough = (dot(toCast, toCast) < reach * reach) ? 1.0f : 0.0f;
        hit = (tf >= max(tn, 0.05f) && khcNearOk && near_enough > 0.5f) ? 1.0f : 0.0f;   // Near floor: whole-texture
                                                                                         // Verdict above.
    }

    // Full occlusion here; the apply colours the shadow by the ambient floor
    // (amb/(amb+sun)).
    float shade = 1.0f - hit * saturate(castView[2].w);
    return float4(shade, shade, shade, 1.0f);
}

// Fullscreen triangle from SV_VertexID: no vertex buffer, no input layout.
VSOut VSFullscreen(uint vid : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    o.wpos = float3(0.0f, 0.0f, 0.0f);
    o.nrm = float3(0.0f, 1.0f, 0.0f);
    o.wrel = float3(0.0f, 0.0f, 0.0f);   // (fullscreen path: no self sampling).
    o.icol = color;   // Every VSOut carries the colour lane (PSEffect reads the CB directly).
    KhObjLanesCb(o.iobj0, o.iobj1);   // KH_OBJBUF: every VSOut carries the object lanes too.
    return o;
}

float4 PSMain(VSOut i) : SV_Target
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
    // Twin: PSMain / PSComposite / PSEffect. The shared tail below is kept as
    // two copies on purpose: PSComposite interleaves KH_ARB_DEPTH blocks and a
    // khb_a lane through it.
    if (khObjFarVis < 0.5f && khObjCut > 0.0f && i.pos.w > khObjCut) discard;
    // Punch-through / overlay-occlusion guard, flush-path edition: the same
    // contract as PSComposite's. The CPU arms tight margins only for
    // single-sample snapshots, so MSAA frames stand down by construction.
    if (fxParams1.x < 1e8f) {
        int2 gpx = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        float sceneZ = KhSceneMeters(KhSceneLoad(gpx));
        if (i.pos.w > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;
    }

    // Analytic terrain lane (flush-path twin of PSComposite's, endpoint test
    // included).
    if (thmParams.w >= 0.5f) {
        float khtC = 2.0e9f;
        float khtHe = KhThmHeight(i.wpos.xz);
        if (khtHe > -1.0e5f) khtC = i.wpos.y - khtHe;

        if (i.pos.w >= thmMeta.w) {
            // 1.5-cell aliasing slack (twin).
            float khtMc = KhThmClearance(fxParams0.xyz, i.wpos) + 1.5f * thmParams.z;
            if (khtMc < khtC) khtC = khtMc;
        }

        if (khtC < 1.5e9f && khtC < -thmMeta.z) discard;
    }

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
    // Twin edit (PSComposite's ARB variant declares SV_Depth - the one place
    // the twins differ). The 2 px deadband keeps the historic raster tap
    // bit-exactly at steady motion.
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
        // hoisting applies as it does for the N.L branch.
        if (smf > 0.0f) smf = min(smf, SunShadowFactorSelf(i.wpos, i.wrel, khBiasN));
        if (maskMeta.w >= 0.5f) {
            float khStenU = KhStenUnit(i.pos.xy);
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

    if (blendCtl.x >= 0.5f) {
        // Background trust: PSComposite's rule stands.
        float3 scn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float3 ts = scn / (1.0f + scn);
        float3 tl = lc / (1.0f + lc);
        float3 tm = lerp(ts, tl, a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // Cap ~HDR 255.
    }

    return float4(lc, a);
}
