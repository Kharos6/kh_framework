#pragma once

// The feature half (rendering_integration_features.hpp): the disk cache: its directories, the kind budgets, the atomic
// file commit, the trim and the writer thread. Never include it on its own.
namespace RenderIntegration {

// Defined in rendering/mesh_cache.hpp (declared here for the uses below).
inline void mesh_cache_store(uint64_t hash, const MeshDef& d);


// Mod folders are read-only cache sources; only Documents is written.

inline const std::vector<std::filesystem::path>& mod_cache_dirs() {
    static const std::vector<std::filesystem::path> dirs = []() {
        std::vector<std::filesystem::path> v;

        try {
            v = ModFolderSearcher::find_directories_in_mods("cache");
        } catch (...) { v.clear(); }

        return v;
    }();
    return dirs;
}

// "khs_<16 hex>.khsc" / "khm_<16 hex>.khmc" / "kht_<16 hex>.khtc" - the one spelling (CACHE_NAME_ONE) for the
// shader cache's file / probe pair and the mesh and texture caches' file / load pairs.
inline std::string cache_file_name(const char* pfx, uint64_t h,
                                   const char* ext) {
    std::ostringstream ss;
    ss << pfx << std::hex << std::setfill('0') << std::setw(16) << h << ext;
    return ss.str();
}

inline void mod_cache_append(std::vector<std::filesystem::path>& out,
                             const std::string& name) {

    try {
        for (const std::filesystem::path& d : mod_cache_dirs()) {
            std::filesystem::path p = d / name;
            std::error_code ec;

            if (std::filesystem::is_regular_file(p, ec)) out.push_back(std::move(p));
        }
    } catch (...) {}
}

// CACHE_KIND_BUDGET - the disk cache's three kinds, each its own least-recently-used budget (mesh_cache_trim);
// under one shared budget a mission's decoded textures evicted every mesh and shader blob before their next use (a
// texture is held as RGBA8 with its mip chain: ~85 MiB at 4096^2). Shader blobs: the 92 built-in jobs are ~4.7 MiB,
// a user material / effect variant ~0.4 MiB, so 64 MiB keeps the built-in set and ~140 user variants - nearly never
// evicted. Meshes: 1 GiB, the one budget all three shared, so no mesh set that fit before loses room. Textures: 2 GiB,
// ~24 decoded 4096^2 or ~96 2048^2 maps.
static constexpr uint64_t CACHE_BUDGET_SHADER = 64ull << 20;
static constexpr uint64_t CACHE_BUDGET_MESH = 1024ull << 20;
static constexpr uint64_t CACHE_BUDGET_TEX = 2048ull << 20;
// CACHE_SESSION_KEEP - the clean mission edge's time (session_objects_reset; the DLL's load before the first):
// a file read or written since is this session's and is not evicted to make the budget - the access order repeats
// every session, so evicting by age alone takes each session's own first files to make room for its later ones and
// every file misses again next time. The session's own set may pass the budget up to twice it; past that the newest
// go first, which keeps the files the session reached first (they hit next session) instead of losing all of them.
static std::atomic<std::filesystem::file_time_type::rep> g_meshcache_session_t0{
    std::filesystem::file_time_type::clock::now().time_since_epoch().count() };

// Documents unavailable or creation failed: the cache silently disables and every load is a plain import.
inline const std::filesystem::path& mesh_cache_dir() {
    static const std::filesystem::path dir = []() {
        std::filesystem::path p;

        try {
            // DOCS_WIDE: the wide call. Not the ANSI one: it hands a folder name outside the ANSI code page back
            // with '?' in it, the directory cannot be made, and every disk cache (meshes, textures, shaders) is off.
            wchar_t docs[MAX_PATH];

            if (SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, docs) == S_OK) {
                p = std::filesystem::path(docs) / "Arma 3" / "kh_framework" / "cache";
                std::filesystem::create_directories(p);
            }
        } catch (...) { p.clear(); }

        return p;
    }();
    return dir;
}

// CACHE_TMP_WRITE - the mesh and texture caches write as the shader cache does (shader_compile_raw): to a
// private '<name>.tmp<pid>', every write checked (the stream's state after the flush and after the close), then
// renamed over the name, so a failed write leaves nothing under it (the temp goes; a crash's temp,
// CACHE_TMP_SWEEP) and each writer renames a whole file in. Do not write the name in place, unchecked: a failed or
// cut-off write (a full disk, a crash) stands under the name, and two game instances writing one file interleave -
// one truncates it under the other, whose next write leaves a zero-filled hole the reader's size check passes.
inline std::filesystem::path cache_tmp_of(const std::filesystem::path& p) {
    std::filesystem::path t = p;   // DOCS_WIDE: appended to the path, as the shader cache's temp.
    t += ".tmp" + std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
    return t;
}
inline void cache_tmp_commit(std::ofstream& f, const std::filesystem::path& tmp,
                             const std::filesystem::path& p) {
    f.flush();
    bool ok = static_cast<bool>(f);   // Any failed write left the stream failed.
    f.close();
    ok = ok && !f.fail();
    std::error_code ec;
    if (ok) std::filesystem::rename(tmp, p, ec);   // A sharing violation keeps the old file.
    if (!ok || ec) std::filesystem::remove(tmp, ec);
}

// Least-recently-used eviction down to each kind's budget (CACHE_KIND_BUDGET), after every mesh or texture store
// and, for shader blobs, once per CACHE_SHADER_TRIM_SLACK bytes written (CACHE_TRIM_THROTTLE); a fresh write
// and every hit of every kind (shader_compile_raw, mesh_cache_load_one, tex_cache_load_one) carry the
// newest time. CACHE_SESSION_KEEP: this session's files are kept past the budget, to twice it.
inline void mesh_cache_trim() {
    try {
        const std::filesystem::path& dir = mesh_cache_dir();
        if (dir.empty()) return;

        struct CacheEnt {
            std::filesystem::path p;
            uint64_t sz;
            std::filesystem::file_time_type t;
        };

        std::vector<CacheEnt> ents[3];   // CACHE_KIND_BUDGET: .khsc, .khmc, .khtc.
        uint64_t total[3] = { 0u, 0u, 0u };
        static constexpr uint64_t budget[3] = { CACHE_BUDGET_SHADER, CACHE_BUDGET_MESH,
                                                CACHE_BUDGET_TEX };

        // CACHE_KIND_BUDGET: the enumeration and each entry's attributes through the error_code overloads - an
        // entry no stat can read (a file another process holds, a reparse point) is skipped (the throwing forms end
        // the whole trim at the catch below, and no budget holds while the obstacle stands). A folder that cannot be
        // opened or read on (an increment's error: the iterator is then the end) still ends it.
        std::error_code dec;
        std::filesystem::directory_iterator di(dir, dec);
        for (; !dec && di != std::filesystem::directory_iterator(); di.increment(dec)) {
            const std::filesystem::directory_entry& e = *di;
            std::error_code eec;
            if (!e.is_regular_file(eec) || eec) continue;
            // CACHE_TMP_SWEEP - every cache file is written to '<name>.<ext>.tmp<pid>' and renamed into place
            // (shader_compile_raw; CACHE_TMP_WRITE); a process that dies or is closed between the two leaves the
            // temp file for good, under an extension no budget counts. One older than an hour is no live writer's (a
            // write takes milliseconds) and goes; a younger one may be another process's in flight and is left. Matched
            // on the native string: path::string() throws on a name the code page cannot hold, and a throw here ends
            // the whole trim.
            const std::filesystem::path fn = e.path().filename();
            auto has = [&fn](const char* tag) {
                return fn.native().find(std::filesystem::path(tag).native()) !=
                       std::filesystem::path::string_type::npos;
            };
            if (has(".khsc.tmp") || has(".khmc.tmp") || has(".khtc.tmp")) {
                std::error_code tec;
                const auto tt = std::filesystem::last_write_time(e.path(), tec);
                if (!tec && std::filesystem::file_time_type::clock::now() - tt > std::chrono::hours(1)) {
                    std::filesystem::remove(e.path(), tec);
                }
                continue;
            }
            const std::filesystem::path x = e.path().extension();
            const int kind = x == ".khsc" ? 0 : x == ".khmc" ? 1 : x == ".khtc" ? 2 : -1;
            if (kind < 0) continue;
            const uint64_t fs = static_cast<uint64_t>(e.file_size(eec));
            if (eec) continue;
            const std::filesystem::file_time_type ft = e.last_write_time(eec);
            if (eec) continue;
            CacheEnt k{ e.path(), fs, ft };
            total[kind] += k.sz;
            ents[kind].push_back(std::move(k));
        }
        if (dec) return;   // The folder itself could not be read on: no eviction from a partial listing.

        const std::filesystem::file_time_type::rep t0 = g_meshcache_session_t0.load(std::memory_order_relaxed);
        for (int kind = 0; kind < 3; ++kind) {
            uint64_t& sum = total[kind];
            const uint64_t cap = budget[kind];
            if (sum <= cap) continue;
            std::vector<CacheEnt>& v = ents[kind];
            std::sort(v.begin(), v.end(), [](const CacheEnt& a, const CacheEnt& b) { return a.t < b.t; });

            // Earlier sessions' files, oldest first, down to the budget.
            for (const auto& k : v) {
                if (sum <= cap) break;
                if (k.t.time_since_epoch().count() >= t0) break;   // Sorted: the rest are this session's.
                std::error_code ec;

                if (std::filesystem::remove(k.p, ec)) {
                    sum -= k.sz;
                }
            }
            // CACHE_SESSION_KEEP: this session's own files, newest first, down to twice the budget.
            for (auto it = v.rbegin(); it != v.rend(); ++it) {
                if (sum <= 2u * cap) break;
                if (it->t.time_since_epoch().count() < t0) break;   // Only earlier sessions' remain.
                std::error_code ec;

                if (std::filesystem::remove(it->p, ec)) {
                    sum -= it->sz;
                }
            }
        }
    } catch (...) {}
}

// The mesh cache writer (mesh_cache_writer, below). A published MeshDef never moves and is immutable, so the
// writer reads it without a lock. It can die (mesh_gc releases an idle mesh), but never while the writer holds or
// has queued it: the busy id is set under g_meshwriter_mu as the job leaves the queue, and mesh_write_busy reads both
// under it.
struct TexBlob {   // TEX_CACHE: one decoded RGBA8 texture, every mip level.
    uint32_t w = 0, h = 0, mips = 0;
    bool srgb = false;
    std::vector<uint32_t> level_off;   // Byte offset of each level into data (pitch = level width *
                                       // 4).
    std::vector<uint8_t> data;
};
struct MeshWriteJob { uint64_t hash; int id; std::shared_ptr<const TexBlob> tex; };   // tex set = a .khtc write.
static std::vector<MeshWriteJob> g_meshwriter_q;
static std::mutex g_meshwriter_mu;
static std::condition_variable g_meshwriter_cv;
// Deliberately never destructed (the shader pool's rule): on a process exit the OS kills the thread but the object
// stays joinable, and a joinable std::thread's static destructor is std::terminate under the loader lock.
static std::thread& g_meshwriter_thr = *(new std::thread());
static bool g_meshwriter_running = false;
static std::atomic<bool> g_meshwriter_abort{false};
static std::atomic<int>  g_meshwriter_busy_id{-1};   // MESH_FREE: the def the writer is reading now.

inline void tex_cache_store(uint64_t hash, const TexBlob& b);
inline void mesh_cache_writer() {
    for (;;) {
        MeshWriteJob j;
        {
            std::unique_lock<std::mutex> l(g_meshwriter_mu);
            g_meshwriter_cv.wait(l, [] {
                return !g_meshwriter_q.empty() || g_meshwriter_abort.load(std::memory_order_relaxed);
            });
            // MESHWRITER_ABORT_STOP: a stop request ends the writer before its next job, as the texture loader's. Do
            // not test 'abort and an empty queue': a process detach sets the flag and cannot take the mutex to clear
            // the queue, so the writer would write its whole backlog (multi-MB texture blobs) until the bounded wait
            // terminates it mid-write. mesh_cache_writer_stop empties the queue first.
            if (g_meshwriter_abort.load(std::memory_order_relaxed)) return;
            if (g_meshwriter_q.empty()) continue;
            j = g_meshwriter_q.front();
            g_meshwriter_q.erase(g_meshwriter_q.begin());
            // MESH_FREE: busy before the lock drops, so the job is never in neither place.
            if (!j.tex) g_meshwriter_busy_id.store(j.id, std::memory_order_relaxed);
        }
        try {
            // Bounds and liveness re-checked on this thread (mesh_alive: the published count, then the slot's
            // tombstone), so a stale id, or one released since it was queued (MESH_FREE), cannot become a wild read.
            if (j.tex) {   // TEX_CACHE write (defined in rendering/texture_cache.hpp; same thread, same trim).
                tex_cache_store(j.hash, *j.tex);
                std::lock_guard<std::mutex> tl(g_shpool_trim_mu);
                mesh_cache_trim();
            } else if (mesh_alive(j.id)) {
                mesh_cache_store(j.hash, mesh_def(j.id));
                g_meshwriter_busy_id.store(-1, std::memory_order_release);   // Done with the def.
                std::lock_guard<std::mutex> tl(g_shpool_trim_mu);
                mesh_cache_trim();
            }
        } catch (...) {}
        if (!j.tex) g_meshwriter_busy_id.store(-1, std::memory_order_release);   // A dead id, or a throw.
    }
}

// Started on first use. A spawn failure is non-fatal and falls back to writing inline.
inline bool mesh_cache_writer_start() {
    std::lock_guard<std::mutex> l(g_meshwriter_mu);
    if (g_meshwriter_running) return true;
    g_meshwriter_abort.store(false, std::memory_order_relaxed);
    try {
        g_meshwriter_thr = std::thread(mesh_cache_writer);
    } catch (...) {
        return false;
    }
    g_meshwriter_running = true;
    return true;
}

// Interruptible between jobs (no multi-minute call), so joined, never detached.
inline void mesh_cache_writer_stop() {
    {
        std::lock_guard<std::mutex> l(g_meshwriter_mu);
        if (!g_meshwriter_running) return;
        g_meshwriter_q.clear();
        g_meshwriter_abort.store(true, std::memory_order_relaxed);
    }
    g_meshwriter_cv.notify_all();
    if (g_meshwriter_thr.joinable()) { try { g_meshwriter_thr.join(); } catch (...) {} }
    std::lock_guard<std::mutex> l2(g_meshwriter_mu);
    g_meshwriter_running = false;
}

// EA_MW_INLINE: only hands the write to a running writer, else drops it (it is only a cache), so the publish
// landing (the render thread) never writes a mesh-cache file or (re)spawns the writer. The import starts the writer,
// or writes inline on the game thread when it cannot (fbx_import).
inline void mesh_cache_queue(uint64_t h, int id) {
    {
        std::lock_guard<std::mutex> l(g_meshwriter_mu);
        if (!g_meshwriter_running) return;
        MeshWriteJob j = { h, id, nullptr };
        g_meshwriter_q.push_back(j);
    }
    g_meshwriter_cv.notify_one();
}
// MESH_CACHE_LATER: an import (fbx_register defers every publish, EA_FBX_NO_PARK) has no published id until
// the next injection or flush lands it (g_mesh_publish_pending), and the writer drops an unpublished id - so its
// disk-cache write waits here and is queued where that publish lands (mesh_publish_land: the injection ahead of its
// scene sync, or flush_locked's housekeeping). A landing between the register and the stash is caught by the stash
// itself (the id already published: it queues at once). An id released meanwhile is dropped, and so is every write
// while no writer runs (mesh_cache_queue, EA_MW_INLINE). One appended but not yet published - its register ran
// after this landing's publish, and its pending flag stands - waits for the next landing (MESH_STAGED). Emptied
// with the pending flag (mesh_release_session).
static std::mutex g_mesh_cache_later_mu;
static std::vector<std::pair<uint64_t, int>> g_mesh_cache_later_q;   // (content hash, id), under the mutex.
inline void mesh_cache_later_land() {
    std::vector<std::pair<uint64_t, int>> q;
    {
        std::lock_guard<std::mutex> g(g_mesh_cache_later_mu);
        q.swap(g_mesh_cache_later_q);
    }
    std::vector<std::pair<uint64_t, int>> wait;
    for (const auto& e : q) {
        if (mesh_alive(e.second)) mesh_cache_queue(e.first, e.second);
        else if (mesh_def_staged(e.second)) wait.push_back(e);   // MESH_STAGED: not yet published.
    }
    if (!wait.empty()) {
        std::lock_guard<std::mutex> g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.insert(g_mesh_cache_later_q.end(), wait.begin(), wait.end());
    }
}
inline void mesh_cache_later(uint64_t h, int id) {
    {
        std::lock_guard<std::mutex> g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.emplace_back(h, id);
    }
    if (mesh_alive(id)) mesh_cache_later_land();   // Published since the register: no landing to wait for.
}
}