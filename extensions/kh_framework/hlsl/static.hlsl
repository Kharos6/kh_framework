// Static.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.

// KH_INSTANCING: both entry points are wrappers over KhVsCore (the shared
// prefix) - the per-object one hands in the CB lanes, the instanced one the
// stream's. Twin: VSComposite / VSCompositeInst carry the identical pair of
// wrappers over the same core.
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

// KH_OBJBUF: the bucket wrapper - the record at the lane's slot supplies what
// the CB supplies above; the rebase-relative centre is the record centre
// minus the pass camera (khPass), a correctly rounded float difference, kept
// precise so it is never re-associated into the absolute path.
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

// KH_SEAM_INST: the stencil-volume seam transport's instance stream. The seam
// submitted one DrawIndexed per caster in each of its two loops - 6,002 draws
// and 6,002 constant-buffer uploads a frame at 3,001 meshes - where the
// structurally identical sun-depth ladder does the same work in one instanced
// draw per mesh. The lane is the sun ladder's own (KhInstLane / VSInSun): the
// record slot on TEXCOORD4, dither/alpha/matBase on TEXCOORD5, so
// layout_sundepth binds both entry points below unchanged. NORMAL is declared
// to keep that layout's slot-0 shape and is unread by the mirror twin.
struct VSInSeam {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;
};

// KH_SEAM_INST: loop A's twin - the footprint into the engine's volume buffer.
// The per-object entry for this pass is VSMain, so this calls the same
// KhVsCore and takes the same stenVol2.z = 3 remap; only the lane source
// differs (record + khPass instead of the CB's centreSize/sizeAxes/objRot and
// centerRel). The rebase arm is khPass.w, NOT centerRel.w - the CB's per-object
// lane is never filled on an instanced draw, so the C++ side must fill khPass
// for this pass or the absolute branch is taken against a rebased viewProj.
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

// KH_OBJBUF: the sun-depth stream is the same lane the colour buckets use;
// the record supplies centre, size and rotation.
struct VSInSun {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    uint   islot : TEXCOORD4;
    float3 ilane : TEXCOORD5;   // y = the caster's alpha (envelope applied, KH_CAST_ALPHA).
};

struct VSInMir { float3 pos : POSITION; };
// KH_SEAM_INST: the mirror prepass's clip transform, factored so the
// per-object entry and its instanced twin cannot drift. Takes the position in
// the visible box's own space, then rebuilds the projection with the near
// plane at 0.05 from the live b2 low pair - deliberately NOT KhVsCore's
// stenVol2.z = 3 remap, which recovers the engine's own z instead. The two
// seam loops rasterise into different spaces and this is the difference.
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

// KH_SEAM_INST: loop B's twin. KhRotate's rotated branch over the record's own
// rows - kh_objrec_fill always writes rot0.w = 1 and identity rows for an
// unrotated object, so the CB entry's objRot0.w gate has no instanced
// equivalent to take.
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

// KH_CAST_DISSOLVE - the engine's rule for far casters, in the map. An object
// past the shadow distance does not cast; instead of dropping it whole, each
// CASTER FRAGMENT is dithered out by its own distance from the camera
// (sunOrigin, the anchor of every sun map) over the same band as the range
// fade (KH_SUN_FADE_START .. 0.995 of mirMeta.w). Per fragment, so the far
// corner of a mesh dissolves before its near one, and the receiver's soft
// filter turns the dither into a smooth fade. A nearer occluder still writes
// its depth whatever a farther one does, so unlike a depth-gap fade this
// cannot lighten a doubly-shadowed spot. Both consumers of the maps (self
// kernels and the world cast) read the same texels, so both agree.
// mirMeta.w = 0 (a caster-anchored map, no camera) leaves every fragment.
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

// KH_DISSOLVE_DITHER: 4x4 Bayer, 16 levels - the SAME period as the alpha
// twin's, on purpose. The receiver's soft filter (five bilinear taps) spans a
// ~4x4 texel footprint, so a period-4 pattern is averaged whole by every
// footprint and the map reads as a uniform coverage; an 8x8 pattern was
// averaged in part, which speckled, and at far-tier texel sizes the speckle
// shimmered with the camera. The map windows move in whole texels, so the
// pattern is fixed in the world and does not crawl.
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
// private sun-depth maps (the self kernels and the world cast through
// PSMaskCast's SunShadowOcclusion) read the same texels, so alpha enters once,
// here, and both agree by construction.
struct VSInSunA {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    uint   islot : TEXCOORD4;   // KH_OBJBUF (the plain twin's lane).
    float3 ilane : TEXCOORD5;   // y = the caster's colour alpha (envelope applied).
};
struct VSOutSunA { float4 pos : SV_Position; float2 uv : TEXCOORD0; float alpha : TEXCOORD1;
                   float3 wp : TEXCOORD2; };   // wp: KH_CAST_DISSOLVE.

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
// covers exactly the requested fraction to a sixteenth, and a 3x3 kernel reads
// it to within a ninth. Anchored to the map's texel grid.
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
    if (khsa_mode == 1) clip(khsa_t - matParams0.z);   // Cutout: the cutoff kills, survivors cast
                                                       // Full.
    else if (khsa_mode == 2 && KhMatRouteTexel(matParams3.y, 1.0f, i.uv) < 0.9f) khsa_a *= khsa_t;
    // KH_CAST_DISSOLVE in the alpha twin: the distance band multiplies INTO
    // the coverage and one dither decides, so a translucent caster dissolves
    // with distance exactly as an opaque one does and the two dithers never
    // interfere (two clips on the same pattern would compose as a min, not a
    // product). Inside the band the original path is bit-identical.
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

// KH_FOOTPRINT_ALPHA - the seam footprint carries depth-writing texels only. A
// translucent texel writes no depth in the colour pass, so the engine's count
// at its pixel must be measured at what is behind it (the ground the player
// sees through the glass) - a whole footprint made the engine count at the
// glass and the ground behind inherited the glass's shadow (the shadow that
// 'followed the surface'). Whole translucent objects never reach it (skipped at
// the list).
void PSInjDepthA(VSOut i)
{
    KhMatLoad(i.matIx);   // KH_MAT_TABLE.
    int khfa_mode = (int)matParams0.y;
    float khfa_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khfa_mode == 1) clip(khfa_t - matParams0.z);
    else if (khfa_mode == 2) clip(KhMatRouteTexel(matParams3.y, 1.0f, i.uv) - 0.9f);   // KH_MAT_SPLIT_TOL //
                                                                                       // KH_MAT_SPLIT_TEXEL: the colour
                                                                                       // pass's own verdict.
}

// KH_DLSW_MASK_ALPHA - the world pass's self-mask carries depth-writing texels
// only: KH_FOOTPRINT_ALPHA's rule, applied to the other consumer that drew our
// meshes alpha-blind. A cutout hole or a blend material's translucent texel
// writes no depth in the colour pass, so the surface at its pixel is whatever
// is behind it - the ground seen through a fence, or through glass - and that
// surface must receive the dynamic-light shadow exactly like the ground beside
// it. A whole-silhouette mask marked the pixel as ours, KhDlsMaskCov exempted
// it, and the world behind the texel drew unshadowed while the texel itself
// carried the in-shader shadow: the lit scenery 'seen through' a shadowed
// surface, most visible where the texel is darkest. The verdict is the colour
// pass's own (PSInjDepthA's lines, per texel); survivors write the view
// distance PSDlsMask writes (SV_Position.w is the clip w, the same quantity
// VSDlsMask stores - PSComposite reads it as metres the same way).
float4 PSDlsMaskA(VSOut i) : SV_Target
{
    KhMatLoad(i.matIx);   // KH_MAT_TABLE.
    int khma_mode = (int)matParams0.y;
    float khma_t = KhMatRoute(matParams3.y, 1.0f, i.uv);
    if (khma_mode == 1) clip(khma_t - matParams0.z);
    else if (khma_mode == 2) clip(KhMatRouteTexel(matParams3.y, 1.0f, i.uv) - 0.9f);   // KH_MAT_SPLIT_TOL.
    return float4(i.pos.w, 0.0f, 0.0f, 0.0f);
}
#endif

// Analytic mask cast: per-pixel ray-vs-AABB toward the sun, drawn into the
// engine's screen-space shadow mask with multiply blending. castMat[0.2] = view
// rows 0.2; castView[0] = view row 3 (translation); castView[1] = (fov.x,
// fov.y, maskW, maskH); castView[2] = sunDir.xyz + strength in w. Scene linear
// depth at t0.
Texture2D<float4> sceneDepthTex : register(t0);
float KhSceneLoad(int2 p) { return sceneDepthTex.Load(int3(p, 0)).x; }

// Pre-mesh scene color capture (t3, single-sample) - the perceptual- composite
// blend source for the flush mesh pass; see PSMain's packing.
Texture2D<float4> sceneColorTex : register(t3);
// Bound only at the mask cast fire; compiled out of every entry that never
// reads it. Extended per-caster locality list (localityMeta.y arms): 2 float4
// per caster, [2i] center / [2i+1] half extents, engine axes - the uncapped
// twin of the 16-pair CB list below.
StructuredBuffer<float4> khrLocalityExt : register(t2);
// KH_CAST_OCC: the caster-occupancy grid. Strict superset of the loop, so no
// caster's shadow can be lost.
Texture2D<float2> khrCastOcc : register(t35);

// The reach is A shadow, not A sphere. That is only right for a caster standing
// on the receiver. Now: sweep the caster down-sun onto the receiver's own
// height plane and test the segment. T0/t1 are where the caster's base and top
// Land at pw.y, so the tested segment is the shadow. lr survives as pure
// Lateral slack. sunShadowOcclusion answers 0 where no occluder stands, so the
// gate can only cost alu, never a wrong verdict.
 
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
    // Superset contract. The sweep is the geometrically right test but it is
    // not a superset of the sphere on its own: a receiver directly under a
    // floating caster has t clamped to t0, so the residual is the whole throw,
    // while the sphere admitted it whenever the drop was under lr. The sweep is
    // right to refuse it - no shadow stands there at a low sun - but this gate
    // may only ever add reach, never remove it, or a fix for the cut becomes a
    // new cut somewhere nobody is looking. A false positive costs one
    // sunShadowOcclusion that answers 0, so the union is free.
    float3 khcr_ld = max(abs(khcr_p - khcr_c) - khcr_h, 0.0f);
    return dot(khcr_ld, khcr_ld) < khcr_lr * khcr_lr;
}

// KH_CAST_REPROJ, they sit above their caller deliberately. The two-plane depth
// carries a far plane in .x and a near/witness plane in .y, and the live rule
// takes .y whenever it is non-zero and silently falls back to .x otherwise -
// per pixel, per region, with nothing counting it. The note at the top of
// PSMaskCast records that .x carries a constant z bias ("drift growing as
// fragments near, fine at range - relative error c/z"), which is why .y was
// Preferred in the first place; the fallback was never removed. Twin count
// (rule 1.5), re-measured.
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
// World -> screen pixel through this frame's view. The inverse direction of the
// above, against a different matrix - which is the entire point.
float4 PSMaskPrime(VSOut i) : SV_Target
{
    const float khpm_v = KH_PRIME_V;
    return float4(khpm_v, khpm_v, khpm_v, 1.0f);
}
 
// VSMirror does not write the clip depth of the matrix it is handed, it
// overwrites z with its own near plane (l22 = f/(f-0.05); z = l22*w - 0.05*l22,
// f out of the engine's b2 block) and does so conditionally, on four tests
// against engBlk. Its plane is another feature's private convention, it changes
// shape frame to frame, and nothing obliges it to keep doing either. The mask
// is a single R32_FLOAT target holding the linear view distance, in metres, of
// the nearest of our meshes at every pixel - the same quantity KhCastZl already
// returns for the world, so the comparison is metres against metres with no
// convention to get wrong, no near/far to recover, and no depth partition to
// track.
struct VSOutDM { float4 pos : SV_Position; float dist : TEXCOORD0; };

VSOutDM VSDlsMask(VSIn i)
{
    // The same object transform every mesh draw in this file uses - including
    // the fp32 rebase, which this shader did not take until now. The zeroed
    // default (centerRel.w = 0) keeps the absolute path, so any caller that
    // does not rebase is unaffected - the same contract KhVsCore honours.
    const float3 khdm_l = KhRotate(i.pos * sizeAxes.xyz);
    float3 khdm_w = (centerRel.w > 0.5f) ? (centerRel.xyz + khdm_l)
                                         : (centerSize.xyz + khdm_l);
    float4 khdm_c = mul(float4(khdm_w, 1.0f), viewProj);
    VSOutDM o;
    o.pos = khdm_c;
    // w of a perspective clip position is the view-space distance along the
    // forward axis - the same quantity KhCastZl returns for the world surface.
    o.dist = khdm_c.w;
    return o;
}

float4 PSDlsMask(VSOutDM i) : SV_Target
{
    return float4(i.dist, 0.0f, 0.0f, 0.0f);
}

// The mask, read back by the world pass. The sentinel means no mesh covers it.
// KH_DLSW_MASK_ALPHA: it carries our depth-writing texels only - an alpha
// caster is drawn through PSDlsMaskA with its material bound, an invisible
// (casterOnly) or whole-translucent object is not drawn at all - so 'nearer
// than the world' means 'our surface is what the eye sees here' and nothing
// else.
Texture2D<float> khDlsMask : register(t37);
// khdw_zl is the world surface's distance at the same pixel. Both are metres.
// Every way that race can go wrong has the same signature, and it is the
// reported artifact word for word, so the region it fails over has the shape of
// that geometry, it parallaxes with that geometry rather than with our mesh,
// and what gets painted there is the world's own shadow factor - which at
// night, fully blocked, is black. What the failed trade did buy is worth
// keeping: the depth race is not the carrier.
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
// any of the four taps is not a surface. R is in pixels and that is the point.
// Widening the stencil buys signal-to-quantisation directly, with no constant
// to guess. That is why the taps are refused rather than clamped when one lands
// on sky or off the depth (rule 1.91) and why the caller keeps the 2x2 quad
// derivative as the fallback: near a silhouette the narrow answer is the honest
// one, and the caller is told so through khw_nrel.
static const float KH_DLSW_NRM_R = 3.0f;        // Base baseline, pixels.
// KH_DLSW_NRM_FLAT: the second-difference bound that decides whether the
// stencil is looking at one surface. For any plane, zl(+r) + zl(-r) - 2*zl is
// exactly zero whatever the plane's slope, so this test is blind to orientation
// and sensitive only to the neighbourhood breaking - which is the question. 2%
// of range is 0.6 m of curvature across the stencil at 30 m, which no
// continuous surface produces and every silhouette does.
static const float KH_DLSW_NRM_FLAT = 0.02f;
// The agreement floor for the two baselines. Dimensionless - it compares two
// unit normals, so it is not a bar standing in for a length (rule 1.84), and it
// needs no knowledge of the quantisation step. Cos(8 deg): two baselines that
// disagree by more than that are not describing one plane.
static const float KH_DLSW_NRM_AGREE = 0.990f;

// KH_DLSW_ONESIDED - the normal at A depth discontinuity. A 2x2 quad derivative
// there straddles the cliff and returns a plane belonging to neither surface;.
// 26876a answered that by refusing and applying the facing prior, which
// over-darkened and drew a dark outline;. 26876b answered it by trusting the
// straddling derivative, which draws a bright one. The centre pixel is not
// ambiguous - it belongs to exactly one surface. Only the difference is, and
// only because it was taken across the cliff. 520 only ever disarmed the normal
// inside KhDlsShadow, never inside the factor.
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
    // One surface, or none. A stencil straddling a silhouette fits a plane
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
// khDlsWorldFactor beside DynLights; this shader's only job is to turn a
// screen pixel into a world position and a normal.
//
// THE DEPTH IS THE ENGINE'S OWN RESOLVED LINEAR DEPTH (g_mask.cast_depth,
// adopted by identity at the resolve sweep - no snapshot of ours), and the
// view is the frame's; that is the one place this pass deliberately does NOT
// copy the sun's world cast. PSMaskCast reconstructs through a FROZEN view
// because it paints at draw 0, where - as the note at castViewN says outright
// - there IS no depth for this frame yet, so it must pair depth(N-1) with
// view(N-1) and then reproject. This pass fires at the resolve, two thirds of
// the way through the frame and after the world is drawn. Pairing the engine's
// depth with the view it was drawn under removes the whole reprojection class
// rather than compensating for it. Everything else - the caster set,
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
//
// Every early exit below returns 1.0, which under the multiply blend is
// invisible - a shader that bails on its first line and one that never ran
// look identical on screen. The per-exit flat-colour paint that once
// disambiguated them (a retired mode) is gone with the catalog; the pass
// is proven landing and this shape is settled.
float4 PSDlsWorld(VSOut i) : SV_Target
{
    const float2 khw_dims = float2(castView[1].z, castView[1].w);
    if (khw_dims.x < 2.0f || khw_dims.y < 2.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    const float khw_zl = KhCastZl(i.pos.xy, khw_dims);
    if (!(khw_zl > 0.05f) || khw_zl > 8000.0f) return float4(1.0f, 1.0f, 1.0f, 1.0f);
    const float3 khw_w = KhCastWorld(i.pos.xy, khw_dims, khw_zl);

    // It does not reach KhDlsWorldFactor, which reads this same normal in three
    // more places 520 never touched: khw_ndl on every light's diffuse, the same
    // term inside khw_dyn, and the sun's N.L in the denominator. It says
    // nothing at all about the normal, and the ledger's hypothesis 5 must be
    // Struck. khCastWorld builds v = (ndc.x*fovx, ndc.y*fovy, 1) * zl, so
    // inside a plateau zl is constant across the quad and the reconstructed
    // surface is the plane z = const in view space. So sample the plane over
    // KH_DLSW_NRM_R pixels instead of one and the signal-to-quantisation ratio
    // improves by that factor directly, with no assumption about the step's
    // size - which matters, because we have never measured it and rule 1.84
    // says not to stand a bar where a principle belongs. On a genuinely
    // resolved surface the two agree closely whatever the surface is doing; on
    // a staircase they do not, because each baseline lands on a different
    // tread.
 
    // KH_DLSW_REACHCUT - the fullscreen pass stops paying for pixels no light
    // can reach. Everything below this line - the two plane fits and the
    // one-sided fallback (8-12 depth Loads), the 3x3 coverage read (9 more) and
    // the per-light kernel - only matters where some casting light's map can
    // Answer. khDlsFaceUV refuses every receiver whose face-axis depth is at or
    // past that light's far plane, and the face-axis depth is the largest
    // absolute component of the light-relative position, which is at least |p|
    // / sqrt(3).
    if (dlCtl.x >= 2.5f) {
        // KH_DLS_RANGE: the same identity one step earlier.
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
        // Two baselines. The taps are central differences, so each spans 2R
        // pixels of depth signal against one quantisation step.
        // KH_DLSW_NRM_FALLBACK - what "unresolved" is allowed to mean. 26876a
        // set khw_nrel to 0 on every failure of the wide fit, and the field
        // found the cost immediately: a thin dark outline wherever shadowed
        // world geometry meets the sky. The prior was reasoned for grazing
        // ground under a lamp, and a silhouette is not that. The error was
        // letting one number mean two different things.
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
            // Broken inside R: a silhouette. The 2x2 quad derivative taken
            // above straddles it and belongs to neither surface - that is the
            // pixel-wide outline, dark under 26876a's prior and bright under
            // 26876b's trust. Take the difference on the side that stays on the
            // centre's own surface instead, and it is a real measurement again.
            // Only if even that fails (an isolated pixel with no valid
            // neighbour on some axis) does the quad derivative stand.
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
    // The field then produced the .x signature of a constant z bias (drift
    // growing as fragments near, fine at range - relative error c/z), the mark
    // of a near-offset encoding, while the engine's own resolve reads .y.
    float2 dimsM = float2(castView[1].z, castView[1].w);
    uint dw, dh;
    sceneDepthTex.GetDimensions(dw, dh);
    int2 px = int2(i.pos.xy * float2(dw, dh) / max(dimsM, float2(1.0f, 1.0f)));
    float4 zt = sceneDepthTex.Load(int3(px, 0));
    float zl = KhCastZPick(zt);   // KH_CAST_ZPLANE (twin of KhCastZl's pick).

    bool khcNearOk = zl > 1.2f || castView[0].w >= 0.5f;

    if (!khcNearOk && zl > 0.05f) {
        // KH_CAST_ZPLANE twin. These four taps used max(.x,.y) - a third plane
        // rule, distinct from both KhCastZPick modes - while the note above
        // khCastZPick claims one selector serves every read site. The claim was
        // false and the four taps also Loaded each texel twice. Routed through
        // the selector: at mode 0 the .y plane is empty field-wide, so
        // khCastZPick returns .x and max(.x, 0) returned .x - bit-identical.
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
                // KH_CAST_OCC: constant time in the caster count.
                // KH_OCC_N_LANE: the bound comes from the CB, not a literal. It
                // was 256 here and KH_OCC_N in the C++ that sizes the texture,
                // with nothing tying the two together - change one and the
                // shader reads outside the grid or ignores part of it, silently.
                // Zero means an unwritten lane, which reads as the old literal.
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
                if (KhCastReach(pw, lce, lhe, lr)) near_ok = true;   // KH_CAST_REACH_DROP (twin).
            }
        } else {
            float castR = length(sizeAxes.xyz) * 0.5f;
            float reach = min(castR * stretch, max(600.0f, castR * 24.0f));
            // KH_CAST_REACH_DROP (twin): the combined-bounds fallback sweeps
            // too.
            near_ok = KhCastReach(pw, centerSize.xyz, sizeAxes.xyz * 0.5f, reach);
        }

        // zl floor 1.2 m: if the captured depth texture transiently holds
        // aliased non-depth content (normalized values <= 1), every pixel
        // reconstructs to ~the camera position - inside reach whenever the
        // player stands near a caster, MIN-darkening the whole screen (the
        // era-independent overcast mode).
        if (near_ok && khcNearOk) {
            hit = SunShadowOcclusion(pw);   // Near floor: whole-texture verdict above.
            // KH_SUN_FADE_WIDE: the fade is measured from the FROZEN fire
            // camera (castMat * -castView[0]), the same view pw came from,
            // not sunOrigin (this frame's camera). With the two a frame of
            // motion apart the band moved under the shadows every frame.
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

        // Reach = a few diagonals stretched by sun elevation (low sun = long
        // shadows); beyond it, geometrically impossible - never darken.
        // Locality clamp: a ground point can only be shadowed by a mesh within
        // plausible shadow reach of it.
        float castR = length(sizeAxes.xyz) * 0.5f;
        float reach = castR * (2.0f + 3.0f / max(abs(castView[2].y), 0.15f));
        float3 toCast = centerSize.xyz - pw;
        float near_enough = (dot(toCast, toCast) < reach * reach) ? 1.0f : 0.0f;
        hit = (tf >= max(tn, 0.05f) && khcNearOk && near_enough > 0.5f) ? 1.0f : 0.0f;   // Near floor: whole-texture
                                                                                         // Verdict above.
    }

    // Full occlusion here; the apply colors the shadow by the ambient floor
    // (amb/(amb+sun)) - depth and hue both derive from the sky.
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
    o.icol = color;   // KH_INSTANCING: every VSOut carries the colour lane (PSEffect reads the CB
                      // Directly).
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
    // Twin: PSMain / PSComposite / PSEffect. The shared tail below is
    // kept as two copies on purpose: PSComposite interleaves KH_ARB_DEPTH blocks
    // and a khb_a lane through it, so a KhShadeTail() helper fails the site-text
    // proof (rule 1.133). Decided 26892; revisit only with a shading change.
    if (khObjFarVis < 0.5f && khObjCut > 0.0f && i.pos.w > khObjCut) discard;
    // Punch-through / overlay-occlusion guard, flush-path edition: the same
    // contract as PSComposite's. The CPU arms tight margins only for
    // single-sample snapshots, matching this texture's declaration, so MSAA
    // frames stand down by construction.
    if (fxParams1.x < 1e8f) {
        int2 gpx = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        float sceneZ = KhSceneMeters(KhSceneLoad(gpx));
        if (i.pos.w > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;
    }

    // Analytic terrain lane (flush-path twin; see PSComposite's note, endpoint
    // test included - the half-buried correction).
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

    int bm = (int)sizeAxes.w;
#if KH_TEXTURED
    // KH_TEXTURED: sample below the far contract + guard blocks (the section-4
    // checkpoint - the textured twin adds no return/discard above them),
    // cutout-clip, then build the mapped shading normal. The geometric normal
    // keeps owning the receive gating below - shadow behavior stays in parity
    // with the untextured twin.
    KhMatLoad(i.matIx);   // KH_MAT_TABLE: the lanes below read from the entry.
    KhMatSurf khtxS = KhSampleMat(i.uv);
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
    // Twin edit: (PSComposite's ARB variant declares SV_Depth and carries the
    // khaODepth note - the one place the twins legitimately differ.) At steady
    // motion the two agree to sub-pixel and the 2 px deadband keeps the
    // historic raster tap bit-exactly - so the halo regime that killed (a
    // standing ~30 px displacement on every moving frame) cannot form.
    // PSComposite declares khStenP in both branches of that conditional; this
    // block sat only in the #else, so the ARB variant never declared khBiasN
    // while the call site below the #endif used it.
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
        // KH_SELF_SKIP_DARK: a pixel the received term already darkens to 0
        // cannot get darker - min(0, x) = 0 - so the self ladder (the four
        // tiers, their rings and pyramids: the costliest term in this shader)
        // is not consulted for it. Exact: the only value it could contribute
        // is discarded by the min. A plain if, as the N.L branch around it is,
        // so the gradient hoisting fxc does for that branch applies here too.
        if (smf > 0.0f) smf = min(smf, SunShadowFactorSelf(i.wpos, i.wrel, khBiasN));
        if (maskMeta.w >= 0.5f) {
            float khStenU = KhStenUnit(i.pos.xy);
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
            float khStRf = KhSunRangeFade(i.wpos);
            smf *= 1.0f - (1.0f - khStenU) * khStRf;
        }
    }

    // Slices surviving this are painted by the engine over our pixels, not by
    // any term of ours. Twin edit: PSMain and PSComposite identical. 0, wiped,
    // code burned. A residual copy of the arm survived in this twin (never in
    // PSComposite; no ladder emitted 81, so it was dead) until the twin-parity
    // check removed it. Twin edit: PSMain and PSComposite identical from here
    // to the shading call.
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
        if (fogEngine.w >= 0.5f && fogEngine.w < 1.5f && khObjFarVis < 0.5f)
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
        // Background trust: PSComposite's rule stands untouched - mid-frame is
        // still mid-frame there.
        float3 scn = sceneColorTex.Load(int3(int2(i.pos.xy), 0)).rgb;
        float3 ts = scn / (1.0f + scn);
        float3 tl = lc / (1.0f + lc);
        float3 tm = lerp(ts, tl, a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // Cap ~HDR 255.
    }

    return float4(lc, a);
}
