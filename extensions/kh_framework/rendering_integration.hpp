namespace RenderIntegration {

enum class DepthMode : int {
    TestOnly = 0,   // default: occluded by scene, depth buffer untouched
    TestWrite = 1,  // participates in depth
    Off = 2,        // overlay
};

static constexpr UINT KH_MAX_QUERY_POINTS = 1024;

struct Resources {
    // --- Box drawing ---
    ID3D11VertexShader*      vs = nullptr;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11VertexShader*      vs_fullscreen = nullptr;   // SV_VertexID fullscreen triangle
    ID3D11PixelShader*       ps_effect = nullptr;       // uber effect shader (boxes + fullscreen)
    UINT                     ps_effect_samples = 0;     // depth MSAA count it was compiled for
    ID3D11InputLayout*       input_layout = nullptr;
    ID3D11Buffer*            vertex_buffer = nullptr;    // static unit cube
    ID3D11Buffer*            constant_buffer = nullptr;  // dynamic, per draw
    ID3D11DepthStencilState* dss_test = nullptr;         // LESS_EQUAL, no write
    ID3D11DepthStencilState* dss_test_write = nullptr;   // LESS_EQUAL, write
    ID3D11DepthStencilState* dss_off = nullptr;          // depth disabled
    ID3D11BlendState*        blend_modes[6] = {};       // normal, additive, multiply, screen, lighten, darken
    ID3D11RasterizerState*   rasterizer = nullptr;       // CullNone, MSAA, depth bias
    bool                     initialized = false;

    // --- Scene HDR color capture (resolved copy of the bound RTV) ---
    // --- Post-tonemap backbuffer capture (UI-affecting passes) ---
    ID3D11Texture2D*          bb_tex = nullptr;
    ID3D11ShaderResourceView* bb_srv = nullptr;
    UINT                      bb_w = 0, bb_h = 0;
    DXGI_FORMAT               bb_fmt = DXGI_FORMAT_UNKNOWN;

    // --- Fullscreen-chain ping-pong targets (single-sample, scene format).
    //     The chain runs on these instead of re-resolving the MSAA scene per
    //     pass: one resolve per frame regardless of pass count. ---
    ID3D11Texture2D*          chain_tex[2] = {};
    ID3D11RenderTargetView*   chain_rtv[2] = {};
    ID3D11ShaderResourceView* chain_srv[2] = {};
    ID3D11Texture2D*          scene_tex = nullptr;
    ID3D11ShaderResourceView* scene_srv = nullptr;
    UINT                      scene_w = 0, scene_h = 0;
    DXGI_FORMAT               scene_fmt = DXGI_FORMAT_UNKNOWN;

    // --- Engine depth buffer as shader resource ---
    ID3D11ShaderResourceView* depth_srv = nullptr;
    ID3D11DepthStencilView*   depth_dsv_ro = nullptr;   // read-only view: lets PS sample depth while depth-testing
    void*                     depth_res_identity = nullptr; // identity only, never dereferenced
    UINT                      depth_sample_count = 0;

    // --- Compute (depth queries / visibility) ---
    ID3D11ComputeShader*      cs_visibility = nullptr;
    ID3D11ComputeShader*      cs_sample_depth = nullptr;
    UINT                      cs_compiled_for_samples = 0; // recompile when MSAA count changes
    ID3D11Buffer*             cs_constant_buffer = nullptr;
    ID3D11Buffer*             points_buffer = nullptr;     // dynamic structured, CPU write
    ID3D11ShaderResourceView* points_srv = nullptr;
    ID3D11Buffer*             output_buffer = nullptr;     // default structured, UAV
    ID3D11UnorderedAccessView* output_uav = nullptr;
    ID3D11Buffer*             staging_buffer = nullptr;    // synchronous readback (immediate commands)
    ID3D11Buffer*             staging_async[2] = {};       // double-buffered async readback

    void release_bb_capture() {
        if (bb_tex) { bb_tex->Release(); bb_tex = nullptr; }
        if (bb_srv) { bb_srv->Release(); bb_srv = nullptr; }
        bb_w = 0; bb_h = 0; bb_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_fx_chain() {
        for (int i = 0; i < 2; ++i) {
            #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
            KH_SAFE_RELEASE(chain_tex[i]);
            KH_SAFE_RELEASE(chain_rtv[i]);
            KH_SAFE_RELEASE(chain_srv[i]);
            #undef KH_SAFE_RELEASE
        }
    }

    void release_scene_capture() {
        #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
        KH_SAFE_RELEASE(scene_tex);
        KH_SAFE_RELEASE(scene_srv);
        #undef KH_SAFE_RELEASE
        scene_w = 0; scene_h = 0; scene_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_depth_srv() {
        if (depth_srv) { depth_srv->Release(); depth_srv = nullptr; }
        if (depth_dsv_ro) { depth_dsv_ro->Release(); depth_dsv_ro = nullptr; }
        depth_res_identity = nullptr;
        depth_sample_count = 0;
    }

    void release_compute_shaders() {
        #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
        KH_SAFE_RELEASE(cs_visibility);
        KH_SAFE_RELEASE(cs_sample_depth);
        #undef KH_SAFE_RELEASE
        cs_compiled_for_samples = 0;
    }

    void release() {
        #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
        KH_SAFE_RELEASE(vs);
        KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(vs_fullscreen);
        KH_SAFE_RELEASE(ps_effect);
        ps_effect_samples = 0;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vertex_buffer);
        KH_SAFE_RELEASE(constant_buffer);
        KH_SAFE_RELEASE(dss_test);
        KH_SAFE_RELEASE(dss_test_write);
        KH_SAFE_RELEASE(dss_off);
        for (int i = 0; i < 6; ++i) KH_SAFE_RELEASE(blend_modes[i]);
        KH_SAFE_RELEASE(rasterizer);
        KH_SAFE_RELEASE(cs_constant_buffer);
        KH_SAFE_RELEASE(points_buffer);
        KH_SAFE_RELEASE(points_srv);
        KH_SAFE_RELEASE(output_buffer);
        KH_SAFE_RELEASE(output_uav);
        KH_SAFE_RELEASE(staging_buffer);
        KH_SAFE_RELEASE(staging_async[0]);
        KH_SAFE_RELEASE(staging_async[1]);
        #undef KH_SAFE_RELEASE
        release_bb_capture();
        release_fx_chain();
        release_scene_capture();
        release_depth_srv();
        release_compute_shaders();
        initialized = false;
    }
};

static Resources g_res;
static bool g_reset_hook_installed = false;

static void __stdcall on_engine_reset() {
    g_res.release();
}

struct RenderObject {
    int   effect = 0;           // 0 = solid color; >0 = screen-space effect (see EffectId)
    float fx[8] = {};           // effect parameters (effect-specific, see set_effect_params)
    bool  fullscreen = false;   // true = fullscreen triangle (post-processing pass), size unused
    bool  affect_ui = false;    // fullscreen passes: true = render post-tonemap over the composited frame (UI included) instead of the 3D scene phase
    bool  localized = false;    // fullscreen pass masked to a world-space sphere around pos
    float local_radius[3] = { 25.0f, 25.0f, 25.0f }; // full-strength radii per SQF axis [x, y, z] (m)
    int   local_shape = 0;      // 0 = sphere/ellipsoid, 1 = cube/box mask
    float local_falloff = 10.0f;// fade-to-zero band beyond the radius (m)
    bool  banded = false;       // effect confined to a camera-distance band
    float band_min = 0.0f;      // band start (m); fades in over band_falloff before it
    float band_max = 0.0f;      // band end (m); <= 0 = unbounded (includes sky)
    float band_falloff = 10.0f; // fade width at both band edges (m)
    float pos[3] = {};          // SQF coords [x, y, zASL]; converted at draw
    float size[3] = { 1.0f, 1.0f, 1.0f };  // box edge lengths per SQF axis [x, y, z] (m)
    int   blend_mode = 0;       // 0 normal, 1 additive, 2 multiply, 3 screen, 4 lighten, 5 darken
    float color[4] = { 1, 1, 1, 1 };
    DepthMode mode = DepthMode::TestOnly;
    bool  visible = true;
};

// Effect ids shared by boxes (localized, clipped to the box's screen footprint)
// and fullscreen passes. 10 and 11 sample the engine depth buffer per pixel.
enum class EffectId : int {
    Solid = 0, Invert = 1, ColorGrade = 2, Vignette = 3, Chromatic = 4,
    Grain = 5, Sharpen = 6, Blur = 7, Bloom = 8, Distortion = 9,
    Outline = 10, Pulse = 11, Halation = 12, Fog = 13,
    LensFlare = 14, Anamorphic = 15, SunFlare = 16,
};

static constexpr int KH_MAX_EFFECT = 16;
static std::unordered_map<uint32_t, RenderObject> g_draw_list;
static std::mutex g_draw_list_mutex;
static uint32_t g_next_handle = 1;

// ===========================================================================
// Async visibility query state. Pump runs inside the per-frame flush:
// frame N dispatches the query and copies results to a staging buffer,
// frame N+1 (or later) maps it without stalling. Results are therefore
// 1-2 frames old, which is imperceptible for marker styling and costs no
// CPU/GPU sync, unlike the synchronous gpuVisibility command.
// ===========================================================================

static std::vector<float> g_query_points_pending;  // SQF coords, xyz triplets
static bool     g_query_pending = false;
static UINT     g_async_write_idx = 0;
static UINT     g_async_inflight_count[2] = { 0, 0 };
static std::vector<float> g_vis_results_cpu;       // float4 per point
static UINT     g_vis_result_count = 0;
static uint64_t g_vis_result_frame = 0;
static uint64_t g_flush_frame = 0;

// ===========================================================================
// Render health statistics (cumulative; exposed via getRenderStats). Every
// early-exit of the flush is counted by reason - a frame the flush skips is
// a frame rendered WITHOUT our effects, i.e. visible flicker.
// ===========================================================================

struct RenderStats {
    uint64_t flushes = 0;            // flush attempts with work queued
    uint64_t gate_passed = 0;        // reached the draw path
    uint64_t lock_retries = 0;       // individual failed lock acquisitions
    uint64_t lock_failed_frames = 0; // frames dropped after all retries failed
    uint64_t skip_no_dsv = 0;        // no depth target bound (not a scene moment)
    uint64_t skip_wrong_pass = 0;    // a DSV was bound, but not the main scene's (PiP/mirror/UAV)
    uint64_t effect_setup_fails = 0; // capture/shader setup failed; effects demoted that frame
    uint64_t ui_flushes = 0;         // UI-phase flush attempts with work queued
    uint64_t ui_gate_passed = 0;     // UI-phase flushes that reached the draw path
    uint64_t ui_gate_skips = 0;      // UI-phase skips (scene pass active / wrong target)
};
static RenderStats g_stats;

// Main-scene identification: the main depth resource is stable across frames;
// PiP/mirror sub-passes bind a DIFFERENT depth resource. First adoption is
// the first DSV seen; a larger one replaces it (covers bootstrap ordering);
// a persistent mismatch streak re-adopts (covers resolution changes, where
// the old resource is destroyed and would otherwise be waited on forever).
static void* g_main_depth_identity = nullptr;
static UINT  g_main_depth_w = 0, g_main_depth_h = 0;
static UINT  g_wrong_pass_streak = 0;
static constexpr UINT KH_WRONG_PASS_READOPT = 120;

// Depth values were written through the SCENE viewport depth range; UI-phase
// shaders must linearize with the same range. Updated by the scene flush;
// initialized to the empirically probed defaults.
static float g_scene_vp_min_d = 0.011f;
static float g_scene_vp_max_d = 0.999f;

// ---------------------------------------------------------------------------
// Shaders convention: row_major, clip = p * viewProj
// ---------------------------------------------------------------------------

static const char* g_cb_hlsl = R"HLSL(
cbuffer CB : register(b0)
{
    row_major float4x4 viewProj;
    row_major float4x4 invViewProj;   // clip -> world (row-vector convention)
    float4 centerSize;   // xyz = world center (engine space), w = edge length
    float4 color;
    float4 fxParams0;    // effect parameters [0..3]
    float4 fxParams1;    // effect parameters [4..7]
    float4 fxMeta;       // x = effect id, y = time (s), z = screen width, w = screen height
    float4 depthParams;  // x = proj m22, y = proj m32, z = viewport MinDepth, w = MaxDepth
    float4 sizeAxes;     // xyz = box edge lengths (engine axes), w = blend mode id
    float4 localParams0; // xyz = mask center (engine space), w = shape (0 sphere, 1 cube)
    float4 localParams1; // x = falloff (normalized to mean radius), y = localized flag
    float4 localRadii;   // xyz = mask radii (engine axes)
    float4 bandParams;   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w = banded flag
};

struct VSIn  { float3 pos : POSITION; };
struct VSOut { float4 pos : SV_Position; };
)HLSL";

// Static entry points (no depth access): compiled once
static const char* g_hlsl_static = R"HLSL(
VSOut VSMain(VSIn i)
{
    VSOut o;
    float3 wp = centerSize.xyz + i.pos * sizeAxes.xyz;
    o.pos = mul(float4(wp, 1.0f), viewProj);
    return o;
}

// Fullscreen triangle from SV_VertexID: no vertex buffer, no input layout
VSOut VSFullscreen(uint vid : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    int bm = (int)sizeAxes.w;
    if (bm == 1 || bm == 3) return float4(color.rgb * color.a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), color.rgb, color.a), 1.0f);
    if (bm == 4 || bm == 5) return float4(color.rgb, 1.0f);
    return color;
}
)HLSL";

// Effect uber-shader: samples the captured scene (t0) and, for effects 10/11,
// the engine depth buffer (t1). Compiled per depth MSAA count.
static const char* g_hlsl_effect =
    R"HLSL(
Texture2D<float4> sceneColor : register(t0);

#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t1);
float LoadDepthPS(int2 px) { return depthTex.Load(px, 0); }
#else
Texture2D<float> depthTex : register(t1);
float LoadDepthPS(int2 px) { return depthTex.Load(int3(px, 0)); }
#endif

float3 SampleScene(int2 px)
{
    px = clamp(px, int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    return sceneColor.Load(int3(px, 0)).rgb;
}

float Luma(float3 c) { return dot(c, float3(0.299f, 0.587f, 0.114f)); }

float LinDepth(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    // At/beyond the far plane the denominator crosses zero and flips sign.
    // Sky pixels sit at the depth-CLEAR value (1.0), which is beyond the
    // viewport depth range entirely - treat everything past the far plane
    // as "very far" instead of returning a negative distance.
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

float Hash(float2 p) { return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f); }

// Per-pixel world position from the depth buffer + inverse view-projection
float3 WorldPos(int2 px, float2 uv)
{
    float raw = LoadDepthPS(px);
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float4 ndc = float4(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, ndcZ, 1.0f);
    float4 wp = mul(ndc, invViewProj);
    return wp.xyz / wp.w;
}

float4 PSEffect(VSOut i) : SV_Target
{
    int2  px = int2(i.pos.xy);
    float2 uv = i.pos.xy / float2(fxMeta.z, fxMeta.w);
    int   effect = (int)fxMeta.x;
    float t = fxMeta.y;
    float3 scene = SampleScene(px);
    float3 outc = scene;

    if (effect == 1)          // Invert: (1 - scene) * tint
    {
        outc = (1.0f - saturate(scene)) * color.rgb;
    }
    else if (effect == 2)     // ColorGrade: [saturation, contrast, brightness, gamma], tint = color.rgb
    {
        float3 c = scene * color.rgb * fxParams0.z;
        float l = Luma(c);
        c = lerp(l.xxx, c, fxParams0.x);
        c = (c - 0.5f) * fxParams0.y + 0.5f;
        c = pow(max(c, 0.0f), fxParams0.w);
        outc = c;
    }
    else if (effect == 3)     // Vignette: [startRadius, softness], color = edge color
    {
        float d = distance(uv, float2(0.5f, 0.5f)) * 1.4142f;
        float v = smoothstep(fxParams0.x, fxParams0.x + max(fxParams0.y, 1e-3f), d);
        outc = lerp(scene, color.rgb, v);
    }
    else if (effect == 4)     // Chromatic aberration: [strengthPx], radial
    {
        float2 off = (uv - 0.5f) * fxParams0.x;
        float r = SampleScene(int2(i.pos.xy + off)).r;
        float b = SampleScene(int2(i.pos.xy - off)).b;
        outc = float3(r, scene.g, b);
    }
    else if (effect == 5)     // Film grain: [amount, fps, grainSizePx, lumaResponse, chroma]
    {
        // Filmic grain: smooth value noise with spatial extent (not per-pixel
        // salt), triangular amplitude distribution, response peaking in the
        // mid-shadows and protecting highlights, optional chroma, and time
        // quantized to a frame rate for the discrete film-frame feel.
        float fps = max(fxParams0.y, 1.0f);
        float seed = floor(t * fps) * 61.7f;
        float2 gp = i.pos.xy / max(fxParams0.z, 1.0f);
        float2 ip = floor(gp);
        float2 fp = frac(gp);
        fp = fp * fp * (3.0f - 2.0f * fp);

        float n00 = Hash(ip + seed);
        float n10 = Hash(ip + float2(1, 0) + seed);
        float n01 = Hash(ip + float2(0, 1) + seed);
        float n11 = Hash(ip + float2(1, 1) + seed);
        float nv = lerp(lerp(n00, n10, fp.x), lerp(n01, n11, fp.x), fp.y);
        float nf = Hash(gp * 2.13f + seed + 17.0f);
        float g = (nv + nf) * 0.5f - 0.5f;   // triangular-ish, signed

        float3 gc = g.xxx;
        if (fxParams1.x > 0.001f)
        {
            float gr = (lerp(Hash(ip + seed + 31.0f), Hash(ip + float2(1, 1) + seed + 31.0f), fp.x) + Hash(gp * 1.71f + seed + 47.0f)) * 0.5f - 0.5f;
            float gb = (lerp(Hash(ip + seed + 73.0f), Hash(ip + float2(1, 1) + seed + 73.0f), fp.x) + Hash(gp * 2.71f + seed + 89.0f)) * 0.5f - 0.5f;
            gc = lerp(gc, float3(gr, g, gb), fxParams1.x);
        }

        float luma = saturate(Luma(scene));
        float resp = lerp(1.0f, 4.0f * luma * (1.0f - luma) * 0.9f + 0.1f, fxParams0.w);
        outc = scene + gc * fxParams0.x * resp;
    }
    else if (effect == 6)     // Sharpen: [strength]
    {
        float3 s = scene * (1.0f + 4.0f * fxParams0.x)
                 - fxParams0.x * (SampleScene(px + int2(1, 0)) + SampleScene(px - int2(1, 0))
                                + SampleScene(px + int2(0, 1)) + SampleScene(px - int2(0, 1)));
        outc = max(s, 0.0f);
    }
    else if (effect == 7)     // Gaussian-ish blur: [radiusPx]
    {
        int r = max((int)fxParams0.x, 1);
        float3 acc = scene * 0.25f;
        acc += (SampleScene(px + int2(r, 0)) + SampleScene(px - int2(r, 0))
              + SampleScene(px + int2(0, r)) + SampleScene(px - int2(0, r))) * 0.125f;
        acc += (SampleScene(px + int2(r, r)) + SampleScene(px - int2(r, r))
              + SampleScene(px + int2(r, -r)) + SampleScene(px + int2(-r, r))) * 0.0625f;
        outc = acc;
    }
    else if (effect == 8)     // Bloom: [threshold, intensity, radiusPx]
    {
        int r = max((int)fxParams0.z, 1);
        float3 acc = 0.0f;
        [unroll] for (int oy = -2; oy <= 2; ++oy)
        [unroll] for (int ox = -2; ox <= 2; ++ox)
            acc += max(SampleScene(px + int2(ox, oy) * r) - fxParams0.x, 0.0f);
        outc = scene + acc / 25.0f * fxParams0.y;
    }
    else if (effect == 9)     // Distortion: [amplitudePx, frequency, speed]
    {
        float2 off = float2(sin(uv.y * fxParams0.y * 6.2832f + t * fxParams0.z),
                            cos(uv.x * fxParams0.y * 6.2832f + t * fxParams0.z)) * fxParams0.x;
        outc = SampleScene(int2(i.pos.xy + off));
    }
)HLSL"
    R"HLSL(    else if (effect == 10)    // Outline: [depthEdgeScale, lumEdgeScale, sceneDarken, glowBoost], color = edge
    {
        float dC = LinDepth(LoadDepthPS(px));
        float dX = LinDepth(LoadDepthPS(px + int2(1, 0))) - dC;
        float dY = LinDepth(LoadDepthPS(px + int2(0, 1))) - dC;
        float depthEdge = saturate((abs(dX) + abs(dY)) / max(dC, 1.0f) * fxParams0.x);
        float lC = Luma(scene);
        float lumEdge = saturate((abs(Luma(SampleScene(px + int2(1, 0))) - lC)
                                + abs(Luma(SampleScene(px + int2(0, 1))) - lC)) * fxParams0.y);
        float edge = saturate(depthEdge + lumEdge);
        outc = scene * fxParams0.z + color.rgb * edge * fxParams0.w;
    }
    else if (effect == 11)    // Pulse: p0.xyz = center (engine space), p0.w = radius; p1 = [bandWidth, intensity]
    {
        float dist = distance(WorldPos(px, uv), fxParams0.xyz);
        float band = max(fxParams1.x, 0.01f);
        float ring = 1.0f - saturate(abs(dist - fxParams0.w) / band);
        ring *= ring;
        outc = scene + color.rgb * ring * fxParams1.y;
    }
    else if (effect == 12)    // Halation: [threshold, intensity, radiusPx], color = glow tint (warm)
    {
        int r = max((int)fxParams0.z, 1);
        const int2 dirs[8] = { int2(1,0), int2(-1,0), int2(0,1), int2(0,-1),
                               int2(1,1), int2(-1,1), int2(1,-1), int2(-1,-1) };
        float3 acc = 0.0f;
        [unroll] for (int k = 0; k < 8; ++k)
            acc += max(SampleScene(px + dirs[k] * r) - fxParams0.x, 0.0f) * 0.09f;
        [unroll] for (int k2 = 0; k2 < 8; ++k2)
            acc += max(SampleScene(px + dirs[k2] * r * 2) - fxParams0.x, 0.0f) * 0.035f;
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 13)    // Distance fog: [startDist m, endDist m, skyAmount 0..1], color = fog color
    {
        float d = LinDepth(LoadDepthPS(px));
        float f = saturate((d - fxParams0.x) / max(fxParams0.y - fxParams0.x, 1.0f));
        if (d > 1e8f) f = saturate(fxParams0.z);   // sky/far-plane pixels
        outc = lerp(scene, color.rgb, f);
    }
    else if (effect == 14)    // Lens flare, image-based: [threshold, intensity, ghostCount, ghostSpacing] + [haloRadius, haloIntensity, chromaPx]
    {
        // Bright pixels anywhere in the capture spawn a "ghost" train mirrored
        // through screen center, plus a halo ring - the sun, headlights and
        // explosions all flare automatically. The HDR capture makes the
        // threshold physically meaningful (sun >> white walls).
        float2 cuv = float2(0.5f, 0.5f);
        float2 ghostVec = (cuv - uv) * fxParams0.w;
        int nGhosts = clamp((int)fxParams0.z, 1, 8);
        float3 acc = 0.0f;
        for (int g = 1; g <= nGhosts; ++g)
        {
            float2 suv = uv + ghostVec * (float)g;
            float w = 1.0f - saturate(length(suv - cuv) * 1.6f);
            w = w * w;
            float2 spf = saturate(suv) * float2(fxMeta.z, fxMeta.w);
            float2 cdir = normalize(ghostVec + 1e-5f) * fxParams1.z;
            float3 s;
            s.r = max(SampleScene(int2(spf + cdir)).r - fxParams0.x, 0.0f);
            s.g = max(SampleScene(int2(spf)).g        - fxParams0.x, 0.0f);
            s.b = max(SampleScene(int2(spf - cdir)).b - fxParams0.x, 0.0f);
            acc += s * w;
        }
        float rC = length(uv - cuv);
        float hw = 1.0f - saturate(abs(rC - fxParams1.x) * 8.0f);
        float2 huv = uv + normalize(cuv - uv + 1e-5f) * fxParams1.x;
        acc += max(SampleScene(int2(saturate(huv) * float2(fxMeta.z, fxMeta.w))) - fxParams0.x, 0.0f) * hw * fxParams1.y;
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 15)    // Anamorphic streak: [threshold, intensity, lengthPx, falloffPow] + [vertical 0/1]; color = tint
    {
        float3 acc = 0.0f;
        float total = 0.0f;
        [unroll] for (int k = 1; k <= 16; ++k)
        {
            float t = (float)k / 16.0f;
            float w = pow(1.0f - t, max(fxParams0.w, 0.1f));
            int off = (int)(t * fxParams0.z);
            int2 d = (fxParams1.x > 0.5f) ? int2(0, off) : int2(off, 0);
            acc += (max(SampleScene(px + d) - fxParams0.x, 0.0f)
                  + max(SampleScene(px - d) - fxParams0.x, 0.0f)) * w;
            total += 2.0f * w;
        }
        acc /= max(total, 1.0f);
        outc = scene + acc * color.rgb * fxParams0.y;
    }
    else if (effect == 16)    // Sun flare, source-aware: p0.xyz = direction (engine space), p0.w = size; p1 = [ghostDots, ringIntensity, starburst]
    {
        // The direction projects as a point at infinity (w component 0); the
        // flare fades via per-pixel depth occlusion at the source: sky =
        // visible, geometry = blocked, partial coverage = smooth fade.
        float4 clip = mul(float4(fxParams0.xyz, 0.0f), viewProj);
        if (clip.w > 0.01f)
        {
            float2 sndc = clip.xy / clip.w;
            if (all(abs(sndc) < 1.3f))
            {
                float2 spos = float2(sndc.x * 0.5f + 0.5f, 0.5f - sndc.y * 0.5f);
                int2 sp = int2(saturate(spos) * float2(fxMeta.z, fxMeta.w));
                float vis = 0.0f;
                [unroll] for (int oy = -2; oy <= 2; ++oy)
                [unroll] for (int ox = -2; ox <= 2; ++ox)
                    vis += (LinDepth(LoadDepthPS(sp + int2(ox, oy) * 3)) > 1e8f) ? 1.0f : 0.0f;
                vis /= 25.0f;
                if (vis > 0.001f)
                {
                    float aspect = fxMeta.z / fxMeta.w;
                    float2 d = uv - spos;
                    d.x *= aspect;
                    float r = length(d) / max(fxParams0.w, 0.01f);
                    float ang = atan2(d.y, d.x);
                    float star = pow(abs(sin(ang * 6.0f)), 8.0f) * fxParams1.z;
                    float core = exp(-r * 18.0f) * 2.0f + exp(-r * 3.0f) * (0.35f + star * saturate(1.0f - r));
                    float2 axis = float2((0.5f - spos.x) * aspect, 0.5f - spos.y);
                    float glow = 0.0f;
                    [unroll] for (int g = 1; g <= 4; ++g)
                    {
                        if ((float)g <= fxParams1.x)
                        {
                            float2 gp = d - axis * (0.5f * (float)g);
                            glow += exp(-length(gp) * (30.0f + (float)g * 14.0f)) * (0.5f / (float)g);
                        }
                    }
                    glow += exp(-abs(r - 1.0f) * 9.0f) * fxParams1.y * 0.4f;
                    outc = scene + color.rgb * (core + glow) * vis;
                }
            }
        }
    }

    // World-space localization mask: full strength within radius, fading to
    // zero over the falloff band. Sky/far pixels resolve to distant world
    // positions and mask out naturally.
    if (localParams1.y > 0.5f)
    {
        float3 nd3 = abs(WorldPos(px, uv) - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        // normalized distance: 1.0 = the mask surface (ellipsoid or box)
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // cube (Chebyshev)
                 : length(nd3);                    // sphere/ellipsoid
        float mask = 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
        outc = lerp(scene, outc, mask);
    }

    // Camera-distance band mask: full strength for scene distances within
    // [min, max], fading over 'falloff' meters at both edges. max <= 0 means
    // unbounded far (sky included). Combines multiplicatively with the
    // world-space localization mask above.
    if (bandParams.w > 0.5f)
    {
        float d = LinDepth(LoadDepthPS(px));
        float fall = max(bandParams.z, 0.01f);
        float mask = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            mask *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        outc = lerp(scene, outc, mask);
    }

    // Chained-fullscreen composite packing: for chain passes (centerSize.w =
    // 1) the destination IS the scene texture we already sampled, so every
    // blend mode is computed exactly in-shader and written opaquely - this is
    // what allows the chain to run on single-sample ping-pong targets with
    // ONE MSAA resolve per frame instead of one per pass.
    int bm = (int)sizeAxes.w;
    if (centerSize.w > 0.5f)
    {
        float a = color.a;
        float3 mixed = lerp(scene, outc, a);
        float3 comp;
        if (bm == 1)      comp = scene + outc * a;                                   // additive
        else if (bm == 2) comp = scene * lerp(float3(1.0f, 1.0f, 1.0f), outc, a);    // multiply
        else if (bm == 3) comp = scene + outc * a - scene * outc * a;                // screen
        else if (bm == 4) comp = max(scene, mixed);                                  // lighten
        else if (bm == 5) comp = min(scene, mixed);                                  // darken
        else              comp = mixed;                                              // normal
        return float4(comp, 1.0f);
    }

    // Blend-mode output packing (boxes: hardware blend against the live
    // framebuffer; intensity pre-applied where blend factors cannot express it)
    if (bm == 1 || bm == 3) return float4(outc * color.a, 1.0f);              // additive, screen
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), outc, color.a), 1.0f); // multiply
    if (bm == 4 || bm == 5) return float4(lerp(scene, outc, color.a), 1.0f);  // lighten, darken (MAX/MIN op)
    return float4(outc, color.a);                                             // normal (alpha lerp)
}
)HLSL";

// Compute kernels. Compiled twice: with MSAA_DEPTH=1 (Texture2DMS) or =0
// (Texture2D), selected by the engine depth buffer's current sample count.
static const char* g_cs_hlsl = R"HLSL(
#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float LoadDepth(int2 px) { return depthTex.Load(px, 0); }
#else
Texture2D<float> depthTex : register(t0);
float LoadDepth(int2 px) { return depthTex.Load(int3(px, 0)); }
#endif

struct QueryPoint  { float4 p; };            // xyz = engine-space world pos
struct QueryResult { float4 r; };            // [status, pointDist, sceneDist, rawDepth]
                                             // status: 1 visible, 0 occluded, -1 offscreen/behind

StructuredBuffer<QueryPoint>    points  : register(t1);
RWStructuredBuffer<QueryResult> results : register(u0);

cbuffer CSCB : register(b0)
{
    row_major float4x4 viewProj;
    float4 depthParams;    // x = proj m22, y = proj m32, z = viewport MinDepth, w = viewport MaxDepth
    float4 screenCount;    // x = width, y = height, z = point count, w = unused
    float4 pixelQuery;     // xy = pixel coords for CSSampleDepth
};

// Converts a raw depth-buffer value back to linear view-space distance (m).
// Inverts the viewport depth-range remap, then the projection:
//   z' = m22 + m32 / z   =>   z = m32 / (z' - m22)
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
)HLSL";

struct alignas(16) ConstantData {
    float view_proj[4][4];
    float inv_view_proj[4][4];
    float center_size[4];
    float color[4];
    float fx0[4];
    float fx1[4];
    float fx_meta[4];      // effect id, time (s), screen width, screen height
    float depth_params[4]; // proj m22, proj m32, viewport MinDepth, MaxDepth
    float size_axes[4];    // xyz = box edge lengths (engine axes), w = blend mode id
    float local0[4];       // xyz = mask center (engine space), w = shape (0 sphere, 1 cube)
    float local1[4];       // x = falloff (normalized to mean radius), y = localized flag
    float local_radii[4];  // xyz = mask radii (engine axes)
    float band0[4];        // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w = banded flag
};

struct alignas(16) CSConstantData {
    float view_proj[4][4];
    float depth_params[4];   // m22, m32, viewport MinDepth, viewport MaxDepth
    float screen_count[4];   // width, height, count, unused
    float pixel_query[4];    // x, y, unused, unused
};

// Unit cube, half-extent 0.5, 12 triangles / 36 vertices
static const float g_cube_verts[36][3] = {
    {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
    {-0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
    { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
    { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
    {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
    {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
};

inline std::string hr_str(HRESULT hr) {
    char b[16];
    sprintf_s(b, "0x%08X", static_cast<unsigned>(hr));
    return std::string(b);
}

inline std::string compile_shader(const char* src, const char* entry, const char* target,
                                  const D3D_SHADER_MACRO* defines, ID3DBlob** out_blob) {
    ID3DBlob* err_blob = nullptr;

    HRESULT hr = D3DCompile(src, strlen(src), "kh_render", defines, nullptr,
                            entry, target, 0, 0, out_blob, &err_blob);

    if (FAILED(hr)) {
        std::string msg = std::string(entry) + " compile failed " + hr_str(hr);

        if (err_blob) {
            msg += ": ";
            msg += static_cast<const char*>(err_blob->GetBufferPointer());
            err_blob->Release();
        }

        return msg;
    }

    if (err_blob) err_blob->Release();
    return "";
}

inline std::string ensure_resources(ID3D11Device* dev) {
    if (g_res.initialized) return "";

    if (!g_reset_hook_installed && RVExtBridge::has_reset_hook()) {
        RVExtBridge::set_reset_hook(&on_engine_reset);
        g_reset_hook_installed = true;
    }

    HRESULT hr;
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* vs_fs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;

    // Shared cbuffer declaration is prepended to every compilation unit
    const std::string static_src = std::string(g_cb_hlsl) + g_hlsl_static;
    std::string err = compile_shader(static_src.c_str(), "VSMain", "vs_5_0", nullptr, &vs_blob);
    if (!err.empty()) return err;
    err = compile_shader(static_src.c_str(), "VSFullscreen", "vs_5_0", nullptr, &vs_fs_blob);
    if (!err.empty()) { vs_blob->Release(); return err; }
    err = compile_shader(static_src.c_str(), "PSMain", "ps_5_0", nullptr, &ps_blob);
    if (!err.empty()) { vs_blob->Release(); vs_fs_blob->Release(); return err; }
    hr = dev->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_res.vs);
    if (FAILED(hr)) { vs_blob->Release(); vs_fs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader " + hr_str(hr); }
    hr = dev->CreateVertexShader(vs_fs_blob->GetBufferPointer(), vs_fs_blob->GetBufferSize(), nullptr, &g_res.vs_fullscreen);
    vs_fs_blob->Release();
    if (FAILED(hr)) { vs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader(fullscreen) " + hr_str(hr); }
    hr = dev->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_res.ps);
    ps_blob->Release();
    if (FAILED(hr)) { vs_blob->Release(); g_res.release(); return "CreatePixelShader " + hr_str(hr); }

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = dev->CreateInputLayout(layout, 1, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &g_res.input_layout);
    vs_blob->Release();
    if (FAILED(hr)) { g_res.release(); return "CreateInputLayout " + hr_str(hr); }

    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(g_cube_verts);
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init = { g_cube_verts, 0, 0 };
        hr = dev->CreateBuffer(&bd, &init, &g_res.vertex_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create VB " + hr_str(hr); }
    }

    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(ConstantData);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.constant_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create CB " + hr_str(hr); }
    }

    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(CSConstantData);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.cs_constant_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create CS CB " + hr_str(hr); }
    }

    // Points input: dynamic structured buffer, CPU-writable, SRV
    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(float) * 4 * KH_MAX_QUERY_POINTS;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(float) * 4;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.points_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create points buffer " + hr_str(hr); }
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = DXGI_FORMAT_UNKNOWN;
        sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        sd.Buffer.NumElements = KH_MAX_QUERY_POINTS;
        hr = dev->CreateShaderResourceView(g_res.points_buffer, &sd, &g_res.points_srv);
        if (FAILED(hr)) { g_res.release(); return "Create points SRV " + hr_str(hr); }
    }

    // Results output: default structured buffer with UAV + staging twin
    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(float) * 4 * KH_MAX_QUERY_POINTS;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(float) * 4;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.output_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create output buffer " + hr_str(hr); }
        D3D11_UNORDERED_ACCESS_VIEW_DESC ud = {};
        ud.Format = DXGI_FORMAT_UNKNOWN;
        ud.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        ud.Buffer.NumElements = KH_MAX_QUERY_POINTS;
        hr = dev->CreateUnorderedAccessView(g_res.output_buffer, &ud, &g_res.output_uav);
        if (FAILED(hr)) { g_res.release(); return "Create output UAV " + hr_str(hr); }
        bd.Usage = D3D11_USAGE_STAGING;
        bd.BindFlags = 0;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.staging_buffer);
        if (FAILED(hr)) { g_res.release(); return "Create staging buffer " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.staging_async[0]);
        if (FAILED(hr)) { g_res.release(); return "Create staging async 0 " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.staging_async[1]);
        if (FAILED(hr)) { g_res.release(); return "Create staging async 1 " + hr_str(hr); }
    }

    {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = TRUE;
        dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;  // conventional Z (probed)
        dd.StencilEnable = FALSE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_test);
        if (FAILED(hr)) { g_res.release(); return "Create DSS(test) " + hr_str(hr); }
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_test_write);
        if (FAILED(hr)) { g_res.release(); return "Create DSS(write) " + hr_str(hr); }
        dd.DepthEnable = FALSE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_off);
        if (FAILED(hr)) { g_res.release(); return "Create DSS(off) " + hr_str(hr); }
    }

    {
        // One blend state per mixing mode. MIN/MAX ignore the blend factors.
        struct BlendSpec { D3D11_BLEND src, dst; D3D11_BLEND_OP op; };
        
        const BlendSpec specs[6] = {
            { D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD }, // 0 normal
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_ADD }, // 1 additive
            { D3D11_BLEND_ZERO,      D3D11_BLEND_SRC_COLOR,     D3D11_BLEND_OP_ADD }, // 2 multiply
            { D3D11_BLEND_ONE,       D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_OP_ADD }, // 3 screen
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_MAX }, // 4 lighten
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_MIN }, // 5 darken
        };

        for (int i = 0; i < 6; ++i) {
            D3D11_BLEND_DESC bd = {};
            bd.RenderTarget[0].BlendEnable = TRUE;
            bd.RenderTarget[0].SrcBlend = specs[i].src;
            bd.RenderTarget[0].DestBlend = specs[i].dst;
            bd.RenderTarget[0].BlendOp = specs[i].op;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            hr = dev->CreateBlendState(&bd, &g_res.blend_modes[i]);
            if (FAILED(hr)) { g_res.release(); return "Create blend " + hr_str(hr); }
        }
    }

    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;      // visible from inside too
        rd.DepthClipEnable = TRUE;
        rd.MultisampleEnable = TRUE;        // scene targets are MSAA per video settings
        // Small negative bias pulls our fragments toward the camera so they
        // consistently win the depth test in the ambiguous band where our
        // geometry intersects scene geometry. Suppresses z-shimmer caused by
        // the sim-thread PV matrix being one frame ahead of the depth buffer
        // (per-frame dynamic near plane). Tune if intersections still crawl:
        // more negative = more suppression, but the cut line sinks further
        // into scene geometry.
        rd.DepthBias = -32;                 // units of 1/2^24 for D24 buffers
        rd.SlopeScaledDepthBias = -1.0f;
        rd.DepthBiasClamp = 0.0f;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer);
        if (FAILED(hr)) { g_res.release(); return "Create rasterizer " + hr_str(hr); }
    }

    g_res.initialized = true;
    return "";
}

// ---------------------------------------------------------------------------
// Scene HDR color capture.
// The bound RTV cannot be sampled while bound, so the scene is resolved
// (MSAA) or copied (non-MSAA) into an owned single-sample texture + SRV.
// Called once per flush when any scene-read object exists; if scene-reading
// common, hoist to one capture per frame.
// ---------------------------------------------------------------------------

// Creates (or recreates on size/format change) the two single-sample chain
// targets. Called after ensure_scene_capture, which establishes dimensions.
inline std::string ensure_fx_chain(ID3D11Device* dev) {
    if (!g_res.scene_tex) return "no scene capture";

    D3D11_TEXTURE2D_DESC sd = {};
    g_res.scene_tex->GetDesc(&sd);

    if (g_res.chain_tex[0]) {
        D3D11_TEXTURE2D_DESC cd = {};
        g_res.chain_tex[0]->GetDesc(&cd);
        if (cd.Width == sd.Width && cd.Height == sd.Height && cd.Format == sd.Format) return "";
        g_res.release_fx_chain();
    }

    for (int i = 0; i < 2; ++i) {
        D3D11_TEXTURE2D_DESC td = sd;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.SampleDesc.Count = 1;
        td.SampleDesc.Quality = 0;
        HRESULT hr = dev->CreateTexture2D(&td, nullptr, &g_res.chain_tex[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "Create chain tex " + hr_str(hr); }
        hr = dev->CreateRenderTargetView(g_res.chain_tex[i], nullptr, &g_res.chain_rtv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "Create chain RTV " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.chain_tex[i], nullptr, &g_res.chain_srv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "Create chain SRV " + hr_str(hr); }
    }

    return "";
}

inline std::string ensure_scene_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return "no RTV bound";
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv->GetDesc(&rtv_desc);
    rtv->Release();
    if (!res) return "RTV has no resource";
    ID3D11Texture2D* src_tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src_tex));
    res->Release();
    if (FAILED(hr) || !src_tex) return "RTV resource is not a Texture2D";
    D3D11_TEXTURE2D_DESC src_desc = {};
    src_tex->GetDesc(&src_desc);

    // Resolve target format must be non-typeless; the RTV view format is.
    DXGI_FORMAT capture_fmt = rtv_desc.Format;

    // (Re)create the capture texture on size/format change
    if (!g_res.scene_tex || g_res.scene_w != src_desc.Width ||
        g_res.scene_h != src_desc.Height || g_res.scene_fmt != capture_fmt) {
        g_res.release_scene_capture();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = src_desc.Width;
        td.Height = src_desc.Height;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = capture_fmt;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        hr = dev->CreateTexture2D(&td, nullptr, &g_res.scene_tex);
        if (FAILED(hr)) { src_tex->Release(); return "Create scene capture tex " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.scene_tex, nullptr, &g_res.scene_srv);
        if (FAILED(hr)) { g_res.release_scene_capture(); src_tex->Release(); return "Create scene capture SRV " + hr_str(hr); }
        g_res.scene_w = src_desc.Width;
        g_res.scene_h = src_desc.Height;
        g_res.scene_fmt = capture_fmt;
    }

    if (src_desc.SampleDesc.Count > 1) {
        ctx->ResolveSubresource(g_res.scene_tex, 0, src_tex, 0, capture_fmt);
    } else {
        ctx->CopyResource(g_res.scene_tex, src_tex);
    }

    src_tex->Release();
    return "";
}

// ---------------------------------------------------------------------------
// Engine depth buffer as shader resource.
// The scene depth resource is created with BIND_SHADER_RESOURCE (probed), so
// an SRV can be created directly over it - no copy. The SRV is cached and
// recreated only when the underlying resource changes (resolution/AA change).
// D3D11 forbids the resource being simultaneously bound as a writable DSV and
// an SRV, so compute dispatches unbind the OM targets around the dispatch.
// ---------------------------------------------------------------------------

inline std::string ensure_depth_srv(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                    UINT* out_width, UINT* out_height) {
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    if (!dsv) return "no DSV bound";
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    dsv->Release();
    if (!res) return "DSV has no resource";
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (FAILED(hr) || !tex) return "DSV resource is not a Texture2D";
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    if (out_width)  *out_width = td.Width;
    if (out_height) *out_height = td.Height;

    if (g_res.depth_srv && g_res.depth_res_identity == static_cast<void*>(tex)) {
        tex->Release();
        return "";
    }

    g_res.release_depth_srv();

    if (!(td.BindFlags & D3D11_BIND_SHADER_RESOURCE)) {
        tex->Release();
        return "depth resource lacks BIND_SHADER_RESOURCE";
    }

    DXGI_FORMAT srv_fmt;

    switch (td.Format) {
        case DXGI_FORMAT_R24G8_TYPELESS:    srv_fmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case DXGI_FORMAT_R32_TYPELESS:      srv_fmt = DXGI_FORMAT_R32_FLOAT; break;
        case DXGI_FORMAT_R32G8X24_TYPELESS: srv_fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
        case DXGI_FORMAT_R16_TYPELESS:      srv_fmt = DXGI_FORMAT_R16_UNORM; break;
        default:
            tex->Release();
            return "unsupported depth format " + std::to_string(static_cast<int>(td.Format));
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = srv_fmt;

    if (td.SampleDesc.Count > 1) {
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
    } else {
        sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        sd.Texture2D.MipLevels = 1;
    }

    hr = dev->CreateShaderResourceView(tex, &sd, &g_res.depth_srv);

    if (FAILED(hr)) {
        tex->Release();
        return "Create depth SRV " + hr_str(hr);
    }

    // Read-only DSV over the same resource: D3D11 permits the depth SRV to be
    // sampled in a pixel shader while a READ_ONLY DSV is bound, so depth
    // testing keeps working during depth-sampling effects (writes disabled).
    {
        DXGI_FORMAT dsv_fmt = DXGI_FORMAT_UNKNOWN;
        UINT ro_flags = D3D11_DSV_READ_ONLY_DEPTH;

        switch (td.Format) {
            case DXGI_FORMAT_R24G8_TYPELESS:    dsv_fmt = DXGI_FORMAT_D24_UNORM_S8_UINT; ro_flags |= D3D11_DSV_READ_ONLY_STENCIL; break;
            case DXGI_FORMAT_R32_TYPELESS:      dsv_fmt = DXGI_FORMAT_D32_FLOAT; break;
            case DXGI_FORMAT_R32G8X24_TYPELESS: dsv_fmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; ro_flags |= D3D11_DSV_READ_ONLY_STENCIL; break;
            case DXGI_FORMAT_R16_TYPELESS:      dsv_fmt = DXGI_FORMAT_D16_UNORM; break;
            default: break;
        }

        if (dsv_fmt != DXGI_FORMAT_UNKNOWN) {
            D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
            dd.Format = dsv_fmt;
            dd.Flags = ro_flags;

            dd.ViewDimension = td.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                       : D3D11_DSV_DIMENSION_TEXTURE2D;

            if (FAILED(dev->CreateDepthStencilView(tex, &dd, &g_res.depth_dsv_ro))) {
                g_res.depth_dsv_ro = nullptr;   // depth-sampling effects will be skipped
            }
        }
    }

    g_res.depth_res_identity = static_cast<void*>(tex);
    g_res.depth_sample_count = td.SampleDesc.Count;
    tex->Release();  // the SRV holds its own reference
    return "";
}

// The effect pixel shader samples the engine depth buffer, whose MSAA count
// follows video settings - compiled (and recompiled on change) per count.
inline std::string ensure_effect_shader(ID3D11Device* dev) {
    if (g_res.ps_effect && g_res.ps_effect_samples == g_res.depth_sample_count) return "";
    if (g_res.ps_effect) { g_res.ps_effect->Release(); g_res.ps_effect = nullptr; }

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { nullptr, nullptr }
    };

    const std::string fx_src = std::string(g_cb_hlsl) + g_hlsl_effect;
    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(fx_src.c_str(), "PSEffect", "ps_5_0", defines, &blob);
    if (!err.empty()) return err;
    HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_effect);
    blob->Release();
    if (FAILED(hr)) return "CreatePixelShader(effect) " + hr_str(hr);
    g_res.ps_effect_samples = g_res.depth_sample_count;
    return "";
}

inline std::string ensure_compute_shaders(ID3D11Device* dev) {
    if (g_res.cs_visibility && g_res.cs_compiled_for_samples == g_res.depth_sample_count) return "";
    g_res.release_compute_shaders();

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { nullptr, nullptr }
    };

    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(g_cs_hlsl, "CSVisibility", "cs_5_0", defines, &blob);
    if (!err.empty()) return err;
    HRESULT hr = dev->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.cs_visibility);
    blob->Release();
    if (FAILED(hr)) return "CreateComputeShader(visibility) " + hr_str(hr);
    err = compile_shader(g_cs_hlsl, "CSSampleDepth", "cs_5_0", defines, &blob);
    if (!err.empty()) { g_res.release_compute_shaders(); return err; }
    hr = dev->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.cs_sample_depth);
    blob->Release();
    if (FAILED(hr)) { g_res.release_compute_shaders(); return "CreateComputeShader(sampleDepth) " + hr_str(hr); }
    g_res.cs_compiled_for_samples = g_res.depth_sample_count;
    return "";
}

// ---------------------------------------------------------------------------
// Pipeline state snapshot / restore (only what we touch).
// Constant buffers are per-stage: b0 of BOTH VS and PS is set and restored.
// ---------------------------------------------------------------------------

struct StateBackup {
    ID3D11InputLayout*       input_layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer*            vb = nullptr;
    UINT                     vb_stride = 0, vb_offset = 0;
    ID3D11VertexShader*      vs = nullptr;
    ID3D11ClassInstance*     vs_insts[8] = {};
    UINT                     vs_inst_count = 8;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11ClassInstance*     ps_insts[8] = {};
    UINT                     ps_inst_count = 8;
    ID3D11GeometryShader*    gs = nullptr;
    ID3D11HullShader*        hs = nullptr;
    ID3D11DomainShader*      ds = nullptr;
    ID3D11Buffer*            vs_cb0 = nullptr;
    ID3D11Buffer*            ps_cb0 = nullptr;
    ID3D11ShaderResourceView* ps_srvs[2] = {};
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     stencil_ref = 0;
    ID3D11BlendState*        blend = nullptr;
    FLOAT                    blend_factor[4] = {};
    UINT                     sample_mask = 0xFFFFFFFF;
    ID3D11RasterizerState*   rasterizer = nullptr;

    void capture(ID3D11DeviceContext* ctx) {
        ctx->IAGetInputLayout(&input_layout);
        ctx->IAGetPrimitiveTopology(&topology);
        ctx->IAGetVertexBuffers(0, 1, &vb, &vb_stride, &vb_offset);
        ctx->VSGetShader(&vs, vs_insts, &vs_inst_count);
        ctx->PSGetShader(&ps, ps_insts, &ps_inst_count);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        ctx->VSGetConstantBuffers(0, 1, &vs_cb0);
        ctx->PSGetConstantBuffers(0, 1, &ps_cb0);
        ctx->PSGetShaderResources(0, 2, ps_srvs);
        ctx->OMGetDepthStencilState(&dss, &stencil_ref);
        ctx->OMGetBlendState(&blend, blend_factor, &sample_mask);
        ctx->RSGetState(&rasterizer);
    }

    void restore(ID3D11DeviceContext* ctx) {
        ctx->IASetInputLayout(input_layout);
        ctx->IASetPrimitiveTopology(topology);
        ctx->IASetVertexBuffers(0, 1, &vb, &vb_stride, &vb_offset);
        ctx->VSSetShader(vs, vs_inst_count ? vs_insts : nullptr, vs_inst_count);
        ctx->PSSetShader(ps, ps_inst_count ? ps_insts : nullptr, ps_inst_count);
        ctx->GSSetShader(gs, nullptr, 0);
        ctx->HSSetShader(hs, nullptr, 0);
        ctx->DSSetShader(ds, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, &vs_cb0);
        ctx->PSSetConstantBuffers(0, 1, &ps_cb0);
        ctx->PSSetShaderResources(0, 2, ps_srvs);
        ctx->OMSetDepthStencilState(dss, stencil_ref);
        ctx->OMSetBlendState(blend, blend_factor, sample_mask);
        ctx->RSSetState(rasterizer);
        #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vb);
        KH_SAFE_RELEASE(vs);
        for (UINT i = 0; i < vs_inst_count; ++i) KH_SAFE_RELEASE(vs_insts[i]);
        KH_SAFE_RELEASE(ps);
        for (UINT i = 0; i < ps_inst_count; ++i) KH_SAFE_RELEASE(ps_insts[i]);
        KH_SAFE_RELEASE(gs);
        KH_SAFE_RELEASE(hs);
        KH_SAFE_RELEASE(ds);
        KH_SAFE_RELEASE(vs_cb0);
        KH_SAFE_RELEASE(ps_cb0);
        KH_SAFE_RELEASE(ps_srvs[0]);
        KH_SAFE_RELEASE(ps_srvs[1]);
        KH_SAFE_RELEASE(dss);
        KH_SAFE_RELEASE(blend);
        KH_SAFE_RELEASE(rasterizer);
        #undef KH_SAFE_RELEASE
    }
};

// Compute-stage state snapshot (only what compute dispatches touch)
struct ComputeStateBackup {
    ID3D11ComputeShader*       cs = nullptr;
    ID3D11ShaderResourceView*  srvs[2] = {};
    ID3D11UnorderedAccessView* uav = nullptr;
    ID3D11Buffer*              cb0 = nullptr;
    ID3D11RenderTargetView*    rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView*    dsv = nullptr;

    void capture(ID3D11DeviceContext* ctx) {
        ctx->CSGetShader(&cs, nullptr, nullptr);
        ctx->CSGetShaderResources(0, 2, srvs);
        ctx->CSGetUnorderedAccessViews(0, 1, &uav);
        ctx->CSGetConstantBuffers(0, 1, &cb0);
        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, &dsv);
    }

    void restore(ID3D11DeviceContext* ctx) {
        ctx->CSSetShader(cs, nullptr, 0);
        ctx->CSSetShaderResources(0, 2, srvs);
        UINT counts[1] = { static_cast<UINT>(-1) };
        ctx->CSSetUnorderedAccessViews(0, 1, &uav, counts);
        ctx->CSSetConstantBuffers(0, 1, &cb0);
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtvs, dsv);
        #define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }
        KH_SAFE_RELEASE(cs);
        KH_SAFE_RELEASE(srvs[0]);
        KH_SAFE_RELEASE(srvs[1]);
        KH_SAFE_RELEASE(uav);
        KH_SAFE_RELEASE(cb0);
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) KH_SAFE_RELEASE(rtvs[i]);
        KH_SAFE_RELEASE(dsv);
        #undef KH_SAFE_RELEASE
    }
};

inline void mul_4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r][c] = a[r][0]*b[0][c] + a[r][1]*b[1][c] + a[r][2]*b[2][c] + a[r][3]*b[3][c];
}

// ---------------------------------------------------------------------------
// Compute dispatches on the engine depth buffer.
//
// kernel selects the operation: Visibility tests 'count' points already
// uploaded into points_buffer against scene depth; SampleDepth reads one
// pixel's depth.
//
// Results land in out_results (float4 per element):
//   [status, pointDist(m), sceneDist(m), rawDepth]
//
// The OM targets are unbound around the dispatch: the depth resource cannot
// legally be an SRV while its DSV is bound for writing. The readback map is
// a synchronous GPU flush - acceptable for per-frame query batches; move to
// double-buffered async readback if this becomes per-frame hot at scale.
// ---------------------------------------------------------------------------

enum class ComputeKernel : int { Visibility = 0, SampleDepth = 1 };

inline std::string run_depth_compute(ComputeKernel kernel, UINT count,
                                     float pixel_x, float pixel_y,
                                     float* out_results, UINT out_capacity) {
    if (!RVExtBridge::is_initialized()) return "RVExtBridge not initialized";
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return "device/context null";
    RVExtBridge::ScopedGraphicsLock lock;
    if (!lock.acquired()) return "SKIP: graphics lock not acquired";
    std::string err = ensure_resources(dev);
    if (!err.empty()) return err;
    UINT depth_w = 0, depth_h = 0;
    err = ensure_depth_srv(dev, ctx, &depth_w, &depth_h);
    if (!err.empty()) return "depth SRV: " + err;
    err = ensure_compute_shaders(dev);
    if (!err.empty()) return "compute: " + err;
    RVExtBridge::ProjectionViewTransform pv = {};

    if (!RVExtBridge::get_projection_view_transform(pv)) {
        return "get_projection_view_transform failed";
    }

    // Viewport depth range for the depth-value remap
    float min_d = 0.0f, max_d = 1.0f;

    {
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n_vp, &vp);
        if (n_vp >= 1) { min_d = vp.MinDepth; max_d = vp.MaxDepth; }
    }

    CSConstantData cbd = {};
    mul_4x4(pv.view, pv.projection, cbd.view_proj);
    cbd.depth_params[0] = pv.projection[2][2];
    cbd.depth_params[1] = pv.projection[3][2];
    cbd.depth_params[2] = min_d;
    cbd.depth_params[3] = max_d;
    cbd.screen_count[0] = static_cast<float>(depth_w);
    cbd.screen_count[1] = static_cast<float>(depth_h);
    cbd.screen_count[2] = static_cast<float>(count);
    cbd.pixel_query[0] = pixel_x;
    cbd.pixel_query[1] = pixel_y;

    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        HRESULT hr = ctx->Map(g_res.cs_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) return "CS CB map " + hr_str(hr);
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.cs_constant_buffer, 0);
    }

    ComputeStateBackup backup;
    backup.capture(ctx);

    // Unbind OM so the depth resource may legally be bound as an SRV
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* srvs[2] = { g_res.depth_srv, g_res.points_srv };
    ctx->CSSetShaderResources(0, 2, srvs);
    UINT counts[1] = { 0 };
    ctx->CSSetUnorderedAccessViews(0, 1, &g_res.output_uav, counts);
    ctx->CSSetConstantBuffers(0, 1, &g_res.cs_constant_buffer);

    UINT result_count;
    if (kernel == ComputeKernel::Visibility) {
        ctx->CSSetShader(g_res.cs_visibility, nullptr, 0);
        ctx->Dispatch((count + 63) / 64, 1, 1);
        result_count = count;
    } else {
        ctx->CSSetShader(g_res.cs_sample_depth, nullptr, 0);
        ctx->Dispatch(1, 1, 1);
        result_count = 1;
    }

    // Unbind our SRVs/UAV before restoring OM (avoids hazard warnings when
    // the depth SRV and re-bound DSV briefly coexist)
    ID3D11ShaderResourceView* null_srvs[2] = { nullptr, nullptr };
    ctx->CSSetShaderResources(0, 2, null_srvs);
    ID3D11UnorderedAccessView* null_uav = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &null_uav, counts);
    backup.restore(ctx);

    // Readback
    D3D11_BOX box = { 0, 0, 0, result_count * sizeof(float) * 4, 1, 1 };
    ctx->CopySubresourceRegion(g_res.staging_buffer, 0, 0, 0, 0, g_res.output_buffer, 0, &box);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = ctx->Map(g_res.staging_buffer, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return "staging map " + hr_str(hr);
    UINT to_copy = result_count * 4;
    if (to_copy > out_capacity) to_copy = out_capacity;
    memcpy(out_results, mapped.pData, to_copy * sizeof(float));
    ctx->Unmap(g_res.staging_buffer, 0);
    return "OK";
}

// Uploads SQF points (converted to engine space) into the points buffer.
// Takes its own lock; the subsequent dispatch in run_depth_compute happens on
// the same thread, so the buffer contents cannot be raced in between.
inline std::string upload_query_points(const float* xyz_sqf, UINT count) {
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev || !ctx) return "device/context null";
    RVExtBridge::ScopedGraphicsLock lock;
    if (!lock.acquired()) return "SKIP: graphics lock not acquired";
    std::string err = ensure_resources(dev);
    if (!err.empty()) return err;
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = ctx->Map(g_res.points_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr)) return "points map " + hr_str(hr);
    float* dst = static_cast<float*>(mapped.pData);

    for (UINT i = 0; i < count; ++i) {
        // SQF [x, y, zASL] -> engine [x, zASL, y]
        dst[i * 4 + 0] = xyz_sqf[i * 3 + 0];
        dst[i * 4 + 1] = xyz_sqf[i * 3 + 2];
        dst[i * 4 + 2] = xyz_sqf[i * 3 + 1];
        dst[i * 4 + 3] = 1.0f;
    }

    ctx->Unmap(g_res.points_buffer, 0);
    return "";
}

// General 4x4 inverse (cofactor expansion). Used to build invViewProj for
// per-pixel world-position reconstruction (Pulse effect).

inline bool inverse_4x4(const float m[4][4], float out[4][4]) {
    const float* a = &m[0][0];
    float inv[16];
    inv[0]  =  a[5]*a[10]*a[15] - a[5]*a[11]*a[14] - a[9]*a[6]*a[15] + a[9]*a[7]*a[14] + a[13]*a[6]*a[11] - a[13]*a[7]*a[10];
    inv[4]  = -a[4]*a[10]*a[15] + a[4]*a[11]*a[14] + a[8]*a[6]*a[15] - a[8]*a[7]*a[14] - a[12]*a[6]*a[11] + a[12]*a[7]*a[10];
    inv[8]  =  a[4]*a[9]*a[15]  - a[4]*a[11]*a[13] - a[8]*a[5]*a[15] + a[8]*a[7]*a[13] + a[12]*a[5]*a[11] - a[12]*a[7]*a[9];
    inv[12] = -a[4]*a[9]*a[14]  + a[4]*a[10]*a[13] + a[8]*a[5]*a[14] - a[8]*a[6]*a[13] - a[12]*a[5]*a[10] + a[12]*a[6]*a[9];
    inv[1]  = -a[1]*a[10]*a[15] + a[1]*a[11]*a[14] + a[9]*a[2]*a[15] - a[9]*a[3]*a[14] - a[13]*a[2]*a[11] + a[13]*a[3]*a[10];
    inv[5]  =  a[0]*a[10]*a[15] - a[0]*a[11]*a[14] - a[8]*a[2]*a[15] + a[8]*a[3]*a[14] + a[12]*a[2]*a[11] - a[12]*a[3]*a[10];
    inv[9]  = -a[0]*a[9]*a[15]  + a[0]*a[11]*a[13] + a[8]*a[1]*a[15] - a[8]*a[3]*a[13] - a[12]*a[1]*a[11] + a[12]*a[3]*a[9];
    inv[13] =  a[0]*a[9]*a[14]  - a[0]*a[10]*a[13] - a[8]*a[1]*a[14] + a[8]*a[2]*a[13] + a[12]*a[1]*a[10] - a[12]*a[2]*a[9];
    inv[2]  =  a[1]*a[6]*a[15]  - a[1]*a[7]*a[14]  - a[5]*a[2]*a[15] + a[5]*a[3]*a[14] + a[13]*a[2]*a[7]  - a[13]*a[3]*a[6];
    inv[6]  = -a[0]*a[6]*a[15]  + a[0]*a[7]*a[14]  + a[4]*a[2]*a[15] - a[4]*a[3]*a[14] - a[12]*a[2]*a[7]  + a[12]*a[3]*a[6];
    inv[10] =  a[0]*a[5]*a[15]  - a[0]*a[7]*a[13]  - a[4]*a[1]*a[15] + a[4]*a[3]*a[13] + a[12]*a[1]*a[7]  - a[12]*a[3]*a[5];
    inv[14] = -a[0]*a[5]*a[14]  + a[0]*a[6]*a[13]  + a[4]*a[1]*a[14] - a[4]*a[2]*a[13] - a[12]*a[1]*a[6]  + a[12]*a[2]*a[5];
    inv[3]  = -a[1]*a[6]*a[11]  + a[1]*a[7]*a[10]  + a[5]*a[2]*a[11] - a[5]*a[3]*a[10] - a[9]*a[2]*a[7]   + a[9]*a[3]*a[6];
    inv[7]  =  a[0]*a[6]*a[11]  - a[0]*a[7]*a[10]  - a[4]*a[2]*a[11] + a[4]*a[3]*a[10] + a[8]*a[2]*a[7]   - a[8]*a[3]*a[6];
    inv[11] = -a[0]*a[5]*a[11]  + a[0]*a[7]*a[9]   + a[4]*a[1]*a[11] - a[4]*a[3]*a[9]  - a[8]*a[1]*a[7]   + a[8]*a[3]*a[5];
    inv[15] =  a[0]*a[5]*a[10]  - a[0]*a[6]*a[9]   - a[4]*a[1]*a[10] + a[4]*a[2]*a[9]  + a[8]*a[1]*a[6]   - a[8]*a[2]*a[5];
    float det = a[0]*inv[0] + a[1]*inv[4] + a[2]*inv[8] + a[3]*inv[12];
    if (fabsf(det) < 1e-12f) return false;
    det = 1.0f / det;
    float* o = &out[0][0];
    for (int k = 0; k < 16; ++k) o[k] = inv[k] * det;
    return true;
}

// Monotonic seconds for effect animation (grain, distortion, pulse)
inline float effect_time_seconds() {
    static const std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
}

// Maps an SQF effect designator (name string or numeric id) to an EffectId.
// Returns -1 if unknown.
// Maps a blend-mode designator (name or id) to 0..5; -1 if unknown.
inline int blend_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = static_cast<int>(static_cast<float>(gv));
        return (id >= 0 && id <= 5) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "normal")   return 0;
    if (s == "additive") return 1;
    if (s == "multiply") return 2;
    if (s == "screen")   return 3;
    if (s == "lighten")  return 4;
    if (s == "darken")   return 5;
    return -1;
}

// Maps a mask-shape designator to 0 (sphere) / 1 (cube); -1 if unknown.
inline int shape_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = static_cast<int>(static_cast<float>(gv));
        return (id == 0 || id == 1) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "sphere" || s == "ellipsoid") return 0;
    if (s == "cube" || s == "box")         return 1;
    return -1;
}

// Reads a scalar (uniform) or [x, y, z] array into out[3]. Returns false on
// a malformed value.
inline bool read_vec3_or_uniform(const game_value& gv, float out[3]) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        const float v = static_cast<float>(gv);
        out[0] = v; out[1] = v; out[2] = v;
        return true;
    }

    if (gv.type_enum() == game_data_type::ARRAY) {
        auto& a = gv.to_array();
        if (a.size() < 3) return false;
        out[0] = static_cast<float>(a[0]);
        out[1] = static_cast<float>(a[1]);
        out[2] = static_cast<float>(a[2]);
        return true;
    }

    return false;
}

inline int effect_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        int id = static_cast<int>(static_cast<float>(gv));
        return (id >= 0 && id <= KH_MAX_EFFECT) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "solid")      return 0;
    if (s == "invert")     return 1;
    if (s == "colorgrade") return 2;
    if (s == "vignette")   return 3;
    if (s == "chromatic")  return 4;
    if (s == "grain")      return 5;
    if (s == "sharpen")    return 6;
    if (s == "blur")       return 7;
    if (s == "bloom")      return 8;
    if (s == "distortion") return 9;
    if (s == "outline")    return 10;
    if (s == "pulse")      return 11;
    if (s == "halation")   return 12;
    if (s == "fog")        return 13;
    if (s == "lensflare")  return 14;
    if (s == "anamorphic") return 15;
    if (s == "sunflare")   return 16;
    return -1;
}

// Writes effect parameters into obj.fx with per-effect defaults for omitted
// values. Pulse positions arrive as SQF [x, y, zASL] and are converted to
// engine space here.
inline void set_effect_params(RenderObject& obj, const auto_array<game_value>* params) {
    static const float defaults[KH_MAX_EFFECT + 1][8] = {
        {},                                          // 0 solid
        {},                                          // 1 invert
        { 1.0f, 1.0f, 1.0f, 1.0f },                  // 2 colorgrade: sat, contrast, brightness, gamma
        { 0.5f, 0.5f },                              // 3 vignette: startRadius, softness
        { 3.0f },                                    // 4 chromatic: strengthPx
        { 0.06f, 24.0f, 1.5f, 0.7f, 0.3f },                            // 5 grain: amount, fps, grainSizePx, lumaResponse, chroma
        { 0.5f },                                    // 6 sharpen: strength
        { 2.0f },                                    // 7 blur: radiusPx
        { 1.0f, 0.8f, 2.0f },                        // 8 bloom: threshold, intensity, radiusPx
        { 6.0f, 6.0f, 2.0f },                        // 9 distortion: amplitudePx, frequency, speed
        { 4.0f, 2.0f, 0.25f, 1.5f },                 // 10 outline: depthEdge, lumEdge, sceneDarken, glowBoost
        { 0.0f, 0.0f, 0.0f, 50.0f, 3.0f, 2.0f },     // 11 pulse: x, y, zASL, radius, bandWidth, intensity
        { 1.0f, 1.2f, 5.0f },                        // 12 halation: threshold, intensity, radiusPx
        { 200.0f, 1200.0f, 1.0f },                   // 13 fog: startDist, endDist, skyAmount
        { 1.1f, 0.7f, 5.0f, 0.35f, 0.45f, 0.6f, 3.0f },  // 14 lensflare: threshold, intensity, ghosts, spacing, haloRadius, haloIntensity, chromaPx
        { 1.2f, 1.2f, 220.0f, 2.0f, 0.0f },              // 15 anamorphic: threshold, intensity, lengthPx, falloffPow, vertical
        { 0.0f, 0.5f, 0.8f, 0.15f, 4.0f, 0.6f, 1.2f },   // 16 sunflare: dirX, dirY, dirZ (SQF axes), size, ghostDots, ringIntensity, starburst
    };

    const int e = (obj.effect >= 0 && obj.effect <= KH_MAX_EFFECT) ? obj.effect : 0;
    memcpy(obj.fx, defaults[e], sizeof(obj.fx));

    if (params) {
        for (size_t i = 0; i < 8 && i < params->size(); ++i) {
            obj.fx[i] = static_cast<float>((*params)[i]);
        }
    }

    if (e == static_cast<int>(EffectId::Pulse) || e == static_cast<int>(EffectId::SunFlare)) {
        // SQF [x, y, zASL] -> engine [x, zASL, y]
        const float sx = obj.fx[0], sy = obj.fx[1], sz = obj.fx[2];
        obj.fx[0] = sx;
        obj.fx[1] = sz;
        obj.fx[2] = sy;
    }
}

// ===========================================================================
// Camera position in engine space, extracted from the view matrix
// (row-vector convention: eye_i = -sum_j T_j * R[i][j])
// ===========================================================================

inline void extract_camera_pos(const float view[4][4], float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = -(view[3][0] * view[i][0] +
                   view[3][1] * view[i][1] +
                   view[3][2] * view[i][2]);
    }
}

// ===========================================================================
// Flush-internal async visibility dispatch. Assumes the graphics lock is
// ALREADY HELD by the caller (flush_frame) - deliberately does not call
// run_depth_compute, which takes its own lock. Dispatches the queued batch
// and copies results into the write-side async staging buffer; no readback.
// ===========================================================================

inline bool flush_dispatch_visibility(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    UINT depth_w = 0, depth_h = 0;
    if (!ensure_depth_srv(dev, ctx, &depth_w, &depth_h).empty()) return false;
    if (!ensure_compute_shaders(dev).empty()) return false;
    const UINT count = static_cast<UINT>(g_query_points_pending.size() / 3);
    if (count == 0) return false;

    // Upload points (SQF -> engine space)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.points_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
        float* dst = static_cast<float*>(mapped.pData);

        for (UINT i = 0; i < count; ++i) {
            dst[i * 4 + 0] = g_query_points_pending[i * 3 + 0];
            dst[i * 4 + 1] = g_query_points_pending[i * 3 + 2];
            dst[i * 4 + 2] = g_query_points_pending[i * 3 + 1];
            dst[i * 4 + 3] = 1.0f;
        }

        ctx->Unmap(g_res.points_buffer, 0);
    }

    RVExtBridge::ProjectionViewTransform pv = {};
    if (!RVExtBridge::get_projection_view_transform(pv)) return false;
    CSConstantData cbd = {};
    mul_4x4(pv.view, pv.projection, cbd.view_proj);
    cbd.depth_params[0] = pv.projection[2][2];
    cbd.depth_params[1] = pv.projection[3][2];

    {
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n_vp, &vp);
        cbd.depth_params[2] = (n_vp >= 1) ? vp.MinDepth : 0.0f;
        cbd.depth_params[3] = (n_vp >= 1) ? vp.MaxDepth : 1.0f;
    }

    cbd.screen_count[0] = static_cast<float>(depth_w);
    cbd.screen_count[1] = static_cast<float>(depth_h);
    cbd.screen_count[2] = static_cast<float>(count);

    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.cs_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.cs_constant_buffer, 0);
    }

    ComputeStateBackup backup;
    backup.capture(ctx);
    ctx->OMSetRenderTargets(0, nullptr, nullptr);   // DSV/SRV hazard
    ID3D11ShaderResourceView* srvs[2] = { g_res.depth_srv, g_res.points_srv };
    ctx->CSSetShaderResources(0, 2, srvs);
    UINT counts[1] = { 0 };
    ctx->CSSetUnorderedAccessViews(0, 1, &g_res.output_uav, counts);
    ctx->CSSetConstantBuffers(0, 1, &g_res.cs_constant_buffer);
    ctx->CSSetShader(g_res.cs_visibility, nullptr, 0);
    ctx->Dispatch((count + 63) / 64, 1, 1);
    ID3D11ShaderResourceView* null_srvs[2] = { nullptr, nullptr };
    ctx->CSSetShaderResources(0, 2, null_srvs);
    ID3D11UnorderedAccessView* null_uav = nullptr;
    ctx->CSSetUnorderedAccessViews(0, 1, &null_uav, counts);
    backup.restore(ctx);
    D3D11_BOX box = { 0, 0, 0, count * sizeof(float) * 4, 1, 1 };

    ctx->CopySubresourceRegion(g_res.staging_async[g_async_write_idx], 0, 0, 0, 0,
                               g_res.output_buffer, 0, &box);

    g_async_inflight_count[g_async_write_idx] = count;
    g_async_write_idx ^= 1;
    return true;
}

// ===========================================================================
// Retained-mode flush: runs once per frame from the C++ Draw3D EH.
// Order: (1) async result pump  (2) async query dispatch  (3) geometry,
// with the scene captured at most ONCE per frame, shared by all
// scene-read objects. Fully dormant (immediate return) when there is nothing to do.
// ===========================================================================

// Runs the actual per-frame work. The graphics lock is ALREADY HELD by the
// caller (flush_frame). Returns after counting the skip reason if the bound
// targets are not the main scene's.
inline void flush_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    bool has_objects;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        has_objects = !g_draw_list.empty();
    }

    const UINT read_idx = g_async_write_idx ^ 1;
    const bool async_read_due = g_async_inflight_count[read_idx] > 0;

    // require a DSV AND require it to be the MAIN
    // scene's depth resource, not a PiP/mirror/UAV sub-pass. Drawing into a
    // sub-pass both misses the visible frame (flicker) and churns the depth
    // SRV cache against the wrong resource.

    {
        ID3D11DepthStencilView* dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &dsv);

        if (!dsv) {
            g_stats.skip_no_dsv++;
            return;
        }

        ID3D11Resource* res = nullptr;
        dsv->GetResource(&res);
        dsv->Release();

        if (!res) {
            g_stats.skip_no_dsv++;
            return;
        }

        ID3D11Texture2D* tex = nullptr;
        HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
        res->Release();

        if (FAILED(hr) || !tex) {
            g_stats.skip_no_dsv++;
            return;
        }

        D3D11_TEXTURE2D_DESC td = {};
        tex->GetDesc(&td);
        void* identity = static_cast<void*>(tex);
        tex->Release();

        const bool adopt = (g_main_depth_identity == nullptr) ||
                           (static_cast<uint64_t>(td.Width) * td.Height >
                            static_cast<uint64_t>(g_main_depth_w) * g_main_depth_h) ||
                           (g_wrong_pass_streak >= KH_WRONG_PASS_READOPT);

        if (adopt) {
            g_main_depth_identity = identity;
            g_main_depth_w = td.Width;
            g_main_depth_h = td.Height;
            g_wrong_pass_streak = 0;
        } else if (identity != g_main_depth_identity) {
            g_stats.skip_wrong_pass++;
            g_wrong_pass_streak++;
            return;
        } else {
            g_wrong_pass_streak = 0;
        }
    }

    if (!ensure_resources(dev).empty()) return;
    g_flush_frame++;
    g_stats.gate_passed++;

    // --- (1) Async result pump: harvest an earlier frame's query, no stall ---
    if (async_read_due) {
        D3D11_MAPPED_SUBRESOURCE mapped = {};

        HRESULT hr = ctx->Map(g_res.staging_async[read_idx], 0, D3D11_MAP_READ,
                              D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);

        if (SUCCEEDED(hr)) {
            const UINT n = g_async_inflight_count[read_idx];
            g_vis_results_cpu.resize(static_cast<size_t>(n) * 4);
            memcpy(g_vis_results_cpu.data(), mapped.pData, static_cast<size_t>(n) * 4 * sizeof(float));
            ctx->Unmap(g_res.staging_async[read_idx], 0);
            g_vis_result_count = n;
            g_vis_result_frame = g_flush_frame;
            g_async_inflight_count[read_idx] = 0;
        }
        // DXGI_ERROR_WAS_STILL_DRAWING: GPU not done - retry next frame
    }

    // --- (2) Async query dispatch (harvested in a later frame) ---
    if (g_query_pending) {
        flush_dispatch_visibility(dev, ctx);
        g_query_pending = false;   // consumed either way; caller re-queues per batch
    }

    // --- (3) Geometry + post-processing ---
    if (!has_objects) return;

    // Snapshot the visible objects under the mutex, draw without holding it.
    // Boxes and fullscreen passes are split; fullscreen passes run last, in
    // handle (creation) order so chained effects compose deterministically.
    std::vector<RenderObject> boxes;
    std::vector<std::pair<uint32_t, RenderObject>> fullscreen;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        boxes.reserve(g_draw_list.size());

        for (const auto& kv : g_draw_list) {
            if (!kv.second.visible) continue;

            if (kv.second.fullscreen) {
                if (!kv.second.affect_ui) fullscreen.emplace_back(kv.first, kv.second);
            } else {
                boxes.push_back(kv.second);
            }
        }
    }

    if (boxes.empty() && fullscreen.empty()) return;

    std::sort(fullscreen.begin(), fullscreen.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    RVExtBridge::ProjectionViewTransform pv = {};
    if (!RVExtBridge::get_projection_view_transform(pv)) return;
    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    bool need_inverse = false;

    for (const auto& o : boxes) {
        if (o.localized || o.effect == static_cast<int>(EffectId::Pulse)) { need_inverse = true; break; }
    }

    if (!need_inverse) {
        for (const auto& f : fullscreen) {
            if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse)) { need_inverse = true; break; }
        }
    }

    float inv_view_proj[4][4] = {};
    const bool has_inverse = need_inverse && inverse_4x4(view_proj, inv_view_proj);
    // has_inverse is only consulted by objects that need it, so skipping the
    // inverse when nothing does is behavior-identical.
    float cam[3];
    extract_camera_pos(pv.view, cam);

    // Screen dimensions + viewport depth range for the effect shader
    float screen_w = 0, screen_h = 0, vp_min_d = 0.0f, vp_max_d = 1.0f;
    {
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n_vp, &vp);

        if (n_vp >= 1) {
            screen_w = vp.Width;
            screen_h = vp.Height;
            vp_min_d = vp.MinDepth;
            vp_max_d = vp.MaxDepth;
            g_scene_vp_min_d = vp.MinDepth;
            g_scene_vp_max_d = vp.MaxDepth;
        }
    }

    const float now = effect_time_seconds();

    // Capability gating per frame:
    //  - effects need the scene capture and the effect shader
    //  - Outline/Pulse additionally need the depth SRV in the PS, which
    //    requires the read-only DSV swap; without it they are skipped
    auto needs_depth = [](const RenderObject& o) {
        return o.localized || o.banded ||
               o.effect == static_cast<int>(EffectId::Outline) ||
               o.effect == static_cast<int>(EffectId::Pulse) ||
               o.effect == static_cast<int>(EffectId::Fog) ||
               o.effect == static_cast<int>(EffectId::SunFlare);
    };

    bool any_effect = !fullscreen.empty();
    bool any_depth_fx = false;

    for (const auto& o : boxes) {
        if (o.effect > 0) any_effect = true;
        if (needs_depth(o)) any_depth_fx = true;
    }

    for (const auto& f : fullscreen) if (needs_depth(f.second)) any_depth_fx = true;
    bool effects_ready = false;
    bool depth_fx_ready = false;
    if (any_effect) {
        UINT dw = 0, dh = 0;

        effects_ready = ensure_depth_srv(dev, ctx, &dw, &dh).empty() &&
                        ensure_effect_shader(dev).empty() &&
                        ensure_scene_capture(dev, ctx).empty();

        depth_fx_ready = effects_ready && any_depth_fx &&
                         g_res.depth_srv && g_res.depth_dsv_ro;

        // Pulse needs the inverse matrix as well
        if (!has_inverse) {
            for (auto& o : boxes)
                if (o.effect == static_cast<int>(EffectId::Pulse)) o.effect = 0;
            for (auto& f : fullscreen)
                if (f.second.effect == static_cast<int>(EffectId::Pulse)) f.second.effect = 0;
        }

        if (!effects_ready) {
            // Demote everything to solid for this frame; fullscreen passes
            // without an effect are meaningless - drop them.
            g_stats.effect_setup_fails++;
            for (auto& o : boxes) o.effect = 0;
            fullscreen.clear();
            if (boxes.empty()) return;
        }
    }

    // Sort boxes: opaque solids first (front-to-back), then translucent /
    // effect boxes (back-to-front), then overlays (back-to-front).
    auto dist_sq = [&cam](const RenderObject& o) {
        const float ex = o.pos[0] - cam[0];   // engine X = SQF x
        const float ey = o.pos[2] - cam[1];   // engine Y = SQF zASL
        const float ez = o.pos[1] - cam[2];   // engine Z = SQF y
        return ex * ex + ey * ey + ez * ez;
    };

    auto group_of = [](const RenderObject& o) {
        if (o.mode == DepthMode::Off) return 2;
        return (o.color[3] >= 0.999f && o.effect == 0 && o.blend_mode == 0) ? 0 : 1;
    };

    std::sort(boxes.begin(), boxes.end(),
              [&](const RenderObject& a, const RenderObject& b) {
                  const int ga = group_of(a), gb = group_of(b);
                  if (ga != gb) return ga < gb;
                  const float da = dist_sq(a), db = dist_sq(b);
                  return ga == 0 ? da < db : da > db;
              });

    StateBackup backup;
    backup.capture(ctx);

    // Depth-sampling effects: swap in the read-only DSV so the depth SRV may
    // legally be bound at PS t1 while depth testing continues to work.
    // Side effect while active: depth WRITES are disabled for the whole
    // phase, so mode-1 objects do not write depth on frames where
    // Outline/Pulse effects are present.
    ID3D11RenderTargetView* saved_rtv = nullptr;
    ID3D11DepthStencilView* saved_dsv = nullptr;

    if (depth_fx_ready) {
        ctx->OMGetRenderTargets(1, &saved_rtv, &saved_dsv);
        ctx->OMSetRenderTargets(1, &saved_rtv, g_res.depth_dsv_ro);
    }

    // Common pipeline state
    UINT stride = sizeof(float) * 3, offset = 0;
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.vertex_buffer, &stride, &offset);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.constant_buffer);
    ctx->PSSetConstantBuffers(0, 1, &g_res.constant_buffer);

    ID3D11ShaderResourceView* ps_srvs[2] = {
        effects_ready ? g_res.scene_srv : nullptr,
        depth_fx_ready ? g_res.depth_srv : nullptr
    };

    ctx->PSSetShaderResources(0, 2, ps_srvs);
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);

    auto upload_cb = [&](const RenderObject& o, bool chain_pass) -> bool {
        ConstantData cbd = {};
        memcpy(cbd.view_proj, view_proj, sizeof(view_proj));
        memcpy(cbd.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y]
        cbd.center_size[2] = o.pos[1];
        cbd.center_size[3] = chain_pass ? 1.0f : 0.0f;
        cbd.size_axes[0] = o.size[0];    // SQF [x,y,z] sizes -> engine [x,z,y]
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        memcpy(cbd.fx0, o.fx, sizeof(cbd.fx0));
        memcpy(cbd.fx1, o.fx + 4, sizeof(cbd.fx1));
        cbd.fx_meta[0] = static_cast<float>(o.effect);
        cbd.fx_meta[1] = now;
        cbd.fx_meta[2] = screen_w;
        cbd.fx_meta[3] = screen_h;
        cbd.depth_params[0] = pv.projection[2][2];
        cbd.depth_params[1] = pv.projection[3][2];
        cbd.depth_params[2] = vp_min_d;
        cbd.depth_params[3] = vp_max_d;
        cbd.local0[0] = o.pos[0];
        cbd.local0[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y]
        cbd.local0[2] = o.pos[1];
        cbd.local0[3] = static_cast<float>(o.local_shape);
        cbd.local_radii[0] = o.local_radius[0];   // SQF [x,y,z] radii -> engine [x,z,y]
        cbd.local_radii[1] = o.local_radius[2];
        cbd.local_radii[2] = o.local_radius[1];
        const float mean_r = (o.local_radius[0] + o.local_radius[1] + o.local_radius[2]) / 3.0f;
        cbd.local1[0] = o.local_falloff / (mean_r > 0.01f ? mean_r : 0.01f);   // normalized falloff
        cbd.local1[1] = o.localized ? 1.0f : 0.0f;
        cbd.band0[0] = o.band_min;
        cbd.band0[1] = o.band_max;
        cbd.band0[2] = o.band_falloff;
        cbd.band0[3] = o.banded ? 1.0f : 0.0f;
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.constant_buffer, 0);
        return true;
    };

    // --- Box pass ---
    for (const auto& o : boxes) {
        if (!upload_cb(o, false)) continue;
        ctx->PSSetShader(o.effect > 0 ? g_res.ps_effect : g_res.ps, nullptr, 0);
        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);

        ID3D11DepthStencilState* dss =
            (o.mode == DepthMode::Off)       ? g_res.dss_off :
            (o.mode == DepthMode::TestWrite) ? g_res.dss_test_write :
                                               g_res.dss_test;

        ctx->OMSetDepthStencilState(dss, 0);
        ctx->Draw(36, 0);
    }

    // --- Fullscreen chain (single-resolve ping-pong) ---
    // The scene is resolved ONCE (re-resolved here only so the chain sees the
    // boxes just drawn); every pass then reads one single-sample chain target
    // and writes its fully composited output opaquely to the other - the
    // blend arithmetic happens in-shader (see the chain packing in PSEffect),
    // which is exact because for a fullscreen pass the blend destination and
    // the sampled scene are the same image. A final opaque blit paints the
    // chain result onto the engine's MSAA target. Bandwidth: one resolve +
    // F single-sample draws + one blit, instead of F resolves + F MSAA draws.
    if (!fullscreen.empty() && effects_ready) {
        std::string chain_err = boxes.empty() ? "" : ensure_scene_capture(dev, ctx);
        if (chain_err.empty()) chain_err = ensure_fx_chain(dev);

        if (!chain_err.empty()) {
            g_stats.effect_setup_fails++;
        } else {
            ID3D11RenderTargetView* saved_chain_rtv = nullptr;
            ID3D11DepthStencilView* saved_chain_dsv = nullptr;
            ctx->OMGetRenderTargets(1, &saved_chain_rtv, &saved_chain_dsv);
            ctx->IASetInputLayout(nullptr);
            ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
            ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
            ctx->OMSetDepthStencilState(g_res.dss_off, 0);
            ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);   // opaque: compositing is in-shader
            ID3D11ShaderResourceView* src_srv = g_res.scene_srv;
            int write_idx = 0;

            for (const auto& f : fullscreen) {
                if (f.second.effect <= 0) continue;
                if (!upload_cb(f.second, true)) continue;

                // Unbind the source slot before binding it as RTV next round
                ID3D11ShaderResourceView* null_srv = nullptr;
                ctx->PSSetShaderResources(0, 1, &null_srv);
                ctx->OMSetRenderTargets(1, &g_res.chain_rtv[write_idx], nullptr);
                ctx->PSSetShaderResources(0, 1, &src_srv);
                ctx->Draw(3, 0);
                src_srv = g_res.chain_srv[write_idx];
                write_idx ^= 1;
            }

            // Blit the chain result onto the engine target: effect 0 passes
            // the sampled scene through; chain packing returns it opaquely.
            if (src_srv != g_res.scene_srv) {
                RenderObject blit;
                blit.effect = 0;

                if (upload_cb(blit, true)) {
                    ID3D11ShaderResourceView* null_srv = nullptr;
                    ctx->PSSetShaderResources(0, 1, &null_srv);
                    ctx->OMSetRenderTargets(1, &saved_chain_rtv, saved_chain_dsv);
                    ctx->PSSetShaderResources(0, 1, &src_srv);
                    ctx->Draw(3, 0);
                }
            } else {
                ctx->OMSetRenderTargets(1, &saved_chain_rtv, saved_chain_dsv);
            }

            if (saved_chain_rtv) saved_chain_rtv->Release();
            if (saved_chain_dsv) saved_chain_dsv->Release();
        }
    }

    if (depth_fx_ready) {
        ctx->OMSetRenderTargets(1, &saved_rtv, saved_dsv);
    }

    if (saved_rtv) saved_rtv->Release();
    if (saved_dsv) saved_dsv->Release();
    backup.restore(ctx);
}

// Per-frame entry point from the Draw3D EH: checks for work, then acquires
// the graphics lock with bounded retries - a transiently contended lock is a
// dropped (effect-less, visibly flickering) frame, so one immediate retry is
// cheap insurance. Persistent failure is counted, not fought.
inline void flush_frame() {
    bool has_work;
    
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        has_work = !g_draw_list.empty();
    }

    has_work = has_work || g_query_pending ||
               g_async_inflight_count[0] > 0 || g_async_inflight_count[1] > 0;

    if (!has_work) return;
    if (!RVExtBridge::is_initialized()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return;
    g_stats.flushes++;

    for (int attempt = 0; attempt < 3; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) {
            g_stats.lock_retries++;
            continue;
        }

        flush_locked(dev, ctx);
        return;
    }

    g_stats.lock_failed_frames++;
}

// ===========================================================================
// UI-affecting passes: post-tonemap backbuffer phase.
// EMPIRICAL (verified in-game): mission-EH moments (EachFrame) can READ the
// completed frame but writes are overwritten before Present; the write window
// is DURING UI rendering, i.e. a control "Draw" event handler. The flush is
// therefore driven by the flushUIRender command, called every frame by a Draw
// EH on an invisible control that ensure_ui_driver() self-hosts on display
// 46. The bound RTV at that moment is the LDR backbuffer (single-sample
// RGBA8/BGRA8, no DSV) - the INVERSE of the scene-pass signature. Passes
// with affect_ui render here over the composited frame, UI included; depth
// remains readable (no DSV bound = no hazard), with UI pixels carrying the
// depth of the scene behind them.
// ===========================================================================

inline std::string ensure_backbuffer_capture(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11Texture2D* src_tex, const D3D11_TEXTURE2D_DESC& td) {
    if (!g_res.bb_tex || g_res.bb_w != td.Width || g_res.bb_h != td.Height || g_res.bb_fmt != td.Format) {
        g_res.release_bb_capture();
        D3D11_TEXTURE2D_DESC bd = {};
        bd.Width = td.Width;
        bd.Height = td.Height;
        bd.MipLevels = 1;
        bd.ArraySize = 1;
        bd.Format = td.Format;
        bd.SampleDesc.Count = 1;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        HRESULT hr = dev->CreateTexture2D(&bd, nullptr, &g_res.bb_tex);
        if (FAILED(hr)) return "Create bb tex " + hr_str(hr);
        hr = dev->CreateShaderResourceView(g_res.bb_tex, nullptr, &g_res.bb_srv);
        if (FAILED(hr)) { g_res.release_bb_capture(); return "Create bb SRV " + hr_str(hr); }
        g_res.bb_w = td.Width;
        g_res.bb_h = td.Height;
        g_res.bb_fmt = td.Format;
    }

    ctx->CopyResource(g_res.bb_tex, src_tex);
    return "";
}

inline void flush_ui_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    std::vector<std::pair<uint32_t, RenderObject>> passes;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            if (kv.second.visible && kv.second.fullscreen && kv.second.affect_ui) {
                passes.emplace_back(kv.first, kv.second);
            }
        }
    }

    if (passes.empty()) return;

    std::sort(passes.begin(), passes.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    // Inverse scene gate: RTV bound, NO DSV, single-sample, 8-bit RGBA/BGRA
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);

    if (dsv) {
        dsv->Release();
        if (rtv) rtv->Release();
        g_stats.ui_gate_skips++;
        return;
    }

    if (!rtv) { g_stats.ui_gate_skips++; return; }
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    if (!res) { g_stats.ui_gate_skips++; return; }
    ID3D11Texture2D* bb = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb));
    res->Release();
    if (FAILED(hr) || !bb) { g_stats.ui_gate_skips++; return; }
    D3D11_TEXTURE2D_DESC td = {};
    bb->GetDesc(&td);
    bool fmt_ok;

    switch (td.Format) {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            fmt_ok = true; break;
        default:
            fmt_ok = false; break;
    }

    if (!fmt_ok || td.SampleDesc.Count > 1) {
        bb->Release();
        g_stats.ui_gate_skips++;
        return;
    }

    if (!ensure_resources(dev).empty()) { bb->Release(); return; }
    if (!ensure_effect_shader(dev).empty()) { bb->Release(); return; }
    g_stats.ui_gate_passed++;
    RVExtBridge::ProjectionViewTransform pv = {};
    if (!RVExtBridge::get_projection_view_transform(pv)) { bb->Release(); return; }
    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    float inv_view_proj[4][4] = {};
    bool need_inverse = false;

    for (const auto& f : passes) {
        if (f.second.localized || f.second.effect == static_cast<int>(EffectId::Pulse)) { need_inverse = true; break; }
    }

    const bool has_inverse = need_inverse && inverse_4x4(view_proj, inv_view_proj);
    const float now = effect_time_seconds();
    const bool depth_ok = g_res.depth_srv != nullptr;
    StateBackup backup;
    backup.capture(ctx);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_effect, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.constant_buffer);
    ctx->PSSetConstantBuffers(0, 1, &g_res.constant_buffer);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->RSSetState(g_res.rasterizer);
    const FLOAT bf[4] = { 0, 0, 0, 0 };

    // The viewport at a control-Draw moment is the CONTROL's rectangle (our
    // driver control is 1px) - a fullscreen triangle through it rasterizes
    // to nothing. Own the viewport explicitly: full target for our passes,
    // engine state restored after.
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&n_saved_vp, saved_vp);
    
    {
        D3D11_VIEWPORT full = {};
        full.Width = static_cast<FLOAT>(td.Width);
        full.Height = static_cast<FLOAT>(td.Height);
        full.MinDepth = 0.0f;
        full.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &full);
    }

    for (const auto& f : passes) {
        const RenderObject& o = f.second;
        if (o.effect <= 0) continue;

        const bool wants_depth = o.localized || o.banded ||
            o.effect == static_cast<int>(EffectId::Outline) ||
            o.effect == static_cast<int>(EffectId::Pulse) ||
            o.effect == static_cast<int>(EffectId::Fog) ||
            o.effect == static_cast<int>(EffectId::SunFlare);

        if (wants_depth && !depth_ok) continue;
        if ((o.localized || o.effect == static_cast<int>(EffectId::Pulse)) && !has_inverse) continue;

        // Per-pass re-capture so UI-phase passes chain (LDR copy - cheap)
        if (!ensure_backbuffer_capture(dev, ctx, bb, td).empty()) { g_stats.effect_setup_fails++; break; }
        ID3D11ShaderResourceView* srvs[2] = { g_res.bb_srv, depth_ok ? g_res.depth_srv : nullptr };
        ctx->PSSetShaderResources(0, 2, srvs);
        ConstantData cbd = {};
        memcpy(cbd.view_proj, view_proj, sizeof(view_proj));
        memcpy(cbd.inv_view_proj, inv_view_proj, sizeof(inv_view_proj));
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];
        cbd.center_size[2] = o.pos[1];
        cbd.center_size[3] = 0.0f;   // hardware blending against the live backbuffer
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        memcpy(cbd.fx0, o.fx, sizeof(cbd.fx0));
        memcpy(cbd.fx1, o.fx + 4, sizeof(cbd.fx1));
        cbd.fx_meta[0] = static_cast<float>(o.effect);
        cbd.fx_meta[1] = now;
        cbd.fx_meta[2] = static_cast<float>(td.Width);
        cbd.fx_meta[3] = static_cast<float>(td.Height);
        cbd.depth_params[0] = pv.projection[2][2];
        cbd.depth_params[1] = pv.projection[3][2];
        cbd.depth_params[2] = g_scene_vp_min_d;   // depth was written through the SCENE viewport
        cbd.depth_params[3] = g_scene_vp_max_d;
        cbd.local0[0] = o.pos[0];
        cbd.local0[1] = o.pos[2];
        cbd.local0[2] = o.pos[1];
        cbd.local0[3] = static_cast<float>(o.local_shape);
        cbd.local_radii[0] = o.local_radius[0];
        cbd.local_radii[1] = o.local_radius[2];
        cbd.local_radii[2] = o.local_radius[1];
        const float mean_r = (o.local_radius[0] + o.local_radius[1] + o.local_radius[2]) / 3.0f;
        cbd.local1[0] = o.local_falloff / (mean_r > 0.01f ? mean_r : 0.01f);
        cbd.local1[1] = o.localized ? 1.0f : 0.0f;
        cbd.band0[0] = o.band_min;
        cbd.band0[1] = o.band_max;
        cbd.band0[2] = o.band_falloff;
        cbd.band0[3] = o.banded ? 1.0f : 0.0f;
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) continue;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.constant_buffer, 0);
        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
        ctx->Draw(3, 0);
    }

    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);
    backup.restore(ctx);
    bb->Release();
}

inline bool flush_ui_frame() {
    bool has_work = false;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (const auto& kv : g_draw_list) {
            if (kv.second.visible && kv.second.fullscreen && kv.second.affect_ui) { has_work = true; break; }
        }
    }

    if (!has_work) return false;
    if (!RVExtBridge::is_initialized()) return false;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (!dev || !ctx) return false;
    g_stats.ui_flushes++;

    for (int attempt = 0; attempt < 3; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) {
            g_stats.lock_retries++;
            continue;
        }

        flush_ui_locked(dev, ctx);
        return true;
    }

    g_stats.lock_failed_frames++;
    return false;
}

// ===========================================================================
// Internal UI-flush driver: self-hosts the control "Draw" callsite. Display
// 46 does not exist at pre_init, so an EachFrame poller waits for it, creates
// an invisible 1px map control once, attaches a compiled-SQF Draw handler
// invoking the flushUIRender command, then idles (one flag check per frame).
// ===========================================================================

static intercept::client::EHIdentifierHandle g_ui_poll_eh;
static bool g_ui_driver_registered = false;
static bool g_ui_ctrl_created = false;
static uint64_t g_ui_poll_attempts = 0;

inline void ensure_ui_driver() {
    if (g_ui_driver_registered) return;
    g_ui_driver_registered = true;
    g_ui_ctrl_created = false;

    g_ui_poll_eh = intercept::client::addMissionEventHandler<
        intercept::client::eventhandlers_mission::EachFrame>([]() {
        if (g_ui_ctrl_created) return;
        g_ui_poll_attempts++;

        try {
            game_value result = raw_call_sqf_native(g_compiled_kh_ui_render_init);

            if (result.type_enum() == game_data_type::BOOL && static_cast<bool>(result)) {
                g_ui_ctrl_created = true;
            }
        } catch (...) {
            // display transitioning or call context unavailable - retry next frame
        }
    });
}

// ===========================================================================
// C++ Draw3D event handler lifecycle.
// Mission EHs die with the mission: call on_mission_start() from
// intercept::pre_init() and on_mission_end() from intercept::mission_ended().
// ensure_draw_eh() also lazily self-registers as a fallback, so retained
// objects added mid-mission always render even without the lifecycle calls.
// ===========================================================================

static intercept::client::EHIdentifierHandle g_draw3d_eh;
static bool g_draw3d_eh_active = false;

inline void ensure_draw_eh() {
    if (g_draw3d_eh_active) return;

    g_draw3d_eh = intercept::client::addMissionEventHandler<
        intercept::client::eventhandlers_mission::Draw3D>([]() {
        try {
            flush_frame();
        } catch (const std::exception& e) {
            report_error(std::string("RenderIntegration flush: ") + e.what());
        } catch (...) {
            report_error("RenderIntegration flush: unknown exception");
        }
    });

    ensure_ui_driver();
    g_draw3d_eh_active = true;
}

inline void reset_retained_state() {
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        g_draw_list.clear();
    }

    g_query_pending = false;
    g_query_points_pending.clear();
    g_async_inflight_count[0] = 0;
    g_async_inflight_count[1] = 0;
    g_vis_result_count = 0;
}

inline void on_mission_start() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    ensure_ui_driver();
    reset_retained_state();
}

inline void on_mission_end() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();
}

// ===========================================================================
// Retained-mode API (called by the SQF wrappers)
// ===========================================================================

inline uint32_t add_render_object(const RenderObject& obj) {
    ensure_draw_eh();
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const uint32_t handle = g_next_handle++;
    g_draw_list[handle] = obj;
    return handle;
}

inline bool remove_render_object(uint32_t handle) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    return g_draw_list.erase(handle) > 0;
}

inline size_t clear_render_objects() {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const size_t n = g_draw_list.size();
    g_draw_list.clear();
    return n;
}

} // namespace RenderIntegration

// ---------------------------------------------------------------------------
// SQF entry points
// ---------------------------------------------------------------------------

static game_value sample_scene_depth_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 2) return game_value("usage: sampleSceneDepth [u, v]");
        float u = static_cast<float>(arr[0]);
        float v = static_cast<float>(arr[1]);

        // Convert uv (0..1) to pixel coords against the live depth buffer
        // dimensions, then run the single-pixel compute sample.
        float results[4] = {};

        std::string status = [&]() -> std::string {
            ID3D11Device* dev = RVExtBridge::get_d3d_device();
            ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
            if (!dev || !ctx) return "device/context null";
            UINT w = 0, h = 0;
            {
                RVExtBridge::ScopedGraphicsLock lock;
                if (!lock.acquired()) return "SKIP: graphics lock not acquired";
                std::string e = RenderIntegration::ensure_depth_srv(dev, ctx, &w, &h);
                if (!e.empty()) return "depth SRV: " + e;
            }
            float px = u * static_cast<float>(w);
            float py = v * static_cast<float>(h);
            return RenderIntegration::run_depth_compute(RenderIntegration::ComputeKernel::SampleDepth, 0, px, py, results, 4);
        }();

        if (status != "OK") return game_value(status);
        auto_array<game_value> out;
        out.push_back(game_value(results[2]));  // scene distance, meters
        out.push_back(game_value(results[3]));  // raw depth buffer value
        return game_value(std::move(out));
    } catch (const std::exception& e) {
        report_error(std::string("sampleSceneDepth: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("sampleSceneDepth: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// gpuVisibility [[x,y,zASL], [x,y,zASL], ...]   (max 1024 points)
// Tests every point against the engine depth buffer in ONE GPU dispatch.
// Returns one entry per point: [status, pointDistM, sceneDistM]
//   status: 1 = visible, 0 = occluded by scene geometry, -1 = offscreen/behind camera
// Note: like all depth-based tests, cannot account for particles (they do not
// write depth). Call from Draw3D.
static game_value gpu_visibility_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        UINT count = static_cast<UINT>(arr.size());
        if (count == 0) return game_value(auto_array<game_value>());
        if (count > RenderIntegration::KH_MAX_QUERY_POINTS) count = RenderIntegration::KH_MAX_QUERY_POINTS;

        std::vector<float> pts(count * 3);
        for (UINT i = 0; i < count; ++i) {
            auto& p = arr[i].to_array();
            if (p.size() < 3) return game_value("each point must be [x, y, zASL]");
            pts[i * 3 + 0] = static_cast<float>(p[0]);
            pts[i * 3 + 1] = static_cast<float>(p[1]);
            pts[i * 3 + 2] = static_cast<float>(p[2]);
        }

        std::string status = RenderIntegration::upload_query_points(pts.data(), count);
        if (!status.empty()) return game_value(status);

        std::vector<float> results(count * 4);
        status = RenderIntegration::run_depth_compute(RenderIntegration::ComputeKernel::Visibility, count, 0.0f, 0.0f,
                                                      results.data(), count * 4);
                                                    
        if (status != "OK") return game_value(status);
        auto_array<game_value> out;
        out.reserve(count);
        for (UINT i = 0; i < count; ++i) {
            auto_array<game_value> e;
            e.push_back(game_value(results[i * 4 + 0]));  // status
            e.push_back(game_value(results[i * 4 + 1]));  // point distance, m
            e.push_back(game_value(results[i * 4 + 2]));  // scene distance at pixel, m
            out.push_back(game_value(std::move(e)));
        }
        return game_value(std::move(out));
    } catch (const std::exception& e) {
        report_error(std::string("gpuVisibility: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("gpuVisibility: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// addRender3D [[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?, effect?, params?, band?]
// Adds a persistent box drawn every frame by the internal Draw3D EH until
// removed. Callable from ANY context (scheduled, unscheduled, callbacks).
//   mode:      0 = depth test (default), 1 = test + depth write, 2 = overlay
//   sceneRead: BOOL, legacy shorthand for effect "invert"
//   effect:    STRING or SCALAR - screen-space effect applied inside the
//              box's footprint: "solid" 0, "invert" 1, "colorgrade" 2,
//              "vignette" 3, "chromatic" 4, "grain" 5, "sharpen" 6,
//              "blur" 7, "bloom" 8, "distortion" 9, "outline" 10, "pulse" 11
//   params:    ARRAY of up to 8 numbers, effect-specific (see set_effect_params
//              for meanings and defaults; omitted entries take defaults)
//   band:      [minDist, maxDist, falloff?] - additionally confines the box's
//              effect to a camera-distance band (maxDist <= 0 = unbounded)
// Returns SCALAR handle (>= 1) or a STRING error.

static game_value add_render3d_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 2) return game_value("usage: addRender3D [[x,y,zASL], size, [r,g,b,a]?, mode?, sceneRead?]");
        RenderIntegration::RenderObject obj;
        auto& pos = arr[0].to_array();
        if (pos.size() < 3) return game_value("position must be [x, y, zASL]");
        obj.pos[0] = static_cast<float>(pos[0]);
        obj.pos[1] = static_cast<float>(pos[1]);
        obj.pos[2] = static_cast<float>(pos[2]);
        
        if (!RenderIntegration::read_vec3_or_uniform(arr[1], obj.size)) {
            return game_value("size must be a number or [x, y, z]");
        }

        if (arr.size() > 2 && arr[2].type_enum() == game_data_type::ARRAY) {
            auto& col = arr[2].to_array();
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        }

        if (arr.size() > 3 && arr[3].type_enum() == game_data_type::SCALAR) {
            int m = static_cast<int>(static_cast<float>(arr[3]));
            if (m >= 0 && m <= 2) obj.mode = static_cast<RenderIntegration::DepthMode>(m);
        }

        if (arr.size() > 4 && arr[4].type_enum() == game_data_type::BOOL) {
            obj.effect = static_cast<bool>(arr[4]) ? 1 : 0;   // legacy sceneRead = invert
        }

        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 5) {
            const int e = RenderIntegration::effect_id_from_gv(arr[5]);
            if (e < 0) return game_value("unknown effect");
            obj.effect = e;
        }
        if (arr.size() > 6 && arr[6].type_enum() == game_data_type::ARRAY) {
            fx_params = &arr[6].to_array();
        }

        RenderIntegration::set_effect_params(obj, fx_params);

        if (arr.size() > 7 && arr[7].type_enum() == game_data_type::ARRAY) {
            auto& band = arr[7].to_array();

            if (band.size() >= 2) {
                obj.banded = true;
                obj.band_min = static_cast<float>(band[0]);
                obj.band_max = static_cast<float>(band[1]);
                if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
            }
        }

        if (arr.size() > 8) {
            const int bm = RenderIntegration::blend_id_from_gv(arr[8]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        return game_value(static_cast<float>(RenderIntegration::add_render_object(obj)));
    } catch (const std::exception& e) {
        report_error(std::string("addRender3D: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addRender3D: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// updatePostFX [handle, property, value] -> BOOL
// properties: "position" [x,y,zASL] | "size" scalar | "color" [r,g,b,a] |
//             "mode" 0..2 | "visible" bool | "sceneread" bool (legacy invert) |
//             "effect" string/scalar | "params" array (resets omitted entries
//             to the effect's defaults) | "radius" scalar | "falloff" scalar
//             (radius/falloff apply to addLocalPostFX passes; "position"
//             moves their world-space center) | "band" [minDist, maxDist,
//             falloff?] (camera-distance confinement; [] clears it) |
//             "localsphere" [radius, falloff?] (enables the world-space
//             sphere mask on any object, centered on its position; [] clears)
static game_value update_post_fx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 3) return game_value(false);
        const uint32_t handle = static_cast<uint32_t>(static_cast<float>(arr[0]));
        std::string prop = static_cast<std::string>(arr[1]);
        std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
        std::lock_guard<std::mutex> g(RenderIntegration::g_draw_list_mutex);
        auto it = RenderIntegration::g_draw_list.find(handle);
        if (it == RenderIntegration::g_draw_list.end()) return game_value(false);
        auto& obj = it->second;

        if (prop == "position") {
            auto& pos = arr[2].to_array();
            if (pos.size() < 3) return game_value(false);
            obj.pos[0] = static_cast<float>(pos[0]);
            obj.pos[1] = static_cast<float>(pos[1]);
            obj.pos[2] = static_cast<float>(pos[2]);
        } else if (prop == "size") {
            if (!RenderIntegration::read_vec3_or_uniform(arr[2], obj.size)) return game_value(false);
        } else if (prop == "color") {
            auto& col = arr[2].to_array();
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        } else if (prop == "mode") {
            int m = static_cast<int>(static_cast<float>(arr[2]));
            if (m < 0 || m > 2) return game_value(false);
            obj.mode = static_cast<RenderIntegration::DepthMode>(m);
        } else if (prop == "visible") {
            obj.visible = static_cast<bool>(arr[2]);
        } else if (prop == "sceneread") {
            obj.effect = static_cast<bool>(arr[2]) ? 1 : 0;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "effect") {
            const int e = RenderIntegration::effect_id_from_gv(arr[2]);
            if (e < 0) return game_value(false);
            obj.effect = e;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "params") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            RenderIntegration::set_effect_params(obj, &arr[2].to_array());
        } else if (prop == "radius") {
            if (!RenderIntegration::read_vec3_or_uniform(arr[2], obj.local_radius)) return game_value(false);
        } else if (prop == "falloff") {
            obj.local_falloff = static_cast<float>(arr[2]);
        } else if (prop == "localsphere") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& sp = arr[2].to_array();
            if (sp.size() < 1) {
                obj.localized = false;
            } else {
                obj.localized = true;
                if (!RenderIntegration::read_vec3_or_uniform(sp[0], obj.local_radius)) return game_value(false);
                if (sp.size() >= 2) obj.local_falloff = static_cast<float>(sp[1]);
            }
        } else if (prop == "shape") {
            const int sh = RenderIntegration::shape_id_from_gv(arr[2]);
            if (sh < 0) return game_value(false);
            obj.local_shape = sh;
        } else if (prop == "blend") {
            const int bm = RenderIntegration::blend_id_from_gv(arr[2]);
            if (bm < 0) return game_value(false);
            obj.blend_mode = bm;
        } else if (prop == "ui") {
            obj.affect_ui = static_cast<bool>(arr[2]);
        } else if (prop == "band") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& band = arr[2].to_array();
            
            if (band.size() < 2) {
                obj.banded = false;   // empty/short array clears the band
            } else {
                obj.banded = true;
                obj.band_min = static_cast<float>(band[0]);
                obj.band_max = static_cast<float>(band[1]);
                if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
            }
        } else {
            return game_value(false);
        }

        return game_value(true);
    } catch (const std::exception& e) {
        report_error(std::string("updatePostFX: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("updatePostFX: unknown exception");
        return game_value(false);
    }
}

// removeRenderHandler handle -> BOOL (true if an object was removed)
// removeRenderHandler -1     -> BOOL (clears the entire draw list)
static game_value remove_render_handler_sqf(game_value_parameter arg) {
    try {
        const float raw = static_cast<float>(arg);

        if (raw < 0.0f) {
            RenderIntegration::clear_render_objects();
            return game_value(true);
        }
        
        return game_value(RenderIntegration::remove_render_object(static_cast<uint32_t>(raw)));
    } catch (const std::exception& e) {
        report_error(std::string("removeRenderHandler: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("removeRenderHandler: unknown exception");
        return game_value(false);
    }
}

// queueVisibility [[x,y,zASL], ...] -> SCALAR accepted count
// Dispatched during the next flush; results readable 1-2 frames later via
// getVisibilityResults (no GPU stall, unlike the synchronous gpuVisibility).
// Queueing again before the flush overwrites the pending batch.
static game_value queue_visibility_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        UINT count = static_cast<UINT>(arr.size());
        if (count > RenderIntegration::KH_MAX_QUERY_POINTS) count = RenderIntegration::KH_MAX_QUERY_POINTS;
        RenderIntegration::g_query_points_pending.resize(static_cast<size_t>(count) * 3);

        for (UINT i = 0; i < count; ++i) {
            auto& p = arr[i].to_array();
            if (p.size() < 3) return game_value("each point must be [x, y, zASL]");
            RenderIntegration::g_query_points_pending[i * 3 + 0] = static_cast<float>(p[0]);
            RenderIntegration::g_query_points_pending[i * 3 + 1] = static_cast<float>(p[1]);
            RenderIntegration::g_query_points_pending[i * 3 + 2] = static_cast<float>(p[2]);
        }

        RenderIntegration::g_query_pending = count > 0;
        if (count > 0) RenderIntegration::ensure_draw_eh();   // the flush performs the dispatch
        return game_value(static_cast<float>(count));
    } catch (const std::exception& e) {
        report_error(std::string("queueVisibility: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("queueVisibility: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// getVisibilityResults -> [ageInFrames, [[status, pointDistM, sceneDistM], ...]]
// status: 1 visible, 0 occluded, -1 offscreen/behind camera.
// Results array is empty until the first queued batch completes.
static game_value get_visibility_results_sqf() {
    try {
        auto_array<game_value> results;
        results.reserve(RenderIntegration::g_vis_result_count);

        for (UINT i = 0; i < RenderIntegration::g_vis_result_count; ++i) {
            auto_array<game_value> e;
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 0]));
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 1]));
            e.push_back(game_value(RenderIntegration::g_vis_results_cpu[i * 4 + 2]));
            results.push_back(game_value(std::move(e)));
        }

        const float age = static_cast<float>(
            RenderIntegration::g_flush_frame - RenderIntegration::g_vis_result_frame);

        auto_array<game_value> out;
        out.push_back(game_value(age));
        out.push_back(game_value(std::move(results)));
        return game_value(std::move(out));
    } catch (...) {
        report_error("getVisibilityResults: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// addPostFX [effect, params?, color?, band?]
// Creates a persistent FULLSCREEN post-processing pass, applied to the whole
// scene every frame until removed. Shares the handle space with addRender3D:
// removeRenderHandler and updatePostFX ("effect", "params", "color",
// "visible") work on it. Multiple passes chain in creation (handle) order and
// compose - each pass sees the output of the previous ones.
//
//   effect: STRING or SCALAR (same table as addRender3D):
//           "invert" | "colorgrade" | "vignette" | "chromatic" | "grain" |
//           "sharpen" | "blur" | "bloom" | "distortion" | "outline" | "pulse" |
//           "halation" | "fog"
//   params: ARRAY of up to 8 numbers, effect-specific; omitted = defaults:
//           colorgrade [saturation 1, contrast 1, brightness 1, gamma 1]
//           vignette   [startRadius 0.5, softness 0.5]
//           chromatic  [strengthPx 3]
//           grain      [amount 0.08, speed 10]
//           sharpen    [strength 0.5]
//           blur       [radiusPx 2]
//           bloom      [threshold 1, intensity 0.8, radiusPx 2]
//           distortion [amplitudePx 6, frequency 6, speed 2]
//           outline    [depthEdgeScale 4, lumEdgeScale 2, sceneDarken 0.25, glowBoost 1.5]
//           pulse      [x, y, zASL, radius 50, bandWidth 3, intensity 2]
//           halation   [threshold 1, intensity 1.2, radiusPx 5]
//           fog        [startDist 200, endDist 1200, skyAmount 1]
//   color:  [r,g,b,a] - effect tint/edge/ring color; ALPHA = overall effect
//           intensity (blends the effect over the untouched scene)
//   band:   [minDist, maxDist, falloff?] - confines the effect to a camera-
//           distance band: full strength for scene distances within
//           [minDist, maxDist] meters, fading over falloff meters (default
//           10) at both edges. maxDist <= 0 = unbounded far (sky included).
//           Enables layered whole-screen effects by distance, e.g. one grade
//           for the foreground and another for the far terrain.
//
// Notes: runs pre-tonemap, so the engine's eye adaptation applies on top.
// Outline and Pulse sample the engine depth buffer per pixel; on frames where
// they are active, mode-1 boxes do not write depth (read-only DSV phase).
// Returns SCALAR handle or a STRING error.
static game_value add_postfx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 1) return game_value("usage: addPostFX [effect, params?, [r,g,b,a]?]");
        RenderIntegration::RenderObject obj;
        obj.fullscreen = true;
        obj.mode = RenderIntegration::DepthMode::Off;
        const int e = RenderIntegration::effect_id_from_gv(arr[0]);
        if (e <= 0) return game_value("unknown or non-fullscreen effect");
        obj.effect = e;
        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 1 && arr[1].type_enum() == game_data_type::ARRAY) {
            fx_params = &arr[1].to_array();
        }

        RenderIntegration::set_effect_params(obj, fx_params);

        if (arr.size() > 2 && arr[2].type_enum() == game_data_type::ARRAY) {
            auto& col = arr[2].to_array();
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        }

        if (arr.size() > 3 && arr[3].type_enum() == game_data_type::ARRAY) {
            auto& band = arr[3].to_array();

            if (band.size() >= 2) {
                obj.banded = true;
                obj.band_min = static_cast<float>(band[0]);
                obj.band_max = static_cast<float>(band[1]);
                if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
            }
        }

        if (arr.size() > 4) {
            const int bm = RenderIntegration::blend_id_from_gv(arr[4]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        if (arr.size() > 5 && arr[5].type_enum() == game_data_type::BOOL) {
            obj.affect_ui = static_cast<bool>(arr[5]);
        }

        return game_value(static_cast<float>(RenderIntegration::add_render_object(obj)));
    } catch (const std::exception& e) {
        report_error(std::string("addPostFX: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addPostFX: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// getRenderStats -> ARRAY of [name, value] pairs (cumulative since load).
// Skip counters name the reason for any effect flicker: a skipped flush is a
// frame rendered without our draws.
static game_value get_render_stats_sqf() {
    try {
        auto kv = [](const char* k, uint64_t v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(static_cast<float>(v)));
            return game_value(std::move(pair));
        };
        auto_array<game_value> out;
        out.push_back(kv("flushes", RenderIntegration::g_stats.flushes));
        out.push_back(kv("gatePassed", RenderIntegration::g_stats.gate_passed));
        out.push_back(kv("lockRetries", RenderIntegration::g_stats.lock_retries));
        out.push_back(kv("lockFailedFrames", RenderIntegration::g_stats.lock_failed_frames));
        out.push_back(kv("skipNoDsv", RenderIntegration::g_stats.skip_no_dsv));
        out.push_back(kv("skipWrongPass", RenderIntegration::g_stats.skip_wrong_pass));
        out.push_back(kv("effectSetupFails", RenderIntegration::g_stats.effect_setup_fails));
        out.push_back(kv("uiFlushes", RenderIntegration::g_stats.ui_flushes));
        out.push_back(kv("uiGatePassed", RenderIntegration::g_stats.ui_gate_passed));
        out.push_back(kv("uiGateSkips", RenderIntegration::g_stats.ui_gate_skips));
        out.push_back(kv("uiDriverPolls", RenderIntegration::g_ui_poll_attempts));
        out.push_back(kv("uiDriverCtrl", RenderIntegration::g_ui_ctrl_created ? 1 : 0));
        out.push_back(kv("mainSceneW", RenderIntegration::g_main_depth_w));
        out.push_back(kv("mainSceneH", RenderIntegration::g_main_depth_h));
        return game_value(std::move(out));
    } catch (...) {
        report_error("getRenderStats: unknown exception");
        return game_value(auto_array<game_value>());
    }
}

// addLocalPostFX [[x,y,zASL], radius, falloff, effect, params?, color?]
// Same effect table and parameters as addPostFX, but the effect is confined
// to a world-space sphere: full strength within 'radius' meters of the
// position, smoothly fading to nothing over the next 'falloff' meters.
// The mask is computed per pixel from the depth buffer, so it hugs geometry:
// a localized colorgrade desaturates the buildings inside the sphere and
// nothing behind them. Shares the handle space with addRender3D/addPostFX;
// manage via updatePostFX ("position" moves the center, "radius",
// "falloff", "effect", "params", "color", "visible") and removeRenderHandler.
// Localized passes always sample the depth buffer (read-only DSV phase rules
// apply). Returns SCALAR handle or a STRING error.
static game_value add_local_postfx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 4) return game_value("usage: addLocalPostFX [[x,y,zASL], radius, falloff, effect, params?, [r,g,b,a]?]");
        RenderIntegration::RenderObject obj;
        obj.fullscreen = true;
        obj.localized = true;
        obj.mode = RenderIntegration::DepthMode::Off;
        auto& pos = arr[0].to_array();
        if (pos.size() < 3) return game_value("position must be [x, y, zASL]");
        obj.pos[0] = static_cast<float>(pos[0]);
        obj.pos[1] = static_cast<float>(pos[1]);
        obj.pos[2] = static_cast<float>(pos[2]);

        if (!RenderIntegration::read_vec3_or_uniform(arr[1], obj.local_radius)) {
            return game_value("radius must be a number or [x, y, z]");
        }
        
        obj.local_falloff = static_cast<float>(arr[2]);
        const int e = RenderIntegration::effect_id_from_gv(arr[3]);
        if (e <= 0) return game_value("unknown or non-fullscreen effect");
        obj.effect = e;
        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 4 && arr[4].type_enum() == game_data_type::ARRAY) {
            fx_params = &arr[4].to_array();
        }
        
        RenderIntegration::set_effect_params(obj, fx_params);

        if (arr.size() > 5 && arr[5].type_enum() == game_data_type::ARRAY) {
            auto& col = arr[5].to_array();
            for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        }

        if (arr.size() > 6) {
            const int sh = RenderIntegration::shape_id_from_gv(arr[6]);
            if (sh < 0) return game_value("unknown shape (sphere | cube)");
            obj.local_shape = sh;
        }

        if (arr.size() > 7) {
            const int bm = RenderIntegration::blend_id_from_gv(arr[7]);
            if (bm < 0) return game_value("unknown blend mode");
            obj.blend_mode = bm;
        }

        return game_value(static_cast<float>(RenderIntegration::add_render_object(obj)));
    } catch (const std::exception& e) {
        report_error(std::string("addLocalPostFX: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addLocalPostFX: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// flushUIRender
// Renders all UI-affecting passes (addPostFX with affectUI = true) into the
// frame being composed. Driven automatically by the internal overlay control
// created by ensure_ui_driver(); also callable from a Draw EH on a custom
// display. Cheap no-op when no UI-affecting passes exist. Returns BOOL: true
// if passes were queued this call.
static game_value flush_ui_render_sqf() {
    try {
        return game_value(RenderIntegration::flush_ui_frame());
    } catch (const std::exception& e) {
        report_error(std::string("flushUIRender: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("flushUIRender: unknown exception");
        return game_value(false);
    }
}