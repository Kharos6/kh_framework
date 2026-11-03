#pragma once

// The feature half (rendering_integration_features.hpp): the FBX importer: tangents, cloth weights, skins and
// kh_fbx_import. Never include it on its own.
namespace RenderIntegration {
// Defined in rendering/mesh_cache.hpp (declared here for the uses below).
inline bool kh_mesh_cache_load(uint64_t khmc_hash, MeshDef& khmc_d);
inline void kh_mesh_note_ref(int khnr_id);
inline bool kh_mesh_take_ref(int khtr_id);

// Bones per skeleton: every index KhSkinInf's uint16_t can hold - the
// importer's cap and the cache loader's bound.
static constexpr int32_t KH_SKIN_BONES_MAX = 65536;

static std::unordered_map<std::string, int> g_fbx_cache;   // Lower(resolved path) -> mesh id.
static std::mutex g_fbx_cache_mutex;

namespace khmk {   // Mikktspace callbacks over the expanded triangle list.
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

inline void kh_gen_tangents(std::vector<MeshVertex>& v) {
    SMikkTSpaceInterface itf = {};
    itf.m_getNumFaces = khmk::get_num_faces;
    itf.m_getNumVerticesOfFace = khmk::get_num_verts;
    itf.m_getPosition = khmk::get_pos;
    itf.m_getNormal = khmk::get_nrm;
    itf.m_getTexCoord = khmk::get_uv;
    itf.m_setTSpaceBasic = khmk::set_tspace;
    SMikkTSpaceContext ctx = {};
    ctx.m_pInterface = &itf;
    ctx.m_pUserData = &v;
    genTangSpaceDefault(&ctx);
}

static constexpr size_t KH_FBX_VERT_CEIL = 0xFFFFFFFFull / sizeof(MeshVertex);

// KH_EA_FBX_NO_PARK: append (invisible) and leave the publish + GPU top-up to the render thread, as a generated mesh
// (KH_MESH_GEN_PUB): never under the graphics lock. The next injection lands it ahead of its scene sync, else the
// flush's housekeeping (kh_mesh_publish_land, which reports a buffer failure through kh_ensure_ok "mesh publish").
// Game thread.
inline bool kh_fbx_register(MeshDef&& d, const std::string& path, int& out_id, std::string& err) {
    d.name = path;
    d.name = kh_lower_copy(std::move(d.name));
    d.alias = "";

    const int id = kh_mesh_append(std::move(d));
    if (id < 0) { err = "mesh registry full"; return false; }
    kh_mesh_note_ref(id);   // KH_MESH_FREE: the idle window starts at import, not at 0.
    // Until the publish lands no object naming the id enters the live scene (KH_SCENE_UNPUB, kh_scene_sync), and a
    // changed one keeps drawing its last synced state; the game-thread readers take the staged lookup
    // (KH_MESH_STAGED) or wait for the publish (kh_skin_sync, kh_cloth_sync, the collider gather).
    g_mesh_publish_pending.store(true, std::memory_order_relaxed);
    out_id = id;
    return true;
}

// KH_CLOTH: the two authored weight groups, by name, case-insensitively.
static const char* const KH_CLOTH_GROUP_SIM = "kh_cloth_sim";
static const char* const KH_CLOTH_GROUP_PIN = "kh_cloth_pin";

inline bool kh_cloth_name_is(const char* khcn_s, size_t khcn_n, const char* khcn_want) {
    if (!khcn_s) return false;
    size_t khcn_i = 0;
    for (; khcn_i < khcn_n && khcn_want[khcn_i]; ++khcn_i) {
        char khcn_c = khcn_s[khcn_i];
        if (khcn_c >= 'A' && khcn_c <= 'Z') khcn_c = static_cast<char>(khcn_c - 'A' + 'a');
        if (khcn_c != khcn_want[khcn_i]) return false;
    }
    return khcn_i == khcn_n && khcn_want[khcn_i] == '\0';
}

// Per-ufbx-VERTEX (not per corner) sim and pin weights for one mesh. Two carriers are read because only one of them
// survives any given export: a Blender vertex group reaches FBX only as a skin cluster, and only when the mesh has an
// armature whose bone shares the group's name - a bare vertex group with no armature is not written to the file at all.
// A colour set of the same name always survives. Whichever is present wins; if both are, the skin cluster does, because
// it is the one the author had to rig deliberately.
inline bool kh_cloth_fbx_read(const ufbx_mesh* khcf_m, std::vector<float>& khcf_sim,
                              std::vector<float>& khcf_pin, bool& khcf_has_sim, bool& khcf_has_pin) {
    khcf_has_sim = false; khcf_has_pin = false;
    if (!khcf_m) return false;
    const size_t khcf_nv = khcf_m->num_vertices;
    if (khcf_nv == 0) return false;
    khcf_sim.assign(khcf_nv, 0.0f);
    khcf_pin.assign(khcf_nv, 0.0f);

    for (size_t khcf_di = 0; khcf_di < khcf_m->skin_deformers.count; ++khcf_di) {
        const ufbx_skin_deformer* khcf_sk = khcf_m->skin_deformers.data[khcf_di];
        if (!khcf_sk) continue;
        for (size_t khcf_ci = 0; khcf_ci < khcf_sk->clusters.count; ++khcf_ci) {
            const ufbx_skin_cluster* khcf_cl = khcf_sk->clusters.data[khcf_ci];
            if (!khcf_cl) continue;
            // The vertex group's name is the BONE's name, not the cluster's - the cluster is the binding, the bone is
            // what the group was called. The cluster name is checked too because some exporters put it there instead.
            const char* khcf_bn = khcf_cl->bone_node ? khcf_cl->bone_node->name.data : nullptr;
            const size_t khcf_bl = khcf_cl->bone_node ? khcf_cl->bone_node->name.length : 0;
            const bool khcf_is_sim = kh_cloth_name_is(khcf_bn, khcf_bl, KH_CLOTH_GROUP_SIM) ||
                                     kh_cloth_name_is(khcf_cl->name.data, khcf_cl->name.length, KH_CLOTH_GROUP_SIM);
            const bool khcf_is_pin = kh_cloth_name_is(khcf_bn, khcf_bl, KH_CLOTH_GROUP_PIN) ||
                                     kh_cloth_name_is(khcf_cl->name.data, khcf_cl->name.length, KH_CLOTH_GROUP_PIN);
            if (!khcf_is_sim && !khcf_is_pin) continue;
            std::vector<float>& khcf_dst = khcf_is_sim ? khcf_sim : khcf_pin;
            const size_t khcf_n = khcf_cl->vertices.count < khcf_cl->weights.count
                                ? khcf_cl->vertices.count : khcf_cl->weights.count;
            for (size_t khcf_k = 0; khcf_k < khcf_n; ++khcf_k) {
                const uint32_t khcf_v = khcf_cl->vertices.data[khcf_k];
                if (khcf_v >= khcf_nv) continue;
                // KH_IMPORT_BOUND: clamped on the double - a float cast out of range is undefined.
                double khcf_wd = khcf_cl->weights.data[khcf_k];
                if (!(khcf_wd == khcf_wd)) continue;   // A NaN weight is not a weight.
                if (khcf_wd < 0.0) khcf_wd = 0.0;
                if (khcf_wd > 1.0) khcf_wd = 1.0;
                const float khcf_w = static_cast<float>(khcf_wd);
                if (khcf_w > khcf_dst[khcf_v]) khcf_dst[khcf_v] = khcf_w;
            }
            if (khcf_is_sim) khcf_has_sim = true; else khcf_has_pin = true;
        }
    }

    // Colour sets, for the rigless export. The red channel carries the weight - a painted greyscale reads the same in
    // any channel, and red is the one a single-channel paint lands in.
    for (size_t khcf_si = 0; khcf_si < khcf_m->color_sets.count; ++khcf_si) {
        const ufbx_color_set& khcf_cs = khcf_m->color_sets.data[khcf_si];
        const bool khcf_is_sim = kh_cloth_name_is(khcf_cs.name.data, khcf_cs.name.length, KH_CLOTH_GROUP_SIM);
        const bool khcf_is_pin = kh_cloth_name_is(khcf_cs.name.data, khcf_cs.name.length, KH_CLOTH_GROUP_PIN);
        if (!khcf_is_sim && !khcf_is_pin) continue;
        if (khcf_is_sim && khcf_has_sim) continue;   // The rig already answered this one.
        if (khcf_is_pin && khcf_has_pin) continue;
        if (!khcf_cs.vertex_color.exists) continue;
        std::vector<float>& khcf_dst = khcf_is_sim ? khcf_sim : khcf_pin;
        // Colour is indexed per CORNER: each corner folds onto the vertex it refers to.
        for (size_t khcf_i = 0; khcf_i < khcf_m->num_indices; ++khcf_i) {
            const uint32_t khcf_v = khcf_m->vertex_indices.data[khcf_i];
            if (khcf_v >= khcf_nv) continue;
            const ufbx_vec4 khcf_c = ufbx_get_vertex_vec4(&khcf_cs.vertex_color, khcf_i);
            double khcf_wd = khcf_c.x;   // KH_IMPORT_BOUND: clamped on the double.
            if (!(khcf_wd == khcf_wd)) continue;
            if (khcf_wd < 0.0) khcf_wd = 0.0;
            if (khcf_wd > 1.0) khcf_wd = 1.0;
            const float khcf_w = static_cast<float>(khcf_wd);
            if (khcf_w > khcf_dst[khcf_v]) khcf_dst[khcf_v] = khcf_w;
        }
        if (khcf_is_sim) khcf_has_sim = true; else khcf_has_pin = true;
    }

    return khcf_has_sim || khcf_has_pin;
}

// The final weight. A mesh painted only with pins simulates everywhere it was not painted; one painted only with sim
// simulates only where it was. Painted with both, the pin wins in proportion - which is what lets a single falloff
// carry the whole gradient the feature is built around.
inline uint8_t kh_cloth_weight_of(float khcw_sim, float khcw_pin, bool khcw_has_sim) {
    float khcw_w = (khcw_has_sim ? khcw_sim : 1.0f) * (1.0f - khcw_pin);
    if (khcw_w < 0.0f) khcw_w = 0.0f;
    if (khcw_w > 1.0f) khcw_w = 1.0f;
    return static_cast<uint8_t>(khcw_w * 255.0f + 0.5f);
}

// KH_SKEL - the skeleton an import carries, gathered across its mesh nodes: a bone per node that any vertex group (skin
// cluster) names, plus every BONE node above one of those, so a matched bone with no vertices of its own still moves
// its children. The kh_cloth_sim / kh_cloth_pin groups are paint, not bones, and are left out. Heads are bind positions
// in the authored frame (the vertices' frame: ufbx's converted world, Z-up metres), stored in engine axes: from the
// cluster's bind pose where a cluster names the bone, from the node's own transform for an ancestor no cluster names.
struct KhSkinImport {
    // Pointers into the ufbx scene: valid only until ufbx_free_scene, which kh_fbx_import calls only after
    // kh_skin_finish (and clears these first).
    std::unordered_map<const ufbx_node*, int32_t> ix;
    std::vector<const ufbx_node*> node;
    std::vector<KhSkinBone> bone;
    std::vector<uint8_t> bind;   // The head came from a cluster's bind pose.
    std::vector<uint8_t> head_bad;   // KH_IMPORT_BOUND: per bone, its head out of bounds - the import is refused.
};
// KH_IMPORT_BOUND: false (and the origin stored) when the head is not finite or is 1e7 m or more from the origin -
// the vertex positions' bound, tested on the double as they are, and again on the float (a double just under 1e7
// can round up onto it, which the mesh cache reader refuses - the box check's case).
inline bool kh_skin_head_of(const ufbx_matrix& khsh_m, float khsh_h[3]) {
    ufbx_vec3 khsh_z;
    khsh_z.x = 0.0; khsh_z.y = 0.0; khsh_z.z = 0.0;
    const ufbx_vec3 khsh_p = ufbx_transform_position(&khsh_m, khsh_z);
    if (!(fabs(khsh_p.x) < 1.0e7 && fabs(khsh_p.y) < 1.0e7 && fabs(khsh_p.z) < 1.0e7)) {
        khsh_h[0] = khsh_h[1] = khsh_h[2] = 0.0f;
        return false;
    }
    khsh_h[0] = static_cast<float>(khsh_p.x);   // Authored Z-up -> engine (east, up, north),
    khsh_h[1] = static_cast<float>(khsh_p.z);   // the swap meshgen::bake makes on the vertices.
    khsh_h[2] = static_cast<float>(khsh_p.y);
    if (!(fabsf(khsh_h[0]) < 1.0e7f && fabsf(khsh_h[1]) < 1.0e7f && fabsf(khsh_h[2]) < 1.0e7f)) {
        khsh_h[0] = khsh_h[1] = khsh_h[2] = 0.0f;
        return false;
    }
    return true;
}
inline int32_t kh_skin_bone_of(KhSkinImport& khsb_s, const ufbx_node* khsb_n) {
    const auto khsb_it = khsb_s.ix.find(khsb_n);
    if (khsb_it != khsb_s.ix.end()) return khsb_it->second;
    if (static_cast<int32_t>(khsb_s.bone.size()) >= KH_SKIN_BONES_MAX) return -1;
    const int32_t khsb_id = static_cast<int32_t>(khsb_s.bone.size());
    khsb_s.ix.emplace(khsb_n, khsb_id);
    khsb_s.node.push_back(khsb_n);
    KhSkinBone khsb_b;
    khsb_b.name.assign(khsb_n->name.data, khsb_n->name.length);
    khsb_b.name = kh_lower_copy(std::move(khsb_b.name));
    const bool khsb_ok = kh_skin_head_of(khsb_n->node_to_world, khsb_b.head);
    khsb_s.bone.push_back(khsb_b);
    khsb_s.bind.push_back(0u);
    khsb_s.head_bad.push_back(khsb_ok ? 0u : 1u);   // KH_IMPORT_BOUND (a bind pose may replace it).
    return khsb_id;
}
// One node's influences, per CONTROL POINT: the four strongest groups, normalized to sum 1 (an exporter's weights need
// not). Same cluster arrays kh_cloth_fbx_read walks. True if any group on this node named a bone.
inline bool kh_skin_fbx_read(const ufbx_mesh* khsf_m, KhSkinImport& khsf_s, std::vector<KhSkinInf>& khsf_cp) {
    khsf_cp.clear();
    if (!khsf_m || khsf_m->num_vertices == 0) return false;
    const size_t khsf_nv = khsf_m->num_vertices;
    khsf_cp.assign(khsf_nv, KhSkinInf());
    bool khsf_any = false;
    // A cluster this reader skips: none, no bone, or a cloth group's (by its bone's or its own name).
    auto khsf_skip = [](const ufbx_skin_cluster* khsf_c) {
        if (!khsf_c || !khsf_c->bone_node) return true;
        const ufbx_node* khsf_n = khsf_c->bone_node;
        return kh_cloth_name_is(khsf_n->name.data, khsf_n->name.length, KH_CLOTH_GROUP_SIM) ||
               kh_cloth_name_is(khsf_n->name.data, khsf_n->name.length, KH_CLOTH_GROUP_PIN) ||
               kh_cloth_name_is(khsf_c->name.data, khsf_c->name.length, KH_CLOTH_GROUP_SIM) ||
               kh_cloth_name_is(khsf_c->name.data, khsf_c->name.length, KH_CLOTH_GROUP_PIN);
    };
    // KH_SKIN_DUP_BONE - a bone that reaches a control point through two or more clusters (a second skin deformer
    // naming it, or a repeated cluster) has its shares summed BEFORE the four strongest are picked: merged where it
    // meets its own slot, a first share below the weakest of four full slots would be dropped and the second compete
    // alone. Only such a bone takes this path (khsf_dup: its node -> its index among them); every other influence takes
    // the plain one. khsf_tot: (control point, repeated bone) -> the summed share, in cluster order; -1 once taken. A
    // bone whose single cluster names a point twice is such a bone too (khsf_stamp: the cluster that last named each
    // point), so the rule does not depend on whether the bone happens to appear in another cluster. Cost, at import,
    // once per file: a 4-byte stamp per control point and one more pass over the influences for every skinned mesh
    // node; and the two maps, transient, about the repeated bones' influence count (a skin deformer duplicated whole:
    // some MB on a large body).
    std::unordered_map<const ufbx_node*, uint32_t> khsf_dup;
    std::unordered_map<uint64_t, float> khsf_tot;
    {
        std::unordered_set<const ufbx_node*> khsf_seen;
        std::vector<uint32_t> khsf_stamp(khsf_nv, 0u);
        uint32_t khsf_sid = 0u;
        for (size_t khsf_di = 0; khsf_di < khsf_m->skin_deformers.count; ++khsf_di) {
            const ufbx_skin_deformer* khsf_sk = khsf_m->skin_deformers.data[khsf_di];
            if (!khsf_sk) continue;
            for (size_t khsf_ci = 0; khsf_ci < khsf_sk->clusters.count; ++khsf_ci) {
                const ufbx_skin_cluster* khsf_cl = khsf_sk->clusters.data[khsf_ci];
                if (khsf_skip(khsf_cl)) continue;
                bool khsf_twice = !khsf_seen.insert(khsf_cl->bone_node).second;
                ++khsf_sid;
                const size_t khsf_cn = khsf_cl->vertices.count < khsf_cl->weights.count
                                     ? khsf_cl->vertices.count : khsf_cl->weights.count;
                for (size_t khsf_k = 0; khsf_k < khsf_cn && !khsf_twice; ++khsf_k) {
                    const uint32_t khsf_v = khsf_cl->vertices.data[khsf_k];
                    if (khsf_v >= khsf_nv) continue;
                    const double khsf_wd = khsf_cl->weights.data[khsf_k];
                    if (!(khsf_wd > 0.0) || !(khsf_wd < 1.0e30)) continue;   // The reader's own test (below).
                    if (khsf_stamp[khsf_v] == khsf_sid) khsf_twice = true;
                    khsf_stamp[khsf_v] = khsf_sid;
                }
                if (khsf_twice) khsf_dup.emplace(khsf_cl->bone_node, static_cast<uint32_t>(khsf_dup.size()));
            }
        }
        for (size_t khsf_di = 0; !khsf_dup.empty() && khsf_di < khsf_m->skin_deformers.count; ++khsf_di) {
            const ufbx_skin_deformer* khsf_sk = khsf_m->skin_deformers.data[khsf_di];
            if (!khsf_sk) continue;
            for (size_t khsf_ci = 0; khsf_ci < khsf_sk->clusters.count; ++khsf_ci) {
                const ufbx_skin_cluster* khsf_cl = khsf_sk->clusters.data[khsf_ci];
                if (khsf_skip(khsf_cl)) continue;
                const auto khsf_dit = khsf_dup.find(khsf_cl->bone_node);
                if (khsf_dit == khsf_dup.end()) continue;
                const size_t khsf_n = khsf_cl->vertices.count < khsf_cl->weights.count
                                    ? khsf_cl->vertices.count : khsf_cl->weights.count;
                for (size_t khsf_k = 0; khsf_k < khsf_n; ++khsf_k) {
                    const uint32_t khsf_v = khsf_cl->vertices.data[khsf_k];
                    if (khsf_v >= khsf_nv) continue;
                    const double khsf_wd = khsf_cl->weights.data[khsf_k];
                    if (!(khsf_wd > 0.0) || !(khsf_wd < 1.0e30)) continue;   // The reader's own test (below).
                    khsf_tot[static_cast<uint64_t>(khsf_v) * khsf_dup.size() + khsf_dit->second] +=
                        static_cast<float>(khsf_wd);
                }
            }
        }
    }
    for (size_t khsf_di = 0; khsf_di < khsf_m->skin_deformers.count; ++khsf_di) {
        const ufbx_skin_deformer* khsf_sk = khsf_m->skin_deformers.data[khsf_di];
        if (!khsf_sk) continue;
        for (size_t khsf_ci = 0; khsf_ci < khsf_sk->clusters.count; ++khsf_ci) {
            const ufbx_skin_cluster* khsf_cl = khsf_sk->clusters.data[khsf_ci];
            if (khsf_skip(khsf_cl)) continue;
            const ufbx_node* khsf_bn = khsf_cl->bone_node;
            const auto khsf_rit = khsf_dup.find(khsf_bn);   // KH_SKIN_DUP_BONE: a repeated bone's cluster.
            const bool khsf_rep = khsf_rit != khsf_dup.end();
            const int32_t khsf_b = kh_skin_bone_of(khsf_s, khsf_bn);
            if (khsf_b < 0) continue;
            if (!khsf_s.bind[khsf_b]) {   // The bind pose is the rest pose the memory points must match.
                khsf_s.head_bad[static_cast<size_t>(khsf_b)] =   // KH_IMPORT_BOUND: the head this bone keeps.
                    kh_skin_head_of(khsf_cl->bind_to_world, khsf_s.bone[khsf_b].head) ? 0u : 1u;
                khsf_s.bind[khsf_b] = 1u;
            }
            khsf_any = true;
            const size_t khsf_n = khsf_cl->vertices.count < khsf_cl->weights.count
                                ? khsf_cl->vertices.count : khsf_cl->weights.count;
            for (size_t khsf_k = 0; khsf_k < khsf_n; ++khsf_k) {
                const uint32_t khsf_v = khsf_cl->vertices.data[khsf_k];
                if (khsf_v >= khsf_nv) continue;
                // KH_IMPORT_BOUND: tested on the double - a float cast out of range is undefined.
                const double khsf_wd = khsf_cl->weights.data[khsf_k];
                if (!(khsf_wd > 0.0) || !(khsf_wd < 1.0e30)) continue;   // NaN, zero, negative, inf.
                float khsf_w = static_cast<float>(khsf_wd);
                if (khsf_rep) {   // KH_SKIN_DUP_BONE: the summed share at the first sighting, nothing after it.
                    const auto khsf_ti =
                        khsf_tot.find(static_cast<uint64_t>(khsf_v) * khsf_dup.size() + khsf_rit->second);
                    if (khsf_ti == khsf_tot.end() || !(khsf_ti->second > 0.0f)) continue;
                    khsf_w = khsf_ti->second;
                    khsf_ti->second = -1.0f;
                }
                KhSkinInf& khsf_f = khsf_cp[khsf_v];
                int khsf_lo = 0;
                bool khsf_done = false;
                for (int q = 0; q < 4; ++q) {
                    if (khsf_f.w[q] > 0.0f && khsf_f.b[q] == static_cast<uint16_t>(khsf_b)) {
                        khsf_f.w[q] += khsf_w;   // The same bone twice at one point (KH_SKIN_DUP_BONE sums first).
                        khsf_done = true;
                        break;
                    }
                    if (khsf_f.w[q] < khsf_f.w[khsf_lo]) khsf_lo = q;
                }
                if (!khsf_done && khsf_w > khsf_f.w[khsf_lo]) {
                    khsf_f.w[khsf_lo] = khsf_w;
                    khsf_f.b[khsf_lo] = static_cast<uint16_t>(khsf_b);
                }
            }
        }
    }
    for (KhSkinInf& khsf_f : khsf_cp) {
        const float khsf_t = khsf_f.w[0] + khsf_f.w[1] + khsf_f.w[2] + khsf_f.w[3];
        if (!(khsf_t > 0.0f)) continue;
        for (int q = 0; q < 4; ++q) {
            khsf_f.w[q] /= khsf_t;
            if (!(khsf_f.w[q] > 0.0f)) { khsf_f.w[q] = 0.0f; khsf_f.b[q] = 0; }
        }
    }
    return khsf_any;
}
// The bone ancestors no cluster names, then every bone's parent.
inline void kh_skin_finish(KhSkinImport& khsn_s) {
    for (size_t khsn_i = 0; khsn_i < khsn_s.node.size(); ++khsn_i) {   // Grows as ancestors join.
        for (const ufbx_node* khsn_p = khsn_s.node[khsn_i]->parent; khsn_p && !khsn_p->is_root; khsn_p = khsn_p->parent) {
            if (khsn_p->bone) { kh_skin_bone_of(khsn_s, khsn_p); break; }   // Its own pass climbs on.
        }
    }
    for (size_t khsn_i = 0; khsn_i < khsn_s.node.size(); ++khsn_i) {
        khsn_s.bone[khsn_i].parent = -1;
        for (const ufbx_node* khsn_p = khsn_s.node[khsn_i]->parent; khsn_p && !khsn_p->is_root; khsn_p = khsn_p->parent) {
            const auto khsn_it = khsn_s.ix.find(khsn_p);
            if (khsn_it != khsn_s.ix.end()) { khsn_s.bone[khsn_i].parent = khsn_it->second; break; }
        }
    }
}

inline bool kh_fbx_import(const std::string& path, int& out_id, std::string& err) {
    // One read, three consumers: the file bytes feed the content hash, the cache probe and, on a miss,
    // ufbx_load_memory.
    std::vector<uint8_t> khmc_bytes;
    if (!kh_asset_read(path, khmc_bytes, ~0ull, &err)) return false;   // PBO_PATH.
    const uint64_t khmc_hash = CryptoGenerator::fnv1a64_raw(khmc_bytes.data(), khmc_bytes.size());
    {
        MeshDef khmc_d;
        const bool khmc_hit = kh_mesh_cache_load(khmc_hash, khmc_d);
        if (khmc_hit) return kh_fbx_register(std::move(khmc_d), path, out_id, err);
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
    ufbx_scene* scene = ufbx_load_memory(khmc_bytes.data(), khmc_bytes.size(), &opts, &uerr);

    if (!scene) {
        char buf[512];
        ufbx_format_error(buf, sizeof(buf), &uerr);
        err = buf;
        // KH_EA_FBX_ERR_LINE: ufbx ends its line with '\n' (its error stack, one more line per frame, is not compiled
        // in); the report is one line.
        while (!err.empty() && (err.back() == '\n' || err.back() == '\r')) err.pop_back();
        return false;
    }
    // KH_FBX_SCENE_HOLD: an exception in the walk below (an allocation: the buckets grow with the file) unwinds to the
    // command's catch - the scene, the bulk of the import's memory, is freed on that path too. The normal path frees it
    // itself and disarms the hold first.
    struct KhFbxSceneHold {
        ufbx_scene* khfh_s;
        ~KhFbxSceneHold() { if (khfh_s) ufbx_free_scene(khfh_s); }
    } khfh_hold{ scene };

    // Buckets keyed by material name, merged across nodes; first-seen order becomes the submesh order.
    std::vector<std::string> bucket_names;
    std::vector<std::vector<MeshVertex>> buckets;
    std::unordered_map<std::string, size_t> bucket_ix;
    // KH_CLOTH: one weight per expanded corner, bucketed exactly like the vertices so the two stay index-aligned
    // through the bucket copy.
    std::vector<std::vector<uint8_t>> bucket_w;
    bool khfc_any = false;   // Any node carried cloth authoring at all.
    // KH_SKEL: the skeleton, and one influence set per expanded corner, bucketed like the weights above.
    KhSkinImport khsk_s;
    std::vector<std::vector<KhSkinInf>> bucket_s;
    bool khsk_any = false;
    bool has_uv = false;
    size_t total = 0;
    std::vector<uint32_t> tri_ix;

    for (size_t ni = 0; ni < scene->nodes.count && err.empty(); ++ni) {
        ufbx_node* node = scene->nodes.data[ni];
        if (!node || !node->mesh) continue;
        // KH_FBX_LOD_GROUP: an LOD group node's children are one model at its levels of detail, in order
        // (ufbx_lod_group); merged, every level would draw over the others. Only the first child's subtree - LOD0 - is
        // imported; kh_lod_build makes the coarser levels from it, as for any import.
        bool khlg_skip = false;
        for (const ufbx_node* khlg_c = node; khlg_c->parent && !khlg_skip; khlg_c = khlg_c->parent) {
            const ufbx_node* khlg_p = khlg_c->parent;
            khlg_skip = khlg_p->attrib_type == UFBX_ELEMENT_LOD_GROUP && khlg_p->children.count > 0 &&
                        khlg_p->children.data[0] != khlg_c;
        }
        if (khlg_skip) continue;
        ufbx_mesh* mesh = node->mesh;
        // KH_CLOTH: resolved once per node, then read per corner below.
        std::vector<float> khfc_sim, khfc_pin;
        bool khfc_has_sim = false, khfc_has_pin = false;
        const bool khfc_node_cloth = kh_cloth_fbx_read(mesh, khfc_sim, khfc_pin, khfc_has_sim, khfc_has_pin);
        if (khfc_node_cloth) khfc_any = true;
        std::vector<KhSkinInf> khsk_cp;   // KH_SKEL: this node's influences per control point.
        const bool khsk_node = kh_skin_fbx_read(mesh, khsk_s, khsk_cp);
        if (khsk_node) khsk_any = true;
        const ufbx_matrix g2w = node->geometry_to_world;
        const ufbx_matrix nmat = ufbx_matrix_for_normals(&g2w);
        const bool khmi_neg = ufbx_matrix_determinant(&g2w) < 0.0;   // KH_MIKK_MIRROR (below).
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
                // KH_FBX_NODE_MAT: a mesh several nodes share may carry other materials on each (3ds Max, Maya
                // instances); ufbx gives a node's own at the same index in node->materials, and mesh->materials
                // is the first node's. ufbx fills a node's list out to the mesh's length from the mesh's, so for
                // a mesh with one node the two name the same materials.
                const ufbx_material* khnd_m = mi < node->materials.count && node->materials.data[mi]
                                            ? node->materials.data[mi]
                                            : mi < mesh->materials.count ? mesh->materials.data[mi] : nullptr;
                if (khnd_m && khnd_m->name.length > 0) {
                    mname = khnd_m->name.data;
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
            std::vector<KhSkinInf>& khsk_bs = bucket_s[bi];

            for (size_t k = 0; k < ntri * 3; ++k) {
                // KH_MIKK_MIRROR: MikkTSpace (kh_gen_tangents) takes a triangle's handedness from the sign of its uv
                // area in corner order, which is right only for a triangle wound with its normal. A node whose
                // transform mirrors (det < 0) winds its file order against its normals (ufbx_matrix_for_normals keeps
                // them outward), so taken in file order its tangents would get the wrong handedness. The corners are
                // taken 0, 2, 1: meshgen::bake's winding test, taken after its y / z swap, is then exactly negated
                // (corner 0 is the same), so bake swaps them back to file order - the geometry is unchanged and only
                // the handedness differs.
                const uint32_t ix = tri_ix[khmi_neg ? k - k % 3 + (3 - k % 3) % 3 : k];
                MeshVertex mv = {};
                const ufbx_vec3 p = ufbx_transform_position(&g2w, ufbx_get_vertex_vec3(&mesh->vertex_position, ix));
                // KH_IMPORT_BOUND: a position that is not finite, or is 1e7 m or more from the origin (the mesh
                // cache reader's bound), is refused - tested on the double, whose float cast out of range is undefined.
                if (!(fabs(p.x) < 1.0e7 && fabs(p.y) < 1.0e7 && fabs(p.z) < 1.0e7)) {
                    err = "mesh has a vertex position that is not finite or is 10,000 km or more from its origin";
                    break;
                }
                mv.pos[0] = static_cast<float>(p.x);
                mv.pos[1] = static_cast<float>(p.y);
                mv.pos[2] = static_cast<float>(p.z);

                // KH_IMPORT_BOUND: a normal past 1e18 (NaN included) takes the default, as a missing one does -
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
                    if (fabs(uv.x) < 1.0e18 && fabs(uv.y) < 1.0e18) {   // KH_IMPORT_BOUND: else this corner's 0.
                        mv.uv[0] = static_cast<float>(uv.x);
                        mv.uv[1] = 1.0f - static_cast<float>(uv.y);   // FBX bottom-left -> D3D top-left.
                    }
                    has_uv = true;
                }

                mv.tan[3] = 1.0f;
                {   // KH_CLOTH: 255 (fully simulated) is the value a node with no authoring contributes, so an
                    // unpainted material on a painted model does not silently pin itself.
                    uint8_t khfc_w = 255u;
                    if (khfc_node_cloth) {
                        const uint32_t khfc_v = mesh->vertex_indices.data[ix];
                        khfc_w = khfc_v < khfc_sim.size()
                               ? kh_cloth_weight_of(khfc_sim[khfc_v], khfc_pin[khfc_v], khfc_has_sim)
                               : 255u;
                    }
                    bw.push_back(khfc_w);
                }
                if (khsk_any) {
                    // KH_SKEL: the corner's control point's influences (none off an unskinned node). Nothing is kept
                    // before the first skinned node, so an unskinned import pays nothing; the corners a bucket took
                    // before it are padded as none here, and the normalization below bounds its read on the bucket's
                    // length.
                    if (khsk_bs.size() < bv.size()) khsk_bs.resize(bv.size());
                    const uint32_t khsk_v = mesh->vertex_indices.data[ix];
                    khsk_bs.push_back(khsk_node && khsk_v < khsk_cp.size() ? khsk_cp[khsk_v] : KhSkinInf());
                }
                bv.push_back(mv);
                if (++total > KH_FBX_VERT_CEIL) { err = "mesh exceeds the D3D11 vertex buffer size limit"; break; }
            }

            if (!err.empty()) break;
        }
    }

    // KH_SKEL: the skeleton is finished while the scene still lives - it walks the bone nodes' parents - and its node
    // pointers are dropped with it, so nothing below can reach the freed scene through them.
    if (khsk_any) kh_skin_finish(khsk_s);
    khsk_s.node.clear();
    khsk_s.ix.clear();
    khfh_hold.khfh_s = nullptr;   // KH_FBX_SCENE_HOLD: freed here.
    ufbx_free_scene(scene);
    if (!err.empty()) return false;
    if (std::find(khsk_s.head_bad.begin(), khsk_s.head_bad.end(), 1u) != khsk_s.head_bad.end()) {   // KH_IMPORT_BOUND.
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
        // KH_IMPORT_BOUND: the box the mesh cache reader accepts (kh_mesh_cache_load_one), so an import is always
        // read back: the extent can reach 2e7 m, and a float position can round up onto 1e7 m.
        if (!(ext[k] < 1.0e7f)) { err = "mesh is 10,000 km or more across"; return false; }
        if (!(fabsf(ctr[k]) < 1.0e7f)) { err = "mesh is centred 10,000 km or more from its origin"; return false; }
        // KH_IMPORT_FLAT: an axis under 1e-4 m (a flat sheet, a decal) is normalized by 1e-4 - the floor native_size
        // takes - so native_ext equals native_size on every axis and KhVsCore's n / size undoes the stored normal's
        // weights exactly (its own floor only guards a zero axis, KH_NRM_FLOOR). Do not normalize such an axis by 1 (it
        // weights the axis 1e4 times too heavily: a flat mesh's bent normals draw pulled onto its plane's normal) or by
        // its true extent (too lightly: they draw over-tilted); the skin's n / native_ext reads both right. The
        // positions stay centred, within +-0.5. An axis of 1e-4 m or more is untouched.
        if (!(ext[k] > 1e-4f)) ext[k] = 1e-4f;
        d.native_size[k] = ext[k];
    }
    // KH_SKEL: the box the positions are normalized by, in engine axes (the swap meshgen::bake makes below), so p *
    // native_ext + native_ctr is where a stored position was authored.
    d.native_ctr[0] = ctr[0]; d.native_ctr[1] = ctr[2]; d.native_ctr[2] = ctr[1];
    d.native_ext[0] = ext[0]; d.native_ext[1] = ext[2]; d.native_ext[2] = ext[1];
    std::unordered_map<KhClothPosKey, KhSkinInf, KhClothPosHash> khsk_map;
    if (khsk_any) khsk_map.reserve(total / 2u + 16u);

    d.verts.reserve(total);
    std::unordered_map<KhClothPosKey, uint8_t, KhClothPosHash> khfc_map;
    if (khfc_any) khfc_map.reserve(total / 2u + 16u);

    for (size_t bi = 0; bi < buckets.size(); ++bi) {
        MeshSubmesh sm;
        sm.name = bucket_names[bi];
        sm.index_start = static_cast<uint32_t>(d.verts.size());   // Expanded units; weld keeps them.
        sm.index_count = static_cast<uint32_t>(buckets[bi].size());
        d.submeshes.push_back(std::move(sm));

        for (size_t khfc_i = 0; khfc_i < buckets[bi].size(); ++khfc_i) {
            MeshVertex& mv = buckets[bi][khfc_i];
            // The vertex stays in the AUTHORED frame (metres, unit normals) through the tangent generation below; the
            // squash into the unit box is one pass after it (KH_IMPORT_SQUASH). The keys the cloth weights and skin
            // influences are resolved by are the squashed position, computed here by the expression that pass uses, so
            // the resolve's un-bake lands on the same floats.
            float khin_sq[3];
            for (int k = 0; k < 3; ++k) khin_sq[k] = (mv.pos[k] - ctr[k]) / ext[k];
            // KH_CLOTH: keyed on the normalized PRE-BAKE position, which is what the resolve below un-bakes back to.
            if (khfc_any && khfc_i < bucket_w[bi].size()) {
                KhClothPosKey khfc_k;
                memcpy(khfc_k.p, khin_sq, sizeof(khfc_k.p));
                const uint8_t khfc_w = bucket_w[bi][khfc_i];
                auto khfc_it = khfc_map.find(khfc_k);
                // A welded position that two corners disagree about takes the more simulated of the two: a seam must
                // not pin a sheet.
                if (khfc_it == khfc_map.end()) khfc_map.emplace(khfc_k, khfc_w);
                else if (khfc_w > khfc_it->second) khfc_it->second = khfc_w;
            }
            // KH_SKEL: the same key. Two corners at one position from different control points keep ONE set of
            // influences - a split in the mesh at a single point would otherwise tear. A weighted corner takes the
            // position from an unweighted one (an unskinned node's corner there must not pin a skinned vertex to the
            // root); between two weighted ones the first stands, as good as the other.
            if (khsk_any && khfc_i < bucket_s[bi].size()) {
                KhClothPosKey khsk_k;
                memcpy(khsk_k.p, khin_sq, sizeof(khsk_k.p));
                const KhSkinInf& khsk_f = bucket_s[bi][khfc_i];
                const auto khsk_it = khsk_map.find(khsk_k);
                if (khsk_it == khsk_map.end()) khsk_map.emplace(khsk_k, khsk_f);
                else if (!(khsk_it->second.w[0] + khsk_it->second.w[1] + khsk_it->second.w[2] + khsk_it->second.w[3] > 0.0f) &&
                         khsk_f.w[0] + khsk_f.w[1] + khsk_f.w[2] + khsk_f.w[3] > 0.0f) khsk_it->second = khsk_f;
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
    if (has_uv) kh_gen_tangents(d.verts);   // Arma space; bake flips handedness.
    // KH_IMPORT_SQUASH: into the unit box, every attribute by its own rule. Positions by 1 / ext. KH_IMPORT_NRM: the
    // stored normal is the NORMALIZED-SPACE normal, as the builtins' are - KhVsCore draws normalize(n / size), the
    // inverse transpose of this scale, which is right only for the squashed geometry's normal, the authored one scaled
    // BY the extents (normals scale inversely to positions); stored as it came, the authored unit normal leans every
    // draw of a non-cubic import toward its thin axes. Tangents are covariant with positions (KhVsCore multiplies by
    // size): by 1 / ext, w untouched. The same frame for all three; the y / z swap is bake's, and moves them together.
    for (MeshVertex& mv : d.verts) {
        for (int k = 0; k < 3; ++k) mv.pos[k] = (mv.pos[k] - ctr[k]) / ext[k];
        float khin_n[3], khin_t[3];
        for (int k = 0; k < 3; ++k) { khin_n[k] = mv.nrm[k] * ext[k]; khin_t[k] = mv.tan[k] / ext[k]; }
        const float khin_l = kh_cloth_v3_len(khin_n);
        if (khin_l > 1.0e-12f) for (int k = 0; k < 3; ++k) mv.nrm[k] = khin_n[k] / khin_l;
        const float khin_tl = kh_cloth_v3_len(khin_t);
        if (khin_tl > 1.0e-12f) for (int k = 0; k < 3; ++k) mv.tan[k] = khin_t[k] / khin_tl;
    }
    meshgen::bake(d.verts, d.native_ext);   // The winding test in the authored frame (see bake).
    // KH_LOD_SKIN: the influences of the points the decimator placed anew (khsk_map holds the source points the
    // import keyed; one it did not - a bucket only unskinned nodes filled - comes back here with none).
    std::unordered_map<KhClothPosKey, KhSkinInf, KhClothPosHash> khsk_lod;
    {
        kh_lod_build(d, khsk_any ? &khsk_map : nullptr, khsk_any ? &khsk_lod : nullptr);
    }
    kh_mesh_weld(d);   // KH_MESH_INDEXED: every level welded at once, ranges unchanged.
    // KH_CLOTH: carry the weights onto the welded array. meshgen::bake's only per-vertex act on a position is the y/z
    // swap, and it is its own inverse, so un-baking a final position lands exactly on the key that was stored. A vertex
    // only a decimated LOD level uses finds a twin where it sits on a source position (KH_LOD_SEAM moves vertices onto
    // their neighbours) and stays 0, 'pinned', where the decimator placed it anew - unread either way, because a cloth
    // builds from level 0 and is lod_locked.
    if (khfc_any && !khfc_map.empty()) {
        d.cloth_w.assign(d.verts.size(), 0u);
        for (size_t khfc_i = 0; khfc_i < d.verts.size(); ++khfc_i) {
            KhClothPosKey khfc_k;
            khfc_k.p[0] = d.verts[khfc_i].pos[0];
            khfc_k.p[1] = d.verts[khfc_i].pos[2];
            khfc_k.p[2] = d.verts[khfc_i].pos[1];
            const auto khfc_it = khfc_map.find(khfc_k);
            if (khfc_it != khfc_map.end()) d.cloth_w[khfc_i] = khfc_it->second;
        }
    }
    // KH_SKEL: the influences onto the welded array, by the same un-bake. A vertex only a decimated level uses takes
    // its twin's where it sits on a source position, else the blend the decimator carried to the point it placed anew
    // (KH_LOD_SKIN, khsk_lod): every level skins.
    if (khsk_any) {
        d.skin_bones = khsk_s.bone;   // Finished above, before the scene was freed.
        d.skin_inf.assign(d.verts.size(), KhSkinInf());
        for (size_t khsk_i = 0; khsk_i < d.verts.size(); ++khsk_i) {
            KhClothPosKey khsk_k;
            khsk_k.p[0] = d.verts[khsk_i].pos[0];
            khsk_k.p[1] = d.verts[khsk_i].pos[2];
            khsk_k.p[2] = d.verts[khsk_i].pos[1];
            const auto khsk_it = khsk_map.find(khsk_k);
            if (khsk_it != khsk_map.end()) {
                d.skin_inf[khsk_i] = khsk_it->second;
            } else {
                const auto khsk_il = khsk_lod.find(khsk_k);   // KH_LOD_SKIN.
                if (khsk_il != khsk_lod.end()) d.skin_inf[khsk_i] = khsk_il->second;
            }
        }
    }
    // Persist the compiled result (post-tangent), then keep the cache under its cap.
    kh_stat(g_stats.fbx_imports);
    // The writer takes g_khsm_trim_mu: an FBX import can land while a shader batch writes .khsc blobs into the same
    // folder, and the trim enumerates and deletes.
    kh_mesh_occ_prep(d);   // KH_OCC_BOX: before the cache write reads it.
    const bool khmc_ok = kh_fbx_register(std::move(d), path, out_id, err);
    // Queued only on success, and only once the id is published: the writer reads the mesh by id and drops an
    // unpublished one, so the write waits for the publish and is queued where it lands (KH_MESH_CACHE_LATER). The
    // writer is started here, on the game thread, so that landing (the render thread) only queues. KH_EA_MW_INLINE:
    // no writer thread - the write and the trim run here, on the game thread (the appended def is immutable), never
    // at the landing.
    if (khmc_ok) {
        if (kh_mesh_cache_writer_start()) {
            kh_mesh_cache_later(khmc_hash, out_id);
        } else if (const MeshDef* khmc_d = kh_mesh_def_staged(out_id)) {
            kh_mesh_cache_store(khmc_hash, *khmc_d);
            std::lock_guard<std::mutex> khmc_tl(g_khsm_trim_mu);
            kh_mesh_cache_trim();
        }
    }
    return khmc_ok;
}

// The mesh-slot front door for ".fbx" strings: resolve via the rendering asset search, then import-or-reuse (cache by
// resolved path).
inline int kh_fbx_mesh_id(const std::string& request, std::string& err) {
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
            // KH_MESH_STAGED: alive, or appended with its publish still pending (kh_fbx_register) - the same mesh
            // either way; kh_mesh_alive alone reads the second as released and would import the file again on every
            // request until the publish lands.
            if (kh_mesh_alive(it->second) || kh_mesh_def_staged(it->second)) return it->second;
            g_fbx_cache.erase(it);   // KH_MESH_FREE: released since (a null tombstone); import again (disk cache hit).
        }
    }
    int id = -1;
    if (!kh_fbx_import(resolved, id, err)) return -1;
    {
        std::lock_guard<std::mutex> g(g_fbx_cache_mutex);
        g_fbx_cache[key] = id;
    }
    return id;
}
}