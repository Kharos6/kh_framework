// effect.hlsl - concatenated after cb.hlsl into the effect unit by C++ (no
// #include). Any edit changes the unit's shader cache key.
// Its shared declarations and helpers are cb.hlsl's KH_FX_UNIT section (a user
// effect compiles against the same text); what is here is the builtins' own.

// KH_FX_SIDE - a value that is the same for a whole pass, or that every pixel would otherwise recompute for its
// neighbours, drawn once before the pass by this same shader into a small target of ours (the side ids 28 - 30
// return it) and read here. Each is armed by its own lane of matCtl, which no effect pass otherwise fills
// (kh_bind_material writes it for textured meshes only, and every effect pass's constants start from a zeroed
// template); unarmed - effect meshes, the PIP, and any route that did not draw it - the pass computes the value in
// place, as it always has. t4 / t5 are free in this unit (cb.hlsl's receive textures there are fenced out of it).
Texture2D<float> khFxSide  : register(t4);   // matCtl.y: fog scatter's per-pixel fog (28, full size) / the sun flare's visibility (29, 1 x 1).
Texture2D<float> khUiProbe : register(t5);   // matCtl.z: the UI lane's coverage probe (30, 1 x 1).

// Bound at t19 only for LUT passes - t19 is reserved for this unit
// codebase-wide (inside StateBackup's save range, so the engine's own bind is
// restored after every flush). Sampled with integer Loads only (tetrahedral
// interpolation; no sampler, no half-texel bias).
Texture3D<float4> khLut : register(t19);
float3 KhLutV(int3 p) { return khLut.Load(int4(p, 0)).rgb; }

// KH_CRT_MASK: the length of [0, u] that stripe k (0 R, 1 G, 2 B) of a triad p pixels wide covers - the
// running integral of the stripe's indicator, continuous across period edges, so a pixel's coverage of the
// stripe is the difference at its two edges (an exact box filter).
float KhCrtStripe(float u, float p, float k)
{
    const float w = p / 3.0f;
    const float n = floor(u / p);
    return n * w + clamp(u - n * p - k * w, 0.0f, w);
}

// KH_GLOW_PYR's builders, drawn by kh_glow_build with VSFullscreen over each level's viewport. The seed (level 0):
// each texel the tent over the frame's pixels 2p - 1 .. 2p + 2 (SampleScene clamps to the frame), each through the
// pass's threshold when localParams0.y arms it (localParams0.x). Negative and NaN values read as 0, and the sum
// stays inside half precision.
float4 PSGlowSeed(float4 pos : SV_Position) : SV_Target
{
    const int2 khgs_b = int2(pos.xy) * 2 - 1;
    const float khgs_k[4] = { 0.125f, 0.375f, 0.375f, 0.125f };
    float3 khgs_a = 0.0f;
    [unroll] for (int khgs_y = 0; khgs_y < 4; ++khgs_y)
    [unroll] for (int khgs_x = 0; khgs_x < 4; ++khgs_x)
    {
        float3 khgs_c = SampleScene(khgs_b + int2(khgs_x, khgs_y));
        if (localParams0.y > 0.5f) khgs_c -= localParams0.x;
        khgs_a += min(max(khgs_c, 0.0f), 65504.0f) * (khgs_k[khgs_x] * khgs_k[khgs_y]);
    }
    return float4(khgs_a, 1.0f);
}
// Each further level: the same tent over the level above (t3, that level's own view), clamped to its edge.
float4 PSGlowDown(float4 pos : SV_Position) : SV_Target
{
    uint khgd_w, khgd_h;
    khsgTex.GetDimensions(khgd_w, khgd_h);
    const int2 khgd_hi = int2((int)khgd_w, (int)khgd_h) - 1;
    const int2 khgd_b = int2(pos.xy) * 2 - 1;
    const float khgd_k[4] = { 0.125f, 0.375f, 0.375f, 0.125f };
    float3 khgd_a = 0.0f;
    [unroll] for (int khgd_y = 0; khgd_y < 4; ++khgd_y)
    [unroll] for (int khgd_x = 0; khgd_x < 4; ++khgd_x)
    {
        const int2 khgd_p = clamp(khgd_b + int2(khgd_x, khgd_y), int2(0, 0), khgd_hi);
        khgd_a += khsgTex.Load(int3(khgd_p, 0)).rgb * (khgd_k[khgd_x] * khgd_k[khgd_y]);
    }
    return float4(khgd_a, 1.0f);
}

// KH_ANA_PYR - anamorphic's pre-filtered picture: a pyramid halved ALONG THE STREAK'S AXIS ONLY, so every level keeps
// the frame's full resolution across it and the streak stays one pixel thin (an isotropic level thickens it; the
// anisotropic read of one costs about ten times its taps). C++ kh_ana_build makes it per pass, of that pass's own
// source, through its threshold: level k (0 .. KH_ANA_LEVELS - 1) has one texel per 2^(k+1) frame pixels along the
// axis and one per pixel across it - level 0 the frame through the glows' tent (1 3 3 1) along the axis
// (PSAnaSeed), each further level the same tent over the level below (PSAnaDown), clamped to that level's edge.
// The levels are strips of two atlases, the even ones at t3 (khsgTex) and the odd ones at t6 (khAnaB), so a level
// is always built from the other atlas and never read while it is a target; KhAnaLevel places them (C++ twins
// kh_ana_n / kh_ana_off). Armed as the glows are (KhGlowOn, fuseMeta.y); on an anamorphic pass fuseMeta.zw is the
// frame the pyramid was made for (C++ kh_ana_lanes), not the glows' padded extent. t6 is free in this unit, as
// t4 / t5 are (cb.hlsl's receive textures there are fenced out of it).
#define KH_ANA_LEVELS 8   // C++ twin KH_ANA_LEVELS: texels of 2 .. 256 frame pixels along the axis.
Texture2D<float4> khAnaB : register(t6);   // KH_ANA_PYR's odd levels (t3 holds the even ones).
// Level k of a frame khal_len pixels long along the axis: khal_n texels (the frame's pixels 2^(k+1) at a time, the
// last one partly past the edge), at along-offset khal_o in its atlas - after the levels of its own parity below it.
void KhAnaLevel(int khal_k, int khal_len, out int khal_n, out int khal_o)
{
    khal_n = (khal_len + (2 << khal_k) - 1) >> (khal_k + 1);
    khal_o = 0;
    [loop] for (int khal_j = khal_k & 1; khal_j < khal_k; khal_j += 2)
        khal_o += (khal_len + (2 << khal_j) - 1) >> (khal_j + 1);
}
// The builders, drawn by kh_ana_build with VSFullscreen over each level's strip (its viewport). localParams0: x = the
// threshold, z = the axis (1 = along y), w = the level. Level 0 (at offset 0 of the even atlas): each texel the tent
// over the frame's pixels 2i - 1 .. 2i + 2 along the axis, of its own row across (SampleScene clamps to the frame),
// each through the threshold - anamorphic is a bright pass. Negative and NaN values read as 0, and the sum stays
// inside half precision (PSGlowSeed's rule).
float4 PSAnaSeed(float4 pos : SV_Position) : SV_Target
{
    const bool khas_v = localParams0.z > 0.5f;
    const int2 khas_p = int2(pos.xy);
    const int khas_b = (khas_v ? khas_p.y : khas_p.x) * 2 - 1;
    const int khas_c = khas_v ? khas_p.x : khas_p.y;
    const float khas_k[4] = { 0.125f, 0.375f, 0.375f, 0.125f };
    float3 khas_a = 0.0f;
    [unroll] for (int khas_i = 0; khas_i < 4; ++khas_i)
    {
        const int khas_q = khas_b + khas_i;
        const float3 khas_s = SampleScene(khas_v ? int2(khas_c, khas_q) : int2(khas_q, khas_c)) - localParams0.x;
        khas_a += min(max(khas_s, 0.0f), 65504.0f) * khas_k[khas_i];
    }
    return float4(khas_a, 1.0f);
}
// Each further level: the same tent over the level below (t3, the other atlas), clamped to that level's own edge.
float4 PSAnaDown(float4 pos : SV_Position) : SV_Target
{
    const bool khad_v = localParams0.z > 0.5f;
    const int khad_k = (int)(localParams0.w + 0.5f);
    const int khad_len = (int)((khad_v ? fxMeta.w : fxMeta.z) + 0.5f);
    int khad_n, khad_o, khad_sn, khad_so;
    KhAnaLevel(khad_k, khad_len, khad_n, khad_o);
    KhAnaLevel(khad_k - 1, khad_len, khad_sn, khad_so);
    const int2 khad_p = int2(pos.xy);
    const int khad_b = ((khad_v ? khad_p.y : khad_p.x) - khad_o) * 2 - 1;
    const int khad_c = khad_v ? khad_p.x : khad_p.y;
    const float khad_w[4] = { 0.125f, 0.375f, 0.375f, 0.125f };
    float3 khad_a = 0.0f;
    [unroll] for (int khad_i = 0; khad_i < 4; ++khad_i)
    {
        const int khad_q = khad_so + clamp(khad_b + khad_i, 0, khad_sn - 1);
        khad_a += khsgTex.Load(int3(khad_v ? int2(khad_c, khad_q) : int2(khad_q, khad_c), 0)).rgb * khad_w[khad_i];
    }
    return float4(khad_a, 1.0f);
}
// The read: level k at a position khat_p along the axis (full-resolution pixels, centres at + 0.5) on row khat_c
// across, linear between the two texels around it (texel i's centre is at (i + 0.5) 2^(k+1)), clamped to the level's
// edge. Loads: across the axis the row is exactly the pixel's own, whatever a sampler would round.
float3 KhAnaTap(Texture2D<float4> khat_t, float khat_p, int khat_k, int khat_n, int khat_o, int khat_c, bool khat_v)
{
    const float khat_u = khat_p / (float)(2 << khat_k) - 0.5f;
    const float khat_f = floor(khat_u);
    const int khat_i0 = khat_o + clamp((int)khat_f, 0, khat_n - 1);
    const int khat_i1 = khat_o + clamp((int)khat_f + 1, 0, khat_n - 1);
    const float3 khat_a = khat_t.Load(int3(khat_v ? int2(khat_c, khat_i0) : int2(khat_i0, khat_c), 0)).rgb;
    const float3 khat_b = khat_t.Load(int3(khat_v ? int2(khat_c, khat_i1) : int2(khat_i1, khat_c), 0)).rgb;
    return lerp(khat_a, khat_b, khat_u - khat_f);
}
// Effect 15's 16 taps each side of khas_pc, khas_l / 16 apart, from level khas_k alone, weighted as its direct taps.
float3 KhAnaStreak(Texture2D<float4> khas_tx, int khas_k, int khas_len, float khas_pc, int khas_c, bool khas_v,
                   float khas_l)
{
    int khas_n, khas_o;
    KhAnaLevel(khas_k, khas_len, khas_n, khas_o);
    float3 khas_a = 0.0f;
    [unroll] for (int khas_j = 1; khas_j <= 16; ++khas_j)
    {
        const float khas_t = (float)khas_j / 16.0f;
        const float khas_w = pow(1.0f - khas_t, max(fxParams0.w, 0.1f));
        const float khas_d = khas_t * khas_l;
        khas_a += (KhAnaTap(khas_tx, khas_pc + khas_d, khas_k, khas_n, khas_o, khas_c, khas_v)
                 + KhAnaTap(khas_tx, khas_pc - khas_d, khas_k, khas_n, khas_o, khas_c, khas_v)) * khas_w;
    }
    return khas_a;
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
    x += (0.42f - abs(x)) * KhSin(tt * 0.7f + n.z * 6.2832f)   // KH_HASH: tt grows with time.
         * 0.4f * (1.0f - saturate(stretch * 1.4f));
    // Drop y: a descending cycle with a linger-then-fall ease (drops hold, then
    // slip - the real-glass cadence).
    float cyc = frac(tt * (0.10f + n.z * 0.07f) + n.x * 7.31f);
    float py = pow(cyc, 0.65f);
    float aniso = grid.x / grid.y;   // Cell tallness in q-space.
    float2 du = float2(f.x - 0.5f - x, (f.y - py) * aniso);
    du.y *= 1.0f / (1.0f + stretch * 0.9f);   // Elongate along the fall (halved).
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

// fxParams1 / fxParams2 are system lanes for effect 23: the chain loop packs up
// to two aggregated fog-pass records [startDist, endDist, skyAmount, opacity]
// (kh_fogscatter_pack); zeroed lanes = no pass.

float KhFsFog(float2 fs_px, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = KhFxFogEngine(fs_px, fs_d, fs_res, fs_m00, fs_m11);   // KH_FX_USER: the engine term, one body.

    // KH fog passes (effect-13 math twin - the same linear-depth ramp and
    // skyAmount rule; edit both or neither), opacity-scaled, combined as
    // independent media.
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
            if (fs_d > 1e8f) fs_f = saturate(fs_e.z);   // Belt (the feather already lands here).
            fs_s = 1.0f - (1.0f - fs_s) * (1.0f - fs_f * fs_e.w);
        }
    }

    return saturate(fs_s);
}

// Sun flare's source (effect 16, side id 29): the direction projects as a point at infinity (w = 0). False = the
// source is behind the camera or well off screen; spos its uv, sp its pixel.
bool KhSunFlareSpot(out float2 spos, out int2 sp)
{
    spos = float2(0.0f, 0.0f);
    sp = int2(0, 0);
    const float4 clip = mul(float4(fxParams0.xyz, 0.0f), viewProj);
    if (!(clip.w > 0.01f)) return false;
    const float2 sndc = clip.xy / clip.w;
    if (!all(abs(sndc) < 1.3f)) return false;
    spos = float2(sndc.x * 0.5f + 0.5f, 0.5f - sndc.y * 0.5f);
    sp = int2(saturate(spos) * float2(fxMeta.z, fxMeta.w));
    return true;
}
// Its visibility: the flare fades via depth occlusion at the source - sky = visible, geometry = blocked - over 25
// taps around it. The same for every pixel of the pass (KH_FX_SIDE draws it once where it can).
float KhSunFlareVis(int2 sp)
{
    float vis = 0.0f;
    const float khsf_f = KhEncFence();
    [unroll] for (int oy = -2; oy <= 2; ++oy)
    [unroll] for (int ox = -2; ox <= 2; ++ox)
    {
        float khsf_d = LinDepth(LoadDepthPS(sp + int2(ox, oy) * max((int)floor(3.0f * KhFxPx() + 0.5f), 1)));   // KH_FX_PX_REF: nearest whole px.
        vis += saturate((min(khsf_d, khsf_f) - khsf_f * 0.98f)
                        / max(khsf_f * 0.019f, 1.0f));
    }
    return vis / 25.0f;
}

float4 PSEffect(VSOut i) : SV_Target
{
    KhFxBegin(i);   // KH_FX_USER: the lanes, the depth gate, the view-distance cut and the arbitration (cb.hlsl).
    int2  px = int2(i.pos.xy);
    float2 uv = i.pos.xy / float2(fxMeta.z, fxMeta.w);
    int   effect = (int)fxMeta.x;
    float t = fxMeta.y;
    float3 scene = SampleScene(px);
    float3 outc = scene;

    if (centerSize.w > 1.5f) {
        // KH_FX_SIDE: eight fixed pixels, so one verdict for the whole pass - the UI flush draws it once, 1 x 1,
        // before the pass (side id 30) and arms matCtl.z; unarmed, each pixel probes.
        float khpMin = 1.0f;
        [branch] if (matCtl.z > 0.5f) {
            khpMin = khUiProbe.Load(int3(0, 0, 0));
        } else {
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.18f, fxMeta.w * 0.21f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.47f, fxMeta.w * 0.16f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.79f, fxMeta.w * 0.24f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.23f, fxMeta.w * 0.52f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.68f, fxMeta.w * 0.47f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.31f, fxMeta.w * 0.77f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.58f, fxMeta.w * 0.84f)));
            khpMin = min(khpMin, KhUiCov(int2(fxMeta.z * 0.86f, fxMeta.w * 0.69f)));
        }
        if (effect == 30) return float4(khpMin, 0.0f, 0.0f, 1.0f);   // KH_FX_SIDE: the probe itself.
        if (khpMin >= 0.75f) {
            float4 khpRaw = sceneColor.Load(int3(px, 0));
            return float4(khpRaw.rgb, khpRaw.a);
        }
    }

    if (effect == 1)   // Invert: (1 - scene) * tint.
    {
        outc = (1.0f - saturate(scene)) * color.rgb;
    }
    else if (effect == 2)   // colorGrade: [saturation, contrast, brightness, gamma], tint =
                            // Color.rgb.
    {
        float3 c = scene * color.rgb * fxParams0.z;
        float l = Luma(c);
        c = lerp(l.xxx, c, fxParams0.x);
        c = (c - 0.5f) * fxParams0.y + 0.5f;
        // The exponent floored at 1e-4: gamma 0 (or below) was pow(0, 0) = NaN on every black pixel (and inf below
        // zero); at the floor black stays black and the rest goes to ~1 - the limit gamma -> 0 gives.
        c = pow(max(c, 0.0f), max(fxParams0.w, 1.0e-4f));
        outc = c;
    }
    else if (effect == 3)   // Vignette: [startRadius, softness], color = edge color.
    {
        float d = distance(uv, float2(0.5f, 0.5f)) * 1.4142f;
        float v = smoothstep(fxParams0.x, fxParams0.x + max(fxParams0.y, 1e-3f), d);
        outc = lerp(scene, color.rgb, v);
    }
    else if (effect == 4)   // Chromatic aberration: [strengthPx], radial.
    {
        float2 off = (uv - 0.5f) * fxParams0.x * KhFxPx();   // KH_FX_PX_REF.
        float r = SampleScene(int2(i.pos.xy + off)).r;
        float b = SampleScene(int2(i.pos.xy - off)).b;
        outc = float3(r, scene.g, b);
    }
    else if (effect == 5)   // Film grain: [amount, fps, grainSizePx, lumaResponse, chroma].
    {
        // Filmic grain: smooth value noise with spatial extent, triangular
        // amplitude distribution, response peaking in the mid-shadows and
        // protecting highlights, optional chroma, time quantized to a frame
        // rate.
        const float3 gc = KhGrainGc(i.pos.xy, t, fxParams0, fxParams1.x);   // KhFusePoint's twin: one body.

        float luma = saturate(Luma(scene));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, fxParams0.w);
        outc = scene + gc * fxParams0.x * resp;
    }
    else if (effect == 6)   // Sharpen: [strength].
    {
        float3 s = scene * (1.0f + 4.0f * fxParams0.x)
                 - fxParams0.x * (SampleScene(px + int2(1, 0)) + SampleScene(px - int2(1, 0))
                                + SampleScene(px + int2(0, 1)) + SampleScene(px - int2(0, 1)));
        outc = max(s, 0.0f);
    }
    else if (effect == 7)   // Gaussian-ish blur: [radiusPx].
    {
        // KH_GLOW_PYR: the same 3 x 3 kernel, each tap over the radius (the tap spacing).
        const float khbl_s = max(fxParams0.x * KhFxPx(), 1.0f);
        const float khbl_m = KhGlowMix(khbl_s);
        float3 acc = 0.0f;
        if (khbl_m < 1.0f)
        {
            int r = max((int)(fxParams0.x * KhFxPx()), 1);   // KH_FX_PX_REF.
            acc = scene * 0.25f;
            acc += (SampleScene(px + int2(r, 0)) + SampleScene(px - int2(r, 0))
                  + SampleScene(px + int2(0, r)) + SampleScene(px - int2(0, r))) * 0.125f;
            acc += (SampleScene(px + int2(r, r)) + SampleScene(px - int2(r, r))
                  + SampleScene(px + int2(r, -r)) + SampleScene(px + int2(-r, r))) * 0.0625f;
        }
        float3 khbl_p = 0.0f;
        if (khbl_m > 0.0f)
        {
            const float2 khbl_c = i.pos.xy;
            const float2 khbl_x = float2(khbl_s, 0.0f);
            const float2 khbl_y = float2(0.0f, khbl_s);
            khbl_p = KhGlowTap(khbl_c, khbl_s) * 0.25f
                   + (KhGlowTap(khbl_c + khbl_x, khbl_s) + KhGlowTap(khbl_c - khbl_x, khbl_s)
                    + KhGlowTap(khbl_c + khbl_y, khbl_s) + KhGlowTap(khbl_c - khbl_y, khbl_s)) * 0.125f
                   + (KhGlowTap(khbl_c + khbl_x + khbl_y, khbl_s) + KhGlowTap(khbl_c - khbl_x - khbl_y, khbl_s)
                    + KhGlowTap(khbl_c + khbl_x - khbl_y, khbl_s) + KhGlowTap(khbl_c - khbl_x + khbl_y, khbl_s)) * 0.0625f;
        }
        outc = KhGlowSel(acc, khbl_p, khbl_m);
    }
    else if (effect == 8)   // Bloom: [threshold, intensity, radiusPx].
    {
        // KH_GLOW_PYR: the same 5 x 5 grid, each tap over the radius (the tap spacing); the pyramid is thresholded.
        const float khbm_s = max(fxParams0.z * KhFxPx(), 1.0f);
        const float khbm_m = KhGlowMix(khbm_s);
        float3 acc = 0.0f;
        if (khbm_m < 1.0f)
        {
            int r = max((int)(fxParams0.z * KhFxPx()), 1);   // KH_FX_PX_REF.
            [unroll] for (int oy = -2; oy <= 2; ++oy)
            [unroll] for (int ox = -2; ox <= 2; ++ox)
                acc += max(SampleScene(px + int2(ox, oy) * r) - fxParams0.x, 0.0f);
        }
        float3 khbm_p = 0.0f;
        if (khbm_m > 0.0f)
        {
            [unroll] for (int khbm_y = -2; khbm_y <= 2; ++khbm_y)
            [unroll] for (int khbm_x = -2; khbm_x <= 2; ++khbm_x)
                khbm_p += KhGlowTap(i.pos.xy + float2((float)khbm_x, (float)khbm_y) * khbm_s, khbm_s);
        }
        outc = scene + KhGlowSel(acc, khbm_p, khbm_m) / 25.0f * fxParams0.y;
    }
    else if (effect == 9)   // Distortion: [amplitudePx, frequency, speed].
    {
        float2 off = float2(KhSin(uv.y * fxParams0.y * 6.2832f + t * fxParams0.z),   // KH_HASH: t grows.
                            KhCos(uv.x * fxParams0.y * 6.2832f + t * fxParams0.z)) * fxParams0.x * KhFxPx();   // KH_FX_PX_REF.
        outc = SampleScene(int2(i.pos.xy + off));
    }

        else if (effect == 10)   // Outline: [depthEdgeScale, lumEdgeScale, sceneDarken, glowBoost],
                                 // Color = edge.
    {
        // KH_FX_PX_REF: the step is the drawn line's width - one reference pixel, to the nearest whole pixel - and a
        // step spanning the same part of the picture measures the same depth and luma differences, so the thresholds
        // hold at any size.
        const int khol_d = max((int)floor(KhFxPx() + 0.5f), 1);
        float dC = LinDepth(LoadDepthPS(px));
        float dX = LinDepth(LoadDepthPS(px + int2(khol_d, 0))) - dC;
        float dY = LinDepth(LoadDepthPS(px + int2(0, khol_d))) - dC;
        float depthEdge = saturate((abs(dX) + abs(dY)) / max(dC, 1.0f) * fxParams0.x);
        float lC = Luma(scene);
        float lumEdge = saturate((abs(Luma(SampleScene(px + int2(khol_d, 0))) - lC)
                                + abs(Luma(SampleScene(px + int2(0, khol_d))) - lC)) * fxParams0.y);
        float edge = saturate(depthEdge + lumEdge);
        outc = scene * fxParams0.z + color.rgb * edge * fxParams0.w;
    }
    else if (effect == 11)   // Pulse: p0.xyz = center (engine space), p0.w = radius; p1 =
                             // [bandWidth, intensity].
    {
        // Fenced reconstruction + the fence feather keep the ring continuous:
        // sky (clamped to the fence) lands at feather zero.
        float khpl_d;
        float dist = distance(KhWorldPosFenced(px, uv, khpl_d), fxParams0.xyz);
        float band = max(fxParams1.x, 0.01f);
        float ring = 1.0f - saturate(abs(dist - fxParams0.w) / band);
        ring *= ring;
        float khpl_f = KhEncFence();
        ring *= 1.0f - saturate((khpl_d - khpl_f * 0.98f) / max(khpl_f * 0.019f, 1.0f));
        outc = scene + color.rgb * ring * fxParams1.y;
    }
    else if (effect == 12)   // Halation: [threshold, intensity, radiusPx], color = glow tint
                             // (warm).
    {
        const int2 dirs[8] = { int2(1,0), int2(-1,0), int2(0,1), int2(0,-1),
                               int2(1,1), int2(-1,1), int2(1,-1), int2(-1,-1) };
        // KH_GLOW_PYR: the same two rings, each tap over its ring's spacing (the radius, twice it); thresholded.
        const float khha_s = max(fxParams0.z * KhFxPx(), 1.0f);
        const float khha_m = KhGlowMix(khha_s);
        float3 acc = 0.0f;
        if (khha_m < 1.0f)
        {
            int r = max((int)(fxParams0.z * KhFxPx()), 1);   // KH_FX_PX_REF.
            [unroll] for (int k = 0; k < 8; ++k)
                acc += max(SampleScene(px + dirs[k] * r) - fxParams0.x, 0.0f) * 0.09f;
            [unroll] for (int k2 = 0; k2 < 8; ++k2)
                acc += max(SampleScene(px + dirs[k2] * r * 2) - fxParams0.x, 0.0f) * 0.035f;
        }
        float3 khha_p = 0.0f;
        if (khha_m > 0.0f)
        {
            [unroll] for (int khha_k = 0; khha_k < 8; ++khha_k)
                khha_p += KhGlowTap(i.pos.xy + float2(dirs[khha_k]) * khha_s, khha_s) * 0.09f;
            [unroll] for (int khha_j = 0; khha_j < 8; ++khha_j)
                khha_p += KhGlowTap(i.pos.xy + float2(dirs[khha_j]) * (khha_s * 2.0f), khha_s * 2.0f) * 0.035f;
        }
        outc = scene + KhGlowSel(acc, khha_p, khha_m) * color.rgb * fxParams0.y;
    }
    else if (effect == 13)   // Distance fog: [startDist m, endDist m, skyAmount 0.1], color = fog
                             // Color.
    {
        // Twin edit at KhFsFog's fog-pass loop.
        float d = LinDepth(LoadDepthPS(px));
        float f = saturate((d - fxParams0.x) / max(fxParams0.y - fxParams0.x, 1.0f));
        float khfg_f = KhEncFence();
        float khfg_w = saturate((min(d, khfg_f) - khfg_f * 0.98f) / max(khfg_f * 0.019f, 1.0f));
        f = lerp(f, saturate(fxParams0.z), khfg_w);
        if (d > 1e8f) f = saturate(fxParams0.z);   // Sky/far-plane pixels (belt; the feather
                                                   // Already lands here).
        outc = lerp(scene, color.rgb, f);
    }
    else if (effect == 14)   // Lens flare, image-based: [threshold, intensity, ghostCount,
                             // ghostSpacing] + [haloRadius, haloIntensity.
    {
        // Bright pixels anywhere in the capture spawn a ghost train mirrored
        // through screen centre, plus a halo ring. The HDR capture makes the
        // threshold physically meaningful.
        float2 cuv = float2(0.5f, 0.5f);
        float2 ghostVec = (cuv - uv) * fxParams0.w;
        int nGhosts = clamp((int)fxParams0.z, 1, 8);
        float3 acc = 0.0f;
        // KH_GLOW_PYR: a ghost is the picture scaled by 1 - ghostSpacing * g about the centre, so a point read
        // either magnifies pixels into blocks or skips them; from the (thresholded) pyramid each ghost reads over
        // that scale (two pixels at least), and so does the halo.
        const bool khlf_g = KhGlowOn();
        for (int g = 1; g <= nGhosts; ++g)
        {
            float2 suv = uv + ghostVec * (float)g;
            float w = 1.0f - saturate(length(suv - cuv) * 1.6f);
            w = w * w;
            float2 spf = saturate(suv) * float2(fxMeta.z, fxMeta.w);
            float2 cdir = normalize(ghostVec + 1e-5f) * fxParams1.z * KhFxPx();   // KH_FX_PX_REF.
            float3 s;
            if (khlf_g)
            {
                const float khlf_f = max(abs(1.0f - fxParams0.w * (float)g), 2.0f);
                s.r = KhGlowTap(spf + cdir, khlf_f).r;
                s.g = KhGlowTap(spf, khlf_f).g;
                s.b = KhGlowTap(spf - cdir, khlf_f).b;
            }
            else
            {
                s.r = max(SampleScene(int2(spf + cdir)).r - fxParams0.x, 0.0f);
                s.g = max(SampleScene(int2(spf)).g        - fxParams0.x, 0.0f);
                s.b = max(SampleScene(int2(spf - cdir)).b - fxParams0.x, 0.0f);
            }
            acc += s * w;
        }
        float rC = length(uv - cuv);
        float hw = 1.0f - saturate(abs(rC - fxParams1.x) * 8.0f);
        float2 huv = uv + normalize(cuv - uv + 1e-5f) * fxParams1.x;
        if (khlf_g)
            acc += KhGlowTap(saturate(huv) * float2(fxMeta.z, fxMeta.w), 2.0f) * hw * fxParams1.y;
        else
            acc += max(SampleScene(int2(saturate(huv) * float2(fxMeta.z, fxMeta.w))) - fxParams0.x, 0.0f) * hw * fxParams1.y;
        outc = scene + acc * color.rgb * fxParams0.y;
    }

        else if (effect == 15)   // Anamorphic streak: [threshold, intensity, lengthPx, falloffPow]
                                 // + [vertical 0/1]; color = tint.
    {
        float3 acc = 0.0f;
        float total = 0.0f;
        // KH_ANA_PYR: the taps are lengthPx / 16 apart; from two pixels up each reads the one-axis pyramid over that
        // spacing (x KH_GLOW_FP, the glows' margin), so the streak is one continuous line instead of 16 copies of
        // each highlight a side. Taps a pixel apart or less read the picture itself, as before (KhGlowMix).
        const float khan_l = fxParams0.z * KhFxPx();   // KH_FX_PX_REF.
        const float khan_m = KhGlowMix(khan_l / 16.0f);

        [unroll] for (int k = 1; k <= 16; ++k)
        {
            float t = (float)k / 16.0f;
            float w = pow(1.0f - t, max(fxParams0.w, 0.1f));
            if (khan_m < 1.0f)
            {
                int off = (int)(t * fxParams0.z * KhFxPx());   // KH_FX_PX_REF.
                int2 d = (fxParams1.x > 0.5f) ? int2(0, off) : int2(off, 0);
                acc += (max(SampleScene(px + d) - fxParams0.x, 0.0f)
                      + max(SampleScene(px - d) - fxParams0.x, 0.0f)) * w;
            }
            total += 2.0f * w;
        }
        float3 khan_p = 0.0f;
        if (khan_m > 0.0f)
        {
            // The level whose texel (2^(k+1) px) is the spacing x KH_GLOW_FP, as the two around it linear in log2
            // (trilinear by hand); a spacing past the top level's reads the top level. The pair is one even and one
            // odd level, one from each atlas; the weights are the pass's, so each branch is uniform.
            const bool khan_v = fxParams1.x > 0.5f;
            const int khan_len = (int)((khan_v ? fuseMeta.w : fuseMeta.z) + 0.5f);
            const int khan_cw = (int)((khan_v ? fuseMeta.z : fuseMeta.w) + 0.5f);
            const int khan_c = clamp((int)(khan_v ? i.pos.x : i.pos.y), 0, max(khan_cw - 1, 0));
            const float khan_pc = khan_v ? i.pos.y : i.pos.x;
            const float khan_lv = clamp(log2(KH_GLOW_FP * khan_l / 16.0f) - 1.0f, 0.0f, (float)(KH_ANA_LEVELS - 1));
            const int khan_k0 = min((int)khan_lv, KH_ANA_LEVELS - 2);
            const float khan_f = khan_lv - (float)khan_k0;
            const bool khan_odd = (khan_k0 & 1) != 0;
            const float khan_we = khan_odd ? khan_f : 1.0f - khan_f;   // The even level's share.
            [branch] if (khan_we > 0.0f)
                khan_p += khan_we * KhAnaStreak(khsgTex, khan_odd ? khan_k0 + 1 : khan_k0, khan_len, khan_pc,
                                                khan_c, khan_v, khan_l);
            [branch] if (khan_we < 1.0f)
                khan_p += (1.0f - khan_we) * KhAnaStreak(khAnaB, khan_odd ? khan_k0 : khan_k0 + 1, khan_len, khan_pc,
                                                         khan_c, khan_v, khan_l);
        }
        acc = KhGlowSel(acc, khan_p, khan_m);

        acc /= max(total, 1.0f);
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 16)   // Sun flare, source-aware: p0.xyz = direction (engine space), p0.w =
                             // Size;
    {
        // KhSunFlareSpot / KhSunFlareVis above. KH_FX_SIDE: the scene chain draws the visibility once, 1 x 1,
        // before the pass (side id 29) and arms matCtl.y; unarmed, each pixel measures it.
        float2 spos;
        int2 sp;
        if (KhSunFlareSpot(spos, sp))
        {
            {
                float vis;   // A branch, not ?: (which evaluates both arms).
                [branch] if (matCtl.y > 0.5f) vis = khFxSide.Load(int3(0, 0, 0));
                else                          vis = KhSunFlareVis(sp);
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
    else if (effect == 29)   // KH_FX_SIDE: the sun flare's source visibility (1 x 1, drawn before the pass).
    {
        float2 spos;
        int2 sp;
        return float4(KhSunFlareSpot(spos, sp) ? KhSunFlareVis(sp) : 0.0f, 0.0f, 0.0f, 1.0f);
    }

        else if (effect == 17)   // Glitch: [intensity, speed, sliceAmountPx, sliceBands] +
                                 // [colorSplitPx, blockAmount, noiseAmount, burstiness].
    {
        float speed = max(fxParams0.y, 0.1f);
        float tf = floor(t * speed);

        float burst = Hash(float2(tf * 0.0131f, 7.31f));
        burst = pow(burst, 1.0f + fxParams1.w * 6.0f);
        float drive = lerp(1.0f, burst, saturate(fxParams1.w)) * fxParams0.x;

        float2 suv = i.pos.xy;

        // Horizontal slice displacement: few large tears, many small ones.
        float bands = max(fxParams0.w, 1.0f);
        float band = floor(uv.y * bands);
        float bh = (Hash(float2(band, tf)) - 0.5f) * 2.0f;
        bh = sign(bh) * pow(abs(bh), 3.0f);
        suv.x += bh * fxParams0.z * drive * KhFxPx();   // KH_FX_PX_REF.

        // Block corruption: coarse grid cells randomly displaced.
        if (fxParams1.y > 0.001f)
        {
            float2 cell = floor(uv * float2(24.0f, 14.0f));
            float ch = Hash(cell + tf * 1.7f);
            if (ch > 1.0f - 0.15f * saturate(fxParams1.y) * saturate(drive))
            {
                float2 off = (float2(Hash(cell + 3.1f + tf), Hash(cell + 5.7f + tf)) - 0.5f) * 80.0f * drive * KhFxPx();   // KH_FX_PX_REF.
                suv += off;
            }
        }

        // RGB channel split along the tear axis.
        float split = fxParams1.x * drive * KhFxPx();   // KH_FX_PX_REF.
        float3 col;
        col.r = SampleScene(int2(suv + float2(split, 0.0f))).r;
        col.g = SampleScene(int2(suv)).g;
        col.b = SampleScene(int2(suv - float2(split, 0.0f))).b;

        // Interference lines + static.
        if (fxParams1.z > 0.001f)
        {
            float ln = Hash(float2(floor(i.pos.y * 0.5f / KhFxPx()), tf * 2.3f));   // KH_FX_PX_REF: 2-px rows.
            float lineHit = step(1.0f - 0.2f * saturate(fxParams1.z) * saturate(drive), ln);
            // KH_FX_PX_REF: one static cell per reference pixel (the cell centre is the pixel centre at 1080).
            float n = Hash((floor(i.pos.xy / KhFxPx()) + 0.5f) * 0.37f + tf * 13.7f);
            col = lerp(col, color.rgb * n, lineHit * 0.85f);
            col += (n - 0.5f) * fxParams1.z * drive * 0.35f;
        }

        outc = max(col, 0.0f);
    }
     else if (effect == 18)   // Clarity: [strength, radiusPx].
    {
        // Wide-radius local contrast on luma only (the scene is rescaled by
        // L'/L, so saturation never moves). Two-ring 12-tap base estimate; the
        // detail term is soft-limited so strong edges do not halo.
        float rad = max(fxParams0.y, 4.0f) * KhFxPx();   // KH_FX_PX_REF.
        float lC = Luma(scene);
        float lB = 0.0f;
        // KH_GLOW_PYR: the twelve taps sit 0.785 rad apart along their rings (2 pi rad / 8, 2 pi (rad / 2) / 4);
        // each over that spacing, the base estimate is the smooth local average it stands for.
        const float khcl_s = 0.7854f * rad;
        const float khcl_m = KhGlowMix(khcl_s);

        if (khcl_m < 1.0f)
        {
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
        }
        float khcl_p = 0.0f;
        if (khcl_m > 0.0f)
        {
            [loop] for (int khcl_k = 0; khcl_k < 8; ++khcl_k)
            {
                const float khcl_a = float(khcl_k) * 0.7854f + 0.3927f;
                khcl_p += Luma(KhGlowTap(i.pos.xy + float2(cos(khcl_a), sin(khcl_a)) * rad, khcl_s)) * 0.0833f;
            }
            [loop] for (int khcl_j = 0; khcl_j < 4; ++khcl_j)
            {
                const float khcl_b = float(khcl_j) * 1.5708f;
                khcl_p += Luma(KhGlowTap(i.pos.xy + float2(cos(khcl_b), sin(khcl_b)) * (rad * 0.5f), khcl_s)) * 0.0833f;
            }
        }
        lB = KhGlowSel(lB, khcl_p, khcl_m);

        float detail = lC - lB;
        detail = detail / (1.0f + 2.5f * abs(detail));   // Soft limiter (anti-halo).
        float scale = (lC + fxParams0.x * detail * 2.2f) / max(lC, 1e-3f);
        outc = scene * clamp(scale, 0.3f, 3.0f);
    }
    else if (effect == 19)   // Deband: [threshold(1/255), rangePx, grain(1/255)].
    {
        // Gradient debanding: average four taps on a hash-rotated cross within
        // range; adopt the average only where it stays inside the threshold
        // (real edges never do). Triangular-pdf grain breaks residual
        // quantization. Static noise (no TAA exists to launder animation).
        float2 h = Hash2(i.pos.xy);
        float ang = h.x * 6.2832f;
        float rad = (0.3f + 0.7f * h.y) * max(fxParams0.y, 2.0f) * KhFxPx();   // KH_FX_PX_REF (the dither stays per pixel).
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
    else if (effect == 20)   // Rainlens: [intensity, speed, condensation, refract]; fx1 = system
                             // Camera velocity.
    {
        float aspect = fxMeta.z / max(fxMeta.w, 1.0f);
        float2 q = float2(uv.x * aspect, uv.y);
        float inten = saturate(fxParams0.x);
        float fwdK = saturate(max(fxParams1.z, 0.0f) / 20.0f);
        float tt = fxParams1.w * max(fxParams0.y, 0.0f);
        float windBase = clamp(-fxParams1.x * 0.055f, -0.5f, 0.5f);
        float wind = windBase + (q.x / aspect - 0.5f) * fwdK * 0.6f;
        float4 L1 = KhRainLayer(q, tt,          float2(6.0f, 1.4f), 0.0f,
                                0.45f + inten * 0.4f, wind, fwdK);
        float4 L2 = KhRainLayer(q, tt * 1.27f,  float2(10.0f, 2.4f), 3.7f,
                                0.3f + inten * 0.45f, wind * 1.15f, fwdK);
        float drop  = max(L1.z, L2.z * 0.85f);
        float trail = max(L1.w, L2.w * 0.8f);
        float2 nrm  = L1.xy + L2.xy * 0.7f;
        // Condensation film: blurred, faintly lifted scene; wiped by trails and
        // punched through by the drops themselves.
        float fogAmt = saturate(fxParams0.z) * saturate(0.25f + inten);
        float3 fogC = 0.0f;
        // KH_GLOW_PYR: the film's eight taps sit 2 pi 7 / 8 = 5.5 reference pixels apart; each over that spacing.
        const float khrl_s = 5.4978f * KhFxPx();
        const float khrl_m = KhGlowMix(khrl_s);

        if (khrl_m < 1.0f)
        {
            [loop] for (int k = 0; k < 8; ++k)
            {
                float ang = float(k) * 0.7854f;
                fogC += SampleScene(px + int2(cos(ang) * 7.0f * KhFxPx(), sin(ang) * 7.0f * KhFxPx())) * 0.125f;   // KH_FX_PX_REF.
            }
        }
        float3 khrl_p = 0.0f;
        if (khrl_m > 0.0f)
        {
            [loop] for (int khrl_k = 0; khrl_k < 8; ++khrl_k)
            {
                const float khrl_a = float(khrl_k) * 0.7854f;
                khrl_p += KhGlowTap(i.pos.xy + float2(cos(khrl_a), sin(khrl_a)) * (7.0f * KhFxPx()), khrl_s) * 0.125f;
            }
        }
        fogC = KhGlowSel(fogC, khrl_p, khrl_m);

        fogC = lerp(fogC, Luma(fogC).xxx, 0.12f) * 1.02f;
        // Fine static condensation droplets (twinkle in with the film, cleared
        // where trails wiped).
        float2 sid = floor(q * 34.0f);
        float sn = Hash(sid);
        float2 sf = frac(q * 34.0f) - 0.5f;
        float slife = saturate(KhSin(t * 0.35f + sn * 6.2832f) * 0.5f + 0.5f);   // KH_HASH.
        float sdrop = smoothstep(0.12f + sn * 0.1f, 0.05f, length(sf))
                    * step(sn, fogAmt * 0.8f) * slife * (1.0f - saturate(trail * 1.6f));
        nrm += (sf / 0.2f) * sdrop * 0.18f;
        drop = max(drop, sdrop * 0.5f);
        float wiped = saturate(trail * 1.5f + drop);
        float3 base = lerp(scene, fogC, fogAmt * (1.0f - wiped) * 0.85f);
        // Refraction through the drops: inverted-wide sample + chroma split.
        float refr = max(fxParams0.w, 0.0f);
        float2 offPx = -nrm * refr * (20.0f + fwdK * 8.0f) * KhFxPx();   // KH_FX_PX_REF.
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
    else if (effect == 21)   // Crt: [curvature, scanlines, lineCount, maskStrength] +
                             // [aberrationPx, flicker, rollingBand, cornerRadius] +
                             // [lineScroll (lines/s), wobblePx, maskSizePx].
    {
        float khc_curv = max(fxParams0.x, 0.0f);
        float2 khc_cc = uv - 0.5f;
        float khc_r2 = dot(khc_cc, khc_cc);
        float2 khc_duv = 0.5f + khc_cc * (1.0f + khc_curv * khc_r2 * (1.0f + 0.8f * khc_r2));

        float khc_lines = max(fxParams0.z, 16.0f);
        float khc_phase = khc_duv.y * khc_lines + t * fxParams2.x;
        float khc_line = floor(khc_phase);

        // Tracking wobble (fxParams2.y, px): each scanline row shifts by a
        // per-line random jitter requantized at 24 Hz plus a slow per-line
        // sway. Only the picture wobbles; the tube mask and the grille live on
        // the glass.
        float khc_wob = ((Hash(float2(khc_line * 0.173f, floor(t * 24.0f) * 0.71f)) - 0.5f)
                       + 0.35f * KhSin(t * 2.3f + khc_line * 0.61f)) * fxParams2.y * KhFxPx();   // KH_FX_PX_REF, KH_HASH.

        float2 khc_dpx = khc_duv * float2(fxMeta.z, fxMeta.w);
        khc_dpx.x += khc_wob;
        float2 khc_fpx = khc_cc * khc_r2 * 4.0f * fxParams1.x * KhFxPx();   // KH_FX_PX_REF.
        float3 khc_col;
        khc_col.r = SampleScene(int2(khc_dpx + khc_fpx)).r;
        khc_col.g = SampleScene(int2(khc_dpx)).g;
        khc_col.b = SampleScene(int2(khc_dpx - khc_fpx)).b;

        // Beam scanlines: a shaped sine whose exponent narrows in the shadows
        // and widens toward the highlights (phosphor blooming); the 1.32 gain
        // recovers the average level the dark gaps remove.
        float khc_lum = saturate(Luma(khc_col));
        float khc_beam = pow(abs(KhSin(khc_phase * 3.14159265f)),   // KH_HASH: the phase scrolls with t.
                             lerp(2.2f, 0.65f, khc_lum));
        khc_col *= lerp(1.0f, khc_beam * 1.32f, saturate(fxParams0.y));

        // Aperture grille (KH_CRT_MASK): R, G and B stripes, one triad maskSizePx wide (fxParams2.z, pixels of a
        // 1080-row picture - KH_FX_PX_REF - at least 1, default 3), each stripe's share of this pixel
        // box-filtered, so any size draws at any resolution without moire; a triad only a few pixels wide
        // averages toward grey, as a real one seen from afar does. On the glass: screen space, not curved.
        // At the default size on 1080 rows each pixel is one whole stripe - the unfiltered triad exactly.
        float khc_mk = saturate(fxParams0.w);
        const float khc_tp = max(fxParams2.z, 1.0f) * KhFxPx();
        const float khc_xl = i.pos.x - 0.5f;
        const float khc_xr = i.pos.x + 0.5f;
        float3 khc_tri = 0.45f + 0.55f * float3(KhCrtStripe(khc_xr, khc_tp, 0.0f) - KhCrtStripe(khc_xl, khc_tp, 0.0f),
                                                KhCrtStripe(khc_xr, khc_tp, 1.0f) - KhCrtStripe(khc_xl, khc_tp, 1.0f),
                                                KhCrtStripe(khc_xr, khc_tp, 2.0f) - KhCrtStripe(khc_xl, khc_tp, 2.0f));
        khc_col *= lerp(float3(1.0f, 1.0f, 1.0f), khc_tri * 1.35f, khc_mk);

        // Rolling sync band: a soft dark bar drifting down the frame and
        // wrapping (the vertical-hold-slipping read).
        float khc_bpos = frac(t * 0.11f);
        float khc_bd = abs(khc_duv.y - khc_bpos);
        khc_bd = min(khc_bd, 1.0f - khc_bd);
        float khc_band = 1.0f - smoothstep(0.0f, 0.16f, khc_bd);
        khc_col *= 1.0f - khc_band * khc_band * 0.22f * saturate(fxParams1.z);

        // Mains flicker: a fast beat plus a per-refresh random sparkle
        // quantized to 60 Hz.
        float khc_fl = saturate(fxParams1.y);
        float khc_hum = KhSin(t * 100.0f * 3.14159265f) * 0.5f + 0.5f;   // KH_HASH.
        float khc_spark = Hash(float2(floor(t * 60.0f), 3.7f)) - 0.5f;
        khc_col *= 1.0f + khc_fl * (khc_hum * 0.04f + khc_spark * 0.05f);

        // Phosphor tint, then the tube face: rounded-rectangle mask in the
        // distorted space (out-of-range refraction lands outside it - the bezel
        // is black), a ~2 px feathered edge and a soft glass falloff into the
        // corners.
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
