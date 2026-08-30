// depth_resolve.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.

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
#if WIT_NEAREST
    return float2(m, n);
#else
    return float2(m, m);
#endif
#else
    float d = resolveSrc.Load(int3(p, 0));
    return float2(d, d);
#endif
}
