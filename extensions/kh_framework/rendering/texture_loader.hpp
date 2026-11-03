#pragma once

// The feature half (rendering_integration_features.hpp): the async texture loader thread. Never include it on its own.
namespace RenderIntegration {

// KH_TEX_ASYNC: the texture loader worker is disk + CPU only (never touches the device), so it is engine-reset-safe
// without a stop. The device half - create and page placement - runs in kh_tex_async_pump at the pass heads (the
// injection, the scene flush, the UI flush), each inside the engine's own one-thread-at-a-time use of the immediate
// context or under a park. Lazy start, joined at mission end.
struct KhTexLoadJob { std::string key; std::string path; bool srgb; bool force; };
struct KhTexLoadDone {
    std::string key;
    std::string path;
    bool srgb = false;
    bool from_cache = false;
    std::shared_ptr<KhTexBlob> blob;   // The stb route's results, and a .dds / .paa KH_DDS_MIP_FILL built a chain for.
    std::vector<uint8_t> dds;          // .dds raw file bytes, or a .paa's made from it (parsed at the pump).
    std::string err;
};
static std::mutex g_khtl_mu;
static std::condition_variable g_khtl_cv;
static std::vector<KhTexLoadJob> g_khtl_q;
static std::vector<KhTexLoadDone> g_khtl_done;
static std::thread& g_khtl_thr = *(new std::thread());   // Never destructed (see g_khmw_thr).
static bool g_khtl_running = false;
static std::atomic<bool> g_khtl_abort{ false };

inline void kh_texldr_worker() {
    for (;;) {
        KhTexLoadJob khtl_j;
        {
            std::unique_lock<std::mutex> khtl_l(g_khtl_mu);
            g_khtl_cv.wait(khtl_l, [] { return g_khtl_abort.load(std::memory_order_relaxed) || !g_khtl_q.empty(); });
            if (g_khtl_abort.load(std::memory_order_relaxed)) return;
            khtl_j = std::move(g_khtl_q.front());
            g_khtl_q.erase(g_khtl_q.begin());
        }
        KhTexLoadDone khtl_d;
        khtl_d.key = std::move(khtl_j.key);
        khtl_d.path = std::move(khtl_j.path);
        khtl_d.srgb = khtl_j.srgb;
        // KH_TEXLDR_GUARD: an exception leaving this thread body is std::terminate. A throwing job fails into
        // its done record (an error, no payload), which kh_tex_async_pump turns into the entry's failed state.
        // The messages are short enough for the string's own buffer: setting them allocates nothing.
        const char* khtl_fail = nullptr;
        try {
            if (kh_ends_with_ci(khtl_d.path, ".dds")) {   // Same read + guard as the sync path.
                // PBO_PATH: kh_asset_read. KH_EA_TEX_READ_WHY: a failed read leaves no bytes and says why in err.
                if (kh_asset_read(khtl_d.path, khtl_d.dds, 256ull * 1024u * 1024u, &khtl_d.err)) {
                    if (!kh_dds_unblock(khtl_d.dds, khtl_d.err)) {   // KH_DDS_BLOCK4: a refusal drops the bytes.
                        std::vector<uint8_t>().swap(khtl_d.dds);
                    } else if (kh_dds_mip_fill(khtl_d.dds, khtl_d.srgb, khtl_d.blob)) {   // KH_DDS_MIP_FILL: the blob.
                        std::vector<uint8_t>().swap(khtl_d.dds);
                    }
                }
            } else if (kh_paa_ext(khtl_d.path)) {   // KH_PAA: the sync path's twin - .dds bytes for the pump.
                std::vector<uint8_t> khtl_paa;
                if (kh_asset_read(khtl_d.path, khtl_paa, 256ull * 1024u * 1024u, &khtl_d.err)) {   // As above.
                    if (!kh_paa_to_dds(khtl_paa, khtl_d.dds, khtl_d.err)) std::vector<uint8_t>().swap(khtl_d.dds);
                    else if (kh_dds_mip_fill(khtl_d.dds, khtl_d.srgb, khtl_d.blob)) {   // KH_DDS_MIP_FILL.
                        std::vector<uint8_t>().swap(khtl_d.dds);
                    }
                }
            } else {
                if (!kh_tex_blob_load(khtl_d.path, khtl_d.srgb, khtl_d.blob, khtl_d.from_cache, khtl_d.err,
                                      khtl_j.force)) {
                    khtl_d.blob.reset();
                }
            }
        } catch (const std::bad_alloc&) {
            khtl_fail = "out of memory";
        } catch (...) {
            khtl_fail = "load failed";
        }
        if (khtl_fail) {
            khtl_d.blob.reset();
            std::vector<uint8_t>().swap(khtl_d.dds);
            khtl_d.from_cache = false;
            khtl_d.err = khtl_fail;
        }
        // KH_TEXLDR_DONE_ROOM: the push does not allocate - every enqueue reserved this record's room - so it cannot
        // throw, and the entry cannot be left pending for the session with no result coming. The fallback stays: a
        // throw drops the payload and retries once as a bare failure.
        for (int khtl_try = 0; khtl_try < 2; ++khtl_try) {
            try {
                std::lock_guard<std::mutex> khtl_l(g_khtl_mu);
                g_khtl_done.push_back(std::move(khtl_d));
                break;
            } catch (...) {
                khtl_d.blob.reset();
                std::vector<uint8_t>().swap(khtl_d.dds);
                khtl_d.from_cache = false;
                khtl_d.err = "out of memory";
            }
        }
    }
}

inline bool kh_texldr_start() {
    std::lock_guard<std::mutex> khtl_l(g_khtl_mu);
    if (g_khtl_running) return true;
    g_khtl_abort.store(false, std::memory_order_relaxed);
    try {
        g_khtl_thr = std::thread(kh_texldr_worker);
    } catch (...) {
        return false;
    }
    g_khtl_running = true;
    return true;
}

// Joined, never detached (interruptible between jobs). Queued and finished results hold no device objects.
inline void kh_texldr_stop() {
    {
        std::lock_guard<std::mutex> khtl_l(g_khtl_mu);
        if (!g_khtl_running) return;
        g_khtl_q.clear();
        g_khtl_done.clear();
        g_khtl_abort.store(true, std::memory_order_relaxed);
    }
    g_khtl_cv.notify_all();
    if (g_khtl_thr.joinable()) { try { g_khtl_thr.join(); } catch (...) {} }
    std::lock_guard<std::mutex> khtl_l2(g_khtl_mu);
    // A worker mid-job at the clear above pushes its result before it sees the abort; this post-join clear makes 'no
    // result outlives the stop' true.
    g_khtl_done.clear();
    g_khtl_running = false;
}

// The pump's corrupt-cache retry, and only that. Unlike kh_texldr_queue it must NOT stand a worker up: once the loader
// is stopped the retry dies with it. The running test and the push happen under one lock acquisition, so a stop cannot
// land between them and resurrect the worker. Forced by construction. This caller must never create the decoder.
inline bool kh_texldr_requeue(const std::string& khtr_key, const std::string& khtr_path, bool khtr_srgb) {
    {
        std::lock_guard<std::mutex> khtr_l(g_khtl_mu);
        if (!g_khtl_running || g_khtl_abort.load(std::memory_order_relaxed)) return false;
        KhTexLoadJob khtr_j = { khtr_key, khtr_path, khtr_srgb, true };
        g_khtl_done.reserve(g_khtl_done.size() + g_khtl_q.size() + 2u);   // KH_TEXLDR_DONE_ROOM (kh_texldr_queue).
        g_khtl_q.push_back(std::move(khtr_j));
    }
    g_khtl_cv.notify_one();
    return true;
}

inline bool kh_texldr_queue(const std::string& khtl_key, const std::string& khtl_path, bool khtl_srgb, bool khtl_force) {
    if (!kh_texldr_start()) return false;
    {
        std::lock_guard<std::mutex> khtl_l(g_khtl_mu);
        KhTexLoadJob khtl_j = { khtl_key, khtl_path, khtl_srgb, khtl_force };
        // KH_TEXLDR_DONE_ROOM: room in the done list for every result that can arrive - those waiting, each
        // queued job, this one and the one the worker may be running - so the worker's push never allocates. A
        // throw here (out of memory) queues nothing: the caller sees it before any entry is made pending.
        g_khtl_done.reserve(g_khtl_done.size() + g_khtl_q.size() + 2u);
        g_khtl_q.push_back(std::move(khtl_j));
    }
    g_khtl_cv.notify_one();
    return true;
}

// The device half: create + page-place up to four finished loads at the pass heads, before kh_mat_ensure_all (a
// placement bumps the page generation, so every base is re-fixed before any lane is written) - serialized as
// KH_TEX_ASYNC says. A key the cache no longer holds drops its result.
static constexpr size_t KH_TEXLDR_PUMP_MAX = 4;
inline void kh_tex_async_pump(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    if (!ctx || !dev) return;
    std::vector<KhTexLoadDone> khtp_take;
    {
        std::lock_guard<std::mutex> khtp_l(g_khtl_mu);
        if (g_khtl_done.empty()) return;
        const size_t khtp_n = g_khtl_done.size() < KH_TEXLDR_PUMP_MAX ? g_khtl_done.size() : KH_TEXLDR_PUMP_MAX;
        khtp_take.reserve(khtp_n);
        for (size_t khtp_i = 0; khtp_i < khtp_n; ++khtp_i) khtp_take.push_back(std::move(g_khtl_done[khtp_i]));
        g_khtl_done.erase(g_khtl_done.begin(), g_khtl_done.begin() + khtp_n);
    }
    for (KhTexLoadDone& khtp_d : khtp_take) {
        auto khtp_it = g_tex_cache.find(khtp_d.key);
        if (khtp_it == g_tex_cache.end()) continue;   // Released meanwhile: drop.
        KhTexEntry& khtp_e = khtp_it->second;
        if (khtp_e.failed || khtp_e.page >= 0) continue;   // No longer pending: drop.
        std::string khtp_err = khtp_d.err;
        ID3D11ShaderResourceView* khtp_srv = nullptr;
        if (khtp_err.empty()) {
            HRESULT khtp_hr = S_OK;   // KH_TEX_RETRY_INVALID.
            if (!khtp_d.dds.empty()) khtp_srv = kh_load_dds(dev, khtp_d.dds, khtp_d.srgb, khtp_err);
            else if (khtp_d.blob)   khtp_srv = kh_tex_create(dev, *khtp_d.blob, khtp_err, &khtp_hr);
            else                    khtp_err = "empty result";
            // Corrupt-cache retry, the load_cached twin (KH_TEX_RETRY_INVALID: only a refused description).
            if (!khtp_srv && khtp_d.from_cache && kh_tex_cache_retry_worth(khtp_hr)) {
                // Re-queued to the worker as a force job (skips the probe), never an in-pass decode: the worker stays
                // the only decoder. A second failure lands in the failed arm and the retry terminates.
                if (kh_texldr_requeue(khtp_d.key, khtp_d.path, khtp_d.srgb)) {
                    khtp_e.last_use_ms = GetTickCount64();
                    continue;
                }
            }
        }
        if (khtp_srv) {
            if (!kh_tex_page_place(dev, ctx, khtp_srv, khtp_e)) khtp_err = "texture page placement failed";
            else ++g_tex_land_serial;   // KH_TEX_LAND: a map a skipped shadow may have drawn without.
            khtp_srv->Release();
        }
        khtp_e.last_use_ms = GetTickCount64();
        if (khtp_e.page < 0) {
            khtp_e.failed = true;
            if (kh_ends_with_ci(khtp_d.path, ".tga")) {
                khtp_err += " - if the file opens elsewhere, re-export as uncompressed true-color 24/32-bit TGA";
            }
            kh_tex_report(khtp_d.path, khtp_err);   // KH_TEX_WARN_CAP.
        }
    }
}
}