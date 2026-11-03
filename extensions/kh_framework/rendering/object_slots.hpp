#pragma once

// The feature half (rendering_integration_features.hpp): the per-object substitute vertex buffers (cloth, chain and
// skin slots, mesh_vb_ref) and VbRef's operators. Never include it on its own.
namespace RenderIntegration {
inline bool operator==(const VbRef& a, const VbRef& b) { return a.vb == b.vb && a.off == b.off; }
inline bool operator!=(const VbRef& a, const VbRef& b) { return !(a == b); }
// SKIN_POOL: the byte offset the element's buffer draws from (0 but for a skin pool chunk's slice), and the chunk
// itself (null for every other buffer) - beside g_cloth_vb_slot at every write, under its rules. A chunk's buffer is
// released only by skin_upload - a growth after the walk began installing re-points the elements it installed
// (skin_pool_rebind); the trim's releases and repack run before the walk installs any; a drop in the walk frees a
// chunk only with its last slice, whose binding installs after it (no element names that chunk) - and by
// skin_release_all, after cloth_release_all emptied them: no element names a released buffer.
static std::vector<UINT> g_cloth_voff_slot;
static std::vector<const SkinPoolChunk*> g_cloth_pool_slot;
// The mesh id each element was built from, under the same rules. A scene slot is reused once the live side has seen its
// object die, and the render thread takes in the new occupant through its own scene_sync - with no park between them
// when nothing else is drawn. scene_sync therefore clears a slot's six elements at the moment it frees the slot, so
// no occupant ever arrives to find a buffer there (cloth_slot_forget), and cloth_upload never installs a retired
// instance. mesh_vb_ref's mesh test stands behind both: a slot match alone must never hand another mesh's index
// buffer this cloth's vertices.
static std::vector<int> g_cloth_mesh_slot;
// LOD_SIM_SLOT: 1 where the element's buffer is a cloth's or a chain's (cloth_upload), under the same rules; 0
// for the skin's, a vertex stage keeps its source's. A cloth leaves a decimated level's own vertices at rest and a
// chain's box is level 0's, so such a buffer draws level 0 alone (lod_locked).
static std::vector<uint8_t> g_cloth_sim_slot;

inline uint32_t cloth_gen_of(uint32_t slot) {
    return slot < g_cloth_gen_slot.size() ? g_cloth_gen_slot[slot] : 0u;
}

// A scene slot has become free (scene_sync, under the park invariant on whichever thread walks next): its
// substitute goes before the slot is reused. The buffer is the instance's; cloth_sync retires it and the grave
// releases it. uvs_release_all also clears a slot still naming a vertex stage's buffer it releases.
inline void cloth_slot_forget(uint32_t slot) {
    if (slot < g_cloth_vb_slot.size()) g_cloth_vb_slot[slot] = nullptr;
    if (slot < g_cloth_mesh_slot.size()) g_cloth_mesh_slot[slot] = -1;
    if (slot < g_cloth_gen_slot.size()) g_cloth_gen_slot[slot] = 0u;
    if (slot < g_cloth_sim_slot.size()) g_cloth_sim_slot[slot] = 0u;   // LOD_SIM_SLOT.
    if (slot < g_cloth_voff_slot.size()) g_cloth_voff_slot[slot] = 0u;   // SKIN_POOL.
    if (slot < g_cloth_pool_slot.size()) g_cloth_pool_slot[slot] = nullptr;
}

// The one place a draw resolves which vertex buffer an object uses. Every pass goes through it. A mesh id with no cloth
// answers exactly what the pass would have bound anyway, so the substitution costs one bounds test and one null test on
// the common path. SKIN_POOL: the buffer and the offset its vertex array starts at (VbRef) - bound with that
// offset, compared as a pair.
inline VbRef mesh_vb_ref(int mesh, uint32_t slot) {
    if (slot < g_cloth_vb_slot.size()) {
        ID3D11Buffer* c = g_cloth_vb_slot[slot];
        if (c && slot < g_cloth_mesh_slot.size() && g_cloth_mesh_slot[slot] == mesh) {
            VbRef r;
            r.vb = c;
            r.off = slot < g_cloth_voff_slot.size() ? g_cloth_voff_slot[slot] : 0u;
            return r;
        }
    }
    VbRef m;
    if (mesh < 0 || static_cast<size_t>(mesh) >= g_res.mesh_vb.size()) return m;
    m.vb = g_res.mesh_vb[mesh];
    return m;
}
// SKIN_POOL: the skin pool chunk the object's slot draws from (its mesh's
// substitute, a chunk), else null. The tables' thread.
inline const SkinPoolChunk* mesh_pool_of(int mesh, uint32_t slot) {
    if (slot >= g_cloth_pool_slot.size() || slot >= g_cloth_vb_slot.size() ||
        slot >= g_cloth_mesh_slot.size()) return nullptr;
    if (!g_cloth_vb_slot[slot] || g_cloth_mesh_slot[slot] != mesh) return nullptr;
    return g_cloth_pool_slot[slot];
}
}