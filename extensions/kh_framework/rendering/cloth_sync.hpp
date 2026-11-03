#pragma once

// The feature half (rendering_integration_features.hpp): the cloth frame: cloth_sync, the pins, the upload and the
// releases. Never include it on its own.
namespace RenderIntegration {

inline bool uvs_draws_from(uint32_t slot, const ID3D11Buffer* src);   // KH_USER_VS: the slot draws a vertex stage's buffer deformed from this one.
inline void uvs_src_wrote(uint32_t slot);   // KH_USER_VS: a slot's own per-instance vertex buffer was just written (every writer of one calls it).
// SKEL: a skeletal binding's pose for its cloth (defined with the skin runtime; read by cloth_sync and
// cloth_pin_fresh).
inline bool skin_guide_of(uint64_t seq, int mesh, std::vector<float>& out);

inline float effect_time_seconds();   // Defined in rendering/effect_clock.hpp (declared here for the use below).

// Defined in rendering/skinning.hpp (declared here for the uses below).
inline void skin_col_take(bool snap);
inline bool skin_step_maps(Attach& a, const SkinInst* in, const RenderObject& o);


// The frame's cloth work, from the game thread, BEFORE the park: creates and retires instances, publishes each
// one's inputs and queues it. Nothing here touches the device. wanted is flush_frame's census verdict for this
// frame (some object has clothSimulation or chainSimulation on); with it false and no instance standing this is the
// inert path.
inline void cloth_sync(float dt, bool wanted) {
    if (!wanted && g_cloth.empty()) {
        // A clear of an already-empty vector: the inert path costs nothing.
        g_physics_col.clear();
        g_physics_col_n.store(0, std::memory_order_relaxed);
        skin_col_take(false);   // The freshness window moves every frame.
        // AFF_REAP_IDLE: aff_frame_build's reap, with nothing to build - otherwise an expired affector goes
        // only when an affector command comes, and physicsAffectors counts it meanwhile.
        if (!g_affectors.empty()) aff_reap(effect_time_seconds_d());
        return;
    }
    // The simulation's consume point: objects are read right before the transforms go to the workers (a cloth
    // stepped against last frame's transform lags its mesh). From g_draw_list under its mutex, not g_scene: the
    // render thread may be walking g_scene lock-free, and g_draw_list is where attach_step writes.
    g_cloth_objs.clear();
    g_cloth_objs_h.clear();
    {
        std::lock_guard<std::mutex> l(g_draw_list_mutex);
        for (const auto& kv : g_draw_list) {
            if (cloth_obj_sim(kv.second) || physics_obj_collider(kv.second) ||
                chain_obj_sim(kv.second)) {   // CHAIN: the chains ride the cloth's plumbing.
                g_cloth_objs.push_back(kv.second);
                g_cloth_objs_h.push_back(kv.first);
            }
        }
        skin_col_take(true);   // The collider table as these boxes pair with it.
    }
    const std::vector<RenderObject>& objs = g_cloth_objs;
    cloth_workers_start();
    physics_gather_colliders(objs, g_physics_col);
    g_physics_col_n.store(g_physics_col.size(), std::memory_order_relaxed);

    // Retire instances whose object (by creation stamp, not merely the same slot and mesh) is gone or has turned
    // cloth off. The buffer goes to the grave, not released here: the render thread may still be inside a pass
    // holding this slot's pointer. One a worker owns is marked retired and goes the first frame it is free; a
    // retired instance never revives, so an object that turned cloth off and on while its worker ran gets a fresh one.
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        for (auto it = g_cloth.begin(); it != g_cloth.end(); ) {
            ClothInst& ri = *it->second;
            bool live = false;
            for (size_t i = 0; i < objs.size() && !ri.retired; ++i) {
                const RenderObject& o = objs[i];
                // CHAIN: a chain lives on as a chain and a cloth as a cloth.
                const bool as = ri.chain ? chain_obj_sim(o) : cloth_obj_sim(o);
                if (o.slot == it->first && as &&
                    mesh_id_clamp(o.mesh) == ri.mesh && o.seq == ri.seq) { live = true; break; }
            }
            if (live) { ++it; continue; }
            if (ri.busy.load(std::memory_order_acquire)) { ri.retired = true; ++it; continue; }
            if (ri.vb) { g_cloth_grave.push_back(ri.vb); ri.vb = nullptr; }
            // CLOTH_ERASE_RETIRE: cloth_upload walks a snapshot of the table taken under an earlier hold of this
            // lock; an instance erased in between must read as retired there, or the walk (seeing no vb) makes it a
            // buffer nothing releases and draws the dead pose in the slot for the frame.
            ri.retired = true;
            it = g_cloth.erase(it);
        }
    }

    aff_frame_build(effect_time_seconds_d());   // AFFECTOR.
    // SIM_LOD: the boundary view once per frame; stale = unknown.
    float lcam[3] = { 0.0f, 0.0f, 0.0f }, lm11 = 0.0f, lt = 0.0f;
    const bool lview = simlod_read(lcam, lm11, lt) &&
                       effect_time_seconds() - lt <= SIMLOD_STALE_S;
    uint64_t lwork = 0, lfull = 0;
    const uint32_t pub = mesh_count();   // EA_FBX_NO_PARK (below).
    for (size_t i = 0; i < objs.size(); ++i) {
        const RenderObject& o = objs[i];
        // CHAIN: a chain takes this loop too, with its own build, settings (by handle, ChainCfg) and refusals;
        // from the busy test down the plumbing is shared.
        const bool ch = chain_obj_sim(o);
        if (!cloth_obj_sim(o) && !ch) continue;
        const ChainCfg* cfg = nullptr;
        if (ch) {
            const auto ci = g_chain_cfg.find(g_cloth_objs_h[i]);
            if (ci != g_chain_cfg.end() && ci->second.seq == o.seq) cfg = &ci->second;
        }
        if (o.slot == 0xFFFFFFFFu) continue;
        // EA_FBX_NO_PARK: a mesh appended but not yet published (an import, a generated mesh - until the next
        // injection or flush lands it) is no verdict: clamped to the box it would be refused and reported as 'box'.
        // The object waits, as skin_sync and the collider gather wait on mesh_alive; the retire walk above
        // already ended an instance of its old mesh, as it would for any new one. A released id (below the count)
        // keeps the path below.
        if (static_cast<uint32_t>(o.mesh) >= pub) continue;
        const int mid = mesh_id_clamp(o.mesh);
        // CHAIN: a refusal is counted and named once per mesh and reason.
        auto refuse = [&](const std::string& why) {
            const std::string key = std::to_string(mid) + "|" + why;
            for (const std::string& w : g_chain_warned) if (w == key) return;
            if (g_chain_warned.size() >= 64u) return;
            g_chain_warned.push_back(key);
            report_error_once_safe("KH RenderIntegration: chain: mesh '" + mesh_def(mid).name + "' " + why +
                                   ", so it cannot simulate as a chain");
        };
        const std::string start = cfg ? cfg->start : std::string();
        const std::string end = cfg ? cfg->end : std::string();
        const bool shead = cfg ? cfg->start_head : false;   // CHAIN_POINT.
        const bool etail = cfg ? cfg->end_tail : false;
        const ChainParams cp = cfg ? cfg->par : chain_defaults();
        if (ch && !chain_mesh_ok(mid)) {
            refuse("carries no skeleton (import a model whose bones skin its vertices)");
            continue;
        }
        // CHAIN_REFUSED_SKIN: the one key rule (skin_sync asks it too).
        const std::string nbk = ch ? chain_nobuild_key(o, cfg) : std::string();
        // CHAIN_REST_BACK: a refused plain chain draws its own buffer at its authored size. The upload's put can
        // have handed the object the old chain's box after the command staged that size, and with no instance
        // nothing puts again. g_draw_list_mutex with no g_cloth_mu held (list -> cloth); a skeletal box is the skin's.
        auto rest_back = [&]() {
            if (o.skel) return;
            float ns[3];
            memcpy(ns, mesh_def(mid).native_size, sizeof(ns));
            std::lock_guard<std::mutex> dl(g_draw_list_mutex);
            const auto de = g_draw_list.find(g_cloth_objs_h[i]);
            if (de == g_draw_list.end()) return;
            RenderObject& lo = de->second;
            if (lo.seq != o.seq || lo.mesh != o.mesh || !lo.chain_sim || lo.skel ||
                lo.slot == 0xFFFFFFFFu) return;
            if (memcmp(lo.size, ns, sizeof(ns)) == 0) return;
            memcpy(lo.size, ns, sizeof(ns));
            scene_mark(lo.slot);
        };
        if (ch && std::find(g_chain_nobuild.begin(), g_chain_nobuild.end(), nbk) != g_chain_nobuild.end()) {
            // CHAIN_REFUSED_LIVE: a refused key can come back while the object holds a chain built from other
            // names (built with X, refused Y, rebuilt X, then Y again); the rebuild below is never reached, so that
            // instance would stay - never stepped, still drawn, frozen. It goes as the rebuild's refusal sends it
            // (marked while a worker owns it, erased by the sweep once free), and the mesh's own buffer draws.
            // The common case (no instance) is told without g_cloth_mu: only this thread adds or erases g_cloth
            // entries (cloth_sync; cloth_drop_all and the session reset, under the park), so a lookup here
            // races no structural change; the render thread's readers only read.
            bool live = false;
            if (g_cloth.find(o.slot) != g_cloth.end()) {
                std::lock_guard<std::mutex> g(g_cloth_mu);
                const auto it = g_cloth.find(o.slot);
                if (it != g_cloth.end() && !it->second->retired) {
                    ClothInst& ri = *it->second;
                    if (ri.chain && ri.seq == o.seq && ri.mesh == mid) {
                        ri.retired = true;   // CLOTH_ERASE_RETIRE (the rebuild's refusal).
                        if (!ri.busy.load(std::memory_order_acquire)) {
                            if (ri.vb) { g_cloth_grave.push_back(ri.vb); ri.vb = nullptr; }
                            g_cloth.erase(it);
                        }
                    } else {
                        live = true;   // Not this object's chain: not this refusal's to end.
                    }
                }
            }
            // CHAIN_REST_BACK: a put the render thread built before the refusal's erase may land after its restore;
            // healed here while no live instance puts for this slot.
            if (!live && !o.skel &&
                memcmp(mesh_def(mid).native_size, o.size, sizeof(o.size)) != 0) {
                rest_back();
            }
            continue;
        }
        auto nobuild = [&](const std::string& why) {
            refuse(why);
            // CHAIN_NOBUILD_PRUNE: always filed; a full list first drops the keys no live chain object has as its
            // current key. Both callers hold no lock here.
            if (g_chain_nobuild.size() >= 256u) chain_nobuild_prune();
            g_chain_nobuild.push_back(nbk);
        };
        if (!ch && !cloth_mesh_ok(mid)) {
            // Named once per mesh: the counter says that it happened, the modeller needs to know which model.
            bool seen = false;
            for (size_t w = 0; w < g_cloth_warned.size(); ++w) {
                if (g_cloth_warned[w] == mid) { seen = true; break; }
            }
            if (!seen && g_cloth_warned.size() < 64u) {
                g_cloth_warned.push_back(mid);
                report_error_once_safe("KH RenderIntegration: cloth: mesh '" + mesh_def(mid).name +
                                       "' carries no kh_cloth_sim / kh_cloth_pin weights - "
                                       "no vertex group reached the FBX as a skin cluster and no colour "
                                       "set of either name was found, so it cannot simulate");
            }
            continue;
        }

        std::shared_ptr<ClothInst> in;
        std::string fail;   // CHAIN: a refused build, reported once the lock is let go.
        {
            std::lock_guard<std::mutex> g(g_cloth_mu);
            const auto it = g_cloth.find(o.slot);
            if (it != g_cloth.end()) {
                // A retired one is another object's, still on its worker; this object gets its own once the slot's
                // entry is free.
                if (it->second->retired) continue;
                in = it->second;
            }
        }
        if (!in) {
            // CLOTH_BUILD_UNLOCKED: a new instance is built before g_cloth_mu is taken - nothing else can see it
            // until the emplace (this thread is the table's only inserter, so the slot is still free at the emplace),
            // and under the lock the build (a cloth's weld and constraints; a chain's bone walk and bodies) would hold
            // the render thread's cloth_upload / pin step / chain-end draw for its whole length. The in-place chain
            // rebuild below keeps the lock: that instance is live.
            std::shared_ptr<ClothInst> nw = std::make_shared<ClothInst>();
            nw->par = o.cloth;
            nw->seq = o.seq;
            if (ch) {
                if (!chain_build(*nw, mesh_def(mid), o.slot, mid, start, end,
                                 shead, etail, cp.mass, fail)) {
                    nobuild(fail);
                    continue;
                }
                nw->cpar = cp;
            } else if (!cloth_build(*nw, mesh_def(mid), o.slot, mid)) {
                continue;
            }
            std::lock_guard<std::mutex> g(g_cloth_mu);
            g_cloth.emplace(o.slot, nw);
            in = std::move(nw);
        }
        // A busy instance is a worker's: its inputs may not be touched and it may not be queued twice. The missed
        // frame is a frame of stale cloth, which g_cloth_skipped / g_chain_skipped count (stats armed only), and its
        // time is owed to the next step (dt_owed, capped at CLOTH_DT_MAX), which then spans the interval its
        // carrier and colliders moved across, up to that cap. Do not drop the owed time: the cloth falls behind real
        // time and reads two frames' travel as one frame's velocity.
        if (in->busy.load(std::memory_order_acquire)) {
            if (stats_on()) (ch ? g_chain_skipped : g_cloth_skipped).fetch_add(1u, std::memory_order_relaxed);
            in->dt_owed += dt;
            if (in->dt_owed > CLOTH_DT_MAX) in->dt_owed = CLOTH_DT_MAX;
            continue;
        }
        // CHAIN: new bone names are a new chain, rebuilt here (the instance is the game thread's) on the same
        // buffers (same mesh).
        if (ch && (in->ch_start != start || in->ch_end != end ||
                   in->ch_start_head != shead || in->ch_end_tail != etail)) {
            std::string why;
            bool rebuilt = false;
            {   // CLOTH_INST: a live instance, whose rest the render-thread upload reads. A refusal has already
                // reset its chain and state (chain_build), so it leaves the table in this same hold.
                std::lock_guard<std::mutex> rb(g_cloth_mu);
                rebuilt = chain_build(*in, mesh_def(mid), o.slot, mid, start, end,
                                      shead, etail, cp.mass, why);
                if (!rebuilt) {   // Refused: the instance goes, and the mesh's own buffer draws.
                    const auto it = g_cloth.find(o.slot);
                    if (it != g_cloth.end() && it->second == in) {
                        if (in->vb) { g_cloth_grave.push_back(in->vb); in->vb = nullptr; }
                        in->retired = true;   // CLOTH_ERASE_RETIRE (above).
                        g_cloth.erase(it);
                    }
                }
            }
            if (!rebuilt) {
                nobuild(why);
                rest_back();   // CHAIN_REST_BACK (above).
                continue;
            }
        }
        // The interval this step covers.
        const float dt_now = dt + in->dt_owed;
        in->dt_owed = 0.0f;

        float size[3];
        cloth_engine_size(o, size);
        // SKEL: a skeletal binding steps in its REST box, whatever box its drawn shape has given the object
        // (cloth_box_out): the metric rest lengths are that box's, and it never resizes.
        const MeshDef& md = mesh_def(mid);
        // CHAIN: a chain steps in its rest box whatever it is bound to - drawn at its authored size, its box is
        // its shape's.
        if (o.skel || ch) {
            size[0] = md.native_size[0];
            size[1] = md.native_size[2];
            size[2] = md.native_size[1];
        }
        for (int k = 0; k < 3; ++k) if (fabsf(size[k]) < 1.0e-6f) size[k] = 1.0e-6f;
        const float tele = ch ? cp.teleport : o.cloth.teleport;
        const float range = ch ? cp.range : o.cloth.range;
        if (ch) {
            // CHAIN: 'mass' is shared into the bodies at the build, so a new one rescales them here, as the
            // cloth's does its particles.
            if (in->ch.mass_built != cp.mass && in->ch.mass_built > 0.0f && cp.mass > 0.0f) {
                const float r = in->ch.mass_built / cp.mass;
                // CLOTH_MASS_LOCK: inv_m is read on the render thread under g_cloth_mu (cloth_pin_place,
                // chain_end_fix), as in_size is (CLOTH_INST).
                std::lock_guard<std::mutex> ml(g_cloth_mu);
                for (ChainBody& b : in->ch.body) {
                    if (!(b.inv_m > 0.0f)) continue;
                    b.inv_m *= r;
                    for (int k = 0; k < 9; ++k) { b.ib_inv[k] *= r; b.ib[k] /= r; }
                }
                in->ch.mass_built = cp.mass;
            }
            in->cpar = cp;
        } else {
        // Rest lengths are metric, so a resized mesh needs them rebuilt, and the tethers (rest-length sums) with
        // them.
        bool resize = !in->built || in->st.stretch.empty();
        // SUB_CLOCK: against the size the last step was given (st.size stands between frames).
        for (int k = 0; k < 3; ++k) if (fabsf(size[k] - in->st.fend.size[k]) > 1.0e-5f) resize = true;
        // The shell links are metric too: the dial moving is a rebuild.
        if (in->shell_built != o.cloth.shell) resize = true;
        // 'mass' is the one parameter the build bakes into the particles rather than the step reading it from par
        // (only the media drag, CLOTH_DRAG, reads par.mass live), so a change must reach the inverse masses here.
        if (in->par.mass != o.cloth.mass) {
            const float w = cloth_inv_mass(o.cloth.mass);
            // CLOTH_MASS_LOCK: w is read on the render thread under g_cloth_mu (cloth_pin_place).
            std::lock_guard<std::mutex> ml(g_cloth_mu);
            for (size_t p = 0; p < in->st.part.size(); ++p) {
                if (in->st.part[p].w > 0.0f) in->st.part[p].w = w;
            }
        }
        in->par = o.cloth;
        if (resize) cloth_rest_lengths(*in, size);
        }

        // The carrier in the world, then in the instance's own frame. A fresh instance takes its origin where it
        // stands; the first step places it from rest whatever the origin was.
        // POS_RES: the same centre exact (the object's residual and the offset in double) for the frame the
        // write-back makes the vertices relative to (the draw adds the residual back); wc, a world float, serves
        // the broadphase, the ground patch and the simulation level.
        float wc[3];
        double wd[3];
        cloth_carrier_at(o, md.native_ctr, wc, wd);
        {
            double od[3];
            for (int k = 0; k < 3; ++k) od[k] = wd[k] - in->org[k];
            if (!in->org_on) {
                for (int k = 0; k < 3; ++k) in->org[k] = wd[k];
                in->org_on = true;
            } else if (od[0] * od[0] + od[1] * od[1] + od[2] * od[2] >
                       CLOTH_ORIGIN_M * CLOTH_ORIGIN_M) {
                cloth_origin_shift(*in, od);
            }
        }
        for (int k = 0; k < 3; ++k) {
            in->in_centre[k] = static_cast<float>(wd[k] - in->org[k]);
        }
        in->st.sea = static_cast<float>(-in->org[CLOTH_UP]);
        memcpy(in->in_rot, o.rot_m, sizeof(in->in_rot));
        in->in_rotated = o.rotated;
        in->in_dt = dt_now;
        memcpy(in->in_wind, g_cloth_wind, sizeof(in->in_wind));
        // SKEL: the box rule, and this frame's pose from skin_sync, which ran just ahead of this function.
        {   // CLOTH_INST: the upload's chain box put reads it; in_size too (cloth_pin_place, render thread).
            std::lock_guard<std::mutex> bx(g_cloth_mu);
            in->in_boxed = o.skel;
            memcpy(in->in_size, size, sizeof(in->in_size));
        }
        if (!o.skel || !skin_guide_of(o.seq, mid, in->in_guide)) in->in_guide.clear();
        if (ch) chain_target_of(cfg, in->org, in->in_tg);   // CHAIN: the end, this frame.

        // The colliders in reach. The cloth's bound is the larger of its rest extent and how far it reached at its
        // last step (a cloth swung or streamed outside its rest box keeps the colliders it swings into), plus the
        // script's range for the motion since.
        float reach = 0.0f;
        for (int k = 0; k < 3; ++k) reach += 0.25f * size[k] * size[k];
        reach = sqrtf(reach);
        if (in->st.reach > reach) reach = in->st.reach;
        reach += range > 0.0f ? range : 1.0f;
        {   // SIM_LOD: this step's counts. The sphere is the larger of the rest box's half-diagonal and how far the
            // cloth reached, about the carrier's centre (world, engine axes).
            float lr2 = 0.0f;
            for (int k = 0; k < 3; ++k) lr2 += 0.25f * size[k] * size[k];
            float lr = sqrtf(lr2);
            if (in->st.reach > lr) lr = in->st.reach;
            const float lq = simlod_share(lview, lcam, lm11, wc, lr, o.sim_lod, o.sim_lod_on);
            const float lqs = lq * 2.0f < 1.0f ? lq * 2.0f : 1.0f;   // Substeps give only below half.
            const uint16_t dit = ch ? in->cpar.iterations : in->par.iterations;
            const uint16_t dsb = ch ? in->cpar.substeps : in->par.substeps;
            in->in_iter = simlod_count(dit, lq, in->in_iter);
            in->in_sub = simlod_count(dsb, lqs, in->in_sub);
            // CHAIN_LOD_FLOOR: the work a chain's step actually runs (cloth_run raises it to the floor).
            uint16_t wit = in->in_iter, wsb = in->in_sub;
            if (ch) {
                wit = chain_lod_floor(dit, wit, CHAIN_LOD_ITER);
                wsb = chain_lod_floor(dsb, wsb, CHAIN_LOD_SUB);
            }
            lwork += static_cast<uint64_t>(wit) * wsb;
            lfull += static_cast<uint64_t>(dit < 1u ? 1u : dit) * (dsb < 1u ? 1u : dsb);
        }
        cloth_ground_patch(*in, wc, reach, tele);   // CLOTH_GROUND.
        // Each collider's pose at THIS cloth's last step, matched by slot and mesh, becomes its prev, so the step
        // sweeps it over exactly the interval it sweeps the cloth's own frame - across a skipped frame too.
        // SUB_CLOCK: its pose where the simulation stands, st.sub_t of the way along what that step swept it over
        // (that step's own pose when nothing was left over), so the sweep runs on from there. A collider not seen
        // before, or one that jumped by the cloth's teleport test, is taken as still. The test predicts from the
        // collider's velocity at that step, as the carrier's does (cloth_step), so a fast collider is swept.
        std::vector<PhysicsColliderView> seen;
        seen.swap(in->in_col);
        in->in_col.clear();
        in->in_col_own.clear();
        for (size_t c = 0; c < g_physics_col.size(); ++c) {
            const PhysicsColliderView& v = g_physics_col[c].view;
            // Never the object's own view: its rest triangles stand exactly where its particles or contact points
            // start, so each would be pushed off its own surface.
            if (v.slot == o.slot) continue;
            if (!v.inside) {
                float d[3];
                cloth_v3_sub(v.bs_c, wc, d);   // Both world: the broadphase.
                const float rr = v.bs_r + reach;
                if (cloth_v3_dot(d, d) > rr * rr) continue;
            }
            in->in_col.push_back(v);
            PhysicsColliderView& nv = in->in_col.back();
            // Into this instance's frame, in double: the gather's views are world (POS_RES: plus the residual, so
            // the collider stands where it is drawn).
            for (int k = 0; k < 3; ++k) {
                const double r = static_cast<double>(v.res[k]);
                nv.centre[k] =
                    static_cast<float>(static_cast<double>(v.centre[k]) - in->org[k] + r);
                nv.bs_c[k] = static_cast<float>(static_cast<double>(v.bs_c[k]) - in->org[k] + r);
                nv.prev.centre[k] =
                    static_cast<float>(static_cast<double>(v.prev.centre[k]) - in->org[k] + r);
                nv.res[k] = 0.0f;
            }
            for (size_t s = 0; s < seen.size(); ++s) {
                const PhysicsColliderView& sv = seen[s];
                if (sv.slot != nv.slot || sv.mesh != nv.mesh) continue;
                ClothFrame pf;
                memcpy(pf.centre, sv.centre, sizeof(pf.centre));
                memcpy(pf.rot, sv.rot, sizeof(pf.rot));
                memcpy(pf.size, sv.size, sizeof(pf.size));
                pf.rotated = sv.rotated;
                // Where its velocity at the last step would have carried it.
                ClothFrame pp = pf;
                for (int k = 0; k < 3; ++k) pp.centre[k] += sv.vel[k] * dt_now;
                const bool jumped = cloth_frame_jumped(pp, nv.prev, tele);
                // Its velocity now, jumped or not (as the carrier's): a collider past the teleport distance per
                // frame would otherwise never have one and be taken as still for good. A real jump's velocity makes
                // the next frame miss once more, then holds still.
                if (dt_now > 0.0f) {
                    for (int k = 0; k < 3; ++k) nv.vel[k] = (nv.centre[k] - pf.centre[k]) / dt_now;
                }
                if (!jumped) {
                    cloth_frame_lerp(sv.prev, pf, in->st.sub_t, nv.prev);   // SUB_CLOCK.
                    if (nv.skinned) {
                        // SKEL: the BVH is this frame's pose alone, so the sweep across the substeps is the
                        // ROOT's motion carrying it: last frame's root (its centre less its own box centre) with
                        // this frame's box centre and size. Do not interpolate the boxes themselves: that moves and
                        // scales the whole body by however the pose's box changed. SKIN_COL_PREV: last frame's
                        // centre less ITS offset from the root - on the first skinned frame after the rest mesh
                        // stood in, that is the rest box's centre, not zero (skin_ctr is zero there).
                        float bd[3];
                        for (int k = 0; k < 3; ++k) bd[k] = nv.root_off[k] - sv.root_off[k];
                        for (int k = 0; k < 3; ++k) {
                            nv.prev.centre[k] += pf.rotated
                                ? bd[0] * pf.rot[k] + bd[1] * pf.rot[3 + k] + bd[2] * pf.rot[6 + k]
                                : bd[k];
                        }
                        memcpy(nv.prev.size, nv.size, sizeof(nv.prev.size));
                    }
                }
                break;
            }
            in->in_col_own.push_back(g_physics_col[c].own);
        }

        // AFFECTOR: the affectors in reach, into this instance's frame in double like the colliders. The reach
        // test is the colliders' (the rest bound plus 'range'); an unbounded affector always reaches.
        in->in_aff.clear();
        for (size_t a = 0; a < g_aff_frame.size(); ++a) {
            const AffFrame& af = g_aff_frame[a];
            if (af.reach >= 0.0f) {
                double dd = 0.0;
                for (int k = 0; k < 3; ++k) {
                    const double e = af.c[k] - static_cast<double>(wc[k]);
                    dd += e * e;
                }
                const double rr = static_cast<double>(af.reach) + static_cast<double>(reach);
                if (dd > rr * rr) continue;
            }
            in->in_aff.push_back(af.v);
            ClothAffView& av = in->in_aff.back();
            for (int k = 0; k < 3; ++k) av.c[k] = static_cast<float>(af.c[k] - in->org[k]);
        }

        {
            std::lock_guard<std::mutex> g(g_cloth_mu);
            g_cloth_q.push_back(in);
            // POOL_BUSY_AFTER_PUSH: busy only once the job is queued - set before a push that throws (out of
            // memory), nothing clears it and the instance never steps (nor is erased) again. Under the lock every pop
            // takes, so no worker holds the job before the flag is set.
            in->busy.store(true, std::memory_order_release);
            g_cloth_q_n.store(static_cast<uint32_t>(g_cloth_q.size()), std::memory_order_relaxed);
        }
        g_cloth_cv.notify_one();
    }
    g_simlod.work.store(lwork, std::memory_order_relaxed);   // SIM_LOD.
    g_simlod.full.store(lfull, std::memory_order_relaxed);
}
// SESSION_RESET: outside the function so the session reset reaches it.
static double g_cloth_dt_last = 0.0;
// The frame delta cloth_sync steps on, from the effect clock (game thread, flush_frame): its contract - steady wall
// time, the clock's age on a session's first frame, 0 after a gap over 10 s - is the note above effect_time_seconds_d's
// declaration in rendering/physics_affectors.hpp.
inline float cloth_frame_dt() {
    const double now = effect_time_seconds_d();
    const double d = now - g_cloth_dt_last;
    g_cloth_dt_last = now;
    if (!(d > 0.0) || d > 10.0) return 0.0f;
    return static_cast<float>(d);
}

// Called by the upload once its walk is done (before its box puts), so a reader sees a whole frame's counters, never a
// half-updated set.
inline void cloth_stats_publish(uint64_t inst, uint64_t ch_inst, uint64_t cols) {
    g_stats.cloth_instances = inst;
    g_stats.cloth_skipped = g_cloth_skipped.load(std::memory_order_relaxed);
    g_stats.physics_colliders = cols;
    g_stats.chain_instances = ch_inst;
    g_stats.chain_skipped = g_chain_skipped.load(std::memory_order_relaxed);
    g_stats.simlod_work = g_simlod.work.load(std::memory_order_relaxed);   // SIM_LOD.
    g_stats.simlod_full = g_simlod.full.load(std::memory_order_relaxed);
}

// CLOTH_PIN_FRESH: the front buffer with the guide-bound part re-placed from a guide (pf_g, nb maps in
// skin_affine_of's layout) into pin_out, in the front's frame - the cloth's hard pins (w == 0) exactly, its
// pin/sim gradient in proportion (CLOTH_PIN_GRADE, ClothInst::out_guide), or a chain mesh re-skinned
// (positions, normals and tangents) with fresh kinematic and guide-mapped bone maps and the job's dynamic ones.
// False when there is nothing to re-place (the front uploads as is). Under g_cloth_mu, from cloth_upload and
// cloth_pin_draw_step; reads only what the job does not write past the build (a particle's w and att among it),
// plus out / box_* / ch_mat / out_guide at front. cloth_pin_fresh feeds cloth_pin_place (below) the Draw3D guide.
inline bool cloth_pin_place(ClothInst& in, uint32_t f, const float* pf_g, size_t nb) {
    const std::vector<MeshVertex>& src = in.out[f];
    const size_t nv = src.size();
    if (nb == 0u || nv == 0u || nv != in.rest.size()) return false;
    const float* bc = in.box_ctr[f];
    const float* be = in.box_size[f];
    if (in.chain) {
        const ChainState& c = in.ch;
        const size_t nbone = c.body_of.size();
        const std::vector<float>& jm = in.ch_mat[f];
        if (nbone == 0u || nb != nbone || jm.size() != nbone * 12u || !in.box_on[f]) return false;
        in.pin_mat = jm;
        in.pin_g.assign(pf_g, pf_g + nb * 12u);   // CHAIN_END_DRAW: an end held on a bone.
        bool any = false;
        for (size_t bi = 0; bi < nbone; ++bi) {
            const int32_t bd = c.body_of[bi];
            const float* off = nullptr;
            if (bd >= 0) {
                if (static_cast<size_t>(bd) >= c.body.size() || c.body[static_cast<size_t>(bd)].inv_m > 0.0f) continue;   // Dynamic: the job's.
                off = chain_start_off(c, static_cast<size_t>(bd));
            }
            float q[4], t[3];
            chain_bone_map(c, pf_g, pf_g, nb, static_cast<int32_t>(bi), 1.0f, q, t);
            if (off) for (int k = 0; k < 3; ++k) t[k] += off[k];   // As chain_place, undone by the writeback: bt + off.
            q_to_axes(q, &in.pin_mat[bi * 12u]);
            memcpy(&in.pin_mat[bi * 12u + 9u], t, sizeof(t));
            any = true;
        }
        if (!any) return false;
        in.pin_out = src;
        const bool inf = c.inf.size() == nv;
        for (size_t i = 0; i < nv; ++i) {
            float pp[3], nn[3], tt[3];
            chain_skin_vert(in.rest[i], c.ext, inf ? &c.inf[i] : nullptr, in.pin_mat.data(), nbone, pp, nn, tt);
            // Position, normal and tangent from the same maps, stored as the writeback stores them (a re-placed
            // bone's surface turns with it).
            MeshVertex& o = in.pin_out[i];
            float n[3], tn[3];
            for (int k = 0; k < 3; ++k) {
                o.pos[k] = (pp[k] - (bc[k] - c.auth_ctr[k])) / be[k];
                n[k] = nn[k] * be[k];
                tn[k] = tt[k] / be[k];
            }
            const float nl = cloth_v3_len(n), tl = cloth_v3_len(tn);
            for (int k = 0; k < 3; ++k) {
                o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : in.rest[i].nrm[k];
                o.tan[k] = tl > 1.0e-30f ? tn[k] / tl : in.rest[i].tan[k];
            }
        }
        return true;
    }
    const size_t np = in.st.part.size();
    if (in.part_inf.size() != np || in.vmap.size() != nv) return false;
    float is[3], s[3];
    for (int k = 0; k < 3; ++k) {
        s[k] = fabsf(in.in_size[k]) < 1.0e-6f ? 1.0e-6f : in.in_size[k];
        is[k] = 1.0f / s[k];
    }
    const bool boxed = in.box_on[f];
    // CLOTH_PIN_GRADE: the front's own guide, when it has one of this bone count and the bones have moved since
    // (standing still grades nothing).
    const std::vector<float>& og = in.out_guide[f];
    const bool grade = og.size() == nb * 12u &&
                       memcmp(og.data(), pf_g, nb * 12u * sizeof(float)) != 0;
    bool any = false;
    for (size_t i = 0; i < nv; ++i) {
        const uint32_t p = in.vmap[i];
        if (p >= np) continue;
        if (in.st.part[p].w > 0.0f) {
            // A gradient particle (att > 0; att and w are the build's, never the job's): the solver's own position,
            // carried along by (1 - w) of its target's move. att = S w / (1 - w), so 1 - w = S / (S + att).
            const float att = in.st.part[p].att;
            if (!grade || !(att > 0.0f)) continue;   // Fully simulated: the job's, as it is.
            float g1[3], g0[3];
            cloth_guide_at(in, pf_g, nb, is, p, g1);
            cloth_guide_at(in, og.data(), nb, is, p, g0);
            const float pf_k = CLOTH_ATT_SOFT / (CLOTH_ATT_SOFT + att);
            float d[3];
            for (int k = 0; k < 3; ++k) d[k] = (g1[k] - g0[k]) * pf_k;
            if (!(d[0] == d[0] && d[1] == d[1] && d[2] == d[2]) || (d[0] == 0.0f && d[1] == 0.0f && d[2] == 0.0f)) continue;
            if (!any) { in.pin_out = src; any = true; }
            // A difference of two guided positions: the box's centre cancels.
            for (int k = 0; k < 3; ++k) in.pin_out[i].pos[k] += boxed ? d[k] * s[k] / be[k] : d[k];
            continue;
        }
        if (!any) { in.pin_out = src; any = true; }
        float g[3];
        cloth_guide_at(in, pf_g, nb, is, p, g);
        if (boxed) {   // As cloth_box_out: metres over the size, then into the front's box.
            for (int k = 0; k < 3; ++k) g[k] = (g[k] * s[k] - (bc[k] - in.auth_ctr[k])) / be[k];
        }
        memcpy(in.pin_out[i].pos, g, sizeof(g));
    }
    return any;
}
// CLOTH_STEP_SHAPE - the key of bytes a consume-point writer made from a front rather than the front itself
// (cloth_pin_seal: the pin re-placement; chain_end_draw: the chain end's correction), by CLOTH_SHAPE's rule
// on metric positions, as chain_writeback keys a front (through this helper too): a new key from the process-wide
// serial when some vertex has left the shape that took key by more than CLOTH_SHAPE_M on any axis, else
// key again. Under the front's key the caching passes (the footprint seam, the sun ladder, the point-light maps)
// would keep what the writer moved where the job left it while the job's front stands.
inline uint32_t cloth_step_shape(std::vector<float>& ref, uint32_t& key,
                                 const std::vector<float>& met) {
    bool moved = key == 0u || ref.size() != met.size();
    for (size_t i = 0; i < met.size() && !moved; ++i) {
        if (!(fabsf(met[i] - ref[i]) <= CLOTH_SHAPE_M)) moved = true;
    }
    if (moved) {
        ref = met;
        uint32_t k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (k == 0u) k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        key = k;
    }
    return key;
}
// CLOTH_STEP_SHAPE - pin_out (cloth_pin_place's re-placement of front f) sealed: in the smallest box
// centred on the front's that holds its level-0 vertices and is never smaller than the front's (the re-placed part
// can leave the front's box, and the object's bounds are the box of its bytes) - positions, normals and tangents
// re-expressed in it when it grew, under KhVsCore's convention - and under its own key (cloth_step_shape). Level
// 0 is the front's own box rule: a chain's lvl0, a cloth's particles. A front with no box keeps the front's box and
// key. Returns the key; be: the box the bytes are in. Under g_cloth_mu.
inline uint32_t cloth_pin_seal(ClothInst& in, uint32_t f, float be[3]) {
    const float* const bf = in.box_size[f];
    memcpy(be, bf, sizeof(float) * 3u);
    if (!in.box_on[f]) return in.shape_of[f];
    std::vector<MeshVertex>& v = in.pin_out;
    const size_t nv = v.size();
    float bc[3];   // The box's centre, carrier-local (the writeback's metric frame).
    for (int k = 0; k < 3; ++k) bc[k] = in.box_ctr[f][k] - in.auth_ctr[k];
    const std::vector<uint8_t>& lv = in.ch.lvl0;
    const size_t np = in.st.part.size();
    float r[3] = { 0.0f, 0.0f, 0.0f };   // The farthest level-0 vertex from the centre, per axis.
    for (size_t i = 0; i < nv; ++i) {
        const bool l0 = in.chain ? (lv.size() != nv || lv[i] != 0u)
                                           : (i < in.vmap.size() && in.vmap[i] < np);
        if (!l0) continue;
        for (int k = 0; k < 3; ++k) {
            const float d = fabsf(v[i].pos[k]) * bf[k];
            if (d > r[k]) r[k] = d;
        }
    }
    bool grew = false;
    for (int k = 0; k < 3; ++k) {
        if (2.0f * r[k] > bf[k]) { be[k] = 2.0f * r[k]; grew = true; }
    }
    if (grew) {
        for (size_t i = 0; i < nv; ++i) {
            MeshVertex& o = v[i];
            float n[3], tn[3];
            for (int k = 0; k < 3; ++k) {
                o.pos[k] = o.pos[k] * bf[k] / be[k];
                n[k] = o.nrm[k] * (be[k] / bf[k]);
                tn[k] = o.tan[k] * (bf[k] / be[k]);
            }
            const float nl = cloth_v3_len(n), tl = cloth_v3_len(tn);
            for (int k = 0; k < 3; ++k) {
                if (nl > 1.0e-30f) o.nrm[k] = n[k] / nl;
                if (tl > 1.0e-30f) o.tan[k] = tn[k] / tl;
            }
        }
    }
    std::vector<float>& met = in.pin_met;
    met.resize(nv * 3u);
    for (size_t i = 0; i < nv; ++i) {
        for (int k = 0; k < 3; ++k) met[i * 3u + k] = v[i].pos[k] * be[k] + bc[k];
    }
    return cloth_step_shape(in.pin_shape_ref, in.pin_shape_key, met);
}
inline bool cloth_pin_fresh(ClothInst& in, uint32_t f) {
    if (!skin_guide_of(in.seq, in.mesh, in.pin_guide)) return false;
    return cloth_pin_place(in, f, in.pin_guide.data(), in.pin_guide.size() / 12u);
}
// cloth_upload, the device-side half (the flush's housekeeping): creates each instance's buffer, uploads newly
// published frames, resizes the slot tables and empties the grave.
inline void cloth_upload(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    KH_PROF_SCOPE(PROF_CLOTH_UPLOAD);   // PROF.
    if (!ctx || !dev) return;
    std::vector<std::shared_ptr<ClothInst>> live;
    uint32_t max_slot = 0;
    uint64_t inst = 0;
    // CHAIN: each plain chain's installed box, applied to its object in the flush that installs its buffer.
    uint64_t ch_inst = 0;
    struct ChainPut { uint32_t slot; uint64_t seq; int mesh; float size[3]; bool cast; };   // cast: CAST_REFIT (a plain cloth's cast_size).
    std::vector<ChainPut> ch_put;
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        for (ID3D11Buffer* b : g_cloth_grave) { if (b) b->Release(); }
        g_cloth_grave.clear();
        live.reserve(g_cloth.size());
        for (const auto& e : g_cloth) {
            live.push_back(e.second);
            if (e.first + 1u > max_slot) max_slot = e.first + 1u;
        }
    }
    // Render thread, or a park holding it still: every reader and writer of these six tables runs there (the draw
    // paths, through mesh_vb_ref, cloth_gen_of, lod_locked and the like; the three uploads; the draw-time
    // steps skin_draw_finish, cloth_pin_draw_step, chain_end_draw and uvs_step; skin_pool_rebind,
    // cloth_release_all, and cloth_slot_forget via scene_sync / uvs_release_all), so the resize cannot
    // invalidate a reader's pointer.
    std::fill(g_cloth_vb_slot.begin(), g_cloth_vb_slot.end(), nullptr);
    std::fill(g_cloth_gen_slot.begin(), g_cloth_gen_slot.end(), 0u);
    std::fill(g_cloth_mesh_slot.begin(), g_cloth_mesh_slot.end(), -1);
    std::fill(g_cloth_sim_slot.begin(), g_cloth_sim_slot.end(), static_cast<uint8_t>(0u));   // LOD_SIM_SLOT.
    std::fill(g_cloth_voff_slot.begin(), g_cloth_voff_slot.end(), 0u);   // SKIN_POOL.
    std::fill(g_cloth_pool_slot.begin(), g_cloth_pool_slot.end(), nullptr);
    if (g_cloth_vb_slot.size() < max_slot) g_cloth_vb_slot.resize(max_slot, nullptr);
    if (g_cloth_gen_slot.size() < max_slot) g_cloth_gen_slot.resize(max_slot, 0u);
    if (g_cloth_mesh_slot.size() < max_slot) g_cloth_mesh_slot.resize(max_slot, -1);
    if (g_cloth_sim_slot.size() < max_slot) g_cloth_sim_slot.resize(max_slot, 0u);
    if (g_cloth_voff_slot.size() < max_slot) g_cloth_voff_slot.resize(max_slot, 0u);   // SKIN_POOL.
    if (g_cloth_pool_slot.size() < max_slot) g_cloth_pool_slot.resize(max_slot, nullptr);

    // CLOTH_INST - the live instances' fields, under g_cloth_mu for the whole walk: cloth_sync rebuilds a
    // live chain in place (rest, in_boxed) and retires or erases instances (vb to the grave) while this loop may be
    // using that vb. Workers only pop the queue under this lock, so they wait at most one walk. Nested: g_skin_mu via
    // cloth_pin_fresh -> skin_guide_of.
    std::unique_lock<std::mutex> il(g_cloth_mu);
    for (size_t i = 0; i < live.size(); ++i) {
        ClothInst& in = *live[i];
        // Retired: its slot may already be another object's (cloth_sync).
        if (!in.built || in.rest.empty() || in.retired) continue;
        ++inst;   // A retired instance is not one; one whose buffer failed below still is.
        bool made = false;   // CLOTH_VB_FRESH (below).
        if (!in.vb) {
            D3D11_BUFFER_DESC bd = {};
            bd.ByteWidth = static_cast<UINT>(in.rest.size() * sizeof(MeshVertex));
            bd.Usage = D3D11_USAGE_DYNAMIC;
            bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(dev->CreateBuffer(&bd, nullptr, &in.vb))) { in.vb = nullptr; continue; }
            in.uploaded_gen = 0xFFFFFFFFu;   // Force the first write.
            made = true;
        }
        const uint32_t gen = in.gen.load(std::memory_order_acquire);
        const uint32_t f = in.front.load(std::memory_order_acquire);
        // CLOTH_PIN_FRESH: the guide-bound part from this frame's guide; the rest is the front's.
        // CLOTH_PIN_STEP: a buffer the consume-point step wrote this cycle keeps its bytes, key and box (this
        // upload's guide is older).
        // CLOTH_VB_FRESH: a buffer made above holds nothing yet (the step's bytes this cycle went to the one
        // cloth_release_all let go), so it is written whatever the stamp says (the write's vb_wr bump also stops
        // the chain end's keep test vouching for it).
        const bool stepped = !made && in.pin_step_cycle == g_topo_cycles;
        const bool fresh = !stepped && cloth_pin_fresh(in, f & 1u);
        if (!stepped && (gen != in.uploaded_gen || fresh)) {
            // CLOTH_STEP_SHAPE: a re-placement goes out in its own box, under its own key; the front, the front's.
            float be[3];
            memcpy(be, in.box_size[f & 1u], sizeof(be));
            const uint32_t key = fresh ? cloth_pin_seal(in, f & 1u, be)
                                                 : in.shape_of[f & 1u];
            const std::vector<MeshVertex>& src = fresh ? in.pin_out : in.out[f & 1u];
            D3D11_MAPPED_SUBRESOURCE m = {};
            if (SUCCEEDED(ctx->Map(in.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) && m.pData) {
                // The WHOLE array, LOD tail included: DISCARD hands back undefined memory, and a stray level-1 draw
                // would render it.
                memcpy(m.pData, src.data(), src.size() * sizeof(MeshVertex));
                ctx->Unmap(in.vb, 0);
                uvs_src_wrote(in.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again.
                ++in.vb_wr;   // CHAIN_END_DRAW (C3).
                in.uploaded_gen = gen;
                in.uploaded_shape = key;   // CLOTH_SHAPE: the bytes' (out[f]'s, or the re-placement's).
                // SKEL: and its box, which skin_upload hands the object.
                in.uploaded_box_on = in.box_on[f & 1u];
                memcpy(in.uploaded_box_ctr, in.box_ctr[f & 1u], sizeof(in.uploaded_box_ctr));
                memcpy(in.uploaded_box_size, be, sizeof(in.uploaded_box_size));
                memcpy(in.uploaded_cast_size, in.cast_size[f & 1u], sizeof(in.uploaded_cast_size));   // CAST_REFIT.
            }
        }
        if (in.slot < g_cloth_vb_slot.size()) g_cloth_vb_slot[in.slot] = in.vb;
        if (in.slot < g_cloth_gen_slot.size()) g_cloth_gen_slot[in.slot] = in.uploaded_shape;
        if (in.slot < g_cloth_mesh_slot.size()) g_cloth_mesh_slot[in.slot] = in.mesh;
        if (in.slot < g_cloth_sim_slot.size()) g_cloth_sim_slot[in.slot] = 1u;   // LOD_SIM_SLOT.
        if (in.slot < g_cloth_voff_slot.size()) g_cloth_voff_slot[in.slot] = 0u;   // SKIN_POOL: its own.
        if (in.slot < g_cloth_pool_slot.size()) g_cloth_pool_slot[in.slot] = nullptr;
        if (in.chain) {
            --inst;
            ++ch_inst;
            if (!in.in_boxed && in.uploaded_box_on && in.vb) {
                ChainPut p;
                p.slot = in.slot;
                p.seq = in.seq;
                p.mesh = in.mesh;
                p.size[0] = in.uploaded_box_size[0];   // SQF order, as RenderObject::size.
                p.size[1] = in.uploaded_box_size[2];
                p.size[2] = in.uploaded_box_size[1];
                p.cast = false;
                ch_put.push_back(p);
            }
        } else if (!in.in_boxed && in.vb && in.uploaded_gen != 0xFFFFFFFFu) {   // CAST_REFIT: a plain cloth (a buffer uploaded at least once).
            ChainPut p;
            p.slot = in.slot;
            p.seq = in.seq;
            p.mesh = in.mesh;
            p.size[0] = in.uploaded_cast_size[0];   // Engine axes -> SQF order, as the chain's put.
            p.size[1] = in.uploaded_cast_size[2];
            p.size[2] = in.uploaded_cast_size[1];
            p.cast = true;
            ch_put.push_back(p);
        }
    }
    il.unlock();   // Before the draw-list scope below.
    cloth_stats_publish(inst, ch_inst, g_physics_col_n.load(std::memory_order_relaxed));
    if (!ch_put.empty()) {
        // CHAIN: a plain chain's buffer is stored in a box about its rest centre, so its position stays the
        // script's and only its size follows (chain_writeback). A skeletal one's box is handed over with its
        // centre by skin_upload, next.
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& kv : g_draw_list) {
            RenderObject& o = kv.second;
            if (o.skel || o.slot == 0xFFFFFFFFu) continue;
            // CAST_REFIT: a plain cloth takes its cast_size; a plain chain its size.
            const bool plain = !o.chain_sim && cloth_obj_sim(o);
            if (!o.chain_sim && !plain) continue;
            for (const ChainPut& p : ch_put) {
                if (p.cast != plain) continue;
                if (p.slot != o.slot || p.seq != o.seq || p.mesh != o.mesh) continue;
                float* dst = p.cast ? o.cast_size : o.size;
                if (memcmp(dst, p.size, sizeof(o.size)) != 0) {
                    memcpy(dst, p.size, sizeof(o.size));
                    scene_mark(o.slot);
                }
                break;
            }
        }
    }
}

// Device teardown: every buffer goes and the slot tables empty (skin_release_all relies on it); workers keep
// running (they hold no device object). Under a park or on the render thread (on_engine_reset's best effort, after
// its lock attempts failed, is neither).
inline void cloth_release_all() {
    std::vector<ID3D11Buffer*> vbs;
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        // Taken out under the lock: cloth_sync's retire and refused rebuild move an instance's vb into the grave
        // under it too, so each buffer is released here or from the grave, never both. A worker never touches vb, so
        // a busy instance's buffer is still ours to release.
        for (const auto& e : g_cloth) {
            vbs.push_back(e.second->vb);
            e.second->vb = nullptr;
            e.second->uploaded_gen = 0xFFFFFFFFu;
        }
        for (ID3D11Buffer* b : g_cloth_grave) { if (b) b->Release(); }
        g_cloth_grave.clear();
    }
    for (ID3D11Buffer* b : vbs) { if (b) b->Release(); }
    std::fill(g_cloth_vb_slot.begin(), g_cloth_vb_slot.end(), nullptr);
    std::fill(g_cloth_gen_slot.begin(), g_cloth_gen_slot.end(), 0u);
    std::fill(g_cloth_mesh_slot.begin(), g_cloth_mesh_slot.end(), -1);
    std::fill(g_cloth_sim_slot.begin(), g_cloth_sim_slot.end(), static_cast<uint8_t>(0u));   // LOD_SIM_SLOT.
    std::fill(g_cloth_voff_slot.begin(), g_cloth_voff_slot.end(), 0u);   // SKIN_POOL.
    std::fill(g_cloth_pool_slot.begin(), g_cloth_pool_slot.end(), nullptr);
}

// Mission teardown, from the game thread under the park. The objects are gone too, so the instances go with their
// buffers rather than waiting for a flush to retire them (a mission that never draws again would never run one).
// Its one caller (session_teardown) has joined the workers first (cloth_workers_stop), so no job runs here; an
// instance whose queued job was dropped still reads busy and goes with the table. The per-mesh warning list
// follows the session's error dedupe, so a fresh mission re-reports.
inline void cloth_drop_all() {
    cloth_release_all();
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        g_cloth.clear();
    }
    g_physics_col.clear();
    g_physics_col_n.store(0, std::memory_order_relaxed);
    g_cloth_objs.clear();
    g_cloth_objs_h.clear();
    g_cloth_warned.clear();
}

// CLOTH_PIN_STEP - a skeletal binding's cloth, bound part at the consume point. cloth_upload's re-place runs
// at the scene resolve, after the injection, so on its own the mesh draws one frame behind its bones. This step
// re-places the same part from the maps of the stash just taken (skin_step_maps), at the upload's front in that
// front's box (grown to hold it, under its own key: cloth_pin_seal), writes the installed buffer and stamps the
// cycle (ClothInst::pin_step_cycle) so the upload leaves it. The simulated part is the job's front either way.
// From attach_step under g_draw_list_mutex; takes g_cloth_mu inside (list -> cloth) and never g_skin_mu.
inline void cloth_pin_draw_step(Attach& a, RenderObject& o, ID3D11DeviceContext* ctx, bool& moved) {
    const SkinInst* const in = a.skin.get();
    if (!ctx || !in || !in->cloth || o.slot == 0xFFFFFFFFu) return;
    if (!skin_step_maps(a, in, o)) return;
    bool box = false;
    float bc[3] = {}, bs[3] = {};
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        const auto it = g_cloth.find(o.slot);
        if (it == g_cloth.end()) return;
        ClothInst& cl = *it->second;
        if (cl.retired || !cl.built || cl.rest.empty() || !cl.vb ||
            cl.seq != o.seq || cl.mesh != in->mesh) return;
        // Installed by an upload already; until then a new instance draws the mesh's own vertices. KH_USER_VS:
        // installed too when the slot draws a vertex stage's buffer deformed from this one (uvs_draws_from) - the
        // table then names the stage's buffer; standing down for it would leave the bound part a frame behind its
        // bones.
        const bool direct = o.slot < g_cloth_vb_slot.size() && g_cloth_vb_slot[o.slot] == cl.vb;
        if (!direct && !uvs_draws_from(o.slot, cl.vb)) return;
        const uint32_t gen = cl.gen.load(std::memory_order_acquire);
        const uint32_t f = cl.front.load(std::memory_order_acquire) & 1u;
        if (!cloth_pin_place(cl, f, a.draw_a.data(), a.draw_a.size() / 12u)) return;   // Nothing bound: the upload's front stands.
        if (cl.pin_out.size() != cl.rest.size()) return;
        float be[3];   // CLOTH_STEP_SHAPE: the bytes' box and key.
        const uint32_t key = cloth_pin_seal(cl, f, be);
        D3D11_MAPPED_SUBRESOURCE m = {};
        if (FAILED(ctx->Map(cl.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) return;
        memcpy(m.pData, cl.pin_out.data(), cl.pin_out.size() * sizeof(MeshVertex));   // The whole array, as the upload.
        ctx->Unmap(cl.vb, 0);
        uvs_src_wrote(o.slot);   // KH_USER_VS: a vertex stage over this buffer evaluates again (uvs_step follows this step).
        ++cl.vb_wr;   // CHAIN_END_DRAW (C3).
        // What the upload records with a write: the generation, the key, the box - these bytes'.
        cl.uploaded_gen = gen;
        // SKIN_RESTEP_KEY (skin_draw_finish's rule): bytes of a new shape, against the key of the bytes this
        // buffer held (the upload's or the last step's), move the mesh, a vertex stage over it or not.
        if (cl.uploaded_shape != key) moved = true;
        cl.uploaded_shape = key;
        cl.uploaded_box_on = cl.box_on[f];
        memcpy(cl.uploaded_box_ctr, cl.box_ctr[f], sizeof(cl.uploaded_box_ctr));
        memcpy(cl.uploaded_box_size, be, sizeof(cl.uploaded_box_size));
        cl.pin_step_cycle = g_topo_cycles;
        cl.pin_front = f;   // CHAIN_END_DRAW.
        // Sized with g_cloth_vb_slot by the upload. Under a vertex stage the slot's key is the stage's (uvs_step
        // moves it with this copy), as skin_draw_finish leaves it.
        if (direct) g_cloth_gen_slot[o.slot] = cl.uploaded_shape;
        box = cl.uploaded_box_on;
        memcpy(bc, cl.uploaded_box_ctr, sizeof(bc));
        memcpy(bs, cl.uploaded_box_size, sizeof(bs));
    }
    // The box of the bytes just written, as skin_upload's cloth put hands it over (centre engine axes, size SQF
    // order). Unboxed, the rest box stands.
    if (!box) return;
    const float sz[3] = { bs[0], bs[2], bs[1] };
    if (memcmp(o.skel_ctr, bc, sizeof(bc)) != 0 || memcmp(o.size, sz, sizeof(sz)) != 0) {
        memcpy(o.skel_ctr, bc, sizeof(bc));
        memcpy(o.size, sz, sizeof(sz));
        moved = true;
    }
}
}