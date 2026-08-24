// g_hlsl_static - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
VSOut VSMain(VSIn i)
{
    VSOut o;
    float3 wp = centerSize.xyz + KhRotate(i.pos * sizeAxes.xyz);
    // FP32 JITTER REBASE (see centerRel): when armed, transform the
    // CAMERA-RELATIVE position through the REBASED viewProj - the
    // world-absolute fp32 cancellation (the stationary micro-jitter's
    // reducible term) never enters the position path.
    float3 khvTp = (centerRel.w > 0.5f)
                 ? (centerRel.xyz + KhRotate(i.pos * sizeAxes.xyz))
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
        o.pos = khClip;
    } else {
        o.pos = (stenVol2.z >= 0.5f) ? mul(float4(khEngP, 1.0f), khEngVP)
                                     : khClip;
    }
    // The farVis-off pop at max view distance is enforced per fragment in the
    // PS (FAR CONTRACT block) instead of here.
    o.wpos = wp;
    // KH_SELF_REL_INTERP: subtract the SAME fp32 anchor the sun matrices
    // subtract - the quantised anchor cancels exactly, and the interpolant
    // leaves at metres scale.
    o.wrel = wp - sunOrigin.xyz;
    // Per-axis scale is non-uniform: normals take the inverse scale, then the
    // object rotation (the inverse-transpose of scale-then-rotate for
    // orthonormal R - see kh_set_rotation).
    o.nrm = normalize(KhRotate(i.nrm / max(sizeAxes.xyz, float3(1e-4f, 1e-4f, 1e-4f))));
#if KH_TEXTURED
    o.uv = i.uv;
    // Tangents are COVARIANT (transform like positions, not normals):
    // per-axis scale then the object rotation, renormalized. The handedness
    // sign rides untouched in w.
    o.tanw = float4(normalize(KhRotate(i.tan.xyz * sizeAxes.xyz)), i.tan.w);
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

)HLSL" R"HLSL(   // CHUNK BOUNDARY (precedent; chunks concatenate).
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

)HLSL" R"HLSL(
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
)HLSL" R"HLSL(float4 PSMaskCast(VSOut i) : SV_Target
{
    // The field then produced the.x signature of a CONSTANT z BIAS (drift
    // growing as fragments near, fine at range - relative error c/z), the
    // mark of a near-offset encoding, while the engine's own resolve reads.y.
    float2 dimsM = float2(castView[1].z, castView[1].w);
    uint dw, dh;
    sceneDepthTex.GetDimensions(dw, dh);
    int2 px = int2(i.pos.xy * float2(dw, dh) / max(dimsM, float2(1.0f, 1.0f)));
    float4 zt = sceneDepthTex.Load(int3(px, 0));
    float zl = zt.y > 0.0f ? zt.y : zt.x;

    bool khcNearOk = zl > 1.2f || castView[0].w >= 0.5f;

    if (!khcNearOk && zl > 0.05f) {
        float khcM0 = max(sceneDepthTex.Load(int3(int2(dw >> 2, dh >> 1), 0)).x,
                          sceneDepthTex.Load(int3(int2(dw >> 2, dh >> 1), 0)).y);
        float khcM1 = max(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 2), 0)).x,
                          sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 2), 0)).y);
        float khcM2 = max(sceneDepthTex.Load(int3(int2((dw * 3) >> 2, (dh * 3) >> 2), 0)).x,
                          sceneDepthTex.Load(int3(int2((dw * 3) >> 2, (dh * 3) >> 2), 0)).y);
        float khcM3 = max(sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 4), 0)).x,
                          sceneDepthTex.Load(int3(int2(dw >> 1, dh >> 4), 0)).y);
        float khcHi = max(max(khcM0, khcM1), max(khcM2, khcM3));
        float khcLo = min(min(khcM0, khcM1), min(khcM2, khcM3));
        khcNearOk = khcHi > 1.2f ||
                    (khcLo > 0.05f && khcHi > khcLo * 1.3f);   // perspective structure
    }

    float2 ndc = float2(i.pos.x / dimsM.x * 2.0f - 1.0f, 1.0f - i.pos.y / dimsM.y * 2.0f);
    float3 vp = float3(ndc.x * castView[1].x, ndc.y * castView[1].y, 1.0f) * zl;
    float3 q = vp - castView[0].xyz;
    float3 pw;
    pw.x = q.x * castMat[0].x + q.y * castMat[0].y + q.z * castMat[0].z;
    pw.y = q.x * castMat[1].x + q.y * castMat[1].y + q.z * castMat[1].z;
    pw.z = q.x * castMat[2].x + q.y * castMat[2].y + q.z * castMat[2].z;

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

            [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                float3 lce = khrLocalityExt[li * 2].xyz;
                float3 lhe = khrLocalityExt[li * 2 + 1].xyz;
                float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                float3 ld = max(abs(pw - lce) - lhe, 0.0f);
                if (dot(ld, ld) < lr * lr) near_ok = true;
            }
        } else if (localityMeta.x >= 0.5f && localityMeta.x <= 16.5f) {
            int lc = (int)localityMeta.x;

            [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                float3 lce = locality[li * 2].xyz;
                float3 lhe = locality[li * 2 + 1].xyz;
                float lr = min(length(lhe) * stretch, max(600.0f, length(lhe) * 24.0f));
                float3 ld = max(abs(pw - lce) - lhe, 0.0f);
                if (dot(ld, ld) < lr * lr) near_ok = true;
            }
        } else {
            float castR = length(sizeAxes.xyz) * 0.5f;
            float reach = min(castR * stretch, max(600.0f, castR * 24.0f));
            float3 toCast = max(abs(pw - centerSize.xyz) - sizeAxes.xyz * 0.5f, 0.0f);
            near_ok = dot(toCast, toCast) < reach * reach;   // AABB-surface (above)
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
)HLSL" R"HLSL(            abs(sd.z) > 1e-6f ? sd.z : 1e-6f);
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
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
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
)HLSL" R"HLSL(

    int bm = (int)sizeAxes.w;
#if KH_TEXTURED
    // KH_TEXTURED: sample BELOW the FAR CONTRACT + guard blocks (the
    // section-4 checkpoint - the textured twin adds no return/discard above
    // them), cutout-clip, then build the mapped shading normal. The GEOMETRIC
    // normal keeps owning the receive gating below - shadow behavior stays in
    // parity with the untextured twin.
    KhMatSurf khtxS = KhSampleMat(i.uv);
    if (matParams0.y >= 0.5f) clip(khtxS.alpha - matParams0.z);   // cutout kill
    // OPAQUE ALPHA CONTRACT (the whitish-see-through fix): sampled alpha
    // NEVER reaches the blend - survivors draw at alpha 1 in BOTH modes.
    khtxS.alpha = 1.0f;
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
    }
#endif
    float smf = 1.0f;
    float  khStenZ = KhVolZ(i.wpos);
    float2 khStenG = float2(ddx(khStenZ), ddy(khStenZ));
    // Taken here, outside the divergent N.L branch, for the same quad-op
    // legality as the pair above. TWIN EDIT: PSMain and PSComposite carry the
    // identical line.
)HLSL" R"HLSL(   // THE SLICE IS A UNITS MISMATCH BETWEEN WHAT WE WRITE AND WHAT WE
    // This is the one place the twins legitimately differ; do not "restore"
    // it. COMPARE. This chunk declares SV_Depth and the contact clamp above
    // may have moved it, so the value that reached the depth buffer - and
    // therefore the value the witness snapshot holds - is khaODepth, NOT
    // i.pos.z.
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
)HLSL" R"HLSL(   // MODE 167 (lighting0.y == 3): cascade receive forced lit
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
            float khStRf = (lighting0.y >= 42.5f && lighting0.y < 43.5f)
                         ? 1.0f : KhSunRangeFade(i.wpos);
            smf *= 1.0f - (1.0f - khStenU) * khStRf;
        }
    }

    // Slices surviving this are painted by the ENGINE over our pixels, not by
    // any term of ours. TWIN EDIT: PSMain and PSComposite identical.
    if (lighting0.y >= 1.5f && lighting0.y < 2.5f) smf = 1.0f;
#if KH_TEXTURED
    khtxS.albedo *= color.rgb;   // the object color tints the albedo lane only
#if KH_USER_MAT
    float3 lc = KhUserShade(khtxS, i.wpos, khtxN, smf);
#else
    float3 lc = KhApplyPBR(khtxS, i.wpos, khtxN, smf);
#endif
#else
    float3 lc = ApplyLighting(color.rgb, i.wpos, i.nrm, smf);
#endif

    if (dbgCtl.x >= 4.5f && dbgCtl.x < 7.5f) {
        if (dbgCtl.x < 5.5f) return float4(color.rgb, 1.0f);
        if (dbgCtl.x < 6.5f) return float4(lc, 1.0f);
        return float4(color.a, SolidMask(i.wpos), smf, 1.0f);
    }

        if (maskMeta.y >= 0.5f) {
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
)HLSL" R"HLSL(   // CHUNK BOUNDARY - the visual-25 ledger took PSMain's
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
)HLSL" R"HLSL(   // CHUNK BOUNDARY - FIFTH C2026 CATCH OF THIS
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

)HLSL" R"HLSL(   // CHUNK BOUNDARY - the visual-22 ledger took this segment
        // At full optical depth our mesh paints exactly fog_target and
        // nothing else, so a silhouette can only exist if fog_target differs
        // from what the engine leaves on the pixels around it. Deliberately
        // placed AFTER the fogSky gradient resolves, so it paints the target
        // actually handed to the lerp, not a stand-in.
        if (dbgCtl.x >= 21.5f && dbgCtl.x < 22.5f) return float4(fog_target, 1.0f);
        lc = lerp(fog_target, lc, trans);
    }

#if KH_TEXTURED
    float a = color.a * khtxS.alpha * SolidMask(i.wpos);
#else
    float a = color.a * SolidMask(i.wpos);
#endif
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
        float3 tm = lerp(ts, tl, a);
        return float4(tm / max(1.0f - tm, 0.0039f), 1.0f);   // cap ~HDR 255
    }

    return float4(lc, a);
}
)HLSL"