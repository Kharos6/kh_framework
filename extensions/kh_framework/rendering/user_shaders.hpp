#pragma once

// The feature half (rendering_integration_features.hpp): the user shaders (material and effect .hlsl), the effect
// textures and the user constant lanes. Never include it on its own.
namespace RenderIntegration {
inline void kh_user_shader_cache_release();   // User.hlsl PS cache (defined with the loader).

struct KhUserShaderEntry {
    ID3D11PixelShader* ps = nullptr;
    // KH_USER_VS: a vertex-stage entry ("v|path") holds these three instead - the VS, the stream-output object made
    // from its bytecode, and the input layout made against it (kh_user_mat_vs).
    ID3D11VertexShader*   vs = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11InputLayout*    il = nullptr;
    bool failed = false;
    bool pending = false;   // A compile is queued or in flight.
    bool pyr = false;       // KH_FX_USER: an effect whose file reads the pre-filtered pyramid (kh_user_fx_ps).
    // KH_VMSEE_USER_SCAN: an effect whose file reads the depth or KH_VM_SEE's views (kh_user_src_reads_depth) -
    // true until its source is scanned.
    bool depth = true;
    // KH_WSEE: an effect whose file names KH_WSEE's helpers or views (kh_user_src_uses_wsee) - true until scanned.
    bool wsee = true;
    uint64_t hash = 0;
    uint64_t ask = 0;   // KH_USER_REQ_IDLE: g_user_pump_n when the getter last asked for a pending unit.
};

static std::unordered_map<std::string, KhUserShaderEntry> g_user_ps_cache;

// KH_USER_ASYNC: user shaders never compile on the render thread. compile_shader may block; the getters must never call
// it while a unit is in flight.
struct KhUserReq {
    std::string key;   // The g_user_ps_cache key this serves.
    std::string src;   // Full concatenated source, owned.
    std::string entry;   // PSMain / PSComposite / PSEffect / VSUserSo.
    std::string target = "ps_5_0";   // KH_USER_VS: "vs_5_0" for VSUserSo.
    std::vector<std::string> dn;
    std::vector<std::string> dv;
    uint64_t hash = 0;
    bool armed = false;   // Handed to a batch at least once.
    int retries = 0;
};
static std::vector<KhUserReq> g_user_req;
static size_t g_user_req_seen = 0;   // Queue size at the previous pump.
static int    g_user_req_settle = 0;   // Consecutive pumps the queue held still.
// The cap keeps a scene registering a material every frame from starving the queue: it arms anyway and the next batch
// collects the rest.
static constexpr int KH_USER_SETTLE_PUMPS = 2;
static constexpr int KH_USER_SETTLE_CAP   = 16;
static int g_user_req_waits = 0;   // Consecutive pumps declined; the cap.
static bool g_user_batch_own = false;
// An overflowing request is not recorded: the getter returns nullptr, the caller keeps its fallback, the next frame
// asks again.
static constexpr size_t KH_USER_REQ_MAX = 32;
// KH_USER_REQ_IDLE: the pumps run so far, and how many pumps a landed unit may wait unasked before a full queue
// takes its slot back (kh_user_shader_pump) - some ten seconds of flushes.
static uint64_t g_user_pump_n = 0;
static constexpr uint64_t KH_USER_REQ_IDLE_PUMPS = 600;

inline std::vector<D3D_SHADER_MACRO> kh_user_req_macros(const KhUserReq& khur) {
    std::vector<D3D_SHADER_MACRO> khur_m;
    khur_m.reserve(khur.dn.size() + 1);
    for (size_t khur_i = 0; khur_i < khur.dn.size(); ++khur_i) {
        D3D_SHADER_MACRO khur_e = { khur.dn[khur_i].c_str(), khur.dv[khur_i].c_str() };
        khur_m.push_back(khur_e);
    }
    D3D_SHADER_MACRO khur_t = { nullptr, nullptr };
    khur_m.push_back(khur_t);
    return khur_m;
}

// Called from the cache release (device reset and mission end): the pending requests go with the cache they would fill
// (a blob is device-independent bytecode - it is the request that is gone).
inline void kh_user_shader_purge_pending() {
    g_user_batch_own = false;
    g_user_req_seen = 0;   // The settle state goes with the queue.
    g_user_req_settle = 0;
    g_user_req_waits = 0;
    for (const KhUserReq& khur : g_user_req) {
        if (khur.armed) kh_shader_memo_discard(khur.hash);
    }
    g_user_req.clear();
}

inline void kh_user_shader_cache_release() {
    kh_user_shader_purge_pending();   // Before the map, never after.
    for (auto& khus_kv : g_user_ps_cache) {
        KH_SAFE_RELEASE(khus_kv.second.ps);
        KH_SAFE_RELEASE(khus_kv.second.il);   // KH_USER_VS.
        KH_SAFE_RELEASE(khus_kv.second.gs);
        KH_SAFE_RELEASE(khus_kv.second.vs);
    }
    g_user_ps_cache.clear();
}

// The pump: once per scene flush. The cache map and g_user_req need no lock: every access runs inside the engine's
// calls on the immediate context (which it drives from one thread at a time) or under a park. Never blocks, never
// nests.

inline void kh_user_shader_pump(ID3D11Device* dev) {
    ++g_user_pump_n;   // KH_USER_REQ_IDLE.
    if (!dev || g_user_req.empty()) return;
    for (KhUserReq& khur : g_user_req) {
        if (!khur.armed) continue;
        bool khur_present;
        {
            std::lock_guard<std::mutex> khur_l(g_khsm_mu);
            khur_present = g_khsm_map.find(khur.hash) != g_khsm_map.end();
        }
        if (khur_present) continue;

        if (khur.retries < 3) {
            khur.armed = false;
            khur.retries++;
        } else khur.retries = 4;   // Unservable - latched below.
    }

    for (size_t khup_i = g_user_req.size(); khup_i-- > 0; ) {
        if (g_user_req[khup_i].retries < 4) continue;
        auto khup_ce = g_user_ps_cache.find(g_user_req[khup_i].key);

        if (khup_ce != g_user_ps_cache.end()) {
            khup_ce->second.pending = false;
            khup_ce->second.failed = true;
        }

        report_error_once_safe("KH user shader: the compiled unit for '" + g_user_req[khup_i].key +
                        "' was lost from the memo four times - the request is abandoned");
        g_user_req.erase(g_user_req.begin() + static_cast<ptrdiff_t>(khup_i));
    }
    // KH_USER_REQ_IDLE: a full queue refuses every new user shader (kh_user_async_full), and a request leaves it only
    // when its getter takes the unit, so one asked for once (an object removed before its compile landed, a variant of
    // a route it took for a frame, a closed picture-in-picture's) would hold its slot to the mission's end. With the
    // queue full, a request whose unit has landed and whose getter has not asked for it in KH_USER_REQ_IDLE_PUMPS pumps
    // gives the slot back: its blob discarded and its cache entry erased, so an ask after that queues it afresh (the
    // disk cache serves the compile). Below the cap nothing changes.
    if (g_user_req.size() >= KH_USER_REQ_MAX) {
        for (size_t khup_i = g_user_req.size(); khup_i-- > 0; ) {
            const KhUserReq& khup_r = g_user_req[khup_i];
            if (!khup_r.armed) continue;
            const auto khup_ce = g_user_ps_cache.find(khup_r.key);
            if (khup_ce == g_user_ps_cache.end()) continue;
            if (g_user_pump_n - khup_ce->second.ask <= KH_USER_REQ_IDLE_PUMPS) continue;
            bool khup_landed;
            {
                std::lock_guard<std::mutex> khup_l(g_khsm_mu);
                const auto khup_m = g_khsm_map.find(khup_r.hash);
                khup_landed = khup_m != g_khsm_map.end() && khup_m->second.done;
            }
            if (!khup_landed) continue;   // In flight: left to land.
            kh_shader_memo_discard(khup_r.hash);
            g_user_ps_cache.erase(khup_ce);
            g_user_req.erase(g_user_req.begin() + static_cast<ptrdiff_t>(khup_i));
        }
    }
    bool khup_any = false;
    for (const KhUserReq& khur : g_user_req) if (!khur.armed) { khup_any = true; break; }
    if (!khup_any) { g_user_req_seen = g_user_req.size(); g_user_req_settle = 0; return; }
    // Wait for the queue to stop growing so one material's contexts batch together; a growing queue resets the count,
    // the cap arms regardless.
    if (g_user_req_waits < KH_USER_SETTLE_CAP) {
        const size_t khup_now = g_user_req.size();
        g_user_req_waits++;
        if (khup_now != g_user_req_seen) {
            g_user_req_seen = khup_now;
            g_user_req_settle = 0;   // Still arriving - start the count over.
            return;
        }
        if (g_user_req_settle < KH_USER_SETTLE_PUMPS) {
            g_user_req_settle++;
            return;
        }
        g_user_req_waits--;   // Do not spend the cap.
    }
    // The macro tables must outlive the prewarm call; prewarm deep-copies them, so this scope is enough.
    std::vector<std::vector<D3D_SHADER_MACRO>> khup_tabs;
    std::vector<KhShaderJob> khup_jobs;
    khup_tabs.reserve(g_user_req.size());
    khup_jobs.reserve(g_user_req.size());
    for (const KhUserReq& khur : g_user_req) {
        if (khur.armed) continue;
        khup_tabs.push_back(kh_user_req_macros(khur));
        KhShaderJob khup_j = { khur.src.c_str(), khur.entry.c_str(), khur.target.c_str(),
                               khup_tabs.back().data(), khur.hash };
        khup_jobs.push_back(khup_j);
    }
    if (khup_jobs.empty()) return;
    if (!kh_shader_mt_prewarm(khup_jobs.data(), khup_jobs.size(), false)) return;
    g_user_batch_own = true;   // Only we may reap it.
    g_user_req_settle = 0;   // The next arrival starts a fresh settle.
    g_user_req_waits = 0;
    g_user_req_seen = g_user_req.size();
    for (KhUserReq& khur : g_user_req) khur.armed = true;
}

// Retire the batch once every unit has landed, keeping the memo (the getters have not consumed their blobs yet).
inline void kh_user_shader_reap() {
    if (!g_user_batch_own) return;   // Ours, or nobody's business.
    if (!g_khsm_active) { g_user_batch_own = false; return; }
    bool khur_pending = false;
    {
        std::lock_guard<std::mutex> khur_l(g_khsm_mu);
        for (auto& khur_kv : g_khsm_map) {
            if (!khur_kv.second.done) { khur_pending = true; break; }
        }
    }
    if (khur_pending) return;
    kh_shader_mt_finish(true);   // KH_JOIN_TAIL: no compile left - the join waits out each return. Keeps the memo.
    g_user_batch_own = false;
}

// KH_USER_ERR_ONCE - the report of a user unit's failure. A compile that failed with fxc's message is reported as that
// message alone, under the file's path - kh_shader_compile_raw's text is the entry, " compile failed ", the HRESULT as
// 0x and eight hex digits, ": ", then fxc's - so the variants of one file that fail the same way (the material's flush
// and composite twins and its vertex stage: an error in the file is numbered in the file, KH_USER_LINE, and reads alike
// in each) make one text, which report_error_once_safe reports once. Any other failure (no fxc text, a Create refused,
// no blob) keeps its kind and its whole text.
inline std::string kh_user_err_report(const std::string& path, const char* khue_what, const std::string& khue_err) {
    static const char khue_k[] = " compile failed 0x";
    const size_t khue_p = khue_err.find(khue_k);
    if (khue_p != std::string::npos) {
        const size_t khue_t = khue_p + (sizeof(khue_k) - 1u) + 8u;   // Past the eight hex digits.
        if (khue_t + 2u < khue_err.size() && khue_err.compare(khue_t, 2u, ": ") == 0) {
            return "KH user shader '" + path + "': " + khue_err.substr(khue_t + 2u);
        }
    }
    return std::string("KH user ") + khue_what + " '" + path + "': " +
           (khue_err.empty() ? std::string("no blob") : khue_err);
}

// Never blocks - a unit in flight reads as not-ready. A failure latches, so a broken .hlsl reports once and is never
// re-queued.
inline ID3D11PixelShader* kh_user_async_take(ID3D11Device* dev, const std::string& khua_key,
                                             const std::string& path, const char* khua_what) {
    auto khua_it = g_user_ps_cache.find(khua_key);
    if (khua_it == g_user_ps_cache.end()) return nullptr;
    KhUserShaderEntry& khua_e = khua_it->second;
    if (!khua_e.pending) return khua_e.ps;
    khua_e.ask = g_user_pump_n;   // KH_USER_REQ_IDLE.
    ID3DBlob* khua_blob = nullptr;
    std::string khua_err;
    bool khua_gone = false;
    if (!kh_shader_memo_try_take(khua_e.hash, &khua_blob, khua_err, khua_gone)) {
        // Not ready, or the entry was cleared under us; the pump re-arms a lost request, bounded by its retry count.
        return nullptr;
    }
    khua_e.pending = false;
    if (khua_err.empty() && khua_blob) {
        const HRESULT khua_hr = dev->CreatePixelShader(khua_blob->GetBufferPointer(),
                                                       khua_blob->GetBufferSize(),
                                                       nullptr, &khua_e.ps);
        if (FAILED(khua_hr)) { khua_e.ps = nullptr; khua_err = "CreatePixelShader " + hr_str(khua_hr); }
    }
    if (khua_blob) khua_blob->Release();
    for (size_t khua_i = 0; khua_i < g_user_req.size(); ++khua_i) {
        if (g_user_req[khua_i].key == khua_key) {
            g_user_req.erase(g_user_req.begin() + static_cast<ptrdiff_t>(khua_i));
            break;
        }
    }
    if (!khua_e.ps) {
        khua_e.failed = true;
        report_error_once_safe(kh_user_err_report(path, khua_what, khua_err));   // KH_USER_ERR_ONCE.
    }
    return khua_e.ps;
}

// KH_USER_REQ_FULL: the queue is full - a getter asks this before it reads the file and builds the unit's source
// (cb.hlsl and a twin, some 300 KB), which kh_user_async_queue would refuse and discard; the next call asks again.
inline bool kh_user_async_full() { return g_user_req.size() >= KH_USER_REQ_MAX; }
// Record a request and mark the entry pending. False when the queue is full: nothing is cached and the next frame asks
// again.
inline bool kh_user_async_queue(std::string khuq_key, std::string khuq_full,
                                const char* khuq_entry,
                                const std::vector<std::string>& khuq_dn,
                                const std::vector<std::string>& khuq_dv,
                                const char* khuq_target = "ps_5_0") {   // KH_USER_VS: "vs_5_0".
    if (g_user_req.size() >= KH_USER_REQ_MAX) return false;
    KhUserReq khuq_r;
    khuq_r.key = khuq_key;
    khuq_r.src = std::move(khuq_full);
    khuq_r.entry = khuq_entry;
    khuq_r.dn = khuq_dn;
    khuq_r.dv = khuq_dv;
    khuq_r.target = khuq_target;
    const std::vector<D3D_SHADER_MACRO> khuq_m = kh_user_req_macros(khuq_r);
    khuq_r.hash = kh_shader_cache_hash(khuq_r.src.c_str(), khuq_r.entry.c_str(),
                                       khuq_r.target.c_str(), khuq_m.data());
    KhUserShaderEntry khuq_e;
    khuq_e.pending = true;
    khuq_e.hash = khuq_r.hash;
    khuq_e.ask = g_user_pump_n;   // KH_USER_REQ_IDLE.
    g_user_ps_cache.emplace(khuq_key, khuq_e);
    g_user_req.push_back(std::move(khuq_r));
    return true;
}

// KH_USER_SRC_MAX: a user .hlsl or .cube is read whole at its first use (an effect's or a material's first draw,
// a material's vertex scan on the game thread) and kept as text and its parse - 256 MB holds the largest LUT
// the parser takes (128^3 rows) many times over.
static constexpr uint64_t KH_USER_SRC_MAX = 256ull << 20;
inline bool kh_user_shader_source(const std::string& path, std::string& out, std::string& err) {
    std::vector<uint8_t> khus_b;
    std::string khus_e;

    if (!kh_asset_read(path, khus_b, KH_USER_SRC_MAX, &khus_e)) {   // PBO_PATH.
        if (khus_e == "empty file") err = "'" + path + "' is empty";
        else if (khus_e == "file too large") err = "'" + path + "' is 256 MB or more";   // KH_USER_SRC_MAX.
        else if (ModFolderSearcher::is_pbo_path(path)) err = "cannot read '" + path + "' (" + khus_e + ")";
        else err = "cannot open '" + path + "'";
        return false;
    }

    // KH_USER_SRC_MAX: the copy never throws - every caller latches a false (a failed entry); a throw out of here would
    // skip that latch, and the read would run again at the next draw.
    try {
        out.assign(reinterpret_cast<const char*>(khus_b.data()), khus_b.size());
    } catch (...) {
        out.clear();
        err = "'" + path + "': out of memory";
        return false;
    }
    // KH_USER_UTF16: UTF-16 text (Notepad's "Unicode": FF FE, or FE FF) or any NUL byte is refused with a report that
    // says so. The unit goes to D3DCompile and the cache key by strlen, so it would end at the file's first NUL
    // (UTF-16's second byte): a material's unit would lose the builtin text after the file too, and fxc's message (an
    // illegal character, or a missing KhUserShade) would not say why. A .cube is text as well.
    if ((out.size() >= 2 && ((out[0] == '\xFF' && out[1] == '\xFE') || (out[0] == '\xFE' && out[1] == '\xFF'))) ||
        out.find('\0') != std::string::npos) {
        err = "'" + path + "' is UTF-16 text or holds a NUL byte: save it as UTF-8 (or ANSI)";
        out.clear();
        return false;
    }
    // A UTF-8 byte-order mark (editors add one): a .hlsl is compiled after cb.hlsl, where fxc rejects it as an illegal
    // character, and a .cube's first keyword would not match. Dropped here, the one read of both.
    if (out.size() >= 3 && out[0] == '\xEF' && out[1] == '\xBB' && out[2] == '\xBF') out.erase(0, 3);
    if (out.empty()) { err = "'" + path + "' is empty"; return false; }
    return true;
}

// KH_USER_LINE: the #line directive that opens a user file's text in its unit, so fxc numbers its messages from
// the file's own first line under the file's name ("fog.hlsl(12,5): error X3004 ...") rather than from cb.hlsl's,
// whose length changes with every renderer update; kh_user_line_close names the builtin text after it. The file
// name alone, every character but a letter, a digit, '.', '-' or '_' made '_' (a quote or a backslash would end
// or escape the directive's string). Each starts a line of its own: the text before it need not end in one.
inline std::string kh_user_line_open(const std::string& path) {
    const size_t khul_s = path.find_last_of("/\\");
    std::string khul_d = "\n#line 1 \"";
    for (size_t khul_i = khul_s == std::string::npos ? 0u : khul_s + 1u; khul_i < path.size(); ++khul_i) {
        const char khul_c = path[khul_i];
        const bool khul_ok = (khul_c >= 'a' && khul_c <= 'z') || (khul_c >= 'A' && khul_c <= 'Z') ||
                             (khul_c >= '0' && khul_c <= '9') || khul_c == '.' || khul_c == '-' || khul_c == '_';
        khul_d += khul_ok ? khul_c : '_';
    }
    khul_d += "\"\n";
    return khul_d;
}
inline std::string kh_user_line_close(const char* khul_builtin) {
    return std::string("\n#line 1 \"") + khul_builtin + "\"\n";
}

// Does this source name khsv_id as a call or a definition: the identifier, whole, followed by '(' , outside every
// comment (a mention in a comment is neither). khsv_call false (KH_VMSEE_USER_SCAN): any whole use of it.
// khsv_prefix (KH_WSEE): any use of an identifier that begins with khsv_id (implies khsv_call false).
inline bool kh_user_src_calls(const std::string& khsv_s, const char* khsv_id, bool khsv_call = true,
                              bool khsv_prefix = false) {
    const size_t khsv_n = khsv_s.size(), khsv_m = strlen(khsv_id);
    auto khsv_word = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
    for (size_t i = 0; i < khsv_n;) {
        if (khsv_s[i] == '/' && i + 1u < khsv_n && khsv_s[i + 1u] == '/') {
            while (i < khsv_n && khsv_s[i] != '\n') ++i;
            continue;
        }
        if (khsv_s[i] == '/' && i + 1u < khsv_n && khsv_s[i + 1u] == '*') {
            i += 2u;
            while (i + 1u < khsv_n && !(khsv_s[i] == '*' && khsv_s[i + 1u] == '/')) ++i;
            i = i + 1u < khsv_n ? i + 2u : khsv_n;
            continue;
        }
        if (khsv_s.compare(i, khsv_m, khsv_id) == 0) {
            const bool khsv_head = i == 0u || !khsv_word(khsv_s[i - 1u]);
            size_t j = i + khsv_m;
            const bool khsv_more = j < khsv_n && khsv_word(khsv_s[j]);
            while (j < khsv_n && (khsv_s[j] == ' ' || khsv_s[j] == '\t' || khsv_s[j] == '\r' || khsv_s[j] == '\n')) ++j;
            if (khsv_head && khsv_prefix) return true;
            if (khsv_head && !khsv_more && (!khsv_call || (j < khsv_n && khsv_s[j] == '('))) return true;
            i += khsv_m;
            continue;
        }
        ++i;
    }
    return false;
}
// KH_VMSEE_USER_SCAN - does an effect source read the depth or KH_VM_SEE's views: it calls a helper of cb.hlsl's
// KH_FX_UNIT section that does, or names one of the views. A read hidden behind a macro reads as none - its pass then
// does not arm KH_VM_SEE by itself (kh_vmsee_arm; another depth reader still does). KH_WSEE: any identifier
// beginning KhWsee (its helpers - they read LoadDepthPS) or khWs (its views, t53 - t69) counts too. (KH_WSEE's own
// arming is kh_user_src_uses_wsee's.)
inline bool kh_user_src_reads_depth(const std::string& khsd_s) {
    static const char* const khsd_fn[] = { "LoadDepthPS", "LoadDepthRaw", "KhLinZ", "KhWorldPosFenced",
                                           "KhWorldPosRelFenced", "KhSunFlareVis", "KhVmSeeT", "KhVmSeeCovered",
                                           "KhVmSeeFade", "KhVmSeeArmed" };
    static const char* const khsd_view[] = { "depthTex", "khNzMark", "khVmSee", "khVmT", "khLinZ" };
    static const char* const khsd_pre[] = { "KhWsee", "khWs" };   // KH_WSEE: prefixes.
    for (const char* khsd_i : khsd_fn) if (kh_user_src_calls(khsd_s, khsd_i)) return true;
    for (const char* khsd_i : khsd_view) if (kh_user_src_calls(khsd_s, khsd_i, false)) return true;
    for (const char* khsd_i : khsd_pre) if (kh_user_src_calls(khsd_s, khsd_i, false, true)) return true;
    return false;
}
// KH_WSEE: does an effect source use KH_WSEE itself - an identifier beginning KhWsee or khWs, or KhSunFlareVis (which
// sees the sun through the layers by their transmittance). Only such a user effect (or a localized / banded one,
// whose builtin masks blend) has a use for the capture (kh_wsee_mode_of); one that reads the depth through
// LoadDepthPS / KhLinZ alone sees the front depth either way. Textual (kh_user_src_calls skips // and /* */
// comments, nothing else): such a name under #if 0, in a string or as a user's own name (khWsum) arms the pass too -
// the capture's cost for reads the shader never makes, never a change to the picture. A preprocessor-aware scan
// would have to evaluate #if conditions, and a lone #define line is a legitimate use.
inline bool kh_user_src_uses_wsee(const std::string& khsw_s) {
    return kh_user_src_calls(khsw_s, "KhWsee", false, true) || kh_user_src_calls(khsw_s, "khWs", false, true) ||
           kh_user_src_calls(khsw_s, "KhSunFlareVis");
}
// KH_USER_VS - does this source define a vertex stage: KhUserVertex followed by '('. (A false yes would only cost a
// compile that fails, reported once.)
inline bool kh_user_src_has_vertex(const std::string& khsv_s) { return kh_user_src_calls(khsv_s, "KhUserVertex"); }
// The parse's answer, per resolved path and session (a script that re-sends a material every frame must not re-read the
// file every frame). Game thread only; emptied by kh_session_globals_reset, so an edited file is read again by the next
// mission. An unreadable file says no here and reports through the pixel stage's own read.
static std::unordered_map<std::string, bool> g_user_vtx_scan;
inline bool kh_user_vertex_scan(const std::string& khvs_path) {
    const auto khvs_it = g_user_vtx_scan.find(khvs_path);
    if (khvs_it != g_user_vtx_scan.end()) return khvs_it->second;
    std::string khvs_src, khvs_err;
    const bool khvs_has = kh_user_shader_source(khvs_path, khvs_src, khvs_err) && kh_user_src_has_vertex(khvs_src);
    g_user_vtx_scan.emplace(khvs_path, khvs_has);
    return khvs_has;
}

// Custom post-processing PS for a resolved user path (nullptr = pending, absent or latched failure; the caller
// skips the pass). Keyed per depth MSAA count like the builtin effect shader: khuf_samples is the count of the depth
// the pass reads - 0 the live scene's, 1 a single-sample copy (the PIP's). KH_FX_USER: compiled as cb.hlsl + the
// file with MSAA_DEPTH and KH_FX_UNIT = 1 (cb.hlsl's KH_FX_UNIT section: the builtin effects' own declarations and
// helpers). khuf_pyr (optional, out): true when the pass reads the pre-filtered pyramid (its file calls KhGlowTap or
// KhGlowTapA), so the route builds one for it; false while nothing is ready.
inline ID3D11PixelShader* kh_user_fx_ps(ID3D11Device* dev, const std::string& path, bool* khuf_pyr = nullptr,
                                        UINT khuf_samples = 0u) {
    if (khuf_pyr) *khuf_pyr = false;
    if (!dev || path.empty()) return nullptr;
    const UINT khuf_n = khuf_samples ? khuf_samples : g_res.depth_sample_count;
    // The cold miss path std::moves the buffer into the map; the next call's assign re-grows once.
    static std::string khus_key;
    khus_key.assign("fx|");
    khus_key += std::to_string(khuf_n);
    khus_key += '|';
    khus_key += path;
    khus_key = kh_lower_copy(std::move(khus_key));
    auto khus_it = g_user_ps_cache.find(khus_key);
    if (khus_it != g_user_ps_cache.end()) {
        ID3D11PixelShader* khuf_ps = kh_user_async_take(dev, khus_key, path, "effect");
        if (khuf_pyr && khuf_ps) {
            const auto khuf_it = g_user_ps_cache.find(khus_key);
            *khuf_pyr = khuf_it != g_user_ps_cache.end() && khuf_it->second.pyr;
        }
        return khuf_ps;
    }
    // The compile is queued and this returns nullptr; the caller skips the pass this frame and takes it when it lands.
    if (kh_user_async_full()) return nullptr;   // KH_USER_REQ_FULL.
    std::string khua_src, khua_err;
    if (!kh_user_shader_source(path, khua_src, khua_err)) {
        KhUserShaderEntry khua_e;
        khua_e.failed = true;
        report_error_once_safe("KH user effect '" + path + "': " + khua_err);
        g_user_ps_cache.emplace(std::move(khus_key), khua_e);
        return nullptr;
    }
    const bool khuf_wants = kh_user_src_calls(khua_src, "KhGlowTap") ||   // KH_FX_USER.
                            kh_user_src_calls(khua_src, "KhGlowTapA");
    std::vector<std::string> khua_dn, khua_dv;
    khua_dn.push_back("MSAA_DEPTH");
    khua_dv.push_back(khuf_n > 1 ? "1" : "0");
    khua_dn.push_back("KH_FX_UNIT");
    khua_dv.push_back("1");
    const bool khuf_depth = kh_user_src_reads_depth(khua_src);   // KH_VMSEE_USER_SCAN.
    const bool khuf_wsee = kh_user_src_uses_wsee(khua_src);   // KH_WSEE.
    if (kh_user_async_queue(khus_key, kh_hlsl_src(KH_HLSL_CB) + kh_user_line_open(path) + khua_src,   // KH_USER_LINE.
                            "PSEffect", khua_dn, khua_dv)) {
        const auto khuf_it = g_user_ps_cache.find(khus_key);
        if (khuf_it != g_user_ps_cache.end()) {
            khuf_it->second.pyr = khuf_wants;
            khuf_it->second.depth = khuf_depth;
            khuf_it->second.wsee = khuf_wsee;
        }
    }
    return nullptr;

}
// KH_VMSEE_USER_SCAN: the scene chain may draw this user effect (its shader at the live depth's count stands, or its
// compile is in flight - it can land at the chain's own take this cycle) and its file reads the depth. The flush's
// thread, as kh_user_fx_ps.
inline bool kh_user_fx_reads_depth(const std::string& path) {
    if (path.empty()) return false;
    static std::string khrd_key;
    khrd_key.assign("fx|");
    khrd_key += std::to_string(g_res.depth_sample_count);
    khrd_key += '|';
    khrd_key += path;
    khrd_key = kh_lower_copy(std::move(khrd_key));
    const auto khrd_it = g_user_ps_cache.find(khrd_key);
    if (khrd_it == g_user_ps_cache.end()) return false;
    const KhUserShaderEntry& khrd_e = khrd_it->second;
    return (khrd_e.ps || khrd_e.pending) && !khrd_e.failed && khrd_e.depth;
}
// KH_WSEE: kh_user_fx_reads_depth's twin for kh_user_src_uses_wsee (the same cache entry, the same thread).
inline bool kh_user_fx_uses_wsee(const std::string& path) {
    if (path.empty()) return false;
    static std::string khuw_key;
    khuw_key.assign("fx|");
    khuw_key += std::to_string(g_res.depth_sample_count);
    khuw_key += '|';
    khuw_key += path;
    khuw_key = kh_lower_copy(std::move(khuw_key));
    const auto khuw_it = g_user_ps_cache.find(khuw_key);
    if (khuw_it == g_user_ps_cache.end()) return false;
    const KhUserShaderEntry& khuw_e = khuw_it->second;
    return (khuw_e.ps || khuw_e.pending) && !khuw_e.failed && khuw_e.wsee;
}

// KH_FX_TEX - an effect pass's own textures (a user effect's, or a builtin pass's that names some - KH_FX_TEX_ALL):
// each resolved through the texture cache as a material's maps are (kh_tex_resolve: the loader worker, the pages,
// the idle GC - this per-draw resolve keeps it from ageing while drawn), its layer into the draw's lanes (fx_tex:
// 1 + layer, 0 = absent or still loading) and its page into khft_srv[i] for t43 + i (cb.hlsl's khUserMap0..5, the
// KH_FX_UNIT section). Before the upload. KH_WSEE: fx_tex[1].z is its mode and [1].w its availability bits, not
// texture lanes - left as the caller set them (the scene chain's pass, before or after this fill; zero on every
// other route: every template is zero there).
inline void kh_fx_tex_lanes_zero(ConstantData& cbd) {
    memset(cbd.fx_tex[0], 0, sizeof(cbd.fx_tex[0]));
    cbd.fx_tex[1][0] = 0.0f;
    cbd.fx_tex[1][1] = 0.0f;
}
inline void kh_fx_tex_fill(ID3D11Device* dev, ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o,
                           ID3D11ShaderResourceView* khft_srv[KH_MAT_USER_MAPS]) {
    // A synchronous load (kh_tex_resolve's fallback, the loader thread down) can grow a page mid-set and release the
    // view an earlier slot took, so the set is taken again until one pass leaves the pages as they were (the
    // material table's rule, kh_mat_gpu_ensure). The loader's path places only at the pass heads: one pass.
    for (int khft_pass = 0; khft_pass < 4; ++khft_pass) {
        const uint32_t khft_gen0 = g_tex_page_gen;
        kh_fx_tex_lanes_zero(cbd);
        for (int khft_i = 0; khft_i < KH_MAT_USER_MAPS; ++khft_i) {
            khft_srv[khft_i] = nullptr;
            if (!o.fx_tex || !o.fx_tex->path[khft_i]) continue;
            const KhTexRef khft_r = kh_tex_resolve(dev, ctx, *o.fx_tex->path[khft_i],
                                                   ((o.fx_tex->srgb >> khft_i) & 1u) != 0u);
            if (!khft_r.srv || khft_r.layer < 0) continue;
            khft_srv[khft_i] = khft_r.srv;
            cbd.fx_tex[khft_i >> 2][khft_i & 3] = 1.0f + static_cast<float>(khft_r.layer);
        }
        if (g_tex_page_gen == khft_gen0) return;
    }
    // Still moving: none this draw (a view taken now could be the next one released).
    kh_fx_tex_lanes_zero(cbd);
    for (int khft_i = 0; khft_i < KH_MAT_USER_MAPS; ++khft_i) khft_srv[khft_i] = nullptr;
}
// Bound around the draw: the pages at t43 - t48 and the material sampler at s0 (anisotropic, wrap; null = the
// default state until the core set made it), both inside StateBackup's saved range. The pages come off after the
// draw (kh_fx_tex_unbind) - on the effect-mesh route at the next object without a set (khf_t43), on the scene chain
// and the UI lane at the pass's end (KhFxTexScope), or at the flush's state restore when none follows.
inline void kh_fx_tex_bind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* const khft_srv[KH_MAT_USER_MAPS]) {
    ctx->PSSetShaderResources(43, KH_MAT_USER_MAPS, khft_srv);
    kh_bc_forget();   // KH_BIND_FILTER: t43 - t48.
    ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
}
inline void kh_fx_tex_unbind(ID3D11DeviceContext* ctx) {
    ID3D11ShaderResourceView* const khft_null[KH_MAT_USER_MAPS] = {};
    ctx->PSSetShaderResources(43, KH_MAT_USER_MAPS, khft_null);
    kh_bc_forget();   // KH_BIND_FILTER.
}
// KH_FX_TEX_ALL: the passes that take their "textures" - every user effect (its lanes zeroed when it names none) and
// any BUILTIN effect pass whose object carries a set, so a builtin written to read them (cb.hlsl's KH_FX_UNIT
// section, KhUserTexLod / KhUserTexGrad) finds them bound on every route. A pass without a set pays this test only.
inline bool kh_fx_tex_on(const RenderObject& o) {
    return o.effect == KH_EFFECT_CUSTOM || (o.effect > 0 && o.fx_tex != nullptr);
}

// Custom material PS for a resolved user path and pipeline variant (khum_ctx: 0 flush/static twin, 1 composite
// guard twin, 2 composite arb twin - the builtin textured define sets plus KH_USER_MAT). nullptr = pending/absent/
// failed: the caller (kh_draw_textured) draws that submesh flat white with the white placeholder - builtin PBR only
// if the placeholder itself was not built.
inline ID3D11PixelShader* kh_user_mat_ps(ID3D11Device* dev, const std::string& path, int khum_ctx) {
    if (!dev || path.empty()) return nullptr;
    const UINT khum_samp = khum_ctx > 0 ? g_res.comp_depth_samples : 0;
    static std::string khum_key;
    khum_key.assign("m");
    khum_key += std::to_string(khum_ctx);
    khum_key += '|';
    khum_key += std::to_string(khum_samp);
    khum_key += '|';
    khum_key += path;
    khum_key = kh_lower_copy(std::move(khum_key));
    auto khum_it = g_user_ps_cache.find(khum_key);
    if (khum_it != g_user_ps_cache.end())
        return kh_user_async_take(dev, khum_key, path, "material shader");
    if (kh_user_async_full()) return nullptr;   // KH_USER_REQ_FULL: the placeholder, as while pending.
    KhUserShaderEntry khum_e;
    std::string khum_src, khum_err;

    if (kh_user_shader_source(path, khum_src, khum_err)) {
        const std::string khum_sc = std::to_string(g_res.comp_depth_samples);
        const D3D_SHADER_MACRO khum_def_flush[] = {
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        const D3D_SHADER_MACRO khum_def_comp[] = {
            { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
            { "SAMPLE_COUNT", khum_sc.c_str() },
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        const D3D_SHADER_MACRO khum_def_arb[] = {
            { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
            { "SAMPLE_COUNT", khum_sc.c_str() },
            { "KH_ARB_DEPTH", "1" },
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        std::string khum_full = kh_hlsl_src(KH_HLSL_CB) + kh_user_line_open(path) + khum_src;   // KH_USER_LINE.
        const char* khum_entry = "PSMain";
        const D3D_SHADER_MACRO* khum_defs = khum_def_flush;

        if (khum_ctx == 0) {
            khum_full += kh_user_line_close("static.hlsl");
            khum_full += kh_hlsl_src(KH_HLSL_STATIC).c_str();
        } else {
            khum_full += kh_user_line_close("composite.hlsl");
            khum_full += kh_hlsl_src(KH_HLSL_COMPOSITE).c_str();
            khum_entry = "PSComposite";
            khum_defs = khum_ctx == 2 ? khum_def_arb : khum_def_comp;
        }
        // Queued: the caller draws the flat-white placeholder on nullptr while the compile is pending, not a freeze.
        std::vector<std::string> khua_dn, khua_dv;
        for (const D3D_SHADER_MACRO* khua_d = khum_defs; khua_d && khua_d->Name; ++khua_d) {
            khua_dn.push_back(khua_d->Name);
            khua_dv.push_back(khua_d->Definition ? khua_d->Definition : "");
        }
        kh_user_async_queue(khum_key, std::move(khum_full), khum_entry, khua_dn, khua_dv);
        return nullptr;
    }

    // Unreadable source: the failure is cached so the path is not retried.
    khum_e.failed = true;
    report_error_once_safe("KH user material shader '" + path + "': " + khum_err);
    g_user_ps_cache.emplace(std::move(khum_key), khum_e);
    return nullptr;
}

// KH_USER_VS - the vertex stage of a resolved user path: VSUserSo (static.hlsl) over cb.hlsl + the user file +
// static.hlsl, the flush / static twin's defines (KH_RECEIVE_TEX, KH_TEXTURED, KH_USER_MAT) plus KH_USER_VS,
// compiled on the pool like every user unit. Returns 0 = pending (or the queue was full: asked again), 1 = ready (the
// three objects handed out), 2 = failed (latched, reported once). The stream-output object is made from the VS
// bytecode - MeshVertex's four lanes, 48 bytes, nothing rasterised - exactly as kh_skin_so_ensure makes GPU
// skinning's. Flush side or the render thread's step: the cache's one-thread-at-a-time contract.
inline int kh_user_mat_vs(ID3D11Device* dev, const std::string& path, ID3D11VertexShader*& khuv_vs,
                          ID3D11GeometryShader*& khuv_gs, ID3D11InputLayout*& khuv_il) {
    khuv_vs = nullptr; khuv_gs = nullptr; khuv_il = nullptr;
    if (!dev || path.empty()) return 2;
    static std::string khuv_key;
    khuv_key.assign("v|");
    khuv_key += path;
    khuv_key = kh_lower_copy(std::move(khuv_key));
    auto khuv_it = g_user_ps_cache.find(khuv_key);
    if (khuv_it == g_user_ps_cache.end()) {
        if (kh_user_async_full()) return 0;   // KH_USER_REQ_FULL: pending, asked again.
        std::string khuv_src, khuv_err;
        if (!kh_user_shader_source(path, khuv_src, khuv_err)) {
            KhUserShaderEntry khuv_f;
            khuv_f.failed = true;
            report_error_once_safe("KH user vertex stage '" + path + "': " + khuv_err);
            g_user_ps_cache.emplace(khuv_key, khuv_f);
            return 2;
        }
        std::vector<std::string> khuv_dn, khuv_dv;
        khuv_dn.push_back("KH_RECEIVE_TEX"); khuv_dv.push_back("1");
        khuv_dn.push_back("KH_TEXTURED");    khuv_dv.push_back("1");
        khuv_dn.push_back("KH_USER_MAT");    khuv_dv.push_back("1");
        khuv_dn.push_back("KH_USER_VS");     khuv_dv.push_back("1");
        kh_user_async_queue(khuv_key, kh_hlsl_src(KH_HLSL_CB) + kh_user_line_open(path) + khuv_src +   // KH_USER_LINE.
                                      kh_user_line_close("static.hlsl") + kh_hlsl_src(KH_HLSL_STATIC).c_str(),
                            "VSUserSo", khuv_dn, khuv_dv, "vs_5_0");
        return 0;
    }
    KhUserShaderEntry& khuv_e = khuv_it->second;
    if (khuv_e.pending) {
        khuv_e.ask = g_user_pump_n;   // KH_USER_REQ_IDLE.
        ID3DBlob* khuv_blob = nullptr;
        std::string khuv_err;
        bool khuv_gone = false;
        if (!kh_shader_memo_try_take(khuv_e.hash, &khuv_blob, khuv_err, khuv_gone)) return 0;   // Not ready; the pump re-arms a lost one.
        khuv_e.pending = false;
        if (khuv_err.empty() && khuv_blob) {
            static const D3D11_SO_DECLARATION_ENTRY khuv_so[] = {
                { 0, "POSITION", 0, 0, 3, 0 },
                { 0, "NORMAL",   0, 0, 3, 0 },
                { 0, "TEXCOORD", 0, 0, 2, 0 },
                { 0, "TANGENT",  0, 0, 4, 0 },
            };
            static const D3D11_INPUT_ELEMENT_DESC khuv_ild[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            const UINT khuv_stride = static_cast<UINT>(sizeof(MeshVertex));
            HRESULT khuv_hr = dev->CreateVertexShader(khuv_blob->GetBufferPointer(), khuv_blob->GetBufferSize(), nullptr, &khuv_e.vs);
            if (SUCCEEDED(khuv_hr)) {
                khuv_hr = dev->CreateGeometryShaderWithStreamOutput(khuv_blob->GetBufferPointer(), khuv_blob->GetBufferSize(),
                                                                    khuv_so, 4u, &khuv_stride, 1u, D3D11_SO_NO_RASTERIZED_STREAM,
                                                                    nullptr, &khuv_e.gs);
            }
            if (SUCCEEDED(khuv_hr)) {
                khuv_hr = dev->CreateInputLayout(khuv_ild, 4u, khuv_blob->GetBufferPointer(), khuv_blob->GetBufferSize(), &khuv_e.il);
            }
            if (FAILED(khuv_hr)) khuv_err = "stream-output objects: " + hr_str(khuv_hr);
        }
        if (khuv_blob) khuv_blob->Release();
        for (size_t khuv_i = 0; khuv_i < g_user_req.size(); ++khuv_i) {
            if (g_user_req[khuv_i].key == khuv_key) {
                g_user_req.erase(g_user_req.begin() + static_cast<ptrdiff_t>(khuv_i));
                break;
            }
        }
        if (!khuv_e.vs || !khuv_e.gs || !khuv_e.il) {
            KH_SAFE_RELEASE(khuv_e.il);
            KH_SAFE_RELEASE(khuv_e.gs);
            KH_SAFE_RELEASE(khuv_e.vs);
            khuv_e.failed = true;
            report_error_once_safe(kh_user_err_report(path, "vertex stage", khuv_err));   // KH_USER_ERR_ONCE.
        }
    }
    if (khuv_e.failed || !khuv_e.vs || !khuv_e.gs || !khuv_e.il) return 2;
    khuv_vs = khuv_e.vs; khuv_gs = khuv_e.gs; khuv_il = khuv_e.il;
    return 1;
}

// KH_USER_LANES - what a user .hlsl reads through cb.hlsl's accessors (KhUserTime and the rest; the contract is there).
// The object half: the session clock and this object's creation on it, so a shader animates on the object's age however
// it is drawn - a bucket instance reads its creation from its record (kh_objrec_fill), the clock from the CB.
inline void kh_fill_user_obj_cb(ConstantData& cbd, const RenderObject& o) {
    cbd.user_obj[0] = effect_time_seconds();
    cbd.user_obj[1] = static_cast<float>(o.fx_t0);
    cbd.user_obj[2] = 0.0f;
    cbd.user_obj[3] = 0.0f;
}
// The frame half: the pass camera. khuc_view is the pass's row-vector view (world -> view; absolute or camera-relative,
// only its rotation is read), or nullptr to keep the rotation rows the block already carries (a template's).
inline void kh_fill_user_cam_cb(ConstantData& cbd, const float* khuc_cam, const float (*khuc_view)[4]) {
    cbd.user_cam[0] = khuc_cam[0];
    cbd.user_cam[1] = khuc_cam[1];
    cbd.user_cam[2] = khuc_cam[2];
    cbd.user_cam[3] = 1.0f;
    if (!khuc_view) return;
    for (int khuc_a = 0; khuc_a < 3; ++khuc_a) {   // Row-vector view: the camera axes are its columns.
        cbd.user_cam_rot[khuc_a][0] = khuc_view[0][khuc_a];
        cbd.user_cam_rot[khuc_a][1] = khuc_view[1][khuc_a];
        cbd.user_cam_rot[khuc_a][2] = khuc_view[2][khuc_a];
        cbd.user_cam_rot[khuc_a][3] = 0.0f;
    }
}
}