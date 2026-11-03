#pragma once

// The feature half (rendering_integration_features.hpp): the user shaders (material and effect .hlsl), the effect
// textures and the user constant lanes. Never include it on its own.
namespace RenderIntegration {
inline void user_shader_cache_release();   // User.hlsl PS cache (defined with the loader).

struct UserShaderEntry {
    ID3D11PixelShader* ps = nullptr;
    // KH_USER_VS: a vertex-stage entry ("v|path") holds these three instead - the VS, the stream-output object made
    // from its bytecode, and the input layout made against it (user_mat_vs).
    ID3D11VertexShader*   vs = nullptr;
    ID3D11GeometryShader* gs = nullptr;
    ID3D11InputLayout*    il = nullptr;
    bool failed = false;
    bool pending = false;   // A compile is queued or in flight.
    bool pyr = false;       // FX_USER: an effect whose file reads the pre-filtered pyramid (user_fx_ps).
    // VMSEE_USER_SCAN: an effect whose file reads the depth or VM_SEE's views (user_src_reads_depth) -
    // true until its source is scanned.
    bool depth = true;
    // WSEE: an effect whose file names WSEE's helpers or views (user_src_uses_wsee) - true until scanned.
    bool wsee = true;
    uint64_t hash = 0;
    uint64_t ask = 0;   // USER_REQ_IDLE: g_user_shaders.pump_n when the getter last asked for a pending unit.
};

// USER_ASYNC: user shaders never compile on the render thread. compile_shader may block; the getters must never call
// it while a unit is in flight.
struct UserReq {
    std::string key;   // The g_user_shaders.ps_cache key this serves.
    std::string src;   // Full concatenated source, owned.
    std::string entry;   // PSMain / PSComposite / PSEffect / VSUserSo.
    std::string target = "ps_5_0";   // KH_USER_VS: "vs_5_0" for VSUserSo.
    std::vector<std::string> dn;
    std::vector<std::string> dv;
    uint64_t hash = 0;
    bool armed = false;   // Handed to a batch at least once.
    int retries = 0;
};
// User .hlsl shaders: the compiled cache, the requests and their settle / wait bookkeeping, the batch ownership, the
// pump count, the vertex-stage scan.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: ps_cache, req, batch_own, vtx_scan.
// Session reset: session_globals_reset resets every member.
struct UserShaderState {
    std::unordered_map<std::string, UserShaderEntry> ps_cache;
    std::vector<UserReq> req;
    size_t req_seen = 0;   // Queue size at the previous pump.
    int    req_settle = 0;   // Consecutive pumps the queue held still.
    int req_waits = 0;   // Consecutive pumps declined; the cap.
    bool batch_own = false;
    // USER_REQ_IDLE: the pumps run so far (the limit is USER_REQ_IDLE_PUMPS).
    uint64_t pump_n = 0;
    // The parse's answer, per resolved path and session (a script that re-sends a material every frame must not re-read
    // the file every frame). Game thread only; emptied by session_globals_reset, so an edited file is read again by
    // the next mission. An unreadable file says no here and reports through the pixel stage's own read.
    std::unordered_map<std::string, bool> vtx_scan;
};
static UserShaderState g_user_shaders;

// The cap keeps a scene registering a material every frame from starving the queue: it arms anyway and the next batch
// collects the rest.
static constexpr int USER_SETTLE_PUMPS = 2;
static constexpr int USER_SETTLE_CAP   = 16;
// An overflowing request is not recorded: the getter returns nullptr, the caller keeps its fallback, the next frame
// asks again.
static constexpr size_t USER_REQ_MAX = 32;
// USER_REQ_IDLE: how many pumps (g_user_shaders.pump_n) a landed unit may wait unasked before a full queue takes
// its slot back (user_shader_pump) - some ten seconds of flushes.
static constexpr uint64_t USER_REQ_IDLE_PUMPS = 600;

inline std::vector<D3D_SHADER_MACRO> user_req_macros(const UserReq& ur) {
    std::vector<D3D_SHADER_MACRO> m;
    m.reserve(ur.dn.size() + 1);
    for (size_t i = 0; i < ur.dn.size(); ++i) {
        D3D_SHADER_MACRO e = { ur.dn[i].c_str(), ur.dv[i].c_str() };
        m.push_back(e);
    }
    D3D_SHADER_MACRO t = { nullptr, nullptr };
    m.push_back(t);
    return m;
}

// Called from the cache release (device reset and mission end): the pending requests go with the cache they would fill
// (a blob is device-independent bytecode - it is the request that is gone).
inline void user_shader_purge_pending() {
    g_user_shaders.batch_own = false;
    g_user_shaders.req_seen = 0;   // The settle state goes with the queue.
    g_user_shaders.req_settle = 0;
    g_user_shaders.req_waits = 0;
    for (const UserReq& ur : g_user_shaders.req) {
        if (ur.armed) shader_memo_discard(ur.hash);
    }
    g_user_shaders.req.clear();
}

inline void user_shader_cache_release() {
    user_shader_purge_pending();   // Before the map, never after.
    for (auto& kv : g_user_shaders.ps_cache) {
        KH_SAFE_RELEASE(kv.second.ps);
        KH_SAFE_RELEASE(kv.second.il);   // KH_USER_VS.
        KH_SAFE_RELEASE(kv.second.gs);
        KH_SAFE_RELEASE(kv.second.vs);
    }
    g_user_shaders.ps_cache.clear();
}

// The pump: once per scene flush. The cache map and g_user_shaders.req need no lock: every access runs inside the
// engine's calls on the immediate context (which it drives from one thread at a time) or under a park. Never blocks,
// never nests.

inline void user_shader_pump(ID3D11Device* dev) {
    ++g_user_shaders.pump_n;   // USER_REQ_IDLE.
    if (!dev || g_user_shaders.req.empty()) return;
    for (UserReq& ur : g_user_shaders.req) {
        if (!ur.armed) continue;
        bool present;
        {
            std::lock_guard<std::mutex> l(g_shpool_mu);
            present = g_shpool_map.find(ur.hash) != g_shpool_map.end();
        }
        if (present) continue;

        if (ur.retries < 3) {
            ur.armed = false;
            ur.retries++;
        } else ur.retries = 4;   // Unservable - latched below.
    }

    for (size_t i = g_user_shaders.req.size(); i-- > 0; ) {
        if (g_user_shaders.req[i].retries < 4) continue;
        auto ce = g_user_shaders.ps_cache.find(g_user_shaders.req[i].key);

        if (ce != g_user_shaders.ps_cache.end()) {
            ce->second.pending = false;
            ce->second.failed = true;
        }

        report_error_once_safe("KH RenderIntegration: user shader: the compiled unit for '" +
                               g_user_shaders.req[i].key +
                        "' was lost from the memo four times - the request is abandoned");
        g_user_shaders.req.erase(g_user_shaders.req.begin() + static_cast<ptrdiff_t>(i));
    }
    // USER_REQ_IDLE: a full queue refuses every new user shader (user_async_full), and a request leaves it only
    // when its getter takes the unit, so one asked for once (an object removed before its compile landed, a variant of
    // a route it took for a frame, a closed picture-in-picture's) would hold its slot to the mission's end. With the
    // queue full, a request whose unit has landed and whose getter has not asked for it in USER_REQ_IDLE_PUMPS pumps
    // gives the slot back: its blob discarded and its cache entry erased, so an ask after that queues it afresh (the
    // disk cache serves the compile). Below the cap nothing changes.
    if (g_user_shaders.req.size() >= USER_REQ_MAX) {
        for (size_t i = g_user_shaders.req.size(); i-- > 0; ) {
            const UserReq& r = g_user_shaders.req[i];
            if (!r.armed) continue;
            const auto ce = g_user_shaders.ps_cache.find(r.key);
            if (ce == g_user_shaders.ps_cache.end()) continue;
            if (g_user_shaders.pump_n - ce->second.ask <= USER_REQ_IDLE_PUMPS) continue;
            bool landed;
            {
                std::lock_guard<std::mutex> l(g_shpool_mu);
                const auto m = g_shpool_map.find(r.hash);
                landed = m != g_shpool_map.end() && m->second.done;
            }
            if (!landed) continue;   // In flight: left to land.
            shader_memo_discard(r.hash);
            g_user_shaders.ps_cache.erase(ce);
            g_user_shaders.req.erase(g_user_shaders.req.begin() + static_cast<ptrdiff_t>(i));
        }
    }
    bool any = false;
    for (const UserReq& ur : g_user_shaders.req) if (!ur.armed) { any = true; break; }
    if (!any) { g_user_shaders.req_seen = g_user_shaders.req.size(); g_user_shaders.req_settle = 0; return; }
    // Wait for the queue to stop growing so one material's contexts batch together; a growing queue resets the count,
    // the cap arms regardless.
    if (g_user_shaders.req_waits < USER_SETTLE_CAP) {
        const size_t now = g_user_shaders.req.size();
        g_user_shaders.req_waits++;
        if (now != g_user_shaders.req_seen) {
            g_user_shaders.req_seen = now;
            g_user_shaders.req_settle = 0;   // Still arriving - start the count over.
            return;
        }
        if (g_user_shaders.req_settle < USER_SETTLE_PUMPS) {
            g_user_shaders.req_settle++;
            return;
        }
        g_user_shaders.req_waits--;   // Do not spend the cap.
    }
    // The macro tables must outlive the prewarm call; prewarm deep-copies them, so this scope is enough.
    std::vector<std::vector<D3D_SHADER_MACRO>> tabs;
    std::vector<ShaderJob> jobs;
    tabs.reserve(g_user_shaders.req.size());
    jobs.reserve(g_user_shaders.req.size());
    for (const UserReq& ur : g_user_shaders.req) {
        if (ur.armed) continue;
        tabs.push_back(user_req_macros(ur));
        ShaderJob j = { ur.src.c_str(), ur.entry.c_str(), ur.target.c_str(),
                        tabs.back().data(), ur.hash };
        jobs.push_back(j);
    }
    if (jobs.empty()) return;
    if (!shader_mt_prewarm(jobs.data(), jobs.size(), false)) return;
    g_user_shaders.batch_own = true;   // Only we may reap it.
    g_user_shaders.req_settle = 0;   // The next arrival starts a fresh settle.
    g_user_shaders.req_waits = 0;
    g_user_shaders.req_seen = g_user_shaders.req.size();
    for (UserReq& ur : g_user_shaders.req) ur.armed = true;
}

// Retire the batch once every unit has landed, keeping the memo (the getters have not consumed their blobs yet).
inline void user_shader_reap() {
    if (!g_user_shaders.batch_own) return;   // Ours, or nobody's business.
    if (!g_shpool_active) { g_user_shaders.batch_own = false; return; }
    bool pending = false;
    {
        std::lock_guard<std::mutex> l(g_shpool_mu);
        for (auto& kv : g_shpool_map) {
            if (!kv.second.done) { pending = true; break; }
        }
    }
    if (pending) return;
    shader_mt_finish(true);   // JOIN_TAIL: no compile left - the join waits out each return. Keeps the memo.
    g_user_shaders.batch_own = false;
}

// USER_ERR_ONCE - the report of a user unit's failure. A compile that failed with fxc's message is reported as that
// message alone, under the file's path - shader_compile_raw's text is the entry, " compile failed ", the HRESULT as
// 0x and eight hex digits, ": ", then fxc's - so the variants of one file that fail the same way (the material's flush
// and composite twins and its vertex stage: an error in the file is numbered in the file, USER_LINE, and reads alike
// in each) make one text, which report_error_once_safe reports once. Any other failure (no fxc text, a Create refused,
// no blob) keeps its kind and its whole text.
inline std::string user_err_report(const std::string& path, const char* what, const std::string& err) {
    static const char k[] = " compile failed 0x";
    const size_t p = err.find(k);
    if (p != std::string::npos) {
        const size_t t = p + (sizeof(k) - 1u) + 8u;   // Past the eight hex digits.
        if (t + 2u < err.size() && err.compare(t, 2u, ": ") == 0) {
            return "KH RenderIntegration: user shader '" + path + "': " + err.substr(t + 2u);
        }
    }
    return std::string("KH RenderIntegration: user ") + what + " '" + path + "': " +
           (err.empty() ? std::string("no blob") : err);
}

// Never blocks - a unit in flight reads as not-ready. A failure latches, so a broken .hlsl reports once and is never
// re-queued.
inline ID3D11PixelShader* user_async_take(ID3D11Device* dev, const std::string& key,
                                          const std::string& path, const char* what) {
    auto it = g_user_shaders.ps_cache.find(key);
    if (it == g_user_shaders.ps_cache.end()) return nullptr;
    UserShaderEntry& e = it->second;
    if (!e.pending) return e.ps;
    e.ask = g_user_shaders.pump_n;   // USER_REQ_IDLE.
    ID3DBlob* blob = nullptr;
    std::string err;
    bool gone = false;
    if (!shader_memo_try_take(e.hash, &blob, err, gone)) {
        // Not ready, or the entry was cleared under us; the pump re-arms a lost request, bounded by its retry count.
        return nullptr;
    }
    e.pending = false;
    if (err.empty() && blob) {
        const HRESULT hr = dev->CreatePixelShader(blob->GetBufferPointer(),
                                                  blob->GetBufferSize(),
                                                  nullptr, &e.ps);
        if (FAILED(hr)) { e.ps = nullptr; err = "CreatePixelShader " + hr_str(hr); }
    }
    if (blob) blob->Release();
    for (size_t i = 0; i < g_user_shaders.req.size(); ++i) {
        if (g_user_shaders.req[i].key == key) {
            g_user_shaders.req.erase(g_user_shaders.req.begin() + static_cast<ptrdiff_t>(i));
            break;
        }
    }
    if (!e.ps) {
        e.failed = true;
        report_error_once_safe(user_err_report(path, what, err));   // USER_ERR_ONCE.
    }
    return e.ps;
}

// USER_REQ_FULL: the queue is full - a getter asks this before it reads the file and builds the unit's source
// (cb.hlsl and a twin, some 300 KB), which user_async_queue would refuse and discard; the next call asks again.
inline bool user_async_full() { return g_user_shaders.req.size() >= USER_REQ_MAX; }
// Record a request and mark the entry pending. False when the queue is full: nothing is cached and the next frame asks
// again.
inline bool user_async_queue(std::string key, std::string full,
                             const char* entry,
                             const std::vector<std::string>& dn,
                             const std::vector<std::string>& dv,
                             const char* target = "ps_5_0") {   // KH_USER_VS: "vs_5_0".
    if (g_user_shaders.req.size() >= USER_REQ_MAX) return false;
    UserReq r;
    r.key = key;
    r.src = std::move(full);
    r.entry = entry;
    r.dn = dn;
    r.dv = dv;
    r.target = target;
    const std::vector<D3D_SHADER_MACRO> m = user_req_macros(r);
    r.hash = shader_cache_hash(r.src.c_str(), r.entry.c_str(),
                               r.target.c_str(), m.data());
    UserShaderEntry e;
    e.pending = true;
    e.hash = r.hash;
    e.ask = g_user_shaders.pump_n;   // USER_REQ_IDLE.
    g_user_shaders.ps_cache.emplace(key, e);
    g_user_shaders.req.push_back(std::move(r));
    return true;
}

// USER_SRC_MAX: a user .hlsl or .cube is read whole at its first use (an effect's or a material's first draw,
// a material's vertex scan on the game thread) and kept as text and its parse - 256 MB holds the largest LUT
// the parser takes (128^3 rows) many times over.
static constexpr uint64_t USER_SRC_MAX = 256ull << 20;
inline bool user_shader_source(const std::string& path, std::string& out, std::string& err) {
    std::vector<uint8_t> b;
    std::string e;

    if (!asset_read(path, b, USER_SRC_MAX, &e)) {   // PBO_PATH.
        if (e == "empty file") err = "'" + path + "' is empty";
        else if (e == "file too large") err = "'" + path + "' is 256 MB or more";   // USER_SRC_MAX.
        else if (ModFolderSearcher::is_pbo_path(path)) err = "cannot read '" + path + "' (" + e + ")";
        else err = "cannot open '" + path + "'";
        return false;
    }

    // USER_SRC_MAX: the copy never throws - every caller latches a false (a failed entry); a throw out of here would
    // skip that latch, and the read would run again at the next draw.
    try {
        out.assign(reinterpret_cast<const char*>(b.data()), b.size());
    } catch (...) {
        out.clear();
        err = "'" + path + "': out of memory";
        return false;
    }
    // USER_UTF16: UTF-16 text (Notepad's "Unicode": FF FE, or FE FF) or any NUL byte is refused with a report that
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

// USER_LINE: the #line directive that opens a user file's text in its unit, so fxc numbers its messages from
// the file's own first line under the file's name ("fog.hlsl(12,5): error X3004 ...") rather than from cb.hlsl's,
// whose length changes with every renderer update; user_line_close names the builtin text after it. The file
// name alone, every character but a letter, a digit, '.', '-' or '_' made '_' (a quote or a backslash would end
// or escape the directive's string). Each starts a line of its own: the text before it need not end in one.
inline std::string user_line_open(const std::string& path) {
    const size_t s = path.find_last_of("/\\");
    std::string d = "\n#line 1 \"";
    for (size_t i = s == std::string::npos ? 0u : s + 1u; i < path.size(); ++i) {
        const char c = path[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_';
        d += ok ? c : '_';
    }
    d += "\"\n";
    return d;
}
inline std::string user_line_close(const char* builtin) {
    return std::string("\n#line 1 \"") + builtin + "\"\n";
}

// Does this source name id as a call or a definition: the identifier, whole, followed by '(' , outside every
// comment (a mention in a comment is neither). call false (VMSEE_USER_SCAN): any whole use of it.
// prefix (WSEE): any use of an identifier that begins with id (implies call false).
inline bool user_src_calls(const std::string& s, const char* id, bool call = true,
                           bool prefix = false) {
    const size_t n = s.size(), m = strlen(id);
    auto word = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
    for (size_t i = 0; i < n;) {
        if (s[i] == '/' && i + 1u < n && s[i + 1u] == '/') {
            while (i < n && s[i] != '\n') ++i;
            continue;
        }
        if (s[i] == '/' && i + 1u < n && s[i + 1u] == '*') {
            i += 2u;
            while (i + 1u < n && !(s[i] == '*' && s[i + 1u] == '/')) ++i;
            i = i + 1u < n ? i + 2u : n;
            continue;
        }
        if (s.compare(i, m, id) == 0) {
            const bool head = i == 0u || !word(s[i - 1u]);
            size_t j = i + m;
            const bool more = j < n && word(s[j]);
            while (j < n && (s[j] == ' ' || s[j] == '\t' || s[j] == '\r' || s[j] == '\n')) ++j;
            if (head && prefix) return true;
            if (head && !more && (!call || (j < n && s[j] == '('))) return true;
            i += m;
            continue;
        }
        ++i;
    }
    return false;
}
// VMSEE_USER_SCAN - does an effect source read the depth or VM_SEE's views: it calls a helper of cb.hlsl's
// KH_FX_UNIT section that does, or names one of the views. A read hidden behind a macro reads as none - its pass then
// does not arm VM_SEE by itself (vmsee_arm; another depth reader still does). WSEE: any identifier
// beginning KhWsee (its helpers - they read LoadDepthPS) or khWs (its views, t53 - t69) counts too. (WSEE's own
// arming is user_src_uses_wsee's.)
inline bool user_src_reads_depth(const std::string& s) {
    static const char* const fn[] = { "LoadDepthPS", "LoadDepthRaw", "KhLinZ", "KhWorldPosFenced",
                                           "KhWorldPosRelFenced", "KhSunFlareVis", "KhVmSeeT", "KhVmSeeCovered",
                                           "KhVmSeeFade", "KhVmSeeArmed" };
    static const char* const view[] = { "depthTex", "khNzMark", "khVmSee", "khVmT", "khLinZ" };
    static const char* const pre[] = { "KhWsee", "khWs" };   // WSEE: prefixes.
    for (const char* i : fn) if (user_src_calls(s, i)) return true;
    for (const char* i : view) if (user_src_calls(s, i, false)) return true;
    for (const char* i : pre) if (user_src_calls(s, i, false, true)) return true;
    return false;
}
// WSEE: does an effect source use WSEE itself - an identifier beginning KhWsee or khWs, or KhSunFlareVis (which
// sees the sun through the layers by their transmittance). Only such a user effect (or a localized / banded one,
// whose builtin masks blend) has a use for the capture (wsee_mode_of); one that reads the depth through
// LoadDepthPS / KhLinZ alone sees the front depth either way. Textual (user_src_calls skips // and /* */
// comments, nothing else): such a name under #if 0, in a string or as a user's own name (khWsum) arms the pass too -
// the capture's cost for reads the shader never makes, never a change to the picture. A preprocessor-aware scan
// would have to evaluate #if conditions, and a lone #define line is a legitimate use.
inline bool user_src_uses_wsee(const std::string& s) {
    return user_src_calls(s, "KhWsee", false, true) || user_src_calls(s, "khWs", false, true) ||
           user_src_calls(s, "KhSunFlareVis");
}
// KH_USER_VS - does this source define a vertex stage: KhUserVertex followed by '('. (A false yes would only cost a
// compile that fails, reported once.)
inline bool user_src_has_vertex(const std::string& s) { return user_src_calls(s, "KhUserVertex"); }
inline bool user_vertex_scan(const std::string& path) {
    const auto it = g_user_shaders.vtx_scan.find(path);
    if (it != g_user_shaders.vtx_scan.end()) return it->second;
    std::string src, err;
    const bool has = user_shader_source(path, src, err) && user_src_has_vertex(src);
    g_user_shaders.vtx_scan.emplace(path, has);
    return has;
}

// Custom post-processing PS for a resolved user path (nullptr = pending, absent or latched failure; the caller
// skips the pass). Keyed per depth MSAA count like the builtin effect shader: samples is the count of the depth
// the pass reads - 0 the live scene's, 1 a single-sample copy (the PIP's). FX_USER: compiled as cb.hlsl + the
// file with MSAA_DEPTH and KH_FX_UNIT = 1 (cb.hlsl's KH_FX_UNIT section: the builtin effects' own declarations and
// helpers). pyr (optional, out): true when the pass reads the pre-filtered pyramid (its file calls KhGlowTap or
// KhGlowTapA), so the route builds one for it; false while nothing is ready.
inline ID3D11PixelShader* user_fx_ps(ID3D11Device* dev, const std::string& path, bool* pyr = nullptr,
                                     UINT samples = 0u) {
    if (pyr) *pyr = false;
    if (!dev || path.empty()) return nullptr;
    const UINT n = samples ? samples : g_res.depth_sample_count;
    // The cold miss path std::moves the buffer into the map; the next call's assign re-grows once.
    static std::string key;
    key.assign("fx|");
    key += std::to_string(n);
    key += '|';
    key += path;
    key = kh_lower_copy(std::move(key));
    auto us_it = g_user_shaders.ps_cache.find(key);
    if (us_it != g_user_shaders.ps_cache.end()) {
        ID3D11PixelShader* ps = user_async_take(dev, key, path, "effect");
        if (pyr && ps) {
            const auto uf_it = g_user_shaders.ps_cache.find(key);
            *pyr = uf_it != g_user_shaders.ps_cache.end() && uf_it->second.pyr;
        }
        return ps;
    }
    // The compile is queued and this returns nullptr; the caller skips the pass this frame and takes it when it lands.
    if (user_async_full()) return nullptr;   // USER_REQ_FULL.
    std::string src, err;
    if (!user_shader_source(path, src, err)) {
        UserShaderEntry e;
        e.failed = true;
        report_error_once_safe("KH RenderIntegration: user effect '" + path + "': " + err);
        g_user_shaders.ps_cache.emplace(std::move(key), e);
        return nullptr;
    }
    const bool wants = user_src_calls(src, "KhGlowTap") ||   // FX_USER.
                       user_src_calls(src, "KhGlowTapA");
    std::vector<std::string> dn, dv;
    dn.push_back("MSAA_DEPTH");
    dv.push_back(n > 1 ? "1" : "0");
    dn.push_back("KH_FX_UNIT");
    dv.push_back("1");
    const bool depth = user_src_reads_depth(src);   // VMSEE_USER_SCAN.
    const bool wsee = user_src_uses_wsee(src);   // WSEE.
    if (user_async_queue(key, hlsl_src(HLSL_CB) + user_line_open(path) + src,   // USER_LINE.
                         "PSEffect", dn, dv)) {
        const auto uf_it = g_user_shaders.ps_cache.find(key);
        if (uf_it != g_user_shaders.ps_cache.end()) {
            uf_it->second.pyr = wants;
            uf_it->second.depth = depth;
            uf_it->second.wsee = wsee;
        }
    }
    return nullptr;

}
// VMSEE_USER_SCAN: the scene chain may draw this user effect (its shader at the live depth's count stands, or its
// compile is in flight - it can land at the chain's own take this cycle) and its file reads the depth. The flush's
// thread, as user_fx_ps.
inline bool user_fx_reads_depth(const std::string& path) {
    if (path.empty()) return false;
    static std::string key;
    key.assign("fx|");
    key += std::to_string(g_res.depth_sample_count);
    key += '|';
    key += path;
    key = kh_lower_copy(std::move(key));
    const auto it = g_user_shaders.ps_cache.find(key);
    if (it == g_user_shaders.ps_cache.end()) return false;
    const UserShaderEntry& e = it->second;
    return (e.ps || e.pending) && !e.failed && e.depth;
}
// WSEE: user_fx_reads_depth's twin for user_src_uses_wsee (the same cache entry, the same thread).
inline bool user_fx_uses_wsee(const std::string& path) {
    if (path.empty()) return false;
    static std::string key;
    key.assign("fx|");
    key += std::to_string(g_res.depth_sample_count);
    key += '|';
    key += path;
    key = kh_lower_copy(std::move(key));
    const auto it = g_user_shaders.ps_cache.find(key);
    if (it == g_user_shaders.ps_cache.end()) return false;
    const UserShaderEntry& e = it->second;
    return (e.ps || e.pending) && !e.failed && e.wsee;
}

// FX_TEX - an effect pass's own textures (a user effect's, or a builtin pass's that names some - FX_TEX_ALL):
// each resolved through the texture cache as a material's maps are (tex_resolve: the loader worker, the pages,
// the idle GC - this per-draw resolve keeps it from ageing while drawn), its layer into the draw's lanes (fx_tex:
// 1 + layer, 0 = absent or still loading) and its page into srv[i] for t43 + i (cb.hlsl's khUserMap0..5, the
// KH_FX_UNIT section). Before the upload. WSEE: fx_tex[1].z is its mode and [1].w its availability bits, not
// texture lanes - left as the caller set them (the scene chain's pass, before or after this fill; zero on every
// other route: every template is zero there).
inline void fx_tex_lanes_zero(ConstantData& cbd) {
    memset(cbd.fx_tex[0], 0, sizeof(cbd.fx_tex[0]));
    cbd.fx_tex[1][0] = 0.0f;
    cbd.fx_tex[1][1] = 0.0f;
}
inline void fx_tex_fill(ID3D11Device* dev, ID3D11DeviceContext* ctx, ConstantData& cbd, const RenderObject& o,
                        ID3D11ShaderResourceView* srv[MAT_USER_MAPS]) {
    // A synchronous load (tex_resolve's fallback, the loader thread down) can grow a page mid-set and release the
    // view an earlier slot took, so the set is taken again until one pass leaves the pages as they were (the
    // material table's rule, mat_gpu_ensure). The loader's path places only at the pass heads: one pass.
    for (int pass = 0; pass < 4; ++pass) {
        const uint32_t gen0 = g_tex_page_gen;
        fx_tex_lanes_zero(cbd);
        for (int i = 0; i < MAT_USER_MAPS; ++i) {
            srv[i] = nullptr;
            if (!o.fx_tex || !o.fx_tex->path[i]) continue;
            const TexRef r = tex_resolve(dev, ctx, *o.fx_tex->path[i],
                                         ((o.fx_tex->srgb >> i) & 1u) != 0u);
            if (!r.srv || r.layer < 0) continue;
            srv[i] = r.srv;
            cbd.fx_tex[i >> 2][i & 3] = 1.0f + static_cast<float>(r.layer);
        }
        if (g_tex_page_gen == gen0) return;
    }
    // Still moving: none this draw (a view taken now could be the next one released).
    fx_tex_lanes_zero(cbd);
    for (int i = 0; i < MAT_USER_MAPS; ++i) srv[i] = nullptr;
}
// Bound around the draw: the pages at t43 - t48 and the material sampler at s0 (anisotropic, wrap; null = the
// default state until the core set made it), both inside StateBackup's saved range. The pages come off after the
// draw (fx_tex_unbind) - on the effect-mesh route at the next object without a set (t43), on the scene chain
// and the UI lane at the pass's end (FxTexScope), or at the flush's state restore when none follows.
inline void fx_tex_bind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* const srv[MAT_USER_MAPS]) {
    ctx->PSSetShaderResources(43, MAT_USER_MAPS, srv);
    bc_forget();   // BIND_FILTER: t43 - t48.
    ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
}
inline void fx_tex_unbind(ID3D11DeviceContext* ctx) {
    ID3D11ShaderResourceView* const null[MAT_USER_MAPS] = {};
    ctx->PSSetShaderResources(43, MAT_USER_MAPS, null);
    bc_forget();   // BIND_FILTER.
}
// FX_TEX_ALL: the passes that take their "textures" - every user effect (its lanes zeroed when it names none) and
// any BUILTIN effect pass whose object carries a set, so a builtin written to read them (cb.hlsl's KH_FX_UNIT
// section, KhUserTexLod / KhUserTexGrad) finds them bound on every route. A pass without a set pays this test only.
inline bool fx_tex_on(const RenderObject& o) {
    return o.effect == EFFECT_CUSTOM || (o.effect > 0 && o.fx_tex != nullptr);
}

// Custom material PS for a resolved user path and pipeline variant (ctx: 0 flush/static twin, 1 composite
// guard twin, 2 composite arb twin - the builtin textured define sets plus KH_USER_MAT). nullptr = pending/absent/
// failed: the caller (draw_textured) draws that submesh flat white with the white placeholder - builtin PBR only
// if the placeholder itself was not built.
inline ID3D11PixelShader* user_mat_ps(ID3D11Device* dev, const std::string& path, int ctx) {
    if (!dev || path.empty()) return nullptr;
    const UINT samp = ctx > 0 ? g_res.comp_depth_samples : 0;
    static std::string key;
    key.assign("m");
    key += std::to_string(ctx);
    key += '|';
    key += std::to_string(samp);
    key += '|';
    key += path;
    key = kh_lower_copy(std::move(key));
    auto it = g_user_shaders.ps_cache.find(key);
    if (it != g_user_shaders.ps_cache.end())
        return user_async_take(dev, key, path, "material shader");
    if (user_async_full()) return nullptr;   // USER_REQ_FULL: the placeholder, as while pending.
    UserShaderEntry e;
    std::string src, err;

    if (user_shader_source(path, src, err)) {
        const std::string sc = std::to_string(g_res.comp_depth_samples);
        const D3D_SHADER_MACRO def_flush[] = {
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        const D3D_SHADER_MACRO def_comp[] = {
            { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
            { "SAMPLE_COUNT", sc.c_str() },
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        const D3D_SHADER_MACRO def_arb[] = {
            { "MSAA_DEPTH", g_res.comp_depth_samples > 1 ? "1" : "0" },
            { "SAMPLE_COUNT", sc.c_str() },
            { "KH_ARB_DEPTH", "1" },
            { "KH_RECEIVE_TEX", "1" },
            { "KH_TEXTURED", "1" },
            { "KH_USER_MAT", "1" },
            { nullptr, nullptr },
        };
        std::string full = hlsl_src(HLSL_CB) + user_line_open(path) + src;   // USER_LINE.
        const char* entry = "PSMain";
        const D3D_SHADER_MACRO* defs = def_flush;

        if (ctx == 0) {
            full += user_line_close("static.hlsl");
            full += hlsl_src(HLSL_STATIC).c_str();
        } else {
            full += user_line_close("composite.hlsl");
            full += hlsl_src(HLSL_COMPOSITE).c_str();
            entry = "PSComposite";
            defs = ctx == 2 ? def_arb : def_comp;
        }
        // Queued: the caller draws the flat-white placeholder on nullptr while the compile is pending, not a freeze.
        std::vector<std::string> dn, dv;
        for (const D3D_SHADER_MACRO* d = defs; d && d->Name; ++d) {
            dn.push_back(d->Name);
            dv.push_back(d->Definition ? d->Definition : "");
        }
        user_async_queue(key, std::move(full), entry, dn, dv);
        return nullptr;
    }

    // Unreadable source: the failure is cached so the path is not retried.
    e.failed = true;
    report_error_once_safe("KH RenderIntegration: user material shader '" + path + "': " + err);
    g_user_shaders.ps_cache.emplace(std::move(key), e);
    return nullptr;
}

// KH_USER_VS - the vertex stage of a resolved user path: VSUserSo (static.hlsl) over cb.hlsl + the user file +
// static.hlsl, the flush / static twin's defines (KH_RECEIVE_TEX, KH_TEXTURED, KH_USER_MAT) plus KH_USER_VS,
// compiled on the pool like every user unit. Returns 0 = pending (or the queue was full: asked again), 1 = ready (the
// three objects handed out), 2 = failed (latched, reported once). The stream-output object is made from the VS
// bytecode - MeshVertex's four lanes, 48 bytes, nothing rasterised - exactly as skin_so_ensure makes GPU
// skinning's. Flush side or the render thread's step: the cache's one-thread-at-a-time contract.
inline int user_mat_vs(ID3D11Device* dev, const std::string& path, ID3D11VertexShader*& vs,
                       ID3D11GeometryShader*& gs, ID3D11InputLayout*& il) {
    vs = nullptr; gs = nullptr; il = nullptr;
    if (!dev || path.empty()) return 2;
    static std::string key;
    key.assign("v|");
    key += path;
    key = kh_lower_copy(std::move(key));
    auto it = g_user_shaders.ps_cache.find(key);
    if (it == g_user_shaders.ps_cache.end()) {
        if (user_async_full()) return 0;   // USER_REQ_FULL: pending, asked again.
        std::string src, err;
        if (!user_shader_source(path, src, err)) {
            UserShaderEntry f;
            f.failed = true;
            report_error_once_safe("KH RenderIntegration: user vertex stage '" + path + "': " + err);
            g_user_shaders.ps_cache.emplace(key, f);
            return 2;
        }
        std::vector<std::string> dn, dv;
        dn.push_back("KH_RECEIVE_TEX"); dv.push_back("1");
        dn.push_back("KH_TEXTURED");    dv.push_back("1");
        dn.push_back("KH_USER_MAT");    dv.push_back("1");
        dn.push_back("KH_USER_VS");     dv.push_back("1");
        user_async_queue(key, hlsl_src(HLSL_CB) + user_line_open(path) + src +   // USER_LINE.
                              user_line_close("static.hlsl") + hlsl_src(HLSL_STATIC).c_str(),
                         "VSUserSo", dn, dv, "vs_5_0");
        return 0;
    }
    UserShaderEntry& e = it->second;
    if (e.pending) {
        e.ask = g_user_shaders.pump_n;   // USER_REQ_IDLE.
        ID3DBlob* blob = nullptr;
        std::string err;
        bool gone = false;
        if (!shader_memo_try_take(e.hash, &blob, err, gone)) return 0;   // Not ready; the pump re-arms a lost one.
        e.pending = false;
        if (err.empty() && blob) {
            static const D3D11_SO_DECLARATION_ENTRY so[] = {
                { 0, "POSITION", 0, 0, 3, 0 },
                { 0, "NORMAL",   0, 0, 3, 0 },
                { 0, "TEXCOORD", 0, 0, 2, 0 },
                { 0, "TANGENT",  0, 0, 4, 0 },
            };
            static const D3D11_INPUT_ELEMENT_DESC ild[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TANGENT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };
            const UINT stride = static_cast<UINT>(sizeof(MeshVertex));
            HRESULT hr = dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &e.vs);
            if (SUCCEEDED(hr)) {
                hr = dev->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(),
                                                               so, 4u, &stride, 1u, D3D11_SO_NO_RASTERIZED_STREAM,
                                                               nullptr, &e.gs);
            }
            if (SUCCEEDED(hr)) {
                hr = dev->CreateInputLayout(ild, 4u, blob->GetBufferPointer(), blob->GetBufferSize(), &e.il);
            }
            if (FAILED(hr)) err = "stream-output objects: " + hr_str(hr);
        }
        if (blob) blob->Release();
        for (size_t i = 0; i < g_user_shaders.req.size(); ++i) {
            if (g_user_shaders.req[i].key == key) {
                g_user_shaders.req.erase(g_user_shaders.req.begin() + static_cast<ptrdiff_t>(i));
                break;
            }
        }
        if (!e.vs || !e.gs || !e.il) {
            KH_SAFE_RELEASE(e.il);
            KH_SAFE_RELEASE(e.gs);
            KH_SAFE_RELEASE(e.vs);
            e.failed = true;
            report_error_once_safe(user_err_report(path, "vertex stage", err));   // USER_ERR_ONCE.
        }
    }
    if (e.failed || !e.vs || !e.gs || !e.il) return 2;
    vs = e.vs; gs = e.gs; il = e.il;
    return 1;
}

// USER_LANES - what a user .hlsl reads through cb.hlsl's accessors (KhUserTime and the rest; the contract is there).
// The object half: the session clock and this object's creation on it, so a shader animates on the object's age however
// it is drawn - a bucket instance reads its creation from its record (objrec_fill), the clock from the CB.
inline void fill_user_obj_cb(ConstantData& cbd, const RenderObject& o) {
    cbd.user_obj[0] = effect_time_seconds();
    cbd.user_obj[1] = static_cast<float>(o.fx_t0);
    cbd.user_obj[2] = 0.0f;
    cbd.user_obj[3] = 0.0f;
}
// The frame half: the pass camera. view is the pass's row-vector view (world -> view; absolute or camera-relative,
// only its rotation is read), or nullptr to keep the rotation rows the block already carries (a template's).
inline void fill_user_cam_cb(ConstantData& cbd, const float* cam, const float (*view)[4]) {
    cbd.user_cam[0] = cam[0];
    cbd.user_cam[1] = cam[1];
    cbd.user_cam[2] = cam[2];
    cbd.user_cam[3] = 1.0f;
    if (!view) return;
    for (int a = 0; a < 3; ++a) {   // Row-vector view: the camera axes are its columns.
        cbd.user_cam_rot[a][0] = view[0][a];
        cbd.user_cam_rot[a][1] = view[1][a];
        cbd.user_cam_rot[a][2] = view[2][a];
        cbd.user_cam_rot[a][3] = 0.0f;
    }
}
}