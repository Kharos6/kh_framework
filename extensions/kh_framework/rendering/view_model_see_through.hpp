#pragma once

// The injection half (rendering_integration_injection.hpp): KH_VM_SEE, the view-model see-through. Never include it on
// its own.
namespace RenderIntegration {

// KH_VM_SEE - without it the scene chain past the view model (KH_FX_LATE) reads, behind a sight's translucent planes,
// the depth those planes wrote: a 3D scope's eyepiece (the last of its planes) writes a near depth over its whole
// aperture, so fog, fog scatter, dynamicLightFog and SSGI measure a wall a few centimetres away and draw nothing
// there. The scene flush that leaves a depth-reading chain to a later resolve arms the cycle (kh_vmsee_arm); at the
// view model's first translucent draw the draw hook snapshots the main depth (sample 0, raw: the world and the view
// model's opaque parts, all drawn by its depth prepass by then) and clears the transmittance to 1
// (kh_vmsee_snapshot); each of its translucent draws that blends over with 1 - alpha is drawn again into the
// transmittance (kh_vmsee_pre_draw marks it, the draw hooks re-issue it: kh_vmsee_t_begin / kh_vmsee_t_end). The
// chain binds both (t34, t41) and arms its depth readers (shadowMeta2.w; cb.hlsl's KH_VM_SEE). KH_INFRONT: our own
// inFront parts that write no depth are drawn after the engine's planes (the slice's deferred tail); their
// normal-blend draws go into the transmittance the same way (kh_infront_vmsee_take, which also takes the snapshot when
// the engine's planes took none).
// Not covered: FSAA 1x (the chain runs before the view model), a PIP's sight, the UI lane, a plane drawn indirectly,
// and planes that overlap out of their draw order (no depth test into the transmittance). Render thread: the want
// stamp (the scene flush) and the pending flag (the draw hooks). The snapshot stamp: the draw hook's, or the deferred
// tail's (kh_infront_vmsee_take), which the scene resolve may host on the game thread - kept apart from the render
// thread's writes only because the engine never drives the immediate context from two threads at once. Any thread:
// the landed stamp, read by the chain (a game-thread chain-only flush, kh_fx_late_gt) against the published cycle.
static uint64_t g_vmsee_want_cycle = ~0ull;   // The cycle the scene flush armed (~0 = none).
static uint64_t g_vmsee_snap_cycle = ~0ull;   // The cycle whose snapshot was attempted - once, landed or not.
static std::atomic<uint64_t> g_vmsee_ok_cycle{ ~0ull };   // The cycle whose snapshot and clear landed.
static bool     g_vmsee_pending = false;      // This draw's transmittance re-issue (the pre-draw to its hook's post).
// The re-issue's saves, null outside one kh_vmsee_t_begin / kh_vmsee_t_end pair (a draw hook's post, render thread;
// or the slice's deferred tail, which the scene resolve may host on the game thread).
static ID3D11RenderTargetView* g_vmsee_sv_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
static ID3D11DepthStencilView* g_vmsee_sv_dsv = nullptr;
static ID3D11BlendState*       g_vmsee_sv_bs = nullptr;
static FLOAT                   g_vmsee_sv_bf[4] = {};
static UINT                    g_vmsee_sv_mask = 0xFFFFFFFFu;

// The chain's arm: this cycle's snapshot landed and its targets stand.
inline bool kh_vmsee_valid() {
    return g_vmsee_ok_cycle.load(std::memory_order_relaxed) == g_topo_cycles_pub.load(std::memory_order_relaxed) &&
           g_res.vmsee_s_srv != nullptr && g_res.vmsee_t_srv != nullptr;
}

// The scene flush's KH_FX_LATE take (render thread): a pass the chain will run that reads the depth (its skips -
// effect, opacity, a dynamicLightFog with nothing to add, an SSGI without its entry points - and KH_NEARZ_READER's
// rule; a user effect by its file, KH_VMSEE_USER_SCAN) arms the cycle, once its shader and blend stand
// (kh_vmsee_ensure; the targets are the snapshot's to make - KH_VMSEE_LAZY).
inline void kh_vmsee_arm(ID3D11Device* dev, const std::vector<std::pair<uint64_t, RenderObject>>& khva_fs) {
    bool khva_dem = false;
    for (const auto& khva_f : khva_fs) {
        const RenderObject& khva_o = khva_f.second;
        if (khva_o.effect <= 0 || khva_o.color[3] <= 0.001f || !kh_fx_needs_depth(khva_o)) continue;
        if (khva_o.effect == static_cast<int>(EffectId::DynLightFog) &&
            (khva_o.blend_mode == 0 || khva_o.blend_mode == 4 || khva_o.blend_mode == 5) &&
            (!(khva_o.fx[0] > 0.0f) || !kh_dlf_pool_live())) continue;
        if (khva_o.effect == static_cast<int>(EffectId::Ssgi) && (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) {
            continue;
        }
        // KH_VMSEE_USER_SCAN: a user effect counts when the chain may draw it and its file reads the depth (a colour
        // filter does not; nor one whose compile failed); localized / banded, its mask does.
        if (khva_o.effect == KH_EFFECT_CUSTOM && !khva_o.localized && !khva_o.banded &&
            !kh_user_fx_reads_depth(kh_fx_shader_of(khva_o))) continue;
        khva_dem = true;
        break;
    }
    if (khva_dem && kh_vmsee_ensure(dev)) g_vmsee_want_cycle = g_topo_cycles;
}

// The snapshot (snapshot_composite_depth's pattern): the main depth's sample 0 into the R32F target, the
// transmittance cleared to 1, everything handed back. Our binds and the restore run under in_injection, so the OM
// hooks do not take them for the engine's (the main-depth test, the blend / PIP / shadow trackers). The bound depth
// must be the texture depth_srv views (its identity) - the one the targets are sized by (kh_vmsee_targets, made or
// remade here: KH_VMSEE_LAZY); false = not this cycle.
inline bool kh_vmsee_snapshot(ID3D11DeviceContext* ctx) {
    if (!g_res.ps_vmsee_s0 || !g_res.depth_srv || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) {
        return false;
    }
    if (g_res.ps_vmsee_samples != g_res.depth_sample_count) return false;
    {
        ID3D11DepthStencilView* khvn_dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &khvn_dsv);
        if (!khvn_dsv) return false;
        const bool khvn_same = g_res.depth_res_identity != nullptr &&
                               reorder_dsv_identity(khvn_dsv) == g_res.depth_res_identity;
        khvn_dsv->Release();
        if (!khvn_same) return false;
    }
    {
        ID3D11Device* khvn_dev = nullptr;
        ctx->GetDevice(&khvn_dev);
        const bool khvn_tg = kh_vmsee_targets(khvn_dev);   // KH_VMSEE_LAZY.
        if (khvn_dev) khvn_dev->Release();
        if (!khvn_tg || !g_res.vmsee_s_rtv || !g_res.vmsee_t_rtv || !g_res.vmsee_s_srv || !g_res.vmsee_t_srv ||
            g_res.vmsee_w == 0 || g_res.vmsee_h == 0) return false;
    }
    KhInjectionFlagScope khvn_fl;   // Ahead of the saves: it unwinds after their restore.
    g_ro.in_injection = true;
    StateBackup khvn_bk;
    khvn_bk.capture(ctx);
    KhOmSave khvn_om;
    khvn_om.capture(ctx);
    D3D11_VIEWPORT khvn_vp = {};
    khvn_vp.Width = static_cast<FLOAT>(g_res.vmsee_w);
    khvn_vp.Height = static_cast<FLOAT>(g_res.vmsee_h);
    khvn_vp.MaxDepth = 1.0f;
    // Replacing the OM targets unbinds the main depth's view first, which makes its SRV legal at t0.
    ctx->OMSetRenderTargets(1, &g_res.vmsee_s_rtv, nullptr);
    ctx->RSSetViewports(1, &khvn_vp);
    ctx->RSSetState(g_res.rasterizer);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_vmsee_s0, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &g_res.depth_srv);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT khvn_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(nullptr, khvn_bf, 0xFFFFFFFF);
    ctx->Draw(3, 0);
    ID3D11ShaderResourceView* khvn_null = nullptr;
    ctx->PSSetShaderResources(0, 1, &khvn_null);   // Off t0 before the engine's depth view is bound again.
    const FLOAT khvn_one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(g_res.vmsee_t_rtv, khvn_one);
    khvn_om.restore(ctx);
    khvn_om.release();
    khvn_bk.restore(ctx);
    return true;
}

// reorder_pre_draw's call, armed cycles only: a tracked translucent draw on the main depth. Inside the view model's
// slice with the scene target bound (KH_SCENE_NAME's texture - its depth prepass binds none, and a draw into another
// target is not the picture), the cycle's first takes the snapshot; each, once it landed, whose blend leaves the
// scene behind it times 1 - alpha - premultiplied (ONE), alpha (SRC_ALPHA) or a darkening plane (ZERO) over
// INV_SRC_ALPHA, writing colour - is marked for the re-issue. Additive and other modes leave the transmittance
// alone. With no scene texture named yet, nothing is (the chain reads the live depth). Alpha-to-coverage is not
// tested here (KH_WSEE refuses it): an A2C plane's re-issue (bs_vmsee_t, no A2C) records 1 - a over its whole
// raster coverage, more opacity than the samples its alpha covered.
inline void kh_vmsee_pre_draw(ID3D11DeviceContext* ctx) {
    UINT khvp_n = 1;
    D3D11_VIEWPORT khvp_vp = {};
    ctx->RSGetViewports(&khvp_n, &khvp_vp);
    if (khvp_n < 1 || !kh_infront_vp_is_slice(khvp_vp)) return;
    ID3D11RenderTargetView* khvp_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khvp_rtv, nullptr);
    if (!khvp_rtv) return;
    ID3D11Resource* khvp_res = nullptr;
    khvp_rtv->GetResource(&khvp_res);
    khvp_rtv->Release();
    const bool khvp_scene = khvp_res != nullptr && g_topo_scene_tex_id != nullptr &&
                            static_cast<void*>(khvp_res) == g_topo_scene_tex_id;
    if (khvp_res) khvp_res->Release();
    if (!khvp_scene) return;
    if (g_vmsee_snap_cycle != g_topo_cycles) {
        g_vmsee_snap_cycle = g_topo_cycles;
        if (kh_vmsee_snapshot(ctx)) g_vmsee_ok_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (g_vmsee_ok_cycle.load(std::memory_order_relaxed) != g_topo_cycles) return;
    ID3D11BlendState* khvp_bs = nullptr;
    FLOAT khvp_bf[4] = {};
    UINT khvp_mask = 0;
    ctx->OMGetBlendState(&khvp_bs, khvp_bf, &khvp_mask);
    if (!khvp_bs) return;   // The default state: opaque.
    D3D11_BLEND_DESC khvp_bd = {};
    khvp_bs->GetDesc(&khvp_bd);
    khvp_bs->Release();
    const D3D11_RENDER_TARGET_BLEND_DESC& khvp_r = khvp_bd.RenderTarget[0];
    g_vmsee_pending = khvp_r.BlendEnable && khvp_r.BlendOp == D3D11_BLEND_OP_ADD &&
                      khvp_r.DestBlend == D3D11_BLEND_INV_SRC_ALPHA &&
                      (khvp_r.SrcBlend == D3D11_BLEND_ONE || khvp_r.SrcBlend == D3D11_BLEND_SRC_ALPHA ||
                       khvp_r.SrcBlend == D3D11_BLEND_ZERO) &&
                      (khvp_r.RenderTargetWriteMask & 7u) != 0 && khvp_mask != 0;
}

// KH_INFRONT: the slice's deferred tail (kh_infront_inject, khvi_late - the pass's scene RT0 and main depth bound,
// after the engine's slice draws) arms its re-issue into T: in an armed cycle whose snapshot the engine's planes
// never took (no translucent slice draw - an iron sight), it is taken now, the depth holding the world, the hands
// and our solids, what the engine-time one holds; else the landed one stands. True = this cycle's snapshot landed,
// and each normal-blend part is drawn again into T (kh_vmsee_t_begin / kh_vmsee_t_end), as the engine's planes are.
inline bool kh_infront_vmsee_take(ID3D11DeviceContext* khvt_ctx) {
    if (g_vmsee_want_cycle != g_topo_cycles) return false;
    if (g_vmsee_snap_cycle != g_topo_cycles) {
        g_vmsee_snap_cycle = g_topo_cycles;
        if (kh_vmsee_snapshot(khvt_ctx)) g_vmsee_ok_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    return g_vmsee_ok_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
}

inline void kh_vmsee_sv_release() {
    for (ID3D11RenderTargetView*& khvr_r : g_vmsee_sv_rtvs) KH_SAFE_RELEASE(khvr_r);
    KH_SAFE_RELEASE(g_vmsee_sv_dsv);
    KH_SAFE_RELEASE(g_vmsee_sv_bs);
}

// The re-issue's OM: the transmittance alone, no depth view - so no depth or stencil test, and the bound
// depth-stencil state and reference untouched - under bs_vmsee_t. The viewport, scissor, rasterizer, shaders,
// constants, views, input and predicate stay the caller's (the engine's at a draw hook's post). Refused with a UAV
// bound (kh_vmir_begin's rule: a plain OMSetRenderTargets would drop it) or a stream-output target (the draw again
// would append to it). Callers: the draw hooks' post blocks (render thread) and the slice's deferred tail
// (kh_infront_inject's late pass, after kh_infront_vmsee_take), which the scene resolve may host on the game thread.
inline bool kh_vmsee_t_begin(ID3D11DeviceContext* ctx) {
    if (!g_res.vmsee_t_rtv || !g_res.bs_vmsee_t) return false;
    ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmsee_sv_rtvs, &g_vmsee_sv_dsv);
    ID3D11UnorderedAccessView* khtb_uavs[8] = {};
    ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, khtb_uavs);   // UAV probe only.
    bool khtb_uav = false;
    for (ID3D11UnorderedAccessView*& khtb_u : khtb_uavs) {
        if (khtb_u) khtb_uav = true;
        KH_SAFE_RELEASE(khtb_u);
    }
    ID3D11Buffer* khtb_so = nullptr;
    ctx->SOGetTargets(1, &khtb_so);
    if (khtb_so) {
        khtb_uav = true;
        khtb_so->Release();
    }
    if (khtb_uav) {
        kh_vmsee_sv_release();
        return false;
    }
    ctx->OMGetBlendState(&g_vmsee_sv_bs, g_vmsee_sv_bf, &g_vmsee_sv_mask);
    KhInjectionFlagScope khtb_fl;
    g_ro.in_injection = true;
    ctx->OMSetRenderTargets(1, &g_res.vmsee_t_rtv, nullptr);
    const FLOAT khtb_bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(g_res.bs_vmsee_t, khtb_bf, 0xFFFFFFFF);
    return true;
}

inline void kh_vmsee_t_end(ID3D11DeviceContext* ctx) {
    {
        KhInjectionFlagScope khte_fl;   // The engine's own set back, which its trackers already saw.
        g_ro.in_injection = true;
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmsee_sv_rtvs, g_vmsee_sv_dsv);
        ctx->OMSetBlendState(g_vmsee_sv_bs, g_vmsee_sv_bf, g_vmsee_sv_mask);
    }
    kh_vmsee_sv_release();
}
}