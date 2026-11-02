#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the Present
// hook and the overlay bracket. Never include it on its own.
namespace RenderIntegration {
// KH_PRESENT_UI: the UI chain is drawn from a hooked IDXGISwapChain::Present - after the engine's widgets and before
// the flip, on the thread that presents, which owns the immediate context exclusively at that call (the runtime
// forbids concurrent use; the engine's own Present would otherwise be UB). KH_UI_DIAG says why not the Draw EH. The
// hook is found through a throwaway swapchain on a hidden window (a vtable is per class, so its Present slot is the
// game's) and handed to framework.hpp's one Present detour (KH_SHARED_PRESENT: kh_present_hook, then
// kh_present_subscribe - nothing of ours MinHooks Present itself). The subscription is never withdrawn; it passes
// through whenever the demand is off. flush_ui_locked runs unchanged inside it, with an RTV bound under a KhOmSave
// (it draws into whatever RT0 is bound): the engine's own UI view (KH_PRESENT_RTV) or, without one taken this frame,
// one on the swapchain's buffer 0. While this path is live (a Present within KH_PRESENT_UI_STALE_MS drew the chain)
// the driver's EH takes no lock; otherwise it falls back to the lock path (flush_ui_frame).
static constexpr uint64_t     KH_PRESENT_UI_STALE_MS = 250;
static std::atomic<bool>      g_present_hook_active{ false };
// g_present_hook_failed: game thread, per PROCESS - no reset clears it. MinHook refusing the slot latches it at
// once (a later mission would fail the same way); a probe swapchain that could not be made is asked again
// (KH_PRESENT_RETRY) and latches only at its third failure.
static bool                   g_present_hook_failed = false;
// KH_PRESENT_RETRY: the probe swapchain's failures can pass (DXGI_ERROR_DEVICE_REMOVED in a TDR, E_OUTOFMEMORY), and a
// failed attempt returns before the ResizeBuffers hook too (both come from the probe), so a failure is asked again 1 s
// and then 10 s later before it latches. Game thread, per process, as g_present_hook_failed.
static uint32_t               g_present_hook_fail_n = 0;
static uint64_t               g_present_hook_retry_ms = 0;   // Steady deadline of the next attempt (0 = none).
static std::atomic<uint64_t>  g_present_ui_ms{ 0 };   // Steady ms of the last Present that drew the UI chain.
// KH_PRESENT_SEQ: presented frames, counted at every real (non-TEST) Present of the game's swapchain. While the
// Present path is live this is the UI flush's frame clock (kh_ui_frame_ordinal: its dark-driver test) in place of the
// game thread's flush ordinal, which ticks about twice per presented frame once the Draw3D park no longer holds the
// two in step (measured); it also stamps g_ui_frame_rtv (the frame it was taken in). The machine's clear has no clock:
// it is placed by an event (KH_UI_QUAD_CLEAR).
static std::atomic<uint64_t>  g_present_seq{ 0 };
// RTVs on the swapchain's buffers, by weak identity (device state). Each holds a reference on the buffer it views, so
// the ring is released ahead of any resize as well as at the device reset (KH_PRESENT_RESIZE).
static ID3D11RenderTargetView* g_present_rtv[4] = {};
static void*                   g_present_rtv_id[4] = {};
static uint32_t                g_present_rtv_next = 0;
// KH_OVERLAY_BRACKET: another module's present-time draws on the game's context (framework.hpp's dispatcher brackets
// them) are excluded from our hooks as kh_present_cb's own are: g_ui_mask_injecting, and on the render thread
// g_ro.in_injection. The token: bit 0 = the render thread's flag was raised, bit 1 = its previous value, bit 2 =
// nothing raised (KH_OVERLAY_FOREIGN: the Present is a foreign swap chain's - kh_present_cb, which the dispatcher runs
// first, noted it on this thread - so a tool presenting its own chain from a thread of its own does not flip the flags
// under the render thread's hooks; the other subscriber draws nothing on a foreign chain either. Until
// ensure_present_hook subscribes kh_present_cb - a UI pass of ours exists - nothing writes the verdict and the bracket
// always raises the flags).
static thread_local bool t_kh_present_foreign = false;
// KH_PRESENT_RESIZE: the renderer's one swapchain hook. ResizeBuffers fails with DXGI_ERROR_INVALID_CALL while ANY
// reference on a swapchain buffer is outstanding, and two caches of ours hold one: the Present RTV ring (g_present_rtv,
// views we create on GetBuffer(0)) and the mask machine's captured frame view (g_ui_frame_rtv, the engine's own UI
// render target, taken at the arm). The engine's reset hook releases both, but nothing orders that hook AHEAD of this
// call - measured in game: a 4K -> 1080p change left the swapchain at its old size with the scene in the top-left
// quarter and the rest black, the effect returning seconds later when the reset finally ran. Releasing in
// hooked_resizebuffers (rendering/present_hook.hpp) is ahead of the resize by construction, which is the only thing
// that closes it. A fullscreen toggle is covered by the same hook: whatever path changes the buffer size comes through
// ResizeBuffers. Nothing is re-created there - the ring refills on the next Present (its key is the buffer pointer, and
// the new buffers are new objects) and the mask re-arms on the next UI pass.
typedef HRESULT (STDMETHODCALLTYPE* KhResizeBuffersFn)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
static KhResizeBuffersFn g_orig_resize_buffers = nullptr;
}