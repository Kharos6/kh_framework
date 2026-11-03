#pragma once

// The injection half (rendering_integration_injection.hpp): the UI mask (UiMask): learning, the clear, the UI draw
// blends and the quad. Never include it on its own.
namespace RenderIntegration {
// Device reset, session destroy, or a buffer resize about to invalidate every learned identity (PRESENT_RESIZE).
inline void ui_mask_reset() {
    g_ui_mask = UiMask{};
    KH_SAFE_RELEASE(g_ui_frame_rtv);   // PRESENT_RTV.
    g_ui_frame_rtv_seq = 0;
    g_ui_mask_wanted.store(false, std::memory_order_relaxed);
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
}

inline bool ui_mask_known_bb(void* id, UINT* w = nullptr, UINT* h = nullptr) {
    for (uint32_t i = 0; i < g_ui_mask.bb_n; ++i) {
        if (g_ui_mask.bb_id[i] == id) {
            if (w) *w = g_ui_mask.bb_w[i];
            if (h) *h = g_ui_mask.bb_h[i];
            return true;
        }
    }
    return false;
}

// The ring's reader is ui_mask_known_bb (ui_mask_thread_draw), on a draw hook of the immediate context; the
// writer runs inside the UI flush on that context. Plain stores give a concurrent reader no order: the pair is safe
// because the engine drives the context from one thread at a time (UI_DIAG), not by the write order.
inline void ui_mask_learn(void* id, UINT w, UINT h) {
    for (uint32_t i = 0; i < g_ui_mask.bb_n; ++i) {
        if (g_ui_mask.bb_id[i] == id) {
            g_ui_mask.bb_w[i] = w;   // Resize: same buffer object, new dims.
            g_ui_mask.bb_h[i] = h;
            return;
        }
    }
    const uint32_t slot = g_ui_mask.bb_n < 4 ? g_ui_mask.bb_n : 3;
    g_ui_mask.bb_id[slot] = id;
    g_ui_mask.bb_w[slot] = w;
    g_ui_mask.bb_h[slot] = h;
    if (g_ui_mask.bb_n < 4) g_ui_mask.bb_n++;
}

// Resources may lag one frame at spawn (this never builds them: ensure_resources runs in the flush and in the
// render-thread passes); a missing resource, or a depth view or a target other than the armed one bound at the clear
// (UI_CLEAR_REFUSE), leaves the phase at 2, mask_valid false, and the UI-phase flush skips - the designed fail-safe.
inline void ui_mask_clear_alpha(ID3D11DeviceContext* ctx) {
    if (!g_res.ps_alpha_zero || !g_res.blend_alpha_zero ||
        !g_res.vs_fullscreen || !g_res.dss_off || !g_res.rasterizer) return;

    {
        ID3D11RenderTargetView* rtv = nullptr;
        ID3D11DepthStencilView* dsv = nullptr;
        ctx->OMGetRenderTargets(1, &rtv, &dsv);
        void* id = nullptr;

        if (rtv) {
            ID3D11Resource* r = nullptr;
            rtv->GetResource(&r);
            rtv->Release();
            if (r) { id = static_cast<void*>(r); r->Release(); }
        }

        const bool bad = (dsv != nullptr) || id != g_ui_mask.phase_bb;
        if (dsv) dsv->Release();

        if (bad) return;   // UI_CLEAR_REFUSE.
    }

    g_ui_mask_injecting.store(true, std::memory_order_relaxed);   // Keeps out the mask machine, the UI-thread
                                                                  // branch and the OM hooks only. Not
                                                                  // g_ro.in_injection: on the render thread the
                                                                  // hooks keyed on it alone see this Draw.
    StateBackup backup;
    backup.capture(ctx);

    UINT nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT saved_vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&nvp, saved_vp);

    D3D11_VIEWPORT full = {};
    full.Width = static_cast<FLOAT>(g_ui_mask.phase_w);
    full.Height = static_cast<FLOAT>(g_ui_mask.phase_h);
    full.MinDepth = 0.0f;
    full.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &full);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_alpha_zero, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->RSSetState(g_res.rasterizer);
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_alpha_zero, bf, 0xFFFFFFFF);
    ctx->Draw(3, 0);

    if (nvp > 0) ctx->RSSetViewports(nvp, saved_vp);
    backup.restore(ctx);
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);

    g_ui_mask.phase = 3;
    g_ui_mask.mask_valid = true;
}

// Does this draw blend against its target? UI_QUAD_CLEAR's opaque test: on the HUD the scene quad is opaque
// and every widget blends. The OM hook's verdict (g_ro.blend_translucent) is render-thread state and the
// backbuffer draws come from both threads, so the state is read here (one GetDesc on an immutable object per
// backbuffer draw).
inline bool ui_draw_blends(ID3D11DeviceContext* ctx) {
    ID3D11BlendState* bs = nullptr;
    FLOAT bf[4] = {};
    UINT mask = 0;
    ctx->OMGetBlendState(&bs, bf, &mask);
    if (!bs) return false;   // The default state: opaque.
    D3D11_BLEND_DESC d = {};
    bs->GetDesc(&d);
    bs->Release();
    return d.RenderTarget[0].BlendEnable && d.RenderTarget[0].DestBlend != D3D11_BLEND_ZERO;
}
// Post-draw, the four core draw hooks (not DrawAuto or the indirect draws); one thread-local test when nothing is
// pending.
inline void ui_quad_post(ID3D11DeviceContext* ctx) {
    if (!g_uiq_ctx) return;
    // VMIR_OWN_DRAW's rule: a draw of our own inside the qualifying draw's pre-draw leaves the pending note to that
    // draw's own post (the clear must land after the engine's quad, not before it). The flag is the render thread's:
    // read only there (the game thread's draws never run under a raise of its own outside a park, and a park issues no
    // UI quad).
    if (g_ro.in_injection && reorder_on_render_thread()) return;
    const bool mine = g_uiq_ctx == ctx;
    g_uiq_ctx = nullptr;
    if (!mine) return;
    if (!g_ui_mask_wanted.load(std::memory_order_relaxed) || g_ui_mask_injecting.load(std::memory_order_relaxed) ||
        g_flush_active.load(std::memory_order_relaxed)) return;
    g_ui_mask.phase_bb = g_uiq_id;
    g_ui_mask.phase_w = g_uiq_w;
    g_ui_mask.phase_h = g_uiq_h;
    KH_SAFE_RELEASE(g_ui_frame_rtv);
    if (g_present.hook_active.load(std::memory_order_relaxed)) {   // UI_FRAME_RTV_GATE: its reader exists.
        // PRESENT_RTV: the Present flush draws into the buffer this clear marked.
        ID3D11RenderTargetView* rtv = nullptr;
        ctx->OMGetRenderTargets(1, &rtv, nullptr);
        g_ui_frame_rtv = rtv;
        g_ui_frame_rtv_seq = g_present.seq.load(std::memory_order_relaxed);
    }
    g_ui_mask.mask_valid = false;
    g_ui_mask.compose_serial++;
    g_ui_mask.phase = 2;
    ui_mask_clear_alpha(ctx);   // Phase 3, mask valid.
}
inline void ui_mask_thread_draw(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_UI_MASK_DRAW);   // PROF.
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);

    if (dsv) {
        dsv->Release();
        if (rtv) rtv->Release();
        if (g_ui_mask.phase == 2) g_ui_mask.phase = 0;
        return;
    }

    if (!rtv) return;
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    rtv->Release();
    if (!res) return;
    void* id = static_cast<void*>(res);
    res->Release();
    UINT w = 0, h = 0;

    if (!ui_mask_known_bb(id, &w, &h)) {

        if (g_ui_mask.phase == 2) {   // Foreign target before the clear: abort.
            g_ui_mask.phase = 0;
            g_ui_mask.mask_valid = false;
        }

        return;
    }

    // Note the qualifying draw; its post-draw hook (ui_quad_post) clears.
    if (!ui_draw_blends(ctx)) {
        UINT n = 1;
        D3D11_VIEWPORT vp = {};
        ctx->RSGetViewports(&n, &vp);
        if (n >= 1 && vp.TopLeftX <= 1.0f && vp.TopLeftY <= 1.0f &&
            vp.Width >= 0.9f * static_cast<float>(w) && vp.Height >= 0.9f * static_cast<float>(h)) {
            g_uiq_ctx = ctx;
            g_uiq_id = id;
            g_uiq_w = w;
            g_uiq_h = h;
        }
    }
}
}