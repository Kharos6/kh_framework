#pragma once

// The feature half (rendering_integration_features.hpp): the mesh disk cache: the file format, load and store, the
// reference notes. Never include it on its own.
namespace RenderIntegration {
// KH_CLOTH: the weight chunk, written after the LOD tables and probed by
// its magic. A change to how a weight is DERIVED (the group names, the
// sim/pin combination, the seam rule) must change this magic, or every cached
// mesh keeps weights the current code would not have produced.
static constexpr uint32_t KH_CLOTH_CACHE_MAGIC = 0x574C434Bu;            // "KCLW" little-endian.
// KH_SKEL: the authoring-frame and skin chunk, written after the weight chunk
// for every import and probed the same way. A change to how the skeleton or an
// influence is DERIVED must change this magic, for the reason above.
static constexpr uint32_t KH_SKIN_CACHE_MAGIC = 0x334B534Bu;             // "KSK3" little-endian: a decimated
                                                                          // level's influences are blended
                                                                          // (KH_LOD_SKIN). KSK2: twin or none;
                                                                          // an earlier build could derive the
                                                                          // parents from a freed scene.
// KH_LOD_SKIN: a KSK2 chunk that carries no influences is read as it stands - nothing in it is derived
// differently now - so only a skinned mesh's file is a miss and re-imports.
static constexpr uint32_t KH_SKIN_CACHE_MAGIC_KSK2 = 0x324B534Bu;
// KH_OCC_BOX: the occluder-box chunk (kh_mesh_occ_accept), written after the
// skin chunk for every import and required by the loader. A change to how the
// box is DERIVED must change this magic, for the reason above.
static constexpr uint32_t KH_OCC_CACHE_MAGIC = 0x3342434Bu;              // "KCB3" little-endian: every mesh's
                                                                          // bounds (KCB2: with a box only).
static constexpr uint32_t KH_MESH_CACHE_MAGIC = 0x434D484Bu;   // "KHMC" little-endian.
// The format's version: a cached mesh at any other version regenerates. Bump it with every change to what a .khmc
// holds or to what the import or the LOD builder derives into it (its chunks and their order, axes, normals, tangents,
// winding, skin influences, cloth weights, the LOD ladders, the occluder box). A bump is never optional: a cache hit
// returns before ufbx is opened, so anything that exists only in the source FBX (cloth weights, the skeleton, the box
// centre) or that the import derives stays missing or stale in every mesh an earlier build imported, silently (the
// mesh draws normally; it refuses to simulate, faces the old way, keeps its old normals or LOD cracks).
static constexpr uint32_t KH_MESH_CACHE_VERSION = 21;

// KH_CACHE_LIB_VER - the version word a .khmc holds and a load requires: KH_MESH_CACHE_VERSION in its low byte and
// ufbx's release (UFBX_HEADER_VERSION) above it, as the file holds ufbx's import (triangulation, transforms,
// normals) - a build with another ufbx reads no file written with this one and writes its own. mikktspace.c (the
// tangents) carries no version: updating it needs a bump of KH_MESH_CACHE_VERSION by hand.
static_assert(KH_MESH_CACHE_VERSION < 256u && UFBX_HEADER_VERSION < (1u << 24), "KH_CACHE_LIB_VER: the word's fields");
static constexpr uint32_t KH_MESH_CACHE_WORD =
    KH_MESH_CACHE_VERSION | (static_cast<uint32_t>(UFBX_HEADER_VERSION) << 8);

inline std::filesystem::path kh_mesh_cache_file(uint64_t khmc_hash) {
    return kh_mesh_cache_dir() / kh_cache_file_name("khm_", khmc_hash, ".khmc");
}

// Full-validation load; true = khmc_d carries the mesh and the file's write time was touched (the lru term). Any
// structural doubt is a miss. khmc_own = the Documents copy: only that one gets the lru touch and is under the trim
// budget.
inline bool kh_mesh_cache_load_one(const std::filesystem::path& khmc_p, bool khmc_own,
                                   uint64_t khmc_hash, MeshDef& khmc_d) {
    try {
        std::ifstream khmc_f(khmc_p, std::ios::binary);
        if (!khmc_f) return false;
        std::vector<uint8_t> khmc_b((std::istreambuf_iterator<char>(khmc_f)),
                                    std::istreambuf_iterator<char>());
        khmc_f.close();
        size_t khmc_o = 0;
        auto khmc_rd = [&](void* khmc_dst, size_t khmc_n) -> bool {
            if (khmc_o + khmc_n > khmc_b.size()) return false;
            memcpy(khmc_dst, khmc_b.data() + khmc_o, khmc_n);
            khmc_o += khmc_n;
            return true;
        };
        uint32_t khmc_mag = 0, khmc_ver = 0, khmc_nv = 0, khmc_ns = 0, khmc_ni = 0;
        uint64_t khmc_src = 0;
        if (!khmc_rd(&khmc_mag, 4) || khmc_mag != KH_MESH_CACHE_MAGIC) return false;
        if (!khmc_rd(&khmc_ver, 4) || khmc_ver != KH_MESH_CACHE_WORD) return false;   // KH_CACHE_LIB_VER.
        if (!khmc_rd(&khmc_src, 8) || khmc_src != khmc_hash) return false;
        if (!khmc_rd(khmc_d.native_size, 12)) return false;
        if (!khmc_rd(&khmc_nv, 4) || !khmc_rd(&khmc_ns, 4) || !khmc_rd(&khmc_ni, 4)) return false;
        if (khmc_nv == 0 || khmc_nv > KH_FBX_VERT_CEIL) return false;
        if (khmc_ns == 0 || khmc_ns > 100000u) return false;
        if (khmc_ni == 0 || khmc_ni > KH_FBX_VERT_CEIL || khmc_ni % 3u != 0u) return false;
        if ((khmc_b.size() - khmc_o) < static_cast<uint64_t>(khmc_nv) * sizeof(MeshVertex) +
                                       static_cast<uint64_t>(khmc_ni) * sizeof(uint32_t)) return false;
        khmc_d.verts.resize(khmc_nv);
        if (!khmc_rd(khmc_d.verts.data(), static_cast<size_t>(khmc_nv) * sizeof(MeshVertex))) return false;
        khmc_d.indices.resize(khmc_ni);
        if (!khmc_rd(khmc_d.indices.data(), static_cast<size_t>(khmc_ni) * sizeof(uint32_t))) return false;
        for (uint32_t khmc_ix : khmc_d.indices) if (khmc_ix >= khmc_nv) return false;
        khmc_d.submeshes.resize(khmc_ns);
        // One submesh table, range-checked against the vertex count. Shared by level 0 and every LOD level.
        auto khmc_tab = [&](std::vector<MeshSubmesh>& khmc_t) -> bool {
            for (uint32_t khmc_i = 0; khmc_i < khmc_ns; ++khmc_i) {
                uint32_t khmc_nl = 0;
                if (!khmc_rd(&khmc_nl, 4) || khmc_nl > 1024u) return false;
                khmc_t[khmc_i].name.resize(khmc_nl);
                if (khmc_nl && !khmc_rd(&khmc_t[khmc_i].name[0], khmc_nl)) return false;
                if (!khmc_rd(&khmc_t[khmc_i].index_start, 4) ||
                    !khmc_rd(&khmc_t[khmc_i].index_count, 4)) return false;
                const uint64_t khmc_end = static_cast<uint64_t>(khmc_t[khmc_i].index_start) +
                                          khmc_t[khmc_i].index_count;
                if (khmc_end > khmc_ni) return false;
            }

            return true;
        };
        if (!khmc_tab(khmc_d.submeshes)) return false;
        // LOD block: lod_n, then per level the whole-mesh range and a submesh table of level 0's cardinality -
        // validated here rather than assumed. Any doubt is a miss.
        uint32_t khmc_ln = 0;
        if (!khmc_rd(&khmc_ln, 4) || khmc_ln > static_cast<uint32_t>(KH_LOD_MAX)) return false;
        khmc_d.lod_n = static_cast<uint8_t>(khmc_ln);

        for (uint32_t khmc_k = 0; khmc_k < khmc_ln; ++khmc_k) {
            if (!khmc_rd(&khmc_d.lod_istart[khmc_k], 4) ||
                !khmc_rd(&khmc_d.lod_icount[khmc_k], 4)) return false;
            const uint64_t khmc_le = static_cast<uint64_t>(khmc_d.lod_istart[khmc_k]) +
                                     khmc_d.lod_icount[khmc_k];
            if (khmc_le > khmc_ni) return false;
            khmc_d.lod_sub[khmc_k].resize(khmc_ns);
            if (!khmc_tab(khmc_d.lod_sub[khmc_k])) return false;
        }

        // Level 0's count is where level 1 begins (mesh_base_icount): it must lie inside the index stream (the
        // per-level test above implies it).
        if (khmc_ln > 0 && khmc_d.lod_istart[0] > khmc_ni) return false;
        // KH_CLOTH: the weight chunk, probed by its magic (no chunk precedes it since version 10; the distance-field
        // chunk of versions 8 and 9 is gone with the field, KH_SSAO). A file written before the chunk existed carries
        // none and the mesh re-imports its weights on the next cold load; a length that does not match the vertex
        // array is a chunk from another build: the offset rewinds, the skin chunk's magic test below then reads this
        // one's and fails, and the whole file is a miss (a re-import).
        khmc_d.cloth_w.clear();
        if (khmc_b.size() - khmc_o >= 8u) {
            uint32_t khmc_cm = 0, khmc_cn = 0;
            const size_t khmc_co = khmc_o;
            if (khmc_rd(&khmc_cm, 4) && khmc_rd(&khmc_cn, 4) && khmc_cm == KH_CLOTH_CACHE_MAGIC &&
                khmc_cn == khmc_nv && khmc_b.size() - khmc_o >= static_cast<uint64_t>(khmc_cn)) {
                khmc_d.cloth_w.resize(khmc_cn);
                if (!khmc_rd(khmc_d.cloth_w.data(), khmc_cn)) khmc_d.cloth_w.clear();
            } else {
                khmc_o = khmc_co;
            }
        }
        // KH_SKEL: the frame and skin chunk. Every import writes it, so a file without one (or with a malformed one)
        // is a miss: the box centre a skeletal binding places the model by would otherwise be lost silently.
        khmc_d.skin_bones.clear();
        khmc_d.skin_inf.clear();
        {
            uint32_t khmc_km = 0, khmc_kb = 0, khmc_ki = 0;
            if (!khmc_rd(&khmc_km, 4) || (khmc_km != KH_SKIN_CACHE_MAGIC && khmc_km != KH_SKIN_CACHE_MAGIC_KSK2)) {
                return false;
            }
            if (!khmc_rd(khmc_d.native_ctr, 12) || !khmc_rd(khmc_d.native_ext, 12)) return false;
            for (int k = 0; k < 3; ++k) {   // The skinning divides by the extent.
                if (!(khmc_d.native_ext[k] > 0.0f && khmc_d.native_ext[k] < 1.0e7f)) return false;
                if (!(fabsf(khmc_d.native_ctr[k]) < 1.0e7f)) return false;   // NaN fails too.
            }
            if (!khmc_rd(&khmc_kb, 4) || khmc_kb > static_cast<uint32_t>(KH_SKIN_BONES_MAX)) return false;
            khmc_d.skin_bones.resize(khmc_kb);
            for (uint32_t khmc_b = 0; khmc_b < khmc_kb; ++khmc_b) {
                KhSkinBone& khmc_sb = khmc_d.skin_bones[khmc_b];
                uint32_t khmc_nl = 0;
                if (!khmc_rd(&khmc_nl, 4) || khmc_nl > 1024u) return false;
                khmc_sb.name.resize(khmc_nl);
                if (khmc_nl && !khmc_rd(&khmc_sb.name[0], khmc_nl)) return false;
                if (!khmc_rd(&khmc_sb.parent, 4) || !khmc_rd(khmc_sb.head, 12)) return false;
                for (int k = 0; k < 3; ++k) {   // KH_IMPORT_BOUND: the import's bone bound (NaN fails too).
                    if (!(fabsf(khmc_sb.head[k]) < 1.0e7f)) return false;
                }
                if (khmc_sb.parent < -1 || khmc_sb.parent >= static_cast<int32_t>(khmc_kb) ||
                    khmc_sb.parent == static_cast<int32_t>(khmc_b)) return false;
            }
            if (!khmc_rd(&khmc_ki, 4) || (khmc_ki != 0u && khmc_ki != khmc_nv)) return false;
            if (khmc_ki != 0u && khmc_km != KH_SKIN_CACHE_MAGIC) return false;   // KH_LOD_SKIN: KSK2 influences.
            if (khmc_ki) {
                khmc_d.skin_inf.resize(khmc_ki);
                if (!khmc_rd(khmc_d.skin_inf.data(), static_cast<size_t>(khmc_ki) * sizeof(KhSkinInf))) return false;
                for (const KhSkinInf& khmc_f : khmc_d.skin_inf) {
                    for (int q = 0; q < 4; ++q) {
                        if (!(khmc_f.w[q] >= 0.0f && khmc_f.w[q] <= 1.0001f)) return false;   // NaN fails too.
                        if (khmc_f.w[q] > 0.0f && khmc_f.b[q] >= khmc_kb) return false;
                    }
                }
            }
        }
        // KH_OCC_BOX: the box chunk, last and required - taken only under a seal that matches this geometry, so a
        // cached box always belongs to exactly the triangles it was proven on. Anything else is a miss.
        {
            uint32_t khmc_om = 0, khmc_oo = 0;
            float khmc_or[12];
            uint64_t khmc_os = 0;
            if (!khmc_rd(&khmc_om, 4) || khmc_om != KH_OCC_CACHE_MAGIC || !khmc_rd(&khmc_oo, 4) ||
                !khmc_rd(khmc_or, 48) || !khmc_rd(&khmc_os, 8)) return false;
            if (!kh_mesh_occ_accept(khmc_d, khmc_oo, khmc_or, khmc_os)) return false;
        }

        if (khmc_own) {
            try {
                std::filesystem::last_write_time(khmc_p, std::filesystem::file_time_type::clock::now());
            } catch (...) {}
        }

        return true;
    } catch (...) { return false; }
}

// Full-validation load across every candidate: the Documents copy first, then every mod that ships the compiled
// form. A partially filled MeshDef from a rejected candidate is never carried forward.
inline bool kh_mesh_cache_load(uint64_t khmc_hash, MeshDef& khmc_d) {
    std::vector<std::filesystem::path> khmc_cand;
    const bool khmc_docs = !kh_mesh_cache_dir().empty();
    if (khmc_docs) khmc_cand.push_back(kh_mesh_cache_file(khmc_hash));
    kh_mod_cache_append(khmc_cand, kh_cache_file_name("khm_", khmc_hash, ".khmc"));

    for (size_t khmc_i = 0; khmc_i < khmc_cand.size(); ++khmc_i) {
        const bool khmc_own = khmc_i == 0 && khmc_docs;
        MeshDef khmc_try;

        if (kh_mesh_cache_load_one(khmc_cand[khmc_i], khmc_own, khmc_hash, khmc_try)) {
            khmc_d = std::move(khmc_try);
            return true;
        }
    }

    return false;
}

inline void kh_mesh_cache_store(uint64_t khmc_hash, const MeshDef& khmc_d) {
    if (kh_mesh_cache_dir().empty()) return;
    if (khmc_d.verts.empty() || khmc_d.indices.empty() || khmc_d.submeshes.empty()) return;

    std::filesystem::path khmc_tmp;   // KH_CACHE_TMP_WRITE.
    try {
        const std::filesystem::path khmc_p = kh_mesh_cache_file(khmc_hash);
        khmc_tmp = kh_cache_tmp_of(khmc_p);
        std::ofstream khmc_f(khmc_tmp, std::ios::binary | std::ios::trunc);
        if (!khmc_f) return;
        auto khmc_wr = [&](const void* khmc_src, size_t khmc_n) {
            khmc_f.write(static_cast<const char*>(khmc_src), static_cast<std::streamsize>(khmc_n));
        };
        const uint32_t khmc_nv = static_cast<uint32_t>(khmc_d.verts.size());
        const uint32_t khmc_ns = static_cast<uint32_t>(khmc_d.submeshes.size());
        const uint32_t khmc_ni = static_cast<uint32_t>(khmc_d.indices.size());
        khmc_wr(&KH_MESH_CACHE_MAGIC, 4);
        khmc_wr(&KH_MESH_CACHE_WORD, 4);   // KH_CACHE_LIB_VER.
        khmc_wr(&khmc_hash, 8);
        khmc_wr(khmc_d.native_size, 12);
        khmc_wr(&khmc_nv, 4);
        khmc_wr(&khmc_ns, 4);
        khmc_wr(&khmc_ni, 4);
        khmc_wr(khmc_d.verts.data(), static_cast<size_t>(khmc_nv) * sizeof(MeshVertex));
        khmc_wr(khmc_d.indices.data(), static_cast<size_t>(khmc_ni) * sizeof(uint32_t));

        auto khmc_tab = [&](const std::vector<MeshSubmesh>& khmc_t) {
            for (uint32_t khmc_i = 0; khmc_i < khmc_ns; ++khmc_i) {
                const MeshSubmesh& khmc_sm = khmc_t[khmc_i];
                const uint32_t khmc_nl = khmc_sm.name.size() > 1024u
                                       ? 1024u : static_cast<uint32_t>(khmc_sm.name.size());
                khmc_wr(&khmc_nl, 4);
                if (khmc_nl) khmc_wr(khmc_sm.name.data(), khmc_nl);
                khmc_wr(&khmc_sm.index_start, 4);
                khmc_wr(&khmc_sm.index_count, 4);
            }
        };
        khmc_tab(khmc_d.submeshes);
        // A level whose table cardinality disagrees with level 0 is not written at all; the reader validates the same.
        uint32_t khmc_ln = khmc_d.lod_n;

        for (uint32_t khmc_k = 0; khmc_k < khmc_ln; ++khmc_k) {
            if (khmc_d.lod_sub[khmc_k].size() != khmc_d.submeshes.size()) { khmc_ln = khmc_k; break; }
        }

        khmc_wr(&khmc_ln, 4);

        for (uint32_t khmc_k = 0; khmc_k < khmc_ln; ++khmc_k) {
            khmc_wr(&khmc_d.lod_istart[khmc_k], 4);
            khmc_wr(&khmc_d.lod_icount[khmc_k], 4);
            khmc_tab(khmc_d.lod_sub[khmc_k]);
        }

        // KH_CLOTH: the weight chunk, after the levels. Written only when the mesh carried authoring - an absent
        // chunk and an all-zero one mean different things (not cloth, versus fully pinned cloth).
        if (khmc_d.cloth_w.size() == khmc_d.verts.size() && !khmc_d.cloth_w.empty()) {
            const uint32_t khmc_cn = static_cast<uint32_t>(khmc_d.cloth_w.size());
            khmc_wr(&KH_CLOTH_CACHE_MAGIC, 4);
            khmc_wr(&khmc_cn, 4);
            khmc_wr(khmc_d.cloth_w.data(), khmc_cn);
        }
        // KH_SKEL: the frame and skin chunk, always (the loader requires it); the box chunk follows.
        {
            khmc_wr(&KH_SKIN_CACHE_MAGIC, 4);
            khmc_wr(khmc_d.native_ctr, 12);
            khmc_wr(khmc_d.native_ext, 12);
            const uint32_t khmc_kb = static_cast<uint32_t>(khmc_d.skin_bones.size());
            khmc_wr(&khmc_kb, 4);
            for (const KhSkinBone& khmc_sb : khmc_d.skin_bones) {
                const uint32_t khmc_nl = khmc_sb.name.size() > 1024u ? 1024u : static_cast<uint32_t>(khmc_sb.name.size());
                khmc_wr(&khmc_nl, 4);
                if (khmc_nl) khmc_wr(khmc_sb.name.data(), khmc_nl);
                khmc_wr(&khmc_sb.parent, 4);
                khmc_wr(khmc_sb.head, 12);
            }
            const uint32_t khmc_ki = khmc_d.skin_inf.size() == khmc_d.verts.size()
                                   ? static_cast<uint32_t>(khmc_d.skin_inf.size()) : 0u;
            khmc_wr(&khmc_ki, 4);
            if (khmc_ki) khmc_wr(khmc_d.skin_inf.data(), static_cast<size_t>(khmc_ki) * sizeof(KhSkinInf));
        }
        // KH_OCC_BOX: the box chunk, last and always (a mesh with no box records that too; the loader requires it).
        {
            const uint32_t khmc_oo = khmc_d.occ_ok ? 1u : 0u;
            float khmc_or[12];
            kh_mesh_occ_record(khmc_d, khmc_or);
            const uint64_t khmc_os = kh_mesh_occ_seal(khmc_d, khmc_oo, khmc_or);
            khmc_wr(&KH_OCC_CACHE_MAGIC, 4);
            khmc_wr(&khmc_oo, 4);
            khmc_wr(khmc_or, 48);
            khmc_wr(&khmc_os, 8);
        }
        kh_cache_tmp_commit(khmc_f, khmc_tmp, khmc_p);   // KH_CACHE_TMP_WRITE.
    } catch (...) {
        std::error_code khmc_ec;   // KH_CACHE_TMP_WRITE: a throw mid-write leaves no temp behind.
        if (!khmc_tmp.empty()) std::filesystem::remove(khmc_tmp, khmc_ec);
    }
}

// Registration tail shared by the ufbx import and the cache hit (kh_fbx_register): name the def, append, publish + GPU
// top-up under the park (deferred to the next injection or flush, kh_mesh_publish_land, when it cannot park). Game
// thread. g_mesh_last_ref: per id, GetTickCount64 of the last reference note (the mesh GC's input). KH_MESH_REF_LOCK:
// under g_mesh_ref_mu, which nests nothing but the store's append_mx (kh_mesh_gc's KH_GC_RECHECK tombstone; append_mx
// nests nothing): game-thread writers (flush_frame's census, kh_fbx_register) may resize it while kh_mesh_gc - which
// also writes it (its own walk's note) - reads it on the flush's thread (the render thread, or the game thread under
// the park).
static std::vector<uint64_t> g_mesh_last_ref;
static std::mutex g_mesh_ref_mu;

// Called for every object of every flush (visible or not: an invisible object still owns its mesh) and at import
// (kh_mesh_note_ref, game thread). KH_REF_NOTE_ONCE: a walk's reference notes, batched - the distinct non-builtin ids
// it saw (builtins are never collected: the GC's loop starts at builtin_n and their stamps are never read), stamped
// once under one lock at the walk's end. One record per walk (the census on the game thread, the GC's own on the
// flush's thread - the render thread, or the game thread under the park); each is that walk's scratch.
struct KhRefNote { std::vector<int> ids; std::vector<uint32_t> mark; uint32_t serial = 0; };
static KhRefNote g_ref_note_cen;   // flush_frame's census (game thread).
static KhRefNote g_ref_note_gc;    // kh_mesh_gc (the flush's thread).
inline void kh_ref_note_begin(KhRefNote& khrn_n) { if (++khrn_n.serial == 0u) khrn_n.serial = 1u; khrn_n.ids.clear(); }
inline void kh_ref_note_add(KhRefNote& khrn_n, int khrn_id) {
    if (khrn_id < 0 || static_cast<uint32_t>(khrn_id) < mesh_store().builtin_n) return;
    const size_t khrn_i = static_cast<size_t>(khrn_id);
    if (khrn_i >= khrn_n.mark.size()) khrn_n.mark.resize(khrn_i + 1u, 0u);
    if (khrn_n.mark[khrn_i] == khrn_n.serial) return;
    khrn_n.mark[khrn_i] = khrn_n.serial;
    khrn_n.ids.push_back(khrn_id);
}
inline void kh_ref_note_end(KhRefNote& khrn_n) {
    if (khrn_n.ids.empty()) return;
    std::lock_guard<std::mutex> khrn_g(g_mesh_ref_mu);
    const uint64_t khrn_now = GetTickCount64();
    for (int khrn_id : khrn_n.ids) {
        const size_t khrn_i = static_cast<size_t>(khrn_id);
        if (khrn_i >= g_mesh_last_ref.size()) g_mesh_last_ref.resize(khrn_i + 1, 0);
        g_mesh_last_ref[khrn_i] = khrn_now;
    }
    khrn_n.ids.clear();
}
inline void kh_mesh_note_ref(int khnr_id) {
    if (khnr_id < 0) return;
    const size_t khnr_i = static_cast<size_t>(khnr_id);
    std::lock_guard<std::mutex> khnr_g(g_mesh_ref_mu);
    if (khnr_i >= g_mesh_last_ref.size()) g_mesh_last_ref.resize(khnr_i + 1, 0);
    g_mesh_last_ref[khnr_i] = GetTickCount64();
}
// KH_GC_RECHECK - a new user's reference to a mesh it just resolved: stamped first, then the id read alive (or
// appended, its publish pending) again. kh_mesh_gc re-reads the stamp and tombstones in one g_mesh_ref_mu section, so
// either it saw this stamp and keeps the mesh, or its tombstone is visible here. False = released meanwhile: resolve
// again (a by-path or generated mesh is imported or built anew; a released id is gone). Game thread.
inline bool kh_mesh_take_ref(int khtr_id) {
    if (khtr_id < 0) return false;
    kh_mesh_note_ref(khtr_id);
    return kh_mesh_alive(khtr_id) || kh_mesh_def_staged(khtr_id) != nullptr;
}
}