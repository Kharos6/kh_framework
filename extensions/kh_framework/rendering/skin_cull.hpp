#pragma once

// The feature half (rendering_integration_features.hpp): SKIN_CULL, the culling pass of GPU-skinned bindings. Never
// include it on its own.
namespace RenderIntegration {
// SKIN_CULL (defined below; render_sun_depth, among the passes it reasons about, is in
// rendering/sun_shadow_render.hpp): which of a step's jobs no pass can draw this cycle.
inline void skin_cull_pass(ID3D11DeviceContext* ctx, std::vector<SkinDrawJob>& jobs);
inline void skin_cull_catchup(ID3D11DeviceContext* ctx, std::vector<RenderObject>* only = nullptr);

// Defined in rendering/sun_shadow_render.hpp (declared here for the uses below).
inline bool sun_cam_pick(float cam_e[3]);
inline bool sun_admits(const RenderObject& o, const float cam_e[3], float rad);


// SKIN_CULL (the rule is set out at skin_cull_pass): a culled binding may exist (render thread; the park holds
// it still), and the least and greatest shadow range a cull read while one may (any other range re-admits).
static bool  g_skin_cull_any = false;
static float g_skin_cull_rad_lo = 0.0f, g_skin_cull_rad_hi = 0.0f;

// SKIN_CULL - the exact rule: a step leaves a GPU-skinned skeletal binding unskinned when no pass can draw any of
// its pose this cycle, and skins it the moment one can. The test is the pose's own bound (the trees' box, the box the
// binding carries either way), and a culled binding's finish takes that box, its shape key and the stamps exactly as a
// skinned one's: every reader of the box, the key or the stamps (culling, sorting, LOD, the near-clearance, the caster
// hashes, the upload's and the sync's ownership) sees what it would have seen; only the stream-output draw is left
// out, so the buffer's bytes are an older pose that nothing rasterises.
// Admission, against every pass that can draw a skeletal binding:
// - the sun maps: render_sun_depth's own caster rule (sun_cam_pick and sun_admits, shared), whenever a sun is
//   published;
// - the DLS maps and the dlsw mask: within any pool light's reach (dls_far's floor) of the bound (a caster, a reach
//   owner) - the slots dls_select fills this cycle copy pool lights' positions and reaches, and the pool it reads
//   is the one the step saw: the injection's harvest follows its select, the flush's fallback harvest precedes the
//   flush's own step; or dlsw_view_covers against every pool light's sphere at the largest shade multiple any slot
//   has (the owners the mask takes by view);
// - the camera passes (the injection's colour, its SSAO marks, the seam footprint and the mirror prepass; the prime
//   and the dlsw mask draw a snapshot taken behind such a step, under its view): the ball is behind the eye plane of
//   the latch's view, of its frame-view adoption (adopt_frame_view, evaluated here, in the hook the pass then runs
//   in) and of the live view, with every eye within the live camera's and the seam take's offsets of the latch's - so
//   its clip w is negative in every one of those views, whatever the projection (zoom, raster copy, near plane), and
//   the GPU clips it. Each of those passes is preceded by its own step in the same hook, which decides again for a
//   culled binding (it stays unstamped, and the gates take it whatever its flags).
// Each term is geometry alone - every binding is tested as if it were visible, cast and took part - so no flag a
// script flips between this step and a pass can bring a culled binding into the pass. Never culled: a hidden binding
// (posed by no pass: one whose buffer stands culled is skinned at its next step, and leaves the gates); an inFront,
// cloth, chain, vertex-stage or collider binding; a visible one that is translucent, timed, or not a composite mesh
// (the flush draws those late, under its own view); one the GPU would not skin; one whose buffer is not the slot's;
// with no latch of this cycle's own (stale, or the live jump rule's), nothing. A pass with no step ahead of it skins
// what was culled first (skin_cull_catchup): the PIP and the park's flush every culled binding, the compile-time
// white draw likewise, and a flush the bindings its walk copied for a late draw (a flag changed since the step, or no
// landing).
static constexpr bool  SKIN_CULL_ON = true;
static constexpr float SKIN_CULL_PAD_M = 0.5f;          // The eye offsets' float slack.
static constexpr float SKIN_CULL_MARGIN = 0.035f;       // Radians past the eye plane (2 degrees).
// A pass that would skin every culled binding anyway - a PIP drew within this many cycles, or the last cycle did not
// land (its flush drew the meshes late) - makes a cull cost more than the skin: none then.
static constexpr uint64_t SKIN_CULL_PIP_CYCLES = 2u;
struct SkinCullCam {
    float eye[3];      // The latch's eye (engine axes).
    float fwd[3][3];   // Unit forward axes: the latch's, its adoption's, the live view's.
    int   nf;
    float pad;         // The farthest eye of those passes from eye.
};
// True when no point of the ball (centre, engine axes; radius) is in front of any of the views' eye planes, by the
// margin: behind every one, clip w < 0.
inline bool skin_cull_behind(const SkinCullCam& c, const float ce[3], float r) {
    const float v[3] = { ce[0] - c.eye[0], ce[1] - c.eye[1], ce[2] - c.eye[2] };
    const float d = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    const float rr = r + c.pad;
    if (!(d > rr) || !(rr >= 0.0f)) return false;
    const float t = asinf(rr / d) + SKIN_CULL_MARGIN;   // The ball's angular radius, and the margin.
    if (!(t < 1.5f)) return false;
    const float lim = -sinf(t);
    for (int i = 0; i < c.nf; ++i) {
        const float* const f = c.fwd[i];
        const float cos = (v[0] * f[0] + v[1] * f[1] + v[2] * f[2]) / d;
        if (!(cos <= lim)) return false;   // In front of this eye plane, or within the margin of it.
    }
    return true;
}
// A view's forward axis (its z column: w = (p - eye) . that column), unit; false for a degenerate one.
inline bool skin_cull_fwd(const float v[4][4], float o[3]) {
    const float l = sqrtf(v[0][2] * v[0][2] + v[1][2] * v[1][2] + v[2][2] * v[2][2]);
    if (!(l > 1.0e-6f)) return false;
    for (int k = 0; k < 3; ++k) o[k] = v[k][2] / l;
    return true;
}
inline void skin_cull_pass(ID3D11DeviceContext* ctx, std::vector<SkinDrawJob>& jobs) {
    if (!SKIN_CULL_ON || !ctx || jobs.empty() || !reorder_on_render_thread()) return;
    if (!g_ro.cycle_pv_valid || g_ro.cycle_pv_stale || !g_boundary_pv_valid) return;   // No latch of the cycle's own.
    if (g_skin_cull_pip_cycle != ~0ull && g_topo_cycles >= g_skin_cull_pip_cycle &&
        g_topo_cycles - g_skin_cull_pip_cycle <= SKIN_CULL_PIP_CYCLES) return;   // A PIP skins them anyway.
    if (g_composite_land_cycle == ~0ull || g_topo_cycles > g_composite_land_cycle + 1u) return;   // So would the flush.
    // The camera passes' views, as the passes after this step take them.
    SkinCullCam cam = {};
    cam_of(g_ro.cycle_pv.view, cam.eye);
    if (!skin_cull_fwd(g_ro.cycle_pv.view, cam.fwd[0])) return;
    cam.nf = 1;
    {
        RVExtBridge::ProjectionViewTransform ad = g_ro.cycle_pv;
        if (adopt_frame_view(ad) && skin_cull_fwd(ad.view, cam.fwd[cam.nf])) ++cam.nf;
    }
    float pad = 0.0f;
    {
        RVExtBridge::ProjectionViewTransform lv = {};
        if (RVExtBridge::get_projection_view_transform(lv)) {
            float le[3];
            cam_of(lv.view, le);
            if (!skin_cull_fwd(lv.view, cam.fwd[cam.nf])) return;
            ++cam.nf;
            auto dist = [](const float* x, const float* y) {
                return sqrtf((x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]) +
                             (x[2] - y[2]) * (x[2] - y[2]));
            };
            pad = dist(le, cam.eye);   // The live eye.
            // The seam take's offset.
            if (g_seam_take.live_lp_ok) pad = fmaxf(pad, dist(le, g_seam_take.live_lp));
        }
    }
    cam.pad = pad + SKIN_CULL_PAD_M;
    if (!(cam.pad < 1.0e6f)) return;
    // The sun: render_sun_depth's gates and camera.
    float sc[3];
    const bool sc_ok = sun_cam_pick(sc);
    const bool sun = (g_sun_valid || sun_axis_witnessed()) && shadow_sun() != nullptr;
    const float rad = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    // The DLS: the dlsw owners' camera (dls_frame reads g_sun_setup.sun_cam_now, which the sun pass sets to this
    // pick, or zero camera-less), and the largest shade multiple of any slot.
    const bool dcam = sc_ok && (sc[0] != 0.0f || sc[1] != 0.0f || sc[2] != 0.0f);
    float k = sqrtf(3.0f);
    for (uint32_t s = 0; s < DLS_MAX; ++s) k = fmaxf(k, dls_shade_k(s));
    // Each pool light's reach and view terms, once for every job (they read no job).
    static std::vector<float> cp_far;
    static std::vector<DlswLight> vls;
    const uint32_t pn = g_dl.pool_n;
    cp_far.resize(pn);
    vls.resize(pn);
    for (uint32_t i = 0; i < pn; ++i) {
        cp_far[i] = fmaxf(dls_reach(g_dl.pool[i]), DLS_REACH_MIN_M);
        if (dcam) {
            dlsw_view_light(sc, g_dl.pool[i].rec, k * 1.05f * cp_far[i], vls[i]);
        }
    }
    for (SkinDrawJob& j : jobs) {
        RenderObject& o = *j.o;
        Attach& a = *j.a;
        const SkinInst* const in = a.skin.get();
        if (!in || !in->rest_draw || in->cloth || in->want_collide) continue;
        if (o.in_front || o.cloth_sim || o.chain_sim || (o.materials && o.materials->vertex_any)) continue;
        if (!skin_pose_drawn(o)) continue;   // Hidden: skinned once, then out of the gates (no step decides it).
        if (o.visible && (!is_composite_eligible(o) || o.timed || o.localized || o.banded)) continue;
        SbTarget tg = { VbRef{}, nullptr, 0u };
        {
            std::lock_guard<std::mutex> g(g_skin_mu);   // g_draw_list_mutex -> g_skin_mu, the batch's order.
            tg = skin_so_target(j);
        }
        if (!tg.vb || o.slot >= g_cloth_vb_slot.size() || g_cloth_vb_slot[o.slot] != tg.vb.vb ||
            o.slot >= g_cloth_voff_slot.size() || g_cloth_voff_slot[o.slot] != tg.vb.off ||   // SKIN_POOL.
            o.slot >= g_cloth_mesh_slot.size() || g_cloth_mesh_slot[o.slot] != o.mesh) continue;
        // The bound: the batch's function of the same inputs, reused while they stand.
        if (a.cull_rest != in->rest_draw || a.cull_a != a.draw_a) {
            skin_so_bound(*in->rest_draw, a.draw_a.data(), a.cull_bc, a.cull_bs);
            a.cull_rest = in->rest_draw;
            a.cull_a = a.draw_a;
        }
        memcpy(j.bc, a.cull_bc, sizeof(j.bc));
        memcpy(j.bs, a.cull_bs, sizeof(j.bs));
        j.bnd = true;
        // The object as the finish and the placement will leave it (skin_draw_finish's box, attach_skel_place's
        // centre), put in place for the tests and taken back.
        float pos[3], res[3], rat[3], sz[3], ct[3];
        memcpy(pos, o.pos, sizeof(pos));
        memcpy(res, o.pos_res, sizeof(res));
        memcpy(rat, o.pos_res_at, sizeof(rat));
        memcpy(sz, o.size, sizeof(sz));
        memcpy(ct, o.skel_ctr, sizeof(ct));
        memcpy(o.skel_ctr, j.bc, sizeof(o.skel_ctr));
        o.size[0] = j.bs[0]; o.size[1] = j.bs[2]; o.size[2] = j.bs[1];
        {
            double c[3] = { j.root[0], j.root[1], j.root[2] };
            skel_centre_d(o, c);
            pos_set_exact(o, c);
        }
        // Each term is the pass's geometry alone, whatever the binding's flags say (visible, casterOnly, castShadow,
        // lit...): a script's change landing between this step and a pass's read of the scene cannot let a pass
        // draw what was culled.
        bool draw = sun && (!sc_ok || sun_admits(o, sc, rad));
        float ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
        float he[3];
        world_half_extents(o, he);
        const float hd = sqrtf(he[0] * he[0] + he[1] * he[1] + he[2] * he[2]);
        if (!draw && pn > 0u) {
            for (uint32_t i = 0; i < pn && !draw; ++i) {
                const DlPoolLight& l = g_dl.pool[i];
                {   // A caster or a reach owner.
                    const float dx = ce[0] - l.rec[0];
                    const float dy = ce[1] - l.rec[1];
                    const float dz = ce[2] - l.rec[2];
                    const float lim = cp_far[i] + hd;
                    if (!(dx * dx + dy * dy + dz * dz > lim * lim)) {
                        draw = true;
                    }
                }
                if (!draw) {   // A view owner.
                    if (!dcam) {
                        draw = true;   // Camera-less: every visible one joins.
                    } else {
                        draw = dlsw_view_covers(sc, ce, hd, vls[i]);
                    }
                }
            }
        }
        if (!draw) draw = !skin_cull_behind(cam, ce, hd);
        memcpy(o.pos, pos, sizeof(pos));
        memcpy(o.pos_res, res, sizeof(res));
        memcpy(o.pos_res_at, rat, sizeof(rat));
        memcpy(o.size, sz, sizeof(sz));
        memcpy(o.skel_ctr, ct, sizeof(ct));
        if (draw) continue;
        j.culled = true;
        j.so_vb = tg.vb;
        a.skel_smp_cycle = j.prev_smp_cycle;   // Unstamped: every later step of the cycle decides again.
        a.skel_draw_seq = j.prev_draw_seq;
        if (!g_skin_cull_any) {   // render_sun_depth's range check.
            g_skin_cull_rad_lo = rad;
            g_skin_cull_rad_hi = rad;
        } else {
            g_skin_cull_rad_lo = fminf(g_skin_cull_rad_lo, rad);
            g_skin_cull_rad_hi = fmaxf(g_skin_cull_rad_hi, rad);
        }
        g_skin_cull_any = true;
    }
}
// SKIN_CULL: a pass no step precedes draws what the culled bindings' boxes say, so their buffers take this
// stash's pose first, by the step's own path - the batch, the pool for what it leaves, the finish and the
// placement about the root the binding stands on. only null: every culled binding (the PIP, the park, the
// white draw, and render_sun_depth when the shadow range moved off the one the steps culled against). Else the
// flush's late-draw copies: the culled bindings among them, and the copies then take the boxes those poses carry.
// Takes g_draw_list_mutex itself (a caller must not hold it); render thread, or the park (the render thread
// stalled).
inline void skin_cull_catchup(ID3D11DeviceContext* ctx, std::vector<RenderObject>* only) {
    if (!g_skin_cull_any || !ctx) return;
    static std::vector<uint8_t> mark;   // By scene slot: a skeletal copy is in only.
    if (only) {
        mark.assign(g_cloth_vb_slot.size(), 0u);
        bool any = false;
        for (const RenderObject& m : *only) {
            // WSEE: an injection's deferred copy with its geometry held keeps the injection's pose and box.
            if (!m.skel || m.ws_hold != 0u || m.slot >= mark.size()) continue;
            mark[m.slot] = 1u;
            any = true;
        }
        if (!any) return;
    }
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    static std::vector<SkinDrawJob> jobs;
    jobs.clear();
    const bool rt = reorder_on_render_thread();
    // STEP_OWN_FLAG: the buffer writes below are ours (the batch raises the flag for its draws; the finish writes a
    // buffer the batch did not take) - under the flag on the hooks' render thread they stand aside. The PIP and the
    // sun pass's range check (from the cast fire) call this with it down; the render thread's flushes run under it
    // already. Raised and restored on that thread only (fx_late_gt's rule): on any other the hooks admit nothing.
    std::optional<InjectionFlagScope> flag;
    if (rt) {
        flag.emplace();
        g_ro.in_injection = true;
    }
    bool left = false;
    for (auto it = g_attach.begin(); it != g_attach.end(); ++it) {
        Attach& a = it->second;
        if (!a.skin_culled) continue;
        const auto e = g_draw_list.find(it->first);
        if (e == g_draw_list.end() || !a.skel) { a.skin_culled = false; continue; }
        RenderObject& o = e->second;
        if (only && (o.slot >= mark.size() || !mark[o.slot])) {
            left = true;   // Not among the copies: it stands culled.
            continue;
        }
        if (!a.skel_smp_ok || !skin_draw_prep(a, o, ctx)) {
            left = true;   // No pose to take: it stands culled.
            continue;
        }
        // The root the binding stands on: its centre less its box's offset (skel_centre_d's inverse).
        double c[3], d[3] = { 0.0, 0.0, 0.0 };
        pos_exact(o, c);
        skel_centre_d(o, d);
        jobs.push_back(SkinDrawJob{ &it->first, &a, &o,
                                           { c[0] - d[0], c[1] - d[1], c[2] - d[2] },
                                           { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, false, false, rt, VbRef{},
                                           false, false, a.skel_smp_cycle, a.skel_draw_seq });
        // The cull's bound, while its inputs stand (the same function of the same inputs as the batch's).
        const SkinInst* const in = a.skin.get();
        if (in && in->rest_draw && a.cull_rest == in->rest_draw && a.cull_a == a.draw_a) {
            SkinDrawJob& nj = jobs.back();
            memcpy(nj.bc, a.cull_bc, sizeof(nj.bc));
            memcpy(nj.bs, a.cull_bs, sizeof(nj.bs));
            nj.bnd = true;
        }
    }
    g_skin_cull_any = left;
    if (jobs.empty()) return;
    skin_so_batch(ctx, jobs);
    uint32_t ng = 0u, nc = 0u;
    for (SkinDrawJob& j : jobs) {   // The pool's part, serially: the step's fork, for a rare pass.
        if (j.so_vb) { j.ok = true; ++ng; continue; }
        ++nc;
        try { skin_draw_verts(*j.a, j.bc, j.bs); j.ok = true; }
        catch (...) { j.ok = false; }
    }
    if (rt) skin_stat_note(ng, nc, 0u);
    for (SkinDrawJob& j : jobs) {   // TWIN: attach_step's tails.
        if (j.ok) {
            skin_draw_finish(*j.h, *j.a, *j.o, ctx, j.bc, j.bs, j.moved,
                             j.so_vb);
            j.a->skin_culled = false;
            if (rt) {   // This sample is skinned: the cycle's later steps keep it, as after a step's skin.
                j.a->skel_draw_seq = j.a->skel_smp_seq;
                j.a->skel_smp_cycle = g_topo_cycles;
            }
        } else {
            g_skin_cull_any = true;
        }
        attach_skel_place(*j.o, j.root, j.moved);
        attach_moved_note(*j.o, j.moved, j.rt);
        if (!only || !j.ok) continue;
        for (RenderObject& m : *only) {   // The copies draw the box this pose carries.
            if (!m.skel || m.ws_hold != 0u || m.slot != j.o->slot) continue;   // WSEE: held.
            memcpy(m.pos, j.o->pos, sizeof(m.pos));
            memcpy(m.pos_res, j.o->pos_res, sizeof(m.pos_res));
            memcpy(m.pos_res_at, j.o->pos_res_at, sizeof(m.pos_res_at));
            memcpy(m.size, j.o->size, sizeof(m.size));
            memcpy(m.skel_ctr, j.o->skel_ctr, sizeof(m.skel_ctr));
        }
    }
}
}