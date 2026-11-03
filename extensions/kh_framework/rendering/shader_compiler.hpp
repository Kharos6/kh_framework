#pragma once

// The feature half (rendering_integration_features.hpp): the shader sources and compiler: the HLSL store, the disk
// cache key, the DXBC check, the compile workers and compile_shader. Never include it on its own.
namespace RenderIntegration {

inline void kh_shader_mt_shutdown(bool khss_wait = true);

inline const std::filesystem::path& kh_mesh_cache_dir();
inline void kh_mesh_cache_trim();

// Defined in rendering/disk_cache.hpp (declared here for the uses below).
inline std::string kh_cache_file_name(const char* khcf_pfx, uint64_t khcf_h,
                                      const char* khcf_ext);
inline void kh_mod_cache_append(std::vector<std::filesystem::path>& khma_out,
                                const std::string& khma_name);

static const wchar_t* const KH_HLSL_RES_NAMES[KH_HLSL_N] = {
    L"KH_CB_HLSL", L"KH_WHITE_HLSL", L"KH_STATIC_HLSL", L"KH_COMPOSITE_HLSL", L"KH_EFFECT_HLSL",
    L"KH_DEPTH_RESOLVE_HLSL", L"KH_SUNPF_HLSL", L"KH_VMIR_CS_HLSL",
    L"KH_SSAO_HLSL", L"KH_SKIN_HLSL",
};

struct KhHlslStore { std::string s[KH_HLSL_N]; };

inline KhHlslStore kh_hlsl_load_all() {
    KhHlslStore khr_out;
    HMODULE khr_hm = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&kh_hlsl_load_all), &khr_hm);

    for (int khr_i = 0; khr_i < KH_HLSL_N; ++khr_i) {
        const char* khr_p = nullptr;
        DWORD khr_sz = 0;
        if (khr_hm) {
            HRSRC khr_r = FindResourceW(khr_hm, KH_HLSL_RES_NAMES[khr_i], MAKEINTRESOURCEW(10));   // RT_RCDATA, W-typed regardless
                                                                                                   // Of unicode.
            if (khr_r) {
                HGLOBAL khr_g = LoadResource(khr_hm, khr_r);
                khr_sz = SizeofResource(khr_hm, khr_r);
                if (khr_g && khr_sz) khr_p = static_cast<const char*>(LockResource(khr_g));
            }
        }

        if (!khr_p) {
            khr_out.s[khr_i] = "// KH: shader resource missing from the DLL - add kh_shaders.rc to the build\n";
            // KH_EA_HLSL_MISSING: named once (from any thread: the store loads on whichever asks first), so the fxc
            // failures that follow (an entry point or an identifier it cannot find) carry their cause.
            std::string khr_name;
            for (const wchar_t* khr_w = KH_HLSL_RES_NAMES[khr_i]; *khr_w; ++khr_w) {
                khr_name += static_cast<char>(*khr_w);   // The names are ASCII.
            }
            report_error_once_safe("KH RenderIntegration: shader sources: resource " + khr_name +
                                   " missing from the DLL (hlslResMissing) - add kh_shaders.rc to the build");
            continue;
        }

        std::string& khr_s = khr_out.s[khr_i];
        khr_s.reserve(khr_sz);
        for (DWORD khr_k = 0; khr_k < khr_sz; ++khr_k) {
            if (khr_p[khr_k] != '\r') khr_s.push_back(khr_p[khr_k]);
        }
    }

    return khr_out;
}

// One load for the process (thread-safe magic static: the shader workers and the two threads that ensure may all
// arrive first). The pointer the .c_str consumers hold is stable for the process lifetime.
inline const std::string& kh_hlsl_src(int khr_i) {
    static const KhHlslStore khr_all = kh_hlsl_load_all();
    return khr_all.s[khr_i];
}

// The compile flags join the shader cache hash (content-addressed over source + entry + target + defines; every hash
// here is CryptoGenerator's FNV-1a 64, field-separated), so a change here recompiles every unit once and a
// differently-flagged blob is never served under the same key. D3DCompile's default is optimization level 1; level
// 3 is the shipping setting (scheduling, register allocation, dead code). Without D3DCOMPILE_IEEE_STRICTNESS fxc is not
// IEEE-strict at any level (it may fuse and reorder float arithmetic); the units' 'precise' qualifiers hold the sums
// that must stay exact.
inline uint32_t kh_shader_flags() {
    return D3DCOMPILE_OPTIMIZATION_LEVEL3;
}

// KH_SHADER_KEY_EPOCH: the key's version, the last value kh_shader_cache_hash folds in - bump it to make every blob
// (ours and every mod's shipped .khsc) a miss, so every unit compiles once more. KH_SHADER_CACHE_VERSION gates only
// the file's header layout.
// Not in the key, deliberately: the d3dcompiler_47.dll's version - so a shipped blob serves every player and survives
// a Windows update of the DLL (a blob is served whichever compiler version wrote it).
static constexpr uint64_t KH_SHADER_KEY_EPOCH = 26481ull;

inline uint64_t kh_shader_cache_hash(const char* src, const char* entry, const char* target,
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
    h ^= static_cast<uint64_t>(kh_shader_flags()); h *= CryptoGenerator::FNV1A64_PRIME;
    h ^= KH_SHADER_KEY_EPOCH; h *= CryptoGenerator::FNV1A64_PRIME;
    return h;
}

// Hits touch the write time: eviction is least-recently-used, the FBX cache's own rule.
inline std::filesystem::path kh_shader_cache_file(uint64_t h) {
    const std::filesystem::path& khsc_d = kh_mesh_cache_dir();
    if (khsc_d.empty()) return std::filesystem::path();
    return khsc_d / kh_cache_file_name("khs_", h, ".khsc");   // KH_CACHE_NAME_ONE: the probe's spelling.
}
static constexpr uint32_t KH_SHADER_CACHE_MAGIC = 0x4353484Bu;   // "khsc".
static constexpr uint32_t KH_SHADER_CACHE_VERSION = 1;
// KH_SHADER_CACHE_SUM: the DXBC container checksum - MD5 over the bytes from offset 20 (past the magic and the
// checksum itself), closed by the container's own final block (the bit count first, then the tail bytes, 0x80, and
// (bits >> 2) | 1 as the last word; a tail of 56 bytes or more takes a block of its own first). Direct3D checks it
// at Create, so a cached blob whose body changed on disk fails there; checked here, it is a miss. Verified against
// DXC's signed containers (every shader of every unit) and every one-bit change to them.
inline uint32_t kh_dxbc_rol(uint32_t khdr_x, int khdr_c) { return (khdr_x << khdr_c) | (khdr_x >> (32 - khdr_c)); }
inline void kh_dxbc_md5_block(uint32_t khdm_s[4], const uint8_t* khdm_p) {
    static const uint32_t khdm_k[64] = {
        0xd76aa478u, 0xe8c7b756u, 0x242070dbu, 0xc1bdceeeu, 0xf57c0fafu, 0x4787c62au, 0xa8304613u, 0xfd469501u,
        0x698098d8u, 0x8b44f7afu, 0xffff5bb1u, 0x895cd7beu, 0x6b901122u, 0xfd987193u, 0xa679438eu, 0x49b40821u,
        0xf61e2562u, 0xc040b340u, 0x265e5a51u, 0xe9b6c7aau, 0xd62f105du, 0x02441453u, 0xd8a1e681u, 0xe7d3fbc8u,
        0x21e1cde6u, 0xc33707d6u, 0xf4d50d87u, 0x455a14edu, 0xa9e3e905u, 0xfcefa3f8u, 0x676f02d9u, 0x8d2a4c8au,
        0xfffa3942u, 0x8771f681u, 0x6d9d6122u, 0xfde5380cu, 0xa4beea44u, 0x4bdecfa9u, 0xf6bb4b60u, 0xbebfbc70u,
        0x289b7ec6u, 0xeaa127fau, 0xd4ef3085u, 0x04881d05u, 0xd9d4d039u, 0xe6db99e5u, 0x1fa27cf8u, 0xc4ac5665u,
        0xf4292244u, 0x432aff97u, 0xab9423a7u, 0xfc93a039u, 0x655b59c3u, 0x8f0ccc92u, 0xffeff47du, 0x85845dd1u,
        0x6fa87e4fu, 0xfe2ce6e0u, 0xa3014314u, 0x4e0811a1u, 0xf7537e82u, 0xbd3af235u, 0x2ad7d2bbu, 0xeb86d391u };
    static const int khdm_r[16] = { 7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21 };
    uint32_t khdm_m[16];
    for (int khdm_i = 0; khdm_i < 16; ++khdm_i) {
        khdm_m[khdm_i] = static_cast<uint32_t>(khdm_p[4 * khdm_i]) |
                         (static_cast<uint32_t>(khdm_p[4 * khdm_i + 1]) << 8) |
                         (static_cast<uint32_t>(khdm_p[4 * khdm_i + 2]) << 16) |
                         (static_cast<uint32_t>(khdm_p[4 * khdm_i + 3]) << 24);
    }
    uint32_t khdm_a = khdm_s[0], khdm_b = khdm_s[1], khdm_c = khdm_s[2], khdm_d = khdm_s[3];
    for (int khdm_i = 0; khdm_i < 64; ++khdm_i) {
        uint32_t khdm_f;
        int khdm_g;
        if (khdm_i < 16)      { khdm_f = (khdm_b & khdm_c) | (~khdm_b & khdm_d); khdm_g = khdm_i; }
        else if (khdm_i < 32) { khdm_f = (khdm_d & khdm_b) | (~khdm_d & khdm_c); khdm_g = (5 * khdm_i + 1) & 15; }
        else if (khdm_i < 48) { khdm_f = khdm_b ^ khdm_c ^ khdm_d; khdm_g = (3 * khdm_i + 5) & 15; }
        else                  { khdm_f = khdm_c ^ (khdm_b | ~khdm_d); khdm_g = (7 * khdm_i) & 15; }
        const uint32_t khdm_t = khdm_d;
        khdm_d = khdm_c;
        khdm_c = khdm_b;
        khdm_b = khdm_b + kh_dxbc_rol(khdm_a + khdm_f + khdm_k[khdm_i] + khdm_m[khdm_g],
                                      khdm_r[(khdm_i >> 4) * 4 + (khdm_i & 3)]);
        khdm_a = khdm_t;
    }
    khdm_s[0] += khdm_a; khdm_s[1] += khdm_b; khdm_s[2] += khdm_c; khdm_s[3] += khdm_d;
}
inline bool kh_dxbc_sum_ok(const uint8_t* khds_p, size_t khds_n) {
    if (khds_n < 32) return false;
    const uint8_t* const khds_q = khds_p + 20;
    const size_t khds_len = khds_n - 20;
    uint32_t khds_s[4] = { 0x67452301u, 0xefcdab89u, 0x98badcfeu, 0x10325476u };
    const size_t khds_full = khds_len & ~static_cast<size_t>(63);
    for (size_t khds_o = 0; khds_o < khds_full; khds_o += 64) kh_dxbc_md5_block(khds_s, khds_q + khds_o);
    const size_t khds_rest = khds_len - khds_full;
    const uint32_t khds_bits = static_cast<uint32_t>(khds_len * 8u);
    uint8_t khds_blk[64];
    auto khds_put = [](uint8_t* khds_d, uint32_t khds_v) {
        khds_d[0] = static_cast<uint8_t>(khds_v);
        khds_d[1] = static_cast<uint8_t>(khds_v >> 8);
        khds_d[2] = static_cast<uint8_t>(khds_v >> 16);
        khds_d[3] = static_cast<uint8_t>(khds_v >> 24);
    };
    memset(khds_blk, 0, sizeof(khds_blk));
    if (khds_rest >= 56) {
        memcpy(khds_blk, khds_q + khds_full, khds_rest);
        khds_blk[khds_rest] = 0x80;
        kh_dxbc_md5_block(khds_s, khds_blk);
        memset(khds_blk, 0, sizeof(khds_blk));
        khds_put(khds_blk, khds_bits);
    } else {
        khds_put(khds_blk, khds_bits);
        memcpy(khds_blk + 4, khds_q + khds_full, khds_rest);
        khds_blk[4 + khds_rest] = 0x80;
    }
    khds_put(khds_blk + 60, (khds_bits >> 2) | 1u);
    kh_dxbc_md5_block(khds_s, khds_blk);
    uint8_t khds_dig[16];
    for (int khds_i = 0; khds_i < 4; ++khds_i) khds_put(khds_dig + 4 * khds_i, khds_s[khds_i]);
    return memcmp(khds_dig, khds_p + 4, 16) == 0;
}
// Read them as a pair.
static std::atomic<uint64_t> g_shader_cache_hits{0};
static std::atomic<uint64_t> g_shader_cache_misses{0};

// A memo miss compiles inline, so a job list that drifts out of step with its ensure costs speed, never correctness.
struct KhShaderJob {
    const char* src;
    const char* entry;
    const char* target;
    const D3D_SHADER_MACRO* defines;
    uint64_t hash;   // Filled by the prewarm.
};
struct KhShaderMemo {
    ID3DBlob*   blob;
    std::string err;
    bool        done;
    bool        taken;
};
static std::mutex g_khsm_mu;
static std::condition_variable g_khsm_cv;
static std::unordered_map<uint64_t, KhShaderMemo> g_khsm_map;
static std::vector<KhShaderJob> g_khsm_q;   // Built before any worker starts.
static std::atomic<size_t> g_khsm_next{0};
// Deliberately never destructed (detached workers may outlive static teardown).
static std::vector<std::thread>& g_khsm_thr = *(new std::vector<std::thread>());
// The workers kh_shader_mt_shutdown detached, by a duplicate of each thread's handle (the detach closes std::thread's
// own): process_detach bounds and ends them as it does the listed ones. Pruned of exited threads at each shutdown.
// Never destructed, as g_khsm_thr; the same callers touch both.
static std::vector<HANDLE>& g_khsm_orphans = *(new std::vector<HANDLE>());
static bool g_khsm_active = false;   // A batch is live (caller thread only).
static std::atomic<int> g_khsm_live{0};
static std::atomic<bool> g_khsa_abort{false};
struct KhShaderJobOwned {
    std::shared_ptr<std::string> src_sh;   // Shared across jobs over one unit.
    std::string entry, target;
    std::vector<std::string> dn, dv;
    std::vector<unsigned char> dhas;
    std::vector<D3D_SHADER_MACRO> macros;
    bool had_defines;
};
static std::vector<std::unique_ptr<KhShaderJobOwned>> g_khsm_own;
// Decrements on every worker exit path, normal or exceptional.
struct KhShaderLive {
    ~KhShaderLive() { g_khsm_live.fetch_sub(1, std::memory_order_acq_rel); }
};
static std::mutex g_khsm_trim_mu;   // kh_mesh_cache_trim is not re-entrant.
// KH_CACHE_TRIM_THROTTLE - shader-blob bytes written since the last trim a shader write ran. The trim scans the
// whole cache directory (per blob it would be ~95 scans for a cold start's prewarm, serialised across the pool on
// g_khsm_trim_mu, and on the render thread for an inline compile). A blob counts against its own budget only
// (KH_CACHE_KIND_BUDGET), so one scan per 4 MiB written holds .khsc within 4 MiB of what the trim allows (its budget,
// or twice it for this session's own blobs - KH_CACHE_SESSION_KEEP); the debt starts full at each session
// (kh_session_globals_reset), so a session's first blob scans. Mesh and texture writes trim at every write.
static constexpr uint64_t KH_CACHE_SHADER_TRIM_SLACK = 4ull << 20;
static std::atomic<uint64_t> g_khsc_trim_debt{ KH_CACHE_SHADER_TRIM_SLACK };
// KH_SHADER_FAIL_LATCH - the D3DCompile failures of this session, by cache hash (the source, entry, target, defines
// and flags): a unit that failed with fxc's message fails the same way every time; unlatched, a failed built-in unit
// compiles again at every ensure, on the render thread, several times a frame. kh_shader_compile_raw answers a
// latched hash with the stored message ahead of the disk probe - a failed unit wrote no blob and the mod caches are
// fixed at first use, so the probe would miss again (a latched answer counts as neither a cache hit nor a miss). Not
// latched: a failure without fxc's message, or out of memory (E_OUTOFMEMORY and the Win32 not-enough-memory codes -
// transient). Cleared by kh_shader_mt_shutdown (the mission edges, a device reset), which also bumps
// g_khsc_fail_epoch: a compile that began before the clear (a detached worker's, still inside D3DCompile) does not
// latch into the next session, nor reach it through the memo (KH_SHADER_MEMO_EPOCH, kh_shader_mt_worker).
// Never destructed (a detached worker may compile past static teardown), as g_khsm_thr.
static std::mutex& g_khsc_fail_mu = *(new std::mutex());
static std::unordered_map<uint64_t, std::string>& g_khsc_fail = *(new std::unordered_map<uint64_t, std::string>());
static uint64_t g_khsc_fail_epoch = 0u;   // KH_SHADER_FAIL_LATCH: under g_khsc_fail_mu.


inline std::string kh_shader_compile_raw(const char* src, const char* entry, const char* target,
                                         const D3D_SHADER_MACRO* defines, ID3DBlob** out_blob) {
    ID3DBlob* err_blob = nullptr;

    const uint64_t khsc_h = kh_shader_cache_hash(src, entry, target, defines);
    uint64_t khsc_epoch = 0u;   // KH_SHADER_FAIL_LATCH: the session this compile began in.
    {   // KH_SHADER_FAIL_LATCH: a unit that already failed this session answers with its message.
        std::lock_guard<std::mutex> khsc_fl_l(g_khsc_fail_mu);
        khsc_epoch = g_khsc_fail_epoch;
        const auto khsc_fit = g_khsc_fail.find(khsc_h);
        if (khsc_fit != g_khsc_fail.end()) {
            *out_blob = nullptr;
            return khsc_fit->second;
        }
    }
    const std::filesystem::path khsc_p = kh_shader_cache_file(khsc_h);

    // Index 0 is the Documents copy and the only one the lru touches (a mod folder may be read-only). Ours first,
    // then every mod that ships this blob.
    std::vector<std::filesystem::path> khsc_cand;
    if (!khsc_p.empty()) khsc_cand.push_back(khsc_p);
    kh_mod_cache_append(khsc_cand, kh_cache_file_name("khs_", khsc_h, ".khsc"));

    for (size_t khsc_ci = 0; khsc_ci < khsc_cand.size(); ++khsc_ci) {
        const bool khsc_own = khsc_ci == 0 && !khsc_p.empty();

        try {
            std::ifstream khsc_f(khsc_cand[khsc_ci], std::ios::binary);
            if (khsc_f) {
                struct { uint32_t magic, ver; uint64_t hash, size; } khsc_hdr = {};
                khsc_f.read(reinterpret_cast<char*>(&khsc_hdr), sizeof(khsc_hdr));
                if (khsc_f.gcount() == sizeof(khsc_hdr) &&
                    khsc_hdr.magic == KH_SHADER_CACHE_MAGIC &&
                    khsc_hdr.ver == KH_SHADER_CACHE_VERSION &&
                    khsc_hdr.hash == khsc_h &&
                    khsc_hdr.size > 0 && khsc_hdr.size < (64ull << 20)) {
                    ID3DBlob* khsc_b = nullptr;
                    if (SUCCEEDED(D3DCreateBlob(static_cast<SIZE_T>(khsc_hdr.size), &khsc_b))) {
                        khsc_f.read(static_cast<char*>(khsc_b->GetBufferPointer()),
                                    static_cast<std::streamsize>(khsc_hdr.size));
                        bool khsc_dxbc = false;
                        if (khsc_f.gcount() == static_cast<std::streamsize>(khsc_hdr.size) &&
                            khsc_hdr.size >= 32) {
                            const uint8_t* khsc_pb = static_cast<const uint8_t*>(khsc_b->GetBufferPointer());
                            uint32_t khsc_tot = 0;
                            memcpy(&khsc_tot, khsc_pb + 24, sizeof(khsc_tot));
                            // KH_SHADER_CACHE_SUM: a body changed on disk fails here, as the Create would.
                            khsc_dxbc = khsc_pb[0] == 'D' && khsc_pb[1] == 'X' &&
                                        khsc_pb[2] == 'B' && khsc_pb[3] == 'C' &&
                                        khsc_tot == static_cast<uint32_t>(khsc_hdr.size) &&
                                        kh_dxbc_sum_ok(khsc_pb, static_cast<size_t>(khsc_hdr.size));
                        }
                        if (khsc_f.gcount() == static_cast<std::streamsize>(khsc_hdr.size) && !khsc_dxbc) {
                            khsc_f.close();
                            if (khsc_own) {   // Ours only; a mod's copy is not our budget.
                                std::error_code khsc_rm;
                                std::filesystem::remove(khsc_cand[khsc_ci], khsc_rm);
                            }
                        }
                        if (khsc_dxbc) {
                            khsc_f.close();

                            if (khsc_own) {
                                std::error_code khsc_ec;
                                std::filesystem::last_write_time(
                                    khsc_cand[khsc_ci],
                                    std::filesystem::file_time_type::clock::now(), khsc_ec);
                            }

                            *out_blob = khsc_b;
                            if (kh_stats_on()) g_shader_cache_hits.fetch_add(1, std::memory_order_relaxed);
                            return "";   // Cache hit - no compile.
                        }
                        khsc_b->Release();
                    }
                }
            }
        } catch (...) {}   // Any doubt is a miss; the next candidate or the compile.
    }

    if (kh_stats_on()) g_shader_cache_misses.fetch_add(1, std::memory_order_relaxed);
    const uint32_t khsc_fl = kh_shader_flags();
    HRESULT hr = D3DCompile(src, strlen(src), "kh_render", defines, nullptr,
                            entry, target, khsc_fl, 0, out_blob, &err_blob);

    if (FAILED(hr)) {
        std::string msg = std::string(entry) + " compile failed " + hr_str(hr);
        const bool khsc_latch = err_blob != nullptr && hr != E_OUTOFMEMORY &&   // KH_SHADER_FAIL_LATCH.
                                hr != HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY) &&
                                hr != HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

        if (err_blob) {
            msg += ": ";
            msg += static_cast<const char*>(err_blob->GetBufferPointer());
            err_blob->Release();
        }

        if (khsc_latch) {
            try {
                std::lock_guard<std::mutex> khsc_fl_l(g_khsc_fail_mu);
                if (khsc_epoch == g_khsc_fail_epoch) g_khsc_fail[khsc_h] = msg;
            } catch (...) {}   // Unlatched: the next call compiles again.
        }

        return msg;
    }

    if (err_blob) err_blob->Release();

    // Cache write-through (best-effort), then the trim - each kind under its own budget (KH_CACHE_KIND_BUDGET).
    if (!khsc_p.empty() && *out_blob) {
        // Written to a private temp name and renamed into place so two processes sharing the cache dir never read
        // each other's half-written blob. A sharing violation leaves the existing file and drops the temp.
        // KH_DOCS_WIDE: appended to the path itself - khsc_p.string() throws on a name outside the ANSI page.
        std::filesystem::path khsc_tmp = khsc_p;
        khsc_tmp += ".tmp" + std::to_string(static_cast<unsigned long>(GetCurrentProcessId()));
        bool khsc_wrote = false;
        try {
            std::ofstream khsc_w(khsc_tmp, std::ios::binary | std::ios::trunc);
            if (khsc_w) {
                struct { uint32_t magic, ver; uint64_t hash, size; } khsc_hdr = {
                    KH_SHADER_CACHE_MAGIC, KH_SHADER_CACHE_VERSION,
                    khsc_h, static_cast<uint64_t>((*out_blob)->GetBufferSize()) };
                khsc_w.write(reinterpret_cast<const char*>(&khsc_hdr), sizeof(khsc_hdr));
                khsc_w.write(static_cast<const char*>((*out_blob)->GetBufferPointer()),
                             static_cast<std::streamsize>((*out_blob)->GetBufferSize()));
                khsc_w.flush();
                khsc_wrote = static_cast<bool>(khsc_w);
            }
        } catch (...) { khsc_wrote = false; }
        std::error_code khsc_ec;
        if (khsc_wrote) std::filesystem::rename(khsc_tmp, khsc_p, khsc_ec);
        if (!khsc_wrote || khsc_ec) std::filesystem::remove(khsc_tmp, khsc_ec);
        // KH_CACHE_TRIM_THROTTLE: a scan per KH_CACHE_SHADER_TRIM_SLACK bytes written, not per blob.
        const uint64_t khsc_sz = static_cast<uint64_t>((*out_blob)->GetBufferSize());
        if (khsc_wrote && !khsc_ec &&
            g_khsc_trim_debt.fetch_add(khsc_sz, std::memory_order_relaxed) + khsc_sz >= KH_CACHE_SHADER_TRIM_SLACK) {
            std::lock_guard<std::mutex> khsm_tl(g_khsm_trim_mu);
            g_khsc_trim_debt.store(0u, std::memory_order_relaxed);
            kh_mesh_cache_trim();
        }
    }
    return "";
}

inline void kh_shader_mt_worker() {
    KhShaderLive khsm_lv;
    uint64_t khsm_cur = 0;
    uint64_t khsm_ep = 0u;   // KH_SHADER_MEMO_EPOCH: the session (g_khsc_fail_epoch) the held job began in.
    bool khsm_held = false;
    try {
        for (;;) {
            // Teardown stops new claims at once; a job already inside D3DCompile has to finish.
            if (g_khsa_abort.load(std::memory_order_relaxed)) return;
            const size_t khsm_i = g_khsm_next.fetch_add(1, std::memory_order_relaxed);
            if (khsm_i >= g_khsm_q.size()) return;
            const KhShaderJob khsm_j = g_khsm_q[khsm_i];
            {
                std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
                auto khsm_it = g_khsm_map.find(khsm_j.hash);
                if (khsm_it == g_khsm_map.end() ||
                    khsm_it->second.done || khsm_it->second.taken) continue;
                khsm_it->second.taken = true;
            }
            khsm_cur = khsm_j.hash;
            khsm_held = true;   // From here a result is owed to any waiter.
            {   // KH_SHADER_MEMO_EPOCH: read as kh_shader_compile_raw reads it for the fail latch.
                std::lock_guard<std::mutex> khsm_fl(g_khsc_fail_mu);
                khsm_ep = g_khsc_fail_epoch;
            }
            ID3DBlob* khsm_b = nullptr;
            std::string khsm_e;
            try {
                khsm_e = kh_shader_compile_raw(khsm_j.src, khsm_j.entry,
                                               khsm_j.target, khsm_j.defines, &khsm_b);
            } catch (...) {
                if (khsm_b) { khsm_b->Release(); khsm_b = nullptr; }
                khsm_e = std::string(khsm_j.entry) + " compile threw";
            }
            {
                std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
                // KH_SHADER_MEMO_EPOCH: a failure from a job that began before a shutdown (this worker was detached
                // and runs on into the next session or device) is erased, not stored: the memo keeps a done entry
                // across the edge (KH_SHADER_ORPHANS), so the next session's consumer would take this session's
                // error (a transient one included) against the fail latch's rule that a new session compiles a
                // failed unit once more. A blob is good bytecode in any session and is stored. Lock order: g_khsm_mu,
                // then g_khsc_fail_mu (nothing takes them the other way round).
                bool khsm_stale = false;
                if (!khsm_b) {
                    std::lock_guard<std::mutex> khsm_fl(g_khsc_fail_mu);
                    khsm_stale = khsm_ep != g_khsc_fail_epoch;
                }
                if (khsm_stale) {
                    g_khsm_map.erase(khsm_j.hash);
                } else {
                    KhShaderMemo& khsm_m = g_khsm_map[khsm_j.hash];
                    khsm_m.blob = khsm_b;
                    khsm_m.err = khsm_e;
                    khsm_m.done = true;
                }
            }
            khsm_held = false;
            g_khsm_cv.notify_all();
        }
    } catch (...) {
        if (khsm_held) {
            try {
                std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
                bool khsm_stale = false;   // KH_SHADER_MEMO_EPOCH, as above.
                {
                    std::lock_guard<std::mutex> khsm_fl(g_khsc_fail_mu);
                    khsm_stale = khsm_ep != g_khsc_fail_epoch;
                }
                if (khsm_stale) {
                    g_khsm_map.erase(khsm_cur);
                } else {
                    KhShaderMemo& khsm_m = g_khsm_map[khsm_cur];
                    khsm_m.done = true;
                    khsm_m.err = "shader worker aborted";
                }
            } catch (...) {}
            g_khsm_cv.notify_all();
        }
    }
}

// A batch that was built but never got a worker is torn down here: the whole map (this call's entries and any memo
// it held), the owned sources, the queue. No worker exists at either call site, so the map is touched without the
// lock. The caller compiles inline.
inline void kh_shader_mt_abandon() {
    for (auto& khsm_kv : g_khsm_map) {
        if (khsm_kv.second.blob) khsm_kv.second.blob->Release();
    }
    g_khsm_map.clear();
    g_khsm_own.clear();
    g_khsm_q.clear();
    g_khsm_next.store(0, std::memory_order_relaxed);
    g_khsm_active = false;
}

inline bool kh_shader_mt_prewarm(const KhShaderJob* khsm_jobs, size_t khsm_n,
                                 bool khsm_caller_works = true) {
    if (!khsm_jobs || khsm_n == 0) return false;
    if (g_khsm_active) return false;
    // A detached worker from a previous session may still be reading the queue and the owned-job storage; refusing
    // here is what makes that detach safe.
    if (g_khsm_live.load(std::memory_order_acquire) > 0) return false;
    // KH_SHADER_ORPHANS - a shutdown that could not wait (the device reset's never does; a mission end's gives up
    // after its bound) detaches its workers and LEAVES the memo for them: they finish the unit they hold and exit on
    // the abort flag; a unit none of them had claimed stays in the map undone for good (the queue below skips a hash
    // the map holds, and nothing erases it). Left there it holds the main ensure's deferral up for the session
    // whenever this batch queues anything new - white placeholders, no mesh (kh_shader_async_hold waits for EVERY
    // entry; a reset changes the sample-count defines, so new units are the usual case) - or with nothing new sends
    // every orphaned unit to an inline compile inside the ensure (the seconds-long stall the pool exists to avoid),
    // and keeps a user-shader batch from being reaped (kh_user_shader_reap waits the same way) so g_khsm_active
    // stands and every later prewarm is refused. Here no batch is live and no worker exists, so an undone entry is an
    // orphan by construction: dropped (it holds no blob) and queued again below. Done entries stay - a blob is good
    // bytecode, and a failure is this session's (KH_SHADER_MEMO_EPOCH: the shutdown and the worker drop an earlier
    // session's).
    {
        std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
        for (auto khsm_it = g_khsm_map.begin(); khsm_it != g_khsm_map.end();) {
            if (!khsm_it->second.done) khsm_it = g_khsm_map.erase(khsm_it);
            else ++khsm_it;
        }
    }
    g_khsa_abort.store(false, std::memory_order_relaxed);
    unsigned khsm_hw = std::thread::hardware_concurrency();
    if (khsm_hw == 0) khsm_hw = 1;
    if (khsm_hw > 64) khsm_hw = 64;
    // One core: a pool would only add threads to work the caller would do itself. KH_SHADER_ONE_CORE - do not refuse
    // when the caller does not work (the async gate, the user-shader pump): refused, the gate compiles the whole set
    // inline on the render thread (minutes on a cold cache, the game frozen) and the pump never arms, so no user
    // shader ever compiles. They get the one worker below; the frames draw the placeholder meanwhile.
    if (khsm_hw < 2 && khsm_caller_works) return false;
    try {
    std::unordered_map<const char*, std::shared_ptr<std::string>> khsm_srcs;
    g_khsm_q.clear();
    // The owned-job storage is freed here, not only at shutdown: user-shader batches each carry a full
    // cb.hlsl-prefixed source. The memo holds blobs and error strings only.
    g_khsm_own.clear();
    g_khsm_q.reserve(khsm_n);
    {
        std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
        for (size_t khsm_i = 0; khsm_i < khsm_n; ++khsm_i) {
            const KhShaderJob khsm_in = khsm_jobs[khsm_i];
            if (!khsm_in.src || !khsm_in.entry || !khsm_in.target) continue;
            const uint64_t khsm_h = kh_shader_cache_hash(khsm_in.src, khsm_in.entry,
                                                         khsm_in.target, khsm_in.defines);
            if (g_khsm_map.find(khsm_h) != g_khsm_map.end()) continue;
            // Deep copy: the caller's strings and macro table are function locals and a worker outlives them.
            std::unique_ptr<KhShaderJobOwned> khsm_o(new KhShaderJobOwned());
            {
                auto khsm_si = khsm_srcs.find(khsm_in.src);
                if (khsm_si == khsm_srcs.end())
                    khsm_si = khsm_srcs.emplace(khsm_in.src,
                        std::make_shared<std::string>(khsm_in.src)).first;
                khsm_o->src_sh = khsm_si->second;
            }
            khsm_o->entry = khsm_in.entry;
            khsm_o->target = khsm_in.target;
            khsm_o->had_defines = (khsm_in.defines != nullptr);
            for (const D3D_SHADER_MACRO* khsm_d = khsm_in.defines;
                 khsm_d && khsm_d->Name; ++khsm_d) {
                khsm_o->dn.push_back(khsm_d->Name);
                // A null Definition is preserved as null (dhas): the cache hash mixes nothing for null but a
                // separator for "" - collapsing them would change every key.
                khsm_o->dv.push_back(khsm_d->Definition ? khsm_d->Definition : "");
                khsm_o->dhas.push_back(khsm_d->Definition ? 1u : 0u);
            }
            khsm_o->macros.reserve(khsm_o->dn.size() + 1);
            for (size_t khsm_k = 0; khsm_k < khsm_o->dn.size(); ++khsm_k) {
                D3D_SHADER_MACRO khsm_m;
                khsm_m.Name = khsm_o->dn[khsm_k].c_str();
                khsm_m.Definition = khsm_o->dhas[khsm_k] ? khsm_o->dv[khsm_k].c_str()
                                                         : nullptr;   // Null != "".
                khsm_o->macros.push_back(khsm_m);
            }
            D3D_SHADER_MACRO khsm_term; khsm_term.Name = nullptr; khsm_term.Definition = nullptr;
            khsm_o->macros.push_back(khsm_term);
            KhShaderJob khsm_j;
            khsm_j.src = khsm_o->src_sh->c_str();
            khsm_j.entry = khsm_o->entry.c_str();
            khsm_j.target = khsm_o->target.c_str();
            khsm_j.defines = khsm_o->had_defines ? khsm_o->macros.data() : nullptr;
            khsm_j.hash = khsm_h;
            g_khsm_map[khsm_h] = KhShaderMemo{ nullptr, std::string(), false, false };
            g_khsm_own.push_back(std::move(khsm_o));
            g_khsm_q.push_back(khsm_j);
        }
    }
    if (g_khsm_q.empty()) return false;
    g_khsm_active = true;   // From here the caller owns the teardown.
    g_khsm_next.store(0, std::memory_order_relaxed);
    // When the caller does not participate (the async gate returns instead of stealing) the pool gets every core.
    size_t khsm_w = static_cast<size_t>(khsm_hw) - (khsm_caller_works ? 1u : 0u);
    const size_t khsm_cap = khsm_caller_works ? g_khsm_q.size() - 1 : g_khsm_q.size();
    if (khsm_w > khsm_cap) khsm_w = khsm_cap;
    for (size_t khsm_i = 0; khsm_i < khsm_w; ++khsm_i) {
        g_khsm_live.fetch_add(1, std::memory_order_acq_rel);   // Before the spawn.
        try {
            g_khsm_thr.emplace_back(kh_shader_mt_worker);
        } catch (...) {
            g_khsm_live.fetch_sub(1, std::memory_order_acq_rel);
            break;   // Fewer threads is slower, never wrong.
        }
    }
    if (khsm_w > 0 && g_khsm_thr.empty()) {
        kh_shader_mt_abandon();
        return false;
    }
    return true;
    } catch (...) {
        // Every throw site above precedes the spawn loop, so a throw here means map entries with no worker to finish
        // them; kh_shader_async_gate has no destructor and would scan them every flush forever with white
        // placeholders. Tear down and refuse; the batches of KhShaderMtScope and the async gate then compile inline,
        // and the user-shader pump leaves its requests unarmed for a later pump. The spawned-worker branch is
        // unreachable today and kept so a future throw after the spawn cannot orphan a live thread.
        if (g_khsm_thr.empty()) {
            kh_shader_mt_abandon();
            return false;
        }
        g_khsm_active = true;   // Workers exist: the caller's teardown joins them.
        return true;
    }
}

// Join every worker, then (unless khsm_keep_memo) release every blob the memo holds: those no call site consumed (a
// conditional ensure can legitimately skip a unit the pool compiled) and those kept after a take (KH_SHADER_MEMO_KEEP).
inline void kh_shader_mt_finish(bool khsm_keep_memo = false) {
    for (std::thread& khsm_t : g_khsm_thr) {
        if (khsm_t.joinable()) {
            try { khsm_t.join(); } catch (...) {}
        }
    }
    g_khsm_thr.clear();
    std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
    if (!khsm_keep_memo) {
        for (auto& khsm_kv : g_khsm_map) {
            if (khsm_kv.second.blob) khsm_kv.second.blob->Release();
        }
        g_khsm_map.clear();
    }
    // KH_KHSM_OWN_FREE: the batch's copied sources and defines go in both branches (every worker is joined, the
    // queue that pointed into them is cleared below, and the memo holds blobs and strings of its own).
    g_khsm_own.clear();
    g_khsm_q.clear();
    g_khsm_q.shrink_to_fit();
    g_khsm_next.store(0, std::memory_order_relaxed);
    g_khsm_active = false;
}

// One declaration per batch covers every early return, Create failure and throw. The prewarm deep-copies the jobs'
// sources and define tables (KhShaderJobOwned), so the scope holds no pointer into the caller's strings.
struct KhShaderMtScope {
    bool khsm_own;   // Only tear down a batch this scope armed.
    bool khsm_keep;   // Leave the compiled blobs for a later ensure.
    KhShaderMtScope(const KhShaderJob* khsm_j, size_t khsm_n, bool khsm_k = false)
        : khsm_own(kh_shader_mt_prewarm(khsm_j, khsm_n)), khsm_keep(khsm_k) {}
    ~KhShaderMtScope() {
        if (khsm_own) kh_shader_mt_finish(khsm_keep);
    }
    KhShaderMtScope(const KhShaderMtScope&) = delete;
    KhShaderMtScope& operator=(const KhShaderMtScope&) = delete;
};

static int      g_khsa_state = 0;   // 0 idle, 1 compiling, 2 done.

// The poll needs no source, so it runs before the strings are built on a deferred frame.
inline bool kh_shader_async_hold() {
    if (g_khsa_state == 2) return false;
    if (g_khsa_state == 0) {
        if (g_khsm_live.load(std::memory_order_acquire) > 0) {
            return true;
        }
        return false;   // Nothing armed yet; kh_shader_async_gate arms below.
    }
    bool khsa_pending = false;
    {
        std::lock_guard<std::mutex> khsa_l(g_khsm_mu);
        for (auto& khsa_kv : g_khsm_map) {
            if (!khsa_kv.second.done) { khsa_pending = true; break; }
        }
    }
    if (khsa_pending) {
        return true;
    }
    // KH_JOIN_TAIL: every unit has landed, so no worker is compiling - each is past its last result and returning
    // (the join waits out that tail, no compile). Keeps the memo.
    kh_shader_mt_finish(true);
    g_khsa_state = 2;
    return false;
}

inline bool kh_shader_async_gate(const KhShaderJob* khsa_jobs, size_t khsa_n) {
    if (g_khsa_state != 0) return false;
    // caller_works = false: it returns instead of stealing, so the pool takes every core rather than cores-1.
    if (!kh_shader_mt_prewarm(khsa_jobs, khsa_n, false)) {
        g_khsa_state = 2;   // Nothing to do, or the pool refused - compile inline.
        return false;
    }
    g_khsa_state = 1;
    return true;
}

// Closes the kept handles of detached workers that have exited.
inline void kh_khsm_orphans_prune() {
    size_t khop_k = 0;
    for (size_t khop_i = 0; khop_i < g_khsm_orphans.size(); ++khop_i) {
        const HANDLE khop_h = g_khsm_orphans[khop_i];
        if (WaitForSingleObject(khop_h, 0) == WAIT_OBJECT_0) CloseHandle(khop_h);
        else g_khsm_orphans[khop_k++] = khop_h;
    }
    g_khsm_orphans.resize(khop_k);
}

inline void kh_shader_mt_shutdown(bool khss_wait) {
    g_khsa_abort.store(true, std::memory_order_relaxed);
    const uint64_t khss_t0 = steady_now_ms();
    // INTENTIONAL: both mission-end callers hold the graphics lock on the game thread, so a mission ended during a
    // cold prewarm hitches for the tail of the jobs in flight, bounded here (a heavy unit - PSMain, PSComposite, the
    // effect unit - runs a minute or two under fxc, past the bound: it detaches). The reset paths do not wait at all,
    // nor does a mission end that meets a user-shader batch (KH_USER_END_NOWAIT, kh_session_teardown). A shorter
    // bound trades the hitch for a detached worker on a cold cache.
    const uint64_t khss_bound = khss_wait ? 5000u : 0u;
    while (khss_bound && g_khsm_live.load(std::memory_order_acquire) > 0 &&
           steady_now_ms() - khss_t0 < khss_bound) {
        Sleep(10);
    }
    kh_khsm_orphans_prune();
    if (g_khsm_live.load(std::memory_order_acquire) == 0) {
        kh_shader_mt_finish(false);   // The clean path: join and free everything.
    } else {
        for (std::thread& khss_t : g_khsm_thr) {
            if (!khss_t.joinable()) continue;
            HANDLE khss_h = nullptr;   // Kept for process_detach (g_khsm_orphans).
            if (DuplicateHandle(GetCurrentProcess(), static_cast<HANDLE>(khss_t.native_handle()), GetCurrentProcess(),
                                &khss_h, 0, FALSE, DUPLICATE_SAME_ACCESS) && khss_h) {
                try { g_khsm_orphans.push_back(khss_h); } catch (...) { CloseHandle(khss_h); }
            }
            try { khss_t.detach(); } catch (...) {}
        }
        g_khsm_thr.clear();
        g_khsm_active = false;
    }
    g_khsa_state = 0;
    {   // KH_SHADER_FAIL_LATCH: a new session (or device) compiles a failed unit once more.
        std::lock_guard<std::mutex> khss_fl(g_khsc_fail_mu);
        g_khsc_fail.clear();
        ++g_khsc_fail_epoch;
    }
    // KH_SHADER_MEMO_EPOCH: the memo too - a detach above leaves it to the detached workers, failures that landed
    // before the edge with it, and a done entry crosses into the next session (a unit no ensure took: a conditional
    // one, another sample count's). Every failure goes; a blob stays (good bytecode in any session). After the bump,
    // so a failure a worker lands from here on is one it drops itself (kh_shader_mt_worker).
    {
        std::lock_guard<std::mutex> khss_ml(g_khsm_mu);
        for (auto khss_it = g_khsm_map.begin(); khss_it != g_khsm_map.end();) {
            if (khss_it->second.done && !khss_it->second.blob) khss_it = g_khsm_map.erase(khss_it);
            else ++khss_it;
        }
    }
}

// The memo-aware front door; identical contract to kh_shader_compile_raw.
inline std::string compile_shader(const char* src, const char* entry, const char* target,
                                  const D3D_SHADER_MACRO* defines, ID3DBlob** out_blob) {
    const uint64_t khsm_h = kh_shader_cache_hash(src, entry, target, defines);
    {
        std::unique_lock<std::mutex> khsm_l(g_khsm_mu);
        auto khsm_it = g_khsm_map.find(khsm_h);
        if (khsm_it != g_khsm_map.end()) {
            if (!khsm_it->second.done && !khsm_it->second.taken) {
                khsm_it->second.taken = true;   // Steal: this thread compiles it.
                khsm_l.unlock();
                ID3DBlob* khsm_b = nullptr;
                std::string khsm_e;
                try {
                    khsm_e = kh_shader_compile_raw(src, entry, target, defines, &khsm_b);
                } catch (...) {
                    if (khsm_b) { khsm_b->Release(); khsm_b = nullptr; }
                    khsm_e = std::string(entry) + " compile threw";
                }
                khsm_l.lock();
                KhShaderMemo& khsm_m = g_khsm_map[khsm_h];
                khsm_m.blob = khsm_b;
                khsm_m.err = khsm_e;
                khsm_m.done = true;
                khsm_l.unlock();
                g_khsm_cv.notify_all();
                khsm_l.lock();
            } else if (!khsm_it->second.done) {
                g_khsm_cv.wait(khsm_l, [khsm_h] {
                    auto khsm_w = g_khsm_map.find(khsm_h);
                    return khsm_w == g_khsm_map.end() || khsm_w->second.done;
                });
            }
            khsm_it = g_khsm_map.find(khsm_h);
            if (khsm_it != g_khsm_map.end() && khsm_it->second.done) {
                *out_blob = khsm_it->second.blob;
                const std::string khsm_e = khsm_it->second.err;
                // KH_SHADER_MEMO_KEEP: a blob stays in the memo, the caller handed a reference of its own, to the
                // memo's next full release (kh_shader_mt_finish(false) - a mission edge, a device reset's clean path,
                // a composite scope - or kh_shader_mt_abandon). Two consumers share keys (ensure_effect_shader at one
                // sample and kh_pip_fx_shader ask for the same PSEffect; a sample-count change and back asks a unit
                // again); taken once, the second would compile the unit inline on the render thread wherever the
                // disk cache does not serve it (no writable Documents folder) - a minute or two of fxc for PSEffect.
                // A failure is taken once (the fail latch answers it after that; a transient one compiles again).
                // Only built-in units come here (the user getters take through kh_shader_memo_try_take). Cost: the
                // taken units' bytecode held to that release - some 5 MB at most (the ten heavy units 0.3 - 0.4 MB
                // each, the rest about 1 MB together).
                if (khsm_it->second.blob && khsm_e.empty()) khsm_it->second.blob->AddRef();
                else g_khsm_map.erase(khsm_it);
                return khsm_e;
            }
        }
    }
    return kh_shader_compile_raw(src, entry, target, defines, out_blob);
}

// Never blocks: absent or unfinished both read as false. khsm_gone distinguishes "still compiling" from "never
// queued / already taken", so the pump can re-arm a request whose memo entry was cleared by a teardown.
inline bool kh_shader_memo_try_take(uint64_t khsm_h, ID3DBlob** khsm_out,
                                    std::string& khsm_err, bool& khsm_gone) {
    khsm_gone = false;
    std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
    auto khsm_it = g_khsm_map.find(khsm_h);
    if (khsm_it == g_khsm_map.end()) { khsm_gone = true; return false; }
    if (!khsm_it->second.done) return false;   // In flight - do not wait.
    *khsm_out = khsm_it->second.blob;
    khsm_err = khsm_it->second.err;
    g_khsm_map.erase(khsm_it);
    return true;
}

// A unit still compiling is left in place (a worker holds its node): its blob lands in the map and stays until an
// identical request takes it or the memo's next full release (kh_shader_mt_finish(false): a mission edge, a
// composite scope, a device reset's clean path; or kh_shader_mt_abandon). Used when a pending request is abandoned
// (the cache release at a device reset or a mission edge; a full queue's idle request, KH_USER_REQ_IDLE) so an
// unconsumed blob cannot outlive its request.
inline void kh_shader_memo_discard(uint64_t khsm_h) {
    std::lock_guard<std::mutex> khsm_l(g_khsm_mu);
    auto khsm_it = g_khsm_map.find(khsm_h);
    if (khsm_it == g_khsm_map.end()) return;
    if (!khsm_it->second.done) return;   // In flight - left (above).
    if (khsm_it->second.blob) khsm_it->second.blob->Release();
    g_khsm_map.erase(khsm_it);
}
}