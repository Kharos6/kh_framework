#pragma once

// The feature half (rendering_integration_features.hpp): the asset GC: meshes, materials and textures released when
// idle. Never include it on its own.
namespace RenderIntegration {
// Session VRAM is not monotonic: a registry mesh unreferenced for KH_MESH_GC_IDLE_MS is released (def + VB + IB) by
// kh_mesh_gc in flush_locked; textures follow through kh_tex_cache_gc. Ids are never reused: a freed slot is a
// tombstone mesh_id_clamp folds to 0; a by-path lookup re-imports from the disk cache.
static constexpr uint64_t KH_MESH_GC_IDLE_MS = 120000;
static constexpr uint64_t KH_TEX_GC_IDLE_MS  = 180000;
static constexpr uint64_t KH_MESH_GRAVE_FLUSHES = 2;   // The release wall, in flushes.
inline bool kh_mesh_write_busy(int khwb_id) {
    std::lock_guard<std::mutex> khwb_l(g_khmw_mu);   // KH_MESH_FREE: the busy id and the queue in one look.
    if (g_khmw_busy_id.load(std::memory_order_acquire) == khwb_id) return true;
    for (const KhMeshWriteJob& khwb_j : g_khmw_q) if (!khwb_j.tex && khwb_j.id == khwb_id) return true;
    return false;
}

// From the flush's housekeeping (kh_mesh_gc: the render thread, or the game thread under the park) and the session
// teardown (kh_mesh_release_session, under the park). The GPU objects + def go to the graveyard for the release wall.
inline void kh_mesh_release(int khmr_id) {
    MeshDef* khmr_d = kh_mesh_tombstone(khmr_id);
    if (!khmr_d) return;
    {
        std::lock_guard<std::mutex> khmr_l(g_fbx_cache_mutex);
        for (auto khmr_it = g_fbx_cache.begin(); khmr_it != g_fbx_cache.end();) {
            if (khmr_it->second == khmr_id) khmr_it = g_fbx_cache.erase(khmr_it);
            else ++khmr_it;
        }
    }
    {
        // KH_CLOTH: ids are never reused, so a stale entry is never read - but nothing else would ever free it. A
        // worker mid-step holds its own reference (in_col_own), as does this frame's g_physics_col.
        std::lock_guard<std::mutex> khmr_c(g_physics_bvh_mu);
        for (auto khmr_it = g_physics_bvh.begin(); khmr_it != g_physics_bvh.end();) {
            if (static_cast<int>(khmr_it->first >> 32) == khmr_id) khmr_it = g_physics_bvh.erase(khmr_it);
            else ++khmr_it;
        }
    }
    Resources::KhMeshGrave khmr_g = { nullptr, nullptr, khmr_d, g_flush_serial.load(std::memory_order_relaxed) };
    const size_t khmr_i = static_cast<size_t>(khmr_id);
    if (khmr_i < g_res.mesh_vb.size()) { khmr_g.vb = g_res.mesh_vb[khmr_i]; g_res.mesh_vb[khmr_i] = nullptr; }
    if (khmr_i < g_res.mesh_ib.size()) { khmr_g.ib = g_res.mesh_ib[khmr_i]; g_res.mesh_ib[khmr_i] = nullptr; }
    g_res.mesh_grave.push_back(khmr_g);
    kh_stat(g_stats.meshes_released);
}

// Once per flush: drain graves older than the wall.
inline void kh_mesh_grave_sweep() {
    for (size_t khgs_i = 0; khgs_i < g_res.mesh_grave.size();) {
        Resources::KhMeshGrave& khgs_g = g_res.mesh_grave[khgs_i];
        if (g_flush_serial.load(std::memory_order_relaxed) - khgs_g.flush_at < KH_MESH_GRAVE_FLUSHES) { ++khgs_i; continue; }
        KH_SAFE_RELEASE(khgs_g.vb);
        KH_SAFE_RELEASE(khgs_g.ib);
        delete khgs_g.def;
        g_res.mesh_grave[khgs_i] = g_res.mesh_grave.back();
        g_res.mesh_grave.pop_back();
    }
}

// Once per flush. "Idle" means no object carried the id for the whole window. KH_MESH_REF_LOCK - the census may be a
// frame old, so the GC first notes the list itself (under g_draw_list_mutex; skipped while the census stamped under 1 s
// ago, KH_GC_NOTE_SKIP), then reads the table under its own lock, then releases holding neither (kh_mesh_write_busy
// takes g_khmw_mu, kh_mesh_release g_fbx_cache_mutex, g_physics_bvh_mu and the mesh store's append_mx). An object
// inserted between the note and the release, on a mesh idle for two minutes, draws the release's fallback. KH_GC_CLOCK:
// the idle test's clock is read under g_mesh_ref_mu, which every g_mesh_last_ref write holds, so no stamp is later than
// it. Do not read it before the lock: a census or import stamp (flush_frame / the importer, game thread) taken on the
// next tick makes now - stamp wrap to a huge age and releases a mesh a live object carries.
inline void kh_mesh_gc() {
    KH_PROF_SCOPE(KHP_MESH_GC);   // KH_PROF.
    // KH_GC_NOTE_SKIP: the census stamps the same references every frame; its notes under 1 s old stand for this walk
    // (the idle window is 120 s).
    if (GetTickCount64() - g_ref_note_cen_ms.load(std::memory_order_acquire) > 1000ull) {
        std::lock_guard<std::mutex> khgc_l(g_draw_list_mutex);
        kh_ref_note_begin(g_ref_note_gc);   // KH_REF_NOTE_ONCE.
        for (const auto& khgc_kv : g_draw_list) {
            if (!khgc_kv.second.fullscreen) kh_ref_note_add(g_ref_note_gc, khgc_kv.second.mesh);
        }
        kh_ref_note_end(g_ref_note_gc);
    }
    const uint32_t khgc_n = mesh_count();
    const uint32_t khgc_b = mesh_store().builtin_n;
    static std::vector<int> khgc_idle;
    khgc_idle.clear();
    {
        std::lock_guard<std::mutex> khgc_r(g_mesh_ref_mu);
        const uint64_t khgc_now = GetTickCount64();   // KH_GC_CLOCK: under the lock.
        for (uint32_t khgc_id = khgc_b; khgc_id < khgc_n; ++khgc_id) {
            if (!kh_mesh_alive(static_cast<int>(khgc_id))) continue;
            const uint64_t khgc_ref = khgc_id < g_mesh_last_ref.size() ? g_mesh_last_ref[khgc_id] : 0;
            if (khgc_now - khgc_ref < KH_MESH_GC_IDLE_MS) continue;
            khgc_idle.push_back(static_cast<int>(khgc_id));
        }
    }
    for (int khgc_id : khgc_idle) {
        if (kh_mesh_write_busy(khgc_id)) continue;
        kh_mesh_release(khgc_id);
    }
}

// KH_SESSION_RESET - the collector with no idle window: every non-builtin mesh is released (tombstoned, its path key
// forgotten, its buffers and def to the grave g_res.release then frees). A pending publish is landed first so its mesh
// goes too. Ids are never reused, so an id a script kept from an earlier session reads as no mesh rather than as
// another one. Under the park, with the cache writer stopped (kh_mesh_write_busy).
inline void kh_mesh_release_session() {
    kh_mesh_publish();
    g_mesh_publish_pending.store(false, std::memory_order_relaxed);
    {   // KH_MESH_CACHE_LATER: with the pending publish it waited for.
        std::lock_guard<std::mutex> khml_g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.clear();
    }
    const uint32_t khmx_n = mesh_count();
    for (uint32_t khmx_id = mesh_store().builtin_n; khmx_id < khmx_n; ++khmx_id) {
        if (kh_mesh_alive(static_cast<int>(khmx_id))) kh_mesh_release(static_cast<int>(khmx_id));
    }
}

// Once per flush: idle material sets go through the grave. References are noted by flush_frame's census and, first, by
// this collector itself. KH_MAT_REF_LOCK - as kh_mesh_gc: the collector runs in flush_locked's housekeeping, on the
// render thread every scene flush, and that flush keeps running while g_kh_track_wanted stands even when flush_frame
// (the census) does not - a Draw3D handler removed by a script leaves it standing. Without its own note, every set a
// live object carries would read idle after KH_MAT_GC_IDLE_MS and be deleted under the objects. The draw list is noted
// under g_draw_list_mutex, then g_mat_pool_mu (the census's order); with the census running this only refreshes what it
// already refreshed. KH_GC_CLOCK: the idle test's clock is read under g_mat_pool_mu, which every last_ref_ms write
// holds, so no stamp is later than it. Do not read it before the lock: a census or intern stamp taken on the next tick
// makes now - stamp wrap to a huge age and graves a set a live object carries.
inline void kh_material_gc() {
    const KhMaterialSet* khmg_pin = kh_default_material_set();   // Interns on first call: before the lock.
    {
        std::lock_guard<std::mutex> khmg_d(g_draw_list_mutex);
        std::lock_guard<std::mutex> khmg_r(g_mat_pool_mu);
        const uint64_t khmg_seen = GetTickCount64();
        for (const auto& khmg_kv : g_draw_list) {
            const KhMaterialSet* khmg_s = khmg_kv.second.materials;
            if (!khmg_s) continue;
            auto khmg_it = g_mat_pool.find(khmg_s->hash);
            if (khmg_it != g_mat_pool.end()) khmg_it->second.last_ref_ms = khmg_seen;
        }
    }
    std::lock_guard<std::mutex> khmg_l(g_mat_pool_mu);
    const uint64_t khmg_now = GetTickCount64();   // KH_GC_CLOCK: under the lock.
    for (size_t khmg_i = 0; khmg_i < g_mat_grave.size();) {
        if (g_flush_serial.load(std::memory_order_relaxed) - g_mat_grave[khmg_i].flush_at < KH_MESH_GRAVE_FLUSHES) { ++khmg_i; continue; }
        g_mat_grave[khmg_i] = std::move(g_mat_grave.back());
        g_mat_grave.pop_back();
    }
    bool khmg_moved = false;
    for (auto khmg_it = g_mat_pool.begin(); khmg_it != g_mat_pool.end();) {
        if (khmg_it->second.set.get() == khmg_pin ||
            khmg_now - khmg_it->second.last_ref_ms < KH_MAT_GC_IDLE_MS) { ++khmg_it; continue; }
        KhMatGrave khmg_g;
        khmg_g.set = std::move(khmg_it->second.set);
        khmg_g.flush_at = g_flush_serial.load(std::memory_order_relaxed);
        g_mat_grave.push_back(std::move(khmg_g));
        khmg_it = g_mat_pool.erase(khmg_it);
        khmg_moved = true;
    }
    if (khmg_moved) kh_mat_gpu_rebuild();   // KH_MAT_TABLE: compaction by re-add.
}

// KH_SESSION_RESET - every material set but the pinned default goes, and the default's GPU state with it (its table
// entries named the old pages). No object references a set here: the draw list and the scene are empty.
inline void kh_material_pool_reset() {
    const KhMaterialSet* khmx_pin = kh_default_material_set();   // Interns on first call: before the lock.
    std::lock_guard<std::mutex> khmx_l(g_mat_pool_mu);
    g_mat_grave.clear();
    for (auto khmx_it = g_mat_pool.begin(); khmx_it != g_mat_pool.end();) {
        if (khmx_it->second.set.get() == khmx_pin) {
            khmx_it->second.set->gpu = KhMaterialSet::Gpu();
            ++khmx_it;
        } else {
            khmx_it = g_mat_pool.erase(khmx_it);
        }
    }
}

// Once per flush: idle texture views go. Failed entries stay (no VRAM; they suppress repeated reports).
inline void kh_tex_cache_gc() {
    const uint64_t khtg_now = GetTickCount64();
    bool khtg_any = false;
    // A resolve is the only refresh of last_use_ms, so a texture in continuous use reads idle here; before the sweep,
    // touch every entry a pool set still names. Done only when something has aged out.
    {
        bool khtg_aged = false;
        for (const auto& khtg_kv : g_tex_cache) {
            if (!khtg_kv.second.failed && khtg_now - khtg_kv.second.last_use_ms >= KH_TEX_GC_IDLE_MS) { khtg_aged = true; break; }
        }
        if (khtg_aged) {
            std::lock_guard<std::mutex> khtg_l(g_mat_pool_mu);
            for (const auto& khtg_kv : g_mat_pool) {
                for (const KhMaterial& khtg_m : khtg_kv.second.set->slots) {
                    if (!khtg_m.used) continue;
                    for (int khtg_s = 0; khtg_s < KH_MAT_MAPS; ++khtg_s) {
                        if (khtg_m.maps[khtg_s].path.empty()) continue;
                        auto khtg_hit = g_tex_cache.find(kh_tex_cache_key(khtg_m.maps[khtg_s].path,
                                                                          kh_tex_map_srgb(khtg_m, khtg_s)));
                        if (khtg_hit != g_tex_cache.end()) khtg_hit->second.last_use_ms = khtg_now;
                    }
                }
            }
        }
    }
    for (auto khtg_it = g_tex_cache.begin(); khtg_it != g_tex_cache.end();) {
        KhTexEntry& khtg_e = khtg_it->second;
        if (khtg_e.failed || khtg_now - khtg_e.last_use_ms < KH_TEX_GC_IDLE_MS) { ++khtg_it; continue; }
        if (khtg_e.page >= 0 && static_cast<size_t>(khtg_e.page) < g_tex_pages.size()) {
            KhTexPage& khtg_p = g_tex_pages[khtg_e.page];
            if (khtg_p.tex && khtg_e.layer >= 0 && static_cast<UINT>(khtg_e.layer) < khtg_p.cap && khtg_p.used[khtg_e.layer]) {
                khtg_p.used[khtg_e.layer] = 0;
                if (--khtg_p.used_n == 0) { KH_SAFE_RELEASE(khtg_p.srv); KH_SAFE_RELEASE(khtg_p.tex); khtg_p.used.clear(); khtg_p.cap = 0; ++g_tex_page_gen; }
            }
        }
        khtg_it = g_tex_cache.erase(khtg_it);
        kh_stat(g_stats.textures_released);
        khtg_any = true;
    }
    if (khtg_any) kh_mat_gpu_rebuild();   // KH_MAT_TABLE: entries named freed layers.
}
}