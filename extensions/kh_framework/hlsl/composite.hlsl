// composite.hlsl - joined with composite2.hlsl after cb.hlsl into the composite unit (no #include). Any edit changes the unit's shader cache key.

// Two-plane snapshot: .x farthest (guard), .y nearest (witness). The snapshot
// is ALWAYS single-sample - snapshot_composite_depth pins comp_depth_samples
// to 1, and PSDepthResolve (compiled at the live count) folds the samples into
// the farthest plane - so this unit is only ever compiled MSAA_DEPTH 0. The
// Texture2DMS<float> path this file used to carry under #if MSAA_DEPTH was
// therefore never built, and is gone; MSAA_DEPTH and SAMPLE_COUNT are still
// passed, and still join the cache key.
Texture2D<float2> depthTex : register(t0);
// Raw guard read: the snapshot's .x (farthest plane) at the pixel.
float GuardSceneRaw(int2 px)
{
    return depthTex.Load(int3(px, 0)).x;
}

Texture2D<float4> sceneColorTex : register(t3);
