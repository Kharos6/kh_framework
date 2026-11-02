#pragma once

// The injection half (rendering_integration_injection.hpp): KH_FX_LATE, the late effect chain. Never include it on its
// own.
namespace RenderIntegration {

// KH_RENDER_FLUSH: the flush body, defined with the flush (rendering/flush.hpp).
// khfl_chain_only (KH_FX_LATE): run the fullscreen scene chain only.
inline void flush_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool khfl_rt, bool khfl_chain_only = false);

// Defined in rendering/depth_identity.hpp (declared here for the uses below).
inline bool kh_main_depth_dims_match(ID3D11DepthStencilView* dsv);


// KH_FX_LATE - flush_locked runs at the render thread's scene resolve, before the view model is drawn, so the
// fullscreen scene chain there would miss the hands and weapon. A later resolve of the scene target (render thread or
// game thread) comes after it. The chain moves there when the flush's entry test passed at a late resolve on the
// previous cycle (KH_FX_LATE_STREAK): the scene flush leaves the chain owed, a later resolve runs it through a
// chain-only flush, and one that fails the test leaves it owed (KH_FX_LATE_KEEP). A cycle whose owed chain never ran is
// a miss and loses that frame's effects. A flapping site keeps the chain on the scene flush for KH_FX_LATE_BACKOFF
// cycles: a miss after fewer than KH_FX_LATE_FLAP passing cycles, or the third miss within KH_FX_LATE_FLAP_WINDOW. The
// UI passes are not this path's (flush_ui_locked, at Present). KH_VM_SEE: past the view model, the chain's depth
// readers see the scene behind its translucent planes (a 3D scope's eyepiece writes its own near depth over the
// aperture) - kh_vmsee_arm.
static std::atomic<uint64_t> g_fx_late_owed_cycle{ ~0ull };  // Chain the render thread left owed to a later resolve.
static std::atomic<uint64_t> g_fx_late_ran_cycle{ ~0ull };   // A chain-only flush ran the chain in this cycle.
static std::atomic<uint64_t> g_fx_late_dry_cycle{ ~0ull };   // The last cycle whose late scene resolve passed the entry test.
static std::atomic<uint32_t> g_fx_late_streak{ 0 };
static std::atomic<uint64_t> g_fx_late_backoff{ 0 };
static constexpr uint32_t KH_FX_LATE_STREAK = 1u;   // Consecutive passing cycles before the chain moves.
static constexpr uint64_t KH_FX_LATE_BACKOFF = 600u;
static constexpr uint32_t KH_FX_LATE_FLAP = 30u;   // A miss after a shorter passing run backs off.
static constexpr uint64_t KH_FX_LATE_FLAP_WINDOW = 300u;   // Three misses inside it back off.
static std::atomic<uint64_t> g_fx_late_miss_cycle{ ~0ull }, g_fx_late_miss_cycle2{ ~0ull };   // The last two misses' cycles (newest first).
inline void kh_fx_late_miss(uint64_t khfm_c) {
    const uint32_t khfm_run = g_fx_late_streak.load(std::memory_order_relaxed);   // Passing cycles ahead of the missed one.
    g_fx_late_streak.store(0, std::memory_order_relaxed);
    const uint64_t khfm_old = g_fx_late_miss_cycle2.load(std::memory_order_relaxed);
    const bool khfm_third = khfm_old != ~0ull && khfm_c >= khfm_old && khfm_c - khfm_old <= KH_FX_LATE_FLAP_WINDOW;
    if (khfm_run < KH_FX_LATE_FLAP || khfm_third) {
        g_fx_late_backoff.store(khfm_c + KH_FX_LATE_BACKOFF, std::memory_order_relaxed);
    }
    g_fx_late_miss_cycle2.store(g_fx_late_miss_cycle.load(std::memory_order_relaxed), std::memory_order_relaxed);
    g_fx_late_miss_cycle.store(khfm_c, std::memory_order_relaxed);
}
// Render thread, flush_locked's chain site: true = leave this cycle's chain to a later resolve.
inline bool kh_fx_late_take() {
    const uint64_t khft_c = g_topo_cycles;
    const uint64_t khft_owed = g_fx_late_owed_cycle.load(std::memory_order_relaxed);
    // An owed chain runs only in its own cycle (kh_fx_late_gt), so any older
    // one that never ran is a miss, however many cycles back.
    if (khft_owed != ~0ull && khft_owed < khft_c && g_fx_late_ran_cycle.load(std::memory_order_relaxed) != khft_owed) {
        g_fx_late_owed_cycle.store(~0ull, std::memory_order_relaxed);
        kh_fx_late_miss(khft_c);   // An earlier cycle's owed chain never ran.
    }
    if (khft_c < g_fx_late_backoff.load(std::memory_order_relaxed)) return false;
    const uint64_t khft_dry = g_fx_late_dry_cycle.load(std::memory_order_relaxed);
    if (g_fx_late_streak.load(std::memory_order_relaxed) < KH_FX_LATE_STREAK || khft_dry == ~0ull || khft_dry + 1u != khft_c) return false;
    g_fx_late_owed_cycle.store(khft_c, std::memory_order_relaxed);
    return true;
}
inline void kh_fx_late_gt(ID3D11DeviceContext* khfg_ctx, bool khfg_rt = false) {
    const uint64_t khfg_c = khfg_rt ? g_topo_cycles : g_topo_cycles_pub.load(std::memory_order_relaxed);
    ID3D11DepthStencilView* khfg_dsv = nullptr;
    khfg_ctx->OMGetRenderTargets(0, nullptr, &khfg_dsv);
    bool khfg_ok = false;
    if (khfg_dsv) {
        khfg_ok = g_main_depth_identity != nullptr && reorder_dsv_identity(khfg_dsv) == g_main_depth_identity &&
                  kh_main_depth_dims_match(khfg_dsv);
        khfg_dsv->Release();
    }
    if (khfg_ok) {
        const uint64_t khfg_prev = g_fx_late_dry_cycle.load(std::memory_order_relaxed);
        if (khfg_prev != khfg_c) {
            const bool khfg_consec = khfg_prev != ~0ull && khfg_prev + 1u == khfg_c;
            g_fx_late_streak.store(khfg_consec ? g_fx_late_streak.load(std::memory_order_relaxed) + 1u : 1u, std::memory_order_relaxed);
            g_fx_late_dry_cycle.store(khfg_c, std::memory_order_relaxed);
        }
    } else {
        g_fx_late_streak.store(0, std::memory_order_relaxed);
    }
    if (g_fx_late_owed_cycle.load(std::memory_order_relaxed) != khfg_c) return;
    ID3D11Device* const khfg_dev = RVExtBridge::get_d3d_device();
    // KH_FX_LATE_KEEP: a resolve that cannot take the chain leaves it owed for
    // a later resolve of the cycle; a cycle none takes is the next take's miss.
    if (!khfg_ok || !khfg_dev) return;
    g_fx_late_owed_cycle.store(~0ull, std::memory_order_relaxed);
    {
        KhFlushActiveScope khfg_active;   // Our draws stay out of every thread's hooks (the flag is process-wide).
        const bool khfg_pinj = g_ro.in_injection;
        if (khfg_rt) g_ro.in_injection = true;   // The render thread's hooks stand aside, as for its flush.
        flush_locked(khfg_dev, khfg_ctx, true, true);
        if (khfg_rt) g_ro.in_injection = khfg_pinj;
    }
    if (g_fx_late_ran_cycle.load(std::memory_order_relaxed) != khfg_c) kh_fx_late_miss(khfg_c);
}
}