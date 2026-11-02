#pragma once

// The shared head (rendering_integration.hpp): KH_WSEE's shared cycle state (the see-through record's arm, cycle and
// hold flags, KH_WSEE_BUILD). Never include it on its own.
namespace RenderIntegration {
// KH_WSEE - the see-through record's globals (the KH_WSEE note at g_wsee_sv_rtvs in rendering/see_through_shaders.hpp);
// the chain (rendering/post_chain.hpp) and the flush read them. The want: stamped with its cycle by a chain run in
// which some pass would take a mode, valid capture or not (kh_fx_chain_run - any thread: the late chain-only flush may
// be the game thread's), and by the scene flush that leaves such a chain to KH_FX_LATE (kh_wsee_arm). The landed and
// tainted cycles: written by the render thread's hooks and by a flush's own depth writes (a park's too), read by the
// chain against the published cycle (kh_wsee_valid). Render thread alone: the attempt stamp, the cycle's verdict (taken
// at the main depth's clear, kh_wsee_cycle_note) and the pending re-issue.
// KH_WSEE_ON: the whole system, chosen when the extension is compiled (no script switch). KH_WSEE_BUILD 0 builds it
// out: no capture, no record targets, no CreatePixelShader hook or blob store, no per-clear see-through work and no
// work in the hooks beyond the early-out tests of the Zb guard's (kh_psb_zb_draw / _dss / _clear / _dst / _list: a
// context compare and a relaxed load each; _dss a store per OMSetDepthStencilState, KH_PSB_ZB_DSS_ARMED), XDRAW's
// (kh_wsee_xdraw), the pre-draw's g_wsee_on test and the four core draw hooks' post-side g_wsee_pending compare (a
// byte never set); per flush one kh_wsee_sort_mode call where a translucent or deferred part exists (0 at its first
// test); every chain pass's KH_WSEE lanes 0 and t53 - t69 as the engine left them (no bind: KH_WSEE_BIND_ARMED).
// Tested with `if constexpr` where a bare constant test would be one, and a function that does nothing with it out
// wraps its whole body in one (no code left unreachable: MSVC C4127 / C4702).
#ifndef KH_WSEE_BUILD
#define KH_WSEE_BUILD 1
#endif
static constexpr bool KH_WSEE_ON = KH_WSEE_BUILD != 0;
static std::atomic<uint64_t> g_wsee_want_cycle{ ~0ull };    // The newest cycle a chain wanted the capture in.
static std::atomic<uint64_t> g_wsee_ok_cycle{ ~0ull };      // The cycle whose capture (S, B, T and NW cleared) landed.
static std::atomic<uint64_t> g_wsee_taint_cycle{ ~0ull };   // The cycle whose capture a later write spoiled.
// KH_WSEE_BWANT: the newest cycle a reader of the scene colours B / B2 wanted them - a chain run with an armed
// distance fog or user effect (kh_wsee_col_reader), the flush's KH_FX_LATE take of one (kh_wsee_arm), the flush's
// sorted parts compositing (modes 2 / 3) - read by the capture as its own want is (KH_WSEE_WANT_SPAN); and the cycle
// whose capture resolved B (its B2 moment resolves B2 then, and only then).
static std::atomic<uint64_t> g_wsee_col_want_cycle{ ~0ull };
static std::atomic<uint64_t> g_wsee_col_cycle{ ~0ull };
// This cycle's layer-depth verdict (kh_wsee_cycle_note: the split not refused - wsee_ld_failed; render thread).
static bool g_wsee_cyc_layer = false;
// The cycle whose Tp / Tf the capture cleared (the chain binds them for that cycle alone; the late chain may be the
// game thread's), and the far plane's raw D its Tf depth was cleared to (render thread; false = none: no Tf replay
// this cycle - Tf stays 1).
static std::atomic<uint64_t> g_wsee_ld_cycle{ ~0ull };
static bool  g_wsee_ld_far_ok = false;
static float g_wsee_ld_far_lo = 0.0f, g_wsee_ld_far_hi = 1.0f;   // The world range D's raw was computed in.
// KH_PSB: the one pass's record - the cycle whose capture cleared it, the cycle a re-issue drew into it (the chain's
// and the sort's lane bit 6: its loads), the cycle a particle-type layer went into the old record instead (a re-issue
// the one pass could not take, our own tail; bit 7 is its complement: the old record's NW / Tp / Tf hold their clears
// and are not read), and D's raw the Tf verdict compares with (render thread writes; the late chain may read the
// cycles on the game thread).
static std::atomic<uint64_t> g_psb_cycle{ ~0ull };
static std::atomic<uint64_t> g_psb_drew_cycle{ ~0ull };
// KH_PSB_LORES: log2 of the one pass's scale s = 2 - P0 / P1 / N / N2 made at 1 / s of the frame a side and the one
// pass drawn into them at that scale (Resources' KH_PSB_LORES note). TWIN of cb.hlsl's KH_WSEE_PSB_LS (the readers'
// texel px >> it) and of PSPsbZb's KH_PSB_ZB_LS (KH_PSB_ZB_DEFS below); the static_assert pins the value (the patcher,
// kh_psb_patch, keeps an s = 1 arm no caller takes).
static constexpr UINT KH_PSB_LS = 1u;
static_assert(KH_PSB_LS == 1u, "KH_PSB_LS is the twin of KH_WSEE_PSB_LS / KH_PSB_ZB_LS (s = 2)");
// KH_PSB_ZB: the capture builds Zb (the block's largest S and where it lies) and the one pass's loop reads the live
// depth only for the verdicts Zb leaves open. g_psb_zb_cycle: the cycle whose capture built Zb, while its guard
// stands (kh_psb_zb_trip sets ~0: any write that could leave the main depth farther than S since the capture - the
// guard's note in rendering/see_through.hpp); g_psb_zb_gen: the depth view's generation it was read through
// (Resources' depth_srv_gen); g_psb_zb_far: the depth state bound on the engine's context (any caller's set while Zb
// stands - kh_psb_zb_dss) can write the depth farther (GREATER, GREATER_EQUAL, NOT_EQUAL or ALWAYS with a write) -
// true where unknown (Zb not standing or just built: the next draw looks at the live state). Render thread.
static std::atomic<uint64_t> g_psb_zb_cycle{ ~0ull };
static uint32_t g_psb_zb_gen = 0u;
static bool g_psb_zb_far = true;
// KH_PSB_ZB: PSPsbZb's defines at log2 s = KH_PSB_LS (kh_wsee_ensure's compile and the prewarm table's: one cache
// key).
static const D3D_SHADER_MACRO KH_PSB_ZB_DEFS[3] = {
    { "MSAA_DEPTH", "0" }, { "KH_PSB_ZB_LS", "1" }, { nullptr, nullptr } };
static std::atomic<uint64_t> g_psa_cycle{ ~0ull };   // KH_PSB: the old record's stamp (the note at g_psb_cycle).
// KH_PSB_TPA: the cycle a layer went into the old record's Tp (kh_wsee_post's Tp passes, kh_wsee_tail's) - the
// late-opaque replays reset Tp from then on (kh_wsee_re_set's khwb_l); before it Tp holds its clear, which the reset
// would write again. g_psa_cycle stays the whole old record's stamp (lane bit 7): a light-adding NW-only layer sets it
// and writes no Tp. Render thread (the tail's: a park that stalls it).
static std::atomic<uint64_t> g_psa_tp_cycle{ ~0ull };
// KH_WSEE_NOPRE: a late opaque marked before the cycle's B2 attempt ("pre") is not replayed (kh_wsee_mark_opaque) and
// the shaders derive what its replay would have written (cb.hlsl's note at khWsS2). g_wsee_nopre_cycle: the cycle a
// pre mark was skipped in (lane bit 8; and D3's verdict, kh_wsee_d3_ok); g_wsee_s2_cycle: the cycle S2 was taken in
// (kh_wsee_b2_take - S2 bound at t62 with lane bit 9). Render thread writes; the chain (the late one may be the game
// thread's) and the sort scope read them.
static std::atomic<uint64_t> g_wsee_nopre_cycle{ ~0ull };
static std::atomic<uint64_t> g_wsee_s2_cycle{ ~0ull };
static float g_psb_far_raw = 1.0f;   // KH_PSB: D's raw the Tf verdict compares with (the note at g_psb_cycle).
// The late-opaque replay keeps each late opaque's own depth, D3, and the shaders take it as the behind of such a
// surface in place of the live depth (a fence-type layer drawn over it later held that). The capture stamps
// g_wsee_od_cycle, and the chain binds D3 at t65 with bit 5 of fx_tex[1].w for that cycle alone (the late chain may
// be the game thread's).
static std::atomic<uint64_t> g_wsee_od_cycle{ ~0ull };
// KH_WSEE_OQ: a pre late opaque is skipped (g_wsee_nopre_cycle), so D3 holds no depth of it: g_wsee_d3_sync_cycle is
// the cycle S2 was taken in after such a skip (kh_wsee_b2_take), from which the readers take S2 itself there. Render
// thread writes; the chain (the late one may be the game thread's) and the sort scope read them through
// kh_wsee_d3_ok.
static std::atomic<uint64_t> g_wsee_d3_sync_cycle{ ~0ull };
// KH_WSEE_OQ2: the cycle a post late opaque last wrote D3m in since D3's last resolve (kh_wsee_re_set sets it,
// kh_wsee_d3m_resolve clears it to ~0 once drawn): a reader resolves D3 only where it is the reader's cycle. Render
// thread writes; the chain (the late one may be the game thread's, inside a resolve the engine issues there: the
// engine never drives the immediate context from two threads at once) and the sort scope read and clear it.
static std::atomic<uint64_t> g_wsee_d3m_cycle{ ~0ull };
// KH_WSEE_OQ: D3 stands complete for cycle khdo_c - its capture filled it (g_wsee_od_cycle) and either no pre opaque
// was skipped or S2 was taken after them. Else its readers leave t65 null and bit 5 clear: the behind of a late opaque
// is the live depth - the same picture at sample 0 wherever no fence-type layer drew after a late opaque (a reader
// before the B2 attempt: the flush's sort scope), samples k > 0 taking their own depth in place of sample 0's D3;
// S2's pass refused at the attempt leaves that cycle so.
inline bool kh_wsee_d3_ok(uint64_t khdo_c) {
    return g_wsee_od_cycle.load(std::memory_order_relaxed) == khdo_c &&
           (g_wsee_nopre_cycle.load(std::memory_order_relaxed) != khdo_c ||
            g_wsee_d3_sync_cycle.load(std::memory_order_relaxed) == khdo_c);
}
// The injection's translucent parts left to the flush's tail (the KH_WSEE note's SORT paragraph): copies of the
// objects as the injection drew them (the transform, size and colour the solid took; draw_part 1), and the cycle they
// belong to. Written by the injection (kh_wsee_defer_note), taken whole by the cycle's first scene flush that builds
// its mesh list (flush_locked: drawn first in its order where it stands down for the injection's meshes, dropped where
// it draws them itself - their parts with them), dropped unconsumed at the next main-depth clear (kh_wsee_cycle_note).
// g_wsee_defer_taken / g_wsee_defer_drawn: the cycle whose flush appended them and the cycle whose flush then reached
// its draw loop - a flush that bails between the two counts as a loss; a loss (a tainted deferred cycle too,
// KH_WSEE_DEFER_TAINT) stands the deferral down until g_wsee_defer_back (KH_WSEE_DEFER_BACKOFF cycles: a persistent
// cause would otherwise lose our parts every frame). The render thread's, or a park's that stalls it; reset with the
// session's scratch (the copies hold material-set references until then).
static uint64_t g_wsee_defer_cycle = ~0ull;
static std::vector<RenderObject> g_wsee_defer_objs;
static uint64_t g_wsee_defer_taken = ~0ull;
static uint64_t g_wsee_defer_drawn = ~0ull;
static uint64_t g_wsee_defer_back = 0;
// The cycle's geometry hold: one entry per deferred copy (KH_WSEE_OWN_STATIC). own: an object drawn from its mesh's
// own buffer at the injection (static, or a pose or a stage not yet installed), or one with nothing to draw at its
// levels - no copy, the flush binds that buffer whatever the slot holds by then; else (a skinned pose - its own buffer
// or a skin pool chunk's slice -, a cloth's or a chain's buffer, a vertex stage's output) the vertices the injection
// drew with, as they stood then: its copy in g_res.wsee_hold_vb (kh_wsee_hold_take) - only the vertex runs the flush
// can draw, the translucent submeshes at the injection's level and, crossfading, the next (lvl, t: the injection's
// pick, which the flush then takes) - each at its own place relative to base (the IA offset the flush binds: vertex
// v at base + 48 v, so the mesh's absolute indices address the copy unchanged), within [off, off + bytes). gen: the
// hold buffer's generation at the copy (a release since: the entry is void). Beside g_wsee_defer_objs (the copies'
// ws_hold index it, 1-based), but cleared only by the cycle's first hold (kh_wsee_hold_begin) and the session: the
// flush reads it while it draws, after it took the copies; g_wsee_hold_used: the bytes this cycle's entries take.
// Render thread, or a park's flush that stalls it.
struct KhWseeHold {
    int      mesh = -1;
    bool     own = false;
    UINT     base = 0u;
    UINT     off = 0u;
    UINT     bytes = 0u;
    uint64_t gen = 0u;
    int      lvl = 0;
    float    t = 0.0f;
};
static std::vector<KhWseeHold> g_wsee_hold;
static UINT g_wsee_hold_used = 0u;
static uint64_t g_wsee_hold_cycle = ~0ull;   // The cycle g_wsee_hold's entries belong to (kh_wsee_hold_begin).
// KH_WSEE_HOLD_IDLE_COPY: the last cycle a COPY entry used the hold buffer or its run caches (kh_wsee_hold_take, a
// refused group that built caches) - the idle release's stamp. An own entry (the mesh's own buffer) stamps nothing:
// static parts deferring every cycle must not keep a buffer an earlier crowd grew.
static uint64_t g_wsee_hold_copy_cycle = ~0ull;
// The cycle whose chain run bound its capture (kh_fx_chain_run; exactly one chain a cycle). Past it nothing of the
// cycle reaches the picture: the pre-draw stands down and a taint is moot (kh_wsee_taint ignores it, so a late write
// - an engine pass on the scene after the chain - cannot count the used capture as waste).
static std::atomic<uint64_t> g_wsee_used_cycle{ ~0ull };
static uint64_t g_wsee_snap_cycle = ~0ull;   // The cycle whose capture was attempted - once, landed or not.
static bool     g_wsee_on = false;   // This cycle captures: KH_WSEE_ON, a want <= WANT_SPAN old, no back-off and
                                    // no KH_WSEE_FLUSHW_PRED stand-down.
// This draw's re-issue (the pre-draw to its hook's post; kh_wsee_post): none, a blended depth-writer's (T), a
// blended no-write draw's (T, then NW) or a non-blended depth-writer's after the capture (T's late-opaque flag).
static constexpr uint8_t KH_WSEE_RE_NONE = 0u, KH_WSEE_RE_BLEND = 1u, KH_WSEE_RE_NOWRITE = 2u, KH_WSEE_RE_OPAQUE = 3u;
// A light-adding blended draw writing no depth (DestBlend ONE), into NW alone; an over-blend writing no depth with
// the depth test off, into T without a test, then NW (kh_wsee_pre_draw's note).
static constexpr uint8_t KH_WSEE_RE_NWONLY = 4u, KH_WSEE_RE_NOWRITE_OFF = 5u;
static uint8_t  g_wsee_pending = KH_WSEE_RE_NONE;
// The pending particle-type re-issue is drawn after this cycle's B2 (Tpost / NWpost), else before (Tpre / NWpre) -
// kh_wsee_pre_draw's verdict for its post; a pending late opaque is always post (kh_wsee_mark_opaque: a pre one is
// not replayed). Render thread.
static bool     g_wsee_pending_post = false;
// The cycle whose second colour snapshot B2 landed (taken at its first fence-type layer, kh_wsee_b2_take; the chain
// reads it - atomic: the late chain may be the game thread's) and the cycle B2 was attempted in (once).
static std::atomic<uint64_t> g_wsee_b2_cycle{ ~0ull };
static uint64_t g_wsee_b2_try = ~0ull;
static constexpr uint64_t KH_WSEE_WANT_SPAN = 2u;
// KH_WSEE_HOLD_FAIL_RETRY: cycles a refused make stays refused at its key before one more ask - the hold's (its
// size: kh_wsee_hold_reserve) and, KH_WSEE_FAIL_RETRY, the capture targets' (kh_wsee_targets, kh_wsee_ld_targets,
// kh_psb_targets, kh_wsee_d3m_targets; Resources' note at wsee_fail_cycle).
static constexpr uint64_t KH_WSEE_HOLD_FAIL_RETRY = 600u;
// The cycle's capture would lie (the KH_WSEE note's TAINT list: a depth write of ours on the main depth after it
// landed, a re-issue refused or impossible, a DrawAuto / indirect engine draw writing the main depth - KH_WSEE_XDRAW -,
// an engine draw that does not blend and writes the scene's colour with the depth test off): the chain leaves it
// unused for the cycle. An engine draw that does not blend and writes depth does not taint: it is re-issued into T's
// late-opaque flag. The cycle is the writer's: g_topo_cycles (the render thread's, or a park's - it stalls the render
// thread inside that cycle). Once a cycle.
inline void kh_wsee_taint(uint64_t khwt_c) {
    if (g_wsee_ok_cycle.load(std::memory_order_relaxed) != khwt_c) return;
    if (g_wsee_used_cycle.load(std::memory_order_relaxed) == khwt_c) return;   // The chain has drawn.
    if (g_wsee_taint_cycle.load(std::memory_order_relaxed) == khwt_c) return;
    g_wsee_taint_cycle.store(khwt_c, std::memory_order_relaxed);
}
}