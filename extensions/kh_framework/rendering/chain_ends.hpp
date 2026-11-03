#pragma once

// The injection half (rendering_integration_injection.hpp): CHAIN_END: the chain-end table and draw, the skeleton
// bone names and chain_prepare. Never include it on its own.
namespace RenderIntegration {
// CHAIN_END_DRAW - a chain's end that follows an object or a memory point is a physics target: the job holds
// the end where the Draw3D it was fed from put it, and on a terrain whose Draw3D lands after the frame's injection
// that is one frame older than the pose the frame draws. So each render-thread step with a device context re-places
// the end where THIS frame puts it (chain_end_draw): the frame's own Draw3D targets by sample number (the ring,
// published by chain_prepare), else the render thread's read of them (chain_end_rt_read, RT_PAINT_READ).
// An end fixed in the world (a world endPosition / endRotation, or one held where it first stood) is the same
// target every frame, but the job placed it in the frame the carrier had then, and the chain draws in the frame
// the carrier has now: the draw re-places it where the job held it, in the world. An end held on the bound
// skeleton's bone (true, on a skeletal binding) moves with the skeleton: the draw re-places it where the pin step's
// guide puts that bone, against where the job's guide did (chain_end_bone). On any other object the bone's
// place is fixed on the carrier, and nothing is corrected.
// The end body and everything hanging past it take the whole correction (turned about the held point), the links
// between the start and the end a share growing along the chain - the target's motion since the Draw3D the job was
// fed from: a frame's when a job lands every frame, more while the job is still busy with an earlier one. Both
// targets are taken as the solver holds them, within the chain's reach of its start (chain_end_taut). What is
// written is stored in the front's box grown to hold it (the object's bounds) and keyed by its own shape
// (chain_end_store, cloth_step_shape): the front's key would tell the caching passes nothing changed.
// A crew member's end keeps the Draw3D's measurement alone (CREW_LAG): the render thread does not read it, and
// the final step waits for the frame's own sample (chain_end_on_crew), as for a crew binding.
struct ChainEndSmp {
    uint64_t seq = 0;                  // The Draw3D sample number (a chain's own number never matches a cycle).
    bool     p_ok = false, r_ok = false;
    double   p[3] = {};                // World, engine axes (ChainCfg::live_pos).
    float    r[9] = {};                // Engine axes (ChainCfg::live_rot).
};
struct ChainEnd {
    uint64_t   seq = 0;                // The object (RenderObject::seq).
    uint8_t    pos_k = CHE_FREE, rot_k = CHE_FREE;
    game_value pos_obj, proxy, rot_obj;   // Game thread's (ChainCfg's), read by the render thread under the mutex.
    ChainEndSmp smp[GTS_N];
    uint32_t   smp_w = 0;
    // Render thread: its own page caches and its read for cycle rt_cyc.
    ChainRead rt_rd_p, rt_rd_r;
    uint64_t   rt_cyc = ~0ull;
    ChainEndSmp rt;
};
static std::unordered_map<std::string, ChainEnd>& g_chain_end = *(new std::unordered_map<std::string, ChainEnd>());
// Does a chain file in the table: a lane followed or fixed in the world, or held on the bone of a skeletal object's
// skeleton. Under g_draw_list_mutex (the object's kind is the draw list's). Game thread (chain_end_publish).
// CHAIN_REFUSED_END: not a chain that builds nothing (ChainCfg::nobuild - no skeleton, or its build refused):
// it has no instance, so nothing draws its end and nothing its end would move; filed, the render thread would read
// its ends every frame, the final step wait up to GTS_CREW_CAP_MS for its crew sample, and every step take
// g_cloth_mu for it, all for nothing.
inline bool chain_end_filed(const ChainCfg& c, const std::string& h) {
    if (c.nobuild) return false;   // CHAIN_REFUSED_END.
    if (chain_end_drawn(c.pos) || chain_end_drawn(c.rot)) return true;
    if (c.pos != CHE_BONE && c.rot != CHE_BONE) return false;
    const auto d = g_draw_list.find(h);
    return d != g_draw_list.end() && d->second.seq == c.seq && d->second.skel;
}
// Game thread, at the end of chain_prepare (seq: that pass's sample number): every chain with a followed
// lane files this Draw3D's targets (a lane fixed in the world or held on a bone files nothing: the job and the pin
// step record those); a chain with none of them leaves the table.
inline void chain_end_publish(uint64_t seq) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    for (auto it = g_chain_end.begin(); it != g_chain_end.end();) {
        const auto ep_c = g_chain_cfg.find(it->first);
        if (ep_c == g_chain_cfg.end() || ep_c->second.seq != it->second.seq ||
            !(chain_end_filed(ep_c->second, it->first))) {
            it = g_chain_end.erase(it);
        } else {
            ++it;
        }
    }
    for (auto& kv : g_chain_cfg) {
        const ChainCfg& c = kv.second;
        if (!chain_end_filed(c, kv.first)) continue;
        ChainEnd& e = g_chain_end[kv.first];
        if (e.seq != c.seq || e.pos_k != c.pos || e.rot_k != c.rot || e.pos_obj.data.get() != c.pos_obj.data.get() ||
            e.proxy.data.get() != c.proxy.data.get() || e.rot_obj.data.get() != c.rot_obj.data.get()) {
            e = ChainEnd();   // A new object or new lanes: nothing earlier describes them.
            e.seq = c.seq;
            e.pos_k = c.pos;
            e.rot_k = c.rot;
            e.pos_obj = c.pos_obj;
            e.proxy = c.proxy;
            e.rot_obj = c.rot_obj;
        }
        ChainEndSmp& s = e.smp[e.smp_w++ % GTS_N];
        s.seq = seq;
        s.p_ok = chain_end_followed(c.pos) && c.live_p;
        s.r_ok = chain_end_followed(c.rot) && c.live_r;
        memcpy(s.p, c.live_pos, sizeof(s.p));
        memcpy(s.r, c.live_rot, sizeof(s.r));
    }
    g_chain_end_n.store(static_cast<uint32_t>(g_chain_end.size()), std::memory_order_relaxed);
    if (g_gts.done_defer != 0u) {   // The sample and its chain targets together (g_gts.done_defer).
        g_gts.done_pub.store(g_gts.done_defer, std::memory_order_release);
        g_gts.done_defer = 0u;
    }
}
// CREW_LAG - a chain end on a crew member: its position lane's object (or memory point's owner) or its own
// rotation object riding in a vehicle. The render thread does not read it (its drawn pose is the Draw3D's
// measurement, not a read of its state); chain_end_crew_n counts it for g_gts.crew_n, so the final step waits
// for the frame's own sample as it does for a crew binding. Under g_draw_list_mutex (either thread).
inline bool chain_end_on_crew(const ChainEnd& e) {
    const CrewEnt* const c1 = crew_of(e.pos_obj);
    const CrewEnt* const c2 = e.rot_k == CHE_OBJ ? crew_of(e.rot_obj) : nullptr;
    return (c1 && c1->in) || (c2 && c2->in);
}
// CREW_LAG - the chain ends with a followed lane chain_end_on_crew holds on a crew member (the Draw3D sampler,
// game thread, under g_draw_list_mutex; the table as the last chain_end_publish left it).
inline uint32_t chain_end_crew_n() {
    uint32_t n = 0;
    for (const auto& kv : g_chain_end) {
        const ChainEnd& e = kv.second;
        if ((chain_end_followed(e.pos_k) || chain_end_followed(e.rot_k)) &&
            chain_end_on_crew(e)) ++n;
    }
    return n;
}
// Render thread, from rt_paint_read (under g_draw_list_mutex, the frame's own sample absent): each followed end
// read as chain_prepare reads it - twice, taken only if both passes agree bit for bit - a memory point through
// the estimator with the frame's own number. A lane the newest Draw3D could not place and a dead object are not
// read (the draw then keeps what the job held); nor is a crew member's end (chain_end_on_crew: the final step
// waits for its own sample instead).
inline void chain_end_rt_read(uint64_t cyc, uint64_t own, int64_t q) {
    for (auto& kv : g_chain_end) {
        ChainEnd& e = kv.second;
        const ChainEndSmp* n = nullptr;
        for (const ChainEndSmp& s : e.smp) {
            if (s.seq != 0u && (!n || s.seq > n->seq)) n = &s;
        }
        if (!n) continue;
        const bool wp = chain_end_followed(e.pos_k) && n->p_ok;
        const bool wr = chain_end_followed(e.rot_k) && n->r_ok;
        if (!wp && !wr) continue;
        const game_value& px = e.pos_k == CHE_MEM ? e.proxy : e.pos_obj;
        const game_value& rx = e.rot_k == CHE_MEM ? e.proxy : e.rot_obj;
        if (chain_end_on_crew(e)) continue;   // CREW_LAG: the final step waits for its own sample.
        if ((wp && attach_obj_dead(px)) || (wr && attach_obj_dead(rx))) continue;
        float a[24] = {}, b[24] = {};   // Position lane: p, r; rotation lane: p, r.
        auto read = [&](float* o) -> bool {
            if (wp && !attach_raw(px, e.rt_rd_p.off, e.rt_rd_p.vb, e.rt_rd_p.bb, e.rt_rd_p.avb,
                                  e.rt_rd_p.abb, o, o + 3)) return false;
            if (wr && !attach_raw(rx, e.rt_rd_r.off, e.rt_rd_r.vb, e.rt_rd_r.bb, e.rt_rd_r.avb,
                                  e.rt_rd_r.abb, o + 12, o + 15)) return false;
            return true;
        };
        if (!read(a) || !read(b) || memcmp(a, b, sizeof(a)) != 0) continue;
        ChainEndSmp& r = e.rt;
        r.seq = own;
        r.p_ok = wp;
        r.r_ok = wr;
        if (wp) {
            float sd[3] = { 0.0f, 0.0f, 0.0f };
            if (e.pos_k == CHE_MEM) pxy_smooth_note(e.proxy, own, q, a, sd);
            r.p[0] = static_cast<double>(a[0]) + static_cast<double>(sd[0]);   // SQF order -> engine axes.
            r.p[1] = static_cast<double>(a[2]) + static_cast<double>(sd[2]);
            r.p[2] = static_cast<double>(a[1]) + static_cast<double>(sd[1]);
        }
        if (wr) memcpy(r.r, a + 15, sizeof(r.r));
        e.rt_cyc = cyc;
    }
}

// SKEL_PROXY_CULL - the skeleton as the proxy filter. A bone has geometry when any level's vertex takes weight from
// it (skin_inf, parallel to verts). h / seq name the handle whose chain settings (g_chain_cfg) add their
// bones to keep; "" for a mesh with no handle yet (addRender3D), where no chain can have been configured. Those
// settings, when the handle has any (a set that has been switched off keeps them), add their start bone (or the
// skeleton's first root, chain_build's default) and their end bone (or the chain's tip when the start's subtree does
// not branch) to the keep set. GAME THREAD (staged lookup; g_chain_cfg is game thread only).
inline SkelBones skel_bone_names(int mesh, const std::string& h = std::string(),
                                 uint64_t seq = 0) {
    SkelBones out;
    const MeshDef* d = mesh_def_staged(mesh);
    if (!d) return out;
    const size_t nb = d->skin_bones.size();
    out.name.reserve(nb);
    out.parent.reserve(nb);
    for (const SkinBone& b : d->skin_bones) {
        out.name.push_back(b.name);
        out.parent.push_back(b.parent);
    }
    out.geom.assign(nb, 0u);
    if (d->skin_inf.size() == d->verts.size()) {
        for (const SkinInf& f : d->skin_inf) {
            for (int j = 0; j < 4; ++j) {
                if (f.w[j] > 0.0f && f.b[j] < nb) out.geom[f.b[j]] = 1u;
            }
        }
    }
    if (h.empty() || nb == 0u) return out;
    const auto ci = g_chain_cfg.find(h);
    if (ci == g_chain_cfg.end() || ci->second.seq != seq) return out;
    const ChainCfg& c = ci->second;
    // The start, as chain_build resolves it.
    int32_t s = -1;
    if (c.start.empty()) {
        for (size_t b = 0; b < nb; ++b) if (out.parent[b] < 0) { s = static_cast<int32_t>(b); break; }
    } else {
        for (size_t b = 0; b < nb; ++b) if (out.name[b] == c.start) { s = static_cast<int32_t>(b); break; }
    }
    if (s < 0) return out;   // The chain will not build; nothing to keep for it.
    out.keep.push_back(out.name[static_cast<size_t>(s)]);
    if (!c.end.empty()) {
        out.keep.push_back(c.end);   // Below the start or not, the build decides; a proxy for it costs one.
        return out;
    }
    // The default end: the tip of the start's subtree, when it does not branch.
    std::vector<std::vector<int32_t>> kids(nb);
    for (size_t b = 0; b < nb; ++b) {
        const int32_t p = out.parent[b];
        if (p >= 0 && static_cast<size_t>(p) < nb && p != static_cast<int32_t>(b)) kids[static_cast<size_t>(p)].push_back(static_cast<int32_t>(b));
    }
    std::vector<uint8_t> seen(nb, 0u);
    std::vector<int32_t> order;
    order.push_back(s);
    seen[static_cast<size_t>(s)] = 1u;
    bool branch = false;
    for (size_t i = 0; i < order.size(); ++i) {
        const std::vector<int32_t>& k = kids[static_cast<size_t>(order[i])];
        if (k.size() > 1u) branch = true;
        for (int32_t x : k) {
            if (seen[static_cast<size_t>(x)]) continue;
            seen[static_cast<size_t>(x)] = 1u;
            order.push_back(x);
        }
    }
    if (!branch && order.size() >= 2u) out.keep.push_back(out.name[static_cast<size_t>(order.back())]);
    return out;
}

// SKEL_PROXY_CULL - re-makes a bound skeleton's proxies when its chain settings change (chainSimulation, after
// g_chain_cfg took the new set): the keep set may have gained or lost a bone. Only when the kept memory points differ
// from the installed ones, so a stiffness change costs one SQF list read and no proxy. A rebuild runs
// attach_skel_set's ordinary path (old proxies to the delete queue, skel_gen moves, the skin's caches and the stash
// start over). GAME THREAD, from a command entry point holding no lock (it makes SQF calls). False with err only when
// the make fails; a handle with no skeletal binding is true.
inline bool skel_chain_rebind(const std::string& h, const RenderObject& o, std::string& err) {
    if (!o.skel) return true;
    game_value parent;
    std::vector<std::string> have;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        const auto it = g_attach.find(h);
        if (it == g_attach.end() || !it->second.skel) return true;
        parent = it->second.bone_parent;
        have = it->second.skel_mem;
    }
    if (!attach_is_obj(parent) || attach_obj_dead(parent)) return true;   // The step's death test owns this.
    const SkelBones b = skel_bone_names(o.mesh, h, o.seq);
    std::vector<std::string> plan, raw;
    skel_plan(static_cast<object>(parent), b, plan, raw);
    if (plan == have) return true;
    SkelOwn own;
    std::vector<std::string> mem;
    if (!skel_make(parent, b, own.proxies, mem, err)) return false;
    attach_skel_set(h, own.release(), mem, parent, attach_bone_rot_state(h));
    return true;
}
// CHAIN_END_DRAW - from attach_step's end (render thread, g_draw_list_mutex held, every binding placed and
// skinned), at each step with a device context: every chain in g_chain_end whose frame targets are known - a followed
// lane's own Draw3D's by number, else the render thread's read for this cycle; a lane fixed in the world, the job's
// world target; a lane on a skeletal object's bone, the pin step's guide - each taken within the chain's reach
// (chain_end_taut), is re-skinned from its front's bone maps (a skeletal one's: this cycle's pin re-placement,
// CLOTH_PIN_STEP, which must have run) with the end corrected, in the frame the object draws at now
// (cloth_carrier_at), and written as the pin step writes, in the front's box grown to hold it and under its own key
// (chain_end_store / cloth_step_shape); the object takes that box, as the upload's put would. Bytes it wrote
// whose inputs have not changed since, with no other write between, are kept (C3, ClothInst::vb_wr). Takes
// g_cloth_mu (list -> cloth).
inline void chain_end_draw(ID3D11DeviceContext* ctx) {
    if (!ctx || !reorder_on_render_thread() || g_chain_end.empty()) return;
    const uint64_t cyc = g_topo_cycles;
    const uint64_t want = static_cast<uint64_t>(static_cast<int64_t>(cyc) + g_gid_k);
    for (auto& kv : g_chain_end) {
        ChainEnd& e = kv.second;
        // A followed lane: this frame's target, when known (identity learned); a fixed one: the job's own.
        const ChainEndSmp* s = nullptr;
        if (g_gid_ok) {
            for (const ChainEndSmp& ed_k : e.smp) if (ed_k.seq == want && ed_k.seq != 0u) s = &ed_k;
            if (!s && e.rt_cyc == cyc) s = &e.rt;
        }
        const bool wp = chain_end_fixed(e.pos_k);   // Fixed in the world (not fr: the carrier frame).
        const bool wr = chain_end_fixed(e.rot_k);
        const bool bp = e.pos_k == CHE_BONE;   // Held on the skeleton's bone (a skeletal object's alone).
        const bool br = e.rot_k == CHE_BONE;
        bool tp = wp || bp || (s && s->p_ok && chain_end_followed(e.pos_k));
        bool tr = wr || br || (s && s->r_ok && chain_end_followed(e.rot_k));
        if (!tp && !tr) continue;   // Nothing known for this frame: the job's end stands.
        const auto it = g_draw_list.find(kv.first);
        if (it == g_draw_list.end() || it->second.seq != e.seq || it->second.slot == 0xFFFFFFFFu) {
            continue;
        }
        RenderObject& o = it->second;
        bool wrote = false;
        bool ed_new = false;   // CHAIN_END_NOTE: bytes written this call (not the kept ones).
        bool cycw = false;  // CHAIN_END_NOTE: the bytes drawn now were written this cycle.
        float size[3] = {};
        bool race_cycw = false;   // CHAIN_END_RACE: skipped on a running job, over bytes written this cycle.
        // A do-while(false): every early exit leaves wrote false, and the race skip still notes the bytes an
        // earlier step wrote (below).
        do {
            std::lock_guard<std::mutex> g(g_cloth_mu);
            const auto ci = g_cloth.find(o.slot);
            if (ci == g_cloth.end()) continue;
            ClothInst& in = *ci->second;
            if (!in.chain || in.retired || !in.built || in.rest.empty() || !in.vb || in.seq != o.seq) continue;
            // CHAIN_END_OWNER: the box below is the object's only while it simulates this chain on this mesh (the
            // upload's put tests the same two). A chain switched off, or a mesh changed, before the sync retires the
            // instance keeps its box off the object: the command already gave the object its own size back.
            if (!o.chain_sim || in.mesh != o.mesh) continue;
            const bool direct = o.slot < g_cloth_vb_slot.size() && g_cloth_vb_slot[o.slot] == in.vb;
            if (!direct && !uvs_draws_from(o.slot, in.vb)) continue;   // Not installed yet.
            // A skeletal chain's kinematic bones are this cycle's pin re-placement (CLOTH_PIN_STEP, which must
            // have run), in the front it placed and whose box it handed the object; a plain chain's the front now.
            const bool skel = o.skel;
            if (skel && in.pin_step_cycle != cyc) continue;
            const uint32_t gen = in.gen.load(std::memory_order_acquire);
            const uint32_t f = skel ? (in.pin_front & 1u) : (in.front.load(std::memory_order_acquire) & 1u);
            // CHAIN_END_RACE: a skeletal chain reads the front its pin step placed. Once a job has published past
            // it, the next job writes that buffer (its back) without the lock and the bytes below would be read
            // mid-write, so while one runs on it the job's end stands this frame, as on a frame with nothing known.
            // running, not busy: a queued job cannot start while g_cloth_mu is held (running is set at the pop, under
            // it), so that read goes ahead. Bytes an earlier step of this cycle wrote are still noted (race_cycw).
            if (skel && in.running.load(std::memory_order_acquire) &&
                (in.front.load(std::memory_order_acquire) & 1u) != f) {
                race_cycw = in.end_new_cyc == cyc;
                continue;
            }
            const ChainState& c = in.ch;
            const size_t nb = c.body_of.size();
            const size_t nv = in.rest.size();
            if (c.end_body < 0 || nb == 0u || in.ch_mat[f].size() != nb * 12u || in.out[f].size() != nv ||
                !in.box_on[f]) continue;
            // A bone lane: the pin step's guide (this cycle's, above) against the job's; a skeletal object's alone.
            const bool bg = skel && in.pin_g.size() == nb * 12u;
            tp = tp && (bp ? bg : in.end_job_p[f]);
            tr = tr && (br ? bg : in.end_job_r[f]);
            if (!tp && !tr) continue;
            if (skel && in.pin_mat.size() != nb * 12u) continue;
            float bnq[4] = { 0.0f, 0.0f, 0.0f, 1.0f }, bjq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            float bnp[3] = {}, bjp[3] = {};
            // CHAIN_REACH_ROT: the job's bone pose also for a bone ROTATION lane this frame does not follow (a
            // plain object: no guide, the rest - the orientation the solver held), so the position twin below clamps
            // as the solver did.
            if ((tp && bp) || (tr && br) || (tp && br)) {
                const std::vector<float>& jg = in.out_guide[f];   // The job's as it held it (CHAIN_HELD_REC), or none: the rest.
                chain_end_bone(c, jg.size() == nb * 12u ? jg.data() : nullptr, nb, bjq, bjp);
                if (tr || bg) chain_end_bone(c, in.pin_g.data(), nb, bnq, bnp);
            }
            // The frame the object draws at now, as cloth_sync builds the job's.
            float wc[3];
            double wd[3];
            cloth_carrier_at(o, c.auth_ctr, wc, wd);
            ClothFrame fr;
            memcpy(fr.rot, o.rot_m, sizeof(fr.rot));
            fr.rotated = o.rotated;
            float qf[4];
            chain_frame_q(fr, qf);
            const float qfc[4] = { -qf[0], -qf[1], -qf[2], qf[3] };
            float dp[3] = { 0.0f, 0.0f, 0.0f };
            float dq[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            // This frame's rotation target, carrier-local (a followed kind: its frame's; a bone: this cycle's pin
            // guide's), and the job's - what the solver held (CHAIN_REACH_ROT: the position twin reads them too).
            float qn[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            if (tr && !br) {
                float qt[4];
                if (wr) memcpy(qt, in.end_job_wq[f], sizeof(qt));
                else q_from_axes(s->r, qt);
                q_mul(qfc, qt, qn);
                q_norm(qn);
            }
            const float* const qj = br ? bjq : (in.end_job_r[f] ? in.end_job_q[f] : nullptr);
            const float* const ed_qf = tr ? (br ? bnq : qn) : qj;
            if (tp) {
                float tn[3], tj[3];   // This frame's position target and the job's, carrier-local.
                if (bp) {
                    memcpy(tn, bnp, sizeof(tn));
                    memcpy(tj, bjp, sizeof(tj));
                } else {
                    const double* const tw =   // The target, in the world.
                        wp ? static_cast<const double*>(in.end_job_w[f]) : static_cast<const double*>(s->p);
                    const float d[3] = { static_cast<float>(tw[0] - wd[0]),
                                         static_cast<float>(tw[1] - wd[1]),
                                         static_cast<float>(tw[2] - wd[2]) };
                    q_rot(qfc, d, tn);
                    memcpy(tj, in.end_job_x[f], sizeof(tj));
                }
                // As the solver holds them: within the chain's reach of its pin - the job's where the job's maps put
                // the start, this frame's where this frame's do (the maps the correction is applied to, below).
                chain_end_taut(c, in.ch_mat[f], tj, qj);
                chain_end_taut(c, skel ? in.pin_mat : in.ch_mat[f], tn, ed_qf);
                for (int k = 0; k < 3; ++k) dp[k] = tn[k] - tj[k];
            }
            if (tr && br) {
                const float jc[4] = { -bjq[0], -bjq[1], -bjq[2], bjq[3] };
                q_mul(bnq, jc, dq);
                q_norm(dq);
            } else if (tr) {
                const float* jq = in.end_job_q[f];
                const float jc[4] = { -jq[0], -jq[1], -jq[2], jq[3] };
                q_mul(qn, jc, dq);
                q_norm(dq);
            }
            if (!(dp[0] == dp[0] && dp[1] == dp[1] && dp[2] == dp[2] && dq[0] == dq[0] && dq[1] == dq[1] &&
                  dq[2] == dq[2] && dq[3] == dq[3])) continue;
            in.end_mat = skel ? in.pin_mat : in.ch_mat[f];
            chain_end_fix(c, in.end_mat, dp, dq, in.end_anc);
            // C3: the bytes this draw last wrote are kept (not skinned and written again) unless another write came
            // since or an input differs (the maps bit for bit, the front, its box).
            float box[6];
            memcpy(box, in.box_ctr[f], sizeof(float) * 3u);
            memcpy(box + 3, in.box_size[f], sizeof(float) * 3u);
            const bool keep = in.end_wr_ok && in.end_wr == in.vb_wr && in.end_vb == in.vb && in.end_f == f &&
                              in.end_gen == gen && in.end_mat_last.size() == in.end_mat.size() &&
                              memcmp(in.end_mat_last.data(), in.end_mat.data(),
                                     in.end_mat.size() * sizeof(float)) == 0 &&
                              memcmp(in.end_box, box, sizeof(box)) == 0;
            float be[3];
            if (keep) {
                memcpy(be, in.end_be, sizeof(be));
            } else {
                // Skinned and stored as the writeback stores a shape: in the front's box, grown about its centre
                // where the corrected end leaves it (chain_end_store).
                float bc[3];
                for (int k = 0; k < 3; ++k) bc[k] = in.box_ctr[f][k] - c.auth_ctr[k];
                in.end_out.resize(nv);
                chain_end_store(in.rest.data(), nv, c.ext, c.inf.size() == nv ? c.inf.data() : nullptr,
                                c.lvl0.size() == nv ? c.lvl0.data() : nullptr, in.end_mat.data(), nb, bc,
                                in.box_size[f], in.end_met, in.end_out.data(), be);
                D3D11_MAPPED_SUBRESOURCE m = {};
                if (FAILED(ctx->Map(in.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) continue;
                memcpy(m.pData, in.end_out.data(), nv * sizeof(MeshVertex));   // The whole array, as the upload.
                ctx->Unmap(in.vb, 0);
                uvs_src_wrote(o.slot);   // KH_USER_VS.
                ed_new = true;
                in.end_new_cyc = cyc;   // CHAIN_END_NOTE.
                in.uploaded_shape = cloth_step_shape(in.end_shape_ref, in.end_shape_key, in.end_met);
                in.end_wr = ++in.vb_wr;
                in.end_wr_ok = true;
                in.end_vb = in.vb;
                in.end_f = f;
                in.end_gen = gen;
                in.end_mat_last = in.end_mat;
                memcpy(in.end_box, box, sizeof(box));
                memcpy(in.end_be, be, sizeof(be));
            }
            // What the upload records with a write, for these bytes: their key (cloth_step_shape: the key follows
            // the bytes) and their box (the front's centre, the grown size) - which the upload's chain put and
            // skin_upload hand the object. A plain chain the rest too (CLOTH_PIN_STEP's rule), so the upload
            // leaves this buffer this cycle; a skeletal one's the pin step recorded, for this same front.
            memcpy(in.uploaded_box_size, be, sizeof(in.uploaded_box_size));
            if (!skel) {
                in.uploaded_gen = gen;
                in.uploaded_box_on = in.box_on[f];
                memcpy(in.uploaded_box_ctr, in.box_ctr[f], sizeof(in.uploaded_box_ctr));
                in.pin_step_cycle = cyc;
            }
            if (direct) g_cloth_gen_slot[o.slot] = in.uploaded_shape;
            size[0] = be[0];   // SQF order, as the upload's put.
            size[1] = be[2];
            size[2] = be[1];
            cycw = in.end_new_cyc == cyc;   // CHAIN_END_NOTE.
            wrote = true;
        } while (false);
        if (!wrote) {
            // CHAIN_END_RACE: as the note below takes such bytes at a final or repick step.
            if (race_cycw && (g_attach_final_step || g_attach_repick_step)) {
                attach_moved_note(o, true, true);
            }
            continue;
        }
        if (memcmp(o.size, size, sizeof(size)) != 0) {   // Either kind: the box of its bytes.
            memcpy(o.size, size, sizeof(size));
            ed_new = true;
        }
        // Re-filed; a final step's reopens the sun map (SUN_RESTEP), a repick step's the fire's (CAST_REPICK).
        // CHAIN_END_NOTE: only when the bytes or the box changed, or - at a final or repick step - the bytes an
        // earlier step of this cycle wrote (a seam, mirror or in-front step writes a plain chain's newer front after
        // the cast fire built the sun map, and its note reopens nothing). Do not note bytes kept from an earlier cycle
        // in an unchanged box: the scene and the maps already hold them, and noting them re-runs the sun ladder and
        // the repick every frame for an idle chain.
        attach_moved_note(o, ed_new || ((g_attach_final_step || g_attach_repick_step) && cycw), true);
    }
}

// CREW_LAG (declared with the crew table): any chain set up; the characters a simulating chain's end follows -
// its object, its memory point's owner, its rotation object - for the census (a Man among them joins the table).
// Game thread; the second under g_draw_list_mutex (the draw list is read).
inline bool crew_chain_any() { return !g_chain_cfg.empty(); }
inline void crew_chain_cand(std::vector<std::pair<game_value, bool>>& out) {
    for (const auto& kv : g_chain_cfg) {
        const ChainCfg& c = kv.second;
        const auto d = g_draw_list.find(kv.first);
        if (d == g_draw_list.end() || d->second.seq != c.seq || !chain_obj_sim(d->second)) {
            continue;
        }
        if ((c.pos == CHE_OBJ || c.pos == CHE_MEM) && attach_is_obj(c.pos_obj)) {
            out.emplace_back(c.pos_obj, false);
        }
        if (c.rot == CHE_OBJ && attach_is_obj(c.rot_obj)) out.emplace_back(c.rot_obj, false);
    }
}
// CHAIN - GAME THREAD, from flush_frame ahead of the cloth step and ahead of its empty-list return (a removed
// object's proxy is deleted whether or not anything is left to draw), holding no lock across an engine read. Drops the
// settings of objects that are gone, handing their proxies to the delete queue, and reads every followed end: an
// object's transform, or a memory point's through its proxy, by the raw page-gated reads attach_step uses - no
// script call. A memory-point proxy the attach has not yet moved off its spawn point fails the parent-reach test, as a
// skeletal binding's does, and the lane stays free that frame. gts: this flush_frame's sampler took a sample
// (attach_gt_snap's answer) - its number and stamp are this pass's.
inline void chain_prepare(bool gts) {
    KH_PROF_SCOPE(PROF_CHAIN_PREPARE);   // PROF.
    if (g_chain_cfg.empty()) {
        if (g_chain_end_n.load(std::memory_order_relaxed) != 0u) chain_end_publish(0u);   // Empties the table.
        return;
    }
    std::vector<std::string> dead, read;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& kv : g_chain_cfg) {
            const auto it = g_draw_list.find(kv.first);
            if (it == g_draw_list.end() || it->second.seq != kv.second.seq) {
                dead.push_back(kv.first);
                continue;
            }
            ChainCfg& c = kv.second;
            c.live_p = false;
            c.live_r = false;
            // CHAIN_REFUSED_END: the verdict, once per chain per Draw3D (a key built only while a refusal is filed),
            // for chain_end_publish; the reads below take every chain. A mesh still loading is not a verdict: it
            // files, and builds once it lands.
            c.nobuild = (mesh_alive(it->second.mesh) && !chain_mesh_ok(it->second.mesh)) ||
                        chain_refused(kv.first, it->second);
            const bool follow = c.pos == CHE_OBJ || c.pos == CHE_MEM ||
                                c.rot == CHE_OBJ || c.rot == CHE_MEM;
            if (follow && chain_obj_sim(it->second)) read.push_back(kv.first);
        }
    }
    for (const std::string& h : dead) {
        const auto it = g_chain_cfg.find(h);
        if (it == g_chain_cfg.end()) continue;
        attach_proxy_orphan(it->second.proxy);   // Takes g_draw_list_mutex itself.
        g_chain_cfg.erase(it);
    }
    // PXY_SMOOTH: this pass's sample. When the Draw3D sampler took one in this flush_frame (it runs first): its
    // number and stamp, so a proxy a binding shares steps once (the entry hands the sampler's result back) and one
    // it does not takes one step at the frame's stamp. When it took none, a sample of the chain's own
    // (PXY_SEQ_CHAIN). Key on the sampler's run, not on the binding count: a binding the census expires after the
    // sampler stepped a shared proxy leaves the count at zero, and a sample of the chain's own then steps the entry
    // twice in one pass.
    const int64_t q = gts ? g_gts.stamp_pub[g_gts_seq % GTS_STAMPS].load(std::memory_order_relaxed)
                                    : gts_qpc();
    const uint64_t seq = gts ? g_gts_seq : (PXY_SEQ_CHAIN | static_cast<uint64_t>(q));
    for (const std::string& h : read) {
        const auto it = g_chain_cfg.find(h);
        if (it == g_chain_cfg.end()) continue;
        ChainCfg& c = it->second;
        float p[3], r[9];
        bool mem_ok = false;
        float mr[9];
        if (c.pos == CHE_MEM || c.rot == CHE_MEM) {
            float pp[3], pr[9];
            if (!attach_obj_dead(c.pos_obj) && !attach_obj_dead(c.proxy) &&
                attach_raw(c.proxy, c.rd_pos.off, c.rd_pos.vb, c.rd_pos.bb, c.rd_pos.avb, c.rd_pos.abb, p, mr) &&
                attach_raw(c.pos_obj, c.rd_par.off, c.rd_par.vb, c.rd_par.bb, c.rd_par.avb, c.rd_par.abb, pp, pr)) {
                // CREW_LAG: the memory point moved by its character's correction, its reach tested against where
                // the character is drawn (its own state holds the get-in pose while it is crew).
                const CrewEnt* const cw = crew_of(c.pos_obj);
                if (cw) {
                    crew_apply(cw->t, cw->h, cw->dm, p, mr);
                    for (int k = 0; k < 3; ++k) pp[k] = static_cast<float>(cw->t[k]);
                }
                float d[3];
                for (int k = 0; k < 3; ++k) d[k] = p[k] - pp[k];
                mem_ok = d[0] * d[0] + d[1] * d[1] + d[2] * d[2] <=
                         SKEL_REACH_M * SKEL_REACH_M;
            }
            if (mem_ok && c.pos == CHE_MEM) {
                // PXY_SMOOTH: the memory point where the estimator has it (the proxy is the pool's), in double.
                float sd[3];
                pxy_smooth_note(c.proxy, seq, q, p, sd);
                c.live_pos[0] = static_cast<double>(p[0]) + static_cast<double>(sd[0]);
                c.live_pos[1] = static_cast<double>(p[2]) + static_cast<double>(sd[2]);
                c.live_pos[2] = static_cast<double>(p[1]) + static_cast<double>(sd[1]);
                c.live_p = true;
            }
            if (mem_ok && c.rot == CHE_MEM) {
                memcpy(c.live_rot, mr, sizeof(c.live_rot));
                c.live_r = true;
            }
        }
        if (c.pos == CHE_OBJ && !attach_obj_dead(c.pos_obj) &&
            attach_raw(c.pos_obj, c.rd_pos.off, c.rd_pos.vb, c.rd_pos.bb, c.rd_pos.avb, c.rd_pos.abb, p, r)) {
            c.live_pos[0] = p[0]; c.live_pos[1] = p[2]; c.live_pos[2] = p[1];
            const CrewEnt* const cw = crew_of(c.pos_obj);   // CREW_LAG: where it is drawn.
            if (cw) {
                c.live_pos[0] = cw->t[0];
                c.live_pos[1] = cw->t[2];
                c.live_pos[2] = cw->t[1];
            }
            c.live_p = true;
        }
        if (c.rot == CHE_OBJ && !attach_obj_dead(c.rot_obj) &&
            attach_raw(c.rot_obj, c.rd_rot.off, c.rd_rot.vb, c.rd_rot.bb, c.rd_rot.avb, c.rd_rot.abb, p, r)) {
            memcpy(c.live_rot, r, sizeof(c.live_rot));
            const CrewEnt* const cw = crew_of(c.rot_obj);   // CREW_LAG: turned as drawn.
            if (cw) memcpy(c.live_rot, cw->cr, sizeof(c.live_rot));
            c.live_r = true;
        }
        // A refused read holds the lane's last transform (ChainCfg::read_p); a dead object frees it.
        const game_value& po = c.pos_obj;
        if (c.pos == CHE_OBJ || c.pos == CHE_MEM) {
            if (c.live_p) {
                memcpy(c.last_pos, c.live_pos, sizeof(c.last_pos));
                c.read_p = true;
            } else {
                if (c.read_p && !attach_obj_dead(po)) {
                    memcpy(c.live_pos, c.last_pos, sizeof(c.live_pos));
                    c.live_p = true;
                } else {
                    c.read_p = false;
                }
            }
        }
        if (c.rot == CHE_OBJ || c.rot == CHE_MEM) {
            const game_value& ro = c.rot == CHE_MEM ? po : c.rot_obj;
            if (c.live_r) {
                memcpy(c.last_rot, c.live_rot, sizeof(c.last_rot));
                c.read_r = true;
            } else {
                if (c.read_r && !attach_obj_dead(ro)) {
                    memcpy(c.live_rot, c.last_rot, sizeof(c.live_rot));
                    c.live_r = true;
                } else {
                    c.read_r = false;
                }
            }
        }
    }
    chain_end_publish(seq);   // CHAIN_END_DRAW: this Draw3D's targets, by its sample number.
}
}