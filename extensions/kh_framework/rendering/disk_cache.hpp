#pragma once

// The feature half (rendering_integration_features.hpp): the disk cache: its directories, the kind budgets, the atomic
// file commit, the trim and the writer thread. Never include it on its own.
namespace RenderIntegration {

struct KhTexBlob;

// Defined in rendering/mesh_cache.hpp (declared here for the uses below).
inline void kh_mesh_cache_store(uint64_t khmc_hash, const MeshDef& khmc_d);


// Mod folders are read-only cache sources; only Documents is written.

inline const std::vector<std::filesystem::path>& kh_mod_cache_dirs() {
    static const std::vector<std::filesystem::path> khmd_dirs = []() {
        std::vector<std::filesystem::path> khmd_v;

        try {
            khmd_v = ModFolderSearcher::find_directories_in_mods("cache");
        } catch (...) { khmd_v.clear(); }

        return khmd_v;
    }();
    return khmd_dirs;
}

// "khs_<16 hex>.khsc" / "khm_<16 hex>.khmc" / "kht_<16 hex>.khtc" - the one spelling (KH_CACHE_NAME_ONE) for the
// shader cache's file / probe pair and the mesh and texture caches' file / load pairs.
inline std::string kh_cache_file_name(const char* khcf_pfx, uint64_t khcf_h,
                                      const char* khcf_ext) {
    std::ostringstream khcf_ss;
    khcf_ss << khcf_pfx << std::hex << std::setfill('0') << std::setw(16) << khcf_h << khcf_ext;
    return khcf_ss.str();
}

inline void kh_mod_cache_append(std::vector<std::filesystem::path>& khma_out,
                                const std::string& khma_name) {

    try {
        for (const std::filesystem::path& khma_d : kh_mod_cache_dirs()) {
            std::filesystem::path khma_p = khma_d / khma_name;
            std::error_code khma_ec;

            if (std::filesystem::is_regular_file(khma_p, khma_ec)) khma_out.push_back(std::move(khma_p));
        }
    } catch (...) {}
}

// KH_CACHE_KIND_BUDGET - the disk cache's three kinds, each its own least-recently-used budget (kh_mesh_cache_trim);
// under one shared budget a mission's decoded textures evicted every mesh and shader blob before their next use (a
// texture is held as RGBA8 with its mip chain: ~85 MiB at 4096^2). Shader blobs: the 92 built-in jobs are ~4.7 MiB,
// a user material / effect variant ~0.4 MiB, so 64 MiB keeps the built-in set and ~140 user variants - nearly never
// evicted. Meshes: 1 GiB, the one budget all three shared, so no mesh set that fit before loses room. Textures: 2 GiB,
// ~24 decoded 4096^2 or ~96 2048^2 maps.
static constexpr uint64_t KH_CACHE_BUDGET_SHADER = 64ull << 20;
static constexpr uint64_t KH_CACHE_BUDGET_MESH = 1024ull << 20;
static constexpr uint64_t KH_CACHE_BUDGET_TEX = 2048ull << 20;
// KH_CACHE_SESSION_KEEP - the clean mission edge's time (kh_session_objects_reset; the DLL's load before the first):
// a file read or written since is this session's and is not evicted to make the budget - the access order repeats
// every session, so evicting by age alone takes each session's own first files to make room for its later ones and
// every file misses again next time. The session's own set may pass the budget up to twice it; past that the newest
// go first, which keeps the files the session reached first (they hit next session) instead of losing all of them.
static std::atomic<std::filesystem::file_time_type::rep> g_khmc_session_t0{
    std::filesystem::file_time_type::clock::now().time_since_epoch().count() };

// Documents unavailable or creation failed: the cache silently disables and every load is a plain import.
inline const std::filesystem::path& kh_mesh_cache_dir() {
    static const std::filesystem::path khmc_dir = []() {
        std::filesystem::path khmc_p;

        try {
            // KH_DOCS_WIDE: the wide call. Not the ANSI one: it hands a folder name outside the ANSI code page back
            // with '?' in it, the directory cannot be made, and every disk cache (meshes, textures, shaders) is off.
            wchar_t khmc_docs[MAX_PATH];

            if (SHGetFolderPathW(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, khmc_docs) == S_OK) {
                khmc_p = std::filesystem::path(khmc_docs) / "Arma 3" / "kh_framework" / "cache";
                std::filesystem::create_directories(khmc_p);
            }
        } catch (...) { khmc_p.clear(); }

        return khmc_p;
    }();
    return khmc_dir;
}

// KH_CACHE_TMP_WRITE - the mesh and texture caches write as the shader cache does (kh_shader_compile_raw): to a
// private '<name>.tmp<pid>', every write checked (the stream's state after the flush and after the close), then
// renamed over the name, so a failed write leaves nothing under it (the temp goes; a crash's temp,
// KH_CACHE_TMP_SWEEP) and each writer renames a whole file in. Do not write the name in place, unchecked: a failed or
// cut-off write (a full disk, a crash) stands under the name, and two game instances writing one file interleave -
// one truncates it under the other, whose next write leaves a zero-filled hole the reader's size check passes.
inline std::filesystem::path kh_cache_tmp_of(const std::filesystem::path& khct_p) {
    std::filesystem::path khct_t = khct_p;   // KH_DOCS_WIDE: appended to the path, as the shader cache's temp.
    khct_t += ".tmp" + std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
    return khct_t;
}
inline void kh_cache_tmp_commit(std::ofstream& khct_f, const std::filesystem::path& khct_tmp,
                                const std::filesystem::path& khct_p) {
    khct_f.flush();
    bool khct_ok = static_cast<bool>(khct_f);   // Any failed write left the stream failed.
    khct_f.close();
    khct_ok = khct_ok && !khct_f.fail();
    std::error_code khct_ec;
    if (khct_ok) std::filesystem::rename(khct_tmp, khct_p, khct_ec);   // A sharing violation keeps the old file.
    if (!khct_ok || khct_ec) std::filesystem::remove(khct_tmp, khct_ec);
}

// Least-recently-used eviction down to each kind's budget (KH_CACHE_KIND_BUDGET), after every mesh or texture store
// and, for shader blobs, once per KH_CACHE_SHADER_TRIM_SLACK bytes written (KH_CACHE_TRIM_THROTTLE); a fresh write
// and every hit of every kind (kh_shader_compile_raw, kh_mesh_cache_load_one, kh_tex_cache_load_one) carry the
// newest time. KH_CACHE_SESSION_KEEP: this session's files are kept past the budget, to twice it.
inline void kh_mesh_cache_trim() {
    try {
        const std::filesystem::path& khmc_dir = kh_mesh_cache_dir();
        if (khmc_dir.empty()) return;

        struct KhmcEnt {
            std::filesystem::path p;
            uint64_t sz;
            std::filesystem::file_time_type t;
        };

        std::vector<KhmcEnt> khmc_ents[3];   // KH_CACHE_KIND_BUDGET: .khsc, .khmc, .khtc.
        uint64_t khmc_total[3] = { 0u, 0u, 0u };
        static constexpr uint64_t khmc_budget[3] = { KH_CACHE_BUDGET_SHADER, KH_CACHE_BUDGET_MESH,
                                                     KH_CACHE_BUDGET_TEX };

        // KH_CACHE_KIND_BUDGET: the enumeration and each entry's attributes through the error_code overloads - an
        // entry no stat can read (a file another process holds, a reparse point) is skipped (the throwing forms end
        // the whole trim at the catch below, and no budget holds while the obstacle stands). A folder that cannot be
        // opened or read on (an increment's error: the iterator is then the end) still ends it.
        std::error_code khmc_dec;
        std::filesystem::directory_iterator khmc_di(khmc_dir, khmc_dec);
        for (; !khmc_dec && khmc_di != std::filesystem::directory_iterator(); khmc_di.increment(khmc_dec)) {
            const std::filesystem::directory_entry& khmc_e = *khmc_di;
            std::error_code khmc_eec;
            if (!khmc_e.is_regular_file(khmc_eec) || khmc_eec) continue;
            // KH_CACHE_TMP_SWEEP - every cache file is written to '<name>.<ext>.tmp<pid>' and renamed into place
            // (kh_shader_compile_raw; KH_CACHE_TMP_WRITE); a process that dies or is closed between the two leaves the
            // temp file for good, under an extension no budget counts. One older than an hour is no live writer's (a
            // write takes milliseconds) and goes; a younger one may be another process's in flight and is left. Matched
            // on the native string: path::string() throws on a name the code page cannot hold, and a throw here ends
            // the whole trim.
            const std::filesystem::path khmc_fn = khmc_e.path().filename();
            auto khmc_has = [&khmc_fn](const char* khmc_tag) {
                return khmc_fn.native().find(std::filesystem::path(khmc_tag).native()) !=
                       std::filesystem::path::string_type::npos;
            };
            if (khmc_has(".khsc.tmp") || khmc_has(".khmc.tmp") || khmc_has(".khtc.tmp")) {
                std::error_code khmc_tec;
                const auto khmc_tt = std::filesystem::last_write_time(khmc_e.path(), khmc_tec);
                if (!khmc_tec && std::filesystem::file_time_type::clock::now() - khmc_tt > std::chrono::hours(1)) {
                    std::filesystem::remove(khmc_e.path(), khmc_tec);
                }
                continue;
            }
            const std::filesystem::path khmc_x = khmc_e.path().extension();
            const int khmc_kind = khmc_x == ".khsc" ? 0 : khmc_x == ".khmc" ? 1 : khmc_x == ".khtc" ? 2 : -1;
            if (khmc_kind < 0) continue;
            const uint64_t khmc_fs = static_cast<uint64_t>(khmc_e.file_size(khmc_eec));
            if (khmc_eec) continue;
            const std::filesystem::file_time_type khmc_ft = khmc_e.last_write_time(khmc_eec);
            if (khmc_eec) continue;
            KhmcEnt khmc_k{ khmc_e.path(), khmc_fs, khmc_ft };
            khmc_total[khmc_kind] += khmc_k.sz;
            khmc_ents[khmc_kind].push_back(std::move(khmc_k));
        }
        if (khmc_dec) return;   // The folder itself could not be read on: no eviction from a partial listing.

        const std::filesystem::file_time_type::rep khmc_t0 = g_khmc_session_t0.load(std::memory_order_relaxed);
        for (int khmc_kind = 0; khmc_kind < 3; ++khmc_kind) {
            uint64_t& khmc_sum = khmc_total[khmc_kind];
            const uint64_t khmc_cap = khmc_budget[khmc_kind];
            if (khmc_sum <= khmc_cap) continue;
            std::vector<KhmcEnt>& khmc_v = khmc_ents[khmc_kind];
            std::sort(khmc_v.begin(), khmc_v.end(), [](const KhmcEnt& a, const KhmcEnt& b) { return a.t < b.t; });

            // Earlier sessions' files, oldest first, down to the budget.
            for (const auto& khmc_k : khmc_v) {
                if (khmc_sum <= khmc_cap) break;
                if (khmc_k.t.time_since_epoch().count() >= khmc_t0) break;   // Sorted: the rest are this session's.
                std::error_code khmc_ec;

                if (std::filesystem::remove(khmc_k.p, khmc_ec)) {
                    khmc_sum -= khmc_k.sz;
                }
            }
            // KH_CACHE_SESSION_KEEP: this session's own files, newest first, down to twice the budget.
            for (auto khmc_it = khmc_v.rbegin(); khmc_it != khmc_v.rend(); ++khmc_it) {
                if (khmc_sum <= 2u * khmc_cap) break;
                if (khmc_it->t.time_since_epoch().count() < khmc_t0) break;   // Only earlier sessions' remain.
                std::error_code khmc_ec;

                if (std::filesystem::remove(khmc_it->p, khmc_ec)) {
                    khmc_sum -= khmc_it->sz;
                }
            }
        }
    } catch (...) {}
}

// The mesh cache writer (kh_mesh_cache_writer, below). A published MeshDef never moves and is immutable, so the
// writer reads it without a lock. It can die (kh_mesh_gc releases an idle mesh), but never while the writer holds or
// has queued it: the busy id is set under g_khmw_mu as the job leaves the queue, and kh_mesh_write_busy reads both
// under it.
struct KhTexBlob {   // KH_TEX_CACHE: one decoded RGBA8 texture, every mip level.
    uint32_t w = 0, h = 0, mips = 0;
    bool srgb = false;
    std::vector<uint32_t> level_off;   // Byte offset of each level into data (pitch = level width *
                                       // 4).
    std::vector<uint8_t> data;
};
struct KhMeshWriteJob { uint64_t hash; int id; std::shared_ptr<const KhTexBlob> tex; };   // tex set = a .khtc write.
static std::vector<KhMeshWriteJob> g_khmw_q;
static std::mutex g_khmw_mu;
static std::condition_variable g_khmw_cv;
// Deliberately never destructed (the shader pool's rule): on a process exit the OS kills the thread but the object
// stays joinable, and a joinable std::thread's static destructor is std::terminate under the loader lock.
static std::thread& g_khmw_thr = *(new std::thread());
static bool g_khmw_running = false;
static std::atomic<bool> g_khmw_abort{false};
static std::atomic<int>  g_khmw_busy_id{-1};   // KH_MESH_FREE: the def the writer is reading now.

inline void kh_tex_cache_store(uint64_t khtc_hash, const KhTexBlob& khtc_b);
inline void kh_mesh_cache_writer() {
    for (;;) {
        KhMeshWriteJob khmw_j;
        {
            std::unique_lock<std::mutex> khmw_l(g_khmw_mu);
            g_khmw_cv.wait(khmw_l, [] {
                return !g_khmw_q.empty() || g_khmw_abort.load(std::memory_order_relaxed);
            });
            // KH_KHMW_ABORT_STOP: a stop request ends the writer before its next job, as the texture loader's. Do not
            // test 'abort and an empty queue': a process detach sets the flag and cannot take the mutex to clear the
            // queue, so the writer would write its whole backlog (multi-MB texture blobs) until the bounded wait
            // terminates it mid-write. kh_mesh_cache_writer_stop empties the queue first.
            if (g_khmw_abort.load(std::memory_order_relaxed)) return;
            if (g_khmw_q.empty()) continue;
            khmw_j = g_khmw_q.front();
            g_khmw_q.erase(g_khmw_q.begin());
            // KH_MESH_FREE: busy before the lock drops, so the job is never in neither place.
            if (!khmw_j.tex) g_khmw_busy_id.store(khmw_j.id, std::memory_order_relaxed);
        }
        try {
            // Bounds and liveness re-checked on this thread (kh_mesh_alive: the published count, then the slot's
            // tombstone), so a stale id, or one released since it was queued (KH_MESH_FREE), cannot become a wild read.
            if (khmw_j.tex) {   // KH_TEX_CACHE write (defined in rendering/texture_cache.hpp; same thread, same trim).
                kh_tex_cache_store(khmw_j.hash, *khmw_j.tex);
                std::lock_guard<std::mutex> khmw_tl(g_khsm_trim_mu);
                kh_mesh_cache_trim();
            } else if (kh_mesh_alive(khmw_j.id)) {
                kh_mesh_cache_store(khmw_j.hash, mesh_def(khmw_j.id));
                g_khmw_busy_id.store(-1, std::memory_order_release);   // Done with the def.
                std::lock_guard<std::mutex> khmw_tl(g_khsm_trim_mu);
                kh_mesh_cache_trim();
            }
        } catch (...) {}
        if (!khmw_j.tex) g_khmw_busy_id.store(-1, std::memory_order_release);   // A dead id, or a throw.
    }
}

// Started on first use. A spawn failure is non-fatal and falls back to writing inline.
inline bool kh_mesh_cache_writer_start() {
    std::lock_guard<std::mutex> khmw_l(g_khmw_mu);
    if (g_khmw_running) return true;
    g_khmw_abort.store(false, std::memory_order_relaxed);
    try {
        g_khmw_thr = std::thread(kh_mesh_cache_writer);
    } catch (...) {
        return false;
    }
    g_khmw_running = true;
    return true;
}

// Interruptible between jobs (no multi-minute call), so joined, never detached.
inline void kh_mesh_cache_writer_stop() {
    {
        std::lock_guard<std::mutex> khmw_l(g_khmw_mu);
        if (!g_khmw_running) return;
        g_khmw_q.clear();
        g_khmw_abort.store(true, std::memory_order_relaxed);
    }
    g_khmw_cv.notify_all();
    if (g_khmw_thr.joinable()) { try { g_khmw_thr.join(); } catch (...) {} }
    std::lock_guard<std::mutex> khmw_l2(g_khmw_mu);
    g_khmw_running = false;
}

inline void kh_mesh_cache_queue(uint64_t khmw_h, int khmw_id) {
    if (!kh_mesh_cache_writer_start()) {
        kh_mesh_cache_store(khmw_h, mesh_def(khmw_id));
        std::lock_guard<std::mutex> khmw_tl(g_khsm_trim_mu);
        kh_mesh_cache_trim();
        return;
    }
    {
        std::lock_guard<std::mutex> khmw_l(g_khmw_mu);
        KhMeshWriteJob khmw_j = { khmw_h, khmw_id, nullptr };
        g_khmw_q.push_back(khmw_j);
    }
    g_khmw_cv.notify_one();
}
// KH_MESH_CACHE_LATER: an import whose publish kh_fbx_register had to defer (the graphics lock refused three times)
// has no published id until the next injection or flush lands it (g_mesh_publish_pending), and the writer drops an
// unpublished id - so its disk-cache write waits here and is queued where that publish lands (kh_mesh_publish_land:
// the injection ahead of its scene sync, or flush_locked's housekeeping). A landing between the register and the
// stash is caught by the stash itself (the id already published: it queues at once). An id released meanwhile is
// dropped (the writer's inline fallback would read mesh 0 in its place). One appended but not yet published - its
// register lost the lock after this landing's publish ran, and its pending flag stands - waits for the next landing
// (KH_MESH_STAGED). Emptied with the pending flag (kh_mesh_release_session).
static std::mutex g_mesh_cache_later_mu;
static std::vector<std::pair<uint64_t, int>> g_mesh_cache_later_q;   // (content hash, id), under the mutex.
inline void kh_mesh_cache_later_land() {
    std::vector<std::pair<uint64_t, int>> khml_q;
    {
        std::lock_guard<std::mutex> khml_g(g_mesh_cache_later_mu);
        khml_q.swap(g_mesh_cache_later_q);
    }
    std::vector<std::pair<uint64_t, int>> khml_wait;
    for (const auto& khml_e : khml_q) {
        if (kh_mesh_alive(khml_e.second)) kh_mesh_cache_queue(khml_e.first, khml_e.second);
        else if (kh_mesh_def_staged(khml_e.second)) khml_wait.push_back(khml_e);   // KH_MESH_STAGED: not yet published.
    }
    if (!khml_wait.empty()) {
        std::lock_guard<std::mutex> khml_g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.insert(g_mesh_cache_later_q.end(), khml_wait.begin(), khml_wait.end());
    }
}
inline void kh_mesh_cache_later(uint64_t khml_h, int khml_id) {
    {
        std::lock_guard<std::mutex> khml_g(g_mesh_cache_later_mu);
        g_mesh_cache_later_q.emplace_back(khml_h, khml_id);
    }
    if (kh_mesh_alive(khml_id)) kh_mesh_cache_later_land();   // Published since the register: no landing to wait for.
}
}