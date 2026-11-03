#pragma once

// The shared tail (rendering_integration.hpp): the render-object API (add / update / remove / clear, the handle list).
// Never include it on its own.
namespace RenderIntegration {

inline std::string add_render_object(const RenderObject& obj) {
    if (!render_on()) return std::string();   // PLAYER_ONLY.
    ensure_draw_eh();
    g_track_wanted.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const std::string handle = make_render_uid();
    RenderObject& stored = g_draw_list[handle];
    stored = obj;
    stored.seq = ++g_next_seq;   // Creation order (fullscreen pass chaining).
    stored.birth_time = effect_time_seconds();
    stored.fx_t0 = effect_time_seconds_d();
    stored.slot = scene_slot_alloc();   // SCENE.
    g_scene_stage[stored.slot] = &stored;   // Node-stable in unordered_map.
    g_scene_handle[stored.slot] = handle;
    scene_mark(stored.slot);
    return handle;
}

// The script side's property update. The staged copy carries the slot from the entry it was copied from; the entry
// may have been removed and the slot reused meanwhile, so the slot is re-taken from the live entry.
// UPDATE_MERGE: staged was copied from the entry (base) and the property applied to it outside the lock, while
// other writers kept writing the entry: attach_step (render thread too) the pose (pos, rot_m, rotated), the skin
// and chain uploads and the skinned draw the box (size, skel_ctr, pos), and the cloth upload a plain cloth's
// refitted bound (cast_size). pos, rot_m / rotated, size and skel_ctr are each taken from the live entry unless the
// property changed it (staged differs from base; for size and skel_ctr, also unless the mesh or the box's owner
// changed); cast_size, which no property writes, is the live entry's while the object stays a plain cloth on the same
// mesh and zero otherwise (CAST_REFIT, below), and skin_vbase, which no property writes either, is always the live
// entry's (EA_VBASE_MERGE: the skin upload's slice base) - the property wins what it set, and nothing it did not
// set is written back from the copy's older state.
inline bool update_render_object(const std::string& handle, RenderObject&& staged, const RenderObject& base) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_draw_list.find(handle);
    if (it == g_draw_list.end()) return false;
    const RenderObject& live = it->second;
    staged.slot = live.slot;
    if (memcmp(staged.pos, base.pos, sizeof(staged.pos)) == 0) {
        memcpy(staged.pos, live.pos, sizeof(staged.pos));
        memcpy(staged.pos_res, live.pos_res, sizeof(staged.pos_res));   // POS_RES.
        memcpy(staged.pos_res_at, live.pos_res_at, sizeof(staged.pos_res_at));
    }
    if (memcmp(staged.rot_m, base.rot_m, sizeof(staged.rot_m)) == 0 && staged.rotated == base.rotated) {
        memcpy(staged.rot_m, live.rot_m, sizeof(staged.rot_m));
        staged.rotated = live.rotated;
    }
    // The box is its mesh's and its owner's (the skin while skel, the chain while chain_sim, else the script): a
    // property that changed either owns it whole, and so does an entry another update re-owned meanwhile.
    if (staged.mesh == base.mesh && live.mesh == base.mesh &&
        staged.skel == base.skel && live.skel == base.skel &&
        staged.chain_sim == base.chain_sim && live.chain_sim == base.chain_sim) {
        if (memcmp(staged.size, base.size, sizeof(staged.size)) == 0) {
            memcpy(staged.size, live.size, sizeof(staged.size));
        }
        if (memcmp(staged.skel_ctr, base.skel_ctr, sizeof(staged.skel_ctr)) == 0) {
            memcpy(staged.skel_ctr, live.skel_ctr, sizeof(staged.skel_ctr));
        }
    }
    // CAST_REFIT: the cloth upload's put (cloth_upload, in flush_locked - the render thread's in the NO_PARK
    // path) owns a plain cloth's cast_size and no property writes it, so the live entry's is the newest; the copy's
    // is the snapshot's, a bound the reach may have outgrown since (a frame of the smaller bound at the frustum or
    // shadow edge, until the next flush re-put it). It is a bound only while the object is still what the put admits
    // (a plain cloth: not skeletal, not a chain) on the mesh it was measured on; a property that ends that returns it
    // to zero, and the bound is size again (the put never clears it, and a new mesh's reach starts unmeasured).
    if (staged.mesh == base.mesh && !staged.skel && !staged.chain_sim && cloth_obj_sim(staged)) {
        memcpy(staged.cast_size, live.cast_size, sizeof(staged.cast_size));
    } else {
        memset(staged.cast_size, 0, sizeof(staged.cast_size));
    }
    // EA_VBASE_MERGE: skin_upload's in-place write; the copy's may name a slice the binding has left (its
    // record then mismatches the tables and the bucket route draws it per object until the next upload).
    staged.skin_vbase = live.skin_vbase;
    it->second = staged;
    scene_mark(it->second.slot);
    return true;
}

inline bool remove_render_object(const std::string& handle) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_draw_list.find(handle);
    if (it == g_draw_list.end()) return false;
    scene_stage_remove(it->second);
    attach_drop(handle);   // ATTACH.
    g_draw_list.erase(it);
    return true;
}

inline size_t clear_render_objects() {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const size_t n = g_draw_list.size();
    for (auto& kv : g_draw_list) scene_stage_remove(kv.second);
    attach_drop_all(true);   // ATTACH: mid-mission, so the proxies are ours to delete.
    g_draw_list.clear();
    return n;
}

// ALL_HANDLES: every live handle, appended in creation order (seq). The map is unordered, so the walk collects
// under the mutex and sorts outside it.
inline void render_object_handles(std::vector<std::string>& out) {
    std::vector<std::pair<uint64_t, std::string>> v;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        v.reserve(g_draw_list.size());
        for (const auto& kv : g_draw_list) v.emplace_back(kv.second.seq, kv.first);
    }
    std::sort(v.begin(), v.end());
    for (auto& p : v) out.push_back(std::move(p.second));
}

}