// g_hlsl_white - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
struct VSOutW { float4 pos : SV_Position; };

VSOutW VSWhite(VSIn i)
{
    VSOutW o;
    float3 khw_l = KhRotate(i.pos * sizeAxes.xyz);
    float3 khw_p = (centerRel.w > 0.5f) ? (centerRel.xyz + khw_l)
                                        : (centerSize.xyz + khw_l);
    o.pos = mul(float4(khw_p, 1.0f), viewProj);
    return o;
}

float4 PSWhite(VSOutW i) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
)HLSL"
