#pragma once

// The feature half (rendering_integration_features.hpp): the mesh store (MeshStore, mesh_def, the LOD ranges) and the
// mesh vertex buffers (ensure_mesh_vbs). Never include it on its own.
namespace RenderIntegration {

// Mesh store: block-chained slot table, append-only, release-published count. A
// block-pointer store sequences before the publish release-store, so any reader
// that can see an id (acquire on the count) sees its block.
static constexpr uint32_t MESH_BLOCK_SHIFT = 8;
static constexpr uint32_t MESH_BLOCK = 1u << MESH_BLOCK_SHIFT;   // 256 meshes per block.
static constexpr uint32_t MESH_ROOT = 4096;

struct MeshStore {
    MeshDef** root[MESH_ROOT] = {};   // on-demand blocks; entries stable for the session.
    std::atomic<uint32_t> published{ 0 };
    std::mutex append_mx;   // append / publish / tombstone, and the staged lookup (mesh_def_staged); mesh_def never.
    uint32_t appended = 0;
    uint32_t builtin_n = 0;   // MESH_FREE: ids below this are builtins, never released.

    // Append-side slot fetch: allocates the covering block on first touch (blocks are never freed, ids never
    // reused). Caller holds append_mx or is the store's single-threaded static init.
    MeshDef*& slot(uint32_t id) {
        MeshDef**& b = root[id >> MESH_BLOCK_SHIFT];
        if (!b) b = new MeshDef*[MESH_BLOCK]();
        return b[id & (MESH_BLOCK - 1u)];
    }
};

inline MeshStore& mesh_store() {
    static MeshStore s;
    // Builtins register inside this static's one-time init (thread-safe, happens-before every return; no lock).
    // Do not call mesh_append from this lambda: it re-enters mesh_store mid-init.
    static const bool init = []() {   // s has static storage: no capture needed.
        auto add = [](const char* name, const char* alias,
                             std::vector<MeshVertex>&& verts) {
            meshgen::bake(verts);   // Arma axes -> engine axes + winding.
            MeshDef* ms_d = new MeshDef();
            ms_d->name = name;
            ms_d->alias = alias;
            ms_d->verts = std::move(verts);
            MeshSubmesh sm;
            sm.name = "default";
            sm.index_start = 0;
            sm.index_count = static_cast<uint32_t>(ms_d->verts.size());
            ms_d->submeshes.push_back(std::move(sm));
            mesh_weld(*ms_d);
            mesh_occ_prep(*ms_d);   // OCC_BOX: the builtins by the imports' rule.
            s.slot(s.appended) = ms_d;
            s.appended++;
        };

        // "box" - the original unit cube, per-face normals.
        {
            std::vector<MeshVertex> v;
            const float mn[3] = { -0.5f, -0.5f, -0.5f };
            const float mx[3] = {  0.5f,  0.5f,  0.5f };
            meshgen::block(v, mn, mx);
            add("box", "cube", std::move(v));
        }

        // "steps": 3-step staircase ascending toward +y (north), concave so each riser shadows the tread below (the
        // self-shadowing test case). Arma axes.
        {
            std::vector<MeshVertex> v;
            const float t = 1.0f / 3.0f;
            const float y0 = -0.5f, y1 = -0.5f + t, y2 = -0.5f + 2.0f * t, y3 = 0.5f;
            const float z0 = -0.5f, z1 = -0.5f + t, z2 = -0.5f + 2.0f * t, z3 = 0.5f;
            const float nzp[3] = { 0.0f, 0.0f, 1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
            const float nyp[3] = { 0.0f, 1.0f, 0.0f }, nyn[3] = { 0.0f, -1.0f, 0.0f };
            const float nxp[3] = { 1.0f, 0.0f, 0.0f }, nxn[3] = { -1.0f, 0.0f, 0.0f };

            // Bottom (facing -z: down).
            { const float a[3] = { -0.5f, y0, z0 }, b[3] = { 0.5f, y0, z0 }, c[3] = { 0.5f, y3, z0 }, d[3] = { -0.5f, y3, z0 }; meshgen::quad(v, a, b, c, d, nzn); }
            // Back (the tall face at y3).
            { const float a[3] = { -0.5f, y3, z0 }, b[3] = { 0.5f, y3, z0 }, c[3] = { 0.5f, y3, z3 }, d[3] = { -0.5f, y3, z3 }; meshgen::quad(v, a, b, c, d, nyp); }
            // Risers (facing -y, the open side).
            { const float a[3] = { -0.5f, y0, z0 }, b[3] = { 0.5f, y0, z0 }, c[3] = { 0.5f, y0, z1 }, d[3] = { -0.5f, y0, z1 }; meshgen::quad(v, a, b, c, d, nyn); }
            { const float a[3] = { -0.5f, y1, z1 }, b[3] = { 0.5f, y1, z1 }, c[3] = { 0.5f, y1, z2 }, d[3] = { -0.5f, y1, z2 }; meshgen::quad(v, a, b, c, d, nyn); }
            { const float a[3] = { -0.5f, y2, z2 }, b[3] = { 0.5f, y2, z2 }, c[3] = { 0.5f, y2, z3 }, d[3] = { -0.5f, y2, z3 }; meshgen::quad(v, a, b, c, d, nyn); }
            // Treads (facing +z: up).
            { const float a[3] = { -0.5f, y0, z1 }, b[3] = { 0.5f, y0, z1 }, c[3] = { 0.5f, y1, z1 }, d[3] = { -0.5f, y1, z1 }; meshgen::quad(v, a, b, c, d, nzp); }
            { const float a[3] = { -0.5f, y1, z2 }, b[3] = { 0.5f, y1, z2 }, c[3] = { 0.5f, y2, z2 }, d[3] = { -0.5f, y2, z2 }; meshgen::quad(v, a, b, c, d, nzp); }
            { const float a[3] = { -0.5f, y2, z3 }, b[3] = { 0.5f, y2, z3 }, c[3] = { 0.5f, y3, z3 }, d[3] = { -0.5f, y3, z3 }; meshgen::quad(v, a, b, c, d, nzp); }

            // Sides: the staircase profile as one polygon, fanned from its bottom back corner, mapped planar.
            // OCC_BOX: every corner a neighbouring face shares is one of its own, so the mesh is watertight
            // (per-column quads leave T-junctions, and -0.5 + 3 t is not 0.5).
            {
                const float y[8] = { y3, y3, y2, y2, y1, y1, y0, y0 };
                const float z[8] = { z0, z3, z3, z2, z2, z1, z1, z0 };
                for (int sd = 0; sd < 2; ++sd) {
                    const float x = sd == 0 ? 0.5f : -0.5f;
                    const float* n = sd == 0 ? nxp : nxn;
                    for (int f = 1; f + 1 < 8; ++f) {
                        const int q[3] = { 0, f, f + 1 };
                        float p[3][3], u[3][2];
                        for (int k = 0; k < 3; ++k) {
                            p[k][0] = x;
                            p[k][1] = y[q[k]];
                            p[k][2] = z[q[k]];
                            u[k][0] = y[q[k]] + 0.5f;
                            u[k][1] = z[q[k]] + 0.5f;
                        }
                        meshgen::tri(v, p[0], n, u[0], p[1], n, u[1],
                                     p[2], n, u[2]);
                    }
                }
            }

            add("steps", "test", std::move(v));
        }

        // Meshes 2-5: the parametric builtins.
        { std::vector<MeshVertex> v; meshgen::uv_sphere(v, 16, 32); add("sphere", "ball", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::cylinder(v, 32);      add("cylinder", "cyl", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::cone(v, 32);          add("cone", "", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::pyramid(v);           add("pyramid", "", std::move(v)); }

        s.builtin_n = s.appended;
        s.published.store(s.appended, std::memory_order_release);
        return true;
    }();
    (void)init;
    return s;
}

// The published mesh count - the bound every consumer uses, never the raw append count.
inline uint32_t mesh_count() {
    return mesh_store().published.load(std::memory_order_acquire);
}

// id must already be bounded by the published count (mesh_id_clamp / mesh_id_from_gv). A released slot is a null
// tombstone (ids never reused) and resolves to mesh 0, so a stale id in a per-frame caster list still reads a valid
// def.
inline const MeshDef& mesh_def(int id) {
    const uint32_t md_id = static_cast<uint32_t>(id);
    const MeshDef* p = mesh_store().root[md_id >> MESH_BLOCK_SHIFT][md_id & (MESH_BLOCK - 1u)];
    if (p) return *p;
    return *mesh_store().root[0][0];
}

// MESH_FREE: a published, non-tombstoned slot. Builtins are always alive.
inline bool mesh_alive(int id) {
    if (id < 0) return false;
    const uint32_t ma_id = static_cast<uint32_t>(id);
    MeshStore& s = mesh_store();
    if (ma_id >= s.published.load(std::memory_order_acquire)) return false;
    MeshDef** b = s.root[ma_id >> MESH_BLOCK_SHIFT];
    return b && b[ma_id & (MESH_BLOCK - 1u)] != nullptr;
}

inline int mesh_append(MeshDef&& def) {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    // Full: ids are never reused (MESH_FREE), so a script that builds a new spec every frame reaches this
    // (mesh_generate.hpp's header); the callers report "mesh registry full".
    if (s.appended >= MESH_ROOT * MESH_BLOCK) return -1;
    s.slot(s.appended) = new MeshDef(std::move(def));   // Lives until mesh_release (MESH_FREE).
    return static_cast<int>(s.appended++);
}

// CPU half of a release: the slot becomes a null tombstone (id never reused); the def is handed back and parked with
// the GPU objects in Resources::mesh_grave for mesh_grave_sweep. From the releases only: the flush's housekeeping
// (mesh_gc - the render thread, or the game thread under the park; it tombstones under g_mesh_ref_mu,
// GC_RECHECK, then mesh_release_def) and the session teardown (mesh_release_session, under the park, through
// mesh_release), never beside a draw; the caller verified the id alive and unreferenced.
inline MeshDef* mesh_tombstone(int id) {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    const uint32_t mt_id = static_cast<uint32_t>(id);
    if (mt_id >= s.appended) return nullptr;
    MeshDef*& slot = s.slot(mt_id);
    MeshDef* d = slot;
    slot = nullptr;
    return d;
}

// Only where no draw can index the new id before ensure_mesh_vbs lands: mesh_publish_land (flush_locked's
// housekeeping, and the injection ahead of its scene sync - MESH_GEN_PUB), followed by ensure_mesh_vbs on its
// thread; and mesh_release_session, under the park, which then releases every non-builtin mesh, the one it
// published included.
inline uint32_t mesh_publish() {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    s.published.store(s.appended, std::memory_order_release);
    return s.appended;
}

// Level 0's index count: its block ends where level 1 begins.
inline uint32_t mesh_base_icount(const MeshDef& d) {
    return d.lod_n ? d.lod_istart[0] : static_cast<uint32_t>(d.indices.size());
}

inline int mesh_lod_clamp(const MeshDef& d, int l) {
    if (l < 0) return 0;
    return l > static_cast<int>(d.lod_n) ? static_cast<int>(d.lod_n) : l;
}

// Whole-mesh index range for a level, for the untextured draw.
inline void mesh_lod_range(const MeshDef& d, int l, UINT& s, UINT& c) {
    l = mesh_lod_clamp(d, l);

    if (l == 0) {
        s = 0;
        c = static_cast<UINT>(mesh_base_icount(d));
        return;
    }

    s = static_cast<UINT>(d.lod_istart[l - 1]);
    c = static_cast<UINT>(d.lod_icount[l - 1]);
}

inline const std::vector<MeshSubmesh>& mesh_lod_submeshes(const MeshDef& d, int l) {
    l = mesh_lod_clamp(d, l);
    return l == 0 ? d.submeshes : d.lod_sub[l - 1];
}

// Level 0's index count by id (the whole-mesh draws with no LOD walk).
inline UINT mesh_index_count(int id) {
    if (id < 0 || static_cast<uint32_t>(id) >= mesh_count()) id = 0;
    return static_cast<UINT>(mesh_base_icount(mesh_def(id)));
}

// Clamp shared by every draw path: an out-of-range or released id degrades to mesh 0.
inline int mesh_id_clamp(int id) {
    return mesh_alive(id) ? id : 0;
}

// Lookup that also covers ids appended but not yet published: the game thread's, and the cache-later landing in
// mesh_publish_land (a null test alone, and only while imports wait there). The one reader that takes append_mx -
// never per draw. Null outside the appended range; callers fall back to mesh 0.
inline const MeshDef* mesh_def_staged(int id) {
    if (id < 0) return nullptr;
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    const uint32_t ms_id = static_cast<uint32_t>(id);
    if (ms_id >= s.appended) return nullptr;
    return s.root[ms_id >> MESH_BLOCK_SHIFT][ms_id & (MESH_BLOCK - 1u)];
}

// Called from both mesh paths' ensure_resources, mesh_publish_land (the flush's and the injection's publish) and
// white_draw. No lock: every caller holds the park (render thread stalled) or is the render thread inside an engine
// call; they never overlap.
inline std::string ensure_mesh_vbs(ID3D11Device* dev) {
    const uint32_t n = mesh_count();
    if (g_res.mesh_ib.size() > g_res.mesh_vb.size()) g_res.mesh_ib.resize(g_res.mesh_vb.size());

    while (g_res.mesh_vb.size() < n) {
        const int id = static_cast<int>(g_res.mesh_vb.size());
        if (!mesh_alive(id)) {   // MESH_FREE tombstone (device re-create after a release).
            g_res.mesh_vb.push_back(nullptr);
            g_res.mesh_ib.push_back(nullptr);
            continue;
        }
        const MeshDef& md = mesh_def(id);
        // Empty geometry cannot make a buffer; registration must reject it. Fail loudly rather than publish a hole.
        if (md.verts.empty() || md.indices.empty()) return "mesh '" + md.name + "': empty geometry";
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(md.verts.size() * sizeof(MeshVertex));
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init = { md.verts.data(), 0, 0 };
        ID3D11Buffer* vb = nullptr;
        const HRESULT hr = dev->CreateBuffer(&bd, &init, &vb);
        if (FAILED(hr)) return "mesh '" + md.name + "': create VB " + hr_str(hr);
        D3D11_BUFFER_DESC ibd = {};
        ibd.ByteWidth = static_cast<UINT>(md.indices.size() * sizeof(uint32_t));
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA iinit = { md.indices.data(), 0, 0 };
        ID3D11Buffer* ib = nullptr;
        const HRESULT ihr = dev->CreateBuffer(&ibd, &iinit, &ib);
        if (FAILED(ihr)) {
            vb->Release();
            return "mesh '" + md.name + "': create IB " + hr_str(ihr);
        }
        g_res.mesh_vb.push_back(vb);
        g_res.mesh_ib.push_back(ib);
    }

    return "";
}
}