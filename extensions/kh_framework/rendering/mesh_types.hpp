#pragma once

// The shared head (rendering_integration.hpp): the mesh types (MeshVertex, MeshSubmesh, the skin influences, MeshDef)
// and the mesh-store flags. Never include it on its own.
namespace RenderIntegration {

struct MeshVertex {
    float pos[3];   // Offset 0 (contract: never moves - both input layouts).
    float nrm[3];   // Offset 12 (contract: never moves).
    float uv[2];   // Offset 24 - texture lane (builtins carry usable UVs).
    float tan[4];   // Offset 32 - tangent xyz, handedness w (mikktspace); builtins analytic (meshgen), w = +-1.
};

// Per-material index range into MeshDef::indices; builtins carry one full-range "default" entry, so no consumer
// special-cases them. Every draw takes (index_count, index_start) with base vertex 0 (DrawIndexed or
// DrawIndexedInstanced): indices absolute.
struct MeshSubmesh {
    std::string name;
    uint32_t index_start = 0;
    uint32_t index_count = 0;
};

// Submesh count is invariant across LOD levels (one range per submesh per level, in order: LOD_JOINT decimates them
// together and a triangle keeps its submesh; a small submesh may draw nothing at a coarse level), so a material slot
// index addresses the same surface at every level.
static constexpr int LOD_MAX = 12;   // 13 total with level 0.

// SKEL: one bone of an imported skeleton, and one vertex's influences. Both are written raw into the mesh cache
// (SKIN_CACHE_MAGIC), so SkinInf keeps a fixed layout.
struct SkinBone {
    std::string name;       // Lower-cased: matched to memory point names case-insensitively.
    int32_t parent = -1;    // Index into MeshDef::skin_bones; -1 = a root of the skeleton.
    float head[3] = { 0.0f, 0.0f, 0.0f };   // Bind position: engine axes, metres, authored frame.
};
struct SkinInf {
    uint16_t b[4] = { 0, 0, 0, 0 };           // MeshDef::skin_bones indices.
    // Summing to 1 or under: what is under 1 follows the root (all 0: the whole vertex does). An import's sum
    // is 1 or 0; a decimated vertex between weighted and unweighted ones carries a part (LOD_SKIN).
    float w[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert(sizeof(SkinInf) == 24, "SkinInf is cached raw: four uint16 then four float");

struct MeshDef {
    std::string name;   // Primary name (SQF mesh selector, lower-case).
    std::string alias;   // Accepted alternative ("" = none).
    // Welded vertices + a triangle-list index stream laid out [level 0][level 1]...[level lod_n]. Builders produce an
    // expanded triangle list in verts with ranges in triangle-vertex units; mesh_weld dedups and fills indices,
    // then reorders the triangles inside each range for the vertex cache (mesh_cache_order): the ranges become
    // index ranges with starts and counts unchanged. indices.empty() = pre-weld form; never published.
    std::vector<MeshVertex> verts;
    std::vector<uint32_t>   indices;
    std::vector<MeshSubmesh> submeshes;
    std::vector<MeshSubmesh> lod_sub[LOD_MAX];
    uint32_t lod_istart[LOD_MAX] = {};
    uint32_t lod_icount[LOD_MAX] = {};
    uint8_t  lod_n = 0;
    // Builtins are authored at unit size.
    float native_size[3] = { 1.0f, 1.0f, 1.0f };
    // CLOTH: per-vertex simulation weight, parallel to verts, 0 = fully pinned, 255 = fully simulated; from the
    // kh_cloth_sim / kh_cloth_pin vertex groups (FBX skin clusters) or the colour sets of the same names. EMPTY = no
    // cloth authoring (every builtin, every unpainted model): an all-zero array is a fully pinned cloth, an empty one
    // is not cloth. A vertex only a decimated LOD level uses takes the weight of the source position it sits on, else
    // 0; none is simulated (the cloth builds from level 0, and a cloth is lod_locked).
    std::vector<uint8_t> cloth_w;
    // SKEL. native_ctr / native_ext: the box the importer normalized the positions by, ENGINE axes and metres: a
    // stored position p was authored at p * native_ext + native_ctr - the frame a skeletal binding puts on the parent
    // object's origin. Builtins keep the unit box at the origin. skin_bones: the skeleton (every bone a vertex group
    // names, plus its bone ancestors); skin_inf: the four strongest influences per vertex, parallel to verts, EMPTY =
    // no vertex groups. A vertex only a decimated LOD level uses carries the influences of the source position it
    // sits on, else the blend its collapses carried to the point the decimator placed it at (LOD_SKIN): every
    // level skins.
    float native_ctr[3] = { 0.0f, 0.0f, 0.0f };
    float native_ext[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<SkinBone> skin_bones;
    std::vector<SkinInf> skin_inf;
    // OCC_BOX (mesh_occ_prep): the occluder box VIS_OCC draws for this mesh - normalised, engine axes, centre
    // occ_c, half extents occ_h - valid while occ_ok; and every level's vertex bounds, made whatever the proof decides
    // (OCC_TARGET; the unit box until made).
    bool  occ_ok = false;
    float occ_c[3] = { 0.0f, 0.0f, 0.0f };
    float occ_h[3] = { 0.0f, 0.0f, 0.0f };
    float occ_hmn[3] = { -0.5f, -0.5f, -0.5f };
    float occ_hmx[3] = { 0.5f, 0.5f, 0.5f };
};
static std::atomic<uint64_t> g_ref_note_cen_ms{ 0 };   // GC_NOTE_SKIP: when the census last stamped (GetTickCount64).

static std::atomic<bool> g_mesh_publish_pending{false};
}