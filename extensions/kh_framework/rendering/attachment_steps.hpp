#pragma once

// The injection half (rendering_integration_injection.hpp): the attachment steps: the game-thread samples, CREW_LAG,
// the render-thread paint read, proxy smoothing, attach_gt_snap and attach_step. Never include it on its own.
namespace RenderIntegration {
inline void chain_end_rt_read(uint64_t cyc, uint64_t own, int64_t q);   // Defined with the chain.
inline void chain_end_draw(ID3D11DeviceContext* ctx);                            // Likewise.
inline uint32_t chain_end_crew_n();                                                   // Likewise (CREW_LAG).
// CHAIN's side of the census (defined with chain_prepare, rendering/chain_ends.hpp): whether any chain is set up,
// and the characters a simulating chain's end follows. Game thread; the second under g_draw_list_mutex.
inline bool crew_chain_any();
inline void crew_chain_cand(std::vector<std::pair<game_value, bool>>& out);
inline void pxy_smooth_note(const game_value& x, uint64_t seq, int64_t q, const float r[3],
                            float d[3]);   // PXY_SMOOTH, defined with the estimator below.

// ATTACH_FINAL: raised around a cycle's final attach_step (the injection's, or the render-thread flush's when the
// injection did not run); attach_moved_note reads it (SUN_RESTEP). Render thread only, raised only through
// FinalStepScope so a throw cannot leave it up.
static bool g_attach_final_step = false;
struct FinalStepScope {
    FinalStepScope() { g_attach_final_step = true; }
    ~FinalStepScope() { g_attach_final_step = false; }
    FinalStepScope(const FinalStepScope&) = delete;
    FinalStepScope& operator=(const FinalStepScope&) = delete;
};
// CAST_REPICK: raised around the step the cast fire runs just before it paints our shadow into the engine's mask
// (mask_cast_engine). That step picks again, as the final step does, but never waits; g_attach_repick_moved notes that
// it moved something, so the fire rebuilds the sun map from the new poses before painting. g_gts.first_done is the
// newest published sample when the cycle's first render-thread step ran (g_gts.first_cyc): a paint with nothing newer
// has nothing to pick again. Render thread only; the flag is raised only through RepickStepScope.
static bool g_attach_repick_step = false;
static bool g_attach_repick_moved = false;
static constexpr uint32_t GTS_STAMPS = 8u;
// GTS: the Draw3D sample clock the attachments are stepped by - the cycle's first sample, the clear's timing and
// the half period, the published stamps, the crew count. (The sample sequence itself, g_gts_seq, is the attachments'
// own: attachments.hpp.)
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: done_pub, stamp_pub, crew_n, done_defer.
// Session reset: session_objects_reset resets 3 of them. session_scratch_reset resets 9 of them.
struct GtsState {
    uint64_t first_cyc = ~0ull, first_done = 0;
    // ATTACH_GT_SNAP - which sample a cycle draws (GTS_NEAREST). The Draw3D sampler (attach_gt_snap) samples
    // every binding's objects once a frame; the step draws one of those samples and never reads an object itself. Own =
    // a frame's Draw3D lands within a few milliseconds of that frame's main depth clear - before it or after it, and
    // which side flips with the load - while every other frame's lands most of a frame away. That is how the frame's
    // own sample is recognised: the offset between sample numbers and cycles is learned from it (GTS_IDENTITY,
    // below) and carried from cycle to cycle, relearned every cycle, and a cycle's own sample is sample number cycle +
    // that offset. Before the offset is known, the sample stamped nearest the clear within half a frame period either
    // side stands in (g_gts.half: half the shorter of the last two frames, so a hitch or a pause does not stretch it).
    // Wait = none, but for a binding or a followed chain end on a crew member: the render thread does not wait for a
    // Draw3D. On some terrains a frame's own Draw3D runs only after that frame's injection (cytech_underground_terrain,
    // measured below), so a wait for it there ends only at its cap, and a wait at all can hold the game thread back
    // from its Draw3D. A crew member's exact pose is the Draw3D's own measurement (CREW_LAG's pose commands), so
    // while one is bound, or a chain's end follows one (chain_end_crew_n), the final step waits for that frame's
    // sample, up to GTS_CREW_CAP_MS after the clear (gts_step_prep) - and only while that sample is the next the
    // sampler will publish (CREW_SILENT: a silent or lagging sampler is not waited for). Own or read = every
    // render-thread step draws the frame's own sample by number when it has landed (GTS_IDENTITY, below); when it
    // has not, the render thread reads the objects itself - at the cycle's first step and again at its final step
    // (gts_step_prep) - and the step draws that read, so the mesh and everything drawn from it in the cycle (the sun
    // and dynamic-light maps, the footprint and stencil seams, the view-model slice, the world cast's paint) take the
    // frame's pose (RT_PAINT_READ, below). Before the offset is known (a session's first cycles) a step draws the
    // sample stamped nearest the clear within half a period, else the newest before the clear (B). Final = the cast
    // fire's first step (0.03 ms after the clear) takes the frame's own sample if it has landed, else the render
    // thread's read ("Own or read"): in a light scene the frame's Draw3D lands after it (measured: 0.3-1.7 ms after the
    // clear). The final step picks again; a pose that changes there re-skins its skeletal mesh and reopens the sun map
    // (SUN_RESTEP). Paint = the engine reads its shadow mask right after the fire paints our shadow into it
    // (0.08-0.47 ms after the clear). The fire picks again just before that paint when a sample has landed since the
    // cycle's first step (CAST_REPICK) and, with the frame's own still absent, reads the objects itself
    // (RT_PAINT_READ). Measured (the engine's diag_frameNo against the render cycle): three runs - a heavy scene
    // (21.7 ms frames), a light one after a vehicle exit (10.4 ms) and one flipping between 20 ms and 10 ms frames
    // across an exit - 756 cycles, each frame's own Draw3D within 3.1 ms before or 2.6 ms after its clear (at most 26 %
    // of a frame), the nearest sample the frame's own on every one. Do not go back to the rules this replaced (the
    // newest sample before the clear as the pick, with an aim carried from cycle to cycle, write-offs and fallbacks):
    // they drew 321 of them a frame behind after vehicle exits. Measured on cytech_underground_terrain: the own Draw3D
    // landed 1-3 ms after the clear, after the final step (about 0.6 ms). Do not wait for every binding's sample by
    // number: capped at 100 ms, such waits ended at the cap 13 times in about 7 s, the sample landing just after each,
    // and after them a half period swollen past 14 ms let the previous frame's sample be learned as the frame's own, so
    // every cycle drew one frame behind (1,230 in one run).
    int64_t  clear_qpc = 0;                 // Render thread: this cycle's main depth clear (gts_qpc).
    int64_t  clear_prev = 0;                // The clear before it.
    int64_t  half = 0;                      // Half the frame period at this clear (0: not known yet).
    std::atomic<uint64_t> done_pub{ 0 };   // Game thread: the seq whose sampler has written every binding.
    // Game thread: seq's stamp at [seq % GTS_STAMPS], stored before g_gts.done_pub names it.
    std::atomic<int64_t> stamp_pub[GTS_STAMPS];
    uint64_t final_cyc = ~0ull;             // The cycle whose final step has run gts_step_prep.
    uint64_t learn_cyc = ~0ull;     // Render thread: the cycle whose offset has been learned.
    uint64_t first_rd_cyc = ~0ull;  // Render thread: the cycle whose first step has run its read.
    // CREW_LAG: bindings the last Draw3D sample placed on a crew member, and followed chain ends on one (the
    // sampler; the render thread reads it); GTS_CREW_CAP_MS is how long after the clear the final step waits for the
    // frame's own sample while there is one.
    std::atomic<uint32_t> crew_n{ 0 };
    // With a chain end followed, the sampler's number is published with that Draw3D's chain targets (at the end of
    // chain_prepare, chain_end_publish, still inside the Draw3D handler and under g_draw_list_mutex): a step that
    // saw the sample landed but not its chain targets would draw the chain's end a frame old. Game thread.
    uint64_t done_defer = 0;
};
static GtsState g_gts;
struct RepickStepScope {
    RepickStepScope() { g_attach_repick_step = true; }
    ~RepickStepScope() { g_attach_repick_step = false; }
    RepickStepScope(const RepickStepScope&) = delete;
    RepickStepScope& operator=(const RepickStepScope&) = delete;
};
// SUN_RESTEP - the cycle in which the final step (ATTACH_FINAL: the injection's, or the render-thread flush's
// when the injection did not run) moved or re-skinned (SKIN_RESTEP_KEY: its shape key changed) a followed object
// after the cast fire's step had built the sun map, which our meshes' self-shadow also samples.
// inject_composited_meshes (or flush_locked, for its late draws) reopens the map for that cycle (it re-renders only if
// its input hash changed).
// Cost: a second ladder render on such cycles. The final step picks the cycle's sample again (gts_for_cycle), so
// it moves a binding wherever the frame's own sample landed after the fire's step - most cycles of a light scene - as
// well as where a mesh was shown or edited between the two. The world cast is painted once a frame, into the engine's
// own mask, and cannot be repainted: the fire picks again just before that paint instead (CAST_REPICK).
static uint64_t g_sun_restep_cycle = ~0ull;

inline int64_t gts_qpc() { LARGE_INTEGER q; QueryPerformanceCounter(&q); return q.QuadPart; }
// The main depth clear (render thread): this cycle's boundary, and half the frame period - the shorter of the last two
// frames, so one long frame (a hitch, a pause) does not widen the window.
inline void gts_clear_note(int64_t now) {
    const int64_t a = g_gts.clear_qpc;
    const int64_t b = g_gts.clear_prev;
    g_gts.clear_prev = a;
    g_gts.clear_qpc = now;
    if (b > 0 && a > b && now > a) {
        const int64_t d1 = now - a;
        const int64_t d2 = a - b;
        g_gts.half = (d1 < d2 ? d1 : d2) / 2;
    } else {
        g_gts.half = 0;
    }
}
// The object a binding is placed relative to: a memory-point binding's parent, else its position lane's object, else
// (a binding that follows a rotation alone - ATTACH_ROT_ONLY) its rotation lane's object.
inline const game_value& attach_parent_gv(const Attach& a) {
    if (!a.skel && !a.bone_parent.is_nil()) return a.bone_parent;
    return a.obj_pos.is_nil() ? a.obj_rot : a.obj_pos;
}
// The object a binding's samples are keyed on (GtSnap::xb): its position lane's object, else its rotation lane's.
inline const game_value& attach_key_gv(const Attach& a) {
    return a.obj_pos.is_nil() ? a.obj_rot : a.obj_pos;
}
// ATTACH_MAN_HELPER: the object the Draw3D sampler reads for obj - the lane's helper where one stands in for
// it and has been attached (read after its stamp, the first sample that saw it: that sample reads the character,
// because attachTo places a new helper at the next simulation step; a lane with no stamp stamps its own first
// sample), else the object itself - which is also what a helper something else deleted falls back to (the
// character's own state: right outside a vehicle). Game thread, under g_draw_list_mutex.
inline const game_value& attach_org_read(Attach& a, const game_value& obj, uint64_t seq) {
    const void* const p = obj.data.get();
    if (!p) return obj;
    if (!a.org_pos.is_nil() && a.org_pos_for == p) {
        if (a.org_pos_seq == 0u) a.org_pos_seq = seq;
        return seq > a.org_pos_seq && !attach_obj_dead(a.org_pos) ? a.org_pos : obj;
    }
    if (!a.org_rot.is_nil() && a.org_rot_for == p) {
        if (a.org_rot_seq == 0u) a.org_rot_seq = seq;
        return seq > a.org_rot_seq && !attach_obj_dead(a.org_rot) ? a.org_rot : obj;
    }
    return obj;
}
// The sampler's read of one object, its validation noted for getRenderStats (ATTACH_DIAG).
inline bool gts_read(const game_value& gv, uint32_t& off, uintptr_t& vb, uintptr_t& bb,
                     uintptr_t& avb, uintptr_t& abb, float pos[3], float rot[9]) {
    AttachDiag dg;
    const bool ok = attach_raw(gv, off, vb, bb, avb, abb, pos, rot, &dg);
    attach_diag_note(ok, dg);
    return ok;
}
// GTS_HOLD - a helper whose read was refused this frame (attachRefused) keeps the pose it had RELATIVE TO ITS PARENT
// in the binding's previous sample, carried onto this frame's parent: the rest of the binding stays current and that
// one helper holds its local pose for the frame, instead of the whole binding going without a sample - which the pick
// then draws as a held frame, a visible jump. Poses are 12 floats: position (SQF order), then the basis rows (engine
// axes, orthonormal as attach_vs_eval hands them back). out must not alias an input.
inline void gts_carry(const float pp[12], const float pn[12], const float x[12], float out[12]) {
    const float d[3] = { x[0] - pp[0], x[2] - pp[2], x[1] - pp[1] };   // Engine axes.
    const float* const rp = pp + 3;
    const float* const rn = pn + 3;
    float w[3] = { 0.0f, 0.0f, 0.0f };   // The offset, taken into the new parent's frame.
    for (int k = 0; k < 3; ++k) {
        const float l = d[0] * rp[k * 3] + d[1] * rp[k * 3 + 1] + d[2] * rp[k * 3 + 2];
        for (int c = 0; c < 3; ++c) w[c] += l * rn[k * 3 + c];
    }
    out[0] = pn[0] + w[0];
    out[1] = pn[1] + w[2];   // SQF north = engine z.
    out[2] = pn[2] + w[1];   // SQF up = engine y.
    for (int i = 0; i < 3; ++i) {   // Each basis row, re-expressed from the old parent's frame in the new one's.
        const float* const r = x + 3 + i * 3;
        float* const o = out + 3 + i * 3;
        o[0] = o[1] = o[2] = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float l = r[0] * rp[k * 3] + r[1] * rp[k * 3 + 1] + r[2] * rp[k * 3 + 2];
            for (int c = 0; c < 3; ++c) o[c] += l * rn[k * 3 + c];
        }
    }
}
// GTS_IDENTITY - render cycles and Draw3D samples run one for one, so the frame's own sample is sample number
// cycle + g_gid_k. The offset is learned once a cycle, at its first render-thread step, from the PREVIOUS cycle,
// whose own Draw3D has landed by then (gts_id_learn): the one sample stamped within GTS_OWN_US and within
// half a period of that cycle's clear, when there is exactly one. The window is fixed: after a stall the half period
// swelled past 14 ms and let the previous frame's sample, stamped 12.65 ms before the clear, pass for the frame's
// own - after which every cycle drew one frame behind. Own samples landed between 3.1 ms before and 3.2 ms after
// their clear in every measured run. A learning that changes the offset is a break (getRenderStats
// "identityBreaks").
static constexpr int64_t GTS_OWN_US = 4000;
static int64_t  g_gid_k = 0;                 // Render thread: sample number - cycle.
static bool     g_gid_ok = false;            // The offset is known (learned since the session start).
static uint64_t g_gid_breaks = 0;            // getRenderStats "identityBreaks".
// CREW_LAG: how long after the clear the final step waits for the frame's own sample while there is a crew binding
// (g_gts.crew_n).
static constexpr int64_t GTS_CREW_CAP_MS = 100;
// CHAIN_END_DRAW: chains whose end follows an object or a memory point, is fixed in the world or is held on a
// skeletal object's bone (g_chain_end, game thread under g_draw_list_mutex; chain_end_filed). With one, the
// Draw3D sampler and the steps run even with no binding: the frame's identity and the render thread's read serve a
// followed end as they serve a binding.
static std::atomic<uint32_t> g_chain_end_n{ 0 };
inline bool attach_live() {
    return g_attach_n.load(std::memory_order_relaxed) != 0u || g_chain_end_n.load(std::memory_order_relaxed) != 0u;
}
// RT_PAINT_READ: getRenderStats "paintRead" - [paints that tried a read, bindings read, torn (the two reads
// disagreed), refused (a read failed validation), then each read against the frame's own sample once it has landed:
// matched, stale (equal to the previous frame's sample), other; and final steps that read for the mesh].
struct RtpCounters {
    uint64_t paints = 0, bindings = 0, torn = 0, refused = 0, matched = 0, stale = 0, other = 0, finals = 0;
};
static RtpCounters g_rtp_c;
static uint64_t g_rtp_cyc = ~0ull;                          // The cycle whose paint has run its read.
static constexpr uint64_t RT_SEQ_BIT = 1ull << 62;       // A render-thread read's number: never a sample's, so the
                                                            // final step re-stashes a skeletal pose from the real one.
// RT_READ_SER: bits 48-55 of a render-thread read's number count the reads (the cycle's first step, the paint,
// the final step), so a read that CHANGED a binding's pose is a sample of its own - the skeletal stash and skin
// (SKEL_DRAW) key on the number. Do not give every read of a cycle one number: it leaves the bones on the first
// read while the root and every rigid binding take the later ones. A read of the pose the cycle's last read held
// keeps that read's number (nothing to stash or skin again). Render thread; the numbers are compared only for
// equality.
static uint64_t g_rtp_read_ser = 0;
// GTS_IDENTITY: the offset from the previous cycle - the newest published samples' stamps against that cycle's
// clear (g_gts.clear_prev): learned only when exactly one lies within the window (after a hitch a frame's late
// sample can land near the next clear, beside that frame's own; two there is no answer, and the offset stands).
// Seven, not eight: the sampler writes the stamp of the next sample into the eighth slot before it publishes.
// Render thread, once a cycle (gts_step_prep).
inline void gts_id_learn() {
    const int64_t cp = g_gts.clear_prev;
    if (cp <= 0 || g_topo_cycles == 0u) return;
    int64_t w = static_cast<int64_t>(prof_freq()) * GTS_OWN_US / 1000000;
    if (g_gts.half > 0 && g_gts.half < w) w = g_gts.half;
    const uint64_t done = g_gts.done_pub.load(std::memory_order_acquire);
    uint64_t best = 0;
    uint32_t n = 0;
    for (uint64_t i = 0; i + 1u < GTS_STAMPS && i < done; ++i) {
        const uint64_t s = done - i;
        const int64_t q = g_gts.stamp_pub[s % GTS_STAMPS].load(std::memory_order_relaxed);
        const int64_t d = q >= cp ? q - cp : cp - q;
        if (d <= w) {
            best = s;
            ++n;
        }
    }
    if (n != 1u) return;
    const int64_t k = static_cast<int64_t>(best) - static_cast<int64_t>(g_topo_cycles - 1u);
    if (g_gid_ok && k != g_gid_k && stats_on()) ++g_gid_breaks;
    g_gid_k = k;
    g_gid_ok = true;
}
// RT_PAINT_READ: two poses equal bit for bit (the check of a read against the frame's own sample).
inline bool gts_same(const GtSnap& x, const GtSnap& y) {
    return memcmp(x.par, y.par, sizeof(x.par)) == 0 && x.px.size() == y.px.size() &&
           (x.px.empty() || memcmp(x.px.data(), y.px.data(), x.px.size() * sizeof(float)) == 0) &&
           x.rot_ok == y.rot_ok &&
           (!x.rot_ok || memcmp(x.rot, y.rot, sizeof(x.rot)) == 0);
}
// The sample cycle cyc draws (ATTACH_GT_SNAP, above), or null (the ring holds none for this binding): the
// frame's own by number when the identity offset is known and the ring holds it (GTS_IDENTITY); else, at a
// render-thread pick, the render thread's read for this cycle (RT_PAINT_READ: the frame's own has not landed);
// else the one stamped nearest the cycle's clear within half a period; else the newest before the clear, else the
// oldest held. The final step also checks the read it drew the cycle before against that frame's own sample,
// landed since (matched / stale / other).
// Memoized per binding and cycle by the render thread (store), so the steps of a cycle draw one sample even when
// the next Draw3D lands between them - except that the final step (g_attach_final_step) and the cast fire's step
// before its paint (g_attach_repick_step, CAST_REPICK) pick again and store their pick for the steps after them.
// A game-thread step (the park's, with the render thread held still) reads the memo, or computes the same pick
// unstored. Under g_draw_list_mutex.
inline const GtSnap* gts_for_cycle(Attach& a, uint64_t cyc, bool store) {
    const size_t np = a.skel ? a.skel_proxy.size() : (a.proxy.is_nil() ? 0u : 1u);
    const uint32_t gen = a.skel ? a.skel_gen : 0u;
    const uintptr_t xb = attach_base_of(attach_key_gv(a));
    const GtSnap* c[GTS_N];
    uint32_t nc = 0u;
    for (uint32_t i = 0; i < GTS_N; ++i) {
        const GtSnap& s = a.gts[i];
        if (s.ok && s.gen == gen && s.xb == xb && s.px.size() == np * 12u) {
            c[nc++] = &s;
        }
    }
    const bool gc_new = a.gts_cyc != cyc || (store && (g_attach_final_step || g_attach_repick_step));
    // RT_PAINT_READ: the read this cycle's pick took - while it is still this binding's (the ring's filter above;
    // a command can re-bind between two steps of a cycle, and a read of the old binding carries its helpers).
    if (!gc_new && a.rt_cyc == cyc && a.gts_pick_seq == a.rt_snap.seq && a.rt_snap.ok &&
        a.rt_snap.gen == gen && a.rt_snap.xb == xb && a.rt_snap.px.size() == np * 12u) {
        return &a.rt_snap;
    }
    if (!gc_new) {   // This cycle's pick stands.
        for (uint32_t i = 0; i < nc; ++i) {
            if (c[i]->seq == a.gts_pick_seq) return c[i];
        }
    }
    // RT_PAINT_READ: the read the previous cycle's final step drew, against that frame's own sample, once.
    if (store && g_attach_final_step && g_gid_ok && a.rt_chk_cyc != ~0ull &&
        a.rt_chk_cyc + 1u == cyc) {
        const uint64_t ow = static_cast<uint64_t>(static_cast<int64_t>(a.rt_chk_cyc) + g_gid_k);
        const GtSnap* o = nullptr;
        const GtSnap* ob = nullptr;
        for (uint32_t i = 0; i < nc; ++i) {
            if (c[i]->seq == ow) o = c[i];
            if (c[i]->seq + 1u == ow) ob = c[i];
        }
        if (o) {
            a.rt_chk_cyc = ~0ull;
            // A read of the binding a command has re-bound since judges nothing (the ring's filter, as the memo's).
            if (a.rt_chk.ok && a.rt_chk.gen == gen && a.rt_chk.xb == xb &&
                a.rt_chk.px.size() == np * 12u) {
                if (gts_same(a.rt_chk, *o)) { if (stats_on()) ++g_rtp_c.matched; }
                else if (ob && gts_same(a.rt_chk, *ob)) { if (stats_on()) ++g_rtp_c.stale; }
                else { if (stats_on()) ++g_rtp_c.other; }
            }
        }
    }
    if (nc == 0u) return nullptr;
    const int64_t cl = g_gts.clear_qpc;
    const int64_t half = g_gts.half;
    const GtSnap* p = nullptr;   // The frame's own: nearest the clear, within half a period.
    if (g_gid_ok) {   // GTS_IDENTITY: by number, when it is in the ring (a late one the time rule would pass over).
        const uint64_t want = static_cast<uint64_t>(static_cast<int64_t>(cyc) + g_gid_k);
        for (uint32_t i = 0; i < nc; ++i) if (c[i]->seq == want) p = c[i];
    }
    // RT_PAINT_READ: a render-thread pick with the frame's own not landed - the render thread's read of it for this
    // cycle (gts_step_prep), which the steps after it keep (the memo). A crew member's read is its prediction
    // (CREW_LAG), which only the paint's repick takes: the mesh keeps the Draw3D's measurement for it. The final
    // step keeps its read for the check against the frame's own sample at the next final step.
    if (!p && g_gid_ok && store && gc_new && (!a.rt_crew || g_attach_repick_step) &&
        a.rt_cyc == cyc && a.rt_snap.ok && a.rt_snap.gen == gen &&
        a.rt_snap.xb == xb && a.rt_snap.px.size() == np * 12u) {
        a.gts_cyc = cyc;
        a.gts_pick_seq = a.rt_snap.seq;
        if (g_attach_final_step) {
            a.rt_chk = a.rt_snap;
            a.rt_chk_cyc = cyc;
        }
        return &a.rt_snap;
    }
    int64_t best = 0;
    if (!p && half > 0) {   // GTS_IDENTITY: the time rule when identity has not picked.
        for (uint32_t i = 0; i < nc; ++i) {
            const int64_t d = c[i]->qpc >= cl ? c[i]->qpc - cl : cl - c[i]->qpc;
            if (d <= half && (!p || d < best)) {
                p = c[i];
                best = d;
            }
        }
    }
    if (!p) {   // Not arrived (or the period not known yet): B, the newest before the clear, else the oldest held.
        for (uint32_t i = 0; i < nc; ++i) {
            if (c[i]->qpc <= cl && (!p || c[i]->seq > p->seq)) p = c[i];
        }
    }
    if (!p) {
        for (uint32_t i = 0; i < nc; ++i) if (!p || c[i]->seq < p->seq) p = c[i];
    }
    if (store && g_attach_final_step && a.rt_cyc == cyc && g_gid_ok && p &&   // RT_PAINT_READ:
        p->seq == static_cast<uint64_t>(static_cast<int64_t>(cyc) + g_gid_k)) {        // the check.
        const GtSnap& r = a.rt_snap;
        const GtSnap* prev = nullptr;
        for (uint32_t i = 0; i < nc; ++i) if (c[i]->seq + 1u == p->seq) prev = c[i];
        // A read of the binding a command has re-bound since judges nothing (the ring's filter, as the memo's).
        if (r.ok && r.gen == gen && r.xb == xb && r.px.size() == np * 12u) {
            if (gts_same(r, *p)) { if (stats_on()) ++g_rtp_c.matched; }
            else if (prev && gts_same(r, *prev)) { if (stats_on()) ++g_rtp_c.stale; }
            else { if (stats_on()) ++g_rtp_c.other; }
        }
        a.rt_cyc = ~0ull;   // Counted once.
    }
    if (store && gc_new) {
        a.gts_cyc = cyc;
        a.gts_pick_seq = p->seq;
    }
    return p;
}

// CREW_LAG - the engine places a crew member's attachTo children (its origin helper, every memory-point proxy on
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
// (chain_prepare) takes the same: a memory point moved so and its reach tested against the rendered position; an
// end following the character itself placed at that position and turned to that orientation - the character's own
// state is not written while it is crew (ATTACH_MAN_HELPER), so a raw read of it would hold the get-in pose.
// Crew or not: the character's GetInMan / GetOutMan events and a check when its entry is made, each asking
// `objectParent` (crew_vehicle) - the handlers take Intercept's argument lists as it declares them and read none
// of the arguments. While crew, the pose is read every Draw3D (getPosWorldVisual, vectorDirVisual, vectorUpVisual per
// character), so a seat switch (which stays in the vehicle: no GetInMan / GetOutMan), a turret's traverse or a turn-out
// needs no event of its own. In the Eden editor the three are the simulation's - getPosWorld, vectorDir, vectorUp
// (ATTACH_EDEN). RT_PAINT_READ carries it too: the render thread makes no engine call, so it places the character
// from the vehicle (read raw, with the paint read's other reads) and the character's pose in that vehicle's frame as
// the last Draw3D measured it - a prediction over the one frame whose sample has not landed.
// One entry per character a binding places from (attach_parent_gv) through a helper - an origin helper standing in
// for it, or helpers on it (a memory-point or skeletal binding) when it is a Man - or a simulating chain's end
// follows when it is a Man (crew_chain_cand: its object, its memory point's owner, its rotation object), keyed by
// its base (two holders may have different game values of one object; an entry whose character was deleted drains at
// the next sync). The entry holds a pool reference of its own on the character's origin helper, which a memory-point
// binding or a chain's end alone has none of. A rotation lane that follows a character of its own (not the object its
// binding is placed from) through that character's helper brings it in too: the lane is turned as that character is
// drawn (its rows, from the same rigid move).
// GAME THREAD maintains it (crew_sync, from flush_frame; the events): inserted, erased and every field the render
// thread reads (in, d_ok, helper, veh, l_ok, l, lq) written under g_draw_list_mutex; r_* are the render thread's own
// page caches (the paint read, under the mutex too); t / tr with their flags, and seen, are the game thread's alone.
// Never destructed (game values); the mission edge empties it (crew_clear_all, outside the mutex:
// dropping an entry removes its event handlers, an SQF call).
struct CrewEnt {
    game_value man;
    game_value helper;           // The character's origin helper (this entry's pool reference).
    game_value veh;              // The vehicle it rides in; nil on foot.
    uint64_t   serial = 0;       // The events' key (g_crew_serial).
    bool       in = false;
    bool       seen = false;     // crew_sync's census mark.
    bool       t_ok = false;
    double     t[3] = {};        // getPosWorldVisual this Draw3D (getPosWorld in Eden), SQF order.
    bool       tr_ok = false;
    float      tr[9] = {};       // vectorDir/UpVisual this Draw3D (vectorDir/Up in Eden), as rows (crew_rows).
    bool       d_ok = false;     // This Draw3D's correction (crew_measure): h, D, cr.
    double     h[3] = {};        // The origin helper's read, SQF order.
    float      dm[9] = {};       // D = (the helper's rows)^-1 * cr.
    float      cr[9] = {};       // The character's rows as drawn: tr, or the helper's where tr failed its checks.
    bool       l_ok = false;
    float      l[3] = {};        // The character in the vehicle's frame (its rows, engine axes),
    float      lq[9] = {};       // and its rows in it (cr * the vehicle's rows^-1).
    uint64_t   born = 0;         // g_crew_frame when the entry (and its helper reference) was made.
    uintptr_t  g_hb = 0, g_hvb = 0, g_hbb = 0, g_vb = 0, g_vvb = 0, g_vbb = 0;   // The sampler's page caches.
    uintptr_t  g_havb = 0, g_habb = 0, g_vavb = 0, g_vabb = 0;   // ATTACH_VS_CACHE: their visual attempts'.
    uint32_t   g_hoff = KH_ATTACH_OFF_NONE, g_voff = KH_ATTACH_OFF_NONE;
    uintptr_t  r_hb = 0, r_hvb = 0, r_hbb = 0, r_vb = 0, r_vvb = 0, r_vbb = 0;   // The paint read's (render thread).
    uintptr_t  r_havb = 0, r_habb = 0, r_vavb = 0, r_vabb = 0;   // ATTACH_VS_CACHE: their visual attempts'.
    uint32_t   r_hoff = KH_ATTACH_OFF_NONE, r_voff = KH_ATTACH_OFF_NONE;
    intercept::client::EHIdentifierHandle eh_in, eh_out;
};
static std::unordered_map<uintptr_t, CrewEnt>& g_crew = *(new std::unordered_map<uintptr_t, CrewEnt>());
// Game thread: characters found not to be a Man (base -> the object's identity), rebuilt by every sync from the
// candidates it saw - an address the engine reuses for another object is asked again.
static std::unordered_map<uintptr_t, const void*>& g_crew_notman = *(new std::unordered_map<uintptr_t, const void*>());
static uint64_t g_crew_serial = 0;   // Game thread; restarted with the table (crew_clear_all).
static uint64_t g_crew_frame = 0;    // Game thread: crew_sync's count; restarted with the table.
// One raw read with a page cache of the entry's own, restarted when the object behind it changes.
inline bool crew_read(const game_value& gv, uintptr_t& b, uint32_t& off, uintptr_t& vb,
                      uintptr_t& bb, uintptr_t& avb, uintptr_t& abb, float p[3], float r[9]) {
    if (gv.is_nil() || attach_obj_dead(gv)) return false;
    const uintptr_t nb = attach_base_of(gv);
    if (nb == 0) return false;
    if (b != nb) { b = nb; off = KH_ATTACH_OFF_NONE; vb = 0; bb = 0; avb = 0; abb = 0; }
    return attach_raw(gv, off, vb, bb, avb, abb, p, r, nullptr);
}
// A point in a pose's frame and back: positions SQF order, the pose's rows engine axes (row-vector convention, as
// attach_raw hands them back; each row divided by its own length squared, so a scaled pose round-trips too).
inline void crew_local(const float vp[3], const float vr[9], const double t[3], float l[3]) {
    const double w[3] = { t[0] - vp[0], t[2] - vp[2],   // Engine axes.
                          t[1] - vp[1] };
    for (int k = 0; k < 3; ++k) {
        const double x = vr[k * 3 + 0], y = vr[k * 3 + 1];
        const double z = vr[k * 3 + 2];
        const double n = x * x + y * y + z * z;
        l[k] = n > 0.0
            ? static_cast<float>((x * w[0] + y * w[1] + z * w[2]) / n) : 0.0f;
    }
}
inline void crew_place(const float vp[3], const float vr[9], const float l[3], double t[3]) {
    double w[3] = { 0.0, 0.0, 0.0 };   // Engine axes.
    for (int k = 0; k < 3; ++k) {
        for (int j = 0; j < 3; ++j) {
            w[j] += static_cast<double>(l[k]) * static_cast<double>(vr[k * 3 + j]);
        }
    }
    t[0] = vp[0] + w[0];
    t[1] = vp[1] + w[2];
    t[2] = vp[2] + w[1];
}
// 3x3 rows (row k = axis k, engine axes; a pose's rows multiply on the right): a product, an inverse (false when
// singular), D = a^-1 * b (the identity when a is singular).
inline void crew_mul3(const float a[9], const float b[9], float o[9]) {
    float t[9];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            double s = 0.0;
            for (int k = 0; k < 3; ++k) {
                s += static_cast<double>(a[i * 3 + k]) *
                     static_cast<double>(b[k * 3 + j]);
            }
            t[i * 3 + j] = static_cast<float>(s);
        }
    }
    memcpy(o, t, sizeof(t));
}
inline bool crew_inv3(const float m[9], float o[9]) {
    const double a = m[0], b = m[1], c = m[2], d = m[3], e = m[4], f = m[5];
    const double g = m[6], h = m[7], i = m[8];
    const double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!(std::fabs(det) > 1e-9)) return false;
    const double r = 1.0 / det;
    const double v[9] = { (e * i - f * h) * r, (c * h - b * i) * r, (b * f - c * e) * r,
                               (f * g - d * i) * r, (a * i - c * g) * r, (c * d - a * f) * r,
                               (d * h - e * g) * r, (b * g - a * h) * r, (a * e - b * d) * r };
    for (int k = 0; k < 9; ++k) o[k] = static_cast<float>(v[k]);
    return true;
}
inline void crew_delta(const float a[9], const float b[9], float o[9]) {
    float i[9];
    if (!crew_inv3(a, i)) {
        const float one[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        memcpy(o, one, sizeof(one));
        return;
    }
    crew_mul3(i, b, o);
}
// A pose read on the character, moved with it: p (SQF order) to t + (p - h) * D, its rows r (null: none) to r * D.
inline void crew_apply(const double t[3], const double h[3], const float d[9], float p[3],
                       float* r) {
    const double v[3] = { p[0] - h[0], p[2] - h[2], p[1] - h[1] };   // Engine.
    double w[3];
    for (int j = 0; j < 3; ++j) {
        w[j] = v[0] * d[j] + v[1] * d[3 + j] + v[2] * d[6 + j];
    }
    p[0] = static_cast<float>(t[0] + w[0]);
    p[1] = static_cast<float>(t[1] + w[2]);
    p[2] = static_cast<float>(t[2] + w[1]);
    if (r) crew_mul3(r, d, r);
}
// The crew pose's dir / up (SQF order) as rows - aside (up x dir), up, dir in engine axes, the order the
// visual state keeps them in (ATTACH_RAW). False when the pair is not a unit, orthogonal pair.
inline bool crew_rows(const float dir[3], const float up[3], float o[9]) {
    const double d[3] = { dir[0], dir[2], dir[1] };
    const double u[3] = { up[0], up[2], up[1] };
    const double dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
    const double du = d[0] * u[0] + d[1] * u[1] + d[2] * u[2];
    if (!(std::fabs(dd - 1.0) < 1e-3 && std::fabs(uu - 1.0) < 1e-3 && std::fabs(du) < 1e-3)) return false;
    const double s[3] = { u[1] * d[2] - u[2] * d[1], u[2] * d[0] - u[0] * d[2], u[0] * d[1] - u[1] * d[0] };
    for (int k = 0; k < 3; ++k) {
        o[k] = static_cast<float>(s[k]);
        o[3 + k] = static_cast<float>(u[k]);
        o[6 + k] = static_cast<float>(d[k]);
    }
    return true;
}
// The entry's helper is placed: a Draw3D after the entry was made (attachTo places a new helper at the next
// simulation step - ATTACH_MAN_HELPER's stamp, counted in syncs, which run whether or not any binding samples).
inline bool crew_placed(const CrewEnt& e) {
    return g_crew_frame > e.born;
}
// This Draw3D's corrections: crew_sync, under g_draw_list_mutex, after its pose reads - the same instant
// as the sampler's reads and the chain end's (all in this Draw3D, with no engine step between).
inline void crew_measure() {
    for (auto& kv : g_crew) {
        CrewEnt& e = kv.second;
        e.d_ok = false;
        if (!e.in || !e.t_ok || !crew_placed(e)) { e.l_ok = false; continue; }
        float p[3], r[9];
        if (!crew_read(e.helper, e.g_hb, e.g_hoff, e.g_hvb, e.g_hbb, e.g_havb, e.g_habb, p, r)) {
            e.l_ok = false;
            continue;
        }
        for (int k = 0; k < 3; ++k) e.h[k] = p[k];
        memcpy(e.cr, e.tr_ok ? e.tr : r, sizeof(e.cr));
        crew_delta(r, e.cr, e.dm);
        e.d_ok = true;
        e.l_ok = crew_read(e.veh, e.g_vb, e.g_voff, e.g_vvb, e.g_vbb, e.g_vavb, e.g_vabb, p, r);
        float vi[9];
        if (e.l_ok) e.l_ok = crew_inv3(r, vi);
        if (e.l_ok) {
            crew_local(p, r, e.t, e.l);
            crew_mul3(e.cr, vi, e.lq);
        }
    }
}
// The entry correcting what a binding places from (par, attach_parent_gv) this sample; null: none. By base:
// the sync ahead of this sample dropped every entry whose character was deleted, so a live entry's base is its own.
inline CrewEnt* crew_of(const game_value& par) {
    if (g_crew.empty() || par.is_nil()) return nullptr;
    const auto it = g_crew.find(attach_base_of(par));
    return it == g_crew.end() || !it->second.d_ok ? nullptr : &it->second;
}
// The character's vehicle now (nil: on foot). GAME THREAD, holding no lock (an SQF call). CREW_PARENT:
// objectParent - the vehicle a unit rides in, null on foot (an attachTo parent is not one). Not `vehicle`: it answers
// the unit itself on foot, which a test against the character's own base has to tell apart.
inline game_value crew_vehicle(const game_value& man) {
    try {
        const object o = sqf::object_parent(static_cast<object>(man));
        if (!sqf::is_null(o)) return o;
    } catch (...) {}
    return game_value();
}
// An event of the character's: crew or not, asked again. GAME THREAD (the engine's event dispatch), holding no lock.
inline void crew_event(uintptr_t b, uint64_t s) {
    const auto it = g_crew.find(b);
    if (it == g_crew.end() || it->second.serial != s) return;
    CrewEnt& e = it->second;
    game_value v = crew_vehicle(e.man);
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    e.veh = v;
    e.in = !v.is_nil();
    e.d_ok = false;
    e.l_ok = false;
}
// The entry's two event handlers (GetInMan, GetOutMan). False when one could not be added: the caller drops the entry
// (its handles remove what was added).
inline bool crew_listen(CrewEnt& e, uintptr_t b) {
    const uint64_t s = e.serial;
    auto f = [b, s](const auto&...) { crew_event(b, s); };
    namespace icl = intercept::client;
    try {
        const object o = static_cast<object>(e.man);
        e.eh_in = icl::addEventHandler<icl::eventhandlers_object::GetInMan>(o, f);
        e.eh_out = icl::addEventHandler<icl::eventhandlers_object::GetOutMan>(o, f);
    } catch (...) {
        return false;
    }
    return true;
}
// The table against this frame's bindings and chain ends, and this Draw3D's positions. GAME THREAD, from flush_frame
// after the reaps and before the sampler, holding no lock across an SQF call. An entry whose character nothing brings
// in any more (CREW_LAG's list: a binding, a rotation lane, a simulating chain's end) or that was deleted drops:
// its helper reference to the reap, its event handlers here.
inline void crew_sync() {
    if (g_attach_n.load(std::memory_order_relaxed) == 0 && g_crew.empty() && !crew_chain_any()) return;
    ++g_crew_frame;
    static std::vector<std::pair<game_value, bool>> cand;   // (character, has an origin helper: a Man).
    cand.clear();
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (const auto& kv : g_attach) {
            const Attach& a = kv.second;
            const game_value& p = attach_parent_gv(a);
            // A rotation lane's own character (its helper stands in for an object that is not the parent).
            const game_value& r = a.obj_rot;
            if (attach_is_obj(r) && r.data.get() != p.data.get() && !a.org_rot.is_nil() &&
                a.org_rot_for == r.data.get()) cand.emplace_back(r, true);
            if (!attach_is_obj(p)) continue;
            const void* const pd = p.data.get();
            const bool org = (!a.org_pos.is_nil() && a.org_pos_for == pd) ||
                             (!a.org_rot.is_nil() && a.org_rot_for == pd);
            const bool pxy = a.skel ? !a.skel_proxy.empty() : !a.proxy.is_nil();
            if (org || pxy) cand.emplace_back(p, org);
        }
        crew_chain_cand(cand);
    }
    for (auto& kv : g_crew) kv.second.seen = false;
    std::unordered_map<uintptr_t, const void*> nm;
    for (const auto& c : cand) {
        const game_value& m = c.first;
        const uintptr_t b = attach_base_of(m);
        if (b == 0 || attach_obj_dead(m)) continue;
        const auto it = g_crew.find(b);
        if (it != g_crew.end()) {   // By base: two bindings may hold two game values of one object.
            if (!attach_obj_dead(it->second.man)) it->second.seen = true;   // Else it drains below.
            continue;
        }
        if (!c.second) {   // Helpers only (a memory point, a skeleton): a Man?
            const auto n = g_crew_notman.find(b);
            bool man = false;
            if (n == g_crew_notman.end() || n->second != m.data.get()) {
                try { man = sqf::is_kind_of(static_cast<object>(m), "Man"); }
                catch (...) { man = false; }
            }
            if (!man) { nm[b] = m.data.get(); continue; }
        }
        CrewEnt e;
        e.man = m;
        e.serial = ++g_crew_serial;
        e.born = g_crew_frame;
        std::string err;
        try {
            if (!pxy_pool_get(m, std::string(), e.helper, "a character's", err)) {
                e.helper = game_value();
            }
        } catch (...) {
            e.helper = game_value();
        }
        if (e.helper.is_nil()) continue;   // No helper to correct against: its bindings read as before.
        e.veh = crew_vehicle(m);
        e.in = !e.veh.is_nil();
        if (!crew_listen(e, b)) { attach_proxy_orphan(e.helper); continue; }
        e.seen = true;
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        g_crew.emplace(b, std::move(e));
    }
    g_crew_notman.swap(nm);
    std::vector<CrewEnt> gone;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto it = g_crew.begin(); it != g_crew.end();) {
            if (it->second.seen && !attach_obj_dead(it->second.man)) { ++it; continue; }
            gone.push_back(std::move(it->second));
            it = g_crew.erase(it);
        }
    }
    for (CrewEnt& e : gone) attach_proxy_orphan(e.helper);   // Takes the mutex itself.
    gone.clear();   // Their event handlers go here, holding nothing.
    const bool ed = attach_eden();   // ATTACH_EDEN: the simulation's pose, as every read takes.
    for (auto& kv : g_crew) {
        CrewEnt& e = kv.second;
        e.t_ok = false;
        e.tr_ok = false;
        if (!e.in) continue;
        try {
            const object o = static_cast<object>(e.man);
            const vector3 dv = ed ? sqf::vector_dir(o) : sqf::vector_dir_visual(o);
            const vector3 uv = ed ? sqf::vector_up(o) : sqf::vector_up_visual(o);
            const float df[3] = { dv.x, dv.y, dv.z };
            const float uf[3] = { uv.x, uv.y, uv.z };
            e.tr_ok = crew_rows(df, uf, e.tr);   // Else the helper's rows (crew_measure).
        } catch (...) {
            e.tr_ok = false;
        }
        try {
            const object mo = static_cast<object>(e.man);
            const vector3 v = ed ? sqf::get_pos_world(mo) : sqf::get_pos_world_visual(mo);
            e.t[0] = v.x;
            e.t[1] = v.y;
            e.t[2] = v.z;
            e.t_ok = std::isfinite(e.t[0]) && std::isfinite(e.t[1]) && std::isfinite(e.t[2]);
        } catch (...) {
            e.t_ok = false;
        }
    }
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        crew_measure();
    }
    cand.clear();
}
// The mission edge: the table emptied, its event handlers removed outside the mutex; the helper references are
// released, not queued (the pool is emptied with the bindings - attach_drop_all).
inline void crew_clear_all() {
    std::unordered_map<uintptr_t, CrewEnt> old;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        old.swap(g_crew);
    }
    old.clear();
    g_crew_notman.clear();
    g_crew_serial = 0;
    g_crew_frame = 0;
}

// RT_PAINT_READ - nothing on the render thread waits for a Draw3D but the final step while a binding or a followed
// chain end is on a crew member (see "Wait"). When the frame's own sample is absent (identity known, sample cycle +
// g_gid_k not published) at the cast fire's paint or at the cycle's steps (gts_step_prep), the render thread reads
// the same objects the sampler reads - twice - and a binding takes the read only if every read validated and both
// passes agree bit for bit (the game thread may be writing the object meanwhile). Every render-thread step of the
// cycle draws it (a crew member's prediction, the paint's repick alone); a read the final step drew is counted
// against the frame's own sample once that lands (matched / stale / other).
// PXY_SMOOTH: its memory-point helpers are smoothed as the sampler smooths them (the estimator stepped with the
// frame's own sample number); the check compares the raw reads.
// CREW_LAG: a binding placed from a crew member also reads the character's origin helper and vehicle (both
// passes) and moves its helpers by the prediction, which only the paint's repick draws (the mesh keeps the frame's
// own sample: the final step waits for it); the final step's check of the paint's read counts 'other' wherever the
// prediction and the frame's own measurement differ in a bit.
inline bool rt_own_absent() {
    if (!g_gid_ok) return false;   // The frame's own number unknown: no read (today's pick).
    const uint64_t want = static_cast<uint64_t>(static_cast<int64_t>(g_topo_cycles) + g_gid_k);
    return g_gts.done_pub.load(std::memory_order_acquire) < want;
}
// Render thread, outside g_draw_list_mutex (taken here). The bindings read; 0 = none; RT_OWN_ARRIVED = the own
// sample was published before the mutex was ours (read nothing - the paint's repick takes the sample itself). Under
// the mutex the answer is exact: the sample is published under it (by the sampler, or - with a chain end followed -
// with the chain's targets later in the same Draw3D handler, g_gts.done_defer), and the game thread moves objects
// on to the next frame only after that handler - so a read made while the sample is absent cannot see the next
// frame's pose.
static constexpr uint32_t RT_OWN_ARRIVED = ~0u;
inline uint32_t rt_paint_read() {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    if (!rt_own_absent()) return RT_OWN_ARRIVED;
    const uint64_t cyc = g_topo_cycles;
    // PXY_SMOOTH: the frame's own sample number.
    const uint64_t own = static_cast<uint64_t>(static_cast<int64_t>(cyc) + g_gid_k);
    const uint64_t seq = own | RT_SEQ_BIT | ((++g_rtp_read_ser & 0xFFull) << 48);   // RT_READ_SER.
    // RT_READ_SER: a binding's previous read of this cycle - assigned before every read of it. Static, so its
    // vectors keep their capacity; only ever touched under g_draw_list_mutex (held for this whole call).
    static GtSnap prev;
    const int64_t q = gts_qpc();
    uint32_t rp_n = 0;
    std::vector<float> rp_a, b;
    for (auto& kv : g_attach) {
        Attach& a = kv.second;
        if (a.rt_par.is_nil()) continue;
        // A helper the game thread deleted since the sampler read it (the sampler's own rule, attach_org_read): its
        // state no longer moves - this binding keeps today's pick for the frame. The rotation lane's object counts
        // only while the lane still follows it (below: one re-pointed since is not read at all).
        if (attach_obj_dead(a.rt_par)) continue;
        const size_t np = a.skel ? a.skel_proxy.size() : (a.proxy.is_nil() ? 0u : 1u);   // 0: the parent alone (the sampler's rule).
        const uint32_t gen = a.skel ? a.skel_gen : 0u;
        const uintptr_t xb = attach_base_of(attach_key_gv(a));
        const uintptr_t pb = attach_base_of(a.rt_par);
        if (xb == 0 || pb == 0) continue;
        // ATTACH_ROT_OF: rt_par names the binding the last Draw3D sampled; one a command re-bound since is not
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
        // ATTACH_ROT_OF: rt_rot only while the rotation lane still follows what it was named for.
        const bool rcur = !a.rt_rot.is_nil() && a.rt_rot_for == attach_base_of(a.obj_rot);
        if (rcur && attach_obj_dead(a.rt_rot)) continue;   // The helper rule above, for the object it reads.
        const uintptr_t rb = rcur ? attach_base_of(a.rt_rot) : 0u;
        if (a.rt_rb != rb) {
            a.rt_rb = rb; a.rt_rot_off = KH_ATTACH_OFF_NONE; a.rt_rot_vb = 0; a.rt_rot_bb = 0; a.rt_rot_avb = 0; a.rt_rot_abb = 0;
        }
        // CREW_LAG: a crew member's origin helper and vehicle too, once a sample has placed it in that vehicle.
        CrewEnt* cw = nullptr;
        if (!g_crew.empty()) {
            const game_value& pg = attach_parent_gv(a);
            const auto cit = g_crew.find(attach_base_of(pg));
            if (cit != g_crew.end() && cit->second.in && cit->second.l_ok) cw = &cit->second;
        }
        // CREW_LAG: the rotation lane's own character, where its helper is what the lane reads: its vehicle too.
        CrewEnt* cwr = nullptr;
        if (rb != 0u && !g_crew.empty()) {
            const auto cit = g_crew.find(attach_base_of(a.obj_rot));
            if (cit != g_crew.end() && cit->second.in && cit->second.l_ok &&
                a.rt_rot.data.get() == cit->second.helper.data.get()) cwr = &cit->second;
        }
        const size_t nc = np + 2u + (cw ? 2u : 0u);   // Parent, helpers, rotation object (, helper, vehicle).
        const size_t n = (nc + (cwr ? 1u : 0u)) * 12u;   // (, the rotation lane's character's vehicle)
        rp_a.assign(n, 0.0f);
        b.assign(n, 0.0f);
        auto read_all = [&](float* out, bool& rot_ok) -> bool {
            if (!attach_raw(a.rt_par, a.rt_par_off, a.rt_par_vb, a.rt_par_bb, a.rt_par_avb, a.rt_par_abb, out, out + 3,
                            nullptr)) return false;
            for (size_t j = 0; j < np; ++j) {
                const game_value& x = a.skel ? a.skel_proxy[j] : a.proxy;
                float* const o = out + (j + 1u) * 12u;
                if (!attach_raw(x, a.rt_off[j], a.rt_vb[j], a.rt_bb[j], a.rt_avb[j], a.rt_abb[j], o, o + 3, nullptr)) return false;
            }
            rot_ok = false;
            if (rb != 0u) {
                float* const o = out + (np + 1u) * 12u;
                rot_ok = attach_raw(a.rt_rot, a.rt_rot_off, a.rt_rot_vb, a.rt_rot_bb, a.rt_rot_avb, a.rt_rot_abb, o, o + 3,
                                    nullptr);
            }
            if (cw) {   // CREW_LAG.
                float* const o = out + (np + 2u) * 12u;
                if (!crew_read(cw->helper, cw->r_hb, cw->r_hoff, cw->r_hvb, cw->r_hbb, cw->r_havb, cw->r_habb, o, o + 3) ||
                    !crew_read(cw->veh, cw->r_vb, cw->r_voff, cw->r_vvb, cw->r_vbb, cw->r_vavb, cw->r_vabb, o + 12, o + 15)) return false;
            }
            if (cwr) {   // CREW_LAG.
                float* const o = out + nc * 12u;
                if (!crew_read(cwr->veh, cwr->r_vb, cwr->r_voff, cwr->r_vvb, cwr->r_vbb, cwr->r_vavb, cwr->r_vabb, o, o + 3)) return false;
            }
            return true;
        };
        bool rot_a = false, rot_b = false;
        if (!read_all(rp_a.data(), rot_a) || !read_all(b.data(), rot_b)) { { if (stats_on()) ++g_rtp_c.refused; } continue; }
        if (rot_a != rot_b || memcmp(rp_a.data(), b.data(), n * sizeof(float)) != 0) { { if (stats_on()) ++g_rtp_c.torn; } continue; }
        GtSnap& sn = a.rt_snap;
        const bool again = a.rt_cyc == cyc && sn.ok && sn.gen == gen && sn.xb == xb;   // RT_READ_SER.
        if (again) prev = sn;
        sn.ok = true;
        sn.rot_ok = rot_a;
        sn.seq = seq;
        sn.qpc = q;
        sn.gen = gen;
        sn.xb = xb;
        memcpy(sn.par, rp_a.data(), sizeof(sn.par));
        sn.px.assign(rp_a.begin() + 12, rp_a.begin() + 12 + static_cast<ptrdiff_t>(np * 12u));
        if (rot_a) memcpy(sn.rot, rp_a.data() + (np + 1u) * 12u, sizeof(sn.rot));
        sn.rot_of = rot_a ? a.rt_rot_for : 0u;   // ATTACH_ROT_OF.
        if (cw) {   // CREW_LAG: the prediction - the character where its last offset in the vehicle puts it now.
            const float* const hc = rp_a.data() + (np + 2u) * 12u;   // The helper's pose, then the vehicle's.
            double tp[3];
            crew_place(hc + 12, hc + 15, cw->l, tp);
            float cr[9], dm[9];
            crew_mul3(cw->lq, hc + 15, cr);   // Its rows in the vehicle, on the vehicle as it is now.
            crew_delta(hc + 3, cr, dm);
            const double hd[3] = { hc[0], hc[1], hc[2] };
            const bool ph = a.rt_par.data.get() == cw->helper.data.get();
            if (ph) crew_apply(tp, hd, dm, sn.par, sn.par + 3);
            if (ph || !a.skel) {
                for (size_t j = 0; j < np; ++j) crew_apply(tp, hd, dm, &sn.px[j * 12u], &sn.px[j * 12u + 3u]);
            }
        }
        if (cwr && rot_a) {   // CREW_LAG: the rotation lane's character, likewise predicted.
            const float* const vc = rp_a.data() + nc * 12u;
            double tp[3];
            crew_place(vc, vc + 3, cwr->l, tp);
            for (int k = 0; k < 3; ++k) sn.rot[k] = static_cast<float>(tp[k]);
            crew_mul3(cwr->lq, vc + 3, sn.rot + 3);
        }
        // PXY_SMOOTH: the memory-point helpers through the estimator, stepped with the frame's own sample number -
        // once for the frame: the Draw3D sampler, reading the same pose later, is handed this result back. A crew
        // member's predicted helpers stay raw (the repick's alone).
        sn.pxd.assign(np * 3u, 0.0f);
        if (!cw) {
            for (size_t j = 0; j < np; ++j) {
                const game_value& x = a.skel ? a.skel_proxy[j] : a.proxy;
                pxy_smooth_note(x, own, q, &sn.px[j * 12u], &sn.pxd[j * 3u]);
            }
        }
        // RT_READ_SER: the same pose read again keeps its number - the steps' stash and skin stand.
        if (again && gts_same(prev, sn) && prev.pxd == sn.pxd) sn.seq = prev.seq;
        a.rt_crew = cw != nullptr || cwr != nullptr;
        a.rt_cyc = cyc;
        ++rp_n;
    }
    if (stats_on()) g_rtp_c.bindings += rp_n;
    chain_end_rt_read(cyc, own, q);   // CHAIN_END_DRAW: the followed ends, likewise.
    return rp_n;
}
// Every render-thread step, before attach_step takes g_draw_list_mutex (ATTACH_GT_SNAP: "Own or read" and
// "Wait"). Once a cycle, the identity offset relearned from the previous cycle. Then, with the frame's own sample not
// landed, the render thread's read of every binding - at the cycle's first step, and again at its final step
// (rt_paint_read takes the mutex itself; a sample that lands first makes it read nothing, and the pick takes the
// sample). At the final step, while a binding or a followed chain end is on a crew member (g_gts.crew_n), the
// frame's own sample is waited for first.
inline void gts_step_prep() {
    const bool sp_final = g_attach_final_step;
    if (g_gts.learn_cyc != g_topo_cycles) {
        g_gts.learn_cyc = g_topo_cycles;
        gts_id_learn();
    }
    uint64_t& done = sp_final ? g_gts.final_cyc : g_gts.first_rd_cyc;
    if (done == g_topo_cycles) return;
    done = g_topo_cycles;
    if (!rt_own_absent()) return;
    if (sp_final && g_gts.crew_n.load(std::memory_order_relaxed) != 0u) {   // CREW_LAG: as before the reads.
        const uint64_t want = static_cast<uint64_t>(static_cast<int64_t>(g_topo_cycles) + g_gid_k);
        // CREW_SILENT: only while the frame's own sample is the NEXT one the sampler publishes (a late Draw3D).
        // Further ahead the sampler is silent - the engine renders with the window unfocused and Draw3D does not
        // fire - or a frame or more behind (the offset stale on its return): the wait would run to its cap every
        // frame (~9 fps) and end without the sample, so the frame draws the render thread's read at once, as a
        // wait that reached the cap does. g_gts.crew_n keeps the sampler's last count while it is silent.
        if (g_gts.done_pub.load(std::memory_order_acquire) + 1u >= want) {
            const int64_t until = g_gts.clear_qpc +
                                  static_cast<int64_t>(prof_freq()) * GTS_CREW_CAP_MS / 1000;
            while (g_gts.done_pub.load(std::memory_order_acquire) < want && gts_qpc() < until) {
                std::this_thread::yield();
            }
        }
        if (!rt_own_absent()) return;
    }
    if (sp_final && stats_on()) ++g_rtp_c.finals;
    (void)rt_paint_read();
}
// attach_step's tail for a skeletal binding: the draw centre from its root, now the rotation (and the skin's box)
// is final.
inline void attach_skel_place(RenderObject& o, const double root[3], bool& moved) {
    double c[3] = { root[0], root[1], root[2] };   // POS_RES: exact to the draw.
    skel_centre_d(o, c);
    if (pos_set_exact(o, c)) moved = true;
}
// attach_step's tail for every binding, and chain_end_draw's for a chain end it re-placed: a moved mesh re-files
// its scene slot, a final step that moved it reopens the sun map (SUN_RESTEP), and a repick step that moved it has
// the cast fire rebuild the sun map before its paint (CAST_REPICK).
inline void attach_moved_note(const RenderObject& o, bool moved, bool rt) {
    if (!moved) return;
    scene_mark(o.slot);
    if (g_attach_final_step && rt) g_sun_restep_cycle = g_topo_cycles;
    if (g_attach_repick_step && rt) g_attach_repick_moved = true;   // CAST_REPICK.
}
// PXY_SMOOTH - a memory-point helper's position through a bounded estimator. The engine places an attached object
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
//             constant that eases from PXY_SMOOTH_TAU_SLOW at or under PXY_SMOOTH_SPD_SLOW spacings a second
//             to PXY_SMOOTH_TAU_FAST at or over PXY_SMOOTH_SPD_FAST (its own speed), gains from the sample's
//             real interval: it turns the target's steps into motion at the pace the steps set. It may never
//             step against the way the target last moved (a type-2 tracker overshoots every step it is given, and
//             the swing back is a new dance on slow breathing): such a step is held, and a velocity against that
//             way is dropped with it;
//   clamp   - the output is kept within one spacing of the reading, and a clamp takes the velocity with it;
//   reset   - a reading more than PXY_SMOOTH_RESET spacings from the last one (a teleport) is the output, and
//             everything starts again from it.
// The velocity is the output's own recent motion, used only to carry the output between readings: nothing is shown
// that the current reading rules out (the clamp), and nothing is extrapolated past a reading. Measured offline over
// dither, idle breathing (clean and noisy), drift, sway of 20 to 200 spacings, a walk and a run, at 30 / 60 / 144 fps
// with 20 % frame jitter, 5 to 25 km (per-frame jerk against the truth, spacings, rms / max, 60 fps; raw -> this):
// dither 0.71 / 1.00 -> 0.01 / 0.02; breathing 0.20 / 0.98 -> 0.08 / 0.39 with the true count of reversals; sway
// 0.39 / 0.99 -> 0.16 / 0.46; drift 0.40 / 0.82 -> 0.10 / 0.21; a walk 0.40 / 0.98 -> 0.18 / 0.50. Only an instant
// step from rest to a run jerks once past raw (1.5 spacings). The envelope: within one spacing of the engine's
// reading at any speed, a teleport exact. The output travels as a delta from the reading (GtSnap::pxd - at most a
// spacing, which a float holds finely) to the step, which places the mesh with it in double (POS_RES) and a
// skeletal binding's bones with it (skin_rel).
// The estimator runs in the sampler, for a chain's end on a memory point (chain_prepare, on the sampler's sample,
// or one of its own) and in the render thread's read (RT_PAINT_READ, with the frame's own sample number, whose
// result the sampler is handed back) - on the pool entry, so every binding and chain end on one memory point moves
// alike - always under g_pxy_pool_mu, and it makes no engine call. An origin helper (no memory point) and a followed
// object are not estimated: each is the point the engine draws its object from, not a rounded offset from it.
static constexpr double PXY_SMOOTH_RESET = 64.0;     // Spacings one sample may step before the state restarts.
static constexpr double PXY_SMOOTH_TAU_SLOW = 0.10;  // The tracker's time constant (s), at rest ...
static constexpr double PXY_SMOOTH_TAU_FAST = 0.05;  // ... and moving;
static constexpr double PXY_SMOOTH_SPD_SLOW = 5.0;   // at or under this speed (spacings a second) the first,
static constexpr double PXY_SMOOTH_SPD_FAST = 30.0;  // at or over this the second, linear between.
// A chain end's own sample number, taken when this flush_frame's sampler took no sample (chain_prepare): this
// bit and the pass's stamp - never a sampler's (small counts) nor a render thread read's (RT_SEQ_BIT, bit 62),
// new every pass.
static constexpr uint64_t PXY_SEQ_CHAIN = 1ull << 61;
// The float spacing at r (one unit in the last place of its binade): half of it bounds the rounding of any value
// that rounds to r - at a power of two the spacing below is the smaller one.
inline double pxy_spacing(float r) {
    uint32_t b = 0;
    memcpy(&b, &r, sizeof(b));
    const int e = static_cast<int>((b >> 23) & 0xFFu);
    return std::ldexp(1.0, (e == 0 ? 1 : e) - 150);
}
// One helper's reading (r, SQF order) in the Draw3D sample seq, stamped q (gts_qpc): d = the
// output minus the reading. A second call for the same sample with the same reading (another binding on the same
// memory point, the sampler reading what the render thread read) hands back the first's. PXY_REPLAY: a second
// call for the same sample with ANOTHER reading - the render thread's reads within one cycle can see different poses
// (RT_READ_SER), and all carry the frame's own number - steps again from the state before that sample's step:
// the estimator still steps once per sample (never twice at a near-zero interval), on the frame's latest reading. Do
// not hand back the first reading's delta there: it puts every later reading off its track by it.
inline void pxy_smooth(PxyPoolEnt& e, uint64_t seq, int64_t q, const float r[3],
                       float d[3]) {
    if (e.sm_ok && e.sm_seq == seq) {
        if (e.sm_r[0] == r[0] && e.sm_r[1] == r[1] && e.sm_r[2] == r[2]) {
            memcpy(d, e.sm_d, sizeof(e.sm_d));
            return;
        }
        e.sm_ok = e.smp_ok;   // PXY_REPLAY: back to before this sample's step.
        e.sm_q = e.smp_q;
        memcpy(e.sm_dir, e.smp_dir, sizeof(e.sm_dir));
        memcpy(e.sm_r, e.smp_r, sizeof(e.sm_r));
        memcpy(e.sm_t, e.smp_t, sizeof(e.sm_t));
        memcpy(e.sm_x, e.smp_x, sizeof(e.sm_x));
        memcpy(e.sm_v, e.smp_v, sizeof(e.sm_v));
    } else {
        e.smp_ok = e.sm_ok;   // PXY_REPLAY: the state this sample's step starts from.
        e.smp_q = e.sm_q;
        memcpy(e.smp_dir, e.sm_dir, sizeof(e.smp_dir));
        memcpy(e.smp_r, e.sm_r, sizeof(e.smp_r));
        memcpy(e.smp_t, e.sm_t, sizeof(e.smp_t));
        memcpy(e.smp_x, e.sm_x, sizeof(e.smp_x));
        memcpy(e.smp_v, e.sm_v, sizeof(e.smp_v));
    }
    if (!cloth_finite3(r)) {
        d[0] = d[1] = d[2] = 0.0f;
        e.sm_ok = false;   // Nothing to hold to: the next reading starts it again.
        return;
    }
    const double dt = e.sm_ok && q > e.sm_q
                    ? static_cast<double>(q - e.sm_q) / static_cast<double>(prof_freq()) : 0.0;
    for (int k = 0; k < 3; ++k) {
        const double x = static_cast<double>(r[k]);
        const double s = pxy_spacing(r[k]);
        const double h = 0.5 * s;
        double& t = e.sm_t[k];
        double& o = e.sm_x[k];
        double& v = e.sm_v[k];
        if (!e.sm_ok || !(fabs(x - static_cast<double>(e.sm_r[k])) <= PXY_SMOOTH_RESET * s)) {
            t = x;   // reset: a teleport, or the first reading.
            o = x;
            v = 0.0;
            e.sm_dir[k] = 0;
        } else {
            const double lo = x - h;
            const double hi = x + h;
            const int8_t mv = t < lo ? 1 : (t > hi ? -1 : 0);   // target.
            if (mv != 0) {
                t = mv == e.sm_dir[k] ? x : (mv > 0 ? lo : hi);
                e.sm_dir[k] = mv;
            }
            if (dt > 0.0) {   // tracker.
                const double sp = fabs(v) / s;
                double u = (sp - PXY_SMOOTH_SPD_SLOW) / (PXY_SMOOTH_SPD_FAST - PXY_SMOOTH_SPD_SLOW);
                u = u < 0.0 ? 0.0 : (u > 1.0 ? 1.0 : u);
                const double w =
                    1.0 / (PXY_SMOOTH_TAU_SLOW + (PXY_SMOOTH_TAU_FAST - PXY_SMOOTH_TAU_SLOW) * u);
                const double a = 1.0 - exp(-2.0 * w * dt);
                const double bc = a * a / (2.0 - a);   // Critical damping.
                const double b = (w * dt) * (w * dt) < bc
                               ? (w * dt) * (w * dt) : bc;
                const double o0 = o;
                const double p = o + v * dt;
                const double err = t - p;
                o = p + a * err;
                v += (b / dt) * err;
                const double g = static_cast<double>(e.sm_dir[k]);   // Never against the target's way.
                if (g != 0.0 && (o - o0) * g < 0.0) {
                    o = o0;
                    if (v * g < 0.0) v = 0.0;
                }
            }
            const double c = o < x - s ? x - s
                           : (o > x + s ? x + s : o);   // clamp.
            if (c != o) {
                if (dt > 0.0) v += (c - o) / dt;
                o = c;
            }
        }
        e.sm_r[k] = r[k];
        d[k] = static_cast<float>(o - x);
    }
    e.sm_ok = true;
    e.sm_seq = seq;
    e.sm_q = q;
    memcpy(e.sm_d, d, sizeof(e.sm_d));
}
// The estimator's entry for one helper read - the sampler's and chain_prepare's (game thread), rt_paint_read's
// and chain_end_rt_read's (render thread): the estimator's delta for a memory-point helper the pool holds, zero for
// anything else (an origin helper, a proxy the pool never filed).
inline void pxy_smooth_note(const game_value& x, uint64_t seq, int64_t q, const float r[3],
                            float d[3]) {
    d[0] = d[1] = d[2] = 0.0f;
    const uintptr_t b = attach_base_of(x);
    if (b == 0) return;
    std::lock_guard<std::mutex> l(g_pxy_pool_mu);   // Both threads step (RT_PAINT_READ).
    const auto it = g_pxy_pool.find(b);
    if (it == g_pxy_pool.end() || !it->second.bone) return;
    pxy_smooth(it->second, seq, q, r, d);
}
// ATTACH_GT_SNAP - GAME THREAD, flush_frame (the Draw3D handler), once a frame and under g_draw_list_mutex:
// every binding's objects read TOGETHER - the parent (or a plain binding's object, or the object a rotation alone
// follows), the proxies, the rotation lane's own object - through attach_raw's copy rule, with page caches of this
// function's own. Draw3D is the instant the engine's own drawn state is coherent (a Draw3D icon at an object's visual
// position never falters). A failed PARENT read leaves the binding without a sample this frame (the step draws the
// ring's, or holds); a failed helper read is carried from the previous sample (GTS_HOLD); a failed read of the
// rotation lane's own object leaves the rotation holding. g_gts.done_pub is published once every binding is written
// (with a chain end followed, once that Draw3D's chain targets are filed too: g_gts.done_defer).
// True when this call took a sample (g_gts_seq names it): chain_prepare's choice (PXY_SMOOTH).
inline bool attach_gt_snap() {
    if (!attach_live()) return false;   // CHAIN_END_DRAW: a followed chain end is sampled by number too.
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    if (g_gts.done_defer != 0u) {   // CHAIN_END_DRAW: a deferral a thrown pass never published, first.
        g_gts.done_pub.store(g_gts.done_defer, std::memory_order_release);
        g_gts.done_defer = 0u;
    }
    const uint64_t seq = ++g_gts_seq;
    const int64_t q = gts_qpc();
    uint32_t crew_n = 0;   // CREW_LAG: bindings placed on a crew member, then chain ends (g_gts.crew_n).
    for (auto& kv : g_attach) {
        Attach& a = kv.second;
        // The object the binding is placed relative to, and the helpers it follows. ATTACH_ROT_ONLY: a binding with
        // no position lane is sampled on its rotation object, which the step then takes as the parent's.
        // ATTACH_MAN_HELPER: lpar is that object, par what is read for it.
        const game_value& lpar = attach_parent_gv(a);
        const size_t np = a.skel ? a.skel_proxy.size() : (a.proxy.is_nil() ? 0u : 1u);
        // A skeletal binding with no proxies (a mesh with no skeleton, or none of whose bones matched a memory point)
        // is sampled for its parent alone: its root, and so all of its geometry, follows the object.
        if (lpar.is_nil()) continue;
        const game_value& par = attach_org_read(a, lpar, seq);
        a.rt_par = par;        // RT_PAINT_READ: what the render thread reads for the parent.
        a.rt_rot = game_value();    // Set below when the rotation lane follows an object of its own.
        a.rt_rot_for = 0u;          // ATTACH_ROT_OF: likewise.
        const uint32_t gen = a.skel ? a.skel_gen : 0u;
        const uintptr_t xb = attach_base_of(attach_key_gv(a));
        const uintptr_t pb = attach_base_of(par);
        a.rt_for_gen = gen;    // ATTACH_ROT_OF: the binding rt_par is named for.
        a.rt_for_xb = xb;
        if (xb == 0 || pb == 0) continue;
        // ATTACH_SEED_PARENT: a memory-point proxy is placed by the simulation step after its attachTo
        // (attach_org_read's rule, made_seq + 1). The sample that first sees a new one is not taken, and the
        // render thread does not read it (rt_paint_read's rt_for_xb test): the mesh holds its seed.
        if (!a.skel && np == 1u) {
            const auto pe = g_pxy_pool.find(attach_base_of(a.proxy));
            if (pe != g_pxy_pool.end() && seq <= pe->second.made_seq + 1u) {
                a.rt_for_xb = 0u;
                continue;
            }
        }
        // The rotation lane, when it follows an object of its own (neither the parent nor the one helper) - named for
        // the render thread's read here, ahead of the reads below, so a Draw3D whose parent or helper read is refused
        // still leaves it named (as rt_par is) and the render thread's read turns it; its helper is stamped as the
        // parent's is above (attach_org_read: the same sample number, once).
        const void* const rd = a.obj_rot.is_nil() ? nullptr : a.obj_rot.data.get();
        const bool rown = rd && rd != lpar.data.get() &&
                          !(np == 1u && !a.skel && rd == a.proxy.data.get());
        if (rown) {
            a.rt_rot = attach_org_read(a, a.obj_rot, seq);   // ATTACH_MAN_HELPER.
            a.rt_rot_for = attach_base_of(a.obj_rot);   // ATTACH_ROT_OF.
        }
        if (a.gts_gen != gen || a.gts_xb != xb || a.gts_off.size() != np) {
            a.gts_off.assign(np, KH_ATTACH_OFF_NONE);   // Another binding on this handle: its caches are not ours.
            a.gts_vb.assign(np, 0u);
            a.gts_bb.assign(np, 0u);
            a.gts_avb.assign(np, 0u);
            a.gts_abb.assign(np, 0u);
            a.gts_gen = gen;
            a.gts_xb = xb;
        }
        if (a.gts_pb != pb) {
            a.gts_pb = pb;
            a.gts_par_off = KH_ATTACH_OFF_NONE;
            a.gts_par_vb = 0;
            a.gts_par_bb = 0;
            a.gts_par_avb = 0;
            a.gts_par_abb = 0;
        }
        GtSnap& s = a.gts[a.gts_w % GTS_N];
        s.ok = false;   // Not a sample until every read of it lands.
        s.rot_ok = false;
        s.rot_of = 0u;   // ATTACH_ROT_OF.
        float p[3], r[9];
        if (!gts_read(par, a.gts_par_off, a.gts_par_vb, a.gts_par_bb, a.gts_par_avb,
                      a.gts_par_abb, p, r)) continue;
        memcpy(s.par, p, sizeof(p));
        memcpy(s.par + 3, r, sizeof(r));
        // CREW_LAG: a crew member's helpers moved by its correction - the parent where its origin helper is read.
        CrewEnt* const cw = crew_of(lpar);
        if (cw && cw->in) ++crew_n;
        if (cw && par.data.get() == cw->helper.data.get()) {
            crew_apply(cw->t, cw->h, cw->dm, s.par, s.par + 3);
        }
        // The helpers on it: a memory point always (its parent read is a check), a skeleton's bones with the parent.
        const bool cwx = cw && (!a.skel || par.data.get() == cw->helper.data.get());
        s.px.resize(np * 12u);
        s.pxd.assign(np * 3u, 0.0f);   // PXY_SMOOTH: zero = the reading as it stands.
        // GTS_HOLD: the binding's previous sample, written before this one (another ring slot), when it is this
        // binding's - a refused helper read is carried from it; with none (the first sample after a bind), as before.
        const GtSnap& prev = a.gts[(a.gts_w + GTS_N - 1u) % GTS_N];
        const bool prev_ok = a.gts_w != 0u && prev.ok && prev.gen == gen &&
                             prev.xb == xb && prev.px.size() == np * 12u;
        bool ok = true;
        for (size_t j = 0; j < np; ++j) {
            const game_value& x = a.skel ? a.skel_proxy[j] : a.proxy;
            if (!gts_read(x, a.gts_off[j], a.gts_vb[j], a.gts_bb[j],
                          a.gts_avb[j], a.gts_abb[j], p, r)) {
                if (!prev_ok) { ok = false; break; }
                gts_carry(prev.par, s.par, &prev.px[j * 12u], &s.px[j * 12u]);
                if (prev.pxd.size() == np * 3u) {   // PXY_SMOOTH: the carried pose keeps its delta.
                    memcpy(&s.pxd[j * 3u], &prev.pxd[j * 3u], 3u * sizeof(float));
                }
                continue;
            }
            if (cwx) {   // CREW_LAG: every helper of the character, ahead of the estimator.
                crew_apply(cw->t, cw->h, cw->dm, p, r);
            }
            memcpy(&s.px[j * 12u], p, sizeof(p));
            memcpy(&s.px[j * 12u + 3u], r, sizeof(r));
            pxy_smooth_note(x, seq, q, p, &s.pxd[j * 3u]);   // PXY_SMOOTH.
        }
        if (!ok) continue;
        // The rotation lane's own object (named above).
        if (rown) {
            const game_value& ro = a.rt_rot;   // ATTACH_MAN_HELPER; RT_PAINT_READ.
            const uintptr_t rb = attach_base_of(ro);
            if (a.gts_rb != rb) {
                a.gts_rb = rb;
                a.gts_rot_off = KH_ATTACH_OFF_NONE;
                a.gts_rot_vb = 0;
                a.gts_rot_bb = 0;
                a.gts_rot_avb = 0;
                a.gts_rot_abb = 0;
            }
            if (gts_read(ro, a.gts_rot_off, a.gts_rot_vb, a.gts_rot_bb, a.gts_rot_avb,
                         a.gts_rot_abb, p, r)) {
                memcpy(s.rot, p, sizeof(p));
                memcpy(s.rot + 3, r, sizeof(r));
                s.rot_ok = true;
                s.rot_of = a.rt_rot_for;   // ATTACH_ROT_OF.
                // CREW_LAG: a crew member of its own, moved (so turned) as that character is drawn.
                const CrewEnt* const cr = crew_of(a.obj_rot);
                if (cr && cr->in && !(cw && cw->in)) ++crew_n;
                if (cr && ro.data.get() == cr->helper.data.get()) {
                    crew_apply(cr->t, cr->h, cr->dm, s.rot, s.rot + 3);
                }
            }
        }
        s.gen = gen;
        s.xb = xb;
        s.seq = seq;
        s.qpc = q;
        s.ok = true;
        ++a.gts_w;
    }
    crew_n += chain_end_crew_n();   // CREW_LAG: a followed chain end on a crew member waits too.
    g_gts.crew_n.store(crew_n, std::memory_order_relaxed);   // CREW_LAG.
    g_gts.stamp_pub[seq % GTS_STAMPS].store(q, std::memory_order_relaxed);   // Before the seq names it.
    if (g_chain_end_n.load(std::memory_order_relaxed) != 0u) {
        g_gts.done_defer = seq;   // CHAIN_END_DRAW: published with the chain ends' targets.
    } else {
        g_gts.done_pub.store(seq, std::memory_order_release);   // Every binding written: rt_own_absent.
    }
    return true;
}

// Runs from TEN sites, each immediately ahead of the scene read its draws are built from: on the render thread the
// injection, the cast fire (and its two CAST_REPICK steps ahead of the paint), the volume seam inject and the
// replay's presync of it (rp_replay, RP_PRESYNC), the in-front prepare and seam inject, and flush_locked's step
// when the injection did not land; on the game thread park_run's. The THREADS note on g_attach: this is the half
// either thread may run, and the half that may never erase. A lane is written only where the sampled value differs
// from the one the mesh already carries - the compare is exact, and a still object hands back the same floats every
// frame - so a parked vehicle costs the compare and nothing else: no scene mark, no grid move, no record upload.
// ctx: a render-thread drawer's context (the cast fire's step, the injection's, the render-thread flush's, the
// footprint seam's and its replay presync's, and the view-model passes' - SEAM_SKIN), with which a skeletal
// binding is skinned from the stash once per cycle and sample, a cloth's pinned part re-placed and a chain's end
// corrected; the park's step passes none.
// Every pose comes from the cycle's pick (gts_for_cycle): a Draw3D sample, or the render thread's read standing
// in for an absent one (RT_PAINT_READ: read at the cycle's first step, the cast fire's paint and the final step).
// The step reads no object itself, and a binding with no sample this cycle holds the pose it has.
inline void attach_step(ID3D11DeviceContext* ctx = nullptr) {
    KH_PROF_SCOPE(PROF_ATTACH_STEP);   // PROF.
    if (!attach_live()) return;   // CHAIN_END_DRAW: a followed chain end steps with no binding.
    if (reorder_on_render_thread() && g_gts.first_cyc != g_topo_cycles) {   // CAST_REPICK: the cycle's first step.
        g_gts.first_cyc = g_topo_cycles;
        g_gts.first_done = g_gts.done_pub.load(std::memory_order_acquire);
    }
    if (reorder_on_render_thread()) gts_step_prep();   // Before the mutex (the read takes it).
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    static std::vector<SkinDrawJob> jobs;   // Under g_draw_list_mutex (steps never overlap).
    jobs.clear();
    for (auto it = g_attach.begin(); it != g_attach.end(); ++it) {
        auto e = g_draw_list.find(it->first);
        // An orphan is SKIPPED, never erased: this runs on the render thread too, and erasing would release the
        // entry's game_values there. Every site that erases a draw-list entry drops its attachment in the same
        // critical section (attach_drop), so an orphan cannot outlive the entry by more than that moment.
        if (e == g_draw_list.end()) continue;
        Attach& a = it->second;
        RenderObject& o = e->second;
        // ATTACH_CYCLE: this cycle's consume-point sample stands. A game-thread step (park_run's) compares the
        // published counter, not the render thread's.
        const bool rt = reorder_on_render_thread();
        if (!rt && a.smp_cycle == g_topo_cycles_pub.load(std::memory_order_relaxed)) continue;
        // ATTACH_DEAD: the object this mesh follows is gone. Queue the handle and read nothing further from it -
        // QUEUED, not erased (the THREADS note on g_attach). Either lane counts: a mesh following a turret that no
        // longer exists has nothing left to follow, whatever its other lane says. ATTACH_BONE adds bone_parent,
        // not redundant with the two lanes: in bone mode the lanes hold the PROXY, and a proxy outlives its parent
        // (attachTo leaves it standing at the parent's last position, readable, forever). Without this third test a
        // mesh following a bone of a deleted vehicle would never die.
        if (attach_obj_dead(a.obj_pos) || attach_obj_dead(a.obj_rot) ||
            attach_obj_dead(a.bone_parent)) {
            attach_dead_queue(it->first);
            continue;
        }
        // Both lanes on the same object is the common case; it takes one read.
        const bool same = !a.obj_pos.is_nil() && !a.obj_rot.is_nil() &&
                          a.obj_pos.data.get() == a.obj_rot.data.get();
        float p[3], r[9];
        double root[3] = { 0.0, 0.0, 0.0 };   // SKEL: the root (POS_RES: exact), past the rotation read.
        // ATTACH_GT_SNAP: this cycle's sample of everything this binding follows, taken once for both lanes, on
        // either thread (a game-thread step reads the render thread's cycle, which the park holds still).
        const GtSnap* const gs =
            gts_for_cycle(a, rt ? g_topo_cycles : g_topo_cycles_pub.load(std::memory_order_relaxed), rt);
        if (!gs) continue;
        bool  raw = false;
        bool  moved = false;
        if (!a.obj_pos.is_nil()) {
            // The parent of a skeletal binding, the followed object of a plain one, the helper of a memory-point one.
            raw = true;
            const float* const src = a.skel || a.bone_parent.is_nil() ? gs->par : gs->px.data();
            memcpy(p, src, sizeof(p));
            memcpy(r, src + 3, sizeof(r));
            // POS_RES: the position exact, in double - a memory-point binding's helper where the estimator puts
            // it (PXY_SMOOTH: the sample's deltas, a render-thread read's included), the offset below composed
            // onto it - and stored as a world float plus its residual (pos_set_exact).
            double pd[3] = { p[0], p[1], p[2] };
            if (src != gs->par && gs->pxd.size() >= 3u) {
                for (int k = 0; k < 3; ++k) pd[k] += static_cast<double>(gs->pxd[k]);
            }
            // SKEL_SAMPLE: a skeletal binding's bones, stashed with the parent read beside them - one pair, the
            // one the skinning makes the bones relative to. A hidden mesh is stashed only when something other than
            // the draw reads its pose - its cloth's guide or its collider (the sync's own rule; skin->cloth /
            // want_collide are written by the sync under this mutex) - or it casts as a casterOnly (SKIN_CASTER),
            // or its buffer stands culled (SKIN_CULL: decided again at every step, whatever its flags); this step
            // skins it (the SKEL block below) only in the last two cases - for a guide or a collider alone it skins
            // nothing (the worker path serves those). Shown again, the next step of the cycle stashes and skins it
            // before any draw.
            const bool pose = skin_pose_drawn(o) || a.skin_culled ||   // SKIN_CASTER, _CULL.
                              (a.skin && skin_pose_read(*a.skin));
            if (a.skel && rt && pose &&
                !(ctx && a.skel_smp_ok && a.skel_smp_cycle == g_topo_cycles &&
                  a.skel_smp_seq == gs->seq)) {   // SKEL_DRAW: this cycle's pose stands.
                a.skel_smp_seq = gs->seq;
                const size_t np = a.skel_proxy.size();
                a.skel_smp_gen = a.skel_gen;
                a.skel_smp.resize(np * 12u);
                if (np != 0u) memcpy(a.skel_smp.data(), gs->px.data(), np * 12u * sizeof(float));
                memcpy(a.skel_smp_par, gs->par, sizeof(float) * 12u);
                a.skel_smp_d.assign(np * 3u, 0.0f);   // PXY_SMOOTH: the bones' deltas (none: zero).
                if (np != 0u && gs->pxd.size() == np * 3u) {
                    memcpy(a.skel_smp_d.data(), gs->pxd.data(), np * 3u * sizeof(float));
                }
                a.skel_smp_ok = true;
                a.skel_smp_ms = steady_now_ms();
            }
            // ATTACH_OFFSET, composed BEFORE the compare: a mesh with an offset still costs nothing on a frame its
            // object did not move, and a CHANGED offset needs no special case (the composed value stops matching what
            // the mesh carries). r is the position object's own basis, read here before the rotation lane below
            // can overwrite it.
            attach_offset_pos_d(o, r, pd);   // POS_RES.
            // SKEL: a skeletal mesh is drawn from its root plus a centre that turns with the rotation the block
            // below may still change, so its write waits for that block (skel_centre_d).
            if (a.skel) {
                memcpy(root, pd, sizeof(root));
            } else if (pos_set_exact(o, pd)) {
                moved = true;
            }
        }
        if (!a.obj_rot.is_nil()) {
            // Same object as the position lane: that read already has it. Otherwise the rotation lane's own object
            // from the sample - its own read, the one helper, or the parent; not in the sample, the rotation holds.
            bool rok = same;
            if (!rok) {
                const float* rs = nullptr;
                // ATTACH_ROT_OF: the sample's own object only while the lane still follows it.
                if (gs->rot_ok && gs->rot_of == attach_base_of(a.obj_rot)) rs = gs->rot;
                else if (!a.skel && !a.bone_parent.is_nil() && !a.proxy.is_nil() &&
                         a.obj_rot.data.get() == a.proxy.data.get() && gs->px.size() >= 12u) rs = gs->px.data();
                else if (a.obj_rot.data.get() == attach_parent_gv(a).data.get()) rs = gs->par;
                if (rs) {
                    rok = true;
                    memcpy(p, rs, sizeof(p));
                    memcpy(r, rs + 3, sizeof(r));
                }
            }
            // ATTACH_OFFSET. In place on r, which the position block above has finished with (it used the
            // basis without changing it; on the shared-object path this IS that basis).
            if (rok) attach_offset_rot(o, r);
            if (rok && memcmp(o.rot_m, r, sizeof(o.rot_m)) != 0) {
                memcpy(o.rot_m, r, sizeof(o.rot_m));
                o.rotated = attach_rotated(r);
                moved = true;
            }
        }
        bool defer = false;   // SKIN_FORK: this binding's tail waits for its vertices.
        if (raw && a.skel) {   // SKEL: the deferred position, now the rotation is final.
            if (ctx && (skin_pose_drawn(o) || a.skin_culled) &&   // SKIN_CASTER, SKIN_CULL.
                a.skel_smp_ok &&
                (a.skel_smp_cycle != g_topo_cycles || a.skel_draw_seq != a.skel_smp_seq)) {
                // SKEL_DRAW: once per cycle and sample, drawn meshes only.
                const uint64_t prev_draw = a.skel_draw_seq;   // SKIN_CULL: a culled job hands these back.
                const uint64_t prev_cyc = a.skel_smp_cycle;
                a.skel_draw_seq = a.skel_smp_seq;
                if (skin_draw_prep(a, o, ctx)) {
                    jobs.push_back(SkinDrawJob{ &it->first, &a, &o,
                                                       { root[0], root[1], root[2] },
                                                       { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, moved, false,
                                                       rt, VbRef{}, false, false, prev_cyc,
                                                       prev_draw });
                    defer = true;
                }
                cloth_pin_draw_step(a, o, ctx, moved);   // A simulating mesh's bound part.
                if (defer) jobs.back().moved = moved;
                a.skel_smp_cycle = g_topo_cycles;
            }
            if (!defer) attach_skel_place(o, root, moved);
        }
        if (!defer) attach_moved_note(o, moved, rt);
        if (rt) a.smp_cycle = g_topo_cycles;   // ATTACH_CYCLE.
    }
    if (jobs.empty()) {
        chain_end_draw(ctx);   // CHAIN_END_DRAW: after every binding is placed.
        return;
    }
    // SKIN_CULL: the jobs no pass can draw this cycle are culled - their bound taken, nothing drawn into their
    // buffers - ahead of the batch.
    skin_cull_pass(ctx, jobs);
    // SKIN_GPU: the bindings the GPU can skin are skinned there (so_vb set, their bound in bc / bs); the pool takes
    // what is left - none, once the stream-output objects and the bindings' buffers exist.
    skin_so_batch(ctx, jobs);
    bool cpu = false;
    {
        uint32_t ng = 0u, nc = 0u, nx = 0u;
        for (const SkinDrawJob& j : jobs) {
            if (j.culled) ++nx;
            else if (j.so_vb) ++ng;
            else { ++nc; cpu = true; }
        }
        skin_stat_note(ng, nc, nx);   // skinGpu / skinCpu / skinCulled.
    }
    // SKIN_FORK: the vertices of every binding this step skins, one chunk per binding, across the cloth pool with
    // this thread taking chunks too (serial, in order, when the pool is busy with another fork). Helpers leave for a
    // queued cloth step (the pool's cloth-first rule). A chunk that throws (an allocation) loses only its own
    // binding's upload this step.
    auto chunk = [](uint32_t i, uint32_t) {
        SkinDrawJob& j = jobs[i];
        if (j.so_vb) { j.ok = true; return; }   // SKIN_GPU: skinned already.
        try { skin_draw_verts(*j.a, j.bc, j.bs); j.ok = true; }
        catch (...) { j.ok = false; }
    };
    if (cpu) {
        cloth_fork_run(static_cast<uint32_t>(jobs.size()), cloth_fork_workers() + 1u,
                       &cloth_fork_call<decltype(chunk)>, &chunk, true);
    } else {
        for (SkinDrawJob& j : jobs) j.ok = true;   // SKIN_GPU: nothing for the pool.
    }
    // The tails, in loop order. SKIN_CULL: a culled job's finish takes the box, the shape key and the stamps
    // exactly as a skinned one's - only the bytes are the buffer's older ones.
    for (SkinDrawJob& j : jobs) {
        if (j.ok) skin_draw_finish(*j.h, *j.a, *j.o, ctx, j.bc, j.bs, j.moved, j.so_vb);
        if (j.ok) j.a->skin_culled = j.culled;
        else if (j.a->skin_culled) {   // SKIN_CULL: older bytes stand under a newer box: decided again.
            j.a->skel_smp_cycle = j.prev_smp_cycle;
            j.a->skel_draw_seq = j.prev_draw_seq;
        }
        attach_skel_place(*j.o, j.root, j.moved);
        attach_moved_note(*j.o, j.moved, j.rt);
    }
    jobs.clear();
    chain_end_draw(ctx);   // CHAIN_END_DRAW: after every binding is placed and skinned.
}
}