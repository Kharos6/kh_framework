#pragma once

#define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

// D3D11.1 context interface: the sky binding probe reads constant-buffer
// OFFSETS via PSGetConstantBuffers1 (the engine binds the scene's
// constants through pool buffers with per-slot offsets - plain
// PSSetConstantBuffers barely appears in its API stream).

namespace RenderIntegration {

enum class DepthMode : int {
    TestOnly = 0,   // default: occluded by scene, depth buffer untouched
    TestWrite = 1,  // participates in depth
    Off = 2,        // overlay
};

static constexpr UINT KH_MAX_QUERY_POINTS = 1024;

// ===========================================================================
// Mesh registry: every drawable 3D object references a mesh by id. Meshes
// are defined in LOCAL ENGINE AXES (x right, y up, z forward), normalized
// to fit [-0.5, 0.5]^3 so that world AABB = center +- sizeAxes * 0.5 stays
// exact for every downstream consumer (cast reach clamp, sort bounds). The
// FBX importer slots in here later: normalize on import, keep the original
// extent as a default size, and draw, lighting, sun-depth casting and
// sorting all work unchanged.
// Vertices carry a position and an outward normal; winding is irrelevant
// (every rasterizer state in this file is CullNone).
// ===========================================================================

struct MeshVertex { float pos[3]; float nrm[3]; };

struct MeshDef {
    const char* name = "";    // primary name (SQF mesh selector)
    const char* alias = "";   // accepted alternative ("" = none)
    std::vector<MeshVertex> verts;
};

static constexpr int KH_MESH_COUNT = 2;   // 0 = "box", 1 = "steps"

namespace meshgen {

// One quad = two triangles with an explicit outward normal. Winding is
// free (CullNone), so corners may arrive in any perimeter order.
inline void quad(std::vector<MeshVertex>& v,
                 const float a[3], const float b[3],
                 const float c[3], const float d[3], const float n[3]) {
    const float* corners[6] = { a, b, c, a, c, d };

    for (int i = 0; i < 6; ++i) {
        MeshVertex mv;
        memcpy(mv.pos, corners[i], sizeof(mv.pos));
        memcpy(mv.nrm, n, sizeof(mv.nrm));
        v.push_back(mv);
    }
}

// Axis-aligned block between mn and mx: 6 quads with exact face normals.
inline void block(std::vector<MeshVertex>& v, const float mn[3], const float mx[3]) {
    const float p000[3] = { mn[0], mn[1], mn[2] };
    const float p001[3] = { mn[0], mn[1], mx[2] };
    const float p010[3] = { mn[0], mx[1], mn[2] };
    const float p011[3] = { mn[0], mx[1], mx[2] };
    const float p100[3] = { mx[0], mn[1], mn[2] };
    const float p101[3] = { mx[0], mn[1], mx[2] };
    const float p110[3] = { mx[0], mx[1], mn[2] };
    const float p111[3] = { mx[0], mx[1], mx[2] };
    const float nxp[3] = { 1.0f, 0.0f, 0.0f }, nxn[3] = { -1.0f, 0.0f, 0.0f };
    const float nyp[3] = { 0.0f, 1.0f, 0.0f }, nyn[3] = { 0.0f, -1.0f, 0.0f };
    const float nzp[3] = { 0.0f, 0.0f, 1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
    quad(v, p100, p110, p111, p101, nxp);   // +x
    quad(v, p000, p001, p011, p010, nxn);   // -x
    quad(v, p010, p011, p111, p110, nyp);   // +y (top)
    quad(v, p000, p100, p101, p001, nyn);   // -y (bottom)
    quad(v, p001, p101, p111, p011, nzp);   // +z
    quad(v, p000, p010, p110, p100, nzn);   // -z
}

}   // namespace meshgen

inline const std::vector<MeshDef>& mesh_registry() {
    static const std::vector<MeshDef> reg = [] {
        std::vector<MeshDef> r(KH_MESH_COUNT);

        // --- mesh 0: "box" - the original unit cube, per-face normals ---
        {
            MeshDef& m = r[0];
            m.name = "box";
            m.alias = "cube";
            const float mn[3] = { -0.5f, -0.5f, -0.5f };
            const float mx[3] = {  0.5f,  0.5f,  0.5f };
            meshgen::block(m.verts, mn, mx);
        }

        // --- mesh 1: "steps" - a 3-step staircase ascending toward +z.
        //     CONCAVE on purpose: with a low sun from the open side each
        //     riser shadows the tread below it - the self-shadowing test
        //     case the private sun-depth map exists for. ---
        {
            MeshDef& m = r[1];
            m.name = "steps";
            m.alias = "test";
            const float t = 1.0f / 3.0f;
            const float z0 = -0.5f, z1 = -0.5f + t, z2 = -0.5f + 2.0f * t, z3 = 0.5f;
            const float y0 = -0.5f, y1 = -0.5f + t, y2 = -0.5f + 2.0f * t, y3 = 0.5f;
            const float nyp[3] = { 0.0f, 1.0f, 0.0f }, nyn[3] = { 0.0f, -1.0f, 0.0f };
            const float nzp[3] = { 0.0f, 0.0f, 1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
            const float nxp[3] = { 1.0f, 0.0f, 0.0f }, nxn[3] = { -1.0f, 0.0f, 0.0f };

            // bottom
            { const float a[3] = { -0.5f, y0, z0 }, b[3] = { 0.5f, y0, z0 }, c[3] = { 0.5f, y0, z3 }, d[3] = { -0.5f, y0, z3 }; meshgen::quad(m.verts, a, b, c, d, nyn); }
            // back (the tall face at z3)
            { const float a[3] = { -0.5f, y0, z3 }, b[3] = { 0.5f, y0, z3 }, c[3] = { 0.5f, y3, z3 }, d[3] = { -0.5f, y3, z3 }; meshgen::quad(m.verts, a, b, c, d, nzp); }
            // risers (facing -z, the open side)
            { const float a[3] = { -0.5f, y0, z0 }, b[3] = { 0.5f, y0, z0 }, c[3] = { 0.5f, y1, z0 }, d[3] = { -0.5f, y1, z0 }; meshgen::quad(m.verts, a, b, c, d, nzn); }
            { const float a[3] = { -0.5f, y1, z1 }, b[3] = { 0.5f, y1, z1 }, c[3] = { 0.5f, y2, z1 }, d[3] = { -0.5f, y2, z1 }; meshgen::quad(m.verts, a, b, c, d, nzn); }
            { const float a[3] = { -0.5f, y2, z2 }, b[3] = { 0.5f, y2, z2 }, c[3] = { 0.5f, y3, z2 }, d[3] = { -0.5f, y3, z2 }; meshgen::quad(m.verts, a, b, c, d, nzn); }
            // treads (facing +y)
            { const float a[3] = { -0.5f, y1, z0 }, b[3] = { 0.5f, y1, z0 }, c[3] = { 0.5f, y1, z1 }, d[3] = { -0.5f, y1, z1 }; meshgen::quad(m.verts, a, b, c, d, nyp); }
            { const float a[3] = { -0.5f, y2, z1 }, b[3] = { 0.5f, y2, z1 }, c[3] = { 0.5f, y2, z2 }, d[3] = { -0.5f, y2, z2 }; meshgen::quad(m.verts, a, b, c, d, nyp); }
            { const float a[3] = { -0.5f, y3, z2 }, b[3] = { 0.5f, y3, z2 }, c[3] = { 0.5f, y3, z3 }, d[3] = { -0.5f, y3, z3 }; meshgen::quad(m.verts, a, b, c, d, nyp); }

            // sides: one column per z segment, floor to that segment's tread
            for (int s = 0; s < 3; ++s) {
                const float za = -0.5f + s * t, zb = -0.5f + (s + 1) * t;
                const float yt = -0.5f + (s + 1) * t;
                { const float a[3] = { 0.5f, y0, za }, b[3] = { 0.5f, y0, zb }, c[3] = { 0.5f, yt, zb }, d[3] = { 0.5f, yt, za }; meshgen::quad(m.verts, a, b, c, d, nxp); }
                { const float a[3] = { -0.5f, y0, za }, b[3] = { -0.5f, y0, zb }, c[3] = { -0.5f, yt, zb }, d[3] = { -0.5f, yt, za }; meshgen::quad(m.verts, a, b, c, d, nxn); }
            }
        }

        return r;
    }();

    return reg;
}

inline UINT mesh_vertex_count(int id) {
    const auto& reg = mesh_registry();
    if (id < 0 || id >= static_cast<int>(reg.size())) id = 0;
    return static_cast<UINT>(reg[id].verts.size());
}

struct Resources {
    // --- Mesh drawing ---
    ID3D11VertexShader*      vs = nullptr;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11VertexShader*      vs_fullscreen = nullptr;   // SV_VertexID fullscreen triangle
    ID3D11PixelShader*       ps_effect = nullptr;       // uber effect shader (meshes + fullscreen)
    UINT                     ps_effect_samples = 0;     // depth MSAA count it was compiled for
    ID3D11VertexShader*      vs_composite = nullptr;    // injected-path VS (adds world position)
    ID3D11PixelShader*       ps_maskcast = nullptr;     // analytic mask cast (single-sample t0)
    ID3D11PixelShader*       ps_composite = nullptr;    // injected-path PS: PSMain + the opaque
                                                        // punch-through guard against the copied
                                                        // scene depth (the HYBRID: hardware depth
                                                        // handles marginal ties, the guard vetoes
                                                        // fragments CLEARLY behind opaque geometry
                                                        // so matrix races can never punch the mesh
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
    ID3D11Buffer*            mesh_vb[KH_MESH_COUNT] = {}; // static registry meshes (pos + normal)
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
    // --- Private single-caster sun-depth map (mesh-shaped cast shadows
    //     + self-shadowing; rendered from the sun by render_sun_depth) ---
    ID3D11Texture2D*          sun_tex = nullptr;
    ID3D11DepthStencilView*   sun_dsv = nullptr;
    ID3D11ShaderResourceView* sun_srv = nullptr;
    ID3D11RasterizerState*    rast_sun = nullptr;   // CullNone, NO scene depth bias
    ID3D11VertexShader*       vs_sundepth = nullptr;     // instanced depth-only transform
    ID3D11GeometryShader*     gs_cascade = nullptr;      // slice router for cascade injection
    ID3D11InputLayout*        layout_sundepth = nullptr; // mesh slot 0 + per-instance slot 1
    ID3D11Buffer*             sun_instance_vb = nullptr; // dynamic per-instance center/extents
    UINT                      sun_instance_cap = 0;      // instances the buffer holds
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
        for (int i = 0; i < KH_MESH_COUNT; ++i) KH_SAFE_RELEASE(mesh_vb[i]);
        KH_SAFE_RELEASE(sun_tex);
        KH_SAFE_RELEASE(sun_dsv);
        KH_SAFE_RELEASE(sun_srv);
        KH_SAFE_RELEASE(rast_sun);
        KH_SAFE_RELEASE(gs_cascade);
        KH_SAFE_RELEASE(vs_sundepth);
        KH_SAFE_RELEASE(layout_sundepth);
        KH_SAFE_RELEASE(sun_instance_vb);
        sun_instance_cap = 0;
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

// Releases every shadow/mask/fire device object living OUTSIDE Resources
// (that state is declared much later in this header, so only the prototype
// can exist this early). Invoked from the reset hook alongside
// g_res.release(), under the same quiescence guarantee.
inline void release_shadow_device_state();

static void __stdcall on_engine_reset() {
    g_res.release();
    release_shadow_device_state();
}

struct RenderObject {
    int   effect = 0;           // 0 = solid color; >0 = screen-space effect (see EffectId)
    float fx[8] = {};           // effect parameters (effect-specific, see set_effect_params)
    bool  fullscreen = false;   // true = fullscreen triangle (post-processing pass), size unused
    bool  affect_ui = false;    // fullscreen passes: true = render post-tonemap over the composited frame (UI included) instead of the 3D scene phase
    bool  localized = false;    // fullscreen pass masked to a world-space sphere around pos
    float local_radius[3] = { 25.0f, 25.0f, 25.0f }; // full-strength radii per SQF axis [x, y, z] (m)
    int   local_shape = 0;      // 0 = sphere/ellipsoid, 1 = cube/mesh mask
    float local_falloff = 10.0f;// fade-to-zero band beyond the radius (m)
    bool  banded = false;       // effect confined to a camera-distance band
    float band_min = 0.0f;      // band start (m); fades in over band_falloff before it
    float band_max = 0.0f;      // band end (m); <= 0 = unbounded (includes sky)
    float band_falloff = 10.0f; // fade width at both band edges (m)
    float pos[3] = {};          // SQF coords [x, y, zASL]; converted at draw
    float size[3] = { 1.0f, 1.0f, 1.0f };  // mesh edge lengths per SQF axis [x, y, z] (m)
    int   blend_mode = 0;       // 0 normal, 1 additive, 2 multiply, 3 screen, 4 lighten, 5 darken
    int   mesh = 0;             // mesh_registry() index (0 = "box"). Local
                                // space is normalized to [-0.5, 0.5]^3,
                                // scaled per axis by 'size', moved to 'pos'.
    float color[4] = { 1, 1, 1, 1 };
    // --- World-lighting interaction (opt-in via 'lit') ---
    // Shaded per pixel against the sun/moon (derived direction + located
    // lighting block) in ApplyLighting; world shadowing is per pixel too
    // (band/live/mask receive + the private sun-depth self term).
    bool  lit = false;
    float light_ambient = 0.40f;   // base-color fraction kept in full shadow
    float light_diffuse = 0.60f;   // N.L-scaled fraction (ambient + diffuse ~ 1
                                   // preserves the unlit look in full sun)
    // Far-visibility (opt-in): clamp clip z inside the engine far plane so
    // the mesh stays drawn beyond max view distance. In-range depth
    // semantics are untouched - the clamp only engages past the far plane,
    // where the depth buffer holds nothing but the clear value. Carried to
    // the shaders through shadowMeta2.x.
    bool  far_vis = false;
    uint64_t seq = 0;           // creation order (fullscreen pass chaining)
    DepthMode mode = DepthMode::TestOnly;
    bool  visible = true;
    // Reordered compositing is ALWAYS ON for solid, non-overlay meshes: they
    // are drawn by the D3D hook immediately BEFORE the engine's first
    // translucent scene draw, with depth written - the ENGINE then
    // composites every particle/translucent against them (smoke in front
    // blends over the mesh, smoke behind is rejected), pixel-perfect. The
    // Draw3D flush covers them only while the hook is unavailable. Notes:
    // composited meshes write depth even in mode 0, and a translucent
    // composited mesh fully occludes engine translucents behind it. Effect
    // meshes and overlays always render on the flush path.
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

// Effect ids shared by meshes (localized, clipped to the mesh's screen footprint)
// and fullscreen passes. 10 and 11 sample the engine depth buffer per pixel.
enum class EffectId : int {
    Solid = 0, Invert = 1, ColorGrade = 2, Vignette = 3, Chromatic = 4,
    Grain = 5, Sharpen = 6, Blur = 7, Bloom = 8, Distortion = 9,
    Outline = 10, Pulse = 11, Halation = 12, Fog = 13,
    LensFlare = 14, Anamorphic = 15, SunFlare = 16, Glitch = 17,
};

static constexpr int KH_MAX_EFFECT = 17;
static std::unordered_map<std::string, RenderObject> g_draw_list;
static std::mutex g_draw_list_mutex;

// STRING UID HANDLES: SQF scalars are 32-bit floats - integer-exact only to
// 2^24 - so a numeric handle eventually collides with itself in a long
// process. Handles are opaque strings from the framework's own UID
// generator, prefixed "khr_" - command errors are plain sentences and never
// carry the prefix, so callers distinguish success by it. g_next_seq is the
// CREATION-ORDER key only (fullscreen passes chain by it; string order is
// lexicographic and useless for that); it never resets, so ordering also
// survives missions.
static uint64_t g_next_seq = 0;

inline std::string make_render_uid() {
    return std::string("khr_") + UIDGenerator::generate();
}

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
    uint64_t composite_meshes = 0;      // meshes drawn through the composited path
    uint64_t composite_skips = 0;      // injections aborted (resources/PV unavailable)
    uint64_t composite_ambiguous = 0;  // frames where the sim republished matrices mid-cycle
                                       // (diagnostic only - the clear-time latch is retained)
    uint64_t composite_proj_lock = 0;  // cycles where the engine's true projection depth
                                       // coefficients were captured and applied
    uint64_t composite_rearms = 0;     // mid-cycle partition changes that re-armed the injection
    uint64_t composite_rej_span = 0;   // triggers rejected: partition does not span the broad middle
    uint64_t composite_rej_verify = 0; // triggers rejected: live DSV verification mismatch
    uint64_t composite_rej_floor = 0;  // triggers rejected: opaque evidence below the floor
    uint64_t composite_slot_encodes = 0;    // injections encoded with the live slot pair (engine-verbatim)
    uint64_t composite_far_phase_skips = 0; // injections withheld while a far partition is the live phase
    uint64_t composite_far_injects = 0;     // far-phase injections that PROCEEDED (all meshes clear of the far near plane)
    uint64_t composite_keep_encodes = 0;    // injections encoded with the PERSISTED slot pair (silent cycle, fresh keep)
    uint64_t composite_anomaly_skips = 0;   // injections withheld from anomalous cycles
    // --- Shadow-pass recon diagnostics (setShadowRecon; see the recon
    //     section for what each number decides) ---
    uint64_t shadow_live_latches = 0;             // live cascade transforms captured (cumulative)
    uint64_t shadow_live_cascades = 0;            // cascade table size in the last completed frame
    uint64_t shadow_srv_failed = 0;               // atlas SRV creation failures
    uint64_t live_rej_ortho = 0;                  // live latch rejections, by filter
    uint64_t live_rej_scale = 0;
    uint64_t live_rej_iso = 0;
    uint64_t live_rej_ratio = 0;
    uint64_t live_rej_trans = 0;
    uint64_t live_accepts = 0;
    // --- Private sun-depth map (mesh-shaped cast + self-shadowing) ---
    uint64_t sun_depth_passes = 0;                // per-frame map renders completed
    uint64_t sun_depth_casters = 0;               // caster draws across all passes
    uint64_t sun_jump_flushes = 0;                // sun-dependent snapshots wiped on a direction jump
    uint64_t enc_vp_rejects = 0;
                                                  // containment - STICKY: any dump shows whether
                                                  // poisoning occurred this session at all
};
static RenderStats g_stats;

// getRenderStats is OPT-IN (operator requirement): the FIRST call ARMS the
// diagnostics - zeroing every pure-diagnostic counter so later dumps are
// session-scoped from the arm - and returns [["status", "armed"]]; real
// stats come from the second call on. Functional counters that drive
// containment (view_locks and its lockWipes pairing, probe hits, streaks)
// are not touched by the arm; they die only with the mission.
static std::atomic<bool> g_stats_armed{ false };

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
    float4 sizeAxes;     // xyz = mesh edge lengths (engine axes), w = blend mode id
    float4 localParams0; // xyz = mask center (engine space), w = shape (0 sphere, 1 cube)
    float4 localParams1; // x = falloff (normalized to mean radius), y = localized flag
    float4 localRadii;   // xyz = mask radii (engine axes)
    float4 bandParams;   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w = banded flag
    float4 lighting0;    // x = lit flag, y = unused, z = ambient, w = diffuse
    float4 lighting1;    // xyz = unit vector TOWARD the sun/moon (engine axes), w = lighting valid flag
    float4 lighting2;    // rgb = light color (max-component normalized), w = shadow-map strength
    float4 shadowMeta;   // x = cascade count, y = depth compare sign, z = bias, w = atlas size (px)
    float4 shadowTiles[8];   // per-cascade atlas rect in UV: x0, y0, x1, y1
    float4 shadowMats[24];   // per-cascade world->atlasUV+depth 4x3: 3 rows of [a, b, c, t]
    float4 shadowMeta2;      // x = far-visibility clamp flag (VSMain/VSComposite);
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
    // Cast pass: draw the meshes depth-only INTO the engine's shadow atlas
    // at each cascade pass end, so the world receives THEIR shadows.
    float4 castMat[3];       // this cascade's sampling matrix rows
    float4 castView[3];      // matching view columns
    float4 castVp;           // atlas-tile viewport: x, y, w, h (pixels)
    float4 castMeta;         // x = atlas size, y = enable
    float4 maskMeta;         // x = enable, y = channel (0-3), z = invert, w = unused
    float4 maskDims;         // xy = mask w/h, zw = viewport w/h at injection
    float4 fogParams;        // x = fogValue, y = fogDecay, z = fogBase, w = enable
    float4 fogColor;         // rgb = fog color, w = camera altitude ASL
    // --- appended (private sun-depth map); append-only: no register shifts ---
    row_major float4x4 sunVP;   // world -> private sun-depth clip (row-vector)
    float4 sunMeta;             // x = valid, y = map size (px), z = compare bias
                                // (normalized depth units), w = strength
    // Per-caster locality list for the map-path cast (append-only): the
    // combined-bounds sphere let reconstruction noise anywhere within
    // hundreds of meters MIN-darken the world; per-caster reach restores
    // the slab era's proven tube-tight rejection. x = pair count.
    float4 localityMeta;
    float4 locality[32];        // [2i] = center.xyz (engine), [2i+1] = half extents.xyz
    float4 lightAmb;            // rgb = engine ambient color (HDR scene units) from the
                                // located lighting block (last-known lanes between
                                // confirmations), or (1,1,1) in the sub-second cold
                                // before the first lock; w = engine-mode flag
    float4 fogEngine;           // THE ENGINE'S OWN FOG TERMS (read from its
                                // disassembly, values from the located block):
                                // x = density scale (lane 41), y = fog end
                                // distance (lane 48), z = inverse ramp range
                                // (lane 49), w = terms valid
    float4 fogSky;              // the sky CB's VIEW-ELEVATION GRADIENT control
                                // points (row 17: down/horizon/zenith - field-
                                // confirmed static (0.25, 0.75, 3.5)); w = valid
    float4 fogSkyCol;           // the sky CB's fog base color (row 7; tracks
                                // time of day - the verbatim fog color)
};

// Shared raw-depth -> view-axis distance (m): inverts the viewport depth
// range remap, then the projection (z' = m22 + m32 / z). At/beyond the far
// plane (sky / depth-clear) the denominator crosses zero: treat as very far.
float KhLinDepth(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    if (denom > -1e-7f) return 1e9f;
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

// Private single-caster sun-depth map: OUR meshes rendered depth-only from
// the sun by render_sun_depth. Two consumers, one truth: (1) SELF-shadowing
// - PSMain/PSComposite fold the factor into the direct term, so concave
// meshes shade their own treads and undersides; (2) MESH-SHAPED cast
// shadows - PSMaskCast tests reconstructed ground points against the same
// map, so the world receives the mesh's true silhouette instead of an AABB
// slab. t11 sits inside StateBackup's saved SRV range (t0-t13).
Texture2D<float> khSunDepth : register(t11);

// Raw occlusion (0 lit .. 1 occluded), pre-strength. Bilinear 4-tap PCF:
// one-texel-soft edges, and the acne band averages instead of flipping.
float z_bias(float z) { return z - sunMeta.z; }

float SunShadowCompareBilin(float2 uv, float z)
{
    float2 tx = uv * sunMeta.y - 0.5f;
    float2 f = frac(tx);
    int2 p0 = int2(tx);
    float o00 = (z > khSunDepth.Load(int3(p0 + int2(0, 0), 0))) ? 1.0f : 0.0f;
    float o10 = (z > khSunDepth.Load(int3(p0 + int2(1, 0), 0))) ? 1.0f : 0.0f;
    float o01 = (z > khSunDepth.Load(int3(p0 + int2(0, 1), 0))) ? 1.0f : 0.0f;
    float o11 = (z > khSunDepth.Load(int3(p0 + int2(1, 1), 0))) ? 1.0f : 0.0f;
    return lerp(lerp(o00, o10, f.x), lerp(o01, o11, f.x), f.y);
}

float SunShadowOcclusion(float3 wpos)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    float4 c = mul(float4(wpos, 1.0f), sunVP);   // ortho: w = 1
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f) return 0.0f;
    if (c.z <= 0.0f || c.z >= 1.0f) return 0.0f;
    return SunShadowCompareBilin(uv, z_bias(c.z));
}

// Soft variant for the SELF term: five bilinear taps in a +/-0.75-texel
// diamond - a ~2.5-texel penumbra, 'very slightly smoothed' rather than
// the bilinear's hard 1-texel ramp. The world CAST keeps the single tap:
)HLSL" R"HLSL(// its edges land on engine-lit ground where the engine's own shadows are
// hard-edged too.
float SunShadowOcclusionPCF(float3 wpos)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    float4 c = mul(float4(wpos, 1.0f), sunVP);   // ortho: w = 1
    float2 uv = float2(0.5f + 0.5f * c.x, 0.5f - 0.5f * c.y);
    if (uv.x <= 0.001f || uv.x >= 0.999f || uv.y <= 0.001f || uv.y >= 0.999f) return 0.0f;
    if (c.z <= 0.0f || c.z >= 1.0f) return 0.0f;
    float z = z_bias(c.z);
    float px = 1.0f / max(sunMeta.y, 1.0f);
    float o = SunShadowCompareBilin(uv, z) * 0.4f;
    o += SunShadowCompareBilin(uv + float2( 0.75f,  0.75f) * px, z) * 0.15f;
    o += SunShadowCompareBilin(uv + float2(-0.75f,  0.75f) * px, z) * 0.15f;
    o += SunShadowCompareBilin(uv + float2( 0.75f, -0.75f) * px, z) * 0.15f;
    o += SunShadowCompareBilin(uv + float2(-0.75f, -0.75f) * px, z) * 0.15f;
    return o;
}

float SunShadowFactor(float3 wpos)
{
    return 1.0f - SunShadowOcclusion(wpos) * saturate(sunMeta.w);
}

// SELF-term variant: receiver-normal offset + grazing fade. A surface
// near-parallel to the sun aliases against the map's texel grid - its own
// depth ramp crosses texel rows in bands, and the compare flips in
// stair-steps (the 'rising bands' at grazing incidence, worst with a
// near-zenith sun on vertical faces). Offsetting the sample point one
// texel along the surface normal moves the compare off the surface's own
// ramp, and the N.L fade retires the term where the direct light is
// heading to zero anyway (so nothing visible is lost). The plain
// SunShadowOcclusion stays for the world CAST (PSMaskCast): the ground is
// not in our map, never compares against itself, and cannot acne.
float SunShadowOcclusionSelf(float3 wpos, float3 nrm)
{
    if (sunMeta.x < 0.5f) return 0.0f;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    if (ndl < 0.02f) return 0.0f;
    // World size of one map texel: the ortho maps 2R of world onto
    // [-1, 1], and column 0 of sunVP has length 1/R.
    float invR = length(float3(sunVP[0].x, sunVP[1].x, sunVP[2].x));
    float texelWorld = 2.0f / (max(sunMeta.y, 1.0f) * max(invR, 1e-6f));
    float occ = SunShadowOcclusionPCF(wpos + n * texelWorld * 1.5f);
    return occ * smoothstep(0.02f, 0.20f, ndl);
}

float SunShadowFactorSelf(float3 wpos, float3 nrm)
{
    return 1.0f - SunShadowOcclusionSelf(wpos, nrm) * saturate(sunMeta.w);
}

// Solid-mesh band / local-volume mask (PSMain and PSComposite): the same
// semantics as the effect-path masks, evaluated on the MESH FRAGMENT's own
// world position (for solid meshes fxParams0.xyz carries the camera).
// Returns an intensity multiplier in [0, 1]; 1 when neither mask is set.
// NOTE: solid meshes previously IGNORED these masks on every path - the
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

// Sun/moon shading for solid meshes (PSMain and PSComposite), opt-in per
// object via lighting0.x. The normal is the mesh's OWN, carried per vertex
// and interpolated: exact on flat faces (no dominant-axis flips at edges
// and corners, which read as per-pixel noise up close - the old analytic
// derivation's failure mode) and it generalizes to any imported geometry.
// N.L on a convex shape is the dominant SELF-shading term; concave
// self-shadowing is the per-pixel sun-depth factor the callers fold into
// smf.
// World shadowing is entirely per pixel.
// smf: per-pixel shadow factor from the caller (received world shadows and
// the private sun-depth self term, min-combined - they answer the same
// question at different granularities and must not stack).
float3 ApplyLighting(float3 base, float3 nrm, float smf)
{
    if (lighting0.x < 0.5f || lighting1.w < 0.5f) return base;
    float3 n = normalize(nrm);
    float ndl = saturate(dot(n, lighting1.xyz));
    float shadow = smf;   // per-pixel receive + self term (min-combined upstream)
    float3 direct = lighting2.rgb * (ndl * lighting0.w * shadow);
    // Ambient: lightAmb carries the located block's HDR ambient color
    // (moonlight included - the engine zeroes the sun lane at night and
    // routes the moon here), or (1,1,1) in the sub-second cold before
    // the first lock so lighting0.z alone shades.
    return base * (lightAmb.rgb * lighting0.z + direct);
}

struct VSIn  { float3 pos : POSITION; float3 nrm : NORMAL; };
struct VSOut { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1; };
)HLSL";

// Static entry points (no depth access): compiled once
static const char* g_hlsl_static = R"HLSL(
VSOut VSMain(VSIn i)
{
    VSOut o;
    float3 wp = centerSize.xyz + i.pos * sizeAxes.xyz;
    o.pos = mul(float4(wp, 1.0f), viewProj);
    // Far-visibility clamp (opt-in, shadowMeta2.x): pull clip z just inside
    // the far plane so max view distance stops hard-clipping the mesh. Past
    // the far plane the depth buffer holds only the clear value / sky, so
    // ordering cannot break; the guard compares SV_Position.w, untouched.
    if (shadowMeta2.x > 0.5f && o.pos.z > o.pos.w * 0.9999f) o.pos.z = o.pos.w * 0.9999f;
    o.wpos = wp;
    // Per-axis scale is non-uniform: normals transform by the inverse
    // scale. (Rotation arrives with the FBX model matrix later; add its
    // inverse-transpose here when it does.)
    o.nrm = normalize(i.nrm / max(sizeAxes.xyz, float3(1e-4f, 1e-4f, 1e-4f)));
    return o;
}

// Instanced depth-only transform for the private sun-depth pass: ONE
// DrawInstanced per mesh type regardless of object count - the per-object
// constant uploads (and with them the caster cap) are gone. Slot-1
// per-instance data: TEXCOORD4 = engine-space center, TEXCOORD5 =
// engine-space extents.
struct VSInSun {
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float4 ipos : TEXCOORD4;
    float4 isize : TEXCOORD5;
};

float4 VSSunDepth(VSInSun i) : SV_Position
{
    float3 wp = i.ipos.xyz + i.pos * i.isize.xyz;
    float4 cp = mul(float4(wp, 1.0f), viewProj);

    // Cascade tile remap (localityMeta.w = 1, cascade injection only):
    // the engine places tiles INSIDE its VS - clip.xy scaled into the
    // tile's sub-rect. localityMeta = (scale, offsetX, offsetY, flag).
    // The sun-map pass never sets the flag; its path is unchanged.
    if (localityMeta.w > 0.5f) {
        cp.x = cp.x * localityMeta.x + localityMeta.y * cp.w;
        cp.y = cp.y * localityMeta.x + localityMeta.z * cp.w;
    }

    return cp;
}

// Cascade-slice router: the engine's shadow atlas is an ARRAY target with
// per-draw slice routing (field census: hundreds of cascade draws share
// one DSV pointer and one viewport corner). A plain VS pipeline lands in
// slice 0 only; this pass-through GS routes our triangles to the tile the
// injected matrix belongs to. localityMeta.y carries the slice index.
struct GSSliceIn  { float4 pos : SV_Position; };
struct GSSliceOut { float4 pos : SV_Position; uint rt : SV_RenderTargetArrayIndex; uint vp : SV_ViewportArrayIndex; };

[maxvertexcount(3)]
void GSCascadeSlice(triangle GSSliceIn i[3], inout TriangleStream<GSSliceOut> s)
{
    GSSliceOut o;
    o.rt = 0;                          // non-array DSV: RT index is a no-op
    o.vp = (uint)localityMeta.y;       // VIEWPORT ARRAY routing - the tile scheme
    o.pos = i[0].pos;
    s.Append(o);
    o.pos = i[1].pos;
    s.Append(o);
    o.pos = i[2].pos;
    s.Append(o);
}

// Analytic mask cast: per-pixel ray-vs-AABB toward the sun, drawn into
// the engine's screen-space shadow mask with multiply blending.
// castMat[0..2] = view rows 0..2; castView[0] = view row 3 (translation);
// castView[1] = (fov.x, fov.y, maskW, maskH); castView[2] = sunDir.xyz +
// strength in w. Scene linear depth at t0.
Texture2D<float4> sceneDepthTex : register(t0);
float KhSceneLoad(int2 p) { return sceneDepthTex.Load(int3(p, 0)).x; }

)HLSL" R"HLSL(float4 PSMaskCast(VSOut i) : SV_Target
{
    // t0 is the engine's own linearized scene depth, registered against
    // the published view + the sweep's fov/dims (frozen at the frame's
    // first fire).
    // RESOLUTION-AGNOSTIC depth addressing (the half-res lesson): the
    // engine's resolve samples its depth by NORMALIZED UV; this shader
    // Loaded by the MASK's pixel coordinates - correct while mask and
    // depth share a resolution, but the fog-economy HALF-RES mask made
    // the Loads touch only the depth's top-left quadrant: garbage
    // reconstruction, angle-flickering overcast, no coherent cast, all
    // healing the moment the engine returned to full res. Scale mask
    // pixels into the depth texture's own grid.
    // .y RETRIAL, and the prior falsification is FORMALLY WITHDRAWN as
    // confounded: the 70% overcast trial ran under the quadrant-sampling
    // bug - every read was garbage regardless of channel, so the
    // channels were never fairly tested. The field then produced the .x
    // signature of a CONSTANT z BIAS (drift growing as fragments near,
    // fine at range - relative error c/z), the mark of a near-offset
    // encoding, while the engine's own resolve reads .y. With the
    // addressing fixed, .y gets its clean trial; .x remains the
    // single-channel fallback.
    float2 dimsM = float2(castView[1].z, castView[1].w);
    uint dw, dh;
    sceneDepthTex.GetDimensions(dw, dh);
    int2 px = int2(i.pos.xy * float2(dw, dh) / max(dimsM, float2(1.0f, 1.0f)));
    float4 zt = sceneDepthTex.Load(int3(px, 0));
    float zl = zt.y > 0.0f ? zt.y : zt.x;

    // PER-BAND write confinement - castMeta.y/z carry the batch's own
    float2 dims = dimsM;
    float2 ndc = float2(i.pos.x / dims.x * 2.0f - 1.0f, 1.0f - i.pos.y / dims.y * 2.0f);
    float3 vp = float3(ndc.x * castView[1].x, ndc.y * castView[1].y, 1.0f) * zl;
    float3 q = vp - castView[0].xyz;
    float3 pw;
    pw.x = q.x * castMat[0].x + q.y * castMat[0].y + q.z * castMat[0].z;
    pw.y = q.x * castMat[1].x + q.y * castMat[1].y + q.z * castMat[1].z;
    pw.z = q.x * castMat[2].x + q.y * castMat[2].y + q.z * castMat[2].z;
    float hit = 0.0f;

    if (sunMeta.x >= 0.5f) {
        // MESH-SHAPED cast: the reconstructed point is tested against the
        // private sun-depth map, so the world receives the mesh's TRUE
        // silhouette (risers, notches, future FBX geometry) instead of an
        // AABB slab. centerSize/sizeAxes carry the COMBINED caster bounds
        // on this path, so the field-proven locality clamp below keeps
        // rejecting the garbage world positions that far-plane pixels
        // reconstruct (the 'overcast world' lesson).
        // PER-CASTER locality (the overcast attribution's fix): a ground
        // point may only darken within plausible shadow reach of SOME
        // caster - the same tube-tight test the slab era field-proved,
        // now per entry of the locality list. The combined-bounds sphere
        // (kept as the >16-caster fallback, 600 m ceiling) let any
        // reconstruction noise within hundreds of meters MIN-darken the
        // world wholesale.
        bool near_ok = false;
        float stretch = 2.0f + 3.0f / max(abs(castView[2].y), 0.15f);

        if (localityMeta.x >= 0.5f) {
            int lc = (int)localityMeta.x;

            [loop] for (int li = 0; li < lc && !near_ok; ++li) {
                float3 lce = locality[li * 2].xyz;
                float3 lhe = locality[li * 2 + 1].xyz;
                float lr = min(length(lhe) * stretch, 600.0f);
                float3 ld = lce - pw;
                if (dot(ld, ld) < lr * lr) near_ok = true;
            }
        } else {
            float castR = length(sizeAxes.xyz) * 0.5f;
            float reach = min(castR * stretch, 600.0f);
            float3 toCast = centerSize.xyz - pw;
            near_ok = dot(toCast, toCast) < reach * reach;
        }

        // zl floor 1.2 m: if the captured depth texture transiently holds
        // aliased non-depth content (normalized values <= 1), EVERY pixel
        // reconstructs to ~the camera position - inside reach whenever
        // the player stands near a caster, MIN-darkening the whole screen
        // (the era-independent overcast mode). Real scene depth below
        // 1.2 m is only the player's own feet.
        if (near_ok && zl > 1.2f) hit = SunShadowOcclusion(pw);
    } else {
        // AABB slab fallback (the pre-map analytic cast, kept verbatim):
        // ray toward the sun against the object's bounds, for frames where
        // the sun-depth pass could not run. AABB-shaped by construction.
        float3 sd = castView[2].xyz;
        float3 sds = float3(
            abs(sd.x) > 1e-6f ? sd.x : 1e-6f,
            abs(sd.y) > 1e-6f ? sd.y : 1e-6f,
)HLSL" R"HLSL(            abs(sd.z) > 1e-6f ? sd.z : 1e-6f);
        float3 inv = 1.0f / sds;
        float3 bmin = centerSize.xyz - sizeAxes.xyz * 0.5f;
        float3 bmax = centerSize.xyz + sizeAxes.xyz * 0.5f;
        float3 ta = (bmin - pw) * inv;
        float3 tb = (bmax - pw) * inv;
        float3 tmn = min(ta, tb);
        float3 tmx = max(ta, tb);
        float tn = max(max(tmn.x, tmn.y), tmn.z);
        float tf = min(min(tmx.x, tmx.y), tmx.z);

        // LOCALITY CLAMP: a ground point can only be shadowed by a mesh
        // within plausible shadow reach of it. Meshes beyond the captured
        // depth's valid range reconstruct GARBAGE world positions at
        // far-plane pixels; the slab test then hit pseudo-randomly across
        // the whole screen and MIN-darkened the world's mask wholesale
        // (the 'overcast world' when the distant fog array spawned).
        // Reach = a few diagonals stretched by sun elevation (low sun =
        // long shadows); beyond it, geometrically impossible - never
        // darken.
        float castR = length(sizeAxes.xyz) * 0.5f;
        float reach = castR * (2.0f + 3.0f / max(abs(castView[2].y), 0.15f));
        float3 toCast = centerSize.xyz - pw;
        float near_enough = (dot(toCast, toCast) < reach * reach) ? 1.0f : 0.0f;
        hit = (tf >= max(tn, 0.05f) && zl > 1.2f && near_enough > 0.5f) ? 1.0f : 0.0f;   // zl floor: see the map branch

        // Screen path only (castMeta.x = 1): if the CAMERA's ray to this
        // pixel passes through the mesh bounds first, the pixel shows the
        // mesh face - the scene-end apply runs after the meshes draw, so
        // shading it would paint the ground's shadow ONTO the mesh (the
        // see-through bug).
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
            if (vtf >= max(vtn, 0.0f) && vtn < 0.99f) hit = 0.0f;   // mesh face in front
        }
    }

    // Full occlusion here; the APPLY colors the shadow by the ambient
    // floor (amb/(amb+sun)) - depth and hue both derive from the sky.
    float shade = 1.0f - hit * saturate(castView[2].w);
    return float4(shade, shade, shade, 1.0f);
}

// Fullscreen triangle from SV_VertexID: no vertex buffer, no input layout
VSOut VSFullscreen(uint vid : SV_VertexID)
{
    VSOut o;
    float2 uv = float2((vid << 1) & 2, vid & 2);
    o.pos = float4(uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    o.wpos = float3(0.0f, 0.0f, 0.0f);
    o.nrm = float3(0.0f, 1.0f, 0.0f);
    return o;
}

float4 PSMain(VSOut i) : SV_Target
{
    // Punch-through / overlay-occlusion guard, flush-path edition: the
    // same contract as PSComposite's. t0 (sceneDepthTex) holds the
    // mid-frame scene-depth snapshot when the CPU armed tight margins in
    // fxParams1; 1e9 margins stand the guard down entirely - draw
    // unoccluded, never vanish (the state-leak ledger rule). The CPU arms
    // tight margins only for SINGLE-SAMPLE snapshots, matching this
    // texture's declaration, so MSAA frames stand down by construction.
    if (fxParams1.x < 1e8f) {
        int2 gpx = clamp(int2(i.pos.xy), int2(0, 0), int2((int)fxMeta.z - 1, (int)fxMeta.w - 1));
        float sceneZ = KhLinDepth(KhSceneLoad(gpx));
        if (i.pos.w > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;
    }

    int bm = (int)sizeAxes.w;
    // Intensity (color.a, which carries the lifetime envelope) applies to
    // every blend mode; the band/local mask folds into it. Lighten/darken
    // cannot express intensity through hardware blend factors (MAX/MIN
    // ignore alpha), so they fade toward their respective neutral element
    // instead: black for MAX, a large value for MIN - both leave the
    // scene untouched.
    // Self/cast term from the private sun-depth map: this path has no
    // atlas access, but the sun map is ours and path-agnostic. N.L
    // early-out: no direct term, no shadow samples.
    float smf = (lighting0.x >= 0.5f && dot(i.nrm, lighting1.xyz) > 0.01f)
              ? SunShadowFactorSelf(i.wpos, i.nrm) : 1.0f;
    float3 lc = ApplyLighting(color.rgb, i.nrm, smf);

    // Fog parity with the composite path (this shader serves the
    // fallback draw on injection-miss frames): the engine's verbatim
    // transmittance, constant color target (no scene capture here).
    if (fogParams.w >= 0.5f) {
        float distM = i.pos.w;
        float hgt = i.wpos.y;
        float camY = fogColor.w;
        float trans;

        if (fogEngine.w >= 0.5f) {
            float ramp = saturate((fogEngine.y - distM) * fogEngine.z);
            float dh = abs(hgt - camY);
            float k = fogParams.y * dh / max(distM, 1.0e-4f);
            float integ = k < 1.0e-6f ? distM : (1.0f - exp(-distM * k)) / k;
            float minY = min(hgt, camY);
            trans = ramp * exp(-integ * fogEngine.x * exp(-fogParams.y * max(minY, 0.0f)));
        } else {
            float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
            trans = exp(-distM * dens * 0.0153f);
        }

        float3 fog_target = fogColor.rgb;

        if (fogSky.w >= 0.5f) {
            float dirY = (hgt - camY) / max(distM, 1.0e-4f);
            float g;

            if (dirY < 0.0f) {
                float u = dirY + 1.0f;
                g = u * u * (fogSky.y - fogSky.x) + fogSky.x;
            } else {
                g = dirY * (fogSky.z - fogSky.y) + fogSky.y;
            }

            fog_target = fogSkyCol.rgb * g;
        }

        lc = lerp(fog_target, lc, trans);
    }

    float a = color.a * SolidMask(i.wpos);
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);
    return float4(lc, a);
}
)HLSL";

// Injected-path shaders (the HYBRID): identical mesh transform and blend
// packing, plus a per-pixel punch-through guard. The fragment's TRUE
// camera distance (world-space, matrix-independent) is compared against
// the scene distance reconstructed from a mid-frame copy of the depth
// buffer; a fragment CLEARLY behind opaque geometry - beyond a margin
// generous enough to absorb any near-plane/coefficient uncertainty - is
// discarded. Hardware depth (with the shimmer bias) still resolves the
// marginal ties; the guard only vetoes the gross violations that show up
// as the mesh punching through walls. Compiled per depth MSAA count.
static const char* g_hlsl_composite = R"HLSL(
#if MSAA_DEPTH
Texture2DMS<float> depthTex : register(t0);
float LoadDepthC(int2 px) { return depthTex.Load(px, 0); }
// Guard-side scene distance: the MOST PERMISSIVE (farthest) across the
// pixel's samples. The guard runs at PIXEL frequency while the hardware
// depth test runs at SAMPLE frequency: on silhouette pixels sample 0 can
// hold a NEAR foreign surface while the fragment legitimately covers the
// FAR samples, and a sample-0 guard then discards the whole pixel -
// rasterization jitter turns that into the close-up edge sparkle. Taking
// the farthest sample keeps the anti-punch-through duty intact (interior
// pixels agree across samples) and removes the false edge vetoes.
float GuardSceneDist(int2 px)
{
    // CLEAR-VALUE = NO SCENE (the see-through root): a narrow render
    // partition's depth copy holds the clear value at every pixel the
    // partition did not draw - which linearizes not to 'infinitely far'
    // but to THAT PARTITION'S far plane, a phantom wall that discarded
    // every mesh fragment beyond it on partition-churn cycles. Raw
    // depth at either clear extreme (standard far = 1, reversed = 0)
    // can never discard. Both extremes are safe to skip: nothing
    // legitimate needs occluding at the clear planes themselves.
    float m = 0.0f;

    [unroll] for (int s = 0; s < SAMPLE_COUNT; ++s) {
        float r = depthTex.Load(px, s);
        if (r > 0.000001f && r < 0.999999f) m = max(m, KhLinDepth(r));
    }

    return m > 0.0f ? m : 1.0e9f;
}
#else
Texture2D<float> depthTex : register(t0);
float LoadDepthC(int2 px) { return depthTex.Load(int3(px, 0)); }
float GuardSceneDist(int2 px)
{
    // clear-value = no scene: see the MSAA variant's note
    float r = depthTex.Load(int3(px, 0));
    return (r <= 0.000001f || r >= 0.999999f) ? 1.0e9f : KhLinDepth(r);
}
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
Texture2D<float> shadowBand6 : register(t12);   // slots 6-7; t10 is free
Texture2D<float> shadowBand7 : register(t13);   // (convergence retired)

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

        // TILE-INTERIOR CLAMP (the circle-with-dot artifact): the atlas
        // holds more than sun cascades - round proxy-blob tiles among
        // them - and taps near the cascade tile's border read across
        // into neighbors. Every tap stays one texel inside the rect.
        // 2x2 BILINEAR PCF (the 'slightly smoothed' checklist row): four
        // compares blended by the sample's subtexel position replace the
        // hard one-texel step - the stipple at grazing incidence softens
        // into the same ~1-texel ramp the self term uses.
        float px1 = 1.0f / shadowMeta.w;
        float2 uv = clamp(float2(u, v), t.xy + px1, t.zw - px1);
        float2 fpx = uv * shadowMeta.w - 0.5f;
        int2 p0 = int2(floor(fpx));
        float2 fr = frac(fpx);
        bool fine = (c == (int)shadowSrc.x);
        float d00 = fine ? shadowAtlasFine.Load(int3(p0, 0)) : shadowAtlas.Load(int3(p0, 0));
        float d10 = fine ? shadowAtlasFine.Load(int3(p0 + int2(1, 0), 0)) : shadowAtlas.Load(int3(p0 + int2(1, 0), 0));
        float d01 = fine ? shadowAtlasFine.Load(int3(p0 + int2(0, 1), 0)) : shadowAtlas.Load(int3(p0 + int2(0, 1), 0));
        float d11 = fine ? shadowAtlasFine.Load(int3(p0 + int2(1, 1), 0)) : shadowAtlas.Load(int3(p0 + int2(1, 1), 0));
        float o00 = ((z - d00) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o10 = ((z - d10) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o01 = ((z - d01) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        float o11 = ((z - d11) * shadowMeta.y > shadowMeta.z) ? 1.0f : 0.0f;
        cascade = c;
        occluded = lerp(lerp(o00, o10, fr.x), lerp(o01, o11, fr.x), fr.y);
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
float BandLoad(int t, int2 px)
{
    return (t == 0) ? shadowBand0.Load(int3(px, 0))
         : (t == 1) ? shadowBand1.Load(int3(px, 0))
         : (t == 2) ? shadowBand2.Load(int3(px, 0))
         : (t == 3) ? shadowBand3.Load(int3(px, 0))
         : (t == 4) ? shadowBand4.Load(int3(px, 0))
         : (t == 5) ? shadowBand5.Load(int3(px, 0))
         : (t == 6) ? shadowBand6.Load(int3(px, 0))
                    : shadowBand7.Load(int3(px, 0));
}

// BILINEAR COMPARE per tap - the engine's sample_c equivalent (the
// quality verdict from the path tint: the band path owns 100% of the
// shading, and its POINT taps printed the shadow map's dithered foliage
// raw - the stipple, and each dither cluster as a circle-with-dot. A
)HLSL" R"HLSL(// 2x2 weighted compare resolves every dither cell to its smooth
// coverage fraction, exactly like the engine's 16-tap sample_c tier).
float BandCmpBilin(int t, float2 pos, float z)
{
    // ZERO shader-side bias, per PS-17: the engine's sample_c compares
    // the projected depth RAW - its bias is baked into the shadow-map
    // RENDER (slope-scaled at rasterization), and these seals are
    // copies of that same pre-biased content. The shadowMeta.z bias on
    // top was DOUBLE-biasing: washed contact shadows, lightened thin
    // occluders. (shadowMeta.z remains in the LIVE-atlas path, whose
    // transforms are harvested rather than engine-published and need
    // the registration slack.)
    float2 f = pos - 0.5f;
    int2 p0 = int2(floor(f));
    float2 fr = frac(f);
    float b00 = ((z - BandLoad(t, p0)) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b10 = ((z - BandLoad(t, p0 + int2(1, 0))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b01 = ((z - BandLoad(t, p0 + int2(0, 1))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    float b11 = ((z - BandLoad(t, p0 + int2(1, 1))) * shadowMeta.y > 0.0f) ? 1.0f : 0.0f;
    return lerp(lerp(b00, b10, fr.x), lerp(b01, b11, fr.x), fr.y);
}

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
                    // PS-17 VERBATIM, the full recipe (the 8-tap set was
                    // a SUBSET of the engine's own table - offsets 1-7
                    // plus 16): sixteen poisson offsets, adaptive
                    // early-out after four (the extra twelve run only in
                    // the indecisive penumbra - exactly where the
                    // dithered foliage lives), bilinear compare per tap
                    // (sample_c equivalent), per-pixel disk rotation.
                    float ang = frac(sin(dot(wpos.xz, float2(12.9898f, 78.233f))) * 43758.5469f) * 6.2831853f;
                    float ca = cos(ang);
                    float sa = sin(ang);
                    // ENGINE-EXACT RADIUS (PS-17 disassembly closes the
                    // last named deviation): 'mul r0.y, l(1.3), cb0[6].y'
                    // where cb0 = PSCB_NonFrequent and register 6 is
                    // PSC_SBTSize_invSBTSize - the engine's radius is
                    // 1.3 x invSBTSize in UV, i.e. 1.3 TEXELS of the very
                    // atlas these seals copy. This loop works in texel
                    // space against the same atlas size, so the constant
                    // transfers directly; the former 3.0 was a by-eye tune
                    // from before the constant was decoded.
                    float r = 1.3f;
                    float2 base = float2(u, v) * shadowMeta.w;
                    float acc = 0.0f;

                    for (int k = 0; k < 4; ++k) {
                        float2 d0 =
                            (k == 0) ? float2( 0.974844f, 0.756484f)
                          : (k == 1) ? float2(-0.814100f, 0.914376f)
                          : (k == 2) ? float2( 0.945586f,-0.768907f)
                                     : float2(-0.815442f,-0.879125f);
                        float2 off = float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca);
                        acc += BandCmpBilin(t, base + off * r, z);
                    }

                    if (acc >= 3.999f || acc <= 0.001f) {
                        occ = acc >= 3.999f ? 1.0f : 0.0f;   // decisive: engine's early-out
                    } else {
                        for (int k2 = 0; k2 < 12; ++k2) {
                            float2 d0 =
                                (k2 ==  0) ? float2( 0.443233f,-0.975116f)
                              : (k2 ==  1) ? float2(-0.241888f, 0.997065f)
                              : (k2 ==  2) ? float2(-0.915886f, 0.457714f)
                              : (k2 ==  3) ? float2(-0.942016f,-0.399062f)
                              : (k2 ==  4) ? float2(-0.094184f,-0.929389f)
                              : (k2 ==  5) ? float2( 0.791975f, 0.190902f)
                              : (k2 ==  6) ? float2( 0.199841f, 0.786414f)
                              : (k2 ==  7) ? float2( 0.537430f,-0.473734f)
                              : (k2 ==  8) ? float2(-0.264969f,-0.418930f)
                              : (k2 ==  9) ? float2(-0.382775f, 0.276768f)
                              : (k2 == 10) ? float2( 0.344959f, 0.293878f)
                                           : float2( 0.143832f,-0.141008f);
                            float2 off = float2(d0.x * ca - d0.y * sa, d0.x * sa + d0.y * ca);
                            acc += BandCmpBilin(t, base + off * r, z);
                        }

                        occ = acc * 0.0625f;   // /16, the engine's own weight
                    }

                    done = 1;
                }
            }
        }
    }

    if (occ < 0.0f) return 1.0f;   // no band covers this depth: lit
    return 1.0f - occ * saturate(lighting2.w);
}

)HLSL";

// (split: MSVC C2026 caps a single literal at ~16 KB - the fog additions
// crossed it; the two halves are joined at the compile site)
static const char* g_hlsl_composite2 = R"HLSL(
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

float LinDepthC(float raw)
{
    float ndcZ = (raw - depthParams.z) / max(depthParams.w - depthParams.z, 1e-6f);
    float denom = ndcZ - depthParams.x;
    if (denom > -1e-7f) return 1e9f;   // at/beyond far plane (sky/clear): very far
    float d = depthParams.y / denom;
    return d > 0.0f ? d : 1e9f;
}

struct VSOutC { float4 pos : SV_Position; float3 wpos : TEXCOORD0; float3 nrm : TEXCOORD1; };

VSOutC VSComposite(VSIn i)
{
    VSOutC o;
    float3 wp = centerSize.xyz + i.pos * sizeAxes.xyz;
    o.pos = mul(float4(wp, 1.0f), viewProj);
    // Far-visibility clamp (opt-in, shadowMeta2.x): pull clip z just inside
    // the far plane so max view distance stops hard-clipping the mesh. Past
    // the far plane the depth buffer holds only the clear value / sky, so
    // ordering cannot break; the guard compares SV_Position.w, untouched.
    if (shadowMeta2.x > 0.5f && o.pos.z > o.pos.w * 0.9999f) o.pos.z = o.pos.w * 0.9999f;
    o.wpos = wp;
    // Inverse per-axis scale for the normal (see VSMain).
    o.nrm = normalize(i.nrm / max(sizeAxes.xyz, float3(1e-4f, 1e-4f, 1e-4f)));
    return o;
}

)HLSL" R"HLSL(float4 PSComposite(VSOutC i) : SV_Target
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
    float sceneZ = GuardSceneDist(px);
    float fragZ = i.pos.w;
    if (fragZ > sceneZ * (1.0f + fxParams1.y) + fxParams1.x) discard;

    int bm = (int)sizeAxes.w;

    float smf = 1.0f;

    // N.L early-out: a face pointing away from (or grazing) the light has
    // no direct term for shadows to attenuate - skip the band scan, the
    // PCF taps and the sun-map samples outright. On typical meshes this
    // halves the shadow work.
    if (lighting0.x >= 0.5f && dot(i.nrm, lighting1.xyz) > 0.01f) {
        if (maskMeta.x >= 0.5f)      smf = ShadowMaskFactor(i.pos);
        else if (maskMeta.w >= 0.5f) smf = ShadowBandFactor(i.wpos);
        else                         smf = ShadowMapFactor(i.wpos - shadowMeta2.yzw);

        // Concave SELF-shadowing from the private sun-depth map: min with
        // the received term - same question, different occluders, and the
        // darker answer wins without stacking.
        smf = min(smf, SunShadowFactorSelf(i.wpos, i.nrm));
    }
    float3 lc = ApplyLighting(color.rgb, i.nrm, smf);

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
    // Extinction LINEAR in density (lambda = dens * 0.0153 / m). The
    // original dens^2 fit threaded two endpoint measurements (fog 1.0 ~
    // 90% at 150 m; a fog-0.5 note now superseded), but a quadratic
    // through those points collapses at MID values: at fog 0.37 it
    // predicted 90% obscured at ~1.1 km while engine rocks at a few
    // hundred meters were already swallowed and the mesh behind them
    // stood half-clear (the under-fogged slab screenshot). Linear keeps
    // the fog-1.0 endpoint and matches the terrain rate at mid values -
    // verified by direct engine-geometry comparison, the only calibration
    // source that counts.
    if (fogParams.w >= 0.5f) {
        float distM = i.pos.w;
        float hgt = i.wpos.y;
        float camY = fogColor.w;
        // THE ENGINE'S OWN TRANSMITTANCE, verbatim from its terrain PS
        // disassembly: a LINEAR distance ramp (fog end + inverse range -
        // the block's 'mystery pair', finally named) times the analytic
        // HEIGHT-FOG integral (density scale = lane 41, reference = the
        // ray's LOWER endpoint - the lane's 'inconsistency' across
        // sessions was this reference, not a broken formula). The
        // exponential-lambda era and its spectral fog approximation are
        // retired: the engine's spectral vectors extinguish LIGHTING,
        // not fog.
        float trans;

        if (fogEngine.w >= 0.5f) {
            float ramp = saturate((fogEngine.y - distM) * fogEngine.z);
            float dh = abs(hgt - camY);
            float k = fogParams.y * dh / max(distM, 1.0e-4f);
            float integ = k < 1.0e-6f ? distM : (1.0f - exp(-distM * k)) / k;
            float minY = min(hgt, camY);
            float th = exp(-integ * fogEngine.x * exp(-fogParams.y * max(minY, 0.0f)));
            trans = ramp * th;
        } else {
            // block not locked (sub-second cold): the legacy exponential
            float dens = fogParams.x * exp(-fogParams.y * max(hgt - fogParams.z, 0.0f));
            trans = exp(-distM * dens * 0.0153f);
        }

        // VERBATIM FOG COLOR (the sky CB, located and field-confirmed):
        // base color row 7 times the view-elevation gradient of row 17 -
        // two curves picked by the ray's vertical sign, exactly the
        // engine's own instructions. Field values (0.25, 0.75, 3.5):
        // dim looking down, bright toward zenith - the warm-over-sand /
        // blue-against-sky behavior no constant could produce. The
        // convergence capture and its isolation weighting retire here:
        // scaffolding that served until the real source was located.
        float3 fog_target = fogColor.rgb;

        if (fogSky.w >= 0.5f) {
            float dirY = (hgt - camY) / max(distM, 1.0e-4f);
            float g;

            if (dirY < 0.0f) {
                float u = dirY + 1.0f;
                g = u * u * (fogSky.y - fogSky.x) + fogSky.x;
            } else {
                g = dirY * (fogSky.z - fogSky.y) + fogSky.y;
            }

            fog_target = fogSkyCol.rgb * g;
        }

        // (The scene-convergence override is DELETED, finally and by the
        // operator's explicit verdict: it produced correct color by
        // construction but reads as transparency - the mesh dissolving
        // into its literal backdrop image. The gradient above IS the
        // shader-based fog this work built toward, now running on a
        // permanently live mirror.)
        lc = lerp(fog_target, lc, trans);
    }
    float a = color.a * SolidMask(i.wpos);
    if (bm == 1 || bm == 3) return float4(lc * a, 1.0f);
    if (bm == 2) return float4(lerp(float3(1.0f, 1.0f, 1.0f), lc, a), 1.0f);
    if (bm == 4) return float4(lc * a, 1.0f);
)HLSL" R"HLSL(    if (bm == 5) return float4(lerp(float3(65504.0f, 65504.0f, 65504.0f), lc, a), 1.0f);
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
        // normalized distance: 1.0 = the mask surface (ellipsoid or mesh)
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
)HLSL" R"HLSL(        else              comp = mixed;                                              // normal
        return float4(comp, 1.0f);
    }

    // Blend-mode output packing (meshes: hardware blend against the live
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
    float size_axes[4];    // xyz = mesh edge lengths (engine axes), w = blend mode id
    float local0[4];       // xyz = mask center (engine space), w = shape (0 sphere, 1 cube)
    float local1[4];       // x = falloff (normalized to mean radius), y = localized flag
    float local_radii[4];  // xyz = mask radii (engine axes)
    float band0[4];        // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w = banded flag
    float lighting0[4];    // x = lit flag, y = unused, z = ambient, w = diffuse
    float lighting1[4];    // xyz = unit vector TOWARD the light (engine axes), w = valid flag
    float lighting2[4];    // rgb = light color (max-component normalized), w = shadow-map strength
    float shadow_meta[4];      // count, compare sign, bias, atlas size
    float shadow_tiles[8][4];  // per-cascade atlas UV rects
    float shadow_mats[24][4];  // per-cascade 4x3 sampling transforms
    float shadow_meta2[4];     // x = far-visibility clamp flag; yzw = live-table camera origin
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
    // --- appended (private sun-depth map); append-only, mirrors sunVP/
    // sunMeta at the end of the HLSL cbuffer ---
    float sun_vp[4][4];    // row-vector world -> sun-depth clip
    float sun_meta[4];     // x = valid, y = map size (px), z = compare bias
                           // (normalized depth units), w = strength
    float locality_meta[4];    // x = per-caster locality pair count
    float locality[32][4];     // [2i] = center.xyz, [2i+1] = half extents.xyz
    // --- appended (engine lighting block); append-only, mirrors lightAmb
    // at the end of the HLSL cbuffer ---
    float light_amb[4];        // rgb = engine ambient (HDR) or (1,1,1) cold; w = engine mode
    float fog_engine[4];       // x = density scale (blk 41), y = fog end (blk 48),
                               // z = inv ramp range (blk 49), w = valid
    float fog_sky[4];          // sky row 17 gradient points; w = sky probe valid
    float fog_sky_col[4];      // sky row 7 base color (verbatim fog color)
};

struct alignas(16) CSConstantData {
    float view_proj[4][4];
    float depth_params[4];   // m22, m32, viewport MinDepth, viewport MaxDepth
    float screen_count[4];   // width, height, count, unused
    float pixel_query[4];    // x, y, unused, unused
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

    {   // instanced sun-depth VS + layout: non-fatal (per-caster loop covers)
        ID3DBlob* sd_blob = nullptr;
        const std::string sd_err = compile_shader(static_src.c_str(), "VSSunDepth", "vs_5_0", nullptr, &sd_blob);

        if (sd_err.empty() && sd_blob) {
            if (SUCCEEDED(dev->CreateVertexShader(sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), nullptr, &g_res.vs_sundepth))) {
                D3D11_INPUT_ELEMENT_DESC sl[] = {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                };

                if (FAILED(dev->CreateInputLayout(sl, 4, sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), &g_res.layout_sundepth))) {
                    KH_SAFE_RELEASE(g_res.vs_sundepth);
                }
            }

            sd_blob->Release();
        } else if (!sd_err.empty()) {
            report_error("KH sun-depth shader: " + sd_err);
        }
    }

    {   // cascade slice-router GS: non-fatal (injection stands down without it)
        ID3DBlob* gs_blob = nullptr;
        const std::string gs_err = compile_shader(static_src.c_str(), "GSCascadeSlice", "gs_5_0", nullptr, &gs_blob);

        if (gs_err.empty() && gs_blob) {
            dev->CreateGeometryShader(gs_blob->GetBufferPointer(), gs_blob->GetBufferSize(), nullptr, &g_res.gs_cascade);
            gs_blob->Release();
        } else if (!gs_err.empty()) {
            report_error("KH cascade GS: " + gs_err);
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    hr = dev->CreateInputLayout(layout, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &g_res.input_layout);
    vs_blob->Release();
    if (FAILED(hr)) { g_res.release(); return "CreateInputLayout " + hr_str(hr); }

    {
        const auto& reg = mesh_registry();

        for (int m = 0; m < KH_MESH_COUNT; ++m) {
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = static_cast<UINT>(reg[m].verts.size() * sizeof(MeshVertex));
            bd.Usage = D3D11_USAGE_IMMUTABLE;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA init = { reg[m].verts.data(), 0, 0 };
            hr = dev->CreateBuffer(&bd, &init, &g_res.mesh_vb[m]);
            if (FAILED(hr)) { g_res.release(); return "Create mesh VB " + hr_str(hr); }
        }
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
        // -1.0 overcorrected at grazing angles (mesh faces near edge-on to
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
        // Sun-depth pass rasterizer: the scene bias above exists to win
        // marginal ties against ENGINE depth; in our own private map it
        // would only carve the acne band deeper. Plain CullNone.
        rd.DepthBias = 0;
        rd.SlopeScaledDepthBias = 0.0f;
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_sun);
        if (FAILED(hr)) { g_res.release(); return "Create sun rasterizer " + hr_str(hr); }
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

    // SAMPLE_COUNT feeds the guard's across-samples loop; the shader
    // already recompiles whenever the depth MSAA count changes.
    const std::string sc = std::to_string(g_res.comp_depth_samples > 0 ? g_res.comp_depth_samples : 1);

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { nullptr, nullptr },
    };

    const std::string comp_src = std::string(g_cb_hlsl) + g_hlsl_composite + g_hlsl_composite2;
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

// Private sun-depth map target: single-sample R32 depth, DSV to render,
// SRV to sample. Non-fatal - every consumer stands down when absent.
static constexpr UINT KH_SUN_DEPTH_SIZE = 2048;
// Casters beyond this camera radius stay OUT of the sun-depth fit (no
// self-shadow, no cast) - the same distance behavior the engine's own
// object shadows have. Bounding the fit is what bounds the texel size,
// the locality reach and the normal-offset all at once.
static constexpr float KH_SUN_FIT_RADIUS = 250.0f;

inline bool ensure_sun_depth(ID3D11Device* dev) {
    if (g_res.sun_tex && g_res.sun_dsv && g_res.sun_srv) return true;
    KH_SAFE_RELEASE(g_res.sun_srv);
    KH_SAFE_RELEASE(g_res.sun_dsv);
    KH_SAFE_RELEASE(g_res.sun_tex);
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = KH_SUN_DEPTH_SIZE;
    td.Height = KH_SUN_DEPTH_SIZE;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_TYPELESS;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_res.sun_tex))) return false;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D32_FLOAT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

    if (FAILED(dev->CreateDepthStencilView(g_res.sun_tex, &dd, &g_res.sun_dsv))) {
        KH_SAFE_RELEASE(g_res.sun_tex);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = DXGI_FORMAT_R32_FLOAT;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;

    if (FAILED(dev->CreateShaderResourceView(g_res.sun_tex, &sd, &g_res.sun_srv))) {
        KH_SAFE_RELEASE(g_res.sun_dsv);
        KH_SAFE_RELEASE(g_res.sun_tex);
        return false;
    }

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
    D3D11_BOX mesh = { 0, 0, 0, result_count * sizeof(float) * 4, 1, 1 };
    ctx->CopySubresourceRegion(g_res.staging_buffer, 0, 0, 0, 0, g_res.output_buffer, 0, &mesh);
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
// Mesh selector: STRING name/alias (case-insensitive) or SCALAR registry
// index. Returns -1 for anything unknown.
inline int mesh_id_from_gv(const game_value& gv) {
    if (gv.type_enum() == game_data_type::SCALAR) {
        const int id = static_cast<int>(static_cast<float>(gv));
        return (id >= 0 && id < KH_MESH_COUNT) ? id : -1;
    }

    if (gv.type_enum() != game_data_type::STRING) return -1;
    std::string s = static_cast<std::string>(gv);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    const auto& reg = mesh_registry();

    for (int i = 0; i < static_cast<int>(reg.size()); ++i) {
        if (s == reg[i].name || (reg[i].alias[0] != '\0' && s == reg[i].alias)) return i;
    }

    return -1;
}

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
// participate in that - which is why depth-tested meshes paint OVER smoke.
// The fix is ordering, not measurement: hook the immediate context's draw
// calls (MinHook, shared via framework's ensure_minhook), watch the output-
// merger state, and the moment the engine issues its FIRST translucent draw
// of the scene (alpha blending on, depth test on / write off, main scene
// depth bound), draw our eligible meshes right there with DEPTH WRITES.
// Every translucent the engine draws afterwards then composites against
// them natively: smoke in front blends over the mesh, smoke behind it is
// depth-rejected. Pixel-perfect, zero rays, zero estimation.
//
// Mechanics:
//  - State hooks (OMSetBlendState / OMSetDepthStencilState /
//    OMSetRenderTargets) maintain three booleans; state objects are
//    classified LIVE via GetDesc on every set - deliberately never
//    memoized by pointer (see the no-memoization note below: reused
//    addresses would inherit stale classifications). Per-draw cost is
//    still just a few flag loads; GetDesc runs only at state SETS.
//  - ClearDepthStencilView on the main scene depth marks the new frame
//    (injection re-arms).
//  - Deferred contexts share the vtable: every hook first checks that the
//    calling context IS the tracked immediate context and passes straight
//    through otherwise. Injection recursion is excluded with a flag, so
//    our own state sets/draws never disturb the tracking.
//  - Eligibility: solid (effect 0), non-overlay, composite (default true)
//    meshes. Effect meshes need the mid-flush scene capture and overlays are
//    post-scene by definition - both stay on the flush path.
//  - Health: each injection stamps a timestamp; the flush only stands
//    down for eligible meshes while an injection happened within the last
//    half second. If the hook never engages (install failure,
//    unrecognized pipeline), meshes degrade to the flush path
//    automatically.
//  - Engine reset: a generation counter tells the render thread to drop
//    its memoized state-object classifications (pointers may be reused).
// ===========================================================================

static std::atomic<bool>     g_reorder_hook_active{false};
static bool                  g_reorder_hook_failed = false;      // game thread only
static std::atomic<void*>    g_reorder_target_ctx{nullptr};      // immediate context to act on
static std::atomic<uint64_t> g_composite_last_inject_ms{0};
static std::atomic<uint64_t> g_composite_inject_serial{0};

// Injection bridge-PV memory (formerly function statics; hoisted so the
// mission-end session destroy can reset them). Render thread only.
static RVExtBridge::ProjectionViewTransform g_last_pv = {};
static uint64_t g_last_pv_ms = 0;

// The locator's validity BLINKS on exactly the foreign-pass frames, so
// the anomaly gate keys on ever-proven-this-session instead. Cleared
// only with the session.
static bool g_proj_locator_ever = false;

inline uint64_t steady_now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// The composited path counts as healthy while injections are actually
// happening; outside that window (menus, install failure, pipeline change)
// eligible meshes revert to the post-scene flush path.
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
// never falsely clip a mesh that is genuinely in front.
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
// before it got overdrawn by the meshes for that frame (and a fully starved
// frame lost the meshes entirely). Triggering early on an isolated blended
// draw amid the opaques is harmless by comparison: opaque ordering is
// depth-resolved regardless of when the meshes land, subsequent translucents
// still composite against them, and the only residue is a translucent mesh
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
// mesh draws carry the exact translucent signature (alpha blend, no-write
// depth, main DSV), so if the hooks tracked them, the game thread would
// pollute the render thread's phase state and could even trigger a second,
// post-scene injection - meshes over everything for a frame. Every hook
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
    // were rejected and the mesh blinked out. The world-shape SPAN test on
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
    // frame's projection. A near-plane difference skews the meshes' stored
    // depth hyperbolically: negligible for far fragments, enormous for near
    // ones - which is why the resulting flicker scales inversely with the
    // distance to the mesh. Latching at the clear keeps the meshes' depth
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
    // PASSIVE PARTITION TELLTALE: the latest proj-shaped upload seen at
    // the KNOWN locator slot this cycle - m22/m32 verbatim and the near
    // they imply. This is the engine's own statement of the projection
    // the CURRENT phase renders with, refreshed even while the sniffer's
    // lock idles, and it is the injection's preferred depth encode: the
    // sniffer's discovery anchors to the clear-time latch, so a latch
    // that caught the bridge mid-publication (holding a foreign
    // projection, e.g. the near-0.01 viewmodel window) poisons the lock
    // itself, while the telltale's fov anchors reject foreign windows
    // outright. Read-only: it never touches the lock, adoption, or
    // re-arm machinery.
    float slot_near_live = -1.0f;
    float slot_m22 = 0.0f;
    float slot_m32 = 0.0f;
    // Opaque draws since the last landed injection, counted ACROSS
    // clears: a foreign cycle repainting the world over an injected
    // frame reads scene-scale here while we sit uninjected - and that
    // redraw is itself the re-injection trigger.
    uint32_t opaques_since_inject = 0;
    // Frame-scoped risk flag for the hybrid guarantee: set when an
    // anomalous (slot-silent) cycle is seen at an injection attempt;
    // consumed by the flush, which then ALSO draws the meshes late that
    // frame - identical overdraw if the injection survived, fill-in if
    // a foreign pass erased it.
    bool anomaly_seen = false;
};
static ReorderState g_ro;
// PERSISTED SLOT PAIR: the last LIVE camera-class telltale values, stamped
// at the probe site with a timestamp. The flush's carried draw prefers
// these when the live slot is silent and the stamp is fresh - the field
// convicted a near-0.01 (viewmodel-window) latch reaching the carry
// through exactly that blind spot (missLastNear 0.01, whole-box-gone).
// Render-thread writes; game-thread reads are park-ordered by the flush's
// graphics lock, the same contract as every other g_ro consumer.
static float    g_slot_keep_m22 = 0.0f;
static float    g_slot_keep_m32 = 0.0f;
static float    g_slot_keep_near = -1.0f;
static uint64_t g_slot_keep_ms = 0;
static uint64_t g_keep_stamp_rejects = 0;   // probe-accepted pairs the keep's band refused
static uint64_t g_keep_stale_skips = 0;     // keep deferred to a fresher in-band pv
                                            // (the moving-camera silent frame)
static uint64_t g_cast_arm_lost_ms = 0;     // stamp of the LAST cast arm lost at the
                                            // boundary (the ACCEPTED baseline blink class)
// Trigger-range forensics (render thread writes, stats read): the last
// span-REJECTED viewport depth range and the last ACCEPTED one.
static float g_trig_rej_vp[2] = { -1.0f, -1.0f };
static float g_trig_acc_vp[2] = { -1.0f, -1.0f };
// Fog forensics: the LAST fog color/enable actually written into a CB
// (game or render thread writes at fill; stats read - diagnostics).
static float g_fog_dbg[4] = { 0.0f, 0.0f, 0.0f, 0.0f };   // rgb + enable

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
//    validating against it rejected the truth and blinked the mesh exactly
//    on those frames. Instead, accept when within 10% of the latch OR 25%
//    of the previously captured value (continuity): genuine per-frame
//    changes pass, a repurposed buffer (foreign near plane) fails both
//    and forces rediscovery.
// Read-only partition probe for the KNOWN locator slot: the sniffer's
// structural signature and sx/sy/m22 anchors against the latch, with m32
// free by sign - m32 (~ -near) is exactly the value that legitimately
// changes between partitions and phases, so the anchored sniffer cannot
// track it live; this probe can. Returns the implied near (and the
// verbatim pair) or -1.
inline float proj_slot_probe(const float* w, float* out_m22, float* out_m32) {
    const float sx = g_ro.cycle_pv.projection[0][0];
    const float sy = g_ro.cycle_pv.projection[1][1];
    const float m22_l = g_ro.cycle_pv.projection[2][2];
    const float m32_l = g_ro.cycle_pv.projection[3][2];
    if (fabsf(m32_l) < 1e-9f) return -1.0f;

    auto close = [](float a, float b, float rel) {
        return fabsf(a - b) <= rel * (fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b)) + 1e-5f;
    };

    if (!g_proj_locator.transposed) {
        if (proj_near_zero(w[1]) && proj_near_zero(w[2]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[15]) &&
            fabsf(w[11] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
            w[14] != 0.0f && w[14] * m32_l > 0.0f && fabsf(w[10]) > 1e-9f) {
            *out_m22 = w[10];
            *out_m32 = w[14];
            return -w[14] / w[10];
        }
    } else {
        if (proj_near_zero(w[1]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[8]) && proj_near_zero(w[9]) && proj_near_zero(w[12]) && proj_near_zero(w[13]) &&
            fabsf(w[14] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
            w[11] != 0.0f && w[11] * m32_l > 0.0f && fabsf(w[10]) > 1e-9f) {
            *out_m22 = w[10];
            *out_m32 = w[11];
            return -w[11] / w[10];
        }
    }

    return -1.0f;
}

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
    if (!g_ro.cycle_pv_valid) return;

    // Partition telltale: uploads to the KNOWN slot refresh the live pair
    // even while the lock idles. Read-only; the lock/idle/adoption
    // discipline below is unchanged.
    if (g_proj_locator.valid && res == g_proj_locator.buf && bytes >= 16 * sizeof(float)) {
        const uint32_t pn = (bytes > 16384 ? 16384 : bytes) / 4;

        if (g_proj_locator.float_offset + 16 <= pn) {
            float p22 = 0.0f, p32 = 0.0f;
            const float pnear = proj_slot_probe(
                static_cast<const float*>(data) + g_proj_locator.float_offset, &p22, &p32);

            if (pnear > 0.0f) {
                g_ro.slot_near_live = pnear;
                g_ro.slot_m22 = p22;
                g_ro.slot_m32 = p32;

                // KEEP SANITIZATION (field conviction: flushSlotKeeps moved
                // in LOCKSTEP with missFrames while missLastNear read 0.01
                // across three dumps - the keep branch took on the flicker
                // frames and delivered the poison). The probe passes
                // viewmodel-class windows by design (m32 free, fov and m22
                // match the scene's), so the keep cached a late-frame
                // viewmodel upload and fed it to every silent frame for
                // 250 ms - a poison reservoir the per-cycle slot never
                // was. The slot's semantics stay untouched (the far-phase
                // test NEEDS far values); the KEEP accepts only ordinary-
                // play camera nears (documented sweep 0.09-0.96, banded
                // with margin: 0.05-5.0).
                if (pnear >= 0.05f && pnear <= 5.0f) {
                    g_slot_keep_m22 = p22;
                    g_slot_keep_m32 = p32;
                    g_slot_keep_near = pnear;
                    g_slot_keep_ms = steady_now_ms();
                } else {
                    g_keep_stamp_rejects++;
                }
            }
        }
    }

    if (g_ro.engine_proj_valid) return;
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
            g_proj_locator_ever = true;   // persistent: the anomaly gate keys on this
            g_stats.composite_proj_lock++;
            return;
        }
    }
}

// Returns the buffer's byte width when it is a plausibly-sized constant
// buffer, 0 otherwise.
inline uint32_t proj_upload_byte_width(ID3D11Resource* res) {
    // GetType instead of QueryInterface: this runs on EVERY tracked
    // map/unmap - ~1,600 per frame with lit meshes - and QI adds an
    // AddRef/Release round trip on top of the lookup. The dimension enum
    // guarantees the static_cast; GetDesc stays (the width must be truth,
    // never a cache: a recycled buffer pointer with a stale cached width
    // would let the scanners read past a smaller mapping).
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    res->GetType(&dim);
    if (dim != D3D11_RESOURCE_DIMENSION_BUFFER) return 0;

    D3D11_BUFFER_DESC bd = {};
    static_cast<ID3D11Buffer*>(res)->GetDesc(&bd);

    // 16-float floor RESTORED: a one-session 8-float floor let tiny CBs
    // compete for the pending capture slots and starved the view CB's
    // capture at spawn (viewLocks stuck at 0, first cast at 48 s) - and
    // it bought nothing: every located fingerprint lives in CBs >= 176 B.
    if ((bd.BindFlags & D3D11_BIND_CONSTANT_BUFFER) == 0 || bd.ByteWidth < 16 * sizeof(float)) {
        return 0;
    }

    return bd.ByteWidth;
}

// ===========================================================================
// World lighting (sun/moon) + shadow-pass recon.
//
// LIGHTING (live now, fully script-free): direction comes from the
// cascade-derived latch, colors from the located lighting block; the only
// scripted fetch left is fogParams (value/base are not separable from the
// block, and staged decay anchors the block probe). Once per flush the
// game thread STAGES the fog (stage_world_lighting) and PUBLISHES
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
// Ambient light (SQF-staged alongside the sun, elems 4-5 of the lighting
// command: [r,g,b] color and brightness). The shadow's color/depth derive
// from it: lit = ambient + sun, shadowed = ambient, so the per-channel
// factor is amb/(amb+sun).

// --- Game-thread staging (written and read on the game thread only) ---
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


// Set by publish_world_lighting when the published sun direction jumps
// more than ~3 degrees in a single frame (skipTime / setDate / sun<->moon
// swaps); consumed at the next main depth clear, where the render thread
// wipes every sun-dependent frozen snapshot (band seals, live cascade
// entries). Natural time never trips this per-frame threshold - slow drift
// is covered by the staleness expiry in shadow_live_frame_reset instead.
static bool g_sun_jump_pending = false;
// Engine-sun reference for the jump arbiter, mirrored from the sky CB's
// floats 8-10 at every sky-mirror refresh (defined later in the file;
// these early globals exist because the detector precedes the probe).
static float g_skysun_ref[3] = { 0, 0, 0 };
static bool  g_skysun_ref_valid = false;
static uint64_t g_sun_jump_refused = 0;   // candidate jumps refused by the engine-sun arbiter

// The ONE sun for shadow geometry (map render + both fire fills): the
// cascade-DERIVED direction is the sun the world's shadows actually
// follow, but the derivation can be CONTAMINATED (low-sun sessions
// measured it 13 deg off the reported sun, with liveRejOrtho going
// nonzero - foreign uploads entering the fit). Trust it only within
// ~4 deg of the reported sun; beyond that the measurement is broken and
// the reported sun bounds the worst-case cast error at 4 deg instead.
static uint32_t g_sun_derived_samples = 0;   // derivation warm-up meter

// Sun debounce latch (formerly publish_world_lighting statics; hoisted so
// the mission-end session destroy can reset them). Game thread only.
static bool  g_pub_valid = false;
static float g_pub_dir[3] = {};
static float g_cand_dir[3] = {};
static int   g_cand_hold = 0;

inline const float* kh_shadow_sun() {
    // The DEBOUNCED published direction, not the raw latch: the sun map
    // and the fire flapping on raw derived was the cast-flicker symptom
    // of the jump storm. g_sun_dir_engine is game-published under the
    // flush lock (render thread parked), so render-thread reads here see
    // stable values - the same precedent the old warm-up relied on. The
    // validity gate covers the 120-sample convergence; consumers stand
    // down cleanly on nullptr. The sunset lesson survives: this measures
    // the engine's CLAMPED shadow light, correct where getLighting
    // diverged.
    if (g_sun_valid) return g_sun_dir_engine;
    return nullptr;
}

inline void publish_world_lighting() {
    // SQF-FREE SUN/MOON: the cascade-derived latch IS the published
    // direction (getLighting retired). This runs inside flush_locked with
    // the render thread parked, so reading the render-written latch here
    // is the same invariant every other consumer relies on. Validity
    // waits for the 120-sample warm-up (~0.5 s of cascade finalizes) -
    // the same convergence window the old engine-verbatim blend covered.
    // The derived direction measures the engine's CLAMPED shadow light
    // and is correct exactly where getLighting diverged most (low sun).
    const bool sun_ok = g_sun_dir_derived_valid && g_sun_derived_samples >= 120;

    if (sun_ok) {
        // DEBOUNCE LATCH (the jump-storm lesson: sunJumpFlushes 207 in one
        // session after the cutover, castArmsLost and rearms trailing it -
        // receive shadows wiped and rebuilt on every flap). The derivation
        // occasionally flaps past 3 deg between cascade measurements at
        // certain sun angles; getLighting used to hide that. Small moves
        // GLIDE; a big move must HOLD for 8 consecutive flushes (~150 ms)
        // before it is adopted and fires exactly one jump - a real
        // skipTime still snaps promptly, oscillation between two readings
        // adopts neither and the published direction stays put.
        if (!g_pub_valid) {
            g_pub_dir[0] = g_sun_dir_derived[0];
            g_pub_dir[1] = g_sun_dir_derived[1];
            g_pub_dir[2] = g_sun_dir_derived[2];
            g_pub_valid = true;
        } else {
            const float dp = g_pub_dir[0] * g_sun_dir_derived[0] +
                             g_pub_dir[1] * g_sun_dir_derived[1] +
                             g_pub_dir[2] * g_sun_dir_derived[2];

            if (dp >= 0.99863f) {
                // within ~3 deg: glide, no jump machinery
                g_cand_hold = 0;
                g_pub_dir[0] += (g_sun_dir_derived[0] - g_pub_dir[0]) * 0.25f;
                g_pub_dir[1] += (g_sun_dir_derived[1] - g_pub_dir[1]) * 0.25f;
                g_pub_dir[2] += (g_sun_dir_derived[2] - g_pub_dir[2]) * 0.25f;
                const float pl = sqrtf(g_pub_dir[0] * g_pub_dir[0] +
                                       g_pub_dir[1] * g_pub_dir[1] +
                                       g_pub_dir[2] * g_pub_dir[2]);

                if (pl > 1e-6f) {
                    g_pub_dir[0] /= pl;
                    g_pub_dir[1] /= pl;
                    g_pub_dir[2] /= pl;
                }
            } else {
                const float dc = g_cand_dir[0] * g_sun_dir_derived[0] +
                                 g_cand_dir[1] * g_sun_dir_derived[1] +
                                 g_cand_dir[2] * g_sun_dir_derived[2];

                if (g_cand_hold > 0 && dc >= 0.99863f) {
                    g_cand_hold++;
                } else {
                    g_cand_dir[0] = g_sun_dir_derived[0];
                    g_cand_dir[1] = g_sun_dir_derived[1];
                    g_cand_dir[2] = g_sun_dir_derived[2];
                    g_cand_hold = 1;
                }

                if (g_cand_hold >= 8) {
                    // ENGINE CONFIRMATION (the false-jump lesson: 21
                    // wipes on a static sun, keyed to look direction -
                    // the derivation settles into VIEW-DEPENDENT stable
                    // readings, so each look-away legitimately passes
                    // the hold). A jump is adopted only if the ENGINE
                    // sun (sky CB floats 8-10) has ALSO left the
                    // published direction; otherwise the candidate is
                    // refused and the cascade table lives. Fails open
                    // when the sky mirror is unavailable.
                    bool engine_confirms = true;

                    if (g_skysun_ref_valid) {
                        const float de = g_skysun_ref[0] * g_pub_dir[0] +
                                         g_skysun_ref[1] * g_pub_dir[1] +
                                         g_skysun_ref[2] * g_pub_dir[2];
                        if (de >= 0.99863f) engine_confirms = false;
                    }

                    if (engine_confirms) {
                        g_pub_dir[0] = g_cand_dir[0];
                        g_pub_dir[1] = g_cand_dir[1];
                        g_pub_dir[2] = g_cand_dir[2];
                        g_cand_hold = 0;
                        g_sun_jump_pending = true;   // exactly one jump per real move
                    } else {
                        g_cand_hold = 0;
                        g_sun_jump_refused++;
                    }
                }
            }
        }

        g_sun_dir_engine[0] = g_pub_dir[0];
        g_sun_dir_engine[1] = g_pub_dir[1];
        g_sun_dir_engine[2] = g_pub_dir[2];
    }

    g_sun_valid = sun_ok;
    g_fog_valid = g_fog_staged_valid;
    g_fog[0] = g_fog_staged[0];
    g_fog[1] = g_fog_staged[1];
    g_fog[2] = g_fog_staged[2];
}

// ===========================================================================
// LIVE shadow-map capture: the recon's verdict, promoted to infrastructure.
// The engine renders its cascades as depth-only cycles into one SRV-capable
// atlas (one cycle per cascade, distinguished by viewport), and uploads the
// cascade's world->atlasUV+depth 4x3 into a constant block during that
// cycle. Per frame, per cascade cycle: latch that matrix from the upload
// (locator-cached offset, recon-style rediscovery on miss), snapshot the
// bound viewport at the cycle's next draw as the cascade's atlas tile, and
// hand the table to the composited mesh shader, which then performs the
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
    float    time = 0.0f; // seal wall-clock (staleness expiry; see frame_reset)
};

struct LiveShadowState {
    std::atomic<bool> wanted{ false };        // game thread: any lit object exists
    // Atlas tracking (render thread)
    void*    atlas_identity = nullptr;
    ID3D11Texture2D*          atlas_tex = nullptr;   // AddRef'd
    ID3D11ShaderResourceView* atlas_srv = nullptr;
    uint32_t atlas_size = 0;
    uint32_t atlas_fmt = 0;
    float    atlas_last_seen = 0.0f;   // last t15 consumption of the HELD atlas (stamped in mask_note_draw)
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
    uint64_t resolve_hits = 0;
    uint64_t resolve_draws = 0;        // resolve-target draws examined
    uint64_t resolve_cb_found = 0;     // bound CBs matched in the registry
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
        // The provisional vcol came from the BRIDGE fallback (no engine
        // view published yet - the spawn window): a cross-convention
        // pairing the receive term must not consume. Cleared when the
        // true same-frame view completes the seal.
        bool  vcol_bridge = false;
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
    // Stamp of the last EXACT-CLASS (<= 1e-3) publication - the cast
    // gate's input. e_rot is measured against the bridge latch, which
    // lags +-1 frame, so camera motion pushes single publications above
    // 1e-3 under a perfectly healthy lock (field: castArmsLost
    // 2,694->3,978 in one session against a 0-7 norm, castBatches at
    // ~43% of resolveHits, viewBestRot 0.0 throughout - roughly half
    // the resolve batches starved by measurement jitter, not lock
    // quality). A healthy lock re-proves exactness within the window
    // continuously; a genuinely off-axis era never does and still
    // starves - at most one window late. Exported as viewPubExactAgeS.
    uint64_t pub_exact_ms = 0;                // 0 = no exact publication yet
    uint64_t cold_pub_rejects = 0;            // cold-guard publishes rejected (full no-ops; forensic)
    uint64_t band_bail_view = 0;     // no same-frame view latched: reseal skipped
    uint64_t band_bail_quality = 0;  // seal pairings withheld: publication not exact-class
    uint64_t band_prov_skips = 0;    // pending bridge-provisional bands withheld from receive
    float band_last_reject[4] = {};  // last border quad rejected by sanity
    uint64_t cast_misses = 0;   // FIRST failed fire guard this frame (0 = firing; see mask_cast_engine)
    // A GENUINE shadow resolve is the screen-sized single-channel draw
    // that FOLLOWS its cascade's atlas pass. PSCB_PostProcess (cb13) is
    // bound across the engine's whole postprocess chain, so matching the
    // block alone admits every R8-target postprocess draw - casting with
    // its stale matrix lanes splatted the mesh across the atlas and
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
    // per-frame view (the mesh renders pixel-registered with it, every
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
    uint32_t pub_rej_streak = 0;     // consecutive bridge-truth publish rejections
    uint32_t view_src_miss = 0;
    uint64_t view_locks = 0;
    uint64_t frame_view_hits = 0;
    float view_best_rot = -1.0f;    // best candidate's rotation error at last injection
    // Exactness-preference wait: time of the first bar-passing but
    // non-exact candidate deferral (0 = not waiting). Cleared on lock.
    float view_wait_since = 0.0f;
    float view_best_trans = -1.0f;  // and its translation error (diagnosis when unlocked)
    float prev_view[16] = {};       // previous accepted view (rotation extrapolation)
    bool  prev_view_valid = false;
    uint64_t seal_completions = 0;  // deferred seals finished by an arriving view
    // Upload locator cache
    bool     cache_valid = false;
    uint32_t cache_bytes = 0;
    uint32_t cache_offset = 0;
    uint64_t latches = 0;
    float    cam[3] = {};      // the frame's render camera (engine axes),
                               // recorded at injection - the shadow space origin
};
static LiveShadowState g_ls;

// ===========================================================================
// LIGHTING-BLOCK LOCATOR, v4: read-only forensics.
// The v2/v3 fingerprint hunts ended with a decisive accident: during a
// setFog transition, the base needle froze a 56-float CB that IS the
// engine's per-frame lighting+fog block - HDR sun color at [16..18]
// (magnitude tracks sunBrightness, warm morning ratios), the same chroma
// at another scale at [24..26], an ambient-shaped triple at [20..22], a
// near-white slightly-blue triple at [36..38] (the fog/sky color
// candidate), the staged fog DECAY at [40], a fogValue-derived scalar at
// [41], and NEGATIVE CAMERA ALTITUDE at [52]. The direction-vector probe
// meanwhile only ever locked light-space rotation rows (a rotation's LAST
// row evades a forward-only orthogonality check because the translation
// follows it): the whole aligned-vector approach is retired.
// v4 keys on STRUCTURE instead - two independent live quantities at a
// fixed spacing: a lane equal to the staged fog decay with a lane 12
// floats later equal to minus our injection-recorded camera altitude.
// Both are known exactly, both move independently of everything else in
// a CB, and the joint match at that spacing is collision-proof without
// any planted values or transitions. On lock, the 56 floats from
// [anchor-40] mirror into getRenderStats every time the location
// re-uploads. Nothing here writes to the pipeline; behavior is unchanged.
// ===========================================================================
struct CbColorProbe {
    float    last_mode = 0.0f;       // block lane 15: 1 = standard layout,
                                     // 2 = the altitude/atmospheric VARIANT
                                     // (different lane semantics - black-box
                                     // green-flicker lesson; mirror refresh
                                     // is gated on mode 1)
    ID3D11Resource* buf = nullptr;   // locked upload location (weak identity)
    uint32_t off = 0;                // float index of the DECAY anchor lane
    uint32_t floats = 0;             // CB size (floats) at lock time
    uint32_t nb_base = 0;            // first float mirrored into nb[]
    int      meta = 0;               // off - nb_base (40 when the full head fits)
    bool     valid = false;
    uint32_t misses = 0;             // locked-CB uploads with the anchor found nowhere
    uint32_t relocs = 0;             // anchor found at a DIFFERENT offset (ring reuse)
    uint64_t hits = 0;               // anchor confirmations (health: climbs steadily)
    float    last_err = 0.0f;        // altitude-lane residual at the last confirmation
    float    last_confirm = 0.0f;    // wall clock of the last confirmation (expiry)
    float    nb[96] = {};            // live block mirror (render thread writes,
                                     // game thread reads - diagnostics AND the
                                     // fog-chroma consumer, under the park
                                     // invariant like everything else)
};
static CbColorProbe g_light_probe;

// LAMP PROBE: same machinery, planted-truth fingerprint. The operator
// creates a #lightpoint at a KNOWN world position with a KNOWN garish
// color and stages the position via setRenderLightProbe; the anchor is
// that position appearing as three consecutive lanes, tested in WORLD
// space and CAMERA-RELATIVE space (the v2/v3 sessions already proved the
// per-light struct family flows through this funnel: a 44-float CB with
// direction/position rows, shared across lights - the shared-buffer miss
// pattern is expected and harmless under time-based expiry). Once the
// block mirrors into the dump, the known planted color names the color
// lanes and intensity steps map the attenuation lanes.

// SKY/ATMOSPHERE CB PROBE: the fog COLOR's home, identified in the
// engine's terrain PS disassembly (18 rows; the gradient that makes the
// haze blue against sky and warm over ground lives at row 17). Anchor is
// structural, all conditions at fixed row offsets from the buffer start:
// row 15 a Rayleigh-shaped extinction triple (blue-dominant, small,
// positive), row 16 a near-neutral positive triple (Mie), row 17
// ascending gradient control points, rows 1 and 7 plausible colors, row
// 14 a (refAlt, scale, decay) pack with a decay-like z.
static CbColorProbe g_sky_probe;

// SKY-CANDIDATE FORENSICS (two sessions locked instantly, two never - a
// blind anchor iteration is off the table): every scanned upload with
// nf >= 72 is scored against the anchor conditions as a bitmask (1 =
// row15 positive-small, 2 = blue >= red, 4 = row17 ascending, 8 = row17
// bounded, 16 = row14 sane, 32 = row17 EXACT (0.25, 0.75, 3.5)); the
// best scorer's first 96 floats are mirrored. One dump names the failing
// condition - or proves no candidate of that size exists in the funnel.

inline bool locator_sky_anchor(const float* f, uint32_t nf) {
    if (nf < 72) return false;

    // FAST PATH: the gradient control points (row 17) read exactly
    // (0.25, 0.75, 3.5) in every locking field dump - clean engine
    // constants. Should a map or config ever move them, the structural
    // path below still stands.
    if (f[68] == 0.25f && f[69] == 0.75f && f[70] == 3.5f) return true;

    // STRUCTURAL PATH, v3. v2's 'blue >= red' Rayleigh assumption was
    // BACKWARDS: the engine's extinction row is RED-dominant (0.1814,
    // 0.0159, 0.0111) - precisely what the mode-2 block's atmospheric
    // lanes showed long ago - and that one inequality rejected the real
    // live 416-byte sky CB for five straight sessions while the
    // forensics mirrored it as 'the impostor'. Its identity is beyond
    // doubt: the sun direction sits at floats 8-10 matching
    // sunDirEngine to four decimals, the color rows track the light,
    // and float 89 is the 16:9 aspect ratio. Only shape remains: a
    // positive small extinction triple, ascending gradient, sane haze
    // pack.
    const float* ray = f + 60;   // row 15
    if (!(ray[0] > 0.0f && ray[1] > 0.0f && ray[2] > 0.0f && ray[0] < 1.0f && ray[2] < 1.0f)) return false;

    const float* grad = f + 68;  // row 17
    if (!(grad[0] >= 0.0f && grad[1] >= grad[0] && grad[2] > grad[1] && grad[2] < 32.0f)) return false;

    const float* lay = f + 56;   // row 14: (refAlt, scale, decay)
    if (!(fabsf(lay[0]) < 5000.0f && lay[1] >= 0.0f && lay[2] > 0.0f && lay[2] < 0.5f)) return false;

    return true;
}

inline void locator_capture(CbColorProbe& pr, const float* f, uint32_t nf, uint32_t win);
inline void shadow_view_scan(ID3D11Resource* res, const void* data, uint32_t bytes, uint32_t base_off);
inline void locator_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes);

// SKY BINDING PROBE: the upload funnel proved session-intermittent for
// the sky CB - 32k sightings one session, ZERO the next, same map and
// mission; the engine fills it through paths the hooks cannot see in
// some sessions. The binding cannot hide: for the engine to render fog
// the buffer must be BOUND. At each injection, before our own state
// touches slot 0, the plausible-size bound PS CBs are copied to staging
// and read back a frame later - DO_NOT_WAIT, never a stall - and the
// contents feed the same anchor and mirror as the upload path.
struct SkyBindProbe {
    ID3D11Buffer* staging[16] = {};   // 0-7 = PS slots, 8-15 = VS slots
    uint32_t      bytes[16] = {};
    ID3D11Buffer* src[16] = {};       // source buffer (identity only, no ref)
    uint32_t      base_fl[16] = {};   // window base offset in FLOATS
    uint16_t      pending = 0;        // bitmask: staging[i] holds an unread copy
};
static SkyBindProbe g_skybind;
static uint64_t g_skybind_reads = 0;   // staging readbacks completed
static uint64_t g_skybind_hits = 0;    // readbacks that passed the anchor
static uint32_t g_skybind_minbw = 0;   // smallest PS CB ByteWidth seen bound
static uint32_t g_skybind_maxbw = 0;   // largest  PS CB ByteWidth seen bound
static uint32_t g_skybind_slots = 0;   // bitmask: slots ever seen non-null
static uint32_t g_skybind_off1 = 0;    // 1 = the 11.1 offset path is live
static uint32_t g_skybind_maxbw_vs = 0;   // largest VS CB ByteWidth seen bound
static uint64_t g_viewbind_scans = 0;     // VS windows fed to the view scan
static uint32_t g_stage_total = 0;        // g_draw_list size at the last flush snapshot
static uint32_t g_stage_rej_vis = 0;      // rejected: invisible / not composite-eligible
static uint64_t g_recv_term_skips = 0;    // lit meshes drawn WITHOUT the received-shadow term
static uint64_t g_recv_wipes = 0;         // sun-jump boundary wipes of the live table
static uint64_t g_view_relock_forced = 0; // locks dropped by the publish-reject streak
static uint64_t g_lock_wipes = 0;         // live-table wipes at view-lock adoption
static uint32_t g_stage_rej_exp = 0;      // rejected: lifetime expired
static float    g_pub_first = -1.0f;       // cold publish-window epoch (hoisted static; session-reset)
static ID3D11Resource* g_prewarm_cand_res = nullptr;   // prewarm double-confirmation memory
static uint32_t g_prewarm_cand_off = 0;
static ID3D11Resource* g_coldlock_cand_res = nullptr;  // injection cold-adoption double-confirmation memory
static uint32_t g_coldlock_cand_off = 0;
static uint64_t g_inj_guard_off = 0;       // injections drawn WITHOUT the punch-through guard
                                           // (the see-through / overlay correlate for field dumps)
static uint64_t g_flush_fallback_draws = 0;// composite-eligible meshes drawn on the LATE flush path
static uint64_t g_flush_latch_pvs = 0;     // flushes transformed with the cycle latch
static uint64_t g_flush_pv_repairs = 0;    // degenerate cycle-state reads repaired from the live bridge
static uint64_t g_flush_repaint_saves = 0; // frames whose world redrew after the injection
static uint64_t g_flush_anomaly_carries = 0; // hybrid late copies drawn on anomalous frames
static uint64_t g_flush_slot_keeps = 0;    // carried draws encoded with the PERSISTED slot pair
                                           // (injection missed the frame: the other rare-artifact correlate)
static uint64_t g_skybind_offs_seen = 0;   // slots observed with nonzero offsets

// Flush serial (game thread, under the graphics lock): once-per-flush key
// for the miss latch and the flush-side census stamps. (The inter-clear
// window census that once lived here was FIELD-FALSIFIED - clears == 
// flushes exactly in every measured session - and removed; the post-flush
// redraw census below is the surviving, field-proven detector.)
static std::atomic<uint64_t> g_cc_flush_serial{0};   // ticks once per flush_locked

// FLICKER SAFETY LOGGING (permanent): dump-on-flicker ages for the two
// live carry correlates. A fresh flAgeFallbackS/flAgeAnomSkipS at a
// visible flicker names a carried frame; castArmLostAgeS (stamped at the
// boundary) names the accepted baseline blink class.
static uint64_t g_fl_fallback_ms = 0;     // flush drew a composite mesh LATE
static uint64_t g_fl_anom_skip_ms = 0;    // injection skipped a silent (anomalous) cycle
// POST-FLUSH REDRAW CENSUS (permanent; field-proven - it photographed the
// real clear-less repaint twice, 33/34 opaques): a world redraw AFTER the
// flush shows up as opaques drawn past the flush's stamp. The class is an
// accepted residual at natural rarity; this is its detector.
static std::atomic<uint32_t> g_cc_flush_opaques{0xFFFFFFFFu};   // sentinel = no flush yet
static uint64_t g_cc_postflush_redraws = 0;   // windows with >=32 opaques after the flush
static uint32_t g_cc_pf_last_draws = 0;       // forensics: last such delta
static uint64_t g_cc_pf_last_ms = 0;          // stamp (age at dump)

// MISS-REASON LATCH (pure diagnostics): the flicker is CONVICTED as the
// fallback-carried frame itself (4-for-4 sub-second lineup coincidence;
// both repaint census variants zero) - the late post-scene draw lands
// over the translucents for one frame. What remains is WHY the injection
// missed: no trigger (attempts 0), anomalous skip (silent slot - possibly
// the engine skipping redundant CB uploads on static sky frames), or the
// pv race. Latched once per missed frame at the first fallback mesh.
static uint64_t g_ms_frames = 0;         // frames carried by the fallback
static uint64_t g_ms_flush_serial = 0;   // once-per-flush latch key
static uint64_t g_ms_ms = 0;             // stamp (age at dump)
// Carried-draw encode forensics (permanent flicker safety): the near of
// the flush's chosen pv on the last carried frame. Camera-class (~0.8-1.3)
// is healthy; 0.01/10-class convicts an encode poison (both classes were
// convicted and fixed in the artifact campaign - this is the tripwire).
static float g_ms_near = -1.0f;          // -m32/m22 of the flush's chosen pv

inline void skybind_release() {
    for (int i = 0; i < 16; ++i) {
        KH_SAFE_RELEASE(g_skybind.staging[i]);
        g_skybind.bytes[i] = 0;
    }
    g_skybind.pending = 0;
}

inline void skybind_step(ID3D11DeviceContext* ctx) {
    // Phase one: harvest last frame's copies.
    for (int i = 0; i < 16; ++i) {
        if (!(g_skybind.pending & (1u << i)) || !g_skybind.staging[i]) continue;
        D3D11_MAPPED_SUBRESOURCE ms = {};
        const HRESULT hr = ctx->Map(g_skybind.staging[i], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &ms);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) continue;   // retry next frame
        g_skybind.pending &= static_cast<uint16_t>(~(1u << i));
        if (FAILED(hr) || !ms.pData) continue;
        g_skybind_reads++;
        const float* f = static_cast<const float*>(ms.pData);
        const uint32_t nf = g_skybind.bytes[i] / 4;

        if (nf >= 72 && locator_sky_anchor(f, nf)) {
            g_skybind_hits++;
            g_sky_probe.floats = nf;
            g_sky_probe.valid = true;
            g_sky_probe.hits++;
            g_sky_probe.last_confirm = effect_time_seconds();
            locator_capture(g_sky_probe, f, nf, 96);
        }

        // VS-side windows carry the world draws' VIEW matrix: feed the
        // same scan the upload hooks feed - ring admission when
        // unlocked, lock MAINTENANCE when the window covers the locked
        // location. Upload-dark sessions can no longer kill the lock.
        if (i >= 8 && g_skybind.src[i]) {
            g_viewbind_scans++;
            shadow_view_scan(g_skybind.src[i], f, g_skybind.bytes[i], g_skybind.base_fl[i]);
        }

        ctx->Unmap(g_skybind.staging[i], 0);
    }

    // Phase two: stage fresh copies. Two independent wants: a stale sky
    // mirror (fog work) or an unlocked view (the harvest). Idles fully
    // when both are satisfied.
    const bool want_sky_stage = !(g_sky_probe.valid &&
        effect_time_seconds() - g_sky_probe.last_confirm < 5.0f);
    const bool want_view_stage = !g_ls.view_src_valid;
    if (!want_sky_stage && !want_view_stage) return;

    // BOTH shader stages (the 416-byte lesson: at 8x sampling density no
    // PS constant buffer over 416 bytes ever appeared - this D3D9-hearted
    // engine computes fog in the VERTEX shaders for the live scene's
    // materials, matching the VS family the export's fog-shape grep
    // flagged; the sky CB binds at VS. Staging slots 0-7 mirror the PS
    // stage, 8-15 the VS stage.)
    ID3D11Buffer* bufs[16] = {};
    UINT first16[16] = {};   // in 16-byte constants
    UINT num16[16] = {};
    ID3D11DeviceContext1* ctx1 = nullptr;
    ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1));

    if (ctx1) {
        ctx1->PSGetConstantBuffers1(0, 8, bufs, first16, num16);
        ctx1->VSGetConstantBuffers1(0, 8, bufs + 8, first16 + 8, num16 + 8);
        g_skybind_off1 = 1;
    } else {
        ctx->PSGetConstantBuffers(0, 8, bufs);
        ctx->VSGetConstantBuffers(0, 8, bufs + 8);
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);

    for (int i = 0; i < 16; ++i) {
        if (!bufs[i]) continue;

        // VS slots: never at the injection (our own CB is bound there -
        // the never-self-learn rule), and only while the view wants
        // feeding; PS slots only while the sky wants feeding.
        const bool stage_ok = i < 8 ? want_sky_stage
                                    : (want_view_stage && !g_ro.in_injection);

        if (stage_ok && dev && !(g_skybind.pending & (1u << i))) {
            D3D11_BUFFER_DESC bd = {};
            bufs[i]->GetDesc(&bd);
            g_skybind_slots |= (1u << i);

            if (i < 8) {
                if (g_skybind_minbw == 0 || bd.ByteWidth < g_skybind_minbw) g_skybind_minbw = bd.ByteWidth;
                if (bd.ByteWidth > g_skybind_maxbw) g_skybind_maxbw = bd.ByteWidth;
            } else {
                if (bd.ByteWidth > g_skybind_maxbw_vs) g_skybind_maxbw_vs = bd.ByteWidth;
            }
            if (first16[i] > 0) g_skybind_offs_seen++;

            const uint32_t off_b = first16[i] * 16u;   // zero on the 11.0 path
            const uint32_t span_b = num16[i] > 0 ? num16[i] * 16u
                                                 : (bd.ByteWidth > off_b ? bd.ByteWidth - off_b : 0u);

            // no size cap: pools are legitimate now - the WINDOW is small
            if (span_b >= 288 && off_b + 288 <= bd.ByteWidth) {
                uint32_t win = span_b < 4096u ? span_b : 4096u;
                if (off_b + win > bd.ByteWidth) win = bd.ByteWidth - off_b;

                if (g_skybind.staging[i] && g_skybind.bytes[i] != win) {
                    KH_SAFE_RELEASE(g_skybind.staging[i]);
                    g_skybind.bytes[i] = 0;
                }

                if (!g_skybind.staging[i]) {
                    D3D11_BUFFER_DESC sd = {};
                    sd.ByteWidth = win;
                    sd.Usage = D3D11_USAGE_STAGING;
                    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

                    if (SUCCEEDED(dev->CreateBuffer(&sd, nullptr, &g_skybind.staging[i]))) {
                        g_skybind.bytes[i] = win;
                    }
                }

                if (g_skybind.staging[i] && g_skybind.bytes[i] == win) {
                    g_skybind.src[i] = bufs[i];
                    g_skybind.base_fl[i] = off_b / 4u;

                    if (win == bd.ByteWidth && off_b == 0) {
                        ctx->CopyResource(g_skybind.staging[i], bufs[i]);
                    } else {
                        D3D11_BOX box = {};
                        box.left = off_b;     // bytes for buffers
                        box.right = off_b + win;
                        box.bottom = 1;
                        box.back = 1;
                        ctx->CopySubresourceRegion(g_skybind.staging[i], 0, 0, 0, 0,
                                                   bufs[i], 0, &box);
                    }

                    g_skybind.pending |= static_cast<uint16_t>(1u << i);
                }
            }
        }

        bufs[i]->Release();   // PSGetConstantBuffers adds a reference
    }

    if (dev) dev->Release();
    if (ctx1) ctx1->Release();
}

static uint64_t g_loc_scan_uploads = 0;    // uploads examined inside attempt windows
static uint32_t g_loc_max_cb_floats = 0;   // largest funnel CB seen (uncapped size)

// The structural anchor: f[i] == staged fog decay (the engine consumes it
// raw - measured mid-transition at 0.5% of the staged value) and
// f[i+12] == -(render camera altitude), the injection-recorded g_ls.cam.
inline bool locator_light_anchor(const float* f, uint32_t i, uint32_t nf, float decay, float cam_alt) {
    if (i + 16 > nf) return false;
    if (fabsf(f[i] - decay) > 1.0e-4f + 5.0e-3f * decay) return false;
    if (fabsf(f[i + 12] + cam_alt) > 2.5f) return false;
    return true;
}

// Planted-light position at f[i..i+2]: world or camera-relative, 0.75 m
// per lane. The relative test stands down when the plant sits too close
// to the camera (a near-zero relative triple would anchor on the world's
// most common float) - the procedure plants the light a few meters out.

// meta is the CALLER's field now (the lamp probe stores the match space
// there; v1 of this helper silently clobbered it with off - nb_base).
inline void locator_capture(CbColorProbe& pr, const float* f, uint32_t nf, uint32_t win) {
    if (win > 96) win = 96;

    if (nf <= win) {
        pr.nb_base = 0;   // small CB: the whole block, head included
    } else {
        pr.nb_base = pr.off >= 40 ? pr.off - 40 : 0;
        if (pr.nb_base + win > nf) pr.nb_base = nf - win;
    }

    const uint32_t n = nf - pr.nb_base < win ? nf - pr.nb_base : win;
    memcpy(pr.nb, f + pr.nb_base, n * sizeof(float));

    // Sky-probe refreshes also update the engine-sun reference (floats
    // 8-10 of the sky block) for the jump arbiter defined above.
    if (&pr == &g_sky_probe && pr.nb_base == 0 && n >= 11) {
        const float sl = sqrtf(pr.nb[8] * pr.nb[8] + pr.nb[9] * pr.nb[9] +
                               pr.nb[10] * pr.nb[10]);

        if (sl > 0.5f && sl < 2.0f) {
            g_skysun_ref[0] = pr.nb[8] / sl;
            g_skysun_ref[1] = pr.nb[9] / sl;
            g_skysun_ref[2] = pr.nb[10] / sl;
            g_skysun_ref_valid = true;
        }
    }
}


static float g_shadow_map_strength = 1.0f;    // 0 disables the map term
static float g_shadow_map_bias = 0.0015f;     // in the transform's depth units
static float g_shadow_map_sign = 1.0f;        // +1 standard depth, -1 reversed
// --- Private sun-depth map state (render thread writes at its pass; the
//     game-thread flush reads under the park invariant, like everything
//     else here). vp/bias/bounds are only meaningful while valid; both
//     flags reset at the main depth clear so a stale sun matrix is never
//     served. ---
static bool  g_sun_map_rendered_frame = false; // this frame's pass already ran
static bool  g_sun_map_valid = false;          // pass produced a usable map
static float g_sun_map_time = -1.0f;           // when it was produced (the fire
                                               // consumes LAST frame's map; 0.25 s
                                               // staleness bounds fallback frames)
static bool  g_sun_map_no_local = false;       // casters exist, none within the fit
                                               // radius: shadows are OFF by the
                                               // distance rule, NOT broken - the
                                               // slab fallback must not fire
static float g_sun_map_vp[16] = {};            // row-vector world -> sun clip
static float g_sun_map_bias = 0.0f;            // compare bias, normalized depth units
static uint64_t g_sun_map_hash = 0;            // last rendered input hash (unchanged-input skip)
static uint64_t g_sun_map_skips = 0;           // passes skipped on identical inputs (perf forensics)
static float g_sun_map_bounds[6] = {};         // combined caster AABB: center xyz, HALF extents xyz (engine axes)
// FROZEN FIRE INPUTS: latched at the frame's FIRST fire, replayed by
// every same-frame re-fire. Re-fires with per-batch inputs produced the
// photographed ghost (different reconstructions unioned under MIN);
// a single fire left later partition batches erasing the shadow (the
// look-down disappearance). Identical replayed writes solve both:
// idempotent under MIN, and every batch is re-shadowed.
static ID3D11ShaderResourceView* g_fire_lock = nullptr;
static float g_fire_fov2[2] = {};
static float g_fire_dims2[2] = {};
static float g_fire_view2[16] = {};   // COMPLETE freeze: the view was read
static float g_fire_sun2[3] = {};     // LIVE per re-fire while it re-publishes
                                      // continuously through the frame - re-fires
                                      // disagreed by the mid-frame camera delta:
                                      // microscopic in person view (the residual
                                      // jitter), large under Zeus's fast motion
                                      // (the double box). Frozen view + sun make
                                      // every re-fire byte-identical: truly
                                      // MIN-idempotent, while per-batch re-firing
                                      // continues (no look-down disappearance).
static bool  g_fire_lock_valid = false;
// The per-frame frozen replay below is the accepted design (a sticky
// cross-frame texture lock cold-latches the wrong partition: falsified).

static float g_sun_local_bounds[16][6] = {};   // up to 16 in-map casters: center xyz + half extents xyz (engine)
static int   g_sun_local_count = 0;            // valid entries above (render thread, like the rest)

static bool g_mask_cast_arm = false;
static uint64_t g_cast_frozen_fires = 0;   // frozen-replay fallback fires
static uint64_t g_rt_resolve_true = 0;     // rt_is_resolve verdicts at the sweep
static uint64_t g_rt_resolve_false = 0;
static uint64_t g_rt_half_accepts = 0;     // half-res resolve acceptances (fog economy)
static uint64_t g_sweep_gap_resets = 0;    // settle resets from capture-stream gaps
static uint32_t g_rt_last_rej_w = 0;       // width of the last REJECTED resolve target
static bool g_mask_cast_fired = false;   // one analytic pass per frame:
// the engine re-uploads its view ~240x/frame and firing at every publish
// stacked 240 fullscreen multiplies (shade^240 -> mask annihilated,
// frame time doubled). analyticCasts == frameViewHits was the tell.

inline bool shadow_live_wanted() {
    // g_sun_valid is deliberately NOT a term anymore: the live capture is
    // what PRODUCES the derived direction, so gating capture on direction
    // validity would deadlock the warm-up. Direction-dependent consumers
    // all gate individually (lighting1.w, kh_shadow_sun's nullptr).
    return g_ls.wanted.load(std::memory_order_relaxed) && g_shadow_map_strength > 0.0f;
}

// Fills the lighting slots of a draw constant block from an object and the
// published sun state. Called by both solid-mesh paths (flush and injection);
// both run either on the game thread with the lock held or on the parked-
// exclusive render thread, so plain reads of the published globals are safe.
inline void fill_lighting_cb(ConstantData& cbd, const RenderObject& o) {
    cbd.lighting0[0] = o.lit ? 1.0f : 0.0f;
    cbd.lighting0[1] = 1.0f;   // unused lane (float4 packing)
    cbd.lighting0[2] = o.light_ambient;
    cbd.lighting0[3] = o.light_diffuse;
    // Direction: the published derived latch, the single source now (the
    // getLighting-era ~9-deg-divergence note is retired with it; see
    // kh_shadow_sun's sunset lesson - the divergence was the engine
    // clamping its shadow light, and the derivation measures that).
    cbd.lighting1[0] = g_sun_dir_engine[0];
    cbd.lighting1[1] = g_sun_dir_engine[1];
    cbd.lighting1[2] = g_sun_dir_engine[2];
    cbd.lighting1[3] = g_sun_valid ? 1.0f : 0.0f;
    // Colors: the located lighting block is the ONLY source (getLighting
    // retired). Ever-locked -> the mirrored lanes (fresh or last-known:
    // the block drifts slowly and the mirror survives expiry); never
    // locked (sub-second cold) -> flat white sun over scalar ambient,
    // which the direction warm-up makes near-unreachable in practice.
    if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
        cbd.lighting2[0] = g_light_probe.nb[16];
        cbd.lighting2[1] = g_light_probe.nb[17];
        cbd.lighting2[2] = g_light_probe.nb[18];
        cbd.light_amb[0] = g_light_probe.nb[8];
        cbd.light_amb[1] = g_light_probe.nb[9];
        cbd.light_amb[2] = g_light_probe.nb[10];
        cbd.light_amb[3] = 1.0f;
    } else {
        cbd.lighting2[0] = 1.0f;
        cbd.lighting2[1] = 1.0f;
        cbd.lighting2[2] = 1.0f;
        cbd.light_amb[0] = 1.0f;
        cbd.light_amb[1] = 1.0f;
        cbd.light_amb[2] = 1.0f;
        cbd.light_amb[3] = 0.0f;
    }
    cbd.lighting2[3] = g_shadow_map_strength;

    // Private sun-depth map (self term here; PSMaskCast consumes the same
    // matrix for the world cast). Zeroed sunMeta = consumers stand down.
    if (g_sun_map_valid) {
        memcpy(cbd.sun_vp, g_sun_map_vp, sizeof(cbd.sun_vp));
        cbd.sun_meta[0] = 1.0f;
        cbd.sun_meta[1] = static_cast<float>(KH_SUN_DEPTH_SIZE);
        cbd.sun_meta[2] = g_sun_map_bias;
        cbd.sun_meta[3] = g_shadow_map_strength;
    }

    // Cascade table, finest scale first (first containing tile decides in
    // the shader). The PUBLISHED table is used - the engine renders its
    // cascades BEFORE the main depth clear, so the building table is
    // already reset by injection time; the published one is this frame's
    // completed set (or at worst last frame's, which its embedded camera
    // origin keeps correct). Skipped entirely for unlit objects.
    if (!o.lit) return;

    // Received-term outage forensics: the world-shadows-on-mesh term
    // vanishes whenever this early-out fires on a LIT object. Three
    // candidate mechanisms (sun-jump wipe, degraded-lock publication
    // rejection window, empty publish) - the split names the one that
    // actually starves it in the field.
    if (g_ls.count == 0 || g_ls.newest < 0) {
        g_recv_term_skips++;
        return;
    }

    uint32_t order[KH_LIVE_MAX_CASCADES];
    // ALL VALID CASCADES, FINEST FIRST (the quality root + the circle
    // artifact, one fix): the newest-only consumption shaded every
    // receiver from whichever cascade rendered LAST - often a coarse
    // quadrant where a trunk is two texels wide and one outlier texel
    // blooms into a circle-with-dot under the bilinear footprint (the
    // artifact survived the tile-interior clamp, acquitting neighbor
    // bleed and convicting coarse content). The tile fractions prove
    // the cascades render into SEPARATE quadrant rects whose contents
    // COEXIST - each entry stays true to its quadrant until its own
    // re-render, which relatches it - so the engine-equal selection is
    // safe: finest containing tile wins, exactly like the engine's own
    // receiver. (The old newest-only note assumed a shared full-atlas
    // rect; the per-entry tiles refute that.)
    uint32_t n = 0;
    float owpt[KH_LIVE_MAX_CASCADES];

    for (uint32_t i = 0; i < g_ls.count && n < KH_LIVE_MAX_CASCADES; ++i) {
        const LiveShadowEntry& le = g_ls.entries[i];
        const float ilen = sqrtf(le.m[0] * le.m[0] + le.m[4] * le.m[4] + le.m[8] * le.m[8]);
        const float texels = le.tile[2] > le.tile[0]
                           ? (le.tile[2] - le.tile[0]) * static_cast<float>(g_ls.atlas_size)
                           : 1.0f;
        const float wpt = ilen > 1e-9f ? (2.0f / ilen) / (texels > 1.0f ? texels : 1.0f) : 1e9f;

        uint32_t j = n;
        while (j > 0 && owpt[j - 1] > wpt) {
            owpt[j] = owpt[j - 1];
            order[j] = order[j - 1];
            --j;
        }

        owpt[j] = wpt;
        order[j] = i;
        ++n;
    }

    if (n == 0) return;

    // The fine copy, when live, samples FIRST (index 0 -> t2); the entries
    // selected above follow as the wide-area fallback on the live atlas.
    // shadow_meta[0] (the count) is written LAST, after both parts land.
    uint32_t out_i = 0;
    cbd.shadow_src[0] = -1.0f;
    cbd.shadow_fix2[0] = 1.0f;   // identity rotation unless the copy path sets it

    cbd.shadow_meta[1] = g_shadow_map_sign;
    cbd.shadow_meta[2] = g_shadow_map_bias;
    cbd.shadow_meta[3] = static_cast<float>(g_ls.atlas_size);
    cbd.shadow_meta2[0] = o.far_vis ? 1.0f : 0.0f;   // far-visibility clamp (see far_vis)
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
// affine-SHAPED window pushes its z-axis
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
        const float t[3] = { w[12], w[13], w[14] };
        consider(shadow_axis_abs_cos(w[2], w[6], w[10]), t);
    }

    // Transposed upload (column_major HLSL - equivalently a column-vector
    // convention stored row-major): affine row at w[12..15], z axis
    // (w[8], w[9], w[10]), translation column (w[3], w[7], w[11]).
    if (proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[14]) &&
        fabsf(w[15] - 1.0f) < 1e-4f) {
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
// Known atlas SRV pointers, memoized WITHIN a frame only: the engine
// rebinds the atlas SRV many times per frame, and the cache turns all but
// the first check into a pointer compare. Cleared at every frame boundary
// (shadow_live_frame_reset) and on atlas adoption, so a destroyed view
// whose address gets reused by an unrelated SRV can never inherit a stale
// classification beyond the frame it died in - the same reuse hazard that
// bans long-lived pointer memoization everywhere else in this machinery.
// Relearning costs one GetResource per pass segment per frame.
static void* g_atlas_srv_cache[8] = {};
static uint32_t g_atlas_srv_count = 0;

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
    // GROW-OR-RECOVER adoption. 'Keep the largest' alone made the hold a
    // one-way latch: a mid-session shadow-quality change RECREATES the
    // engine atlas, and a same-size-or-smaller successor could never
    // displace the held one - a dead texture kept alive while every
    // resolve stopped matching it (cast and receive quietly starve). The
    // engine names its live atlas every frame by consuming it at t15
    // (atlas_last_seen, stamped in mask_note_draw): a held atlas unseen
    // for 3 s is dead, and any qualifying candidate may displace it.
    const float now_at = effect_time_seconds();
    const bool held_dead = g_ls.atlas_tex && (now_at - g_ls.atlas_last_seen > 3.0f);
    if (g_ls.atlas_tex && td.Width <= g_ls.atlas_size && !held_dead) return;   // keep the largest LIVE atlas

    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); }
    tex->AddRef();
    g_ls.atlas_tex = tex;

    g_ls.atlas_identity = id;
    g_ls.atlas_size = td.Width;
    g_ls.atlas_fmt = static_cast<uint32_t>(td.Format);
    g_ls.atlas_last_seen = now_at;   // adoption grace: displaceable only after 3 s unseen
    g_ls.srv_failed = false;
    g_ls.cache_valid = false;   // new atlas: relearn the upload locator
    g_atlas_srv_count = 0;      // cached SRVs belong to the OLD atlas

    // BAND DESC REBUILD: the frozen band copies are clones of the ATLAS
    // desc, and CopyResource demands exact desc identity - after a swap
    // (quality change: new size or format) every copy into an old-desc
    // clone is silently-dropped invalid API use, serving stale content
    // forever. Drop the textures; band_capture's batch-create rebuilds
    // all eight at the adopted desc on the first reseal.
    for (int b = 0; b < 8; ++b) {
        auto& bb = g_ls.band[b];
        if (bb.srv) { bb.srv->Release(); bb.srv = nullptr; }
        if (bb.tex) { bb.tex->Release(); bb.tex = nullptr; }
        bb.valid = false;
        bb.pending_view = false;
        memset(bb.sm, 0, sizeof(bb.sm));
        memset(bb.border, 0, sizeof(bb.border));
        bb.last_time = 0.0f;
    }
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

inline void shadow_view_scan(ID3D11Resource* res, const void* data, uint32_t bytes, uint32_t base_off = 0) {
    // base_off (floats): nonzero when the data is a WINDOW into the
    // buffer (the binding-readback path) - offsets stored and matched
    // below are canonical buffer offsets, so both supply paths feed one
    // lock. The view-lock deaths were the sky disease striking here:
    // whole sessions where the engine streams the view CB through paths
    // the upload hooks cannot see (best candidate 79 degrees off all
    // session while the injections proved the bridge correct). The
    // binding cannot go dark: the buffer is bound at every world draw.
    if (!reorder_on_render_thread() || bytes < 64 || !res) return;

    // LOCKED FAST PATH: with a source locked, the publish branch requires
    // res == view_src_res and the admission branch requires the lock to be
    // ABSENT - for every foreign resource the whole offset loop is already
    // a guaranteed no-op (~1,600 uploads x 49 compares per frame of pure
    // waste). One pointer compare is the identical no-op, cheaper.
    if (g_ls.view_src_valid && res != g_ls.view_src_res) return;
    const float* f = static_cast<const float*>(data);
    const uint32_t nf = bytes / 4;

    for (uint32_t off = 0; off + 16 <= nf && off <= 192; off += 4) {
        const float* w = f + off;

        if (g_ls.view_src_valid && res == g_ls.view_src_res && off + base_off == g_ls.view_src_off) {
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
                    // Exact-class stamp (the cast gate's recent-exact
                    // input; see pub_exact_ms). Same 1e-3 bar as the
                    // gates - the bar is unchanged, only WHERE it is
                    // evaluated moved.
                    if (e_rot <= 1e-3f) g_ls.pub_exact_ms = steady_now_ms();
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

            // COLD PUBLISH GUARD (the spawn drift), v2: the early view
            // lock can sit on a subtly wrong CB slot (viewBestRot read
            // 0.0157 with a later re-lock in the drifting session) - its
            // publishes track the camera slightly until a better lock
            // wins. For the first 3 s of publishing, a candidate must
            // agree in ROTATION with the live bridge PV or the publish is
            // rejected as a COMPLETE no-op. v1 rejected AFTER frame_view
            // and prev_view were already overwritten (frame_view_time left
            // stale): every rejected candidate still fed the fire's freeze
            // and the receive fallback unvetted - the spawn-window
            // camera-following shadows this guard existed to prevent.
            // Rejection now precedes every publish mutation.
            // MOTION ALLOWANCE: the bridge reference is +-1 frame skewed
            // from the engine's own upload, so the fixed 0.02 starved
            // every publish while the camera turned at spawn (no publish =
            // no cast arm, no seal completion: shadows missing, or
            // swimming on provisional bridge views, until the 3 s window
            // expired - the reported cold symptom, and the same
            // speed-fragility as the retired 0.15 family filter). The
            // rotation delta between this candidate and the last ACCEPTED
            // publish bounds that skew, so the threshold widens by the
            // measured motion (capped) and collapses back to 0.02 the
            // moment the camera rests - the stationary wrong-slot
            // protection is unchanged.
            {
                const float pub_now = effect_time_seconds();
                if (g_pub_first < 0.0f) g_pub_first = pub_now;

                if (pub_now - g_pub_first < 3.0f) {
                    RVExtBridge::ProjectionViewTransform bpv = {};

                    if (RVExtBridge::get_projection_view_transform(bpv)) {
                        float bvv[16];
                        memcpy(bvv, &bpv.view, sizeof(bvv));
                        float rd = 0.0f;
                        float md = 0.0f;   // rotation moved since the last accepted publish

                        for (int rr = 0; rr < 3; ++rr) {
                            for (int cc = 0; cc < 3; ++cc) {
                                rd += fabsf(fv[rr * 4 + cc] - bvv[rr * 4 + cc]);
                                if (g_ls.prev_view_valid) md += fabsf(fv[rr * 4 + cc] - g_ls.prev_view[rr * 4 + cc]);
                            }
                        }

                        if (rd > 0.02f + (md < 0.3f ? md : 0.3f)) {
                            g_ls.cold_pub_rejects++;   // forensic: coldPubRejects

                            // BAD-LOCK RECOVERY: the bridge view is the
                            // arbiter here, and a long reject STREAK
                            // means the LOCK is wrong, not the frame -
                            // the spawn-window regression (lighting and
                            // shadows transformed by a wrong first
                            // lock, 'moving with the camera') held for
                            // seconds until an organic relock. Forty
                            // consecutive rejects (~0.7 s) drops the
                            // lock and the ring re-acquires from the
                            // live harvest within a few frames.
                            if (++g_ls.pub_rej_streak >= 40 && g_ls.view_src_valid) {
                                g_ls.view_src_valid = false;
                                g_ls.vc_n = 0;
                                g_ls.pub_rej_streak = 0;
                                g_view_relock_forced++;
                            }

                            return;   // full no-op: no publish state was touched
                        }

                        g_ls.pub_rej_streak = 0;
                    }
                }
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

                // PAIRING QUALITY GATE (operator directive, the receive
                // half): a publication that is not exact-class must not
                // pair a seal - an off-axis view baked into a frozen band
                // is a PERMANENT receive offset until reseal. Withheld
                // pairings stay pending; the epoch guard below retires
                // them cleanly and the next resolve reseals under a
                // better publication. Absent beats offset, as ever.
                if (g_ls.last_publish_rot_err > 1e-3f) {
                    g_ls.band_bail_quality++;
                    continue;
                }

                // RECEIVE-V2 EPOCH GUARD: a pending seal older than this
                // frame would pair a CROSS-FRAME view - the permanent
                // receive offset (the same epoch-mixing disease the cast
                // had). Invalidate instead; the next resolve reseals with
                // a same-frame pair. This also retires the 100 ms bridge
                // fallback (stale pendings die at 50 ms, before it fires).
                if (g_ls.frame_view_time - bs.pending_since > 0.05f) {
                    bs.valid = false;
                    bs.pending_view = false;
                    continue;
                }

                for (int c = 0; c < 3; ++c) {
                    for (int r = 0; r < 4; ++r) {
                        bs.vcol[c * 4 + r] = fv[r * 4 + c];
                    }
                }

                bs.pending_view = false;
                bs.vcol_bridge = false;
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
                g_ls.vc_ring[i].off = off + base_off;
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

inline void locator_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 64) return;   // the block is 56+ floats
    // never self-learn: our own CBs carry the staged fog and camera too
    if (res == static_cast<ID3D11Resource*>(g_res.composite_cb)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.constant_buffer)) return;

    const float* f = static_cast<const float*>(data);
    const uint32_t nf_full = bytes / 4;
    if (nf_full > g_loc_max_cb_floats) g_loc_max_cb_floats = nf_full;
    const uint32_t nf = nf_full > 4096 ? 4096 : nf_full;

    // References: decay from the staged fog (nonzero even with fog off -
    // RV's default is 0.014), altitude from the injection-recorded render
    // camera (valid once the first injection has run).
    const float decay = g_fog[1];
    const float cam_alt = g_ls.cam[1];
    const bool ref_ok = g_fog_valid && decay > 1.0e-4f && cam_alt != 0.0f;

    // --- locked probe: verify at the known offset, follow ring
    //     relocations, mirror the block on every confirmation ---
    if (g_light_probe.valid && res == g_light_probe.buf && ref_ok) {
        bool ok = locator_light_anchor(f, g_light_probe.off, nf, decay, cam_alt);

        if (!ok) {
            for (uint32_t i = 0; i + 16 <= nf; ++i) {
                if (locator_light_anchor(f, i, nf, decay, cam_alt)) {
                    g_light_probe.off = i;
                    g_light_probe.relocs++;
                    ok = true;
                    break;
                }
            }
        }

        if (ok) {
            g_light_probe.hits++;
            g_light_probe.last_err = fabsf(f[g_light_probe.off + 12] + cam_alt);
            g_light_probe.last_confirm = effect_time_seconds();
            // MODE GATE (the black-box lesson): at altitude (and plausibly
            // heavy overcast) the engine publishes a VARIANT block - same
            // decay/camAlt anchor offsets, different lane semantics (lane
            // 15 reads 2, the 'sun' lanes went (0, 0.29, 0.19) = the green
            // flicker, 'ambient' near-black = the black boxes). Confirms
            // still stamp (the buffer IS the block - no expiry churn), but
            // the MIRROR only refreshes on the standard layout, so every
            // consumer keeps the last-known standard lanes up high.
            const uint32_t mode_i = g_light_probe.off - 25;   // block lane 15

            if (mode_i < nf) g_light_probe.last_mode = f[mode_i];

            if (mode_i < nf && f[mode_i] == 1.0f) {
                locator_capture(g_light_probe, f, nf, 56);
                g_light_probe.meta = static_cast<int>(g_light_probe.off - g_light_probe.nb_base);
            }
        } else {
            g_light_probe.misses++;   // shared/ring CBs make misses normal
        }
    }

    const float now = effect_time_seconds();

    // SKY CAPTURE, VALUE-BASED, FULL-RATE (field conviction, the fog-lag
    // hunt): this block used to live BELOW the attempt-window gate, so
    // despite its own charter ('every anchor-passing upload refreshes
    // the mirror') it actually refreshed at a 12% duty cycle - the
    // mirror froze for up to ~0.88 s each second while the engine's
    // live sky/atmosphere block evolved with the camera, and the mesh
    // fog target (fogSkyCol = mirror rows) snapped to current only when
    // the window reopened: the reported 'fog color takes about a second
    // to lock in place' on every move through fog. Dump signature that
    // convicted it: skyLocAge 0.71 / 0.36 at random moments (uniform in
    // the gap) with lightLocAge 0.011 (the light probe's locked path
    // runs above the gate - the asymmetry named the gate), and
    // skyLocHits/flushes = 1.27 and 1.35 across two sessions - both
    // implying the SAME ~11 anchor-passing uploads/frame at full rate
    // through a 0.12 window. The gate's cost rationale never applied
    // here: the anchor's fast path is three float compares at fixed
    // offsets per nf>=72 upload, and a hit is a 384-byte memcpy
    // (~4 KB/frame at the measured rate). The ring-CB lesson stands
    // unchanged below: identity is meaningless, the mirror is the sole
    // carrier - it is now simply fed at the rate its charter always
    // claimed. (The skybind readback path keeps its >5 s staleness
    // trigger: it remains the upload-dark-session fallback, parked as a
    // follow-up if such a session ever shows this residual again.)
    if (nf >= 72) {
        if (g_sky_probe.valid && now - g_sky_probe.last_confirm > 60.0f) {
            g_sky_probe.valid = false;   // freshness flag only: the mirror and
                                         // hits persist - consumption is
                                         // mirror-based (hits > 0)
        }

        if (locator_sky_anchor(f, nf)) {
            g_sky_probe.buf = res;                 // last host (diagnostic only)
            g_sky_probe.off = 0;
            g_sky_probe.floats = nf_full;
            g_sky_probe.valid = true;
            g_sky_probe.hits++;
            g_sky_probe.last_confirm = now;
            locator_capture(g_sky_probe, f, nf, 96);
        }
    }

    // --- attempt window (~120 ms each second): expiry, then the
    //     unlocked LIGHT scan (hundreds of uploads per attempt,
    //     negligible) ---
    if (now - floorf(now) >= 0.12f) return;

    // TIME-based expiry: a lock survives any miss rate and expires only
    // after 30 s without a confirmation; valid/buf drop but off/meta/nb
    // SURVIVE, so a dump taken mid-relearn still shows the last-known
    // block contents.
    if (g_light_probe.valid && now - g_light_probe.last_confirm > 30.0f) {
        g_light_probe.valid = false;
        g_light_probe.buf = nullptr;
        g_light_probe.misses = 0;
    }

    // (The sky value-based capture that lived here moved ABOVE the gate
    // - see the full-rate ledger at the hoist. The ring-CB lesson it
    // carried moved with it. From here down the window is light-only;
    // locScanUploads counts light-discovery window uploads now.)
    const bool want_light = !g_light_probe.valid && ref_ok;
    if (!want_light) return;
    g_loc_scan_uploads++;

    for (uint32_t i = 0; i + 16 <= nf; ++i) {
        if (locator_light_anchor(f, i, nf, decay, cam_alt)) {
            g_light_probe.buf = res;
            g_light_probe.off = i;
            g_light_probe.floats = nf_full;
            g_light_probe.valid = true;
            g_light_probe.hits = 1;
            g_light_probe.misses = 0;
            g_light_probe.relocs = 0;
            g_light_probe.last_err = fabsf(f[i + 12] + cam_alt);
            g_light_probe.last_confirm = now;
            const uint32_t mode_i2 = g_light_probe.off - 25;   // block lane 15

            if (mode_i2 < nf) g_light_probe.last_mode = f[mode_i2];

            if (mode_i2 < nf && f[mode_i2] == 1.0f) {
                locator_capture(g_light_probe, f, nf, 56);
                g_light_probe.meta = static_cast<int>(g_light_probe.off - g_light_probe.nb_base);
            }
            break;
        }
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

    if (g_ls.pending_valid && bytes < g_ls.pending_bytes) return;   // never downgrade (same size: last upload wins)

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

    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_ls.pending_vp_valid = false;

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
    e.time = effect_time_seconds();
    g_ls.newest = slot;
    g_ls.cycle_latched = true;
    g_stats.shadow_live_latches++;

}

// CASCADE BINDING HARVEST (upload-dark disease, third victim, third
// application of the cure): a session arrived with shadowLiveLatches 0,
// liveAccepts 0 AND every rejection counter 0 - shadow_live_upload never
// received a single cascade-family upload while the general hooks ran
// millions of light-probe confirmations. During atlas phases, the
// cascade matrices are BOUND at the VS of every cascade draw; this
// mini-probe stages those windows and feeds them to the very same
// shadow_live_upload the hooks feed. Idles whenever the table holds any
// entry fresher than three seconds.
struct CascBindProbe {
    ID3D11Buffer* staging[4] = {};
    uint32_t      bytes[4] = {};
    uint8_t       pending = 0;
};
static CascBindProbe g_cascbind;
static uint64_t g_cascbind_scans = 0;   // harvested windows fed to live_upload
static uint32_t g_cascharv_ctr = 0;     // atlas-draw sampling counter

inline void cascbind_release() {
    for (int i = 0; i < 4; ++i) {
        KH_SAFE_RELEASE(g_cascbind.staging[i]);
        g_cascbind.bytes[i] = 0;
    }
    g_cascbind.pending = 0;
}

inline void cascbind_step(ID3D11DeviceContext* ctx) {
    for (int i = 0; i < 4; ++i) {
        if (!(g_cascbind.pending & (1u << i)) || !g_cascbind.staging[i]) continue;
        D3D11_MAPPED_SUBRESOURCE ms = {};
        const HRESULT hr = ctx->Map(g_cascbind.staging[i], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &ms);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) continue;
        g_cascbind.pending &= static_cast<uint8_t>(~(1u << i));
        if (FAILED(hr) || !ms.pData) continue;
        g_cascbind_scans++;
        shadow_live_upload(ms.pData, g_cascbind.bytes[i]);
        ctx->Unmap(g_cascbind.staging[i], 0);
    }

    // Idle while the table is being fed by anyone (uploads included).
    const float now_c = effect_time_seconds();

    for (uint32_t i = 0; i < KH_LIVE_MAX_CASCADES; ++i) {
        const auto& e = g_ls.entries[i];
        if (e.tile[2] > 0.0f && e.stamp != 0 && now_c - e.time < 3.0f) return;
    }

    ID3D11Buffer* bufs[4] = {};
    UINT first16[4] = {};
    UINT num16[4] = {};
    ID3D11DeviceContext1* ctx1 = nullptr;
    ctx->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&ctx1));

    if (ctx1) {
        ctx1->VSGetConstantBuffers1(0, 4, bufs, first16, num16);
    } else {
        ctx->VSGetConstantBuffers(0, 4, bufs);
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);

    for (int i = 0; i < 4; ++i) {
        if (!bufs[i]) continue;

        if (dev && !(g_cascbind.pending & (1u << i))) {
            D3D11_BUFFER_DESC bd = {};
            bufs[i]->GetDesc(&bd);
            const uint32_t off_b = first16[i] * 16u;
            const uint32_t span_b = num16[i] > 0 ? num16[i] * 16u
                                                 : (bd.ByteWidth > off_b ? bd.ByteWidth - off_b : 0u);

            if (span_b >= 64 && off_b + 64 <= bd.ByteWidth) {
                uint32_t win = span_b < 2048u ? span_b : 2048u;
                if (off_b + win > bd.ByteWidth) win = bd.ByteWidth - off_b;

                if (g_cascbind.staging[i] && g_cascbind.bytes[i] != win) {
                    KH_SAFE_RELEASE(g_cascbind.staging[i]);
                    g_cascbind.bytes[i] = 0;
                }

                if (!g_cascbind.staging[i]) {
                    D3D11_BUFFER_DESC sd = {};
                    sd.ByteWidth = win;
                    sd.Usage = D3D11_USAGE_STAGING;
                    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

                    if (SUCCEEDED(dev->CreateBuffer(&sd, nullptr, &g_cascbind.staging[i]))) {
                        g_cascbind.bytes[i] = win;
                    }
                }

                if (g_cascbind.staging[i] && g_cascbind.bytes[i] == win) {
                    if (win == bd.ByteWidth && off_b == 0) {
                        ctx->CopyResource(g_cascbind.staging[i], bufs[i]);
                    } else {
                        D3D11_BOX box = {};
                        box.left = off_b;
                        box.right = off_b + win;
                        box.bottom = 1;
                        box.back = 1;
                        ctx->CopySubresourceRegion(g_cascbind.staging[i], 0, 0, 0, 0,
                                                   bufs[i], 0, &box);
                    }

                    g_cascbind.pending |= static_cast<uint8_t>(1u << i);
                }
            }
        }

        bufs[i]->Release();
    }

    if (dev) dev->Release();
    if (ctx1) ctx1->Release();
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
    // re-renders and replaces them - WITH ONE EXCEPTION, learned from the
    // time-change field test: a cascade-LAYOUT change (sun elevation
    // swings alter the count; 8 -> 5 observed) orphans slots that will
    // never re-render, and they keep testing fragments against the OLD
    // sun's transforms - shadows visibly drift and never heal. Dropping a
    // dead slot degrades that receive to LIT, never to a wrong-sun shadow.
    // Windows differ by refresh cadence: band seals ride the RESOLVE
    // cadence (constant, sub-second) so 1 s of silence is death; live
    // entries ride the cascade RENDER cadence, which the engine slows on
    // static scenes - 3 s keeps a parked camera from bleeding its table
    // (the shadowLiveCascades=0 reading), while a sun JUMP is handled
    // instantly by the boundary wipe, not by expiry.
    const float now_fr = effect_time_seconds();

    // CONDITIONAL expiry (the parked-camera lesson: shadowLiveCascades=0
    // in every static dump): the engine caches static cascade tiles and
    // re-renders NOTHING while the camera is parked, so a pure time
    // window drains a perfectly valid table. An orphaned slot is only
    // diagnosable RELATIVE to activity: if some entry refreshed within
    // the last second, slots silent for 3 s are dead (layout change);
    // if NOTHING refreshed, the scene is static - keep everything (a
    // sun JUMP is still handled instantly by the boundary wipe).
    float freshest = -1e9f;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        if (g_ls.entries[i].time > freshest) freshest = g_ls.entries[i].time;
    }

    const bool table_active = g_ls.count > 0 && now_fr - freshest < 1.0f;
    uint32_t keep = 0;
    int newest = -1;
    uint64_t newest_stamp = 0;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        if (table_active && g_ls.entries[i].time > 0.0f && now_fr - g_ls.entries[i].time > 3.0f) continue;

        if (keep != i) g_ls.entries[keep] = g_ls.entries[i];

        if (g_ls.entries[keep].stamp >= newest_stamp) {
            newest_stamp = g_ls.entries[keep].stamp;
            newest = static_cast<int>(keep);
        }

        ++keep;
    }

    g_ls.count = keep;
    g_ls.newest = newest;

    for (int b = 0; b < 8; ++b) {
        if (g_ls.band[b].valid && now_fr - g_ls.band[b].last_time > 1.0f) {
            g_ls.band[b].valid = false;
            g_ls.band[b].pending_view = false;
        }
    }

    g_stats.shadow_live_cascades = g_ls.count;
    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_atlas_srv_count = 0;   // per-frame SRV relearn (see the cache note)
}

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
    bool  rt_fmt_ok = false;           // current RT0 has a mask-family format (any size)
    uint32_t rt_w = 0;                 // current RT0 width
    void* rt_key = nullptr;            // current RT0 resource (weak, for candidate draw counts)
    MaskCandidate cands[8];
    uint32_t cand_count = 0;
    ID3D11Texture2D*          tex = nullptr;   // chosen mask (AddRef held)
    ID3D11ShaderResourceView* srv = nullptr;
    void* chosen_key = nullptr;
    float dims[2] = {};                // chosen mask w, h
    // --- Analytic mask cast: the mesh's shadow drawn INTO the engine's
    // screen-space mask, per-pixel, with the true same-frame view (the
    // supervised latch fires at the main pass, BEFORE the mask is
    // consumed). Depth-reconstruct (Nsight recipe: linear depth t0,
    // vpos = (ndc*PSC_FOV,1)*z), fold to world, ray-vs-AABB toward the
    // sun. No atlas, no cascades, no prediction: jitter is impossible.
    // ENGINE-MASK path (the hybrid): the analytic shadow written into the
    // engine's own R8 resolve output - the field-verified candidate-0
    // texture, discriminated by atlas-sampling evidence - with MIN
    // (darken-only) blending. The ENGINE then colors it with its exact
    // ambient and tonemapping: analytic position, engine-perfect color.
    ID3D11RenderTargetView* engine_mask_rtv = nullptr;   // captured at the gated resolve sweep
    uint64_t mask_rtv_swaps = 0;    // capture-time target CHANGES (>: partitions use separate masks)
    uint64_t cast_batches = 0;      // resolve-batch re-arms
    uint64_t cast_arms_lost = 0;    // re-arms that died unfired at a frame boundary
    uint64_t arms_lost_miss = 0;    // the cast_misses guard code active at the LAST
                                    // lost arm - names the starving gate if the
                                    // castArmsLost census ever grows into a chase
    float    last_fire_fov[2] = {}; // reconstruction inputs at the LAST fire -
    float    last_fire_dims[2] = {};// an overcast-moment dump shows directly
    float    last_fire_rot_err = 0; // whether the fire was fed garbage
                                    // (high resolve cadence: the mask's last batch
                                    // that frame rendered without our shadow)
    // cold chain, reborn lean: seconds from the first mesh-carrying flush to
    // each milestone (-1 until reached), plus the leading rejection
    // streak - the spawn-delay question answered in one log.
    double cold_t0 = -1.0;
    float  cold_first_inject = -1.0f;   // first SUCCESSFUL injection
    float  cold_first_cast = -1.0f;     // first analytic mask write
    uint32_t cold_lead_ambiguous = 0;   // ambiguous rejections before the first injection
    float  cold_first_stage = -1.0f;    // first flush that staged >= 1 mesh for injection
    float  cold_first_trigger = -1.0f;  // first entry into the trigger body (past silent guards)
    uint32_t cold_g_nodsv = 0;          // cold exits: no main DSV this cycle
    uint32_t cold_g_floor = 0;          // cold exits: opaque floor
    uint32_t cold_g_tid = 0;            // cold exits: render thread not yet identified
                                        // (needs the flush to adopt the main depth
                                        // first - a cold-start ordering dependency)
    uint32_t cold_cast_miss = 0;        // sticky: last castMisses code seen while the first cast was pending
    float last_inject_near = -1.0f;     // the near plane of the LAST injection (slice forensics)
                                        // x = band near, y = band far, z = fade (the
                                        // doc's f198, named at last), w = captured
    // overlay trisection gauges (per latest flush, not cumulative)
    uint32_t ov_listed = 0;    // mode-2 solids in the flush's mesh list
    uint32_t ov_skipped = 0;   // skipped by the comp_healthy gate (must stay 0 for mode-2)
    uint32_t ov_drawn = 0;     // Draw() actually issued
    ID3D11BlendState*       min_blend = nullptr;         // out = min(dst, src)
    bool engine_mask_failed = false;
    ID3D11ShaderResourceView* cast_depth = nullptr;   // scene linear depth (AddRef)
    float cast_dims[2] = {};
    float cast_fov[4] = {};
    bool  cast_fov_valid = false;
    // Sweep-settle stream: the fire's reconstruction inputs (fov, view,
    // linear depth, mask target) are only trustworthy while the capture
    // stream is CONTINUOUS. Shadow-distance re-engages, time skips and
    // sun<->moon swaps interleave transitional passes whose captures
    // misregister the reconstruction - the boundary overcast flash, the
    // offset shadow on return, the two-direction duplicates.
    float    sweep_last_time = -1.0f;   // last gated capture
    uint32_t sweep_settle = 0;          // coherent sweeps since a discontinuity
    uint32_t sweep_need = 3;            // sweeps required before firing: 3 after
                                        // an fov blip, 10 after a capture GAP
                                        // (a boundary re-engage runs transitional
                                        // passes for several sweeps - the
                                        // 'overcast only when crossing the
                                        // boundary facing away' signature)
    ID3D11DepthStencilState* cast_dss = nullptr;      // depth off
    bool  cast_states_failed = false;
    uint64_t analytic_casts = 0;
    // SQF-set params (game thread writes, plain floats)
    // Mask mode is the DEFAULT shadow path: candidate 0 in the field test
    // (3840x2160 R8_UNORM, channel 0) proved to be the engine's shadow
    // resolve, and sampling it at each pixel's own screen position is
    // registration-exact by construction. The cascade path remains the
    // automatic fallback whenever no mask candidate exists yet.
    float enable = 1.0f;
    float index = -1.0f;               // -1 = auto
    // SRV-failure blacklist: a failed candidate is SKIPPED; the mode stays
    // alive. (One transient cold failure used to zero 'enable' permanently
    // - shadowSrvFailed=1 in every field log - silently demoting every
    // session to the band fallback, which is exactly the path that can
    // drift on sun changes. The mask path is the registration-exact one
    // and must survive one bad candidate.)
    void*    failed_keys[4] = {};
    uint32_t failed_count = 0;
    uint32_t fail_fmt = 0;             // last failed candidate's DXGI format (diagnostic:
                                       // the recurring shadowSrvFailed=1 needs a name)
    float channel = 0.0f;
    float invert = 0.0f;
};

static ShadowMaskState g_mask;

// ===========================================================================
// DEVICE RESET: release every shadow/mask/fire device object held outside
// Resources, and forget every weak identity the hooks compare against.
// The bridge invokes the reset hook with rendering quiescent - the same
// guarantee g_res.release() has always relied on. Everything here is
// re-learned from live captures on the new device: the atlas re-adopts,
// bands reseal, the view source re-locks, the fire re-freezes. Holding any
// of it across a reset pairs old-device objects with the new immediate
// context (invalid API use) and keeps the dead device alive; stale weak
// identities could silently false-match recycled allocations.
// ===========================================================================
inline void release_shadow_device_state() {
    // --- screen-mask machinery ---
    if (g_mask.srv) { g_mask.srv->Release(); g_mask.srv = nullptr; }
    if (g_mask.tex) { g_mask.tex->Release(); g_mask.tex = nullptr; }
    if (g_mask.engine_mask_rtv) { g_mask.engine_mask_rtv->Release(); g_mask.engine_mask_rtv = nullptr; }
    if (g_mask.cast_depth) { g_mask.cast_depth->Release(); g_mask.cast_depth = nullptr; }
    if (g_mask.min_blend) { g_mask.min_blend->Release(); g_mask.min_blend = nullptr; }
    if (g_mask.cast_dss) { g_mask.cast_dss->Release(); g_mask.cast_dss = nullptr; }
    g_mask.chosen_key = nullptr;
    g_mask.rt_key = nullptr;
    g_mask.rt_is_resolve = false;
    g_mask.atlas_bound = false;
    memset(g_mask.cands, 0, sizeof(g_mask.cands));
    g_mask.cand_count = 0;
    memset(g_mask.failed_keys, 0, sizeof(g_mask.failed_keys));
    g_mask.failed_count = 0;
    g_mask.engine_mask_failed = false;
    g_mask.cast_states_failed = false;
    g_mask.cast_fov_valid = false;
    g_mask.cast_dims[0] = g_mask.cast_dims[1] = 0.0f;
    g_mask.sweep_settle = 0;
    g_mask.sweep_need = 10;          // a reset IS a capture gap
    g_mask.sweep_last_time = -1.0f;

    // --- live capture state ---
    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); g_ls.atlas_tex = nullptr; }
    g_ls.atlas_identity = nullptr;
    g_ls.atlas_size = 0;
    g_ls.atlas_fmt = 0;
    g_ls.atlas_last_seen = 0.0f;
    g_ls.srv_failed = false;
    g_ls.phase_on_atlas = false;
    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_ls.pending_vp_valid = false;
    g_ls.cycle_latched = false;
    g_ls.cache_valid = false;
    g_ls.count = 0;
    g_ls.newest = -1;
    g_ls.resolve_window = 0;
    g_ls.resolve_seen_since_cast = false;

    for (int b = 0; b < 8; ++b) {
        auto& bb = g_ls.band[b];
        if (bb.srv) { bb.srv->Release(); bb.srv = nullptr; }
        if (bb.tex) { bb.tex->Release(); bb.tex = nullptr; }
        bb.valid = false;
        bb.pending_view = false;
        memset(bb.sm, 0, sizeof(bb.sm));
        memset(bb.border, 0, sizeof(bb.border));
        bb.last_time = 0.0f;
    }

    for (uint32_t i = 0; i < 16; ++i) {
        g_ls.cb_reg[i].buf = nullptr;    // weak keys into a dead device's allocations
        g_ls.cb_reg[i].floats = 0;
    }

    memset(g_ls.vc_ring, 0, sizeof(g_ls.vc_ring));
    g_ls.vc_n = 0;
    g_ls.view_src_res = nullptr;
    g_ls.view_src_valid = false;
    g_ls.view_src_miss = 0;
    g_ls.frame_view_valid = false;
    g_ls.frame_view_time = -1.0f;
    g_ls.prev_view_valid = false;
    g_ls.view_published_this_frame = false;

    // --- frozen fire + private sun map ---
    if (g_fire_lock) { g_fire_lock->Release(); g_fire_lock = nullptr; }
    g_fire_lock_valid = false;
    g_mask_cast_arm = false;
    g_mask_cast_fired = false;
    g_sun_map_valid = false;
    g_sun_map_rendered_frame = false;
    g_sun_map_no_local = false;
    g_sun_map_hash = 0;
    g_sun_local_count = 0;

    // --- weak identities the render hooks compare against ---
    g_main_depth_identity = nullptr;
    g_atlas_srv_count = 0;
    g_proj_locator = ProjLocator{};
    for (int p = 0; p < 8; ++p) g_proj_pending[p] = ProjPendingMap{};
    g_light_probe = CbColorProbe{};
    g_sky_probe = CbColorProbe{};
    skybind_release();
    cascbind_release();
}

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
        // is the mesh itself - it renders pixel-registered to the world
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
            // shadow-less mesh at spawn.
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
        b.vcol_bridge = !fv;
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
                // HYSTERESIS LATCH: each cascade refit measures a
                // fractionally different direction; adopting every sample
                // made the cast follow the measurement noise - the drift
                // that grew with camera spinning (refit churn). Adopt
                // only when the candidate moves beyond ~0.25 deg of the
                // latched value: noise freezes, real sun motion (and
                // jumps) still tracks in imperceptible steps.
                const float dd = fabsf(wdir[0] - g_sun_dir_derived[0]) +
                                 fabsf(wdir[1] - g_sun_dir_derived[1]) +
                                 fabsf(wdir[2] - g_sun_dir_derived[2]);

                if (g_sun_derived_samples < 1000000) g_sun_derived_samples++;

                if (!g_sun_dir_derived_valid) {
                    // MEASUREMENT SEED (the engine-sun seed retired with
                    // getLighting): the latch starts at the first measured
                    // direction, and the 120-sample validity gate holds
                    // every consumer off until the glide has converged -
                    // nobody sees the early junk, so nothing can slew.
                    g_sun_dir_derived[0] = wdir[0];
                    g_sun_dir_derived[1] = wdir[1];
                    g_sun_dir_derived[2] = wdir[2];
                } else if (dd > 0.05f && g_sun_derived_samples > 120) {
                    // (snap suppressed during warm-up: seed-to-measured
                    // distance can exceed the snap bar at low sun, and
                    // snapping there IS the spawn step)
                    // Big move (a jump): snap.
                    g_sun_dir_derived[0] = wdir[0];
                    g_sun_dir_derived[1] = wdir[1];
                    g_sun_dir_derived[2] = wdir[2];
                } else if (dd > 0.004f) {
                    // Real-but-small motion: GLIDE 25% per sample instead
                    // of hopping the full 0.25-deg step - the discrete
                    // adoptions were a visible per-hop shadow jump under
                    // refit churn (the 'jitters even initially' report).
                    g_sun_dir_derived[0] += (wdir[0] - g_sun_dir_derived[0]) * 0.25f;
                    g_sun_dir_derived[1] += (wdir[1] - g_sun_dir_derived[1]) * 0.25f;
                    g_sun_dir_derived[2] += (wdir[2] - g_sun_dir_derived[2]) * 0.25f;
                    const float gl = sqrtf(g_sun_dir_derived[0] * g_sun_dir_derived[0] +
                                           g_sun_dir_derived[1] * g_sun_dir_derived[1] +
                                           g_sun_dir_derived[2] * g_sun_dir_derived[2]);

                    if (gl > 1e-6f) {
                        g_sun_dir_derived[0] /= gl;
                        g_sun_dir_derived[1] /= gl;
                        g_sun_dir_derived[2] /= gl;
                    }
                }

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
    if (!shadow_live_wanted() || !g_ls.atlas_tex) return;
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
    {   // resolve-window gate + per-band capture (the pair-mode selector is retired: bands ARE the design)
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
                        // HALF-RES resolve acceptance: under heavy fog the
                        // engine economizes its shadow mask to half res and
                        // the 1280 width bar rejected it for 74,941 straight
                        // batches - stale pairing, the overcast, absent
                        // casts. Safe here because the sweep itself already
                        // runs only when the ATLAS was just seen in PS SRVs
                        // (atlas_bound, consumed at entry): the target is
                        // the atlas-sampling pass's RT by construction; the
                        // AO/cloud stomp the width bar guarded against
                        // cannot reach this site.
                        const bool rt_resolve_ok = g_mask.rt_is_resolve ||
                            (g_mask.rt_fmt_ok && g_mask.rt_w >= 620);

                        if (rt_resolve_ok) g_rt_resolve_true++;
                        else { g_rt_resolve_false++; g_rt_last_rej_w = g_mask.rt_w; }
                        if (rt_resolve_ok && !g_mask.rt_is_resolve) g_rt_half_accepts++;

                        if (rt_resolve_ok) {
                        if (nf >= 180) {
                            // Settle accounting: a gap in the capture
                            // stream (> 0.25 s: the shadow system was off
                            // or out of range) or a >10% fov discontinuity
                            // (re-engage with different geometry; player
                            // zoom also trips this, harmlessly - the hold
                            // is sub-frame at steady sweep cadence) resets
                            // the count. The fire requires 3 settled
                            // sweeps (miss code 52).
                            const float swp_now = effect_time_seconds();
                            const bool swp_gap = g_mask.sweep_last_time < 0.0f ||
                                                 swp_now - g_mask.sweep_last_time > 0.25f;
                            bool swp_reset = swp_gap;

                            if (!swp_reset && g_mask.cast_fov_valid) {
                                swp_reset =
                                    fabsf(f[176] - g_mask.cast_fov[0]) > 0.1f * fabsf(g_mask.cast_fov[0]) + 1e-4f ||
                                    fabsf(f[177] - g_mask.cast_fov[1]) > 0.1f * fabsf(g_mask.cast_fov[1]) + 1e-4f;
                            }

                            if (swp_reset) {
                                g_mask.sweep_settle = 0;
                                // gap penalty 10 -> 5: under fog at cold the
                                // stream is naturally sparse and stacked
                                // gaps deferred the first cast 13-19 s;
                                // five consecutive clean batches remain a
                                // solid settle
                                g_mask.sweep_need = swp_gap ? 5 : 3;
                                if (swp_gap) g_sweep_gap_resets++;
                            }
                            if (g_mask.sweep_settle < 1000) g_mask.sweep_settle++;

                            g_mask.sweep_last_time = swp_now;

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
                                // both proven gates - PLUS an identity
                                // check: the real mask is SINGLE-CHANNEL
                                // and scene-scale. The look-down partition
                                // pass (lastInjectNear elevated in every
                                // overcast session) can satisfy the sweep
                                // signature while binding a DIFFERENT
                                // pooled target; MIN-writing shadows into
                                // an arbitrary buffer darkened the world
                                // wholesale. Wrong identity: keep the
                                // previously proven target instead.
                                bool rtv_mask_like = false;

                                {
                                    ID3D11Resource* mres = nullptr;
                                    rtv0->GetResource(&mres);
                                    ID3D11Texture2D* mtex = nullptr;

                                    if (mres && SUCCEEDED(mres->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&mtex)))) {
                                        D3D11_TEXTURE2D_DESC md = {};
                                        mtex->GetDesc(&md);
                                        const DXGI_FORMAT mf = md.Format;
                                        const bool single =
                                            mf == DXGI_FORMAT_R8_UNORM || mf == DXGI_FORMAT_R8_TYPELESS ||
                                            mf == DXGI_FORMAT_R16_UNORM || mf == DXGI_FORMAT_R16_TYPELESS ||
                                            mf == DXGI_FORMAT_R16_FLOAT || mf == DXGI_FORMAT_R8_UINT;
                                        rtv_mask_like = single && md.Width >= 1280;
                                        mtex->Release();
                                    }

                                    if (mres) mres->Release();
                                }

                                if (rtv_mask_like && g_mask.engine_mask_rtv != rtv0) {
                                    if (g_mask.engine_mask_rtv) {
                                        g_mask.engine_mask_rtv->Release();
                                        g_mask.mask_rtv_swaps++;   // target CHANGED mid-session
                                    }
                                    g_mask.engine_mask_rtv = rtv0;   // keep the ref
                                } else {
                                    rtv0->Release();   // duplicate OR failed identity
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

    g_mask.rt_fmt_ok =
        (td.Format == DXGI_FORMAT_R8_UNORM || td.Format == DXGI_FORMAT_R8_TYPELESS ||
         td.Format == DXGI_FORMAT_R16_UNORM || td.Format == DXGI_FORMAT_R16_TYPELESS ||
         td.Format == DXGI_FORMAT_R16_FLOAT);
    g_mask.rt_w = td.Width;
    g_mask.rt_is_resolve = td.Width >= 1280 && g_mask.rt_fmt_ok;
    tex->Release();
}

inline bool mask_ensure_srv(ID3D11DeviceContext* ctx);   // defined below


// The hybrid: analytic ray-vs-AABB written into the ENGINE'S mask at the
// view-publish moment (after all resolves, before the mask is consumed),
// MIN-blended so it can only darken. No clear - the engine's content
// stays; we contribute our meshes' occlusion and the engine's lighting
// pass colors everything identically.
// ===========================================================================
// Private single-caster sun-depth pass: every eligible mesh, rendered
// depth-only from the sun into our own small DSV, once per frame at the
// injection. Entirely our pipeline - own target, own matrix, no engine
// forensics - which is what makes MESH-SHAPED cast shadows and concave
// SELF-shadowing possible at all: our geometry never enters the engine's
// atlas, and the analytic slab could only ever cast an AABB.
// Runs on the render thread inside a hooked draw; every pipeline stage it
// binds is captured and restored (the Get/Set/Restore rule), and
// in_injection excludes its own draws and its own depth clear from the
// reorder tracking.
// ===========================================================================

inline bool render_sun_depth(ID3D11DeviceContext* ctx) {
    if (g_sun_map_rendered_frame) return g_sun_map_valid;
    g_sun_map_rendered_frame = true;
    // NOTE: validity is NOT cleared here - the previous frame's map stays
    // consumable (the fire runs before this frame's injection renders a
    // new one); a failed render simply leaves the old map to age out of
    // its 0.25 s window.
    g_sun_map_no_local = false;
    if (!ctx) return false;

    // SHADOW GEOMETRY uses the DERIVED sun - the direction measured from
    // the engine's own cascade uploads, i.e. the sun the world's shadows
    // actually follow. getLighting's reported sun skews from it by a
    // variable 1-9 degrees (field-measured; CONTINUATION's old '9 deg
    // skew' note), and casting with the reported sun offset our shadow
    // along the sun axis by exactly that error - the drift that tracked
    // cascade refits and never recovered after a time skip, in BOTH eras.
    const float* sun = kh_shadow_sun();

    if (!sun) return false;

    // CAMERA-LOCAL FIT (the two-box lesson): fitting EVERY caster into one
    // ortho let two distant objects blow the fit up - meter-scale texels
    // (the 'pixelated' casts), a locality reach in the kilometers (the
    // returning overcast) and a meters-long normal offset in the self
    // term. Only casters within KH_SUN_FIT_RADIUS of the camera enter the
    // map; without a camera this frame, everything does (single-cluster
    // sessions are unaffected either way).
    float cam_e[3] = { 0.0f, 0.0f, 0.0f };
    bool cam_valid = false;

    {
        RVExtBridge::ProjectionViewTransform cpv = {};

        if (RVExtBridge::get_projection_view_transform(cpv)) {
            extract_camera_pos(cpv.view, cam_e);
            cam_valid = true;
        } else if (g_ls.cam[0] != 0.0f || g_ls.cam[1] != 0.0f || g_ls.cam[2] != 0.0f) {
            cam_e[0] = g_ls.cam[0];
            cam_e[1] = g_ls.cam[1];
            cam_e[2] = g_ls.cam[2];
            cam_valid = true;
        }
    }

    struct SunCaster { float pos[3]; float size[3]; int mesh; };
    static std::vector<SunCaster> casters;   // render-thread scratch
    casters.clear();

    {
        std::lock_guard<std::mutex> lk(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            const auto& o = kv.second;
            // Same eligibility as the cast snapshot: world-space, visible,
            // non-overlay meshes only.
            if (o.fullscreen || !o.visible || o.mode == DepthMode::Off) continue;

            if (cam_valid) {
                const float ce0[3] = { o.pos[0], o.pos[2], o.pos[1] };
                const float he0[3] = { o.size[0] * 0.5f, o.size[2] * 0.5f, o.size[1] * 0.5f };
                const float dx = ce0[0] - cam_e[0];
                const float dy = ce0[1] - cam_e[1];
                const float dz = ce0[2] - cam_e[2];
                const float hd = sqrtf(he0[0] * he0[0] + he0[1] * he0[1] + he0[2] * he0[2]);
                const float lim = KH_SUN_FIT_RADIUS + hd;

                if (dx * dx + dy * dy + dz * dz > lim * lim) {
                    g_sun_map_no_local = true;   // provisional: cleared below if any caster lands in radius
                    continue;
                }
            }
            SunCaster c;
            memcpy(c.pos, o.pos, sizeof(c.pos));
            memcpy(c.size, o.size, sizeof(c.size));
            c.mesh = (o.mesh >= 0 && o.mesh < KH_MESH_COUNT) ? o.mesh : 0;
            casters.push_back(c);
        }
    }

    if (!casters.empty()) g_sun_map_no_local = false;   // at least one local caster
    if (casters.empty()) return false;

    // UNCHANGED-INPUT SKIP: the map is a pure function of the caster set
    // (camera inclusion already folded into the snapshot) and the derived
    // sun; identical inputs were re-rendering an identical 2048^2 depth
    // pass, its clear and a full state save/restore every frame. Hash
    // both; on a match the previous map IS this frame's map - refresh its
    // clock and keep everything (bounds, VP, locality exports are equally
    // unchanged). Any float delta re-renders. The hash commits ONLY at
    // render success below, so a failed render can never let a stale map
    // vouch for new inputs. (Failure-exonerated re-application: this block
    // sits after the derived-sun guard and provably never executed in the
    // corrupted sessions.)
    uint64_t input_hash = 1469598103934665603ull;

    {
        auto fnv = [&input_hash](const void* p, size_t n) {
            const unsigned char* b = static_cast<const unsigned char*>(p);

            for (size_t i = 0; i < n; ++i) {
                input_hash ^= b[i];
                input_hash *= 1099511628211ull;
            }
        };

        for (const auto& c : casters) {
            fnv(c.pos, sizeof(c.pos));
            fnv(c.size, sizeof(c.size));
            fnv(&c.mesh, sizeof(c.mesh));
        }

        fnv(sun, 12);

        if (g_sun_map_valid && input_hash == g_sun_map_hash) {
            g_sun_map_time = effect_time_seconds();
            g_sun_map_skips++;
            return true;
        }
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    const bool res_ok = ensure_resources(dev).empty() && ensure_sun_depth(dev);
    bool instanced = res_ok && g_res.vs_sundepth && g_res.layout_sundepth;

    if (instanced) {
        // Grow-only dynamic instance buffer: 8 floats per caster
        // (engine-space center, engine-space extents).
        const UINT need = static_cast<UINT>(casters.size());

        if (need > g_res.sun_instance_cap) {
            KH_SAFE_RELEASE(g_res.sun_instance_vb);
            UINT cap = g_res.sun_instance_cap > 64 ? g_res.sun_instance_cap : 64;
            while (cap < need) cap *= 2;
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = cap * sizeof(float) * 8;
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            if (SUCCEEDED(dev->CreateBuffer(&bd, nullptr, &g_res.sun_instance_vb))) {
                g_res.sun_instance_cap = cap;
            } else {
                g_res.sun_instance_cap = 0;
                instanced = false;
            }
        }
    }

    dev->Release();
    if (!res_ok) return false;

    // Combined caster AABB, engine axes (SQF [x, y, zASL] -> [x, zASL, y]).
    float mn[3] = { 1e18f, 1e18f, 1e18f };
    float mx[3] = { -1e18f, -1e18f, -1e18f };

    for (const auto& c : casters) {
        const float ce[3] = { c.pos[0], c.pos[2], c.pos[1] };
        const float he[3] = { c.size[0] * 0.5f, c.size[2] * 0.5f, c.size[1] * 0.5f };

        for (int k = 0; k < 3; ++k) {
            if (ce[k] - he[k] < mn[k]) mn[k] = ce[k] - he[k];
            if (ce[k] + he[k] > mx[k]) mx[k] = ce[k] + he[k];
        }
    }

    const float ctr[3] = { (mn[0] + mx[0]) * 0.5f, (mn[1] + mx[1]) * 0.5f, (mn[2] + mx[2]) * 0.5f };
    const float hf[3] = { (mx[0] - mn[0]) * 0.5f, (mx[1] - mn[1]) * 0.5f, (mx[2] - mn[2]) * 0.5f };
    const float R = sqrtf(hf[0] * hf[0] + hf[1] * hf[1] + hf[2] * hf[2]) + 1.0f;

    // Ortho sun basis (row-vector convention, like every matrix here).
    // The light LOOKS opposite the "toward the light" direction.
    const float f[3] = { -sun[0], -sun[1], -sun[2] };
    float up_hint[3] = { 0.0f, 1.0f, 0.0f };
    if (fabsf(f[1]) > 0.99f) { up_hint[0] = 1.0f; up_hint[1] = 0.0f; }

    float r3[3] = {
        up_hint[1] * f[2] - up_hint[2] * f[1],
        up_hint[2] * f[0] - up_hint[0] * f[2],
        up_hint[0] * f[1] - up_hint[1] * f[0],
    };

    const float rl = sqrtf(r3[0] * r3[0] + r3[1] * r3[1] + r3[2] * r3[2]);
    if (rl < 1e-6f) return false;
    r3[0] /= rl; r3[1] /= rl; r3[2] /= rl;

    const float u3[3] = {
        f[1] * r3[2] - f[2] * r3[1],
        f[2] * r3[0] - f[0] * r3[2],
        f[0] * r3[1] - f[1] * r3[0],
    };

    const float eye[3] = {
        ctr[0] + sun[0] * (R + 1.0f),
        ctr[1] + sun[1] * (R + 1.0f),
        ctr[2] + sun[2] * (R + 1.0f),
    };

    const float D = 2.0f * R + 2.0f;   // view-axis range: casters sit in [1, 2R+1]

    float view[4][4] = {};
    view[0][0] = r3[0]; view[0][1] = u3[0]; view[0][2] = f[0];
    view[1][0] = r3[1]; view[1][1] = u3[1]; view[1][2] = f[1];
    view[2][0] = r3[2]; view[2][1] = u3[2]; view[2][2] = f[2];
    view[3][0] = -(eye[0] * r3[0] + eye[1] * r3[1] + eye[2] * r3[2]);
    view[3][1] = -(eye[0] * u3[0] + eye[1] * u3[1] + eye[2] * u3[2]);
    view[3][2] = -(eye[0] * f[0] + eye[1] * f[1] + eye[2] * f[2]);
    view[3][3] = 1.0f;

    float proj[4][4] = {};
    proj[0][0] = 1.0f / R;
    proj[1][1] = 1.0f / R;
    proj[2][2] = 1.0f / D;   // z in [0, D] -> [0, 1]
    proj[3][3] = 1.0f;

    float lvp[4][4];
    mul_4x4(view, proj, lvp);

    // Compare bias, deliberately SMALL: the world CAST cannot acne (the
    // ground is never in our map, so it never compares against itself),
    // and the SELF term's acne is handled geometrically by the receiver-
    // normal offset + grazing fade. The old 1.5-texel bias scaled with
    // the fit and peter-panned the cast contact by up to half a meter as
    // caster inclusion changed with camera movement - the 'gradually
    // shifting shadow' of the Zeus field test.
    const float texel_world = (2.0f * R) / static_cast<float>(KH_SUN_DEPTH_SIZE);
    const float bias_world = texel_world * 0.5f > 0.01f ? texel_world * 0.5f : 0.01f;

    // --- depth-only pass: full save/restore, own clear, recursion off ---
    const bool prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup backup;
    backup.capture(ctx);
    // Slot-1 IA save/restore is LOCAL to this pass (the only code that
    // binds it): keeping StateBackup itself byte-identical to its
    // pre-refactor single-slot behavior removes a global behavioral diff
    // from every other save/restore user (fire, flush, injection).
    ID3D11Buffer* old_vb1 = nullptr;
    UINT old_vb1_stride = 0, old_vb1_offset = 0;
    ctx->IAGetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    ID3D11RenderTargetView* old_rtvs[4] = {};
    ID3D11DepthStencilView* old_dsv = nullptr;
    ctx->OMGetRenderTargets(4, old_rtvs, &old_dsv);
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);

    ctx->ClearDepthStencilView(g_res.sun_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(0, nullptr, g_res.sun_dsv);

    {
        D3D11_VIEWPORT vp = {};
        vp.Width = static_cast<FLOAT>(KH_SUN_DEPTH_SIZE);
        vp.Height = static_cast<FLOAT>(KH_SUN_DEPTH_SIZE);
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
    }

    ctx->IASetInputLayout(instanced ? g_res.layout_sundepth : g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(instanced ? g_res.vs_sundepth : g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);   // depth only
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rast_sun);
    if (instanced) {
        // ONE constant upload, ONE instance-buffer fill, ONE DrawInstanced
        // per mesh type: object count no longer multiplies map/draw calls,
        // which is what makes the uncapped caster set plausible.
        std::sort(casters.begin(), casters.end(),
                  [](const SunCaster& a, const SunCaster& b) { return a.mesh < b.mesh; });

        bool filled = false;
        D3D11_MAPPED_SUBRESOURCE im = {};

        if (SUCCEEDED(ctx->Map(g_res.sun_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) {
            float* outp = static_cast<float*>(im.pData);

            for (const auto& c : casters) {
                outp[0] = c.pos[0];    // SQF [x,y,zASL] -> engine [x,zASL,y]
                outp[1] = c.pos[2];
                outp[2] = c.pos[1];
                outp[3] = 0.0f;
                outp[4] = c.size[0];   // SQF [x,y,z] sizes -> engine [x,z,y]
                outp[5] = c.size[2];
                outp[6] = c.size[1];
                outp[7] = 0.0f;
                outp += 8;
            }

            ctx->Unmap(g_res.sun_instance_vb, 0);
            filled = true;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};

        if (filled && SUCCEEDED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            ConstantData cbd = {};
            memcpy(cbd.view_proj, lvp, sizeof(cbd.view_proj));
            memcpy(mapped.pData, &cbd, sizeof(cbd));
            ctx->Unmap(g_res.composite_cb, 0);

            ID3D11Buffer* vbs[2] = { nullptr, g_res.sun_instance_vb };
            UINT strides[2] = { sizeof(MeshVertex), sizeof(float) * 8 };
            UINT offsets[2] = { 0, 0 };
            size_t first = 0;

            while (first < casters.size()) {
                const int mid = casters[first].mesh;
                size_t last = first + 1;
                while (last < casters.size() && casters[last].mesh == mid) ++last;
                vbs[0] = g_res.mesh_vb[mid];
                ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);
                ctx->DrawInstanced(mesh_vertex_count(mid),
                                   static_cast<UINT>(last - first), 0,
                                   static_cast<UINT>(first));
                g_stats.sun_depth_casters += last - first;
                first = last;
            }
        }
    } else {
        // Per-caster fallback (instancing unavailable): one constant
        // upload + draw each - correct at any count, just not cheap.
        UINT stride = sizeof(MeshVertex), offset = 0;
        int bound_mesh = -1;

        for (const auto& c : casters) {
            if (c.mesh != bound_mesh) {
                ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[c.mesh], &stride, &offset);
                bound_mesh = c.mesh;
            }

            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) break;
            ConstantData cbd = {};
            memcpy(cbd.view_proj, lvp, sizeof(cbd.view_proj));
            cbd.center_size[0] = c.pos[0];
            cbd.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y]
            cbd.center_size[2] = c.pos[1];
            cbd.size_axes[0] = c.size[0];    // SQF [x,y,z] sizes -> engine [x,z,y]
            cbd.size_axes[1] = c.size[2];
            cbd.size_axes[2] = c.size[1];
            memcpy(mapped.pData, &cbd, sizeof(cbd));
            ctx->Unmap(g_res.composite_cb, 0);
            ctx->Draw(mesh_vertex_count(c.mesh), 0);
            g_stats.sun_depth_casters++;
        }
    }

    ctx->OMSetRenderTargets(4, old_rtvs, old_dsv);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    ctx->IASetVertexBuffers(1, 1, &old_vb1, &old_vb1_stride, &old_vb1_offset);
    KH_SAFE_RELEASE(old_vb1);
    backup.restore(ctx);

    for (int r4 = 0; r4 < 4; ++r4) KH_SAFE_RELEASE(old_rtvs[r4]);

    KH_SAFE_RELEASE(old_dsv);
    g_ro.in_injection = prev_inj;

    // Locality export: up to 16 casters for the fire's per-pixel reach
    // test (engine axes; more than 16 falls back to the combined bounds,
    // which is still capped - just looser).
    g_sun_local_count = 0;

    for (const auto& c : casters) {
        if (g_sun_local_count >= 16) { g_sun_local_count = 0; break; }
        float* lb = g_sun_local_bounds[g_sun_local_count];
        lb[0] = c.pos[0];
        lb[1] = c.pos[2];   // SQF -> engine axes
        lb[2] = c.pos[1];
        lb[3] = c.size[0] * 0.5f;
        lb[4] = c.size[2] * 0.5f;
        lb[5] = c.size[1] * 0.5f;
        g_sun_local_count++;
    }

    memcpy(g_sun_map_vp, lvp, sizeof(g_sun_map_vp));
    g_sun_map_hash = input_hash;   // commit-on-success only (see the skip above)
    g_sun_map_bias = bias_world / D;
    g_sun_map_bounds[0] = ctr[0];
    g_sun_map_bounds[1] = ctr[1];
    g_sun_map_bounds[2] = ctr[2];
    g_sun_map_bounds[3] = hf[0];
    g_sun_map_bounds[4] = hf[1];
    g_sun_map_bounds[5] = hf[2];
    g_sun_map_valid = true;
    g_sun_map_time = effect_time_seconds();
    g_stats.sun_depth_passes++;
    return true;
}

inline void mask_cast_engine(ID3D11DeviceContext* ctx) {
    // cast_misses = FIRST failed guard: 1 arm/fired never satisfied (set
    // once armed, cleared on entry), 3 resources, 41 depth, 42 fov,
    // 43 rtv-failed, 5 view, 51 cold view-quality, 52 unsettled capture
    // stream, 61 ensure_srv, 81 all casters beyond the fit radius
    // (distance rule, not a failure); post-guard:
    // 7 states, 8 no meshes, 9 Map; 0 = full success.
    if (!ctx) return;
    if (g_mask_cast_arm && !g_mask_cast_fired) g_ls.cast_misses = 1;

    // (The NEAR-REGIME GATE lived here through two versions and is
    // RETIRED: v1 at 0.15 and v2 at 5.0 both strangled the cast system -
    // 2.48M and 519k skips, 26 s and 25 s first casts - because the
    // tracked near both moves legitimately with fog density AND spikes
    // transiently during dense-fog pairing churn. The overcast risk it
    // guarded is unmitigated again; the real fix is the per-regime
    // pairing session, whose fingerprints are all collected:
    // lastInjectNear elevation, rearm/ambiguity churn, fireFov regime.
    // castRegimeSkips remains as a frozen forensic.)

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
    // Per-batch re-fire (the accepted configuration): each band batch's
    // mask rewrite is followed by a re-shadowing write. All fires within
    // a frame replay the FROZEN inputs latched at the frame's first fire.
    if (g_mask_cast_fired && g_ls.resolve_seen_since_cast && g_ls.frame_view_valid) {
        g_mask_cast_arm = true;
        g_mask_cast_fired = false;
        g_mask.cast_batches++;
    }

    if (!g_mask_cast_arm || g_mask_cast_fired) return;
    g_ls.resolve_seen_since_cast = false;
    if (!g_res.initialized || !g_res.ps_maskcast || !g_res.vs_fullscreen || !g_res.composite_cb) { g_ls.cast_misses = 3; return; }
    if (!g_mask.cast_depth) { g_ls.cast_misses = 41; return; }
    if (!g_mask.cast_fov_valid) { g_ls.cast_misses = 42; return; }
    if (g_mask.engine_mask_failed) { g_ls.cast_misses = 43; return; }
    if (!g_ls.frame_view_valid) { g_ls.cast_misses = 5; return; }

    // STANDING view-quality gate (operator directive: no cast unless the
    // shadow is PRECISE - delayed shadowing beats broken shadowing). This
    // supersedes the cold-only 0.05 gate: steady state previously had NO
    // quality gate at all, which is how the bar-passing lock sessions
    // (viewBestRot 0.0077 / 0.0119, the off-axis world-drift class) cast
    // drifting shadows for their whole duration. The exact class (1e-3,
    // the exact-lock preference's own threshold) is required ALWAYS; the
    // cold-overcast case is covered a fortiori. On a session that never
    // reaches exact quality, the cast simply never fires - by directive.
    // RECENT-EXACT recalibration (field conviction, one session): the
    // raw per-publication e_rot compares against the +-1-frame-lagged
    // bridge latch, so camera motion breaches 1e-3 on a perfectly
    // healthy lock - castBatches ran at ~43% of resolveHits (8,902 vs
    // 20,856), castArmsLost 2,694->3,978 against a 0-7 norm, with
    // viewBestRot 0.0 the whole session. The bar stays 1e-3; the gate
    // now passes while an exact-class publication exists within the
    // trailing 250 ms (~15 publications at 60 fps - a healthy lock
    // re-proves exactness within a frame or two in every measured
    // session, so jitter passes; an off-axis era never stamps and
    // starves at most 250 ms later than before). The SEAL-PAIRING gate
    // is deliberately unchanged: a seal bakes ONE specific publication
    // into a frozen band, so that publication's own error remains the
    // correct input there.
    if (g_ls.pub_exact_ms == 0 ||
        steady_now_ms() - g_ls.pub_exact_ms > 250) { g_ls.cast_misses = 51; return; }
    // Transition hold: fire only into a SETTLED capture stream (see the
    // sweep_settle note). Steady state saturates this within one frame.
    if (g_mask.sweep_settle < g_mask.sweep_need) { g_ls.cast_misses = 52; return; }
    // Falsified at this site: a hard fire-gate on the near-plane latch
    // (elevated on ordinary frames too - starved the fire) and a
    // same-frame depth copy at the fire (predates the view-model's
    // render: shadowed the player's hands). The fire consumes the
    // injection-captured basis: one frame late, post-view-model,
    // epoch-consistent.
    // HYBRID: the fire reconstructs from the ENGINE'S OWN linearized
    // depth (its linearization, its epoch - immune to our encode errors,
    // captured post-view-model), paired with the sweep fov and published
    // view it was always paired with. All v2-era epoch discipline AROUND
    // the fire (settle typing, target identity, derived sun, sanitizer)
    // stays - that stack took the overcast from frequent-and-sustained
    // to a rare flicker, and none of it lives in the reconstruction.
    // Forensics: snapshot this fire's reconstruction inputs for the stats.
    g_mask.last_fire_fov[0] = g_mask.cast_fov[0];
    g_mask.last_fire_fov[1] = g_mask.cast_fov[1];
    g_mask.last_fire_dims[0] = g_mask.cast_dims[0];
    g_mask.last_fire_dims[1] = g_mask.cast_dims[1];
    g_mask.last_fire_rot_err = g_ls.last_publish_rot_err;
    if (!g_mask.engine_mask_rtv) { g_ls.cast_misses = 61; return; }   // captured at the gated resolve sweep
    g_ls.cast_misses = 0;
    g_mask_cast_arm = false;
    g_mask_cast_fired = true;

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_cast < 0.0f) {
        g_mask.cold_first_cast = static_cast<float>(effect_time_seconds() - g_mask.cold_t0);
    }

    // Render (or reuse) this frame's private sun-depth map FIRST: the fire
    // consumes it for mesh-shaped casts. On INFRASTRUCTURE failure the
    // legacy AABB slab below carries the frame - never a lost fire. But
    // 'every caster is beyond the fit radius' is the distance rule doing
    // its job, not a failure: shadows are OFF out there (the engine's own
    // object shadows behave the same), and letting the slab fire instead
    // put long-range AABB casts under a fast-moving Zeus camera - the
    // patternless overcast of the two-box field test.
    // The map renders at the INJECTION (a proven post-resolve point); the
    // fire consumes the PREVIOUS frame's. Sun and casters are frame-
    // coherent to +-1 frame, so a one-frame-late world-anchored map is
    // invisible - and the fire no longer executes a foreign render pass
    // in the middle of the engine's resolve, which the forceSlab bisect
    // implicated (the overcast survived the branch switch, and the pass
    // was the one element that ran in both branches and did not exist in
    // the pre-refactor version).
    const bool map_fresh = g_sun_map_valid &&
                           g_sun_map_time >= 0.0f &&
                           effect_time_seconds() - g_sun_map_time < 0.25f;
    if (!map_fresh && g_sun_map_no_local) { g_ls.cast_misses = 81; return; }
    const bool sun_map = map_fresh;

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

    struct CastEntry { float pos[3]; float size[3]; };
    CastEntry centries[64];   // FALLBACK-ONLY cap: the sun-map path is one
                              // fullscreen draw for ANY caster count; only
                              // the per-caster AABB-slab fallback (cold /
                              // map-unavailable frames) pays per object,
                              // and 64 nearest-in-map-order bounds that.
    uint32_t ncb = 0;

    if (!sun_map) {
        std::lock_guard<std::mutex> lk(g_draw_list_mutex);

        for (const auto& kv2 : g_draw_list) {
            if (ncb >= 64) break;
            const auto& o = kv2.second;
            // Casters are world-space meshes only: fullscreen passes carry
            // default pos/size (a phantom 1 m shadow at the map origin),
            // hidden objects must not shadow the world they are hidden
            // from, and overlays (mode 2) never participate in world
            // depth. Effect meshes remain casters - they are real volumes.
            if (o.fullscreen || !o.visible || o.mode == DepthMode::Off) continue;
            memcpy(centries[ncb].pos, o.pos, 12);
            memcpy(centries[ncb].size, o.size, 12);
            ncb++;
        }
    }

    // Map path: a valid sun map implies casters existed this frame (the
    // pass refuses to render without any) - no snapshot needed.
    if (!sun_map && ncb == 0) { g_ls.cast_misses = 8; return; }
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
    // read our mesh cbd as its constants - the last-fired mesh's COLOR
    // tinted all particles (blue, from the fog array). The dual-gated
    // arm exposed it by moving the fire later in the frame, past the
    // engine's last natural VS-b0 rebind before the particle pass.
    ctx->OMGetBlendState(&old_blend, old_bf, &old_bmask);
    ctx->OMGetDepthStencilState(&old_dss, &old_sref);
    ctx->PSGetShaderResources(0, 1, &old_t0);
    ID3D11ShaderResourceView* old_t11 = nullptr;
    ctx->PSGetShaderResources(11, 1, &old_t11);   // sun-map slot (Get/Set/Restore rule)
    // GS/HS/DS + rasterizer parity (the state-leak ledger, closed): every
    // other foreign draw site nulls the geometry stages before drawing
    // (sun-depth pass, injection, both flushes - all via StateBackup or an
    // explicit null), but the fire's manual save set never covered them or
    // the rasterizer. The fire draws a fullscreen triangle: a bound GS
    // would route our vertices through a foreign expansion, and an engine
    // rasterizer state with ScissorEnable would clip the MIN write to
    // whatever rect the resolve left behind - both silent partial-cast
    // modes. Saved here, nulled below, restored with the rest.
    ID3D11GeometryShader* old_gs = nullptr;
    ID3D11HullShader* old_hs = nullptr;
    ID3D11DomainShader* old_ds2 = nullptr;
    ID3D11RasterizerState* old_rs = nullptr;
    ctx->GSGetShader(&old_gs, nullptr, nullptr);
    ctx->HSGetShader(&old_hs, nullptr, nullptr);
    ctx->DSGetShader(&old_ds2, nullptr, nullptr);
    ctx->RSGetState(&old_rs);
    ctx->OMSetRenderTargets(1, &g_mask.engine_mask_rtv, nullptr);
    D3D11_VIEWPORT vpn = {};
    vpn.Width = g_mask.cast_dims[0];
    vpn.Height = g_mask.cast_dims[1];
    vpn.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpn);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskcast, nullptr, 0);   // engine linear depth: single-sample
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Default rasterizer: solid fill, scissor OFF. The standard SV_VertexID
    // fullscreen triangle is clockwise in render-target space - the default
    // front face - so CullBack never rejects it; DepthClip is moot at z = 0.
    ctx->RSSetState(nullptr);
    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
    const FLOAT bfm[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.min_blend, bfm, 0xFFFFFFFF);
    ctx->OMSetDepthStencilState(g_mask.cast_dss, 0);
    if (!g_fire_lock_valid) {   // frame's first fire: freeze the inputs
        g_fire_lock = g_mask.cast_depth;
        g_fire_lock->AddRef();
        g_fire_fov2[0] = g_mask.cast_fov[0];
        g_fire_fov2[1] = g_mask.cast_fov[1];
        g_fire_dims2[0] = g_mask.cast_dims[0];
        g_fire_dims2[1] = g_mask.cast_dims[1];
        memcpy(g_fire_view2, g_ls.frame_view, sizeof(g_fire_view2));
        const float* fsun = kh_shadow_sun();
        if (!fsun) fsun = g_sun_dir_engine;
        g_fire_sun2[0] = fsun[0];
        g_fire_sun2[1] = fsun[1];
        g_fire_sun2[2] = fsun[2];
        g_fire_lock_valid = true;
    }

    // FROZEN REPLAY, RESTORED as the fire's input mode - the live-input
    // experiment is FIELD-FALSIFIED and retired: offset/gapped shadow
    // contact, 18.8 s cold-to-first-cast, overcast still breaking
    // through. The frozen replay was field-stable for months: inputs
    // latched at the frame's first fire, every same-frame re-fire a
    // byte-identical replay (idempotent under MIN: no ghost; every
    // partition batch re-shadowed: no look-down gap). The band-era
    // keeps its two proven, input-mode-independent wins upstream: the
    // half-res resolve acceptance (the 74,941-batch classifier
    // starvation - the fog-overcast root) and resolution-agnostic
    // depth addressing (the quadrant bug).
    g_cast_frozen_fires++;
    ctx->PSSetShaderResources(0, 1, &g_fire_lock);   // frozen: identical replay per batch
    if (sun_map && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);

    auto fill_frame = [&](ConstantData& cbd) {
        for (int r = 0; r < 3; ++r) {
            memcpy(cbd.cast_mat[r], g_fire_view2 + r * 4, 16);   // frozen view
        }

        memcpy(cbd.cast_view[0], g_fire_view2 + 12, 16);
        cbd.cast_view[1][0] = g_fire_fov2[0];
        cbd.cast_view[1][1] = g_fire_fov2[1];
        cbd.cast_view[1][2] = g_fire_dims2[0];
        cbd.cast_view[1][3] = g_fire_dims2[1];
        cbd.cast_view[2][0] = g_fire_sun2[0];   // frozen sun (see the latch)
        cbd.cast_view[2][1] = g_fire_sun2[1];
        cbd.cast_view[2][2] = g_fire_sun2[2];
        cbd.cast_view[2][3] = g_shadow_map_strength;   // 1 = full: engine handles ambient
    };

    if (sun_map) {
        // MESH-SHAPED cast: ONE fullscreen test against the sun-depth map
        // covers EVERY caster at once - the map already holds them all.
        // centerSize/sizeAxes carry the COMBINED caster bounds (already
        // engine axes) for the shader's locality clamp.
        D3D11_MAPPED_SUBRESOURCE mapped = {};

        if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            g_ls.cast_misses = 9;
        } else {
            ConstantData cbd = {};
            fill_frame(cbd);
            cbd.center_size[0] = g_sun_map_bounds[0];
            cbd.center_size[1] = g_sun_map_bounds[1];
            cbd.center_size[2] = g_sun_map_bounds[2];
            cbd.size_axes[0] = g_sun_map_bounds[3] * 2.0f;
            cbd.size_axes[1] = g_sun_map_bounds[4] * 2.0f;
            cbd.size_axes[2] = g_sun_map_bounds[5] * 2.0f;
            memcpy(cbd.sun_vp, g_sun_map_vp, sizeof(cbd.sun_vp));
            cbd.sun_meta[0] = 1.0f;
            cbd.sun_meta[1] = static_cast<float>(KH_SUN_DEPTH_SIZE);
            cbd.sun_meta[2] = g_sun_map_bias;
            cbd.sun_meta[3] = g_shadow_map_strength;
            cbd.locality_meta[0] = static_cast<float>(g_sun_local_count);

            for (int li = 0; li < g_sun_local_count; ++li) {
                cbd.locality[li * 2][0] = g_sun_local_bounds[li][0];
                cbd.locality[li * 2][1] = g_sun_local_bounds[li][1];
                cbd.locality[li * 2][2] = g_sun_local_bounds[li][2];
                // (reach tightening under the partition latch is falsified:
                // it INCREASED the Zeus-view overcast)
                cbd.locality[li * 2 + 1][0] = g_sun_local_bounds[li][3];
                cbd.locality[li * 2 + 1][1] = g_sun_local_bounds[li][4];
                cbd.locality[li * 2 + 1][2] = g_sun_local_bounds[li][5];
            }

            memcpy(mapped.pData, &cbd, sizeof(cbd));
            ctx->Unmap(g_res.composite_cb, 0);
            ctx->Draw(3, 0);
            g_mask.analytic_casts++;
        }
    } else {
        // AABB slab fallback: one fullscreen test per caster, exactly the
        // pre-map behavior (zeroed sunMeta selects the shader's slab path).
        for (uint32_t bi = 0; bi < ncb; ++bi) {
            D3D11_MAPPED_SUBRESOURCE mapped = {};
            if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) { g_ls.cast_misses = 9; break; }
            ConstantData cbd = {};
            fill_frame(cbd);
            cbd.center_size[0] = centries[bi].pos[0];
            cbd.center_size[1] = centries[bi].pos[2];
            cbd.center_size[2] = centries[bi].pos[1];
            cbd.size_axes[0] = centries[bi].size[0];
            cbd.size_axes[1] = centries[bi].size[2];
            cbd.size_axes[2] = centries[bi].size[1];
            memcpy(mapped.pData, &cbd, sizeof(cbd));
            ctx->Unmap(g_res.composite_cb, 0);
            ctx->Draw(3, 0);
            g_mask.analytic_casts++;
        }
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
    ctx->PSSetShaderResources(11, 1, &old_t11);
    ctx->GSSetShader(old_gs, nullptr, 0);
    ctx->HSSetShader(old_hs, nullptr, 0);
    ctx->DSSetShader(old_ds2, nullptr, 0);
    ctx->RSSetState(old_rs);
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
    KH_SAFE_RELEASE(old_t11);
    KH_SAFE_RELEASE(old_gs);
    KH_SAFE_RELEASE(old_hs);
    KH_SAFE_RELEASE(old_ds2);
    KH_SAFE_RELEASE(old_rs);
}

inline void mask_note_draw(ID3D11DeviceContext* ctx) {
    if (!g_mask.atlas_bound || !ctx) return;
    g_mask.atlas_bound = false;   // one record per pass segment is plenty
    g_ls.atlas_last_seen = effect_time_seconds();   // the engine just consumed the held atlas: it is alive

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
        bool single_seen = false;

        for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
            const MaskCandidate& c = g_mask.cands[i];
            if (c.w < 1280) continue;
            const bool single =
                c.fmt == DXGI_FORMAT_R8_UNORM || c.fmt == DXGI_FORMAT_R8_TYPELESS ||
                c.fmt == DXGI_FORMAT_R16_UNORM || c.fmt == DXGI_FORMAT_R16_TYPELESS ||
                c.fmt == DXGI_FORMAT_R16_FLOAT;
            if (!single) continue;
            single_seen = true;
            bool failed = false;

            for (uint32_t fk = 0; fk < g_mask.failed_count; ++fk) {
                if (g_mask.failed_keys[fk] == c.key) { failed = true; break; }
            }

            if (failed) continue;

            if (c.draws > best) {
                best = c.draws;
                pick = static_cast<int>(i);
            }
        }

        // Pre-identification bootstrap ONLY: fires while NO single-channel
        // candidate has ever appeared. A blacklisted single-channel
        // candidate must NOT fall through here - picking the most-drawn
        // any-format target read the HDR scene as a shadow mask (the
        // 'received shadows shifting with scene content' field report).
        if (pick < 0 && !single_seen) {
            for (uint32_t i = 0; i < g_mask.cand_count; ++i) {
                if (g_mask.cands[i].w < 640 || g_mask.cands[i].draws <= best) continue;
                bool failed = false;

                for (uint32_t fk = 0; fk < g_mask.failed_count; ++fk) {
                    if (g_mask.failed_keys[fk] == g_mask.cands[i].key) { failed = true; break; }
                }

                if (failed) continue;
                best = g_mask.cands[i].draws;
                pick = static_cast<int>(i);
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
        // Blacklist THIS candidate only (bounded, so no per-frame retry
        // storm on it); the mode stays alive for the other candidates.
        if (g_mask.failed_count < 4) g_mask.failed_keys[g_mask.failed_count++] = g_mask.cands[pick].key;
        g_mask.fail_fmt = static_cast<uint32_t>(fmt);
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
        ((!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted() ||
         (g_proj_locator.valid && res == g_proj_locator.buf))) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            // RE-MAP RECLAIM: if a tracked mapping's unmap was ever missed
            // (a pair split across a device-reset boundary is the realistic
            // path), its slot parked a STALE pointer that the resource's
            // NEXT unmap would hand to the scanners - a read through a
            // dead dynamic-buffer mapping. A fresh Map of the same resource
            // supersedes the stale entry in place, so the pointer a slot
            // holds is always the one the CURRENT map/unmap pair owns. The
            // free-slot search is unchanged when no same-res entry exists.
            ProjPendingMap* slot = nullptr;

            for (auto& p : g_proj_pending) {
                if (p.res == res) { slot = &p; break; }
                if (!p.res && !slot) slot = &p;
            }

            if (slot) {
                slot->res = res;
                slot->data = mapped->pData;
                slot->bytes = bytes;
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
                locator_note_upload(res, p.data, p.bytes);   // fog/sun color locators (read-only)
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

static void STDMETHODCALLTYPE hooked_updatesubresource(ID3D11DeviceContext* self, ID3D11Resource* res, UINT sub, const D3D11_BOX* dst_box, const void* data, UINT row_pitch, UINT depth_pitch) {
    if (sub == 0 && !dst_box && data &&
        self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
        reorder_on_render_thread() && !g_ro.in_injection &&
        ((!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted() ||
         (g_proj_locator.valid && res == g_proj_locator.buf))) {
        const uint32_t bytes = proj_upload_byte_width(res);

        if (bytes != 0) {
            proj_scan_upload(res, data, bytes);
            shadow_live_upload(data, bytes);
            locator_note_upload(res, data, bytes);   // fog/sun color locators (read-only)
            if (!g_ro.in_injection) {
                shadow_register_upload(res, data, bytes);
                shadow_view_scan(res, data, bytes);
            }
        }
    }

    g_orig_updatesubresource(self, res, sub, dst_box, data, row_pitch, depth_pitch);
}

// Draws every eligible mesh into the CURRENT scene targets with depth
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
// which needs meshes - so a freshly spawned mesh's first frames ran
// lockless (frame_view invalid, the hybrid's small spawn delay). The
// same attempt, run at the frame boundary against the bridge cycle PV,
// acquires the lock BEFORE any mesh exists. Does not clear the ring and
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
    if (g_ls.vc_ring[best_i].res != g_prewarm_cand_res ||
        g_ls.vc_ring[best_i].off != g_prewarm_cand_off) {
        g_prewarm_cand_res = g_ls.vc_ring[best_i].res;
        g_prewarm_cand_off = g_ls.vc_ring[best_i].off;
        return;   // first sighting: confirm next boundary
    }

    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
    g_ls.view_src_orient = best_k;
    g_ls.view_src_relative = true;
    g_ls.view_src_valid = true;
    g_ls.view_src_miss = 0;
    g_ls.view_locks++;

    // VIEW-LOCK BOUNDARY WIPE (the post-spawn drift): cascade entries
    // latched BEFORE this lock were paired with a settling view - their
    // transforms track the camera instead of the world, and finest-first
    // consumption lets a wrong coarse entry linger for many seconds
    // (coarse cascades relatch rarely). Entries rebuild from live
    // latches within a second of the wipe; a briefly thinner cascade
    // set is delay-class, drift is artifact-class.
    g_ls.count = 0;
    g_ls.newest = -1;
    g_lock_wipes++;
}

inline void inject_composited_meshes(ID3D11DeviceContext* ctx) {
    const float snapshot_now = effect_time_seconds();

    if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) {
        g_mask.cold_first_trigger = static_cast<float>(snapshot_now - g_mask.cold_t0);
    }
    static std::vector<RenderObject> meshes;   // render-thread scratch
    meshes.clear();

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        // Staging-funnel forensics: a session arrived with
        // coldFirstStaged -1 and an UNCHANGED script - these split
        // 'the add calls never arrived' (total 0: script/call side)
        // from 'everything was filtered' (total > 0 with a rejection
        // counter naming the gate: extension side, e.g. a clock fault
        // expiring every object instantly).
        g_stage_total = static_cast<uint32_t>(g_draw_list.size());
        g_stage_rej_vis = 0;
        g_stage_rej_exp = 0;

        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;

            if (!o.visible || !is_composite_eligible(o)) { g_stage_rej_vis++; continue; }
            bool expired = false;
            const float env = lifetime_envelope(o, snapshot_now, expired);
            if (expired) { g_stage_rej_exp++; continue; }   // the Draw3D flush owns the erasure
            meshes.push_back(o);
            meshes.back().color[3] *= env;
        }

        if (!meshes.empty() && g_mask.cold_t0 >= 0.0 && g_mask.cold_first_stage < 0.0f) {
            g_mask.cold_first_stage = static_cast<float>(snapshot_now - g_mask.cold_t0);
        }
    }

    // The path is alive even when the eligible set is momentarily empty -
    // stamping here keeps the flush from flapping over.
    g_composite_last_inject_ms.store(steady_now_ms(), std::memory_order_relaxed);
    if (meshes.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) { g_stats.composite_skips++; return; }
    if (!ensure_resources(dev).empty()) { g_stats.composite_skips++; return; }

    // Render this frame's private sun-depth map HERE - the one place a
    // foreign pass is proven safe (post-resolve, inside our own injection
    // window). This frame's self term samples it below; NEXT frame's cast
    // fire consumes it one frame late (world-anchored: invisible).
    // Attribution combo: self off + forceSlab on = nobody consumes the
    // map, so it does not render at all - the exact pre-refactor
    // configuration (slab cast + bands), for bisecting refactor damage.
    render_sun_depth(ctx);

    // Matrices: the CYCLE'S latched PV (captured at the main depth clear,
    // synchronous with the engine's own frame-setup snapshot - see the
    // cycle_pv note in ReorderState). A mid-frame bridge fetch is the LAST
    // resort only: by the translucent phase the sim thread has often
    // published the NEXT frame's projection, whose dynamic near plane can
    // differ - skewing the meshes' stored depth worst at close range.
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
            // plane SLICED the injected mesh (the look-up clip). Keep the
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
        g_last_pv = pv;
        g_last_pv_ms = steady_now_ms();
    } else if (g_last_pv_ms != 0 && steady_now_ms() - g_last_pv_ms < 250) {
        pv = g_last_pv;
    } else {
        g_stats.composite_skips++;
        return;   // no usable matrices anywhere this frame
    }

    // MEASURED depth coefficients: when the projection sniffer captured the
    // engine's own m22/m32 this cycle, overwrite the bridge values. The
    // meshes then rasterize with the TRUE dynamic near plane of the frame
    // being rendered - the one scalar the bridge can never pin down - and
    // the guard reconstructs scene distance with the same truth, so its
    // margins tighten from race-absorbing to precision-absorbing.
    const bool measured = g_ro.engine_proj_valid;

    // STATE SNAPSHOT: injection decisions read the telltale ONCE. The
    // engine mutates cycle state from other threads (deferred-context
    // recording); the snapshot keeps every decision below
    // self-consistent.
    const float snap_slot_near = g_ro.slot_near_live;
    const float snap_slot_m22 = g_ro.slot_m22;
    const float snap_slot_m32 = g_ro.slot_m32;

    if (measured) {
        pv.projection[2][2] = g_ro.engine_m22;
        pv.projection[3][2] = g_ro.engine_m32;
    }

    // The camera-class near authority BEFORE slot arbitration: the
    // latch/measured value, which the sniffer's anchoring keeps
    // camera-class by construction.
    const float khr_bridge_near = fabsf(pv.projection[2][2]) > 1e-9f
                                ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;

    // ENCODE ARBITRATION BY THE LIVE SLOT: the telltale is the freshest
    // engine-verbatim projection of the current phase, correct in both
    // directions the latch can be poisoned (a mid-publication foreign
    // window, or a stale far value), so when it is live this cycle it IS
    // the encode. Measured, then bridge, remain the fallbacks.
    // PERSISTED-PAIR FALLBACK (the injection side of the flush's fix): a
    // silent slot with a fresh keep means the engine skipped a redundant
    // upload - the last live values remain this camera's truth, and they
    // shield the encode from a poisoned latch on exactly the frames the
    // arbitration is otherwise blind.
    const bool khr_keep_fresh = g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
                                steady_now_ms() - g_slot_keep_ms < 250;

    if (snap_slot_near > 0.0f) {
        pv.projection[2][2] = snap_slot_m22;
        pv.projection[3][2] = snap_slot_m32;
        g_stats.composite_slot_encodes++;
    } else if (khr_keep_fresh) {
        // KEEP ARBITRATION (field: a rescued flicker carried keep near
        // 0.544 against a frame near ~0.86 - the keep goes stale the
        // moment the camera moves through a silent frame, while the
        // latch/live arbitration above already holds the frame's camera.
        // The keep's proven job is the out-of-band poison shield; an
        // IN-BAND pv that disagrees is fresher by construction and wins.
        const float khr_pv_near = fabsf(pv.projection[2][2]) > 1e-9f
                                ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool khr_pv_in_band = khr_pv_near >= 0.05f && khr_pv_near <= 5.0f;
        const bool khr_keep_agrees = khr_pv_in_band &&
            fabsf(khr_pv_near - g_slot_keep_near) <= 0.25f * khr_pv_near;

        if (khr_keep_agrees || !khr_pv_in_band) {
            pv.projection[2][2] = g_slot_keep_m22;
            pv.projection[3][2] = g_slot_keep_m32;
            g_stats.composite_keep_encodes++;
        } else {
            g_keep_stale_skips++;
        }
    }

    // FAR-PHASE SKIP, refined by field conviction (miss-reason latch,
    // 4-for-4 lineup coincidence): the blanket skip assumed "the camera
    // partition's own trigger takes the frame", but on sky/horizon frames
    // the far partition IS the frame - no later trigger arrives, the
    // frame goes uninjected, and the flush's late carry over the
    // translucents is itself the visible flicker. The skip's one genuine
    // protective case is a mesh at or inside the far partition's near
    // plane (clips to nothing / degenerates into the fog term) - so
    // MEASURE it: when every staged mesh sits comfortably beyond that
    // near plane, the slot (far) pair selected above is simply the
    // CORRECT encode for this phase, and the injection proceeds. A
    // following camera partition still gets its own injection via the
    // partition-change re-arm ("an earlier far-partition draw is
    // corrected by the nearer re-draw"). One-sided as before - the
    // probe's fov anchors already reject nearer foreign windows.
    if (snap_slot_near > 0.0f && khr_bridge_near > 1e-4f &&
        snap_slot_near > 1.5f * khr_bridge_near) {
        float khr_cam[3];
        extract_camera_pos(pv.view, khr_cam);
        float khr_min_clear = 1e9f;

        for (const auto& m : meshes) {
            const float me[3] = { m.pos[0], m.pos[2], m.pos[1] };   // SQF -> engine axes
            const float dx = me[0] - khr_cam[0];
            const float dy = me[1] - khr_cam[1];
            const float dz = me[2] - khr_cam[2];
            const float r = 0.5f * sqrtf(m.size[0] * m.size[0] +
                                         m.size[1] * m.size[1] +
                                         m.size[2] * m.size[2]);
            const float d = sqrtf(dx * dx + dy * dy + dz * dz) - r;

            if (d < khr_min_clear) khr_min_clear = d;
        }

        if (khr_min_clear < 1.5f * snap_slot_near) {
            // Clip risk is real for at least one mesh: the protective
            // skip, unchanged. A repaint-invoked call landing here gets
            // its one-shot refunded, so a genuine repaint later the same
            // cycle still fires.
            g_stats.composite_far_phase_skips++;
            g_ro.injected = false;      // stay armed for the camera partition
            g_ro.inject_attempts = 0;
            return;
        }

        g_stats.composite_far_injects++;   // proceed: far pair is this phase's truth
    }

    // ANOMALOUS CYCLE: a proven locator with a silent slot means the
    // engine is rendering a pass that never published a camera-class
    // projection - a foreign scene-class pass whose output does not
    // reliably reach presentation. Injections placed anywhere inside it
    // are wasted at best; attempts SKIP and stay armed - a clean cycle's
    // own trigger or the hybrid guarantee below carries the frame.
    // SILENT-SLOT RESCUE - PERMANENTLY FALSIFIED (do not re-derive, do
    // not retry; full trial history): (1) first trial: 1:1 whole-frame
    // flicker correlation, reverted; (2) evidence contaminated by the
    // keep poison (probe repoisons ~28x/frame; band fixed it); (3)
    // isolated retry on the sanitized keep: flickers at moving-camera
    // rescues, keep near 0.544 vs frame ~0.86 - the stale-keep class,
    // fixed by the pv-agreement arbitration; (4) FINAL trial with the
    // arbitration: a flicker with rescueLastAgeS 0.748, rescueLastNear
    // 0.869 vs frame 0.889 - fresh, agreeing, correctly encoded, and
    // STILL not presented. Every alternative explanation is exhausted:
    // output injected into a slot-silent (no-upload) cycle does not
    // reliably reach presentation, on calm sparse frames and heavy fog
    // alike. Silent cycles SKIP and the flush carries them late with the
    // arbitrated keep encode - the pop is the architecture's floor on
    // no-upload frames absent an upload-independent injection design.
    // The counter and forensics below remain as a passive overlap census.
    const bool khr_anomalous = g_proj_locator_ever && snap_slot_near <= 0.0f;

    if (khr_anomalous) g_ro.anomaly_seen = true;   // flush carries this frame

    if (khr_anomalous) {
        g_stats.composite_anomaly_skips++;
        g_fl_anom_skip_ms = steady_now_ms();
        g_ro.injected = false;      // stay armed
        g_ro.inject_attempts = 0;
        return;
    }

    // near = 10 is this scene's CHRONIC normal (partition-far latch);
    // it only bites when mesh fragments come within ~near of the camera
    // (the look-up slice - open design item, see CONTINUATION.md).
    // Forensics only; the near-draw attempt was reverted after causing
    // see-through on ordinary-near frames.
    const float inject_near = fabsf(pv.projection[2][2]) > 1e-9f
                            ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    g_mask.last_inject_near = inject_near;

    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    // supervise the view source: lock the upload location whose contents
    // equal the PROVEN view used for the meshes this frame. QUARANTINE:
    // never on anomalous cycles - those are exactly the moments this
    // frame's pv is least trustworthy, and a supervisor taught by them
    // mis-locks (world-sliding shadows). Safety paths are not training
    // paths.
    if (!khr_anomalous) {
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

            // Lock bar: the COLD lock has no reference and accepts 0.05.
            // RE-locks (a prior lock existed and was evicted, usually by
            // fast-flight bridge lag) must beat 0.02: the 0.045 re-lock
            // in the Zeus field test was a wrong candidate that then
            // reconstructed every cast off-axis, permanently.
            const float lock_bar = g_ls.view_locks == 0 ? 0.05f : 0.02f;

            // EXACTNESS PREFERENCE: exact candidates (rot ~ 0) appear
            // within a frame or two in every measured session, so a
            // bar-passing 0.008 residual is a lock taken one frame too
            // eagerly during churn - small, but the same off-axis
            // reconstruction class as the Zeus re-lock, latent. Prefer
            // essentially-exact candidates; the bar-passing argmin
            // remains as a 2 s timeout fallback so a map with a genuine
            // convention gap can still lock.
            const bool exact_lock = best_rot < 1e-3f;

            if (best_i >= 0 && best_rot < lock_bar && !exact_lock &&
                g_ls.view_wait_since <= 0.0f) {
                g_ls.view_wait_since = effect_time_seconds();
            }

            const bool wait_expired = g_ls.view_wait_since > 0.0f &&
                effect_time_seconds() - g_ls.view_wait_since > 2.0f;

            if (best_i >= 0 && best_rot < lock_bar && (exact_lock || wait_expired)) {
                g_ls.view_wait_since = 0.0f;
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

                // FIRST-ADOPTION CONFIRMATION (watch item #1's cure,
                // promoted from containment): the prewarm already demands
                // the same candidate win two consecutive frame boundaries;
                // the injection-site COLD adoption (no prior lock) now
                // demands the same two-consecutive-injection agreement, so
                // a one-frame fluke that squeaks under the 0.05 cold bar
                // can no longer become the spawn lock. Cost is one frame
                // of delay - delay-class by the acceptance criteria. RE-
                // locks keep the tighter 0.02 bar and adopt immediately (a
                // lock dropped mid-flight must not wait an extra frame).
                bool cold_confirmed = true;

                if (g_ls.view_locks == 0) {
                    cold_confirmed = g_ls.vc_ring[best_i].res == g_coldlock_cand_res &&
                                     g_ls.vc_ring[best_i].off == g_coldlock_cand_off;
                    g_coldlock_cand_res = g_ls.vc_ring[best_i].res;
                    g_coldlock_cand_off = g_ls.vc_ring[best_i].off;
                }

                // RELATIVE ONLY: the engine's view is camera-relative
                // (measured); an absolute-exact candidate is an echo of
                // our own uploads (bandView columns) - locking onto it
                // made frame_view read back our own half-filled CB
                // (garbage seals: both shadow paths vanished).
                if (relative && cold_confirmed) {
                    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
                    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
                    g_ls.view_src_orient = best_k;
                    g_ls.view_src_relative = true;
                    g_ls.view_src_valid = true;

                    g_ls.view_src_miss = 0;
                    g_ls.view_locks++;

                    // view-lock boundary wipe, second adoption site
                    // (the first session on the wipe build locked
                    // twice through THIS path and lockWipes stayed 0
                    // - entries predating the lock survived and the
                    // post-spawn drift with them)
                    g_ls.count = 0;
                    g_ls.newest = -1;
                    g_lock_wipes++;
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

                // 0.10 was speed-fragile the same way the publish filter
                // once was: the bridge truth lags a frame, so a fast spin
                // inflated the error, evicted a CORRECT lock, and the
                // re-lock landed on a worse candidate (the Zeus drift).
                // The publish-side family filter still guards at 0.35.
                healthy = e_rot < 0.20f;
            }

            if (healthy) {
                g_ls.view_src_miss = 0;
            } else if (++g_ls.view_src_miss > 90) {
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

    // Translucent-correct ordering among our own meshes: back to front by
    // the camera's distance to each object's world BOUNDS (nearest point
    // of the AABB). The old center metric misordered large meshes whose
    // faces reach far closer than their centers - exactly the error that
    // grows with mesh size.
    std::sort(meshes.begin(), meshes.end(), [&cam](const RenderObject& a, const RenderObject& b) {
        auto d2 = [&cam](const RenderObject& o) {
            const float c[3] = { cam[0], cam[2], cam[1] };   // engine -> SQF axes
            float acc = 0.0f;

            for (int k = 0; k < 3; ++k) {
                const float lo = o.pos[k] - o.size[k] * 0.5f;
                const float hi = o.pos[k] + o.size[k] * 0.5f;
                const float p = c[k] < lo ? lo : (c[k] > hi ? hi : c[k]);
                const float d = c[k] - p;
                acc += d * d;
            }

            return acc;
        };
        return d2(a) > d2(b);
    });

    StateBackup backup;
    backup.capture(ctx);

    // CRITICAL - viewport depth range: the engine renders in depth
    // PARTITIONS, remapping each pass's NDC z through the viewport
    // MinDepth/MaxDepth of its slice. The meshes' stored depth must be
    // encoded through the SAME range the surrounding scene geometry used,
    // or every comparison skews - the mesh beats opaque walls it sits
    // behind, or loses to the skydome it sits in front of, worst for NEAR
    // fragments where a min-depth offset is a huge world-space error
    // (hence flicker worsening close to the mesh). The authoritative range
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
    // mesh through a wall. When the copy or shader is unavailable the plain
    // pipeline runs, which is simply the previous behavior.
    // (A rearm-cycle guard standdown lived here for one build and is
    // REMOVED by the acceptance criteria: an unguarded frame is the
    // same artifact class as a hollow one. The see-through root is
    // fixed at the source instead - GuardSceneDist treats clear-value
    // depth as no-scene, so partition-churn frames can no longer
    // discard through a phantom far plane.)
    const bool guard = ensure_composite_depth(dev, ctx) &&
                       ensure_composite_shader(dev).empty();
    if (!guard) g_inj_guard_off++;   // guardless injection: the rare see-through/overlay correlate

    UINT stride = sizeof(MeshVertex), offset = 0;
    int bound_mesh = 0;
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[0], &stride, &offset);
    ctx->VSSetShader(guard ? g_res.vs_composite : g_res.vs, nullptr, 0);
    ctx->PSSetShader(guard ? g_res.ps_composite : g_res.ps, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    // Binding probe: sky rows (fog) and the view harvest (lock
    // survival). Runs when either wants; idles when both are satisfied.
    if ((g_fog_valid && g_fog[0] > 1e-4f) || !g_ls.view_src_valid) skybind_step(ctx);

    ctx->VSSetConstantBuffers(0, 1, &g_res.composite_cb);
    ctx->PSSetConstantBuffers(0, 1, &g_res.composite_cb);
    if (guard) ctx->PSSetShaderResources(0, 1, &g_res.comp_depth_srv);
    // Shadow atlas for the per-pixel map compare (harmless when the table
    // is empty: shadowMeta.x = 0 short-circuits the shader). Slot t1 is
    // inside the StateBackup's saved SRV range.
    if (guard && shadow_live_ensure_srv()) ctx->PSSetShaderResources(1, 1, &g_ls.atlas_srv);
    if (mask_ensure_srv(ctx)) ctx->PSSetShaderResources(3, 1, &g_mask.srv);
    // engine shadow mask (captured at the resolve sweep): the depth-gated
    // receive samples it where mesh surfaces hug world geometry - unit
    // stencil shadows land on our meshes through the engine's own factor.
    for (UINT bslot = 0; bslot < 8; ++bslot) {
        if (g_ls.band[bslot].valid && g_ls.band[bslot].srv) {
            const UINT treg = bslot < 6 ? 4 + bslot : 12 + (bslot - 6);   // t4-t9, t12-t13
            ctx->PSSetShaderResources(treg, 1, &g_ls.band[bslot].srv);
        }
    }
    // Private sun-depth map for the self term (t11, inside the saved range).
    if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
    // Depth WRITE is the whole point: the engine's translucents must be
    // able to depth-reject against these meshes.
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    const FLOAT bf[4] = { 0, 0, 0, 0 };


    for (const auto& o : meshes) {
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
        // RECEIVE PATH: never the screen mask. The engine's shadow-resolve
        // mask is computed from WORLD depth before our meshes inject; at
        // our pixels it holds the shadowing of whatever the mesh occludes
        // - view-dependent background values, not the mesh's. Reading it
        // only became possible when the SRV blacklist un-killed the mode,
        // and the field result was shadows on the mesh appearing and
        // vanishing with camera angle. Bands (then the live table) are the
        // receive truth; the mask machinery stays alive for the CAST
        // write, which is registration-exact by construction.
        cbd.mask_meta[0] = 0.0f;
        cbd.mask_meta[1] = g_mask.channel;
        cbd.mask_meta[2] = g_mask.invert;
        cbd.mask_meta[3] = 0.0f;

        {
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

                // SPAWN-WINDOW GUARD: a pending band whose provisional
                // view is the bridge fallback pairs cross-convention -
                // the initial-shadow drift. Absent beats offset; the
                // band re-enters the moment its seal completes.
                if (bs.pending_view && bs.vcol_bridge) { g_ls.band_prov_skips++; continue; }

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
            // (units, clouds, everything the engine resolves) on mesh
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

                // (A lane-41 density calibration lived here briefly and is
                // RETIRED: lane 41 matched the density formula exactly in
                // the base=113 session but read a flat 0.05 across 239 m
                // and 981 m with base=0 - an unmodeled regime - and the
                // resulting 4x clamp at altitude over-fogged ground boxes
                // to the dim tint. Density is params-faithful again; the
                // extinction constant is the operator's knob.)

                // FOG COLOR = RAW TINT, final: the blue belongs to the
                // EXTINCTION, not the target (see the spectral term in the
                // shader - the engine's altitude block volunteered its
                // red-dominant extinction coefficients, closing the
                // ladder). The tint is the convergence color at full
                // optical depth - near-neutral by day, exactly like the
                // engine's own total whiteout in extreme fog - while the
                // blue-shift lives in the journey there. tint x sky is
                // retired: it baked sky hue into the TARGET and still
                // read white once tonemapped.
                // FOG DECOUPLED FROM THE BLOCK LOCK (settings-
                // independence): the whole fill used to be gated on the
                // light-probe lock, so a degenerate staged decay (setFog
                // with decay 0 can never anchor the probe) or the pre-lock
                // cold window left the mesh UNFOGGED in a fogged world - a
                // mismatch, not a delay. Density is params-faithful and
                // needs no block; only the verbatim tint and the engine
                // transmittance terms do. Unlocked frames run the legacy
                // exponential (the shader's fogEngine.w = 0 branch) toward
                // a neutral whiteout target - the engine's own extreme-fog
                // convergence color - until the mirrors supply verbatim.
                if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
                    const float* e = g_light_probe.nb + 36;

                    for (int c = 0; c < 3; ++c) cbd.fog_color[c] = e[c];

                    // Engine transmittance terms from the block mirror: the
                    // density scale, the linear ramp's end + inverse range.
                    cbd.fog_engine[0] = g_light_probe.nb[41];
                    cbd.fog_engine[1] = g_light_probe.nb[48];
                    cbd.fog_engine[2] = g_light_probe.nb[49];
                    cbd.fog_engine[3] = (g_light_probe.nb[48] > 1.0f &&
                                         g_light_probe.nb[49] > 0.0f &&
                                         g_light_probe.nb[41] >= 0.0f) ? 1.0f : 0.0f;
                } else {
                    cbd.fog_color[0] = 1.0f;
                    cbd.fog_color[1] = 1.0f;
                    cbd.fog_color[2] = 1.0f;
                }

                // Verbatim fog COLOR, corrected branch: world geometry
                // takes the engine's HEIGHT-fog path, which fogs toward
                // ROW 1, FLAT - (0.94, 1.37, 2.31) at the last decode,
                // B/R 2.5, the strong blue of the vanilla comparison.
                // Row 7 x gradient is the sky/far branch and was the
                // wrong target for meshes. Gradient points fill as
                // (1,1,1): the shader's g collapses to 1 - flat - with
                // zero shader changes.
                if (g_sky_probe.hits > 0) {
                    cbd.fog_sky[0] = 1.0f;
                    cbd.fog_sky[1] = 1.0f;
                    cbd.fog_sky[2] = 1.0f;
                    cbd.fog_sky[3] = 1.0f;
                    cbd.fog_sky_col[0] = g_sky_probe.nb[4];
                    cbd.fog_sky_col[1] = g_sky_probe.nb[5];
                    cbd.fog_sky_col[2] = g_sky_probe.nb[6];
                }

                g_fog_dbg[0] = cbd.fog_color[0];
                g_fog_dbg[1] = cbd.fog_color[1];
                g_fog_dbg[2] = cbd.fog_color[2];
                g_fog_dbg[3] = cbd.fog_params[3];

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

            // BUG #2 CONTAINMENT: the look-down partition latch can poison
            // trig_vp with slice values (lastInjectNear read 1..10 in
            // every overcast session vs 0.07 in the clean one) - but a
            // real viewport depth range obeys 0 <= min < max <= 1. A
            // poisoned range garbles the linearization for EVERY pixel:
            // the guard's false discards AND, through the v2 basis
            // captured below, the cast's overcast. Reject the impossible.
            if (!(cbd.depth_params[2] >= 0.0f && cbd.depth_params[3] <= 1.0f &&
                  cbd.depth_params[2] < cbd.depth_params[3])) {
                cbd.depth_params[2] = g_scene_vp_min_d;
                cbd.depth_params[3] = g_scene_vp_max_d;
                g_stats.enc_vp_rejects++;
            }
            cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
            cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
            // Slot-encoded frames carry engine-verbatim coefficients:
            // exact margins apply.
            const bool exact_encode = measured || snap_slot_near > 0.0f;
            cbd.fx1[0] = exact_encode ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
            cbd.fx1[1] = exact_encode ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
        } else {
            // No depth copy this frame: the plain-pipeline fallback PS now
            // carries the same guard contract, so stand it down EXPLICITLY
            // - zero margins with invalid depth is the invisible-geometry
            // landmine.
            cbd.fx1[0] = 1e9f;
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(ctx->Map(g_res.composite_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) continue;
        memcpy(mapped.pData, &cbd, sizeof(cbd));
        ctx->Unmap(g_res.composite_cb, 0);
        const int mid = (o.mesh >= 0 && o.mesh < KH_MESH_COUNT) ? o.mesh : 0;

        if (mid != bound_mesh) {
            ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[mid], &stride, &offset);
            bound_mesh = mid;
        }

        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);
        ctx->Draw(mesh_vertex_count(mid), 0);
        g_stats.composite_meshes++;
    }

    if (n_saved_vp > 0) ctx->RSSetViewports(n_saved_vp, saved_vp);

    {
        backup.restore(ctx);
    }
    g_stats.composite_injections++;
    g_composite_inject_serial.fetch_add(1, std::memory_order_relaxed);
    g_ro.opaques_since_inject = 0;   // the flush's repaint check counts from this landing

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
    if (!reorder_on_render_thread()) {   // our own game-thread draws must not be tracked
        // Cold forensics: until hooked_clear_depthstencil identifies the
        // render thread - which requires the game-thread flush to have
        // adopted the main depth identity first - EVERY render-thread
        // draw on this context bails here silently. This was one of the
        // uncounted exits the cold logs pointed at. Counted only while
        // the cold window is open (first compare short-circuits forever
        // after), on the target context only, so the steady-state cost
        // is one failed compare. The count includes our own flush draws
        // (a handful per frame); the render thread's thousands dominate
        // whenever this gate is the actual blocker.
        if (g_mask.cold_first_trigger < 0.0f && g_mask.cold_t0 >= 0.0) g_mask.cold_g_tid++;
        return;
    }

    if (!g_ro.in_injection) {
        shadow_note_draw(self);
        mask_note_draw(self);
        mask_cast_engine(self);
    }
    if (g_ro.in_injection || g_ro.injected) return;

    // Cascade binding harvest: during atlas phases the draws' VS CBs
    // carry the cascade matrices - sample every 16th; the step idles
    // whenever the table is fresh, so healthy sessions pay nothing.
    if (g_ls.phase_on_atlas && ((++g_cascharv_ctr & 15) == 0)) {
        cascbind_step(self);
    }

    if (!g_ro.blend_translucent || !g_ro.dss_nowrite) {
        if (g_ro.dsv_main) {
            // An opaque draw against the main scene depth: evidence that a
            // genuine scene pass is in progress, and the source of this
            // cycle's authoritative viewport depth range.
            ++g_ro.opaque_draws;
            ++g_ro.opaques_since_inject;

            // Sky binding probe, opaque-phase sampling, v2: every 8th
            // draw. v1's every-64th clustered in each partition's EARLY
            // draws (the counter resets per depth-clear cycle) and only
            // ever saw trivial prepass-style constant state - max 416
            // bytes bound across an entire session; the fog-material
            // draws later in the color pass were never sampled. Size
            // queries are nanoseconds; copies stay rare behind the size
            // gate, the pending bits, and the freshness idle. The
            // self-proof: skyBindMaxBw must reach the sky CB's 512 the
            // moment a terrain draw is actually sampled.
            if ((g_ro.opaque_draws & 7) == 0 &&
                ((g_fog_valid && g_fog[0] > 1e-4f) || !g_ls.view_src_valid)) {
                skybind_step(self);
            }

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
                    // world's translucent phase - where the meshes actually
                    // matter - with nothing: the mesh blinks out for the
                    // frame. When the range shifts between opaque samples,
                    // a new partition has begun: re-arm so it gets its own
                    // injection (an earlier far-partition draw of the mesh
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
    // could pull the injection into a completed frame and paint the meshes
    // over everything. Beyond that, the FIRST qualifying translucent draw
    // is the trigger (see the run-gate note above the vtable slots).
    // Terrain-only frames (looking straight down) issue only a few dozen
    // opaque draws - the original threshold never qualified, no injection
    // landed all frame, and the fallback path drew the meshes UNSHADOWED
    // (both shadows 'vanishing' on look-down). A proven projection lock
    // is stronger mistiming evidence than draw count alone: with it, a
    // much lower floor suffices; previews/overlays still fail both.
    const bool khr_floor_keep = g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
                                steady_now_ms() - g_slot_keep_ms < 250;
    const uint32_t min_opaques = (g_ro.engine_proj_valid || khr_floor_keep)
                               ? KH_REORDER_MIN_OPAQUE_DRAWS / 8
                               : KH_REORDER_MIN_OPAQUE_DRAWS;

    if (g_ro.opaque_draws < min_opaques) {
        g_stats.composite_rej_floor++;
        if (g_mask.cold_t0 >= 0.0 && g_mask.cold_first_trigger < 0.0f) g_mask.cold_g_floor++;
        return;
    }

    // (A sparse-frame "off main depth" recheck once lived here; it was
    // UNREACHABLE - the dsv_main gate above had already returned - so its
    // cold-exit counter was structurally zero and could only mislead the
    // cold forensics. The protection it described is real but lives
    // downstream anyway: the live-DSV identity verification below rejects
    // preview/aux passes for dense and sparse frames alike, counted by
    // composite_rej_verify.)

    // CYCLE DISCRIMINATION - the decisive gate. The engine runs MORE THAN
    // ONE clear->draw cycle on the same main depth resource per frame
    // (weapon/hands pass, reflection or overlay renders); each mid-frame
    // clear re-arms this state machine, and a secondary cycle that happens
    // to contain enough opaque draws plus one blended draw would either
    // fire a SECOND injection over the completed frame - mesh over opaques,
    // translucents AND particles at once - or, if it runs before the world,
    // SPEND the injection so the real scene gets no mesh at all. The
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
        // world - injecting there paints the mesh over the completed
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
            // Altitude forensics: at ~500 m the engine moved to near=10
            // and this gate started firing thousands of times per session
            // (black boxes at altitude). Record the ranges it rejects so a
            // dump names the partition layout up there.
            g_trig_rej_vp[0] = vp.MinDepth;
            g_trig_rej_vp[1] = vp.MaxDepth;
            return;
        }

        // The passing trigger's range IS the encode range: the world's
        // translucents depth-test correctly through it against the
        // opaque-written depth, so the meshes must encode through it too.
        g_ro.trig_vp_min = vp.MinDepth;
        g_ro.trig_vp_max = vp.MaxDepth;
        g_ro.trig_vp_valid = true;
        g_trig_acc_vp[0] = vp.MinDepth;
        g_trig_acc_vp[1] = vp.MaxDepth;
    }

    if (g_ro.inject_attempts >= KH_REORDER_MAX_INJECT_ATTEMPTS) return;
    ++g_ro.inject_attempts;

    // Authoritative verification at each attempt: confirm the LIVE depth
    // binding is the main scene's before committing. On mismatch, correct
    // the tracking and RETRY on a later qualifying draw this frame -
    // standing down entirely would blank the meshes for the whole frame
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
    inject_composited_meshes(self);
    g_ro.in_injection = false;
}

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

            // POST-FLUSH REDRAW CENSUS: a clear-less world redraw AFTER
            // the flush shows up as opaques drawn past the flush's stamp
            // (normal frames end at ~the stamp: translucents draw after
            // the flush, opaques do not). Field-proven detector for the
            // accepted repaint residual. Counters only.
            {
                const uint32_t cc_fo = g_cc_flush_opaques.load(std::memory_order_relaxed);

                if (cc_fo != 0xFFFFFFFFu && g_ro.opaque_draws >= cc_fo + 32u) {
                    g_cc_postflush_redraws++;
                    g_cc_pf_last_draws = g_ro.opaque_draws - cc_fo;
                    g_cc_pf_last_ms = steady_now_ms();
                }

                g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
            }

            // A depth clear of the main scene buffer marks the new frame:
            // injection re-arms and the phase evidence resets.
            g_ro.injected = false;
            g_ro.opaque_draws = 0;
            g_ro.inject_attempts = 0;
            g_ro.cycle_vp_valid = false;
            g_ro.trig_vp_valid = false;
            g_ro.engine_proj_valid = false;
            g_ro.slot_near_live = -1.0f;
            shadow_close_cycle();   // frame boundary: fold the recon cycle into the stats

            if (g_sun_jump_pending) {
                // The sun moved discontinuously (skipTime / setDate /
                // sun<->moon swap): every frozen sun-dependent snapshot is
                // wrong NOW, not in a second. Wipe band seals and the live
                // cascade table; both relearn within a few frames from the
                // engine's own re-rendered cascades.
                g_sun_jump_pending = false;

                for (int b = 0; b < 8; ++b) {
                    g_ls.band[b].valid = false;
                    g_ls.band[b].pending_view = false;
                    // Zero the stored matrices too: the reseal change-
                    // detector compares fresh uploads against these, and
                    // a wiped-but-populated slot read as 'unchanged' until
                    // camera rotation forced a grid-turn - the post-skip
                    // 'shadows gone until you spin around the box'.
                    memset(g_ls.band[b].sm, 0, sizeof(g_ls.band[b].sm));
                    memset(g_ls.band[b].border, 0, sizeof(g_ls.band[b].border));
                    g_ls.band[b].last_time = 0.0f;
                }

                g_ls.count = 0;
                g_ls.newest = -1;
                g_recv_wipes++;
                g_mask.sweep_settle = 0;   // the stream's content just changed sun
                g_mask.sweep_need = 10;    // skip-type transitions run transitional
                                           // resolves for several sweeps, exactly
                                           // like a boundary re-engage - the
                                           // post-skip overcast fired into them
                g_sun_map_valid = false;   // the persisted map is the OLD sun's
                g_stats.sun_jump_flushes++;
            }

            shadow_live_frame_reset();
            shadow_view_prewarm();   // lock can exist before the first mesh does
            g_sun_map_rendered_frame = false;   // new frame: the injection re-renders;
                                                // validity/bounds/no_local PERSIST so the
                                                // fire (pre-injection) can consume the
                                                // previous frame's map (staleness is
                                                // bounded by g_sun_map_time, and a sun
                                                // jump wipes validity below)
            if (g_mask_cast_arm) {
                g_mask.cast_arms_lost++;   // re-arm died unfired at the boundary
                g_mask.arms_lost_miss = g_ls.cast_misses;   // which guard starved it (watch item #2 forensics)
                g_cast_arm_lost_ms = steady_now_ms();   // the accepted baseline blink class:
                                                        // its age at a flicker dump closes
                                                        // (or reopens) the campaign
            }
            if (g_fire_lock) { g_fire_lock->Release(); g_fire_lock = nullptr; }
            g_fire_lock_valid = false;   // new frame: first fire re-freezes
            g_mask_cast_fired = false;   // new frame: one analytic pass allowed
            g_mask_cast_arm = false;   // don't arm at the boundary: this frame's
                                       // gated capture hasn't run yet, so the fire
                                       // would use the PREVIOUS frame's depth
                                       // snapshot - on partitioned look-down frames
                                       // near the mesh that stale close-range depth
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
    D3D11_BOX mesh = { 0, 0, 0, count * sizeof(float) * 4, 1, 1 };

    ctx->CopySubresourceRegion(g_res.staging_async[g_async_write_idx], 0, 0, 0, 0,
                               g_res.output_buffer, 0, &mesh);

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

    // Flush stamps: the serial keys the miss latch (once per flush); the
    // opaque count feeds the post-flush redraw census at the next clear.
    g_cc_flush_serial.fetch_add(1, std::memory_order_relaxed);
    g_cc_flush_opaques.store(g_ro.opaque_draws, std::memory_order_relaxed);

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
    // Meshes and fullscreen passes are split; fullscreen passes run last, in
    // handle (creation) order so chained effects compose deterministically.
    const float snapshot_now = effect_time_seconds();
    // A single missed injection inside the 500 ms health window used to
    // leave composited meshes undrawn for that frame - the visible blink
    // when a gate transiently rejects during a camera sweep. The flush now
    // stands down only when an injection actually LANDED since the previous
    // flush; a missed frame falls back to the post-scene draw instead of
    // vanishing. Double-draw on phase misalignment is benign: identical
    // opaque geometry at equal depth resolves to the same pixels.
    static uint64_t s_prev_inject_serial = 0;   // flush runs on the game thread only
    const uint64_t inject_serial = g_composite_inject_serial.load(std::memory_order_relaxed);
    const bool injected_since_last_flush = inject_serial != s_prev_inject_serial;
    s_prev_inject_serial = inject_serial;
    // HYBRID GUARANTEE: the flush stands down for composited meshes only
    // when the frame is provably clean. A frame that saw an anomalous
    // cycle, or whose world redrew after the injection (the repaint), or
    // that never injected, gets its meshes drawn LATE here as well -
    // identical overdraw where the injection survived, fill-in where a
    // foreign pass erased it. One frame of translucent-ordering cost on
    // exactly the frames that would otherwise flicker.
    const bool repainted_since_inject = g_ro.opaques_since_inject >= 32;
    if (injected_since_last_flush && repainted_since_inject) g_flush_repaint_saves++;
    const bool anomaly_this_frame = g_ro.anomaly_seen;
    g_ro.anomaly_seen = false;
    if (anomaly_this_frame && injected_since_last_flush) g_flush_anomaly_carries++;
    const bool comp_healthy = composite_path_healthy() && injected_since_last_flush &&
                              !repainted_since_inject && !anomaly_this_frame;
    std::vector<RenderObject> meshes;
    std::vector<std::pair<uint64_t, RenderObject>> fullscreen;   // key = creation seq

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        meshes.reserve(g_draw_list.size());

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
                    if (!o.affect_ui) fullscreen.emplace_back(o.seq, o);
                } else if (!(comp_healthy && is_composite_eligible(o))) {
                    // Composited meshes are drawn pre-translucent by the
                    // reorder hook; the flush stands down for them while
                    // injections are actually happening.
                    if (is_composite_eligible(o)) {
                        // injection missed the frame: the LATE post-scene
                        // draw carries it (the other rare-artifact correlate)
                        g_flush_fallback_draws++;
                        g_fl_fallback_ms = steady_now_ms();
                        const uint64_t ms_fs = g_cc_flush_serial.load(std::memory_order_relaxed);

                        if (g_ms_flush_serial != ms_fs) {
                            g_ms_flush_serial = ms_fs;
                            g_ms_frames++;
                            g_ms_ms = steady_now_ms();
                        }
                    }
                    meshes.push_back(o);
                }
            }

            ++it;
        }
    }

    if (meshes.empty() && fullscreen.empty()) return;

    std::sort(fullscreen.begin(), fullscreen.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    RVExtBridge::ProjectionViewTransform pv = {};

    // FLUSH FIDELITY: a live bridge fetch at flush time is frequently the
    // NEXT frame's camera (the publication race at its worst point in
    // the frame) - meshes carried by the flush would pop one frame
    // ahead. The flush uses the same truth the injection does: the cycle
    // latch for the camera, the slot pair for the depth encode.
    if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;
        g_flush_latch_pvs++;
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        return;
    }

    // Cycle state can be read mid-reset from another thread; never
    // transform with a degenerate projection - the live bridge repairs.
    if (fabsf(pv.projection[2][2]) < 1e-6f) {
        RVExtBridge::ProjectionViewTransform live_fix = {};
        if (!RVExtBridge::get_projection_view_transform(live_fix)) return;
        pv = live_fix;
        g_flush_pv_repairs++;
    }

    if (g_ro.slot_near_live > 0.0f) {
        pv.projection[2][2] = g_ro.slot_m22;
        pv.projection[3][2] = g_ro.slot_m32;
    } else if (g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
               steady_now_ms() - g_slot_keep_ms < 250) {
        // Slot silent this cycle: the keep is the poison shield, but an
        // IN-BAND pv that disagrees with it is fresher (the moving-camera
        // silent frame) and wins - same arbitration as the injection side.
        const float khf_pv_near = fabsf(pv.projection[2][2]) > 1e-9f
                                ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
        const bool khf_pv_in_band = khf_pv_near >= 0.05f && khf_pv_near <= 5.0f;
        const bool khf_keep_agrees = khf_pv_in_band &&
            fabsf(khf_pv_near - g_slot_keep_near) <= 0.25f * khf_pv_near;

        if (khf_keep_agrees || !khf_pv_in_band) {
            pv.projection[2][2] = g_slot_keep_m22;
            pv.projection[3][2] = g_slot_keep_m32;
            g_flush_slot_keeps++;
        } else {
            g_keep_stale_skips++;
        }
    }

    float view_proj[4][4];
    mul_4x4(pv.view, pv.projection, view_proj);
    bool need_inverse = false;

    for (const auto& o : meshes) {
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

    // Encode forensics for a miss latched THIS flush (the early latch runs
    // in the snapshot loop, before the transform inputs exist): fill them
    // here, where pv is final (post repair + slot) and the live viewport
    // range has been sampled. Serial equality identifies this-flush misses.
    if (g_ms_ms != 0 &&
        g_ms_flush_serial == g_cc_flush_serial.load(std::memory_order_relaxed)) {
        g_ms_near = fabsf(pv.projection[2][2]) > 1e-9f
                  ? (-pv.projection[3][2] / pv.projection[2][2]) : -1.0f;
    }

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

    for (const auto& o : meshes) {
        if (o.effect > 0) any_effect = true;

        // overlay meshes (mode 2, no hardware depth test) occlude via the
        // shader guard, which needs REAL depth: a solid overlay mesh must
        // keep the depth machinery alive even with no effects present
        // (with both dormant, t0 read null -> sceneZ 0 -> the zero-margin
        // guard discarded every fragment: the mesh despawned once the
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
            for (auto& o : meshes)
                if (o.effect == static_cast<int>(EffectId::Pulse)) o.effect = 0;
            for (auto& f : fullscreen)
                if (f.second.effect == static_cast<int>(EffectId::Pulse)) f.second.effect = 0;
        }

        if (!effects_ready) {
            // Demote everything to solid for this frame; fullscreen passes
            // without an effect are meaningless - drop them.
            g_stats.effect_setup_fails++;
            for (auto& o : meshes) o.effect = 0;
            fullscreen.clear();
            if (meshes.empty()) return;
        }
    }

    // Sort meshes: opaque solids first (front-to-back), then translucent /
    // effect meshes (back-to-front), then overlays (back-to-front).
    // Distance metric for draw ordering: the camera's distance to the
    // object's world BOUNDS (nearest point of the AABB), not its center -
    // large meshes reach far closer than their centers, and the center
    // metric misordered exactly those.
    auto dist_sq = [&cam](const RenderObject& o) {
        const float c[3] = { cam[0], cam[2], cam[1] };   // engine -> SQF axes
        float acc = 0.0f;

        for (int k = 0; k < 3; ++k) {
            const float lo = o.pos[k] - o.size[k] * 0.5f;
            const float hi = o.pos[k] + o.size[k] * 0.5f;
            const float p = c[k] < lo ? lo : (c[k] > hi ? hi : c[k]);
            const float d = c[k] - p;
            acc += d * d;
        }

        return acc;
    };

    auto group_of = [](const RenderObject& o) {
        if (o.mode == DepthMode::Off) return 2;
        return (o.color[3] >= 0.999f && o.effect == 0 && o.blend_mode == 0) ? 0 : 1;
    };

    std::sort(meshes.begin(), meshes.end(),
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
    UINT stride = sizeof(MeshVertex), offset = 0;
    int bound_mesh = 0;
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[0], &stride, &offset);
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
    // Private sun-depth map (self term in PSMain), rendered by the render
    // thread earlier this frame - t11, inside StateBackup's saved range.
    if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
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

        // Solid meshes repurpose fx0.xyz (unused effect params at effect 0)
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

        // SOLID MESHES: align the guard's inputs to the INJECTION's
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
            // GUARD ARMING POLICY (flush path). Tight margins only where
            // the guard is the SOLE occlusion authority AND the snapshot
            // is same-frame coherent:
            //  - mode 0/1 solids reach this path only as the fallback for
            //    a MISSED injection - which by definition means the
            //    snapshot (copied by the injection) is at least one frame
            //    stale, while the coefficients below are the CURRENT
            //    cycle's. Reconstructing a stale snapshot with current
            //    coefficients produced 1-2 frame see-through flicker
            //    (false discards) on exactly the gate-churn frames - the
            //    failure the parked overlay-despawn note predicted. Their
            //    occlusion is the hardware depth test anyway: stand down.
            //  - mode-2 overlays have NO hardware depth test; the guard
            //    is their occlusion. Arm it only when comp_healthy (the
            //    injection ran THIS frame, so it copied the snapshot THIS
            //    frame under THIS cycle's coefficients). Unhealthy frames
            //    degrade to unoccluded-not-invisible, the documented rule.
            // Single-sample snapshots only, as before: PSMain's guard
            // texture is declared single-sample.
            const bool snap_fresh = (o.mode == DepthMode::Off) &&
                                    comp_healthy &&
                                    g_res.comp_depth_srv &&
                                    g_res.comp_depth_samples == 1 &&
                                    g_res.comp_depth_time >= 0.0f &&
                                    now - g_res.comp_depth_time < 0.5f;

            if (snap_fresh) {
                const bool measured = g_ro.engine_proj_valid;

                if (measured) {
                    cbd.depth_params[0] = g_ro.engine_m22;
                    cbd.depth_params[1] = g_ro.engine_m32;
                }

                // Slot pair preferred, mirroring the injection's encode
                // arbitration (engine-verbatim beats sniffer-anchored).
                if (g_ro.slot_near_live > 0.0f) {
                    cbd.depth_params[0] = g_ro.slot_m22;
                    cbd.depth_params[1] = g_ro.slot_m32;
                }

                cbd.depth_params[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
                cbd.depth_params[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
                cbd.fx_meta[2] = static_cast<float>(g_res.comp_depth_w);
                cbd.fx_meta[3] = static_cast<float>(g_res.comp_depth_h);
                const bool exact_flush = measured || g_ro.slot_near_live > 0.0f;
                cbd.fx1[0] = exact_flush ? KH_COMPOSITE_GUARD_BASE_MEASURED : KH_COMPOSITE_GUARD_BASE;
                cbd.fx1[1] = exact_flush ? KH_COMPOSITE_GUARD_REL_MEASURED : KH_COMPOSITE_GUARD_REL;
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
        // RECEIVE PATH: never the screen mask. The engine's shadow-resolve
        // mask is computed from WORLD depth before our meshes inject; at
        // our pixels it holds the shadowing of whatever the mesh occludes
        // - view-dependent background values, not the mesh's. Reading it
        // only became possible when the SRV blacklist un-killed the mode,
        // and the field result was shadows on the mesh appearing and
        // vanishing with camera angle. Bands (then the live table) are the
        // receive truth; the mask machinery stays alive for the CAST
        // write, which is registration-exact by construction.
        cbd.mask_meta[0] = 0.0f;
        cbd.mask_meta[1] = g_mask.channel;
        cbd.mask_meta[2] = g_mask.invert;
        cbd.mask_meta[3] = 0.0f;

        {
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

                // SPAWN-WINDOW GUARD: a pending band whose provisional
                // view is the bridge fallback pairs cross-convention -
                // the initial-shadow drift. Absent beats offset; the
                // band re-enters the moment its seal completes.
                if (bs.pending_view && bs.vcol_bridge) { g_ls.band_prov_skips++; continue; }

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
            // (units, clouds, everything the engine resolves) on mesh
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

                // (A lane-41 density calibration lived here briefly and is
                // RETIRED: lane 41 matched the density formula exactly in
                // the base=113 session but read a flat 0.05 across 239 m
                // and 981 m with base=0 - an unmodeled regime - and the
                // resulting 4x clamp at altitude over-fogged ground boxes
                // to the dim tint. Density is params-faithful again; the
                // extinction constant is the operator's knob.)

                // FOG COLOR = RAW TINT, final: the blue belongs to the
                // EXTINCTION, not the target (see the spectral term in the
                // shader - the engine's altitude block volunteered its
                // red-dominant extinction coefficients, closing the
                // ladder). The tint is the convergence color at full
                // optical depth - near-neutral by day, exactly like the
                // engine's own total whiteout in extreme fog - while the
                // blue-shift lives in the journey there. tint x sky is
                // retired: it baked sky hue into the TARGET and still
                // read white once tonemapped.
                // FOG DECOUPLED FROM THE BLOCK LOCK (settings-
                // independence): the whole fill used to be gated on the
                // light-probe lock, so a degenerate staged decay (setFog
                // with decay 0 can never anchor the probe) or the pre-lock
                // cold window left the mesh UNFOGGED in a fogged world - a
                // mismatch, not a delay. Density is params-faithful and
                // needs no block; only the verbatim tint and the engine
                // transmittance terms do. Unlocked frames run the legacy
                // exponential (the shader's fogEngine.w = 0 branch) toward
                // a neutral whiteout target - the engine's own extreme-fog
                // convergence color - until the mirrors supply verbatim.
                if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
                    const float* e = g_light_probe.nb + 36;

                    for (int c = 0; c < 3; ++c) cbd.fog_color[c] = e[c];

                    // Engine transmittance terms from the block mirror: the
                    // density scale, the linear ramp's end + inverse range.
                    cbd.fog_engine[0] = g_light_probe.nb[41];
                    cbd.fog_engine[1] = g_light_probe.nb[48];
                    cbd.fog_engine[2] = g_light_probe.nb[49];
                    cbd.fog_engine[3] = (g_light_probe.nb[48] > 1.0f &&
                                         g_light_probe.nb[49] > 0.0f &&
                                         g_light_probe.nb[41] >= 0.0f) ? 1.0f : 0.0f;
                } else {
                    cbd.fog_color[0] = 1.0f;
                    cbd.fog_color[1] = 1.0f;
                    cbd.fog_color[2] = 1.0f;
                }

                // Verbatim fog COLOR, corrected branch: world geometry
                // takes the engine's HEIGHT-fog path, which fogs toward
                // ROW 1, FLAT - (0.94, 1.37, 2.31) at the last decode,
                // B/R 2.5, the strong blue of the vanilla comparison.
                // Row 7 x gradient is the sky/far branch and was the
                // wrong target for meshes. Gradient points fill as
                // (1,1,1): the shader's g collapses to 1 - flat - with
                // zero shader changes.
                if (g_sky_probe.hits > 0) {
                    cbd.fog_sky[0] = 1.0f;
                    cbd.fog_sky[1] = 1.0f;
                    cbd.fog_sky[2] = 1.0f;
                    cbd.fog_sky[3] = 1.0f;
                    cbd.fog_sky_col[0] = g_sky_probe.nb[4];
                    cbd.fog_sky_col[1] = g_sky_probe.nb[5];
                    cbd.fog_sky_col[2] = g_sky_probe.nb[6];
                }

                g_fog_dbg[0] = cbd.fog_color[0];
                g_fog_dbg[1] = cbd.fog_color[1];
                g_fog_dbg[2] = cbd.fog_color[2];
                g_fog_dbg[3] = cbd.fog_params[3];

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

    // --- Mesh pass ---
    for (const auto& o : meshes) {
        const bool ov = (o.effect == 0 && o.mode == DepthMode::Off);
        if (ov) g_mask.ov_listed++;      // reached the draw loop
        if (!upload_cb(o, false)) { if (ov) g_mask.ov_skipped++; continue; }
        if (ov) g_mask.ov_drawn++;       // Draw() will be issued below
        ctx->PSSetShader(o.effect > 0 ? g_res.ps_effect : g_res.ps, nullptr, 0);

        // PSComposite reads depth at t0; the flush's default pair puts
        // scene color there (for effect shaders). EVERY solid mesh swaps
        // the composite path's PROVEN depth snapshot into t0 for its
        // draw; the pair returns for effect meshes.
        if (o.effect == 0 && g_res.comp_depth_srv) {
            ctx->PSSetShaderResources(0, 1, &g_res.comp_depth_srv);
        } else {
            ctx->PSSetShaderResources(0, 2, ps_srvs);
        }

        ctx->OMSetBlendState(g_res.blend_modes[o.blend_mode], bf, 0xFFFFFFFF);

        // FALLBACK DEPTH-WRITE PARITY (screenshot conviction: a carried
        // frame showed a distant box THROUGH a near box and the concave
        // staircase with jumbled faces - both are the same defect). The
        // injection depth-writes every composite-eligible solid by
        // contract; the late fallback drew mode-0 solids test-only, so on
        // exactly the carried frames our meshes could occlude neither
        // each other nor themselves. Mirror the injection's contract
        // here: composite-eligible solids write depth on the fallback
        // too. Late depth writes are as valid as late color writes - the
        // mesh IS at that depth - and mode-1 meshes have exercised this
        // very path since the beginning.
        ID3D11DepthStencilState* dss =
            (o.mode == DepthMode::Off)       ? g_res.dss_off :
            (o.mode == DepthMode::TestWrite) ? g_res.dss_test_write :
            is_composite_eligible(o)         ? g_res.dss_test_write :
                                               g_res.dss_test;

        ctx->OMSetDepthStencilState(dss, 0);
        const int mid = (o.mesh >= 0 && o.mesh < KH_MESH_COUNT) ? o.mesh : 0;

        if (mid != bound_mesh) {
            ctx->IASetVertexBuffers(0, 1, &g_res.mesh_vb[mid], &stride, &offset);
            bound_mesh = mid;
        }

        ctx->Draw(mesh_vertex_count(mid), 0);
    }

    // --- Fullscreen chain (single-resolve ping-pong) ---
    // The scene is resolved ONCE (re-resolved here only so the chain sees the
    // meshes just drawn); every pass then reads one single-sample chain target
    // and writes its fully composited output opaquely to the other - the
    // blend arithmetic happens in-shader (see the chain packing in PSEffect),
    // which is exact because for a fullscreen pass the blend destination and
    // the sampled scene are the same image. A final opaque blit paints the
    // chain result onto the engine's MSAA target. Bandwidth: one resolve +
    // F single-sample draws + one blit, instead of F resolves + F MSAA draws.
    if (!fullscreen.empty() && effects_ready) {
        std::string chain_err = meshes.empty() ? "" : ensure_scene_capture(dev, ctx);
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
// Game-thread lighting staging. Runs from flush_frame BEFORE the graphics
// lock is taken: it makes SQF/engine calls and must not extend the
// render-thread park. Publication to the render-thread-visible globals
// happens inside flush_locked.
// ===========================================================================

inline void stage_world_lighting() {
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

    // FOG IS NOT LIGHTING (settings-independence): staging used to bail
    // without a lit object, so an UNLIT mesh stood unfogged in a fogged
    // world. Atmospheric occlusion applies to every world-space mesh;
    // stage it whenever the flush runs.

    try {
        // Typed fogParams wrapper - the LAST scripted lighting fetch: the
        // direction comes from the cascade-derived latch and the colors
        // from the located lighting block. Fog params stay scripted
        // because value and base are NOT separable from the block - only
        // decay and the density-at-camera product appear in it - and the
        // staged decay is half the block probe's structural anchor.
        g_fog_staged_valid = false;
        const auto fp = sqf::fog_params();
        g_fog_staged[0] = fp.value;
        g_fog_staged[1] = fp.decay;
        g_fog_staged[2] = fp.base;
        g_fog_staged_valid = true;
    } catch (...) {
        // fog unavailable this frame: the fog term stands down
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
    // cold_t0: first flush whose draw list holds a composite-eligible mesh
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
    std::vector<std::pair<uint64_t, RenderObject>> passes;   // key = creation seq

    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);

        for (const auto& kv : g_draw_list) {
            if (!(kv.second.visible && kv.second.fullscreen && kv.second.affect_ui)) continue;
            bool expired = false;
            const float env = lifetime_envelope(kv.second, snapshot_now, expired);
            if (expired) continue;   // the scene flush owns the erasure
            RenderObject o = kv.second;
            o.color[3] *= env;
            passes.emplace_back(o.seq, o);
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

        // Overlay solid meshes need the camera for SolidMask's band term.
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

    g_draw3d_eh_active = true;
}

// Pure-diagnostic counter reset: the stats ARM (first getRenderStats call)
// and the mission-end session destroy both zero these. Functional counters
// (view_locks / lock_wipes / relock_forced pairing, probe hits, streaks,
// backoff) are deliberately excluded here - resetting them mid-session
// would re-open the loose cold lock bar or kill mirror consumers; they
// reset only in reset_session_state below.
inline void reset_stat_counters() {
    g_stats = RenderStats{};
    g_stage_total = 0; g_stage_rej_vis = 0; g_stage_rej_exp = 0;
    g_recv_term_skips = 0; g_recv_wipes = 0;
    g_sun_jump_refused = 0;
    g_cast_frozen_fires = 0;
    g_rt_resolve_true = 0; g_rt_resolve_false = 0;
    g_rt_half_accepts = 0; g_sweep_gap_resets = 0; g_rt_last_rej_w = 0;
    g_inj_guard_off = 0; g_flush_fallback_draws = 0; g_flush_latch_pvs = 0;
    g_flush_pv_repairs = 0; g_flush_repaint_saves = 0; g_flush_anomaly_carries = 0;
    g_sun_map_skips = 0;
    g_fl_fallback_ms = 0; g_fl_anom_skip_ms = 0;
    g_flush_slot_keeps = 0;
    g_keep_stamp_rejects = 0;
    g_keep_stale_skips = 0;
    g_cast_arm_lost_ms = 0;
    g_cc_postflush_redraws = 0; g_cc_pf_last_draws = 0; g_cc_pf_last_ms = 0;
    g_ms_frames = 0; g_ms_ms = 0; g_ms_near = -1.0f;
    g_skybind_reads = 0; g_skybind_hits = 0; g_skybind_minbw = 0;
    g_skybind_maxbw = 0; g_skybind_slots = 0; g_skybind_off1 = 0;
    g_skybind_maxbw_vs = 0; g_skybind_offs_seen = 0;
    g_viewbind_scans = 0; g_cascbind_scans = 0;
    g_loc_scan_uploads = 0; g_loc_max_cb_floats = 0;
    g_comp_compiles = 0;
    g_ui_poll_attempts = 0;
    g_ls.latches = 0; g_ls.resolve_hits = 0; g_ls.resolve_draws = 0;
    g_ls.resolve_cb_found = 0; g_ls.resolve_gated = 0;
    g_ls.band_captures = 0; g_ls.band_bail_pv = 0; g_ls.band_bail_off = 0;
    g_ls.band_bail_border = 0; g_ls.band_bail_slot = 0; g_ls.band_bail_time = 0;
    g_ls.band_bail_view = 0; g_ls.seal_completions = 0; g_ls.band_prov_skips = 0;
    g_ls.band_bail_quality = 0;
    g_ls.cold_pub_rejects = 0; g_ls.frame_view_hits = 0;

    for (int b = 0; b < 8; ++b) g_ls.band[b].copies = 0;

    g_mask.cast_arms_lost = 0; g_mask.arms_lost_miss = 0;
    g_mask.analytic_casts = 0; g_mask.cast_batches = 0; g_mask.mask_rtv_swaps = 0;
}

// FULL SESSION DESTROY (operator requirement): everything learned,
// published, staged, counted or derived dies with the mission - the next
// mission starts from the same nothing a fresh process would. Device
// objects and locator/probe state go through release_shadow_device_state()
// + g_res.release() (the caller runs those first, under the graphics lock);
// this resets what those don't. Must run with the render thread PARKED:
// g_ro, the mask/live structs and the hoisted memories are render-thread
// state.
inline void reset_session_state() {
    reset_stat_counters();
    g_stats_armed.store(false, std::memory_order_relaxed);
    // the lifetime pairing trio is session-scoped under full destroy
    g_view_relock_forced = 0; g_lock_wipes = 0; g_ls.view_locks = 0;
    g_ls.pub_rej_streak = 0; g_ls.view_src_miss = 0;
    g_ls.last_publish_rot_err = 1.0f;
    g_ls.pub_exact_ms = 0;
    g_ls.view_best_rot = -1.0f; g_ls.view_best_trans = -1.0f;
    g_ls.cam[0] = g_ls.cam[1] = g_ls.cam[2] = 0.0f;
    g_ls.stamp_counter = 0;
    // published / staged / derived lighting and its debounce latch
    g_sun_valid = false;
    g_sun_dir_engine[0] = 0.0f; g_sun_dir_engine[1] = 1.0f; g_sun_dir_engine[2] = 0.0f;
    g_sun_dir_derived_valid = false;
    g_sun_dir_derived[0] = 0.0f; g_sun_dir_derived[1] = 1.0f; g_sun_dir_derived[2] = 0.0f;
    g_sun_derived_samples = 0;
    g_sun_jump_pending = false;
    g_skysun_ref_valid = false;
    g_skysun_ref[0] = g_skysun_ref[1] = g_skysun_ref[2] = 0.0f;
    g_pub_valid = false; g_cand_hold = 0;
    g_fog_staged_valid = false; g_fog_valid = false;

    for (int k = 0; k < 3; ++k) { g_fog[k] = 0.0f; g_fog_staged[k] = 0.0f; }

    // hoisted lock memories
    g_pub_first = -1.0f;
    g_prewarm_cand_res = nullptr; g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr; g_coldlock_cand_off = 0;
    g_last_pv_ms = 0;
    g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_slot_keep_m22 = 0.0f; g_slot_keep_m32 = 0.0f;
    g_slot_keep_near = -1.0f; g_slot_keep_ms = 0;
    // render-thread cycle state (the caller's graphics lock parks that thread)
    // Hook DISARM: with the tracked context cleared, every installed hook
    // bails at its very first pointer compare on every context - the whole
    // pipeline is a single relaxed load per call until the next render
    // command re-stores the context via ensure_reorder_hook.
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_ro = ReorderState{};
    g_sr = ShadowReconState{};
    g_sr_cap_view = ShadowCapture43{};
    g_sr_cap_vp = ShadowCapture43{};
    g_sr_cascades = ShadowCascadeSet{};
    g_sr_cascades_scratch = ShadowCascadeSet{};
    memset(g_sr_last_trans, 0, sizeof(g_sr_last_trans));
    g_sr_last_trans_count = 0;
    g_sr_best_dot44 = 0.0f; g_sr_best_dot43 = 0.0f;
    g_reorder_render_tid.store(0, std::memory_order_relaxed);
    g_main_depth_w = 0; g_main_depth_h = 0; g_wrong_pass_streak = 0;
    g_proj_last_m32 = 0.0f;
    g_mask = ShadowMaskState{};   // device pointers already nulled by the release
    g_trig_rej_vp[0] = g_trig_rej_vp[1] = -1.0f;
    g_trig_acc_vp[0] = g_trig_acc_vp[1] = -1.0f;
    g_fog_dbg[0] = g_fog_dbg[1] = g_fog_dbg[2] = g_fog_dbg[3] = 0.0f;
    g_proj_locator_ever = false;
    g_comp_fail_streak = 0; g_comp_next_retry = 0.0f; g_comp_last_err.clear();
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
    // NOTHING ACTIVATES HERE (operator requirement: state stays dead until
    // a command demands it). Draw3D registers at the first addRender3D /
    // addPostFX; the UI overlay driver registers at the first UI-phase
    // demand (affectUI pass or flushUIRender). This only clears leftovers.
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();
}

inline void on_mission_end() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();

    // FULL SESSION DESTROY: device objects, learned locations, probes,
    // mirrors, published lighting, stats - everything - dies with the
    // mission. The render thread must be PARKED before device objects and
    // render-thread state are touched; the graphics lock is that
    // invariant. The draw list is already empty, so the injection is a
    // no-op even before the lock lands - a transiently contended lock only
    // delays the release by a retry. The vtable hooks stay installed
    // (uninstalling live draw hooks is the one genuinely dangerous
    // teardown; disarmed they cost a pointer compare per draw) - but
    // everything they FEED is destroyed, and nothing re-arms until a
    // command re-creates work.
    for (int attempt = 0; attempt < 8; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) continue;
        release_shadow_device_state();
        g_res.release();
        reset_session_state();
        break;
    }

    // Retry-exhaustion backstop: if every acquisition failed, the destroy
    // above never ran and the next mission would inherit an armed pipeline.
    // The disarm itself is a relaxed atomic store the render thread only
    // ever READS at each hook's entry compare - safe without the park
    // (unlike the state/device teardown, which stays lock-gated). Idempotent
    // on the success path: reset_session_state already stored the null.
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
}

// ===========================================================================
// Retained-mode API (called by the SQF wrappers)
// ===========================================================================

inline std::string add_render_object(const RenderObject& obj) {
    ensure_draw_eh();
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const std::string handle = make_render_uid();
    RenderObject& stored = g_draw_list[handle];
    stored = obj;
    stored.seq = ++g_next_seq;   // creation order (fullscreen pass chaining)
    stored.birth_time = effect_time_seconds();
    return handle;
}

inline bool remove_render_object(const std::string& handle) {
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
// Adds a persistent mesh drawn every frame by the internal Draw3D EH until
// removed. Callable from ANY context (scheduled, unscheduled, callbacks).
//   mode:      0 = depth test (default), 1 = test + depth write, 2 = overlay
//   sceneRead: BOOL, shorthand for a tinted scene-read surface
//              (effect "colorgrade" at neutral defaults: scene through the
//              mesh, tinted by color.rgb, blended by color.a)
//   effect:    STRING or SCALAR - screen-space effect applied inside the
//              mesh's footprint: "solid" 0, "invert" 1, "colorgrade" 2,
//              "vignette" 3, "chromatic" 4, "grain" 5, "sharpen" 6,
//              "blur" 7, "bloom" 8, "distortion" 9, "outline" 10, "pulse" 11
//   params:    ARRAY of up to 8 numbers, effect-specific (see set_effect_params
//              for meanings and defaults; omitted entries take defaults)
//   band:      [minDist, maxDist, falloff?] - additionally confines the mesh's
//              effect to a camera-distance band (maxDist <= 0 = unbounded)
//   lit:       (index 10) BOOL, or ARRAY [ambient, diffuse] - shade the
//              mesh with the engine's own sun/moon light (cascade-derived
//              direction, located-block colors) and per-pixel world
//              shadowing. Defaults ambient 0.4 / diffuse 0.6; with the
//              lighting block live,
//              [1, 1] is engine-true brightness.
//   mesh:      (index 11) STRING or SCALAR - registry mesh: "box"/"cube"
//              (0, default) or "steps"/"test" (1, a concave 3-step
//              staircase for exercising self-shadowing). Local space is
//              normalized to [-0.5, 0.5]^3 and scaled per axis by 'size'.
// Solid, non-overlay meshes are ALWAYS composited: injected into the frame
// BEFORE the engine draws its translucents, with depth written, so the
// engine itself composites smoke/particles against them pixel-perfectly
// (automatic fallback to the post-scene flush if the draw hook is
// unavailable). Effect and overlay meshes render on the flush path.
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

            }
        }

        if (arr.size() > 11) {
            const int mid = RenderIntegration::mesh_id_from_gv(arr[11]);
            if (mid < 0) return game_value("unknown mesh (box | steps, or a registry index)");
            obj.mesh = mid;
        }

        if (arr.size() > 12 && arr[12].type_enum() == game_data_type::BOOL) {
            obj.far_vis = static_cast<bool>(arr[12]);   // visible beyond max view distance
        }

        return game_value(RenderIntegration::add_render_object(obj));
    } catch (const std::exception& e) {
        report_error(std::string("addRender3D: ") + e.what());
        return game_value(std::string("EXCEPTION: ") + e.what());
    } catch (...) {
        report_error("addRender3D: unknown exception");
        return game_value("EXCEPTION: unknown");
    }
}

// ---------------------------------------------------------------------------
// Update commands, SPLIT by object kind. 3D mesh objects (addRender3D) and
// fullscreen passes (addPostFX / addLocalPostFX) share a handle space and
// several properties, but their non-shared properties must not overlap:
// each command owns exactly its kind, rejects the other's handles, and the
// genuinely common set lives in ONE helper so the two can never drift.
// ---------------------------------------------------------------------------

// Property set BOTH kinds own. Returns 1 = applied, 0 = recognized but the
// value was invalid, -1 = not a shared property (fall through to the
// caller's kind-specific set).
static int kh_apply_shared_prop(RenderIntegration::RenderObject& obj,
                                const std::string& prop, const game_value& val) {
    using namespace RenderIntegration;

    if (prop == "color") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        auto& col = val.to_array();
        for (size_t i = 0; i < 4 && i < col.size(); ++i) obj.color[i] = static_cast<float>(col[i]);
        return 1;
    }

    if (prop == "visible") {
        obj.visible = static_cast<bool>(val);
        return 1;
    }

    if (prop == "params") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        set_effect_params(obj, &val.to_array());
        return 1;
    }

    if (prop == "blend") {
        const int bm = blend_id_from_gv(val);
        if (bm < 0) return 0;
        obj.blend_mode = bm;
        return 1;
    }

    if (prop == "band") {
        if (val.type_enum() != game_data_type::ARRAY) return 0;
        auto& band = val.to_array();

        if (band.size() < 2) {
            obj.banded = false;   // empty/short array clears the band
        } else {
            obj.banded = true;
            obj.band_min = static_cast<float>(band[0]);
            obj.band_max = static_cast<float>(band[1]);
            if (band.size() >= 3) obj.band_falloff = static_cast<float>(band[2]);
        }

        return 1;
    }

    if (prop == "duration") {
        if (!parse_duration_gv(val, obj)) return 0;
        obj.birth_time = effect_time_seconds();   // re-arm from now
        return 1;
    }

    return -1;
}

// updateRender3D [handle, property, value] -> BOOL
// 3D mesh objects ONLY (addRender3D handles); fullscreen passes belong to
// updatePostFX. Properties: "position" [x,y,zASL] | "size" number|[x,y,z] |
// "mesh" string/scalar ("box"/"cube" 0, "steps"/"test" 1) | "color"
// [r,g,b,a] | "mode" 0..2 | "visible" bool | "sceneread" bool | "effect"
// string/scalar | "params" array (resets omitted entries to the effect's
// defaults) | "blend" string | "band" [minDist, maxDist, falloff?] ([]
// clears) | "lit" bool or [ambient, diffuse] ("lighting"
// accepted as an alias) | "duration" number or [fadeIn, hold, fadeOut].
// Returns false for unknown handles, fullscreen handles, unknown
// properties, or invalid values.
static game_value update_render3d_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 3) return game_value(false);
        if (arr[0].type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arr[0]);
        std::string prop = static_cast<std::string>(arr[1]);
        std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
        std::lock_guard<std::mutex> g(RenderIntegration::g_draw_list_mutex);
        auto it = RenderIntegration::g_draw_list.find(handle);
        if (it == RenderIntegration::g_draw_list.end()) return game_value(false);
        auto& obj = it->second;
        if (obj.fullscreen) return game_value(false);   // that handle belongs to updatePostFX

        const int shared = kh_apply_shared_prop(obj, prop, arr[2]);
        if (shared >= 0) return game_value(shared == 1);

        if (prop == "position") {
            auto& pos = arr[2].to_array();
            if (pos.size() < 3) return game_value(false);
            obj.pos[0] = static_cast<float>(pos[0]);
            obj.pos[1] = static_cast<float>(pos[1]);
            obj.pos[2] = static_cast<float>(pos[2]);
        } else if (prop == "size") {
            if (!RenderIntegration::read_vec3_or_uniform(arr[2], obj.size)) return game_value(false);
        } else if (prop == "mesh") {
            const int mid = RenderIntegration::mesh_id_from_gv(arr[2]);
            if (mid < 0) return game_value(false);
            obj.mesh = mid;
        } else if (prop == "mode") {
            int m = static_cast<int>(static_cast<float>(arr[2]));
            if (m < 0 || m > 2) return game_value(false);
            obj.mode = static_cast<RenderIntegration::DepthMode>(m);
        } else if (prop == "sceneread") {
            obj.effect = static_cast<bool>(arr[2]) ? 2 : 0;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "effect") {
            const int e = RenderIntegration::effect_id_from_gv(arr[2]);
            if (e < 0) return game_value(false);
            obj.effect = e;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "lit" || prop == "lighting") {
            // BOOL toggles, ARRAY [ambient, diffuse] configures.
            if (arr[2].type_enum() == game_data_type::ARRAY) {
                auto& la = arr[2].to_array();

                if (la.size() < 1) {
                    obj.lit = false;   // empty array disables the shading
                } else {
                    obj.lit = true;
                    obj.light_ambient = static_cast<float>(la[0]);
                    if (la.size() >= 2) obj.light_diffuse = static_cast<float>(la[1]);

                }
            } else {
                obj.lit = static_cast<bool>(arr[2]);
            }
        } else if (prop == "farvis") {
            obj.far_vis = static_cast<bool>(arr[2]);   // visible beyond max view distance
        } else {
            return game_value(false);
        }

        return game_value(true);
    } catch (const std::exception& e) {
        report_error(std::string("updateRender3D: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("updateRender3D: unknown exception");
        return game_value(false);
    }
}

// updatePostFX [handle, property, value] -> BOOL
// Fullscreen post-processing passes ONLY (addPostFX / addLocalPostFX
// handles); 3D mesh objects belong to updateRender3D. Properties:
// "effect" string/scalar (fullscreen effects only, id > 0) | "params"
// array | "color" [r,g,b,a] | "blend" string | "band" [minDist, maxDist,
// falloff?] ([] clears) | "visible" bool | "ui" bool (post-tonemap phase)
// | "duration" number or [fadeIn, hold, fadeOut] | "position" [x,y,zASL]
// (the localized volume's center) | "radius" number|[x,y,z] | "falloff"
// scalar | "shape" "sphere"|"cube" | "localsphere" [radius, falloff?]
// (enables the world-space volume mask; [] clears it). Returns false for
// unknown handles, 3D-object handles, unknown properties, or invalid
// values.
static game_value update_post_fx_sqf(game_value_parameter args) {
    try {
        auto& arr = args.to_array();
        if (arr.size() < 3) return game_value(false);
        if (arr[0].type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arr[0]);
        std::string prop = static_cast<std::string>(arr[1]);
        std::transform(prop.begin(), prop.end(), prop.begin(), ::tolower);
        std::lock_guard<std::mutex> g(RenderIntegration::g_draw_list_mutex);
        auto it = RenderIntegration::g_draw_list.find(handle);
        if (it == RenderIntegration::g_draw_list.end()) return game_value(false);
        auto& obj = it->second;
        if (!obj.fullscreen) return game_value(false);   // that handle belongs to updateRender3D

        const int shared = kh_apply_shared_prop(obj, prop, arr[2]);
        if (shared >= 0) return game_value(shared == 1);

        if (prop == "position") {
            auto& pos = arr[2].to_array();
            if (pos.size() < 3) return game_value(false);
            obj.pos[0] = static_cast<float>(pos[0]);
            obj.pos[1] = static_cast<float>(pos[1]);
            obj.pos[2] = static_cast<float>(pos[2]);
        } else if (prop == "effect") {
            const int e = RenderIntegration::effect_id_from_gv(arr[2]);
            if (e <= 0) return game_value(false);   // a fullscreen pass without an effect is meaningless
            obj.effect = e;
            RenderIntegration::set_effect_params(obj, nullptr);
        } else if (prop == "ui") {
            obj.affect_ui = static_cast<bool>(arr[2]);
            if (obj.affect_ui) RenderIntegration::ensure_ui_driver();   // UI phase demanded
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
        if (arg.type_enum() != game_data_type::STRING) return game_value(false);
        const std::string handle = static_cast<std::string>(arg);

        if (handle.empty() || handle == "all") {
            RenderIntegration::clear_render_objects();
            return game_value(true);
        }

        return game_value(RenderIntegration::remove_render_object(handle));
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
// they are active, mode-1 meshes do not write depth (read-only DSV phase).
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

        // UI phase demanded: start the overlay control driver (state stays
        // dead otherwise - the operator's no-passive-activation rule)
        if (obj.affect_ui) RenderIntegration::ensure_ui_driver();

        if (arr.size() > 6) {
            if (!RenderIntegration::parse_duration_gv(arr[6], obj)) {
                return game_value("duration must be seconds or [fadeIn, hold, fadeOut]");
            }
        }

        return game_value(RenderIntegration::add_render_object(obj));
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
        // OPT-IN (see g_stats_armed): first call arms + zeroes the pure
        // diagnostics and returns a status pair; stats flow from call two.
        if (!RenderIntegration::g_stats_armed.exchange(true, std::memory_order_relaxed)) {
            RenderIntegration::reset_stat_counters();
            auto_array<game_value> pair;
            pair.push_back(game_value("status"));
            pair.push_back(game_value("armed"));
            auto_array<game_value> armed_out;
            armed_out.push_back(game_value(std::move(pair)));
            return game_value(std::move(armed_out));
        }

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
        out.push_back(kv("compositeMeshes", RenderIntegration::g_stats.composite_meshes));
        out.push_back(kv("compositeSkips", RenderIntegration::g_stats.composite_skips));
        out.push_back(kv("compositeAmbiguous", RenderIntegration::g_stats.composite_ambiguous));
        out.push_back(kv("compositeProjLock", RenderIntegration::g_stats.composite_proj_lock));
        out.push_back(kv("compositeRearms", RenderIntegration::g_stats.composite_rearms));
        out.push_back(kv("compositeRejSpan", RenderIntegration::g_stats.composite_rej_span));
        out.push_back(kv("compositeRejVerify", RenderIntegration::g_stats.composite_rej_verify));
        out.push_back(kv("compositeRejFloor", RenderIntegration::g_stats.composite_rej_floor));
        out.push_back(kv("compositeSlotEncodes", RenderIntegration::g_stats.composite_slot_encodes));
        out.push_back(kv("compositeFarPhaseSkips", RenderIntegration::g_stats.composite_far_phase_skips));
        out.push_back(kv("compositeFarInjects", RenderIntegration::g_stats.composite_far_injects));
        out.push_back(kv("compositeKeepEncodes", RenderIntegration::g_stats.composite_keep_encodes));
        out.push_back(kv("compositeAnomalySkips", RenderIntegration::g_stats.composite_anomaly_skips));
        out.push_back(kv("sunDepthPasses", RenderIntegration::g_stats.sun_depth_passes));
        out.push_back(kv("sunDepthCasters", RenderIntegration::g_stats.sun_depth_casters));
        out.push_back(kv("sunJumpFlushes", RenderIntegration::g_stats.sun_jump_flushes));
        out.push_back(kv("castArmsLost", RenderIntegration::g_mask.cast_arms_lost));

        {
            uint32_t bsv = 0;

            for (int b = 0; b < 8; ++b) {
                if (RenderIntegration::g_ls.band[b].valid) ++bsv;
            }

            out.push_back(kv("bandSlotsValid", bsv));
        }

        out.push_back(kv("maskFailFmt", RenderIntegration::g_mask.fail_fmt));
        out.push_back(kvf("fireFovX", RenderIntegration::g_mask.last_fire_fov[0]));
        out.push_back(kvf("fireFovY", RenderIntegration::g_mask.last_fire_fov[1]));
        out.push_back(kvf("fireDimsW", RenderIntegration::g_mask.last_fire_dims[0]));
        out.push_back(kvf("fireDimsH", RenderIntegration::g_mask.last_fire_dims[1]));
        out.push_back(kvf("fireRotErr", RenderIntegration::g_mask.last_fire_rot_err));
        out.push_back(kv("encVpRejects", RenderIntegration::g_stats.enc_vp_rejects));
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
        out.push_back(kvf("coldFirstStaged", RenderIntegration::g_mask.cold_first_stage));
        out.push_back(kv("coldGNoDsv", RenderIntegration::g_mask.cold_g_nodsv));
        out.push_back(kv("coldGFloor", RenderIntegration::g_mask.cold_g_floor));
        out.push_back(kv("coldGTid", RenderIntegration::g_mask.cold_g_tid));
        out.push_back(kv("coldCastMiss", RenderIntegration::g_mask.cold_cast_miss));
        out.push_back(kv("coldPubRejects", RenderIntegration::g_ls.cold_pub_rejects));
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
        out.push_back(kv("bandBailView", RenderIntegration::g_ls.band_bail_view));
        out.push_back(kv("bandBailQuality", RenderIntegration::g_ls.band_bail_quality));
        out.push_back(kv("bandProvSkips", RenderIntegration::g_ls.band_prov_skips));
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
        {   // engine HDR sun magnitude (getLighting retired): peak of the
            // located block's sun lane; zero under moonlight by design
            const float* sl = RenderIntegration::g_light_probe.nb + 16;
            const float sm = sl[0] > sl[1] ? (sl[0] > sl[2] ? sl[0] : sl[2]) : (sl[1] > sl[2] ? sl[1] : sl[2]);
            out.push_back(kvf("sunBrightness", sm));
        }
        out.push_back(kv("lightLocValid", RenderIntegration::g_light_probe.valid ? 1u : 0u));
        out.push_back(kv("lightLocOff", RenderIntegration::g_light_probe.off));
        out.push_back(kv("lightLocFloats", RenderIntegration::g_light_probe.floats));
        out.push_back(kv("lightLocMeta", static_cast<uint64_t>(RenderIntegration::g_light_probe.meta)));
        out.push_back(kv("lightLocHits", RenderIntegration::g_light_probe.hits));
        out.push_back(kv("lightLocMisses", RenderIntegration::g_light_probe.misses));
        out.push_back(kv("lightLocRelocs", RenderIntegration::g_light_probe.relocs));
        out.push_back(kv("lightLocNbBase", RenderIntegration::g_light_probe.nb_base));
        out.push_back(kvf("lightLocErr", RenderIntegration::g_light_probe.last_err));
        out.push_back(kvf("lightLocAge", RenderIntegration::effect_time_seconds() - RenderIntegration::g_light_probe.last_confirm));
        out.push_back(kvf("lightLocMode", RenderIntegration::g_light_probe.last_mode));


        out.push_back(kv("skyLocValid", RenderIntegration::g_sky_probe.valid ? 1u : 0u));
        out.push_back(kv("skyLocFloats", RenderIntegration::g_sky_probe.floats));
        out.push_back(kv("skyLocHits", RenderIntegration::g_sky_probe.hits));
        out.push_back(kv("skyLocMisses", RenderIntegration::g_sky_probe.misses));
        out.push_back(kvf("skyLocAge", RenderIntegration::effect_time_seconds() - RenderIntegration::g_sky_probe.last_confirm));
        out.push_back(kv("skyBindReads", RenderIntegration::g_skybind_reads));
        out.push_back(kv("skyBindHits", RenderIntegration::g_skybind_hits));
        out.push_back(kv("skyBindMinBw", static_cast<uint64_t>(RenderIntegration::g_skybind_minbw)));
        out.push_back(kv("skyBindMaxBw", static_cast<uint64_t>(RenderIntegration::g_skybind_maxbw)));
        out.push_back(kv("skyBindSlots", static_cast<uint64_t>(RenderIntegration::g_skybind_slots)));
        out.push_back(kv("skyBindOff1", static_cast<uint64_t>(RenderIntegration::g_skybind_off1)));
        out.push_back(kv("skyBindMaxBwVs", static_cast<uint64_t>(RenderIntegration::g_skybind_maxbw_vs)));
        out.push_back(kv("viewBindScans", RenderIntegration::g_viewbind_scans));
        out.push_back(kv("stageTotal", static_cast<uint64_t>(RenderIntegration::g_stage_total)));
        out.push_back(kv("stageRejVis", static_cast<uint64_t>(RenderIntegration::g_stage_rej_vis)));
        out.push_back(kv("recvTermSkips", RenderIntegration::g_recv_term_skips));
        out.push_back(kv("recvWipes", RenderIntegration::g_recv_wipes));
        out.push_back(kv("sunJumpRefused", RenderIntegration::g_sun_jump_refused));
        out.push_back(kv("viewRelockForced", RenderIntegration::g_view_relock_forced));
        out.push_back(kv("lockWipes", RenderIntegration::g_lock_wipes));
        out.push_back(kv("stageRejExp", static_cast<uint64_t>(RenderIntegration::g_stage_rej_exp)));
        out.push_back(kv("cascBindScans", RenderIntegration::g_cascbind_scans));
        out.push_back(kv("skyBindOffsSeen", RenderIntegration::g_skybind_offs_seen));



        out.push_back(kv("castFrozenFires", RenderIntegration::g_cast_frozen_fires));
        out.push_back(kv("castRtResolveTrue", RenderIntegration::g_rt_resolve_true));
        out.push_back(kv("castRtResolveFalse", RenderIntegration::g_rt_resolve_false));
        out.push_back(kv("castRtHalfAccepts", RenderIntegration::g_rt_half_accepts));
        out.push_back(kv("sweepGapResets", RenderIntegration::g_sweep_gap_resets));
        out.push_back(kv("injGuardOff", RenderIntegration::g_inj_guard_off));
        out.push_back(kv("flushFallbackDraws", RenderIntegration::g_flush_fallback_draws));
        out.push_back(kv("flushLatchPvs", RenderIntegration::g_flush_latch_pvs));
        out.push_back(kv("flushPvRepairs", RenderIntegration::g_flush_pv_repairs));
        out.push_back(kv("flushRepaintSaves", RenderIntegration::g_flush_repaint_saves));
        out.push_back(kv("flushAnomalyCarries", RenderIntegration::g_flush_anomaly_carries));
        out.push_back(kv("castArmsLostMiss", RenderIntegration::g_mask.arms_lost_miss));
        out.push_back(kv("sunMapSkips", RenderIntegration::g_sun_map_skips));
        auto age_s = [](uint64_t ms) {
            return ms == 0 ? -1.0f :
                static_cast<float>(RenderIntegration::steady_now_ms() - ms) * 0.001f;
        };
        out.push_back(kvf("flAgeFallbackS", age_s(RenderIntegration::g_fl_fallback_ms)));
        out.push_back(kvf("flAgeAnomSkipS", age_s(RenderIntegration::g_fl_anom_skip_ms)));
        out.push_back(kv("ccPostFlushRedraws", RenderIntegration::g_cc_postflush_redraws));
        out.push_back(kv("ccPfLastDraws", static_cast<uint64_t>(RenderIntegration::g_cc_pf_last_draws)));
        out.push_back(kvf("ccPfLastAgeS", age_s(RenderIntegration::g_cc_pf_last_ms)));
        out.push_back(kv("missFrames", RenderIntegration::g_ms_frames));
        out.push_back(kvf("missLastNear", RenderIntegration::g_ms_near));
        out.push_back(kv("flushSlotKeeps", RenderIntegration::g_flush_slot_keeps));
        out.push_back(kv("keepStampRejects", RenderIntegration::g_keep_stamp_rejects));
        out.push_back(kv("keepStaleSkips", RenderIntegration::g_keep_stale_skips));
        out.push_back(kvf("castArmLostAgeS", age_s(RenderIntegration::g_cast_arm_lost_ms)));
        out.push_back(kv("viewSrcMisses", static_cast<uint64_t>(RenderIntegration::g_ls.view_src_miss)));
        out.push_back(kv("viewCandN", static_cast<uint64_t>(RenderIntegration::g_ls.vc_n)));
        out.push_back(kvf("viewPubRotErr", RenderIntegration::g_ls.last_publish_rot_err));
        out.push_back(kvf("viewPubExactAgeS", age_s(RenderIntegration::g_ls.pub_exact_ms)));
        out.push_back(kvf("missLastAgeS", age_s(RenderIntegration::g_ms_ms)));
        out.push_back(kv("rtLastRejW", static_cast<uint64_t>(RenderIntegration::g_rt_last_rej_w)));
        out.push_back(kvf("fogColR", RenderIntegration::g_fog_dbg[0]));
        out.push_back(kvf("fogColG", RenderIntegration::g_fog_dbg[1]));
        out.push_back(kvf("fogColB", RenderIntegration::g_fog_dbg[2]));
        out.push_back(kvf("fogEnabled", RenderIntegration::g_fog_dbg[3]));
        out.push_back(kvf("fogTgtR", RenderIntegration::g_sky_probe.nb[4]));
        out.push_back(kvf("fogTgtG", RenderIntegration::g_sky_probe.nb[5]));
        out.push_back(kvf("fogTgtB", RenderIntegration::g_sky_probe.nb[6]));
        out.push_back(kvf("trigRejMin", RenderIntegration::g_trig_rej_vp[0]));
        out.push_back(kvf("trigRejMax", RenderIntegration::g_trig_rej_vp[1]));
        out.push_back(kvf("trigAccMin", RenderIntegration::g_trig_acc_vp[0]));
        out.push_back(kvf("trigAccMax", RenderIntegration::g_trig_acc_vp[1]));

        {   // finest published cascade, world meters per shadow texel: the
            // receive-resolution question in one number (compare across
            // sessions; if it grew, fine cascades stopped entering the
            // table). Diagnostic read of render-written state, like the
            // rest of the stats.
            float finest = -1.0f;
            int   valid = 0;

            for (uint32_t i = 0; i < RenderIntegration::KH_LIVE_MAX_CASCADES; ++i) {
                const auto& e = RenderIntegration::g_ls.entries[i];
                if (e.tile[2] <= 0.0f || e.stamp == 0) continue;
                valid++;
                const float ilen = sqrtf(e.m[0] * e.m[0] + e.m[3] * e.m[3] + e.m[6] * e.m[6]);
                if (ilen <= 1e-9f) continue;
                const float texels = e.tile[2] * static_cast<float>(RenderIntegration::g_ls.atlas_size);
                if (texels <= 0.0f) continue;
                const float wpt = (2.0f / ilen) / texels;
                if (finest < 0.0f || wpt < finest) finest = wpt;
            }

            out.push_back(kvf("liveFinestWpt", finest));
            out.push_back(kv("liveValidEntries", static_cast<uint64_t>(valid)));
        }

        out.push_back(kv("locScanUploads", RenderIntegration::g_loc_scan_uploads));
        out.push_back(kv("locMaxCbFloats", RenderIntegration::g_loc_max_cb_floats));
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

        return game_value(RenderIntegration::add_render_object(obj));
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
        RenderIntegration::ensure_ui_driver();   // explicit UI-render demand is an enabling command
        return game_value(RenderIntegration::flush_ui_frame());
    } catch (const std::exception& e) {
        report_error(std::string("flushUIRender: ") + e.what());
        return game_value(false);
    } catch (...) {
        report_error("flushUIRender: unknown exception");
        return game_value(false);
    }
}