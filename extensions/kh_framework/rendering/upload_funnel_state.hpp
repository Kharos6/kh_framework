#pragma once

// The injection half (rendering_integration_injection.hpp): the state (types, globals and constants) of the upload
// funnel: the capture entry (kh_upload_capture), the Map redirect, the write-combine copy, the scratch capture, the
// width cache and the upload scan. Never include it on its own.
namespace RenderIntegration {
// KH_MAP_REDIRECT - the capture's source moved into cached memory. Every map the capture takes is the engine's
// WRITE_DISCARD of a constant buffer (KH_MAP_MIX: 100 % of them, none partial, every width <= 4 KiB, in the town and
// the interior), and reading the mapped memory is the capture's whole cost: a fence and a streaming read per map on
// write-combined memory, and on memory behind the PCIe BAR (AMD with Smart Access Memory) reads that cross the bus
// at a fraction of a GB/s. So hooked_map hands the engine a cached buffer of ours instead of the driver's pointer
// (kh_redir_open), and hooked_unmap copies the buffer's whole ByteWidth to the driver's pointer just before the
// original Unmap (kh_redir_close) - a write, which no memory type makes slow. The capture between them reads the
// cached copy (g_wc_cached_src: kh_wc_copy's plain memcpy, no fence). 3Dmigoto's TrackAndDivertMap does the same
// for its texture tracking and states its limit: a WRITE_NO_OVERWRITE map cannot be redirected - nobody knows which
// bytes the application wrote - so those keep the mapped read, as does every map the capture does not take (no
// pending slot, another context, a width past KH_REDIR_MAX).
// The GPU sees the engine's bytes: everything the engine wrote reaches the driver's pointer before the original
// Unmap, in the same thread. What the engine left unwritten is undefined after a DISCARD (the driver's renamed
// memory held anything); here it is the same buffer's previous bytes - one cached buffer per engine buffer
// (g_rbuf), never another buffer's, so a scanner reading an unwritten tail meets this buffer's own past contents,
// not a stranger's matrices (zeros when the cached buffer was just assigned).
// The copy-back does not depend on the capture: its record (g_redir, keyed by context and buffer) is written at the
// Map and consumed by the Unmap of that context, whatever the slot table, the gate or a reset did in between, and
// the records are never reset (a mapping in flight must reach its Unmap). A record is published by its context's
// pointer (stored last), so a Map or Unmap on another thread - a deferred context - reads the live count and
// compares context pointers, and touches nothing else. Any successful Map of a buffer drops a record of an earlier
// mapping of it on that context whose Unmap never came (kh_redir_drop). A cached buffer still named by a live
// record is never handed out again or re-assigned. The capture checks its slot's buffer is a live record's
// (kh_redir_holds) before reading it: a stale slot is not captured. The redirect costs a cached write by the engine,
// the capture's plain read, and one write of the whole ByteWidth to the driver's pointer per map.
// KH_MAP_REDIRECT_ON false: no map is redirected (the build-time off switch).
static constexpr bool     KH_MAP_REDIRECT_ON = true;
static constexpr uint32_t KH_REDIR_N = 8u;           // Maps in flight at once (as g_proj_pending).
static constexpr uint32_t KH_REDIR_MAX = 16384u;     // The widest map redirected (measured: none past 4 KiB).
static constexpr uint32_t KH_RBUF_N = 2048u;         // Cached buffers, one per engine buffer.
static constexpr uint32_t KH_RBUF_PROBE = 8u;
static_assert(KH_RBUF_N == 2048u, "KH_MAP_REDIRECT: kh_rbuf_home's >> 53 takes 11 bits - re-derive it");
struct KhRedirBuf {
    const void* res = nullptr;       // The engine buffer it serves (weak: an identity, never dereferenced).
    uint32_t    cap = 0;             // Usable bytes at buf.
    uint32_t    want = 0;            // The width it last served (another: a new buffer at a recycled address).
    uint8_t*    buf = nullptr;       // 64-byte aligned, inside mem.
    // new[] / delete[] by kh_redir_buf_for alone, and never freed by a destructor: at an unload (FreeLibrary) the
    // engine can be writing a buffer handed out before the hooks went, and a static destructor would free it under
    // that write (the renderer's rule for objects that may outlive static teardown, as g_khsm_thr).
    uint8_t*    mem = nullptr;
};
// KH_HOT_ALIGN: the per-buffer tables the Map / Unmap path probes start on a cache line - an entry of g_rbuf (32 B),
// g_uwc (16 B) or g_cbs (64 B) then never straddles two, and the want index (128 B), g_redir (8 x 40 B) and
// g_proj_pending (8 x 24 B) take their fewest lines (2, 5, 3). Addresses only.
alignas(64) static KhRedirBuf g_rbuf[KH_RBUF_N];
struct KhRedir {
    std::atomic<void*> ctx{ nullptr };   // The mapping context; null = free. Stored last at the Map; cleared at the
                                         // Unmap after its copy, or when the next Map of the buffer drops it.
    const void* res = nullptr;           // The mapped buffer.
    void*       real = nullptr;          // The driver's pointer.
    uint8_t*    buf = nullptr;           // The cached buffer the engine writes.
    uint32_t    bytes = 0;               // The buffer's ByteWidth: all of it goes to real.
};
alignas(64) static KhRedir g_redir[KH_REDIR_N];   // KH_HOT_ALIGN.
// Live records: a hint that lets an Unmap or a Map with none skip the search. Written (plain load and store, no
// locked instruction - a map is 21k - 42k a frame) only by a Map or Unmap on a context that owns a record: the
// immediate context, which the engine never uses from two threads at once (KH_UI_DIAG: the runtime's multithread
// protection is off and the engine hands the context over between its render and game threads) - so no update is
// lost, and the hand-over orders each update before the next. A deferred context only reads it (a record could name
// one only at the address of a context torn down mid-map, recycled - then one copy-back could be skipped, and no
// write lands in freed memory), and owns no record a stale value could make it miss.
static std::atomic<uint32_t> g_redir_live{ 0u };
static bool g_wc_cached_src = false;   // The capture in flight reads a cached buffer (render thread; hooked_unmap).
// KH_AMD_PROBE / KH_WC_COPY (measured, the lanes retired): the funnel's cost is its capture count times the bytes each
// capture copies, and the count is the engine's draw count (12-13k a cycle here, ~1.3 ms with the scan) - it is at its
// per-capture floor, so kh_wc_copy's streaming read is the lever, not the count. The staging polls that answer
// WAS_STILL_DRAWING and the read-only DSV swap were the two other vendor-sensitive shapes examined and neither was the
// AMD collapse.
// KH_UPLOAD_LAZY: that lever, applied. The scanners read a few hundred bytes of most admitted maps (the sky block's 96
// floats, the PIP view block's 60, a locked locator's own offset) and walk a whole buffer only while hunting, so the
// capture copies the first KH_UPLOAD_EAGER_BYTES and remembers the source (g_upc); every scanner states, at the read,
// how far it is about to read (kh_upload_need), and the rest is copied from the same source then. The source stays
// valid for the whole scan (the unmap hook scans before the original Unmap runs; UpdateSubresource's data is the
// caller's until it returns), and no one writes it in between (the engine finished writing before it called Unmap),
// so a scanner reads exactly the bytes a whole copy would hold. The size each scanner is handed is the buffer's own.
// RULE: a scanner fed from kh_upload_scan must call kh_upload_need before it reads past KH_UPLOAD_EAGER_BYTES - a read
// past what was copied is the previous capture's bytes. The per-buffer shadows (kh_cbs_write, KH_DL_CPU) still copy
// whole: the light sampler reads them later, at draw time.
static constexpr uint32_t KH_UPLOAD_SCRATCH_BYTES = 65536u;
static constexpr uint32_t KH_UPLOAD_EAGER_BYTES = 384u;   // The sky block's 96 floats; covers the PIP / b2 reads.
static alignas(64) uint8_t g_upload_scratch[KH_UPLOAD_SCRATCH_BYTES];
// The capture in flight (render thread): its source, the bytes copied so far and the size handed to the scanners. src =
// nullptr: none (every need is a no-op). Set by kh_upload_scratch, cleared by kh_upload_capture_end. fenced: the eager
// copy took kh_wc_copy's fence (KH_WC_TAIL).
struct KhUploadCap { const uint8_t* src = nullptr; uint32_t have = 0; uint32_t total = 0; bool fenced = false; };
static KhUploadCap g_upc;

// KH_UPLOAD_WIDTH_CACHE: the engine maps ~12k buffers a frame, and the two desc calls of proj_upload_byte_width_query
// (rendering/upload_funnel.hpp) would be paid on each. The width is cached by object; the object is AddRef'd while
// cached, which makes a pointer key sound (the hazard: a recycled address carrying a stale width lets a scanner read
// past a smaller mapping; a pinned object's address is never recycled). Open addressing, a short probe, eviction at the
// probe's head. Render thread only; released with the device in release_shadow_device_state.
static constexpr uint32_t KH_UWC_N = 512;   // Power of two.
static constexpr uint32_t KH_UWC_PROBE = 8;
struct KhUploadWidthEntry { ID3D11Resource* res; uint32_t width; };
alignas(64) static KhUploadWidthEntry g_uwc[KH_UWC_N] = {};   // KH_HOT_ALIGN.
}