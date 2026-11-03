#pragma once

// The injection half (rendering_integration_injection.hpp): the attachment steps: the game-thread samples, KH_CREW_LAG,
// the render-thread paint read, proxy smoothing, kh_attach_gt_snap and kh_attach_step. Never include it on its own.
namespace RenderIntegration {
inline void kh_chain_end_rt_read(uint64_t khcr_cyc, uint64_t khcr_own, int64_t khcr_q);   // Defined with the chain.
inline void kh_chain_end_draw(ID3D11DeviceContext* khed_ctx);                            // Likewise.
inline uint32_t kh_chain_end_crew_n();                                                   // Likewise (KH_CREW_LAG).
// KH_CHAIN's side of the census (defined with kh_chain_prepare, rendering/chain_ends.hpp): whether any chain is set up,
// and the characters a simulating chain's end follows. Game thread; the second under g_draw_list_mutex.
inline bool kh_crew_chain_any();
inline void kh_crew_chain_cand(std::vector<std::pair<game_value, bool>>& khcc_out);
inline void kh_pxy_smooth_note(const game_value& khpn_x, uint64_t khpn_seq, int64_t khpn_q, const float khpn_r[3],
                               float khpn_d[3]);   // KH_PXY_SMOOTH, defined with the estimator below.

// KH_ATTACH_FINAL: raised around a cycle's final kh_attach_step (the injection's, or the render-thread flush's when the
// injection did not run); kh_attach_moved_note reads it (KH_SUN_RESTEP). Render thread only, raised only through
// KhFinalStepScope so a throw cannot leave it up.
static bool g_attach_final_step = false;
struct KhFinalStepScope {
    KhFinalStepScope() { g_attach_final_step = true; }
    ~KhFinalStepScope() { g_attach_final_step = false; }
    KhFinalStepScope(const KhFinalStepScope&) = delete;
    KhFinalStepScope& operator=(const KhFinalStepScope&) = delete;
};
// KH_CAST_REPICK: raised around the step the cast fire runs just before it paints our shadow into the engine's mask
// (mask_cast_engine). That step picks again, as the final step does, but never waits; g_attach_repick_moved notes that
// it moved something, so the fire rebuilds the sun map from the new poses before painting. g_gts_first_done is the
// newest published sample when the cycle's first render-thread step ran (g_gts_first_cyc): a paint with nothing newer
// has nothing to pick again. Render thread only; the flag is raised only through KhRepickStepScope.
static bool g_attach_repick_step = false;
static bool g_attach_repick_moved = false;
static uint64_t g_gts_first_cyc = ~0ull, g_gts_first_done = 0;
struct KhRepickStepScope {
    KhRepickStepScope() { g_attach_repick_step = true; }
    ~KhRepickStepScope() { g_attach_repick_step = false; }
    KhRepickStepScope(const KhRepickStepScope&) = delete;
    KhRepickStepScope& operator=(const KhRepickStepScope&) = delete;
};
// KH_SUN_RESTEP - the cycle in which the final step (KH_ATTACH_FINAL: the injection's, or the render-thread flush's
// when the injection did not run) moved or re-skinned (KH_SKIN_RESTEP_KEY: its shape key changed) a followed object
// after the cast fire's step had built the sun map, which our meshes' self-shadow also samples.
// inject_composited_meshes (or flush_locked, for its late draws) reopens the map for that cycle (it re-renders only if
// its input hash changed).
// Cost: a second ladder render on such cycles. The final step picks the cycle's sample again (kh_gts_for_cycle), so
// it moves a binding wherever the frame's own sample landed after the fire's step - most cycles of a light scene - as
// well as where a mesh was shown or edited between the two. The world cast is painted once a frame, into the engine's
// own mask, and cannot be repainted: the fire picks again just before that paint instead (KH_CAST_REPICK).
static uint64_t g_sun_restep_cycle = ~0ull;

// KH_ATTACH_GT_SNAP - which sample a cycle draws (KH_GTS_NEAREST). The Draw3D sampler (kh_attach_gt_snap) samples every
// binding's objects once a frame; the step draws one of those samples and never reads an object itself.
//   Own  = a frame's Draw3D lands within a few milliseconds of that frame's main depth clear - before it or after it,
//          and which side flips with the load - while every other frame's lands most of a frame away. That is how
//          the frame's own sample is recognised: the offset between sample numbers and cycles is learned from it
//          (KH_GTS_IDENTITY, below) and carried from cycle to cycle, relearned every cycle, and a cycle's own sample
//          is sample number cycle + that offset. Before the offset is known, the sample stamped nearest the clear
//          within half a frame period either side stands in (g_gts_half: half the shorter of the last two frames, so
//          a hitch or a pause does not stretch it).
//   Wait = none, but for a binding or a followed chain end on a crew member: the render thread does not wait for a
//          Draw3D. On some terrains a frame's own Draw3D runs only after that frame's injection
//          (cytech_underground_terrain, measured below), so a wait for it there ends only at its cap, and a wait at
//          all can hold the game thread back from its Draw3D. A crew member's exact pose is the Draw3D's own
//          measurement (KH_CREW_LAG's pose commands), so while one is bound, or a chain's end follows one
//          (kh_chain_end_crew_n), the final step waits for that frame's sample, up to KH_GTS_CREW_CAP_MS after the
//          clear (kh_gts_step_prep) - and only while that sample is the next the sampler will publish
//          (KH_CREW_SILENT: a silent or lagging sampler is not waited for).
//   Own or read = every render-thread step draws the frame's own sample by number when it has landed
//          (KH_GTS_IDENTITY, below); when it has not, the render thread reads the objects itself - at the cycle's
//          first step and again at its final step (kh_gts_step_prep) - and the step draws that read, so the mesh
//          and everything drawn from it in the cycle (the sun and dynamic-light maps, the footprint and stencil
//          seams, the view-model slice, the world cast's paint) take the frame's pose (KH_RT_PAINT_READ, below).
//          Before the offset is known (a session's first cycles) a step draws the sample stamped nearest the clear
//          within half a period, else the newest before the clear (B).
//   Final = the cast fire's first step (0.03 ms after the clear) takes the frame's own sample if it has landed, else
//          the render thread's read ("Own or read"): in a light scene the frame's Draw3D lands after it (measured:
//          0.3-1.7 ms after the clear). The final step picks again; a pose that changes there re-skins its skeletal
//          mesh and reopens the sun map (KH_SUN_RESTEP).
//   Paint = the engine reads its shadow mask right after the fire paints our shadow into it (0.08-0.47 ms after the
//          clear). The fire picks again just before that paint when a sample has landed since the cycle's first step
//          (KH_CAST_REPICK) and, with the frame's own still absent, reads the objects itself (KH_RT_PAINT_READ).
// Measured (the engine's diag_frameNo against the render cycle): three runs - a heavy scene (21.7 ms frames), a light
// one after a vehicle exit (10.4 ms) and one flipping between 20 ms and 10 ms frames across an exit - 756 cycles,
// each frame's own Draw3D within 3.1 ms before or 2.6 ms after its clear (at most 26 % of a frame), the nearest
// sample the frame's own on every one. Do not go back to the rules this replaced (the newest sample before the clear as
// the pick, with an aim carried from cycle to cycle, write-offs and fallbacks): they drew 321 of them a frame behind
// after vehicle exits.
// Measured on cytech_underground_terrain: the own Draw3D landed 1-3 ms after the clear, after the final step (about
// 0.6 ms). Do not wait for every binding's sample by number: capped at 100 ms, such waits ended at the cap 13 times
// in about 7 s, the sample landing just after each, and after them a half period swollen past 14 ms let the previous
// frame's sample be learned as the frame's own, so every cycle drew one frame behind (1,230 in one run).
static int64_t  g_gts_clear_qpc = 0;                 // Render thread: this cycle's main depth clear (kh_gts_qpc).
static int64_t  g_gts_clear_prev = 0;                // The clear before it.
static int64_t  g_gts_half = 0;                      // Half the frame period at this clear (0: not known yet).
static std::atomic<uint64_t> g_gts_done_pub{ 0 };   // Game thread: the seq whose sampler has written every binding.
static constexpr uint32_t KH_GTS_STAMPS = 8u;
static std::atomic<int64_t> g_gts_stamp_pub[KH_GTS_STAMPS];   // Game thread: seq's stamp at [seq % KH_GTS_STAMPS], stored
                                                              // before g_gts_done_pub names it.
static uint64_t g_gts_final_cyc = ~0ull;             // The cycle whose final step has run kh_gts_step_prep.
inline int64_t kh_gts_qpc() { LARGE_INTEGER khgq_q; QueryPerformanceCounter(&khgq_q); return khgq_q.QuadPart; }
// The main depth clear (render thread): this cycle's boundary, and half the frame period - the shorter of the last two
// frames, so one long frame (a hitch, a pause) does not widen the window.
inline void kh_gts_clear_note(int64_t khcn_now) {
    const int64_t khcn_a = g_gts_clear_qpc;
    const int64_t khcn_b = g_gts_clear_prev;
    g_gts_clear_prev = khcn_a;
    g_gts_clear_qpc = khcn_now;
    if (khcn_b > 0 && khcn_a > khcn_b && khcn_now > khcn_a) {
        const int64_t khcn_d1 = khcn_now - khcn_a;
        const int64_t khcn_d2 = khcn_a - khcn_b;
        g_gts_half = (khcn_d1 < khcn_d2 ? khcn_d1 : khcn_d2) / 2;
    } else {
        g_gts_half = 0;
    }
}
// The object a binding is placed relative to: a memory-point binding's parent, else its position lane's object, else
// (a binding that follows a rotation alone - KH_ATTACH_ROT_ONLY) its rotation lane's object.
inline const game_value& kh_attach_parent_gv(const KhAttach& khpg_a) {
    if (!khpg_a.skel && !khpg_a.bone_parent.is_nil()) return khpg_a.bone_parent;
    return khpg_a.obj_pos.is_nil() ? khpg_a.obj_rot : khpg_a.obj_pos;
}
// The object a binding's samples are keyed on (KhGtSnap::xb): its position lane's object, else its rotation lane's.
inline const game_value& kh_attach_key_gv(const KhAttach& khkg_a) {
    return khkg_a.obj_pos.is_nil() ? khkg_a.obj_rot : khkg_a.obj_pos;
}
// KH_ATTACH_MAN_HELPER: the object the Draw3D sampler reads for khor_obj - the lane's helper where one stands in for
// it and has been attached (read after its stamp, the first sample that saw it: that sample reads the character,
// because attachTo places a new helper at the next simulation step; a lane with no stamp stamps its own first
// sample), else the object itself - which is also what a helper something else deleted falls back to (the
// character's own state: right outside a vehicle). Game thread, under g_draw_list_mutex.
inline const game_value& kh_attach_org_read(KhAttach& khor_a, const game_value& khor_obj, uint64_t khor_seq) {
    const void* const khor_p = khor_obj.data.get();
    if (!khor_p) return khor_obj;
    if (!khor_a.org_pos.is_nil() && khor_a.org_pos_for == khor_p) {
        if (khor_a.org_pos_seq == 0u) khor_a.org_pos_seq = khor_seq;
        return khor_seq > khor_a.org_pos_seq && !kh_attach_obj_dead(khor_a.org_pos) ? khor_a.org_pos : khor_obj;
    }
    if (!khor_a.org_rot.is_nil() && khor_a.org_rot_for == khor_p) {
        if (khor_a.org_rot_seq == 0u) khor_a.org_rot_seq = khor_seq;
        return khor_seq > khor_a.org_rot_seq && !kh_attach_obj_dead(khor_a.org_rot) ? khor_a.org_rot : khor_obj;
    }
    return khor_obj;
}
// The sampler's read of one object, its validation noted for getRenderStats (KH_ATTACH_DIAG).
inline bool kh_gts_read(const game_value& khgr_gv, uint32_t& khgr_off, uintptr_t& khgr_vb, uintptr_t& khgr_bb,
                        uintptr_t& khgr_avb, uintptr_t& khgr_abb, float khgr_pos[3], float khgr_rot[9]) {
    KhAttachDiag khgr_dg;
    const bool khgr_ok = kh_attach_raw(khgr_gv, khgr_off, khgr_vb, khgr_bb, khgr_avb, khgr_abb, khgr_pos, khgr_rot, &khgr_dg);
    kh_attach_diag_note(khgr_ok, khgr_dg);
    return khgr_ok;
}
// KH_GTS_HOLD - a helper whose read was refused this frame (attachRefused) keeps the pose it had RELATIVE TO ITS PARENT
// in the binding's previous sample, carried onto this frame's parent: the rest of the binding stays current and that
// one helper holds its local pose for the frame, instead of the whole binding going without a sample - which the pick
// then draws as a held frame, a visible jump. Poses are 12 floats: position (SQF order), then the basis rows (engine
// axes, orthonormal as kh_attach_vs_eval hands them back). khgy_out must not alias an input.
inline void kh_gts_carry(const float khgy_pp[12], const float khgy_pn[12], const float khgy_x[12], float khgy_out[12]) {
    const float khgy_d[3] = { khgy_x[0] - khgy_pp[0], khgy_x[2] - khgy_pp[2], khgy_x[1] - khgy_pp[1] };   // Engine axes.
    const float* const khgy_rp = khgy_pp + 3;
    const float* const khgy_rn = khgy_pn + 3;
    float khgy_w[3] = { 0.0f, 0.0f, 0.0f };   // The offset, taken into the new parent's frame.
    for (int k = 0; k < 3; ++k) {
        const float khgy_l = khgy_d[0] * khgy_rp[k * 3] + khgy_d[1] * khgy_rp[k * 3 + 1] + khgy_d[2] * khgy_rp[k * 3 + 2];
        for (int c = 0; c < 3; ++c) khgy_w[c] += khgy_l * khgy_rn[k * 3 + c];
    }
    khgy_out[0] = khgy_pn[0] + khgy_w[0];
    khgy_out[1] = khgy_pn[1] + khgy_w[2];   // SQF north = engine z.
    khgy_out[2] = khgy_pn[2] + khgy_w[1];   // SQF up = engine y.
    for (int i = 0; i < 3; ++i) {   // Each basis row, re-expressed from the old parent's frame in the new one's.
        const float* const khgy_r = khgy_x + 3 + i * 3;
        float* const khgy_o = khgy_out + 3 + i * 3;
        khgy_o[0] = khgy_o[1] = khgy_o[2] = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float khgy_l = khgy_r[0] * khgy_rp[k * 3] + khgy_r[1] * khgy_rp[k * 3 + 1] + khgy_r[2] * khgy_rp[k * 3 + 2];
            for (int c = 0; c < 3; ++c) khgy_o[c] += khgy_l * khgy_rn[k * 3 + c];
        }
    }
}
// KH_GTS_IDENTITY - render cycles and Draw3D samples run one for one, so the frame's own sample is sample number
// cycle + g_gid_k. The offset is learned once a cycle, at its first render-thread step, from the PREVIOUS cycle,
// whose own Draw3D has landed by then (kh_gts_id_learn): the one sample stamped within KH_GTS_OWN_US and within
// half a period of that cycle's clear, when there is exactly one. The window is fixed: after a stall the half period
// swelled past 14 ms and let the previous frame's sample, stamped 12.65 ms before the clear, pass for the frame's
// own - after which every cycle drew one frame behind. Own samples landed between 3.1 ms before and 3.2 ms after
// their clear in every measured run. A learning that changes the offset is a break (getRenderStats
// "identityBreaks").
static constexpr int64_t KH_GTS_OWN_US = 4000;
static int64_t  g_gid_k = 0;                 // Render thread: sample number - cycle.
static bool     g_gid_ok = false;            // The offset is known (learned since the session start).
static uint64_t g_gid_breaks = 0;            // getRenderStats "identityBreaks".
static uint64_t g_gts_learn_cyc = ~0ull;     // Render thread: the cycle whose offset has been learned.
static uint64_t g_gts_first_rd_cyc = ~0ull;  // Render thread: the cycle whose first step has run its read.
// KH_CREW_LAG: bindings the last Draw3D sample placed on a crew member, and followed chain ends on one (the sampler;
// the render thread reads it), and how long after the clear the final step waits for the frame's own sample while
// there is one.
static std::atomic<uint32_t> g_gts_crew_n{ 0 };
static constexpr int64_t KH_GTS_CREW_CAP_MS = 100;
// KH_CHAIN_END_DRAW: chains whose end follows an object or a memory point, is fixed in the world or is held on a
// skeletal object's bone (g_chain_end, game thread under g_draw_list_mutex; kh_chain_end_filed). With one, the
// Draw3D sampler and the steps run even with no binding: the frame's identity and the render thread's read serve a
// followed end as they serve a binding.
static std::atomic<uint32_t> g_chain_end_n{ 0 };
// With a chain end followed, the sampler's number is published with that Draw3D's chain targets (at the end of
// kh_chain_prepare, kh_chain_end_publish, still inside the Draw3D handler and under g_draw_list_mutex): a step that
// saw the sample landed but not its chain targets would draw the chain's end a frame old. Game thread.
static uint64_t g_gts_done_defer = 0;
inline bool kh_attach_live() {
    return g_attach_n.load(std::memory_order_relaxed) != 0u || g_chain_end_n.load(std::memory_order_relaxed) != 0u;
}
// KH_RT_PAINT_READ: getRenderStats "paintRead" - [paints that tried a read, bindings read, torn (the two reads
// disagreed), refused (a read failed validation), then each read against the frame's own sample once it has landed:
// matched, stale (equal to the previous frame's sample), other; and final steps that read for the mesh].
struct KhRtpCounters {
    uint64_t paints = 0, bindings = 0, torn = 0, refused = 0, matched = 0, stale = 0, other = 0, finals = 0;
};
static KhRtpCounters g_rtp_c;
static uint64_t g_rtp_cyc = ~0ull;                          // The cycle whose paint has run its read.
static constexpr uint64_t KH_RT_SEQ_BIT = 1ull << 62;       // A render-thread read's number: never a sample's, so the
                                                            // final step re-stashes a skeletal pose from the real one.
// KH_RT_READ_SER: bits 48-55 of a render-thread read's number count the reads (the cycle's first step, the paint,
// the final step), so a read that CHANGED a binding's pose is a sample of its own - the skeletal stash and skin
// (KH_SKEL_DRAW) key on the number. Do not give every read of a cycle one number: it leaves the bones on the first
// read while the root and every rigid binding take the later ones. A read of the pose the cycle's last read held
// keeps that read's number (nothing to stash or skin again). Render thread; the numbers are compared only for
// equality.
static uint64_t g_rtp_read_ser = 0;
// KH_GTS_IDENTITY: the offset from the previous cycle - the newest published samples' stamps against that cycle's
// clear (g_gts_clear_prev): learned only when exactly one lies within the window (after a hitch a frame's late
// sample can land near the next clear, beside that frame's own; two there is no answer, and the offset stands).
// Seven, not eight: the sampler writes the stamp of the next sample into the eighth slot before it publishes.
// Render thread, once a cycle (kh_gts_step_prep).
inline void kh_gts_id_learn() {
    const int64_t khil_cp = g_gts_clear_prev;
    if (khil_cp <= 0 || g_topo_cycles == 0u) return;
    int64_t khil_w = static_cast<int64_t>(kh_prof_freq()) * KH_GTS_OWN_US / 1000000;
    if (g_gts_half > 0 && g_gts_half < khil_w) khil_w = g_gts_half;
    const uint64_t khil_done = g_gts_done_pub.load(std::memory_order_acquire);
    uint64_t khil_best = 0;
    uint32_t khil_n = 0;
    for (uint64_t khil_i = 0; khil_i + 1u < KH_GTS_STAMPS && khil_i < khil_done; ++khil_i) {
        const uint64_t khil_s = khil_done - khil_i;
        const int64_t khil_q = g_gts_stamp_pub[khil_s % KH_GTS_STAMPS].load(std::memory_order_relaxed);
        const int64_t khil_d = khil_q >= khil_cp ? khil_q - khil_cp : khil_cp - khil_q;
        if (khil_d <= khil_w) {
            khil_best = khil_s;
            ++khil_n;
        }
    }
    if (khil_n != 1u) return;
    const int64_t khil_k = static_cast<int64_t>(khil_best) - static_cast<int64_t>(g_topo_cycles - 1u);
    if (g_gid_ok && khil_k != g_gid_k && kh_stats_on()) ++g_gid_breaks;
    g_gid_k = khil_k;
    g_gid_ok = true;
}
// KH_RT_PAINT_READ: two poses equal bit for bit (the check of a read against the frame's own sample).
inline bool kh_gts_same(const KhGtSnap& khgs_x, const KhGtSnap& khgs_y) {
    return memcmp(khgs_x.par, khgs_y.par, sizeof(khgs_x.par)) == 0 && khgs_x.px.size() == khgs_y.px.size() &&
           (khgs_x.px.empty() || memcmp(khgs_x.px.data(), khgs_y.px.data(), khgs_x.px.size() * sizeof(float)) == 0) &&
           khgs_x.rot_ok == khgs_y.rot_ok &&
           (!khgs_x.rot_ok || memcmp(khgs_x.rot, khgs_y.rot, sizeof(khgs_x.rot)) == 0);
}
// The sample cycle khgc_cyc draws (KH_ATTACH_GT_SNAP, above), or null (the ring holds none for this binding): the
// frame's own by number when the identity offset is known and the ring holds it (KH_GTS_IDENTITY); else, at a
// render-thread pick, the render thread's read for this cycle (KH_RT_PAINT_READ: the frame's own has not landed);
// else the one stamped nearest the cycle's clear within half a period; else the newest before the clear, else the
// oldest held. The final step also checks the read it drew the cycle before against that frame's own sample,
// landed since (matched / stale / other).
// Memoized per binding and cycle by the render thread (khgc_store), so the steps of a cycle draw one sample even when
// the next Draw3D lands between them - except that the final step (g_attach_final_step) and the cast fire's step
// before its paint (g_attach_repick_step, KH_CAST_REPICK) pick again and store their pick for the steps after them.
// A game-thread step (the park's, with the render thread held still) reads the memo, or computes the same pick
// unstored. Under g_draw_list_mutex.
inline const KhGtSnap* kh_gts_for_cycle(KhAttach& khgc_a, uint64_t khgc_cyc, bool khgc_store) {
    const size_t khgc_np = khgc_a.skel ? khgc_a.skel_proxy.size() : (khgc_a.proxy.is_nil() ? 0u : 1u);
    const uint32_t khgc_gen = khgc_a.skel ? khgc_a.skel_gen : 0u;
    const uintptr_t khgc_xb = kh_attach_base_of(kh_attach_key_gv(khgc_a));
    const KhGtSnap* khgc_c[KH_GTS_N];
    uint32_t khgc_nc = 0u;
    for (uint32_t khgc_i = 0; khgc_i < KH_GTS_N; ++khgc_i) {
        const KhGtSnap& khgc_s = khgc_a.gts[khgc_i];
        if (khgc_s.ok && khgc_s.gen == khgc_gen && khgc_s.xb == khgc_xb && khgc_s.px.size() == khgc_np * 12u) {
            khgc_c[khgc_nc++] = &khgc_s;
        }
    }
    const bool khgc_new = khgc_a.gts_cyc != khgc_cyc || (khgc_store && (g_attach_final_step || g_attach_repick_step));
    // KH_RT_PAINT_READ: the read this cycle's pick took - while it is still this binding's (the ring's filter above;
    // a command can re-bind between two steps of a cycle, and a read of the old binding carries its helpers).
    if (!khgc_new && khgc_a.rt_cyc == khgc_cyc && khgc_a.gts_pick_seq == khgc_a.rt_snap.seq && khgc_a.rt_snap.ok &&
        khgc_a.rt_snap.gen == khgc_gen && khgc_a.rt_snap.xb == khgc_xb && khgc_a.rt_snap.px.size() == khgc_np * 12u) {
        return &khgc_a.rt_snap;
    }
    if (!khgc_new) {   // This cycle's pick stands.
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) {
            if (khgc_c[khgc_i]->seq == khgc_a.gts_pick_seq) return khgc_c[khgc_i];
        }
    }
    // KH_RT_PAINT_READ: the read the previous cycle's final step drew, against that frame's own sample, once.
    if (khgc_store && g_attach_final_step && g_gid_ok && khgc_a.rt_chk_cyc != ~0ull &&
        khgc_a.rt_chk_cyc + 1u == khgc_cyc) {
        const uint64_t khgc_ow = static_cast<uint64_t>(static_cast<int64_t>(khgc_a.rt_chk_cyc) + g_gid_k);
        const KhGtSnap* khgc_o = nullptr;
        const KhGtSnap* khgc_ob = nullptr;
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) {
            if (khgc_c[khgc_i]->seq == khgc_ow) khgc_o = khgc_c[khgc_i];
            if (khgc_c[khgc_i]->seq + 1u == khgc_ow) khgc_ob = khgc_c[khgc_i];
        }
        if (khgc_o) {
            khgc_a.rt_chk_cyc = ~0ull;
            // A read of the binding a command has re-bound since judges nothing (the ring's filter, as the memo's).
            if (khgc_a.rt_chk.ok && khgc_a.rt_chk.gen == khgc_gen && khgc_a.rt_chk.xb == khgc_xb &&
                khgc_a.rt_chk.px.size() == khgc_np * 12u) {
                if (kh_gts_same(khgc_a.rt_chk, *khgc_o)) { if (kh_stats_on()) ++g_rtp_c.matched; }
                else if (khgc_ob && kh_gts_same(khgc_a.rt_chk, *khgc_ob)) { if (kh_stats_on()) ++g_rtp_c.stale; }
                else { if (kh_stats_on()) ++g_rtp_c.other; }
            }
        }
    }
    if (khgc_nc == 0u) return nullptr;
    const int64_t khgc_cl = g_gts_clear_qpc;
    const int64_t khgc_half = g_gts_half;
    const KhGtSnap* khgc_p = nullptr;   // The frame's own: nearest the clear, within half a period.
    if (g_gid_ok) {   // KH_GTS_IDENTITY: by number, when it is in the ring (a late one the time rule would pass over).
        const uint64_t khgc_want = static_cast<uint64_t>(static_cast<int64_t>(khgc_cyc) + g_gid_k);
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) if (khgc_c[khgc_i]->seq == khgc_want) khgc_p = khgc_c[khgc_i];
    }
    // KH_RT_PAINT_READ: a render-thread pick with the frame's own not landed - the render thread's read of it for this
    // cycle (kh_gts_step_prep), which the steps after it keep (the memo). A crew member's read is its prediction
    // (KH_CREW_LAG), which only the paint's repick takes: the mesh keeps the Draw3D's measurement for it. The final
    // step keeps its read for the check against the frame's own sample at the next final step.
    if (!khgc_p && g_gid_ok && khgc_store && khgc_new && (!khgc_a.rt_crew || g_attach_repick_step) &&
        khgc_a.rt_cyc == khgc_cyc && khgc_a.rt_snap.ok && khgc_a.rt_snap.gen == khgc_gen &&
        khgc_a.rt_snap.xb == khgc_xb && khgc_a.rt_snap.px.size() == khgc_np * 12u) {
        khgc_a.gts_cyc = khgc_cyc;
        khgc_a.gts_pick_seq = khgc_a.rt_snap.seq;
        if (g_attach_final_step) {
            khgc_a.rt_chk = khgc_a.rt_snap;
            khgc_a.rt_chk_cyc = khgc_cyc;
        }
        return &khgc_a.rt_snap;
    }
    int64_t khgc_best = 0;
    if (!khgc_p && khgc_half > 0) {   // KH_GTS_IDENTITY: the time rule when identity has not picked.
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) {
            const int64_t khgc_d = khgc_c[khgc_i]->qpc >= khgc_cl ? khgc_c[khgc_i]->qpc - khgc_cl : khgc_cl - khgc_c[khgc_i]->qpc;
            if (khgc_d <= khgc_half && (!khgc_p || khgc_d < khgc_best)) {
                khgc_p = khgc_c[khgc_i];
                khgc_best = khgc_d;
            }
        }
    }
    if (!khgc_p) {   // Not arrived (or the period not known yet): B, the newest before the clear, else the oldest held.
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) {
            if (khgc_c[khgc_i]->qpc <= khgc_cl && (!khgc_p || khgc_c[khgc_i]->seq > khgc_p->seq)) khgc_p = khgc_c[khgc_i];
        }
    }
    if (!khgc_p) {
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) if (!khgc_p || khgc_c[khgc_i]->seq < khgc_p->seq) khgc_p = khgc_c[khgc_i];
    }
    if (khgc_store && g_attach_final_step && khgc_a.rt_cyc == khgc_cyc && g_gid_ok && khgc_p &&   // KH_RT_PAINT_READ:
        khgc_p->seq == static_cast<uint64_t>(static_cast<int64_t>(khgc_cyc) + g_gid_k)) {        // the check.
        const KhGtSnap& khgc_r = khgc_a.rt_snap;
        const KhGtSnap* khgc_prev = nullptr;
        for (uint32_t khgc_i = 0; khgc_i < khgc_nc; ++khgc_i) if (khgc_c[khgc_i]->seq + 1u == khgc_p->seq) khgc_prev = khgc_c[khgc_i];
        // A read of the binding a command has re-bound since judges nothing (the ring's filter, as the memo's).
        if (khgc_r.ok && khgc_r.gen == khgc_gen && khgc_r.xb == khgc_xb && khgc_r.px.size() == khgc_np * 12u) {
            if (kh_gts_same(khgc_r, *khgc_p)) { if (kh_stats_on()) ++g_rtp_c.matched; }
            else if (khgc_prev && kh_gts_same(khgc_r, *khgc_prev)) { if (kh_stats_on()) ++g_rtp_c.stale; }
            else { if (kh_stats_on()) ++g_rtp_c.other; }
        }
        khgc_a.rt_cyc = ~0ull;   // Counted once.
    }
    if (khgc_store && khgc_new) {
        khgc_a.gts_cyc = khgc_cyc;
        khgc_a.gts_pick_seq = khgc_p->seq;
    }
    return khgc_p;
}

// KH_CREW_LAG - the engine places a crew member's attachTo children (its origin helper, every memory-point proxy on
// it) behind where it draws the character (seen on player-controlled crew; the direction does not trail), while the
// character's own rendered position is right: getPosWorldVisual, the model centre - the point the origin helper
// stands for (getPosASLVisual is the land contact, a constant apart) - and so is its rendered orientation
// (vectorDirVisual / vectorUpVisual). So while a character rides in a vehicle the Draw3D sampler moves every helper
// it reads for that character as ONE rigid body, from where the origin helper is to where the character is drawn:
// a pose p, r read on it becomes t + (p - h) * D and r * D - h the origin helper's read, t the rendered position,
// D = (the helper's rows)^-1 * (the rendered rows), all taken in the same Draw3D. Every pose relative to the
// character stays as the engine has it (a skeletal binding's bones against their parent, a memory point against
// the origin) and the whole set lands where, and turns as, the character is drawn; with no trail in the direction
// (the case seen) D is the identity to float precision and this is the translation t - h. A chain's end
// (kh_chain_prepare) takes the same: a memory point moved so and its reach tested against the rendered position; an
// end following the character itself placed at that position and turned to that orientation - the character's own
// state is not written while it is crew (KH_ATTACH_MAN_HELPER), so a raw read of it would hold the get-in pose.
// Crew or not: the character's GetInMan / GetOutMan events and a check when its entry is made, each asking
// `objectParent` (kh_crew_vehicle) - the handlers take Intercept's argument lists as it declares them and read none
// of the arguments. While crew, the pose is read every Draw3D (getPosWorldVisual, vectorDirVisual, vectorUpVisual per
// character), so a seat switch (which stays in the vehicle: no GetInMan / GetOutMan), a turret's traverse or a turn-out
// needs no event of its own. In the Eden editor the three are the simulation's - getPosWorld, vectorDir, vectorUp
// (KH_ATTACH_EDEN). KH_RT_PAINT_READ carries it too: the render thread makes no engine call, so it places the character
// from the vehicle (read raw, with the paint read's other reads) and the character's pose in that vehicle's frame as
// the last Draw3D measured it - a prediction over the one frame whose sample has not landed.
// One entry per character a binding places from (kh_attach_parent_gv) through a helper - an origin helper standing in
// for it, or helpers on it (a memory-point or skeletal binding) when it is a Man - or a simulating chain's end
// follows when it is a Man (kh_crew_chain_cand: its object, its memory point's owner, its rotation object), keyed by
// its base (two holders may have different game values of one object; an entry whose character was deleted drains at
// the next sync). The entry holds a pool reference of its own on the character's origin helper, which a memory-point
// binding or a chain's end alone has none of. A rotation lane that follows a character of its own (not the object its
// binding is placed from) through that character's helper brings it in too: the lane is turned as that character is
// drawn (its rows, from the same rigid move).
// GAME THREAD maintains it (kh_crew_sync, from flush_frame; the events): inserted, erased and every field the render
// thread reads (in, d_ok, helper, veh, l_ok, l, lq) written under g_draw_list_mutex; r_* are the render thread's own
// page caches (the paint read, under the mutex too); t / tr with their flags, and seen, are the game thread's alone.
// Never destructed (game values); the mission edge empties it (kh_crew_clear_all, outside the mutex:
// dropping an entry removes its event handlers, an SQF call).
struct KhCrewEnt {
    game_value man;
    game_value helper;           // The character's origin helper (this entry's pool reference).
    game_value veh;              // The vehicle it rides in; nil on foot.
    uint64_t   serial = 0;       // The events' key (g_crew_serial).
    bool       in = false;
    bool       seen = false;     // kh_crew_sync's census mark.
    bool       t_ok = false;
    double     t[3] = {};        // getPosWorldVisual this Draw3D (getPosWorld in Eden), SQF order.
    bool       tr_ok = false;
    float      tr[9] = {};       // vectorDir/UpVisual this Draw3D (vectorDir/Up in Eden), as rows (kh_crew_rows).
    bool       d_ok = false;     // This Draw3D's correction (kh_crew_measure): h, D, cr.
    double     h[3] = {};        // The origin helper's read, SQF order.
    float      dm[9] = {};       // D = (the helper's rows)^-1 * cr.
    float      cr[9] = {};       // The character's rows as drawn: tr, or the helper's where tr failed its checks.
    bool       l_ok = false;
    float      l[3] = {};        // The character in the vehicle's frame (its rows, engine axes),
    float      lq[9] = {};       // and its rows in it (cr * the vehicle's rows^-1).
    uint64_t   born = 0;         // g_crew_frame when the entry (and its helper reference) was made.
    uintptr_t  g_hb = 0, g_hvb = 0, g_hbb = 0, g_vb = 0, g_vvb = 0, g_vbb = 0;   // The sampler's page caches.
    uintptr_t  g_havb = 0, g_habb = 0, g_vavb = 0, g_vabb = 0;   // KH_ATTACH_VS_CACHE: their visual attempts'.
    uint32_t   g_hoff = KH_ATTACH_OFF_NONE, g_voff = KH_ATTACH_OFF_NONE;
    uintptr_t  r_hb = 0, r_hvb = 0, r_hbb = 0, r_vb = 0, r_vvb = 0, r_vbb = 0;   // The paint read's (render thread).
    uintptr_t  r_havb = 0, r_habb = 0, r_vavb = 0, r_vabb = 0;   // KH_ATTACH_VS_CACHE: their visual attempts'.
    uint32_t   r_hoff = KH_ATTACH_OFF_NONE, r_voff = KH_ATTACH_OFF_NONE;
    intercept::client::EHIdentifierHandle eh_in, eh_out;
};
static std::unordered_map<uintptr_t, KhCrewEnt>& g_crew = *(new std::unordered_map<uintptr_t, KhCrewEnt>());
// Game thread: characters found not to be a Man (base -> the object's identity), rebuilt by every sync from the
// candidates it saw - an address the engine reuses for another object is asked again.
static std::unordered_map<uintptr_t, const void*>& g_crew_notman = *(new std::unordered_map<uintptr_t, const void*>());
static uint64_t g_crew_serial = 0;   // Game thread; restarted with the table (kh_crew_clear_all).
static uint64_t g_crew_frame = 0;    // Game thread: kh_crew_sync's count; restarted with the table.
// One raw read with a page cache of the entry's own, restarted when the object behind it changes.
inline bool kh_crew_read(const game_value& khcr_gv, uintptr_t& khcr_b, uint32_t& khcr_off, uintptr_t& khcr_vb,
                         uintptr_t& khcr_bb, uintptr_t& khcr_avb, uintptr_t& khcr_abb, float khcr_p[3], float khcr_r[9]) {
    if (khcr_gv.is_nil() || kh_attach_obj_dead(khcr_gv)) return false;
    const uintptr_t khcr_nb = kh_attach_base_of(khcr_gv);
    if (khcr_nb == 0) return false;
    if (khcr_b != khcr_nb) { khcr_b = khcr_nb; khcr_off = KH_ATTACH_OFF_NONE; khcr_vb = 0; khcr_bb = 0; khcr_avb = 0; khcr_abb = 0; }
    return kh_attach_raw(khcr_gv, khcr_off, khcr_vb, khcr_bb, khcr_avb, khcr_abb, khcr_p, khcr_r, nullptr);
}
// A point in a pose's frame and back: positions SQF order, the pose's rows engine axes (row-vector convention, as
// kh_attach_raw hands them back; each row divided by its own length squared, so a scaled pose round-trips too).
inline void kh_crew_local(const float khcl_vp[3], const float khcl_vr[9], const double khcl_t[3], float khcl_l[3]) {
    const double khcl_w[3] = { khcl_t[0] - khcl_vp[0], khcl_t[2] - khcl_vp[2],   // Engine axes.
                               khcl_t[1] - khcl_vp[1] };
    for (int khcl_k = 0; khcl_k < 3; ++khcl_k) {
        const double khcl_x = khcl_vr[khcl_k * 3 + 0], khcl_y = khcl_vr[khcl_k * 3 + 1];
        const double khcl_z = khcl_vr[khcl_k * 3 + 2];
        const double khcl_n = khcl_x * khcl_x + khcl_y * khcl_y + khcl_z * khcl_z;
        khcl_l[khcl_k] = khcl_n > 0.0
            ? static_cast<float>((khcl_x * khcl_w[0] + khcl_y * khcl_w[1] + khcl_z * khcl_w[2]) / khcl_n) : 0.0f;
    }
}
inline void kh_crew_place(const float khcp_vp[3], const float khcp_vr[9], const float khcp_l[3], double khcp_t[3]) {
    double khcp_w[3] = { 0.0, 0.0, 0.0 };   // Engine axes.
    for (int khcp_k = 0; khcp_k < 3; ++khcp_k) {
        for (int khcp_j = 0; khcp_j < 3; ++khcp_j) {
            khcp_w[khcp_j] += static_cast<double>(khcp_l[khcp_k]) * static_cast<double>(khcp_vr[khcp_k * 3 + khcp_j]);
        }
    }
    khcp_t[0] = khcp_vp[0] + khcp_w[0];
    khcp_t[1] = khcp_vp[1] + khcp_w[2];
    khcp_t[2] = khcp_vp[2] + khcp_w[1];
}
// 3x3 rows (row k = axis k, engine axes; a pose's rows multiply on the right): a product, an inverse (false when
// singular), D = a^-1 * b (the identity when a is singular).
inline void kh_crew_mul3(const float khcu_a[9], const float khcu_b[9], float khcu_o[9]) {
    float khcu_t[9];
    for (int khcu_i = 0; khcu_i < 3; ++khcu_i) {
        for (int khcu_j = 0; khcu_j < 3; ++khcu_j) {
            double khcu_s = 0.0;
            for (int khcu_k = 0; khcu_k < 3; ++khcu_k) {
                khcu_s += static_cast<double>(khcu_a[khcu_i * 3 + khcu_k]) *
                          static_cast<double>(khcu_b[khcu_k * 3 + khcu_j]);
            }
            khcu_t[khcu_i * 3 + khcu_j] = static_cast<float>(khcu_s);
        }
    }
    memcpy(khcu_o, khcu_t, sizeof(khcu_t));
}
inline bool kh_crew_inv3(const float khci_m[9], float khci_o[9]) {
    const double a = khci_m[0], b = khci_m[1], c = khci_m[2], d = khci_m[3], e = khci_m[4], f = khci_m[5];
    const double g = khci_m[6], h = khci_m[7], i = khci_m[8];
    const double khci_det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!(std::fabs(khci_det) > 1e-9)) return false;
    const double r = 1.0 / khci_det;
    const double khci_v[9] = { (e * i - f * h) * r, (c * h - b * i) * r, (b * f - c * e) * r,
                               (f * g - d * i) * r, (a * i - c * g) * r, (c * d - a * f) * r,
                               (d * h - e * g) * r, (b * g - a * h) * r, (a * e - b * d) * r };
    for (int k = 0; k < 9; ++k) khci_o[k] = static_cast<float>(khci_v[k]);
    return true;
}
inline void kh_crew_delta(const float khcd_a[9], const float khcd_b[9], float khcd_o[9]) {
    float khcd_i[9];
    if (!kh_crew_inv3(khcd_a, khcd_i)) {
        const float khcd_one[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        memcpy(khcd_o, khcd_one, sizeof(khcd_one));
        return;
    }
    kh_crew_mul3(khcd_i, khcd_b, khcd_o);
}
// A pose read on the character, moved with it: p (SQF order) to t + (p - h) * D, its rows r (null: none) to r * D.
inline void kh_crew_apply(const double khca_t[3], const double khca_h[3], const float khca_d[9], float khca_p[3],
                          float* khca_r) {
    const double khca_v[3] = { khca_p[0] - khca_h[0], khca_p[2] - khca_h[2], khca_p[1] - khca_h[1] };   // Engine.
    double khca_w[3];
    for (int j = 0; j < 3; ++j) {
        khca_w[j] = khca_v[0] * khca_d[j] + khca_v[1] * khca_d[3 + j] + khca_v[2] * khca_d[6 + j];
    }
    khca_p[0] = static_cast<float>(khca_t[0] + khca_w[0]);
    khca_p[1] = static_cast<float>(khca_t[1] + khca_w[2]);
    khca_p[2] = static_cast<float>(khca_t[2] + khca_w[1]);
    if (khca_r) kh_crew_mul3(khca_r, khca_d, khca_r);
}
// The crew pose's dir / up (SQF order) as rows - aside (up x dir), up, dir in engine axes, the order the
// visual state keeps them in (KH_ATTACH_RAW). False when the pair is not a unit, orthogonal pair.
inline bool kh_crew_rows(const float khrw_dir[3], const float khrw_up[3], float khrw_o[9]) {
    const double d[3] = { khrw_dir[0], khrw_dir[2], khrw_dir[1] };
    const double u[3] = { khrw_up[0], khrw_up[2], khrw_up[1] };
    const double dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
    const double du = d[0] * u[0] + d[1] * u[1] + d[2] * u[2];
    if (!(std::fabs(dd - 1.0) < 1e-3 && std::fabs(uu - 1.0) < 1e-3 && std::fabs(du) < 1e-3)) return false;
    const double s[3] = { u[1] * d[2] - u[2] * d[1], u[2] * d[0] - u[0] * d[2], u[0] * d[1] - u[1] * d[0] };
    for (int k = 0; k < 3; ++k) {
        khrw_o[k] = static_cast<float>(s[k]);
        khrw_o[3 + k] = static_cast<float>(u[k]);
        khrw_o[6 + k] = static_cast<float>(d[k]);
    }
    return true;
}
// The entry's helper is placed: a Draw3D after the entry was made (attachTo places a new helper at the next
// simulation step - KH_ATTACH_MAN_HELPER's stamp, counted in syncs, which run whether or not any binding samples).
inline bool kh_crew_placed(const KhCrewEnt& khpl_e) {
    return g_crew_frame > khpl_e.born;
}
// This Draw3D's corrections: kh_crew_sync, under g_draw_list_mutex, after its pose reads - the same instant
// as the sampler's reads and the chain end's (all in this Draw3D, with no engine step between).
inline void kh_crew_measure() {
    for (auto& khcm_kv : g_crew) {
        KhCrewEnt& khcm_e = khcm_kv.second;
        khcm_e.d_ok = false;
        if (!khcm_e.in || !khcm_e.t_ok || !kh_crew_placed(khcm_e)) { khcm_e.l_ok = false; continue; }
        float khcm_p[3], khcm_r[9];
        if (!kh_crew_read(khcm_e.helper, khcm_e.g_hb, khcm_e.g_hoff, khcm_e.g_hvb, khcm_e.g_hbb, khcm_e.g_havb, khcm_e.g_habb, khcm_p, khcm_r)) {
            khcm_e.l_ok = false;
            continue;
        }
        for (int khcm_k = 0; khcm_k < 3; ++khcm_k) khcm_e.h[khcm_k] = khcm_p[khcm_k];
        memcpy(khcm_e.cr, khcm_e.tr_ok ? khcm_e.tr : khcm_r, sizeof(khcm_e.cr));
        kh_crew_delta(khcm_r, khcm_e.cr, khcm_e.dm);
        khcm_e.d_ok = true;
        khcm_e.l_ok = kh_crew_read(khcm_e.veh, khcm_e.g_vb, khcm_e.g_voff, khcm_e.g_vvb, khcm_e.g_vbb, khcm_e.g_vavb, khcm_e.g_vabb, khcm_p, khcm_r);
        float khcm_vi[9];
        if (khcm_e.l_ok) khcm_e.l_ok = kh_crew_inv3(khcm_r, khcm_vi);
        if (khcm_e.l_ok) {
            kh_crew_local(khcm_p, khcm_r, khcm_e.t, khcm_e.l);
            kh_crew_mul3(khcm_e.cr, khcm_vi, khcm_e.lq);
        }
    }
}
// The entry correcting what a binding places from (khco_par, kh_attach_parent_gv) this sample; null: none. By base:
// the sync ahead of this sample dropped every entry whose character was deleted, so a live entry's base is its own.
inline KhCrewEnt* kh_crew_of(const game_value& khco_par) {
    if (g_crew.empty() || khco_par.is_nil()) return nullptr;
    const auto khco_it = g_crew.find(kh_attach_base_of(khco_par));
    return khco_it == g_crew.end() || !khco_it->second.d_ok ? nullptr : &khco_it->second;
}
// The character's vehicle now (nil: on foot). GAME THREAD, holding no lock (an SQF call). KH_CREW_PARENT:
// objectParent - the vehicle a unit rides in, null on foot (an attachTo parent is not one). Not `vehicle`: it answers
// the unit itself on foot, which a test against the character's own base has to tell apart.
inline game_value kh_crew_vehicle(const game_value& khcv_man) {
    try {
        const object khcv_o = sqf::object_parent(static_cast<object>(khcv_man));
        if (!sqf::is_null(khcv_o)) return khcv_o;
    } catch (...) {}
    return game_value();
}
// An event of the character's: crew or not, asked again. GAME THREAD (the engine's event dispatch), holding no lock.
inline void kh_crew_event(uintptr_t khce_b, uint64_t khce_s) {
    const auto khce_it = g_crew.find(khce_b);
    if (khce_it == g_crew.end() || khce_it->second.serial != khce_s) return;
    KhCrewEnt& khce_e = khce_it->second;
    game_value khce_v = kh_crew_vehicle(khce_e.man);
    std::lock_guard<std::mutex> khce_g(g_draw_list_mutex);
    khce_e.veh = khce_v;
    khce_e.in = !khce_v.is_nil();
    khce_e.d_ok = false;
    khce_e.l_ok = false;
}
// The entry's two event handlers (GetInMan, GetOutMan). False when one could not be added: the caller drops the entry
// (its handles remove what was added).
inline bool kh_crew_listen(KhCrewEnt& khcl_e, uintptr_t khcl_b) {
    const uint64_t khcl_s = khcl_e.serial;
    auto khcl_f = [khcl_b, khcl_s](const auto&...) { kh_crew_event(khcl_b, khcl_s); };
    namespace khcl_ic = intercept::client;
    try {
        const object khcl_o = static_cast<object>(khcl_e.man);
        khcl_e.eh_in = khcl_ic::addEventHandler<khcl_ic::eventhandlers_object::GetInMan>(khcl_o, khcl_f);
        khcl_e.eh_out = khcl_ic::addEventHandler<khcl_ic::eventhandlers_object::GetOutMan>(khcl_o, khcl_f);
    } catch (...) {
        return false;
    }
    return true;
}
// The table against this frame's bindings and chain ends, and this Draw3D's positions. GAME THREAD, from flush_frame
// after the reaps and before the sampler, holding no lock across an SQF call. An entry whose character nothing brings
// in any more (KH_CREW_LAG's list: a binding, a rotation lane, a simulating chain's end) or that was deleted drops:
// its helper reference to the reap, its event handlers here.
inline void kh_crew_sync() {
    if (g_attach_n.load(std::memory_order_relaxed) == 0 && g_crew.empty() && !kh_crew_chain_any()) return;
    ++g_crew_frame;
    static std::vector<std::pair<game_value, bool>> khcs_cand;   // (character, has an origin helper: a Man).
    khcs_cand.clear();
    {
        std::lock_guard<std::mutex> khcs_g(g_draw_list_mutex);
        for (const auto& khcs_kv : g_attach) {
            const KhAttach& khcs_a = khcs_kv.second;
            const game_value& khcs_p = kh_attach_parent_gv(khcs_a);
            // A rotation lane's own character (its helper stands in for an object that is not the parent).
            const game_value& khcs_r = khcs_a.obj_rot;
            if (kh_attach_is_obj(khcs_r) && khcs_r.data.get() != khcs_p.data.get() && !khcs_a.org_rot.is_nil() &&
                khcs_a.org_rot_for == khcs_r.data.get()) khcs_cand.emplace_back(khcs_r, true);
            if (!kh_attach_is_obj(khcs_p)) continue;
            const void* const khcs_pd = khcs_p.data.get();
            const bool khcs_org = (!khcs_a.org_pos.is_nil() && khcs_a.org_pos_for == khcs_pd) ||
                                  (!khcs_a.org_rot.is_nil() && khcs_a.org_rot_for == khcs_pd);
            const bool khcs_pxy = khcs_a.skel ? !khcs_a.skel_proxy.empty() : !khcs_a.proxy.is_nil();
            if (khcs_org || khcs_pxy) khcs_cand.emplace_back(khcs_p, khcs_org);
        }
        kh_crew_chain_cand(khcs_cand);
    }
    for (auto& khcs_kv : g_crew) khcs_kv.second.seen = false;
    std::unordered_map<uintptr_t, const void*> khcs_nm;
    for (const auto& khcs_c : khcs_cand) {
        const game_value& khcs_m = khcs_c.first;
        const uintptr_t khcs_b = kh_attach_base_of(khcs_m);
        if (khcs_b == 0 || kh_attach_obj_dead(khcs_m)) continue;
        const auto khcs_it = g_crew.find(khcs_b);
        if (khcs_it != g_crew.end()) {   // By base: two bindings may hold two game values of one object.
            if (!kh_attach_obj_dead(khcs_it->second.man)) khcs_it->second.seen = true;   // Else it drains below.
            continue;
        }
        if (!khcs_c.second) {   // Helpers only (a memory point, a skeleton): a Man?
            const auto khcs_n = g_crew_notman.find(khcs_b);
            bool khcs_man = false;
            if (khcs_n == g_crew_notman.end() || khcs_n->second != khcs_m.data.get()) {
                try { khcs_man = sqf::is_kind_of(static_cast<object>(khcs_m), "Man"); }
                catch (...) { khcs_man = false; }
            }
            if (!khcs_man) { khcs_nm[khcs_b] = khcs_m.data.get(); continue; }
        }
        KhCrewEnt khcs_e;
        khcs_e.man = khcs_m;
        khcs_e.serial = ++g_crew_serial;
        khcs_e.born = g_crew_frame;
        std::string khcs_err;
        try {
            if (!kh_pxy_pool_get(khcs_m, std::string(), khcs_e.helper, "a character's", khcs_err)) {
                khcs_e.helper = game_value();
            }
        } catch (...) {
            khcs_e.helper = game_value();
        }
        if (khcs_e.helper.is_nil()) continue;   // No helper to correct against: its bindings read as before.
        khcs_e.veh = kh_crew_vehicle(khcs_m);
        khcs_e.in = !khcs_e.veh.is_nil();
        if (!kh_crew_listen(khcs_e, khcs_b)) { kh_attach_proxy_orphan(khcs_e.helper); continue; }
        khcs_e.seen = true;
        std::lock_guard<std::mutex> khcs_g(g_draw_list_mutex);
        g_crew.emplace(khcs_b, std::move(khcs_e));
    }
    g_crew_notman.swap(khcs_nm);
    std::vector<KhCrewEnt> khcs_gone;
    {
        std::lock_guard<std::mutex> khcs_g(g_draw_list_mutex);
        for (auto khcs_it = g_crew.begin(); khcs_it != g_crew.end();) {
            if (khcs_it->second.seen && !kh_attach_obj_dead(khcs_it->second.man)) { ++khcs_it; continue; }
            khcs_gone.push_back(std::move(khcs_it->second));
            khcs_it = g_crew.erase(khcs_it);
        }
    }
    for (KhCrewEnt& khcs_e : khcs_gone) kh_attach_proxy_orphan(khcs_e.helper);   // Takes the mutex itself.
    khcs_gone.clear();   // Their event handlers go here, holding nothing.
    const bool khcs_ed = kh_attach_eden();   // KH_ATTACH_EDEN: the simulation's pose, as every read takes.
    for (auto& khcs_kv : g_crew) {
        KhCrewEnt& khcs_e = khcs_kv.second;
        khcs_e.t_ok = false;
        khcs_e.tr_ok = false;
        if (!khcs_e.in) continue;
        try {
            const object khcs_o = static_cast<object>(khcs_e.man);
            const vector3 khcs_dv = khcs_ed ? sqf::vector_dir(khcs_o) : sqf::vector_dir_visual(khcs_o);
            const vector3 khcs_uv = khcs_ed ? sqf::vector_up(khcs_o) : sqf::vector_up_visual(khcs_o);
            const float khcs_df[3] = { khcs_dv.x, khcs_dv.y, khcs_dv.z };
            const float khcs_uf[3] = { khcs_uv.x, khcs_uv.y, khcs_uv.z };
            khcs_e.tr_ok = kh_crew_rows(khcs_df, khcs_uf, khcs_e.tr);   // Else the helper's rows (kh_crew_measure).
        } catch (...) {
            khcs_e.tr_ok = false;
        }
        try {
            const object khcs_mo = static_cast<object>(khcs_e.man);
            const vector3 khcs_v = khcs_ed ? sqf::get_pos_world(khcs_mo) : sqf::get_pos_world_visual(khcs_mo);
            khcs_e.t[0] = khcs_v.x;
            khcs_e.t[1] = khcs_v.y;
            khcs_e.t[2] = khcs_v.z;
            khcs_e.t_ok = std::isfinite(khcs_e.t[0]) && std::isfinite(khcs_e.t[1]) && std::isfinite(khcs_e.t[2]);
        } catch (...) {
            khcs_e.t_ok = false;
        }
    }
    {
        std::lock_guard<std::mutex> khcs_g(g_draw_list_mutex);
        kh_crew_measure();
    }
    khcs_cand.clear();
}
// The mission edge: the table emptied, its event handlers removed outside the mutex; the helper references are
// released, not queued (the pool is emptied with the bindings - kh_attach_drop_all).
inline void kh_crew_clear_all() {
    std::unordered_map<uintptr_t, KhCrewEnt> khca_old;
    {
        std::lock_guard<std::mutex> khca_g(g_draw_list_mutex);
        khca_old.swap(g_crew);
    }
    khca_old.clear();
    g_crew_notman.clear();
    g_crew_serial = 0;
    g_crew_frame = 0;
}

// KH_RT_PAINT_READ - nothing on the render thread waits for a Draw3D but the final step while a binding or a followed
// chain end is on a crew member (see "Wait"). When the frame's own sample is absent (identity known, sample cycle +
// g_gid_k not published) at the cast fire's paint or at the cycle's steps (kh_gts_step_prep), the render thread reads
// the same objects the sampler reads - twice - and a binding takes the read only if every read validated and both
// passes agree bit for bit (the game thread may be writing the object meanwhile). Every render-thread step of the
// cycle draws it (a crew member's prediction, the paint's repick alone); a read the final step drew is counted
// against the frame's own sample once that lands (matched / stale / other).
// KH_PXY_SMOOTH: its memory-point helpers are smoothed as the sampler smooths them (the estimator stepped with the
// frame's own sample number); the check compares the raw reads.
// KH_CREW_LAG: a binding placed from a crew member also reads the character's origin helper and vehicle (both
// passes) and moves its helpers by the prediction, which only the paint's repick draws (the mesh keeps the frame's
// own sample: the final step waits for it); the final step's check of the paint's read counts 'other' wherever the
// prediction and the frame's own measurement differ in a bit.
inline bool kh_rt_own_absent() {
    if (!g_gid_ok) return false;   // The frame's own number unknown: no read (today's pick).
    const uint64_t khra_want = static_cast<uint64_t>(static_cast<int64_t>(g_topo_cycles) + g_gid_k);
    return g_gts_done_pub.load(std::memory_order_acquire) < khra_want;
}
// Render thread, outside g_draw_list_mutex (taken here). The bindings read; 0 = none; KH_RT_OWN_ARRIVED = the own
// sample was published before the mutex was ours (read nothing - the paint's repick takes the sample itself). Under
// the mutex the answer is exact: the sample is published under it (by the sampler, or - with a chain end followed -
// with the chain's targets later in the same Draw3D handler, g_gts_done_defer), and the game thread moves objects
// on to the next frame only after that handler - so a read made while the sample is absent cannot see the next
// frame's pose.
static constexpr uint32_t KH_RT_OWN_ARRIVED = ~0u;
inline uint32_t kh_rt_paint_read() {
    std::lock_guard<std::mutex> khrp_g(g_draw_list_mutex);
    if (!kh_rt_own_absent()) return KH_RT_OWN_ARRIVED;
    const uint64_t khrp_cyc = g_topo_cycles;
    // KH_PXY_SMOOTH: the frame's own sample number.
    const uint64_t khrp_own = static_cast<uint64_t>(static_cast<int64_t>(khrp_cyc) + g_gid_k);
    const uint64_t khrp_seq = khrp_own | KH_RT_SEQ_BIT | ((++g_rtp_read_ser & 0xFFull) << 48);   // KH_RT_READ_SER.
    // KH_RT_READ_SER: a binding's previous read of this cycle - assigned before every read of it. Static, so its
    // vectors keep their capacity; only ever touched under g_draw_list_mutex (held for this whole call).
    static KhGtSnap khrp_prev;
    const int64_t khrp_q = kh_gts_qpc();
    uint32_t khrp_n = 0;
    std::vector<float> khrp_a, khrp_b;
    for (auto& khrp_kv : g_attach) {
        KhAttach& a = khrp_kv.second;
        if (a.rt_par.is_nil()) continue;
        // A helper the game thread deleted since the sampler read it (the sampler's own rule, kh_attach_org_read): its
        // state no longer moves - this binding keeps today's pick for the frame. The rotation lane's object counts
        // only while the lane still follows it (below: one re-pointed since is not read at all).
        if (kh_attach_obj_dead(a.rt_par)) continue;
        const size_t np = a.skel ? a.skel_proxy.size() : (a.proxy.is_nil() ? 0u : 1u);   // 0: the parent alone (the sampler's rule).
        const uint32_t gen = a.skel ? a.skel_gen : 0u;
        const uintptr_t xb = kh_attach_base_of(kh_attach_key_gv(a));
        const uintptr_t pb = kh_attach_base_of(a.rt_par);
        if (xb == 0 || pb == 0) continue;
        // KH_ATTACH_ROT_OF: rt_par names the binding the last Draw3D sampled; one a command re-bound since is not
        // read (its objects are another binding's) until the next Draw3D names them - the pick holds meanwhile.
        if (a.rt_for_gen != gen || a.rt_for_xb != xb) continue;
        if (a.rt_gen != gen || a.rt_xb != xb || a.rt_off.size() != np) {   // The render thread's own page caches.
            a.rt_off.assign(np, KH_ATTACH_OFF_NONE);
            a.rt_vb.assign(np, 0u);
            a.rt_bb.assign(np, 0u);
            a.rt_avb.assign(np, 0u);
            a.rt_abb.assign(np, 0u);
            a.rt_gen = gen;
            a.rt_xb = xb;
        }
        if (a.rt_pb != pb) {
            a.rt_pb = pb; a.rt_par_off = KH_ATTACH_OFF_NONE; a.rt_par_vb = 0; a.rt_par_bb = 0; a.rt_par_avb = 0; a.rt_par_abb = 0;
        }
        // KH_ATTACH_ROT_OF: rt_rot only while the rotation lane still follows what it was named for.
        const bool rcur = !a.rt_rot.is_nil() && a.rt_rot_for == kh_attach_base_of(a.obj_rot);
        if (rcur && kh_attach_obj_dead(a.rt_rot)) continue;   // The helper rule above, for the object it reads.
        const uintptr_t rb = rcur ? kh_attach_base_of(a.rt_rot) : 0u;
        if (a.rt_rb != rb) {
            a.rt_rb = rb; a.rt_rot_off = KH_ATTACH_OFF_NONE; a.rt_rot_vb = 0; a.rt_rot_bb = 0; a.rt_rot_avb = 0; a.rt_rot_abb = 0;
        }
        // KH_CREW_LAG: a crew member's origin helper and vehicle too, once a sample has placed it in that vehicle.
        KhCrewEnt* cw = nullptr;
        if (!g_crew.empty()) {
            const game_value& pg = kh_attach_parent_gv(a);
            const auto cit = g_crew.find(kh_attach_base_of(pg));
            if (cit != g_crew.end() && cit->second.in && cit->second.l_ok) cw = &cit->second;
        }
        // KH_CREW_LAG: the rotation lane's own character, where its helper is what the lane reads: its vehicle too.
        KhCrewEnt* cwr = nullptr;
        if (rb != 0u && !g_crew.empty()) {
            const auto cit = g_crew.find(kh_attach_base_of(a.obj_rot));
            if (cit != g_crew.end() && cit->second.in && cit->second.l_ok &&
                a.rt_rot.data.get() == cit->second.helper.data.get()) cwr = &cit->second;
        }
        const size_t nc = np + 2u + (cw ? 2u : 0u);   // Parent, helpers, rotation object (, helper, vehicle).
        const size_t n = (nc + (cwr ? 1u : 0u)) * 12u;   // (, the rotation lane's character's vehicle)
        khrp_a.assign(n, 0.0f);
        khrp_b.assign(n, 0.0f);
        auto read_all = [&](float* out, bool& rot_ok) -> bool {
            if (!kh_attach_raw(a.rt_par, a.rt_par_off, a.rt_par_vb, a.rt_par_bb, a.rt_par_avb, a.rt_par_abb, out, out + 3,
                               nullptr)) return false;
            for (size_t j = 0; j < np; ++j) {
                const game_value& x = a.skel ? a.skel_proxy[j] : a.proxy;
                float* const o = out + (j + 1u) * 12u;
                if (!kh_attach_raw(x, a.rt_off[j], a.rt_vb[j], a.rt_bb[j], a.rt_avb[j], a.rt_abb[j], o, o + 3, nullptr)) return false;
            }
            rot_ok = false;
            if (rb != 0u) {
                float* const o = out + (np + 1u) * 12u;
                rot_ok = kh_attach_raw(a.rt_rot, a.rt_rot_off, a.rt_rot_vb, a.rt_rot_bb, a.rt_rot_avb, a.rt_rot_abb, o, o + 3,
                                       nullptr);
            }
            if (cw) {   // KH_CREW_LAG.
                float* const o = out + (np + 2u) * 12u;
                if (!kh_crew_read(cw->helper, cw->r_hb, cw->r_hoff, cw->r_hvb, cw->r_hbb, cw->r_havb, cw->r_habb, o, o + 3) ||
                    !kh_crew_read(cw->veh, cw->r_vb, cw->r_voff, cw->r_vvb, cw->r_vbb, cw->r_vavb, cw->r_vabb, o + 12, o + 15)) return false;
            }
            if (cwr) {   // KH_CREW_LAG.
                float* const o = out + nc * 12u;
                if (!kh_crew_read(cwr->veh, cwr->r_vb, cwr->r_voff, cwr->r_vvb, cwr->r_vbb, cwr->r_vavb, cwr->r_vabb, o, o + 3)) return false;
            }
            return true;
        };
        bool rot_a = false, rot_b = false;
        if (!read_all(khrp_a.data(), rot_a) || !read_all(khrp_b.data(), rot_b)) { { if (kh_stats_on()) ++g_rtp_c.refused; } continue; }
        if (rot_a != rot_b || memcmp(khrp_a.data(), khrp_b.data(), n * sizeof(float)) != 0) { { if (kh_stats_on()) ++g_rtp_c.torn; } continue; }
        KhGtSnap& sn = a.rt_snap;
        const bool khrp_again = a.rt_cyc == khrp_cyc && sn.ok && sn.gen == gen && sn.xb == xb;   // KH_RT_READ_SER.
        if (khrp_again) khrp_prev = sn;
        sn.ok = true;
        sn.rot_ok = rot_a;
        sn.seq = khrp_seq;
        sn.qpc = khrp_q;
        sn.gen = gen;
        sn.xb = xb;
        memcpy(sn.par, khrp_a.data(), sizeof(sn.par));
        sn.px.assign(khrp_a.begin() + 12, khrp_a.begin() + 12 + static_cast<ptrdiff_t>(np * 12u));
        if (rot_a) memcpy(sn.rot, khrp_a.data() + (np + 1u) * 12u, sizeof(sn.rot));
        sn.rot_of = rot_a ? a.rt_rot_for : 0u;   // KH_ATTACH_ROT_OF.
        if (cw) {   // KH_CREW_LAG: the prediction - the character where its last offset in the vehicle puts it now.
            const float* const hc = khrp_a.data() + (np + 2u) * 12u;   // The helper's pose, then the vehicle's.
            double tp[3];
            kh_crew_place(hc + 12, hc + 15, cw->l, tp);
            float cr[9], dm[9];
            kh_crew_mul3(cw->lq, hc + 15, cr);   // Its rows in the vehicle, on the vehicle as it is now.
            kh_crew_delta(hc + 3, cr, dm);
            const double hd[3] = { hc[0], hc[1], hc[2] };
            const bool ph = a.rt_par.data.get() == cw->helper.data.get();
            if (ph) kh_crew_apply(tp, hd, dm, sn.par, sn.par + 3);
            if (ph || !a.skel) {
                for (size_t j = 0; j < np; ++j) kh_crew_apply(tp, hd, dm, &sn.px[j * 12u], &sn.px[j * 12u + 3u]);
            }
        }
        if (cwr && rot_a) {   // KH_CREW_LAG: the rotation lane's character, likewise predicted.
            const float* const vc = khrp_a.data() + nc * 12u;
            double tp[3];
            kh_crew_place(vc, vc + 3, cwr->l, tp);
            for (int k = 0; k < 3; ++k) sn.rot[k] = static_cast<float>(tp[k]);
            kh_crew_mul3(cwr->lq, vc + 3, sn.rot + 3);
        }
        // KH_PXY_SMOOTH: the memory-point helpers through the estimator, stepped with the frame's own sample number -
        // once for the frame: the Draw3D sampler, reading the same pose later, is handed this result back. A crew
        // member's predicted helpers stay raw (the repick's alone).
        sn.pxd.assign(np * 3u, 0.0f);
        if (!cw) {
            for (size_t j = 0; j < np; ++j) {
                const game_value& x = a.skel ? a.skel_proxy[j] : a.proxy;
                kh_pxy_smooth_note(x, khrp_own, khrp_q, &sn.px[j * 12u], &sn.pxd[j * 3u]);
            }
        }
        // KH_RT_READ_SER: the same pose read again keeps its number - the steps' stash and skin stand.
        if (khrp_again && kh_gts_same(khrp_prev, sn) && khrp_prev.pxd == sn.pxd) sn.seq = khrp_prev.seq;
        a.rt_crew = cw != nullptr || cwr != nullptr;
        a.rt_cyc = khrp_cyc;
        ++khrp_n;
    }
    if (kh_stats_on()) g_rtp_c.bindings += khrp_n;
    kh_chain_end_rt_read(khrp_cyc, khrp_own, khrp_q);   // KH_CHAIN_END_DRAW: the followed ends, likewise.
    return khrp_n;
}
// Every render-thread step, before kh_attach_step takes g_draw_list_mutex (KH_ATTACH_GT_SNAP: "Own or read" and
// "Wait"). Once a cycle, the identity offset relearned from the previous cycle. Then, with the frame's own sample not
// landed, the render thread's read of every binding - at the cycle's first step, and again at its final step
// (kh_rt_paint_read takes the mutex itself; a sample that lands first makes it read nothing, and the pick takes the
// sample). At the final step, while a binding or a followed chain end is on a crew member (g_gts_crew_n), the
// frame's own sample is waited for first.
inline void kh_gts_step_prep() {
    const bool khsp_final = g_attach_final_step;
    if (g_gts_learn_cyc != g_topo_cycles) {
        g_gts_learn_cyc = g_topo_cycles;
        kh_gts_id_learn();
    }
    uint64_t& khsp_done = khsp_final ? g_gts_final_cyc : g_gts_first_rd_cyc;
    if (khsp_done == g_topo_cycles) return;
    khsp_done = g_topo_cycles;
    if (!kh_rt_own_absent()) return;
    if (khsp_final && g_gts_crew_n.load(std::memory_order_relaxed) != 0u) {   // KH_CREW_LAG: as before the reads.
        const uint64_t khsp_want = static_cast<uint64_t>(static_cast<int64_t>(g_topo_cycles) + g_gid_k);
        // KH_CREW_SILENT: only while the frame's own sample is the NEXT one the sampler publishes (a late Draw3D).
        // Further ahead the sampler is silent - the engine renders with the window unfocused and Draw3D does not
        // fire - or a frame or more behind (the offset stale on its return): the wait would run to its cap every
        // frame (~9 fps) and end without the sample, so the frame draws the render thread's read at once, as a
        // wait that reached the cap does. g_gts_crew_n keeps the sampler's last count while it is silent.
        if (g_gts_done_pub.load(std::memory_order_acquire) + 1u >= khsp_want) {
            const int64_t khsp_until = g_gts_clear_qpc +
                                       static_cast<int64_t>(kh_prof_freq()) * KH_GTS_CREW_CAP_MS / 1000;
            while (g_gts_done_pub.load(std::memory_order_acquire) < khsp_want && kh_gts_qpc() < khsp_until) {
                std::this_thread::yield();
            }
        }
        if (!kh_rt_own_absent()) return;
    }
    if (khsp_final && kh_stats_on()) ++g_rtp_c.finals;
    (void)kh_rt_paint_read();
}
// kh_attach_step's tail for a skeletal binding: the draw centre from its root, now the rotation (and the skin's box)
// is final.
inline void kh_attach_skel_place(RenderObject& khsp_o, const double khsp_root[3], bool& khsp_moved) {
    double khsp_c[3] = { khsp_root[0], khsp_root[1], khsp_root[2] };   // KH_POS_RES: exact to the draw.
    kh_skel_centre_d(khsp_o, khsp_c);
    if (kh_pos_set_exact(khsp_o, khsp_c)) khsp_moved = true;
}
// kh_attach_step's tail for every binding, and kh_chain_end_draw's for a chain end it re-placed: a moved mesh re-files
// its scene slot, a final step that moved it reopens the sun map (KH_SUN_RESTEP), and a repick step that moved it has
// the cast fire rebuild the sun map before its paint (KH_CAST_REPICK).
inline void kh_attach_moved_note(const RenderObject& khmn_o, bool khmn_moved, bool khmn_rt) {
    if (!khmn_moved) return;
    kh_scene_mark(khmn_o.slot);
    if (g_attach_final_step && khmn_rt) g_sun_restep_cycle = g_topo_cycles;
    if (g_attach_repick_step && khmn_rt) g_attach_repick_moved = true;   // KH_CAST_REPICK.
}
// KH_PXY_SMOOTH - a memory-point helper's position through a bounded estimator. The engine places an attached object
// by a world position it stores as float, so a helper on a memory point carries that position's rounding: steps of
// the float spacing there (0.49 mm at 4-8 km from the map origin, 0.98 mm at 8-16 km, 1.95 mm at 16-32 km; height
// stays small), which an idle bone crosses back and forth - the dance the engine's own attached objects show too.
// The rounding is bounded - the true point lies within half a spacing of the reading on each axis - and the output
// never leaves one spacing of the reading. Per SQF axis, per helper, once per Draw3D sample, on the reading as the
// sampler took it:
//   target  - it stays where it is while it lies within half a spacing of the reading, else it moves: to the reading
//             itself when it moves the way it last moved (a drift - the boundary it just crossed is behind it),
//             otherwise to the nearest point within half a spacing (a reversal, which is where dither lives): a
//             reading dithering across a boundary leaves it still;
//   tracker - an alpha-beta tracker follows the target: position and velocity, critically damped at a time
//             constant that eases from KH_PXY_SMOOTH_TAU_SLOW at or under KH_PXY_SMOOTH_SPD_SLOW spacings a second
//             to KH_PXY_SMOOTH_TAU_FAST at or over KH_PXY_SMOOTH_SPD_FAST (its own speed), gains from the sample's
//             real interval: it turns the target's steps into motion at the pace the steps set. It may never
//             step against the way the target last moved (a type-2 tracker overshoots every step it is given, and
//             the swing back is a new dance on slow breathing): such a step is held, and a velocity against that
//             way is dropped with it;
//   clamp   - the output is kept within one spacing of the reading, and a clamp takes the velocity with it;
//   reset   - a reading more than KH_PXY_SMOOTH_RESET spacings from the last one (a teleport) is the output, and
//             everything starts again from it.
// The velocity is the output's own recent motion, used only to carry the output between readings: nothing is shown
// that the current reading rules out (the clamp), and nothing is extrapolated past a reading. Measured offline over
// dither, idle breathing (clean and noisy), drift, sway of 20 to 200 spacings, a walk and a run, at 30 / 60 / 144 fps
// with 20 % frame jitter, 5 to 25 km (per-frame jerk against the truth, spacings, rms / max, 60 fps; raw -> this):
// dither 0.71 / 1.00 -> 0.01 / 0.02; breathing 0.20 / 0.98 -> 0.08 / 0.39 with the true count of reversals; sway
// 0.39 / 0.99 -> 0.16 / 0.46; drift 0.40 / 0.82 -> 0.10 / 0.21; a walk 0.40 / 0.98 -> 0.18 / 0.50. Only an instant
// step from rest to a run jerks once past raw (1.5 spacings). The envelope: within one spacing of the engine's
// reading at any speed, a teleport exact. The output travels as a delta from the reading (KhGtSnap::pxd - at most a
// spacing, which a float holds finely) to the step, which places the mesh with it in double (KH_POS_RES) and a
// skeletal binding's bones with it (kh_skin_rel).
// The estimator runs in the sampler, for a chain's end on a memory point (kh_chain_prepare, on the sampler's sample,
// or one of its own) and in the render thread's read (KH_RT_PAINT_READ, with the frame's own sample number, whose
// result the sampler is handed back) - on the pool entry, so every binding and chain end on one memory point moves
// alike - always under g_pxy_pool_mu, and it makes no engine call. An origin helper (no memory point) and a followed
// object are not estimated: each is the point the engine draws its object from, not a rounded offset from it.
static constexpr double KH_PXY_SMOOTH_RESET = 64.0;     // Spacings one sample may step before the state restarts.
static constexpr double KH_PXY_SMOOTH_TAU_SLOW = 0.10;  // The tracker's time constant (s), at rest ...
static constexpr double KH_PXY_SMOOTH_TAU_FAST = 0.05;  // ... and moving;
static constexpr double KH_PXY_SMOOTH_SPD_SLOW = 5.0;   // at or under this speed (spacings a second) the first,
static constexpr double KH_PXY_SMOOTH_SPD_FAST = 30.0;  // at or over this the second, linear between.
// A chain end's own sample number, taken when this flush_frame's sampler took no sample (kh_chain_prepare): this
// bit and the pass's stamp - never a sampler's (small counts) nor a render thread read's (KH_RT_SEQ_BIT, bit 62),
// new every pass.
static constexpr uint64_t KH_PXY_SEQ_CHAIN = 1ull << 61;
// The float spacing at khps_r (one unit in the last place of its binade): half of it bounds the rounding of any value
// that rounds to khps_r - at a power of two the spacing below is the smaller one.
inline double kh_pxy_spacing(float khps_r) {
    uint32_t khps_b = 0;
    memcpy(&khps_b, &khps_r, sizeof(khps_b));
    const int khps_e = static_cast<int>((khps_b >> 23) & 0xFFu);
    return std::ldexp(1.0, (khps_e == 0 ? 1 : khps_e) - 150);
}
// One helper's reading (khsm_r, SQF order) in the Draw3D sample khsm_seq, stamped khsm_q (kh_gts_qpc): khsm_d = the
// output minus the reading. A second call for the same sample with the same reading (another binding on the same
// memory point, the sampler reading what the render thread read) hands back the first's. KH_PXY_REPLAY: a second
// call for the same sample with ANOTHER reading - the render thread's reads within one cycle can see different poses
// (KH_RT_READ_SER), and all carry the frame's own number - steps again from the state before that sample's step:
// the estimator still steps once per sample (never twice at a near-zero interval), on the frame's latest reading. Do
// not hand back the first reading's delta there: it puts every later reading off its track by it.
inline void kh_pxy_smooth(KhPxyPoolEnt& khsm_e, uint64_t khsm_seq, int64_t khsm_q, const float khsm_r[3],
                          float khsm_d[3]) {
    if (khsm_e.sm_ok && khsm_e.sm_seq == khsm_seq) {
        if (khsm_e.sm_r[0] == khsm_r[0] && khsm_e.sm_r[1] == khsm_r[1] && khsm_e.sm_r[2] == khsm_r[2]) {
            memcpy(khsm_d, khsm_e.sm_d, sizeof(khsm_e.sm_d));
            return;
        }
        khsm_e.sm_ok = khsm_e.smp_ok;   // KH_PXY_REPLAY: back to before this sample's step.
        khsm_e.sm_q = khsm_e.smp_q;
        memcpy(khsm_e.sm_dir, khsm_e.smp_dir, sizeof(khsm_e.sm_dir));
        memcpy(khsm_e.sm_r, khsm_e.smp_r, sizeof(khsm_e.sm_r));
        memcpy(khsm_e.sm_t, khsm_e.smp_t, sizeof(khsm_e.sm_t));
        memcpy(khsm_e.sm_x, khsm_e.smp_x, sizeof(khsm_e.sm_x));
        memcpy(khsm_e.sm_v, khsm_e.smp_v, sizeof(khsm_e.sm_v));
    } else {
        khsm_e.smp_ok = khsm_e.sm_ok;   // KH_PXY_REPLAY: the state this sample's step starts from.
        khsm_e.smp_q = khsm_e.sm_q;
        memcpy(khsm_e.smp_dir, khsm_e.sm_dir, sizeof(khsm_e.smp_dir));
        memcpy(khsm_e.smp_r, khsm_e.sm_r, sizeof(khsm_e.smp_r));
        memcpy(khsm_e.smp_t, khsm_e.sm_t, sizeof(khsm_e.smp_t));
        memcpy(khsm_e.smp_x, khsm_e.sm_x, sizeof(khsm_e.smp_x));
        memcpy(khsm_e.smp_v, khsm_e.sm_v, sizeof(khsm_e.smp_v));
    }
    if (!kh_cloth_finite3(khsm_r)) {
        khsm_d[0] = khsm_d[1] = khsm_d[2] = 0.0f;
        khsm_e.sm_ok = false;   // Nothing to hold to: the next reading starts it again.
        return;
    }
    const double khsm_dt = khsm_e.sm_ok && khsm_q > khsm_e.sm_q
                         ? static_cast<double>(khsm_q - khsm_e.sm_q) / static_cast<double>(kh_prof_freq()) : 0.0;
    for (int k = 0; k < 3; ++k) {
        const double khsm_x = static_cast<double>(khsm_r[k]);
        const double khsm_s = kh_pxy_spacing(khsm_r[k]);
        const double khsm_h = 0.5 * khsm_s;
        double& khsm_t = khsm_e.sm_t[k];
        double& khsm_o = khsm_e.sm_x[k];
        double& khsm_v = khsm_e.sm_v[k];
        if (!khsm_e.sm_ok || !(fabs(khsm_x - static_cast<double>(khsm_e.sm_r[k])) <= KH_PXY_SMOOTH_RESET * khsm_s)) {
            khsm_t = khsm_x;   // reset: a teleport, or the first reading.
            khsm_o = khsm_x;
            khsm_v = 0.0;
            khsm_e.sm_dir[k] = 0;
        } else {
            const double khsm_lo = khsm_x - khsm_h;
            const double khsm_hi = khsm_x + khsm_h;
            const int8_t khsm_mv = khsm_t < khsm_lo ? 1 : (khsm_t > khsm_hi ? -1 : 0);   // target.
            if (khsm_mv != 0) {
                khsm_t = khsm_mv == khsm_e.sm_dir[k] ? khsm_x : (khsm_mv > 0 ? khsm_lo : khsm_hi);
                khsm_e.sm_dir[k] = khsm_mv;
            }
            if (khsm_dt > 0.0) {   // tracker.
                const double khsm_sp = fabs(khsm_v) / khsm_s;
                double khsm_u = (khsm_sp - KH_PXY_SMOOTH_SPD_SLOW) / (KH_PXY_SMOOTH_SPD_FAST - KH_PXY_SMOOTH_SPD_SLOW);
                khsm_u = khsm_u < 0.0 ? 0.0 : (khsm_u > 1.0 ? 1.0 : khsm_u);
                const double khsm_w =
                    1.0 / (KH_PXY_SMOOTH_TAU_SLOW + (KH_PXY_SMOOTH_TAU_FAST - KH_PXY_SMOOTH_TAU_SLOW) * khsm_u);
                const double khsm_a = 1.0 - exp(-2.0 * khsm_w * khsm_dt);
                const double khsm_bc = khsm_a * khsm_a / (2.0 - khsm_a);   // Critical damping.
                const double khsm_b = (khsm_w * khsm_dt) * (khsm_w * khsm_dt) < khsm_bc
                                    ? (khsm_w * khsm_dt) * (khsm_w * khsm_dt) : khsm_bc;
                const double khsm_o0 = khsm_o;
                const double khsm_p = khsm_o + khsm_v * khsm_dt;
                const double khsm_err = khsm_t - khsm_p;
                khsm_o = khsm_p + khsm_a * khsm_err;
                khsm_v += (khsm_b / khsm_dt) * khsm_err;
                const double khsm_g = static_cast<double>(khsm_e.sm_dir[k]);   // Never against the target's way.
                if (khsm_g != 0.0 && (khsm_o - khsm_o0) * khsm_g < 0.0) {
                    khsm_o = khsm_o0;
                    if (khsm_v * khsm_g < 0.0) khsm_v = 0.0;
                }
            }
            const double khsm_c = khsm_o < khsm_x - khsm_s ? khsm_x - khsm_s
                                : (khsm_o > khsm_x + khsm_s ? khsm_x + khsm_s : khsm_o);   // clamp.
            if (khsm_c != khsm_o) {
                if (khsm_dt > 0.0) khsm_v += (khsm_c - khsm_o) / khsm_dt;
                khsm_o = khsm_c;
            }
        }
        khsm_e.sm_r[k] = khsm_r[k];
        khsm_d[k] = static_cast<float>(khsm_o - khsm_x);
    }
    khsm_e.sm_ok = true;
    khsm_e.sm_seq = khsm_seq;
    khsm_e.sm_q = khsm_q;
    memcpy(khsm_e.sm_d, khsm_d, sizeof(khsm_e.sm_d));
}
// The estimator's entry for one helper read - the sampler's and kh_chain_prepare's (game thread), kh_rt_paint_read's
// and kh_chain_end_rt_read's (render thread): the estimator's delta for a memory-point helper the pool holds, zero for
// anything else (an origin helper, a proxy the pool never filed).
inline void kh_pxy_smooth_note(const game_value& khpn_x, uint64_t khpn_seq, int64_t khpn_q, const float khpn_r[3],
                               float khpn_d[3]) {
    khpn_d[0] = khpn_d[1] = khpn_d[2] = 0.0f;
    const uintptr_t khpn_b = kh_attach_base_of(khpn_x);
    if (khpn_b == 0) return;
    std::lock_guard<std::mutex> khpn_l(g_pxy_pool_mu);   // Both threads step (KH_RT_PAINT_READ).
    const auto khpn_it = g_pxy_pool.find(khpn_b);
    if (khpn_it == g_pxy_pool.end() || !khpn_it->second.bone) return;
    kh_pxy_smooth(khpn_it->second, khpn_seq, khpn_q, khpn_r, khpn_d);
}
// KH_ATTACH_GT_SNAP - GAME THREAD, flush_frame (the Draw3D handler), once a frame and under g_draw_list_mutex:
// every binding's objects read TOGETHER - the parent (or a plain binding's object, or the object a rotation alone
// follows), the proxies, the rotation lane's own object - through kh_attach_raw's copy rule, with page caches of this
// function's own. Draw3D is the instant the engine's own drawn state is coherent (a Draw3D icon at an object's visual
// position never falters). A failed PARENT read leaves the binding without a sample this frame (the step draws the
// ring's, or holds); a failed helper read is carried from the previous sample (KH_GTS_HOLD); a failed read of the
// rotation lane's own object leaves the rotation holding. g_gts_done_pub is published once every binding is written
// (with a chain end followed, once that Draw3D's chain targets are filed too: g_gts_done_defer).
// True when this call took a sample (g_gts_seq names it): kh_chain_prepare's choice (KH_PXY_SMOOTH).
inline bool kh_attach_gt_snap() {
    if (!kh_attach_live()) return false;   // KH_CHAIN_END_DRAW: a followed chain end is sampled by number too.
    std::lock_guard<std::mutex> khgt_g(g_draw_list_mutex);
    if (g_gts_done_defer != 0u) {   // KH_CHAIN_END_DRAW: a deferral a thrown pass never published, first.
        g_gts_done_pub.store(g_gts_done_defer, std::memory_order_release);
        g_gts_done_defer = 0u;
    }
    const uint64_t khgt_seq = ++g_gts_seq;
    const int64_t khgt_q = kh_gts_qpc();
    uint32_t khgt_crew_n = 0;   // KH_CREW_LAG: bindings placed on a crew member, then chain ends (g_gts_crew_n).
    for (auto& khgt_kv : g_attach) {
        KhAttach& khgt_a = khgt_kv.second;
        // The object the binding is placed relative to, and the helpers it follows. KH_ATTACH_ROT_ONLY: a binding with
        // no position lane is sampled on its rotation object, which the step then takes as the parent's.
        // KH_ATTACH_MAN_HELPER: khgt_lpar is that object, khgt_par what is read for it.
        const game_value& khgt_lpar = kh_attach_parent_gv(khgt_a);
        const size_t khgt_np = khgt_a.skel ? khgt_a.skel_proxy.size() : (khgt_a.proxy.is_nil() ? 0u : 1u);
        // A skeletal binding with no proxies (a mesh with no skeleton, or none of whose bones matched a memory point)
        // is sampled for its parent alone: its root, and so all of its geometry, follows the object.
        if (khgt_lpar.is_nil()) continue;
        const game_value& khgt_par = kh_attach_org_read(khgt_a, khgt_lpar, khgt_seq);
        khgt_a.rt_par = khgt_par;        // KH_RT_PAINT_READ: what the render thread reads for the parent.
        khgt_a.rt_rot = game_value();    // Set below when the rotation lane follows an object of its own.
        khgt_a.rt_rot_for = 0u;          // KH_ATTACH_ROT_OF: likewise.
        const uint32_t khgt_gen = khgt_a.skel ? khgt_a.skel_gen : 0u;
        const uintptr_t khgt_xb = kh_attach_base_of(kh_attach_key_gv(khgt_a));
        const uintptr_t khgt_pb = kh_attach_base_of(khgt_par);
        khgt_a.rt_for_gen = khgt_gen;    // KH_ATTACH_ROT_OF: the binding rt_par is named for.
        khgt_a.rt_for_xb = khgt_xb;
        if (khgt_xb == 0 || khgt_pb == 0) continue;
        // KH_ATTACH_SEED_PARENT: a memory-point proxy is placed by the simulation step after its attachTo
        // (kh_attach_org_read's rule, made_seq + 1). The sample that first sees a new one is not taken, and the
        // render thread does not read it (kh_rt_paint_read's rt_for_xb test): the mesh holds its seed.
        if (!khgt_a.skel && khgt_np == 1u) {
            const auto khgt_pe = g_pxy_pool.find(kh_attach_base_of(khgt_a.proxy));
            if (khgt_pe != g_pxy_pool.end() && khgt_seq <= khgt_pe->second.made_seq + 1u) {
                khgt_a.rt_for_xb = 0u;
                continue;
            }
        }
        // The rotation lane, when it follows an object of its own (neither the parent nor the one helper) - named for
        // the render thread's read here, ahead of the reads below, so a Draw3D whose parent or helper read is refused
        // still leaves it named (as rt_par is) and the render thread's read turns it; its helper is stamped as the
        // parent's is above (kh_attach_org_read: the same sample number, once).
        const void* const khgt_rd = khgt_a.obj_rot.is_nil() ? nullptr : khgt_a.obj_rot.data.get();
        const bool khgt_rown = khgt_rd && khgt_rd != khgt_lpar.data.get() &&
                               !(khgt_np == 1u && !khgt_a.skel && khgt_rd == khgt_a.proxy.data.get());
        if (khgt_rown) {
            khgt_a.rt_rot = kh_attach_org_read(khgt_a, khgt_a.obj_rot, khgt_seq);   // KH_ATTACH_MAN_HELPER.
            khgt_a.rt_rot_for = kh_attach_base_of(khgt_a.obj_rot);   // KH_ATTACH_ROT_OF.
        }
        if (khgt_a.gts_gen != khgt_gen || khgt_a.gts_xb != khgt_xb || khgt_a.gts_off.size() != khgt_np) {
            khgt_a.gts_off.assign(khgt_np, KH_ATTACH_OFF_NONE);   // Another binding on this handle: its caches are not ours.
            khgt_a.gts_vb.assign(khgt_np, 0u);
            khgt_a.gts_bb.assign(khgt_np, 0u);
            khgt_a.gts_avb.assign(khgt_np, 0u);
            khgt_a.gts_abb.assign(khgt_np, 0u);
            khgt_a.gts_gen = khgt_gen;
            khgt_a.gts_xb = khgt_xb;
        }
        if (khgt_a.gts_pb != khgt_pb) {
            khgt_a.gts_pb = khgt_pb;
            khgt_a.gts_par_off = KH_ATTACH_OFF_NONE;
            khgt_a.gts_par_vb = 0;
            khgt_a.gts_par_bb = 0;
            khgt_a.gts_par_avb = 0;
            khgt_a.gts_par_abb = 0;
        }
        KhGtSnap& khgt_s = khgt_a.gts[khgt_a.gts_w % KH_GTS_N];
        khgt_s.ok = false;   // Not a sample until every read of it lands.
        khgt_s.rot_ok = false;
        khgt_s.rot_of = 0u;   // KH_ATTACH_ROT_OF.
        float khgt_p[3], khgt_r[9];
        if (!kh_gts_read(khgt_par, khgt_a.gts_par_off, khgt_a.gts_par_vb, khgt_a.gts_par_bb, khgt_a.gts_par_avb,
                         khgt_a.gts_par_abb, khgt_p, khgt_r)) continue;
        memcpy(khgt_s.par, khgt_p, sizeof(khgt_p));
        memcpy(khgt_s.par + 3, khgt_r, sizeof(khgt_r));
        // KH_CREW_LAG: a crew member's helpers moved by its correction - the parent where its origin helper is read.
        KhCrewEnt* const khgt_cw = kh_crew_of(khgt_lpar);
        if (khgt_cw && khgt_cw->in) ++khgt_crew_n;
        if (khgt_cw && khgt_par.data.get() == khgt_cw->helper.data.get()) {
            kh_crew_apply(khgt_cw->t, khgt_cw->h, khgt_cw->dm, khgt_s.par, khgt_s.par + 3);
        }
        // The helpers on it: a memory point always (its parent read is a check), a skeleton's bones with the parent.
        const bool khgt_cwx = khgt_cw && (!khgt_a.skel || khgt_par.data.get() == khgt_cw->helper.data.get());
        khgt_s.px.resize(khgt_np * 12u);
        khgt_s.pxd.assign(khgt_np * 3u, 0.0f);   // KH_PXY_SMOOTH: zero = the reading as it stands.
        // KH_GTS_HOLD: the binding's previous sample, written before this one (another ring slot), when it is this
        // binding's - a refused helper read is carried from it; with none (the first sample after a bind), as before.
        const KhGtSnap& khgt_prev = khgt_a.gts[(khgt_a.gts_w + KH_GTS_N - 1u) % KH_GTS_N];
        const bool khgt_prev_ok = khgt_a.gts_w != 0u && khgt_prev.ok && khgt_prev.gen == khgt_gen &&
                                  khgt_prev.xb == khgt_xb && khgt_prev.px.size() == khgt_np * 12u;
        bool khgt_ok = true;
        for (size_t khgt_j = 0; khgt_j < khgt_np; ++khgt_j) {
            const game_value& khgt_x = khgt_a.skel ? khgt_a.skel_proxy[khgt_j] : khgt_a.proxy;
            if (!kh_gts_read(khgt_x, khgt_a.gts_off[khgt_j], khgt_a.gts_vb[khgt_j], khgt_a.gts_bb[khgt_j],
                             khgt_a.gts_avb[khgt_j], khgt_a.gts_abb[khgt_j], khgt_p, khgt_r)) {
                if (!khgt_prev_ok) { khgt_ok = false; break; }
                kh_gts_carry(khgt_prev.par, khgt_s.par, &khgt_prev.px[khgt_j * 12u], &khgt_s.px[khgt_j * 12u]);
                if (khgt_prev.pxd.size() == khgt_np * 3u) {   // KH_PXY_SMOOTH: the carried pose keeps its delta.
                    memcpy(&khgt_s.pxd[khgt_j * 3u], &khgt_prev.pxd[khgt_j * 3u], 3u * sizeof(float));
                }
                continue;
            }
            if (khgt_cwx) {   // KH_CREW_LAG: every helper of the character, ahead of the estimator.
                kh_crew_apply(khgt_cw->t, khgt_cw->h, khgt_cw->dm, khgt_p, khgt_r);
            }
            memcpy(&khgt_s.px[khgt_j * 12u], khgt_p, sizeof(khgt_p));
            memcpy(&khgt_s.px[khgt_j * 12u + 3u], khgt_r, sizeof(khgt_r));
            kh_pxy_smooth_note(khgt_x, khgt_seq, khgt_q, khgt_p, &khgt_s.pxd[khgt_j * 3u]);   // KH_PXY_SMOOTH.
        }
        if (!khgt_ok) continue;
        // The rotation lane's own object (named above).
        if (khgt_rown) {
            const game_value& khgt_ro = khgt_a.rt_rot;   // KH_ATTACH_MAN_HELPER; KH_RT_PAINT_READ.
            const uintptr_t khgt_rb = kh_attach_base_of(khgt_ro);
            if (khgt_a.gts_rb != khgt_rb) {
                khgt_a.gts_rb = khgt_rb;
                khgt_a.gts_rot_off = KH_ATTACH_OFF_NONE;
                khgt_a.gts_rot_vb = 0;
                khgt_a.gts_rot_bb = 0;
                khgt_a.gts_rot_avb = 0;
                khgt_a.gts_rot_abb = 0;
            }
            if (kh_gts_read(khgt_ro, khgt_a.gts_rot_off, khgt_a.gts_rot_vb, khgt_a.gts_rot_bb, khgt_a.gts_rot_avb,
                            khgt_a.gts_rot_abb, khgt_p, khgt_r)) {
                memcpy(khgt_s.rot, khgt_p, sizeof(khgt_p));
                memcpy(khgt_s.rot + 3, khgt_r, sizeof(khgt_r));
                khgt_s.rot_ok = true;
                khgt_s.rot_of = khgt_a.rt_rot_for;   // KH_ATTACH_ROT_OF.
                // KH_CREW_LAG: a crew member of its own, moved (so turned) as that character is drawn.
                const KhCrewEnt* const khgt_cr = kh_crew_of(khgt_a.obj_rot);
                if (khgt_cr && khgt_cr->in && !(khgt_cw && khgt_cw->in)) ++khgt_crew_n;
                if (khgt_cr && khgt_ro.data.get() == khgt_cr->helper.data.get()) {
                    kh_crew_apply(khgt_cr->t, khgt_cr->h, khgt_cr->dm, khgt_s.rot, khgt_s.rot + 3);
                }
            }
        }
        khgt_s.gen = khgt_gen;
        khgt_s.xb = khgt_xb;
        khgt_s.seq = khgt_seq;
        khgt_s.qpc = khgt_q;
        khgt_s.ok = true;
        ++khgt_a.gts_w;
    }
    khgt_crew_n += kh_chain_end_crew_n();   // KH_CREW_LAG: a followed chain end on a crew member waits too.
    g_gts_crew_n.store(khgt_crew_n, std::memory_order_relaxed);   // KH_CREW_LAG.
    g_gts_stamp_pub[khgt_seq % KH_GTS_STAMPS].store(khgt_q, std::memory_order_relaxed);   // Before the seq names it.
    if (g_chain_end_n.load(std::memory_order_relaxed) != 0u) {
        g_gts_done_defer = khgt_seq;   // KH_CHAIN_END_DRAW: published with the chain ends' targets.
    } else {
        g_gts_done_pub.store(khgt_seq, std::memory_order_release);   // Every binding written: kh_rt_own_absent.
    }
    return true;
}

// Runs from TEN sites, each immediately ahead of the scene read its draws are built from: on the render thread the
// injection, the cast fire (and its two KH_CAST_REPICK steps ahead of the paint), the volume seam inject and the
// replay's presync of it (kh_rp_replay, KH_RP_PRESYNC), the in-front prepare and seam inject, and flush_locked's step
// when the injection did not land; on the game thread kh_park_run's. The THREADS note on g_attach: this is the half
// either thread may run, and the half that may never erase. A lane is written only where the sampled value differs
// from the one the mesh already carries - the compare is exact, and a still object hands back the same floats every
// frame - so a parked vehicle costs the compare and nothing else: no scene mark, no grid move, no record upload.
// khap_ctx: a render-thread drawer's context (the cast fire's step, the injection's, the render-thread flush's, the
// footprint seam's and its replay presync's, and the view-model passes' - KH_SEAM_SKIN), with which a skeletal
// binding is skinned from the stash once per cycle and sample, a cloth's pinned part re-placed and a chain's end
// corrected; the park's step passes none.
// Every pose comes from the cycle's pick (kh_gts_for_cycle): a Draw3D sample, or the render thread's read standing
// in for an absent one (KH_RT_PAINT_READ: read at the cycle's first step, the cast fire's paint and the final step).
// The step reads no object itself, and a binding with no sample this cycle holds the pose it has.
inline void kh_attach_step(ID3D11DeviceContext* khap_ctx = nullptr) {
    KH_PROF_SCOPE(KHP_ATTACH_STEP);   // KH_PROF.
    if (!kh_attach_live()) return;   // KH_CHAIN_END_DRAW: a followed chain end steps with no binding.
    if (reorder_on_render_thread() && g_gts_first_cyc != g_topo_cycles) {   // KH_CAST_REPICK: the cycle's first step.
        g_gts_first_cyc = g_topo_cycles;
        g_gts_first_done = g_gts_done_pub.load(std::memory_order_acquire);
    }
    if (reorder_on_render_thread()) kh_gts_step_prep();   // Before the mutex (the read takes it).
    std::lock_guard<std::mutex> khap_g(g_draw_list_mutex);
    static std::vector<KhSkinDrawJob> khap_jobs;   // Under g_draw_list_mutex (steps never overlap).
    khap_jobs.clear();
    for (auto khap_it = g_attach.begin(); khap_it != g_attach.end(); ++khap_it) {
        auto khap_e = g_draw_list.find(khap_it->first);
        // An orphan is SKIPPED, never erased: this runs on the render thread too, and erasing would release the
        // entry's game_values there. Every site that erases a draw-list entry drops its attachment in the same
        // critical section (kh_attach_drop), so an orphan cannot outlive the entry by more than that moment.
        if (khap_e == g_draw_list.end()) continue;
        KhAttach& khap_a = khap_it->second;
        RenderObject& khap_o = khap_e->second;
        // KH_ATTACH_CYCLE: this cycle's consume-point sample stands. A game-thread step (kh_park_run's) compares the
        // published counter, not the render thread's.
        const bool khap_rt = reorder_on_render_thread();
        if (!khap_rt && khap_a.smp_cycle == g_topo_cycles_pub.load(std::memory_order_relaxed)) continue;
        // KH_ATTACH_DEAD: the object this mesh follows is gone. Queue the handle and read nothing further from it -
        // QUEUED, not erased (the THREADS note on g_attach). Either lane counts: a mesh following a turret that no
        // longer exists has nothing left to follow, whatever its other lane says. KH_ATTACH_BONE adds bone_parent,
        // not redundant with the two lanes: in bone mode the lanes hold the PROXY, and a proxy outlives its parent
        // (attachTo leaves it standing at the parent's last position, readable, forever). Without this third test a
        // mesh following a bone of a deleted vehicle would never die.
        if (kh_attach_obj_dead(khap_a.obj_pos) || kh_attach_obj_dead(khap_a.obj_rot) ||
            kh_attach_obj_dead(khap_a.bone_parent)) {
            kh_attach_dead_queue(khap_it->first);
            continue;
        }
        // Both lanes on the same object is the common case; it takes one read.
        const bool khap_same = !khap_a.obj_pos.is_nil() && !khap_a.obj_rot.is_nil() &&
                               khap_a.obj_pos.data.get() == khap_a.obj_rot.data.get();
        float khap_p[3], khap_r[9];
        double khap_root[3] = { 0.0, 0.0, 0.0 };   // KH_SKEL: the root (KH_POS_RES: exact), past the rotation read.
        // KH_ATTACH_GT_SNAP: this cycle's sample of everything this binding follows, taken once for both lanes, on
        // either thread (a game-thread step reads the render thread's cycle, which the park holds still).
        const KhGtSnap* const khap_gs =
            kh_gts_for_cycle(khap_a, khap_rt ? g_topo_cycles : g_topo_cycles_pub.load(std::memory_order_relaxed), khap_rt);
        if (!khap_gs) continue;
        bool  khap_raw = false;
        bool  khap_moved = false;
        if (!khap_a.obj_pos.is_nil()) {
            // The parent of a skeletal binding, the followed object of a plain one, the helper of a memory-point one.
            khap_raw = true;
            const float* const khap_src = khap_a.skel || khap_a.bone_parent.is_nil() ? khap_gs->par : khap_gs->px.data();
            memcpy(khap_p, khap_src, sizeof(khap_p));
            memcpy(khap_r, khap_src + 3, sizeof(khap_r));
            // KH_POS_RES: the position exact, in double - a memory-point binding's helper where the estimator puts
            // it (KH_PXY_SMOOTH: the sample's deltas, a render-thread read's included), the offset below composed
            // onto it - and stored as a world float plus its residual (kh_pos_set_exact).
            double khap_pd[3] = { khap_p[0], khap_p[1], khap_p[2] };
            if (khap_src != khap_gs->par && khap_gs->pxd.size() >= 3u) {
                for (int khap_k = 0; khap_k < 3; ++khap_k) khap_pd[khap_k] += static_cast<double>(khap_gs->pxd[khap_k]);
            }
            // KH_SKEL_SAMPLE: a skeletal binding's bones, stashed with the parent read beside them - one pair, the
            // one the skinning makes the bones relative to. A hidden mesh is stashed only when something other than
            // the draw reads its pose - its cloth's guide or its collider (the sync's own rule; skin->cloth /
            // want_collide are written by the sync under this mutex) - or it casts as a casterOnly (KH_SKIN_CASTER),
            // or its buffer stands culled (KH_SKIN_CULL: decided again at every step, whatever its flags); this step
            // skins it (the KH_SKEL block below) only in the last two cases - for a guide or a collider alone it skins
            // nothing (the worker path serves those). Shown again, the next step of the cycle stashes and skins it
            // before any draw.
            const bool khap_pose = kh_skin_pose_drawn(khap_o) || khap_a.skin_culled ||   // KH_SKIN_CASTER, _CULL.
                                   (khap_a.skin && kh_skin_pose_read(*khap_a.skin));
            if (khap_a.skel && khap_rt && khap_pose &&
                !(khap_ctx && khap_a.skel_smp_ok && khap_a.skel_smp_cycle == g_topo_cycles &&
                  khap_a.skel_smp_seq == khap_gs->seq)) {   // KH_SKEL_DRAW: this cycle's pose stands.
                khap_a.skel_smp_seq = khap_gs->seq;
                const size_t khap_np = khap_a.skel_proxy.size();
                khap_a.skel_smp_gen = khap_a.skel_gen;
                khap_a.skel_smp.resize(khap_np * 12u);
                if (khap_np != 0u) memcpy(khap_a.skel_smp.data(), khap_gs->px.data(), khap_np * 12u * sizeof(float));
                memcpy(khap_a.skel_smp_par, khap_gs->par, sizeof(float) * 12u);
                khap_a.skel_smp_d.assign(khap_np * 3u, 0.0f);   // KH_PXY_SMOOTH: the bones' deltas (none: zero).
                if (khap_np != 0u && khap_gs->pxd.size() == khap_np * 3u) {
                    memcpy(khap_a.skel_smp_d.data(), khap_gs->pxd.data(), khap_np * 3u * sizeof(float));
                }
                khap_a.skel_smp_ok = true;
                khap_a.skel_smp_ms = steady_now_ms();
            }
            // KH_ATTACH_OFFSET, composed BEFORE the compare: a mesh with an offset still costs nothing on a frame its
            // object did not move, and a CHANGED offset needs no special case (the composed value stops matching what
            // the mesh carries). khap_r is the position object's own basis, read here before the rotation lane below
            // can overwrite it.
            kh_attach_offset_pos_d(khap_o, khap_r, khap_pd);   // KH_POS_RES.
            // KH_SKEL: a skeletal mesh is drawn from its root plus a centre that turns with the rotation the block
            // below may still change, so its write waits for that block (kh_skel_centre_d).
            if (khap_a.skel) {
                memcpy(khap_root, khap_pd, sizeof(khap_root));
            } else if (kh_pos_set_exact(khap_o, khap_pd)) {
                khap_moved = true;
            }
        }
        if (!khap_a.obj_rot.is_nil()) {
            // Same object as the position lane: that read already has it. Otherwise the rotation lane's own object
            // from the sample - its own read, the one helper, or the parent; not in the sample, the rotation holds.
            bool khap_rok = khap_same;
            if (!khap_rok) {
                const float* khap_rs = nullptr;
                // KH_ATTACH_ROT_OF: the sample's own object only while the lane still follows it.
                if (khap_gs->rot_ok && khap_gs->rot_of == kh_attach_base_of(khap_a.obj_rot)) khap_rs = khap_gs->rot;
                else if (!khap_a.skel && !khap_a.bone_parent.is_nil() && !khap_a.proxy.is_nil() &&
                         khap_a.obj_rot.data.get() == khap_a.proxy.data.get() && khap_gs->px.size() >= 12u) khap_rs = khap_gs->px.data();
                else if (khap_a.obj_rot.data.get() == kh_attach_parent_gv(khap_a).data.get()) khap_rs = khap_gs->par;
                if (khap_rs) {
                    khap_rok = true;
                    memcpy(khap_p, khap_rs, sizeof(khap_p));
                    memcpy(khap_r, khap_rs + 3, sizeof(khap_r));
                }
            }
            // KH_ATTACH_OFFSET. In place on khap_r, which the position block above has finished with (it used the
            // basis without changing it; on the shared-object path this IS that basis).
            if (khap_rok) kh_attach_offset_rot(khap_o, khap_r);
            if (khap_rok && memcmp(khap_o.rot_m, khap_r, sizeof(khap_o.rot_m)) != 0) {
                memcpy(khap_o.rot_m, khap_r, sizeof(khap_o.rot_m));
                khap_o.rotated = kh_attach_rotated(khap_r);
                khap_moved = true;
            }
        }
        bool khap_defer = false;   // KH_SKIN_FORK: this binding's tail waits for its vertices.
        if (khap_raw && khap_a.skel) {   // KH_SKEL: the deferred position, now the rotation is final.
            if (khap_ctx && (kh_skin_pose_drawn(khap_o) || khap_a.skin_culled) &&   // KH_SKIN_CASTER, KH_SKIN_CULL.
                khap_a.skel_smp_ok &&
                (khap_a.skel_smp_cycle != g_topo_cycles || khap_a.skel_draw_seq != khap_a.skel_smp_seq)) {
                // KH_SKEL_DRAW: once per cycle and sample, drawn meshes only.
                const uint64_t khap_prev_draw = khap_a.skel_draw_seq;   // KH_SKIN_CULL: a culled job hands these back.
                const uint64_t khap_prev_cyc = khap_a.skel_smp_cycle;
                khap_a.skel_draw_seq = khap_a.skel_smp_seq;
                if (kh_skin_draw_prep(khap_a, khap_o, khap_ctx)) {
                    khap_jobs.push_back(KhSkinDrawJob{ &khap_it->first, &khap_a, &khap_o,
                                                       { khap_root[0], khap_root[1], khap_root[2] },
                                                       { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, khap_moved, false,
                                                       khap_rt, KhVbRef{}, false, false, khap_prev_cyc,
                                                       khap_prev_draw });
                    khap_defer = true;
                }
                kh_cloth_pin_draw_step(khap_a, khap_o, khap_ctx, khap_moved);   // A simulating mesh's bound part.
                if (khap_defer) khap_jobs.back().moved = khap_moved;
                khap_a.skel_smp_cycle = g_topo_cycles;
            }
            if (!khap_defer) kh_attach_skel_place(khap_o, khap_root, khap_moved);
        }
        if (!khap_defer) kh_attach_moved_note(khap_o, khap_moved, khap_rt);
        if (khap_rt) khap_a.smp_cycle = g_topo_cycles;   // KH_ATTACH_CYCLE.
    }
    if (khap_jobs.empty()) {
        kh_chain_end_draw(khap_ctx);   // KH_CHAIN_END_DRAW: after every binding is placed.
        return;
    }
    // KH_SKIN_CULL: the jobs no pass can draw this cycle are culled - their bound taken, nothing drawn into their
    // buffers - ahead of the batch.
    kh_skin_cull_pass(khap_ctx, khap_jobs);
    // KH_SKIN_GPU: the bindings the GPU can skin are skinned there (so_vb set, their bound in bc / bs); the pool takes
    // what is left - none, once the stream-output objects and the bindings' buffers exist.
    kh_skin_so_batch(khap_ctx, khap_jobs);
    bool khap_cpu = false;
    {
        uint32_t khap_ng = 0u, khap_nc = 0u, khap_nx = 0u;
        for (const KhSkinDrawJob& khap_j : khap_jobs) {
            if (khap_j.culled) ++khap_nx;
            else if (khap_j.so_vb) ++khap_ng;
            else { ++khap_nc; khap_cpu = true; }
        }
        kh_skin_stat_note(khap_ng, khap_nc, khap_nx);   // skinGpu / skinCpu / skinCulled.
    }
    // KH_SKIN_FORK: the vertices of every binding this step skins, one chunk per binding, across the cloth pool with
    // this thread taking chunks too (serial, in order, when the pool is busy with another fork). Helpers leave for a
    // queued cloth step (the pool's cloth-first rule). A chunk that throws (an allocation) loses only its own
    // binding's upload this step.
    auto khap_chunk = [](uint32_t khap_i, uint32_t) {
        KhSkinDrawJob& khap_j = khap_jobs[khap_i];
        if (khap_j.so_vb) { khap_j.ok = true; return; }   // KH_SKIN_GPU: skinned already.
        try { kh_skin_draw_verts(*khap_j.a, khap_j.bc, khap_j.bs); khap_j.ok = true; }
        catch (...) { khap_j.ok = false; }
    };
    if (khap_cpu) {
        kh_cloth_fork_run(static_cast<uint32_t>(khap_jobs.size()), kh_cloth_fork_workers() + 1u,
                          &kh_cloth_fork_call<decltype(khap_chunk)>, &khap_chunk, true);
    } else {
        for (KhSkinDrawJob& khap_j : khap_jobs) khap_j.ok = true;   // KH_SKIN_GPU: nothing for the pool.
    }
    // The tails, in loop order. KH_SKIN_CULL: a culled job's finish takes the box, the shape key and the stamps
    // exactly as a skinned one's - only the bytes are the buffer's older ones.
    for (KhSkinDrawJob& khap_j : khap_jobs) {
        if (khap_j.ok) kh_skin_draw_finish(*khap_j.h, *khap_j.a, *khap_j.o, khap_ctx, khap_j.bc, khap_j.bs, khap_j.moved, khap_j.so_vb);
        if (khap_j.ok) khap_j.a->skin_culled = khap_j.culled;
        else if (khap_j.a->skin_culled) {   // KH_SKIN_CULL: older bytes stand under a newer box: decided again.
            khap_j.a->skel_smp_cycle = khap_j.prev_smp_cycle;
            khap_j.a->skel_draw_seq = khap_j.prev_draw_seq;
        }
        kh_attach_skel_place(*khap_j.o, khap_j.root, khap_j.moved);
        kh_attach_moved_note(*khap_j.o, khap_j.moved, khap_j.rt);
    }
    khap_jobs.clear();
    kh_chain_end_draw(khap_ctx);   // KH_CHAIN_END_DRAW: after every binding is placed and skinned.
}
}