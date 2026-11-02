#pragma once

// The injection half (rendering_integration_injection.hpp): the upload funnel: the capture entry (kh_upload_capture),
// the Map redirect, the write-combine copy, the scratch capture, the width cache and the upload scan. Never include it
// on its own.
namespace RenderIntegration {
// Defined in rendering/volume_seam.hpp (declared here for the uses below).
inline bool kh_fog_probe_admits(ID3D11Resource* khfp_res);
// Defined in rendering/pip.hpp (declared here for the uses below).
inline void kh_pip_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes);


// KH_WC_COPY - the one copy out of a mapped dynamic buffer, into a cached scratch every scanner then walks. The mapped
// memory is what the driver hands the engine for a WRITE_DISCARD / NO_OVERWRITE map: on NVIDIA that is cacheable system
// memory and a plain memcpy runs at memory speed; on AMD it is write-combined system memory or host-visible VRAM behind
// the PCIe BAR, where an ordinary load is one uncached bus transaction per 16 bytes and a memcpy of the engine's ~12k
// maps a frame costs tens of milliseconds - the AMD-only frame-rate collapse when the meshes light up
// (kh_upload_hook_wanted captures every constant buffer the engine maps while a lit mesh exists). MOVNTDQA (SSE4.1) is
// the documented way to read such memory: on a WC region it fetches whole 64-byte lines into the core's streaming
// buffers, one bus transaction per line and several in flight; on a cacheable region it is an ordinary load.
// Staleness: the streaming buffers are not coherent, so a line still held from an earlier capture could be returned.
// MFENCE before the loads is the SDM / APM discipline for exactly that - it retires the engine's own WC stores (made on
// this thread, before its Unmap) and drains every streaming buffer, so each load below reads memory as it stands at
// the fence. A CPU without SSE4.1 takes a plain memcpy. Render thread only; 64 KiB is the D3D11 constant-buffer cap.
// The callers: the capture (kh_upload_scratch, kh_cbs_write) and, through kh_wc_more, its extensions (KH_WC_TAIL
// below). No accounting is kept (KH_AMD_PROBE's lanes are retired).
inline bool kh_wc_stream_ok() {
    static const bool khwc_ok = [] {
        int khwc_r[4] = { 0, 0, 0, 0 };
        __cpuid(khwc_r, 1);
        return (khwc_r[2] & (1 << 19)) != 0;   // ECX bit 19: SSE4.1 (MOVNTDQA).
    }();
    return khwc_ok;
}
// KH_WC_TAIL: the streaming body alone - no fence, no size floor. Callers: kh_wc_copy, after its fence, and kh_wc_more,
// under the fence the capture in flight already took. The head is clamped to the copy, so a short copy never reads past
// its end.
inline void kh_wc_stream(uint8_t* khwc_d, const uint8_t* khwc_s, uint32_t khwc_n) {
    uint32_t khwc_head = static_cast<uint32_t>((16u - (reinterpret_cast<uintptr_t>(khwc_s) & 15u)) & 15u);
    if (khwc_head > khwc_n) khwc_head = khwc_n;
    if (khwc_head) { memcpy(khwc_d, khwc_s, khwc_head); khwc_d += khwc_head; khwc_s += khwc_head; khwc_n -= khwc_head; }
    uint32_t khwc_i = 0;
    const uint32_t khwc_n64 = khwc_n & ~63u;
    for (; khwc_i < khwc_n64; khwc_i += 64u) {
        __m128i* khwc_p = reinterpret_cast<__m128i*>(const_cast<uint8_t*>(khwc_s + khwc_i));
        const __m128i khwc_a = _mm_stream_load_si128(khwc_p);
        const __m128i khwc_b = _mm_stream_load_si128(khwc_p + 1);
        const __m128i khwc_c = _mm_stream_load_si128(khwc_p + 2);
        const __m128i khwc_e = _mm_stream_load_si128(khwc_p + 3);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(khwc_d + khwc_i), khwc_a);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(khwc_d + khwc_i + 16), khwc_b);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(khwc_d + khwc_i + 32), khwc_c);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(khwc_d + khwc_i + 48), khwc_e);
    }
    const uint32_t khwc_n16 = khwc_n & ~15u;
    for (; khwc_i < khwc_n16; khwc_i += 16u) {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(khwc_d + khwc_i),
                         _mm_stream_load_si128(reinterpret_cast<__m128i*>(const_cast<uint8_t*>(khwc_s + khwc_i))));
    }
    if (khwc_i < khwc_n) memcpy(khwc_d + khwc_i, khwc_s + khwc_i, khwc_n - khwc_i);
}
inline void kh_redir_live_add(int32_t khrl_d) noexcept {
    const int32_t khrl_n = static_cast<int32_t>(g_redir_live.load(std::memory_order_relaxed)) + khrl_d;
    g_redir_live.store(static_cast<uint32_t>(khrl_n), std::memory_order_relaxed);
}
// Is khrb_buf named by a live record (of any context)? Only the render thread writes a record's buffer, at the Map.
inline bool kh_redir_buf_live(const uint8_t* khrb_buf) noexcept {
    for (uint32_t khrb_i = 0; khrb_i < KH_REDIR_N; ++khrb_i) {
        if (g_redir[khrb_i].ctx.load(std::memory_order_acquire) && g_redir[khrb_i].buf == khrb_buf) return true;
    }
    return false;
}
// The cached buffer for engine buffer khrb_res, at least khrb_n bytes; nullptr = none can be given (khrb_on: the hook
// scope's armed snapshot, for the funnel's count). Its own entry when the probe run holds one, as it is while the
// width it serves is unchanged; else - grown, or another width at the address (a released buffer's address recycled)
// - re-assigned; else an unused entry, else one whose memory no live record names (the table's churn). A buffer a
// live record names is never handed out again or re-assigned. Memory is kept when large enough, and zeroed whenever
// the entry is (re)assigned. Not distinguished: a recycled address carrying the same width (the width cache pins what
// it holds, so only after its eviction) - its first map may meet the released buffer's bytes where the engine writes
// none, as the driver's renamed memory could. The home slot takes the product's high bits (an allocator's aligned
// addresses differ in their high bits).
inline uint32_t kh_rbuf_home(const void* khrh_res) noexcept {   // g_rbuf's home slot (kh_map_prefetch hints it too).
    return static_cast<uint32_t>(
        ((static_cast<uint64_t>(reinterpret_cast<uintptr_t>(khrh_res)) >> 4) * 0x9E3779B97F4A7C15ull) >> 53) &
        (KH_RBUF_N - 1u);
}
inline uint8_t* kh_redir_buf_for(const void* khrb_res, uint32_t khrb_n, bool khrb_on) {
    const uint32_t khrb_h = kh_rbuf_home(khrb_res);
    int32_t khrb_own = -1, khrb_empty = -1;
    for (uint32_t khrb_p = 0; khrb_p < KH_RBUF_PROBE; ++khrb_p) {
        const int32_t khrb_i = static_cast<int32_t>((khrb_h + khrb_p) & (KH_RBUF_N - 1u));
        const KhRedirBuf& khrb_e = g_rbuf[khrb_i];
        if (!khrb_e.res) { if (khrb_empty < 0) khrb_empty = khrb_i; continue; }
        if (khrb_e.res == khrb_res) { khrb_own = khrb_i; break; }
    }
    if (khrb_own >= 0) {
        KhRedirBuf& khrb_o = g_rbuf[khrb_own];
        if (kh_redir_buf_live(khrb_o.buf)) return nullptr;
        if (khrb_o.cap >= khrb_n && khrb_o.want == khrb_n) return khrb_o.buf;
    }
    int32_t khrb_at = khrb_own >= 0 ? khrb_own : khrb_empty;
    for (uint32_t khrb_p = 0; khrb_at < 0 && khrb_p < KH_RBUF_PROBE; ++khrb_p) {   // Churn: the first idle one.
        const int32_t khrb_i = static_cast<int32_t>((khrb_h + khrb_p) & (KH_RBUF_N - 1u));
        if (!kh_redir_buf_live(g_rbuf[khrb_i].buf)) khrb_at = khrb_i;
    }
    if (khrb_at < 0) return nullptr;
    KhRedirBuf& khrb_t = g_rbuf[khrb_at];
    const uint32_t khrb_cap = (khrb_n + 63u) & ~63u;
    if (khrb_t.cap < khrb_cap) {
        uint8_t* const khrb_m = new uint8_t[static_cast<size_t>(khrb_cap) + 63u];   // May throw: nothing changed yet.
        delete[] khrb_t.mem;   // No live record names it (above).
        khrb_t.mem = khrb_m;
        const uintptr_t khrb_a = (reinterpret_cast<uintptr_t>(khrb_t.mem) + 63u) & ~static_cast<uintptr_t>(63u);
        khrb_t.buf = reinterpret_cast<uint8_t*>(khrb_a);
        khrb_t.cap = khrb_cap;
    }
    memset(khrb_t.buf, 0, khrb_t.cap);
    khrb_t.res = khrb_res;
    khrb_t.want = khrb_n;
    if (khrb_on) kh_fun(KHF_REDIR_BUFS, 1u);   // KH_PROF_FUNNEL.
    return khrb_t.buf;
}
// hooked_map, after any successful Map of khrd_res on khrd_ctx (any thread): a record of an earlier mapping of it
// whose Unmap never came through the hook (a context torn down mid-map) is dropped - its driver pointer is dead, and
// an Unmap of the new mapping must not copy into it.
inline void kh_redir_drop(void* khrd_ctx, const void* khrd_res) noexcept {
    for (uint32_t khrd_i = 0; khrd_i < KH_REDIR_N; ++khrd_i) {
        KhRedir& khrd_r = g_redir[khrd_i];
        if (khrd_r.ctx.load(std::memory_order_acquire) != khrd_ctx || khrd_r.res != khrd_res) continue;
        khrd_r.ctx.store(nullptr, std::memory_order_release);
        kh_redir_live_add(-1);
    }
}
// hooked_map, an admitted WRITE_DISCARD with a pending slot (render thread): hands the engine our cached buffer -
// khro_m->pData is replaced, the record already published, and nothing is left to throw - and returns it; or
// returns nullptr, the map left as it is. khro_n: the buffer's ByteWidth; khro_on: the hook scope's armed snapshot.
inline uint8_t* kh_redir_open(void* khro_ctx, const void* khro_res, D3D11_MAPPED_SUBRESOURCE* khro_m, uint32_t khro_n,
                              bool khro_on) {
    if (!KH_MAP_REDIRECT_ON || !khro_m || !khro_m->pData || khro_n == 0u || khro_n > KH_REDIR_MAX) return nullptr;
    if (!g_reorder_hook_active.load(std::memory_order_acquire)) return nullptr;   // Map and Unmap both hooked.
    int32_t khro_free = -1;
    for (uint32_t khro_i = 0; khro_i < KH_REDIR_N && khro_free < 0; ++khro_i) {
        if (!g_redir[khro_i].ctx.load(std::memory_order_acquire)) khro_free = static_cast<int32_t>(khro_i);
    }
    if (khro_free < 0) return nullptr;
    uint8_t* const khro_b = kh_redir_buf_for(khro_res, khro_n, khro_on);   // The one call that may throw.
    if (!khro_b) return nullptr;
    KhRedir& khro_r = g_redir[khro_free];
    khro_r.res = khro_res;
    khro_r.real = khro_m->pData;
    khro_r.buf = khro_b;
    khro_r.bytes = khro_n;
    kh_redir_live_add(1);
    khro_r.ctx.store(khro_ctx, std::memory_order_release);
    khro_m->pData = khro_b;
    return khro_b;
}
// The capture's check (render thread, inside the Unmap, before kh_redir_close): is khrh_buf this context's live
// record for khrh_res?
inline bool kh_redir_holds(void* khrh_ctx, const void* khrh_res, const void* khrh_buf) noexcept {
    for (uint32_t khrh_i = 0; khrh_i < KH_REDIR_N; ++khrh_i) {
        const KhRedir& khrh_r = g_redir[khrh_i];
        if (khrh_r.ctx.load(std::memory_order_acquire) == khrh_ctx && khrh_r.res == khrh_res &&
            khrh_r.buf == khrh_buf) return true;
    }
    return false;
}
// hooked_unmap, any thread, before the original Unmap: this context's record for khrc_res, if any - its bytes to the
// driver's pointer, whole, and the record freed.
inline void kh_redir_close(void* khrc_ctx, const void* khrc_res) noexcept {
    for (uint32_t khrc_i = 0; khrc_i < KH_REDIR_N; ++khrc_i) {
        KhRedir& khrc_r = g_redir[khrc_i];
        if (khrc_r.ctx.load(std::memory_order_acquire) != khrc_ctx || khrc_r.res != khrc_res) continue;
        memcpy(khrc_r.real, khrc_r.buf, khrc_r.bytes);
        khrc_r.ctx.store(nullptr, std::memory_order_release);
        kh_redir_live_add(-1);
        return;
    }
}
// True when it fenced and streamed; false = the plain memcpy (a cached source - a redirected map's buffer,
// KH_MAP_REDIRECT - or no SSE4.1, or under 16 bytes, where no aligned 16-byte load fits). KH_WC_SMALL: do not raise
// the floor (it was 128 bytes): that sends the engine's small constant buffers (64 to 127 bytes: the scanners admit
// 64 and up) to memcpy - one uncached bus read per 16 bytes of write-combined memory. Streaming them costs one MFENCE
// per such capture where the source is cacheable (UpdateSubresource data on every vendor, and a map the redirect did
// not take on NVIDIA): the accepted price.
inline bool kh_wc_copy(void* khwc_dst, const void* khwc_src, uint32_t khwc_n) {
    if (g_wc_cached_src || khwc_n < 16u || !kh_wc_stream_ok()) {   // KH_MAP_REDIRECT: a cached source - plain.
        memcpy(khwc_dst, khwc_src, khwc_n);
        return false;
    }
    if (g_prof_up_on) kh_fun(KHF_FENCES, 1u);   // KH_PROF_FUNNEL (the funnel is this function's one user).
    _mm_mfence();
    kh_wc_stream(static_cast<uint8_t*>(khwc_dst), static_cast<const uint8_t*>(khwc_src), khwc_n);
    return true;
}
// KH_WC_TAIL: a capture's extensions are short and many - KH_REG_SLICE's 96-byte slice runs past the eager copy on
// every upload the registry records (every captured constant buffer of 800 bytes or more while the live shadow is
// wanted), and the locked scanners reach a few floats past it. Do not send a fenced capture's extensions through
// kh_wc_copy: each would pay a second fence, and a floor above 96 bytes there (KH_WC_SMALL: it was 128) would send the
// slice to memcpy - one uncached bus read per 16 bytes of write-combined memory (AMD, and Intel's shared memory where
// the driver maps it WC). An extension exists only for a buffer past KH_UPLOAD_EAGER_BYTES, so the eager copy before
// it was 384 bytes and took the fence wherever SSE4.1 streams; nothing writes the source between the two (the engine
// finished before its Unmap), so that fence stands for the extension: it streams with no fence of its own. The bytes
// are the same on every path.
inline void kh_wc_more(void* khwm_d, const void* khwm_s, uint32_t khwm_n) {
    if (g_upc.fenced) kh_wc_stream(static_cast<uint8_t*>(khwm_d), static_cast<const uint8_t*>(khwm_s), khwm_n);
    else kh_wc_copy(khwm_d, khwm_s, khwm_n);
}
inline const void* kh_upload_scratch(const void* khus_src, uint32_t& khus_bytes) {
    if (khus_bytes > KH_UPLOAD_SCRATCH_BYTES) khus_bytes = KH_UPLOAD_SCRATCH_BYTES;
    const uint32_t khus_eager = khus_bytes < KH_UPLOAD_EAGER_BYTES ? khus_bytes : KH_UPLOAD_EAGER_BYTES;
    if (g_prof_up_on) { kh_fun(KHF_EAGER_N, 1u); kh_fun(KHF_EAGER_B, khus_eager); }   // KH_PROF_FUNNEL.
    const bool khus_fenced = kh_wc_copy(g_upload_scratch, khus_src, khus_eager);
    g_upc.src = static_cast<const uint8_t*>(khus_src);
    g_upc.fenced = khus_fenced;   // KH_WC_TAIL.
    g_upc.have = khus_eager;
    g_upc.total = khus_bytes;
    return g_upload_scratch;
}
// The scanner's side: bytes [0, khun_n) of the capture at khun_d are valid when this returns. A pointer that is not the
// scratch (a per-buffer shadow, a staging readback, the binding-readback window) was copied whole: no-op.
inline void kh_upload_need(const void* khun_d, uint32_t khun_n) {
    if (!g_upc.src || khun_d != static_cast<const void*>(g_upload_scratch)) return;
    if (khun_n > g_upc.total) khun_n = g_upc.total;
    if (khun_n <= g_upc.have) return;
    if (g_prof_up_on) kh_fun_ext(khun_n - g_upc.have);   // KH_PROF_FUNNEL.
    kh_wc_more(g_upload_scratch + g_upc.have, g_upc.src + g_upc.have, khun_n - g_upc.have);   // KH_WC_TAIL.
    g_upc.have = khun_n;
}
// After the scan (the source is about to be unmapped or returned).
inline void kh_upload_capture_end() { g_upc = KhUploadCap{}; }
// KH_REG_SLICE: bytes [khur_off, khur_off + khur_n) of the capture at khur_d into khur_dst, without extending the
// scratch. What the scratch does not hold yet is read from the source it is copied from - unchanged for the whole
// scan (kh_upload_need's rule), and past the eager copy's fence - so these are the bytes a kh_upload_need to the
// range's end would have put there. The range lies inside the size the scanner was handed.
inline void kh_upload_copy_range(void* khur_dst, const void* khur_d, uint32_t khur_off, uint32_t khur_n) {
    uint8_t* const khur_o = static_cast<uint8_t*>(khur_dst);
    if (g_upc.src && khur_d == static_cast<const void*>(g_upload_scratch) && khur_off + khur_n > g_upc.have) {
        const uint32_t khur_h = khur_off < g_upc.have ? g_upc.have - khur_off : 0u;   // Already in the scratch.
        if (khur_h) memcpy(khur_o, g_upload_scratch + khur_off, khur_h);
        if (g_prof_up_on) kh_fun_ext(khur_n - khur_h);   // KH_PROF_FUNNEL.
        kh_wc_more(khur_o + khur_h, g_upc.src + khur_off + khur_h, khur_n - khur_h);   // KH_WC_TAIL.
        return;
    }
    memcpy(khur_o, static_cast<const uint8_t*>(khur_d) + khur_off, khur_n);
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
inline uint32_t kh_uwc_slot(ID3D11Resource* khuw_r) {
    return static_cast<uint32_t>((reinterpret_cast<uintptr_t>(khuw_r) >> 4) * 2654435761u) & (KH_UWC_N - 1);
}
inline void kh_upload_width_cache_release() {
    for (uint32_t khuw_i = 0; khuw_i < KH_UWC_N; ++khuw_i) {
        if (g_uwc[khuw_i].res) { g_uwc[khuw_i].res->Release(); g_uwc[khuw_i].res = nullptr; }
        g_uwc[khuw_i].width = 0;
    }
}
inline uint32_t proj_upload_byte_width(ID3D11Resource* res) {
    const uint32_t khuw_h = kh_uwc_slot(res);
    for (uint32_t khuw_p = 0; khuw_p < KH_UWC_PROBE; ++khuw_p) {
        const KhUploadWidthEntry& khuw_e = g_uwc[(khuw_h + khuw_p) & (KH_UWC_N - 1)];
        if (khuw_e.res == res) return khuw_e.width;
        if (!khuw_e.res) break;   // The run ends here: not cached.
    }
    const uint32_t khuw_w = proj_upload_byte_width_query(res);
    uint32_t khuw_at = khuw_h;   // The probe's head is the victim when the run is full.
    for (uint32_t khuw_p = 0; khuw_p < KH_UWC_PROBE; ++khuw_p) {
        const uint32_t khuw_i = (khuw_h + khuw_p) & (KH_UWC_N - 1);
        if (!g_uwc[khuw_i].res) { khuw_at = khuw_i; break; }
    }
    if (g_uwc[khuw_at].res) g_uwc[khuw_at].res->Release();
    res->AddRef();
    g_uwc[khuw_at].res = res;
    g_uwc[khuw_at].width = khuw_w;
    return khuw_w;
}
// Every captured upload, in the hooks: the shadow when the CPU capture wants the buffer, the scratch otherwise.
// KH_PROF_UPLOAD: the funnel's first step for every captured upload, so the upload's measurement decision is taken
// here (kh_prof_up_begin), and the capture's own time and counts (upCapture, KH_PROF_FUNNEL) with it.
inline const void* kh_upload_capture(ID3D11Resource* khuc_res, const void* khuc_src, uint32_t& khuc_bytes) {
    kh_prof_up_begin();
    KH_PROF_SUB(KHP_UP_CAPTURE, g_prof_up_on, g_prof_up_t);
    if (g_prof_up_on) kh_fun(KHF_UPLOADS, 1u);
    kh_upload_capture_end();   // KH_UPLOAD_LAZY: a shadow capture is whole; only the scratch sets a record.
    if (kh_dl_cpu_capture_now()) {   // KH_DL_IDLE.
        uint8_t  khuc_kind = 0u;     // KH_PROF_FUNNEL (armed only).
        uint32_t khuc_probes = 0u;
        const bool khuc_want = kh_dl_want_gate(static_cast<const void*>(khuc_res),   // KH_DL_WANT_GATE.
                                               g_prof_up_on ? &khuc_kind : nullptr,
                                               g_prof_up_on ? &khuc_probes : nullptr);
        if (g_prof_up_on) kh_fun(KHF_GATE_PROBES, khuc_probes);
        if (khuc_want) {
            const void* khuc_p = kh_cbs_write(static_cast<void*>(khuc_res), khuc_src, khuc_bytes);
            if (khuc_p) {
                if (g_prof_up_on) kh_fun_shadow(khuc_kind, khuc_bytes);
                return khuc_p;
            }
        } else {
            kh_cbs_stale(static_cast<const void*>(khuc_res));   // KH_DL_WANT_STALE: this upload goes uncaptured.
            if (g_prof_up_on) kh_fun(KHF_WANT_REFUSED, 1u);
        }
    } else if (g_prof_up_on && kh_dl_cpu_wanted()) {
        kh_fun(KHF_IDLE_SKIP, 1u);   // KH_DL_IDLE's cadence: a cycle that captures nothing whole.
    }
    return kh_upload_scratch(khuc_src, khuc_bytes);
}
inline bool kh_upload_hook_wanted(ID3D11DeviceContext* self, ID3D11Resource* res) {
    return self == g_reorder_target_ctx.load(std::memory_order_relaxed) &&
           reorder_on_render_thread() &&
           !g_ro.in_injection &&
           g_kh_track_wanted.load(std::memory_order_relaxed) &&
           (kh_cbc_on() || g_pip_seen ||
            kh_dl_cpu_wanted() ||   // KH_DL_CPU: every constant buffer the engine maps.
            kh_engcam_wanted(res) ||   // KH_ENGCAM_GATE: every map until locked, then its buffer.
            (!g_ro.engine_proj_valid && g_ro.cycle_pv_valid) || shadow_live_wanted() ||
            (g_proj_locator.valid && res == g_proj_locator.buf) ||
            // The b2 the mirror patch reads. One extra buffer, uploaded a
            // handful of times a frame; nullptr until learned.
            (g_b2_learn && static_cast<void*>(res) == g_b2_learn) ||
            kh_fog_probe_admits(res));   // KH_FOG_PROBE: effects alone (a mesh admits everything above).
}

// KH_UPLOAD_LAZY: every scanner below calls kh_upload_need before it reads past KH_UPLOAD_EAGER_BYTES of khus_d (the
// rule in rendering/upload_funnel_state.hpp) - all but shadow_register_upload, whose KH_REG_SLICE read goes through
// kh_upload_copy_range (the same bytes, no capture extended).
// KH_PROF_UPLOAD: each scanner its own step (KhProfSub, on the upload's decision taken at the capture).
inline void kh_upload_scan(ID3D11Resource* res, const void* khus_d, uint32_t khus_n) {
    KH_PROF_SCOPE(KHP_UPLOAD_SCAN);   // KH_PROF.
    {
        KH_PROF_SUB(KHP_SC_B2, g_prof_up_on, g_prof_up_t);
        kh_b2_snap_note(res, khus_d, khus_n);   // KH_MIR_CPU (no-op until b2 is learned).
    }
    {
        KH_PROF_SUB(KHP_SC_PROJ, g_prof_up_on, g_prof_up_t);
        proj_scan_upload(res, khus_d, khus_n);
    }
    {
        KH_PROF_SUB(KHP_SC_LIVE, g_prof_up_on, g_prof_up_t);
        shadow_live_upload(khus_d, khus_n);
    }
    {
        KH_PROF_SUB(KHP_SC_LOCATOR, g_prof_up_on, g_prof_up_t);
        locator_note_upload(res, khus_d, khus_n);   // fog/sun color locators (read-only).
    }
    {
        KH_PROF_SUB(KHP_SC_PIP, g_prof_up_on, g_prof_up_t);
        kh_pip_note_upload(res, khus_d, khus_n);   // KH_PIP: the engine's view / projection blocks, any pass.
    }
    {
        KH_PROF_SUB(KHP_SC_ENGCAM, g_prof_up_on, g_prof_up_t);
        kh_engcam_scan(res, khus_d, khus_n);
    }
    // The injection term is the scan's own re-check: both callers (the Unmap and UpdateSubresource
    // hooks) already turn our own uploads away, so no CB of ours reaches here.
    if (!g_ro.in_injection && shadow_live_wanted()) {
        {
            KH_PROF_SUB(KHP_SC_REG, g_prof_up_on, g_prof_up_t);
            shadow_register_upload(res, khus_d, khus_n);
        }
        KH_PROF_SUB(KHP_SC_VIEW, g_prof_up_on, g_prof_up_t);
        shadow_view_scan(res, khus_d, khus_n);
    }
}

// KH_MAP_PREFETCH - cache hints for one buffer's table lines, issued by hooked_map before the driver's Map so that
// call's time covers their loads. The render-thread cost on a CPU with a small L3 (Zen 2: 16 MB per four cores) tracks
// the lines that fall out of L2 between two uses, and those are per buffer: what every Map touches (the gates, the
// scanners' heads, the want index) is reused within microseconds and stays in L2, whose latency an X3D part shares. So
// the hints name three per-buffer lines, each an address computed from the buffer alone (no load): the redirect's slot
// g_rbuf[home] (KH_MAP_REDIRECT reads it right after the Map, and the engine's stores into the cached buffer it names
// start as soon as it is in), the width cache's slot (proj_upload_byte_width), and the shadow table's probe head the
// capture reads at the Unmap (kh_cbs_write or kh_cbs_stale). Measured on a model of one map with the buffer's lines and
// slots flushed to memory (not on a Zen 2 part): 10 - 300 ns saved cold, nothing measurable warm. Do not also hint the
// cached buffer's own lines: with any other line of the map cold, that made the map up to ~150 ns slower than no hint.
// _mm_prefetch reads nothing the code sees, writes nothing and cannot fault. Issued for a WRITE_DISCARD of subresource
// 0 on the target context, on the render thread, outside our own passes and while the hooks track anything at all -
// kh_upload_hook_wanted's first terms: every map the redirect takes (the capture's NO_OVERWRITE maps, never seen in
// Arma's constant buffers, get no hint).
inline void kh_map_prefetch(ID3D11Resource* khmf_res) noexcept {
    _mm_prefetch(reinterpret_cast<const char*>(&g_uwc[kh_uwc_slot(khmf_res)]), _MM_HINT_T0);
    _mm_prefetch(reinterpret_cast<const char*>(&g_cbs[kh_cbs_hash(khmf_res)]), _MM_HINT_T0);
    if constexpr (KH_MAP_REDIRECT_ON) {
        _mm_prefetch(reinterpret_cast<const char*>(&g_rbuf[kh_rbuf_home(khmf_res)]), _MM_HINT_T0);
    }
}
}