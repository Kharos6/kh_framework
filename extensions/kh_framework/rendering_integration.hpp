// COM release helper used throughout this header. Defined once here and
// undefined at the end of the file so it never leaks into later includes.
#define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

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
    ID3D11VertexShader*      vs_composite = nullptr;    // injected-path VS (adds world position)
    ID3D11PixelShader*       ps_maskcast = nullptr;     // analytic mask cast
    ID3D11PixelShader*       ps_maskapply = nullptr;    // composite factor onto scene
    ID3D11PixelShader*       ps_composite = nullptr;    // injected-path PS: PSMain + the opaque
                                                        // punch-through guard against the copied
                                                        // scene depth (the HYBRID: hardware depth
                                                        // handles marginal ties, the guard vetoes
                                                        // fragments CLEARLY behind opaque geometry
                                                        // so matrix races can never punch the box
                                                        // through a wall)
    UINT                     ps_composite_samples = 0;  // depth MSAA count it was compiled for
    ID3D11Texture2D*          comp_depth_tex = nullptr; // owned mid-frame copy of the scene depth
    ID3D11ShaderResourceView* comp_depth_srv = nullptr; // (the live depth is bound as a writable
                                                        // DSV during injection - sampling it
                                                        // directly is a bind hazard)
    void*                    comp_depth_identity = nullptr;
    UINT                     comp_depth_w = 0, comp_depth_h = 0, comp_depth_samples = 1;
    float                    comp_depth_time = -1.0f;   // last snapshot copy (overlay freshness gate)
    ID3D11InputLayout*       input_layout = nullptr;
    ID3D11Buffer*            vertex_buffer = nullptr;    // static unit cube
    ID3D11Buffer*            constant_buffer = nullptr;  // dynamic, per draw (game-thread flush)
    ID3D11Buffer*            composite_cb = nullptr;     // dynamic, per draw (render-thread injection)
                                                         // - separate from constant_buffer so the two
                                                         // paths never share a mapped resource
    ID3D11DepthStencilState* dss_test = nullptr;         // LESS_EQUAL, no write
    ID3D11DepthStencilState* dss_test_write = nullptr;   // LESS_EQUAL, write
    ID3D11DepthStencilState* dss_off = nullptr;          // depth disabled
    ID3D11BlendState*        blend_modes[6] = {};       // normal, additive, multiply, screen, lighten, darken
    ID3D11RasterizerState*   rasterizer = nullptr;       // CullNone, MSAA, depth bias. The -32/-1
                                                         // near-pull bias applies to the INJECTED
                                                         // path too: it suppresses the marginal-tie
                                                         // shimmer from the sim PV being one frame
                                                         // ahead (removing it for the injection was
                                                         // tried and measurably WORSENED close-range
                                                         // flicker)
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
            KH_SAFE_RELEASE(chain_tex[i]);
            KH_SAFE_RELEASE(chain_rtv[i]);
            KH_SAFE_RELEASE(chain_srv[i]);
        }
    }

    void release_scene_capture() {
        KH_SAFE_RELEASE(scene_tex);
        KH_SAFE_RELEASE(scene_srv);
        scene_w = 0; scene_h = 0; scene_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_depth_srv() {
        if (depth_srv) { depth_srv->Release(); depth_srv = nullptr; }
        if (depth_dsv_ro) { depth_dsv_ro->Release(); depth_dsv_ro = nullptr; }
        depth_res_identity = nullptr;
        depth_sample_count = 0;
    }

    void release_compute_shaders() {
        KH_SAFE_RELEASE(cs_visibility);
        KH_SAFE_RELEASE(cs_sample_depth);
        cs_compiled_for_samples = 0;
    }

    void release() {
        KH_SAFE_RELEASE(vs);
        KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(vs_fullscreen);
        KH_SAFE_RELEASE(ps_effect);
        ps_effect_samples = 0;
        KH_SAFE_RELEASE(vs_composite);
        KH_SAFE_RELEASE(ps_composite);
        ps_composite_samples = 0;
        KH_SAFE_RELEASE(comp_depth_tex);
        KH_SAFE_RELEASE(comp_depth_srv);
        comp_depth_identity = nullptr;
        comp_depth_w = comp_depth_h = 0;
        comp_depth_samples = 1;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vertex_buffer);
        KH_SAFE_RELEASE(constant_buffer);
        KH_SAFE_RELEASE(composite_cb);
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
    // --- World-lighting interaction (opt-in via 'lit') ---
    // Shaded per pixel against the sun/moon (getLighting) in ApplyLighting;
    // the direct term is additionally attenuated by shadow_factor - the
    // per-object "does the world shadow this box" answer, measured on the
    // game thread each flush with checkVisibility rays toward the sun.
    bool  lit = false;
    float light_ambient = 0.40f;   // base-color fraction kept in full shadow
    float light_diffuse = 0.60f;   // N.L-scaled fraction (ambient + diffuse ~ 1
                                   // preserves the unlit look in full sun)
    int   shadow_mode = 1;         // 0 = no occlusion rays, 1 = center ray,
                                   // 2 = center + 8 corners (soft edges)
    float shadow_factor = 1.0f;    // smoothed occlusion result (0 shadowed .. 1 lit)
    float shadow_update_time = -1.0f;
    DepthMode mode = DepthMode::TestOnly;
    bool  visible = true;
    // Reordered compositing is ALWAYS ON for solid, non-overlay boxes: they
    // are drawn by the D3D hook immediately BEFORE the engine's first
    // translucent scene draw, with depth written - the ENGINE then
    // composites every particle/translucent against them (smoke in front
    // blends over the box, smoke behind is rejected), pixel-perfect. The
    // Draw3D flush covers them only while the hook is unavailable. Notes:
    // composited boxes write depth even in mode 0, and a translucent
    // composited box fully occludes engine translucents behind it. Effect
    // boxes and overlays always render on the flush path.
    // Lifetime: all zero = infinite (removed only by removeRenderHandler).
    // Otherwise the object fades in over fade_in s, holds for hold_dur s,
    // fades out over fade_out s, then auto-removes itself. The envelope
    // multiplies the effective color alpha (the universal intensity).
    bool  timed = false;
    float birth_time = 0.0f;
    float fade_in = 0.0f;
    float hold_dur = 0.0f;
    float fade_out = 0.0f;

};

// Effect ids shared by boxes (localized, clipped to the box's screen footprint)
// and fullscreen passes. 10 and 11 sample the engine depth buffer per pixel.
enum class EffectId : int {
    Solid = 0, Invert = 1, ColorGrade = 2, Vignette = 3, Chromatic = 4,
    Grain = 5, Sharpen = 6, Blur = 7, Bloom = 8, Distortion = 9,
    Outline = 10, Pulse = 11, Halation = 12, Fog = 13,
    LensFlare = 14, Anamorphic = 15, SunFlare = 16, Glitch = 17,
};

static constexpr int KH_MAX_EFFECT = 17;
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
    uint64_t composite_injections = 0; // pre-translucent injection events (once per scene frame)
    uint64_t composite_boxes = 0;      // boxes drawn through the composited path
    uint64_t composite_skips = 0;      // injections aborted (resources/PV unavailable)
    uint64_t composite_ambiguous = 0;  // frames where the sim republished matrices mid-cycle
                                       // (diagnostic only - the clear-time latch is retained)
    uint64_t composite_proj_lock = 0;  // cycles where the engine's true projection depth
                                       // coefficients were captured and applied
    uint64_t composite_rearms = 0;     // mid-cycle partition changes that re-armed the injection
    uint64_t composite_rej_span = 0;   // triggers rejected: partition does not span the broad middle
    uint64_t composite_rej_verify = 0; // triggers rejected: live DSV verification mismatch
    uint64_t composite_rej_floor = 0;  // triggers rejected: opaque evidence below the floor
    // --- Shadow-pass recon diagnostics (setShadowRecon; see the recon
    //     section for what each number decides) ---
    uint64_t shadow_affine44 = 0;                 // affine-shaped 4x4 windows seen (either orientation)
    uint64_t shadow_live_latches = 0;             // live cascade transforms captured (cumulative)
    uint64_t shadow_live_cascades = 0;            // cascade table size in the last completed frame
    uint64_t shadow_srv_failed = 0;               // atlas SRV creation failures
    uint64_t live_rej_ortho = 0;                  // live latch rejections, by filter
    uint64_t live_rej_scale = 0;
    uint64_t live_rej_iso = 0;
    uint64_t live_rej_ratio = 0;
    uint64_t live_rej_trans = 0;
    uint64_t live_accepts = 0;
    uint64_t shadow_copies = 0;                   // fine-copy executions
    uint64_t commits_large = 0;                   // cycle commits won by a >=512B CB
    uint64_t commits_small = 0;                   // ... by a smaller CB
    uint64_t same_size_conflicts = 0;             // same-size upload with a DIFFERENT matrix
                                                  // arrived after the pending one in a cycle
    uint64_t sun_distinct_translations_max = 0;   // max distinct candidate translations in one cycle
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
    float4 lighting0;    // x = lit flag, y = sun-occlusion factor (0 shadowed .. 1 lit), z = ambient, w = diffuse
    float4 lighting1;    // xyz = unit vector TOWARD the sun/moon (engine axes), w = lighting valid flag
    float4 lighting2;    // rgb = light color (max-component normalized), w = shadow-map strength
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size (px)
    float4 shadowTiles[8];   // per-cascade atlas rect in UV: x0, y0, x1, y1
    float4 shadowMats[24];   // per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t]
    float4 shadowMeta2;      // x = debug mode (0 off, 1 occlusion viz, 2 cascade viz);
                             // yzw = the table's OWN camera origin (engine axes)
    float4 shadowSrc;        // x = table index sampling the FINE COPY texture (t2), -1 = none;
                             // yzw = (du, dv, dz) correction added to the copy entry's uvz
    float4 shadowFix2;       // uv ROTATION for the copy entry (spun-fit correction):
                             // x = cos, y = sin, zw = pivot (u, v). Depth is along the
                             // light axis and invariant under light-axis spin.
    // View-paired bands: the engine's decoded receiver contract.
    // bandView rows are the view matrix COLUMNS (world->view, row-vector
    // convention): viewPos.k = dot(float4(wpos,1), bandView[slot*3+k]).
    // bandMat rows are PSC_ShadowmapMatrix rows: uvz.k = dot(float4(vp,1), row).
    float4 bandMat[24];
    float4 bandView[24];
    float4 bandBorder[8];    // x = near, y = far, z = fade, w = 0 invalid / 1+texIndex
    // Cast pass: draw the boxes depth-only INTO the engine's shadow atlas
    // at each cascade pass end, so the world receives THEIR shadows.
    float4 castMat[3];       // this cascade's sampling matrix rows
    float4 castView[3];      // matching view columns
    float4 castVp;           // atlas-tile viewport: x, y, w, h (pixels)
    float4 castMeta;         // x = atlas size, y = enable
    float4 maskMeta;         // x = enable, y = channel (0-3), z = invert, w = unused
    float4 maskDims;         // xy = mask w/h, zw = viewport w/h at injection
    float4 fogParams;        // x = fogValue, y = fogDecay, z = fogBase, w = enable
    float4 fogColor;         // rgb = fog color, w = camera altitude ASL
};

// Solid-box band / local-volume mask (PSMain and PSComposite): the same
// semantics as the effect-path masks, evaluated on the BOX FRAGMENT's own
// world position (for solid boxes fxParams0.xyz carries the camera).
// Returns an intensity multiplier in [0, 1]; 1 when neither mask is set.
// NOTE: solid boxes previously IGNORED these masks on every path - the
// mask logic lived only in PSEffect, which the flush selects strictly for
// effect > 0. This closes that silent gap for both render paths at once.
float SolidMask(float3 wpos)
{
    float m = 1.0f;

    if (bandParams.w > 0.5f)
    {
        float d = distance(wpos, fxParams0.xyz);
        float fall = max(bandParams.z, 0.01f);
        float bm = smoothstep(bandParams.x - fall, bandParams.x, d);
        if (bandParams.y > 0.0f)
            bm *= 1.0f - smoothstep(bandParams.y, bandParams.y + fall, d);
        m *= bm;
    }

    if (localParams1.y > 0.5f)
    {
        float3 nd3 = abs(wpos - localParams0.xyz) / max(localRadii.xyz, 0.01f);
        float nd = (localParams0.w > 0.5f)
                 ? max(nd3.x, max(nd3.y, nd3.z))   // cube (Chebyshev)
                 : length(nd3);                    // sphere/ellipsoid
        m *= 1.0f - smoothstep(1.0f, 1.0f + max(localParams1.x, 0.001f), nd);
    }

    return m;
}

// Sun/moon shading for solid boxes (PSMain and PSComposite), opt-in per
// object via lighting0.x. Face normals come from screen-space derivatives
// of the interpolated world position - mesh-agnostic, so this carries over
// unchanged when the unit cube grows into arbitrary meshes - oriented
// outward via the camera in fxParams0.xyz (both solid paths always fill it).
// N.L on a convex shape is also the correct SELF-shading term; true cast
// self-shadows arrive with the shadow-map capture (see the recon section).
// lighting0.y is the world-occlusion factor (checkVisibility toward the
// sun, measured on the game thread): it attenuates ONLY the direct term,
// so a box standing in a building's shadow drops to its ambient level the
// way the world around it does.
// smf: per-pixel shadow-map factor from the caller (1.0 where the path has
// no atlas access). Combined with the object-level ray factor by min - they
// answer the same question at different granularities and must not stack.
float3 ApplyLighting(float3 base, float3 wpos, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    // Analytic axis-aligned box normal: exact and noise-free, unlike
    // ddx/ddy of kilometer-scale world positions (float32 steps there are
    // centimeter-sized, which reads as per-pixel shading noise).
    float3 rel = (wpos - centerSize.xyz) / max(sizeAxes.xyz * 0.5f, 1e-4f);
    float3 a = abs(rel);
    float3 n;
    if (a.x >= a.y && a.x >= a.z)      n = float3(rel.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
    else if (a.y >= a.z)               n = float3(0.0f, rel.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
    else                               n = float3(0.0f, 0.0f, rel.z >= 0.0f ? 1.0f : -1.0f);
    float ndl = saturate(dot(n, lighting1.xyz));
    float shadow = min(saturate(lighting0.y), smf);
    float3 direct = lighting2.rgb * (ndl * lighting0.w * shadow);
    return base * (lighting0.z + direct);
}

struct VSIn  { float3 pos : POSITION; };
struct VSOut { float4 pos : SV_Position; float3 wpos : TEXCOORD0; };
)HLSL";

// Static entry points (no depth access): compiled once
static const char* g_hlsl_static = R"HLSL(
VSOut VSMain(VSIn i)
{
    VSOut o;
    float3 wp = centerSize.xyz + i.pos * sizeAxes.xyz;
    o.pos = mul(float4(wp, 1.0f), viewProj);
    o.wpos = wp;
    return o;
}

// Analytic mask cast: per-pixel ray-vs-box toward the sun, drawn into
// the engine's screen-space shadow mask with multiply blending.
// castMat[0..2] = view rows 0..2; castView[0] = view row 3 (translation);
// castView[1] = (fov.x, fov.y, maskW, maskH); castView[2] = sunDir.xyz +
// strength in w. Scene linear depth at t0.
Texture2D<float> sceneDepthTex : register(t0);

float4 PSMaskCast(VSOut i) : SV_Target
{
    int3 px = int3(int2(i.pos.xy), 0);
    float zl = sceneDepthTex.Load(px);
    float2 dims = float2(castView[1].z, castView[1].w);
    float2 ndc = float2(i.pos.x / dims.x * 2.0f - 1.0f, 1.0f - i.pos.y / dims.y * 2.0f);
    float3 vp = float3(ndc.x * castView[1].x, ndc.y * castView[1].y, 1.0f) * zl;
    float3 q = vp - castView[0].xyz;
    float3 pw;
    pw.x = q.x * castMat[0].x + q.y * castMat[0].y + q.z * castMat[0].z;
    pw.y = q.x * castMat[1].x + q.y * castMat[1].y + q.z * castMat[1].z;
    pw.z = q.x * castMat[2].x + q.y * castMat[2].y + q.z * castMat[2].z;
    float3 sd = castView[2].xyz;
    float3 sds = float3(
        abs(sd.x) > 1e-6f ? sd.x : 1e-6f,
        abs(sd.y) > 1e-6f ? sd.y : 1e-6f,
        abs(sd.z) > 1e-6f ? sd.z : 1e-6f);
    float3 inv = 1.0f / sds;
    float3 bmin = centerSize.xyz - sizeAxes.xyz * 0.5f;
    float3 bmax = centerSize.xyz + sizeAxes.xyz * 0.5f;
    float3 ta = (bmin - pw) * inv;
    float3 tb = (bmax - pw) * inv;
    float3 tmn = min(ta, tb);
    float3 tmx = max(ta, tb);
    float tn = max(max(tmn.x, tmn.y), tmn.z);
    float tf = min(min(tmx.x, tmx.y), tmx.z);

    // LOCALITY CLAMP: a ground point can only be shadowed by a box
    // within plausible shadow reach of it. Boxes beyond the captured
    // depth's valid range reconstruct GARBAGE world positions at
    // far-plane pixels; the slab test then hit pseudo-randomly across
    // the whole screen and MIN-darkened the world's mask wholesale (the
    // 'overcast world' when the distant fog array spawned). Reach =
    // a few box diagonals stretched by sun elevation (low sun = long
    // shadows); beyond it, geometrically impossible - never darken.
    float boxR = length(sizeAxes.xyz) * 0.5f;
    float reach = boxR * (2.0f + 3.0f / max(abs(castView[2].y), 0.15f));
    float3 toBox = centerSize.xyz - pw;
    float near_enough = (dot(toBox, toBox) < reach * reach) ? 1.0f : 0.0f;
    float hit = (tf >= max(tn, 0.05f) && zl > 0.01f && near_enough > 0.5f) ? 1.0f : 0.0f;

    // Screen path only (castMeta.x = 1): if the CAMERA's ray to this
    // pixel passes through the box first, the pixel shows the box face -
    // the scene-end apply runs after the boxes draw, so shading it would
    // paint the ground's shadow ONTO the box (the see-through bug).
    if (castMeta.x > 0.5f && hit > 0.5f) {
        float3 cq = -castView[0].xyz;
        float3 cam;
        cam.x = cq.x * castMat[0].x + cq.y * castMat[0].y + cq.z * castMat[0].z;
        cam.y = cq.x * castMat[1].x + cq.y * castMat[1].y + cq.z * castMat[1].z;
        cam.z = cq.x * castMat[2].x + cq.y * castMat[2].y + cq.z * castMat[2].z;
        float3 vd = pw - cam;
        float3 vds = float3(
            abs(vd.x) > 1e-6f ? vd.x : 1e-6f,
            abs(vd.y) > 1e-6f ? vd.y : 1e-6f,
            abs(vd.z) > 1e-6f ? vd.z : 1e-6f);
        float3 vinv = 1.0f / vds;
        float3 va = (bmin - cam) * vinv;
        float3 vb = (bmax - cam) * vinv;
        float3 vmn = min(va, vb);
        float3 vmx = max(va, vb);
        float vtn = max(max(vmn.x, vmn.y), vmn.z);
        float vtf = min(min(vmx.x, vmx.y), vmx.z);
        if (vtf >= max(vtn, 0.0f) && vtn < 0.99f) hit = 0.0f;   // box face in front
    }

    // Full occlusion here; the APPLY colors the shadow by the ambient
    // floor (amb/(amb+sun)) - depth and hue both derive from the sky.
    float shade = 1.0f - hit * saturate(castView[2].w);
    return float4(shade, shade, shade, 1.0f);
}

// Composite our shadow-factor texture onto the scene (multiply).
Texture2D<float> khShadowFactor : register(t10);

float4 PSMaskApply(VSOut i) : SV_Target
{
    float v = khShadowFactor.Load(int3(int2(i.pos.xy), 0));
    // v = 1 lit, 0 fully occluded. Shadow tint: lit = ambient + sun,
    // shadowed = ambient, so the per-channel floor is amb/(amb+sun)
    // (precomputed on the CPU into maskMeta.xyz). The factor texture
    // interpolates toward that floor instead of black.
    float3 floorRGB = maskMeta.xyz;
    float3 outRGB = lerp(floorRGB, float3(1.0f, 1.0f, 1.0f), v);
    return float4(outRGB, 1.0f);
}

// Fullscreen triangle from SV_VertexID: no vertex buffer, no input layout
VSOut VSFullscreen(uint vid : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    o.wpos = float3(0.0f, 0.0f, 0.0f);
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    int bm = (int)sizeAxes.w;
    // Intensity (color.a, which carries the lifetime envelope) applies to
    // every blend mode; the band/local mask folds into it. Lighten/darken
    // cannot express intensity through hardware blend factors (MAX/MIN
    // ignore alpha), so they fade toward their respective neutral element
    // instead: black for MAX, a large value for MIN - both leave the
    // scene untouched.
    float3 lc = ApplyLighting(color.rgb, i.wpos, 1.0f);
    float a = color.a * SolidMask(i.wpos);
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);
    return float4(lc, a);
}
)HLSL";

// Injected-path shaders (the HYBRID): identical box transform and blend
// packing, plus a per-pixel punch-through guard. The fragment's TRUE
// camera distance (world-space, matrix-independent) is compared against
// the scene distance reconstructed from a mid-frame copy of the depth
// buffer; a fragment CLEARLY behind opaque geometry - beyond a margin
// generous enough to absorb any near-plane/coefficient uncertainty - is
// discarded. Hardware depth (with the shimmer bias) still resolves the
// marginal ties; the guard only vetoes the gross violations that show up
// as the box punching through walls. Compiled per depth MSAA count.
static const char* g_hlsl_composite = R"HLSL(
#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float LoadDepthC(int2 px) { return depthTex.Load(px, 0); }
#else
Texture2D<float> depthTex : register(t0);
float LoadDepthC(int2 px) { return depthTex.Load(int3(px, 0)); }
#endif

// The engine's shadow atlas (the depth texture its cascade passes render
// into), sampled with the engine's OWN per-cascade world->atlasUV+depth
// transforms harvested from its constant uploads - so this compare is the
// same one the engine performs for its own geometry.
Texture2D<float> shadowAtlas : register(t1);
// Our OWN copy of the atlas, grabbed the moment the FINEST cascade finished
// rendering - the engine reuses one map for all cascades sequentially, so
// the fine content only exists in it transiently. The copy plus its stored
// transform is a permanently correct pair.
Texture2D<float> shadowAtlasFine : register(t2);
// The engine's screen-space shadow mask (its shadow resolve output). Read
// at this pixel's own screen position: registration-exact by construction.
Texture2D<float4> shadowMask : register(t3);
Texture2D<float> shadowBand0 : register(t4);
Texture2D<float> shadowBand1 : register(t5);
Texture2D<float> shadowBand2 : register(t6);
Texture2D<float> shadowBand3 : register(t7);
Texture2D<float> shadowBand4 : register(t8);
Texture2D<float> shadowBand5 : register(t9);
Texture2D<float> shadowBand6 : register(t12);   // slots 6-7: t10 is the
Texture2D<float> shadowBand7 : register(t13);   // screen path's factor

// rel: CAMERA-RELATIVE position (wpos - fxParams0.xyz). The engine renders
// camera-relative for float precision, and its shadow sampling transforms
// consume that same space - evaluated at absolute world coordinates they
// produce UVs off by tens of atlas widths (which is exactly how this was
// diagnosed: at the camera the transform yields its own translation, dead
// center in a tile).
// Core sample: which cascade contains this camera-relative point, and is
// it occluded there. cascade -1 = no cascade contains the point.
void ShadowMapSample(float3 rel, out int cascade, out float occluded)
{
    cascade = -1;
    occluded = 0.0f;
    int n = (int)shadowMeta.x;
    if (n <= 0) return;

    for (int c = 0; c < n; ++c) {
        float4 r0 = shadowMats[c * 3 + 0];
        float4 r1 = shadowMats[c * 3 + 1];
        float4 r2 = shadowMats[c * 3 + 2];
        float u = dot(r0.xyz, rel) + r0.w;
        float v = dot(r1.xyz, rel) + r1.w;
        float z = dot(r2.xyz, rel) + r2.w;

        if (c == (int)shadowSrc.x) {
            float ru = u - shadowFix2.z;
            float rv = v - shadowFix2.w;
            u = shadowFix2.z + shadowFix2.x * ru - shadowFix2.y * rv;
            v = shadowFix2.w + shadowFix2.y * ru + shadowFix2.x * rv;
            u += shadowSrc.y;
            v += shadowSrc.z;
            z += shadowSrc.w;
        }

        float4 t = shadowTiles[c];
        // Tiles are sorted finest-first; the first cascade whose tile and
        // depth range contain the point decides.
        if (u < t.x || u > t.z || v < t.y || v > t.w) continue;
        if (z <= 0.001f || z >= 0.999f) continue;
        int3 px = int3(int2(float2(u, v) * shadowMeta.w), 0);
        float mapZ = (c == (int)shadowSrc.x) ? shadowAtlasFine.Load(px) : shadowAtlas.Load(px);
        cascade = c;
        occluded = ((z - mapZ) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        return;
    }
}

float ShadowMapFactor(float3 rel)
{
    int cascade;
    float occluded;
    ShadowMapSample(rel, cascade, occluded);
    return 1.0f - occluded * saturate(lighting2.w);
}

// The engine's receiver contract, replicated verbatim from its resolve
// shader: view-space position, band selection by view depth against
// PSC_ShadowmapLayerBorder, PSC_ShadowmapMatrix as three dot products
// with w = 1, comparison against stored depth. Each band's view matrix
// was frozen WITH its matrix and content, so rotation is exact.
float ShadowBandFactor(float3 wpos)
{
    // Straight-line evaluation (no [unroll]/continue/break: X4575 in
    // this unit). Slots arrive finest-first; the first containing band
    // wins; bandBorder.w-1 names the physical texture.
    float4 p = float4(wpos, 1.0f);
    float occ = -1.0f;
    int done = 0;

    for (int s = 0; s < 8; ++s) {
        if (done != 0) break;
        if (bandBorder[s].w < 0.5f) { done = 0; }
        else {
            float vz = dot(p, bandView[s * 3 + 2]);

            if (vz >= bandBorder[s].x && vz < bandBorder[s].y) {
                float4 vp4 = float4(dot(p, bandView[s * 3 + 0]), dot(p, bandView[s * 3 + 1]), vz, 1.0f);
                float u = dot(vp4, bandMat[s * 3 + 0]);
                float v = dot(vp4, bandMat[s * 3 + 1]);
                float z = dot(vp4, bandMat[s * 3 + 2]);

                if (u > 0.001f && u < 0.999f && v > 0.001f && v < 0.999f && z > 0.001f && z < 0.999f) {
                    int t = (int)(bandBorder[s].w + 0.5f) - 1;   // w = 1 + texIndex
                    // Rotated-poisson PCF, the engine's own recipe: a
                    // per-pixel random disk rotation turns fixed-offset
                    // banding into fine noise. Hash from WORLD position so
                    // the pattern doesn't swim with the camera.
                    float ang = frac(sin(dot(wpos.xz, float2(12.9898f, 78.233f))) * 43758.5469f) * 6.2831853f;
                    float ca = cos(ang);
                    float sa = sin(ang);
                    float r = 2.0f;
                    float2 base = float2(u, v) * shadowMeta.w;
                    float acc = 0.0f;

                    for (int k = 0; k < 8; ++k) {
                        float2 d0 =
                            (k == 0) ? float2( 0.974844f, 0.756484f)
                          : (k == 1) ? float2(-0.814100f, 0.914376f)
                          : (k == 2) ? float2( 0.945586f,-0.768907f)
                          : (k == 3) ? float2(-0.815442f,-0.879125f)
                          : (k == 4) ? float2( 0.443233f,-0.975116f)
                          : (k == 5) ? float2(-0.241888f, 0.997065f)
                          : (k == 6) ? float2(-0.915886f, 0.457714f)
                                     : float2( 0.143832f,-0.141008f);
                        float2 off = float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca);
                        int3 px = int3(int2(base + off * r + 0.5f), 0);
                        float stored = (t == 0) ? shadowBand0.Load(px)
                                     : (t == 1) ? shadowBand1.Load(px)
                                     : (t == 2) ? shadowBand2.Load(px)
                                     : (t == 3) ? shadowBand3.Load(px)
                                     : (t == 4) ? shadowBand4.Load(px)
                                     : (t == 5) ? shadowBand5.Load(px)
                                     : (t == 6) ? shadowBand6.Load(px)
                                                : shadowBand7.Load(px);
                        acc += ((z - stored) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
                    }

                    occ = acc * 0.125f;
                    done = 1;
                }
            }
        }
    }

    if (occ < 0.0f) return 1.0f;   // no band covers this depth: lit
    return 1.0f - occ * saturate(lighting2.w);
}

float ShadowMaskValue(float4 svpos)
{
    float2 scale = maskDims.xy / max(maskDims.zw, float2(1.0f, 1.0f));
    int2 px = int2(svpos.xy * scale);
    float4 m = shadowMask.Load(int3(px, 0));
    float v = m.x;
    if (maskMeta.y > 0.5f && maskMeta.y < 1.5f) v = m.y;
    else if (maskMeta.y > 1.5f && maskMeta.y < 2.5f) v = m.z;
    else if (maskMeta.y > 2.5f) v = m.w;
    if (maskMeta.z > 0.5f) v = 1.0f - v;
    return v;
}

float ShadowMaskFactor(float4 svpos)
{
    float v = ShadowMaskValue(svpos);
    return 1.0f - (1.0f - v) * saturate(lighting2.w);
}

// Debug palettes: mode 1 paints occlusion (red = occluded, green = lit,
// blue = no cascade contains the pixel); mode 2 paints the selected
// cascade index (gray = none). Both replace the box color outright - the
// point is reading the shadow path's decisions off the geometry.
float4 ShadowMapDebug(float3 rel)
{
    int cascade;
    float occluded;
    ShadowMapSample(rel, cascade, occluded);

    if (shadowMeta2.x < 1.5f) {
        if (cascade < 0) return float4(0.1f, 0.2f, 1.0f, 1.0f);
        return float4(occluded, 1.0f - occluded, 0.0f, 1.0f);
    }

    if (cascade < 0) return float4(0.4f, 0.4f, 0.4f, 1.0f);
    float3 pal[8] = {
        float3(1, 0, 0),   float3(1, 0.5f, 0), float3(1, 1, 0), float3(0, 1, 0),
        float3(0, 1, 1),   float3(0, 0.4f, 1), float3(0.6f, 0, 1), float3(1, 0, 1)
    };
    return float4(pal[cascade & 7], 1.0f);
}

float LinDepthC(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    if (denom > -1e-7f) return 1e9f;   // at/beyond far plane (sky/clear): very far
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

struct VSOutC { float4 pos : SV_Position; float3 wpos : TEXCOORD0; };

VSOutC VSComposite(VSIn i)
{
    VSOutC o;
    float3 wp = centerSize.xyz + i.pos * sizeAxes.xyz;
    o.pos = mul(float4(wp, 1.0f), viewProj);
    o.wpos = wp;
    return o;
}

float4 PSComposite(VSOutC i) : SV_Target
{
    // Punch-through guard: fxParams0.xyz = camera (engine space),
    // fxParams1.x = base margin (m), fxParams1.y = relative margin.
    int2 px = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
    // Same-metric compare: LinDepthC returns VIEW-AXIS scene depth, and
    // SV_Position.w is this fragment's own view-axis depth (clip-space w).
    // The old compare put Euclidean camera distance against view-axis
    // depth, overstating the fragment by 1/cos(view angle) - up to tens of
    // percent at screen edges - so edge fragments sitting close in front
    // of world geometry were wrongly discarded (world bleeding through
    // with no flicker: a guard discard, not a depth-test loss).
    float sceneZ = LinDepthC(LoadDepthC(px));
    float fragZ = i.pos.w;
    if (fragZ > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;

    int bm = (int)sizeAxes.w;

    if (lighting0.x >= 0.5f && shadowMeta2.x >= 2.5f && maskMeta.x >= 0.5f) {
        float mv = ShadowMaskValue(i.pos);
        return float4(mv, mv, mv, 1.0f);   // debug 3: raw mask grayscale
    }

    if (lighting0.x >= 0.5f && shadowMeta2.x >= 3.5f && maskMeta.w >= 0.5f) {
        // debug 4: which band covers this pixel (green = 0, blue = 1, red = none)
        // Straight-line: [unroll] + continue + return-of-loop-var trips
        // fxc's flow analysis (X4575 uninitialized read).
        float4 p4 = float4(i.wpos, 1.0f);
        float4 dbg = float4(0.9f, 0.1f, 0.1f, 1.0f);   // red = uncovered

        for (int s = 7; s >= 0; --s) {
            if (bandBorder[s].w >= 0.5f) {
                float vz = dot(p4, bandView[s * 3 + 2]);

                if (vz >= bandBorder[s].x && vz < bandBorder[s].y) {
                    dbg = (s == 0) ? float4(0.1f, 0.8f, 0.1f, 1.0f)   // green: finest
                        : (s == 1) ? float4(0.1f, 0.1f, 0.9f, 1.0f)   // blue
                        : (s == 2) ? float4(0.8f, 0.8f, 0.1f, 1.0f)   // yellow
                        : (s == 3) ? float4(0.8f, 0.1f, 0.8f, 1.0f)   // magenta
                        : (s == 4) ? float4(0.1f, 0.8f, 0.8f, 1.0f)   // cyan
                                   : float4(0.9f, 0.5f, 0.1f, 1.0f);  // orange
                }
            }
        }

        return dbg;
    }

    if (lighting0.x >= 0.5f && shadowMeta2.x >= 0.5f && shadowMeta2.x < 2.5f) return ShadowMapDebug(i.wpos - shadowMeta2.yzw);
    float smf = 1.0f;

    if (lighting0.x >= 0.5f) {
        if (maskMeta.x >= 0.5f)      smf = ShadowMaskFactor(i.pos);
        else if (maskMeta.w >= 0.5f) smf = ShadowBandFactor(i.wpos);
        else                         smf = ShadowMapFactor(i.wpos - shadowMeta2.yzw);

    }
    float3 lc = ApplyLighting(color.rgb, i.wpos, smf);

    // FOG OCCLUSION: RV's height-decay exponential. Density at height h:
    // d(h) = fogValue * exp(-fogDecay * (h - fogBase)). Transmittance over
    // the ray uses the density at the FRAGMENT's height (single-sample
    // approximation - exact for level sight lines, visually right for
    // slanted ones), distance from the camera in meters.
    // FOG OCCLUSION (final form; the red-tracer run proved the chain).
    // RV semantics: full density at and below fogBase, exponential decay
    // above it at fogDecay per meter - the original bug attenuated BELOW
    // base too (fogStagedBase=111 m with terrain lower killed nothing,
    // but earlier defaults with base 0 gutted the density at any
    // altitude). max(hgt - base, 0) keeps full density below base.
    // Extinction calibrated to RV visibility: fog 1.0 = ~90% obscured at
    // 150 m, fog 0.5 = at ~600 m (lambda = dens^2 * 0.0153 / m).
    if (fogParams.w >= 0.5f) {
        float distM = i.pos.w;
        float hgt = i.wpos.y;
        float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
        float trans = exp(-distM * dens * dens * 0.0153f);
        lc = lerp(fogColor.rgb, lc, saturate(trans));
    }
    float a = color.a * SolidMask(i.wpos);
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);
    return float4(lc, a);
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
)HLSL"
    R"HLSL(    else if (effect == 15)    // Anamorphic streak: [threshold, intensity, lengthPx, falloffPow] + [vertical 0/1]; color = tint
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

        else if (effect == 17)    // Glitch: [intensity, speed, sliceAmountPx, sliceBands] + [colorSplitPx, blockAmount, noiseAmount, burstiness]
    {
        // Digital damage: horizontal slice tears, block corruption, RGB
        // split, and interference noise, driven by a burst gate so the
        // glitches strike intermittently rather than boiling constantly.
        // burstiness 0 = continuous corruption, 1 = calm with hard spikes.
        // Runs until the pass is removed; animate params (intensity ramp-
        // down) for a "system recovering" feel. color.rgb tints the static.
        float speed = max(fxParams0.y, 0.1f);
        float tf = floor(t * speed);

        float burst = Hash(float2(tf * 0.0131f, 7.31f));
        burst = pow(burst, 1.0f + fxParams1.w * 6.0f);
        float drive = lerp(1.0f, burst, saturate(fxParams1.w)) * fxParams0.x;

        float2 suv = i.pos.xy;

        // Horizontal slice displacement: few large tears, many small ones
        float bands = max(fxParams0.w, 1.0f);
        float band = floor(uv.y * bands);
        float bh = (Hash(float2(band, tf)) - 0.5f) * 2.0f;
        bh = sign(bh) * pow(abs(bh), 3.0f);
        suv.x += bh * fxParams0.z * drive;

        // Block corruption: coarse grid cells randomly displaced
        if (fxParams1.y > 0.001f)
        {
            float2 cell = floor(uv * float2(24.0f, 14.0f));
            float ch = Hash(cell + tf * 1.7f);
            if (ch > 1.0f - 0.15f * saturate(fxParams1.y) * saturate(drive))
            {
                float2 off = (float2(Hash(cell + 3.1f + tf), Hash(cell + 5.7f + tf)) - 0.5f) * 80.0f * drive;
                suv += off;
            }
        }

        // RGB channel split along the tear axis
        float split = fxParams1.x * drive;
        float3 col;
        col.r = SampleScene(int2(suv + float2(split, 0.0f))).r;
        col.g = SampleScene(int2(suv)).g;
        col.b = SampleScene(int2(suv - float2(split, 0.0f))).b;

        // Interference lines + static
        if (fxParams1.z > 0.001f)
        {
            float ln = Hash(float2(floor(i.pos.y * 0.5f), tf * 2.3f));
            float lineHit = step(1.0f - 0.2f * saturate(fxParams1.z) * saturate(drive), ln);
            float n = Hash(i.pos.xy * 0.37f + tf * 13.7f);
            col = lerp(col, color.rgb * n, lineHit * 0.85f);
            col += (n - 0.5f) * fxParams1.z * drive * 0.35f;
        }

        outc = max(col, 0.0f);
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
    float lighting0[4];    // x = lit flag, y = sun-occlusion factor, z = ambient, w = diffuse
    float lighting1[4];    // xyz = unit vector TOWARD the light (engine axes), w = valid flag
    float lighting2[4];    // rgb = light color (max-component normalized), w = shadow-map strength
    float shadow_meta[4];      // count, compare sign, bias, atlas size
    float shadow_tiles[8][4];  // per-cascade atlas UV rects
    float shadow_mats[24][4];  // per-cascade 4x3 sampling transforms
    float shadow_meta2[4];     // x = debug mode
    float shadow_src[4];       // x = fine-copy entry index or -1
    float shadow_fix2[4];
    float band_mat[24][4];
    float band_view[24][4];
    float band_border[8][4];
    float cast_mat[3][4];
    float cast_view[3][4];
    float cast_vp[4];
    float cast_meta[4];
    float mask_meta[4];
    float mask_dims[4];
    // --- appended (fog + engine-mask receive); append-only so no
    // existing register shifts ---
    float fog_params[4];   // x = fogValue, y = fogDecay, z = fogBase, w = enable
    float fog_color[4];    // rgb = fog color approximation, w = camera altitude ASL
                           // (relative), z/w = mask w/h for texel math
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

    {   // analytic mask cast PS: non-fatal
        ID3DBlob* mc_blob = nullptr;
        const std::string mc_err = compile_shader(static_src.c_str(), "PSMaskCast", "ps_5_0", nullptr, &mc_blob);

        if (mc_err.empty() && mc_blob) {
            dev->CreatePixelShader(mc_blob->GetBufferPointer(), mc_blob->GetBufferSize(), nullptr, &g_res.ps_maskcast);
            mc_blob->Release();
        } else if (!mc_err.empty()) {
            report_error("KH maskcast shader: " + mc_err);
        }
    }

    {   // apply PS: non-fatal
        ID3DBlob* ma_blob = nullptr;
        const std::string ma_err = compile_shader(static_src.c_str(), "PSMaskApply", "ps_5_0", nullptr, &ma_blob);

        if (ma_err.empty() && ma_blob) {
            dev->CreatePixelShader(ma_blob->GetBufferPointer(), ma_blob->GetBufferSize(), nullptr, &g_res.ps_maskapply);
            ma_blob->Release();
        } else if (!ma_err.empty()) {
            report_error("KH maskapply shader: " + ma_err);
        }
    }
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
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.composite_cb);
        if (FAILED(hr)) { g_res.release(); return "Create composite CB " + hr_str(hr); }
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
        // -1.0 overcorrected at grazing angles (box faces near edge-on to
        // the camera): the slope-scaled offset gets large and varies per
        // pixel, so intersection contours with world geometry crawled.
        // The constant -32 term carries the anti-shimmer duty; keep only
        // a light slope component. If intersections still shimmer under
        // camera motion, prefer deepening DepthBias (e.g. -48) over
        // raising this again.
        rd.SlopeScaledDepthBias = -0.25f;
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

// Injected-path shaders, compiled for the current depth MSAA count
// (mirrors ensure_effect_shader).
static uint64_t g_comp_compiles = 0;        // full compile attempts (diagnostic)
static uint64_t g_comp_fail_streak = 0;     // consecutive failures
static float    g_comp_next_retry = 0.0f;   // backoff: a failed compile must
                                            // NOT retry per injection - a
                                            // ~170 ms D3DCompile every frame
                                            // IS a framerate collapse, and
                                            // the silent guard=false fallback
                                            // removes shadow shading too.
static std::string g_comp_last_err;

inline std::string ensure_composite_shader(ID3D11Device* dev) {
    if (g_res.vs_composite && g_res.ps_composite &&
        g_res.ps_composite_samples == g_res.comp_depth_samples) {
        return "";
    }

    // self-contained clock: effect_time_seconds() is defined later in the file
    LARGE_INTEGER _bo_q, _bo_f;
    QueryPerformanceCounter(&_bo_q);
    QueryPerformanceFrequency(&_bo_f);
    const float now_bo = static_cast<float>(static_cast<double>(_bo_q.QuadPart) / static_cast<double>(_bo_f.QuadPart));
    if (g_comp_fail_streak > 0 && now_bo < g_comp_next_retry) return g_comp_last_err;
    g_comp_compiles++;

    if (g_res.vs_composite) { g_res.vs_composite->Release(); g_res.vs_composite = nullptr; }
    if (g_res.ps_composite) { g_res.ps_composite->Release(); g_res.ps_composite = nullptr; }

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { nullptr, nullptr },
    };

    const std::string comp_src = std::string(g_cb_hlsl) + g_hlsl_composite;
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    auto comp_fail = [&](const std::string& e) {
        g_comp_last_err = e;
        if (g_comp_fail_streak == 0) report_error("KH composite shader: " + e);
        g_comp_fail_streak++;
        g_comp_next_retry = now_bo + 2.0f;
        g_stats.effect_setup_fails++;
        return e;
    };

    std::string err = compile_shader(comp_src.c_str(), "VSComposite", "vs_5_0", defines, &vs_blob);
    if (!err.empty()) return comp_fail("composite VS: " + err);
    err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", defines, &ps_blob);
    if (!err.empty()) { vs_blob->Release(); return comp_fail("composite PS: " + err); }
    g_comp_fail_streak = 0;
    HRESULT hr = dev->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_res.vs_composite);
    vs_blob->Release();
    if (FAILED(hr)) { ps_blob->Release(); return "Create composite VS " + hr_str(hr); }
    hr = dev->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_res.ps_composite);
    ps_blob->Release();
    if (FAILED(hr)) return "Create composite PS " + hr_str(hr);
    g_res.ps_composite_samples = g_res.comp_depth_samples;
    return "";
}

// Copies the CURRENT (mid-frame, opaques-complete) scene depth into an
// owned texture and keeps an SRV over the copy. The live depth is bound as
// a writable DSV during injection, so sampling it directly is a bind
// hazard; copying while bound follows the same pattern as the production
// scene-color capture. Returns false when unavailable (the injection then
// runs without the guard).

inline float effect_time_seconds();   // defined below; the snapshot
                                      // timestamp at the depth copy needs it
inline bool ensure_composite_depth(ID3D11Device* dev, ID3D11DeviceContext* ctx) {
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    if (!dsv) return false;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    dsv->Release();
    if (!res) return false;
    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (FAILED(hr) || !tex) return false;
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);

    const bool matches = g_res.comp_depth_tex &&
        g_res.comp_depth_identity == static_cast<void*>(tex) &&
        g_res.comp_depth_w == td.Width && g_res.comp_depth_h == td.Height &&
        g_res.comp_depth_samples == td.SampleDesc.Count;

    if (!matches) {
        if (g_res.comp_depth_srv) { g_res.comp_depth_srv->Release(); g_res.comp_depth_srv = nullptr; }
        if (g_res.comp_depth_tex) { g_res.comp_depth_tex->Release(); g_res.comp_depth_tex = nullptr; }
        DXGI_FORMAT srv_fmt;

        switch (td.Format) {
            case DXGI_FORMAT_R24G8_TYPELESS:    srv_fmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
            case DXGI_FORMAT_R32_TYPELESS:      srv_fmt = DXGI_FORMAT_R32_FLOAT; break;
            case DXGI_FORMAT_R32G8X24_TYPELESS: srv_fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
            case DXGI_FORMAT_R16_TYPELESS:      srv_fmt = DXGI_FORMAT_R16_UNORM; break;
            default:
                tex->Release();
                return false;
        }

        D3D11_TEXTURE2D_DESC cd = td;
        cd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        cd.Usage = D3D11_USAGE_DEFAULT;
        cd.CPUAccessFlags = 0;
        cd.MiscFlags = 0;

        if (FAILED(dev->CreateTexture2D(&cd, nullptr, &g_res.comp_depth_tex))) {
            tex->Release();
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = srv_fmt;

        if (td.SampleDesc.Count > 1) {
            sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        } else {
            sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            sd.Texture2D.MipLevels = 1;
        }

        if (FAILED(dev->CreateShaderResourceView(g_res.comp_depth_tex, &sd, &g_res.comp_depth_srv))) {
            g_res.comp_depth_tex->Release();
            g_res.comp_depth_tex = nullptr;
            tex->Release();
            return false;
        }

        g_res.comp_depth_identity = static_cast<void*>(tex);
        g_res.comp_depth_w = td.Width;
        g_res.comp_depth_h = td.Height;
        g_res.comp_depth_samples = td.SampleDesc.Count;
    }

    ctx->CopyResource(g_res.comp_depth_tex, tex);
    g_res.comp_depth_time = effect_time_seconds();
    tex->Release();
    return true;
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
    ID3D11ShaderResourceView* ps_srvs[14] = {};   // t0-t13: bands t4-t9 + t12-t13
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
        ctx->PSGetShaderResources(0, 14, ps_srvs);
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
        ctx->PSSetShaderResources(0, 14, ps_srvs);
        ctx->OMSetDepthStencilState(dss, stencil_ref);
        ctx->OMSetBlendState(blend, blend_factor, sample_mask);
        ctx->RSSetState(rasterizer);
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
        KH_SAFE_RELEASE(ps_srvs[2]);
        KH_SAFE_RELEASE(ps_srvs[3]);
        KH_SAFE_RELEASE(ps_srvs[4]);
        KH_SAFE_RELEASE(ps_srvs[5]);
        KH_SAFE_RELEASE(ps_srvs[6]);
        KH_SAFE_RELEASE(ps_srvs[7]);
        KH_SAFE_RELEASE(ps_srvs[8]);
        KH_SAFE_RELEASE(ps_srvs[9]);
        KH_SAFE_RELEASE(ps_srvs[10]);
        KH_SAFE_RELEASE(ps_srvs[11]);
        KH_SAFE_RELEASE(ps_srvs[12]);
        KH_SAFE_RELEASE(ps_srvs[13]);
        KH_SAFE_RELEASE(dss);
        KH_SAFE_RELEASE(blend);
        KH_SAFE_RELEASE(rasterizer);
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
        KH_SAFE_RELEASE(cs);
        KH_SAFE_RELEASE(srvs[0]);
        KH_SAFE_RELEASE(srvs[1]);
        KH_SAFE_RELEASE(uav);
        KH_SAFE_RELEASE(cb0);
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) KH_SAFE_RELEASE(rtvs[i]);
        KH_SAFE_RELEASE(dsv);
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
    if (s == "glitch")     return 17;
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
        { 1.0f, 8.0f, 24.0f, 12.0f, 6.0f, 0.5f, 0.35f, 0.7f },  // 17 glitch: intensity, speed, sliceAmountPx, sliceBands, colorSplitPx, blockAmount, noiseAmount, burstiness
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

// Parses a duration designator into the object's lifetime fields:
// SCALAR seconds -> hold that long, no fades; ARRAY [fadeIn, hold, fadeOut].
// 0 / all-zero = infinite (default). Returns false on a malformed value.
inline bool parse_duration_gv(const game_value& gv, RenderObject& obj) {
    float fi = 0.0f, hd = 0.0f, fo = 0.0f;

    if (gv.type_enum() == game_data_type::SCALAR) {
        hd = static_cast<float>(gv);
    } else if (gv.type_enum() == game_data_type::ARRAY) {
        auto& a = gv.to_array();
        if (a.size() >= 1) fi = static_cast<float>(a[0]);
        if (a.size() >= 2) hd = static_cast<float>(a[1]);
        if (a.size() >= 3) fo = static_cast<float>(a[2]);
    } else {
        return false;
    }
    
    fi = fi < 0.0f ? 0.0f : fi;
    hd = hd < 0.0f ? 0.0f : hd;
    fo = fo < 0.0f ? 0.0f : fo;
    obj.fade_in = fi;
    obj.hold_dur = hd;
    obj.fade_out = fo;
    obj.timed = (fi + hd + fo) > 0.0f;
    return true;
}

// Lifetime envelope at time 'now': 0..1 intensity multiplier; sets 'expired'
// once the fade-out has completed.
inline float lifetime_envelope(const RenderObject& o, float now, bool& expired) {
    expired = false;
    if (!o.timed) return 1.0f;
    float age = now - o.birth_time;
    if (age < 0.0f) age = 0.0f;
    if (age < o.fade_in) return age / (o.fade_in > 0.001f ? o.fade_in : 0.001f);
    age -= o.fade_in;
    if (age < o.hold_dur) return 1.0f;
    age -= o.hold_dur;
    if (age < o.fade_out) return 1.0f - age / (o.fade_out > 0.001f ? o.fade_out : 0.001f);
    expired = true;
    return 0.0f;
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
// Reordered compositing: pre-translucent depth injection.
//
// The engine composites its translucents (particles, smoke, glass, water)
// by depth-testing them against whatever is in the depth buffer and alpha-
// blending over the color target. Nothing our post-scene flush draws can
// participate in that - which is why depth-tested boxes paint OVER smoke.
// The fix is ordering, not measurement: hook the immediate context's draw
// calls (MinHook, shared via framework's ensure_minhook), watch the output-
// merger state, and the moment the engine issues its FIRST translucent draw
// of the scene (alpha blending on, depth test on / write off, main scene
// depth bound), draw our eligible boxes right there with DEPTH WRITES.
// Every translucent the engine draws afterwards then composites against
// them natively: smoke in front blends over the box, smoke behind it is
// depth-rejected. Pixel-perfect, zero rays, zero estimation.
//
// Mechanics:
//  - State hooks (OMSetBlendState / OMSetDepthStencilState /
//    OMSetRenderTargets) maintain three booleans; classification of the
//    immutable state objects is memoized by pointer, so the per-draw hook
//    cost is a pointer compare and a few flag loads.
//  - ClearDepthStencilView on the main scene depth marks the new frame
//    (injection re-arms).
//  - Deferred contexts share the vtable: every hook first checks that the
//    calling context IS the tracked immediate context and passes straight
//    through otherwise. Injection recursion is excluded with a flag, so
//    our own state sets/draws never disturb the tracking.
//  - Eligibility: solid (effect 0), non-overlay, composite (default true)
//    boxes. Effect boxes need the mid-flush scene capture and overlays are
//    post-scene by definition - both stay on the flush path.
//  - Health: each injection stamps a timestamp; the flush only stands
//    down for eligible boxes while an injection happened within the last
//    half second. If the hook never engages (install failure,
//    unrecognized pipeline), boxes degrade to the flush path
//    automatically.
//  - Engine reset: a generation counter tells the render thread to drop
//    its memoized state-object classifications (pointers may be reused).
// ===========================================================================

static std::atomic<bool>     g_reorder_hook_active{false};
static bool                  g_reorder_hook_failed = false;      // game thread only
static std::atomic<void*>    g_reorder_target_ctx{nullptr};      // immediate context to act on
static std::atomic<uint64_t> g_composite_last_inject_ms{0};
static std::atomic<uint64_t> g_composite_inject_serial{0};

inline uint64_t steady_now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// The composited path counts as healthy while injections are actually
// happening; outside that window (menus, install failure, pipeline change)
// eligible boxes revert to the post-scene flush path.
inline bool composite_path_healthy() {
    const uint64_t last = g_composite_last_inject_ms.load(std::memory_order_relaxed);
    return last != 0 && (steady_now_ms() - last) < 500;
}

inline bool is_composite_eligible(const RenderObject& o) {
    return !o.fullscreen && o.effect == 0 && o.mode != DepthMode::Off;
}

// A genuine scene issues a large number of opaque draws between its depth
// clear and its first translucent; anomaly passes (late re-clears, preview
// renders, UI-phase blending) issue almost none. Requiring this many opaque
// main-scene draws before an injection can fire rejects those single-frame
// mistimings outright.
static constexpr uint32_t KH_REORDER_MIN_OPAQUE_DRAWS = 128;
// Punch-through guard margins: a fragment is discarded only when its true
// camera distance exceeds the reconstructed scene distance by base +
// relative * sceneDistance. The relative term must exceed the worst-case
// reconstruction error from near-plane coefficient uncertainty (d_est =
// d_true * n_used / n_true - relative, not absolute), so the guard can
// never falsely clip a box that is genuinely in front.
static constexpr float KH_COMPOSITE_GUARD_BASE = 0.5f;
static constexpr float KH_COMPOSITE_GUARD_REL = 0.25f;
// With MEASURED coefficients (projection sniffer) the reconstruction is
// exact up to precision, so the guard collapses to a tight backstop.
static constexpr float KH_COMPOSITE_GUARD_BASE_MEASURED = 0.25f;
static constexpr float KH_COMPOSITE_GUARD_REL_MEASURED = 0.02f;

// Injection attempts per scene frame: the live-DSV verification can
// transiently mismatch; retrying on subsequent translucent draws (instead
// of standing down for the frame) prevents single-frame blink-outs, and
// the cap bounds the retry cost.
static constexpr uint32_t KH_REORDER_MAX_INJECT_ATTEMPTS = 16;

// NOTE - inject at the FIRST qualifying translucent draw, deliberately
// without any consecutive-run requirement. A run gate was tried and caused
// the very flicker it meant to prevent: the engine interleaves depth-
// writing/alpha-tested draws INSIDE its translucent phase, which reset the
// run counter and pushed the injection mid-phase - everything blended
// before it got overdrawn by the boxes for that frame (and a fully starved
// frame lost the boxes entirely). Triggering early on an isolated blended
// draw amid the opaques is harmless by comparison: opaque ordering is
// depth-resolved regardless of when the boxes land, subsequent translucents
// still composite against them, and the only residue is a translucent box
// briefly blending over a not-yet-complete background. Determinism beats
// cleverness here.

// --- ID3D11DeviceContext vtable slots (d3d11.h declaration order) ---
static constexpr int KH_VT_PSSETSHADERRESOURCES  = 8;
static constexpr int KH_VT_DRAWINDEXED           = 12;
static constexpr int KH_VT_DRAW                  = 13;
static constexpr int KH_VT_MAP                   = 14;
static constexpr int KH_VT_UNMAP                 = 15;
static constexpr int KH_VT_DRAWINDEXEDINSTANCED  = 20;
static constexpr int KH_VT_DRAWINSTANCED         = 21;
static constexpr int KH_VT_OMSETRENDERTARGETS    = 33;
static constexpr int KH_VT_OMSETRTS_AND_UAVS     = 34;
static constexpr int KH_VT_OMSETBLENDSTATE       = 35;
static constexpr int KH_VT_OMSETDEPTHSTENCIL     = 36;
static constexpr int KH_VT_UPDATESUBRESOURCE     = 48;
static constexpr int KH_VT_CLEARDEPTHSTENCIL     = 53;

typedef void (STDMETHODCALLTYPE* FnPSSetShaderResources)(ID3D11DeviceContext*, UINT, UINT, ID3D11ShaderResourceView* const*);
typedef void (STDMETHODCALLTYPE* FnDrawIndexed)(ID3D11DeviceContext*, UINT, UINT, INT);
typedef void (STDMETHODCALLTYPE* FnDraw)(ID3D11DeviceContext*, UINT, UINT);
typedef void (STDMETHODCALLTYPE* FnDrawIndexedInstanced)(ID3D11DeviceContext*, UINT, UINT, UINT, INT, UINT);
typedef void (STDMETHODCALLTYPE* FnDrawInstanced)(ID3D11DeviceContext*, UINT, UINT, UINT, UINT);
typedef void (STDMETHODCALLTYPE* FnOMSetRenderTargets)(ID3D11DeviceContext*, UINT, ID3D11RenderTargetView* const*, ID3D11DepthStencilView*);
typedef void (STDMETHODCALLTYPE* FnOMSetRTsAndUAVs)(ID3D11DeviceContext*, UINT, ID3D11RenderTargetView* const*, ID3D11DepthStencilView*, UINT, UINT, ID3D11UnorderedAccessView* const*, const UINT*);
typedef void (STDMETHODCALLTYPE* FnOMSetBlendState)(ID3D11DeviceContext*, ID3D11BlendState*, const FLOAT[4], UINT);
typedef void (STDMETHODCALLTYPE* FnOMSetDepthStencilState)(ID3D11DeviceContext*, ID3D11DepthStencilState*, UINT);
typedef void (STDMETHODCALLTYPE* FnClearDepthStencilView)(ID3D11DeviceContext*, ID3D11DepthStencilView*, UINT, FLOAT, UINT8);
typedef HRESULT (STDMETHODCALLTYPE* FnMap)(ID3D11DeviceContext*, ID3D11Resource*, UINT, D3D11_MAP, UINT, D3D11_MAPPED_SUBRESOURCE*);
typedef void (STDMETHODCALLTYPE* FnUnmap)(ID3D11DeviceContext*, ID3D11Resource*, UINT);
typedef void (STDMETHODCALLTYPE* FnUpdateSubresource)(ID3D11DeviceContext*, ID3D11Resource*, UINT, const D3D11_BOX*, const void*, UINT, UINT);

static FnPSSetShaderResources    g_orig_pssetshaderresources = nullptr;
static FnDrawIndexed             g_orig_draw_indexed = nullptr;
static FnDraw                    g_orig_draw = nullptr;
static FnDrawIndexedInstanced    g_orig_draw_indexed_instanced = nullptr;
static FnDrawInstanced           g_orig_draw_instanced = nullptr;
static FnOMSetRenderTargets      g_orig_omset_rendertargets = nullptr;
static FnOMSetRTsAndUAVs         g_orig_omset_rts_and_uavs = nullptr;
static FnOMSetBlendState         g_orig_omset_blendstate = nullptr;
static FnOMSetDepthStencilState  g_orig_omset_depthstencil = nullptr;
static FnClearDepthStencilView   g_orig_clear_depthstencil = nullptr;
static FnMap                     g_orig_map = nullptr;
static FnUnmap                   g_orig_unmap = nullptr;
static FnUpdateSubresource       g_orig_updatesubresource = nullptr;

// CRITICAL - which thread is which:
// The hooks fire for EVERY caller of the immediate context - the engine's
// render thread AND our own extension's game-thread work (the Draw3D flush,
// compute dispatches, the UI pass all drive this context under the bridge's
// graphics lock, which parks the render thread while held). The flush's own
// box draws carry the exact translucent signature (alpha blend, no-write
// depth, main DSV), so if the hooks tracked them, the game thread would
// pollute the render thread's phase state and could even trigger a second,
// post-scene injection - boxes over everything for a frame. Every hook
// therefore gates its tracking to the RENDER THREAD, identified as the
// thread that clears the main scene depth (captured in the clear hook).
// Consequently ReorderState is single-threaded by construction.
//
// Equally critical: the injection must NEVER acquire the bridge graphics
// lock. Acquisition parks the render thread; acquiring FROM the render
// thread while the game thread holds it (and waits for the park) deadlocks
// the game - verified the hard way. No lock is needed either: game-thread
// context use only ever happens while the render thread is parked, so the
// two can never actually overlap; the dedicated composite constant buffer
// removes the last shared-resource concern.
//
// Deliberately NO pointer memoization anywhere in this machinery: state
// objects and views get destroyed and reallocated (PiP toggles, settings
// changes), and a new object at a reused address would inherit a stale
// classification - misattributed depth clears and misclassified blend
// states produce exactly the rare, single-frame, both-direction glitches
// this path must not have. GetDesc/GetResource are refcount-level calls at
// a few hundred per frame; correctness is free here.
static std::atomic<uint32_t> g_reorder_render_tid{0};

inline bool reorder_on_render_thread() {
    const uint32_t tid = g_reorder_render_tid.load(std::memory_order_relaxed);
    return tid != 0 && tid == GetCurrentThreadId();
}

struct ReorderState {
    bool injected = false;           // this scene frame already got its injection
    bool in_injection = false;       // our own draws in flight: bypass tracking
    bool blend_translucent = false;  // current blend state alpha-blends
    bool dss_nowrite = false;        // current DSS tests but does not write depth
    bool dsv_main = false;           // the main scene depth is bound
    uint32_t opaque_draws = 0;       // opaque main-scene draws since the last main depth clear
    uint32_t inject_attempts = 0;    // injection attempts this scene frame (retry bound)
    // Viewport depth range sampled every 16th opaque draw - used ONLY to
    // detect mid-cycle partition changes (the re-arm). It is deliberately
    // NOT a trigger gate or an encode source anymore: gating on it
    // required identity with the translucent phase's range, and a single
    // poisoned sample (the cadence landing on an oddball draw) could not
    // heal during the phase - no opaques run there - so entire frames
    // were rejected and the box blinked out. The world-shape SPAN test on
    // the trigger's own live range does all the legitimate cycle
    // discrimination on its own.
    float cycle_vp_min = 0.0f;
    float cycle_vp_max = 1.0f;
    bool  cycle_vp_valid = false;
    // Encode range for the injection: the TRIGGER draw's own live viewport
    // range - definitionally the range the world's translucents run under,
    // captured at the moment the trigger passes the world-shape test.
    float trig_vp_min = 0.0f;
    float trig_vp_max = 1.0f;
    bool  trig_vp_valid = false;
    // Projection-view transform latched AT THE MAIN DEPTH CLEAR - i.e., in
    // lockstep with the engine's own per-frame matrix snapshot. The engine
    // uses a DYNAMIC NEAR PLANE re-evaluated per frame; the sim thread runs
    // a frame ahead, so fetching the bridge PV mid-frame (at the translucent
    // phase, milliseconds after frame setup) frequently returns the NEXT
    // frame's projection. A near-plane difference skews the boxes' stored
    // depth hyperbolically: negligible for far fragments, enormous for near
    // ones - which is why the resulting flicker scales inversely with the
    // distance to the box. Latching at the clear keeps the boxes' depth
    // encoding synchronous with the geometry they are tested against.
    RVExtBridge::ProjectionViewTransform cycle_pv = {};
    bool  cycle_pv_valid = false;
    // The engine's TRUE projection depth coefficients for this cycle,
    // sniffed from its constant-buffer uploads (see the projection sniffer
    // below). Everything left of the flicker problem reduces to one
    // scalar - the dynamic near plane of the frame actually being
    // rendered, which no bridge fetch can pin down (publication races) -
    // and m32 IS that scalar (m32 ~= -n). Unlike the combined VP (which a
    // camera-relative renderer never uploads in world space), the
    // projection matrix survives camera-relative rendering intact and has
    // a near-unmistakable structural signature.
    float engine_m22 = 0.0f;
    float engine_m32 = 0.0f;
    bool  engine_proj_valid = false;
};
static ReorderState g_ro;

// Resolves a depth view to its underlying resource identity, live - never
// cached (see the note above).
inline void* reorder_dsv_identity(ID3D11DepthStencilView* dsv) {
    void* id = nullptr;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);

    if (res) {
        ID3D11Texture2D* tex = nullptr;

        if (SUCCEEDED(res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex))) && tex) {
            id = static_cast<void*>(tex);
            tex->Release();
        }

        res->Release();
    }

    return id;
}



// ===========================================================================
// Projection sniffer (v2 - depth coefficients only).
//
// A v1 sniffer hunted the engine's combined view-projection and found
// nothing: a camera-relative renderer never uploads that matrix in world
// space. The PROJECTION, however, survives camera-relative rendering
// unchanged, and a D3D perspective projection is structurally unmistakable
// inside arbitrary constant data: a dozen exact zeros, a literal one in a
// fixed slot, FOV scales matching the latch to a couple of percent, a
// stable m22 - and m32, the one volatile element (m32 ~= -nearPlane),
// exactly the scalar every remaining depth artifact hinges on. Matching is
// therefore TIGHT on structure and the stable elements and deliberately
// LOOSE on m32, since m32 is what is being measured.
//
// Cost control: scanning stops for the cycle once captured; a cached
// buffer + offset + orientation makes subsequent frames a single windowed
// test; unknown-locator scans reject almost every window on the first
// zero-check. All hooks are immediate-context + render-thread gated and
// injection-guarded like the rest of the machinery.
// ===========================================================================

struct ProjLocator {
    ID3D11Resource* buf = nullptr;   // identity only - never dereferenced
    uint32_t float_offset = 0;
    bool transposed = false;
    bool valid = false;
};
static ProjLocator g_proj_locator;   // render thread only

struct ProjPendingMap {
    ID3D11Resource* res = nullptr;
    void* data = nullptr;
    uint32_t bytes = 0;
};
static ProjPendingMap g_proj_pending[8];   // render thread only

inline bool proj_near_zero(float v) { return fabsf(v) < 1e-6f; }

// Persisted last captured m32: the continuity anchor for measurement-mode
// acceptance (see below).
static float g_proj_last_m32 = 0.0f;   // render thread only

// Tests a 16-float window for the row-vector D3D perspective layout
// [ sx 0 0 0 / 0 sy 0 0 / jx jy m22 1 / 0 0 m32 0 ] and its transpose.
// Structure, sx, sy and m22 must always match the latch tightly (stable
// frame to frame). m32 acceptance depends on the MODE, and getting this
// wrong caused a full cycle of regressions:
//  - DISCOVERY (scanning unknown buffers): m32 must be within 10% of the
//    latch. Discovery overwhelmingly happens on stable frames, and this is
//    what keeps the locator off the engine's OTHER projections (the
//    weapon slice's near plane is far outside 10% of the world's).
//  - MEASUREMENT (reading the locked location): the latch must NOT veto
//    m32 - on the very frames the sniffer exists for (a near-plane change
//    in flight) the true value DIFFERS from the latch by definition;
//    validating against it rejected the truth and blinked the box exactly
//    on those frames. Instead, accept when within 10% of the latch OR 25%
//    of the previously captured value (continuity): genuine per-frame
//    changes pass, a repurposed buffer (foreign near plane) fails both
//    and forces rediscovery.
inline bool proj_try_window(const float* w, bool discovery) {
    const float sx = g_ro.cycle_pv.projection[0][0];
    const float sy = g_ro.cycle_pv.projection[1][1];
    const float m22_l = g_ro.cycle_pv.projection[2][2];
    const float m32_l = g_ro.cycle_pv.projection[3][2];
    if (fabsf(m32_l) < 1e-9f) return false;

    auto close = [](float a, float b, float rel) {
        return fabsf(a - b) <= rel * (fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b)) + 1e-5f;
    };

    auto m32_ok = [&](float v) {
        if (close(v, m32_l, 0.10f)) return true;
        if (discovery) return false;
        return g_proj_last_m32 != 0.0f && close(v, g_proj_last_m32, 0.25f);
    };

    // Row-vector, row-major upload.
    if (proj_near_zero(w[1]) && proj_near_zero(w[2]) && proj_near_zero(w[3]) &&
        proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
        proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[15]) &&
        fabsf(w[11] - 1.0f) < 1e-4f &&
        close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
        m32_ok(w[14])) {
        {
            const float n_prev = fabsf(g_ro.engine_m22) > 1e-9f ? (-g_ro.engine_m32 / g_ro.engine_m22) : 0.0f;
            const float n_new  = fabsf(w[10]) > 1e-9f ? (-w[14] / w[10]) : 0.0f;

            // near-plane shift mid-frame: a partition boundary the
            // viewport-range check misses (the look-up slice: one
            // injection spent in the far partition, the near partition -
            // whose near plane clips less - never got its own draw).
            if (g_ro.engine_proj_valid && g_ro.injected &&
                n_prev > 1e-4f && fabsf(n_new - n_prev) > 0.05f * n_prev) {
                g_ro.injected = false;
                g_stats.composite_rearms++;
            }
        }
        g_ro.engine_m22 = w[10];
        g_ro.engine_m32 = w[14];
        g_proj_last_m32 = w[14];
        g_ro.engine_proj_valid = true;
        g_proj_locator.transposed = false;
        return true;
    }

    // Transposed (HLSL column_major) upload.
    if (proj_near_zero(w[1]) && proj_near_zero(w[3]) &&
        proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
        proj_near_zero(w[8]) && proj_near_zero(w[9]) && proj_near_zero(w[12]) && proj_near_zero(w[13]) &&
        fabsf(w[14] - 1.0f) < 1e-4f &&
        close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
        m32_ok(w[11])) {
        {
            const float n_prev = fabsf(g_ro.engine_m22) > 1e-9f ? (-g_ro.engine_m32 / g_ro.engine_m22) : 0.0f;
            const float n_new  = fabsf(w[10]) > 1e-9f ? (-w[11] / w[10]) : 0.0f;

            if (g_ro.engine_proj_valid && g_ro.injected &&
                n_prev > 1e-4f && fabsf(n_new - n_prev) > 0.05f * n_prev) {
                g_ro.injected = false;
                g_stats.composite_rearms++;
            }
        }
        g_ro.engine_m22 = w[10];
        g_ro.engine_m32 = w[11];
        g_proj_last_m32 = w[11];
        g_ro.engine_proj_valid = true;
        g_proj_locator.transposed = true;
        return true;
    }

    return false;
}

inline void proj_scan_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!g_ro.cycle_pv_valid || g_ro.engine_proj_valid) return;
    if (bytes < 16 * sizeof(float)) return;
    if (bytes > 16384) bytes = 16384;   // per-frame CBs are small; cap the scan
    const float* f = static_cast<const float*>(data);
    const uint32_t nfloats = bytes / 4;

    if (g_proj_locator.valid && g_proj_locator.buf == res) {
        if (g_proj_locator.float_offset + 16 <= nfloats &&
            proj_try_window(f + g_proj_locator.float_offset, false)) {
            g_stats.composite_proj_lock++;
            return;
        }

        g_proj_locator.valid = false;   // buffer repurposed: rescan
    }

    for (uint32_t off = 0; off + 16 <= nfloats; off += 4) {
        if (proj_try_window(f + off, true)) {
            g_proj_locator.buf = res;
            g_proj_locator.float_offset = off;
            g_proj_locator.valid = true;
            g_stats.composite_proj_lock++;
            return;
        }
    }
}

// Returns the buffer's byte width when it is a plausibly-sized constant
// buffer, 0 otherwise.
inline uint32_t proj_upload_byte_width(ID3D11Resource* res) {
    ID3D11Buffer* buf = nullptr;

    if (FAILED(res->QueryInterface(__uuidof(ID3D11Buffer), reinterpret_cast<void**>(&buf))) || !buf) {
        return 0;
    }

    D3D11_BUFFER_DESC bd = {};
    buf->GetDesc(&bd);
    buf->Release();

    if ((bd.BindFlags & D3D11_BIND_CONSTANT_BUFFER) == 0 || bd.ByteWidth < 16 * sizeof(float)) {
        return 0;
    }

    return bd.ByteWidth;
}

// ===========================================================================
// World lighting (sun/moon) + shadow-pass recon.
//
// LIGHTING (live now): the sun state is knowable from the game thread -
// getLighting supplies the light color, brightness and direction. Once per
// flush the game thread STAGES that state (stage_world_lighting), casts the
// per-object occlusion rays (update_shadow_rays - checkVisibility from the
// box toward the sun answers "does the world shadow our box" at object
// granularity, today, with zero render-thread involvement), and PUBLISHES
// the staged state inside flush_locked, where the graphics lock parks the
// render thread - the same invariant the injection already relies on for
// lock-free context use - so the render-thread consumers (composited-path
// shading, the recon scan) read plain globals that can never be observed
// mid-write. Shading itself is per pixel in ApplyLighting (see the shader).
//
// SHADOW RECON (evidence before a build is gambled): per-pixel shadows in
// BOTH remaining directions - sampling the engine's shadow maps so the
// world shadows our pixels, and injecting our geometry into the shadow
// passes so our objects shadow the world - hinge on ONE unknown: does the
// engine upload a SEPARABLE light view-projection, or only combined
// per-object World x LightVP matrices? The discovery template is the
// projection sniffer's, transplanted: a light VP's clip-Z axis is, by
// construction, the light direction, and the sun direction is
// independently known from the game thread - so a candidate 4x4 in a
// shadow-pass constant upload validates by its depth axis normalizing to
// within a couple of degrees of the published sun vector. The one failure
// mode the direction test CANNOT see is the combined-matrix case (a
// translation-only World preserves the rotation, so the axis still aligns
// while the translation is per-object garbage) - which is exactly what the
// DISTINCT-TRANSLATION counter measures: a separable light VP yields one
// translation per cascade (expect 1-4 distinct per cycle, and the count IS
// the cascade count), per-object combined matrices yield one per draw (the
// counter saturates at the cap). Enable with setShadowRecon and read
// shadowCyclesSeen / shadowScanUploads / sunAlignedCandidates /
// sunAlignedLastCycle / sunDistinctTranslationsMax off getRenderStats over
// a normal test scene: that one monitor run decides whether the shadow-map
// route is viable. Zero cost while disabled; while enabled the scan is
// bounded like the projection sniffer's (structural zero checks reject
// almost every window before any arithmetic).
//
// A shadow pass is fingerprinted as depth-only output: a DSV that is NOT
// the main scene's with no color target bound, plus a floor of draws (a
// depth pre-pass would bind the MAIN depth and is excluded by identity).
// ===========================================================================





typedef HRESULT (WINAPI* FnD3DDisassemble)(const void*, SIZE_T, UINT, const char*, void**);


static bool  g_sun_valid = false;
static float g_sun_dir_engine[3] = { 0.0f, 1.0f, 0.0f };  // unit vector TOWARD the light, engine axes
static float g_sun_color[3] = { 1.0f, 1.0f, 1.0f };       // light color, max-component normalized
// Ambient light (SQF-staged alongside the sun, elems 4-5 of the lighting
// command: [r,g,b] color and brightness). The shadow's color/depth derive
// from it: lit = ambient + sun, shadowed = ambient, so the per-channel
// factor is amb/(amb+sun).
static float g_amb_staged_color[3] = { 0.5f, 0.6f, 0.8f };
static float g_amb_staged_brightness = 0.35f;
static float g_amb_color[3] = { 0.5f, 0.6f, 0.8f };
static float g_amb_brightness = 0.35f;
static bool  g_amb_brightness_staged_flag = false;   // SQF actually provided it
static float g_sun_brightness = 0.0f;                     // raw getLighting brightness (diagnostic)

// --- Game-thread staging (written and read on the game thread only) ---
static bool  g_sun_staged_valid = false;
static float g_sun_staged_dir_engine[3] = { 0.0f, 1.0f, 0.0f };
static float g_sun_staged_dir_sqf[3] = { 0.0f, 0.0f, 1.0f };  // SQF axes, for the occlusion rays
static float g_sun_staged_color[3] = { 1.0f, 1.0f, 1.0f };
static float g_sun_staged_brightness = 0.0f;
// Fog: fetched on the game thread alongside getLighting (fogParams is an
// SQF-side getter; the extension self-fetches - no user SQF involved).
// RV's fog: density decays exponentially with height above fogBase.
static bool  g_fog_staged_valid = false;
static float g_fog_staged[3] = { 0.0f, 0.0f, 0.0f };   // value, decay, base
static bool  g_fog_valid = false;
static float g_fog[3] = { 0.0f, 0.0f, 0.0f };
// Sun direction derived from the engine's own cascade matrices (the SM's
// depth row is the light axis in view space; fold through the view
// rotation's transpose to world). Preferred over the script-fetched
// direction when fresh: pure engine truth, no SQF in the loop.
static bool  g_sun_dir_derived_valid = false;
static float g_sun_dir_derived[3] = { 0.0f, 1.0f, 0.0f };


inline void publish_world_lighting() {
    g_sun_valid = g_sun_staged_valid;
    g_fog_valid = g_fog_staged_valid;
    g_fog[0] = g_fog_staged[0];
    g_fog[1] = g_fog_staged[1];
    g_fog[2] = g_fog_staged[2];
    g_sun_dir_engine[0] = g_sun_staged_dir_engine[0];
    g_sun_dir_engine[1] = g_sun_staged_dir_engine[1];
    g_sun_dir_engine[2] = g_sun_staged_dir_engine[2];
    g_sun_color[0] = g_sun_staged_color[0];
    g_sun_color[1] = g_sun_staged_color[1];
    g_sun_color[2] = g_sun_staged_color[2];
    g_amb_color[0] = g_amb_staged_color[0];
    g_amb_color[1] = g_amb_staged_color[1];
    g_amb_color[2] = g_amb_staged_color[2];
    g_amb_brightness = g_amb_staged_brightness;
    g_sun_brightness = g_sun_staged_brightness;
}

// ===========================================================================
// LIVE shadow-map capture: the recon's verdict, promoted to infrastructure.
// The engine renders its cascades as depth-only cycles into one SRV-capable
// atlas (one cycle per cascade, distinguished by viewport), and uploads the
// cascade's world->atlasUV+depth 4x3 into a constant block during that
// cycle. Per frame, per cascade cycle: latch that matrix from the upload
// (locator-cached offset, recon-style rediscovery on miss), snapshot the
// bound viewport at the cycle's next draw as the cascade's atlas tile, and
// hand the table to the composited box shader, which then performs the
// engine's own shadow compare for our pixels. Render-thread owned; the
// flush path reads it only while the render thread is parked.
// ===========================================================================
static constexpr uint32_t KH_LIVE_MAX_CASCADES = 8;

struct LiveShadowEntry {
    float    m[12];
    float    tile[4];
    float    cam[3];     // the camera origin THIS entry's transform is
                         // relative to (entries persist across frames, so
                         // each carries its own; folded into the
                         // translation at fill time -> the shader consumes
                         // absolute world positions and staleness is
                         // mathematically irrelevant)
    uint64_t stamp;      // finalize order; the newest entry IS the atlas
};

struct LiveShadowState {
    std::atomic<bool> wanted{ false };        // game thread: any lit object exists
    // Atlas tracking (render thread)
    void*    atlas_identity = nullptr;
    ID3D11Texture2D*          atlas_tex = nullptr;   // AddRef'd
    ID3D11ShaderResourceView* atlas_srv = nullptr;
    uint32_t atlas_size = 0;
    uint32_t atlas_fmt = 0;
    bool     srv_failed = false;
    bool     phase_on_atlas = false;
    // Per-cycle latch
    bool     pending_valid = false;
    bool     cycle_latched = false;   // one DISTINCT entry per cascade cycle
    uint32_t pending_bytes = 0;       // source-CB size of the pending matrix
    bool     pending_vp_valid = false;
    float    pending_vp[4] = {};      // TopLeftX, TopLeftY, Width, Height
    float    pending_m[12] = {};
    // Decoded from the SMALL block per commit: its second 4x3 is a pure
    // camera-YAW rotation R with translation t2 = (0.30, -1.59, 0.01) - an
    // eye-height-shaped origin offset applied in yaw-rotated space. The
    // world-space equivalent R^T * t2 is the fold correction candidate.
    // GATED COMMITS: the engine's light-space fit can SPIN about the light
    // axis between refits, so a mispaired commit is offset PLUS rotated -
    // uncorrectable by translation. Static occluders make frozen pairs
    // valid forever, so once a pair is BLESSED (visually verified, via
    // setShadowBless), every candidate commit is cross-checked against it:
    // caster texels from the trusted copy, unprojected to world through
    // the trusted pair, reprojected through the candidate into the live
    // atlas - depths must agree. Mismatched candidates are DISCARDED and
    // the trusted pair kept. Registration becomes permanent.
    // RESOLVE-PAIRED CAPTURE: the engine's resolve draw (detected by the
    // atlas bound as a PS SRV) consumes the atlas with the CB it has bound
    // AT THAT DRAW - the transform<->content pairing stated by the engine
    // itself, at the consumption site. No timing heuristics, no spin
    // ambiguity, no blessing: valid from the first resolve of any session.
    struct CbRecord { void* buf = nullptr; uint32_t floats = 0; uint64_t seq = 0; float data[512]; };
    CbRecord cb_reg[16];
    uint32_t cb_reg_next = 0;
    uint64_t cb_reg_seq = 0;   // recency: which record was written last
    // Pass-scoped stash of the last window-shaped matrix ACCEPTED during
    // an atlas pass: the engine's cascade CB exceeds the registry's size
    // gate, so the registry never held it (castDraws==shadowLiveLatches
    // was the tell: only the small 48-float block ever matched). The
    // test-window choke point sees every candidate at ANY size.
    // Keyed by WINDOW SCALE (row-0 norm, unique per cascade): a single
    // unkeyed slot paired resolves with whichever cascade uploaded last -
    // sticky-phase ordering made that a coin flip (wrong-window casts:
    // per-direction shadow pop-in/out, close-range jitter).
    float    resolve_best_n0 = 0.0f;   // finest uv scale seen at resolves
    uint64_t resolve_hits = 0;
    uint64_t resolve_draws = 0;        // resolve-target draws examined
    uint64_t resolve_cb_found = 0;     // bound CBs matched in the registry
    float    align_last[3] = {};       // last solved (du, dv, dz) correction
    // SHADER-INPUT TRACKER: mirrors every CB field the copy path consumes,
    // per frame, and counts which one changed - plus the sampled uv of the
    // staged probe point through the EXACT shader math. uv changing while
    // copy_stamp is unchanged = a per-frame varying input (impossible slide
    // source named directly); uv changing only at reseals = alignment gap.
    bool     trk_valid = false;
    float    trk_mats[12] = {};
    float    trk_tile[4] = {};
    float    trk_src[4] = {};
    float    trk_fix2[4] = {};
    float    trk_uv[3] = {};
    uint64_t trk_stamp = 0;
                                          // only then does hold-on-unverifiable engage
                                          // (otherwise the blind first pair - possibly
                                          // coarse or mispaired - freezes forever)
    ID3D11Texture2D* align_staging = nullptr;   // 480x480: the shift search must
                                                // cover METERS (hundreds of texels),
                                                // not the verifier's 160-window
    ID3D11Texture2D* verify_staging = nullptr;   // 160x160 R32F reads
    uint64_t verify_offwindow = 0;     // candidate doesn't cover trusted casters
    // PERSISTENT cascade table: the engine re-renders (and re-uploads the
    // transform for) a cascade only when it needs updating - round-robin,
    // on demand - so a per-frame table is empty on most frames. Entries are
    // keyed by scale (each cascade has a distinct extent) and replaced in
    // place when their cascade re-renders.
    LiveShadowEntry entries[KH_LIVE_MAX_CASCADES] = {};
    uint32_t count = 0;
    uint64_t stamp_counter = 0;
    int      newest = -1;          // entry index of the last finalize
    // Fine-cascade copy: pending is armed at finalize when the finished
    // cascade is the finest known; the copy executes on the cycle's exit
    // (its draws must be complete), pairing the grabbed content with the
    // transform stashed here.
    // VIEW-PAIRED BANDS (pair mode 2, default): the engine's own resolve
    // contract, decoded from its shader - the sampling matrix consumes
    // VIEW-SPACE position, cascades are selected by VIEW DEPTH band
    // (PSC_ShadowmapLayerBorder), and freezing (SM, view matrix, border,
    // content) together registers the static world exactly, rotation
    // included. Two slots cover the two nearest bands.
    struct BandSlot {
        bool  valid = false;
        bool  pending_view = false;   // sealed content+SM awaiting this frame's view
        float pending_since = 0.0f;   // fallback: complete from the bridge after 100 ms
        float sm[12] = {};        // PSC_ShadowmapMatrix rows (float4x3)
        float vcol[12] = {};      // view matrix columns 0..2 (world->view, row-vector conv)
        float border[4] = {};     // x = near, y = far, z = 1/(y-x) fade
        float last_time = 0.0f;
        ID3D11Texture2D*          tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        uint64_t copies = 0;
    };
    BandSlot band[8];   // all 8 engine cascades: full-range receive
    uint64_t band_captures = 0;
    uint64_t band_bail_pv = 0;       // no cycle view available
    uint64_t band_bail_off = 0;      // window too close to CB end (no border lanes)
    uint64_t band_bail_border = 0;   // border lanes not a sane depth band
    uint64_t band_bail_slot = 0;     // both slots hold nearer bands
    uint64_t band_bail_time = 0;     // throttled (healthy)
    // Depth-partitioned frames (the look-down case) run resolves PER
    // PARTITION, and each batch REWRITES the mask - a single early write
    // was erased by the next partition's resolve. Set at every gated
    // resolve; the fire consumes it, so the mask is re-shadowed after
    // every batch. MIN blending makes repeated writes idempotent.
    bool resolve_seen_since_cast = false;
    bool view_published_this_frame = false;   // dual gate for the cast arm
    float last_publish_rot_err = 1.0f;        // rot agreement of the last accepted publish
    uint64_t band_bail_view = 0;     // no same-frame view latched: reseal skipped
    float band_last_reject[4] = {};  // last border quad rejected by sanity
    // Cast tables: SM/V per RESOLVE ORDER within a frame. The cascade
    // render precedes its resolve, so at each atlas-pass end the k-th
    // pass uses the PREVIOUS frame's k-th captured matrices (the cascade
    // sequence is stable; one frame of latency is the v1 tradeoff).
    uint64_t casts = 0;
    uint64_t cast_misses = 0;   // passes with no matching cast entry
    ID3D11DepthStencilView* atlas_dsv = nullptr;   // our own DSV on the atlas
    bool atlas_dsv_failed = false;
    // A GENUINE shadow resolve is the screen-sized single-channel draw
    // that FOLLOWS its cascade's atlas pass. PSCB_PostProcess (cb13) is
    // bound across the engine's whole postprocess chain, so matching the
    // block alone admits every R8-target postprocess draw - casting with
    // its stale matrix lanes splatted the box across the atlas and
    // shadowed the world. Sequencing is the discriminator we own.
    uint32_t resolve_window = 0;   // qualifying draws remaining after an atlas-pass exit
    uint64_t resolve_gated = 0;    // sweeps skipped outside the window (diagnostic)
    // Freshest engine VIEW matrix, latched from render-thread uploads
    // (same-frame with the resolves by construction). cycle_pv is the
    // game-thread snapshot and runs a frame AHEAD during movement - the
    // exact source of motion-proportional slide when frozen into bands.
    // Frame-consistent bridge view: snapshotted ONCE at the frame
    // boundary so every seal and cast in a frame uses the IDENTICAL
    // view - no mid-frame sampling of a value another thread updates.
    float frame_view[16] = {};
    bool  frame_view_valid = false;
    float frame_view_time = -1.0f;
    // SUPERVISED VIEW-SOURCE LEARNING. The injection holds the PROVEN
    // per-frame view (the box renders pixel-registered with it, every
    // frame) but only LATE in the frame; the resolves run EARLY, when the
    // bridge can still hold LAST frame's camera - an error proportional
    // to angular velocity (rotation jitter; linear motion hides it).
    // During each frame, view-shaped uploads are recorded with their
    // LOCATION; at injection, whichever location matched the proven view
    // is locked. From then on the view is read from that exact location -
    // early, same-frame, zero tolerance-matching (the retired latch's
    // three pathologies were all its fuzzy family selection; its PHASE
    // was the thing it had right).
    struct ViewCand { ID3D11Resource* res; uint32_t off; float m[16]; };
    ViewCand vc_ring[16];
    uint32_t vc_n = 0;
    ID3D11Resource* view_src_res = nullptr;   // learned location (weak key)
    uint32_t view_src_off = 0;
    int  view_src_orient = 0;                 // 1 direct, 2 transposed
    bool view_src_relative = false;           // engine view is camera-relative (t ~ 0)
    bool view_src_valid = false;
    uint32_t view_src_miss = 0;
    uint64_t view_locks = 0;
    uint64_t frame_view_hits = 0;
    float view_best_rot = -1.0f;    // best candidate's rotation error at last injection
    float view_best_trans = -1.0f;  // and its translation error (diagnosis when unlocked)
    float prev_view[16] = {};       // previous accepted view (rotation extrapolation)
    bool  prev_view_valid = false;
    uint64_t seal_completions = 0;  // deferred seals finished by an arriving view
    float copy_stash_m[12] = {};
    float    copy_scale_n0 = 0.0f;    // n0 of the copied cascade's transform
    uint64_t copy_stamp = 0;          // stamp_counter at copy time
    // Depth probe: a KNOWN sunlit ground point (its depth is its own entry
    // in the map). Registration error = offset between where the copy's
    // transform SAYS its depth is and where the readback FINDS it.
    ID3D11Texture2D* probe_staging = nullptr;   // 64x64 R32F CPU-read
    float probe_last_time = 0.0f;
    // results (render thread writes, game thread reads - diagnostics)
    // Plane-fit outputs: on locally planar sunlit ground the map depth is
    // d(x,y) = a x + b y + c. One plane yields ONE constraint on the
    // translation error (-a du - b dv + dz = e); THREE differently sloped
    // planes make (du, dv, dz) solvable exactly - no physical landmark
    // needed (units are stencil-volume shadows and never enter the buffer).
                                      // mostly sky (fit skipped; center still valid),
                                      // 3 measured, 4 map failed
    float probe_uvz[3] = {};          // the probe point through the copy pair
                                      // map holds OBJECT CASTERS ONLY (terrain
                                      // is not buffer-rendered - open ground is
                                      // far plane), so the occlusion VERDICT
                                      // (centerZ < expected) vs a CPU
                                      // checkVisibility truth at the same point
                                      // is the edge-scan calibration signal.
    // Upload locator cache
    bool     cache_valid = false;
    uint32_t cache_bytes = 0;
    uint32_t cache_offset = 0;
    uint64_t latches = 0;
    float    cam[3] = {};      // the frame's render camera (engine axes),
                               // recorded at injection - the shadow space origin
};
static LiveShadowState g_ls;

// Tunables (setShadowMapParams): benign cross-thread plain floats.
static float g_shadow_map_strength = 1.0f;    // 0 disables the map term
static float g_shadow_map_bias = 0.0015f;     // in the transform's depth units
static float g_shadow_map_sign = 1.0f;        // +1 standard depth, -1 reversed
static float g_shadow_map_debug = 0.0f;       // 0 off, 1 occlusion viz, 2 cascade viz
// Which latched transform to sample with. The engine renders its cascades
// SEQUENTIALLY into the same full-atlas map (full-viewport tiles, one
// screen-space apply per cascade), so at injection time the atlas holds
// only the LAST cascade rendered - sampling with any other cascade's
// transform gives shadows that swing with camera rotation as the cascade
// fits track the frustum. -1 = last latched (matches atlas content;
// default), k >= 0 = force entry k (live probing), -2 = all entries
// finest-first (the multi-cascade behavior, correct only for tiled
// atlases).
static float g_shadow_map_cascade = -1.0f;
// The live-atlas fallback samples UNVERIFIED content: a cascade that
// re-renders with an unchanged transform skips the CB upload, so 'newest'
// cannot track it. The fine copy is a frozen verified pair and can never
// misregister. Default: fallback only when no copy exists yet.
static float g_shadow_map_fallback = 0.0f;    // 1 = always add live-atlas entries after the copy
// Same-size pick order within a cycle. The receiver transform for the
// cascade being rendered plausibly uploads at the cycle's TAIL (for the
// apply pass that follows), while an early same-size upload can still hold
// the PREVIOUS cascade's constants - first-wins then commits a crossed
// pair. Live-togglable so the truth costs ten seconds, not a rebuild.
static float g_shadow_map_pick = 1.0f;        // 1 = last same-size upload wins, 0 = first
// Copy shift: which cycle's CONTENT pairs with the latched transform. 0 =
// the latch cycle itself (transform describes the cascade being rendered);
// 1 = the NEXT atlas cycle (the upload is one-cycle-early prep for the
// apply pass, so the content it describes renders in the following cycle).
// Live-togglable: if shadows lock at 1, the prep hypothesis is the truth.
static float g_shadow_map_shift = 0.0f;
// Fold origin for the copy pair. The dumped source CB carries a STATIC
// world-magnitude triple at floats [344..346] in engine axis order (x,
// altitude, y) - the signature of RV's REBASED rendering origin. If the
// shadow transforms are relative to that snapped origin rather than the
// exact render eye, folding the eye explains every measured symptom:
// meter-scale error, constant while standing, jumping on re-snap.
// 0 = fold render eye (default), 1 = fold CB[344..346] (DISPROVEN: that
// triple sits kilometers from the play area - a static map constant, and
// folding it maps every pixel outside the cascade -> no shadows at all),
// 2 = fold eye - CB[352..354], 3 = fold eye + CB[352..354]. [352..354] is
// the only camera-magnitude PER-COMMIT triple in the block (changed with
// player movement between dumps) - candidate origin-to-camera offset.
// Direct UV-depth correction for the copy entry: the error lives in the
// cascade's output space, so correct it there - no origin guessing. Set
// via setShadowUvFix [du, dv, dz]; visible immediately (du of 0.01 slides
// the shadow ~24 cm at the fine cascade).
static float g_shadow_uvfix[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   // du, dv, dz, ROTATION degrees
// 1 = resolve-paired capture (default): the fine pair is sealed AT THE
// ENGINE'S OWN RESOLVE DRAW - pairing by construction. 0 = legacy
// cycle-exit pairing (heuristic).
// 2 = VIEW-PAIRED BANDS (default): the engine's decoded receiver
// contract - SM matrix consumes view-space position, band selected by
// view depth, view matrix frozen with each band. 1 = resolve-paired
// world-relative (legacy). 0 = cycle-exit heuristic (legacy).
static float g_shadow_pair_mode = 2.0f;
// 0 (default) = the bridge PV, demonstrated same-frame correct by the
// box's own registration every frame. 1 = opt-in experimental upload
// latch (historically pathological: promiscuity, lockout, wrong-family).
static float g_shadow_freshview = 0.0f;
// Boxes CAST shadows onto the world: depth-only draws into the engine's
// shadow atlas at each cascade pass end. setShadowMapParams elem 14.
static float g_shadow_cast = 1.0f;
// Order of cast vs receive-copy at the resolve. 1 (default) = cast first:
// receive copies CONTAIN the box (self-shadowing via bands), but any cast
// imprecision then dances ON the box itself, indistinguishable from world
// shadows. 0 = copy first: the receive path never sees the box's depth -
// no band self-shadow, but world shadows on the box are provably pure.
// setShadowMapParams elem 16.
// 0 (new default): the atlas cast is OFF - the analytic mask cast is the
// world-facing shadow now (per-pixel exact, jitter impossible by
// construction), and running both double-darkens with the atlas cast's
// jittery copy visible as a fringe. 1 restores the atlas cast (and with
// it band-copy self-shadowing) for A/B. setShadowMapParams elem 16.
// EXPERIMENTAL cast via the live-latched cascade-RENDER transforms
// (world-relative, zero view). Correct family, wrong freshness as-is:
// the table finalizes ONE latch per cycle, so entries run up to ~8
// frames stale - and a stale window map under rotation aims the cast at
// where the window USED to be (the shadow that 'slid away and whizzed
// past'). Receiving tolerates stale entries because each pairs with its
// own-era content; casting needs THIS frame's map. Re-enable (elem 17)
// only together with per-pass latching. Default off.
// 0 (default): cast with the extrapolated same-frame view - the best
// measured behavior. 1: experimental world-relative stash cast. The
// recon counters (sunAligned43/44 == 0 in EVERY session) say the pure
// world-space cascade map likely never appears in the upload stream -
// the engine renders cascades via per-object combined transforms - so
// the stash only ever catches the view-space twin at varying camera
// alignments (the slide-and-flicker). Chase ended on that evidence.
// Cast output: 0 (default) = ENGINE-MASK hybrid (analytic position, the
// engine's own coloring); 1 = screen-multiply fallback (self-contained,
// ambient-approximated). setShadowMapParams elem 18.
static float g_shadow_cast_screen = 0.0f;
// Armed by the view latch (defined before g_mask, which lives later in
// the file); consumed by mask_cast_fire.
static bool g_mask_cast_arm = false;
static bool g_mask_cast_fired = false;   // one analytic pass per frame:
// the engine re-uploads its view ~240x/frame and firing at every publish
// stacked 240 fullscreen multiplies (shade^240 -> mask annihilated,
// frame time doubled). analyticCasts == frameViewHits was the tell.

inline bool shadow_live_wanted() {
    return g_ls.wanted.load(std::memory_order_relaxed) && g_sun_valid && g_shadow_map_strength > 0.0f;
}

// Fills the lighting slots of a draw constant block from an object and the
// published sun state. Called by both solid-box paths (flush and injection);
// both run either on the game thread with the lock held or on the parked-
// exclusive render thread, so plain reads of the published globals are safe.
inline void fill_lighting_cb(ConstantData& cbd, const RenderObject& o) {
    cbd.lighting0[0] = o.lit ? 1.0f : 0.0f;
    cbd.lighting0[1] = o.shadow_factor;
    cbd.lighting0[2] = o.light_ambient;
    cbd.lighting0[3] = o.light_diffuse;
    // derived-from-matrices measured ~9 deg off the engine's reported
    // direction (SM depth-row bias/oblique skew) and moves per seal.
    // FALLBACK only: script-free operation survives a lighting-fetch
    // failure, but exactness steers when both exist.
    const float* sunp = g_sun_valid ? g_sun_dir_engine :
                        (g_sun_dir_derived_valid ? g_sun_dir_derived : g_sun_dir_engine);
    cbd.lighting1[0] = sunp[0];
    cbd.lighting1[1] = sunp[1];
    cbd.lighting1[2] = sunp[2];
    cbd.lighting1[3] = g_sun_valid ? 1.0f : 0.0f;
    cbd.lighting2[0] = g_sun_color[0];
    cbd.lighting2[1] = g_sun_color[1];
    cbd.lighting2[2] = g_sun_color[2];
    cbd.lighting2[3] = g_shadow_map_strength;

    // Cascade table, finest scale first (first containing tile decides in
    // the shader). The PUBLISHED table is used - the engine renders its
    // cascades BEFORE the main depth clear, so the building table is
    // already reset by injection time; the published one is this frame's
    // completed set (or at worst last frame's, which its embedded camera
    // origin keeps correct). Skipped entirely for unlit objects.
    if (!o.lit || g_ls.count == 0 || g_ls.newest < 0) return;

    uint32_t order[KH_LIVE_MAX_CASCADES];
    const uint32_t avail = g_ls.count <= KH_LIVE_MAX_CASCADES ? g_ls.count : KH_LIVE_MAX_CASCADES;
    uint32_t n;
    const float sel = g_shadow_map_cascade;

    if (sel <= -1.5f) {
        // -2: every entry, finest scale first (tiled-atlas semantics).
        n = avail;
        for (uint32_t i = 0; i < n; ++i) order[i] = i;

        auto scale_of = [&](uint32_t i) {
            const float* r0 = g_ls.entries[i].m;
            return r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2];
        };

        for (uint32_t i = 1; i < n; ++i) {      // tiny insertion sort, n <= 8
            const uint32_t v = order[i];
            uint32_t j = i;
            while (j > 0 && scale_of(order[j - 1]) < scale_of(v)) { order[j] = order[j - 1]; --j; }
            order[j] = v;
        }
    } else if (sel < 0.0f) {
        // -1: the most recently RE-RENDERED cascade = what the atlas holds.
        n = 1;
        order[0] = static_cast<uint32_t>(g_ls.newest);
    } else {
        // Forced index (clamped): live probing of which entry matches.
        uint32_t k = static_cast<uint32_t>(sel);
        if (k >= avail) k = avail - 1;
        n = 1;
        order[0] = k;
    }

    // The fine copy, when live, samples FIRST (index 0 -> t2); the entries
    // selected above follow as the wide-area fallback on the live atlas.
    // shadow_meta[0] (the count) is written LAST, after both parts land.
    uint32_t out_i = 0;
    cbd.shadow_src[0] = -1.0f;
    cbd.shadow_fix2[0] = 1.0f;   // identity rotation unless the copy path sets it

    cbd.shadow_meta[1] = g_shadow_map_sign;
    cbd.shadow_meta[2] = g_shadow_map_bias;
    cbd.shadow_meta[3] = static_cast<float>(g_ls.atlas_size);
    cbd.shadow_meta2[0] = g_shadow_map_debug;
    // shadowMeta2.yzw stays zero: each entry's camera origin is folded into
    // its translation below, so the shader's (wpos - meta2.yzw) consumes
    // ABSOLUTE world positions and per-entry staleness cannot misregister.
    cbd.shadow_meta2[1] = 0.0f;
    cbd.shadow_meta2[2] = 0.0f;
    cbd.shadow_meta2[3] = 0.0f;

    for (uint32_t i = 0; i < n && out_i < KH_LIVE_MAX_CASCADES; ++i, ++out_i) {
        const LiveShadowEntry& e = g_ls.entries[order[i]];
        memcpy(cbd.shadow_tiles[out_i], e.tile, sizeof(e.tile));

        for (int r = 0; r < 3; ++r) {
            float* dst = cbd.shadow_mats[out_i * 3 + r];
            const float* row = e.m + r * 4;
            dst[0] = row[0];
            dst[1] = row[1];
            dst[2] = row[2];
            // t' = t - M.row . cam_entry: uvz = M*(p - cam) + t = M*p + t'
            dst[3] = row[3] - (row[0] * e.cam[0] + row[1] * e.cam[1] + row[2] * e.cam[2]);
        }
    }

    cbd.shadow_meta[0] = static_cast<float>(out_i);

}

// --- Recon state: render thread only, like ReorderState. ---
static constexpr uint32_t KH_SHADOW_CYCLE_MIN_DRAWS = 8;    // draws before a depth-only phase counts
static constexpr float    KH_SHADOW_AXIS_COS_TOL   = 0.995f; // ~5.7 degrees off the sun vector
static constexpr float    KH_SHADOW_ORTHO_TOL      = 0.02f;  // orthonormality slack for the 3x3 test

// Distance-to-success watermarks (render thread writes, game thread reads;
// aligned 32-bit floats - torn reads impossible on x86, and the values are
// diagnostics). Reset when the recon is armed.
static float g_sr_best_dot44 = 0.0f;   // best |cos(zAxis, sun)| among affine 4x4 windows
static float g_sr_best_dot43 = 0.0f;   // best |cos(basisVec, sun)| among orthogonal 3x3 bases

// Capture of the best sun-aligned orthogonal 4x3 seen while armed. If the
// engine uploads the COMPLETE light VP in RV's native affine form, this is
// the whole prize: the row norms are the ortho scale factors (extents), the
// fourth-element column is the translation, and the aligned axis is the
// light. Render-thread writes; read from the game thread via
// getShadowCapture after disarming (or accept a frame of skew - it is a
// diagnostic).
static constexpr uint32_t KH_SHADOW_CTX_BEFORE = 8;    // floats captured before the matrix
static constexpr uint32_t KH_SHADOW_CTX_AFTER  = 28;   // floats captured after it
static constexpr uint32_t KH_SHADOW_CTX_TOTAL  = KH_SHADOW_CTX_BEFORE + 12 + KH_SHADOW_CTX_AFTER;

struct ShadowCapture43 {
    bool     valid = false;
    bool     in_phase = false;
    float    dot = 0.0f;         // |cos| of the best-aligned normalized axis
    int      axis = -1;          // 0-2 = row, 3-5 = column index of that axis
    float    norms[3] = {};      // row lengths (scale factors; 1,1,1 = pure view)
    float    m[12] = {};         // the raw window: rows w[0..3], w[4..7], w[8..11]
    // The matrix's surroundings in its upload: the ortho scale/offset a bare
    // view implies MUST be uploaded somewhere, and "the adjacent constants
    // in the same CB" is the overwhelmingly likely somewhere. ctx holds
    // [KH_SHADOW_CTX_BEFORE floats before | the 12 | KH_SHADOW_CTX_AFTER
    // after], zero-padded at upload edges; ctx_start is how many of the
    // BEFORE floats were actually available.
    float    ctx[KH_SHADOW_CTX_TOTAL] = {};
    uint32_t ctx_start = 0;
    uint32_t upload_floats = 0;  // total floats in the upload
    uint32_t offset_floats = 0;  // matrix offset within it
};
// Two independent slots: the best UNIFORM-norm hit (a bare light view - the
// rotation is the light, projection elsewhere) and the best SCALED hit (an
// affine VP or a world->shadowUV sampling transform - projection INCLUDED).
static ShadowCapture43 g_sr_cap_view;
static ShadowCapture43 g_sr_cap_vp;

// Snapshot of the translation table from the last closed cycle that had
// aligned candidates: the cascade structure, readable after disarming.
static float    g_sr_last_trans[16][3] = {};
static uint32_t g_sr_last_trans_count = 0;

// The full cascade FAMILY: every scaled sun-aligned 4x3 found in a single
// upload, offsets and all. The engine packs the per-cascade sampling
// transforms into one constant block; whichever upload yields the most of
// them is that block, and this set is the shader-ready cascade table the
// sampling build will consume.
struct ShadowCascadeSet {
    uint32_t upload_floats = 0;
    uint32_t count = 0;
    uint32_t offsets[8] = {};
    float    m[8][12] = {};
};
static ShadowCascadeSet g_sr_cascades;           // best-so-far published set
static ShadowCascadeSet g_sr_cascades_scratch;   // per-upload collection

// Topology of the depth targets bound during classified shadow phases:
// dimensions, format, array shape and the DSV's slice - the difference
// between one atlas, a texture array, and separate per-cascade maps, which
// decides how the sampling SRV must be built.
struct ShadowTargetInfo {
    void*    tex_identity = nullptr;
    uint32_t width = 0, height = 0;
    uint32_t format = 0;             // DXGI_FORMAT of the texture
    uint32_t array_size = 0;
    uint32_t first_slice = 0;        // from the DSV desc (array DSVs)
    uint32_t bind_flags = 0;
    uint64_t cycles = 0;             // shadow cycles that used this target
};

// Forward: defined with the live-capture group below.
inline void shadow_live_consider_atlas(ID3D11Texture2D* tex, const D3D11_TEXTURE2D_DESC& td);

static constexpr uint32_t KH_SHADOW_MAX_TRANSLATIONS = 16;   // distinct-translation cap per cycle

struct ShadowReconState {
    bool     phase_active = false;    // depth-only, non-main DSV currently bound
    void*    target_identity = nullptr;
    uint32_t draws_this_cycle = 0;
    uint32_t candidates_this_cycle = 0;
    float    trans[KH_SHADOW_MAX_TRANSLATIONS][3] = {};   // quantized candidate translations
    uint32_t trans_count = 0;
};
static ShadowReconState g_sr;   // render thread only

// Folds the finished cycle into the cumulative stats and resets the
// per-cycle evidence. Called on target changes, phase exits, and the main
// depth clear (frame boundary).
inline void shadow_close_cycle() {
    if (g_sr.candidates_this_cycle > 0) {
        if (g_sr.trans_count > g_stats.sun_distinct_translations_max) {
            g_stats.sun_distinct_translations_max = g_sr.trans_count;
        }

        if (g_sr.trans_count > 0) {
            memcpy(g_sr_last_trans, g_sr.trans, sizeof(g_sr_last_trans));
            g_sr_last_trans_count = g_sr.trans_count;
        }
    }

    g_sr.draws_this_cycle = 0;
    g_sr.candidates_this_cycle = 0;
    g_sr.trans_count = 0;
}

inline void shadow_record_translation(float tx, float ty, float tz) {
    g_sr.candidates_this_cycle++;
    // Saturated = one translation per draw = combined per-object matrices.
    if (g_sr.trans_count >= KH_SHADOW_MAX_TRANSLATIONS) return;
    // Half-meter quantization: cascades re-center per frame but not per draw.
    const float qx = floorf(tx * 2.0f), qy = floorf(ty * 2.0f), qz = floorf(tz * 2.0f);

    for (uint32_t i = 0; i < g_sr.trans_count; ++i) {
        if (g_sr.trans[i][0] == qx && g_sr.trans[i][1] == qy && g_sr.trans[i][2] == qz) return;
    }

    g_sr.trans[g_sr.trans_count][0] = qx;
    g_sr.trans[g_sr.trans_count][1] = qy;
    g_sr.trans[g_sr.trans_count][2] = qz;
    ++g_sr.trans_count;
}

// Dedupes a 4x3 candidate's translation into the shared per-cycle table
// WITHOUT touching the 4x4 candidate counters - the distinct-translation
// verdict is representation-agnostic, the counters are not.
inline void shadow_record_translation43(float tx, float ty, float tz) {
    g_sr.candidates_this_cycle++;
    if (g_sr.trans_count >= KH_SHADOW_MAX_TRANSLATIONS) return;
    const float qx = floorf(tx * 2.0f), qy = floorf(ty * 2.0f), qz = floorf(tz * 2.0f);

    for (uint32_t i = 0; i < g_sr.trans_count; ++i) {
        if (g_sr.trans[i][0] == qx && g_sr.trans[i][1] == qy && g_sr.trans[i][2] == qz) return;
    }

    g_sr.trans[g_sr.trans_count][0] = qx;
    g_sr.trans[g_sr.trans_count][1] = qy;
    g_sr.trans[g_sr.trans_count][2] = qz;
    ++g_sr.trans_count;
}

// Tests a 16-float window for an AFFINE 4x4 (a light VP is orthographic -
// no perspective terms, unlike the scene projection the other sniffer
// hunts) whose clip-Z axis aligns with the published sun direction. Both
// upload orientations are covered; sign is irrelevant (|dot|), which also
// makes the test agnostic to whether the engine's light vector points to
// or from the sun.
// Absolute cosine between a (possibly scaled) axis and the published sun.
inline float shadow_axis_abs_cos(float ax, float ay, float az) {
    const float len2 = ax * ax + ay * ay + az * az;
    if (len2 < 1e-12f) return 0.0f;
    const float d = ax * g_sun_dir_engine[0] + ay * g_sun_dir_engine[1] + az * g_sun_dir_engine[2];
    return fabsf(d) / sqrtf(len2);
}

// One 16-float window, both 4x4 orientations. Measures as it tests: every
// affine-SHAPED window bumps shadow_affine44 and pushes its z-axis
// alignment into the best-dot watermark, so a run that finds no candidates
// still reports how close the closest miss came - the difference between
// "loosen the tolerance" and "wrong representation entirely".
inline void shadow_try_window(const float* w, bool in_phase) {
    // Reject identity-shaped windows: they pass the axis test whenever the
    // sun happens to align with a world axis, and engines upload identity
    // constantly.
    if (fabsf(w[0] - 1.0f) < 1e-6f && fabsf(w[5] - 1.0f) < 1e-6f &&
        fabsf(w[10] - 1.0f) < 1e-6f && proj_near_zero(w[1]) && proj_near_zero(w[2]) &&
        proj_near_zero(w[4]) && proj_near_zero(w[6])) {
        return;
    }

    auto consider = [&](float c, const float* t) {
        if (c > g_sr_best_dot44) g_sr_best_dot44 = c;
        if (c < KH_SHADOW_AXIS_COS_TOL) return;

        if (in_phase) {
            shadow_record_translation(t[0], t[1], t[2]);
        } else {
        }
    };

    // Row-vector, row-major upload: affine column at w[3,7,11,15], clip-Z
    // axis (w[2], w[6], w[10]), translation row (w[12..14]).
    if (proj_near_zero(w[3]) && proj_near_zero(w[7]) && proj_near_zero(w[11]) &&
        fabsf(w[15] - 1.0f) < 1e-4f) {
        g_stats.shadow_affine44++;
        const float t[3] = { w[12], w[13], w[14] };
        consider(shadow_axis_abs_cos(w[2], w[6], w[10]), t);
    }

    // Transposed upload (column_major HLSL - equivalently a column-vector
    // convention stored row-major): affine row at w[12..15], z axis
    // (w[8], w[9], w[10]), translation column (w[3], w[7], w[11]).
    if (proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[14]) &&
        fabsf(w[15] - 1.0f) < 1e-4f) {
        g_stats.shadow_affine44++;
        const float t[3] = { w[3], w[7], w[11] };
        consider(shadow_axis_abs_cos(w[8], w[9], w[10]), t);
    }
}

// 12-float window as a 3x4 affine (rows w[0..2], w[4..6], w[8..10] = a 3x3
// basis; w[3], w[7], w[11] = translation) - the shape RV's native 4x3
// Matrix4 takes under cbuffer packing. A light VIEW matrix never passes the
// 4x4 affine test (no projective row at all), but its rotation is
// orthonormal and one basis vector IS the light direction - in EITHER
// storage orientation that vector is a row or a column of the 3x3, so all
// six are tested against the sun. Watermark + counter only: this detector
// exists to tell the next run WHERE the light transform lives, not to
// harvest translations.
inline void shadow_try_window43(const float* base, uint32_t nfloats, uint32_t off, bool in_phase) {
    const float* w = base + off;
    const float* r0 = w + 0;
    const float* r1 = w + 4;
    const float* r2 = w + 8;

    const float n0 = r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2];
    const float n1 = r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2];
    const float n2 = r2[0] * r2[0] + r2[1] * r2[1] + r2[2] * r2[2];
    if (n0 < 1e-12f || n1 < 1e-12f || n2 < 1e-12f) return;

    // Mutual orthogonality, scale-aware (normalized by the pair's norms):
    // an affine ORTHO VP is a rotation with per-axis scale - rows stay
    // orthogonal but their norms are the projection's scale factors. The
    // orthonormal case (equal norms ~ a bare view/rotation) is the special
    // case norms ~= n0.
    const float d01 = r0[0] * r1[0] + r0[1] * r1[1] + r0[2] * r1[2];
    const float d02 = r0[0] * r2[0] + r0[1] * r2[1] + r0[2] * r2[2];
    const float d12 = r1[0] * r2[0] + r1[1] * r2[1] + r1[2] * r2[2];
    const float t01 = KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n1);
    const float t02 = KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n2);
    const float t12 = KH_SHADOW_ORTHO_TOL * sqrtf(n1 * n2);
    if (fabsf(d01) > t01 || fabsf(d02) > t02 || fabsf(d12) > t12) return;

    const float tol_n = KH_SHADOW_ORTHO_TOL * n0;
    const bool uniform = fabsf(n1 - n0) <= tol_n && fabsf(n2 - n0) <= tol_n;

    // Axis-aligned diagonal basis (scaled identity): the false-positive
    // class that dominates uploads. Rejecting it costs nothing real - a sun
    // exactly on a world axis is a measure-zero scene.
    if (fabsf(r0[0] * r0[0] - n0) < KH_SHADOW_ORTHO_TOL * n0 &&
        fabsf(r1[1] * r1[1] - n1) < KH_SHADOW_ORTHO_TOL * n1 &&
        fabsf(r2[2] * r2[2] - n2) < KH_SHADOW_ORTHO_TOL * n2) {
        return;
    }


    float axes[6][3] = {
        { r0[0], r0[1], r0[2] }, { r1[0], r1[1], r1[2] }, { r2[0], r2[1], r2[2] },   // rows
        { r0[0], r1[0], r2[0] }, { r0[1], r1[1], r2[1] }, { r0[2], r1[2], r2[2] },   // columns
    };
    float best = 0.0f;
    int best_axis = -1;

    for (int i = 0; i < 6; ++i) {
        const float c = shadow_axis_abs_cos(axes[i][0], axes[i][1], axes[i][2]);
        if (c > best) { best = c; best_axis = i; }
    }

    if (best > g_sr_best_dot43) g_sr_best_dot43 = best;

    if (best >= KH_SHADOW_AXIS_COS_TOL) {
        if (in_phase) {
            shadow_record_translation43(w[3], w[7], w[11]);

            // Scaled = projection-bearing: collect the whole family found in
            if (!uniform && g_sr_cascades_scratch.count < 8) {
                ShadowCascadeSet& cs = g_sr_cascades_scratch;
                cs.offsets[cs.count] = off;
                memcpy(cs.m[cs.count], w, sizeof(cs.m[cs.count]));
                cs.count++;
            }
        }

        // Each shape class keeps its own best: the uniform hit is the light
        // VIEW, the scaled hit carries the PROJECTION (an affine VP, or the
        // main pass's world->shadowUV sampling transform). In-phase wins
        // over out-of-phase at equal alignment.
        ShadowCapture43& slot = uniform ? g_sr_cap_view : g_sr_cap_vp;
        const bool better =
            !slot.valid ||
            best > slot.dot + 1e-6f ||
            (best >= slot.dot - 1e-6f && in_phase && !slot.in_phase);

        if (better) {
            slot.valid = true;
            slot.in_phase = in_phase;
            slot.dot = best;
            slot.axis = best_axis;
            slot.norms[0] = sqrtf(n0);
            slot.norms[1] = sqrtf(n1);
            slot.norms[2] = sqrtf(n2);
            memcpy(slot.m, w, sizeof(slot.m));
            slot.upload_floats = nfloats;
            slot.offset_floats = off;

            const uint32_t avail_before = off < KH_SHADOW_CTX_BEFORE ? off : KH_SHADOW_CTX_BEFORE;
            uint32_t avail_after = nfloats - off - 12;
            if (avail_after > KH_SHADOW_CTX_AFTER) avail_after = KH_SHADOW_CTX_AFTER;
            memset(slot.ctx, 0, sizeof(slot.ctx));
            slot.ctx_start = avail_before;

            memcpy(slot.ctx + (KH_SHADOW_CTX_BEFORE - avail_before),
                   base + off - avail_before,
                   (avail_before + 12 + avail_after) * sizeof(float));
        }
    }
}


// --- Live capture pieces --------------------------------------------------

// Atlas candidacy: SRV-capable square depth texture, generously sized. The
// engine's cascade passes dominate cycle counts on exactly one such target;
// keeping the LARGEST candidate resolves ties in its favor.
inline void shadow_live_consider_atlas(ID3D11Texture2D* tex, const D3D11_TEXTURE2D_DESC& td) {
    if ((td.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0) return;
    if (td.Width != td.Height || td.Width < 1024) return;
    if (td.SampleDesc.Count > 1 || td.ArraySize != 1) return;

    if (td.Format != DXGI_FORMAT_R32_TYPELESS && td.Format != DXGI_FORMAT_R24G8_TYPELESS &&
        td.Format != DXGI_FORMAT_R16_TYPELESS && td.Format != DXGI_FORMAT_R32G8X24_TYPELESS) {
        return;
    }

    void* id = static_cast<void*>(tex);
    if (id == g_ls.atlas_identity) return;
    if (g_ls.atlas_tex && td.Width <= g_ls.atlas_size) return;   // keep the largest

    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); }
    tex->AddRef();
    g_ls.atlas_tex = tex;

    g_ls.atlas_identity = id;
    g_ls.atlas_size = td.Width;
    g_ls.atlas_fmt = static_cast<uint32_t>(td.Format);
    g_ls.srv_failed = false;
    g_ls.cache_valid = false;   // new atlas: relearn the upload locator
}

// SAMPLING-transform test - stricter than the recon's generic scaled-
// orthogonal detector, because the cascade cycles also upload the light
// VIEW (a unit-norm rotation), and a first-match latch grabs that instead
// (diagnosed by a table of metersPerUV ~ 1 and view-space uvz in the
// dump). The sampling transform's shape signature, consistent across every
// capture: mutually orthogonal rows, ISOTROPIC u/v scales, and all scales
// far below 1 (they are 1/extent for extents of meters to kilometers).
inline bool shadow_live_test_window(const float* w) {
    const float* r0 = w + 0;
    const float* r1 = w + 4;
    const float* r2 = w + 8;
    const float n0 = r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2];
    const float n1 = r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2];
    const float n2 = r2[0] * r2[0] + r2[1] * r2[1] + r2[2] * r2[2];
    if (n0 < 1e-12f || n1 < 1e-12f || n2 < 1e-12f) return false;

    // Scale window: every genuine sampling transform observed sits at
    // 1/160 .. 1/24 per meter (plus a 1/600 long-range one); [1/1000, 1/8]
    // keeps that family with margin and rejects the 1/2.2-scale look-alikes
    // that slipped under a bare <0.5 ceiling.
    if (n0 > 0.015625f || n1 > 0.015625f) { g_stats.live_rej_scale++; return false; }                 // uv scale <= 1/8
    if (n0 < 1e-7f || n1 < 1e-7f) { g_stats.live_rej_scale++; return false; }   // floor 1/3162: LOW sun stretches extents                         // uv scale >= 1/1000
    // Depth scale within a factor ~20 of the uv scale (observed ~0.5-2x).
    if (n2 < 0.0025f * n0 || n2 > 400.0f * n0) { g_stats.live_rej_ratio++; return false; }
    // u/v isotropy (norms-squared within ~10% = norms within ~5%).
    if (fabsf(n1 - n0) > 0.1f * n0) { g_stats.live_rej_iso++; return false; }
    // THE discriminator: the translation IS the transform's output at the
    // camera, and the output space is atlas-UV + normalized depth - all
    // three components of every genuine capture lie in [0,1] ((0.47, 0.18,
    // 0.71), (0.82, 0.48, 0.45), ...), while view-shaped impostors carry
    // meters (47, -38, 66). Range padded for a camera near a tile edge.
    if (w[3] < -8.0f || w[3] > 9.0f) { g_stats.live_rej_trans++; return false; }
    if (w[7] < -8.0f || w[7] > 9.0f) { g_stats.live_rej_trans++; return false; }
    if (w[11] < -8.0f || w[11] > 9.0f) { g_stats.live_rej_trans++; return false; }
    const float d01 = r0[0] * r1[0] + r0[1] * r1[1] + r0[2] * r1[2];
    const float d02 = r0[0] * r2[0] + r0[1] * r2[1] + r0[2] * r2[2];
    const float d12 = r1[0] * r2[0] + r1[1] * r2[1] + r1[2] * r2[2];

    if (fabsf(d01) > KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n1) ||
        fabsf(d02) > KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n2) ||
        fabsf(d12) > KH_SHADOW_ORTHO_TOL * sqrtf(n1 * n2)) {
        g_stats.live_rej_ortho++;
        return false;
    }

    if (fabsf(r0[0] * r0[0] - n0) < KH_SHADOW_ORTHO_TOL * n0 &&
        fabsf(r1[1] * r1[1] - n1) < KH_SHADOW_ORTHO_TOL * n1 &&
        fabsf(r2[2] * r2[2] - n2) < KH_SHADOW_ORTHO_TOL * n2) {
        return false;   // scaled-identity false positives
    }

    for (int a = 0; a < 6; ++a) {
        float x, y, z;
        if (a < 3)  { const float* r = w + a * 4; x = r[0]; y = r[1]; z = r[2]; }
        else        { const int c = a - 3; x = r0[c]; y = r1[c]; z = r2[c]; }
        if (shadow_axis_abs_cos(x, y, z) >= KH_SHADOW_AXIS_COS_TOL) {
            g_stats.live_accepts++;

            return true;
        }
    }

    return false;
}

// Upload scan on the atlas cycle: locator-cached offset first, full window
// scan to (re)learn it. One latch per cycle; the tile arrives at the next
// draw.
// Remember recent CB uploads by BUFFER identity so the resolve draw can
// look up what its bound constants contain without any GPU readback.
// Match a 4x4 against the bridge view loosely: same rotation family
// (rows within tolerance), translation within a few meters - the engine
// upload is the SAME camera, possibly one frame fresher.
// Match a candidate 4x4 against a reference view (rows layout).
// 0 = no, 1 = direct, 2 = transposed. Tight tolerance: the loose matcher
// admitted multiple different view-family uploads per frame, and WHICH
// one had been latched last varied while moving - the cast wobble.
inline int shadow_view_match(const float* w, const float* v, float tol, float trans_tol) {
    if (fabsf(w[15] - 1.0f) > 1e-3f) return 0;
    float e_direct = 0.0f, e_trans = 0.0f;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            e_direct += fabsf(w[r * 4 + c] - v[r * 4 + c]);
            e_trans += fabsf(w[c * 4 + r] - v[r * 4 + c]);
        }
    }

    if (e_direct < tol) {
        for (int c = 0; c < 3; ++c) {
            if (fabsf(w[12 + c] - v[12 + c]) > trans_tol) return 0;
        }

        return 1;
    }

    if (e_trans < tol) {
        for (int r = 0; r < 3; ++r) {
            if (fabsf(w[r * 4 + 3] - v[12 + r]) > trans_tol) return 0;
        }

        return 2;
    }

    return 0;
}

inline bool shadow_view_shaped(const float* w) {
    if (fabsf(w[15] - 1.0f) > 1e-3f) return false;
    const float n0 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    const float n1 = w[4] * w[4] + w[5] * w[5] + w[6] * w[6];
    if (n0 < 0.9f || n0 > 1.1f || n1 < 0.9f || n1 > 1.1f) return false;
    const float d01 = w[0] * w[4] + w[1] * w[5] + w[2] * w[6];
    return fabsf(d01) < 0.1f;
}

inline void shadow_view_scan(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!reorder_on_render_thread() || bytes < 64 || !res) return;
    const float* f = static_cast<const float*>(data);
    const uint32_t nf = bytes / 4;

    for (uint32_t off = 0; off + 16 <= nf && off <= 192; off += 4) {
        const float* w = f + off;

        if (g_ls.view_src_valid && res == g_ls.view_src_res && off == g_ls.view_src_off) {
            // The locked location is a STREAMING buffer: the engine maps
            // the same dynamic CB ~1,600x per frame for per-object
            // uploads (1.93M reads/session taught us this). Location
            // alone identifies a firehose; the true view is fished out
            // of it by its unfakeable fingerprint: translation EXACTLY
            // zero (camera-relative: eye at origin) AND rotation within
            // a frame of the bridge's. Unrotated world objects match the
            // rotation but sit meters away; camera-attached items sit at
            // zero but are counter-rotated. Nothing matches both.
            float fv[16];

            if (g_ls.view_src_orient == 1) {
                memcpy(fv, w, sizeof(fv));
            } else {
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        fv[r * 4 + c] = w[c * 4 + r];
                    }
                }
            }

            if (g_ls.view_src_relative) {
                const float tmag = fabsf(fv[12]) + fabsf(fv[13]) + fabsf(fv[14]);
                if (tmag > 0.05f) continue;   // an object, not the view

                if (g_ro.cycle_pv_valid) {
                    const float* bv = &g_ro.cycle_pv.view[0][0];
                    float e_rot = 0.0f;

                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            e_rot += fabsf(fv[r * 4 + c] - bv[r * 4 + c]);
                        }
                    }

                    // Family filter only: non-view families differ by >>1.
                    // 0.15 was speed-fragile (the bridge lags a frame; a
                    // fast spin exceeded it, reads stalled, the health
                    // check evicted the lock, and seals fell to the skewed
                    // fallback until relearn - the mid-session received
                    // drift that 'calmed down halfway through').
                    if (e_rot > 0.35f) continue;
                    g_ls.last_publish_rot_err = e_rot;   // cold fire quality gate
                }
            }

            if (g_ls.view_src_relative && g_ro.cycle_pv_valid) {
                // HYBRID: rotation from the engine's same-frame upload
                // (the phase-critical part); translation rebuilt as
                // -cam * R_fresh, camera extracted from the bridge's own
                // consistent pair (cam = -t_bridge * R_bridge^T). A one-
                // frame-late camera is centimeters under motion - the
                // component the eye demonstrably cannot see.
                const float* bv = &g_ro.cycle_pv.view[0][0];
                float cam[3];

                for (int j = 0; j < 3; ++j) {
                    cam[j] = -(bv[12] * bv[j * 4 + 0] + bv[13] * bv[j * 4 + 1] + bv[14] * bv[j * 4 + 2]);
                }

                for (int c = 0; c < 3; ++c) {
                    fv[12 + c] = -(cam[0] * fv[0 * 4 + c] + cam[1] * fv[1 * 4 + c] + cam[2] * fv[2 * 4 + c]);
                }

                fv[15] = 1.0f;
            }

            if (g_ls.frame_view_valid) {
                memcpy(g_ls.prev_view, g_ls.frame_view, sizeof(g_ls.prev_view));
                g_ls.prev_view_valid = true;
            }

            memcpy(g_ls.frame_view, fv, sizeof(g_ls.frame_view));
            g_ls.frame_view_valid = true;
            g_ls.frame_view_time = effect_time_seconds();
            g_ls.frame_view_hits++;
            g_ls.view_published_this_frame = true;
            if (!g_mask_cast_fired) g_mask_cast_arm = true;   // once per frame (engine-mask path)

            // complete any seals waiting for this frame's view
            for (int bi = 0; bi < 8; ++bi) {
                auto& bs = g_ls.band[bi];
                if (!bs.valid || !bs.pending_view) continue;

                for (int c = 0; c < 3; ++c) {
                    for (int r = 0; r < 4; ++r) {
                        bs.vcol[c * 4 + r] = fv[r * 4 + c];
                    }
                }

                bs.pending_view = false;
                g_ls.seal_completions++;

            }

            return;
        }

        if (!g_ls.view_src_valid && shadow_view_shaped(w)) {
            // RELATIVE FINGERPRINT at admission: ~1,600 streaming object
            // uploads per frame flood a 16-slot ring in microseconds -
            // after an eviction (e.g. a tab-out stalls uploads past the
            // health limit) the true view could never survive in the
            // ring long enough to relearn, and everything downstream
            // froze (camera-following cast, 91% incomplete seals).
            // Objects sit meters away; the view's translation is ~0.
            const float tmag_r = fabsf(w[12]) + fabsf(w[13]) + fabsf(w[14]);
            const float tmag_c = fabsf(w[3]) + fabsf(w[7]) + fabsf(w[11]);

            if (tmag_r < 10.0f || tmag_c < 10.0f) {
                const uint32_t i = g_ls.vc_n & 15;
                g_ls.vc_ring[i].res = res;
                g_ls.vc_ring[i].off = off;
                memcpy(g_ls.vc_ring[i].m, w, 64);
                g_ls.vc_n++;
            }
        }
    }
}

inline void shadow_register_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 48) return;   // cap removed: big CBs hold the maps

    for (uint32_t i = 0; i < 16; ++i) {
        if (g_ls.cb_reg[i].buf == res) {
            g_ls.cb_reg[i].floats = bytes / 4 <= 512 ? bytes / 4 : 512;
            g_ls.cb_reg[i].seq = ++g_ls.cb_reg_seq;
            memcpy(g_ls.cb_reg[i].data, data, g_ls.cb_reg[i].floats * sizeof(float));
            return;
        }
    }

    CbRegSlot:
    {
        auto& r = g_ls.cb_reg[g_ls.cb_reg_next];
        g_ls.cb_reg_next = (g_ls.cb_reg_next + 1) & 15;
        r.seq = ++g_ls.cb_reg_seq;
        r.buf = res;
        r.floats = bytes / 4 <= 512 ? bytes / 4 : 512;
        memcpy(r.data, data, r.floats * sizeof(float));
    }
}

inline void shadow_live_upload(const void* data, uint32_t bytes) {
    // No table-fullness gate here: the table is PERSISTENT and a full one
    // is its normal steady state - uploads must keep flowing so entries
    // REPLACE in place as cascades re-render. (A leftover per-frame-era
    // cap here froze the whole pipeline after the first eight latches:
    // transforms, the newest pointer, and the fine copy all fossilized.)
    // pending_valid does NOT gate: TWO constant blocks carry sampling-family
    // transforms (the 1696-byte general per-cascade block and a 192-byte
    // specialized one), and first-upload-wins made the (transform, content)
    // pairing a per-cycle coin flip - the residual "occasional drift". A
    // candidate from a LARGER upload upgrades the pending latch: the big
    // block is the cycle's own cascade constants.
    if (!shadow_live_wanted() || !g_ls.phase_on_atlas) return;
    if (bytes < 12 * sizeof(float)) return;
    if (bytes > 16384) bytes = 16384;
    const float* f = static_cast<const float*>(data);
    const uint32_t nfloats = bytes / 4;

    // STASH-ONLY scan once the cycle has latched: the once-per-cycle gate
    // also silenced test_window (whose accept feeds the pass-end cast
    // stash) for every pass after the first - the stash starved and
    // castDraws pinned to shadowLiveLatches three builds running. The
    // latch bookkeeping below stays once-per-cycle; the SCAN does not.
    if (g_ls.cycle_latched) {
        for (uint32_t off = 0; off + 12 <= nfloats && off <= 128; off += 4) {
            if (shadow_live_test_window(f + off)) break;   // accept stashes internally
        }

        return;
    }

    // Keep the small sampling block of the cycle regardless of who wins
    // the pending race - it complements the big block per commit.
    if (bytes <= 256) {
        const float* ff = static_cast<const float*>(data);
        const uint32_t nf = static_cast<uint32_t>(bytes / 4);

        for (uint32_t off = 0; off + 12 <= nf && off <= 8; off += 4) {
            if (shadow_live_test_window(ff + off)) {
                break;
            }
        }
    }

    if (g_ls.pending_valid) {
        if (bytes < g_ls.pending_bytes) return;   // never downgrade

        if (bytes == g_ls.pending_bytes) {
            // Detect whether a same-size successor even differs (conflict
            // accounting), and honor the pick order.
            bool differs = false;

            if (g_ls.cache_valid && bytes == g_ls.cache_bytes && g_ls.cache_offset + 12 <= nfloats) {
                const float* cand = f + g_ls.cache_offset;
                differs = fabsf(cand[3] - g_ls.pending_m[3]) > 1e-6f ||
                          fabsf(cand[7] - g_ls.pending_m[7]) > 1e-6f ||
                          fabsf(cand[11] - g_ls.pending_m[11]) > 1e-6f;
                if (differs) g_stats.same_size_conflicts++;
            }

            if (g_shadow_map_pick < 0.5f) return;   // first wins: keep pending
        }
    }

    if (g_ls.cache_valid && bytes == g_ls.cache_bytes && g_ls.cache_offset + 12 <= nfloats) {
        if (shadow_live_test_window(f + g_ls.cache_offset)) {
            memcpy(g_ls.pending_m, f + g_ls.cache_offset, sizeof(g_ls.pending_m));
            g_ls.pending_valid = true;
            g_ls.pending_bytes = static_cast<uint32_t>(bytes);
            return;
        }
    }

    for (uint32_t off = 0; off + 12 <= nfloats; off += 4) {
        if (shadow_live_test_window(f + off)) {
            g_ls.cache_valid = true;
            g_ls.cache_bytes = static_cast<uint32_t>(bytes);
            g_ls.cache_offset = off;
            memcpy(g_ls.pending_m, f + off, sizeof(g_ls.pending_m));
            g_ls.pending_valid = true;
            g_ls.pending_bytes = static_cast<uint32_t>(bytes);
            return;
        }
    }
}

// A draw inside the atlas cycle has the cascade's viewport bound: snapshot
// it ONCE per cycle. The pending matrix is NOT committed here - the engine
// interleaves uploads and draws, and committing at the first draw let a
// small specialized CB win the latch before the big per-cascade block ever
// uploaded (the preference existed but the finalize raced it - the exact
// mechanism behind pairs that were internally crossed). Commit happens at
// CYCLE EXIT, after every upload had its chance to upgrade the pending.
inline void shadow_live_on_draw(ID3D11DeviceContext* ctx) {
    if (!g_ls.pending_valid || g_ls.pending_vp_valid || !g_ls.phase_on_atlas || !ctx || g_ls.atlas_size == 0) return;
    UINT nvp = 1;
    D3D11_VIEWPORT vp = {};
    ctx->RSGetViewports(&nvp, &vp);
    if (nvp == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f) return;
    g_ls.pending_vp[0] = vp.TopLeftX;
    g_ls.pending_vp[1] = vp.TopLeftY;
    g_ls.pending_vp[2] = vp.Width;
    g_ls.pending_vp[3] = vp.Height;
    g_ls.pending_vp_valid = true;
}

// Cycle exit: the fully-upgraded pending latch becomes a table entry, and
// arms the fine copy when its cascade is the finest known.
inline void shadow_live_finalize_cycle() {
    if (!g_ls.pending_valid || !g_ls.pending_vp_valid || g_ls.atlas_size == 0) {
        g_ls.pending_valid = false;
        g_ls.pending_bytes = 0;
        g_ls.pending_vp_valid = false;
        return;
    }

    const uint32_t winner_bytes = g_ls.pending_bytes;
    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_ls.pending_vp_valid = false;
    if (winner_bytes >= 512) g_stats.commits_large++;
    else                     g_stats.commits_small++;

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = g_ls.pending_vp[0];
    vp.TopLeftY = g_ls.pending_vp[1];
    vp.Width = g_ls.pending_vp[2];
    vp.Height = g_ls.pending_vp[3];

    // Slot selection: same cascade = same scale (each cascade's extent is
    // distinct); a re-render replaces its entry in place. New scales append;
    // a full table evicts the stalest entry.
    const float* pm = g_ls.pending_m;
    const float pn0 = pm[0] * pm[0] + pm[1] * pm[1] + pm[2] * pm[2];
    int slot = -1;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        const float* m = g_ls.entries[i].m;
        const float n0 = m[0] * m[0] + m[1] * m[1] + m[2] * m[2];
        if (fabsf(n0 - pn0) <= 0.2f * n0) { slot = static_cast<int>(i); break; }
    }

    if (slot < 0) {
        if (g_ls.count < KH_LIVE_MAX_CASCADES) {
            slot = static_cast<int>(g_ls.count++);
        } else {
            uint64_t oldest = ~0ull;

            for (uint32_t i = 0; i < g_ls.count; ++i) {
                if (g_ls.entries[i].stamp < oldest) { oldest = g_ls.entries[i].stamp; slot = static_cast<int>(i); }
            }
        }
    }

    const float inv = 1.0f / static_cast<float>(g_ls.atlas_size);
    const float texel = inv;
    LiveShadowEntry& e = g_ls.entries[slot];
    memcpy(e.m, g_ls.pending_m, sizeof(e.m));
    e.tile[0] = vp.TopLeftX * inv + texel;
    e.tile[1] = vp.TopLeftY * inv + texel;
    e.tile[2] = (vp.TopLeftX + vp.Width) * inv - texel;
    e.tile[3] = (vp.TopLeftY + vp.Height) * inv - texel;
    if (e.tile[2] <= e.tile[0] || e.tile[3] <= e.tile[1]) return;
    e.cam[0] = g_ls.cam[0];   // last injection's camera ~ this frame's origin
    e.cam[1] = g_ls.cam[1];
    e.cam[2] = g_ls.cam[2];
    e.stamp = ++g_ls.stamp_counter;
    g_ls.newest = slot;
    g_ls.cycle_latched = true;
    g_stats.shadow_live_latches++;

}

inline bool shadow_probe_target(ID3D11DepthStencilView* dsv) {
    if (!dsv) return false;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return false;

    ID3D11Texture2D* tex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (!tex) return false;

    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    shadow_live_consider_atlas(tex, td);
    const bool is_atlas = static_cast<void*>(tex) == g_ls.atlas_identity;

    D3D11_DEPTH_STENCIL_VIEW_DESC vd = {};
    dsv->GetDesc(&vd);
    uint32_t slice = 0;

    if (vd.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DARRAY) {
        slice = vd.Texture2DArray.FirstArraySlice;
    } else if (vd.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY) {
        slice = vd.Texture2DMSArray.FirstArraySlice;
    }

    void* id = static_cast<void*>(tex);
    tex->Release();   // identity only; the desc is already copied out

    return is_atlas;   // (recon-era target census removed)
}

// Frame boundary (main depth clear): publish the size, start a new table.
inline void shadow_live_frame_reset() {
    // The table PERSISTS - entries stay valid until their cascade
    // re-renders and replaces them. Frame boundary is statistics only.
    g_stats.shadow_live_cascades = g_ls.count;
    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
}

// Depth probe: 64x64 texel readback around the probe point's UV in the
// FINE COPY, searching for the texel whose depth best matches the expected
// value. dx/dy of that argmin IS the registration error in texels. Runs on
// the render thread from the injection, throttled; the Map(READ) stall on
// a 16 KB staging copy is negligible at 1 Hz.
static constexpr int KH_PROBE_R = 80;   // half-size: 160x160 texel window
                                        // (~0.5 m at the fine cascade - a
                                        // player-torso landmark fits, and a
                                        // compact blob defeats the flat-
                                        // ground isodepth degeneracy that
                                        // pinned the old 64x64 argmin to a
                                        // corner)

// --- Screen-space shadow mask -----------------------------------------
// The engine's shadow-RESOLVE pass is the draw that SAMPLES the atlas; the
// render target it writes at that moment is the full-screen shadow mask
// (the engine's finished answer). Sampling that mask at our own pixel's
// screen position is registration-exact BY CONSTRUCTION - no transforms,
// no origins, no cascades.
struct MaskCandidate {
    void*    key = nullptr;        // texture identity (weak)
    uint32_t w = 0, h = 0, fmt = 0;
    uint64_t draws = 0;
};

struct ShadowMaskState {
    bool atlas_bound = false;          // atlas seen in PS SRVs since last target change
    bool  rt_is_resolve = false;       // current RT0 is the screen-sized single-channel resolve target
    void* rt_key = nullptr;            // current RT0 resource (weak, for candidate draw counts)
    MaskCandidate cands[8];
    uint32_t cand_count = 0;
    ID3D11Texture2D*          tex = nullptr;   // chosen mask (AddRef held)
    ID3D11ShaderResourceView* srv = nullptr;
    void* chosen_key = nullptr;
    float dims[2] = {};                // chosen mask w, h
    // --- Analytic mask cast: the box's shadow drawn INTO the engine's
    // screen-space mask, per-pixel, with the true same-frame view (the
    // supervised latch fires at the main pass, BEFORE the mask is
    // consumed). Depth-reconstruct (Nsight recipe: linear depth t0,
    // vpos = (ndc*PSC_FOV,1)*z), fold to world, ray-vs-box toward the
    // sun. No atlas, no cascades, no prediction: jitter is impossible.
    // OUR OWN offscreen shadow factor texture. Writing into engine-owned
    // buffers is over: rt_is_resolve matches a SIGNATURE (screen-sized
    // single-channel) that RV's AO and cloud-shadow buffers share - only
    // a human eye ever discriminated them (candidate 0). Engine buffers
    // are read-only to us now; the darkening composites onto the scene
    // at OUR injection, where the target is known because we already
    // draw into it.
    ID3D11Texture2D*          own_tex = nullptr;
    ID3D11RenderTargetView*   own_rtv = nullptr;
    ID3D11ShaderResourceView* own_srv = nullptr;
    bool own_failed = false;
    bool own_written = false;   // this frame's factor is ready to composite
    // ENGINE-MASK path (the hybrid): the analytic shadow written into the
    // engine's own R8 resolve output - the field-verified candidate-0
    // texture, discriminated by atlas-sampling evidence - with MIN
    // (darken-only) blending. The ENGINE then colors it with its exact
    // ambient and tonemapping: analytic position, engine-perfect color.
    ID3D11RenderTargetView* engine_mask_rtv = nullptr;   // captured at the gated resolve sweep
    uint64_t mask_rtv_swaps = 0;    // capture-time target CHANGES (>: partitions use separate masks)
    uint64_t cast_batches = 0;      // resolve-batch re-arms
    // cold chain, reborn lean: seconds from the first boxed flush to
    // each milestone (-1 until reached), plus the leading rejection
    // streak - the spawn-delay question answered in one log.
    double cold_t0 = -1.0;
    float  cold_first_inject = -1.0f;   // first SUCCESSFUL injection
    float  cold_first_cast = -1.0f;     // first analytic mask write
    uint32_t cold_lead_ambiguous = 0;   // ambiguous rejections before the first injection
    float  cold_first_stage = -1.0f;    // first flush that staged >= 1 box for injection
    float  cold_first_trigger = -1.0f;  // first entry into the trigger body (past silent guards)
    uint32_t cold_g_nodsv = 0;          // cold exits: no main DSV this cycle
    uint32_t cold_g_floor = 0;          // cold exits: opaque floor
    uint32_t cold_g_sparse = 0;         // cold exits: sparse frame off main depth
    uint32_t cold_cast_miss = 0;        // sticky: last castMisses code seen while the first cast was pending
    float last_inject_near = -1.0f;     // the near plane of the LAST injection (slice forensics)
    // overlay trisection gauges (per latest flush, not cumulative)
    uint32_t ov_listed = 0;    // mode-2 solids in the flush's box list
    uint32_t ov_skipped = 0;   // skipped by the comp_healthy gate (must stay 0 for mode-2)
    uint32_t ov_drawn = 0;     // Draw() actually issued
    ID3D11BlendState*       min_blend = nullptr;         // out = min(dst, src)
    bool engine_mask_failed = false;
    ID3D11ShaderResourceView* cast_depth = nullptr;   // scene linear depth (AddRef)
    float cast_dims[2] = {};
    float cast_fov[4] = {};
    bool  cast_fov_valid = false;
    ID3D11BlendState*        cast_blend = nullptr;    // dst *= src
    ID3D11DepthStencilState* cast_dss = nullptr;      // depth off
    bool  cast_states_failed = false;
    uint64_t analytic_casts = 0;
    uint64_t mask_applies = 0;    // scene-end composites actually drawn
    // Scene-end apply: the injection's position among the translucent
    // draws MOVES as the gate rearms - some frames before the road
    // decals, some after - so applying at the injection let decals paint
    // over the shadow on alternating frames (the flicker; the user's
    // under/over-decal observation was the smoking gun). Instead: hold
    // the scene RTV (identity known - we just drew boxes into it) and
    // apply when the engine UNBINDS it. Post-everything, deterministic.
    ID3D11RenderTargetView* scene_rtv = nullptr;   // AddRef at injection
    void* scene_key = nullptr;
    bool  scene_apply_armed = false;
    // SQF-set params (game thread writes, plain floats)
    // Mask mode is the DEFAULT shadow path: candidate 0 in the field test
    // (3840x2160 R8_UNORM, channel 0) proved to be the engine's shadow
    // resolve, and sampling it at each pixel's own screen position is
    // registration-exact by construction. The cascade path remains the
    // automatic fallback whenever no mask candidate exists yet.
    float enable = 1.0f;
    float index = -1.0f;               // -1 = auto
    float channel = 0.0f;
    float invert = 0.0f;
};

static ShadowMaskState g_mask;

// View-paired band capture: at this band's resolve, freeze SM + border
// (from the resolve CB, at fixed offsets relative to the found matrix) +
// this frame's view matrix + the atlas content, together.
inline void band_capture(ID3D11DeviceContext* ctx, const float* cb, uint32_t off, uint32_t nf) {
    if (!g_ro.cycle_pv_valid || !g_ls.atlas_tex) { g_ls.band_bail_pv++; return; }
    if (off + 20 > nf) { g_ls.band_bail_off++; return; }   // need matrix + border
    const float* sm = cb + off;
    const float* border = cb + off + 16;

    if (!(border[1] > border[0]) || border[0] < 0.0f || border[1] > 4000.0f) {
        g_ls.band_bail_border++;
        memcpy(g_ls.band_last_reject, border, sizeof(g_ls.band_last_reject));
        return;
    }
    // slot: match an existing band by near edge, else take invalid/oldest,
    // preferring to keep the two NEAREST bands
    int slot = -1;

    for (int i = 0; i < 8; ++i) {
        if (g_ls.band[i].valid && fabsf(g_ls.band[i].border[0] - border[0]) < 0.5f) { slot = i; break; }
    }

    if (slot < 0) {
        for (int i = 0; i < 8; ++i) {
            if (!g_ls.band[i].valid) { slot = i; break; }
        }
    }

    if (slot < 0) {   // all valid, different bands: replace the farthest if nearer
        int far_i = 0;

        for (int i = 1; i < 8; ++i) {
            if (g_ls.band[i].border[0] > g_ls.band[far_i].border[0]) far_i = i;
        }

        if (border[0] < g_ls.band[far_i].border[0]) slot = far_i;
        else { g_ls.band_bail_slot++; return; }
    }

    BandSlotRef:
    {
        auto& b = g_ls.band[slot];
        const float now = effect_time_seconds();
        // View source: the BRIDGE PV. The strongest evidence in every log
        // is the box itself - it renders pixel-registered to the world
        // every frame from this same transform, so it is same-frame
        // correct by demonstration. The upload-matching latch (three
        // designs, three pathologies: promiscuity, lockout, wrong-family
        // tracking) remains available as an opt-in experiment (elem 13)
        // and is used only when actually fresh - NEVER by skipping work:
        // the skips were the cast flicker and the reseal starvation.
        if (!g_ro.cycle_pv_valid && !g_ls.frame_view_valid) { g_ls.band_bail_view++; return; }
        // Big-jump bypass: a large window move (look-away/look-back) must
        // reseal IMMEDIATELY - the sub-second "shadow appears late" was
        // the throttle serving 0.25 s stale windows after a view swing.
        const bool big_jump = b.valid &&
            (fabsf(sm[3] - b.sm[3]) > 0.08f || fabsf(sm[7] - b.sm[7]) > 0.08f);
        // ROTATION-DELTA reseal: the cascade grid ROTATES with the view;
        // a frozen copy requantizes shadow edges by up to a texel at each
        // reseal - at 4 Hz that is a visible discrete hop proportional to
        // turn rate (the received-side "step-dance"; the engine never
        // freezes, so it spreads the same shimmer into per-frame dust).
        // Dense reseals while the grid turns convert the hop into the
        // same dust; a 0.05 s floor bounds the copy bandwidth.
        bool grid_turned = false;

        if (b.valid) {
            float n_new = sqrtf(sm[0] * sm[0] + sm[1] * sm[1] + sm[2] * sm[2]);
            float n_old = sqrtf(b.sm[0] * b.sm[0] + b.sm[1] * b.sm[1] + b.sm[2] * b.sm[2]);

            if (n_new > 1e-12f && n_old > 1e-12f) {
                const float d = (sm[0] * b.sm[0] + sm[1] * b.sm[1] + sm[2] * b.sm[2]) / (n_new * n_old);
                grid_turned = d < 0.9999995f;   // ~0.06 deg of grid rotation
            }
        }
        // TIME-ONLY seal: a view-space SM changes EVERY frame (the camera
        // is baked in), so any 'moved' bypass degenerates into full-rate
        // 67 MB copies against an in-flight SRV - a hazard-stall storm.
        // A frozen pair registers the static world perfectly by the
        // contract; 4 Hz reseals keep dynamic casters fresh enough.
        // TIERED reseal cadence. The dancing foliage taught us the frozen-
        // copy truth: received content can never be fresher than the
        // reseal rate, and RV vegetation both animates (wind) and
        // RE-ORIENTS WITH THE CAMERA (leaf planes are partially camera-
        // facing) - so its shadows genuinely evolve per frame, and copies
        // serve that evolution in steps. Near bands (magnified, where the
        // player looks) reseal at 20 Hz permanently; far bands stay calm,
        // stepping up while the grid turns. Bandwidth ~2-3 GB/s worst
        // case - an order below the copy storm that once cost the frame.
        const bool near_band = border[0] < 10.0f;
        const float min_interval = near_band ? 0.05f : (grid_turned ? 0.10f : 0.25f);
        if (b.valid && !big_jump && now - b.last_time < min_interval) { g_ls.band_bail_time++; return; }

        if (!b.tex) {
            // Batch: create ALL six slots' textures now - staggering six
            // 67 MB allocations across the first seals was the seconds of
            // shadow-less box at spawn.
            D3D11_TEXTURE2D_DESC td = {};
            g_ls.atlas_tex->GetDesc(&td);
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.Usage = D3D11_USAGE_DEFAULT;
            td.CPUAccessFlags = 0;
            td.MiscFlags = 0;
            ID3D11Device* dev = nullptr;
            g_ls.atlas_tex->GetDevice(&dev);
            if (!dev) return;

            for (int bi = 0; bi < 8; ++bi) {
                auto& bb = g_ls.band[bi];
                if (bb.tex) continue;

                if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &bb.tex)) && bb.tex) {
                    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
                    sd.Format = DXGI_FORMAT_R32_FLOAT;
                    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    sd.Texture2D.MipLevels = 1;

                    if (FAILED(dev->CreateShaderResourceView(bb.tex, &sd, &bb.srv))) {
                        bb.srv = nullptr;
                        bb.tex->Release();
                        bb.tex = nullptr;
                    }
                }
            }

            dev->Release();
            if (!b.tex || !b.srv) { g_stats.shadow_srv_failed++; return; }
        }

        memcpy(b.sm, sm, sizeof(b.sm));
        memcpy(b.border, border, sizeof(b.border));

        // DEFERRED VIEW: the main-scene view of THIS frame uploads only
        // after the resolves. Fill vcol provisionally (previous view -
        // exactly the transient we are killing) and mark pending: the
        // arriving upload completes the seal with the true same-frame V.
        const bool fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                        now - g_ls.frame_view_time < 0.05f;

        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 4; ++r) {
                b.vcol[c * 4 + r] = fv ? g_ls.frame_view[r * 4 + c]
                                       : g_ro.cycle_pv.view[r][c];
            }
        }

        b.pending_view = true;
        b.pending_since = now;

        ctx->CopyResource(b.tex, g_ls.atlas_tex);
        b.last_time = now;
        b.valid = true;
        b.copies++;
        g_ls.band_captures++;

        // sun direction from the engine's own matrices: SM depth row
        // (light axis, view space) through the view rotation transpose.
        if (g_ls.frame_view_valid) {
            const float* r2 = sm + 8;                  // view-space light axis
            const float* V = g_ls.frame_view;          // world->view (row-major rows = world axes in view space)
            float wdir[3];

            for (int c = 0; c < 3; ++c) {              // R^T * r2
                wdir[c] = V[c * 4 + 0] * r2[0] + V[c * 4 + 1] * r2[1] + V[c * 4 + 2] * r2[2];
            }

            const float n2 = sqrtf(wdir[0] * wdir[0] + wdir[1] * wdir[1] + wdir[2] * wdir[2]);

            if (n2 > 1e-6f) {
                for (int c = 0; c < 3; ++c) wdir[c] /= n2;
                if (wdir[1] < 0.0f) { wdir[0] = -wdir[0]; wdir[1] = -wdir[1]; wdir[2] = -wdir[2]; }   // skyward
                g_sun_dir_derived[0] = wdir[0];
                g_sun_dir_derived[1] = wdir[1];
                g_sun_dir_derived[2] = wdir[2];
                g_sun_dir_derived_valid = true;
            }
        }

    }
}

// At a resolve draw: the bound PS CBs are looked up in the registry; a
// sampling-family matrix found there IS the transform for the atlas
// content this very draw samples. If it's the finest scale, seal the pair
// right here: copy the atlas and store the matrix together.
inline void resolve_pair_capture(ID3D11DeviceContext* ctx) {
    if (g_shadow_pair_mode < 0.5f || !shadow_live_wanted() || !g_ls.atlas_tex) return;
    g_ls.resolve_draws++;
    // RV binds constants at varying slots: scan PS b0..b7 AND VS b0..b3.
    // The engine binds PSCB_PostProcess - the resolve's OWN constant
    // block, with the sampling matrix at [180] and the layer borders at
    // [196] - at slot cb13 (from its shader's own binding table). The
    // original 0..7 query never saw it: every 'resolve hit' this code has
    // ever processed came from the small VS-side block instead.
    ID3D11Buffer* bufs[18] = {};
    ctx->PSGetConstantBuffers(0, 14, bufs);
    ctx->VSGetConstantBuffers(0, 4, bufs + 14);
    if (g_shadow_pair_mode >= 1.5f) {
        if (g_ls.resolve_window == 0) {
            g_ls.resolve_gated++;

            for (int b = 0; b < 18; ++b) { if (bufs[b]) bufs[b]->Release(); }

            return;
        }

        g_ls.resolve_window--;
        // The mode-2 sweep must NOT sit behind the legacy first-hit gate:
        // that gate fails on most resolves (its window test rejects many
        // sun geometries), which starved the cast table to 0-1 entries a
        // frame while 4-8 cascade passes consumed indices - sparse,
        // misaligned cast matrices (jitter, angle-dependent vanishing).
        bool any_capture = false;

        for (int b = 0; b < 18; ++b) {
            if (!bufs[b]) continue;

            for (uint32_t i = 0; i < 16; ++i) {
                if (g_ls.cb_reg[i].buf != static_cast<ID3D11Resource*>(bufs[b])) continue;
                g_ls.resolve_cb_found++;
                const float* f = g_ls.cb_reg[i].data;
                const uint32_t nf = g_ls.cb_reg[i].floats;

                if (nf >= 200 && nf <= 512) {
                    const float* m180 = f + 180;
                    const float n0 = m180[0] * m180[0] + m180[1] * m180[1] + m180[2] * m180[2];
                    const float n1 = m180[4] * m180[4] + m180[5] * m180[5] + m180[6] * m180[6];

                    if (n0 > 1e-10f && n0 < 1.0f && n1 > 1e-10f && n1 < 1.0f) {
                        g_ls.resolve_seen_since_cast = true;   // partition's resolves ran: re-arm the mask write

                        // dual-gated arm: fire only when BOTH this frame's
                        // capture (here) and this frame's view publish have
                        // happened. Capture-only arming dragged the shadow a
                        // frame behind the camera; boundary arming used stale
                        // depth (the overcast). Field-verified.
                        if (!g_mask_cast_fired && g_ls.frame_view_valid && g_ls.view_published_this_frame) g_mask_cast_arm = true;
                        // analytic-cast inputs: mask RTV, depth SRV, FOV.
                        // GATED on the proven resolve classifier: the 424
                        // block rides cb13 across the whole postprocess
                        // chain, and ungated capture latched cloud/AO/
                        // lighting buffers (also screen-sized single-
                        // channel) - our multiply-draw then stomped them
                        // (black clouds, flickering exposure, no visible
                        // cast). rt_is_resolve identified candidate 0 in
                        // the mask-mode field test; trust it.
                        if (g_mask.rt_is_resolve) {
                        if (nf >= 180) {
                            memcpy(g_mask.cast_fov, f + 176, sizeof(g_mask.cast_fov));
                            g_mask.cast_fov_valid = true;
                        }

                        {
                            // dims from the RTV desc (READ only; no latch)
                            ID3D11RenderTargetView* rtv0 = nullptr;
                            ctx->OMGetRenderTargets(1, &rtv0, nullptr);

                            if (rtv0) {
                                ID3D11Resource* rr = nullptr;
                                rtv0->GetResource(&rr);

                                if (rr) {
                                    ID3D11Texture2D* rt = nullptr;
                                    rr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&rt));

                                    if (rt) {
                                        D3D11_TEXTURE2D_DESC rd = {};
                                        rt->GetDesc(&rd);
                                        g_mask.cast_dims[0] = static_cast<float>(rd.Width);
                                        g_mask.cast_dims[1] = static_cast<float>(rd.Height);
                                        rt->Release();
                                    }

                                    rr->Release();
                                }

                                // hold the mask RTV for the engine-mask
                                // hybrid: we are AT the resolve, behind
                                // both proven gates - this IS the mask.
                                if (g_mask.engine_mask_rtv != rtv0) {
                                    if (g_mask.engine_mask_rtv) {
                                        g_mask.engine_mask_rtv->Release();
                                        g_mask.mask_rtv_swaps++;   // target CHANGED mid-session
                                    }
                                    g_mask.engine_mask_rtv = rtv0;   // keep the ref
                                } else {
                                    rtv0->Release();
                                }
                            }

                            ID3D11ShaderResourceView* d0 = nullptr;
                            ctx->PSGetShaderResources(0, 1, &d0);

                            if (d0) {
                                // must be a screen-sized single-channel
                                // float texture (the linear depth)
                                bool depth_ok = false;
                                ID3D11Resource* dr = nullptr;
                                d0->GetResource(&dr);

                                if (dr) {
                                    ID3D11Texture2D* dt = nullptr;
                                    dr->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&dt));

                                    if (dt) {
                                        D3D11_TEXTURE2D_DESC dd2 = {};
                                        dt->GetDesc(&dd2);
                                        const int df = static_cast<int>(dd2.Format);
                                        depth_ok = dd2.Width == static_cast<UINT>(g_mask.cast_dims[0]) &&
                                                   (df == 39 || df == 40 || df == 41 || df == 46 || df == 54);
                                        dt->Release();
                                    }

                                    dr->Release();
                                }

                                if (depth_ok && g_mask.cast_depth != d0) {
                                    if (g_mask.cast_depth) g_mask.cast_depth->Release();
                                    g_mask.cast_depth = d0;   // keep the ref
                                } else {
                                    d0->Release();
                                }
                            }
                        }
                        }   // rt_is_resolve gate

                        const uint64_t before = g_ls.band_captures + g_ls.band_bail_time + g_ls.band_bail_slot;
                        band_capture(ctx, f, 180, nf);
                        if (g_ls.band_captures + g_ls.band_bail_time + g_ls.band_bail_slot != before) any_capture = true;
                    }
                }
            }
        }

        if (any_capture) g_ls.resolve_hits++;

        for (int b = 0; b < 18; ++b) { if (bufs[b]) bufs[b]->Release(); }

        return;
    }

    const float* hit = nullptr;
    const float* hit_cb = nullptr;
    uint32_t hit_off = 0, hit_nf = 0;
    float hit_n0 = 0.0f;

    for (int b = 0; b < 18; ++b) {
        if (!bufs[b] || hit) { continue; }

        for (uint32_t i = 0; i < 16 && !hit; ++i) {
            if (g_ls.cb_reg[i].buf != static_cast<ID3D11Resource*>(bufs[b])) continue;
            g_ls.resolve_cb_found++;
            const float* f = g_ls.cb_reg[i].data;
            const uint32_t nf = g_ls.cb_reg[i].floats;

            for (uint32_t off = 0; off + 12 <= nf; off += 4) {
                if (shadow_live_test_window(f + off)) {
                    hit = f + off;
                    hit_cb = f;
                    hit_off = off;
                    hit_nf = nf;
                    hit_n0 = f[off] * f[off] + f[off + 1] * f[off + 1] + f[off + 2] * f[off + 2];
                    break;
                }
            }
        }
    }

    for (int b = 0; b < 18; ++b) { if (bufs[b]) bufs[b]->Release(); }
    if (!hit) return;
    g_ls.resolve_hits++;


    if (hit_n0 > g_ls.resolve_best_n0) g_ls.resolve_best_n0 = hit_n0;
    if (hit_n0 < 0.8f * g_ls.resolve_best_n0) return;   // not the finest cascade's resolve
}

// Per-RT-change classification: runs at OMSet rate (dozens/frame),
// NOT per draw: the per-draw OMGet/QueryInterface/GetDesc chain ran for
// every atlas-sampling forward draw (tens of thousands/frame) and cost a
// measured ~170 ms/frame - the entire framerate collapse.
inline void mask_classify_rt(UINT n, ID3D11RenderTargetView* const* rtvs) {
    g_mask.rt_is_resolve = false;
    g_mask.rt_key = nullptr;
    if (!n || !rtvs || !rtvs[0]) return;
    ID3D11Resource* res = nullptr;
    rtvs[0]->GetResource(&res);
    if (!res) return;
    ID3D11Texture2D* tex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (!tex) return;
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    g_mask.rt_key = tex;

    bool known = false;

    for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
        if (g_mask.cands[i].key == tex) { known = true; break; }
    }

    if (!known && g_mask.cand_count < 8) {
        MaskCandidate& c = g_mask.cands[g_mask.cand_count++];
        c.key = tex;
        c.w = td.Width;
        c.h = td.Height;
        c.fmt = static_cast<uint32_t>(td.Format);
        c.draws = 0;
    }

    g_mask.rt_is_resolve =
        td.Width >= 1280 &&
        (td.Format == DXGI_FORMAT_R8_UNORM || td.Format == DXGI_FORMAT_R8_TYPELESS ||
         td.Format == DXGI_FORMAT_R16_UNORM || td.Format == DXGI_FORMAT_R16_TYPELESS ||
         td.Format == DXGI_FORMAT_R16_FLOAT);
    tex->Release();
}

// Per-draw path: two boolean tests and (rarely) the capture.
// Draw the boxes' analytic shadows into the engine's mask. Fires once
// per frame, at the first draw after the view latch publishes - after
// every resolve, before the mask is consumed.
inline void mask_cast_fire(ID3D11DeviceContext* ctx) {
    if (!ctx) return;   // called from the injection, once per frame
    if (g_shadow_cast < 0.5f) return;
    if (!g_res.initialized || !g_res.ps_maskcast || !g_res.vs_fullscreen || !g_res.composite_cb) return;
    if (!g_mask.cast_depth || !g_mask.cast_fov_valid || g_mask.own_failed) return;
    if (!g_ls.frame_view_valid || g_mask.cast_dims[0] < 1.0f) return;

    if (!g_mask.own_tex) {
        ID3D11Device* dev0 = nullptr;
        ctx->GetDevice(&dev0);
        if (!dev0) { g_mask.own_failed = true; return; }
        D3D11_TEXTURE2D_DESC td0 = {};
        td0.Width = static_cast<UINT>(g_mask.cast_dims[0]);
        td0.Height = static_cast<UINT>(g_mask.cast_dims[1]);
        td0.MipLevels = 1;
        td0.ArraySize = 1;
        td0.Format = DXGI_FORMAT_R8_UNORM;
        td0.SampleDesc.Count = 1;
        td0.Usage = D3D11_USAGE_DEFAULT;
        td0.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        if (FAILED(dev0->CreateTexture2D(&td0, nullptr, &g_mask.own_tex)) ||
            FAILED(dev0->CreateRenderTargetView(g_mask.own_tex, nullptr, &g_mask.own_rtv)) ||
            FAILED(dev0->CreateShaderResourceView(g_mask.own_tex, nullptr, &g_mask.own_srv))) {
            g_mask.own_failed = true;
            dev0->Release();
            return;
        }

        dev0->Release();
    }

    if (!g_mask.cast_blend && !g_mask.cast_states_failed) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) { g_mask.cast_states_failed = true; return; }
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;   // dst *= src
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = FALSE;

        if (FAILED(dev->CreateBlendState(&bd, &g_mask.cast_blend)) ||
            FAILED(dev->CreateDepthStencilState(&dd, &g_mask.cast_dss))) {
            g_mask.cast_states_failed = true;
            dev->Release();
            return;
        }

        dev->Release();
    }

    if (!g_mask.cast_blend || !g_mask.cast_dss) return;
    struct CastBox { float pos[3]; float size[3]; };
    CastBox cboxes[64];   // 16 starved the stress test's 31st box
    uint32_t ncb = 0;

    {
        std::lock_guard<std::mutex> lk(g_draw_list_mutex);

        for (const auto& kv2 : g_draw_list) {
            if (ncb >= 64) break;
            const auto& o = kv2.second;
            memcpy(cboxes[ncb].pos, o.pos, 12);
            memcpy(cboxes[ncb].size, o.size, 12);
            ncb++;
        }
    }

    if (ncb == 0) return;
    g_ro.in_injection = true;
    ID3D11RenderTargetView* old_rtvs[4] = {};
    ID3D11DepthStencilView* old_dsv = nullptr;
    ctx->OMGetRenderTargets(4, old_rtvs, &old_dsv);
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);
    ID3D11VertexShader* old_vs = nullptr;
    ID3D11PixelShader* old_ps = nullptr;
    ID3D11InputLayout* old_il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY old_topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* old_cb = nullptr;
    ID3D11BlendState* old_blend = nullptr;
    FLOAT old_bf[4] = {};
    UINT old_bmask = 0xFFFFFFFF;
    ID3D11DepthStencilState* old_dss = nullptr;
    UINT old_sref = 0;
    ID3D11ShaderResourceView* old_t0 = nullptr;
    ctx->VSGetShader(&old_vs, nullptr, nullptr);
    ctx->PSGetShader(&old_ps, nullptr, nullptr);
    ctx->IAGetInputLayout(&old_il);
    ctx->IAGetPrimitiveTopology(&old_topo);
    ctx->VSGetConstantBuffers(0, 1, &old_cb);
    ctx->OMGetBlendState(&old_blend, old_bf, &old_bmask);
    ctx->OMGetDepthStencilState(&old_dss, &old_sref);
    ctx->PSGetShaderResources(0, 1, &old_t0);
    const FLOAT white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(g_mask.own_rtv, white);
    ctx->OMSetRenderTargets(1, &g_mask.own_rtv, nullptr);
    D3D11_VIEWPORT vpn = {};
    vpn.Width = g_mask.cast_dims[0];
    vpn.Height = g_mask.cast_dims[1];
    vpn.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpn);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskcast, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
    const FLOAT bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.cast_blend, bf, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
    ctx->PSSetShaderResources(0, 1, &g_mask.cast_depth);

    for (uint32_t bi = 0; bi < ncb; ++bi) {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) break;
        ConstantData cbd = {};
        cbd.center_size[0] = cboxes[bi].pos[0];
        cbd.center_size[1] = cboxes[bi].pos[2];   // SQF -> engine axes
        cbd.center_size[2] = cboxes[bi].pos[1];
        cbd.size_axes[0] = cboxes[bi].size[0];
        cbd.size_axes[1] = cboxes[bi].size[2];
        cbd.size_axes[2] = cboxes[bi].size[1];

        for (int r = 0; r < 3; ++r) {
            memcpy(cbd.cast_mat[r], g_ls.frame_view + r * 4, 16);
        }

        memcpy(cbd.cast_view[0], g_ls.frame_view + 12, 16);
        cbd.cast_view[1][0] = g_mask.cast_fov[0];
        cbd.cast_view[1][1] = g_mask.cast_fov[1];
        cbd.cast_view[1][2] = g_mask.cast_dims[0];
        cbd.cast_view[1][3] = g_mask.cast_dims[1];
        const float* csun = g_sun_valid ? g_sun_dir_engine :
                            (g_sun_dir_derived_valid ? g_sun_dir_derived : g_sun_dir_engine);
        cbd.cast_view[2][0] = csun[0];
        cbd.cast_view[2][1] = csun[1];
        cbd.cast_view[2][2] = csun[2];
        cbd.cast_view[2][3] = g_shadow_map_strength;
        cbd.cast_meta[0] = 1.0f;   // screen path: camera-occlusion ON
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.composite_cb, 0);
        ctx->Draw(3, 0);
        g_mask.analytic_casts++;
    }

    g_mask.own_written = true;

    ctx->OMSetRenderTargets(4, old_rtvs, old_dsv);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    ctx->VSSetShader(old_vs, nullptr, 0);
    ctx->PSSetShader(old_ps, nullptr, 0);
    ctx->IASetInputLayout(old_il);
    ctx->IASetPrimitiveTopology(old_topo);
    ctx->VSSetConstantBuffers(0, 1, &old_cb);
    ctx->OMSetBlendState(old_blend, old_bf, old_bmask);
    ctx->OMSetDepthStencilState(old_dss, old_sref);
    ctx->PSSetShaderResources(0, 1, &old_t0);
    g_ro.in_injection = false;

    for (int r = 0; r < 4; ++r) KH_SAFE_RELEASE(old_rtvs[r]);

    KH_SAFE_RELEASE(old_dsv);
    KH_SAFE_RELEASE(old_vs);
    KH_SAFE_RELEASE(old_ps);
    KH_SAFE_RELEASE(old_il);
    KH_SAFE_RELEASE(old_cb);
    KH_SAFE_RELEASE(old_blend);
    KH_SAFE_RELEASE(old_dss);
    KH_SAFE_RELEASE(old_t0);
}

// Apply the shadow onto the LDR backbuffer the moment the engine binds
// it (signature: single-sample RGBA8/BGRA8, NO DSV - the inverse of the
// scene pass; see the UI-phase notes). Post-everything: no partition, no
// post-process, nothing can draw over the shadow afterwards except UI.
inline void mask_apply_at_backbuffer(ID3D11DeviceContext* ctx, UINT n,
                                     ID3D11RenderTargetView* const* rtvs,
                                     ID3D11DepthStencilView* dsv) {
    // A bound DSV no longer disqualifies: the engine may keep one
    // attached at the backbuffer bind; our depth-disabled draw ignores
    // it, and format+samplecount+size carry the discrimination.
    // Repurposed: SCENE-END apply. Fires when the scene RT (held since
    // the injection) is no longer among the incoming targets: the scene
    // is complete - decals, translucents, all of it - and the shadow
    // goes on last. rtvs/n here are the NEW targets being bound.
    if (g_mask.scene_apply_armed && g_mask.scene_rtv && g_mask.own_srv &&
        g_res.ps_maskapply && g_res.vs_fullscreen && g_mask.cast_blend &&
        g_mask.cast_dss && g_shadow_cast >= 0.5f && ctx) {
        bool still_bound = false;

        for (UINT i = 0; i < n && rtvs; ++i) {
            if (rtvs[i] == g_mask.scene_rtv) { still_bound = true; break; }
        }

        if (!still_bound) {
            g_mask.scene_apply_armed = false;
            g_ro.in_injection = true;
            ID3D11VertexShader* svs = nullptr;
            ID3D11PixelShader* sps = nullptr;
            ID3D11InputLayout* sil = nullptr;
            D3D11_PRIMITIVE_TOPOLOGY stp = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
            ID3D11Buffer* scb = nullptr;
            ID3D11BlendState* sbl = nullptr;
            FLOAT sbf[4] = {};
            UINT sbm = 0xFFFFFFFF;
            ID3D11DepthStencilState* sds = nullptr;
            UINT ssr = 0;
            UINT snv = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
            D3D11_VIEWPORT svp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
            ctx->RSGetViewports(&snv, svp);
            ctx->VSGetShader(&svs, nullptr, nullptr);
            ctx->PSGetShader(&sps, nullptr, nullptr);
            ctx->IAGetInputLayout(&sil);
            ctx->IAGetPrimitiveTopology(&stp);
            ctx->PSGetConstantBuffers(0, 1, &scb);
            ctx->OMGetBlendState(&sbl, sbf, &sbm);
            ctx->OMGetDepthStencilState(&sds, &ssr);
            ctx->OMSetRenderTargets(1, &g_mask.scene_rtv, nullptr);

            {
                D3D11_MAPPED_SUBRESOURCE mm = {};

                if (SUCCEEDED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mm))) {
                    ConstantData cbm = {};
                    const float sb2 = g_sun_brightness > 0.0f ? g_sun_brightness : 1.0f;
                    const bool amb_ok = g_amb_brightness_staged_flag;
                    const float ab2 = amb_ok && g_amb_brightness > 0.0f ? g_amb_brightness : sb2 * 0.8f;

                    for (int c = 0; c < 3; ++c) {
                        const float amb = g_amb_color[c] * ab2;
                        const float sun = g_sun_color[c] * sb2;
                        cbm.mask_meta[c] = amb / (amb + sun > 1e-4f ? amb + sun : 1e-4f);
                    }

                    memcpy(mm.pData, &cbm, sizeof(cbm));
                    ctx->Unmap(g_res.composite_cb, 0);
                }
            }

            D3D11_VIEWPORT vps2 = {};
            vps2.Width = g_mask.cast_dims[0] > 1.0f ? g_mask.cast_dims[0] : static_cast<FLOAT>(g_main_depth_w);
            vps2.Height = g_mask.cast_dims[1] > 1.0f ? g_mask.cast_dims[1] : static_cast<FLOAT>(g_main_depth_h);
            vps2.MaxDepth = 1.0f;
            ctx->RSSetViewports(1, &vps2);
            ctx->IASetInputLayout(nullptr);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
            ctx->PSSetShader(g_res.ps_maskapply, nullptr, 0);
            ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
            ctx->PSSetShaderResources(10, 1, &g_mask.own_srv);
            const FLOAT bfa2[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->OMSetBlendState(g_mask.cast_blend, bfa2, 0xFFFFFFFF);
            ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
            ctx->Draw(3, 0);
            g_mask.mask_applies++;
            ID3D11ShaderResourceView* nl = nullptr;
            ctx->PSSetShaderResources(10, 1, &nl);
            if (snv > 0) ctx->RSSetViewports(snv, svp);
            ctx->VSSetShader(svs, nullptr, 0);
            ctx->PSSetShader(sps, nullptr, 0);
            ctx->IASetInputLayout(sil);
            ctx->IASetPrimitiveTopology(stp);
            ctx->PSSetConstantBuffers(0, 1, &scb);
            ctx->OMSetBlendState(sbl, sbf, sbm);
            ctx->OMSetDepthStencilState(sds, ssr);
            // the caller's OMSet proceeds after us, binding the new targets
            g_ro.in_injection = false;
            KH_SAFE_RELEASE(svs);
            KH_SAFE_RELEASE(sps);
            KH_SAFE_RELEASE(sil);
            KH_SAFE_RELEASE(scb);
            KH_SAFE_RELEASE(sbl);
            KH_SAFE_RELEASE(sds);
        }
    }

}

inline bool mask_ensure_srv(ID3D11DeviceContext* ctx);   // defined below

// The hybrid: analytic ray-vs-box written into the ENGINE'S mask at the
// view-publish moment (after all resolves, before the mask is consumed),
// MIN-blended so it can only darken. No clear - the engine's content
// stays; we contribute our boxes' occlusion and the engine's lighting
// pass colors everything identically.
inline void mask_cast_engine(ID3D11DeviceContext* ctx) {
    // cast_misses = FIRST failed guard: 1 arm/fired never satisfied (set
    // once armed, cleared on entry), 2 mode, 3 resources, 41 depth,
    // 42 fov, 43 rtv-failed, 5 view, 61 ensure_srv, 62 tex; post-guard:
    // 7 states, 8 no boxes, 9 Map; 0 = full success.
    if (!ctx) return;
    if (g_mask_cast_arm && !g_mask_cast_fired) g_ls.cast_misses = 1;

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_cast < 0.0f && g_ls.cast_misses != 0) {
        g_mask.cold_cast_miss = g_ls.cast_misses;   // sticky while cold
    }
    // fired-once-per-frame relaxed to fired-once-per-RESOLVE-BATCH: on
    // partitioned frames each batch rewrites the mask, so each needs its
    // own re-shadowing write (idempotent under MIN).
    // (No phase gate here: the field-verified baseline fired at the first
    // armed pre-draw regardless of phase, and a later gate - added on a
    // partition-rewrite theory the maskRtvSwaps=0 measurement refuted -
    // starved the fire on look-down frames. Baseline semantics restored.)
    if (g_mask_cast_fired && g_ls.resolve_seen_since_cast && g_ls.frame_view_valid) {
        g_mask_cast_arm = true;
        g_mask_cast_fired = false;
        g_mask.cast_batches++;
    }

    if (!g_mask_cast_arm || g_mask_cast_fired) return;
    g_ls.resolve_seen_since_cast = false;
    if (g_shadow_cast < 0.5f || g_shadow_cast_screen >= 0.5f) { g_ls.cast_misses = 2; return; }
    if (!g_res.initialized || !g_res.ps_maskcast || !g_res.vs_fullscreen || !g_res.composite_cb) { g_ls.cast_misses = 3; return; }
    if (!g_mask.cast_depth) { g_ls.cast_misses = 41; return; }
    if (!g_mask.cast_fov_valid) { g_ls.cast_misses = 42; return; }
    if (g_mask.engine_mask_failed) { g_ls.cast_misses = 43; return; }
    if (!g_ls.frame_view_valid) { g_ls.cast_misses = 5; return; }

    // cold view-quality gate: within 2 s of the first box, require TIGHT
    // rot agreement (0.05) before firing - the 0.35 family filter can
    // pass a still-settling view at spawn, and a ~20-degree-off view
    // reconstructs badly enough to MIN-darken the screen briefly (the
    // short cold overcast). Steady state is unaffected.
    if (g_mask.cold_t0 >= 0.0 &&
        static_cast<double>(effect_time_seconds()) - g_mask.cold_t0 < 2.0 &&
        g_ls.last_publish_rot_err > 0.05f) { g_ls.cast_misses = 51; return; }
    if (!g_mask.engine_mask_rtv) { g_ls.cast_misses = 61; return; }   // captured at the gated resolve sweep
    g_ls.cast_misses = 0;
    g_mask_cast_arm = false;
    g_mask_cast_fired = true;

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_cast < 0.0f) {
        g_mask.cold_first_cast = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }

    if (!g_mask.cast_dss && !g_mask.cast_states_failed) {
        ID3D11Device* dv2 = nullptr;
        ctx->GetDevice(&dv2);

        if (dv2) {
            D3D11_DEPTH_STENCIL_DESC dd2 = {};
            dd2.DepthEnable = FALSE;
            if (FAILED(dv2->CreateDepthStencilState(&dd2, &g_mask.cast_dss))) g_mask.cast_states_failed = true;
            dv2->Release();
        }
    }

    if (!g_mask.min_blend) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) { g_mask.engine_mask_failed = true; g_ls.cast_misses = 7; return; }

        if (!g_mask.min_blend) {
            D3D11_BLEND_DESC bd = {};
            bd.RenderTarget[0].BlendEnable = TRUE;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;   // darken-only
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
            bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            if (FAILED(dev->CreateBlendState(&bd, &g_mask.min_blend))) {
                g_mask.engine_mask_failed = true;
                dev->Release();
                return;
            }
        }

        dev->Release();
    }

    struct CastBox { float pos[3]; float size[3]; };
    CastBox cboxes[64];   // 16 starved the stress test's 31st box
    uint32_t ncb = 0;

    {
        std::lock_guard<std::mutex> lk(g_draw_list_mutex);

        for (const auto& kv2 : g_draw_list) {
            if (ncb >= 64) break;
            const auto& o = kv2.second;
            memcpy(cboxes[ncb].pos, o.pos, 12);
            memcpy(cboxes[ncb].size, o.size, 12);
            ncb++;
        }
    }

    if (ncb == 0) { g_ls.cast_misses = 8; return; }
    g_ro.in_injection = true;
    ID3D11RenderTargetView* old_rtvs[4] = {};
    ID3D11DepthStencilView* old_dsv = nullptr;
    ctx->OMGetRenderTargets(4, old_rtvs, &old_dsv);
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);
    ID3D11VertexShader* old_vs = nullptr;
    ID3D11PixelShader* old_ps = nullptr;
    ID3D11InputLayout* old_il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY old_topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* old_cb = nullptr;
    ID3D11BlendState* old_blend = nullptr;
    FLOAT old_bf[4] = {};
    UINT old_bmask = 0xFFFFFFFF;
    ID3D11DepthStencilState* old_dss = nullptr;
    UINT old_sref = 0;
    ID3D11ShaderResourceView* old_t0 = nullptr;
    ctx->VSGetShader(&old_vs, nullptr, nullptr);
    ctx->PSGetShader(&old_ps, nullptr, nullptr);
    ctx->IAGetInputLayout(&old_il);
    ctx->IAGetPrimitiveTopology(&old_topo);
    ctx->PSGetConstantBuffers(0, 1, &old_cb);
    ID3D11Buffer* old_vs_cb = nullptr;
    ctx->VSGetConstantBuffers(0, 1, &old_vs_cb);   // the fire sets VS b0
    // and never restored it: every engine VERTEX shader after the fire
    // read our box cbd as its constants - the last-fired box's COLOR
    // tinted all particles (blue, from the fog array). The dual-gated
    // arm exposed it by moving the fire later in the frame, past the
    // engine's last natural VS-b0 rebind before the particle pass.
    ctx->OMGetBlendState(&old_blend, old_bf, &old_bmask);
    ctx->OMGetDepthStencilState(&old_dss, &old_sref);
    ctx->PSGetShaderResources(0, 1, &old_t0);
    ctx->OMSetRenderTargets(1, &g_mask.engine_mask_rtv, nullptr);
    D3D11_VIEWPORT vpn = {};
    vpn.Width = g_mask.cast_dims[0];
    vpn.Height = g_mask.cast_dims[1];
    vpn.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpn);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskcast, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
    const FLOAT bfm[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.min_blend, bfm, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
    ctx->PSSetShaderResources(0, 1, &g_mask.cast_depth);

    for (uint32_t bi = 0; bi < ncb; ++bi) {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) { g_ls.cast_misses = 9; break; }
        ConstantData cbd = {};
        cbd.center_size[0] = cboxes[bi].pos[0];
        cbd.center_size[1] = cboxes[bi].pos[2];
        cbd.center_size[2] = cboxes[bi].pos[1];
        cbd.size_axes[0] = cboxes[bi].size[0];
        cbd.size_axes[1] = cboxes[bi].size[2];
        cbd.size_axes[2] = cboxes[bi].size[1];

        for (int r = 0; r < 3; ++r) {
            memcpy(cbd.cast_mat[r], g_ls.frame_view + r * 4, 16);
        }

        memcpy(cbd.cast_view[0], g_ls.frame_view + 12, 16);
        cbd.cast_view[1][0] = g_mask.cast_fov[0];
        cbd.cast_view[1][1] = g_mask.cast_fov[1];
        cbd.cast_view[1][2] = g_mask.cast_dims[0];
        cbd.cast_view[1][3] = g_mask.cast_dims[1];
        const float* csun = g_sun_valid ? g_sun_dir_engine :
                            (g_sun_dir_derived_valid ? g_sun_dir_derived : g_sun_dir_engine);
        cbd.cast_view[2][0] = csun[0];
        cbd.cast_view[2][1] = csun[1];
        cbd.cast_view[2][2] = csun[2];
        cbd.cast_view[2][3] = g_shadow_map_strength;   // 1 = full: engine handles ambient
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.composite_cb, 0);
        ctx->Draw(3, 0);
        g_mask.analytic_casts++;
    }

    ctx->OMSetRenderTargets(4, old_rtvs, old_dsv);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    ctx->VSSetShader(old_vs, nullptr, 0);
    ctx->PSSetShader(old_ps, nullptr, 0);
    ctx->IASetInputLayout(old_il);
    ctx->IASetPrimitiveTopology(old_topo);
    ctx->PSSetConstantBuffers(0, 1, &old_cb);
    ctx->VSSetConstantBuffers(0, 1, &old_vs_cb);
    ctx->OMSetBlendState(old_blend, old_bf, old_bmask);
    ctx->OMSetDepthStencilState(old_dss, old_sref);
    ctx->PSSetShaderResources(0, 1, &old_t0);
    g_ro.in_injection = false;
    KH_SAFE_RELEASE(old_vs_cb);

    for (int r = 0; r < 4; ++r) KH_SAFE_RELEASE(old_rtvs[r]);

    KH_SAFE_RELEASE(old_dsv);
    KH_SAFE_RELEASE(old_vs);
    KH_SAFE_RELEASE(old_ps);
    KH_SAFE_RELEASE(old_il);
    KH_SAFE_RELEASE(old_cb);
    KH_SAFE_RELEASE(old_blend);
    KH_SAFE_RELEASE(old_dss);
    KH_SAFE_RELEASE(old_t0);
}

inline void mask_note_draw(ID3D11DeviceContext* ctx) {
    if (!g_mask.atlas_bound || !ctx) return;
    g_mask.atlas_bound = false;   // one record per pass segment is plenty

    if (g_mask.rt_key) {
        for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
            if (g_mask.cands[i].key == g_mask.rt_key) { g_mask.cands[i].draws++; break; }
        }
    }

    if (g_mask.rt_is_resolve) {
        resolve_pair_capture(ctx);
    }
}

// Choose + hold the mask texture and its SRV. Render thread (injection).
inline bool mask_ensure_srv(ID3D11DeviceContext* ctx) {
    if (g_mask.enable < 0.5f || g_mask.cand_count == 0 || !ctx) return false;
    int pick = -1;

    if (g_mask.index >= 0.0f && static_cast<uint32_t>(g_mask.index) < g_mask.cand_count) {
        pick = static_cast<int>(g_mask.index);
    } else {
        // Auto-pick: the shadow resolve is a SCREEN-SIZED SINGLE-CHANNEL
        // target (field-confirmed: R8_UNORM). Most-draws alone grabbed the
        // HDR scene color, which also samples the atlas.
        uint64_t best = 0;

        for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
            const MaskCandidate& c = g_mask.cands[i];
            if (c.w < 1280) continue;
            const bool single =
                c.fmt == DXGI_FORMAT_R8_UNORM || c.fmt == DXGI_FORMAT_R8_TYPELESS ||
                c.fmt == DXGI_FORMAT_R16_UNORM || c.fmt == DXGI_FORMAT_R16_TYPELESS ||
                c.fmt == DXGI_FORMAT_R16_FLOAT;
            if (!single) continue;

            if (c.draws > best) {
                best = c.draws;
                pick = static_cast<int>(i);
            }
        }

        if (pick < 0) {   // no single-channel candidate yet: most draws
            for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
                if (g_mask.cands[i].w >= 640 && g_mask.cands[i].draws > best) {
                    best = g_mask.cands[i].draws;
                    pick = static_cast<int>(i);
                }
            }
        }
    }

    if (pick < 0) return false;

    if (g_mask.chosen_key != g_mask.cands[pick].key) {
        if (g_mask.srv) { g_mask.srv->Release(); g_mask.srv = nullptr; }
        if (g_mask.tex) { g_mask.tex->Release(); g_mask.tex = nullptr; }
        g_mask.chosen_key = nullptr;
    }

    if (g_mask.srv) return true;
    // Re-acquire the texture pointer via the stored key: the key IS the
    // texture pointer recorded while alive; the engine keeps these
    // persistent, but guard creation failures anyway.
    ID3D11Texture2D* tex = static_cast<ID3D11Texture2D*>(g_mask.cands[pick].key);
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    DXGI_FORMAT fmt = td.Format;

    switch (td.Format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     fmt = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     fmt = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: fmt = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        case DXGI_FORMAT_R8_TYPELESS:           fmt = DXGI_FORMAT_R8_UNORM; break;
        case DXGI_FORMAT_R16_TYPELESS:          fmt = DXGI_FORMAT_R16_UNORM; break;
        default: break;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    ID3D11Device* dev = nullptr;
    tex->GetDevice(&dev);
    if (!dev) return false;
    HRESULT hr = dev->CreateShaderResourceView(tex, &sd, &g_mask.srv);
    dev->Release();
    if (FAILED(hr) || !g_mask.srv) {
        g_mask.srv = nullptr;
        g_mask.enable = 0.0f;   // cache the failure: stop per-frame retries
        g_stats.shadow_srv_failed++;
        return false;
    }
    tex->AddRef();
    g_mask.tex = tex;
    g_mask.chosen_key = g_mask.cands[pick].key;
    g_mask.dims[0] = static_cast<float>(g_mask.cands[pick].w);
    g_mask.dims[1] = static_cast<float>(g_mask.cands[pick].h);
    return true;
}

// Lazy SRV over the atlas; typeless depth formats map to their color-read
// equivalents. Render thread (injection) only.
inline bool shadow_live_ensure_srv() {
    if (g_ls.atlas_srv) return true;
    if (!g_ls.atlas_tex || g_ls.srv_failed) return false;

    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

    switch (static_cast<DXGI_FORMAT>(g_ls.atlas_fmt)) {
        case DXGI_FORMAT_R32_TYPELESS:    fmt = DXGI_FORMAT_R32_FLOAT; break;
        case DXGI_FORMAT_R24G8_TYPELESS:  fmt = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; break;
        case DXGI_FORMAT_R16_TYPELESS:    fmt = DXGI_FORMAT_R16_UNORM; break;
        case DXGI_FORMAT_R32G8X24_TYPELESS: fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS; break;
        default: break;
    }

    if (fmt == DXGI_FORMAT_UNKNOWN) { g_ls.srv_failed = true; g_stats.shadow_srv_failed++; return false; }

    ID3D11Device* dev = nullptr;
    g_ls.atlas_tex->GetDevice(&dev);
    if (!dev) { g_ls.srv_failed = true; g_stats.shadow_srv_failed++; return false; }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    const HRESULT hr = dev->CreateShaderResourceView(g_ls.atlas_tex, &sd, &g_ls.atlas_srv);
    dev->Release();

    if (FAILED(hr) || !g_ls.atlas_srv) {
        g_ls.atlas_srv = nullptr;
        g_ls.srv_failed = true;
        g_stats.shadow_srv_failed++;
        return false;
    }

    return true;
}

// OM-binding tracking for the recon: keeps phase_active / target_identity
// current. Called from the OMSet hooks on the render thread, injection
// excluded, with the freshly computed main-DSV verdict.
inline void shadow_track_targets(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, bool dsv_is_main,
                                 UINT n, ID3D11RenderTargetView* const* rtvs) {
    if (!shadow_live_wanted()) {
        if (g_sr.phase_active) {
            shadow_close_cycle();
            g_sr.phase_active = false;
            g_sr.target_identity = nullptr;
        }

        g_ls.phase_on_atlas = false;
        g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
        return;
    }

    const bool depth_only = dsv && !dsv_is_main && (n == 0 || !rtvs || !rtvs[0]);
    void* id = depth_only ? reorder_dsv_identity(dsv) : nullptr;
    bool is_atlas = false;

    if (depth_only && (!g_sr.phase_active || g_sr.target_identity != id)) {
        is_atlas = shadow_probe_target(dsv);
    } else if (depth_only) {
        is_atlas = g_ls.phase_on_atlas;   // unchanged target within the phase
    }

    if (g_sr.phase_active && (!depth_only || id != g_sr.target_identity)) {
        shadow_close_cycle();
    }

    const bool atlas_cycle_start =
        is_atlas && (!g_ls.phase_on_atlas || id != g_sr.target_identity);

    // Cycle boundary on the atlas: commit the cycle's fully-upgraded latch
    // then grab per-cycle state; the cascade's draws are complete here.
    if (g_ls.phase_on_atlas && (!is_atlas || atlas_cycle_start)) {
        shadow_live_finalize_cycle();

    }

    g_sr.phase_active = depth_only;
    g_sr.target_identity = id;
    g_ls.phase_on_atlas = is_atlas;

    if (atlas_cycle_start) g_ls.cycle_latched = false;   // new cascade cycle, new latch
    if (!is_atlas) { g_ls.pending_valid = false; g_ls.pending_bytes = 0; g_ls.pending_vp_valid = false; }
}

// Per-draw recon accounting; a depth-only cycle counts once it clears the
// draw floor (rejects tiny utility clears/copies masquerading as passes).
inline void shadow_note_draw(ID3D11DeviceContext* ctx) {
    if (g_ls.pending_valid) shadow_live_on_draw(ctx);
    return;   // (recon-era per-draw census removed; live latching runs above)
}


static HRESULT STDMETHODCALLTYPE hooked_map(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped) {
    const HRESULT hr = g_orig_map(self, res, sub, type, flags, mapped);

    if (SUCCEEDED(hr) && mapped && mapped->pData && sub == 0 &&
        (type == D3D11_MAP_WRITE_DISCARD || type == D3D11_MAP_WRITE_NO_OVERWRITE) &&
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection &&
        ((!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted())) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            for (auto& p : g_proj_pending) {
                if (!p.res) {
                    p.res = res;
                    p.data = mapped->pData;
                    p.bytes = bytes;
                    break;
                }
            }
        }
    }

    return hr;
}

static void STDMETHODCALLTYPE hooked_unmap(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub) {
    if (sub == 0 && self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection) {
        for (auto& p : g_proj_pending) {
            if (p.res == res) {
                // The engine has finished writing; the pointer is valid
                // until the original Unmap below runs.
                proj_scan_upload(res, p.data, p.bytes);
                shadow_live_upload(p.data, p.bytes);
                if (!g_ro.in_injection) {   // our own CBs carry view columns
                    shadow_register_upload(res, p.data, p.bytes);   // and window rows:
                    shadow_view_scan(res, p.data, p.bytes);         // never self-learn
                }
                p = ProjPendingMap{};
                break;
            }
        }
    }

    g_orig_unmap(self, res, sub);
}

static void STDMETHODCALLTYPE hooked_updatesubresource(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, const D3D11_BOX* box, const void* data, UINT row_pitch, UINT depth_pitch) {
    if (sub == 0 && !box && data &&
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection &&
        ((!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted())) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            proj_scan_upload(res, data, bytes);
            shadow_live_upload(data, bytes);
            if (!g_ro.in_injection) {
                shadow_register_upload(res, data, bytes);
                shadow_view_scan(res, data, bytes);
            }
        }
    }

    g_orig_updatesubresource(self, res, sub, box, data, row_pitch, depth_pitch);
}

// Draws every eligible box into the CURRENT scene targets with depth
// writes, back-to-front, so the engine's subsequent translucent passes
// composite against them. Runs on the render thread inside a hooked draw;
// full pipeline state is snapshotted and restored around it.
//
// CONCURRENCY: no lock, deliberately. The bridge's graphics lock parks the
// render thread while the game thread uses the context, so game-thread
// context use can never overlap this function's execution; acquiring that
// lock FROM the render thread deadlocks the game (the holder is waiting
// for this very thread to park). The injection writes only its own
// dedicated constant buffer, so nothing is shared with the flush anyway.
// Cold-start pre-warm: the view lock's argmin only ran at the injection,
// which needs boxes - so a freshly spawned box's first frames ran
// lockless (frame_view invalid, the hybrid's small spawn delay). The
// same attempt, run at the frame boundary against the bridge cycle PV,
// acquires the lock BEFORE any box exists. Does not clear the ring and
// never runs once locked; the injection block stays authoritative.
inline void shadow_view_prewarm() {
    if (g_ls.view_src_valid || !g_ro.cycle_pv_valid) return;
    const uint32_t vcn = g_ls.vc_n < 16 ? g_ls.vc_n : 16;
    if (vcn == 0) return;
    const float* truth = &g_ro.cycle_pv.view[0][0];
    int best_i = -1, best_k = 0;
    float best_rot = 1e9f;

    for (uint32_t i = 0; i < vcn; ++i) {
        const float* m = g_ls.vc_ring[i].m;

        for (int orient = 1; orient <= 2; ++orient) {
            float e_rot = 0.0f;

            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    const float mv = orient == 1 ? m[r * 4 + c] : m[c * 4 + r];
                    e_rot += fabsf(mv - truth[r * 4 + c]);
                }
            }

            if (e_rot < best_rot) {
                best_rot = e_rot;
                best_i = static_cast<int>(i);
                best_k = orient;
            }
        }
    }

    // 0.15, not 0.05: the truth here is the BRIDGE view, one frame stale,
    // so camera motion held the 0.05 match just over the line for seconds
    // (coldLock=8 caught it). Family separation is >1; the relative-
    // fingerprint commit rule and the health loop carry the safety.
    // 0.06: true locks score ~0.00; a STABLE impostor family at ~0.13
    // beat the double-confirmation twice running (viewLocks=2 with
    // viewBestRot=0.132 - the recurring cold drift). The injection-time
    // argmin remains the backstop if truth scores poorly at first.
    if (best_i < 0 || best_rot >= 0.03f) return;   // impostors measured at 0.048 and 0.132; truth scores ~0.00
    const float* m = g_ls.vc_ring[best_i].m;
    float tmag = 0.0f;

    for (int c = 0; c < 3; ++c) {
        const float tv = best_k == 1 ? m[12 + c] : m[c * 4 + 3];
        tmag += fabsf(tv);
    }

    if (tmag >= 10.0f) return;   // relative only (same rule as the injection)

    // CONFIRM before committing: the motion-relaxed 0.15 threshold can
    // crown a one-frame fluke, and a wrong cold lock made the cast
    // shadow lag camera rotation for ~1 s until the health monitor's
    // 30-miss window kicked it (viewLocks = 2 was the tell). The same
    // candidate must win two consecutive boundary attempts.
    static ID3D11Resource* s_prev_cand_res = nullptr;
    static uint32_t s_prev_cand_off = 0;

    if (g_ls.vc_ring[best_i].res != s_prev_cand_res ||
        g_ls.vc_ring[best_i].off != s_prev_cand_off) {
        s_prev_cand_res = g_ls.vc_ring[best_i].res;
        s_prev_cand_off = g_ls.vc_ring[best_i].off;
        return;   // first sighting: confirm next boundary
    }

    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
    g_ls.view_src_orient = best_k;
    g_ls.view_src_relative = true;
    g_ls.view_src_valid = true;
    g_ls.view_src_miss = 0;
    g_ls.view_locks++;
}

inline void inject_composited_boxes(ID3D11DeviceContext* ctx) {
    const float snapshot_now = effect_time_seconds();

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) {
        g_mask.cold_first_trigger = static_cast<float>(snapshot_now - g_mask.cold_t0);
    }
    static std::vector<RenderObject> boxes;   // render-thread scratch
    boxes.clear();

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;
            if (!o.visible || !is_composite_eligible(o)) continue;
            bool expired = false;
            const float env = lifetime_envelope(o, snapshot_now, expired);
            if (expired) continue;   // the Draw3D flush owns the erasure
            boxes.push_back(o);
            boxes.back().color[3] *= env;
        }

        if (!boxes.empty() && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_stage < 0.0f) {
            g_mask.cold_first_stage = static_cast<float>(snapshot_now - g_mask.cold_t0);
        }
    }

    // The path is alive even when the eligible set is momentarily empty -
    // stamping here keeps the flush from flapping over.
    g_composite_last_inject_ms.store(steady_now_ms(), std::memory_order_relaxed);
    if (boxes.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) { g_stats.composite_skips++; return; }
    if (!ensure_resources(dev).empty()) { g_stats.composite_skips++; return; }

    // Matrices: the CYCLE'S latched PV (captured at the main depth clear,
    // synchronous with the engine's own frame-setup snapshot - see the
    // cycle_pv note in ReorderState). A mid-frame bridge fetch is the LAST
    // resort only: by the translucent phase the sim thread has often
    // published the NEXT frame's projection, whose dynamic near plane can
    // differ - skewing the boxes' stored depth worst at close range.
    static RVExtBridge::ProjectionViewTransform s_last_pv = {};
    static uint64_t s_last_pv_ms = 0;
    RVExtBridge::ProjectionViewTransform pv = {};

    if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;

        // TRUST THE LATCH. A live re-fetch differing from the latch means
        // the sim republished MID-CYCLE, i.e., AFTER the clear - in which
        // case the latch captured the pre-publication value, which is what
        // the engine's frame data was fixed to at handover: the latch is
        // CORRECT by construction in exactly the case this detector
        // detects. (An earlier revision "conservatively" overrode the
        // latch here and thereby manufactured a visible one-frame skew on
        // ambiguous frames - the flickers correlated 1:1 with this
        // counter. Diagnosis, not intervention.) The genuinely losing race
        // - publication BEFORE the clear - makes latch == live and is
        // undetectable from here; it is at most one frame per near-plane
        // change event.
        RVExtBridge::ProjectionViewTransform live = {};


        if (RVExtBridge::get_projection_view_transform(live)) {
            const float m22_latch = pv.projection[2][2];
            const float m32_latch = pv.projection[3][2];
            const float m22_live = live.projection[2][2];
            const float m32_live = live.projection[3][2];
            const float n_latch = (fabsf(m22_latch) > 1e-9f) ? (-m32_latch / m22_latch) : 0.0f;
            const float n_live  = (fabsf(m22_live)  > 1e-9f) ? (-m32_live  / m22_live)  : 0.0f;

            // On disagreement the LATCH is the suspect: on sky-heavy
            // frames it belongs to a partition/aux pass whose huge near
            // plane SLICED the injected box (the look-up clip). Keep the
            // latch when it agrees (frame-exact); use the live bridge
            // values when it does not (correct near, one frame soft).
            // Count-only otherwise: ambiguity is chronic (56% of frames
            // at noon) and must never reject or switch AUTHORS - the
            // flicker lesson. This switches only two projection scalars.
            if (fabsf(n_latch - n_live) > 1e-4f) {
                g_stats.composite_ambiguous++;
                pv.projection[2][2] = m22_live;
                pv.projection[3][2] = m32_live;
            }
        }
    } else if (RVExtBridge::get_projection_view_transform(pv)) {
        s_last_pv = pv;
        s_last_pv_ms = steady_now_ms();
    } else if (s_last_pv_ms != 0 && steady_now_ms() - s_last_pv_ms < 250) {
        pv = s_last_pv;
    } else {
        g_stats.composite_skips++;
        return;   // no usable matrices anywhere this frame
    }

    // MEASURED depth coefficients: when the projection sniffer captured the
    // engine's own m22/m32 this cycle, overwrite the bridge values. The
    // boxes then rasterize with the TRUE dynamic near plane of the frame
    // being rendered - the one scalar the bridge can never pin down - and
    // the guard reconstructs scene distance with the same truth, so its
    // margins tighten from race-absorbing to precision-absorbing.
    const bool measured = g_ro.engine_proj_valid;

    if (measured) {
        pv.projection[2][2] = g_ro.engine_m22;
        pv.projection[3][2] = g_ro.engine_m32;
    }

    // near = 10 is this scene's CHRONIC normal (partition-far latch);
    // it only bites when box fragments come within ~near of the camera
    // (the look-up slice - open design item, see CONTINUATION.md).
    // Forensics only; the near-draw attempt was reverted after causing
    // see-through on ordinary-near frames.
    const float inject_near = fabsf(pv.projection[2][2]) > 1e-9f
                            ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    g_mask.last_inject_near = inject_near;

    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    // supervise the view source: lock the upload location whose contents
    // equal the PROVEN view used for the boxes this frame
    {
        const float* truth = &pv.view[0][0];
        const uint32_t vcn = g_ls.vc_n < 16 ? g_ls.vc_n : 16;

        if (!g_ls.view_src_valid && vcn > 0) {
            // ARGMIN, not first-passing: pick the closest candidate and
            // report its error - zero-lock runs then tell us the true
            // convention gap instead of guessing tolerances.
            int best_i = -1, best_k = 0;
            float best_rot = 1e9f, best_tr = 1e9f;

            for (uint32_t i = 0; i < vcn; ++i) {
                const float* m = g_ls.vc_ring[i].m;

                for (int orient = 1; orient <= 2; ++orient) {
                    float e_rot = 0.0f, e_tr = 0.0f;

                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            const float mv = orient == 1 ? m[r * 4 + c] : m[c * 4 + r];
                            e_rot += fabsf(mv - truth[r * 4 + c]);
                        }
                    }

                    for (int c = 0; c < 3; ++c) {
                        const float mv = orient == 1 ? m[12 + c] : m[c * 4 + 3];
                        e_tr += fabsf(mv - truth[12 + c]);
                    }

                    if (e_rot < best_rot) {
                        best_rot = e_rot;
                        best_tr = e_tr;
                        best_i = static_cast<int>(i);
                        best_k = orient;
                    }
                }
            }

            g_ls.view_best_rot = best_rot;
            g_ls.view_best_trans = best_tr;

            if (best_i >= 0 && best_rot < 0.05f) {
                // Convention classification (measured: engine uploads the
                // view CAMERA-RELATIVE - rotation identical to the bridge,
                // translation ~0 vs the bridge's world-absolute -cam*R).
                const float* m = g_ls.vc_ring[best_i].m;
                float tmag = 0.0f;

                for (int c = 0; c < 3; ++c) {
                    const float tv = best_k == 1 ? m[12 + c] : m[c * 4 + 3];
                    tmag += fabsf(tv);
                }

                const bool relative = tmag < 10.0f;

                // RELATIVE ONLY: the engine's view is camera-relative
                // (measured); an absolute-exact candidate is an echo of
                // our own uploads (bandView columns) - locking onto it
                // made frame_view read back our own half-filled CB
                // (garbage seals: both shadow paths vanished).
                if (relative) {
                    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
                    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
                    g_ls.view_src_orient = best_k;
                    g_ls.view_src_relative = true;
                    g_ls.view_src_valid = true;

                    g_ls.view_src_miss = 0;
                    g_ls.view_locks++;
                }
            }
        }

        g_ls.vc_n = 0;   // fresh ring per injection: stale entries can't win

        if (g_ls.view_src_valid) {
            // health: rotation agreement is the signal (the hybrid's
            // translation is bridge-derived by construction)
            bool healthy = false;

            if (g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f) {
                float e_rot = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        e_rot += fabsf(g_ls.frame_view[r * 4 + c] - truth[r * 4 + c]);
                    }
                }

                healthy = e_rot < 0.10f;
            }

            if (healthy) {
                g_ls.view_src_miss = 0;
            } else if (++g_ls.view_src_miss > 30) {
                g_ls.view_src_valid = false;
                g_ls.vc_n = 0;
            }
        }
    }
    float cam[3];
    extract_camera_pos(pv.view, cam);
    g_ls.cam[0] = cam[0];   // shadow-space origin for the CPU-side diagnostic
    g_ls.cam[1] = cam[1];
    g_ls.cam[2] = cam[2];

    // Translucent-correct ordering among our own boxes: back to front.
    std::sort(boxes.begin(), boxes.end(), [&cam](const RenderObject& a, const RenderObject& b) {
        auto d2 = [&cam](const RenderObject& o) {
            const float ex = o.pos[0] - cam[0];
            const float ey = o.pos[2] - cam[1];
            const float ez = o.pos[1] - cam[2];
            return ex * ex + ey * ey + ez * ez;
        };
        return d2(a) > d2(b);
    });

    StateBackup backup;
    backup.capture(ctx);

    // Render the analytic shadow factor NOW (view is frame-fresh; the
    // injection runs every frame - no mid-pipeline interruption, no
    // missed frames), then composite it onto the scene BEFORE the boxes
    // draw: world darkens under the shadow, boxes stay unaffected.
    // Screen-multiply path only when elem 18 = 1; the default is the
    // ENGINE-MASK hybrid (fired at the pre-draw), whose color and alpha
    // are the engine's own - the baseline quality, by the same pipeline.
    if (g_shadow_cast_screen >= 0.5f) mask_cast_fire(ctx);

    // Apply AT THE INJECTION - the configuration that was SEEN working
    // ("both shadows present", stable). The backbuffer route drew every
    // frame (maskApplies == flushes) yet nothing reached the screen: the
    // signature-matched target is an intermediate LDR buffer, discarded
    // before present. The injection's RT is proven visible - we draw
    // boxes into it.
    if (g_shadow_cast_screen >= 0.5f &&
        g_mask.own_srv && g_res.ps_maskapply && g_res.vs_fullscreen &&
        g_mask.cast_blend && g_mask.cast_dss && g_shadow_cast >= 0.5f) {
        g_mask.own_written = false;
        // hold the scene RT identity; the apply happens at its UNBIND
        ID3D11RenderTargetView* rtv_now[1] = {};
        ctx->OMGetRenderTargets(1, rtv_now, nullptr);

        if (rtv_now[0]) {
            if (g_mask.scene_rtv != rtv_now[0]) {
                if (g_mask.scene_rtv) g_mask.scene_rtv->Release();
                g_mask.scene_rtv = rtv_now[0];   // keep the ref
                ID3D11Resource* sr = nullptr;
                rtv_now[0]->GetResource(&sr);
                g_mask.scene_key = sr;
                if (sr) sr->Release();
            } else {
                rtv_now[0]->Release();
            }

            g_mask.scene_apply_armed = true;
        }
    }

    if (false) {
        g_mask.own_written = false;
        // BIND RT0 ONLY: with independent blend off, D3D applies our
        // multiply to EVERY bound target - the engine binds auxiliary
        // RTs alongside the scene here, and stomping them was the fog/
        // sky/lighting corruption. Backup restores the full set after.
        ID3D11RenderTargetView* cur_rtvs[4] = {};
        ID3D11DepthStencilView* cur_dsv = nullptr;
        ctx->OMGetRenderTargets(4, cur_rtvs, &cur_dsv);

        if (cur_rtvs[0]) {
            ctx->OMSetRenderTargets(1, &cur_rtvs[0], cur_dsv);
            ctx->IASetInputLayout(nullptr);
            ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            // amb/(amb+sun) per channel: the physically-derived shadow
            // floor. Brightness scalars weight the normalized colors.
            {
                D3D11_MAPPED_SUBRESOURCE mm = {};

                if (SUCCEEDED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mm))) {
                    ConstantData cbm = {};
                    const float sb = g_sun_brightness > 0.0f ? g_sun_brightness : 1.0f;
                    // Ambient default is RELATIVE to the sun (35%): the
                    // raw sun brightness is exposure-coupled (~10), and
                    // an absolute ambient default divided against it
                    // collapsed the floor to ~0.02 - the pitch black.
                    // SQF-provided ambient (getLighting units, matching
                    // the sun's) is used as-is.
                    const bool amb_staged = g_amb_brightness_staged_flag;
                    const float ab = amb_staged && g_amb_brightness > 0.0f
                                   ? g_amb_brightness
                                   : sb * 0.8f;   // RV daytime ambient is strong

                    for (int c = 0; c < 3; ++c) {
                        const float amb = g_amb_color[c] * ab;
                        const float sun = g_sun_color[c] * sb;
                        cbm.mask_meta[c] = amb / (amb + sun > 1e-4f ? amb + sun : 1e-4f);
                    }

                    cbm.mask_meta[3] = 0.0f;
                    memcpy(mm.pData, &cbm, sizeof(cbm));
                    ctx->Unmap(g_res.composite_cb, 0);
                }
            }

            ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
            ctx->PSSetShader(g_res.ps_maskapply, nullptr, 0);
            ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
            ctx->PSSetShaderResources(10, 1, &g_mask.own_srv);
            const FLOAT bfa[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
            ctx->OMSetBlendState(g_mask.cast_blend, bfa, 0xFFFFFFFF);
            ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
            ctx->Draw(3, 0);
            ID3D11ShaderResourceView* nul10 = nullptr;
            ctx->PSSetShaderResources(10, 1, &nul10);
            ctx->OMSetRenderTargets(4, cur_rtvs, cur_dsv);
        }

        for (int r4 = 0; r4 < 4; ++r4) KH_SAFE_RELEASE(cur_rtvs[r4]);

        KH_SAFE_RELEASE(cur_dsv);
    }

    // CRITICAL - viewport depth range: the engine renders in depth
    // PARTITIONS, remapping each pass's NDC z through the viewport
    // MinDepth/MaxDepth of its slice. The boxes' stored depth must be
    // encoded through the SAME range the surrounding scene geometry used,
    // or every comparison skews - the box beats opaque walls it sits
    // behind, or loses to the skydome it sits in front of, worst for NEAR
    // fragments where a min-depth offset is a huge world-space error
    // (hence flicker worsening close to the box). The authoritative range
    // is the CURRENT CYCLE'S, captured from its own opaque phase on this
    // very thread (g_ro.cycle_vp_*) - deliberately not the flush-probed
    // globals, which the flush can poison by parking the render thread
    // inside a secondary pass. Keep the engine's viewport rectangle, force
    // the range, restore afterwards - StateBackup does not cover viewports.
    UINT n_saved_vp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&n_saved_vp, saved_vp);

    {
        D3D11_VIEWPORT vp = {};

        if (n_saved_vp >= 1) {
            vp = saved_vp[0];
        } else {
            vp.Width = static_cast<FLOAT>(g_main_depth_w);
            vp.Height = static_cast<FLOAT>(g_main_depth_h);
        }

        vp.MinDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
        vp.MaxDepth = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
        ctx->RSSetViewports(1, &vp);
    }

    // The HYBRID: hardware depth (with the shimmer bias) resolves marginal
    // ties exactly as the months-stable flush path always did, and the
    // pixel-shader guard vetoes fragments clearly behind opaque geometry
    // using a mid-frame depth copy - matrix races can then never punch the
    // box through a wall. When the copy or shader is unavailable the plain
    // pipeline runs, which is simply the previous behavior.
    const bool guard = ensure_composite_depth(dev, ctx) &&
                       ensure_composite_shader(dev).empty();

    UINT stride = sizeof(float) * 3, offset = 0;
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.vertex_buffer, &stride, &offset);
    ctx->VSSetShader(guard ? g_res.vs_composite : g_res.vs, nullptr, 0);
    ctx->PSSetShader(guard ? g_res.ps_composite : g_res.ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
    if (guard) ctx->PSSetShaderResources(0, 1, &g_res.comp_depth_srv);
    // Shadow atlas for the per-pixel map compare (harmless when the table
    // is empty: shadowMeta.x = 0 short-circuits the shader). Slot t1 is
    // inside the StateBackup's saved SRV range.
    if (guard && shadow_live_ensure_srv()) ctx->PSSetShaderResources(1, 1, &g_ls.atlas_srv);
    if (mask_ensure_srv(ctx)) ctx->PSSetShaderResources(3, 1, &g_mask.srv);
    // engine shadow mask (captured at the resolve sweep): the depth-gated
    // receive samples it where box surfaces hug world geometry - unit
    // stencil shadows land on our boxes through the engine's own factor.
    for (UINT bslot = 0; bslot < 8; ++bslot) {
        if (g_ls.band[bslot].valid && g_ls.band[bslot].srv) {
            const UINT treg = bslot < 6 ? 4 + bslot : 12 + (bslot - 6);   // t4-t9, t12-t13
            ctx->PSSetShaderResources(treg, 1, &g_ls.band[bslot].srv);
        }
    }
    // Depth WRITE is the whole point: the engine's translucents must be
    // able to depth-reject against these boxes.
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    const FLOAT bf[4] = { 0, 0, 0, 0 };


    for (const auto& o : boxes) {
        ConstantData cbd = {};
        memcpy(cbd.view_proj, view_proj, sizeof(cbd.view_proj));
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y]
        cbd.center_size[2] = o.pos[1];
        cbd.size_axes[0] = o.size[0];    // SQF [x,y,z] sizes -> engine [x,z,y]
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        // The camera (engine space) serves both the guard's true fragment
        // distance AND the solid band mask, so it is filled regardless.
        cbd.fx0[0] = cam[0];
        cbd.fx0[1] = cam[1];
        cbd.fx0[2] = cam[2];
        // Band / local-volume mask inputs (same conversion as the flush).
        cbd.local0[0] = o.pos[0];
        cbd.local0[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y]
        cbd.local0[2] = o.pos[1];
        cbd.local0[3] = static_cast<float>(o.local_shape);
        cbd.local_radii[0] = o.local_radius[0];   // SQF [x,y,z] radii -> engine [x,z,y]
        cbd.local_radii[1] = o.local_radius[2];
        cbd.local_radii[2] = o.local_radius[1];
        const float mean_r = (o.local_radius[0] + o.local_radius[1] + o.local_radius[2]) / 3.0f;
        cbd.local1[0] = o.local_falloff / (mean_r > 0.01f ? mean_r : 0.01f);
        cbd.local1[1] = o.localized ? 1.0f : 0.0f;
        cbd.band0[0] = o.band_min;
        cbd.band0[1] = o.band_max;
        cbd.band0[2] = o.band_falloff;
        cbd.band0[3] = o.banded ? 1.0f : 0.0f;
        fill_lighting_cb(cbd, o);
        cbd.mask_meta[0] = (g_mask.enable > 0.5f && g_mask.srv) ? 1.0f : 0.0f;
        cbd.mask_meta[1] = g_mask.channel;
        cbd.mask_meta[2] = g_mask.invert;
        cbd.mask_meta[3] = 0.0f;

        if (g_shadow_pair_mode >= 1.5f) {
            bool band_any = false;
            // FINEST-FIRST: order slots by near edge ascending, so the
            // shader's first containing band holds the finest content in
            // the engine's overlap (crossfade) zones. bandBorder.w encodes
            // valid + which PHYSICAL texture slot to sample (1 + index).
            int order[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

            for (int a = 0; a < 7; ++a) {
                for (int b2 = a + 1; b2 < 8; ++b2) {
                    const float na = g_ls.band[order[a]].valid ? g_ls.band[order[a]].border[0] : 1e9f;
                    const float nb = g_ls.band[order[b2]].valid ? g_ls.band[order[b2]].border[0] : 1e9f;

                    if (nb < na) {
                        const int t = order[a];
                        order[a] = order[b2];
                        order[b2] = t;
                    }
                }
            }

            float prev_far = -1e9f;

            for (int b = 0; b < 8; ++b) {
                const auto& bs = g_ls.band[order[b]];
                cbd.band_border[b][3] = 0.0f;
                if (!bs.valid || !bs.srv) continue;

                // twin dedupe: near inside the previous band's range and
                // far not meaningfully beyond it = a partition duplicate
                // of the same range; two twins alternate seal content and
                // the shader's winner flip-flops (the range flicker).
                if (bs.border[0] < prev_far - 0.5f && bs.border[1] < prev_far * 1.25f + 1.0f) continue;
                prev_far = bs.border[1];
                memcpy(cbd.band_mat[b * 3 + 0], bs.sm + 0, 16);
                memcpy(cbd.band_mat[b * 3 + 1], bs.sm + 4, 16);
                memcpy(cbd.band_mat[b * 3 + 2], bs.sm + 8, 16);
                memcpy(cbd.band_view[b * 3 + 0], bs.vcol + 0, 16);
                memcpy(cbd.band_view[b * 3 + 1], bs.vcol + 4, 16);
                memcpy(cbd.band_view[b * 3 + 2], bs.vcol + 8, 16);
                cbd.band_border[b][0] = bs.border[0];
                cbd.band_border[b][1] = bs.border[1];
                cbd.band_border[b][2] = bs.border[2];
                cbd.band_border[b][3] = 1.0f + static_cast<float>(order[b]);
                band_any = true;
            }

            if (band_any) cbd.mask_meta[3] = 1.0f;

            // engine-mask depth-gated receive: registration-exact shadows
            // (units, clouds, everything the engine resolves) on box
            // surfaces within the depth margin of world geometry.

            // fog occlusion (self-fetched fogParams; RV's height-decay
            // exponential). Color: the staged ambient tint is the closest
            // engine-truth horizon approximation available without the
            // sky pipeline's own constants.
            if (g_fog_valid && g_fog[0] > 1e-4f) {
                cbd.fog_params[0] = g_fog[0];
                cbd.fog_params[1] = g_fog[1];
                cbd.fog_params[2] = g_fog[2];
                cbd.fog_params[3] = 1.0f;
                // RAW scene-lighting units - the same HDR space ApplyLighting
                // shades the box in. Normalizing to unit peak made the fog
                // white; on a white box that is invisible by construction
                // (the red tracer showed for exactly this reason). Horizon
                // approximation: ambient plus a quarter of the sun term.
                const float ab = g_amb_brightness > 0.0f ? g_amb_brightness :
                                 (g_sun_brightness > 0.0f ? g_sun_brightness * 0.8f : 1.0f);
                const float sb = g_sun_brightness > 0.0f ? g_sun_brightness : 1.0f;

                for (int c = 0; c < 3; ++c) {
                    // ambient-anchored haze: 0.25*sun made far boxes GLOW
                    // (sunBrightness 6-9 in HDR units). 0.9*amb + 0.06*sun
                    // sits at the engine's own horizon level; adjust these
                    // two constants if box haze drifts from terrain haze.
                    cbd.fog_color[c] = g_amb_color[c] * ab * 0.9f + g_sun_color[c] * sb * 0.06f;
                }

                cbd.fog_color[3] = g_ls.cam[1];   // camera altitude (engine Y-up)
            }
        }
        cbd.mask_dims[0] = g_mask.dims[0];
        cbd.mask_dims[1] = g_mask.dims[1];

        {
            UINT nvp = 1;
            D3D11_VIEWPORT vvp = {};
            ctx->RSGetViewports(&nvp, &vvp);
            cbd.mask_dims[2] = nvp && vvp.Width > 0 ? vvp.Width : g_mask.dims[0];
            cbd.mask_dims[3] = nvp && vvp.Height > 0 ? vvp.Height : g_mask.dims[1];
        }

        if (guard) {
            // Guard inputs: reconstruction coefficients + the encode range
            // of the copied depth, the copy's pixel dimensions, and the
            // margins.
            cbd.depth_params[0] = pv.projection[2][2];
            cbd.depth_params[1] = pv.projection[3][2];
            cbd.depth_params[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
            cbd.depth_params[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
            cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
            cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
            cbd.fx1[0] = measured ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
            cbd.fx1[1] = measured ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) continue;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.composite_cb, 0);
        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
        ctx->Draw(36, 0);
        g_stats.composite_boxes++;
    }

    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);

    {
        backup.restore(ctx);
    }
    g_stats.composite_injections++;
    g_composite_inject_serial.fetch_add(1, std::memory_order_relaxed);

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_inject < 0.0f) {
        g_mask.cold_first_inject = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }

}

inline void reorder_pre_draw(ID3D11DeviceContext* self) {
    // Vtable hooks intercept EVERY context: Arma's worker threads record
    // draws through deferred contexts at enormous rates, and anything
    // placed before these two loads (a QPC timer + a shared atomic, in
    // the first profiler build) becomes a cross-core cache-line storm -
    // a measured 172 ms/frame of self-inflicted contention. Foreign
    // contexts and threads must bail on relaxed loads only.
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    if (!reorder_on_render_thread()) return;   // our own game-thread draws must not be tracked

    if (!g_ro.in_injection) {
        shadow_note_draw(self);
        mask_note_draw(self);
        // The hybrid's earlier no-fire was the latch collapse (bisected
        // and fixed): its arm/reset chain rides the finalize cycle,
        // which is healthy again. castMisses instruments every bail.
        mask_cast_engine(self);
    }
    if (g_ro.in_injection || g_ro.injected) return;

    if (!g_ro.blend_translucent || !g_ro.dss_nowrite) {
        if (g_ro.dsv_main) {
            // An opaque draw against the main scene depth: evidence that a
            // genuine scene pass is in progress, and the source of this
            // cycle's authoritative viewport depth range.
            ++g_ro.opaque_draws;

            if ((g_ro.opaque_draws & 15) == 0) {
                UINT n_vp = 1;
                D3D11_VIEWPORT vp = {};
                self->RSGetViewports(&n_vp, &vp);

                if (n_vp >= 1) {
                    // PARTITION CHANGE RE-ARM: a single depth clear can
                    // cover MULTIPLE render partitions - far terrain and
                    // clouds first, the near world after, each under its
                    // own viewport depth range. The far partition can
                    // qualify entirely on its own (its opaques sample its
                    // own range, clouds provide the translucent trigger)
                    // and would SPEND the injection, leaving the near
                    // world's translucent phase - where the boxes actually
                    // matter - with nothing: the box blinks out for the
                    // frame. When the range shifts between opaque samples,
                    // a new partition has begun: re-arm so it gets its own
                    // injection (an earlier far-partition draw of the box
                    // is simply corrected by the nearer re-draw).
                    if (g_ro.cycle_vp_valid &&
                        (fabsf(vp.MinDepth - g_ro.cycle_vp_min) > 0.002f ||
                         fabsf(vp.MaxDepth - g_ro.cycle_vp_max) > 0.002f)) {
                        g_ro.injected = false;
                        g_ro.inject_attempts = 0;
                        // The opaque count deliberately SURVIVES the
                        // re-arm. Resetting it (an earlier weapon-slice
                        // fix, since superseded by the span gate's
                        // MaxDepth test) starved whole frames: when close
                        // to large geometry the engine's partition
                        // boundaries drift per frame, a late-phase range
                        // change re-armed with a zeroed count, and the
                        // world's translucents arrived against a floor
                        // that cannot refill mid-phase - multi-frame
                        // blink-out bursts at close range.
                        g_stats.composite_rearms++;
                    }

                    g_ro.cycle_vp_min = vp.MinDepth;
                    g_ro.cycle_vp_max = vp.MaxDepth;
                    g_ro.cycle_vp_valid = true;
                }
            }
        }

        return;
    }

    if (!g_ro.dsv_main) {
        if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) g_mask.cold_g_nodsv++;
        return;
    }

    // Mistiming rejection: a real scene issues a wall of opaque draws
    // between its depth clear and its translucent phase; late re-clears
    // and preview/overlay passes mostly do not - without this, such passes
    // could pull the injection into a completed frame and paint the boxes
    // over everything. Beyond that, the FIRST qualifying translucent draw
    // is the trigger (see the run-gate note above the vtable slots).
    // Terrain-only frames (looking straight down) issue only a few dozen
    // opaque draws - the original threshold never qualified, no injection
    // landed all frame, and the fallback path drew the boxes UNSHADOWED
    // (both shadows 'vanishing' on look-down). A proven projection lock
    // is stronger mistiming evidence than draw count alone: with it, a
    // much lower floor suffices; previews/overlays still fail both.
    const uint32_t min_opaques = g_ro.engine_proj_valid
                               ? KH_REORDER_MIN_OPAQUE_DRAWS / 8
                               : KH_REORDER_MIN_OPAQUE_DRAWS;

    if (g_ro.opaque_draws < min_opaques) {
        g_stats.composite_rej_floor++;
        if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) g_mask.cold_g_floor++;
        return;
    }

    // Floor-relaxed frames (sparse: look-down terrain, look-up sky) must
    // ALSO be on the frame's true main depth: a sparse cycle passing the
    // low floor can be a preview/aux pass whose output is discarded -
    // injection lands, the serial reports success, the box vanishes at
    // exactly those camera angles. dsv_main is identity-matched; the
    // real scene always qualifies, previews never do. Dense frames keep
    // the draw-count evidence alone, as always.
    if (g_ro.opaque_draws < KH_REORDER_MIN_OPAQUE_DRAWS && !g_ro.dsv_main) {
        g_stats.composite_rej_floor++;
        if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) g_mask.cold_g_sparse++;
        return;
    }

    // CYCLE DISCRIMINATION - the decisive gate. The engine runs MORE THAN
    // ONE clear->draw cycle on the same main depth resource per frame
    // (weapon/hands pass, reflection or overlay renders); each mid-frame
    // clear re-arms this state machine, and a secondary cycle that happens
    // to contain enough opaque draws plus one blended draw would either
    // fire a SECOND injection over the completed frame - box over opaques,
    // translucents AND particles at once - or, if it runs before the world,
    // SPEND the injection so the real scene gets no box at all. The
    // fingerprint is the viewport DEPTH RANGE: this cycle's translucent
    // phase must run under the SAME range its own opaque phase used
    // (captured above on this thread, this cycle). A mismatch means the
    // engine moved on to a different partition or pass: pass through
    // without consuming an attempt, leaving the injection armed for a
    // draw that matches. Comparing against the cycle's OWN capture -
    // rather than any cross-thread probe - is what makes this immune to
    // the flush parking the render thread inside a secondary pass and
    // recording the wrong range.
    {
        // WORLD-SHAPE test on the trigger draw's OWN live range - the sole
        // range gate, by design. Only a partition spanning the BROAD
        // MIDDLE of [0, 1] is the world pass: a range starting deep is a
        // far partition (distant terrain, clouds); one that never reaches
        // the far side is the weapon/hands slice (rendered AFTER the
        // world - injecting there paints the box over the completed
        // frame). An earlier revision ALSO required identity with the
        // cycle's opaque-phase sample; that gate was redundant where it
        // was right and unrecoverable where it was wrong (a poisoned
        // sample cannot heal during the translucent phase - no opaques
        // run there to refresh it), silently rejecting whole frames.
        UINT n_vp = 1;
        D3D11_VIEWPORT vp = {};
        self->RSGetViewports(&n_vp, &vp);
        if (n_vp < 1) return;

        if (vp.MinDepth > 0.3f || vp.MaxDepth < 0.7f) {
            g_stats.composite_rej_span++;
            return;
        }

        // The passing trigger's range IS the encode range: the world's
        // translucents depth-test correctly through it against the
        // opaque-written depth, so the boxes must encode through it too.
        g_ro.trig_vp_min = vp.MinDepth;
        g_ro.trig_vp_max = vp.MaxDepth;
        g_ro.trig_vp_valid = true;
    }

    if (g_ro.inject_attempts >= KH_REORDER_MAX_INJECT_ATTEMPTS) return;
    ++g_ro.inject_attempts;

    // Authoritative verification at each attempt: confirm the LIVE depth
    // binding is the main scene's before committing. On mismatch, correct
    // the tracking and RETRY on a later qualifying draw this frame -
    // standing down entirely would blank the boxes for the whole frame
    // over a transient.
    {
        ID3D11DepthStencilView* live_dsv = nullptr;
        self->OMGetRenderTargets(0, nullptr, &live_dsv);
        void* id = nullptr;

        if (live_dsv) {
            id = reorder_dsv_identity(live_dsv);
            live_dsv->Release();
        }

        if (!id || id != g_main_depth_identity) {
            g_ro.dsv_main = false;
            g_stats.composite_rej_verify++;
            return;
        }
    }

    g_ro.injected = true;
    g_ro.in_injection = true;
    inject_composited_boxes(self);
    g_ro.in_injection = false;
}

// Known atlas SRV pointer cache: the engine reuses view objects, so after
// first identification the check is pointer-compare only - no COM calls.
static void* g_atlas_srv_cache[8] = {};
static uint32_t g_atlas_srv_count = 0;

static void STDMETHODCALLTYPE hooked_pssetshaderresources(ID3D11DeviceContext* self, UINT start, UINT n, ID3D11ShaderResourceView* const* srvs) {
    if (reorder_on_render_thread() && !g_ro.in_injection && g_ls.atlas_tex && srvs) {
        // The engine binds the shadow atlas at t15 (from its own resolve
        // shader) - inspect ONLY that slot. The previous per-SRV
        // GetResource scan over every binding call was the framerate.
        if (start <= 15 && start + n > 15) {
            ID3D11ShaderResourceView* srv = srvs[15 - start];

            if (srv) {
                bool known = false;

                for (uint32_t i = 0; i < g_atlas_srv_count; ++i) {
                    if (g_atlas_srv_cache[i] == srv) { known = true; break; }
                }

                if (known) {
                    g_mask.atlas_bound = true;
                } else {
                    ID3D11Resource* res = nullptr;
                    srv->GetResource(&res);

                    if (res) {
                        if (res == static_cast<ID3D11Resource*>(g_ls.atlas_tex)) {
                            g_mask.atlas_bound = true;

                            if (g_atlas_srv_count < 8) {
                                g_atlas_srv_cache[g_atlas_srv_count++] = srv;
                            }
                        }

                        res->Release();
                    }
                }
            }
        }
    }

    g_orig_pssetshaderresources(self, start, n, srvs);
}

static void STDMETHODCALLTYPE hooked_draw_indexed(ID3D11DeviceContext* self, UINT ic, UINT sil, INT bvl) {
    reorder_pre_draw(self);
    g_orig_draw_indexed(self, ic, sil, bvl);
}

static void STDMETHODCALLTYPE hooked_draw(ID3D11DeviceContext* self, UINT vc, UINT svl) {
    reorder_pre_draw(self);
    g_orig_draw(self, vc, svl);
}

static void STDMETHODCALLTYPE hooked_draw_indexed_instanced(ID3D11DeviceContext* self, UINT icpi, UINT ic, UINT sil, INT bvl, UINT sil2) {
    reorder_pre_draw(self);
    g_orig_draw_indexed_instanced(self, icpi, ic, sil, bvl, sil2);
}

static void STDMETHODCALLTYPE hooked_draw_instanced(ID3D11DeviceContext* self, UINT vcpi, UINT ic, UINT svl, UINT sil) {
    reorder_pre_draw(self);
    g_orig_draw_instanced(self, vcpi, ic, svl, sil);
}

static void STDMETHODCALLTYPE hooked_omset_blendstate(ID3D11DeviceContext* self, ID3D11BlendState* bs, const FLOAT bf[4], UINT mask) {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        reorder_on_render_thread()) {
        if (!bs) {
            g_ro.blend_translucent = false;   // default blend state: opaque
        } else {
            D3D11_BLEND_DESC d = {};
            bs->GetDesc(&d);
            // Translucent = blending enabled with a destination term:
            // covers alpha (INV_SRC_ALPHA), additive (ONE) and the rest.
            g_ro.blend_translucent = d.RenderTarget[0].BlendEnable &&
                                     d.RenderTarget[0].DestBlend != D3D11_BLEND_ZERO;
        }
    }

    g_orig_omset_blendstate(self, bs, bf, mask);
}

static void STDMETHODCALLTYPE hooked_omset_depthstencil(ID3D11DeviceContext* self, ID3D11DepthStencilState* dss, UINT ref) {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        reorder_on_render_thread()) {
        if (!dss) {
            g_ro.dss_nowrite = false;   // default DSS: depth test + write
        } else {
            D3D11_DEPTH_STENCIL_DESC d = {};
            dss->GetDesc(&d);
            g_ro.dss_nowrite = d.DepthEnable && d.DepthWriteMask == D3D11_DEPTH_WRITE_MASK_ZERO;
        }
    }

    g_orig_omset_depthstencil(self, dss, ref);
}


static void STDMETHODCALLTYPE hooked_omset_rendertargets(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv) {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        reorder_on_render_thread()) {
        g_ro.dsv_main = dsv && g_main_depth_identity &&
                        reorder_dsv_identity(dsv) == g_main_depth_identity;
        const bool was_on_atlas = g_ls.phase_on_atlas;
        shadow_track_targets(self, dsv, g_ro.dsv_main, n, rtvs);
        mask_apply_at_backbuffer(self, n, rtvs, dsv);

        if (was_on_atlas && !g_ls.phase_on_atlas) {
            g_ls.resolve_window = 4;
            // (pass-end cast retired: the phase exit is NOT a per-cascade
            // boundary - it fires ~1.3x/frame. Casting now happens at the
            // resolves, the true per-cascade cadence, with the stashed
            // world-relative map.)
        }

        mask_classify_rt(n, rtvs);
    }

    g_orig_omset_rendertargets(self, n, rtvs, dsv);
}

static void STDMETHODCALLTYPE hooked_omset_rts_and_uavs(ID3D11DeviceContext* self, UINT n, ID3D11RenderTargetView* const* rtvs, ID3D11DepthStencilView* dsv, UINT uav_start, UINT n_uavs, ID3D11UnorderedAccessView* const* uavs, const UINT* counts) {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        reorder_on_render_thread() && n != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL) {
        // The DSV changes through this path too; leaving it untracked lets
        // the flag go stale (n == KEEP leaves the current bindings alone).
        g_ro.dsv_main = dsv && g_main_depth_identity &&
                        reorder_dsv_identity(dsv) == g_main_depth_identity;
        shadow_track_targets(self, dsv, g_ro.dsv_main, n, rtvs);
    }

    g_orig_omset_rts_and_uavs(self, n, rtvs, dsv, uav_start, n_uavs, uavs, counts);
}

static void STDMETHODCALLTYPE hooked_clear_depthstencil(ID3D11DeviceContext* self, ID3D11DepthStencilView* dsv, UINT flags, FLOAT depth, UINT8 stencil) {
    if (self == g_reorder_target_ctx.load(std::memory_order_relaxed) && !g_ro.in_injection &&
        dsv && (flags & D3D11_CLEAR_DEPTH)) {
        if (g_main_depth_identity && reorder_dsv_identity(dsv) == g_main_depth_identity) {
            // The engine clears the main scene depth on its render thread:
            // this is where that thread is identified for the tracking gate.
            g_reorder_render_tid.store(GetCurrentThreadId(), std::memory_order_relaxed);

            // A depth clear of the main scene buffer marks the new frame:
            // injection re-arms and the phase evidence resets.
            g_ro.injected = false;
            g_ro.opaque_draws = 0;
            g_ro.inject_attempts = 0;
            g_ro.cycle_vp_valid = false;
            g_ro.trig_vp_valid = false;
            g_ro.engine_proj_valid = false;
            shadow_close_cycle();   // frame boundary: fold the recon cycle into the stats
            shadow_live_frame_reset();
            shadow_view_prewarm();   // lock can exist before the first box does
            g_mask_cast_fired = false;   // new frame: one analytic pass allowed
            g_mask_cast_arm = false;   // don't arm at the boundary: this frame's
                                       // gated capture hasn't run yet, so the fire
                                       // would use the PREVIOUS frame's depth
                                       // snapshot - on partitioned look-down frames
                                       // near the box that stale close-range depth
                                       // MIN-darkened the screen (the overcast).
                                       // The capture arms instead (dual-gated).
            g_ls.view_published_this_frame = false;

            // Latch this frame's matrices NOW, in lockstep with the engine's
            // own frame-setup snapshot (see the cycle_pv note above). On a
            // transient fetch failure the previous cycle's latch is kept -
            // one frame stale beats the mid-frame fetch this replaces.
            RVExtBridge::ProjectionViewTransform pv = {};

            if (RVExtBridge::get_projection_view_transform(pv)) {
                g_ro.cycle_pv = pv;
                g_ro.cycle_pv_valid = true;
            }
        }
    }

    g_orig_clear_depthstencil(self, dsv, flags, depth, stencil);
}

// Installs the reorder hooks on the immediate context's vtable. Called every
// flush (cheap early-out): retries until the bridge exposes the context,
// refreshes the tracked context pointer across device resets, and gives up
// permanently only on a definitive MinHook failure - in which case the
// post-scene flush path carries on as before.
inline void ensure_reorder_hook() {
    ID3D11DeviceContext* ctx = RVExtBridge::get_d3d_device_context();
    if (ctx) g_reorder_target_ctx.store(static_cast<void*>(ctx), std::memory_order_relaxed);
    if (g_reorder_hook_active.load(std::memory_order_acquire) || g_reorder_hook_failed) return;
    if (!ctx) return;   // bridge not ready: retry next flush

    if (!ensure_minhook()) {
        g_reorder_hook_failed = true;
        report_error("KH RenderIntegration: MinHook init failed; composited rendering disabled");
        return;
    }

    void** vt = *reinterpret_cast<void***>(ctx);
    struct HookSpec { int slot; void* detour; void** original; };

    const HookSpec specs[] = {
        { KH_VT_PSSETSHADERRESOURCES, reinterpret_cast<void*>(&hooked_pssetshaderresources),  reinterpret_cast<void**>(&g_orig_pssetshaderresources) },
        { KH_VT_DRAWINDEXED,          reinterpret_cast<void*>(&hooked_draw_indexed),           reinterpret_cast<void**>(&g_orig_draw_indexed) },
        { KH_VT_DRAW,                 reinterpret_cast<void*>(&hooked_draw),                   reinterpret_cast<void**>(&g_orig_draw) },
        { KH_VT_DRAWINDEXEDINSTANCED, reinterpret_cast<void*>(&hooked_draw_indexed_instanced), reinterpret_cast<void**>(&g_orig_draw_indexed_instanced) },
        { KH_VT_DRAWINSTANCED,        reinterpret_cast<void*>(&hooked_draw_instanced),         reinterpret_cast<void**>(&g_orig_draw_instanced) },
        { KH_VT_OMSETRENDERTARGETS,   reinterpret_cast<void*>(&hooked_omset_rendertargets),    reinterpret_cast<void**>(&g_orig_omset_rendertargets) },
        { KH_VT_OMSETRTS_AND_UAVS,    reinterpret_cast<void*>(&hooked_omset_rts_and_uavs),     reinterpret_cast<void**>(&g_orig_omset_rts_and_uavs) },
        { KH_VT_OMSETBLENDSTATE,      reinterpret_cast<void*>(&hooked_omset_blendstate),       reinterpret_cast<void**>(&g_orig_omset_blendstate) },
        { KH_VT_OMSETDEPTHSTENCIL,    reinterpret_cast<void*>(&hooked_omset_depthstencil),     reinterpret_cast<void**>(&g_orig_omset_depthstencil) },
        { KH_VT_CLEARDEPTHSTENCIL,    reinterpret_cast<void*>(&hooked_clear_depthstencil),     reinterpret_cast<void**>(&g_orig_clear_depthstencil) },
        { KH_VT_MAP,                  reinterpret_cast<void*>(&hooked_map),                     reinterpret_cast<void**>(&g_orig_map) },
        { KH_VT_UNMAP,                reinterpret_cast<void*>(&hooked_unmap),                   reinterpret_cast<void**>(&g_orig_unmap) },
        { KH_VT_UPDATESUBRESOURCE,    reinterpret_cast<void*>(&hooked_updatesubresource),      reinterpret_cast<void**>(&g_orig_updatesubresource) },
    };

    size_t created = 0;

    for (const auto& s : specs) {
        if (MH_CreateHook(vt[s.slot], s.detour, s.original) != MH_OK) break;
        ++created;
    }

    if (created == sizeof(specs) / sizeof(specs[0])) {
        size_t enabled = 0;

        for (const auto& s : specs) {
            if (MH_EnableHook(vt[s.slot]) != MH_OK) break;
            ++enabled;
        }

        if (enabled == created) {
            g_reorder_hook_active.store(true, std::memory_order_release);
            return;
        }

        for (size_t i = 0; i < enabled; ++i) MH_DisableHook(vt[specs[i].slot]);
    }

    for (size_t i = 0; i < created; ++i) MH_RemoveHook(vt[specs[i].slot]);
    g_reorder_hook_failed = true;
    report_error("KH RenderIntegration: draw hook install failed; composited rendering disabled");
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
    bool _pv_ok;

    {
        _pv_ok = RVExtBridge::get_projection_view_transform(pv);
    }

    if (!_pv_ok) return false;
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
    // Publish this frame's staged world lighting for the render-thread
    // consumers (composited-path shading, shadow recon). The graphics lock
    // is held for all of flush_locked, which PARKS the render thread - the
    // same invariant the injection relies on for lock-free context use - so
    // these plain stores can never be observed mid-write.
    publish_world_lighting();

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
    const float snapshot_now = effect_time_seconds();
    // A single missed injection inside the 500 ms health window used to
    // leave composited boxes undrawn for that frame - the visible blink
    // when a gate transiently rejects during a camera sweep. The flush now
    // stands down only when an injection actually LANDED since the previous
    // flush; a missed frame falls back to the post-scene draw instead of
    // vanishing. Double-draw on phase misalignment is benign: identical
    // opaque geometry at equal depth resolves to the same pixels.
    static uint64_t s_prev_inject_serial = 0;   // flush runs on the game thread only
    const uint64_t inject_serial = g_composite_inject_serial.load(std::memory_order_relaxed);
    const bool injected_since_last_flush = inject_serial != s_prev_inject_serial;
    s_prev_inject_serial = inject_serial;
    const bool comp_healthy = composite_path_healthy() && injected_since_last_flush;
    std::vector<RenderObject> boxes;
    std::vector<std::pair<uint32_t, RenderObject>> fullscreen;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        boxes.reserve(g_draw_list.size());

        for (auto it = g_draw_list.begin(); it != g_draw_list.end(); ) {
            bool expired = false;
            const float env = lifetime_envelope(it->second, snapshot_now, expired);

            if (expired) {
                it = g_draw_list.erase(it);   // lifetime over: auto-remove
                continue;
            }

            if (it->second.visible) {
                RenderObject o = it->second;
                o.color[3] *= env;   // envelope = universal intensity

                if (o.fullscreen) {
                    if (!o.affect_ui) fullscreen.emplace_back(it->first, o);
                } else if (!(comp_healthy && is_composite_eligible(o))) {
                    // Composited boxes are drawn pre-translucent by the
                    // reorder hook; the flush stands down for them while
                    // injections are actually happening.
                    boxes.push_back(o);
                }
            }

            ++it;
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

            // The Draw3D park point can land inside a SECONDARY render
            // cycle whose viewport range is a narrow slice; recording that
            // would poison the UI-phase depth linearization. Only accept a
            // range that spans the bulk of [0, 1] - the scene range does,
            // sub-passes do not. (The injection path no longer depends on
            // these globals at all; see the reorder machinery.)
            if (vp.MinDepth < 0.5f && vp.MaxDepth > 0.5f) {
                g_scene_vp_min_d = vp.MinDepth;
                g_scene_vp_max_d = vp.MaxDepth;
            }
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

        // overlay boxes (mode 2, no hardware depth test) occlude via the
        // shader guard, which needs REAL depth: a solid overlay box must
        // keep the depth machinery alive even with no effects present
        // (with both dormant, t0 read null -> sceneZ 0 -> the zero-margin
        // guard discarded every fragment: the box despawned once the
        // engine's stale t0 leftovers drifted to zero).
        if (needs_depth(o) || (o.effect == 0 && o.mode == DepthMode::Off)) {
            any_effect = true;
            any_depth_fx = true;
        }
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

        // Solid boxes repurpose fx0.xyz (unused effect params at effect 0)
        // for the camera, which SolidMask's band term measures against.
        if (o.effect == 0) {
            cbd.fx0[0] = cam[0];
            cbd.fx0[1] = cam[1];
            cbd.fx0[2] = cam[2];

        }

        cbd.fx_meta[0] = static_cast<float>(o.effect);
        cbd.fx_meta[1] = now;
        cbd.fx_meta[2] = screen_w;
        cbd.fx_meta[3] = screen_h;
        cbd.depth_params[0] = pv.projection[2][2];
        cbd.depth_params[1] = pv.projection[3][2];
        cbd.depth_params[2] = vp_min_d;
        cbd.depth_params[3] = vp_max_d;

        // SOLID BOXES: align the guard's inputs to the INJECTION's
        // field-proven fill, VERBATIM - reconstruction coefficients from
        // the MEASURED engine projection (the bridge's convention fed
        // the guard wrong sceneZ), the SCENE's depth-encode range (the
        // snapshot was written under it; the flush-time viewport range
        // was not it), the SNAPSHOT's pixel dims (the earlier override
        // sat above the general fx_meta fill and was silently
        // overwritten - LoadDepthC scaled by screen dims read out of
        // bounds), and the injection's own guard margin constants.
        // Without a fresh snapshot: guard stands down. Unoccluded,
        // never invisible.
        if (o.effect == 0) {
            const bool snap_fresh = g_res.comp_depth_srv &&
                                    g_res.comp_depth_time >= 0.0f &&
                                    now - g_res.comp_depth_time < 0.5f;

            if (snap_fresh) {
                const bool measured = g_ro.engine_proj_valid;

                if (measured) {
                    cbd.depth_params[0] = g_ro.engine_m22;
                    cbd.depth_params[1] = g_ro.engine_m32;
                }

                cbd.depth_params[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
                cbd.depth_params[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
                cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
                cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
                cbd.fx1[0] = measured ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
                cbd.fx1[1] = measured ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
            } else {
                cbd.fx1[0] = 1e9f;
                cbd.fx1[1] = 0.0f;
            }
        }
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
        fill_lighting_cb(cbd, o);
        cbd.mask_meta[0] = (g_mask.enable > 0.5f && g_mask.srv) ? 1.0f : 0.0f;
        cbd.mask_meta[1] = g_mask.channel;
        cbd.mask_meta[2] = g_mask.invert;
        cbd.mask_meta[3] = 0.0f;

        if (g_shadow_pair_mode >= 1.5f) {
            bool band_any = false;
            // FINEST-FIRST: order slots by near edge ascending, so the
            // shader's first containing band holds the finest content in
            // the engine's overlap (crossfade) zones. bandBorder.w encodes
            // valid + which PHYSICAL texture slot to sample (1 + index).
            int order[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };

            for (int a = 0; a < 7; ++a) {
                for (int b2 = a + 1; b2 < 8; ++b2) {
                    const float na = g_ls.band[order[a]].valid ? g_ls.band[order[a]].border[0] : 1e9f;
                    const float nb = g_ls.band[order[b2]].valid ? g_ls.band[order[b2]].border[0] : 1e9f;

                    if (nb < na) {
                        const int t = order[a];
                        order[a] = order[b2];
                        order[b2] = t;
                    }
                }
            }

            float prev_far = -1e9f;

            for (int b = 0; b < 8; ++b) {
                const auto& bs = g_ls.band[order[b]];
                cbd.band_border[b][3] = 0.0f;
                if (!bs.valid || !bs.srv) continue;

                // twin dedupe: near inside the previous band's range and
                // far not meaningfully beyond it = a partition duplicate
                // of the same range; two twins alternate seal content and
                // the shader's winner flip-flops (the range flicker).
                if (bs.border[0] < prev_far - 0.5f && bs.border[1] < prev_far * 1.25f + 1.0f) continue;
                prev_far = bs.border[1];
                memcpy(cbd.band_mat[b * 3 + 0], bs.sm + 0, 16);
                memcpy(cbd.band_mat[b * 3 + 1], bs.sm + 4, 16);
                memcpy(cbd.band_mat[b * 3 + 2], bs.sm + 8, 16);
                memcpy(cbd.band_view[b * 3 + 0], bs.vcol + 0, 16);
                memcpy(cbd.band_view[b * 3 + 1], bs.vcol + 4, 16);
                memcpy(cbd.band_view[b * 3 + 2], bs.vcol + 8, 16);
                cbd.band_border[b][0] = bs.border[0];
                cbd.band_border[b][1] = bs.border[1];
                cbd.band_border[b][2] = bs.border[2];
                cbd.band_border[b][3] = 1.0f + static_cast<float>(order[b]);
                band_any = true;
            }

            if (band_any) cbd.mask_meta[3] = 1.0f;

            // engine-mask depth-gated receive: registration-exact shadows
            // (units, clouds, everything the engine resolves) on box
            // surfaces within the depth margin of world geometry.

            // fog occlusion (self-fetched fogParams; RV's height-decay
            // exponential). Color: the staged ambient tint is the closest
            // engine-truth horizon approximation available without the
            // sky pipeline's own constants.
            if (g_fog_valid && g_fog[0] > 1e-4f) {
                cbd.fog_params[0] = g_fog[0];
                cbd.fog_params[1] = g_fog[1];
                cbd.fog_params[2] = g_fog[2];
                cbd.fog_params[3] = 1.0f;
                // RAW scene-lighting units - the same HDR space ApplyLighting
                // shades the box in. Normalizing to unit peak made the fog
                // white; on a white box that is invisible by construction
                // (the red tracer showed for exactly this reason). Horizon
                // approximation: ambient plus a quarter of the sun term.
                const float ab = g_amb_brightness > 0.0f ? g_amb_brightness :
                                 (g_sun_brightness > 0.0f ? g_sun_brightness * 0.8f : 1.0f);
                const float sb = g_sun_brightness > 0.0f ? g_sun_brightness : 1.0f;

                for (int c = 0; c < 3; ++c) {
                    // ambient-anchored haze: 0.25*sun made far boxes GLOW
                    // (sunBrightness 6-9 in HDR units). 0.9*amb + 0.06*sun
                    // sits at the engine's own horizon level; adjust these
                    // two constants if box haze drifts from terrain haze.
                    cbd.fog_color[c] = g_amb_color[c] * ab * 0.9f + g_sun_color[c] * sb * 0.06f;
                }

                cbd.fog_color[3] = g_ls.cam[1];   // camera altitude (engine Y-up)
            }
        }
        cbd.mask_dims[0] = g_mask.dims[0];
        cbd.mask_dims[1] = g_mask.dims[1];

        {
            UINT nvp = 1;
            D3D11_VIEWPORT vvp = {};
            ctx->RSGetViewports(&nvp, &vvp);
            cbd.mask_dims[2] = nvp && vvp.Width > 0 ? vvp.Width : g_mask.dims[0];
            cbd.mask_dims[3] = nvp && vvp.Height > 0 ? vvp.Height : g_mask.dims[1];
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.constant_buffer, 0);
        return true;
    };

    // --- Box pass ---
    for (const auto& o : boxes) {
        const bool ov = (o.effect == 0 && o.mode == DepthMode::Off);
        if (ov) g_mask.ov_listed++;      // reached the draw loop
        if (!upload_cb(o, false)) { if (ov) g_mask.ov_skipped++; continue; }
        if (ov) g_mask.ov_drawn++;       // Draw() will be issued below
        ctx->PSSetShader(o.effect > 0 ? g_res.ps_effect : g_res.ps, nullptr, 0);

        // PSComposite reads depth at t0; the flush's default pair puts
        // scene color there (for effect shaders). EVERY solid box swaps
        // the composite path's PROVEN depth snapshot into t0 for its
        // draw; the pair returns for effect boxes.
        if (o.effect == 0 && g_res.comp_depth_srv) {
            ctx->PSSetShaderResources(0, 1, &g_res.comp_depth_srv);
        } else {
            ctx->PSSetShaderResources(0, 2, ps_srvs);
        }

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


// ===========================================================================
// Game-thread lighting staging + per-object sun-occlusion rays. Both run
// from flush_frame BEFORE the graphics lock is taken: they are SQF/engine
// calls and must not extend the render-thread park. The rays use THIS
// frame's staged direction; publication to the render-thread-visible
// globals happens inside flush_locked.
// ===========================================================================

inline void stage_world_lighting() {
    g_sun_staged_valid = false;
    bool lit_exists = false;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;

            if (o.lit && o.visible && !o.fullscreen) {
                lit_exists = true;
                break;
            }
        }
    }

    // Live shadow-map capture follows lit objects; the recon keeps the sun
    // published even without any.
    g_ls.wanted.store(lit_exists, std::memory_order_relaxed);
    if (!lit_exists) return;

    try {
        // Parsed from the raw getLighting array - [color, brightness,
        // direction, starsVisibility] - rather than through a typed
        // wrapper, so this file does not depend on the wrapper's field
        // naming. Compiled once, on this (game) thread. raw_call_sqf_native
        // only returns what the code hands to setReturnValue (the
        // g_return_value channel) - the expression's own result is
        // discarded - so the value must be routed explicitly.
        static game_value s_compiled_get_lighting;

        if (s_compiled_get_lighting.is_nil()) {
            s_compiled_get_lighting = sqf::compile("setReturnValue getLighting");
        }

        // fog, same channel: [fogValue, fogDecay, fogBase]
        static game_value s_compiled_fog;

        if (s_compiled_fog.is_nil()) {
            s_compiled_fog = sqf::compile("setReturnValue fogParams");
        }

        g_fog_staged_valid = false;
        game_value fr = raw_call_sqf_native(s_compiled_fog);

        if (fr.type_enum() == game_data_type::ARRAY) {
            auto& fa = fr.to_array();

            if (fa.size() >= 3) {
                g_fog_staged[0] = static_cast<float>(fa[0]);
                g_fog_staged[1] = static_cast<float>(fa[1]);
                g_fog_staged[2] = static_cast<float>(fa[2]);
                g_fog_staged_valid = true;
            }
        }

        game_value r = raw_call_sqf_native(s_compiled_get_lighting);
        if (r.type_enum() != game_data_type::ARRAY) return;
        auto& a = r.to_array();
        if (a.size() < 3) return;
        if (a[0].type_enum() != game_data_type::ARRAY || a[2].type_enum() != game_data_type::ARRAY) return;
        auto& col = a[0].to_array();
        auto& dir = a[2].to_array();
        if (col.size() < 3 || dir.size() < 3) return;

        float d[3] = { static_cast<float>(dir[0]), static_cast<float>(dir[1]), static_cast<float>(dir[2]) };
        const float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (len < 1e-6f) return;
        d[0] /= len; d[1] /= len; d[2] /= len;

        // getLighting's direction is the direction the light TRAVELS
        // (downward during the day). Shading and recon both want the vector
        // TOWARD the light: flip into the sky-side hemisphere. (The recon's
        // axis test uses |dot| and is sign-agnostic regardless.)
        if (d[2] < 0.0f) { d[0] = -d[0]; d[1] = -d[1]; d[2] = -d[2]; }

        g_sun_staged_dir_sqf[0] = d[0];
        g_sun_staged_dir_sqf[1] = d[1];
        g_sun_staged_dir_sqf[2] = d[2];
        // SQF [x, y, z-up] -> engine [x, up, y]
        g_sun_staged_dir_engine[0] = d[0];
        g_sun_staged_dir_engine[1] = d[2];
        g_sun_staged_dir_engine[2] = d[1];

        float c[3] = { static_cast<float>(col[0]), static_cast<float>(col[1]), static_cast<float>(col[2]) };
        float m = c[0];
        if (c[1] > m) m = c[1];
        if (c[2] > m) m = c[2];
        if (m < 1e-4f) m = 1e-4f;
        // Max-component normalization: the light modulates the box color's
        // HUE without changing its magnitude, so a lit box in full sun
        // keeps the months-stable unlit look (getLighting's raw brightness
        // is exposure-coupled and would blow the color out; it is kept only
        // as a diagnostic).
        g_sun_staged_color[0] = c[0] / m;
        g_sun_staged_color[1] = c[1] / m;
        g_sun_staged_color[2] = c[2] / m;

        g_sun_staged_brightness =
            (a.size() > 1 && a[1].type_enum() == game_data_type::SCALAR) ? static_cast<float>(a[1]) : 0.0f;

        // optional ambient: elem 4 = [r,g,b], elem 5 = brightness
        if (a.size() > 4 && a[4].type_enum() == game_data_type::ARRAY) {
            auto& ac = a[4].to_array();

            if (ac.size() >= 3) {
                float am = static_cast<float>(ac[0]);
                if (static_cast<float>(ac[1]) > am) am = static_cast<float>(ac[1]);
                if (static_cast<float>(ac[2]) > am) am = static_cast<float>(ac[2]);
                if (am < 1e-4f) am = 1e-4f;
                RenderIntegration::g_amb_staged_color[0] = static_cast<float>(ac[0]) / am;
                RenderIntegration::g_amb_staged_color[1] = static_cast<float>(ac[1]) / am;
                RenderIntegration::g_amb_staged_color[2] = static_cast<float>(ac[2]) / am;
            }
        }

        if (a.size() > 5 && a[5].type_enum() == game_data_type::SCALAR) {
            RenderIntegration::g_amb_staged_brightness = static_cast<float>(a[5]);
            RenderIntegration::g_amb_brightness_staged_flag = true;
        }

        g_sun_staged_valid = true;
    } catch (...) {
        // lighting unavailable this frame: lit objects fall back to unlit
    }
}

static constexpr uint32_t KH_SHADOW_RAYS_PER_FRAME = 64;   // engine raycast budget per frame
static constexpr float    KH_SHADOW_RAY_LENGTH = 2000.0f;  // toward the sun (m)
static constexpr float    KH_SHADOW_SMOOTH_RATE = 10.0f;   // 1/s; ~100 ms settle across a shadow edge

inline void update_shadow_rays() {
    if (!g_sun_staged_valid) return;

    struct RayJob {
        uint32_t handle;
        float pos[3];
        float half[3];
        int   mode;
        float factor;
        float update_time;
    };

    static std::vector<RayJob> jobs;   // game-thread scratch
    jobs.clear();

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;
            if (!o.lit || o.fullscreen || !o.visible || o.shadow_mode <= 0) continue;
            RayJob j;
            j.handle = kv.first;
            j.pos[0] = o.pos[0]; j.pos[1] = o.pos[1]; j.pos[2] = o.pos[2];
            j.half[0] = o.size[0] * 0.5f; j.half[1] = o.size[1] * 0.5f; j.half[2] = o.size[2] * 0.5f;
            j.mode = o.shadow_mode;
            j.factor = o.shadow_factor;
            j.update_time = o.shadow_update_time;
            jobs.push_back(j);
        }
    }

    if (jobs.empty()) return;

    // The rays run WITHOUT the draw-list mutex: checkVisibility is an
    // engine call, and the injection takes this mutex on the render thread
    // - holding it across a batch of raycasts would stall a hooked draw.
    const float now = effect_time_seconds();
    const object ignore = sqf::obj_null();
    const float sx = g_sun_staged_dir_sqf[0];
    const float sy = g_sun_staged_dir_sqf[1];
    const float sz = g_sun_staged_dir_sqf[2];
    uint32_t budget = KH_SHADOW_RAYS_PER_FRAME;

    for (auto& j : jobs) {
        if (budget == 0) break;
        float acc = 0.0f;
        uint32_t n = 0;

        auto cast = [&](float ox, float oy, float oz) {
            if (budget == 0) return;
            --budget;
            const vector3 begin(j.pos[0] + ox, j.pos[1] + oy, j.pos[2] + oz);

            const vector3 end(begin.x + sx * KH_SHADOW_RAY_LENGTH,
                              begin.y + sy * KH_SHADOW_RAY_LENGTH,
                              begin.z + sz * KH_SHADOW_RAY_LENGTH);

            // "VIEW" so vegetation and view geometry occlude the sun the
            // way they visually do; returns 0..1 (partial through glass).
            acc += sqf::check_visibility(ignore, "VIEW", begin, end);
            ++n;
        };

        cast(0.0f, 0.0f, 0.0f);

        if (j.mode >= 2) {
            for (int cx = -1; cx <= 1; cx += 2)
            for (int cy = -1; cy <= 1; cy += 2)
            for (int cz = -1; cz <= 1; cz += 2) {
                cast(j.half[0] * cx, j.half[1] * cy, j.half[2] * cz);
            }
        }

        if (n == 0) continue;
        const float target = acc / static_cast<float>(n);
        // Exponential settle instead of a hard pop when crossing an edge.
        float sf = target;

        if (j.update_time >= 0.0f) {
            const float dt = now - j.update_time;
            const float alpha = 1.0f - expf(-KH_SHADOW_SMOOTH_RATE * (dt > 0.0f ? dt : 0.0f));
            sf = j.factor + (target - j.factor) * alpha;
        }

        j.factor = sf;
        j.update_time = now;
    }

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& j : jobs) {
            auto it = g_draw_list.find(j.handle);
            if (it == g_draw_list.end()) continue;
            it->second.shadow_factor = j.factor;
            it->second.shadow_update_time = j.update_time;
        }
    }
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
    // cold_t0: first flush whose draw list holds a composite-eligible box
    if (g_mask.cold_t0 < 0.0) {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            if (kv.second.visible && is_composite_eligible(kv.second)) {
                g_mask.cold_t0 = static_cast<double>(effect_time_seconds());
                break;
            }
        }
    }
    g_mask.ov_listed = g_mask.ov_skipped = g_mask.ov_drawn = 0;
    stage_world_lighting();   // game thread: getLighting -> staged sun state (pre-lock)
    update_shadow_rays();     // game thread: per-object sun-occlusion rays (pre-lock)
    ensure_reorder_hook();   // cheap early-out once installed; refreshes the tracked context
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
    const float snapshot_now = effect_time_seconds();
    std::vector<std::pair<uint32_t, RenderObject>> passes;

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            if (!(kv.second.visible && kv.second.fullscreen && kv.second.affect_ui)) continue;
            bool expired = false;
            const float env = lifetime_envelope(kv.second, snapshot_now, expired);
            if (expired) continue;   // the scene flush owns the erasure
            RenderObject o = kv.second;
            o.color[3] *= env;
            passes.emplace_back(kv.first, o);
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

        // Overlay solid boxes need the camera for SolidMask's band term.
        if (o.effect == 0) {
            float ui_cam[3];
            extract_camera_pos(pv.view, ui_cam);
            cbd.fx0[0] = ui_cam[0];
            cbd.fx0[1] = ui_cam[1];
            cbd.fx0[2] = ui_cam[2];
        }

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
    g_draw_list[handle].birth_time = effect_time_seconds();
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

#undef KH_SAFE_RELEASE

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
//   sceneRead: BOOL, shorthand for a tinted scene-read surface
//              (effect "colorgrade" at neutral defaults: scene through the
//              box, tinted by color.rgb, blended by color.a)
//   effect:    STRING or SCALAR - screen-space effect applied inside the
//              box's footprint: "solid" 0, "invert" 1, "colorgrade" 2,
//              "vignette" 3, "chromatic" 4, "grain" 5, "sharpen" 6,
//              "blur" 7, "bloom" 8, "distortion" 9, "outline" 10, "pulse" 11
//   params:    ARRAY of up to 8 numbers, effect-specific (see set_effect_params
//              for meanings and defaults; omitted entries take defaults)
//   band:      [minDist, maxDist, falloff?] - additionally confines the box's
//              effect to a camera-distance band (maxDist <= 0 = unbounded)
//   lit:       (index 10) BOOL, or ARRAY [ambient, diffuse, shadowMode] -
//              shade the box with the world's sun/moon light (getLighting)
//              and attenuate the direct term by per-object sun-occlusion
//              rays (checkVisibility). shadowMode: 0 = no rays, 1 = center
//              ray (default), 2 = center + 8 corners (softer). Defaults
//              ambient 0.4 / diffuse 0.6, which preserves the unlit look
//              in full sun.
// Solid, non-overlay boxes are ALWAYS composited: injected into the frame
// BEFORE the engine draws its translucents, with depth written, so the
// engine itself composites smoke/particles against them pixel-perfectly
// (automatic fallback to the post-scene flush if the draw hook is
// unavailable). Effect and overlay boxes render on the flush path.
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
            obj.effect = static_cast<bool>(arr[4]) ? 2 : 0;   // sceneRead = tinted scene-read (colorgrade defaults)
        }

        const auto_array<game_value>* fx_params = nullptr;

        if (arr.size() > 5 &&
            !(arr[5].type_enum() == game_data_type::STRING && static_cast<std::string>(arr[5]).empty())) {
            // empty string = slot skipped (placeholder to reach later args)
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

        if (arr.size() > 9) {
            if (!RenderIntegration::parse_duration_gv(arr[9], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
        }

        if (arr.size() > 10) {
            if (arr[10].type_enum() == game_data_type::BOOL) {
                obj.lit = static_cast<bool>(arr[10]);
            } else if (arr[10].type_enum() == game_data_type::ARRAY) {
                auto& la = arr[10].to_array();
                obj.lit = true;
                if (la.size() >= 1) obj.light_ambient = static_cast<float>(la[0]);
                if (la.size() >= 2) obj.light_diffuse = static_cast<float>(la[1]);

                if (la.size() >= 3) {
                    const int sm = static_cast<int>(static_cast<float>(la[2]));
                    if (sm < 0 || sm > 2) return game_value("shadowMode must be 0, 1 or 2");
                    obj.shadow_mode = sm;
                }
            }
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
//             "mode" 0..2 | "visible" bool | "sceneread" bool |
//             "effect" string/scalar | "params" array (resets omitted entries
//             to the effect's defaults) | "radius" scalar | "falloff" scalar
//             (radius/falloff apply to addLocalPostFX passes; "position"
//             moves their world-space center) | "band" [minDist, maxDist,
//             falloff?] (camera-distance confinement; [] clears it) |
//             "localsphere" [radius, falloff?] (enables the world-space
//             sphere mask on any object, centered on its position; [] clears) |
//             "lit" bool (sun/moon shading with current lighting params) |
//             "lighting" [ambient, diffuse, shadowMode?] (enables lit and
//             sets the parameters; [] disables)
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
            obj.effect = static_cast<bool>(arr[2]) ? 2 : 0;
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
        } else if (prop == "duration") {
            if (!RenderIntegration::parse_duration_gv(arr[2], obj)) return game_value(false);
            obj.birth_time = RenderIntegration::effect_time_seconds();   // re-arm from now
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
        } else if (prop == "lit") {
            obj.lit = static_cast<bool>(arr[2]);
        } else if (prop == "lighting") {
            if (arr[2].type_enum() != game_data_type::ARRAY) return game_value(false);
            auto& la = arr[2].to_array();

            if (la.size() < 1) {
                obj.lit = false;   // empty array disables the shading
            } else {
                obj.lit = true;
                obj.light_ambient = static_cast<float>(la[0]);
                if (la.size() >= 2) obj.light_diffuse = static_cast<float>(la[1]);

                if (la.size() >= 3) {
                    const int sm = static_cast<int>(static_cast<float>(la[2]));
                    if (sm < 0 || sm > 2) return game_value(false);
                    obj.shadow_mode = sm;
                }
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

        if (arr.size() > 6) {
            if (!RenderIntegration::parse_duration_gv(arr[6], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
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
        auto kvf = [](const char* k, float v) {
            auto_array<game_value> pair;
            pair.push_back(game_value(k));
            pair.push_back(game_value(v));
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
        out.push_back(kv("compositeInjections", RenderIntegration::g_stats.composite_injections));
        out.push_back(kv("compositeBoxes", RenderIntegration::g_stats.composite_boxes));
        out.push_back(kv("compositeSkips", RenderIntegration::g_stats.composite_skips));
        out.push_back(kv("compositeAmbiguous", RenderIntegration::g_stats.composite_ambiguous));
        out.push_back(kv("compositeProjLock", RenderIntegration::g_stats.composite_proj_lock));
        out.push_back(kv("compositeRearms", RenderIntegration::g_stats.composite_rearms));
        out.push_back(kv("compositeRejSpan", RenderIntegration::g_stats.composite_rej_span));
        out.push_back(kv("compositeRejVerify", RenderIntegration::g_stats.composite_rej_verify));
        out.push_back(kv("compositeRejFloor", RenderIntegration::g_stats.composite_rej_floor));
        out.push_back(kv("reorderHook", RenderIntegration::g_reorder_hook_active.load(std::memory_order_acquire) ? 1 : 0));
        out.push_back(kv("uiDriverPolls", RenderIntegration::g_ui_poll_attempts));
        out.push_back(kv("uiDriverCtrl", RenderIntegration::g_ui_ctrl_created ? 1 : 0));
        out.push_back(kv("mainSceneW", RenderIntegration::g_main_depth_w));
        out.push_back(kv("mainSceneH", RenderIntegration::g_main_depth_h));
        out.push_back(kv("shadowLiveLatches", RenderIntegration::g_stats.shadow_live_latches));
        out.push_back(kv("shadowLiveCascades", RenderIntegration::g_stats.shadow_live_cascades));
        out.push_back(kv("shadowSrvFailed", RenderIntegration::g_stats.shadow_srv_failed));
        out.push_back(kv("liveRejOrtho", RenderIntegration::g_stats.live_rej_ortho));
        out.push_back(kv("liveRejScale", RenderIntegration::g_stats.live_rej_scale));
        out.push_back(kv("liveRejIso", RenderIntegration::g_stats.live_rej_iso));
        out.push_back(kv("liveRejRatio", RenderIntegration::g_stats.live_rej_ratio));
        out.push_back(kv("liveRejTrans", RenderIntegration::g_stats.live_rej_trans));
        out.push_back(kv("liveAccepts", RenderIntegration::g_stats.live_accepts));
        out.push_back(kv("shadowAtlasSize", RenderIntegration::g_ls.atlas_size));
        out.push_back(kvf("sunBestDot44", RenderIntegration::g_sr_best_dot44));
        out.push_back(kvf("sunBestDot43", RenderIntegration::g_sr_best_dot43));
        out.push_back(kv("resolveHits", RenderIntegration::g_ls.resolve_hits));
        out.push_back(kv("resolveDraws", RenderIntegration::g_ls.resolve_draws));
        out.push_back(kv("resolveCbFound", RenderIntegration::g_ls.resolve_cb_found));
        out.push_back(kv("bandCaptures", RenderIntegration::g_ls.band_captures));
        out.push_back(kvf("band0Near", RenderIntegration::g_ls.band[0].valid ? RenderIntegration::g_ls.band[0].border[0] : -1.0f));
        out.push_back(kvf("band0Far", RenderIntegration::g_ls.band[0].valid ? RenderIntegration::g_ls.band[0].border[1] : -1.0f));
        out.push_back(kv("band0Copies", RenderIntegration::g_ls.band[0].copies));
        out.push_back(kvf("band1Near", RenderIntegration::g_ls.band[1].valid ? RenderIntegration::g_ls.band[1].border[0] : -1.0f));
        out.push_back(kvf("band1Far", RenderIntegration::g_ls.band[1].valid ? RenderIntegration::g_ls.band[1].border[1] : -1.0f));
        out.push_back(kv("band1Copies", RenderIntegration::g_ls.band[1].copies));
        out.push_back(kv("compCompiles", RenderIntegration::g_comp_compiles));
        out.push_back(kv("compFailStreak", RenderIntegration::g_comp_fail_streak));
        out.push_back(kv("bandBailPv", RenderIntegration::g_ls.band_bail_pv));
        out.push_back(kv("bandBailOff", RenderIntegration::g_ls.band_bail_off));
        out.push_back(kv("bandBailBorder", RenderIntegration::g_ls.band_bail_border));
        out.push_back(kv("bandBailSlot", RenderIntegration::g_ls.band_bail_slot));
        out.push_back(kv("bandBailTime", RenderIntegration::g_ls.band_bail_time));
        out.push_back(kvf("bandRejB0", RenderIntegration::g_ls.band_last_reject[0]));
        out.push_back(kvf("bandRejB1", RenderIntegration::g_ls.band_last_reject[1]));
        out.push_back(kvf("bandRejB2", RenderIntegration::g_ls.band_last_reject[2]));
        out.push_back(kvf("bandRejB3", RenderIntegration::g_ls.band_last_reject[3]));
        out.push_back(kv("castMisses", RenderIntegration::g_ls.cast_misses));
        out.push_back(kv("resolveGated", RenderIntegration::g_ls.resolve_gated));
        out.push_back(kv("analyticCasts", RenderIntegration::g_mask.analytic_casts));
        out.push_back(kv("maskRtvSwaps", RenderIntegration::g_mask.mask_rtv_swaps));
        out.push_back(kv("castBatches", RenderIntegration::g_mask.cast_batches));
        out.push_back(kvf("coldFirstInject", RenderIntegration::g_mask.cold_first_inject));
        out.push_back(kvf("coldFirstCast", RenderIntegration::g_mask.cold_first_cast));
        out.push_back(kv("coldLeadAmbiguous", RenderIntegration::g_mask.cold_lead_ambiguous));
        out.push_back(kvf("coldFirstTrigger", RenderIntegration::g_mask.cold_first_trigger));
        out.push_back(kvf("coldFirstBoxes", RenderIntegration::g_mask.cold_first_stage));
        out.push_back(kv("coldGNoDsv", RenderIntegration::g_mask.cold_g_nodsv));
        out.push_back(kv("coldGFloor", RenderIntegration::g_mask.cold_g_floor));
        out.push_back(kv("coldGSparse", RenderIntegration::g_mask.cold_g_sparse));
        out.push_back(kv("coldCastMiss", RenderIntegration::g_mask.cold_cast_miss));
        out.push_back(kvf("lastInjectNear", RenderIntegration::g_mask.last_inject_near));
        out.push_back(kv("ovListed", RenderIntegration::g_mask.ov_listed));
        out.push_back(kv("ovSkipped", RenderIntegration::g_mask.ov_skipped));
        out.push_back(kv("ovDrawn", RenderIntegration::g_mask.ov_drawn));
        out.push_back(kvf("fogStagedValue", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[0] : -1.0f));
        out.push_back(kvf("fogStagedDecay", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[1] : -1.0f));
        out.push_back(kvf("fogStagedBase", RenderIntegration::g_fog_valid ? RenderIntegration::g_fog[2] : -1.0f));
        out.push_back(kv("sunDirDerivedValid", RenderIntegration::g_sun_dir_derived_valid ? 1u : 0u));
        out.push_back(kvf("sunDirDerivedX", RenderIntegration::g_sun_dir_derived[0]));
        out.push_back(kvf("sunDirDerivedY", RenderIntegration::g_sun_dir_derived[1]));
        out.push_back(kvf("sunDirDerivedZ", RenderIntegration::g_sun_dir_derived[2]));
        out.push_back(kv("maskApplies", RenderIntegration::g_mask.mask_applies));
        out.push_back(kv("bandBailView", RenderIntegration::g_ls.band_bail_view));
        out.push_back(kv("viewLocks", RenderIntegration::g_ls.view_locks));
        out.push_back(kv("viewSrcValid", RenderIntegration::g_ls.view_src_valid ? 1u : 0u));
        out.push_back(kv("frameViewHits", RenderIntegration::g_ls.frame_view_hits));
        out.push_back(kvf("viewBestRot", RenderIntegration::g_ls.view_best_rot));
        out.push_back(kvf("viewBestTrans", RenderIntegration::g_ls.view_best_trans));
        out.push_back(kv("sealCompletions", RenderIntegration::g_ls.seal_completions));
        out.push_back(kv("sunDirValid", RenderIntegration::g_sun_valid ? 1 : 0));
        out.push_back(kvf("sunDirEngineX", RenderIntegration::g_sun_dir_engine[0]));
        out.push_back(kvf("sunDirEngineY", RenderIntegration::g_sun_dir_engine[1]));
        out.push_back(kvf("sunDirEngineZ", RenderIntegration::g_sun_dir_engine[2]));
        out.push_back(kvf("sunBrightness", RenderIntegration::g_sun_brightness));
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

        if (arr.size() > 8) {
            if (!RenderIntegration::parse_duration_gv(arr[8], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
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