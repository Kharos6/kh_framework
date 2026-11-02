#pragma once

// The injection half (rendering_integration_injection.hpp): KH_VOL_REPLAY, the recorder and the replay. Never include
// it on its own.
namespace RenderIntegration {

// KH_INFRONT - the view-model seam. MEASURED (the same capture): the engine's stencil shadows are counted in a separate
// volume buffer BEFORE the main depth clear - a world depth prepass, the world's volume counting (stencil IncWrap /
// DecWrap under depth GreaterEqual, no write), then the HANDS PREPASS into the same buffer in the hands range (depth
// LESS_EQUAL + write, stencil AlwaysTrue / Replace 0: the count under the hands reset), then the hands' own counting in
// the hands range (non-hands pixels fail the depth test with zfail Keep and are untouched), then the resolve our copy
// is taken at. The recount redraws only the volumes that reach the camera (MEASURED: 15 of the world count's 58 draws -
// the character's own and a few near ones), and the depth partition hides every world-range volume from a hands-range
// depth. So an inFront mesh takes part twice: at the world seam (inside kh_volume_seam_inject, the world range under
// the depth clamp) for the world count, and here, at the FIRST hands-range draw on the volume buffer - the engine's own
// hands prepass, ahead of its first draw - with its hands-range depth and the stencil untouched: the engine's Replace 0
// then fires only where its hands beat us, the world count survives under the mesh, and the recount adds its near
// subset against our depth (a shadowed count stays nonzero and small, the shader's test).
//
// The view is the world seam's own take (g_svs_prime_vp, this window) with column 2 rebuilt from the hands pair - exact
// for a projection that differs only in m22 / m32 - or, when the world seam did not run this window (no world caster),
// the same take made here. Depth only, level 0, the solid texels of an alpha material through PSInjDepthA (the slice
// prepass's recipe). Defined in rendering/view_model_seam.hpp.
inline void kh_infront_seam_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& khvs_vp);
// KH_VOL_REPLAY - the recorder and the replay (the design note is with the stage state, above the lane fill).
// Render thread. A pass is every draw with the volume buffer bound from its first counting draw to the stencil
// resolve (the bracket): the counting draws and the uncounted ones beside a target (the engine's hands prepass among
// them), in order. A draw is recorded with everything it had bound but its outputs, its class instances and its
// predicate (KH_RP_BIND_ALL; the replay draws into its own depth target, unpredicated: a draw the engine's predicate
// skipped is replayed - the query's verdict is the GPU's, and the predicate may be issued again before the replay).
// The constant, vertex and index buffers a draw reads that the engine can still rewrite are copied as the draw used
// them: constant buffers whole (once per buffer per draw, or once per pass while the engine does not write it -
// KH_REPLAY_CBREUSE; from the CPU where the upload funnel captured the write - KH_REPLAY_CPUCB, never an update made
// under a predicate - KH_RP_IMG_PRED), vertex / index buffers at the pass's end, and a vertex / index
// buffer a record reads that the engine writes inside the pass, ahead of the write, for the draws before it
// (KH_REPLAY_SPLIT): a DISCARD map (a ring's wrap), UpdateSubresource(1), CopyResource, CopySubresourceRegion(1) or
// CopyStructureCount into it, or its bind as an output-merger UAV (KH_RP_WRITE_SPLIT). A NO_OVERWRITE map or a
// D3D11_COPY_NO_OVERWRITE write splits nothing: its contract leaves what the draws so far read alone; a pass whose
// other writes split past KH_RP_WSPLIT_MAX / KH_RP_WSPLIT_BYTES is refused (KH_RP_SPLIT_CAP). Not seen, so not
// covered: a write through a stream-output target, a compute-stage UAV, a render target view of a buffer or a view's
// clear, DiscardResource / DiscardView, and a writer whose hook did not install (kh_rp_extra_hooks) - none of which
// the engine is known to aim at a vertex or index buffer; and the contents behind a VS / PS shader-resource view,
// bound by reference as recorded and read as they stand at the replay. A pass something else writes the volume
// buffer in (a clear, our world seam, a command list, an indirect draw or DrawAuto, a copy into it) is refused and
// the frame keeps the copy; our in-front footprint is left to the merge's per-pixel trust (kh_rp_foreign).
// KH_RP_UNCAPPED: a pass has no draw limit - the records grow on demand (g_rp_draws). A map, a write or a constant
// capture is a lookup (KhRpPtrMap), not a walk of the records or the reuse list; only a split (KH_REPLAY_SPLIT: an
// engine write into a buffer a record still flags - a DISCARD map once per ring wrap) walks the records drawn so far.
// A pass costs its draws plus one such walk per split.
// KH_RP_BIND_ALL: every vertex buffer slot, every VS / PS shader-resource and sampler slot - what the engine's draw had
// bound, the slots its shaders read among them, is what the replayed draw binds (no shader's reads are knowable here,
// and a slot left to the injection's bindings could hold anything). Its cost: per recorded draw two more D3D calls
// (VSGetShaderResources 128, VSGetSamplers 16), PSGetShaderResources 16 -> 128 slots and IAGetVertexBuffers 4 -> 32, an
// AddRef for every bound object in them (stale binds too) and its Release at the record's release (a record is 4,096 B
// against 1,568 without it; its release walks 355 pointer slots against 71); per replayed draw (replay B, A after a
// fallback, the mirror's draws twice) two more Set calls and the PS shader-resource / vertex buffer Sets at 128 / 32
// slots; per replay two more Get and Set pairs (KhRpSave). A rewritable buffer bound, even stale, in vertex slots 4 -
// 31 is copied at the pass's end and split on a write, as one in slots 0 - 3.
// KH_RP_BIND_HW: the replay's Sets stop at the highest bound slot. A record keeps, per array, the count up to its last
// non-null slot (vb_n / vsrv_n / psrv_n, from the full Get - the Get stays whole: which slots are bound is not knowable
// before it); a replay loop sets max(this record's count, the previous record's) slots from the record's array, whose
// entries past its count are null. The pipeline's slots then equal the record's whole array after every Set exactly
// as the full Set left them: the first draw of a loop sets every slot (the loop starts from the caller's unknown
// state), and from then on a slot past both counts was already null. Nothing inside either loop binds a slot between
// two records (the seams run between kh_rp_issue calls); the loops' locals start full for that reason.
static constexpr uint32_t KH_RP_CB = 14u, KH_RP_VB = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
static constexpr uint32_t KH_RP_SRV = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
static constexpr uint32_t KH_RP_SMP = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
static constexpr uint32_t KH_RP_VPS = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
// KH_RP_BIND_HW: the count up to an array's last non-null entry (0 = none bound).
template <typename T>
inline uint32_t kh_rp_bound_n(T* const* khbn_a, uint32_t khbn_n) {
    uint32_t khbn_r = khbn_n;
    while (khbn_r > 0u && khbn_a[khbn_r - 1u] == nullptr) --khbn_r;
    return khbn_r;
}
struct KhRpDraw {
    uint8_t kind = 0;
    bool mir = false;   // KH_MIR_REPLAY: the real mirror re-issued this draw (g_vmir_pending, set just before it).
    UINT a = 0, b = 0, c = 0, e = 0;
    INT d = 0;
    ID3D11InputLayout* il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* vb[KH_RP_VB] = {};
    UINT vbs[KH_RP_VB] = {}, vbo[KH_RP_VB] = {};
    bool vb_dyn[KH_RP_VB] = {};   // Still the engine's rewritable buffer: copied at the pass's end.
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ifmt = DXGI_FORMAT_UNKNOWN;
    UINT ioff = 0;
    bool ib_dyn = false;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    bool cb1 = false;   // The windows below are real (captured through context 1).
    ID3D11Buffer* vcb[KH_RP_CB] = {};
    UINT vcf[KH_RP_CB] = {}, vcn[KH_RP_CB] = {};
    ID3D11Buffer* pcb[KH_RP_CB] = {};
    UINT pcf[KH_RP_CB] = {}, pcn[KH_RP_CB] = {};
    ID3D11ShaderResourceView* vsrv[KH_RP_SRV] = {};   // KH_RP_BIND_ALL.
    ID3D11SamplerState* vsmp[KH_RP_SMP] = {};
    ID3D11ShaderResourceView* psrv[KH_RP_SRV] = {};
    ID3D11SamplerState* psmp[KH_RP_SMP] = {};
    uint8_t vb_n = 0, vsrv_n = 0, psrv_n = 0;   // KH_RP_BIND_HW: slots up to the last bound one (<= 32 / 128 / 128).
    ID3D11DepthStencilState* dss = nullptr;
    UINT sref = 0;
    ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* bs = nullptr;
    FLOAT bf[4] = {};
    UINT bmask = 0xFFFFFFFFu;
    UINT nvp = 0, nsc = 0;
    D3D11_VIEWPORT vp[KH_RP_VPS] = {};
    D3D11_RECT sc[KH_RP_VPS] = {};
    void release() {
        KH_SAFE_RELEASE(il); KH_SAFE_RELEASE(ib); KH_SAFE_RELEASE(vs); KH_SAFE_RELEASE(ps);
        for (auto& x : vb) KH_SAFE_RELEASE(x);
        for (auto& x : vcb) KH_SAFE_RELEASE(x);
        for (auto& x : pcb) KH_SAFE_RELEASE(x);
        for (auto& x : vsrv) KH_SAFE_RELEASE(x);   // KH_RP_BIND_ALL.
        for (auto& x : vsmp) KH_SAFE_RELEASE(x);
        for (auto& x : psrv) KH_SAFE_RELEASE(x);
        for (auto& x : psmp) KH_SAFE_RELEASE(x);
        KH_SAFE_RELEASE(dss); KH_SAFE_RELEASE(rs); KH_SAFE_RELEASE(bs);
        // KH_RP_RELEASE_LEAN: no reset of the rest - every pointer is nulled above, and kh_rp_record writes every
        // other field a reader takes before the record is read (the windows only under cb1, the viewports / rects
        // only up to their counts); a record a throw cut short refuses its pass, whose records are only released.
    }
};
// getRenderStats "stencilReplay": [merged frames, fallback frames, refused passes, ring wraps captured, scissored
// frames, full-screen frames, mirror merged, mirror fallback, mirror idle (KH_MIR_REPLAY: merged frames whose pass drew
// the mirror - the mirror merged too, could not, or no pass that cycle had a mesh reading it), uncovered passes
// (KH_REPLAY_SCISSOR: a pass on a merged cycle that read the copy, an object of its outside the replay's rectangle),
// constant snapshots copied on the GPU, constant snapshots taken from the CPU (KH_REPLAY_CPUCB), write splits
// (KH_RP_WRITE_SPLIT: a buffer copied ahead of an engine write other than a DISCARD map), passes refused at the write
// split cap (KH_RP_SPLIT_CAP, counted in the refused passes too)].
struct KhRpCounters {
    uint64_t merged = 0, fallback = 0, refused = 0, splits = 0, scissored = 0, fullscreen = 0;
    uint64_t mir_merged = 0, mir_fallback = 0, mir_idle = 0, uncovered = 0;
    uint64_t cb_gpu = 0, cb_cpu = 0;
    uint64_t wsplits = 0, capped = 0;
};
static KhRpCounters g_rp_c;
static std::vector<KhRpDraw> g_rp_draws;   // KH_RP_UNCAPPED: [0, g_rp_n) this pass's; the rest released, kept.
static uint32_t g_rp_n = 0;
static uint64_t g_rp_pass = ~0ull;      // The g_svs_frame_seq of the pass being / last recorded.
static bool     g_rp_ok = false, g_rp_ended = false, g_rp_replayed = false;
static ID3D11Texture2D*          g_rp_pre = nullptr;    // The buffer at the pass's first draw (replay A's start).
static ID3D11Texture2D*          g_rp_tgt = nullptr;    // Replay A.
static ID3D11DepthStencilView*   g_rp_dsv = nullptr;
static ID3D11ShaderResourceView* g_rp_sten = nullptr;
static uint64_t g_rp_eng_seq = ~0ull;                   // The pass whose engine snapshot replay B's target holds.
static uint64_t g_rp_vol_clear_seq = ~0ull;             // The frame the engine last cleared the buffer in.
// Replay B. Its start is the buffer at the world seam's start - the engine's depth only - which kh_rp_snap_engine
// copies into it there; nothing else writes it until that pass's replay.
static ID3D11Texture2D*          g_rp_tgtb = nullptr;
static ID3D11DepthStencilView*   g_rp_dsvb = nullptr;
static ID3D11ShaderResourceView* g_rp_stenb = nullptr;
static ID3D11Texture2D*          g_rp_mst_tex = nullptr;   // The merged pair.
static ID3D11RenderTargetView*   g_rp_mst_rtv = nullptr;
static ID3D11Texture2D*          g_rp_mft_tex = nullptr;
static ID3D11RenderTargetView*   g_rp_mft_rtv = nullptr;
static uint32_t g_rp_w = 0, g_rp_h = 0;
// KH_RP_CB_EXACT: the GPU constant snapshots, pooled by the engine buffer's exact width - a snapshot is a buffer of
// the engine's own size bound with the engine's own window, so a read past the engine buffer's end reads past the
// snapshot's too (0, as on the engine's - a wider pooled buffer would hold another snapshot's bytes there). [0, used)
// this pass's; pass = the last pass (g_rp_pass) that took one, pruned with KH_RP_DYN_PRUNE's rule (kh_rp_pass_end).
struct KhRpCbCls { UINT bytes = 0; uint32_t used = 0; uint64_t pass = ~0ull; std::vector<ID3D11Buffer*> pool; };
static std::vector<KhRpCbCls> g_rp_cbpool;
// KH_REPLAY_CBREUSE / KH_REPLAY_SPLIT: an engine object's address -> V, by open addressing (linear probing; erase
// shifts the run back, so no tombstones), at most half full; the power-of-two capacity is kept across clears, so a
// warmed-up pass allocates nothing. A key must stay pinned while listed - a reference held by the entry or by a record
// (KH_HOOK_DESC_CACHE's rule).
template <typename V> struct KhRpPtrMap {
    struct E { const void* k; V v; };
    std::vector<E> t;
    uint32_t n = 0, bits = 0;
    static uint32_t home_of(const void* k, uint32_t b) {
        const uint64_t h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(k)) * 0x9E3779B97F4A7C15ull;
        return static_cast<uint32_t>(h >> (64u - b));
    }
    uint32_t home(const void* k) const { return home_of(k, bits); }
    V* find(const void* k) {
        if (n == 0u || !k) return nullptr;
        const uint32_t m = static_cast<uint32_t>(t.size()) - 1u;
        for (uint32_t i = home(k);; i = (i + 1u) & m) {
            if (t[i].k == k) return &t[i].v;
            if (!t[i].k) return nullptr;
        }
    }
    // k -> v when k is absent (true); the map unchanged when it is present (false).
    bool put(const void* k, const V& v) {
        if (!k) return false;
        if ((n + 1u) * 2u > t.size()) {   // Grow (64 slots first), re-placing every entry.
            // The new table is built whole before it replaces the old one: an allocation that throws leaves the map
            // as it was (the callers catch it; the entries are plain, so nothing after the allocation can throw).
            const uint32_t nb = bits ? bits + 1u : 6u;
            std::vector<E> nt(static_cast<size_t>(1) << nb, E{ nullptr, V{} });
            const uint32_t nm = static_cast<uint32_t>(nt.size()) - 1u;
            for (const E& e : t) {
                if (!e.k) continue;
                uint32_t i = home_of(e.k, nb);
                while (nt[i].k) i = (i + 1u) & nm;
                nt[i] = e;
            }
            t.swap(nt);
            bits = nb;
        }
        const uint32_t m = static_cast<uint32_t>(t.size()) - 1u;
        for (uint32_t i = home(k);; i = (i + 1u) & m) {
            if (t[i].k == k) return false;
            if (!t[i].k) { t[i] = E{ k, v }; ++n; return true; }
        }
    }
    // Removes k (true, its value to khpe_out) or finds nothing (false).
    bool erase(const void* k, V* khpe_out = nullptr) {
        if (n == 0u || !k) return false;
        const uint32_t m = static_cast<uint32_t>(t.size()) - 1u;
        uint32_t i = home(k);
        while (t[i].k != k) {
            if (!t[i].k) return false;
            i = (i + 1u) & m;
        }
        if (khpe_out) *khpe_out = t[i].v;
        for (;;) {   // The hole at i: pull back the first later entry of the run that may sit there.
            t[i] = E{ nullptr, V{} };
            uint32_t j = i;
            for (;;) {
                j = (j + 1u) & m;
                if (!t[j].k) { --n; return true; }
                const uint32_t h = home(t[j].k);
                const bool stays = i <= j ? (h > i && h <= j) : (h > i || h <= j);   // Its home is in (i, j].
                if (!stays) { t[i] = t[j]; i = j; break; }
            }
        }
    }
    template <typename F> void clear(F each) {
        if (n == 0u) return;
        for (E& e : t) {
            if (!e.k) continue;
            each(e.v);
            e = E{ nullptr, V{} };
        }
        n = 0;
    }
    void clear() { clear([](V&) {}); }
    void free() { std::vector<E>().swap(t); n = 0; bits = 0; }   // The storage too (clear it first).
    // Entries, count and storage traded with o whole (KH_RP_UNLOCKED_RELEASE).
    void swap(KhRpPtrMap& o) { t.swap(o.t); std::swap(n, o.n); std::swap(bits, o.bits); }
};
// KH_REPLAY_SPLIT: the rewritable vertex / index buffers a record of the open pass still flags (vb_dyn / ib_dyn) - the
// buffers an engine write splits the pass at (kh_rp_split). Added by kh_rp_record, removed by the split that
// re-points them, cleared with the records and at the pass's end; the records' references pin the keys.
static KhRpPtrMap<ID3D11Buffer*> g_rp_dynset;
// KH_REPLAY_CBREUSE: this pass's constant snapshots by engine buffer - each copied whole (kh_rp_cb_capture), so the
// buffer alone is the key. A later draw binding a buffer the engine has not written since takes the same copy - its
// contents are those of that earlier draw. Every write path forgets the buffer (kh_rp_cb_forget); reuse runs only
// while every writer the context has is hooked (g_rp_cbreuse_on, set where the hooks are installed). Kept for one pass
// (cleared at its start and end). Each entry holds a reference to its engine buffer, so the address cannot be recycled
// under the key while it is listed: a buffer created at a released one's address gets its contents from CreateBuffer,
// which no hook sees, and would take the dead buffer's copy (KH_HOOK_DESC_CACHE's rule). The copies are the pool's.
// Writes can come from any thread, so the map is locked; the count lets a Map outside a pass pay one atomic load.
// A record's constant snapshot: buf is a GPU copy (slot_c 0: the slot keeps its recorded window) or a page of CPU
// images (KH_REPLAY_CPUCB: the image at constant base_c, slot_c constants long, the slot's window moved into it).
struct KhRpSnap { ID3D11Buffer* buf = nullptr; UINT base_c = 0, slot_c = 0; };
struct KhRpCbKeep { ID3D11Buffer* eng; KhRpSnap sn; };
static KhRpPtrMap<KhRpCbKeep> g_rp_cbkeep;
static std::mutex g_rp_cbkeep_mx;
static std::atomic<uint32_t> g_rp_cbkeep_n{ 0 };   // Entries of both maps (the reuse's and the images').
// KH_XTHREAD_ATOMIC: written by kh_rp_extra_hooks (game thread, ensure_reorder_hook), read by the render thread.
static std::atomic<bool> g_rp_cbreuse_on{ false };
// KH_REPLAY_CPUCB - the recorder's constants from the CPU. Copying a rewritable constant buffer on the GPU between the
// engine's volume draws cost that pass ~3 us a copy (~1.25 ms a frame at ~390 copies); ~92% of them were of a buffer
// the engine uploaded while the volume buffer was bound. The upload funnel reads every such upload before the runtime
// sees its Unmap (or from UpdateSubresource's own data), so while the volume buffer is bound it keeps a whole image of
// the buffer (kh_rp_img_note): exactly what the GPU reads, since nothing can write the buffer between that read and a
// draw without kh_rp_cb_forget dropping the image first - every writer hook calls it, a Map included (a Map the funnel
// does not capture leaves no image), and a command list drops them all; an update made while a predicate is bound,
// which D3D may skip, leaves none either (KH_RP_IMG_PRED). A draw's snapshot then copies the image into a page of our
// own (kh_rp_img_slot) instead of a GPU copy, and the replay uploads the pages with one Map each (kh_rp_img_upload).
// The slot is bound through a context-1 window moved into the page and cut at the image's end, which is zero-padded to
// a 16-constant boundary: every read inside the buffer sees its bytes, every read past it 0, as on the engine's buffer.
// Only with offsetting (g_cb_offsetting), every writer hooked (g_rp_cbreuse_on) and a window the recorder read
// (KhRpDraw::cb1, a nonzero count); anything else is copied on the GPU. Images pin their buffers and are dropped at the
// pass's end, at the main depth clear, and with the session and the device.
struct KhRpImg { ID3D11Buffer* eng; uint32_t off, bytes; };
static KhRpPtrMap<KhRpImg> g_rp_img;            // Under g_rp_cbkeep_mx, as the arena.
static std::vector<uint8_t> g_rp_img_arena;
static constexpr uint32_t KH_RP_PAGE = 65536u;   // One page: 4096 constants, the largest window.
static std::vector<ID3D11Buffer*> g_rp_pages;    // DYNAMIC pages (render thread, as the stage).
static std::vector<uint8_t> g_rp_stage;          // Their contents, page after page.
static uint32_t g_rp_stage_used = 0;             // Bytes this pass; a slot never straddles a page.
inline void kh_rp_cb_count() {   // The caller holds g_rp_cbkeep_mx.
    g_rp_cbkeep_n.store(g_rp_cbkeep.n + g_rp_img.n, std::memory_order_release);
}
// KH_MAP_BRANCH: true when it took g_rp_cbkeep_mx (hooked_map counts it; the other callers ignore it).
inline bool kh_rp_cb_forget(ID3D11Resource* khcf_r) {
    if (!khcf_r || g_rp_cbkeep_n.load(std::memory_order_acquire) == 0u) return false;
    std::lock_guard<std::mutex> khcf_l(g_rp_cbkeep_mx);
    if (kh_stats_on()) kh_fun_any(KHF_RP_LOCKS, 1u);   // KH_MAP_BRANCH: one relaxed load, on the locked path only.
    KhRpCbKeep khcf_e = {};
    if (g_rp_cbkeep.erase(static_cast<const void*>(khcf_r), &khcf_e)) {
        khcf_e.eng->Release();   // The entry's reference; the writer holds its own, so never the last.
    }
    KhRpImg khcf_i = {};
    if (g_rp_img.erase(static_cast<const void*>(khcf_r), &khcf_i)) khcf_i.eng->Release();   // KH_REPLAY_CPUCB.
    kh_rp_cb_count();
    return true;
}
// KH_RP_UNLOCKED_RELEASE: one of the two locked maps emptied, its pins released after g_rp_cbkeep_mx is let go. A pin
// can be its buffer's last reference (the engine may drop a buffer while it is listed), and a final Release enters
// the runtime's destruction - and any layer wrapping it - which must not run under this mutex (KH_RP_ONE_LOCK: no
// D3D call under it). The entries leave the map under the lock, with khrf_also (the images' arena); their storage
// comes back once they are released unless the map was put into meanwhile, so a warmed-up pass allocates nothing.
template <typename V, typename KhRfA> inline void kh_rp_keep_release(KhRpPtrMap<V>& khrf_m, KhRfA&& khrf_also) {
    KhRpPtrMap<V> khrf_out;
    {
        std::lock_guard<std::mutex> khrf_l(g_rp_cbkeep_mx);
        if (khrf_m.n != 0u) khrf_m.swap(khrf_out);   // The map is left empty, with no storage until the return.
        khrf_also();
        kh_rp_cb_count();
    }
    if (khrf_out.n == 0u) return;
    khrf_out.clear([](V& e) { e.eng->Release(); });   // The entries' references (may be the last): unlocked.
    std::lock_guard<std::mutex> khrf_l(g_rp_cbkeep_mx);
    if (khrf_m.t.empty()) khrf_m.swap(khrf_out);   // Nothing put since: the emptied storage goes back.
}
inline void kh_rp_cb_keep_clear() { kh_rp_keep_release(g_rp_cbkeep, [] {}); }
// KH_REPLAY_CPUCB: every image dropped (the pass's end, the main depth clear, a command list, the resets).
inline void kh_rp_img_clear() { kh_rp_keep_release(g_rp_img, [] { g_rp_img_arena.clear(); }); }
// KH_REPLAY_CPUCB: the funnel's upload of res (khin_d, khin_n bytes: the buffer's width unless it is past the
// scratch), while the volume buffer is bound - the whole buffer as the GPU will read it. Render thread.
// KH_RP_IMG_PRED: khin_pctx (UpdateSubresource's context; null for an Unmap, which no predicate skips) - while it has
// a predicate bound, D3D may skip the update, so no image is noted: the draws' snapshots then copy the buffer on the
// GPU, behind the update or its skip (KH_PRED_ASIDE), as it stands.
inline void kh_rp_img_note(ID3D11Resource* khin_r, const void* khin_d, uint32_t khin_n,
                           ID3D11DeviceContext* khin_pctx = nullptr) {
    if (!g_svs_vol_dsv_bound || !g_rp_cbreuse_on.load(std::memory_order_relaxed) || !g_cb_offsetting || !khin_r ||
        !khin_d || khin_n == 0) return;
    if (kh_shadows_off()) return;   // KH_SHADOW_OFF: nothing replays them.
    // KH_RP_IMG_IDLE: this frame's pass ended or refused - no draw snapshots an image before the main depth clear
    // drops them all and moves the epoch (one pass a frame); an upload ahead of the pass's first draw is still noted.
    if (g_rp_pass == g_svs_frame_seq && (g_rp_ended || !g_rp_ok)) return;
    if (proj_upload_byte_width(khin_r) != khin_n) return;   // Whole buffers only.
    if (khin_pctx) {   // KH_RP_IMG_PRED: one GetPredication per captured update that reached here.
        ID3D11Predicate* khin_p = nullptr;
        BOOL khin_pv = FALSE;
        khin_pctx->GetPredication(&khin_p, &khin_pv);
        const bool khin_pred = khin_p != nullptr;
        KH_SAFE_RELEASE(khin_p);
        if (khin_pred) return;
    }
    kh_upload_need(khin_d, khin_n);   // KH_UPLOAD_LAZY: all of it.
    std::lock_guard<std::mutex> khin_l(g_rp_cbkeep_mx);
    const uint32_t khin_off = static_cast<uint32_t>(g_rp_img_arena.size());
    g_rp_img_arena.insert(g_rp_img_arena.end(), static_cast<const uint8_t*>(khin_d),
                          static_cast<const uint8_t*>(khin_d) + khin_n);
    ID3D11Buffer* const khin_b = static_cast<ID3D11Buffer*>(khin_r);
    if (KhRpImg* const khin_e = g_rp_img.find(khin_r)) {
        khin_e->off = khin_off;
        khin_e->bytes = khin_n;
    } else if (g_rp_img.put(khin_r, KhRpImg{ khin_b, khin_off, khin_n })) {
        khin_b->AddRef();   // The image's pin (kh_rp_cb_forget / kh_rp_img_clear release it).
    }
    kh_rp_cb_count();
}
// KH_REPLAY_CPUCB: eng's image (bytes long) into this pass's pages - khis_sn names the page and the slot. False =
// no image of that width, or no page could be made (the caller copies on the GPU). Render thread, with
// g_rp_cbkeep_mx held by the caller in khis_l (KH_RP_ONE_LOCK) and held again on return; released only while a
// page is made, so that no D3D call runs under it (the pages and the staging are the render thread's own).
inline bool kh_rp_img_slot(ID3D11DeviceContext* ctx, ID3D11Buffer* eng, UINT bytes, KhRpSnap& khis_sn,
                           std::unique_lock<std::mutex>& khis_l) {
    const UINT khis_c = ((bytes + 255u) / 256u) * 16u;   // Constants, a multiple of 16.
    const uint32_t khis_b = khis_c * 16u;
    if (khis_b > KH_RP_PAGE || bytes == 0) return false;
    uint32_t khis_at = g_rp_stage_used;
    if (khis_at % KH_RP_PAGE + khis_b > KH_RP_PAGE) khis_at = (khis_at / KH_RP_PAGE + 1u) * KH_RP_PAGE;
    const uint32_t khis_p = khis_at / KH_RP_PAGE;
    if (khis_p >= g_rp_pages.size()) {
        g_rp_pages.reserve(g_rp_pages.size() + 1u);   // KH_RP_THROW: a throw here owns nothing yet.
        ID3D11Buffer* khis_nb = nullptr;
        khis_l.unlock();   // KH_RP_ONE_LOCK: no D3D call under the lock (a wrapping layer may take its own).
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (dev) {
            D3D11_BUFFER_DESC d = {};
            d.ByteWidth = KH_RP_PAGE;
            d.Usage = D3D11_USAGE_DYNAMIC;
            d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(dev->CreateBuffer(&d, nullptr, &khis_nb))) khis_nb = nullptr;
            dev->Release();
        }
        khis_l.lock();
        if (!khis_nb) return false;
        g_rp_pages.push_back(khis_nb);   // Within the reserve: cannot throw.
        if (khis_p >= g_rp_pages.size()) return false;
    }
    if (g_rp_stage.size() < static_cast<size_t>(khis_p + 1u) * KH_RP_PAGE) {
        g_rp_stage.resize(static_cast<size_t>(khis_p + 1u) * KH_RP_PAGE);
    }
    {   // The caller's lock (KH_RP_ONE_LOCK).
        const KhRpImg* const khis_i = g_rp_img.find(eng);
        if (!khis_i || khis_i->bytes != bytes) return false;
        memcpy(g_rp_stage.data() + khis_at, g_rp_img_arena.data() + khis_i->off, bytes);
    }
    memset(g_rp_stage.data() + khis_at + bytes, 0, khis_b - bytes);   // Past the buffer: 0, as the engine's reads.
    khis_sn.buf = g_rp_pages[khis_p];
    khis_sn.base_c = (khis_at % KH_RP_PAGE) / 16u;
    khis_sn.slot_c = khis_c;
    g_rp_stage_used = khis_at + khis_b;
    return true;
}
// KH_REPLAY_CPUCB: this pass's pages, one Map each, before anything replays them. Render thread, in the injection.
inline bool kh_rp_img_upload(ID3D11DeviceContext* ctx) {
    const uint32_t khiu_n = (g_rp_stage_used + KH_RP_PAGE - 1u) / KH_RP_PAGE;
    for (uint32_t p = 0; p < khiu_n; ++p) {
        if (p >= g_rp_pages.size() || !g_rp_pages[p]) return false;
        D3D11_MAPPED_SUBRESOURCE m = {};
        if (FAILED(ctx->Map(g_rp_pages[p], 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) return false;
        const uint32_t khiu_b = g_rp_stage_used - p * KH_RP_PAGE < KH_RP_PAGE ? g_rp_stage_used - p * KH_RP_PAGE
                                                                             : KH_RP_PAGE;
        memcpy(m.pData, g_rp_stage.data() + static_cast<size_t>(p) * KH_RP_PAGE, khiu_b);
        ctx->Unmap(g_rp_pages[p], 0);
    }
    return true;
}
struct KhRpDyn { ID3D11Buffer* ours = nullptr; UINT bytes = 0, bind = 0; uintptr_t eng = 0; uint64_t pass = ~0ull; };
static std::vector<KhRpDyn> g_rp_dyn;                    // Pass-end copies of the rewritable vertex / index buffers.
static constexpr uint64_t KH_RP_DYN_KEEP = 1024;          // KH_RP_DYN_PRUNE: frames (g_rp_pass) a copy outlives use.
// KH_REPLAY_SPLIT copies. KH_RP_SPLIT_PRUNE: an entry is taken by a split of its size and bind flags that no split of
// this pass holds (pass = the last pass, g_rp_pass, that took it), else one is made - a size the pool lacks is
// added, never swapped in for another's (ring sizes that alternate keep theirs); an entry no pass has taken for more
// than KH_RP_DYN_KEEP frames goes at a pass's end. Held: per size, the most splits one pass of those frames made.
struct KhRpSplit { ID3D11Buffer* b = nullptr; UINT bytes = 0, bind = 0; uint64_t pass = ~0ull; };
static std::vector<KhRpSplit> g_rp_split_pool;
// KH_RP_SPLIT_CAP: a write split (KH_RP_WRITE_SPLIT; a DISCARD map's split - a ring's wrap - is not capped) costs a
// whole-buffer GPU copy, a pooled buffer of that size held for KH_RP_DYN_KEEP frames and a walk of the records; an
// engine that rewrote a vertex buffer before every volume draw would pay that per draw. The pass is refused at the
// write split that would pass KH_RP_WSPLIT_MAX splits or KH_RP_WSPLIT_BYTES copied (the frame keeps the copy, as every
// refusal): 64 rewrites of buffers the pass's draws read is far past a pass that mends a few, and 64 MB of copies
// (~0.15 ms at a 3080 Ti's ~900 GB/s, read and written; as much pooled VRAM) bounds the worst pass. A write anywhere in
// a flagged buffer splits, whether or not a recorded draw read the region it writes, so the cap can refuse a pass whose
// replay needed no split (a flagged buffer over 64 MB at its first write): stencilReplay counts those refusals. This
// pass's count and bytes, reset with the records.
static constexpr uint32_t KH_RP_WSPLIT_MAX = 64u;
static constexpr uint64_t KH_RP_WSPLIT_BYTES = 64ull << 20;
static uint32_t g_rp_wsplit_n = 0;
static uint64_t g_rp_wsplit_bytes = 0;
// The in-front footprint: where in the pass the hands seam came (the draw it preceded) and its slice viewport.
static uint64_t       g_rp_front_seq = ~0ull;
static uint32_t       g_rp_front_idx = 0;
static D3D11_VIEWPORT g_rp_front_vp = {};
// KH_REPLAY_SCISSOR: scissor-enabled twins of the rasterizer states the replay binds.
struct KhRpRs { ID3D11RasterizerState* eng = nullptr; ID3D11RasterizerState* ours = nullptr; };
static std::vector<KhRpRs> g_rp_rs;
// KH_MIR_REPLAY: depth-clamp twins of the rasterizer states the mirror replay binds ([0] as the engine's, [1] with
// the scissor on).
struct KhRpRsMir { ID3D11RasterizerState* eng = nullptr; ID3D11RasterizerState* ours[2] = {}; };
static std::vector<KhRpRsMir> g_rp_rs_mir;
// KH_RP_DESC_CACHE: a buffer's description, fixed at its creation - what kh_rp_is_mutable answers, asked once per
// buffer per pass instead of once per bind. Pinned (a pinned address cannot be a recycled one); cleared with the
// records. Render thread, like every caller of kh_rp_is_mutable.
struct KhRpBufDesc { ID3D11Buffer* b; UINT bytes; UINT bind; bool mut; };
static KhRpPtrMap<KhRpBufDesc> g_rp_desc;
inline void kh_rp_desc_clear() { g_rp_desc.clear([](KhRpBufDesc& khdc_e) { khdc_e.b->Release(); }); }
inline void kh_rp_release_records() {
    for (uint32_t i = 0; i < g_rp_n; ++i) g_rp_draws[i].release();
    g_rp_n = 0;
    g_rp_dynset.clear();   // KH_REPLAY_SPLIT: no record flags a buffer now.
    kh_rp_cb_keep_clear();   // KH_REPLAY_CBREUSE: before the pool it names is reused.
    for (auto& c : g_rp_cbpool) c.used = 0;   // KH_RP_CB_EXACT.
    g_rp_wsplit_n = 0;   // KH_RP_SPLIT_CAP.
    g_rp_wsplit_bytes = 0;
    g_rp_stage_used = 0;   // KH_REPLAY_CPUCB: the pages are this pass's again (the images are not the records').
    kh_rp_desc_clear();   // KH_RP_DESC_CACHE.
}
inline void kh_rp_release_if_held() { if (g_rp_n != 0) kh_rp_release_records(); }   // KH_SHADOW_OFF.
// KH_RP_DEAD_RELEASE: at the main depth clear, after the epoch moved - the records of a pass nothing can replay any
// more let their engine objects go, and the reuse map they listed empties (left listed, it would keep every engine Map
// on the locked path): a pass that never reached its bracket (kh_rp_pass_end needs its own epoch, kh_rp_replay an ended
// pass) or one already replayed (KH_REPLAY_ONCE; its cycle ends at this clear, and kh_rp_mir_finish needs that
// cycle). An ended pass not yet replayed is kept - the injection after this clear replays it.
inline void kh_rp_release_dead() {
    if (g_rp_n != 0 && (!g_rp_ended || g_rp_replayed)) kh_rp_release_records();
}
inline void kh_rp_release_all() {   // Device loss and session teardown.
    kh_rp_release_records();
    std::vector<KhRpDraw>().swap(g_rp_draws);   // KH_RP_UNCAPPED: the storage too (every record released above).
    g_rp_dynset.free();
    g_rp_desc.free();   // KH_RP_DESC_CACHE: emptied by kh_rp_release_records.
    kh_rp_img_clear();   // KH_REPLAY_CPUCB.
    {
        std::lock_guard<std::mutex> khra_l(g_rp_cbkeep_mx);
        g_rp_cbkeep.free();   // Emptied by kh_rp_release_records.
        g_rp_img.free();
        std::vector<uint8_t>().swap(g_rp_img_arena);
    }
    for (auto& b : g_rp_pages) KH_SAFE_RELEASE(b);
    g_rp_pages.clear();
    std::vector<uint8_t>().swap(g_rp_stage);
    KH_SAFE_RELEASE(g_rp_sten); KH_SAFE_RELEASE(g_rp_dsv); KH_SAFE_RELEASE(g_rp_tgt); KH_SAFE_RELEASE(g_rp_pre);
    KH_SAFE_RELEASE(g_rp_stenb); KH_SAFE_RELEASE(g_rp_dsvb); KH_SAFE_RELEASE(g_rp_tgtb);
    KH_SAFE_RELEASE(g_rp_msten_srv); KH_SAFE_RELEASE(g_rp_mst_rtv); KH_SAFE_RELEASE(g_rp_mst_tex);
    KH_SAFE_RELEASE(g_rp_mfoot_srv); KH_SAFE_RELEASE(g_rp_mft_rtv); KH_SAFE_RELEASE(g_rp_mft_tex);
    g_rp_w = g_rp_h = 0;
    for (auto& cls : g_rp_cbpool) { for (auto& b : cls.pool) KH_SAFE_RELEASE(b); }   // KH_RP_CB_EXACT.
    g_rp_cbpool.clear();
    for (auto& d : g_rp_dyn) KH_SAFE_RELEASE(d.ours);
    g_rp_dyn.clear();
    for (auto& x : g_rp_split_pool) KH_SAFE_RELEASE(x.b);
    g_rp_split_pool.clear();
    for (auto& x : g_rp_rs) { KH_SAFE_RELEASE(x.eng); KH_SAFE_RELEASE(x.ours); }
    g_rp_rs.clear();
    kh_rp_mir_release();   // KH_MIR_REPLAY.
    for (auto& x : g_rp_rs_mir) { KH_SAFE_RELEASE(x.eng); KH_SAFE_RELEASE(x.ours[0]); KH_SAFE_RELEASE(x.ours[1]); }
    g_rp_rs_mir.clear();
    g_rp_pass = ~0ull;
    g_rp_ok = g_rp_ended = g_rp_replayed = false;
    g_rp_merge_ok = false;
    g_rp_eng_seq = ~0ull;
    g_rp_vol_clear_seq = ~0ull;
    g_rp_front_seq = ~0ull;
}
inline bool kh_rp_ensure_tex(ID3D11Device* dev) {
    if (!dev || !g_svs_vol_src) return false;
    D3D11_TEXTURE2D_DESC sd = {};
    g_svs_vol_src->GetDesc(&sd);
    if (g_rp_pre && g_rp_tgt && g_rp_dsv && g_rp_sten && g_rp_tgtb && g_rp_dsvb && g_rp_stenb && g_rp_mst_rtv &&
        g_rp_msten_srv && g_rp_mft_rtv && g_rp_mfoot_srv && g_rp_w == sd.Width && g_rp_h == sd.Height) return true;
    KH_SAFE_RELEASE(g_rp_sten); KH_SAFE_RELEASE(g_rp_dsv); KH_SAFE_RELEASE(g_rp_tgt); KH_SAFE_RELEASE(g_rp_pre);
    KH_SAFE_RELEASE(g_rp_stenb); KH_SAFE_RELEASE(g_rp_dsvb); KH_SAFE_RELEASE(g_rp_tgtb);
    KH_SAFE_RELEASE(g_rp_msten_srv); KH_SAFE_RELEASE(g_rp_mst_rtv); KH_SAFE_RELEASE(g_rp_mst_tex);
    KH_SAFE_RELEASE(g_rp_mfoot_srv); KH_SAFE_RELEASE(g_rp_mft_rtv); KH_SAFE_RELEASE(g_rp_mft_tex);
    g_rp_merge_ok = false;
    if (sd.SampleDesc.Count != 1 || sd.ArraySize != 1 || sd.MipLevels != 1) return false;
    if (sd.Format != DXGI_FORMAT_D24_UNORM_S8_UINT && sd.Format != DXGI_FORMAT_R24G8_TYPELESS) return false;
    D3D11_TEXTURE2D_DESC td = sd;
    td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
    vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    vd.Texture2D.MipLevels = 1;
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_pre))) { g_rp_pre = nullptr; return false; }
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_tgt))) { g_rp_tgt = nullptr; return false; }
    if (FAILED(dev->CreateDepthStencilView(g_rp_tgt, &dd, &g_rp_dsv))) { g_rp_dsv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_tgt, &vd, &g_rp_sten))) { g_rp_sten = nullptr; return false; }
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_tgtb))) { g_rp_tgtb = nullptr; return false; }
    if (FAILED(dev->CreateDepthStencilView(g_rp_tgtb, &dd, &g_rp_dsvb))) { g_rp_dsvb = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_tgtb, &vd, &g_rp_stenb))) { g_rp_stenb = nullptr; return false; }
    D3D11_TEXTURE2D_DESC md = {};
    md.Width = sd.Width; md.Height = sd.Height; md.MipLevels = 1; md.ArraySize = 1; md.SampleDesc.Count = 1;
    md.Usage = D3D11_USAGE_DEFAULT;
    md.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    md.Format = DXGI_FORMAT_R8G8_UINT;
    if (FAILED(dev->CreateTexture2D(&md, nullptr, &g_rp_mst_tex))) { g_rp_mst_tex = nullptr; return false; }
    if (FAILED(dev->CreateRenderTargetView(g_rp_mst_tex, nullptr, &g_rp_mst_rtv))) { g_rp_mst_rtv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_mst_tex, nullptr, &g_rp_msten_srv))) { g_rp_msten_srv = nullptr; return false; }
    md.Format = DXGI_FORMAT_R32_FLOAT;
    if (FAILED(dev->CreateTexture2D(&md, nullptr, &g_rp_mft_tex))) { g_rp_mft_tex = nullptr; return false; }
    if (FAILED(dev->CreateRenderTargetView(g_rp_mft_tex, nullptr, &g_rp_mft_rtv))) { g_rp_mft_rtv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_mft_tex, nullptr, &g_rp_mfoot_srv))) { g_rp_mfoot_srv = nullptr; return false; }
    g_rp_w = sd.Width;
    g_rp_h = sd.Height;
    return true;
}
// Any buffer the engine can still write (DYNAMIC or DEFAULT): the replay copies it rather than referencing it.
inline bool kh_rp_is_mutable(ID3D11Buffer* b, UINT* bytes = nullptr, UINT* bind = nullptr) {
    if (!b) return false;
    if (const KhRpBufDesc* const khim_e = g_rp_desc.find(b)) {   // KH_RP_DESC_CACHE.
        if (bytes) *bytes = khim_e->bytes;
        if (bind) *bind = khim_e->bind;
        return khim_e->mut;
    }
    D3D11_BUFFER_DESC d = {};
    b->GetDesc(&d);
    if (bytes) *bytes = d.ByteWidth;
    if (bind) *bind = d.BindFlags;
    const bool khim_mut = d.Usage != D3D11_USAGE_IMMUTABLE;
    try {   // A failed insert only means no cache: the answer above stands.
        if (g_rp_desc.put(b, KhRpBufDesc{ b, d.ByteWidth, d.BindFlags, khim_mut })) b->AddRef();
    } catch (...) {}
    return khim_mut;
}
// A copy of a constant buffer (bytes long) as it stands now, whole, into a pooled buffer of the same width
// (KH_RP_CB_EXACT).
inline ID3D11Buffer* kh_rp_cb_snapshot(ID3D11DeviceContext* ctx, ID3D11Buffer* eng, UINT bytes) {
    KhRpCbCls* cls = nullptr;
    for (KhRpCbCls& x : g_rp_cbpool) {
        if (x.bytes == bytes) { cls = &x; break; }
    }
    if (!cls) {
        g_rp_cbpool.push_back(KhRpCbCls());
        cls = &g_rp_cbpool.back();
        cls->bytes = bytes;
    }
    cls->pass = g_rp_pass;
    ID3D11Buffer* ours = cls->used < cls->pool.size() ? cls->pool[cls->used] : nullptr;
    if (!ours) {
        cls->pool.reserve(cls->pool.size() + 1u);   // KH_RP_THROW: a throw here owns nothing yet.
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return nullptr;
        D3D11_BUFFER_DESC d = {};
        d.ByteWidth = bytes;
        d.Usage = D3D11_USAGE_DEFAULT;
        d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        const HRESULT hr = dev->CreateBuffer(&d, nullptr, &ours);
        dev->Release();
        if (FAILED(hr)) return nullptr;
        cls->pool.push_back(ours);   // Within the reserve: cannot throw.
    }
    ++cls->used;
    // KH_RP_SNAP_OWN: our copy - the copy hook takes it as ours (kh_rp_engine_call false, no PIP target probe).
    const bool khcs_prev = g_ro.in_injection;
    g_ro.in_injection = true;
    {
        KhPredAside khcs_pa;   // KH_PRED_ASIDE: not skipped by a predicate the engine left bound at its draw.
        khcs_pa.begin(ctx);
        ctx->CopyResource(ours, eng);
        khcs_pa.end();
    }
    g_ro.in_injection = khcs_prev;
    ours->AddRef();   // The record's own reference; the pool keeps its.
    return ours;
}
// The record's rewritable constants: each buffer copied whole (once per draw, or once per pass while the engine does
// not write it - KH_REPLAY_CBREUSE); every slot bound to it keeps its window. Do not copy windows only: they mismatch
// more than whole buffers. False = a copy could not be made (that slot is left unbound).
inline bool kh_rp_cb_capture(ID3D11DeviceContext* ctx, KhRpDraw& r) {
    // KH_RP_ONE_LOCK: one hold of g_rp_cbkeep_mx across the slots, the same steps in the same order, released only
    // around a GPU copy - kh_rp_cb_snapshot copies through the hooked context, whose copy hook takes this mutex
    // (kh_rp_cb_forget) - and around a page's creation (kh_rp_img_slot: no D3D call under it). Taken when first needed;
    // the unique_lock releases it on every exit.
    std::unique_lock<std::mutex> khcr_l(g_rp_cbkeep_mx, std::defer_lock);
    // KH_RP_REUSE_ONCE: read once - the image branch below may run only after the reuse lookup took the lock, and an
    // install round's store between two reads would break that (an unlock of a lock not held throws).
    const bool khcr_reuse = g_rp_cbreuse_on.load(std::memory_order_relaxed);
    // Every slot's type first, outside the lock: kh_rp_is_mutable asks D3D on a cache miss. The same answers the
    // loop would get - no slot's buffer changes before its own turn.
    bool khcr_mut[2][KH_RP_CB];
    UINT khcr_bytes[2][KH_RP_CB];
    for (uint32_t st = 0; st < 2u; ++st) {
        for (uint32_t k = 0; k < KH_RP_CB; ++k) {
            khcr_bytes[st][k] = 0;
            khcr_mut[st][k] = kh_rp_is_mutable(st ? r.pcb[k] : r.vcb[k], &khcr_bytes[st][k]);
        }
    }
    struct KhCbSeen { ID3D11Buffer* eng; KhRpSnap sn; };
    KhCbSeen seen[2u * KH_RP_CB];
    uint32_t nseen = 0;
    bool ok = true;
    for (uint32_t st = 0; st < 2u; ++st) {
        ID3D11Buffer** set = st ? r.pcb : r.vcb;
        UINT* const fst = st ? r.pcf : r.vcf;
        UINT* const num = st ? r.pcn : r.vcn;
        for (uint32_t k = 0; k < KH_RP_CB; ++k) {
            const UINT bytes = khcr_bytes[st][k];
            if (!khcr_mut[st][k]) continue;
            const bool win = r.cb1 && num[k] != 0;   // KH_REPLAY_CPUCB: this slot can take a page window.
            KhRpSnap sn = {};
            bool listed = false;   // Already in seen.
            for (uint32_t q = 0; q < nseen && !sn.buf; ++q) {
                if (seen[q].eng == set[k] && (win || seen[q].sn.slot_c == 0)) { sn = seen[q].sn; listed = true; }
            }
            // KH_REPLAY_CBREUSE: an earlier draw's, the buffer unwritten since.
            if (!sn.buf && khcr_reuse) {
                if (!khcr_l.owns_lock()) khcr_l.lock();
                const KhRpCbKeep* const e = g_rp_cbkeep.find(set[k]);
                if (e && (win || e->sn.slot_c == 0)) sn = e->sn;
            }
            bool fresh = false;   // Made for this draw: the reuse keeps it.
            if (sn.buf) {
                sn.buf->AddRef();   // The record's reference.
            } else if (win && khcr_reuse && g_cb_offsetting &&
                       kh_rp_img_slot(ctx, set[k], bytes, sn, khcr_l)) {
                // KH_RP_ONE_LOCK: held - this branch runs only after the reuse lookup above, which took it.
                sn.buf->AddRef();   // KH_REPLAY_CPUCB: the record's reference (the pages keep theirs).
                fresh = true;
                kh_stat(g_rp_c.cb_cpu);
            } else {
                sn = KhRpSnap{};
                if (khcr_l.owns_lock()) khcr_l.unlock();   // KH_RP_ONE_LOCK: its copy enters the hooks.
                sn.buf = kh_rp_cb_snapshot(ctx, set[k], bytes);   // With the record's reference.
                if (!sn.buf) {
                    ok = false;
                    set[k]->Release();
                    set[k] = nullptr;
                    continue;
                }
                fresh = true;
                kh_stat(g_rp_c.cb_gpu);
            }
            if (!listed && nseen < 2u * KH_RP_CB) seen[nseen++] = KhCbSeen{ set[k], sn };
            if (fresh && khcr_reuse) {   // KH_REPLAY_CBREUSE.
                if (!khcr_l.owns_lock()) khcr_l.lock();
                try {   // KH_RP_THROW: a failed insert (its growth) lists nothing - a later draw copies afresh.
                    if (g_rp_cbkeep.put(set[k], KhRpCbKeep{ set[k], sn })) {
                        set[k]->AddRef();   // The entry's own (released by kh_rp_cb_forget / kh_rp_cb_keep_clear).
                    }
                } catch (...) {}
                kh_rp_cb_count();
            }
            ID3D11Buffer* use = sn.buf;
            if (sn.slot_c != 0) {   // KH_REPLAY_CPUCB: the recorded window, moved into the page and cut at the image.
                if (fst[k] >= sn.slot_c) {   // Wholly past the buffer: every read 0, as unbound.
                    use->Release();
                    use = nullptr;
                    fst[k] = 0;
                    num[k] = 0;
                } else {
                    const UINT rem = sn.slot_c - fst[k];
                    fst[k] += sn.base_c;
                    if (num[k] > rem) num[k] = rem;
                }
            }
            set[k]->Release();
            set[k] = use;
        }
    }
    return ok;
}
// The first counting draw of a pass. The bound DSV must be the volume buffer. Replay A's start is snapshotted only on a
// pass the world seam did not withhold (a withheld pass is replayed as B alone).
inline void kh_rp_pass_begin(ID3D11DeviceContext* ctx) {
    kh_rp_release_records();
    g_rp_pass = g_svs_frame_seq;
    g_rp_ok = true;
    g_rp_ended = false;
    g_rp_replayed = false;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    void* const id = dsv ? reorder_dsv_identity(dsv) : nullptr;
    KH_SAFE_RELEASE(dsv);
    if (!g_svs_vol_src || !id || id != g_svs_vol_src_id) { g_rp_ok = false; kh_stat(g_rp_c.refused); return; }
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = kh_rp_ensure_tex(dev);
    if (dev) dev->Release();
    if (!ok) { g_rp_ok = false; kh_stat(g_rp_c.refused); return; }
    if (g_rp_withheld_seq == g_rp_pass) return;
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    KhOmSave om;
    om.capture(ctx);
    KhPredAside pa;   // KH_PRED_ASIDE: a draw hook's copy - not skipped by a predicate the engine left bound.
    pa.begin(ctx);
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->CopyResource(g_rp_pre, g_svs_vol_src);
    pa.end();
    om.restore(ctx);
    om.release();
    g_ro.in_injection = prev;
}
// The volume buffer as the engine left it for this frame's count, before our world seam's footprint: replay B's start,
// copied into its target (g_rp_tgtb).
inline void kh_rp_snap_engine(ID3D11DeviceContext* ctx) {
    try {
        if (!ctx || !g_svs_vol_src) return;
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        const bool ok = kh_rp_ensure_tex(dev);
        if (dev) dev->Release();
        if (!ok) return;
        const bool prev = g_ro.in_injection;
        g_ro.in_injection = true;
        KhOmSave om;
        om.capture(ctx);
        KhPredAside pa;   // KH_PRED_ASIDE: ahead of the seam's StateBackup - the copy is not skipped by a predicate.
        pa.begin(ctx);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        ctx->CopyResource(g_rp_tgtb, g_svs_vol_src);
        pa.end();
        om.restore(ctx);
        om.release();
        g_ro.in_injection = prev;
        g_rp_eng_seq = g_svs_frame_seq;
    } catch (...) {}
}
// One engine draw with the volume buffer bound, after our own pre-draw work (the seams have run).
inline void kh_rp_record(ID3D11DeviceContext* ctx, uint8_t kind, UINT a, UINT b, UINT c, INT d, UINT e) {
    try {
        if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !reorder_on_render_thread()) return;
        if (g_ro.in_injection) return;
        const bool open = g_rp_pass == g_svs_frame_seq && !g_rp_ended;
        if (!g_svs_vol_dsv_now) {   // Bound beside a target: recorded inside a pass (depth and stencil on replay).
            if (!g_svs_vol_dsv_bound || !open) return;   // Never starts a pass.
        } else if (g_rp_pass != g_svs_frame_seq) {   // The frame's first counting draw (one pass a frame).
            kh_rp_pass_begin(ctx);
        }
        if (!g_rp_ok || g_rp_ended) return;
        ID3D11GeometryShader* gs = nullptr; ctx->GSGetShader(&gs, nullptr, nullptr);
        ID3D11HullShader* hs = nullptr; ctx->HSGetShader(&hs, nullptr, nullptr);
        ID3D11DomainShader* ds = nullptr; ctx->DSGetShader(&ds, nullptr, nullptr);
        const bool stages = gs || hs || ds;
        KH_SAFE_RELEASE(gs); KH_SAFE_RELEASE(hs); KH_SAFE_RELEASE(ds);
        if (stages) { g_rp_ok = false; kh_stat(g_rp_c.refused); return; }
        if (g_rp_n >= g_rp_draws.size()) {   // KH_RP_UNCAPPED: doubling (a throw refuses the pass: the catch below).
            g_rp_draws.resize(g_rp_draws.size() < 64u ? 64u : g_rp_draws.size() * 2u);
        }
        KhRpDraw& r = g_rp_draws[g_rp_n++];
        r.kind = kind; r.a = a; r.b = b; r.c = c; r.d = d; r.e = e;
        r.mir = g_vmir_pending;   // KH_MIR_REPLAY.
        ctx->IAGetInputLayout(&r.il);
        ctx->IAGetPrimitiveTopology(&r.topo);
        ctx->IAGetVertexBuffers(0, KH_RP_VB, r.vb, r.vbs, r.vbo);
        for (uint32_t k = 0; k < KH_RP_VB; ++k) r.vb_dyn[k] = kh_rp_is_mutable(r.vb[k]);
        ctx->IAGetIndexBuffer(&r.ib, &r.ifmt, &r.ioff);
        r.ib_dyn = kh_rp_is_mutable(r.ib);
        for (uint32_t k = 0; k < KH_RP_VB; ++k) {   // KH_REPLAY_SPLIT.
            if (r.vb_dyn[k]) g_rp_dynset.put(r.vb[k], r.vb[k]);
        }
        if (r.ib_dyn) g_rp_dynset.put(r.ib, r.ib);
        ctx->VSGetShader(&r.vs, nullptr, nullptr);
        ctx->PSGetShader(&r.ps, nullptr, nullptr);
        ID3D11DeviceContext1* const c1 = g_cb_offsetting ? kh_ctx1(ctx) : nullptr;
        r.cb1 = c1 != nullptr;
        if (c1) {
            c1->VSGetConstantBuffers1(0, KH_RP_CB, r.vcb, r.vcf, r.vcn);
            c1->PSGetConstantBuffers1(0, KH_RP_CB, r.pcb, r.pcf, r.pcn);
        } else {
            ctx->VSGetConstantBuffers(0, KH_RP_CB, r.vcb);
            ctx->PSGetConstantBuffers(0, KH_RP_CB, r.pcb);
        }
        if (!kh_rp_cb_capture(ctx, r)) { g_rp_ok = false; kh_stat(g_rp_c.refused); }   // Rewritable constants.
        ctx->VSGetShaderResources(0, KH_RP_SRV, r.vsrv);   // KH_RP_BIND_ALL.
        ctx->VSGetSamplers(0, KH_RP_SMP, r.vsmp);
        ctx->PSGetShaderResources(0, KH_RP_SRV, r.psrv);
        ctx->PSGetSamplers(0, KH_RP_SMP, r.psmp);
        r.vb_n = static_cast<uint8_t>(kh_rp_bound_n(r.vb, KH_RP_VB));   // KH_RP_BIND_HW.
        r.vsrv_n = static_cast<uint8_t>(kh_rp_bound_n(r.vsrv, KH_RP_SRV));
        r.psrv_n = static_cast<uint8_t>(kh_rp_bound_n(r.psrv, KH_RP_SRV));
        ctx->OMGetDepthStencilState(&r.dss, &r.sref);
        ctx->RSGetState(&r.rs);
        ctx->OMGetBlendState(&r.bs, r.bf, &r.bmask);
        r.nvp = KH_RP_VPS; ctx->RSGetViewports(&r.nvp, r.vp);
        r.nsc = KH_RP_VPS; ctx->RSGetScissorRects(&r.nsc, r.sc);
    } catch (...) {
        if (g_rp_ok) kh_stat(g_rp_c.refused);   // KH_RP_THROW: counted once, as every refusal.
        g_rp_ok = false;
    }
}
// Anything else writing the volume buffer inside a pass: the replay could not reproduce it. (0 a clear, 2 our world
// footprint, 4 a command list, 5 an indirect draw or DrawAuto, 6 a copy into the buffer. Our in-front footprint is not
// one: replay A lacks it, so the merge keeps the copy only where it changed the count - the in-front call site,
// reorder_pre_draw.)
inline void kh_rp_foreign(int khrf_kind) {
    (void)khrf_kind;
    if (g_rp_pass != g_svs_frame_seq || !g_rp_ok || g_rp_ended || g_rp_n == 0) return;
    g_rp_ok = false;
    kh_stat(g_rp_c.refused);
}
// The hands seam's place in the pass (the in-front call site, before the draw it precedes is recorded).
inline void kh_rp_front_note(const D3D11_VIEWPORT& khfn_vp) {
    g_rp_front_seq = g_svs_frame_seq;
    g_rp_front_idx = (g_rp_pass == g_svs_frame_seq && !g_rp_ended) ? g_rp_n : 0u;
    g_rp_front_vp = khfn_vp;
}
// KH_RP_WRITE_SPLIT: a split may run - the engine's own call on the target context's render thread, inside this
// frame's open, unrefused pass with a draw recorded. A few loads and compares; the lookup after it is one probe of an
// empty map when no record flags a buffer.
inline bool kh_rp_split_armed(ID3D11DeviceContext* self) {
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed) || !reorder_on_render_thread() ||
        g_ro.in_injection) return false;
    return g_rp_pass == g_svs_frame_seq && !g_rp_ended && g_rp_ok && g_rp_n != 0;
}
// KH_REPLAY_SPLIT: res is about to be written by the engine, from a hook ahead of the runtime's call, so the copy
// queued here reads the contents the draws recorded so far used. When a record still flags it (g_rp_dynset), it is
// copied into a pool buffer and every such record reads the copy (no pass-end copy for them); a later draw that binds
// it lists it again. khsp_wrap: a DISCARD map, counted as a ring wrap, else a write split, capped (getRenderStats
// "stencilReplay"). KH_MAP_BRANCH:
// 0 = stood down before the lookup, 1 = looked the buffer up (on the render thread) and did not split it, 2 = split it.
inline int kh_rp_split(ID3D11DeviceContext* self, ID3D11Resource* res, bool khsp_wrap) {
    if (!res || !kh_rp_split_armed(self)) return 0;
    // KH_RP_UNCAPPED: a lookup in the set of buffers a record still flags, not a walk of the records - this runs on
    // every DISCARD map and engine write inside the pass (one per draw or more), so a walk would make the pass
    // quadratic in its draws.
    ID3D11Buffer* const* const khom_e = g_rp_dynset.find(res);
    if (!khom_e) return 1;
    ID3D11Buffer* const eng = *khom_e;
    UINT bytes = 0, bind = 0;
    kh_rp_is_mutable(eng, &bytes, &bind);
    if (!khsp_wrap) {   // KH_RP_SPLIT_CAP: past the cap the pass is refused, nothing copied.
        if (g_rp_wsplit_n >= KH_RP_WSPLIT_MAX || g_rp_wsplit_bytes + bytes > KH_RP_WSPLIT_BYTES) {
            g_rp_ok = false;
            kh_stat(g_rp_c.refused);
            kh_stat(g_rp_c.capped);
            return 1;
        }
        ++g_rp_wsplit_n;
        g_rp_wsplit_bytes += bytes;
    }
    KhRpSplit* sp = nullptr;   // KH_RP_SPLIT_PRUNE: a free entry of this size and bind, else a new one.
    for (KhRpSplit& x : g_rp_split_pool) {
        if (x.pass != g_rp_pass && x.bytes == bytes && x.bind == bind) { sp = &x; break; }
    }
    if (!sp) {
        g_rp_split_pool.reserve(g_rp_split_pool.size() + 1u);   // KH_RP_THROW: a throw here owns nothing yet.
        ID3D11Buffer* khsp_b = nullptr;
        ID3D11Device* dev = nullptr;
        self->GetDevice(&dev);
        D3D11_BUFFER_DESC d = {};
        d.ByteWidth = bytes;
        d.Usage = D3D11_USAGE_DEFAULT;
        d.BindFlags = bind & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER);
        if (!dev || FAILED(dev->CreateBuffer(&d, nullptr, &khsp_b))) khsp_b = nullptr;
        if (dev) dev->Release();
        if (!khsp_b) { g_rp_ok = false; kh_stat(g_rp_c.refused); return 1; }
        KhRpSplit khsp_n;
        khsp_n.b = khsp_b;
        khsp_n.bytes = bytes;
        khsp_n.bind = bind;
        g_rp_split_pool.push_back(khsp_n);   // Within the reserve: cannot throw.
        sp = &g_rp_split_pool.back();
    }
    sp->pass = g_rp_pass;
    ID3D11Buffer* const khsp_c = sp->b;
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    {
        KhPredAside khsp_pa;   // KH_PRED_ASIDE: what the draws so far replay from - not skipped by a predicate.
        khsp_pa.begin(self);
        self->CopyResource(khsp_c, eng);
        khsp_pa.end();
    }
    g_ro.in_injection = prev;
    for (uint32_t i = 0; i < g_rp_n; ++i) {   // Every draw so far that used it reads the copy (no pass-end copy for them).
        KhRpDraw& r = g_rp_draws[i];
        for (uint32_t k = 0; k < KH_RP_VB; ++k) {
            if (r.vb_dyn[k] && r.vb[k] == eng) {
                r.vb[k]->Release(); khsp_c->AddRef(); r.vb[k] = khsp_c; r.vb_dyn[k] = false;
            }
        }
        if (r.ib_dyn && r.ib == eng) { r.ib->Release(); khsp_c->AddRef(); r.ib = khsp_c; r.ib_dyn = false; }
    }
    g_rp_dynset.erase(eng);   // No record flags it now; a later draw that binds it adds it again.
    kh_stat(khsp_wrap ? g_rp_c.splits : g_rp_c.wsplits);   // KH_RP_SPLIT_CAP: write splits apart.
    return 2;
}
// KH_REPLAY_SPLIT: from the Map hook - a DISCARD map (a ring's wrap) splits; a NO_OVERWRITE map leaves, by its
// contract, the bytes the draws since the last split read, so the pass-end copy still serves them.
inline int kh_rp_on_map(ID3D11DeviceContext* self, ID3D11Resource* res, D3D11_MAP type) {
    if (type != D3D11_MAP_WRITE_DISCARD) return 0;
    return kh_rp_split(self, res, true);
}
// KH_RP_WRITE_SPLIT: from the hooks of the engine's other writers into a buffer (UpdateSubresource(1), CopyResource,
// CopySubresourceRegion(1), CopyStructureCount, and a UAV bound at the output merger - hooked_omset_rts_and_uavs;
// khwr_nooverwrite: a D3D11_COPY_NO_OVERWRITE copy or update, which keeps the NO_OVERWRITE map's contract and does not
// split). A split that threw (its pool's growth) refuses the pass, as hooked_map's does - left armed, the draws would
// keep reading the buffer the write changes.
inline void kh_rp_on_write(ID3D11DeviceContext* self, ID3D11Resource* res, bool khwr_nooverwrite) {
    if (khwr_nooverwrite) return;
    try {
        (void)kh_rp_split(self, res, false);
    } catch (...) {
        if (g_rp_ok) kh_stat(g_rp_c.refused);   // KH_RP_THROW: counted once, as every refusal.
        g_rp_ok = false;
    }
}
// The stencil resolve (the bracket): the rewritable vertex / index buffers the pass still references, copied once.
inline void kh_rp_pass_end(ID3D11DeviceContext* ctx) {
    if (g_rp_pass != g_svs_frame_seq || g_rp_ended) return;
    g_rp_ended = true;
    kh_rp_cb_keep_clear();   // KH_REPLAY_CBREUSE: the pass is recorded.
    kh_rp_img_clear();   // KH_REPLAY_CPUCB: its images are in its pages.
    g_rp_dynset.clear();   // KH_REPLAY_SPLIT: likewise (kh_rp_split stands down on an ended pass).
    // KH_RP_SPLIT_PRUNE / KH_RP_CB_EXACT: KH_RP_DYN_PRUNE's rule at every pass's end (a refused one too), after the
    // reuse map that names pool buffers is cleared; a record still holding a released one keeps it (its reference).
    for (size_t khpp_i = 0; khpp_i < g_rp_split_pool.size();) {
        KhRpSplit& khpp_s = g_rp_split_pool[khpp_i];
        if (khpp_s.pass != g_rp_pass && (khpp_s.pass == ~0ull || g_rp_pass - khpp_s.pass > KH_RP_DYN_KEEP)) {
            KH_SAFE_RELEASE(khpp_s.b);
            khpp_s = g_rp_split_pool.back();
            g_rp_split_pool.pop_back();
        } else {
            ++khpp_i;
        }
    }
    for (size_t khpp_i = 0; khpp_i < g_rp_cbpool.size();) {
        KhRpCbCls& khpp_c = g_rp_cbpool[khpp_i];
        if (khpp_c.pass != g_rp_pass && (khpp_c.pass == ~0ull || g_rp_pass - khpp_c.pass > KH_RP_DYN_KEEP)) {
            for (auto& b : khpp_c.pool) KH_SAFE_RELEASE(b);
            if (&khpp_c != &g_rp_cbpool.back()) khpp_c = std::move(g_rp_cbpool.back());
            g_rp_cbpool.pop_back();
        } else {
            ++khpp_i;
        }
    }
    if (!g_rp_ok || g_rp_n == 0) return;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) { g_rp_ok = false; return; }
    // KH_PRED_ASIDE: the copies below are the replay's geometry (kh_svs_vol_copy's bracket, its rule) - not skipped by
    // a predicate the engine left bound at this draw.
    KhPredAside khpe_pa;
    khpe_pa.begin(ctx);
    auto ours_for = [&](ID3D11Buffer* eng) -> ID3D11Buffer* {
        UINT bytes = 0, bind = 0;
        kh_rp_is_mutable(eng, &bytes, &bind);
        const uintptr_t id = reinterpret_cast<uintptr_t>(eng);
        KhRpDyn* slot = nullptr;
        for (auto& x : g_rp_dyn) if (x.eng == id) { slot = &x; break; }
        if (!slot) { g_rp_dyn.push_back(KhRpDyn()); slot = &g_rp_dyn.back(); slot->eng = id; }
        if (!slot->ours || slot->bytes != bytes || slot->bind != bind) {
            KH_SAFE_RELEASE(slot->ours);
            D3D11_BUFFER_DESC d = {};
            d.ByteWidth = bytes;
            d.Usage = D3D11_USAGE_DEFAULT;
            d.BindFlags = bind & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER);
            if (FAILED(dev->CreateBuffer(&d, nullptr, &slot->ours))) { slot->ours = nullptr; return nullptr; }
            slot->bytes = bytes;
            slot->bind = bind;
        }
        if (slot->pass != g_rp_pass) {
            ctx->CopyResource(slot->ours, eng);
            slot->pass = g_rp_pass;
        }
        return slot->ours;
    };
    try {   // KH_RP_THROW: a slot's growth that throws refuses the pass (its records half re-pointed) and lets dev go.
        for (uint32_t i = 0; i < g_rp_n && g_rp_ok; ++i) {
            KhRpDraw& r = g_rp_draws[i];
            for (uint32_t k = 0; k < KH_RP_VB; ++k) {
                if (!r.vb_dyn[k] || !r.vb[k]) continue;
                ID3D11Buffer* o = ours_for(r.vb[k]);
                if (!o) { g_rp_ok = false; kh_stat(g_rp_c.refused); break; }
                r.vb[k]->Release(); o->AddRef(); r.vb[k] = o;
            }
            if (r.ib_dyn && r.ib && g_rp_ok) {
                ID3D11Buffer* o = ours_for(r.ib);
                if (!o) { g_rp_ok = false; kh_stat(g_rp_c.refused); }
                else { r.ib->Release(); o->AddRef(); r.ib = o; }
            }
        }
    } catch (...) {
        g_rp_ok = false;
        kh_stat(g_rp_c.refused);
    }
    // KH_RP_DYN_PRUNE: a copy no pass has named for more than KH_RP_DYN_KEEP frames goes (g_rp_pass is the frame
    // sequence, g_svs_frame_seq, bumped at every main-depth clear), at the end of a pass that reaches the copies
    // above; a pass refused before its end, or with no draw, returns ahead of them and adds no slot either.
    // The slots are keyed by an engine
    // buffer's address, which the engine frees and reuses, so without this every buffer a volume pass ever drew with
    // would keep a copy of its size for the session. A record still holding one keeps it alive (its own reference).
    for (size_t khpd_i = 0; khpd_i < g_rp_dyn.size();) {
        KhRpDyn& khpd_d = g_rp_dyn[khpd_i];
        if (khpd_d.pass != g_rp_pass && (khpd_d.pass == ~0ull || g_rp_pass - khpd_d.pass > KH_RP_DYN_KEEP)) {
            KH_SAFE_RELEASE(khpd_d.ours);
            khpd_d = g_rp_dyn.back();
            g_rp_dyn.pop_back();
        } else {
            ++khpd_i;
        }
    }
    dev->Release();
}
// KH_RP_UNWIND: runs khou_f only when its scope is left by an exception - every normal exit does the same work
// itself, so the normal path is unchanged (KH_UI_BB_UNWIND's rule). khou_f must not throw.
template <typename KhF>
struct KhRpOnUnwind {
    KhF khou_f;
    const int khou_ex;
    explicit KhRpOnUnwind(KhF khou_v) : khou_f(khou_v), khou_ex(std::uncaught_exceptions()) {}
    ~KhRpOnUnwind() { if (std::uncaught_exceptions() > khou_ex) khou_f(); }
    KhRpOnUnwind(const KhRpOnUnwind&) = delete;
    KhRpOnUnwind& operator=(const KhRpOnUnwind&) = delete;
};
// Everything the replay touches, saved and restored whole (StateBackup covers only our own passes' slots).
// KH_RP_UNWIND: a capture not yet restored is restored by the destructor (a throw inside the replay), and every
// reference released; the normal path's restore() clears armed and its release() nulls every pointer, so the
// destructor then does nothing (KhOmSave's and KhPredAside's own destructors find nothing either).
struct KhRpSave {
    ID3D11InputLayout* il = nullptr; D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* vb[KH_RP_VB] = {}; UINT vbs[KH_RP_VB] = {}, vbo[KH_RP_VB] = {};
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT ifmt = DXGI_FORMAT_UNKNOWN; UINT ioff = 0;
    ID3D11VertexShader* vs = nullptr; ID3D11PixelShader* ps = nullptr;
    ID3D11GeometryShader* gs = nullptr; ID3D11HullShader* hs = nullptr; ID3D11DomainShader* ds = nullptr;
    bool cb1 = false;
    ID3D11Buffer* vcb[KH_RP_CB] = {}; UINT vcf[KH_RP_CB] = {}, vcn[KH_RP_CB] = {};
    ID3D11Buffer* pcb[KH_RP_CB] = {}; UINT pcf[KH_RP_CB] = {}, pcn[KH_RP_CB] = {};
    // KH_RP_BIND_ALL: every slot a replayed draw binds (the merges' t28, t34, t41, t49 - t51 among them).
    ID3D11ShaderResourceView* vsrv[KH_RP_SRV] = {}; ID3D11SamplerState* vsmp[KH_RP_SMP] = {};
    ID3D11ShaderResourceView* psrv[KH_RP_SRV] = {}; ID3D11SamplerState* psmp[KH_RP_SMP] = {};
    ID3D11DepthStencilState* dss = nullptr; UINT sref = 0; ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* bs = nullptr; FLOAT bf[4] = {}; UINT bmask = 0xFFFFFFFFu;
    UINT nvp = 0, nsc = 0; D3D11_VIEWPORT vp[KH_RP_VPS] = {}; D3D11_RECT sc[KH_RP_VPS] = {};
    KhOmSave om;
    // KH_PRED_ASIDE: the engine's predicate, set aside for the replay (its copies, clears and merge draws are ours; a
    // predicate the engine left bound at the injection's trigger draw - not the recorded draws' own, which the
    // recorder never kept - would skip them, leaving the merge targets a cycle stale) and handed back by restore.
    KhPredAside pred;
    ID3D11DeviceContext* ctx_saved = nullptr;   // KH_RP_UNWIND.
    bool armed = false;                         // Captured, not yet restored.
    KhRpSave() = default;
    KhRpSave(const KhRpSave&) = delete;
    KhRpSave& operator=(const KhRpSave&) = delete;
    ~KhRpSave() {
        if (armed && ctx_saved) restore(ctx_saved);
        release();
    }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        pred.begin(ctx);
        ctx->IAGetInputLayout(&il); ctx->IAGetPrimitiveTopology(&topo);
        ctx->IAGetVertexBuffers(0, KH_RP_VB, vb, vbs, vbo); ctx->IAGetIndexBuffer(&ib, &ifmt, &ioff);
        ctx->VSGetShader(&vs, nullptr, nullptr); ctx->PSGetShader(&ps, nullptr, nullptr);
        ctx->GSGetShader(&gs, nullptr, nullptr); ctx->HSGetShader(&hs, nullptr, nullptr); ctx->DSGetShader(&ds, nullptr, nullptr);
        ID3D11DeviceContext1* const c1 = g_cb_offsetting ? kh_ctx1(ctx) : nullptr;
        cb1 = c1 != nullptr;
        if (c1) { c1->VSGetConstantBuffers1(0, KH_RP_CB, vcb, vcf, vcn); c1->PSGetConstantBuffers1(0, KH_RP_CB, pcb, pcf, pcn); }
        else { ctx->VSGetConstantBuffers(0, KH_RP_CB, vcb); ctx->PSGetConstantBuffers(0, KH_RP_CB, pcb); }
        ctx->VSGetShaderResources(0, KH_RP_SRV, vsrv); ctx->VSGetSamplers(0, KH_RP_SMP, vsmp);   // KH_RP_BIND_ALL.
        ctx->PSGetShaderResources(0, KH_RP_SRV, psrv); ctx->PSGetSamplers(0, KH_RP_SMP, psmp);
        ctx->OMGetDepthStencilState(&dss, &sref); ctx->RSGetState(&rs); ctx->OMGetBlendState(&bs, bf, &bmask);
        nvp = KH_RP_VPS; ctx->RSGetViewports(&nvp, vp); nsc = KH_RP_VPS; ctx->RSGetScissorRects(&nsc, sc);
        om.capture(ctx);
    }
    void restore(ID3D11DeviceContext* ctx) {
        armed = false;
        ctx->IASetInputLayout(il); ctx->IASetPrimitiveTopology(topo);
        ctx->IASetVertexBuffers(0, KH_RP_VB, vb, vbs, vbo); ctx->IASetIndexBuffer(ib, ifmt, ioff);
        // KH_CLASS_INST: every shader comes back without class instances (the capture takes none) - the injection's
        // replay runs outside any StateBackup, so an engine shader bound with interfaces would come back not its own.
        // Accepted as StateBackup's GS / HS / DS are: the engine is not known to use dynamic linkage.
        ctx->VSSetShader(vs, nullptr, 0); ctx->PSSetShader(ps, nullptr, 0);
        ctx->GSSetShader(gs, nullptr, 0); ctx->HSSetShader(hs, nullptr, 0); ctx->DSSetShader(ds, nullptr, 0);
        ID3D11DeviceContext1* const c1 = cb1 ? kh_ctx1(ctx) : nullptr;
        if (c1) { c1->VSSetConstantBuffers1(0, KH_RP_CB, vcb, vcf, vcn); c1->PSSetConstantBuffers1(0, KH_RP_CB, pcb, pcf, pcn); }
        else { ctx->VSSetConstantBuffers(0, KH_RP_CB, vcb); ctx->PSSetConstantBuffers(0, KH_RP_CB, pcb); }
        ctx->VSSetShaderResources(0, KH_RP_SRV, vsrv); ctx->VSSetSamplers(0, KH_RP_SMP, vsmp);   // KH_RP_BIND_ALL.
        ctx->PSSetShaderResources(0, KH_RP_SRV, psrv); ctx->PSSetSamplers(0, KH_RP_SMP, psmp);
        kh_bc_forget();   // KH_BIND_FILTER: t0 - t127.
        ctx->OMSetDepthStencilState(dss, sref); ctx->RSSetState(rs); ctx->OMSetBlendState(bs, bf, bmask);
        // KH_RP_SAVE_RS: the counts as captured, zero included - a zero unbinds, so a scissor rect of ours does
        // not stay behind an engine that had none bound (StateBackup's rule).
        ctx->RSSetViewports(nvp, vp);
        ctx->RSSetScissorRects(nsc, sc);
        om.restore(ctx);
        pred.end();   // KH_PRED_ASIDE.
    }
    void release() {
        KH_SAFE_RELEASE(il); KH_SAFE_RELEASE(ib); KH_SAFE_RELEASE(vs); KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(gs); KH_SAFE_RELEASE(hs); KH_SAFE_RELEASE(ds);
        for (auto& x : vb) KH_SAFE_RELEASE(x);
        for (auto& x : vcb) KH_SAFE_RELEASE(x);
        for (auto& x : pcb) KH_SAFE_RELEASE(x);
        for (auto& x : vsrv) KH_SAFE_RELEASE(x);   // KH_RP_BIND_ALL.
        for (auto& x : vsmp) KH_SAFE_RELEASE(x);
        for (auto& x : psrv) KH_SAFE_RELEASE(x);
        for (auto& x : psmp) KH_SAFE_RELEASE(x);
        KH_SAFE_RELEASE(dss); KH_SAFE_RELEASE(rs); KH_SAFE_RELEASE(bs);
        om.release();
    }
};
// KH_REPLAY_SCISSOR: the scissor-enabled twin of a rasterizer state (null = the default state), cached per state.
inline ID3D11RasterizerState* kh_rp_rs_scissor(ID3D11DeviceContext* ctx, ID3D11RasterizerState* eng) {
    for (const auto& x : g_rp_rs) if (x.eng == eng) return x.ours;
    D3D11_RASTERIZER_DESC d = {};
    if (eng) {
        eng->GetDesc(&d);
    } else {
        d.FillMode = D3D11_FILL_SOLID;
        d.CullMode = D3D11_CULL_BACK;
        d.DepthClipEnable = TRUE;
    }
    d.ScissorEnable = TRUE;
    ID3D11RasterizerState* ours = nullptr;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (dev) { if (FAILED(dev->CreateRasterizerState(&d, &ours))) ours = nullptr; dev->Release(); }
    if (!ours) return nullptr;
    try {   // KH_RP_UNWIND: room first - a throwing push_back would leak ours and the engine's reference.
        g_rp_rs.reserve(g_rp_rs.size() + 1u);
    } catch (...) { ours->Release(); throw; }
    if (eng) eng->AddRef();
    g_rp_rs.push_back(KhRpRs{ eng, ours });
    return ours;
}
// KH_MIR_REPLAY: the depth-clamp twin (kh_vmir_begin's clone: DepthClipEnable off) of a rasterizer state, with the
// scissor forced on for khrm_sc, cached per state.
inline ID3D11RasterizerState* kh_rp_rs_mir(ID3D11DeviceContext* ctx, ID3D11RasterizerState* eng, bool khrm_sc) {
    const int k = khrm_sc ? 1 : 0;
    for (const auto& x : g_rp_rs_mir) if (x.eng == eng && x.ours[k]) return x.ours[k];
    D3D11_RASTERIZER_DESC d = {};
    if (eng) {
        eng->GetDesc(&d);
    } else {
        d.FillMode = D3D11_FILL_SOLID;
        d.CullMode = D3D11_CULL_BACK;
        d.DepthClipEnable = TRUE;
    }
    d.DepthClipEnable = FALSE;
    if (khrm_sc) d.ScissorEnable = TRUE;
    ID3D11RasterizerState* ours = nullptr;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (dev) { if (FAILED(dev->CreateRasterizerState(&d, &ours))) ours = nullptr; dev->Release(); }
    if (!ours) return nullptr;
    for (auto& x : g_rp_rs_mir) if (x.eng == eng) { x.ours[k] = ours; return ours; }
    try {   // KH_RP_UNWIND: room first (kh_rp_rs_scissor's rule).
        g_rp_rs_mir.reserve(g_rp_rs_mir.size() + 1u);
    } catch (...) { ours->Release(); throw; }
    KhRpRsMir e;
    e.eng = eng;
    if (eng) eng->AddRef();
    e.ours[k] = ours;
    g_rp_rs_mir.push_back(e);
    return ours;
}
// KH_REPLAY_SCISSOR: the nearest a fragment of ours can be to the camera - ClipOwnNear's floor (cb.hlsl: KH_OWN_NEAR,
// 0.05 m, or a pass's own nearer near, the view-model slice's measured 0.01 m). A bounding sphere is clipped here
// rather than given up on: nothing of it nearer than this is ever drawn.
static constexpr float KH_RP_CLIP_NEAR_M = 0.01f;
// KH_MIR_BAND: a near-z routed mesh reads the mirror through PSComposite's near-plane fade, whose weight is
// saturate((w / near - 1) / (KH_STEN_FADE - 1)): only a fragment of view depth under KH_RP_STEN_FADE x near takes the
// mirror's count into its verdict. Every other fragment takes lerp(mirror, count, 1) - the count, to within one float
// step (an accepted envelope for this gate) - so the mirror replay runs for such a mesh only when one of its fragments
// can lie in that band. HLSL twin KH_STEN_FADE (cb.hlsl).
static constexpr float KH_RP_STEN_FADE = 1.35f;
// The band's padding, relative then absolute: the cycle latch's camera and projection against the drawing pass's
// (the same frame), on top of the bounding sphere's own padding.
static constexpr float KH_RP_MIR_BAND_REL = 1.25f;
static constexpr float KH_RP_MIR_BAND_PAD_M = 0.25f;
// KH_MIR_BAND: the farthest (m, straight line from the camera) a fragment of view depth under KH_RP_STEN_FADE x near
// can lie inside the frustum of the row-vector projection P - the band's depth times the frustum's widest corner ray
// (the jitter offsets included) - padded. Negative when P is unusable: the caller then counts the mesh a reader.
inline float kh_rp_mir_band(const float P[4][4]) {
    const float w = P[2][3];
    if (!(w > 1.0e-6f) || !(P[2][2] > 1.0e-6f)) return -1.0f;
    const float n = -P[3][2] / P[2][2];
    const float ax = fabsf(P[0][0] / w), ay = fabsf(P[1][1] / w);
    if (!(n > 0.0f) || !(ax > 1.0e-6f) || !(ay > 1.0e-6f)) return -1.0f;
    const float tx = (1.0f + fabsf(P[2][0] / w)) / ax, ty = (1.0f + fabsf(P[2][1] / w)) / ay;
    const float r = KH_RP_STEN_FADE * n * sqrtf(1.0f + tx * tx + ty * ty);
    return (r == r && r < 1.0e6f) ? r * KH_RP_MIR_BAND_REL + KH_RP_MIR_BAND_PAD_M : -1.0f;
}
// KH_MIR_BAND: kh_rp_obj_ndc's bounding sphere - its centre (engine axes) and radius (the rectangles' bound).
inline float kh_rp_sphere_of(const RenderObject& o, float khrs_c[3]) {
    float khrs_bs[3];
    kh_bounds_size_of(o, khrs_bs);
    khrs_c[0] = o.pos[0]; khrs_c[1] = o.pos[2]; khrs_c[2] = o.pos[1];   // Engine axes.
    return 1.25f * kh_lod_radius_of(khrs_bs, o.rot_m, o.rotated) + 0.05f;
}
// KH_MIR_BAND: the nearest the mesh's drawn box comes to cam (m, engine axes) - kh_bounds_size_of's size (a cloth's
// reach and a vertex stage's bound included), taken as the world-axis box around its rotation, which holds every
// fragment the mesh can draw. The band test's bound; not the rectangles' sphere (1.25 x the box's half-diagonal): it
// bulges far past a large box's faces and swallows the camera beside a big mesh.
inline float kh_rp_box_dist(const RenderObject& o, const float khbd_cam[3]) {
    return sqrtf(kh_bounds_dist_sq(o, khbd_cam));   // KH_NEARZ_BOUNDS: the one drawn-box distance.
}
// KH_MIR_GATE - the real mirror (the world seam's mirror prepass and the re-issue of the counting draws into it) is
// built at the end of a cycle for the next cycle's passes, so it is built only when some visible mesh can read it
// then: an inFront, fading, translucent or alpha-material mesh (as the readers are always counted), or one whose
// drawn box lies within KH_VMIR_REACH_MUL x the band's reach (KH_MIR_BAND) plus KH_VMIR_MOTION_M and four of
// the camera's last steps - what the camera and a mesh can close before that cycle draws. A cycle that wanted it
// holds it for KH_VMIR_HOLD more. A reader the gate did not foresee (a teleport into a mesh, a mesh shown or moved
// there at once) reads no mirror for that one cycle - its near band loses the stencil shadow's near-plane repair
// for a frame: an accepted envelope. Render thread (the seam).
static constexpr float    KH_VMIR_REACH_MUL = 2.0f;
static constexpr float    KH_VMIR_MOTION_M = 1.0f;
static constexpr uint64_t KH_VMIR_HOLD = 8u;
static uint64_t g_vmir_want_cycle = ~0ull;   // The last cycle that wanted the mirror (~0 = none).
inline bool kh_vmir_wanted(const float khvw_cam[3], const float khvw_proj[4][4]) {
    const float khvw_band = kh_rp_mir_band(khvw_proj);
    bool khvw_want = khvw_band < 0.0f;
    const float khvw_reach = khvw_band * KH_VMIR_REACH_MUL + KH_VMIR_MOTION_M +
                             4.0f * (g_cam_step_m > 0.0f ? g_cam_step_m : 0.0f);
    for (uint32_t i = 0; i < g_scene.objs.size() && !khvw_want; ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!kh_obj_vis(o)) continue;   // KH_SHOW_INFRONT.
        if (kh_in_front(o) || o.timed || o.color[3] < 0.999f || kh_mat_set_has_alpha(o.materials) ||
            kh_rp_box_dist(o, khvw_cam) < khvw_reach) khvw_want = true;
    }
    if (khvw_want) g_vmir_want_cycle = g_topo_cycles;
    return khvw_want || (g_vmir_want_cycle != ~0ull && g_topo_cycles >= g_vmir_want_cycle &&
                         g_topo_cycles - g_vmir_want_cycle <= KH_VMIR_HOLD);
}
// KH_REPLAY_SCISSOR: one object's bound in NDC through the cycle latch - its bounding sphere (radius x1.25 + 5 cm, for
// the witness ring and skinned motion) clipped at KH_RP_CLIP_NEAR_M. 0 = nothing of it can be drawn (behind the camera
// or wholly nearer than that), 1 = khro_b holds [x0, y0, x1, y1], 2 = no bound (an inFront mesh, drawn under its own
// projection). khro_d = the sphere's nearest distance to khro_cam (m, world). The caller has checked the latch.
// KH_REPLAY_RASTER: the latch's view, the screen terms of the replay's raster (the copy's where it parted).
inline int kh_rp_obj_ndc(const RenderObject& o, const float (*khro_v)[4], const float khro_cam[3], float khro_b[4],
                         float& khro_d) {
    khro_d = 0.0f;
    if (kh_in_front(o)) return 2;
    const float (*V)[4] = khro_v;   // KH_RP_ADOPT: the caller's view (kh_rp_view_of).
    const float (*Pm)[4] = g_ro.cycle_pv.projection;
    const bool khro_r = g_rp_rast_on;   // KH_REPLAY_RASTER.
    const float sx = khro_r ? g_rp_rast[0] : Pm[0][0] / Pm[2][3], sy = khro_r ? g_rp_rast[1] : Pm[1][1] / Pm[2][3];
    const float ox = khro_r ? g_rp_rast[2] : Pm[2][0] / Pm[2][3], oy = khro_r ? g_rp_rast[3] : Pm[2][1] / Pm[2][3];
    float c[3];
    const float rad = kh_rp_sphere_of(o, c);   // KH_MIR_BAND: the sphere, shared.
    const float dx = c[0] - khro_cam[0], dy = c[1] - khro_cam[1], dz = c[2] - khro_cam[2];
    khro_d = sqrtf(dx * dx + dy * dy + dz * dz) - rad;
    const float vx = c[0] * V[0][0] + c[1] * V[1][0] + c[2] * V[2][0] + V[3][0];
    const float vy = c[0] * V[0][1] + c[1] * V[1][1] + c[2] * V[2][1] + V[3][1];
    const float vz = c[0] * V[0][2] + c[1] * V[1][2] + c[2] * V[2][2] + V[3][2];
    if (vz + rad <= KH_RP_CLIP_NEAR_M) return 0;
    const float zn = fmaxf(vz - rad, KH_RP_CLIP_NEAR_M), zf = vz + rad;
    const float tx0 = fminf((vx - rad) / zn, (vx - rad) / zf), tx1 = fmaxf((vx + rad) / zn, (vx + rad) / zf);
    const float ty0 = fminf((vy - rad) / zn, (vy - rad) / zf), ty1 = fmaxf((vy + rad) / zn, (vy + rad) / zf);
    khro_b[0] = fminf(sx * tx0, sx * tx1) + ox;
    khro_b[2] = fmaxf(sx * tx0, sx * tx1) + ox;
    khro_b[1] = fminf(sy * ty0, sy * ty1) + oy;
    khro_b[3] = fmaxf(sy * ty0, sy * ty1) + oy;
    return 1;
}
// KH_MIR_BAND_RECT: a reader through its texels anywhere (an inFront mesh, a translucent texel) rather than through
// the near fade's band alone.
inline bool kh_rp_mir_flagged(const RenderObject& o) {
    return kh_in_front(o) || o.timed || o.color[3] < 0.999f || kh_mat_set_has_alpha(o.materials);
}
// KH_MIR_BAND_RECT: where a band-only mirror reader (not kh_rp_mir_flagged) can draw a fragment in the fade's band,
// as an NDC bound - its drawn box (kh_rp_box_dist's: the world-axis box around the rotated, grown size) cut to view
// depth [KH_RP_CLIP_NEAR_M, the band's depth] through the cycle latch, in the replay's raster (kh_rp_obj_ndc's
// terms). The band's depth is KH_RP_STEN_FADE x the latch's near, padded as the band's reach is. The cut box is
// convex and in front of the camera, so its screen bound is that of its corners inside the slab and of its edges'
// crossings of the two planes. 1 = khmb_b holds [x0, y0, x1, y1]; 0 = none of the box lies in the band; -1 = no
// usable projection (the caller takes the whole object's bound). The caller has checked the latch.
inline int kh_rp_mir_band_ndc(const RenderObject& o, const float (*khmb_view)[4], float khmb_b[4]) {
    const float (*V)[4] = khmb_view;   // KH_RP_ADOPT: the caller's view (kh_rp_view_of).
    const float (*Pm)[4] = g_ro.cycle_pv.projection;
    if (!(Pm[2][2] > 1.0e-6f) || !(fabsf(Pm[2][3]) > 1.0e-6f)) return -1;
    const float khmb_n = -Pm[3][2] / Pm[2][2];
    if (!(khmb_n > 0.0f)) return -1;
    const float khmb_zb = KH_RP_STEN_FADE * khmb_n * KH_RP_MIR_BAND_REL + KH_RP_MIR_BAND_PAD_M;
    const float khmb_zl = KH_RP_CLIP_NEAR_M;
    const bool khmb_rs = g_rp_rast_on;   // KH_REPLAY_RASTER: kh_rp_obj_ndc's terms.
    const float sx = khmb_rs ? g_rp_rast[0] : Pm[0][0] / Pm[2][3], sy = khmb_rs ? g_rp_rast[1] : Pm[1][1] / Pm[2][3];
    const float ox = khmb_rs ? g_rp_rast[2] : Pm[2][0] / Pm[2][3], oy = khmb_rs ? g_rp_rast[3] : Pm[2][1] / Pm[2][3];
    float khmb_s[3];
    kh_bounds_size_of(o, khmb_s);
    const float khmb_hl[3] = { khmb_s[0] * 0.5f, khmb_s[2] * 0.5f, khmb_s[1] * 0.5f };   // Engine axes.
    float khmb_he[3];
    kh_rot_half_extents(khmb_hl, o.rot_m, o.rotated, khmb_he);
    const float c[3] = { o.pos[0], o.pos[2], o.pos[1] };
    float khmb_v[8][3];
    for (int k = 0; k < 8; ++k) {
        const float p[3] = { c[0] + ((k & 1) ? khmb_he[0] : -khmb_he[0]), c[1] + ((k & 2) ? khmb_he[1] : -khmb_he[1]),
                             c[2] + ((k & 4) ? khmb_he[2] : -khmb_he[2]) };
        for (int j = 0; j < 3; ++j) khmb_v[k][j] = p[0] * V[0][j] + p[1] * V[1][j] + p[2] * V[2][j] + V[3][j];
    }
    bool khmb_any = false;
    float x0 = 1.0e30f, y0 = 1.0e30f, x1 = -1.0e30f, y1 = -1.0e30f;
    auto khmb_add = [&](float vx, float vy, float vz) {
        const float xn = sx * (vx / vz) + ox, yn = sy * (vy / vz) + oy;
        x0 = fminf(x0, xn); x1 = fmaxf(x1, xn); y0 = fminf(y0, yn); y1 = fmaxf(y1, yn);
        khmb_any = true;
    };
    for (int k = 0; k < 8; ++k) {
        if (khmb_v[k][2] >= khmb_zl && khmb_v[k][2] <= khmb_zb) khmb_add(khmb_v[k][0], khmb_v[k][1], khmb_v[k][2]);
    }
    for (int k = 0; k < 8; ++k) {
        for (int khmb_bit = 1; khmb_bit < 8; khmb_bit <<= 1) {
            if (k & khmb_bit) continue;   // Each of the twelve edges once, from its lower corner.
            const float* va = khmb_v[k];
            const float* vb = khmb_v[k | khmb_bit];
            const float khmb_pl[2] = { khmb_zl, khmb_zb };
            for (float zp : khmb_pl) {
                if ((va[2] - zp) * (vb[2] - zp) >= 0.0f) continue;   // No crossing (or an end on the plane).
                const float khmb_t = (zp - va[2]) / (vb[2] - va[2]);
                khmb_add(va[0] + (vb[0] - va[0]) * khmb_t, va[1] + (vb[1] - va[1]) * khmb_t, zp);
            }
        }
    }
    if (!khmb_any) return 0;
    khmb_b[0] = x0; khmb_b[1] = y0; khmb_b[2] = x1; khmb_b[3] = y1;
    return 1;
}
// KH_MIR_REPLAY: an object that can read the mirror (t28): an inFront mesh (all of it), a translucent texel (an object
// below full alpha, one whose lifetime fades it, or one with an alpha material - PSMain / PSComposite's khStenTl), or
// a near-z routed mesh with a fragment that can lie in the fade's band (KH_MIR_BAND, through the cycle latch's
// projection, its drawn box against khmr_cam - the latch's camera).
inline bool kh_rp_mir_reader(const RenderObject& o, const float khmr_cam[3]) {
    if (kh_rp_mir_flagged(o)) return true;
    const float khmr_r = g_ro.cycle_pv_valid ? kh_rp_mir_band(g_ro.cycle_pv.projection) : -1.0f;
    return khmr_r < 0.0f || kh_rp_box_dist(o, khmr_cam) < khmr_r;
}
// KH_MIR_REPLAY: a mesh that reads the mirror in a pass drawing under khmp_proj from khmp_cam: kh_rp_mir_reader's
// rule made the pass's own. A mesh reads the fade only when the pass routes it near-z - a near in (KH_CAM_NEAR_MIN,
// 50) and its drawn box within near + 2 m (KH_NEARZ_BOUNDS; the passes' other conditions only narrow that) - and
// only when that box comes within the band's reach (KH_MIR_BAND).
inline bool kh_rp_mir_reads(const RenderObject& o, const float khmp_proj[4][4], const float khmp_cam[3]) {
    if (kh_in_front(o) || o.timed || o.color[3] < 0.999f || kh_mat_set_has_alpha(o.materials)) return true;
    const float khmp_near = khmp_proj[2][2] > 1.0e-6f ? -khmp_proj[3][2] / khmp_proj[2][2] : 0.0f;
    if (!(khmp_near > KH_CAM_NEAR_MIN && khmp_near < 50.0f)) return false;
    const float khmp_r = khmp_near + 2.0f;
    const float khmp_d2 = kh_bounds_dist_sq(o, khmp_cam);   // KH_NEARZ_BOUNDS: the route's distance.
    if (!(khmp_d2 < khmp_r * khmp_r)) return false;
    const float khmp_band = kh_rp_mir_band(khmp_proj);
    if (khmp_band < 0.0f) return true;
    return sqrtf(khmp_d2) < khmp_band;   // kh_rp_box_dist's value.
}
// KH_REPLAY_SCISSOR: an NDC bound (x0, y0, x1, y1) as a target rectangle, plus a 16 px margin; empty when nothing of it
// is on the target.
inline D3D11_RECT kh_rp_px_of(float x0, float y0, float x1, float y1) {
    const float W = static_cast<float>(g_rp_w), H = static_cast<float>(g_rp_h), M = 16.0f;
    const float px0 = (x0 * 0.5f + 0.5f) * W - M, px1 = (x1 * 0.5f + 0.5f) * W + M;
    const float py0 = (0.5f - y1 * 0.5f) * H - M, py1 = (0.5f - y0 * 0.5f) * H + M;
    D3D11_RECT r;
    r.left = static_cast<LONG>(fmaxf(0.0f, fminf(W, floorf(px0))));
    r.right = static_cast<LONG>(fmaxf(0.0f, fminf(W, ceilf(px1))));
    r.top = static_cast<LONG>(fmaxf(0.0f, fminf(H, floorf(py0))));
    r.bottom = static_cast<LONG>(fmaxf(0.0f, fminf(H, ceilf(py1))));
    if (r.right <= r.left || r.bottom <= r.top) r = D3D11_RECT{ 0, 0, 0, 0 };
    return r;
}
inline bool kh_rp_px_empty(const D3D11_RECT& r) { return r.right <= r.left || r.bottom <= r.top; }
inline void kh_rp_rect_add(KhRpRect& khra_r, const float khra_b[4]) {
    khra_r.x0 = fminf(khra_r.x0, khra_b[0]); khra_r.y0 = fminf(khra_r.y0, khra_b[1]);
    khra_r.x1 = fmaxf(khra_r.x1, khra_b[2]); khra_r.y1 = fmaxf(khra_r.y1, khra_b[3]);
    khra_r.any = true;
}
inline void kh_rp_rect_close(KhRpRect& khrc_r) {
    if (khrc_r.full) return;
    khrc_r.px = khrc_r.any ? kh_rp_px_of(khrc_r.x0, khrc_r.y0, khrc_r.x1, khrc_r.y1) : D3D11_RECT{ 0, 0, 0, 0 };
    if (kh_rp_px_empty(khrc_r.px)) khrc_r.any = false;
}
// KH_RP_ADOPT (KH_FX_FRAME_ADOPT): the view the rectangles bound our meshes through - the cycle latch under the
// engine's frame rotation when fresh, the frame our meshes are drawn in (the injection and the flush adopt the same
// latch) and replay B's footprint with them. Not the bridge's own rotation: a fast turn puts the meshes' leading
// edge up to a frame of rotation outside the 16 px margin. The camera is the latch's (adoption keeps it).
inline void kh_rp_view_of(float khrv_v[4][4]) {
    RVExtBridge::ProjectionViewTransform khrv_pv = g_ro.cycle_pv;
    kh_adopt_frame_view(khrv_pv);
    memcpy(khrv_v, &khrv_pv.view[0][0], sizeof(float) * 16u);
}
// KH_REPLAY_SCISSOR: the rectangles the replay draws in - every visible mesh of ours (the volume count) and the ones
// that can read the mirror (KH_MIR_REPLAY) - through the cycle latch (KH_RP_ADOPT), in the replay's raster
// (KH_REPLAY_RASTER). Both are the whole target when the latch is not usable or an inFront mesh is visible.
inline void kh_rp_scissor_rects(KhRpRect& khsr_all, KhRpRect& khsr_mir) {
    khsr_all = KhRpRect();
    khsr_mir = KhRpRect();
    if (!g_ro.cycle_pv_valid || g_rp_w == 0 || g_rp_h == 0) return;
    if (!(fabsf(g_ro.cycle_pv.projection[2][3]) > 1.0e-6f)) return;
    float khsr_v[4][4];
    kh_rp_view_of(khsr_v);
    float cam[3];
    kh_cam_of(khsr_v, cam);
    KhRpRect a, m;
    a.full = m.full = false;
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!kh_obj_vis(o)) continue;   // KH_SHOW_INFRONT.
        float b[4], d = 0.0f;
        const int k = kh_rp_obj_ndc(o, khsr_v, cam, b, d);
        if (k == 0) continue;
        if (k == 2) return;   // Both whole.
        kh_rp_rect_add(a, b);
        if (kh_rp_mir_reader(o, cam)) {   // KH_MIR_BAND_RECT: a band-only reader, its band's part alone.
            float mb[4];
            const int mk = kh_rp_mir_flagged(o) ? -1 : kh_rp_mir_band_ndc(o, khsr_v, mb);
            if (mk < 0) kh_rp_rect_add(m, b);
            else if (mk == 1) kh_rp_rect_add(m, mb);
        }
    }
    kh_rp_rect_close(a);
    kh_rp_rect_close(m);
    khsr_all = a;
    khsr_mir = m;
}
// KH_REPLAY_SCISSOR: whether a pass drawing khrc_list may read the merged count (khrc_vol) and the merged mirror
// (khrc_mir): each is this cycle's merge, and every object of the list inside the rectangle the replay drew in for
// it - trivially when the scene is the one the rectangles were made from (its generation unchanged) or a rectangle
// was the whole target; otherwise object by object, through the same bound (a mirror reader inside the mirror's).
inline void kh_rp_covered(const std::vector<RenderObject>& khrc_list, bool& khrc_vol, bool& khrc_mir) {
    khrc_vol = kh_rp_merged_now();
    khrc_mir = kh_rp_mir_merged_now();
    if (!khrc_vol && !khrc_mir) return;
    if (g_scene.gen == g_rp_sc_gen || (g_rp_sc_all.full && g_rp_sc_mir.full)) return;
    if (!g_ro.cycle_pv_valid || !(fabsf(g_ro.cycle_pv.projection[2][3]) > 1.0e-6f)) {   // No bound to test against.
        if (!g_rp_sc_all.full) khrc_vol = false;
        if (!g_rp_sc_mir.full) khrc_mir = false;
    } else {
        float khrc_v[4][4];
        kh_rp_view_of(khrc_v);   // KH_RP_ADOPT: the rectangles' own view.
        float cam[3];
        kh_cam_of(khrc_v, cam);
        auto inside = [](const D3D11_RECT& khin_r, const KhRpRect& khin_in) {
            if (khin_in.full || kh_rp_px_empty(khin_r)) return true;
            return khin_in.any && khin_r.left >= khin_in.px.left && khin_r.top >= khin_in.px.top &&
                   khin_r.right <= khin_in.px.right && khin_r.bottom <= khin_in.px.bottom;
        };
        for (const RenderObject& o : khrc_list) {
            if (!khrc_vol && !khrc_mir) break;
            if (!kh_obj_vis(o)) continue;   // KH_SHOW_INFRONT.
            float b[4], d = 0.0f;
            const int k = kh_rp_obj_ndc(o, khrc_v, cam, b, d);
            if (k == 0) continue;
            const D3D11_RECT pr = k == 1 ? kh_rp_px_of(b[0], b[1], b[2], b[3]) : D3D11_RECT{ 0, 0, 1, 1 };
            if (khrc_vol && !g_rp_sc_all.full && (k == 2 || !inside(pr, g_rp_sc_all))) khrc_vol = false;
            if (khrc_mir && !g_rp_sc_mir.full && kh_rp_mir_reader(o, cam)) {   // KH_MIR_BAND_RECT: the same bound.
                float mb[4];
                const int mk = (k == 2 || kh_rp_mir_flagged(o)) ? -1 : kh_rp_mir_band_ndc(o, khrc_v, mb);
                const D3D11_RECT pm = mk < 0 ? pr : (mk == 1 ? kh_rp_px_of(mb[0], mb[1], mb[2], mb[3])
                                                              : D3D11_RECT{ 0, 0, 0, 0 });
                if (k == 2 || !inside(pm, g_rp_sc_mir)) khrc_mir = false;
            }
        }
    }
    if (kh_rp_merged_now() && !khrc_vol) kh_stat(g_rp_c.uncovered);
}
// The recorded draws [khri_from, khri_to), re-issued into whatever depth target is bound; with khri_sc, each through
// its rasterizer's scissor twin, clipped to *khri_sc (intersected with the draw's own scissor when it had one).
inline void kh_rp_issue(ID3D11DeviceContext* ctx, uint32_t khri_from, uint32_t khri_to, const D3D11_RECT* khri_sc) {
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    uint32_t khri_vb_hw = KH_RP_VB, khri_vs_hw = KH_RP_SRV, khri_ps_hw = KH_RP_SRV;   // KH_RP_BIND_HW: full first.
    for (uint32_t i = khri_from; i < khri_to && i < g_rp_n; ++i) {
        const KhRpDraw& r = g_rp_draws[i];
        const uint32_t khri_vb_n = r.vb_n > khri_vb_hw ? r.vb_n : khri_vb_hw;   // KH_RP_BIND_HW.
        const uint32_t khri_vs_n = r.vsrv_n > khri_vs_hw ? r.vsrv_n : khri_vs_hw;
        const uint32_t khri_ps_n = r.psrv_n > khri_ps_hw ? r.psrv_n : khri_ps_hw;
        khri_vb_hw = r.vb_n; khri_vs_hw = r.vsrv_n; khri_ps_hw = r.psrv_n;
        ctx->IASetInputLayout(r.il);
        ctx->IASetPrimitiveTopology(r.topo);
        if (khri_vb_n) ctx->IASetVertexBuffers(0, khri_vb_n, r.vb, r.vbs, r.vbo);
        ctx->IASetIndexBuffer(r.ib, r.ifmt, r.ioff);
        ctx->VSSetShader(r.vs, nullptr, 0);
        ctx->PSSetShader(r.ps, nullptr, 0);
        ID3D11DeviceContext1* const c1 = r.cb1 ? kh_ctx1(ctx) : nullptr;
        if (c1) {
            c1->VSSetConstantBuffers1(0, KH_RP_CB, r.vcb, r.vcf, r.vcn);
            c1->PSSetConstantBuffers1(0, KH_RP_CB, r.pcb, r.pcf, r.pcn);
        } else {
            ctx->VSSetConstantBuffers(0, KH_RP_CB, r.vcb);
            ctx->PSSetConstantBuffers(0, KH_RP_CB, r.pcb);
        }
        if (khri_vs_n) ctx->VSSetShaderResources(0, khri_vs_n, r.vsrv);   // KH_RP_BIND_ALL / KH_RP_BIND_HW.
        ctx->VSSetSamplers(0, KH_RP_SMP, r.vsmp);
        if (khri_ps_n) ctx->PSSetShaderResources(0, khri_ps_n, r.psrv);
        kh_bc_forget();   // KH_BIND_FILTER: t0 - t127.
        ctx->PSSetSamplers(0, KH_RP_SMP, r.psmp);
        ctx->OMSetDepthStencilState(r.dss, r.sref);
        ctx->OMSetBlendState(r.bs, r.bf, r.bmask);
        if (r.nvp) ctx->RSSetViewports(r.nvp, r.vp);
        ID3D11RasterizerState* const sc_rs = khri_sc ? kh_rp_rs_scissor(ctx, r.rs) : nullptr;
        if (sc_rs) {
            D3D11_RECT rc = *khri_sc;
            D3D11_RASTERIZER_DESC rd = {};
            if (r.rs) r.rs->GetDesc(&rd);
            if (r.rs && rd.ScissorEnable && r.nsc) {   // The draw clipped itself too: both.
                rc.left = rc.left > r.sc[0].left ? rc.left : r.sc[0].left;
                rc.top = rc.top > r.sc[0].top ? rc.top : r.sc[0].top;
                rc.right = rc.right < r.sc[0].right ? rc.right : r.sc[0].right;
                rc.bottom = rc.bottom < r.sc[0].bottom ? rc.bottom : r.sc[0].bottom;
                if (rc.right <= rc.left || rc.bottom <= rc.top) continue;   // Nothing of it inside.
            }
            ctx->RSSetState(sc_rs);
            ctx->RSSetScissorRects(1, &rc);
        } else {
            ctx->RSSetState(r.rs);
            if (r.nsc) ctx->RSSetScissorRects(r.nsc, r.sc);
        }
        switch (r.kind) {
            case 1: ctx->DrawIndexed(r.a, r.b, r.d); break;
            case 2: ctx->Draw(r.a, r.b); break;
            case 3: ctx->DrawIndexedInstanced(r.a, r.b, r.c, r.d, r.e); break;
            case 4: ctx->DrawInstanced(r.a, r.b, r.c, r.e); break;
            default: break;
        }
    }
}
// KH_MIR_REPLAY: the recorded draws the real mirror re-issued, into khrm_dsv the way kh_vmir_begin re-issues them - no
// render target, the whole mirror's viewport, the near-patched view block at b2 (g_vmir_b2_patch, still this pass's:
// it is patched once a pass, at its first counting draw), the depth-clamp twin of the draw's rasterizer state - and
// everything else as recorded. With khrm_sc each is clipped to it (and to its own scissor when it had one), as
// kh_rp_issue clips. False = a twin could not be made.
inline bool kh_rp_issue_mir(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* khrm_dsv, const D3D11_RECT* khrm_sc) {
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetRenderTargets(0, nullptr, khrm_dsv);
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(g_vmir_w);
    vp.Height = static_cast<float>(g_vmir_h);
    vp.MaxDepth = 1.0f;
    uint32_t khrm_vb_hw = KH_RP_VB, khrm_vs_hw = KH_RP_SRV, khrm_ps_hw = KH_RP_SRV;   // KH_RP_BIND_HW: full first.
    for (uint32_t i = 0; i < g_rp_n; ++i) {
        const KhRpDraw& r = g_rp_draws[i];
        if (!r.mir) continue;
        ID3D11RasterizerState* const rs = kh_rp_rs_mir(ctx, r.rs, khrm_sc != nullptr);
        if (!rs) return false;
        D3D11_RECT rc = {};
        if (khrm_sc) {
            rc = *khrm_sc;
            D3D11_RASTERIZER_DESC rd = {};
            if (r.rs) r.rs->GetDesc(&rd);
            if (r.rs && rd.ScissorEnable && r.nsc) {   // The draw clipped itself too: both.
                rc.left = rc.left > r.sc[0].left ? rc.left : r.sc[0].left;
                rc.top = rc.top > r.sc[0].top ? rc.top : r.sc[0].top;
                rc.right = rc.right < r.sc[0].right ? rc.right : r.sc[0].right;
                rc.bottom = rc.bottom < r.sc[0].bottom ? rc.bottom : r.sc[0].bottom;
                if (rc.right <= rc.left || rc.bottom <= rc.top) continue;   // Nothing of it inside.
            }
        }
        const uint32_t khrm_vb_n = r.vb_n > khrm_vb_hw ? r.vb_n : khrm_vb_hw;   // KH_RP_BIND_HW (past the skips:
        const uint32_t khrm_vs_n = r.vsrv_n > khrm_vs_hw ? r.vsrv_n : khrm_vs_hw;   // a skipped record set nothing).
        const uint32_t khrm_ps_n = r.psrv_n > khrm_ps_hw ? r.psrv_n : khrm_ps_hw;
        khrm_vb_hw = r.vb_n; khrm_vs_hw = r.vsrv_n; khrm_ps_hw = r.psrv_n;
        ctx->IASetInputLayout(r.il);
        ctx->IASetPrimitiveTopology(r.topo);
        if (khrm_vb_n) ctx->IASetVertexBuffers(0, khrm_vb_n, r.vb, r.vbs, r.vbo);
        ctx->IASetIndexBuffer(r.ib, r.ifmt, r.ioff);
        ctx->VSSetShader(r.vs, nullptr, 0);
        ctx->PSSetShader(r.ps, nullptr, 0);
        ID3D11DeviceContext1* const c1 = r.cb1 ? kh_ctx1(ctx) : nullptr;
        if (c1) {
            c1->VSSetConstantBuffers1(0, KH_RP_CB, r.vcb, r.vcf, r.vcn);
            c1->PSSetConstantBuffers1(0, KH_RP_CB, r.pcb, r.pcf, r.pcn);
        } else {
            ctx->VSSetConstantBuffers(0, KH_RP_CB, r.vcb);
            ctx->PSSetConstantBuffers(0, KH_RP_CB, r.pcb);
        }
        ctx->VSSetConstantBuffers(2, 1, &g_vmir_b2_patch);   // Whole, as kh_vmir_begin binds it.
        if (khrm_vs_n) ctx->VSSetShaderResources(0, khrm_vs_n, r.vsrv);   // KH_RP_BIND_ALL / KH_RP_BIND_HW.
        ctx->VSSetSamplers(0, KH_RP_SMP, r.vsmp);
        if (khrm_ps_n) ctx->PSSetShaderResources(0, khrm_ps_n, r.psrv);
        kh_bc_forget();   // KH_BIND_FILTER: t0 - t127.
        ctx->PSSetSamplers(0, KH_RP_SMP, r.psmp);
        ctx->OMSetDepthStencilState(r.dss, r.sref);
        ctx->OMSetBlendState(r.bs, r.bf, r.bmask);
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(rs);
        if (khrm_sc) ctx->RSSetScissorRects(1, &rc);
        else if (r.nsc) ctx->RSSetScissorRects(r.nsc, r.sc);
        switch (r.kind) {
            case 1: ctx->DrawIndexed(r.a, r.b, r.d); break;
            case 2: ctx->Draw(r.a, r.b); break;
            case 3: ctx->DrawIndexedInstanced(r.a, r.b, r.c, r.d, r.e); break;
            case 4: ctx->DrawInstanced(r.a, r.b, r.c, r.e); break;
            default: break;
        }
    }
    return true;
}
// KH_MIR_REPLAY: mirror A, B and the merged mirror, at the volume buffer's size (the mirror's own, kh_vmir_ensure_targets).
inline bool kh_rp_ensure_mir(ID3D11Device* dev) {
    if (!dev || g_rp_w == 0 || g_rp_h == 0) return false;
    if (g_rp_mira_tex && g_rp_mira_dsv && g_rp_mira_srv && g_rp_mirb_tex && g_rp_mirb_dsv && g_rp_mirb_srv &&
        g_rp_mirm_tex && g_rp_mirm_rtv && g_rp_mirm_srv && g_rp_mir_w == g_rp_w && g_rp_mir_h == g_rp_h) return true;
    kh_rp_mir_release();
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_rp_w; td.Height = g_rp_h; td.MipLevels = 1; td.ArraySize = 1; td.SampleDesc.Count = 1;
    td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    dd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
    vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    vd.Texture2D.MipLevels = 1;
    bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_rp_mira_tex)) && g_rp_mira_tex &&
              SUCCEEDED(dev->CreateDepthStencilView(g_rp_mira_tex, &dd, &g_rp_mira_dsv)) && g_rp_mira_dsv &&
              SUCCEEDED(dev->CreateShaderResourceView(g_rp_mira_tex, &vd, &g_rp_mira_srv)) && g_rp_mira_srv &&
              SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_rp_mirb_tex)) && g_rp_mirb_tex &&
              SUCCEEDED(dev->CreateDepthStencilView(g_rp_mirb_tex, &dd, &g_rp_mirb_dsv)) && g_rp_mirb_dsv &&
              SUCCEEDED(dev->CreateShaderResourceView(g_rp_mirb_tex, &vd, &g_rp_mirb_srv)) && g_rp_mirb_srv;
    if (ok) {
        D3D11_TEXTURE2D_DESC md = td;
        md.Format = DXGI_FORMAT_R8G8_UINT;
        md.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        ok = SUCCEEDED(dev->CreateTexture2D(&md, nullptr, &g_rp_mirm_tex)) && g_rp_mirm_tex &&
             SUCCEEDED(dev->CreateRenderTargetView(g_rp_mirm_tex, nullptr, &g_rp_mirm_rtv)) && g_rp_mirm_rtv &&
             SUCCEEDED(dev->CreateShaderResourceView(g_rp_mirm_tex, nullptr, &g_rp_mirm_srv)) && g_rp_mirm_srv;
    }
    if (!ok) { kh_rp_mir_release(); return false; }
    g_rp_mir_w = g_rp_w;
    g_rp_mir_h = g_rp_h;
    return true;
}
// KH_MIR_REPLAY: the recorded pass drew the real mirror - its prepass ran in this pass's frame on this buffer, and at
// least one of its draws was re-issued there.
inline bool kh_rp_mir_used() {
    if (g_vmir_prepass_stamp != static_cast<uint32_t>(g_rp_pass) || !g_vmir_prepass_src ||
        g_vmir_prepass_src != g_svs_vol_src) return false;
    for (uint32_t i = 0; i < g_rp_n; ++i) if (g_rp_draws[i].mir) return true;
    return false;
}
// KH_MIR_REPLAY: the mirror can be replayed this cycle - it was drawn, what its re-issue bound still stands, and the
// targets exist at its size.
inline bool kh_rp_mir_eligible(ID3D11DeviceContext* ctx) {
    if (!kh_rp_mir_used() || !g_res.ps_rpmergemir || !g_vmir_tex || !g_vmir_srv || !g_vmir_b2_patch) return false;
    if (g_vmir_w != g_rp_w || g_vmir_h != g_rp_h) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = kh_rp_ensure_mir(dev);
    if (dev) dev->Release();
    return ok;
}
// KH_MIR_REPLAY: run by kh_rp_mir_finish, in its own state bracket, for the first pass this cycle that reads the mirror
// (kh_rp_replay's merge armed it). Mirror A: the real mirror's depth (the pass's prepass) with its count cleared;
// mirror B: this cycle's prepass (replay B's world seam drew it); the same re-issued draws into both; then the merge -
// B where A equals the real mirror, else the real mirror. False = the real mirror stands.
inline bool kh_rp_mir_replay(ID3D11DeviceContext* ctx, const D3D11_RECT* khmr_sc) {
    if (!g_rp_mirb_drawn || !g_rp_mira_tex || !g_rp_mira_dsv || !g_rp_mirb_dsv || !g_rp_mirm_rtv ||
        !g_res.ps_rpmergemir || !g_vmir_tex) return false;
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->CopyResource(g_rp_mira_tex, g_vmir_tex);
    ctx->ClearDepthStencilView(g_rp_mira_dsv, D3D11_CLEAR_STENCIL, 1.0f, 0);   // The count only: the depth stands.
    if (!kh_rp_issue_mir(ctx, g_rp_mira_dsv, khmr_sc)) return false;
    if (!kh_rp_issue_mir(ctx, g_rp_mirb_dsv, khmr_sc)) return false;
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* nul[1] = { nullptr };
    ctx->PSSetShaderResources(28, 1, nul);   // The merged mirror may still be bound for reading from the last cycle.
    ctx->OMSetRenderTargets(1, &g_rp_mirm_rtv, nullptr);
    D3D11_VIEWPORT vpc = {};
    vpc.Width = static_cast<float>(g_rp_w);
    vpc.Height = static_cast<float>(g_rp_h);
    vpc.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpc);
    ID3D11RasterizerState* const mrs = khmr_sc ? kh_rp_rs_scissor(ctx, nullptr) : nullptr;
    ctx->RSSetState(mrs);
    if (mrs) ctx->RSSetScissorRects(1, khmr_sc);
    ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_rpmergemir, nullptr, 0);
    ID3D11ShaderResourceView* const in_a = g_rp_mira_srv;
    ID3D11ShaderResourceView* const in_e = g_vmir_srv;
    ID3D11ShaderResourceView* const in_b = g_rp_mirb_srv;
    ctx->PSSetShaderResources(34, 1, &in_a);
    ctx->PSSetShaderResources(41, 1, &in_e);
    ctx->PSSetShaderResources(49, 1, &in_b);
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };   // VSFullscreen's lanes (the volume merge's).
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->Draw(3, 0);
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->PSSetShaderResources(34, 1, nul);
    ctx->PSSetShaderResources(41, 1, nul);
    ctx->PSSetShaderResources(49, 1, nul);
    return true;
}
// Replay B can run this frame (the reason it cannot is its fallback).
inline bool kh_rp_b_eligible() {
    if (g_rp_eng_seq != g_rp_pass) return false;          // No engine snapshot of this pass.
    if (g_rp_vol_clear_seq != g_rp_pass) return false;    // Not cleared this frame: a footprint could survive in it.
    if (!g_svs_vol_foot_ok || !kh_svs_foot_bound()) return false;   // The fallback pixels' witness.
    if (!g_res.ps_rpmerge || !g_rp_dsvb || !g_rp_stenb || !g_rp_mst_rtv || !g_rp_mft_rtv) return false;
    // B's footprint in the copy's raster: the latch's projection where it agrees with the copy's, else the copy's own
    // terms (KH_REPLAY_RASTER, set here for kh_rp_replay's cycle; the same handedness is required, as the lookup's
    // map requires it).
    g_rp_rast_on = false;
    if (g_svs_vol_proj_ok) {
        float now[4];
        if (!g_ro.cycle_pv_valid || !kh_sten_proj_terms(g_ro.cycle_pv.projection, now)) return false;
        bool khbe_part = false;
        for (int k = 0; k < 4; ++k) {
            const float tol = 1.0e-4f * (fabsf(now[k]) > 1.0f ? fabsf(now[k]) : 1.0f);
            if (fabsf(g_svs_vol_proj[k] - now[k]) > tol) khbe_part = true;
        }
        if (khbe_part) {
            if (!(g_svs_vol_proj[0] * now[0] > 0.0f && g_svs_vol_proj[1] * now[1] > 0.0f)) return false;
            memcpy(g_rp_rast, g_svs_vol_proj, sizeof(g_rp_rast));
            g_rp_rast_on = true;
        }
    }
    return true;
}
// Replay B: the engine's depth (already in its target: kh_rp_snap_engine), our world footprint at this cycle's pose
// (the world seam's own pass in B mode, with the volume pass's b2..b4 bound - its depth goes through the engine's view
// block), then the recorded draws with our in-front footprint (the hands seam in B mode, its slice viewport) at the
// place the hands seam came in the pass. The seams' globals are their own frame's again afterwards. False when the
// world footprint did not draw.
inline bool kh_rp_replay_b(ID3D11DeviceContext* ctx, const D3D11_RECT* khrb_sc) {
    ctx->OMSetRenderTargets(0, nullptr, g_rp_dsvb);
    {
        const KhRpDraw& r0 = g_rp_draws[0];
        ID3D11DeviceContext1* const c1 = r0.cb1 ? kh_ctx1(ctx) : nullptr;
        if (c1) c1->VSSetConstantBuffers1(2, 3, &r0.vcb[2], &r0.vcf[2], &r0.vcn[2]);
        else ctx->VSSetConstantBuffers(2, 3, &r0.vcb[2]);
    }
    const uint64_t sv_sig = g_svs_skip_sig;
    const bool sv_valid = g_svs_skip_valid, sv_inj = g_svs_injected_frame, sv_prime = g_svs_prime_ready;
    float sv_vp[4][4]; memcpy(sv_vp, g_svs_prime_vp, sizeof(sv_vp));
    float sv_cam[3]; memcpy(sv_cam, g_svs_prime_cam, sizeof(sv_cam));
    const bool sv_rebase = g_svs_prime_rebase;
    const float sv_lo = g_svs_prime_vp_lo, sv_hi = g_svs_prime_vp_hi;
    float sv_enc[4]; memcpy(sv_enc, g_svs_seam_enc, sizeof(sv_enc));
    const uint64_t sv_enc_seq = g_svs_seam_enc_seq;
    float sv_proj[4]; memcpy(sv_proj, g_svs_seam_proj, sizeof(sv_proj));
    const bool sv_proj_ok = g_svs_seam_proj_ok;
    const int sv_fix = g_svs_foot_seam_ix;
    const uint64_t sv_fseq = g_svs_foot_seam_seq;
    auto khrb_back = [&]() {   // The seams' globals their own frame's again.
        g_rp_seam_b = false;
        g_svs_skip_sig = sv_sig; g_svs_skip_valid = sv_valid;
        g_svs_injected_frame = sv_inj; g_svs_prime_ready = sv_prime;
        memcpy(g_svs_prime_vp, sv_vp, sizeof(sv_vp)); memcpy(g_svs_prime_cam, sv_cam, sizeof(sv_cam));
        g_svs_prime_rebase = sv_rebase; g_svs_prime_vp_lo = sv_lo; g_svs_prime_vp_hi = sv_hi;
        memcpy(g_svs_seam_enc, sv_enc, sizeof(sv_enc)); g_svs_seam_enc_seq = sv_enc_seq;
        memcpy(g_svs_seam_proj, sv_proj, sizeof(sv_proj)); g_svs_seam_proj_ok = sv_proj_ok;
        g_svs_foot_seam_ix = sv_fix; g_svs_foot_seam_seq = sv_fseq;
    };
    KhRpOnUnwind khrb_uw(khrb_back);   // KH_RP_UNWIND: a throw in a seam would leave B mode on for every seam after.
    g_svs_foot_seam_seq = ~0ull;
    g_rp_seam_b = true;
    kh_volume_seam_inject(ctx, g_rp_w, g_rp_h);
    const int b_foot = g_svs_foot_pub == 0 ? 1 : 0;
    const bool drew = g_svs_foot_seam_seq == g_svs_frame_seq && g_svs_foot_seam_ix == b_foot;
    if (drew) {
        const bool front = g_rp_front_seq == g_rp_pass;
        const uint32_t at = front ? (g_rp_front_idx < g_rp_n ? g_rp_front_idx : g_rp_n) : g_rp_n;
        ctx->OMSetRenderTargets(0, nullptr, g_rp_dsvb);
        kh_rp_issue(ctx, 0, at, khrb_sc);
        if (front) {   // The in-front footprint where the hands seam came: its slice, into B, at this cycle's pose.
            ctx->OMSetRenderTargets(0, nullptr, g_rp_dsvb);
            ctx->RSSetViewports(1, &g_rp_front_vp);
            kh_infront_seam_inject(ctx, g_rp_front_vp);
        }
        ctx->OMSetRenderTargets(0, nullptr, g_rp_dsvb);
        kh_rp_issue(ctx, at, g_rp_n, khrb_sc);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
    }
    khrb_back();
    return drew;
}
// At the injection: replay the last recorded pass and merge. A withheld pass is B everywhere (its copy counted against
// whatever is behind our meshes); one that was not keeps B only where replay A equals the engine's count.
inline void kh_rp_replay(ID3D11DeviceContext* ctx) {
    // KH_REPLAY_ONCE: an injection re-armed within the cycle (the camera partition's wait, the anomalous and
    // rescue windows) calls again after the pass was replayed; that replay's verdicts - the merge, the mirror
    // armed for the first pass that reads it, the raster - stand for the rest of the cycle.
    if (g_rp_replayed && g_rp_replay_cyc == g_topo_cycles) return;
    g_rp_merge_ok = false;   // This cycle is merged only if the replay below completes.
    g_rp_mir_ok = false;     // KH_MIR_REPLAY: likewise the mirror.
    g_rp_rast_on = false;    // KH_REPLAY_RASTER: this cycle's replay sets it (kh_rp_b_eligible).
    if (g_rp_mir_pend) { g_rp_mir_pend = false; kh_stat(g_rp_c.mir_idle); }   // Last cycle's: no pass read it.
    if (g_rp_pass == ~0ull || g_rp_replayed || !g_rp_ended) return;
    g_rp_replayed = true;
    g_rp_replay_cyc = g_topo_cycles;   // KH_REPLAY_ONCE.
    if (!g_rp_ok || g_rp_n == 0 || g_svs_vol_seq != g_rp_pass || !g_svs_vol_sten_srv || !g_svs_vol_primed ||
        !g_res.vs_fullscreen || !g_res.dss_off || !g_res.composite_cb || !kh_rp_b_eligible()) {
        kh_stat(g_rp_c.fallback);
        return;
    }
    const bool withheld = g_rp_withheld_seq == g_rp_pass;
    {
        // KH_RP_PRESYNC: replay B's world seam's step and sync (kh_volume_seam_inject's, under the same flag), taken
        // ahead of the rectangles, so they and sc_gen are of the scene B draws and the pass after this draws (its list
        // taken again from it, KH_OBJBUF_LATE): covered by the generation alone. A step after the cycle's final one
        // reuses its pick (kh_gts_for_cycle), so B's own step and sync take only what changed since (a game-thread
        // stage). A return below that B does not follow leaves the pass this scene, as B would. Outside sv's capture,
        // as the injection's own step: a step binds only inside its own StateBackup (kh_skin_so_batch, kh_uvs_step).
        // Cost, per cycle that reaches it: one more kh_attach_step (its g_draw_list_mutex walk of every binding and
        // its per-chain end pass), kh_uvs_step and kh_scene_sync on the CPU; GPU work only for what changed since the
        // injection's own step.
        KhInjectionFlagScope khrp_flag;
        g_ro.in_injection = true;
        kh_attach_step(ctx);
        kh_uvs_step(ctx);
    }
    kh_scene_sync();
    KhRpRect sc_all, sc_mir;   // KH_REPLAY_SCISSOR.
    kh_rp_scissor_rects(sc_all, sc_mir);
    const uint64_t sc_gen = g_scene.gen;   // What they were made from (KH_RP_PRESYNC).
    if (!sc_all.full && !sc_all.any) return;   // No mesh of ours on screen.
    kh_stat(sc_all.full ? g_rp_c.fullscreen : g_rp_c.scissored);
    const D3D11_RECT* const scp = sc_all.full ? nullptr : &sc_all.px;
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    // KH_RP_UNWIND: declared ahead of sv, so a throw restores the state (sv's destructor, our hooks still aside) first.
    KhRpOnUnwind khrp_uw([&]() { g_rp_mir_want = false; g_ro.in_injection = prev; });
    if (!kh_rp_img_upload(ctx)) {   // KH_REPLAY_CPUCB: the pages the records bind, before any of them replays.
        g_ro.in_injection = prev;
        kh_stat(g_rp_c.fallback);
        return;
    }
    KhRpSave sv;
    sv.capture(ctx);
    bool ok = true;
    if (!withheld) {   // Replay A (the trust): the pass as the engine drew it, from its first draw.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        ctx->CopyResource(g_rp_tgt, g_rp_pre);
        ctx->OMSetRenderTargets(0, nullptr, g_rp_dsv);
        kh_rp_issue(ctx, 0, g_rp_n, scp);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
    }
    // KH_MIR_REPLAY: when the pass drew the real mirror and a mesh that can read it is on screen, replay B's world
    // seam draws mirror B's prepass too; the rest is replayed, in that mesh set's rectangle (g_rp_sc_mir), for the
    // first pass that reads the mirror (kh_rp_mir_finish).
    const bool mir_used = kh_rp_mir_used();
    const bool mir_read = sc_mir.full || sc_mir.any;
    const bool mir = mir_used && mir_read && kh_rp_mir_eligible(ctx);
    g_rp_mir_want = mir;
    g_rp_mirb_drawn = false;
    ok = kh_rp_replay_b(ctx, scp);
    g_rp_mir_want = false;
    if (ok) {   // The merge: the count and its footprint distance, B where trusted (everywhere when withheld).
        ID3D11ShaderResourceView* nul[1] = { nullptr };
        ctx->PSSetShaderResources(24, 1, nul);   // Neither merged texture may be bound for reading while written.
        ctx->PSSetShaderResources(33, 1, nul);
        ID3D11RenderTargetView* mrt[2] = { g_rp_mst_rtv, g_rp_mft_rtv };
        ctx->OMSetRenderTargets(2, mrt, nullptr);
        D3D11_VIEWPORT vpc = {};
        vpc.Width = static_cast<float>(g_rp_w);
        vpc.Height = static_cast<float>(g_rp_h);
        vpc.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vpc);
        ID3D11RasterizerState* const mrs = scp ? kh_rp_rs_scissor(ctx, nullptr) : nullptr;
        ctx->RSSetState(mrs);
        if (mrs) ctx->RSSetScissorRects(1, scp);
        ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_rpmerge, nullptr, 0);
        ID3D11ShaderResourceView* in2[2] = { withheld ? nullptr : g_rp_sten, g_svs_vol_sten_srv };
        ctx->PSSetShaderResources(34, 1, &in2[0]);
        ctx->PSSetShaderResources(41, 1, &in2[1]);
        ID3D11ShaderResourceView* in3[3] = { g_rp_stenb, g_svs_foot_srv[g_svs_foot_pub == 0 ? 1 : 0], kh_svs_foot_bound() };
        ctx->PSSetShaderResources(49, 3, in3);
        ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };   // Bound before the upload (ring).
        ctx->VSSetConstantBuffers(0, 2, cbs);
        ctx->PSSetConstantBuffers(0, 2, cbs);
        ConstantData cbd = {};
        cbd.color[1] = withheld ? 1.0f : 0.0f;
        ok = kh_upload_obj_cb(ctx, g_res.composite_cb, cbd);
        if (ok) ctx->Draw(3, 0);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        ID3D11ShaderResourceView* nul5[5] = {};
        ctx->PSSetShaderResources(49, 3, nul5);
        ctx->PSSetShaderResources(34, 1, nul5);
        ctx->PSSetShaderResources(41, 1, nul5);
    }
    sv.restore(ctx);
    sv.release();
    g_ro.in_injection = prev;
    if (ok) {
        g_rp_merge_ok = true;
        g_rp_merge_cyc = g_topo_cycles;
        g_rp_sc_all = sc_all;   // KH_REPLAY_SCISSOR: what the passes after it test themselves against.
        g_rp_sc_mir = sc_mir;
        g_rp_sc_gen = sc_gen;
        kh_stat(g_rp_c.merged);
        if (mir_used) {   // KH_MIR_REPLAY: armed for the first pass that reads the mirror (kh_rp_mir_finish).
            if (!mir_read) {
                kh_stat(g_rp_c.mir_idle);
            } else if (mir && g_rp_mirb_drawn) {
                g_rp_mir_pend = true;
                g_rp_mir_pend_cyc = g_topo_cycles;
                g_rp_mir_pend_pass = g_rp_pass;
            } else {
                kh_stat(g_rp_c.mir_fallback);
            }
        }
    } else {
        kh_stat(g_rp_c.fallback);
    }
}
// KH_MIR_REPLAY: the armed mirror replay, run once, by the first pass this cycle with a mesh that reads the mirror -
// in its own state bracket. Everything it replays must still be the armed pass's: the same cycle, the same recorded
// pass (ended; a new pass releases its records), the real mirror not prepassed again, the volume replay merged.
// Otherwise the real mirror stands (a mirror fallback).
inline void kh_rp_mir_finish(ID3D11DeviceContext* ctx) {
    if (!g_rp_mir_pend || !ctx) return;
    g_rp_mir_pend = false;
    const bool live = g_rp_mir_pend_cyc == g_topo_cycles && g_rp_mir_pend_pass == g_rp_pass && g_rp_ended &&
                      g_vmir_prepass_stamp == static_cast<uint32_t>(g_rp_pass) && kh_rp_merged_now();
    bool done = false;
    if (live) {
        const bool prev = g_ro.in_injection;
        g_ro.in_injection = true;
        KhRpOnUnwind khmf_uw([&]() { g_ro.in_injection = prev; });   // KH_RP_UNWIND: after sv's restore.
        KhRpSave sv;
        sv.capture(ctx);
        done = kh_rp_mir_replay(ctx, g_rp_sc_mir.full ? nullptr : &g_rp_sc_mir.px);
        sv.restore(ctx);
        sv.release();
        g_ro.in_injection = prev;
    }
    g_rp_mir_ok = done;
    g_rp_mir_cyc = g_topo_cycles;
    kh_stat(done ? g_rp_c.mir_merged : g_rp_c.mir_fallback);
}
// KH_VOL_REPLAY: the engine's own call - the target context, on the render thread, not one of ours. The gate of the
// hooks that refuse a pass the recorder cannot follow (kh_rp_foreign: a command list, DrawAuto or an indirect draw, a
// copy into the volume buffer); CopyResource's is a core hook, the others are installed one by one after the core hooks
// (kh_rp_extra_hooks; a failure leaves that one off).
inline bool kh_rp_engine_call(ID3D11DeviceContext* self) {
    return self == g_reorder_target_ctx.load(std::memory_order_relaxed) && reorder_on_render_thread() && !g_ro.in_injection;
}
}