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

// KH_VM_SEE: sample 0 of the live depth, raw - the sample cb.hlsl's LoadDepthRaw reads, so the effect chain compares
// a pixel's snapshot with its live depth bit for bit - into an R32_FLOAT target (kh_vmsee_snapshot).
float PSDepthS0(float4 pos : SV_Position) : SV_Target
{
    const int2 p = int2(pos.xy);
#if MSAA_DEPTH
    return resolveSrc.Load(p, 0);
#else
    return resolveSrc.Load(int3(p, 0));
#endif
}
