#pragma once

// The feature half (rendering_integration_features.hpp): the device resources: ensure_resources, the optional shaders,
// the effect, composite and depth-resolve shaders. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/white_placeholder.hpp (declared here for the uses below).
inline void white_ensure(ID3D11Device* dev);

// ENSURE_ONCE: one report per fault, not one per site. The same ensure error reaches several sites a frame (the
// flush, the injection, the seam, the UI flush, the sun pass, the mesh publish), and report_error_once_safe keys on
// the prefixed text, so the bare error is filed as the key first: the first site names itself, every later site
// finds the key and says nothing (two of the 256-message cap's slots per fault; ensure faults are few and distinct).
inline bool ensure_ok(const char* what, const std::string& err) {
    if (err.empty()) return true;
    if (err == SHADERS_COMPILING) return false;
    {
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        if (std::find(g_err_once.begin(), g_err_once.end(), err) != g_err_once.end()) return false;
        if (g_err_once.size() < 256) g_err_once.push_back(err);
    }
    report_error_once_safe(std::string("KH RenderIntegration: ") + what + ": " + err);
    return false;
}
// Non-fatal shader creation: a failed compile or create is reported once (the text names the entry point, so one
// failing entry does not silence the next) and leaves the slot null; every consumer gates on the pointer. label: the
// feature the report names ("KH RenderIntegration: <label>: ...").
inline void ps_optional(ID3D11Device* dev, const std::string& src, const char* entry,
                        const D3D_SHADER_MACRO* defs, ID3D11PixelShader** out, const char* label) {
    ID3DBlob* blob = nullptr;
    const std::string err = compile_shader(src.c_str(), entry, "ps_5_0", defs, &blob);
    if (err.empty() && blob) {
        const HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out);
        blob->Release();
        if (FAILED(hr)) {
            *out = nullptr;
            report_error_once_safe(std::string("KH RenderIntegration: ") + label + ": " + entry +
                                   " could not be created " + hr_str(hr));
        }
    } else if (!err.empty()) {
        // compile_shader's error text already names the entry.
        report_error_once_safe(std::string("KH RenderIntegration: ") + label + ": " + err);
    }
}
inline void vs_optional(ID3D11Device* dev, const std::string& src, const char* entry,
                        const D3D_SHADER_MACRO* defs, ID3D11VertexShader** out, const char* label) {
    ID3DBlob* blob = nullptr;
    const std::string err = compile_shader(src.c_str(), entry, "vs_5_0", defs, &blob);
    if (err.empty() && blob) {
        const HRESULT hr = dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out);
        blob->Release();
        if (FAILED(hr)) {
            *out = nullptr;
            report_error_once_safe(std::string("KH RenderIntegration: ") + label + ": " + entry +
                                   " could not be created " + hr_str(hr));
        }
    } else if (!err.empty()) {
        // compile_shader's error text already names the entry.
        report_error_once_safe(std::string("KH RenderIntegration: ") + label + ": " + err);
    }
}

// PREWARM_ALL: the two inline-source pixel shaders ensure_resources builds (the UI coverage reset, the
// premultiplying copy), named once so the prewarm batch queues exactly the source the build compiles.
static const char* const PS_ALPHA_ZERO_SRC =
    "float4 main() : SV_Target { return float4(0.0f, 0.0f, 0.0f, 0.0f); }";
static const char* const PS_PREMULT_SRC =
    "Texture2D khSrc : register(t0);"
    "float4 main(float4 pos : SV_Position) : SV_Target"
    "{ float4 c = khSrc.Load(int3(int2(pos.xy), 0));"
    "  return float4(c.rgb * c.a, c.a); }";
// WSEE - the chain's final write when its last pass read the scene's sample colours (fx_chain_run: that
// pass draws into the ping-pong target, this copies it into the scene target): one texel a pixel, no constants. Built
// by wsee_ensure, not ensure_resources (a third inline source, prewarmed with the two).
static const char* const PS_WSEE_COPY_SRC =
    "Texture2D khSrc : register(t0);"
    "float4 main(float4 pos : SV_Position) : SV_Target"
    "{ return khSrc.Load(int3(int2(pos.xy), 0)); }";

inline std::string ensure_resources(ID3D11Device* dev) {
    KH_PROF_SCOPE(PROF_ENSURE_RES);   // PROF.
    // No feature-level test: every unit is vs_5_0 / ps_5_0, so a feature level 10.x device (pre-2010 GPUs) fails
    // the first CreateVertexShader, and this is asked again each call.
    // The mod cache scan is a magic static that walks mod folders on disk; it must not first run on a shader worker
    // (ModFolderSearcher reentrancy). This site runs on the game or render thread before any pool arms.
    mod_cache_dirs();
    // The initialized fast path still tops up mesh VBs: the ensure after mesh_publish creates the new buffers.
    if (g_res.initialized) return ensure_mesh_vbs(dev);

    // CB_OFFSET_EARLY: StateBackup's offset-aware capture keys on g_cb_offsetting, and the white placeholder
    // (white_draw) runs one while the deferrals below return - before the caps read further down sets it. Read
    // here too, so the engine's b0 / b1 windows go back at their offsets in the compile window (a plain restore
    // rebinds a window at constant 0). The read below sets the same value.
    if (!g_cb_offsetting) {
        D3D11_FEATURE_DATA_D3D11_OPTIONS opt = {};
        g_cb_offsetting = SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &opt,
                                                             sizeof(opt))) &&
                          opt.ConstantBufferOffsetting;
    }

    // The deferral is asked before anything is built; a deferred frame arms the white placeholder on its way out.
    if (shader_async_hold()) { white_ensure(dev); return SHADERS_COMPILING; }

    if (!g_reset_hook_installed && RVExtBridge::has_reset_hook()) {
        RVExtBridge::set_reset_hook(&on_engine_reset);
        g_reset_hook_installed = true;
    }

    HRESULT hr;
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* vs_fs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;

    const std::string static_src = hlsl_src(HLSL_CB) + hlsl_src(HLSL_STATIC).c_str();
    // The shared receive block (band / atlas textures + compare functions) arms for this unit; the effect unit
    // compiles without it (its depthTex owns t1).
    static const D3D_SHADER_MACRO rx_defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr },
    };
    // Shared by every KH_TEXTURED compile below (the textured twins, the alpha casters, the instancing twins) and the
    // batch.
    static const D3D_SHADER_MACRO defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { "KH_TEXTURED", "1" }, { nullptr, nullptr },
    };
    const std::string sc1 = "1";
    // Every define table in this speculation list must match its ensure_* twin byte for byte: the memo key is an
    // FNV over source, entry, target and the defines in order - one reordered or added entry makes the speculation
    // miss silently.
    const std::string comp_src =
        hlsl_src(HLSL_CB) + hlsl_src(HLSL_COMPOSITE).c_str();
    const D3D_SHADER_MACRO sp_d0[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", sc1.c_str() },
        { "KH_ARB_DEPTH", "0" }, { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO d0a[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", sc1.c_str() },
        { "KH_ARB_DEPTH", "1" }, { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO d0t[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", sc1.c_str() },
        { "KH_RECEIVE_TEX", "1" }, { "KH_TEXTURED", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO d0ta[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", sc1.c_str() },
        { "KH_ARB_DEPTH", "1" }, { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" }, { nullptr, nullptr } };
    // Must match ensure_effect_shader byte for byte: MSAA_DEPTH, then KH_FX_UNIT (cb.hlsl's effect section; no
    // SAMPLE_COUNT, no KH_RECEIVE_TEX - the effect unit never receives).
    const std::string fx_src =
        hlsl_src(HLSL_CB) + hlsl_src(HLSL_EFFECT).c_str();
    const D3D_SHADER_MACRO fx_d0[] = {
        { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    // The rest of the core set rides this batch so the first post FX, depth snapshot, sun prefilter and mirror
    // prepass are memo hits. Twins: ensure_effect_shader (either MSAA_DEPTH value, the main depth's) and
    // pip_fx_shader (MSAA_DEPTH 0, the PIP's), ensure_depth_resolve_shader and ensure_ssao_shaders (every count:
    // PREWARM_ALL below), vmsee_ensure (PSDepthS0, the multisampled counts alone - VM_SEE), ensure_sun_pf
    // (empty table), vmir_ensure_gpu (null) - the hash reads those two alike.
    const D3D_SHADER_MACRO d1[] = {
        { "MSAA_DEPTH", "1" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO l0[] = {   // KH_FX_LEAN: ensure_effect_shader's lean builds.
        { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { "KH_FX_LEAN", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO l1[] = {
        { "MSAA_DEPTH", "1" }, { "KH_FX_UNIT", "1" }, { "KH_FX_LEAN", "1" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO none[] = { { nullptr, nullptr } };
    // VSWhite / PSWhite stay out of this list: they are the placeholder drawn WHILE it compiles, so white_ensure
    // builds them at the arming, on the arming thread beside the pool - queued, compile_shader would only make that
    // thread wait for a worker to reach them.
    const ShaderJob jobs[] = {
        { static_src.c_str(), "PSMain",      "ps_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSMain",      "ps_5_0", defines,    0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", sp_d0,   0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", d0a,  0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", d0t,  0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", d0ta, 0 },
        { static_src.c_str(), "PSMaskCast",  "ps_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSDlsWorld",  "ps_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSDlsWorldFog", "ps_5_0", rx_defines, 0 },   // DLSW_FOG.
        { static_src.c_str(), "VSDlsMask",   "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSDlsMask",   "ps_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSDlsMaskA",  "ps_5_0", defines,    0 },   // DLSW_MASK_ALPHA.
        { static_src.c_str(), "PSMaskPrime", "ps_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSInjDepthA", "ps_5_0", defines,    0 },
        { static_src.c_str(), "VSMain",      "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSMain",      "vs_5_0", defines,    0 },
        { static_src.c_str(), "VSMainInst",  "vs_5_0", rx_defines, 0 },   // INSTANCING twins.
        { static_src.c_str(), "VSMainInst",  "vs_5_0", defines,    0 },
        { static_src.c_str(), "VSMainInstSk",  "vs_5_0", rx_defines, 0 },   // SKIN_POOL twins.
        { static_src.c_str(), "VSMainInstSk",  "vs_5_0", defines,    0 },
        { static_src.c_str(), "VSSunDepthSk",  "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSSeamInstSk",  "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSMirrorInstSk", "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSMirror",    "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSSeamInst",   "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "PSSeamFoot",   "ps_5_0", rx_defines, 0 },   // VOL_FOOT.
        { static_src.c_str(), "PSReplayMerge", "ps_5_0", rx_defines, 0 },   // VOL_REPLAY (ps_optional's twin).
        { static_src.c_str(), "PSReplayMergeMir", "ps_5_0", rx_defines, 0 },   // MIR_REPLAY (likewise).
        { static_src.c_str(), "VSMirrorInst", "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSFullscreen","vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSSunDepth",  "vs_5_0", rx_defines, 0 },
        { static_src.c_str(), "VSSunDepthA", "vs_5_0", defines,    0 },   // CAST_ALPHA twins.
        { static_src.c_str(), "VSSunDepthAObj", "vs_5_0", defines, 0 },   // CAST_ALPHA: the fallback's.
        { static_src.c_str(), "PSSunDepthA", "ps_5_0", defines,    0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", sp_d0,  0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", d0t, 0 },
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", sp_d0,  0 },   // INSTANCING twins.
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", d0t, 0 },
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", sp_d0,  0 },   // SKIN_POOL twins.
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", d0t, 0 },
        { fx_src.c_str(), "PSEffect", "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSEffect", "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSEffect", "ps_5_0", l0, 0 },   // FX_LEAN_PASS.
        { fx_src.c_str(), "PSEffect", "ps_5_0", l1, 0 },
        // FX_SPLIT / FX_LINZ / FS_SPLIT: ensure_effect_shader's other six, at either MSAA_DEPTH (the main
        // depth's).
        { fx_src.c_str(), "PSFxLinZ",     "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSFxLinZ",     "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSSsgiGather", "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSSsgiGather", "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSSsgiAtrous", "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSSsgiAtrous", "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSDlfGather",  "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSDlfGather",  "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSDlfFilter",  "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSDlfFilter",  "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSFsSplitS",   "ps_5_0", fx_d0, 0 },   // FS_SPLIT.
        { fx_src.c_str(), "PSFsSplitS",   "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSFsSide",     "ps_5_0", fx_d0, 0 },   // WSEE_EDGE_COST.
        { fx_src.c_str(), "PSFsSide",     "ps_5_0", d1, 0 },
        { fx_src.c_str(), "PSFsSplitS",   "ps_5_0", l0, 0 },   // KH_FX_LEAN.
        { fx_src.c_str(), "PSFsSplitS",   "ps_5_0", l1, 0 },
        { fx_src.c_str(), "PSDlfComp",    "ps_5_0", l0, 0 },
        { fx_src.c_str(), "PSDlfComp",    "ps_5_0", l1, 0 },
        { fx_src.c_str(), "PSDlfGather",  "ps_5_0", l0, 0 },   // DLF_SPLIT.
        { fx_src.c_str(), "PSDlfGather",  "ps_5_0", l1, 0 },
        { fx_src.c_str(), "PSGlowSeed", "ps_5_0", fx_d0, 0 },   // GLOW_PYR (the creation below).
        { fx_src.c_str(), "PSGlowDown", "ps_5_0", fx_d0, 0 },
        { fx_src.c_str(), "PSAnaSeed", "ps_5_0", fx_d0, 0 },   // ANA_PYR (the creation below).
        { fx_src.c_str(), "PSAnaDown", "ps_5_0", fx_d0, 0 },
        { hlsl_src(HLSL_SUNPF).c_str(),   "VSPf",    "vs_5_0", none, 0 },
        { hlsl_src(HLSL_SUNPF).c_str(),   "PSPf",    "ps_5_0", none, 0 },
        { hlsl_src(HLSL_SUNPF).c_str(),   "PSPfMip", "ps_5_0", none, 0 },
        { hlsl_src(HLSL_VMIR_CS).c_str(), "VSMirB2", "vs_5_0", nullptr,   0 },   // MIR_DRAW.
        { hlsl_src(HLSL_VMIR_CS).c_str(), "PSMirB2", "ps_5_0", nullptr,   0 },
    };
    // PREWARM_ALL: every other built-in unit (WSEE's depth_resolve entries too), so no ensure compiles one
    // cold once the batch has run: a memo hit, or a disk hit after a later batch's finish released the memo. The
    // depth-resolve and SSAO units key on the main depth's sample count (their ensures' tables, MSAA_DEPTH +
    // SAMPLE_COUNT, an unknown count read as 1), usually not adopted yet at this first ensure (it follows the video
    // settings): each count Arma's FSAA uses (1, 2, 4, 8) is queued, plus an adopted count outside those. A count
    // never queued still compiles at its ensure (from the disk cache once seen). Then VSSkinSo (skin_so_ensure's
    // empty table, and skin_so_big_ensure's SKIN_SO_DEFS_L) and the three inline-source shaders
    // (ensure_resources' two, WSEE's copy).
    std::vector<ShaderJob> all(jobs, jobs + _countof(jobs));
    // WSEE: the agreement mask, multisampled only; this and every see-through job below only with the system built
    // in (WSEE_ON; nothing would create them).
    if constexpr (WSEE_ON) all.push_back({ fx_src.c_str(), "PSWseeAgree", "ps_5_0", d1, 0 });
    if constexpr (WSEE_ON) {   // DLF_SPLIT: the gather's texel marker (its lean build is in the table above).
        all.push_back({ fx_src.c_str(), "PSDlfClass", "ps_5_0", fx_d0, 0 });
        all.push_back({ fx_src.c_str(), "PSDlfClass", "ps_5_0", d1, 0 });
    }
    static const D3D_SHADER_MACRO dr1[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", "1" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO dr2[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "2" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO dr4[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "4" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO dr8[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "8" }, { nullptr, nullptr } };
    const UINT live = g_res.depth_sample_count;
    const std::string sc = std::to_string(live);
    const D3D_SHADER_MACRO drx[] = {
        { "MSAA_DEPTH", live > 1 ? "1" : "0" }, { "SAMPLE_COUNT", sc.c_str() }, { nullptr, nullptr } };
    const bool other = live > 0 && live != 1 && live != 2 && live != 4 && live != 8;
    const D3D_SHADER_MACRO* const dr[5] = { dr1, dr2, dr4, dr8,
                                            other ? drx : nullptr };
    static const char* const sa_ent[5] = {   // SSAO (ensure_ssao_shaders's five; NEARZ_MARK).
        "PSSsaoDepth", "PSSsaoMain", "PSSsaoBlur", "PSSsaoApply", "PSSsaoNearMark" };
    for (const D3D_SHADER_MACRO* d : dr) {
        if (!d) continue;
        all.push_back({ hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSDepthResolve", "ps_5_0", d, 0 });
        if (d != dr1) {   // VM_SEE: multisampled only (vmsee_ensure).
            all.push_back({ hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSDepthS0", "ps_5_0", d, 0 });
            // WSEE: wsee_ensure's late-opaque PS and the layer-depth split's MRT twin, at the same tables
            // (PREWARM_ALL: wsee_ensure compiles both at the flush).
            if constexpr (WSEE_ON) {
                const char* const pw_src = hlsl_src(HLSL_DEPTH_RESOLVE).c_str();
                all.push_back({ pw_src, "PSWseeOne", "ps_5_0", d, 0 });
                all.push_back({ pw_src, "PSWseeOneL", "ps_5_0", d, 0 });
                // WSEE_OQ2: wsee_ensure's D3m twins and D3's resolve, at the same tables.
                all.push_back({ pw_src, "PSWseeOneZ", "ps_5_0", d, 0 });
                all.push_back({ pw_src, "PSWseeOneZL", "ps_5_0", d, 0 });
                all.push_back({ pw_src, "PSWseeD3Res", "ps_5_0", d, 0 });
            }
        }
        for (const char* e : sa_ent) {
            all.push_back({ hlsl_src(HLSL_SSAO).c_str(), e, "ps_5_0", d, 0 });
        }
    }
    if constexpr (WSEE_ON) {   // PSB_ZB: wsee_ensure's block bound, at PSB_LS alone.
        all.push_back({ hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSPsbZb", "ps_5_0", PSB_ZB_DEFS, 0 });
    }
    if (SKIN_GPU_ON) all.push_back({ hlsl_src(HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", none, 0 });
    if (SKIN_GPU_ON) {   // SKIN_SO_BIG: the large class (SKIN_SO_LAZY: skin_so_big_ensure, after it).
        all.push_back({ hlsl_src(HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", SKIN_SO_DEFS_L, 0 });
    }
    all.push_back({ PS_ALPHA_ZERO_SRC, "main", "ps_5_0", nullptr, 0 });
    all.push_back({ PS_PREMULT_SRC,    "main", "ps_5_0", nullptr, 0 });
    if constexpr (WSEE_ON) {   // WSEE (wsee_ensure): with the system built in alone.
        all.push_back({ PS_WSEE_COPY_SRC,  "main", "ps_5_0", nullptr, 0 });
    }
    const size_t n = all.size();
    if (shader_async_gate(all.data(), n)) {
        white_ensure(dev);   // The arming frame gets a placeholder too.
        return SHADERS_COMPILING;
    }
    std::string err = compile_shader(static_src.c_str(), "VSMain", "vs_5_0", rx_defines, &vs_blob);
    if (!err.empty()) return err;
    if (!g_vmir.vs) {   // VSMirror - non-fatal (see the declaration).
        // Guarded so an ensure retry cannot double-create.
        ID3DBlob* blob = nullptr;
        const std::string mv_err = compile_shader(static_src.c_str(), "VSMirror",
                                                  "vs_5_0", rx_defines, &blob);
        if (mv_err.empty() && blob) {
            if (FAILED(dev->CreateVertexShader(blob->GetBufferPointer(),
                                               blob->GetBufferSize(),
                                               nullptr, &g_vmir.vs))) {
                g_vmir.vs = nullptr;
                report_error_once_safe("KH RenderIntegration: mirror: VSMirror could not be created (the mirror "
                                       "stands down)");
            }
            blob->Release();
        } else {
            report_error_once_safe("KH RenderIntegration: mirror: VSMirror compile failed (the mirror stands down): " +
                                   mv_err);
        }
    }
    err = compile_shader(static_src.c_str(), "VSFullscreen", "vs_5_0", rx_defines, &vs_fs_blob);
    if (!err.empty()) { vs_blob->Release(); return err; }
    err = compile_shader(static_src.c_str(), "PSMain", "ps_5_0", rx_defines, &ps_blob);
    if (!err.empty()) { vs_blob->Release(); vs_fs_blob->Release(); return err; }
    // Reflect the blob (compiled or served from .khsc) and compare its declared cbuffer sizes to the C++ split.
    // Non-fatal.
    {
        ID3D11ShaderReflection* r = nullptr;
        if (ps_blob && SUCCEEDED(D3DReflect(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
                                            __uuidof(ID3D11ShaderReflection),
                                            reinterpret_cast<void**>(&r))) && r) {
            auto size = [&](const char* name) -> uint32_t {
                ID3D11ShaderReflectionConstantBuffer* cb = r->GetConstantBufferByName(name);
                D3D11_SHADER_BUFFER_DESC d = {};
                return (cb && SUCCEEDED(cb->GetDesc(&d))) ? d.Size : 0u;
            };
            const uint32_t obj_b = size("CBObj");
            const uint32_t frame_b = size("CBFrame");
            r->Release();
            if (obj_b != static_cast<uint32_t>(CBOBJ_BYTES) ||
                frame_b != static_cast<uint32_t>(CBFRAME_BYTES)) {
                report_error_once_safe("KH RenderIntegration: CB mirror: HLSL CBObj/CBFrame " +
                                       std::to_string(obj_b) + "/" + std::to_string(frame_b) + " B vs C++ " +
                                       std::to_string(CBOBJ_BYTES) + "/" + std::to_string(CBFRAME_BYTES) + " B");
            }
        }
    }

    // Non-fatal entry points: each consumer gates on its pointer.
    ps_optional(dev, static_src, "PSInjDepthA", defines, &g_res.ps_inj_depth_a,
                "inject-depth alpha shader");   // FOOTPRINT_ALPHA (KH_TEXTURED for KhMatRouteG).
    ps_optional(dev, static_src, "PSMaskCast", rx_defines, &g_res.ps_maskcast,
                "maskcast shader");   // Analytic mask cast.
    ps_optional(dev, static_src, "PSReplayMerge", rx_defines, &g_res.ps_rpmerge,
                "replay merge shader");   // VOL_REPLAY.
    ps_optional(dev, static_src, "PSReplayMergeMir", rx_defines, &g_res.ps_rpmergemir,
                "mirror merge shader");   // MIR_REPLAY.
    ps_optional(dev, static_src, "PSDlsWorld", rx_defines, &g_res.ps_dls_world,
                "dlsworld shader");   // DLS_WORLD receive.
    ps_optional(dev, static_src, "PSSeamFoot", rx_defines, &g_res.ps_seam_foot,
                "seam footprint shader");   // VOL_FOOT; absent = the depth-plane witness.
    ps_optional(dev, static_src, "PSDlsWorldFog", rx_defines, &g_res.ps_dls_world_fog,
                "dlsworld fog shader");   // DLSW_FOG; absent = the plain multiply.
    vs_optional(dev, static_src, "VSDlsMask", rx_defines, &g_res.vs_dls_mask,
                "dlsmask VS");   // DLSW_MASK pair: a failed
    ps_optional(dev, static_src, "PSDlsMask", rx_defines, &g_res.ps_dls_mask,
                "dlsmask PS");   // compile leaves the mask off.
    ps_optional(dev, static_src, "PSDlsMaskA", defines, &g_res.ps_dls_mask_a,
                "dlsmask alpha PS");   // DLSW_MASK_ALPHA (KH_TEXTURED for KhMatRouteG); absent = alpha casters
                                          // mask opaque.
    ps_optional(dev, static_src, "PSMaskPrime", rx_defines, &g_res.ps_maskprime,
                "maskprime shader");   // Priming pass, same contract.
    // GLOW_PYR: the effect unit's two pyramid builders (MSAA_DEPTH 0 - neither reads depth); absent = direct taps.
    ps_optional(dev, fx_src, "PSGlowSeed", fx_d0, &g_res.ps_glow_seed, "glow seed shader");
    ps_optional(dev, fx_src, "PSGlowDown", fx_d0, &g_res.ps_glow_down, "glow down shader");
    // ANA_PYR: anamorphic's two, likewise (MSAA_DEPTH 0); absent = its direct taps.
    ps_optional(dev, fx_src, "PSAnaSeed", fx_d0, &g_res.ps_ana_seed, "anamorphic seed shader");
    ps_optional(dev, fx_src, "PSAnaDown", fx_d0, &g_res.ps_ana_down, "anamorphic down shader");

    {   // Instanced sun-depth VS + layout: non-fatal (per-caster loop covers).
        ID3DBlob* sd_blob = nullptr;
        const std::string sd_err = compile_shader(static_src.c_str(), "VSSunDepth", "vs_5_0", rx_defines, &sd_blob);

        if (sd_err.empty() && sd_blob) {
            if (SUCCEEDED(dev->CreateVertexShader(sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), nullptr, &g_res.vs_sundepth))) {
                D3D11_INPUT_ELEMENT_DESC sl[] = {   // OBJBUF: mesh slot 0 + the lane on slot 1.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // InstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };

                if (FAILED(dev->CreateInputLayout(sl, 4, sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), &g_res.layout_sundepth))) {
                    KH_SAFE_RELEASE(g_res.vs_sundepth);
                }
            }

            sd_blob->Release();
        } else if (!sd_err.empty()) {
            report_error_once_safe("KH RenderIntegration: sun shadow: sun-depth shader: " + sd_err);
        }
    }

    // The seam transport's instanced twins. Both consume the sun ladder's lane shape (VSInSeam mirrors VSInSun), so
    // they bind g_res.layout_sundepth. Non-fatal: either one absent leaves that loop on its per-caster path.
    {
        ID3D11VertexShader** const si_out[2] = { &g_res.vs_seam_inst, &g_res.vs_mirror_inst };
        const char* const si_ent[2] = { "VSSeamInst", "VSMirrorInst" };

        for (int si_i = 0; si_i < 2; ++si_i) {
            ID3DBlob* blob = nullptr;
            const std::string si_err = compile_shader(static_src.c_str(), si_ent[si_i],
                                                      "vs_5_0", rx_defines, &blob);

            if (si_err.empty() && blob) {
                if (FAILED(dev->CreateVertexShader(blob->GetBufferPointer(),
                                                   blob->GetBufferSize(),
                                                   nullptr, si_out[si_i]))) {
                    *si_out[si_i] = nullptr;
                }
            } else if (!si_err.empty()) {
                report_error_once_safe(std::string("KH RenderIntegration: seam: instanced VS ") + si_ent[si_i] +
                                       " - that loop stays per caster: " + si_err);
            }

            if (blob) blob->Release();
        }
    }

    // The alpha-aware sun-depth VS/PS + their layout (mesh slot 0 with the uv, the same per-instance slot 1).
    // Non-fatal: absent, every caster draws whole. Compiled KH_TEXTURED for KhMatRouteG.
    {
        ID3DBlob* vb = nullptr;
        ID3DBlob* pb = nullptr;
        std::string sa_err = compile_shader(static_src.c_str(), "VSSunDepthA", "vs_5_0", defines, &vb);
        if (sa_err.empty()) sa_err = compile_shader(static_src.c_str(), "PSSunDepthA", "ps_5_0", defines, &pb);

        if (sa_err.empty() && vb && pb) {
            HRESULT sa_hr = dev->CreateVertexShader(vb->GetBufferPointer(), vb->GetBufferSize(), nullptr, &g_res.vs_sundepth_a);
            if (SUCCEEDED(sa_hr)) sa_hr = dev->CreatePixelShader(pb->GetBufferPointer(), pb->GetBufferSize(), nullptr, &g_res.ps_sundepth_a);

            if (SUCCEEDED(sa_hr)) {
                const D3D11_INPUT_ELEMENT_DESC el[] = {   // OBJBUF: + uv on slot 0.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // InstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };
                sa_hr = dev->CreateInputLayout(el, 5, vb->GetBufferPointer(), vb->GetBufferSize(), &g_res.layout_sundepth_a);
            }

            if (FAILED(sa_hr)) {
                KH_SAFE_RELEASE(g_res.vs_sundepth_a);
                KH_SAFE_RELEASE(g_res.ps_sundepth_a);
                KH_SAFE_RELEASE(g_res.layout_sundepth_a);
                report_error_once_safe("KH RenderIntegration: sun shadow: sun-depth alpha shaders: create " +
                                       hr_str(sa_hr));
            }
        } else if (!sa_err.empty()) {
            report_error_once_safe("KH RenderIntegration: sun shadow: sun-depth alpha shaders: " + sa_err);
        }

        if (vb) vb->Release();
        if (pb) pb->Release();
    }
    // CAST_ALPHA: VSSunDepthA's per-object twin and its layout (the mesh's own four lanes, validated against
    // its blob) - the depth maps' per-object route (SUN_OBJ) draws its alpha casters with it and PSSunDepthA.
    // Non-fatal: absent, that route draws them whole.
    {
        ID3DBlob* vb = nullptr;
        const std::string so_err = compile_shader(static_src.c_str(), "VSSunDepthAObj", "vs_5_0", defines,
                                                  &vb);
        if (so_err.empty() && vb) {
            HRESULT so_hr = dev->CreateVertexShader(vb->GetBufferPointer(), vb->GetBufferSize(), nullptr,
                                                    &g_res.vs_sundepth_ao);
            if (SUCCEEDED(so_hr)) {
                const D3D11_INPUT_ELEMENT_DESC el[] = {   // layout_tex's lanes.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                so_hr = dev->CreateInputLayout(el, 4, vb->GetBufferPointer(), vb->GetBufferSize(),
                                               &g_res.layout_sundepth_ao);
            }
            if (FAILED(so_hr)) {
                KH_SAFE_RELEASE(g_res.vs_sundepth_ao);
                KH_SAFE_RELEASE(g_res.layout_sundepth_ao);
                report_error_once_safe("KH RenderIntegration: sun shadow: sun-depth alpha fallback shader: create " +
                                       hr_str(so_hr));
            }
        } else if (!so_err.empty()) {
            report_error_once_safe("KH RenderIntegration: sun shadow: sun-depth alpha fallback shader: " + so_err);
        }
        if (vb) vb->Release();
    }

    hr = dev->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_res.vs);
    if (FAILED(hr)) { vs_blob->Release(); vs_fs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader " + hr_str(hr); }
    hr = dev->CreateVertexShader(vs_fs_blob->GetBufferPointer(), vs_fs_blob->GetBufferSize(), nullptr, &g_res.vs_fullscreen);
    vs_fs_blob->Release();
    if (FAILED(hr)) { vs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader(fullscreen) " + hr_str(hr); }
    hr = dev->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_res.ps);
    if (SUCCEEDED(hr)) psv_own(0, ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), g_res.ps);   // PSV.
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
        const std::string mv_err = ensure_mesh_vbs(dev);
        if (!mv_err.empty()) { g_res.release(); return mv_err; }
    }

    // KH_TEXTURED static twins (non-fatal): the same source with KH_TEXTURED=1. The 4-element layout is validated
    // against the textured VS blob and serves both textured VS's (identical input signatures).
    {
        // defines: the function's top (the SHADER_MT batch names it too).
        ID3DBlob* vsb = nullptr;
        ID3DBlob* psb = nullptr;
        std::string tx_err = compile_shader(static_src.c_str(), "VSMain", "vs_5_0", defines, &vsb);
        if (tx_err.empty()) tx_err = compile_shader(static_src.c_str(), "PSMain", "ps_5_0", defines, &psb);

        if (tx_err.empty()) {
            HRESULT tx_hr = dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_res.vs_tex);
            if (SUCCEEDED(tx_hr)) tx_hr = dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_res.ps_tex);
            if (SUCCEEDED(tx_hr)) {   // PSV: the tail's builtin (D3).
                psv_own(1, psb->GetBufferPointer(), psb->GetBufferSize(), g_res.ps_tex);
            }

            if (SUCCEEDED(tx_hr)) {
                const D3D11_INPUT_ELEMENT_DESC il[] = {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                tx_hr = dev->CreateInputLayout(il, 4, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_res.layout_tex);
            }

            if (SUCCEEDED(tx_hr) && !g_res.mat_sampler) {
                D3D11_SAMPLER_DESC sd = {};
                sd.Filter = D3D11_FILTER_ANISOTROPIC;
                // ANISO_16: the hardware's full 16x for our materials (cheap on any D3D11 GPU) - textures on
                // surfaces seen at a grazing angle stay sharp twice as far out. Twin: cb.hlsl KhToksvigA2's model.
                sd.MaxAnisotropy = 16;
                sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
                sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
                sd.MaxLOD = D3D11_FLOAT32_MAX;
                tx_hr = dev->CreateSamplerState(&sd, &g_res.mat_sampler);
            }

            if (FAILED(tx_hr)) {
                KH_SAFE_RELEASE(g_res.vs_tex);
                KH_SAFE_RELEASE(g_res.ps_tex);
                KH_SAFE_RELEASE(g_res.layout_tex);
                report_error_once_safe("KH RenderIntegration: textured shaders: create " + hr_str(tx_hr));
            }
        } else {
            report_error_once_safe("KH RenderIntegration: textured shaders: " + tx_err);
        }

        if (vsb) vsb->Release();
        if (psb) psb->Release();
    }

    // Instancing twins (non-fatal): VSMainInst plain and KH_TEXTURED, each with its layout (the mesh slot's elements
    // plus VSInst on slot 1), also serving the composite twins (identical input signatures). Then the two stream
    // rings.
    {
        auto make = [&](const D3D_SHADER_MACRO* in_defs, bool tex,
                        ID3D11VertexShader** vs, ID3D11InputLayout** il) -> bool {
            ID3DBlob* b = nullptr;
            const std::string in_err = compile_shader(static_src.c_str(), "VSMainInst", "vs_5_0", in_defs, &b);

            if (!in_err.empty() || !b) {
                if (!in_err.empty()) report_error_once_safe("KH RenderIntegration: instanced shaders: " + in_err);
                return false;
            }

            HRESULT in_hr = dev->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, vs);

            if (SUCCEEDED(in_hr)) {
                const D3D11_INPUT_ELEMENT_DESC el[] = {   // OBJBUF: the lane on slot 1.
                    { "POSITION", 0,  DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0,  DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 0,  DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TANGENT",  0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // InstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };
                // position / normal then the 2 instance elements (the textured pair is skipped, not renumbered).
                D3D11_INPUT_ELEMENT_DESC plain[4];
                plain[0] = el[0];
                plain[1] = el[1];
                for (int k = 0; k < 2; ++k) plain[2 + k] = el[4 + k];
                in_hr = dev->CreateInputLayout(tex ? el : plain, tex ? 6u : 4u,
                                               b->GetBufferPointer(), b->GetBufferSize(), il);
            }

            b->Release();

            if (FAILED(in_hr)) {
                KH_SAFE_RELEASE(*vs);
                KH_SAFE_RELEASE(*il);
                report_error_once_safe("KH RenderIntegration: instanced shaders: create " + hr_str(in_hr));
                return false;
            }

            return true;
        };

        bool ok = make(rx_defines, false, &g_res.vs_inst, &g_res.layout_inst);
        if (ok) ok = make(defines, true, &g_res.vs_inst_tex, &g_res.layout_inst_tex);

        if (ok) {
            D3D11_BUFFER_DESC in_bd = {};
            in_bd.ByteWidth = INST_STRIDE * INST_CAP;
            in_bd.Usage = D3D11_USAGE_DYNAMIC;
            in_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            in_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            for (int in_i = 0; in_i < 2 && ok; ++in_i) {
                g_res.inst_cursor[in_i] = 0;
                g_res.inst_fresh[in_i] = true;
                const HRESULT in_hr = dev->CreateBuffer(&in_bd, nullptr, &g_res.inst_vb[in_i]);

                if (FAILED(in_hr)) {
                    report_error_once_safe("KH RenderIntegration: instanced stream: create " + hr_str(in_hr));
                    ok = false;
                }
            }
        }

        if (!ok) {
            KH_SAFE_RELEASE(g_res.vs_inst);
            KH_SAFE_RELEASE(g_res.vs_inst_tex);
            KH_SAFE_RELEASE(g_res.layout_inst);
            KH_SAFE_RELEASE(g_res.layout_inst_tex);
            for (int in_i = 0; in_i < 2; ++in_i) KH_SAFE_RELEASE(g_res.inst_vb[in_i]);
        }
    }

    // SKIN_POOL - the static unit's vertex-pulling twins (non-fatal): the colour buckets' VSMainInstSk pair and
    // the depth runs' three, with the one layout they share (identical input signatures: the lane on slot 1, the
    // vertex id; nothing on slot 0), made against the first that compiles. A missing colour twin keeps skeletal
    // bindings out of the buckets (inst_sk_on); a missing depth twin leaves that route's skeletal casters in runs
    // of one (cast_pool_of).
    {
        struct SkTwin { const char* ent; const D3D_SHADER_MACRO* defs; ID3D11VertexShader** out; };
        const SkTwin tw[] = {
            { "VSMainInstSk",   rx_defines, &g_res.vs_inst_sk },
            { "VSMainInstSk",   defines,    &g_res.vs_inst_sk_tex },
            { "VSSunDepthSk",   rx_defines, &g_res.vs_sundepth_sk },
            { "VSSeamInstSk",   rx_defines, &g_res.vs_seam_inst_sk },
            { "VSMirrorInstSk", rx_defines, &g_res.vs_mirror_inst_sk },
        };
        for (const SkTwin& t : tw) {
            ID3DBlob* b = nullptr;
            const std::string sk_err = compile_shader(static_src.c_str(), t.ent, "vs_5_0", t.defs, &b);
            if (sk_err.empty() && b) {
                if (FAILED(dev->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                                   t.out))) {
                    *t.out = nullptr;
                }
                if (*t.out && !g_res.layout_inst_sk) {
                    const D3D11_INPUT_ELEMENT_DESC el[] = {   // InstLane on slot 1 (VSInstSk).
                        { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,        1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                        { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT, 1, 4, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                    };
                    if (FAILED(dev->CreateInputLayout(el, 2, b->GetBufferPointer(), b->GetBufferSize(),
                                                      &g_res.layout_inst_sk))) {
                        g_res.layout_inst_sk = nullptr;
                    }
                }
            } else if (!sk_err.empty()) {
                report_error_once_safe(std::string("KH RenderIntegration: skin pool: twin ") + t.ent +
                                       " - that route draws skeletal meshes one by one: " + sk_err);
            }
            if (b) b->Release();
        }
    }

    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(CBOBJ_BYTES);   // b0 carries the object block only.
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.constant_buffer);
        if (FAILED(hr)) { g_res.release(); return "create CB " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.composite_cb);
        if (FAILED(hr)) { g_res.release(); return "create composite CB " + hr_str(hr); }
        bd.ByteWidth = static_cast<UINT>(CBFRAME_BYTES);
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.frame_cb);
        if (FAILED(hr)) { g_res.release(); return "create frame CB " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.composite_frame_cb);
        if (FAILED(hr)) { g_res.release(); return "create composite frame CB " + hr_str(hr); }
        // CB_RING: non-fatal.
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS opt = {};
            const bool have = SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &opt, sizeof(opt)));
            g_cb_offsetting = have && opt.ConstantBufferOffsetting;
            g_cbr_supported = have &&
                opt.ConstantBufferOffsetting && opt.MapNoOverwriteOnDynamicConstantBuffer;

            if (g_cbr_supported) {
                D3D11_BUFFER_DESC cr_bd = {};
                cr_bd.ByteWidth = CBR_STRIDE * CBR_SLICES;
                cr_bd.Usage = D3D11_USAGE_DYNAMIC;
                cr_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                cr_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                for (int cr_i = 0; cr_i < 2; ++cr_i) {
                    g_res.cb_ring_cursor[cr_i] = 0;
                    g_res.cb_ring_fresh[cr_i] = true;
                    const HRESULT cr_hr = dev->CreateBuffer(&cr_bd, nullptr, &g_res.cb_ring[cr_i]);

                    if (FAILED(cr_hr)) {
                        for (int j = 0; j < 2; ++j) KH_SAFE_RELEASE(g_res.cb_ring[j]);
                        g_cbr_supported = false;
                        report_error_once_safe("KH RenderIntegration: cb ring: create " + hr_str(cr_hr) +
                                               " (per-object uploads)");
                        break;
                    }
                }
            }
        }
    }

    {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = TRUE;
        dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dd.StencilEnable = FALSE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_test);
        if (FAILED(hr)) { g_res.release(); return "create DSS(test) " + hr_str(hr); }
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_test_write);
        if (FAILED(hr)) { g_res.release(); return "create DSS(write) " + hr_str(hr); }
        // SSAO_MARK: the marking twin (LESS_EQUAL + write, stencil ALWAYS / REPLACE on pass, KEEP otherwise) and
        // the eraser (depth off, stencil EQUAL / ZERO on pass). Both faces alike; the reference is the bind's.
        dd.StencilEnable = TRUE;
        dd.StencilReadMask = 0xFF;
        dd.StencilWriteMask = 0xFF;
        dd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        dd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        dd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dd.BackFace = dd.FrontFace;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_test_write_mark);
        if (FAILED(hr)) { g_res.release(); return "create DSS(write+mark) " + hr_str(hr); }
        dd.DepthEnable = FALSE;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dd.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
        dd.BackFace = dd.FrontFace;
        // SSAO_SEE_THROUGH: the eraser compares bit 0 alone (ref 1), so it clears a see-through draw's mark (3)
        // with the solids' (1); the apply's mask below compares the whole byte again and takes 1 alone.
        dd.StencilReadMask = 0x01;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_mark_clear);
        if (FAILED(hr)) { g_res.release(); return "create DSS(mark clear) " + hr_str(hr); }
        dd.StencilReadMask = 0xFF;
        // The apply's mask (non-fatal). The write mask is restored for dss_off's desc.
        dd.StencilWriteMask = 0x00;
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dd.BackFace = dd.FrontFace;
        if (FAILED(dev->CreateDepthStencilState(&dd, &g_res.dss_mark_test))) g_res.dss_mark_test = nullptr;
        dd.StencilWriteMask = 0xFF;
        dd.StencilEnable = FALSE;
        dd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        dd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        dd.BackFace = dd.FrontFace;
        hr = dev->CreateDepthStencilState(&dd, &g_res.dss_off);
        if (FAILED(hr)) { g_res.release(); return "create DSS(off) " + hr_str(hr); }
    }

    {
        // One blend state per mixing mode. MIN/MAX ignore the blend factors.
        struct BlendSpec { D3D11_BLEND src, dst; D3D11_BLEND_OP op; };

        const BlendSpec specs[6] = {
            { D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD },
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_ADD },
            { D3D11_BLEND_ZERO,      D3D11_BLEND_SRC_COLOR,     D3D11_BLEND_OP_ADD },
            { D3D11_BLEND_ONE,       D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_OP_ADD },
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_MAX },
            { D3D11_BLEND_ONE,       D3D11_BLEND_ONE,           D3D11_BLEND_OP_MIN },
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
            if (FAILED(hr)) { g_res.release(); return "create blend " + hr_str(hr); }
        }

        {   // DLS_WORLD: dest.rgb *= src.rgb, RGB write mask only.
            D3D11_BLEND_DESC wb = {};
            wb.RenderTarget[0].BlendEnable = TRUE;
            wb.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
            wb.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
            wb.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            wb.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            wb.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            wb.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            wb.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (FAILED(dev->CreateBlendState(&wb, &g_res.dls_world_blend))) {
                g_res.dls_world_blend = nullptr;
            }
        }

        {   // DLSW_FOG: dest.rgb = src0.rgb + dest.rgb * src1.rgb (dual
            // source), RGB write mask only; absent = the plain multiply.
            D3D11_BLEND_DESC wf = {};
            wf.RenderTarget[0].BlendEnable = TRUE;
            wf.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            wf.RenderTarget[0].DestBlend = D3D11_BLEND_SRC1_COLOR;
            wf.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            wf.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            wf.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            wf.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            wf.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (FAILED(dev->CreateBlendState(&wf, &g_res.dls_world_fog_blend))) {
                g_res.dls_world_fog_blend = nullptr;
            }
        }

        {   // DLSW_MASK: dest = min(src, dest). The minimum of a set of distances is the nearest one, so this
            // blend is the mask's depth test and no depth-stencil target is needed.
            D3D11_BLEND_DESC mb = {};
            mb.RenderTarget[0].BlendEnable = TRUE;
            mb.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            mb.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            mb.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
            mb.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            mb.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            mb.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
            mb.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
            if (FAILED(dev->CreateBlendState(&mb, &g_res.dlsw_min_blend))) {
                g_res.dlsw_min_blend = nullptr;
            }
        }

        // Builtin UI-phase state: blend off, RGB-only write - the shader composites and coverage-masks in-shader and
        // returns the finished frame. The mask channel survives for chaining.
        {
            D3D11_BLEND_DESC brp = {};
            brp.RenderTarget[0].BlendEnable = FALSE;
            brp.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            hr = dev->CreateBlendState(&brp, &g_res.blend_ui_replace);
            if (FAILED(hr)) { g_res.release(); return "create blend(uiReplace) " + hr_str(hr); }

            // Custom user FX keep the hardware DEST_ALPHA lerp (their shaders
            // cannot be assumed coverage-aware).
            D3D11_BLEND_DESC bd = {};
            bd.RenderTarget[0].BlendEnable = TRUE;
            bd.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
            bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
            bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bd.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            hr = dev->CreateBlendState(&bd, &g_res.blend_ui_masked);
            if (FAILED(hr)) { g_res.release(); return "create blend(uiMask) " + hr_str(hr); }

            // The coverage reset: blend off, alpha-only write mask - the
            // injected post-compose clear touches nothing but dst.a.
            D3D11_BLEND_DESC bz = {};
            bz.RenderTarget[0].BlendEnable = FALSE;
            bz.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
            hr = dev->CreateBlendState(&bz, &g_res.blend_alpha_zero);
            if (FAILED(hr)) { g_res.release(); return "create blend(alphaZero) " + hr_str(hr); }

            ID3DBlob* z_blob = nullptr;
            const std::string z_err = compile_shader(PS_ALPHA_ZERO_SRC,   // PREWARM_ALL.
                "main", "ps_5_0", nullptr, &z_blob);
            if (!z_err.empty()) { g_res.release(); return "compile PS(alphaZero): " + z_err; }
            hr = dev->CreatePixelShader(z_blob->GetBufferPointer(), z_blob->GetBufferSize(),
                                        nullptr, &g_res.ps_alpha_zero);
            z_blob->Release();
            if (FAILED(hr)) { g_res.release(); return "create PS(alphaZero) " + hr_str(hr); }

            ID3DBlob* pm_blob = nullptr;
            const std::string pm_err = compile_shader(PS_PREMULT_SRC,   // PREWARM_ALL.
                "main", "ps_5_0", nullptr, &pm_blob);
            if (!pm_err.empty()) { g_res.release(); return "compile PS(premult): " + pm_err; }
            hr = dev->CreatePixelShader(pm_blob->GetBufferPointer(), pm_blob->GetBufferSize(),
                                        nullptr, &g_res.ps_premult_copy);
            pm_blob->Release();
            if (FAILED(hr)) { g_res.release(); return "create PS(premult) " + hr_str(hr); }

            D3D11_BLEND_DESC bsp = {};
            bsp.RenderTarget[0].BlendEnable = TRUE;
            bsp.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            bsp.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
            bsp.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bsp.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            bsp.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            bsp.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            bsp.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            hr = dev->CreateBlendState(&bsp, &g_res.blend_ui_spill);
            if (FAILED(hr)) { g_res.release(); return "create blend(uiSpill) " + hr_str(hr); }
        }
    }

    {
        D3D11_RASTERIZER_DESC rd = {};
        rd.FillMode = D3D11_FILL_SOLID;
        rd.CullMode = D3D11_CULL_NONE;   // Visible from inside too.
        rd.MultisampleEnable = TRUE;   // Scene targets are MSAA per video settings.
        // No scene bias: the engine-depth tie duty lives in the shaders' own near / far contracts (a bias pair
        // crawls at grazing angles).
        rd.DepthBias = 0;
        rd.SlopeScaledDepthBias = 0.0f;
        rd.DepthBiasClamp = 0.0f;
        // The far plane's contract is a per-fragment PS test (the far contract block in PSMain / PSComposite /
        // PSEffect), so objects still pop at max view distance.
        rd.DepthClipEnable = FALSE;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer);
        if (FAILED(hr)) { g_res.release(); return "create rasterizer " + hr_str(hr); }
        // Back-face-culling twin for twoSided = false: only the cull mode differs. Meshes bake outward-front
        // winding, so D3D11_CULL_BACK drops exactly the interior-facing triangles.
        rd.CullMode = D3D11_CULL_BACK;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer_cull);
        if (FAILED(hr)) { g_res.release(); return "create cull rasterizer " + hr_str(hr); }
        rd.CullMode = D3D11_CULL_FRONT;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer_front);
        if (FAILED(hr)) { g_res.release(); return "create front rasterizer " + hr_str(hr); }
        // Sun-depth rasterizers. All stay CullNone regardless of twoSided: an open mesh must still cast the full
        // closed-surface shadow.
        rd.CullMode = D3D11_CULL_NONE;
        // SUN_RS_TEXT: the sun ladder binds no state minted here but the lo pair - sun_rs_pick prefers
        // rast_sun_lo (8 / 1.0; created below on this same path, fatal if it fails), the tiers its DepthClip-off
        // twin, the union the clip form. This base state (16 / 3.0) serves the prefilter's fullscreen draw,
        // dls_frame's placeholder bind (the DLS states supersede it), svs_prime_mask and the volume seam's
        // fallback for a null rast_inject. The ladder's compare bias is the metric one in the shader (sunMeta.z);
        // its hardware bias is the lo pair's.
        rd.DepthBias = 16;
        rd.SlopeScaledDepthBias = 3.0f;
        rd.DepthBiasClamp = 0.0f;   // No bias clamp on any sun state; the twins below differ in DepthClipEnable.
        rd.DepthClipEnable = TRUE;
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_sun);
        if (FAILED(hr)) { g_res.release(); return "create sun rasterizer " + hr_str(hr); }
        // Each sun state's clamp twin is minted from the same desc with DepthClipEnable false, so the pair cannot
        // drift. Non-fatal: a null twin falls through to clip at sun_rs_pick.
        auto twin = [&](ID3D11RasterizerState** pk_out, const char* what) {
            rd.DepthClipEnable = FALSE;
            if (FAILED(dev->CreateRasterizerState(&rd, pk_out))) {
                *pk_out = nullptr;
                report_error_once_safe(std::string("KH RenderIntegration: sun shadow: pancake rasterizer (") +
                                       what + ") could not be created (that tier state clips)");
            }
            rd.DepthClipEnable = TRUE;
        };
        twin(&g_res.rast_sun_pk, "base");
        rd.DepthBias = 8;
        rd.SlopeScaledDepthBias = 1.0f;
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_sun_lo);
        if (FAILED(hr)) { g_res.release(); return "create sun rasterizer(lo) " + hr_str(hr); }
        twin(&g_res.rast_sun_lo_pk, "lo");

        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthBias = 0;
        rd.SlopeScaledDepthBias = 0.0f;
        rd.DepthBiasClamp = 0.0f;

        rd.DepthClipEnable = FALSE;
        // TWIN (SEAM_FACE): this desc is rasterizer's, field for field; the volume seam binds rasterizer_cull
        // (rasterizer's D3D11_CULL_BACK twin) as this state's twin for a single-sided caster - change them together.
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_inject);
        if (FAILED(hr)) { g_res.release(); return "create inject rasterizer " + hr_str(hr); }
    }

    // The placeholder pair is ensured on the success path too, not only from the two deferral returns.
    white_ensure(dev);
    g_res.initialized = true;
    return "";
}
static int   g_ssgi_w = 0;   // Exact scaled dims (viewport + bounds truth).
static int   g_ssgi_h = 0;

// Creates (or recreates on a size, format or scale change) the five single-sample chain targets (the SSGI group
// among them at its own scale, the radiance seed). Called after ensure_scene_capture, which establishes dimensions.
inline std::string ensure_fx_chain(ID3D11Device* dev) {
    if (!g_res.scene_tex) return "no scene capture";

    D3D11_TEXTURE2D_DESC sd = {};
    g_res.scene_tex->GetDesc(&sd);

    const float g = g_ssgi_scale.load(std::memory_order_relaxed);   // XTHREAD_ATOMIC: read once.
    float s = !(g >= 0.25f) ? 0.25f : (g > 2.0f ? 2.0f : g);
    // SSGI_CAP: the group may not pass D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION on either side (its create failed
    // and the whole chain dropped, every frame). One factor capped for both sides keeps the group's aspect, so the
    // grid factor the chain derives from g_ssgi_w (scene_w / g_ssgi_w) holds on both axes.
    const float cap = static_cast<float>(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    if (static_cast<float>(sd.Width) * s > cap) s = cap / static_cast<float>(sd.Width);
    if (static_cast<float>(sd.Height) * s > cap) s = cap / static_cast<float>(sd.Height);
    UINT sw = (UINT)(sd.Width  * s + 0.5f) > 0 ? (UINT)(sd.Width  * s + 0.5f) : 1;
    UINT sh = (UINT)(sd.Height * s + 0.5f) > 0 ? (UINT)(sd.Height * s + 0.5f) : 1;
    const UINT sg_max = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    if (sw > sg_max) sw = sg_max;   // Float rounding at the cap.
    if (sh > sg_max) sh = sg_max;

    if (g_res.chain_tex[0] && g_res.chain_tex[2] && g_res.chain_tex[3] && g_res.chain_tex[4]) {
        D3D11_TEXTURE2D_DESC cd = {};
        D3D11_TEXTURE2D_DESC gd = {};
        g_res.chain_tex[0]->GetDesc(&cd);
        g_res.chain_tex[2]->GetDesc(&gd);   // Scale change recreates the group.
        if (cd.Width == sd.Width && cd.Height == sd.Height && cd.Format == sd.Format &&
            gd.Width == sw && gd.Height == sh) return "";
        g_res.release_fx_chain();
    }

    for (int i = 0; i < 5; ++i) {   // Gather; + pre-smooth; + radiance seed.
        D3D11_TEXTURE2D_DESC td = sd;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.SampleDesc.Count = 1;
        td.SampleDesc.Quality = 0;
        // [3] is [2]'s a-trous sibling: same half grid, same fp16. [4], the radiance seed the gather's taps read, is
        // one level (SSGI_SEED: the gather reads the base alone - SSGI_VB - so no pyramid is built).
        if (i >= 2) {
            td.Width  = sw;
            td.Height = sh;
            td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
        HRESULT hr = dev->CreateTexture2D(&td, nullptr, &g_res.chain_tex[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain tex " + hr_str(hr); }
        hr = dev->CreateRenderTargetView(g_res.chain_tex[i], nullptr, &g_res.chain_rtv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain RTV " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.chain_tex[i], nullptr, &g_res.chain_srv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain SRV " + hr_str(hr); }
    }

    g_ssgi_w = (int)sw;
    g_ssgi_h = (int)sh;

    if (!g_res.sampler) {   // The ssgi chain's bilinear sampler (seed, gather taps, resolve).
        D3D11_SAMPLER_DESC sm = {};
        sm.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.MaxLOD = D3D11_FLOAT32_MAX;
        HRESULT sm_hr = dev->CreateSamplerState(&sm, &g_res.sampler);
        if (FAILED(sm_hr)) { g_res.release_fx_chain(); return "create ssgi sampler " + hr_str(sm_hr); }
    }

    return "";
}

// Samples the engine depth buffer, whose MSAA count follows video settings: compiled per count.
inline std::string ensure_effect_shader(ID3D11Device* dev) {
    if (g_res.ps_effect && g_res.ps_effect_samples == g_res.depth_sample_count) return "";
    if (g_res.ps_effect) { g_res.ps_effect->Release(); g_res.ps_effect = nullptr; }
    KH_SAFE_RELEASE(g_res.ps_effect_lean);   // FX_LEAN_PASS: goes and comes with it.
    // FX_SPLIT / FX_LINZ: the unit's chain-side entry points go and come with it (a sample-count change), so
    // none outlives the MSAA_DEPTH it was compiled for. FS_SPLIT: fog scatter's likewise.
    ID3D11PixelShader** const ps[6] = { &g_res.ps_fx_linz, &g_res.ps_ssgi_gather, &g_res.ps_ssgi_atrous,
                                             &g_res.ps_dlf_gather, &g_res.ps_dlf_filter, &g_res.ps_fs_split };
    KH_SAFE_RELEASE(g_res.ps_wsee_agree);   // WSEE: likewise.
    KH_SAFE_RELEASE(g_res.ps_fs_side);   // WSEE_EDGE_COST: likewise.
    KH_SAFE_RELEASE(g_res.ps_fs_split_lean);   // KH_FX_LEAN: likewise.
    KH_SAFE_RELEASE(g_res.ps_dlf_comp);
    KH_SAFE_RELEASE(g_res.ps_dlf_gather_lean);   // DLF_SPLIT: likewise.
    KH_SAFE_RELEASE(g_res.ps_dlf_class);
    for (ID3D11PixelShader** p : ps) KH_SAFE_RELEASE(*p);

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { "KH_FX_UNIT", "1" },   // cb.hlsl's effect section (the prewarm tables' twin).
        { nullptr, nullptr }
    };

    const std::string fx_src = hlsl_src(HLSL_CB) + hlsl_src(HLSL_EFFECT).c_str();
    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(fx_src.c_str(), "PSEffect", "ps_5_0", defines, &blob);
    if (!err.empty()) return err;
    HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_effect);
    blob->Release();
    if (FAILED(hr)) return "CreatePixelShader(effect) " + hr_str(hr);
    // FX_SPLIT / FX_LINZ (the prewarm tables' twins): non-fatal - absent, the pass that draws one stands down
    // (SSGI skipped at the chain's loop top, dynamicLightFog unarmed, the depth read in place), reported once.
    // FS_SPLIT: absent, fog scatter draws PSEffect's branch.
    static const char* const ent[6] = { "PSFxLinZ", "PSSsgiGather", "PSSsgiAtrous", "PSDlfGather", "PSDlfFilter",
                                             "PSFsSplitS" };
    for (int k = 0; k < 6; ++k) {
        ps_optional(dev, fx_src, ent[k], defines, ps[k], "effect split shader");
    }
    // WSEE_EDGE_COST: fog scatter's side value (non-fatal, the prewarm tables' twin) - absent, fx_side_draw
    // draws side 28 with PSEffect's branch.
    ps_optional(dev, fx_src, "PSFsSide", defines, &g_res.ps_fs_side, "effect split shader");
    // KH_FX_LEAN: fog scatter's entry and dynamicLightFog's composite without the record maths (non-fatal, the
    // prewarm tables' twins: their KH_FX_LEAN jobs) - absent, the full build or PSEffect's branch.
    const D3D_SHADER_MACRO lean[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { "KH_FX_UNIT", "1" },
        { "KH_FX_LEAN", "1" },
        { nullptr, nullptr }
    };
    ps_optional(dev, fx_src, "PSFsSplitS", lean, &g_res.ps_fs_split_lean, "effect split shader");
    ps_optional(dev, fx_src, "PSDlfComp", lean, &g_res.ps_dlf_comp, "effect split shader");
    // FX_LEAN_PASS: PSEffect itself without the record code (non-fatal, the prewarm tables' twin).
    ps_optional(dev, fx_src, "PSEffect", lean, &g_res.ps_effect_lean, "effect split shader");
    // DLF_SPLIT: the gather's lean build and the texel marker (non-fatal, the prewarm tables' twins; the marker
    // with the system built in alone - only an armed pass draws it). Without the lean build the full gather draws
    // alone; with it and without the marker or its stencil, an unarmed pass draws the lean build, an armed one the
    // full.
    ps_optional(dev, fx_src, "PSDlfGather", lean, &g_res.ps_dlf_gather_lean, "effect split shader");
    if constexpr (WSEE_ON) {
        ps_optional(dev, fx_src, "PSDlfClass", defines, &g_res.ps_dlf_class, "effect split shader");
    }
    // WSEE: the run's agreement mask (wsee_agree_draw; non-fatal, the prewarm tables' twin) - multisampled
    // only, as the capture is (wsee_ensure); not made with the system built out (no pass is armed). Absent, no
    // mask is drawn and the armed passes test inline.
    if constexpr (WSEE_ON) {
        if (g_res.depth_sample_count > 1) {
            ps_optional(dev, fx_src, "PSWseeAgree", defines, &g_res.ps_wsee_agree, "world see-through");
        }
    }
    g_res.ps_effect_samples = g_res.depth_sample_count;
    return "";
}

// The composite unit (the injection's shaders, and the flush's near-gap route), compiled once: single-sample
// whatever the depth's count (below).
static uint64_t g_comp_fail_streak = 0;
// Backoff: a failed compile must not retry per injection (a D3DCompile every frame is a framerate collapse).
static float    g_comp_next_retry = 0.0f;
static std::string g_comp_last_err;

inline std::string ensure_composite_shader(ID3D11Device* dev) {
    if (g_res.vs_composite && g_res.ps_composite && g_res.ps_composite_arb &&
        g_res.ps_composite_samples == g_res.comp_depth_samples) {
        return "";
    }

    LARGE_INTEGER _bo_q, _bo_f;
    QueryPerformanceCounter(&_bo_q);
    QueryPerformanceFrequency(&_bo_f);
    const float now_bo = static_cast<float>(static_cast<double>(_bo_q.QuadPart) / static_cast<double>(_bo_f.QuadPart));
    if (g_comp_fail_streak > 0 && now_bo < g_comp_next_retry) return g_comp_last_err;

    if (g_res.vs_composite) { g_res.vs_composite->Release(); g_res.vs_composite = nullptr; }
    if (g_res.ps_composite) { g_res.ps_composite->Release(); g_res.ps_composite = nullptr; }
    if (g_res.ps_composite_arb) { g_res.ps_composite_arb->Release(); g_res.ps_composite_arb = nullptr; }
    // The textured and instanced twins go with the base three: every twin read is gated on guard or
    // ps_composite_arb, so the unit is whole or absent.
    KH_SAFE_RELEASE(g_res.vs_comp_tex);
    KH_SAFE_RELEASE(g_res.ps_comp_tex);
    KH_SAFE_RELEASE(g_res.ps_comp_arb_tex);
    KH_SAFE_RELEASE(g_res.vs_comp_inst);
    KH_SAFE_RELEASE(g_res.vs_comp_inst_tex);
    KH_SAFE_RELEASE(g_res.vs_comp_inst_sk);   // SKIN_POOL.
    KH_SAFE_RELEASE(g_res.vs_comp_inst_sk_tex);

    // comp_depth_samples is the snapshot's count, pinned to 1 by snapshot_composite_depth: this unit always compiles
    // MSAA_DEPTH 0 / SAMPLE_COUNT 1 (composite.hlsl has no MSAA path: it reads the snapshot alone), matching
    // ensure_resources' hard-coded speculation tables.
    const std::string sc = std::to_string(g_res.comp_depth_samples > 0 ? g_res.comp_depth_samples : 1);

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_ARB_DEPTH", "0" },
        { "KH_RECEIVE_TEX", "1" },
        { nullptr, nullptr },
    };
    const D3D_SHADER_MACRO defines_arb[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_ARB_DEPTH", "1" },
        { "KH_RECEIVE_TEX", "1" },
        { nullptr, nullptr },
    };

    const D3D_SHADER_MACRO tx_defines[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" },
        { nullptr, nullptr },
    };
    const D3D_SHADER_MACRO tx_defines_arb[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_ARB_DEPTH", "1" },
        { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" },
        { nullptr, nullptr },
    };

    const std::string comp_src = hlsl_src(HLSL_CB) + hlsl_src(HLSL_COMPOSITE).c_str();
    // The composite unit's ten heavy compiles as one batch. Declared after comp_src and every table it names so the
    // scope joins and frees before they go out of scope; covers the comp_fail early returns.
    const ShaderJob jobs[] = {
        { comp_src.c_str(), "PSComposite", "ps_5_0", defines,          0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", defines_arb,      0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", tx_defines,     0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", tx_defines_arb, 0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", defines,          0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", tx_defines,     0 },
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", defines,      0 },   // INSTANCING twins.
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", tx_defines, 0 },
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", defines,      0 },   // SKIN_POOL twins.
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", tx_defines, 0 },
    };
    ShaderMtScope scope(jobs, _countof(jobs));
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    auto comp_fail = [&](const std::string& e) {
        g_comp_last_err = e;
        if (g_comp_fail_streak == 0) report_error_once_safe("KH RenderIntegration: composite shader: " + e);
        g_comp_fail_streak++;
        g_comp_next_retry = now_bo + 2.0f;
        return e;
    };

    std::string err = compile_shader(comp_src.c_str(), "VSComposite", "vs_5_0", defines, &vs_blob);
    if (!err.empty()) return comp_fail("composite VS: " + err);
    err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", defines, &ps_blob);
    if (!err.empty()) { vs_blob->Release(); return comp_fail("composite PS: " + err); }
    g_comp_fail_streak = 0;
    HRESULT hr = dev->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_res.vs_composite);
    vs_blob->Release();
    if (FAILED(hr)) { ps_blob->Release(); return comp_fail("create composite VS " + hr_str(hr)); }
    hr = dev->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_res.ps_composite);
    if (SUCCEEDED(hr)) {   // PSV: the tail's builtin (D3).
        psv_own(2, ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), g_res.ps_composite);
    }
    ps_blob->Release();
    if (FAILED(hr)) return comp_fail("create composite PS " + hr_str(hr));
    // Arb variant: same source, KH_ARB_DEPTH=1 switches the SV_Depth output signature + the analytic clamp block on.
    ID3DBlob* psa_blob = nullptr;
    err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", defines_arb, &psa_blob);
    if (!err.empty()) return comp_fail("composite PS(arb): " + err);
    hr = dev->CreatePixelShader(psa_blob->GetBufferPointer(), psa_blob->GetBufferSize(), nullptr, &g_res.ps_composite_arb);
    if (SUCCEEDED(hr)) {   // PSV: the tail's builtin (D3).
        psv_own(3, psa_blob->GetBufferPointer(), psa_blob->GetBufferSize(), g_res.ps_composite_arb);
    }
    psa_blob->Release();
    if (FAILED(hr)) return comp_fail("create composite PS(arb) " + hr_str(hr));
    // The twins are released with the base three at the top.
    {
        // The textured twins (non-fatal): VS, PS and the arb PS. tx_defines / tx_defines_arb: the function's top
        // (the SHADER_MT batch names them too - one table, so the twins take the batch's blobs).
        ID3DBlob* vsb = nullptr;
        ID3DBlob* psb = nullptr;
        ID3DBlob* pab = nullptr;
        std::string tx_err = compile_shader(comp_src.c_str(), "VSComposite", "vs_5_0", tx_defines, &vsb);
        if (tx_err.empty()) tx_err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", tx_defines, &psb);
        if (tx_err.empty()) tx_err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", tx_defines_arb, &pab);

        if (tx_err.empty()) {
            HRESULT tx_hr = dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_res.vs_comp_tex);
            if (SUCCEEDED(tx_hr)) tx_hr = dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_res.ps_comp_tex);
            if (SUCCEEDED(tx_hr)) tx_hr = dev->CreatePixelShader(pab->GetBufferPointer(), pab->GetBufferSize(), nullptr, &g_res.ps_comp_arb_tex);
            if (SUCCEEDED(tx_hr)) {   // PSV: the tail's builtins (D3).
                psv_own(4, psb->GetBufferPointer(), psb->GetBufferSize(), g_res.ps_comp_tex);
                psv_own(5, pab->GetBufferPointer(), pab->GetBufferSize(), g_res.ps_comp_arb_tex);
            }

            if (FAILED(tx_hr)) {
                KH_SAFE_RELEASE(g_res.vs_comp_tex);
                KH_SAFE_RELEASE(g_res.ps_comp_tex);
                KH_SAFE_RELEASE(g_res.ps_comp_arb_tex);
                report_error_once_safe("KH RenderIntegration: textured composite: create " + hr_str(tx_hr));
            }
        } else {
            report_error_once_safe("KH RenderIntegration: textured composite: " + tx_err);
        }

        if (vsb) vsb->Release();
        if (psb) psb->Release();
        if (pab) pab->Release();
    }
    // The injection's instanced VS pair (non-fatal); layouts are the static unit's (identical input signatures). A
    // missing twin stands the injection's bucketing down whole (inst_on gates on both) - never a bucket whose
    // members were marked done with no twin to draw them.
    {
        ID3DBlob* b = nullptr;
        std::string in_err = compile_shader(comp_src.c_str(), "VSCompositeInst", "vs_5_0", defines, &b);
        if (in_err.empty() && b) {
            if (FAILED(dev->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &g_res.vs_comp_inst)))
                KH_SAFE_RELEASE(g_res.vs_comp_inst);
            b->Release();
            b = nullptr;
        } else if (!in_err.empty()) {
            report_error_once_safe("KH RenderIntegration: instanced composite: " + in_err);
        }

        in_err = compile_shader(comp_src.c_str(), "VSCompositeInst", "vs_5_0", tx_defines, &b);
        if (in_err.empty() && b) {
            if (FAILED(dev->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr, &g_res.vs_comp_inst_tex)))
                KH_SAFE_RELEASE(g_res.vs_comp_inst_tex);
            b->Release();
        } else if (!in_err.empty()) {
            report_error_once_safe("KH RenderIntegration: instanced composite (textured): " + in_err);
        }
    }
    // SKIN_POOL: VSCompositeInstSk, plain and textured (non-fatal: a missing one keeps skeletal bindings out of
    // every bucket, inst_sk_on). Layout: the static unit's layout_inst_sk (identical input signatures).
    {
        const D3D_SHADER_MACRO* const defs[2] = { defines, tx_defines };
        ID3D11VertexShader** const out[2] = { &g_res.vs_comp_inst_sk, &g_res.vs_comp_inst_sk_tex };
        for (int i = 0; i < 2; ++i) {
            ID3DBlob* b = nullptr;
            const std::string ck_err = compile_shader(comp_src.c_str(), "VSCompositeInstSk", "vs_5_0",
                                                      defs[i], &b);
            if (ck_err.empty() && b) {
                if (FAILED(dev->CreateVertexShader(b->GetBufferPointer(), b->GetBufferSize(), nullptr,
                                                   out[i]))) {
                    *out[i] = nullptr;
                }
            } else if (!ck_err.empty()) {
                report_error_once_safe("KH RenderIntegration: skin pool: twin VSCompositeInstSk - skeletal meshes "
                                       "draw one by one: " + ck_err);
            }
            if (b) b->Release();
        }
    }

    g_res.ps_composite_samples = g_res.comp_depth_samples;
    return "";
}

// Resolve PS, compiled per source MSAA count. Standalone source; the shared vs_fullscreen's extra texcoord outputs
// a subset-reading PS legally ignores.
inline std::string ensure_depth_resolve_shader(ID3D11Device* dev) {
    if (g_res.ps_depth_resolve && g_res.ps_resolve_samples == g_res.depth_sample_count) return "";
    if (g_res.ps_depth_resolve) { g_res.ps_depth_resolve->Release(); g_res.ps_depth_resolve = nullptr; }

    const std::string sc = std::to_string(g_res.depth_sample_count > 0 ? g_res.depth_sample_count : 1);

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { nullptr, nullptr },
    };

    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(hlsl_src(HLSL_DEPTH_RESOLVE).c_str(), "PSDepthResolve", "ps_5_0", defines, &blob);
    if (!err.empty()) return "depth resolve PS: " + err;
    HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_depth_resolve);
    blob->Release();
    if (FAILED(hr)) return "create depth resolve PS " + hr_str(hr);
    g_res.ps_resolve_samples = g_res.depth_sample_count;
    return "";
}
}