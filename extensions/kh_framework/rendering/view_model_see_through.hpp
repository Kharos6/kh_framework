#pragma once

// The injection half (rendering_integration_injection.hpp): VM_SEE, the view-model see-through. Never include it on
// its own.
namespace RenderIntegration {

// VM_SEE: the want, snapshot and landed cycles, the pending flag, the re-issue's saved output-merger state.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_scratch_reset resets 4 of them.
struct VmseeState {
    // VM_SEE - without it the scene chain past the view model (FX_LATE) reads, behind a sight's translucent
    // planes, the depth those planes wrote: a 3D scope's eyepiece (the last of its planes) writes a near depth over its
    // whole aperture, so fog, fog scatter, dynamicLightFog and SSGI measure a wall a few centimetres away and draw
    // nothing there. The scene flush that leaves a depth-reading chain to a later resolve arms the cycle
    // (vmsee_arm); at the view model's first translucent draw the draw hook snapshots the main depth (sample 0, raw:
    // the world and the view model's opaque parts, all drawn by its depth prepass by then) and clears the transmittance
    // to 1 (vmsee_snapshot); each of its translucent draws that blends over with 1 - alpha is drawn again into the
    // transmittance (vmsee_pre_draw marks it, the draw hooks re-issue it: vmsee_t_begin / vmsee_t_end). The
    // chain binds both (t34, t41) and arms its depth readers (shadowMeta2.w; cb.hlsl's VM_SEE). INFRONT: our own
    // inFront parts that write no depth are drawn after the engine's planes (the slice's deferred tail); their
    // normal-blend draws go into the transmittance the same way (infront_vmsee_take, which also takes the snapshot
    // when the engine's planes took none). Not covered: FSAA 1x (the chain runs before the view model), a PIP's sight,
    // the UI lane, a plane drawn indirectly, and planes that overlap out of their draw order (no depth test into the
    // transmittance). Render thread: the want stamp (the scene flush) and the pending flag (the draw hooks). The
    // snapshot stamp: the draw hook's, or the deferred tail's (infront_vmsee_take), which the scene resolve may host
    // on the game thread - kept apart from the render thread's writes only because the engine never drives the
    // immediate context from two threads at once. Any thread: the landed stamp, read by the chain (a game-thread
    // chain-only flush, fx_late_gt) against the published cycle.
    uint64_t want_cycle = ~0ull;   // The cycle the scene flush armed (~0 = none).
    uint64_t snap_cycle = ~0ull;   // The cycle whose snapshot was attempted - once, landed or not.
    std::atomic<uint64_t> ok_cycle{ ~0ull };   // The cycle whose snapshot and clear landed.
    bool     pending = false;      // This draw's transmittance re-issue (the pre-draw to its hook's post).
    // The re-issue's saves, null outside one vmsee_t_begin / vmsee_t_end pair (a draw hook's post, render thread;
    // or the slice's deferred tail, which the scene resolve may host on the game thread).
    ID3D11RenderTargetView* sv_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* sv_dsv = nullptr;
    ID3D11BlendState*       sv_bs = nullptr;
    FLOAT                   sv_bf[4] = {};
    UINT                    sv_mask = 0xFFFFFFFFu;
};
static VmseeState g_vmsee;

// The chain's arm: this cycle's snapshot landed and its targets stand.
inline bool vmsee_valid() {
    return g_vmsee.ok_cycle.load(std::memory_order_relaxed) == g_topo_cycles_pub.load(std::memory_order_relaxed) &&
           g_res.vmsee_s_srv != nullptr && g_res.vmsee_t_srv != nullptr;
}

// The scene flush's FX_LATE take (render thread): a pass the chain will run that reads the depth (its skips -
// effect, opacity, a dynamicLightFog with nothing to add, an SSGI without its entry points - and NEARZ_READER's
// rule; a user effect by its file, VMSEE_USER_SCAN) arms the cycle, once its shader and blend stand
// (vmsee_ensure; the targets are the snapshot's to make - VMSEE_LAZY).
inline void vmsee_arm(ID3D11Device* dev, const std::vector<std::pair<uint64_t, RenderObject>>& fs) {
    bool dem = false;
    for (const auto& f : fs) {
        const RenderObject& o = f.second;
        if (o.effect <= 0 || o.color[3] <= 0.001f || !fx_needs_depth(o)) continue;
        if (o.effect == static_cast<int>(EffectId::DynLightFog) &&
            (o.blend_mode == 0 || o.blend_mode == 4 || o.blend_mode == 5) &&
            (!(o.fx[0] > 0.0f) || !dlf_pool_live())) continue;
        if (o.effect == static_cast<int>(EffectId::Ssgi) && (!g_res.ps_ssgi_gather || !g_res.ps_ssgi_atrous)) {
            continue;
        }
        // VMSEE_USER_SCAN: a user effect counts when the chain may draw it and its file reads the depth (a colour
        // filter does not; nor one whose compile failed); localized / banded, its mask does.
        if (o.effect == EFFECT_CUSTOM && !o.localized && !o.banded &&
            !user_fx_reads_depth(fx_shader_of(o))) continue;
        dem = true;
        break;
    }
    if (dem && vmsee_ensure(dev)) g_vmsee.want_cycle = g_topo_cycles;
}

// EA_SCENE_RTV: whether rtv (not null) views the scene texture (SCENE_NAME's, g_topo_scene_tex_id; none
// named yet = false). The pre-draws', the injection capture's and the in-front tail arm's RT0 test (here,
// rendering/see_through.hpp and rendering/in_front.hpp).
inline bool rtv_is_scene(ID3D11RenderTargetView* rtv) {
    if (!g_topo_scene_tex_id) return false;
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    const bool scene = res != nullptr && static_cast<void*>(res) == g_topo_scene_tex_id;
    if (res) res->Release();
    return scene;
}

// EA_S0_PASS: the main depth's sample 0, raw, into rtv (R32F, w x h) - PSDepthS0 in one fullscreen
// triangle: the snapshot below, WSEE's capture (S) and its B2 moment (S2). The caller has saved the state
// (StateBackup, OmSave) under in_injection and checked ps_vmsee_s0's sample count; the pass's state stays bound but
// t0, nulled.
inline void depth_s0_pass(ID3D11DeviceContext* ctx, ID3D11RenderTargetView* rtv, UINT w, UINT h) {
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<FLOAT>(w);
    vp.Height = static_cast<FLOAT>(h);
    vp.MaxDepth = 1.0f;
    // Replacing the OM targets unbinds the main depth's view first, which makes its SRV legal at t0.
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    ctx->RSSetViewports(1, &vp);
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
    const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(nullptr, bf, 0xFFFFFFFF);
    ctx->Draw(3, 0);
    ID3D11ShaderResourceView* null = nullptr;
    ctx->PSSetShaderResources(0, 1, &null);   // Off t0 before the engine's depth view is bound again.
}

// The snapshot (snapshot_composite_depth's pattern): the main depth's sample 0 into the R32F target, the
// transmittance cleared to 1, everything handed back. Our binds and the restore run under in_injection, so the OM
// hooks do not take them for the engine's (the main-depth test, the blend / PIP / shadow trackers). The bound depth
// must be the texture depth_srv views (its identity) - the one the targets are sized by (vmsee_targets, made or
// remade here: VMSEE_LAZY); false = not this cycle.
inline bool vmsee_snapshot(ID3D11DeviceContext* ctx) {
    if (!g_res.ps_vmsee_s0 || !g_res.depth_srv || !g_res.vs_fullscreen || !g_res.rasterizer || !g_res.dss_off) {
        return false;
    }
    if (g_res.ps_vmsee_samples != g_res.depth_sample_count) return false;
    {
        ID3D11DepthStencilView* dsv = nullptr;
        ctx->OMGetRenderTargets(0, nullptr, &dsv);
        if (!dsv) return false;
        const bool same = g_res.depth_res_identity != nullptr &&
                          reorder_dsv_identity(dsv) == g_res.depth_res_identity;
        dsv->Release();
        if (!same) return false;
    }
    {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        const bool tg = vmsee_targets(dev);   // VMSEE_LAZY.
        if (dev) dev->Release();
        if (!tg || !g_res.vmsee_s_rtv || !g_res.vmsee_t_rtv || !g_res.vmsee_s_srv || !g_res.vmsee_t_srv ||
            g_res.vmsee_w == 0 || g_res.vmsee_h == 0) return false;
    }
    InjectionFlagScope fl;   // Ahead of the saves: it unwinds after their restore.
    g_ro.in_injection = true;
    StateBackup bk;
    bk.capture(ctx);
    OmSave om;
    om.capture(ctx);
    depth_s0_pass(ctx, g_res.vmsee_s_rtv, g_res.vmsee_w, g_res.vmsee_h);   // EA_S0_PASS.
    const FLOAT one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->ClearRenderTargetView(g_res.vmsee_t_rtv, one);
    om.restore(ctx);
    om.release();
    bk.restore(ctx);
    return true;
}

// reorder_pre_draw's call, armed cycles only: a tracked translucent draw on the main depth. Inside the view model's
// slice with the scene target bound (SCENE_NAME's texture - its depth prepass binds none, and a draw into another
// target is not the picture), the cycle's first takes the snapshot; each, once it landed, whose blend leaves the
// scene behind it times 1 - alpha - premultiplied (ONE), alpha (SRC_ALPHA) or a darkening plane (ZERO) over
// INV_SRC_ALPHA, writing colour - is marked for the re-issue. Additive and other modes leave the transmittance
// alone. With no scene texture named yet, nothing is (the chain reads the live depth). Alpha-to-coverage is not
// tested here (WSEE refuses it): an A2C plane's re-issue (bs_vmsee_t, no A2C) records 1 - a over its whole
// raster coverage, more opacity than the samples its alpha covered.
inline void vmsee_pre_draw(ID3D11DeviceContext* ctx) {
    UINT n = 1;
    D3D11_VIEWPORT vp = {};
    ctx->RSGetViewports(&n, &vp);
    if (n < 1 || !infront_vp_is_slice(vp)) return;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) return;
    const bool scene = rtv_is_scene(rtv);   // EA_SCENE_RTV.
    rtv->Release();
    if (!scene) return;
    if (g_vmsee.snap_cycle != g_topo_cycles) {
        g_vmsee.snap_cycle = g_topo_cycles;
        if (vmsee_snapshot(ctx)) g_vmsee.ok_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    if (g_vmsee.ok_cycle.load(std::memory_order_relaxed) != g_topo_cycles) return;
    ID3D11BlendState* bs = nullptr;
    FLOAT bf[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&bs, bf, &mask);
    if (!bs) return;   // The default state: opaque.
    D3D11_BLEND_DESC bd = {};
    bs->GetDesc(&bd);
    bs->Release();
    const D3D11_RENDER_TARGET_BLEND_DESC& r = bd.RenderTarget[0];
    g_vmsee.pending = r.BlendEnable && r.BlendOp == D3D11_BLEND_OP_ADD &&
                      r.DestBlend == D3D11_BLEND_INV_SRC_ALPHA &&
                      (r.SrcBlend == D3D11_BLEND_ONE || r.SrcBlend == D3D11_BLEND_SRC_ALPHA ||
                       r.SrcBlend == D3D11_BLEND_ZERO) &&
                      (r.RenderTargetWriteMask & 7u) != 0 && mask != 0;
}

// INFRONT: the slice's deferred tail (infront_inject, late - the pass's scene RT0 and main depth bound,
// after the engine's slice draws) arms its re-issue into T: in an armed cycle whose snapshot the engine's planes
// never took (no translucent slice draw - an iron sight), it is taken now, the depth holding the world, the hands
// and our solids, what the engine-time one holds; else the landed one stands. True = this cycle's snapshot landed,
// and each normal-blend part is drawn again into T (vmsee_t_begin / vmsee_t_end), as the engine's planes are.
inline bool infront_vmsee_take(ID3D11DeviceContext* ctx) {
    if (g_vmsee.want_cycle != g_topo_cycles) return false;
    if (g_vmsee.snap_cycle != g_topo_cycles) {
        g_vmsee.snap_cycle = g_topo_cycles;
        if (vmsee_snapshot(ctx)) g_vmsee.ok_cycle.store(g_topo_cycles, std::memory_order_relaxed);
    }
    return g_vmsee.ok_cycle.load(std::memory_order_relaxed) == g_topo_cycles;
}

inline void vmsee_sv_release() {
    for (ID3D11RenderTargetView*& r : g_vmsee.sv_rtvs) KH_SAFE_RELEASE(r);
    KH_SAFE_RELEASE(g_vmsee.sv_dsv);
    KH_SAFE_RELEASE(g_vmsee.sv_bs);
}

// The re-issue's OM: the transmittance alone, no depth view - so no depth or stencil test, and the bound
// depth-stencil state and reference untouched - under bs_vmsee_t. The viewport, scissor, rasterizer, shaders,
// constants, views, input and predicate stay the caller's (the engine's at a draw hook's post). Refused with a UAV
// bound (vmir_begin's rule: a plain OMSetRenderTargets would drop it) or a stream-output target (the draw again
// would append to it). Callers: the draw hooks' post blocks (render thread) and the slice's deferred tail
// (infront_inject's late pass, after infront_vmsee_take), which the scene resolve may host on the game thread.
inline bool vmsee_t_begin(ID3D11DeviceContext* ctx) {
    if (!g_res.vmsee_t_rtv || !g_res.bs_vmsee_t) return false;
    ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmsee.sv_rtvs, &g_vmsee.sv_dsv);
    ID3D11UnorderedAccessView* uavs[8] = {};
    ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 8, uavs);   // UAV probe only.
    bool uav = false;
    for (ID3D11UnorderedAccessView*& u : uavs) {
        if (u) uav = true;
        KH_SAFE_RELEASE(u);
    }
    ID3D11Buffer* so = nullptr;
    ctx->SOGetTargets(1, &so);
    if (so) {
        uav = true;
        so->Release();
    }
    if (uav) {
        vmsee_sv_release();
        return false;
    }
    ctx->OMGetBlendState(&g_vmsee.sv_bs, g_vmsee.sv_bf, &g_vmsee.sv_mask);
    InjectionFlagScope fl;
    g_ro.in_injection = true;
    ctx->OMSetRenderTargets(1, &g_res.vmsee_t_rtv, nullptr);
    const FLOAT bf[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    ctx->OMSetBlendState(g_res.bs_vmsee_t, bf, 0xFFFFFFFF);
    return true;
}

inline void vmsee_t_end(ID3D11DeviceContext* ctx) {
    {
        InjectionFlagScope fl;   // The engine's own set back, which its trackers already saw.
        g_ro.in_injection = true;
        ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, g_vmsee.sv_rtvs, g_vmsee.sv_dsv);
        ctx->OMSetBlendState(g_vmsee.sv_bs, g_vmsee.sv_bf, g_vmsee.sv_mask);
    }
    vmsee_sv_release();
}
}