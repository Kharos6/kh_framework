#pragma once

// The injection half (rendering_integration_injection.hpp): FX_LATE, the late effect chain. Never include it on its
// own.
namespace RenderIntegration {

// RENDER_FLUSH: the flush body, defined with the flush (rendering/flush.hpp).
// chain_only (FX_LATE): run the fullscreen scene chain only.
inline void flush_locked(ID3D11Device* dev, ID3D11DeviceContext* ctx, bool rt, bool chain_only = false);

// Defined in rendering/depth_identity.hpp (declared here for the uses below).
inline bool main_depth_dims_match(ID3D11DepthStencilView* dsv);


// FX_LATE: the chain owed to a later resolve (past the view model), what ran, the dry and miss cycles, the flap
// backoff.
// Threads: the render thread's scene flush owes the chain; fx_late_gt runs it at a later resolve - the render
// thread's, or the game thread's inside the engine's hand-over: every member is an atomic.
// Session reset: session_globals_reset resets every member.
struct FxLateState {
    // FX_LATE - flush_locked runs at the render thread's scene resolve, before the view model is drawn, so the
    // fullscreen scene chain there would miss the hands and weapon. A later resolve of the scene target (render thread
    // or game thread) comes after it. The chain moves there when the flush's entry test passed at a late resolve on the
    // previous cycle (FX_LATE_STREAK): the scene flush leaves the chain owed, a later resolve runs it through a
    // chain-only flush, and one that fails the test leaves it owed (FX_LATE_KEEP). A cycle whose owed chain never
    // ran is a miss and loses that frame's effects. A flapping site keeps the chain on the scene flush for
    // FX_LATE_BACKOFF cycles: a miss after fewer than FX_LATE_FLAP passing cycles, or the third miss within
    // FX_LATE_FLAP_WINDOW. The UI passes are not this path's (flush_ui_locked, at Present). VM_SEE: past the view
    // model, the chain's depth readers see the scene behind its translucent planes (a 3D scope's eyepiece writes its
    // own near depth over the aperture) - vmsee_arm.
    std::atomic<uint64_t> owed_cycle{ ~0ull };  // Chain the render thread left owed to a later resolve.
    std::atomic<uint64_t> ran_cycle{ ~0ull };   // A chain-only flush ran the chain in this cycle.
    std::atomic<uint64_t> dry_cycle{ ~0ull };   // The last cycle whose late scene resolve passed the entry test.
    std::atomic<uint32_t> streak{ 0 };
    // A cycle stamp: g_topo_cycles restarts with the session, so the session reset must clear it - a stale backoff
    // would hold the chain on the scene flush for the whole session.
    std::atomic<uint64_t> backoff{ 0 };
    std::atomic<uint64_t> miss_cycle{ ~0ull }, miss_cycle2{ ~0ull };   // The last two misses' cycles (newest first).
};
static FxLateState g_fx_late;
static constexpr uint32_t FX_LATE_STREAK = 1u;   // Consecutive passing cycles before the chain moves.
static constexpr uint64_t FX_LATE_BACKOFF = 600u;
static constexpr uint32_t FX_LATE_FLAP = 30u;   // A miss after a shorter passing run backs off.
static constexpr uint64_t FX_LATE_FLAP_WINDOW = 300u;   // Three misses inside it back off.
inline void fx_late_miss(uint64_t c) {
    // Passing cycles ahead of the missed one.
    const uint32_t run = g_fx_late.streak.load(std::memory_order_relaxed);
    g_fx_late.streak.store(0, std::memory_order_relaxed);
    const uint64_t old = g_fx_late.miss_cycle2.load(std::memory_order_relaxed);
    const bool third = old != ~0ull && c >= old && c - old <= FX_LATE_FLAP_WINDOW;
    if (run < FX_LATE_FLAP || third) {
        g_fx_late.backoff.store(c + FX_LATE_BACKOFF, std::memory_order_relaxed);
    }
    g_fx_late.miss_cycle2.store(g_fx_late.miss_cycle.load(std::memory_order_relaxed), std::memory_order_relaxed);
    g_fx_late.miss_cycle.store(c, std::memory_order_relaxed);
}
// Render thread, flush_locked's chain site: true = leave this cycle's chain to a later resolve.
inline bool fx_late_take() {
    const uint64_t c = g_topo_cycles;
    const uint64_t owed = g_fx_late.owed_cycle.load(std::memory_order_relaxed);
    // An owed chain runs only in its own cycle (fx_late_gt), so any older
    // one that never ran is a miss, however many cycles back.
    if (owed != ~0ull && owed < c && g_fx_late.ran_cycle.load(std::memory_order_relaxed) != owed) {
        g_fx_late.owed_cycle.store(~0ull, std::memory_order_relaxed);
        fx_late_miss(c);   // An earlier cycle's owed chain never ran.
    }
    if (c < g_fx_late.backoff.load(std::memory_order_relaxed)) return false;
    const uint64_t dry = g_fx_late.dry_cycle.load(std::memory_order_relaxed);
    if (g_fx_late.streak.load(std::memory_order_relaxed) < FX_LATE_STREAK || dry == ~0ull ||
        dry + 1u != c) return false;
    g_fx_late.owed_cycle.store(c, std::memory_order_relaxed);
    return true;
}
inline void fx_late_gt(ID3D11DeviceContext* ctx, bool rt = false) {
    const uint64_t c = rt ? g_topo_cycles : g_topo_cycles_pub.load(std::memory_order_relaxed);
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    bool ok = false;
    if (dsv) {
        ok = g_main_depth_identity != nullptr && reorder_dsv_identity(dsv) == g_main_depth_identity &&
             main_depth_dims_match(dsv);
        dsv->Release();
    }
    if (ok) {
        const uint64_t prev = g_fx_late.dry_cycle.load(std::memory_order_relaxed);
        if (prev != c) {
            const bool consec = prev != ~0ull && prev + 1u == c;
            g_fx_late.streak.store(consec ? g_fx_late.streak.load(std::memory_order_relaxed) + 1u : 1u,
                std::memory_order_relaxed);
            g_fx_late.dry_cycle.store(c, std::memory_order_relaxed);
        }
    } else {
        g_fx_late.streak.store(0, std::memory_order_relaxed);
    }
    if (g_fx_late.owed_cycle.load(std::memory_order_relaxed) != c) return;
    ID3D11Device* const dev = RVExtBridge::get_d3d_device();
    // FX_LATE_KEEP: a resolve that cannot take the chain leaves it owed for
    // a later resolve of the cycle; a cycle none takes is the next take's miss.
    if (!ok || !dev) return;
    g_fx_late.owed_cycle.store(~0ull, std::memory_order_relaxed);
    {
        FlushActiveScope active;   // Our draws stay out of every thread's hooks (the flag is process-wide).
        const bool pinj = g_ro.in_injection;
        if (rt) g_ro.in_injection = true;   // The render thread's hooks stand aside, as for its flush.
        flush_locked(dev, ctx, true, true);
        if (rt) g_ro.in_injection = pinj;
    }
    if (g_fx_late.ran_cycle.load(std::memory_order_relaxed) != c) fx_late_miss(c);
}
}