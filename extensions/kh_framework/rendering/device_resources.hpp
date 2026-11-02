#pragma once

// The feature half (rendering_integration_features.hpp): the device resources: ensure_resources, the optional shaders,
// the effect, composite and depth-resolve shaders. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/white_placeholder.hpp (declared here for the uses below).
inline void kh_white_ensure(ID3D11Device* dev);

static UINT g_eds_memo_w = 0;
static UINT g_eds_memo_h = 0;

// KH_ENSURE_ONCE: one report per fault, not one per site. The same ensure error reaches several sites a frame (the
// flush, the injection, the seam, the UI flush, the sun pass, the mesh publish), and report_error_once_safe keys on
// the prefixed text, so the bare error is filed as the key first: the first site names itself, every later site
// finds the key and says nothing (two of the 256-message cap's slots per fault; ensure faults are few and distinct).
inline bool kh_ensure_ok(const char* khe_what, const std::string& khe_err) {
    if (khe_err.empty()) return true;
    if (khe_err == KH_SHADERS_COMPILING) return false;
    {
        std::lock_guard<std::mutex> khe_g(g_err_once_mutex);
        if (std::find(g_err_once.begin(), g_err_once.end(), khe_err) != g_err_once.end()) return false;
        if (g_err_once.size() < 256) g_err_once.push_back(khe_err);
    }
    report_error_once_safe(std::string("KH ") + khe_what + ": " + khe_err);
    return false;
}
// Non-fatal shader creation: a failed compile or create is reported once (the text names the entry point, so one
// failing entry does not silence the next) and leaves the slot null; every consumer gates on the pointer.
inline void kh_ps_optional(ID3D11Device* dev, const std::string& src, const char* entry,
                           const D3D_SHADER_MACRO* defs, ID3D11PixelShader** out, const char* label) {
    ID3DBlob* blob = nullptr;
    const std::string err = compile_shader(src.c_str(), entry, "ps_5_0", defs, &blob);
    if (err.empty() && blob) {
        const HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out);
        blob->Release();
        if (FAILED(hr)) {
            *out = nullptr;
            report_error_once_safe(std::string(label) + entry + " could not be created " + hr_str(hr));
        }
    } else if (!err.empty()) {
        report_error_once_safe(std::string(label) + err);   // compile_shader's error text already names the entry.
    }
}
inline void kh_vs_optional(ID3D11Device* dev, const std::string& src, const char* entry,
                           const D3D_SHADER_MACRO* defs, ID3D11VertexShader** out, const char* label) {
    ID3DBlob* blob = nullptr;
    const std::string err = compile_shader(src.c_str(), entry, "vs_5_0", defs, &blob);
    if (err.empty() && blob) {
        const HRESULT hr = dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, out);
        blob->Release();
        if (FAILED(hr)) {
            *out = nullptr;
            report_error_once_safe(std::string(label) + entry + " could not be created " + hr_str(hr));
        }
    } else if (!err.empty()) {
        report_error_once_safe(std::string(label) + err);   // compile_shader's error text already names the entry.
    }
}

// KH_PREWARM_ALL: the two inline-source pixel shaders ensure_resources builds (the UI coverage reset, the
// premultiplying copy), named once so the prewarm batch queues exactly the source the build compiles.
static const char* const KH_PS_ALPHA_ZERO_SRC =
    "float4 main() : SV_Target { return float4(0.0f, 0.0f, 0.0f, 0.0f); }";
static const char* const KH_PS_PREMULT_SRC =
    "Texture2D khSrc : register(t0);"
    "float4 main(float4 pos : SV_Position) : SV_Target"
    "{ float4 c = khSrc.Load(int3(int2(pos.xy), 0));"
    "  return float4(c.rgb * c.a, c.a); }";
// KH_WSEE - the chain's final write when its last pass read the scene's sample colours (kh_fx_chain_run: that
// pass draws into the ping-pong target, this copies it into the scene target): one texel a pixel, no constants. Built
// by kh_wsee_ensure, not ensure_resources (a third inline source, prewarmed with the two).
static const char* const KH_PS_WSEE_COPY_SRC =
    "Texture2D khSrc : register(t0);"
    "float4 main(float4 pos : SV_Position) : SV_Target"
    "{ return khSrc.Load(int3(int2(pos.xy), 0)); }";

inline std::string ensure_resources(ID3D11Device* dev) {
    KH_PROF_SCOPE(KHP_ENSURE_RES);   // KH_PROF.
    // No feature-level test: every unit is vs_5_0 / ps_5_0, so a feature level 10.x device (pre-2010 GPUs) fails
    // the first CreateVertexShader, and this is asked again each call.
    // The mod cache scan is a magic static that walks mod folders on disk; it must not first run on a shader worker
    // (ModFolderSearcher reentrancy). This site runs on the game or render thread before any pool arms.
    kh_mod_cache_dirs();
    // The initialized fast path still tops up mesh VBs: the ensure after kh_mesh_publish creates the new buffers.
    if (g_res.initialized) return ensure_mesh_vbs(dev);

    // KH_CB_OFFSET_EARLY: StateBackup's offset-aware capture keys on g_cb_offsetting, and the white placeholder
    // (kh_white_draw) runs one while the deferrals below return - before the caps read further down sets it. Read
    // here too, so the engine's b0 / b1 windows go back at their offsets in the compile window (a plain restore
    // rebinds a window at constant 0). The read below sets the same value.
    if (!g_cb_offsetting) {
        D3D11_FEATURE_DATA_D3D11_OPTIONS khco_opt = {};
        g_cb_offsetting = SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &khco_opt,
                                                             sizeof(khco_opt))) &&
                          khco_opt.ConstantBufferOffsetting;
    }

    // The deferral is asked before anything is built; a deferred frame arms the white placeholder on its way out.
    if (kh_shader_async_hold()) { kh_white_ensure(dev); return KH_SHADERS_COMPILING; }

    if (!g_reset_hook_installed && RVExtBridge::has_reset_hook()) {
        RVExtBridge::set_reset_hook(&on_engine_reset);
        g_reset_hook_installed = true;
    }

    HRESULT hr;
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* vs_fs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;

    const std::string static_src = kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_STATIC).c_str();
    // The shared receive block (band / atlas textures + compare functions) arms for this unit; the effect unit
    // compiles without it (its depthTex owns t1).
    static const D3D_SHADER_MACRO khcb_rx_defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr },
    };
    // Shared by every KH_TEXTURED compile below (the textured twins, the alpha casters, the instancing twins) and the
    // batch.
    static const D3D_SHADER_MACRO khtx_defines[] = {
        { "KH_RECEIVE_TEX", "1" }, { "KH_TEXTURED", "1" }, { nullptr, nullptr },
    };
    const std::string khsp_sc1 = "1";
    // Every define table in this speculation list must match its ensure_* twin byte for byte: the memo key is an
    // FNV over source, entry, target and the defines in order - one reordered or added entry makes the speculation
    // miss silently.
    const std::string khsp_comp_src =
        kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_COMPOSITE).c_str();
    const D3D_SHADER_MACRO khsp_d0[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", khsp_sc1.c_str() },
        { "KH_ARB_DEPTH", "0" }, { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO khsp_d0a[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", khsp_sc1.c_str() },
        { "KH_ARB_DEPTH", "1" }, { "KH_RECEIVE_TEX", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO khsp_d0t[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", khsp_sc1.c_str() },
        { "KH_RECEIVE_TEX", "1" }, { "KH_TEXTURED", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO khsp_d0ta[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", khsp_sc1.c_str() },
        { "KH_ARB_DEPTH", "1" }, { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" }, { nullptr, nullptr } };
    // Must match ensure_effect_shader byte for byte: MSAA_DEPTH, then KH_FX_UNIT (cb.hlsl's effect section; no
    // SAMPLE_COUNT, no KH_RECEIVE_TEX - the effect unit never receives).
    const std::string khfx_src =
        kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_EFFECT).c_str();
    const D3D_SHADER_MACRO khfx_d0[] = {
        { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    // The rest of the core set rides this batch so the first post FX, depth snapshot, sun prefilter and mirror
    // prepass are memo hits. Twins: ensure_effect_shader (either MSAA_DEPTH value, the main depth's) and
    // kh_pip_fx_shader (MSAA_DEPTH 0, the PIP's), ensure_depth_resolve_shader and ensure_ssao_shaders (every count:
    // KH_PREWARM_ALL below), kh_vmsee_ensure (PSDepthS0, the multisampled counts alone - KH_VM_SEE), ensure_sun_pf
    // (empty table), kh_vmir_ensure_gpu (null) - the hash reads those two alike.
    const D3D_SHADER_MACRO khfx_d1[] = {
        { "MSAA_DEPTH", "1" }, { "KH_FX_UNIT", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO khfx_l0[] = {   // KH_FX_LEAN: ensure_effect_shader's lean builds (khes_lean).
        { "MSAA_DEPTH", "0" }, { "KH_FX_UNIT", "1" }, { "KH_FX_LEAN", "1" }, { nullptr, nullptr } };
    const D3D_SHADER_MACRO khfx_l1[] = {
        { "MSAA_DEPTH", "1" }, { "KH_FX_UNIT", "1" }, { "KH_FX_LEAN", "1" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO khpw_none[] = { { nullptr, nullptr } };
    // VSWhite / PSWhite stay out of this list: they are the placeholder drawn WHILE it compiles, so kh_white_ensure
    // builds them at the arming, on the arming thread beside the pool - queued, compile_shader would only make that
    // thread wait for a worker to reach them.
    const KhShaderJob khsp_jobs[] = {
        { static_src.c_str(), "PSMain",      "ps_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSMain",      "ps_5_0", khtx_defines,    0 },
        { khsp_comp_src.c_str(), "PSComposite", "ps_5_0", khsp_d0,   0 },
        { khsp_comp_src.c_str(), "PSComposite", "ps_5_0", khsp_d0a,  0 },
        { khsp_comp_src.c_str(), "PSComposite", "ps_5_0", khsp_d0t,  0 },
        { khsp_comp_src.c_str(), "PSComposite", "ps_5_0", khsp_d0ta, 0 },
        { static_src.c_str(), "PSMaskCast",  "ps_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSDlsWorld",  "ps_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSDlsWorldFog", "ps_5_0", khcb_rx_defines, 0 },   // KH_DLSW_FOG.
        { static_src.c_str(), "VSDlsMask",   "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSDlsMask",   "ps_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSDlsMaskA",  "ps_5_0", khtx_defines,    0 },   // KH_DLSW_MASK_ALPHA.
        { static_src.c_str(), "PSMaskPrime", "ps_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSInjDepthA", "ps_5_0", khtx_defines,    0 },
        { static_src.c_str(), "VSMain",      "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSMain",      "vs_5_0", khtx_defines,    0 },
        { static_src.c_str(), "VSMainInst",  "vs_5_0", khcb_rx_defines, 0 },   // KH_INSTANCING twins.
        { static_src.c_str(), "VSMainInst",  "vs_5_0", khtx_defines,    0 },
        { static_src.c_str(), "VSMainInstSk",  "vs_5_0", khcb_rx_defines, 0 },   // KH_SKIN_POOL twins.
        { static_src.c_str(), "VSMainInstSk",  "vs_5_0", khtx_defines,    0 },
        { static_src.c_str(), "VSSunDepthSk",  "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSSeamInstSk",  "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSMirrorInstSk", "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSMirror",    "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSSeamInst",   "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "PSSeamFoot",   "ps_5_0", khcb_rx_defines, 0 },   // KH_VOL_FOOT.
        { static_src.c_str(), "PSReplayMerge", "ps_5_0", khcb_rx_defines, 0 },   // KH_VOL_REPLAY (kh_ps_optional's twin).
        { static_src.c_str(), "PSReplayMergeMir", "ps_5_0", khcb_rx_defines, 0 },   // KH_MIR_REPLAY (likewise).
        { static_src.c_str(), "VSMirrorInst", "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSFullscreen","vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSSunDepth",  "vs_5_0", khcb_rx_defines, 0 },
        { static_src.c_str(), "VSSunDepthA", "vs_5_0", khtx_defines,    0 },   // KH_CAST_ALPHA twins.
        { static_src.c_str(), "VSSunDepthAObj", "vs_5_0", khtx_defines, 0 },   // KH_CAST_ALPHA: the fallback's.
        { static_src.c_str(), "PSSunDepthA", "ps_5_0", khtx_defines,    0 },
        { khsp_comp_src.c_str(), "VSComposite", "vs_5_0", khsp_d0,  0 },
        { khsp_comp_src.c_str(), "VSComposite", "vs_5_0", khsp_d0t, 0 },
        { khsp_comp_src.c_str(), "VSCompositeInst", "vs_5_0", khsp_d0,  0 },   // KH_INSTANCING twins.
        { khsp_comp_src.c_str(), "VSCompositeInst", "vs_5_0", khsp_d0t, 0 },
        { khsp_comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", khsp_d0,  0 },   // KH_SKIN_POOL twins.
        { khsp_comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", khsp_d0t, 0 },
        { khfx_src.c_str(), "PSEffect", "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSEffect", "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSEffect", "ps_5_0", khfx_l0, 0 },   // KH_FX_LEAN_PASS.
        { khfx_src.c_str(), "PSEffect", "ps_5_0", khfx_l1, 0 },
        // KH_FX_SPLIT / KH_FX_LINZ / KH_FS_SPLIT: ensure_effect_shader's other six, at either MSAA_DEPTH (the main
        // depth's).
        { khfx_src.c_str(), "PSFxLinZ",     "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSFxLinZ",     "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSSsgiGather", "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSSsgiGather", "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSSsgiAtrous", "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSSsgiAtrous", "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSDlfGather",  "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSDlfGather",  "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSDlfFilter",  "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSDlfFilter",  "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSFsSplitS",   "ps_5_0", khfx_d0, 0 },   // KH_FS_SPLIT.
        { khfx_src.c_str(), "PSFsSplitS",   "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSFsSide",     "ps_5_0", khfx_d0, 0 },   // KH_WSEE_EDGE_COST.
        { khfx_src.c_str(), "PSFsSide",     "ps_5_0", khfx_d1, 0 },
        { khfx_src.c_str(), "PSFsSplitS",   "ps_5_0", khfx_l0, 0 },   // KH_FX_LEAN.
        { khfx_src.c_str(), "PSFsSplitS",   "ps_5_0", khfx_l1, 0 },
        { khfx_src.c_str(), "PSDlfComp",    "ps_5_0", khfx_l0, 0 },
        { khfx_src.c_str(), "PSDlfComp",    "ps_5_0", khfx_l1, 0 },
        { khfx_src.c_str(), "PSDlfGather",  "ps_5_0", khfx_l0, 0 },   // KH_DLF_SPLIT.
        { khfx_src.c_str(), "PSDlfGather",  "ps_5_0", khfx_l1, 0 },
        { khfx_src.c_str(), "PSGlowSeed", "ps_5_0", khfx_d0, 0 },   // KH_GLOW_PYR (the creation below).
        { khfx_src.c_str(), "PSGlowDown", "ps_5_0", khfx_d0, 0 },
        { khfx_src.c_str(), "PSAnaSeed", "ps_5_0", khfx_d0, 0 },   // KH_ANA_PYR (the creation below).
        { khfx_src.c_str(), "PSAnaDown", "ps_5_0", khfx_d0, 0 },
        { kh_hlsl_src(KH_HLSL_SUNPF).c_str(),   "VSPf",    "vs_5_0", khpw_none, 0 },
        { kh_hlsl_src(KH_HLSL_SUNPF).c_str(),   "PSPf",    "ps_5_0", khpw_none, 0 },
        { kh_hlsl_src(KH_HLSL_SUNPF).c_str(),   "PSPfMip", "ps_5_0", khpw_none, 0 },
        { kh_hlsl_src(KH_HLSL_VMIR_CS).c_str(), "VSMirB2", "vs_5_0", nullptr,   0 },   // KH_MIR_DRAW.
        { kh_hlsl_src(KH_HLSL_VMIR_CS).c_str(), "PSMirB2", "ps_5_0", nullptr,   0 },
    };
    // KH_PREWARM_ALL: every other built-in unit (KH_WSEE's depth_resolve entries too), so no ensure compiles one
    // cold once the batch has run: a memo hit, or a disk hit after a later batch's finish released the memo. The
    // depth-resolve and SSAO units key on the main depth's sample count (their ensures' tables, MSAA_DEPTH +
    // SAMPLE_COUNT, an unknown count read as 1), usually not adopted yet at this first ensure (it follows the video
    // settings): each count Arma's FSAA uses (1, 2, 4, 8) is queued, plus an adopted count outside those. A count
    // never queued still compiles at its ensure (from the disk cache once seen). Then VSSkinSo (kh_skin_so_ensure's
    // empty table, and kh_skin_so_big_ensure's KH_SKIN_SO_DEFS_L) and the three inline-source shaders
    // (ensure_resources' two, KH_WSEE's copy).
    std::vector<KhShaderJob> khsp_all(khsp_jobs, khsp_jobs + _countof(khsp_jobs));
    // KH_WSEE: the agreement mask, multisampled only; this and every see-through job below only with the system built
    // in (KH_WSEE_ON; nothing would create them).
    if constexpr (KH_WSEE_ON) khsp_all.push_back({ khfx_src.c_str(), "PSWseeAgree", "ps_5_0", khfx_d1, 0 });
    if constexpr (KH_WSEE_ON) {   // KH_DLF_SPLIT: the gather's texel marker (its lean build is in the table above).
        khsp_all.push_back({ khfx_src.c_str(), "PSDlfClass", "ps_5_0", khfx_d0, 0 });
        khsp_all.push_back({ khfx_src.c_str(), "PSDlfClass", "ps_5_0", khfx_d1, 0 });
    }
    static const D3D_SHADER_MACRO khpw_dr1[] = {
        { "MSAA_DEPTH", "0" }, { "SAMPLE_COUNT", "1" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO khpw_dr2[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "2" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO khpw_dr4[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "4" }, { nullptr, nullptr } };
    static const D3D_SHADER_MACRO khpw_dr8[] = {
        { "MSAA_DEPTH", "1" }, { "SAMPLE_COUNT", "8" }, { nullptr, nullptr } };
    const UINT khpw_live = g_res.depth_sample_count;
    const std::string khpw_sc = std::to_string(khpw_live);
    const D3D_SHADER_MACRO khpw_drx[] = {
        { "MSAA_DEPTH", khpw_live > 1 ? "1" : "0" }, { "SAMPLE_COUNT", khpw_sc.c_str() }, { nullptr, nullptr } };
    const bool khpw_other = khpw_live > 0 && khpw_live != 1 && khpw_live != 2 && khpw_live != 4 && khpw_live != 8;
    const D3D_SHADER_MACRO* const khpw_dr[5] = { khpw_dr1, khpw_dr2, khpw_dr4, khpw_dr8,
                                                 khpw_other ? khpw_drx : nullptr };
    static const char* const khpw_sa_ent[5] = {   // KH_SSAO (ensure_ssao_shaders's five; KH_NEARZ_MARK).
        "PSSsaoDepth", "PSSsaoMain", "PSSsaoBlur", "PSSsaoApply", "PSSsaoNearMark" };
    for (const D3D_SHADER_MACRO* khpw_d : khpw_dr) {
        if (!khpw_d) continue;
        khsp_all.push_back({ kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSDepthResolve", "ps_5_0", khpw_d, 0 });
        if (khpw_d != khpw_dr1) {   // KH_VM_SEE: multisampled only (kh_vmsee_ensure).
            khsp_all.push_back({ kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSDepthS0", "ps_5_0", khpw_d, 0 });
            // KH_WSEE: kh_wsee_ensure's late-opaque PS and the layer-depth split's MRT twin, at the same tables
            // (KH_PREWARM_ALL: kh_wsee_ensure compiles both at the flush).
            if constexpr (KH_WSEE_ON) {
                const char* const khpw_src = kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str();
                khsp_all.push_back({ khpw_src, "PSWseeOne", "ps_5_0", khpw_d, 0 });
                khsp_all.push_back({ khpw_src, "PSWseeOneL", "ps_5_0", khpw_d, 0 });
                // KH_WSEE_OQ2: kh_wsee_ensure's D3m twins and D3's resolve, at the same tables.
                khsp_all.push_back({ khpw_src, "PSWseeOneZ", "ps_5_0", khpw_d, 0 });
                khsp_all.push_back({ khpw_src, "PSWseeOneZL", "ps_5_0", khpw_d, 0 });
                khsp_all.push_back({ khpw_src, "PSWseeD3Res", "ps_5_0", khpw_d, 0 });
            }
        }
        for (const char* khpw_e : khpw_sa_ent) {
            khsp_all.push_back({ kh_hlsl_src(KH_HLSL_SSAO).c_str(), khpw_e, "ps_5_0", khpw_d, 0 });
        }
    }
    if constexpr (KH_WSEE_ON) {   // KH_PSB_ZB: kh_wsee_ensure's block bound, at KH_PSB_LS alone.
        khsp_all.push_back({ kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSPsbZb", "ps_5_0", KH_PSB_ZB_DEFS, 0 });
    }
    if (KH_SKIN_GPU_ON) khsp_all.push_back({ kh_hlsl_src(KH_HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", khpw_none, 0 });
    if (KH_SKIN_GPU_ON) {   // KH_SKIN_SO_BIG: the large class (KH_SKIN_SO_LAZY: kh_skin_so_big_ensure, after it).
        khsp_all.push_back({ kh_hlsl_src(KH_HLSL_SKIN).c_str(), "VSSkinSo", "vs_5_0", KH_SKIN_SO_DEFS_L, 0 });
    }
    khsp_all.push_back({ KH_PS_ALPHA_ZERO_SRC, "main", "ps_5_0", nullptr, 0 });
    khsp_all.push_back({ KH_PS_PREMULT_SRC,    "main", "ps_5_0", nullptr, 0 });
    if constexpr (KH_WSEE_ON) {   // KH_WSEE (kh_wsee_ensure): with the system built in alone.
        khsp_all.push_back({ KH_PS_WSEE_COPY_SRC,  "main", "ps_5_0", nullptr, 0 });
    }
    const size_t khsp_n = khsp_all.size();
    if (kh_shader_async_gate(khsp_all.data(), khsp_n)) {
        kh_white_ensure(dev);   // The arming frame gets a placeholder too.
        return KH_SHADERS_COMPILING;
    }
    std::string err = compile_shader(static_src.c_str(), "VSMain", "vs_5_0", khcb_rx_defines, &vs_blob);
    if (!err.empty()) return err;
    if (!g_vmir_vs) {   // VSMirror - non-fatal (see the declaration).
        // Guarded so an ensure retry cannot double-create.
        ID3DBlob* khmv_blob = nullptr;
        const std::string khmv_err = compile_shader(static_src.c_str(), "VSMirror",
                                                    "vs_5_0", khcb_rx_defines, &khmv_blob);
        if (khmv_err.empty() && khmv_blob) {
            if (FAILED(dev->CreateVertexShader(khmv_blob->GetBufferPointer(),
                                               khmv_blob->GetBufferSize(),
                                               nullptr, &g_vmir_vs))) {
                g_vmir_vs = nullptr;
                report_error_once_safe("KH mirror: VSMirror could not be created (the mirror stands down)");
            }
            khmv_blob->Release();
        } else {
            report_error_once_safe("KH mirror: VSMirror compile failed (the mirror stands down): " + khmv_err);
        }
    }
    err = compile_shader(static_src.c_str(), "VSFullscreen", "vs_5_0", khcb_rx_defines, &vs_fs_blob);
    if (!err.empty()) { vs_blob->Release(); return err; }
    err = compile_shader(static_src.c_str(), "PSMain", "ps_5_0", khcb_rx_defines, &ps_blob);
    if (!err.empty()) { vs_blob->Release(); vs_fs_blob->Release(); return err; }
    // Reflect the blob (compiled or served from .khsc) and compare its declared cbuffer sizes to the C++ split.
    // Non-fatal.
    {
        ID3D11ShaderReflection* khcm_r = nullptr;
        if (ps_blob && SUCCEEDED(D3DReflect(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(),
                                            __uuidof(ID3D11ShaderReflection),
                                            reinterpret_cast<void**>(&khcm_r))) && khcm_r) {
            auto khcm_size = [&](const char* khcm_name) -> uint32_t {
                ID3D11ShaderReflectionConstantBuffer* khcm_cb = khcm_r->GetConstantBufferByName(khcm_name);
                D3D11_SHADER_BUFFER_DESC khcm_d = {};
                return (khcm_cb && SUCCEEDED(khcm_cb->GetDesc(&khcm_d))) ? khcm_d.Size : 0u;
            };
            g_cb_mirror_obj_b = khcm_size("CBObj");
            g_cb_mirror_frame_b = khcm_size("CBFrame");
            g_cb_mirror_bad = g_cb_mirror_obj_b != static_cast<uint32_t>(KH_CBOBJ_BYTES) ||
                              g_cb_mirror_frame_b != static_cast<uint32_t>(KH_CBFRAME_BYTES);
            khcm_r->Release();
            if (g_cb_mirror_bad) {
                report_error_once_safe("KH CB mirror: HLSL CBObj/CBFrame " + std::to_string(g_cb_mirror_obj_b) + "/" +
                                std::to_string(g_cb_mirror_frame_b) + " B vs C++ " +
                                std::to_string(KH_CBOBJ_BYTES) + "/" + std::to_string(KH_CBFRAME_BYTES) + " B");
            }
        }
    }

    // Non-fatal entry points: each consumer gates on its pointer.
    kh_ps_optional(dev, static_src, "PSInjDepthA", khtx_defines, &g_res.ps_inj_depth_a, "KH inject-depth alpha shader: ");   // KH_FOOTPRINT_ALPHA (KH_TEXTURED for KhMatRouteG).
    kh_ps_optional(dev, static_src, "PSMaskCast", khcb_rx_defines, &g_res.ps_maskcast, "KH maskcast shader: ");   // Analytic mask cast.
    kh_ps_optional(dev, static_src, "PSReplayMerge", khcb_rx_defines, &g_res.ps_rpmerge, "KH replay merge shader: ");   // KH_VOL_REPLAY.
    kh_ps_optional(dev, static_src, "PSReplayMergeMir", khcb_rx_defines, &g_res.ps_rpmergemir, "KH mirror merge shader: ");   // KH_MIR_REPLAY.
    kh_ps_optional(dev, static_src, "PSDlsWorld", khcb_rx_defines, &g_res.ps_dls_world, "KH dlsworld shader: ");   // KH_DLS_WORLD receive.
    kh_ps_optional(dev, static_src, "PSSeamFoot", khcb_rx_defines, &g_res.ps_seam_foot, "KH seam footprint shader: ");   // KH_VOL_FOOT; absent = the depth-plane witness.
    kh_ps_optional(dev, static_src, "PSDlsWorldFog", khcb_rx_defines, &g_res.ps_dls_world_fog, "KH dlsworld fog shader: ");   // KH_DLSW_FOG; absent = the plain multiply.
    kh_vs_optional(dev, static_src, "VSDlsMask", khcb_rx_defines, &g_res.vs_dls_mask, "KH dlsmask VS: ");   // KH_DLSW_MASK pair: a failed
    kh_ps_optional(dev, static_src, "PSDlsMask", khcb_rx_defines, &g_res.ps_dls_mask, "KH dlsmask PS: ");   // compile leaves the mask off.
    kh_ps_optional(dev, static_src, "PSDlsMaskA", khtx_defines, &g_res.ps_dls_mask_a, "KH dlsmask alpha PS: ");   // KH_DLSW_MASK_ALPHA (KH_TEXTURED for KhMatRouteG); absent = alpha casters mask opaque.
    kh_ps_optional(dev, static_src, "PSMaskPrime", khcb_rx_defines, &g_res.ps_maskprime, "KH maskprime shader: ");   // Priming pass, same contract.
    // KH_GLOW_PYR: the effect unit's two pyramid builders (MSAA_DEPTH 0 - neither reads depth); absent = direct taps.
    kh_ps_optional(dev, khfx_src, "PSGlowSeed", khfx_d0, &g_res.ps_glow_seed, "KH glow seed shader: ");
    kh_ps_optional(dev, khfx_src, "PSGlowDown", khfx_d0, &g_res.ps_glow_down, "KH glow down shader: ");
    // KH_ANA_PYR: anamorphic's two, likewise (MSAA_DEPTH 0); absent = its direct taps.
    kh_ps_optional(dev, khfx_src, "PSAnaSeed", khfx_d0, &g_res.ps_ana_seed, "KH anamorphic seed shader: ");
    kh_ps_optional(dev, khfx_src, "PSAnaDown", khfx_d0, &g_res.ps_ana_down, "KH anamorphic down shader: ");

    {   // Instanced sun-depth VS + layout: non-fatal (per-caster loop covers).
        ID3DBlob* sd_blob = nullptr;
        const std::string sd_err = compile_shader(static_src.c_str(), "VSSunDepth", "vs_5_0", khcb_rx_defines, &sd_blob);

        if (sd_err.empty() && sd_blob) {
            if (SUCCEEDED(dev->CreateVertexShader(sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), nullptr, &g_res.vs_sundepth))) {
                D3D11_INPUT_ELEMENT_DESC sl[] = {   // KH_OBJBUF: mesh slot 0 + the lane on slot 1.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // KhInstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };

                if (FAILED(dev->CreateInputLayout(sl, 4, sd_blob->GetBufferPointer(), sd_blob->GetBufferSize(), &g_res.layout_sundepth))) {
                    KH_SAFE_RELEASE(g_res.vs_sundepth);
                }
            }

            sd_blob->Release();
        } else if (!sd_err.empty()) {
            report_error_once_safe("KH sun-depth shader: " + sd_err);
        }
    }

    // The seam transport's instanced twins. Both consume the sun ladder's lane shape (VSInSeam mirrors VSInSun), so
    // they bind g_res.layout_sundepth. Non-fatal: either one absent leaves that loop on its per-caster path.
    {
        ID3D11VertexShader** const khsi_out[2] = { &g_res.vs_seam_inst, &g_res.vs_mirror_inst };
        const char* const khsi_ent[2] = { "VSSeamInst", "VSMirrorInst" };

        for (int khsi_i = 0; khsi_i < 2; ++khsi_i) {
            ID3DBlob* khsi_blob = nullptr;
            const std::string khsi_err = compile_shader(static_src.c_str(), khsi_ent[khsi_i],
                                                        "vs_5_0", khcb_rx_defines, &khsi_blob);

            if (khsi_err.empty() && khsi_blob) {
                if (FAILED(dev->CreateVertexShader(khsi_blob->GetBufferPointer(),
                                                   khsi_blob->GetBufferSize(),
                                                   nullptr, khsi_out[khsi_i]))) {
                    *khsi_out[khsi_i] = nullptr;
                }
            } else if (!khsi_err.empty()) {
                report_error_once_safe(std::string("KH seam: instanced VS ") + khsi_ent[khsi_i] +
                                       " - that loop stays per caster: " + khsi_err);
            }

            if (khsi_blob) khsi_blob->Release();
        }
    }

    // The alpha-aware sun-depth VS/PS + their layout (mesh slot 0 with the uv, the same per-instance slot 1).
    // Non-fatal: absent, every caster draws whole. Compiled KH_TEXTURED for KhMatRouteG.
    {
        ID3DBlob* khsa_vb = nullptr;
        ID3DBlob* khsa_pb = nullptr;
        std::string khsa_err = compile_shader(static_src.c_str(), "VSSunDepthA", "vs_5_0", khtx_defines, &khsa_vb);
        if (khsa_err.empty()) khsa_err = compile_shader(static_src.c_str(), "PSSunDepthA", "ps_5_0", khtx_defines, &khsa_pb);

        if (khsa_err.empty() && khsa_vb && khsa_pb) {
            HRESULT khsa_hr = dev->CreateVertexShader(khsa_vb->GetBufferPointer(), khsa_vb->GetBufferSize(), nullptr, &g_res.vs_sundepth_a);
            if (SUCCEEDED(khsa_hr)) khsa_hr = dev->CreatePixelShader(khsa_pb->GetBufferPointer(), khsa_pb->GetBufferSize(), nullptr, &g_res.ps_sundepth_a);

            if (SUCCEEDED(khsa_hr)) {
                const D3D11_INPUT_ELEMENT_DESC khsa_el[] = {   // KH_OBJBUF: + uv on slot 0.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // KhInstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };
                khsa_hr = dev->CreateInputLayout(khsa_el, 5, khsa_vb->GetBufferPointer(), khsa_vb->GetBufferSize(), &g_res.layout_sundepth_a);
            }

            if (FAILED(khsa_hr)) {
                KH_SAFE_RELEASE(g_res.vs_sundepth_a);
                KH_SAFE_RELEASE(g_res.ps_sundepth_a);
                KH_SAFE_RELEASE(g_res.layout_sundepth_a);
                report_error_once_safe("KH sun-depth alpha shaders: create " + hr_str(khsa_hr));
            }
        } else if (!khsa_err.empty()) {
            report_error_once_safe("KH sun-depth alpha shaders: " + khsa_err);
        }

        if (khsa_vb) khsa_vb->Release();
        if (khsa_pb) khsa_pb->Release();
    }
    // KH_CAST_ALPHA: VSSunDepthA's per-object twin and its layout (the mesh's own four lanes, validated against
    // its blob) - the depth maps' per-object route (KH_SUN_OBJ) draws its alpha casters with it and PSSunDepthA.
    // Non-fatal: absent, that route draws them whole.
    {
        ID3DBlob* khso_vb = nullptr;
        const std::string khso_err = compile_shader(static_src.c_str(), "VSSunDepthAObj", "vs_5_0", khtx_defines,
                                                    &khso_vb);
        if (khso_err.empty() && khso_vb) {
            HRESULT khso_hr = dev->CreateVertexShader(khso_vb->GetBufferPointer(), khso_vb->GetBufferSize(), nullptr,
                                                      &g_res.vs_sundepth_ao);
            if (SUCCEEDED(khso_hr)) {
                const D3D11_INPUT_ELEMENT_DESC khso_el[] = {   // layout_tex's lanes.
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                khso_hr = dev->CreateInputLayout(khso_el, 4, khso_vb->GetBufferPointer(), khso_vb->GetBufferSize(),
                                                 &g_res.layout_sundepth_ao);
            }
            if (FAILED(khso_hr)) {
                KH_SAFE_RELEASE(g_res.vs_sundepth_ao);
                KH_SAFE_RELEASE(g_res.layout_sundepth_ao);
                report_error_once_safe("KH sun-depth alpha fallback shader: create " + hr_str(khso_hr));
            }
        } else if (!khso_err.empty()) {
            report_error_once_safe("KH sun-depth alpha fallback shader: " + khso_err);
        }
        if (khso_vb) khso_vb->Release();
    }

    hr = dev->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &g_res.vs);
    if (FAILED(hr)) { vs_blob->Release(); vs_fs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader " + hr_str(hr); }
    hr = dev->CreateVertexShader(vs_fs_blob->GetBufferPointer(), vs_fs_blob->GetBufferSize(), nullptr, &g_res.vs_fullscreen);
    vs_fs_blob->Release();
    if (FAILED(hr)) { vs_blob->Release(); ps_blob->Release(); g_res.release(); return "CreateVertexShader(fullscreen) " + hr_str(hr); }
    hr = dev->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &g_res.ps);
    if (SUCCEEDED(hr)) kh_psv_own(0, ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), g_res.ps);   // KH_PSV.
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
        const std::string khmv_err = ensure_mesh_vbs(dev);
        if (!khmv_err.empty()) { g_res.release(); return khmv_err; }
    }

    // KH_TEXTURED static twins (non-fatal): the same source with KH_TEXTURED=1. The 4-element layout is validated
    // against the textured VS blob and serves both textured VS's (identical input signatures).
    {
        // khtx_defines: the function's top (the KH_SHADER_MT batch names it too).
        ID3DBlob* khtx_vsb = nullptr;
        ID3DBlob* khtx_psb = nullptr;
        std::string khtx_err = compile_shader(static_src.c_str(), "VSMain", "vs_5_0", khtx_defines, &khtx_vsb);
        if (khtx_err.empty()) khtx_err = compile_shader(static_src.c_str(), "PSMain", "ps_5_0", khtx_defines, &khtx_psb);

        if (khtx_err.empty()) {
            HRESULT khtx_hr = dev->CreateVertexShader(khtx_vsb->GetBufferPointer(), khtx_vsb->GetBufferSize(), nullptr, &g_res.vs_tex);
            if (SUCCEEDED(khtx_hr)) khtx_hr = dev->CreatePixelShader(khtx_psb->GetBufferPointer(), khtx_psb->GetBufferSize(), nullptr, &g_res.ps_tex);
            if (SUCCEEDED(khtx_hr)) {   // KH_PSV: the tail's builtin (D3).
                kh_psv_own(1, khtx_psb->GetBufferPointer(), khtx_psb->GetBufferSize(), g_res.ps_tex);
            }

            if (SUCCEEDED(khtx_hr)) {
                const D3D11_INPUT_ELEMENT_DESC khtx_il[] = {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                    { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                };
                khtx_hr = dev->CreateInputLayout(khtx_il, 4, khtx_vsb->GetBufferPointer(), khtx_vsb->GetBufferSize(), &g_res.layout_tex);
            }

            if (SUCCEEDED(khtx_hr) && !g_res.mat_sampler) {
                D3D11_SAMPLER_DESC khtx_sd = {};
                khtx_sd.Filter = D3D11_FILTER_ANISOTROPIC;
                // KH_ANISO_16: the hardware's full 16x for our materials (cheap on any D3D11 GPU) - textures on
                // surfaces seen at a grazing angle stay sharp twice as far out. Twin: cb.hlsl KhToksvigA2's model.
                khtx_sd.MaxAnisotropy = 16;
                khtx_sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
                khtx_sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
                khtx_sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
                khtx_sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
                khtx_sd.MaxLOD = D3D11_FLOAT32_MAX;
                khtx_hr = dev->CreateSamplerState(&khtx_sd, &g_res.mat_sampler);
            }

            if (FAILED(khtx_hr)) {
                KH_SAFE_RELEASE(g_res.vs_tex);
                KH_SAFE_RELEASE(g_res.ps_tex);
                KH_SAFE_RELEASE(g_res.layout_tex);
                report_error_once_safe("KH textured shaders: create " + hr_str(khtx_hr));
            }
        } else {
            report_error_once_safe("KH textured shaders: " + khtx_err);
        }

        if (khtx_vsb) khtx_vsb->Release();
        if (khtx_psb) khtx_psb->Release();
    }

    // Instancing twins (non-fatal): VSMainInst plain and KH_TEXTURED, each with its layout (the mesh slot's elements
    // plus VSInst on slot 1), also serving the composite twins (identical input signatures). Then the two stream
    // rings.
    {
        auto khin_make = [&](const D3D_SHADER_MACRO* khin_defs, bool khin_tex,
                             ID3D11VertexShader** khin_vs, ID3D11InputLayout** khin_il) -> bool {
            ID3DBlob* khin_b = nullptr;
            const std::string khin_err = compile_shader(static_src.c_str(), "VSMainInst", "vs_5_0", khin_defs, &khin_b);

            if (!khin_err.empty() || !khin_b) {
                if (!khin_err.empty()) report_error_once_safe("KH instanced shaders: " + khin_err);
                return false;
            }

            HRESULT khin_hr = dev->CreateVertexShader(khin_b->GetBufferPointer(), khin_b->GetBufferSize(), nullptr, khin_vs);

            if (SUCCEEDED(khin_hr)) {
                const D3D11_INPUT_ELEMENT_DESC khin_el[] = {   // KH_OBJBUF: the lane on slot 1.
                    { "POSITION", 0,  DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "NORMAL",   0,  DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 0,  DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TANGENT",  0,  DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                    { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,          1, 0,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // KhInstLane.slot
                    { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT,   1, 4,  D3D11_INPUT_PER_INSTANCE_DATA, 1 },   // dither, alpha, matBase
                };
                // position / normal then the 2 instance elements (the textured pair is skipped, not renumbered).
                D3D11_INPUT_ELEMENT_DESC khin_plain[4];
                khin_plain[0] = khin_el[0];
                khin_plain[1] = khin_el[1];
                for (int khin_k = 0; khin_k < 2; ++khin_k) khin_plain[2 + khin_k] = khin_el[4 + khin_k];
                khin_hr = dev->CreateInputLayout(khin_tex ? khin_el : khin_plain, khin_tex ? 6u : 4u,
                                                 khin_b->GetBufferPointer(), khin_b->GetBufferSize(), khin_il);
            }

            khin_b->Release();

            if (FAILED(khin_hr)) {
                KH_SAFE_RELEASE(*khin_vs);
                KH_SAFE_RELEASE(*khin_il);
                report_error_once_safe("KH instanced shaders: create " + hr_str(khin_hr));
                return false;
            }

            return true;
        };

        bool khin_ok = khin_make(khcb_rx_defines, false, &g_res.vs_inst, &g_res.layout_inst);
        if (khin_ok) khin_ok = khin_make(khtx_defines, true, &g_res.vs_inst_tex, &g_res.layout_inst_tex);

        if (khin_ok) {
            D3D11_BUFFER_DESC khin_bd = {};
            khin_bd.ByteWidth = KH_INST_STRIDE * KH_INST_CAP;
            khin_bd.Usage = D3D11_USAGE_DYNAMIC;
            khin_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            khin_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            for (int khin_i = 0; khin_i < 2 && khin_ok; ++khin_i) {
                g_res.inst_cursor[khin_i] = 0;
                g_res.inst_fresh[khin_i] = true;
                const HRESULT khin_hr = dev->CreateBuffer(&khin_bd, nullptr, &g_res.inst_vb[khin_i]);

                if (FAILED(khin_hr)) {
                    report_error_once_safe("KH instanced stream: create " + hr_str(khin_hr));
                    khin_ok = false;
                }
            }
        }

        if (!khin_ok) {
            KH_SAFE_RELEASE(g_res.vs_inst);
            KH_SAFE_RELEASE(g_res.vs_inst_tex);
            KH_SAFE_RELEASE(g_res.layout_inst);
            KH_SAFE_RELEASE(g_res.layout_inst_tex);
            for (int khin_i = 0; khin_i < 2; ++khin_i) KH_SAFE_RELEASE(g_res.inst_vb[khin_i]);
        }
    }

    // KH_SKIN_POOL - the static unit's vertex-pulling twins (non-fatal): the colour buckets' VSMainInstSk pair and
    // the depth runs' three, with the one layout they share (identical input signatures: the lane on slot 1, the
    // vertex id; nothing on slot 0), made against the first that compiles. A missing colour twin keeps skeletal
    // bindings out of the buckets (kh_inst_sk_on); a missing depth twin leaves that route's skeletal casters in runs
    // of one (kh_cast_pool_of).
    {
        struct KhSkTwin { const char* ent; const D3D_SHADER_MACRO* defs; ID3D11VertexShader** out; };
        const KhSkTwin khsk_tw[] = {
            { "VSMainInstSk",   khcb_rx_defines, &g_res.vs_inst_sk },
            { "VSMainInstSk",   khtx_defines,    &g_res.vs_inst_sk_tex },
            { "VSSunDepthSk",   khcb_rx_defines, &g_res.vs_sundepth_sk },
            { "VSSeamInstSk",   khcb_rx_defines, &g_res.vs_seam_inst_sk },
            { "VSMirrorInstSk", khcb_rx_defines, &g_res.vs_mirror_inst_sk },
        };
        for (const KhSkTwin& khsk_t : khsk_tw) {
            ID3DBlob* khsk_b = nullptr;
            const std::string khsk_err = compile_shader(static_src.c_str(), khsk_t.ent, "vs_5_0", khsk_t.defs, &khsk_b);
            if (khsk_err.empty() && khsk_b) {
                if (FAILED(dev->CreateVertexShader(khsk_b->GetBufferPointer(), khsk_b->GetBufferSize(), nullptr,
                                                   khsk_t.out))) {
                    *khsk_t.out = nullptr;
                }
                if (*khsk_t.out && !g_res.layout_inst_sk) {
                    const D3D11_INPUT_ELEMENT_DESC khsk_el[] = {   // KhInstLane on slot 1 (VSInstSk).
                        { "TEXCOORD", 4, DXGI_FORMAT_R32_UINT,        1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                        { "TEXCOORD", 5, DXGI_FORMAT_R32G32B32_FLOAT, 1, 4, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
                    };
                    if (FAILED(dev->CreateInputLayout(khsk_el, 2, khsk_b->GetBufferPointer(), khsk_b->GetBufferSize(),
                                                      &g_res.layout_inst_sk))) {
                        g_res.layout_inst_sk = nullptr;
                    }
                }
            } else if (!khsk_err.empty()) {
                report_error_once_safe(std::string("KH skin pool: twin ") + khsk_t.ent +
                                       " - that route draws skeletal meshes one by one: " + khsk_err);
            }
            if (khsk_b) khsk_b->Release();
        }
    }

    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(KH_CBOBJ_BYTES);   // b0 carries the object block only.
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.constant_buffer);
        if (FAILED(hr)) { g_res.release(); return "create CB " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.composite_cb);
        if (FAILED(hr)) { g_res.release(); return "create composite CB " + hr_str(hr); }
        bd.ByteWidth = static_cast<UINT>(KH_CBFRAME_BYTES);
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.frame_cb);
        if (FAILED(hr)) { g_res.release(); return "create frame CB " + hr_str(hr); }
        hr = dev->CreateBuffer(&bd, nullptr, &g_res.composite_frame_cb);
        if (FAILED(hr)) { g_res.release(); return "create composite frame CB " + hr_str(hr); }
        // KH_CB_RING: non-fatal.
        {
            D3D11_FEATURE_DATA_D3D11_OPTIONS khcr_opt = {};
            const bool khcr_have = SUCCEEDED(dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &khcr_opt, sizeof(khcr_opt)));
            g_cb_offsetting = khcr_have && khcr_opt.ConstantBufferOffsetting;
            g_cbr_supported = khcr_have &&
                khcr_opt.ConstantBufferOffsetting && khcr_opt.MapNoOverwriteOnDynamicConstantBuffer;

            if (g_cbr_supported) {
                D3D11_BUFFER_DESC khcr_bd = {};
                khcr_bd.ByteWidth = KH_CBR_STRIDE * KH_CBR_SLICES;
                khcr_bd.Usage = D3D11_USAGE_DYNAMIC;
                khcr_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                khcr_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                for (int khcr_i = 0; khcr_i < 2; ++khcr_i) {
                    g_res.cb_ring_cursor[khcr_i] = 0;
                    g_res.cb_ring_fresh[khcr_i] = true;
                    const HRESULT khcr_hr = dev->CreateBuffer(&khcr_bd, nullptr, &g_res.cb_ring[khcr_i]);

                    if (FAILED(khcr_hr)) {
                        for (int khcr_j = 0; khcr_j < 2; ++khcr_j) KH_SAFE_RELEASE(g_res.cb_ring[khcr_j]);
                        g_cbr_supported = false;
                        report_error_once_safe("KH cb ring: create " + hr_str(khcr_hr) + " (per-object uploads)");
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
        // KH_SSAO_MARK: the marking twin (LESS_EQUAL + write, stencil ALWAYS / REPLACE on pass, KEEP otherwise) and
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
        // KH_SSAO_SEE_THROUGH: the eraser compares bit 0 alone (ref 1), so it clears a see-through draw's mark (3)
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

        {   // KH_DLS_WORLD: dest.rgb *= src.rgb, RGB write mask only.
            D3D11_BLEND_DESC khwb = {};
            khwb.RenderTarget[0].BlendEnable = TRUE;
            khwb.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
            khwb.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
            khwb.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            khwb.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            khwb.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            khwb.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            khwb.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (FAILED(dev->CreateBlendState(&khwb, &g_res.dls_world_blend))) {
                g_res.dls_world_blend = nullptr;
            }
        }

        {   // KH_DLSW_FOG: dest.rgb = src0.rgb + dest.rgb * src1.rgb (dual
            // source), RGB write mask only; absent = the plain multiply.
            D3D11_BLEND_DESC khwf = {};
            khwf.RenderTarget[0].BlendEnable = TRUE;
            khwf.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            khwf.RenderTarget[0].DestBlend = D3D11_BLEND_SRC1_COLOR;
            khwf.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            khwf.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            khwf.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            khwf.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            khwf.RenderTarget[0].RenderTargetWriteMask =
                D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN |
                D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (FAILED(dev->CreateBlendState(&khwf, &g_res.dls_world_fog_blend))) {
                g_res.dls_world_fog_blend = nullptr;
            }
        }

        {   // KH_DLSW_MASK: dest = min(src, dest). The minimum of a set of distances is the nearest one, so this
            // blend is the mask's depth test and no depth-stencil target is needed.
            D3D11_BLEND_DESC khmb = {};
            khmb.RenderTarget[0].BlendEnable = TRUE;
            khmb.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            khmb.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            khmb.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MIN;
            khmb.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            khmb.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            khmb.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MIN;
            khmb.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
            if (FAILED(dev->CreateBlendState(&khmb, &g_res.dlsw_min_blend))) {
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

            ID3DBlob* khz_blob = nullptr;
            const std::string khz_err = compile_shader(KH_PS_ALPHA_ZERO_SRC,   // KH_PREWARM_ALL.
                "main", "ps_5_0", nullptr, &khz_blob);
            if (!khz_err.empty()) { g_res.release(); return "compile PS(alphaZero): " + khz_err; }
            hr = dev->CreatePixelShader(khz_blob->GetBufferPointer(), khz_blob->GetBufferSize(),
                                        nullptr, &g_res.ps_alpha_zero);
            khz_blob->Release();
            if (FAILED(hr)) { g_res.release(); return "create PS(alphaZero) " + hr_str(hr); }

            ID3DBlob* khpm_blob = nullptr;
            const std::string khpm_err = compile_shader(KH_PS_PREMULT_SRC,   // KH_PREWARM_ALL.
                "main", "ps_5_0", nullptr, &khpm_blob);
            if (!khpm_err.empty()) { g_res.release(); return "compile PS(premult): " + khpm_err; }
            hr = dev->CreatePixelShader(khpm_blob->GetBufferPointer(), khpm_blob->GetBufferSize(),
                                        nullptr, &g_res.ps_premult_copy);
            khpm_blob->Release();
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
        // winding, so CULL_BACK drops exactly the interior-facing triangles.
        rd.CullMode = D3D11_CULL_BACK;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer_cull);
        if (FAILED(hr)) { g_res.release(); return "create cull rasterizer " + hr_str(hr); }
        rd.CullMode = D3D11_CULL_FRONT;
        hr = dev->CreateRasterizerState(&rd, &g_res.rasterizer_front);
        if (FAILED(hr)) { g_res.release(); return "create front rasterizer " + hr_str(hr); }
        // Sun-depth rasterizers. All stay CullNone regardless of twoSided: an open mesh must still cast the full
        // closed-surface shadow.
        rd.CullMode = D3D11_CULL_NONE;
        // KH_SUN_RS_TEXT: the sun ladder binds no state minted here but the lo pair - kh_sun_rs_pick prefers
        // rast_sun_lo (8 / 1.0; created below on this same path, fatal if it fails), the tiers its DepthClip-off
        // twin, the union the clip form. This base state (16 / 3.0) serves the prefilter's fullscreen draw,
        // kh_dls_frame's placeholder bind (the DLS states supersede it), kh_svs_prime_mask and the volume seam's
        // fallback for a null rast_inject. The ladder's compare bias is the metric one in the shader (sunMeta.z);
        // its hardware bias is the lo pair's.
        rd.DepthBias = 16;
        rd.SlopeScaledDepthBias = 3.0f;
        rd.DepthBiasClamp = 0.0f;   // No bias clamp on any sun state; the twins below differ in DepthClipEnable.
        rd.DepthClipEnable = TRUE;
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_sun);
        if (FAILED(hr)) { g_res.release(); return "create sun rasterizer " + hr_str(hr); }
        // Each sun state's clamp twin is minted from the same desc with DepthClipEnable false, so the pair cannot
        // drift. Non-fatal: a null twin falls through to clip at kh_sun_rs_pick.
        auto khpk_twin = [&](ID3D11RasterizerState** khpk_out, const char* khpk_what) {
            rd.DepthClipEnable = FALSE;
            if (FAILED(dev->CreateRasterizerState(&rd, khpk_out))) {
                *khpk_out = nullptr;
                report_error_once_safe(std::string("KH sun shadow: pancake rasterizer (") + khpk_what +
                                       ") could not be created (that tier state clips)");
            }
            rd.DepthClipEnable = TRUE;
        };
        khpk_twin(&g_res.rast_sun_pk, "base");
        rd.DepthBias = 8;
        rd.SlopeScaledDepthBias = 1.0f;
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_sun_lo);
        if (FAILED(hr)) { g_res.release(); return "create sun rasterizer(lo) " + hr_str(hr); }
        khpk_twin(&g_res.rast_sun_lo_pk, "lo");

        rd.CullMode = D3D11_CULL_NONE;
        rd.DepthBias = 0;
        rd.SlopeScaledDepthBias = 0.0f;
        rd.DepthBiasClamp = 0.0f;

        rd.DepthClipEnable = FALSE;
        // TWIN (KH_SEAM_FACE): this desc is rasterizer's, field for field; the volume seam binds rasterizer_cull
        // (rasterizer's CULL_BACK twin) as this state's twin for a single-sided caster - change them together.
        hr = dev->CreateRasterizerState(&rd, &g_res.rast_inject);
        if (FAILED(hr)) { g_res.release(); return "create inject rasterizer " + hr_str(hr); }
    }

    // The placeholder pair is ensured on the success path too, not only from the two deferral returns.
    kh_white_ensure(dev);
    g_res.initialized = true;
    return "";
}
static int   g_khsg_w = 0;   // Exact scaled dims (viewport + bounds truth).
static int   g_khsg_h = 0;

// Creates (or recreates on a size, format or scale change) the five single-sample chain targets (the SSGI group
// among them at its own scale, the radiance seed). Called after ensure_scene_capture, which establishes dimensions.
inline std::string ensure_fx_chain(ID3D11Device* dev) {
    if (!g_res.scene_tex) return "no scene capture";

    D3D11_TEXTURE2D_DESC sd = {};
    g_res.scene_tex->GetDesc(&sd);

    const float khsg_g = g_khsg_scale.load(std::memory_order_relaxed);   // KH_XTHREAD_ATOMIC: read once.
    float khsg_s = !(khsg_g >= 0.25f) ? 0.25f : (khsg_g > 2.0f ? 2.0f : khsg_g);
    // KH_SSGI_CAP: the group may not pass D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION on either side (its create failed
    // and the whole chain dropped, every frame). One factor capped for both sides keeps the group's aspect, so the
    // grid factor the chain derives from g_khsg_w (scene_w / g_khsg_w) holds on both axes.
    const float khsg_cap = static_cast<float>(D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
    if (static_cast<float>(sd.Width) * khsg_s > khsg_cap) khsg_s = khsg_cap / static_cast<float>(sd.Width);
    if (static_cast<float>(sd.Height) * khsg_s > khsg_cap) khsg_s = khsg_cap / static_cast<float>(sd.Height);
    UINT khsg_sw = (UINT)(sd.Width  * khsg_s + 0.5f) > 0 ? (UINT)(sd.Width  * khsg_s + 0.5f) : 1;
    UINT khsg_sh = (UINT)(sd.Height * khsg_s + 0.5f) > 0 ? (UINT)(sd.Height * khsg_s + 0.5f) : 1;
    const UINT khsg_max = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    if (khsg_sw > khsg_max) khsg_sw = khsg_max;   // Float rounding at the cap.
    if (khsg_sh > khsg_max) khsg_sh = khsg_max;

    if (g_res.chain_tex[0] && g_res.chain_tex[2] && g_res.chain_tex[3] && g_res.chain_tex[4]) {
        D3D11_TEXTURE2D_DESC cd = {};
        D3D11_TEXTURE2D_DESC gd = {};
        g_res.chain_tex[0]->GetDesc(&cd);
        g_res.chain_tex[2]->GetDesc(&gd);   // Scale change recreates the group.
        if (cd.Width == sd.Width && cd.Height == sd.Height && cd.Format == sd.Format &&
            gd.Width == khsg_sw && gd.Height == khsg_sh) return "";
        g_res.release_fx_chain();
    }

    for (int i = 0; i < 5; ++i) {   // Gather; + pre-smooth; + radiance seed.
        D3D11_TEXTURE2D_DESC td = sd;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.SampleDesc.Count = 1;
        td.SampleDesc.Quality = 0;
        // [3] is [2]'s a-trous sibling: same half grid, same fp16. [4], the radiance seed the gather's taps read, is
        // one level (KH_SSGI_SEED: the gather reads the base alone - KH_SSGI_VB - so no pyramid is built).
        if (i >= 2) {
            td.Width  = khsg_sw;
            td.Height = khsg_sh;
            td.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
        HRESULT hr = dev->CreateTexture2D(&td, nullptr, &g_res.chain_tex[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain tex " + hr_str(hr); }
        hr = dev->CreateRenderTargetView(g_res.chain_tex[i], nullptr, &g_res.chain_rtv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain RTV " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.chain_tex[i], nullptr, &g_res.chain_srv[i]);
        if (FAILED(hr)) { g_res.release_fx_chain(); return "create chain SRV " + hr_str(hr); }
    }

    g_khsg_w = (int)khsg_sw;
    g_khsg_h = (int)khsg_sh;

    if (!g_res.khsg_sampler) {   // The ssgi chain's bilinear sampler (seed, gather taps, resolve).
        D3D11_SAMPLER_DESC khsm = {};
        khsm.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        khsm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        khsm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        khsm.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        khsm.MaxLOD = D3D11_FLOAT32_MAX;
        HRESULT khsm_hr = dev->CreateSamplerState(&khsm, &g_res.khsg_sampler);
        if (FAILED(khsm_hr)) { g_res.release_fx_chain(); return "create ssgi sampler " + hr_str(khsm_hr); }
    }

    return "";
}

// Samples the engine depth buffer, whose MSAA count follows video settings: compiled per count.
inline std::string ensure_effect_shader(ID3D11Device* dev) {
    if (g_res.ps_effect && g_res.ps_effect_samples == g_res.depth_sample_count) return "";
    if (g_res.ps_effect) { g_res.ps_effect->Release(); g_res.ps_effect = nullptr; }
    KH_SAFE_RELEASE(g_res.ps_effect_lean);   // KH_FX_LEAN_PASS: goes and comes with it.
    // KH_FX_SPLIT / KH_FX_LINZ: the unit's chain-side entry points go and come with it (a sample-count change), so
    // none outlives the MSAA_DEPTH it was compiled for. KH_FS_SPLIT: fog scatter's likewise.
    ID3D11PixelShader** const khes_ps[6] = { &g_res.ps_fx_linz, &g_res.ps_ssgi_gather, &g_res.ps_ssgi_atrous,
                                             &g_res.ps_dlf_gather, &g_res.ps_dlf_filter, &g_res.ps_fs_split };
    KH_SAFE_RELEASE(g_res.ps_wsee_agree);   // KH_WSEE: likewise.
    KH_SAFE_RELEASE(g_res.ps_fs_side);   // KH_WSEE_EDGE_COST: likewise.
    KH_SAFE_RELEASE(g_res.ps_fs_split_lean);   // KH_FX_LEAN: likewise.
    KH_SAFE_RELEASE(g_res.ps_dlf_comp);
    KH_SAFE_RELEASE(g_res.ps_dlf_gather_lean);   // KH_DLF_SPLIT: likewise.
    KH_SAFE_RELEASE(g_res.ps_dlf_class);
    for (ID3D11PixelShader** khes_p : khes_ps) KH_SAFE_RELEASE(*khes_p);

    const D3D_SHADER_MACRO defines[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { "KH_FX_UNIT", "1" },   // cb.hlsl's effect section (the prewarm tables' twin).
        { nullptr, nullptr }
    };

    const std::string fx_src = kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_EFFECT).c_str();
    ID3DBlob* blob = nullptr;
    std::string err = compile_shader(fx_src.c_str(), "PSEffect", "ps_5_0", defines, &blob);
    if (!err.empty()) return err;
    HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_effect);
    blob->Release();
    if (FAILED(hr)) return "CreatePixelShader(effect) " + hr_str(hr);
    // KH_FX_SPLIT / KH_FX_LINZ (the prewarm tables' twins): non-fatal - absent, the pass that draws one stands down
    // (SSGI skipped at the chain's loop top, dynamicLightFog unarmed, the depth read in place), reported once.
    // KH_FS_SPLIT: absent, fog scatter draws PSEffect's branch.
    static const char* const khes_ent[6] = { "PSFxLinZ", "PSSsgiGather", "PSSsgiAtrous", "PSDlfGather", "PSDlfFilter",
                                             "PSFsSplitS" };
    for (int khes_k = 0; khes_k < 6; ++khes_k) {
        kh_ps_optional(dev, fx_src, khes_ent[khes_k], defines, khes_ps[khes_k], "KH effect split shader: ");
    }
    // KH_WSEE_EDGE_COST: fog scatter's side value (non-fatal, the prewarm tables' twin) - absent, kh_fx_side_draw
    // draws side 28 with PSEffect's branch.
    kh_ps_optional(dev, fx_src, "PSFsSide", defines, &g_res.ps_fs_side, "KH effect split shader: ");
    // KH_FX_LEAN: fog scatter's entry and dynamicLightFog's composite without the record maths (non-fatal, the
    // prewarm tables' twins: their KH_FX_LEAN jobs) - absent, the full build or PSEffect's branch.
    const D3D_SHADER_MACRO khes_lean[] = {
        { "MSAA_DEPTH", g_res.depth_sample_count > 1 ? "1" : "0" },
        { "KH_FX_UNIT", "1" },
        { "KH_FX_LEAN", "1" },
        { nullptr, nullptr }
    };
    kh_ps_optional(dev, fx_src, "PSFsSplitS", khes_lean, &g_res.ps_fs_split_lean, "KH effect split shader: ");
    kh_ps_optional(dev, fx_src, "PSDlfComp", khes_lean, &g_res.ps_dlf_comp, "KH effect split shader: ");
    // KH_FX_LEAN_PASS: PSEffect itself without the record code (non-fatal, the prewarm tables' twin).
    kh_ps_optional(dev, fx_src, "PSEffect", khes_lean, &g_res.ps_effect_lean, "KH effect split shader: ");
    // KH_DLF_SPLIT: the gather's lean build and the texel marker (non-fatal, the prewarm tables' twins; the marker
    // with the system built in alone - only an armed pass draws it). Without the lean build the full gather draws
    // alone; with it and without the marker or its stencil, an unarmed pass draws the lean build, an armed one the
    // full.
    kh_ps_optional(dev, fx_src, "PSDlfGather", khes_lean, &g_res.ps_dlf_gather_lean, "KH effect split shader: ");
    if constexpr (KH_WSEE_ON) {
        kh_ps_optional(dev, fx_src, "PSDlfClass", defines, &g_res.ps_dlf_class, "KH effect split shader: ");
    }
    // KH_WSEE: the run's agreement mask (kh_wsee_agree_draw; non-fatal, the prewarm tables' twin) - multisampled
    // only, as the capture is (kh_wsee_ensure); not made with the system built out (no pass is armed). Absent, no
    // mask is drawn and the armed passes test inline.
    if constexpr (KH_WSEE_ON) {
        if (g_res.depth_sample_count > 1) {
            kh_ps_optional(dev, fx_src, "PSWseeAgree", defines, &g_res.ps_wsee_agree, "KH world see-through: ");
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
    KH_SAFE_RELEASE(g_res.vs_comp_inst_sk);   // KH_SKIN_POOL.
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

    const D3D_SHADER_MACRO khtx_defines[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" },
        { nullptr, nullptr },
    };
    const D3D_SHADER_MACRO khtx_defines_arb[] = {
        { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
        { "SAMPLE_COUNT", sc.c_str() },
        { "KH_ARB_DEPTH", "1" },
        { "KH_RECEIVE_TEX", "1" },
        { "KH_TEXTURED", "1" },
        { nullptr, nullptr },
    };

    const std::string comp_src = kh_hlsl_src(KH_HLSL_CB) + kh_hlsl_src(KH_HLSL_COMPOSITE).c_str();
    // The composite unit's ten heavy compiles as one batch. Declared after comp_src and every table it names so the
    // scope joins and frees before they go out of scope; covers the comp_fail early returns.
    const KhShaderJob khsm_jobs[] = {
        { comp_src.c_str(), "PSComposite", "ps_5_0", defines,          0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", defines_arb,      0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", khtx_defines,     0 },
        { comp_src.c_str(), "PSComposite", "ps_5_0", khtx_defines_arb, 0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", defines,          0 },
        { comp_src.c_str(), "VSComposite", "vs_5_0", khtx_defines,     0 },
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", defines,      0 },   // KH_INSTANCING twins.
        { comp_src.c_str(), "VSCompositeInst", "vs_5_0", khtx_defines, 0 },
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", defines,      0 },   // KH_SKIN_POOL twins.
        { comp_src.c_str(), "VSCompositeInstSk", "vs_5_0", khtx_defines, 0 },
    };
    KhShaderMtScope khsm_scope(khsm_jobs, _countof(khsm_jobs));
    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    auto comp_fail = [&](const std::string& e) {
        g_comp_last_err = e;
        if (g_comp_fail_streak == 0) report_error_once_safe("KH composite shader: " + e);
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
    if (SUCCEEDED(hr)) {   // KH_PSV: the tail's builtin (D3).
        kh_psv_own(2, ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), g_res.ps_composite);
    }
    ps_blob->Release();
    if (FAILED(hr)) return comp_fail("create composite PS " + hr_str(hr));
    // Arb variant: same source, KH_ARB_DEPTH=1 switches the SV_Depth output signature + the analytic clamp block on.
    ID3DBlob* psa_blob = nullptr;
    err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", defines_arb, &psa_blob);
    if (!err.empty()) return comp_fail("composite PS(arb): " + err);
    hr = dev->CreatePixelShader(psa_blob->GetBufferPointer(), psa_blob->GetBufferSize(), nullptr, &g_res.ps_composite_arb);
    if (SUCCEEDED(hr)) {   // KH_PSV: the tail's builtin (D3).
        kh_psv_own(3, psa_blob->GetBufferPointer(), psa_blob->GetBufferSize(), g_res.ps_composite_arb);
    }
    psa_blob->Release();
    if (FAILED(hr)) return comp_fail("create composite PS(arb) " + hr_str(hr));
    // The twins are released with the base three at the top.
    {
        // The textured twins (non-fatal): VS, PS and the arb PS. khtx_defines / khtx_defines_arb: the function's top
        // (the KH_SHADER_MT batch names them too - one table, so the twins take the batch's blobs).
        ID3DBlob* khtx_vsb = nullptr;
        ID3DBlob* khtx_psb = nullptr;
        ID3DBlob* khtx_pab = nullptr;
        std::string khtx_err = compile_shader(comp_src.c_str(), "VSComposite", "vs_5_0", khtx_defines, &khtx_vsb);
        if (khtx_err.empty()) khtx_err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", khtx_defines, &khtx_psb);
        if (khtx_err.empty()) khtx_err = compile_shader(comp_src.c_str(), "PSComposite", "ps_5_0", khtx_defines_arb, &khtx_pab);

        if (khtx_err.empty()) {
            HRESULT khtx_hr = dev->CreateVertexShader(khtx_vsb->GetBufferPointer(), khtx_vsb->GetBufferSize(), nullptr, &g_res.vs_comp_tex);
            if (SUCCEEDED(khtx_hr)) khtx_hr = dev->CreatePixelShader(khtx_psb->GetBufferPointer(), khtx_psb->GetBufferSize(), nullptr, &g_res.ps_comp_tex);
            if (SUCCEEDED(khtx_hr)) khtx_hr = dev->CreatePixelShader(khtx_pab->GetBufferPointer(), khtx_pab->GetBufferSize(), nullptr, &g_res.ps_comp_arb_tex);
            if (SUCCEEDED(khtx_hr)) {   // KH_PSV: the tail's builtins (D3).
                kh_psv_own(4, khtx_psb->GetBufferPointer(), khtx_psb->GetBufferSize(), g_res.ps_comp_tex);
                kh_psv_own(5, khtx_pab->GetBufferPointer(), khtx_pab->GetBufferSize(), g_res.ps_comp_arb_tex);
            }

            if (FAILED(khtx_hr)) {
                KH_SAFE_RELEASE(g_res.vs_comp_tex);
                KH_SAFE_RELEASE(g_res.ps_comp_tex);
                KH_SAFE_RELEASE(g_res.ps_comp_arb_tex);
                report_error_once_safe("KH textured composite: create " + hr_str(khtx_hr));
            }
        } else {
            report_error_once_safe("KH textured composite: " + khtx_err);
        }

        if (khtx_vsb) khtx_vsb->Release();
        if (khtx_psb) khtx_psb->Release();
        if (khtx_pab) khtx_pab->Release();
    }
    // The injection's instanced VS pair (non-fatal); layouts are the static unit's (identical input signatures). A
    // missing twin stands the injection's bucketing down whole (khr_inst_on gates on both) - never a bucket whose
    // members were marked done with no twin to draw them.
    {
        ID3DBlob* khin_b = nullptr;
        std::string khin_err = compile_shader(comp_src.c_str(), "VSCompositeInst", "vs_5_0", defines, &khin_b);
        if (khin_err.empty() && khin_b) {
            if (FAILED(dev->CreateVertexShader(khin_b->GetBufferPointer(), khin_b->GetBufferSize(), nullptr, &g_res.vs_comp_inst)))
                KH_SAFE_RELEASE(g_res.vs_comp_inst);
            khin_b->Release();
            khin_b = nullptr;
        } else if (!khin_err.empty()) {
            report_error_once_safe("KH instanced composite: " + khin_err);
        }

        khin_err = compile_shader(comp_src.c_str(), "VSCompositeInst", "vs_5_0", khtx_defines, &khin_b);
        if (khin_err.empty() && khin_b) {
            if (FAILED(dev->CreateVertexShader(khin_b->GetBufferPointer(), khin_b->GetBufferSize(), nullptr, &g_res.vs_comp_inst_tex)))
                KH_SAFE_RELEASE(g_res.vs_comp_inst_tex);
            khin_b->Release();
        } else if (!khin_err.empty()) {
            report_error_once_safe("KH instanced composite (textured): " + khin_err);
        }
    }
    // KH_SKIN_POOL: VSCompositeInstSk, plain and textured (non-fatal: a missing one keeps skeletal bindings out of
    // every bucket, kh_inst_sk_on). Layout: the static unit's layout_inst_sk (identical input signatures).
    {
        const D3D_SHADER_MACRO* const khck_defs[2] = { defines, khtx_defines };
        ID3D11VertexShader** const khck_out[2] = { &g_res.vs_comp_inst_sk, &g_res.vs_comp_inst_sk_tex };
        for (int khck_i = 0; khck_i < 2; ++khck_i) {
            ID3DBlob* khck_b = nullptr;
            const std::string khck_err = compile_shader(comp_src.c_str(), "VSCompositeInstSk", "vs_5_0",
                                                        khck_defs[khck_i], &khck_b);
            if (khck_err.empty() && khck_b) {
                if (FAILED(dev->CreateVertexShader(khck_b->GetBufferPointer(), khck_b->GetBufferSize(), nullptr,
                                                   khck_out[khck_i]))) {
                    *khck_out[khck_i] = nullptr;
                }
            } else if (!khck_err.empty()) {
                report_error_once_safe("KH skin pool: twin VSCompositeInstSk - skeletal meshes draw one by one: " +
                                       khck_err);
            }
            if (khck_b) khck_b->Release();
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
    std::string err = compile_shader(kh_hlsl_src(KH_HLSL_DEPTH_RESOLVE).c_str(), "PSDepthResolve", "ps_5_0", defines, &blob);
    if (!err.empty()) return "depth resolve PS: " + err;
    HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &g_res.ps_depth_resolve);
    blob->Release();
    if (FAILED(hr)) return "create depth resolve PS " + hr_str(hr);
    g_res.ps_resolve_samples = g_res.depth_sample_count;
    return "";
}
}