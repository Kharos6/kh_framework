#pragma once

// The feature half (rendering_integration_features.hpp): KH_CHAIN runtime: chain building and write-back, the chain
// ends, the build refusals and the config. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/skinning.hpp (declared here for the uses below).
inline void kh_skin_nrm_cof(const float khsn_r[3][3], const float khsn_n[3], float khsn_nn[3]);
// Defined in rendering/cloth_sync.hpp (declared here for kh_chain_writeback).
inline uint32_t kh_cloth_step_shape(std::vector<float>& khck_ref, uint32_t& khck_key,
                                    const std::vector<float>& khck_met);


// KH_CHAIN - the bodies, joints and contact points of a mesh's skeleton, from the start bone down. Game thread, on an
// instance that is not busy. False with a caller-facing reason in why. Every rest quantity is carrier-local metres
// (KhChainState).
//
// A bone's body is its geometry: the level-0 vertices whose strongest influence it is, welded by position, are its
// contact points, their centroid its centre of mass and their spread its inertia. The chain's mass is shared by length
// - each bone's reach from its head to its farthest child head or point - and its inertia gains a solid sphere's of
// radius a tenth of that reach, so a thin cord is not a body with no resistance to spin about its own axis. The end
// bone defaults to the tip of the chain when the start bone's subtree does not branch; with branches it must be named.
inline bool kh_chain_build(KhClothInst& khcb_in, const MeshDef& khcb_d, uint32_t khcb_slot, int khcb_mesh,
                           const std::string& khcb_start, const std::string& khcb_end, bool khcb_start_head,
                           bool khcb_end_tail, float khcb_mass, std::string& khcb_why) {
    KhChainState& c = khcb_in.ch;
    c = KhChainState();
    khcb_in.st = KhClothState();
    const size_t nb = khcb_d.skin_bones.size();
    if (nb == 0u || khcb_d.skin_inf.size() != khcb_d.verts.size() || khcb_d.verts.empty()) {
        khcb_why = "carries no skeleton (import a model whose bones skin its vertices)";
        return false;
    }
    // The start: named, or the skeleton's first root.
    int32_t s = -1;
    if (khcb_start.empty()) {
        for (size_t b = 0; b < nb; ++b) if (khcb_d.skin_bones[b].parent < 0) { s = static_cast<int32_t>(b); break; }
    } else {
        for (size_t b = 0; b < nb; ++b) if (khcb_d.skin_bones[b].name == khcb_start) { s = static_cast<int32_t>(b); break; }
    }
    if (s < 0) {
        khcb_why = khcb_start.empty() ? "has no root bone" : "has no bone named '" + khcb_start + "'";
        return false;
    }
    // Its subtree, parents before children (breadth first). A malformed parent
    // table cannot loop: each bone is taken once.
    std::vector<std::vector<int32_t>> kids(nb);
    for (size_t b = 0; b < nb; ++b) {
        const int32_t p = khcb_d.skin_bones[b].parent;
        if (p >= 0 && static_cast<size_t>(p) < nb && p != static_cast<int32_t>(b)) kids[static_cast<size_t>(p)].push_back(static_cast<int32_t>(b));
    }
    std::vector<int32_t> order;
    c.body_of.assign(nb, -1);
    order.push_back(s);
    c.body_of[static_cast<size_t>(s)] = 0;
    for (size_t i = 0; i < order.size(); ++i) {
        for (int32_t k : kids[static_cast<size_t>(order[i])]) {
            if (c.body_of[static_cast<size_t>(k)] >= 0) continue;
            c.body_of[static_cast<size_t>(k)] = static_cast<int32_t>(order.size());
            order.push_back(k);
        }
    }
    if (order.size() < 2u) {
        khcb_why = "has nothing below bone '" + khcb_d.skin_bones[static_cast<size_t>(s)].name + "' to simulate";
        return false;
    }
    // The end.
    if (!khcb_end.empty()) {
        int32_t e = -1;
        for (size_t i = 1; i < order.size(); ++i) {
            if (khcb_d.skin_bones[static_cast<size_t>(order[i])].name == khcb_end) { e = static_cast<int32_t>(i); break; }
        }
        if (e < 0) {
            khcb_why = "has no bone named '" + khcb_end + "' below its start bone '" +
                       khcb_d.skin_bones[static_cast<size_t>(s)].name + "'";
            return false;
        }
        c.end_body = e;
    } else {
        bool branch = false;
        for (int32_t b : order) if (kids[static_cast<size_t>(b)].size() > 1u) { branch = true; break; }
        c.end_body = branch ? -1 : static_cast<int32_t>(order.size() - 1u);
    }
    for (int k = 0; k < 3; ++k) { c.auth_ctr[k] = khcb_d.native_ctr[k]; c.ext[k] = khcb_d.native_ext[k]; }

    // Contact points: level 0's vertices, welded by position, on their strongest bone.
    const uint32_t n0 = mesh_base_icount(khcb_d);
    const size_t nv = khcb_d.verts.size();
    std::vector<uint8_t> used(nv, 0u);
    for (uint32_t i = 0; i < n0 && i < khcb_d.indices.size(); ++i) {
        const uint32_t v = khcb_d.indices[i];
        if (v < nv) used[v] = 1u;
    }
    std::vector<std::vector<float>> pts(order.size());
    std::unordered_map<KhClothPosKey, uint8_t, KhClothPosHash> seen;
    for (size_t v = 0; v < nv; ++v) {
        if (!used[v]) continue;
        const KhSkinInf& f = khcb_d.skin_inf[v];
        int32_t best = -1;
        float bw = 0.0f;
        for (int k = 0; k < 4; ++k) {
            if (f.w[k] > bw && f.b[k] < nb) { bw = f.w[k]; best = static_cast<int32_t>(f.b[k]); }
        }
        if (best < 0) continue;
        const int32_t bi = c.body_of[static_cast<size_t>(best)];
        if (bi <= 0) continue;   // Outside the chain, or on the start, which nothing moves.
        KhClothPosKey key;
        memcpy(key.p, khcb_d.verts[v].pos, sizeof(key.p));
        if (!seen.emplace(key, 1u).second) continue;
        std::vector<float>& pv = pts[static_cast<size_t>(bi)];
        for (int k = 0; k < 3; ++k) pv.push_back(khcb_d.verts[v].pos[k] * c.ext[k]);
    }

    // The bodies.
    const size_t nbd = order.size();
    c.body.resize(nbd);
    std::vector<float> head(nbd * 3u), reach(nbd, 0.0f);
    for (size_t i = 0; i < nbd; ++i) {
        const float* hd = khcb_d.skin_bones[static_cast<size_t>(order[i])].head;
        for (int k = 0; k < 3; ++k) head[i * 3u + k] = hd[k] - c.auth_ctr[k];
    }
    float reach_sum = 0.0f;
    for (size_t i = 0; i < nbd; ++i) {
        KhChainBody& b = c.body[i];
        b.bone = order[i];
        b.parent = i == 0 ? -1 : c.body_of[static_cast<size_t>(khcb_d.skin_bones[static_cast<size_t>(order[i])].parent)];
        const float* hd = &head[i * 3u];
        float r = 0.0f;
        int32_t first_kid = -1;
        for (int32_t k : kids[static_cast<size_t>(order[i])]) {
            const int32_t kb = c.body_of[static_cast<size_t>(k)];
            if (kb < 0) continue;
            if (first_kid < 0) first_kid = kb;
            float d[3];
            kh_cloth_v3_sub(&head[static_cast<size_t>(kb) * 3u], hd, d);
            r = fmaxf(r, kh_cloth_v3_len(d));
        }
        const std::vector<float>& pv = pts[i];
        const size_t np = pv.size() / 3u;
        float com[3] = { 0.0f, 0.0f, 0.0f };
        if (np) {
            for (size_t p = 0; p < np; ++p) for (int k = 0; k < 3; ++k) com[k] += pv[p * 3u + k];
            for (int k = 0; k < 3; ++k) com[k] /= static_cast<float>(np);
            for (size_t p = 0; p < np; ++p) {
                float d[3];
                kh_cloth_v3_sub(&pv[p * 3u], hd, d);
                r = fmaxf(r, kh_cloth_v3_len(d));
            }
        } else if (first_kid >= 0) {
            for (int k = 0; k < 3; ++k) com[k] = 0.5f * (hd[k] + head[static_cast<size_t>(first_kid) * 3u + k]);
        } else {
            memcpy(com, hd, sizeof(com));
        }
        memcpy(b.x0, com, sizeof(com));
        reach[i] = r > 0.01f ? r : 0.01f;
        if (i > 0) reach_sum += reach[i];
    }
    const float mass = khcb_mass > 1.0e-4f ? khcb_mass : 1.0e-4f;
    c.mass_built = khcb_mass;
    for (size_t i = 0; i < nbd; ++i) {
        KhChainBody& b = c.body[i];
        const std::vector<float>& pv = pts[i];
        const size_t np = pv.size() / 3u;
        b.s0 = static_cast<uint32_t>(c.samp.size() / 3u);
        b.sn = static_cast<uint32_t>(np);
        float rad = 0.0f;
        for (size_t p = 0; p < np; ++p) {
            float d[3];
            kh_cloth_v3_sub(&pv[p * 3u], b.x0, d);
            for (int k = 0; k < 3; ++k) c.samp.push_back(d[k]);
            rad = fmaxf(rad, kh_cloth_v3_len(d));
        }
        b.radius = fmaxf(rad, 0.5f * reach[i]);
        if (i == 0) {   // The start: kinematic.
            b.inv_m = 0.0f;
            memset(b.ib_inv, 0, sizeof(b.ib_inv));
            memset(b.ib, 0, sizeof(b.ib));
            continue;
        }
        const float m = mass * reach[i] / (reach_sum > 0.0f ? reach_sum : 1.0f);
        b.inv_m = 1.0f / m;
        // Inertia about the centre of mass: the points share the mass, plus the floor.
        double I[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        const double mp = np ? static_cast<double>(m) / static_cast<double>(np) : 0.0;
        for (size_t p = 0; p < np; ++p) {
            const float* d = &c.samp[(static_cast<size_t>(b.s0) + p) * 3u];
            const double r2 = static_cast<double>(d[0]) * d[0] + static_cast<double>(d[1]) * d[1] + static_cast<double>(d[2]) * d[2];
            for (int rr = 0; rr < 3; ++rr) for (int cc = 0; cc < 3; ++cc) {
                I[rr * 3 + cc] += mp * ((rr == cc ? r2 : 0.0) - static_cast<double>(d[rr]) * d[cc]);
            }
        }
        const double rho = 0.1 * static_cast<double>(reach[i]);
        for (int k = 0; k < 3; ++k) I[k * 4] += 0.4 * static_cast<double>(m) * rho * rho;
        for (int k = 0; k < 9; ++k) b.ib[k] = static_cast<float>(I[k]);
        const double det = I[0] * (I[4] * I[8] - I[5] * I[7]) - I[1] * (I[3] * I[8] - I[5] * I[6]) + I[2] * (I[3] * I[7] - I[4] * I[6]);
        if (!(det > 1.0e-30)) {
            const double e = 1.0 / (0.4 * static_cast<double>(m) * rho * rho);
            memset(b.ib_inv, 0, sizeof(b.ib_inv));
            for (int k = 0; k < 3; ++k) b.ib_inv[k * 4] = static_cast<float>(e);
        } else {
            const double id = 1.0 / det;
            b.ib_inv[0] = static_cast<float>((I[4] * I[8] - I[5] * I[7]) * id);
            b.ib_inv[1] = static_cast<float>((I[2] * I[7] - I[1] * I[8]) * id);
            b.ib_inv[2] = static_cast<float>((I[1] * I[5] - I[2] * I[4]) * id);
            b.ib_inv[3] = static_cast<float>((I[5] * I[6] - I[3] * I[8]) * id);
            b.ib_inv[4] = static_cast<float>((I[0] * I[8] - I[2] * I[6]) * id);
            b.ib_inv[5] = static_cast<float>((I[2] * I[3] - I[0] * I[5]) * id);
            b.ib_inv[6] = static_cast<float>((I[3] * I[7] - I[4] * I[6]) * id);
            b.ib_inv[7] = static_cast<float>((I[1] * I[6] - I[0] * I[7]) * id);
            b.ib_inv[8] = static_cast<float>((I[0] * I[4] - I[1] * I[3]) * id);
        }
    }
    // The joints, one per body but the start, in body order (joint i - 1 is body i's).
    c.joint.resize(nbd - 1u);
    for (size_t i = 1; i < nbd; ++i) {
        KhChainBody& b = c.body[i];
        KhChainJoint& j = c.joint[i - 1u];
        j.a = static_cast<uint32_t>(b.parent);
        j.b = static_cast<uint32_t>(i);
        const float* hd = &head[i * 3u];
        kh_cloth_v3_sub(hd, c.body[j.a].x0, j.ra);
        kh_cloth_v3_sub(hd, b.x0, j.rb);
        // The twist axis: down the bone, toward its first child or its own mass.
        float ax[3] = { 0.0f, 0.0f, 0.0f };
        for (int32_t k : kids[static_cast<size_t>(b.bone)]) {
            const int32_t kb = c.body_of[static_cast<size_t>(k)];
            if (kb < 0) continue;
            kh_cloth_v3_sub(&head[static_cast<size_t>(kb) * 3u], hd, ax);
            break;
        }
        if (!(kh_cloth_v3_len(ax) > 1.0e-5f)) kh_cloth_v3_sub(b.x0, hd, ax);
        if (!(kh_cloth_v3_len(ax) > 1.0e-5f)) kh_cloth_v3_sub(hd, &head[static_cast<size_t>(j.a) * 3u], ax);
        if (!(kh_cloth_v3_len(ax) > 1.0e-5f)) { ax[0] = 0.0f; ax[1] = -1.0f; ax[2] = 0.0f; }
        const float al = kh_cloth_v3_len(ax);
        for (int k = 0; k < 3; ++k) j.u[k] = ax[k] / al;
        // Across it: the world axis it is least along, made perpendicular.
        const float a0 = fabsf(j.u[0]), a1 = fabsf(j.u[1]), a2 = fabsf(j.u[2]);
        float ref[3] = { 0.0f, 0.0f, 0.0f };
        ref[a0 <= a1 && a0 <= a2 ? 0 : (a1 <= a2 ? 1 : 2)] = 1.0f;
        const float dr = kh_cloth_v3_dot(ref, j.u);
        for (int k = 0; k < 3; ++k) j.e[k] = ref[k] - j.u[k] * dr;
        const float el = kh_cloth_v3_len(j.e);
        for (int k = 0; k < 3; ++k) j.e[k] /= el;
    }
    // KH_CHAIN_POINT - the START's anchor. With "head" every joint on the start body anchors at the start bone's own
    // head instead of at its child's, and the rest placement carries each free subtree with it by the same amount, so
    // the pose the joints want is the pose kh_chain_reset lays down. The mesh IS one start bone shorter that way, which
    // is what asking for the head means; the bones below are unmoved relative to each other.
    if (khcb_start_head) {
        for (size_t i = 1; i < nbd; ++i) {
            if (c.body[i].parent != 0) continue;
            kh_cloth_v3_sub(&head[0], c.body[0].x0, c.joint[i - 1u].ra);
        }
        c.start_off.assign(nbd * 3u, 0.0f);
        for (size_t i = 1; i < nbd; ++i) {
            size_t khcb_r = i;   // The top of this body's subtree: the joint that moved.
            while (c.body[khcb_r].parent > 0) khcb_r = static_cast<size_t>(c.body[khcb_r].parent);
            for (int k = 0; k < 3; ++k) c.start_off[i * 3u + k] = head[k] - head[khcb_r * 3u + k];
        }
    }
    // KH_CHAIN_POINT - the END's held point, and how much further than the bone path it stands (the reach below has to
    // carry it, or a target at the chain's full extent would read as beyond it and clamp for ever).
    float khcb_tip = 0.0f;
    if (c.end_body > 0) {
        const size_t khcb_e = static_cast<size_t>(c.end_body);
        memcpy(c.end_pt, c.joint[khcb_e - 1u].rb, sizeof(c.end_pt));   // The head: the default.
        if (khcb_end_tail) {
            const float* khcb_hd = &head[khcb_e * 3u];
            const KhChainBody& khcb_eb = c.body[khcb_e];
            float khcb_tl[3] = { khcb_hd[0], khcb_hd[1], khcb_hd[2] };
            int32_t khcb_kid = -1;
            for (int32_t k : kids[static_cast<size_t>(khcb_eb.bone)]) {
                const int32_t kb = c.body_of[static_cast<size_t>(k)];
                if (kb > 0) { khcb_kid = kb; break; }
            }
            if (khcb_kid > 0) {
                memcpy(khcb_tl, &head[static_cast<size_t>(khcb_kid) * 3u], sizeof(khcb_tl));
            } else {
                // A tip bone: nothing below it says where it ends, so its own geometry does - the farthest contact
                // point down the joint's axis, which is the bone's direction. A bone with no points of its own falls
                // back to the reach the mass shares used.
                const float* khcb_u = c.joint[khcb_e - 1u].u;
                float khcb_far = 0.0f;
                for (uint32_t khcb_s = 0; khcb_s < khcb_eb.sn; ++khcb_s) {
                    const float* khcb_o = &c.samp[(static_cast<size_t>(khcb_eb.s0) + khcb_s) * 3u];
                    float khcb_v[3];
                    for (int k = 0; k < 3; ++k) khcb_v[k] = khcb_eb.x0[k] + khcb_o[k] - khcb_hd[k];
                    const float khcb_dp = kh_cloth_v3_dot(khcb_v, khcb_u);
                    if (khcb_dp > khcb_far) khcb_far = khcb_dp;
                }
                if (!(khcb_far > 1.0e-4f)) khcb_far = reach[khcb_e];
                for (int k = 0; k < 3; ++k) khcb_tl[k] = khcb_hd[k] + khcb_u[k] * khcb_far;
            }
            kh_cloth_v3_sub(khcb_tl, khcb_eb.x0, c.end_pt);
            float khcb_t3[3];
            kh_cloth_v3_sub(khcb_tl, khcb_hd, khcb_t3);
            khcb_tip = kh_cloth_v3_len(khcb_t3);
        }
    }
    // The end's reach along the bone path from it up to the body that hangs off the start, and that body
    // (KhChainState::end_reach, end_root), now that the joints carry the heads.
    if (c.end_body > 0) {
        double khcb_len = 0.0;
        int32_t khcb_b = c.end_body;
        while (khcb_b > 0) {
            const int32_t khcb_pa = c.body[static_cast<size_t>(khcb_b)].parent;
            if (khcb_pa < 0) break;
            c.end_root = khcb_b;
            if (khcb_pa == 0) break;   // Its joint's anchor on the start IS the pin.
            float khcb_hb[3], khcb_ha[3];
            for (int k = 0; k < 3; ++k) {
                khcb_hb[k] = c.body[static_cast<size_t>(khcb_b)].x0[k] + c.joint[static_cast<size_t>(khcb_b) - 1u].rb[k];
                khcb_ha[k] = c.body[static_cast<size_t>(khcb_pa)].x0[k] + c.joint[static_cast<size_t>(khcb_pa) - 1u].rb[k];
            }
            float khcb_d3[3];
            kh_cloth_v3_sub(khcb_hb, khcb_ha, khcb_d3);
            khcb_len += static_cast<double>(kh_cloth_v3_len(khcb_d3));
            khcb_b = khcb_pa;
        }
        c.end_reach = static_cast<float>(khcb_len) + khcb_tip;   // KH_CHAIN_POINT.
    }
    // What the writeback skins from.
    c.inf = khcb_d.skin_inf;
    if (khcb_d.lod_n != 0) c.lvl0 = used;
    khcb_in.rest = khcb_d.verts;
    khcb_in.out[0] = khcb_in.rest;
    khcb_in.out[1] = khcb_in.rest;
    khcb_in.out_guide[0].clear();   // KH_CHAIN_JOB_GUIDE: the rest pose was solved against no guide.
    khcb_in.out_guide[1].clear();
    // Room for a guide ahead of the first job, so a job's copy never re-allocates a buffer a chain-end read
    // (out_guide[pin_front], under g_cloth_mu) may be walking - KH_CHAIN_END_RACE keeps that read off a buffer a
    // running job is writing.
    khcb_in.out_guide[0].reserve(c.body_of.size() * 12u);
    khcb_in.out_guide[1].reserve(c.body_of.size() * 12u);
    memcpy(khcb_in.auth_ctr, c.auth_ctr, sizeof(khcb_in.auth_ctr));
    memcpy(khcb_in.auth_ext, c.ext, sizeof(khcb_in.auth_ext));
    khcb_in.lvl0_idx.clear();   // KH_CLOTH_OWN_IDX: a chain skins (kh_chain_writeback); it reads no index.
    khcb_in.mesh = khcb_mesh;
    khcb_in.slot = khcb_slot;
    khcb_in.chain = true;
    khcb_in.ch_start = khcb_start;
    khcb_in.ch_end = khcb_end;
    khcb_in.ch_start_head = khcb_start_head;   // KH_CHAIN_POINT: a change rebuilds.
    khcb_in.ch_end_tail = khcb_end_tail;
    khcb_in.built = true;
    return true;
}

// One render vertex of a chain mesh skinned through per-bone maps (12 floats a bone, rows then translation): position,
// normal and tangent in metres of the authored frame, missing weight to the rest. The writeback's per-vertex step,
// shared with the pin re-skin (kh_cloth_pin_place: kh_cloth_upload via kh_cloth_pin_fresh, kh_cloth_pin_draw_step) and
// kh_chain_end_store.
inline void kh_chain_skin_vert(const MeshVertex& khsv_r, const float* khsv_ext, const KhSkinInf* khsv_fi,
                               const float* khsv_mat, size_t khsv_nbone, float* khsv_pp, float* khsv_nn, float* khsv_tt) {
    float p[3], n[3], tn[3];
    for (int k = 0; k < 3; ++k) {
        p[k] = khsv_r.pos[k] * khsv_ext[k];
        n[k] = khsv_r.nrm[k] / khsv_ext[k];   // The authored normal (KH_IMPORT_NRM; the skin runtime's NORMALS note).
        tn[k] = khsv_r.tan[k] * khsv_ext[k];
    }
    for (int k = 0; k < 3; ++k) { khsv_pp[k] = 0.0f; khsv_nn[k] = 0.0f; khsv_tt[k] = 0.0f; }
    float ws = 0.0f;
    float br[3][3] = {};   // KH_SKIN_NRM_COF: the blended map's rows.
    if (khsv_fi) {
        for (int k = 0; k < 4; ++k) {
            const float w = khsv_fi->w[k];
            if (!(w > 0.0f) || khsv_fi->b[k] >= khsv_nbone) continue;
            const float* m = khsv_mat + static_cast<size_t>(khsv_fi->b[k]) * 12u;
            for (int e = 0; e < 3; ++e) {
                khsv_pp[e] += w * (p[0] * m[e] + p[1] * m[3 + e] + p[2] * m[6 + e] + m[9 + e]);
                khsv_tt[e] += w * (tn[0] * m[e] + tn[1] * m[3 + e] + tn[2] * m[6 + e]);
                br[e][0] += w * m[e];   // KH_SKIN_NRM_COF: the blended map's rows, as kh_skin_verts sums them.
                br[e][1] += w * m[3 + e];
                br[e][2] += w * m[6 + e];
            }
            ws += w;
        }
    }
    const float rw = ws < 1.0f ? 1.0f - ws : 0.0f;
    for (int k = 0; k < 3; ++k) {
        khsv_pp[k] += rw * p[k];
        khsv_tt[k] += rw * tn[k];
        br[k][k] += rw;   // KH_SKIN_NRM_COF: the root's share, the identity.
    }
    kh_skin_nrm_cof(br, n, khsv_nn);   // KH_SKIN_NRM_COF: kh_skin_verts' normal, so a chain takes over unseen.
}
// KH_CHAIN - buffer b from the bodies: every bone's map at the step's end (a simulated bone's from its body, any other
// from the skeleton's pose or the rest), linear blend skinning as kh_skin_run does it (missing weight to the root, the
// identity), then stored in the box of the drawn shape. A skeletal binding (in_boxed) takes the exact box of the
// level-0 vertices, which kh_skin_upload hands the object with its centre; any other object takes the smallest box
// CENTRED ON THE REST BOX'S CENTRE that holds them, because such an object's position is the script's and stays the
// rest centre - only its size follows the chain (kh_cloth_upload). Keyed like the skin: a new shape key when some
// vertex has left the shape that took the current one by more than KH_CLOTH_SHAPE_M.
inline void kh_chain_writeback(KhClothInst& khcw_in, uint32_t khcw_back) {
    const KhChainState& c = khcw_in.ch;
    const KhClothState& s = khcw_in.st;
    const size_t nbone = c.body_of.size();
    const size_t ng = khcw_in.in_guide.size() / 12u;
    const float* g = ng == nbone && ng != 0u ? khcw_in.in_guide.data() : nullptr;
    // Per bone: the 3x3 (images of the axes) and translation, carrier-local.
    std::vector<float> mat(nbone * 12u);
    KhClothFrame f;
    for (int k = 0; k < 3; ++k) { f.centre[k] = s.centre[k]; f.size[k] = s.size[k]; }
    memcpy(f.rot, s.rot, sizeof(f.rot));
    f.rotated = s.rotated;
    float qf[4];
    kh_chain_frame_q(f, qf);
    const float qfc[4] = { -qf[0], -qf[1], -qf[2], qf[3] };
    for (size_t bi = 0; bi < nbone; ++bi) {
        float q[4], t[3];
        const int32_t bd = c.body_of[bi];
        if (bd >= 0 && static_cast<size_t>(bd) < c.body.size()) {
            const KhChainBody& b = c.body[static_cast<size_t>(bd)];
            kh_q_mul(qfc, b.q, q);
            kh_q_norm(q);
            float d[3], xl[3], rx[3];
            kh_cloth_v3_sub(b.x, s.centre, d);
            kh_q_rot(qfc, d, xl);
            kh_q_rot(q, b.x0, rx);
            for (int k = 0; k < 3; ++k) t[k] = xl[k] - rx[k];
        } else {
            kh_chain_bone_map(c, g, g, nbone, static_cast<int32_t>(bi), 1.0f, q, t);
        }
        kh_q_to_axes(q, &mat[bi * 12u]);
        memcpy(&mat[bi * 12u + 9u], t, sizeof(t));
    }
    khcw_in.ch_mat[khcw_back] = mat;   // KH_CLOTH_PIN_FRESH: the park re-skins the guide's bones from these.
    // KH_CHAIN_END_DRAW: the end's targets this step held, carrier-local as the bones, and in the world - a held end
    // (KH_CHE_HOLD) as kh_chain_step holds it: where it first stood, once that was captured.
    // KH_CHAIN_HELD_REC: a followed target (pos / rot 2) as the step HELD it, not the frame's end it was given -
    // KH_SUB_CLOCK's substeps end cov of the way across the frame (kh_chain_end_pos / _rot sweep the target from the
    // last step's to this frame's), and the step leaves c.prev_p / prev_q at exactly that swept target (prev_pos /
    // prev_rot 2 after a step that held one; the reset path leaves them 0 and the frame's end stands).
    // kh_chain_end_draw corrects from these to this frame's.
    // Do not record the frame's end: it falls (1 - cov) of the target's travel short - a sub-centimetre tremor of a
    // followed end at frame rates whose frames do not divide into substeps (90 / 144 fps; none at 60 / 120, cov 1).
    const bool khcw_hp = khcw_in.in_tg.pos_hold && c.hold_on;
    const bool khcw_hr = khcw_in.in_tg.rot_hold && c.hold_on;
    khcw_in.end_job_p[khcw_back] = khcw_in.in_tg.pos == 2u || khcw_hp;
    if (khcw_in.end_job_p[khcw_back]) {
        const float* const khcw_tp = khcw_hp ? c.hold_p : (c.prev_pos == 2u ? c.prev_p : khcw_in.in_tg.p);
        float d[3];
        kh_cloth_v3_sub(khcw_tp, s.centre, d);
        kh_q_rot(qfc, d, khcw_in.end_job_x[khcw_back]);
        for (int k = 0; k < 3; ++k) {
            khcw_in.end_job_w[khcw_back][k] = khcw_in.org[k] + static_cast<double>(khcw_tp[k]);
        }
    }
    khcw_in.end_job_r[khcw_back] = khcw_in.in_tg.rot == 2u || khcw_hr;
    if (khcw_in.end_job_r[khcw_back]) {
        float qt[4];
        if (khcw_hr) memcpy(qt, c.hold_q, sizeof(qt));
        else if (c.prev_rot == 2u) memcpy(qt, c.prev_q, sizeof(qt));   // KH_CHAIN_HELD_REC.
        else kh_q_from_axes(khcw_in.in_tg.r, qt);
        kh_q_norm(qt);
        memcpy(khcw_in.end_job_wq[khcw_back], qt, sizeof(qt));
        kh_q_mul(qfc, qt, khcw_in.end_job_q[khcw_back]);
        kh_q_norm(khcw_in.end_job_q[khcw_back]);
    }
    const std::vector<MeshVertex>& rest = khcw_in.rest;
    const size_t nv = rest.size();
    std::vector<MeshVertex>& out = khcw_in.out[khcw_back];
    out.resize(nv);
    std::vector<float>& met = khcw_in.sim_met;
    met.resize(nv * 3u);
    const bool inf = c.inf.size() == nv;
    const bool lv = c.lvl0.size() == nv;
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& r = rest[i];
        MeshVertex& o = out[i];
        float pp[3], nn[3], tt[3];
        kh_chain_skin_vert(r, c.ext, inf ? &c.inf[i] : nullptr, mat.data(), nbone, pp, nn, tt);
        const bool box = !lv || c.lvl0[i] != 0u;
        for (int k = 0; k < 3; ++k) {
            if (!box) continue;
            if (pp[k] < mn[k]) mn[k] = pp[k];
            if (pp[k] > mx[k]) mx[k] = pp[k];
        }
        memcpy(&met[i * 3u], pp, sizeof(pp));
        memcpy(o.nrm, nn, sizeof(nn));
        memcpy(o.tan, tt, sizeof(tt));
        o.tan[3] = r.tan[3];
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
    }
    float bc[3], be[3];
    for (int k = 0; k < 3; ++k) {
        if (!(mn[k] <= mx[k])) { mn[k] = mx[k] = 0.0f; }
        if (khcw_in.in_boxed) {
            bc[k] = 0.5f * (mn[k] + mx[k]);
            be[k] = mx[k] - mn[k];
        } else {
            bc[k] = 0.0f;
            be[k] = 2.0f * fmaxf(fabsf(mn[k]), fabsf(mx[k]));
        }
        if (!(be[k] > 1.0e-4f)) be[k] = 1.0e-4f;   // A flat axis, as native_size.
    }
    for (size_t i = 0; i < nv; ++i) {
        MeshVertex& o = out[i];
        const float* p = &met[i * 3u];
        float n[3], t[3];
        for (int k = 0; k < 3; ++k) {
            o.pos[k] = (p[k] - bc[k]) / be[k];
            n[k] = o.nrm[k] * be[k];
            t[k] = o.tan[k] / be[k];
        }
        // Unit length: the shader normalizes after its own scale.
        const float nl = kh_cloth_v3_len(n), tl = kh_cloth_v3_len(t);
        for (int k = 0; k < 3; ++k) {
            o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : rest[i].nrm[k];
            o.tan[k] = tl > 1.0e-30f ? t[k] / tl : rest[i].tan[k];
        }
    }
    khcw_in.box_on[khcw_back] = true;
    for (int k = 0; k < 3; ++k) {
        khcw_in.box_ctr[khcw_back][k] = c.auth_ctr[k] + bc[k];
        khcw_in.box_size[khcw_back][k] = be[k];
    }
    khcw_in.shape_of[khcw_back] = kh_cloth_step_shape(khcw_in.shape_ref, khcw_in.shape_key, met);   // KH_CLOTH_SHAPE.
}

// KH_CHAIN - does this object simulate as a chain, and can its mesh? A chain and a cloth are exclusive (the SQF side
// refuses the second), and the test says so again here so that no path can hand one object to both.
inline bool kh_chain_obj_sim(const RenderObject& khco_o) {
    return khco_o.chain_sim && !khco_o.cloth_sim && !khco_o.fullscreen && khco_o.mesh >= 0;
}
inline bool kh_chain_mesh_ok(int khcm_mesh) {
    if (!kh_mesh_alive(khcm_mesh)) return false;
    const MeshDef& khcm_d = mesh_def(khcm_mesh);
    return !khcm_d.skin_bones.empty() && khcm_d.skin_inf.size() == khcm_d.verts.size() && !khcm_d.verts.empty();
}
inline bool kh_chain_end_followed(uint8_t khef_k) { return khef_k == KH_CHE_OBJ || khef_k == KH_CHE_MEM; }
inline bool kh_chain_end_fixed(uint8_t khef_k) { return khef_k == KH_CHE_WORLD || khef_k == KH_CHE_HOLD; }
inline bool kh_chain_end_drawn(uint8_t khef_k) { return kh_chain_end_followed(khef_k) || kh_chain_end_fixed(khef_k); }
static std::atomic<uint32_t> g_chain_skipped{ 0 };   // Frames a chain was still busy from the last one.
// Why a chain is NOT simulating, reported as the cloth's is (the RPT).
static std::vector<std::string> g_chain_warned;       // One report per mesh and reason.
// Builds already refused, by object, mesh and bone names: a chain whose settings name a bone its mesh lacks is refused
// once, not rebuilt from every vertex each frame. New names or a new mesh are a new key. Game thread; emptied at a
// mission's end, pruned when full (KH_CHAIN_NOBUILD_PRUNE).
static std::vector<std::string> g_chain_nobuild;
// KH_CHAIN_REFUSED_SKIN - the key a refused chain build is filed under: the object (its creation stamp), its mesh, and
// its settings' bone names and points (the defaults when it has none of its own). kh_cloth_sync files and tests it;
// kh_skin_sync tests it (kh_chain_refused), so a skeletal binding whose chain was refused - a bone its mesh lacks -
// stays the skin's to draw instead of standing in its bind pose with nothing drawing it; kh_chain_prepare folds it into
// KhChainCfg::nobuild for the chain-end table (KH_CHAIN_REFUSED_END). GAME THREAD.
inline std::string kh_chain_nobuild_key(const RenderObject& khnk_o, const KhChainCfg* khnk_cfg) {
    const bool khnk_shead = khnk_cfg ? khnk_cfg->start_head : false;
    const bool khnk_etail = khnk_cfg ? khnk_cfg->end_tail : false;
    return std::to_string(khnk_o.seq) + "|" + std::to_string(mesh_id_clamp(khnk_o.mesh)) + "|" +
           (khnk_cfg ? khnk_cfg->start : std::string()) + "|" + (khnk_cfg ? khnk_cfg->end : std::string()) + "|" +
           (khnk_shead ? "h" : "t") + (khnk_etail ? "t" : "h");
}
inline bool kh_chain_refused(const std::string& khcr_h, const RenderObject& khcr_o) {
    if (g_chain_nobuild.empty()) return false;
    const auto khcr_ci = g_chain_cfg.find(khcr_h);
    const KhChainCfg* khcr_cfg = (khcr_ci != g_chain_cfg.end() && khcr_ci->second.seq == khcr_o.seq)
                               ? &khcr_ci->second : nullptr;
    const std::string khcr_k = kh_chain_nobuild_key(khcr_o, khcr_cfg);
    return std::find(g_chain_nobuild.begin(), g_chain_nobuild.end(), khcr_k) != g_chain_nobuild.end();
}
// KH_CHAIN_NOBUILD_PRUNE - the refusals asked for now. A key names one object (its creation stamp), its mesh and its
// bone names, and only an object whose CURRENT key it is looks it up (kh_cloth_sync, kh_chain_refused): a deleted
// object never asks for it again, and one given another mesh or other names asks again only if the script goes back
// to them - a dropped key then costs one refused build, which files it again. Run at 256 keys, it keeps the live
// chains' current keys alone; a refusal is then always filed, so the list holds at most those plus 256. Do not instead
// stop taking keys at 256 and keep them to the mission's end: a long mission's dead objects fill it and every later
// refusal goes unfiled - its chain rebuilt from the whole mesh every frame and, a skeletal one, left in its bind pose
// (KH_CHAIN_REFUSED_SKIN). Game thread, holding no lock (it takes g_draw_list_mutex; g_chain_cfg is the game thread's).
inline void kh_chain_nobuild_prune() {
    std::vector<std::string> khnp_cur;
    {
        std::lock_guard<std::mutex> khnp_g(g_draw_list_mutex);
        for (const auto& khnp_kv : g_draw_list) {
            const RenderObject& khnp_o = khnp_kv.second;
            if (!kh_chain_obj_sim(khnp_o)) continue;
            const auto khnp_ci = g_chain_cfg.find(khnp_kv.first);
            const KhChainCfg* khnp_cfg = (khnp_ci != g_chain_cfg.end() && khnp_ci->second.seq == khnp_o.seq)
                                       ? &khnp_ci->second : nullptr;
            khnp_cur.push_back(kh_chain_nobuild_key(khnp_o, khnp_cfg));
        }
    }
    std::sort(khnp_cur.begin(), khnp_cur.end());
    g_chain_nobuild.erase(std::remove_if(g_chain_nobuild.begin(), g_chain_nobuild.end(),
                                         [&khnp_cur](const std::string& khnp_k) {
                                             return !std::binary_search(khnp_cur.begin(), khnp_cur.end(), khnp_k);
                                         }),
                          g_chain_nobuild.end());
}

// KH_CHAIN - an end rotation given as true follows what the end position follows: the memory point, the object, or else
// the bone on the carrier. So it is resolved again whenever either lane is set. GAME THREAD.
inline void kh_chain_cfg_resolve(KhChainCfg& khcr_c) {
    if (!khcr_c.rot_true) return;
    if (khcr_c.pos == KH_CHE_MEM) { khcr_c.rot = KH_CHE_MEM; khcr_c.rot_obj = game_value(); }
    else if (khcr_c.pos == KH_CHE_OBJ) { khcr_c.rot = KH_CHE_OBJ; khcr_c.rot_obj = khcr_c.pos_obj; }
    else { khcr_c.rot = KH_CHE_BONE; khcr_c.rot_obj = game_value(); }
    khcr_c.rd_rot = KhChainRead();
}

// KH_CHAIN - a plain object that simulates is drawn at its authored size in the box its shape needs
// (kh_chain_writeback); until the first result is installed its own rest buffer draws, in the rest box. For the command
// that switches the chain on, the one that changes its mesh, and a position that ends a skeletal binding. GAME THREAD,
// on the staged copy; staged lookup, as kh_apply_native_size.
inline void kh_chain_rest_size(RenderObject& khrs_o) {
    const MeshDef* khrs_sd = kh_mesh_def_staged(khrs_o.mesh);
    const MeshDef& khrs_d = khrs_sd ? *khrs_sd : mesh_def(0);
    for (int k = 0; k < 3; ++k) khrs_o.size[k] = khrs_d.native_size[k];
}

// KH_CHAIN - the end's targets for this frame, in the instance's frame (org its floating origin). A followed object
// that could not be read this frame leaves its lane free.
inline void kh_chain_target_of(const KhChainCfg* khct_c, const double* khct_org, KhChainTarget& khct_t) {
    khct_t = KhChainTarget();
    if (!khct_c) return;
    switch (khct_c->pos) {
    case KH_CHE_BONE: khct_t.pos = 1u; break;
    case KH_CHE_HOLD: khct_t.pos_hold = true; break;
    case KH_CHE_WORLD: {
        const double khct_w[3] = { khct_c->pos_w[0], khct_c->pos_w[2], khct_c->pos_w[1] };
        for (int k = 0; k < 3; ++k) khct_t.p[k] = static_cast<float>(khct_w[k] - khct_org[k]);
        khct_t.pos = 2u;
        break;
    }
    case KH_CHE_OBJ:
    case KH_CHE_MEM:
        if (khct_c->live_p) {
            for (int k = 0; k < 3; ++k) khct_t.p[k] = static_cast<float>(khct_c->live_pos[k] - khct_org[k]);
            khct_t.pos = 2u;
        }
        break;
    default: break;
    }
    switch (khct_c->rot) {
    case KH_CHE_BONE: khct_t.rot = 1u; break;
    case KH_CHE_HOLD: khct_t.rot_hold = true; break;
    case KH_CHE_WORLD: memcpy(khct_t.r, khct_c->rot_w, sizeof(khct_t.r)); khct_t.rot = 2u; break;
    case KH_CHE_OBJ:
    case KH_CHE_MEM:
        if (khct_c->live_r) { memcpy(khct_t.r, khct_c->live_rot, sizeof(khct_t.r)); khct_t.rot = 2u; }
        break;
    default: break;
    }
}

// KH_CHAIN_END_DRAW - the correction on one chain's bone maps (carrier-local: 3x3 images of the axes, then the
// translation): khcf_dp the end's move, khcf_dq its turn, about the point the end is held by. Along the chain's
// path from its start to the end body: the end body and everything below it take both; a link between the path's
// last kinematic body and the end a share of the move growing with its place on the path, and whatever branches
// off such a link the link's share (it stays on it); the rest - the kinematic start, what branches off before it,
// carrier / skeleton bones - nothing. Bodies are numbered parents before children (kh_chain_build), so on an
// unbranched chain the path is the numbering. khcf_anc: scratch.
inline void kh_chain_end_fix(const KhChainState& c, std::vector<float>& khcf_m, const float khcf_dp[3],
                             const float khcf_dq[4], std::vector<int32_t>& khcf_anc) {
    const int32_t khcf_eb = c.end_body;
    const size_t khcf_nb = c.body_of.size();
    const size_t khcf_nd = c.body.size();
    if (khcf_eb < 0 || static_cast<size_t>(khcf_eb) >= khcf_nd || khcf_m.size() != khcf_nb * 12u) return;
    const KhChainBody& e = c.body[static_cast<size_t>(khcf_eb)];
    // khcf_anc[0, nd): a body's place on the path (0 the start), or -1; [nd, 2 nd): the path body it hangs from.
    khcf_anc.assign(khcf_nd * 2u, -1);
    int32_t khcf_ep = 0;   // The end's place: its depth.
    for (int32_t p = c.body[static_cast<size_t>(khcf_eb)].parent, n = 0; p >= 0 && n < static_cast<int32_t>(khcf_nd);
         p = c.body[static_cast<size_t>(p)].parent, ++n) {
        if (static_cast<size_t>(p) >= khcf_nd) return;
        ++khcf_ep;
    }
    for (int32_t p = khcf_eb, d = khcf_ep; p >= 0 && d >= 0; p = c.body[static_cast<size_t>(p)].parent, --d) {
        khcf_anc[static_cast<size_t>(p)] = d;
    }
    int32_t khcf_s0 = -1;   // The place of the path's last kinematic body before the end.
    for (size_t b = 0; b < khcf_nd; ++b) {
        const int32_t d = khcf_anc[b];
        if (d >= 0 && d < khcf_ep && !(c.body[b].inv_m > 0.0f) && d > khcf_s0) khcf_s0 = d;
    }
    for (size_t b = 0; b < khcf_nd; ++b) {   // Parents before children: a parent's answer is in.
        const int32_t p = c.body[b].parent;
        if (khcf_anc[b] >= 0) khcf_anc[khcf_nd + b] = static_cast<int32_t>(b);
        else if (p >= 0 && static_cast<size_t>(p) < b) {
            khcf_anc[khcf_nd + b] = khcf_anc[khcf_nd + static_cast<size_t>(p)];
        }
    }
    const float* khcf_me = nullptr;
    for (size_t bi = 0; bi < khcf_nb; ++bi) if (c.body_of[bi] == khcf_eb) { khcf_me = &khcf_m[bi * 12u]; break; }
    if (!khcf_me) return;
    float khcf_lp[3], khcf_pv[3];   // The held point: the end body's centre plus end_pt, where the bone puts it.
    for (int k = 0; k < 3; ++k) khcf_lp[k] = e.x0[k] + (e.parent >= 0 ? c.end_pt[k] : 0.0f);
    for (int k = 0; k < 3; ++k) {
        khcf_pv[k] = khcf_lp[0] * khcf_me[k] + khcf_lp[1] * khcf_me[3 + k] + khcf_lp[2] * khcf_me[6 + k] +
                     khcf_me[9 + k];
    }
    for (size_t bi = 0; bi < khcf_nb; ++bi) {
        const int32_t bb = c.body_of[bi];
        if (bb < 0 || static_cast<size_t>(bb) >= khcf_nd) continue;
        const int32_t ba = khcf_anc[khcf_nd + static_cast<size_t>(bb)];   // The path body it moves with.
        if (ba < 0) continue;
        const int32_t bd = khcf_anc[static_cast<size_t>(ba)];   // That body's place.
        if (bd <= khcf_s0) continue;
        float* mm = &khcf_m[bi * 12u];
        if (bd >= khcf_ep) {
            for (int a = 0; a < 3; ++a) {
                const float v[3] = { mm[a * 3], mm[a * 3 + 1], mm[a * 3 + 2] };
                kh_q_rot(khcf_dq, v, &mm[a * 3]);
            }
            const float d[3] = { mm[9] - khcf_pv[0], mm[10] - khcf_pv[1], mm[11] - khcf_pv[2] };
            float r[3];
            kh_q_rot(khcf_dq, d, r);
            for (int k = 0; k < 3; ++k) mm[9 + k] = khcf_pv[k] + khcf_dp[k] + r[k];
        } else {
            const float w = static_cast<float>(bd - khcf_s0) / static_cast<float>(khcf_ep - khcf_s0);
            for (int k = 0; k < 3; ++k) mm[9 + k] += w * khcf_dp[k];
        }
    }
}
// KH_CHAIN_END_DRAW - the corrected shape stored as kh_chain_writeback stores a shape: every vertex skinned through
// khcs_mat (khcs_nb bones) into khcs_met (carrier-local metres, 3 a vertex) and khcs_out, in the smallest box
// centred on the front's (khcs_bc: its centre, carrier-local) that holds the level-0 vertices and is never smaller
// than the front's (khcs_bf) - handed back in khcs_be. The corrected end can leave the front's box, and the object's
// bounds are the box of its bytes. Nothing moved out, the box and the bytes are the front's formula's.
inline void kh_chain_end_store(const MeshVertex* khcs_rest, size_t khcs_nv, const float khcs_ext[3],
                               const KhSkinInf* khcs_inf, const uint8_t* khcs_lvl0, const float* khcs_mat,
                               size_t khcs_nb, const float khcs_bc[3], const float khcs_bf[3],
                               std::vector<float>& khcs_met, MeshVertex* khcs_out, float khcs_be[3]) {
    khcs_met.resize(khcs_nv * 3u);
    float khcs_r[3] = { 0.0f, 0.0f, 0.0f };   // The farthest level-0 vertex from the centre, per axis.
    for (size_t i = 0; i < khcs_nv; ++i) {
        const MeshVertex& r = khcs_rest[i];
        MeshVertex& o = khcs_out[i];
        float pp[3], nn[3], tt[3];
        kh_chain_skin_vert(r, khcs_ext, khcs_inf ? &khcs_inf[i] : nullptr, khcs_mat, khcs_nb, pp, nn, tt);
        if (!khcs_lvl0 || khcs_lvl0[i] != 0u) {   // The writeback's box rule: level 0.
            for (int k = 0; k < 3; ++k) {
                const float d = fabsf(pp[k] - khcs_bc[k]);
                if (d > khcs_r[k]) khcs_r[k] = d;
            }
        }
        memcpy(&khcs_met[i * 3u], pp, sizeof(pp));
        memcpy(o.nrm, nn, sizeof(nn));
        memcpy(o.tan, tt, sizeof(tt));
        o.tan[3] = r.tan[3];
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
    }
    for (int k = 0; k < 3; ++k) khcs_be[k] = 2.0f * khcs_r[k] > khcs_bf[k] ? 2.0f * khcs_r[k] : khcs_bf[k];
    for (size_t i = 0; i < khcs_nv; ++i) {
        MeshVertex& o = khcs_out[i];
        const float* p = &khcs_met[i * 3u];
        float n[3], tn[3];
        for (int k = 0; k < 3; ++k) {
            o.pos[k] = (p[k] - khcs_bc[k]) / khcs_be[k];
            n[k] = o.nrm[k] * khcs_be[k];
            tn[k] = o.tan[k] / khcs_be[k];
        }
        const float nl = kh_cloth_v3_len(n), tl = kh_cloth_v3_len(tn);
        for (int k = 0; k < 3; ++k) {
            o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : khcs_rest[i].nrm[k];
            o.tan[k] = tl > 1.0e-30f ? tn[k] / tl : khcs_rest[i].tan[k];
        }
    }
}
// KH_CHAIN_END_DRAW - a position target khct_p (carrier-local, in place) as kh_chain_step holds it: a target
// further from the pin than the chain can reach (KhChainState::end_reach) is pulled in to that reach along the same
// line - the pin is body 0's point where the end's path hangs off it (joint end_root - 1's ra), placed by the bone
// maps khct_m (the solver's step: body 0's pose at the step's end, which its map is). The job held its end at its
// own target so taken; the correction takes this frame's the same way, so a taut chain stays taut, not stretched.
// KH_CHAIN_REACH_ROT: khct_q, the end's held orientation (carrier-local) where its rotation is held too, else null -
// the solver's sphere (kh_chain_end_reach_rot), so the correction and the hold agree.
inline void kh_chain_end_taut(const KhChainState& c, const std::vector<float>& khct_m, float khct_p[3],
                              const float* khct_q) {
    if (!(c.end_reach >= 0.0f) || c.end_root <= 0 || c.body.empty() ||   // KH_CHAIN_REACH_ZERO (kh_chain_substep).
        static_cast<size_t>(c.end_root) > c.joint.size()) return;
    const int32_t khct_b = c.body[0].bone;
    if (khct_b < 0 || (static_cast<size_t>(khct_b) + 1u) * 12u > khct_m.size()) return;
    const float* const m = &khct_m[static_cast<size_t>(khct_b) * 12u];
    const float* const ra = c.joint[static_cast<size_t>(c.end_root) - 1u].ra;
    float l[3], pin[3], d[3];
    for (int k = 0; k < 3; ++k) l[k] = c.body[0].x0[k] + ra[k];
    for (int k = 0; k < 3; ++k) pin[k] = l[0] * m[k] + l[1] * m[3 + k] + l[2] * m[6 + k] + m[9 + k];
    const float khct_r = kh_chain_end_reach_rot(c, khct_q, pin);
    kh_cloth_v3_sub(khct_p, pin, d);
    const float len = kh_cloth_v3_len(d);
    if (len > khct_r && len > 1.0e-9f) {
        const float sc = khct_r / len;
        for (int k = 0; k < 3; ++k) khct_p[k] = pin[k] + d[k] * sc;
    }
}
// KH_CHAIN_END_DRAW - an end held on the bound skeleton's bone (kind 1: endPosition / endRotation true), where the
// skeleton pose khcb_g (khcb_nb maps in kh_skin_affine's layout, or null: the rest) puts it, carrier-local - what
// kh_chain_end_pos / kh_chain_end_rot hold at a step's end (kh_chain_kin at fraction 1: the bone map, the body
// placed by kh_chain_place with its start offset, then the endPoint), less the carrier frame. khcb_q: the body's
// orientation; khcb_p: the held point.
inline void kh_chain_end_bone(const KhChainState& c, const float* khcb_g, size_t khcb_nb, float khcb_q[4],
                              float khcb_p[3]) {
    const size_t khcb_e = static_cast<size_t>(c.end_body);
    const KhChainBody& E = c.body[khcb_e];
    float t[3], l[3];
    kh_chain_bone_map(c, khcb_g, khcb_g, khcb_nb, E.bone, 1.0f, khcb_q, t);
    kh_q_rot(khcb_q, E.x0, l);
    const float* const off = kh_chain_start_off(c, khcb_e);
    for (int k = 0; k < 3; ++k) l[k] += t[k] + (off ? off[k] : 0.0f);
    if (E.parent < 0) { memcpy(khcb_p, l, sizeof(l)); return; }
    float d[3];
    kh_q_rot(khcb_q, c.end_pt, d);   // KH_CHAIN_POINT.
    for (int k = 0; k < 3; ++k) khcb_p[k] = l[k] + d[k];
}
}