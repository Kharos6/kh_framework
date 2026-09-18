// ssao.hlsl - KH_SSAO, the screen-space ambient occlusion of OUR meshes. A
// standalone unit (no cb.hlsl prefix; its own b0). Any edit changes the
// unit's shader cache key. C++ twins: KhSsaoCb, kh_ssao_pre, kh_ssao_post, KH_SSAO_MARK.
//
// Five draws after a drawer's depth-writing draws (the world injection's on
// the render thread, or the flush's late ones under the park), plus the
// C++ side's eraser draw. The term is computed on a HALF-RESOLUTION grid,
// as the SSGI's gather is: the cost is the meshes' screen coverage, and
// the term is a smooth field the full-resolution apply upsamples
// depth-guided (effect 24's recipe).
//   PSSsaoDepth - sample 0 of the live main depth at every other pixel,
//                 converted to metres (KhSaMeters), into an R32 half-res
//                 texture every later tap reads (one multisampled load and
//                 one conversion per half pixel instead of one per tap).
//   PSSsaoMain  - half res: the occlusion term into an R8 target, 1 wherever
//                 the pixel is not ours.
//   PSSsaoBlur  - twice, half res, ping-ponging two R8 targets: the SSGI
//                 chain's a-trous iterations (5 x 5, depth-weighted, stride
//                 doubling), at our pixels alone (the rest pass through).
//   PSSsaoApply - full res: the term through the SSGI resolve's joint-bilateral
//                 5 x 5 over the half grid, MULTIPLIED into the scene colour
//                 (dest.rgb *= src.rgb) at our samples alone: the C++ side
//                 binds the read-only depth view with a stencil test on the
//                 mark, so unmarked pixels never run it.
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
// (KhSceneMeters's recipe) and the viewport range the pass drew in; the view
// position is rebuilt from the pixel and that depth (KhgVpos's recipe). The
// normal is taken twice: over a 1 px baseline from the nearer neighbour on
// each screen axis (silhouette-safe: the cliff's slope is never the pixel's),
// and over a WIDE baseline that grows with the radius on screen, which
// averages across a mesh's facets the way its smooth shading normal does.
// The wide one is the reference plane wherever its four taps stay on the
// receiver's surface (depth-continuous within the radius); the 1 px one
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
// The gather is the SSGI branch's discipline (effect2.hlsl, effect 22), which
// is where every flicker of this kind was already paid for: taps on a
// screen-space disk of stratified annuli at golden-angle azimuths, rotated per
// pixel from the POSITION ALONE (no frame term: a static frame is
// bit-identical, every shimmer is real motion) - but by a 4 x 4 ORDERED tile
// of sixteen rotations, not the SSGI's gradient noise: the first a-trous
// iteration's 5 x 5 covers a whole tile, so after it every pixel holds the
// average over all sixteen rotations and the field no longer depends on
// where the tile lands on the surface. A skinned mesh is never still, and
// under a screen-anchored random pattern every frame re-rolled its taps; a 2 px
// floor on the tap radius, since a tap on the pixel's own footprint compares
// the surface with its own quantised depth; and no admission cliff - a tap's
// surface point is admitted by how far it rises above the receiver's tangent
// plane, smoothstepped over [floor, 2 floor] with a floor that grows with
// distance, never by a binary depth compare. The floor is a share of the
// radius plus a little distance: enough that a facet a few degrees off the
// plane occludes nothing, small enough that the term stays visible at range
// (the SSGI floor's larger distance term is for bounce acne, not needed
// here). The sum is normalised by tap count (a
// surviving-weight division turns one tap into a sparkle) and faded where the
// reconstruction cannot be trusted: at grazing incidence and where the radius
// spans only a few pixels. What one 5 x 5 could not hide - the tap pattern is
// anchored to the screen, so a moving mesh slides under it - the chain's
// three smoothing passes average out, as they do for the SSGI.
//
// The radius is world-space, so far away it is a few pixels and the term
// with it. As the distance grows the radius is floored at KH_SA_RANGE_FRAC of
// it (a constant footprint on screen from there on), which is what keeps a
// crease or a contact readable across a street: the larger-scale occlusion a
// far mesh shows is as real as the close one's, only coarser.
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
                       // multiplier of this draw (1, 2 for the a-trous pair, 3 for the apply); w unused.
    // The FULL-resolution pixel rectangle every pass draws over (x = left,
    // y = top, z = right, w = bottom, exclusive): a term texel outside it was
    // not written this frame and must not be read.
    float4 khsaRect;
    float4 khsaHalf;   // x, y = the half-resolution grid's width and height (px); z / w unused.
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

#define KH_SA_N 16   // Taps per pixel: 8 annuli, two azimuths each.
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

// Half res: sample 0 of the live depth at full pixel 2p, in metres - the
// conversion every half-grid read took, taken once (R32_FLOAT keeps it exact).
float PSSsaoDepth(float4 pos : SV_Position) : SV_Target
{
    return KhSaMeters(KhSaLive(KhSaClampFull(int2(pos.xy) * 2)));
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
            khsp_N = normalize(lerp(khsp_N, khsp_Nw, khsp_ww));
        }
    }
    const float3 khsp_rd = khsp_P / max(length(khsp_P), 1.0e-4f);   // The pixel's own ray.
    // Grazing confidence: the reconstruction is noise at the silhouette's
    // slope (the SSGI window).
    khsp_conf *= smoothstep(0.008f, 0.05f, abs(dot(khsp_N, khsp_rd)));
    // The plane floor: what a tap must rise above the tangent plane by to
    // count - a share of the radius, plus a little distance.
    const float khsp_pfl = 0.02f * khsp_rad + 0.001f * khsp_z;
    // Position-only seeds from the ordered tile: the azimuth phase, and a
    // per-annulus radial jitter from the same phase (so the blur's average
    // covers the radii as it covers the angles).
    const float khsp_ph0 = KhSaPhase(khsp_hp);
    const float khsp_rot = khsp_ph0 * 6.2831853f;
    const float khsp_ig2 = frac(khsp_ph0 * 0.61803399f + 0.37f);
    const float khsp_hn = (float)(KH_SA_N >> 1);

    float khsp_sum = 0.0f;
    // Annulus by annulus, its two azimuths inside: the angle base, the jitter
    // and the radius belong to the pair and are evaluated once (the taps and
    // their order are the per-tap loop's).
    [loop] for (int khsp_kp = 0; khsp_kp < (KH_SA_N >> 1); ++khsp_kp) {
        const float khsp_ab = (float)khsp_kp * 2.3999632f + khsp_rot;
        const float khsp_jk = frac(khsp_ig2 + (float)khsp_kp * 0.61803399f);
        // At least 1.05 half px: the self-sample floor (a half pixel = two full).
        const float khsp_sr = max(sqrt(((float)khsp_kp + khsp_jk) / khsp_hn) * khsp_spx, 1.05f);
        [loop] for (int khsp_par = 0; khsp_par < 2; ++khsp_par) {
            const float khsp_an = khsp_ab + (float)khsp_par * 3.14159265f;
            const float2 khsp_off = float2(cos(khsp_an), sin(khsp_an)) * khsp_sr;
            const int2 khsp_sp = int2((float2)khsp_hp + 0.5f + khsp_off);
            // Taps outside the rectangle reject rather than clamp (a clamp
            // would read the border pixel's surface as a neighbour): their
            // half-grid depth was not written this frame, and the rectangle
            // test is the on-grid test too (the blurs take the same rule).
            if (!KhSaInRectHalf(khsp_sp)) continue;
            const float khsp_sd = KhSaDepthH(khsp_sp);
            if (khsp_sd > 1.0e8f) continue;   // Sky: nothing there to occlude.
            const float3 khsp_v = KhSaView(khsp_sp * 2, khsp_sd) - khsp_P;
            const float khsp_d = length(khsp_v);
            if (khsp_d < 1.0e-4f) continue;
            const float khsp_ph = dot(khsp_N, khsp_v);   // Height above the tangent plane.
            const float khsp_pw = smoothstep(khsp_pfl, khsp_pfl * 2.0f, khsp_ph);
            if (khsp_pw <= 0.0f) continue;
            // Cosine-ish weight (the sine of the elevation) under a smooth range
            // falloff: a far foreground edge must not darken what is behind it.
            const float khsp_fall = saturate(1.0f - (khsp_d * khsp_d) / (khsp_rad * khsp_rad));
            khsp_sum += khsp_pw * (khsp_ph / khsp_d) * khsp_fall * khsp_fall;
        }
    }
    // Normalised by the tap count, scaled so a right-angled crease reads ~0.3.
    const float khsp_occ = saturate(khsp_sum * (2.0f / (float)KH_SA_N)) * khsp_conf;
    const float khsp_ao = 1.0f - khsp_occ;
    // khsaCtl.y is the strength as an exponent (1 = as measured, more deepens).
    return pow(max(khsp_ao, 1.0e-4f), khsaCtl.y);
}

// The 5 x 5 blur of the term at half pixel hp, weighted by distance and by
// relative depth agreement (the SSGI resolve's weights), at a stride (half
// px) that grows with the radius on screen times this draw's multiplier
// (khsaCtl.z). Taps outside the rectangle and sky taps are dropped.
float KhSaBlur(int2 khsb_hp, float khsb_z)
{
    const float khsb_rad = max(khsaCtl.x, khsb_z * KH_SA_RANGE_FRAC);
    const float khsb_spx = khsb_rad * khsaProj.y * 0.5f * khsaHalf.y / khsb_z;
    const int khsb_st = (int)(clamp(khsb_spx * 0.08f, 1.0f, 3.0f) * max(khsaCtl.z, 1.0f));
    float khsb_sum = 0.0f;
    float khsb_wsum = 0.0f;
    [unroll] for (int khsb_j = -2; khsb_j <= 2; ++khsb_j) {
        [unroll] for (int khsb_i = -2; khsb_i <= 2; ++khsb_i) {
            const int2 khsb_q = khsb_hp + int2(khsb_i, khsb_j) * khsb_st;
            if (!KhSaInRectHalf(khsb_q)) continue;   // Not written this frame (or off the grid).
            const float khsb_qz = KhSaDepthH(khsb_q);
            if (khsb_qz > 1.0e8f) continue;
            const float khsb_dz = abs(khsb_qz - khsb_z) / (khsb_z * 0.06f + 0.05f);
            const float khsb_w = exp(-0.125f * (float)(khsb_i * khsb_i + khsb_j * khsb_j)) * exp(-khsb_dz * khsb_dz);
            khsb_sum += khsaAo.Load(int3(khsb_q, 0)) * khsb_w;
            khsb_wsum += khsb_w;
        }
    }
    return khsb_wsum > 1.0e-4f ? khsb_sum / khsb_wsum : khsaAo.Load(int3(khsb_hp, 0));
}

// Half res, one a-trous iteration (effect 25's twin) at our pixels; a pixel
// that is not ours passes its value through (the next stride reads it).
float PSSsaoBlur(float4 pos : SV_Position) : SV_Target
{
    const int2 khsr_hp = int2(pos.xy);
    if (!KhSaOurs(khsr_hp * 2)) return khsaAo.Load(int3(khsr_hp, 0));
    const float khsr_z = KhSaDepthH(khsr_hp);
    if (khsr_z > 1.0e8f) return khsaAo.Load(int3(khsr_hp, 0));
    return KhSaBlur(khsr_hp, khsr_z);
}

// Full res: the term at this pixel from the half grid, depth-guided (the
// SSGI resolve's upsample: the full-res depth guides the half-res field),
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
    const float khsq_z = KhSaMeters(KhSaLive(khsq_p));
    const int2 khsq_hp = khsq_p >> 1;
    const float khsq_rad = max(khsaCtl.x, khsq_z * KH_SA_RANGE_FRAC);
    const float khsq_spx = khsq_rad * khsaProj.y * 0.5f * khsaHalf.y / max(khsq_z, 1.0e-3f);
    const int khsq_st = (int)clamp(khsq_spx * 0.08f * max(khsaCtl.z, 1.0f), 1.0f, 4.0f);
    float khsq_sum = 0.0f;
    float khsq_wsum = 0.0f;
    [unroll] for (int khsq_j = -2; khsq_j <= 2; ++khsq_j) {
        [unroll] for (int khsq_i = -2; khsq_i <= 2; ++khsq_i) {
            const int2 khsq_q = khsq_hp + int2(khsq_i, khsq_j) * khsq_st;
            if (!KhSaInRectHalf(khsq_q)) continue;
            const float khsq_qz = KhSaDepthH(khsq_q);
            if (khsq_qz > 1.0e8f) continue;
            const float khsq_dz = abs(khsq_qz - khsq_z) / (khsq_z * 0.06f + 0.05f);
            const float khsq_w = exp(-0.125f * (float)(khsq_i * khsq_i + khsq_j * khsq_j)) * exp(-khsq_dz * khsq_dz);
            khsq_sum += khsaAo.Load(int3(khsq_q, 0)) * khsq_w;
            khsq_wsum += khsq_w;
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
