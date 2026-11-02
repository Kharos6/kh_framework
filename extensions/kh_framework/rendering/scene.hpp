#pragma once

// The feature half (rendering_integration_features.hpp): the live scene grid and the object buffer (kh_scene_sync,
// kh_objbuf_sync). Never include it on its own.
namespace RenderIntegration {
inline void kh_cloth_slot_forget(uint32_t khsf_slot);   // KH_CLOTH: a dead slot's substitute (defined in
                                                         // rendering/object_slots.hpp).

// The live scene. g_draw_list is the script-facing authority (under g_draw_list_mutex); the walkers read this
// index-stable copy, synced dirty-only by kh_scene_sync. Every g_draw_list mutation marks its slot dirty: add /
// update / remove / clear, the expiry erases (the flush's and flush_frame's census), kh_attach_reap's erase, and the
// in-place writes of the attachment step, the cloth / chain refits and the skin upload; a slot is reused only after
// the live side has seen its death. The park serializes the readers, so the live vectors carry no lock. The pre-lock
// game-thread walks (flush_frame's demand census, kh_cloth_sync, flush_ui_frame's has_work) stay on g_draw_list under
// the mutex, and none of them may call kh_scene_sync: outside the park the render thread can be walking these
// vectors, and the sync resizes them and rehashes the grid.
static constexpr uint32_t KH_SCENE_NONE = 0xFFFFFFFFu;
static std::vector<uint8_t>       g_scene_dirty_mark;
static std::vector<uint32_t>      g_scene_dirty;
static std::vector<uint32_t>      g_scene_free;   // Slots whose death the live side has seen.
// KH_SYNC_IDLE: g_scene_dirty's size, readable without the lock. Written only under g_draw_list_mutex (the mark
// adds, the sync clears); a sync that reads 0 takes no lock.
static std::atomic<uint32_t>      g_scene_dirty_n{ 0 };

inline void kh_scene_mark(uint32_t khsm_slot) {   // Under g_draw_list_mutex.
    if (khsm_slot >= g_scene_dirty_mark.size()) g_scene_dirty_mark.resize(khsm_slot + 1, 0);
    if (g_scene_dirty_mark[khsm_slot]) return;
    g_scene_dirty.push_back(khsm_slot);   // KH_SCENE_PAIR: listed first - a failed push leaves the slot unmarked.
    g_scene_dirty_mark[khsm_slot] = 1;
    g_scene_dirty_n.store(static_cast<uint32_t>(g_scene_dirty.size()), std::memory_order_release);   // KH_SYNC_IDLE.
}

inline uint32_t kh_scene_slot_alloc() {   // Under g_draw_list_mutex.
    uint32_t khsa_s;
    if (!g_scene_free.empty()) { khsa_s = g_scene_free.back(); g_scene_free.pop_back(); }
    else {
        khsa_s = static_cast<uint32_t>(g_scene_stage.size());
        // KH_SCENE_PAIR: the two tables grow as one - a failed second growth takes the first back, so no slot
        // is in the stage alone (its readers index the handles by the stage's size).
        g_scene_stage.push_back(nullptr);
        try {
            g_scene_handle.emplace_back();
        } catch (...) {
            g_scene_stage.pop_back();
            throw;
        }
    }
    return khsa_s;
}

inline uint64_t kh_scene_cell_key(const RenderObject& o) {
    if (o.fullscreen) return KH_SCENE_CELL_NONE;
    // Clamped before the cast (2^30 cells, 6.9e10 m; a NaN takes the low end): no index is INT32_MIN and no
    // conversion leaves int32's range.
    const float khck_lim = 1073741824.0f;
    const int32_t khck_x = static_cast<int32_t>(fminf(fmaxf(floorf(o.pos[0] / KH_SCENE_CELL_M), -khck_lim), khck_lim));
    const int32_t khck_z = static_cast<int32_t>(fminf(fmaxf(floorf(o.pos[1] / KH_SCENE_CELL_M), -khck_lim),
                                                      khck_lim));   // SQF y = engine z.
    return (static_cast<uint64_t>(static_cast<uint32_t>(khck_x)) << 32) | static_cast<uint32_t>(khck_z);
}

inline void kh_scene_grid_remove(uint32_t khgr_slot) {
    const uint64_t khgr_k = g_scene.cell[khgr_slot];
    if (khgr_k == KH_SCENE_CELL_NONE) return;
    g_scene.cell[khgr_slot] = KH_SCENE_CELL_NONE;
    auto khgr_it = g_scene.grid.find(khgr_k);
    if (khgr_it == g_scene.grid.end()) return;
    std::vector<uint32_t>& khgr_v = khgr_it->second.slots;
    for (size_t khgr_i = 0; khgr_i < khgr_v.size(); ++khgr_i) {
        if (khgr_v[khgr_i] == khgr_slot) { khgr_v[khgr_i] = khgr_v.back(); khgr_v.pop_back(); break; }
    }
    if (khgr_v.empty()) g_scene.grid.erase(khgr_it);
}

// KH_GRID_KEEP: khgi_filed = the slot is already in the cell its key names (kh_scene_sync's unchanged cell) - its
// bounds are taken as the remove + insert gave them (a one-slot cell made afresh, else grown) and nothing is
// pushed. g_scene.cell names the cell only once the slot is in its vector, so "filed" holds after a throw.
inline void kh_scene_grid_insert(uint32_t khgi_slot, const RenderObject& o, bool khgi_filed = false) {
    const uint64_t khgi_k = kh_scene_cell_key(o);
    if (khgi_k == KH_SCENE_CELL_NONE) { g_scene.cell[khgi_slot] = khgi_k; return; }
    if (o.mesh >= 0 && static_cast<uint32_t>(o.mesh) >= mesh_count() &&   // KH_LOD_BOUNDS.
        static_cast<uint32_t>(o.mesh) < g_scene_unpub_min) g_scene_unpub_min = static_cast<uint32_t>(o.mesh);
    float khgi_he[3];
    kh_world_half_extents(o, khgi_he);
    const float khgi_r = sqrtf(khgi_he[0] * khgi_he[0] + khgi_he[1] * khgi_he[1] + khgi_he[2] * khgi_he[2]);
    const float khgi_y = o.pos[2];   // Engine y = SQF zASL.
    auto khgi_it = g_scene.grid.find(khgi_k);
    bool khgi_push = true;
    if (khgi_it == g_scene.grid.end()) {
        KhSceneCell khgi_c;
        khgi_c.ymin = khgi_y; khgi_c.ymax = khgi_y; khgi_c.rmax = khgi_r;
        khgi_it = g_scene.grid.emplace(khgi_k, std::move(khgi_c)).first;
    } else if (khgi_filed && khgi_it->second.slots.size() == 1u && khgi_it->second.slots[0] == khgi_slot) {
        KhSceneCell& khgi_c = khgi_it->second;   // KH_GRID_KEEP: as the remove (the cell erased) and insert.
        khgi_c.ymin = khgi_y; khgi_c.ymax = khgi_y; khgi_c.rmax = khgi_r;
        khgi_push = false;
    } else {
        KhSceneCell& khgi_c = khgi_it->second;
        if (khgi_y < khgi_c.ymin) khgi_c.ymin = khgi_y;
        if (khgi_y > khgi_c.ymax) khgi_c.ymax = khgi_y;
        if (khgi_r > khgi_c.rmax) khgi_c.rmax = khgi_r;
        if (khgi_filed) khgi_push = false;   // KH_GRID_KEEP.
    }
    if (khgi_push) khgi_it->second.slots.push_back(khgi_slot);
    g_scene.cell[khgi_slot] = khgi_k;   // KH_GRID_KEEP: after the push.
}

// GPU object record, HLSL twin KhObjRec (t39). One per live-scene slot, engine axes: what a bucket's instanced VS
// needs per object. Rebuilt in kh_scene_sync per dirty slot, uploaded as ranges by kh_objbuf_sync.
struct KhObjRec {
    float pos[4];    // xyz = centre (engine axes); w = 1 with receiveShadow off (KH_SHADOW_SWITCH) or every
                     // shadow off (KH_SHADOW_OFF, g_objbuf_shoff), else 0.
    float size[4];   // xyz = edge lengths (engine axes); w = creation (s, the session clock; KH_USER_LANES).
    float rot0[4];   // Rotation rows (row-vector; identity when unrotated); rot0.w = 1
    float rot1[4];   // (filled), rot1.w = ambient fraction, rot2.w = diffuse fraction.
    float rot2[4];
    float col[4];    // Object colour, envelope not applied.
    float res[4];    // xyz = KH_POS_RES (engine axes; added to every relative centre the record forms); w =
                     // KH_SKIN_POOL: a skeletal binding's vertex base in its chunk (exact; 0 for any other).
};
static_assert(sizeof(KhObjRec) == 112, "KhObjRec is 7 float4 (HLSL twin)");
static std::vector<KhObjRec> g_objbuf_cpu;          // By slot; the buffer's mirror.
static std::vector<uint32_t> g_objbuf_dirty;        // Slots the GPU copy is behind on.
static std::vector<uint8_t>  g_objbuf_dirty_mark;
// KH_SHADOW_OFF as the records hold it (under the park, as the mirror): kh_objbuf_sync takes the switch and
// re-derives every record's pos.w when it moves, so the records agree; the per-draw receive lane
// (fill_lighting_obj_cb) reads this copy too.
static bool g_objbuf_shoff = false;

inline void kh_objrec_fill(KhObjRec& r, const RenderObject& o) {
    r.pos[0] = o.pos[0]; r.pos[1] = o.pos[2]; r.pos[2] = o.pos[1];   // SQF [x,y,zASL] -> engine [x,zASL,y].
    r.pos[3] = (o.receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;   // KH_SHADOW_SWITCH / KH_SHADOW_OFF (khObjNoRecv).
    r.size[0] = o.size[0]; r.size[1] = o.size[2]; r.size[2] = o.size[1];
    r.size[3] = static_cast<float>(o.fx_t0);   // KH_USER_LANES: creation on the session clock (khUserObj.y's twin).
    for (int rr = 0; rr < 3; ++rr) {
        float* khof_row = rr == 0 ? r.rot0 : rr == 1 ? r.rot1 : r.rot2;
        khof_row[0] = o.rot_m[rr * 3 + 0];
        khof_row[1] = o.rot_m[rr * 3 + 1];
        khof_row[2] = o.rot_m[rr * 3 + 2];
    }
    r.rot0[3] = 1.0f;
    r.rot1[3] = o.light_ambient;
    r.rot2[3] = o.light_diffuse;
    memcpy(r.col, o.color, sizeof(r.col));
    kh_pos_res(o, r.res);   // KH_POS_RES.
    r.res[3] = o.skel ? static_cast<float>(o.skin_vbase) : 0.0f;   // KH_SKIN_POOL: below 2^24 (64 MB chunks).
}

inline void kh_objbuf_mark(uint32_t khom_slot) {   // Under the park (kh_scene_sync).
    if (khom_slot >= g_objbuf_dirty_mark.size()) g_objbuf_dirty_mark.resize(khom_slot + 1, 0);
    if (g_objbuf_dirty_mark[khom_slot]) return;
    g_objbuf_dirty.push_back(khom_slot);   // KH_SCENE_PAIR: listed first, as kh_scene_mark.
    g_objbuf_dirty_mark[khom_slot] = 1;
}

// KH_SCENE_HOLD - while set, kh_scene_sync takes nothing new: flush_locked raises it once its walk has copied the
// objects it draws, so every pass it runs after that (the DLS select's caster walk, the sun ladder, the DLS maps)
// and the records its buckets index (kh_objbuf_sync) are of the scene its copies were culled, keyed and ordered in
// (the injection's KH_OBJBUF_LATE rule). A stage written meanwhile stays dirty for the next sync. Render thread or
// park, as the sync (KhSceneHold scopes it).
static bool g_scene_hold = false;
struct KhSceneHold {
    KhSceneHold() { g_scene_hold = true; }
    ~KhSceneHold() { g_scene_hold = false; }
    KhSceneHold(const KhSceneHold&) = delete;
    KhSceneHold& operator=(const KhSceneHold&) = delete;
};

// The dirty-only sync. Under the park invariant on whichever thread walks next; takes g_draw_list_mutex for the copy
// only. O(dirty), O(1) when idle.
// KH_SUN_GEN: an object a sync takes that can be one of the sun ladder's casters in some view - kh_shadow_active
// less the terms that move between syncs (visibility, the view-model slice, the shadow switch), so a change that
// matters to the ladder is never missed; one that only might is counted.
inline bool kh_scene_cast_kind(const RenderObject& khck_o) {
    return khck_o.cast_shadow && !khck_o.fullscreen && khck_o.effect == 0 && khck_o.lit &&
           khck_o.mode != DepthMode::Off;
}

inline void kh_scene_sync() {
    KH_PROF_SCOPE(KHP_SCENE_SYNC);   // KH_PROF.
    if (g_scene_hold) return;   // KH_SCENE_HOLD.
    if (g_scene_dirty_n.load(std::memory_order_acquire) == 0u) return;   // KH_SYNC_IDLE: no lock when nothing is dirty.
    std::lock_guard<std::mutex> khss_l(g_draw_list_mutex);
    if (g_scene_dirty.empty()) return;
    const size_t khss_n = g_scene_stage.size();
    // KH_SCENE_PAIR: each table on its own length - a growth that threw part-way is finished by the next sync -
    // and objs last: the readers walk objs.size() and index alive / cell with it.
    if (g_scene.alive.size() < khss_n) g_scene.alive.resize(khss_n, 0);
    if (g_scene.cell.size() < khss_n) g_scene.cell.resize(khss_n, KH_SCENE_CELL_NONE);
    if (g_scene.objs.size() < khss_n) g_scene.objs.resize(khss_n);
    if (g_objbuf_cpu.size() < khss_n) g_objbuf_cpu.resize(khss_n, KhObjRec());
    bool khss_cast = false;   // KH_SUN_GEN: a caster (before or after) taken.
    for (uint32_t khss_s : g_scene_dirty) {
        // KH_SCENE_PAIR: a slot is unmarked once done, and one listed but unmarked is passed over - a sync that threw
        // part-way leaves its done slots listed (one marked again since, listed twice; a dead one taken again would be
        // freed twice); the slot it threw on is still marked and is taken again from the top.
        if (!g_scene_dirty_mark[khss_s]) continue;
        const RenderObject* khss_src = g_scene_stage[khss_s];
        // KH_GRID_KEEP: a live slot whose cell is unchanged (an attached mesh moving within its 64 m cell, an
        // update) stays filed - the remove scanned the whole cell, so N movers sharing one cost N^2 / 2 a sync.
        const bool khss_keep = khss_src && g_scene.alive[khss_s] &&
                               g_scene.cell[khss_s] != KH_SCENE_CELL_NONE &&
                               kh_scene_cell_key(*khss_src) == g_scene.cell[khss_s];
        if (!khss_keep) kh_scene_grid_remove(khss_s);
        if (g_scene.alive[khss_s] && kh_scene_cast_kind(g_scene.objs[khss_s])) khss_cast = true;   // KH_SUN_GEN.
        if (khss_src) {
            if (kh_scene_cast_kind(*khss_src)) khss_cast = true;   // KH_SUN_GEN.
            g_scene.objs[khss_s] = *khss_src;
            if (!g_scene.alive[khss_s]) { g_scene.alive[khss_s] = 1; ++g_scene.alive_n; }
            kh_scene_grid_insert(khss_s, g_scene.objs[khss_s], khss_keep);
            kh_objrec_fill(g_objbuf_cpu[khss_s], g_scene.objs[khss_s]);   // KH_OBJBUF.
            kh_objbuf_mark(khss_s);
        } else if (g_scene.alive[khss_s]) {
            g_scene.alive[khss_s] = 0; --g_scene.alive_n;
            kh_cloth_slot_forget(khss_s);   // KH_CLOTH: never handed out still naming a cloth buffer.
            g_scene_free.push_back(khss_s);
        } else {
            kh_cloth_slot_forget(khss_s);
            g_scene_free.push_back(khss_s);   // Added and removed between two syncs.
        }
        g_scene_dirty_mark[khss_s] = 0;   // KH_SCENE_PAIR: done.
    }
    g_scene_dirty.clear();
    g_scene_dirty_n.store(0u, std::memory_order_release);   // KH_SYNC_IDLE.
    ++g_scene.gen;
    if (khss_cast) ++g_scene.gen_cast;   // KH_SUN_GEN.
}

// Brings the GPU buffer up to the mirror after kh_scene_sync, before any bucket draw: the render thread - the
// injection, the mirror and PIP passes (inside a draw hook: device Create* only here), the flush and the sun tiers
// at the scene resolve - or the game thread where a park runs the flush (KH_OBJBUF; g_res.obj_wait is Resources',
// release() resets it). Grows by doubling; otherwise one UpdateSubresource per dirty run (whole upload past half).
// Every write carries a box. False = no buffer.
// KH_OBJBUF_RETRY: a buffer the device refused is asked for again KH_OBJBUF_RETRY_CALLS calls on (about two seconds
// at ~5 calls a frame and 60 fps, as KH_SKIN_GPU_RETRY waits), not on every call; until then the call returns false
// at once (no buffer: every bucket route draws per object).
static constexpr uint32_t KH_OBJBUF_RETRY_CALLS = 600u;
inline bool kh_objbuf_sync(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_OBJBUF_SYNC);   // KH_PROF.
    const uint32_t khob_n = static_cast<uint32_t>(g_objbuf_cpu.size());
    if (khob_n == 0 || !ctx) return false;
    bool khob_whole = false;
    // KH_SHADOW_OFF: the switch moved - every record's receive lane again, and the buffer goes up whole.
    if (kh_shadows_off() != g_objbuf_shoff) {
        g_objbuf_shoff = !g_objbuf_shoff;
        const uint32_t khob_m = khob_n < g_scene.objs.size() ? khob_n : static_cast<uint32_t>(g_scene.objs.size());
        for (uint32_t khob_s = 0; khob_s < khob_m; ++khob_s)
            g_objbuf_cpu[khob_s].pos[3] = (g_scene.objs[khob_s].receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;
        khob_whole = true;
    }
    if (!g_res.obj_sb || g_res.obj_cap < khob_n) {
        if (g_res.obj_wait > 0u) { --g_res.obj_wait; return false; }   // KH_OBJBUF_RETRY.
        ID3D11Device* khob_dev = nullptr;
        ctx->GetDevice(&khob_dev);
        if (!khob_dev) return false;
        KH_SAFE_RELEASE(g_res.obj_srv);
        KH_SAFE_RELEASE(g_res.obj_sb);
        uint32_t khob_cap = g_res.obj_cap > 256 ? g_res.obj_cap : 256;
        while (khob_cap < khob_n) khob_cap *= 2;
        D3D11_BUFFER_DESC khob_bd = {};
        khob_bd.ByteWidth = khob_cap * sizeof(KhObjRec);
        khob_bd.Usage = D3D11_USAGE_DEFAULT;
        khob_bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        khob_bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        khob_bd.StructureByteStride = sizeof(KhObjRec);
        const HRESULT khob_hr = khob_dev->CreateBuffer(&khob_bd, nullptr, &g_res.obj_sb);
        if (SUCCEEDED(khob_hr)) {
            D3D11_SHADER_RESOURCE_VIEW_DESC khob_sd = {};
            khob_sd.Format = DXGI_FORMAT_UNKNOWN;
            khob_sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            khob_sd.Buffer.FirstElement = 0;
            khob_sd.Buffer.NumElements = khob_cap;
            if (FAILED(khob_dev->CreateShaderResourceView(g_res.obj_sb, &khob_sd, &g_res.obj_srv))) {
                KH_SAFE_RELEASE(g_res.obj_sb);
                g_res.obj_srv = nullptr;
            }
        } else {
            g_res.obj_sb = nullptr;
        }
        khob_dev->Release();
        if (!g_res.obj_srv) {
            g_res.obj_cap = 0;
            g_res.obj_wait = KH_OBJBUF_RETRY_CALLS;   // KH_OBJBUF_RETRY.
            return false;
        }
        g_res.obj_cap = khob_cap;
        khob_whole = true;
    }
    auto khob_put = [&](uint32_t khob_a, uint32_t khob_b) {   // [a, b) slots.
        D3D11_BOX khob_box = {};
        khob_box.left = khob_a * sizeof(KhObjRec);
        khob_box.right = khob_b * sizeof(KhObjRec);
        khob_box.top = 0; khob_box.bottom = 1; khob_box.front = 0; khob_box.back = 1;
        ctx->UpdateSubresource(g_res.obj_sb, 0, &khob_box, g_objbuf_cpu.data() + khob_a, 0, 0);
    };
    if (khob_whole || g_objbuf_dirty.size() * 2 > khob_n) {
        khob_put(0, khob_n);
    } else if (!g_objbuf_dirty.empty()) {
        std::sort(g_objbuf_dirty.begin(), g_objbuf_dirty.end());
        uint32_t khob_a = g_objbuf_dirty[0], khob_b = khob_a + 1;
        for (size_t khob_i = 1; khob_i < g_objbuf_dirty.size(); ++khob_i) {
            const uint32_t khob_s = g_objbuf_dirty[khob_i];
            if (khob_s == khob_b) { ++khob_b; continue; }
            if (khob_s == khob_b - 1) continue;   // Duplicate (never, by the mark).
            khob_put(khob_a, khob_b);
            khob_a = khob_s; khob_b = khob_s + 1;
        }
        khob_put(khob_a, khob_b);
    }
    for (uint32_t khob_s : g_objbuf_dirty) g_objbuf_dirty_mark[khob_s] = 0;
    g_objbuf_dirty.clear();
    return true;
}

inline void kh_objbuf_bind(ID3D11DeviceContext* ctx) {   // VS t39 (StateBackup restores it).
    ctx->VSSetShaderResources(39, 1, &g_res.obj_srv);
}

// Under g_draw_list_mutex: the slot's map entry is going away.
inline void kh_scene_stage_remove(const RenderObject& o) {
    if (o.slot == KH_SCENE_NONE || o.slot >= g_scene_stage.size()) return;
    g_scene_stage[o.slot] = nullptr;
    g_scene_handle[o.slot].clear();
    kh_scene_mark(o.slot);
}
}