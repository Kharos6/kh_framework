// ssao.hlsl - KH_SSAO, the screen-space ambient occlusion of OUR meshes. A
// standalone unit (no cb.hlsl prefix; its own b0). Any edit changes the
// unit's shader cache key. C++ twins: KhSsaoCb, kh_ssao_pre, kh_ssao_post, KH_SSAO_MARK.
//
// Nine draws after a drawer's depth-writing draws (the world injection's on
// the render thread, or the flush's late ones under the park), plus - whether
// or not the term runs - the near-plane marker's draw and the C++ side's
// eraser draw. The term is computed on a HALF-RESOLUTION grid,
// as the SSGI's gather is: the cost is the meshes' screen coverage, and
// the term is a smooth field the full-resolution apply upsamples
// depth-guided (effect 24's recipe).
//   PSSsaoDepth - sample 0 of the live main depth at every other pixel,
//                 converted to metres (KhSaMeters), into an R32 half-res
//                 texture every later tap reads (one multisampled load and
//                 one conversion per half pixel instead of one per tap).
//   PSSsaoMain  - half res: the occlusion term into an R8 target, 1 wherever
//                 the pixel is not ours.
//   PSSsaoBlur  - six times, half res, ping-ponging two R8 targets: three
//                 levels of the depth-weighted 5-tap blur, each an x pass then
//                 a y pass (KH_SSAO_SEP: separable - the 5 x 5 kernel's
//                 Gaussian is a product, its depth weight taken per axis), at
//                 strides growing level to level, at our pixels alone (the rest
//                 pass through).
//   PSSsaoApply - full res: the term through a joint-bilateral upsample of the
//                 half grid (the four texels around the pixel, depth-weighted),
//                 MULTIPLIED into the scene colour
//                 (dest.rgb *= src.rgb) at our samples alone: the C++ side
//                 binds the read-only depth view with a stencil test on the
//                 mark, so unmarked pixels never run it.
//   PSSsaoNearMark - full res, while the mark stands, for a pass that routes
//                 near-z alone: (raw, true distance) at this drawer's gap
//                 pixels into the near-plane marker the late effect chain
//                 reads (KH_NEARZ_MARK; see the entry point).
//   (eraser)    - no shader: a stencil-only draw sets the mark back to 0.
//
// "Ours" is the one test every pass takes, and it is the STENCIL's answer
// (KH_SSAO_MARK): every depth-writing draw of ours writes stencil 1 where
// it wins the depth test, the engine's world draws never write the main
// depth's stencil, and the eraser clears the mark after the term - so a
// pixel with the mark was painted by THIS drawer since its last pass,
// whichever drawer that is and however the frame's races fell. An engine
// pixel is left exactly as it was: it neither receives our term nor is
// compared against anything but the depth it already sits in. It does
// occlude ours - the samples read the live depth whoever wrote it - so a
// mesh resting on the ground darkens at the contact, and an engine wall
// darkens the mesh leaning on it. The engine's own AO stays the engine's.
//
// Depth is metres along the view axis from the injection's own pair
// (KhSceneMeters's recipe) and the viewport range the pass drew in - or, for
// a near-z routed fragment of ours nearer than the pass's near plane, from
// the route's own depth ramp (KH_SSAO_NEARZ, KhSaMetersAt); the view
// position is rebuilt from the pixel and that depth (KhgVpos's recipe). The
// normal is taken twice: over a 1 px baseline from the nearer neighbour on
// each screen axis (silhouette-safe: the cliff's slope is never the pixel's),
// and over a WIDE baseline that grows with the radius on screen, which
// averages across a mesh's facets the way its smooth shading normal does.
// The wide one is the reference plane wherever its four taps stay on the
// receiver's surface (depth-continuous within the radius) and it agrees with
// the 1 px one (KH_SSAO_NWIDE: within about 20 degrees, fading out by 35 - a
// tap past a convex corner is depth-continuous with the pixel, and the plane
// through it bent round the corner and greyed the whole face); the 1 px one
// answers at silhouettes and creases. With a per-facet normal the tangent
// plane jumped at every edge and each facet took its own tone; a smooth
// normal makes the term continuous across them.
//
// Every pass draws over the meshes' rectangle padded by the passes' reach
// (KH_SSAO_RECT_PAD). The gather and blur taps read no term or half-grid
// depth texel outside it: nothing wrote there this frame. The wide normal's
// four depth taps (KhSaNormal at a stride of up to 24 half pixels) take no
// such test; they stay inside because a marked pixel lies inside the
// UNPADDED rectangle and the pad exceeds that stride - the C++ side's
// contract at KH_SSAO_RECT_PAD.
//
// KH_SSAO_VB - the gather is a visibility bitmask (Therrien, Levesque and Gilet 2023; the method of Pascal Gilcher's
// MXAO, and of the SSGI's gather, effect3.hlsl PSSsgiGather). The receiver's hemisphere is cut into KH_SA_NS slices
// through its view ray and each slice into 32 sectors; along each slice's line the samples march outward on both
// sides. A sample stands for a solid KH_SA_THICK x the radius deep behind its visible face (along its own view ray)
// and occupies the sectors that solid spans above the receiver's tangent plane (KhSaEdge clips it there): an object
// in front of the receiver occludes what it covers, where the former height-above-the-plane sum took it as solid all
// the way down - the dark halo a post left on the surface behind it. Two neighbouring samples within that depth of
// each other are one surface, and so are the receiver and the first sample: the quad between them occupies its
// sectors too (the ground between a wall's foot and the first sample on it is not left open). A sample counts only
// the sectors no nearer sample held; a quad's take the mean of its two ends' falloff weights. The sectors split a
// slice by GTAO's exact measure, cos(angle to the normal) x |sin(angle to the view ray)| (KhSaG), so a sector's share
// is its share of the cosine-weighted hemisphere (without the |sin|, as the SSGI's gather runs, the occlusion mass
// the estimate missed against a ray-traced reference grew 2.4 x here, and its level fell by about a third).
//
// The samples keep the former gather's discipline. The slices' rotation and the samples' jitter come from the
// pixel's POSITION ALONE (no frame term: a static frame is bit-identical, every shimmer is real motion), by a 4 x 4
// ORDERED tile of sixteen rotations: the first blur level's 5 x 5 support (its x pass then its y pass) covers a whole
// tile - its stride is odd (KH_SSAO_TILE, KhSaBlur), so its five taps fall on all four of the tile's columns, then
// rows - so after it every pixel holds the average over all sixteen and the field no longer depends on where the tile
// lands. A skinned mesh is never still, and under a screen-anchored random pattern every frame re-rolled its taps.
// The receiver is lifted off its own surface by a floor (a share of the radius plus a little distance), heights taken
// over its own tangent plane, so its reconstruction noise holds no sector; the term fades where it cannot be trusted:
// at grazing incidence and where the radius spans only a few pixels. What one blur level could not hide - the
// pattern is anchored to the screen, so a moving mesh slides under it - the three blur levels average out.
//
// The radius is world-space, so far away it is a few pixels and the term
// with it. As the distance grows the radius is floored at KH_SA_RANGE_FRAC of
// it (a constant footprint on screen from there on, at a given field of view:
// the floor is in metres, so through a zoomed optic the same mesh's term
// spans more of its surroundings - 15 m at 300 m, held to 256 half pixels),
// which is what keeps a crease or a contact readable across a street: the
// larger-scale occlusion a far mesh shows is as real as the close one's, only
// coarser.
//
// What this does not do, by design: a translucent or depth-Off draw of ours
// writes no depth and so is never "ours" here (no term on it, as before), and
// the term lands before either drawer's translucent tail blends over it; the
// PIP passes and the view-model slice draw no term; on an MSAA scene the
// shaders' test is sample 0's: a pixel whose sample 0 is not ours takes no
// term on any sample, and one whose sample 0 is ours takes it on its marked
// samples (the apply's stencil test) - on every sample where the read-only
// depth view or the test state is missing (a sub-pixel dark rim at most).

cbuffer CBSsao : register(b0)
{
    float4 khsaProj;   // x = m00, y = m11, z = m22, w = m32 (the drawer's projection, row-vector).
    float4 khsaVp;     // x = viewport MinDepth, y = MaxDepth, z = target width, w = target height (px, full res).
    float4 khsaCtl;    // x = radius (m), y = strength (the term's exponent), z = the blur stride
                       // multiplier of this draw (1, 2, 3 by level); w = the blur's axis and rule
                       // (KH_SSAO_SEP: 0 / 1 = x / y at the first two levels' stride, 2 / 3 = x / y at
                       // the third level's - the stride the full-res apply used to blur at).
    // The FULL-resolution pixel rectangle every pass draws over (x = left,
    // y = top, z = right, w = bottom, exclusive): a term texel outside it was
    // not written this frame and must not be read.
    float4 khsaRect;
    float4 khsaHalf;   // x, y = the half-resolution grid's width and height (px); z = the near-z route's near
                       // (0 = no route this pass), w = its gap floor (KH_SSAO_NEARZ).
};

#if MSAA_DEPTH
Texture2DMS<float> khsaLive : register(t0);
Texture2DMS<uint2> khsaMark : register(t1);   // The main depth's stencil plane (.g); sample 0.
float KhSaLive(int2 khsl_p) { return khsaLive.Load(khsl_p, 0); }
uint  KhSaMark(int2 khsm_p) { return khsaMark.Load(khsm_p, 0).g; }
#else
Texture2D<float> khsaLive : register(t0);
Texture2D<uint2> khsaMark : register(t1);
float KhSaLive(int2 khsl_p) { return khsaLive.Load(int3(khsl_p, 0)); }
uint  KhSaMark(int2 khsm_p) { return khsaMark.Load(int3(khsm_p, 0)).g; }
#endif
Texture2D<float> khsaAo    : register(t2);   // The term on the half grid (the blurs and the apply read it).
Texture2D<float> khsaDepth : register(t3);   // PSSsaoDepth's half-res sample-0 depth (metres).

#define KH_SA_NS 2           // KH_SSAO_VB: slices per pixel, two sides each...
#define KH_SA_M 4            // ...and samples per side: 16 depth reads, the former gather's count.
#define KH_SA_THICK 0.5f     // A sample's solid, deep behind its face, as a share of the radius.
// The term's scale. Against the ray-traced reference (cosine-weighted, this radius and falloff) the estimate reads
// on average about 0.63 of it at this sample count (sparse coverage along each line: about 0.85 at 8 samples a side,
// 0.9 at 16) and the former gather read about 0.32; the gain keeps the former overall level at a given strength
// up close. From about 10 m, where both miss most of the reference, the level depends on the scene: in the harness
// (scenes 30 / 31) our mesh on engine ground reads 2.1 - 2.3 x the former's mean occlusion at 12.5 m and about
// 1.35 x at 17.5 m (nearer the former's level), and with the ground ours too about 0.9 x and 0.5 x.
// Where neither holds up: a sub-pixel thin object before a surface at range (a pole at 18 m; its pixels' normal is a
// depth step) reads as a dark column, somewhat darker than the former's, and a contact whose ground within the radius
// is a pixel or two tall on screen (grazing, at range) is stepped over by both.
#define KH_SA_GAIN 0.5f
#define KH_SA_RANGE_FRAC 0.05f   // The radius is at least this share of the distance.

// The 4 x 4 ordered (Bayer) tile, 0..15: the pixel's rotation phase.
static const float KH_SA_BAYER[16] = {
     0.0f,  8.0f,  2.0f, 10.0f,
    12.0f,  4.0f, 14.0f,  6.0f,
     3.0f, 11.0f,  1.0f,  9.0f,
    15.0f,  7.0f, 13.0f,  5.0f,
};
float KhSaPhase(int2 khsh_p)   // (tile + 0.5) / 16.
{
    return (KH_SA_BAYER[(khsh_p.y & 3) * 4 + (khsh_p.x & 3)] + 0.5f) * (1.0f / 16.0f);
}

int2 KhSaClampFull(int2 khsc_p)
{
    return clamp(khsc_p, int2(0, 0), int2((int)khsaVp.z - 1, (int)khsaVp.w - 1));
}
int2 KhSaClampHalf(int2 khsc_p)
{
    return clamp(khsc_p, int2(0, 0), int2((int)khsaHalf.x - 1, (int)khsaHalf.y - 1));
}
// Is a half-grid pixel inside the rectangle (whose full-res edges round outward)?
// The rectangle lies inside the target (kh_ssao_viewport clamps it), so this is
// also every pass's test that a half-grid tap is on the grid.
bool KhSaInRectHalf(int2 khsr_p)
{
    return khsr_p.x >= (int)(khsaRect.x * 0.5f) && khsr_p.y >= (int)(khsaRect.y * 0.5f) &&
           khsr_p.x < (int)((khsaRect.z + 1.0f) * 0.5f) && khsr_p.y < (int)((khsaRect.w + 1.0f) * 0.5f);
}

// Raw depth to metres along the view axis; 1e9 for the far clear and beyond.
float KhSaMeters(float khsm_raw)
{
    float khsm_ndc = (khsm_raw - khsaVp.x) / max(khsaVp.y - khsaVp.x, 1.0e-6f);
    float khsm_den = khsm_ndc - khsaProj.z;
    if (khsm_den > -1.0e-7f) return 1.0e9f;
    float khsm_d = khsaProj.w / khsm_den;
    return khsm_d > 0.0f ? khsm_d : 1.0e9f;
}
// The half-grid depth, metres (the sample at full pixel 2p; PSSsaoDepth stored
// it converted).
float KhSaDepthH(int2 khsd_hp)
{
    return khsaDepth.Load(int3(KhSaClampHalf(khsd_hp), 0));
}

// The view-space position of FULL pixel p (integer coordinates; the centre is
// taken here) at depth z: x right, y up, z forward.
float3 KhSaView(int2 khsv_p, float khsv_z)
{
    float2 khsv_uv = ((float2)khsv_p + 0.5f) / khsaVp.zw;
    return float3((khsv_uv.x * 2.0f - 1.0f) * khsv_z / khsaProj.x,
                  (1.0f - khsv_uv.y * 2.0f) * khsv_z / khsaProj.y,
                  khsv_z);
}
// ...and of half pixel hp, from the half grid's depth.
float3 KhSaViewH(int2 khsv_hp)
{
    return KhSaView(khsv_hp * 2, KhSaDepthH(khsv_hp));
}

// The normal at half pixel hp from four taps at stride khsn_st (half px), the
// nearer neighbour per axis, facing the camera. false when a tap leaves the
// surface (a depth step over khsn_tol) or the cross is degenerate; khsn_w =
// how far inside the tolerance the taps sit (1 well inside, fading to 0 at it).
bool KhSaNormal(int2 khsn_hp, float3 khsn_P, int khsn_st, float khsn_tol, out float3 khsn_N, out float khsn_w)
{
    khsn_N = float3(0.0f, 0.0f, -1.0f);
    khsn_w = 0.0f;
    const float3 khsn_L = KhSaViewH(khsn_hp + int2(-khsn_st, 0));
    const float3 khsn_R = KhSaViewH(khsn_hp + int2(khsn_st, 0));
    const float3 khsn_U = KhSaViewH(khsn_hp + int2(0, -khsn_st));
    const float3 khsn_D = KhSaViewH(khsn_hp + int2(0, khsn_st));
    const bool khsn_lx = abs(khsn_L.z - khsn_P.z) < abs(khsn_R.z - khsn_P.z);
    const bool khsn_ly = abs(khsn_U.z - khsn_P.z) < abs(khsn_D.z - khsn_P.z);
    const float3 khsn_x = khsn_lx ? khsn_L : khsn_R;
    const float3 khsn_y = khsn_ly ? khsn_U : khsn_D;
    const float khsn_dz = max(abs(khsn_x.z - khsn_P.z), abs(khsn_y.z - khsn_P.z));
    if (khsn_dz > khsn_tol) return false;
    khsn_w = 1.0f - smoothstep(0.5f * khsn_tol, khsn_tol, khsn_dz);
    const float3 khsn_dx = khsn_lx ? (khsn_P - khsn_L) : (khsn_R - khsn_P);
    const float3 khsn_dy = khsn_ly ? (khsn_P - khsn_U) : (khsn_D - khsn_P);
    float3 khsn_n = cross(khsn_dx, khsn_dy);
    const float khsn_nl = length(khsn_n);
    if (!(khsn_nl > 1.0e-12f)) return false;
    khsn_n /= khsn_nl;
    if (dot(khsn_n, khsn_P) > 0.0f) khsn_n = -khsn_n;   // Toward the camera (the origin).
    khsn_N = khsn_n;
    return true;
}
bool KhSaNormal(int2 khsn_hp, float3 khsn_P, int khsn_st, float khsn_tol, out float3 khsn_N)
{
    float khsn_w;
    return KhSaNormal(khsn_hp, khsn_P, khsn_st, khsn_tol, khsn_N, khsn_w);
}

// The test (see the header): this drawer's mark on the FULL pixel.
bool KhSaOurs(int2 khso_p)
{
    return KhSaMark(khso_p) == 1u;
}

// KH_SSAO_NEARZ: the depth of full pixel p in metres. A near-z routed draw of
// ours writes each fragment nearer than the pass's near plane on a straight
// line into the gap below the viewport's MinDepth (PSComposite's KH_ARB_DEPTH
// ramp: raw = gap + (MinDepth - gap) * z / near), which the projection reads
// as about the near distance for every one of them - a flat wall, occluding
// nothing. A raw in that gap on a pixel of ours decodes through the ramp
// instead; every other raw, and every raw of a pixel that is not ours (the
// engine's hands can lie in the gap when the flush draws late), decodes as
// before. khsaHalf.z = the route's near (0 = none), khsaHalf.w = the gap's
// floor; the gap's top is khsaVp.x.
float KhSaMetersAt(int2 khsz_p, float khsz_raw)
{
    if (khsaHalf.z > 0.0f && khsz_raw < khsaVp.x && khsz_raw >= khsaHalf.w && KhSaOurs(khsz_p)) {
        return max(khsaHalf.z * (khsz_raw - khsaHalf.w) / max(khsaVp.x - khsaHalf.w, 1.0e-9f), 1.0e-4f);
    }
    return KhSaMeters(khsz_raw);
}

// KH_NEARZ_MARK: (raw depth, true distance) at a full pixel of ours in the
// near-z gap - KhSaMetersAt's own test and decode - into the near-plane
// marker the late effect chain reads (LoadDepthPS, cb.hlsl); every other
// pixel is discarded and keeps what the marker held.
float2 PSSsaoNearMark(float4 pos : SV_Position) : SV_Target
{
    const int2 khnm_p = int2(pos.xy);
    const float khnm_raw = KhSaLive(khnm_p);
    if (!(khsaHalf.z > 0.0f && khnm_raw < khsaVp.x && khnm_raw >= khsaHalf.w && KhSaOurs(khnm_p))) discard;
    return float2(khnm_raw, KhSaMetersAt(khnm_p, khnm_raw));
}

// Half res: sample 0 of the live depth at full pixel 2p, in metres - the
// conversion every half-grid read took, taken once (R32_FLOAT keeps it exact).
float PSSsaoDepth(float4 pos : SV_Position) : SV_Target
{
    const int2 khsd_p = KhSaClampFull(int2(pos.xy) * 2);
    return KhSaMetersAt(khsd_p, KhSaLive(khsd_p));   // KH_SSAO_NEARZ.
}

// KH_SSAO_VB: one edge a -> b of a sample's solid in its slice ((s, c) = along the slice's tangent and normal, from
// the lifted receiver), clipped to the half-plane above the tangent plane (c > 0): widens [lo, hi] (sines of the angle
// from the normal) by a when a is above, and by the horizon (+-1, the side the edge crosses on) when the edge crosses
// it. Walking a closed outline's edges covers its every vertex and crossing, so [lo, hi] is the angular span of the
// outline's part above the plane - for a convex outline that does not hold the receiver (the caller's guard).
void KhSaEdge(float2 khse_a, float2 khse_b, inout float khse_lo, inout float khse_hi)
{
    // A vertex's sine stays inside +-1, which only a crossing reaches (the caller's guard reads +-1 as a crossing on
    // either side; float rounding takes a vertex within about 1e-3 of the plane to exactly 1).
    const float khse_sa = clamp(khse_a.x * rsqrt(max(dot(khse_a, khse_a), 1.0e-20f)), -0.99999f, 0.99999f);
    // The side the edge crosses on: the sign of s where c = 0, (b.s a.c - a.s b.c) / (a.c - b.c) (read only when a
    // and b lie either side, so a.c - b.c is not 0 and its sign is a's side).
    const float khse_ex = ((khse_b.x * khse_a.y - khse_a.x * khse_b.y >= 0.0f) == (khse_a.y > 0.0f)) ? 1.0f : -1.0f;
    if (khse_a.y > 0.0f) {
        khse_lo = min(khse_lo, khse_sa);
        khse_hi = max(khse_hi, khse_sa);
    }
    if ((khse_a.y > 0.0f) != (khse_b.y > 0.0f)) {
        khse_lo = min(khse_lo, khse_ex);
        khse_hi = max(khse_hi, khse_ex);
    }
}
// KH_SSAO_VB: GTAO's slice measure cos(t - n) |sin t| integrated to angle t = n + asin(s) (radians from the view
// vector; s = the sine of the angle from the normal; n = the normal's angle in the slice, khsg_sn / khsg_cn its sine
// and cosine): sgn(t) (cos n - cos(2t - n)) / 4 + |t| sin n / 2, 0 at t = 0 and rising over [n - pi/2, n + pi/2];
// cos(2t - n) = cos(n + 2 asin s) = cos n (1 - 2 s^2) - 2 sin n s sqrt(1 - s^2).
float KhSaG(float khsg_s, float khsg_n, float khsg_sn, float khsg_cn)
{
    const float khsg_t = khsg_n + asin(khsg_s);
    const float khsg_c2 = khsg_cn * (1.0f - 2.0f * khsg_s * khsg_s)
                        - 2.0f * khsg_sn * khsg_s * sqrt(saturate(1.0f - khsg_s * khsg_s));
    const float khsg_v = 0.25f * (khsg_cn - khsg_c2) + 0.5f * khsg_t * khsg_sn;
    return khsg_t >= 0.0f ? khsg_v : -khsg_v;
}
// KH_SSAO_VB: the sectors of 32 between two shares of the slice's measure (0 .. 1), edges rounded to the nearest.
uint KhSaBits(float khsb_f0, float khsb_f1)
{
    const uint khsb_b0 = (uint)round(saturate(khsb_f0) * 32.0f);
    const uint khsb_b1 = (uint)round(saturate(khsb_f1) * 32.0f);
    const uint khsb_n = khsb_b1 > khsb_b0 ? khsb_b1 - khsb_b0 : 0u;
    // n = 32 takes the first arm; the second, evaluated anyway, shifts by 32 (masked to 0) - harmless.
    return khsb_n >= 32u ? 0xFFFFFFFFu : (((1u << khsb_n) - 1u) << khsb_b0);
}

// Half res: the term at half pixel hp (full pixel 2hp).
float PSSsaoMain(float4 pos : SV_Position) : SV_Target
{
    const int2 khsp_hp = int2(pos.xy);
    const int2 khsp_p = khsp_hp * 2;
    if (!KhSaOurs(khsp_p)) return 1.0f;
    const float khsp_z = KhSaDepthH(khsp_hp);
    if (khsp_z > 1.0e8f) return 1.0f;
    const float3 khsp_P = KhSaView(khsp_p, khsp_z);

    const float khsp_rad = max(khsaCtl.x, khsp_z * KH_SA_RANGE_FRAC);   // Distance-adaptive (see the header).
    // The radius on screen in HALF pixels: fade the term out as it shrinks
    // to a few pixels (nothing left to measure) and floor the taps at 2 px.
    const float khsp_spx0 = khsp_rad * khsaProj.y * 0.5f * khsaHalf.y / khsp_z;
    float khsp_conf = smoothstep(1.0f, 3.0f, khsp_spx0);
    if (khsp_conf <= 0.0f) return 1.0f;
    const float khsp_spx = clamp(khsp_spx0, 2.0f, 256.0f);

    // The normal: the 1 px one always; the wide baseline (a third of the
    // radius on screen) blended over it by how well its taps stay on the
    // surface - a blend, not a switch, so a crease sliding under the pixel
    // does not flip the plane between frames.
    float3 khsp_N;
    if (!KhSaNormal(khsp_hp, khsp_P, 1, 1.0e9f, khsp_N)) return 1.0f;
    {
        const int khsp_wst = (int)clamp(khsp_spx * 0.33f, 2.0f, 24.0f);
        float3 khsp_Nw;
        float khsp_ww = 0.0f;
        if (KhSaNormal(khsp_hp, khsp_P, khsp_wst, khsp_rad, khsp_Nw, khsp_ww)) {
            khsp_ww *= smoothstep(0.82f, 0.94f, dot(khsp_Nw, khsp_N));   // KH_SSAO_NWIDE (the header).
            khsp_N = normalize(lerp(khsp_N, khsp_Nw, khsp_ww));
        }
    }
    const float3 khsp_rd = khsp_P / max(length(khsp_P), 1.0e-4f);   // The pixel's own ray.
    // Grazing confidence: the reconstruction is noise at the silhouette's
    // slope (the SSGI window).
    khsp_conf *= smoothstep(0.008f, 0.05f, abs(dot(khsp_N, khsp_rd)));
    // The receiver's lift off its own surface (the header): a share of the
    // radius, plus a little distance.
    const float  khsp_pfl = 0.02f * khsp_rad + 0.001f * khsp_z;
    const float3 khsp_V = -khsp_rd;
    const float3 khsp_Pr = khsp_P + khsp_N * khsp_pfl;
    const float  khsp_T = KH_SA_THICK * khsp_rad;
    // Position-only seeds from the ordered tile: the slices' rotation, and the
    // samples' jitter along the line from the same phase.
    const float khsp_ph0 = KhSaPhase(khsp_hp);
    const float khsp_jk = frac(khsp_ph0 * 0.61803399f + 0.37f);

    float khsp_occ = 0.0f;
    [loop] for (int khsp_si = 0; khsp_si < KH_SA_NS; ++khsp_si) {
        // The slice: a screen direction and the plane through the view ray it
        // spans (the view-space step of one pixel along it, at a unit depth);
        // the normal's projection into it (n, its length npl, its angle na from
        // the view vector) and the tangent t along the plane.
        const float  khsp_an = ((float)khsp_si + khsp_ph0) * (3.14159265f / (float)KH_SA_NS);
        const float2 khsp_dir = float2(cos(khsp_an), sin(khsp_an));   // Screen, y down.
        const float3 khsp_dv = normalize(float3(khsp_dir.x / (khsaProj.x * khsaVp.z),
                                                -khsp_dir.y / (khsaProj.y * khsaVp.w), 0.0f));
        const float3 khsp_o = normalize(khsp_dv - khsp_V * dot(khsp_dv, khsp_V));
        const float3 khsp_ax = cross(khsp_o, khsp_V);
        const float3 khsp_np = khsp_N - khsp_ax * dot(khsp_N, khsp_ax);
        const float  khsp_npl = length(khsp_np);
        if (khsp_npl < 1.0e-4f) continue;
        const float3 khsp_n = khsp_np / khsp_npl;
        const float3 khsp_t = cross(khsp_n, khsp_ax);   // n rotated a quarter turn toward o.
        const float  khsp_sn = dot(khsp_n, khsp_o);
        const float  khsp_cn = dot(khsp_n, khsp_V);      // > 0: the normal faces the camera.
        const float  khsp_na = atan2(khsp_sn, khsp_cn);
        // The slice's measure at its hemisphere's low end (KhSaG at s = -1) and over the whole of it (cos n + n sin n,
        // at least 1).
        const float  khsp_gl = -0.5f * khsp_cn - 0.5f * (khsp_na - 1.5707963f) * khsp_sn;
        const float  khsp_gt = khsp_cn + khsp_na * khsp_sn;
        const float  khsp_gk = 1.0f / khsp_gt;
        // Heights in the slice are over the receiver's own tangent plane (N), scaled into the slice's frame (1 / npl):
        // a sample rounded off the slice's line lies a little off its plane, and the slice normal n's height would
        // take N's part along the line's axis in with it.
        const float  khsp_ik = 1.0f / khsp_npl;
        uint  khsp_mask = 0u;
        float khsp_so = 0.0f;
        [loop] for (int khsp_sd2 = 0; khsp_sd2 < 2; ++khsp_sd2) {
            const float2 khsp_sdir = khsp_sd2 == 0 ? khsp_dir : -khsp_dir;
            // The chain opens at the receiver (its point and its back), so the
            // first sample, continuous with it, claims the sectors down to the
            // horizon; pz / pf / pb / pw = the last sample's depth, front, back
            // (in the slice) and falloff weight.
            bool   khsp_pv = true;
            float  khsp_pz = khsp_z;
            float  khsp_pw = 1.0f;
            const float3 khsp_q0 = khsp_P - khsp_Pr;
            const float3 khsp_q0b = khsp_q0 + khsp_rd * khsp_T;
            float2 khsp_pf = float2(dot(khsp_q0, khsp_t), dot(khsp_q0, khsp_N) * khsp_ik);
            float2 khsp_pb = float2(dot(khsp_q0b, khsp_t), dot(khsp_q0b, khsp_N) * khsp_ik);
            [loop] for (int khsp_k = 0; khsp_k < KH_SA_M; ++khsp_k) {
                // Evenly out along the line to the radius on screen, jittered;
                // at least 1.05 half px (the self-sample floor).
                const float khsp_sr = max(((float)khsp_k + khsp_jk) / (float)KH_SA_M * khsp_spx, 1.05f);
                const int2  khsp_sp = int2((float2)khsp_hp + 0.5f + khsp_sdir * khsp_sr);
                // Samples outside the rectangle reject rather than clamp (a clamp
                // would read the border pixel's surface as a neighbour): their
                // half-grid depth was not written this frame, and the rectangle
                // test is the on-grid test too (the blurs take the same rule).
                // Either break breaks the chain.
                if (!KhSaInRectHalf(khsp_sp)) {
                    khsp_pv = false;
                    continue;
                }
                const float khsp_sd = KhSaDepthH(khsp_sp);
                if (khsp_sd > 1.0e8f) {   // Sky: nothing there to occlude.
                    khsp_pv = false;
                    continue;
                }
                const float3 khsp_S = KhSaView(khsp_sp * 2, khsp_sd);
                const float3 khsp_B = khsp_S + khsp_S * (khsp_T / max(length(khsp_S), 1.0e-4f));   // Its back.
                const float  khsp_d = length(khsp_S - khsp_P);
                const float3 khsp_qf = khsp_S - khsp_Pr;
                const float3 khsp_qb = khsp_B - khsp_Pr;
                const float2 khsp_f = float2(dot(khsp_qf, khsp_t), dot(khsp_qf, khsp_N) * khsp_ik);
                const float2 khsp_b = float2(dot(khsp_qb, khsp_t), dot(khsp_qb, khsp_N) * khsp_ik);
                const bool   khsp_br = khsp_pv && abs(khsp_sd - khsp_pz) < khsp_T;   // One surface with the last.
                float khsp_lo = 2.0f;
                float khsp_hi = -2.0f;
                if (khsp_br) {   // The quad front -> back -> last back -> last front.
                    KhSaEdge(khsp_f, khsp_b, khsp_lo, khsp_hi);
                    KhSaEdge(khsp_b, khsp_pb, khsp_lo, khsp_hi);
                    KhSaEdge(khsp_pb, khsp_pf, khsp_lo, khsp_hi);
                    KhSaEdge(khsp_pf, khsp_f, khsp_lo, khsp_hi);
                }
                // A convex outline that does not hold the receiver spans less than the half-plane; the whole of it
                // means the quad holds it (the receiver's own quad, at a crease seen at a grazing angle): the
                // segment alone then.
                if (!khsp_br || (khsp_lo <= -1.0f && khsp_hi >= 1.0f)) {   // The segment front -> back (and back).
                    khsp_lo = 2.0f;
                    khsp_hi = -2.0f;
                    KhSaEdge(khsp_f, khsp_b, khsp_lo, khsp_hi);
                    KhSaEdge(khsp_b, khsp_f, khsp_lo, khsp_hi);
                }
                const float khsp_fall = saturate(1.0f - (khsp_d * khsp_d) / (khsp_rad * khsp_rad));
                const float khsp_fw = khsp_fall * khsp_fall;
                // A quad stands for the surface from the last sample out to this
                // one: its sectors take the mean of the two ends' falloff.
                const float khsp_wq = khsp_br ? 0.5f * (khsp_fw + khsp_pw) : khsp_fw;
                khsp_pv = khsp_d < khsp_rad;   // Beyond the radius: no sectors, and the chain breaks.
                khsp_pz = khsp_sd;
                khsp_pw = khsp_fw;
                khsp_pf = khsp_f;
                khsp_pb = khsp_b;
                if (!khsp_pv || !(khsp_hi > khsp_lo)) continue;
                // The span's sines to the slice's measure, then to sectors.
                const float khsp_g0 = KhSaG(clamp(khsp_lo, -1.0f, 1.0f), khsp_na, khsp_sn, khsp_cn);
                const float khsp_g1 = KhSaG(clamp(khsp_hi, -1.0f, 1.0f), khsp_na, khsp_sn, khsp_cn);
                const uint  khsp_bits = KhSaBits((khsp_g0 - khsp_gl) * khsp_gk, (khsp_g1 - khsp_gl) * khsp_gk);
                const uint  khsp_new = khsp_bits & ~khsp_mask;
                khsp_mask = khsp_mask | khsp_bits;
                khsp_so += (float)countbits(khsp_new) * khsp_wq;
            }
        }
        // The slice's occluded share of the cosine-weighted hemisphere: its
        // normal's projected length times the measure its sectors hold.
        khsp_occ += khsp_npl * khsp_gt * khsp_so * (1.0f / 32.0f);
    }
    // The slices' mean, the gain (KH_SA_GAIN) and the confidence.
    const float khsp_ao = 1.0f - saturate(khsp_occ * (KH_SA_GAIN / (float)KH_SA_NS)) * khsp_conf;
    // khsaCtl.y is the strength as an exponent (1 = as measured, more deepens).
    return pow(max(khsp_ao, 1.0e-4f), khsaCtl.y);
}

// KH_SSAO_SEP: the depth-weighted blur of the term at half pixel hp along ONE axis - five taps weighted by
// distance (exp(-i^2 / 8), the old 5 x 5's Gaussian, which is a product of two of these) and by relative depth
// agreement (the SSGI resolve's weights) - at a stride (half px) that grows with the radius on screen. An x pass
// then a y pass is the separable form of the former 5 x 5 (ten taps for twenty-five; the depth weight is taken per
// axis, the textbook approximation). khsaCtl.w: bit 0 = the axis (0 x, 1 y); >= 2 = the third level's stride rule
// (the one the full-res apply blurred at before it became a pure upsample). Taps outside the rectangle and sky
// taps are dropped, and so are taps that are not ours (KH_SSAO_OURS, below).
//
// KH_SSAO_TILE: the first level's stride is odd. The gather's rotation comes from a 4 x 4 ordered tile whose four
// parity classes (even / odd column x even / odd row) each hold one contiguous quarter of the sixteen phases, so a
// level whose taps are all an even number of pixels apart never mixes them: at stride 2 (and 4 at the next two
// levels, a radius of 25 - 31 half px on screen) each pixel kept one quarter of the rotations, and a fixed 4 x 4
// pattern stayed in the term. Five taps at an odd stride land on all four columns of the tile (offsets 0, +-s,
// +-2s cover every residue mod 4), so the first level's x pass then y pass averages all sixteen; strides 1 and 3
// are unchanged and 2 becomes 3 (its neighbours' rule from a radius of 37.5 half px on). The stride's maximum, and
// with it the passes' reach (KH_SSAO_RECT_PAD), is unchanged.
//
// KH_SSAO_OURS: the term exists only at our pixels - PSSsaoMain writes 1 (no occlusion) everywhere else - so a
// tap on a pixel that is not ours carries no measurement, only that placeholder. Where one of our meshes touches
// or pierces an engine surface the two share a depth, the depth weight admits the engine's pixels at full weight,
// and their 1s washed out the contact's occlusion exactly where it is strongest: a bright halo along every
// contact with engine geometry (never between two of our meshes, whose pixels all carry real terms). A masked
// filter reads its mask: the blurs and the apply's upsample take our taps alone.
float KhSaBlur(int2 khsb_hp, float khsb_z)
{
    const float khsb_rad = max(khsaCtl.x, khsb_z * KH_SA_RANGE_FRAC);
    const float khsb_spx = khsb_rad * khsaProj.y * 0.5f * khsaHalf.y / khsb_z;
    const int khsb_m = (int)(khsaCtl.w + 0.5f);
    const int2 khsb_ax = (khsb_m & 1) ? int2(0, 1) : int2(1, 0);
    int khsb_st = (khsb_m >= 2)
                ? (int)clamp(khsb_spx * 0.08f * max(khsaCtl.z, 1.0f), 1.0f, 4.0f)
                : (int)(clamp(khsb_spx * 0.08f, 1.0f, 3.0f) * max(khsaCtl.z, 1.0f));
    if (khsaCtl.z < 1.5f) khsb_st |= 1;   // KH_SSAO_TILE: the first level (ctl.z = 1) at an odd stride.
    float khsb_sum = 0.0f;
    float khsb_wsum = 0.0f;
    [unroll] for (int khsb_i = -2; khsb_i <= 2; ++khsb_i) {
        const int2 khsb_q = khsb_hp + khsb_ax * (khsb_i * khsb_st);
        if (!KhSaInRectHalf(khsb_q)) continue;   // Not written this frame (or off the grid).
        if (!KhSaOurs(khsb_q * 2)) continue;     // KH_SSAO_OURS: the placeholder, not a term.
        const float khsb_qz = KhSaDepthH(khsb_q);
        if (khsb_qz > 1.0e8f) continue;
        const float khsb_dz = abs(khsb_qz - khsb_z) / (khsb_z * 0.06f + 0.05f);
        const float khsb_w = exp(-0.125f * (float)(khsb_i * khsb_i)) * exp(-khsb_dz * khsb_dz);
        khsb_sum += khsaAo.Load(int3(khsb_q, 0)) * khsb_w;
        khsb_wsum += khsb_w;
    }
    return khsb_wsum > 1.0e-4f ? khsb_sum / khsb_wsum : khsaAo.Load(int3(khsb_hp, 0));
}

// Half res, one axis of one blur level (KH_SSAO_SEP) at our pixels; a pixel
// that is not ours passes its value through (the next pass reads it).
float PSSsaoBlur(float4 pos : SV_Position) : SV_Target
{
    const int2 khsr_hp = int2(pos.xy);
    if (!KhSaOurs(khsr_hp * 2)) return khsaAo.Load(int3(khsr_hp, 0));
    const float khsr_z = KhSaDepthH(khsr_hp);
    if (khsr_z > 1.0e8f) return khsaAo.Load(int3(khsr_hp, 0));
    return KhSaBlur(khsr_hp, khsr_z);
}

// Full res: the term at this pixel from the half grid, depth-guided - a joint
// bilateral upsample (KH_SSAO_SEP; the blurring is all done on the half grid)
// over our texels alone (KH_SSAO_OURS) -
// dithered, multiplied into the scene at our pixels. The stencil test runs
// ahead of the shader (the state writes nothing, so the discard below cannot
// need a late test): a pixel with no marked sample is never shaded, and the
// output reaches only the samples that pass. The sample-0 test stays - it is
// what makes the guiding depth below ours.
[earlydepthstencil]
float4 PSSsaoApply(float4 pos : SV_Position) : SV_Target
{
    const int2 khsq_p = int2(pos.xy);
    if (!KhSaOurs(khsq_p)) discard;   // The multiply blend leaves the pixel.
    const float khsq_z = KhSaMetersAt(khsq_p, KhSaLive(khsq_p));   // KH_SSAO_NEARZ.
    const int2 khsq_hp = khsq_p >> 1;
    // KH_SSAO_SEP: the joint bilateral upsample (Kopf et al. 2007). Half texel hp stands at full pixel 2 hp
    // (PSSsaoDepth and the gather read there), so this pixel lies at hp + (p & 1) / 2 on the half grid: the four
    // texels around it take their bilinear weights (an even pixel is its own texel's alone) times the blurs'
    // depth weight against this pixel's full-resolution depth. Taps off the rectangle and sky taps drop; none
    // left falls back to the texel.
    const float2 khsq_f = float2(khsq_p & 1) * 0.5f;
    float khsq_sum = 0.0f;
    float khsq_wsum = 0.0f;
    [unroll] for (int khsq_j = 0; khsq_j <= 1; ++khsq_j) {
        [unroll] for (int khsq_i = 0; khsq_i <= 1; ++khsq_i) {
            const float khsq_bw = (khsq_i ? khsq_f.x : 1.0f - khsq_f.x) * (khsq_j ? khsq_f.y : 1.0f - khsq_f.y);
            if (khsq_bw <= 0.0f) continue;
            const int2 khsq_q = khsq_hp + int2(khsq_i, khsq_j);
            if (!KhSaInRectHalf(khsq_q)) continue;
            if (!KhSaOurs(khsq_q * 2)) continue;   // KH_SSAO_OURS.
            const float khsq_qz = KhSaDepthH(khsq_q);
            if (khsq_qz > 1.0e8f) continue;
            const float khsq_dz = abs(khsq_qz - khsq_z) / (khsq_z * 0.06f + 0.05f);
            const float khsq_w = khsq_bw * exp(-khsq_dz * khsq_dz);
            khsq_sum += khsaAo.Load(int3(khsq_q, 0)) * khsq_w;
            khsq_wsum += khsq_w;
        }
    }
    // KH_SSAO_OURS: an odd pixel of ours whose two texels are not (a sliver of our mesh one pixel wide between
    // engine pixels) has no term among its four; it takes the depth-weighted mean of ours in the 3 x 3 around.
    if (!(khsq_wsum > 1.0e-4f)) {
        [unroll] for (int khsq_n = -1; khsq_n <= 1; ++khsq_n) {
            [unroll] for (int khsq_m = -1; khsq_m <= 1; ++khsq_m) {
                const int2 khsq_q = khsq_hp + int2(khsq_m, khsq_n);
                if (!KhSaInRectHalf(khsq_q) || !KhSaOurs(khsq_q * 2)) continue;
                const float khsq_qz = KhSaDepthH(khsq_q);
                if (khsq_qz > 1.0e8f) continue;
                const float khsq_dz = abs(khsq_qz - khsq_z) / (khsq_z * 0.06f + 0.05f);
                const float khsq_w = exp(-khsq_dz * khsq_dz);
                khsq_sum += khsaAo.Load(int3(khsq_q, 0)) * khsq_w;
                khsq_wsum += khsq_w;
            }
        }
    }
    float khsq_ao = khsq_wsum > 1.0e-4f ? khsq_sum / khsq_wsum : khsaAo.Load(int3(KhSaClampHalf(khsq_hp), 0));
    // The R8 term's own quantisation would band on a smooth gradient: the
    // resolve's position-only dither (effect 24's), a step's width.
    const float khsq_ig = frac(52.9829189f * frac(0.06711056f * pos.x + 0.00583715f * pos.y));
    const float khsq_ig2 = frac(52.9829189f * frac(0.06711056f * (pos.x + 5.588238f) + 0.00583715f * (pos.y + 5.588238f)));
    khsq_ao = saturate(khsq_ao + (khsq_ig - khsq_ig2) * (1.0f / 255.0f));
    return float4(khsq_ao, khsq_ao, khsq_ao, 1.0f);
}
