// g_cs_hlsl - HLSL source, spliced into rendering_integration.hpp as
// C++ raw string tokens via #include. Lines that close and immediately reopen
// the raw string are MSVC C2026 chunk boundaries (16380-byte string-token
// cap): SPLIT, never trim, when a segment approaches the cap. Any edit to
// segment bytes changes this unit's shader cache key (one cold recompile per
// user). Keep CRLF line endings. Never spell raw-string open/close tokens
// inside comments - the gate scripts scan for them textually.
R"HLSL(
#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float LoadDepth(int2 px) { return depthTex.Load(px, 0); }
#else
Texture2D<float> depthTex : register(t0);
float LoadDepth(int2 px) { return depthTex.Load(int3(px, 0)); }
#endif

struct QueryPoint  { float4 p; };   // xyz = engine-space world pos
struct QueryResult { float4 r; };   // [status, pointDist, sceneDist, rawDepth]
                                             // status: 1 visible, 0 occluded, -1
                                             // offscreen/behind

StructuredBuffer<QueryPoint>    points  : register(t1);
RWStructuredBuffer<QueryResult> results : register(u0);

cbuffer CSCB : register(b0)
{
    row_major float4x4 viewProj;
    float4 depthParams;   // x = proj m22, y = proj m32, z = viewport MinDepth, w = viewport MaxDepth
    float4 screenCount;   // x = width, y = height, z = point count, w = unused
    float4 pixelQuery;   // xy = pixel coords for CSSampleDepth
};

// Converts a raw depth-buffer value back to linear view-space distance (m).
// Inverts the viewport depth-range remap, then the projection: z' = m22 + m32
// / z => z = m32 / (z' - m22)
float SceneDistance(float rawDepth)
{
    float minD = depthParams.z;
    float maxD = depthParams.w;
    float ndcZ = (rawDepth - minD) / max(maxD - minD, 1e-6f);
    float denom = ndcZ - depthParams.x;
    // Depth-clear (sky) pixels lie beyond the far plane: denom >= 0 there.
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

[numthreads(64, 1, 1)]
void CSVisibility(uint3 id : SV_DispatchThreadID)
{
    uint count = (uint)screenCount.z;
    if (id.x >= count) return;

    float4 clip = mul(float4(points[id.x].p.xyz, 1.0f), viewProj);

    if (clip.w <= 0.01f) {
        results[id.x].r = float4(-1.0f, clip.w, 0.0f, 0.0f);
        return;
    }

    float3 ndc = clip.xyz / clip.w;

    if (abs(ndc.x) > 1.0f || abs(ndc.y) > 1.0f) {
        results[id.x].r = float4(-1.0f, clip.w, 0.0f, 0.0f);
        return;
    }

    int2 px;
    px.x = clamp((int)((0.5f + 0.5f * ndc.x) * screenCount.x), 0, (int)screenCount.x - 1);
    px.y = clamp((int)((0.5f - 0.5f * ndc.y) * screenCount.y), 0, (int)screenCount.y - 1);

    float rawDepth = LoadDepth(px);
    float sceneDist = SceneDistance(rawDepth);
    float pointDist = clip.w;   // view-space depth is linear distance along forward
    float tolerance = max(0.5f, sceneDist * 0.01f);
    float visible = (pointDist <= sceneDist + tolerance) ? 1.0f : 0.0f;
    results[id.x].r = float4(visible, pointDist, sceneDist, rawDepth);
}

[numthreads(1, 1, 1)]
void CSSampleDepth(uint3 id : SV_DispatchThreadID)
{
    int2 px = int2(pixelQuery.xy);
    px.x = clamp(px.x, 0, (int)screenCount.x - 1);
    px.y = clamp(px.y, 0, (int)screenCount.y - 1);
    float rawDepth = LoadDepth(px);
    results[0].r = float4(1.0f, 0.0f, SceneDistance(rawDepth), rawDepth);
}
)HLSL"
