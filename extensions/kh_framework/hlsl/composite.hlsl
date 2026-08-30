// composite.hlsl - plain HLSL, embedded in the DLL as RCDATA resource KH_COMPOSITE_HLSL by
// kh_shaders.rc (next to rendering_integration.hpp) and loaded at first use
// by kh_hlsl_src, which strips CR before the source is hashed for the shader
// cache, so the cache key does not depend on the checkout's line endings.
// Units are assembled by C++ concatenation of these resources, exactly as the
// old raw-string splice did; there is no #include and no size cap. Any edit
// here changes this unit's shader cache key (one cold recompile per user).

#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float GuardSceneRaw(int2 px)
{
    float m = 0.0f;

    [unroll] for (int s = 0; s < SAMPLE_COUNT; ++s) {
        float r = depthTex.Load(px, s);
        if (r > 0.000001f && r < 0.999999f) m = max(m, r);
    }

    return m > 0.0f ? m : 1.0f;   // all-clear pixel reads as the far clear
}
// Sample 0: the snapshot is pinned single-sample - this arm is compile
// parity, like GuardSceneRaw's.
float KhWitLoad(int2 khwl_p) { return depthTex.Load(khwl_p, 0); }
#else
// two-plane snapshot -.x farthest (guard),.y nearest (witness).
Texture2D<float2> depthTex : register(t0);
// RAW guard read: the snapshot value at the pixel. UNCHANGED -.x is the
// farthest plane, exactly what this read has always returned.
float GuardSceneRaw(int2 px)
{
    return depthTex.Load(int3(px, 0)).x;
}
// the composite-chunk KhWitLoad body (see the MSAA arm's note).
float KhWitLoad(int2 khwl_p) { return depthTex.Load(int3(khwl_p, 0)).y; }
#endif

// Defined per terminal chunk over that chunk's own KhWitLoad, textually
// before its PS body - fxc requires it. TWIN EDIT: both chunk copies of this
// function must stay byte-identical.
float KhVolSoftScene(float2 khfs_r, float3 khfs_p, float khfs_tol, int khfs_m)
{
    int2 khfs_c = KhVolPx(khfs_r);
    if (abs(KhWitLoad(khfs_c) - khfs_p.x) <= khfs_tol)
        return KhVolShadowed(KhVolCount(khfs_c), khfs_m) ? 0.0f : 1.0f;

    float khfs_s = 0.0f;
    float khfs_w = 0.0f;
    // MODE 196 = SNAP, DO NOT BLUR. Tracks the single best-matching tap so
    // the crisp count can be taken from IT instead of from an average. Ledger
    // at
    float khfs_bk = 0.0f;
    int2  khfs_bq = khfs_c;

    for (int khfs_j = -3; khfs_j <= 3; ++khfs_j) {
        for (int khfs_i = -3; khfs_i <= 3; ++khfs_i) {
            int2  khfs_q = KhVolPx(khfs_r + float2(khfs_i, khfs_j));
            float khfs_e = khfs_p.x + khfs_p.y * khfs_i + khfs_p.z * khfs_j;
            float khfs_d = abs(KhWitLoad(khfs_q) - khfs_e);
            float khfs_k = saturate(1.0f - khfs_d / max(khfs_tol * 3.0f, 1.0e-9f));
            khfs_k *= khfs_k;
            khfs_s += khfs_k * (KhVolShadowed(KhVolCount(khfs_q), khfs_m) ? 0.0f : 1.0f);
            khfs_w += khfs_k;
            if (khfs_k > khfs_bk) { khfs_bk = khfs_k; khfs_bq = khfs_q; }
        }
    }

    if (dbgCtl.w >= 9.5f && dbgCtl.w < 10.5f && khfs_bk > 0.0f)
        return KhVolShadowed(KhVolCount(khfs_bq), khfs_m) ? 0.0f : 1.0f;

    if (khfs_w < 1.0e-4f) {
        if (dbgCtl.w >= 11.5f && dbgCtl.w < 12.5f)
            return KhVolShadowed(KhVolCount(khfs_c), khfs_m) ? 0.0f : 1.0f;   // 364: point read
        float khfs_mx = 0.0f;
        for (int khfs_v = -1; khfs_v <= 1; ++khfs_v)
            for (int khfs_u = -1; khfs_u <= 1; ++khfs_u)
                if (KhVolShadowed(KhVolCount(KhVolPx(khfs_r + float2(khfs_u, khfs_v))), khfs_m))
                    khfs_mx = 1.0f;
        return 1.0f - khfs_mx;   // any shadowed neighbor -> shadowed
    }
    return khfs_s / khfs_w;
}

// Raw depth at either clear extreme (standard far = 1, reversed = 0) can
// never discard - nothing legitimate needs occluding at the clear planes
// themselves.

// Pre-mesh scene COLOR capture (always single-sample: the capture path
// resolves MSAA) - the perceptual-composite blend source. Bound at t3 only on
// frames that need it; see the PSComposite packing note.
Texture2D<float4> sceneColorTex : register(t3);


