#pragma once

// The feature half (rendering_integration_features.hpp): the mesh disk cache: the file format, load and store, the
// reference notes. Never include it on its own.
namespace RenderIntegration {
// CLOTH: the weight chunk, written after the LOD tables and probed by
// its magic. A change to how a weight is DERIVED (the group names, the
// sim/pin combination, the seam rule) must change this magic, or every cached
// mesh keeps weights the current code would not have produced.
static constexpr uint32_t CLOTH_CACHE_MAGIC = 0x574C434Bu;            // "KCLW" little-endian.
// SKEL: the authoring-frame and skin chunk, written after the weight chunk
// for every import and probed the same way. A change to how the skeleton or an
// influence is DERIVED must change this magic, for the reason above.
static constexpr uint32_t SKIN_CACHE_MAGIC = 0x334B534Bu;             // "KSK3" little-endian: a decimated
                                                                          // level's influences are blended
                                                                          // (LOD_SKIN). KSK2: twin or none;
                                                                          // an earlier build could derive the
                                                                          // parents from a freed scene.
// EA_KSK2_DEAD: the KSK2 reading (LOD_SKIN: a chunk without influences read as it stands) is gone - every
// KSK2 file carries an older version word (MESH_CACHE_VERSION has been bumped since KSK3), which the loader refuses
// before the chunk.
// OCC_BOX: the occluder-box chunk (mesh_occ_accept), written after the
// skin chunk for every import and required by the loader. A change to how the
// box is DERIVED must change this magic, for the reason above.
static constexpr uint32_t OCC_CACHE_MAGIC = 0x3342434Bu;              // "KCB3" little-endian: every mesh's
                                                                          // bounds (KCB2: with a box only).
static constexpr uint32_t MESH_CACHE_MAGIC = 0x434D484Bu;   // "KHMC" little-endian.
// The format's version: a cached mesh at any other version regenerates. Bump it with every change to what a .khmc
// holds or to what the import or the LOD builder derives into it (its chunks and their order, axes, normals, tangents,
// winding, skin influences, cloth weights, the LOD ladders, the occluder box). A bump is never optional: a cache hit
// returns before ufbx is opened, so anything that exists only in the source FBX (cloth weights, the skeleton, the box
// centre) or that the import derives stays missing or stale in every mesh an earlier build imported, silently (the
// mesh draws normally; it refuses to simulate, faces the old way, keeps its old normals or LOD cracks).
static constexpr uint32_t MESH_CACHE_VERSION = 21;

// CACHE_LIB_VER - the version word a .khmc holds and a load requires: MESH_CACHE_VERSION in its low byte and
// ufbx's release (UFBX_HEADER_VERSION) above it, as the file holds ufbx's import (triangulation, transforms,
// normals) - a build with another ufbx reads no file written with this one and writes its own. mikktspace.c (the
// tangents) carries no version: updating it needs a bump of MESH_CACHE_VERSION by hand.
static_assert(MESH_CACHE_VERSION < 256u && UFBX_HEADER_VERSION < (1u << 24), "CACHE_LIB_VER: the word's fields");
static constexpr uint32_t MESH_CACHE_WORD =
    MESH_CACHE_VERSION | (static_cast<uint32_t>(UFBX_HEADER_VERSION) << 8);

inline std::filesystem::path mesh_cache_file(uint64_t hash) {
    return mesh_cache_dir() / cache_file_name("khm_", hash, ".khmc");
}

// Full-validation load; true = d carries the mesh and the file's write time was touched (the lru term). Any
// structural doubt is a miss. own = the Documents copy: only that one gets the lru touch and is under the trim
// budget.
inline bool mesh_cache_load_one(const std::filesystem::path& p, bool own,
                                uint64_t hash, MeshDef& d) {
    try {
        std::ifstream f(p, std::ios::binary);
        if (!f) return false;
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
        f.close();
        size_t o = 0;
        auto rd = [&](void* dst, size_t n) -> bool {
            if (o + n > b.size()) return false;
            memcpy(dst, b.data() + o, n);
            o += n;
            return true;
        };
        uint32_t mag = 0, ver = 0, nv = 0, ns = 0, ni = 0;
        uint64_t src = 0;
        if (!rd(&mag, 4) || mag != MESH_CACHE_MAGIC) return false;
        if (!rd(&ver, 4) || ver != MESH_CACHE_WORD) return false;   // CACHE_LIB_VER.
        if (!rd(&src, 8) || src != hash) return false;
        if (!rd(d.native_size, 12)) return false;
        if (!rd(&nv, 4) || !rd(&ns, 4) || !rd(&ni, 4)) return false;
        if (nv == 0 || nv > FBX_VERT_CEIL) return false;
        if (ns == 0 || ns > 100000u) return false;
        if (ni == 0 || ni > FBX_VERT_CEIL || ni % 3u != 0u) return false;
        if ((b.size() - o) < static_cast<uint64_t>(nv) * sizeof(MeshVertex) +
                             static_cast<uint64_t>(ni) * sizeof(uint32_t)) return false;
        d.verts.resize(nv);
        if (!rd(d.verts.data(), static_cast<size_t>(nv) * sizeof(MeshVertex))) return false;
        d.indices.resize(ni);
        if (!rd(d.indices.data(), static_cast<size_t>(ni) * sizeof(uint32_t))) return false;
        for (uint32_t ix : d.indices) if (ix >= nv) return false;
        d.submeshes.resize(ns);
        // One submesh table, range-checked against the vertex count. Shared by level 0 and every LOD level.
        auto tab = [&](std::vector<MeshSubmesh>& t) -> bool {
            for (uint32_t i = 0; i < ns; ++i) {
                uint32_t nl = 0;
                if (!rd(&nl, 4) || nl > 1024u) return false;
                t[i].name.resize(nl);
                if (nl && !rd(&t[i].name[0], nl)) return false;
                if (!rd(&t[i].index_start, 4) ||
                    !rd(&t[i].index_count, 4)) return false;
                const uint64_t end = static_cast<uint64_t>(t[i].index_start) +
                                     t[i].index_count;
                if (end > ni) return false;
            }

            return true;
        };
        if (!tab(d.submeshes)) return false;
        // LOD block: lod_n, then per level the whole-mesh range and a submesh table of level 0's cardinality -
        // validated here rather than assumed. Any doubt is a miss.
        uint32_t ln = 0;
        if (!rd(&ln, 4) || ln > static_cast<uint32_t>(LOD_MAX)) return false;
        d.lod_n = static_cast<uint8_t>(ln);

        for (uint32_t mc_k = 0; mc_k < ln; ++mc_k) {
            if (!rd(&d.lod_istart[mc_k], 4) ||
                !rd(&d.lod_icount[mc_k], 4)) return false;
            const uint64_t le = static_cast<uint64_t>(d.lod_istart[mc_k]) +
                                d.lod_icount[mc_k];
            if (le > ni) return false;
            d.lod_sub[mc_k].resize(ns);
            if (!tab(d.lod_sub[mc_k])) return false;
        }

        // Level 0's count is where level 1 begins (mesh_base_icount): it must lie inside the index stream (the
        // per-level test above implies it).
        if (ln > 0 && d.lod_istart[0] > ni) return false;
        // CLOTH: the weight chunk, probed by its magic (no chunk precedes it since version 10; the distance-field
        // chunk of versions 8 and 9 is gone with the field, SSAO). The store writes it only for a mesh with cloth
        // authoring, so its absence is a mesh without; a length that does not match the vertex array (a damaged
        // file) rewinds the offset, the skin chunk's magic test below then reads this one's and fails, and the whole
        // file is a miss (a re-import).
        d.cloth_w.clear();
        if (b.size() - o >= 8u) {
            uint32_t cm = 0, cn = 0;
            const size_t co = o;
            if (rd(&cm, 4) && rd(&cn, 4) && cm == CLOTH_CACHE_MAGIC &&
                cn == nv && b.size() - o >= static_cast<uint64_t>(cn)) {
                d.cloth_w.resize(cn);
                if (!rd(d.cloth_w.data(), cn)) d.cloth_w.clear();
            } else {
                o = co;
            }
        }
        // SKEL: the frame and skin chunk. Every import writes it, so a file without one (or with a malformed one)
        // is a miss: the box centre a skeletal binding places the model by would otherwise be lost silently.
        d.skin_bones.clear();
        d.skin_inf.clear();
        {
            uint32_t km = 0, kb = 0, ki = 0;
            if (!rd(&km, 4) || km != SKIN_CACHE_MAGIC) return false;
            if (!rd(d.native_ctr, 12) || !rd(d.native_ext, 12)) return false;
            for (int k = 0; k < 3; ++k) {   // The skinning divides by the extent.
                if (!(d.native_ext[k] > 0.0f && d.native_ext[k] < 1.0e7f)) return false;
                if (!(fabsf(d.native_ctr[k]) < 1.0e7f)) return false;   // NaN fails too.
            }
            if (!rd(&kb, 4) || kb > static_cast<uint32_t>(SKIN_BONES_MAX)) return false;
            d.skin_bones.resize(kb);
            for (uint32_t b = 0; b < kb; ++b) {
                SkinBone& sb = d.skin_bones[b];
                uint32_t nl = 0;
                if (!rd(&nl, 4) || nl > 1024u) return false;
                sb.name.resize(nl);
                if (nl && !rd(&sb.name[0], nl)) return false;
                if (!rd(&sb.parent, 4) || !rd(sb.head, 12)) return false;
                for (int k = 0; k < 3; ++k) {   // IMPORT_BOUND: the import's bone bound (NaN fails too).
                    if (!(fabsf(sb.head[k]) < 1.0e7f)) return false;
                }
                if (sb.parent < -1 || sb.parent >= static_cast<int32_t>(kb) ||
                    sb.parent == static_cast<int32_t>(b)) return false;
            }
            if (!rd(&ki, 4) || (ki != 0u && ki != nv)) return false;
            if (ki) {
                d.skin_inf.resize(ki);
                if (!rd(d.skin_inf.data(), static_cast<size_t>(ki) * sizeof(SkinInf))) return false;
                for (const SkinInf& f : d.skin_inf) {
                    for (int q = 0; q < 4; ++q) {
                        if (!(f.w[q] >= 0.0f && f.w[q] <= 1.0001f)) return false;   // NaN fails too.
                        if (f.w[q] > 0.0f && f.b[q] >= kb) return false;
                    }
                }
            }
        }
        // OCC_BOX: the box chunk, last and required - taken only under a seal that matches this geometry, so a
        // cached box always belongs to exactly the triangles it was proven on. Anything else is a miss.
        {
            uint32_t om = 0, oo = 0;
            float mc_or[12];
            uint64_t os = 0;
            if (!rd(&om, 4) || om != OCC_CACHE_MAGIC || !rd(&oo, 4) ||
                !rd(mc_or, 48) || !rd(&os, 8)) return false;
            if (!mesh_occ_accept(d, oo, mc_or, os)) return false;
        }

        if (own) {
            try {
                std::filesystem::last_write_time(p, std::filesystem::file_time_type::clock::now());
            } catch (...) {}
        }

        return true;
    } catch (...) { return false; }
}

// Full-validation load across every candidate: the Documents copy first, then every mod that ships the compiled
// form. A partially filled MeshDef from a rejected candidate is never carried forward.
inline bool mesh_cache_load(uint64_t hash, MeshDef& d) {
    std::vector<std::filesystem::path> cand;
    const bool docs = !mesh_cache_dir().empty();
    if (docs) cand.push_back(mesh_cache_file(hash));
    mod_cache_append(cand, cache_file_name("khm_", hash, ".khmc"));

    for (size_t i = 0; i < cand.size(); ++i) {
        const bool own = i == 0 && docs;
        MeshDef mc_try;

        if (mesh_cache_load_one(cand[i], own, hash, mc_try)) {
            d = std::move(mc_try);
            return true;
        }
    }

    return false;
}

inline void mesh_cache_store(uint64_t hash, const MeshDef& d) {
    if (mesh_cache_dir().empty()) return;
    if (d.verts.empty() || d.indices.empty() || d.submeshes.empty()) return;

    std::filesystem::path tmp;   // CACHE_TMP_WRITE.
    try {
        const std::filesystem::path p = mesh_cache_file(hash);
        tmp = cache_tmp_of(p);
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) return;
        auto wr = [&](const void* src, size_t n) {
            f.write(static_cast<const char*>(src), static_cast<std::streamsize>(n));
        };
        const uint32_t nv = static_cast<uint32_t>(d.verts.size());
        const uint32_t ns = static_cast<uint32_t>(d.submeshes.size());
        const uint32_t ni = static_cast<uint32_t>(d.indices.size());
        wr(&MESH_CACHE_MAGIC, 4);
        wr(&MESH_CACHE_WORD, 4);   // CACHE_LIB_VER.
        wr(&hash, 8);
        wr(d.native_size, 12);
        wr(&nv, 4);
        wr(&ns, 4);
        wr(&ni, 4);
        wr(d.verts.data(), static_cast<size_t>(nv) * sizeof(MeshVertex));
        wr(d.indices.data(), static_cast<size_t>(ni) * sizeof(uint32_t));

        auto tab = [&](const std::vector<MeshSubmesh>& t) {
            for (uint32_t i = 0; i < ns; ++i) {
                const MeshSubmesh& sm = t[i];
                const uint32_t nl = sm.name.size() > 1024u
                                  ? 1024u : static_cast<uint32_t>(sm.name.size());
                wr(&nl, 4);
                if (nl) wr(sm.name.data(), nl);
                wr(&sm.index_start, 4);
                wr(&sm.index_count, 4);
            }
        };
        tab(d.submeshes);
        // A level whose table cardinality disagrees with level 0 is not written at all; the reader validates the same.
        uint32_t ln = d.lod_n;

        for (uint32_t k = 0; k < ln; ++k) {
            if (d.lod_sub[k].size() != d.submeshes.size()) { ln = k; break; }
        }

        wr(&ln, 4);

        for (uint32_t k = 0; k < ln; ++k) {
            wr(&d.lod_istart[k], 4);
            wr(&d.lod_icount[k], 4);
            tab(d.lod_sub[k]);
        }

        // CLOTH: the weight chunk, after the levels. Written only when the mesh carried authoring - an absent
        // chunk and an all-zero one mean different things (not cloth, versus fully pinned cloth).
        if (d.cloth_w.size() == d.verts.size() && !d.cloth_w.empty()) {
            const uint32_t cn = static_cast<uint32_t>(d.cloth_w.size());
            wr(&CLOTH_CACHE_MAGIC, 4);
            wr(&cn, 4);
            wr(d.cloth_w.data(), cn);
        }
        // SKEL: the frame and skin chunk, always (the loader requires it); the box chunk follows.
        {
            wr(&SKIN_CACHE_MAGIC, 4);
            wr(d.native_ctr, 12);
            wr(d.native_ext, 12);
            const uint32_t kb = static_cast<uint32_t>(d.skin_bones.size());
            wr(&kb, 4);
            for (const SkinBone& sb : d.skin_bones) {
                const uint32_t nl = sb.name.size() > 1024u ? 1024u : static_cast<uint32_t>(sb.name.size());
                wr(&nl, 4);
                if (nl) wr(sb.name.data(), nl);
                wr(&sb.parent, 4);
                wr(sb.head, 12);
            }
            const uint32_t ki = d.skin_inf.size() == d.verts.size()
                              ? static_cast<uint32_t>(d.skin_inf.size()) : 0u;
            wr(&ki, 4);
            if (ki) wr(d.skin_inf.data(), static_cast<size_t>(ki) * sizeof(SkinInf));
        }
        // OCC_BOX: the box chunk, last and always (a mesh with no box records that too; the loader requires it).
        {
            const uint32_t oo = d.occ_ok ? 1u : 0u;
            float mc_or[12];
            mesh_occ_record(d, mc_or);
            const uint64_t os = mesh_occ_seal(d, oo, mc_or);
            wr(&OCC_CACHE_MAGIC, 4);
            wr(&oo, 4);
            wr(mc_or, 48);
            wr(&os, 8);
        }
        cache_tmp_commit(f, tmp, p);   // CACHE_TMP_WRITE.
    } catch (...) {
        std::error_code ec;   // CACHE_TMP_WRITE: a throw mid-write leaves no temp behind.
        if (!tmp.empty()) std::filesystem::remove(tmp, ec);
    }
}

// Registration tail shared by the ufbx import and the cache hit (fbx_register): name the def, append, and leave the
// publish + GPU top-up to the next injection or flush (mesh_publish_land; never parked). Game thread.
// g_mesh_last_ref: per id, GetTickCount64 of the last reference note (the mesh GC's input). MESH_REF_LOCK:
// under g_mesh_ref_mu, which nests nothing but the store's append_mx (mesh_gc's GC_RECHECK tombstone; append_mx
// nests nothing): game-thread writers (flush_frame's census, fbx_register) may resize it while mesh_gc - which
// also writes it (its own walk's note) - reads it on the flush's thread (the render thread, or the game thread under
// the park).
static std::vector<uint64_t> g_mesh_last_ref;
static std::mutex g_mesh_ref_mu;

// Called for every object of every flush (visible or not: an invisible object still owns its mesh) and at import
// (mesh_note_ref, game thread). REF_NOTE_ONCE: a walk's reference notes, batched - the distinct non-builtin ids
// it saw (builtins are never collected: the GC's loop starts at builtin_n and their stamps are never read), stamped
// once under one lock at the walk's end. One record per walk (the census on the game thread, the GC's own on the
// flush's thread - the render thread, or the game thread under the park); each is that walk's scratch.
struct RefNote { std::vector<int> ids; std::vector<uint32_t> mark; uint32_t serial = 0; };
static RefNote g_ref_note_cen;   // flush_frame's census (game thread).
static RefNote g_ref_note_gc;    // mesh_gc (the flush's thread).
inline void ref_note_begin(RefNote& n) { if (++n.serial == 0u) n.serial = 1u; n.ids.clear(); }
inline void ref_note_add(RefNote& n, int id) {
    if (id < 0 || static_cast<uint32_t>(id) < mesh_store().builtin_n) return;
    const size_t i = static_cast<size_t>(id);
    if (i >= n.mark.size()) n.mark.resize(i + 1u, 0u);
    if (n.mark[i] == n.serial) return;
    n.mark[i] = n.serial;
    n.ids.push_back(id);
}
inline void ref_note_end(RefNote& n) {
    if (n.ids.empty()) return;
    std::lock_guard<std::mutex> g(g_mesh_ref_mu);
    const uint64_t now = GetTickCount64();
    for (int id : n.ids) {
        const size_t i = static_cast<size_t>(id);
        if (i >= g_mesh_last_ref.size()) g_mesh_last_ref.resize(i + 1, 0);
        g_mesh_last_ref[i] = now;
    }
    n.ids.clear();
}
inline void mesh_note_ref(int id) {
    if (id < 0) return;
    const size_t i = static_cast<size_t>(id);
    std::lock_guard<std::mutex> g(g_mesh_ref_mu);
    if (i >= g_mesh_last_ref.size()) g_mesh_last_ref.resize(i + 1, 0);
    g_mesh_last_ref[i] = GetTickCount64();
}
// GC_RECHECK - a new user's reference to a mesh it just resolved: stamped first, then the id read alive (or
// appended, its publish pending) again. mesh_gc re-reads the stamp and tombstones in one g_mesh_ref_mu section, so
// either it saw this stamp and keeps the mesh, or its tombstone is visible here. False = released meanwhile: resolve
// again (a by-path or generated mesh is imported or built anew; a released id is gone). Game thread.
inline bool mesh_take_ref(int id) {
    if (id < 0) return false;
    mesh_note_ref(id);
    return mesh_alive(id) || mesh_def_staged(id) != nullptr;
}
}