#pragma once

// The injection half (rendering_integration_injection.hpp): the dynamic-light harvest, part 1: the CB shadow, the want
// gate, the presence memo and the sampled draws. Never include it on its own.
namespace RenderIntegration {

// Defined with the window table (dl_win_slot's group, rendering/dynamic_lights_pool.hpp); the presence pass reads
// the origin a buffer last resolved to (at any harvest: DL_PRES_FRESH tells which).
inline bool dl_win_origin_of(const void* buf, float& ox, float& oz);
inline bool dl_pres_fresh(const void* buf, float ox, float oz, uint64_t seq);   // DL_PRES_CUR.

// Defined in rendering/upload_funnel.hpp (declared here for the uses below).
inline bool wc_copy(void* dst, const void* src, uint32_t n);

inline float dl_grid_base() { return g_dlcap.cell.load(std::memory_order_relaxed); }
inline float dl_grid() {
    const float c = dl_grid_base();
    return (c > 0.0f && g_dlcap.q_cells > 0u) ? c * static_cast<float>(g_dlcap.q_cells) : 0.0f;
}
inline uint32_t dl_gcd(uint32_t a, uint32_t b) {
    while (b != 0u) {
        const uint32_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}
// A strong route's committed origin: its cell counts folded into the learned multiple (gcd(0, n) = n, so the first
// origin seeds it and every later one can only refine it).
inline void dl_grid_learn(float ox, float oz) {
    const float c = dl_grid_base();
    if (!(c > 0.0f)) return;
    const float fx = fabsf(ox) / c;
    const float fz = fabsf(oz) / c;
    if (!(fx < 1.0e6f && fz < 1.0e6f)) return;   // NaN fails too.
    uint32_t g = g_dlcap.q_cells;
    g = dl_gcd(g, static_cast<uint32_t>(floorf(fx + 0.5f)));
    g = dl_gcd(g, static_cast<uint32_t>(floorf(fz + 0.5f)));
    if (g > 0u) g_dlcap.q_cells = g;
}

inline uint64_t dl_gap_mult() {
    return DL_GAP_MULT;
}

inline bool dl_dense(uint16_t sightings) {
    return sightings >= DL_DENSE_SIGHTS;
}

// Cadence mark update: max(this gap, the decayed previous mark), so the TTL
// never falls below GAP_MULT re-sight intervals.
inline uint16_t dl_gap_mark(uint16_t prev, uint64_t gap) {
    if (gap >= 60000) return prev;   // out-of-era stamp: no evidence.
    const uint16_t now = static_cast<uint16_t>(gap);

    const uint16_t dec = static_cast<uint16_t>(
        (static_cast<uint32_t>(prev) * DL_GAP_DECAY_NUM) / DL_GAP_DECAY_DEN);
    return now > dec ? now : dec;
}

inline bool dl_cpu_wanted() {   // A lit mesh or a caster (g_ls.wanted), or a dynamicLightFog pass (DLF_DEMAND).
    return g_ls.wanted.load(std::memory_order_relaxed) || g_dlf_wanted.load(std::memory_order_relaxed);
}
inline uint32_t dl_want_hash(const void* id) {
    uint64_t h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(id));
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    return static_cast<uint32_t>(h) & (DL_WANT_IX - 1u);
}
// The entry holding id, or DL_WANT_N; probes (when given) takes the index slots looked at.
inline uint32_t dl_want_find(const void* id, uint32_t* probes = nullptr) {
    uint32_t h = dl_want_hash(id), n = 0u;
    for (uint32_t k = 0; k < DL_WANT_IX; ++k) {
        const uint32_t e = g_dl_keep_ix[h];
        ++n;
        if (e == 0u) break;
        const uint32_t i = e - 1u;
        if (i < g_dlcap.keep_n && g_dlcap.keep_id[i] == id) {
            if (probes) *probes = n;
            return i;
        }
        h = (h + 1u) & (DL_WANT_IX - 1u);
    }
    if (probes) *probes = n;
    return DL_WANT_N;
}
inline void dl_want_ix_put(uint32_t i) {
    uint32_t h = dl_want_hash(g_dlcap.keep_id[i]);
    while (g_dl_keep_ix[h] != 0u) h = (h + 1u) & (DL_WANT_IX - 1u);
    g_dl_keep_ix[h] = static_cast<uint8_t>(i + 1u);
}
inline void dl_want_note(const void* id, uint8_t kind) {
    if (!id) return;
    // DL_WANT_GATE: the persistent table (refresh, add, or replace the stalest).
    uint32_t at = dl_want_find(id);   // DL_WANT_INDEX.
    if (at == DL_WANT_N) {
        if (g_dlcap.keep_n < DL_WANT_N) {
            at = g_dlcap.keep_n++;
            g_dlcap.keep_id[at] = id;
            dl_want_ix_put(at);
        } else {
            uint32_t st = 0u;   // The stalest entry, the lowest slot on a tie.
            for (uint32_t i = 1; i < g_dlcap.keep_n; ++i) {
                if (g_dlcap.keep_cycle[i] < g_dlcap.keep_cycle[st]) st = i;
            }
            at = st;
            g_dlcap.keep_id[at] = id;
            memset(g_dl_keep_ix, 0, sizeof(g_dl_keep_ix));   // DL_WANT_INDEX: rebuilt whole.
            for (uint32_t i = 0; i < g_dlcap.keep_n; ++i) dl_want_ix_put(i);
        }
        g_dlcap.keep_kind[at] = 0u;   // PROF_FUNNEL: a new identity's roles start over.
    }
    g_dlcap.keep_cycle[at] = g_topo_cycles;
    if (g_prof.ds_on) g_dlcap.keep_kind[at] |= kind;   // PROF_FUNNEL.
}
// PROF_FUNNEL: kind / probes (when given) take the entry's roles and the index slots looked at.
inline bool dl_want_gate(const void* id, uint8_t* kind = nullptr, uint32_t* probes = nullptr) {
    const uint32_t i = dl_want_find(id, probes);   // DL_WANT_INDEX.
    if (i == DL_WANT_N) return false;
    if (kind) *kind = g_dlcap.keep_kind[i];
    return g_dlcap.keep_cycle[i] <= g_topo_cycles && g_topo_cycles - g_dlcap.keep_cycle[i] <= DL_WANT_KEEP;
}
inline bool dl_cpu_capture_now() {
    if (!dl_cpu_wanted()) return false;
    if (g_dlcap.idle.load(std::memory_order_relaxed) < DL_IDLE_FRAMES) return true;
    return (g_topo_cycles % DL_IDLE_EVERY) == 0u;
}
inline uint32_t cbs_hash(const void* id) {
    uint64_t h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(id));
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    return static_cast<uint32_t>(h) & (CBS_N - 1);
}
// Read side (the sampler, render thread). Entries are never cleared singly
// (only whole-table at reset), so an empty slot ends the probe.
inline CbShadow* cbs_find(const void* id) {
    if (!id) return nullptr;
    uint32_t i = cbs_hash(id);
    for (uint32_t k = 0; k < CBS_PROBE; ++k) {
        CbShadow& e = g_cbs[i];
        if (e.id == id) return &e;
        if (!e.id) return nullptr;
        i = (i + 1) & (CBS_N - 1);
    }
    return nullptr;
}
// Write side (the hooks, render thread): copy the upload into the buffer's
// shadow and return it, so the scanners walk the shadow.
inline const void* cbs_write(void* id, const void* src, uint32_t& bytes) {
    if (!id || !src || bytes == 0) return nullptr;
    if (bytes > UPLOAD_SCRATCH_BYTES) bytes = UPLOAD_SCRATCH_BYTES;
    uint32_t i = cbs_hash(id);
    CbShadow* slot = nullptr;
    CbShadow* lru = nullptr;
    for (uint32_t k = 0; k < CBS_PROBE; ++k) {
        CbShadow& e = g_cbs[i];
        if (e.id == id) { slot = &e; break; }
        if (!e.id) { slot = &e; break; }
        // CBS_LRU_WSEQ: the 64-bit capture clock - gen wraps at 32 bits (hours at thousands of captures a
        // frame), and past a wrap '<' on gen would take the freshest entries for the least recently written.
        if (!lru || e.wseq < lru->wseq) lru = &e;
        i = (i + 1) & (CBS_N - 1);
    }
    if (!slot) {
        if (!lru) return nullptr;
        slot = lru;   // The run is full: the least recently written identity is recycled.
    }
    if (slot->data.size() < bytes) slot->data.resize(bytes);
    wc_copy(slot->data.data(), src, bytes);   // WC_COPY: the mapped source.
    if (slot->id != id) slot->pres_gen = 0;   // A recycled entry forgets its key.
    slot->id = id;
    slot->bytes = bytes;
    if (++g_dlcap.cbs_gen == 0) ++g_dlcap.cbs_gen;
    slot->gen = g_dlcap.cbs_gen;
    slot->wseq = ++g_dlcap.cbs_wseq;   // DL_REC_ORDER.
    return slot->data.data();
}
// DL_WANT_STALE: the identity's shadow emptied in place - every reader's size test then misses, and the
// sampler's 'same upload again' test with it (pres_gen 0 matches no written generation).
inline void cbs_stale(const void* id) {
    CbShadow* const s = cbs_find(id);
    if (s) {
        s->bytes = 0;
        s->pres_gen = 0;
    }
}
inline void cbs_clear() {
    for (uint32_t i = 0; i < CBS_N; ++i) {
        g_cbs[i].id = nullptr;
        g_cbs[i].bytes = 0;
        g_cbs[i].gen = 0;
        g_cbs[i].wseq = 0;   // DL_REC_ORDER.
        g_cbs[i].pres_gen = 0;
        g_cbs[i].pres_first = 0;
        g_cbs[i].pres_seq = 0;
        std::vector<uint8_t>().swap(g_cbs[i].data);
    }
}

inline bool dl_finite(float v) { return v == v && v < 1.0e30f && v > -1.0e30f; }
// DL_PRES_CUR: is the cycle's view the clear's own bridge fetch - not held (cycle_pv_stale) and not a retry's
// mid-frame sample, which runs a step ahead of the frame (SUN_CAM_LATCH)?
inline bool dl_view_live() {
    return g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && g_boundary_pv_valid;
}

// The unplaceable-record table, keyed on the raw list-frame position at metre resolution (the same light lands on the
// same key while the origin stands; a flip re-keys it and the note lapses).
inline uint32_t dl_unpl_slot(const int32_t q[3], bool insert, uint64_t seq) {
    uint32_t h = static_cast<uint32_t>(q[0]) * 73856093u ^
                 static_cast<uint32_t>(q[1]) * 19349663u ^
                 static_cast<uint32_t>(q[2]) * 83492791u;
    h &= DL_UNPL_N - 1;
    uint32_t oldest = h;

    for (uint32_t k = 0; k < 8; ++k) {
        const uint32_t i = (h + k) & (DL_UNPL_N - 1);
        if (g_dl.unpl_seq[i] != 0 &&
            g_dl.unpl_q[i][0] == q[0] && g_dl.unpl_q[i][1] == q[1] && g_dl.unpl_q[i][2] == q[2]) {
            if (insert) g_dl.unpl_seq[i] = seq;
            return i;
        }
        if (g_dl.unpl_seq[i] == 0) {
            if (!insert) return DL_UNPL_N;
            g_dl.unpl_q[i][0] = q[0]; g_dl.unpl_q[i][1] = q[1]; g_dl.unpl_q[i][2] = q[2];
            g_dl.unpl_seq[i] = seq;
            return i;
        }
        if (g_dl.unpl_seq[i] < g_dl.unpl_seq[oldest]) oldest = i;
    }

    if (!insert) return DL_UNPL_N;
    g_dl.unpl_q[oldest][0] = q[0]; g_dl.unpl_q[oldest][1] = q[1]; g_dl.unpl_q[oldest][2] = q[2];
    g_dl.unpl_seq[oldest] = seq;
    return oldest;
}
inline void dl_unpl_key(const float* r, int32_t q[3]) {
    q[0] = static_cast<int32_t>(floorf(r[0]));
    q[1] = static_cast<int32_t>(floorf(r[1]));
    q[2] = static_cast<int32_t>(floorf(r[2]));
}
// Is this record one the harvest recently could not place?
inline bool dl_unpl_recent(const float* r, uint64_t now_seq) {
    int32_t q[3];
    dl_unpl_key(r, q);
    const uint32_t i = dl_unpl_slot(q, false, 0);
    if (i >= DL_UNPL_N) return false;
    return now_seq <= g_dl.unpl_seq[i] + DL_UNPL_KEEP;
}
// Note every finite record of an unplaced window.
inline void dl_unpl_note(const float* l, uint32_t n, uint64_t seq) {
    for (uint32_t i = 0; i < n; ++i) {
        const float* r = l + i * 24;
        if (!dl_finite(r[0]) || !dl_finite(r[1]) || !dl_finite(r[2])) continue;
        int32_t q[3];
        dl_unpl_key(r, q);
        dl_unpl_slot(q, true, seq);
    }
}

// DL_PRES_REC: the merge's main-pass test (TWIN of the main block in dynlights_merge_windows, behind its ctl[8]
// guard). Only a main-pass list may rewrite a record; an aux pass keeps a light alive without rewriting it.
inline bool dl_ctl_main(const float* c) {
    if (!dl_finite(c[8]) || c[8] <= 1.0e-6f || c[8] >= 1.0e6f) return false;
    if (!g_dl.main_ref_valid) return true;
    for (int k = 0; k < 3; ++k) {
        const float ref = g_dl.main_gdiff[k];
        const float tol = 0.5f * (fabsf(ref) > 0.02f ? fabsf(ref) : 0.02f);
        if (fabsf(c[12 + k] - ref) > tol) return false;
    }
    return !(fabsf(c[8] - g_dl.main_scale) > 0.25f * fabsf(g_dl.main_scale));
}
inline uint64_t xxh_rotl(uint64_t x, int r) { return (x << r) | (x >> (64 - r)); }
inline uint64_t xxh_round(uint64_t a, uint64_t w) {
    return xxh_rotl(a + w * XXH_P2, 31) * XXH_P1;
}
inline void xxh_stripe(uint64_t (&v)[4], const uint8_t* p) {
    for (int k = 0; k < 4; ++k) {
        uint64_t w;
        memcpy(&w, p + k * 8, 8);
        v[k] = xxh_round(v[k], w);
    }
}
// c: the ctl (DL_CTL_BYTES); l / n: the records hashed (any length).
inline uint64_t dl_chash(const uint8_t* c, const uint8_t* l, uint32_t n) {
    static_assert(DL_CTL_BYTES == 64u, "DL_CHASH: the ctl is two whole stripes");
    // XXH64's lane seeds at seed 0: P1 + P2 and -P1 (mod 2^64), written out (no folded wrap for MSVC's C4307).
    uint64_t v[4] = { 0x60EA27EEADC0B5D6ull, XXH_P2, 0ull, 0x61C8864E7A143579ull };
    xxh_stripe(v, c);
    xxh_stripe(v, c + 32);
    uint32_t i = 0;
    for (; n - i >= 32u; i += 32u) xxh_stripe(v, l + i);
    uint64_t h = xxh_rotl(v[0], 1) + xxh_rotl(v[1], 7) + xxh_rotl(v[2], 12) +
                 xxh_rotl(v[3], 18);
    for (int k = 0; k < 4; ++k) {
        h = (h ^ xxh_round(0ull, v[k])) * XXH_P1 + XXH_P4;
    }
    h += static_cast<uint64_t>(DL_CTL_BYTES) + n;
    for (; n - i >= 8u; i += 8u) {
        uint64_t w;
        memcpy(&w, l + i, 8);
        h = xxh_rotl(h ^ xxh_round(0ull, w), 27) * XXH_P1 + XXH_P4;
    }
    if (n - i >= 4u) {
        uint32_t w4;
        memcpy(&w4, l + i, 4);
        h = xxh_rotl(h ^ (static_cast<uint64_t>(w4) * XXH_P1), 23) * XXH_P2 + XXH_P3;
        i += 4u;
    }
    for (; i < n; ++i) {
        h = xxh_rotl(h ^ (static_cast<uint64_t>(l[i]) * XXH_P5), 11) * XXH_P1;
    }
    h ^= h >> 33;
    h *= XXH_P2;
    h ^= h >> 29;
    h *= XXH_P3;
    h ^= h >> 32;
    return h;
}
inline void dl_pmemo_clear() {
    for (DlPresMemo& e : g_dlcap.pmemo) e.epoch = 0;
    ++g_dlcap.pres_epoch;
}
inline uint32_t dl_fbits(float f) {
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}
inline uint16_t dl_gap_mark_at(uint16_t prev, uint64_t gap, uint64_t stamp, uint64_t now) {
    if (g_dlcap.stall_b != 0 && stamp <= g_dlcap.stall_a && now >= g_dlcap.stall_b) return prev;
    if (g_dl.pool_stamp != 0 && stamp <= g_dl.pool_stamp && now > g_dl.pool_stamp &&
        now - g_dl.pool_stamp > DL_POOL_STALE_MS) return prev;
    return dl_gap_mark(prev, gap);
}
// A sighting, priced as the aux re-sight prices one - presence's, a run's and a replay's (one body).
inline void dl_pres_sight(DlPoolLight& pl, uint64_t now, uint64_t seq) {
    if (pl.stamp != 0) {
        const uint64_t gap = now >= pl.stamp ? now - pl.stamp : 0;
        pl.gap_max_ms = dl_gap_mark_at(pl.gap_max_ms, gap, pl.stamp, now);
        if (pl.sightings < 0xFFFF) pl.sightings++;
    }
    pl.stamp = now;
    if (seq > pl.seq) pl.seq = seq;
}
// The index of the lowest set bit of a non-zero word (de Bruijn: no intrinsic, no unary minus on unsigned).
inline uint32_t dl_bit_low(uint64_t w) {
    static constexpr uint8_t tab[64] = {
         0,  1, 48,  2, 57, 49, 28,  3, 61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19,  9, 13,  8,  7,  6,
    };
    return tab[((w & (~w + 1u)) * 0x03F79D71B4CB0A89ull) >> 58];
}
inline uint64_t dl_phix_cost(uint32_t n) {
    uint64_t l = 1u;
    while ((1ull << l) < n) ++l;
    return static_cast<uint64_t>(n) * (l + 2u) * DL_HIX_STEP_PAIRS;
}
// The index rebuilt from the pool (DL_HIX_OOM: invalid until it completes; an allocation may throw).
inline void dl_phix_build() {
    DlPresHix& x = g_dlcap.phix;
    x.valid = false;
    const uint32_t n = g_dl.pool_n;
    if (g_prof.ds_on) fun_add(FUNNEL_DL_HIX_BUILDS, 1u);   // PROF_FUNNEL.
    x.hb.resize(n);
    x.hs.clear();
    x.hs.reserve(n);     // At most one entry per light: dl_phix_move's insert never allocates.
    x.cand.reserve(n);   // Nor does dl_phix_cands' read-back.
    for (uint32_t d = 0; d < n; ++d) {
        const float h = g_dl.pool[d].rec[1];
        x.hb[d] = dl_fbits(h);
        if (h == h) x.hs.emplace_back(h, d);   // NaN fails the band: never a candidate.
    }
    std::sort(x.hs.begin(), x.hs.end());   // Height, then index (no NaN: a strict weak order).
    x.mark.assign((static_cast<size_t>(n) + 63u) / 64u, 0ull);
    x.n = n;
    x.epoch = g_dlcap.pres_epoch;
    x.debt = 0u;
    x.valid = true;
}
// A run's start (render thread): may the index serve this run? In step - its epoch, or a compare of every height's
// bits (which clears the debt) - it may. Heights out of step mark it invalid; a pool of another count leaves it
// unused (valid: the count may come back with the same heights, and the compare then finds it in step). Either way it
// is rebuilt (and may serve) only once the debt is paid (DL_HIX_DEBT).
inline bool dl_phix_ready() {
    DlPresHix& x = g_dlcap.phix;
    const uint32_t n = g_dl.pool_n;
    if (n < DL_HIX_MIN_POOL) return false;
    if (x.valid && x.n == n) {
        if (x.epoch == g_dlcap.pres_epoch) return true;
        bool same = x.hb.size() == n;
        for (uint32_t d = 0; same && d < n; ++d) same = x.hb[d] == dl_fbits(g_dl.pool[d].rec[1]);
        if (same) { x.epoch = g_dlcap.pres_epoch; x.debt = 0u; return true; }
        x.valid = false;   // Stale: walks repay a rebuild first.
    }
    if (x.debt < DL_HIX_PAYBACK * dl_phix_cost(n)) return false;
    dl_phix_build();
    return x.valid;
}
// A walk run while the index could not serve (dl_phix_ready false on a pool it would cover): its pairs repay -
// records: the records that walked (a record with a non-finite position walks nothing).
inline void dl_phix_paid(uint32_t records) {
    if (g_dl.pool_n < DL_HIX_MIN_POOL) return;
    g_dlcap.phix.debt += static_cast<uint64_t>(records) * g_dl.pool_n;
}
// A presence rewrite moved pool light d's height from was to now: its entry moves with it, keeping a
// valid index in step (an index not in step for this light is left valid for the next run's compare, which finds
// it stale; an invalid one waits for its rebuild). Allocates nothing (the build reserved one entry per light), so it
// cannot throw between the rewrite and the epoch's bump.
inline void dl_phix_move(uint32_t d, float was, float now) {
    DlPresHix& x = g_dlcap.phix;
    if (!x.valid || d >= x.n || x.hb.size() != x.n) return;
    if (x.hb[d] != dl_fbits(was) || dl_fbits(was) == dl_fbits(now)) return;
    x.valid = false;   // DL_HIX_OOM: valid again only once the move completes.
    if (was == was) {
        const std::pair<float, uint32_t> o(was, d);
        const auto it = std::lower_bound(x.hs.begin(), x.hs.end(), o);
        if (it == x.hs.end() || it->second != d || !(it->first == was)) return;   // Left stale.
        x.hs.erase(it);
    }
    if (now == now) {
        const std::pair<float, uint32_t> e(now, d);
        x.hs.insert(std::lower_bound(x.hs.begin(), x.hs.end(), e), e);
    }
    x.hb[d] = dl_fbits(now);
    if (g_prof.ds_on) fun_add(FUNNEL_DL_HIX_MOVES, 1u);   // PROF_FUNNEL.
    x.valid = true;
}
// A record's candidates at height y (finite): pool indices, ascending, in g_dlcap.phix.cand. False: more than a
// sixth of the pool - walk all of it.
inline bool dl_phix_cands(float y) {
    DlPresHix& x = g_dlcap.phix;
    x.cand.clear();
    const double lo = static_cast<double>(y) - 1.0;
    const double hi = static_cast<double>(y) + 1.0;
    const auto b = std::lower_bound(x.hs.begin(), x.hs.end(), lo,
                                    [](const std::pair<float, uint32_t>& e, double v) {
                                            return static_cast<double>(e.first) < v;
                                    });
    // More than a sixth (k * 6 > n, k > floor(n / 6)): the (floor(n / 6) + 1)-th candidate exists and is in the band.
    const std::ptrdiff_t q = static_cast<std::ptrdiff_t>(x.n / 6u);
    if (x.hs.end() - b > q && static_cast<double>(b[q].first) <= hi) {
        return false;
    }
    auto z = b;
    while (z != x.hs.end() && static_cast<double>(z->first) <= hi) ++z;   // At most n / 6.
    const size_t k = static_cast<size_t>(z - b);
    if (k == 1u) x.cand.push_back(b->second);
    if (k < 2u) return true;
    uint32_t wl = 0xFFFFFFFFu, wh = 0u;
    for (auto it = b; it != z; ++it) {
        const uint32_t w = it->second >> 6;
        x.mark[w] |= 1ull << (it->second & 63u);
        if (w < wl) wl = w;
        if (w > wh) wh = w;
    }
    for (uint32_t w = wl; w <= wh; ++w) {
        uint64_t m = x.mark[w];
        x.mark[w] = 0ull;
        while (m) {
            x.cand.push_back(w * 64u + dl_bit_low(m));
            m &= m - 1u;
        }
    }
    return true;
}

// One upload's records against the pool. Primary: the origin this buffer identity last resolved to (any harvest's)
// puts the record in world; the re-sight's DL_MATCH_M cube names the pool light. No origin yet, or a miss: the
// origin-free grid test (height within 0.25, x / z a multiple of the learned grid, dl_grid, apart within the
// vote's residual; off until a strong route has taught the grid). A record claims the ONE nearest candidate by
// position plus every non-position lane, never a light an earlier record of this list claimed. Returns the count of
// finite records that matched nothing and are not recently-unplaceable - the lights to author. DL_PRES_REC: a
// main-pass upload's cube re-sight also rewrites the record, as the merge does - DL_REC_ORDER: unless the record
// holds a newer capture's content (wseq is this upload's capture order, the shadow's wseq).
inline uint32_t dynlights_presence_upload(const void* buf,
                                          const uint8_t* ctl, const uint8_t* l,
                                          uint32_t bytes11, uint64_t seq,
                                          uint64_t now,
                                          uint32_t& unpl, uint32_t& hit_n,   // DL_UNPL: unplaced / matched.
                                          uint64_t wseq,     // DL_REC_ORDER.
                                          uint64_t chash) {  // DL_PRES_MEMO: the content's (DL_CHASH).
    int32_t pc_i = 0, sc_i = 0;
    memcpy(&pc_i, ctl + 0, 4);
    memcpy(&sc_i, ctl + 16, 4);
    if (pc_i < 0 || pc_i > 1024 || sc_i < 0 || sc_i > 1024) return 0;
    uint32_t total = static_cast<uint32_t>(pc_i) + static_cast<uint32_t>(sc_i);
    if (total > DL_MAX_LIGHTS) total = DL_MAX_LIGHTS;
    const uint32_t avail = bytes11 / DL_LIGHT_BYTES;
    if (total > avail) total = avail;
    stat_inc(g_stats.dl_pres_calls);   // DL_PRES_MEMO.
    float ox = 0.0f, oz = 0.0f;
    const bool origin = dl_win_origin_of(buf, ox, oz);
    uint32_t unknown = 0, hits = 0;
    unpl = 0;
    hit_n = 0;
    int      claim[DL_MAX_LIGHTS];   // DL_PRES_CLAIM: pool lights this list has claimed.
    uint32_t claim_n = 0;
    // DL_PRES_REC: an upload may rewrite what it re-sights only with the buffer's origin (the record's world
    // position) and a main-pass ctl, and (DL_PRES_FRESH) only when the last harvest itself set that origin or
    // DL_PRES_CUR placed another window at it with the camera on the same cells since (dl_pres_fresh). A stale
    // origin can cube-match a DIFFERENT light a grid step away (a row of lamps) and overwrite its record, and an
    // older origin may be stale by any rebase since, so an upload meeting neither test only keeps what it sights
    // alive. Residual: a rebase after the last harvest's windows were drawn (the camera crossing a cell) leaves this
    // span's later uploads a grid step off - for its own windows' buffers, and through DL_PRES_CUR for every
    // buffer at that origin where its camera test is blind (dl_pres_fresh).
    // DL_PRES_STALE: such an upload mostly shows it. A record whose light is in the pool finds that light a whole
    // number of land-grid cells from where the buffer's origin puts it (the engine's origins sit on whole cells,
    // DL_ORIGIN_GRID - the cell itself, not the learned multiple, which can be a multiple of the engine's step
    // until the origins refine it), and that light matches the record's own lanes (colour, cone, attenuation) better
    // than the light the origin put the record on (a neighbour). So the rewrites wait for the whole list, and none is
    // written when any record found a light elsewhere that matches it better than its pick, found one while matching
    // nothing, or was picked through the grid test alone (a light at another origin). What is left: a list whose every
    // light is new to the pool, or moved since its record, but for lamps a grid step from them - and a neighbour whose
    // lanes are the record's own (the same lamp model), whose rewrite moves it within the 1 m match cube for the span.
    // A correct origin shows the same only where a record's own light is missing or matches worse than a light a whole
    // number of cells away (within 3 cm): that list is stamp-only.
    float c_p[16];
    memcpy(c_p, ctl, sizeof(c_p));
    const bool rw = origin && dl_pres_fresh(buf, ox, oz, seq) &&
                    dl_ctl_main(c_p);
    bool any_org = false;   // DL_REF_SEEN: a record of this list re-sighted through the buffer's origin.
    bool     stale = false;              // DL_PRES_STALE: a record showed another origin.
    int      rw_pick[DL_MAX_LIGHTS];   // DL_PRES_STALE: the rewrites owed - pool light, record.
    uint32_t rw_rec[DL_MAX_LIGHTS];
    uint32_t rw_n = 0;
    const float cb = dl_grid_base();   // DL_PRES_STALE: the land-grid cell (0 = not read yet).
    // DL_PRES_STALE: does pool light pl put record r at another origin - a nonzero whole number of
    // cells off the buffer's, within the vote's residual, at the record's height?
    auto elsewhere = [&](const DlPoolLight& pl, const float* r) {
        if (!origin || !(cb > 0.0f) || !(fabsf(pl.rec[1] - r[1]) <= 0.25f)) return false;
        const float ix = pl.rec[0] - r[0] - ox;   // Its origin for the record, less the buffer's.
        const float iz = pl.rec[2] - r[2] - oz;
        const float rx = ix - cb * floorf(ix / cb + 0.5f);
        const float rz = iz - cb * floorf(iz / cb + 0.5f);
        return fabsf(rx) <= DL_ORIGIN_RES_MAX && fabsf(rz) <= DL_ORIGIN_RES_MAX &&
               (fabsf(ix) > 0.5f * cb || fabsf(iz) > 0.5f * cb);
    };
    // The lanes but the position (the pick's cost, less its distance).
    auto lanes = [](const DlPoolLight& pl, const float* r) {
        float n_c = 0.0f;
        for (int k = 3; k < 24; ++k) n_c += fabsf(pl.rec[k] - r[k]);
        return n_c;
    };

    // DL_PRES_MEMO: the entry this content maps to, and whether it holds this match (the key: the note above
    // DL_PMEMO_N). A hit takes the run's outcome and applies its sightings; the loop below does not run.
    const float q0 = dl_grid();
    DlPresMemo& me = g_dlcap.pmemo[static_cast<uint32_t>(chash) & (DL_PMEMO_N - 1u)];
    bool mh = me.epoch == g_dlcap.pres_epoch && me.seq == seq && me.total == total &&
              me.origin == (origin ? 1 : 0) && me.ox == dl_fbits(ox) &&
              me.oz == dl_fbits(oz) && me.rw == (rw ? 1 : 0) &&
              me.cb == dl_fbits(cb) && me.q == dl_fbits(q0) &&
              memcmp(me.bytes, ctl, DL_CTL_BYTES) == 0 &&
              memcmp(me.bytes + DL_CTL_BYTES, l, total * DL_LIGHT_BYTES) == 0;
    for (uint32_t k = 0; mh && k < me.pick_n; ++k) {   // The epoch guarantees it; a stale index never reads.
        if (me.pick[k] < 0 || static_cast<uint32_t>(me.pick[k]) >= g_dl.pool_n) mh = false;
    }
    if (g_prof.ds_on) {   // PROF_FUNNEL: a run's records against the pool (its visits: DL_PRES_HIX, below).
        if (mh) fun_add(FUNNEL_DL_REPLAYS, 1u);
        else { fun_add(FUNNEL_DL_RUNS, 1u); fun_add(FUNNEL_DL_PAIRS, static_cast<uint64_t>(total) * g_dl.pool_n); }
    }
    if (mh) {
        stat_inc(g_stats.dl_pres_memo_hits);
        for (uint32_t k = 0; k < me.pick_n; ++k) dl_pres_sight(g_dl.pool[me.pick[k]], now, seq);
        stale = me.stale != 0;
        any_org = me.any_org != 0;
        unknown = me.unknown;
        unpl = me.unpl;
        hits = me.hits;
        rw_n = me.rw_n;
        for (uint32_t k = 0; k < rw_n; ++k) {
            rw_pick[k] = me.rw_pick[k];
            rw_rec[k] = me.rw_rec[k];
        }
    }
    int      mp[DL_MAX_LIGHTS];   // DL_PRES_MEMO: this run's picks in record order (the entry's log).
    uint32_t mp_n = 0;
    uint64_t vis = 0;   // DL_PRES_HIX: the pairs this run visited (armed only).
    const bool hix = !mh && dl_phix_ready();   // DL_PRES_HIX: may the index serve this run?
    uint32_t walked = 0;   // DL_HIX_DEBT: the records that walked the pool (finite positions).

    for (uint32_t i = 0; !mh && i < total; ++i) {   // DL_PRES_MEMO: a hit has the outcome.
        const float* r = reinterpret_cast<const float*>(l + i * DL_LIGHT_BYTES);
        if (!dl_finite(r[0]) || !dl_finite(r[1]) || !dl_finite(r[2])) continue;
        float best = -1.0f;   // Census: the best grid residual among y-compatible pool lights.
        int   pick = -1;      // The claim: nearest candidate by position + every other lane.
        float pick_cost = 0.0f;
        bool  pick_org = false;   // DL_PRES_REC: the pick matched in the origin's cube.
        float w_best = -1.0f;     // DL_PRES_STALE: the best lanes among lights elsewhere (-1: none).

        // DL_PRES_HIX: the lights whose height can pass the band below, in pool order, when the index serves this
        // run (dl_phix_ready) and the band is narrow; else the whole pool. The two loops' bodies are the same text
        // (no indirection per light) - an edit to one is made to both.
        ++walked;
        if (hix && dl_phix_cands(r[1])) {
            if (g_prof.ds_on) vis += g_dlcap.phix.cand.size();
            for (const uint32_t d : g_dlcap.phix.cand) {
                const DlPoolLight& pl = g_dl.pool[d];
                // DL_PRES_BAND: the origin's cube (strictly within DL_MATCH_M), the stale test (elsewhere)
                // and the grid test (both within 0.25 m) each first need this light's height within their band of
                // the record's, by this same expression; a light outside both bands, or with a NaN height (fails
                // every compare), reaches only a `continue` below (no pick, no census, no stale verdict), so it is
                // skipped here, in pool order. In a town most of the pool lies outside any one record's band. Edit
                // the bands with the tests' own and with DL_PRES_HIX's interval (dl_phix_cands: [r - 1, r + 1],
                // the widest band).
                const float dyb = fabsf(pl.rec[1] - r[1]);
                if (!(dyb < DL_MATCH_M) && !(dyb <= 0.25f)) continue;
                bool  same = false;
                bool  same_org = false;
                float dpos = 0.0f;

                if (origin) {   // The harvest's transform, the re-sight's cube.
                    const float ax = fabsf(pl.rec[0] - (ox + r[0]));
                    const float ay = fabsf(pl.rec[1] - r[1]);
                    const float az = fabsf(pl.rec[2] - (oz + r[2]));
                    if (ax < DL_MATCH_M && ay < DL_MATCH_M && az < DL_MATCH_M) {
                        same = true;
                        same_org = true;
                        dpos = ax + ay + az;
                    }
                }
                // DL_PRES_STALE: read by the rewrites alone - none without rw, none once stale.
                if (rw && !stale && !same_org && elsewhere(pl, r)) {
                    const float wl = lanes(pl, r);
                    if (w_best < 0.0f || wl < w_best) w_best = wl;
                }

                // Grid test / census, on the learned grid (DL_ORIGIN_GRID; none learned: no grid match).
                const float q = dl_grid();
                if (!same && q > 0.0f && fabsf(pl.rec[1] - r[1]) <= 0.25f) {
                    float dx = pl.rec[0] - r[0];
                    dx -= q * floorf(dx / q + 0.5f);
                    float dz = pl.rec[2] - r[2];
                    dz -= q * floorf(dz / q + 0.5f);
                    const float res = fabsf(dx) > fabsf(dz) ? fabsf(dx) : fabsf(dz);
                    if (best < 0.0f || res < best) best = res;

                    // The fallback: no origin, or an origin gone stale on a flip.
                    if (res <= DL_ORIGIN_RES_MAX) {
                        same = true;
                        dpos = fabsf(dx) + fabsf(pl.rec[1] - r[1]) + fabsf(dz);
                    }
                }

                if (!same) continue;
                bool taken = false;
                for (uint32_t c = 0; c < claim_n; ++c) {
                    if (claim[c] == static_cast<int>(d)) { taken = true; break; }
                }
                if (taken) continue;   // An earlier record of this list is that light.
                float cost = dpos;
                for (int k = 3; k < 24; ++k) cost += fabsf(pl.rec[k] - r[k]);
                if (pick < 0 || cost < pick_cost) {
                    pick = static_cast<int>(d);
                    pick_cost = cost;
                    pick_org = same_org;
                }
            }
        } else {
            if (g_prof.ds_on) vis += g_dl.pool_n;
            for (uint32_t d = 0; d < g_dl.pool_n; ++d) {
                const DlPoolLight& pl = g_dl.pool[d];
                // DL_PRES_BAND: the origin's cube (strictly within DL_MATCH_M), the stale test (elsewhere)
                // and the grid test (both within 0.25 m) each first need this light's height within their band of
                // the record's, by this same expression; a light outside both bands, or with a NaN height (fails
                // every compare), reaches only a `continue` below (no pick, no census, no stale verdict), so it is
                // skipped here, in pool order. In a town most of the pool lies outside any one record's band. Edit
                // the bands with the tests' own and with DL_PRES_HIX's interval (dl_phix_cands: [r - 1, r + 1],
                // the widest band).
                const float dyb = fabsf(pl.rec[1] - r[1]);
                if (!(dyb < DL_MATCH_M) && !(dyb <= 0.25f)) continue;
                bool  same = false;
                bool  same_org = false;
                float dpos = 0.0f;

                if (origin) {   // The harvest's transform, the re-sight's cube.
                    const float ax = fabsf(pl.rec[0] - (ox + r[0]));
                    const float ay = fabsf(pl.rec[1] - r[1]);
                    const float az = fabsf(pl.rec[2] - (oz + r[2]));
                    if (ax < DL_MATCH_M && ay < DL_MATCH_M && az < DL_MATCH_M) {
                        same = true;
                        same_org = true;
                        dpos = ax + ay + az;
                    }
                }
                // DL_PRES_STALE: read by the rewrites alone - none without rw, none once stale.
                if (rw && !stale && !same_org && elsewhere(pl, r)) {
                    const float wl = lanes(pl, r);
                    if (w_best < 0.0f || wl < w_best) w_best = wl;
                }

                // Grid test / census, on the learned grid (DL_ORIGIN_GRID; none learned: no grid match).
                const float q = dl_grid();
                if (!same && q > 0.0f && fabsf(pl.rec[1] - r[1]) <= 0.25f) {
                    float dx = pl.rec[0] - r[0];
                    dx -= q * floorf(dx / q + 0.5f);
                    float dz = pl.rec[2] - r[2];
                    dz -= q * floorf(dz / q + 0.5f);
                    const float res = fabsf(dx) > fabsf(dz) ? fabsf(dx) : fabsf(dz);
                    if (best < 0.0f || res < best) best = res;

                    // The fallback: no origin, or an origin gone stale on a flip.
                    if (res <= DL_ORIGIN_RES_MAX) {
                        same = true;
                        dpos = fabsf(dx) + fabsf(pl.rec[1] - r[1]) + fabsf(dz);
                    }
                }

                if (!same) continue;
                bool taken = false;
                for (uint32_t c = 0; c < claim_n; ++c) {
                    if (claim[c] == static_cast<int>(d)) { taken = true; break; }
                }
                if (taken) continue;   // An earlier record of this list is that light.
                float cost = dpos;
                for (int k = 3; k < 24; ++k) cost += fabsf(pl.rec[k] - r[k]);
                if (pick < 0 || cost < pick_cost) {
                    pick = static_cast<int>(d);
                    pick_cost = cost;
                    pick_org = same_org;
                }
            }
        }
        // DL_PRES_STALE: this record's light found elsewhere, and a better match than the origin's (the note
        // above rw).
        if (w_best >= 0.0f &&
            (pick < 0 || !pick_org || w_best < lanes(g_dl.pool[pick], r))) {
            stale = true;
        }

        if (pick >= 0) {
            DlPoolLight& pl = g_dl.pool[pick];
            if (claim_n < DL_MAX_LIGHTS) claim[claim_n++] = pick;
            if (pick_org) any_org = true;   // DL_REF_SEEN.
            dl_pres_sight(pl, now, seq);   // DL_PRES_MEMO: the replay's body too.
            if (mp_n < DL_MAX_LIGHTS) mp[mp_n++] = pick;   // DL_PRES_MEMO.
            // DL_PRES_REC: owed (written below, DL_PRES_STALE). A grid-fallback match has no world
            // position and rewrites none. The light is claimed, so no later record picks it - but later records'
            // stale test (elsewhere, lanes) still reads its record, so the rewrite waits for the whole list.
            if (rw && pick_org && rw_n < DL_MAX_LIGHTS) {
                rw_pick[rw_n] = pick;
                rw_rec[rw_n] = i;
                ++rw_n;
            }
        }

        if (pick >= 0) {
            ++hits;
        } else {
            if (dl_unpl_recent(r, seq)) ++unpl; else ++unknown;
        }
    }

    if (g_prof.ds_on && !mh) fun_add(FUNNEL_DL_VISITS, vis);   // PROF_FUNNEL (DL_PRES_HIX).
    if (!mh && !hix) dl_phix_paid(walked);   // DL_HIX_DEBT: walks the index could not serve.

    // DL_PRES_REC: the re-sight carries the live record (spot direction, colour, position), written as the
    // merge writes it - unless a record showed the origin stale (DL_PRES_STALE): then stamped only.
    bool moved = false;   // DL_PRES_MEMO: a rewrite changed a record's bytes (what the match reads).
    for (uint32_t k = 0; k < rw_n && !stale; ++k) {
        DlPoolLight& pl = g_dl.pool[rw_pick[k]];
        // DL_REC_ORDER: a buffer sampled again at a later draw without a new upload holds an older content than
        // what a newer upload has written since: the light stays stamped, its record stands.
        if (pl.rec_wseq > wseq) continue;
        const float* r = reinterpret_cast<const float*>(l + rw_rec[k] * DL_LIGHT_BYTES);
        float was[24];   // DL_PRES_MEMO.
        memcpy(was, pl.rec, sizeof(was));
        memcpy(pl.rec, r, sizeof(pl.rec));
        pl.rec[0] = ox + r[0];   // +0 rewritten to absolute world.
        pl.rec[2] = oz + r[2];
        pl.spot = rw_rec[k] >= static_cast<uint32_t>(pc_i) ? 1 : 0;
        pl.rec_wseq = wseq;   // DL_REC_ORDER.
        if (memcmp(was, pl.rec, sizeof(was)) != 0) moved = true;
        dl_phix_move(static_cast<uint32_t>(rw_pick[k]), was[1], pl.rec[1]);   // DL_PRES_HIX.
    }
    if (moved) ++g_dlcap.pres_epoch;   // DL_PRES_MEMO: every entry stored before this is out of date.
    // DL_PRES_MEMO: a run that moved nothing is a fixed point - re-run now, it gives this outcome - and is
    // remembered while the land grid read the same at its end (the game thread stages the cell once a session).
    if (!mh && !moved && dl_fbits(dl_grid_base()) == dl_fbits(cb) &&
        dl_fbits(dl_grid()) == dl_fbits(q0)) {
        me.epoch = g_dlcap.pres_epoch;
        me.seq = seq;
        me.total = total;
        me.origin = origin ? 1 : 0;
        me.ox = dl_fbits(ox);
        me.oz = dl_fbits(oz);
        me.rw = rw ? 1 : 0;
        me.cb = dl_fbits(cb);
        me.q = dl_fbits(q0);
        me.stale = stale ? 1 : 0;
        me.any_org = any_org ? 1 : 0;
        me.unknown = unknown;
        me.unpl = unpl;
        me.hits = hits;
        me.pick_n = mp_n;
        for (uint32_t k = 0; k < mp_n; ++k) me.pick[k] = mp[k];
        me.rw_n = rw_n;
        for (uint32_t k = 0; k < rw_n; ++k) {
            me.rw_pick[k] = rw_pick[k];
            me.rw_rec[k] = rw_rec[k];
        }
        memcpy(me.bytes, ctl, DL_CTL_BYTES);
        memcpy(me.bytes + DL_CTL_BYTES, l, total * DL_LIGHT_BYTES);
    }

    // DL_REF_SEEN: a list that re-sighted a record through its buffer's origin (of any age, whatever its rewrite
    // took) offers its ctl to the reference beside the placed windows (dynlights_merge_windows): those are what the
    // routine quota and the unknown lights happened to author, and a variant whose lists carried no new light went
    // unseen. The ctl is the pass's, not the origin's: a cube hit only says the list carries lights.
    if (any_org && dl_finite(c_p[12]) && dl_finite(c_p[13]) && dl_finite(c_p[14]) &&
        dl_finite(c_p[8]) && c_p[8] > 1.0e-6f && c_p[8] < 1.0e6f) {
        if (g_dl.ref_seen_seq != seq) {   // The span's first.
            g_dl.ref_seen_seq = seq;
            g_dl.ref_seen_lum = -1.0f;
        }
        const float lum = c_p[12] + c_p[13] + c_p[14];
        if (dl_finite(lum) && lum > g_dl.ref_seen_lum) {
            g_dl.ref_seen_lum = lum;
            g_dl.ref_seen[0] = c_p[12];
            g_dl.ref_seen[1] = c_p[13];
            g_dl.ref_seen[2] = c_p[14];
            g_dl.ref_seen[3] = c_p[8];
        }
    }

    hit_n = hits;

    return unknown;
}

// DL_WIN_VIEW: stamp a window with the view it is drawn under - the cycle's view (g_ro.cycle_pv: the clear's
// bridge fetch, a later retry's, or a held one) at the capture. No view: the derivation and the range backstop stand
// down for this window. DL_PRES_CUR: view_live when it is the clear's own.
inline void dl_meta_view(DlWindowMeta& m) {
    m.view_live = dl_view_live() ? 1 : 0;   // DL_PRES_CUR.
    if (!g_ro.cycle_pv_valid) {
        m.view_ok = 0;
        return;
    }
    for (int k = 0; k < 3; ++k) {
        m.view[k * 4 + 0] = g_ro.cycle_pv.view[0][k];
        m.view[k * 4 + 1] = g_ro.cycle_pv.view[1][k];
        m.view[k * 4 + 2] = g_ro.cycle_pv.view[2][k];
        m.view[k * 4 + 3] = 0.0f;
    }
    cam_of(g_ro.cycle_pv.view, m.cam);
    m.view_ok = 1;
}

// One draw's cb10 / cb11 / VS windows, read from the shadows into the span's CPU arena (same layout as the GPU arena,
// so the merge is shared). Upload key: (buffer, first, shadow generation). Identical content within a span is one. A
// draw the capture cannot see whole marks the side incomplete, which sends this harvest to the TTL sweep instead of
// presence.
inline void dynlights_sample_draw_cpu(ID3D11DeviceContext1* c1, ID3D11Buffer* b11,
                                      UINT f11, UINT n11) {
    const int w = g_dl.arena_write;
    if (g_dl.win_count[w] > 0 && g_dl.win_meta[w][0].gen == 0) {
        // A mode switch mid-span: the side holds GPU-arena windows whose bytes are not in the CPU arena. Start over.
        g_dl.win_count[w] = 0;
        memset(g_dl.win_meta[w], 0, sizeof(g_dl.win_meta[w]));
        g_dl.side_miss[w] = 0;
        g_dl.side_uploads[w] = 0;
    }
    dl_want_note(b11, DL_KIND_CB11);   // DL_WANT_GATE.
    CbShadow* s11 = cbs_find(b11);
    if (!s11) {
        g_dl.side_miss[w] = 1;
        if (g_prof.ds_on) fun_add(FUNNEL_DL_MISS, 1u);   // PROF_FUNNEL.
        return;
    }
    // Every upload from here to the next injection belongs to the harvest that injection runs. A first draw of the span
    // that binds the same upload as the last draw of the previous span is this span's evidence, not a duplicate.
    const uint64_t seq = g_dl.harvest_seq + 1;

    // The same upload, already processed this span - one compare on the shadow, no list walk.
    if (s11->pres_gen == s11->gen && s11->pres_first == f11 &&
        s11->pres_seq == seq) {
        if (g_prof.ds_on) fun_add(FUNNEL_DL_SAME, 1u);   // PROF_FUNNEL.
        return;
    }

    ID3D11Buffer* b10 = nullptr;
    UINT f10 = 0, n10 = 0;
    c1->PSGetConstantBuffers1(10, 1, &b10, &f10, &n10);
    if (!b10) {
        return;
    }
    dl_want_note(b10, DL_KIND_CB10);   // DL_WANT_GATE.
    const CbShadow* s10 = cbs_find(b10);
    b10->Release();   // Identity taken; the shadow is what is read.
    const uint32_t off10 = f10 * 16u;
    const uint32_t off11 = f11 * 16u;
    if (!s10 || off10 + DL_CTL_BYTES > s10->bytes) {
        g_dl.side_miss[w] = 1;
        if (g_prof.ds_on) fun_add(FUNNEL_DL_MISS, 1u);   // PROF_FUNNEL.
        return;
    }
    if (off11 >= s11->bytes) {
        g_dl.side_miss[w] = 1;
        if (g_prof.ds_on) fun_add(FUNNEL_DL_MISS, 1u);   // PROF_FUNNEL.
        return;
    }
    uint32_t want11 = DL_ARR_BYTES;
    if (n11 > 0 && n11 * 16u < want11) want11 = n11 * 16u;
    if (off11 + want11 > s11->bytes) want11 = s11->bytes - off11;
    want11 -= want11 % DL_LIGHT_BYTES;
    const uint8_t* p10 = s10->data.data() + off10;
    const uint8_t* p11 = s11->data.data() + off11;

    // Content: ctl + the counted records (the merge reads the counts the same way; an implausible count hashes the
    // whole copied window).
    uint32_t hn = want11;
    {
        int32_t pc_i = 0, sc_i = 0;
        memcpy(&pc_i, p10 + 0, 4);
        memcpy(&sc_i, p10 + 16, 4);
        if (pc_i >= 0 && pc_i <= 1024 && sc_i >= 0 && sc_i <= 1024) {
            const uint32_t cn = (static_cast<uint32_t>(pc_i) + static_cast<uint32_t>(sc_i)) * DL_LIGHT_BYTES;
            if (cn < hn) hn = cn;
        }
    }
    const uint64_t h = dl_chash(p10, p11, hn);   // DL_CHASH.
    if (g_prof.ds_on) {   // PROF_FUNNEL.
        fun_add(FUNNEL_DL_LISTS, 1u);
        fun_add(FUNNEL_DL_HASH_B, DL_CTL_BYTES + hn);
    }
    // This upload is processed from here on, whatever becomes of it below.
    s11->pres_gen = s11->gen;
    s11->pres_first = f11;
    s11->pres_seq = seq;
    ++g_dl.side_uploads[w];

    for (uint32_t i = 0; i < g_dl.win_count[w]; ++i) {   // Identical content: its twin did the work.
        if (g_dl.win_meta[w][i].seq == seq && g_dl.win_meta[w][i].chash == h) {
            // DL_REC_ORDER: the same content captured again later - its twin's content is as new as this capture
            // (presence does not run for it; the window, at the harvest, writes it as this capture's).
            if (g_dl.win_meta[w][i].wseq != 0 && g_dl.win_meta[w][i].wseq < s11->wseq) {
                g_dl.win_meta[w][i].wseq = s11->wseq;
            }
            if (g_prof.ds_on) fun_add(FUNNEL_DL_TWIN, 1u);   // PROF_FUNNEL.
            return;
        }
    }

    // DL_PRESENCE: the records against the pool, no window needed.
    uint32_t unpl = 0, hit_n = 0;
    uint32_t unknown = dynlights_presence_upload(static_cast<const void*>(b11),
                                                 p10, p11, want11,
                                                 seq,
                                                 steady_now_ms(), unpl, hit_n,
                                                 s11->wseq,   // DL_REC_ORDER.
                                                 h);   // DL_PRES_MEMO.
    // A note may not starve a light of the window that could place it: an upload carrying two known lights clears the
    // vote's bar, so its noted records author as unknown ones do.
    if (unpl > 0 && hit_n >= 2) { unknown += unpl; unpl = 0; }

    // A window authors on: the routine quota, a buffer identity the harvest has not placed yet, or a record the pool
    // does not know. DL_UNPL_ORIGIN: a buffer that never places would otherwise author on every upload past the
    // notes, so the no-origin trigger yields to a standing note as well.
    float ox = 0.0f, oz = 0.0f;
    const bool routine = g_dl.win_count[w] < DL_WIN_ROUTINE ||
                         (unpl == 0 &&
                          !dl_win_origin_of(static_cast<const void*>(b11), ox, oz));
    if (!routine && unknown == 0) {
        return;
    }
    if (g_dl.win_count[w] >= DL_WIN_MAX_CPU) {
        // Refused: an unknown record waits for a window next span. Not an incomplete harvest - presence saw the upload.
        stat_inc(g_stats.dl_windows_refused);   // DL_DIAG.
        return;
    }
    if (g_dl.cpu_arena[w].size() < static_cast<size_t>(DL_WIN_MAX_CPU) * DL_WIN_BYTES) {
        g_dl.cpu_arena[w].assign(static_cast<size_t>(DL_WIN_MAX_CPU) * DL_WIN_BYTES, static_cast<uint8_t>(0));
    }

    const uint32_t slot = g_dl.win_count[w];
    uint8_t* dst = g_dl.cpu_arena[w].data() + static_cast<size_t>(slot) * DL_WIN_BYTES;
    memcpy(dst, p10, DL_CTL_BYTES);
    if (want11 > 0) memcpy(dst + DL_CTL_BYTES, p11, want11);

    DlWindowMeta& meta = g_dl.win_meta[w][slot];
    meta.buf = static_cast<void*>(b11);
    meta.first = f11;
    meta.bytes11 = want11;
    meta.gen = s11->gen;
    meta.wseq = s11->wseq;   // DL_REC_ORDER.
    meta.chash = h;
    meta.seq = seq;
    dl_meta_view(meta);   // DL_WIN_VIEW.

    ID3D11Buffer* vsb[DL_VS_SLOTS] = {};
    UINT vsf[DL_VS_SLOTS] = {}, vsn[DL_VS_SLOTS] = {};
    c1->VSGetConstantBuffers1(0, DL_VS_SLOTS, vsb, vsf, vsn);
    uint8_t* vs_dst = dst + DL_STAGE_BYTES;

    for (uint32_t vs = 0; vs < DL_VS_SLOTS; ++vs) {
        meta.vs_bytes[vs] = 0;

        if (vsb[vs]) {
            dl_want_note(vsb[vs], DL_KIND_VS);   // DL_WANT_GATE.
            const CbShadow* sv = cbs_find(vsb[vs]);
            const uint32_t voff = vsf[vs] * 16u;

            if (!sv) {
            } else if (voff < sv->bytes) {
                uint32_t vwant = DL_VS_BYTES;
                if (vsn[vs] > 0 && vsn[vs] * 16u < vwant) vwant = vsn[vs] * 16u;
                if (voff + vwant > sv->bytes) vwant = sv->bytes - voff;
                vwant &= ~15u;

                if (vwant >= 64u) {   // At least one 4x4.
                    memcpy(vs_dst, sv->data.data() + voff, vwant);
                    meta.vs_bytes[vs] = static_cast<uint16_t>(vwant);
                    if (g_prof.ds_on) fun_add(FUNNEL_DL_WIN_VS_B, vwant);   // PROF_FUNNEL.
                }
            }

            vsb[vs]->Release();
        }

        vs_dst += DL_VS_BYTES;
    }

    g_dl.win_count[w] = slot + 1;
    if (g_prof.ds_on) fun_add(FUNNEL_DL_WINDOWS, 1u);   // PROF_FUNNEL.
}

inline void dynlights_sample_draw(ID3D11DeviceContext* ctx) {
    ID3D11DeviceContext1* c1 = as_ctx1(ctx);
    if (!c1) return;
    ID3D11Buffer* b11 = nullptr;
    UINT f11 = 0, n11 = 0;
    c1->PSGetConstantBuffers1(11, 1, &b11, &f11, &n11);

    if (!b11) {
        if (g_prof.ds_on) fun_add(FUNNEL_DL_NOLIST, 1u);   // PROF_FUNNEL.
        return;
    }

    if (dl_cpu_wanted()) {   // DL_CPU: the shadows, the span's arena.
        dynlights_sample_draw_cpu(c1, b11, f11, n11);
        b11->Release();
        return;
    }

    // Arena capture: only when this window is new to the write side.
    const int w = g_dl.arena_write;
    bool have = false;
    if (g_dl.win_count[w] > 0 && g_dl.win_meta[w][0].gen != 0) {
        // A mode switch mid-span the other way: the side holds CPU windows whose bytes are not in the GPU arena. Start
        // over.
        g_dl.win_count[w] = 0;
        memset(g_dl.win_meta[w], 0, sizeof(g_dl.win_meta[w]));
        g_dl.side_miss[w] = 0;
        g_dl.side_uploads[w] = 0;
    }

    for (uint32_t i = 0; i < g_dl.win_count[w]; ++i) {
        if (g_dl.win_meta[w][i].buf == static_cast<void*>(b11) &&
            g_dl.win_meta[w][i].first == f11) {
            have = true;
            break;
        }
    }

    if (have) {
        b11->Release();
        return;
    }

    if (g_dl.win_count[w] >= DL_WIN_MAX) {
        b11->Release();
        return;
    }

    if (!g_dl.arena[w]) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);

        if (dev) {
            D3D11_BUFFER_DESC sd = {};
            sd.ByteWidth = DL_ARENA_BYTES;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            dev->CreateBuffer(&sd, nullptr, &g_dl.arena[w]);
            dev->Release();
        }
    }

    ID3D11Buffer* b10 = nullptr;
    UINT f10 = 0, n10 = 0;
    c1->PSGetConstantBuffers1(10, 1, &b10, &f10, &n10);
    ID3D11Buffer* vsb[DL_VS_SLOTS] = {};
    UINT vsf[DL_VS_SLOTS] = {}, vsn[DL_VS_SLOTS] = {};
    c1->VSGetConstantBuffers1(0, DL_VS_SLOTS, vsb, vsf, vsn);

    if (g_dl.arena[w] && b10) {
        D3D11_BUFFER_DESC d10 = {}, d11 = {};
        b10->GetDesc(&d10);
        b11->GetDesc(&d11);
        const uint32_t off10 = f10 * 16u;
        const uint32_t off11 = f11 * 16u;
        uint32_t want11 = DL_ARR_BYTES;
        if (n11 > 0 && n11 * 16u < want11) want11 = n11 * 16u;

        if (off10 + DL_CTL_BYTES <= d10.ByteWidth && off11 < d11.ByteWidth) {
            if (off11 + want11 > d11.ByteWidth) want11 = d11.ByteWidth - off11;
            want11 -= want11 % DL_LIGHT_BYTES;

            const uint32_t slot = g_dl.win_count[w];
            const uint32_t dst = slot * DL_WIN_BYTES;
            D3D11_BOX box10 = { off10, 0, 0, off10 + DL_CTL_BYTES, 1, 1 };
            ctx->CopySubresourceRegion(g_dl.arena[w], 0, dst, 0, 0, b10, 0, &box10);

            if (want11 > 0) {
                D3D11_BOX box11 = { off11, 0, 0, off11 + want11, 1, 1 };
                ctx->CopySubresourceRegion(g_dl.arena[w], 0, dst + DL_CTL_BYTES, 0, 0,
                                           b11, 0, &box11);
            }

            DlWindowMeta& meta = g_dl.win_meta[w][slot];
            meta.buf = static_cast<void*>(b11);
            meta.first = f11;
            meta.bytes11 = want11;
            // DL_WIN_VIEW: the rotation reference and true-world camera for the origin subtraction, per window (the
            // view this draw was drawn under).
            dl_meta_view(meta);
            uint32_t vs_dst = dst + DL_STAGE_BYTES;

            for (uint32_t vs = 0; vs < DL_VS_SLOTS; ++vs) {
                meta.vs_bytes[vs] = 0;

                if (vsb[vs]) {
                    D3D11_BUFFER_DESC vd = {};
                    vsb[vs]->GetDesc(&vd);
                    const uint32_t voff = vsf[vs] * 16u;

                    if (voff < vd.ByteWidth) {
                        uint32_t vwant = DL_VS_BYTES;
                        if (vsn[vs] > 0 && vsn[vs] * 16u < vwant) vwant = vsn[vs] * 16u;
                        if (voff + vwant > vd.ByteWidth) vwant = vd.ByteWidth - voff;
                        vwant &= ~15u;

                        if (vwant >= 64u) {   // At least one 4x4.
                            D3D11_BOX vbox = { voff, 0, 0, voff + vwant, 1, 1 };
                            ctx->CopySubresourceRegion(g_dl.arena[w], 0, vs_dst, 0, 0,
                                                       vsb[vs], 0, &vbox);
                            meta.vs_bytes[vs] = static_cast<uint16_t>(vwant);
                        }
                    }
                }

                vs_dst += DL_VS_BYTES;
            }

            g_dl.win_count[w] = slot + 1;
        }
    }

    if (b10) b10->Release();
    for (uint32_t vs = 0; vs < DL_VS_SLOTS; ++vs) {
        if (vsb[vs]) vsb[vs]->Release();
    }
    b11->Release();
}
}