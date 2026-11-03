// effect.hlsl - concatenated after cb.hlsl into the effect unit by C++ (no #include). Any edit changes the unit's
// shader cache key. Its shared declarations and helpers are cb.hlsl's KH_FX_UNIT section (a user effect compiles
// against the same text); what is here is the builtins' own.

// FX_SIDE - a value that is the same for a whole pass, or that every pixel would otherwise recompute for its
// neighbours, drawn once before the pass by this unit (PSEffect; 28 by PSFsSide where made) into a target of ours (28
// the frame's size, 29 / 30 1 x 1; the side ids 28 - 30 return it) and read here. Each is armed by its own lane of
// matCtl (y, z), lanes no effect pass otherwise fills (bind_material writes matCtl for textured meshes only,
// FX_LINZ below arms x / w alone, and every effect pass's constants start from a zeroed template); unarmed - effect
// meshes, the PIP, and any route that did not draw it - the pass computes the value in place. t4 / t5 are free in this
// unit (cb.hlsl's receive textures there are fenced out of it).
Texture2D<float> khFxSide  : register(t4);   // matCtl.y: fog scatter's per-pixel fog (28, full size) / the sun flare's visibility (29, 1 x 1).
Texture2D<float> khUiProbe : register(t5);   // matCtl.z: the UI lane's coverage probe (30, 1 x 1).

// FX_LINZ - the frame's linear depth (LinDepth(LoadDepthPS(p)), metres), drawn once per scene-chain run by
// PSFxLinZ (the unit's tail) into targets of ours and read here in place of the conversion - a multisampled load, the
// near-plane marker's load and a division per read - by SSGI, dynamicLightFog and fog scatter. t7: the frame (one
// texel per pixel of fxMeta.zw), armed by matCtl.x. t8 / t9: a grid's copy, keyed the way its passes read it - t8
// the SSGI grid's (texel t holds pixel KhSsgiFull(t), the a-trous' mapping), t9
// dynamicLightFog's (KhDlfFull's) - armed by matCtl.w (1 / 2) when the copy was drawn with the reading pass's own
// mapping. matCtl.x and .w are otherwise unread in this unit (a textured mesh's lanes); cb.hlsl's receive textures at
// t7 - t9 are fenced out of it, as at t4 - t6. Every reader takes the conversion in place when unarmed (effect meshes,
// the PIP, the UI lane, a pass whose depth lanes differ from the run's) or past the copy's edge, so a read is the
// value the conversion gives there whichever way it is taken. C++: fx_linz_draw, fx_chain_run (binds t7 - t9
// and puts back what they held). t7 and KhLinZ are cb.hlsl's (the KH_FX_UNIT section: a user effect reads the same).
Texture2D<float> khGridZS : register(t8);
Texture2D<float> khGridZD : register(t9);
// SSGI_CENTRE: the SSGI grid's size (local0.zw, set by the chain's SSGI block; unset, the frame over the scalar
// factor local0.y, 2 below 0.25) and the full-resolution pixel its texel t stands for - its block's centre, each axis
// by its own factor (fxMeta.zw over the grid's size: the grid's sides are rounded apart, so one factor from the width
// drifts up to ~3 px down the frame against the resolve, which reads the grid by normalized coords), held to the frame
// - KhDlfFull's mapping. The seed (26) samples the same centre; the gather (22), the grid's depth copy (PSFxLinZ 35)
// and KhSsgiGridZ's in-place read take this pixel. Do not take the block's first pixel, int2(t x factor): it lies
// (factor - 1) / 2 pixels up and left of the texel the seed and the resolve take (1.5 at setSsgiScale 0.25).
float2 KhSsgiGrid()
{
    const float khsc_f = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
    return localParams0.z >= 1.0f && localParams0.w >= 1.0f ? localParams0.zw
                                                            : max(float2(fxMeta.z, fxMeta.w) / khsc_f, 1.0f);
}
int2 KhSsgiFull(int2 khsc_t)
{
    const int2 khsc_p = int2((float2(khsc_t) + 0.5f) * float2(fxMeta.z, fxMeta.w) / KhSsgiGrid());
    return clamp(khsc_p, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}
// The SSGI grid's: texel t of the grid (KhSsgiFull's pixel). Read only by the chain's SSGI pass, which binds t8 to
// its copy or to nothing (fx_chain_run), so the size query and the load (ahead of the branch, texel 0 when
// unarmed or past the copy) meet a view of ours or none.
float KhSsgiGridZ(int2 khsz_t)
{
    uint khsz_w, khsz_h;
    khGridZS.GetDimensions(khsz_w, khsz_h);
    const bool khsz_in = matCtl.w > 0.5f && matCtl.w < 1.5f && khsz_t.x >= 0 && khsz_t.y >= 0 &&
                         khsz_t.x < (int)khsz_w && khsz_t.y < (int)khsz_h;
    float khsz_z = khGridZS.Load(int3(khsz_in ? khsz_t : int2(0, 0), 0));
    [branch] if (!khsz_in) khsz_z = KhLinZ(KhSsgiFull(khsz_t));   // SSGI_CENTRE.
    return khsz_z;
}

// Bound at t19 only for LUT passes - t19 is reserved for this unit codebase-wide (inside StateBackup's save range, so
// the engine's own bind is restored after every flush). Sampled with integer Loads only (tetrahedral interpolation; no
// sampler, no half-texel bias).
Texture3D<float4> khLut : register(t19);
float3 KhLutV(int3 p) { return khLut.Load(int4(p, 0)).rgb; }

// CRT_MASK: the length of [0, u] that stripe k (0 R, 1 G, 2 B) of a triad p pixels wide covers - the
// running integral of the stripe's indicator, continuous across period edges, so a pixel's coverage of the
// stripe is the difference at its two edges (an exact box filter).
float KhCrtStripe(float u, float p, float k)
{
    const float w = p / 3.0f;
    const float n = floor(u / p);
    return n * w + clamp(u - n * p - k * w, 0.0f, w);
}

// GLOW_PYR's builders, drawn by glow_build with VSFullscreen over each level's viewport. The seed (level 0):
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

// ANA_PYR - anamorphic's pre-filtered picture: a pyramid halved ALONG THE STREAK'S AXIS ONLY, so every level keeps
// the frame's full resolution across it and the streak stays one pixel thin (an isotropic level thickens it; the
// anisotropic read of one costs about ten times its taps). C++ ana_build makes it per pass, of that pass's own
// source, through its threshold: level k (0 .. KH_ANA_LEVELS - 1) has one texel per 2^(k+1) frame pixels along the
// axis and one per pixel across it - level 0 the frame through the glows' tent (1 3 3 1) along the axis
// (PSAnaSeed), each further level the same tent over the level below (PSAnaDown), clamped to that level's edge.
// The levels are strips of two atlases, the even ones at t3 (khsgTex) and the odd ones at t6 (khAnaB), so a level
// is always built from the other atlas and never read while it is a target; KhAnaLevel places them (C++ twins
// ana_n / ana_off). Armed as the glows are (KhGlowOn, fuseMeta.y); on an anamorphic pass fuseMeta.zw is the
// frame the pyramid was made for (C++ ana_lanes), not the glows' padded extent. t6 is free in this unit, as
// t4 / t5 are (cb.hlsl's receive textures there are fenced out of it).
#define KH_ANA_LEVELS 8   // C++ twin KH_ANA_LEVELS: texels of 2 .. 256 frame pixels along the axis.
Texture2D<float4> khAnaB : register(t6);   // ANA_PYR's odd levels (t3 holds the even ones).
// Level k of a frame khal_len pixels long along the axis: khal_n texels (the frame's pixels 2^(k+1) at a time, the
// last one partly past the edge), at along-offset khal_o in its atlas - after the levels of its own parity below it.
void KhAnaLevel(int khal_k, int khal_len, out int khal_n, out int khal_o)
{
    khal_n = (khal_len + (2 << khal_k) - 1) >> (khal_k + 1);
    khal_o = 0;
    [loop] for (int khal_j = khal_k & 1; khal_j < khal_k; khal_j += 2)
        khal_o += (khal_len + (2 << khal_j) - 1) >> (khal_j + 1);
}
// The builders, drawn by ana_build with VSFullscreen over each level's strip (its viewport). localParams0: x = the
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
    x += (0.42f - abs(x)) * KhSin(tt * 0.7f + n.z * 6.2832f)   // HASH: tt grows with time.
         * 0.4f * (1.0f - saturate(stretch * 1.4f));
    // Drop y: a descending cycle with a linger-then-fall ease (drops hold, then slip - the real-glass cadence).
    float cyc = frac(tt * (0.10f + n.z * 0.07f) + n.x * 7.31f);
    float py = pow(cyc, 0.65f);
    float aniso = grid.x / grid.y;   // Cell tallness in q-space.
    float2 du = float2(f.x - 0.5f - x, (f.y - py) * aniso);
    du.y *= 1.0f / (1.0f + stretch * 0.9f);   // Elongate along the fall (halved).
    float r0 = 0.085f + n.y * 0.06f;
    float len = length(du);
    float drop = smoothstep(r0, r0 * 0.55f, len);
    // Trail: the wet wipe above the drop, fading with distance behind, with small satellite beads left on the track.
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

// fxParams1 / fxParams2 are system lanes for effect 23: the chain loop packs up to two aggregated fog-pass records
// [startDist, endDist, skyAmount, opacity] (fogscatter_pack); zeroed lanes = no pass.

float KhFsFog(float2 fs_px, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = KhFxFogEngine(fs_px, fs_d, fs_res, fs_m00, fs_m11);   // FX_USER: the engine term, one body.
    fs_s = KhFxFogPasses(fs_s, fs_d, fxParams1, fxParams2);   // FX_FOGREC: the KH fog passes, one body (cb.hlsl).
    return saturate(fs_s);
}
// WSEE: fog scatter's per-pixel fog share at fs_p of view depth fs_d - where the world's blended layers cover it,
// the share composed by layer group at each group's depth (KhWseeMix; one group at its own depth), per sample where
// the samples disagree; elsewhere and unarmed KhFsFog itself. Every caller that computes a pixel's share takes it (the
// receiver, the taps in place, side draw 28); the scatter's depth comparisons keep the depth buffer's.
float KhFsFogW(int2 fs_p, float fs_d, float2 fs_res, float fs_m00, float fs_m11)
{
    float fs_s = KhFsFog(float2(fs_p), fs_d, fs_res, fs_m00, fs_m11);
    [branch] if (KhWseeArmed()) {
        const float fs_r = LoadDepthPS(fs_p);
        KhWseeMix(fs_p, fs_r, fs_s, fs_z, KhFsFog(float2(fs_p), fs_z, fs_res, fs_m00, fs_m11));
    }
    return fs_s;
}

// Sun flare's source (effect 16, side id 29): its direction, fxParams0.xyz (engine axes - the C++ converts the
// script's), through cb.hlsl's KhFxSunSpot; KhSunFlareVis / KhSunFlareEdge are cb.hlsl's too.
bool KhSunFlareSpot(out float2 spos, out int2 sp) { return KhFxSunSpot(fxParams0.xyz, spos, sp); }

// DLF - dynamicLightFog (effect 31): the world fog lit by the engine's dynamic lights, marched per light through
// the light's own reach and shadowed by our meshes' light maps and by the geometry the camera sees. C++ side:
// dlf_select (the lights: the engine's cb11 records, camera-relative, [5].z = our map's slot + 1 or 0, [5].w = the
// light's reach in metres), fx_chain_run's DLF block (gather 32 onto the SSGI grid, filter 33 twice, then the
// pass itself, 31, composites). The frame is camera-relative (fxCam armed - the C++ side runs nothing without it):
// invViewProj is rotation-only and the camera sits at the origin.
static const int   KH_DLF_SS_TAPS    = 4;       // Screen-space occlusion taps per march sample (shadows 2).
static const float KH_DLF_SS_KEEP    = 0.25f;   // m short of the light the taps stop (the source itself).
static const float KH_DLF_SS_SELF    = 1.0f;    // m: a surface this near the light is its own body - the housing,
                                                 // the lamp head, the hand or hull holding it - and shadows nothing:
                                                 // the engine's lights shine through their housings, so the fog too.
static const float KH_DLF_SS_THICK   = 1.0f;    // m, plus this share of the view depth: the most the visible depth
static const float KH_DLF_SS_THICK_K = 0.02f;   // may change across a crossing for it to be one surface, not an edge.
static const int   KH_DLF_SS_REFINE  = 5;       // Halvings of a tap interval where the segment goes behind the scene.
static const float KH_DLF_SS_BIAS_K  = 0.002f;  // A point counts as behind a surface past this share of its depth
                                                 // (plus 5 cm): the sample's own surface is never its occluder.
static const float KH_DLF_SS_RISE    = 1.0f;    // DLF_SS: m straight up from a surface's point that
                                                 // must still show that surface (within a tenth, plus THICK_K of the
                                                 // depth) for it to STAND - a wall - and be continued behind a nearer
                                                 // one; the ground, a roof, a ceiling recede and are not. No C++ twin.
// DLF_SS_R2: no per-light verdict from single-pixel probes about the light (a walled-light or enclosing-plane
// test): behind foliage each probe is leaf or gap by chance, so camera motion flips such a verdict and the light's
// whole glow with it. KhDlfSsCovered's one-pixel gate (the plane continuation's) flips as often but moves the glow by
// about 1.4 % behind a canopy (up to about 4 % with a wall within SELF before the light). Limits: a lantern 0.3 - 1 m
// behind a wall leaks through it within SELF of the light (the housing rule), and the fog round a body before such a
// wall can glow where the raised probe lands on the body - neither sees what the depth buffer does not hold.

// The camera-relative point on the ray through uv at view depth khdr_d.
float3 KhDlfRel(float2 khdr_uv, float khdr_d)
{
    const float4 khdr_nd = float4(khdr_uv.x * 2.0f - 1.0f, 1.0f - khdr_uv.y * 2.0f,
                                  depthParams.x + depthParams.y / max(khdr_d, 1.0e-3f), 1.0f);
    const float4 khdr_wp = mul(khdr_nd, invViewProj);
    return khdr_wp.xyz / khdr_wp.w;
}
// The full-resolution pixel grid texel t stands for, and its depth key: the view depth there, the sky and everything
// past maxDistance at maxDistance (the march's own end, so equal keys march equal paths; DLF_FOGREC: KhDlfKeyZ moves
// the sky and the far fence's feather band apart under a fog pass drawn after this one). The gather, the filter and the
// composite share both (a texel spans fxMeta.zw / khdf_g pixels).
int2 KhDlfFull(int2 khdf_t, float2 khdf_g)
{
    const int2 khdf_p = int2((float2(khdf_t) + 0.5f) * float2(fxMeta.z, fxMeta.w) / max(khdf_g, 1.0f));
    return clamp(khdf_p, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}
float KhDlfMaxD() { return clamp(fxParams1.y, 10.0f, 5000.0f); }
// DLF_FOGREC: with a fog pass drawn after this one (KhDlfPassMedium's khdm_tz) a texel's glow is scaled for the
// surface it shows, and toward the far fence a pass's factor goes from its ramp to skyAmount (the feather w:
// KhFxFogPasses' 0 at 0.98 of KhEncFence to 1 at 0.999 of it, 1 past 1e8 - the sky), so the key moves with w from
// min(depth, maxDistance) to 4 maxDistance: the sky keys apart from every surface the feather leaves alone, and a
// surface in the feather (a ridge near the far plane) between them, so the filter and the upsample blend glows only
// where their factors are close. Without such a pass the key is min(depth, maxDistance), bit for bit. Not covered:
// covered texels key by the front's depth and take T at the behind; the upsample's 3x3 fallback (no key agreeing)
// compares the keys clamped to maxDistance, ties broken by the unclamped ones.
float KhDlfKeyZ(float khdk_z)
{
    const bool  khdk_af = (khFxFog[0].w > 0.001f && fxParams2.x < 0.5f) ||
                          (khFxFog[1].w > 0.001f && fxParams2.y < 0.5f);
    const float khdk_f = KhEncFence();
    const float khdk_w = khdk_z > 1e8f ? 1.0f
                                       : saturate((min(khdk_z, khdk_f) - khdk_f * 0.98f) / max(khdk_f * 0.019f, 1.0f));
    const float khdk_m = min(khdk_z, KhDlfMaxD());
    return khdk_af ? lerp(khdk_m, 4.0f * KhDlfMaxD(), khdk_w) : khdk_m;
}
float KhDlfKey(int2 khdk_p)
{
    return KhDlfKeyZ(KhLinZ(khdk_p));   // FX_LINZ.
}
// FX_LINZ: KhLinZ(KhDlfFull(t, g)) - KhDlfKey before KhDlfKeyZ, which KhDlfKeyG applies - through
// the grid's copy (t9): drawn with g = its own size, so it serves a reader whose g is that size; any other reads in
// place. Read only by the chain's dynamicLightFog pass (its filter and composite) once its lights are up - the only
// case either runs (dlCtl.x) - when it binds t9 to its copy or to nothing (fx_chain_run), so the size query and
// the load (ahead of the branch, texel 0 when unarmed or past the copy) meet a view of ours or none.
float KhDlfGridZ(int2 khdz_t, float2 khdz_g)
{
    uint khdz_w, khdz_h;
    khGridZD.GetDimensions(khdz_w, khdz_h);
    const bool khdz_in = matCtl.w > 1.5f && khdz_t.x >= 0 && khdz_t.y >= 0 && khdz_t.x < (int)khdz_w &&
                         khdz_t.y < (int)khdz_h && khdz_g.x == (float)khdz_w && khdz_g.y == (float)khdz_h;
    float khdz_z = khGridZD.Load(int3(khdz_in ? khdz_t : int2(0, 0), 0));
    [branch] if (!khdz_in) khdz_z = KhLinZ(KhDlfFull(khdz_t, khdz_g));
    return khdz_z;
}
float KhDlfKeyG(int2 khdk_t, float2 khdk_g)
{
    return KhDlfKeyZ(KhDlfGridZ(khdk_t, khdk_g));   // DLF_FOGREC.
}
// The engine's fog-end ramp on what lies khdn_m metres from the camera (PSMain / PSComposite's: fully fogged past the
// fog end when the engine terms carry one, fogEngine.w in [0.5, 1.5)), a factor on the camera leg's transmittance.
// (KhFxFogEngine applies the same ramp in front of the fog end; past it, it takes the sky's integral, FOG_SKY.)
float KhDlfEndRamp(float khdn_m)
{
    return (fogParams.w >= 0.5f && fogEngine.w >= 0.5f && fogEngine.w < 1.5f)
         ? saturate((fogEngine.y - khdn_m) * fogEngine.z) : 1.0f;
}
// The world fog's extinction (1/m) at absolute height khds_y: the engine's height fog as PSMain / KhFxFogEngine apply
// it (engine terms: fogEngine.x at sea level falling off with fogParams.y; else fogParams.x * 0.0153 up to fogParams.z,
// falling off above it), times fogScale (fxParams0.y), plus baseDensity (fxParams0.z) everywhere. With the fog off the
// game's term is zero; baseDensity still applies.
// TWIN: C++ dlf_medium_zero (DLF_MEDIUM0) reads these terms to skip a pass whose medium is zero - a term
// added here is added there.
float KhDlfSigma(float khds_y)
{
    float khds_s = 0.0f;
    if (fogParams.w >= 0.5f) {
        // FOG_EXP_CAP (composite.hlsl's note): the falloff capped at e^60 under a negative decay.
        khds_s = (fogEngine.w >= 0.5f)
               ? fogEngine.x * exp(min(-fogParams.y * max(khds_y, 0.0f), 60.0f))
               : fogParams.x * 0.0153f * exp(min(-fogParams.y * max(khds_y - fogParams.z, 0.0f), 60.0f));
    }
    return max(khds_s, 0.0f) * max(fxParams0.y, 0.0f) + max(fxParams0.z, 0.0f);
}
// The optical depth of a straight path khdt_d metres long between heights khdt_y0 and khdt_y1 - with the engine terms
// KhFxFogEngine's form (the density at the lower end, decaying along the rise: the mesh fog's own); without them
// KhFxFogEngine takes the far end's density over the whole path, and this integrates the same fallback density instead,
// so the glow's extinction matches the fog it scatters in: flat below its base (fogParams.z), decaying above it.
// DLF_TAU_BASE: the share of the path below the base takes the flat density. Do not use the decaying form alone: it
// would thin a path that dips below the base, and one wholly below it, to as little as a hundredth of its optical depth
// (a valley's lamps would glow through thick fog almost unattenuated). A path wholly above takes the decaying form.
// Scaled and floored as KhDlfSigma. The engine branch is the mesh fog's form and not KhDlfSigma's integral below sea
// level: it takes the density at max(lo, 0) decaying along the whole rise where KhDlfSigma holds it flat below 0, so a
// path that dips below sea level gets less optical depth than the fog it scatters in - at least 1 / (1 + x) of it, x =
// the height decay (fogParams.y) times the depth below 0 the path reaches (a quarter at x = 3, an eighth at 7), the
// bound approached by a long rise from there; no floor as x grows (y >= 0: within 1e-4 relative).
// Neither branch holds the mesh fog's below-layer term (fogBelow, the camera under the layer), nor does KhDlfSigma.
float KhDlfTau(float khdt_d, float khdt_y0, float khdt_y1)
{
    float khdt_w = 0.0f;
    if (fogParams.w >= 0.5f) {
        const float khdt_lo = min(khdt_y0, khdt_y1);
        if (fogEngine.w >= 0.5f) {
            const float khdt_s = fogEngine.x * exp(min(-fogParams.y * max(khdt_lo, 0.0f), 60.0f));   // FOG_EXP_CAP.
            const float khdt_k = fogParams.y * abs(khdt_y1 - khdt_y0);
            const float khdt_i = khdt_k < 1.0e-4f ? khdt_d : khdt_d * (1.0f - exp(-khdt_k)) / khdt_k;
            khdt_w = max(khdt_s, 0.0f) * khdt_i;
        } else {
            // khdt_fb = the path's share below the base (a level path: all or none); the rest rises from
            // max(lo, base) with the decaying form.
            const float khdt_hi = max(khdt_y0, khdt_y1);
            const float khdt_dh = khdt_hi - khdt_lo;
            const float khdt_a = fogParams.x * 0.0153f;
            const float khdt_fb = khdt_dh > 1.0e-4f ? saturate((fogParams.z - khdt_lo) / khdt_dh)
                                                    : (khdt_lo <= fogParams.z ? 1.0f : 0.0f);
            const float khdt_s = khdt_a * exp(min(-fogParams.y * max(khdt_lo - fogParams.z, 0.0f), 60.0f));
            const float khdt_k = fogParams.y * (khdt_hi - max(khdt_lo, fogParams.z));
            const float khdt_da = khdt_d * (1.0f - khdt_fb);   // The path above the base (all of it: khdt_d exactly).
            const float khdt_i = khdt_k < 1.0e-4f ? khdt_da : khdt_da * (1.0f - exp(-khdt_k)) / khdt_k;
            khdt_w = max(khdt_s, 0.0f) * khdt_i + max(khdt_a, 0.0f) * khdt_d * khdt_fb;
        }
    }
    return khdt_w * max(fxParams0.y, 0.0f) + max(fxParams0.z, 0.0f) * khdt_d;
}
// DLF_FOGREC (the chain's own fog passes as a medium for dynamicLightFog, as fog scatter takes them -
// FX_FOGREC): one fog pass's factor f at view depth khdq_z - KhFxFogPasses' ramp, far-fence
// feather and skyAmount rule for one record khdq_e [startDist, endDist, skyAmount, opacity] (TWIN: an edit there is
// made here) - and its derivative in view depth (khdq_df; 0 where saturate holds a term flat, and past 1e8).
float KhDlfPassF(float khdq_z, float4 khdq_e, out float khdq_df)
{
    const float khdq_fence = KhEncFence();
    const float khdq_rd = max(khdq_e.y - khdq_e.x, 1.0f);
    const float khdq_rx = (khdq_z - khdq_e.x) / khdq_rd;
    const float khdq_r = saturate(khdq_rx);
    const float khdq_wd = max(khdq_fence * 0.019f, 1.0f);
    const float khdq_wx = (min(khdq_z, khdq_fence) - khdq_fence * 0.98f) / khdq_wd;
    const float khdq_w = saturate(khdq_wx);
    const float khdq_sky = saturate(khdq_e.z);
    const float khdq_dr = (khdq_rx > 0.0f && khdq_rx < 1.0f) ? 1.0f / khdq_rd : 0.0f;
    const float khdq_dw = (khdq_wx > 0.0f && khdq_wx < 1.0f && khdq_z < khdq_fence) ? 1.0f / khdq_wd : 0.0f;
    const bool  khdq_far = khdq_z > 1e8f;
    khdq_df = khdq_far ? 0.0f : (1.0f - khdq_w) * khdq_dr + (khdq_sky - khdq_r) * khdq_dw;
    return khdq_far ? khdq_sky : lerp(khdq_r, khdq_sky, khdq_w);
}
// DLF_FOGREC: the chain's distance-fog passes as the light fog's medium at a sample of view depth khdm_z (khdm_rl
// metres of ray per unit of view depth), up to two records in khFxFog (the two most opaque fullscreen fog passes, as
// fog scatter takes them; C++ dlf_fog_pack), fxParams2.x / .y = 1 where that record's pass draws before this one. A
// pass fogs what lies at view depth z by a f(z): to the camera it is a medium of transmittance T(z) = 1 - a f(z),
// extinction sigma = a f' / T per unit of view depth (f' held at 0 or more: a factor that falls with depth - the sky's
// feather under the ramp - removes no light). x = the camera leg's transmittance: for a pass drawn BEFORE this one, T
// at the sample (the picture this pass adds to holds its fog already); for one drawn AFTER, which fogs the glow with
// the picture by T at the surface the texel shows (khdm_tz, T at that depth per record, 1 for a pass drawn before or
// none), min(1, T at the sample / that T): where the factor thins with depth (a sky
// under skyAmount below the ramp) the glow is brought to its own T - exact on an uncovered texel - instead of keeping
// the surface's larger one (a hot halo over the sky near endDist); where it thickens, the later pass over-fogs a glow
// before the surface, as any glow in the picture, and nothing is raised (a raise would reach the passes between them
// too). y = the passes' extinction per metre for the light leg, sigma / rl, times fogScale as the game's fog's
// (KhDlfSigma); z = their scattering per metre seen from the camera, fogScale x sigma / rl x x - computed with each
// pass's own factor folded in (a f' / rl for a pass drawn before; min(sigma / rl, a f' / rl / T at the surface) for one
// after), so a pass whose T reaches 0 scatters nothing past it and nothing is divided by 0. (1, 0, 0) with no record.
// TWIN: C++ dlf_medium_zero (DLF_MEDIUM0) reads the record test and fogScale's factor here.
float3 KhDlfPassMedium(float khdm_z, float khdm_rl, float2 khdm_tz)
{
    const float khdm_fs = max(fxParams0.y, 0.0f);
    float  khdm_t[2], khdm_s[2], khdm_k[2];
    bool   khdm_b[2];
    [unroll] for (int khdm_i = 0; khdm_i < 2; ++khdm_i) {
        const float4 khdm_e = khdm_i == 0 ? khFxFog[0] : khFxFog[1];
        const bool   khdm_on = khdm_e.w > 0.001f;   // KhFxFogPasses' record test.
        float khdm_df;
        const float khdm_f = KhDlfPassF(khdm_z, khdm_e, khdm_df);
        const float khdm_a = khdm_on ? saturate(khdm_e.w) : 0.0f;
        khdm_t[khdm_i] = saturate(1.0f - khdm_a * khdm_f);
        khdm_s[khdm_i] = khdm_a * max(khdm_df, 0.0f) / khdm_rl;   // sigma T, per metre.
        khdm_k[khdm_i] = min(khdm_s[khdm_i] / max(khdm_t[khdm_i], 1.0e-30f), 1.0e30f);   // sigma (FOG_EXP_CAP).
        khdm_b[khdm_i] = khdm_on && (khdm_i == 0 ? fxParams2.x : fxParams2.y) > 0.5f;
    }
    // Each record's camera factor (c) and its own scattering with that factor folded in (o).
    const float khdm_q0 = max(khdm_tz.x, 1.0e-30f), khdm_q1 = max(khdm_tz.y, 1.0e-30f);
    const float khdm_c0 = khdm_b[0] ? khdm_t[0] : min(khdm_t[0] / khdm_q0, 1.0f);
    const float khdm_c1 = khdm_b[1] ? khdm_t[1] : min(khdm_t[1] / khdm_q1, 1.0f);
    const float khdm_o0 = khdm_b[0] ? khdm_s[0] : min(khdm_k[0], khdm_s[0] / khdm_q0);
    const float khdm_o1 = khdm_b[1] ? khdm_s[1] : min(khdm_k[1], khdm_s[1] / khdm_q1);
    const float khdm_y = min(khdm_fs * (khdm_k[0] + khdm_k[1]), 1.0e30f);
    return float3(khdm_c0 * khdm_c1, khdm_y, min(khdm_fs * (khdm_o0 * khdm_c1 + khdm_o1 * khdm_c0), 1.0e30f));
}
// Henyey-Greenstein at cos khdp_c (the light's travel against the view ray's reverse: 1 looking into the light),
// times pi: the engine's surfaces show E * albedo, so the fog's radiance is the integral of sigma E p pi ds, and an
// isotropic medium (g = 0) scatters a quarter of E per unit optical depth. g = anisotropy (fxParams0.w).
float KhDlfPhase(float khdp_c)
{
    const float khdp_g = clamp(fxParams0.w, -0.9f, 0.9f);
    const float khdp_g2 = khdp_g * khdp_g;
    const float khdp_b = max(1.0f + khdp_g2 - 2.0f * khdp_g * khdp_c, 1.0e-4f);
    return (1.0f - khdp_g2) / (4.0f * khdp_b * sqrt(khdp_b));
}
// A light's attenuation at distance khda_dist, direction khda_l (point to light, normalised): DynLights' terms
// (cb.hlsl) - the start offset and the constant / linear / quadratic falloff of [4], the hard fade of [5], and for a
// spot the cone of [1] / [2] and its exponent [3].w. TWIN (five copies): DynLights, KhDynLightsPBRE and
// KhDlsWorldFactor's loops (cb.hlsl), dls_atten (C++, the distance terms alone).
float KhDlfAtt(float4 khda_r1, float4 khda_r2, float4 khda_r3, float4 khda_r4, float4 khda_r5,
               float khda_dist, float3 khda_l, bool khda_spot)
{
    const float khda_d = max(khda_dist * dlCtl.w - khda_r4.x, 0.0f);
    float khda_a = saturate(1.0f / (dot(khda_r4.yzw, float3(1.0f, khda_d, khda_d * khda_d)) + 1e-4f));
    khda_a *= 1.0f - saturate((khda_dist * dlCtl.w - khda_r5.x) * khda_r5.y);
    if (khda_spot) {
        const float khda_c = saturate((dot(-khda_r1.xyz, khda_l) - khda_r1.w) * khda_r2.w);
        khda_a *= (khda_c > 0.0f) ? pow(khda_c, khda_r3.w) : 0.0f;   // DynLights' guard (pow(0, 0)).
    }
    return khda_a;
}
// The distance at which a light's attenuation (KhDlfAtt without fade or cone) falls to half its peak: the radius of its
// bright core, which the march's equiangular samples crowd toward (KhDlfEquiInit). For a start offset of 0 or more the
// peak is 1 / max(c, 1) (the attenuation saturates at 1), so the half is where c + l x + q x^2 = 2 max(c, 1). A
// negative start ([4].x < 0: x never falls below -start) is not modelled - the distance comes out where the attenuation
// is anywhere from ~0.17 to nearly all of its value at the start (0.5 just below 0; for a constant term under 1 it dips
// to ~0.17 where the attenuation stops saturating, then rises toward all of it the further below 0 the start); only the
// equiangular width takes it, so the estimate's noise moves, not its mean (KhDlfMisStep is unbiased for any width).
// Zero when the terms give none.
float KhDlfCore(float4 khdo_r4)
{
    const float khdo_rhs = max(khdo_r4.y, 2.0f - khdo_r4.y);   // l x + q x^2 = it, x = dist * scale - start.
    const float khdo_den = khdo_r4.z + sqrt(max(khdo_r4.z * khdo_r4.z + 4.0f * khdo_r4.w * khdo_rhs, 0.0f));
    const float khdo_x = (khdo_rhs > 0.0f && khdo_den > 1.0e-12f) ? 2.0f * khdo_rhs / khdo_den : 0.0f;
    return max((max(khdo_r4.x, 0.0f) + max(khdo_x, 0.0f)) / max(dlCtl.w, 1.0e-6f), 0.0f);
}
// The march's samples along [t0, t1] (metres along a ray), two stratified sets combined by the balance heuristic
// (multiple importance sampling): the first (n + 1) / 2 equiangular about the point khde_tc nearest a light - spread
// evenly in the angle they subtend from a point khde_w off the ray, so they crowd where a light falling off as the
// square of distance is brightest - and the rest evenly spaced, which serve a light that falls off slower, whose far
// samples alone would carry too much weight. Each sample's weight is 1 / (n_e p_e + n_u p_u) at its t: the estimate
// is unbiased for any khde_w > 0, and never much noisier than the better of the two sets. Init returns (first angle,
// angle span, tc, w); Step returns (t, weight) for sample khds_k of khds_n at stratum jitter khds_j.
float4 KhDlfEquiInit(float khde_t0, float khde_t1, float khde_tc, float khde_w)
{
    const float khde_a0 = atan((khde_t0 - khde_tc) / khde_w);
    const float khde_a1 = atan((khde_t1 - khde_tc) / khde_w);
    return float4(khde_a0, max(khde_a1 - khde_a0, 0.0f), khde_tc, khde_w);
}
float2 KhDlfMisStep(float4 khds_e, float khds_t0, float khds_t1, int khds_k, int khds_n, float khds_j)
{
    const int   khds_ne = (int)((uint)(khds_n + 1) / 2u);   // UDIV: as uint (khds_n is 4 - 64) - fxc's X3556.
    const int   khds_nu = (int)((uint)khds_n / 2u);
    const float khds_len = max(khds_t1 - khds_t0, 1.0e-6f);
    const bool  khds_eq = khds_k < khds_ne;
    const float khds_te = khds_e.z + khds_e.w * tan(khds_e.x + ((float)khds_k + khds_j) / (float)khds_ne * khds_e.y);
    const float khds_tu = khds_t0 + ((float)(khds_k - khds_ne) + khds_j) / (float)max(khds_nu, 1) * khds_len;
    const float khds_t = khds_eq ? khds_te : khds_tu;
    const float khds_o = khds_t - khds_e.z;
    // n_e p_e, with p_e = w / (span (w^2 + o^2)); a zero span (a sliver) gives no equiangular density.
    const float khds_pe = (khds_e.y > 0.0f)
                        ? (float)khds_ne * khds_e.w / (khds_e.y * (khds_e.w * khds_e.w + khds_o * khds_o)) : 0.0f;
    const float khds_pu = (float)khds_nu / khds_len;
    return float2(khds_t, 1.0f / max(khds_pe + khds_pu, 1.0e-20f));
}
// A spot's forward cone (axis khdc_a, cosine cut khdc_cs > 0, apex at the origin) clipping [s0, s1] of t along
// P(t) = khdc_o + khdc_d * t, with khdc_o the ray's point nearest the apex, seen from it (KhDlfCone's recentring): the
// half-space in front of the apex, then the double cone's quadratic. A convex cone meets a line once, so one interval
// survives; where the quadratic leaves two pieces the one in the forward nappe is the longer (the other lies behind
// the apex, which the half-space already cut to at most a sliver). False = nothing left.
bool KhDlfConeT(float3 khdc_d, float3 khdc_o, float3 khdc_a, float khdc_cs, inout float khdc_s0, inout float khdc_s1)
{
    const float khdc_ad = dot(khdc_a, khdc_d);
    const float khdc_ao = dot(khdc_a, khdc_o);
    if (abs(khdc_ad) < 1.0e-7f) {
        if (khdc_ao < 0.0f) return false;
    } else {
        const float khdc_h = -khdc_ao / khdc_ad;
        if (khdc_ad > 0.0f) khdc_s0 = max(khdc_s0, khdc_h);
        else                khdc_s1 = min(khdc_s1, khdc_h);
    }
    if (!(khdc_s1 > khdc_s0)) return false;
    const float khdc_c2 = khdc_cs * khdc_cs;
    const float khdc_qa = khdc_ad * khdc_ad - khdc_c2 * dot(khdc_d, khdc_d);
    const float khdc_qb = 2.0f * (khdc_ad * khdc_ao - khdc_c2 * dot(khdc_d, khdc_o));
    const float khdc_qc = khdc_ao * khdc_ao - khdc_c2 * dot(khdc_o, khdc_o);
    // The roots in the stable form - q = -(qb + sign(qb) sq) / 2, roots q / qa and qc / q: a ray near the cut's own
    // angle (a generator's direction) has qa at float noise, where the textbook pair cancels catastrophically and
    // loses the near root (the one that clips), while this keeps it and sends the other far. Only a qa that is
    // negligible even against that (relative: its terms are of size |d|^2) takes the linear case.
    if (abs(khdc_qa) <= 1.0e-10f * dot(khdc_d, khdc_d)) {
        if (abs(khdc_qb) < 1.0e-8f) return khdc_qc >= 0.0f;
        const float khdc_r = -khdc_qc / khdc_qb;
        if (khdc_qb > 0.0f) khdc_s0 = max(khdc_s0, khdc_r);
        else                khdc_s1 = min(khdc_s1, khdc_r);
        return khdc_s1 > khdc_s0;
    }
    const float khdc_disc = khdc_qb * khdc_qb - 4.0f * khdc_qa * khdc_qc;
    // disc > 0 below makes sq > 0, so q is never 0 where the roots are used.
    const float khdc_sq = sqrt(max(khdc_disc, 0.0f));
    const float khdc_q = -0.5f * (khdc_qb + (khdc_qb >= 0.0f ? khdc_sq : -khdc_sq));
    const float khdc_ra = khdc_q / khdc_qa;
    const float khdc_rb = khdc_qc / (abs(khdc_q) > 0.0f ? khdc_q : 1.0f);
    const float khdc_r1 = min(khdc_ra, khdc_rb);
    const float khdc_r2 = max(khdc_ra, khdc_rb);
    if (khdc_qa < 0.0f) {   // Inside between the roots.
        if (khdc_disc <= 0.0f) return false;
        khdc_s0 = max(khdc_s0, khdc_r1);
        khdc_s1 = min(khdc_s1, khdc_r2);
        return khdc_s1 > khdc_s0;
    }
    if (khdc_disc <= 0.0f) return true;   // Inside everywhere (the half-space picked the nappe).
    const float khdc_la = min(khdc_s1, khdc_r1) - khdc_s0;
    const float khdc_lb = khdc_s1 - max(khdc_s0, khdc_r2);
    if (khdc_la <= 0.0f && khdc_lb <= 0.0f) return false;
    if (khdc_la >= khdc_lb) khdc_s1 = min(khdc_s1, khdc_r1);
    else                    khdc_s0 = max(khdc_s0, khdc_r2);
    return true;
}
// The cone clip of [s0, s1] along P(s) = khdc_d * s (s = view depth, the camera at the origin). The quadratic is
// solved about the ray's point nearest the apex: from the camera its constant term is a difference of squares of the
// light's distance, and cancels to float noise that splits the double root of a ray through the lamp by decimetres.
bool KhDlfCone(float3 khdc_d, float3 khdc_lp, float3 khdc_a, float khdc_cs, inout float khdc_s0, inout float khdc_s1)
{
    const float khdc_sc = dot(khdc_d, khdc_lp) / max(dot(khdc_d, khdc_d), 1.0e-12f);
    float khdc_t0 = khdc_s0 - khdc_sc;
    float khdc_t1 = khdc_s1 - khdc_sc;
    const bool khdc_in = KhDlfConeT(khdc_d, khdc_d * khdc_sc - khdc_lp, khdc_a, khdc_cs, khdc_t0, khdc_t1);
    khdc_s0 = khdc_t0 + khdc_sc;
    khdc_s1 = khdc_t1 + khdc_sc;
    return khdc_in;
}
// Our meshes' shadow of light map slot khdv_s at absolute point khdv_p: KhDlsShadow's lookup (the face, the bias - the
// slot's constant and slope plus one texel, taken here always, where KhDlsShadow adds it only with its normal offset
// on, dlsFaceSlice[slot * 6].y - and the range fade) as a single nearest compare: a fog sample has no surface, so no
// normal offset and no receiver-plane gradient, and the march and the grid filter do the smoothing. 1 = lit. TWIN:
// KhDlsShadow (cb.hlsl).
float KhDlfDlsVis(int khdv_s, float3 khdv_p)
{
    if (khdv_s < 0 || khdv_s > 7) return 1.0f;
    const float4 khdv_m = dlsMeta[khdv_s];
    if (khdv_m.w <= 0.0f) return 1.0f;
    const float khdv_rf = KhDlsRangeFade(khdv_p);
    if (khdv_rf <= 0.0f) return 1.0f;   // KhDlsShadow's early out: past the maps' range nothing is shadowed.
    uint khdv_w, khdv_h, khdv_n;
    khDlsMaps.GetDimensions(khdv_w, khdv_h, khdv_n);
    if (khdv_w < 2u) return 1.0f;
    const float3 khdv_p0 = khdv_p - khdv_m.xyz;
    const float  khdv_far = khdv_m.w;
    const float  khdv_near = KhDlsNear(khdv_far);
    float2 khdv_uv;
    float  khdv_z, khdv_slice, khdv_sx;
    float3 khdv_fr, khdv_fu;
    if (!KhDlsFaceUV(khdv_s, khdv_p0, khdv_p0, khdv_near, khdv_far, khdv_uv, khdv_z, khdv_slice,
                     khdv_fr, khdv_fu, khdv_sx)) return 1.0f;
    const float khdv_texel = 2.0f * khdv_z / ((float)khdv_w * max(khdv_sx, 1.0e-3f));
    const float khdv_b = dlsCtl[khdv_s].y + dlsCtl[khdv_s].w * khdv_z + khdv_texel;
    const float khdv_a = khdv_far / (khdv_far - khdv_near);
    const float khdv_c = -khdv_near * khdv_far / (khdv_far - khdv_near);
    const float khdv_ref = khdv_a + khdv_c / max(khdv_z - khdv_b, khdv_near);
    const int2  khdv_t = clamp(int2(khdv_uv * (float)khdv_w), int2(0, 0), int2((int)khdv_w - 1, (int)khdv_w - 1));
    const float khdv_st = khDlsMaps.Load(int4(khdv_t, (int)khdv_slice, 0));
    return (khdv_ref > khdv_st) ? saturate(1.0f - khdv_rf) : 1.0f;
}
// One point of KhDlfSsVis's segment, t of the way from khsp_q along khsp_v, against the depth buffer: x = how far it
// lies behind the visible surface at its pixel (negative: in front), y = that surface's view depth, z = 1; all 0
// when there is nothing to compare (behind the camera or off the screen). khsp_f / khsp_r / khsp_u: the view axis and
// the right and up half-extents at view depth 1; khsp_n: their squared lengths.
float3 KhDlfSsProbe(float3 khsp_q, float3 khsp_v, float khsp_t, float3 khsp_f, float3 khsp_r, float3 khsp_u,
                    float3 khsp_n)
{
    const float3 khsp_w = khsp_q + khsp_v * khsp_t;
    const float  khsp_vz = dot(khsp_w, khsp_f) / khsp_n.x;
    if (khsp_vz <= 0.05f) return float3(0.0f, 0.0f, 0.0f);
    const float2 khsp_c = float2(dot(khsp_w, khsp_r) / (khsp_n.y * khsp_vz),
                                 dot(khsp_w, khsp_u) / (khsp_n.z * khsp_vz));
    if (abs(khsp_c.x) >= 1.0f || abs(khsp_c.y) >= 1.0f) return float3(0.0f, 0.0f, 0.0f);
    const int2 khsp_p = clamp(int2((khsp_c * float2(0.5f, -0.5f) + 0.5f) * float2(fxMeta.z, fxMeta.w)),
                              int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    const float khsp_sd = KhLinZ(khsp_p);   // FX_LINZ.
    return float3(khsp_vz - khsp_sd, khsp_sd, 1.0f);
}
// Whether a probe lies behind the visible surface, past a bias of 5 cm and a share of its depth (the sample's own
// surface never counts).
bool KhDlfSsHidden(float3 khsh_p)
{
    return khsh_p.x > 0.05f + KH_DLF_SS_BIAS_K * (khsh_p.x + khsh_p.y);
}
// DLF_SS: whether the camera's view of the light itself is covered - its pixel shows a surface more
// than KH_DLF_SS_SELF nearer than it (nearer by less: its own housing). KhDlfSsVis's khsv_cov, the gate of its plane
// continuation; the gather works it out once per light per texel. Off the screen or behind the camera: not covered.
bool KhDlfSsCovered(float3 khsc_lp, float3 khsc_f, float3 khsc_r, float3 khsc_u)
{
    const float3 khsc_n = float3(max(dot(khsc_f, khsc_f), 1.0e-8f), max(dot(khsc_r, khsc_r), 1.0e-8f),
                                 max(dot(khsc_u, khsc_u), 1.0e-8f));
    const float3 khsc_p = KhDlfSsProbe(float3(0.0f, 0.0f, 0.0f), khsc_lp, 1.0f, khsc_f, khsc_r, khsc_u, khsc_n);
    return khsc_p.z > 0.5f && khsc_p.x > KH_DLF_SS_SELF;
}
// The screen-space occlusion of the segment from fog sample khsv_q to the light at khsv_lp (both camera-relative) by
// the geometry the camera sees. KH_DLF_SS_TAPS points along it (jittered by khsv_j and spaced so the last is the
// segment's end, KH_DLF_SS_KEEP short of the light) are compared with the depth buffer. Wherever the segment goes
// from in front of the visible surface to behind it, or back, between two of them, that interval is halved
// KH_DLF_SS_REFINE times onto the change; the sample itself comes before the first, in front by construction of the
// surface its own pixel shows, at view depth khsv_sd0. If the visible depth is continuous at the change (it moves
// by less than KH_DLF_SS_THICK plus KH_DLF_SS_THICK_K of the depth), the segment passed through that surface:
// occluded, unless the crossing lies within KH_DLF_SS_SELF of the light (its own body). If the visible depth jumps
// there, the segment only slipped behind a silhouette, or out from behind one - a post, a window's frame, a car's
// body in front of the light - and nothing is concluded. A point off the screen or behind the camera carries no
// information. 1 = lit.
// DLF_SS - the plane continuation, gated by khsv_cov (KhDlfSsCovered: the light itself is hidden behind a surface
// more than KH_DLF_SS_SELF nearer than it). A jump to a NEARER surface from in front of a farther one is the segment
// slipping behind something that stands before the surface it is about to cross - a soldier before the wall of the room
// the light is in: the crossing lies at pixels he covers, and without this the sample stays lit, his surroundings glow
// with a light the wall hides. With the light covered, and the far surface STANDING (one probe KH_DLF_SS_RISE straight
// up from its point still shows it at that depth: a wall; the ground - the lamp behind him over open ground - a roof or
// a ceiling recede from there and are left alone), the far surface is taken to go on behind the nearer one at its view
// depth: where the segment reaches that depth, short of the light, one probe reads the pixel there - the nearer surface
// still covering it, or the far one itself showing within KH_DLF_SS_THICK plus KH_DLF_SS_THICK_K of that depth, is the
// crossing (occluded, under the same KH_DLF_SS_SELF rule); something farther (the far surface ended: a doorway, the
// sky) concludes nothing. The continuation only adds `return 0`: every crossing found without it is still found, and
// with khsv_cov false (the light seen, or off the screen) nothing here runs. Limits: a wall that ENDS behind the nearer
// one - its corner, with the light beyond it - casts a false shadow around the nearer one's silhouette, only while the
// light itself is hidden; a wall whose top lies within KH_DLF_SS_RISE of the crossing is not continued (it leaks
// there); a light within KH_DLF_SS_SELF of its wall still leaks (the housing rule); the gate is hard, so the glow
// switches as the nearer one's edge crosses the light's pixel.
float KhDlfSsVis(float3 khsv_q, float3 khsv_lp, float3 khsv_f, float3 khsv_r, float3 khsv_u, float khsv_j,
                 float khsv_sd0, bool khsv_cov)
{
    const float3 khsv_v = khsv_lp - khsv_q;
    const float  khsv_len = length(khsv_v);
    const float  khsv_span = khsv_len - KH_DLF_SS_KEEP;
    if (khsv_span <= 0.0f) return 1.0f;
    const float3 khsv_n = float3(max(dot(khsv_f, khsv_f), 1.0e-8f), max(dot(khsv_r, khsv_r), 1.0e-8f),
                                 max(dot(khsv_u, khsv_u), 1.0e-8f));
    float  khsv_pt = 0.0f;                          // The previous point: first the sample, in front.
    float3 khsv_pp = float3(-1.0f, khsv_sd0, 1.0f);
    [loop] for (int khsv_k = 0; khsv_k < KH_DLF_SS_TAPS; ++khsv_k) {
        const float  khsv_t = ((float)khsv_k + khsv_j) / ((float)(KH_DLF_SS_TAPS - 1) + khsv_j)
                            * (khsv_span / khsv_len);
        const float3 khsv_p = KhDlfSsProbe(khsv_q, khsv_v, khsv_t, khsv_f, khsv_r, khsv_u, khsv_n);
        const bool   khsv_ha = KhDlfSsHidden(khsv_pp);
        [branch] if (khsv_p.z > 0.5f && khsv_pp.z > 0.5f && KhDlfSsHidden(khsv_p) != khsv_ha) {
            float  khsv_a = khsv_pt;
            float  khsv_b = khsv_t;
            float3 khsv_pa = khsv_pp;
            float3 khsv_pb = khsv_p;
            bool   khsv_known = true;
            [loop] for (int khsv_i = 0; khsv_i < KH_DLF_SS_REFINE; ++khsv_i) {
                const float  khsv_m = 0.5f * (khsv_a + khsv_b);
                const float3 khsv_pm = KhDlfSsProbe(khsv_q, khsv_v, khsv_m, khsv_f, khsv_r, khsv_u, khsv_n);
                if (khsv_pm.z < 0.5f) {
                    khsv_known = false;
                    break;
                }
                if (KhDlfSsHidden(khsv_pm) == khsv_ha) {
                    khsv_a = khsv_m;
                    khsv_pa = khsv_pm;
                } else {
                    khsv_b = khsv_m;
                    khsv_pb = khsv_pm;
                }
            }
            const float  khsv_sm = max(khsv_pa.y, khsv_pb.y);
            const float3 khsv_x = khsv_q + khsv_v * (0.5f * (khsv_a + khsv_b));   // The crossing.
            if (khsv_known && abs(khsv_pa.y - khsv_pb.y) < KH_DLF_SS_THICK + KH_DLF_SS_THICK_K * khsv_sm &&
                length(khsv_x - khsv_lp) >= KH_DLF_SS_SELF) return 0.0f;
            // DLF_SS: the plane continuation (the note above). The far surface's depth, khsv_ext, is
            // pa's (the side the segment was in front of); the segment's view depth is linear in t, so the point
            // where it reaches khsv_ext is exact - clamped at a (within the bias the crossing is at a itself) and at
            // the light, and only where the light lies past that depth (the segment going deeper). The probe there
            // sees either side's surface (within THICK of khsv_ext, or nearer: still covered) - the crossing. First
            // the far surface must stand: its point at a (pa's pixel's ray at depth khsv_ext - the frame is world
            // axes, y up) raised KH_DLF_SS_RISE is probed; a wall shows it there at that depth, the ground shows
            // farther ground or the sky (in front: x well below 0), a roof or ceiling something nearer (x above 0).
            [branch] if (khsv_cov && khsv_known && !khsv_ha &&
                         khsv_pb.y < khsv_pa.y - (KH_DLF_SS_THICK + KH_DLF_SS_THICK_K * khsv_sm)) {
                const float khsv_ext = khsv_pa.y;
                const float khsv_qz = dot(khsv_q, khsv_f) / khsv_n.x;   // The sample's view depth...
                const float khsv_dz = dot(khsv_v, khsv_f) / khsv_n.x;   // ... and the segment's change of it.
                [branch] if (khsv_dz > 1.0e-4f && khsv_qz + khsv_dz > khsv_ext) {
                    const float  khsv_za = max(khsv_qz + khsv_a * khsv_dz, 0.05f);   // The view depth at a.
                    const float3 khsv_sa = (khsv_q + khsv_v * khsv_a) * (khsv_ext / khsv_za);   // The surface at a.
                    const float3 khsv_pr = KhDlfSsProbe(khsv_sa, float3(0.0f, KH_DLF_SS_RISE, 0.0f), 1.0f, khsv_f,
                                                        khsv_r, khsv_u, khsv_n);
                    const bool   khsv_st = khsv_pr.z > 0.5f &&
                                           abs(khsv_pr.x) < 0.1f * KH_DLF_SS_RISE + KH_DLF_SS_THICK_K * khsv_ext;
                    const float  khsv_tx = clamp((khsv_ext - khsv_qz) / khsv_dz, khsv_a, 1.0f);
                    const float3 khsv_px = KhDlfSsProbe(khsv_q, khsv_v, khsv_tx, khsv_f, khsv_r, khsv_u, khsv_n);
                    const float3 khsv_xx = khsv_q + khsv_v * khsv_tx;   // The crossing of the continued surface.
                    if (khsv_st && khsv_px.z > 0.5f &&
                        khsv_px.y < khsv_ext + (KH_DLF_SS_THICK + KH_DLF_SS_THICK_K * khsv_ext) &&
                        length(khsv_xx - khsv_lp) >= KH_DLF_SS_SELF) return 0.0f;
                }
            }
        }
        khsv_pt = khsv_t;
        khsv_pp = khsv_p;
    }
    return 1.0f;
}

// SSGI_VB - the SSGI gather's visibility (effect 22: PSSsgiGather): screen-space indirect light with visibility
// bitmasks (Therrien, Levesque and Gilet 2023; the method of Pascal Gilcher's iMMERSE MXAO). The receiver's hemisphere
// is cut into slices through its view ray; along each, the samples march outward from the pixel. A sample stands for a
// solid of thickness KH_SSGI_THICK behind its visible face (along its own view ray): seen from the receiver it spans
// the angles from its front to its back, and it occupies the slice's sectors over that span. Its light reaches the
// receiver through the sectors it occupies that no nearer sample already held - so a blocker in front of it shadows
// it where it truly covers it, light passes behind a thin object, and an object in front of the receiver's view ray
// (nearer the camera) blocks only the few sectors it spans. A sample stands for its stretch of the line (half the gap
// to each neighbour), laid on its own surface's plane, so the samples tile the line and the coverage thins less with a
// smaller budget. The 32 sectors of a slice split its N.L lobe about the normal's projection evenly (KhSsgiCum; the
// bias sharpens the lobe), so a sector's share is its N.L-weighted share; a slice weighs its normal's projected length
// times its lobe's integral, and the sum is normalized by the bias-free total, so a higher bias dims the result (bias
// 0.9: about a seventh of 0.15's).
static const float KH_SSGI_THICK   = 1.0f;   // m.
static const float KH_SSGI_VB_GAIN = 0.33f;   // The mean onto the former gather's (open scenes, 12).
// Brightness is kept independent of the sample budget: fewer samples still cover fewer sectors (a sender on an edge is
// a point, not a stretch; the thickness's ends), measured as a mean rising with n about as (n / 12)^KH_SSGI_VB_NEXP
// (within about 15 % over 4 - 32 samples); the result is scaled by its inverse, so the budget trades noise, not
// brightness.
static const float KH_SSGI_VB_NEXP = 0.21f;
// A slice's cumulative weight at angle khsc_a (radians from the normal's projection) for a bias khsc_b: the integral of
// the biased N.L lobe (cos a - b) / (1 - b) (b = 0: the plain cosine), times (1 - b) (the gather divides the slice's
// total by it). It rises monotonically over [-acos b, acos b], so it is the sectors' distribution function. The solid
// angle's own |sin| about the view ray (GTAO's slice measure) is left out, as MXAO's bitmask mode leaves it: against a
// ray-traced reference it fitted worse overall in the scenes tried.
float KhSsgiCum(float khsc_a, float khsc_b)
{
    return sin(khsc_a) - khsc_b * khsc_a;
}
// The sectors of 32 that a sample spans: khsb_a = the angles (radians from the normal's projection, in the slice) of
// its footprint's two ends and their backs; khsb_b = the bias; khsb_hw = the admitted half-width about the normal
// (acos bias); khsb_c0 / khsb_cr = the slice's cumulative weight at -khsb_hw and over the whole cone (KhSsgiCum). The
// span is unwrapped the short way from the first angle (a sample's corners lie close in angle), so a sample behind
// the receiver cannot wrap round into it; the edges are rounded to the nearest sector.
uint KhSsgiBits(float4 khsb_a, float khsb_b, float khsb_hw, float khsb_c0, float khsb_cr)
{
    const float  khsb_w0 = khsb_a.x - 6.2831853f * round(khsb_a.x * 0.15915494f);
    const float3 khsb_dd = khsb_a.yzw - khsb_a.xxx;
    const float3 khsb_w = khsb_w0 + (khsb_dd - 6.2831853f * round(khsb_dd * 0.15915494f));
    const float  khsb_lo = clamp(min(khsb_w0, min(khsb_w.x, min(khsb_w.y, khsb_w.z))), -khsb_hw, khsb_hw);
    const float  khsb_hi = clamp(max(khsb_w0, max(khsb_w.x, max(khsb_w.y, khsb_w.z))), -khsb_hw, khsb_hw);
    const float  khsb_k = 1.0f / max(khsb_cr, 1.0e-6f);
    const uint   khsb_b0 = (uint)round(saturate((KhSsgiCum(khsb_lo, khsb_b) - khsb_c0) * khsb_k) * 32.0f);
    const uint   khsb_b1 = (uint)round(saturate((KhSsgiCum(khsb_hi, khsb_b) - khsb_c0) * khsb_k) * 32.0f);
    if (khsb_b1 <= khsb_b0) return 0u;
    const uint khsb_n = khsb_b1 - khsb_b0;
    return (khsb_n >= 32u ? 0xFFFFFFFFu : ((1u << khsb_n) - 1u)) << khsb_b0;
}

// WSEE (cb.hlsl's note): two of the depth-derived quantities of PSEffect's own that a covered pixel takes at both
// depths (fog scatter's share is another: KhFsFogW, above), each from a view depth z (LinDepth's) - the effect's body
// after its depth read, so the front is today's.
// Distance fog's factor (effect 13): the ramp, the far fence's feather toward skyAmount, the sky's skyAmount. Twin
// edit at cb.hlsl's KhFxFogPasses (KhFsFog's and KhFxFogAt's fog-pass loop).
float KhFxDistFogF(float d)
{
    float f = saturate((d - fxParams0.x) / max(fxParams0.y - fxParams0.x, 1.0f));
    float khfg_f = KhEncFence();
    float khfg_w = saturate((min(d, khfg_f) - khfg_f * 0.98f) / max(khfg_f * 0.019f, 1.0f));
    f = lerp(f, saturate(fxParams0.z), khfg_w);
    if (d > 1e8f) f = saturate(fxParams0.z);   // Sky/far-plane pixels (belt; the feather
                                               // already lands here).
    return f;
}
// WSEE: the distance fog's factor for a stage's layers' own light by layer group - the mean of the factor over the
// stage's groups (raw depth khws_z, transmittance khws_t; up to four, sorted here - a stable order), each weighted by
// its share of the stage's opacity front to back, (1 - T_i) prod_{j<i} T_j, and taken at its own depth (khws_qb, the
// behind's factor, at or past the behind khws_b). A group with T >= 1 weighs nothing; khws_d where none weighs.
float KhWseeFogMean(float4 khws_z, float4 khws_t, float khws_b, float khws_qb, float khws_d)
{
    float khws_zz[4] = { khws_z.x, khws_z.y, khws_z.z, khws_z.w };
    float khws_tt[4] = { khws_t.x, khws_t.y, khws_t.z, khws_t.w };
    [unroll] for (int khws_p = 0; khws_p < 3; ++khws_p) {
        [unroll] for (int khws_j = 0; khws_j < 3 - khws_p; ++khws_j) {
            if (khws_zz[khws_j] > khws_zz[khws_j + 1]) {
                const float khws_sz = khws_zz[khws_j], khws_st = khws_tt[khws_j];
                khws_zz[khws_j] = khws_zz[khws_j + 1];
                khws_tt[khws_j] = khws_tt[khws_j + 1];
                khws_zz[khws_j + 1] = khws_sz;
                khws_tt[khws_j + 1] = khws_st;
            }
        }
    }
    float khws_a = 0.0f, khws_w = 0.0f, khws_g = 1.0f;
    [unroll] for (int khws_i = 0; khws_i < 4; ++khws_i) {
        [branch] if (khws_tt[khws_i] < 1.0f) {
            const float khws_wi = khws_g * (1.0f - khws_tt[khws_i]);
            const float khws_f = khws_zz[khws_i] >= khws_b ? khws_qb : KhFxDistFogF(LinDepth(khws_zz[khws_i]));
            khws_a += khws_wi * khws_f;
            khws_w += khws_wi;
            khws_g *= khws_tt[khws_i];
        }
    }
    return khws_w > 0.0f ? khws_a / khws_w : khws_d;
}
// The same over every group of the last covered verdict (up to seven, sorted: KhWseeGroupT7 / Z7).
float KhWseeFogMeanAll(float khws_b, float khws_qb, float khws_d)
{
    float khws_a = 0.0f, khws_w = 0.0f, khws_g = 1.0f;
    [loop] for (uint khws_i = 0u; khws_i < 7u; ++khws_i) {
        const float khws_ti = KhWseeGroupT7(khws_i);
        [branch] if (khws_ti < 1.0f) {
            const float khws_zi = KhWseeGroupZ7(khws_i);
            const float khws_wi = khws_g * (1.0f - khws_ti);
            khws_a += khws_wi * (khws_zi >= khws_b ? khws_qb : KhFxDistFogF(LinDepth(khws_zi)));
            khws_w += khws_wi;
            khws_g *= khws_ti;
        }
    }
    return khws_w > 0.0f ? khws_a / khws_w : khws_d;
}
// WSEE_STAGE_MEAN: a stage's front factor without the layer-depth split - the mean over the stage's groups as they
// stand in the record: the fence-type layers at the live depth khws_r (Tw) and the particle-type phase at its nearest
// khws_n (its T), by their shares of the stage's opacity front to back (KhWseeFogMean). Do not take the factor at
// min(r, NWpost) alone: right for one layer, it is wrong where an opaque-like fence-type layer (a sea surface recorded
// as one, foliage) stands far behind a faint near one - all of the stage's light, nearly all of it the far layer's, is
// fogged as the near one (a rain streak before a fogged sea cuts the fog out). With one group the mean is that group's
// own factor: at r for the fence-type layers alone (min(r, NWpost) with no particle layer), at NWpost for the particle
// phase alone (clamped to r, as min(r, NWpost) puts it). The split (khWsSp) takes its own richer mean (KhWseeFogMean
// over the bands).
float KhWseeStageMean(float khws_r, float khws_tr, float khws_n, float khws_tn, float khws_b, float khws_qb, float khws_d)
{
    return KhWseeFogMean(float4(khws_r, min(khws_n, khws_r), khws_b, khws_b), float4(khws_tr, khws_tn, 1.0f, 1.0f),
                         khws_b, khws_qb, khws_d);
}
// Pulse's ring (effect 11) at view depth z: khpl_c = its centre relative to the camera (LOCAL_REL). Fenced
// reconstruction + the fence feather keep the ring continuous: sky (clamped to the fence) lands at feather zero.
float KhFxPulseRing(float3 khpl_c, float2 uv, float z)
{
    float khpl_d;
    float dist = distance(KhWorldPosRelAt(uv, z, khpl_d), khpl_c);
    float band = max(fxParams1.x, 0.01f);
    float ring = 1.0f - saturate(abs(dist - fxParams0.w) / band);
    ring *= ring;
    float khpl_f = KhEncFence();
    ring *= 1.0f - saturate((khpl_d - khpl_f * 0.98f) / max(khpl_f * 0.019f, 1.0f));
    return ring;
}

// OL_D2 (outline, effect 10): w = 1 / z at p, and one axis's depth edge as the second difference of w, which is 0
// on any plane. KhOlAxis works along unit axis khoa_a from khoa_c (khoa_p its coordinate along the axis, khoa_hi the
// last index, khoa_d the step), in frame pixels, or (khoa_t, the UI lane) in depth texels: there the depth is at the
// 3D render's size, and frame-pixel taps mapped through KhDepthPx land 1 and 2 texels apart at a Sampling that does
// not divide the step - their second difference is then one texel's slope, not 0, so the taps step in texels.
// It counts only where the forward segment holds the break (its jump at least the other segment's), the pixels a
// forward first difference of z marks (its tap clamped to the frame, as LoadDepthPS needs), so a line keeps that
// width and side, at the borders too. Centred (taps -d, 0, +d) inside; within d of the start (no -d tap) the
// one-sided form into the frame (0, +d, +2d); within d of the end the forward tap is the frame's last pixel, s =
// hi - p < d away, with the slopes compared at their spacings: (w(+s) - w0) - (s / d)(w0 - w(-d)) - a backward
// one-sided form (0, -d, -2d) would miss a break between p and the edge, which the clamped forward tap draws. A pixel
// that fits none of the three - p < d with p + 2d > hi, which only a frame under 3d pixels along the axis has - gives
// 0 (a line there can lose all d of its pixels). khoa_wx = w at the tap across the break (the forward one; w0 where
// nothing counts).
float KhOlW(int2 khow_p) { return 1.0f / max(LinDepth(LoadDepthPS(khow_p)), 1.0e-3f); }
// The raw depth at depth texel khor_t, with no pixel map (KhDepthPx) - the UI lane's taps (its LoadDepthPS is this
// at KhDepthPx(px): the near-z mark and VM_SEE are never armed there).
float KhOlRawT(int2 khor_t)
{
#if MSAA_DEPTH
    return depthTex.Load(khor_t, 0);
#else
    return depthTex.Load(int3(khor_t, 0)).x;
#endif
}
int2 KhOlDepthDim()
{
    uint khod_w, khod_h;
#if MSAA_DEPTH
    uint khod_n;
    depthTex.GetDimensions(khod_w, khod_h, khod_n);
#else
    depthTex.GetDimensions(khod_w, khod_h);
#endif
    return int2((int)khod_w, (int)khod_h);
}
float KhOlTap(int2 khot_c, int2 khot_a, int khot_k, bool khot_t)
{
    const int2 khot_q = khot_c + khot_a * khot_k;
    float khot_w;
    [branch] if (khot_t) khot_w = 1.0f / max(LinDepth(KhOlRawT(khot_q)), 1.0e-3f);
    else                 khot_w = KhOlW(khot_q);
    return khot_w;
}
float KhOlAxis(int2 khoa_c, int2 khoa_a, int khoa_d, int khoa_p, int khoa_hi, bool khoa_t, float khoa_w0,
               out float khoa_wx)
{
    float khoa_e = 0.0f;
    khoa_wx = khoa_w0;
    [branch] if (khoa_p - khoa_d >= 0 && khoa_p + khoa_d <= khoa_hi) {   // Centred.
        const float khoa_wp = KhOlTap(khoa_c, khoa_a, khoa_d, khoa_t);
        const float khoa_wm = KhOlTap(khoa_c, khoa_a, -khoa_d, khoa_t);
        if (abs(khoa_wp - khoa_w0) >= abs(khoa_w0 - khoa_wm)) {
            khoa_e = abs(khoa_wp + khoa_wm - 2.0f * khoa_w0);
            khoa_wx = khoa_wp;
        }
    } else if (khoa_p - khoa_d < 0 && khoa_p + 2 * khoa_d <= khoa_hi) {   // The start: into the frame.
        const float khoa_w1 = KhOlTap(khoa_c, khoa_a, khoa_d, khoa_t);
        const float khoa_w2 = KhOlTap(khoa_c, khoa_a, 2 * khoa_d, khoa_t);
        if (abs(khoa_w1 - khoa_w0) >= abs(khoa_w2 - khoa_w1)) {
            khoa_e = abs(khoa_w0 - 2.0f * khoa_w1 + khoa_w2);
            khoa_wx = khoa_w1;
        }
    } else if (khoa_p + khoa_d > khoa_hi && khoa_p - khoa_d >= 0) {   // The end: the forward tap at the edge.
        const float khoa_s = (float)(khoa_hi - khoa_p) / (float)khoa_d;
        const float khoa_wf = KhOlTap(khoa_c, khoa_a, khoa_hi - khoa_p, khoa_t);
        const float khoa_wm = KhOlTap(khoa_c, khoa_a, -khoa_d, khoa_t);
        if (abs(khoa_wf - khoa_w0) >= khoa_s * abs(khoa_w0 - khoa_wm)) {
            khoa_e = abs((khoa_wf - khoa_w0) - khoa_s * (khoa_w0 - khoa_wm));
            khoa_wx = khoa_wf;
        }
    }
    return khoa_e;
}

// FX_ONE_FORMULA: every builtin runs one formula on the scene (linear HDR, before the game's tonemap) and on the UI
// lane (the finished display frame, 0 - 1), by design - no formula has a per-lane variant. invert, colorGrade
// (contrast turns about 0.5), grain, deband (1/255 steps), clarity's limiter, outline's luma edge, glitch's static and
// the rain drops' highlight are written for display values, so on the scene they act on linear light; scripts are
// tuned to that, and a per-lane "fix" changes every tuned look.
float4 PSEffect(VSOut i) : SV_Target
{
    KhFxBegin(i);   // FX_USER: the lanes, the depth gate, the view-distance cut and the arbitration (cb.hlsl).
    int2  px = int2(i.pos.xy);
    float2 uv = i.pos.xy / float2(fxMeta.z, fxMeta.w);
    int   effect = (int)fxMeta.x;
    float t = fxMeta.y;
    float3 scene = SampleScene(px);
    float3 outc = scene;

    if (centerSize.w > 1.5f) {
        // FX_SIDE: eight fixed pixels, so one verdict for the whole pass - the UI flush draws it once, 1 x 1,
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
        if (effect == 30) return float4(khpMin, 0.0f, 0.0f, 1.0f);   // FX_SIDE: the probe itself.
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
                            // color.rgb.
    {
        // GRADE_TINT: brightness, saturation, then the tint, then contrast - before the saturation the tint would
        // only move the luma weights, and saturation 0 would leave a tinted grade grey. TWIN: KhFusePoint's id 2.
        float3 c = scene * fxParams0.z;
        float l = Luma(c);
        c = lerp(l.xxx, c, fxParams0.x) * color.rgb;
        c = (c - 0.5f) * fxParams0.y + 0.5f;
        // GAMMA_DOC: the exponent is gamma itself, not 1 / gamma - above 1 darkens values under 1 and lifts those
        // past 1 (the scene's highlights), below 1 the reverse. Scripts are tuned to that direction: never invert it.
        // The exponent is floored at 1e-4: gamma 0 (or below) would give pow(0, 0) = NaN on every black pixel (and inf
        // below zero); at the floor black stays black and the rest goes to ~1 - the limit gamma -> 0 gives.
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
        // FX_ROUND: the shift grows with the radius on the picture - x in heights (x the aspect), as the sun flare
        // measures it - so it is round (in uv the side edges, 1.78 x farther at 16:9, would shift as the top and
        // bottom). strengthPx / 2 at the top and bottom edges.
        float2 off = float2((uv.x - 0.5f) * (fxMeta.z / fxMeta.w), uv.y - 0.5f)
                   * fxParams0.x * KhFxPx();   // FX_PX_REF.
        float r = KhSampleSceneBil(i.pos.xy + off).r;   // FX_BILINEAR.
        float b = KhSampleSceneBil(i.pos.xy - off).b;
        outc = float3(r, scene.g, b);
    }
    else if (effect == 5)   // Film grain: [amount, fps, grainSizePx, lumaResponse, chroma].
    {
        // Filmic grain: smooth value noise with spatial extent, triangular amplitude distribution, response peaking at
        // mid-grey and falling toward shadows and highlights alike, optional chroma, time quantized to a frame rate.
        const float3 gc = KhGrainGc(i.pos.xy, t, fxParams0, fxParams1.x);   // KhFusePoint's twin: one body.

        float luma = saturate(Luma(scene));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, fxParams0.w);
        // GRAIN_FLOOR: the zero-mean grain would take a dark pixel below zero, a negative radiance in the HDR scene;
        // floored as sharpen's output is. TWIN: KhFusePoint id 5.
        outc = max(scene + gc * fxParams0.x * resp, 0.0f);
    }
    else if (effect == 6)   // Sharpen: [strength].
    {
        // SHARPEN_LIMIT: the 5-tap Laplacian, limited to the five taps' own range per channel (the limiting of
        // contrast-adaptive sharpening). On the linear HDR scene the unlimited form takes the dark side of any edge of
        // (1 + k) / k : 1 or more (3 : 1 at the default 0.5: every skyline against trees) to zero - a black line - and
        // lifts the bright side as far above it. The limit removes every overshoot: a pixel that is its five taps'
        // brightest or darkest (either side of a hard edge, a one-pixel detail) is left as it is, and a pixel between
        // them steepens toward them up to their range - a soft edge sharpens, a hard one keeps no halo.
        const float3 khsh_r = SampleScene(px + int2(1, 0)), khsh_l = SampleScene(px - int2(1, 0));
        const float3 khsh_d = SampleScene(px + int2(0, 1)), khsh_u = SampleScene(px - int2(0, 1));
        float3 s = scene * (1.0f + 4.0f * fxParams0.x)
                 - fxParams0.x * (khsh_r + khsh_l + khsh_d + khsh_u);
        const float3 khsh_mn = min(scene, min(min(khsh_r, khsh_l), min(khsh_d, khsh_u)));
        const float3 khsh_mx = max(scene, max(max(khsh_r, khsh_l), max(khsh_d, khsh_u)));
        outc = max(clamp(s, khsh_mn, khsh_mx), 0.0f);
    }
    else if (effect == 7)   // Gaussian-ish blur: [radiusPx].
    {
        // GLOW_PYR: the same 3 x 3 kernel, each tap over the radius (the tap spacing).
        const float khbl_s = max(fxParams0.x * KhFxPx(), 1.0f);
        const float khbl_m = KhGlowMix(khbl_s);
        float3 acc = 0.0f;
        if (khbl_m < 1.0f)
        {
            int r = max((int)(fxParams0.x * KhFxPx()), 1);   // FX_PX_REF.
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
        // GLOW_PYR: the same 5 x 5 grid, each tap over the radius (the tap spacing); the pyramid is thresholded.
        const float khbm_s = max(fxParams0.z * KhFxPx(), 1.0f);
        const float khbm_m = KhGlowMix(khbm_s);
        float3 acc = 0.0f;
        if (khbm_m < 1.0f)
        {
            int r = max((int)(fxParams0.z * KhFxPx()), 1);   // FX_PX_REF.
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
        float2 off = float2(KhSin(uv.y * fxParams0.y * 6.2832f + t * fxParams0.z),   // HASH: t grows.
                            KhCos(uv.x * fxParams0.y * 6.2832f + t * fxParams0.z)) * fxParams0.x * KhFxPx();   // FX_PX_REF.
        outc = KhSampleSceneBil(i.pos.xy + off);   // FX_BILINEAR.
    }

        else if (effect == 10)   // Outline: [depthEdgeScale, lumEdgeScale, sceneDarken, glowBoost],
                                 // color = edge.
    {
        // FX_PX_REF: the step is the drawn line's width - one reference pixel, to the nearest whole pixel - and a
        // step spanning the same part of the picture measures the same depth and luma differences, so the thresholds
        // hold at any size.
        const int khol_d = max((int)floor(KhFxPx() + 0.5f), 1);
        // The depth taps stay inside the frame (KhOlAxis picks its forms so), as SampleScene clamps the luma taps:
        // LoadDepthPS does not, and a Load past the frame's edge reads 0 - the near plane - which would draw an
        // edge line along the borders.
        const int2 khol_hi = int2((int)fxMeta.z - 1, (int)fxMeta.w - 1);
        // OL_D2: the depth edge is the second difference of w = 1 / z (z = LinDepth), which is affine in screen
        // space on any plane: zero on every plane at any slant and distance, large where the surface breaks (a
        // silhouette; a crease, by its change of slope). A first difference of z grows with the slant: level ground
        // would draw as edge from ~100 m and as solid edge colour past ~400 m (depthEdge 4, 1080 rows). Per axis:
        // KhOlAxis (its note: the gating that keeps the first difference's pixels, the forms at the borders, the UI
        // lane's texel steps). Scale, per axis: x z z_x / max(min(z, z_x), 1), z_x = 1 / w at the tap across the
        // break - a step z -> z' then gives |z' - z| / max(min(z, z'), 1) on BOTH sides of it, at least the first
        // difference's |z' - z| / max(z, 1) on its side (10 -> 11 m at depthEdge 4: 0.40 either side; a hand at
        // 0.1 m against 10 m: saturated either side); scaled by the centre's z alone the near side would stop at
        // depthEdge x min(z, 1) x (1 - z / z'). On a plane z_x ~ z: z^2 / max(z, 1), the residue of the depth's own
        // quantisation. A surface against the sky (1e9: w 1e-9) gives ~1e9 / max(z, 1): saturated on both sides at
        // any depthEdge above ~1e-9 x max(z, 1) (2e-5 at 20 km); the sky against the sky gives 0.
        // Cost: five depth reads a pixel (LoadDepthPS: up to three loads each - the depth, the near-z mark, VM_SEE)
        // where the first difference took three.
        const float khol_z = LinDepth(LoadDepthPS(px));
        const float khol_w = 1.0f / max(khol_z, 1.0e-3f);   // KhOlW(px).
        // The UI lane (centerSize.w above 1.125, KhDepthPx's test): the axes in depth texels, the step the frame's in
        // texels (rounded, at least 1), the centre KhDepthPx(px) - the texel LoadDepthPS(px) read.
        const bool khol_t = centerSize.w > 1.125f;
        int2 khol_c = px;
        int2 khol_e = khol_hi;
        int2 khol_s = int2(khol_d, khol_d);
        [branch] if (khol_t) {
            const int2 khol_n = KhOlDepthDim();
            if (khol_n.x > 0 && khol_n.y > 0) {
                khol_c = KhDepthPx(px);
                khol_e = khol_n - 1;
                khol_s = max(int2(floor((float)khol_d * float2(khol_n) / max(fxMeta.zw, float2(1.0f, 1.0f)) + 0.5f)),
                             int2(1, 1));
            }
        }
        float khol_wx, khol_wy;
        const float khol_ex = KhOlAxis(khol_c, int2(1, 0), khol_s.x, khol_c.x, khol_e.x, khol_t, khol_w, khol_wx);
        const float khol_ey = KhOlAxis(khol_c, int2(0, 1), khol_s.y, khol_c.y, khol_e.y, khol_t, khol_w, khol_wy);
        const float khol_zx = 1.0f / khol_wx, khol_zy = 1.0f / khol_wy;
        float depthEdge = saturate((khol_ex * khol_z * khol_zx / max(min(khol_z, khol_zx), 1.0f)
                                  + khol_ey * khol_z * khol_zy / max(min(khol_z, khol_zy), 1.0f)) * fxParams0.x);
        float lC = Luma(scene);
        float lumEdge = saturate((abs(Luma(SampleScene(px + int2(khol_d, 0))) - lC)
                                + abs(Luma(SampleScene(px + int2(0, khol_d))) - lC)) * fxParams0.y);
        float edge = saturate(depthEdge + lumEdge);
        outc = scene * fxParams0.z + color.rgb * edge * fxParams0.w;
    }
    else if (effect == 11)   // Pulse: p0.xyz = center (engine space), p0.w = radius; p1 =
                             // [bandWidth, intensity].
    {
        // LOCAL_REL (cb.hlsl's localization mask): the camera-relative point against the centre made
        // relative to the camera, exactly. The ring itself: KhFxPulseRing.
        precise float3 khpl_c = fxParams0.xyz - (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
        const float khpl_r = LoadDepthPS(px);
        float ring = KhFxPulseRing(khpl_c, uv, LinDepth(khpl_r));
        // WSEE: the ring on what lies behind the world's blended layers shows through them by their
        // transmittance, the ring on the layers by the rest (additive light: exact), per sample where the pixel's
        // samples disagree (KhWseeMix).
        KhWseeMix(px, khpl_r, ring, khpl_z, KhFxPulseRing(khpl_c, uv, khpl_z));
        outc = scene + color.rgb * ring * fxParams1.y;
    }
    else if (effect == 12)   // Halation: [threshold, intensity, radiusPx], color = glow tint
                             // (warm).
    {
        const int2 dirs[8] = { int2(1,0), int2(-1,0), int2(0,1), int2(0,-1),
                               int2(1,1), int2(-1,1), int2(1,-1), int2(-1,-1) };
        // GLOW_PYR: the same two rings, each tap over its ring's spacing (the radius, twice it); thresholded.
        const float khha_s = max(fxParams0.z * KhFxPx(), 1.0f);
        const float khha_m = KhGlowMix(khha_s);
        float3 acc = 0.0f;
        if (khha_m < 1.0f)
        {
            int r = max((int)(fxParams0.z * KhFxPx()), 1);   // FX_PX_REF.
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
    else if (effect == 13)   // Distance fog: [startDist m, endDist m, skyAmount 1], color = fog
                             // color.
    {
        // The factor: KhFxDistFogF (twin edit at cb.hlsl's KhFxFogPasses).
        const float khfg_r = LoadDepthPS(px);
        const float f = KhFxDistFogF(LinDepth(khfg_r));
        outc = lerp(scene, color.rgb, f);
        // WSEE: S = Tr B + E (E the layers' own light) - the background fogged by f behind, the layers' light by f
        // in front: Tr lerp(B', g, f_b) + (1 - f_f) max(S - Tr B', 0) + (1 - Tr) f_f g, with B' = max(B + (S - R), 0)
        // (the earlier chain passes' change to the pixel, given to the background too). Exact for one blended layer
        // in the first chain pass that changes the colour; approximate after earlier passes and for several layers.
        // Only where G < 0.75 (G = 0, or a derived pre with B2: B / B2 is the background) and VM_SEE's planes do
        // not also cover the pixel; elsewhere covered the factor blend lerp(S, g, Tr f_b + (1 - Tr) f_f). Per sample
        // where the samples disagree: with khWsC each sample's colour C_k' = C_k + (S - R) takes its own rule in place
        // of S (lerp(C_k', g, f(r_k)) uncovered) and the pixel their mean - unclamped, so equal factors give lerp(S, g,
        // f) exactly; without it lerp(S, g, mean of the samples' factors). One-entry caches keep KhFxDistFogF to the
        // depths not yet seen (KhWseeMix's).
        [branch] if (KhWseeArmed()) {
            // WSEE_EDGE_COST: the pairs are classes where khWsE's word stands (cb.hlsl KhWseeCountW), each
            // weighed by its members and coloured by their mean C (KhWseeClassC); else every sample.
            float2       khfg_g;
            uint         khfg_w;
            const uint   khfg_n = KhWseeCountW(px, khfg_r, khfg_g, khfg_w);
            const bool   khfg_pm = khfg_n > 1u || khfg_w != 0u;   // WSEE_EDGE_COST: the per-sample form.
            const bool   khfg_cs = khfg_pm && (KhWseeAvail() & 2u) != 0u;   // The samples' colours (t59).
            const bool   khfg_co = !khfg_pm || khfg_cs;   // A colour per pair: the exact rule can apply.
            float  khfg_lf = khfg_r, khfg_qf = f, khfg_lb = khfg_r, khfg_qb = f, khfg_fa = 0.0f;
            float3 khfg_ca = float3(0.0f, 0.0f, 0.0f);
            bool   khfg_c = khfg_pm;
            float3 khfg_d = float3(0.0f, 0.0f, 0.0f);   // S - R: the earlier chain passes' change (per sample only).
#if MSAA_DEPTH
            [branch] if (khfg_cs) khfg_d = scene - khWsR.Load(int3(px, 0)).rgb;
#endif
            KH_WSEE_LOOP for (uint khfg_k = 0u; khfg_k < khfg_n; ++khfg_k) {
                float3 khfg_o;
                const float khfg_t = KhWseeSampleAtW(px, khfg_r, khfg_k, khfg_n, khfg_w, khfg_g, khfg_o);
                // WSEE_FAINT (cb.hlsl): faint layers alone, the behind the depth buffer's - the factor at r (f),
                // the plain lerp below; nothing evaluated at another depth. KhWseeMix's rule.
                const bool  khfg_fn = khfg_o.y > khfg_o.x && khfg_t >= KH_WSEE_FAINT_T && khfg_o.y == khfg_r;
                float khfg_fk = f;
                bool  khfg_ex = false;
                [branch] if (!khfg_fn) {
                [branch] if (khfg_o.x != khfg_lf) {
                    khfg_lf = khfg_o.x;
                    khfg_qf = KhFxDistFogF(LinDepth(khfg_o.x));
                }
                khfg_fk = khfg_qf;
                [branch] if (khfg_o.y > khfg_o.x) {
                    khfg_c = true;
                    [branch] if (khfg_o.y != khfg_lb) {
                        khfg_lb = khfg_o.y;
                        khfg_qb = KhFxDistFogF(LinDepth(khfg_o.y));
                    }
                    // Two or more layer groups: the factor composed front to back (the factor blend and the mean
                    // factor; the exact stages keep their own formula). One group: its front is KhWseeOneFront's.
                    [branch] if (khWsGn >= 2u) {
                        float khfg_gv = 0.0f, khfg_gp = 1.0f;
                        [loop] for (uint khfg_i = 0u; khfg_i < (khWsSp ? 7u : 3u); ++khfg_i) {
                            const float khfg_gt = KhWseeGroupT7(khfg_i);
                            [branch] if (khfg_gt < 1.0f) {
                                const float khfg_gr = KhWseeGroupZ7(khfg_i);
                                float khfg_gq = khfg_qb;
                                [branch] if (khfg_gr == khfg_lf) {
                                    khfg_gq = khfg_qf;
                                } else {
                                    [branch] if (khfg_gr != khfg_lb) khfg_gq = KhFxDistFogF(LinDepth(khfg_gr));
                                }
                                khfg_gv += khfg_gp * (1.0f - khfg_gt) * khfg_gq;
                                khfg_gp *= khfg_gt;
                            }
                        }
                        khfg_fk = khfg_gv + khfg_gp * khfg_qb;
                    } else {
                        // The factor at the one group's own depth (the cached front / behind reused).
                        const float khfg_of = KhWseeOneFront(khfg_o);
                        float khfg_q1 = khfg_qb;
                        [branch] if (khfg_of == khfg_lf) {
                            khfg_q1 = khfg_qf;
                        } else {
                            [branch] if (khfg_of != khfg_lb) khfg_q1 = KhFxDistFogF(LinDepth(khfg_of));
                        }
                        khfg_fk = khfg_t * khfg_qb + (1.0f - khfg_t) * khfg_q1;
                    }
                    khfg_ex = khfg_o.z < 0.75f;   // G = 0 or a derived pre with B2 (0.5).
                }
                }
                const float khfg_m = KhWseeClassM(khfg_w, khfg_k);   // WSEE_EDGE_COST: the pair's weight.
                khfg_fa += khfg_m * khfg_fk;
                [branch] if (khfg_co) {
                    float3 khfg_sc = scene;
#if MSAA_DEPTH
                    [branch] if (khfg_cs) khfg_sc = KhWseeClassC(px, khfg_w, khfg_k) + khfg_d;   // WSEE_EDGE_COST.
#endif
                    float3 khfg_ok = lerp(khfg_sc, color.rgb, khfg_fk);
                    // TWIN: cb.hlsl KhWseeBehindColor is the per-pixel form of the three stages below (the colour
                    // behind and the capped transmittance it gives the user contract) - WSEE_BEHIND.
                    [branch] if (khfg_ex) {
                        if (KhVmSeeT(px) >= 1.0f) {
                            // An infinite colour in S, R, B or B2 (an engine shader's inf; a 16F target holds inf,
                            // 32F whatever was written) makes the sample NaN below as the plain fog's lerp does; not
                            // capped (4 ALU a covered sample for a write no engine pass is known to make). S - R per
                            // sample as written: loop-invariant, fxc hoists the load out of the sample loop.
                            const float3 khfg_d2 = scene - khWsR.Load(int3(px, 0)).rgb;   // S - R.
                            const float4 khfg_x = khWsLx;
                            [branch] if (khfg_o.z > 0.25f) {
                                // G = 0.5: a late opaque drawn before B2 (derived, B2 bound) - its colour is B2's
                                // and its depth the behind (S2's): one stage over B2' = B2 + (S - R), the fence-type
                                // and post layers (Tw Tpost, capped against the pixel) at the front of them, min(r,
                                // NWpost); the pre layers drawn over it before B2 are in B2's colour, fogged by the
                                // factor blend between its fog and NWpre's (Tpre; exact where there are none).
                                const float3 khfg_b2 = max(khWsB2.Load(int3(px, 0)).rgb + khfg_d2, 0.0f);
                                const float  khfg_tq = KhWseeCap(khfg_x.x * khfg_x.w, khfg_sc, khfg_b2);
                                float khfg_fp = khWsLn.x < khfg_o.y ? KhFxDistFogF(LinDepth(khWsLn.x)) : khfg_qb;
                                // With the layer-depth split each stage's light by its groups (the pre ones at NWpre
                                // and their band / far parts; the fence-type and post ones).
                                const bool khfg_sp = khWsSp && (khWsLs.y < 1.0f || khWsLs.w < 1.0f ||
                                                                khWsLs2.x < 1.0f || khWsLs2.y < 1.0f);
                                [branch] if (khfg_sp) {
                                    khfg_fp = KhWseeFogMean(float4(khWsLn.x, khWsLzx2.x, khWsLzx.x, khfg_o.y),
                                                            float4(khWsLs.x, khWsLs2.x, khWsLs.y, 1.0f), khfg_o.y,
                                                            khfg_qb, khfg_fp);
                                }
                                const float  khfg_fb = khfg_x.z * khfg_qb + (1.0f - khfg_x.z) * khfg_fp;
                                float khfg_ff = KhFxDistFogF(LinDepth(min(khWsLr, khWsLn.y)));
                                [branch] if (khfg_sp) {
                                    khfg_ff = KhWseeFogMean(float4(khWsLr, khWsLn.y, khWsLzx2.y, khWsLzx.y),
                                                            float4(khfg_x.x, khWsLs.z, khWsLs2.y, khWsLs.w), khfg_o.y,
                                                            khfg_qb, khfg_ff);
                                } else {   // WSEE_STAGE_MEAN.
                                    khfg_ff = KhWseeStageMean(khWsLr, khfg_x.x, khWsLn.y, khfg_x.w, khfg_o.y,
                                                              khfg_qb, khfg_ff);
                                }
                                khfg_ok = khfg_tq * lerp(khfg_b2, color.rgb, khfg_fb)
                                        + (1.0f - khfg_ff) * max(khfg_sc - khfg_tq * khfg_b2, 0.0f)
                                        + (1.0f - khfg_tq) * khfg_ff * color.rgb;
                            } else {
                                const float3 khfg_b = max(khWsB.Load(int3(px, 0)).rgb + khfg_d2, 0.0f);
                                // The two stages - the particle-type layers drawn before B2 over B (Tpre, their light
                                // at NWpre), then the fence-type layers and the particle-type ones after B2 over B2
                                // (Tw Tpost, their light at min(r, NWpost)) - each stage's T held by the stacking cap.
                                // The record is KhWseeRule's (khWsLx, khWsLn, khWsLr: this sample's). Without B2: one
                                // stage over B, every layer's T at the front. Per sample the cap's pixel is the
                                // sample's own colour khfg_sc.
                                [branch] if ((KhWseeAvail() & 8u) != 0u) {
                                    const float3 khfg_b2 = max(khWsB2.Load(int3(px, 0)).rgb + khfg_d2, 0.0f);
                                    const float  khfg_tq = KhWseeCap(khfg_x.x * khfg_x.w, khfg_sc, khfg_b2);
                                    const float  khfg_tp = KhWseeCap(khfg_x.z, khfg_b2, khfg_b);
                                    // f_pre: NWpre's, where it lies before the behind (b); f_front: min(r, NWpost)'s.
                                    float khfg_fp = khWsLn.x < khfg_o.y ? KhFxDistFogF(LinDepth(khWsLn.x)) : khfg_qb;
                                    float khfg_ff = KhFxDistFogF(LinDepth(min(khWsLr, khWsLn.y)));
                                    [branch] if (khWsSp && (khWsLs.y < 1.0f || khWsLs.w < 1.0f ||
                                                            khWsLs2.x < 1.0f || khWsLs2.y < 1.0f)) {
                                        khfg_fp = KhWseeFogMean(float4(khWsLn.x, khWsLzx2.x, khWsLzx.x, khfg_o.y),
                                                                float4(khWsLs.x, khWsLs2.x, khWsLs.y, 1.0f), khfg_o.y,
                                                                khfg_qb, khfg_fp);
                                        khfg_ff = KhWseeFogMean(float4(khWsLr, khWsLn.y, khWsLzx2.y, khWsLzx.y),
                                                                float4(khfg_x.x, khWsLs.z, khWsLs2.y, khWsLs.w),
                                                                khfg_o.y, khfg_qb, khfg_ff);
                                    } else {   // WSEE_STAGE_MEAN.
                                        khfg_ff = KhWseeStageMean(khWsLr, khfg_x.x, khWsLn.y, khfg_x.w, khfg_o.y,
                                                                  khfg_qb, khfg_ff);
                                    }
                                    const float3 khfg_f2 = khfg_tp * lerp(khfg_b, color.rgb, khfg_qb)
                                                         + (1.0f - khfg_fp) * max(khfg_b2 - khfg_tp * khfg_b, 0.0f)
                                                         + (1.0f - khfg_tp) * khfg_fp * color.rgb;
                                    khfg_ok = khfg_tq * khfg_f2
                                            + (1.0f - khfg_ff) * max(khfg_sc - khfg_tq * khfg_b2, 0.0f)
                                            + (1.0f - khfg_tq) * khfg_ff * color.rgb;
                                } else {
                                    const float khfg_tc = KhWseeCap(khfg_t, khfg_sc, khfg_b);
                                    float khfg_f1 = khfg_qf;   // The light by group with the split.
                                    [branch] if (khWsSp && (khWsLs.y < 1.0f || khWsLs.w < 1.0f ||
                                                            khWsLs2.x < 1.0f || khWsLs2.y < 1.0f)) {
                                        khfg_f1 = KhWseeFogMeanAll(khfg_o.y, khfg_qb, khfg_qf);
                                    } else {   // WSEE_STAGE_MEAN: the one stage's three groups (KhWseeGroupsSet's).
                                        khfg_f1 = KhWseeFogMean(float4(khWsGz, khfg_o.y), float4(khWsGt, 1.0f),
                                                                khfg_o.y, khfg_qb, khfg_qf);
                                    }
                                    khfg_ok = khfg_tc * lerp(khfg_b, color.rgb, khfg_qb)
                                            + (1.0f - khfg_f1) * max(khfg_sc - khfg_tc * khfg_b, 0.0f)
                                            + (1.0f - khfg_tc) * khfg_f1 * color.rgb;
                                }
                            }
                        }
                    }
                    khfg_ca += khfg_m * khfg_ok;
                }
            }
            const float khfg_dv = KhWseeDivW(khfg_w, khfg_n);   // WSEE_EDGE_COST: the weights' sum.
            if (khfg_c) outc = khfg_co ? khfg_ca / khfg_dv : lerp(scene, color.rgb, khfg_fa / khfg_dv);
        }
    }
    else if (effect == 14)   // Lens flare, image-based: [threshold, intensity, ghostCount,
                             // ghostSpacing] + [haloRadius, haloIntensity, chromaPx].
    {
        // Bright pixels anywhere in the capture spawn a ghost train mirrored through screen centre, plus a halo ring.
        // The HDR capture makes the threshold physically meaningful.
        float2 cuv = float2(0.5f, 0.5f);
        float2 ghostVec = (cuv - uv) * fxParams0.w;
        int nGhosts = clamp((int)fxParams0.z, 1, 8);
        float3 acc = 0.0f;
        // FX_ROUND: the ghosts' window, the halo ring and the colour fringe's direction are measured on the picture
        // - x in heights (khlf_a, the aspect), as the sun flare measures - so the lens's circles are round (in uv they
        // would be ellipses 1.78 x wider at 16:9). haloRadius is in heights.
        const float2 khlf_a = float2(fxMeta.z / fxMeta.w, 1.0f);
        // GLOW_PYR: a ghost is the picture scaled by 1 - ghostSpacing * g about the centre, so a point read
        // either magnifies pixels into blocks or skips them; from the (thresholded) pyramid each ghost reads over
        // that scale (two pixels at least), and so does the halo.
        const bool khlf_g = KhGlowOn();
        for (int g = 1; g <= nGhosts; ++g)
        {
            float2 suv = uv + ghostVec * (float)g;
            float w = 1.0f - saturate(length((suv - cuv) * khlf_a) * 1.6f);
            w = w * w;
            float2 spf = saturate(suv) * float2(fxMeta.z, fxMeta.w);
            float2 cdir = normalize(ghostVec * float2(fxMeta.z, fxMeta.w) + 1e-5f)
                        * fxParams1.z * KhFxPx();   // FX_PX_REF.
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
        float rC = length((uv - cuv) * khlf_a);
        float hw = 1.0f - saturate(abs(rC - fxParams1.x) * 8.0f);
        float2 huv = uv + normalize((cuv - uv) * khlf_a + 1e-5f) * fxParams1.x / khlf_a;
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
        // ANA_PYR: the taps are lengthPx / 16 apart; from two pixels up each reads the one-axis pyramid over that
        // spacing (x KH_GLOW_FP, the glows' margin), so the streak is one continuous line instead of 16 copies of each
        // highlight a side. Taps a pixel apart or less read the picture itself (KhGlowMix).
        const float khan_l = fxParams0.z * KhFxPx();   // FX_PX_REF.
        const float khan_m = KhGlowMix(khan_l / 16.0f);

        [unroll] for (int k = 1; k <= 16; ++k)
        {
            float khan_t = (float)k / 16.0f;   // The tap's share of the length (not the time t).
            float w = pow(1.0f - khan_t, max(fxParams0.w, 0.1f));
            if (khan_m < 1.0f)
            {
                int off = (int)(khan_t * fxParams0.z * KhFxPx());   // FX_PX_REF.
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
                             // size; p1 = [ghostDots, ringIntensity, starburst].
    {
        // KhSunFlareSpot above (KhSunFlareVis / KhSunFlareEdge: cb.hlsl). FX_SIDE: the scene chain draws the
        // visibility (edge fade included) once, 1 x 1, before the pass (side id 29) and arms matCtl.y; unarmed, each
        // pixel measures it.
        float2 spos;
        int2 sp;
        if (KhSunFlareSpot(spos, sp))
        {
            {
                float vis;   // A branch, not ?: (which evaluates both arms).
                [branch] if (matCtl.y > 0.5f) vis = khFxSide.Load(int3(0, 0, 0));
                else                          vis = KhSunFlareVis(sp) * KhSunFlareEdge(spos);   // FLARE_EDGE.
                if (vis > 0.001f)
                {
                    float aspect = fxMeta.z / fxMeta.w;
                    float2 d = uv - spos;
                    d.x *= aspect;
                    float r = length(d) / max(fxParams0.w, 0.01f);
                    // FLARE_ATAN: atan2(0, 0) is NaN as fxc lowers it (0 x rcp(0)) - the pixel whose centre is
                    // the source's; 1e-20 moves no non-zero d.x (a non-zero difference of floats is no finer than
                    // the ulp of the smallest uv - half a texel, 0.5 / width - ~1e-12 at the least, far above where
                    // 1e-20 rounds away).
                    float ang = atan2(d.y, d.x + 1.0e-20f);
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
    else if (effect == 29)   // FX_SIDE: the sun flare's source visibility (1 x 1, drawn before the pass).
    {
        float2 spos;
        int2 sp;
        float khsv_v = 0.0f;   // A branch, not ?: (which evaluates both arms).
        [branch] if (KhSunFlareSpot(spos, sp)) khsv_v = KhSunFlareVis(sp) * KhSunFlareEdge(spos);   // FLARE_EDGE.
        return float4(khsv_v, 0.0f, 0.0f, 1.0f);
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
        suv.x += bh * fxParams0.z * drive * KhFxPx();   // FX_PX_REF.

        // Block corruption: coarse grid cells randomly displaced.
        if (fxParams1.y > 0.001f)
        {
            float2 cell = floor(uv * float2(24.0f, 14.0f));
            float ch = Hash(cell + tf * 1.7f);
            if (ch > 1.0f - 0.15f * saturate(fxParams1.y) * saturate(drive))
            {
                float2 off = (float2(Hash(cell + 3.1f + tf), Hash(cell + 5.7f + tf)) - 0.5f) * 80.0f * drive * KhFxPx();   // FX_PX_REF.
                suv += off;
            }
        }

        // RGB channel split along the tear axis.
        float split = fxParams1.x * drive * KhFxPx();   // FX_PX_REF.
        float3 col;
        col.r = SampleScene(int2(suv + float2(split, 0.0f))).r;
        col.g = SampleScene(int2(suv)).g;
        col.b = SampleScene(int2(suv - float2(split, 0.0f))).b;

        // Interference lines + static.
        if (fxParams1.z > 0.001f)
        {
            float ln = Hash(float2(floor(i.pos.y * 0.5f / KhFxPx()), tf * 2.3f));   // FX_PX_REF: 2-px rows.
            float lineHit = step(1.0f - 0.2f * saturate(fxParams1.z) * saturate(drive), ln);
            // FX_PX_REF: one static cell per reference pixel (the cell centre is the pixel centre at 1080).
            float n = Hash((floor(i.pos.xy / KhFxPx()) + 0.5f) * 0.37f + tf * 13.7f);
            col = lerp(col, color.rgb * n, lineHit * 0.85f);
            col += (n - 0.5f) * fxParams1.z * drive * 0.35f;
        }

        outc = max(col, 0.0f);
    }
     else if (effect == 18)   // Clarity: [strength, radiusPx].
    {
        // Wide-radius local contrast on luma only (the scene is rescaled by L'/L, so saturation never moves). Two-ring
        // 12-tap base estimate; the detail term is soft-limited so strong edges do not halo.
        float rad = max(fxParams0.y, 4.0f) * KhFxPx();   // FX_PX_REF.
        float lC = Luma(scene);
        float lB = 0.0f;
        // GLOW_PYR: the twelve taps sit 0.785 rad apart along their rings (2 pi rad / 8, 2 pi (rad / 2) / 4);
        // each over that spacing, the base estimate is the smooth local average it stands for.
        // CLARITY_ID: each tap weighs exactly 1 / 12 (not 0.0833: those weights sum to 0.9996, so a flat picture
        // reads a detail of 0.0004 x its luma and is scaled by 1.0003 at the default strength).
        const float khcl_s = 0.7854f * rad;
        const float khcl_m = KhGlowMix(khcl_s);

        if (khcl_m < 1.0f)
        {
            [loop] for (int k = 0; k < 8; ++k)
            {
                float ang = float(k) * 0.7854f + 0.3927f;
                lB += Luma(SampleScene(px + int2(cos(ang) * rad, sin(ang) * rad))) * (1.0f / 12.0f);
            }

            [loop] for (int k2 = 0; k2 < 4; ++k2)
            {
                float ang2 = float(k2) * 1.5708f;
                lB += Luma(SampleScene(px + int2(cos(ang2) * rad * 0.5f, sin(ang2) * rad * 0.5f))) * (1.0f / 12.0f);
            }
        }
        float khcl_p = 0.0f;
        if (khcl_m > 0.0f)
        {
            [loop] for (int khcl_k = 0; khcl_k < 8; ++khcl_k)
            {
                const float khcl_a = float(khcl_k) * 0.7854f + 0.3927f;
                khcl_p += Luma(KhGlowTap(i.pos.xy + float2(cos(khcl_a), sin(khcl_a)) * rad, khcl_s)) * (1.0f / 12.0f);
            }
            [loop] for (int khcl_j = 0; khcl_j < 4; ++khcl_j)
            {
                const float khcl_b = float(khcl_j) * 1.5708f;
                khcl_p += Luma(KhGlowTap(i.pos.xy + float2(cos(khcl_b), sin(khcl_b)) * (rad * 0.5f), khcl_s))
                        * (1.0f / 12.0f);
            }
        }
        lB = KhGlowSel(lB, khcl_p, khcl_m);

        float detail = lC - lB;
        detail = detail / (1.0f + 2.5f * abs(detail));   // Soft limiter (anti-halo).
        // CLARITY_ID: the floor guards the division only - the identity term stays 1. Do not write it as
        // (lC + s d 2.2) / max(lC, 1e-3): that scales any pixel under a luma of 1e-3 by lC / 1e-3 with no detail at all
        // (strength 0, a flat area), down to the 0.3 clamp, crushing the darkest night shadows. The two are equal above
        // 1e-3 (to an ulp).
        float scale = 1.0f + fxParams0.x * detail * 2.2f / max(lC, 1e-3f);
        outc = scene * clamp(scale, 0.3f, 3.0f);
    }
    else if (effect == 19)   // Deband: [threshold(1/255), rangePx, grain(1/255)].
    {
        // Gradient debanding: average four taps on a hash-rotated cross within range; adopt the average only where it
        // stays inside the threshold (real edges never do). Triangular-pdf grain breaks residual quantization. Static
        // noise (no TAA exists to launder animation).
        float2 h = Hash2(i.pos.xy);
        float ang = h.x * 6.2832f;
        float rad = (0.3f + 0.7f * h.y) * max(fxParams0.y, 2.0f) * KhFxPx();   // FX_PX_REF (the dither stays per pixel).
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
    else if (effect == 20)   // Rainlens: [intensity, speed, condensation, refract]; fx1.xyz = system
                             // camera velocity, fx1.w = the integrated rain clock (the shader scales it by speed).
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
        // GLOW_PYR: the film's eight taps sit 2 pi 7 / 8 = 5.5 reference pixels apart; each over that spacing.
        const float khrl_s = 5.4978f * KhFxPx();
        const float khrl_m = KhGlowMix(khrl_s);

        if (khrl_m < 1.0f)
        {
            [loop] for (int k = 0; k < 8; ++k)
            {
                float ang = float(k) * 0.7854f;
                fogC += SampleScene(px + int2(cos(ang) * 7.0f * KhFxPx(), sin(ang) * 7.0f * KhFxPx())) * 0.125f;   // FX_PX_REF.
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
        // Fine static condensation droplets (twinkle in with the film, cleared where trails wiped).
        float2 sid = floor(q * 34.0f);
        float sn = Hash(sid);
        float2 sf = frac(q * 34.0f) - 0.5f;
        float slife = saturate(KhSin(t * 0.35f + sn * 6.2832f) * 0.5f + 0.5f);   // HASH.
        float sdrop = smoothstep(0.12f + sn * 0.1f, 0.05f, length(sf))
                    * step(sn, fogAmt * 0.8f) * slife * (1.0f - saturate(trail * 1.6f));
        nrm += (sf / 0.2f) * sdrop * 0.18f;
        drop = max(drop, sdrop * 0.5f);
        float wiped = saturate(trail * 1.5f + drop);
        float3 base = lerp(scene, fogC, fogAmt * (1.0f - wiped) * 0.85f);
        // Refraction through the drops: inverted-wide sample + chroma split.
        float refr = max(fxParams0.w, 0.0f);
        float2 offPx = -nrm * refr * (20.0f + fwdK * 8.0f) * KhFxPx();   // FX_PX_REF.
        const float2 khrl_c = float2(px) + 0.5f;   // FX_BILINEAR: the pixel's centre.
        float3 through;
        through.g = KhSampleSceneBil(khrl_c + offPx).g;
        through.r = KhSampleSceneBil(khrl_c + offPx * 1.07f).r;
        through.b = KhSampleSceneBil(khrl_c + offPx * 0.93f).b;
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

        // Tracking wobble (fxParams2.y, px): each scanline row shifts by a per-line random jitter requantized at 24 Hz
        // plus a slow per-line sway. Only the picture wobbles; the tube mask and the grille live on the glass.
        float khc_wob = ((Hash(float2(khc_line * 0.173f, floor(t * 24.0f) * 0.71f)) - 0.5f)
                       + 0.35f * KhSin(t * 2.3f + khc_line * 0.61f)) * fxParams2.y * KhFxPx();   // FX_PX_REF, HASH.

        float2 khc_dpx = khc_duv * float2(fxMeta.z, fxMeta.w);
        khc_dpx.x += khc_wob;
        float2 khc_fpx = khc_cc * khc_r2 * 4.0f * fxParams1.x * KhFxPx();   // FX_PX_REF.
        float3 khc_col;
        khc_col.r = KhSampleSceneBil(khc_dpx + khc_fpx).r;   // FX_BILINEAR (khc_dpx: the centre, unwarped).
        khc_col.g = KhSampleSceneBil(khc_dpx).g;
        khc_col.b = KhSampleSceneBil(khc_dpx - khc_fpx).b;

        // Beam scanlines: a shaped sine whose exponent narrows in the shadows and widens toward the highlights
        // (phosphor blooming); the 1.32 gain lifts the average level the dark gaps remove (to 0.95 in the highlights,
        // 0.75 at mid-grey, 0.64 in the shadows).
        float khc_lum = saturate(Luma(khc_col));
        const float khc_bex = lerp(2.2f, 0.65f, khc_lum);
        float khc_beam = pow(abs(KhSin(khc_phase * 3.14159265f)),   // HASH: the phase scrolls with t.
                             khc_bex);
        // CRT_BANDLIMIT - the lines' period on screen: rows per line at this pixel, through the warp's vertical
        // stretch (d duv.y / d uv.y). Sampled once per pixel, a period of 2 rows or less cannot be drawn (at 2 a phase
        // of 0.25 lights every row alike, a phase of 0 alternates full and none, and the scroll sweeps between them -
        // blinking lines and moire bands); so the profile fades to its own mean (|sin|^e averaged over a line, a fit
        // within 0.005) from 3 rows per line down to 2. At 3 and more the profile is not faded.
        const float khc_dy = 1.0f + khc_curv * khc_r2 * (1.0f + 0.8f * khc_r2)
                           + 2.0f * khc_cc.y * khc_cc.y * khc_curv * (1.0f + 1.6f * khc_r2);
        const float khc_rows = fxMeta.w / max(khc_lines * khc_dy, 1.0e-4f);
        const float khc_mean = 0.88611f + khc_bex * (0.05438f * khc_bex - 0.30191f);
        if (khc_rows < 3.0f) khc_beam = lerp(khc_mean, khc_beam, smoothstep(2.0f, 3.0f, khc_rows));
        khc_col *= lerp(1.0f, khc_beam * 1.32f, saturate(fxParams0.y));

        // Aperture grille (CRT_MASK): R, G and B stripes, one triad maskSizePx wide (fxParams2.z, pixels of a
        // 1080-row picture - FX_PX_REF - at least 1, default 3), each stripe's share of this pixel
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

        // Rolling sync band: a soft dark bar drifting down the frame and wrapping (the vertical-hold-slipping read).
        float khc_bpos = frac(t * 0.11f);
        float khc_bd = abs(khc_duv.y - khc_bpos);
        khc_bd = min(khc_bd, 1.0f - khc_bd);
        float khc_band = 1.0f - smoothstep(0.0f, 0.16f, khc_bd);
        khc_col *= 1.0f - khc_band * khc_band * 0.22f * saturate(fxParams1.z);

        // Mains flicker: a fast beat plus a per-refresh random sparkle quantized to 60 Hz.
        float khc_fl = saturate(fxParams1.y);
        float khc_hum = KhSin(t * 100.0f * 3.14159265f) * 0.5f + 0.5f;   // HASH.
        float khc_spark = Hash(float2(floor(t * 60.0f), 3.7f)) - 0.5f;
        khc_col *= 1.0f + khc_fl * (khc_hum * 0.04f + khc_spark * 0.05f);

        // Phosphor tint, then the tube face: rounded-rectangle mask in the distorted space (out-of-range refraction
        // lands outside it - the bezel is black), a ~2 px feathered edge and a soft glass falloff into the corners.
        khc_col *= color.rgb;
        // FX_ROUND: the corners measured on the picture - x in heights (khc_as, the aspect) - so they are round and
        // the feather is 2 px on both axes (in uv they would be ellipses 1.78 x wider at 16:9 and the feather 3.6 px
        // across x). cornerRadius is in heights, at most half the shorter side.
        const float khc_as = fxMeta.z / fxMeta.w;
        float khc_cr = clamp(fxParams1.w, 0.003f, 0.5f * min(khc_as, 1.0f));
        float2 khc_q = abs(khc_duv - 0.5f) * float2(khc_as, 1.0f);
        float2 khc_ex = max(khc_q - (float2(0.5f * khc_as, 0.5f) - khc_cr), 0.0f);
        float khc_cd = length(khc_ex);
        float khc_feather = 2.0f / fxMeta.w;
        float khc_tube = 1.0f - smoothstep(khc_cr - khc_feather, khc_cr, khc_cd);
        khc_tube *= 1.0f - smoothstep(0.0f, khc_cr, khc_cd) * 0.35f;
        outc = khc_col * khc_tube;
    }
// FX_SPLIT: the SSGI gather (22) and a-trous (25) are their own entry points at the unit's tail (below:
// PSSsgiGather, PSSsgiAtrous), as are dynamicLightFog's gather and filter (32, 33).
    else if (effect == 24)
    {
        float3 khr_gi = float3(0.0f, 0.0f, 0.0f);
        float khr_cd = KhLinZ(px);   // FX_LINZ.
        if (khr_cd < 1e8f)
        {
            float khr_f = KhEncFence();   // Continuity belt below.
            float khr_sp = fxParams2.w > 0.5f ? clamp(fxParams2.w, 1.0f, 8.0f) : 3.0f;   // Auto width.
            float khr_ws = 0.0f;
            // SSGI_JBU: joint bilateral upsample (Kopf et al.: each low-resolution texel weighed by the guide at
            // its own position) of the grid's radiance, guided by the GRID's own depth. Each tap is a bilinear read
            // of the grid at full pixel q (normalized coords address the grid by its texel centres, SSGI_CENTRE);
            // its depth weight is that of the least agreeing of the four texels the read mixes, Gathered at the same
            // coords from t8 (texel t = the depth at KhSsgiFull(t), whose bounce the gather computed), so a texel
            // across a depth edge never enters the pixel's mean. Weighing a tap by the full-res depth at q alone takes
            // a background texel at the foreground's full weight within a texel of a silhouette (a dip of the bounce
            // there and a leak the other way, each up to 30 %). Where the four depths equal q's (a surface square to
            // the view) this is the 5 x 5 blur at the resolve width bit for bit; on a slope a tap takes its least
            // agreeing texel's weight, its farthest by up to a texel (ground at 4K: within 0.0013 of q's weighting on
            // a smooth field, its noise up to 11 % higher at 50 m). A read on a texel centre (one in b wherever
            // grid / frame reduces to a / b with both odd - a third where the grid is exactly a third of the frame,
            // setSsgiScale 1 / 3 on sides that are multiples of 3) also tests its zero-weight neighbour: near an edge
            // such a tap may drop where it could count. The q-depth weighting (the full-res depth at q as the guide)
            // runs when unarmed (matCtl.w: no copy this run) and when the grid is the frame's size (setSsgiScale 1:
            // every read is one texel, its depth q's, so it is the joint bilateral weighting there).
            // SSGI_JBU_FB: no tap agreeing (weight sum <= 1e-4) is one of two cases. Where the pixel's depth lies
            // within the centre read's four texel depths (widened by the tolerance) the surface runs on through the
            // pixel (ground at a grazing angle, its depth past the tolerance from texel to texel): no edge to keep,
            // so the taps run again under the q-depth weighting (the nearest texel alone would show the grid's F x F
            // blocks there). Elsewhere the pixel shows what no texel about it holds (a wire, a post thinner than a
            // texel, its every read straddling): it takes the centre read's texel nearest its depth
            // (dynamicLightFog's composite falls back so over its 3 x 3).
            // Cost against the q-depth weighting, per full-res pixel: the same 25 + 25 fetches (Gathers of t8, a
            // quarter of t7's texels, for the loads of t7), about +150 ALU (four differences and three max a tap for
            // one difference), a size query; where no tap agrees, its 25 taps once more (grazing ground) or three
            // Gathers (the nearest texel).
            uint khr_gw, khr_gh;
            khsgTex.GetDimensions(khr_gw, khr_gh);
            const bool khr_ga = matCtl.w > 0.5f && matCtl.w < 1.5f &&
                                ((float)khr_gw != fxMeta.z || (float)khr_gh != fxMeta.w);
            bool khr_bw = !khr_ga;   // The q-depth weighting (SSGI_JBU).
            float4 khr_z0 = float4(khr_cd, khr_cd, khr_cd, khr_cd);   // The centre read's four depths.
            [loop] for (int khr_k = 0; khr_k < 2; ++khr_k)
            {
                khr_gi = float3(0.0f, 0.0f, 0.0f);
                khr_ws = 0.0f;
                [unroll] for (int khr_j = -2; khr_j <= 2; ++khr_j)
                [unroll] for (int khr_i = -2; khr_i <= 2; ++khr_i)
                {
                    int2 khr_p = int2(i.pos.xy + float2(khr_i, khr_j) * khr_sp);
                    if (khr_p.x < 0 || khr_p.y < 0 ||
                        khr_p.x >= (int)fxMeta.z || khr_p.y >= (int)fxMeta.w) continue;
                    const float2 khr_uv = (float2(khr_p) + 0.5f) / float2(fxMeta.z, fxMeta.w);
                    float4 khr_d;
                    [branch] if (!khr_bw) {
                        khr_d = khGridZS.Gather(khsgSamp, khr_uv);
                    } else {
                        const float khr_lz = KhLinZ(khr_p);
                        khr_d = float4(khr_lz, khr_lz, khr_lz, khr_lz);
                    }
                    if (khr_i == 0 && khr_j == 0 && !khr_bw) khr_z0 = khr_d;
                    const float4 khr_e = abs(khr_d - khr_cd);   // A sky texel (1e9) weighs exp(-huge) = 0.
                    float khr_dz = max(max(khr_e.x, khr_e.y), max(khr_e.z, khr_e.w)) / (khr_cd * 0.06f + 0.05f);
                    float khr_w = exp(-0.125f * (khr_i * khr_i + khr_j * khr_j))
                                * exp(-khr_dz * khr_dz);
                    khr_gi += khsgTex.SampleLevel(khsgSamp, khr_uv, 0.0f).rgb * khr_w;
                    khr_ws += khr_w;
                }
                if (khr_ws > 1e-4f || khr_bw) break;   // The q-depth weighting's centre tap weighs 1: no third pass.
                const float khr_tl = khr_cd * 0.06f + 0.05f;   // SSGI_JBU_FB.
                const float khr_lo = min(min(khr_z0.x, khr_z0.y), min(khr_z0.z, khr_z0.w)) - khr_tl;
                const float khr_hi = max(max(khr_z0.x, khr_z0.y), max(khr_z0.z, khr_z0.w)) + khr_tl;
                if (!(khr_cd >= khr_lo && khr_cd <= khr_hi)) break;
                khr_bw = true;
            }
            [branch] if (!(khr_ws > 1e-4f)) {
                // Gather's components: w = the read's (lo.x, lo.y), z = (lo.x + 1, lo.y), x = (lo.x, lo.y + 1),
                // y = (lo.x + 1, lo.y + 1) - the same four texels for khr_z0 and the colours, one coordinate.
                const float2 khr_uv0 = (float2(px) + 0.5f) / float2(fxMeta.z, fxMeta.w);
                const float4 khr_cr = khsgTex.GatherRed(khsgSamp, khr_uv0);
                const float4 khr_cg = khsgTex.GatherGreen(khsgSamp, khr_uv0);
                const float4 khr_cb = khsgTex.GatherBlue(khsgSamp, khr_uv0);
                const float4 khr_e0 = abs(khr_z0 - khr_cd);
                float khr_be = khr_e0.w;
                khr_gi = float3(khr_cr.w, khr_cg.w, khr_cb.w);
                if (khr_e0.z < khr_be) { khr_be = khr_e0.z; khr_gi = float3(khr_cr.z, khr_cg.z, khr_cb.z); }
                if (khr_e0.x < khr_be) { khr_be = khr_e0.x; khr_gi = float3(khr_cr.x, khr_cg.x, khr_cb.x); }
                if (khr_e0.y < khr_be) { khr_be = khr_e0.y; khr_gi = float3(khr_cr.y, khr_cg.y, khr_cb.y); }
                khr_ws = 1.0f;
            }
            khr_gi = khr_ws > 1e-4f ? khr_gi / khr_ws : float3(0.0f, 0.0f, 0.0f);
            // Fade before the fence so the silhouette against the sky branch's hard zero has nothing to teeter on.
            khr_gi *= 1.0f - saturate((khr_cd - khr_f * 0.98f)
                                      / max(khr_f * 0.019f, 1.0f));
            float khr_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                                 + 0.00583715f * i.pos.y));
            float khr_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                  + 0.00583715f * (i.pos.y + 5.588238f)));
            // SSGI_DITHER_CH: gated per channel - one offset gated by the luma would reach a zero channel of a
            // saturated bounce and take it (giOnly's output, and scene + it) below 0. For c >= 0 and a gate s(c)
            // = smoothstep(0, 1.5 / 255, c): c - s(c) / 255 = t (2 t^2 - 3 t + 1.5) / 255 >= 0 (t = 170 c below
            // the knee; the quadratic has no real root), so no channel goes negative; still zero-mean.
            khr_gi += (khr_ig - khr_ig2) * (1.0f / 255.0f)
                    * smoothstep(float3(0.0f, 0.0f, 0.0f), float3(1.5f, 1.5f, 1.5f) / 255.0f, khr_gi);
            // WSEE: SSGI keeps the front depth - the bounce lands on the layers, and on what lies behind them as
            // on the layers where that is geometry (both surfaces lit, assumed alike). Where the sky shows behind them
            // only the layers' share (1 - Tr) of the pixel is a surface: the bounce takes that share (none through a
            // fully transparent layer). An approximation (one Tr for the whole bounce; no receiver of its own behind
            // the layers); per pixel (sample 0), not per sample.
            [branch] if (KhWseeArmed()) {
                float3 khr_wo;   // (front raw, behind raw, G) - the depth behind the layers in .y.
                const float khr_wt = KhWseeAt(px, LoadDepthPS(px), khr_wo);
                if (khr_wo.y > khr_wo.x) {
                    if (LinDepth(khr_wo.y) > 1e8f) khr_gi *= 1.0f - khr_wt;
                }
            }
        }
        outc = fxParams2.y > 0.5f ? khr_gi : scene + khr_gi;
    }

    else if (effect == 26)
    {
        // The radiance seed (SSGI_SEED; internal id: effect_id_from_gv refuses 24 - 30 to a script, and DLF's
        // 32 / 33 lie past MAX_EFFECT, so none is reachable from SQF; the flush synthesizes this ahead of the
        // gather). i.pos spans the scaled grid, so the full-frame uv is its texel's normalized centre on the grid
        // (SSGI_CENTRE: the block's centre, each axis by its own factor, as the resolve reads the grid).
        const float2 khrs_g = KhSsgiGrid();
        const float2 khrs_uv = i.pos.xy / khrs_g;
        // GI_FINITE: held to [0, 65504] (the 16F range; min / max take the non-NaN operand, so a NaN reads 0)
        // - unclamped, a NaN pixel (a broken user shader's) seeds every gather sample that reaches it and the
        // a-trous passes spread it ~35 px round. A finite non-negative scene passes unchanged.
        float3 khrs_c = min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv, 0.0f).rgb, 0.0f), 65504.0f);
        // SSGI_SEED_BOX: one bilinear tap at the centre reads at most the middle 2 x 2 full-res pixels - the whole
        // block at a factor of 2 (setSsgiScale 0.5), but 4 of 16 at 4 and at most 4 of 9 at 3 (the middle pixel alone
        // where the side is a multiple of 3: the centre is that pixel's own) - so with that tap alone a small bright
        // detail would leave the seed and come back as the camera moves, the bounce flickering with it. Past a factor
        // of 2 on either axis, four bilinear taps a quarter block about the centre cover the block: a box exactly at 4,
        // every pixel at 3 (the middle row and column at two thirds the weight); at 2 and below the single tap is the
        // whole block. Non-default scales only; there the seed draws on 1 / 9 - 1 / 16 of the pixels.
        const float2 khrs_f = float2(fxMeta.z, fxMeta.w) / khrs_g;
        [branch] if (max(khrs_f.x, khrs_f.y) > 2.01f)
        {
            const float2 khrs_q = 0.25f / khrs_g;
            const float2 khrs_r = float2(khrs_q.x, -khrs_q.y);
            khrs_c = 0.25f * (min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv + khrs_q, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv - khrs_q, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv + khrs_r, 0.0f).rgb, 0.0f), 65504.0f)
                            + min(max(sceneColor.SampleLevel(khsgSamp, khrs_uv - khrs_r, 0.0f).rgb, 0.0f), 65504.0f));
        }
        return float4(khrs_c, 1.0f);
    }
    else if (effect == 23)   // Fogscatter
    {
        // FS_SPLIT: the scene chain draws PSFsSplitS (the unit's tail) where side 28 is armed - this branch's TWIN,
        // its code line for line (five precise marks and khfs_rm's MSAA_DEPTH 0 form added there, the two side tests
        // resolved armed and the four lanes only KhFsFogW reads dropped): an edit here is made there too.
        float2 khfs_res = float2(fxMeta.z, fxMeta.w);
        const float2 khfs_ms = KhFxProjScale();
        float khfs_m00 = khfs_ms.x;
        float khfs_m11 = khfs_ms.y;
        float khfs_in = max(fxParams0.x, 0.0f);
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * KhFxPx()   // FX_PX_REF.
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        float khfs_cd = KhLinZ(px);   // FX_LINZ.
        // FX_SIDE: every tap needs its source pixel's fog (KhFsFog: a matrix transform and two or three
        // exponentials), which each receiver the source reaches would recompute. The scene chain draws every
        // pixel's once before the pass (side id 28, the same call on the same depth) and arms matCtl.y; the pass
        // reads it. Unarmed (effect meshes, the PIP, a failed side draw) each tap computes it.
        const bool khfs_side = matCtl.y > 0.5f;
        float khfs_f0;
        [branch] if (khfs_side) khfs_f0 = khFxSide.Load(int3(px, 0));
        else                    khfs_f0 = KhFsFogW(px, khfs_cd, khfs_res, khfs_m00, khfs_m11);   // WSEE.
        float khfs_sc = saturate(khfs_f0 * khfs_in);
        // FS_SELF (cb.hlsl's KhFsOwnW): the receiver's own term - its unscattered share and its own pixel's share of
        // its own scatter cone. Do not weight it as a whole tap (3 rm^2 / n / rc^2, about 38x at the defaults): every
        // receiver then keeps an extra 3 / (n sc) of its own sharp value - a light's core through thick fog, a
        // one-pixel step at a silhouette where everything is fogged, and a look that changes with the taps. (A near
        // tap's pyramid footprint, GLOW_PYR, still covers the pixel - about 0.7 of its share again.)
        const float khfs_w0 = KhFsOwnW(khfs_sc, khfs_rm);
        float khfs_ws = khfs_w0;
        float3 khfs_acc = scene * khfs_ws;
        // FS_SKY: the most fog a NEARER source that reaches this receiver puts on its ray (R, below the loop).
        float khfs_fx = khfs_sc;
        float khfs_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                              + 0.00583715f * i.pos.y));
        float khfs_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                               + 0.00583715f * (i.pos.y + 5.588238f)));
        float khfs_rot = khfs_ig * 6.2831853f;
        float khfs_hn = (float)((khfs_n + 1) >> 1);
        // GLOW_PYR: each tap's share of the disc (pi rm^2 / n) is a square rm sqrt(pi / n) on a side; a tap's colour
        // is the picture averaged over it, so the per-pixel rotation does not show as grain. (FS_MIX below leaves
        // the footprint the even disc's: it sets the look - a tap stands for the picture around it - while the mixture
        // only decides where the taps land and what each one weighs.)
        const float khfs_gs = khfs_rm * sqrt(3.14159265f / (float)khfs_n);
        const float khfs_gm = KhGlowMix(khfs_gs);
        // FS_MIX: a source scatters over a disc of rm x its scatter (khfs_rk below). In thin fog that disc is a few
        // pixels; with the taps spread evenly over the full radius rm only the pixels whose first tap happens to land
        // inside it take a share, each at the full disc's weight: sparse bright points around every light. Most sources
        // that reach this receiver scatter about as much as it does or less - a farther one takes the receiver's own by
        // the deflection gate (FS_SKY), and a nearer one, on nearly the same view ray, has crossed less fog - so the
        // radii come from a mixture: a quarter of the distribution over the receiver's own disc ri (rm x its scatter,
        // at least 2 px), three quarters evenly over rm, still one stratum per tap pair. Each tap is weighted by the
        // mixture's density where it lands (the balance heuristic) - pi / (2 hn qi) of the disc's area inside ri,
        // pi / (2 hn qo) outside (2 hn = n for an even count; FS_ODD below), in place of pi rm^2 / n - so it
        // estimates the same sum. Where a nearer source does scatter more (a sky whose fog is below a nearer object's:
        // a fog pass's skyAmount under its ramp, the far fence's feather, the engine's fog-end ramp against its sky;
        // or, in the fallback fog mode, a higher farther surface), the even three quarters still find it, with up to
        // about a half more grain there than the even disc (none where the two fogs are close); the quarter is that
        // trade's balance. At ri = rm the mixture is the even disc.
        const float khfs_ri = min(max(khfs_rm * khfs_sc, 2.0f), khfs_rm);
        const float khfs_qo = 0.75f / (khfs_rm * khfs_rm);   // The density outside ri, x pi.
        const float khfs_qi = 0.25f / (khfs_ri * khfs_ri) + khfs_qo;   // ...and inside it.
        const float khfs_fi = khfs_qi * khfs_ri * khfs_ri;   // The share of the distribution inside ri.
        // FS_ODD: a stratum (a tap pair) holds 1 / hn of the distribution, so a tap of a pair stands for half of it,
        // 1 / (2 hn) - 1 / n for an even count - and an odd count's lone last tap for all of it (doubled below).
        // Weighted 1 / n each, an odd count's full strata would take (n + 1) / n of their share and the lone one
        // (n + 1) / 2n (5 taps: the inner rings 1.2x, the outer 0.6x - a tighter halo). The footprint (khfs_gs)
        // keeps 1 / n: a look.
        const float khfs_nt = 2.0f * khfs_hn;
        const float khfs_cwi = 3.0f / (khfs_nt * khfs_qi);   // The cone's 3 / pi x the tap's share of the area.
        const float khfs_cwo = 3.0f / (khfs_nt * khfs_qo);

        [loop] for (int khfs_k = 0; khfs_k < khfs_n; ++khfs_k)
        {
            int khfs_kp = khfs_k >> 1;
            float khfs_an = khfs_kp * 2.3999632f + khfs_rot + (khfs_k & 1) * 3.14159265f;
            // FS_MIX: the stratum's point of the mixture's distribution, inverted to a radius (both arms are
            // finite; the select comes before the root).
            const float khfs_u = (khfs_kp + khfs_ig2) / khfs_hn;
            const bool  khfs_inr = khfs_u < khfs_fi;
            float khfs_sr = max(sqrt(khfs_inr ? khfs_u / khfs_qi
                                              : khfs_ri * khfs_ri + (khfs_u - khfs_fi) / khfs_qo), 1.0f);
            const float2 khfs_pc = float2(px) + 0.5f + float2(cos(khfs_an), sin(khfs_an)) * khfs_sr;
            int2 khfs_sp = int2(khfs_pc);
            // Off-screen taps are absent information: skipping them leaves the closing sum-normalization to
            // renormalize, so edge receivers lean on their surviving weights instead of dimming.
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            float khfs_sd = KhLinZ(khfs_sp);
            float khfs_fs;
            [branch] if (khfs_side) khfs_fs = khFxSide.Load(int3(khfs_sp, 0));   // FX_SIDE.
            else                    khfs_fs = KhFsFogW(khfs_sp, khfs_sd, khfs_res, khfs_m00, khfs_m11);   // WSEE.
            float khfs_ss = saturate(khfs_fs * khfs_in);
            // Deflection gate (FS_SKY - G): a farther source's light reaches this receiver through this receiver's
            // own column, so it scatters with this receiver's fog - also where the farther source reads less (a sky
            // whose fog is below a ramp-fogged ridge in front of it), which min(ss, sc) would keep off the ridge.
            khfs_ss = KhFsSourceFog(khfs_ss, khfs_sc, khfs_sd, khfs_cd);
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // This source's disc does not reach.
            // FS_SKY - R: a nearer source that scatters more - this ray runs beside its column and crossed that fog
            // too. Tapered over the source's disc, as its weight is, so the floor fades out with its reach.
            khfs_fx = KhFsNearFloor(khfs_fx, khfs_ss, khfs_sr, khfs_rk, khfs_sd, khfs_cd);
            float khfs_w = khfs_ss * (khfs_inr ? khfs_cwi : khfs_cwo) * (1.0f - khfs_sr / khfs_rk)
                         / max(khfs_rk * khfs_rk, 1.0f);   // FS_MIX: the tap's own share.
            if ((khfs_k | 1) >= khfs_n) khfs_w *= 2.0f;   // FS_ODD: an odd count's lone last tap.
            float3 khfs_c;
            if (khfs_gm >= 1.0f)      khfs_c = KhGlowTap(khfs_pc, khfs_gs);
            else if (khfs_gm <= 0.0f) khfs_c = SampleScene(khfs_sp);
            else                      khfs_c = lerp(SampleScene(khfs_sp), KhGlowTap(khfs_pc, khfs_gs), khfs_gm);
            khfs_acc += khfs_c * khfs_w;
            khfs_ws += khfs_w;
        }

        // FS_SKY - R: beside a nearer source that scatters more (a sky or past-the-fog-end receiver beside a ridge
        // the ramp has fogged), the receiver's own term at that fog - smaller, so more of the neighbours' scatter shows
        // and the step at the silhouette softens. The own term (FS_SELF) falls with the fog everywhere - 1 - 0.045 f
        // while the cone is under a pixel, 1 - f + 0.955 / (rm^2 f) after - so dw < 0 whenever fx > sc; the min is a
        // guard (never less bleed than without R). fx > sc only beside such a source; elsewhere this is skipped.
        [branch] if (khfs_fx > khfs_sc) {
            const float khfs_dw = min(KhFsOwnW(khfs_fx, khfs_rm) - khfs_w0, 0.0f);
            khfs_acc += scene * khfs_dw;
            khfs_ws += khfs_dw;
        }
        // FS_FLOOR: ws never falls below the own weight at full fog, KhFsOwnW(1, rm) = 0.955 / max(rm, 1)^2 - the
        // least own weight at any fog; the taps only add, and R takes the own term down to KhFsOwnW(fx) at most - so
        // the guard is half of it, which only rounding could reach. Do not use a fixed 1e-4 bound: under FS_SELF,
        // once rm passes 98 px (a radius param past ~49 at 2160 rows), a fully fogged receiver no tap reaches darkens
        // to w0 / 1e-4.
        outc = khfs_acc / max(khfs_ws, 0.5f * KhFsOwnW(1.0f, khfs_rm));
    }
    else if (effect == 28)   // FX_SIDE: fog scatter's per-pixel fog (KhFsFogW), drawn before the pass.
    {
        // WSEE_EDGE_COST: the scene chain draws PSFsSide (the unit's tail) - this branch's TWIN; an edit here is
        // made there too.
        // Fog scatter's own inputs, the same expressions (its lanes are this pass's).
        const float2 khfa_res = float2(fxMeta.z, fxMeta.w);
        const float2 khfa_ms = KhFxProjScale();
        const float khfa_m00 = khfa_ms.x;
        const float khfa_m11 = khfa_ms.y;
        return float4(KhFsFogW(px, KhLinZ(px), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);   // WSEE.
    }
    else if (effect == 31)   // DLF: dynamicLightFog - the grid's fog (t3) upsampled and added to the scene.
    {
        // KH_FX_LEAN: the scene chain draws PSDlfComp (below PSFsSplitS) on an unlocalized, unbanded pass - this
        // branch's TWIN, its code line for line: an edit here is made there too.
        // Joint bilateral upsample (SSAO_SEP's recipe): the four grid texels around the pixel, bilinear weights
        // times depth-key agreement; none agreeing, the nearest in depth of the 3 x 3. Unarmed (dlCtl.x: no light
        // selected, the ring, the grid or the gather / filter entry missing, an unarmed camera, no medium -
        // DLF_MEDIUM0) outc stays the scene: under blend 0 / 4 / 5 the pass leaves the picture as it was; additive,
        // multiply and screen blend it into itself (KhFxFinish), as the C++ chain's skip rule says.
        [branch] if (dlCtl.x >= 0.5f) {
            uint khdc_w, khdc_h;
            khsgTex.GetDimensions(khdc_w, khdc_h);
            const float2 khdc_gd = float2((float)max(khdc_w, 1u), (float)max(khdc_h, 1u));
            const int2   khdc_hi = int2(khdc_gd) - 1;
            const float  khdc_cd = KhDlfKey(px);
            const float2 khdc_g = (float2(px) + 0.5f) * khdc_gd / float2(fxMeta.z, fxMeta.w) - 0.5f;
            const int2   khdc_t0 = int2(floor(khdc_g));
            const float2 khdc_f = khdc_g - floor(khdc_g);
            float3 khdc_acc = float3(0.0f, 0.0f, 0.0f);
            float  khdc_ws = 0.0f;
            [unroll] for (int khdc_j = 0; khdc_j <= 1; ++khdc_j)
            [unroll] for (int khdc_i = 0; khdc_i <= 1; ++khdc_i)
            {
                const int2  khdc_t = clamp(khdc_t0 + int2(khdc_i, khdc_j), int2(0, 0), khdc_hi);
                const float khdc_bw = (khdc_i ? khdc_f.x : 1.0f - khdc_f.x) * (khdc_j ? khdc_f.y : 1.0f - khdc_f.y);
                const float khdc_z = abs(KhDlfKeyG(khdc_t, khdc_gd) - khdc_cd) / (khdc_cd * 0.06f + 0.05f);
                const float khdc_wt = khdc_bw * exp(-khdc_z * khdc_z);
                khdc_acc += khsgTex.Load(int3(khdc_t, 0)).rgb * khdc_wt;
                khdc_ws += khdc_wt;
            }
            if (!(khdc_ws > 1.0e-4f)) {
                const int2 khdc_c = int2(floor(khdc_g + 0.5f));
                float khdc_best = 3.0e38f;
                float khdc_best2 = 3.0e38f;   // DLF_FOGREC: the unclamped distance, the tie-break.
                khdc_acc = float3(0.0f, 0.0f, 0.0f);
                khdc_ws = 1.0f;
                [unroll] for (int khdc_n = -1; khdc_n <= 1; ++khdc_n)
                [unroll] for (int khdc_m = -1; khdc_m <= 1; ++khdc_m)
                {
                    const int2  khdc_t = clamp(khdc_c + int2(khdc_m, khdc_n), int2(0, 0), khdc_hi);
                    // DLF_FOGREC: the nearest key clamped to maxDistance (the sky and everything
                    // past maxDistance compare equal, the feather's band too while maxDistance lies below it), ties
                    // broken by the unclamped key - a pixel of far terrain among sky texels takes theirs, not a nearer
                    // surface's, and a sky pixel keeps a sky texel where one is in reach. Without a fog pass drawn
                    // after this one the keys are already clamped (KhDlfKeyZ), the two distances equal, and the choice
                    // is the plain nearest key's, bit for bit.
                    const float khdc_k = KhDlfKeyG(khdc_t, khdc_gd);
                    const float khdc_e = abs(min(khdc_k, KhDlfMaxD()) - min(khdc_cd, KhDlfMaxD()));
                    const float khdc_e2 = abs(khdc_k - khdc_cd);
                    [flatten] if (khdc_e < khdc_best || (khdc_e == khdc_best && khdc_e2 < khdc_best2)) {
                        khdc_best = khdc_e;
                        khdc_best2 = khdc_e2;
                        khdc_acc = khsgTex.Load(int3(khdc_t, 0)).rgb;
                    }
                }
            }
            outc = scene + max(khdc_acc / khdc_ws, 0.0f);
        }
    }
     else if (effect == 101)   // 3D LUT grade (.cube, effect EFFECT_LUT): [strength, encode].
    {
        // Input is clamped to the LUT's [0,1] domain (the loader resamples non-identity domains onto [0,1], so no
        // domain math lives here). GetDimensions gives the LUT's size, so no lane carries it.
        uint khlW, khlH, khlD;
        khLut.GetDimensions(khlW, khlH, khlD);

        if (khlW >= 2)
        {
            // The write window consumes post-tonemap LDR already (direct lookup is correct there), so under encode 0
            // (fxParams0.y; 1 never, 2 always) the sandwich keys on the phase discriminator (scene lanes w <= 1.0).
            // Encode -> LUT -> decode puts the lookup where the artist designed it and hands the tonemap a linear
            // result.
            bool khlSand = fxParams0.y < 0.5f ? (centerSize.w < 1.5f)
                                              : (fxParams0.y >= 1.5f);
            float3 khlIn = saturate(scene);
            if (khlSand) khlIn = pow(khlIn, 1.0f / 2.2f);
            float3 khlC = khlIn * (float)(khlW - 1);
            int khlHi = (int)khlW - 2;
            int3 khlI0 = clamp(int3(khlC), int3(0, 0, 0), int3(khlHi, khlHi, khlHi));
            float3 khlF = khlC - (float3)khlI0;
            float khlR = khlF.x, khlG = khlF.y, khlB = khlF.z;
            float3 khlV000 = KhLutV(khlI0);
            float3 khlV111 = KhLutV(khlI0 + int3(1, 1, 1));
            float3 khlOut;

            if (khlR >= khlG && khlG >= khlB)
                khlOut = (1.0f - khlR) * khlV000 + (khlR - khlG) * KhLutV(khlI0 + int3(1, 0, 0))
                       + (khlG - khlB) * KhLutV(khlI0 + int3(1, 1, 0)) + khlB * khlV111;
            else if (khlR >= khlB && khlB >= khlG)
                khlOut = (1.0f - khlR) * khlV000 + (khlR - khlB) * KhLutV(khlI0 + int3(1, 0, 0))
                       + (khlB - khlG) * KhLutV(khlI0 + int3(1, 0, 1)) + khlG * khlV111;
            else if (khlB >= khlR && khlR >= khlG)
                khlOut = (1.0f - khlB) * khlV000 + (khlB - khlR) * KhLutV(khlI0 + int3(0, 0, 1))
                       + (khlR - khlG) * KhLutV(khlI0 + int3(1, 0, 1)) + khlG * khlV111;
            else if (khlG >= khlR && khlR >= khlB)
                khlOut = (1.0f - khlG) * khlV000 + (khlG - khlR) * KhLutV(khlI0 + int3(0, 1, 0))
                       + (khlR - khlB) * KhLutV(khlI0 + int3(1, 1, 0)) + khlB * khlV111;
            else if (khlG >= khlB && khlB >= khlR)
                khlOut = (1.0f - khlG) * khlV000 + (khlG - khlB) * KhLutV(khlI0 + int3(0, 1, 0))
                       + (khlB - khlR) * KhLutV(khlI0 + int3(0, 1, 1)) + khlR * khlV111;
            else
                khlOut = (1.0f - khlB) * khlV000 + (khlB - khlG) * KhLutV(khlI0 + int3(0, 0, 1))
                       + (khlG - khlR) * KhLutV(khlI0 + int3(0, 1, 1)) + khlR * khlV111;

            // Decode the graded result back to linear when sandwiched (negative
            // lattice values are legal in .cube; clamp before the pow).
            if (khlSand) khlOut = pow(max(khlOut, 0.0f), 2.2f);

            // color.rgb tints after the grade; strength lerps against the untouched scene before the localization/band
            // masks, which multiply in as everywhere else.
            outc = lerp(scene, khlOut * color.rgb, saturate(fxParams0.x));
        }
    }

    // FX_USER: the finish - the localization and band masks, the UI lane's coverage, opacity and blend mode, the
    // fused stages and the packing - is KhFxFinish (cb.hlsl's KH_FX_UNIT section), one body with the user effects'.
    return KhFxFinish(scene, outc, i.pos.xy);
}

// ---- FX_SPLIT - the effect chain's heavy side draws, each its own entry point. --------------------------------
// The SSGI gather (22) and a-trous (25), dynamicLightFog's gather (32) and grid filter (33), not branches of PSEffect:
// one pixel shader's register allocation is that of its heaviest branch, so every effect would pay for these loops.
// Each takes PSEffect's KhFxBegin and px as its prologue (no scene sample, no UI coverage probe: no body reads the
// sample, and every chain pass has centerSize.w = 1). C++ draws them where the chain draws those ids
// (fx_chain_run); no other route draws them (the PIP, the UI lane and effect meshes skip SSGI and
// dynamicLightFog). FS_SPLIT: fog scatter's own entry for the scene chain (PSFsSplitS, below PSFxLinZ), PSEffect's
// branch 23 kept for every other route.

// Effect 22 - the SSGI gather (SSGI_VB).
float4 PSSsgiGather(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // Grain-style time seeds are forbidden in this entry - a static frame renders bit-identical, so all shimmer
        // reduces to real scene/camera change. Normals come from the smaller-delta side per axis (a naive derivative
        // cross paints edge pixels with cross-object normals). Visibility is SSGI_VB's bitmask (effect.hlsl): the
        // result is the slices' sector-weighted sum, normalized by the sector count, never by surviving weight
        // (weight-sum division turns one surviving bright tap into a sparkling pixel).
        float3 khg_b = float3(0.0f, 0.0f, 0.0f);
        // The resolve upsamples the result depth-guided at full res.
        float khg_inv = localParams0.y >= 0.25f ? localParams0.y : 2.0f;
        int khg_st = max((int)(khg_inv + 0.5f), 1);
        int2 khg_fpx = KhSsgiFull(px);   // SSGI_CENTRE: the block's centre pixel.
        float khg_cd = KhLinZ(khg_fpx);
        float khg_rng = 1.0f;
        if (fxParams1.z > 1.0f)
            khg_rng = 1.0f - smoothstep(fxParams1.z * 0.7f, fxParams1.z, khg_cd);
        if (khg_cd < 1e8f && khg_rng > 0.001f && depthParams.y < -1.0e-3f)
        {
            float2 khg_res = float2(fxMeta.z, fxMeta.w);
            // Reconstruct in view space (KhgVpos), camera-relative by
            // construction, so no world-scale cancellation occurs.
            float khg_m00 = max(length(float3(viewProj[0].x, viewProj[1].x, viewProj[2].x)), 1e-6f);
            float khg_m11 = max(length(float3(viewProj[0].y, viewProj[1].y, viewProj[2].y)), 1e-6f);
            float3 khg_P = KhgVpos(float2(khg_fpx), khg_cd, khg_res, khg_m00, khg_m11);   // Full-res space.
            int2 khg_pl = int2(max(khg_fpx.x - khg_st, 0), khg_fpx.y);
            int2 khg_pr = int2(min(khg_fpx.x + khg_st, (int)fxMeta.z - 1), khg_fpx.y);
            int2 khg_pu = int2(khg_fpx.x, max(khg_fpx.y - khg_st, 0));
            int2 khg_pd = int2(khg_fpx.x, min(khg_fpx.y + khg_st, (int)fxMeta.w - 1));
            float khg_dl = KhLinZ(khg_pl);
            float khg_dr = KhLinZ(khg_pr);
            float khg_du = KhLinZ(khg_pu);
            float khg_dd = KhLinZ(khg_pd);
            // SSGI_EDGE_N: a neighbour clamped onto the receiver itself (the frame's last column / row at a grid
            // factor of 2 or less - setSsgiScale 0.5 and above; the first below 2 - above 0.5) is absent - its zero
            // depth difference would win the nearer test and its zero vector make the cross 0: no bounce on that whole
            // column / row.
            bool khg_lx = khg_pr.x == khg_fpx.x ||
                          (khg_pl.x != khg_fpx.x && abs(khg_dl - khg_cd) < abs(khg_dr - khg_cd));
            bool khg_ly = khg_pd.y == khg_fpx.y ||
                          (khg_pu.y != khg_fpx.y && abs(khg_du - khg_cd) < abs(khg_dd - khg_cd));
            float3 khg_vx = KhgVpos(float2(khg_lx ? khg_pl : khg_pr),
                                    khg_lx ? khg_dl : khg_dr,
                                    khg_res, khg_m00, khg_m11) - khg_P;
            float3 khg_vy = KhgVpos(float2(khg_ly ? khg_pu : khg_pd),
                                    khg_ly ? khg_du : khg_dd,
                                    khg_res, khg_m00, khg_m11) - khg_P;
            float3 khg_N = cross(khg_vy, khg_vx);
            float khg_nl = length(khg_N);

            if (khg_nl > 1e-9f)
            {
                khg_N /= khg_nl;
                float3 khg_rd = khg_P / max(length(khg_P), 1e-4f);
                if (dot(khg_N, khg_rd) > 0.0f) khg_N = -khg_N;
                // Grazing confidence, per-pixel-ray form; the bilateral resolve eats the reconstruction noise this fade
                // guards against, so the window is narrow.
                float khg_conf = smoothstep(0.008f, 0.05f, abs(dot(khg_N, khg_rd)));
                float khg_rad = max(fxParams0.y, 0.1f);
                // 2048 is a sanity ceiling, not a design point.
                float khg_spx = clamp(khg_rad * khg_m11 * 0.5f * fxMeta.w / khg_cd, 3.0f, 2048.0f);
                int khg_n = clamp((int)fxParams0.z, 4, 32);
                float khg_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                                     + 0.00583715f * i.pos.y));
                float khg_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                      + 0.00583715f * (i.pos.y + 5.588238f)));
                float khg_bias = clamp(fxParams0.w, 0.0f, 0.9f);
                float khg_fp = max(fxParams1.x, 0.25f);
                float khg_acne = fxParams2.z > 0.001f ? fxParams2.z : 1.0f;
                float khg_pfl = (0.01f + khg_cd * 0.0035f) * khg_acne;
                // SSGI_VB: the receiver lifted off its own surface by the acne floor, so its reconstruction noise
                // holds no sector; the bias narrows the admitted cone about the normal (acos bias).
                const float3 khg_V = -khg_rd;
                const float3 khg_Pr = khg_P + khg_N * khg_pfl;
                const float  khg_hw = acos(khg_bias);
                // Slices and steps per side from the sample budget (two sides a slice): 12 -> 2 x 3, 16 -> 2 x 4, 32 ->
                // 4 x 4. A budget between those rounds down to 2 x slices x steps (20 takes 16, 5 takes 4).
                const int khg_ns = khg_n <= 8 ? 1 : (khg_n <= 16 ? 2 : 4);
                // UDIV: as uint (both > 0: khg_n 4 - 32, khg_ns 1 - 4) - fxc's X3556; the same quotient.
                const int khg_m = max((int)((uint)khg_n / (uint)(2 * khg_ns)), 1);
                float3 khg_acc = float3(0.0f, 0.0f, 0.0f);
                float khg_ws = 0.0f;

                [loop] for (int khg_si = 0; khg_si < khg_ns; ++khg_si)
                {
                    // The slice: a screen direction rotated per pixel (position only), and the plane through the view
                    // ray it spans - the view-space step of one pixel along it, at a unit depth (KhgVpos; pixels need
                    // not be square).
                    const float  khg_ph = ((float)khg_si + khg_ig) * (3.14159265f / (float)khg_ns);
                    const float2 khg_dir = float2(cos(khg_ph), sin(khg_ph));   // Screen, y down.
                    const float3 khg_dv = normalize(float3(khg_dir.x / (khg_m00 * khg_res.x),
                                                           -khg_dir.y / (khg_m11 * khg_res.y), 0.0f));
                    const float3 khg_o = normalize(khg_dv - khg_V * dot(khg_dv, khg_V));
                    const float3 khg_ax = cross(khg_o, khg_V);
                    const float3 khg_np = khg_N - khg_ax * dot(khg_N, khg_ax);   // The normal in the slice.
                    const float  khg_npl = length(khg_np);
                    if (khg_npl < 1.0e-4f) continue;
                    const float  khg_na = atan2(dot(khg_np, khg_o), dot(khg_np, khg_V));
                    const float  khg_c0 = KhSsgiCum(-khg_hw, khg_bias);
                    const float  khg_cr = KhSsgiCum(khg_hw, khg_bias) - khg_c0;
                    if (!(khg_cr > 1.0e-6f)) continue;
                    const float  khg_jk = frac(khg_ig2 + (float)khg_si * 0.61803399f);
                    uint   khg_mask = 0u;
                    float3 khg_sacc = float3(0.0f, 0.0f, 0.0f);

                    [loop] for (int khg_sd2 = 0; khg_sd2 < 2; ++khg_sd2)
                    {
                        const float  khg_sg = khg_sd2 == 0 ? 1.0f : -1.0f;
                        const float2 khg_sdir = khg_dir * khg_sg;
                        float3 khg_side = float3(0.0f, 0.0f, 0.0f);
                        int    khg_seen = khg_m;
                        [loop] for (int khg_k = 0; khg_k < khg_m; ++khg_k)
                        {
                            // Outward along the line, denser near the receiver, above a 2 px self-sample floor. The
                            // sample stands for its stretch of the line: half the gap to each neighbour (khg_h px
                            // either side).
                            const float khg_u = ((float)khg_k + khg_jk) / (float)khg_m;
                            const float khg_sr = max(khg_u * khg_u * khg_spx, 2.05f);
                            const float khg_h = max(khg_u * khg_spx / (float)khg_m, 0.5f);
                            const int2  khg_sp = int2(float2(khg_fpx) + 0.5f + khg_sdir * khg_sr);   // Full-res.
                            // Off-screen: the rest of this side lies farther out; the side is scaled up by the share it
                            // saw (below). int() truncates toward zero: a sample under one pixel left of (above) the
                            // frame reads the first column (row) at that pixel's own position, as every sample is
                            // rounded onto a pixel - on screen, not off.
                            if (khg_sp.x < 0 || khg_sp.y < 0 ||
                                khg_sp.x >= (int)fxMeta.z || khg_sp.y >= (int)fxMeta.w) {
                                khg_seen = khg_k;
                                break;
                            }
                            const float khg_sd = KhLinZ(khg_sp);
                            if (khg_sd >= 1e8f) continue;   // Sky: no surface, no bounce.
                            const float3 khg_S = KhgVpos(float2(khg_sp), khg_sd, khg_res, khg_m00, khg_m11);
                            // The sender's neighbours one resolve step right and down (its normal, below, and its
                            // plane): on one surface when both lie within the cross-object bound.
                            const int2  khg_nr = int2(min(khg_sp.x + khg_st, (int)fxMeta.z - 1), khg_sp.y);
                            const int2  khg_nd = int2(khg_sp.x, min(khg_sp.y + khg_st, (int)fxMeta.w - 1));
                            const float khg_dr2 = KhLinZ(khg_nr);
                            const float khg_dd2 = KhLinZ(khg_nd);
                            const float khg_xob = 0.15f * khg_sd + 0.5f;   // Cross-object bound.
                            const bool  khg_on = khg_dr2 < 1e8f && khg_dd2 < 1e8f &&
                                                 abs(khg_dr2 - khg_sd) < khg_xob && abs(khg_dd2 - khg_sd) < khg_xob;
                            // The footprint's plane takes a tight bound (a post's neighbour on the wall behind it is
                            // within the loose one) and a real step both ways (at the last row or column the step is
                            // clamped onto the sender itself).
                            const float khg_fob = 0.02f * khg_sd + 0.02f;
                            const bool  khg_fon = khg_nr.x > khg_sp.x && khg_nd.y > khg_sp.y &&
                                                  abs(khg_dr2 - khg_sd) < khg_fob && abs(khg_dd2 - khg_sd) < khg_fob;
                            // The footprint's ends on the sender's plane (1 / depth is affine across a plane in screen
                            // space; per pixel actually stepped); a sender on an edge is taken as a point.
                            const float khg_iz = 1.0f / khg_sd;
                            const float khg_izx = (1.0f / khg_dr2 - khg_iz) / max((float)(khg_nr.x - khg_sp.x), 1.0f);
                            const float khg_izy = (1.0f / khg_dd2 - khg_iz) / max((float)(khg_nd.y - khg_sp.y), 1.0f);
                            const float2 khg_ea = -khg_sdir * (khg_fon ? min(khg_h, khg_sr - 1.0f) : 0.0f);
                            const float2 khg_ez = khg_sdir * (khg_fon ? khg_h : 0.0f);
                            const float khg_iza = khg_iz + khg_izx * khg_ea.x + khg_izy * khg_ea.y;
                            const float khg_izz = khg_iz + khg_izx * khg_ez.x + khg_izy * khg_ez.y;
                            const float3 khg_Sa = KhgVpos(float2(khg_sp) + khg_ea, 1.0f / max(khg_iza, 0.5f * khg_iz),
                                                          khg_res, khg_m00, khg_m11);
                            const float3 khg_Sz = KhgVpos(float2(khg_sp) + khg_ez, 1.0f / max(khg_izz, 0.5f * khg_iz),
                                                          khg_res, khg_m00, khg_m11);
                            // Their backs, KH_SSGI_THICK behind along their own rays.
                            const float3 khg_Ba = khg_Sa + khg_Sa * (KH_SSGI_THICK / max(length(khg_Sa), 1e-4f));
                            const float3 khg_Bz = khg_Sz + khg_Sz * (KH_SSGI_THICK / max(length(khg_Sz), 1e-4f));
                            const float3 khg_qa = khg_Sa - khg_Pr, khg_qz = khg_Sz - khg_Pr;
                            const float3 khg_ra = khg_Ba - khg_Pr, khg_rz = khg_Bz - khg_Pr;
                            // Wholly below the receiver's (lifted) tangent plane: never above its horizon, whatever the
                            // rounding of the sample onto the line.
                            uint khg_bits = 0u;
                            if (max(max(dot(khg_qa, khg_N), dot(khg_qz, khg_N)),
                                    max(dot(khg_ra, khg_N), dot(khg_rz, khg_N))) > 0.0f) {
                                khg_bits = KhSsgiBits(float4(atan2(dot(khg_qa, khg_o), dot(khg_qa, khg_V)),
                                                             atan2(dot(khg_qz, khg_o), dot(khg_qz, khg_V)),
                                                             atan2(dot(khg_ra, khg_o), dot(khg_ra, khg_V)),
                                                             atan2(dot(khg_rz, khg_o), dot(khg_rz, khg_V))) - khg_na,
                                                      khg_bias, khg_hw, khg_c0, khg_cr);
                            }
                            const uint khg_new = khg_bits & ~khg_mask;
                            khg_mask = khg_mask | khg_bits;
                            if (khg_new == 0u) continue;
                            const float3 khg_v = khg_S - khg_P;
                            const float khg_d = length(khg_v);
                            if (khg_d < 1e-4f || khg_d > khg_rad) continue;
                            // Its light: the radiance its face sends the receiver's way, if that face turns toward it.
                            // A Lambertian face's radiance is the same every way, so the facing test only rejects a
                            // back face (the mask's sectors carry the geometry) - smoothly over the slack band.
                            float khg_sl = 0.08f + min(khg_sd * 0.0005f, 0.24f);
                            // The proxy (fallback) until the sender's normal is known.
                            float khg_ce = dot(khg_S, khg_v) / (max(length(khg_S), 1e-4f) * khg_d);

                            if (khg_on)
                            {
                                float3 khg_sx = KhgVpos(float2(khg_nr), khg_dr2, khg_res, khg_m00, khg_m11) - khg_S;
                                float3 khg_sy = KhgVpos(float2(khg_nd), khg_dd2, khg_res, khg_m00, khg_m11) - khg_S;
                                float3 khg_sn = cross(khg_sy, khg_sx);
                                float khg_snl = length(khg_sn);

                                if (khg_snl > 1e-9f)
                                {
                                    khg_sn /= khg_snl;
                                    if (dot(khg_sn, khg_S) > 0.0f) khg_sn = -khg_sn;   // Face the camera.
                                    khg_ce = dot(khg_sn, -khg_v) / khg_d;   // Emission toward the receiver.
                                }
                            }

                            const float khg_face = saturate((khg_ce + khg_sl) / (2.0f * khg_sl));
                            const float khg_fall = pow(saturate(1.0f - khg_d / khg_rad), khg_fp);
                            // Its radiance from the seed on the gather's grid (effect 26, SSGI_SEED), four taps half
                            // a texel about the sample. Do not read a coarser level (a pyramid's): it blurs a bright
                            // surface across an edge onto the one beside it - a wall's light read at the foot of the
                            // ground below it, or the ground's up the wall - and bleeds it back as bounce; the chain's
                            // a-trous and resolve carry the grain that reading the base level alone leaves. The base
                            // level keeps a smaller share of that bleed: a sample 2 - 3 px from the receiver reads
                            // texels holding the receiver's own block, so up to 23 % of its radiance comes from pixels
                            // at or behind the receiver (on its side of the line through it square to the slice; the
                            // worst slice direction) at setSsgiScale 0.5, 36 % at 0.25. Up to 6 % at 0.5 is the
                            // receiver's own pixel, which an even factor puts at its block's lower right - nearest the
                            // samples up and to the left (3.5 % at most for samples down and to the right). The grid's
                            // resolution: narrower taps do not remove it (a point read keeps 16 % at 0.5).
                            float2 khg_uv2 = (float2(khg_sp) + 0.5f) / khg_res;
                            float2 khg_tx = (khg_inv * 0.5f) / float2(fxMeta.z, fxMeta.w);
                            const float2 khg_ty = float2(khg_tx.x, -khg_tx.y);
                            float3 khg_c = 0.25f * (khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_tx, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 + khg_ty, 0.0f).rgb
                                         + khsgTex.SampleLevel(khsgSamp, khg_uv2 - khg_ty, 0.0f).rgb);

                            // SSGI_NODITHER: do not add a +-1/255 dither to each sample's radiance here - a display
                            // step in the linear HDR scene, with no quantizer to serve (the seed, this grid and the
                            // a-trous are 16F; the resolve dithers its own result, gated off the dark). In the dark its
                            // zero-mean sum comes out of the final max(.., 0) as a positive floor and a static pattern;
                            // in the light the a-trous averages it away.
                            float khg_l = Luma(khg_c);
                            if (fxParams1.w > 0.01f && khg_l > fxParams1.w)
                                khg_c *= fxParams1.w / khg_l;   // Firefly clamp.
                            khg_side += khg_c * (khg_face * khg_fall * (float)countbits(khg_new) * (1.0f / 32.0f));
                        }
                        // A side cut short by the frame edge saw only part of its
                        // stretch: scaled by the share it saw, at most 2x.
                        khg_sacc += khg_side * ((float)khg_m / max((float)khg_seen, 0.5f * (float)khg_m));
                    }
                    // GTAO's slice weight: the normal's projected length times the
                    // lobe's integral; the normalization takes the bias-free lobe (2).
                    khg_acc += khg_sacc * (khg_npl * khg_cr / (1.0f - khg_bias));
                    khg_ws += khg_npl * 2.0f;
                }

                // The budget's factor on the samples the slices take (2 ns m).
                float3 khg_gi = khg_acc * (KH_SSGI_VB_GAIN * pow(12.0f / (float)(2 * khg_ns * khg_m), KH_SSGI_VB_NEXP)
                                           / max(khg_ws, 1e-4f));
                float khg_gl = Luma(khg_gi);
                khg_gi = max(lerp(float3(khg_gl, khg_gl, khg_gl), khg_gi,
                                  max(fxParams1.y, 0.0f)), 0.0f);
                // Receiver-albedo proxy: bounce lands tinted by the surface it lights (scene chroma over a luma floor),
                // read at the full-res pixel this grid texel stands for (i.pos spans the grid).
                // GI_FINITE: held to [0, 65504] as the seed is - unclamped, a NaN pixel here makes the texel NaN
                // (0 x NaN) and the a-trous passes spread it; a negative one divides by a luma near -0.3. A finite
                // non-negative scene passes unchanged.
                float3 khg_scn = min(max(SampleScene(khg_fpx), 0.0f), 65504.0f);
                float3 khg_alb = khg_scn / (Luma(khg_scn) + 0.3f);
                khg_b = khg_gi * lerp(float3(1.0f, 1.0f, 1.0f), khg_alb, saturate(fxParams2.x))
                      * color.rgb * max(fxParams0.x, 0.0f) * khg_rng * khg_conf;
            }
        }

        return float4(khg_b, 1.0f);
    }
}

// Effect 25 - the SSGI a-trous (step local0.x).
float4 PSSsgiAtrous(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        int khat_sp = (int)clamp(localParams0.x >= 0.5f ? localParams0.x : 2.0f, 1.0f, 4.0f);
        float3 khat_c = khsgTex.Load(int3(px, 0)).rgb;
        float khat_cd = KhSsgiGridZ(px);   // FX_LINZ.

        if (khat_cd < 1e8f)
        {
            float3 khat_acc = khat_c;
            float khat_ws = 1.0f;
            // SSGI_CENTRE: the grid's own size, not fxMeta.zw over the width's factor (that comes out a column short
            // for some widths and scales, and a row short wherever the sides round apart).
            const int2 khat_hb = int2(KhSsgiGrid());

            [unroll] for (int khat_j = -2; khat_j <= 2; ++khat_j)
            [unroll] for (int khat_i = -2; khat_i <= 2; ++khat_i)
            {
                if (khat_i == 0 && khat_j == 0) continue;
                int2 khat_p = px + int2(khat_i, khat_j) * khat_sp;
                if (khat_p.x < 0 || khat_p.y < 0 ||
                    khat_p.x >= khat_hb.x || khat_p.y >= khat_hb.y) continue;
                float khat_d = KhSsgiGridZ(khat_p);
                if (khat_d >= 1e8f) continue;
                float khat_dz = abs(khat_d - khat_cd) / (khat_cd * 0.06f + 0.05f);
                float khat_w = exp(-0.125f * (khat_i * khat_i + khat_j * khat_j))
                             * exp(-khat_dz * khat_dz);
                khat_acc += khsgTex.Load(int3(khat_p, 0)).rgb * khat_w;
                khat_ws += khat_w;
            }

            khat_c = khat_acc / khat_ws;
        }

        return float4(khat_c, 1.0f);   // Side-buffer draw: no tail (the resolve owns it).
    }
}

// Effect 32 - dynamicLightFog's gather (DLF). DLF_SPLIT: [earlydepthstencil] - the stencil test before the
// shader runs, so the texels the other build takes cost nothing here though KhFxBegin may discard (no depth or stencil
// is written by either gather draw; with no depth-stencil view bound it changes nothing).
[earlydepthstencil]
float4 PSDlfGather(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // Every selected light's in-scattering along this texel's view ray (the DLF note, effect.hlsl), times
        // intensity (fxParams0.x) and the tint (color.rgb). Side-buffer draw: no tail (the composite, 31, owns it).
        float3 khdg_acc = float3(0.0f, 0.0f, 0.0f);
        [branch] if (dlCtl.x >= 0.5f && fxCam.w >= 0.5f) {
            const int2   khdg_fp = KhDlfFull(px, localParams0.zw);
            const float  khdg_sd = KhLinZ(khdg_fp);   // The visible surface's view depth.
            // WSEE: where the world's blended layers cover the texel's pixel (DLF_WS_FAINT: where the split
            // stands, the marker sends a texel whose layers pass KH_DLF_WS_FAINT_T or more with the depth buffer at the
            // behind to the lean build; drawn here, such a texel is covered), one march runs to the depth behind them
            // (khdg_sb) and partial sums are kept at each layer group's depth (khdg_gm: the cut in metres) and at the
            // front (khdg_sf: the nearest layer, which may lie before the depth buffer's khdg_sd); the texel takes them
            // composed front to back -
            // (1 - T1) sum(z1) + T1 (1 - T2) sum(z2) + ... + prod T sum(behind); one group: Tr sum(behind) + (1 - Tr)
            // sum(the group's own depth, KhWseeOneFront). Per pixel (sample 0). Elsewhere and unarmed
            // khdg_sb = khdg_sf = khdg_sd, khdg_wc false: the plain march and sum - the KH_FX_LEAN build's, which
            // DLF_SPLIT draws on those texels (PSDlfClass's note): past the verdict the uncovered path's result
            // depends on nothing of the record.
            // DLF_TW: the sums telescoped into ONE - a sample at t weighs W(t) = the behind's weight + the weight of
            // every group (or the front) whose cut lies at or past t, so the march keeps one accumulator in place of
            // eight: the same sum reassociated (float rounding alone).
            float khdg_wt = 1.0f;
            float khdg_sb = khdg_sd;
            float khdg_sf = khdg_sd;
            bool  khdg_wc = false;
            bool  khdg_gs = false;   // The group split.
            float3 khdg_gz = float3(0.0f, 0.0f, 0.0f);   // The groups' view depths (khWsGz), their weights and
            float3 khdg_gw = float3(0.0f, 0.0f, 0.0f);   // the behind's weight (the product of the groups' T).
            float  khdg_gp = 1.0f;
            float4 khdg_gz2 = float4(0.0f, 0.0f, 0.0f, 0.0f);   // Groups 3 - 6's view depths and
            float4 khdg_gw2 = float4(0.0f, 0.0f, 0.0f, 0.0f);   // weights (the layer-depth split; 0 without it);
            bool   khdg_gs2 = false;   // The split's groups stand (khWsSp): groups 3 - 6.
            [branch] if (KhWseeArmed()) {
                float3 khdg_wo;
                khdg_wt = KhWseeAt(khdg_fp, LoadDepthPS(khdg_fp), khdg_wo);
                if (khdg_wo.y > khdg_wo.x) {
                    khdg_wc = true;
                    khdg_sb = LinDepth(khdg_wo.y);
                    khdg_sf = LinDepth(KhWseeOneFront(khdg_wo));
                    [branch] if (khWsGn >= 2u) {
                        khdg_gs = true;
                        khdg_gz = float3(LinDepth(khWsGz.x), LinDepth(khWsGz.y), LinDepth(khWsGz.z));
                        khdg_gw = float3(1.0f - khWsGt.x, khWsGt.x * (1.0f - khWsGt.y),
                                         khWsGt.x * khWsGt.y * (1.0f - khWsGt.z));
                        khdg_gp = khWsGt.x * khWsGt.y * khWsGt.z;
                        // The split's farther groups (absent: T 1, weight 0 - the sums above unchanged).
                        [branch] if (khWsSp) {   // Four (5 and 6 absent, T 1, without a 2nd band).
                            khdg_gs2 = true;
                            khdg_gz2 = float4(LinDepth(khWsGz2.x), LinDepth(khWsGz2.y), LinDepth(khWsGz2.z),
                                              LinDepth(khWsGz2.w));
                            khdg_gw2.xy = float2(khdg_gp * (1.0f - khWsGt2.x),
                                                 khdg_gp * khWsGt2.x * (1.0f - khWsGt2.y));
                            khdg_gp = khdg_gp * khWsGt2.x * khWsGt2.y;
                            khdg_gw2.zw = float2(khdg_gp * (1.0f - khWsGt2.z),
                                                 khdg_gp * khWsGt2.z * (1.0f - khWsGt2.w));
                            khdg_gp = khdg_gp * khWsGt2.z * khWsGt2.w;
                        }
                    }
                }
            }
            const float  khdg_dz = min(khdg_sb, KhDlfMaxD());   // The march ends there, the sky past it.
            const float2 khdg_uv = (float2(khdg_fp) + 0.5f) / float2(fxMeta.z, fxMeta.w);
            const float3 khdg_d1 = KhDlfRel(khdg_uv, 1.0f);   // P(s) = khdg_d1 * s at view depth s.
            const float  khdg_rl = max(length(khdg_d1), 1.0e-6f);   // Metres of ray per unit of s.
            const float3 khdg_rd = khdg_d1 / khdg_rl;
            // The march's end in s: the surface, or maxDistance metres from the camera (a straight line, as
            // dlf_select takes the lights: a light whose reach lies past it lights nothing marched).
            const float  khdg_end = min(khdg_dz, KhDlfMaxD() / khdg_rl);
            // WSEE: the front surface's end of the march in metres along the ray (khdg_end's rule at khdg_sf).
            const float  khdg_fm = min(min(khdg_sf, KhDlfMaxD()), KhDlfMaxD() / khdg_rl) * khdg_rl;
            // The groups' cuts (khdg_fm's rule at each group's depth).
            const float  khdg_gx = KhDlfMaxD();
            const float3 khdg_gm = min(min(khdg_gz, float3(khdg_gx, khdg_gx, khdg_gx)),
                                       float3(khdg_gx, khdg_gx, khdg_gx) / khdg_rl) * khdg_rl;
            const float4 khdg_gm2 = min(min(khdg_gz2, khdg_gx.xxxx),   // Groups 3 - 6.
                                        khdg_gx.xxxx / khdg_rl) * khdg_rl;
            // DLF_TW: W(t)'s terms - the behind's weight khdg_tb and up to seven cuts (khdg_tc0 / tc1, metres)
            // with their weights (khdg_tw0 / tw1): the groups' (gp; gw at gm, gw2 at gm2), or the one-group mix's
            // (Tr; 1 - Tr at the front, khdg_fm: its second sum took the samples not past it), or W = 1 (uncovered:
            // c x 1, the plain sum bit for bit). An unused cut is -1 with weight 0 (no sample lies before 0).
            float  khdg_tb = 1.0f;
            float4 khdg_tc0 = float4(-1.0f, -1.0f, -1.0f, -1.0f), khdg_tw0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
            float4 khdg_tc1 = float4(-1.0f, -1.0f, -1.0f, -1.0f), khdg_tw1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
            if (khdg_gs) {
                khdg_tb = khdg_gp;
                khdg_tc0 = float4(khdg_gm, -1.0f);
                khdg_tw0 = float4(khdg_gw, 0.0f);
                if (khdg_gs2) {
                    khdg_tc1 = khdg_gm2;
                    khdg_tw1 = khdg_gw2;
                }
            } else if (khdg_wc) {
                khdg_tb = khdg_wt;
                khdg_tc0.x = khdg_fm;
                khdg_tw0.x = 1.0f - khdg_wt;
            }
            // The screen basis the occlusion taps project with (KhDlfSsVis).
            const float3 khdg_f = KhDlfRel(float2(0.5f, 0.5f), 1.0f);
            const float3 khdg_r = KhDlfRel(float2(1.0f, 0.5f), 1.0f) - khdg_f;
            const float3 khdg_u = KhDlfRel(float2(0.5f, 0.0f), 1.0f) - khdg_f;
            const float  khdg_cy = fxCam.y;   // The camera's absolute height: the fog's reference.
            const int    khdg_ns = clamp((int)fxParams1.x, 4, 64);
            const bool   khdg_fr = khFxFog[0].w > 0.001f || khFxFog[1].w > 0.001f;   // DLF_FOGREC: a fog pass.
            // DLF_FOGREC: T at the surface the texel shows - the behind where layers cover it (effect 13's factor
            // for the background; under the layers it blends T at the behind and at the front by their transmittance,
            // so a glow behind glass, a fence or smoke against the sky stays too bright there, by up to 1 / T at the
            // sky) - per record drawn after this pass; 1 for one drawn before or none (KhDlfPassMedium's khdm_tz).
            // The filter and the upsample key the sky apart from the surfaces (KhDlfKeyZ), so a texel's glow blends
            // with those whose factors are close.
            float2 khdg_tz = float2(1.0f, 1.0f);
            [branch] if (khdg_fr) {
                float khdg_dq;
                [unroll] for (int khdg_q = 0; khdg_q < 2; ++khdg_q) {
                    const float4 khdg_e = khdg_q == 0 ? khFxFog[0] : khFxFog[1];
                    const bool   khdg_af = khdg_e.w > 0.001f && (khdg_q == 0 ? fxParams2.x : fxParams2.y) < 0.5f;
                    const float  khdg_tq = saturate(1.0f - saturate(khdg_e.w) * KhDlfPassF(khdg_sb, khdg_e, khdg_dq));
                    khdg_tz[khdg_q] = khdg_af ? khdg_tq : 1.0f;
                }
            }
            const bool   khdg_ss = fxParams1.z >= 1.5f;
            // Position-only jitter (a static frame renders bit-identical): the SSGI gather's gradient noise.
            const float  khdg_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x + 0.00583715f * i.pos.y));
            const float  khdg_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                                           + 0.00583715f * (i.pos.y + 5.588238f)));
            const int    khdg_pn = (int)dlCtl.y;
            const int    khdg_n = khdg_pn + (int)dlCtl.z;
            [loop] for (int khdg_l = 0; khdg_l < khdg_n; ++khdg_l) {
                const int    khdg_b = khdg_l * 6;
                // DLF_REC_LAZY: the light's position ([0]) and reach ([5]) first; [1] - [4] only for a ray
                // that meets its reach (most lights miss most texels).
                const float4 khdg_r0 = KhDlRec(khdg_b + 0);
                const float4 khdg_r5 = KhDlRec(khdg_b + 5);
                // The ray against the sphere of the light's reach ([5].w), in s.
                const float  khdg_qa = dot(khdg_d1, khdg_d1);
                const float  khdg_qb = -2.0f * dot(khdg_d1, khdg_r0.xyz);
                const float  khdg_qc = dot(khdg_r0.xyz, khdg_r0.xyz) - khdg_r5.w * khdg_r5.w;
                const float  khdg_disc = khdg_qb * khdg_qb - 4.0f * khdg_qa * khdg_qc;
                if (!(khdg_disc > 0.0f)) continue;
                const float  khdg_sq = sqrt(khdg_disc);
                float khdg_s0 = max((-khdg_qb - khdg_sq) / (2.0f * khdg_qa), 0.0f);
                float khdg_s1 = min((-khdg_qb + khdg_sq) / (2.0f * khdg_qa), khdg_end);
                if (!(khdg_s1 > khdg_s0)) continue;
                const float4 khdg_r1 = KhDlRec(khdg_b + 1);
                const float4 khdg_r2 = KhDlRec(khdg_b + 2);
                const float4 khdg_r3 = KhDlRec(khdg_b + 3);
                const float4 khdg_r4 = KhDlRec(khdg_b + 4);
                const bool khdg_spot = khdg_l >= khdg_pn;
                // A spot marches its cone alone (a cut below 0.05 - wider than ~87 degrees - is not one cone). A
                // branch, not &&: fxc does not short-circuit, and the clip writes s0 / s1 - a point's too, were it run.
                bool khdg_off = false;
                [branch] if (khdg_spot && khdg_r1.w > 0.05f && khdg_r2.w > 0.0f) {
                    khdg_off = !KhDlfCone(khdg_d1, khdg_r0.xyz, khdg_r1.xyz, khdg_r1.w, khdg_s0, khdg_s1);
                }
                if (khdg_off) continue;
                // The samples, in metres along the ray (KhDlfMisStep): half equiangular about the ray's point nearest
                // the light, so the glow's core, where the light is brightest, gets them; half evenly spaced. The
                // equiangular width is the miss distance, or the light's core (KhDlfCore) when wider - no wider than
                // the marched span, where the even half serves.
                const float khdg_tc = dot(khdg_r0.xyz, khdg_rd);
                const float khdg_mb = length(khdg_r0.xyz - khdg_rd * khdg_tc);
                const float khdg_w = max(max(khdg_mb, min(KhDlfCore(khdg_r4), (khdg_s1 - khdg_s0) * khdg_rl)), 0.05f);
                const float khdg_t0 = khdg_s0 * khdg_rl;
                const float khdg_t1 = khdg_s1 * khdg_rl;
                const float4 khdg_eq = KhDlfEquiInit(khdg_t0, khdg_t1, khdg_tc, khdg_w);
                const int   khdg_slot = (int)khdg_r5.z - 1;   // Our map's slot, or -1 (none, or shadows 0).
                // DLF_SS: whether the light itself is hidden behind a nearer surface - the gate of
                // KhDlfSsVis's plane continuation - once per light per texel (one probe), not per sample; nothing
                // without the geometry test (shadows < 2).
                bool khdg_cov = false;
                [branch] if (khdg_ss) khdg_cov = KhDlfSsCovered(khdg_r0.xyz, khdg_f, khdg_r, khdg_u);
                float khdg_sum = 0.0f;   // DLF_TW: each sample at its W(t).
                [loop] for (int khdg_k = 0; khdg_k < khdg_ns; ++khdg_k) {
                    const float2 khdg_tw = KhDlfMisStep(khdg_eq, khdg_t0, khdg_t1, khdg_k, khdg_ns, khdg_ig);
                    const float3 khdg_p = khdg_rd * khdg_tw.x;
                    float3 khdg_lv = khdg_r0.xyz - khdg_p;
                    const float khdg_dist = length(khdg_lv);
                    khdg_lv /= khdg_dist + 1e-4f;
                    const float khdg_at = KhDlfAtt(khdg_r1, khdg_r2, khdg_r3, khdg_r4, khdg_r5, khdg_dist, khdg_lv,
                                                   khdg_spot);
                    if (khdg_at <= 0.0f) continue;
                    const float khdg_py = khdg_p.y + khdg_cy;
                    const float khdg_sg = KhDlfSigma(khdg_py);
                    // DLF_FOGREC (effect.hlsl KhDlfPassMedium): the chain's fog passes as a medium too - (1, 0, 0)
                    // without one, the expressions below then the game's fog's alone, bit for bit.
                    float3 khdg_pm = float3(1.0f, 0.0f, 0.0f);
                    [branch] if (khdg_fr) khdg_pm = KhDlfPassMedium(khdg_tw.x / khdg_rl, khdg_rl, khdg_tz);
                    // Nothing to scatter (also behind a pass drawn before that is opaque there - its x 0). TWIN: C++
                    // dlf_medium_zero (DLF_MEDIUM0) draws no gather or filter where this holds at every sample
                    // (the composite runs unarmed).
                    if (!(khdg_sg * khdg_pm.x > 0.0f) && !(khdg_pm.z > 0.0f)) continue;
                    float khdg_v = 1.0f;
                    [branch] if (khdg_slot >= 0) khdg_v = KhDlfDlsVis(khdg_slot, khdg_p + fxCam.xyz);
                    // WSEE: a sample past the front surface (covered texels only) lies behind the layers, which
                    // its own pixel shows at the depth buffer's depth; against that depth KhDlfSsVis would read the
                    // layers as a continuous surface it went behind and shadow it, so its own surface is the behind
                    // one (where a late opaque surface is behind, G = 1, that is khdg_sd itself).
                    const bool khdg_bk = khdg_wc && khdg_tw.x > khdg_fm;
                    [branch] if (khdg_ss && khdg_v > 0.0f) {
                        khdg_v *= KhDlfSsVis(khdg_p, khdg_r0.xyz, khdg_f, khdg_r, khdg_u,
                                             frac(khdg_ig2 + (float)khdg_k * 0.61803399f),
                                             khdg_bk ? khdg_sb : khdg_sd, khdg_cov);   // DLF_SS.
                    }
                    if (khdg_v <= 0.0f) continue;
                    // Extinction on both legs: the camera's (the height fog's closed form, and the engine's fog-end
                    // ramp as its fog applies it) and the light's (the local density over the distance to it).
                    // FOG_EXP_CAP: the density capped (finite), so an optical depth that overflows (a huge fogScale
                    // under a negative decay) gives 1e30 x 0 = 0 and a sample exactly at the light a finite optical
                    // depth, not inf x 0 = NaN - a NaN here would take the light's whole glow out of the texel and,
                    // through the filter, its neighbours'.
                    const float khdg_sgc = min(khdg_sg, 1.0e30f);
                    // DLF_FOGREC: the passes' extinction on the light leg, their transmittance on the camera's
                    // (x, for the game's fog's scattering) and their own scattering (z).
                    const float khdg_t = KhDlfTau(khdg_tw.x, khdg_cy, khdg_py)
                                       + min(khdg_sgc + khdg_pm.y, 1.0e30f) * khdg_dist;
                    const float khdg_c = khdg_tw.y * khdg_at * (khdg_sgc * khdg_pm.x + khdg_pm.z)
                                       * KhDlfPhase(dot(khdg_lv, khdg_rd)) * khdg_v
                                       * exp(-khdg_t) * KhDlfEndRamp(khdg_tw.x);
                    float khdg_ww = khdg_tb;   // DLF_TW: W(t).
                    [branch] if (khdg_wc) {
                        khdg_ww += dot(khdg_tw0, float4(khdg_tw.x <= khdg_tc0.x ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.y ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.z ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc0.w ? 1.0f : 0.0f))
                                 + dot(khdg_tw1, float4(khdg_tw.x <= khdg_tc1.x ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.y ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.z ? 1.0f : 0.0f,
                                                        khdg_tw.x <= khdg_tc1.w ? 1.0f : 0.0f));
                    }
                    khdg_sum += khdg_c * khdg_ww;
                }
                // DynLights' combine without the N.L: the diffuse under the global tint, plus the per-light
                // ambient, times the global intensity. The shadow (khdg_v: our light maps' and the screen-space
                // test's) takes both lanes whole in the sum, where DynLights keeps KH_DLS_AMB_KEEP of the ambient
                // under its map's shadow: a shaft's shadowed side is dark in the fog where a wall in that shadow
                // keeps 35 % of the lamp's ambient.
                const float3 khdg_col = (dlGlobal.xyz * khdg_r2.xyz + khdg_r3.xyz) * dlGlobal.w;
                khdg_acc += khdg_col * khdg_sum;   // DLF_TW: the groups already composed in the weights.
            }
            khdg_acc *= max(fxParams0.x, 0.0f) * color.rgb;
        }
        return float4(khdg_acc, 1.0f);
    }
}

// DLF_SPLIT - which gather a grid texel takes. The same entry built with KH_FX_LEAN 1 (ps_dlf_gather_lean) has no
// record code: where no layer covers the texel's pixel the full gather's march and sum are the lean one's (the weight
// W(t) is 1, the march ends at the depth buffer's depth), so C++ draws the gather twice under a stencil this entry
// marks - the lean build where it stays 0, the full build where it reads 1 - and a texel no layer covers runs without
// the record's registers. PSDlfClass marks 1 wherever KhWseeRule COULD find the pixel covered (KhDlfWsMaybe), faint
// layers alone excepted (DLF_WS_FAINT, below), at any pixel KhDlfFull may give the texel (the coordinate is
// truncated: within KH_DLF_SPLIT_TOL of a pixel edge both sides are tested, so another compile's rounding of the same
// expression cannot pick a pixel left untested - at an even grid factor, setSsgiScale 0.5 on an even frame, the
// coordinate lies ON an edge, so most texels test four pixels: a few loads a texel, against the gather's hundreds), so
// a covered texel takes the full build unless faint layers alone cover it (then the lean one); an uncovered one marked
// 1 takes it too, with the same result. No colour output: the stencil alone (C++ binds no target). KhFxBegin as the
// gathers run it: where it discards, neither gather writes the texel, and this entry marks nothing.
static const float KH_DLF_SPLIT_TOL = 0.01f;   // Pixels: ~40 float ulps at 3840, any compile's rounding inside it.
// DLF_WS_FAINT - a covered pixel whose layers pass KH_DLF_WS_FAINT_T or more of the light behind them, with no
// depth-writing layer among them (the behind raw is the depth buffer's own, o.y == r: the lean march ends where the
// full one does), takes the lean gather: the full build's composition there is T sum(behind) + (1 - T) sum(front),
// the front sum a truncation of the behind sum along the same ray, so the two builds differ by at most (1 - T) of
// their difference - under 2 % of a term that is itself small - for the full gather's whole cost (the record's loads
// and registers, W(t) per sample; the march ends at the same depth). Thin rain, drizzle, a faint haze sprite over the
// frame pay nothing; smoke, foliage and glass (T small, or a depth written) keep the full build. The verdict is
// KhWseeAt's at the pixel, the pair the full gather itself takes (sample 0, PSDlfGather's khdg_wt / khdg_wo), so the
// texel is gated on exactly the inputs it would have used. KhDlfWsMaybe's superset stands for every other pixel (its
// rounding note).
static const float KH_DLF_WS_FAINT_T = KH_WSEE_FAINT_T;   // One constant for every reader (cb.hlsl WSEE_FAINT).
// KhWseeRule's head at p (sample 0, khwr_own, khwr_r0 = r, as KhWseeAt calls it): the range test and the entry test
// (n < r or s > r) on the same loads and the same r (LoadDepthPS) - a superset of its covered set, since KhWseeRule
// covers only inside both. Those compare loaded values alone, except LoadDepthPS's near-z mark, whose r is computed
// (re-encoded) and could round apart in another build: such a pixel counts as maybe covered. Callers hold the arm.
// TWIN: KhWseeRule's NW loads and its two outer tests (cb.hlsl), LoadDepthPS's mark test.
bool KhDlfWsMaybe(int2 p)
{
    bool khwm_c = false;
    const float khwm_r = LoadDepthPS(p);
    bool khwm_z = false;   // LoadDepthPS took the near-z mark's computed value.
    [branch] if (shadowMeta2.x > 0.5f) {
        const float  khwm_raw = LoadDepthRaw(p);
        const float2 khwm_m = khNzMark.Load(int3(p, 0));
        khwm_z = khwm_m.x > 0.0f && khwm_m.x == khwm_raw && khwm_m.y > 0.0f;
    }
    [branch] if (khwm_z) {
        khwm_c = true;
    } else {
        float2 khwm_nn = float2(1.0f, 1.0f);
        [branch] if ((KhWseeAvail() & 128u) == 0u) {
            khwm_nn = float2(khWsN.Load(int3(p, 0)), khWsN2.Load(int3(p, 0)));
        }
        [branch] if ((KhWseeAvail() & 64u) != 0u) {
            const int2 khwm_bp = KhWseePsbAt(p);
            khwm_nn = min(khwm_nn, float2(khWsNb.Load(int3(khwm_bp, 0)), khWsNb2.Load(int3(khwm_bp, 0))));
        }
        const float khwm_n = min(khwm_nn.x, khwm_nn.y);
        [branch] if (min(khwm_r, khwm_n) >= depthParams.z) {
            const float khwm_s = khWsS.Load(int3(p, 0));
            khwm_c = khwm_n < khwm_r || khwm_s > khwm_r;
        }
    }
    // DLF_WS_FAINT: a pixel the rule finds covered by faint layers alone, the depth buffer already at the behind.
    [branch] if (khwm_c) {
        float3 khwm_o;
        const float khwm_t = KhWseeAt(p, khwm_r, khwm_o);
        if (khwm_o.y > khwm_o.x && khwm_t >= KH_DLF_WS_FAINT_T && khwm_o.y == khwm_r) khwm_c = false;
    }
    return khwm_c;
}
void PSDlfClass(VSOut i)
{
    KhFxBegin(i);   // The gathers' prologue.
    const int2 px = int2(i.pos.xy);
    bool khdl_c = false;
    [branch] if (KhWseeArmed()) {
        // KhDlfFull's coordinate before its truncation, and the pixels a rounding of it within the tolerance gives.
        const float2 khdl_f = (float2(px) + 0.5f) * float2(fxMeta.z, fxMeta.w) / max(localParams0.zw, 1.0f);
        const int2   khdl_hi = int2((int)fxMeta.z - 1, (int)fxMeta.w - 1);
        const int2   khdl_a = clamp(int2(khdl_f - KH_DLF_SPLIT_TOL), int2(0, 0), khdl_hi);
        const int2   khdl_b = clamp(int2(khdl_f + KH_DLF_SPLIT_TOL), int2(0, 0), khdl_hi);
        khdl_c = KhDlfWsMaybe(khdl_a);
        [branch] if (!khdl_c && khdl_b.x != khdl_a.x) khdl_c = KhDlfWsMaybe(int2(khdl_b.x, khdl_a.y));
        [branch] if (!khdl_c && khdl_b.y != khdl_a.y) khdl_c = KhDlfWsMaybe(int2(khdl_a.x, khdl_b.y));
        [branch] if (!khdl_c && khdl_b.x != khdl_a.x && khdl_b.y != khdl_a.y) khdl_c = KhDlfWsMaybe(khdl_b);
    }
    if (!khdl_c) discard;
}

// Effect 33 - dynamicLightFog's grid filter (DLF).
float4 PSDlfFilter(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    int2 px = int2(i.pos.xy);
    {
        // The SSGI a-trous (effect 25) on the fog: 5 x 5 at the step, Gaussian in the offset and in relative
        // agreement of the depth key (KhDlfKeyG) - the sky and everything past maxDistance, which marched to the same
        // end, blend with one another and with nothing nearer (DLF_FOGREC: with a fog pass drawn after
        // this one the sky keys apart from them - KhDlfKeyZ). Side-buffer draw: no tail.
        const float2 khdt_gd = localParams0.zw;
        const int    khdt_st = clamp((int)(localParams0.x + 0.5f), 1, 8);
        const float  khdt_cd = KhDlfKeyG(px, khdt_gd);
        float3 khdt_acc = khsgTex.Load(int3(px, 0)).rgb;
        float  khdt_ws = 1.0f;
        [unroll] for (int khdt_j = -2; khdt_j <= 2; ++khdt_j)
        [unroll] for (int khdt_i = -2; khdt_i <= 2; ++khdt_i)
        {
            if (khdt_i == 0 && khdt_j == 0) continue;
            const int2 khdt_p = px + int2(khdt_i, khdt_j) * khdt_st;
            if (khdt_p.x < 0 || khdt_p.y < 0 || khdt_p.x >= (int)khdt_gd.x || khdt_p.y >= (int)khdt_gd.y) continue;
            const float khdt_z = abs(KhDlfKeyG(khdt_p, khdt_gd) - khdt_cd) / (khdt_cd * 0.06f + 0.05f);
            const float khdt_w = exp(-0.125f * (float)(khdt_i * khdt_i + khdt_j * khdt_j)) * exp(-khdt_z * khdt_z);
            khdt_acc += khsgTex.Load(int3(khdt_p, 0)).rgb * khdt_w;
            khdt_ws += khdt_w;
        }
        return float4(khdt_acc / khdt_ws, 1.0f);
    }
}

// FX_LINZ: the targets KhLinZ / KhSsgiGridZ / KhDlfGridZ read, one texel per pixel of the draw - fxMeta.x 34: the
// frame (the conversion itself), 35: the SSGI grid (texel t = KhLinZ at KhSsgiFull(t), the a-trous' mapping), 36:
// dynamicLightFog's (KhLinZ at KhDlfFull(t, local0.zw)). C++ fx_linz_draw draws them with the
// reading passes' own lanes; 35 / 36 read the frame's through KhLinZ.
float PSFxLinZ(VSOut i) : SV_Target
{
    const int2 khlb_p = int2(i.pos.xy);
    const int  khlb_m = (int)(fxMeta.x + 0.5f);
    float khlb_z;
    [branch] if (khlb_m == 35) khlb_z = KhLinZ(KhSsgiFull(khlb_p));   // SSGI_CENTRE.
    else if (khlb_m == 36) khlb_z = KhLinZ(KhDlfFull(khlb_p, localParams0.zw));
    else khlb_z = LinDepth(LoadDepthPS(khlb_p));
    return khlb_z;
}

// FS_SPLIT - fog scatter (effect 23) for the scene chain: PSEffect's branch with its side draw (28) mandatory -
// every pixel's and every tap's fog read from khFxSide (t4), which the chain draws before the pass and arms (matCtl.y)
// - so the in-place KhFsFogW arms (KhWseeMix inlined in the tap loop) are not compiled in and the entry's register
// count stays low. C++ (fx_chain_run) draws it where fx_side_draw armed side 28 and the entry stands; PSEffect's
// branch everywhere else (a failed side draw, an effect mesh, the PIP). The branch's code line for line - its two side
// tests resolved as the armed route resolves them, the four lanes only KhFsFogW read dropped -, PSEffect's prologue
// without the UI lane's probe and KhFxFinish as its tail, so the output is the branch's bit for bit. precise on the
// tap geometry and the two depths (khfs_cd, khfs_an, khfs_sr, khfs_pc, khfs_sd) keeps every operation feeding them
// unfused, as PSEffect's build leaves them through its KhFsFogW arms (PSEffect itself marks none; a GPU fusing mad
// would part from PSEffect by an ulp); khfs_rm at MSAA_DEPTH 0 is written in that build's product form. TWIN:
// PSEffect's branch - an edit there is made here, and both blobs compared again (an edit can move fxc's choices); its
// notes (FX_SIDE, FS_SELF, FS_SKY, GLOW_PYR, FS_MIX, FS_ODD, FS_FLOOR) stand for both.
float4 PSFsSplitS(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue.
    int2  px = int2(i.pos.xy);
    float3 scene = SampleScene(px);
    float3 outc = scene;
    {
        float khfs_in = max(fxParams0.x, 0.0f);
#if MSAA_DEPTH
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * KhFxPx()   // FX_PX_REF.
                                           : clamp(fxMeta.w / 90.0f, 4.0f, 64.0f);
#else   // FS_SPLIT: PSEffect's MSAA_DEPTH 0 build folds both divisions into products (its fxc output).
        float khfs_rm = fxParams0.y > 0.5f ? clamp(fxParams0.y, 2.0f, 96.0f) * (max(fxMeta.w, 1.0f) * (1.0f / 1080.0f))
                                           : clamp(fxMeta.w * (1.0f / 90.0f), 4.0f, 64.0f);
#endif
        int khfs_n = clamp((int)fxParams0.z, 4, 24);
        precise float khfs_cd = KhLinZ(px);   // FX_LINZ.
        float khfs_f0 = khFxSide.Load(int3(px, 0));   // FS_SPLIT: armed (the C++ selection).
        float khfs_sc = saturate(khfs_f0 * khfs_in);
        const float khfs_w0 = KhFsOwnW(khfs_sc, khfs_rm);
        float khfs_ws = khfs_w0;
        float3 khfs_acc = scene * khfs_ws;
        float khfs_fx = khfs_sc;
        float khfs_ig = frac(52.9829189f * frac(0.06711056f * i.pos.x
                                              + 0.00583715f * i.pos.y));
        float khfs_ig2 = frac(52.9829189f * frac(0.06711056f * (i.pos.x + 5.588238f)
                                               + 0.00583715f * (i.pos.y + 5.588238f)));
        float khfs_rot = khfs_ig * 6.2831853f;
        float khfs_hn = (float)((khfs_n + 1) >> 1);
        const float khfs_gs = khfs_rm * sqrt(3.14159265f / (float)khfs_n);
        const float khfs_gm = KhGlowMix(khfs_gs);
        const float khfs_ri = min(max(khfs_rm * khfs_sc, 2.0f), khfs_rm);
        const float khfs_qo = 0.75f / (khfs_rm * khfs_rm);   // The density outside ri, x pi.
        const float khfs_qi = 0.25f / (khfs_ri * khfs_ri) + khfs_qo;   // ...and inside it.
        const float khfs_fi = khfs_qi * khfs_ri * khfs_ri;   // The share of the distribution inside ri.
        const float khfs_nt = 2.0f * khfs_hn;
        const float khfs_cwi = 3.0f / (khfs_nt * khfs_qi);   // The cone's 3 / pi x the tap's share of the area.
        const float khfs_cwo = 3.0f / (khfs_nt * khfs_qo);

        [loop] for (int khfs_k = 0; khfs_k < khfs_n; ++khfs_k)
        {
            int khfs_kp = khfs_k >> 1;
            precise float khfs_an = khfs_kp * 2.3999632f + khfs_rot + (khfs_k & 1) * 3.14159265f;
            const float khfs_u = (khfs_kp + khfs_ig2) / khfs_hn;
            const bool  khfs_inr = khfs_u < khfs_fi;
            precise float khfs_sr = max(sqrt(khfs_inr ? khfs_u / khfs_qi
                                                      : khfs_ri * khfs_ri + (khfs_u - khfs_fi) / khfs_qo), 1.0f);
            precise float2 khfs_pc = float2(px) + 0.5f + float2(cos(khfs_an), sin(khfs_an)) * khfs_sr;
            int2 khfs_sp = int2(khfs_pc);
            if (khfs_sp.x < 0 || khfs_sp.y < 0 ||
                khfs_sp.x >= (int)fxMeta.z || khfs_sp.y >= (int)fxMeta.w) continue;
            precise float khfs_sd = KhLinZ(khfs_sp);
            float khfs_fs = khFxSide.Load(int3(khfs_sp, 0));   // FS_SPLIT (FX_SIDE).
            float khfs_ss = saturate(khfs_fs * khfs_in);
            khfs_ss = KhFsSourceFog(khfs_ss, khfs_sc, khfs_sd, khfs_cd);
            float khfs_rk = khfs_rm * khfs_ss;
            if (khfs_sr >= khfs_rk) continue;   // This source's disc does not reach.
            khfs_fx = KhFsNearFloor(khfs_fx, khfs_ss, khfs_sr, khfs_rk, khfs_sd, khfs_cd);
            float khfs_w = khfs_ss * (khfs_inr ? khfs_cwi : khfs_cwo) * (1.0f - khfs_sr / khfs_rk)
                         / max(khfs_rk * khfs_rk, 1.0f);   // FS_MIX: the tap's own share.
            if ((khfs_k | 1) >= khfs_n) khfs_w *= 2.0f;   // FS_ODD: an odd count's lone last tap.
            float3 khfs_c;
            if (khfs_gm >= 1.0f)      khfs_c = KhGlowTap(khfs_pc, khfs_gs);
            else if (khfs_gm <= 0.0f) khfs_c = SampleScene(khfs_sp);
            else                      khfs_c = lerp(SampleScene(khfs_sp), KhGlowTap(khfs_pc, khfs_gs), khfs_gm);
            khfs_acc += khfs_c * khfs_w;
            khfs_ws += khfs_w;
        }

        [branch] if (khfs_fx > khfs_sc) {
            const float khfs_dw = min(KhFsOwnW(khfs_fx, khfs_rm) - khfs_w0, 0.0f);
            khfs_acc += scene * khfs_dw;
            khfs_ws += khfs_dw;
        }
        outc = khfs_acc / max(khfs_ws, 0.5f * KhFsOwnW(1.0f, khfs_rm));
    }
    return KhFxFinish(scene, outc, i.pos.xy);
}

// KH_FX_LEAN - dynamicLightFog's composite (effect 31) for the scene chain: PSEffect's branch 31 line for line, with
// PSEffect's prologue (KhFxBegin, the scene sample; the UI lane's probe left out - a chain pass has centerSize.w 1) and
// KhFxFinish as its tail. C++ (fx_chain_run) draws it, compiled with KH_FX_LEAN (cb.hlsl's note: no record
// maths, ~17 temps where PSEffect holds ~83), where the pass is neither localized nor banded and the build stands;
// PSEffect's branch everywhere else. The same operations on the same inputs (a DXBC emulation of both blobs, mad
// fused and unfused). TWIN: PSEffect's branch 31 - an edit there is made here, and the emulation run again.
float4 PSDlfComp(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue.
    int2  px = int2(i.pos.xy);
    float3 scene = SampleScene(px);
    float3 outc = scene;
    {
        [branch] if (dlCtl.x >= 0.5f) {
            uint khdc_w, khdc_h;
            khsgTex.GetDimensions(khdc_w, khdc_h);
            const float2 khdc_gd = float2((float)max(khdc_w, 1u), (float)max(khdc_h, 1u));
            const int2   khdc_hi = int2(khdc_gd) - 1;
            const float  khdc_cd = KhDlfKey(px);
            const float2 khdc_g = (float2(px) + 0.5f) * khdc_gd / float2(fxMeta.z, fxMeta.w) - 0.5f;
            const int2   khdc_t0 = int2(floor(khdc_g));
            const float2 khdc_f = khdc_g - floor(khdc_g);
            float3 khdc_acc = float3(0.0f, 0.0f, 0.0f);
            float  khdc_ws = 0.0f;
            [unroll] for (int khdc_j = 0; khdc_j <= 1; ++khdc_j)
            [unroll] for (int khdc_i = 0; khdc_i <= 1; ++khdc_i)
            {
                const int2  khdc_t = clamp(khdc_t0 + int2(khdc_i, khdc_j), int2(0, 0), khdc_hi);
                const float khdc_bw = (khdc_i ? khdc_f.x : 1.0f - khdc_f.x) * (khdc_j ? khdc_f.y : 1.0f - khdc_f.y);
                const float khdc_z = abs(KhDlfKeyG(khdc_t, khdc_gd) - khdc_cd) / (khdc_cd * 0.06f + 0.05f);
                const float khdc_wt = khdc_bw * exp(-khdc_z * khdc_z);
                khdc_acc += khsgTex.Load(int3(khdc_t, 0)).rgb * khdc_wt;
                khdc_ws += khdc_wt;
            }
            if (!(khdc_ws > 1.0e-4f)) {
                const int2 khdc_c = int2(floor(khdc_g + 0.5f));
                float khdc_best = 3.0e38f;
                float khdc_best2 = 3.0e38f;   // DLF_FOGREC: the unclamped distance, the tie-break.
                khdc_acc = float3(0.0f, 0.0f, 0.0f);
                khdc_ws = 1.0f;
                [unroll] for (int khdc_n = -1; khdc_n <= 1; ++khdc_n)
                [unroll] for (int khdc_m = -1; khdc_m <= 1; ++khdc_m)
                {
                    const int2  khdc_t = clamp(khdc_c + int2(khdc_m, khdc_n), int2(0, 0), khdc_hi);
                    // DLF_FOGREC: the nearest key - PSEffect's branch 31 rule (its note there), line for line.
                    const float khdc_k = KhDlfKeyG(khdc_t, khdc_gd);
                    const float khdc_e = abs(min(khdc_k, KhDlfMaxD()) - min(khdc_cd, KhDlfMaxD()));
                    const float khdc_e2 = abs(khdc_k - khdc_cd);
                    [flatten] if (khdc_e < khdc_best || (khdc_e == khdc_best && khdc_e2 < khdc_best2)) {
                        khdc_best = khdc_e;
                        khdc_best2 = khdc_e2;
                        khdc_acc = khsgTex.Load(int3(khdc_t, 0)).rgb;
                    }
                }
            }
            outc = scene + max(khdc_acc / khdc_ws, 0.0f);
        }
    }
    return KhFxFinish(scene, outc, i.pos.xy);
}

// WSEE_EDGE_COST - fog scatter's side value (side id 28) as its own entry: PSEffect's branch 28 line for line
// (KhFxBegin, then KhFsFogW at the pixel; the prologue's scene load and the UI lane's probe are dead on a side draw).
// Its own entry because PSEffect's register count starves the per-sample loop (KhWseeMix) of warps to hide its loads.
// fx_side_draw draws it for k 0 where made, else PSEffect. TWIN: PSEffect's branch 28 - an edit there is made here.
float4 PSFsSide(VSOut i) : SV_Target
{
    KhFxBegin(i);   // PSEffect's prologue (inert on a fullscreen side draw).
    const int2 px = int2(i.pos.xy);
    const float2 khfa_res = float2(fxMeta.z, fxMeta.w);
    const float2 khfa_ms = KhFxProjScale();
    const float khfa_m00 = khfa_ms.x;
    const float khfa_m11 = khfa_ms.y;
    return float4(KhFsFogW(px, KhLinZ(px), khfa_res, khfa_m00, khfa_m11), 0.0f, 0.0f, 1.0f);   // WSEE.
}

// WSEE: the chain run's class word (khWsE, t58; cb.hlsl KhWseeClassWord) in an R32_UINT target C++ clears to 0; 0
// everywhere without MSAA. Drawn once a run at its first sited pass, before that pass's side draws, with its lanes,
// so every armed pass's fast path (KhWseeCount) reads one texel in place of 2n. A pixel whose word is 0 is discarded,
// so neither the word nor the stencil C++ binds beside it (marked 1 where drawn) is written there.
uint PSWseeAgree(VSOut i) : SV_Target
{
    uint khwg_w = 0u;
#if MSAA_DEPTH
    khwg_w = KhWseeClassWord(int2(i.pos.xy));
#endif
    if (khwg_w == 0u) discard;
    return khwg_w;
}
