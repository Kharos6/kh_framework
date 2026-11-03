#pragma once

// The injection half (rendering_integration_injection.hpp): the hooked D3D11 vtable slots, the original function
// pointers and the hook's install state. Never include it on its own.
namespace RenderIntegration {


// State hooks (OMSetBlendState / OMSetDepthStencilState / OMSetRenderTargets[AndUnorderedAccessViews]) maintain five
// booleans (g_ro's blend_translucent, dss_nowrite, dss_write, dss_stencil, dsv_main); a state object is classified by
// GetDesc on a cache miss and the verdict cached by object, AddRef'd while cached so a recycled address cannot
// inherit it (HOOK_DESC_CACHE).

// The context hooks' install (the reorder hook): active, the failure and its phase / slot, the vtable and its drift,
// the retry.
// Owner: the game thread (ensure_reorder_hook installs and retries, reorder_hook_fail_round reports); active is the
// atomic the render thread reads.
struct ReorderHookState {
    std::atomic<bool>     active{false};
    bool                  failed = false;
    // MH_STATUS of the failing call (0 = MH_OK / none; -1 = init phase, status unavailable).
    int32_t  mh_status = 0;
    int32_t  fail_phase = 0;   // 0 none, 1 MinHook init, 2 CreateHook, 3 enableHook.
    int32_t  fail_slot = -1;   // Spec index of the failing hook (-1 = none / init phase).
    void*    vt = nullptr;   // Vtable the hooks were installed on.
    uint64_t vt_drift = 0;   // Ensure calls with active set on a different vtable.
    int32_t  fail_count = 0;   // Failed install rounds (the retry ladder).
    uint64_t retry_ms = 0;   // Steady deadline of the next retry (0 = none scheduled).
};
static ReorderHookState g_reorder_hook;
static std::atomic<void*>    g_reorder_target_ctx{nullptr};   // Immediate context to act on.

// Injection attempts per scene frame: the live-DSV verification can transiently mismatch; retrying on later translucent
// draws prevents single-frame blink-outs, and the cap bounds the cost.
static constexpr uint32_t REORDER_MAX_INJECT_ATTEMPTS = 16;

// ID3D11DeviceContext vtable slots (d3d11.h declaration order).
static constexpr int VT_PSSETSHADERRESOURCES  = 8;
static constexpr int VT_DRAWINDEXED           = 12;
static constexpr int VT_DRAW                  = 13;
static constexpr int VT_MAP                   = 14;
static constexpr int VT_UNMAP                 = 15;
static constexpr int VT_DRAWINDEXEDINSTANCED  = 20;
static constexpr int VT_DRAWINSTANCED         = 21;
static constexpr int VT_OMSETRENDERTARGETS    = 33;
static constexpr int VT_OMSETRTS_AND_UAVS     = 34;
static constexpr int VT_OMSETBLENDSTATE       = 35;
static constexpr int VT_OMSETDEPTHSTENCIL     = 36;
static constexpr int VT_RESOLVESUBRESOURCE    = 57;
static constexpr int VT_UPDATESUBRESOURCE     = 48;
static constexpr int VT_COPYRESOURCE          = 47;   // PIP_FX_COPY.
static constexpr int VT_CLEARDEPTHSTENCIL     = 53;
// VOL_REPLAY: the paths into the volume buffer the recorder does not see (d3d11.h order, as the slots above).
static constexpr int VT_DRAWAUTO              = 38;
static constexpr int VT_DRAWINDEXEDINSTIND    = 39;
static constexpr int VT_DRAWINSTIND           = 40;
static constexpr int VT_COPYSUBRESOURCEREGION = 46;
static constexpr int VT_EXECUTECOMMANDLIST    = 58;
static constexpr int VT_COPYSTRUCTURECOUNT    = 49;   // REPLAY_CBREUSE: a GPU write into a buffer.
// ID3D11DeviceContext1's own methods follow the context's 115 (FinishCommandList = 114). REPLAY_CBREUSE.
static constexpr int VT1_COPYSUBRESOURCEREGION1 = 115;
static constexpr int VT1_UPDATESUBRESOURCE1     = 116;
typedef void (STDMETHODCALLTYPE* FnDrawAuto)(ID3D11DeviceContext*);
typedef void (STDMETHODCALLTYPE* FnDrawIndirect)(ID3D11DeviceContext*, ID3D11Buffer*, UINT);
typedef void (STDMETHODCALLTYPE* FnCopySubresourceRegion)(ID3D11DeviceContext*, ID3D11Resource*, UINT, UINT, UINT, UINT,
                                                          ID3D11Resource*, UINT, const D3D11_BOX*);
typedef void (STDMETHODCALLTYPE* FnExecuteCommandList)(ID3D11DeviceContext*, ID3D11CommandList*, BOOL);
typedef void (STDMETHODCALLTYPE* FnCopyStructureCount)(ID3D11DeviceContext*, ID3D11Buffer*, UINT, ID3D11UnorderedAccessView*);
typedef void (STDMETHODCALLTYPE* FnCopySubresourceRegion1)(ID3D11DeviceContext1*, ID3D11Resource*, UINT, UINT, UINT, UINT,
                                                           ID3D11Resource*, UINT, const D3D11_BOX*, UINT);
typedef void (STDMETHODCALLTYPE* FnUpdateSubresource1)(ID3D11DeviceContext1*, ID3D11Resource*, UINT, const D3D11_BOX*,
                                                       const void*, UINT, UINT, UINT);
static FnDrawAuto              g_orig_draw_auto = nullptr;
static FnDrawIndirect          g_orig_draw_indexed_inst_indirect = nullptr;
static FnDrawIndirect          g_orig_draw_inst_indirect = nullptr;
static FnCopySubresourceRegion g_orig_copy_region = nullptr;
static FnExecuteCommandList    g_orig_execute_command_list = nullptr;
static FnCopyStructureCount    g_orig_copy_structure_count = nullptr;   // REPLAY_CBREUSE.
static FnCopySubresourceRegion1 g_orig_copy_region1 = nullptr;
static FnUpdateSubresource1    g_orig_update_subresource1 = nullptr;

typedef void (STDMETHODCALLTYPE* FnPSSetShaderResources)(ID3D11DeviceContext*, UINT, UINT, ID3D11ShaderResourceView* const*);
typedef void (STDMETHODCALLTYPE* FnDrawIndexed)(ID3D11DeviceContext*, UINT, UINT, INT);
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
typedef void (STDMETHODCALLTYPE* FnCopyResource)(ID3D11DeviceContext*, ID3D11Resource*, ID3D11Resource*);   // PIP_FX_COPY.

static FnPSSetShaderResources    g_orig_pssetshaderresources = nullptr;
static FnDrawIndexed             g_orig_draw_indexed = nullptr;
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
static FnCopyResource            g_orig_copyresource = nullptr;   // PIP_FX_COPY.
typedef void (STDMETHODCALLTYPE* FnResolveSubresource)(ID3D11DeviceContext*, ID3D11Resource*, UINT, ID3D11Resource*, UINT, DXGI_FORMAT);
static FnResolveSubresource      g_orig_resolvesubresource = nullptr;

// The injection never takes the bridge graphics lock and needs none: the engine never drives the immediate context from
// two threads at once, and our game-thread context use rides inside its hand-over (Present, UI draws) or a park.
static std::atomic<uint32_t> g_reorder_render_tid{0};

inline bool reorder_on_render_thread() {
    const uint32_t tid = g_reorder_render_tid.load(std::memory_order_relaxed);
    return tid != 0 && tid == GetCurrentThreadId();
}

// An exception may not leave a COM vtable call. Every hook wraps its own work (never the forwarded engine call) and
// lands here, on whichever thread ran the hook: the event is reported once and, on the render thread, the injection
// latch is dropped so the next draw is tracked again. The frame it hit draws without us.
// HOOK_EXCEPT_RT - the latch is dropped on the render thread only. The hooks also run on the game thread (its UI
// draws, Present, the parked flush's own draws), where no path of ours raises the flag outside a park but the in-front
// slice's deferred tail (infront_tail_take at the game thread's resolve: infront_inject and VM_SEE's
// brackets, each under a InjectionFlagScope, inside the engine's own call): a plain write from there raced the
// render thread's injection in flight and handed the rest of its draws to the hooks as engine draws. Under a park the
// raise is the parked flush's own, restored by InjectionFlagScope when the flush unwinds. At FSAA 1x the game
// thread is the render thread (reorder_on_render_thread), so an exception a hook catches during the parked flush's own
// draws drops the flag there while that flush still runs: its later draws reach the hooks as the engine's.
static std::atomic<uint64_t> g_hook_exceptions{0};   // Session count; getRenderStats' hookExceptions.
// Called inside a hook's catch handler. HOOK_EXCEPT_TEXT: the exception's own text, and a count -
// reported at the first, the hundredth and the ten-thousandth, since a throw that recurs every frame
// drops the injection every frame and one line per session hid that.
static inline void hook_except() {
    if (reorder_on_render_thread()) g_ro.in_injection = false;
    const uint64_t n = g_hook_exceptions.fetch_add(1, std::memory_order_relaxed) + 1u;
    if (n != 1u && n != 100u && n != 10000u) return;
    try {
        std::string what = "unknown exception";
        try {
            if (std::current_exception()) std::rethrow_exception(std::current_exception());
        } catch (const std::exception& e) {
            what = e.what();
        } catch (...) {}
        report_error_once_safe("KH RenderIntegration: context hooks: exception inside a context hook (" +
                               std::to_string(n) +
                               (n == 1u ? " so far; that frame drew without the injection): "
                                             : " so far; the injection keeps being dropped): ") +
                               what);
    } catch (...) {}
}
}