#pragma once

// The injection half (rendering_integration_injection.hpp): KH_CHAIN_END: the chain-end table and draw, the skeleton
// bone names and kh_chain_prepare. Never include it on its own.
namespace RenderIntegration {
// khbn_h / khbn_seq name the handle whose chain settings (g_chain_cfg) add their bones to keep; "" for a mesh with no
// handle yet (addRender3D), where no chain can have been configured. Defined further down this file
// (KH_SKEL_PROXY_CULL), after g_chain_cfg, which is in rendering/physics_state.hpp.
inline KhSkelBones kh_skel_bone_names(int khbn_mesh, const std::string& khbn_h = std::string(), uint64_t khbn_seq = 0);
// KH_CHAIN_END_DRAW - a chain's end that follows an object or a memory point is a physics target: the job holds
// the end where the Draw3D it was fed from put it, and on a terrain whose Draw3D lands after the frame's injection
// that is one frame older than the pose the frame draws. So each render-thread step with a device context re-places
// the end where THIS frame puts it (kh_chain_end_draw): the frame's own Draw3D targets by sample number (the ring,
// published by kh_chain_prepare), else the render thread's read of them (kh_chain_end_rt_read, KH_RT_PAINT_READ).
// An end fixed in the world (a world endPosition / endRotation, or one held where it first stood) is the same
// target every frame, but the job placed it in the frame the carrier had then, and the chain draws in the frame
// the carrier has now: the draw re-places it where the job held it, in the world. An end held on the bound
// skeleton's bone (true, on a skeletal binding) moves with the skeleton: the draw re-places it where the pin step's
// guide puts that bone, against where the job's guide did (kh_chain_end_bone). On any other object the bone's
// place is fixed on the carrier, and nothing is corrected.
// The end body and everything hanging past it take the whole correction (turned about the held point), the links
// between the start and the end a share growing along the chain - the target's motion since the Draw3D the job was
// fed from: a frame's when a job lands every frame, more while the job is still busy with an earlier one. Both
// targets are taken as the solver holds them, within the chain's reach of its start (kh_chain_end_taut). What is
// written is stored in the front's box grown to hold it (the object's bounds) and keyed by its own shape
// (kh_chain_end_store, kh_cloth_step_shape): the front's key would tell the caching passes nothing changed.
// A crew member's end keeps the Draw3D's measurement alone (KH_CREW_LAG): the render thread does not read it, and
// the final step waits for the frame's own sample (kh_chain_end_on_crew), as for a crew binding.
struct KhChainEndSmp {
    uint64_t seq = 0;                  // The Draw3D sample number (a chain's own number never matches a cycle).
    bool     p_ok = false, r_ok = false;
    double   p[3] = {};                // World, engine axes (KhChainCfg::live_pos).
    float    r[9] = {};                // Engine axes (KhChainCfg::live_rot).
};
struct KhChainEnd {
    uint64_t   seq = 0;                // The object (RenderObject::seq).
    uint8_t    pos_k = KH_CHE_FREE, rot_k = KH_CHE_FREE;
    game_value pos_obj, proxy, rot_obj;   // Game thread's (KhChainCfg's), read by the render thread under the mutex.
    KhChainEndSmp smp[KH_GTS_N];
    uint32_t   smp_w = 0;
    // Render thread: its own page caches and its read for cycle rt_cyc.
    KhChainRead rt_rd_p, rt_rd_r;
    uint64_t   rt_cyc = ~0ull;
    KhChainEndSmp rt;
};
static std::unordered_map<std::string, KhChainEnd>& g_chain_end = *(new std::unordered_map<std::string, KhChainEnd>());
// Does a chain file in the table: a lane followed or fixed in the world, or held on the bone of a skeletal object's
// skeleton. Under g_draw_list_mutex (the object's kind is the draw list's). Game thread (kh_chain_end_publish).
// KH_CHAIN_REFUSED_END: not a chain that builds nothing (KhChainCfg::nobuild - no skeleton, or its build refused):
// it has no instance, so nothing draws its end and nothing its end would move; filed, the render thread would read
// its ends every frame, the final step wait up to KH_GTS_CREW_CAP_MS for its crew sample, and every step take
// g_cloth_mu for it, all for nothing.
inline bool kh_chain_end_filed(const KhChainCfg& khef_c, const std::string& khef_h) {
    if (khef_c.nobuild) return false;   // KH_CHAIN_REFUSED_END.
    if (kh_chain_end_drawn(khef_c.pos) || kh_chain_end_drawn(khef_c.rot)) return true;
    if (khef_c.pos != KH_CHE_BONE && khef_c.rot != KH_CHE_BONE) return false;
    const auto khef_d = g_draw_list.find(khef_h);
    return khef_d != g_draw_list.end() && khef_d->second.seq == khef_c.seq && khef_d->second.skel;
}
// Game thread, at the end of kh_chain_prepare (khep_seq: that pass's sample number): every chain with a followed
// lane files this Draw3D's targets (a lane fixed in the world or held on a bone files nothing: the job and the pin
// step record those); a chain with none of them leaves the table.
inline void kh_chain_end_publish(uint64_t khep_seq) {
    std::lock_guard<std::mutex> khep_g(g_draw_list_mutex);
    for (auto khep_it = g_chain_end.begin(); khep_it != g_chain_end.end();) {
        const auto khep_c = g_chain_cfg.find(khep_it->first);
        if (khep_c == g_chain_cfg.end() || khep_c->second.seq != khep_it->second.seq ||
            !(kh_chain_end_filed(khep_c->second, khep_it->first))) {
            khep_it = g_chain_end.erase(khep_it);
        } else {
            ++khep_it;
        }
    }
    for (auto& khep_kv : g_chain_cfg) {
        const KhChainCfg& c = khep_kv.second;
        if (!kh_chain_end_filed(c, khep_kv.first)) continue;
        KhChainEnd& e = g_chain_end[khep_kv.first];
        if (e.seq != c.seq || e.pos_k != c.pos || e.rot_k != c.rot || e.pos_obj.data.get() != c.pos_obj.data.get() ||
            e.proxy.data.get() != c.proxy.data.get() || e.rot_obj.data.get() != c.rot_obj.data.get()) {
            e = KhChainEnd();   // A new object or new lanes: nothing earlier describes them.
            e.seq = c.seq;
            e.pos_k = c.pos;
            e.rot_k = c.rot;
            e.pos_obj = c.pos_obj;
            e.proxy = c.proxy;
            e.rot_obj = c.rot_obj;
        }
        KhChainEndSmp& s = e.smp[e.smp_w++ % KH_GTS_N];
        s.seq = khep_seq;
        s.p_ok = kh_chain_end_followed(c.pos) && c.live_p;
        s.r_ok = kh_chain_end_followed(c.rot) && c.live_r;
        memcpy(s.p, c.live_pos, sizeof(s.p));
        memcpy(s.r, c.live_rot, sizeof(s.r));
    }
    g_chain_end_n.store(static_cast<uint32_t>(g_chain_end.size()), std::memory_order_relaxed);
    if (g_gts_done_defer != 0u) {   // The sample and its chain targets together (g_gts_done_defer).
        g_gts_done_pub.store(g_gts_done_defer, std::memory_order_release);
        g_gts_done_defer = 0u;
    }
}
// KH_CREW_LAG - a chain end on a crew member: its position lane's object (or memory point's owner) or its own
// rotation object riding in a vehicle. The render thread does not read it (its drawn pose is the Draw3D's
// measurement, not a read of its state); kh_chain_end_crew_n counts it for g_gts_crew_n, so the final step waits
// for the frame's own sample as it does for a crew binding. Under g_draw_list_mutex (either thread).
inline bool kh_chain_end_on_crew(const KhChainEnd& khecr_e) {
    const KhCrewEnt* const khecr_c1 = kh_crew_of(khecr_e.pos_obj);
    const KhCrewEnt* const khecr_c2 = khecr_e.rot_k == KH_CHE_OBJ ? kh_crew_of(khecr_e.rot_obj) : nullptr;
    return (khecr_c1 && khecr_c1->in) || (khecr_c2 && khecr_c2->in);
}
// KH_CREW_LAG - the chain ends with a followed lane kh_chain_end_on_crew holds on a crew member (the Draw3D sampler,
// game thread, under g_draw_list_mutex; the table as the last kh_chain_end_publish left it).
inline uint32_t kh_chain_end_crew_n() {
    uint32_t khecn_n = 0;
    for (const auto& khecn_kv : g_chain_end) {
        const KhChainEnd& khecn_e = khecn_kv.second;
        if ((kh_chain_end_followed(khecn_e.pos_k) || kh_chain_end_followed(khecn_e.rot_k)) &&
            kh_chain_end_on_crew(khecn_e)) ++khecn_n;
    }
    return khecn_n;
}
// Render thread, from kh_rt_paint_read (under g_draw_list_mutex, the frame's own sample absent): each followed end
// read as kh_chain_prepare reads it - twice, taken only if both passes agree bit for bit - a memory point through
// the estimator with the frame's own number. A lane the newest Draw3D could not place and a dead object are not
// read (the draw then keeps what the job held); nor is a crew member's end (kh_chain_end_on_crew: the final step
// waits for its own sample instead).
inline void kh_chain_end_rt_read(uint64_t khcr_cyc, uint64_t khcr_own, int64_t khcr_q) {
    for (auto& khcr_kv : g_chain_end) {
        KhChainEnd& e = khcr_kv.second;
        const KhChainEndSmp* khcr_n = nullptr;
        for (const KhChainEndSmp& khcr_s : e.smp) {
            if (khcr_s.seq != 0u && (!khcr_n || khcr_s.seq > khcr_n->seq)) khcr_n = &khcr_s;
        }
        if (!khcr_n) continue;
        const bool khcr_wp = kh_chain_end_followed(e.pos_k) && khcr_n->p_ok;
        const bool khcr_wr = kh_chain_end_followed(e.rot_k) && khcr_n->r_ok;
        if (!khcr_wp && !khcr_wr) continue;
        const game_value& khcr_px = e.pos_k == KH_CHE_MEM ? e.proxy : e.pos_obj;
        const game_value& khcr_rx = e.rot_k == KH_CHE_MEM ? e.proxy : e.rot_obj;
        if (kh_chain_end_on_crew(e)) continue;   // KH_CREW_LAG: the final step waits for its own sample.
        if ((khcr_wp && kh_attach_obj_dead(khcr_px)) || (khcr_wr && kh_attach_obj_dead(khcr_rx))) continue;
        float khcr_a[24] = {}, khcr_b[24] = {};   // Position lane: p, r; rotation lane: p, r.
        auto khcr_read = [&](float* o) -> bool {
            if (khcr_wp && !kh_attach_raw(khcr_px, e.rt_rd_p.off, e.rt_rd_p.vb, e.rt_rd_p.bb, e.rt_rd_p.avb,
                                          e.rt_rd_p.abb, o, o + 3)) return false;
            if (khcr_wr && !kh_attach_raw(khcr_rx, e.rt_rd_r.off, e.rt_rd_r.vb, e.rt_rd_r.bb, e.rt_rd_r.avb,
                                          e.rt_rd_r.abb, o + 12, o + 15)) return false;
            return true;
        };
        if (!khcr_read(khcr_a) || !khcr_read(khcr_b) || memcmp(khcr_a, khcr_b, sizeof(khcr_a)) != 0) continue;
        KhChainEndSmp& r = e.rt;
        r.seq = khcr_own;
        r.p_ok = khcr_wp;
        r.r_ok = khcr_wr;
        if (khcr_wp) {
            float khcr_sd[3] = { 0.0f, 0.0f, 0.0f };
            if (e.pos_k == KH_CHE_MEM) kh_pxy_smooth_note(e.proxy, khcr_own, khcr_q, khcr_a, khcr_sd);
            r.p[0] = static_cast<double>(khcr_a[0]) + static_cast<double>(khcr_sd[0]);   // SQF order -> engine axes.
            r.p[1] = static_cast<double>(khcr_a[2]) + static_cast<double>(khcr_sd[2]);
            r.p[2] = static_cast<double>(khcr_a[1]) + static_cast<double>(khcr_sd[1]);
        }
        if (khcr_wr) memcpy(r.r, khcr_a + 15, sizeof(r.r));
        e.rt_cyc = khcr_cyc;
    }
}

// KH_SKEL_PROXY_CULL - the skeleton as the proxy filter (declared above). A bone has geometry when any
// level's vertex takes weight from it (skin_inf, parallel to verts). The chain settings of the handle, when it has any
// (a set that has been switched off keeps them), add their start bone (or the skeleton's first root, kh_chain_build's
// default) and their end bone (or the chain's tip when the start's subtree does not branch) to the keep set. GAME
// THREAD (staged lookup; g_chain_cfg is game thread only).
inline KhSkelBones kh_skel_bone_names(int khbn_mesh, const std::string& khbn_h, uint64_t khbn_seq) {
    KhSkelBones khbn_out;
    const MeshDef* khbn_d = kh_mesh_def_staged(khbn_mesh);
    if (!khbn_d) return khbn_out;
    const size_t khbn_nb = khbn_d->skin_bones.size();
    khbn_out.name.reserve(khbn_nb);
    khbn_out.parent.reserve(khbn_nb);
    for (const KhSkinBone& khbn_b : khbn_d->skin_bones) {
        khbn_out.name.push_back(khbn_b.name);
        khbn_out.parent.push_back(khbn_b.parent);
    }
    khbn_out.geom.assign(khbn_nb, 0u);
    if (khbn_d->skin_inf.size() == khbn_d->verts.size()) {
        for (const KhSkinInf& khbn_f : khbn_d->skin_inf) {
            for (int khbn_j = 0; khbn_j < 4; ++khbn_j) {
                if (khbn_f.w[khbn_j] > 0.0f && khbn_f.b[khbn_j] < khbn_nb) khbn_out.geom[khbn_f.b[khbn_j]] = 1u;
            }
        }
    }
    if (khbn_h.empty() || khbn_nb == 0u) return khbn_out;
    const auto khbn_ci = g_chain_cfg.find(khbn_h);
    if (khbn_ci == g_chain_cfg.end() || khbn_ci->second.seq != khbn_seq) return khbn_out;
    const KhChainCfg& khbn_c = khbn_ci->second;
    // The start, as kh_chain_build resolves it.
    int32_t khbn_s = -1;
    if (khbn_c.start.empty()) {
        for (size_t khbn_b = 0; khbn_b < khbn_nb; ++khbn_b) if (khbn_out.parent[khbn_b] < 0) { khbn_s = static_cast<int32_t>(khbn_b); break; }
    } else {
        for (size_t khbn_b = 0; khbn_b < khbn_nb; ++khbn_b) if (khbn_out.name[khbn_b] == khbn_c.start) { khbn_s = static_cast<int32_t>(khbn_b); break; }
    }
    if (khbn_s < 0) return khbn_out;   // The chain will not build; nothing to keep for it.
    khbn_out.keep.push_back(khbn_out.name[static_cast<size_t>(khbn_s)]);
    if (!khbn_c.end.empty()) {
        khbn_out.keep.push_back(khbn_c.end);   // Below the start or not, the build decides; a proxy for it costs one.
        return khbn_out;
    }
    // The default end: the tip of the start's subtree, when it does not branch.
    std::vector<std::vector<int32_t>> khbn_kids(khbn_nb);
    for (size_t khbn_b = 0; khbn_b < khbn_nb; ++khbn_b) {
        const int32_t khbn_p = khbn_out.parent[khbn_b];
        if (khbn_p >= 0 && static_cast<size_t>(khbn_p) < khbn_nb && khbn_p != static_cast<int32_t>(khbn_b)) khbn_kids[static_cast<size_t>(khbn_p)].push_back(static_cast<int32_t>(khbn_b));
    }
    std::vector<uint8_t> khbn_seen(khbn_nb, 0u);
    std::vector<int32_t> khbn_order;
    khbn_order.push_back(khbn_s);
    khbn_seen[static_cast<size_t>(khbn_s)] = 1u;
    bool khbn_branch = false;
    for (size_t khbn_i = 0; khbn_i < khbn_order.size(); ++khbn_i) {
        const std::vector<int32_t>& khbn_k = khbn_kids[static_cast<size_t>(khbn_order[khbn_i])];
        if (khbn_k.size() > 1u) khbn_branch = true;
        for (int32_t khbn_x : khbn_k) {
            if (khbn_seen[static_cast<size_t>(khbn_x)]) continue;
            khbn_seen[static_cast<size_t>(khbn_x)] = 1u;
            khbn_order.push_back(khbn_x);
        }
    }
    if (!khbn_branch && khbn_order.size() >= 2u) khbn_out.keep.push_back(khbn_out.name[static_cast<size_t>(khbn_order.back())]);
    return khbn_out;
}

// KH_SKEL_PROXY_CULL - re-makes a bound skeleton's proxies when its chain settings change (chainSimulation, after
// g_chain_cfg took the new set): the keep set may have gained or lost a bone. Only when the kept memory points differ
// from the installed ones, so a stiffness change costs one SQF list read and no proxy. A rebuild runs
// kh_attach_skel_set's ordinary path (old proxies to the delete queue, skel_gen moves, the skin's caches and the stash
// start over). GAME THREAD, from a command entry point holding no lock (it makes SQF calls). False with err only when
// the make fails; a handle with no skeletal binding is true.
inline bool kh_skel_chain_rebind(const std::string& khcr_h, const RenderObject& khcr_o, std::string& err) {
    if (!khcr_o.skel) return true;
    game_value khcr_parent;
    std::vector<std::string> khcr_have;
    {
        std::lock_guard<std::mutex> khcr_g(g_draw_list_mutex);
        const auto khcr_it = g_attach.find(khcr_h);
        if (khcr_it == g_attach.end() || !khcr_it->second.skel) return true;
        khcr_parent = khcr_it->second.bone_parent;
        khcr_have = khcr_it->second.skel_mem;
    }
    if (!kh_attach_is_obj(khcr_parent) || kh_attach_obj_dead(khcr_parent)) return true;   // The step's death test owns this.
    const KhSkelBones khcr_b = kh_skel_bone_names(khcr_o.mesh, khcr_h, khcr_o.seq);
    std::vector<std::string> khcr_plan, khcr_raw;
    kh_skel_plan(static_cast<object>(khcr_parent), khcr_b, khcr_plan, khcr_raw);
    if (khcr_plan == khcr_have) return true;
    KhSkelOwn khcr_own;
    std::vector<std::string> khcr_mem;
    if (!kh_skel_make(khcr_parent, khcr_b, khcr_own.proxies, khcr_mem, err)) return false;
    kh_attach_skel_set(khcr_h, khcr_own.release(), khcr_mem, khcr_parent, kh_attach_bone_rot_state(khcr_h));
    return true;
}
// KH_CHAIN_END_DRAW - from kh_attach_step's end (render thread, g_draw_list_mutex held, every binding placed and
// skinned), at each step with a device context: every chain in g_chain_end whose frame targets are known - a followed
// lane's own Draw3D's by number, else the render thread's read for this cycle; a lane fixed in the world, the job's
// world target; a lane on a skeletal object's bone, the pin step's guide - each taken within the chain's reach
// (kh_chain_end_taut), is re-skinned from its front's bone maps (a skeletal one's: this cycle's pin re-placement,
// KH_CLOTH_PIN_STEP, which must have run) with the end corrected, in the frame the object draws at now
// (kh_cloth_carrier_at), and written as the pin step writes, in the front's box grown to hold it and under its own key
// (kh_chain_end_store / kh_cloth_step_shape); the object takes that box, as the upload's put would. Bytes it wrote
// whose inputs have not changed since, with no other write between, are kept (C3, KhClothInst::vb_wr). Takes
// g_cloth_mu (list -> cloth).
inline void kh_chain_end_draw(ID3D11DeviceContext* khed_ctx) {
    if (!khed_ctx || !reorder_on_render_thread() || g_chain_end.empty()) return;
    const uint64_t khed_cyc = g_topo_cycles;
    const uint64_t khed_want = static_cast<uint64_t>(static_cast<int64_t>(khed_cyc) + g_gid_k);
    for (auto& khed_kv : g_chain_end) {
        KhChainEnd& e = khed_kv.second;
        // A followed lane: this frame's target, when known (identity learned); a fixed one: the job's own.
        const KhChainEndSmp* khed_s = nullptr;
        if (g_gid_ok) {
            for (const KhChainEndSmp& khed_k : e.smp) if (khed_k.seq == khed_want && khed_k.seq != 0u) khed_s = &khed_k;
            if (!khed_s && e.rt_cyc == khed_cyc) khed_s = &e.rt;
        }
        const bool khed_wp = kh_chain_end_fixed(e.pos_k);   // Fixed in the world (not khed_fr: the carrier frame).
        const bool khed_wr = kh_chain_end_fixed(e.rot_k);
        const bool khed_bp = e.pos_k == KH_CHE_BONE;   // Held on the skeleton's bone (a skeletal object's alone).
        const bool khed_br = e.rot_k == KH_CHE_BONE;
        bool khed_tp = khed_wp || khed_bp || (khed_s && khed_s->p_ok && kh_chain_end_followed(e.pos_k));
        bool khed_tr = khed_wr || khed_br || (khed_s && khed_s->r_ok && kh_chain_end_followed(e.rot_k));
        if (!khed_tp && !khed_tr) continue;   // Nothing known for this frame: the job's end stands.
        const auto khed_it = g_draw_list.find(khed_kv.first);
        if (khed_it == g_draw_list.end() || khed_it->second.seq != e.seq || khed_it->second.slot == 0xFFFFFFFFu) {
            continue;
        }
        RenderObject& khed_o = khed_it->second;
        bool khed_wrote = false;
        bool khed_new = false;   // KH_CHAIN_END_NOTE: bytes written this call (not the kept ones).
        bool khed_cycw = false;  // KH_CHAIN_END_NOTE: the bytes drawn now were written this cycle.
        float khed_size[3] = {};
        bool khed_race_cycw = false;   // KH_CHAIN_END_RACE: skipped on a running job, over bytes written this cycle.
        // A do-while(false): every early exit leaves khed_wrote false, and the race skip still notes the bytes an
        // earlier step wrote (below).
        do {
            std::lock_guard<std::mutex> khed_g(g_cloth_mu);
            const auto khed_ci = g_cloth.find(khed_o.slot);
            if (khed_ci == g_cloth.end()) continue;
            KhClothInst& in = *khed_ci->second;
            if (!in.chain || in.retired || !in.built || in.rest.empty() || !in.vb || in.seq != khed_o.seq) continue;
            // KH_CHAIN_END_OWNER: the box below is the object's only while it simulates this chain on this mesh (the
            // upload's put tests the same two). A chain switched off, or a mesh changed, before the sync retires the
            // instance keeps its box off the object: the command already gave the object its own size back.
            if (!khed_o.chain_sim || in.mesh != khed_o.mesh) continue;
            const bool khed_direct = khed_o.slot < g_cloth_vb_slot.size() && g_cloth_vb_slot[khed_o.slot] == in.vb;
            if (!khed_direct && !kh_uvs_draws_from(khed_o.slot, in.vb)) continue;   // Not installed yet.
            // A skeletal chain's kinematic bones are this cycle's pin re-placement (KH_CLOTH_PIN_STEP, which must
            // have run), in the front it placed and whose box it handed the object; a plain chain's the front now.
            const bool khed_skel = khed_o.skel;
            if (khed_skel && in.pin_step_cycle != khed_cyc) continue;
            const uint32_t khed_gen = in.gen.load(std::memory_order_acquire);
            const uint32_t f = khed_skel ? (in.pin_front & 1u) : (in.front.load(std::memory_order_acquire) & 1u);
            // KH_CHAIN_END_RACE: a skeletal chain reads the front its pin step placed. Once a job has published past
            // it, the next job writes that buffer (its back) without the lock and the bytes below would be read
            // mid-write, so while one runs on it the job's end stands this frame, as on a frame with nothing known.
            // running, not busy: a queued job cannot start while g_cloth_mu is held (running is set at the pop, under
            // it), so that read goes ahead. Bytes an earlier step of this cycle wrote are still noted (khed_race_cycw).
            if (khed_skel && in.running.load(std::memory_order_acquire) &&
                (in.front.load(std::memory_order_acquire) & 1u) != f) {
                khed_race_cycw = in.end_new_cyc == khed_cyc;
                continue;
            }
            const KhChainState& c = in.ch;
            const size_t nb = c.body_of.size();
            const size_t nv = in.rest.size();
            if (c.end_body < 0 || nb == 0u || in.ch_mat[f].size() != nb * 12u || in.out[f].size() != nv ||
                !in.box_on[f]) continue;
            // A bone lane: the pin step's guide (this cycle's, above) against the job's; a skeletal object's alone.
            const bool khed_bg = khed_skel && in.pin_g.size() == nb * 12u;
            khed_tp = khed_tp && (khed_bp ? khed_bg : in.end_job_p[f]);
            khed_tr = khed_tr && (khed_br ? khed_bg : in.end_job_r[f]);
            if (!khed_tp && !khed_tr) continue;
            if (khed_skel && in.pin_mat.size() != nb * 12u) continue;
            float khed_bnq[4] = { 0.0f, 0.0f, 0.0f, 1.0f }, khed_bjq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float khed_bnp[3] = {}, khed_bjp[3] = {};
            // KH_CHAIN_REACH_ROT: the job's bone pose also for a bone ROTATION lane this frame does not follow (a
            // plain object: no guide, the rest - the orientation the solver held), so the position twin below clamps
            // as the solver did.
            if ((khed_tp && khed_bp) || (khed_tr && khed_br) || (khed_tp && khed_br)) {
                const std::vector<float>& khed_jg = in.out_guide[f];   // The job's as it held it (KH_CHAIN_HELD_REC), or none: the rest.
                kh_chain_end_bone(c, khed_jg.size() == nb * 12u ? khed_jg.data() : nullptr, nb, khed_bjq, khed_bjp);
                if (khed_tr || khed_bg) kh_chain_end_bone(c, in.pin_g.data(), nb, khed_bnq, khed_bnp);
            }
            // The frame the object draws at now, as kh_cloth_sync builds the job's.
            float khed_wc[3];
            double khed_wd[3];
            kh_cloth_carrier_at(khed_o, c.auth_ctr, khed_wc, khed_wd);
            KhClothFrame khed_fr;
            memcpy(khed_fr.rot, khed_o.rot_m, sizeof(khed_fr.rot));
            khed_fr.rotated = khed_o.rotated;
            float qf[4];
            kh_chain_frame_q(khed_fr, qf);
            const float qfc[4] = { -qf[0], -qf[1], -qf[2], qf[3] };
            float dp[3] = { 0.0f, 0.0f, 0.0f };
            float dq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            // This frame's rotation target, carrier-local (a followed kind: its frame's; a bone: this cycle's pin
            // guide's), and the job's - what the solver held (KH_CHAIN_REACH_ROT: the position twin reads them too).
            float khed_qn[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            if (khed_tr && !khed_br) {
                float qt[4];
                if (khed_wr) memcpy(qt, in.end_job_wq[f], sizeof(qt));
                else kh_q_from_axes(khed_s->r, qt);
                kh_q_mul(qfc, qt, khed_qn);
                kh_q_norm(khed_qn);
            }
            const float* const khed_qj = khed_br ? khed_bjq : (in.end_job_r[f] ? in.end_job_q[f] : nullptr);
            const float* const khed_qf = khed_tr ? (khed_br ? khed_bnq : khed_qn) : khed_qj;
            if (khed_tp) {
                float khed_tn[3], khed_tj[3];   // This frame's position target and the job's, carrier-local.
                if (khed_bp) {
                    memcpy(khed_tn, khed_bnp, sizeof(khed_tn));
                    memcpy(khed_tj, khed_bjp, sizeof(khed_tj));
                } else {
                    const double* const khed_tw =   // The target, in the world.
                        khed_wp ? static_cast<const double*>(in.end_job_w[f]) : static_cast<const double*>(khed_s->p);
                    const float d[3] = { static_cast<float>(khed_tw[0] - khed_wd[0]),
                                         static_cast<float>(khed_tw[1] - khed_wd[1]),
                                         static_cast<float>(khed_tw[2] - khed_wd[2]) };
                    kh_q_rot(qfc, d, khed_tn);
                    memcpy(khed_tj, in.end_job_x[f], sizeof(khed_tj));
                }
                // As the solver holds them: within the chain's reach of its pin - the job's where the job's maps put
                // the start, this frame's where this frame's do (the maps the correction is applied to, below).
                kh_chain_end_taut(c, in.ch_mat[f], khed_tj, khed_qj);
                kh_chain_end_taut(c, khed_skel ? in.pin_mat : in.ch_mat[f], khed_tn, khed_qf);
                for (int k = 0; k < 3; ++k) dp[k] = khed_tn[k] - khed_tj[k];
            }
            if (khed_tr && khed_br) {
                const float jc[4] = { -khed_bjq[0], -khed_bjq[1], -khed_bjq[2], khed_bjq[3] };
                kh_q_mul(khed_bnq, jc, dq);
                kh_q_norm(dq);
            } else if (khed_tr) {
                const float* jq = in.end_job_q[f];
                const float jc[4] = { -jq[0], -jq[1], -jq[2], jq[3] };
                kh_q_mul(khed_qn, jc, dq);
                kh_q_norm(dq);
            }
            if (!(dp[0] == dp[0] && dp[1] == dp[1] && dp[2] == dp[2] && dq[0] == dq[0] && dq[1] == dq[1] &&
                  dq[2] == dq[2] && dq[3] == dq[3])) continue;
            in.end_mat = khed_skel ? in.pin_mat : in.ch_mat[f];
            kh_chain_end_fix(c, in.end_mat, dp, dq, in.end_anc);
            // C3: the bytes this draw last wrote are kept (not skinned and written again) unless another write came
            // since or an input differs (the maps bit for bit, the front, its box).
            float khed_box[6];
            memcpy(khed_box, in.box_ctr[f], sizeof(float) * 3u);
            memcpy(khed_box + 3, in.box_size[f], sizeof(float) * 3u);
            const bool khed_keep = in.end_wr_ok && in.end_wr == in.vb_wr && in.end_vb == in.vb && in.end_f == f &&
                                   in.end_gen == khed_gen && in.end_mat_last.size() == in.end_mat.size() &&
                                   memcmp(in.end_mat_last.data(), in.end_mat.data(),
                                          in.end_mat.size() * sizeof(float)) == 0 &&
                                   memcmp(in.end_box, khed_box, sizeof(khed_box)) == 0;
            float be[3];
            if (khed_keep) {
                memcpy(be, in.end_be, sizeof(be));
            } else {
                // Skinned and stored as the writeback stores a shape: in the front's box, grown about its centre
                // where the corrected end leaves it (kh_chain_end_store).
                float bc[3];
                for (int k = 0; k < 3; ++k) bc[k] = in.box_ctr[f][k] - c.auth_ctr[k];
                in.end_out.resize(nv);
                kh_chain_end_store(in.rest.data(), nv, c.ext, c.inf.size() == nv ? c.inf.data() : nullptr,
                                   c.lvl0.size() == nv ? c.lvl0.data() : nullptr, in.end_mat.data(), nb, bc,
                                   in.box_size[f], in.end_met, in.end_out.data(), be);
                D3D11_MAPPED_SUBRESOURCE khed_m = {};
                if (FAILED(khed_ctx->Map(in.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khed_m)) || !khed_m.pData) continue;
                memcpy(khed_m.pData, in.end_out.data(), nv * sizeof(MeshVertex));   // The whole array, as the upload.
                khed_ctx->Unmap(in.vb, 0);
                kh_uvs_src_wrote(khed_o.slot);   // KH_USER_VS.
                khed_new = true;
                in.end_new_cyc = khed_cyc;   // KH_CHAIN_END_NOTE.
                in.uploaded_shape = kh_cloth_step_shape(in.end_shape_ref, in.end_shape_key, in.end_met);
                in.end_wr = ++in.vb_wr;
                in.end_wr_ok = true;
                in.end_vb = in.vb;
                in.end_f = f;
                in.end_gen = khed_gen;
                in.end_mat_last = in.end_mat;
                memcpy(in.end_box, khed_box, sizeof(khed_box));
                memcpy(in.end_be, be, sizeof(be));
            }
            // What the upload records with a write, for these bytes: their key (kh_cloth_step_shape: the key follows
            // the bytes) and their box (the front's centre, the grown size) - which the upload's chain put and
            // kh_skin_upload hand the object. A plain chain the rest too (KH_CLOTH_PIN_STEP's rule), so the upload
            // leaves this buffer this cycle; a skeletal one's the pin step recorded, for this same front.
            memcpy(in.uploaded_box_size, be, sizeof(in.uploaded_box_size));
            if (!khed_skel) {
                in.uploaded_gen = khed_gen;
                in.uploaded_box_on = in.box_on[f];
                memcpy(in.uploaded_box_ctr, in.box_ctr[f], sizeof(in.uploaded_box_ctr));
                in.pin_step_cycle = khed_cyc;
            }
            if (khed_direct) g_cloth_gen_slot[khed_o.slot] = in.uploaded_shape;
            khed_size[0] = be[0];   // SQF order, as the upload's put.
            khed_size[1] = be[2];
            khed_size[2] = be[1];
            khed_cycw = in.end_new_cyc == khed_cyc;   // KH_CHAIN_END_NOTE.
            khed_wrote = true;
        } while (false);
        if (!khed_wrote) {
            // KH_CHAIN_END_RACE: as the note below takes such bytes at a final or repick step.
            if (khed_race_cycw && (g_attach_final_step || g_attach_repick_step)) {
                kh_attach_moved_note(khed_o, true, true);
            }
            continue;
        }
        if (memcmp(khed_o.size, khed_size, sizeof(khed_size)) != 0) {   // Either kind: the box of its bytes.
            memcpy(khed_o.size, khed_size, sizeof(khed_size));
            khed_new = true;
        }
        // Re-filed; a final step's reopens the sun map (KH_SUN_RESTEP), a repick step's the fire's (KH_CAST_REPICK).
        // KH_CHAIN_END_NOTE: only when the bytes or the box changed, or - at a final or repick step - the bytes an
        // earlier step of this cycle wrote (a seam, mirror or in-front step writes a plain chain's newer front after
        // the cast fire built the sun map, and its note reopens nothing). Do not note bytes kept from an earlier cycle
        // in an unchanged box: the scene and the maps already hold them, and noting them re-runs the sun ladder and
        // the repick every frame for an idle chain.
        kh_attach_moved_note(khed_o, khed_new || ((g_attach_final_step || g_attach_repick_step) && khed_cycw), true);
    }
}

// KH_CREW_LAG (declared with the crew table): any chain set up; the characters a simulating chain's end follows -
// its object, its memory point's owner, its rotation object - for the census (a Man among them joins the table).
// Game thread; the second under g_draw_list_mutex (the draw list is read).
inline bool kh_crew_chain_any() { return !g_chain_cfg.empty(); }
inline void kh_crew_chain_cand(std::vector<std::pair<game_value, bool>>& khcc_out) {
    for (const auto& khcc_kv : g_chain_cfg) {
        const KhChainCfg& khcc_c = khcc_kv.second;
        const auto khcc_d = g_draw_list.find(khcc_kv.first);
        if (khcc_d == g_draw_list.end() || khcc_d->second.seq != khcc_c.seq || !kh_chain_obj_sim(khcc_d->second)) {
            continue;
        }
        if ((khcc_c.pos == KH_CHE_OBJ || khcc_c.pos == KH_CHE_MEM) && kh_attach_is_obj(khcc_c.pos_obj)) {
            khcc_out.emplace_back(khcc_c.pos_obj, false);
        }
        if (khcc_c.rot == KH_CHE_OBJ && kh_attach_is_obj(khcc_c.rot_obj)) khcc_out.emplace_back(khcc_c.rot_obj, false);
    }
}
// KH_CHAIN - GAME THREAD, from flush_frame ahead of the cloth step and ahead of its empty-list return (a removed
// object's proxy is deleted whether or not anything is left to draw), holding no lock across an engine read. Drops the
// settings of objects that are gone, handing their proxies to the delete queue, and reads every followed end: an
// object's transform, or a memory point's through its proxy, by the raw page-gated reads kh_attach_step uses - no
// script call. A memory-point proxy the attach has not yet moved off its spawn point fails the parent-reach test, as a
// skeletal binding's does, and the lane stays free that frame. khcp_gts: this flush_frame's sampler took a sample
// (kh_attach_gt_snap's answer) - its number and stamp are this pass's.
inline void kh_chain_prepare(bool khcp_gts) {
    KH_PROF_SCOPE(KHP_CHAIN_PREPARE);   // KH_PROF.
    if (g_chain_cfg.empty()) {
        if (g_chain_end_n.load(std::memory_order_relaxed) != 0u) kh_chain_end_publish(0u);   // Empties the table.
        return;
    }
    std::vector<std::string> khcp_dead, khcp_read;
    {
        std::lock_guard<std::mutex> khcp_g(g_draw_list_mutex);
        for (auto& khcp_kv : g_chain_cfg) {
            const auto khcp_it = g_draw_list.find(khcp_kv.first);
            if (khcp_it == g_draw_list.end() || khcp_it->second.seq != khcp_kv.second.seq) {
                khcp_dead.push_back(khcp_kv.first);
                continue;
            }
            KhChainCfg& khcp_c = khcp_kv.second;
            khcp_c.live_p = false;
            khcp_c.live_r = false;
            // KH_CHAIN_REFUSED_END: the verdict, once per chain per Draw3D (a key built only while a refusal is filed),
            // for kh_chain_end_publish; the reads below take every chain. A mesh still loading is not a verdict: it
            // files, and builds once it lands.
            khcp_c.nobuild = (kh_mesh_alive(khcp_it->second.mesh) && !kh_chain_mesh_ok(khcp_it->second.mesh)) ||
                             kh_chain_refused(khcp_kv.first, khcp_it->second);
            const bool khcp_follow = khcp_c.pos == KH_CHE_OBJ || khcp_c.pos == KH_CHE_MEM ||
                                     khcp_c.rot == KH_CHE_OBJ || khcp_c.rot == KH_CHE_MEM;
            if (khcp_follow && kh_chain_obj_sim(khcp_it->second)) khcp_read.push_back(khcp_kv.first);
        }
    }
    for (const std::string& khcp_h : khcp_dead) {
        const auto khcp_it = g_chain_cfg.find(khcp_h);
        if (khcp_it == g_chain_cfg.end()) continue;
        kh_attach_proxy_orphan(khcp_it->second.proxy);   // Takes g_draw_list_mutex itself.
        g_chain_cfg.erase(khcp_it);
    }
    // KH_PXY_SMOOTH: this pass's sample. When the Draw3D sampler took one in this flush_frame (it runs first): its
    // number and stamp, so a proxy a binding shares steps once (the entry hands the sampler's result back) and one
    // it does not takes one step at the frame's stamp. When it took none, a sample of the chain's own
    // (KH_PXY_SEQ_CHAIN). Key on the sampler's run, not on the binding count: a binding the census expires after the
    // sampler stepped a shared proxy leaves the count at zero, and a sample of the chain's own then steps the entry
    // twice in one pass.
    const int64_t khcp_q = khcp_gts ? g_gts_stamp_pub[g_gts_seq % KH_GTS_STAMPS].load(std::memory_order_relaxed)
                                    : kh_gts_qpc();
    const uint64_t khcp_seq = khcp_gts ? g_gts_seq : (KH_PXY_SEQ_CHAIN | static_cast<uint64_t>(khcp_q));
    for (const std::string& khcp_h : khcp_read) {
        const auto khcp_it = g_chain_cfg.find(khcp_h);
        if (khcp_it == g_chain_cfg.end()) continue;
        KhChainCfg& khcp_c = khcp_it->second;
        float khcp_p[3], khcp_r[9];
        bool khcp_mem_ok = false;
        float khcp_mr[9];
        if (khcp_c.pos == KH_CHE_MEM || khcp_c.rot == KH_CHE_MEM) {
            float khcp_pp[3], khcp_pr[9];
            if (!kh_attach_obj_dead(khcp_c.pos_obj) && !kh_attach_obj_dead(khcp_c.proxy) &&
                kh_attach_raw(khcp_c.proxy, khcp_c.rd_pos.off, khcp_c.rd_pos.vb, khcp_c.rd_pos.bb, khcp_c.rd_pos.avb, khcp_c.rd_pos.abb, khcp_p, khcp_mr) &&
                kh_attach_raw(khcp_c.pos_obj, khcp_c.rd_par.off, khcp_c.rd_par.vb, khcp_c.rd_par.bb, khcp_c.rd_par.avb, khcp_c.rd_par.abb, khcp_pp, khcp_pr)) {
                // KH_CREW_LAG: the memory point moved by its character's correction, its reach tested against where
                // the character is drawn (its own state holds the get-in pose while it is crew).
                const KhCrewEnt* const khcp_cw = kh_crew_of(khcp_c.pos_obj);
                if (khcp_cw) {
                    kh_crew_apply(khcp_cw->t, khcp_cw->h, khcp_cw->dm, khcp_p, khcp_mr);
                    for (int k = 0; k < 3; ++k) khcp_pp[k] = static_cast<float>(khcp_cw->t[k]);
                }
                float khcp_d[3];
                for (int k = 0; k < 3; ++k) khcp_d[k] = khcp_p[k] - khcp_pp[k];
                khcp_mem_ok = khcp_d[0] * khcp_d[0] + khcp_d[1] * khcp_d[1] + khcp_d[2] * khcp_d[2] <=
                              KH_SKEL_REACH_M * KH_SKEL_REACH_M;
            }
            if (khcp_mem_ok && khcp_c.pos == KH_CHE_MEM) {
                // KH_PXY_SMOOTH: the memory point where the estimator has it (the proxy is the pool's), in double.
                float khcp_sd[3];
                kh_pxy_smooth_note(khcp_c.proxy, khcp_seq, khcp_q, khcp_p, khcp_sd);
                khcp_c.live_pos[0] = static_cast<double>(khcp_p[0]) + static_cast<double>(khcp_sd[0]);
                khcp_c.live_pos[1] = static_cast<double>(khcp_p[2]) + static_cast<double>(khcp_sd[2]);
                khcp_c.live_pos[2] = static_cast<double>(khcp_p[1]) + static_cast<double>(khcp_sd[1]);
                khcp_c.live_p = true;
            }
            if (khcp_mem_ok && khcp_c.rot == KH_CHE_MEM) {
                memcpy(khcp_c.live_rot, khcp_mr, sizeof(khcp_c.live_rot));
                khcp_c.live_r = true;
            }
        }
        if (khcp_c.pos == KH_CHE_OBJ && !kh_attach_obj_dead(khcp_c.pos_obj) &&
            kh_attach_raw(khcp_c.pos_obj, khcp_c.rd_pos.off, khcp_c.rd_pos.vb, khcp_c.rd_pos.bb, khcp_c.rd_pos.avb, khcp_c.rd_pos.abb, khcp_p, khcp_r)) {
            khcp_c.live_pos[0] = khcp_p[0]; khcp_c.live_pos[1] = khcp_p[2]; khcp_c.live_pos[2] = khcp_p[1];
            const KhCrewEnt* const khcp_cw = kh_crew_of(khcp_c.pos_obj);   // KH_CREW_LAG: where it is drawn.
            if (khcp_cw) {
                khcp_c.live_pos[0] = khcp_cw->t[0];
                khcp_c.live_pos[1] = khcp_cw->t[2];
                khcp_c.live_pos[2] = khcp_cw->t[1];
            }
            khcp_c.live_p = true;
        }
        if (khcp_c.rot == KH_CHE_OBJ && !kh_attach_obj_dead(khcp_c.rot_obj) &&
            kh_attach_raw(khcp_c.rot_obj, khcp_c.rd_rot.off, khcp_c.rd_rot.vb, khcp_c.rd_rot.bb, khcp_c.rd_rot.avb, khcp_c.rd_rot.abb, khcp_p, khcp_r)) {
            memcpy(khcp_c.live_rot, khcp_r, sizeof(khcp_c.live_rot));
            const KhCrewEnt* const khcp_cw = kh_crew_of(khcp_c.rot_obj);   // KH_CREW_LAG: turned as drawn.
            if (khcp_cw) memcpy(khcp_c.live_rot, khcp_cw->cr, sizeof(khcp_c.live_rot));
            khcp_c.live_r = true;
        }
        // A refused read holds the lane's last transform (KhChainCfg::read_p); a dead object frees it.
        const game_value& khcp_po = khcp_c.pos_obj;
        if (khcp_c.pos == KH_CHE_OBJ || khcp_c.pos == KH_CHE_MEM) {
            if (khcp_c.live_p) {
                memcpy(khcp_c.last_pos, khcp_c.live_pos, sizeof(khcp_c.last_pos));
                khcp_c.read_p = true;
            } else {
                if (khcp_c.read_p && !kh_attach_obj_dead(khcp_po)) {
                    memcpy(khcp_c.live_pos, khcp_c.last_pos, sizeof(khcp_c.live_pos));
                    khcp_c.live_p = true;
                } else {
                    khcp_c.read_p = false;
                }
            }
        }
        if (khcp_c.rot == KH_CHE_OBJ || khcp_c.rot == KH_CHE_MEM) {
            const game_value& khcp_ro = khcp_c.rot == KH_CHE_MEM ? khcp_po : khcp_c.rot_obj;
            if (khcp_c.live_r) {
                memcpy(khcp_c.last_rot, khcp_c.live_rot, sizeof(khcp_c.last_rot));
                khcp_c.read_r = true;
            } else {
                if (khcp_c.read_r && !kh_attach_obj_dead(khcp_ro)) {
                    memcpy(khcp_c.live_rot, khcp_c.last_rot, sizeof(khcp_c.live_rot));
                    khcp_c.live_r = true;
                } else {
                    khcp_c.read_r = false;
                }
            }
        }
    }
    kh_chain_end_publish(khcp_seq);   // KH_CHAIN_END_DRAW: this Draw3D's targets, by its sample number.
}

// Capture core, parameterized on the source RTV: ensure_scene_capture calls it with the bound target (its one caller).
// The caller keeps ownership of the RTV reference. KH_CAP_DEFER: khcap_copy false runs every check and (re)creation -
// the same verdict, the same target - and leaves the copy (a resolve, on an MSAA scene) to a later call.
inline std::string ensure_scene_capture_from(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                             ID3D11RenderTargetView* rtv, bool khcap_copy = true) {
    if (!rtv) return "no RTV";
    ID3D11Resource* res = nullptr;
    rtv->GetResource(&res);
    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv->GetDesc(&rtv_desc);
    if (!res) return "RTV has no resource";
    ID3D11Texture2D* src_tex = nullptr;
    HRESULT hr = res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&src_tex));
    res->Release();
    if (FAILED(hr) || !src_tex) return "RTV resource is not a Texture2D";
    D3D11_TEXTURE2D_DESC src_desc = {};
    src_tex->GetDesc(&src_desc);

    // Resolve target format must be non-typeless; the RTV view format is.
    DXGI_FORMAT capture_fmt = rtv_desc.Format;

    if (!g_res.scene_tex || g_res.scene_w != src_desc.Width ||
        g_res.scene_h != src_desc.Height || g_res.scene_fmt != capture_fmt) {
        g_res.release_scene_capture();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = src_desc.Width;
        td.Height = src_desc.Height;
        // Single-mip.
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = capture_fmt;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        hr = dev->CreateTexture2D(&td, nullptr, &g_res.scene_tex);
        if (FAILED(hr)) { src_tex->Release(); return "create scene capture tex " + hr_str(hr); }
        hr = dev->CreateShaderResourceView(g_res.scene_tex, nullptr, &g_res.scene_srv);
        if (FAILED(hr)) {
            g_res.release_scene_capture();
            src_tex->Release();
            return "create scene capture SRV " + hr_str(hr);
        }
        g_res.scene_w = src_desc.Width;
        g_res.scene_h = src_desc.Height;
        g_res.scene_fmt = capture_fmt;
    }

    if (khcap_copy) {   // KH_CAP_DEFER: false leaves the copy to a later call.
        if (src_desc.SampleDesc.Count > 1) {
            ctx->ResolveSubresource(g_res.scene_tex, 0, src_tex, 0, capture_fmt);
        } else {
            ctx->CopyResource(g_res.scene_tex, src_tex);
        }
    }

    src_tex->Release();
    return "";
}
}