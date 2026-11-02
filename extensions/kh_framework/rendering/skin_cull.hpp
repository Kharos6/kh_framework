#pragma once

// The feature half (rendering_integration_features.hpp): KH_SKIN_CULL, the culling pass of GPU-skinned bindings. Never
// include it on its own.
namespace RenderIntegration {
// KH_SKIN_CULL (defined below; render_sun_depth, among the passes it reasons about, is in
// rendering/sun_shadow_render.hpp): which of a step's jobs no pass can draw this cycle.
inline void kh_skin_cull_pass(ID3D11DeviceContext* khcp_ctx, std::vector<KhSkinDrawJob>& khcp_jobs);
inline void kh_skin_cull_catchup(ID3D11DeviceContext* khcc_ctx, std::vector<RenderObject>* khcc_only = nullptr);

// Defined in rendering/sun_shadow_render.hpp (declared here for the uses below).
inline bool kh_sun_cam_pick(float cam_e[3]);
inline bool kh_sun_admits(const RenderObject& o, const float cam_e[3], float khsr_rad);


// KH_SKIN_CULL (the rule is set out at kh_skin_cull_pass): a culled binding may exist (render thread; the park holds
// it still), and the least and greatest shadow range a cull read while one may (any other range re-admits).
static bool  g_skin_cull_any = false;
static float g_skin_cull_rad_lo = 0.0f, g_skin_cull_rad_hi = 0.0f;

// KH_SKIN_CULL - the exact rule: a step leaves a GPU-skinned skeletal binding unskinned when no pass can draw any of
// its pose this cycle, and skins it the moment one can. The test is the pose's own bound (the trees' box, the box the
// binding carries either way), and a culled binding's finish takes that box, its shape key and the stamps exactly as a
// skinned one's: every reader of the box, the key or the stamps (culling, sorting, LOD, the near-clearance, the caster
// hashes, the upload's and the sync's ownership) sees what it would have seen; only the stream-output draw is left
// out, so the buffer's bytes are an older pose that nothing rasterises.
// Admission, against every pass that can draw a skeletal binding:
// - the sun maps: render_sun_depth's own caster rule (kh_sun_cam_pick and kh_sun_admits, shared), whenever a sun is
//   published;
// - the DLS maps and the dlsw mask: within any pool light's reach (kh_dls_far's floor) of the bound (a caster, a reach
//   owner) - the slots kh_dls_select fills this cycle copy pool lights' positions and reaches, and the pool it reads
//   is the one the step saw: the injection's harvest follows its select, the flush's fallback harvest precedes the
//   flush's own step; or kh_dlsw_view_covers against every pool light's sphere at the largest shade multiple any slot
//   has (the owners the mask takes by view);
// - the camera passes (the injection's colour, its SSAO marks, the seam footprint and the mirror prepass; the prime
//   and the dlsw mask draw a snapshot taken behind such a step, under its view): the ball is behind the eye plane of
//   the latch's view, of its frame-view adoption (kh_adopt_frame_view, evaluated here, in the hook the pass then runs
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
// what was culled first (kh_skin_cull_catchup): the PIP and the park's flush every culled binding, the compile-time
// white draw likewise, and a flush the bindings its walk copied for a late draw (a flag changed since the step, or no
// landing).
static constexpr bool  KH_SKIN_CULL_ON = true;
static constexpr float KH_SKIN_CULL_PAD_M = 0.5f;          // The eye offsets' float slack.
static constexpr float KH_SKIN_CULL_MARGIN = 0.035f;       // Radians past the eye plane (2 degrees).
// A pass that would skin every culled binding anyway - a PIP drew within this many cycles, or the last cycle did not
// land (its flush drew the meshes late) - makes a cull cost more than the skin: none then.
static constexpr uint64_t KH_SKIN_CULL_PIP_CYCLES = 2u;
struct KhSkinCullCam {
    float eye[3];      // The latch's eye (engine axes).
    float fwd[3][3];   // Unit forward axes: the latch's, its adoption's, the live view's.
    int   nf;
    float pad;         // The farthest eye of those passes from eye.
};
// True when no point of the ball (centre, engine axes; radius) is in front of any of the views' eye planes, by the
// margin: behind every one, clip w < 0.
inline bool kh_skin_cull_behind(const KhSkinCullCam& khcb_c, const float khcb_ce[3], float khcb_r) {
    const float khcb_v[3] = { khcb_ce[0] - khcb_c.eye[0], khcb_ce[1] - khcb_c.eye[1], khcb_ce[2] - khcb_c.eye[2] };
    const float khcb_d = sqrtf(khcb_v[0] * khcb_v[0] + khcb_v[1] * khcb_v[1] + khcb_v[2] * khcb_v[2]);
    const float khcb_rr = khcb_r + khcb_c.pad;
    if (!(khcb_d > khcb_rr) || !(khcb_rr >= 0.0f)) return false;
    const float khcb_t = asinf(khcb_rr / khcb_d) + KH_SKIN_CULL_MARGIN;   // The ball's angular radius, and the margin.
    if (!(khcb_t < 1.5f)) return false;
    const float khcb_lim = -sinf(khcb_t);
    for (int khcb_i = 0; khcb_i < khcb_c.nf; ++khcb_i) {
        const float* const khcb_f = khcb_c.fwd[khcb_i];
        const float khcb_cos = (khcb_v[0] * khcb_f[0] + khcb_v[1] * khcb_f[1] + khcb_v[2] * khcb_f[2]) / khcb_d;
        if (!(khcb_cos <= khcb_lim)) return false;   // In front of this eye plane, or within the margin of it.
    }
    return true;
}
// A view's forward axis (its z column: w = (p - eye) . that column), unit; false for a degenerate one.
inline bool kh_skin_cull_fwd(const float khcf_v[4][4], float khcf_o[3]) {
    const float khcf_l = sqrtf(khcf_v[0][2] * khcf_v[0][2] + khcf_v[1][2] * khcf_v[1][2] + khcf_v[2][2] * khcf_v[2][2]);
    if (!(khcf_l > 1.0e-6f)) return false;
    for (int k = 0; k < 3; ++k) khcf_o[k] = khcf_v[k][2] / khcf_l;
    return true;
}
inline void kh_skin_cull_pass(ID3D11DeviceContext* khcp_ctx, std::vector<KhSkinDrawJob>& khcp_jobs) {
    if (!KH_SKIN_CULL_ON || !khcp_ctx || khcp_jobs.empty() || !reorder_on_render_thread()) return;
    if (!g_ro.cycle_pv_valid || g_ro.cycle_pv_stale || !g_boundary_pv_valid) return;   // No latch of the cycle's own.
    if (g_skin_cull_pip_cycle != ~0ull && g_topo_cycles >= g_skin_cull_pip_cycle &&
        g_topo_cycles - g_skin_cull_pip_cycle <= KH_SKIN_CULL_PIP_CYCLES) return;   // A PIP skins them anyway.
    if (g_composite_land_cycle == ~0ull || g_topo_cycles > g_composite_land_cycle + 1u) return;   // So would the flush.
    // The camera passes' views, as the passes after this step take them.
    KhSkinCullCam khcp_cam = {};
    kh_cam_of(g_ro.cycle_pv.view, khcp_cam.eye);
    if (!kh_skin_cull_fwd(g_ro.cycle_pv.view, khcp_cam.fwd[0])) return;
    khcp_cam.nf = 1;
    {
        RVExtBridge::ProjectionViewTransform khcp_ad = g_ro.cycle_pv;
        if (kh_adopt_frame_view(khcp_ad) && kh_skin_cull_fwd(khcp_ad.view, khcp_cam.fwd[khcp_cam.nf])) ++khcp_cam.nf;
    }
    float khcp_pad = 0.0f;
    {
        RVExtBridge::ProjectionViewTransform khcp_lv = {};
        if (RVExtBridge::get_projection_view_transform(khcp_lv)) {
            float khcp_le[3];
            kh_cam_of(khcp_lv.view, khcp_le);
            if (!kh_skin_cull_fwd(khcp_lv.view, khcp_cam.fwd[khcp_cam.nf])) return;
            ++khcp_cam.nf;
            auto khcp_dist = [](const float* x, const float* y) {
                return sqrtf((x[0] - y[0]) * (x[0] - y[0]) + (x[1] - y[1]) * (x[1] - y[1]) +
                             (x[2] - y[2]) * (x[2] - y[2]));
            };
            khcp_pad = khcp_dist(khcp_le, khcp_cam.eye);   // The live eye.
            if (g_khlt_lp_ok) khcp_pad = fmaxf(khcp_pad, khcp_dist(khcp_le, g_khlt_lp));   // The seam take's offset.
        }
    }
    khcp_cam.pad = khcp_pad + KH_SKIN_CULL_PAD_M;
    if (!(khcp_cam.pad < 1.0e6f)) return;
    // The sun: render_sun_depth's gates and camera.
    float khcp_sc[3];
    const bool khcp_sc_ok = kh_sun_cam_pick(khcp_sc);
    const bool khcp_sun = (g_sun_valid || kh_sun_axis_witnessed()) && kh_shadow_sun() != nullptr;
    const float khcp_rad = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    // The DLS: the dlsw owners' camera (kh_dls_frame reads g_sun_cam_now, which the sun pass sets to this pick, or
    // zero camera-less), and the largest shade multiple of any slot.
    const bool khcp_dcam = khcp_sc_ok && (khcp_sc[0] != 0.0f || khcp_sc[1] != 0.0f || khcp_sc[2] != 0.0f);
    float khcp_k = sqrtf(3.0f);
    for (uint32_t khcp_s = 0; khcp_s < KH_DLS_MAX; ++khcp_s) khcp_k = fmaxf(khcp_k, kh_dls_shade_k(khcp_s));
    // Each pool light's reach and view terms, once for every job (they read no job).
    static std::vector<float> khcp_far;
    static std::vector<KhDlswLight> khcp_vls;
    const uint32_t khcp_pn = g_dl.pool_n;
    khcp_far.resize(khcp_pn);
    khcp_vls.resize(khcp_pn);
    for (uint32_t khcp_i = 0; khcp_i < khcp_pn; ++khcp_i) {
        khcp_far[khcp_i] = fmaxf(kh_dls_reach(g_dl.pool[khcp_i]), KH_DLS_REACH_MIN_M);
        if (khcp_dcam) {
            kh_dlsw_view_light(khcp_sc, g_dl.pool[khcp_i].rec, khcp_k * 1.05f * khcp_far[khcp_i], khcp_vls[khcp_i]);
        }
    }
    for (KhSkinDrawJob& j : khcp_jobs) {
        RenderObject& o = *j.o;
        KhAttach& a = *j.a;
        const KhSkinInst* const in = a.skin.get();
        if (!in || !in->rest_draw || in->cloth || in->want_collide) continue;
        if (o.in_front || o.cloth_sim || o.chain_sim || (o.materials && o.materials->vertex_any)) continue;
        if (!kh_skin_pose_drawn(o)) continue;   // Hidden: skinned once, then out of the gates (no step decides it).
        if (o.visible && (!is_composite_eligible(o) || o.timed || o.localized || o.banded)) continue;
        KhSbTarget khcp_tg = { KhVbRef{}, nullptr, 0u };
        {
            std::lock_guard<std::mutex> khcp_g(g_skin_mu);   // g_draw_list_mutex -> g_skin_mu, the batch's order.
            khcp_tg = kh_skin_so_target(j);
        }
        if (!khcp_tg.vb || o.slot >= g_cloth_vb_slot.size() || g_cloth_vb_slot[o.slot] != khcp_tg.vb.vb ||
            o.slot >= g_cloth_voff_slot.size() || g_cloth_voff_slot[o.slot] != khcp_tg.vb.off ||   // KH_SKIN_POOL.
            o.slot >= g_cloth_mesh_slot.size() || g_cloth_mesh_slot[o.slot] != o.mesh) continue;
        // The bound: the batch's function of the same inputs, reused while they stand.
        if (a.cull_rest != in->rest_draw || a.cull_a != a.draw_a) {
            kh_skin_so_bound(*in->rest_draw, a.draw_a.data(), a.cull_bc, a.cull_bs);
            a.cull_rest = in->rest_draw;
            a.cull_a = a.draw_a;
        }
        memcpy(j.bc, a.cull_bc, sizeof(j.bc));
        memcpy(j.bs, a.cull_bs, sizeof(j.bs));
        j.bnd = true;
        // The object as the finish and the placement will leave it (kh_skin_draw_finish's box, kh_attach_skel_place's
        // centre), put in place for the tests and taken back.
        float khcp_pos[3], khcp_res[3], khcp_rat[3], khcp_sz[3], khcp_ct[3];
        memcpy(khcp_pos, o.pos, sizeof(khcp_pos));
        memcpy(khcp_res, o.pos_res, sizeof(khcp_res));
        memcpy(khcp_rat, o.pos_res_at, sizeof(khcp_rat));
        memcpy(khcp_sz, o.size, sizeof(khcp_sz));
        memcpy(khcp_ct, o.skel_ctr, sizeof(khcp_ct));
        memcpy(o.skel_ctr, j.bc, sizeof(o.skel_ctr));
        o.size[0] = j.bs[0]; o.size[1] = j.bs[2]; o.size[2] = j.bs[1];
        {
            double khcp_c[3] = { j.root[0], j.root[1], j.root[2] };
            kh_skel_centre_d(o, khcp_c);
            kh_pos_set_exact(o, khcp_c);
        }
        // Each term is the pass's geometry alone, whatever the binding's flags say (visible, casterOnly, castShadow,
        // lit...): a script's change landing between this step and a pass's read of the scene cannot let a pass
        // draw what was culled.
        bool khcp_draw = khcp_sun && (!khcp_sc_ok || kh_sun_admits(o, khcp_sc, khcp_rad));
        float khcp_ce[3] = { o.pos[0], o.pos[2], o.pos[1] };
        float khcp_he[3];
        kh_world_half_extents(o, khcp_he);
        const float khcp_hd = sqrtf(khcp_he[0] * khcp_he[0] + khcp_he[1] * khcp_he[1] + khcp_he[2] * khcp_he[2]);
        if (!khcp_draw && khcp_pn > 0u) {
            for (uint32_t khcp_i = 0; khcp_i < khcp_pn && !khcp_draw; ++khcp_i) {
                const DlPoolLight& khcp_l = g_dl.pool[khcp_i];
                {   // A caster or a reach owner.
                    const float khcp_dx = khcp_ce[0] - khcp_l.rec[0];
                    const float khcp_dy = khcp_ce[1] - khcp_l.rec[1];
                    const float khcp_dz = khcp_ce[2] - khcp_l.rec[2];
                    const float khcp_lim = khcp_far[khcp_i] + khcp_hd;
                    if (!(khcp_dx * khcp_dx + khcp_dy * khcp_dy + khcp_dz * khcp_dz > khcp_lim * khcp_lim)) {
                        khcp_draw = true;
                    }
                }
                if (!khcp_draw) {   // A view owner.
                    if (!khcp_dcam) {
                        khcp_draw = true;   // Camera-less: every visible one joins.
                    } else {
                        khcp_draw = kh_dlsw_view_covers(khcp_sc, khcp_ce, khcp_hd, khcp_vls[khcp_i]);
                    }
                }
            }
        }
        if (!khcp_draw) khcp_draw = !kh_skin_cull_behind(khcp_cam, khcp_ce, khcp_hd);
        memcpy(o.pos, khcp_pos, sizeof(khcp_pos));
        memcpy(o.pos_res, khcp_res, sizeof(khcp_res));
        memcpy(o.pos_res_at, khcp_rat, sizeof(khcp_rat));
        memcpy(o.size, khcp_sz, sizeof(khcp_sz));
        memcpy(o.skel_ctr, khcp_ct, sizeof(khcp_ct));
        if (khcp_draw) continue;
        j.culled = true;
        j.so_vb = khcp_tg.vb;
        a.skel_smp_cycle = j.prev_smp_cycle;   // Unstamped: every later step of the cycle decides again.
        a.skel_draw_seq = j.prev_draw_seq;
        if (!g_skin_cull_any) {   // render_sun_depth's range check.
            g_skin_cull_rad_lo = khcp_rad;
            g_skin_cull_rad_hi = khcp_rad;
        } else {
            g_skin_cull_rad_lo = fminf(g_skin_cull_rad_lo, khcp_rad);
            g_skin_cull_rad_hi = fmaxf(g_skin_cull_rad_hi, khcp_rad);
        }
        g_skin_cull_any = true;
    }
}
// KH_SKIN_CULL: a pass no step precedes draws what the culled bindings' boxes say, so their buffers take this
// stash's pose first, by the step's own path - the batch, the pool for what it leaves, the finish and the
// placement about the root the binding stands on. khcc_only null: every culled binding (the PIP, the park, the
// white draw, and render_sun_depth when the shadow range moved off the one the steps culled against). Else the
// flush's late-draw copies: the culled bindings among them, and the copies then take the boxes those poses carry.
// Takes g_draw_list_mutex itself (a caller must not hold it); render thread, or the park (the render thread
// stalled).
inline void kh_skin_cull_catchup(ID3D11DeviceContext* khcc_ctx, std::vector<RenderObject>* khcc_only) {
    if (!g_skin_cull_any || !khcc_ctx) return;
    static std::vector<uint8_t> khcc_mark;   // By scene slot: a skeletal copy is in khcc_only.
    if (khcc_only) {
        khcc_mark.assign(g_cloth_vb_slot.size(), 0u);
        bool khcc_any = false;
        for (const RenderObject& khcc_m : *khcc_only) {
            // KH_WSEE: an injection's deferred copy with its geometry held keeps the injection's pose and box.
            if (!khcc_m.skel || khcc_m.ws_hold != 0u || khcc_m.slot >= khcc_mark.size()) continue;
            khcc_mark[khcc_m.slot] = 1u;
            khcc_any = true;
        }
        if (!khcc_any) return;
    }
    std::lock_guard<std::mutex> khcc_g(g_draw_list_mutex);
    static std::vector<KhSkinDrawJob> khcc_jobs;
    khcc_jobs.clear();
    const bool khcc_rt = reorder_on_render_thread();
    // KH_STEP_OWN_FLAG: the buffer writes below are ours (the batch raises the flag for its draws; the finish writes a
    // buffer the batch did not take) - under the flag on the hooks' render thread they stand aside. The PIP and the
    // sun pass's range check (from the cast fire) call this with it down; the render thread's flushes run under it
    // already. Raised and restored on that thread only (kh_fx_late_gt's rule): on any other the hooks admit nothing.
    std::optional<KhInjectionFlagScope> khcc_flag;
    if (khcc_rt) {
        khcc_flag.emplace();
        g_ro.in_injection = true;
    }
    bool khcc_left = false;
    for (auto khcc_it = g_attach.begin(); khcc_it != g_attach.end(); ++khcc_it) {
        KhAttach& khcc_a = khcc_it->second;
        if (!khcc_a.skin_culled) continue;
        const auto khcc_e = g_draw_list.find(khcc_it->first);
        if (khcc_e == g_draw_list.end() || !khcc_a.skel) { khcc_a.skin_culled = false; continue; }
        RenderObject& khcc_o = khcc_e->second;
        if (khcc_only && (khcc_o.slot >= khcc_mark.size() || !khcc_mark[khcc_o.slot])) {
            khcc_left = true;   // Not among the copies: it stands culled.
            continue;
        }
        if (!khcc_a.skel_smp_ok || !kh_skin_draw_prep(khcc_a, khcc_o, khcc_ctx)) {
            khcc_left = true;   // No pose to take: it stands culled.
            continue;
        }
        // The root the binding stands on: its centre less its box's offset (kh_skel_centre_d's inverse).
        double khcc_c[3], khcc_d[3] = { 0.0, 0.0, 0.0 };
        kh_pos_exact(khcc_o, khcc_c);
        kh_skel_centre_d(khcc_o, khcc_d);
        khcc_jobs.push_back(KhSkinDrawJob{ &khcc_it->first, &khcc_a, &khcc_o,
                                           { khcc_c[0] - khcc_d[0], khcc_c[1] - khcc_d[1], khcc_c[2] - khcc_d[2] },
                                           { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, false, false, khcc_rt, KhVbRef{},
                                           false, false, khcc_a.skel_smp_cycle, khcc_a.skel_draw_seq });
        // The cull's bound, while its inputs stand (the same function of the same inputs as the batch's).
        const KhSkinInst* const khcc_in = khcc_a.skin.get();
        if (khcc_in && khcc_in->rest_draw && khcc_a.cull_rest == khcc_in->rest_draw && khcc_a.cull_a == khcc_a.draw_a) {
            KhSkinDrawJob& khcc_nj = khcc_jobs.back();
            memcpy(khcc_nj.bc, khcc_a.cull_bc, sizeof(khcc_nj.bc));
            memcpy(khcc_nj.bs, khcc_a.cull_bs, sizeof(khcc_nj.bs));
            khcc_nj.bnd = true;
        }
    }
    g_skin_cull_any = khcc_left;
    if (khcc_jobs.empty()) return;
    kh_skin_so_batch(khcc_ctx, khcc_jobs);
    uint32_t khcc_ng = 0u, khcc_nc = 0u;
    for (KhSkinDrawJob& khcc_j : khcc_jobs) {   // The pool's part, serially: the step's fork, for a rare pass.
        if (khcc_j.so_vb) { khcc_j.ok = true; ++khcc_ng; continue; }
        ++khcc_nc;
        try { kh_skin_draw_verts(*khcc_j.a, khcc_j.bc, khcc_j.bs); khcc_j.ok = true; }
        catch (...) { khcc_j.ok = false; }
    }
    if (khcc_rt) kh_skin_stat_note(khcc_ng, khcc_nc, 0u);
    for (KhSkinDrawJob& khcc_j : khcc_jobs) {   // TWIN: kh_attach_step's tails.
        if (khcc_j.ok) {
            kh_skin_draw_finish(*khcc_j.h, *khcc_j.a, *khcc_j.o, khcc_ctx, khcc_j.bc, khcc_j.bs, khcc_j.moved,
                                khcc_j.so_vb);
            khcc_j.a->skin_culled = false;
            if (khcc_rt) {   // This sample is skinned: the cycle's later steps keep it, as after a step's skin.
                khcc_j.a->skel_draw_seq = khcc_j.a->skel_smp_seq;
                khcc_j.a->skel_smp_cycle = g_topo_cycles;
            }
        } else {
            g_skin_cull_any = true;
        }
        kh_attach_skel_place(*khcc_j.o, khcc_j.root, khcc_j.moved);
        kh_attach_moved_note(*khcc_j.o, khcc_j.moved, khcc_j.rt);
        if (!khcc_only || !khcc_j.ok) continue;
        for (RenderObject& khcc_m : *khcc_only) {   // The copies draw the box this pose carries.
            if (!khcc_m.skel || khcc_m.ws_hold != 0u || khcc_m.slot != khcc_j.o->slot) continue;   // KH_WSEE: held.
            memcpy(khcc_m.pos, khcc_j.o->pos, sizeof(khcc_m.pos));
            memcpy(khcc_m.pos_res, khcc_j.o->pos_res, sizeof(khcc_m.pos_res));
            memcpy(khcc_m.pos_res_at, khcc_j.o->pos_res_at, sizeof(khcc_m.pos_res_at));
            memcpy(khcc_m.size, khcc_j.o->size, sizeof(khcc_m.size));
            memcpy(khcc_m.skel_ctr, khcc_j.o->skel_ctr, sizeof(khcc_m.skel_ctr));
        }
    }
}
}