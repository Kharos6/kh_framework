#pragma once

// The feature half (rendering_integration_features.hpp): the shader sources and compiler: the HLSL store, the disk
// cache key, the DXBC check, the compile workers and compile_shader. Never include it on its own.
namespace RenderIntegration {

inline void shader_mt_shutdown(bool wait = true);

inline const std::filesystem::path& mesh_cache_dir();
inline void mesh_cache_trim();

// Defined in rendering/disk_cache.hpp (declared here for the uses below).
inline std::string cache_file_name(const char* pfx, uint64_t h,
                                   const char* ext);
inline void mod_cache_append(std::vector<std::filesystem::path>& out,
                             const std::string& name);

static const wchar_t* const HLSL_RES_NAMES[HLSL_N] = {
    L"KH_CB_HLSL", L"KH_WHITE_HLSL", L"KH_STATIC_HLSL", L"KH_COMPOSITE_HLSL", L"KH_EFFECT_HLSL",
    L"KH_DEPTH_RESOLVE_HLSL", L"KH_SUNPF_HLSL", L"KH_VMIR_CS_HLSL",
    L"KH_SSAO_HLSL", L"KH_SKIN_HLSL",
};

struct HlslStore { std::string s[HLSL_N]; };

inline HlslStore hlsl_load_all() {
    HlslStore out;
    HMODULE hm = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&hlsl_load_all), &hm);

    for (int i = 0; i < HLSL_N; ++i) {
        const char* p = nullptr;
        DWORD sz = 0;
        if (hm) {
            HRSRC r = FindResourceW(hm, HLSL_RES_NAMES[i], MAKEINTRESOURCEW(10));   // RT_RCDATA, W-typed regardless
                                                                                                   // Of unicode.
            if (r) {
                HGLOBAL g = LoadResource(hm, r);
                sz = SizeofResource(hm, r);
                if (g && sz) p = static_cast<const char*>(LockResource(g));
            }
        }

        if (!p) {
            out.s[i] = "// KH: shader resource missing from the DLL - add kh_shaders.rc to the build\n";
            // EA_HLSL_MISSING: named once (from any thread: the store loads on whichever asks first), so the fxc
            // failures that follow (an entry point or an identifier it cannot find) carry their cause.
            std::string name;
            for (const wchar_t* w = HLSL_RES_NAMES[i]; *w; ++w) {
                name += static_cast<char>(*w);   // The names are ASCII.
            }
            report_error_once_safe("KH RenderIntegration: shader sources: resource " + name +
                                   " missing from the DLL (hlslResMissing) - add kh_shaders.rc to the build");
            continue;
        }

        std::string& s = out.s[i];
        s.reserve(sz);
        for (DWORD k = 0; k < sz; ++k) {
            if (p[k] != '\r') s.push_back(p[k]);
        }
    }

    return out;
}

// One load for the process (thread-safe magic static: the shader workers and the two threads that ensure may all
// arrive first). The pointer the .c_str consumers hold is stable for the process lifetime.
inline const std::string& hlsl_src(int i) {
    static const HlslStore all = hlsl_load_all();
    return all.s[i];
}

// The compile flags join the shader cache hash (content-addressed over source + entry + target + defines; every hash
// here is CryptoGenerator's FNV-1a 64, field-separated), so a change here recompiles every unit once and a
// differently-flagged blob is never served under the same key. D3DCompile's default is optimization level 1; level
// 3 is the shipping setting (scheduling, register allocation, dead code). Without D3DCOMPILE_IEEE_STRICTNESS fxc is not
// IEEE-strict at any level (it may fuse and reorder float arithmetic); the units' 'precise' qualifiers hold the sums
// that must stay exact.
inline uint32_t shader_flags() {
    return D3DCOMPILE_OPTIMIZATION_LEVEL3;
}

// SHADER_KEY_EPOCH: the key's version, the last value shader_cache_hash folds in - bump it to make every blob
// (ours and every mod's shipped .khsc) a miss, so every unit compiles once more. SHADER_CACHE_VERSION gates only
// the file's header layout.
// Not in the key, deliberately: the d3dcompiler_47.dll's version - so a shipped blob serves every player and survives
// a Windows update of the DLL (a blob is served whichever compiler version wrote it).
static constexpr uint64_t SHADER_KEY_EPOCH = 26481ull;

inline uint64_t shader_cache_hash(const char* src, const char* entry, const char* target,
                                  const D3D_SHADER_MACRO* defines) {
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    auto mix = [&h](const char* s) {
        if (!s) return;   // A null macro Definition contributes nothing;
        h = CryptoGenerator::fnv1a64_update(h, s, strlen(s));
        h = CryptoGenerator::fnv1a64_update(h, "\xFF", 1);
    };
    mix(src); mix(entry); mix(target);
    for (const D3D_SHADER_MACRO* d = defines; d && d->Name; ++d) { mix(d->Name); mix(d->Definition); }
    // The compile flags are part of the identity; folded in as whole values.
    h ^= static_cast<uint64_t>(shader_flags()); h *= CryptoGenerator::FNV1A64_PRIME;
    h ^= SHADER_KEY_EPOCH; h *= CryptoGenerator::FNV1A64_PRIME;
    return h;
}

// Hits touch the write time: eviction is least-recently-used, the FBX cache's own rule.
inline std::filesystem::path shader_cache_file(uint64_t h) {
    const std::filesystem::path& d = mesh_cache_dir();
    if (d.empty()) return std::filesystem::path();
    return d / cache_file_name("khs_", h, ".khsc");   // CACHE_NAME_ONE: the probe's spelling.
}
static constexpr uint32_t SHADER_CACHE_MAGIC = 0x4353484Bu;   // "khsc".
static constexpr uint32_t SHADER_CACHE_VERSION = 1;
// SHADER_CACHE_SUM: the DXBC container checksum - MD5 over the bytes from offset 20 (past the magic and the
// checksum itself), closed by the container's own final block (the bit count first, then the tail bytes, 0x80, and
// (bits >> 2) | 1 as the last word; a tail of 56 bytes or more takes a block of its own first). Direct3D checks it
// at Create, so a cached blob whose body changed on disk fails there; checked here, it is a miss. Verified against
// DXC's signed containers (every shader of every unit) and every one-bit change to them.
inline uint32_t dxbc_rol(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }
inline void dxbc_md5_block(uint32_t s[4], const uint8_t* p) {
    static const uint32_t k[64] = {
        0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
        0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
        0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau, 0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
        0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu, 0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
        0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
        0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u, 0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
        0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
        0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u, 0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u };
    static const int r[16] = { 7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21 };
    uint32_t m[16];
    for (int i = 0; i < 16; ++i) {
        m[i] = static_cast<uint32_t>(p[4 * i]) |
               (static_cast<uint32_t>(p[4 * i + 1]) << 8) |
               (static_cast<uint32_t>(p[4 * i + 2]) << 16) |
               (static_cast<uint32_t>(p[4 * i + 3]) << 24);
    }
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3];
    for (int i = 0; i < 64; ++i) {
        uint32_t f;
        int g;
        if (i < 16)      { f = (b & c) | (~b & d); g = i; }
        else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
        else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) & 15; }
        else                  { f = c ^ (b | ~d); g = (7 * i) & 15; }
        const uint32_t t = d;
        d = c;
        c = b;
        b = b + dxbc_rol(a + f + k[i] + m[g],
                         r[(i >> 4) * 4 + (i & 3)]);
        a = t;
    }
    s[0] += a; s[1] += b; s[2] += c; s[3] += d;
}
inline bool dxbc_sum_ok(const uint8_t* p, size_t n) {
    if (n < 32) return false;
    const uint8_t* const q = p + 20;
    const size_t len = n - 20;
    uint32_t s[4] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u };
    const size_t full = len & ~static_cast<size_t>(63);
    for (size_t o = 0; o < full; o += 64) dxbc_md5_block(s, q + o);
    const size_t rest = len - full;
    const uint32_t bits = static_cast<uint32_t>(len * 8u);
    uint8_t blk[64];
    auto put = [](uint8_t* d, uint32_t v) {
        d[0] = static_cast<uint8_t>(v);
        d[1] = static_cast<uint8_t>(v >> 8);
        d[2] = static_cast<uint8_t>(v >> 16);
        d[3] = static_cast<uint8_t>(v >> 24);
    };
    memset(blk, 0, sizeof(blk));
    if (rest >= 56) {
        memcpy(blk, q + full, rest);
        blk[rest] = 0x80;
        dxbc_md5_block(s, blk);
        memset(blk, 0, sizeof(blk));
        put(blk, bits);
    } else {
        put(blk, bits);
        memcpy(blk + 4, q + full, rest);
        blk[4 + rest] = 0x80;
    }
    put(blk + 60, (bits >> 2) | 1u);
    dxbc_md5_block(s, blk);
    uint8_t dig[16];
    for (int i = 0; i < 4; ++i) put(dig + 4 * i, s[i]);
    return memcmp(dig, p + 4, 16) == 0;
}
// Read them as a pair.
static std::atomic<uint64_t> g_shader_cache_hits{0};
static std::atomic<uint64_t> g_shader_cache_misses{0};

// A memo miss compiles inline, so a job list that drifts out of step with its ensure costs speed, never correctness.
struct ShaderJob {
    const char* src;
    const char* entry;
    const char* target;
    const D3D_SHADER_MACRO* defines;
    uint64_t hash;   // Filled by the prewarm.
};
struct ShaderMemo {
    ID3DBlob*   blob;
    std::string err;
    bool        done;
    bool        taken;
};
static std::mutex g_shpool_mu;
static std::condition_variable g_shpool_cv;
static std::unordered_map<uint64_t, ShaderMemo> g_shpool_map;
static std::vector<ShaderJob> g_shpool_q;   // Built before any worker starts.
static std::atomic<size_t> g_shpool_next{0};
// Deliberately never destructed (detached workers may outlive static teardown).
static std::vector<std::thread>& g_shpool_thr = *(new std::vector<std::thread>());
// The workers shader_mt_shutdown detached, by a duplicate of each thread's handle (the detach closes std::thread's
// own): process_detach bounds and ends them as it does the listed ones. Pruned of exited threads at each shutdown.
// Never destructed, as g_shpool_thr; the same callers touch both.
static std::vector<HANDLE>& g_shpool_orphans = *(new std::vector<HANDLE>());
static bool g_shpool_active = false;   // A batch is live (caller thread only).
static std::atomic<int> g_shpool_live{0};
static std::atomic<bool> g_shbatch_abort{false};
struct ShaderJobOwned {
    std::shared_ptr<std::string> src_sh;   // Shared across jobs over one unit.
    std::string entry, target;
    std::vector<std::string> dn, dv;
    std::vector<unsigned char> dhas;
    std::vector<D3D_SHADER_MACRO> macros;
    bool had_defines;
};
static std::vector<std::unique_ptr<ShaderJobOwned>> g_shpool_own;
// Decrements on every worker exit path, normal or exceptional.
struct ShaderLive {
    ~ShaderLive() { g_shpool_live.fetch_sub(1, std::memory_order_acq_rel); }
};
static std::mutex g_shpool_trim_mu;   // mesh_cache_trim is not re-entrant.
// CACHE_TRIM_THROTTLE - shader-blob bytes written since the last trim a shader write ran. The trim scans the
// whole cache directory (per blob it would be ~95 scans for a cold start's prewarm, serialised across the pool on
// g_shpool_trim_mu, and on the render thread for an inline compile). A blob counts against its own budget only
// (CACHE_KIND_BUDGET), so one scan per 4 MiB written holds .khsc within 4 MiB of what the trim allows (its budget,
// or twice it for this session's own blobs - CACHE_SESSION_KEEP); the debt starts full at each session
// (session_globals_reset), so a session's first blob scans. Mesh and texture writes trim at every write.
static constexpr uint64_t CACHE_SHADER_TRIM_SLACK = 4ull << 20;
static std::atomic<uint64_t> g_shcache_trim_debt{ CACHE_SHADER_TRIM_SLACK };
// SHADER_FAIL_LATCH - the D3DCompile failures of this session, by cache hash (the source, entry, target, defines
// and flags): a unit that failed with fxc's message fails the same way every time; unlatched, a failed built-in unit
// compiles again at every ensure, on the render thread, several times a frame. shader_compile_raw answers a
// latched hash with the stored message ahead of the disk probe - a failed unit wrote no blob and the mod caches are
// fixed at first use, so the probe would miss again (a latched answer counts as neither a cache hit nor a miss). Not
// latched: a failure without fxc's message, or out of memory (E_OUTOFMEMORY and the Win32 not-enough-memory codes -
// transient). Cleared by shader_mt_shutdown (the mission edges, a device reset), which also bumps
// g_shcache_fail_epoch: a compile that began before the clear (a detached worker's, still inside D3DCompile) does not
// latch into the next session, nor reach it through the memo (SHADER_MEMO_EPOCH, shader_mt_worker).
// Never destructed (a detached worker may compile past static teardown), as g_shpool_thr.
static std::mutex& g_shcache_fail_mu = *(new std::mutex());
static std::unordered_map<uint64_t, std::string>& g_shcache_fail = *(new std::unordered_map<uint64_t, std::string>());
static uint64_t g_shcache_fail_epoch = 0u;   // SHADER_FAIL_LATCH: under g_shcache_fail_mu.


inline std::string shader_compile_raw(const char* src, const char* entry, const char* target,
                                      const D3D_SHADER_MACRO* defines, ID3DBlob** out_blob) {
    ID3DBlob* err_blob = nullptr;

    const uint64_t h = shader_cache_hash(src, entry, target, defines);
    uint64_t epoch = 0u;   // SHADER_FAIL_LATCH: the session this compile began in.
    {   // SHADER_FAIL_LATCH: a unit that already failed this session answers with its message.
        std::lock_guard<std::mutex> fl_l(g_shcache_fail_mu);
        epoch = g_shcache_fail_epoch;
        const auto fit = g_shcache_fail.find(h);
        if (fit != g_shcache_fail.end()) {
            *out_blob = nullptr;
            return fit->second;
        }
    }
    const std::filesystem::path p = shader_cache_file(h);

    // Index 0 is the Documents copy and the only one the lru touches (a mod folder may be read-only). Ours first,
    // then every mod that ships this blob.
    std::vector<std::filesystem::path> cand;
    if (!p.empty()) cand.push_back(p);
    mod_cache_append(cand, cache_file_name("khs_", h, ".khsc"));

    for (size_t ci = 0; ci < cand.size(); ++ci) {
        const bool own = ci == 0 && !p.empty();

        try {
            std::ifstream f(cand[ci], std::ios::binary);
            if (f) {
                struct { uint32_t magic, ver; uint64_t hash, size; } hdr = {};
                f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
                if (f.gcount() == sizeof(hdr) &&
                    hdr.magic == SHADER_CACHE_MAGIC &&
                    hdr.ver == SHADER_CACHE_VERSION &&
                    hdr.hash == h &&
                    hdr.size > 0 && hdr.size < (64ull << 20)) {
                    ID3DBlob* b = nullptr;
                    if (SUCCEEDED(D3DCreateBlob(static_cast<SIZE_T>(hdr.size), &b))) {
                        f.read(static_cast<char*>(b->GetBufferPointer()),
                               static_cast<std::streamsize>(hdr.size));
                        bool dxbc = false;
                        if (f.gcount() == static_cast<std::streamsize>(hdr.size) &&
                            hdr.size >= 32) {
                            const uint8_t* pb = static_cast<const uint8_t*>(b->GetBufferPointer());
                            uint32_t tot = 0;
                            memcpy(&tot, pb + 24, sizeof(tot));
                            // SHADER_CACHE_SUM: a body changed on disk fails here, as the Create would.
                            dxbc = pb[0] == 'D' && pb[1] == 'X' &&
                                   pb[2] == 'B' && pb[3] == 'C' &&
                                   tot == static_cast<uint32_t>(hdr.size) &&
                                   dxbc_sum_ok(pb, static_cast<size_t>(hdr.size));
                        }
                        if (f.gcount() == static_cast<std::streamsize>(hdr.size) && !dxbc) {
                            f.close();
                            if (own) {   // Ours only; a mod's copy is not our budget.
                                std::error_code rm;
                                std::filesystem::remove(cand[ci], rm);
                            }
                        }
                        if (dxbc) {
                            f.close();

                            if (own) {
                                std::error_code ec;
                                std::filesystem::last_write_time(
                                    cand[ci],
                                    std::filesystem::file_time_type::clock::now(), ec);
                            }

                            *out_blob = b;
                            if (stats_on()) g_shader_cache_hits.fetch_add(1, std::memory_order_relaxed);
                            return "";   // Cache hit - no compile.
                        }
                        b->Release();
                    }
                }
            }
        } catch (...) {}   // Any doubt is a miss; the next candidate or the compile.
    }

    if (stats_on()) g_shader_cache_misses.fetch_add(1, std::memory_order_relaxed);
    const uint32_t fl = shader_flags();
    HRESULT hr = D3DCompile(src, strlen(src), "kh_render", defines, nullptr,
                            entry, target, fl, 0, out_blob, &err_blob);

    if (FAILED(hr)) {
        std::string msg = std::string(entry) + " compile failed " + hr_str(hr);
        const bool latch = err_blob != nullptr && hr != E_OUTOFMEMORY &&   // SHADER_FAIL_LATCH.
                           hr != HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY) &&
                           hr != HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

        if (err_blob) {
            msg += ": ";
            msg += static_cast<const char*>(err_blob->GetBufferPointer());
            err_blob->Release();
        }

        if (latch) {
            try {
                std::lock_guard<std::mutex> fl_l(g_shcache_fail_mu);
                if (epoch == g_shcache_fail_epoch) g_shcache_fail[h] = msg;
            } catch (...) {}   // Unlatched: the next call compiles again.
        }

        return msg;
    }

    if (err_blob) err_blob->Release();

    // Cache write-through (best-effort), then the trim - each kind under its own budget (CACHE_KIND_BUDGET).
    if (!p.empty() && *out_blob) {
        // Written to a private temp name and renamed into place so two processes sharing the cache dir never read
        // each other's half-written blob. A sharing violation leaves the existing file and drops the temp.
        // DOCS_WIDE: appended to the path itself - p.string() throws on a name outside the ANSI page.
        std::filesystem::path tmp = p;
        tmp += ".tmp" + std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
        bool wrote = false;
        try {
            std::ofstream w(tmp, std::ios::binary | std::ios::trunc);
            if (w) {
                struct { uint32_t magic, ver; uint64_t hash, size; } hdr = {
                    SHADER_CACHE_MAGIC, SHADER_CACHE_VERSION,
                    h, static_cast<uint64_t>((*out_blob)->GetBufferSize()) };
                w.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
                w.write(static_cast<const char*>((*out_blob)->GetBufferPointer()),
                        static_cast<std::streamsize>((*out_blob)->GetBufferSize()));
                w.flush();
                wrote = static_cast<bool>(w);
            }
        } catch (...) { wrote = false; }
        std::error_code ec;
        if (wrote) std::filesystem::rename(tmp, p, ec);
        if (!wrote || ec) std::filesystem::remove(tmp, ec);
        // CACHE_TRIM_THROTTLE: a scan per CACHE_SHADER_TRIM_SLACK bytes written, not per blob.
        const uint64_t sz = static_cast<uint64_t>((*out_blob)->GetBufferSize());
        if (wrote && !ec &&
            g_shcache_trim_debt.fetch_add(sz, std::memory_order_relaxed) + sz >= CACHE_SHADER_TRIM_SLACK) {
            std::lock_guard<std::mutex> tl(g_shpool_trim_mu);
            g_shcache_trim_debt.store(0u, std::memory_order_relaxed);
            mesh_cache_trim();
        }
    }
    return "";
}

inline void shader_mt_worker() {
    ShaderLive lv;
    uint64_t cur = 0;
    uint64_t ep = 0u;   // SHADER_MEMO_EPOCH: the session (g_shcache_fail_epoch) the held job began in.
    bool held = false;
    try {
        for (;;) {
            // Teardown stops new claims at once; a job already inside D3DCompile has to finish.
            if (g_shbatch_abort.load(std::memory_order_relaxed)) return;
            const size_t i = g_shpool_next.fetch_add(1, std::memory_order_relaxed);
            if (i >= g_shpool_q.size()) return;
            const ShaderJob j = g_shpool_q[i];
            {
                std::lock_guard<std::mutex> l(g_shpool_mu);
                auto it = g_shpool_map.find(j.hash);
                if (it == g_shpool_map.end() ||
                    it->second.done || it->second.taken) continue;
                it->second.taken = true;
            }
            cur = j.hash;
            held = true;   // From here a result is owed to any waiter.
            {   // SHADER_MEMO_EPOCH: read as shader_compile_raw reads it for the fail latch.
                std::lock_guard<std::mutex> fl(g_shcache_fail_mu);
                ep = g_shcache_fail_epoch;
            }
            ID3DBlob* b = nullptr;
            std::string e;
            try {
                e = shader_compile_raw(j.src, j.entry,
                                       j.target, j.defines, &b);
            } catch (...) {
                if (b) { b->Release(); b = nullptr; }
                e = std::string(j.entry) + " compile threw";
            }
            {
                std::lock_guard<std::mutex> l(g_shpool_mu);
                // SHADER_MEMO_EPOCH: a failure from a job that began before a shutdown (this worker was detached
                // and runs on into the next session or device) is erased, not stored: the memo keeps a done entry
                // across the edge (SHADER_ORPHANS), so the next session's consumer would take this session's
                // error (a transient one included) against the fail latch's rule that a new session compiles a
                // failed unit once more. A blob is good bytecode in any session and is stored. Lock order: g_shpool_mu,
                // then g_shcache_fail_mu (nothing takes them the other way round).
                bool stale = false;
                if (!b) {
                    std::lock_guard<std::mutex> fl(g_shcache_fail_mu);
                    stale = ep != g_shcache_fail_epoch;
                }
                if (stale) {
                    g_shpool_map.erase(j.hash);
                } else {
                    ShaderMemo& m = g_shpool_map[j.hash];
                    m.blob = b;
                    m.err = e;
                    m.done = true;
                }
            }
            held = false;
            g_shpool_cv.notify_all();
        }
    } catch (...) {
        if (held) {
            try {
                std::lock_guard<std::mutex> l(g_shpool_mu);
                bool stale = false;   // SHADER_MEMO_EPOCH, as above.
                {
                    std::lock_guard<std::mutex> fl(g_shcache_fail_mu);
                    stale = ep != g_shcache_fail_epoch;
                }
                if (stale) {
                    g_shpool_map.erase(cur);
                } else {
                    ShaderMemo& m = g_shpool_map[cur];
                    m.done = true;
                    m.err = "shader worker aborted";
                }
            } catch (...) {}
            g_shpool_cv.notify_all();
        }
    }
}

// A batch that was built but never got a worker is torn down here: the whole map (this call's entries and any memo
// it held), the owned sources, the queue. No worker exists at either call site, so the map is touched without the
// lock. The caller compiles inline.
inline void shader_mt_abandon() {
    for (auto& kv : g_shpool_map) {
        if (kv.second.blob) kv.second.blob->Release();
    }
    g_shpool_map.clear();
    g_shpool_own.clear();
    g_shpool_q.clear();
    g_shpool_next.store(0, std::memory_order_relaxed);
    g_shpool_active = false;
}

inline bool shader_mt_prewarm(const ShaderJob* jobs, size_t n,
                              bool caller_works = true) {
    if (!jobs || n == 0) return false;
    if (g_shpool_active) return false;
    // A detached worker from a previous session may still be reading the queue and the owned-job storage; refusing
    // here is what makes that detach safe.
    if (g_shpool_live.load(std::memory_order_acquire) > 0) return false;
    // SHADER_ORPHANS - a shutdown that could not wait (the device reset's never does; a mission end's gives up
    // after its bound) detaches its workers and LEAVES the memo for them: they finish the unit they hold and exit on
    // the abort flag; a unit none of them had claimed stays in the map undone for good (the queue below skips a hash
    // the map holds, and nothing erases it). Left there it holds the main ensure's deferral up for the session
    // whenever this batch queues anything new - white placeholders, no mesh (shader_async_hold waits for EVERY
    // entry; a reset changes the sample-count defines, so new units are the usual case) - or with nothing new sends
    // every orphaned unit to an inline compile inside the ensure (the seconds-long stall the pool exists to avoid),
    // and keeps a user-shader batch from being reaped (user_shader_reap waits the same way) so g_shpool_active
    // stands and every later prewarm is refused. Here no batch is live and no worker exists, so an undone entry is an
    // orphan by construction: dropped (it holds no blob) and queued again below. Done entries stay - a blob is good
    // bytecode, and a failure is this session's (SHADER_MEMO_EPOCH: the shutdown and the worker drop an earlier
    // session's).
    {
        std::lock_guard<std::mutex> l(g_shpool_mu);
        for (auto it = g_shpool_map.begin(); it != g_shpool_map.end();) {
            if (!it->second.done) it = g_shpool_map.erase(it);
            else ++it;
        }
    }
    g_shbatch_abort.store(false, std::memory_order_relaxed);
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 1;
    if (hw > 64) hw = 64;
    // One core: a pool would only add threads to work the caller would do itself. SHADER_ONE_CORE - do not refuse
    // when the caller does not work (the async gate, the user-shader pump): refused, the gate compiles the whole set
    // inline on the render thread (minutes on a cold cache, the game frozen) and the pump never arms, so no user
    // shader ever compiles. They get the one worker below; the frames draw the placeholder meanwhile.
    if (hw < 2 && caller_works) return false;
    try {
    std::unordered_map<const char*, std::shared_ptr<std::string>> srcs;
    g_shpool_q.clear();
    // The owned-job storage is freed here, not only at shutdown: user-shader batches each carry a full
    // cb.hlsl-prefixed source. The memo holds blobs and error strings only.
    g_shpool_own.clear();
    g_shpool_q.reserve(n);
    {
        std::lock_guard<std::mutex> l(g_shpool_mu);
        for (size_t i = 0; i < n; ++i) {
            const ShaderJob in = jobs[i];
            if (!in.src || !in.entry || !in.target) continue;
            const uint64_t h = shader_cache_hash(in.src, in.entry,
                                                 in.target, in.defines);
            if (g_shpool_map.find(h) != g_shpool_map.end()) continue;
            // Deep copy: the caller's strings and macro table are function locals and a worker outlives them.
            std::unique_ptr<ShaderJobOwned> o(new ShaderJobOwned());
            {
                auto si = srcs.find(in.src);
                if (si == srcs.end())
                    si = srcs.emplace(in.src,
                        std::make_shared<std::string>(in.src)).first;
                o->src_sh = si->second;
            }
            o->entry = in.entry;
            o->target = in.target;
            o->had_defines = (in.defines != nullptr);
            for (const D3D_SHADER_MACRO* d = in.defines;
                 d && d->Name; ++d) {
                o->dn.push_back(d->Name);
                // A null Definition is preserved as null (dhas): the cache hash mixes nothing for null but a
                // separator for "" - collapsing them would change every key.
                o->dv.push_back(d->Definition ? d->Definition : "");
                o->dhas.push_back(d->Definition ? 1u : 0u);
            }
            o->macros.reserve(o->dn.size() + 1);
            for (size_t k = 0; k < o->dn.size(); ++k) {
                D3D_SHADER_MACRO m;
                m.Name = o->dn[k].c_str();
                m.Definition = o->dhas[k] ? o->dv[k].c_str()
                                                         : nullptr;   // Null != "".
                o->macros.push_back(m);
            }
            D3D_SHADER_MACRO term; term.Name = nullptr; term.Definition = nullptr;
            o->macros.push_back(term);
            ShaderJob j;
            j.src = o->src_sh->c_str();
            j.entry = o->entry.c_str();
            j.target = o->target.c_str();
            j.defines = o->had_defines ? o->macros.data() : nullptr;
            j.hash = h;
            g_shpool_map[h] = ShaderMemo{ nullptr, std::string(), false, false };
            g_shpool_own.push_back(std::move(o));
            g_shpool_q.push_back(j);
        }
    }
    if (g_shpool_q.empty()) return false;
    g_shpool_active = true;   // From here the caller owns the teardown.
    g_shpool_next.store(0, std::memory_order_relaxed);
    // When the caller does not participate (the async gate returns instead of stealing) the pool gets every core.
    size_t w = static_cast<size_t>(hw) - (caller_works ? 1u : 0u);
    const size_t cap = caller_works ? g_shpool_q.size() - 1 : g_shpool_q.size();
    if (w > cap) w = cap;
    for (size_t i = 0; i < w; ++i) {
        g_shpool_live.fetch_add(1, std::memory_order_acq_rel);   // Before the spawn.
        try {
            g_shpool_thr.emplace_back(shader_mt_worker);
        } catch (...) {
            g_shpool_live.fetch_sub(1, std::memory_order_acq_rel);
            break;   // Fewer threads is slower, never wrong.
        }
    }
    if (w > 0 && g_shpool_thr.empty()) {
        shader_mt_abandon();
        return false;
    }
    return true;
    } catch (...) {
        // Every throw site above precedes the spawn loop, so a throw here means map entries with no worker to finish
        // them; shader_async_gate has no destructor and would scan them every flush forever with white
        // placeholders. Tear down and refuse; the batches of ShaderMtScope and the async gate then compile inline,
        // and the user-shader pump leaves its requests unarmed for a later pump. The spawned-worker branch is
        // unreachable today and kept so a future throw after the spawn cannot orphan a live thread.
        if (g_shpool_thr.empty()) {
            shader_mt_abandon();
            return false;
        }
        g_shpool_active = true;   // Workers exist: the caller's teardown joins them.
        return true;
    }
}

// Join every worker, then (unless keep_memo) release every blob the memo holds: those no call site consumed (a
// conditional ensure can legitimately skip a unit the pool compiled) and those kept after a take (SHADER_MEMO_KEEP).
inline void shader_mt_finish(bool keep_memo = false) {
    for (std::thread& t : g_shpool_thr) {
        if (t.joinable()) {
            try { t.join(); } catch (...) {}
        }
    }
    g_shpool_thr.clear();
    std::lock_guard<std::mutex> l(g_shpool_mu);
    if (!keep_memo) {
        for (auto& kv : g_shpool_map) {
            if (kv.second.blob) kv.second.blob->Release();
        }
        g_shpool_map.clear();
    }
    // SHPOOL_OWN_FREE: the batch's copied sources and defines go in both branches (every worker is joined, the
    // queue that pointed into them is cleared below, and the memo holds blobs and strings of its own).
    g_shpool_own.clear();
    g_shpool_q.clear();
    g_shpool_q.shrink_to_fit();
    g_shpool_next.store(0, std::memory_order_relaxed);
    g_shpool_active = false;
}

// One declaration per batch covers every early return, Create failure and throw. The prewarm deep-copies the jobs'
// sources and define tables (ShaderJobOwned), so the scope holds no pointer into the caller's strings.
struct ShaderMtScope {
    bool own;   // Only tear down a batch this scope armed.
    bool keep;   // Leave the compiled blobs for a later ensure.
    ShaderMtScope(const ShaderJob* j, size_t n, bool k = false)
        : own(shader_mt_prewarm(j, n)), keep(k) {}
    ~ShaderMtScope() {
        if (own) shader_mt_finish(keep);
    }
    ShaderMtScope(const ShaderMtScope&) = delete;
    ShaderMtScope& operator=(const ShaderMtScope&) = delete;
};

static int      g_shbatch_state = 0;   // 0 idle, 1 compiling, 2 done.

// The poll needs no source, so it runs before the strings are built on a deferred frame.
inline bool shader_async_hold() {
    if (g_shbatch_state == 2) return false;
    if (g_shbatch_state == 0) {
        if (g_shpool_live.load(std::memory_order_acquire) > 0) {
            return true;
        }
        return false;   // Nothing armed yet; shader_async_gate arms below.
    }
    bool pending = false;
    {
        std::lock_guard<std::mutex> l(g_shpool_mu);
        for (auto& kv : g_shpool_map) {
            if (!kv.second.done) { pending = true; break; }
        }
    }
    if (pending) {
        return true;
    }
    // JOIN_TAIL: every unit has landed, so no worker is compiling - each is past its last result and returning
    // (the join waits out that tail, no compile). Keeps the memo.
    shader_mt_finish(true);
    g_shbatch_state = 2;
    return false;
}

inline bool shader_async_gate(const ShaderJob* jobs, size_t n) {
    if (g_shbatch_state != 0) return false;
    // caller_works = false: it returns instead of stealing, so the pool takes every core rather than cores-1.
    if (!shader_mt_prewarm(jobs, n, false)) {
        g_shbatch_state = 2;   // Nothing to do, or the pool refused - compile inline.
        return false;
    }
    g_shbatch_state = 1;
    return true;
}

// Closes the kept handles of detached workers that have exited.
inline void shpool_orphans_prune() {
    size_t k = 0;
    for (size_t i = 0; i < g_shpool_orphans.size(); ++i) {
        const HANDLE h = g_shpool_orphans[i];
        if (WaitForSingleObject(h, 0) == WAIT_OBJECT_0) CloseHandle(h);
        else g_shpool_orphans[k++] = h;
    }
    g_shpool_orphans.resize(k);
}

inline void shader_mt_shutdown(bool wait) {
    g_shbatch_abort.store(true, std::memory_order_relaxed);
    const uint64_t t0 = steady_now_ms();
    // INTENTIONAL: both mission-end callers hold the graphics lock on the game thread, so a mission ended during a
    // cold prewarm hitches for the tail of the jobs in flight, bounded here (a heavy unit - PSMain, PSComposite, the
    // effect unit - runs a minute or two under fxc, past the bound: it detaches). The reset paths do not wait at all,
    // nor does a mission end that meets a user-shader batch (USER_END_NOWAIT, session_teardown). A shorter
    // bound trades the hitch for a detached worker on a cold cache.
    const uint64_t bound = wait ? 5000u : 0u;
    while (bound && g_shpool_live.load(std::memory_order_acquire) > 0 &&
           steady_now_ms() - t0 < bound) {
        Sleep(10);
    }
    shpool_orphans_prune();
    if (g_shpool_live.load(std::memory_order_acquire) == 0) {
        shader_mt_finish(false);   // The clean path: join and free everything.
    } else {
        for (std::thread& t : g_shpool_thr) {
            if (!t.joinable()) continue;
            HANDLE h = nullptr;   // Kept for process_detach (g_shpool_orphans).
            if (DuplicateHandle(GetCurrentProcess(), static_cast<HANDLE>(t.native_handle()), GetCurrentProcess(),
                                &h, 0, FALSE, DUPLICATE_SAME_ACCESS) && h) {
                try { g_shpool_orphans.push_back(h); } catch (...) { CloseHandle(h); }
            }
            try { t.detach(); } catch (...) {}
        }
        g_shpool_thr.clear();
        g_shpool_active = false;
    }
    g_shbatch_state = 0;
    {   // SHADER_FAIL_LATCH: a new session (or device) compiles a failed unit once more.
        std::lock_guard<std::mutex> fl(g_shcache_fail_mu);
        g_shcache_fail.clear();
        ++g_shcache_fail_epoch;
    }
    // SHADER_MEMO_EPOCH: the memo too - a detach above leaves it to the detached workers, failures that landed
    // before the edge with it, and a done entry crosses into the next session (a unit no ensure took: a conditional
    // one, another sample count's). Every failure goes; a blob stays (good bytecode in any session). After the bump,
    // so a failure a worker lands from here on is one it drops itself (shader_mt_worker).
    {
        std::lock_guard<std::mutex> ml(g_shpool_mu);
        for (auto it = g_shpool_map.begin(); it != g_shpool_map.end();) {
            if (it->second.done && !it->second.blob) it = g_shpool_map.erase(it);
            else ++it;
        }
    }
}

// The memo-aware front door; identical contract to shader_compile_raw.
inline std::string compile_shader(const char* src, const char* entry, const char* target,
                                  const D3D_SHADER_MACRO* defines, ID3DBlob** out_blob) {
    const uint64_t h = shader_cache_hash(src, entry, target, defines);
    {
        std::unique_lock<std::mutex> l(g_shpool_mu);
        auto it = g_shpool_map.find(h);
        if (it != g_shpool_map.end()) {
            if (!it->second.done && !it->second.taken) {
                it->second.taken = true;   // Steal: this thread compiles it.
                l.unlock();
                ID3DBlob* b = nullptr;
                std::string e;
                try {
                    e = shader_compile_raw(src, entry, target, defines, &b);
                } catch (...) {
                    if (b) { b->Release(); b = nullptr; }
                    e = std::string(entry) + " compile threw";
                }
                l.lock();
                ShaderMemo& m = g_shpool_map[h];
                m.blob = b;
                m.err = e;
                m.done = true;
                l.unlock();
                g_shpool_cv.notify_all();
                l.lock();
            } else if (!it->second.done) {
                g_shpool_cv.wait(l, [h] {
                    auto w = g_shpool_map.find(h);
                    return w == g_shpool_map.end() || w->second.done;
                });
            }
            it = g_shpool_map.find(h);
            if (it != g_shpool_map.end() && it->second.done) {
                *out_blob = it->second.blob;
                const std::string e = it->second.err;
                // SHADER_MEMO_KEEP: a blob stays in the memo, the caller handed a reference of its own, to the
                // memo's next full release (shader_mt_finish(false) - a mission edge, a device reset's clean path,
                // a composite scope - or shader_mt_abandon). Two consumers share keys (ensure_effect_shader at one
                // sample and pip_fx_shader ask for the same PSEffect; a sample-count change and back asks a unit
                // again); taken once, the second would compile the unit inline on the render thread wherever the
                // disk cache does not serve it (no writable Documents folder) - a minute or two of fxc for PSEffect.
                // A failure is taken once (the fail latch answers it after that; a transient one compiles again).
                // Only built-in units come here (the user getters take through shader_memo_try_take). Cost: the
                // taken units' bytecode held to that release - some 5 MB at most (the ten heavy units 0.3 - 0.4 MB
                // each, the rest about 1 MB together).
                if (it->second.blob && e.empty()) it->second.blob->AddRef();
                else g_shpool_map.erase(it);
                return e;
            }
        }
    }
    return shader_compile_raw(src, entry, target, defines, out_blob);
}

// Never blocks: absent or unfinished both read as false. gone distinguishes "still compiling" from "never
// queued / already taken", so the pump can re-arm a request whose memo entry was cleared by a teardown.
inline bool shader_memo_try_take(uint64_t h, ID3DBlob** out,
                                 std::string& err, bool& gone) {
    gone = false;
    std::lock_guard<std::mutex> l(g_shpool_mu);
    auto it = g_shpool_map.find(h);
    if (it == g_shpool_map.end()) { gone = true; return false; }
    if (!it->second.done) return false;   // In flight - do not wait.
    *out = it->second.blob;
    err = it->second.err;
    g_shpool_map.erase(it);
    return true;
}

// A unit still compiling is left in place (a worker holds its node): its blob lands in the map and stays until an
// identical request takes it or the memo's next full release (shader_mt_finish(false): a mission edge, a
// composite scope, a device reset's clean path; or shader_mt_abandon). Used when a pending request is abandoned
// (the cache release at a device reset or a mission edge; a full queue's idle request, USER_REQ_IDLE) so an
// unconsumed blob cannot outlive its request.
inline void shader_memo_discard(uint64_t h) {
    std::lock_guard<std::mutex> l(g_shpool_mu);
    auto it = g_shpool_map.find(h);
    if (it == g_shpool_map.end()) return;
    if (!it->second.done) return;   // In flight - left (above).
    if (it->second.blob) it->second.blob->Release();
    g_shpool_map.erase(it);
}
}