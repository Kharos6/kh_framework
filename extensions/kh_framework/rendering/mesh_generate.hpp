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
// The build (gen_build): each corner is welded onto the first position already placed (the lowest id) whose every
// coordinate lies within the tolerance of it - GEN_WELD_REL of the farthest coordinate, about 8 float steps, and
// GEN_WELD_MIN at least - so faces meeting at a corner share it whatever rounding the script's arithmetic left (the
// weld is greedy: a chain of corners each within reach of the next is not one position); a triangle that faces its
// normal by no more than the coordinates' precision (GEN_THIN_REL: collinear corners) is dropped, as is a face that
// thin, or one within the rounding noise whose normal lies in its rounded plane (a face all of whose triangles go draws
// nothing; a submesh all of whose faces go is refused with a report: the material slots after it would move up an
// index); fans of thousands of triangles around single points are refused past GEN_SMOOTH_WORK (the smoothing and
// the vertex-cache order are quadratic in them); each face is wound to its normal as a whole; normals, uvs and the
// mikktspace tangents are made per corner in metres; then the importer's squash into the unit box - about the ORIGIN,
// not the bounds' centre (the box is twice the farthest extent per axis, IMPORT_FLAT's 1e-4 m floor), so the
// object's position is the mesh's origin and it turns and scales about it (the cost: every bounds test takes that box,
// up to twice the geometry's where it sits off its origin, and the occluder proof seeds at the origin, so a closed mesh
// about a point outside it, or on its surface, gets no automatic occluder) - meshgen::bake, the weld (corners identical
// in position, normal, uv and tangent become one vertex: a smooth surface shares its vertices across faces, within each
// region its uvs agree over - the box projection splits it where a face's dominant axis changes; a flat one shares them
// within each face) and the occluder prep. No LOD levels, as the builtins.
// MESH_GEN_SHARE: a spec identical to one a live generated mesh was built from (its FNV-1a 64 over the parsed spec)
// takes that mesh: copies cost no memory and draw in one instanced bucket. g_mesh_gen_cache, under g_fbx_cache_mutex as
// the import cache; mesh_release forgets a released id's entry.
// MESH_GEN_PUB: never under the graphics lock. The mesh is appended and its publish left to the render thread
// (g_mesh_publish_pending, as the importer's): the next injection lands it ahead of its scene sync, else the
// flush (mesh_publish_land). Until then scene_sync holds every object naming it out of the live scene
// (SCENE_UNPUB): a pass drawn earlier in that cycle keeps the object's last synced state (a new one: nothing)
// instead of drawing mesh 0. MESH_GEN_GC: an unreferenced generated mesh is released after MESH_GEN_GC_IDLE_MS
// (asset_gc.hpp), not the imports' two minutes - a rebuilt mesh's predecessors do not pile up. Registry ids are never
// reused (MESH_FREE): every distinct spec built takes one of the registry's 1,048,576 for the process's life, so a
// mesh rebuilt every frame runs it out in hours - animate with a material's vertex stage (KH_USER_VS) or the object's
// transform, and rebuild only when the shape changes. Game thread only, as the importer.
namespace RenderIntegration {

static constexpr uint32_t GEN_TRI_MAX = 262144u;   // Triangles after the fan: 37.7 MB of vertices at most.
static constexpr uint32_t GEN_POLY_MAX = 256u;     // Corners per face.
static constexpr float    GEN_POS_MAX = 5.0e6f;    // |coordinate|: the box stays under IMPORT_BOUND's 1e7 m.
static constexpr float    GEN_UV_MAX = 1.0e7f;     // |u|, |v|.
static constexpr float    GEN_WELD_REL = 1.0e-6f;  // About 8 float steps of the farthest coordinate.
static constexpr float    GEN_WELD_MIN = 1.0e-6f;  // Metres.
static constexpr double   GEN_SMOOTH_WORK = 2.0e8;  // The sum over positions of (corners there)^2, most.
static constexpr double   GEN_THIN_REL = 5.0e-7;     // Of the farthest coordinate: about 4 float steps of the
                                                       // squashed box - the least height a triangle keeps.
static constexpr double   GEN_THIN_MIN = 1.0e-7;     // Metres.
static constexpr double   GEN_EDGE_ON = 1.0e-3;     // The least cos of a face (or fan triangle) to its normal.

static std::unordered_map<uint64_t, int> g_mesh_gen_cache;   // MESH_GEN_SHARE: spec hash -> mesh id.

struct GenFace {
    uint32_t c0 = 0;    // First corner: GenSpec::pos at 3 * c0, uv at 2 * c0.
    uint32_t cn = 0;    // Corner count (3 .. GEN_POLY_MAX).
    uint32_t sub = 0;   // GenSpec::subs index.
    uint32_t src = 0;   // The face's index in the script's array (reports; not hashed).
    float nrm[3] = { 0.0f, 0.0f, 0.0f };   // Unit: as given, or from the corners (Newell).
    bool has_uv = false;
};
struct GenSpec {
    bool  flat = true;
    float crease_cos = 0.0f;   // Smooth: a face at the position blends in when its normal's dot with the corner's
                               // own face's is this or more.
    std::vector<float> pos;    // 3 per corner, authored frame (+0.0f: no negative zero).
    std::vector<float> uv;     // 2 per corner, zero where the face has none.
    std::vector<GenFace> faces;
    std::vector<std::string> subs;
};

// n finite numbers, each under lim in size; -0 folds to +0.
inline bool gen_vec(const game_value& v, size_t n, float* out, float lim) {
    if (v.type_enum() != game_data_type::ARRAY) return false;
    auto& a = v.to_array();
    if (a.size() != n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (a[i].type_enum() != game_data_type::SCALAR) return false;
        const float f = static_cast<float>(a[i]) + 0.0f;
        if (!(fabsf(f) < lim)) return false;   // NaN fails too.
        out[i] = f;
    }
    return true;
}
// An optional slot left empty: nil, or an empty array.
inline bool gen_unset(const game_value& v) {
    return v.is_nil() || (v.type_enum() == game_data_type::ARRAY && v.to_array().size() == 0);
}

inline bool gen_parse(const game_value& sh, const game_value& fv, GenSpec& s, std::string& err) {
    float ang = 0.0f;   // The crease angle: true = 90, false = 0.
    if (!sh.is_nil() && sh.type_enum() == game_data_type::BOOL) {
        ang = static_cast<bool>(sh) ? 90.0f : 0.0f;
    } else if (!sh.is_nil() && sh.type_enum() == game_data_type::SCALAR) {
        ang = static_cast<float>(sh);
        if (ang != ang) {
            err = "shading must be true (smooth), false (flat) or a crease angle in degrees";
            return false;
        }
        ang = ang < 0.0f ? 0.0f : (ang > 180.0f ? 180.0f : ang);   // RANGE RULE: clamped.
    } else {
        err = "shading must be true (smooth), false (flat) or a crease angle in degrees";
        return false;
    }
    // One value per meaning (MESH_GEN_SHARE): false and 0 are both flat, true and 90 both a right angle.
    s.flat = ang <= 0.0f;
    s.crease_cos = s.flat || ang == 90.0f ? 0.0f
                 : (ang >= 180.0f ? -2.0f : cosf(ang * (3.14159265358979f / 180.0f)));
    static const char* const form = "faces must be an array of [corners, normal?, uvs?, submesh?] entries";
    if (fv.type_enum() != game_data_type::ARRAY) { err = form; return false; }
    auto& fa = fv.to_array();
    if (fa.size() == 0) { err = "faces is empty"; return false; }
    std::unordered_map<std::string, uint32_t> sub_ix;
    uint64_t tris = 0;
    auto fail = [&](size_t i, const char* m) {
        err = "face " + std::to_string(i) + ": " + m;
        return false;
    };
    for (size_t i = 0; i < fa.size(); ++i) {
        const game_value& e = fa[i];
        if (e.type_enum() != game_data_type::ARRAY) {
            return fail(i, "must be [corners, normal?, uvs?, submesh?]");
        }
        auto& ea = e.to_array();
        if (ea.size() < 1 || ea.size() > 4) {
            return fail(i, "must be [corners, normal?, uvs?, submesh?] (one to four elements)");
        }
        if (ea[0].type_enum() != game_data_type::ARRAY) {
            return fail(i, "corners must be an array of three or more [x, y, z]");
        }
        auto& ca = ea[0].to_array();
        if (ca.size() < 3 || ca.size() > GEN_POLY_MAX) {
            return fail(i, "corners must be three to 256 [x, y, z] positions");
        }
        GenFace f;
        f.c0 = static_cast<uint32_t>(s.pos.size() / 3u);
        f.cn = static_cast<uint32_t>(ca.size());
        tris += f.cn - 2u;
        if (tris > GEN_TRI_MAX) return fail(i, "the mesh passes 262144 triangles");
        for (size_t c = 0; c < ca.size(); ++c) {
            float p[3];
            if (!gen_vec(ca[c], 3, p, GEN_POS_MAX)) {
                return fail(i, "a corner must be [x, y, z], finite numbers under 5,000 km in size");
            }
            s.pos.insert(s.pos.end(), p, p + 3);
        }
        // Newell's normal of the corners as given: the right-hand rule's, so counter-clockwise faces the viewer.
        double nw[3] = { 0.0, 0.0, 0.0 };
        for (uint32_t c = 0; c < f.cn; ++c) {
            const float* a = &s.pos[3u * (f.c0 + c)];
            const float* b = &s.pos[3u * (f.c0 + (c + 1u) % f.cn)];
            nw[0] += (static_cast<double>(a[1]) - b[1]) * (static_cast<double>(a[2]) + b[2]);
            nw[1] += (static_cast<double>(a[2]) - b[2]) * (static_cast<double>(a[0]) + b[0]);
            nw[2] += (static_cast<double>(a[0]) - b[0]) * (static_cast<double>(a[1]) + b[1]);
        }
        double n[3] = { nw[0], nw[1], nw[2] };
        if (ea.size() > 1 && !gen_unset(ea[1])) {
            float g[3];
            if (!gen_vec(ea[1], 3, g, 3.0e38f)) {
                return fail(i, "normal must be [x, y, z] finite numbers (nil or [] = from the corners'"
                               " order)");
            }
            for (int k = 0; k < 3; ++k) n[k] = g[k];
            if (!(n[0] != 0.0 || n[1] != 0.0 || n[2] != 0.0)) {
                return fail(i, "normal must not be [0, 0, 0] (nil or [] = from the corners' order)");
            }
        }
        const double nl = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        const bool area = nl > 0.0 && nl < 1.0e300;
        if (area) for (int k = 0; k < 3; ++k) f.nrm[k] = static_cast<float>(n[k] / nl);
        s.uv.resize(s.pos.size() / 3u * 2u, 0.0f);
        if (ea.size() > 2 && !gen_unset(ea[2])) {
            const char* const um = "uvs must be one [u, v] per corner, finite numbers (nil or [] = projected)";
            if (ea[2].type_enum() != game_data_type::ARRAY) return fail(i, um);
            auto& ua = ea[2].to_array();
            if (ua.size() != f.cn) return fail(i, um);
            for (uint32_t c = 0; c < f.cn; ++c) {
                if (!gen_vec(ua[c], 2, &s.uv[2u * (f.c0 + c)], GEN_UV_MAX)) {
                    return fail(i, um);
                }
            }
            f.has_uv = true;
        }
        std::string sn = "default";
        if (ea.size() > 3 && !gen_unset(ea[3])) {
            if (ea[3].type_enum() != game_data_type::STRING) {
                return fail(i, "submesh must be a name (nil, [] or \"\" = \"default\")");
            }
            std::string g = static_cast<std::string>(ea[3]);
            if (!g.empty()) sn = std::move(g);
        }
        // Keyed lower-cased, as the material selector matches (apply_material_update): spelled as first seen.
        std::string key = kh_lower_copy(std::string(sn));
        auto it = sub_ix.find(key);
        if (it == sub_ix.end()) {
            it = sub_ix.emplace(std::move(key), static_cast<uint32_t>(s.subs.size())).first;
            s.subs.push_back(sn);
        }
        // Its submesh is registered first, so a submesh whose faces all go is refused (gen_build), never left
        // out with the later ones moving up an index. A face with no area (a corner order that encloses none, and no
        // normal to say where it faces) draws nothing: left out, its corners kept unread.
        if (!area) continue;
        f.sub = it->second;
        f.src = static_cast<uint32_t>(i);
        s.faces.push_back(f);
    }
    if (s.faces.empty()) { err = "no face encloses any area"; return false; }
    return true;
}

// MESH_GEN_SHARE: FNV-1a 64 over every field of the parsed spec, each fed explicitly (GenFace has padding).
// Corners a face left out are hashed too: harmless, the same spec gives the same bytes.
inline uint64_t gen_hash(const GenSpec& s) {
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    auto put = [&](const void* p, size_t n) {
        h = CryptoGenerator::fnv1a64_update(h, static_cast<const char*>(p), n);
    };
    const uint32_t hd[4] = { 0x4B47454Eu, s.flat ? 1u : 0u, static_cast<uint32_t>(s.faces.size()),
                             static_cast<uint32_t>(s.pos.size()) };   // "KGEN".
    put(hd, sizeof(hd));
    put(&s.crease_cos, sizeof(float));
    for (const GenFace& f : s.faces) {
        const uint32_t u[4] = { f.c0, f.cn, f.sub, f.has_uv ? 1u : 0u };
        put(u, sizeof(u));
        put(f.nrm, sizeof(f.nrm));
    }
    if (!s.pos.empty()) put(s.pos.data(), s.pos.size() * sizeof(float));
    if (!s.uv.empty()) put(s.uv.data(), s.uv.size() * sizeof(float));
    for (const std::string& n : s.subs) {
        const uint32_t l = static_cast<uint32_t>(n.size());
        put(&l, sizeof(l));
        if (l) put(n.data(), n.size());
    }
    return h;
}

// The spec into a MeshDef ready to append (gen_parse succeeded). False with err when nothing is left to draw.
inline bool gen_build(const GenSpec& s, uint64_t hash, MeshDef& d, std::string& err) {
    // Positions: each corner a face uses, welded within the tolerance onto the first position near it (a 27-cell
    // search of a grid one tolerance wide; the lowest id among the positions in reach, so the result does not depend on
    // the search's order).
    float gb_far = 0.0f;
    for (const GenFace& gb_f : s.faces) {
        for (uint32_t gb_c = 0; gb_c < gb_f.cn; ++gb_c) {
            const float* p = &s.pos[3u * (gb_f.c0 + gb_c)];
            for (int k = 0; k < 3; ++k) gb_far = fmaxf(gb_far, fabsf(p[k]));
        }
    }
    const double tol = fmax(static_cast<double>(GEN_WELD_MIN),
                            static_cast<double>(GEN_WELD_REL) * gb_far);
    const double thin = fmax(GEN_THIN_MIN, GEN_THIN_REL * gb_far);   // The least height that is area.
    struct GbCell {
        int64_t c[3];
        bool operator==(const GbCell& o) const { return c[0] == o.c[0] && c[1] == o.c[1] && c[2] == o.c[2]; }
    };
    struct GbCellHash {
        size_t operator()(const GbCell& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(k.c, sizeof(k.c)));
        }
    };
    std::unordered_map<GbCell, std::vector<uint32_t>, GbCellHash> grid;
    std::vector<float> P;   // Welded positions, 3 each.
    std::vector<uint32_t> pid(s.pos.size() / 3u, 0xFFFFFFFFu);   // Corner -> welded position.
    grid.reserve(s.pos.size() / 3u + 16u);
    for (const GenFace& gb_f : s.faces) {
        for (uint32_t gb_c = gb_f.c0; gb_c < gb_f.c0 + gb_f.cn; ++gb_c) {
            const float* p = &s.pos[3u * gb_c];
            GbCell gb_k;
            for (int k = 0; k < 3; ++k) {
                gb_k.c[k] = static_cast<int64_t>(floor(static_cast<double>(p[k]) / tol));
            }
            uint32_t best = 0xFFFFFFFFu;
            for (int dz = -1; dz <= 1; ++dz)
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const GbCell n = { { gb_k.c[0] + dx, gb_k.c[1] + dy,
                                             gb_k.c[2] + dz } };
                        const auto it = grid.find(n);
                        if (it == grid.end()) continue;
                        for (uint32_t q : it->second) {
                            if (q >= best) continue;
                            const float* r = &P[3u * q];
                            if (fabs(static_cast<double>(r[0]) - p[0]) <= tol &&
                                fabs(static_cast<double>(r[1]) - p[1]) <= tol &&
                                fabs(static_cast<double>(r[2]) - p[2]) <= tol) best = q;
                        }
                    }
            if (best == 0xFFFFFFFFu) {
                best = static_cast<uint32_t>(P.size() / 3u);
                P.insert(P.end(), p, p + 3);
                grid[gb_k].push_back(best);
            }
            pid[gb_c] = best;
        }
    }
    // Triangles: each face fanned from its first corner and wound to its normal as a whole, by the sign of the welded
    // polygon's Newell normal against it. A face, or a fan triangle, no thicker than the coordinates' precision draws
    // nothing and goes. Refused, the face named: a normal within GEN_EDGE_ON of the face's plane (strict; it cannot
    // say which side is the front), a fan triangle turned back against the polygon (a concave or self-crossing
    // polygon, whose fan would draw over its notch), and a fan triangle facing away from, or edge-on to, the normal (a
    // polygon far from planar) - those two allowing the noise below.
    struct GbTri {
        uint32_t c[3];   // Corners (GenSpec indexing).
        uint32_t f;      // Face.
        float ng[3];     // Unit geometric normal, wound to the face's normal.
        float ang[3];    // Corner angles (radians), the smoothing weights.
        bool  sl;        // A sliver (GEN_SLIVER): its corners keep the face's normal.
    };
    std::vector<GbTri> t;
    for (uint32_t fi = 0; fi < static_cast<uint32_t>(s.faces.size()); ++fi) {
        const GenFace& gb_f = s.faces[fi];
        double N[3] = { 0.0, 0.0, 0.0 };
        for (uint32_t gb_c = 0; gb_c < gb_f.cn; ++gb_c) {
            const float* a = &P[3u * pid[gb_f.c0 + gb_c]];
            const float* b = &P[3u * pid[gb_f.c0 + (gb_c + 1u) % gb_f.cn]];
            N[0] += (static_cast<double>(a[1]) - b[1]) * (static_cast<double>(a[2]) + b[2]);
            N[1] += (static_cast<double>(a[2]) - b[2]) * (static_cast<double>(a[0]) + b[0]);
            N[2] += (static_cast<double>(a[0]) - b[0]) * (static_cast<double>(a[1]) + b[1]);
        }
        const double Nl = sqrt(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]);
        // The noise: a corner may sit up to the weld tolerance off where the script meant it. That moves the polygon's
        // N (twice its area) by up to about 2 tol times its perimeter, and a fan triangle's g by about 4 tol times its
        // longest edge. The turned-back and facing refusals allow it, and a face within it whose normal lies in its
        // rounded plane goes rather than being refused; the edge-on test of any face above it is strict. Nothing is
        // dropped for the noise - only for the coordinates' own precision (GEN_THIN_REL, far finer).
        double per = 0.0;
        for (uint32_t gb_c = 0; gb_c < gb_f.cn; ++gb_c) {
            const float* a = &P[3u * pid[gb_f.c0 + gb_c]];
            const float* b = &P[3u * pid[gb_f.c0 + (gb_c + 1u) % gb_f.cn]];
            double d2 = 0.0;
            for (int k = 0; k < 3; ++k) {
                const double e = static_cast<double>(b[k]) - a[k];
                d2 += e * e;
            }
            per += sqrt(d2);
        }
        const double nN = 2.0 * tol * per;
        // Thinner than the coordinates' own precision (GEN_THIN_REL): no area at all - the face goes. Only there:
        // a thin face is still a face.
        if (!(Nl > thin * per)) continue;
        const double dN = N[0] * gb_f.nrm[0] + N[1] * gb_f.nrm[1] + N[2] * gb_f.nrm[2];
        if (!(fabs(dN) >= GEN_EDGE_ON * Nl)) {
            // Within the noise, the polygon's plane is the rounding's (a sliver of collinear corners): it goes.
            if (!(Nl > nN)) continue;
            err = "face " + std::to_string(gb_f.src) + ": its normal lies in (or within 0.06 degrees of) the face's "
                  "plane, so it cannot say which side is the front";
            return false;
        }
        const bool flip = dN < 0.0;
        for (uint32_t gb_k = 1; gb_k + 1u < gb_f.cn; ++gb_k) {
            GbTri r;
            r.c[0] = gb_f.c0;
            r.c[1] = gb_f.c0 + gb_k;
            r.c[2] = gb_f.c0 + gb_k + 1u;
            r.f = fi;
            const float* pa = &P[3u * pid[r.c[0]]];
            const float* pb = &P[3u * pid[r.c[1]]];
            const float* pc = &P[3u * pid[r.c[2]]];
            double e1[3], e2[3];
            for (int k = 0; k < 3; ++k) {
                e1[k] = static_cast<double>(pb[k]) - pa[k];
                e2[k] = static_cast<double>(pc[k]) - pa[k];
            }
            double g[3] = { e1[1] * e2[2] - e1[2] * e2[1],
                            e1[2] * e2[0] - e1[0] * e2[2],
                            e1[0] * e2[1] - e1[1] * e2[0] };
            double l2 = 0.0, m2 = 0.0, n2 = 0.0;   // The three edges' squared lengths.
            for (int k = 0; k < 3; ++k) {
                l2 += e1[k] * e1[k];
                m2 += e2[k] * e2[k];
                n2 += (e2[k] - e1[k]) * (e2[k] - e1[k]);
            }
            const double gb_ng = 4.0 * tol * sqrt(fmax(l2, fmax(m2, n2)));   // g's noise.
            if ((g[0] * N[0] + g[1] * N[1] + g[2] * N[2]) / Nl < -gb_ng) {
                err = "face " + std::to_string(gb_f.src) + ": its corners are not a convex polygon in order (fanned "
                      "from the first corner, a triangle turns back) - split it into convex faces or triangles";
                return false;
            }
            const double gl = sqrt(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
            if (pid[r.c[0]] == pid[r.c[1]] || pid[r.c[1]] == pid[r.c[2]] ||
                pid[r.c[0]] == pid[r.c[2]] || !(gl > 0.0)) continue;
            if (flip) {
                const uint32_t sw = r.c[1];
                r.c[1] = r.c[2];
                r.c[2] = sw;
                for (int k = 0; k < 3; ++k) g[k] = -g[k];
            }
            // Every triangle faces its face's normal, as the face does (a polygon far from planar may not): what
            // meshgen::bake winds by, and the flat corners' and the smoothing's fallback normal.
            const double sf = g[0] * gb_f.nrm[0] + g[1] * gb_f.nrm[1] + g[2] * gb_f.nrm[2];
            if (!(sf >= GEN_EDGE_ON * gl - gb_ng)) {
                err = "face " + std::to_string(gb_f.src) + ": a triangle of it turns away from its normal, or lies "
                      "edge-on to it - its corners are not planar enough";
                return false;
            }
            // Facing the normal by no more than the coordinates' precision - collinear corners (a corner on the middle
            // of an edge), or a sliver the rounding turned: it draws nothing and goes. Any thinner triangle facing
            // forward stays.
            if (!(sf > thin * sqrt(fmax(l2, fmax(m2, n2))))) continue;
            // GEN_SLIVER: under 64 precision steps tall, a smoothed normal could lean far enough off the triangle
            // for meshgen::bake's float winding test to turn it; the face's normal faces it by construction.
            r.sl = !(sf > 64.0 * thin * sqrt(fmax(l2, fmax(m2, n2))));
            for (int k = 0; k < 3; ++k) r.ng[k] = static_cast<float>(g[k] / gl);
            for (int j = 0; j < 3; ++j) {
                const float* gb_o = &P[3u * pid[r.c[j]]];
                const float* u = &P[3u * pid[r.c[(j + 1) % 3]]];
                const float* v = &P[3u * pid[r.c[(j + 2) % 3]]];
                double x[3], y[3];
                for (int k = 0; k < 3; ++k) {
                    x[k] = static_cast<double>(u[k]) - gb_o[k];
                    y[k] = static_cast<double>(v[k]) - gb_o[k];
                }
                const double cx = x[1] * y[2] - x[2] * y[1];
                const double cy = x[2] * y[0] - x[0] * y[2];
                const double cz = x[0] * y[1] - x[1] * y[0];
                const double sn = sqrt(cx * cx + cy * cy + cz * cz);
                const double cs = x[0] * y[0] + x[1] * y[1] + x[2] * y[2];
                r.ang[j] = static_cast<float>(atan2(sn, cs));
            }
            t.push_back(r);
        }
    }
    if (t.empty()) {
        err = "no triangle has any area once corners closer than a few float steps are merged";
        return false;
    }
    // Corner normals. Smooth: over every triangle corner at the same position (CSR by position), the faces within the
    // crease, weighted by their corner angle, summed in one order for every corner there, so equal sets give equal bits
    // (the weld then shares them). A sum that cancels, or leans 90 degrees or more off the corner's own face or
    // triangle, takes the face's normal: meshgen::bake winds by the first corner's normal, and the lighting of a
    // corner facing away from its triangle is wrong anyway.
    const size_t tn = t.size();
    std::vector<float> cn(tn * 9u);   // Per triangle corner.
    for (size_t i = 0; i < tn; ++i) {
        const float* fn = s.faces[t[i].f].nrm;
        for (int j = 0; j < 3; ++j) {
            memcpy(&cn[i * 9u + 3u * j], fn, 3u * sizeof(float));
        }
    }
    {   // Corners per position, and the work they cost: both the smoothing below (each corner visits every corner at
        // its position) and the vertex-cache order (mco_range re-scores a cached vertex's every triangle on each
        // emit - a flat fan's centre is one vertex too).
        const size_t pn = P.size() / 3u;
        std::vector<uint32_t> off(pn + 1u, 0u);
        for (const GbTri& r : t) {
            for (int j = 0; j < 3; ++j) ++off[pid[r.c[j]] + 1u];
        }
        double work = 0.0;   // Each corner visits every corner at its position: n^2 a position.
        for (size_t i = 0; i < pn; ++i) {
            work += static_cast<double>(off[i + 1u]) * off[i + 1u];
            off[i + 1u] += off[i];
        }
        if (work > GEN_SMOOTH_WORK) {
            err = "too many faces meet at single corners (fans of thousands of triangles around one point) - split "
                  "such a corner into several";
            return false;
        }
        if (!s.flat) {   // Flat: every corner keeps its face's normal.
            std::vector<uint32_t> inc(off[pn]);   // 3 * triangle + corner.
            {
                std::vector<uint32_t> fill(off.begin(), off.end() - 1);
                for (size_t i = 0; i < tn; ++i)
                    for (uint32_t j = 0; j < 3u; ++j)
                        inc[fill[pid[t[i].c[j]]]++] =
                            static_cast<uint32_t>(i * 3u + j);
            }
            const double cut = static_cast<double>(s.crease_cos) - 1.0e-6;   // Exact right angles included.
            for (size_t i = 0; i < tn; ++i) {
                const GbTri& r = t[i];
                const float* fn = s.faces[r.f].nrm;
                for (int j = 0; j < 3 && !r.sl; ++j) {   // GEN_SLIVER.
                    const uint32_t p = pid[r.c[j]];
                    double m[3] = { 0.0, 0.0, 0.0 };
                    for (uint32_t a = off[p]; a < off[p + 1u]; ++a) {
                        const GbTri& gb_o = t[inc[a] / 3u];
                        const float* on = s.faces[gb_o.f].nrm;
                        const double gb_d = static_cast<double>(on[0]) * fn[0] +
                                            static_cast<double>(on[1]) * fn[1] +
                                            static_cast<double>(on[2]) * fn[2];
                        if (gb_d < cut) continue;
                        const double w = gb_o.ang[inc[a] % 3u];
                        for (int k = 0; k < 3; ++k) m[k] += w * on[k];
                    }
                    const double ml = sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
                    if (!(ml > 1.0e-12)) continue;
                    float n[3];
                    for (int k = 0; k < 3; ++k) n[k] = static_cast<float>(m[k] / ml);
                    const float df = n[0] * fn[0] + n[1] * fn[1] + n[2] * fn[2];
                    const float dg = n[0] * r.ng[0] + n[1] * r.ng[1] +
                                     n[2] * r.ng[2];
                    if (!(df > 1.0e-3f) || !(dg > 1.0e-3f)) continue;
                    memcpy(&cn[i * 9u + 3u * j], n, sizeof(n));
                }
            }
        }
    }
    // The bounds of what draws: the box projection's frame, and the squash's.
    float mn[3] = { 1.0e30f, 1.0e30f, 1.0e30f }, mx[3] = { -1.0e30f, -1.0e30f, -1.0e30f };
    for (const GbTri& r : t) {
        for (int j = 0; j < 3; ++j) {
            const float* p = &P[3u * pid[r.c[j]]];
            for (int k = 0; k < 3; ++k) {
                mn[k] = fminf(mn[k], p[k]);
                mx[k] = fmaxf(mx[k], p[k]);
            }
        }
    }
    // The expanded triangle list, submesh by submesh in first-seen order.
    std::vector<uint32_t> subn(s.subs.size(), 0u);
    for (const GbTri& r : t) ++subn[s.faces[r.f].sub];
    for (size_t sb = 0; sb < subn.size(); ++sb) {
        if (subn[sb] != 0u) continue;   // Its material slot would shift every later one's.
        err = "submesh '" + s.subs[sb] + "': none of its faces has any area once corners closer than a "
              "few float steps are merged";
        return false;
    }
    d.verts.reserve(tn * 3u);
    for (uint32_t sb = 0; sb < static_cast<uint32_t>(s.subs.size()); ++sb) {
        MeshSubmesh sm;
        sm.name = s.subs[sb];
        sm.index_start = static_cast<uint32_t>(d.verts.size());   // Expanded units; the weld keeps them.
        sm.index_count = subn[sb] * 3u;
        d.submeshes.push_back(std::move(sm));
        for (size_t i = 0; i < tn; ++i) {
            const GbTri& r = t[i];
            const GenFace& gb_f = s.faces[r.f];
            if (gb_f.sub != sb) continue;
            // Box projection: the plane of the face normal's dominant axis (z, then y, then x on a tie), each axis
            // over the bounds; u runs to the viewer's right seen from the front, v down.
            const float ax = fabsf(gb_f.nrm[0]), ay = fabsf(gb_f.nrm[1]), az = fabsf(gb_f.nrm[2]);
            for (int j = 0; j < 3; ++j) {
                MeshVertex v = {};
                const uint32_t gb_c = r.c[j];
                memcpy(v.pos, &P[3u * pid[gb_c]], sizeof(v.pos));
                memcpy(v.nrm, &cn[i * 9u + 3u * j], sizeof(v.nrm));
                if (gb_f.has_uv) {
                    v.uv[0] = s.uv[2u * gb_c];
                    v.uv[1] = s.uv[2u * gb_c + 1u];
                } else {
                    float q[3];
                    for (int k = 0; k < 3; ++k) {
                        const float e = mx[k] - mn[k];
                        q[k] = e > 0.0f ? (v.pos[k] - mn[k]) / e : 0.0f;
                    }
                    if (az >= ay && az >= ax) {   // Up: +y up the texture; down: from below.
                        v.uv[0] = gb_f.nrm[2] >= 0.0f ? q[0] : 1.0f - q[0];
                        v.uv[1] = 1.0f - q[1];
                    } else if (ay >= ax) {   // Forward: the viewer's right is -x; back: +x.
                        v.uv[0] = gb_f.nrm[1] >= 0.0f ? 1.0f - q[0] : q[0];
                        v.uv[1] = 1.0f - q[2];
                    } else {   // Right: the viewer's right is +y; left: -y.
                        v.uv[0] = gb_f.nrm[0] >= 0.0f ? q[1] : 1.0f - q[1];
                        v.uv[1] = 1.0f - q[2];
                    }
                }
                v.tan[3] = 1.0f;
                d.verts.push_back(v);
            }
        }
    }
    // Tangents in metres (the importer's IMPORT_SQUASH reason), every corner wound with its normal.
    gen_tangents(d.verts);
    // The squash about the origin: twice the farthest extent per axis, floored as IMPORT_FLAT floors.
    float ext[3];
    for (int k = 0; k < 3; ++k) {
        ext[k] = 2.0f * fmaxf(fabsf(mn[k]), fabsf(mx[k]));
        if (!(ext[k] > 1e-4f)) ext[k] = 1e-4f;
        d.native_size[k] = ext[k];
    }
    d.native_ctr[0] = 0.0f; d.native_ctr[1] = 0.0f; d.native_ctr[2] = 0.0f;
    d.native_ext[0] = ext[0]; d.native_ext[1] = ext[2]; d.native_ext[2] = ext[1];
    for (MeshVertex& v : d.verts) {   // IMPORT_SQUASH / IMPORT_NRM, as the importer.
        float n[3], tg[3];
        for (int k = 0; k < 3; ++k) {
            v.pos[k] = v.pos[k] / ext[k];
            n[k] = v.nrm[k] * ext[k];
            tg[k] = v.tan[k] / ext[k];
        }
        const float nl = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (nl > 1.0e-12f) for (int k = 0; k < 3; ++k) v.nrm[k] = n[k] / nl;
        const float tl = sqrtf(tg[0] * tg[0] + tg[1] * tg[1] + tg[2] * tg[2]);
        if (tl > 1.0e-12f) for (int k = 0; k < 3; ++k) v.tan[k] = tg[k] / tl;
    }
    meshgen::bake(d.verts, d.native_ext);   // The winding test in the authored frame (see bake).
    mesh_weld(d);   // MESH_INDEXED, then the vertex-cache order.
    mesh_occ_prep(d);   // OCC_BOX.
    char nm[40];
    snprintf(nm, sizeof(nm), "generated:%016llx", static_cast<unsigned long long>(hash));
    d.name = nm;
    d.alias = "";
    return true;
}

// A generated mesh, by its name (gen_build's): the release's share-entry erase and the collector's short window.
inline bool mesh_is_generated(const MeshDef& d) { return d.name.compare(0, 10, "generated:") == 0; }

// MESH_GEN_PUB: land a pending publish (the flush's housekeeping, and the injection ahead of its scene sync -
// flush.hpp, inject_meshes.hpp): the count, the imports' deferred cache writes (MESH_CACHE_LATER) and the vertex
// and index buffers. The render thread between our draws, or the park; no device, nothing taken.
inline void mesh_publish_land(ID3D11Device* dev) {
    if (!dev) return;
    if (!g_mesh_publish_pending.exchange(false, std::memory_order_relaxed)) return;
    mesh_publish();
    mesh_cache_later_land();   // MESH_CACHE_LATER: the imports that waited for this publish.
    const std::string err = ensure_mesh_vbs(dev);
    // ENSURE_ONCE. The ids stay published: until a top-up succeeds (or the mesh is released) every ensure_resources
    // returns this error and the flush and the injection stand down whole - every object, not only this mesh's.
    ensure_ok("mesh publish", err);
}

// The mesh slot's [shading, faces] form: the registry id of the mesh the spec describes (shared or built and
// appended, its publish pending), or -1 with err. Game thread.
inline int mesh_gen_from_gv(const game_value& v, std::string& err) {
    if (v.type_enum() != game_data_type::ARRAY || v.to_array().size() != 2) {
        err = "a generated mesh is [shading, faces]";
        return -1;
    }
    auto& a = v.to_array();
    GenSpec s;
    if (!gen_parse(a[0], a[1], s, err)) return -1;
    const uint64_t h = gen_hash(s);
    int id = -1;
    {
        std::lock_guard<std::mutex> l(g_fbx_cache_mutex);
        const auto it = g_mesh_gen_cache.find(h);
        if (it != g_mesh_gen_cache.end()) {
            // MESH_STAGED: alive, or appended with its publish still pending - the same mesh either way.
            if (mesh_alive(it->second) || mesh_def_staged(it->second)) id = it->second;
            else g_mesh_gen_cache.erase(it);   // MESH_FREE: released since; built again.
        }
    }
    // MESH_FREE: a fresh idle window for the mesh a new user takes - stamped, then read alive again
    // (GC_RECHECK); released meanwhile, it is built again below (the release erased its entry).
    if (id >= 0 && mesh_take_ref(id)) return id;
    MeshDef d;
    if (!gen_build(s, h, d, err)) return -1;
    id = mesh_append(std::move(d));
    if (id < 0) { err = "mesh registry full"; return -1; }
    mesh_note_ref(id);   // MESH_FREE: the idle window starts at the build.
    g_mesh_publish_pending.store(true, std::memory_order_relaxed);   // MESH_GEN_PUB: the next injection or flush
                                                                     // lands it.
    {
        std::lock_guard<std::mutex> l(g_fbx_cache_mutex);
        g_mesh_gen_cache[h] = id;
    }
    return id;
}
}