// sunpf.hlsl - plain HLSL, embedded in the DLL as RCDATA resource KH_SUNPF_HLSL by
// kh_shaders.rc (next to rendering_integration.hpp) and loaded at first use
// by kh_hlsl_src, which strips CR before the source is hashed for the shader
// cache, so the cache key does not depend on the checkout's line endings.
// Units are assembled by C++ concatenation of these resources, exactly as the
// old raw-string splice did; there is no #include and no size cap. Any edit
// here changes this unit's shader cache key (one cold recompile per user).

Texture2D<float> khpf_src : register(t0);
struct VSOPf { float4 pos : SV_Position; };
VSOPf VSPf(uint vid : SV_VertexID)
{
    VSOPf o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return o;
}
float4 PSPf(VSOPf i) : SV_Target
{
    int2 khpf_p = int2(i.pos.xy) * 2;
    float z0 = saturate(khpf_src.Load(int3(khpf_p, 0)));
    float z1 = saturate(khpf_src.Load(int3(khpf_p + int2(1, 0), 0)));
    float z2 = saturate(khpf_src.Load(int3(khpf_p + int2(0, 1), 0)));
    float z3 = saturate(khpf_src.Load(int3(khpf_p + int2(1, 1), 0)));
    float khpf_mu = 0.25f * (z0 + z1 + z2 + z3);
    float khpf_m2 = 0.25f * (z0 * z0 + z1 * z1 + z2 * z2 + z3 * z3);
    return float4(khpf_mu, khpf_m2, 0.0f, 0.0f);
}
// Reads the scratch copy (t1) - never its own resource.
Texture2D<float2> khpfm_src : register(t1);
float4 PSPfMip(VSOPf i) : SV_Target
{
    int2 khpfm_p = int2(i.pos.xy) * 2;
    float2 khpfm_a = saturate(khpfm_src.Load(int3(khpfm_p, 0)));
    float2 khpfm_b = saturate(khpfm_src.Load(int3(khpfm_p + int2(1, 0), 0)));
    float2 khpfm_c = saturate(khpfm_src.Load(int3(khpfm_p + int2(0, 1), 0)));
    float2 khpfm_d = saturate(khpfm_src.Load(int3(khpfm_p + int2(1, 1), 0)));
    float2 khpfm_m = 0.25f * (khpfm_a + khpfm_b + khpfm_c + khpfm_d);
    return float4(khpfm_m.x, khpfm_m.y, 0.0f, 0.0f);
}

