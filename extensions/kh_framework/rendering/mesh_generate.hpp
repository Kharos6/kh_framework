#pragma once

// The feature half (rendering_integration_features.hpp): generated meshes (KH_MESH_GEN) - a mesh built from a script's
// own faces (generateMesh, and the [shading, faces] form of the mesh slot: addRender3D's and updateRender3D "mesh"),
// parsed, built by the importer's rules and registered as one more registry mesh, so it draws, casts, takes materials,
// shaders and every other property as any mesh does. Never include it on its own.
//
// The spec, [shading, faces] (the script reference documents it for scripts):
//   shading  true = smooth: a corner's normal is the corner-angle-weighted mean of the face normals at its position
//            that turn 90 degrees or less from its own face's (so the back of a double-sided sheet stays apart);
//            false = flat: a corner takes its face's normal; a number = that crease angle in degrees, clamped to
//            [0, 180] (0 = flat, 180 = every face at the position). Not KH_BOOL_SCALAR: a number is an angle.
//   faces    [face, ...], each [corners, normal?, uvs?, submesh?]:
//     corners  three or more [x, y, z]: metres from the mesh's origin in the mesh's own axes (x right, y forward, z
//              up - the builtins' frame, before the object's rotation), a triangle or a convex planar polygon in order
//              around its edge (fanned from the first corner; a fan triangle that turns back - a concave or
//              self-crossing polygon - is refused with a report).
//     normal   [x, y, z], the face's outward direction (any length but zero; not in the face's plane - refused within
//              about 0.06 degrees of it): it decides the face's front, whatever the corners' order, and is the flat
//              shading's normal. nil / [] / left out = from the corners' order, counter-clockwise seen from the front.
//     uvs      one [u, v] per corner. nil / [] / left out = a box projection: the face's dominant axis picks the
//              plane, the mesh's bounds map to 0..1, upright and unmirrored seen from the front.
//     submesh  the face's submesh name (a material slot, in first-seen order: updateRender3D "material" selects it by
//              name or index; names that differ only in case are one submesh, spelled as first seen). nil / [] / "" /
//              left out = "default".
// The build (kh_gen_build): each corner is welded onto the first position already placed (the lowest id) whose every
// coordinate lies within the tolerance of it - KH_GEN_WELD_REL of the farthest coordinate, about 8 float steps, and
// KH_GEN_WELD_MIN at least - so faces meeting at a corner share it whatever rounding the script's arithmetic left (the
// weld is greedy: a chain of corners each within reach of the next is not one position); a triangle that faces its
// normal by no more than the coordinates' precision (KH_GEN_THIN_REL: collinear corners) is dropped, as is a face that
// thin, or one within the rounding noise whose normal lies in its rounded plane (a face all of whose triangles go draws
// nothing; a submesh all of whose faces go is refused with a report: the material slots after it would move up an
// index); fans of thousands of triangles around single points are refused past KH_GEN_SMOOTH_WORK (the smoothing and
// the vertex-cache order are quadratic in them); each face is wound to its normal as a whole; normals, uvs and the
// mikktspace tangents are made per corner in metres; then the importer's squash into the unit box - about the ORIGIN,
// not the bounds' centre (the box is twice the farthest extent per axis, KH_IMPORT_FLAT's 1e-4 m floor), so the
// object's position is the mesh's origin and it turns and scales about it (the cost: every bounds test takes that box,
// up to twice the geometry's where it sits off its origin, and the occluder proof seeds at the origin, so a closed mesh
// about a point outside it, or on its surface, gets no automatic occluder) - meshgen::bake, the weld (corners identical
// in position, normal, uv and tangent become one vertex: a smooth surface shares its vertices across faces, within each
// region its uvs agree over - the box projection splits it where a face's dominant axis changes; a flat one shares them
// within each face) and the occluder prep. No LOD levels, as the builtins.
// KH_MESH_GEN_SHARE: a spec identical to one a live generated mesh was built from (its FNV-1a 64 over the parsed spec)
// takes that mesh: copies cost no memory and draw in one instanced bucket. g_mesh_gen_cache, under g_fbx_cache_mutex as
// the import cache; kh_mesh_release forgets a released id's entry.
// KH_MESH_GEN_PUB: never under the graphics lock. The mesh is appended and its publish left to the render thread
// (g_mesh_publish_pending, the importer's own fallback): the next injection lands it ahead of its scene sync, else the
// flush (kh_mesh_publish_land). Until then kh_scene_sync holds every object naming it out of the live scene
// (KH_SCENE_UNPUB): a pass drawn earlier in that cycle keeps the object's last synced state (a new one: nothing)
// instead of drawing mesh 0. KH_MESH_GEN_GC: an unreferenced generated mesh is released after KH_MESH_GEN_GC_IDLE_MS
// (asset_gc.hpp), not the imports' two minutes - a rebuilt mesh's predecessors do not pile up. Registry ids are never
// reused (KH_MESH_FREE): every distinct spec built takes one of the registry's 1,048,576 for the process's life, so a
// mesh rebuilt every frame runs it out in hours - animate with a material's vertex stage (KH_USER_VS) or the object's
// transform, and rebuild only when the shape changes. Game thread only, as the importer.
namespace RenderIntegration {

static constexpr uint32_t KH_GEN_TRI_MAX = 262144u;   // Triangles after the fan: 37.7 MB of vertices at most.
static constexpr uint32_t KH_GEN_POLY_MAX = 256u;     // Corners per face.
static constexpr float    KH_GEN_POS_MAX = 5.0e6f;    // |coordinate|: the box stays under KH_IMPORT_BOUND's 1e7 m.
static constexpr float    KH_GEN_UV_MAX = 1.0e7f;     // |u|, |v|.
static constexpr float    KH_GEN_WELD_REL = 1.0e-6f;  // About 8 float steps of the farthest coordinate.
static constexpr float    KH_GEN_WELD_MIN = 1.0e-6f;  // Metres.
static constexpr double   KH_GEN_SMOOTH_WORK = 2.0e8;  // The sum over positions of (corners there)^2, most.
static constexpr double   KH_GEN_THIN_REL = 5.0e-7;     // Of the farthest coordinate: about 4 float steps of the
                                                       // squashed box - the least height a triangle keeps.
static constexpr double   KH_GEN_THIN_MIN = 1.0e-7;     // Metres.
static constexpr double   KH_GEN_EDGE_ON = 1.0e-3;     // The least cos of a face (or fan triangle) to its normal.

static std::unordered_map<uint64_t, int> g_mesh_gen_cache;   // KH_MESH_GEN_SHARE: spec hash -> mesh id.

struct KhGenFace {
    uint32_t c0 = 0;    // First corner: KhGenSpec::pos at 3 * c0, uv at 2 * c0.
    uint32_t cn = 0;    // Corner count (3 .. KH_GEN_POLY_MAX).
    uint32_t sub = 0;   // KhGenSpec::subs index.
    uint32_t src = 0;   // The face's index in the script's array (reports; not hashed).
    float nrm[3] = { 0.0f, 0.0f, 0.0f };   // Unit: as given, or from the corners (Newell).
    bool has_uv = false;
};
struct KhGenSpec {
    bool  flat = true;
    float crease_cos = 0.0f;   // Smooth: a face at the position blends in when its normal's dot with the corner's
                               // own face's is this or more.
    std::vector<float> pos;    // 3 per corner, authored frame (+0.0f: no negative zero).
    std::vector<float> uv;     // 2 per corner, zero where the face has none.
    std::vector<KhGenFace> faces;
    std::vector<std::string> subs;
};

// n finite numbers, each under khgv_lim in size; -0 folds to +0.
inline bool kh_gen_vec(const game_value& khgv_v, size_t khgv_n, float* khgv_out, float khgv_lim) {
    if (khgv_v.type_enum() != game_data_type::ARRAY) return false;
    auto& khgv_a = khgv_v.to_array();
    if (khgv_a.size() != khgv_n) return false;
    for (size_t khgv_i = 0; khgv_i < khgv_n; ++khgv_i) {
        if (khgv_a[khgv_i].type_enum() != game_data_type::SCALAR) return false;
        const float khgv_f = static_cast<float>(khgv_a[khgv_i]) + 0.0f;
        if (!(fabsf(khgv_f) < khgv_lim)) return false;   // NaN fails too.
        khgv_out[khgv_i] = khgv_f;
    }
    return true;
}
// An optional slot left empty: nil, or an empty array.
inline bool kh_gen_unset(const game_value& khgu_v) {
    return khgu_v.is_nil() || (khgu_v.type_enum() == game_data_type::ARRAY && khgu_v.to_array().size() == 0);
}

inline bool kh_gen_parse(const game_value& khgp_sh, const game_value& khgp_fv, KhGenSpec& khgp_s, std::string& err) {
    float khgp_ang = 0.0f;   // The crease angle: true = 90, false = 0.
    if (!khgp_sh.is_nil() && khgp_sh.type_enum() == game_data_type::BOOL) {
        khgp_ang = static_cast<bool>(khgp_sh) ? 90.0f : 0.0f;
    } else if (!khgp_sh.is_nil() && khgp_sh.type_enum() == game_data_type::SCALAR) {
        khgp_ang = static_cast<float>(khgp_sh);
        if (khgp_ang != khgp_ang) {
            err = "shading must be true (smooth), false (flat) or a crease angle in degrees";
            return false;
        }
        khgp_ang = khgp_ang < 0.0f ? 0.0f : (khgp_ang > 180.0f ? 180.0f : khgp_ang);   // RANGE RULE: clamped.
    } else {
        err = "shading must be true (smooth), false (flat) or a crease angle in degrees";
        return false;
    }
    // One value per meaning (KH_MESH_GEN_SHARE): false and 0 are both flat, true and 90 both a right angle.
    khgp_s.flat = khgp_ang <= 0.0f;
    khgp_s.crease_cos = khgp_s.flat || khgp_ang == 90.0f ? 0.0f
                      : (khgp_ang >= 180.0f ? -2.0f : cosf(khgp_ang * (3.14159265358979f / 180.0f)));
    static const char* const khgp_form = "faces must be an array of [corners, normal?, uvs?, submesh?] entries";
    if (khgp_fv.type_enum() != game_data_type::ARRAY) { err = khgp_form; return false; }
    auto& khgp_fa = khgp_fv.to_array();
    if (khgp_fa.size() == 0) { err = "faces is empty"; return false; }
    std::unordered_map<std::string, uint32_t> khgp_sub_ix;
    uint64_t khgp_tris = 0;
    auto khgp_fail = [&](size_t khgp_i, const char* khgp_m) {
        err = "face " + std::to_string(khgp_i) + ": " + khgp_m;
        return false;
    };
    for (size_t khgp_i = 0; khgp_i < khgp_fa.size(); ++khgp_i) {
        const game_value& khgp_e = khgp_fa[khgp_i];
        if (khgp_e.type_enum() != game_data_type::ARRAY) {
            return khgp_fail(khgp_i, "must be [corners, normal?, uvs?, submesh?]");
        }
        auto& khgp_ea = khgp_e.to_array();
        if (khgp_ea.size() < 1 || khgp_ea.size() > 4) {
            return khgp_fail(khgp_i, "must be [corners, normal?, uvs?, submesh?] (one to four elements)");
        }
        if (khgp_ea[0].type_enum() != game_data_type::ARRAY) {
            return khgp_fail(khgp_i, "corners must be an array of three or more [x, y, z]");
        }
        auto& khgp_ca = khgp_ea[0].to_array();
        if (khgp_ca.size() < 3 || khgp_ca.size() > KH_GEN_POLY_MAX) {
            return khgp_fail(khgp_i, "corners must be three to 256 [x, y, z] positions");
        }
        KhGenFace khgp_f;
        khgp_f.c0 = static_cast<uint32_t>(khgp_s.pos.size() / 3u);
        khgp_f.cn = static_cast<uint32_t>(khgp_ca.size());
        khgp_tris += khgp_f.cn - 2u;
        if (khgp_tris > KH_GEN_TRI_MAX) return khgp_fail(khgp_i, "the mesh passes 262144 triangles");
        for (size_t khgp_c = 0; khgp_c < khgp_ca.size(); ++khgp_c) {
            float khgp_p[3];
            if (!kh_gen_vec(khgp_ca[khgp_c], 3, khgp_p, KH_GEN_POS_MAX)) {
                return khgp_fail(khgp_i, "a corner must be [x, y, z], finite numbers under 5,000 km in size");
            }
            khgp_s.pos.insert(khgp_s.pos.end(), khgp_p, khgp_p + 3);
        }
        // Newell's normal of the corners as given: the right-hand rule's, so counter-clockwise faces the viewer.
        double khgp_nw[3] = { 0.0, 0.0, 0.0 };
        for (uint32_t khgp_c = 0; khgp_c < khgp_f.cn; ++khgp_c) {
            const float* khgp_a = &khgp_s.pos[3u * (khgp_f.c0 + khgp_c)];
            const float* khgp_b = &khgp_s.pos[3u * (khgp_f.c0 + (khgp_c + 1u) % khgp_f.cn)];
            khgp_nw[0] += (static_cast<double>(khgp_a[1]) - khgp_b[1]) * (static_cast<double>(khgp_a[2]) + khgp_b[2]);
            khgp_nw[1] += (static_cast<double>(khgp_a[2]) - khgp_b[2]) * (static_cast<double>(khgp_a[0]) + khgp_b[0]);
            khgp_nw[2] += (static_cast<double>(khgp_a[0]) - khgp_b[0]) * (static_cast<double>(khgp_a[1]) + khgp_b[1]);
        }
        double khgp_n[3] = { khgp_nw[0], khgp_nw[1], khgp_nw[2] };
        if (khgp_ea.size() > 1 && !kh_gen_unset(khgp_ea[1])) {
            float khgp_g[3];
            if (!kh_gen_vec(khgp_ea[1], 3, khgp_g, 3.0e38f)) {
                return khgp_fail(khgp_i, "normal must be [x, y, z] finite numbers (nil or [] = from the corners'"
                                         " order)");
            }
            for (int k = 0; k < 3; ++k) khgp_n[k] = khgp_g[k];
            if (!(khgp_n[0] != 0.0 || khgp_n[1] != 0.0 || khgp_n[2] != 0.0)) {
                return khgp_fail(khgp_i, "normal must not be [0, 0, 0] (nil or [] = from the corners' order)");
            }
        }
        const double khgp_nl = sqrt(khgp_n[0] * khgp_n[0] + khgp_n[1] * khgp_n[1] + khgp_n[2] * khgp_n[2]);
        const bool khgp_area = khgp_nl > 0.0 && khgp_nl < 1.0e300;
        if (khgp_area) for (int k = 0; k < 3; ++k) khgp_f.nrm[k] = static_cast<float>(khgp_n[k] / khgp_nl);
        khgp_s.uv.resize(khgp_s.pos.size() / 3u * 2u, 0.0f);
        if (khgp_ea.size() > 2 && !kh_gen_unset(khgp_ea[2])) {
            const char* const khgp_um = "uvs must be one [u, v] per corner, finite numbers (nil or [] = projected)";
            if (khgp_ea[2].type_enum() != game_data_type::ARRAY) return khgp_fail(khgp_i, khgp_um);
            auto& khgp_ua = khgp_ea[2].to_array();
            if (khgp_ua.size() != khgp_f.cn) return khgp_fail(khgp_i, khgp_um);
            for (uint32_t khgp_c = 0; khgp_c < khgp_f.cn; ++khgp_c) {
                if (!kh_gen_vec(khgp_ua[khgp_c], 2, &khgp_s.uv[2u * (khgp_f.c0 + khgp_c)], KH_GEN_UV_MAX)) {
                    return khgp_fail(khgp_i, khgp_um);
                }
            }
            khgp_f.has_uv = true;
        }
        std::string khgp_sn = "default";
        if (khgp_ea.size() > 3 && !kh_gen_unset(khgp_ea[3])) {
            if (khgp_ea[3].type_enum() != game_data_type::STRING) {
                return khgp_fail(khgp_i, "submesh must be a name (nil, [] or \"\" = \"default\")");
            }
            std::string khgp_g = static_cast<std::string>(khgp_ea[3]);
            if (!khgp_g.empty()) khgp_sn = std::move(khgp_g);
        }
        // Keyed lower-cased, as the material selector matches (kh_apply_material_update): spelled as first seen.
        std::string khgp_key = kh_lower_copy(std::string(khgp_sn));
        auto khgp_it = khgp_sub_ix.find(khgp_key);
        if (khgp_it == khgp_sub_ix.end()) {
            khgp_it = khgp_sub_ix.emplace(std::move(khgp_key), static_cast<uint32_t>(khgp_s.subs.size())).first;
            khgp_s.subs.push_back(khgp_sn);
        }
        // Its submesh is registered first, so a submesh whose faces all go is refused (kh_gen_build), never left
        // out with the later ones moving up an index. A face with no area (a corner order that encloses none, and no
        // normal to say where it faces) draws nothing: left out, its corners kept unread.
        if (!khgp_area) continue;
        khgp_f.sub = khgp_it->second;
        khgp_f.src = static_cast<uint32_t>(khgp_i);
        khgp_s.faces.push_back(khgp_f);
    }
    if (khgp_s.faces.empty()) { err = "no face encloses any area"; return false; }
    return true;
}

// KH_MESH_GEN_SHARE: FNV-1a 64 over every field of the parsed spec, each fed explicitly (KhGenFace has padding).
// Corners a face left out are hashed too: harmless, the same spec gives the same bytes.
inline uint64_t kh_gen_hash(const KhGenSpec& khgh_s) {
    uint64_t khgh_h = CryptoGenerator::FNV1A64_OFFSET;
    auto khgh_put = [&](const void* khgh_p, size_t khgh_n) {
        khgh_h = CryptoGenerator::fnv1a64_update(khgh_h, static_cast<const char*>(khgh_p), khgh_n);
    };
    const uint32_t khgh_hd[4] = { 0x4B47454Eu, khgh_s.flat ? 1u : 0u, static_cast<uint32_t>(khgh_s.faces.size()),
                                  static_cast<uint32_t>(khgh_s.pos.size()) };   // "KGEN".
    khgh_put(khgh_hd, sizeof(khgh_hd));
    khgh_put(&khgh_s.crease_cos, sizeof(float));
    for (const KhGenFace& khgh_f : khgh_s.faces) {
        const uint32_t khgh_u[4] = { khgh_f.c0, khgh_f.cn, khgh_f.sub, khgh_f.has_uv ? 1u : 0u };
        khgh_put(khgh_u, sizeof(khgh_u));
        khgh_put(khgh_f.nrm, sizeof(khgh_f.nrm));
    }
    if (!khgh_s.pos.empty()) khgh_put(khgh_s.pos.data(), khgh_s.pos.size() * sizeof(float));
    if (!khgh_s.uv.empty()) khgh_put(khgh_s.uv.data(), khgh_s.uv.size() * sizeof(float));
    for (const std::string& khgh_n : khgh_s.subs) {
        const uint32_t khgh_l = static_cast<uint32_t>(khgh_n.size());
        khgh_put(&khgh_l, sizeof(khgh_l));
        if (khgh_l) khgh_put(khgh_n.data(), khgh_n.size());
    }
    return khgh_h;
}

// The spec into a MeshDef ready to append (kh_gen_parse succeeded). False with err when nothing is left to draw.
inline bool kh_gen_build(const KhGenSpec& khgb_s, uint64_t khgb_hash, MeshDef& d, std::string& err) {
    // Positions: each corner a face uses, welded within the tolerance onto the first position near it (a 27-cell
    // search of a grid one tolerance wide; the lowest id among the positions in reach, so the result does not depend on
    // the search's order).
    float khgb_far = 0.0f;
    for (const KhGenFace& khgb_f : khgb_s.faces) {
        for (uint32_t khgb_c = 0; khgb_c < khgb_f.cn; ++khgb_c) {
            const float* khgb_p = &khgb_s.pos[3u * (khgb_f.c0 + khgb_c)];
            for (int k = 0; k < 3; ++k) khgb_far = fmaxf(khgb_far, fabsf(khgb_p[k]));
        }
    }
    const double khgb_tol = fmax(static_cast<double>(KH_GEN_WELD_MIN),
                                 static_cast<double>(KH_GEN_WELD_REL) * khgb_far);
    const double khgb_thin = fmax(KH_GEN_THIN_MIN, KH_GEN_THIN_REL * khgb_far);   // The least height that is area.
    struct KhGbCell {
        int64_t c[3];
        bool operator==(const KhGbCell& o) const { return c[0] == o.c[0] && c[1] == o.c[1] && c[2] == o.c[2]; }
    };
    struct KhGbCellHash {
        size_t operator()(const KhGbCell& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(k.c, sizeof(k.c)));
        }
    };
    std::unordered_map<KhGbCell, std::vector<uint32_t>, KhGbCellHash> khgb_grid;
    std::vector<float> khgb_P;   // Welded positions, 3 each.
    std::vector<uint32_t> khgb_pid(khgb_s.pos.size() / 3u, 0xFFFFFFFFu);   // Corner -> welded position.
    khgb_grid.reserve(khgb_s.pos.size() / 3u + 16u);
    for (const KhGenFace& khgb_f : khgb_s.faces) {
        for (uint32_t khgb_c = khgb_f.c0; khgb_c < khgb_f.c0 + khgb_f.cn; ++khgb_c) {
            const float* khgb_p = &khgb_s.pos[3u * khgb_c];
            KhGbCell khgb_k;
            for (int k = 0; k < 3; ++k) {
                khgb_k.c[k] = static_cast<int64_t>(floor(static_cast<double>(khgb_p[k]) / khgb_tol));
            }
            uint32_t khgb_best = 0xFFFFFFFFu;
            for (int khgb_dz = -1; khgb_dz <= 1; ++khgb_dz)
                for (int khgb_dy = -1; khgb_dy <= 1; ++khgb_dy)
                    for (int khgb_dx = -1; khgb_dx <= 1; ++khgb_dx) {
                        const KhGbCell khgb_n = { { khgb_k.c[0] + khgb_dx, khgb_k.c[1] + khgb_dy,
                                                    khgb_k.c[2] + khgb_dz } };
                        const auto khgb_it = khgb_grid.find(khgb_n);
                        if (khgb_it == khgb_grid.end()) continue;
                        for (uint32_t khgb_q : khgb_it->second) {
                            if (khgb_q >= khgb_best) continue;
                            const float* khgb_r = &khgb_P[3u * khgb_q];
                            if (fabs(static_cast<double>(khgb_r[0]) - khgb_p[0]) <= khgb_tol &&
                                fabs(static_cast<double>(khgb_r[1]) - khgb_p[1]) <= khgb_tol &&
                                fabs(static_cast<double>(khgb_r[2]) - khgb_p[2]) <= khgb_tol) khgb_best = khgb_q;
                        }
                    }
            if (khgb_best == 0xFFFFFFFFu) {
                khgb_best = static_cast<uint32_t>(khgb_P.size() / 3u);
                khgb_P.insert(khgb_P.end(), khgb_p, khgb_p + 3);
                khgb_grid[khgb_k].push_back(khgb_best);
            }
            khgb_pid[khgb_c] = khgb_best;
        }
    }
    // Triangles: each face fanned from its first corner and wound to its normal as a whole, by the sign of the welded
    // polygon's Newell normal against it. A face, or a fan triangle, no thicker than the coordinates' precision draws
    // nothing and goes. Refused, the face named: a normal within KH_GEN_EDGE_ON of the face's plane (strict; it cannot
    // say which side is the front), a fan triangle turned back against the polygon (a concave or self-crossing
    // polygon, whose fan would draw over its notch), and a fan triangle facing away from, or edge-on to, the normal (a
    // polygon far from planar) - those two allowing the noise below.
    struct KhGbTri {
        uint32_t c[3];   // Corners (KhGenSpec indexing).
        uint32_t f;      // Face.
        float ng[3];     // Unit geometric normal, wound to the face's normal.
        float ang[3];    // Corner angles (radians), the smoothing weights.
        bool  sl;        // A sliver (KH_GEN_SLIVER): its corners keep the face's normal.
    };
    std::vector<KhGbTri> khgb_t;
    for (uint32_t khgb_fi = 0; khgb_fi < static_cast<uint32_t>(khgb_s.faces.size()); ++khgb_fi) {
        const KhGenFace& khgb_f = khgb_s.faces[khgb_fi];
        double khgb_N[3] = { 0.0, 0.0, 0.0 };
        for (uint32_t khgb_c = 0; khgb_c < khgb_f.cn; ++khgb_c) {
            const float* khgb_a = &khgb_P[3u * khgb_pid[khgb_f.c0 + khgb_c]];
            const float* khgb_b = &khgb_P[3u * khgb_pid[khgb_f.c0 + (khgb_c + 1u) % khgb_f.cn]];
            khgb_N[0] += (static_cast<double>(khgb_a[1]) - khgb_b[1]) * (static_cast<double>(khgb_a[2]) + khgb_b[2]);
            khgb_N[1] += (static_cast<double>(khgb_a[2]) - khgb_b[2]) * (static_cast<double>(khgb_a[0]) + khgb_b[0]);
            khgb_N[2] += (static_cast<double>(khgb_a[0]) - khgb_b[0]) * (static_cast<double>(khgb_a[1]) + khgb_b[1]);
        }
        const double khgb_Nl = sqrt(khgb_N[0] * khgb_N[0] + khgb_N[1] * khgb_N[1] + khgb_N[2] * khgb_N[2]);
        // The noise: a corner may sit up to the weld tolerance off where the script meant it. That moves the polygon's
        // N (twice its area) by up to about 2 tol times its perimeter, and a fan triangle's g by about 4 tol times its
        // longest edge. The turned-back and facing refusals allow it, and a face within it whose normal lies in its
        // rounded plane goes rather than being refused; the edge-on test of any face above it is strict. Nothing is
        // dropped for the noise - only for the coordinates' own precision (KH_GEN_THIN_REL, far finer).
        double khgb_per = 0.0;
        for (uint32_t khgb_c = 0; khgb_c < khgb_f.cn; ++khgb_c) {
            const float* khgb_a = &khgb_P[3u * khgb_pid[khgb_f.c0 + khgb_c]];
            const float* khgb_b = &khgb_P[3u * khgb_pid[khgb_f.c0 + (khgb_c + 1u) % khgb_f.cn]];
            double khgb_d2 = 0.0;
            for (int k = 0; k < 3; ++k) {
                const double khgb_e = static_cast<double>(khgb_b[k]) - khgb_a[k];
                khgb_d2 += khgb_e * khgb_e;
            }
            khgb_per += sqrt(khgb_d2);
        }
        const double khgb_nN = 2.0 * khgb_tol * khgb_per;
        // Thinner than the coordinates' own precision (KH_GEN_THIN_REL): no area at all - the face goes. Only there:
        // a thin face is still a face.
        if (!(khgb_Nl > khgb_thin * khgb_per)) continue;
        const double khgb_dN = khgb_N[0] * khgb_f.nrm[0] + khgb_N[1] * khgb_f.nrm[1] + khgb_N[2] * khgb_f.nrm[2];
        if (!(fabs(khgb_dN) >= KH_GEN_EDGE_ON * khgb_Nl)) {
            // Within the noise, the polygon's plane is the rounding's (a sliver of collinear corners): it goes.
            if (!(khgb_Nl > khgb_nN)) continue;
            err = "face " + std::to_string(khgb_f.src) + ": its normal lies in (or within 0.06 degrees of) the face's "
                  "plane, so it cannot say which side is the front";
            return false;
        }
        const bool khgb_flip = khgb_dN < 0.0;
        for (uint32_t khgb_k = 1; khgb_k + 1u < khgb_f.cn; ++khgb_k) {
            KhGbTri khgb_r;
            khgb_r.c[0] = khgb_f.c0;
            khgb_r.c[1] = khgb_f.c0 + khgb_k;
            khgb_r.c[2] = khgb_f.c0 + khgb_k + 1u;
            khgb_r.f = khgb_fi;
            const float* khgb_pa = &khgb_P[3u * khgb_pid[khgb_r.c[0]]];
            const float* khgb_pb = &khgb_P[3u * khgb_pid[khgb_r.c[1]]];
            const float* khgb_pc = &khgb_P[3u * khgb_pid[khgb_r.c[2]]];
            double khgb_e1[3], khgb_e2[3];
            for (int k = 0; k < 3; ++k) {
                khgb_e1[k] = static_cast<double>(khgb_pb[k]) - khgb_pa[k];
                khgb_e2[k] = static_cast<double>(khgb_pc[k]) - khgb_pa[k];
            }
            double khgb_g[3] = { khgb_e1[1] * khgb_e2[2] - khgb_e1[2] * khgb_e2[1],
                                 khgb_e1[2] * khgb_e2[0] - khgb_e1[0] * khgb_e2[2],
                                 khgb_e1[0] * khgb_e2[1] - khgb_e1[1] * khgb_e2[0] };
            double khgb_l2 = 0.0, khgb_m2 = 0.0, khgb_n2 = 0.0;   // The three edges' squared lengths.
            for (int k = 0; k < 3; ++k) {
                khgb_l2 += khgb_e1[k] * khgb_e1[k];
                khgb_m2 += khgb_e2[k] * khgb_e2[k];
                khgb_n2 += (khgb_e2[k] - khgb_e1[k]) * (khgb_e2[k] - khgb_e1[k]);
            }
            const double khgb_ng = 4.0 * khgb_tol * sqrt(fmax(khgb_l2, fmax(khgb_m2, khgb_n2)));   // g's noise.
            if ((khgb_g[0] * khgb_N[0] + khgb_g[1] * khgb_N[1] + khgb_g[2] * khgb_N[2]) / khgb_Nl < -khgb_ng) {
                err = "face " + std::to_string(khgb_f.src) + ": its corners are not a convex polygon in order (fanned "
                      "from the first corner, a triangle turns back) - split it into convex faces or triangles";
                return false;
            }
            const double khgb_gl = sqrt(khgb_g[0] * khgb_g[0] + khgb_g[1] * khgb_g[1] + khgb_g[2] * khgb_g[2]);
            if (khgb_pid[khgb_r.c[0]] == khgb_pid[khgb_r.c[1]] || khgb_pid[khgb_r.c[1]] == khgb_pid[khgb_r.c[2]] ||
                khgb_pid[khgb_r.c[0]] == khgb_pid[khgb_r.c[2]] || !(khgb_gl > 0.0)) continue;
            if (khgb_flip) {
                const uint32_t khgb_sw = khgb_r.c[1];
                khgb_r.c[1] = khgb_r.c[2];
                khgb_r.c[2] = khgb_sw;
                for (int k = 0; k < 3; ++k) khgb_g[k] = -khgb_g[k];
            }
            // Every triangle faces its face's normal, as the face does (a polygon far from planar may not): what
            // meshgen::bake winds by, and the flat corners' and the smoothing's fallback normal.
            const double khgb_sf = khgb_g[0] * khgb_f.nrm[0] + khgb_g[1] * khgb_f.nrm[1] + khgb_g[2] * khgb_f.nrm[2];
            if (!(khgb_sf >= KH_GEN_EDGE_ON * khgb_gl - khgb_ng)) {
                err = "face " + std::to_string(khgb_f.src) + ": a triangle of it turns away from its normal, or lies "
                      "edge-on to it - its corners are not planar enough";
                return false;
            }
            // Facing the normal by no more than the coordinates' precision - collinear corners (a corner on the middle
            // of an edge), or a sliver the rounding turned: it draws nothing and goes. Any thinner triangle facing
            // forward stays.
            if (!(khgb_sf > khgb_thin * sqrt(fmax(khgb_l2, fmax(khgb_m2, khgb_n2))))) continue;
            // KH_GEN_SLIVER: under 64 precision steps tall, a smoothed normal could lean far enough off the triangle
            // for meshgen::bake's float winding test to turn it; the face's normal faces it by construction.
            khgb_r.sl = !(khgb_sf > 64.0 * khgb_thin * sqrt(fmax(khgb_l2, fmax(khgb_m2, khgb_n2))));
            for (int k = 0; k < 3; ++k) khgb_r.ng[k] = static_cast<float>(khgb_g[k] / khgb_gl);
            for (int khgb_j = 0; khgb_j < 3; ++khgb_j) {
                const float* khgb_o = &khgb_P[3u * khgb_pid[khgb_r.c[khgb_j]]];
                const float* khgb_u = &khgb_P[3u * khgb_pid[khgb_r.c[(khgb_j + 1) % 3]]];
                const float* khgb_v = &khgb_P[3u * khgb_pid[khgb_r.c[(khgb_j + 2) % 3]]];
                double khgb_x[3], khgb_y[3];
                for (int k = 0; k < 3; ++k) {
                    khgb_x[k] = static_cast<double>(khgb_u[k]) - khgb_o[k];
                    khgb_y[k] = static_cast<double>(khgb_v[k]) - khgb_o[k];
                }
                const double khgb_cx = khgb_x[1] * khgb_y[2] - khgb_x[2] * khgb_y[1];
                const double khgb_cy = khgb_x[2] * khgb_y[0] - khgb_x[0] * khgb_y[2];
                const double khgb_cz = khgb_x[0] * khgb_y[1] - khgb_x[1] * khgb_y[0];
                const double khgb_sn = sqrt(khgb_cx * khgb_cx + khgb_cy * khgb_cy + khgb_cz * khgb_cz);
                const double khgb_cs = khgb_x[0] * khgb_y[0] + khgb_x[1] * khgb_y[1] + khgb_x[2] * khgb_y[2];
                khgb_r.ang[khgb_j] = static_cast<float>(atan2(khgb_sn, khgb_cs));
            }
            khgb_t.push_back(khgb_r);
        }
    }
    if (khgb_t.empty()) {
        err = "no triangle has any area once corners closer than a few float steps are merged";
        return false;
    }
    // Corner normals. Smooth: over every triangle corner at the same position (CSR by position), the faces within the
    // crease, weighted by their corner angle, summed in one order for every corner there, so equal sets give equal bits
    // (the weld then shares them). A sum that cancels, or leans 90 degrees or more off the corner's own face or
    // triangle, takes the face's normal: meshgen::bake winds by the first corner's normal, and the lighting of a
    // corner facing away from its triangle is wrong anyway.
    const size_t khgb_tn = khgb_t.size();
    std::vector<float> khgb_cn(khgb_tn * 9u);   // Per triangle corner.
    for (size_t khgb_i = 0; khgb_i < khgb_tn; ++khgb_i) {
        const float* khgb_fn = khgb_s.faces[khgb_t[khgb_i].f].nrm;
        for (int khgb_j = 0; khgb_j < 3; ++khgb_j) {
            memcpy(&khgb_cn[khgb_i * 9u + 3u * khgb_j], khgb_fn, 3u * sizeof(float));
        }
    }
    {   // Corners per position, and the work they cost: both the smoothing below (each corner visits every corner at
        // its position) and the vertex-cache order (kh_mco_range re-scores a cached vertex's every triangle on each
        // emit - a flat fan's centre is one vertex too).
        const size_t khgb_pn = khgb_P.size() / 3u;
        std::vector<uint32_t> khgb_off(khgb_pn + 1u, 0u);
        for (const KhGbTri& khgb_r : khgb_t) {
            for (int khgb_j = 0; khgb_j < 3; ++khgb_j) ++khgb_off[khgb_pid[khgb_r.c[khgb_j]] + 1u];
        }
        double khgb_work = 0.0;   // Each corner visits every corner at its position: n^2 a position.
        for (size_t khgb_i = 0; khgb_i < khgb_pn; ++khgb_i) {
            khgb_work += static_cast<double>(khgb_off[khgb_i + 1u]) * khgb_off[khgb_i + 1u];
            khgb_off[khgb_i + 1u] += khgb_off[khgb_i];
        }
        if (khgb_work > KH_GEN_SMOOTH_WORK) {
            err = "too many faces meet at single corners (fans of thousands of triangles around one point) - split "
                  "such a corner into several";
            return false;
        }
        if (!khgb_s.flat) {   // Flat: every corner keeps its face's normal.
            std::vector<uint32_t> khgb_inc(khgb_off[khgb_pn]);   // 3 * triangle + corner.
            {
                std::vector<uint32_t> khgb_fill(khgb_off.begin(), khgb_off.end() - 1);
                for (size_t khgb_i = 0; khgb_i < khgb_tn; ++khgb_i)
                    for (uint32_t khgb_j = 0; khgb_j < 3u; ++khgb_j)
                        khgb_inc[khgb_fill[khgb_pid[khgb_t[khgb_i].c[khgb_j]]]++] =
                            static_cast<uint32_t>(khgb_i * 3u + khgb_j);
            }
            const double khgb_cut = static_cast<double>(khgb_s.crease_cos) - 1.0e-6;   // Exact right angles included.
            for (size_t khgb_i = 0; khgb_i < khgb_tn; ++khgb_i) {
                const KhGbTri& khgb_r = khgb_t[khgb_i];
                const float* khgb_fn = khgb_s.faces[khgb_r.f].nrm;
                for (int khgb_j = 0; khgb_j < 3 && !khgb_r.sl; ++khgb_j) {   // KH_GEN_SLIVER.
                    const uint32_t khgb_p = khgb_pid[khgb_r.c[khgb_j]];
                    double khgb_m[3] = { 0.0, 0.0, 0.0 };
                    for (uint32_t khgb_a = khgb_off[khgb_p]; khgb_a < khgb_off[khgb_p + 1u]; ++khgb_a) {
                        const KhGbTri& khgb_o = khgb_t[khgb_inc[khgb_a] / 3u];
                        const float* khgb_on = khgb_s.faces[khgb_o.f].nrm;
                        const double khgb_d = static_cast<double>(khgb_on[0]) * khgb_fn[0] +
                                              static_cast<double>(khgb_on[1]) * khgb_fn[1] +
                                              static_cast<double>(khgb_on[2]) * khgb_fn[2];
                        if (khgb_d < khgb_cut) continue;
                        const double khgb_w = khgb_o.ang[khgb_inc[khgb_a] % 3u];
                        for (int k = 0; k < 3; ++k) khgb_m[k] += khgb_w * khgb_on[k];
                    }
                    const double khgb_ml = sqrt(khgb_m[0] * khgb_m[0] + khgb_m[1] * khgb_m[1] + khgb_m[2] * khgb_m[2]);
                    if (!(khgb_ml > 1.0e-12)) continue;
                    float khgb_n[3];
                    for (int k = 0; k < 3; ++k) khgb_n[k] = static_cast<float>(khgb_m[k] / khgb_ml);
                    const float khgb_df = khgb_n[0] * khgb_fn[0] + khgb_n[1] * khgb_fn[1] + khgb_n[2] * khgb_fn[2];
                    const float khgb_dg = khgb_n[0] * khgb_r.ng[0] + khgb_n[1] * khgb_r.ng[1] +
                                          khgb_n[2] * khgb_r.ng[2];
                    if (!(khgb_df > 1.0e-3f) || !(khgb_dg > 1.0e-3f)) continue;
                    memcpy(&khgb_cn[khgb_i * 9u + 3u * khgb_j], khgb_n, sizeof(khgb_n));
                }
            }
        }
    }
    // The bounds of what draws: the box projection's frame, and the squash's.
    float khgb_mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, khgb_mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (const KhGbTri& khgb_r : khgb_t) {
        for (int khgb_j = 0; khgb_j < 3; ++khgb_j) {
            const float* khgb_p = &khgb_P[3u * khgb_pid[khgb_r.c[khgb_j]]];
            for (int k = 0; k < 3; ++k) {
                khgb_mn[k] = fminf(khgb_mn[k], khgb_p[k]);
                khgb_mx[k] = fmaxf(khgb_mx[k], khgb_p[k]);
            }
        }
    }
    // The expanded triangle list, submesh by submesh in first-seen order.
    std::vector<uint32_t> khgb_subn(khgb_s.subs.size(), 0u);
    for (const KhGbTri& khgb_r : khgb_t) ++khgb_subn[khgb_s.faces[khgb_r.f].sub];
    for (size_t khgb_sb = 0; khgb_sb < khgb_subn.size(); ++khgb_sb) {
        if (khgb_subn[khgb_sb] != 0u) continue;   // Its material slot would shift every later one's.
        err = "submesh '" + khgb_s.subs[khgb_sb] + "': none of its faces has any area once corners closer than a "
              "few float steps are merged";
        return false;
    }
    d.verts.reserve(khgb_tn * 3u);
    for (uint32_t khgb_sb = 0; khgb_sb < static_cast<uint32_t>(khgb_s.subs.size()); ++khgb_sb) {
        MeshSubmesh khgb_sm;
        khgb_sm.name = khgb_s.subs[khgb_sb];
        khgb_sm.index_start = static_cast<uint32_t>(d.verts.size());   // Expanded units; the weld keeps them.
        khgb_sm.index_count = khgb_subn[khgb_sb] * 3u;
        d.submeshes.push_back(std::move(khgb_sm));
        for (size_t khgb_i = 0; khgb_i < khgb_tn; ++khgb_i) {
            const KhGbTri& khgb_r = khgb_t[khgb_i];
            const KhGenFace& khgb_f = khgb_s.faces[khgb_r.f];
            if (khgb_f.sub != khgb_sb) continue;
            // Box projection: the plane of the face normal's dominant axis (z, then y, then x on a tie), each axis
            // over the bounds; u runs to the viewer's right seen from the front, v down.
            const float khgb_ax = fabsf(khgb_f.nrm[0]), khgb_ay = fabsf(khgb_f.nrm[1]), khgb_az = fabsf(khgb_f.nrm[2]);
            for (int khgb_j = 0; khgb_j < 3; ++khgb_j) {
                MeshVertex khgb_v = {};
                const uint32_t khgb_c = khgb_r.c[khgb_j];
                memcpy(khgb_v.pos, &khgb_P[3u * khgb_pid[khgb_c]], sizeof(khgb_v.pos));
                memcpy(khgb_v.nrm, &khgb_cn[khgb_i * 9u + 3u * khgb_j], sizeof(khgb_v.nrm));
                if (khgb_f.has_uv) {
                    khgb_v.uv[0] = khgb_s.uv[2u * khgb_c];
                    khgb_v.uv[1] = khgb_s.uv[2u * khgb_c + 1u];
                } else {
                    float khgb_q[3];
                    for (int k = 0; k < 3; ++k) {
                        const float khgb_e = khgb_mx[k] - khgb_mn[k];
                        khgb_q[k] = khgb_e > 0.0f ? (khgb_v.pos[k] - khgb_mn[k]) / khgb_e : 0.0f;
                    }
                    if (khgb_az >= khgb_ay && khgb_az >= khgb_ax) {   // Up: +y up the texture; down: from below.
                        khgb_v.uv[0] = khgb_f.nrm[2] >= 0.0f ? khgb_q[0] : 1.0f - khgb_q[0];
                        khgb_v.uv[1] = 1.0f - khgb_q[1];
                    } else if (khgb_ay >= khgb_ax) {   // Forward: the viewer's right is -x; back: +x.
                        khgb_v.uv[0] = khgb_f.nrm[1] >= 0.0f ? 1.0f - khgb_q[0] : khgb_q[0];
                        khgb_v.uv[1] = 1.0f - khgb_q[2];
                    } else {   // Right: the viewer's right is +y; left: -y.
                        khgb_v.uv[0] = khgb_f.nrm[0] >= 0.0f ? khgb_q[1] : 1.0f - khgb_q[1];
                        khgb_v.uv[1] = 1.0f - khgb_q[2];
                    }
                }
                khgb_v.tan[3] = 1.0f;
                d.verts.push_back(khgb_v);
            }
        }
    }
    // Tangents in metres (the importer's KH_IMPORT_SQUASH reason), every corner wound with its normal.
    kh_gen_tangents(d.verts);
    // The squash about the origin: twice the farthest extent per axis, floored as KH_IMPORT_FLAT floors.
    float khgb_ext[3];
    for (int k = 0; k < 3; ++k) {
        khgb_ext[k] = 2.0f * fmaxf(fabsf(khgb_mn[k]), fabsf(khgb_mx[k]));
        if (!(khgb_ext[k] > 1e-4f)) khgb_ext[k] = 1e-4f;
        d.native_size[k] = khgb_ext[k];
    }
    d.native_ctr[0] = 0.0f; d.native_ctr[1] = 0.0f; d.native_ctr[2] = 0.0f;
    d.native_ext[0] = khgb_ext[0]; d.native_ext[1] = khgb_ext[2]; d.native_ext[2] = khgb_ext[1];
    for (MeshVertex& khgb_v : d.verts) {   // KH_IMPORT_SQUASH / KH_IMPORT_NRM, as the importer.
        float khgb_n[3], khgb_tg[3];
        for (int k = 0; k < 3; ++k) {
            khgb_v.pos[k] = khgb_v.pos[k] / khgb_ext[k];
            khgb_n[k] = khgb_v.nrm[k] * khgb_ext[k];
            khgb_tg[k] = khgb_v.tan[k] / khgb_ext[k];
        }
        const float khgb_nl = sqrtf(khgb_n[0] * khgb_n[0] + khgb_n[1] * khgb_n[1] + khgb_n[2] * khgb_n[2]);
        if (khgb_nl > 1.0e-12f) for (int k = 0; k < 3; ++k) khgb_v.nrm[k] = khgb_n[k] / khgb_nl;
        const float khgb_tl = sqrtf(khgb_tg[0] * khgb_tg[0] + khgb_tg[1] * khgb_tg[1] + khgb_tg[2] * khgb_tg[2]);
        if (khgb_tl > 1.0e-12f) for (int k = 0; k < 3; ++k) khgb_v.tan[k] = khgb_tg[k] / khgb_tl;
    }
    meshgen::bake(d.verts, d.native_ext);   // The winding test in the authored frame (see bake).
    kh_mesh_weld(d);   // KH_MESH_INDEXED, then the vertex-cache order.
    kh_mesh_occ_prep(d);   // KH_OCC_BOX.
    char khgb_nm[40];
    snprintf(khgb_nm, sizeof(khgb_nm), "generated:%016llx", static_cast<unsigned long long>(khgb_hash));
    d.name = khgb_nm;
    d.alias = "";
    return true;
}

// A generated mesh, by its name (kh_gen_build's): the release's share-entry erase and the collector's short window.
inline bool kh_mesh_is_generated(const MeshDef& khig_d) { return khig_d.name.compare(0, 10, "generated:") == 0; }

// KH_MESH_GEN_PUB: land a pending publish (the flush's housekeeping, and the injection ahead of its scene sync -
// flush.hpp, inject_meshes.hpp): the count, the imports' deferred cache writes (KH_MESH_CACHE_LATER) and the vertex
// and index buffers. The render thread between our draws, or the park; no device, nothing taken.
inline void kh_mesh_publish_land(ID3D11Device* dev) {
    if (!dev) return;
    if (!g_mesh_publish_pending.exchange(false, std::memory_order_relaxed)) return;
    kh_mesh_publish();
    kh_mesh_cache_later_land();   // KH_MESH_CACHE_LATER: the imports that waited for this publish.
    const std::string khmp_err = ensure_mesh_vbs(dev);
    kh_ensure_ok("mesh publish", khmp_err);   // KH_ENSURE_ONCE: the next ensure_resources sees the same error.
}

// The mesh slot's [shading, faces] form: the registry id of the mesh the spec describes (shared or built and
// appended, its publish pending), or -1 with err. Game thread.
inline int kh_mesh_gen_from_gv(const game_value& khmg_v, std::string& err) {
    if (khmg_v.type_enum() != game_data_type::ARRAY || khmg_v.to_array().size() != 2) {
        err = "a generated mesh is [shading, faces]";
        return -1;
    }
    auto& khmg_a = khmg_v.to_array();
    KhGenSpec khmg_s;
    if (!kh_gen_parse(khmg_a[0], khmg_a[1], khmg_s, err)) return -1;
    const uint64_t khmg_h = kh_gen_hash(khmg_s);
    int khmg_id = -1;
    {
        std::lock_guard<std::mutex> khmg_l(g_fbx_cache_mutex);
        const auto khmg_it = g_mesh_gen_cache.find(khmg_h);
        if (khmg_it != g_mesh_gen_cache.end()) {
            // KH_MESH_STAGED: alive, or appended with its publish still pending - the same mesh either way.
            if (kh_mesh_alive(khmg_it->second) || kh_mesh_def_staged(khmg_it->second)) khmg_id = khmg_it->second;
            else g_mesh_gen_cache.erase(khmg_it);   // KH_MESH_FREE: released since; built again.
        }
    }
    // KH_MESH_FREE: a fresh idle window for the mesh a new user takes - stamped, then read alive again
    // (KH_GC_RECHECK); released meanwhile, it is built again below (the release erased its entry).
    if (khmg_id >= 0 && kh_mesh_take_ref(khmg_id)) return khmg_id;
    MeshDef khmg_d;
    if (!kh_gen_build(khmg_s, khmg_h, khmg_d, err)) return -1;
    khmg_id = kh_mesh_append(std::move(khmg_d));
    if (khmg_id < 0) { err = "mesh registry full"; return -1; }
    kh_mesh_note_ref(khmg_id);   // KH_MESH_FREE: the idle window starts at the build.
    g_mesh_publish_pending.store(true, std::memory_order_relaxed);   // KH_MESH_GEN_PUB: the next injection or flush
                                                                     // lands it.
    {
        std::lock_guard<std::mutex> khmg_l(g_fbx_cache_mutex);
        g_mesh_gen_cache[khmg_h] = khmg_id;
    }
    return khmg_id;
}
}