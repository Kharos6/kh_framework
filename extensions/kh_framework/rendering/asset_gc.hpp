#pragma once

// The feature half (rendering_integration_features.hpp): the asset GC: meshes, materials and textures released when
// idle. Never include it on its own.
namespace RenderIntegration {
// Session VRAM is not monotonic: a registry mesh unreferenced for MESH_GC_IDLE_MS (a generated one:
// MESH_GEN_GC_IDLE_MS) is released (def + VB + IB) by mesh_gc in flush_locked; textures follow through
// tex_cache_gc. Ids are never reused: a freed slot is a tombstone mesh_id_clamp folds to 0; a by-path lookup
// re-imports from the disk cache, a generated mesh's spec builds it again.
static constexpr uint64_t MESH_GC_IDLE_MS = 120000;
// MESH_GEN_GC: a generated mesh's window - a rebuild's predecessors go within seconds, not two minutes (it is built
// again from its spec, not read from disk). Far above the census's staleness (a mesh a live object carries is stamped
// every frame, or by mesh_gc's own note when the census is over 1 s old - GC_NOTE_SKIP).
static constexpr uint64_t MESH_GEN_GC_IDLE_MS = 5000;
static constexpr uint64_t TEX_GC_IDLE_MS  = 180000;
static constexpr uint64_t MESH_GRAVE_FLUSHES = 2;   // The release wall, in flushes.
inline bool mesh_write_busy(int id) {
    std::lock_guard<std::mutex> l(g_meshwriter_mu);   // MESH_FREE: the busy id and the queue in one look.
    if (g_meshwriter_busy_id.load(std::memory_order_acquire) == id) return true;
    for (const MeshWriteJob& j : g_meshwriter_q) if (!j.tex && j.id == id) return true;
    return false;
}

// From the flush's housekeeping (mesh_gc: the render thread, or the game thread under the park) and the session
// teardown (mesh_release_session, under the park). The GPU objects + def go to the graveyard for the release wall.
// mesh_release_def: the rest of a release whose mesh_tombstone already ran (d its result; null = nothing).
inline void mesh_release_def(int id, MeshDef* d) {
    if (!d) return;
    {
        std::lock_guard<std::mutex> l(g_fbx_cache_mutex);
        for (auto it = g_fbx_cache.begin(); it != g_fbx_cache.end();) {
            if (it->second == id) it = g_fbx_cache.erase(it);
            else ++it;
        }
        // MESH_GEN_SHARE: a generated mesh's entry, found by the spec hash its name carries (gen_build); the
        // id test leaves an entry that names another mesh alone.
        if (mesh_is_generated(*d)) {
            const uint64_t h = strtoull(d->name.c_str() + 10, nullptr, 16);
            const auto g = g_mesh_gen_cache.find(h);
            if (g != g_mesh_gen_cache.end() && g->second == id) g_mesh_gen_cache.erase(g);
        }
    }
    {
        // CLOTH: ids are never reused, so a stale entry is never read - but nothing else would ever free it. A
        // worker mid-step holds its own reference (in_col_own), as does this frame's g_physics_col.
        std::lock_guard<std::mutex> c(g_physics_bvh_mu);
        for (auto it = g_physics_bvh.begin(); it != g_physics_bvh.end();) {
            if (static_cast<int>(it->first >> 32) == id) it = g_physics_bvh.erase(it);
            else ++it;
        }
    }
    Resources::MeshGrave g = { nullptr, nullptr, d, g_flush_serial.load(std::memory_order_relaxed) };
    const size_t i = static_cast<size_t>(id);
    if (i < g_res.mesh_vb.size()) { g.vb = g_res.mesh_vb[i]; g_res.mesh_vb[i] = nullptr; }
    if (i < g_res.mesh_ib.size()) { g.ib = g_res.mesh_ib[i]; g_res.mesh_ib[i] = nullptr; }
    g_res.mesh_grave.push_back(g);
    stat_inc(g_stats.meshes_released);
}
inline void mesh_release(int id) { mesh_release_def(id, mesh_tombstone(id)); }

// Once per flush: drain graves older than the wall.
inline void mesh_grave_sweep() {
    for (size_t i = 0; i < g_res.mesh_grave.size();) {
        Resources::MeshGrave& g = g_res.mesh_grave[i];
        if (g_flush_serial.load(std::memory_order_relaxed) - g.flush_at < MESH_GRAVE_FLUSHES) { ++i; continue; }
        KH_SAFE_RELEASE(g.vb);
        KH_SAFE_RELEASE(g.ib);
        delete g.def;
        g_res.mesh_grave[i] = g_res.mesh_grave.back();
        g_res.mesh_grave.pop_back();
    }
}

// Once per flush. "Idle" means no object carried the id for the whole window. MESH_REF_LOCK - the census may be a
// frame old, so the GC first notes the list itself (under g_draw_list_mutex; skipped while the census stamped under 1 s
// ago, GC_NOTE_SKIP), then reads the table under its own lock, then releases (mesh_write_busy takes g_meshwriter_mu;
// GC_RECHECK takes g_mesh_ref_mu again around the stamp's second read and the tombstone - append_mx; the rest of the
// release, holding neither, g_fbx_cache_mutex and g_physics_bvh_mu). A user taking the mesh meanwhile stamps it first
// and then reads it alive (mesh_take_ref): its stamp lands before that section, which then keeps the mesh, or after
// it, and the user sees the tombstone and resolves again (once). GC_CLOCK:
// the idle test's clock is read under g_mesh_ref_mu, which every g_mesh_last_ref write holds, so no stamp is later than
// it. Do not read it before the lock: a census or import stamp (flush_frame / the importer, game thread) taken on the
// next tick makes now - stamp wrap to a huge age and releases a mesh a live object carries.
inline void mesh_gc() {
    KH_PROF_SCOPE(PROF_MESH_GC);   // PROF.
    // GC_NOTE_SKIP: the census stamps the same references every frame; its notes under 1 s old stand for this walk
    // (the idle windows are 120 s, and 5 s for a generated mesh - MESH_GEN_GC).
    if (GetTickCount64() - g_ref_note_cen_ms.load(std::memory_order_acquire) > 1000ull) {
        std::lock_guard<std::mutex> l(g_draw_list_mutex);
        ref_note_begin(g_ref_note_gc);   // REF_NOTE_ONCE.
        for (const auto& kv : g_draw_list) {
            if (!kv.second.fullscreen) ref_note_add(g_ref_note_gc, kv.second.mesh);
        }
        ref_note_end(g_ref_note_gc);
    }
    const uint32_t n = mesh_count();
    const uint32_t b = mesh_store().builtin_n;
    static std::vector<int> idle;
    idle.clear();
    {
        std::lock_guard<std::mutex> r(g_mesh_ref_mu);
        const uint64_t now = GetTickCount64();   // GC_CLOCK: under the lock.
        for (uint32_t id = b; id < n; ++id) {
            if (!mesh_alive(static_cast<int>(id))) continue;
            const uint64_t ref = id < g_mesh_last_ref.size() ? g_mesh_last_ref[id] : 0;
            const uint64_t age = now - ref;
            if (age < MESH_GEN_GC_IDLE_MS) continue;
            if (age < MESH_GC_IDLE_MS && !mesh_is_generated(mesh_def(static_cast<int>(id)))) continue;
            idle.push_back(static_cast<int>(id));
        }
    }
    for (int id : idle) {
        if (mesh_write_busy(id)) continue;
        MeshDef* d = nullptr;
        {   // GC_RECHECK: the stamp again, just before the release - a user that took the mesh since the walk
            // above (a share hit, a by-name, by-index or by-path assignment: mesh_take_ref) keeps it - and the
            // tombstone in the same section, so no stamp lands between the two.
            std::lock_guard<std::mutex> r(g_mesh_ref_mu);
            const size_t i = static_cast<size_t>(id);
            const uint64_t ref = i < g_mesh_last_ref.size() ? g_mesh_last_ref[i] : 0;
            const uint64_t win = mesh_is_generated(mesh_def(id)) ? MESH_GEN_GC_IDLE_MS
                                                                              : MESH_GC_IDLE_MS;
            if (GetTickCount64() - ref < win) continue;
            d = mesh_tombstone(id);
        }
        mesh_release_def(id, d);
    }
}

// SESSION_RESET - the collector with no idle window: every non-builtin mesh is released (tombstoned, its path key
// forgotten, its buffers and def to the grave g_res.release then frees). A pending publish is landed first so its mesh
// goes too. Ids are never reused, so an id a script kept from an earlier session reads as no mesh rather than as
// another one. Under the park, with the cache writer stopped (mesh_write_busy).
inline void mesh_release_session() {
    mesh_publish();
    g_mesh_publish_pending.store(false, std::memory_order_relaxed);
    {   // MESH_CACHE_LATER: with the pending publish it waited for.
        std::lock_guard<std::mutex> g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.clear();
    }
    const uint32_t n = mesh_count();
    for (uint32_t id = mesh_store().builtin_n; id < n; ++id) {
        if (mesh_alive(static_cast<int>(id))) mesh_release(static_cast<int>(id));
    }
}

// Once per flush: idle material sets go through the grave. References are noted by flush_frame's census and, first, by
// this collector itself. MAT_REF_LOCK - as mesh_gc: the collector runs in flush_locked's housekeeping, on the
// render thread every scene flush, and that flush keeps running while g_track_wanted stands even when flush_frame
// (the census) does not - a Draw3D handler removed by a script leaves it standing. Without its own note, every set a
// live object carries would read idle after MAT_GC_IDLE_MS and be deleted under the objects. The draw list is noted
// under g_draw_list_mutex, then g_mat_pool_mu (the census's order); with the census running this only refreshes what it
// already refreshed. GC_CLOCK: the idle test's clock is read under g_mat_pool_mu, which every last_ref_ms write
// holds, so no stamp is later than it. Do not read it before the lock: a census or intern stamp taken on the next tick
// makes now - stamp wrap to a huge age and graves a set a live object carries.
inline void material_gc() {
    const MaterialSet* pin = default_material_set();   // Interns on first call: before the lock.
    {
        std::lock_guard<std::mutex> d(g_draw_list_mutex);
        std::lock_guard<std::mutex> r(g_mat_pool_mu);
        const uint64_t seen = GetTickCount64();
        for (const auto& kv : g_draw_list) {
            const MaterialSet* s = kv.second.materials;
            if (!s) continue;
            auto it = g_mat_pool.find(s->hash);
            if (it != g_mat_pool.end()) it->second.last_ref_ms = seen;
        }
    }
    std::lock_guard<std::mutex> l(g_mat_pool_mu);
    const uint64_t now = GetTickCount64();   // GC_CLOCK: under the lock.
    for (size_t i = 0; i < g_mat_grave.size();) {
        if (g_flush_serial.load(std::memory_order_relaxed) - g_mat_grave[i].flush_at < MESH_GRAVE_FLUSHES) { ++i; continue; }
        g_mat_grave[i] = std::move(g_mat_grave.back());
        g_mat_grave.pop_back();
    }
    bool moved = false;
    for (auto it = g_mat_pool.begin(); it != g_mat_pool.end();) {
        if (it->second.set.get() == pin ||
            now - it->second.last_ref_ms < MAT_GC_IDLE_MS) { ++it; continue; }
        MatGrave g;
        g.set = std::move(it->second.set);
        g.flush_at = g_flush_serial.load(std::memory_order_relaxed);
        g_mat_grave.push_back(std::move(g));
        it = g_mat_pool.erase(it);
        moved = true;
    }
    if (moved) mat_gpu_rebuild();   // MAT_TABLE: compaction by re-add.
}

// SESSION_RESET - every material set but the pinned default goes, and the default's GPU state with it (its table
// entries named the old pages). No object references a set here: the draw list and the scene are empty.
inline void material_pool_reset() {
    const MaterialSet* pin = default_material_set();   // Interns on first call: before the lock.
    std::lock_guard<std::mutex> l(g_mat_pool_mu);
    g_mat_grave.clear();
    for (auto it = g_mat_pool.begin(); it != g_mat_pool.end();) {
        if (it->second.set.get() == pin) {
            it->second.set->gpu = MaterialSet::Gpu();
            ++it;
        } else {
            it = g_mat_pool.erase(it);
        }
    }
}

// Once per flush: idle texture views go. Failed entries stay (no VRAM; they suppress repeated reports).
inline void tex_cache_gc() {
    const uint64_t now = GetTickCount64();
    bool any = false;
    // A resolve is the only refresh of last_use_ms, so a texture in continuous use reads idle here; before the sweep,
    // touch every entry a pool set still names. Done only when something has aged out.
    {
        bool aged = false;
        for (const auto& kv : g_tex_cache) {
            if (!kv.second.failed && now - kv.second.last_use_ms >= TEX_GC_IDLE_MS) { aged = true; break; }
        }
        if (aged) {
            std::lock_guard<std::mutex> l(g_mat_pool_mu);
            for (const auto& kv : g_mat_pool) {
                for (const Material& m : kv.second.set->slots) {
                    if (!m.used) continue;
                    for (int s = 0; s < MAT_MAPS; ++s) {
                        if (m.maps[s].path.empty()) continue;
                        auto hit = g_tex_cache.find(tex_cache_key(m.maps[s].path,
                                                                  tex_map_srgb(m, s)));
                        if (hit != g_tex_cache.end()) hit->second.last_use_ms = now;
                    }
                }
            }
        }
    }
    for (auto it = g_tex_cache.begin(); it != g_tex_cache.end();) {
        TexEntry& e = it->second;
        if (e.failed || now - e.last_use_ms < TEX_GC_IDLE_MS) { ++it; continue; }
        if (e.page >= 0 && static_cast<size_t>(e.page) < g_tex_pages.size()) {
            TexPage& p = g_tex_pages[e.page];
            if (p.tex && e.layer >= 0 && static_cast<UINT>(e.layer) < p.cap && p.used[e.layer]) {
                p.used[e.layer] = 0;
                if (--p.used_n == 0) { KH_SAFE_RELEASE(p.srv); KH_SAFE_RELEASE(p.tex); p.used.clear(); p.cap = 0; ++g_tex_page_gen; }
            }
        }
        it = g_tex_cache.erase(it);
        stat_inc(g_stats.textures_released);
        any = true;
    }
    if (any) mat_gpu_rebuild();   // MAT_TABLE: entries named freed layers.
}
}