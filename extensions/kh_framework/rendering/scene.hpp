#pragma once

// The feature half (rendering_integration_features.hpp): the live scene grid and the object buffer (scene_sync,
// objbuf_sync). Never include it on its own.
namespace RenderIntegration {
inline void cloth_slot_forget(uint32_t slot);   // CLOTH: a dead slot's substitute (defined in
                                                         // rendering/object_slots.hpp).

// The live scene. g_draw_list is the script-facing authority (under g_draw_list_mutex); the walkers read this
// index-stable copy, synced dirty-only by scene_sync. Every g_draw_list mutation marks its slot dirty: add /
// update / remove / clear, the expiry erases (the flush's and flush_frame's census), attach_reap's erase, and the
// in-place writes of the attachment step, the cloth / chain refits and the skin upload; a slot is reused only after
// the live side has seen its death. The park serializes the readers, so the live vectors carry no lock. The pre-lock
// game-thread walks (flush_frame's demand census, cloth_sync, flush_ui_frame's has_work) stay on g_draw_list under
// the mutex, and none of them may call scene_sync: outside the park the render thread can be walking these
// vectors, and the sync resizes them and rehashes the grid.
static constexpr uint32_t SCENE_NONE = 0xFFFFFFFFu;
static std::vector<uint8_t>       g_scene_dirty_mark;
static std::vector<uint32_t>      g_scene_dirty;
static std::vector<uint32_t>      g_scene_free;   // Slots whose death the live side has seen.
// SYNC_IDLE: g_scene_dirty's size, readable without the lock. Written only under g_draw_list_mutex (the mark
// adds, the sync clears all but the slots it holds, SCENE_UNPUB); a sync that reads 0 takes no lock.
static std::atomic<uint32_t>      g_scene_dirty_n{ 0 };

inline void scene_mark(uint32_t slot) {   // Under g_draw_list_mutex.
    if (slot >= g_scene_dirty_mark.size()) g_scene_dirty_mark.resize(slot + 1, 0);
    if (g_scene_dirty_mark[slot]) return;
    g_scene_dirty.push_back(slot);   // SCENE_PAIR: listed first - a failed push leaves the slot unmarked.
    g_scene_dirty_mark[slot] = 1;
    g_scene_dirty_n.store(static_cast<uint32_t>(g_scene_dirty.size()), std::memory_order_release);   // SYNC_IDLE.
}

inline uint32_t scene_slot_alloc() {   // Under g_draw_list_mutex.
    uint32_t s;
    if (!g_scene_free.empty()) { s = g_scene_free.back(); g_scene_free.pop_back(); }
    else {
        s = static_cast<uint32_t>(g_scene_stage.size());
        // SCENE_PAIR: the two tables grow as one - a failed second growth takes the first back, so no slot
        // is in the stage alone (its readers index the handles by the stage's size).
        g_scene_stage.push_back(nullptr);
        try {
            g_scene_handle.emplace_back();
        } catch (...) {
            g_scene_stage.pop_back();
            throw;
        }
    }
    return s;
}

inline uint64_t scene_cell_key(const RenderObject& o) {
    if (o.fullscreen) return SCENE_CELL_NONE;
    // Clamped before the cast (2^30 cells, 6.9e10 m; a NaN takes the low end): no index is INT32_MIN and no
    // conversion leaves int32's range.
    const float lim = 1073741824.0f;
    const int32_t x = static_cast<int32_t>(fminf(fmaxf(floorf(o.pos[0] / SCENE_CELL_M), -lim), lim));
    const int32_t z = static_cast<int32_t>(fminf(fmaxf(floorf(o.pos[1] / SCENE_CELL_M), -lim),
                                                 lim));   // SQF y = engine z.
    return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(z);
}

inline void scene_grid_remove(uint32_t slot) {
    const uint64_t k = g_scene.cell[slot];
    if (k == SCENE_CELL_NONE) return;
    g_scene.cell[slot] = SCENE_CELL_NONE;
    auto it = g_scene.grid.find(k);
    if (it == g_scene.grid.end()) return;
    std::vector<uint32_t>& v = it->second.slots;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i] == slot) { v[i] = v.back(); v.pop_back(); break; }
    }
    if (v.empty()) g_scene.grid.erase(it);
}

// GRID_KEEP: filed = the slot is already in the cell its key names (scene_sync's unchanged cell) - its
// bounds are taken as the remove + insert gave them (a one-slot cell made afresh, else grown) and nothing is
// pushed. g_scene.cell names the cell only once the slot is in its vector, so "filed" holds after a throw.
inline void scene_grid_insert(uint32_t slot, const RenderObject& o, bool filed = false) {
    const uint64_t k = scene_cell_key(o);
    if (k == SCENE_CELL_NONE) { g_scene.cell[slot] = k; return; }
    if (o.mesh >= 0 && static_cast<uint32_t>(o.mesh) >= mesh_count() &&   // LOD_BOUNDS.
        static_cast<uint32_t>(o.mesh) < g_scene_unpub_min) g_scene_unpub_min = static_cast<uint32_t>(o.mesh);
    float he[3];
    world_half_extents(o, he);
    const float r = sqrtf(he[0] * he[0] + he[1] * he[1] + he[2] * he[2]);
    const float y = o.pos[2];   // Engine y = SQF zASL.
    auto it = g_scene.grid.find(k);
    bool push = true;
    if (it == g_scene.grid.end()) {
        SceneCell c;
        c.ymin = y; c.ymax = y; c.rmax = r;
        it = g_scene.grid.emplace(k, std::move(c)).first;
    } else if (filed && it->second.slots.size() == 1u && it->second.slots[0] == slot) {
        SceneCell& c = it->second;   // GRID_KEEP: as the remove (the cell erased) and insert.
        c.ymin = y; c.ymax = y; c.rmax = r;
        push = false;
    } else {
        SceneCell& c = it->second;
        if (y < c.ymin) c.ymin = y;
        if (y > c.ymax) c.ymax = y;
        if (r > c.rmax) c.rmax = r;
        if (filed) push = false;   // GRID_KEEP.
    }
    if (push) it->second.slots.push_back(slot);
    g_scene.cell[slot] = k;   // GRID_KEEP: after the push.
}

// GPU object record, HLSL twin KhObjRec (t39). One per live-scene slot, engine axes: what a bucket's instanced VS
// needs per object. Rebuilt in scene_sync per dirty slot, uploaded as ranges by objbuf_sync.
struct KhObjRec {
    float pos[4];    // xyz = centre (engine axes); w = 1 with receiveShadow off (SHADOW_SWITCH) or every
                     // shadow off (SHADOW_OFF, g_objbuf_shoff), else 0.
    float size[4];   // xyz = edge lengths (engine axes); w = creation (s, the session clock; USER_LANES).
    float rot0[4];   // Rotation rows (row-vector; identity when unrotated); rot0.w = 1
    float rot1[4];   // (filled), rot1.w = ambient fraction, rot2.w = diffuse fraction.
    float rot2[4];
    float col[4];    // Object colour, envelope not applied.
    float res[4];    // xyz = POS_RES (engine axes; added to every relative centre the record forms); w =
                     // SKIN_POOL: a skeletal binding's vertex base in its chunk (exact; 0 for any other).
};
static_assert(sizeof(KhObjRec) == 112, "KhObjRec is 7 float4 (HLSL twin)");
static std::vector<KhObjRec> g_objbuf_cpu;          // By slot; the buffer's mirror.
static std::vector<uint32_t> g_objbuf_dirty;        // Slots the GPU copy is behind on.
static std::vector<uint8_t>  g_objbuf_dirty_mark;
// SHADOW_OFF as the records hold it (under the park, as the mirror): objbuf_sync takes the switch and
// re-derives every record's pos.w when it moves, so the records agree; the per-draw receive lane
// (fill_lighting_obj_cb) reads this copy too.
static bool g_objbuf_shoff = false;

inline void objrec_fill(KhObjRec& r, const RenderObject& o) {
    r.pos[0] = o.pos[0]; r.pos[1] = o.pos[2]; r.pos[2] = o.pos[1];   // SQF [x,y,zASL] -> engine [x,zASL,y].
    r.pos[3] = (o.receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;   // SHADOW_SWITCH / SHADOW_OFF (khObjNoRecv).
    r.size[0] = o.size[0]; r.size[1] = o.size[2]; r.size[2] = o.size[1];
    r.size[3] = static_cast<float>(o.fx_t0);   // USER_LANES: creation on the session clock (khUserObj.y's twin).
    for (int rr = 0; rr < 3; ++rr) {
        float* row = rr == 0 ? r.rot0 : rr == 1 ? r.rot1 : r.rot2;
        row[0] = o.rot_m[rr * 3 + 0];
        row[1] = o.rot_m[rr * 3 + 1];
        row[2] = o.rot_m[rr * 3 + 2];
    }
    r.rot0[3] = 1.0f;
    r.rot1[3] = o.light_ambient;
    r.rot2[3] = o.light_diffuse;
    memcpy(r.col, o.color, sizeof(r.col));
    obj_pos_res(o, r.res);   // POS_RES.
    r.res[3] = o.skel ? static_cast<float>(o.skin_vbase) : 0.0f;   // SKIN_POOL: below 2^24 (64 MB chunks).
}

inline void objbuf_mark(uint32_t slot) {   // Under the park (scene_sync).
    if (slot >= g_objbuf_dirty_mark.size()) g_objbuf_dirty_mark.resize(slot + 1, 0);
    if (g_objbuf_dirty_mark[slot]) return;
    g_objbuf_dirty.push_back(slot);   // SCENE_PAIR: listed first, as scene_mark.
    g_objbuf_dirty_mark[slot] = 1;
}

// SCENE_HOLD - while set, scene_sync takes nothing new: flush_locked raises it once its walk has copied the
// objects it draws, so every pass it runs after that (the DLS select's caster walk, the sun ladder, the DLS maps)
// and the records its buckets index (objbuf_sync) are of the scene its copies were culled, keyed and ordered in
// (the injection's OBJBUF_LATE rule). A stage written meanwhile stays dirty for the next sync. Render thread or
// park, as the sync (SceneHold scopes it).
static bool g_scene_hold = false;
struct SceneHold {
    SceneHold() { g_scene_hold = true; }
    ~SceneHold() { g_scene_hold = false; }
    SceneHold(const SceneHold&) = delete;
    SceneHold& operator=(const SceneHold&) = delete;
};

// The dirty-only sync. Under the park invariant on whichever thread walks next; takes g_draw_list_mutex for the copy
// only. O(dirty), O(1) when idle - and O(held) on each call while an object waits for its mesh's publish
// (SCENE_UNPUB, below: a frame at most).
// SUN_GEN: an object a sync takes that can be one of the sun ladder's casters in some view - shadow_active
// less the terms that move between syncs (visibility, the view-model slice, the shadow switch), so a change that
// matters to the ladder is never missed; one that only might is counted.
inline bool scene_cast_kind(const RenderObject& o) {
    return o.cast_shadow && !o.fullscreen && o.effect == 0 && o.lit &&
           o.mode != DepthMode::Off;
}

inline void scene_sync() {
    KH_PROF_SCOPE(PROF_SCENE_SYNC);   // PROF.
    if (g_scene_hold) return;   // SCENE_HOLD.
    if (g_scene_dirty_n.load(std::memory_order_acquire) == 0u) return;   // SYNC_IDLE: no lock when nothing is dirty.
    std::lock_guard<std::mutex> l(g_draw_list_mutex);
    if (g_scene_dirty.empty()) return;
    const size_t n = g_scene_stage.size();
    // SCENE_PAIR: each table on its own length - a growth that threw part-way is finished by the next sync -
    // and objs last: the readers walk objs.size() and index alive / cell with it.
    if (g_scene.alive.size() < n) g_scene.alive.resize(n, 0);
    if (g_scene.cell.size() < n) g_scene.cell.resize(n, SCENE_CELL_NONE);
    if (g_scene.objs.size() < n) g_scene.objs.resize(n);
    if (g_objbuf_cpu.size() < n) g_objbuf_cpu.resize(n, KhObjRec());
    bool ss_cast = false;   // SUN_GEN: a caster (before or after) taken.
    // SCENE_UNPUB: a staged object whose mesh is appended but not yet published (an import or a
    // generated mesh - each until the next injection or flush lands it, MESH_GEN_PUB) is not taken:
    // every draw would clamp the id to mesh 0 (the box, at the new mesh's size) until then. Its slot stays marked and
    // listed, moved to the front in order, and the scene keeps what it had - nothing for a new object, the last synced
    // state for a changed one - until a sync after the publish. The in-place compaction keeps SCENE_PAIR: an entry
    // is overwritten only once passed (taken, unmarked, or itself held and copied nearer the front), so a throw leaves
    // every marked slot listed.
    const uint32_t pub = mesh_count();
    size_t w = 0;   // SCENE_UNPUB: the slots held so far.
    bool ss_took = false;
    // SCENE_UNPUB: a sync that throws after taking a slot has changed the live scene, and the next sync may only
    // hold (no bump), so the unwind bumps the generations here.
    struct SsGen {
        const bool& took; const bool& cast; bool armed;
        ~SsGen() { if (armed && took) { ++g_scene.gen; if (cast) ++g_scene.gen_cast; } }
    } gen{ ss_took, ss_cast, true };
    for (size_t i = 0; i < g_scene_dirty.size(); ++i) {
        const uint32_t s = g_scene_dirty[i];
        // SCENE_PAIR: a slot is unmarked once done, and one listed but unmarked is passed over - a sync that threw
        // part-way leaves its done slots listed (one marked again since, listed twice; a dead one taken again would be
        // freed twice); the slot it threw on is still marked and is taken again from the top.
        if (!g_scene_dirty_mark[s]) continue;
        const RenderObject* src = g_scene_stage[s];
        if (src && src->mesh >= 0 && static_cast<uint32_t>(src->mesh) >= pub) {
            g_scene_dirty[w++] = s;   // SCENE_UNPUB: held, still marked.
            continue;
        }
        ss_took = true;
        // GRID_KEEP: a live slot whose cell is unchanged (an attached mesh moving within its 64 m cell, an
        // update) stays filed - the remove scanned the whole cell, so N movers sharing one cost N^2 / 2 a sync.
        const bool keep = src && g_scene.alive[s] &&
                          g_scene.cell[s] != SCENE_CELL_NONE &&
                          scene_cell_key(*src) == g_scene.cell[s];
        if (!keep) scene_grid_remove(s);
        if (g_scene.alive[s] && scene_cast_kind(g_scene.objs[s])) ss_cast = true;   // SUN_GEN.
        if (src) {
            if (scene_cast_kind(*src)) ss_cast = true;   // SUN_GEN.
            g_scene.objs[s] = *src;
            if (!g_scene.alive[s]) { g_scene.alive[s] = 1; ++g_scene.alive_n; }
            scene_grid_insert(s, g_scene.objs[s], keep);
            objrec_fill(g_objbuf_cpu[s], g_scene.objs[s]);   // OBJBUF.
            objbuf_mark(s);
        } else if (g_scene.alive[s]) {
            g_scene.alive[s] = 0; --g_scene.alive_n;
            cloth_slot_forget(s);   // CLOTH: never handed out still naming a cloth buffer.
            g_scene_free.push_back(s);
        } else {
            cloth_slot_forget(s);
            g_scene_free.push_back(s);   // Added and removed between two syncs.
        }
        g_scene_dirty_mark[s] = 0;   // SCENE_PAIR: done.
    }
    gen.armed = false;   // No throw: the bump below.
    // SCENE_UNPUB: a slot listed twice (a throw's leftover, SCENE_PAIR) is kept once - as the old full sync
    // emptied the list - by a pass that cannot throw (mark 2 = kept this pass, put back to 1 after).
    size_t k = 0;
    for (size_t i = 0; i < w; ++i) {
        const uint32_t s = g_scene_dirty[i];
        if (g_scene_dirty_mark[s] != 1u) continue;
        g_scene_dirty_mark[s] = 2u;
        g_scene_dirty[k++] = s;
    }
    for (size_t i = 0; i < k; ++i) g_scene_dirty_mark[g_scene_dirty[i]] = 1u;
    w = k;
    g_scene_dirty.resize(w);   // SCENE_UNPUB: the held slots (none: emptied, as before).
    g_scene_dirty_n.store(static_cast<uint32_t>(w), std::memory_order_release);   // SYNC_IDLE.
    if (w != 0 && !ss_took) return;   // SCENE_UNPUB: only holds - nothing changed.
    ++g_scene.gen;
    if (ss_cast) ++g_scene.gen_cast;   // SUN_GEN.
}

// Brings the GPU buffer up to the mirror after scene_sync, before any bucket draw: the render thread - the
// injection, the mirror and PIP passes (inside a draw hook: device Create* only here), the flush and the sun tiers
// at the scene resolve - or the game thread where a park runs the flush (OBJBUF; g_res.obj_wait is Resources',
// release() resets it). Grows by doubling; otherwise one UpdateSubresource per dirty run (whole upload past half).
// Every write carries a box. False = no buffer.
// OBJBUF_RETRY: a buffer the device refused is asked for again OBJBUF_RETRY_CALLS calls on (about two seconds
// at ~5 calls a frame and 60 fps, as SKIN_GPU_RETRY waits), not on every call; until then the call returns false
// at once (no buffer: every bucket route draws per object).
static constexpr uint32_t OBJBUF_RETRY_CALLS = 600u;
inline bool objbuf_sync(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_OBJBUF_SYNC);   // PROF.
    const uint32_t n = static_cast<uint32_t>(g_objbuf_cpu.size());
    if (n == 0 || !ctx) return false;
    bool whole = false;
    // SHADOW_OFF: the switch moved - every record's receive lane again, and the buffer goes up whole.
    if (shadows_off() != g_objbuf_shoff) {
        g_objbuf_shoff = !g_objbuf_shoff;
        const uint32_t m = n < g_scene.objs.size() ? n : static_cast<uint32_t>(g_scene.objs.size());
        for (uint32_t s = 0; s < m; ++s)
            g_objbuf_cpu[s].pos[3] = (g_scene.objs[s].receive_shadow && !g_objbuf_shoff) ? 0.0f : 1.0f;
        whole = true;
    }
    if (!g_res.obj_sb || g_res.obj_cap < n) {
        if (g_res.obj_wait > 0u) { --g_res.obj_wait; return false; }   // OBJBUF_RETRY.
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return false;
        KH_SAFE_RELEASE(g_res.obj_srv);
        KH_SAFE_RELEASE(g_res.obj_sb);
        uint32_t cap = g_res.obj_cap > 256 ? g_res.obj_cap : 256;
        while (cap < n) cap *= 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = cap * sizeof(KhObjRec);
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        bd.StructureByteStride = sizeof(KhObjRec);
        const HRESULT hr = dev->CreateBuffer(&bd, nullptr, &g_res.obj_sb);
        if (SUCCEEDED(hr)) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
            sd.Format = DXGI_FORMAT_UNKNOWN;
            sd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            sd.Buffer.FirstElement = 0;
            sd.Buffer.NumElements = cap;
            if (FAILED(dev->CreateShaderResourceView(g_res.obj_sb, &sd, &g_res.obj_srv))) {
                KH_SAFE_RELEASE(g_res.obj_sb);
                g_res.obj_srv = nullptr;
            }
        } else {
            g_res.obj_sb = nullptr;
        }
        dev->Release();
        if (!g_res.obj_srv) {
            g_res.obj_cap = 0;
            g_res.obj_wait = OBJBUF_RETRY_CALLS;   // OBJBUF_RETRY.
            return false;
        }
        g_res.obj_cap = cap;
        whole = true;
    }
    auto put = [&](uint32_t a, uint32_t b) {   // [a, b) slots.
        D3D11_BOX box = {};
        box.left = a * sizeof(KhObjRec);
        box.right = b * sizeof(KhObjRec);
        box.top = 0; box.bottom = 1; box.front = 0; box.back = 1;
        ctx->UpdateSubresource(g_res.obj_sb, 0, &box, g_objbuf_cpu.data() + a, 0, 0);
    };
    if (whole || g_objbuf_dirty.size() * 2 > n) {
        put(0, n);
    } else if (!g_objbuf_dirty.empty()) {
        std::sort(g_objbuf_dirty.begin(), g_objbuf_dirty.end());
        uint32_t a = g_objbuf_dirty[0], b = a + 1;
        for (size_t i = 1; i < g_objbuf_dirty.size(); ++i) {
            const uint32_t s = g_objbuf_dirty[i];
            if (s == b) { ++b; continue; }
            if (s == b - 1) continue;   // Duplicate (never, by the mark).
            put(a, b);
            a = s; b = s + 1;
        }
        put(a, b);
    }
    for (uint32_t s : g_objbuf_dirty) g_objbuf_dirty_mark[s] = 0;
    g_objbuf_dirty.clear();
    return true;
}

inline void objbuf_bind(ID3D11DeviceContext* ctx) {   // VS t39 (StateBackup restores it).
    ctx->VSSetShaderResources(39, 1, &g_res.obj_srv);
}

// Under g_draw_list_mutex: the slot's map entry is going away.
inline void scene_stage_remove(const RenderObject& o) {
    if (o.slot == SCENE_NONE || o.slot >= g_scene_stage.size()) return;
    g_scene_stage[o.slot] = nullptr;
    g_scene_handle[o.slot].clear();
    scene_mark(o.slot);
}
}