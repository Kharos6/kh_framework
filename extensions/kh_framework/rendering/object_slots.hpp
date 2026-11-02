#pragma once

// The feature half (rendering_integration_features.hpp): the per-object substitute vertex buffers (cloth, chain and
// skin slots, kh_mesh_vb_ref) and KhVbRef's operators. Never include it on its own.
namespace RenderIntegration {
inline bool operator==(const KhVbRef& a, const KhVbRef& b) { return a.vb == b.vb && a.off == b.off; }
inline bool operator!=(const KhVbRef& a, const KhVbRef& b) { return !(a == b); }
// KH_SKIN_POOL: the byte offset the element's buffer draws from (0 but for a skin pool chunk's slice), and the chunk
// itself (null for every other buffer) - beside g_cloth_vb_slot at every write, under its rules. A chunk's buffer is
// released only by kh_skin_upload - a growth after the walk began installing re-points the elements it installed
// (kh_skin_pool_rebind); the trim's releases and repack run before the walk installs any; a drop in the walk frees a
// chunk only with its last slice, whose binding installs after it (no element names that chunk) - and by
// kh_skin_release_all, after kh_cloth_release_all emptied them: no element names a released buffer.
static std::vector<UINT> g_cloth_voff_slot;
static std::vector<const KhSkinPoolChunk*> g_cloth_pool_slot;
// The mesh id each element was built from, under the same rules. A scene slot is reused once the live side has seen its
// object die, and the render thread takes in the new occupant through its own kh_scene_sync - with no park between them
// when nothing else is drawn. kh_scene_sync therefore clears a slot's six elements at the moment it frees the slot, so
// no occupant ever arrives to find a buffer there (kh_cloth_slot_forget), and kh_cloth_upload never installs a retired
// instance. kh_mesh_vb_ref's mesh test stands behind both: a slot match alone must never hand another mesh's index
// buffer this cloth's vertices.
static std::vector<int> g_cloth_mesh_slot;
// KH_LOD_SIM_SLOT: 1 where the element's buffer is a cloth's or a chain's (kh_cloth_upload), under the same rules; 0
// for the skin's, a vertex stage keeps its source's. A cloth leaves a decimated level's own vertices at rest and a
// chain's box is level 0's, so such a buffer draws level 0 alone (kh_lod_locked).
static std::vector<uint8_t> g_cloth_sim_slot;

inline uint32_t kh_cloth_gen_of(uint32_t khcg_slot) {
    return khcg_slot < g_cloth_gen_slot.size() ? g_cloth_gen_slot[khcg_slot] : 0u;
}

// A scene slot has become free (kh_scene_sync, under the park invariant on whichever thread walks next): its
// substitute goes before the slot is reused. The buffer is the instance's; kh_cloth_sync retires it and the grave
// releases it. kh_uvs_release_all also clears a slot still naming a vertex stage's buffer it releases.
inline void kh_cloth_slot_forget(uint32_t khsf_slot) {
    if (khsf_slot < g_cloth_vb_slot.size()) g_cloth_vb_slot[khsf_slot] = nullptr;
    if (khsf_slot < g_cloth_mesh_slot.size()) g_cloth_mesh_slot[khsf_slot] = -1;
    if (khsf_slot < g_cloth_gen_slot.size()) g_cloth_gen_slot[khsf_slot] = 0u;
    if (khsf_slot < g_cloth_sim_slot.size()) g_cloth_sim_slot[khsf_slot] = 0u;   // KH_LOD_SIM_SLOT.
    if (khsf_slot < g_cloth_voff_slot.size()) g_cloth_voff_slot[khsf_slot] = 0u;   // KH_SKIN_POOL.
    if (khsf_slot < g_cloth_pool_slot.size()) g_cloth_pool_slot[khsf_slot] = nullptr;
}

// The one place a draw resolves which vertex buffer an object uses. Every pass goes through it. A mesh id with no cloth
// answers exactly what the pass would have bound anyway, so the substitution costs one bounds test and one null test on
// the common path. KH_SKIN_POOL: the buffer and the offset its vertex array starts at (KhVbRef) - bound with that
// offset, compared as a pair.
inline KhVbRef kh_mesh_vb_ref(int khmv_mesh, uint32_t khmv_slot) {
    if (khmv_slot < g_cloth_vb_slot.size()) {
        ID3D11Buffer* khmv_c = g_cloth_vb_slot[khmv_slot];
        if (khmv_c && khmv_slot < g_cloth_mesh_slot.size() && g_cloth_mesh_slot[khmv_slot] == khmv_mesh) {
            KhVbRef khmv_r;
            khmv_r.vb = khmv_c;
            khmv_r.off = khmv_slot < g_cloth_voff_slot.size() ? g_cloth_voff_slot[khmv_slot] : 0u;
            return khmv_r;
        }
    }
    KhVbRef khmv_m;
    if (khmv_mesh < 0 || static_cast<size_t>(khmv_mesh) >= g_res.mesh_vb.size()) return khmv_m;
    khmv_m.vb = g_res.mesh_vb[khmv_mesh];
    return khmv_m;
}
// KH_SKIN_POOL: the skin pool chunk the object's slot draws from (its mesh's
// substitute, a chunk), else null. The tables' thread.
inline const KhSkinPoolChunk* kh_mesh_pool_of(int khmp_mesh, uint32_t khmp_slot) {
    if (khmp_slot >= g_cloth_pool_slot.size() || khmp_slot >= g_cloth_vb_slot.size() ||
        khmp_slot >= g_cloth_mesh_slot.size()) return nullptr;
    if (!g_cloth_vb_slot[khmp_slot] || g_cloth_mesh_slot[khmp_slot] != khmp_mesh) return nullptr;
    return g_cloth_pool_slot[khmp_slot];
}
}