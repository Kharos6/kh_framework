// g_hlsl_depth_resolve - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
#if MSAA_DEPTH
Texture2DMS<float> resolveSrc : register(t0);
#else
Texture2D<float> resolveSrc : register(t0);
#endif

// WIT_NEAREST 0 (mode 178) writes the farthest into BOTH planes, which is
// exactly behaviour - the revert costs a recompile, not a branch.
float2 PSDepthResolve(float4 pos : SV_Position) : SV_Target
{
    int2 p = int2(pos.xy);
#if MSAA_DEPTH
    float m = 0.0f;
    float n = 1.0f;
    [unroll] for (int s = 0; s < SAMPLE_COUNT; ++s) {
        float r = resolveSrc.Load(p, s);
        m = max(m, r);
        // skip the far clear the same way the composite guard does, so an
        // uncovered sample cannot drag the nearest plane to the clear value
        if (r > 0.000001f && r < 0.999999f) n = min(n, r);
    }
    if (n > m) n = m;   // no covered sample: both planes agree, as before
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
)HLSL"
