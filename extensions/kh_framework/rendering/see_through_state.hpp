#pragma once

// The shared head (rendering_integration.hpp): WSEE's shared cycle state (the see-through record's arm, cycle and
// hold flags, KH_WSEE_BUILD). Never include it on its own.
namespace RenderIntegration {
// WSEE - the see-through record's globals (the WSEE note at g_wsee_sv.rtvs in rendering/see_through_shaders.hpp);
// the chain (rendering/post_chain.hpp) and the flush read them. The want: stamped with its cycle by a chain run in
// which some pass would take a mode, valid capture or not (fx_chain_run - any thread: the late chain-only flush may
// be the game thread's), and by the scene flush that leaves such a chain to FX_LATE (wsee_arm). The landed and
// tainted cycles: written by the render thread's hooks and by a flush's own depth writes (a park's too), read by the
// chain against the published cycle (wsee_valid). Render thread alone: the attempt stamp, the cycle's verdict (taken
// at the main depth's clear, wsee_cycle_note) and the pending re-issue.
// WSEE_ON: the whole system, chosen when the extension is compiled (no script switch). KH_WSEE_BUILD 0 builds it
// out: no capture, no record targets, no CreatePixelShader hook or blob store, no per-clear see-through work and no
// work in the hooks beyond the early-out tests of the Zb guard's (psb_zb_draw / _dss / _clear / _dst / _list: a
// context compare and a relaxed load each; _dss a store per OMSetDepthStencilState, PSB_ZB_DSS_ARMED), XDRAW's
// (wsee_xdraw), the pre-draw's g_wsee.on test and the four core draw hooks' post-side g_wsee.pending compare (a
// byte never set); per flush one wsee_sort_mode call where a translucent or deferred part exists (0 at its first
// test); every chain pass's WSEE lanes 0 and t53 - t69 as the engine left them (no bind: WSEE_BIND_ARMED).
// Tested with `if constexpr` where a bare constant test would be one, and a function that does nothing with it out
// wraps its whole body in one (no code left unreachable: MSVC C4127 / C4702).
#ifndef KH_WSEE_BUILD
#define KH_WSEE_BUILD 1
#endif
static constexpr bool WSEE_ON = KH_WSEE_BUILD != 0;
// This draw's re-issue (the pre-draw to its hook's post; wsee_post): none, a blended depth-writer's (T), a
// blended no-write draw's (T, then NW) or a non-blended depth-writer's after the capture (T's late-opaque flag).
static constexpr uint8_t WSEE_RE_NONE = 0u, WSEE_RE_BLEND = 1u, WSEE_RE_NOWRITE = 2u, WSEE_RE_OPAQUE = 3u;
// The cycle's geometry hold: one entry per deferred copy (WSEE_OWN_STATIC). own: an object drawn from its mesh's
// own buffer at the injection (static, or a pose or a stage not yet installed), or one with nothing to draw at its
// levels - no copy, the flush binds that buffer whatever the slot holds by then; else (a skinned pose - its own buffer
// or a skin pool chunk's slice -, a cloth's or a chain's buffer, a vertex stage's output) the vertices the injection
// drew with, as they stood then: its copy in g_res.wsee_hold_vb (wsee_hold_take) - only the vertex runs the flush
// can draw, the translucent submeshes at the injection's level and, crossfading, the next (lvl, t: the injection's
// pick, which the flush then takes) - each at its own place relative to base (the IA offset the flush binds: vertex
// v at base + 48 v, so the mesh's absolute indices address the copy unchanged), within [off, off + bytes). gen: the
// hold buffer's generation at the copy (a release since: the entry is void). Beside g_wsee.defer_objs (the copies'
// ws_hold index it, 1-based), but cleared only by the cycle's first hold (wsee_hold_begin) and the session: the
// flush reads it while it draws, after it took the copies; g_wsee.hold_used: the bytes this cycle's entries take.
// Render thread, or a park's flush that stalls it.
struct WseeHold {
    int      mesh = -1;
    bool     own = false;
    UINT     base = 0u;
    UINT     off = 0u;
    UINT     bytes = 0u;
    uint64_t gen = 0u;
    int      lvl = 0;
    float    t = 0.0f;
};
// WSEE's shared cycle state: the see-through record's arm, cycle and hold flags (the want, taint and layer cycles,
// the deferred translucent parts, the hold).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// The late chain (FX_LATE) may read cycles on the game thread: the ones it reads are the atomic members.
// Session reset: session_scratch_reset resets every member.
struct WseeCycleState {
    std::atomic<uint64_t> want_cycle{ ~0ull };    // The newest cycle a chain wanted the capture in.
    std::atomic<uint64_t> ok_cycle{ ~0ull };      // The cycle whose capture (S, B, T and NW cleared) landed.
    std::atomic<uint64_t> taint_cycle{ ~0ull };   // The cycle whose capture a later write spoiled.
    // WSEE_BWANT: the newest cycle a reader of the scene colours B / B2 wanted them - a chain run with an armed
    // distance fog or user effect (wsee_col_reader), the flush's FX_LATE take of one (wsee_arm), the flush's
    // sorted parts compositing (modes 2 / 3) - read by the capture as its own want is (WSEE_WANT_SPAN); and the
    // cycle whose capture resolved B (its B2 moment resolves B2 then, and only then).
    std::atomic<uint64_t> col_want_cycle{ ~0ull };
    std::atomic<uint64_t> col_cycle{ ~0ull };
    // This cycle's layer-depth verdict (wsee_cycle_note: the split not refused - wsee_ld_failed; render thread).
    bool cyc_layer = false;
    // The cycle whose Tp / Tf the capture cleared (the chain binds them for that cycle alone; the late chain may be the
    // game thread's), and the far plane's raw D its Tf depth was cleared to (render thread; false = none: no Tf replay
    // this cycle - Tf stays 1).
    std::atomic<uint64_t> ld_cycle{ ~0ull };
    bool  ld_far_ok = false;
    float ld_far_lo = 0.0f, ld_far_hi = 1.0f;   // The world range D's raw was computed in.
    // WSEE_NOPRE: a late opaque marked before the cycle's B2 attempt ("pre") is not replayed (wsee_mark_opaque)
    // and the shaders derive what its replay would have written (cb.hlsl's note at khWsS2). g_wsee.nopre_cycle: the
    // cycle a pre mark was skipped in (lane bit 8; and D3's verdict, wsee_d3_ok); g_wsee.s2_cycle: the cycle S2 was
    // taken in (wsee_b2_take - S2 bound at t62 with lane bit 9). Render thread writes; the chain (the late one may
    // be the game thread's) and the sort scope read them.
    std::atomic<uint64_t> nopre_cycle{ ~0ull };
    std::atomic<uint64_t> s2_cycle{ ~0ull };
    // The late-opaque replay keeps each late opaque's own depth, D3, and the shaders take it as the behind of such a
    // surface in place of the live depth (a fence-type layer drawn over it later held that). The capture stamps
    // g_wsee.od_cycle, and the chain binds D3 at t65 with bit 5 of fx_tex[1].w for that cycle alone (the late chain may
    // be the game thread's).
    std::atomic<uint64_t> od_cycle{ ~0ull };
    // WSEE_OQ: a pre late opaque is skipped (g_wsee.nopre_cycle), so D3 holds no depth of it: g_wsee.d3_sync_cycle
    // is the cycle S2 was taken in after such a skip (wsee_b2_take), from which the readers take S2 itself there.
    // Render thread writes; the chain (the late one may be the game thread's) and the sort scope read them through
    // wsee_d3_ok.
    std::atomic<uint64_t> d3_sync_cycle{ ~0ull };
    // WSEE_OQ2: the cycle a post late opaque last wrote D3m in since D3's last resolve (wsee_re_set sets it,
    // wsee_d3m_resolve clears it to ~0 once drawn): a reader resolves D3 only where it is the reader's cycle. Render
    // thread writes; the chain (the late one may be the game thread's, inside a resolve the engine issues there: the
    // engine never drives the immediate context from two threads at once) and the sort scope read and clear it.
    std::atomic<uint64_t> d3m_cycle{ ~0ull };
    // The injection's translucent parts left to the flush's tail (the WSEE note's SORT paragraph): copies of the
    // objects as the injection drew them (the transform, size and colour the solid took; draw_part 1), and the cycle
    // they belong to. Written by the injection (wsee_defer_note), taken whole by the cycle's first scene flush that
    // builds its mesh list (flush_locked: drawn first in its order where it stands down for the injection's meshes,
    // dropped where it draws them itself - their parts with them), dropped unconsumed at the next main-depth clear
    // (wsee_cycle_note). g_wsee.defer_taken / g_wsee.defer_drawn: the cycle whose flush appended them and the cycle
    // whose flush then reached its draw loop - a flush that bails between the two counts as a loss; a loss (a tainted
    // deferred cycle too, WSEE_DEFER_TAINT) stands the deferral down until g_wsee.defer_back (WSEE_DEFER_BACKOFF
    // cycles: a persistent cause would otherwise lose our parts every frame). The render thread's, or a park's that
    // stalls it; reset with the session's scratch (the copies hold material-set references until then).
    uint64_t defer_cycle = ~0ull;
    std::vector<RenderObject> defer_objs;
    uint64_t defer_taken = ~0ull;
    uint64_t defer_drawn = ~0ull;
    uint64_t defer_back = 0;
    std::vector<WseeHold> hold;
    UINT hold_used = 0u;
    uint64_t hold_cycle = ~0ull;   // The cycle g_wsee.hold's entries belong to (wsee_hold_begin).
    // WSEE_HOLD_IDLE_COPY: the last cycle a COPY entry used the hold buffer or its run caches (wsee_hold_take, a
    // refused group that built caches) - the idle release's stamp. An own entry (the mesh's own buffer) stamps nothing:
    // static parts deferring every cycle must not keep a buffer an earlier crowd grew.
    uint64_t hold_copy_cycle = ~0ull;
    // The cycle whose chain run bound its capture (fx_chain_run; exactly one chain a cycle). Past it nothing of the
    // cycle reaches the picture: the pre-draw stands down and a taint is moot (wsee_taint ignores it, so a late
    // write - an engine pass on the scene after the chain - cannot count the used capture as waste).
    std::atomic<uint64_t> used_cycle{ ~0ull };
    uint64_t snap_cycle = ~0ull;   // The cycle whose capture was attempted - once, landed or not.
    // This cycle captures: WSEE_ON, a want <= WANT_SPAN old, no back-off and no WSEE_FLUSHW_PRED stand-down.
    bool     on = false;
    uint8_t  pending = WSEE_RE_NONE;
    // The pending particle-type re-issue is drawn after this cycle's B2 (Tpost / NWpost), else before (Tpre / NWpre) -
    // wsee_pre_draw's verdict for its post; a pending late opaque is always post (wsee_mark_opaque: a pre one is
    // not replayed). Render thread.
    bool     pending_post = false;
    // The cycle whose second colour snapshot B2 landed (taken at its first fence-type layer, wsee_b2_take; the chain
    // reads it - atomic: the late chain may be the game thread's) and the cycle B2 was attempted in (once).
    std::atomic<uint64_t> b2_cycle{ ~0ull };
    uint64_t b2_try = ~0ull;
};
static WseeCycleState g_wsee;
// PSB: the particle-type layers' cycles (the record's P0 / P1 / N / N2 planes, the far raw the Tf verdict compares
// with) and PSA's two cycles.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// The late chain (FX_LATE) may read cycles on the game thread: the ones it reads are the atomic members.
// Session reset: session_scratch_reset resets 8 of them.
struct PsbState {
    // PSB: the one pass's record - the cycle whose capture cleared it, the cycle a re-issue drew into it (the
    // chain's and the sort's lane bit 6: its loads), the cycle a particle-type layer went into the old record instead
    // (a re-issue the one pass could not take, our own tail; bit 7 is its complement: the old record's NW / Tp / Tf
    // hold their clears and are not read), and D's raw the Tf verdict compares with (render thread writes; the late
    // chain may read the cycles on the game thread).
    std::atomic<uint64_t> cycle{ ~0ull };
    std::atomic<uint64_t> drew_cycle{ ~0ull };
    // PSB_ZB: the capture builds Zb (the block's largest S and where it lies) and the one pass's loop reads the live
    // depth only for the verdicts Zb leaves open. g_psb.zb_cycle: the cycle whose capture built Zb, while its guard
    // stands (psb_zb_trip sets ~0: any write that could leave the main depth farther than S since the capture - the
    // guard's note in rendering/see_through.hpp); g_psb.zb_gen: the depth view's generation it was read through
    // (Resources' depth_srv_gen); g_psb.zb_far: the depth state bound on the engine's context (any caller's set while
    // Zb stands - psb_zb_dss) can write the depth farther (GREATER, GREATER_EQUAL, NOT_EQUAL or ALWAYS with a write)
    // - true where unknown (Zb not standing or just built: the next draw looks at the live state). Render thread.
    std::atomic<uint64_t> zb_cycle{ ~0ull };
    uint32_t zb_gen = 0u;
    bool zb_far = true;
    std::atomic<uint64_t> psa_cycle{ ~0ull };   // PSB: the old record's stamp (the note at g_psb.cycle).
    // PSB_TPA: the cycle a layer went into the old record's Tp (wsee_post's Tp passes, wsee_tail's) - the
    // late-opaque replays reset Tp from then on (wsee_re_set's l); before it Tp holds its clear, which the
    // reset would write again. g_psb.psa_cycle stays the whole old record's stamp (lane bit 7): a light-adding NW-only
    // layer sets it and writes no Tp. Render thread (the tail's: a park that stalls it).
    std::atomic<uint64_t> psa_tp_cycle{ ~0ull };
    float far_raw = 1.0f;   // PSB: D's raw the Tf verdict compares with (the note at g_psb.cycle).
    bool have = false;                  // PSB: some entry holds one (the capture makes the record).
};
static PsbState g_psb;
// PSB_LORES: log2 of the one pass's scale s = 2 - P0 / P1 / N / N2 made at 1 / s of the frame a side and the one
// pass drawn into them at that scale (Resources' PSB_LORES note). TWIN of cb.hlsl's KH_WSEE_PSB_LS (the readers'
// texel px >> it) and of PSPsbZb's KH_PSB_ZB_LS (PSB_ZB_DEFS below); the static_assert pins the value (the patcher,
// psb_patch, keeps an s = 1 arm no caller takes).
static constexpr UINT PSB_LS = 1u;
static_assert(PSB_LS == 1u, "PSB_LS is the twin of KH_WSEE_PSB_LS / KH_PSB_ZB_LS (s = 2)");
// PSB_ZB: PSPsbZb's defines at log2 s = PSB_LS (wsee_ensure's compile and the prewarm table's: one cache
// key).
static const D3D_SHADER_MACRO PSB_ZB_DEFS[3] = {
    { "MSAA_DEPTH", "0" }, { "KH_PSB_ZB_LS", "1" }, { nullptr, nullptr } };
// WSEE_OQ: D3 stands complete for cycle c - its capture filled it (g_wsee.od_cycle) and either no pre opaque
// was skipped or S2 was taken after them. Else its readers leave t65 null and bit 5 clear: the behind of a late opaque
// is the live depth - the same picture at sample 0 wherever no fence-type layer drew after a late opaque (a reader
// before the B2 attempt: the flush's sort scope), samples k > 0 taking their own depth in place of sample 0's D3;
// S2's pass refused at the attempt leaves that cycle so.
inline bool wsee_d3_ok(uint64_t c) {
    return g_wsee.od_cycle.load(std::memory_order_relaxed) == c &&
           (g_wsee.nopre_cycle.load(std::memory_order_relaxed) != c ||
            g_wsee.d3_sync_cycle.load(std::memory_order_relaxed) == c);
}
// A light-adding blended draw writing no depth (DestBlend ONE), into NW alone; an over-blend writing no depth with
// the depth test off, into T without a test, then NW (wsee_pre_draw's note).
static constexpr uint8_t WSEE_RE_NWONLY = 4u, WSEE_RE_NOWRITE_OFF = 5u;
static constexpr uint64_t WSEE_WANT_SPAN = 2u;
// WSEE_HOLD_FAIL_RETRY: cycles a refused make stays refused at its key before one more ask - the hold's (its
// size: wsee_hold_reserve) and, WSEE_FAIL_RETRY, the capture targets' (wsee_targets, wsee_ld_targets,
// psb_targets, wsee_d3m_targets; Resources' note at wsee_fail_cycle).
static constexpr uint64_t WSEE_HOLD_FAIL_RETRY = 600u;
// The cycle's capture would lie (the WSEE note's TAINT list: a depth write of ours on the main depth after it
// landed, a re-issue refused or impossible, a DrawAuto / indirect engine draw writing the main depth - WSEE_XDRAW -,
// an engine draw that does not blend and writes the scene's colour with the depth test off): the chain leaves it
// unused for the cycle. An engine draw that does not blend and writes depth does not taint: it is re-issued into T's
// late-opaque flag. The cycle is the writer's: g_topo_cycles (the render thread's, or a park's - it stalls the render
// thread inside that cycle). Once a cycle.
inline void wsee_taint(uint64_t c) {
    if (g_wsee.ok_cycle.load(std::memory_order_relaxed) != c) return;
    if (g_wsee.used_cycle.load(std::memory_order_relaxed) == c) return;   // The chain has drawn.
    if (g_wsee.taint_cycle.load(std::memory_order_relaxed) == c) return;
    g_wsee.taint_cycle.store(c, std::memory_order_relaxed);
}
}