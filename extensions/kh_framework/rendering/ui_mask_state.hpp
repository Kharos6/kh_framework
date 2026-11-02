#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the UI mask
// (KhUiMask): learning, the clear, the UI draw blends and the quad. Never include it on its own.
namespace RenderIntegration {

// The backbuffer is only ever a color target (never sampled, never copied), so its alpha channel is engine-dead and
// ours to repurpose.
struct KhUiMask {
    // Learned swapchain buffers (written by the UI flush).
    void*    bb_id[4] = {};
    UINT     bb_w[4] = {}, bb_h[4] = {};
    uint32_t bb_n = 0;
    // Phase machine, fed by the backbuffer draws of BOTH context threads (the engine draws its UI from either,
    // serialised on the immediate context; KH_UI_MASK_RT). KH_DDX_UI_LEARN: the quad-clear event (kh_ui_quad_post) is
    // the one clear placement - no timing arm, no learned-shader table.
    int      phase = 0;   // 0 idle, 2 the quad qualified and the clear is pending, 3 cleared (mask valid).
    void*    phase_bb = nullptr;   // The learned entry the current phase locked.
    UINT     phase_w = 0, phase_h = 0;
    bool     mask_valid = false;   // Set with phase 3; cleared at a re-arm, an abort or the UI flush's dark-driver
                                   // test; read by the UI flush.
    uint64_t compose_serial = 0;
    uint64_t applied_serial = 0;
    bool        alpha_copy_pending = false;
};
static KhUiMask g_ui_mask;
// KH_UI_POISON_RUN: consecutive readings of the UI flush's alpha stage (eight 64 x 8 boxes of the backbuffer's alpha,
// copied at one flush and read at a later one) in which every texel reads 255 - the UI covers the whole display; a
// zero of the alpha clear or any partial coverage breaks the run (the HUD alone never covers it: the clear's zero
// stays wherever no widget is drawn). A reading or two is a full-coverage display or a fade opening: on those frames
// PSEffect's full-coverage guard (KH_FX_SIDE: eight points each 0.75 or more) leaves the UI as it is for every builtin
// pass - KH_UI_COV_LIVE: no pass of ours rewrites the alpha, so the guard reads the live coverage - while a .hlsl pass,
// which has no guard, takes the whole display as its coverage. KH_UI_POISON_DEAD in a row is a display where the
// machine's clear placement does not hold (the map: the engine enters the Present body ~30 times per presented frame
// from one call site with plain flags, every entry re-arms the machine and wipes the alpha, and the real Present is
// the last). There the mask is declared dead and no pass draws - no .hlsl pass over the whole display, none of the
// chain's work (a capture and every pass's draws, which the guard would discard) at each re-armed entry - until a
// reading has a texel below 255 again. uiPoisonRun publishes the run.
static constexpr int32_t     KH_UI_POISON_DEAD = 3;
static int32_t               g_ui_poison_run = 0;   // UI-flush thread only.
// KH_UI_DIAG (measured): the UI driver's Draw EH fires OUTSIDE the engine's UI pass (the bound target there is never a
// learned backbuffer); a graphics lock taken from it is granted where the backbuffer IS bound but ahead of the
// engine's own UI composition (the target changes across the wait), which then paints its scene quad over the effect;
// and the runtime's multithread protection is OFF on this engine (the two context threads never overlap; the engine
// hands the context over), so drawing from the EH without a lock collides with the render thread and crashes. Never
// draw the UI chain from the EH without the lock; the EH's lock path (flush_ui_frame) is only the fallback: Present is
// the point after the widgets, and it needs no lock.
// KH_UI_THREADS (measured): the engine draws MOST of its UI pass on the RENDER thread and a few widgets plus Present
// on the game thread, which is why the machine is fed from both context threads (reorder_pre_draw's two calls of
// kh_ui_mask_thread_draw, one per thread branch).
// A build on which that stops holding feeds the machine from one thread and places the alpha clear against the
// other's draws; the symptom is 'some elements, flashing'.
// KH_PRESENT (measured): one swapchain, one calling site, sync 0, and the body IS entered about twice per presented
// HUD frame and about thirty times per map frame. kh_present_cb is written for all of that and counts none of it, so
// a build that presents differently costs correctness. A DXGI_PRESENT_TEST call presents nothing and passes straight
// through (KH_PRESENT_TEST).
static std::atomic<bool> g_ui_mask_wanted{false};   // Any visible UI-mode pass exists.

// KH_PRESENT_RTV: the engine's own render-target view for the frame's UI pass - the object the widgets draw into -
// taken (AddRef'd) at the mask machine's arm and bound at Present for the UI flush. A view the machine made on
// GetBuffer(0) does not serve (measured: ~4 of ~50 widget draws per frame landed on a learned target, the rest on one
// the machine had never learned, and which widgets took the effect changed with the frame and the open display). Not
// because flip-model buffers rotate - in Direct3D 11 the interface GetBuffer(0) returns stays the same across
// Presents, its resource changing behind it (DXGI 1.4 "Invariant backbuffer identity") - the widgets draw into a
// target that is not that object, or a layer stands between; the engine's own view is right either way. Written at
// the arm (EITHER context thread, KH_UI_MASK_RT), read at Present on the presenting thread: safe for the same reason
// as the machine's other state - the engine drives the immediate context from one thread at a time, and Present's
// flush_ui_locked draws on that context. The reference the reader takes (kh_present_cb) only holds the view to the
// bind; it is no cross-thread guard.
// Released with the mask - at the device reset, and ahead of a buffer resize (KH_PRESENT_RESIZE: this reference is
// one of the two that would otherwise fail the engine's ResizeBuffers). KH_UI_FRAME_RTV_GATE: taken only while the
// Present hook is installed (its one reader is kh_present_cb), so a session without the ResizeBuffers hook because
// the probe swap chain failed (both hooks come from it; KH_PRESENT_RETRY's windows too) holds none.
static ID3D11RenderTargetView* g_ui_frame_rtv = nullptr;
static uint64_t                g_ui_frame_rtv_seq = 0;   // The present sequence it was taken in.
// KH_UI_QUAD_CLEAR - the coverage mask's alpha clear is placed by an event (a timing rule cannot place it: the two
// threads' draws are not ordered against a clock). The engine's last opaque full-screen backbuffer draw - the
// tonemapped scene quad - precedes the UI, so the clear runs in that draw's post-draw hook, on whichever thread issued
// it, into the still-bound target; every later widget then writes coverage. Opaque = the blend state does not blend;
// full screen = viewport at the origin covering 90% of the learned backbuffer; no depth view. An opaque full-screen UI
// layer qualifies too and re-places the clear after itself (KH_UI_QUAD_OPAQUE): its pixels count as scene, and only
// what is drawn over it as UI. A blended full-screen layer at alpha 1 reads 255: PSEffect's full-coverage guard and
// KH_UI_POISON_RUN act.
static thread_local ID3D11DeviceContext* g_uiq_ctx = nullptr;   // Pending: this thread's last draw qualified.
static thread_local void*                g_uiq_id = nullptr;
static thread_local UINT                 g_uiq_w = 0, g_uiq_h = 0;
}