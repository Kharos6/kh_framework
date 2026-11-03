#pragma once

// The feature half (rendering_integration_features.hpp): the async texture loader thread. Never include it on its own.
namespace RenderIntegration {

// TEX_ASYNC: the texture loader worker is disk + CPU only (never touches the device), so it is engine-reset-safe
// without a stop. The device half - create and page placement - runs in tex_async_pump at the pass heads (the
// injection, the scene flush, the UI flush), each inside the engine's own one-thread-at-a-time use of the immediate
// context or under a park. Lazy start, joined at mission end.
struct TexLoadJob { std::string key; std::string path; bool srgb; bool force; };
struct TexLoadDone {
    std::string key;
    std::string path;
    bool srgb = false;
    bool from_cache = false;
    std::shared_ptr<TexBlob> blob;   // The stb route's results, and a .dds / .paa DDS_MIP_FILL built a chain for.
    std::vector<uint8_t> dds;          // .dds raw file bytes, or a .paa's made from it (parsed at the pump).
    std::string err;
};
static std::mutex g_texldr_mu;
static std::condition_variable g_texldr_cv;
static std::vector<TexLoadJob> g_texldr_q;
static std::vector<TexLoadDone> g_texldr_done;
static std::thread& g_texldr_thr = *(new std::thread());   // Never destructed (see g_meshwriter_thr).
static bool g_texldr_running = false;
static std::atomic<bool> g_texldr_abort{ false };

inline void texldr_worker() {
    for (;;) {
        TexLoadJob j;
        {
            std::unique_lock<std::mutex> l(g_texldr_mu);
            g_texldr_cv.wait(l, [] { return g_texldr_abort.load(std::memory_order_relaxed) || !g_texldr_q.empty(); });
            if (g_texldr_abort.load(std::memory_order_relaxed)) return;
            j = std::move(g_texldr_q.front());
            g_texldr_q.erase(g_texldr_q.begin());
        }
        TexLoadDone d;
        d.key = std::move(j.key);
        d.path = std::move(j.path);
        d.srgb = j.srgb;
        // TEXLDR_GUARD: an exception leaving this thread body is std::terminate. A throwing job fails into
        // its done record (an error, no payload), which tex_async_pump turns into the entry's failed state.
        // The messages are short enough for the string's own buffer: setting them allocates nothing.
        const char* fail = nullptr;
        try {
            if (ends_with_ci(d.path, ".dds")) {   // Same read + guard as the sync path.
                // PBO_PATH: asset_read. EA_TEX_READ_WHY: a failed read leaves no bytes and says why in err.
                if (asset_read(d.path, d.dds, 256ull * 1024u * 1024u, &d.err)) {
                    if (!dds_unblock(d.dds, d.err)) {   // DDS_BLOCK4: a refusal drops the bytes.
                        std::vector<uint8_t>().swap(d.dds);
                    } else if (dds_mip_fill(d.dds, d.srgb, d.blob)) {   // DDS_MIP_FILL: the blob.
                        std::vector<uint8_t>().swap(d.dds);
                    }
                }
            } else if (paa_ext(d.path)) {   // PAA: the sync path's twin - .dds bytes for the pump.
                std::vector<uint8_t> paa;
                if (asset_read(d.path, paa, 256ull * 1024u * 1024u, &d.err)) {   // As above.
                    if (!paa_to_dds(paa, d.dds, d.err)) std::vector<uint8_t>().swap(d.dds);
                    else if (dds_mip_fill(d.dds, d.srgb, d.blob)) {   // DDS_MIP_FILL.
                        std::vector<uint8_t>().swap(d.dds);
                    }
                }
            } else {
                if (!tex_blob_load(d.path, d.srgb, d.blob, d.from_cache, d.err,
                                   j.force)) {
                    d.blob.reset();
                }
            }
        } catch (const std::bad_alloc&) {
            fail = "out of memory";
        } catch (...) {
            fail = "load failed";
        }
        if (fail) {
            d.blob.reset();
            std::vector<uint8_t>().swap(d.dds);
            d.from_cache = false;
            d.err = fail;
        }
        // TEXLDR_DONE_ROOM: the push does not allocate - every enqueue reserved this record's room - so it cannot
        // throw, and the entry cannot be left pending for the session with no result coming. The fallback stays: a
        // throw drops the payload and retries once as a bare failure.
        for (int tl_try = 0; tl_try < 2; ++tl_try) {
            try {
                std::lock_guard<std::mutex> l(g_texldr_mu);
                g_texldr_done.push_back(std::move(d));
                break;
            } catch (...) {
                d.blob.reset();
                std::vector<uint8_t>().swap(d.dds);
                d.from_cache = false;
                d.err = "out of memory";
            }
        }
    }
}

inline bool texldr_start() {
    std::lock_guard<std::mutex> l(g_texldr_mu);
    if (g_texldr_running) return true;
    g_texldr_abort.store(false, std::memory_order_relaxed);
    try {
        g_texldr_thr = std::thread(texldr_worker);
    } catch (...) {
        return false;
    }
    g_texldr_running = true;
    return true;
}

// Joined, never detached (interruptible between jobs). Queued and finished results hold no device objects.
inline void texldr_stop() {
    {
        std::lock_guard<std::mutex> l(g_texldr_mu);
        if (!g_texldr_running) return;
        g_texldr_q.clear();
        g_texldr_done.clear();
        g_texldr_abort.store(true, std::memory_order_relaxed);
    }
    g_texldr_cv.notify_all();
    if (g_texldr_thr.joinable()) { try { g_texldr_thr.join(); } catch (...) {} }
    std::lock_guard<std::mutex> l2(g_texldr_mu);
    // A worker mid-job at the clear above pushes its result before it sees the abort; this post-join clear makes 'no
    // result outlives the stop' true.
    g_texldr_done.clear();
    g_texldr_running = false;
}

// The pump's corrupt-cache retry, and only that. Unlike texldr_queue it must NOT stand a worker up: once the loader
// is stopped the retry dies with it. The running test and the push happen under one lock acquisition, so a stop cannot
// land between them and resurrect the worker. Forced by construction. This caller must never create the decoder.
inline bool texldr_requeue(const std::string& key, const std::string& path, bool srgb) {
    {
        std::lock_guard<std::mutex> l(g_texldr_mu);
        if (!g_texldr_running || g_texldr_abort.load(std::memory_order_relaxed)) return false;
        TexLoadJob j = { key, path, srgb, true };
        g_texldr_done.reserve(g_texldr_done.size() + g_texldr_q.size() + 2u);   // TEXLDR_DONE_ROOM (texldr_queue).
        g_texldr_q.push_back(std::move(j));
    }
    g_texldr_cv.notify_one();
    return true;
}

inline bool texldr_queue(const std::string& key, const std::string& path, bool srgb, bool force) {
    if (!texldr_start()) return false;
    {
        std::lock_guard<std::mutex> l(g_texldr_mu);
        TexLoadJob j = { key, path, srgb, force };
        // TEXLDR_DONE_ROOM: room in the done list for every result that can arrive - those waiting, each
        // queued job, this one and the one the worker may be running - so the worker's push never allocates. A
        // throw here (out of memory) queues nothing: the caller sees it before any entry is made pending.
        g_texldr_done.reserve(g_texldr_done.size() + g_texldr_q.size() + 2u);
        g_texldr_q.push_back(std::move(j));
    }
    g_texldr_cv.notify_one();
    return true;
}

// The device half: create + page-place up to four finished loads at the pass heads, before mat_ensure_all (a
// placement bumps the page generation, so every base is re-fixed before any lane is written) - serialized as
// TEX_ASYNC says. A key the cache no longer holds drops its result.
static constexpr size_t TEXLDR_PUMP_MAX = 4;
inline void tex_async_pump(ID3D11DeviceContext* ctx, ID3D11Device* dev) {
    if (!ctx || !dev) return;
    std::vector<TexLoadDone> take;
    {
        std::lock_guard<std::mutex> l(g_texldr_mu);
        if (g_texldr_done.empty()) return;
        const size_t n = g_texldr_done.size() < TEXLDR_PUMP_MAX ? g_texldr_done.size() : TEXLDR_PUMP_MAX;
        take.reserve(n);
        for (size_t i = 0; i < n; ++i) take.push_back(std::move(g_texldr_done[i]));
        g_texldr_done.erase(g_texldr_done.begin(), g_texldr_done.begin() + n);
    }
    for (TexLoadDone& d : take) {
        auto it = g_tex_cache.find(d.key);
        if (it == g_tex_cache.end()) continue;   // Released meanwhile: drop.
        TexEntry& e = it->second;
        if (e.failed || e.page >= 0) continue;   // No longer pending: drop.
        std::string err = d.err;
        ID3D11ShaderResourceView* srv = nullptr;
        if (err.empty()) {
            HRESULT hr = S_OK;   // TEX_RETRY_INVALID.
            if (!d.dds.empty()) srv = load_dds(dev, d.dds, d.srgb, err);
            else if (d.blob)   srv = tex_create(dev, *d.blob, err, &hr);
            else                    err = "empty result";
            // Corrupt-cache retry, the load_cached twin (TEX_RETRY_INVALID: only a refused description).
            if (!srv && d.from_cache && tex_cache_retry_worth(hr)) {
                // Re-queued to the worker as a force job (skips the probe), never an in-pass decode: the worker stays
                // the only decoder. A second failure lands in the failed arm and the retry terminates.
                if (texldr_requeue(d.key, d.path, d.srgb)) {
                    e.last_use_ms = GetTickCount64();
                    continue;
                }
            }
        }
        if (srv) {
            if (!tex_page_place(dev, ctx, srv, e)) err = "texture page placement failed";
            else ++g_tex_land_serial;   // TEX_LAND: a map a skipped shadow may have drawn without.
            srv->Release();
        }
        e.last_use_ms = GetTickCount64();
        if (e.page < 0) {
            e.failed = true;
            if (ends_with_ci(d.path, ".tga")) {
                err += " - if the file opens elsewhere, re-export as uncompressed true-color 24/32-bit TGA";
            }
            tex_report(d.path, err);   // TEX_WARN_CAP.
        }
    }
}
}