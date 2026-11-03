#pragma once

// The feature half (rendering_integration_features.hpp): the cloth frame: kh_cloth_sync, the pins, the upload and the
// releases. Never include it on its own.
namespace RenderIntegration {

inline bool kh_uvs_draws_from(uint32_t khud_slot, const ID3D11Buffer* khud_src);   // KH_USER_VS: the slot draws a vertex stage's buffer deformed from this one.
inline void kh_uvs_src_wrote(uint32_t khsw_slot);   // KH_USER_VS: a slot's own per-instance vertex buffer was just written (every writer of one calls it).
// KH_SKEL: a skeletal binding's pose for its cloth (defined with the skin runtime; read by kh_cloth_sync and
// kh_cloth_pin_fresh).
inline bool kh_skin_guide_of(uint64_t khgo_seq, int khgo_mesh, std::vector<float>& khgo_out);

inline float effect_time_seconds();   // Defined in rendering/effect_clock.hpp (declared here for the use below).

// Defined in rendering/skinning.hpp (declared here for the uses below).
inline void kh_skin_col_take(bool khst_snap);
inline bool kh_skin_step_maps(KhAttach& khsd_a, const KhSkinInst* khsd_in, const RenderObject& khsd_o);


// The frame's cloth work, from the game thread, BEFORE the park: creates and retires instances, publishes each
// one's inputs and queues it. Nothing here touches the device. khcs_wanted is flush_frame's census verdict for this
// frame (some object has clothSimulation or chainSimulation on); with it false and no instance standing this is the
// inert path.
inline void kh_cloth_sync(float khcs_dt, bool khcs_wanted) {
    if (!khcs_wanted && g_cloth.empty()) {
        // A clear of an already-empty vector: the inert path costs nothing.
        g_physics_col.clear();
        g_physics_col_n.store(0, std::memory_order_relaxed);
        kh_skin_col_take(false);   // The freshness window moves every frame.
        // KH_AFF_REAP_IDLE: kh_aff_frame_build's reap, with nothing to build - otherwise an expired affector goes
        // only when an affector command comes, and physicsAffectors counts it meanwhile.
        if (!g_affectors.empty()) kh_aff_reap(effect_time_seconds_d());
        return;
    }
    // The simulation's consume point: objects are read right before the transforms go to the workers (a cloth
    // stepped against last frame's transform lags its mesh). From g_draw_list under its mutex, not g_scene: the
    // render thread may be walking g_scene lock-free, and g_draw_list is where kh_attach_step writes.
    g_cloth_objs.clear();
    g_cloth_objs_h.clear();
    {
        std::lock_guard<std::mutex> khcs_l(g_draw_list_mutex);
        for (const auto& khcs_kv : g_draw_list) {
            if (kh_cloth_obj_sim(khcs_kv.second) || kh_physics_obj_collider(khcs_kv.second) ||
                kh_chain_obj_sim(khcs_kv.second)) {   // KH_CHAIN: the chains ride the cloth's plumbing.
                g_cloth_objs.push_back(khcs_kv.second);
                g_cloth_objs_h.push_back(khcs_kv.first);
            }
        }
        kh_skin_col_take(true);   // The collider table as these boxes pair with it.
    }
    const std::vector<RenderObject>& khcs_objs = g_cloth_objs;
    kh_cloth_workers_start();
    kh_physics_gather_colliders(khcs_objs, g_physics_col);
    g_physics_col_n.store(g_physics_col.size(), std::memory_order_relaxed);

    // Retire instances whose object (by creation stamp, not merely the same slot and mesh) is gone or has turned
    // cloth off. The buffer goes to the grave, not released here: the render thread may still be inside a pass
    // holding this slot's pointer. One a worker owns is marked retired and goes the first frame it is free; a
    // retired instance never revives, so an object that turned cloth off and on while its worker ran gets a fresh one.
    {
        std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
        for (auto khcs_it = g_cloth.begin(); khcs_it != g_cloth.end(); ) {
            KhClothInst& khcs_ri = *khcs_it->second;
            bool khcs_live = false;
            for (size_t khcs_i = 0; khcs_i < khcs_objs.size() && !khcs_ri.retired; ++khcs_i) {
                const RenderObject& khcs_o = khcs_objs[khcs_i];
                // KH_CHAIN: a chain lives on as a chain and a cloth as a cloth.
                const bool khcs_as = khcs_ri.chain ? kh_chain_obj_sim(khcs_o) : kh_cloth_obj_sim(khcs_o);
                if (khcs_o.slot == khcs_it->first && khcs_as &&
                    mesh_id_clamp(khcs_o.mesh) == khcs_ri.mesh && khcs_o.seq == khcs_ri.seq) { khcs_live = true; break; }
            }
            if (khcs_live) { ++khcs_it; continue; }
            if (khcs_ri.busy.load(std::memory_order_acquire)) { khcs_ri.retired = true; ++khcs_it; continue; }
            if (khcs_ri.vb) { g_cloth_grave.push_back(khcs_ri.vb); khcs_ri.vb = nullptr; }
            // KH_CLOTH_ERASE_RETIRE: kh_cloth_upload walks a snapshot of the table taken under an earlier hold of this
            // lock; an instance erased in between must read as retired there, or the walk (seeing no vb) makes it a
            // buffer nothing releases and draws the dead pose in the slot for the frame.
            khcs_ri.retired = true;
            khcs_it = g_cloth.erase(khcs_it);
        }
    }

    kh_aff_frame_build(effect_time_seconds_d());   // KH_AFFECTOR.
    // KH_SIM_LOD: the boundary view once per frame; stale = unknown.
    float khcs_lcam[3] = { 0.0f, 0.0f, 0.0f }, khcs_lm11 = 0.0f, khcs_lt = 0.0f;
    const bool khcs_lview = kh_simlod_read(khcs_lcam, khcs_lm11, khcs_lt) &&
                            effect_time_seconds() - khcs_lt <= KH_SIMLOD_STALE_S;
    uint64_t khcs_lwork = 0, khcs_lfull = 0;
    const uint32_t khcs_pub = mesh_count();   // KH_EA_FBX_NO_PARK (below).
    for (size_t khcs_i = 0; khcs_i < khcs_objs.size(); ++khcs_i) {
        const RenderObject& khcs_o = khcs_objs[khcs_i];
        // KH_CHAIN: a chain takes this loop too, with its own build, settings (by handle, KhChainCfg) and refusals;
        // from the busy test down the plumbing is shared.
        const bool khcs_ch = kh_chain_obj_sim(khcs_o);
        if (!kh_cloth_obj_sim(khcs_o) && !khcs_ch) continue;
        const KhChainCfg* khcs_cfg = nullptr;
        if (khcs_ch) {
            const auto khcs_ci = g_chain_cfg.find(g_cloth_objs_h[khcs_i]);
            if (khcs_ci != g_chain_cfg.end() && khcs_ci->second.seq == khcs_o.seq) khcs_cfg = &khcs_ci->second;
        }
        if (khcs_o.slot == 0xFFFFFFFFu) continue;
        // KH_EA_FBX_NO_PARK: a mesh appended but not yet published (an import, a generated mesh - until the next
        // injection or flush lands it) is no verdict: clamped to the box it would be refused and reported as 'box'.
        // The object waits, as kh_skin_sync and the collider gather wait on kh_mesh_alive; the retire walk above
        // already ended an instance of its old mesh, as it would for any new one. A released id (below the count)
        // keeps the path below.
        if (static_cast<uint32_t>(khcs_o.mesh) >= khcs_pub) continue;
        const int khcs_mid = mesh_id_clamp(khcs_o.mesh);
        // KH_CHAIN: a refusal is counted and named once per mesh and reason.
        auto khcs_refuse = [&](const std::string& khcs_why) {
            const std::string khcs_key = std::to_string(khcs_mid) + "|" + khcs_why;
            for (const std::string& khcs_w : g_chain_warned) if (khcs_w == khcs_key) return;
            if (g_chain_warned.size() >= 64u) return;
            g_chain_warned.push_back(khcs_key);
            report_error_once_safe("KH RenderIntegration: chain: mesh '" + mesh_def(khcs_mid).name + "' " + khcs_why +
                                   ", so it cannot simulate as a chain");
        };
        const std::string khcs_start = khcs_cfg ? khcs_cfg->start : std::string();
        const std::string khcs_end = khcs_cfg ? khcs_cfg->end : std::string();
        const bool khcs_shead = khcs_cfg ? khcs_cfg->start_head : false;   // KH_CHAIN_POINT.
        const bool khcs_etail = khcs_cfg ? khcs_cfg->end_tail : false;
        const KhChainParams khcs_cp = khcs_cfg ? khcs_cfg->par : kh_chain_defaults();
        if (khcs_ch && !kh_chain_mesh_ok(khcs_mid)) {
            khcs_refuse("carries no skeleton (import a model whose bones skin its vertices)");
            continue;
        }
        // KH_CHAIN_REFUSED_SKIN: the one key rule (kh_skin_sync asks it too).
        const std::string khcs_nbk = khcs_ch ? kh_chain_nobuild_key(khcs_o, khcs_cfg) : std::string();
        // KH_CHAIN_REST_BACK: a refused plain chain draws its own buffer at its authored size. The upload's put can
        // have handed the object the old chain's box after the command staged that size, and with no instance
        // nothing puts again. g_draw_list_mutex with no g_cloth_mu held (list -> cloth); a skeletal box is the skin's.
        auto khcs_rest_back = [&]() {
            if (khcs_o.skel) return;
            float khcs_ns[3];
            memcpy(khcs_ns, mesh_def(khcs_mid).native_size, sizeof(khcs_ns));
            std::lock_guard<std::mutex> khcs_dl(g_draw_list_mutex);
            const auto khcs_de = g_draw_list.find(g_cloth_objs_h[khcs_i]);
            if (khcs_de == g_draw_list.end()) return;
            RenderObject& khcs_lo = khcs_de->second;
            if (khcs_lo.seq != khcs_o.seq || khcs_lo.mesh != khcs_o.mesh || !khcs_lo.chain_sim || khcs_lo.skel ||
                khcs_lo.slot == 0xFFFFFFFFu) return;
            if (memcmp(khcs_lo.size, khcs_ns, sizeof(khcs_ns)) == 0) return;
            memcpy(khcs_lo.size, khcs_ns, sizeof(khcs_ns));
            kh_scene_mark(khcs_lo.slot);
        };
        if (khcs_ch && std::find(g_chain_nobuild.begin(), g_chain_nobuild.end(), khcs_nbk) != g_chain_nobuild.end()) {
            // KH_CHAIN_REFUSED_LIVE: a refused key can come back while the object holds a chain built from other
            // names (built with X, refused Y, rebuilt X, then Y again); the rebuild below is never reached, so that
            // instance would stay - never stepped, still drawn, frozen. It goes as the rebuild's refusal sends it
            // (marked while a worker owns it, erased by the sweep once free), and the mesh's own buffer draws.
            // The common case (no instance) is told without g_cloth_mu: only this thread adds or erases g_cloth
            // entries (kh_cloth_sync; kh_cloth_drop_all and the session reset, under the park), so a lookup here
            // races no structural change; the render thread's readers only read.
            bool khcs_live = false;
            if (g_cloth.find(khcs_o.slot) != g_cloth.end()) {
                std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
                const auto khcs_it = g_cloth.find(khcs_o.slot);
                if (khcs_it != g_cloth.end() && !khcs_it->second->retired) {
                    KhClothInst& khcs_ri = *khcs_it->second;
                    if (khcs_ri.chain && khcs_ri.seq == khcs_o.seq && khcs_ri.mesh == khcs_mid) {
                        khcs_ri.retired = true;   // KH_CLOTH_ERASE_RETIRE (the rebuild's refusal).
                        if (!khcs_ri.busy.load(std::memory_order_acquire)) {
                            if (khcs_ri.vb) { g_cloth_grave.push_back(khcs_ri.vb); khcs_ri.vb = nullptr; }
                            g_cloth.erase(khcs_it);
                        }
                    } else {
                        khcs_live = true;   // Not this object's chain: not this refusal's to end.
                    }
                }
            }
            // KH_CHAIN_REST_BACK: a put the render thread built before the refusal's erase may land after its restore;
            // healed here while no live instance puts for this slot.
            if (!khcs_live && !khcs_o.skel &&
                memcmp(mesh_def(khcs_mid).native_size, khcs_o.size, sizeof(khcs_o.size)) != 0) {
                khcs_rest_back();
            }
            continue;
        }
        auto khcs_nobuild = [&](const std::string& khcs_why) {
            khcs_refuse(khcs_why);
            // KH_CHAIN_NOBUILD_PRUNE: always filed; a full list first drops the keys no live chain object has as its
            // current key. Both callers hold no lock here.
            if (g_chain_nobuild.size() >= 256u) kh_chain_nobuild_prune();
            g_chain_nobuild.push_back(khcs_nbk);
        };
        if (!khcs_ch && !kh_cloth_mesh_ok(khcs_mid)) {
            // Named once per mesh: the counter says that it happened, the modeller needs to know which model.
            bool khcs_seen = false;
            for (size_t khcs_w = 0; khcs_w < g_cloth_warned.size(); ++khcs_w) {
                if (g_cloth_warned[khcs_w] == khcs_mid) { khcs_seen = true; break; }
            }
            if (!khcs_seen && g_cloth_warned.size() < 64u) {
                g_cloth_warned.push_back(khcs_mid);
                report_error_once_safe("KH RenderIntegration: cloth: mesh '" + mesh_def(khcs_mid).name +
                                       "' carries no kh_cloth_sim / kh_cloth_pin weights - "
                                       "no vertex group reached the FBX as a skin cluster and no colour "
                                       "set of either name was found, so it cannot simulate");
            }
            continue;
        }

        std::shared_ptr<KhClothInst> khcs_in;
        std::string khcs_fail;   // KH_CHAIN: a refused build, reported once the lock is let go.
        {
            std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
            const auto khcs_it = g_cloth.find(khcs_o.slot);
            if (khcs_it != g_cloth.end()) {
                // A retired one is another object's, still on its worker; this object gets its own once the slot's
                // entry is free.
                if (khcs_it->second->retired) continue;
                khcs_in = khcs_it->second;
            }
        }
        if (!khcs_in) {
            // KH_CLOTH_BUILD_UNLOCKED: a new instance is built before g_cloth_mu is taken - nothing else can see it
            // until the emplace (this thread is the table's only inserter, so the slot is still free at the emplace),
            // and under the lock the build (a cloth's weld and constraints; a chain's bone walk and bodies) would hold
            // the render thread's kh_cloth_upload / pin step / chain-end draw for its whole length. The in-place chain
            // rebuild below keeps the lock: that instance is live.
            std::shared_ptr<KhClothInst> khcs_nw = std::make_shared<KhClothInst>();
            khcs_nw->par = khcs_o.cloth;
            khcs_nw->seq = khcs_o.seq;
            if (khcs_ch) {
                if (!kh_chain_build(*khcs_nw, mesh_def(khcs_mid), khcs_o.slot, khcs_mid, khcs_start, khcs_end,
                                    khcs_shead, khcs_etail, khcs_cp.mass, khcs_fail)) {
                    khcs_nobuild(khcs_fail);
                    continue;
                }
                khcs_nw->cpar = khcs_cp;
            } else if (!kh_cloth_build(*khcs_nw, mesh_def(khcs_mid), khcs_o.slot, khcs_mid)) {
                continue;
            }
            std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
            g_cloth.emplace(khcs_o.slot, khcs_nw);
            khcs_in = std::move(khcs_nw);
        }
        // A busy instance is a worker's: its inputs may not be touched and it may not be queued twice. The missed
        // frame is a frame of stale cloth, which g_cloth_skipped / g_chain_skipped count (stats armed only), and its
        // time is owed to the next step (dt_owed, capped at KH_CLOTH_DT_MAX), which then spans the interval its
        // carrier and colliders moved across, up to that cap. Do not drop the owed time: the cloth falls behind real
        // time and reads two frames' travel as one frame's velocity.
        if (khcs_in->busy.load(std::memory_order_acquire)) {
            if (kh_stats_on()) (khcs_ch ? g_chain_skipped : g_cloth_skipped).fetch_add(1u, std::memory_order_relaxed);
            khcs_in->dt_owed += khcs_dt;
            if (khcs_in->dt_owed > KH_CLOTH_DT_MAX) khcs_in->dt_owed = KH_CLOTH_DT_MAX;
            continue;
        }
        // KH_CHAIN: new bone names are a new chain, rebuilt here (the instance is the game thread's) on the same
        // buffers (same mesh).
        if (khcs_ch && (khcs_in->ch_start != khcs_start || khcs_in->ch_end != khcs_end ||
                        khcs_in->ch_start_head != khcs_shead || khcs_in->ch_end_tail != khcs_etail)) {
            std::string khcs_why;
            bool khcs_rebuilt = false;
            {   // KH_CLOTH_INST: a live instance, whose rest the render-thread upload reads. A refusal has already
                // reset its chain and state (kh_chain_build), so it leaves the table in this same hold.
                std::lock_guard<std::mutex> khcs_rb(g_cloth_mu);
                khcs_rebuilt = kh_chain_build(*khcs_in, mesh_def(khcs_mid), khcs_o.slot, khcs_mid, khcs_start, khcs_end,
                                              khcs_shead, khcs_etail, khcs_cp.mass, khcs_why);
                if (!khcs_rebuilt) {   // Refused: the instance goes, and the mesh's own buffer draws.
                    const auto khcs_it = g_cloth.find(khcs_o.slot);
                    if (khcs_it != g_cloth.end() && khcs_it->second == khcs_in) {
                        if (khcs_in->vb) { g_cloth_grave.push_back(khcs_in->vb); khcs_in->vb = nullptr; }
                        khcs_in->retired = true;   // KH_CLOTH_ERASE_RETIRE (above).
                        g_cloth.erase(khcs_it);
                    }
                }
            }
            if (!khcs_rebuilt) {
                khcs_nobuild(khcs_why);
                khcs_rest_back();   // KH_CHAIN_REST_BACK (above).
                continue;
            }
        }
        // The interval this step covers.
        const float khcs_dt_now = khcs_dt + khcs_in->dt_owed;
        khcs_in->dt_owed = 0.0f;

        float khcs_size[3];
        kh_cloth_engine_size(khcs_o, khcs_size);
        // KH_SKEL: a skeletal binding steps in its REST box, whatever box its drawn shape has given the object
        // (kh_cloth_box_out): the metric rest lengths are that box's, and it never resizes.
        const MeshDef& khcs_md = mesh_def(khcs_mid);
        // KH_CHAIN: a chain steps in its rest box whatever it is bound to - drawn at its authored size, its box is
        // its shape's.
        if (khcs_o.skel || khcs_ch) {
            khcs_size[0] = khcs_md.native_size[0];
            khcs_size[1] = khcs_md.native_size[2];
            khcs_size[2] = khcs_md.native_size[1];
        }
        for (int k = 0; k < 3; ++k) if (fabsf(khcs_size[k]) < 1.0e-6f) khcs_size[k] = 1.0e-6f;
        const float khcs_tele = khcs_ch ? khcs_cp.teleport : khcs_o.cloth.teleport;
        const float khcs_range = khcs_ch ? khcs_cp.range : khcs_o.cloth.range;
        if (khcs_ch) {
            // KH_CHAIN: 'mass' is shared into the bodies at the build, so a new one rescales them here, as the
            // cloth's does its particles.
            if (khcs_in->ch.mass_built != khcs_cp.mass && khcs_in->ch.mass_built > 0.0f && khcs_cp.mass > 0.0f) {
                const float khcs_r = khcs_in->ch.mass_built / khcs_cp.mass;
                // KH_CLOTH_MASS_LOCK: inv_m is read on the render thread under g_cloth_mu (kh_cloth_pin_place,
                // kh_chain_end_fix), as in_size is (KH_CLOTH_INST).
                std::lock_guard<std::mutex> khcs_ml(g_cloth_mu);
                for (KhChainBody& khcs_b : khcs_in->ch.body) {
                    if (!(khcs_b.inv_m > 0.0f)) continue;
                    khcs_b.inv_m *= khcs_r;
                    for (int k = 0; k < 9; ++k) { khcs_b.ib_inv[k] *= khcs_r; khcs_b.ib[k] /= khcs_r; }
                }
                khcs_in->ch.mass_built = khcs_cp.mass;
            }
            khcs_in->cpar = khcs_cp;
        } else {
        // Rest lengths are metric, so a resized mesh needs them rebuilt, and the tethers (rest-length sums) with
        // them.
        bool khcs_resize = !khcs_in->built || khcs_in->st.stretch.empty();
        // KH_SUB_CLOCK: against the size the last step was given (st.size stands between frames).
        for (int k = 0; k < 3; ++k) if (fabsf(khcs_size[k] - khcs_in->st.fend.size[k]) > 1.0e-5f) khcs_resize = true;
        // The shell links are metric too: the dial moving is a rebuild.
        if (khcs_in->shell_built != khcs_o.cloth.shell) khcs_resize = true;
        // 'mass' is the one parameter the build bakes into the particles rather than the step reading it from par
        // (only the media drag, KH_CLOTH_DRAG, reads par.mass live), so a change must reach the inverse masses here.
        if (khcs_in->par.mass != khcs_o.cloth.mass) {
            const float khcs_w = kh_cloth_inv_mass(khcs_o.cloth.mass);
            // KH_CLOTH_MASS_LOCK: w is read on the render thread under g_cloth_mu (kh_cloth_pin_place).
            std::lock_guard<std::mutex> khcs_ml(g_cloth_mu);
            for (size_t khcs_p = 0; khcs_p < khcs_in->st.part.size(); ++khcs_p) {
                if (khcs_in->st.part[khcs_p].w > 0.0f) khcs_in->st.part[khcs_p].w = khcs_w;
            }
        }
        khcs_in->par = khcs_o.cloth;
        if (khcs_resize) kh_cloth_rest_lengths(*khcs_in, khcs_size);
        }

        // The carrier in the world, then in the instance's own frame. A fresh instance takes its origin where it
        // stands; the first step places it from rest whatever the origin was.
        // KH_POS_RES: the same centre exact (the object's residual and the offset in double) for the frame the
        // write-back makes the vertices relative to (the draw adds the residual back); khcs_wc, a world float, serves
        // the broadphase, the ground patch and the simulation level.
        float khcs_wc[3];
        double khcs_wd[3];
        kh_cloth_carrier_at(khcs_o, khcs_md.native_ctr, khcs_wc, khcs_wd);
        {
            double khcs_od[3];
            for (int k = 0; k < 3; ++k) khcs_od[k] = khcs_wd[k] - khcs_in->org[k];
            if (!khcs_in->org_on) {
                for (int k = 0; k < 3; ++k) khcs_in->org[k] = khcs_wd[k];
                khcs_in->org_on = true;
            } else if (khcs_od[0] * khcs_od[0] + khcs_od[1] * khcs_od[1] + khcs_od[2] * khcs_od[2] >
                       KH_CLOTH_ORIGIN_M * KH_CLOTH_ORIGIN_M) {
                kh_cloth_origin_shift(*khcs_in, khcs_od);
            }
        }
        for (int k = 0; k < 3; ++k) {
            khcs_in->in_centre[k] = static_cast<float>(khcs_wd[k] - khcs_in->org[k]);
        }
        khcs_in->st.sea = static_cast<float>(-khcs_in->org[KH_CLOTH_UP]);
        memcpy(khcs_in->in_rot, khcs_o.rot_m, sizeof(khcs_in->in_rot));
        khcs_in->in_rotated = khcs_o.rotated;
        khcs_in->in_dt = khcs_dt_now;
        memcpy(khcs_in->in_wind, g_cloth_wind, sizeof(khcs_in->in_wind));
        // KH_SKEL: the box rule, and this frame's pose from kh_skin_sync, which ran just ahead of this function.
        {   // KH_CLOTH_INST: the upload's chain box put reads it; in_size too (kh_cloth_pin_place, render thread).
            std::lock_guard<std::mutex> khcs_bx(g_cloth_mu);
            khcs_in->in_boxed = khcs_o.skel;
            memcpy(khcs_in->in_size, khcs_size, sizeof(khcs_in->in_size));
        }
        if (!khcs_o.skel || !kh_skin_guide_of(khcs_o.seq, khcs_mid, khcs_in->in_guide)) khcs_in->in_guide.clear();
        if (khcs_ch) kh_chain_target_of(khcs_cfg, khcs_in->org, khcs_in->in_tg);   // KH_CHAIN: the end, this frame.

        // The colliders in reach. The cloth's bound is the larger of its rest extent and how far it reached at its
        // last step (a cloth swung or streamed outside its rest box keeps the colliders it swings into), plus the
        // script's range for the motion since.
        float khcs_reach = 0.0f;
        for (int k = 0; k < 3; ++k) khcs_reach += 0.25f * khcs_size[k] * khcs_size[k];
        khcs_reach = sqrtf(khcs_reach);
        if (khcs_in->st.reach > khcs_reach) khcs_reach = khcs_in->st.reach;
        khcs_reach += khcs_range > 0.0f ? khcs_range : 1.0f;
        {   // KH_SIM_LOD: this step's counts. The sphere is the larger of the rest box's half-diagonal and how far the
            // cloth reached, about the carrier's centre (world, engine axes).
            float khcs_lr2 = 0.0f;
            for (int k = 0; k < 3; ++k) khcs_lr2 += 0.25f * khcs_size[k] * khcs_size[k];
            float khcs_lr = sqrtf(khcs_lr2);
            if (khcs_in->st.reach > khcs_lr) khcs_lr = khcs_in->st.reach;
            const float khcs_lq = kh_simlod_share(khcs_lview, khcs_lcam, khcs_lm11, khcs_wc, khcs_lr, khcs_o.sim_lod, khcs_o.sim_lod_on);
            const float khcs_lqs = khcs_lq * 2.0f < 1.0f ? khcs_lq * 2.0f : 1.0f;   // Substeps give only below half.
            const uint16_t khcs_dit = khcs_ch ? khcs_in->cpar.iterations : khcs_in->par.iterations;
            const uint16_t khcs_dsb = khcs_ch ? khcs_in->cpar.substeps : khcs_in->par.substeps;
            khcs_in->in_iter = kh_simlod_count(khcs_dit, khcs_lq, khcs_in->in_iter);
            khcs_in->in_sub = kh_simlod_count(khcs_dsb, khcs_lqs, khcs_in->in_sub);
            // KH_CHAIN_LOD_FLOOR: the work a chain's step actually runs (kh_cloth_run raises it to the floor).
            uint16_t khcs_wit = khcs_in->in_iter, khcs_wsb = khcs_in->in_sub;
            if (khcs_ch) {
                khcs_wit = kh_chain_lod_floor(khcs_dit, khcs_wit, KH_CHAIN_LOD_ITER);
                khcs_wsb = kh_chain_lod_floor(khcs_dsb, khcs_wsb, KH_CHAIN_LOD_SUB);
            }
            khcs_lwork += static_cast<uint64_t>(khcs_wit) * khcs_wsb;
            khcs_lfull += static_cast<uint64_t>(khcs_dit < 1u ? 1u : khcs_dit) * (khcs_dsb < 1u ? 1u : khcs_dsb);
        }
        kh_cloth_ground_patch(*khcs_in, khcs_wc, khcs_reach, khcs_tele);   // KH_CLOTH_GROUND.
        // Each collider's pose at THIS cloth's last step, matched by slot and mesh, becomes its prev, so the step
        // sweeps it over exactly the interval it sweeps the cloth's own frame - across a skipped frame too.
        // KH_SUB_CLOCK: its pose where the simulation stands, st.sub_t of the way along what that step swept it over
        // (that step's own pose when nothing was left over), so the sweep runs on from there. A collider not seen
        // before, or one that jumped by the cloth's teleport test, is taken as still. The test predicts from the
        // collider's velocity at that step, as the carrier's does (kh_cloth_step), so a fast collider is swept.
        std::vector<KhPhysicsColliderView> khcs_seen;
        khcs_seen.swap(khcs_in->in_col);
        khcs_in->in_col.clear();
        khcs_in->in_col_own.clear();
        for (size_t khcs_c = 0; khcs_c < g_physics_col.size(); ++khcs_c) {
            const KhPhysicsColliderView& khcs_v = g_physics_col[khcs_c].view;
            // Never the object's own view: its rest triangles stand exactly where its particles or contact points
            // start, so each would be pushed off its own surface.
            if (khcs_v.slot == khcs_o.slot) continue;
            if (!khcs_v.inside) {
                float khcs_d[3];
                kh_cloth_v3_sub(khcs_v.bs_c, khcs_wc, khcs_d);   // Both world: the broadphase.
                const float khcs_rr = khcs_v.bs_r + khcs_reach;
                if (kh_cloth_v3_dot(khcs_d, khcs_d) > khcs_rr * khcs_rr) continue;
            }
            khcs_in->in_col.push_back(khcs_v);
            KhPhysicsColliderView& khcs_nv = khcs_in->in_col.back();
            // Into this instance's frame, in double: the gather's views are world (KH_POS_RES: plus the residual, so
            // the collider stands where it is drawn).
            for (int k = 0; k < 3; ++k) {
                const double khcs_r = static_cast<double>(khcs_v.res[k]);
                khcs_nv.centre[k] =
                    static_cast<float>(static_cast<double>(khcs_v.centre[k]) - khcs_in->org[k] + khcs_r);
                khcs_nv.bs_c[k] = static_cast<float>(static_cast<double>(khcs_v.bs_c[k]) - khcs_in->org[k] + khcs_r);
                khcs_nv.prev.centre[k] =
                    static_cast<float>(static_cast<double>(khcs_v.prev.centre[k]) - khcs_in->org[k] + khcs_r);
                khcs_nv.res[k] = 0.0f;
            }
            for (size_t khcs_s = 0; khcs_s < khcs_seen.size(); ++khcs_s) {
                const KhPhysicsColliderView& khcs_sv = khcs_seen[khcs_s];
                if (khcs_sv.slot != khcs_nv.slot || khcs_sv.mesh != khcs_nv.mesh) continue;
                KhClothFrame khcs_pf;
                memcpy(khcs_pf.centre, khcs_sv.centre, sizeof(khcs_pf.centre));
                memcpy(khcs_pf.rot, khcs_sv.rot, sizeof(khcs_pf.rot));
                memcpy(khcs_pf.size, khcs_sv.size, sizeof(khcs_pf.size));
                khcs_pf.rotated = khcs_sv.rotated;
                // Where its velocity at the last step would have carried it.
                KhClothFrame khcs_pp = khcs_pf;
                for (int k = 0; k < 3; ++k) khcs_pp.centre[k] += khcs_sv.vel[k] * khcs_dt_now;
                const bool khcs_jumped = kh_cloth_frame_jumped(khcs_pp, khcs_nv.prev, khcs_tele);
                // Its velocity now, jumped or not (as the carrier's): a collider past the teleport distance per
                // frame would otherwise never have one and be taken as still for good. A real jump's velocity makes
                // the next frame miss once more, then holds still.
                if (khcs_dt_now > 0.0f) {
                    for (int k = 0; k < 3; ++k) khcs_nv.vel[k] = (khcs_nv.centre[k] - khcs_pf.centre[k]) / khcs_dt_now;
                }
                if (!khcs_jumped) {
                    kh_cloth_frame_lerp(khcs_sv.prev, khcs_pf, khcs_in->st.sub_t, khcs_nv.prev);   // KH_SUB_CLOCK.
                    if (khcs_nv.skinned) {
                        // KH_SKEL: the BVH is this frame's pose alone, so the sweep across the substeps is the
                        // ROOT's motion carrying it: last frame's root (its centre less its own box centre) with
                        // this frame's box centre and size. Do not interpolate the boxes themselves: that moves and
                        // scales the whole body by however the pose's box changed. KH_SKIN_COL_PREV: last frame's
                        // centre less ITS offset from the root - on the first skinned frame after the rest mesh
                        // stood in, that is the rest box's centre, not zero (skin_ctr is zero there).
                        float khcs_bd[3];
                        for (int k = 0; k < 3; ++k) khcs_bd[k] = khcs_nv.root_off[k] - khcs_sv.root_off[k];
                        for (int k = 0; k < 3; ++k) {
                            khcs_nv.prev.centre[k] += khcs_pf.rotated
                                ? khcs_bd[0] * khcs_pf.rot[k] + khcs_bd[1] * khcs_pf.rot[3 + k] + khcs_bd[2] * khcs_pf.rot[6 + k]
                                : khcs_bd[k];
                        }
                        memcpy(khcs_nv.prev.size, khcs_nv.size, sizeof(khcs_nv.prev.size));
                    }
                }
                break;
            }
            khcs_in->in_col_own.push_back(g_physics_col[khcs_c].own);
        }

        // KH_AFFECTOR: the affectors in reach, into this instance's frame in double like the colliders. The reach
        // test is the colliders' (the rest bound plus 'range'); an unbounded affector always reaches.
        khcs_in->in_aff.clear();
        for (size_t khcs_a = 0; khcs_a < g_aff_frame.size(); ++khcs_a) {
            const KhAffFrame& khcs_af = g_aff_frame[khcs_a];
            if (khcs_af.reach >= 0.0f) {
                double khcs_dd = 0.0;
                for (int k = 0; k < 3; ++k) {
                    const double khcs_e = khcs_af.c[k] - static_cast<double>(khcs_wc[k]);
                    khcs_dd += khcs_e * khcs_e;
                }
                const double khcs_rr = static_cast<double>(khcs_af.reach) + static_cast<double>(khcs_reach);
                if (khcs_dd > khcs_rr * khcs_rr) continue;
            }
            khcs_in->in_aff.push_back(khcs_af.v);
            KhClothAffView& khcs_av = khcs_in->in_aff.back();
            for (int k = 0; k < 3; ++k) khcs_av.c[k] = static_cast<float>(khcs_af.c[k] - khcs_in->org[k]);
        }

        {
            std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
            g_cloth_q.push_back(khcs_in);
            // KH_POOL_BUSY_AFTER_PUSH: busy only once the job is queued - set before a push that throws (out of
            // memory), nothing clears it and the instance never steps (nor is erased) again. Under the lock every pop
            // takes, so no worker holds the job before the flag is set.
            khcs_in->busy.store(true, std::memory_order_release);
            g_cloth_q_n.store(static_cast<uint32_t>(g_cloth_q.size()), std::memory_order_relaxed);
        }
        g_cloth_cv.notify_one();
    }
    g_simlod_work.store(khcs_lwork, std::memory_order_relaxed);   // KH_SIM_LOD.
    g_simlod_full.store(khcs_lfull, std::memory_order_relaxed);
}
// KH_SESSION_RESET: outside the function so the session reset reaches it.
static double g_cloth_dt_last = 0.0;
// The frame delta kh_cloth_sync steps on, from the effect clock (game thread, flush_frame): its contract - steady wall
// time, the clock's age on a session's first frame, 0 after a gap over 10 s - is the note above effect_time_seconds_d's
// declaration in rendering/physics_affectors.hpp.
inline float kh_cloth_frame_dt() {
    const double khfd_now = effect_time_seconds_d();
    const double khfd_d = khfd_now - g_cloth_dt_last;
    g_cloth_dt_last = khfd_now;
    if (!(khfd_d > 0.0) || khfd_d > 10.0) return 0.0f;
    return static_cast<float>(khfd_d);
}

// Called by the upload once its walk is done (before its box puts), so a reader sees a whole frame's counters, never a
// half-updated set.
inline void kh_cloth_stats_publish(uint64_t khsp_inst, uint64_t khsp_ch_inst, uint64_t khsp_cols) {
    g_stats.cloth_instances = khsp_inst;
    g_stats.cloth_skipped = g_cloth_skipped.load(std::memory_order_relaxed);
    g_stats.physics_colliders = khsp_cols;
    g_stats.chain_instances = khsp_ch_inst;
    g_stats.chain_skipped = g_chain_skipped.load(std::memory_order_relaxed);
    g_stats.simlod_work = g_simlod_work.load(std::memory_order_relaxed);   // KH_SIM_LOD.
    g_stats.simlod_full = g_simlod_full.load(std::memory_order_relaxed);
}

// KH_CLOTH_PIN_FRESH: the front buffer with the guide-bound part re-placed from a guide (khpf_g, khpf_nb maps in
// kh_skin_affine_of's layout) into pin_out, in the front's frame - the cloth's hard pins (w == 0) exactly, its
// pin/sim gradient in proportion (KH_CLOTH_PIN_GRADE, KhClothInst::out_guide), or a chain mesh re-skinned
// (positions, normals and tangents) with fresh kinematic and guide-mapped bone maps and the job's dynamic ones.
// False when there is nothing to re-place (the front uploads as is). Under g_cloth_mu, from kh_cloth_upload and
// kh_cloth_pin_draw_step; reads only what the job does not write past the build (a particle's w and att among it),
// plus out / box_* / ch_mat / out_guide at front. kh_cloth_pin_fresh feeds kh_cloth_pin_place (below) the Draw3D guide.
inline bool kh_cloth_pin_place(KhClothInst& khpf_in, uint32_t khpf_f, const float* khpf_g, size_t khpf_nb) {
    const std::vector<MeshVertex>& khpf_src = khpf_in.out[khpf_f];
    const size_t khpf_nv = khpf_src.size();
    if (khpf_nb == 0u || khpf_nv == 0u || khpf_nv != khpf_in.rest.size()) return false;
    const float* khpf_bc = khpf_in.box_ctr[khpf_f];
    const float* khpf_be = khpf_in.box_size[khpf_f];
    if (khpf_in.chain) {
        const KhChainState& c = khpf_in.ch;
        const size_t khpf_nbone = c.body_of.size();
        const std::vector<float>& khpf_jm = khpf_in.ch_mat[khpf_f];
        if (khpf_nbone == 0u || khpf_nb != khpf_nbone || khpf_jm.size() != khpf_nbone * 12u || !khpf_in.box_on[khpf_f]) return false;
        khpf_in.pin_mat = khpf_jm;
        khpf_in.pin_g.assign(khpf_g, khpf_g + khpf_nb * 12u);   // KH_CHAIN_END_DRAW: an end held on a bone.
        bool khpf_any = false;
        for (size_t bi = 0; bi < khpf_nbone; ++bi) {
            const int32_t bd = c.body_of[bi];
            const float* khpf_off = nullptr;
            if (bd >= 0) {
                if (static_cast<size_t>(bd) >= c.body.size() || c.body[static_cast<size_t>(bd)].inv_m > 0.0f) continue;   // Dynamic: the job's.
                khpf_off = kh_chain_start_off(c, static_cast<size_t>(bd));
            }
            float q[4], t[3];
            kh_chain_bone_map(c, khpf_g, khpf_g, khpf_nb, static_cast<int32_t>(bi), 1.0f, q, t);
            if (khpf_off) for (int k = 0; k < 3; ++k) t[k] += khpf_off[k];   // As kh_chain_place, undone by the writeback: bt + off.
            kh_q_to_axes(q, &khpf_in.pin_mat[bi * 12u]);
            memcpy(&khpf_in.pin_mat[bi * 12u + 9u], t, sizeof(t));
            khpf_any = true;
        }
        if (!khpf_any) return false;
        khpf_in.pin_out = khpf_src;
        const bool inf = c.inf.size() == khpf_nv;
        for (size_t i = 0; i < khpf_nv; ++i) {
            float pp[3], nn[3], tt[3];
            kh_chain_skin_vert(khpf_in.rest[i], c.ext, inf ? &c.inf[i] : nullptr, khpf_in.pin_mat.data(), khpf_nbone, pp, nn, tt);
            // Position, normal and tangent from the same maps, stored as the writeback stores them (a re-placed
            // bone's surface turns with it).
            MeshVertex& o = khpf_in.pin_out[i];
            float n[3], tn[3];
            for (int k = 0; k < 3; ++k) {
                o.pos[k] = (pp[k] - (khpf_bc[k] - c.auth_ctr[k])) / khpf_be[k];
                n[k] = nn[k] * khpf_be[k];
                tn[k] = tt[k] / khpf_be[k];
            }
            const float nl = kh_cloth_v3_len(n), tl = kh_cloth_v3_len(tn);
            for (int k = 0; k < 3; ++k) {
                o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : khpf_in.rest[i].nrm[k];
                o.tan[k] = tl > 1.0e-30f ? tn[k] / tl : khpf_in.rest[i].tan[k];
            }
        }
        return true;
    }
    const size_t khpf_np = khpf_in.st.part.size();
    if (khpf_in.part_inf.size() != khpf_np || khpf_in.vmap.size() != khpf_nv) return false;
    float khpf_is[3], khpf_s[3];
    for (int k = 0; k < 3; ++k) {
        khpf_s[k] = fabsf(khpf_in.in_size[k]) < 1.0e-6f ? 1.0e-6f : khpf_in.in_size[k];
        khpf_is[k] = 1.0f / khpf_s[k];
    }
    const bool khpf_boxed = khpf_in.box_on[khpf_f];
    // KH_CLOTH_PIN_GRADE: the front's own guide, when it has one of this bone count and the bones have moved since
    // (standing still grades nothing).
    const std::vector<float>& khpf_og = khpf_in.out_guide[khpf_f];
    const bool khpf_grade = khpf_og.size() == khpf_nb * 12u &&
                            memcmp(khpf_og.data(), khpf_g, khpf_nb * 12u * sizeof(float)) != 0;
    bool khpf_any = false;
    for (size_t i = 0; i < khpf_nv; ++i) {
        const uint32_t p = khpf_in.vmap[i];
        if (p >= khpf_np) continue;
        if (khpf_in.st.part[p].w > 0.0f) {
            // A gradient particle (att > 0; att and w are the build's, never the job's): the solver's own position,
            // carried along by (1 - w) of its target's move. att = S w / (1 - w), so 1 - w = S / (S + att).
            const float khpf_att = khpf_in.st.part[p].att;
            if (!khpf_grade || !(khpf_att > 0.0f)) continue;   // Fully simulated: the job's, as it is.
            float g1[3], g0[3];
            kh_cloth_guide_at(khpf_in, khpf_g, khpf_nb, khpf_is, p, g1);
            kh_cloth_guide_at(khpf_in, khpf_og.data(), khpf_nb, khpf_is, p, g0);
            const float khpf_k = KH_CLOTH_ATT_SOFT / (KH_CLOTH_ATT_SOFT + khpf_att);
            float d[3];
            for (int k = 0; k < 3; ++k) d[k] = (g1[k] - g0[k]) * khpf_k;
            if (!(d[0] == d[0] && d[1] == d[1] && d[2] == d[2]) || (d[0] == 0.0f && d[1] == 0.0f && d[2] == 0.0f)) continue;
            if (!khpf_any) { khpf_in.pin_out = khpf_src; khpf_any = true; }
            // A difference of two guided positions: the box's centre cancels.
            for (int k = 0; k < 3; ++k) khpf_in.pin_out[i].pos[k] += khpf_boxed ? d[k] * khpf_s[k] / khpf_be[k] : d[k];
            continue;
        }
        if (!khpf_any) { khpf_in.pin_out = khpf_src; khpf_any = true; }
        float g[3];
        kh_cloth_guide_at(khpf_in, khpf_g, khpf_nb, khpf_is, p, g);
        if (khpf_boxed) {   // As kh_cloth_box_out: metres over the size, then into the front's box.
            for (int k = 0; k < 3; ++k) g[k] = (g[k] * khpf_s[k] - (khpf_bc[k] - khpf_in.auth_ctr[k])) / khpf_be[k];
        }
        memcpy(khpf_in.pin_out[i].pos, g, sizeof(g));
    }
    return khpf_any;
}
// KH_CLOTH_STEP_SHAPE - the key of bytes a consume-point writer made from a front rather than the front itself
// (kh_cloth_pin_seal: the pin re-placement; kh_chain_end_draw: the chain end's correction), by KH_CLOTH_SHAPE's rule
// on metric positions, as kh_chain_writeback keys a front (through this helper too): a new key from the process-wide
// serial when some vertex has left the shape that took khck_key by more than KH_CLOTH_SHAPE_M on any axis, else
// khck_key again. Under the front's key the caching passes (the footprint seam, the sun ladder, the point-light maps)
// would keep what the writer moved where the job left it while the job's front stands.
inline uint32_t kh_cloth_step_shape(std::vector<float>& khck_ref, uint32_t& khck_key,
                                    const std::vector<float>& khck_met) {
    bool khck_moved = khck_key == 0u || khck_ref.size() != khck_met.size();
    for (size_t i = 0; i < khck_met.size() && !khck_moved; ++i) {
        if (!(fabsf(khck_met[i] - khck_ref[i]) <= KH_CLOTH_SHAPE_M)) khck_moved = true;
    }
    if (khck_moved) {
        khck_ref = khck_met;
        uint32_t k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (k == 0u) k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        khck_key = k;
    }
    return khck_key;
}
// KH_CLOTH_STEP_SHAPE - pin_out (kh_cloth_pin_place's re-placement of front khps_f) sealed: in the smallest box
// centred on the front's that holds its level-0 vertices and is never smaller than the front's (the re-placed part
// can leave the front's box, and the object's bounds are the box of its bytes) - positions, normals and tangents
// re-expressed in it when it grew, under KhVsCore's convention - and under its own key (kh_cloth_step_shape). Level
// 0 is the front's own box rule: a chain's lvl0, a cloth's particles. A front with no box keeps the front's box and
// key. Returns the key; khps_be: the box the bytes are in. Under g_cloth_mu.
inline uint32_t kh_cloth_pin_seal(KhClothInst& khps_in, uint32_t khps_f, float khps_be[3]) {
    const float* const khps_bf = khps_in.box_size[khps_f];
    memcpy(khps_be, khps_bf, sizeof(float) * 3u);
    if (!khps_in.box_on[khps_f]) return khps_in.shape_of[khps_f];
    std::vector<MeshVertex>& khps_v = khps_in.pin_out;
    const size_t khps_nv = khps_v.size();
    float khps_bc[3];   // The box's centre, carrier-local (the writeback's metric frame).
    for (int k = 0; k < 3; ++k) khps_bc[k] = khps_in.box_ctr[khps_f][k] - khps_in.auth_ctr[k];
    const std::vector<uint8_t>& khps_lv = khps_in.ch.lvl0;
    const size_t khps_np = khps_in.st.part.size();
    float khps_r[3] = { 0.0f, 0.0f, 0.0f };   // The farthest level-0 vertex from the centre, per axis.
    for (size_t i = 0; i < khps_nv; ++i) {
        const bool khps_l0 = khps_in.chain ? (khps_lv.size() != khps_nv || khps_lv[i] != 0u)
                                           : (i < khps_in.vmap.size() && khps_in.vmap[i] < khps_np);
        if (!khps_l0) continue;
        for (int k = 0; k < 3; ++k) {
            const float d = fabsf(khps_v[i].pos[k]) * khps_bf[k];
            if (d > khps_r[k]) khps_r[k] = d;
        }
    }
    bool khps_grew = false;
    for (int k = 0; k < 3; ++k) {
        if (2.0f * khps_r[k] > khps_bf[k]) { khps_be[k] = 2.0f * khps_r[k]; khps_grew = true; }
    }
    if (khps_grew) {
        for (size_t i = 0; i < khps_nv; ++i) {
            MeshVertex& o = khps_v[i];
            float n[3], tn[3];
            for (int k = 0; k < 3; ++k) {
                o.pos[k] = o.pos[k] * khps_bf[k] / khps_be[k];
                n[k] = o.nrm[k] * (khps_be[k] / khps_bf[k]);
                tn[k] = o.tan[k] * (khps_bf[k] / khps_be[k]);
            }
            const float nl = kh_cloth_v3_len(n), tl = kh_cloth_v3_len(tn);
            for (int k = 0; k < 3; ++k) {
                if (nl > 1.0e-30f) o.nrm[k] = n[k] / nl;
                if (tl > 1.0e-30f) o.tan[k] = tn[k] / tl;
            }
        }
    }
    std::vector<float>& khps_met = khps_in.pin_met;
    khps_met.resize(khps_nv * 3u);
    for (size_t i = 0; i < khps_nv; ++i) {
        for (int k = 0; k < 3; ++k) khps_met[i * 3u + k] = khps_v[i].pos[k] * khps_be[k] + khps_bc[k];
    }
    return kh_cloth_step_shape(khps_in.pin_shape_ref, khps_in.pin_shape_key, khps_met);
}
inline bool kh_cloth_pin_fresh(KhClothInst& khpf_in, uint32_t khpf_f) {
    if (!kh_skin_guide_of(khpf_in.seq, khpf_in.mesh, khpf_in.pin_guide)) return false;
    return kh_cloth_pin_place(khpf_in, khpf_f, khpf_in.pin_guide.data(), khpf_in.pin_guide.size() / 12u);
}
// kh_cloth_upload, the device-side half (the flush's housekeeping): creates each instance's buffer, uploads newly
// published frames, resizes the slot tables and empties the grave.
inline void kh_cloth_upload(ID3D11DeviceContext* khcu_ctx, ID3D11Device* khcu_dev) {
    KH_PROF_SCOPE(KHP_CLOTH_UPLOAD);   // KH_PROF.
    if (!khcu_ctx || !khcu_dev) return;
    std::vector<std::shared_ptr<KhClothInst>> khcu_live;
    uint32_t khcu_max_slot = 0;
    uint64_t khcu_inst = 0;
    // KH_CHAIN: each plain chain's installed box, applied to its object in the flush that installs its buffer.
    uint64_t khcu_ch_inst = 0;
    struct KhChainPut { uint32_t slot; uint64_t seq; int mesh; float size[3]; bool cast; };   // cast: KH_CAST_REFIT (a plain cloth's cast_size).
    std::vector<KhChainPut> khcu_ch_put;
    {
        std::lock_guard<std::mutex> khcu_g(g_cloth_mu);
        for (ID3D11Buffer* khcu_b : g_cloth_grave) { if (khcu_b) khcu_b->Release(); }
        g_cloth_grave.clear();
        khcu_live.reserve(g_cloth.size());
        for (const auto& khcu_e : g_cloth) {
            khcu_live.push_back(khcu_e.second);
            if (khcu_e.first + 1u > khcu_max_slot) khcu_max_slot = khcu_e.first + 1u;
        }
    }
    // Render thread, or a park holding it still: every reader and writer of these six tables runs there (the draw
    // paths, through kh_mesh_vb_ref, kh_cloth_gen_of, kh_lod_locked and the like; the three uploads; the draw-time
    // steps kh_skin_draw_finish, kh_cloth_pin_draw_step, kh_chain_end_draw and kh_uvs_step; kh_skin_pool_rebind,
    // kh_cloth_release_all, and kh_cloth_slot_forget via kh_scene_sync / kh_uvs_release_all), so the resize cannot
    // invalidate a reader's pointer.
    std::fill(g_cloth_vb_slot.begin(), g_cloth_vb_slot.end(), nullptr);
    std::fill(g_cloth_gen_slot.begin(), g_cloth_gen_slot.end(), 0u);
    std::fill(g_cloth_mesh_slot.begin(), g_cloth_mesh_slot.end(), -1);
    std::fill(g_cloth_sim_slot.begin(), g_cloth_sim_slot.end(), static_cast<uint8_t>(0u));   // KH_LOD_SIM_SLOT.
    std::fill(g_cloth_voff_slot.begin(), g_cloth_voff_slot.end(), 0u);   // KH_SKIN_POOL.
    std::fill(g_cloth_pool_slot.begin(), g_cloth_pool_slot.end(), nullptr);
    if (g_cloth_vb_slot.size() < khcu_max_slot) g_cloth_vb_slot.resize(khcu_max_slot, nullptr);
    if (g_cloth_gen_slot.size() < khcu_max_slot) g_cloth_gen_slot.resize(khcu_max_slot, 0u);
    if (g_cloth_mesh_slot.size() < khcu_max_slot) g_cloth_mesh_slot.resize(khcu_max_slot, -1);
    if (g_cloth_sim_slot.size() < khcu_max_slot) g_cloth_sim_slot.resize(khcu_max_slot, 0u);
    if (g_cloth_voff_slot.size() < khcu_max_slot) g_cloth_voff_slot.resize(khcu_max_slot, 0u);   // KH_SKIN_POOL.
    if (g_cloth_pool_slot.size() < khcu_max_slot) g_cloth_pool_slot.resize(khcu_max_slot, nullptr);

    // KH_CLOTH_INST - the live instances' fields, under g_cloth_mu for the whole walk: kh_cloth_sync rebuilds a
    // live chain in place (rest, in_boxed) and retires or erases instances (vb to the grave) while this loop may be
    // using that vb. Workers only pop the queue under this lock, so they wait at most one walk. Nested: g_skin_mu via
    // kh_cloth_pin_fresh -> kh_skin_guide_of.
    std::unique_lock<std::mutex> khcu_il(g_cloth_mu);
    for (size_t khcu_i = 0; khcu_i < khcu_live.size(); ++khcu_i) {
        KhClothInst& khcu_in = *khcu_live[khcu_i];
        // Retired: its slot may already be another object's (kh_cloth_sync).
        if (!khcu_in.built || khcu_in.rest.empty() || khcu_in.retired) continue;
        ++khcu_inst;   // A retired instance is not one; one whose buffer failed below still is.
        bool khcu_made = false;   // KH_CLOTH_VB_FRESH (below).
        if (!khcu_in.vb) {
            D3D11_BUFFER_DESC khcu_bd = {};
            khcu_bd.ByteWidth = static_cast<UINT>(khcu_in.rest.size() * sizeof(MeshVertex));
            khcu_bd.Usage = D3D11_USAGE_DYNAMIC;
            khcu_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            khcu_bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(khcu_dev->CreateBuffer(&khcu_bd, nullptr, &khcu_in.vb))) { khcu_in.vb = nullptr; continue; }
            khcu_in.uploaded_gen = 0xFFFFFFFFu;   // Force the first write.
            khcu_made = true;
        }
        const uint32_t khcu_gen = khcu_in.gen.load(std::memory_order_acquire);
        const uint32_t khcu_f = khcu_in.front.load(std::memory_order_acquire);
        // KH_CLOTH_PIN_FRESH: the guide-bound part from this frame's guide; the rest is the front's.
        // KH_CLOTH_PIN_STEP: a buffer the consume-point step wrote this cycle keeps its bytes, key and box (this
        // upload's guide is older).
        // KH_CLOTH_VB_FRESH: a buffer made above holds nothing yet (the step's bytes this cycle went to the one
        // kh_cloth_release_all let go), so it is written whatever the stamp says (the write's vb_wr bump also stops
        // the chain end's keep test vouching for it).
        const bool khcu_stepped = !khcu_made && khcu_in.pin_step_cycle == g_topo_cycles;
        const bool khcu_fresh = !khcu_stepped && kh_cloth_pin_fresh(khcu_in, khcu_f & 1u);
        if (!khcu_stepped && (khcu_gen != khcu_in.uploaded_gen || khcu_fresh)) {
            // KH_CLOTH_STEP_SHAPE: a re-placement goes out in its own box, under its own key; the front, the front's.
            float khcu_be[3];
            memcpy(khcu_be, khcu_in.box_size[khcu_f & 1u], sizeof(khcu_be));
            const uint32_t khcu_key = khcu_fresh ? kh_cloth_pin_seal(khcu_in, khcu_f & 1u, khcu_be)
                                                 : khcu_in.shape_of[khcu_f & 1u];
            const std::vector<MeshVertex>& khcu_src = khcu_fresh ? khcu_in.pin_out : khcu_in.out[khcu_f & 1u];
            D3D11_MAPPED_SUBRESOURCE khcu_m = {};
            if (SUCCEEDED(khcu_ctx->Map(khcu_in.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khcu_m)) && khcu_m.pData) {
                // The WHOLE array, LOD tail included: DISCARD hands back undefined memory, and a stray level-1 draw
                // would render it.
                memcpy(khcu_m.pData, khcu_src.data(), khcu_src.size() * sizeof(MeshVertex));
                khcu_ctx->Unmap(khcu_in.vb, 0);
                kh_uvs_src_wrote(khcu_in.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again.
                ++khcu_in.vb_wr;   // KH_CHAIN_END_DRAW (C3).
                khcu_in.uploaded_gen = khcu_gen;
                khcu_in.uploaded_shape = khcu_key;   // KH_CLOTH_SHAPE: the bytes' (out[f]'s, or the re-placement's).
                // KH_SKEL: and its box, which kh_skin_upload hands the object.
                khcu_in.uploaded_box_on = khcu_in.box_on[khcu_f & 1u];
                memcpy(khcu_in.uploaded_box_ctr, khcu_in.box_ctr[khcu_f & 1u], sizeof(khcu_in.uploaded_box_ctr));
                memcpy(khcu_in.uploaded_box_size, khcu_be, sizeof(khcu_in.uploaded_box_size));
                memcpy(khcu_in.uploaded_cast_size, khcu_in.cast_size[khcu_f & 1u], sizeof(khcu_in.uploaded_cast_size));   // KH_CAST_REFIT.
            }
        }
        if (khcu_in.slot < g_cloth_vb_slot.size()) g_cloth_vb_slot[khcu_in.slot] = khcu_in.vb;
        if (khcu_in.slot < g_cloth_gen_slot.size()) g_cloth_gen_slot[khcu_in.slot] = khcu_in.uploaded_shape;
        if (khcu_in.slot < g_cloth_mesh_slot.size()) g_cloth_mesh_slot[khcu_in.slot] = khcu_in.mesh;
        if (khcu_in.slot < g_cloth_sim_slot.size()) g_cloth_sim_slot[khcu_in.slot] = 1u;   // KH_LOD_SIM_SLOT.
        if (khcu_in.slot < g_cloth_voff_slot.size()) g_cloth_voff_slot[khcu_in.slot] = 0u;   // KH_SKIN_POOL: its own.
        if (khcu_in.slot < g_cloth_pool_slot.size()) g_cloth_pool_slot[khcu_in.slot] = nullptr;
        if (khcu_in.chain) {
            --khcu_inst;
            ++khcu_ch_inst;
            if (!khcu_in.in_boxed && khcu_in.uploaded_box_on && khcu_in.vb) {
                KhChainPut khcu_p;
                khcu_p.slot = khcu_in.slot;
                khcu_p.seq = khcu_in.seq;
                khcu_p.mesh = khcu_in.mesh;
                khcu_p.size[0] = khcu_in.uploaded_box_size[0];   // SQF order, as RenderObject::size.
                khcu_p.size[1] = khcu_in.uploaded_box_size[2];
                khcu_p.size[2] = khcu_in.uploaded_box_size[1];
                khcu_p.cast = false;
                khcu_ch_put.push_back(khcu_p);
            }
        } else if (!khcu_in.in_boxed && khcu_in.vb && khcu_in.uploaded_gen != 0xFFFFFFFFu) {   // KH_CAST_REFIT: a plain cloth (a buffer uploaded at least once).
            KhChainPut khcu_p;
            khcu_p.slot = khcu_in.slot;
            khcu_p.seq = khcu_in.seq;
            khcu_p.mesh = khcu_in.mesh;
            khcu_p.size[0] = khcu_in.uploaded_cast_size[0];   // Engine axes -> SQF order, as the chain's put.
            khcu_p.size[1] = khcu_in.uploaded_cast_size[2];
            khcu_p.size[2] = khcu_in.uploaded_cast_size[1];
            khcu_p.cast = true;
            khcu_ch_put.push_back(khcu_p);
        }
    }
    khcu_il.unlock();   // Before the draw-list scope below.
    kh_cloth_stats_publish(khcu_inst, khcu_ch_inst, g_physics_col_n.load(std::memory_order_relaxed));
    if (!khcu_ch_put.empty()) {
        // KH_CHAIN: a plain chain's buffer is stored in a box about its rest centre, so its position stays the
        // script's and only its size follows (kh_chain_writeback). A skeletal one's box is handed over with its
        // centre by kh_skin_upload, next.
        std::lock_guard<std::mutex> khcu_g(g_draw_list_mutex);
        for (auto& khcu_kv : g_draw_list) {
            RenderObject& khcu_o = khcu_kv.second;
            if (khcu_o.skel || khcu_o.slot == 0xFFFFFFFFu) continue;
            // KH_CAST_REFIT: a plain cloth takes its cast_size; a plain chain its size.
            const bool khcu_plain = !khcu_o.chain_sim && kh_cloth_obj_sim(khcu_o);
            if (!khcu_o.chain_sim && !khcu_plain) continue;
            for (const KhChainPut& khcu_p : khcu_ch_put) {
                if (khcu_p.cast != khcu_plain) continue;
                if (khcu_p.slot != khcu_o.slot || khcu_p.seq != khcu_o.seq || khcu_p.mesh != khcu_o.mesh) continue;
                float* khcu_dst = khcu_p.cast ? khcu_o.cast_size : khcu_o.size;
                if (memcmp(khcu_dst, khcu_p.size, sizeof(khcu_o.size)) != 0) {
                    memcpy(khcu_dst, khcu_p.size, sizeof(khcu_o.size));
                    kh_scene_mark(khcu_o.slot);
                }
                break;
            }
        }
    }
}

// Device teardown: every buffer goes and the slot tables empty (kh_skin_release_all relies on it); workers keep
// running (they hold no device object). Under a park or on the render thread (on_engine_reset's best effort, after
// its lock attempts failed, is neither).
inline void kh_cloth_release_all() {
    std::vector<ID3D11Buffer*> khcr_vbs;
    {
        std::lock_guard<std::mutex> khcr_g(g_cloth_mu);
        // Taken out under the lock: kh_cloth_sync's retire and refused rebuild move an instance's vb into the grave
        // under it too, so each buffer is released here or from the grave, never both. A worker never touches vb, so
        // a busy instance's buffer is still ours to release.
        for (const auto& khcr_e : g_cloth) {
            khcr_vbs.push_back(khcr_e.second->vb);
            khcr_e.second->vb = nullptr;
            khcr_e.second->uploaded_gen = 0xFFFFFFFFu;
        }
        for (ID3D11Buffer* khcr_b : g_cloth_grave) { if (khcr_b) khcr_b->Release(); }
        g_cloth_grave.clear();
    }
    for (ID3D11Buffer* khcr_b : khcr_vbs) { if (khcr_b) khcr_b->Release(); }
    std::fill(g_cloth_vb_slot.begin(), g_cloth_vb_slot.end(), nullptr);
    std::fill(g_cloth_gen_slot.begin(), g_cloth_gen_slot.end(), 0u);
    std::fill(g_cloth_mesh_slot.begin(), g_cloth_mesh_slot.end(), -1);
    std::fill(g_cloth_sim_slot.begin(), g_cloth_sim_slot.end(), static_cast<uint8_t>(0u));   // KH_LOD_SIM_SLOT.
    std::fill(g_cloth_voff_slot.begin(), g_cloth_voff_slot.end(), 0u);   // KH_SKIN_POOL.
    std::fill(g_cloth_pool_slot.begin(), g_cloth_pool_slot.end(), nullptr);
}

// Mission teardown, from the game thread under the park. The objects are gone too, so the instances go with their
// buffers rather than waiting for a flush to retire them (a mission that never draws again would never run one).
// Its one caller (kh_session_teardown) has joined the workers first (kh_cloth_workers_stop), so no job runs here; an
// instance whose queued job was dropped still reads busy and goes with the table. The per-mesh warning list
// follows the session's error dedupe, so a fresh mission re-reports.
inline void kh_cloth_drop_all() {
    kh_cloth_release_all();
    {
        std::lock_guard<std::mutex> khcd_g(g_cloth_mu);
        g_cloth.clear();
    }
    g_physics_col.clear();
    g_physics_col_n.store(0, std::memory_order_relaxed);
    g_cloth_objs.clear();
    g_cloth_objs_h.clear();
    g_cloth_warned.clear();
}

// KH_CLOTH_PIN_STEP - a skeletal binding's cloth, bound part at the consume point. kh_cloth_upload's re-place runs
// at the scene resolve, after the injection, so on its own the mesh draws one frame behind its bones. This step
// re-places the same part from the maps of the stash just taken (kh_skin_step_maps), at the upload's front in that
// front's box (grown to hold it, under its own key: kh_cloth_pin_seal), writes the installed buffer and stamps the
// cycle (KhClothInst::pin_step_cycle) so the upload leaves it. The simulated part is the job's front either way.
// From kh_attach_step under g_draw_list_mutex; takes g_cloth_mu inside (list -> cloth) and never g_skin_mu.
inline void kh_cloth_pin_draw_step(KhAttach& khcp_a, RenderObject& khcp_o, ID3D11DeviceContext* khcp_ctx, bool& khcp_moved) {
    const KhSkinInst* const khcp_in = khcp_a.skin.get();
    if (!khcp_ctx || !khcp_in || !khcp_in->cloth || khcp_o.slot == 0xFFFFFFFFu) return;
    if (!kh_skin_step_maps(khcp_a, khcp_in, khcp_o)) return;
    bool khcp_box = false;
    float khcp_bc[3] = {}, khcp_bs[3] = {};
    {
        std::lock_guard<std::mutex> khcp_g(g_cloth_mu);
        const auto khcp_it = g_cloth.find(khcp_o.slot);
        if (khcp_it == g_cloth.end()) return;
        KhClothInst& khcp_cl = *khcp_it->second;
        if (khcp_cl.retired || !khcp_cl.built || khcp_cl.rest.empty() || !khcp_cl.vb ||
            khcp_cl.seq != khcp_o.seq || khcp_cl.mesh != khcp_in->mesh) return;
        // Installed by an upload already; until then a new instance draws the mesh's own vertices. KH_USER_VS:
        // installed too when the slot draws a vertex stage's buffer deformed from this one (kh_uvs_draws_from) - the
        // table then names the stage's buffer; standing down for it would leave the bound part a frame behind its
        // bones.
        const bool khcp_direct = khcp_o.slot < g_cloth_vb_slot.size() && g_cloth_vb_slot[khcp_o.slot] == khcp_cl.vb;
        if (!khcp_direct && !kh_uvs_draws_from(khcp_o.slot, khcp_cl.vb)) return;
        const uint32_t khcp_gen = khcp_cl.gen.load(std::memory_order_acquire);
        const uint32_t khcp_f = khcp_cl.front.load(std::memory_order_acquire) & 1u;
        if (!kh_cloth_pin_place(khcp_cl, khcp_f, khcp_a.draw_a.data(), khcp_a.draw_a.size() / 12u)) return;   // Nothing bound: the upload's front stands.
        if (khcp_cl.pin_out.size() != khcp_cl.rest.size()) return;
        float khcp_be[3];   // KH_CLOTH_STEP_SHAPE: the bytes' box and key.
        const uint32_t khcp_key = kh_cloth_pin_seal(khcp_cl, khcp_f, khcp_be);
        D3D11_MAPPED_SUBRESOURCE khcp_m = {};
        if (FAILED(khcp_ctx->Map(khcp_cl.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khcp_m)) || !khcp_m.pData) return;
        memcpy(khcp_m.pData, khcp_cl.pin_out.data(), khcp_cl.pin_out.size() * sizeof(MeshVertex));   // The whole array, as the upload.
        khcp_ctx->Unmap(khcp_cl.vb, 0);
        kh_uvs_src_wrote(khcp_o.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again (kh_uvs_step follows this step).
        ++khcp_cl.vb_wr;   // KH_CHAIN_END_DRAW (C3).
        // What the upload records with a write: the generation, the key, the box - these bytes'.
        khcp_cl.uploaded_gen = khcp_gen;
        // KH_SKIN_RESTEP_KEY (kh_skin_draw_finish's rule): bytes of a new shape, against the key of the bytes this
        // buffer held (the upload's or the last step's), move the mesh, a vertex stage over it or not.
        if (khcp_cl.uploaded_shape != khcp_key) khcp_moved = true;
        khcp_cl.uploaded_shape = khcp_key;
        khcp_cl.uploaded_box_on = khcp_cl.box_on[khcp_f];
        memcpy(khcp_cl.uploaded_box_ctr, khcp_cl.box_ctr[khcp_f], sizeof(khcp_cl.uploaded_box_ctr));
        memcpy(khcp_cl.uploaded_box_size, khcp_be, sizeof(khcp_cl.uploaded_box_size));
        khcp_cl.pin_step_cycle = g_topo_cycles;
        khcp_cl.pin_front = khcp_f;   // KH_CHAIN_END_DRAW.
        // Sized with g_cloth_vb_slot by the upload. Under a vertex stage the slot's key is the stage's (kh_uvs_step
        // moves it with this copy), as kh_skin_draw_finish leaves it.
        if (khcp_direct) g_cloth_gen_slot[khcp_o.slot] = khcp_cl.uploaded_shape;
        khcp_box = khcp_cl.uploaded_box_on;
        memcpy(khcp_bc, khcp_cl.uploaded_box_ctr, sizeof(khcp_bc));
        memcpy(khcp_bs, khcp_cl.uploaded_box_size, sizeof(khcp_bs));
    }
    // The box of the bytes just written, as kh_skin_upload's cloth put hands it over (centre engine axes, size SQF
    // order). Unboxed, the rest box stands.
    if (!khcp_box) return;
    const float khcp_sz[3] = { khcp_bs[0], khcp_bs[2], khcp_bs[1] };
    if (memcmp(khcp_o.skel_ctr, khcp_bc, sizeof(khcp_bc)) != 0 || memcmp(khcp_o.size, khcp_sz, sizeof(khcp_sz)) != 0) {
        memcpy(khcp_o.skel_ctr, khcp_bc, sizeof(khcp_bc));
        memcpy(khcp_o.size, khcp_sz, sizeof(khcp_sz));
        khcp_moved = true;
    }
}
}