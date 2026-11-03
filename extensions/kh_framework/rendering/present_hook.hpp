#pragma once

// The injection half (rendering_integration_injection.hpp): the Present hook and the overlay bracket. Never include it
// on its own.
namespace RenderIntegration {
// Defined in rendering/ui_flush.hpp (declared here for the uses below).
inline void flush_ui_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool present);

inline bool present_ui_live() {
    if (!g_present.hook_active.load(std::memory_order_relaxed)) return false;
    const uint64_t ms = g_present.ui_ms.load(std::memory_order_relaxed);
    return ms != 0 && steady_now_ms() - ms < PRESENT_UI_STALE_MS;
}
inline void present_rtv_release() {
    for (int i = 0; i < 4; ++i) { KH_SAFE_RELEASE(g_present.rtv[i]);
        g_present.rtv_id[i] = nullptr; }
    g_present.rtv_next = 0;
}

// PRESENT_UI: the Present path (see the globals) - present_cb below, a subscriber of framework.hpp's one Present
// detour (KH_SHARED_PRESENT; slot KH_PRESENT_SLOT_RENDER, before the HTML overlay's), runs the UI flush into the
// swapchain's backbuffer on the presenting thread; the detour forwards. This helper is its view of GetBuffer(0) when
// the mask machine armed none this frame: the g_present.rtv ring, keyed by the buffer pointer (a TYPELESS buffer gets
// a UNORM view), its four slots reused in turn.
inline ID3D11RenderTargetView* present_rtv(ID3D11Device* dev, ID3D11Texture2D* bb) {
    void* id = static_cast<void*>(bb);
    for (int i = 0; i < 4; ++i) if (g_present.rtv[i] &&
        g_present.rtv_id[i] == id) return g_present.rtv[i];
    D3D11_TEXTURE2D_DESC td = {};
    bb->GetDesc(&td);
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    const D3D11_RENDER_TARGET_VIEW_DESC* rdp = nullptr;
    if (td.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS || td.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS) {
        rd.Format = td.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
        rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rdp = &rd;
    }
    ID3D11RenderTargetView* rtv = nullptr;
    if (FAILED(dev->CreateRenderTargetView(bb, rdp, &rtv)) || !rtv) return nullptr;
    const uint32_t s = g_present.rtv_next++ & 3u;
    KH_SAFE_RELEASE(g_present.rtv[s]);
    g_present.rtv[s] = rtv;
    g_present.rtv_id[s] = id;
    return rtv;
}
inline int overlay_begin() {
    if (t_kh_present_foreign) return 4;
    int tok = 0;
    if (reorder_on_render_thread()) {
        tok = 1 | (g_ro.in_injection ? 2 : 0);
        g_ro.in_injection = true;
    }
    g_ui_mask_injecting.store(true, std::memory_order_relaxed);
    return tok;
}
inline void overlay_end(int tok) {
    if (tok & 4) return;
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
    if (tok & 1) g_ro.in_injection = (tok & 2) != 0;
}
static void present_cb(IDXGISwapChain* self, UINT sync, UINT flags) {
    (void)sync;
    t_kh_present_foreign = false;   // OVERLAY_FOREIGN: this Present's verdict, below.
    try {
        // PRESENT_TEST: a DXGI_PRESENT_TEST call presents nothing (an occlusion probe) and ends no frame; only a
        // presenting call does. The shared detour runs no subscriber on one (framework.hpp); this return holds the same
        // rule here.
        if (flags & DXGI_PRESENT_TEST) return;   // Shows nothing: the detour forwards it.
        // PRESENT_OWN: the detour is on DXGI's shared Present, so it runs for every swap chain this process
        // presents. One of another device (an overlay or capture tool with a D3D11 device of its own) is not the game's
        // frame: nothing of ours is drawn on it and the presented-frame clock does not count it. Identity by IUnknown
        // (COM's rule); a device that cannot be told (no bridge device yet, a failed query) is served. Two
        // queries and two releases per Present. The HTML UI's subscriber (UIFramework::on_present) holds the same rule.
        {
            ID3D11Device* const game = RVExtBridge::get_d3d_device();
            if (self && game) {
                IUnknown* sd = nullptr;
                IUnknown* gd = nullptr;
                const bool have =
                    SUCCEEDED(self->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&sd))) &&
                    SUCCEEDED(game->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&gd)));
                const bool foreign = have && sd != gd;
                if (sd) sd->Release();
                if (gd) gd->Release();
                if (foreign) { t_kh_present_foreign = true; return; }   // OVERLAY_FOREIGN.
            }
        }
        ID3D11DeviceContext* ctx = static_cast<ID3D11DeviceContext*>(g_reorder_target_ctx.load(std::memory_order_relaxed));
        if (self && ctx && g_ui_mask_wanted.load(std::memory_order_relaxed) &&
            !g_flush_active.load(std::memory_order_relaxed)) {
            // The device is a term, not a fallback: flush_ui_locked dereferences it (ensure_resources, the alpha-stage
            // create), and the lock path guards it the same way at flush_ui_frame's head. Without the term a null
            // device would reach the flush whenever the held-RTV branch below supplied the view, which needs no
            // device of its own. Read here, behind the cheap flags, as every other call site reads it - a Present with
            // no UI pass reads it only for PRESENT_OWN.
            ID3D11Device* dev = RVExtBridge::get_d3d_device();
            // PRESENT_CTX: g_reorder_target_ctx is a raw pointer that ensure_reorder_hook refreshes from the
            // bridge at the game thread's flush, and the device ladder (engine_reset_release) does not clear it:
            // after a reset that made a new context, the old one stays tracked until that refresh. It is called
            // here only while the bridge still names it - the same object in normal play (the hooks only compare
            // it); otherwise this Present draws nothing of ours and flush_ui_frame's lock path stands in.
            if (dev && RVExtBridge::get_d3d_device_context() != ctx) dev = nullptr;
            ID3D11Texture2D* bb = nullptr;
            if (dev && FAILED(self->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb)))) bb = nullptr;
            if (bb) {
                // PRESENT_RTV: the engine's own view when the machine armed
                // this frame (same thread); GetBuffer(0)'s otherwise.
                ID3D11RenderTargetView* rtv =
                    (g_ui_frame_rtv && g_ui_frame_rtv_seq == g_present.seq.load(std::memory_order_relaxed))
                        ? g_ui_frame_rtv
                        : present_rtv(dev, bb);
                // Neither source hands out a reference (g_ui_frame_rtv is the mask machine's, the ring's views the
                // ring's), so one is held to the bind below. That is not a cross-thread guard: it relies on the view's
                // replacement running in a draw hook of this same context, which the engine never drives from two
                // threads at once.
                if (rtv) rtv->AddRef();
                // The view holds the buffer from here, and nothing below reads bb - so it goes back now rather than
                // after the flush. A throw out of flush_ui_locked reaches this function's catch, which cannot see this
                // scope, and a stranded reference on backbuffer 0 is not an ordinary leak: it fails the engine's next
                // ResizeBuffers.
                bb->Release();
                bb = nullptr;
                if (rtv) {
                    // Our draws are ours to the hooks on either thread: the render-thread hooks stand aside on
                    // in_injection, the UI-thread branch and the OM hooks on g_ui_mask_injecting.
                    const bool rt = reorder_on_render_thread();
                    const bool pinj = g_ro.in_injection;
                    if (rt) g_ro.in_injection = true;
                    g_ui_mask_injecting.store(true, std::memory_order_relaxed);
                    {
                        OmSave om;
                        om.capture(ctx);
                        ctx->OMSetRenderTargets(1, &rtv, nullptr);
                        rtv->Release();   // The bind holds it now; nothing below reads the pointer.
                        flush_ui_locked(dev, ctx, true);   // UI_LEARN_OWN.
                        om.restore(ctx);
                        om.release();
                    }
                    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
                    if (rt) g_ro.in_injection = pinj;
                    g_present.ui_ms.store(steady_now_ms(), std::memory_order_relaxed);
                }
            }
        }
        // PRESENT_SEQ: the presented-frame clock, bumped after the flush so the next frame's flush sees +1 (a
        // Present with nothing to draw for us still ends a frame).
        g_present.seq.fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
        g_ui_mask_injecting.store(false, std::memory_order_relaxed);
        hook_except();
    }
}
static HRESULT STDMETHODCALLTYPE hooked_resizebuffers(IDXGISwapChain* self, UINT count, UINT w,
                                                      UINT h, DXGI_FORMAT fmt, UINT flags) {
    try {
        // The Present RTV ring is the presenting thread's; g_ui_frame_rtv is replaced at the mask machine's arm.
        // Releasing it here relies on the engine not drawing on the immediate context while it resizes the swapchain.
        // The whole mask reset is done because every backbuffer identity and size it learned is about to be stale.
        // RESIZE_OWN: PRESENT_OWN's device test - a swap chain of another device (an overlay or capture
        // tool) holds nothing of ours, so its resize keeps the game's; one that cannot be told is served.
        bool foreign = false;
        ID3D11Device* const game = RVExtBridge::get_d3d_device();
        if (self && game) {
            IUnknown* sd = nullptr;
            IUnknown* gd = nullptr;
            const bool have =
                SUCCEEDED(self->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&sd))) &&
                SUCCEEDED(game->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&gd)));
            foreign = have && sd != gd;
            if (sd) sd->Release();
            if (gd) gd->Release();
        }
        if (!foreign) {
            present_rtv_release();
            ui_mask_reset();
            g_res.release_wsee_c();   // WSEE: C's reference to the engine's scene target (Resources' note).
        }
    } catch (...) {
        // None of the releases can throw; the guard is here so that nothing of ours can turn a resize into a failed
        // one. hook_except is deliberately NOT called - on the render thread it clears g_ro.in_injection, and a
        // resize raises no latch of ours to drop.
    }
    return g_orig_resize_buffers ? g_orig_resize_buffers(self, count, w, h, fmt, flags)
                                 : DXGI_ERROR_INVALID_CALL;
}
// The install: a swapchain on a hidden window, on the game's device, for its vtable; the window and the swapchain are
// gone before the hook is enabled. g_present.hook_failed is never reset: a MinHook refusal latches at once, a swapchain
// that could not be made after three attempts (PRESENT_RETRY); a failure leaves the lock path in place.
inline void ensure_present_hook() {
    if (g_present.hook_active.load(std::memory_order_relaxed) || g_present.hook_failed) return;
    if (g_present.hook_retry_ms != 0 && steady_now_ms() < g_present.hook_retry_ms) return;   // PRESENT_RETRY.
    if (!g_reorder_hook.active.load(std::memory_order_acquire)) return;   // MinHook is up once the context hooks are.
    // OVERLAY_BRACKET: registered by flush_frame whenever the context hooks are live; again here, whether or not our
    // own Present subscription below succeeds.
    kh_present_overlay_register(&overlay_begin, &overlay_end);
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    g_present.hook_failed = true;   // Cleared on success below.
    IDXGIDevice* dxgi = nullptr;
    IDXGIAdapter* ad = nullptr;
    IDXGIFactory* fac = nullptr;
    IDXGISwapChain* sc = nullptr;
    HWND hw = nullptr;
    void* slot = nullptr;
    void* slot_rb = nullptr;   // PRESENT_RESIZE.
    do {
        if (FAILED(dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi))) || !dxgi) break;
        if (FAILED(dxgi->GetAdapter(&ad)) || !ad) break;
        if (FAILED(ad->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&fac))) || !fac) break;
        hw = CreateWindowExA(0, "STATIC", "kh_present_probe", WS_POPUP, 0, 0, 2, 2, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
        if (!hw) break;
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 2;
        sd.BufferDesc.Height = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hw;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        if (FAILED(fac->CreateSwapChain(dev, &sd, &sc)) || !sc) break;
        slot = (*reinterpret_cast<void***>(sc))[8];   // IDXGISwapChain::Present.
        slot_rb = (*reinterpret_cast<void***>(sc))[13];   // IDXGISwapChain::ResizeBuffers.
    } while (false);
    KH_SAFE_RELEASE(sc);
    if (hw) DestroyWindow(hw);
    KH_SAFE_RELEASE(fac);
    KH_SAFE_RELEASE(ad);
    KH_SAFE_RELEASE(dxgi);
    if (!slot) {
        // PRESENT_RETRY: asked again 1 s, then 10 s later; the third failure latches and is reported.
        ++g_present.hook_fail_n;
        if (g_present.hook_fail_n < 3u) {
            g_present.hook_failed = false;
            g_present.hook_retry_ms = steady_now_ms() + (g_present.hook_fail_n == 1u ? 1000u : 10000u);
            return;
        }
        report_error_once_safe("KH RenderIntegration: Present hook: no swapchain vtable (UI passes keep the lock "
                               "path)");
        return;
    }
    // PRESENT_RESIZE: installed BEFORE the Present hook. The two swapchain-buffer references it releases (the ring,
    // g_ui_frame_rtv) are taken only once the Present hook is installed (UI_FRAME_RTV_GATE), so a session in which
    // the probe swap chain failed holds none. Its third release, WSEE's C, is on the engine's scene target, not a
    // swapchain buffer: without this hook C keeps the old target alive until the next chain run rekeys it. A failure
    // here is reported and stands nothing down: the session then resizes without this hook.
    if (slot_rb && !g_orig_resize_buffers &&
        (MH_CreateHook(slot_rb, reinterpret_cast<void*>(&hooked_resizebuffers), reinterpret_cast<void**>(&g_orig_resize_buffers)) != MH_OK ||
         MH_EnableHook(slot_rb) != MH_OK)) {
        // MinHook writes the trampoline only on a successful create, so a failed create leaves g_orig_resize_buffers
        // null and the hook uninstalled - our function is never entered. It is NOT nulled here: if the create succeeded
        // and only the enable failed, the trampoline is valid and must stay, or an enable that partly took would
        // forward through a null pointer and fail every resize outright.
        report_error_once_safe("KH RenderIntegration: ResizeBuffers hook: MinHook failed (a resolution change with a UI effect active may fail to resize)");
    }
    // KH_SHARED_PRESENT: the one Present detour (framework.hpp) - the HTML
    // overlay subscribes to the same target, which a second MinHook could not.
    if (kh_present_hook(slot) != MH_OK) {
        report_error_once_safe("KH RenderIntegration: Present hook: MinHook failed (UI passes keep the lock path)");
        return;
    }
    kh_present_subscribe(KH_PRESENT_SLOT_RENDER, &present_cb);
    g_present.hook_failed = false;
    g_present.hook_active.store(true, std::memory_order_release);
}
}