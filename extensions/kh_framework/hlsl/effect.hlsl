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

// KH_FX_LINZ - the frame's linear depth (LinDepth(LoadDepthPS(p)), metres), drawn once per scene-chain run by
// PSFxLinZ (the unit's tail) into targets of ours and read here in place of the conversion - a multisampled load, the
// near-plane marker's load and a division per read - by SSGI, dynamicLightFog and fog scatter. t7: the frame (one
// texel per pixel of fxMeta.zw), armed by matCtl.x. t8 / t9: a grid's copy, keyed the way its passes read it - t8
// the SSGI grid's (texel t holds pixel KhSsgiFull(t), the a-trous' mapping), t9
// dynamicLightFog's (KhDlfFull's) - armed by matCtl.w (1 / 2) when the copy was drawn with the reading pass's own
// mapping. matCtl.x and .w are otherwise unread in this unit (a textured mesh's lanes); cb.hlsl's receive textures at
// t7 - t9 are fenced out of it, as at t4 - t6. Every reader takes the conversion in place when unarmed (effect meshes,
// the PIP, the UI lane, a pass whose depth lanes differ from the run's) or past the copy's edge, so a read is the
// value the conversion gives there whichever way it is taken. C++: kh_fx_linz_draw, kh_fx_chain_run (binds t7 - t9
// and puts back what they held). t7 and KhLinZ are cb.hlsl's (the KH_FX_UNIT section: a user effect reads the same).
Texture2D<float> khGridZS : register(t8);
Texture2D<float> khGridZD : register(t9);
// KH_SSGI_CENTRE: the SSGI grid's size (local0.zw, set by the chain's SSGI block; unset, the frame over the scalar
// factor local0.y, 2 below 0.25) and the full-resolution pixel its texel t stands for - its block's centre, each
// axis by its own factor (fxMeta.zw over the grid's size: the grid's sides are rounded apart, so one factor from the
// width drifted up to ~3 px down the frame against the resolve, which reads the grid by normalized coords), held
// to the frame - KhDlfFull's mapping. The seed (26) samples the same centre; the gather (22), the grid's depth copy
// (PSFxLinZ 35) and KhSsgiGridZ's in-place read take this pixel. It was the block's first pixel, int2(t x factor):
// (factor - 1) / 2 pixels up and left of the texel the seed and the resolve take, 1.5 at setSsgiScale 0.25.
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
// its copy or to nothing (kh_fx_chain_run), so the size query and the load (ahead of the branch, texel 0 when
// unarmed or past the copy) meet a view of ours or none.
float KhSsgiGridZ(int2 khsz_t)
{
    uint khsz_w, khsz_h;
    khGridZS.GetDimensions(khsz_w, khsz_h);
    const bool khsz_in = matCtl.w > 0.5f && matCtl.w < 1.5f && khsz_t.x >= 0 && khsz_t.y >= 0 &&
                         khsz_t.x < (int)khsz_w && khsz_t.y < (int)khsz_h;
    float khsz_z = khGridZS.Load(int3(khsz_in ? khsz_t : int2(0, 0), 0));
    [branch] if (!khsz_in) khsz_z = KhLinZ(KhSsgiFull(khsz_t));   // KH_SSGI_CENTRE.
    return khsz_z;
}

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
    fs_s = KhFxFogPasses(fs_s, fs_d, fxParams1, fxParams2);   // KH_FX_FOGREC: the KH fog passes, one body (cb.hlsl).
    return saturate(fs_s);
}
// KH_WSEE: fog scatter's per-pixel fog share at fs_p of view depth fs_d - where the world's blended layers cover it,
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

// KH_DLF - dynamicLightFog (effect 31): the world fog lit by the engine's dynamic lights, marched per light through
// the light's own reach and shadowed by our meshes' light maps and by the geometry the camera sees. C++ side:
// kh_dlf_select (the lights: the engine's cb11 records, camera-relative, [5].z = our map's slot + 1 or 0, [5].w = the
// light's reach in metres), kh_fx_chain_run's KH_DLF block (gather 32 onto the SSGI grid, filter 33 twice, then the
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
static const float KH_DLF_SS_RISE    = 1.0f;    // KH_DLF_SS: m straight up from a surface's point that
                                                 // must still show that surface (within a tenth, plus THICK_K of the
                                                 // depth) for it to STAND - a wall - and be continued behind a nearer
                                                 // one; the ground, a roof, a ceiling recede and are not. No C++ twin.
// KH_DLF_SS_R2: no per-light verdict from single-pixel probes about the light (a walled-light or enclosing-plane
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
// past maxDistance at maxDistance (the march's own end, so equal keys march equal paths; KH_DLF_FOGREC: KhDlfKeyZ moves
// the sky and the far fence's feather band apart under a fog pass drawn after this one). The gather, the filter and the
// composite share both (a texel spans fxMeta.zw / khdf_g pixels).
int2 KhDlfFull(int2 khdf_t, float2 khdf_g)
{
    const int2 khdf_p = int2((float2(khdf_t) + 0.5f) * float2(fxMeta.z, fxMeta.w) / max(khdf_g, 1.0f));
    return clamp(khdf_p, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
}
float KhDlfMaxD() { return clamp(fxParams1.y, 10.0f, 5000.0f); }
// KH_DLF_FOGREC: with a fog pass drawn after this one (KhDlfPassMedium's khdm_tz) a texel's glow is scaled for the
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
    return KhDlfKeyZ(KhLinZ(khdk_p));   // KH_FX_LINZ.
}
// KH_FX_LINZ: KhLinZ(KhDlfFull(t, g)) - KhDlfKey before KhDlfKeyZ, which KhDlfKeyG applies - through
// the grid's copy (t9): drawn with g = its own size, so it serves a reader whose g is that size; any other reads in
// place. Read only by the chain's dynamicLightFog pass (its filter and composite) once its lights are up - the only
// case either runs (dlCtl.x) - when it binds t9 to its copy or to nothing (kh_fx_chain_run), so the size query and
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
    return KhDlfKeyZ(KhDlfGridZ(khdk_t, khdk_g));   // KH_DLF_FOGREC.
}
// The engine's fog-end ramp on what lies khdn_m metres from the camera (PSMain / PSComposite's: fully fogged past the
// fog end when the engine terms carry one, fogEngine.w in [0.5, 1.5)), a factor on the camera leg's transmittance.
// (KhFxFogEngine applies the same ramp in front of the fog end; past it, it takes the sky's integral, KH_FOG_SKY.)
float KhDlfEndRamp(float khdn_m)
{
    return (fogParams.w >= 0.5f && fogEngine.w >= 0.5f && fogEngine.w < 1.5f)
         ? saturate((fogEngine.y - khdn_m) * fogEngine.z) : 1.0f;
}
// The world fog's extinction (1/m) at absolute height khds_y: the engine's height fog as PSMain / KhFxFogEngine apply
// it (engine terms: fogEngine.x at sea level falling off with fogParams.y; else fogParams.x * 0.0153 above
// fogParams.z), times fogScale (fxParams0.y), plus baseDensity (fxParams0.z) everywhere. Zero with the fog off.
// TWIN: C++ kh_dlf_medium_zero (KH_DLF_MEDIUM0) reads these terms to skip a pass whose medium is zero - a term
// added here is added there.
float KhDlfSigma(float khds_y)
{
    float khds_s = 0.0f;
    if (fogParams.w >= 0.5f) {
        // KH_FOG_EXP_CAP (composite2.hlsl's note): the falloff capped at e^60 under a negative decay.
        khds_s = (fogEngine.w >= 0.5f)
               ? fogEngine.x * exp(min(-fogParams.y * max(khds_y, 0.0f), 60.0f))
               : fogParams.x * 0.0153f * exp(min(-fogParams.y * max(khds_y - fogParams.z, 0.0f), 60.0f));
    }
    return max(khds_s, 0.0f) * max(fxParams0.y, 0.0f) + max(fxParams0.z, 0.0f);
}
// The optical depth of a straight path khdt_d metres long between heights khdt_y0 and khdt_y1 - with the engine terms
// KhFxFogEngine's form (the density at the lower end, decaying along the rise: the mesh fog's own); without them
// KhFxFogEngine takes the far end's density over the whole path, and this integrates the same fallback density
// instead, so the glow's extinction matches the fog it scatters in: flat below its base (fogParams.z), decaying above
// it. KH_DLF_TAU_BASE: the share of the path below the base takes the flat density - the decaying form alone thinned
// a path that dips below the base, and one wholly below it, to as little as a hundredth of its optical depth (a
// valley's lamps glowed through thick fog almost unattenuated); a path wholly above is as before. Scaled and floored
// as KhDlfSigma. The engine branch is the mesh fog's form and not KhDlfSigma's integral below sea level: it takes the
// density at max(lo, 0) decaying along the whole rise where KhDlfSigma holds it flat below 0, so a path that dips
// below sea level gets less optical depth than the fog it scatters in - at least 1 / (1 + x) of it, x = the height
// decay (fogParams.y) times the depth below 0 the path reaches (a quarter at x = 3, an eighth at 7), the bound
// approached by a long rise from there; no floor as x grows (y >= 0: within 1e-4 relative).
// Neither branch holds the mesh fog's below-layer term (fogBelow, the camera under the layer), nor does KhDlfSigma.
float KhDlfTau(float khdt_d, float khdt_y0, float khdt_y1)
{
    float khdt_w = 0.0f;
    if (fogParams.w >= 0.5f) {
        const float khdt_lo = min(khdt_y0, khdt_y1);
        if (fogEngine.w >= 0.5f) {
            const float khdt_s = fogEngine.x * exp(min(-fogParams.y * max(khdt_lo, 0.0f), 60.0f));   // KH_FOG_EXP_CAP.
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
// KH_DLF_FOGREC (the chain's own fog passes as a medium for dynamicLightFog, as fog scatter takes them -
// KH_FX_FOGREC): one fog pass's factor f at view depth khdq_z - KhFxFogPasses' ramp, far-fence
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
// KH_DLF_FOGREC: the chain's distance-fog passes as the light fog's medium at a sample of view depth khdm_z (khdm_rl
// metres of ray per unit of view depth), up to two records in khFxFog (the two most opaque fullscreen fog passes, as
// fog scatter takes them; C++ kh_dlf_fog_pack), fxParams2.x / .y = 1 where that record's pass draws before this one. A
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
// TWIN: C++ kh_dlf_medium_zero (KH_DLF_MEDIUM0) reads the record test and fogScale's factor here.
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
        khdm_k[khdm_i] = min(khdm_s[khdm_i] / max(khdm_t[khdm_i], 1.0e-30f), 1.0e30f);   // sigma (KH_FOG_EXP_CAP).
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
// KhDlsWorldFactor's loops (cb.hlsl), kh_dls_atten (C++, the distance terms alone).
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
// The distance at which a light's attenuation (KhDlfAtt without fade or cone) falls to half its peak: the radius of
// its bright core, which the march's equiangular samples crowd toward (KhDlfEquiInit). For a start offset of 0 or
// more the peak is 1 / max(c, 1) (the attenuation saturates at 1), so the half is where c + l x + q x^2 = 2 max(c, 1).
// A negative start ([4].x < 0: x never falls below -start) is not modelled - the distance comes out where the
// attenuation is anywhere from ~0.17 to nearly all of its value at the start (0.5 just below 0; for a constant term
// under 1 it dips to ~0.17 where the attenuation stops saturating, then rises toward all of it the further below 0
// the start); only the equiangular width takes it, so the estimate's noise moves, not its mean (KhDlfMisStep is
// unbiased for any width).
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
    const int   khds_ne = (int)((uint)(khds_n + 1) / 2u);   // KH_UDIV: as uint (khds_n is 4 - 64) - fxc's X3556.
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
// Our meshes' shadow of light map slot khdv_s at absolute point khdv_p: KhDlsShadow's lookup (the face, the bias -
// the slot's constant and slope plus one texel - and the range fade) as a single nearest compare: a fog sample has no
// surface, so no normal offset and no receiver-plane gradient, and the march and the grid filter do the smoothing.
// 1 = lit. TWIN: KhDlsShadow (cb.hlsl).
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
    const float khsp_sd = KhLinZ(khsp_p);   // KH_FX_LINZ.
    return float3(khsp_vz - khsp_sd, khsp_sd, 1.0f);
}
// Whether a probe lies behind the visible surface, past a bias of 5 cm and a share of its depth (the sample's own
// surface never counts).
bool KhDlfSsHidden(float3 khsh_p)
{
    return khsh_p.x > 0.05f + KH_DLF_SS_BIAS_K * (khsh_p.x + khsh_p.y);
}
// KH_DLF_SS: whether the camera's view of the light itself is covered - its pixel shows a surface more
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
// KH_DLF_SS - the plane continuation, gated by khsv_cov (KhDlfSsCovered: the light itself is hidden
// behind a surface more than KH_DLF_SS_SELF nearer than it). A jump to a NEARER surface from in front of a farther
// one is the segment slipping behind something that stands before the surface it is about to cross - a soldier
// before the wall of the room the light is in: the crossing lies at pixels he covers, and without this the sample
// stays lit, his surroundings glow with a light the wall hides. With the light covered, and the far surface STANDING
// (one probe KH_DLF_SS_RISE straight up from its point still shows it at that depth: a wall; the ground - the lamp
// behind him over open ground - a roof or a ceiling recede from there and are left alone), the far surface is taken
// to go on behind the nearer one at its view depth: where the segment reaches that depth, short of the light, one
// probe reads the pixel there - the nearer surface still covering it, or the far one itself showing within
// KH_DLF_SS_THICK, is the crossing (occluded, under the same KH_DLF_SS_SELF rule); something farther (the far
// surface ended: a doorway, the sky) concludes nothing, as before. Only `return 0` is added: every crossing found
// without it is still found, and with khsv_cov false (the light seen, or off the screen) nothing here runs. Limits:
// a wall that ENDS behind the nearer one - its corner, with the light beyond it - casts a false shadow around the
// nearer one's silhouette, only while the light itself is hidden; a wall whose top lies within KH_DLF_SS_RISE of the
// crossing is not continued (today's leak there); a light within KH_DLF_SS_SELF of its wall still leaks (the housing
// rule); the gate is hard, so the glow switches as the nearer one's edge crosses the light's pixel.
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
            // KH_DLF_SS: the plane continuation (the note above). The far surface's depth, khsv_ext, is
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

// KH_SSGI_VB - the SSGI gather's visibility (effect 22: PSSsgiGather): screen-space indirect light with visibility
// bitmasks (Therrien, Levesque and Gilet 2023; the method of Pascal Gilcher's iMMERSE MXAO). The receiver's hemisphere
// is cut into slices through its view ray; along each, the samples march outward from the pixel. A sample stands for a
// solid of thickness KH_SSGI_THICK behind its visible face (along its own view ray): seen from the receiver it spans
// the angles from its front to its back, and it occupies the slice's sectors over that span. Its light reaches the
// receiver through the sectors it occupies that no nearer sample already held - so a blocker in front of it shadows
// it where it truly covers it, light passes behind a thin object, and an object in front of the receiver's view ray
// (nearer the camera) blocks only the few sectors it spans. A sample stands for its stretch of the line (half the gap
// to each neighbour), laid on its own surface's plane, so the samples tile the line and the coverage thins less with
// a smaller budget. The 32 sectors of a slice split its N.L lobe about the normal's projection evenly (KhSsgiCum; the
// bias sharpens the lobe as it did the former gather's N.L), so a sector's share is its N.L-weighted share; a slice
// weighs its normal's projected length times its lobe's integral, and the sum is normalized by the bias-free total,
// so a higher bias dims the result as it did before, if less steeply at the top (bias 0.9: about a seventh of 0.15's,
// the former gather's a sixteenth).
static const float KH_SSGI_THICK   = 1.0f;   // m.
static const float KH_SSGI_VB_GAIN = 0.33f;   // The mean onto the former gather's (open scenes, 12).
// Brightness is kept independent of the sample budget: fewer samples still cover fewer sectors (a sender on an edge
// is a point, not a stretch; the thickness's ends), measured as a mean rising with n about as (n / 12)^
// KH_SSGI_VB_NEXP (within about 15 % over 4 - 32 samples); the result is scaled by its inverse, so the
// budget trades noise, not brightness, as the former gather's count normalization did.
static const float KH_SSGI_VB_NEXP = 0.21f;
// A slice's cumulative weight at angle khsc_a (radians from the normal's projection) for a bias khsc_b: the integral
// of the lobe (cos a - b) / (1 - b) that the former gather weighted N.L by (b = 0: the plain cosine), times (1 - b)
// (the gather divides the slice's total by it). It rises monotonically over [-acos b, acos b], so it is the sectors'
// distribution function. The solid angle's own |sin| about the view ray (GTAO's slice measure) is left out, as
// MXAO's bitmask mode leaves it: against a ray-traced reference it fitted worse overall in the scenes tried.
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

// KH_WSEE (cb.hlsl's note): the two depth-derived quantities of PSEffect's own that a covered pixel takes at both
// depths, each from a view depth z (LinDepth's) - the effect's body after its depth read, so the front is today's.
// Distance fog's factor (effect 13): the ramp, the far fence's feather toward skyAmount, the sky's skyAmount. Twin
// edit at cb.hlsl's KhFxFogPasses (KhFsFog's and KhFxFogAt's fog-pass loop).
float KhFxDistFogF(float d)
{
    float f = saturate((d - fxParams0.x) / max(fxParams0.y - fxParams0.x, 1.0f));
    float khfg_f = KhEncFence();
    float khfg_w = saturate((min(d, khfg_f) - khfg_f * 0.98f) / max(khfg_f * 0.019f, 1.0f));
    f = lerp(f, saturate(fxParams0.z), khfg_w);
    if (d > 1e8f) f = saturate(fxParams0.z);   // Sky/far-plane pixels (belt; the feather
                                               // Already lands here).
    return f;
}
// KH_WSEE: the distance fog's factor for a stage's layers' own light by layer group - the mean of the factor over the
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
// KH_WSEE_STAGE_MEAN: a stage's front factor without the layer-depth split - the mean over the stage's groups as they
// stand in the record: the fence-type layers at the live depth khws_r (Tw) and the particle-type phase at its nearest
// khws_n (its T), by their shares of the stage's opacity front to back (KhWseeFogMean). Before, the stage's light took
// the factor at min(r, NWpost) alone - right for one layer, wrong where an opaque-like fence-type layer (a sea surface
// recorded as one, foliage) stands far behind a faint near one: all of the stage's light, nearly all of it the far
// layer's, was fogged as the near one (a rain streak before a fogged sea cut the fog out). With one group the mean is
// that group's own factor: at r for the fence-type layers alone (min(r, NWpost) with no particle layer), at NWpost
// for the particle phase alone (clamped to r, where min(r, NWpost) put it). The split (khWsSp) takes its own richer
// mean (KhWseeFogMean over the bands), as before.
float KhWseeStageMean(float khws_r, float khws_tr, float khws_n, float khws_tn, float khws_b, float khws_qb, float khws_d)
{
    return KhWseeFogMean(float4(khws_r, min(khws_n, khws_r), khws_b, khws_b), float4(khws_tr, khws_tn, 1.0f, 1.0f),
                         khws_b, khws_qb, khws_d);
}
// Pulse's ring (effect 11) at view depth z: khpl_c = its centre relative to the camera (KH_LOCAL_REL). Fenced
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

// KH_OL_D2 (outline, effect 10): w = 1 / z at p, and one axis's depth edge as the second difference of w, which is 0
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
// at KhDepthPx(px): the near-z mark and KH_VM_SEE are never armed there).
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
        // KH_GRADE_TINT: brightness, saturation, then the tint, then contrast - before the saturation the tint would
        // only move the luma weights, and saturation 0 would leave a tinted grade grey. TWIN: KhFusePoint's id 2.
        float3 c = scene * fxParams0.z;
        float l = Luma(c);
        c = lerp(l.xxx, c, fxParams0.x) * color.rgb;
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
        // KH_FX_ROUND: the shift grows with the radius on the picture - x in heights (x the aspect), as the sun
        // flare measures it - so it is round; in uv the side edges, 1.78 x farther at 16:9, shifted as the top
        // and bottom. Unchanged along the vertical: strengthPx / 2 at the top and bottom edges.
        float2 off = float2((uv.x - 0.5f) * (fxMeta.z / fxMeta.w), uv.y - 0.5f)
                   * fxParams0.x * KhFxPx();   // KH_FX_PX_REF.
        float r = KhSampleSceneBil(i.pos.xy + off).r;   // KH_FX_BILINEAR.
        float b = KhSampleSceneBil(i.pos.xy - off).b;
        outc = float3(r, scene.g, b);
    }
    else if (effect == 5)   // Film grain: [amount, fps, grainSizePx, lumaResponse, chroma].
    {
        // Filmic grain: smooth value noise with spatial extent, triangular
        // amplitude distribution, response peaking at mid-grey and falling
        // toward shadows and highlights alike, optional chroma, time quantized
        // to a frame rate.
        const float3 gc = KhGrainGc(i.pos.xy, t, fxParams0, fxParams1.x);   // KhFusePoint's twin: one body.

        float luma = saturate(Luma(scene));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, fxParams0.w);
        // KH_GRAIN_FLOOR: the zero-mean grain took a dark pixel below zero, a negative radiance in the HDR
        // scene; floored as sharpen's output is. TWIN: KhFusePoint id 5.
        outc = max(scene + gc * fxParams0.x * resp, 0.0f);
    }
    else if (effect == 6)   // Sharpen: [strength].
    {
        // KH_SHARPEN_LIMIT: the 5-tap Laplacian, limited to the five taps' own range per channel (the limiting of
        // contrast-adaptive sharpening). On the linear HDR scene the unlimited form took the dark side of any edge of
        // (1 + k) / k : 1 or more (3 : 1 at the default 0.5: every skyline against trees) to zero - a black line -
        // and lifted the bright side as far above it. The limit removes every overshoot, not only that one: a pixel
        // that is its five taps' brightest or darkest (either side of a hard edge, a one-pixel detail) is left as it
        // is, and a pixel between them steepens toward them as before, up to their range - a soft edge sharpens, a
        // hard one keeps no halo.
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
        outc = KhSampleSceneBil(i.pos.xy + off);   // KH_FX_BILINEAR.
    }

        else if (effect == 10)   // Outline: [depthEdgeScale, lumEdgeScale, sceneDarken, glowBoost],
                                 // Color = edge.
    {
        // KH_FX_PX_REF: the step is the drawn line's width - one reference pixel, to the nearest whole pixel - and a
        // step spanning the same part of the picture measures the same depth and luma differences, so the thresholds
        // hold at any size.
        const int khol_d = max((int)floor(KhFxPx() + 0.5f), 1);
        // The depth taps stay inside the frame (KhOlAxis picks its forms so), as SampleScene clamps the luma taps:
        // LoadDepthPS does not, and a Load past the frame's edge reads 0 - the near plane - which would draw an
        // edge line along the borders.
        const int2 khol_hi = int2((int)fxMeta.z - 1, (int)fxMeta.w - 1);
        // KH_OL_D2: the depth edge is the second difference of w = 1 / z (z = LinDepth), which is affine in screen
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
        // Cost: five depth reads a pixel (LoadDepthPS: up to three loads each - the depth, the near-z mark, KH_VM_SEE)
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
        // KH_LOCAL_REL (cb.hlsl's localization mask): the camera-relative point against the centre made
        // relative to the camera, exactly. The ring itself: KhFxPulseRing.
        precise float3 khpl_c = fxParams0.xyz - (fxCam.w > 0.5f ? fxCam.xyz : float3(0.0f, 0.0f, 0.0f));
        const float khpl_r = LoadDepthPS(px);
        float ring = KhFxPulseRing(khpl_c, uv, LinDepth(khpl_r));
        // KH_WSEE: the ring on what lies behind the world's blended layers shows through them by their
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
    else if (effect == 13)   // Distance fog: [startDist m, endDist m, skyAmount 1], color = fog
                             // Color.
    {
        // The factor: KhFxDistFogF (twin edit at cb.hlsl's KhFxFogPasses).
        const float khfg_r = LoadDepthPS(px);
        const float f = KhFxDistFogF(LinDepth(khfg_r));
        outc = lerp(scene, color.rgb, f);
        // KH_WSEE: S = Tr B + E (E the layers' own light) - the background fogged by f behind, the layers' light by f
        // in front: Tr lerp(B', g, f_b) + (1 - f_f) max(S - Tr B', 0) + (1 - Tr) f_f g, with B' = max(B + (S - R), 0)
        // (the earlier chain passes' change to the pixel, given to the background too). Exact for one blended layer
        // in the first chain pass that changes the colour; approximate after earlier passes and for several layers.
        // Only where G < 0.75 (G = 0, or a derived pre with B2: B / B2 is the background) and KH_VM_SEE's planes do
        // not also cover the pixel; elsewhere covered the factor blend lerp(S, g, Tr f_b + (1 - Tr) f_f). Per sample
        // where the samples disagree: with khWsC each sample's colour C_k' = C_k + (S - R) takes its own rule in place
        // of S (lerp(C_k', g, f(r_k)) uncovered) and the pixel their mean - unclamped, so equal factors give lerp(S, g,
        // f) exactly; without it lerp(S, g, mean of the samples' factors). One-entry caches keep KhFxDistFogF to the
        // depths not yet seen (KhWseeMix's).
        [branch] if (KhWseeArmed()) {
            // KH_WSEE_EDGE_COST: the pairs are classes where khWsE's word stands (cb.hlsl KhWseeCountW), each
            // weighed by its members and coloured by their mean C (KhWseeClassC); else every sample.
            float2       khfg_g;
            uint         khfg_w;
            const uint   khfg_n = KhWseeCountW(px, khfg_r, khfg_g, khfg_w);
            const bool   khfg_pm = khfg_n > 1u || khfg_w != 0u;   // KH_WSEE_EDGE_COST: the per-sample form.
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
                // KH_WSEE_FAINT (cb.hlsl): faint layers alone, the behind the depth buffer's - the factor at r (f),
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
                const float khfg_m = KhWseeClassM(khfg_w, khfg_k);   // KH_WSEE_EDGE_COST: the pair's weight.
                khfg_fa += khfg_m * khfg_fk;
                [branch] if (khfg_co) {
                    float3 khfg_sc = scene;
#if MSAA_DEPTH
                    [branch] if (khfg_cs) khfg_sc = KhWseeClassC(px, khfg_w, khfg_k) + khfg_d;   // KH_WSEE_EDGE_COST.
#endif
                    float3 khfg_ok = lerp(khfg_sc, color.rgb, khfg_fk);
                    // TWIN: cb.hlsl KhWseeBehindColor is the per-pixel form of the three stages below (the colour
                    // behind and the capped transmittance it gives the user contract) - KH_WSEE_BEHIND.
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
                                } else {   // KH_WSEE_STAGE_MEAN.
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
                                    } else {   // KH_WSEE_STAGE_MEAN.
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
                                    } else {   // KH_WSEE_STAGE_MEAN: the one stage's three groups (KhWseeGroupsSet's).
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
            const float khfg_dv = KhWseeDivW(khfg_w, khfg_n);   // KH_WSEE_EDGE_COST: the weights' sum.
            if (khfg_c) outc = khfg_co ? khfg_ca / khfg_dv : lerp(scene, color.rgb, khfg_fa / khfg_dv);
        }
    }
    else if (effect == 14)   // Lens flare, image-based: [threshold, intensity, ghostCount,
                             // ghostSpacing] + [haloRadius, haloIntensity, chromaPx.
    {
        // Bright pixels anywhere in the capture spawn a ghost train mirrored
        // through screen centre, plus a halo ring. The HDR capture makes the
        // threshold physically meaningful.
        float2 cuv = float2(0.5f, 0.5f);
        float2 ghostVec = (cuv - uv) * fxParams0.w;
        int nGhosts = clamp((int)fxParams0.z, 1, 8);
        float3 acc = 0.0f;
        // KH_FX_ROUND: the ghosts' window, the halo ring and the colour fringe's direction are measured on the
        // picture - x in heights (khlf_a, the aspect), as the sun flare measures - so the lens's circles are round;
        // in uv they were ellipses 1.78 x wider at 16:9. haloRadius is in heights (unchanged along the vertical).
        const float2 khlf_a = float2(fxMeta.z / fxMeta.w, 1.0f);
        // KH_GLOW_PYR: a ghost is the picture scaled by 1 - ghostSpacing * g about the centre, so a point read
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
                        * fxParams1.z * KhFxPx();   // KH_FX_PX_REF.
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
        // KhSunFlareSpot above (KhSunFlareVis / KhSunFlareEdge: cb.hlsl). KH_FX_SIDE: the scene chain draws the
        // visibility (edge fade included) once, 1 x 1, before the pass (side id 29) and arms matCtl.y; unarmed, each
        // pixel measures it.
        float2 spos;
        int2 sp;
        if (KhSunFlareSpot(spos, sp))
        {
            {
                float vis;   // A branch, not ?: (which evaluates both arms).
                [branch] if (matCtl.y > 0.5f) vis = khFxSide.Load(int3(0, 0, 0));
                else                          vis = KhSunFlareVis(sp) * KhSunFlareEdge(spos);   // KH_FLARE_EDGE.
                if (vis > 0.001f)
                {
                    float aspect = fxMeta.z / fxMeta.w;
                    float2 d = uv - spos;
                    d.x *= aspect;
                    float r = length(d) / max(fxParams0.w, 0.01f);
                    // KH_FLARE_ATAN: atan2(0, 0) is NaN as fxc lowers it (0 x rcp(0)) - the pixel whose centre is
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
    else if (effect == 29)   // KH_FX_SIDE: the sun flare's source visibility (1 x 1, drawn before the pass).
    {
        float2 spos;
        int2 sp;
        float khsv_v = 0.0f;   // A branch, not ?: (which evaluates both arms).
        [branch] if (KhSunFlareSpot(spos, sp)) khsv_v = KhSunFlareVis(sp) * KhSunFlareEdge(spos);   // KH_FLARE_EDGE.
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
        // KH_CLARITY_ID: each tap weighs 1 / 12 (it was 0.0833: the weights summed to 0.9996, so a flat picture
        // read a detail of 0.0004 x its luma and was scaled by 1.0003 at the default strength).
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
        // KH_CLARITY_ID: the floor guards the division only - the identity term stays 1. As (lC + s d 2.2) /
        // max(lC, 1e-3) it scaled any pixel under a luma of 1e-3 by lC / 1e-3 with no detail at all (strength 0, a
        // flat area): down to the 0.3 clamp, the darkest night shadows crushed. Equal above 1e-3 (to an ulp).
        float scale = 1.0f + fxParams0.x * detail * 2.2f / max(lC, 1e-3f);
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
        const float2 khrl_c = float2(px) + 0.5f;   // KH_FX_BILINEAR: the pixel's centre.
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
        khc_col.r = KhSampleSceneBil(khc_dpx + khc_fpx).r;   // KH_FX_BILINEAR (khc_dpx: the centre, unwarped).
        khc_col.g = KhSampleSceneBil(khc_dpx).g;
        khc_col.b = KhSampleSceneBil(khc_dpx - khc_fpx).b;

        // Beam scanlines: a shaped sine whose exponent narrows in the shadows
        // and widens toward the highlights (phosphor blooming); the 1.32 gain
        // lifts the average level the dark gaps remove (to 0.95 in the
        // highlights, 0.75 at mid-grey, 0.64 in the shadows).
        float khc_lum = saturate(Luma(khc_col));
        const float khc_bex = lerp(2.2f, 0.65f, khc_lum);
        float khc_beam = pow(abs(KhSin(khc_phase * 3.14159265f)),   // KH_HASH: the phase scrolls with t.
                             khc_bex);
        // KH_CRT_BANDLIMIT - the lines' period on screen: rows per line at this pixel, through the warp's vertical
        // stretch (d duv.y / d uv.y). Sampled once per pixel, a period of 2 rows or less cannot be drawn (at 2 a
        // phase of 0.25 lights every row alike, a phase of 0 alternates full and none, and the scroll swept between
        // them - blinking lines and moire bands); so the profile fades to its own mean (|sin|^e averaged over a line,
        // a fit within 0.005) from 3 rows per line down to 2. At 3 and more the lines are exactly as before.
        const float khc_dy = 1.0f + khc_curv * khc_r2 * (1.0f + 0.8f * khc_r2)
                           + 2.0f * khc_cc.y * khc_cc.y * khc_curv * (1.0f + 1.6f * khc_r2);
        const float khc_rows = fxMeta.w / max(khc_lines * khc_dy, 1.0e-4f);
        const float khc_mean = 0.88611f + khc_bex * (0.05438f * khc_bex - 0.30191f);
        if (khc_rows < 3.0f) khc_beam = lerp(khc_mean, khc_beam, smoothstep(2.0f, 3.0f, khc_rows));
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
        // KH_FX_ROUND: the corners measured on the picture - x in heights (khc_as, the aspect) - so they are
        // round and the feather is 2 px on both axes; in uv they were ellipses 1.78 x wider at 16:9 and the
        // feather 3.6 px across x. cornerRadius is in heights (unchanged along the vertical), at most half the
        // shorter side.
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
