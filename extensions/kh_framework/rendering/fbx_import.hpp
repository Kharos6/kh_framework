#pragma once

// The feature half (rendering_integration_features.hpp): the FBX importer: tangents, cloth weights, skins and
// fbx_import. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/mesh_cache.hpp (declared here for the uses below).
inline bool mesh_cache_load(uint64_t hash, MeshDef& d);
inline void mesh_note_ref(int id);
inline bool mesh_take_ref(int id);

// Bones per skeleton: every index SkinInf's uint16_t can hold - the
// importer's cap and the cache loader's bound.
static constexpr int32_t SKIN_BONES_MAX = 65536;

static std::unordered_map<std::string, int> g_fbx_cache;   // Lower(resolved path) -> mesh id.
static std::mutex g_fbx_cache_mutex;

namespace mikk {   // Mikktspace callbacks over the expanded triangle list.
    inline std::vector<MeshVertex>& V(const SMikkTSpaceContext* c) {
        return *static_cast<std::vector<MeshVertex>*>(c->m_pUserData);
    }
    inline int get_num_faces(const SMikkTSpaceContext* c) { return static_cast<int>(V(c).size() / 3); }
    inline int get_num_verts(const SMikkTSpaceContext*, const int) { return 3; }
    inline void get_pos(const SMikkTSpaceContext* c, float out[], const int f, const int v) {
        memcpy(out, V(c)[f * 3 + v].pos, 12);
    }
    inline void get_nrm(const SMikkTSpaceContext* c, float out[], const int f, const int v) {
        memcpy(out, V(c)[f * 3 + v].nrm, 12);
    }
    inline void get_uv(const SMikkTSpaceContext* c, float out[], const int f, const int v) {
        memcpy(out, V(c)[f * 3 + v].uv, 8);
    }
    inline void set_tspace(const SMikkTSpaceContext* c, const float t[], const float sign,
                           const int f, const int v) {
        MeshVertex& mv = V(c)[f * 3 + v];
        memcpy(mv.tan, t, 12);
        mv.tan[3] = sign;
    }
}

inline void gen_tangents(std::vector<MeshVertex>& v) {
    SMikkTSpaceInterface itf = {};
    itf.m_getNumFaces = mikk::get_num_faces;
    itf.m_getNumVerticesOfFace = mikk::get_num_verts;
    itf.m_getPosition = mikk::get_pos;
    itf.m_getNormal = mikk::get_nrm;
    itf.m_getTexCoord = mikk::get_uv;
    itf.m_setTSpaceBasic = mikk::set_tspace;
    SMikkTSpaceContext ctx = {};
    ctx.m_pInterface = &itf;
    ctx.m_pUserData = &v;
    genTangSpaceDefault(&ctx);
}

static constexpr size_t FBX_VERT_CEIL = 0xFFFFFFFFull / sizeof(MeshVertex);

// EA_FBX_NO_PARK: append (invisible) and leave the publish + GPU top-up to the render thread, as a generated mesh
// (MESH_GEN_PUB): never under the graphics lock. The next injection lands it ahead of its scene sync, else the
// flush's housekeeping (mesh_publish_land, which reports a buffer failure through ensure_ok "mesh publish").
// Game thread.
inline bool fbx_register(MeshDef&& d, const std::string& path, int& out_id, std::string& err) {
    d.name = path;
    d.name = kh_lower_copy(std::move(d.name));
    d.alias = "";

    const int id = mesh_append(std::move(d));
    if (id < 0) { err = "mesh registry full"; return false; }
    mesh_note_ref(id);   // MESH_FREE: the idle window starts at import, not at 0.
    // Until the publish lands no object naming the id enters the live scene (SCENE_UNPUB, scene_sync), and a
    // changed one keeps drawing its last synced state; the game-thread readers take the staged lookup
    // (MESH_STAGED) or wait for the publish (skin_sync, cloth_sync, the collider gather).
    g_mesh_publish_pending.store(true, std::memory_order_relaxed);
    out_id = id;
    return true;
}

// CLOTH: the two authored weight groups, by name, case-insensitively.
static const char* const CLOTH_GROUP_SIM = "kh_cloth_sim";
static const char* const CLOTH_GROUP_PIN = "kh_cloth_pin";

inline bool cloth_name_is(const char* s, size_t n, const char* want) {
    if (!s) return false;
    size_t i = 0;
    for (; i < n && want[i]; ++i) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
        if (c != want[i]) return false;
    }
    return i == n && want[i] == '\0';
}

// Per-ufbx-VERTEX (not per corner) sim and pin weights for one mesh. Two carriers are read because only one of them
// survives any given export: a Blender vertex group reaches FBX only as a skin cluster, and only when the mesh has an
// armature whose bone shares the group's name - a bare vertex group with no armature is not written to the file at all.
// A colour set of the same name always survives. Whichever is present wins; if both are, the skin cluster does, because
// it is the one the author had to rig deliberately.
inline bool cloth_fbx_read(const ufbx_mesh* m, std::vector<float>& sim,
                           std::vector<float>& pin, bool& has_sim, bool& has_pin) {
    has_sim = false; has_pin = false;
    if (!m) return false;
    const size_t nv = m->num_vertices;
    if (nv == 0) return false;
    sim.assign(nv, 0.0f);
    pin.assign(nv, 0.0f);

    for (size_t di = 0; di < m->skin_deformers.count; ++di) {
        const ufbx_skin_deformer* sk = m->skin_deformers.data[di];
        if (!sk) continue;
        for (size_t ci = 0; ci < sk->clusters.count; ++ci) {
            const ufbx_skin_cluster* cl = sk->clusters.data[ci];
            if (!cl) continue;
            // The vertex group's name is the BONE's name, not the cluster's - the cluster is the binding, the bone is
            // what the group was called. The cluster name is checked too because some exporters put it there instead.
            const char* bn = cl->bone_node ? cl->bone_node->name.data : nullptr;
            const size_t bl = cl->bone_node ? cl->bone_node->name.length : 0;
            const bool is_sim = cloth_name_is(bn, bl, CLOTH_GROUP_SIM) ||
                                cloth_name_is(cl->name.data, cl->name.length, CLOTH_GROUP_SIM);
            const bool is_pin = cloth_name_is(bn, bl, CLOTH_GROUP_PIN) ||
                                cloth_name_is(cl->name.data, cl->name.length, CLOTH_GROUP_PIN);
            if (!is_sim && !is_pin) continue;
            std::vector<float>& dst = is_sim ? sim : pin;
            const size_t n = cl->vertices.count < cl->weights.count
                           ? cl->vertices.count : cl->weights.count;
            for (size_t k = 0; k < n; ++k) {
                const uint32_t v = cl->vertices.data[k];
                if (v >= nv) continue;
                // IMPORT_BOUND: clamped on the double - a float cast out of range is undefined.
                double wd = cl->weights.data[k];
                if (!(wd == wd)) continue;   // A NaN weight is not a weight.
                if (wd < 0.0) wd = 0.0;
                if (wd > 1.0) wd = 1.0;
                const float w = static_cast<float>(wd);
                if (w > dst[v]) dst[v] = w;
            }
            if (is_sim) has_sim = true; else has_pin = true;
        }
    }

    // Colour sets, for the rigless export. The red channel carries the weight - a painted greyscale reads the same in
    // any channel, and red is the one a single-channel paint lands in.
    for (size_t si = 0; si < m->color_sets.count; ++si) {
        const ufbx_color_set& cs = m->color_sets.data[si];
        const bool is_sim = cloth_name_is(cs.name.data, cs.name.length, CLOTH_GROUP_SIM);
        const bool is_pin = cloth_name_is(cs.name.data, cs.name.length, CLOTH_GROUP_PIN);
        if (!is_sim && !is_pin) continue;
        if (is_sim && has_sim) continue;   // The rig already answered this one.
        if (is_pin && has_pin) continue;
        if (!cs.vertex_color.exists) continue;
        std::vector<float>& dst = is_sim ? sim : pin;
        // Colour is indexed per CORNER: each corner folds onto the vertex it refers to.
        for (size_t i = 0; i < m->num_indices; ++i) {
            const uint32_t v = m->vertex_indices.data[i];
            if (v >= nv) continue;
            const ufbx_vec4 c = ufbx_get_vertex_vec4(&cs.vertex_color, i);
            double wd = c.x;   // IMPORT_BOUND: clamped on the double.
            if (!(wd == wd)) continue;
            if (wd < 0.0) wd = 0.0;
            if (wd > 1.0) wd = 1.0;
            const float w = static_cast<float>(wd);
            if (w > dst[v]) dst[v] = w;
        }
        if (is_sim) has_sim = true; else has_pin = true;
    }

    return has_sim || has_pin;
}

// The final weight. A mesh painted only with pins simulates everywhere it was not painted; one painted only with sim
// simulates only where it was. Painted with both, the pin wins in proportion - which is what lets a single falloff
// carry the whole gradient the feature is built around.
inline uint8_t cloth_weight_of(float sim, float pin, bool has_sim) {
    float w = (has_sim ? sim : 1.0f) * (1.0f - pin);
    if (w < 0.0f) w = 0.0f;
    if (w > 1.0f) w = 1.0f;
    return static_cast<uint8_t>(w * 255.0f + 0.5f);
}

// SKEL - the skeleton an import carries, gathered across its mesh nodes: a bone per node that any vertex group (skin
// cluster) names, plus every BONE node above one of those, so a matched bone with no vertices of its own still moves
// its children. The kh_cloth_sim / kh_cloth_pin groups are paint, not bones, and are left out. Heads are bind positions
// in the authored frame (the vertices' frame: ufbx's converted world, Z-up metres), stored in engine axes: from the
// cluster's bind pose where a cluster names the bone, from the node's own transform for an ancestor no cluster names.
struct SkinImport {
    // Pointers into the ufbx scene: valid only until ufbx_free_scene, which fbx_import calls only after
    // skin_finish (and clears these first).
    std::unordered_map<const ufbx_node*, int32_t> ix;
    std::vector<const ufbx_node*> node;
    std::vector<SkinBone> bone;
    std::vector<uint8_t> bind;   // The head came from a cluster's bind pose.
    std::vector<uint8_t> head_bad;   // IMPORT_BOUND: per bone, its head out of bounds - the import is refused.
};
// IMPORT_BOUND: false (and the origin stored) when the head is not finite or is 1e7 m or more from the origin -
// the vertex positions' bound, tested on the double as they are, and again on the float (a double just under 1e7
// can round up onto it, which the mesh cache reader refuses - the box check's case).
inline bool skin_head_of(const ufbx_matrix& m, float h[3]) {
    ufbx_vec3 z;
    z.x = 0.0; z.y = 0.0; z.z = 0.0;
    const ufbx_vec3 p = ufbx_transform_position(&m, z);
    if (!(fabs(p.x) < 1.0e7 && fabs(p.y) < 1.0e7 && fabs(p.z) < 1.0e7)) {
        h[0] = h[1] = h[2] = 0.0f;
        return false;
    }
    h[0] = static_cast<float>(p.x);   // Authored Z-up -> engine (east, up, north),
    h[1] = static_cast<float>(p.z);   // the swap meshgen::bake makes on the vertices.
    h[2] = static_cast<float>(p.y);
    if (!(fabsf(h[0]) < 1.0e7f && fabsf(h[1]) < 1.0e7f && fabsf(h[2]) < 1.0e7f)) {
        h[0] = h[1] = h[2] = 0.0f;
        return false;
    }
    return true;
}
inline int32_t skin_bone_of(SkinImport& s, const ufbx_node* n) {
    const auto it = s.ix.find(n);
    if (it != s.ix.end()) return it->second;
    if (static_cast<int32_t>(s.bone.size()) >= SKIN_BONES_MAX) return -1;
    const int32_t id = static_cast<int32_t>(s.bone.size());
    s.ix.emplace(n, id);
    s.node.push_back(n);
    SkinBone b;
    b.name.assign(n->name.data, n->name.length);
    b.name = kh_lower_copy(std::move(b.name));
    const bool ok = skin_head_of(n->node_to_world, b.head);
    s.bone.push_back(b);
    s.bind.push_back(0u);
    s.head_bad.push_back(ok ? 0u : 1u);   // IMPORT_BOUND (a bind pose may replace it).
    return id;
}
// One node's influences, per CONTROL POINT: the four strongest groups, normalized to sum 1 (an exporter's weights need
// not). Same cluster arrays cloth_fbx_read walks. True if any group on this node named a bone.
inline bool skin_fbx_read(const ufbx_mesh* m, SkinImport& s, std::vector<SkinInf>& cp) {
    cp.clear();
    if (!m || m->num_vertices == 0) return false;
    const size_t nv = m->num_vertices;
    cp.assign(nv, SkinInf());
    bool any = false;
    // A cluster this reader skips: none, no bone, or a cloth group's (by its bone's or its own name).
    auto skip = [](const ufbx_skin_cluster* c) {
        if (!c || !c->bone_node) return true;
        const ufbx_node* n = c->bone_node;
        return cloth_name_is(n->name.data, n->name.length, CLOTH_GROUP_SIM) ||
               cloth_name_is(n->name.data, n->name.length, CLOTH_GROUP_PIN) ||
               cloth_name_is(c->name.data, c->name.length, CLOTH_GROUP_SIM) ||
               cloth_name_is(c->name.data, c->name.length, CLOTH_GROUP_PIN);
    };
    // SKIN_DUP_BONE - a bone that reaches a control point through two or more clusters (a second skin deformer
    // naming it, or a repeated cluster) has its shares summed BEFORE the four strongest are picked: merged where it
    // meets its own slot, a first share below the weakest of four full slots would be dropped and the second compete
    // alone. Only such a bone takes this path (dup: its node -> its index among them); every other influence takes
    // the plain one. tot: (control point, repeated bone) -> the summed share, in cluster order; -1 once taken. A
    // bone whose single cluster names a point twice is such a bone too (stamp: the cluster that last named each
    // point), so the rule does not depend on whether the bone happens to appear in another cluster. Cost, at import,
    // once per file: a 4-byte stamp per control point and one more pass over the influences for every skinned mesh
    // node; and the two maps, transient, about the repeated bones' influence count (a skin deformer duplicated whole:
    // some MB on a large body).
    std::unordered_map<const ufbx_node*, uint32_t> dup;
    std::unordered_map<uint64_t, float> tot;
    {
        std::unordered_set<const ufbx_node*> seen;
        std::vector<uint32_t> stamp(nv, 0u);
        uint32_t sid = 0u;
        for (size_t di = 0; di < m->skin_deformers.count; ++di) {
            const ufbx_skin_deformer* sk = m->skin_deformers.data[di];
            if (!sk) continue;
            for (size_t ci = 0; ci < sk->clusters.count; ++ci) {
                const ufbx_skin_cluster* cl = sk->clusters.data[ci];
                if (skip(cl)) continue;
                bool twice = !seen.insert(cl->bone_node).second;
                ++sid;
                const size_t cn = cl->vertices.count < cl->weights.count
                                ? cl->vertices.count : cl->weights.count;
                for (size_t k = 0; k < cn && !twice; ++k) {
                    const uint32_t v = cl->vertices.data[k];
                    if (v >= nv) continue;
                    const double wd = cl->weights.data[k];
                    if (!(wd > 0.0) || !(wd < 1.0e30)) continue;   // The reader's own test (below).
                    if (stamp[v] == sid) twice = true;
                    stamp[v] = sid;
                }
                if (twice) dup.emplace(cl->bone_node, static_cast<uint32_t>(dup.size()));
            }
        }
        for (size_t di = 0; !dup.empty() && di < m->skin_deformers.count; ++di) {
            const ufbx_skin_deformer* sk = m->skin_deformers.data[di];
            if (!sk) continue;
            for (size_t ci = 0; ci < sk->clusters.count; ++ci) {
                const ufbx_skin_cluster* cl = sk->clusters.data[ci];
                if (skip(cl)) continue;
                const auto dit = dup.find(cl->bone_node);
                if (dit == dup.end()) continue;
                const size_t n = cl->vertices.count < cl->weights.count
                               ? cl->vertices.count : cl->weights.count;
                for (size_t k = 0; k < n; ++k) {
                    const uint32_t v = cl->vertices.data[k];
                    if (v >= nv) continue;
                    const double wd = cl->weights.data[k];
                    if (!(wd > 0.0) || !(wd < 1.0e30)) continue;   // The reader's own test (below).
                    tot[static_cast<uint64_t>(v) * dup.size() + dit->second] +=
                        static_cast<float>(wd);
                }
            }
        }
    }
    for (size_t di = 0; di < m->skin_deformers.count; ++di) {
        const ufbx_skin_deformer* sk = m->skin_deformers.data[di];
        if (!sk) continue;
        for (size_t ci = 0; ci < sk->clusters.count; ++ci) {
            const ufbx_skin_cluster* cl = sk->clusters.data[ci];
            if (skip(cl)) continue;
            const ufbx_node* bn = cl->bone_node;
            const auto rit = dup.find(bn);   // SKIN_DUP_BONE: a repeated bone's cluster.
            const bool rep = rit != dup.end();
            const int32_t b = skin_bone_of(s, bn);
            if (b < 0) continue;
            if (!s.bind[b]) {   // The bind pose is the rest pose the memory points must match.
                s.head_bad[static_cast<size_t>(b)] =   // IMPORT_BOUND: the head this bone keeps.
                    skin_head_of(cl->bind_to_world, s.bone[b].head) ? 0u : 1u;
                s.bind[b] = 1u;
            }
            any = true;
            const size_t n = cl->vertices.count < cl->weights.count
                           ? cl->vertices.count : cl->weights.count;
            for (size_t k = 0; k < n; ++k) {
                const uint32_t v = cl->vertices.data[k];
                if (v >= nv) continue;
                // IMPORT_BOUND: tested on the double - a float cast out of range is undefined.
                const double wd = cl->weights.data[k];
                if (!(wd > 0.0) || !(wd < 1.0e30)) continue;   // NaN, zero, negative, inf.
                float w = static_cast<float>(wd);
                if (rep) {   // SKIN_DUP_BONE: the summed share at the first sighting, nothing after it.
                    const auto ti =
                        tot.find(static_cast<uint64_t>(v) * dup.size() + rit->second);
                    if (ti == tot.end() || !(ti->second > 0.0f)) continue;
                    w = ti->second;
                    ti->second = -1.0f;
                }
                SkinInf& f = cp[v];
                int lo = 0;
                bool done = false;
                for (int q = 0; q < 4; ++q) {
                    if (f.w[q] > 0.0f && f.b[q] == static_cast<uint16_t>(b)) {
                        f.w[q] += w;   // The same bone twice at one point (SKIN_DUP_BONE sums first).
                        done = true;
                        break;
                    }
                    if (f.w[q] < f.w[lo]) lo = q;
                }
                if (!done && w > f.w[lo]) {
                    f.w[lo] = w;
                    f.b[lo] = static_cast<uint16_t>(b);
                }
            }
        }
    }
    for (SkinInf& f : cp) {
        const float t = f.w[0] + f.w[1] + f.w[2] + f.w[3];
        if (!(t > 0.0f)) continue;
        for (int q = 0; q < 4; ++q) {
            f.w[q] /= t;
            if (!(f.w[q] > 0.0f)) { f.w[q] = 0.0f; f.b[q] = 0; }
        }
    }
    return any;
}
// The bone ancestors no cluster names, then every bone's parent.
inline void skin_finish(SkinImport& s) {
    for (size_t i = 0; i < s.node.size(); ++i) {   // Grows as ancestors join.
        for (const ufbx_node* p = s.node[i]->parent; p && !p->is_root; p = p->parent) {
            if (p->bone) { skin_bone_of(s, p); break; }   // Its own pass climbs on.
        }
    }
    for (size_t i = 0; i < s.node.size(); ++i) {
        s.bone[i].parent = -1;
        for (const ufbx_node* p = s.node[i]->parent; p && !p->is_root; p = p->parent) {
            const auto it = s.ix.find(p);
            if (it != s.ix.end()) { s.bone[i].parent = it->second; break; }
        }
    }
}

inline bool fbx_import(const std::string& path, int& out_id, std::string& err) {
    // One read, three consumers: the file bytes feed the content hash, the cache probe and, on a miss,
    // ufbx_load_memory.
    std::vector<uint8_t> bytes;
    if (!asset_read(path, bytes, ~0ull, &err)) return false;   // PBO_PATH.
    const uint64_t hash = CryptoGenerator::fnv1a64_raw(bytes.data(), bytes.size());
    {
        MeshDef mc_d;
        const bool hit = mesh_cache_load(hash, mc_d);
        if (hit) return fbx_register(std::move(mc_d), path, out_id, err);
    }
    ufbx_load_opts opts = {};
    // Arma authoring axes: +X east, +Y north, +Z up, the model FACING +Y (north, the way a dir-0 object faces). ufbx's
    // triple is right / up / front as the front view shows it - screen right, screen up, and the way the model faces,
    // toward that view - so north-facing is -X / +Z / +Y. Do not use ufbx_axes_right_handed_z_up (+X / +Z / -Y,
    // Blender's): it faces every import south. The two differ by 180 degrees about up - a rotation, not a mirror, so
    // winding, tangent handedness and every bone (ufbx converts the node matrices the heads come from along with the
    // geometry) follow as they are.
    opts.target_axes.right = UFBX_COORDINATE_AXIS_NEGATIVE_X;
    opts.target_axes.up    = UFBX_COORDINATE_AXIS_POSITIVE_Z;
    opts.target_axes.front = UFBX_COORDINATE_AXIS_POSITIVE_Y;
    opts.target_unit_meters = 1.0f;
    opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
    opts.generate_missing_normals = true;
    opts.load_external_files = false;   // Geometry only; textures are SQF-side.
    ufbx_error uerr;
    ufbx_scene* scene = ufbx_load_memory(bytes.data(), bytes.size(), &opts, &uerr);

    if (!scene) {
        char buf[512];
        ufbx_format_error(buf, sizeof(buf), &uerr);
        err = buf;
        // EA_FBX_ERR_LINE: ufbx ends its line with '\n' (its error stack, one more line per frame, is not compiled
        // in); the report is one line.
        while (!err.empty() && (err.back() == '\n' || err.back() == '\r')) err.pop_back();
        return false;
    }
    // FBX_SCENE_HOLD: an exception in the walk below (an allocation: the buckets grow with the file) unwinds to the
    // command's catch - the scene, the bulk of the import's memory, is freed on that path too. The normal path frees it
    // itself and disarms the hold first.
    struct FbxSceneHold {
        ufbx_scene* s;
        ~FbxSceneHold() { if (s) ufbx_free_scene(s); }
    } hold{ scene };

    // Buckets keyed by material name, merged across nodes; first-seen order becomes the submesh order.
    std::vector<std::string> bucket_names;
    std::vector<std::vector<MeshVertex>> buckets;
    std::unordered_map<std::string, size_t> bucket_ix;
    // CLOTH: one weight per expanded corner, bucketed exactly like the vertices so the two stay index-aligned
    // through the bucket copy.
    std::vector<std::vector<uint8_t>> bucket_w;
    bool fc_any = false;   // Any node carried cloth authoring at all.
    // SKEL: the skeleton, and one influence set per expanded corner, bucketed like the weights above.
    SkinImport sk_s;
    std::vector<std::vector<SkinInf>> bucket_s;
    bool sk_any = false;
    bool has_uv = false;
    size_t total = 0;
    std::vector<uint32_t> tri_ix;

    for (size_t ni = 0; ni < scene->nodes.count && err.empty(); ++ni) {
        ufbx_node* node = scene->nodes.data[ni];
        if (!node || !node->mesh) continue;
        // FBX_LOD_GROUP: an LOD group node's children are one model at its levels of detail, in order
        // (ufbx_lod_group); merged, every level would draw over the others. Only the first child's subtree - LOD0 - is
        // imported; lod_build makes the coarser levels from it, as for any import.
        bool skip = false;
        for (const ufbx_node* c = node; c->parent && !skip; c = c->parent) {
            const ufbx_node* lg_p = c->parent;
            skip = lg_p->attrib_type == UFBX_ELEMENT_LOD_GROUP && lg_p->children.count > 0 &&
                   lg_p->children.data[0] != c;
        }
        if (skip) continue;
        ufbx_mesh* mesh = node->mesh;
        // CLOTH: resolved once per node, then read per corner below.
        std::vector<float> sim, pin;
        bool has_sim = false, has_pin = false;
        const bool node_cloth = cloth_fbx_read(mesh, sim, pin, has_sim, has_pin);
        if (node_cloth) fc_any = true;
        std::vector<SkinInf> cp;   // SKEL: this node's influences per control point.
        const bool sk_node = skin_fbx_read(mesh, sk_s, cp);
        if (sk_node) sk_any = true;
        const ufbx_matrix g2w = node->geometry_to_world;
        const ufbx_matrix nmat = ufbx_matrix_for_normals(&g2w);
        const bool neg = ufbx_matrix_determinant(&g2w) < 0.0;   // MIKK_MIRROR (below).
        tri_ix.resize(mesh->max_face_triangles * 3);

        for (size_t fi = 0; fi < mesh->faces.count; ++fi) {
            const ufbx_face face = mesh->faces.data[fi];
            const size_t ntri = ufbx_triangulate_face(tri_ix.data(), tri_ix.size(), mesh, face);
            if (!ntri) continue;

            // Material name for this face (per-face index into the mesh's material list; absent list = one "default"
            // bucket).
            const char* mname = "default";

            if (mesh->face_material.count > fi && mesh->materials.count > 0) {
                const uint32_t mi = mesh->face_material.data[fi];
                // FBX_NODE_MAT: a mesh several nodes share may carry other materials on each (3ds Max, Maya
                // instances); ufbx gives a node's own at the same index in node->materials, and mesh->materials
                // is the first node's. ufbx fills a node's list out to the mesh's length from the mesh's, so for
                // a mesh with one node the two name the same materials.
                const ufbx_material* m = mi < node->materials.count && node->materials.data[mi]
                                       ? node->materials.data[mi]
                                       : mi < mesh->materials.count ? mesh->materials.data[mi] : nullptr;
                if (m && m->name.length > 0) {
                    mname = m->name.data;
                }
            }

            auto bit = bucket_ix.find(mname);
            size_t bi;

            if (bit == bucket_ix.end()) {
                bi = buckets.size();
                bucket_ix.emplace(mname, bi);
                bucket_names.push_back(mname);
                buckets.emplace_back();
                bucket_w.emplace_back();
                bucket_s.emplace_back();
            } else bi = bit->second;

            std::vector<MeshVertex>& bv = buckets[bi];
            std::vector<uint8_t>& bw = bucket_w[bi];
            std::vector<SkinInf>& bs = bucket_s[bi];

            for (size_t k = 0; k < ntri * 3; ++k) {
                // MIKK_MIRROR: MikkTSpace (gen_tangents) takes a triangle's handedness from the sign of its uv
                // area in corner order, which is right only for a triangle wound with its normal. A node whose
                // transform mirrors (det < 0) winds its file order against its normals (ufbx_matrix_for_normals keeps
                // them outward), so taken in file order its tangents would get the wrong handedness. The corners are
                // taken 0, 2, 1: meshgen::bake's winding test, taken after its y / z swap, is then exactly negated
                // (corner 0 is the same), so bake swaps them back to file order - the geometry is unchanged and only
                // the handedness differs.
                const uint32_t ix = tri_ix[neg ? k - k % 3 + (3 - k % 3) % 3 : k];
                MeshVertex mv = {};
                const ufbx_vec3 p = ufbx_transform_position(&g2w, ufbx_get_vertex_vec3(&mesh->vertex_position, ix));
                // IMPORT_BOUND: a position that is not finite, or is 1e7 m or more from the origin (the mesh
                // cache reader's bound), is refused - tested on the double, whose float cast out of range is undefined.
                if (!(fabs(p.x) < 1.0e7 && fabs(p.y) < 1.0e7 && fabs(p.z) < 1.0e7)) {
                    err = "mesh has a vertex position that is not finite or is 10,000 km or more from its origin";
                    break;
                }
                mv.pos[0] = static_cast<float>(p.x);
                mv.pos[1] = static_cast<float>(p.y);
                mv.pos[2] = static_cast<float>(p.z);

                // IMPORT_BOUND: a normal past 1e18 (NaN included) takes the default, as a missing one does -
                // the float cast of it (or of its squared length) would be undefined.
                const ufbx_vec3 n = mesh->vertex_normal.exists
                                  ? ufbx_transform_direction(&nmat, ufbx_get_vertex_vec3(&mesh->vertex_normal, ix))
                                  : ufbx_vec3{};
                if (mesh->vertex_normal.exists && fabs(n.x) < 1.0e18 && fabs(n.y) < 1.0e18 && fabs(n.z) < 1.0e18) {
                    const float nl = sqrtf(static_cast<float>(n.x * n.x + n.y * n.y + n.z * n.z));
                    if (nl > 1e-8f) {
                        mv.nrm[0] = static_cast<float>(n.x) / nl;
                        mv.nrm[1] = static_cast<float>(n.y) / nl;
                        mv.nrm[2] = static_cast<float>(n.z) / nl;
                    } else mv.nrm[2] = 1.0f;
                } else mv.nrm[2] = 1.0f;   // generate_missing_normals should prevent this.

                if (mesh->vertex_uv.exists) {
                    const ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, ix);
                    if (fabs(uv.x) < 1.0e18 && fabs(uv.y) < 1.0e18) {   // IMPORT_BOUND: else this corner's 0.
                        mv.uv[0] = static_cast<float>(uv.x);
                        mv.uv[1] = 1.0f - static_cast<float>(uv.y);   // FBX bottom-left -> D3D top-left.
                    }
                    has_uv = true;
                }

                mv.tan[3] = 1.0f;
                {   // CLOTH: 255 (fully simulated) is the value a node with no authoring contributes, so an
                    // unpainted material on a painted model does not silently pin itself.
                    uint8_t w = 255u;
                    if (node_cloth) {
                        const uint32_t v = mesh->vertex_indices.data[ix];
                        w = v < sim.size()
                          ? cloth_weight_of(sim[v], pin[v], has_sim)
                          : 255u;
                    }
                    bw.push_back(w);
                }
                if (sk_any) {
                    // SKEL: the corner's control point's influences (none off an unskinned node). Nothing is kept
                    // before the first skinned node, so an unskinned import pays nothing; the corners a bucket took
                    // before it are padded as none here, and the normalization below bounds its read on the bucket's
                    // length.
                    if (bs.size() < bv.size()) bs.resize(bv.size());
                    const uint32_t v = mesh->vertex_indices.data[ix];
                    bs.push_back(sk_node && v < cp.size() ? cp[v] : SkinInf());
                }
                bv.push_back(mv);
                if (++total > FBX_VERT_CEIL) { err = "mesh exceeds the D3D11 vertex buffer size limit"; break; }
            }

            if (!err.empty()) break;
        }
    }

    // SKEL: the skeleton is finished while the scene still lives - it walks the bone nodes' parents - and its node
    // pointers are dropped with it, so nothing below can reach the freed scene through them.
    if (sk_any) skin_finish(sk_s);
    sk_s.node.clear();
    sk_s.ix.clear();
    hold.s = nullptr;   // FBX_SCENE_HOLD: freed here.
    ufbx_free_scene(scene);
    if (!err.empty()) return false;
    if (std::find(sk_s.head_bad.begin(), sk_s.head_bad.end(), 1u) != sk_s.head_bad.end()) {   // IMPORT_BOUND.
        err = "mesh has a bone whose position is not finite or is 10,000 km or more from its origin";
        return false;
    }
    if (total == 0) { err = "no triangle geometry found"; return false; }

    // AABB over everything -> per-axis normalization into [-0.5, 0.5]^3.
    float mn[3] = { 1e30f, 1e30f, 1e30f }, mx[3] = { -1e30f, -1e30f, -1e30f };

    for (const auto& b : buckets) {
        for (const auto& mv : b) {
            for (int k = 0; k < 3; ++k) {
                if (mv.pos[k] < mn[k]) mn[k] = mv.pos[k];
                if (mv.pos[k] > mx[k]) mx[k] = mv.pos[k];
            }
        }
    }

    MeshDef d;
    float ctr[3], ext[3];

    for (int k = 0; k < 3; ++k) {
        ctr[k] = 0.5f * (mn[k] + mx[k]);
        ext[k] = mx[k] - mn[k];
        // IMPORT_BOUND: the box the mesh cache reader accepts (mesh_cache_load_one), so an import is always
        // read back: the extent can reach 2e7 m, and a float position can round up onto 1e7 m.
        if (!(ext[k] < 1.0e7f)) { err = "mesh is 10,000 km or more across"; return false; }
        if (!(fabsf(ctr[k]) < 1.0e7f)) { err = "mesh is centred 10,000 km or more from its origin"; return false; }
        // IMPORT_FLAT: an axis under 1e-4 m (a flat sheet, a decal) is normalized by 1e-4 - the floor native_size
        // takes - so native_ext equals native_size on every axis and KhVsCore's n / size undoes the stored normal's
        // weights exactly (its own floor only guards a zero axis, NRM_FLOOR). Do not normalize such an axis by 1 (it
        // weights the axis 1e4 times too heavily: a flat mesh's bent normals draw pulled onto its plane's normal) or by
        // its true extent (too lightly: they draw over-tilted); the skin's n / native_ext reads both right. The
        // positions stay centred, within +-0.5. An axis of 1e-4 m or more is untouched.
        if (!(ext[k] > 1e-4f)) ext[k] = 1e-4f;
        d.native_size[k] = ext[k];
    }
    // SKEL: the box the positions are normalized by, in engine axes (the swap meshgen::bake makes below), so p *
    // native_ext + native_ctr is where a stored position was authored.
    d.native_ctr[0] = ctr[0]; d.native_ctr[1] = ctr[2]; d.native_ctr[2] = ctr[1];
    d.native_ext[0] = ext[0]; d.native_ext[1] = ext[2]; d.native_ext[2] = ext[1];
    std::unordered_map<ClothPosKey, SkinInf, ClothPosHash> sk_map;
    if (sk_any) sk_map.reserve(total / 2u + 16u);

    d.verts.reserve(total);
    std::unordered_map<ClothPosKey, uint8_t, ClothPosHash> fc_map;
    if (fc_any) fc_map.reserve(total / 2u + 16u);

    for (size_t bi = 0; bi < buckets.size(); ++bi) {
        MeshSubmesh sm;
        sm.name = bucket_names[bi];
        sm.index_start = static_cast<uint32_t>(d.verts.size());   // Expanded units; weld keeps them.
        sm.index_count = static_cast<uint32_t>(buckets[bi].size());
        d.submeshes.push_back(std::move(sm));

        for (size_t i = 0; i < buckets[bi].size(); ++i) {
            MeshVertex& mv = buckets[bi][i];
            // The vertex stays in the AUTHORED frame (metres, unit normals) through the tangent generation below; the
            // squash into the unit box is one pass after it (IMPORT_SQUASH). The keys the cloth weights and skin
            // influences are resolved by are the squashed position, computed here by the expression that pass uses, so
            // the resolve's un-bake lands on the same floats.
            float sq[3];
            for (int k = 0; k < 3; ++k) sq[k] = (mv.pos[k] - ctr[k]) / ext[k];
            // CLOTH: keyed on the normalized PRE-BAKE position, which is what the resolve below un-bakes back to.
            if (fc_any && i < bucket_w[bi].size()) {
                ClothPosKey fc_k;
                memcpy(fc_k.p, sq, sizeof(fc_k.p));
                const uint8_t w = bucket_w[bi][i];
                auto it = fc_map.find(fc_k);
                // A welded position that two corners disagree about takes the more simulated of the two: a seam must
                // not pin a sheet.
                if (it == fc_map.end()) fc_map.emplace(fc_k, w);
                else if (w > it->second) it->second = w;
            }
            // SKEL: the same key. Two corners at one position from different control points keep ONE set of
            // influences - a split in the mesh at a single point would otherwise tear. A weighted corner takes the
            // position from an unweighted one (an unskinned node's corner there must not pin a skinned vertex to the
            // root); between two weighted ones the first stands, as good as the other.
            if (sk_any && i < bucket_s[bi].size()) {
                ClothPosKey sk_k;
                memcpy(sk_k.p, sq, sizeof(sk_k.p));
                const SkinInf& f = bucket_s[bi][i];
                const auto it = sk_map.find(sk_k);
                if (it == sk_map.end()) sk_map.emplace(sk_k, f);
                else if (!(it->second.w[0] + it->second.w[1] + it->second.w[2] + it->second.w[3] > 0.0f) &&
                         f.w[0] + f.w[1] + f.w[2] + f.w[3] > 0.0f) it->second = f;
            }
            d.verts.push_back(mv);
        }

        buckets[bi].clear();
        buckets[bi].shrink_to_fit();
        bucket_w[bi].clear();
        bucket_w[bi].shrink_to_fit();
        bucket_s[bi].clear();
        bucket_s[bi].shrink_to_fit();
    }

    // Tangents in the AUTHORED frame (metres, unit normals): mikktspace's projection of the uv tangent onto the plane
    // perpendicular to the normal is a Euclidean step, and taken after the per-axis squash it shears the frame (on a 1
    // x 0.23 x 1.5 m cape a sixth of the corners' tangents turned more than 30 degrees in the tangent plane, some
    // reversed). Handedness is the uv orientation and survives any frame; bake negates it for the y / z swap.
    if (has_uv) gen_tangents(d.verts);   // Arma space; bake flips handedness.
    // IMPORT_SQUASH: into the unit box, every attribute by its own rule. Positions by 1 / ext. IMPORT_NRM: the
    // stored normal is the NORMALIZED-SPACE normal, as the builtins' are - KhVsCore draws normalize(n / size), the
    // inverse transpose of this scale, which is right only for the squashed geometry's normal, the authored one scaled
    // BY the extents (normals scale inversely to positions); stored as it came, the authored unit normal leans every
    // draw of a non-cubic import toward its thin axes. Tangents are covariant with positions (KhVsCore multiplies by
    // size): by 1 / ext, w untouched. The same frame for all three; the y / z swap is bake's, and moves them together.
    for (MeshVertex& mv : d.verts) {
        for (int k = 0; k < 3; ++k) mv.pos[k] = (mv.pos[k] - ctr[k]) / ext[k];
        float in_n[3], t[3];
        for (int k = 0; k < 3; ++k) { in_n[k] = mv.nrm[k] * ext[k]; t[k] = mv.tan[k] / ext[k]; }
        const float l = cloth_v3_len(in_n);
        if (l > 1.0e-12f) for (int k = 0; k < 3; ++k) mv.nrm[k] = in_n[k] / l;
        const float tl = cloth_v3_len(t);
        if (tl > 1.0e-12f) for (int k = 0; k < 3; ++k) mv.tan[k] = t[k] / tl;
    }
    meshgen::bake(d.verts, d.native_ext);   // The winding test in the authored frame (see bake).
    // LOD_SKIN: the influences of the points the decimator placed anew (sk_map holds the source points the
    // import keyed; one it did not - a bucket only unskinned nodes filled - comes back here with none).
    std::unordered_map<ClothPosKey, SkinInf, ClothPosHash> lod;
    {
        lod_build(d, sk_any ? &sk_map : nullptr, sk_any ? &lod : nullptr);
    }
    mesh_weld(d);   // MESH_INDEXED: every level welded at once, ranges unchanged.
    // CLOTH: carry the weights onto the welded array. meshgen::bake's only per-vertex act on a position is the y/z
    // swap, and it is its own inverse, so un-baking a final position lands exactly on the key that was stored. A vertex
    // only a decimated LOD level uses finds a twin where it sits on a source position (LOD_SEAM moves vertices onto
    // their neighbours) and stays 0, 'pinned', where the decimator placed it anew - unread either way, because a cloth
    // builds from level 0 and is lod_locked.
    if (fc_any && !fc_map.empty()) {
        d.cloth_w.assign(d.verts.size(), 0u);
        for (size_t i = 0; i < d.verts.size(); ++i) {
            ClothPosKey fc_k;
            fc_k.p[0] = d.verts[i].pos[0];
            fc_k.p[1] = d.verts[i].pos[2];
            fc_k.p[2] = d.verts[i].pos[1];
            const auto it = fc_map.find(fc_k);
            if (it != fc_map.end()) d.cloth_w[i] = it->second;
        }
    }
    // SKEL: the influences onto the welded array, by the same un-bake. A vertex only a decimated level uses takes
    // its twin's where it sits on a source position, else the blend the decimator carried to the point it placed anew
    // (LOD_SKIN, lod): every level skins.
    if (sk_any) {
        d.skin_bones = sk_s.bone;   // Finished above, before the scene was freed.
        d.skin_inf.assign(d.verts.size(), SkinInf());
        for (size_t i = 0; i < d.verts.size(); ++i) {
            ClothPosKey sk_k;
            sk_k.p[0] = d.verts[i].pos[0];
            sk_k.p[1] = d.verts[i].pos[2];
            sk_k.p[2] = d.verts[i].pos[1];
            const auto it = sk_map.find(sk_k);
            if (it != sk_map.end()) {
                d.skin_inf[i] = it->second;
            } else {
                const auto il = lod.find(sk_k);   // LOD_SKIN.
                if (il != lod.end()) d.skin_inf[i] = il->second;
            }
        }
    }
    // Persist the compiled result (post-tangent), then keep the cache under its cap.
    stat_inc(g_stats.fbx_imports);
    // The writer takes g_shpool_trim_mu: an FBX import can land while a shader batch writes .khsc blobs into the same
    // folder, and the trim enumerates and deletes.
    mesh_occ_prep(d);   // OCC_BOX: before the cache write reads it.
    const bool ok = fbx_register(std::move(d), path, out_id, err);
    // Queued only on success, and only once the id is published: the writer reads the mesh by id and drops an
    // unpublished one, so the write waits for the publish and is queued where it lands (MESH_CACHE_LATER). The
    // writer is started here, on the game thread, so that landing (the render thread) only queues. EA_MW_INLINE:
    // no writer thread - the write and the trim run here, on the game thread (the appended def is immutable), never
    // at the landing.
    if (ok) {
        if (mesh_cache_writer_start()) {
            mesh_cache_later(hash, out_id);
        } else if (const MeshDef* mc_d = mesh_def_staged(out_id)) {
            mesh_cache_store(hash, *mc_d);
            std::lock_guard<std::mutex> tl(g_shpool_trim_mu);
            mesh_cache_trim();
        }
    }
    return ok;
}

// The mesh-slot front door for ".fbx" strings: resolve via the rendering asset search, then import-or-reuse (cache by
// resolved path).
inline int fbx_mesh_id(const std::string& request, std::string& err) {
    const std::string resolved = RenderAssetDiscovery::find_asset_file(request);

    if (resolved.empty()) {
        err = "'" + request + "' not found " + RenderAssetDiscovery::searched(request);
        return -1;
    }

    std::string key = resolved;
    key = kh_lower_copy(std::move(key));
    {
        std::lock_guard<std::mutex> g(g_fbx_cache_mutex);
        auto it = g_fbx_cache.find(key);
        if (it != g_fbx_cache.end()) {
            // MESH_STAGED: alive, or appended with its publish still pending (fbx_register) - the same mesh
            // either way; mesh_alive alone reads the second as released and would import the file again on every
            // request until the publish lands.
            if (mesh_alive(it->second) || mesh_def_staged(it->second)) return it->second;
            g_fbx_cache.erase(it);   // MESH_FREE: released since (a null tombstone); import again (disk cache hit).
        }
    }
    int id = -1;
    if (!fbx_import(resolved, id, err)) return -1;
    {
        std::lock_guard<std::mutex> g(g_fbx_cache_mutex);
        g_fbx_cache[key] = id;
    }
    return id;
}
}