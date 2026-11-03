#pragma once

// The feature half (rendering_integration_features.hpp): the mesh store (MeshStore, mesh_def, the LOD ranges) and the
// mesh vertex buffers (ensure_mesh_vbs). Never include it on its own.
namespace RenderIntegration {

// Mesh store: block-chained slot table, append-only, release-published count. A
// block-pointer store sequences before the publish release-store, so any reader
// that can see an id (acquire on the count) sees its block.
static constexpr uint32_t KH_MESH_BLOCK_SHIFT = 8;
static constexpr uint32_t KH_MESH_BLOCK = 1u << KH_MESH_BLOCK_SHIFT;   // 256 meshes per block.
static constexpr uint32_t KH_MESH_ROOT = 4096;

struct MeshStore {
    MeshDef** root[KH_MESH_ROOT] = {};   // on-demand blocks; entries stable for the session.
    std::atomic<uint32_t> published{ 0 };
    std::mutex append_mx;   // append / publish / tombstone, and the staged lookup (kh_mesh_def_staged); mesh_def never.
    uint32_t appended = 0;
    uint32_t builtin_n = 0;   // KH_MESH_FREE: ids below this are builtins, never released.

    // Append-side slot fetch: allocates the covering block on first touch (blocks are never freed, ids never
    // reused). Caller holds append_mx or is the store's single-threaded static init.
    MeshDef*& slot(uint32_t id) {
        MeshDef**& b = root[id >> KH_MESH_BLOCK_SHIFT];
        if (!b) b = new MeshDef*[KH_MESH_BLOCK]();
        return b[id & (KH_MESH_BLOCK - 1u)];
    }
};

inline MeshStore& mesh_store() {
    static MeshStore s;
    // Builtins register inside this static's one-time init (thread-safe, happens-before every return; no lock).
    // Do not call kh_mesh_append from this lambda: it re-enters mesh_store mid-init.
    static const bool khms_init = []() {   // s has static storage: no capture needed.
        auto khms_add = [](const char* khms_name, const char* khms_alias,
                             std::vector<MeshVertex>&& khms_verts) {
            meshgen::bake(khms_verts);   // Arma axes -> engine axes + winding.
            MeshDef* khms_d = new MeshDef();
            khms_d->name = khms_name;
            khms_d->alias = khms_alias;
            khms_d->verts = std::move(khms_verts);
            MeshSubmesh khms_sm;
            khms_sm.name = "default";
            khms_sm.index_start = 0;
            khms_sm.index_count = static_cast<uint32_t>(khms_d->verts.size());
            khms_d->submeshes.push_back(std::move(khms_sm));
            kh_mesh_weld(*khms_d);
            kh_mesh_occ_prep(*khms_d);   // KH_OCC_BOX: the builtins by the imports' rule.
            s.slot(s.appended) = khms_d;
            s.appended++;
        };

        // "box" - the original unit cube, per-face normals.
        {
            std::vector<MeshVertex> v;
            const float mn[3] = { -0.5f, -0.5f, -0.5f };
            const float mx[3] = {  0.5f,  0.5f,  0.5f };
            meshgen::block(v, mn, mx);
            khms_add("box", "cube", std::move(v));
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
            // KH_OCC_BOX: every corner a neighbouring face shares is one of its own, so the mesh is watertight
            // (per-column quads leave T-junctions, and -0.5 + 3 t is not 0.5).
            {
                const float khst_y[8] = { y3, y3, y2, y2, y1, y1, y0, y0 };
                const float khst_z[8] = { z0, z3, z3, z2, z2, z1, z1, z0 };
                for (int khst_sd = 0; khst_sd < 2; ++khst_sd) {
                    const float khst_x = khst_sd == 0 ? 0.5f : -0.5f;
                    const float* khst_n = khst_sd == 0 ? nxp : nxn;
                    for (int khst_f = 1; khst_f + 1 < 8; ++khst_f) {
                        const int khst_q[3] = { 0, khst_f, khst_f + 1 };
                        float khst_p[3][3], khst_u[3][2];
                        for (int k = 0; k < 3; ++k) {
                            khst_p[k][0] = khst_x;
                            khst_p[k][1] = khst_y[khst_q[k]];
                            khst_p[k][2] = khst_z[khst_q[k]];
                            khst_u[k][0] = khst_y[khst_q[k]] + 0.5f;
                            khst_u[k][1] = khst_z[khst_q[k]] + 0.5f;
                        }
                        meshgen::tri(v, khst_p[0], khst_n, khst_u[0], khst_p[1], khst_n, khst_u[1],
                                     khst_p[2], khst_n, khst_u[2]);
                    }
                }
            }

            khms_add("steps", "test", std::move(v));
        }

        // Meshes 2-5: the parametric builtins.
        { std::vector<MeshVertex> v; meshgen::uv_sphere(v, 16, 32); khms_add("sphere", "ball", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::cylinder(v, 32);      khms_add("cylinder", "cyl", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::cone(v, 32);          khms_add("cone", "", std::move(v)); }
        { std::vector<MeshVertex> v; meshgen::pyramid(v);           khms_add("pyramid", "", std::move(v)); }

        s.builtin_n = s.appended;
        s.published.store(s.appended, std::memory_order_release);
        return true;
    }();
    (void)khms_init;
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
    const uint32_t khmd_id = static_cast<uint32_t>(id);
    const MeshDef* khmd_p = mesh_store().root[khmd_id >> KH_MESH_BLOCK_SHIFT][khmd_id & (KH_MESH_BLOCK - 1u)];
    if (khmd_p) return *khmd_p;
    return *mesh_store().root[0][0];
}

// KH_MESH_FREE: a published, non-tombstoned slot. Builtins are always alive.
inline bool kh_mesh_alive(int id) {
    if (id < 0) return false;
    const uint32_t khma_id = static_cast<uint32_t>(id);
    MeshStore& khma_s = mesh_store();
    if (khma_id >= khma_s.published.load(std::memory_order_acquire)) return false;
    MeshDef** khma_b = khma_s.root[khma_id >> KH_MESH_BLOCK_SHIFT];
    return khma_b && khma_b[khma_id & (KH_MESH_BLOCK - 1u)] != nullptr;
}

inline int kh_mesh_append(MeshDef&& def) {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    if (s.appended >= KH_MESH_ROOT * KH_MESH_BLOCK) return -1;   // Structurally unreachable in
                                                                 // Practice.
    s.slot(s.appended) = new MeshDef(std::move(def));   // Lives until kh_mesh_release (KH_MESH_FREE).
    return static_cast<int>(s.appended++);
}

// CPU half of a release: the slot becomes a null tombstone (id never reused); the def is handed back and parked with
// the GPU objects in Resources::mesh_grave for kh_mesh_grave_sweep. From the releases only: the flush's housekeeping
// (kh_mesh_gc - the render thread, or the game thread under the park; it tombstones under g_mesh_ref_mu,
// KH_GC_RECHECK, then kh_mesh_release_def) and the session teardown (kh_mesh_release_session, under the park, through
// kh_mesh_release), never beside a draw; the caller verified the id alive and unreferenced.
inline MeshDef* kh_mesh_tombstone(int id) {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    const uint32_t khmt_id = static_cast<uint32_t>(id);
    if (khmt_id >= s.appended) return nullptr;
    MeshDef*& khmt_slot = s.slot(khmt_id);
    MeshDef* khmt_d = khmt_slot;
    khmt_slot = nullptr;
    return khmt_d;
}

// Only where no draw can index the new id before ensure_mesh_vbs lands: kh_fbx_register under a park and
// kh_mesh_publish_land (flush_locked's housekeeping, and the injection ahead of its scene sync - KH_MESH_GEN_PUB), each
// followed by ensure_mesh_vbs on its thread; and kh_mesh_release_session, under the park, which then releases every
// non-builtin mesh, the one it published included.
inline uint32_t kh_mesh_publish() {
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    s.published.store(s.appended, std::memory_order_release);
    return s.appended;
}

// Level 0's index count: its block ends where level 1 begins.
inline uint32_t mesh_base_icount(const MeshDef& khl_d) {
    return khl_d.lod_n ? khl_d.lod_istart[0] : static_cast<uint32_t>(khl_d.indices.size());
}

inline int mesh_lod_clamp(const MeshDef& khl_d, int khl_l) {
    if (khl_l < 0) return 0;
    return khl_l > static_cast<int>(khl_d.lod_n) ? static_cast<int>(khl_d.lod_n) : khl_l;
}

// Whole-mesh index range for a level, for the untextured draw.
inline void mesh_lod_range(const MeshDef& khl_d, int khl_l, UINT& khl_s, UINT& khl_c) {
    khl_l = mesh_lod_clamp(khl_d, khl_l);

    if (khl_l == 0) {
        khl_s = 0;
        khl_c = static_cast<UINT>(mesh_base_icount(khl_d));
        return;
    }

    khl_s = static_cast<UINT>(khl_d.lod_istart[khl_l - 1]);
    khl_c = static_cast<UINT>(khl_d.lod_icount[khl_l - 1]);
}

inline const std::vector<MeshSubmesh>& mesh_lod_submeshes(const MeshDef& khl_d, int khl_l) {
    khl_l = mesh_lod_clamp(khl_d, khl_l);
    return khl_l == 0 ? khl_d.submeshes : khl_d.lod_sub[khl_l - 1];
}

// Level 0's index count by id (the whole-mesh draws with no LOD walk).
inline UINT mesh_index_count(int id) {
    if (id < 0 || static_cast<uint32_t>(id) >= mesh_count()) id = 0;
    return static_cast<UINT>(mesh_base_icount(mesh_def(id)));
}

// Clamp shared by every draw path: an out-of-range or released id degrades to mesh 0.
inline int mesh_id_clamp(int id) {
    return kh_mesh_alive(id) ? id : 0;
}

// Lookup that also covers ids appended but not yet published: the game thread's, and the cache-later landing in
// kh_mesh_publish_land (a null test alone, and only while imports wait there). The one reader that takes append_mx -
// never per draw. Null outside the appended range; callers fall back to mesh 0.
inline const MeshDef* kh_mesh_def_staged(int id) {
    if (id < 0) return nullptr;
    MeshStore& s = mesh_store();
    std::lock_guard<std::mutex> g(s.append_mx);
    const uint32_t khms_id = static_cast<uint32_t>(id);
    if (khms_id >= s.appended) return nullptr;
    return s.root[khms_id >> KH_MESH_BLOCK_SHIFT][khms_id & (KH_MESH_BLOCK - 1u)];
}

// Called from both mesh paths' ensure_resources, kh_mesh_publish_land (the flush's and the injection's publish),
// kh_white_draw and kh_fbx_register. No lock: every caller holds the park (render thread stalled) or is the render
// thread inside an engine call; they never overlap.
inline std::string ensure_mesh_vbs(ID3D11Device* dev) {
    const uint32_t khmv_n = mesh_count();
    if (g_res.mesh_ib.size() > g_res.mesh_vb.size()) g_res.mesh_ib.resize(g_res.mesh_vb.size());

    while (g_res.mesh_vb.size() < khmv_n) {
        const int khmv_id = static_cast<int>(g_res.mesh_vb.size());
        if (!kh_mesh_alive(khmv_id)) {   // KH_MESH_FREE tombstone (device re-create after a release).
            g_res.mesh_vb.push_back(nullptr);
            g_res.mesh_ib.push_back(nullptr);
            continue;
        }
        const MeshDef& khmv_md = mesh_def(khmv_id);
        // Empty geometry cannot make a buffer; registration must reject it. Fail loudly rather than publish a hole.
        if (khmv_md.verts.empty() || khmv_md.indices.empty()) return "mesh '" + khmv_md.name + "': empty geometry";
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(khmv_md.verts.size() * sizeof(MeshVertex));
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA init = { khmv_md.verts.data(), 0, 0 };
        ID3D11Buffer* khmv_vb = nullptr;
        const HRESULT khmv_hr = dev->CreateBuffer(&bd, &init, &khmv_vb);
        if (FAILED(khmv_hr)) return "mesh '" + khmv_md.name + "': create VB " + hr_str(khmv_hr);
        D3D11_BUFFER_DESC khmv_ibd = {};
        khmv_ibd.ByteWidth = static_cast<UINT>(khmv_md.indices.size() * sizeof(uint32_t));
        khmv_ibd.Usage = D3D11_USAGE_IMMUTABLE;
        khmv_ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA khmv_iinit = { khmv_md.indices.data(), 0, 0 };
        ID3D11Buffer* khmv_ib = nullptr;
        const HRESULT khmv_ihr = dev->CreateBuffer(&khmv_ibd, &khmv_iinit, &khmv_ib);
        if (FAILED(khmv_ihr)) {
            khmv_vb->Release();
            return "mesh '" + khmv_md.name + "': create IB " + hr_str(khmv_ihr);
        }
        g_res.mesh_vb.push_back(khmv_vb);
        g_res.mesh_ib.push_back(khmv_ib);
    }

    return "";
}
}