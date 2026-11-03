#pragma once

// The injection half (rendering_integration_injection.hpp): VOL_REPLAY, the recorder and the replay. Never include
// it on its own.
namespace RenderIntegration {

// INFRONT - the view-model seam. MEASURED (the same capture): the engine's stencil shadows are counted in a separate
// volume buffer BEFORE the main depth clear - a world depth prepass, the world's volume counting (stencil IncWrap /
// DecWrap under depth GreaterEqual, no write), then the HANDS PREPASS into the same buffer in the hands range (depth
// LESS_EQUAL + write, stencil AlwaysTrue / Replace 0: the count under the hands reset), then the hands' own counting in
// the hands range (non-hands pixels fail the depth test with zfail Keep and are untouched), then the resolve our copy
// is taken at. The recount redraws only the volumes that reach the camera (MEASURED: 15 of the world count's 58 draws -
// the character's own and a few near ones), and the depth partition hides every world-range volume from a hands-range
// depth. So an inFront mesh takes part twice: at the world seam (inside volume_seam_inject, the world range under
// the depth clamp) for the world count, and here, at the FIRST hands-range draw on the volume buffer - the engine's own
// hands prepass, ahead of its first draw - with its hands-range depth and the stencil untouched: the engine's Replace 0
// then fires only where its hands beat us, the world count survives under the mesh, and the recount adds its near
// subset against our depth (a shadowed count stays nonzero and small, the shader's test).
//
// The view is the world seam's own take (g_svs.prime_vp, this window) with column 2 rebuilt from the hands pair - exact
// for a projection that differs only in m22 / m32 - or, when the world seam did not run this window (no world caster),
// the same take made here. Depth only, level 0, the solid texels of an alpha material through PSInjDepthA (the slice
// prepass's recipe). Defined in rendering/view_model_seam.hpp.
inline void infront_seam_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& vp);
// VOL_REPLAY - the recorder and the replay (the design note is with the shared state, rendering/replay_state.hpp).
// Render thread. A pass is every draw with the volume buffer bound from its first counting draw to the stencil
// resolve (the bracket): the counting draws and the uncounted ones beside a target (the engine's hands prepass among
// them), in order. A draw is recorded with everything it had bound but its outputs, its class instances and its
// predicate (RP_BIND_ALL; the replay draws into its own depth target, unpredicated: a draw the engine's predicate
// skipped is replayed - the query's verdict is the GPU's, and the predicate may be issued again before the replay).
// The constant, vertex and index buffers a draw reads that the engine can still rewrite are copied as the draw used
// them: constant buffers whole (once per buffer per draw, or once per pass while the engine does not write it -
// REPLAY_CBREUSE; from the CPU where the upload funnel captured the write - REPLAY_CPUCB, never an update made
// under a predicate - RP_IMG_PRED), vertex / index buffers at the pass's end, and a vertex / index
// buffer a record reads that the engine writes inside the pass, ahead of the write, for the draws before it
// (REPLAY_SPLIT): a DISCARD map (a ring's wrap), UpdateSubresource(1), CopyResource, CopySubresourceRegion(1) or
// CopyStructureCount into it, or its bind as an output-merger UAV (RP_WRITE_SPLIT). A NO_OVERWRITE map or a
// D3D11_COPY_NO_OVERWRITE write splits nothing: its contract leaves what the draws so far read alone; a pass whose
// other writes split past RP_WSPLIT_MAX / RP_WSPLIT_BYTES is refused (RP_SPLIT_CAP). Not seen, so not
// covered: a write through a stream-output target, a compute-stage UAV, a render target view of a buffer or a view's
// clear, DiscardResource / DiscardView, and a writer whose hook did not install (rp_extra_hooks) - none of which
// the engine is known to aim at a vertex or index buffer; and the contents behind a VS / PS shader-resource view,
// bound by reference as recorded and read as they stand at the replay. A pass something else writes the volume
// buffer in (a clear, our world seam, a command list, an indirect draw or DrawAuto, a copy into it) is refused and
// the frame keeps the copy; our in-front footprint is left to the merge's per-pixel trust (rp_foreign).
// RP_UNCAPPED: a pass has no draw limit - the records grow on demand (g_rp_rec.draws). A map, a write or a constant
// capture is a lookup (RpPtrMap), not a walk of the records or the reuse list; only a split (REPLAY_SPLIT: an
// engine write into a buffer a record still flags - a DISCARD map once per ring wrap) walks the records drawn so far.
// A pass costs its draws plus one such walk per split.
// RP_BIND_ALL: every vertex buffer slot, every VS / PS shader-resource and sampler slot - what the engine's draw had
// bound, the slots its shaders read among them, is what the replayed draw binds (no shader's reads are knowable here,
// and a slot left to the injection's bindings could hold anything). Its cost: per recorded draw two more D3D calls
// (VSGetShaderResources 128, VSGetSamplers 16), PSGetShaderResources 16 -> 128 slots and IAGetVertexBuffers 4 -> 32, an
// AddRef for every bound object in them (stale binds too) and its Release at the record's release (a record is 4,096 B
// against 1,568 without it; its release walks 355 pointer slots against 71); per replayed draw (replay B, A after a
// fallback, the mirror's draws twice) two more Set calls and the PS shader-resource / vertex buffer Sets at 128 / 32
// slots; per replay two more Get and Set pairs (RpSave). A rewritable buffer bound, even stale, in vertex slots 4 -
// 31 is copied at the pass's end and split on a write, as one in slots 0 - 3.
// RP_BIND_HW: the replay's Sets stop at the highest bound slot. A record keeps, per array, the count up to its last
// non-null slot (vb_n / vsrv_n / psrv_n, from the full Get - the Get stays whole: which slots are bound is not knowable
// before it); a replay loop sets max(this record's count, the previous record's) slots from the record's array, whose
// entries past its count are null. The pipeline's slots then equal the record's whole array after every Set exactly
// as the full Set left them: the first draw of a loop sets every slot (the loop starts from the caller's unknown
// state), and from then on a slot past both counts was already null. Nothing inside either loop binds a slot between
// two records (the seams run between rp_issue calls); the loops' locals start full for that reason.
static constexpr uint32_t RP_CB = 14u, RP_VB = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
static constexpr uint32_t RP_SRV = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
static constexpr uint32_t RP_SMP = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
static constexpr uint32_t RP_VPS = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
// RP_BIND_HW: the count up to an array's last non-null entry (0 = none bound).
template <typename T>
inline uint32_t rp_bound_n(T* const* a, uint32_t n) {
    uint32_t r = n;
    while (r > 0u && a[r - 1u] == nullptr) --r;
    return r;
}
struct RpDraw {
    uint8_t kind = 0;
    bool mir = false;   // MIR_REPLAY: the real mirror re-issued this draw (g_vmir.pending, set just before it).
    UINT a = 0, b = 0, c = 0, e = 0;
    INT d = 0;
    ID3D11InputLayout* il = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* vb[RP_VB] = {};
    UINT vbs[RP_VB] = {}, vbo[RP_VB] = {};
    bool vb_dyn[RP_VB] = {};   // Still the engine's rewritable buffer: copied at the pass's end.
    ID3D11Buffer* ib = nullptr;
    DXGI_FORMAT ifmt = DXGI_FORMAT_UNKNOWN;
    UINT ioff = 0;
    bool ib_dyn = false;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    bool cb1 = false;   // The windows below are real (captured through context 1).
    ID3D11Buffer* vcb[RP_CB] = {};
    UINT vcf[RP_CB] = {}, vcn[RP_CB] = {};
    ID3D11Buffer* pcb[RP_CB] = {};
    UINT pcf[RP_CB] = {}, pcn[RP_CB] = {};
    ID3D11ShaderResourceView* vsrv[RP_SRV] = {};   // RP_BIND_ALL.
    ID3D11SamplerState* vsmp[RP_SMP] = {};
    ID3D11ShaderResourceView* psrv[RP_SRV] = {};
    ID3D11SamplerState* psmp[RP_SMP] = {};
    uint8_t vb_n = 0, vsrv_n = 0, psrv_n = 0;   // RP_BIND_HW: slots up to the last bound one (<= 32 / 128 / 128).
    ID3D11DepthStencilState* dss = nullptr;
    UINT sref = 0;
    ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* bs = nullptr;
    FLOAT bf[4] = {};
    UINT bmask = 0xFFFFFFFFu;
    UINT nvp = 0, nsc = 0;
    D3D11_VIEWPORT vp[RP_VPS] = {};
    D3D11_RECT sc[RP_VPS] = {};
    void release() {
        KH_SAFE_RELEASE(il); KH_SAFE_RELEASE(ib); KH_SAFE_RELEASE(vs); KH_SAFE_RELEASE(ps);
        for (auto& x : vb) KH_SAFE_RELEASE(x);
        for (auto& x : vcb) KH_SAFE_RELEASE(x);
        for (auto& x : pcb) KH_SAFE_RELEASE(x);
        for (auto& x : vsrv) KH_SAFE_RELEASE(x);   // RP_BIND_ALL.
        for (auto& x : vsmp) KH_SAFE_RELEASE(x);
        for (auto& x : psrv) KH_SAFE_RELEASE(x);
        for (auto& x : psmp) KH_SAFE_RELEASE(x);
        KH_SAFE_RELEASE(dss); KH_SAFE_RELEASE(rs); KH_SAFE_RELEASE(bs);
        // RP_RELEASE_LEAN: no reset of the rest - every pointer is nulled above, and rp_record writes every
        // other field a reader takes before the record is read (the windows only under cb1, the viewports / rects
        // only up to their counts); a record a throw cut short refuses its pass, whose records are only released.
    }
};
// getRenderStats "stencilReplay": [merged frames, fallback frames, refused passes, ring wraps captured, scissored
// frames, full-screen frames, mirror merged, mirror fallback, mirror idle (MIR_REPLAY: merged frames whose pass drew
// the mirror - the mirror merged too, could not, or no pass that cycle had a mesh reading it), uncovered passes
// (REPLAY_SCISSOR: a pass on a merged cycle that read the copy, an object of its outside the replay's rectangle),
// constant snapshots copied on the GPU, constant snapshots taken from the CPU (REPLAY_CPUCB), write splits
// (RP_WRITE_SPLIT: a buffer copied ahead of an engine write other than a DISCARD map), passes refused at the write
// split cap (RP_SPLIT_CAP, counted in the refused passes too)].
struct RpCounters {
    uint64_t merged = 0, fallback = 0, refused = 0, splits = 0, scissored = 0, fullscreen = 0;
    uint64_t mir_merged = 0, mir_fallback = 0, mir_idle = 0, uncovered = 0;
    uint64_t cb_gpu = 0, cb_cpu = 0;
    uint64_t wsplits = 0, capped = 0;
};
// REPLAY_CBREUSE: this pass's constant snapshots by engine buffer - each copied whole (rp_cb_capture), so the
// buffer alone is the key. A later draw binding a buffer the engine has not written since takes the same copy - its
// contents are those of that earlier draw. Every write path forgets the buffer (rp_cb_forget); reuse runs only while
// every writer the context has is hooked (g_rp_rec.cbreuse_on, set where the hooks are installed). Kept for one pass
// (cleared at its start and end). Each entry holds a reference to its engine buffer, so the address cannot be recycled
// under the key while it is listed: a buffer created at a released one's address gets its contents from CreateBuffer,
// which no hook sees, and would take the dead buffer's copy (HOOK_DESC_CACHE's rule). The copies are the pool's.
// Writes can come from any thread, so the map is locked; the count lets a Map outside a pass pay one atomic load. A
// record's constant snapshot: buf is a GPU copy (slot_c 0: the slot keeps its recorded window) or a page of CPU images
// (REPLAY_CPUCB: the image at constant base_c, slot_c constants long, the slot's window moved into it).
struct RpSnap { ID3D11Buffer* buf = nullptr; UINT base_c = 0, slot_c = 0; };
// REPLAY_CBREUSE / REPLAY_SPLIT: an engine object's address -> V, by open addressing (linear probing; erase
// shifts the run back, so no tombstones), at most half full; the power-of-two capacity is kept across clears, so a
// warmed-up pass allocates nothing. A key must stay pinned while listed - a reference held by the entry or by a record
// (HOOK_DESC_CACHE's rule).
template <typename V> struct RpPtrMap {
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
    // Removes k (true, its value to out) or finds nothing (false).
    bool erase(const void* k, V* out = nullptr) {
        if (n == 0u || !k) return false;
        const uint32_t m = static_cast<uint32_t>(t.size()) - 1u;
        uint32_t i = home(k);
        while (t[i].k != k) {
            if (!t[i].k) return false;
            i = (i + 1u) & m;
        }
        if (out) *out = t[i].v;
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
    // Entries, count and storage traded with o whole (RP_UNLOCKED_RELEASE).
    void swap(RpPtrMap& o) { t.swap(o.t); std::swap(n, o.n); std::swap(bits, o.bits); }
};
// RP_DESC_CACHE: a buffer's description, fixed at its creation - what rp_is_mutable answers, asked once per
// buffer per pass instead of once per bind. Pinned (a pinned address cannot be a recycled one); cleared with the
// records. Render thread, like every caller of rp_is_mutable.
struct RpBufDesc { ID3D11Buffer* b; UINT bytes; UINT bind; bool mut; };
// RP_CB_EXACT: the GPU constant snapshots, pooled by the engine buffer's exact width - a snapshot is a buffer of
// the engine's own size bound with the engine's own window, so a read past the engine buffer's end reads past the
// snapshot's too (0, as on the engine's - a wider pooled buffer would hold another snapshot's bytes there). [0, used)
// this pass's; pass = the last pass (g_rp_rec.pass) that took one, pruned with RP_DYN_PRUNE's rule (rp_pass_end).
struct RpCbCls { UINT bytes = 0; uint32_t used = 0; uint64_t pass = ~0ull; std::vector<ID3D11Buffer*> pool; };
struct RpCbKeep { ID3D11Buffer* eng; RpSnap sn; };
// REPLAY_CPUCB - the recorder's constants from the CPU. Copying a rewritable constant buffer on the GPU between the
// engine's volume draws cost that pass ~3 us a copy (~1.25 ms a frame at ~390 copies); ~92% of them were of a buffer
// the engine uploaded while the volume buffer was bound. The upload funnel reads every such upload before the runtime
// sees its Unmap (or from UpdateSubresource's own data), so while the volume buffer is bound it keeps a whole image of
// the buffer (rp_img_note): exactly what the GPU reads, since nothing can write the buffer between that read and a
// draw without rp_cb_forget dropping the image first - every writer hook calls it, a Map included (a Map the funnel
// does not capture leaves no image), and a command list drops them all; an update made while a predicate is bound,
// which D3D may skip, leaves none either (RP_IMG_PRED). A draw's snapshot then copies the image into a page of our
// own (rp_img_slot) instead of a GPU copy, and the replay uploads the pages with one Map each (rp_img_upload).
// The slot is bound through a context-1 window moved into the page and cut at the image's end, which is zero-padded to
// a 16-constant boundary: every read inside the buffer sees its bytes, every read past it 0, as on the engine's buffer.
// Only with offsetting (g_cb_offsetting), every writer hooked (g_rp_rec.cbreuse_on) and a window the recorder read
// (RpDraw::cb1, a nonzero count); anything else is copied on the GPU. Images pin their buffers and are dropped at the
// pass's end, at the main depth clear, and with the session and the device.
struct RpImg { ID3D11Buffer* eng; uint32_t off, bytes; };
struct RpDyn { ID3D11Buffer* ours = nullptr; UINT bytes = 0, bind = 0; uintptr_t eng = 0; uint64_t pass = ~0ull; };
// REPLAY_SPLIT copies. RP_SPLIT_PRUNE: an entry is taken by a split of its size and bind flags that no split of
// this pass holds (pass = the last pass, g_rp_rec.pass, that took it), else one is made - a size the pool lacks is
// added, never swapped in for another's (ring sizes that alternate keep theirs); an entry no pass has taken for more
// than RP_DYN_KEEP frames goes at a pass's end. Held: per size, the most splits one pass of those frames made.
struct RpSplit { ID3D11Buffer* b = nullptr; UINT bytes = 0, bind = 0; uint64_t pass = ~0ull; };
// REPLAY_SCISSOR: scissor-enabled twins of the rasterizer states the replay binds.
struct RpRs { ID3D11RasterizerState* eng = nullptr; ID3D11RasterizerState* ours = nullptr; };
// MIR_REPLAY: depth-clamp twins of the rasterizer states the mirror replay binds ([0] as the engine's, [1] with
// the scissor on).
struct RpRsMir { ID3D11RasterizerState* eng = nullptr; ID3D11RasterizerState* ours[2] = {}; };
// VOL_REPLAY's recorder and replay: the recorded draws and passes, the targets and depth / stencil copies, the
// constant-buffer pool and keep list, the image arena and staging, the front pass, the rasterizer states.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: c, cbreuse_on.
// Session reset: rp_globals_reset resets 3 of them. session_scratch_reset resets 8 of them. reset_stat_counters
// resets 1 of them.
struct ReplayRecorder {
    RpCounters c;
    std::vector<RpDraw> draws;   // RP_UNCAPPED: [0, g_rp_rec.n) this pass's; the rest released, kept.
    uint32_t n = 0;
    uint64_t pass = ~0ull;      // The g_svs.frame_seq of the pass being / last recorded.
    bool     ok = false, ended = false, replayed = false;
    ID3D11Texture2D*          pre = nullptr;    // The buffer at the pass's first draw (replay A's start).
    ID3D11Texture2D*          tgt = nullptr;    // Replay A.
    ID3D11DepthStencilView*   dsv = nullptr;
    ID3D11ShaderResourceView* sten = nullptr;
    uint64_t eng_seq = ~0ull;                   // The pass whose engine snapshot replay B's target holds.
    uint64_t vol_clear_seq = ~0ull;             // The frame the engine last cleared the buffer in.
    // Replay B. Its start is the buffer at the world seam's start - the engine's depth only - which rp_snap_engine
    // copies into it there; nothing else writes it until that pass's replay.
    ID3D11Texture2D*          tgtb = nullptr;
    ID3D11DepthStencilView*   dsvb = nullptr;
    ID3D11ShaderResourceView* stenb = nullptr;
    ID3D11Texture2D*          mst_tex = nullptr;   // The merged pair.
    ID3D11RenderTargetView*   mst_rtv = nullptr;
    ID3D11Texture2D*          mft_tex = nullptr;
    ID3D11RenderTargetView*   mft_rtv = nullptr;
    uint32_t w = 0, h = 0;
    std::vector<RpCbCls> cbpool;
    // REPLAY_SPLIT: the rewritable vertex / index buffers a record of the open pass still flags (vb_dyn / ib_dyn) -
    // the buffers an engine write splits the pass at (rp_split). Added by rp_record, removed by the split that
    // re-points them, cleared with the records and at the pass's end; the records' references pin the keys.
    RpPtrMap<ID3D11Buffer*> dynset;
    RpPtrMap<RpCbKeep> cbkeep;
    std::atomic<uint32_t> cbkeep_n{ 0 };   // Entries of both maps (the reuse's and the images').
    // XTHREAD_ATOMIC: written by rp_extra_hooks (game thread, ensure_reorder_hook), read by the render thread.
    std::atomic<bool> cbreuse_on{ false };
    RpPtrMap<RpImg> img;            // Under g_rp_cbkeep_mx, as the arena.
    std::vector<uint8_t> img_arena;
    std::vector<ID3D11Buffer*> pages;    // DYNAMIC pages (render thread, as the stage).
    std::vector<uint8_t> stage;          // Their contents, page after page.
    uint32_t stage_used = 0;             // Bytes this pass; a slot never straddles a page.
    std::vector<RpDyn> dyn;                    // Pass-end copies of the rewritable vertex / index buffers.
    std::vector<RpSplit> split_pool;
    uint32_t wsplit_n = 0;
    uint64_t wsplit_bytes = 0;
    // The in-front footprint: where in the pass the hands seam came (the draw it preceded) and its slice viewport.
    uint64_t       front_seq = ~0ull;
    uint32_t       front_idx = 0;
    D3D11_VIEWPORT front_vp = {};
    std::vector<RpRs> rs;
    std::vector<RpRsMir> rs_mir;
    RpPtrMap<RpBufDesc> desc;
};
static ReplayRecorder g_rp_rec;
static std::mutex g_rp_cbkeep_mx;
static constexpr uint32_t RP_PAGE = 65536u;   // One page: 4096 constants, the largest window.
inline void rp_cb_count() {   // The caller holds g_rp_cbkeep_mx.
    g_rp_rec.cbkeep_n.store(g_rp_rec.cbkeep.n + g_rp_rec.img.n, std::memory_order_release);
}
// MAP_BRANCH: true when it took g_rp_cbkeep_mx (hooked_map counts it; the other callers ignore it).
inline bool rp_cb_forget(ID3D11Resource* r) {
    if (!r || g_rp_rec.cbkeep_n.load(std::memory_order_acquire) == 0u) return false;
    std::lock_guard<std::mutex> l(g_rp_cbkeep_mx);
    if (stats_on()) fun_any(FUNNEL_RP_LOCKS, 1u);   // MAP_BRANCH: one relaxed load, on the locked path only.
    RpCbKeep e = {};
    if (g_rp_rec.cbkeep.erase(static_cast<const void*>(r), &e)) {
        e.eng->Release();   // The entry's reference; the writer holds its own, so never the last.
    }
    RpImg i = {};
    if (g_rp_rec.img.erase(static_cast<const void*>(r), &i)) i.eng->Release();   // REPLAY_CPUCB.
    rp_cb_count();
    return true;
}
// RP_UNLOCKED_RELEASE: one of the two locked maps emptied, its pins released after g_rp_cbkeep_mx is let go. A pin
// can be its buffer's last reference (the engine may drop a buffer while it is listed), and a final Release enters
// the runtime's destruction - and any layer wrapping it - which must not run under this mutex (RP_ONE_LOCK: no
// D3D call under it). The entries leave the map under the lock, with also (the images' arena); their storage
// comes back once they are released unless the map was put into meanwhile, so a warmed-up pass allocates nothing.
template <typename V, typename RfA> inline void rp_keep_release(RpPtrMap<V>& m, RfA&& also) {
    RpPtrMap<V> out;
    {
        std::lock_guard<std::mutex> l(g_rp_cbkeep_mx);
        if (m.n != 0u) m.swap(out);   // The map is left empty, with no storage until the return.
        also();
        rp_cb_count();
    }
    if (out.n == 0u) return;
    out.clear([](V& e) { e.eng->Release(); });   // The entries' references (may be the last): unlocked.
    std::lock_guard<std::mutex> l(g_rp_cbkeep_mx);
    if (m.t.empty()) m.swap(out);   // Nothing put since: the emptied storage goes back.
}
inline void rp_cb_keep_clear() { rp_keep_release(g_rp_rec.cbkeep, [] {}); }
// REPLAY_CPUCB: every image dropped (the pass's end, the main depth clear, a command list, the resets).
inline void rp_img_clear() { rp_keep_release(g_rp_rec.img, [] { g_rp_rec.img_arena.clear(); }); }
// REPLAY_CPUCB: the funnel's upload of res (d, n bytes: the buffer's width unless it is past the
// scratch), while the volume buffer is bound - the whole buffer as the GPU will read it. Render thread.
// RP_IMG_PRED: pctx (UpdateSubresource's context; null for an Unmap, which no predicate skips) - while it has
// a predicate bound, D3D may skip the update, so no image is noted: the draws' snapshots then copy the buffer on the
// GPU, behind the update or its skip (PRED_ASIDE), as it stands.
inline void rp_img_note(ID3D11Resource* r, const void* d, uint32_t n,
                        ID3D11DeviceContext* pctx = nullptr) {
    if (!g_svs.vol_dsv_bound || !g_rp_rec.cbreuse_on.load(std::memory_order_relaxed) || !g_cb_offsetting || !r ||
        !d || n == 0) return;
    if (shadows_off()) return;   // SHADOW_OFF: nothing replays them.
    // RP_IMG_IDLE: this frame's pass ended or refused - no draw snapshots an image before the main depth clear
    // drops them all and moves the epoch (one pass a frame); an upload ahead of the pass's first draw is still noted.
    if (g_rp_rec.pass == g_svs.frame_seq && (g_rp_rec.ended || !g_rp_rec.ok)) return;
    if (proj_upload_byte_width(r) != n) return;   // Whole buffers only.
    if (pctx) {   // RP_IMG_PRED: one GetPredication per captured update that reached here.
        ID3D11Predicate* p = nullptr;
        BOOL pv = FALSE;
        pctx->GetPredication(&p, &pv);
        const bool pred = p != nullptr;
        KH_SAFE_RELEASE(p);
        if (pred) return;
    }
    upload_need(d, n);   // UPLOAD_LAZY: all of it.
    std::lock_guard<std::mutex> l(g_rp_cbkeep_mx);
    const uint32_t off = static_cast<uint32_t>(g_rp_rec.img_arena.size());
    g_rp_rec.img_arena.insert(g_rp_rec.img_arena.end(), static_cast<const uint8_t*>(d),
                              static_cast<const uint8_t*>(d) + n);
    ID3D11Buffer* const b = static_cast<ID3D11Buffer*>(r);
    if (RpImg* const e = g_rp_rec.img.find(r)) {
        e->off = off;
        e->bytes = n;
    } else if (g_rp_rec.img.put(r, RpImg{ b, off, n })) {
        b->AddRef();   // The image's pin (rp_cb_forget / rp_img_clear release it).
    }
    rp_cb_count();
}
// REPLAY_CPUCB: eng's image (bytes long) into this pass's pages - sn names the page and the slot. False =
// no image of that width, or no page could be made (the caller copies on the GPU). Render thread, with
// g_rp_cbkeep_mx held by the caller in l (RP_ONE_LOCK) and held again on return; released only while a
// page is made, so that no D3D call runs under it (the pages and the staging are the render thread's own).
inline bool rp_img_slot(ID3D11DeviceContext* ctx, ID3D11Buffer* eng, UINT bytes, RpSnap& sn,
                        std::unique_lock<std::mutex>& l) {
    const UINT c = ((bytes + 255u) / 256u) * 16u;   // Constants, a multiple of 16.
    const uint32_t b = c * 16u;
    if (b > RP_PAGE || bytes == 0) return false;
    uint32_t at = g_rp_rec.stage_used;
    if (at % RP_PAGE + b > RP_PAGE) at = (at / RP_PAGE + 1u) * RP_PAGE;
    const uint32_t p = at / RP_PAGE;
    if (p >= g_rp_rec.pages.size()) {
        g_rp_rec.pages.reserve(g_rp_rec.pages.size() + 1u);   // RP_THROW: a throw here owns nothing yet.
        ID3D11Buffer* nb = nullptr;
        l.unlock();   // RP_ONE_LOCK: no D3D call under the lock (a wrapping layer may take its own).
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (dev) {
            D3D11_BUFFER_DESC d = {};
            d.ByteWidth = RP_PAGE;
            d.Usage = D3D11_USAGE_DYNAMIC;
            d.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            d.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if (FAILED(dev->CreateBuffer(&d, nullptr, &nb))) nb = nullptr;
            dev->Release();
        }
        l.lock();
        if (!nb) return false;
        g_rp_rec.pages.push_back(nb);   // Within the reserve: cannot throw.
        if (p >= g_rp_rec.pages.size()) return false;
    }
    if (g_rp_rec.stage.size() < static_cast<size_t>(p + 1u) * RP_PAGE) {
        g_rp_rec.stage.resize(static_cast<size_t>(p + 1u) * RP_PAGE);
    }
    {   // The caller's lock (RP_ONE_LOCK).
        const RpImg* const i = g_rp_rec.img.find(eng);
        if (!i || i->bytes != bytes) return false;
        memcpy(g_rp_rec.stage.data() + at, g_rp_rec.img_arena.data() + i->off, bytes);
    }
    memset(g_rp_rec.stage.data() + at + bytes, 0, b - bytes);   // Past the buffer: 0, as the engine's reads.
    sn.buf = g_rp_rec.pages[p];
    sn.base_c = (at % RP_PAGE) / 16u;
    sn.slot_c = c;
    g_rp_rec.stage_used = at + b;
    return true;
}
// REPLAY_CPUCB: this pass's pages, one Map each, before anything replays them. Render thread, in the injection.
inline bool rp_img_upload(ID3D11DeviceContext* ctx) {
    const uint32_t n = (g_rp_rec.stage_used + RP_PAGE - 1u) / RP_PAGE;
    for (uint32_t p = 0; p < n; ++p) {
        if (p >= g_rp_rec.pages.size() || !g_rp_rec.pages[p]) return false;
        D3D11_MAPPED_SUBRESOURCE m = {};
        if (FAILED(ctx->Map(g_rp_rec.pages[p], 0, D3D11_MAP_WRITE_DISCARD, 0, &m)) || !m.pData) return false;
        const uint32_t b = g_rp_rec.stage_used - p * RP_PAGE < RP_PAGE ? g_rp_rec.stage_used - p * RP_PAGE
                                                                             : RP_PAGE;
        memcpy(m.pData, g_rp_rec.stage.data() + static_cast<size_t>(p) * RP_PAGE, b);
        ctx->Unmap(g_rp_rec.pages[p], 0);
    }
    return true;
}
// RP_DYN_PRUNE: frames (g_rp_rec.pass) a copy outlives use.
static constexpr uint64_t RP_DYN_KEEP = 1024;
// RP_SPLIT_CAP: a write split (RP_WRITE_SPLIT; a DISCARD map's split - a ring's wrap - is not capped) costs a
// whole-buffer GPU copy, a pooled buffer of that size held for RP_DYN_KEEP frames and a walk of the records; an
// engine that rewrote a vertex buffer before every volume draw would pay that per draw. The pass is refused at the
// write split that would pass RP_WSPLIT_MAX splits or RP_WSPLIT_BYTES copied (the frame keeps the copy, as every
// refusal): 64 rewrites of buffers the pass's draws read is far past a pass that mends a few, and 64 MB of copies
// (~0.15 ms at a 3080 Ti's ~900 GB/s, read and written; as much pooled VRAM) bounds the worst pass. A write anywhere in
// a flagged buffer splits, whether or not a recorded draw read the region it writes, so the cap can refuse a pass whose
// replay needed no split (a flagged buffer over 64 MB at its first write): stencilReplay counts those refusals. This
// pass's count and bytes, reset with the records.
static constexpr uint32_t RP_WSPLIT_MAX = 64u;
static constexpr uint64_t RP_WSPLIT_BYTES = 64ull << 20;
inline void rp_desc_clear() { g_rp_rec.desc.clear([](RpBufDesc& e) { e.b->Release(); }); }
inline void rp_release_records() {
    for (uint32_t i = 0; i < g_rp_rec.n; ++i) g_rp_rec.draws[i].release();
    g_rp_rec.n = 0;
    g_rp_rec.dynset.clear();   // REPLAY_SPLIT: no record flags a buffer now.
    rp_cb_keep_clear();   // REPLAY_CBREUSE: before the pool it names is reused.
    for (auto& c : g_rp_rec.cbpool) c.used = 0;   // RP_CB_EXACT.
    g_rp_rec.wsplit_n = 0;   // RP_SPLIT_CAP.
    g_rp_rec.wsplit_bytes = 0;
    g_rp_rec.stage_used = 0;   // REPLAY_CPUCB: the pages are this pass's again (the images are not the records').
    rp_desc_clear();   // RP_DESC_CACHE.
}
inline void rp_release_if_held() { if (g_rp_rec.n != 0) rp_release_records(); }   // SHADOW_OFF.
// RP_DEAD_RELEASE: at the main depth clear, after the epoch moved - the records of a pass nothing can replay any
// more let their engine objects go, and the reuse map they listed empties (left listed, it would keep every engine Map
// on the locked path): a pass that never reached its bracket (rp_pass_end needs its own epoch, rp_replay an ended
// pass) or one already replayed (REPLAY_ONCE; its cycle ends at this clear, and rp_mir_finish needs that
// cycle). An ended pass not yet replayed is kept - the injection after this clear replays it.
inline void rp_release_dead() {
    if (g_rp_rec.n != 0 && (!g_rp_rec.ended || g_rp_rec.replayed)) rp_release_records();
}
inline void rp_release_all() {   // Device loss and session teardown.
    rp_release_records();
    std::vector<RpDraw>().swap(g_rp_rec.draws);   // RP_UNCAPPED: the storage too (every record released above).
    g_rp_rec.dynset.free();
    g_rp_rec.desc.free();   // RP_DESC_CACHE: emptied by rp_release_records.
    rp_img_clear();   // REPLAY_CPUCB.
    {
        std::lock_guard<std::mutex> l(g_rp_cbkeep_mx);
        g_rp_rec.cbkeep.free();   // Emptied by rp_release_records.
        g_rp_rec.img.free();
        std::vector<uint8_t>().swap(g_rp_rec.img_arena);
    }
    for (auto& b : g_rp_rec.pages) KH_SAFE_RELEASE(b);
    g_rp_rec.pages.clear();
    std::vector<uint8_t>().swap(g_rp_rec.stage);
    KH_SAFE_RELEASE(g_rp_rec.sten); KH_SAFE_RELEASE(g_rp_rec.dsv); KH_SAFE_RELEASE(g_rp_rec.tgt);
    KH_SAFE_RELEASE(g_rp_rec.pre);
    KH_SAFE_RELEASE(g_rp_rec.stenb); KH_SAFE_RELEASE(g_rp_rec.dsvb); KH_SAFE_RELEASE(g_rp_rec.tgtb);
    KH_SAFE_RELEASE(g_rp.msten_srv); KH_SAFE_RELEASE(g_rp_rec.mst_rtv); KH_SAFE_RELEASE(g_rp_rec.mst_tex);
    KH_SAFE_RELEASE(g_rp.mfoot_srv); KH_SAFE_RELEASE(g_rp_rec.mft_rtv); KH_SAFE_RELEASE(g_rp_rec.mft_tex);
    g_rp_rec.w = g_rp_rec.h = 0;
    for (auto& cls : g_rp_rec.cbpool) { for (auto& b : cls.pool) KH_SAFE_RELEASE(b); }   // RP_CB_EXACT.
    g_rp_rec.cbpool.clear();
    for (auto& d : g_rp_rec.dyn) KH_SAFE_RELEASE(d.ours);
    g_rp_rec.dyn.clear();
    for (auto& x : g_rp_rec.split_pool) KH_SAFE_RELEASE(x.b);
    g_rp_rec.split_pool.clear();
    for (auto& x : g_rp_rec.rs) { KH_SAFE_RELEASE(x.eng); KH_SAFE_RELEASE(x.ours); }
    g_rp_rec.rs.clear();
    rp_mir_release();   // MIR_REPLAY.
    for (auto& x : g_rp_rec.rs_mir) { KH_SAFE_RELEASE(x.eng); KH_SAFE_RELEASE(x.ours[0]); KH_SAFE_RELEASE(x.ours[1]); }
    g_rp_rec.rs_mir.clear();
    g_rp_rec.pass = ~0ull;
    g_rp_rec.ok = g_rp_rec.ended = g_rp_rec.replayed = false;
    g_rp.merge_ok = false;
    g_rp_rec.eng_seq = ~0ull;
    g_rp_rec.vol_clear_seq = ~0ull;
    g_rp_rec.front_seq = ~0ull;
}
inline bool rp_ensure_tex(ID3D11Device* dev) {
    if (!dev || !g_svs.vol_src) return false;
    D3D11_TEXTURE2D_DESC sd = {};
    g_svs.vol_src->GetDesc(&sd);
    if (g_rp_rec.pre && g_rp_rec.tgt && g_rp_rec.dsv && g_rp_rec.sten && g_rp_rec.tgtb && g_rp_rec.dsvb &&
        g_rp_rec.stenb && g_rp_rec.mst_rtv &&
        g_rp.msten_srv && g_rp_rec.mft_rtv && g_rp.mfoot_srv && g_rp_rec.w == sd.Width &&
            g_rp_rec.h == sd.Height) return true;
    KH_SAFE_RELEASE(g_rp_rec.sten); KH_SAFE_RELEASE(g_rp_rec.dsv); KH_SAFE_RELEASE(g_rp_rec.tgt);
    KH_SAFE_RELEASE(g_rp_rec.pre);
    KH_SAFE_RELEASE(g_rp_rec.stenb); KH_SAFE_RELEASE(g_rp_rec.dsvb); KH_SAFE_RELEASE(g_rp_rec.tgtb);
    KH_SAFE_RELEASE(g_rp.msten_srv); KH_SAFE_RELEASE(g_rp_rec.mst_rtv); KH_SAFE_RELEASE(g_rp_rec.mst_tex);
    KH_SAFE_RELEASE(g_rp.mfoot_srv); KH_SAFE_RELEASE(g_rp_rec.mft_rtv); KH_SAFE_RELEASE(g_rp_rec.mft_tex);
    g_rp.merge_ok = false;
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
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_rec.pre))) { g_rp_rec.pre = nullptr; return false; }
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_rec.tgt))) { g_rp_rec.tgt = nullptr; return false; }
    if (FAILED(dev->CreateDepthStencilView(g_rp_rec.tgt, &dd, &g_rp_rec.dsv))) { g_rp_rec.dsv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_rec.tgt, &vd, &g_rp_rec.sten))) { g_rp_rec.sten = nullptr;
        return false; }
    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_rp_rec.tgtb))) { g_rp_rec.tgtb = nullptr; return false; }
    if (FAILED(dev->CreateDepthStencilView(g_rp_rec.tgtb, &dd, &g_rp_rec.dsvb))) { g_rp_rec.dsvb = nullptr;
        return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_rec.tgtb, &vd, &g_rp_rec.stenb))) { g_rp_rec.stenb = nullptr;
        return false; }
    D3D11_TEXTURE2D_DESC md = {};
    md.Width = sd.Width; md.Height = sd.Height; md.MipLevels = 1; md.ArraySize = 1; md.SampleDesc.Count = 1;
    md.Usage = D3D11_USAGE_DEFAULT;
    md.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    md.Format = DXGI_FORMAT_R8G8_UINT;
    if (FAILED(dev->CreateTexture2D(&md, nullptr, &g_rp_rec.mst_tex))) { g_rp_rec.mst_tex = nullptr; return false; }
    if (FAILED(dev->CreateRenderTargetView(g_rp_rec.mst_tex, nullptr,
        &g_rp_rec.mst_rtv))) { g_rp_rec.mst_rtv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_rec.mst_tex, nullptr, &g_rp.msten_srv))) { g_rp.msten_srv = nullptr;
        return false; }
    md.Format = DXGI_FORMAT_R32_FLOAT;
    if (FAILED(dev->CreateTexture2D(&md, nullptr, &g_rp_rec.mft_tex))) { g_rp_rec.mft_tex = nullptr; return false; }
    if (FAILED(dev->CreateRenderTargetView(g_rp_rec.mft_tex, nullptr,
        &g_rp_rec.mft_rtv))) { g_rp_rec.mft_rtv = nullptr; return false; }
    if (FAILED(dev->CreateShaderResourceView(g_rp_rec.mft_tex, nullptr, &g_rp.mfoot_srv))) { g_rp.mfoot_srv = nullptr;
        return false; }
    g_rp_rec.w = sd.Width;
    g_rp_rec.h = sd.Height;
    return true;
}
// Any buffer the engine can still write (DYNAMIC or DEFAULT): the replay copies it rather than referencing it.
inline bool rp_is_mutable(ID3D11Buffer* b, UINT* bytes = nullptr, UINT* bind = nullptr) {
    if (!b) return false;
    if (const RpBufDesc* const e = g_rp_rec.desc.find(b)) {   // RP_DESC_CACHE.
        if (bytes) *bytes = e->bytes;
        if (bind) *bind = e->bind;
        return e->mut;
    }
    D3D11_BUFFER_DESC d = {};
    b->GetDesc(&d);
    if (bytes) *bytes = d.ByteWidth;
    if (bind) *bind = d.BindFlags;
    const bool mut = d.Usage != D3D11_USAGE_IMMUTABLE;
    try {   // A failed insert only means no cache: the answer above stands.
        if (g_rp_rec.desc.put(b, RpBufDesc{ b, d.ByteWidth, d.BindFlags, mut })) b->AddRef();
    } catch (...) {}
    return mut;
}
// A copy of a constant buffer (bytes long) as it stands now, whole, into a pooled buffer of the same width
// (RP_CB_EXACT).
inline ID3D11Buffer* rp_cb_snapshot(ID3D11DeviceContext* ctx, ID3D11Buffer* eng, UINT bytes) {
    RpCbCls* cls = nullptr;
    for (RpCbCls& x : g_rp_rec.cbpool) {
        if (x.bytes == bytes) { cls = &x; break; }
    }
    if (!cls) {
        g_rp_rec.cbpool.push_back(RpCbCls());
        cls = &g_rp_rec.cbpool.back();
        cls->bytes = bytes;
    }
    cls->pass = g_rp_rec.pass;
    ID3D11Buffer* ours = cls->used < cls->pool.size() ? cls->pool[cls->used] : nullptr;
    if (!ours) {
        cls->pool.reserve(cls->pool.size() + 1u);   // RP_THROW: a throw here owns nothing yet.
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
    // RP_SNAP_OWN: our copy - the copy hook takes it as ours (rp_engine_call false, no PIP target probe).
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    {
        PredAside pa;   // PRED_ASIDE: not skipped by a predicate the engine left bound at its draw.
        pa.begin(ctx);
        ctx->CopyResource(ours, eng);
        pa.end();
    }
    g_ro.in_injection = prev;
    ours->AddRef();   // The record's own reference; the pool keeps its.
    return ours;
}
// The record's rewritable constants: each buffer copied whole (once per draw, or once per pass while the engine does
// not write it - REPLAY_CBREUSE); every slot bound to it keeps its window. Do not copy windows only: they mismatch
// more than whole buffers. False = a copy could not be made (that slot is left unbound).
inline bool rp_cb_capture(ID3D11DeviceContext* ctx, RpDraw& r) {
    // RP_ONE_LOCK: one hold of g_rp_cbkeep_mx across the slots, the same steps in the same order, released only
    // around a GPU copy - rp_cb_snapshot copies through the hooked context, whose copy hook takes this mutex
    // (rp_cb_forget) - and around a page's creation (rp_img_slot: no D3D call under it). Taken when first needed;
    // the unique_lock releases it on every exit.
    std::unique_lock<std::mutex> l(g_rp_cbkeep_mx, std::defer_lock);
    // RP_REUSE_ONCE: read once - the image branch below may run only after the reuse lookup took the lock, and an
    // install round's store between two reads would break that (an unlock of a lock not held throws).
    const bool reuse = g_rp_rec.cbreuse_on.load(std::memory_order_relaxed);
    // Every slot's type first, outside the lock: rp_is_mutable asks D3D on a cache miss. The same answers the
    // loop would get - no slot's buffer changes before its own turn.
    bool mut[2][RP_CB];
    UINT cr_bytes[2][RP_CB];
    for (uint32_t st = 0; st < 2u; ++st) {
        for (uint32_t k = 0; k < RP_CB; ++k) {
            cr_bytes[st][k] = 0;
            mut[st][k] = rp_is_mutable(st ? r.pcb[k] : r.vcb[k], &cr_bytes[st][k]);
        }
    }
    struct CbSeen { ID3D11Buffer* eng; RpSnap sn; };
    CbSeen seen[2u * RP_CB];
    uint32_t nseen = 0;
    bool ok = true;
    for (uint32_t st = 0; st < 2u; ++st) {
        ID3D11Buffer** set = st ? r.pcb : r.vcb;
        UINT* const fst = st ? r.pcf : r.vcf;
        UINT* const num = st ? r.pcn : r.vcn;
        for (uint32_t k = 0; k < RP_CB; ++k) {
            const UINT bytes = cr_bytes[st][k];
            if (!mut[st][k]) continue;
            const bool win = r.cb1 && num[k] != 0;   // REPLAY_CPUCB: this slot can take a page window.
            RpSnap sn = {};
            bool listed = false;   // Already in seen.
            for (uint32_t q = 0; q < nseen && !sn.buf; ++q) {
                if (seen[q].eng == set[k] && (win || seen[q].sn.slot_c == 0)) { sn = seen[q].sn; listed = true; }
            }
            // REPLAY_CBREUSE: an earlier draw's, the buffer unwritten since.
            if (!sn.buf && reuse) {
                if (!l.owns_lock()) l.lock();
                const RpCbKeep* const e = g_rp_rec.cbkeep.find(set[k]);
                if (e && (win || e->sn.slot_c == 0)) sn = e->sn;
            }
            bool fresh = false;   // Made for this draw: the reuse keeps it.
            if (sn.buf) {
                sn.buf->AddRef();   // The record's reference.
            } else if (win && reuse && g_cb_offsetting &&
                       rp_img_slot(ctx, set[k], bytes, sn, l)) {
                // RP_ONE_LOCK: held - this branch runs only after the reuse lookup above, which took it.
                sn.buf->AddRef();   // REPLAY_CPUCB: the record's reference (the pages keep theirs).
                fresh = true;
                stat_inc(g_rp_rec.c.cb_cpu);
            } else {
                sn = RpSnap{};
                if (l.owns_lock()) l.unlock();   // RP_ONE_LOCK: its copy enters the hooks.
                sn.buf = rp_cb_snapshot(ctx, set[k], bytes);   // With the record's reference.
                if (!sn.buf) {
                    ok = false;
                    set[k]->Release();
                    set[k] = nullptr;
                    continue;
                }
                fresh = true;
                stat_inc(g_rp_rec.c.cb_gpu);
            }
            if (!listed && nseen < 2u * RP_CB) seen[nseen++] = CbSeen{ set[k], sn };
            if (fresh && reuse) {   // REPLAY_CBREUSE.
                if (!l.owns_lock()) l.lock();
                try {   // RP_THROW: a failed insert (its growth) lists nothing - a later draw copies afresh.
                    if (g_rp_rec.cbkeep.put(set[k], RpCbKeep{ set[k], sn })) {
                        set[k]->AddRef();   // The entry's own (released by rp_cb_forget / rp_cb_keep_clear).
                    }
                } catch (...) {}
                rp_cb_count();
            }
            ID3D11Buffer* use = sn.buf;
            if (sn.slot_c != 0) {   // REPLAY_CPUCB: the recorded window, moved into the page and cut at the image.
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
inline void rp_pass_begin(ID3D11DeviceContext* ctx) {
    rp_release_records();
    g_rp_rec.pass = g_svs.frame_seq;
    g_rp_rec.ok = true;
    g_rp_rec.ended = false;
    g_rp_rec.replayed = false;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(0, nullptr, &dsv);
    void* const id = dsv ? reorder_dsv_identity(dsv) : nullptr;
    KH_SAFE_RELEASE(dsv);
    if (!g_svs.vol_src || !id || id != g_svs.vol_src_id) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); return; }
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = rp_ensure_tex(dev);
    if (dev) dev->Release();
    if (!ok) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); return; }
    if (g_rp.withheld_seq == g_rp_rec.pass) return;
    // PRED_ASIDE: a draw hook's copy - not skipped by a predicate the engine left bound (EA_VOL_COPY).
    svs_copy_aside(ctx, g_rp_rec.pre, g_svs.vol_src);
}
// The volume buffer as the engine left it for this frame's count, before our world seam's footprint: replay B's start,
// copied into its target (g_rp_rec.tgtb).
inline void rp_snap_engine(ID3D11DeviceContext* ctx) {
    try {
        if (!ctx || !g_svs.vol_src) return;
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        const bool ok = rp_ensure_tex(dev);
        if (dev) dev->Release();
        if (!ok) return;
        // PRED_ASIDE: ahead of the seam's StateBackup - the copy is not skipped by a predicate (EA_VOL_COPY).
        svs_copy_aside(ctx, g_rp_rec.tgtb, g_svs.vol_src);
        g_rp_rec.eng_seq = g_svs.frame_seq;
    } catch (...) {}
}
// One engine draw with the volume buffer bound, after our own pre-draw work (the seams have run).
inline void rp_record(ID3D11DeviceContext* ctx, uint8_t kind, UINT a, UINT b, UINT c, INT d, UINT e) {
    try {
        if (ctx != g_reorder_target_ctx.load(std::memory_order_relaxed) || !reorder_on_render_thread()) return;
        if (g_ro.in_injection) return;
        const bool open = g_rp_rec.pass == g_svs.frame_seq && !g_rp_rec.ended;
        if (!g_svs.vol_dsv_now) {   // Bound beside a target: recorded inside a pass (depth and stencil on replay).
            if (!g_svs.vol_dsv_bound || !open) return;   // Never starts a pass.
        } else if (g_rp_rec.pass != g_svs.frame_seq) {   // The frame's first counting draw (one pass a frame).
            rp_pass_begin(ctx);
        }
        if (!g_rp_rec.ok || g_rp_rec.ended) return;
        ID3D11GeometryShader* gs = nullptr; ctx->GSGetShader(&gs, nullptr, nullptr);
        ID3D11HullShader* hs = nullptr; ctx->HSGetShader(&hs, nullptr, nullptr);
        ID3D11DomainShader* ds = nullptr; ctx->DSGetShader(&ds, nullptr, nullptr);
        const bool stages = gs || hs || ds;
        KH_SAFE_RELEASE(gs); KH_SAFE_RELEASE(hs); KH_SAFE_RELEASE(ds);
        if (stages) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); return; }
        // RP_UNCAPPED: doubling (a throw refuses the pass: the catch below).
        if (g_rp_rec.n >= g_rp_rec.draws.size()) {
            g_rp_rec.draws.resize(g_rp_rec.draws.size() < 64u ? 64u : g_rp_rec.draws.size() * 2u);
        }
        RpDraw& r = g_rp_rec.draws[g_rp_rec.n++];
        r.kind = kind; r.a = a; r.b = b; r.c = c; r.d = d; r.e = e;
        r.mir = g_vmir.pending;   // MIR_REPLAY.
        ctx->IAGetInputLayout(&r.il);
        ctx->IAGetPrimitiveTopology(&r.topo);
        ctx->IAGetVertexBuffers(0, RP_VB, r.vb, r.vbs, r.vbo);
        for (uint32_t k = 0; k < RP_VB; ++k) r.vb_dyn[k] = rp_is_mutable(r.vb[k]);
        ctx->IAGetIndexBuffer(&r.ib, &r.ifmt, &r.ioff);
        r.ib_dyn = rp_is_mutable(r.ib);
        for (uint32_t k = 0; k < RP_VB; ++k) {   // REPLAY_SPLIT.
            if (r.vb_dyn[k]) g_rp_rec.dynset.put(r.vb[k], r.vb[k]);
        }
        if (r.ib_dyn) g_rp_rec.dynset.put(r.ib, r.ib);
        ctx->VSGetShader(&r.vs, nullptr, nullptr);
        ctx->PSGetShader(&r.ps, nullptr, nullptr);
        ID3D11DeviceContext1* const c1 = g_cb_offsetting ? as_ctx1(ctx) : nullptr;
        r.cb1 = c1 != nullptr;
        if (c1) {
            c1->VSGetConstantBuffers1(0, RP_CB, r.vcb, r.vcf, r.vcn);
            c1->PSGetConstantBuffers1(0, RP_CB, r.pcb, r.pcf, r.pcn);
        } else {
            ctx->VSGetConstantBuffers(0, RP_CB, r.vcb);
            ctx->PSGetConstantBuffers(0, RP_CB, r.pcb);
        }
        if (!rp_cb_capture(ctx, r)) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); }   // Rewritable constants.
        ctx->VSGetShaderResources(0, RP_SRV, r.vsrv);   // RP_BIND_ALL.
        ctx->VSGetSamplers(0, RP_SMP, r.vsmp);
        ctx->PSGetShaderResources(0, RP_SRV, r.psrv);
        ctx->PSGetSamplers(0, RP_SMP, r.psmp);
        r.vb_n = static_cast<uint8_t>(rp_bound_n(r.vb, RP_VB));   // RP_BIND_HW.
        r.vsrv_n = static_cast<uint8_t>(rp_bound_n(r.vsrv, RP_SRV));
        r.psrv_n = static_cast<uint8_t>(rp_bound_n(r.psrv, RP_SRV));
        ctx->OMGetDepthStencilState(&r.dss, &r.sref);
        ctx->RSGetState(&r.rs);
        ctx->OMGetBlendState(&r.bs, r.bf, &r.bmask);
        r.nvp = RP_VPS; ctx->RSGetViewports(&r.nvp, r.vp);
        r.nsc = RP_VPS; ctx->RSGetScissorRects(&r.nsc, r.sc);
    } catch (...) {
        if (g_rp_rec.ok) stat_inc(g_rp_rec.c.refused);   // RP_THROW: counted once, as every refusal.
        g_rp_rec.ok = false;
    }
}
// Anything else writing the volume buffer inside a pass: the replay could not reproduce it. (0 a clear, 2 our world
// footprint, 4 a command list, 5 an indirect draw or DrawAuto, 6 a copy into the buffer. Our in-front footprint is not
// one: replay A lacks it, so the merge keeps the copy only where it changed the count - the in-front call site,
// reorder_pre_draw.)
inline void rp_foreign(int kind) {
    (void)kind;
    if (g_rp_rec.pass != g_svs.frame_seq || !g_rp_rec.ok || g_rp_rec.ended || g_rp_rec.n == 0) return;
    g_rp_rec.ok = false;
    stat_inc(g_rp_rec.c.refused);
}
// The hands seam's place in the pass (the in-front call site, before the draw it precedes is recorded).
inline void rp_front_note(const D3D11_VIEWPORT& vp) {
    g_rp_rec.front_seq = g_svs.frame_seq;
    g_rp_rec.front_idx = (g_rp_rec.pass == g_svs.frame_seq && !g_rp_rec.ended) ? g_rp_rec.n : 0u;
    g_rp_rec.front_vp = vp;
}
// RP_WRITE_SPLIT: a split may run - the engine's own call on the target context's render thread, inside this
// frame's open, unrefused pass with a draw recorded. A few loads and compares; the lookup after it is one probe of an
// empty map when no record flags a buffer.
inline bool rp_split_armed(ID3D11DeviceContext* self) {
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed) || !reorder_on_render_thread() ||
        g_ro.in_injection) return false;
    return g_rp_rec.pass == g_svs.frame_seq && !g_rp_rec.ended && g_rp_rec.ok && g_rp_rec.n != 0;
}
// REPLAY_SPLIT: res is about to be written by the engine, from a hook ahead of the runtime's call, so the copy
// queued here reads the contents the draws recorded so far used. When a record still flags it (g_rp_rec.dynset), it is
// copied into a pool buffer and every such record reads the copy (no pass-end copy for them); a later draw that binds
// it lists it again. wrap: a DISCARD map, counted as a ring wrap, else a write split, capped (getRenderStats
// "stencilReplay"). MAP_BRANCH:
// 0 = stood down before the lookup, 1 = looked the buffer up (on the render thread) and did not split it, 2 = split it.
inline int rp_split(ID3D11DeviceContext* self, ID3D11Resource* res, bool wrap) {
    if (!res || !rp_split_armed(self)) return 0;
    // RP_UNCAPPED: a lookup in the set of buffers a record still flags, not a walk of the records - this runs on
    // every DISCARD map and engine write inside the pass (one per draw or more), so a walk would make the pass
    // quadratic in its draws.
    ID3D11Buffer* const* const e = g_rp_rec.dynset.find(res);
    if (!e) return 1;
    ID3D11Buffer* const eng = *e;
    UINT bytes = 0, bind = 0;
    rp_is_mutable(eng, &bytes, &bind);
    if (!wrap) {   // RP_SPLIT_CAP: past the cap the pass is refused, nothing copied.
        if (g_rp_rec.wsplit_n >= RP_WSPLIT_MAX || g_rp_rec.wsplit_bytes + bytes > RP_WSPLIT_BYTES) {
            g_rp_rec.ok = false;
            stat_inc(g_rp_rec.c.refused);
            stat_inc(g_rp_rec.c.capped);
            return 1;
        }
        ++g_rp_rec.wsplit_n;
        g_rp_rec.wsplit_bytes += bytes;
    }
    RpSplit* sp = nullptr;   // RP_SPLIT_PRUNE: a free entry of this size and bind, else a new one.
    for (RpSplit& x : g_rp_rec.split_pool) {
        if (x.pass != g_rp_rec.pass && x.bytes == bytes && x.bind == bind) { sp = &x; break; }
    }
    if (!sp) {
        g_rp_rec.split_pool.reserve(g_rp_rec.split_pool.size() + 1u);   // RP_THROW: a throw here owns nothing yet.
        ID3D11Buffer* b = nullptr;
        ID3D11Device* dev = nullptr;
        self->GetDevice(&dev);
        D3D11_BUFFER_DESC d = {};
        d.ByteWidth = bytes;
        d.Usage = D3D11_USAGE_DEFAULT;
        d.BindFlags = bind & (D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER);
        if (!dev || FAILED(dev->CreateBuffer(&d, nullptr, &b))) b = nullptr;
        if (dev) dev->Release();
        if (!b) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); return 1; }
        RpSplit n;
        n.b = b;
        n.bytes = bytes;
        n.bind = bind;
        g_rp_rec.split_pool.push_back(n);   // Within the reserve: cannot throw.
        sp = &g_rp_rec.split_pool.back();
    }
    sp->pass = g_rp_rec.pass;
    ID3D11Buffer* const c = sp->b;
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    {
        PredAside pa;   // PRED_ASIDE: what the draws so far replay from - not skipped by a predicate.
        pa.begin(self);
        self->CopyResource(c, eng);
        pa.end();
    }
    g_ro.in_injection = prev;
    // Every draw so far that used it reads the copy (no pass-end copy for them).
    for (uint32_t i = 0; i < g_rp_rec.n; ++i) {
        RpDraw& r = g_rp_rec.draws[i];
        for (uint32_t k = 0; k < RP_VB; ++k) {
            if (r.vb_dyn[k] && r.vb[k] == eng) {
                r.vb[k]->Release(); c->AddRef(); r.vb[k] = c; r.vb_dyn[k] = false;
            }
        }
        if (r.ib_dyn && r.ib == eng) { r.ib->Release(); c->AddRef(); r.ib = c; r.ib_dyn = false; }
    }
    g_rp_rec.dynset.erase(eng);   // No record flags it now; a later draw that binds it adds it again.
    stat_inc(wrap ? g_rp_rec.c.splits : g_rp_rec.c.wsplits);   // RP_SPLIT_CAP: write splits apart.
    return 2;
}
// REPLAY_SPLIT: from the Map hook - a DISCARD map (a ring's wrap) splits; a NO_OVERWRITE map leaves, by its
// contract, the bytes the draws since the last split read, so the pass-end copy still serves them.
inline int rp_on_map(ID3D11DeviceContext* self, ID3D11Resource* res, D3D11_MAP type) {
    if (type != D3D11_MAP_WRITE_DISCARD) return 0;
    return rp_split(self, res, true);
}
// RP_WRITE_SPLIT: from the hooks of the engine's other writers into a buffer (UpdateSubresource(1), CopyResource,
// CopySubresourceRegion(1), CopyStructureCount, and a UAV bound at the output merger - hooked_omset_rts_and_uavs;
// nooverwrite: a D3D11_COPY_NO_OVERWRITE copy or update, which keeps the NO_OVERWRITE map's contract and does not
// split). A split that threw (its pool's growth) refuses the pass, as hooked_map's does - left armed, the draws would
// keep reading the buffer the write changes.
inline void rp_on_write(ID3D11DeviceContext* self, ID3D11Resource* res, bool nooverwrite) {
    if (nooverwrite) return;
    try {
        (void)rp_split(self, res, false);
    } catch (...) {
        if (g_rp_rec.ok) stat_inc(g_rp_rec.c.refused);   // RP_THROW: counted once, as every refusal.
        g_rp_rec.ok = false;
    }
}
// The stencil resolve (the bracket): the rewritable vertex / index buffers the pass still references, copied once.
inline void rp_pass_end(ID3D11DeviceContext* ctx) {
    if (g_rp_rec.pass != g_svs.frame_seq || g_rp_rec.ended) return;
    g_rp_rec.ended = true;
    rp_cb_keep_clear();   // REPLAY_CBREUSE: the pass is recorded.
    rp_img_clear();   // REPLAY_CPUCB: its images are in its pages.
    g_rp_rec.dynset.clear();   // REPLAY_SPLIT: likewise (rp_split stands down on an ended pass).
    // RP_SPLIT_PRUNE / RP_CB_EXACT: RP_DYN_PRUNE's rule at every pass's end (a refused one too), after the
    // reuse map that names pool buffers is cleared; a record still holding a released one keeps it (its reference).
    for (size_t pp_i = 0; pp_i < g_rp_rec.split_pool.size();) {
        RpSplit& s = g_rp_rec.split_pool[pp_i];
        if (s.pass != g_rp_rec.pass && (s.pass == ~0ull || g_rp_rec.pass - s.pass > RP_DYN_KEEP)) {
            KH_SAFE_RELEASE(s.b);
            s = g_rp_rec.split_pool.back();
            g_rp_rec.split_pool.pop_back();
        } else {
            ++pp_i;
        }
    }
    for (size_t pp_i = 0; pp_i < g_rp_rec.cbpool.size();) {
        RpCbCls& c = g_rp_rec.cbpool[pp_i];
        if (c.pass != g_rp_rec.pass && (c.pass == ~0ull || g_rp_rec.pass - c.pass > RP_DYN_KEEP)) {
            for (auto& b : c.pool) KH_SAFE_RELEASE(b);
            if (&c != &g_rp_rec.cbpool.back()) c = std::move(g_rp_rec.cbpool.back());
            g_rp_rec.cbpool.pop_back();
        } else {
            ++pp_i;
        }
    }
    if (!g_rp_rec.ok || g_rp_rec.n == 0) return;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) { g_rp_rec.ok = false; return; }
    // PRED_ASIDE: the copies below are the replay's geometry (svs_vol_copy's bracket, its rule) - not skipped by
    // a predicate the engine left bound at this draw.
    PredAside pa;
    pa.begin(ctx);
    auto ours_for = [&](ID3D11Buffer* eng) -> ID3D11Buffer* {
        UINT bytes = 0, bind = 0;
        rp_is_mutable(eng, &bytes, &bind);
        const uintptr_t id = reinterpret_cast<uintptr_t>(eng);
        RpDyn* slot = nullptr;
        for (auto& x : g_rp_rec.dyn) if (x.eng == id) { slot = &x; break; }
        if (!slot) { g_rp_rec.dyn.push_back(RpDyn()); slot = &g_rp_rec.dyn.back(); slot->eng = id; }
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
        if (slot->pass != g_rp_rec.pass) {
            ctx->CopyResource(slot->ours, eng);
            slot->pass = g_rp_rec.pass;
        }
        return slot->ours;
    };
    try {   // RP_THROW: a slot's growth that throws refuses the pass (its records half re-pointed) and lets dev go.
        for (uint32_t i = 0; i < g_rp_rec.n && g_rp_rec.ok; ++i) {
            RpDraw& r = g_rp_rec.draws[i];
            for (uint32_t k = 0; k < RP_VB; ++k) {
                if (!r.vb_dyn[k] || !r.vb[k]) continue;
                ID3D11Buffer* o = ours_for(r.vb[k]);
                if (!o) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); break; }
                r.vb[k]->Release(); o->AddRef(); r.vb[k] = o;
            }
            if (r.ib_dyn && r.ib && g_rp_rec.ok) {
                ID3D11Buffer* o = ours_for(r.ib);
                if (!o) { g_rp_rec.ok = false; stat_inc(g_rp_rec.c.refused); }
                else { r.ib->Release(); o->AddRef(); r.ib = o; }
            }
        }
    } catch (...) {
        g_rp_rec.ok = false;
        stat_inc(g_rp_rec.c.refused);
    }
    // RP_DYN_PRUNE: a copy no pass has named for more than RP_DYN_KEEP frames goes (g_rp_rec.pass is the frame
    // sequence, g_svs.frame_seq, bumped at every main-depth clear), at the end of a pass that reaches the copies
    // above; a pass refused before its end, or with no draw, returns ahead of them and adds no slot either.
    // The slots are keyed by an engine
    // buffer's address, which the engine frees and reuses, so without this every buffer a volume pass ever drew with
    // would keep a copy of its size for the session. A record still holding one keeps it alive (its own reference).
    for (size_t pd_i = 0; pd_i < g_rp_rec.dyn.size();) {
        RpDyn& pd_d = g_rp_rec.dyn[pd_i];
        if (pd_d.pass != g_rp_rec.pass && (pd_d.pass == ~0ull || g_rp_rec.pass - pd_d.pass > RP_DYN_KEEP)) {
            KH_SAFE_RELEASE(pd_d.ours);
            pd_d = g_rp_rec.dyn.back();
            g_rp_rec.dyn.pop_back();
        } else {
            ++pd_i;
        }
    }
    dev->Release();
}
// RP_UNWIND: runs f only when its scope is left by an exception - every normal exit does the same work
// itself, so the normal path is unchanged (UI_BB_UNWIND's rule). f must not throw.
template <typename F>
struct RpOnUnwind {
    F f;
    const int ex;
    explicit RpOnUnwind(F v) : f(v), ex(std::uncaught_exceptions()) {}
    ~RpOnUnwind() { if (std::uncaught_exceptions() > ex) f(); }
    RpOnUnwind(const RpOnUnwind&) = delete;
    RpOnUnwind& operator=(const RpOnUnwind&) = delete;
};
// Everything the replay touches, saved and restored whole (StateBackup covers only our own passes' slots).
// RP_UNWIND: a capture not yet restored is restored by the destructor (a throw inside the replay), and every
// reference released; the normal path's restore() clears armed and its release() nulls every pointer, so the
// destructor then does nothing (OmSave's and PredAside's own destructors find nothing either).
struct RpSave {
    ID3D11InputLayout* il = nullptr; D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* vb[RP_VB] = {}; UINT vbs[RP_VB] = {}, vbo[RP_VB] = {};
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT ifmt = DXGI_FORMAT_UNKNOWN; UINT ioff = 0;
    ID3D11VertexShader* vs = nullptr; ID3D11PixelShader* ps = nullptr;
    ID3D11GeometryShader* gs = nullptr; ID3D11HullShader* hs = nullptr; ID3D11DomainShader* ds = nullptr;
    bool cb1 = false;
    ID3D11Buffer* vcb[RP_CB] = {}; UINT vcf[RP_CB] = {}, vcn[RP_CB] = {};
    ID3D11Buffer* pcb[RP_CB] = {}; UINT pcf[RP_CB] = {}, pcn[RP_CB] = {};
    // RP_BIND_ALL: every slot a replayed draw binds (the merges' t28, t34, t41, t49 - t51 among them).
    ID3D11ShaderResourceView* vsrv[RP_SRV] = {}; ID3D11SamplerState* vsmp[RP_SMP] = {};
    ID3D11ShaderResourceView* psrv[RP_SRV] = {}; ID3D11SamplerState* psmp[RP_SMP] = {};
    ID3D11DepthStencilState* dss = nullptr; UINT sref = 0; ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* bs = nullptr; FLOAT bf[4] = {}; UINT bmask = 0xFFFFFFFFu;
    UINT nvp = 0, nsc = 0; D3D11_VIEWPORT vp[RP_VPS] = {}; D3D11_RECT sc[RP_VPS] = {};
    OmSave om;
    // PRED_ASIDE: the engine's predicate, set aside for the replay (its copies, clears and merge draws are ours; a
    // predicate the engine left bound at the injection's trigger draw - not the recorded draws' own, which the
    // recorder never kept - would skip them, leaving the merge targets a cycle stale) and handed back by restore.
    PredAside pred;
    ID3D11DeviceContext* ctx_saved = nullptr;   // RP_UNWIND.
    bool armed = false;                         // Captured, not yet restored.
    RpSave() = default;
    RpSave(const RpSave&) = delete;
    RpSave& operator=(const RpSave&) = delete;
    ~RpSave() {
        if (armed && ctx_saved) restore(ctx_saved);
        release();
    }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        pred.begin(ctx);
        ctx->IAGetInputLayout(&il); ctx->IAGetPrimitiveTopology(&topo);
        ctx->IAGetVertexBuffers(0, RP_VB, vb, vbs, vbo); ctx->IAGetIndexBuffer(&ib, &ifmt, &ioff);
        ctx->VSGetShader(&vs, nullptr, nullptr); ctx->PSGetShader(&ps, nullptr, nullptr);
        ctx->GSGetShader(&gs, nullptr, nullptr); ctx->HSGetShader(&hs, nullptr, nullptr); ctx->DSGetShader(&ds, nullptr, nullptr);
        ID3D11DeviceContext1* const c1 = g_cb_offsetting ? as_ctx1(ctx) : nullptr;
        cb1 = c1 != nullptr;
        if (c1) { c1->VSGetConstantBuffers1(0, RP_CB, vcb, vcf, vcn); c1->PSGetConstantBuffers1(0, RP_CB, pcb, pcf, pcn); }
        else { ctx->VSGetConstantBuffers(0, RP_CB, vcb); ctx->PSGetConstantBuffers(0, RP_CB, pcb); }
        ctx->VSGetShaderResources(0, RP_SRV, vsrv); ctx->VSGetSamplers(0, RP_SMP, vsmp);   // RP_BIND_ALL.
        ctx->PSGetShaderResources(0, RP_SRV, psrv); ctx->PSGetSamplers(0, RP_SMP, psmp);
        ctx->OMGetDepthStencilState(&dss, &sref); ctx->RSGetState(&rs); ctx->OMGetBlendState(&bs, bf, &bmask);
        nvp = RP_VPS; ctx->RSGetViewports(&nvp, vp); nsc = RP_VPS; ctx->RSGetScissorRects(&nsc, sc);
        om.capture(ctx);
    }
    void restore(ID3D11DeviceContext* ctx) {
        armed = false;
        ctx->IASetInputLayout(il); ctx->IASetPrimitiveTopology(topo);
        ctx->IASetVertexBuffers(0, RP_VB, vb, vbs, vbo); ctx->IASetIndexBuffer(ib, ifmt, ioff);
        // CLASS_INST: every shader comes back without class instances (the capture takes none) - the injection's
        // replay runs outside any StateBackup, so an engine shader bound with interfaces would come back not its own.
        // Accepted as StateBackup's GS / HS / DS are: the engine is not known to use dynamic linkage.
        ctx->VSSetShader(vs, nullptr, 0); ctx->PSSetShader(ps, nullptr, 0);
        ctx->GSSetShader(gs, nullptr, 0); ctx->HSSetShader(hs, nullptr, 0); ctx->DSSetShader(ds, nullptr, 0);
        ID3D11DeviceContext1* const c1 = cb1 ? as_ctx1(ctx) : nullptr;
        if (c1) { c1->VSSetConstantBuffers1(0, RP_CB, vcb, vcf, vcn); c1->PSSetConstantBuffers1(0, RP_CB, pcb, pcf, pcn); }
        else { ctx->VSSetConstantBuffers(0, RP_CB, vcb); ctx->PSSetConstantBuffers(0, RP_CB, pcb); }
        ctx->VSSetShaderResources(0, RP_SRV, vsrv); ctx->VSSetSamplers(0, RP_SMP, vsmp);   // RP_BIND_ALL.
        ctx->PSSetShaderResources(0, RP_SRV, psrv); ctx->PSSetSamplers(0, RP_SMP, psmp);
        bc_forget();   // BIND_FILTER: t0 - t127.
        ctx->OMSetDepthStencilState(dss, sref); ctx->RSSetState(rs); ctx->OMSetBlendState(bs, bf, bmask);
        // RP_SAVE_RS: the counts as captured, zero included - a zero unbinds, so a scissor rect of ours does
        // not stay behind an engine that had none bound (StateBackup's rule).
        ctx->RSSetViewports(nvp, vp);
        ctx->RSSetScissorRects(nsc, sc);
        om.restore(ctx);
        pred.end();   // PRED_ASIDE.
    }
    void release() {
        KH_SAFE_RELEASE(il); KH_SAFE_RELEASE(ib); KH_SAFE_RELEASE(vs); KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(gs); KH_SAFE_RELEASE(hs); KH_SAFE_RELEASE(ds);
        for (auto& x : vb) KH_SAFE_RELEASE(x);
        for (auto& x : vcb) KH_SAFE_RELEASE(x);
        for (auto& x : pcb) KH_SAFE_RELEASE(x);
        for (auto& x : vsrv) KH_SAFE_RELEASE(x);   // RP_BIND_ALL.
        for (auto& x : vsmp) KH_SAFE_RELEASE(x);
        for (auto& x : psrv) KH_SAFE_RELEASE(x);
        for (auto& x : psmp) KH_SAFE_RELEASE(x);
        KH_SAFE_RELEASE(dss); KH_SAFE_RELEASE(rs); KH_SAFE_RELEASE(bs);
        om.release();
    }
};
// REPLAY_SCISSOR: the scissor-enabled twin of a rasterizer state (null = the default state), cached per state.
inline ID3D11RasterizerState* rp_rs_scissor(ID3D11DeviceContext* ctx, ID3D11RasterizerState* eng) {
    for (const auto& x : g_rp_rec.rs) if (x.eng == eng) return x.ours;
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
    try {   // RP_UNWIND: room first - a throwing push_back would leak ours and the engine's reference.
        g_rp_rec.rs.reserve(g_rp_rec.rs.size() + 1u);
    } catch (...) { ours->Release(); throw; }
    if (eng) eng->AddRef();
    g_rp_rec.rs.push_back(RpRs{ eng, ours });
    return ours;
}
// MIR_REPLAY: the depth-clamp twin (vmir_begin's clone: DepthClipEnable off) of a rasterizer state, with the
// scissor forced on for sc, cached per state.
inline ID3D11RasterizerState* rp_rs_mir(ID3D11DeviceContext* ctx, ID3D11RasterizerState* eng, bool sc) {
    const int k = sc ? 1 : 0;
    for (const auto& x : g_rp_rec.rs_mir) if (x.eng == eng && x.ours[k]) return x.ours[k];
    D3D11_RASTERIZER_DESC d = {};
    if (eng) {
        eng->GetDesc(&d);
    } else {
        d.FillMode = D3D11_FILL_SOLID;
        d.CullMode = D3D11_CULL_BACK;
        d.DepthClipEnable = TRUE;
    }
    d.DepthClipEnable = FALSE;
    if (sc) d.ScissorEnable = TRUE;
    ID3D11RasterizerState* ours = nullptr;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (dev) { if (FAILED(dev->CreateRasterizerState(&d, &ours))) ours = nullptr; dev->Release(); }
    if (!ours) return nullptr;
    for (auto& x : g_rp_rec.rs_mir) if (x.eng == eng) { x.ours[k] = ours; return ours; }
    try {   // RP_UNWIND: room first (rp_rs_scissor's rule).
        g_rp_rec.rs_mir.reserve(g_rp_rec.rs_mir.size() + 1u);
    } catch (...) { ours->Release(); throw; }
    RpRsMir e;
    e.eng = eng;
    if (eng) eng->AddRef();
    e.ours[k] = ours;
    g_rp_rec.rs_mir.push_back(e);
    return ours;
}
// REPLAY_SCISSOR: the nearest a fragment of ours can be to the camera - ClipOwnNear's floor (cb.hlsl: KH_OWN_NEAR,
// 0.05 m, or a pass's own nearer near, the view-model slice's measured 0.01 m). A bounding sphere is clipped here
// rather than given up on: nothing of it nearer than this is ever drawn.
static constexpr float RP_CLIP_NEAR_M = 0.01f;
// MIR_BAND: a near-z routed mesh reads the mirror through PSComposite's near-plane fade, whose weight is
// saturate((w / near - 1) / (KH_STEN_FADE - 1)): only a fragment of view depth under RP_STEN_FADE x near takes the
// mirror's count into its verdict. Every other fragment takes lerp(mirror, count, 1) - the count, to within one float
// step (an accepted envelope for this gate) - so the mirror replay runs for such a mesh only when one of its fragments
// can lie in that band. HLSL twin KH_STEN_FADE (cb.hlsl).
static constexpr float RP_STEN_FADE = 1.35f;
// The band's padding, relative then absolute: the cycle latch's camera and projection against the drawing pass's
// (the same frame), on top of the bounding sphere's own padding.
static constexpr float RP_MIR_BAND_REL = 1.25f;
static constexpr float RP_MIR_BAND_PAD_M = 0.25f;
// MIR_BAND: the farthest (m, straight line from the camera) a fragment of view depth under RP_STEN_FADE x near
// can lie inside the frustum of the row-vector projection P - the band's depth times the frustum's widest corner ray
// (the jitter offsets included) - padded. Negative when P is unusable: the caller then counts the mesh a reader.
inline float rp_mir_band(const float P[4][4]) {
    const float w = P[2][3];
    if (!(w > 1.0e-6f) || !(P[2][2] > 1.0e-6f)) return -1.0f;
    const float n = -P[3][2] / P[2][2];
    const float ax = fabsf(P[0][0] / w), ay = fabsf(P[1][1] / w);
    if (!(n > 0.0f) || !(ax > 1.0e-6f) || !(ay > 1.0e-6f)) return -1.0f;
    const float tx = (1.0f + fabsf(P[2][0] / w)) / ax, ty = (1.0f + fabsf(P[2][1] / w)) / ay;
    const float r = RP_STEN_FADE * n * sqrtf(1.0f + tx * tx + ty * ty);
    return (r == r && r < 1.0e6f) ? r * RP_MIR_BAND_REL + RP_MIR_BAND_PAD_M : -1.0f;
}
// MIR_BAND: rp_obj_ndc's bounding sphere - its centre (engine axes) and radius (the rectangles' bound).
inline float rp_sphere_of(const RenderObject& o, float c[3]) {
    float bs[3];
    bounds_size_of(o, bs);
    c[0] = o.pos[0]; c[1] = o.pos[2]; c[2] = o.pos[1];   // Engine axes.
    return 1.25f * lod_radius_of(bs, o.rot_m, o.rotated) + 0.05f;
}
// MIR_BAND: the nearest the mesh's drawn box comes to cam (m, engine axes) - bounds_size_of's size (a cloth's
// reach and a vertex stage's bound included), taken as the world-axis box around its rotation, which holds every
// fragment the mesh can draw. The band test's bound; not the rectangles' sphere (1.25 x the box's half-diagonal): it
// bulges far past a large box's faces and swallows the camera beside a big mesh.
inline float rp_box_dist(const RenderObject& o, const float cam[3]) {
    return sqrtf(bounds_dist_sq(o, cam));   // NEARZ_BOUNDS: the one drawn-box distance.
}
// MIR_GATE - the real mirror (the world seam's mirror prepass and the re-issue of the counting draws into it) is
// built at the end of a cycle for the next cycle's passes, so it is built only when some visible mesh can read it
// then: an inFront, fading, translucent or alpha-material mesh (as the readers are always counted), or one whose
// drawn box lies within VMIR_REACH_MUL x the band's reach (MIR_BAND) plus VMIR_MOTION_M and four of
// the camera's last steps - what the camera and a mesh can close before that cycle draws. A cycle that wanted it
// holds it for VMIR_HOLD more. A reader the gate did not foresee (a teleport into a mesh, a mesh shown or moved
// there at once) reads no mirror for that one cycle - its near band loses the stencil shadow's near-plane repair
// for a frame: an accepted envelope. Render thread (the seam).
static constexpr float    VMIR_REACH_MUL = 2.0f;
static constexpr float    VMIR_MOTION_M = 1.0f;
static constexpr uint64_t VMIR_HOLD = 8u;
// MIR_BAND_RECT: a reader through its texels anywhere (an inFront mesh, a translucent texel) rather than through
// the near fade's band alone - the readers the gate, the rectangles and every pass always count.
inline bool rp_mir_flagged(const RenderObject& o) {
    return obj_in_front(o) || o.timed || o.color[3] < 0.999f || mat_set_has_alpha(o.materials);
}
inline bool vmir_wanted(const float cam[3], const float proj[4][4]) {
    const float band = rp_mir_band(proj);
    bool want = band < 0.0f;
    const float reach = band * VMIR_REACH_MUL + VMIR_MOTION_M +
                        4.0f * (g_cam_step_m > 0.0f ? g_cam_step_m : 0.0f);
    for (uint32_t i = 0; i < g_scene.objs.size() && !want; ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!obj_vis(o)) continue;   // SHOW_INFRONT.
        if (rp_mir_flagged(o) || rp_box_dist(o, cam) < reach) want = true;
    }
    if (want) g_vmir.want_cycle = g_topo_cycles;
    return want || (g_vmir.want_cycle != ~0ull && g_topo_cycles >= g_vmir.want_cycle &&
                    g_topo_cycles - g_vmir.want_cycle <= VMIR_HOLD);
}
// REPLAY_SCISSOR: one object's bound in NDC through the cycle latch - its bounding sphere (radius x1.25 + 5 cm, for
// the witness ring and skinned motion) clipped at RP_CLIP_NEAR_M. 0 = nothing of it can be drawn (behind the camera
// or wholly nearer than that), 1 = b holds [x0, y0, x1, y1], 2 = no bound (an inFront mesh, drawn under its own
// projection). The caller has checked the latch.
// REPLAY_RASTER: the latch's view, the screen terms of the replay's raster (the copy's where it parted).
inline int rp_obj_ndc(const RenderObject& o, const float (*v)[4], float b[4]) {
    if (obj_in_front(o)) return 2;
    const float (*V)[4] = v;   // RP_ADOPT: the caller's view (rp_view_of).
    const float (*Pm)[4] = g_ro.cycle_pv.projection;
    const bool r = g_rp.rast_on;   // REPLAY_RASTER.
    const float sx = r ? g_rp.rast[0] : Pm[0][0] / Pm[2][3], sy = r ? g_rp.rast[1] : Pm[1][1] / Pm[2][3];
    const float ox = r ? g_rp.rast[2] : Pm[2][0] / Pm[2][3], oy = r ? g_rp.rast[3] : Pm[2][1] / Pm[2][3];
    float c[3];
    const float rad = rp_sphere_of(o, c);   // MIR_BAND: the sphere, shared.
    const float vx = c[0] * V[0][0] + c[1] * V[1][0] + c[2] * V[2][0] + V[3][0];
    const float vy = c[0] * V[0][1] + c[1] * V[1][1] + c[2] * V[2][1] + V[3][1];
    const float vz = c[0] * V[0][2] + c[1] * V[1][2] + c[2] * V[2][2] + V[3][2];
    if (vz + rad <= RP_CLIP_NEAR_M) return 0;
    const float zn = fmaxf(vz - rad, RP_CLIP_NEAR_M), zf = vz + rad;
    const float tx0 = fminf((vx - rad) / zn, (vx - rad) / zf), tx1 = fmaxf((vx + rad) / zn, (vx + rad) / zf);
    const float ty0 = fminf((vy - rad) / zn, (vy - rad) / zf), ty1 = fmaxf((vy + rad) / zn, (vy + rad) / zf);
    b[0] = fminf(sx * tx0, sx * tx1) + ox;
    b[2] = fmaxf(sx * tx0, sx * tx1) + ox;
    b[1] = fminf(sy * ty0, sy * ty1) + oy;
    b[3] = fmaxf(sy * ty0, sy * ty1) + oy;
    return 1;
}
// MIR_BAND_RECT: where a band-only mirror reader (not rp_mir_flagged) can draw a fragment in the fade's band,
// as an NDC bound - its drawn box (rp_box_dist's: the world-axis box around the rotated, grown size) cut to view
// depth [RP_CLIP_NEAR_M, the band's depth] through the cycle latch, in the replay's raster (rp_obj_ndc's
// terms). The band's depth is RP_STEN_FADE x the latch's near, padded as the band's reach is. The cut box is
// convex and in front of the camera, so its screen bound is that of its corners inside the slab and of its edges'
// crossings of the two planes. 1 = b holds [x0, y0, x1, y1]; 0 = none of the box lies in the band; -1 = no
// usable projection (the caller takes the whole object's bound). The caller has checked the latch.
inline int rp_mir_band_ndc(const RenderObject& o, const float (*view)[4], float b[4]) {
    const float (*V)[4] = view;   // RP_ADOPT: the caller's view (rp_view_of).
    const float (*Pm)[4] = g_ro.cycle_pv.projection;
    if (!(Pm[2][2] > 1.0e-6f) || !(fabsf(Pm[2][3]) > 1.0e-6f)) return -1;
    const float n = -Pm[3][2] / Pm[2][2];
    if (!(n > 0.0f)) return -1;
    const float zb = RP_STEN_FADE * n * RP_MIR_BAND_REL + RP_MIR_BAND_PAD_M;
    const float zl = RP_CLIP_NEAR_M;
    const bool rs = g_rp.rast_on;   // REPLAY_RASTER: rp_obj_ndc's terms.
    const float sx = rs ? g_rp.rast[0] : Pm[0][0] / Pm[2][3], sy = rs ? g_rp.rast[1] : Pm[1][1] / Pm[2][3];
    const float ox = rs ? g_rp.rast[2] : Pm[2][0] / Pm[2][3], oy = rs ? g_rp.rast[3] : Pm[2][1] / Pm[2][3];
    float s[3];
    bounds_size_of(o, s);
    const float hl[3] = { s[0] * 0.5f, s[2] * 0.5f, s[1] * 0.5f };   // Engine axes.
    float he[3];
    rot_half_extents(hl, o.rot_m, o.rotated, he);
    const float c[3] = { o.pos[0], o.pos[2], o.pos[1] };
    float v[8][3];
    for (int k = 0; k < 8; ++k) {
        const float p[3] = { c[0] + ((k & 1) ? he[0] : -he[0]), c[1] + ((k & 2) ? he[1] : -he[1]),
                             c[2] + ((k & 4) ? he[2] : -he[2]) };
        for (int j = 0; j < 3; ++j) v[k][j] = p[0] * V[0][j] + p[1] * V[1][j] + p[2] * V[2][j] + V[3][j];
    }
    bool any = false;
    float x0 = 1.0e30f, y0 = 1.0e30f, x1 = -1.0e30f, y1 = -1.0e30f;
    auto add = [&](float vx, float vy, float vz) {
        const float xn = sx * (vx / vz) + ox, yn = sy * (vy / vz) + oy;
        x0 = fminf(x0, xn); x1 = fmaxf(x1, xn); y0 = fminf(y0, yn); y1 = fmaxf(y1, yn);
        any = true;
    };
    for (int k = 0; k < 8; ++k) {
        if (v[k][2] >= zl && v[k][2] <= zb) add(v[k][0], v[k][1], v[k][2]);
    }
    for (int k = 0; k < 8; ++k) {
        for (int bit = 1; bit < 8; bit <<= 1) {
            if (k & bit) continue;   // Each of the twelve edges once, from its lower corner.
            const float* va = v[k];
            const float* vb = v[k | bit];
            const float pl[2] = { zl, zb };
            for (float zp : pl) {
                if ((va[2] - zp) * (vb[2] - zp) >= 0.0f) continue;   // No crossing (or an end on the plane).
                const float t = (zp - va[2]) / (vb[2] - va[2]);
                add(va[0] + (vb[0] - va[0]) * t, va[1] + (vb[1] - va[1]) * t, zp);
            }
        }
    }
    if (!any) return 0;
    b[0] = x0; b[1] = y0; b[2] = x1; b[3] = y1;
    return 1;
}
// MIR_REPLAY: an object that can read the mirror (t28): an inFront mesh (all of it), a translucent texel (an object
// below full alpha, one whose lifetime fades it, or one with an alpha material - PSMain / PSComposite's khStenTl), or
// a near-z routed mesh with a fragment that can lie in the fade's band (MIR_BAND, through the cycle latch's
// projection, its drawn box against cam - the latch's camera).
inline bool rp_mir_reader(const RenderObject& o, const float cam[3]) {
    if (rp_mir_flagged(o)) return true;
    const float r = g_ro.cycle_pv_valid ? rp_mir_band(g_ro.cycle_pv.projection) : -1.0f;
    return r < 0.0f || rp_box_dist(o, cam) < r;
}
// MIR_REPLAY: a mesh that reads the mirror in a pass drawing under proj from cam: rp_mir_reader's
// rule made the pass's own. A mesh reads the fade only when the pass routes it near-z - a near in (CAM_NEAR_MIN,
// 50) and its drawn box within near + 2 m (NEARZ_BOUNDS; the passes' other conditions only narrow that) - and
// only when that box comes within the band's reach (MIR_BAND).
inline bool rp_mir_reads(const RenderObject& o, const float proj[4][4], const float cam[3]) {
    if (rp_mir_flagged(o)) return true;
    const float mp_near = proj[2][2] > 1.0e-6f ? -proj[3][2] / proj[2][2] : 0.0f;
    if (!(mp_near > CAM_NEAR_MIN && mp_near < 50.0f)) return false;
    const float r = mp_near + 2.0f;
    const float d2 = bounds_dist_sq(o, cam);   // NEARZ_BOUNDS: the route's distance.
    if (!(d2 < r * r)) return false;
    const float band = rp_mir_band(proj);
    if (band < 0.0f) return true;
    return sqrtf(d2) < band;   // rp_box_dist's value.
}
// REPLAY_SCISSOR: an NDC bound (x0, y0, x1, y1) as a target rectangle, plus a 16 px margin; empty when nothing of it
// is on the target.
inline D3D11_RECT rp_px_of(float x0, float y0, float x1, float y1) {
    const float W = static_cast<float>(g_rp_rec.w), H = static_cast<float>(g_rp_rec.h), M = 16.0f;
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
inline bool rp_px_empty(const D3D11_RECT& r) { return r.right <= r.left || r.bottom <= r.top; }
inline void rp_rect_add(RpRect& r, const float b[4]) {
    r.x0 = fminf(r.x0, b[0]); r.y0 = fminf(r.y0, b[1]);
    r.x1 = fmaxf(r.x1, b[2]); r.y1 = fmaxf(r.y1, b[3]);
    r.any = true;
}
inline void rp_rect_close(RpRect& r) {
    if (r.full) return;
    r.px = r.any ? rp_px_of(r.x0, r.y0, r.x1, r.y1) : D3D11_RECT{ 0, 0, 0, 0 };
    if (rp_px_empty(r.px)) r.any = false;
}
// RP_ADOPT (FX_FRAME_ADOPT): the view the rectangles bound our meshes through - the cycle latch under the
// engine's frame rotation when fresh, the frame our meshes are drawn in (the injection and the flush adopt the same
// latch) and replay B's footprint with them. Not the bridge's own rotation: a fast turn puts the meshes' leading
// edge up to a frame of rotation outside the 16 px margin. The camera is the latch's (adoption keeps it).
inline void rp_view_of(float v[4][4]) {
    RVExtBridge::ProjectionViewTransform pv = g_ro.cycle_pv;
    adopt_frame_view(pv);
    memcpy(v, &pv.view[0][0], sizeof(float) * 16u);
}
// REPLAY_SCISSOR: the rectangles the replay draws in - every visible mesh of ours (the volume count) and the ones
// that can read the mirror (MIR_REPLAY) - through the cycle latch (RP_ADOPT), in the replay's raster
// (REPLAY_RASTER). Both are the whole target when the latch is not usable or an inFront mesh is visible.
inline void rp_scissor_rects(RpRect& all, RpRect& mir) {
    all = RpRect();
    mir = RpRect();
    if (!g_ro.cycle_pv_valid || g_rp_rec.w == 0 || g_rp_rec.h == 0) return;
    if (!(fabsf(g_ro.cycle_pv.projection[2][3]) > 1.0e-6f)) return;
    float v[4][4];
    rp_view_of(v);
    float cam[3];
    cam_of(v, cam);
    RpRect a, m;
    a.full = m.full = false;
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!obj_vis(o)) continue;   // SHOW_INFRONT.
        float b[4];
        const int k = rp_obj_ndc(o, v, b);
        if (k == 0) continue;
        if (k == 2) return;   // Both whole.
        rp_rect_add(a, b);
        if (rp_mir_reader(o, cam)) {   // MIR_BAND_RECT: a band-only reader, its band's part alone.
            float mb[4];
            const int mk = rp_mir_flagged(o) ? -1 : rp_mir_band_ndc(o, v, mb);
            if (mk < 0) rp_rect_add(m, b);
            else if (mk == 1) rp_rect_add(m, mb);
        }
    }
    rp_rect_close(a);
    rp_rect_close(m);
    all = a;
    mir = m;
}
// REPLAY_SCISSOR: whether a pass drawing list may read the merged count (vol) and the merged mirror
// (mir): each is this cycle's merge, and every object of the list inside the rectangle the replay drew in for
// it - trivially when the scene is the one the rectangles were made from (its generation unchanged) or a rectangle
// was the whole target; otherwise object by object, through the same bound (a mirror reader inside the mirror's).
inline void rp_covered(const std::vector<RenderObject>& list, bool& vol, bool& mir) {
    vol = rp_merged_now();
    mir = rp_mir_merged_now();
    if (!vol && !mir) return;
    if (g_scene.gen == g_rp.sc_gen || (g_rp.sc_all.full && g_rp.sc_mir.full)) return;
    if (!g_ro.cycle_pv_valid || !(fabsf(g_ro.cycle_pv.projection[2][3]) > 1.0e-6f)) {   // No bound to test against.
        if (!g_rp.sc_all.full) vol = false;
        if (!g_rp.sc_mir.full) mir = false;
    } else {
        float v[4][4];
        rp_view_of(v);   // RP_ADOPT: the rectangles' own view.
        float cam[3];
        cam_of(v, cam);
        auto inside = [](const D3D11_RECT& r, const RpRect& in) {
            if (in.full || rp_px_empty(r)) return true;
            return in.any && r.left >= in.px.left && r.top >= in.px.top &&
                   r.right <= in.px.right && r.bottom <= in.px.bottom;
        };
        for (const RenderObject& o : list) {
            if (!vol && !mir) break;
            if (!obj_vis(o)) continue;   // SHOW_INFRONT.
            float b[4];
            const int k = rp_obj_ndc(o, v, b);
            if (k == 0) continue;
            const D3D11_RECT pr = k == 1 ? rp_px_of(b[0], b[1], b[2], b[3]) : D3D11_RECT{ 0, 0, 1, 1 };
            if (vol && !g_rp.sc_all.full && (k == 2 || !inside(pr, g_rp.sc_all))) vol = false;
            if (mir && !g_rp.sc_mir.full && rp_mir_reader(o, cam)) {   // MIR_BAND_RECT: the same bound.
                float mb[4];
                const int mk = (k == 2 || rp_mir_flagged(o)) ? -1 : rp_mir_band_ndc(o, v, mb);
                const D3D11_RECT pm = mk < 0 ? pr : (mk == 1 ? rp_px_of(mb[0], mb[1], mb[2], mb[3])
                                                              : D3D11_RECT{ 0, 0, 0, 0 });
                if (k == 2 || !inside(pm, g_rp.sc_mir)) mir = false;
            }
        }
    }
    if (rp_merged_now() && !vol) stat_inc(g_rp_rec.c.uncovered);
}
// EA_RP_REC: one record's pipeline state but its rasterizer state, viewports and scissor rects (each loop sets
// those its own way), with its vertex buffers and VS / PS shader resources set up to the loop's RP_BIND_HW counts.
inline void rp_bind_rec(ID3D11DeviceContext* ctx, const RpDraw& r, uint32_t vb_n, uint32_t vs_n,
                        uint32_t ps_n) {
    ctx->IASetInputLayout(r.il);
    ctx->IASetPrimitiveTopology(r.topo);
    if (vb_n) ctx->IASetVertexBuffers(0, vb_n, r.vb, r.vbs, r.vbo);
    ctx->IASetIndexBuffer(r.ib, r.ifmt, r.ioff);
    ctx->VSSetShader(r.vs, nullptr, 0);
    ctx->PSSetShader(r.ps, nullptr, 0);
    ID3D11DeviceContext1* const c1 = r.cb1 ? as_ctx1(ctx) : nullptr;
    if (c1) {
        c1->VSSetConstantBuffers1(0, RP_CB, r.vcb, r.vcf, r.vcn);
        c1->PSSetConstantBuffers1(0, RP_CB, r.pcb, r.pcf, r.pcn);
    } else {
        ctx->VSSetConstantBuffers(0, RP_CB, r.vcb);
        ctx->PSSetConstantBuffers(0, RP_CB, r.pcb);
    }
    if (vs_n) ctx->VSSetShaderResources(0, vs_n, r.vsrv);   // RP_BIND_ALL / RP_BIND_HW.
    ctx->VSSetSamplers(0, RP_SMP, r.vsmp);
    if (ps_n) ctx->PSSetShaderResources(0, ps_n, r.psrv);
    bc_forget();   // BIND_FILTER: t0 - t127.
    ctx->PSSetSamplers(0, RP_SMP, r.psmp);
    ctx->OMSetDepthStencilState(r.dss, r.sref);
    ctx->OMSetBlendState(r.bs, r.bf, r.bmask);
}
// EA_RP_REC: rc cut to the record's own scissor rect when its rasterizer state clipped the draw (both clip
// it); false = nothing of it is left.
inline bool rp_clip_rec(const RpDraw& r, D3D11_RECT& rc) {
    D3D11_RASTERIZER_DESC rd = {};
    if (r.rs) r.rs->GetDesc(&rd);
    if (!r.rs || !rd.ScissorEnable || !r.nsc) return true;
    rc.left = rc.left > r.sc[0].left ? rc.left : r.sc[0].left;
    rc.top = rc.top > r.sc[0].top ? rc.top : r.sc[0].top;
    rc.right = rc.right < r.sc[0].right ? rc.right : r.sc[0].right;
    rc.bottom = rc.bottom < r.sc[0].bottom ? rc.bottom : r.sc[0].bottom;
    return rc.right > rc.left && rc.bottom > rc.top;
}
// EA_RP_REC: the record's draw call, as the engine issued it.
inline void rp_draw_rec(ID3D11DeviceContext* ctx, const RpDraw& r) {
    switch (r.kind) {
        case 1: ctx->DrawIndexed(r.a, r.b, r.d); break;
        case 2: ctx->Draw(r.a, r.b); break;
        case 3: ctx->DrawIndexedInstanced(r.a, r.b, r.c, r.d, r.e); break;
        case 4: ctx->DrawInstanced(r.a, r.b, r.c, r.e); break;
        default: break;
    }
}
// The recorded draws [from, to), re-issued into whatever depth target is bound; with sc, each through
// its rasterizer's scissor twin, clipped to *sc (intersected with the draw's own scissor when it had one).
inline void rp_issue(ID3D11DeviceContext* ctx, uint32_t from, uint32_t to, const D3D11_RECT* sc) {
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    uint32_t vb_hw = RP_VB, vs_hw = RP_SRV, ps_hw = RP_SRV;   // RP_BIND_HW: full first.
    for (uint32_t i = from; i < to && i < g_rp_rec.n; ++i) {
        const RpDraw& r = g_rp_rec.draws[i];
        const uint32_t vb_n = r.vb_n > vb_hw ? r.vb_n : vb_hw;   // RP_BIND_HW.
        const uint32_t vs_n = r.vsrv_n > vs_hw ? r.vsrv_n : vs_hw;
        const uint32_t ps_n = r.psrv_n > ps_hw ? r.psrv_n : ps_hw;
        vb_hw = r.vb_n; vs_hw = r.vsrv_n; ps_hw = r.psrv_n;
        rp_bind_rec(ctx, r, vb_n, vs_n, ps_n);
        if (r.nvp) ctx->RSSetViewports(r.nvp, r.vp);
        ID3D11RasterizerState* const sc_rs = sc ? rp_rs_scissor(ctx, r.rs) : nullptr;
        if (sc_rs) {
            D3D11_RECT rc = *sc;
            if (!rp_clip_rec(r, rc)) continue;   // Nothing of it inside.
            ctx->RSSetState(sc_rs);
            ctx->RSSetScissorRects(1, &rc);
        } else {
            ctx->RSSetState(r.rs);
            if (r.nsc) ctx->RSSetScissorRects(r.nsc, r.sc);
        }
        rp_draw_rec(ctx, r);
    }
}
// MIR_REPLAY: the recorded draws the real mirror re-issued, into dsv the way vmir_begin re-issues them - no
// render target, the whole mirror's viewport, the near-patched view block at b2 (g_vmir.b2_patch, still this pass's:
// it is patched once a pass, at its first counting draw), the depth-clamp twin of the draw's rasterizer state - and
// everything else as recorded. With sc each is clipped to it (and to its own scissor when it had one), as
// rp_issue clips. False = a twin could not be made.
inline bool rp_issue_mir(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, const D3D11_RECT* sc) {
    ctx->GSSetShader(nullptr, nullptr, 0); ctx->HSSetShader(nullptr, nullptr, 0); ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetRenderTargets(0, nullptr, dsv);
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(g_vmir.w);
    vp.Height = static_cast<float>(g_vmir.h);
    vp.MaxDepth = 1.0f;
    uint32_t vb_hw = RP_VB, vs_hw = RP_SRV, ps_hw = RP_SRV;   // RP_BIND_HW: full first.
    for (uint32_t i = 0; i < g_rp_rec.n; ++i) {
        const RpDraw& r = g_rp_rec.draws[i];
        if (!r.mir) continue;
        ID3D11RasterizerState* const rs = rp_rs_mir(ctx, r.rs, sc != nullptr);
        if (!rs) return false;
        D3D11_RECT rc = {};
        if (sc) {
            rc = *sc;
            if (!rp_clip_rec(r, rc)) continue;   // Nothing of it inside.
        }
        const uint32_t vb_n = r.vb_n > vb_hw ? r.vb_n : vb_hw;   // RP_BIND_HW (past the skips:
        const uint32_t vs_n = r.vsrv_n > vs_hw ? r.vsrv_n : vs_hw;   // a skipped record set nothing).
        const uint32_t ps_n = r.psrv_n > ps_hw ? r.psrv_n : ps_hw;
        vb_hw = r.vb_n; vs_hw = r.vsrv_n; ps_hw = r.psrv_n;
        rp_bind_rec(ctx, r, vb_n, vs_n, ps_n);
        ctx->VSSetConstantBuffers(2, 1, &g_vmir.b2_patch);   // Whole, as vmir_begin binds it.
        ctx->RSSetViewports(1, &vp);
        ctx->RSSetState(rs);
        if (sc) ctx->RSSetScissorRects(1, &rc);
        else if (r.nsc) ctx->RSSetScissorRects(r.nsc, r.sc);
        rp_draw_rec(ctx, r);
    }
    return true;
}
// MIR_REPLAY: mirror A, B and the merged mirror, at the volume buffer's size (the mirror's own, vmir_ensure_targets).
inline bool rp_ensure_mir(ID3D11Device* dev) {
    if (!dev || g_rp_rec.w == 0 || g_rp_rec.h == 0) return false;
    if (g_rp.mira_tex && g_rp.mira_dsv && g_rp.mira_srv && g_rp.mirb_tex && g_rp.mirb_dsv && g_rp.mirb_srv &&
        g_rp.mirm_tex && g_rp.mirm_rtv && g_rp.mirm_srv && g_rp.mir_w == g_rp_rec.w &&
            g_rp.mir_h == g_rp_rec.h) return true;
    rp_mir_release();
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_rp_rec.w; td.Height = g_rp_rec.h; td.MipLevels = 1; td.ArraySize = 1; td.SampleDesc.Count = 1;
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
    bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_rp.mira_tex)) && g_rp.mira_tex &&
              SUCCEEDED(dev->CreateDepthStencilView(g_rp.mira_tex, &dd, &g_rp.mira_dsv)) && g_rp.mira_dsv &&
              SUCCEEDED(dev->CreateShaderResourceView(g_rp.mira_tex, &vd, &g_rp.mira_srv)) && g_rp.mira_srv &&
              SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_rp.mirb_tex)) && g_rp.mirb_tex &&
              SUCCEEDED(dev->CreateDepthStencilView(g_rp.mirb_tex, &dd, &g_rp.mirb_dsv)) && g_rp.mirb_dsv &&
              SUCCEEDED(dev->CreateShaderResourceView(g_rp.mirb_tex, &vd, &g_rp.mirb_srv)) && g_rp.mirb_srv;
    if (ok) {
        D3D11_TEXTURE2D_DESC md = td;
        md.Format = DXGI_FORMAT_R8G8_UINT;
        md.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        ok = SUCCEEDED(dev->CreateTexture2D(&md, nullptr, &g_rp.mirm_tex)) && g_rp.mirm_tex &&
             SUCCEEDED(dev->CreateRenderTargetView(g_rp.mirm_tex, nullptr, &g_rp.mirm_rtv)) && g_rp.mirm_rtv &&
             SUCCEEDED(dev->CreateShaderResourceView(g_rp.mirm_tex, nullptr, &g_rp.mirm_srv)) && g_rp.mirm_srv;
    }
    if (!ok) { rp_mir_release(); return false; }
    g_rp.mir_w = g_rp_rec.w;
    g_rp.mir_h = g_rp_rec.h;
    return true;
}
// MIR_REPLAY: the recorded pass drew the real mirror - its prepass ran in this pass's frame on this buffer, and at
// least one of its draws was re-issued there.
inline bool rp_mir_used() {
    if (g_vmir.prepass_stamp != static_cast<uint32_t>(g_rp_rec.pass) || !g_vmir.prepass_src ||
        g_vmir.prepass_src != g_svs.vol_src) return false;
    for (uint32_t i = 0; i < g_rp_rec.n; ++i) if (g_rp_rec.draws[i].mir) return true;
    return false;
}
// MIR_REPLAY: the mirror can be replayed this cycle - it was drawn, what its re-issue bound still stands, and the
// targets exist at its size.
inline bool rp_mir_eligible(ID3D11DeviceContext* ctx) {
    if (!rp_mir_used() || !g_res.ps_rpmergemir || !g_vmir.tex || !g_vmir.srv || !g_vmir.b2_patch) return false;
    if (g_vmir.w != g_rp_rec.w || g_vmir.h != g_rp_rec.h) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = rp_ensure_mir(dev);
    if (dev) dev->Release();
    return ok;
}
// MIR_REPLAY: run by rp_mir_finish, in its own state bracket, for the first pass this cycle that reads the mirror
// (rp_replay's merge armed it). Mirror A: the real mirror's depth (the pass's prepass) with its count cleared;
// mirror B: this cycle's prepass (replay B's world seam drew it); the same re-issued draws into both; then the merge -
// B where A equals the real mirror, else the real mirror. False = the real mirror stands.
inline bool rp_mir_replay(ID3D11DeviceContext* ctx, const D3D11_RECT* sc) {
    if (!g_rp.mirb_drawn || !g_rp.mira_tex || !g_rp.mira_dsv || !g_rp.mirb_dsv || !g_rp.mirm_rtv ||
        !g_res.ps_rpmergemir || !g_vmir.tex) return false;
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->CopyResource(g_rp.mira_tex, g_vmir.tex);
    ctx->ClearDepthStencilView(g_rp.mira_dsv, D3D11_CLEAR_STENCIL, 1.0f, 0);   // The count only: the depth stands.
    if (!rp_issue_mir(ctx, g_rp.mira_dsv, sc)) return false;
    if (!rp_issue_mir(ctx, g_rp.mirb_dsv, sc)) return false;
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* nul[1] = { nullptr };
    ctx->PSSetShaderResources(28, 1, nul);   // The merged mirror may still be bound for reading from the last cycle.
    ctx->OMSetRenderTargets(1, &g_rp.mirm_rtv, nullptr);
    D3D11_VIEWPORT vpc = {};
    vpc.Width = static_cast<float>(g_rp_rec.w);
    vpc.Height = static_cast<float>(g_rp_rec.h);
    vpc.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vpc);
    ID3D11RasterizerState* const mrs = sc ? rp_rs_scissor(ctx, nullptr) : nullptr;
    ctx->RSSetState(mrs);
    if (mrs) ctx->RSSetScissorRects(1, sc);
    ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    ctx->PSSetShader(g_res.ps_rpmergemir, nullptr, 0);
    ID3D11ShaderResourceView* const in_a = g_rp.mira_srv;
    ID3D11ShaderResourceView* const in_e = g_vmir.srv;
    ID3D11ShaderResourceView* const in_b = g_rp.mirb_srv;
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
inline bool rp_b_eligible() {
    if (g_rp_rec.eng_seq != g_rp_rec.pass) return false;          // No engine snapshot of this pass.
    // Not cleared this frame: a footprint could survive in it.
    if (g_rp_rec.vol_clear_seq != g_rp_rec.pass) return false;
    if (!g_svs.vol_foot_ok || !svs_foot_bound()) return false;   // The fallback pixels' witness.
    if (!g_res.ps_rpmerge || !g_rp_rec.dsvb || !g_rp_rec.stenb || !g_rp_rec.mst_rtv || !g_rp_rec.mft_rtv) return false;
    // B's footprint in the copy's raster: the latch's projection where it agrees with the copy's, else the copy's own
    // terms (REPLAY_RASTER, set here for rp_replay's cycle; the same handedness is required, as the lookup's
    // map requires it).
    g_rp.rast_on = false;
    if (g_svs.vol_proj_ok) {
        float now[4];
        if (!g_ro.cycle_pv_valid || !sten_proj_terms(g_ro.cycle_pv.projection, now)) return false;
        bool part = false;
        for (int k = 0; k < 4; ++k) {
            const float tol = 1.0e-4f * (fabsf(now[k]) > 1.0f ? fabsf(now[k]) : 1.0f);
            if (fabsf(g_svs.vol_proj[k] - now[k]) > tol) part = true;
        }
        if (part) {
            if (!(g_svs.vol_proj[0] * now[0] > 0.0f && g_svs.vol_proj[1] * now[1] > 0.0f)) return false;
            memcpy(g_rp.rast, g_svs.vol_proj, sizeof(g_rp.rast));
            g_rp.rast_on = true;
        }
    }
    return true;
}
// Replay B: the engine's depth (already in its target: rp_snap_engine), our world footprint at this cycle's pose
// (the world seam's own pass in B mode, with the volume pass's b2..b4 bound - its depth goes through the engine's view
// block), then the recorded draws with our in-front footprint (the hands seam in B mode, its slice viewport) at the
// place the hands seam came in the pass. The seams' globals are their own frame's again afterwards. False when the
// world footprint did not draw.
inline bool rp_replay_b(ID3D11DeviceContext* ctx, const D3D11_RECT* sc) {
    ctx->OMSetRenderTargets(0, nullptr, g_rp_rec.dsvb);
    {
        const RpDraw& r0 = g_rp_rec.draws[0];
        ID3D11DeviceContext1* const c1 = r0.cb1 ? as_ctx1(ctx) : nullptr;
        if (c1) c1->VSSetConstantBuffers1(2, 3, &r0.vcb[2], &r0.vcf[2], &r0.vcn[2]);
        else ctx->VSSetConstantBuffers(2, 3, &r0.vcb[2]);
    }
    const uint64_t sv_sig = g_svs.skip_sig;
    const bool sv_valid = g_svs.skip_valid, sv_inj = g_svs.injected_frame, sv_prime = g_svs.prime_ready;
    float sv_vp[4][4]; memcpy(sv_vp, g_svs.prime_vp, sizeof(sv_vp));
    float sv_cam[3]; memcpy(sv_cam, g_svs.prime_cam, sizeof(sv_cam));
    const bool sv_rebase = g_svs.prime_rebase;
    const float sv_lo = g_svs.prime_vp_lo, sv_hi = g_svs.prime_vp_hi;
    float sv_enc[4]; memcpy(sv_enc, g_svs.seam_enc, sizeof(sv_enc));
    const uint64_t sv_enc_seq = g_svs.seam_enc_seq;
    float sv_proj[4]; memcpy(sv_proj, g_svs.seam_proj, sizeof(sv_proj));
    const bool sv_proj_ok = g_svs.seam_proj_ok;
    const int sv_fix = g_svs.foot_seam_ix;
    const uint64_t sv_fseq = g_svs.foot_seam_seq;
    auto back = [&]() {   // The seams' globals their own frame's again.
        g_rp.seam_b = false;
        g_svs.skip_sig = sv_sig; g_svs.skip_valid = sv_valid;
        g_svs.injected_frame = sv_inj; g_svs.prime_ready = sv_prime;
        memcpy(g_svs.prime_vp, sv_vp, sizeof(sv_vp)); memcpy(g_svs.prime_cam, sv_cam, sizeof(sv_cam));
        g_svs.prime_rebase = sv_rebase; g_svs.prime_vp_lo = sv_lo; g_svs.prime_vp_hi = sv_hi;
        memcpy(g_svs.seam_enc, sv_enc, sizeof(sv_enc)); g_svs.seam_enc_seq = sv_enc_seq;
        memcpy(g_svs.seam_proj, sv_proj, sizeof(sv_proj)); g_svs.seam_proj_ok = sv_proj_ok;
        g_svs.foot_seam_ix = sv_fix; g_svs.foot_seam_seq = sv_fseq;
    };
    RpOnUnwind uw(back);   // RP_UNWIND: a throw in a seam would leave B mode on for every seam after.
    g_svs.foot_seam_seq = ~0ull;
    g_rp.seam_b = true;
    volume_seam_inject(ctx, g_rp_rec.w, g_rp_rec.h);
    const int b_foot = g_svs.foot_pub == 0 ? 1 : 0;
    const bool drew = g_svs.foot_seam_seq == g_svs.frame_seq && g_svs.foot_seam_ix == b_foot;
    if (drew) {
        const bool front = g_rp_rec.front_seq == g_rp_rec.pass;
        const uint32_t at = front ? (g_rp_rec.front_idx < g_rp_rec.n ? g_rp_rec.front_idx : g_rp_rec.n) : g_rp_rec.n;
        ctx->OMSetRenderTargets(0, nullptr, g_rp_rec.dsvb);
        rp_issue(ctx, 0, at, sc);
        if (front) {   // The in-front footprint where the hands seam came: its slice, into B, at this cycle's pose.
            ctx->OMSetRenderTargets(0, nullptr, g_rp_rec.dsvb);
            ctx->RSSetViewports(1, &g_rp_rec.front_vp);
            infront_seam_inject(ctx, g_rp_rec.front_vp);
        }
        ctx->OMSetRenderTargets(0, nullptr, g_rp_rec.dsvb);
        rp_issue(ctx, at, g_rp_rec.n, sc);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
    }
    back();
    return drew;
}
// At the injection: replay the last recorded pass and merge. A withheld pass is B everywhere (its copy counted against
// whatever is behind our meshes); one that was not keeps B only where replay A equals the engine's count.
inline void rp_replay(ID3D11DeviceContext* ctx) {
    // REPLAY_ONCE: an injection re-armed within the cycle (the camera partition's wait, the anomalous and
    // rescue windows) calls again after the pass was replayed; that replay's verdicts - the merge, the mirror
    // armed for the first pass that reads it, the raster - stand for the rest of the cycle.
    if (g_rp_rec.replayed && g_rp.replay_cyc == g_topo_cycles) return;
    g_rp.merge_ok = false;   // This cycle is merged only if the replay below completes.
    g_rp.mir_ok = false;     // MIR_REPLAY: likewise the mirror.
    g_rp.rast_on = false;    // REPLAY_RASTER: this cycle's replay sets it (rp_b_eligible).
    if (g_rp.mir_pend) { g_rp.mir_pend = false; stat_inc(g_rp_rec.c.mir_idle); }   // Last cycle's: no pass read it.
    if (g_rp_rec.pass == ~0ull || g_rp_rec.replayed || !g_rp_rec.ended) return;
    g_rp_rec.replayed = true;
    g_rp.replay_cyc = g_topo_cycles;   // REPLAY_ONCE.
    if (!g_rp_rec.ok || g_rp_rec.n == 0 || g_svs.vol_seq != g_rp_rec.pass || !g_svs.vol_sten_srv || !g_svs.vol_primed ||
        !g_res.vs_fullscreen || !g_res.dss_off || !g_res.composite_cb || !rp_b_eligible()) {
        stat_inc(g_rp_rec.c.fallback);
        return;
    }
    const bool withheld = g_rp.withheld_seq == g_rp_rec.pass;
    {
        // RP_PRESYNC: replay B's world seam's step and sync (volume_seam_inject's, under the same flag), taken
        // ahead of the rectangles, so they and sc_gen are of the scene B draws and the pass after this draws (its list
        // taken again from it, OBJBUF_LATE): covered by the generation alone. A step after the cycle's final one
        // reuses its pick (gts_for_cycle), so B's own step and sync take only what changed since (a game-thread
        // stage). A return below that B does not follow leaves the pass this scene, as B would. Outside sv's capture,
        // as the injection's own step: a step binds only inside its own StateBackup (skin_so_batch, uvs_step).
        // Cost, per cycle that reaches it: one more attach_step (its g_draw_list_mutex walk of every binding and
        // its per-chain end pass), uvs_step and scene_sync on the CPU; GPU work only for what changed since the
        // injection's own step.
        InjectionFlagScope flag;
        g_ro.in_injection = true;
        attach_step(ctx);
        uvs_step(ctx);
    }
    scene_sync();
    RpRect sc_all, sc_mir;   // REPLAY_SCISSOR.
    rp_scissor_rects(sc_all, sc_mir);
    const uint64_t sc_gen = g_scene.gen;   // What they were made from (RP_PRESYNC).
    if (!sc_all.full && !sc_all.any) return;   // No mesh of ours on screen.
    stat_inc(sc_all.full ? g_rp_rec.c.fullscreen : g_rp_rec.c.scissored);
    const D3D11_RECT* const scp = sc_all.full ? nullptr : &sc_all.px;
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    // RP_UNWIND: declared ahead of sv, so a throw restores the state (sv's destructor, our hooks still aside) first.
    RpOnUnwind uw([&]() { g_rp.mir_want = false; g_ro.in_injection = prev; });
    if (!rp_img_upload(ctx)) {   // REPLAY_CPUCB: the pages the records bind, before any of them replays.
        g_ro.in_injection = prev;
        stat_inc(g_rp_rec.c.fallback);
        return;
    }
    RpSave sv;
    sv.capture(ctx);
    bool ok = true;
    if (!withheld) {   // Replay A (the trust): the pass as the engine drew it, from its first draw.
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
        ctx->CopyResource(g_rp_rec.tgt, g_rp_rec.pre);
        ctx->OMSetRenderTargets(0, nullptr, g_rp_rec.dsv);
        rp_issue(ctx, 0, g_rp_rec.n, scp);
        ctx->OMSetRenderTargets(0, nullptr, nullptr);
    }
    // MIR_REPLAY: when the pass drew the real mirror and a mesh that can read it is on screen, replay B's world
    // seam draws mirror B's prepass too; the rest is replayed, in that mesh set's rectangle (g_rp.sc_mir), for the
    // first pass that reads the mirror (rp_mir_finish).
    const bool mir_used = rp_mir_used();
    const bool mir_read = sc_mir.full || sc_mir.any;
    const bool mir = mir_used && mir_read && rp_mir_eligible(ctx);
    g_rp.mir_want = mir;
    g_rp.mirb_drawn = false;
    ok = rp_replay_b(ctx, scp);
    g_rp.mir_want = false;
    if (ok) {   // The merge: the count and its footprint distance, B where trusted (everywhere when withheld).
        ID3D11ShaderResourceView* nul[1] = { nullptr };
        ctx->PSSetShaderResources(24, 1, nul);   // Neither merged texture may be bound for reading while written.
        ctx->PSSetShaderResources(33, 1, nul);
        ID3D11RenderTargetView* mrt[2] = { g_rp_rec.mst_rtv, g_rp_rec.mft_rtv };
        ctx->OMSetRenderTargets(2, mrt, nullptr);
        D3D11_VIEWPORT vpc = {};
        vpc.Width = static_cast<float>(g_rp_rec.w);
        vpc.Height = static_cast<float>(g_rp_rec.h);
        vpc.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vpc);
        ID3D11RasterizerState* const mrs = scp ? rp_rs_scissor(ctx, nullptr) : nullptr;
        ctx->RSSetState(mrs);
        if (mrs) ctx->RSSetScissorRects(1, scp);
        ctx->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->IASetInputLayout(nullptr);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
        ctx->PSSetShader(g_res.ps_rpmerge, nullptr, 0);
        ID3D11ShaderResourceView* in2[2] = { withheld ? nullptr : g_rp_rec.sten, g_svs.vol_sten_srv };
        ctx->PSSetShaderResources(34, 1, &in2[0]);
        ctx->PSSetShaderResources(41, 1, &in2[1]);
        ID3D11ShaderResourceView* in3[3] = { g_rp_rec.stenb, g_svs.foot_srv[g_svs.foot_pub == 0 ? 1 : 0],
            svs_foot_bound() };
        ctx->PSSetShaderResources(49, 3, in3);
        ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };   // Bound before the upload (ring).
        ctx->VSSetConstantBuffers(0, 2, cbs);
        ctx->PSSetConstantBuffers(0, 2, cbs);
        ConstantData cbd = {};
        cbd.color[1] = withheld ? 1.0f : 0.0f;
        ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
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
        g_rp.merge_ok = true;
        g_rp.merge_cyc = g_topo_cycles;
        g_rp.sc_all = sc_all;   // REPLAY_SCISSOR: what the passes after it test themselves against.
        g_rp.sc_mir = sc_mir;
        g_rp.sc_gen = sc_gen;
        stat_inc(g_rp_rec.c.merged);
        if (mir_used) {   // MIR_REPLAY: armed for the first pass that reads the mirror (rp_mir_finish).
            if (!mir_read) {
                stat_inc(g_rp_rec.c.mir_idle);
            } else if (mir && g_rp.mirb_drawn) {
                g_rp.mir_pend = true;
                g_rp.mir_pend_cyc = g_topo_cycles;
                g_rp.mir_pend_pass = g_rp_rec.pass;
            } else {
                stat_inc(g_rp_rec.c.mir_fallback);
            }
        }
    } else {
        stat_inc(g_rp_rec.c.fallback);
    }
}
// MIR_REPLAY: the armed mirror replay, run once, by the first pass this cycle with a mesh that reads the mirror -
// in its own state bracket. Everything it replays must still be the armed pass's: the same cycle, the same recorded
// pass (ended; a new pass releases its records), the real mirror not prepassed again, the volume replay merged.
// Otherwise the real mirror stands (a mirror fallback).
inline void rp_mir_finish(ID3D11DeviceContext* ctx) {
    if (!g_rp.mir_pend || !ctx) return;
    g_rp.mir_pend = false;
    const bool live = g_rp.mir_pend_cyc == g_topo_cycles && g_rp.mir_pend_pass == g_rp_rec.pass && g_rp_rec.ended &&
                      g_vmir.prepass_stamp == static_cast<uint32_t>(g_rp_rec.pass) && rp_merged_now();
    bool done = false;
    if (live) {
        const bool prev = g_ro.in_injection;
        g_ro.in_injection = true;
        RpOnUnwind uw([&]() { g_ro.in_injection = prev; });   // RP_UNWIND: after sv's restore.
        RpSave sv;
        sv.capture(ctx);
        done = rp_mir_replay(ctx, g_rp.sc_mir.full ? nullptr : &g_rp.sc_mir.px);
        sv.restore(ctx);
        sv.release();
        g_ro.in_injection = prev;
    }
    g_rp.mir_ok = done;
    g_rp.mir_cyc = g_topo_cycles;
    stat_inc(done ? g_rp_rec.c.mir_merged : g_rp_rec.c.mir_fallback);
}
// VOL_REPLAY: the engine's own call - the target context, on the render thread, not one of ours. The gate of the
// hooks that refuse a pass the recorder cannot follow (rp_foreign: a command list, DrawAuto or an indirect draw, a
// copy into the volume buffer); CopyResource's is a core hook, the others are installed one by one after the core hooks
// (rp_extra_hooks; a failure leaves that one off).
inline bool rp_engine_call(ID3D11DeviceContext* self) {
    return self == g_reorder_target_ctx.load(std::memory_order_relaxed) && reorder_on_render_thread() && !g_ro.in_injection;
}
}