#pragma once

// The feature half (rendering_integration_features.hpp): CHAIN runtime: chain building and write-back, the chain
// ends, the build refusals and the config. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/skinning.hpp (declared here for the uses below).
inline void skin_nrm_cof(const float r[3][3], const float nrm[3], float nn[3]);
// Defined in rendering/cloth_sync.hpp (declared here for chain_writeback).
inline uint32_t cloth_step_shape(std::vector<float>& ref, uint32_t& key,
                                 const std::vector<float>& met);


// CHAIN - the bodies, joints and contact points of a mesh's skeleton, from the start bone down. Game thread, on an
// instance that is not busy. False with a caller-facing reason in why. Every rest quantity is carrier-local metres
// (ChainState).
//
// A bone's body is its geometry: the level-0 vertices whose strongest influence it is, welded by position, are its
// contact points, their centroid its centre of mass and their spread its inertia. The chain's mass is shared by length
// - each bone's reach from its head to its farthest child head or point - and its inertia gains a solid sphere's of
// radius a tenth of that reach, so a thin cord is not a body with no resistance to spin about its own axis. The end
// bone defaults to the tip of the chain when the start bone's subtree does not branch; with branches it must be named.
inline bool chain_build(ClothInst& in, const MeshDef& cb_d, uint32_t slot, int mesh,
                        const std::string& start, const std::string& end, bool start_head,
                        bool end_tail, float cb_mass, std::string& why) {
    ChainState& c = in.ch;
    c = ChainState();
    in.st = ClothState();
    const size_t nb = cb_d.skin_bones.size();
    if (nb == 0u || cb_d.skin_inf.size() != cb_d.verts.size() || cb_d.verts.empty()) {
        why = "carries no skeleton (import a model whose bones skin its vertices)";
        return false;
    }
    // The start: named, or the skeleton's first root.
    int32_t s = -1;
    if (start.empty()) {
        for (size_t b = 0; b < nb; ++b) if (cb_d.skin_bones[b].parent < 0) { s = static_cast<int32_t>(b); break; }
    } else {
        for (size_t b = 0; b < nb; ++b) if (cb_d.skin_bones[b].name == start) { s = static_cast<int32_t>(b); break; }
    }
    if (s < 0) {
        why = start.empty() ? "has no root bone" : "has no bone named '" + start + "'";
        return false;
    }
    // Its subtree, parents before children (breadth first). A malformed parent
    // table cannot loop: each bone is taken once.
    std::vector<std::vector<int32_t>> kids(nb);
    for (size_t b = 0; b < nb; ++b) {
        const int32_t p = cb_d.skin_bones[b].parent;
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
        why = "has nothing below bone '" + cb_d.skin_bones[static_cast<size_t>(s)].name + "' to simulate";
        return false;
    }
    // The end.
    if (!end.empty()) {
        int32_t e = -1;
        for (size_t i = 1; i < order.size(); ++i) {
            if (cb_d.skin_bones[static_cast<size_t>(order[i])].name == end) { e = static_cast<int32_t>(i); break; }
        }
        if (e < 0) {
            why = "has no bone named '" + end + "' below its start bone '" +
                  cb_d.skin_bones[static_cast<size_t>(s)].name + "'";
            return false;
        }
        c.end_body = e;
    } else {
        bool branch = false;
        for (int32_t b : order) if (kids[static_cast<size_t>(b)].size() > 1u) { branch = true; break; }
        c.end_body = branch ? -1 : static_cast<int32_t>(order.size() - 1u);
    }
    for (int k = 0; k < 3; ++k) { c.auth_ctr[k] = cb_d.native_ctr[k]; c.ext[k] = cb_d.native_ext[k]; }

    // Contact points: level 0's vertices, welded by position, on their strongest bone.
    const uint32_t n0 = mesh_base_icount(cb_d);
    const size_t nv = cb_d.verts.size();
    std::vector<uint8_t> used(nv, 0u);
    for (uint32_t i = 0; i < n0 && i < cb_d.indices.size(); ++i) {
        const uint32_t v = cb_d.indices[i];
        if (v < nv) used[v] = 1u;
    }
    std::vector<std::vector<float>> pts(order.size());
    std::unordered_map<ClothPosKey, uint8_t, ClothPosHash> seen;
    for (size_t v = 0; v < nv; ++v) {
        if (!used[v]) continue;
        const SkinInf& f = cb_d.skin_inf[v];
        int32_t best = -1;
        float bw = 0.0f;
        for (int k = 0; k < 4; ++k) {
            if (f.w[k] > bw && f.b[k] < nb) { bw = f.w[k]; best = static_cast<int32_t>(f.b[k]); }
        }
        if (best < 0) continue;
        const int32_t bi = c.body_of[static_cast<size_t>(best)];
        if (bi <= 0) continue;   // Outside the chain, or on the start, which nothing moves.
        ClothPosKey key;
        memcpy(key.p, cb_d.verts[v].pos, sizeof(key.p));
        if (!seen.emplace(key, 1u).second) continue;
        std::vector<float>& pv = pts[static_cast<size_t>(bi)];
        for (int k = 0; k < 3; ++k) pv.push_back(cb_d.verts[v].pos[k] * c.ext[k]);
    }

    // The bodies.
    const size_t nbd = order.size();
    c.body.resize(nbd);
    std::vector<float> head(nbd * 3u), reach(nbd, 0.0f);
    for (size_t i = 0; i < nbd; ++i) {
        const float* hd = cb_d.skin_bones[static_cast<size_t>(order[i])].head;
        for (int k = 0; k < 3; ++k) head[i * 3u + k] = hd[k] - c.auth_ctr[k];
    }
    float reach_sum = 0.0f;
    for (size_t i = 0; i < nbd; ++i) {
        ChainBody& b = c.body[i];
        b.bone = order[i];
        b.parent = i == 0 ? -1 : c.body_of[static_cast<size_t>(cb_d.skin_bones[static_cast<size_t>(order[i])].parent)];
        const float* hd = &head[i * 3u];
        float r = 0.0f;
        int32_t first_kid = -1;
        for (int32_t k : kids[static_cast<size_t>(order[i])]) {
            const int32_t kb = c.body_of[static_cast<size_t>(k)];
            if (kb < 0) continue;
            if (first_kid < 0) first_kid = kb;
            float d[3];
            cloth_v3_sub(&head[static_cast<size_t>(kb) * 3u], hd, d);
            r = fmaxf(r, cloth_v3_len(d));
        }
        const std::vector<float>& pv = pts[i];
        const size_t np = pv.size() / 3u;
        float com[3] = { 0.0f, 0.0f, 0.0f };
        if (np) {
            for (size_t p = 0; p < np; ++p) for (int k = 0; k < 3; ++k) com[k] += pv[p * 3u + k];
            for (int k = 0; k < 3; ++k) com[k] /= static_cast<float>(np);
            for (size_t p = 0; p < np; ++p) {
                float d[3];
                cloth_v3_sub(&pv[p * 3u], hd, d);
                r = fmaxf(r, cloth_v3_len(d));
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
    const float mass = cb_mass > 1.0e-4f ? cb_mass : 1.0e-4f;
    c.mass_built = cb_mass;
    for (size_t i = 0; i < nbd; ++i) {
        ChainBody& b = c.body[i];
        const std::vector<float>& pv = pts[i];
        const size_t np = pv.size() / 3u;
        b.s0 = static_cast<uint32_t>(c.samp.size() / 3u);
        b.sn = static_cast<uint32_t>(np);
        float rad = 0.0f;
        for (size_t p = 0; p < np; ++p) {
            float d[3];
            cloth_v3_sub(&pv[p * 3u], b.x0, d);
            for (int k = 0; k < 3; ++k) c.samp.push_back(d[k]);
            rad = fmaxf(rad, cloth_v3_len(d));
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
        ChainBody& b = c.body[i];
        ChainJoint& j = c.joint[i - 1u];
        j.a = static_cast<uint32_t>(b.parent);
        j.b = static_cast<uint32_t>(i);
        const float* hd = &head[i * 3u];
        cloth_v3_sub(hd, c.body[j.a].x0, j.ra);
        cloth_v3_sub(hd, b.x0, j.rb);
        // The twist axis: down the bone, toward its first child or its own mass.
        float ax[3] = { 0.0f, 0.0f, 0.0f };
        for (int32_t k : kids[static_cast<size_t>(b.bone)]) {
            const int32_t kb = c.body_of[static_cast<size_t>(k)];
            if (kb < 0) continue;
            cloth_v3_sub(&head[static_cast<size_t>(kb) * 3u], hd, ax);
            break;
        }
        if (!(cloth_v3_len(ax) > 1.0e-5f)) cloth_v3_sub(b.x0, hd, ax);
        if (!(cloth_v3_len(ax) > 1.0e-5f)) cloth_v3_sub(hd, &head[static_cast<size_t>(j.a) * 3u], ax);
        if (!(cloth_v3_len(ax) > 1.0e-5f)) { ax[0] = 0.0f; ax[1] = -1.0f; ax[2] = 0.0f; }
        const float al = cloth_v3_len(ax);
        for (int k = 0; k < 3; ++k) j.u[k] = ax[k] / al;
        // Across it: the world axis it is least along, made perpendicular.
        const float a0 = fabsf(j.u[0]), a1 = fabsf(j.u[1]), a2 = fabsf(j.u[2]);
        float ref[3] = { 0.0f, 0.0f, 0.0f };
        ref[a0 <= a1 && a0 <= a2 ? 0 : (a1 <= a2 ? 1 : 2)] = 1.0f;
        const float dr = cloth_v3_dot(ref, j.u);
        for (int k = 0; k < 3; ++k) j.e[k] = ref[k] - j.u[k] * dr;
        const float el = cloth_v3_len(j.e);
        for (int k = 0; k < 3; ++k) j.e[k] /= el;
    }
    // CHAIN_POINT - the START's anchor. With "head" every joint on the start body anchors at the start bone's own
    // head instead of at its child's, and the rest placement carries each free subtree with it by the same amount, so
    // the pose the joints want is the pose chain_reset lays down. The mesh IS one start bone shorter that way, which
    // is what asking for the head means; the bones below are unmoved relative to each other.
    if (start_head) {
        for (size_t i = 1; i < nbd; ++i) {
            if (c.body[i].parent != 0) continue;
            cloth_v3_sub(&head[0], c.body[0].x0, c.joint[i - 1u].ra);
        }
        c.start_off.assign(nbd * 3u, 0.0f);
        for (size_t i = 1; i < nbd; ++i) {
            size_t cb_r = i;   // The top of this body's subtree: the joint that moved.
            while (c.body[cb_r].parent > 0) cb_r = static_cast<size_t>(c.body[cb_r].parent);
            for (int k = 0; k < 3; ++k) c.start_off[i * 3u + k] = head[k] - head[cb_r * 3u + k];
        }
    }
    // CHAIN_POINT - the END's held point, and how much further than the bone path it stands (the reach below has to
    // carry it, or a target at the chain's full extent would read as beyond it and clamp for ever).
    float tip = 0.0f;
    if (c.end_body > 0) {
        const size_t cb_e = static_cast<size_t>(c.end_body);
        memcpy(c.end_pt, c.joint[cb_e - 1u].rb, sizeof(c.end_pt));   // The head: the default.
        if (end_tail) {
            const float* cb_hd = &head[cb_e * 3u];
            const ChainBody& eb = c.body[cb_e];
            float tl[3] = { cb_hd[0], cb_hd[1], cb_hd[2] };
            int32_t kid = -1;
            for (int32_t k : kids[static_cast<size_t>(eb.bone)]) {
                const int32_t kb = c.body_of[static_cast<size_t>(k)];
                if (kb > 0) { kid = kb; break; }
            }
            if (kid > 0) {
                memcpy(tl, &head[static_cast<size_t>(kid) * 3u], sizeof(tl));
            } else {
                // A tip bone: nothing below it says where it ends, so its own geometry does - the farthest contact
                // point down the joint's axis, which is the bone's direction. A bone with no points of its own falls
                // back to the reach the mass shares used.
                const float* u = c.joint[cb_e - 1u].u;
                float cb_far = 0.0f;
                for (uint32_t cb_s = 0; cb_s < eb.sn; ++cb_s) {
                    const float* o = &c.samp[(static_cast<size_t>(eb.s0) + cb_s) * 3u];
                    float cb_v[3];
                    for (int k = 0; k < 3; ++k) cb_v[k] = eb.x0[k] + o[k] - cb_hd[k];
                    const float dp = cloth_v3_dot(cb_v, u);
                    if (dp > cb_far) cb_far = dp;
                }
                if (!(cb_far > 1.0e-4f)) cb_far = reach[cb_e];
                for (int k = 0; k < 3; ++k) tl[k] = cb_hd[k] + u[k] * cb_far;
            }
            cloth_v3_sub(tl, eb.x0, c.end_pt);
            float t3[3];
            cloth_v3_sub(tl, cb_hd, t3);
            tip = cloth_v3_len(t3);
        }
    }
    // The end's reach along the bone path from it up to the body that hangs off the start, and that body
    // (ChainState::end_reach, end_root), now that the joints carry the heads.
    if (c.end_body > 0) {
        double len = 0.0;
        int32_t cb_b = c.end_body;
        while (cb_b > 0) {
            const int32_t pa = c.body[static_cast<size_t>(cb_b)].parent;
            if (pa < 0) break;
            c.end_root = cb_b;
            if (pa == 0) break;   // Its joint's anchor on the start IS the pin.
            float hb[3], ha[3];
            for (int k = 0; k < 3; ++k) {
                hb[k] = c.body[static_cast<size_t>(cb_b)].x0[k] + c.joint[static_cast<size_t>(cb_b) - 1u].rb[k];
                ha[k] = c.body[static_cast<size_t>(pa)].x0[k] + c.joint[static_cast<size_t>(pa) - 1u].rb[k];
            }
            float d3[3];
            cloth_v3_sub(hb, ha, d3);
            len += static_cast<double>(cloth_v3_len(d3));
            cb_b = pa;
        }
        c.end_reach = static_cast<float>(len) + tip;   // CHAIN_POINT.
    }
    // What the writeback skins from.
    c.inf = cb_d.skin_inf;
    if (cb_d.lod_n != 0) c.lvl0 = used;
    in.rest = cb_d.verts;
    in.out[0] = in.rest;
    in.out[1] = in.rest;
    in.out_guide[0].clear();   // CHAIN_JOB_GUIDE: the rest pose was solved against no guide.
    in.out_guide[1].clear();
    // Room for a guide ahead of the first job, so a job's copy never re-allocates a buffer a chain-end read
    // (out_guide[pin_front], under g_cloth_mu) may be walking - CHAIN_END_RACE keeps that read off a buffer a
    // running job is writing.
    in.out_guide[0].reserve(c.body_of.size() * 12u);
    in.out_guide[1].reserve(c.body_of.size() * 12u);
    memcpy(in.auth_ctr, c.auth_ctr, sizeof(in.auth_ctr));
    memcpy(in.auth_ext, c.ext, sizeof(in.auth_ext));
    in.lvl0_idx.clear();   // CLOTH_OWN_IDX: a chain skins (chain_writeback); it reads no index.
    in.mesh = mesh;
    in.slot = slot;
    in.chain = true;
    in.ch_start = start;
    in.ch_end = end;
    in.ch_start_head = start_head;   // CHAIN_POINT: a change rebuilds.
    in.ch_end_tail = end_tail;
    in.built = true;
    return true;
}

// One render vertex of a chain mesh skinned through per-bone maps (12 floats a bone, rows then translation): position,
// normal and tangent in metres of the authored frame, missing weight to the rest. The writeback's per-vertex step,
// shared with the pin re-skin (cloth_pin_place: cloth_upload via cloth_pin_fresh, cloth_pin_draw_step) and
// chain_end_store.
inline void chain_skin_vert(const MeshVertex& r, const float* ext, const SkinInf* fi,
                            const float* mat, size_t nbone, float* pp, float* nn, float* tt) {
    float p[3], n[3], tn[3];
    for (int k = 0; k < 3; ++k) {
        p[k] = r.pos[k] * ext[k];
        n[k] = r.nrm[k] / ext[k];   // The authored normal (IMPORT_NRM; the skin runtime's NORMALS note).
        tn[k] = r.tan[k] * ext[k];
    }
    for (int k = 0; k < 3; ++k) { pp[k] = 0.0f; nn[k] = 0.0f; tt[k] = 0.0f; }
    float ws = 0.0f;
    float br[3][3] = {};   // SKIN_NRM_COF: the blended map's rows.
    if (fi) {
        for (int k = 0; k < 4; ++k) {
            const float w = fi->w[k];
            if (!(w > 0.0f) || fi->b[k] >= nbone) continue;
            const float* m = mat + static_cast<size_t>(fi->b[k]) * 12u;
            for (int e = 0; e < 3; ++e) {
                pp[e] += w * (p[0] * m[e] + p[1] * m[3 + e] + p[2] * m[6 + e] + m[9 + e]);
                tt[e] += w * (tn[0] * m[e] + tn[1] * m[3 + e] + tn[2] * m[6 + e]);
                br[e][0] += w * m[e];   // SKIN_NRM_COF: the blended map's rows, as skin_verts sums them.
                br[e][1] += w * m[3 + e];
                br[e][2] += w * m[6 + e];
            }
            ws += w;
        }
    }
    const float rw = ws < 1.0f ? 1.0f - ws : 0.0f;
    for (int k = 0; k < 3; ++k) {
        pp[k] += rw * p[k];
        tt[k] += rw * tn[k];
        br[k][k] += rw;   // SKIN_NRM_COF: the root's share, the identity.
    }
    skin_nrm_cof(br, n, nn);   // SKIN_NRM_COF: skin_verts' normal, so a chain takes over unseen.
}
// CHAIN - buffer b from the bodies: every bone's map at the step's end (a simulated bone's from its body, any other
// from the skeleton's pose or the rest), linear blend skinning as skin_run does it (missing weight to the root, the
// identity), then stored in the box of the drawn shape. A skeletal binding (in_boxed) takes the exact box of the
// level-0 vertices, which skin_upload hands the object with its centre; any other object takes the smallest box
// CENTRED ON THE REST BOX'S CENTRE that holds them, because such an object's position is the script's and stays the
// rest centre - only its size follows the chain (cloth_upload). Keyed like the skin: a new shape key when some
// vertex has left the shape that took the current one by more than CLOTH_SHAPE_M.
inline void chain_writeback(ClothInst& in, uint32_t back) {
    const ChainState& c = in.ch;
    const ClothState& s = in.st;
    const size_t nbone = c.body_of.size();
    const size_t ng = in.in_guide.size() / 12u;
    const float* g = ng == nbone && ng != 0u ? in.in_guide.data() : nullptr;
    // Per bone: the 3x3 (images of the axes) and translation, carrier-local.
    std::vector<float> mat(nbone * 12u);
    ClothFrame f;
    for (int k = 0; k < 3; ++k) { f.centre[k] = s.centre[k]; f.size[k] = s.size[k]; }
    memcpy(f.rot, s.rot, sizeof(f.rot));
    f.rotated = s.rotated;
    float qf[4];
    chain_frame_q(f, qf);
    const float qfc[4] = { -qf[0], -qf[1], -qf[2], qf[3] };
    for (size_t bi = 0; bi < nbone; ++bi) {
        float q[4], t[3];
        const int32_t bd = c.body_of[bi];
        if (bd >= 0 && static_cast<size_t>(bd) < c.body.size()) {
            const ChainBody& b = c.body[static_cast<size_t>(bd)];
            q_mul(qfc, b.q, q);
            q_norm(q);
            float d[3], xl[3], rx[3];
            cloth_v3_sub(b.x, s.centre, d);
            q_rot(qfc, d, xl);
            q_rot(q, b.x0, rx);
            for (int k = 0; k < 3; ++k) t[k] = xl[k] - rx[k];
        } else {
            chain_bone_map(c, g, g, nbone, static_cast<int32_t>(bi), 1.0f, q, t);
        }
        q_to_axes(q, &mat[bi * 12u]);
        memcpy(&mat[bi * 12u + 9u], t, sizeof(t));
    }
    in.ch_mat[back] = mat;   // CLOTH_PIN_FRESH: the park re-skins the guide's bones from these.
    // CHAIN_END_DRAW: the end's targets this step held, carrier-local as the bones, and in the world - a held end
    // (CHE_HOLD) as chain_step holds it: where it first stood, once that was captured.
    // CHAIN_HELD_REC: a followed target (pos / rot 2) as the step HELD it, not the frame's end it was given -
    // SUB_CLOCK's substeps end cov of the way across the frame (chain_end_pos / _rot sweep the target from the
    // last step's to this frame's), and the step leaves c.prev_p / prev_q at exactly that swept target (prev_pos /
    // prev_rot 2 after a step that held one; the reset path leaves them 0 and the frame's end stands).
    // chain_end_draw corrects from these to this frame's.
    // Do not record the frame's end: it falls (1 - cov) of the target's travel short - a sub-centimetre tremor of a
    // followed end at frame rates whose frames do not divide into substeps (90 / 144 fps; none at 60 / 120, cov 1).
    const bool hp = in.in_tg.pos_hold && c.hold_on;
    const bool hr = in.in_tg.rot_hold && c.hold_on;
    in.end_job_p[back] = in.in_tg.pos == 2u || hp;
    if (in.end_job_p[back]) {
        const float* const tp = hp ? c.hold_p : (c.prev_pos == 2u ? c.prev_p : in.in_tg.p);
        float d[3];
        cloth_v3_sub(tp, s.centre, d);
        q_rot(qfc, d, in.end_job_x[back]);
        for (int k = 0; k < 3; ++k) {
            in.end_job_w[back][k] = in.org[k] + static_cast<double>(tp[k]);
        }
    }
    in.end_job_r[back] = in.in_tg.rot == 2u || hr;
    if (in.end_job_r[back]) {
        float qt[4];
        if (hr) memcpy(qt, c.hold_q, sizeof(qt));
        else if (c.prev_rot == 2u) memcpy(qt, c.prev_q, sizeof(qt));   // CHAIN_HELD_REC.
        else q_from_axes(in.in_tg.r, qt);
        q_norm(qt);
        memcpy(in.end_job_wq[back], qt, sizeof(qt));
        q_mul(qfc, qt, in.end_job_q[back]);
        q_norm(in.end_job_q[back]);
    }
    const std::vector<MeshVertex>& rest = in.rest;
    const size_t nv = rest.size();
    std::vector<MeshVertex>& out = in.out[back];
    out.resize(nv);
    std::vector<float>& met = in.sim_met;
    met.resize(nv * 3u);
    const bool inf = c.inf.size() == nv;
    const bool lv = c.lvl0.size() == nv;
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& r = rest[i];
        MeshVertex& o = out[i];
        float pp[3], nn[3], tt[3];
        chain_skin_vert(r, c.ext, inf ? &c.inf[i] : nullptr, mat.data(), nbone, pp, nn, tt);
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
        if (in.in_boxed) {
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
        const float nl = cloth_v3_len(n), tl = cloth_v3_len(t);
        for (int k = 0; k < 3; ++k) {
            o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : rest[i].nrm[k];
            o.tan[k] = tl > 1.0e-30f ? t[k] / tl : rest[i].tan[k];
        }
    }
    in.box_on[back] = true;
    for (int k = 0; k < 3; ++k) {
        in.box_ctr[back][k] = c.auth_ctr[k] + bc[k];
        in.box_size[back][k] = be[k];
    }
    in.shape_of[back] = cloth_step_shape(in.shape_ref, in.shape_key, met);   // CLOTH_SHAPE.
}

// CHAIN - does this object simulate as a chain, and can its mesh? A chain and a cloth are exclusive (the SQF side
// refuses the second), and the test says so again here so that no path can hand one object to both.
inline bool chain_obj_sim(const RenderObject& o) {
    return o.chain_sim && !o.cloth_sim && !o.fullscreen && o.mesh >= 0;
}
inline bool chain_mesh_ok(int mesh) {
    if (!mesh_alive(mesh)) return false;
    const MeshDef& d = mesh_def(mesh);
    return !d.skin_bones.empty() && d.skin_inf.size() == d.verts.size() && !d.verts.empty();
}
inline bool chain_end_followed(uint8_t k) { return k == CHE_OBJ || k == CHE_MEM; }
inline bool chain_end_fixed(uint8_t k) { return k == CHE_WORLD || k == CHE_HOLD; }
inline bool chain_end_drawn(uint8_t k) { return chain_end_followed(k) || chain_end_fixed(k); }
static std::atomic<uint32_t> g_chain_skipped{ 0 };   // Frames a chain was still busy from the last one.
// Why a chain is NOT simulating, reported as the cloth's is (the RPT).
static std::vector<std::string> g_chain_warned;       // One report per mesh and reason.
// Builds already refused, by object, mesh and bone names: a chain whose settings name a bone its mesh lacks is refused
// once, not rebuilt from every vertex each frame. New names or a new mesh are a new key. Game thread; emptied at a
// mission's end, pruned when full (CHAIN_NOBUILD_PRUNE).
static std::vector<std::string> g_chain_nobuild;
// CHAIN_REFUSED_SKIN - the key a refused chain build is filed under: the object (its creation stamp), its mesh, and
// its settings' bone names and points (the defaults when it has none of its own). cloth_sync files and tests it;
// skin_sync tests it (chain_refused), so a skeletal binding whose chain was refused - a bone its mesh lacks -
// stays the skin's to draw instead of standing in its bind pose with nothing drawing it; chain_prepare folds it into
// ChainCfg::nobuild for the chain-end table (CHAIN_REFUSED_END). GAME THREAD.
inline std::string chain_nobuild_key(const RenderObject& o, const ChainCfg* cfg) {
    const bool shead = cfg ? cfg->start_head : false;
    const bool etail = cfg ? cfg->end_tail : false;
    return std::to_string(o.seq) + "|" + std::to_string(mesh_id_clamp(o.mesh)) + "|" +
           (cfg ? cfg->start : std::string()) + "|" + (cfg ? cfg->end : std::string()) + "|" +
           (shead ? "h" : "t") + (etail ? "t" : "h");
}
inline bool chain_refused(const std::string& h, const RenderObject& o) {
    if (g_chain_nobuild.empty()) return false;
    const auto ci = g_chain_cfg.find(h);
    const ChainCfg* cfg = (ci != g_chain_cfg.end() && ci->second.seq == o.seq)
                        ? &ci->second : nullptr;
    const std::string k = chain_nobuild_key(o, cfg);
    return std::find(g_chain_nobuild.begin(), g_chain_nobuild.end(), k) != g_chain_nobuild.end();
}
// CHAIN_NOBUILD_PRUNE - the refusals asked for now. A key names one object (its creation stamp), its mesh and its
// bone names, and only an object whose CURRENT key it is looks it up (cloth_sync, chain_refused): a deleted
// object never asks for it again, and one given another mesh or other names asks again only if the script goes back
// to them - a dropped key then costs one refused build, which files it again. Run at 256 keys, it keeps the live
// chains' current keys alone; a refusal is then always filed, so the list holds at most those plus 256. Do not instead
// stop taking keys at 256 and keep them to the mission's end: a long mission's dead objects fill it and every later
// refusal goes unfiled - its chain rebuilt from the whole mesh every frame and, a skeletal one, left in its bind pose
// (CHAIN_REFUSED_SKIN). Game thread, holding no lock (it takes g_draw_list_mutex; g_chain_cfg is the game thread's).
inline void chain_nobuild_prune() {
    std::vector<std::string> cur;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (const auto& kv : g_draw_list) {
            const RenderObject& o = kv.second;
            if (!chain_obj_sim(o)) continue;
            const auto ci = g_chain_cfg.find(kv.first);
            const ChainCfg* cfg = (ci != g_chain_cfg.end() && ci->second.seq == o.seq)
                                ? &ci->second : nullptr;
            cur.push_back(chain_nobuild_key(o, cfg));
        }
    }
    std::sort(cur.begin(), cur.end());
    g_chain_nobuild.erase(std::remove_if(g_chain_nobuild.begin(), g_chain_nobuild.end(),
                                         [&cur](const std::string& k) {
                                             return !std::binary_search(cur.begin(), cur.end(), k);
                                         }),
                          g_chain_nobuild.end());
}

// CHAIN - an end rotation given as true follows what the end position follows: the memory point, the object, or else
// the bone on the carrier. So it is resolved again whenever either lane is set. GAME THREAD.
inline void chain_cfg_resolve(ChainCfg& c) {
    if (!c.rot_true) return;
    if (c.pos == CHE_MEM) { c.rot = CHE_MEM; c.rot_obj = game_value(); }
    else if (c.pos == CHE_OBJ) { c.rot = CHE_OBJ; c.rot_obj = c.pos_obj; }
    else { c.rot = CHE_BONE; c.rot_obj = game_value(); }
    c.rd_rot = ChainRead();
}

// CHAIN - a plain object that simulates is drawn at its authored size in the box its shape needs
// (chain_writeback); until the first result is installed its own rest buffer draws, in the rest box. For the command
// that switches the chain on, the one that changes its mesh, and a position that ends a skeletal binding. GAME THREAD,
// on the staged copy; staged lookup, as apply_native_size.
inline void chain_rest_size(RenderObject& o) {
    const MeshDef* sd = mesh_def_staged(o.mesh);
    const MeshDef& d = sd ? *sd : mesh_def(0);
    for (int k = 0; k < 3; ++k) o.size[k] = d.native_size[k];
}

// CHAIN - the end's targets for this frame, in the instance's frame (org its floating origin). A followed object
// that could not be read this frame leaves its lane free.
inline void chain_target_of(const ChainCfg* c, const double* org, ChainTarget& t) {
    t = ChainTarget();
    if (!c) return;
    switch (c->pos) {
    case CHE_BONE: t.pos = 1u; break;
    case CHE_HOLD: t.pos_hold = true; break;
    case CHE_WORLD: {
        const double w[3] = { c->pos_w[0], c->pos_w[2], c->pos_w[1] };
        for (int k = 0; k < 3; ++k) t.p[k] = static_cast<float>(w[k] - org[k]);
        t.pos = 2u;
        break;
    }
    case CHE_OBJ:
    case CHE_MEM:
        if (c->live_p) {
            for (int k = 0; k < 3; ++k) t.p[k] = static_cast<float>(c->live_pos[k] - org[k]);
            t.pos = 2u;
        }
        break;
    default: break;
    }
    switch (c->rot) {
    case CHE_BONE: t.rot = 1u; break;
    case CHE_HOLD: t.rot_hold = true; break;
    case CHE_WORLD: memcpy(t.r, c->rot_w, sizeof(t.r)); t.rot = 2u; break;
    case CHE_OBJ:
    case CHE_MEM:
        if (c->live_r) { memcpy(t.r, c->live_rot, sizeof(t.r)); t.rot = 2u; }
        break;
    default: break;
    }
}

// CHAIN_END_DRAW - the correction on one chain's bone maps (carrier-local: 3x3 images of the axes, then the
// translation): dp the end's move, dq its turn, about the point the end is held by. Along the chain's
// path from its start to the end body: the end body and everything below it take both; a link between the path's
// last kinematic body and the end a share of the move growing with its place on the path, and whatever branches
// off such a link the link's share (it stays on it); the rest - the kinematic start, what branches off before it,
// carrier / skeleton bones - nothing. Bodies are numbered parents before children (chain_build), so on an
// unbranched chain the path is the numbering. anc: scratch.
inline void chain_end_fix(const ChainState& c, std::vector<float>& m, const float dp[3],
                          const float dq[4], std::vector<int32_t>& anc) {
    const int32_t eb = c.end_body;
    const size_t nb = c.body_of.size();
    const size_t nd = c.body.size();
    if (eb < 0 || static_cast<size_t>(eb) >= nd || m.size() != nb * 12u) return;
    const ChainBody& e = c.body[static_cast<size_t>(eb)];
    // anc[0, nd): a body's place on the path (0 the start), or -1; [nd, 2 nd): the path body it hangs from.
    anc.assign(nd * 2u, -1);
    int32_t ep = 0;   // The end's place: its depth.
    for (int32_t p = c.body[static_cast<size_t>(eb)].parent, n = 0; p >= 0 && n < static_cast<int32_t>(nd);
         p = c.body[static_cast<size_t>(p)].parent, ++n) {
        if (static_cast<size_t>(p) >= nd) return;
        ++ep;
    }
    for (int32_t p = eb, d = ep; p >= 0 && d >= 0; p = c.body[static_cast<size_t>(p)].parent, --d) {
        anc[static_cast<size_t>(p)] = d;
    }
    int32_t s0 = -1;   // The place of the path's last kinematic body before the end.
    for (size_t b = 0; b < nd; ++b) {
        const int32_t d = anc[b];
        if (d >= 0 && d < ep && !(c.body[b].inv_m > 0.0f) && d > s0) s0 = d;
    }
    for (size_t b = 0; b < nd; ++b) {   // Parents before children: a parent's answer is in.
        const int32_t p = c.body[b].parent;
        if (anc[b] >= 0) anc[nd + b] = static_cast<int32_t>(b);
        else if (p >= 0 && static_cast<size_t>(p) < b) {
            anc[nd + b] = anc[nd + static_cast<size_t>(p)];
        }
    }
    const float* me = nullptr;
    for (size_t bi = 0; bi < nb; ++bi) if (c.body_of[bi] == eb) { me = &m[bi * 12u]; break; }
    if (!me) return;
    float lp[3], pv[3];   // The held point: the end body's centre plus end_pt, where the bone puts it.
    for (int k = 0; k < 3; ++k) lp[k] = e.x0[k] + (e.parent >= 0 ? c.end_pt[k] : 0.0f);
    for (int k = 0; k < 3; ++k) {
        pv[k] = lp[0] * me[k] + lp[1] * me[3 + k] + lp[2] * me[6 + k] +
                me[9 + k];
    }
    for (size_t bi = 0; bi < nb; ++bi) {
        const int32_t bb = c.body_of[bi];
        if (bb < 0 || static_cast<size_t>(bb) >= nd) continue;
        const int32_t ba = anc[nd + static_cast<size_t>(bb)];   // The path body it moves with.
        if (ba < 0) continue;
        const int32_t bd = anc[static_cast<size_t>(ba)];   // That body's place.
        if (bd <= s0) continue;
        float* mm = &m[bi * 12u];
        if (bd >= ep) {
            for (int a = 0; a < 3; ++a) {
                const float v[3] = { mm[a * 3], mm[a * 3 + 1], mm[a * 3 + 2] };
                q_rot(dq, v, &mm[a * 3]);
            }
            const float d[3] = { mm[9] - pv[0], mm[10] - pv[1], mm[11] - pv[2] };
            float r[3];
            q_rot(dq, d, r);
            for (int k = 0; k < 3; ++k) mm[9 + k] = pv[k] + dp[k] + r[k];
        } else {
            const float w = static_cast<float>(bd - s0) / static_cast<float>(ep - s0);
            for (int k = 0; k < 3; ++k) mm[9 + k] += w * dp[k];
        }
    }
}
// CHAIN_END_DRAW - the corrected shape stored as chain_writeback stores a shape: every vertex skinned through
// mat (nb bones) into met (carrier-local metres, 3 a vertex) and out, in the smallest box
// centred on the front's (bc: its centre, carrier-local) that holds the level-0 vertices and is never smaller
// than the front's (bf) - handed back in be. The corrected end can leave the front's box, and the object's
// bounds are the box of its bytes. Nothing moved out, the box and the bytes are the front's formula's.
inline void chain_end_store(const MeshVertex* rest, size_t nv, const float ext[3],
                            const SkinInf* inf, const uint8_t* lvl0, const float* mat,
                            size_t nb, const float bc[3], const float bf[3],
                            std::vector<float>& met, MeshVertex* out, float be[3]) {
    met.resize(nv * 3u);
    float cs_r[3] = { 0.0f, 0.0f, 0.0f };   // The farthest level-0 vertex from the centre, per axis.
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& r = rest[i];
        MeshVertex& o = out[i];
        float pp[3], nn[3], tt[3];
        chain_skin_vert(r, ext, inf ? &inf[i] : nullptr, mat, nb, pp, nn, tt);
        if (!lvl0 || lvl0[i] != 0u) {   // The writeback's box rule: level 0.
            for (int k = 0; k < 3; ++k) {
                const float d = fabsf(pp[k] - bc[k]);
                if (d > cs_r[k]) cs_r[k] = d;
            }
        }
        memcpy(&met[i * 3u], pp, sizeof(pp));
        memcpy(o.nrm, nn, sizeof(nn));
        memcpy(o.tan, tt, sizeof(tt));
        o.tan[3] = r.tan[3];
        o.uv[0] = r.uv[0];
        o.uv[1] = r.uv[1];
    }
    for (int k = 0; k < 3; ++k) be[k] = 2.0f * cs_r[k] > bf[k] ? 2.0f * cs_r[k] : bf[k];
    for (size_t i = 0; i < nv; ++i) {
        MeshVertex& o = out[i];
        const float* p = &met[i * 3u];
        float n[3], tn[3];
        for (int k = 0; k < 3; ++k) {
            o.pos[k] = (p[k] - bc[k]) / be[k];
            n[k] = o.nrm[k] * be[k];
            tn[k] = o.tan[k] / be[k];
        }
        const float nl = cloth_v3_len(n), tl = cloth_v3_len(tn);
        for (int k = 0; k < 3; ++k) {
            o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : rest[i].nrm[k];
            o.tan[k] = tl > 1.0e-30f ? tn[k] / tl : rest[i].tan[k];
        }
    }
}
// CHAIN_END_DRAW - a position target p (carrier-local, in place) as chain_step holds it: a target
// further from the pin than the chain can reach (ChainState::end_reach) is pulled in to that reach along the same
// line - the pin is body 0's point where the end's path hangs off it (joint end_root - 1's ra), placed by the bone
// maps ct_m (the solver's step: body 0's pose at the step's end, which its map is). The job held its end at its
// own target so taken; the correction takes this frame's the same way, so a taut chain stays taut, not stretched.
// CHAIN_REACH_ROT: q, the end's held orientation (carrier-local) where its rotation is held too, else null -
// the solver's sphere (chain_end_reach_rot), so the correction and the hold agree.
inline void chain_end_taut(const ChainState& c, const std::vector<float>& ct_m, float p[3],
                           const float* q) {
    if (!(c.end_reach >= 0.0f) || c.end_root <= 0 || c.body.empty() ||   // CHAIN_REACH_ZERO (chain_substep).
        static_cast<size_t>(c.end_root) > c.joint.size()) return;
    const int32_t b = c.body[0].bone;
    if (b < 0 || (static_cast<size_t>(b) + 1u) * 12u > ct_m.size()) return;
    const float* const m = &ct_m[static_cast<size_t>(b) * 12u];
    const float* const ra = c.joint[static_cast<size_t>(c.end_root) - 1u].ra;
    float l[3], pin[3], d[3];
    for (int k = 0; k < 3; ++k) l[k] = c.body[0].x0[k] + ra[k];
    for (int k = 0; k < 3; ++k) pin[k] = l[0] * m[k] + l[1] * m[3 + k] + l[2] * m[6 + k] + m[9 + k];
    const float r = chain_end_reach_rot(c, q, pin);
    cloth_v3_sub(p, pin, d);
    const float len = cloth_v3_len(d);
    if (len > r && len > 1.0e-9f) {
        const float sc = r / len;
        for (int k = 0; k < 3; ++k) p[k] = pin[k] + d[k] * sc;
    }
}
// CHAIN_END_DRAW - an end held on the bound skeleton's bone (kind 1: endPosition / endRotation true), where the
// skeleton pose g (nb maps in skin_affine's layout, or null: the rest) puts it, carrier-local - what
// chain_end_pos / chain_end_rot hold at a step's end (chain_kin at fraction 1: the bone map, the body
// placed by chain_place with its start offset, then the endPoint), less the carrier frame. q: the body's
// orientation; p: the held point.
inline void chain_end_bone(const ChainState& c, const float* g, size_t nb, float q[4],
                           float p[3]) {
    const size_t e = static_cast<size_t>(c.end_body);
    const ChainBody& E = c.body[e];
    float t[3], l[3];
    chain_bone_map(c, g, g, nb, E.bone, 1.0f, q, t);
    q_rot(q, E.x0, l);
    const float* const off = chain_start_off(c, e);
    for (int k = 0; k < 3; ++k) l[k] += t[k] + (off ? off[k] : 0.0f);
    if (E.parent < 0) { memcpy(p, l, sizeof(l)); return; }
    float d[3];
    q_rot(q, c.end_pt, d);   // CHAIN_POINT.
    for (int k = 0; k < 3; ++k) p[k] = l[k] + d[k];
}
}