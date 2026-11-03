#pragma once

// The injection half (rendering_integration_injection.hpp): the upload funnel: the capture entry (upload_capture),
// the Map redirect, the write-combine copy, the scratch capture, the width cache and the upload scan. Never include it
// on its own.
namespace RenderIntegration {
// Defined in rendering/volume_seam.hpp (declared here for the uses below).
inline bool fog_probe_admits(ID3D11Resource* res);
// Defined in rendering/pip.hpp (declared here for the uses below).
inline void pip_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes);


// WC_COPY - the one copy out of a mapped dynamic buffer, into a cached scratch every scanner then walks. The mapped
// memory is what the driver hands the engine for a WRITE_DISCARD / NO_OVERWRITE map: on NVIDIA that is cacheable system
// memory and a plain memcpy runs at memory speed; on AMD it is write-combined system memory or host-visible VRAM behind
// the PCIe BAR, where an ordinary load is one uncached bus transaction per 16 bytes and a memcpy of the engine's ~12k
// maps a frame costs tens of milliseconds - the AMD-only frame-rate collapse when the meshes light up
// (upload_hook_wanted captures every constant buffer the engine maps while a lit mesh exists). MOVNTDQA (SSE4.1) is
// the documented way to read such memory: on a WC region it fetches whole 64-byte lines into the core's streaming
// buffers, one bus transaction per line and several in flight; on a cacheable region it is an ordinary load.
// Staleness: the streaming buffers are not coherent, so a line still held from an earlier capture could be returned.
// MFENCE before the loads is the SDM / APM discipline for exactly that - it retires the engine's own WC stores (made on
// this thread, before its Unmap) and drains every streaming buffer, so each load below reads memory as it stands at
// the fence. A CPU without SSE4.1 takes a plain memcpy. Render thread only; 64 KiB is the D3D11 constant-buffer cap.
// The callers: the capture (upload_scratch, cbs_write) and, through wc_more, its extensions (WC_TAIL
// below). No accounting is kept (AMD_PROBE's lanes are retired).
inline bool wc_stream_ok() {
    static const bool ok = [] {
        int r[4] = { 0, 0, 0, 0 };
        __cpuid(r, 1);
        return (r[2] & (1 << 19)) != 0;   // ECX bit 19: SSE4.1 (MOVNTDQA).
    }();
    return ok;
}
// WC_TAIL: the streaming body alone - no fence, no size floor. Callers: wc_copy, after its fence, and wc_more,
// under the fence the capture in flight already took. The head is clamped to the copy, so a short copy never reads past
// its end.
inline void wc_stream(uint8_t* d, const uint8_t* s, uint32_t n) {
    uint32_t head = static_cast<uint32_t>((16u - (reinterpret_cast<uintptr_t>(s) & 15u)) & 15u);
    if (head > n) head = n;
    if (head) { memcpy(d, s, head); d += head; s += head; n -= head; }
    uint32_t i = 0;
    const uint32_t n64 = n & ~63u;
    for (; i < n64; i += 64u) {
        __m128i* p = reinterpret_cast<__m128i*>(const_cast<uint8_t*>(s + i));
        const __m128i a = _mm_stream_load_si128(p);
        const __m128i b = _mm_stream_load_si128(p + 1);
        const __m128i c = _mm_stream_load_si128(p + 2);
        const __m128i e = _mm_stream_load_si128(p + 3);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i), a);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i + 16), b);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i + 32), c);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i + 48), e);
    }
    const uint32_t n16 = n & ~15u;
    for (; i < n16; i += 16u) {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(d + i),
                         _mm_stream_load_si128(reinterpret_cast<__m128i*>(const_cast<uint8_t*>(s + i))));
    }
    if (i < n) memcpy(d + i, s + i, n - i);
}
inline void redir_live_add(int32_t d) noexcept {
    const int32_t n = static_cast<int32_t>(g_redir_live.load(std::memory_order_relaxed)) + d;
    g_redir_live.store(static_cast<uint32_t>(n), std::memory_order_relaxed);
}
// Is buf named by a live record (of any context)? Only the render thread writes a record's buffer, at the Map.
inline bool redir_buf_live(const uint8_t* buf) noexcept {
    for (uint32_t i = 0; i < REDIR_N; ++i) {
        if (g_redir[i].ctx.load(std::memory_order_acquire) && g_redir[i].buf == buf) return true;
    }
    return false;
}
// The cached buffer for engine buffer res, at least n bytes; nullptr = none can be given (on: the hook
// scope's armed snapshot, for the funnel's count). Its own entry when the probe run holds one, as it is while the
// width it serves is unchanged; else - grown, or another width at the address (a released buffer's address recycled)
// - re-assigned; else an unused entry, else one whose memory no live record names (the table's churn). A buffer a
// live record names is never handed out again or re-assigned. Memory is kept when large enough, and zeroed whenever
// the entry is (re)assigned. Not distinguished: a recycled address carrying the same width (the width cache pins what
// it holds, so only after its eviction) - its first map may meet the released buffer's bytes where the engine writes
// none, as the driver's renamed memory could. The home slot takes the product's high bits (an allocator's aligned
// addresses differ in their high bits).
inline uint32_t rbuf_home(const void* res) noexcept {   // g_rbuf's home slot (map_prefetch hints it too).
    return static_cast<uint32_t>(
        ((static_cast<uint64_t>(reinterpret_cast<uintptr_t>(res)) >> 4) * 0x9E3779B97F4A7C15ull) >> 53) &
        (RBUF_N - 1u);
}
inline uint8_t* redir_buf_for(const void* res, uint32_t n, bool on) {
    const uint32_t h = rbuf_home(res);
    int32_t own = -1, empty = -1;
    for (uint32_t p = 0; p < RBUF_PROBE; ++p) {
        const int32_t i = static_cast<int32_t>((h + p) & (RBUF_N - 1u));
        const RedirBuf& e = g_rbuf[i];
        if (!e.res) { if (empty < 0) empty = i; continue; }
        if (e.res == res) { own = i; break; }
    }
    if (own >= 0) {
        RedirBuf& o = g_rbuf[own];
        if (redir_buf_live(o.buf)) return nullptr;
        if (o.cap >= n && o.want == n) return o.buf;
    }
    int32_t at = own >= 0 ? own : empty;
    for (uint32_t p = 0; at < 0 && p < RBUF_PROBE; ++p) {   // Churn: the first idle one.
        const int32_t i = static_cast<int32_t>((h + p) & (RBUF_N - 1u));
        if (!redir_buf_live(g_rbuf[i].buf)) at = i;
    }
    if (at < 0) return nullptr;
    RedirBuf& t = g_rbuf[at];
    const uint32_t cap = (n + 63u) & ~63u;
    if (t.cap < cap) {
        uint8_t* const m = new uint8_t[static_cast<size_t>(cap) + 63u];   // May throw: nothing changed yet.
        delete[] t.mem;   // No live record names it (above).
        t.mem = m;
        const uintptr_t a = (reinterpret_cast<uintptr_t>(t.mem) + 63u) & ~static_cast<uintptr_t>(63u);
        t.buf = reinterpret_cast<uint8_t*>(a);
        t.cap = cap;
    }
    memset(t.buf, 0, t.cap);
    t.res = res;
    t.want = n;
    if (on) fun_add(FUNNEL_REDIR_BUFS, 1u);   // PROF_FUNNEL.
    return t.buf;
}
// hooked_map, after any successful Map of res on ctx (any thread): a record of an earlier mapping of it
// whose Unmap never came through the hook (a context torn down mid-map) is dropped - its driver pointer is dead, and
// an Unmap of the new mapping must not copy into it.
inline void redir_drop(void* ctx, const void* res) noexcept {
    for (uint32_t i = 0; i < REDIR_N; ++i) {
        Redir& r = g_redir[i];
        if (r.ctx.load(std::memory_order_acquire) != ctx || r.res != res) continue;
        r.ctx.store(nullptr, std::memory_order_release);
        redir_live_add(-1);
    }
}
// hooked_map, an admitted WRITE_DISCARD with a pending slot (render thread): hands the engine our cached buffer -
// m->pData is replaced, the record already published, and nothing is left to throw - and returns it; or
// returns nullptr, the map left as it is. n: the buffer's ByteWidth; on: the hook scope's armed snapshot.
inline uint8_t* redir_open(void* ctx, const void* res, D3D11_MAPPED_SUBRESOURCE* m, uint32_t n,
                           bool on) {
    if (!MAP_REDIRECT_ON || !m || !m->pData || n == 0u || n > REDIR_MAX) return nullptr;
    if (!g_reorder_hook.active.load(std::memory_order_acquire)) return nullptr;   // Map and Unmap both hooked.
    int32_t ro_free = -1;
    for (uint32_t i = 0; i < REDIR_N && ro_free < 0; ++i) {
        if (!g_redir[i].ctx.load(std::memory_order_acquire)) ro_free = static_cast<int32_t>(i);
    }
    if (ro_free < 0) return nullptr;
    uint8_t* const b = redir_buf_for(res, n, on);   // The one call that may throw.
    if (!b) return nullptr;
    Redir& r = g_redir[ro_free];
    r.res = res;
    r.real = m->pData;
    r.buf = b;
    r.bytes = n;
    redir_live_add(1);
    r.ctx.store(ctx, std::memory_order_release);
    m->pData = b;
    return b;
}
// The capture's check (render thread, inside the Unmap, before redir_close): is buf this context's live
// record for res?
inline bool redir_holds(void* ctx, const void* res, const void* buf) noexcept {
    for (uint32_t i = 0; i < REDIR_N; ++i) {
        const Redir& r = g_redir[i];
        if (r.ctx.load(std::memory_order_acquire) == ctx && r.res == res &&
            r.buf == buf) return true;
    }
    return false;
}
// hooked_unmap, any thread, before the original Unmap: this context's record for res, if any - its bytes to the
// driver's pointer, whole, and the record freed.
inline void redir_close(void* ctx, const void* res) noexcept {
    for (uint32_t i = 0; i < REDIR_N; ++i) {
        Redir& r = g_redir[i];
        if (r.ctx.load(std::memory_order_acquire) != ctx || r.res != res) continue;
        memcpy(r.real, r.buf, r.bytes);
        r.ctx.store(nullptr, std::memory_order_release);
        redir_live_add(-1);
        return;
    }
}
// True when it fenced and streamed; false = the plain memcpy (a cached source - a redirected map's buffer,
// MAP_REDIRECT - or no SSE4.1, or under 16 bytes, where no aligned 16-byte load fits). WC_SMALL: do not raise
// the floor (it was 128 bytes): that sends the engine's small constant buffers (64 to 127 bytes: the scanners admit
// 64 and up) to memcpy - one uncached bus read per 16 bytes of write-combined memory. Streaming them costs one MFENCE
// per such capture where the source is cacheable (UpdateSubresource data on every vendor, and a map the redirect did
// not take on NVIDIA): the accepted price.
inline bool wc_copy(void* dst, const void* src, uint32_t n) {
    if (g_wc_cached_src || n < 16u || !wc_stream_ok()) {   // MAP_REDIRECT: a cached source - plain.
        memcpy(dst, src, n);
        return false;
    }
    if (g_prof.up_on) fun_add(FUNNEL_FENCES, 1u);   // PROF_FUNNEL (the funnel is this function's one user).
    _mm_mfence();
    wc_stream(static_cast<uint8_t*>(dst), static_cast<const uint8_t*>(src), n);
    return true;
}
// WC_TAIL: a capture's extensions are short and many - REG_SLICE's 96-byte slice runs past the eager copy on
// every upload the registry records (every captured constant buffer of 800 bytes or more while the live shadow is
// wanted), and the locked scanners reach a few floats past it. Do not send a fenced capture's extensions through
// wc_copy: each would pay a second fence, and a floor above 96 bytes there (WC_SMALL: it was 128) would send the
// slice to memcpy - one uncached bus read per 16 bytes of write-combined memory (AMD, and Intel's shared memory where
// the driver maps it WC). An extension exists only for a buffer past UPLOAD_EAGER_BYTES, so the eager copy before
// it was 384 bytes and took the fence wherever SSE4.1 streams; nothing writes the source between the two (the engine
// finished before its Unmap), so that fence stands for the extension: it streams with no fence of its own. The bytes
// are the same on every path.
inline void wc_more(void* d, const void* s, uint32_t n) {
    if (g_upc.fenced) wc_stream(static_cast<uint8_t*>(d), static_cast<const uint8_t*>(s), n);
    else wc_copy(d, s, n);
}
inline const void* upload_scratch(const void* src, uint32_t& bytes) {
    if (bytes > UPLOAD_SCRATCH_BYTES) bytes = UPLOAD_SCRATCH_BYTES;
    const uint32_t eager = bytes < UPLOAD_EAGER_BYTES ? bytes : UPLOAD_EAGER_BYTES;
    if (g_prof.up_on) { fun_add(FUNNEL_EAGER_N, 1u); fun_add(FUNNEL_EAGER_B, eager); }   // PROF_FUNNEL.
    const bool fenced = wc_copy(g_upload_scratch, src, eager);
    g_upc.src = static_cast<const uint8_t*>(src);
    g_upc.fenced = fenced;   // WC_TAIL.
    g_upc.have = eager;
    g_upc.total = bytes;
    return g_upload_scratch;
}
// The scanner's side: bytes [0, n) of the capture at d are valid when this returns. A pointer that is not the
// scratch (a per-buffer shadow, a staging readback, the binding-readback window) was copied whole: no-op.
inline void upload_need(const void* d, uint32_t n) {
    if (!g_upc.src || d != static_cast<const void*>(g_upload_scratch)) return;
    if (n > g_upc.total) n = g_upc.total;
    if (n <= g_upc.have) return;
    if (g_prof.up_on) fun_ext(n - g_upc.have);   // PROF_FUNNEL.
    wc_more(g_upload_scratch + g_upc.have, g_upc.src + g_upc.have, n - g_upc.have);   // WC_TAIL.
    g_upc.have = n;
}
// After the scan (the source is about to be unmapped or returned).
inline void upload_capture_end() { g_upc = UploadCap{}; }
// REG_SLICE: bytes [off, off + n) of the capture at d into dst, without extending the
// scratch. What the scratch does not hold yet is read from the source it is copied from - unchanged for the whole
// scan (upload_need's rule), and past the eager copy's fence - so these are the bytes a upload_need to the
// range's end would have put there. The range lies inside the size the scanner was handed.
inline void upload_copy_range(void* dst, const void* d, uint32_t off, uint32_t n) {
    uint8_t* const o = static_cast<uint8_t*>(dst);
    if (g_upc.src && d == static_cast<const void*>(g_upload_scratch) && off + n > g_upc.have) {
        const uint32_t h = off < g_upc.have ? g_upc.have - off : 0u;   // Already in the scratch.
        if (h) memcpy(o, g_upload_scratch + off, h);
        if (g_prof.up_on) fun_ext(n - h);   // PROF_FUNNEL.
        wc_more(o + h, g_upc.src + off + h, n - h);   // WC_TAIL.
        return;
    }
    memcpy(o, static_cast<const uint8_t*>(d) + off, n);
}

// Returns the buffer's byte width when it is a plausibly-sized constant buffer, 0 otherwise.
inline uint32_t proj_upload_byte_width_query(ID3D11Resource* res) {
    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    res->GetType(&dim);
    if (dim != D3D11_RESOURCE_DIMENSION_BUFFER) return 0;

    D3D11_BUFFER_DESC bd = {};
    static_cast<ID3D11Buffer*>(res)->GetDesc(&bd);

    if ((bd.BindFlags & D3D11_BIND_CONSTANT_BUFFER) == 0 || bd.ByteWidth < 16 * sizeof(float)) {
        return 0;
    }

    return bd.ByteWidth;
}
inline uint32_t uwc_slot(ID3D11Resource* r) {
    return static_cast<uint32_t>((reinterpret_cast<uintptr_t>(r) >> 4) * 2654435761u) & (UWC_N - 1);
}
inline void upload_width_cache_release() {
    for (uint32_t i = 0; i < UWC_N; ++i) {
        if (g_uwc[i].res) { g_uwc[i].res->Release(); g_uwc[i].res = nullptr; }
        g_uwc[i].width = 0;
    }
}
inline uint32_t proj_upload_byte_width(ID3D11Resource* res) {
    const uint32_t h = uwc_slot(res);
    for (uint32_t p = 0; p < UWC_PROBE; ++p) {
        const UploadWidthEntry& e = g_uwc[(h + p) & (UWC_N - 1)];
        if (e.res == res) return e.width;
        if (!e.res) break;   // The run ends here: not cached.
    }
    const uint32_t w = proj_upload_byte_width_query(res);
    uint32_t at = h;   // The probe's head is the victim when the run is full.
    for (uint32_t p = 0; p < UWC_PROBE; ++p) {
        const uint32_t i = (h + p) & (UWC_N - 1);
        if (!g_uwc[i].res) { at = i; break; }
    }
    if (g_uwc[at].res) g_uwc[at].res->Release();
    res->AddRef();
    g_uwc[at].res = res;
    g_uwc[at].width = w;
    return w;
}
// Every captured upload, in the hooks: the shadow when the CPU capture wants the buffer, the scratch otherwise.
// PROF_UPLOAD: the funnel's first step for every captured upload, so the upload's measurement decision is taken
// here (prof_up_begin), and the capture's own time and counts (upCapture, PROF_FUNNEL) with it.
inline const void* upload_capture(ID3D11Resource* res, const void* src, uint32_t& bytes) {
    prof_up_begin();
    KH_PROF_SUB(PROF_UP_CAPTURE, g_prof.up_on, g_prof.up_t);
    if (g_prof.up_on) fun_add(FUNNEL_UPLOADS, 1u);
    upload_capture_end();   // UPLOAD_LAZY: a shadow capture is whole; only the scratch sets a record.
    if (dl_cpu_capture_now()) {   // DL_IDLE.
        uint8_t  kind = 0u;     // PROF_FUNNEL (armed only).
        uint32_t probes = 0u;
        const bool want = dl_want_gate(static_cast<const void*>(res),   // DL_WANT_GATE.
                                       g_prof.up_on ? &kind : nullptr,
                                       g_prof.up_on ? &probes : nullptr);
        if (g_prof.up_on) fun_add(FUNNEL_GATE_PROBES, probes);
        if (want) {
            const void* p = cbs_write(static_cast<void*>(res), src, bytes);
            if (p) {
                if (g_prof.up_on) fun_shadow(kind, bytes);
                return p;
            }
        } else {
            cbs_stale(static_cast<const void*>(res));   // DL_WANT_STALE: this upload goes uncaptured.
            if (g_prof.up_on) fun_add(FUNNEL_WANT_REFUSED, 1u);
        }
    } else if (g_prof.up_on && dl_cpu_wanted()) {
        fun_add(FUNNEL_IDLE_SKIP, 1u);   // DL_IDLE's cadence: a cycle that captures nothing whole.
    }
    return upload_scratch(src, bytes);
}
inline bool upload_hook_wanted(ID3D11DeviceContext* self, ID3D11Resource* res) {
    return self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
           reorder_on_render_thread() &&
           !g_ro.in_injection &&
           g_track_wanted.load(std::memory_order_relaxed) &&
           (cbc_on() || g_pip_seen ||
            dl_cpu_wanted() ||   // DL_CPU: every constant buffer the engine maps.
            engcam_wanted(res) ||   // ENGCAM_GATE: every map until locked, then its buffer.
            (!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted() ||
            (g_proj_locator.valid && res == g_proj_locator.buf) ||
            // The b2 the mirror patch reads. One extra buffer, uploaded a
            // handful of times a frame; nullptr until learned.
            (g_b2_learn && static_cast<void*>(res) == g_b2_learn) ||
            fog_probe_admits(res));   // FOG_PROBE: effects alone (a mesh admits everything above).
}

// UPLOAD_LAZY: every scanner below calls upload_need before it reads past UPLOAD_EAGER_BYTES of d (the
// rule in rendering/upload_funnel_state.hpp) - all but shadow_register_upload, whose REG_SLICE read goes through
// upload_copy_range (the same bytes, no capture extended).
// PROF_UPLOAD: each scanner its own step (ProfSub, on the upload's decision taken at the capture).
inline void upload_scan(ID3D11Resource* res, const void* d, uint32_t n) {
    KH_PROF_SCOPE(PROF_UPLOAD_SCAN);   // PROF.
    {
        KH_PROF_SUB(PROF_SC_B2, g_prof.up_on, g_prof.up_t);
        b2_snap_note(res, d, n);   // MIR_CPU (no-op until b2 is learned).
    }
    {
        KH_PROF_SUB(PROF_SC_PROJ, g_prof.up_on, g_prof.up_t);
        proj_scan_upload(res, d, n);
    }
    {
        KH_PROF_SUB(PROF_SC_LIVE, g_prof.up_on, g_prof.up_t);
        shadow_live_upload(d, n);
    }
    {
        KH_PROF_SUB(PROF_SC_LOCATOR, g_prof.up_on, g_prof.up_t);
        locator_note_upload(res, d, n);   // fog/sun color locators (read-only).
    }
    {
        KH_PROF_SUB(PROF_SC_PIP, g_prof.up_on, g_prof.up_t);
        pip_note_upload(res, d, n);   // PIP: the engine's view / projection blocks, any pass.
    }
    {
        KH_PROF_SUB(PROF_SC_ENGCAM, g_prof.up_on, g_prof.up_t);
        engcam_scan(res, d, n);
    }
    // The injection term is the scan's own re-check: both callers (the Unmap and UpdateSubresource
    // hooks) already turn our own uploads away, so no CB of ours reaches here.
    if (!g_ro.in_injection && shadow_live_wanted()) {
        {
            KH_PROF_SUB(PROF_SC_REG, g_prof.up_on, g_prof.up_t);
            shadow_register_upload(res, d, n);
        }
        KH_PROF_SUB(PROF_SC_VIEW, g_prof.up_on, g_prof.up_t);
        shadow_view_scan(res, d, n);
    }
}

// MAP_PREFETCH - cache hints for one buffer's table lines, issued by hooked_map before the driver's Map so that
// call's time covers their loads. The render-thread cost on a CPU with a small L3 (Zen 2: 16 MB per four cores) tracks
// the lines that fall out of L2 between two uses, and those are per buffer: what every Map touches (the gates, the
// scanners' heads, the want index) is reused within microseconds and stays in L2, whose latency an X3D part shares. So
// the hints name three per-buffer lines, each an address computed from the buffer alone (no load): the redirect's slot
// g_rbuf[home] (MAP_REDIRECT reads it right after the Map, and the engine's stores into the cached buffer it names
// start as soon as it is in), the width cache's slot (proj_upload_byte_width), and the shadow table's probe head the
// capture reads at the Unmap (cbs_write or cbs_stale). Measured on a model of one map with the buffer's lines and
// slots flushed to memory (not on a Zen 2 part): 10 - 300 ns saved cold, nothing measurable warm. Do not also hint the
// cached buffer's own lines: with any other line of the map cold, that made the map up to ~150 ns slower than no hint.
// _mm_prefetch reads nothing the code sees, writes nothing and cannot fault. Issued for a WRITE_DISCARD of subresource
// 0 on the target context, on the render thread, outside our own passes and while the hooks track anything at all -
// upload_hook_wanted's first terms: every map the redirect takes (the capture's NO_OVERWRITE maps, never seen in
// Arma's constant buffers, get no hint).
inline void map_prefetch(ID3D11Resource* res) noexcept {
    _mm_prefetch(reinterpret_cast<const char*>(&g_uwc[uwc_slot(res)]), _MM_HINT_T0);
    _mm_prefetch(reinterpret_cast<const char*>(&g_cbs[cbs_hash(res)]), _MM_HINT_T0);
    if constexpr (MAP_REDIRECT_ON) {
        _mm_prefetch(reinterpret_cast<const char*>(&g_rbuf[rbuf_home(res)]), _MM_HINT_T0);
    }
}
}