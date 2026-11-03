#pragma once

// The feature half (rendering_integration_features.hpp): CLOTH runtime: the collider BVH registry, cloth building
// and write-back, the shape notes, the collider gather and the ground patch. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/skinning.hpp (declared here for the uses below).
inline bool skin_col_fresh(uint64_t s);


inline uint64_t physics_bvh_key(int mesh, uint32_t shape) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(mesh)) << 32) | shape;
}

inline std::shared_ptr<const PhysicsBvh> physics_bvh_for(int mesh, uint32_t shape, const float* bake) {
    const uint64_t key = physics_bvh_key(mesh, shape);
    {
        std::lock_guard<std::mutex> g(g_physics_bvh_mu);
        const auto it = g_physics_bvh.find(key);
        if (it != g_physics_bvh.end()) { it->second.used = g_physics_bvh_frame; return it->second.bvh; }
    }
    // Built OUTSIDE the lock. The one caller is physics_gather_colliders, on the game thread: the build is paid
    // there, once per collider mesh and shape (the key), on the first frame that shape collides (a large collider
    // costs that frame). Two threads racing on one key both build and the recheck below drops the loser's copy: a
    // duplicated build, never a torn map.
    std::shared_ptr<PhysicsBvh> b = std::make_shared<PhysicsBvh>();
    if (mesh_alive(mesh)) {
        const MeshDef& d = mesh_def(mesh);
        const uint32_t ni = mesh_base_icount(d);
        std::vector<float> p(d.verts.size() * 3u);
        for (size_t i = 0; i < d.verts.size(); ++i) {
            p[i * 3u]      = d.verts[i].pos[0] * bake[0];
            p[i * 3u + 1u] = d.verts[i].pos[1] * bake[1];
            p[i * 3u + 2u] = d.verts[i].pos[2] * bake[2];
        }
        physics_bvh_build(*b, p.data(), static_cast<uint32_t>(d.verts.size()),
                           d.indices.data(), ni);
    }
    std::lock_guard<std::mutex> g(g_physics_bvh_mu);
    const auto it = g_physics_bvh.find(key);
    if (it != g_physics_bvh.end()) { it->second.used = g_physics_bvh_frame; return it->second.bvh; }
    g_physics_bvh.emplace(key, PhysicsBvhEntry{ b, g_physics_bvh_frame });
    return b;
}

// A simulated particle's inverse mass. Cloth has uniform mass (cloth_build), so this is the value of every w that
// is not zero; the build and cloth_sync's live 'mass' update both take it from here.
inline float cloth_inv_mass(float mass) {
    return 1.0f / (mass > 1.0e-4f ? mass : 1.0e-4f);
}

// Build the particle set, the constraint sets and the render-vertex map from a mesh's level-0 topology. Particles
// are welded by POSITION, not by vertex identity: the mesh's own weld splits a vertex wherever a UV, a normal or a
// tangent differs, and simulating those as independent points opens a hole along every seam.
inline bool cloth_build(ClothInst& in, const MeshDef& d, uint32_t slot, int mesh) {
    in.st.part.clear(); in.st.stretch.clear(); in.st.bend.clear();
    in.st.tri.clear();
    in.vmap.clear();
    if (d.verts.empty() || d.indices.empty()) return false;
    if (d.cloth_w.size() != d.verts.size()) return false;   // Not a cloth mesh.
    const uint32_t ni = mesh_base_icount(d);
    if (ni < 3u) return false;
    const uint32_t nv = static_cast<uint32_t>(d.verts.size());

    // Position weld.
    std::unordered_map<ClothPosKey, uint32_t, ClothPosHash> map;
    map.reserve(nv / 2u + 16u);
    in.vmap.assign(nv, 0xFFFFFFFFu);
    std::vector<float> wsum;
    for (uint32_t i = 0; i < ni; ++i) {
        const uint32_t v = d.indices[i];
        if (v >= nv || in.vmap[v] != 0xFFFFFFFFu) continue;
        ClothPosKey k;
        memcpy(k.p, d.verts[v].pos, sizeof(k.p));
        auto it = map.find(k);
        if (it == map.end()) {
            const uint32_t id = static_cast<uint32_t>(in.st.part.size());
            ClothPart p = {};
            memcpy(p.rest, d.verts[v].pos, sizeof(p.rest));
            p.anchor = id;
            p.tether = -1.0f;
            in.st.part.push_back(p);
            wsum.push_back(0.0f);
            it = map.emplace(k, id).first;
        }
        in.vmap[v] = it->second;
        // A welded particle takes the LARGEST weight of the render vertices on it (the importer's seam rule).
        const float w = static_cast<float>(d.cloth_w[v]) * (1.0f / 255.0f);
        if (w > wsum[it->second]) wsum[it->second] = w;
    }
    const uint32_t np = static_cast<uint32_t>(in.st.part.size());
    if (np < 3u) return false;

    // Unique edges, and the bend pairs across every edge shared by two faces.
    std::unordered_map<uint64_t, uint32_t> edge;   // key -> opposite particle of the first face.
    edge.reserve(ni);
    auto key = [](uint32_t x, uint32_t y) -> uint64_t {
        return x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
    };
    auto add = [&](std::vector<ClothDist>& set, uint32_t x, uint32_t y) {
        if (x == y || x >= np || y >= np) return;
        ClothDist c;
        c.a = x; c.b = y;
        c.rest = 0.0f;   // Filled once the object's size is known.
        set.push_back(c);
    };
    for (uint32_t t = 0; t + 2u < ni; t += 3u) {
        const uint32_t r[3] = { d.indices[t], d.indices[t + 1u], d.indices[t + 2u] };
        if (r[0] >= nv || r[1] >= nv || r[2] >= nv) continue;
        const uint32_t p[3] = { in.vmap[r[0]], in.vmap[r[1]], in.vmap[r[2]] };
        if (p[0] >= np || p[1] >= np || p[2] >= np) continue;
        // The face, for wind's orientation. A triangle welded onto fewer than three particles has no area or facing.
        if (p[0] != p[1] && p[1] != p[2] && p[0] != p[2]) {
            ClothTri ft;
            ft.a = p[0]; ft.b = p[1]; ft.c = p[2];
            in.st.tri.push_back(ft);
        }
        for (int e = 0; e < 3; ++e) {
            const uint32_t a = p[e], b = p[(e + 1) % 3];
            const uint32_t op = p[(e + 2) % 3];
            if (a == b) continue;
            const uint64_t kk = key(a, b);
            const auto it = edge.find(kk);
            if (it == edge.end()) {   // The edge's first face: its stretch constraint.
                add(in.st.stretch, a, b);
                edge.emplace(kk, op);
            } else if (it->second != op) {
                // The two corners facing each other across a shared edge: a distance constraint between them resists
                // folding without a dihedral solve, which keeps this affordable per substep.
                add(in.st.bend, it->second, op);
            }
        }
    }

    for (uint32_t i = 0; i < np; ++i) {
        ClothPart& pp = in.st.part[i];
        const float w = wsum[i];
        // INVERSE MASS CARRIES NO PART OF THE GRADIENT. Do not scale it by the paint weight: gravity reaches a
        // particle whatever its mass while every constraint correction is scaled BY inverse mass, so a barely-painted
        // vertex would fall at a full g and be held by nothing. Cloth has uniform mass.
        pp.w = w > 0.0f ? cloth_inv_mass(in.par.mass) : 0.0f;
        // The gradient itself, as the attachment's compliance. Harmonic in the paint weight: 0 (welded to the rest
        // pose) at the pinned end, unbounded at the simulated end, so it fades out with no step to cut at the top.
        // Zero is the sentinel for 'no attachment' and a fully simulated vertex takes it (the limit's own answer).
        // CLOTH_ATT_SOFT centres the curve so that painting half sags about half.
        pp.att = w >= 1.0f ? 0.0f
                                     : CLOTH_ATT_SOFT * w / (1.0f - w);
    }

    in.mesh_edges = static_cast<uint32_t>(in.st.stretch.size());
    in.shell_built = -1.0f;
    // One authored normal per particle, from the first render vertex that landed on it. A UV seam splits a vertex
    // without splitting the normal, so any copy answers; a smoothing split's sides disagree, and either answers
    // 'which way does this surface face'.
    in.part_nrm.assign(static_cast<size_t>(np) * 3u, 0.0f);
    {
        std::vector<uint8_t> seen_n(np, 0u);
        for (uint32_t v = 0; v < nv; ++v) {
            const uint32_t pi = in.vmap[v];
            if (pi >= np || seen_n[pi]) continue;
            seen_n[pi] = 1u;
            memcpy(&in.part_nrm[static_cast<size_t>(pi) * 3u],
                   d.verts[v].nrm, sizeof(float) * 3u);
        }
    }
    // SKEL: the influences, per particle, from the same first render vertex.
    in.part_inf.clear();
    if (d.skin_inf.size() == nv) {
        in.part_inf.assign(np, SkinInf());
        std::vector<uint8_t> seen_i(np, 0u);
        for (uint32_t v = 0; v < nv; ++v) {
            const uint32_t pi = in.vmap[v];
            if (pi >= np || seen_i[pi]) continue;
            seen_i[pi] = 1u;
            in.part_inf[pi] = d.skin_inf[v];
        }
    }
    memcpy(in.auth_ctr, d.native_ctr, sizeof(in.auth_ctr));
    memcpy(in.auth_ext, d.native_ext, sizeof(in.auth_ext));
    // The writeback's smoothing groups. Every render vertex on a particle whose authored normal matches an earlier
    // one's (CLOTH_NRM_JOIN) names that earlier vertex, so a UV seam's copies share one accumulator while a hard
    // edge's copies (differing authored normals) each keep their own. The chain per particle is the handful of
    // distinct normals meeting there.
    in.nrm_rep.resize(nv);
    {
        std::vector<uint32_t> head(np, 0xFFFFFFFFu);
        std::vector<uint32_t> next(nv, 0xFFFFFFFFu);
        for (uint32_t v = 0; v < nv; ++v) {
            in.nrm_rep[v] = v;
            const uint32_t pi = in.vmap[v];
            if (pi >= np) continue;
            // CLOTH_JOIN_AUTH: compared as authored (n / native_ext, the skin runtime's NORMALS note): the stored
            // normals carry each axis by its extent, and on a thin or flat sheet (IMPORT_FLAT) that squash makes
            // normals differing across the sheet's plane read as one.
            float vn[3], rn[3];
            for (int k = 0; k < 3; ++k) {
                vn[k] = d.verts[v].nrm[k] / d.native_ext[k];
            }
            const float vl = cloth_v3_len(vn);
            for (uint32_t r = head[pi]; r != 0xFFFFFFFFu; r = next[r]) {
                for (int k = 0; k < 3; ++k) {
                    rn[k] = d.verts[r].nrm[k] / d.native_ext[k];
                }
                const float den = vl * cloth_v3_len(rn);
                if (den > 1.0e-12f && cloth_v3_dot(vn, rn) >= CLOTH_NRM_JOIN * den) {
                    in.nrm_rep[v] = r;
                    break;
                }
            }
            if (in.nrm_rep[v] == v) {
                next[v] = head[pi];
                head[pi] = v;
            }
        }
    }
    in.rest = d.verts;
    in.out[0] = d.verts;
    in.out[1] = d.verts;
    in.out_guide[0].clear();   // CLOTH_PIN_GRADE: the rest pose was solved against no guide.
    in.out_guide[1].clear();
    in.guide_held.clear();   // CLOTH_HELD_REC.
    in.box_on[0] = in.box_on[1] = false;   // SKEL: both start in the rest box.
    in.uploaded_box_on = false;
    in.sim_out.clear();
    in.nrm_acc.assign(static_cast<size_t>(nv) * 3u, 0.0f);
    in.tan_acc.assign(static_cast<size_t>(nv) * 3u, 0.0f);
    in.lvl0_idx.assign(d.indices.begin(), d.indices.begin() + ni);   // CLOTH_OWN_IDX.
    in.mesh = mesh;
    in.slot = slot;
    in.st.primed = false;
    in.st.accum = 0.0f;
    in.st.h_last = 0.0f;   // SIM_LOD.
    in.st.speed_mean = 0.0f;
    in.built = true;
    return true;
}

// THICK CLOTH: bind opposing surfaces so a solidified mesh behaves as one piece. Rebuilt rather than appended, so
// calling this twice cannot double the links - the mesh edges are the prefix and everything after them is ours.
// A pair qualifies only when all three hold: the two particles are within 'shell' metres, their authored normals
// OPPOSE, and the second lies along the first's BACKWARD normal rather than merely beside it. The third keeps it from
// fusing a fold: two faces of a pleat passing close satisfy the first two and fail the third unless they are
// genuinely back to back, at which point they are a shell.
inline void cloth_build_shell(ClothInst& in, const float* size) {
    if (in.st.stretch.size() > in.mesh_edges) in.st.stretch.resize(in.mesh_edges);
    in.shell_built = in.par.shell;
    const float r = in.par.shell;
    if (!(r > 0.0f)) return;
    const size_t n = in.st.part.size();
    if (n < 2u || in.part_nrm.size() < n * 3u) return;
    // The search runs in metres: the normalized rest positions go through the object's size as the draw does.
    const float org[3] = { 0.0f, 0.0f, 0.0f };
    const float id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    std::vector<float> w(n * 3u);
    for (size_t i = 0; i < n; ++i) {
        cloth_l2w(in.st.part[i].rest, org, id, false, size, &w[i * 3u]);
    }
    // The normals too. Stored normals live in NORMALIZED mesh space - the draw divides them by size (KhVsCore) - so
    // each goes through the same inverse scale before either test below holds it against a metric offset. Do not
    // mix the spaces: a thin box then tilts every normal away from its short axis, and on pleated thick cloth the
    // 'behind' test missed 55 of 405 front/back pairs at 5 mm pleats and cross-linked 50 extra at 10 mm.
    std::vector<float> nm(n * 3u, 0.0f);
    for (size_t i = 0; i < n; ++i) {
        const float* a = &in.part_nrm[i * 3u];
        if (cloth_v3_dot(a, a) < 0.25f) continue;   // No usable normal: stays zero.
        float* m = &nm[i * 3u];
        for (int k = 0; k < 3; ++k) m[k] = a[k] / (fabsf(size[k]) < 1.0e-6f ? 1.0e-6f : size[k]);
        const float ml = cloth_v3_len(m);
        if (ml > 1.0e-20f) for (int k = 0; k < 3; ++k) m[k] /= ml;
        else m[0] = m[1] = m[2] = 0.0f;
    }
    // Uniform grid at the search radius: one cell step in each direction covers every candidate, so this stays
    // linear in the particle count. The cell is never under 2^-19 of the farthest particle's offset, so every cell
    // index fits the key's 21 bits (+-2^20) and its int conversion whatever shell a script gives (for every finite
    // placement): a larger cell only adds candidates the distance test turns away.
    // CLOTH_SHELL_FINITE: a size that is not finite (a script's 1e39, or a multiplier that overflows the native
    // size) places particles at infinity or NaN, and the cell index's int conversion of either is undefined; such a
    // cloth has no metric shell to bind, so it links none.
    float ext = 0.0f;
    for (size_t i = 0; i < n * 3u; ++i) {
        if (!(fabsf(w[i]) <= 3.0e38f)) return;
        ext = fmaxf(ext, fabsf(w[i]));
    }
    const float cell = fmaxf(r, ext * (1.0f / 524288.0f));
    std::unordered_map<uint64_t, std::vector<uint32_t>> grid;
    grid.reserve(n * 2u);
    auto key = [&](int x, int y, int z) -> uint64_t {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x + 1048576)) << 42) ^
               (static_cast<uint64_t>(static_cast<uint32_t>(y + 1048576)) << 21) ^
                static_cast<uint64_t>(static_cast<uint32_t>(z + 1048576));
    };
    auto cellof = [&](const float* p, int* o) {
        for (int k = 0; k < 3; ++k) o[k] = static_cast<int>(floorf(p[k] / cell));
    };
    for (uint32_t i = 0; i < n; ++i) {
        int c[3]; cellof(&w[i * 3u], c);
        grid[key(c[0], c[1], c[2])].push_back(i);
    }
    // Never link a pair the mesh already joins.
    std::unordered_map<uint64_t, uint8_t> have;
    have.reserve(in.mesh_edges * 2u + 16u);
    auto pk = [](uint32_t x, uint32_t y) -> uint64_t {
        return x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
    };
    for (uint32_t e = 0; e < in.mesh_edges; ++e) {
        have.emplace(pk(in.st.stretch[e].a, in.st.stretch[e].b), 1u);
    }
    for (uint32_t i = 0; i < n; ++i) {
        const float* pi = &w[i * 3u];
        const float* ni = &nm[i * 3u];
        if (cloth_v3_dot(ni, ni) < 0.25f) continue;   // No usable normal.
        uint32_t best = 0xFFFFFFFFu;
        float    bd = r;
        int c[3]; cellof(pi, c);
        for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) for (int dz = -1; dz <= 1; ++dz) {
            const auto it = grid.find(key(c[0] + dx, c[1] + dy, c[2] + dz));
            if (it == grid.end()) continue;
            for (size_t k = 0; k < it->second.size(); ++k) {
                const uint32_t j = it->second[k];
                if (j == i) continue;
                const float* pj = &w[j * 3u];
                float d[3];
                cloth_v3_sub(pj, pi, d);
                const float l = cloth_v3_len(d);
                if (l >= bd || l < 1.0e-9f) continue;
                const float* nj = &nm[j * 3u];
                if (cloth_v3_dot(ni, nj) > -0.5f) continue;        // Not opposing.
                if (cloth_v3_dot(d, ni) / l > -0.5f) continue;   // Beside, not behind.
                bd = l; best = j;
            }
        }
        if (best == 0xFFFFFFFFu) continue;
        if (!have.emplace(pk(i, best), 1u).second) continue;
        ClothDist cl_c;
        cl_c.a = i; cl_c.b = best; cl_c.rest = 0.0f;   // Filled by the caller.
        in.st.stretch.push_back(cl_c);
    }
}

// Rest lengths are metric, so they must be re-derived whenever the object's size changes (cloth_sync also asks on a
// fresh build and on a 'shell' change). The shell links are rebuilt first: their search radius is metric too.
inline void cloth_rest_lengths(ClothInst& in, const float* size) {
    cloth_build_shell(in, size);
    const float org[3] = { 0.0f, 0.0f, 0.0f };
    const float id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    std::vector<ClothDist>* sets[2] = { &in.st.stretch, &in.st.bend };
    for (int s = 0; s < 2; ++s) {
        for (size_t i = 0; i < sets[s]->size(); ++i) {
            ClothDist& c = (*sets[s])[i];
            float a[3], b[3];
            cloth_l2w(in.st.part[c.a].rest, org, id, false, size, a);
            cloth_l2w(in.st.part[c.b].rest, org, id, false, size, b);
            float d[3];
            cloth_v3_sub(b, a, d);
            c.rest = cloth_v3_len(d);
        }
    }
    cloth_build_tethers(in.st.part, in.st.stretch);
}

// Write the stepped particles back into a MeshVertex array and rebuild the normals. Positions go back to MESH-LOCAL
// space: the cloth substitutes a vertex buffer, it does not change any pass's object transform.
inline void cloth_writeback(ClothInst& in, std::vector<MeshVertex>& out) {
    const size_t nv = in.rest.size();
    float inv[3];
    for (int k = 0; k < 3; ++k) {
        const float s = fabsf(in.st.size[k]) < 1.0e-6f ? 1.0e-6f : in.st.size[k];
        inv[k] = 1.0f / s;
    }
    for (size_t i = 0; i < nv; ++i) {
        const uint32_t p = i < in.vmap.size() ? in.vmap[i] : 0xFFFFFFFFu;
        if (p >= in.st.part.size()) continue;   // An LOD-only vertex keeps its rest pose.
        cloth_w2l(in.st.part[p].p, in.st.centre, in.st.rot,
                  in.st.rotated, inv, out[i].pos);
    }
    // Normals, accumulated per SMOOTHING GROUP (cloth_build's nrm_rep): neither per particle, which rounds off
    // every hard edge the model was built with, nor per render vertex, which splits a UV seam as well - each copy of
    // a seam vertex then sees only the faces on its own side, and the two disagree by up to a whole face angle
    // wherever the cloth bends across the seam (a lit line along it).
    std::fill(in.nrm_acc.begin(), in.nrm_acc.end(), 0.0f);
    // Tangents, per RENDER vertex: a UV seam's copies carry different tangents by construction, each of its own
    // chart. The mesh's tangents describe the REST shape; the pixel shader's re-orthogonalisation recovers a bend
    // about the tangent, or one across it below 90 degrees, and nothing else (a patch turned in its plane would turn
    // its normal map with it). A tangent is a pushforward: each face maps its rest edges onto its deformed edges, and
    // that map applied to the authored tangent is the deformed one - the authored frame at rest to float precision,
    // exact per face for any bend, turn or shear. Rest and deformed positions are both normalized mesh space, the
    // covariant space KhVsCore scales by size.
    std::fill(in.tan_acc.begin(), in.tan_acc.end(), 0.0f);
    const bool tan = in.tan_acc.size() == nv * 3u;
    const bool grp = in.nrm_rep.size() == nv;
    // CLOTH_OWN_IDX: the instance's own copy of level 0's indices. Do not read mesh_def(mesh).indices here: the
    // worker has no hand-off from mesh_gc (the cache writer has the busy id; this has nothing) and would read the
    // slot pointer without the lock its tombstone write holds.
    const std::vector<uint32_t>& ix = in.lvl0_idx;
    const uint32_t ni = static_cast<uint32_t>(ix.size());
    for (uint32_t t = 0; t + 2u < ni; t += 3u) {
        const uint32_t a = ix[t], b = ix[t + 1u], c = ix[t + 2u];
        if (a >= nv || b >= nv || c >= nv) continue;
        float e1[3], e2[3], n[3];
        cloth_v3_sub(out[b].pos, out[a].pos, e1);
        cloth_v3_sub(out[c].pos, out[a].pos, e2);
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        // Unnormalised: the cross product's length is twice the triangle area, so accumulating it is an area weighting.
        const uint32_t v[3] = { a, b, c };
        for (int cw_k = 0; cw_k < 3; ++cw_k) {
            const uint32_t r = grp ? in.nrm_rep[v[cw_k]] : v[cw_k];
            float* acc = &in.nrm_acc[static_cast<size_t>(r) * 3u];
            acc[0] += n[0]; acc[1] += n[1]; acc[2] += n[2];
        }
        if (!tan) continue;
        // The face's rest edges r1, r2 and rest normal rn. A tangent t splits into t.rn along rn and an in-plane part
        // ca * r1 + cb * r2 (solved through the edges' Gram matrix, whose determinant is |rn|^2); the face carries r1
        // to e1, r2 to e2 and the unit rest normal to the unit deformed one.
        float r1[3], r2[3], rn[3];
        cloth_v3_sub(in.rest[b].pos, in.rest[a].pos, r1);
        cloth_v3_sub(in.rest[c].pos, in.rest[a].pos, r2);
        rn[0] = r1[1] * r2[2] - r1[2] * r2[1];
        rn[1] = r1[2] * r2[0] - r1[0] * r2[2];
        rn[2] = r1[0] * r2[1] - r1[1] * r2[0];
        const float rn2 = cloth_v3_dot(rn, rn);
        const float nl = cloth_v3_len(n);
        if (!(rn2 > 1.0e-30f) || !(nl > 1.0e-15f)) continue;   // No face to map through.
        const float rnl = sqrtf(rn2);
        const float g11 = cloth_v3_dot(r1, r1), g12 = cloth_v3_dot(r1, r2);
        const float g22 = cloth_v3_dot(r2, r2);
        for (int cw_k = 0; cw_k < 3; ++cw_k) {
            const float* t = in.rest[v[cw_k]].tan;
            const float tc = cloth_v3_dot(t, rn) / rn2;
            const float tp[3] = { t[0] - tc * rn[0], t[1] - tc * rn[1],
                                  t[2] - tc * rn[2] };
            const float q1 = cloth_v3_dot(r1, tp), q2 = cloth_v3_dot(r2, tp);
            const float ca = (g22 * q1 - g12 * q2) / rn2;
            const float cb = (g11 * q2 - g12 * q1) / rn2;
            const float cn = tc * rnl / nl;
            // Weighted by the face's rest area, as the normals are by theirs.
            float* ta = &in.tan_acc[static_cast<size_t>(v[cw_k]) * 3u];
            for (int q = 0; q < 3; ++q) {
                ta[q] += rnl * (ca * e1[q] + cb * e2[q] + cn * n[q]);
            }
        }
    }
    for (size_t i = 0; i < nv; ++i) {
        const uint32_t p = i < in.vmap.size() ? in.vmap[i] : 0xFFFFFFFFu;
        if (p >= in.st.part.size()) continue;
        if (tan) {
            // The handedness in w is the chart's and does not deform. A vertex with no face to map through keeps
            // what this buffer last carried, as its normal does below.
            const float* ta = &in.tan_acc[i * 3u];
            const float tl = cloth_v3_len(ta);
            if (tl > 1.0e-20f) {
                out[i].tan[0] = ta[0] / tl;
                out[i].tan[1] = ta[1] / tl;
                out[i].tan[2] = ta[2] / tl;
            }
        }
        const size_t r = grp ? in.nrm_rep[i] : i;
        const float* acc = &in.nrm_acc[r * 3u];
        const float l = cloth_v3_len(acc);
        // A fully degenerate fan keeps the normal this buffer last carried (the authored one until a frame has a face).
        if (l < 1.0e-12f) continue;
        out[i].nrm[0] = acc[0] / l;
        out[i].nrm[1] = acc[1] / l;
        out[i].nrm[2] = acc[2] / l;
    }
}

inline uint32_t cloth_shape_note(ClothInst& in, const std::vector<MeshVertex>& out) {
    const size_t n = out.size();
    bool moved = in.shape_key == 0u || in.shape_ref.size() != n * 3u;
    if (!moved) {
        // The positions are mesh-local, divided by size per axis.
        float tol[3];
        for (int k = 0; k < 3; ++k) {
            const float s = fabsf(in.st.size[k]);
            tol[k] = CLOTH_SHAPE_M / (s > 1.0e-6f ? s : 1.0e-6f);
        }
        for (size_t i = 0; i < n && !moved; ++i) {
            const float* r = &in.shape_ref[i * 3u];
            for (int k = 0; k < 3; ++k) {
                if (!(fabsf(out[i].pos[k] - r[k]) <= tol[k])) { moved = true; break; }
            }
        }
    }
    if (moved) {
        in.shape_ref.resize(n * 3u);
        for (size_t i = 0; i < n; ++i) {
            memcpy(&in.shape_ref[i * 3u], out[i].pos, sizeof(float) * 3u);
        }
        uint32_t sn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (sn_k == 0u) sn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        in.shape_key = sn_k;
    }
    return in.shape_key;
}

// SKEL - the skeleton's targets for this frame, ON THE WORKER, from the inputs: the frame's end into guide_b, and
// into guide_a where the targets stood at the simulation's time - sub_t of the way from what guide_a held to what
// guide_b held (SUB_CLOCK), the last frame's end itself when nothing was left over. A particle something pins,
// wholly or in part, goes where its bones take its authored position, re-expressed in the frame's rest box (the
// frame's centre stands for the authored native_ctr - cloth_sync); a simulated particle keeps its rest position
// (the simulation moves it). The first guided frame starts where it ends; an unguided frame clears both, and every
// target is 'rest' again.
// One particle's guided position: its rest, skinned by its influences through the guide's maps (linear blend
// skinning as skin_run does it, missing weight to the root, the identity), in the cloth's local frame - metres
// from auth_ctr over is (1 / the size). The rest and the influences are the build's; the guide may be any frame's.
inline void cloth_guide_at(const ClothInst& in, const float* m0, size_t nb, const float* is,
                           size_t i, float* g) {
    const ClothPart& p = in.st.part[i];
    float a[3];   // Authored: engine axes, metres, the model's frame.
    for (int k = 0; k < 3; ++k) a[k] = p.rest[k] * in.auth_ext[k] + in.auth_ctr[k];
    const SkinInf& f = in.part_inf[i];
    float q[3] = { 0.0f, 0.0f, 0.0f };
    float ws = 0.0f;
    for (int j = 0; j < 4; ++j) {
        const float w = f.w[j];
        if (!(w > 0.0f) || f.b[j] >= nb) continue;
        const float* m = m0 + static_cast<size_t>(f.b[j]) * 12u;
        for (int c = 0; c < 3; ++c) {
            q[c] += w * (a[0] * m[c] + a[1] * m[3 + c] + a[2] * m[6 + c] +
                         m[9 + c]);
        }
        ws += w;
    }
    const float r = ws < 1.0f ? 1.0f - ws : 0.0f;
    for (int k = 0; k < 3; ++k) g[k] = (q[k] + r * a[k] - in.auth_ctr[k]) * is[k];
}
inline void cloth_guide_prep(ClothInst& in) {
    ClothState& s = in.st;
    const size_t n = s.part.size();
    const size_t nb = in.in_guide.size() / 12u;
    if (nb == 0u || in.part_inf.size() != n) {
        s.guide_a.clear();
        s.guide_b.clear();
        return;
    }
    const bool had = s.guide_b.size() == n * 3u;
    // SUB_CLOCK: lane a stays the simulation's time, blended in place as
    // cloth_target blends it; with nothing left over it is lane b (the swap).
    if (had && s.guide_a.size() == n * 3u && s.sub_t < 1.0f) {
        if (s.sub_t > 0.0f) {
            for (size_t i = 0; i < n * 3u; ++i) {
                s.guide_a[i] = s.guide_a[i] + (s.guide_b[i] - s.guide_a[i]) * s.sub_t;
            }
        }
    } else {
        std::swap(s.guide_a, s.guide_b);
    }
    s.guide_b.resize(n * 3u);
    const float* m0 = in.in_guide.data();
    float is[3];
    for (int k = 0; k < 3; ++k) {
        const float v = fabsf(in.in_size[k]) < 1.0e-6f ? 1.0e-6f : in.in_size[k];
        is[k] = 1.0f / v;
    }
    for (size_t i = 0; i < n; ++i) {
        const ClothPart& p = s.part[i];
        float* g = &s.guide_b[i * 3u];
        if (p.w > 0.0f && !(p.att > 0.0f)) {   // Simulated: the bones do nothing here.
            memcpy(g, p.rest, sizeof(p.rest));
            continue;
        }
        cloth_guide_at(in, m0, nb, is, i, g);
    }
    if (!had) s.guide_a = s.guide_b;
}

// SKEL - a boxed instance's buffer b from sim_out: positions, normals and tangents re-expressed in the box of the
// drawn shape - the exact box of the level-0 vertices, since a cloth is lod_locked and its decimated levels are never
// drawn - under KhVsCore's convention (a position divided by the box, a normal multiplied by it, a tangent divided by
// it, each against the frame's own size first). The box is published with the buffer, and the object takes it where the
// buffer is installed (skin_upload), so the drawn bounds follow the cloth and the skeleton instead of standing in
// the rest box.
inline void cloth_box_out(ClothInst& in, uint32_t b) {
    const std::vector<MeshVertex>& src = in.sim_out;
    std::vector<MeshVertex>& dst = in.out[b];
    const size_t nv = src.size();
    dst.resize(nv);
    float s[3];
    for (int k = 0; k < 3; ++k) s[k] = fabsf(in.st.size[k]) < 1.0e-6f ? 1.0e-6f : in.st.size[k];
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    const size_t np = in.st.part.size();
    for (size_t i = 0; i < nv; ++i) {
        if (i >= in.vmap.size() || in.vmap[i] >= np) continue;   // Not level 0.
        for (int k = 0; k < 3; ++k) {
            const float m = src[i].pos[k] * s[k];
            if (m < mn[k]) mn[k] = m;
            if (m > mx[k]) mx[k] = m;
        }
    }
    float c[3], e[3];
    for (int k = 0; k < 3; ++k) {
        if (!(mn[k] <= mx[k])) { mn[k] = -0.5f * s[k]; mx[k] = 0.5f * s[k]; }
        c[k] = 0.5f * (mn[k] + mx[k]);
        const float w = mx[k] - mn[k];
        e[k] = w > 1.0e-4f ? w : 1.0e-4f;   // A flat axis, as the skin and native_size.
    }
    for (size_t i = 0; i < nv; ++i) {
        const MeshVertex& v = src[i];
        MeshVertex& o = dst[i];
        float n[3], t[3];
        for (int k = 0; k < 3; ++k) {
            o.pos[k] = (v.pos[k] * s[k] - c[k]) / e[k];
            n[k] = v.nrm[k] * (e[k] / s[k]);
            t[k] = v.tan[k] * (s[k] / e[k]);
        }
        // Rescaled to unit length: the shader normalizes after its own scale,
        // so only the direction matters, and the ratios above can be large.
        const float nl = cloth_v3_len(n);
        const float tl = cloth_v3_len(t);
        for (int k = 0; k < 3; ++k) {
            o.nrm[k] = nl > 1.0e-30f ? n[k] / nl : v.nrm[k];
            o.tan[k] = tl > 1.0e-30f ? t[k] / tl : v.tan[k];
        }
        o.tan[3] = v.tan[3];
        o.uv[0] = v.uv[0];
        o.uv[1] = v.uv[1];
    }
    in.box_on[b] = true;
    for (int k = 0; k < 3; ++k) {
        in.box_ctr[b][k] = in.auth_ctr[k] + c[k];
        in.box_size[b][k] = e[k];
    }
}

// One job. Runs on a cloth worker and touches nothing the game thread may be
// writing: 'busy' was set with the queue push, under the lock every pop takes
// (POOL_BUSY_AFTER_PUSH), and is cleared last.
inline void cloth_run(const std::shared_ptr<ClothInst>& in) {
    KH_PROF_SCOPE(PROF_W_CLOTH_SIM);   // PROF.
    ClothInst& cj = *in;
    const uint32_t back = 1u - cj.front.load(std::memory_order_relaxed);
    if (cj.chain) {
        // CHAIN: the chain's step and its skinning, then the same publish.
        const size_t nb = cj.in_guide.size() / 12u;
        const bool g = nb != 0u && nb == cj.ch.body_of.size();
        ChainParams cp = cj.cpar;   // SIM_LOD: the dials at this step's counts.
        if (cj.in_iter >= 1u && cj.in_sub >= 1u) {   // CHAIN_LOD_FLOOR: never below the chain's floor.
            cp.iterations = chain_lod_floor(cj.cpar.iterations, cj.in_iter, CHAIN_LOD_ITER);
            cp.substeps = chain_lod_floor(cj.cpar.substeps, cj.in_sub, CHAIN_LOD_SUB);
        }
        chain_step(cj.st, cj.ch, cp, cj.in_col.data(), cj.in_col.size(),
                   cj.in_centre, cj.in_rot, cj.in_rotated, cj.in_size, cj.in_dt, cj.in_wind,
                   g ? cj.in_guide.data() : nullptr, g ? nb : 0u, cj.in_tg,
                   cj.in_aff.data(), cj.in_aff.size(), &cj.in_ground);
        chain_writeback(cj, back);
        // CHAIN_JOB_GUIDE: the guide this front was solved against (none: the rest pose), with it - what
        // chain_end_draw corrects an end held on a bone from. The cloth branch's twin is CLOTH_PIN_GRADE's.
        // CHAIN_HELD_REC: the guide as the step HELD it - c.guide_prev, which chain_step leaves at the
        // substeps' end (cov of the way from the last step's held guide to this frame's, chain_bone_map's blend;
        // the frame's own when nothing was left over), not the frame's end as given.
        if (g && cj.ch.guide_prev.size() == cj.in_guide.size()) cj.out_guide[back] = cj.ch.guide_prev;
        else if (g) cj.out_guide[back] = cj.in_guide;
        else cj.out_guide[back].clear();
    } else {
    cloth_guide_prep(cj);   // SKEL.
    ClothParams pr = cj.par;   // SIM_LOD: the dials at this step's counts (par itself stays the object's).
    if (cj.in_iter >= 1u && cj.in_sub >= 1u) { pr.iterations = cj.in_iter; pr.substeps = cj.in_sub; }
    cloth_step(cj.st, pr, cj.in_col.data(), cj.in_col.size(),
               cj.in_centre, cj.in_rot, cj.in_rotated, cj.in_size, cj.in_dt,
               cj.in_wind, cj.in_aff.data(), cj.in_aff.size(), &cj.in_ground);
    if (cj.in_boxed) {
        // SKEL: written and keyed in the frame's own space, drawn in the shape's box.
        if (cj.sim_out.size() != cj.rest.size()) cj.sim_out = cj.rest;
        cloth_writeback(cj, cj.sim_out);
        cj.shape_of[back] = cloth_shape_note(cj, cj.sim_out);   // CLOTH_SHAPE.
        cloth_box_out(cj, back);
    } else {
        // A buffer last written in a shape's box starts over from rest: what the writeback leaves as it was (the LOD
        // tail, a degenerate fan) must be in the frame's space.
        if (cj.box_on[back]) {
            cj.out[back] = cj.rest;
            cj.box_on[back] = false;
        }
        if (!cj.sim_out.empty()) std::vector<MeshVertex>().swap(cj.sim_out);
        cloth_writeback(cj, cj.out[back]);
        cj.shape_of[back] = cloth_shape_note(cj, cj.out[back]);   // CLOTH_SHAPE.
        {   // CAST_REFIT: what the level-0 vertices reach, about the centre.
            const std::vector<MeshVertex>& o = cj.out[back];
            const size_t np = cj.st.part.size();
            float mx[3] = { 0.0f, 0.0f, 0.0f };
            for (size_t i = 0; i < o.size(); ++i) {
                if (i >= cj.vmap.size() || cj.vmap[i] >= np) continue;   // An LOD-only vertex keeps its rest pose.
                for (int k = 0; k < 3; ++k) { const float a = fabsf(o[i].pos[k]); if (a > mx[k]) mx[k] = a; }
            }
            for (int k = 0; k < 3; ++k) {   // pos is the frame's unit cube: metres = pos * size.
                const float s = fabsf(cj.st.size[k]);
                const float r = 2.0f * mx[k] * s;
                cj.cast_size[back][k] = (r == r && r > s) ? r : s;
            }
        }
    }
    // CLOTH_PIN_GRADE: what this front's targets were (inputs stand while busy) - as the step held them
    // (CLOTH_HELD_REC): the last held guide blended sub_t of the way to this frame's, as cloth_guide_prep
    // blends lane a (sub_t 1: this frame's; 0, no substep ran: the last held stands); another bone count or none
    // held yet: this frame's.
    {
        const size_t gn = cj.in_guide.size();
        const float gt = cj.st.sub_t;
        if (gn != 0u && cj.guide_held.size() == gn && gt < 1.0f) {
            if (gt > 0.0f) {
                for (size_t i = 0; i < gn; ++i) {
                    cj.guide_held[i] = cj.guide_held[i] + (cj.in_guide[i] - cj.guide_held[i]) * gt;
                }
            }
        } else {
            cj.guide_held = cj.in_guide;
        }
        cj.out_guide[back] = cj.guide_held;
    }
    }
    cj.front.store(back, std::memory_order_release);
    cj.gen.fetch_add(1u, std::memory_order_release);
    // Last, and after the publish: the game thread may write this instance's inputs the instant it turns false.
    for (std::shared_ptr<const PhysicsBvh>& o : cj.in_col_own) o.reset();
    cj.running.store(false, std::memory_order_release);   // CHAIN_END_RACE: its buffers are written.
    cj.busy.store(false, std::memory_order_release);
}

// SQF [x, y, zASL] -> engine [x, zASL, y], and the same swap for the edge lengths, against a plain float triple. The
// fill sites do the same swap inline into ConstantData lanes: a convention change must find them all.
inline void cloth_engine_pos(const RenderObject& o, float* out) {
    out[0] = o.pos[0]; out[1] = o.pos[2]; out[2] = o.pos[1];
}
inline void cloth_engine_size(const RenderObject& o, float* out) {
    out[0] = o.size[0]; out[1] = o.size[2]; out[2] = o.size[1];
}

// The carrier's centre in the world (engine axes): wc the world float, wd exact (POS_RES) - the frame
// the write-back makes a chain's or cloth's vertices relative to. A skeletal object stands at root + skel_ctr * rot
// (skel_centre); the rest box's centre is the authored native_ctr, at root + native_ctr * rot. cloth_sync
// builds its instance's frame from it, and chain_end_draw the frame a chain is drawn in (CHAIN_END_DRAW).
inline void cloth_carrier_at(const RenderObject& o, const float nc[3], float wc[3],
                             double wd[3]) {
    cloth_engine_pos(o, wc);
    {
        float wr[3];
        obj_pos_res(o, wr);
        for (int k = 0; k < 3; ++k) wd[k] = static_cast<double>(wc[k]) + static_cast<double>(wr[k]);
    }
    if (!o.skel) return;
    float d[3];
    for (int k = 0; k < 3; ++k) d[k] = nc[k] - o.skel_ctr[k];
    if (o.rotated) {
        for (int k = 0; k < 3; ++k) {
            const float a = d[0] * o.rot_m[k] + d[1] * o.rot_m[3 + k] +
                            d[2] * o.rot_m[6 + k];
            wc[k] += a;
            wd[k] += static_cast<double>(a);
        }
    } else {
        for (int k = 0; k < 3; ++k) {
            wc[k] += d[k];
            wd[k] += static_cast<double>(d[k]);
        }
    }
}
inline bool cloth_obj_sim(const RenderObject& o) {
    return o.cloth_sim && !o.fullscreen && o.mesh >= 0;
}
inline bool physics_obj_collider(const RenderObject& o) {
    return o.physics_collider && !o.fullscreen && o.mesh >= 0;
}

// Does this mesh carry cloth authoring at all? An unpainted model asked to simulate is a script error, not a silent
// no-op: cloth_sync reports it on this (once per mesh), and skin_sync keeps such a mesh for the skin.
inline bool cloth_mesh_ok(int mesh) {
    if (!mesh_alive(mesh)) return false;
    const MeshDef& d = mesh_def(mesh);
    return d.cloth_w.size() == d.verts.size() && !d.cloth_w.empty();
}

inline void physics_gather_colliders(const std::vector<RenderObject>& objs,
                                      std::vector<PhysicsColSrc>& out) {
    KH_PROF_SCOPE(PROF_PHYSICS_GATHER);   // PROF.
    out.clear();
    ++g_physics_bvh_frame;
    for (size_t i = 0; i < objs.size(); ++i) {
        const RenderObject& o = objs[i];
        // A hidden collider is not drawn and still collides.
        if (!physics_obj_collider(o)) continue;
        if (!mesh_alive(o.mesh)) continue;
        float sz[3], bake[3];
        cloth_engine_size(o, sz);
        std::shared_ptr<const PhysicsBvh> b;
        bool skin = false;
        bool fresh = false;   // SKEL: the last park installed the pose, so its motion is this frame's.
        float sctr[3] = { 0.0f, 0.0f, 0.0f };   // SKIN_COL_BOX: the BVH's own box centre (engine axes, the model's frame).
        if (o.skel) {
            // SKEL: a skeletal binding collides as its installed skinned result's BVH, or as the rest mesh's while
            // its own vertices draw. Any other shape has no BVH and the collider sits the frame out. The table is the
            // one taken with the objects.
            // SKIN_COL_BOX: the BVH is metres about ITS result's box centre (skin_run: met - box_ctr), so it
            // stands at root + ctr * R whatever box the object draws in now - the render thread's step's (SKEL_DRAW:
            // with SkinGpu::by_step the upload's put does not land, and the object's box is the step's, sampled apart
            // from the worker's and never byte-equal to it) or the put's. So the result is taken by mesh alone and
            // placed at its own centre below (sctr, in the box it was made in); the object's box is not required
            // to match it. The collider is then the worker's pose, a sample behind the step's draw at most.
            const SkinCol* sc = nullptr;
            if (g_skin_col_snap) {
                const auto it = g_skin_col_snap->find(o.seq);
                if (it != g_skin_col_snap->end()) sc = &it->second;
            }
            const MeshDef& md = mesh_def(o.mesh);
            if (sc && sc->bvh && sc->mesh == o.mesh) {
                b = sc->bvh;
                sz[0] = sc->size[0];   // The BVH's box: SQF order in, engine axes out (cloth_engine_size).
                sz[1] = sc->size[2];
                sz[2] = sc->size[1];
                memcpy(bake, sz, sizeof(bake));
                memcpy(sctr, sc->ctr, sizeof(sctr));
                skin = true;
                // A pose published since the previous cloth sync sweeps; an older stamp holds still.
                fresh = skin_col_fresh(sc->serial);
            } else if (memcmp(md.native_size, o.size, sizeof(o.size)) != 0 ||
                       memcmp(md.native_ctr, o.skel_ctr, sizeof(o.skel_ctr)) != 0) {
                continue;
            }
        } else if (o.chain_sim && memcmp(mesh_def(o.mesh).native_size, o.size, sizeof(o.size)) != 0) {
            // CHAIN: a plain chain's box is its shape's, not its rest's, so like a simulating skeletal mesh it
            // collides as its rest mesh only while that draws, and otherwise sits the frame out.
            continue;
        }
        // A clothSimulation collider collides as its rest mesh (the shared per-mesh BVH below), not its simulated
        // shape: other cloth meets it where it was modelled, wherever it has sagged to.
        if (!skin) {
            const uint32_t shape = physics_bvh_shape(sz, bake);
            b = physics_bvh_for(mesh_id_clamp(o.mesh), shape, bake);
        }
        if (!b || b->empty()) continue;
        PhysicsColSrc s;
        s.own = b;
        PhysicsColliderView& v = s.view;
        v.bvh = b.get();
        memcpy(v.bake, bake, sizeof(v.bake));
        memcpy(v.rot, o.rot_m, sizeof(v.rot));
        v.rotated = o.rotated;
        // The object's engine-axes centre and edge lengths, the same pair the vertex shader uses, so the collider
        // occupies exactly the volume the player sees it occupy. SKIN_COL_BOX: a skinned BVH's centre is its own
        // box's - the object stands at root + skel_ctr * R (skel_centre), so the BVH's is that plus (ctr - skel_ctr)
        // * R - and its edge lengths that box's (sz).
        cloth_engine_pos(o, v.centre);
        obj_pos_res(o, v.res);   // POS_RES: added where the view enters an instance's frame.
        if (skin) {
            float d[3];
            for (int k = 0; k < 3; ++k) d[k] = sctr[k] - o.skel_ctr[k];
            for (int k = 0; k < 3; ++k) {
                v.centre[k] += o.rotated
                    ? d[0] * o.rot_m[k] + d[1] * o.rot_m[3 + k] + d[2] * o.rot_m[6 + k]
                    : d[k];
            }
        }
        memcpy(v.size, sz, sizeof(v.size));
        float bs = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float e = fabsf(v.size[k]) < 1.0e-6f ? 1.0e-6f : v.size[k];
            v.size[k] = e;
            v.inv_size[k] = 1.0f / e;
            v.bs_c[k] = v.centre[k];
            bs += 0.25f * e * e;
        }
        // The normalized mesh spans [-0.5, 0.5], so half the diagonal of the sized box bounds it whatever the rotation.
        v.bs_r = sqrtf(bs);
        v.margin = o.physics_col_margin;
        v.friction = o.physics_col_friction;
        v.inside = o.physics_col_inside;
        v.convex = o.physics_col_convex;   // COL_CONVEX.
        v.slot = o.slot;
        v.mesh = mesh_id_clamp(o.mesh);
        v.skinned = skin;
        memcpy(v.skin_ctr, sctr, sizeof(v.skin_ctr));   // SKIN_COL_BOX: the BVH's own box centre.
        if (o.skel) {   // SKIN_COL_PREV: the centre above, less the root.
            memcpy(v.root_off, skin ? sctr : o.skel_ctr, sizeof(v.root_off));
        }
        // SKEL: a pose the last park installed sweeps from the last one (PhysicsBvh::vd); one already collided
        // with last frame holds still. Its bound then covers both poses: the root box, metric here.
        v.deform = skin && fresh && !b->vd.empty();
        v.def0 = 0.0f;
        v.def1 = 0.0f;
        if (v.deform) {
            const PhysicsBvhNode& rn = b->nodes[0];
            float sr = 0.0f;
            for (int k = 0; k < 3; ++k) {
                const float e = fabsf(rn.mn[k]) > fabsf(rn.mx[k]) ? fabsf(rn.mn[k]) : fabsf(rn.mx[k]);
                sr += e * e;
            }
            sr = sqrtf(sr);
            if (sr > v.bs_r) v.bs_r = sr;
        }
        // No earlier pose is known here - cloth_sync hands each cloth the
        // one it saw at its own last step - so the view starts out still.
        memcpy(v.prev.centre, v.centre, sizeof(v.prev.centre));
        memcpy(v.prev.rot, v.rot, sizeof(v.prev.rot));
        memcpy(v.prev.size, v.size, sizeof(v.prev.size));
        v.prev.rotated = v.rotated;
        v.vel[0] = 0.0f; v.vel[1] = 0.0f; v.vel[2] = 0.0f;
        out.push_back(s);
    }
    std::lock_guard<std::mutex> g(g_physics_bvh_mu);
    for (auto it = g_physics_bvh.begin(); it != g_physics_bvh.end();) {
        if (g_physics_bvh_frame - it->second.used > PHYSICS_BVH_IDLE) it = g_physics_bvh.erase(it);
        else ++it;
    }
}

// Move an instance's origin by d (metres, engine axes): every stored position shifts by -d so no world position
// changes. Game thread, non-busy instance.
inline void cloth_origin_shift(ClothInst& in, const double* d) {
    const float f[3] = { static_cast<float>(d[0]), static_cast<float>(d[1]),
                         static_cast<float>(d[2]) };
    for (ClothPart& p : in.st.part) {
        for (int k = 0; k < 3; ++k) { p.p[k] -= f[k]; p.pp[k] -= f[k]; }
    }
    for (int k = 0; k < 3; ++k) {
        in.st.centre[k] -= f[k];
        in.st.fend.centre[k] -= f[k];   // SUB_CLOCK.
    }
    // Last frame's collider views: cloth_sync reads each collider's previous pose from them before it rewrites them.
    for (PhysicsColliderView& v : in.in_col) {
        for (int k = 0; k < 3; ++k) { v.centre[k] -= f[k]; v.bs_c[k] -= f[k]; v.prev.centre[k] -= f[k]; }
    }
    // CHAIN: the bodies, and the end's held and last targets, likewise.
    for (ChainBody& b : in.ch.body) {
        for (int k = 0; k < 3; ++k) { b.x[k] -= f[k]; b.xp[k] -= f[k]; }
    }
    for (int k = 0; k < 3; ++k) { in.ch.hold_p[k] -= f[k]; in.ch.prev_p[k] -= f[k]; }
    for (int k = 0; k < 3; ++k) in.org[k] += d[k];
}

// The terrain nodes within the cloth's reach about its carrier's centre, at last step's centre and this one's (a
// fast carrier's whole path), into its frame. Game thread, non-busy instance, after its origin and reach are this
// frame's. tele: the teleport distance its step tests with (the cloth's or the chain's setting).
inline void cloth_ground_patch(ClothInst& in, const float* wc, float reach, float tele) {
    ClothGround& g = in.in_ground;
    g.on = false;
    if (!g_ground_ready) return;
    const size_t n = g_ground_n;
    const float c = g_ground_cell;
    if (g_terrain_matrix.size() != n || g_terrain_grid_width != c) return;   // Rebuilt since: next frame.
    double lo[2] = { wc[0], wc[2] }, hi[2] = { wc[0], wc[2] };
    if (in.st.primed) {
        const ClothState& s = in.st;
        const double px = static_cast<double>(s.centre[0]) + in.org[0];
        const double pz = static_cast<double>(s.centre[2]) + in.org[2];
        // EA_GROUND_JUMP: the last centre only while the step can sweep from it. A carrier landing more than the
        // teleport distance (2 m unset) from where its velocity, over at most CLOTH_DT_MAX, carried it from the
        // last frame's end jumped (cloth_frame_jumped): both steps rebase it and sweep nothing. fend stands
        // |fend - centre| from the last centre; 1 m covers the world float; a non-finite bound keeps the path, as
        // the step's test does. A patch spanning a teleport across the map copied, and kept, millions of nodes.
        float fd[3];
        cloth_v3_sub(s.fend.centre, s.centre, fd);
        const float v = s.carrier_vok ? cloth_v3_len(s.carrier_v) : 0.0f;
        const double sw = static_cast<double>(tele > 0.0f ? tele : 2.0f) +
                          static_cast<double>(v) * CLOTH_DT_MAX +
                          static_cast<double>(cloth_v3_len(fd)) + 1.0;
        const double dx = px - static_cast<double>(wc[0]);
        const double dz = pz - static_cast<double>(wc[2]);
        if (!(dx * dx + dz * dz > sw * sw)) {
            lo[0] = std::min(lo[0], px); hi[0] = std::max(hi[0], px);
            lo[1] = std::min(lo[1], pz); hi[1] = std::max(hi[1], pz);
        }
    }
    const double last = static_cast<double>(n - 1u);
    int64_t a[2], b[2];
    for (int k = 0; k < 2; ++k) {
        const double l = std::floor((lo[k] - reach) / c);
        const double h = std::ceil((hi[k] + reach) / c);
        if (!(h >= 0.0) || !(l <= last)) return;   // Off the map: no terrain known there.
        a[k] = static_cast<int64_t>(l < 0.0 ? 0.0 : l);
        b[k] = static_cast<int64_t>(h > last ? last : h);
        if (b[k] == a[k]) { if (b[k] > 0) --a[k]; else ++b[k]; }   // A cell, at least.
    }
    const uint32_t nx = static_cast<uint32_t>(b[0] - a[0] + 1);
    const uint32_t nz = static_cast<uint32_t>(b[1] - a[1] + 1);
    g.h.resize(static_cast<size_t>(nx) * nz);
    for (uint32_t j = 0; j < nz; ++j) {
        const std::vector<float>& row = g_terrain_matrix[static_cast<size_t>(a[1]) + j];
        float* dst = &g.h[static_cast<size_t>(j) * nx];
        for (uint32_t i = 0; i < nx; ++i) {
            dst[i] = static_cast<float>(static_cast<double>(row[static_cast<size_t>(a[0]) + i]) -
                                        in.org[CLOTH_UP]);
        }
    }
    // Node positions as the framework sampled them (float index * spacing).
    g.x0 = static_cast<float>(static_cast<double>(static_cast<float>(a[0]) * c) - in.org[0]);
    g.z0 = static_cast<float>(static_cast<double>(static_cast<float>(a[1]) * c) - in.org[2]);
    g.cell = c;
    g.nx = nx;
    g.nz = nz;
    g.px = static_cast<uint32_t>(a[0] & 1);
    g.pz = static_cast<uint32_t>(a[1] & 1);
    memcpy(g.split, g_ground_split, sizeof(g.split));
    g.on = true;
}
}