#pragma once

// The feature half (rendering_integration_features.hpp): KH_CLOTH runtime: the collider BVH registry, cloth building
// and write-back, the shape notes, the collider gather and the ground patch. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/skinning.hpp (declared here for the uses below).
inline bool kh_skin_col_fresh(uint64_t khsf_s);


inline uint64_t kh_physics_bvh_key(int khbk_mesh, uint32_t khbk_shape) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(khbk_mesh)) << 32) | khbk_shape;
}

inline std::shared_ptr<const KhPhysicsBvh> kh_physics_bvh_for(int khcv_mesh, uint32_t khcv_shape, const float* khcv_bake) {
    const uint64_t khcv_key = kh_physics_bvh_key(khcv_mesh, khcv_shape);
    {
        std::lock_guard<std::mutex> khcv_g(g_physics_bvh_mu);
        const auto khcv_it = g_physics_bvh.find(khcv_key);
        if (khcv_it != g_physics_bvh.end()) { khcv_it->second.used = g_physics_bvh_frame; return khcv_it->second.bvh; }
    }
    // Built OUTSIDE the lock. The one caller is kh_physics_gather_colliders, on the game thread: the build is paid
    // there, once per collider mesh and shape (the key), on the first frame that shape collides (a large collider
    // costs that frame). Two threads racing on one key both build and the recheck below drops the loser's copy: a
    // duplicated build, never a torn map.
    std::shared_ptr<KhPhysicsBvh> khcv_b = std::make_shared<KhPhysicsBvh>();
    if (kh_mesh_alive(khcv_mesh)) {
        const MeshDef& khcv_d = mesh_def(khcv_mesh);
        const uint32_t khcv_ni = khcv_d.lod_n ? khcv_d.lod_istart[0]
                                              : static_cast<uint32_t>(khcv_d.indices.size());
        std::vector<float> khcv_p(khcv_d.verts.size() * 3u);
        for (size_t khcv_i = 0; khcv_i < khcv_d.verts.size(); ++khcv_i) {
            khcv_p[khcv_i * 3u]      = khcv_d.verts[khcv_i].pos[0] * khcv_bake[0];
            khcv_p[khcv_i * 3u + 1u] = khcv_d.verts[khcv_i].pos[1] * khcv_bake[1];
            khcv_p[khcv_i * 3u + 2u] = khcv_d.verts[khcv_i].pos[2] * khcv_bake[2];
        }
        kh_physics_bvh_build(*khcv_b, khcv_p.data(), static_cast<uint32_t>(khcv_d.verts.size()),
                           khcv_d.indices.data(), khcv_ni);
    }
    std::lock_guard<std::mutex> khcv_g(g_physics_bvh_mu);
    const auto khcv_it = g_physics_bvh.find(khcv_key);
    if (khcv_it != g_physics_bvh.end()) { khcv_it->second.used = g_physics_bvh_frame; return khcv_it->second.bvh; }
    g_physics_bvh.emplace(khcv_key, KhPhysicsBvhEntry{ khcv_b, g_physics_bvh_frame });
    return khcv_b;
}

// A simulated particle's inverse mass. Cloth has uniform mass (kh_cloth_build), so this is the value of every w that
// is not zero; the build and kh_cloth_sync's live 'mass' update both take it from here.
inline float kh_cloth_inv_mass(float khcm_mass) {
    return 1.0f / (khcm_mass > 1.0e-4f ? khcm_mass : 1.0e-4f);
}

// Build the particle set, the constraint sets and the render-vertex map from a mesh's level-0 topology. Particles
// are welded by POSITION, not by vertex identity: the mesh's own weld splits a vertex wherever a UV, a normal or a
// tangent differs, and simulating those as independent points opens a hole along every seam.
inline bool kh_cloth_build(KhClothInst& khcb_in, const MeshDef& khcb_d, uint32_t khcb_slot, int khcb_mesh) {
    khcb_in.st.part.clear(); khcb_in.st.stretch.clear(); khcb_in.st.bend.clear();
    khcb_in.st.tri.clear();
    khcb_in.vmap.clear();
    if (khcb_d.verts.empty() || khcb_d.indices.empty()) return false;
    if (khcb_d.cloth_w.size() != khcb_d.verts.size()) return false;   // Not a cloth mesh.
    const uint32_t khcb_ni = khcb_d.lod_n ? khcb_d.lod_istart[0]
                                          : static_cast<uint32_t>(khcb_d.indices.size());
    if (khcb_ni < 3u) return false;
    const uint32_t khcb_nv = static_cast<uint32_t>(khcb_d.verts.size());

    // Position weld.
    std::unordered_map<KhClothPosKey, uint32_t, KhClothPosHash> khcb_map;
    khcb_map.reserve(khcb_nv / 2u + 16u);
    khcb_in.vmap.assign(khcb_nv, 0xFFFFFFFFu);
    std::vector<float> khcb_wsum;
    for (uint32_t khcb_i = 0; khcb_i < khcb_ni; ++khcb_i) {
        const uint32_t khcb_v = khcb_d.indices[khcb_i];
        if (khcb_v >= khcb_nv || khcb_in.vmap[khcb_v] != 0xFFFFFFFFu) continue;
        KhClothPosKey khcb_k;
        memcpy(khcb_k.p, khcb_d.verts[khcb_v].pos, sizeof(khcb_k.p));
        auto khcb_it = khcb_map.find(khcb_k);
        if (khcb_it == khcb_map.end()) {
            const uint32_t khcb_id = static_cast<uint32_t>(khcb_in.st.part.size());
            KhClothPart khcb_p = {};
            memcpy(khcb_p.rest, khcb_d.verts[khcb_v].pos, sizeof(khcb_p.rest));
            khcb_p.anchor = khcb_id;
            khcb_p.tether = -1.0f;
            khcb_in.st.part.push_back(khcb_p);
            khcb_wsum.push_back(0.0f);
            khcb_it = khcb_map.emplace(khcb_k, khcb_id).first;
        }
        khcb_in.vmap[khcb_v] = khcb_it->second;
        // A welded particle takes the LARGEST weight of the render vertices on it (the importer's seam rule).
        const float khcb_w = static_cast<float>(khcb_d.cloth_w[khcb_v]) * (1.0f / 255.0f);
        if (khcb_w > khcb_wsum[khcb_it->second]) khcb_wsum[khcb_it->second] = khcb_w;
    }
    const uint32_t khcb_np = static_cast<uint32_t>(khcb_in.st.part.size());
    if (khcb_np < 3u) return false;

    // Unique edges, and the bend pairs across every edge shared by two faces.
    std::unordered_map<uint64_t, uint32_t> khcb_edge;   // key -> opposite particle of the first face.
    khcb_edge.reserve(khcb_ni);
    std::unordered_map<uint64_t, uint8_t> khcb_seen;
    khcb_seen.reserve(khcb_ni);
    auto khcb_key = [](uint32_t x, uint32_t y) -> uint64_t {
        return x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
    };
    auto khcb_add = [&](std::vector<KhClothDist>& khcb_set, uint32_t x, uint32_t y) {
        if (x == y || x >= khcb_np || y >= khcb_np) return;
        KhClothDist khcb_c;
        khcb_c.a = x; khcb_c.b = y;
        khcb_c.rest = 0.0f;   // Filled once the object's size is known.
        khcb_set.push_back(khcb_c);
    };
    for (uint32_t khcb_t = 0; khcb_t + 2u < khcb_ni; khcb_t += 3u) {
        const uint32_t khcb_r[3] = { khcb_d.indices[khcb_t], khcb_d.indices[khcb_t + 1u], khcb_d.indices[khcb_t + 2u] };
        if (khcb_r[0] >= khcb_nv || khcb_r[1] >= khcb_nv || khcb_r[2] >= khcb_nv) continue;
        const uint32_t khcb_p[3] = { khcb_in.vmap[khcb_r[0]], khcb_in.vmap[khcb_r[1]], khcb_in.vmap[khcb_r[2]] };
        if (khcb_p[0] >= khcb_np || khcb_p[1] >= khcb_np || khcb_p[2] >= khcb_np) continue;
        // The face, for wind's orientation. A triangle welded onto fewer than three particles has no area or facing.
        if (khcb_p[0] != khcb_p[1] && khcb_p[1] != khcb_p[2] && khcb_p[0] != khcb_p[2]) {
            KhClothTri khcb_ft;
            khcb_ft.a = khcb_p[0]; khcb_ft.b = khcb_p[1]; khcb_ft.c = khcb_p[2];
            khcb_in.st.tri.push_back(khcb_ft);
        }
        for (int khcb_e = 0; khcb_e < 3; ++khcb_e) {
            const uint32_t khcb_a = khcb_p[khcb_e], khcb_b = khcb_p[(khcb_e + 1) % 3];
            const uint32_t khcb_op = khcb_p[(khcb_e + 2) % 3];
            if (khcb_a == khcb_b) continue;
            const uint64_t khcb_kk = khcb_key(khcb_a, khcb_b);
            if (khcb_seen.emplace(khcb_kk, 1u).second) khcb_add(khcb_in.st.stretch, khcb_a, khcb_b);
            const auto khcb_it = khcb_edge.find(khcb_kk);
            if (khcb_it == khcb_edge.end()) {
                khcb_edge.emplace(khcb_kk, khcb_op);
            } else if (khcb_it->second != khcb_op) {
                // The two corners facing each other across a shared edge: a distance constraint between them resists
                // folding without a dihedral solve, which keeps this affordable per substep.
                khcb_add(khcb_in.st.bend, khcb_it->second, khcb_op);
            }
        }
    }

    for (uint32_t khcb_i = 0; khcb_i < khcb_np; ++khcb_i) {
        KhClothPart& khcb_pp = khcb_in.st.part[khcb_i];
        const float khcb_w = khcb_wsum[khcb_i];
        // INVERSE MASS CARRIES NO PART OF THE GRADIENT. Do not scale it by the paint weight: gravity reaches a
        // particle whatever its mass while every constraint correction is scaled BY inverse mass, so a barely-painted
        // vertex would fall at a full g and be held by nothing. Cloth has uniform mass.
        khcb_pp.w = khcb_w > 0.0f ? kh_cloth_inv_mass(khcb_in.par.mass) : 0.0f;
        // The gradient itself, as the attachment's compliance. Harmonic in the paint weight: 0 (welded to the rest
        // pose) at the pinned end, unbounded at the simulated end, so it fades out with no step to cut at the top.
        // Zero is the sentinel for 'no attachment' and a fully simulated vertex takes it (the limit's own answer).
        // KH_CLOTH_ATT_SOFT centres the curve so that painting half sags about half.
        khcb_pp.att = khcb_w >= 1.0f ? 0.0f
                                     : KH_CLOTH_ATT_SOFT * khcb_w / (1.0f - khcb_w);
    }

    khcb_in.mesh_edges = static_cast<uint32_t>(khcb_in.st.stretch.size());
    khcb_in.shell_built = -1.0f;
    // One authored normal per particle, from the first render vertex that landed on it. A UV seam splits a vertex
    // without splitting the normal, so any copy answers; a smoothing split's sides disagree, and either answers
    // 'which way does this surface face'.
    khcb_in.part_nrm.assign(static_cast<size_t>(khcb_np) * 3u, 0.0f);
    {
        std::vector<uint8_t> khcb_seen_n(khcb_np, 0u);
        for (uint32_t khcb_v = 0; khcb_v < khcb_nv; ++khcb_v) {
            const uint32_t khcb_pi = khcb_in.vmap[khcb_v];
            if (khcb_pi >= khcb_np || khcb_seen_n[khcb_pi]) continue;
            khcb_seen_n[khcb_pi] = 1u;
            memcpy(&khcb_in.part_nrm[static_cast<size_t>(khcb_pi) * 3u],
                   khcb_d.verts[khcb_v].nrm, sizeof(float) * 3u);
        }
    }
    // KH_SKEL: the influences, per particle, from the same first render vertex.
    khcb_in.part_inf.clear();
    if (khcb_d.skin_inf.size() == khcb_nv) {
        khcb_in.part_inf.assign(khcb_np, KhSkinInf());
        std::vector<uint8_t> khcb_seen_i(khcb_np, 0u);
        for (uint32_t khcb_v = 0; khcb_v < khcb_nv; ++khcb_v) {
            const uint32_t khcb_pi = khcb_in.vmap[khcb_v];
            if (khcb_pi >= khcb_np || khcb_seen_i[khcb_pi]) continue;
            khcb_seen_i[khcb_pi] = 1u;
            khcb_in.part_inf[khcb_pi] = khcb_d.skin_inf[khcb_v];
        }
    }
    memcpy(khcb_in.auth_ctr, khcb_d.native_ctr, sizeof(khcb_in.auth_ctr));
    memcpy(khcb_in.auth_ext, khcb_d.native_ext, sizeof(khcb_in.auth_ext));
    // The writeback's smoothing groups. Every render vertex on a particle whose authored normal matches an earlier
    // one's (KH_CLOTH_NRM_JOIN) names that earlier vertex, so a UV seam's copies share one accumulator while a hard
    // edge's copies (differing authored normals) each keep their own. The chain per particle is the handful of
    // distinct normals meeting there.
    khcb_in.nrm_rep.resize(khcb_nv);
    {
        std::vector<uint32_t> khcb_head(khcb_np, 0xFFFFFFFFu);
        std::vector<uint32_t> khcb_next(khcb_nv, 0xFFFFFFFFu);
        for (uint32_t khcb_v = 0; khcb_v < khcb_nv; ++khcb_v) {
            khcb_in.nrm_rep[khcb_v] = khcb_v;
            const uint32_t khcb_pi = khcb_in.vmap[khcb_v];
            if (khcb_pi >= khcb_np) continue;
            // KH_CLOTH_JOIN_AUTH: compared as authored (n / native_ext, the skin runtime's NORMALS note): the stored
            // normals carry each axis by its extent, and on a thin or flat sheet (KH_IMPORT_FLAT) that squash makes
            // normals differing across the sheet's plane read as one.
            float khcb_vn[3], khcb_rn[3];
            for (int khcb_k = 0; khcb_k < 3; ++khcb_k) {
                khcb_vn[khcb_k] = khcb_d.verts[khcb_v].nrm[khcb_k] / khcb_d.native_ext[khcb_k];
            }
            const float khcb_vl = kh_cloth_v3_len(khcb_vn);
            for (uint32_t khcb_r = khcb_head[khcb_pi]; khcb_r != 0xFFFFFFFFu; khcb_r = khcb_next[khcb_r]) {
                for (int khcb_k = 0; khcb_k < 3; ++khcb_k) {
                    khcb_rn[khcb_k] = khcb_d.verts[khcb_r].nrm[khcb_k] / khcb_d.native_ext[khcb_k];
                }
                const float khcb_den = khcb_vl * kh_cloth_v3_len(khcb_rn);
                if (khcb_den > 1.0e-12f && kh_cloth_v3_dot(khcb_vn, khcb_rn) >= KH_CLOTH_NRM_JOIN * khcb_den) {
                    khcb_in.nrm_rep[khcb_v] = khcb_r;
                    break;
                }
            }
            if (khcb_in.nrm_rep[khcb_v] == khcb_v) {
                khcb_next[khcb_v] = khcb_head[khcb_pi];
                khcb_head[khcb_pi] = khcb_v;
            }
        }
    }
    khcb_in.rest = khcb_d.verts;
    khcb_in.out[0] = khcb_d.verts;
    khcb_in.out[1] = khcb_d.verts;
    khcb_in.out_guide[0].clear();   // KH_CLOTH_PIN_GRADE: the rest pose was solved against no guide.
    khcb_in.out_guide[1].clear();
    khcb_in.guide_held.clear();   // KH_CLOTH_HELD_REC.
    khcb_in.box_on[0] = khcb_in.box_on[1] = false;   // KH_SKEL: both start in the rest box.
    khcb_in.uploaded_box_on = false;
    khcb_in.sim_out.clear();
    khcb_in.nrm_acc.assign(static_cast<size_t>(khcb_nv) * 3u, 0.0f);
    khcb_in.tan_acc.assign(static_cast<size_t>(khcb_nv) * 3u, 0.0f);
    khcb_in.lvl0_idx.assign(khcb_d.indices.begin(), khcb_d.indices.begin() + khcb_ni);   // KH_CLOTH_OWN_IDX.
    khcb_in.mesh = khcb_mesh;
    khcb_in.slot = khcb_slot;
    khcb_in.st.primed = false;
    khcb_in.st.accum = 0.0f;
    khcb_in.st.h_last = 0.0f;   // KH_SIM_LOD.
    khcb_in.st.speed_mean = 0.0f;
    khcb_in.built = true;
    return true;
}

// THICK CLOTH: bind opposing surfaces so a solidified mesh behaves as one piece. Rebuilt rather than appended, so
// calling this twice cannot double the links - the mesh edges are the prefix and everything after them is ours.
// A pair qualifies only when all three hold: the two particles are within 'shell' metres, their authored normals
// OPPOSE, and the second lies along the first's BACKWARD normal rather than merely beside it. The third keeps it from
// fusing a fold: two faces of a pleat passing close satisfy the first two and fail the third unless they are
// genuinely back to back, at which point they are a shell.
inline void kh_cloth_build_shell(KhClothInst& khcl_in, const float* khcl_size) {
    if (khcl_in.st.stretch.size() > khcl_in.mesh_edges) khcl_in.st.stretch.resize(khcl_in.mesh_edges);
    khcl_in.shell_built = khcl_in.par.shell;
    const float khcl_r = khcl_in.par.shell;
    if (!(khcl_r > 0.0f)) return;
    const size_t khcl_n = khcl_in.st.part.size();
    if (khcl_n < 2u || khcl_in.part_nrm.size() < khcl_n * 3u) return;
    // The search runs in metres: the normalized rest positions go through the object's size as the draw does.
    const float khcl_org[3] = { 0.0f, 0.0f, 0.0f };
    const float khcl_id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    std::vector<float> khcl_w(khcl_n * 3u);
    for (size_t i = 0; i < khcl_n; ++i) {
        kh_cloth_l2w(khcl_in.st.part[i].rest, khcl_org, khcl_id, false, khcl_size, &khcl_w[i * 3u]);
    }
    // The normals too. Stored normals live in NORMALIZED mesh space - the draw divides them by size (KhVsCore) - so
    // each goes through the same inverse scale before either test below holds it against a metric offset. Do not
    // mix the spaces: a thin box then tilts every normal away from its short axis, and on pleated thick cloth the
    // 'behind' test missed 55 of 405 front/back pairs at 5 mm pleats and cross-linked 50 extra at 10 mm.
    std::vector<float> khcl_nm(khcl_n * 3u, 0.0f);
    for (size_t i = 0; i < khcl_n; ++i) {
        const float* khcl_a = &khcl_in.part_nrm[i * 3u];
        if (kh_cloth_v3_dot(khcl_a, khcl_a) < 0.25f) continue;   // No usable normal: stays zero.
        float* khcl_m = &khcl_nm[i * 3u];
        for (int k = 0; k < 3; ++k) khcl_m[k] = khcl_a[k] / (fabsf(khcl_size[k]) < 1.0e-6f ? 1.0e-6f : khcl_size[k]);
        const float khcl_ml = kh_cloth_v3_len(khcl_m);
        if (khcl_ml > 1.0e-20f) for (int k = 0; k < 3; ++k) khcl_m[k] /= khcl_ml;
        else khcl_m[0] = khcl_m[1] = khcl_m[2] = 0.0f;
    }
    // Uniform grid at the search radius: one cell step in each direction covers every candidate, so this stays
    // linear in the particle count. The cell is never under 2^-19 of the farthest particle's offset, so every cell
    // index fits the key's 21 bits (+-2^20) and its int conversion whatever shell a script gives (for every finite
    // placement): a larger cell only adds candidates the distance test turns away.
    // KH_CLOTH_SHELL_FINITE: a size that is not finite (a script's 1e39, or a multiplier that overflows the native
    // size) places particles at infinity or NaN, and the cell index's int conversion of either is undefined; such a
    // cloth has no metric shell to bind, so it links none.
    float khcl_ext = 0.0f;
    for (size_t i = 0; i < khcl_n * 3u; ++i) {
        if (!(fabsf(khcl_w[i]) <= 3.0e38f)) return;
        khcl_ext = fmaxf(khcl_ext, fabsf(khcl_w[i]));
    }
    const float khcl_cell = fmaxf(khcl_r, khcl_ext * (1.0f / 524288.0f));
    std::unordered_map<uint64_t, std::vector<uint32_t>> khcl_grid;
    khcl_grid.reserve(khcl_n * 2u);
    auto khcl_key = [&](int x, int y, int z) -> uint64_t {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x + 1048576)) << 42) ^
               (static_cast<uint64_t>(static_cast<uint32_t>(y + 1048576)) << 21) ^
                static_cast<uint64_t>(static_cast<uint32_t>(z + 1048576));
    };
    auto khcl_cellof = [&](const float* p, int* o) {
        for (int k = 0; k < 3; ++k) o[k] = static_cast<int>(floorf(p[k] / khcl_cell));
    };
    for (uint32_t i = 0; i < khcl_n; ++i) {
        int c[3]; khcl_cellof(&khcl_w[i * 3u], c);
        khcl_grid[khcl_key(c[0], c[1], c[2])].push_back(i);
    }
    // Never link a pair the mesh already joins.
    std::unordered_map<uint64_t, uint8_t> khcl_have;
    khcl_have.reserve(khcl_in.mesh_edges * 2u + 16u);
    auto khcl_pk = [](uint32_t x, uint32_t y) -> uint64_t {
        return x < y ? (static_cast<uint64_t>(x) << 32) | y : (static_cast<uint64_t>(y) << 32) | x;
    };
    for (uint32_t e = 0; e < khcl_in.mesh_edges; ++e) {
        khcl_have.emplace(khcl_pk(khcl_in.st.stretch[e].a, khcl_in.st.stretch[e].b), 1u);
    }
    for (uint32_t i = 0; i < khcl_n; ++i) {
        const float* khcl_pi = &khcl_w[i * 3u];
        const float* khcl_ni = &khcl_nm[i * 3u];
        if (kh_cloth_v3_dot(khcl_ni, khcl_ni) < 0.25f) continue;   // No usable normal.
        uint32_t khcl_best = 0xFFFFFFFFu;
        float    khcl_bd = khcl_r;
        int c[3]; khcl_cellof(khcl_pi, c);
        for (int dx = -1; dx <= 1; ++dx) for (int dy = -1; dy <= 1; ++dy) for (int dz = -1; dz <= 1; ++dz) {
            const auto khcl_it = khcl_grid.find(khcl_key(c[0] + dx, c[1] + dy, c[2] + dz));
            if (khcl_it == khcl_grid.end()) continue;
            for (size_t k = 0; k < khcl_it->second.size(); ++k) {
                const uint32_t j = khcl_it->second[k];
                if (j == i) continue;
                const float* khcl_pj = &khcl_w[j * 3u];
                float khcl_d[3];
                kh_cloth_v3_sub(khcl_pj, khcl_pi, khcl_d);
                const float khcl_l = kh_cloth_v3_len(khcl_d);
                if (khcl_l >= khcl_bd || khcl_l < 1.0e-9f) continue;
                const float* khcl_nj = &khcl_nm[j * 3u];
                if (kh_cloth_v3_dot(khcl_ni, khcl_nj) > -0.5f) continue;        // Not opposing.
                if (kh_cloth_v3_dot(khcl_d, khcl_ni) / khcl_l > -0.5f) continue;   // Beside, not behind.
                khcl_bd = khcl_l; khcl_best = j;
            }
        }
        if (khcl_best == 0xFFFFFFFFu) continue;
        if (!khcl_have.emplace(khcl_pk(i, khcl_best), 1u).second) continue;
        KhClothDist khcl_c;
        khcl_c.a = i; khcl_c.b = khcl_best; khcl_c.rest = 0.0f;   // Filled by the caller.
        khcl_in.st.stretch.push_back(khcl_c);
    }
}

// Rest lengths are metric, so they must be re-derived whenever the object's size changes (kh_cloth_sync also asks on a
// fresh build and on a 'shell' change). The shell links are rebuilt first: their search radius is metric too.
inline void kh_cloth_rest_lengths(KhClothInst& khcr_in, const float* khcr_size) {
    kh_cloth_build_shell(khcr_in, khcr_size);
    const float khcr_org[3] = { 0.0f, 0.0f, 0.0f };
    const float khcr_id[9] = { 1,0,0, 0,1,0, 0,0,1 };
    std::vector<KhClothDist>* khcr_sets[2] = { &khcr_in.st.stretch, &khcr_in.st.bend };
    for (int khcr_s = 0; khcr_s < 2; ++khcr_s) {
        for (size_t khcr_i = 0; khcr_i < khcr_sets[khcr_s]->size(); ++khcr_i) {
            KhClothDist& khcr_c = (*khcr_sets[khcr_s])[khcr_i];
            float khcr_a[3], khcr_b[3];
            kh_cloth_l2w(khcr_in.st.part[khcr_c.a].rest, khcr_org, khcr_id, false, khcr_size, khcr_a);
            kh_cloth_l2w(khcr_in.st.part[khcr_c.b].rest, khcr_org, khcr_id, false, khcr_size, khcr_b);
            float khcr_d[3];
            kh_cloth_v3_sub(khcr_b, khcr_a, khcr_d);
            khcr_c.rest = kh_cloth_v3_len(khcr_d);
        }
    }
    kh_cloth_build_tethers(khcr_in.st.part, khcr_in.st.stretch);
}

// Write the stepped particles back into a MeshVertex array and rebuild the normals. Positions go back to MESH-LOCAL
// space: the cloth substitutes a vertex buffer, it does not change any pass's object transform.
inline void kh_cloth_writeback(KhClothInst& khcw_in, std::vector<MeshVertex>& khcw_out) {
    const size_t khcw_nv = khcw_in.rest.size();
    float khcw_inv[3];
    for (int k = 0; k < 3; ++k) {
        const float khcw_s = fabsf(khcw_in.st.size[k]) < 1.0e-6f ? 1.0e-6f : khcw_in.st.size[k];
        khcw_inv[k] = 1.0f / khcw_s;
    }
    for (size_t khcw_i = 0; khcw_i < khcw_nv; ++khcw_i) {
        const uint32_t khcw_p = khcw_i < khcw_in.vmap.size() ? khcw_in.vmap[khcw_i] : 0xFFFFFFFFu;
        if (khcw_p >= khcw_in.st.part.size()) continue;   // An LOD-only vertex keeps its rest pose.
        kh_cloth_w2l(khcw_in.st.part[khcw_p].p, khcw_in.st.centre, khcw_in.st.rot,
                     khcw_in.st.rotated, khcw_inv, khcw_out[khcw_i].pos);
    }
    // Normals, accumulated per SMOOTHING GROUP (kh_cloth_build's nrm_rep): neither per particle, which rounds off
    // every hard edge the model was built with, nor per render vertex, which splits a UV seam as well - each copy of
    // a seam vertex then sees only the faces on its own side, and the two disagree by up to a whole face angle
    // wherever the cloth bends across the seam (a lit line along it).
    std::fill(khcw_in.nrm_acc.begin(), khcw_in.nrm_acc.end(), 0.0f);
    // Tangents, per RENDER vertex: a UV seam's copies carry different tangents by construction, each of its own
    // chart. The mesh's tangents describe the REST shape; the pixel shader's re-orthogonalisation recovers a bend
    // about the tangent, or one across it below 90 degrees, and nothing else (a patch turned in its plane would turn
    // its normal map with it). A tangent is a pushforward: each face maps its rest edges onto its deformed edges, and
    // that map applied to the authored tangent is the deformed one - the authored frame at rest to float precision,
    // exact per face for any bend, turn or shear. Rest and deformed positions are both normalized mesh space, the
    // covariant space KhVsCore scales by size.
    std::fill(khcw_in.tan_acc.begin(), khcw_in.tan_acc.end(), 0.0f);
    const bool khcw_tan = khcw_in.tan_acc.size() == khcw_nv * 3u;
    const bool khcw_grp = khcw_in.nrm_rep.size() == khcw_nv;
    // KH_CLOTH_OWN_IDX: the instance's own copy of level 0's indices. Do not read mesh_def(mesh).indices here: the
    // worker has no hand-off from kh_mesh_gc (the cache writer has the busy id; this has nothing) and would read the
    // slot pointer without the lock its tombstone write holds.
    const std::vector<uint32_t>& khcw_ix = khcw_in.lvl0_idx;
    const uint32_t khcw_ni = static_cast<uint32_t>(khcw_ix.size());
    for (uint32_t khcw_t = 0; khcw_t + 2u < khcw_ni; khcw_t += 3u) {
        const uint32_t a = khcw_ix[khcw_t], b = khcw_ix[khcw_t + 1u], c = khcw_ix[khcw_t + 2u];
        if (a >= khcw_nv || b >= khcw_nv || c >= khcw_nv) continue;
        float e1[3], e2[3], n[3];
        kh_cloth_v3_sub(khcw_out[b].pos, khcw_out[a].pos, e1);
        kh_cloth_v3_sub(khcw_out[c].pos, khcw_out[a].pos, e2);
        n[0] = e1[1] * e2[2] - e1[2] * e2[1];
        n[1] = e1[2] * e2[0] - e1[0] * e2[2];
        n[2] = e1[0] * e2[1] - e1[1] * e2[0];
        // Unnormalised: the cross product's length is twice the triangle area, so accumulating it is an area weighting.
        const uint32_t khcw_v[3] = { a, b, c };
        for (int khcw_k = 0; khcw_k < 3; ++khcw_k) {
            const uint32_t khcw_r = khcw_grp ? khcw_in.nrm_rep[khcw_v[khcw_k]] : khcw_v[khcw_k];
            float* khcw_acc = &khcw_in.nrm_acc[static_cast<size_t>(khcw_r) * 3u];
            khcw_acc[0] += n[0]; khcw_acc[1] += n[1]; khcw_acc[2] += n[2];
        }
        if (!khcw_tan) continue;
        // The face's rest edges r1, r2 and rest normal rn. A tangent t splits into t.rn along rn and an in-plane part
        // ca * r1 + cb * r2 (solved through the edges' Gram matrix, whose determinant is |rn|^2); the face carries r1
        // to e1, r2 to e2 and the unit rest normal to the unit deformed one.
        float r1[3], r2[3], rn[3];
        kh_cloth_v3_sub(khcw_in.rest[b].pos, khcw_in.rest[a].pos, r1);
        kh_cloth_v3_sub(khcw_in.rest[c].pos, khcw_in.rest[a].pos, r2);
        rn[0] = r1[1] * r2[2] - r1[2] * r2[1];
        rn[1] = r1[2] * r2[0] - r1[0] * r2[2];
        rn[2] = r1[0] * r2[1] - r1[1] * r2[0];
        const float khcw_rn2 = kh_cloth_v3_dot(rn, rn);
        const float khcw_nl = kh_cloth_v3_len(n);
        if (!(khcw_rn2 > 1.0e-30f) || !(khcw_nl > 1.0e-15f)) continue;   // No face to map through.
        const float khcw_rnl = sqrtf(khcw_rn2);
        const float khcw_g11 = kh_cloth_v3_dot(r1, r1), khcw_g12 = kh_cloth_v3_dot(r1, r2);
        const float khcw_g22 = kh_cloth_v3_dot(r2, r2);
        for (int khcw_k = 0; khcw_k < 3; ++khcw_k) {
            const float* khcw_t = khcw_in.rest[khcw_v[khcw_k]].tan;
            const float khcw_tc = kh_cloth_v3_dot(khcw_t, rn) / khcw_rn2;
            const float khcw_tp[3] = { khcw_t[0] - khcw_tc * rn[0], khcw_t[1] - khcw_tc * rn[1],
                                       khcw_t[2] - khcw_tc * rn[2] };
            const float khcw_q1 = kh_cloth_v3_dot(r1, khcw_tp), khcw_q2 = kh_cloth_v3_dot(r2, khcw_tp);
            const float khcw_ca = (khcw_g22 * khcw_q1 - khcw_g12 * khcw_q2) / khcw_rn2;
            const float khcw_cb = (khcw_g11 * khcw_q2 - khcw_g12 * khcw_q1) / khcw_rn2;
            const float khcw_cn = khcw_tc * khcw_rnl / khcw_nl;
            // Weighted by the face's rest area, as the normals are by theirs.
            float* khcw_ta = &khcw_in.tan_acc[static_cast<size_t>(khcw_v[khcw_k]) * 3u];
            for (int q = 0; q < 3; ++q) {
                khcw_ta[q] += khcw_rnl * (khcw_ca * e1[q] + khcw_cb * e2[q] + khcw_cn * n[q]);
            }
        }
    }
    for (size_t khcw_i = 0; khcw_i < khcw_nv; ++khcw_i) {
        const uint32_t khcw_p = khcw_i < khcw_in.vmap.size() ? khcw_in.vmap[khcw_i] : 0xFFFFFFFFu;
        if (khcw_p >= khcw_in.st.part.size()) continue;
        if (khcw_tan) {
            // The handedness in w is the chart's and does not deform. A vertex with no face to map through keeps
            // what this buffer last carried, as its normal does below.
            const float* khcw_ta = &khcw_in.tan_acc[khcw_i * 3u];
            const float khcw_tl = kh_cloth_v3_len(khcw_ta);
            if (khcw_tl > 1.0e-20f) {
                khcw_out[khcw_i].tan[0] = khcw_ta[0] / khcw_tl;
                khcw_out[khcw_i].tan[1] = khcw_ta[1] / khcw_tl;
                khcw_out[khcw_i].tan[2] = khcw_ta[2] / khcw_tl;
            }
        }
        const size_t khcw_r = khcw_grp ? khcw_in.nrm_rep[khcw_i] : khcw_i;
        const float* khcw_acc = &khcw_in.nrm_acc[khcw_r * 3u];
        const float khcw_l = kh_cloth_v3_len(khcw_acc);
        // A fully degenerate fan keeps the normal this buffer last carried (the authored one until a frame has a face).
        if (khcw_l < 1.0e-12f) continue;
        khcw_out[khcw_i].nrm[0] = khcw_acc[0] / khcw_l;
        khcw_out[khcw_i].nrm[1] = khcw_acc[1] / khcw_l;
        khcw_out[khcw_i].nrm[2] = khcw_acc[2] / khcw_l;
    }
}

inline uint32_t kh_cloth_shape_note(KhClothInst& khsn_in, const std::vector<MeshVertex>& khsn_out) {
    const size_t khsn_n = khsn_out.size();
    bool khsn_moved = khsn_in.shape_key == 0u || khsn_in.shape_ref.size() != khsn_n * 3u;
    if (!khsn_moved) {
        // The positions are mesh-local, divided by size per axis.
        float khsn_tol[3];
        for (int k = 0; k < 3; ++k) {
            const float khsn_s = fabsf(khsn_in.st.size[k]);
            khsn_tol[k] = KH_CLOTH_SHAPE_M / (khsn_s > 1.0e-6f ? khsn_s : 1.0e-6f);
        }
        for (size_t khsn_i = 0; khsn_i < khsn_n && !khsn_moved; ++khsn_i) {
            const float* khsn_r = &khsn_in.shape_ref[khsn_i * 3u];
            for (int k = 0; k < 3; ++k) {
                if (!(fabsf(khsn_out[khsn_i].pos[k] - khsn_r[k]) <= khsn_tol[k])) { khsn_moved = true; break; }
            }
        }
    }
    if (khsn_moved) {
        khsn_in.shape_ref.resize(khsn_n * 3u);
        for (size_t khsn_i = 0; khsn_i < khsn_n; ++khsn_i) {
            memcpy(&khsn_in.shape_ref[khsn_i * 3u], khsn_out[khsn_i].pos, sizeof(float) * 3u);
        }
        uint32_t khsn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        if (khsn_k == 0u) khsn_k = g_cloth_shape_serial.fetch_add(1u, std::memory_order_relaxed) + 1u;
        khsn_in.shape_key = khsn_k;
    }
    return khsn_in.shape_key;
}

// KH_SKEL - the skeleton's targets for this frame, ON THE WORKER, from the inputs: the frame's end into guide_b, and
// into guide_a where the targets stood at the simulation's time - sub_t of the way from what guide_a held to what
// guide_b held (KH_SUB_CLOCK), the last frame's end itself when nothing was left over. A particle something pins,
// wholly or in part, goes where its bones take its authored position, re-expressed in the frame's rest box (the
// frame's centre stands for the authored native_ctr - kh_cloth_sync); a simulated particle keeps its rest position
// (the simulation moves it). The first guided frame starts where it ends; an unguided frame clears both, and every
// target is 'rest' again.
// One particle's guided position: its rest, skinned by its influences through the guide's maps (linear blend
// skinning as kh_skin_run does it, missing weight to the root, the identity), in the cloth's local frame - metres
// from auth_ctr over khga_is (1 / the size). The rest and the influences are the build's; the guide may be any frame's.
inline void kh_cloth_guide_at(const KhClothInst& khga_in, const float* khga_m0, size_t khga_nb, const float* khga_is,
                              size_t khga_i, float* khga_g) {
    const KhClothPart& khga_p = khga_in.st.part[khga_i];
    float khga_a[3];   // Authored: engine axes, metres, the model's frame.
    for (int k = 0; k < 3; ++k) khga_a[k] = khga_p.rest[k] * khga_in.auth_ext[k] + khga_in.auth_ctr[k];
    const KhSkinInf& khga_f = khga_in.part_inf[khga_i];
    float khga_q[3] = { 0.0f, 0.0f, 0.0f };
    float khga_ws = 0.0f;
    for (int j = 0; j < 4; ++j) {
        const float khga_w = khga_f.w[j];
        if (!(khga_w > 0.0f) || khga_f.b[j] >= khga_nb) continue;
        const float* khga_m = khga_m0 + static_cast<size_t>(khga_f.b[j]) * 12u;
        for (int c = 0; c < 3; ++c) {
            khga_q[c] += khga_w * (khga_a[0] * khga_m[c] + khga_a[1] * khga_m[3 + c] + khga_a[2] * khga_m[6 + c] +
                                   khga_m[9 + c]);
        }
        khga_ws += khga_w;
    }
    const float khga_r = khga_ws < 1.0f ? 1.0f - khga_ws : 0.0f;
    for (int k = 0; k < 3; ++k) khga_g[k] = (khga_q[k] + khga_r * khga_a[k] - khga_in.auth_ctr[k]) * khga_is[k];
}
inline void kh_cloth_guide_prep(KhClothInst& khgp_in) {
    KhClothState& khgp_s = khgp_in.st;
    const size_t khgp_n = khgp_s.part.size();
    const size_t khgp_nb = khgp_in.in_guide.size() / 12u;
    if (khgp_nb == 0u || khgp_in.part_inf.size() != khgp_n) {
        khgp_s.guide_a.clear();
        khgp_s.guide_b.clear();
        return;
    }
    const bool khgp_had = khgp_s.guide_b.size() == khgp_n * 3u;
    // KH_SUB_CLOCK: lane a stays the simulation's time, blended in place as
    // kh_cloth_target blends it; with nothing left over it is lane b (the swap).
    if (khgp_had && khgp_s.guide_a.size() == khgp_n * 3u && khgp_s.sub_t < 1.0f) {
        if (khgp_s.sub_t > 0.0f) {
            for (size_t i = 0; i < khgp_n * 3u; ++i) {
                khgp_s.guide_a[i] = khgp_s.guide_a[i] + (khgp_s.guide_b[i] - khgp_s.guide_a[i]) * khgp_s.sub_t;
            }
        }
    } else {
        std::swap(khgp_s.guide_a, khgp_s.guide_b);
    }
    khgp_s.guide_b.resize(khgp_n * 3u);
    const float* khgp_m0 = khgp_in.in_guide.data();
    float khgp_is[3];
    for (int k = 0; k < 3; ++k) {
        const float khgp_v = fabsf(khgp_in.in_size[k]) < 1.0e-6f ? 1.0e-6f : khgp_in.in_size[k];
        khgp_is[k] = 1.0f / khgp_v;
    }
    for (size_t i = 0; i < khgp_n; ++i) {
        const KhClothPart& khgp_p = khgp_s.part[i];
        float* khgp_g = &khgp_s.guide_b[i * 3u];
        if (khgp_p.w > 0.0f && !(khgp_p.att > 0.0f)) {   // Simulated: the bones do nothing here.
            memcpy(khgp_g, khgp_p.rest, sizeof(khgp_p.rest));
            continue;
        }
        kh_cloth_guide_at(khgp_in, khgp_m0, khgp_nb, khgp_is, i, khgp_g);
    }
    if (!khgp_had) khgp_s.guide_a = khgp_s.guide_b;
}

// KH_SKEL - a boxed instance's buffer b from sim_out: positions, normals and tangents re-expressed in the box of the
// drawn shape - the exact box of the level-0 vertices, since a cloth is lod_locked and its decimated levels are never
// drawn - under KhVsCore's convention (a position divided by the box, a normal multiplied by it, a tangent divided by
// it, each against the frame's own size first). The box is published with the buffer, and the object takes it where the
// buffer is installed (kh_skin_upload), so the drawn bounds follow the cloth and the skeleton instead of standing in
// the rest box.
inline void kh_cloth_box_out(KhClothInst& khbo_in, uint32_t khbo_b) {
    const std::vector<MeshVertex>& khbo_src = khbo_in.sim_out;
    std::vector<MeshVertex>& khbo_dst = khbo_in.out[khbo_b];
    const size_t khbo_nv = khbo_src.size();
    khbo_dst.resize(khbo_nv);
    float khbo_s[3];
    for (int k = 0; k < 3; ++k) khbo_s[k] = fabsf(khbo_in.st.size[k]) < 1.0e-6f ? 1.0e-6f : khbo_in.st.size[k];
    float khbo_mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, khbo_mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    const size_t khbo_np = khbo_in.st.part.size();
    for (size_t i = 0; i < khbo_nv; ++i) {
        if (i >= khbo_in.vmap.size() || khbo_in.vmap[i] >= khbo_np) continue;   // Not level 0.
        for (int k = 0; k < 3; ++k) {
            const float khbo_m = khbo_src[i].pos[k] * khbo_s[k];
            if (khbo_m < khbo_mn[k]) khbo_mn[k] = khbo_m;
            if (khbo_m > khbo_mx[k]) khbo_mx[k] = khbo_m;
        }
    }
    float khbo_c[3], khbo_e[3];
    for (int k = 0; k < 3; ++k) {
        if (!(khbo_mn[k] <= khbo_mx[k])) { khbo_mn[k] = -0.5f * khbo_s[k]; khbo_mx[k] = 0.5f * khbo_s[k]; }
        khbo_c[k] = 0.5f * (khbo_mn[k] + khbo_mx[k]);
        const float khbo_w = khbo_mx[k] - khbo_mn[k];
        khbo_e[k] = khbo_w > 1.0e-4f ? khbo_w : 1.0e-4f;   // A flat axis, as the skin and native_size.
    }
    for (size_t i = 0; i < khbo_nv; ++i) {
        const MeshVertex& khbo_v = khbo_src[i];
        MeshVertex& khbo_o = khbo_dst[i];
        float khbo_n[3], khbo_t[3];
        for (int k = 0; k < 3; ++k) {
            khbo_o.pos[k] = (khbo_v.pos[k] * khbo_s[k] - khbo_c[k]) / khbo_e[k];
            khbo_n[k] = khbo_v.nrm[k] * (khbo_e[k] / khbo_s[k]);
            khbo_t[k] = khbo_v.tan[k] * (khbo_s[k] / khbo_e[k]);
        }
        // Rescaled to unit length: the shader normalizes after its own scale,
        // so only the direction matters, and the ratios above can be large.
        const float khbo_nl = kh_cloth_v3_len(khbo_n);
        const float khbo_tl = kh_cloth_v3_len(khbo_t);
        for (int k = 0; k < 3; ++k) {
            khbo_o.nrm[k] = khbo_nl > 1.0e-30f ? khbo_n[k] / khbo_nl : khbo_v.nrm[k];
            khbo_o.tan[k] = khbo_tl > 1.0e-30f ? khbo_t[k] / khbo_tl : khbo_v.tan[k];
        }
        khbo_o.tan[3] = khbo_v.tan[3];
        khbo_o.uv[0] = khbo_v.uv[0];
        khbo_o.uv[1] = khbo_v.uv[1];
    }
    khbo_in.box_on[khbo_b] = true;
    for (int k = 0; k < 3; ++k) {
        khbo_in.box_ctr[khbo_b][k] = khbo_in.auth_ctr[k] + khbo_c[k];
        khbo_in.box_size[khbo_b][k] = khbo_e[k];
    }
}

// One job. Runs on a cloth worker and touches nothing the game thread may be
// writing: 'busy' was set before this instance was queued and is cleared last.
inline void kh_cloth_run(const std::shared_ptr<KhClothInst>& khcj_in) {
    KH_PROF_SCOPE(KHP_W_CLOTH_SIM);   // KH_PROF.
    KhClothInst& khcj = *khcj_in;
    const uint32_t khcj_back = 1u - khcj.front.load(std::memory_order_relaxed);
    if (khcj.chain) {
        // KH_CHAIN: the chain's step and its skinning, then the same publish.
        const size_t khcj_nb = khcj.in_guide.size() / 12u;
        const bool khcj_g = khcj_nb != 0u && khcj_nb == khcj.ch.body_of.size();
        KhChainParams khcj_cp = khcj.cpar;   // KH_SIM_LOD: the dials at this step's counts.
        if (khcj.in_iter >= 1u && khcj.in_sub >= 1u) {   // KH_CHAIN_LOD_FLOOR: never below the chain's floor.
            khcj_cp.iterations = kh_chain_lod_floor(khcj.cpar.iterations, khcj.in_iter, KH_CHAIN_LOD_ITER);
            khcj_cp.substeps = kh_chain_lod_floor(khcj.cpar.substeps, khcj.in_sub, KH_CHAIN_LOD_SUB);
        }
        kh_chain_step(khcj.st, khcj.ch, khcj_cp, khcj.in_col.data(), khcj.in_col.size(),
                      khcj.in_centre, khcj.in_rot, khcj.in_rotated, khcj.in_size, khcj.in_dt, khcj.in_wind,
                      khcj_g ? khcj.in_guide.data() : nullptr, khcj_g ? khcj_nb : 0u, khcj.in_tg,
                      khcj.in_aff.data(), khcj.in_aff.size(), &khcj.in_ground);
        kh_chain_writeback(khcj, khcj_back);
        // KH_CHAIN_JOB_GUIDE: the guide this front was solved against (none: the rest pose), with it - what
        // kh_chain_end_draw corrects an end held on a bone from. The cloth branch's twin is KH_CLOTH_PIN_GRADE's.
        // KH_CHAIN_HELD_REC: the guide as the step HELD it - c.guide_prev, which kh_chain_step leaves at the
        // substeps' end (cov of the way from the last step's held guide to this frame's, kh_chain_bone_map's blend;
        // the frame's own when nothing was left over), not the frame's end as given.
        if (khcj_g && khcj.ch.guide_prev.size() == khcj.in_guide.size()) khcj.out_guide[khcj_back] = khcj.ch.guide_prev;
        else if (khcj_g) khcj.out_guide[khcj_back] = khcj.in_guide;
        else khcj.out_guide[khcj_back].clear();
    } else {
    kh_cloth_guide_prep(khcj);   // KH_SKEL.
    KhClothParams khcj_pr = khcj.par;   // KH_SIM_LOD: the dials at this step's counts (par itself stays the object's).
    if (khcj.in_iter >= 1u && khcj.in_sub >= 1u) { khcj_pr.iterations = khcj.in_iter; khcj_pr.substeps = khcj.in_sub; }
    kh_cloth_step(khcj.st, khcj_pr, khcj.in_col.data(), khcj.in_col.size(),
                  khcj.in_centre, khcj.in_rot, khcj.in_rotated, khcj.in_size, khcj.in_dt,
                  khcj.in_wind, khcj.in_aff.data(), khcj.in_aff.size(), &khcj.in_ground);
    if (khcj.in_boxed) {
        // KH_SKEL: written and keyed in the frame's own space, drawn in the shape's box.
        if (khcj.sim_out.size() != khcj.rest.size()) khcj.sim_out = khcj.rest;
        kh_cloth_writeback(khcj, khcj.sim_out);
        khcj.shape_of[khcj_back] = kh_cloth_shape_note(khcj, khcj.sim_out);   // KH_CLOTH_SHAPE.
        kh_cloth_box_out(khcj, khcj_back);
    } else {
        // A buffer last written in a shape's box starts over from rest: what the writeback leaves as it was (the LOD
        // tail, a degenerate fan) must be in the frame's space.
        if (khcj.box_on[khcj_back]) {
            khcj.out[khcj_back] = khcj.rest;
            khcj.box_on[khcj_back] = false;
        }
        if (!khcj.sim_out.empty()) std::vector<MeshVertex>().swap(khcj.sim_out);
        kh_cloth_writeback(khcj, khcj.out[khcj_back]);
        khcj.shape_of[khcj_back] = kh_cloth_shape_note(khcj, khcj.out[khcj_back]);   // KH_CLOTH_SHAPE.
        {   // KH_CAST_REFIT: what the level-0 vertices reach, about the centre.
            const std::vector<MeshVertex>& khcj_o = khcj.out[khcj_back];
            const size_t khcj_np = khcj.st.part.size();
            float khcj_mx[3] = { 0.0f, 0.0f, 0.0f };
            for (size_t i = 0; i < khcj_o.size(); ++i) {
                if (i >= khcj.vmap.size() || khcj.vmap[i] >= khcj_np) continue;   // An LOD-only vertex keeps its rest pose.
                for (int k = 0; k < 3; ++k) { const float a = fabsf(khcj_o[i].pos[k]); if (a > khcj_mx[k]) khcj_mx[k] = a; }
            }
            for (int k = 0; k < 3; ++k) {   // pos is the frame's unit cube: metres = pos * size.
                const float khcj_s = fabsf(khcj.st.size[k]);
                const float khcj_r = 2.0f * khcj_mx[k] * khcj_s;
                khcj.cast_size[khcj_back][k] = (khcj_r == khcj_r && khcj_r > khcj_s) ? khcj_r : khcj_s;
            }
        }
    }
    // KH_CLOTH_PIN_GRADE: what this front's targets were (inputs stand while busy) - as the step held them
    // (KH_CLOTH_HELD_REC): the last held guide blended sub_t of the way to this frame's, as kh_cloth_guide_prep
    // blends lane a (sub_t 1: this frame's; 0, no substep ran: the last held stands); another bone count or none
    // held yet: this frame's.
    {
        const size_t khcj_gn = khcj.in_guide.size();
        const float khcj_gt = khcj.st.sub_t;
        if (khcj_gn != 0u && khcj.guide_held.size() == khcj_gn && khcj_gt < 1.0f) {
            if (khcj_gt > 0.0f) {
                for (size_t i = 0; i < khcj_gn; ++i) {
                    khcj.guide_held[i] = khcj.guide_held[i] + (khcj.in_guide[i] - khcj.guide_held[i]) * khcj_gt;
                }
            }
        } else {
            khcj.guide_held = khcj.in_guide;
        }
        khcj.out_guide[khcj_back] = khcj.guide_held;
    }
    }
    khcj.front.store(khcj_back, std::memory_order_release);
    khcj.gen.fetch_add(1u, std::memory_order_release);
    // Last, and after the publish: the game thread may write this instance's inputs the instant it turns false.
    for (std::shared_ptr<const KhPhysicsBvh>& khcj_o : khcj.in_col_own) khcj_o.reset();
    khcj.running.store(false, std::memory_order_release);   // KH_CHAIN_END_RACE: its buffers are written.
    khcj.busy.store(false, std::memory_order_release);
}

// SQF [x, y, zASL] -> engine [x, zASL, y], and the same swap for the edge lengths, against a plain float triple. The
// fill sites do the same swap inline into ConstantData lanes: a convention change must find them all.
inline void kh_cloth_engine_pos(const RenderObject& khce_o, float* khce_out) {
    khce_out[0] = khce_o.pos[0]; khce_out[1] = khce_o.pos[2]; khce_out[2] = khce_o.pos[1];
}
inline void kh_cloth_engine_size(const RenderObject& khce_o, float* khce_out) {
    khce_out[0] = khce_o.size[0]; khce_out[1] = khce_o.size[2]; khce_out[2] = khce_o.size[1];
}

// The carrier's centre in the world (engine axes): khca_wc the world float, khca_wd exact (KH_POS_RES) - the frame
// the write-back makes a chain's or cloth's vertices relative to. A skeletal object stands at root + skel_ctr * rot
// (kh_skel_centre); the rest box's centre is the authored native_ctr, at root + native_ctr * rot. kh_cloth_sync
// builds its instance's frame from it, and kh_chain_end_draw the frame a chain is drawn in (KH_CHAIN_END_DRAW).
inline void kh_cloth_carrier_at(const RenderObject& khca_o, const float khca_nc[3], float khca_wc[3],
                                double khca_wd[3]) {
    kh_cloth_engine_pos(khca_o, khca_wc);
    {
        float khca_wr[3];
        kh_pos_res(khca_o, khca_wr);
        for (int k = 0; k < 3; ++k) khca_wd[k] = static_cast<double>(khca_wc[k]) + static_cast<double>(khca_wr[k]);
    }
    if (!khca_o.skel) return;
    float khca_d[3];
    for (int k = 0; k < 3; ++k) khca_d[k] = khca_nc[k] - khca_o.skel_ctr[k];
    if (khca_o.rotated) {
        for (int k = 0; k < 3; ++k) {
            const float khca_a = khca_d[0] * khca_o.rot_m[k] + khca_d[1] * khca_o.rot_m[3 + k] +
                                 khca_d[2] * khca_o.rot_m[6 + k];
            khca_wc[k] += khca_a;
            khca_wd[k] += static_cast<double>(khca_a);
        }
    } else {
        for (int k = 0; k < 3; ++k) {
            khca_wc[k] += khca_d[k];
            khca_wd[k] += static_cast<double>(khca_d[k]);
        }
    }
}
inline bool kh_cloth_obj_sim(const RenderObject& khco_o) {
    return khco_o.cloth_sim && !khco_o.fullscreen && khco_o.mesh >= 0;
}
inline bool kh_physics_obj_collider(const RenderObject& khco_o) {
    return khco_o.physics_collider && !khco_o.fullscreen && khco_o.mesh >= 0;
}

// Does this mesh carry cloth authoring at all? An unpainted model asked to simulate is a script error, not a silent
// no-op: kh_cloth_sync reports it on this (once per mesh), and kh_skin_sync keeps such a mesh for the skin.
inline bool kh_cloth_mesh_ok(int khcm_mesh) {
    if (!kh_mesh_alive(khcm_mesh)) return false;
    const MeshDef& khcm_d = mesh_def(khcm_mesh);
    return khcm_d.cloth_w.size() == khcm_d.verts.size() && !khcm_d.cloth_w.empty();
}

inline void kh_physics_gather_colliders(const std::vector<RenderObject>& khcg_objs,
                                      std::vector<KhPhysicsColSrc>& khcg_out) {
    KH_PROF_SCOPE(KHP_PHYSICS_GATHER);   // KH_PROF.
    khcg_out.clear();
    ++g_physics_bvh_frame;
    for (size_t khcg_i = 0; khcg_i < khcg_objs.size(); ++khcg_i) {
        const RenderObject& khcg_o = khcg_objs[khcg_i];
        // A hidden collider is not drawn and still collides.
        if (!kh_physics_obj_collider(khcg_o)) continue;
        if (!kh_mesh_alive(khcg_o.mesh)) continue;
        float khcg_sz[3], khcg_bake[3];
        kh_cloth_engine_size(khcg_o, khcg_sz);
        std::shared_ptr<const KhPhysicsBvh> khcg_b;
        bool khcg_skin = false;
        bool khcg_fresh = false;   // KH_SKEL: the last park installed the pose, so its motion is this frame's.
        float khcg_sctr[3] = { 0.0f, 0.0f, 0.0f };   // KH_SKIN_COL_BOX: the BVH's own box centre (engine axes, the model's frame).
        if (khcg_o.skel) {
            // KH_SKEL: a skeletal binding collides as its installed skinned result's BVH, or as the rest mesh's while
            // its own vertices draw. Any other shape has no BVH and the collider sits the frame out. The table is the
            // one taken with the objects.
            // KH_SKIN_COL_BOX: the BVH is metres about ITS result's box centre (kh_skin_run: met - box_ctr), so it
            // stands at root + ctr * R whatever box the object draws in now - the render thread's step's (KH_SKEL_DRAW:
            // with KhSkinGpu::by_step the upload's put does not land, and the object's box is the step's, sampled apart
            // from the worker's and never byte-equal to it) or the put's. So the result is taken by mesh alone and
            // placed at its own centre below (khcg_sctr, in the box it was made in); the object's box is not required
            // to match it. The collider is then the worker's pose, a sample behind the step's draw at most.
            const KhSkinCol* khcg_sc = nullptr;
            if (g_skin_col_snap) {
                const auto khcg_it = g_skin_col_snap->find(khcg_o.seq);
                if (khcg_it != g_skin_col_snap->end()) khcg_sc = &khcg_it->second;
            }
            const MeshDef& khcg_md = mesh_def(khcg_o.mesh);
            if (khcg_sc && khcg_sc->bvh && khcg_sc->mesh == khcg_o.mesh) {
                khcg_b = khcg_sc->bvh;
                khcg_sz[0] = khcg_sc->size[0];   // The BVH's box: SQF order in, engine axes out (kh_cloth_engine_size).
                khcg_sz[1] = khcg_sc->size[2];
                khcg_sz[2] = khcg_sc->size[1];
                memcpy(khcg_bake, khcg_sz, sizeof(khcg_bake));
                memcpy(khcg_sctr, khcg_sc->ctr, sizeof(khcg_sctr));
                khcg_skin = true;
                // A pose published since the previous cloth sync sweeps; an older stamp holds still.
                khcg_fresh = kh_skin_col_fresh(khcg_sc->serial);
            } else if (memcmp(khcg_md.native_size, khcg_o.size, sizeof(khcg_o.size)) != 0 ||
                       memcmp(khcg_md.native_ctr, khcg_o.skel_ctr, sizeof(khcg_o.skel_ctr)) != 0) {
                continue;
            }
        } else if (khcg_o.chain_sim && memcmp(mesh_def(khcg_o.mesh).native_size, khcg_o.size, sizeof(khcg_o.size)) != 0) {
            // KH_CHAIN: a plain chain's box is its shape's, not its rest's, so like a simulating skeletal mesh it
            // collides as its rest mesh only while that draws, and otherwise sits the frame out.
            continue;
        }
        // A clothSimulation collider collides as its rest mesh (the shared per-mesh BVH below), not its simulated
        // shape: other cloth meets it where it was modelled, wherever it has sagged to.
        if (!khcg_skin) {
            const uint32_t khcg_shape = kh_physics_bvh_shape(khcg_sz, khcg_bake);
            khcg_b = kh_physics_bvh_for(mesh_id_clamp(khcg_o.mesh), khcg_shape, khcg_bake);
        }
        if (!khcg_b || khcg_b->empty()) continue;
        KhPhysicsColSrc khcg_s;
        khcg_s.own = khcg_b;
        KhPhysicsColliderView& khcg_v = khcg_s.view;
        khcg_v.bvh = khcg_b.get();
        memcpy(khcg_v.bake, khcg_bake, sizeof(khcg_v.bake));
        memcpy(khcg_v.rot, khcg_o.rot_m, sizeof(khcg_v.rot));
        khcg_v.rotated = khcg_o.rotated;
        // The object's engine-axes centre and edge lengths, the same pair the vertex shader uses, so the collider
        // occupies exactly the volume the player sees it occupy. KH_SKIN_COL_BOX: a skinned BVH's centre is its own
        // box's - the object stands at root + skel_ctr * R (kh_skel_centre), so the BVH's is that plus (ctr - skel_ctr)
        // * R - and its edge lengths that box's (khcg_sz).
        kh_cloth_engine_pos(khcg_o, khcg_v.centre);
        kh_pos_res(khcg_o, khcg_v.res);   // KH_POS_RES: added where the view enters an instance's frame.
        if (khcg_skin) {
            float khcg_d[3];
            for (int k = 0; k < 3; ++k) khcg_d[k] = khcg_sctr[k] - khcg_o.skel_ctr[k];
            for (int k = 0; k < 3; ++k) {
                khcg_v.centre[k] += khcg_o.rotated
                    ? khcg_d[0] * khcg_o.rot_m[k] + khcg_d[1] * khcg_o.rot_m[3 + k] + khcg_d[2] * khcg_o.rot_m[6 + k]
                    : khcg_d[k];
            }
        }
        memcpy(khcg_v.size, khcg_sz, sizeof(khcg_v.size));
        float khcg_bs = 0.0f;
        for (int k = 0; k < 3; ++k) {
            const float khcg_e = fabsf(khcg_v.size[k]) < 1.0e-6f ? 1.0e-6f : khcg_v.size[k];
            khcg_v.size[k] = khcg_e;
            khcg_v.inv_size[k] = 1.0f / khcg_e;
            khcg_v.bs_c[k] = khcg_v.centre[k];
            khcg_bs += 0.25f * khcg_e * khcg_e;
        }
        // The normalized mesh spans [-0.5, 0.5], so half the diagonal of the sized box bounds it whatever the rotation.
        khcg_v.bs_r = sqrtf(khcg_bs);
        khcg_v.margin = khcg_o.physics_col_margin;
        khcg_v.friction = khcg_o.physics_col_friction;
        khcg_v.inside = khcg_o.physics_col_inside;
        khcg_v.convex = khcg_o.physics_col_convex;   // KH_COL_CONVEX.
        khcg_v.slot = khcg_o.slot;
        khcg_v.mesh = mesh_id_clamp(khcg_o.mesh);
        khcg_v.skinned = khcg_skin;
        memcpy(khcg_v.skin_ctr, khcg_sctr, sizeof(khcg_v.skin_ctr));   // KH_SKIN_COL_BOX: the BVH's own box centre.
        if (khcg_o.skel) {   // KH_SKIN_COL_PREV: the centre above, less the root.
            memcpy(khcg_v.root_off, khcg_skin ? khcg_sctr : khcg_o.skel_ctr, sizeof(khcg_v.root_off));
        }
        // KH_SKEL: a pose the last park installed sweeps from the last one (KhPhysicsBvh::vd); one already collided
        // with last frame holds still. Its bound then covers both poses: the root box, metric here.
        khcg_v.deform = khcg_skin && khcg_fresh && !khcg_b->vd.empty();
        khcg_v.def0 = 0.0f;
        khcg_v.def1 = 0.0f;
        if (khcg_v.deform) {
            const KhPhysicsBvhNode& khcg_rn = khcg_b->nodes[0];
            float khcg_sr = 0.0f;
            for (int k = 0; k < 3; ++k) {
                const float khcg_e = fabsf(khcg_rn.mn[k]) > fabsf(khcg_rn.mx[k]) ? fabsf(khcg_rn.mn[k]) : fabsf(khcg_rn.mx[k]);
                khcg_sr += khcg_e * khcg_e;
            }
            khcg_sr = sqrtf(khcg_sr);
            if (khcg_sr > khcg_v.bs_r) khcg_v.bs_r = khcg_sr;
        }
        // No earlier pose is known here - kh_cloth_sync hands each cloth the
        // one it saw at its own last step - so the view starts out still.
        memcpy(khcg_v.prev.centre, khcg_v.centre, sizeof(khcg_v.prev.centre));
        memcpy(khcg_v.prev.rot, khcg_v.rot, sizeof(khcg_v.prev.rot));
        memcpy(khcg_v.prev.size, khcg_v.size, sizeof(khcg_v.prev.size));
        khcg_v.prev.rotated = khcg_v.rotated;
        khcg_v.vel[0] = 0.0f; khcg_v.vel[1] = 0.0f; khcg_v.vel[2] = 0.0f;
        khcg_out.push_back(khcg_s);
    }
    std::lock_guard<std::mutex> khcg_g(g_physics_bvh_mu);
    for (auto khcg_it = g_physics_bvh.begin(); khcg_it != g_physics_bvh.end();) {
        if (g_physics_bvh_frame - khcg_it->second.used > KH_PHYSICS_BVH_IDLE) khcg_it = g_physics_bvh.erase(khcg_it);
        else ++khcg_it;
    }
}

// Move an instance's origin by d (metres, engine axes): every stored position shifts by -d so no world position
// changes. Game thread, non-busy instance.
inline void kh_cloth_origin_shift(KhClothInst& khos_in, const double* khos_d) {
    const float khos_f[3] = { static_cast<float>(khos_d[0]), static_cast<float>(khos_d[1]),
                              static_cast<float>(khos_d[2]) };
    for (KhClothPart& khos_p : khos_in.st.part) {
        for (int k = 0; k < 3; ++k) { khos_p.p[k] -= khos_f[k]; khos_p.pp[k] -= khos_f[k]; }
    }
    for (int k = 0; k < 3; ++k) {
        khos_in.st.centre[k] -= khos_f[k];
        khos_in.st.fend.centre[k] -= khos_f[k];   // KH_SUB_CLOCK.
    }
    // Last frame's collider views: kh_cloth_sync reads each collider's previous pose from them before it rewrites them.
    for (KhPhysicsColliderView& khos_v : khos_in.in_col) {
        for (int k = 0; k < 3; ++k) { khos_v.centre[k] -= khos_f[k]; khos_v.bs_c[k] -= khos_f[k]; khos_v.prev.centre[k] -= khos_f[k]; }
    }
    // KH_CHAIN: the bodies, and the end's held and last targets, likewise.
    for (KhChainBody& khos_b : khos_in.ch.body) {
        for (int k = 0; k < 3; ++k) { khos_b.x[k] -= khos_f[k]; khos_b.xp[k] -= khos_f[k]; }
    }
    for (int k = 0; k < 3; ++k) { khos_in.ch.hold_p[k] -= khos_f[k]; khos_in.ch.prev_p[k] -= khos_f[k]; }
    for (int k = 0; k < 3; ++k) khos_in.org[k] += khos_d[k];
}

// The terrain nodes within the cloth's reach about its carrier's centre, at last step's centre and this one's (a
// fast carrier's whole path), into its frame. Game thread, non-busy instance, after its origin and reach are this
// frame's.
inline void kh_cloth_ground_patch(KhClothInst& khgc_in, const float* khgc_wc, float khgc_reach) {
    KhClothGround& khgc_g = khgc_in.in_ground;
    khgc_g.on = false;
    if (!g_ground_ready) return;
    const size_t khgc_n = g_ground_n;
    const float khgc_c = g_ground_cell;
    if (g_terrain_matrix.size() != khgc_n || g_terrain_grid_width != khgc_c) return;   // Rebuilt since: next frame.
    double khgc_lo[2] = { khgc_wc[0], khgc_wc[2] }, khgc_hi[2] = { khgc_wc[0], khgc_wc[2] };
    if (khgc_in.st.primed) {
        const double khgc_px = static_cast<double>(khgc_in.st.centre[0]) + khgc_in.org[0];
        const double khgc_pz = static_cast<double>(khgc_in.st.centre[2]) + khgc_in.org[2];
        khgc_lo[0] = std::min(khgc_lo[0], khgc_px); khgc_hi[0] = std::max(khgc_hi[0], khgc_px);
        khgc_lo[1] = std::min(khgc_lo[1], khgc_pz); khgc_hi[1] = std::max(khgc_hi[1], khgc_pz);
    }
    const double khgc_last = static_cast<double>(khgc_n - 1u);
    int64_t khgc_a[2], khgc_b[2];
    for (int k = 0; k < 2; ++k) {
        const double khgc_l = std::floor((khgc_lo[k] - khgc_reach) / khgc_c);
        const double khgc_h = std::ceil((khgc_hi[k] + khgc_reach) / khgc_c);
        if (!(khgc_h >= 0.0) || !(khgc_l <= khgc_last)) return;   // Off the map: no terrain known there.
        khgc_a[k] = static_cast<int64_t>(khgc_l < 0.0 ? 0.0 : khgc_l);
        khgc_b[k] = static_cast<int64_t>(khgc_h > khgc_last ? khgc_last : khgc_h);
        if (khgc_b[k] == khgc_a[k]) { if (khgc_b[k] > 0) --khgc_a[k]; else ++khgc_b[k]; }   // A cell, at least.
    }
    const uint32_t khgc_nx = static_cast<uint32_t>(khgc_b[0] - khgc_a[0] + 1);
    const uint32_t khgc_nz = static_cast<uint32_t>(khgc_b[1] - khgc_a[1] + 1);
    khgc_g.h.resize(static_cast<size_t>(khgc_nx) * khgc_nz);
    for (uint32_t khgc_j = 0; khgc_j < khgc_nz; ++khgc_j) {
        const std::vector<float>& khgc_row = g_terrain_matrix[static_cast<size_t>(khgc_a[1]) + khgc_j];
        float* khgc_dst = &khgc_g.h[static_cast<size_t>(khgc_j) * khgc_nx];
        for (uint32_t khgc_i = 0; khgc_i < khgc_nx; ++khgc_i) {
            khgc_dst[khgc_i] = static_cast<float>(static_cast<double>(khgc_row[static_cast<size_t>(khgc_a[0]) + khgc_i]) -
                                                  khgc_in.org[KH_CLOTH_UP]);
        }
    }
    // Node positions as the framework sampled them (float index * spacing).
    khgc_g.x0 = static_cast<float>(static_cast<double>(static_cast<float>(khgc_a[0]) * khgc_c) - khgc_in.org[0]);
    khgc_g.z0 = static_cast<float>(static_cast<double>(static_cast<float>(khgc_a[1]) * khgc_c) - khgc_in.org[2]);
    khgc_g.cell = khgc_c;
    khgc_g.nx = khgc_nx;
    khgc_g.nz = khgc_nz;
    khgc_g.px = static_cast<uint32_t>(khgc_a[0] & 1);
    khgc_g.pz = static_cast<uint32_t>(khgc_a[1] & 1);
    memcpy(khgc_g.split, g_ground_split, sizeof(khgc_g.split));
    khgc_g.on = true;
}
}