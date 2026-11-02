#pragma once

// The injection half (rendering_integration_injection.hpp): the Present hook and the overlay bracket. Never include it
// on its own.
namespace RenderIntegration {
// Defined in rendering/ui_flush.hpp (declared here for the uses below).
inline void flush_ui_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool khul_present);

inline bool kh_present_ui_live() {
    if (!g_present_hook_active.load(std::memory_order_relaxed)) return false;
    const uint64_t khpl_ms = g_present_ui_ms.load(std::memory_order_relaxed);
    return khpl_ms != 0 && steady_now_ms() - khpl_ms < KH_PRESENT_UI_STALE_MS;
}
inline void kh_present_rtv_release() {
    for (int khpr_i = 0; khpr_i < 4; ++khpr_i) { KH_SAFE_RELEASE(g_present_rtv[khpr_i]); g_present_rtv_id[khpr_i] = nullptr; }
    g_present_rtv_next = 0;
}

// KH_PRESENT_UI: the Present path (see the globals) - kh_present_cb below, a subscriber of framework.hpp's one Present
// detour (KH_SHARED_PRESENT; slot KH_PRESENT_SLOT_RENDER, before the HTML overlay's), runs the UI flush into the
// swapchain's backbuffer on the presenting thread; the detour forwards. This helper is its view of GetBuffer(0) when
// the mask machine armed none this frame: the g_present_rtv ring, keyed by the buffer pointer (a TYPELESS buffer gets
// a UNORM view), its four slots reused in turn.
inline ID3D11RenderTargetView* kh_present_rtv(ID3D11Device* khpv_dev, ID3D11Texture2D* khpv_bb) {
    void* khpv_id = static_cast<void*>(khpv_bb);
    for (int khpv_i = 0; khpv_i < 4; ++khpv_i) if (g_present_rtv[khpv_i] && g_present_rtv_id[khpv_i] == khpv_id) return g_present_rtv[khpv_i];
    D3D11_TEXTURE2D_DESC khpv_td = {};
    khpv_bb->GetDesc(&khpv_td);
    D3D11_RENDER_TARGET_VIEW_DESC khpv_rd = {};
    const D3D11_RENDER_TARGET_VIEW_DESC* khpv_rdp = nullptr;
    if (khpv_td.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS || khpv_td.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS) {
        khpv_rd.Format = khpv_td.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_B8G8R8A8_UNORM;
        khpv_rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        khpv_rdp = &khpv_rd;
    }
    ID3D11RenderTargetView* khpv_rtv = nullptr;
    if (FAILED(khpv_dev->CreateRenderTargetView(khpv_bb, khpv_rdp, &khpv_rtv)) || !khpv_rtv) return nullptr;
    const uint32_t khpv_s = g_present_rtv_next++ & 3u;
    KH_SAFE_RELEASE(g_present_rtv[khpv_s]);
    g_present_rtv[khpv_s] = khpv_rtv;
    g_present_rtv_id[khpv_s] = khpv_id;
    return khpv_rtv;
}
inline int kh_overlay_begin() {
    if (t_kh_present_foreign) return 4;
    int khob_tok = 0;
    if (reorder_on_render_thread()) {
        khob_tok = 1 | (g_ro.in_injection ? 2 : 0);
        g_ro.in_injection = true;
    }
    g_ui_mask_injecting.store(true, std::memory_order_relaxed);
    return khob_tok;
}
inline void kh_overlay_end(int khoe_tok) {
    if (khoe_tok & 4) return;
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
    if (khoe_tok & 1) g_ro.in_injection = (khoe_tok & 2) != 0;
}
static void kh_present_cb(IDXGISwapChain* self, UINT khp_sync, UINT khp_flags) {
    (void)khp_sync;
    t_kh_present_foreign = false;   // KH_OVERLAY_FOREIGN: this Present's verdict, below.
    try {
        // KH_PRESENT_TEST: a DXGI_PRESENT_TEST call presents nothing (an occlusion probe) and ends no frame; only a
        // presenting call does. The shared detour runs no subscriber on one (framework.hpp); this return holds the same
        // rule here.
        if (khp_flags & DXGI_PRESENT_TEST) return;   // Shows nothing: the detour forwards it.
        // KH_PRESENT_OWN: the detour is on DXGI's shared Present, so it runs for every swap chain this process
        // presents. One of another device (an overlay or capture tool with a D3D11 device of its own) is not the game's
        // frame: nothing of ours is drawn on it and the presented-frame clock does not count it. Identity by IUnknown
        // (COM's rule); a device that cannot be told (no bridge device yet, a failed query) is served. Two
        // queries and two releases per Present. The HTML UI's subscriber (UIFramework::on_present) holds the same rule.
        {
            ID3D11Device* const khpo_game = RVExtBridge::get_d3d_device();
            if (self && khpo_game) {
                IUnknown* khpo_sd = nullptr;
                IUnknown* khpo_gd = nullptr;
                const bool khpo_have =
                    SUCCEEDED(self->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&khpo_sd))) &&
                    SUCCEEDED(khpo_game->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&khpo_gd)));
                const bool khpo_foreign = khpo_have && khpo_sd != khpo_gd;
                if (khpo_sd) khpo_sd->Release();
                if (khpo_gd) khpo_gd->Release();
                if (khpo_foreign) { t_kh_present_foreign = true; return; }   // KH_OVERLAY_FOREIGN.
            }
        }
        ID3D11DeviceContext* khp_ctx = static_cast<ID3D11DeviceContext*>(g_reorder_target_ctx.load(std::memory_order_relaxed));
        if (self && khp_ctx && g_ui_mask_wanted.load(std::memory_order_relaxed) &&
            !g_kh_flush_active.load(std::memory_order_relaxed)) {
            // The device is a term, not a fallback: flush_ui_locked dereferences it (ensure_resources, the alpha-stage
            // create), and the lock path guards it the same way at flush_ui_frame's head. Without the term a null
            // device would reach the flush whenever the held-RTV branch below supplied the view, which needs no
            // device of its own. Read here, behind the cheap flags, as every other call site reads it - a Present with
            // no UI pass reads it only for KH_PRESENT_OWN.
            ID3D11Device* khp_dev = RVExtBridge::get_d3d_device();
            // KH_PRESENT_CTX: g_reorder_target_ctx is a raw pointer that ensure_reorder_hook refreshes from the
            // bridge at the game thread's flush, and the device ladder (kh_engine_reset_release) does not clear it:
            // after a reset that made a new context, the old one stays tracked until that refresh. It is called
            // here only while the bridge still names it - the same object in normal play (the hooks only compare
            // it); otherwise this Present draws nothing of ours and flush_ui_frame's lock path stands in.
            if (khp_dev && RVExtBridge::get_d3d_device_context() != khp_ctx) khp_dev = nullptr;
            ID3D11Texture2D* khp_bb = nullptr;
            if (khp_dev && FAILED(self->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khp_bb)))) khp_bb = nullptr;
            if (khp_bb) {
                // KH_PRESENT_RTV: the engine's own view when the machine armed
                // this frame (same thread); GetBuffer(0)'s otherwise.
                ID3D11RenderTargetView* khp_rtv =
                    (g_ui_frame_rtv && g_ui_frame_rtv_seq == g_present_seq.load(std::memory_order_relaxed))
                        ? g_ui_frame_rtv
                        : kh_present_rtv(khp_dev, khp_bb);
                // Neither source hands out a reference (g_ui_frame_rtv is the mask machine's, the ring's views the
                // ring's), so one is held to the bind below. That is not a cross-thread guard: it relies on the view's
                // replacement running in a draw hook of this same context, which the engine never drives from two
                // threads at once.
                if (khp_rtv) khp_rtv->AddRef();
                // The view holds the buffer from here, and nothing below reads khp_bb - so it goes back now rather than
                // after the flush. A throw out of flush_ui_locked reaches this function's catch, which cannot see this
                // scope, and a stranded reference on backbuffer 0 is not an ordinary leak: it fails the engine's next
                // ResizeBuffers.
                khp_bb->Release();
                khp_bb = nullptr;
                if (khp_rtv) {
                    // Our draws are ours to the hooks on either thread: the render-thread hooks stand aside on
                    // in_injection, the UI-thread branch and the OM hooks on g_ui_mask_injecting.
                    const bool khp_rt = reorder_on_render_thread();
                    const bool khp_pinj = g_ro.in_injection;
                    if (khp_rt) g_ro.in_injection = true;
                    g_ui_mask_injecting.store(true, std::memory_order_relaxed);
                    {
                        KhOmSave khp_om;
                        khp_om.capture(khp_ctx);
                        khp_ctx->OMSetRenderTargets(1, &khp_rtv, nullptr);
                        khp_rtv->Release();   // The bind holds it now; nothing below reads the pointer.
                        flush_ui_locked(khp_dev, khp_ctx, true);   // KH_UI_LEARN_OWN.
                        khp_om.restore(khp_ctx);
                        khp_om.release();
                    }
                    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
                    if (khp_rt) g_ro.in_injection = khp_pinj;
                    g_present_ui_ms.store(steady_now_ms(), std::memory_order_relaxed);
                }
            }
        }
        // KH_PRESENT_SEQ: the presented-frame clock, bumped after the flush so the next frame's flush sees +1 (a
        // Present with nothing to draw for us still ends a frame).
        g_present_seq.fetch_add(1, std::memory_order_relaxed);
    } catch (...) {
        g_ui_mask_injecting.store(false, std::memory_order_relaxed);
        kh_hook_except();
    }
}
static HRESULT STDMETHODCALLTYPE hooked_resizebuffers(IDXGISwapChain* self, UINT khrb_count, UINT khrb_w,
                                                      UINT khrb_h, DXGI_FORMAT khrb_fmt, UINT khrb_flags) {
    try {
        // The Present RTV ring is the presenting thread's; g_ui_frame_rtv is replaced at the mask machine's arm.
        // Releasing it here relies on the engine not drawing on the immediate context while it resizes the swapchain.
        // The whole mask reset is done because every backbuffer identity and size it learned is about to be stale.
        // KH_RESIZE_OWN: KH_PRESENT_OWN's device test - a swap chain of another device (an overlay or capture
        // tool) holds nothing of ours, so its resize keeps the game's; one that cannot be told is served.
        bool khrb_foreign = false;
        ID3D11Device* const khrb_game = RVExtBridge::get_d3d_device();
        if (self && khrb_game) {
            IUnknown* khrb_sd = nullptr;
            IUnknown* khrb_gd = nullptr;
            const bool khrb_have =
                SUCCEEDED(self->GetDevice(__uuidof(IUnknown), reinterpret_cast<void**>(&khrb_sd))) &&
                SUCCEEDED(khrb_game->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&khrb_gd)));
            khrb_foreign = khrb_have && khrb_sd != khrb_gd;
            if (khrb_sd) khrb_sd->Release();
            if (khrb_gd) khrb_gd->Release();
        }
        if (!khrb_foreign) {
            kh_present_rtv_release();
            kh_ui_mask_reset();
            g_res.release_wsee_c();   // KH_WSEE: C's reference to the engine's scene target (Resources' note).
        }
    } catch (...) {
        // None of the releases can throw; the guard is here so that nothing of ours can turn a resize into a failed
        // one. kh_hook_except is deliberately NOT called - on the render thread it clears g_ro.in_injection, and a
        // resize raises no latch of ours to drop.
    }
    return g_orig_resize_buffers ? g_orig_resize_buffers(self, khrb_count, khrb_w, khrb_h, khrb_fmt, khrb_flags)
                                 : DXGI_ERROR_INVALID_CALL;
}
// The install: a swapchain on a hidden window, on the game's device, for its vtable; the window and the swapchain are
// gone before the hook is enabled. g_present_hook_failed is never reset: a MinHook refusal latches at once, a swapchain
// that could not be made after three attempts (KH_PRESENT_RETRY); a failure leaves the lock path in place.
inline void ensure_present_hook() {
    if (g_present_hook_active.load(std::memory_order_relaxed) || g_present_hook_failed) return;
    if (g_present_hook_retry_ms != 0 && steady_now_ms() < g_present_hook_retry_ms) return;   // KH_PRESENT_RETRY.
    if (!g_reorder_hook_active.load(std::memory_order_acquire)) return;   // MinHook is up once the context hooks are.
    // KH_OVERLAY_BRACKET: registered by flush_frame whenever the context hooks are live; again here, whether or not our
    // own Present subscription below succeeds.
    kh_present_overlay_register(&kh_overlay_begin, &kh_overlay_end);
    ID3D11Device* khph_dev = RVExtBridge::get_d3d_device();
    if (!khph_dev) return;
    g_present_hook_failed = true;   // Cleared on success below.
    IDXGIDevice* khph_dxgi = nullptr;
    IDXGIAdapter* khph_ad = nullptr;
    IDXGIFactory* khph_fac = nullptr;
    IDXGISwapChain* khph_sc = nullptr;
    HWND khph_hw = nullptr;
    void* khph_slot = nullptr;
    void* khph_slot_rb = nullptr;   // KH_PRESENT_RESIZE.
    do {
        if (FAILED(khph_dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&khph_dxgi))) || !khph_dxgi) break;
        if (FAILED(khph_dxgi->GetAdapter(&khph_ad)) || !khph_ad) break;
        if (FAILED(khph_ad->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&khph_fac))) || !khph_fac) break;
        khph_hw = CreateWindowExA(0, "STATIC", "kh_present_probe", WS_POPUP, 0, 0, 2, 2, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
        if (!khph_hw) break;
        DXGI_SWAP_CHAIN_DESC khph_sd = {};
        khph_sd.BufferCount = 1;
        khph_sd.BufferDesc.Width = 2;
        khph_sd.BufferDesc.Height = 2;
        khph_sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        khph_sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        khph_sd.OutputWindow = khph_hw;
        khph_sd.SampleDesc.Count = 1;
        khph_sd.Windowed = TRUE;
        khph_sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        if (FAILED(khph_fac->CreateSwapChain(khph_dev, &khph_sd, &khph_sc)) || !khph_sc) break;
        khph_slot = (*reinterpret_cast<void***>(khph_sc))[8];   // IDXGISwapChain::Present.
        khph_slot_rb = (*reinterpret_cast<void***>(khph_sc))[13];   // IDXGISwapChain::ResizeBuffers.
    } while (false);
    KH_SAFE_RELEASE(khph_sc);
    if (khph_hw) DestroyWindow(khph_hw);
    KH_SAFE_RELEASE(khph_fac);
    KH_SAFE_RELEASE(khph_ad);
    KH_SAFE_RELEASE(khph_dxgi);
    if (!khph_slot) {
        // KH_PRESENT_RETRY: asked again 1 s, then 10 s later; the third failure latches and is reported.
        ++g_present_hook_fail_n;
        if (g_present_hook_fail_n < 3u) {
            g_present_hook_failed = false;
            g_present_hook_retry_ms = steady_now_ms() + (g_present_hook_fail_n == 1u ? 1000u : 10000u);
            return;
        }
        report_error_once_safe("KH RenderIntegration: Present hook: no swapchain vtable (UI passes keep the lock "
                               "path)");
        return;
    }
    // KH_PRESENT_RESIZE: installed BEFORE the Present hook. The two swapchain-buffer references it releases (the ring,
    // g_ui_frame_rtv) are taken only once the Present hook is installed (KH_UI_FRAME_RTV_GATE), so a session in which
    // the probe swap chain failed holds none. Its third release, KH_WSEE's C, is on the engine's scene target, not a
    // swapchain buffer: without this hook C keeps the old target alive until the next chain run rekeys it. A failure
    // here is reported and stands nothing down: the session then resizes without this hook.
    if (khph_slot_rb && !g_orig_resize_buffers &&
        (MH_CreateHook(khph_slot_rb, reinterpret_cast<void*>(&hooked_resizebuffers), reinterpret_cast<void**>(&g_orig_resize_buffers)) != MH_OK ||
         MH_EnableHook(khph_slot_rb) != MH_OK)) {
        // MinHook writes the trampoline only on a successful create, so a failed create leaves g_orig_resize_buffers
        // null and the hook uninstalled - our function is never entered. It is NOT nulled here: if the create succeeded
        // and only the enable failed, the trampoline is valid and must stay, or an enable that partly took would
        // forward through a null pointer and fail every resize outright.
        report_error_once_safe("KH RenderIntegration: ResizeBuffers hook: MinHook failed (a resolution change with a UI effect active may fail to resize)");
    }
    // KH_SHARED_PRESENT: the one Present detour (framework.hpp) - the HTML
    // overlay subscribes to the same target, which a second MinHook could not.
    if (kh_present_hook(khph_slot) != MH_OK) {
        report_error_once_safe("KH RenderIntegration: Present hook: MinHook failed (UI passes keep the lock path)");
        return;
    }
    kh_present_subscribe(KH_PRESENT_SLOT_RENDER, &kh_present_cb);
    g_present_hook_failed = false;
    g_present_hook_active.store(true, std::memory_order_release);
}
}