// white.hlsl - concatenated after cb.hlsl into the white unit (no #include). Any edit changes the unit's shader cache key.

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

// KH_WSEE: in place of a sorted tail part's user material (pending or failed - kh_draw_textured), the same test
// and composite as PSMain's (cb.hlsl KhWseeMeshCut / KhWseeMeshOut; the lane is set on normal blend alone): that draw
// has no depth test. Unarmed, flat unlit white (KH_MESH_O1: the colour alone, KhMeshOutC). The test's
// per-sample verdict goes out as SV_Coverage (cb.hlsl khMsCov, PSMain's khA2cCov twin; every sample unarmed - the
// raster's coverage stands).
KhMeshOut PSWhite(VSOutW i, out uint khwc_cov : SV_Coverage)
{
    KhWseeMeshCut(i.pos);
    khwc_cov = khMsCov;
    [branch] if (KhWseeMeshMode() > 0.5f) return KhWseeMeshOut(float3(1.0f, 1.0f, 1.0f), 1.0f);
    return KhMeshOutC(float4(1.0f, 1.0f, 1.0f, 1.0f));
}
