// White.hlsl - RCDATA resource in kh_shaders.rc, concatenated into its unit by C++ (no #include). Any edit changes the unit's shader cache key.

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
