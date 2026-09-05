// composite.hlsl - joined with composite2.hlsl after cb.hlsl into the composite unit (no #include). Any edit changes the unit's shader cache key.

#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float GuardSceneRaw(int2 px)
{
    float m = 0.0f;

    [unroll] for (int s = 0; s < SAMPLE_COUNT; ++s) {
        float r = depthTex.Load(px, s);
        if (r > 0.000001f && r < 0.999999f) m = max(m, r);
    }

    return m > 0.0f ? m : 1.0f;   // All-clear pixel reads as the far clear.
}
#else
// Two-plane snapshot: .x farthest (guard), .y nearest (witness).
Texture2D<float2> depthTex : register(t0);
// Raw guard read: the snapshot's .x (farthest plane) at the pixel.
float GuardSceneRaw(int2 px)
{
    return depthTex.Load(int3(px, 0)).x;
}
#endif

Texture2D<float4> sceneColorTex : register(t3);
