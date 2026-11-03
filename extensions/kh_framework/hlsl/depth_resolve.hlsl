// depth_resolve.hlsl - a standalone unit (no #include). Any edit changes the unit's shader cache key.

#if MSAA_DEPTH
Texture2DMS<float> resolveSrc : register(t0);
#else
Texture2D<float> resolveSrc : register(t0);
#endif

float2 PSDepthResolve(float4 pos : SV_Position) : SV_Target
{
    int2 p = int2(pos.xy);
#if MSAA_DEPTH
    float m = 0.0f;
    float n = 1.0f;
    [unroll] for (int s = 0; s < SAMPLE_COUNT; ++s) {
        float r = resolveSrc.Load(p, s);
        m = max(m, r);
        // Skip the far clear the same way the composite guard does, so an
        // uncovered sample cannot drag the nearest plane to the clear value.
        if (r > 0.000001f && r < 0.999999f) n = min(n, r);
    }
    if (n > m) n = m;
    return float2(m, n);
#else
    float d = resolveSrc.Load(int3(p, 0));
    return float2(d, d);
#endif
}

// VM_SEE: sample 0 of the live depth, raw - the sample cb.hlsl's LoadDepthRaw reads, so the effect chain compares
// a pixel's snapshot with its live depth bit for bit - into an R32_FLOAT target (vmsee_snapshot).
float PSDepthS0(float4 pos : SV_Position) : SV_Target
{
    const int2 p = int2(pos.xy);
#if MSAA_DEPTH
    return resolveSrc.Load(p, 0);
#else
    return resolveSrc.Load(int3(p, 0));
#endif
}

// PSB_ZB: the one pass's block bound Zb (C++ wsee_snapshot, right after S; compiled with MSAA_DEPTH 0 - t0 is S,
// 1x at the main depth's size - at KH_PSB_ZB_LS = log2 s, TWIN of PSB_LS): at the record's texel (X, Y) the largest
// of S over the block's s x s pixels (X s + (k & (s - 1)), Y s + (k >> log2 s)), a pixel past the frame's edge loads 0
// as the variant's loop does, and the first k holding it (the loop's first read). R32G32_FLOAT, a fullscreen triangle.
#ifndef KH_PSB_ZB_LS
#define KH_PSB_ZB_LS 1
#endif
float2 PSPsbZb(float4 pos : SV_Position) : SV_Target
{
    const int2 khzb_c = int2(pos.xy) << KH_PSB_ZB_LS;
    float khzb_m = -1.0f;
    uint khzb_k = 0u;
    [unroll] for (uint khzb_i = 0u; khzb_i < (1u << (2 * KH_PSB_ZB_LS)); ++khzb_i) {
        const int2 khzb_p = khzb_c + int2(khzb_i & ((1u << KH_PSB_ZB_LS) - 1u), khzb_i >> KH_PSB_ZB_LS);
#if MSAA_DEPTH
        const float khzb_v = resolveSrc.Load(khzb_p, 0);   // Compiled single-sample only (the C++ defines).
#else
        const float khzb_v = resolveSrc.Load(int3(khzb_p, 0));
#endif
        if (khzb_v > khzb_m) {
            khzb_m = khzb_v;
            khzb_k = khzb_i;
        }
    }
    return float2(khzb_m, float(khzb_k));
}

// WSEE: the late opaque replay's pixel shader (the store path) - an engine draw that writes depth after the
// snapshot, drawn again into khWsT with depth EQUAL: (1, 1, 1, 1) = T reset (the surface hides the layers behind it),
// G = 1. Inputs: system values alone, so the signature links after any vertex shader. It also keeps the opaque's OWN
// depth, D3 (u3: R32F, one value a pixel): the pass runs per SAMPLE (SV_SampleIndex) under [earlydepthstencil], so an
// invocation exists exactly for a sample this draw owns, and the sample-0 invocation stores the main depth's sample 0
// raw (resolveSrc t0: the depth's MS view beside its read-only DSV - the value PSDepthS0 reads, bit for bit). A store
// to an unbound UAV is dropped and an unbound t0 loads 0, so the same blob serves without D3. Compiled multisampled
// only (MSAA_DEPTH 1 at the live count; prewarmed at 2 / 4 / 8); the single-sample branch keeps the unit compiling.
// WSEE_OQ2: u3 - above the post twins' three targets (SV_Target0 - SV_Target2: T, Tp, D3m; PSWseeOneZ's note).
RWTexture2D<float> khWsD3 : register(u3);
void KhWseeD3Store(float4 pos, uint khs)
{
    const int2 p = int2(pos.xy);
#if MSAA_DEPTH
    if (khs == 0u) khWsD3[p] = resolveSrc.Load(p, 0);
#else
    if (khs == 0u) khWsD3[p] = resolveSrc.Load(int3(p, 0));
#endif
}
// WSEE_OQ2: a post replay's third target D3m (SV_Target2: R32_FLOAT at the main depth's sample count and quality,
// bound beside T and Tp - C++ wsee_re_set): the Z twins below write their own depth there, the store path
// (PSWseeOne / PSWseeOneL, the store above) writes -1 - "no depth here": the resolve (PSWseeD3Res) then keeps the
// store's D3. Written at exactly the samples T's G is (one pass, no blend state, the engine's sample mask). Without D3
// standing the post replay binds one or two targets: the third output is dropped (the D3D11 debug layer logs it).
struct KhWseeOneM { float4 t : SV_Target0; float z : SV_Target2; };
[earlydepthstencil]
KhWseeOneM PSWseeOne(float4 pos : SV_Position, uint khs : SV_SampleIndex)
{
    KhWseeD3Store(pos, khs);   // D3's store.
    KhWseeOneM o;
    o.t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.z = -1.0f;   // WSEE_OQ2: stored above, not here.
    return o;
}
// The late-opaque replay's MRT twins with Tp bound as the second target (khWsTp: pre in R, post in G, the 100 % band's
// in B / A - all four reset, no blend state on the pass) - reset with T's particle channels, so Tp's layers stay a
// subset of T's.
struct KhWseeOneLM { float4 t : SV_Target0; float4 p : SV_Target1; float z : SV_Target2; };   // WSEE_OQ2.
[earlydepthstencil]
KhWseeOneLM PSWseeOneL(float4 pos : SV_Position, uint khs : SV_SampleIndex)
{
    KhWseeD3Store(pos, khs);   // D3's store.
    KhWseeOneLM o;
    o.t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.p = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.z = -1.0f;   // WSEE_OQ2: PSWseeOne's.
    return o;
}
// WSEE_OQ2 - the post twins without D3's UAV and the depth view (the WSEE note's RE-ISSUES): the same T / Tp
// writes, and the opaque's depth at sample 0 into D3m (SV_Target2), at PIXEL rate (WSEE_PXZ) under
// [earlydepthstencil]: EQUAL still runs per sample ahead of the shader and the one invocation's outputs go to the
// covered samples that passed it, so T / Tp take exactly the samples the per-sample pass wrote, and D3m one value a
// pixel. The depth the engine wrote at sample 0 is the fixed-function z there after the format's conversion (D3D11.3
// 15.16.2 / 16.3.1: the PS's input z matches the interpolated z and carries no format quantization); D3m lies within
// KhWseeZAtS0's delta of it (PSWseeD3Res's tolerance). A depth bias, clip off, a wireframe fill, a non-triangle
// topology or a GS takes the store path (C++ wsee_d3m_path).
// WSEE_PXZ - KhWseeZAtS0: the centre's z plus its fine derivatives times sample 0's offset
// (GetRenderTargetSamplePosition: the bound targets' actual pattern). z is affine over a triangle and every lane of
// the quad (helpers, centres off the triangle) evaluates the same interpolator, so delta <= (2 + 2 |ox| + 2 |oy|) E +
// two float steps, E the hardware interpolator's own deviation from a plane (D3D11 fixes none). Modelled delta:
// fixed-point edge functions with fp32 interpolation <= 4 D24 steps; an fp32 vertex-relative evaluator up to 15 on
// ~1e-5 of grazing samples near the camera; a naive absolute fp32 plane up to ~1150 on ~0.02 %. Every covered sample
// of the pixel takes the one value.
float KhWseeZAtS0(float4 pos)
{
#if MSAA_DEPTH
    const float2 khzs_o = GetRenderTargetSamplePosition(0);
    return pos.z + ddx_fine(pos.z) * khzs_o.x + ddy_fine(pos.z) * khzs_o.y;
#else
    return pos.z;
#endif
}
[earlydepthstencil]
KhWseeOneM PSWseeOneZ(float4 pos : SV_Position)   // WSEE_PXZ: pixel rate.
{
    KhWseeOneM o;
    o.t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.z = KhWseeZAtS0(pos);
    return o;
}
[earlydepthstencil]
KhWseeOneLM PSWseeOneZL(float4 pos : SV_Position)   // WSEE_OQ2: PSWseeOneZ's MRT twin (Tp reset too); PXZ.
{
    KhWseeOneLM o;
    o.t = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.p = float4(1.0f, 1.0f, 1.0f, 1.0f);
    o.z = KhWseeZAtS0(pos);
    return o;
}
// WSEE_OQ2 - D3 from D3m, once before a reader binds D3 (C++ wsee_d3m_resolve; a fullscreen pass into D3's RTV,
// sample 0). Only where sample 0's last late opaque was a post one (T's G > 0.75) and wrote D3m (z > -0.5: not the
// store path's -1); else discarded (D3 keeps its value; where no late opaque owns sample 0 it holds none and no
// reader takes it). There D3 = the live sample 0 r where z lies within KH_WSEE_D3_TOL of it, else z. Where nothing
// moved sample 0 since the replay r IS the engine's value, so D3 is the store's bit for bit GIVEN |z - stored| <= TOL
// (KhWseeZAtS0's delta + the conversion's 0.6 step; nothing here measures the hardware's interpolator); where a
// fence-type layer or an untracked writer moved it, z (the opaque's own) or r where the writer lies within TOL of z.
// KH_WSEE_D3_TOL = 8 steps of a 24-bit UNORM depth (a 32-bit float depth within the same absolute bound). t0 the
// main depth (MS view), t1 T, t2 D3m.
#define KH_WSEE_D3_TOL (8.0f / 16777215.0f)
#if MSAA_DEPTH
Texture2DMS<float4> khWsResT : register(t1);
Texture2DMS<float>  khWsResM : register(t2);
#endif
float PSWseeD3Res(float4 pos : SV_Position) : SV_Target
{
#if MSAA_DEPTH
    const int2 p = int2(pos.xy);
    const float4 khdr_t = khWsResT.Load(p, 0);
    const float khdr_z = khWsResM.Load(p, 0);
    if (!(khdr_t.y > 0.75f) || !(khdr_z > -0.5f)) discard;
    const float khdr_r = resolveSrc.Load(p, 0);
    return abs(khdr_z - khdr_r) <= KH_WSEE_D3_TOL ? khdr_r : khdr_z;
#else
    discard;   // Multisampled only (wsee_ensure); the branch keeps the unit compiling at 1x.
    return 0.0f;
#endif
}
