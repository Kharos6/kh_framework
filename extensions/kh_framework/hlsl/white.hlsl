// white.hlsl - plain HLSL, embedded in the DLL as RCDATA resource KH_WHITE_HLSL by
// kh_shaders.rc (next to rendering_integration.hpp) and loaded at first use
// by kh_hlsl_src, which strips CR before the source is hashed for the shader
// cache, so the cache key does not depend on the checkout's line endings.
// Units are assembled by C++ concatenation of these resources, exactly as the
// old raw-string splice did; there is no #include and no size cap. Any edit
// here changes this unit's shader cache key (one cold recompile per user).

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

