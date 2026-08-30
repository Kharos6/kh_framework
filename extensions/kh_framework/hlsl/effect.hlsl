// effect.hlsl - plain HLSL, embedded in the DLL as RCDATA resource KH_EFFECT_HLSL by
// kh_shaders.rc (next to rendering_integration.hpp) and loaded at first use
// by kh_hlsl_src, which strips CR before the source is hashed for the shader
// cache, so the cache key does not depend on the checkout's line endings.
// Units are assembled by C++ concatenation of these resources, exactly as the
// old raw-string splice did; there is no #include and no size cap. Any edit
// here changes this unit's shader cache key (one cold recompile per user).

Texture2D<float4> sceneColor : register(t0);
Texture2D<float4> khsgTex : register(t3);
// KH_FX_SAMP_S2 (26783): s2, NOT s1. The shared prefix (g_cb_hlsl) declares
// khPfSamp at s1 unconditionally, and the prefix is compiled into THIS unit
// too, so both objects claimed s1 in one assembly. It built only because fxc
// drops an unreferenced resource and PSEffect happens not to reach khPfSamp -
// which is reachable from KhPfProbe / KhPfProbe2 and nothing else. The day any
// effect entry point calls a prefix helper that samples the moment pyramids,
// fxc fails the whole unit with X4509 (overlapping register semantics) and
// EVERY effect goes dark at once, with the error surfacing only at the
// runtime ensure. A unit-local resource yields to the shared prefix, not the
// other way round: the prefix is seen by four assemblies and this one is seen
// by one. s2 is free in all four. C++ twin: the bind and the save/restore
// bracket in the fx chain moved to slot 2 in the same build.
SamplerState khsgSamp : register(s2);   // linear CLAMP, bound only for the resolve draw

#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t1);
float LoadDepthPS(int2 px) { return depthTex.Load(px, 0); }
#else
// the snapshot is two-plane now; this pass wants the farthest,.x, which is
// what it has always received. Declaration only.
Texture2D<float2> depthTex : register(t1);
float LoadDepthPS(int2 px) { return depthTex.Load(int3(px, 0)).x; }
#endif

Texture2D<float> khArbSnap : register(t2);

// Bound at t19 only for LUT passes - t19 is RESERVED for this unit
// codebase-wide (it is inside StateBackup's widened save range, so the
// engine's own bind is restored after every flush). Sampled with integer
// Loads only (tetrahedral interpolation in the effect branch - no sampler, no
// half-texel bias to manage).
Texture3D<float4> khLut : register(t19);
float3 KhLutV(int3 p) { return khLut.Load(int4(p, 0)).rgb; }

float3 SampleScene(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    float4 khss = sceneColor.Load(int3(px, 0));
    // Every other caller is byte-identical.
    if (centerSize.w > 2.5f) return khss.rgb * khss.a;
    return khss.rgb;
}

// Meaningless in the scene phase - callers gate on the flag.
float KhUiCov(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    return sceneColor.Load(int3(px, 0)).a;
}

float Luma(float3 c) { return dot(c, float3(0.299f, 0.587f, 0.114f)); }

float LinDepth(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    // At/beyond the far plane the denominator crosses zero and flips sign.
    // Sky pixels sit at the depth-CLEAR value (1.0), which is beyond the
    // viewport depth range entirely - treat everything past the far plane as
    // "very far" instead of returning a negative distance.
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

float Hash(float2 p) { return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f); }

// quality-package helpers
float2 Hash2(float2 p)
{
    return float2(Hash(p), Hash(p + float2(41.13f, 7.77f)));
}

float4 KhRainLayer(float2 q, float tt, float2 grid, float seedOfs,
                   float amount, float wind, float stretch)
{
    float2 sq = float2(q.x - q.y * wind, q.y);
    float2 cellUv = sq * grid + float2(seedOfs, seedOfs * 0.37f);
    float2 id = floor(cellUv);
    float3 n = float3(Hash(id), Hash(id + 17.17f), Hash(id + 43.7f));
    if (n.x > amount) return float4(0, 0, 0, 0);
    float2 f = frac(cellUv);
    float x = (n.y - 0.5f) * 0.55f;
    x += (0.42f - abs(x)) * sin(tt * 0.7f + n.z * 6.2832f)
         * 0.4f * (1.0f - saturate(stretch * 1.4f));
    // Drop y: a descending cycle with a linger-then-fall ease (drops hold,
    // then slip - the real-glass cadence).
    float cyc = frac(tt * (0.10f + n.z * 0.07f) + n.x * 7.31f);
    float py = pow(cyc, 0.65f);
    float aniso = grid.x / grid.y;   // cell tallness in q-space
    float2 du = float2(f.x - 0.5f - x, (f.y - py) * aniso);
    du.y *= 1.0f / (1.0f + stretch * 0.9f);   // elongate along the fall (halved)
    float r0 = 0.085f + n.y * 0.06f;
    float len = length(du);
    float drop = smoothstep(r0, r0 * 0.55f, len);
    // Trail: the wet wipe above the drop, fading with distance behind, with
    // small satellite beads left on the track.
    float trackX = smoothstep(0.14f, 0.02f, abs(du.x));
    float behind = (py - f.y);
    float trail = trackX * step(0.0f, behind) * saturate(1.0f - behind * 1.15f)
                  * smoothstep(0.05f, 0.25f, py);
    float beads = trail * smoothstep(0.32f, 0.0f,
        abs(frac(f.y * (9.0f + n.z * 5.0f) + n.y * 4.0f) - 0.5f))
        * smoothstep(0.6f, 0.95f, Hash(id + floor(f.y * 9.0f)));
    float2 g = (len > 1e-4f) ? du / max(len, 1e-4f) : float2(0, 0);
    float curve = saturate(1.0f - len / max(r0, 1e-4f));
    float2 nrm = g * curve * drop + g * beads * 0.35f;
    return float4(nrm, saturate(drop + beads * 0.6f), saturate(trail + beads));
}

// Per-pixel world position from the depth buffer + inverse view-projection
float3 WorldPos(int2 px, float2 uv)
{
    float raw = LoadDepthPS(px);
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float4 ndc = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, ndcZ, 1.0f);
    float4 wp = mul(ndc, invViewProj);
    return wp.xyz / wp.w;
}

float KhEncFence()
{
    float khef_den = 1.0f - depthParams.x;
    float khef_far = khef_den < -1.0e-7f ? depthParams.y / khef_den : 20000.0f;
    return clamp(khef_far, 500.0f, 100000.0f);
}

float3 KhWorldPosFenced(int2 px, float2 uv, out float khwf_d)
{
    float khwf_raw = LinDepth(LoadDepthPS(px));
    khwf_d = min(khwf_raw, KhEncFence() * 0.999f);
    float4 khwf_nd = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f,
                            depthParams.x + depthParams.y / max(khwf_d, 1.0f), 1.0f);
    float4 khwf_wp = mul(khwf_nd, invViewProj);
    return khwf_wp.xyz / khwf_wp.w;
}

// Pure function of its arguments; reads no CB.
float3 KhgVpos(float2 vp_px, float vp_d, float2 vp_res, float vp_m00, float vp_m11)
{
    float2 vp_uv = (vp_px + 0.5f) / vp_res;
    return float3((vp_uv.x * 2.0f - 1.0f) * vp_d / vp_m00,
                  (1.0f - vp_uv.y * 2.0f) * vp_d / vp_m11,
                  vp_d);
}



// fxParams1 / fxParams2 are SYSTEM lanes for effect 23: the chain loop packs
// up to two aggregated fog-pass records [startDist, endDist, skyAmount,
// opacity] (kh_fogscatter_pack - the CPU mirror; zeroed lanes = no pass, and
// every other fill site leaves them zeroed, so the loop is cold there).

float KhFsFog(float2 fs_px, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = 0.0f;

    if (fogParams.w >= 0.5f && depthParams.y < -1.0e-3f)
    {
        // the encode pair's far fence (m32 gate above guarantees the
        // standard-z shape: 1 - m22 < 0, m32 < 0 -> positive fence)
        float fs_fden = 1.0f - depthParams.x;
        float fs_far = fs_fden < -1.0e-7f ? depthParams.y / fs_fden : 20000.0f;
        float fs_de = min(fs_d, clamp(fs_far, 500.0f, 100000.0f) * 0.999f);
        float fs_distM = length(KhgVpos(fs_px, fs_de, fs_res, fs_m00, fs_m11));
        // height at the clamped distance (WorldPos's own ndc form, fed the
        // ANALYTIC ndcZ of fs_de - immune to the beyond-far mirror and to
        // raw-depth teeter by construction)
        float2 fs_uv = (fs_px + 0.5f) / fs_res;
        float4 fs_nd = float4(fs_uv.x * 2.0f - 1.0f, 1.0f - fs_uv.y * 2.0f,
                              depthParams.x + depthParams.y / max(fs_de, 1.0f), 1.0f);
        float4 fs_wp = mul(fs_nd, invViewProj);
        float fs_hgt = fs_wp.y / fs_wp.w;
        float fs_camY = fogColor.w;
        float fs_tr;

        if (fogEngine.w >= 0.5f)
        {
            // TWIN PARITY. w = 2 is set ONLY under mode 61, so outside that
            // mode this line is byte-identical to and needs no revert of its
            // own.
            float fs_ramp = fogEngine.w >= 1.5f
                          ? 1.0f
                          : saturate((fogEngine.y - fs_distM) * fogEngine.z);
            float fs_dh = abs(fs_hgt - fs_camY);
            float fs_k = fogParams.y * fs_dh / max(fs_distM, 1.0e-4f);
            float fs_integ = fs_k < 1.0e-6f ? fs_distM : (1.0f - exp(-fs_distM * fs_k)) / fs_k;
            float fs_minY = min(fs_hgt, fs_camY);
            fs_tr = fs_ramp * exp(-fs_integ * fogEngine.x * exp(-fogParams.y * max(fs_minY, 0.0f)));
        }
        else
        {
            float fs_dens = fogParams.x * exp(-fogParams.y * max(fs_hgt - fogParams.z, 0.0f));
            fs_tr = exp(-fs_distM * fs_dens * 0.0153f);
        }

        fs_s = 1.0f - saturate(fs_tr);
    }

    // KH fog passes (effect-13 MATH TWIN - the same LINEAR-depth ramp and the
    // same skyAmount rule; edit both or neither), opacity- scaled, combined
    // as independent media.
    float fs_fence = KhEncFence();

    [unroll] for (int fs_i = 0; fs_i < 2; ++fs_i)
    {
        float4 fs_e = fs_i == 0 ? fxParams1 : fxParams2;
        if (fs_e.w > 0.001f)
        {
            float fs_f = saturate((fs_d - fs_e.x) / max(fs_e.y - fs_e.x, 1.0f));
            float fs_w = saturate((min(fs_d, fs_fence) - fs_fence * 0.98f)
                                  / max(fs_fence * 0.019f, 1.0f));
            fs_f = lerp(fs_f, saturate(fs_e.z), fs_w);
            if (fs_d > 1e8f) fs_f = saturate(fs_e.z);   // belt (the feather already lands here)
            fs_s = 1.0f - (1.0f - fs_s) * (1.0f - fs_f * fs_e.w);
        }
    }

    return saturate(fs_s);
}

// CPU twin: the chain loops' pending append (kh_fuse_append; the fusible set
// {1 invert, 2 colorgrade, 3 vignette, 5 grain}, never localized / banded /
// spill / LUT / custom, is enforced THERE, so this path carries no masks).
// POINT-OP PASS FUSION.
float3 KhFusePoint(int id, float3 c, float2 uv, float2 pos, float t,
                   float4 p0, float4 p1, float4 col)
{
    if (id == 1) return (1.0f - saturate(c)) * col.rgb;
    if (id == 2)
    {
        float3 g = c * col.rgb * p0.z;
        float l = Luma(g);
        g = lerp(l.xxx, g, p0.x);
        g = (g - 0.5f) * p0.y + 0.5f;
        return pow(max(g, 0.0f), p0.w);
    }
    if (id == 3)
    {
        float d = distance(uv, float2(0.5f, 0.5f)) * 1.4142f;
        float v = smoothstep(p0.x, p0.x + max(p0.y, 1e-3f), d);
        return lerp(c, col.rgb, v);
    }
    if (id == 5)
    {
        float fps = max(p0.y, 1.0f);
        float seed = floor(t * fps) * 61.7f;
        float2 gp = pos / max(p0.z, 1.0f);
        float2 ip = floor(gp);
        float2 fp = frac(gp);
        fp = fp * fp * (3.0f - 2.0f * fp);
        float n00 = Hash(ip + seed);
        float n10 = Hash(ip + float2(1, 0) + seed);
        float n01 = Hash(ip + float2(0, 1) + seed);
        float n11 = Hash(ip + float2(1, 1) + seed);
        float nv = lerp(lerp(n00, n10, fp.x), lerp(n01, n11, fp.x), fp.y);
        float nf = Hash(gp * 2.13f + seed + 17.0f);
        float g = (nv + nf) * 0.5f - 0.5f;
        float3 gc = g.xxx;
        if (p1.x > 0.001f)
        {
            float gr = (lerp(Hash(ip + seed + 31.0f), Hash(ip + float2(1, 1) + seed + 31.0f), fp.x) + Hash(gp * 1.71f + seed + 47.0f)) * 0.5f - 0.5f;
            float gb = (lerp(Hash(ip + seed + 73.0f), Hash(ip + float2(1, 1) + seed + 73.0f), fp.x) + Hash(gp * 2.71f + seed + 89.0f)) * 0.5f - 0.5f;
            gc = lerp(gc, float3(gr, g, gb), p1.x);
        }
        float luma = saturate(Luma(c));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, p0.w);
        return c + gc * p0.x * resp;
    }
    return c;
}

// Fused-stage composite: the packing tail's blend algebra VERBATIM over the
// running value, plus - write-window lanes only - the coverage destination
// lerp in its exact pre-composite position (the masked lane's order; spill
// stage-0 completes its own algebra BEFORE this runs, and appended stages are
// never spill-classified).
float3 KhFuseTail(float3 v, float cov, bool uiLane, float2 uv, float2 pos, float t)
{
    int n = (int)fuseMeta.x;
    [loop] for (int s = 0; s < n; ++s)
    {
        float4 fm = fuseStage[s * 4];
        float4 fcol = fuseStage[s * 4 + 3];
        float3 c = KhFusePoint((int)fm.x, v, uv, pos, t,
                               fuseStage[s * 4 + 1], fuseStage[s * 4 + 2], fcol);
        if (uiLane) c = lerp(v, c, cov);
        float a = fcol.w;
        int bm = (int)fm.y;
        float3 mixed = lerp(v, c, a);
        if (bm == 1)      v = v + c * a;
        else if (bm == 2) v = v * lerp(float3(1.0f, 1.0f, 1.0f), c, a);
        else if (bm == 3) v = v + c * a - v * c * a;
        else if (bm == 4) v = max(v, mixed);
        else if (bm == 5) v = min(v, mixed);
        else              v = mixed;
    }
    return v;
}
 float4 PSEffect(VSOut i) : SV_Target
{
    // Fullscreen passes are inert by construction (w = 1 -> ndc far below 1),
    // and zeroed/degenerate depthParams stand the test down via the m32 gate.
    if (shadowMeta2.x < 0.5f && depthParams.y < -1.0e-3f &&
        depthParams.x + depthParams.y / max(i.pos.w, 1.0e-4f) > 1.0f) discard;
    if (shadowMeta2.x < 0.5f && shadowMeta2.y > 0.0f && i.pos.w > shadowMeta2.y) discard;
    int2  px = int2(i.pos.xy);
    float2 uv = i.pos.xy / float2(fxMeta.z, fxMeta.w);
    int   effect = (int)fxMeta.x;
    float t = fxMeta.y;
    float3 scene = SampleScene(px);
    float3 outc = scene;

    if (centerSize.w > 1.5f) {
        float khpMin = 1.0f;
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.18f, fxMeta.w * 0.21f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.47f, fxMeta.w * 0.16f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.79f, fxMeta.w * 0.24f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.23f, fxMeta.w * 0.52f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.68f, fxMeta.w * 0.47f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.31f, fxMeta.w * 0.77f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.58f, fxMeta.w * 0.84f)));
        khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.86f, fxMeta.w * 0.69f)));
        if (khpMin >= 0.75f) {
            float4 khpRaw = sceneColor.Load(int3(px, 0));
            return float4(khpRaw.rgb, khpRaw.a);
        }
    }

    // It never compiled: the effect shader builds LAZILY and no effect mesh
    // had spawned since the arb round landed, so the X3004 surfaced only when
    // the perceptual path first requested the chain. FAR-FRAME ANALYTIC
    // ARBITRATION (flush/effect edition; the sec 2.4 mirror).
    if (localParams1.z >= 0.5f) {
        int2 khaPx = clamp(int2(i.pos.xy), int2(0, 0),
                           int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        float khaRaw = khArbSnap.Load(int3(khaPx, 0));

        if (khaRaw > 0.000001f && khaRaw < 0.999999f) {
            float khaNdc = (khaRaw - localParams0.z) / max(localParams0.w - localParams0.z, 1e-6f);
            float khaDen = khaNdc - localParams0.x;
            float khaScene = (khaDen > -1e-7f) ? 1.0e9f : localParams0.y / khaDen;
            if (khaScene <= 0.0f) khaScene = 1.0e9f;
            if (i.pos.w > khaScene * (1.0f + localParams1.w) + localParams1.z) discard;
        }

        if (thmParams.w >= 0.5f) {
            float khaHe = KhThmHeight(i.wpos.xz);
            if (khaHe > -1.0e5f && (i.wpos.y - khaHe) < -thmMeta.z) discard;
        }
    }

    if (effect == 1)   // Invert: (1 - scene) * tint
    {
        outc = (1.0f - saturate(scene)) * color.rgb;
    }
    else if (effect == 2)   // ColorGrade: [saturation, contrast, brightness, gamma], tint = color.rgb
    {
        float3 c = scene * color.rgb * fxParams0.z;
        float l = Luma(c);
        c = lerp(l.xxx, c, fxParams0.x);
        c = (c - 0.5f) * fxParams0.y + 0.5f;
        c = pow(max(c, 0.0f), fxParams0.w);
        outc = c;
    }
    else if (effect == 3)   // Vignette: [startRadius, softness], color = edge color
    {
        float d = distance(uv, float2(0.5f, 0.5f)) * 1.4142f;
        float v = smoothstep(fxParams0.x, fxParams0.x + max(fxParams0.y, 1e-3f), d);
        outc = lerp(scene, color.rgb, v);
    }
    else if (effect == 4)   // Chromatic aberration: [strengthPx], radial
    {
        float2 off = (uv - 0.5f) * fxParams0.x;
        float r = SampleScene(int2(i.pos.xy + off)).r;
        float b = SampleScene(int2(i.pos.xy - off)).b;
        outc = float3(r, scene.g, b);
    }
    else if (effect == 5)   // Film grain: [amount, fps, grainSizePx, lumaResponse, chroma]
    {
        // Filmic grain: smooth value noise with spatial extent (not per-pixel
        // salt), triangular amplitude distribution, response peaking in the
        // mid-shadows and protecting highlights, optional chroma, and time
        // quantized to a frame rate for the discrete film-frame feel.
        float fps = max(fxParams0.y, 1.0f);
        float seed = floor(t * fps) * 61.7f;
        float2 gp = i.pos.xy / max(fxParams0.z, 1.0f);
        float2 ip = floor(gp);
        float2 fp = frac(gp);
        fp = fp * fp * (3.0f - 2.0f * fp);

        float n00 = Hash(ip + seed);
        float n10 = Hash(ip + float2(1, 0) + seed);
        float n01 = Hash(ip + float2(0, 1) + seed);
        float n11 = Hash(ip + float2(1, 1) + seed);
        float nv = lerp(lerp(n00, n10, fp.x), lerp(n01, n11, fp.x), fp.y);
        float nf = Hash(gp * 2.13f + seed + 17.0f);
        float g = (nv + nf) * 0.5f - 0.5f;   // triangular-ish, signed

        float3 gc = g.xxx;
        if (fxParams1.x > 0.001f)
        {
            float gr = (lerp(Hash(ip + seed + 31.0f), Hash(ip + float2(1, 1) + seed + 31.0f), fp.x) + Hash(gp * 1.71f + seed + 47.0f)) * 0.5f - 0.5f;
            float gb = (lerp(Hash(ip + seed + 73.0f), Hash(ip + float2(1, 1) + seed + 73.0f), fp.x) + Hash(gp * 2.71f + seed + 89.0f)) * 0.5f - 0.5f;
            gc = lerp(gc, float3(gr, g, gb), fxParams1.x);
        }

        float luma = saturate(Luma(scene));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, fxParams0.w);
        outc = scene + gc * fxParams0.x * resp;
    }
    else if (effect == 6)   // Sharpen: [strength]
    {
        float3 s = scene * (1.0f + 4.0f * fxParams0.x)
                 - fxParams0.x * (SampleScene(px + int2(1, 0)) + SampleScene(px - int2(1, 0))
                                + SampleScene(px + int2(0, 1)) + SampleScene(px - int2(0, 1)));
        outc = max(s, 0.0f);
    }
    else if (effect == 7)   // Gaussian-ish blur: [radiusPx]
    {
        int r = max((int)fxParams0.x, 1);
        float3 acc = scene * 0.25f;
        acc += (SampleScene(px + int2(r, 0)) + SampleScene(px - int2(r, 0))
              + SampleScene(px + int2(0, r)) + SampleScene(px - int2(0, r))) * 0.125f;
        acc += (SampleScene(px + int2(r, r)) + SampleScene(px - int2(r, r))
              + SampleScene(px + int2(r, -r)) + SampleScene(px + int2(-r, r))) * 0.0625f;
        outc = acc;
    }
    else if (effect == 8)   // Bloom: [threshold, intensity, radiusPx]
    {
        int r = max((int)fxParams0.z, 1);
        float3 acc = 0.0f;
        [unroll] for (int oy = -2; oy <= 2; ++oy)
        [unroll] for (int ox = -2; ox <= 2; ++ox)
            acc += max(SampleScene(px + int2(ox, oy) * r) - fxParams0.x, 0.0f);
        outc = scene + acc / 25.0f * fxParams0.y;
    }
    else if (effect == 9)   // Distortion: [amplitudePx, frequency, speed]
    {
        float2 off = float2(sin(uv.y * fxParams0.y * 6.2832f + t * fxParams0.z),
                            cos(uv.x * fxParams0.y * 6.2832f + t * fxParams0.z)) * fxParams0.x;
        outc = SampleScene(int2(i.pos.xy + off));
    }

        else if (effect == 10)   // Outline: [depthEdgeScale, lumEdgeScale, sceneDarken, glowBoost], color = edge
    {
        float dC = LinDepth(LoadDepthPS(px));
        float dX = LinDepth(LoadDepthPS(px + int2(1, 0))) - dC;
        float dY = LinDepth(LoadDepthPS(px + int2(0, 1))) - dC;
        float depthEdge = saturate((abs(dX) + abs(dY)) / max(dC, 1.0f) * fxParams0.x);
        float lC = Luma(scene);
        float lumEdge = saturate((abs(Luma(SampleScene(px + int2(1, 0))) - lC)
                                + abs(Luma(SampleScene(px + int2(0, 1))) - lC)) * fxParams0.y);
        float edge = saturate(depthEdge + lumEdge);
        outc = scene * fxParams0.z + color.rgb * edge * fxParams0.w;
    }
    else if (effect == 11)   // Pulse: p0.xyz = center (engine space), p0.w = radius; p1 = [bandWidth, intensity]
    {
        // Fenced reconstruction + the fence feather keep the ring CONTINUOUS:
        // sky (clamped TO the fence) lands at feather zero, and a ring band
        // never reaches fence distances in normal use, so visible output is
        // unchanged.
        float khpl_d;
        float dist = distance(KhWorldPosFenced(px, uv, khpl_d), fxParams0.xyz);
        float band = max(fxParams1.x, 0.01f);
        float ring = 1.0f - saturate(abs(dist - fxParams0.w) / band);
        ring *= ring;
        float khpl_f = KhEncFence();
        ring *= 1.0f - saturate((khpl_d - khpl_f * 0.98f) / max(khpl_f * 0.019f, 1.0f));
        outc = scene + color.rgb * ring * fxParams1.y;
    }
    else if (effect == 12)   // Halation: [threshold, intensity, radiusPx], color = glow tint (warm)
    {
        // full-res gather everywhere (pyramid family retired).
        int r = max((int)fxParams0.z, 1);
        const int2 dirs[8] = { int2(1,0), int2(-1,0), int2(0,1), int2(0,-1),
                               int2(1,1), int2(-1,1), int2(1,-1), int2(-1,-1) };
        float3 acc = 0.0f;
        [unroll] for (int k = 0; k < 8; ++k)
            acc += max(SampleScene(px + dirs[k] * r) - fxParams0.x, 0.0f) * 0.09f;
        [unroll] for (int k2 = 0; k2 < 8; ++k2)
            acc += max(SampleScene(px + dirs[k2] * r * 2) - fxParams0.x, 0.0f) * 0.035f;
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 13)   // Distance fog: [startDist m, endDist m, skyAmount 0..1], color = fog color
    {
        // skyAmount = 1 with endDist inside the fence - every practical
        // config - is output-identical. TWIN EDIT at KhFsFog's fog-pass loop.
        float d = LinDepth(LoadDepthPS(px));
        float f = saturate((d - fxParams0.x) / max(fxParams0.y - fxParams0.x, 1.0f));
        float khfg_f = KhEncFence();
        float khfg_w = saturate((min(d, khfg_f) - khfg_f * 0.98f) / max(khfg_f * 0.019f, 1.0f));
        f = lerp(f, saturate(fxParams0.z), khfg_w);
        if (d > 1e8f) f = saturate(fxParams0.z);   // sky/far-plane pixels (belt; the feather already lands here)
        outc = lerp(scene, color.rgb, f);
    }
    else if (effect == 14)   // Lens flare, image-based: [threshold, intensity, ghostCount, ghostSpacing] + [haloRadius, haloIntensity,
    {
        // Bright pixels anywhere in the capture spawn a "ghost" train
        // mirrored through screen center, plus a halo ring - the sun,
        // headlights and explosions all flare automatically. The HDR capture
        // makes the threshold physically meaningful (sun >> white walls).
        float2 cuv = float2(0.5f, 0.5f);
        float2 ghostVec = (cuv - uv) * fxParams0.w;
        int nGhosts = clamp((int)fxParams0.z, 1, 8);
        float3 acc = 0.0f;
        for (int g = 1; g <= nGhosts; ++g)
        {
            float2 suv = uv + ghostVec * (float)g;
            float w = 1.0f - saturate(length(suv - cuv) * 1.6f);
            w = w * w;
            float2 spf = saturate(suv) * float2(fxMeta.z, fxMeta.w);
            float2 cdir = normalize(ghostVec + 1e-5f) * fxParams1.z;
            float3 s;
            s.r = max(SampleScene(int2(spf + cdir)).r - fxParams0.x, 0.0f);
            s.g = max(SampleScene(int2(spf)).g        - fxParams0.x, 0.0f);
            s.b = max(SampleScene(int2(spf - cdir)).b - fxParams0.x, 0.0f);
            acc += s * w;
        }
        float rC = length(uv - cuv);
        float hw = 1.0f - saturate(abs(rC - fxParams1.x) * 8.0f);
        float2 huv = uv + normalize(cuv - uv + 1e-5f) * fxParams1.x;
        acc += max(SampleScene(int2(saturate(huv) * float2(fxMeta.z, fxMeta.w))) - fxParams0.x, 0.0f) * hw * fxParams1.y;
        outc = scene + acc * color.rgb * fxParams0.y;
    }

        else if (effect == 15)   // Anamorphic streak: [threshold, intensity, lengthPx, falloffPow] + [vertical 0/1]; color = tint
    {
        float3 acc = 0.0f;
        float total = 0.0f;

        // full-res gather everywhere (pyramid family retired).
        [unroll] for (int k = 1; k <= 16; ++k)
        {
            float t = (float)k / 16.0f;
            float w = pow(1.0f - t, max(fxParams0.w, 0.1f));
            int off = (int)(t * fxParams0.z);
            int2 d = (fxParams1.x > 0.5f) ? int2(0, off) : int2(off, 0);
            acc += (max(SampleScene(px + d) - fxParams0.x, 0.0f)
                  + max(SampleScene(px - d) - fxParams0.x, 0.0f)) * w;
            total += 2.0f * w;
        }

        acc /= max(total, 1.0f);
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 16)   // Sun flare, source-aware: p0.xyz = direction (engine space), p0.w = size;
    {
        // The direction projects as a point at infinity (w component 0); the
        // flare fades via per-pixel depth occlusion at the source: sky =
        // visible, geometry = blocked, partial coverage = smooth fade.
        float4 clip = mul(float4(fxParams0.xyz, 0.0f), viewProj);
        if (clip.w > 0.01f)
        {
            float2 sndc = clip.xy / clip.w;
            if (all(abs(sndc) < 1.3f))
            {
                float2 spos = float2(sndc.x * 0.5f + 0.5f, 0.5f - sndc.y * 0.5f);
                int2 sp = int2(saturate(spos) * float2(fxMeta.z, fxMeta.w));
                float vis = 0.0f;
                // VERDICT-CONTINUITY (the doctrine WATCH item, closed): each
                // tap's sky verdict was a hard > 1e8 cut - horizon terrain
                // teetering at the far sentinel flipped taps in 1/25 steps,
                // several together under the churn.
                float khsf_f = KhEncFence();
                [unroll] for (int oy = -2; oy <= 2; ++oy)
                [unroll] for (int ox = -2; ox <= 2; ++ox)
                {
                    float khsf_d = LinDepth(LoadDepthPS(sp + int2(ox, oy) * 3));
                    vis += saturate((min(khsf_d, khsf_f) - khsf_f * 0.98f)
                                    / max(khsf_f * 0.019f, 1.0f));
                }
                vis /= 25.0f;
                if (vis > 0.001f)
                {
                    float aspect = fxMeta.z / fxMeta.w;
                    float2 d = uv - spos;
                    d.x *= aspect;
                    float r = length(d) / max(fxParams0.w, 0.01f);
                    float ang = atan2(d.y, d.x);
                    float star = pow(abs(sin(ang * 6.0f)), 8.0f) * fxParams1.z;
                    float core = exp(-r * 18.0f) * 2.0f + exp(-r * 3.0f) * (0.35f + star * saturate(1.0f - r));
                    float2 axis = float2((0.5f - spos.x) * aspect, 0.5f - spos.y);
                    float glow = 0.0f;
                    [unroll] for (int g = 1; g <= 4; ++g)
                    {
                        if ((float)g <= fxParams1.x)
                        {
                            float2 gp = d - axis * (0.5f * (float)g);
                            glow += exp(-length(gp) * (30.0f + (float)g * 14.0f)) * (0.5f / (float)g);
                        }
                    }
                    glow += exp(-abs(r - 1.0f) * 9.0f) * fxParams1.y * 0.4f;
                    outc = scene + color.rgb * (core + glow) * vis;
                }
            }
        }
    }

        else if (effect == 17)   // Glitch: [intensity, speed, sliceAmountPx, sliceBands] + [colorSplitPx, blockAmount, noiseAmount, burstiness]
    {
        float speed = max(fxParams0.y, 0.1f);
        float tf = floor(t * speed);

        float burst = Hash(float2(tf * 0.0131f, 7.31f));
        burst = pow(burst, 1.0f + fxParams1.w * 6.0f);
        float drive = lerp(1.0f, burst, saturate(fxParams1.w)) * fxParams0.x;

        float2 suv = i.pos.xy;

        // Horizontal slice displacement: few large tears, many small ones
        float bands = max(fxParams0.w, 1.0f);
        float band = floor(uv.y * bands);
        float bh = (Hash(float2(band, tf)) - 0.5f) * 2.0f;
        bh = sign(bh) * pow(abs(bh), 3.0f);
        suv.x += bh * fxParams0.z * drive;

        // Block corruption: coarse grid cells randomly displaced
        if (fxParams1.y > 0.001f)
        {
            float2 cell = floor(uv * float2(24.0f, 14.0f));
            float ch = Hash(cell + tf * 1.7f);
            if (ch > 1.0f - 0.15f * saturate(fxParams1.y) * saturate(drive))
            {
                float2 off = (float2(Hash(cell + 3.1f + tf), Hash(cell + 5.7f + tf)) - 0.5f) * 80.0f * drive;
                suv += off;
            }
        }

        // RGB channel split along the tear axis
        float split = fxParams1.x * drive;
        float3 col;
        col.r = SampleScene(int2(suv + float2(split, 0.0f))).r;
        col.g = SampleScene(int2(suv)).g;
        col.b = SampleScene(int2(suv - float2(split, 0.0f))).b;

        // Interference lines + static
        if (fxParams1.z > 0.001f)
        {
            float ln = Hash(float2(floor(i.pos.y * 0.5f), tf * 2.3f));
            float lineHit = step(1.0f - 0.2f * saturate(fxParams1.z) * saturate(drive), ln);
            float n = Hash(i.pos.xy * 0.37f + tf * 13.7f);
            col = lerp(col, color.rgb * n, lineHit * 0.85f);
            col += (n - 0.5f) * fxParams1.z * drive * 0.35f;
        }

        outc = max(col, 0.0f);
    }
     else if (effect == 18)   // clarity: [strength, radiusPx]
    {
        // Wide-radius local contrast on LUMA ONLY (chroma-preserving by
        // construction: the scene is rescaled by L'/L, so saturation never
        // moves). Two-ring 12-tap base estimate; the detail term is
        // soft-limited so strong edges do not halo.
        float rad = max(fxParams0.y, 4.0f);
        float lC = Luma(scene);
        float lB = 0.0f;

        [loop] for (int k = 0; k < 8; ++k)
        {
            float ang = float(k) * 0.7854f + 0.3927f;
            lB += Luma(SampleScene(px + int2(cos(ang) * rad, sin(ang) * rad))) * 0.0833f;
        }

        [loop] for (int k2 = 0; k2 < 4; ++k2)
        {
            float ang2 = float(k2) * 1.5708f;
            lB += Luma(SampleScene(px + int2(cos(ang2) * rad * 0.5f, sin(ang2) * rad * 0.5f))) * 0.0833f;
        }

        float detail = lC - lB;
        detail = detail / (1.0f + 2.5f * abs(detail));   // soft limiter (anti-halo)
        float scale = (lC + fxParams0.x * detail * 2.2f) / max(lC, 1e-3f);
        outc = scene * clamp(scale, 0.3f, 3.0f);
    }
    else if (effect == 19)   // deband: [threshold(1/255), rangePx, grain(1/255)]
    {
        // Gradient debanding (the mpv/flyguy family): average four taps on a
        // hash-rotated cross within range; adopt the average only where it
        // stays inside the threshold (real edges never do). Triangular-PDF
        // grain breaks residual quantization. STATIC noise (temporally inert;
        // no TAA exists to launder animation).
        float2 h = Hash2(i.pos.xy);
        float ang = h.x * 6.2832f;
        float rad = (0.3f + 0.7f * h.y) * max(fxParams0.y, 2.0f);
        float2 dir = float2(cos(ang), sin(ang)) * rad;
        float3 avg = (SampleScene(px + int2(dir))
                    + SampleScene(px - int2(dir))
                    + SampleScene(px + int2(-dir.y, dir.x))
                    + SampleScene(px - int2(-dir.y, dir.x))) * 0.25f;
        float3 diff = avg - scene;
        float th = max(fxParams0.x, 0.1f) / 255.0f;
        float within = step(max(abs(diff.r), max(abs(diff.g), abs(diff.b))), th);
        outc = lerp(scene, avg, within);
        float tri = (Hash(i.pos.xy + 0.17f) + Hash(i.pos.xy + 0.53f) - 1.0f);
        outc += tri * (fxParams0.z / 255.0f);
        outc = max(outc, 0.0f);
    }
    else if (effect == 20)   // rainlens: [intensity, speed, condensation, refract]; fx1 = SYSTEM camera velocity
    {
        float aspect = fxMeta.z / max(fxMeta.w, 1.0f);
        float2 q = float2(uv.x * aspect, uv.y);
        float inten = saturate(fxParams0.x);
        float fwdK = saturate(max(fxParams1.z, 0.0f) / 20.0f);
        float tt = fxParams1.w * max(fxParams0.y, 0.0f);
        float windBase = clamp(-fxParams1.x * 0.055f, -0.5f, 0.5f);
        float wind = windBase + (q.x / aspect - 0.5f) * fwdK * 0.6f;
        // small-splatter retune: finer grids, smaller drops, slower cadence,
        // gentler refraction (constants in KhRainLayer and below) - the
        // requested "small splatters, slight motion".
        float4 L1 = KhRainLayer(q, tt,          float2(6.0f, 1.4f), 0.0f,
                                0.45f + inten * 0.4f, wind, fwdK);
        float4 L2 = KhRainLayer(q, tt * 1.27f,  float2(10.0f, 2.4f), 3.7f,
                                0.3f + inten * 0.45f, wind * 1.15f, fwdK);
        float drop  = max(L1.z, L2.z * 0.85f);
        float trail = max(L1.w, L2.w * 0.8f);
        float2 nrm  = L1.xy + L2.xy * 0.7f;
        // Condensation film: blurred, faintly lifted scene; wiped by trails
        // and punched through by the drops themselves.
        float fogAmt = saturate(fxParams0.z) * saturate(0.25f + inten);
        float3 fogC = 0.0f;

        [loop] for (int k = 0; k < 8; ++k)
        {
            float ang = float(k) * 0.7854f;
            fogC += SampleScene(px + int2(cos(ang) * 7.0f, sin(ang) * 7.0f)) * 0.125f;
        }

        fogC = lerp(fogC, Luma(fogC).xxx, 0.12f) * 1.02f;
        // Fine static condensation droplets (twinkle in with the film,
        // cleared where trails wiped).
        float2 sid = floor(q * 34.0f);
        float sn = Hash(sid);
        float2 sf = frac(q * 34.0f) - 0.5f;
        float slife = saturate(sin(t * 0.35f + sn * 6.2832f) * 0.5f + 0.5f);
        float sdrop = smoothstep(0.12f + sn * 0.1f, 0.05f, length(sf))
                    * step(sn, fogAmt * 0.8f) * slife * (1.0f - saturate(trail * 1.6f));
        nrm += (sf / 0.2f) * sdrop * 0.18f;
        drop = max(drop, sdrop * 0.5f);
        float wiped = saturate(trail * 1.5f + drop);
        float3 base = lerp(scene, fogC, fogAmt * (1.0f - wiped) * 0.85f);
        // Refraction through the drops: inverted-wide sample + chroma split.
        float refr = max(fxParams0.w, 0.0f);
        float2 offPx = -nrm * refr * (20.0f + fwdK * 8.0f);
        int2 rp = px + int2(offPx);
        float3 through;
        through.g = SampleScene(rp).g;
        through.r = SampleScene(px + int2(offPx * 1.07f)).r;
        through.b = SampleScene(px + int2(offPx * 0.93f)).b;
        float3 col = lerp(base, through, saturate(drop * 1.4f));
        // Specular bead: light from the upper hemisphere of the drop.
        col += drop * pow(saturate(-nrm.y + 0.15f), 6.0f) * 0.25f;
        // Trails read slightly darker-sharp (wet glass transmits more).
        col = lerp(col, scene * 0.985f, saturate(trail * (1.0f - drop) * 0.55f) * fogAmt);
        outc = col;
    }
    else if (effect == 21)   // crt: [curvature, scanlines, lineCount, maskStrength] + [aberrationPx, flicker, rollingBand, cornerRadius] +
    {
        float khc_curv = max(fxParams0.x, 0.0f);
        float2 khc_cc = uv - 0.5f;
        float khc_r2 = dot(khc_cc, khc_cc);
        float2 khc_duv = 0.5f + khc_cc * (1.0f + khc_curv * khc_r2 * (1.0f + 0.8f * khc_r2));

        float khc_lines = max(fxParams0.z, 16.0f);
        float khc_phase = khc_duv.y * khc_lines + t * fxParams2.x;
        float khc_line = floor(khc_phase);

        // TRACKING WOBBLE (fxParams2.y, px): each scanline row shifts
        // horizontally by a per-line random jitter requantized at 24 Hz (the
        // loose-sync read) plus a slow per-line sway. ONLY the picture
        // wobbles: the tube mask and the grille live on the glass and stay
        // put by construction.
        float khc_wob = ((Hash(float2(khc_line * 0.173f, floor(t * 24.0f) * 0.71f)) - 0.5f)
                       + 0.35f * sin(t * 2.3f + khc_line * 0.61f)) * fxParams2.y;

        float2 khc_dpx = khc_duv * float2(fxMeta.z, fxMeta.w);
        khc_dpx.x += khc_wob;
        float2 khc_fpx = khc_cc * khc_r2 * 4.0f * fxParams1.x;
        float3 khc_col;
        khc_col.r = SampleScene(int2(khc_dpx + khc_fpx)).r;
        khc_col.g = SampleScene(int2(khc_dpx)).g;
        khc_col.b = SampleScene(int2(khc_dpx - khc_fpx)).b;

        // Beam scanlines over the distorted picture: the beam profile is a
        // shaped sine whose exponent NARROWS in the shadows and WIDENS toward
        // the highlights (phosphor blooming), and the 1.32 gain recovers the
        // average level the dark gaps remove. the profile rides the shared
        // raster phase.
        float khc_lum = saturate(Luma(khc_col));
        float khc_beam = pow(abs(sin(khc_phase * 3.14159265f)),
                             lerp(2.2f, 0.65f, khc_lum));
        khc_col *= lerp(1.0f, khc_beam * 1.32f, saturate(fxParams0.y));

        // Aperture grille: RGB triad fixed to SCREEN columns, with a matching
        // gain so full mask strength keeps the average level.
        float khc_mk = saturate(fxParams0.w);
        int khc_m = (int)fmod(i.pos.x, 3.0f);
        float3 khc_tri = khc_m == 0 ? float3(1.0f, 0.45f, 0.45f)
                       : khc_m == 1 ? float3(0.45f, 1.0f, 0.45f)
                                    : float3(0.45f, 0.45f, 1.0f);
        khc_col *= lerp(float3(1.0f, 1.0f, 1.0f), khc_tri * 1.35f, khc_mk);

        // Rolling sync band: a soft dark bar drifting down the frame and
        // wrapping (the vertical-hold-slipping read).
        float khc_bpos = frac(t * 0.11f);
        float khc_bd = abs(khc_duv.y - khc_bpos);
        khc_bd = min(khc_bd, 1.0f - khc_bd);
        float khc_band = 1.0f - smoothstep(0.0f, 0.16f, khc_bd);
        khc_col *= 1.0f - khc_band * khc_band * 0.22f * saturate(fxParams1.z);

        // Mains flicker: a fast beat (aliases against any real frame rate -
        // authentically so) plus a per-refresh random sparkle quantized to 60
        // Hz through the grain family's Hash.
        float khc_fl = saturate(fxParams1.y);
        float khc_hum = sin(t * 100.0f * 3.14159265f) * 0.5f + 0.5f;
        float khc_spark = Hash(float2(floor(t * 60.0f), 3.7f)) - 0.5f;
        khc_col *= 1.0f + khc_fl * (khc_hum * 0.04f + khc_spark * 0.05f);

        // Phosphor tint, then the tube face: rounded-rectangle mask in the
        // DISTORTED space (out-of-range refraction lands outside it by
        // construction - the bezel is black), with a ~2 px feathered edge and
        // a soft glass falloff into the corners.
        khc_col *= color.rgb;
        float khc_cr = clamp(fxParams1.w, 0.003f, 0.5f);
        float2 khc_q = abs(khc_duv - 0.5f);
        float2 khc_ex = max(khc_q - (0.5f - khc_cr), 0.0f);
        float khc_cd = length(khc_ex);
        float khc_feather = 2.0f / fxMeta.w;
        float khc_tube = 1.0f - smoothstep(khc_cr - khc_feather, khc_cr, khc_cd);
        khc_tube *= 1.0f - smoothstep(0.0f, khc_cr, khc_cd) * 0.35f;
        outc = khc_col * khc_tube;
    }

