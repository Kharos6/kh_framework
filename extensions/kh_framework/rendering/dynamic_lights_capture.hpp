#pragma once

// The injection half (rendering_integration_injection.hpp): the dynamic-light harvest, part 1: the CB shadow, the want
// gate, the presence memo and the sampled draws. Never include it on its own.
namespace RenderIntegration {

// Defined with the window table (kh_dl_win_slot's group, rendering/dynamic_lights_pool.hpp); the presence pass reads
// the origin a buffer last resolved to (at any harvest: KH_DL_PRES_FRESH tells which).
inline bool kh_dl_win_origin_of(const void* khp_buf, float& khp_ox, float& khp_oz);
inline bool kh_dl_pres_fresh(const void* khc_buf, float khc_ox, float khc_oz, uint64_t khc_seq);   // KH_DL_PRES_CUR.

// Defined in rendering/upload_funnel.hpp (declared here for the uses below).
inline bool kh_wc_copy(void* khwc_dst, const void* khwc_src, uint32_t khwc_n);

inline float kh_dl_grid_base() { return g_dl_cell.load(std::memory_order_relaxed); }
inline float kh_dl_grid() {
    const float khdg_c = kh_dl_grid_base();
    return (khdg_c > 0.0f && g_dl_q_cells > 0u) ? khdg_c * static_cast<float>(g_dl_q_cells) : 0.0f;
}
inline uint32_t kh_dl_gcd(uint32_t khgc_a, uint32_t khgc_b) {
    while (khgc_b != 0u) {
        const uint32_t khgc_t = khgc_a % khgc_b;
        khgc_a = khgc_b;
        khgc_b = khgc_t;
    }
    return khgc_a;
}
// A strong route's committed origin: its cell counts folded into the learned multiple (gcd(0, n) = n, so the first
// origin seeds it and every later one can only refine it).
inline void kh_dl_grid_learn(float khgl_ox, float khgl_oz) {
    const float khgl_c = kh_dl_grid_base();
    if (!(khgl_c > 0.0f)) return;
    const float khgl_fx = fabsf(khgl_ox) / khgl_c;
    const float khgl_fz = fabsf(khgl_oz) / khgl_c;
    if (!(khgl_fx < 1.0e6f && khgl_fz < 1.0e6f)) return;   // NaN fails too.
    uint32_t khgl_g = g_dl_q_cells;
    khgl_g = kh_dl_gcd(khgl_g, static_cast<uint32_t>(floorf(khgl_fx + 0.5f)));
    khgl_g = kh_dl_gcd(khgl_g, static_cast<uint32_t>(floorf(khgl_fz + 0.5f)));
    if (khgl_g > 0u) g_dl_q_cells = khgl_g;
}

inline uint64_t kh_dl_gap_mult() {
    return KH_DL_GAP_MULT;
}

inline bool kh_dl_dense(uint16_t khdn_sightings) {
    return khdn_sightings >= KH_DL_DENSE_SIGHTS;
}

// Cadence mark update: max(this gap, the decayed previous mark), so the TTL
// never falls below GAP_MULT re-sight intervals.
inline uint16_t kh_dl_gap_mark(uint16_t khg_prev, uint64_t khg_gap) {
    if (khg_gap >= 60000) return khg_prev;   // out-of-era stamp: no evidence.
    const uint16_t khg_now = static_cast<uint16_t>(khg_gap);

    const uint16_t khg_dec = static_cast<uint16_t>(
        (static_cast<uint32_t>(khg_prev) * KH_DL_GAP_DECAY_NUM) / KH_DL_GAP_DECAY_DEN);
    return khg_now > khg_dec ? khg_now : khg_dec;
}

inline bool kh_dl_cpu_wanted() {   // A lit mesh or a caster (g_ls.wanted), or a dynamicLightFog pass (KH_DLF_DEMAND).
    return g_ls.wanted.load(std::memory_order_relaxed) || g_dlf_wanted.load(std::memory_order_relaxed);
}
inline uint32_t kh_dl_want_hash(const void* khwh_id) {
    uint64_t khwh_h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(khwh_id));
    khwh_h ^= khwh_h >> 33;
    khwh_h *= 0xff51afd7ed558ccdull;
    khwh_h ^= khwh_h >> 33;
    return static_cast<uint32_t>(khwh_h) & (KH_DL_WANT_IX - 1u);
}
// The entry holding khwf_id, or KH_DL_WANT_N; khwf_probes (when given) takes the index slots looked at.
inline uint32_t kh_dl_want_find(const void* khwf_id, uint32_t* khwf_probes = nullptr) {
    uint32_t khwf_h = kh_dl_want_hash(khwf_id), khwf_n = 0u;
    for (uint32_t khwf_k = 0; khwf_k < KH_DL_WANT_IX; ++khwf_k) {
        const uint32_t khwf_e = g_dl_keep_ix[khwf_h];
        ++khwf_n;
        if (khwf_e == 0u) break;
        const uint32_t khwf_i = khwf_e - 1u;
        if (khwf_i < g_dl_keep_n && g_dl_keep_id[khwf_i] == khwf_id) {
            if (khwf_probes) *khwf_probes = khwf_n;
            return khwf_i;
        }
        khwf_h = (khwf_h + 1u) & (KH_DL_WANT_IX - 1u);
    }
    if (khwf_probes) *khwf_probes = khwf_n;
    return KH_DL_WANT_N;
}
inline void kh_dl_want_ix_put(uint32_t khwp_i) {
    uint32_t khwp_h = kh_dl_want_hash(g_dl_keep_id[khwp_i]);
    while (g_dl_keep_ix[khwp_h] != 0u) khwp_h = (khwp_h + 1u) & (KH_DL_WANT_IX - 1u);
    g_dl_keep_ix[khwp_h] = static_cast<uint8_t>(khwp_i + 1u);
}
inline void kh_dl_want_note(const void* khdw_id, uint8_t khdw_kind) {
    if (!khdw_id) return;
    // KH_DL_WANT_GATE: the persistent table (refresh, add, or replace the stalest).
    uint32_t khdw_at = kh_dl_want_find(khdw_id);   // KH_DL_WANT_INDEX.
    if (khdw_at == KH_DL_WANT_N) {
        if (g_dl_keep_n < KH_DL_WANT_N) {
            khdw_at = g_dl_keep_n++;
            g_dl_keep_id[khdw_at] = khdw_id;
            kh_dl_want_ix_put(khdw_at);
        } else {
            uint32_t khdw_st = 0u;   // The stalest entry, the lowest slot on a tie.
            for (uint32_t i = 1; i < g_dl_keep_n; ++i) {
                if (g_dl_keep_cycle[i] < g_dl_keep_cycle[khdw_st]) khdw_st = i;
            }
            khdw_at = khdw_st;
            g_dl_keep_id[khdw_at] = khdw_id;
            memset(g_dl_keep_ix, 0, sizeof(g_dl_keep_ix));   // KH_DL_WANT_INDEX: rebuilt whole.
            for (uint32_t i = 0; i < g_dl_keep_n; ++i) kh_dl_want_ix_put(i);
        }
        g_dl_keep_kind[khdw_at] = 0u;   // KH_PROF_FUNNEL: a new identity's roles start over.
    }
    g_dl_keep_cycle[khdw_at] = g_topo_cycles;
    if (g_prof_ds_on) g_dl_keep_kind[khdw_at] |= khdw_kind;   // KH_PROF_FUNNEL.
}
// KH_PROF_FUNNEL: khdg_kind / khdg_probes (when given) take the entry's roles and the index slots looked at.
inline bool kh_dl_want_gate(const void* khdg_id, uint8_t* khdg_kind = nullptr, uint32_t* khdg_probes = nullptr) {
    const uint32_t i = kh_dl_want_find(khdg_id, khdg_probes);   // KH_DL_WANT_INDEX.
    if (i == KH_DL_WANT_N) return false;
    if (khdg_kind) *khdg_kind = g_dl_keep_kind[i];
    return g_dl_keep_cycle[i] <= g_topo_cycles && g_topo_cycles - g_dl_keep_cycle[i] <= KH_DL_WANT_KEEP;
}
inline bool kh_dl_cpu_capture_now() {
    if (!kh_dl_cpu_wanted()) return false;
    if (g_dl_idle.load(std::memory_order_relaxed) < KH_DL_IDLE_FRAMES) return true;
    return (g_topo_cycles % KH_DL_IDLE_EVERY) == 0u;
}
inline uint32_t kh_cbs_hash(const void* khch_id) {
    uint64_t khch_h = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(khch_id));
    khch_h ^= khch_h >> 33;
    khch_h *= 0xff51afd7ed558ccdull;
    khch_h ^= khch_h >> 33;
    return static_cast<uint32_t>(khch_h) & (KH_CBS_N - 1);
}
// Read side (the sampler, render thread). Entries are never cleared singly
// (only whole-table at reset), so an empty slot ends the probe.
inline KhCbShadow* kh_cbs_find(const void* khcf_id) {
    if (!khcf_id) return nullptr;
    uint32_t khcf_i = kh_cbs_hash(khcf_id);
    for (uint32_t khcf_k = 0; khcf_k < KH_CBS_PROBE; ++khcf_k) {
        KhCbShadow& khcf_e = g_cbs[khcf_i];
        if (khcf_e.id == khcf_id) return &khcf_e;
        if (!khcf_e.id) return nullptr;
        khcf_i = (khcf_i + 1) & (KH_CBS_N - 1);
    }
    return nullptr;
}
// Write side (the hooks, render thread): copy the upload into the buffer's
// shadow and return it, so the scanners walk the shadow.
inline const void* kh_cbs_write(void* khcw_id, const void* khcw_src, uint32_t& khcw_bytes) {
    if (!khcw_id || !khcw_src || khcw_bytes == 0) return nullptr;
    if (khcw_bytes > KH_UPLOAD_SCRATCH_BYTES) khcw_bytes = KH_UPLOAD_SCRATCH_BYTES;
    uint32_t khcw_i = kh_cbs_hash(khcw_id);
    KhCbShadow* khcw_slot = nullptr;
    KhCbShadow* khcw_lru = nullptr;
    for (uint32_t khcw_k = 0; khcw_k < KH_CBS_PROBE; ++khcw_k) {
        KhCbShadow& khcw_e = g_cbs[khcw_i];
        if (khcw_e.id == khcw_id) { khcw_slot = &khcw_e; break; }
        if (!khcw_e.id) { khcw_slot = &khcw_e; break; }
        // KH_CBS_LRU_WSEQ: the 64-bit capture clock - gen wraps at 32 bits (hours at thousands of captures a
        // frame), and past a wrap '<' on gen would take the freshest entries for the least recently written.
        if (!khcw_lru || khcw_e.wseq < khcw_lru->wseq) khcw_lru = &khcw_e;
        khcw_i = (khcw_i + 1) & (KH_CBS_N - 1);
    }
    if (!khcw_slot) {
        if (!khcw_lru) return nullptr;
        khcw_slot = khcw_lru;   // The run is full: the least recently written identity is recycled.
    }
    if (khcw_slot->data.size() < khcw_bytes) khcw_slot->data.resize(khcw_bytes);
    kh_wc_copy(khcw_slot->data.data(), khcw_src, khcw_bytes);   // KH_WC_COPY: the mapped source.
    if (khcw_slot->id != khcw_id) khcw_slot->pres_gen = 0;   // A recycled entry forgets its key.
    khcw_slot->id = khcw_id;
    khcw_slot->bytes = khcw_bytes;
    if (++g_cbs_gen == 0) ++g_cbs_gen;
    khcw_slot->gen = g_cbs_gen;
    khcw_slot->wseq = ++g_cbs_wseq;   // KH_DL_REC_ORDER.
    return khcw_slot->data.data();
}
// KH_DL_WANT_STALE: the identity's shadow emptied in place - every reader's size test then misses, and the
// sampler's 'same upload again' test with it (pres_gen 0 matches no written generation).
inline void kh_cbs_stale(const void* khcs_id) {
    KhCbShadow* const khcs_s = kh_cbs_find(khcs_id);
    if (khcs_s) {
        khcs_s->bytes = 0;
        khcs_s->pres_gen = 0;
    }
}
inline void kh_cbs_clear() {
    for (uint32_t khcc_i = 0; khcc_i < KH_CBS_N; ++khcc_i) {
        g_cbs[khcc_i].id = nullptr;
        g_cbs[khcc_i].bytes = 0;
        g_cbs[khcc_i].gen = 0;
        g_cbs[khcc_i].wseq = 0;   // KH_DL_REC_ORDER.
        g_cbs[khcc_i].pres_gen = 0;
        g_cbs[khcc_i].pres_first = 0;
        g_cbs[khcc_i].pres_seq = 0;
        std::vector<uint8_t>().swap(g_cbs[khcc_i].data);
    }
}

inline bool dl_finite(float v) { return v == v && v < 1.0e30f && v > -1.0e30f; }
// KH_DL_PRES_CUR: is the cycle's view the clear's own bridge fetch - not held (cycle_pv_stale) and not a retry's
// mid-frame sample, which runs a step ahead of the frame (KH_SUN_CAM_LATCH)?
inline bool kh_dl_view_live() {
    return g_ro.cycle_pv_valid && !g_ro.cycle_pv_stale && g_boundary_pv_valid;
}

// The unplaceable-record table, keyed on the raw list-frame position at metre resolution (the same light lands on the
// same key while the origin stands; a flip re-keys it and the note lapses).
inline uint32_t kh_dl_unpl_slot(const int32_t khu_q[3], bool khu_insert, uint64_t khu_seq) {
    uint32_t khu_h = static_cast<uint32_t>(khu_q[0]) * 73856093u ^
                     static_cast<uint32_t>(khu_q[1]) * 19349663u ^
                     static_cast<uint32_t>(khu_q[2]) * 83492791u;
    khu_h &= KH_DL_UNPL_N - 1;
    uint32_t khu_oldest = khu_h;

    for (uint32_t k = 0; k < 8; ++k) {
        const uint32_t i = (khu_h + k) & (KH_DL_UNPL_N - 1);
        if (g_dl.unpl_seq[i] != 0 &&
            g_dl.unpl_q[i][0] == khu_q[0] && g_dl.unpl_q[i][1] == khu_q[1] && g_dl.unpl_q[i][2] == khu_q[2]) {
            if (khu_insert) g_dl.unpl_seq[i] = khu_seq;
            return i;
        }
        if (g_dl.unpl_seq[i] == 0) {
            if (!khu_insert) return KH_DL_UNPL_N;
            g_dl.unpl_q[i][0] = khu_q[0]; g_dl.unpl_q[i][1] = khu_q[1]; g_dl.unpl_q[i][2] = khu_q[2];
            g_dl.unpl_seq[i] = khu_seq;
            return i;
        }
        if (g_dl.unpl_seq[i] < g_dl.unpl_seq[khu_oldest]) khu_oldest = i;
    }

    if (!khu_insert) return KH_DL_UNPL_N;
    g_dl.unpl_q[khu_oldest][0] = khu_q[0]; g_dl.unpl_q[khu_oldest][1] = khu_q[1]; g_dl.unpl_q[khu_oldest][2] = khu_q[2];
    g_dl.unpl_seq[khu_oldest] = khu_seq;
    return khu_oldest;
}
inline void kh_dl_unpl_key(const float* khu_r, int32_t khu_q[3]) {
    khu_q[0] = static_cast<int32_t>(floorf(khu_r[0]));
    khu_q[1] = static_cast<int32_t>(floorf(khu_r[1]));
    khu_q[2] = static_cast<int32_t>(floorf(khu_r[2]));
}
// Is this record one the harvest recently could not place?
inline bool kh_dl_unpl_recent(const float* khu_r, uint64_t khu_now_seq) {
    int32_t khu_q[3];
    kh_dl_unpl_key(khu_r, khu_q);
    const uint32_t i = kh_dl_unpl_slot(khu_q, false, 0);
    if (i >= KH_DL_UNPL_N) return false;
    return khu_now_seq <= g_dl.unpl_seq[i] + KH_DL_UNPL_KEEP;
}
// Note every finite record of an unplaced window.
inline void kh_dl_unpl_note(const float* khu_l, uint32_t khu_n, uint64_t khu_seq) {
    for (uint32_t i = 0; i < khu_n; ++i) {
        const float* khu_r = khu_l + i * 24;
        if (!dl_finite(khu_r[0]) || !dl_finite(khu_r[1]) || !dl_finite(khu_r[2])) continue;
        int32_t khu_q[3];
        kh_dl_unpl_key(khu_r, khu_q);
        kh_dl_unpl_slot(khu_q, true, khu_seq);
    }
}

// KH_DL_PRES_REC: the merge's main-pass test (TWIN of the khd_main block in dynlights_merge_windows, behind its ctl[8]
// guard). Only a main-pass list may rewrite a record; an aux pass keeps a light alive without rewriting it.
inline bool kh_dl_ctl_main(const float* khm_c) {
    if (!dl_finite(khm_c[8]) || khm_c[8] <= 1.0e-6f || khm_c[8] >= 1.0e6f) return false;
    if (!g_dl.main_ref_valid) return true;
    for (int khm_k = 0; khm_k < 3; ++khm_k) {
        const float khm_ref = g_dl.main_gdiff[khm_k];
        const float khm_tol = 0.5f * (fabsf(khm_ref) > 0.02f ? fabsf(khm_ref) : 0.02f);
        if (fabsf(khm_c[12 + khm_k] - khm_ref) > khm_tol) return false;
    }
    return !(fabsf(khm_c[8] - g_dl.main_scale) > 0.25f * fabsf(g_dl.main_scale));
}
inline uint64_t kh_xxh_rotl(uint64_t khxl_x, int khxl_r) { return (khxl_x << khxl_r) | (khxl_x >> (64 - khxl_r)); }
inline uint64_t kh_xxh_round(uint64_t khxr_a, uint64_t khxr_w) {
    return kh_xxh_rotl(khxr_a + khxr_w * KH_XXH_P2, 31) * KH_XXH_P1;
}
inline void kh_xxh_stripe(uint64_t (&khxs_v)[4], const uint8_t* khxs_p) {
    for (int khxs_k = 0; khxs_k < 4; ++khxs_k) {
        uint64_t khxs_w;
        memcpy(&khxs_w, khxs_p + khxs_k * 8, 8);
        khxs_v[khxs_k] = kh_xxh_round(khxs_v[khxs_k], khxs_w);
    }
}
// khch_c: the ctl (KH_DL_CTL_BYTES); khch_l / khch_n: the records hashed (any length).
inline uint64_t kh_dl_chash(const uint8_t* khch_c, const uint8_t* khch_l, uint32_t khch_n) {
    static_assert(KH_DL_CTL_BYTES == 64u, "KH_DL_CHASH: the ctl is two whole stripes");
    // XXH64's lane seeds at seed 0: P1 + P2 and -P1 (mod 2^64), written out (no folded wrap for MSVC's C4307).
    uint64_t khch_v[4] = { 0x60EA27EEADC0B5D6ull, KH_XXH_P2, 0ull, 0x61C8864E7A143579ull };
    kh_xxh_stripe(khch_v, khch_c);
    kh_xxh_stripe(khch_v, khch_c + 32);
    uint32_t khch_i = 0;
    for (; khch_n - khch_i >= 32u; khch_i += 32u) kh_xxh_stripe(khch_v, khch_l + khch_i);
    uint64_t khch_h = kh_xxh_rotl(khch_v[0], 1) + kh_xxh_rotl(khch_v[1], 7) + kh_xxh_rotl(khch_v[2], 12) +
                      kh_xxh_rotl(khch_v[3], 18);
    for (int khch_k = 0; khch_k < 4; ++khch_k) {
        khch_h = (khch_h ^ kh_xxh_round(0ull, khch_v[khch_k])) * KH_XXH_P1 + KH_XXH_P4;
    }
    khch_h += static_cast<uint64_t>(KH_DL_CTL_BYTES) + khch_n;
    for (; khch_n - khch_i >= 8u; khch_i += 8u) {
        uint64_t khch_w;
        memcpy(&khch_w, khch_l + khch_i, 8);
        khch_h = kh_xxh_rotl(khch_h ^ kh_xxh_round(0ull, khch_w), 27) * KH_XXH_P1 + KH_XXH_P4;
    }
    if (khch_n - khch_i >= 4u) {
        uint32_t khch_w4;
        memcpy(&khch_w4, khch_l + khch_i, 4);
        khch_h = kh_xxh_rotl(khch_h ^ (static_cast<uint64_t>(khch_w4) * KH_XXH_P1), 23) * KH_XXH_P2 + KH_XXH_P3;
        khch_i += 4u;
    }
    for (; khch_i < khch_n; ++khch_i) {
        khch_h = kh_xxh_rotl(khch_h ^ (static_cast<uint64_t>(khch_l[khch_i]) * KH_XXH_P5), 11) * KH_XXH_P1;
    }
    khch_h ^= khch_h >> 33;
    khch_h *= KH_XXH_P2;
    khch_h ^= khch_h >> 29;
    khch_h *= KH_XXH_P3;
    khch_h ^= khch_h >> 32;
    return khch_h;
}
inline void kh_dl_pmemo_clear() {
    for (KhDlPresMemo& khpc_e : g_dl_pmemo) khpc_e.epoch = 0;
    ++g_dl_pres_epoch;
}
inline uint32_t kh_dl_fbits(float khfb_f) {
    uint32_t khfb_u;
    memcpy(&khfb_u, &khfb_f, 4);
    return khfb_u;
}
inline uint16_t kh_dl_gap_mark_at(uint16_t khgs_prev, uint64_t khgs_gap, uint64_t khgs_stamp, uint64_t khgs_now) {
    if (g_dl_stall_b != 0 && khgs_stamp <= g_dl_stall_a && khgs_now >= g_dl_stall_b) return khgs_prev;
    if (g_dl.pool_stamp != 0 && khgs_stamp <= g_dl.pool_stamp && khgs_now > g_dl.pool_stamp &&
        khgs_now - g_dl.pool_stamp > KH_DL_POOL_STALE_MS) return khgs_prev;
    return kh_dl_gap_mark(khgs_prev, khgs_gap);
}
// A sighting, priced as the aux re-sight prices one - presence's, a run's and a replay's (one body).
inline void kh_dl_pres_sight(DlPoolLight& khps_pl, uint64_t khps_now, uint64_t khps_seq) {
    if (khps_pl.stamp != 0) {
        const uint64_t khps_gap = khps_now >= khps_pl.stamp ? khps_now - khps_pl.stamp : 0;
        khps_pl.gap_max_ms = kh_dl_gap_mark_at(khps_pl.gap_max_ms, khps_gap, khps_pl.stamp, khps_now);
        if (khps_pl.sightings < 0xFFFF) khps_pl.sightings++;
    }
    khps_pl.stamp = khps_now;
    if (khps_seq > khps_pl.seq) khps_pl.seq = khps_seq;
}
// The index of the lowest set bit of a non-zero word (de Bruijn: no intrinsic, no unary minus on unsigned).
inline uint32_t kh_dl_bit_low(uint64_t khbl_w) {
    static constexpr uint8_t khbl_tab[64] = {
         0,  1, 48,  2, 57, 49, 28,  3, 61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19,  9, 13,  8,  7,  6,
    };
    return khbl_tab[((khbl_w & (~khbl_w + 1u)) * 0x03F79D71B4CB0A89ull) >> 58];
}
inline uint64_t kh_dl_phix_cost(uint32_t khx_n) {
    uint64_t khx_l = 1u;
    while ((1ull << khx_l) < khx_n) ++khx_l;
    return static_cast<uint64_t>(khx_n) * (khx_l + 2u) * KH_DL_HIX_STEP_PAIRS;
}
// The index rebuilt from the pool (KH_DL_HIX_OOM: invalid until it completes; an allocation may throw).
inline void kh_dl_phix_build() {
    KhDlPresHix& khx = g_dl_phix;
    khx.valid = false;
    const uint32_t khx_n = g_dl.pool_n;
    if (g_prof_ds_on) kh_fun(KHF_DL_HIX_BUILDS, 1u);   // KH_PROF_FUNNEL.
    khx.hb.resize(khx_n);
    khx.hs.clear();
    khx.hs.reserve(khx_n);     // At most one entry per light: kh_dl_phix_move's insert never allocates.
    khx.cand.reserve(khx_n);   // Nor does kh_dl_phix_cands' read-back.
    for (uint32_t d = 0; d < khx_n; ++d) {
        const float khx_h = g_dl.pool[d].rec[1];
        khx.hb[d] = kh_dl_fbits(khx_h);
        if (khx_h == khx_h) khx.hs.emplace_back(khx_h, d);   // NaN fails the band: never a candidate.
    }
    std::sort(khx.hs.begin(), khx.hs.end());   // Height, then index (no NaN: a strict weak order).
    khx.mark.assign((static_cast<size_t>(khx_n) + 63u) / 64u, 0ull);
    khx.n = khx_n;
    khx.epoch = g_dl_pres_epoch;
    khx.debt = 0u;
    khx.valid = true;
}
// A run's start (render thread): may the index serve this run? In step - its epoch, or a compare of every height's
// bits (which clears the debt) - it may. Heights out of step mark it invalid; a pool of another count leaves it
// unused (valid: the count may come back with the same heights, and the compare then finds it in step). Either way it
// is rebuilt (and may serve) only once the debt is paid (KH_DL_HIX_DEBT).
inline bool kh_dl_phix_ready() {
    KhDlPresHix& khx = g_dl_phix;
    const uint32_t khx_n = g_dl.pool_n;
    if (khx_n < KH_DL_HIX_MIN_POOL) return false;
    if (khx.valid && khx.n == khx_n) {
        if (khx.epoch == g_dl_pres_epoch) return true;
        bool khx_same = khx.hb.size() == khx_n;
        for (uint32_t d = 0; khx_same && d < khx_n; ++d) khx_same = khx.hb[d] == kh_dl_fbits(g_dl.pool[d].rec[1]);
        if (khx_same) { khx.epoch = g_dl_pres_epoch; khx.debt = 0u; return true; }
        khx.valid = false;   // Stale: walks repay a rebuild first.
    }
    if (khx.debt < KH_DL_HIX_PAYBACK * kh_dl_phix_cost(khx_n)) return false;
    kh_dl_phix_build();
    return khx.valid;
}
// A walk run while the index could not serve (kh_dl_phix_ready false on a pool it would cover): its pairs repay -
// khx_records: the records that walked (a record with a non-finite position walks nothing).
inline void kh_dl_phix_paid(uint32_t khx_records) {
    if (g_dl.pool_n < KH_DL_HIX_MIN_POOL) return;
    g_dl_phix.debt += static_cast<uint64_t>(khx_records) * g_dl.pool_n;
}
// A presence rewrite moved pool light khx_d's height from khx_was to khx_now: its entry moves with it, keeping a
// valid index in step (an index not in step for this light is left valid for the next run's compare, which finds
// it stale; an invalid one waits for its rebuild). Allocates nothing (the build reserved one entry per light), so it
// cannot throw between the rewrite and the epoch's bump.
inline void kh_dl_phix_move(uint32_t khx_d, float khx_was, float khx_now) {
    KhDlPresHix& khx = g_dl_phix;
    if (!khx.valid || khx_d >= khx.n || khx.hb.size() != khx.n) return;
    if (khx.hb[khx_d] != kh_dl_fbits(khx_was) || kh_dl_fbits(khx_was) == kh_dl_fbits(khx_now)) return;
    khx.valid = false;   // KH_DL_HIX_OOM: valid again only once the move completes.
    if (khx_was == khx_was) {
        const std::pair<float, uint32_t> khx_o(khx_was, khx_d);
        const auto khx_it = std::lower_bound(khx.hs.begin(), khx.hs.end(), khx_o);
        if (khx_it == khx.hs.end() || khx_it->second != khx_d || !(khx_it->first == khx_was)) return;   // Left stale.
        khx.hs.erase(khx_it);
    }
    if (khx_now == khx_now) {
        const std::pair<float, uint32_t> khx_e(khx_now, khx_d);
        khx.hs.insert(std::lower_bound(khx.hs.begin(), khx.hs.end(), khx_e), khx_e);
    }
    khx.hb[khx_d] = kh_dl_fbits(khx_now);
    if (g_prof_ds_on) kh_fun(KHF_DL_HIX_MOVES, 1u);   // KH_PROF_FUNNEL.
    khx.valid = true;
}
// A record's candidates at height khc_y (finite): pool indices, ascending, in g_dl_phix.cand. False: more than a
// sixth of the pool - walk all of it.
inline bool kh_dl_phix_cands(float khc_y) {
    KhDlPresHix& khx = g_dl_phix;
    khx.cand.clear();
    const double khc_lo = static_cast<double>(khc_y) - 1.0;
    const double khc_hi = static_cast<double>(khc_y) + 1.0;
    const auto khc_b = std::lower_bound(khx.hs.begin(), khx.hs.end(), khc_lo,
                                        [](const std::pair<float, uint32_t>& khc_e, double khc_v) {
                                            return static_cast<double>(khc_e.first) < khc_v;
                                        });
    // More than a sixth (k * 6 > n, k > floor(n / 6)): the (floor(n / 6) + 1)-th candidate exists and is in the band.
    const std::ptrdiff_t khc_q = static_cast<std::ptrdiff_t>(khx.n / 6u);
    if (khx.hs.end() - khc_b > khc_q && static_cast<double>(khc_b[khc_q].first) <= khc_hi) {
        return false;
    }
    auto khc_z = khc_b;
    while (khc_z != khx.hs.end() && static_cast<double>(khc_z->first) <= khc_hi) ++khc_z;   // At most n / 6.
    const size_t khc_k = static_cast<size_t>(khc_z - khc_b);
    if (khc_k == 1u) khx.cand.push_back(khc_b->second);
    if (khc_k < 2u) return true;
    uint32_t khc_wl = 0xFFFFFFFFu, khc_wh = 0u;
    for (auto khc_it = khc_b; khc_it != khc_z; ++khc_it) {
        const uint32_t khc_w = khc_it->second >> 6;
        khx.mark[khc_w] |= 1ull << (khc_it->second & 63u);
        if (khc_w < khc_wl) khc_wl = khc_w;
        if (khc_w > khc_wh) khc_wh = khc_w;
    }
    for (uint32_t khc_w = khc_wl; khc_w <= khc_wh; ++khc_w) {
        uint64_t khc_m = khx.mark[khc_w];
        khx.mark[khc_w] = 0ull;
        while (khc_m) {
            khx.cand.push_back(khc_w * 64u + kh_dl_bit_low(khc_m));
            khc_m &= khc_m - 1u;
        }
    }
    return true;
}

// One upload's records against the pool. Primary: the origin this buffer identity last resolved to (any harvest's)
// puts the record in world; the re-sight's KH_DL_MATCH_M cube names the pool light. No origin yet, or a miss: the
// origin-free grid test (height within 0.25, x / z a multiple of the learned grid, kh_dl_grid, apart within the
// vote's residual; off until a strong route has taught the grid). A record claims the ONE nearest candidate by
// position plus every non-position lane, never a light an earlier record of this list claimed. Returns the count of
// finite records that matched nothing and are not recently-unplaceable - the lights to author. KH_DL_PRES_REC: a
// main-pass upload's cube re-sight also rewrites the record, as the merge does - KH_DL_REC_ORDER: unless the record
// holds a newer capture's content (khp_wseq is this upload's capture order, the shadow's wseq).
inline uint32_t dynlights_presence_upload(const void* khp_buf,
                                          const uint8_t* khp_ctl, const uint8_t* khp_l,
                                          uint32_t khp_bytes11, uint64_t khp_seq,
                                          uint64_t khp_now,
                                          uint32_t& khp_unpl, uint32_t& khp_hit_n,   // KH_DL_UNPL: unplaced / matched.
                                          uint64_t khp_wseq,     // KH_DL_REC_ORDER.
                                          uint64_t khp_chash) {  // KH_DL_PRES_MEMO: the content's (KH_DL_CHASH).
    int32_t khp_pc_i = 0, khp_sc_i = 0;
    memcpy(&khp_pc_i, khp_ctl + 0, 4);
    memcpy(&khp_sc_i, khp_ctl + 16, 4);
    if (khp_pc_i < 0 || khp_pc_i > 1024 || khp_sc_i < 0 || khp_sc_i > 1024) return 0;
    uint32_t khp_total = static_cast<uint32_t>(khp_pc_i) + static_cast<uint32_t>(khp_sc_i);
    if (khp_total > KH_DL_MAX_LIGHTS) khp_total = KH_DL_MAX_LIGHTS;
    const uint32_t khp_avail = khp_bytes11 / KH_DL_LIGHT_BYTES;
    if (khp_total > khp_avail) khp_total = khp_avail;
    kh_stat(g_stats.dl_pres_calls);   // KH_DL_PRES_MEMO.
    float khp_ox = 0.0f, khp_oz = 0.0f;
    const bool khp_origin = kh_dl_win_origin_of(khp_buf, khp_ox, khp_oz);
    uint32_t khp_unknown = 0, khp_hits = 0;
    khp_unpl = 0;
    khp_hit_n = 0;
    int      khp_claim[KH_DL_MAX_LIGHTS];   // KH_DL_PRES_CLAIM: pool lights this list has claimed.
    uint32_t khp_claim_n = 0;
    // KH_DL_PRES_REC: an upload may rewrite what it re-sights only with the buffer's origin (the record's world
    // position) and a main-pass ctl, and (KH_DL_PRES_FRESH) only when the last harvest itself set that origin or
    // KH_DL_PRES_CUR placed another window at it with the camera on the same cells since (kh_dl_pres_fresh). A stale
    // origin can cube-match a DIFFERENT light a grid step away (a row of lamps) and overwrite its record, and an
    // older origin may be stale by any rebase since, so an upload meeting neither test only keeps what it sights
    // alive. Residual: a rebase after the last harvest's windows were drawn (the camera crossing a cell) leaves this
    // span's later uploads a grid step off - for its own windows' buffers, and through KH_DL_PRES_CUR for every
    // buffer at that origin where its camera test is blind (kh_dl_pres_fresh).
    // KH_DL_PRES_STALE: such an upload mostly shows it. A record whose light is in the pool finds that light a whole
    // number of land-grid cells from where the buffer's origin puts it (the engine's origins sit on whole cells,
    // KH_DL_ORIGIN_GRID - the cell itself, not the learned multiple, which can be a multiple of the engine's step
    // until the origins refine it), and that light matches the record's own lanes (colour, cone, attenuation) better
    // than the light the origin put the record on (a neighbour). So the rewrites wait for the whole list, and none is
    // written when any record found a light elsewhere that matches it better than its pick, found one while matching
    // nothing, or was picked through the grid test alone (a light at another origin). What is left: a list whose every
    // light is new to the pool, or moved since its record, but for lamps a grid step from them - and a neighbour whose
    // lanes are the record's own (the same lamp model), whose rewrite moves it within the 1 m match cube for the span.
    // A correct origin shows the same only where a record's own light is missing or matches worse than a light a whole
    // number of cells away (within 3 cm): that list is stamp-only.
    float khp_c[16];
    memcpy(khp_c, khp_ctl, sizeof(khp_c));
    const bool khp_rw = khp_origin && kh_dl_pres_fresh(khp_buf, khp_ox, khp_oz, khp_seq) &&
                        kh_dl_ctl_main(khp_c);
    bool khp_any_org = false;   // KH_DL_REF_SEEN: a record of this list re-sighted through the buffer's origin.
    bool     khp_stale = false;              // KH_DL_PRES_STALE: a record showed another origin.
    int      khp_rw_pick[KH_DL_MAX_LIGHTS];   // KH_DL_PRES_STALE: the rewrites owed - pool light, record.
    uint32_t khp_rw_rec[KH_DL_MAX_LIGHTS];
    uint32_t khp_rw_n = 0;
    const float khp_cb = kh_dl_grid_base();   // KH_DL_PRES_STALE: the land-grid cell (0 = not read yet).
    // KH_DL_PRES_STALE: does pool light khe_pl put record khe_r at another origin - a nonzero whole number of
    // cells off the buffer's, within the vote's residual, at the record's height?
    auto khp_elsewhere = [&](const DlPoolLight& khe_pl, const float* khe_r) {
        if (!khp_origin || !(khp_cb > 0.0f) || !(fabsf(khe_pl.rec[1] - khe_r[1]) <= 0.25f)) return false;
        const float khe_ix = khe_pl.rec[0] - khe_r[0] - khp_ox;   // Its origin for the record, less the buffer's.
        const float khe_iz = khe_pl.rec[2] - khe_r[2] - khp_oz;
        const float khe_rx = khe_ix - khp_cb * floorf(khe_ix / khp_cb + 0.5f);
        const float khe_rz = khe_iz - khp_cb * floorf(khe_iz / khp_cb + 0.5f);
        return fabsf(khe_rx) <= KH_DL_ORIGIN_RES_MAX && fabsf(khe_rz) <= KH_DL_ORIGIN_RES_MAX &&
               (fabsf(khe_ix) > 0.5f * khp_cb || fabsf(khe_iz) > 0.5f * khp_cb);
    };
    // The lanes but the position (the pick's cost, less its distance).
    auto khp_lanes = [](const DlPoolLight& khn_pl, const float* khn_r) {
        float khn_c = 0.0f;
        for (int k = 3; k < 24; ++k) khn_c += fabsf(khn_pl.rec[k] - khn_r[k]);
        return khn_c;
    };

    // KH_DL_PRES_MEMO: the entry this content maps to, and whether it holds this match (the key: the note above
    // KH_DL_PMEMO_N). A hit takes the run's outcome and applies its sightings; the loop below does not run.
    const float khp_q0 = kh_dl_grid();
    KhDlPresMemo& khp_me = g_dl_pmemo[static_cast<uint32_t>(khp_chash) & (KH_DL_PMEMO_N - 1u)];
    bool khp_mh = khp_me.epoch == g_dl_pres_epoch && khp_me.seq == khp_seq && khp_me.total == khp_total &&
                  khp_me.origin == (khp_origin ? 1 : 0) && khp_me.ox == kh_dl_fbits(khp_ox) &&
                  khp_me.oz == kh_dl_fbits(khp_oz) && khp_me.rw == (khp_rw ? 1 : 0) &&
                  khp_me.cb == kh_dl_fbits(khp_cb) && khp_me.q == kh_dl_fbits(khp_q0) &&
                  memcmp(khp_me.bytes, khp_ctl, KH_DL_CTL_BYTES) == 0 &&
                  memcmp(khp_me.bytes + KH_DL_CTL_BYTES, khp_l, khp_total * KH_DL_LIGHT_BYTES) == 0;
    for (uint32_t k = 0; khp_mh && k < khp_me.pick_n; ++k) {   // The epoch guarantees it; a stale index never reads.
        if (khp_me.pick[k] < 0 || static_cast<uint32_t>(khp_me.pick[k]) >= g_dl.pool_n) khp_mh = false;
    }
    if (g_prof_ds_on) {   // KH_PROF_FUNNEL: a run's records against the pool (its visits: KH_DL_PRES_HIX, below).
        if (khp_mh) kh_fun(KHF_DL_REPLAYS, 1u);
        else { kh_fun(KHF_DL_RUNS, 1u); kh_fun(KHF_DL_PAIRS, static_cast<uint64_t>(khp_total) * g_dl.pool_n); }
    }
    if (khp_mh) {
        kh_stat(g_stats.dl_pres_memo_hits);
        for (uint32_t k = 0; k < khp_me.pick_n; ++k) kh_dl_pres_sight(g_dl.pool[khp_me.pick[k]], khp_now, khp_seq);
        khp_stale = khp_me.stale != 0;
        khp_any_org = khp_me.any_org != 0;
        khp_unknown = khp_me.unknown;
        khp_unpl = khp_me.unpl;
        khp_hits = khp_me.hits;
        khp_rw_n = khp_me.rw_n;
        for (uint32_t k = 0; k < khp_rw_n; ++k) {
            khp_rw_pick[k] = khp_me.rw_pick[k];
            khp_rw_rec[k] = khp_me.rw_rec[k];
        }
    }
    int      khp_mp[KH_DL_MAX_LIGHTS];   // KH_DL_PRES_MEMO: this run's picks in record order (the entry's log).
    uint32_t khp_mp_n = 0;
    uint64_t khp_vis = 0;   // KH_DL_PRES_HIX: the pairs this run visited (armed only).
    const bool khp_hix = !khp_mh && kh_dl_phix_ready();   // KH_DL_PRES_HIX: may the index serve this run?
    uint32_t khp_walked = 0;   // KH_DL_HIX_DEBT: the records that walked the pool (finite positions).

    for (uint32_t i = 0; !khp_mh && i < khp_total; ++i) {   // KH_DL_PRES_MEMO: a hit has the outcome.
        const float* khp_r = reinterpret_cast<const float*>(khp_l + i * KH_DL_LIGHT_BYTES);
        if (!dl_finite(khp_r[0]) || !dl_finite(khp_r[1]) || !dl_finite(khp_r[2])) continue;
        float khp_best = -1.0f;   // Census: the best grid residual among y-compatible pool lights.
        int   khp_pick = -1;      // The claim: nearest candidate by position + every other lane.
        float khp_pick_cost = 0.0f;
        bool  khp_pick_org = false;   // KH_DL_PRES_REC: the pick matched in the origin's cube.
        float khp_w_best = -1.0f;     // KH_DL_PRES_STALE: the best lanes among lights elsewhere (-1: none).

        // KH_DL_PRES_HIX: the lights whose height can pass the band below, in pool order, when the index serves this
        // run (kh_dl_phix_ready) and the band is narrow; else the whole pool. The two loops' bodies are the same text
        // (no indirection per light) - an edit to one is made to both.
        ++khp_walked;
        if (khp_hix && kh_dl_phix_cands(khp_r[1])) {
            if (g_prof_ds_on) khp_vis += g_dl_phix.cand.size();
            for (const uint32_t d : g_dl_phix.cand) {
                const DlPoolLight& khp_pl = g_dl.pool[d];
                // KH_DL_PRES_BAND: the origin's cube (strictly within KH_DL_MATCH_M), the stale test (khp_elsewhere)
                // and the grid test (both within 0.25 m) each first need this light's height within their band of
                // the record's, by this same expression; a light outside both bands, or with a NaN height (fails
                // every compare), reaches only a `continue` below (no pick, no census, no stale verdict), so it is
                // skipped here, in pool order. In a town most of the pool lies outside any one record's band. Edit
                // the bands with the tests' own and with KH_DL_PRES_HIX's interval (kh_dl_phix_cands: [r - 1, r + 1],
                // the widest band).
                const float khp_dyb = fabsf(khp_pl.rec[1] - khp_r[1]);
                if (!(khp_dyb < KH_DL_MATCH_M) && !(khp_dyb <= 0.25f)) continue;
                bool  khp_same = false;
                bool  khp_same_org = false;
                float khp_dpos = 0.0f;

                if (khp_origin) {   // The harvest's transform, the re-sight's cube.
                    const float khp_ax = fabsf(khp_pl.rec[0] - (khp_ox + khp_r[0]));
                    const float khp_ay = fabsf(khp_pl.rec[1] - khp_r[1]);
                    const float khp_az = fabsf(khp_pl.rec[2] - (khp_oz + khp_r[2]));
                    if (khp_ax < KH_DL_MATCH_M && khp_ay < KH_DL_MATCH_M && khp_az < KH_DL_MATCH_M) {
                        khp_same = true;
                        khp_same_org = true;
                        khp_dpos = khp_ax + khp_ay + khp_az;
                    }
                }
                // KH_DL_PRES_STALE: read by the rewrites alone - none without khp_rw, none once stale.
                if (khp_rw && !khp_stale && !khp_same_org && khp_elsewhere(khp_pl, khp_r)) {
                    const float khp_wl = khp_lanes(khp_pl, khp_r);
                    if (khp_w_best < 0.0f || khp_wl < khp_w_best) khp_w_best = khp_wl;
                }

                // Grid test / census, on the learned grid (KH_DL_ORIGIN_GRID; none learned: no grid match).
                const float khp_q = kh_dl_grid();
                if (!khp_same && khp_q > 0.0f && fabsf(khp_pl.rec[1] - khp_r[1]) <= 0.25f) {
                    float khp_dx = khp_pl.rec[0] - khp_r[0];
                    khp_dx -= khp_q * floorf(khp_dx / khp_q + 0.5f);
                    float khp_dz = khp_pl.rec[2] - khp_r[2];
                    khp_dz -= khp_q * floorf(khp_dz / khp_q + 0.5f);
                    const float khp_res = fabsf(khp_dx) > fabsf(khp_dz) ? fabsf(khp_dx) : fabsf(khp_dz);
                    if (khp_best < 0.0f || khp_res < khp_best) khp_best = khp_res;

                    // The fallback: no origin, or an origin gone stale on a flip.
                    if (khp_res <= KH_DL_ORIGIN_RES_MAX) {
                        khp_same = true;
                        khp_dpos = fabsf(khp_dx) + fabsf(khp_pl.rec[1] - khp_r[1]) + fabsf(khp_dz);
                    }
                }

                if (!khp_same) continue;
                bool khp_taken = false;
                for (uint32_t c = 0; c < khp_claim_n; ++c) {
                    if (khp_claim[c] == static_cast<int>(d)) { khp_taken = true; break; }
                }
                if (khp_taken) continue;   // An earlier record of this list is that light.
                float khp_cost = khp_dpos;
                for (int k = 3; k < 24; ++k) khp_cost += fabsf(khp_pl.rec[k] - khp_r[k]);
                if (khp_pick < 0 || khp_cost < khp_pick_cost) {
                    khp_pick = static_cast<int>(d);
                    khp_pick_cost = khp_cost;
                    khp_pick_org = khp_same_org;
                }
            }
        } else {
            if (g_prof_ds_on) khp_vis += g_dl.pool_n;
            for (uint32_t d = 0; d < g_dl.pool_n; ++d) {
                const DlPoolLight& khp_pl = g_dl.pool[d];
                // KH_DL_PRES_BAND: the origin's cube (strictly within KH_DL_MATCH_M), the stale test (khp_elsewhere)
                // and the grid test (both within 0.25 m) each first need this light's height within their band of
                // the record's, by this same expression; a light outside both bands, or with a NaN height (fails
                // every compare), reaches only a `continue` below (no pick, no census, no stale verdict), so it is
                // skipped here, in pool order. In a town most of the pool lies outside any one record's band. Edit
                // the bands with the tests' own and with KH_DL_PRES_HIX's interval (kh_dl_phix_cands: [r - 1, r + 1],
                // the widest band).
                const float khp_dyb = fabsf(khp_pl.rec[1] - khp_r[1]);
                if (!(khp_dyb < KH_DL_MATCH_M) && !(khp_dyb <= 0.25f)) continue;
                bool  khp_same = false;
                bool  khp_same_org = false;
                float khp_dpos = 0.0f;

                if (khp_origin) {   // The harvest's transform, the re-sight's cube.
                    const float khp_ax = fabsf(khp_pl.rec[0] - (khp_ox + khp_r[0]));
                    const float khp_ay = fabsf(khp_pl.rec[1] - khp_r[1]);
                    const float khp_az = fabsf(khp_pl.rec[2] - (khp_oz + khp_r[2]));
                    if (khp_ax < KH_DL_MATCH_M && khp_ay < KH_DL_MATCH_M && khp_az < KH_DL_MATCH_M) {
                        khp_same = true;
                        khp_same_org = true;
                        khp_dpos = khp_ax + khp_ay + khp_az;
                    }
                }
                // KH_DL_PRES_STALE: read by the rewrites alone - none without khp_rw, none once stale.
                if (khp_rw && !khp_stale && !khp_same_org && khp_elsewhere(khp_pl, khp_r)) {
                    const float khp_wl = khp_lanes(khp_pl, khp_r);
                    if (khp_w_best < 0.0f || khp_wl < khp_w_best) khp_w_best = khp_wl;
                }

                // Grid test / census, on the learned grid (KH_DL_ORIGIN_GRID; none learned: no grid match).
                const float khp_q = kh_dl_grid();
                if (!khp_same && khp_q > 0.0f && fabsf(khp_pl.rec[1] - khp_r[1]) <= 0.25f) {
                    float khp_dx = khp_pl.rec[0] - khp_r[0];
                    khp_dx -= khp_q * floorf(khp_dx / khp_q + 0.5f);
                    float khp_dz = khp_pl.rec[2] - khp_r[2];
                    khp_dz -= khp_q * floorf(khp_dz / khp_q + 0.5f);
                    const float khp_res = fabsf(khp_dx) > fabsf(khp_dz) ? fabsf(khp_dx) : fabsf(khp_dz);
                    if (khp_best < 0.0f || khp_res < khp_best) khp_best = khp_res;

                    // The fallback: no origin, or an origin gone stale on a flip.
                    if (khp_res <= KH_DL_ORIGIN_RES_MAX) {
                        khp_same = true;
                        khp_dpos = fabsf(khp_dx) + fabsf(khp_pl.rec[1] - khp_r[1]) + fabsf(khp_dz);
                    }
                }

                if (!khp_same) continue;
                bool khp_taken = false;
                for (uint32_t c = 0; c < khp_claim_n; ++c) {
                    if (khp_claim[c] == static_cast<int>(d)) { khp_taken = true; break; }
                }
                if (khp_taken) continue;   // An earlier record of this list is that light.
                float khp_cost = khp_dpos;
                for (int k = 3; k < 24; ++k) khp_cost += fabsf(khp_pl.rec[k] - khp_r[k]);
                if (khp_pick < 0 || khp_cost < khp_pick_cost) {
                    khp_pick = static_cast<int>(d);
                    khp_pick_cost = khp_cost;
                    khp_pick_org = khp_same_org;
                }
            }
        }
        // KH_DL_PRES_STALE: this record's light found elsewhere, and a better match than the origin's (the note
        // above khp_rw).
        if (khp_w_best >= 0.0f &&
            (khp_pick < 0 || !khp_pick_org || khp_w_best < khp_lanes(g_dl.pool[khp_pick], khp_r))) {
            khp_stale = true;
        }

        if (khp_pick >= 0) {
            DlPoolLight& khp_pl = g_dl.pool[khp_pick];
            if (khp_claim_n < KH_DL_MAX_LIGHTS) khp_claim[khp_claim_n++] = khp_pick;
            if (khp_pick_org) khp_any_org = true;   // KH_DL_REF_SEEN.
            kh_dl_pres_sight(khp_pl, khp_now, khp_seq);   // KH_DL_PRES_MEMO: the replay's body too.
            if (khp_mp_n < KH_DL_MAX_LIGHTS) khp_mp[khp_mp_n++] = khp_pick;   // KH_DL_PRES_MEMO.
            // KH_DL_PRES_REC: owed (written below, KH_DL_PRES_STALE). A grid-fallback match has no world
            // position and rewrites none. The light is claimed, so no later record picks it - but later records'
            // stale test (khp_elsewhere, khp_lanes) still reads its record, so the rewrite waits for the whole list.
            if (khp_rw && khp_pick_org && khp_rw_n < KH_DL_MAX_LIGHTS) {
                khp_rw_pick[khp_rw_n] = khp_pick;
                khp_rw_rec[khp_rw_n] = i;
                ++khp_rw_n;
            }
        }

        if (khp_pick >= 0) {
            ++khp_hits;
        } else {
            if (kh_dl_unpl_recent(khp_r, khp_seq)) ++khp_unpl; else ++khp_unknown;
        }
    }

    if (g_prof_ds_on && !khp_mh) kh_fun(KHF_DL_VISITS, khp_vis);   // KH_PROF_FUNNEL (KH_DL_PRES_HIX).
    if (!khp_mh && !khp_hix) kh_dl_phix_paid(khp_walked);   // KH_DL_HIX_DEBT: walks the index could not serve.

    // KH_DL_PRES_REC: the re-sight carries the live record (spot direction, colour, position), written as the
    // merge writes it - unless a record showed the origin stale (KH_DL_PRES_STALE): then stamped only.
    bool khp_moved = false;   // KH_DL_PRES_MEMO: a rewrite changed a record's bytes (what the match reads).
    for (uint32_t k = 0; k < khp_rw_n && !khp_stale; ++k) {
        DlPoolLight& khp_pl = g_dl.pool[khp_rw_pick[k]];
        // KH_DL_REC_ORDER: a buffer sampled again at a later draw without a new upload holds an older content than
        // what a newer upload has written since: the light stays stamped, its record stands.
        if (khp_pl.rec_wseq > khp_wseq) continue;
        const float* khp_r = reinterpret_cast<const float*>(khp_l + khp_rw_rec[k] * KH_DL_LIGHT_BYTES);
        float khp_was[24];   // KH_DL_PRES_MEMO.
        memcpy(khp_was, khp_pl.rec, sizeof(khp_was));
        memcpy(khp_pl.rec, khp_r, sizeof(khp_pl.rec));
        khp_pl.rec[0] = khp_ox + khp_r[0];   // +0 rewritten to absolute world.
        khp_pl.rec[2] = khp_oz + khp_r[2];
        khp_pl.spot = khp_rw_rec[k] >= static_cast<uint32_t>(khp_pc_i) ? 1 : 0;
        khp_pl.rec_wseq = khp_wseq;   // KH_DL_REC_ORDER.
        if (memcmp(khp_was, khp_pl.rec, sizeof(khp_was)) != 0) khp_moved = true;
        kh_dl_phix_move(static_cast<uint32_t>(khp_rw_pick[k]), khp_was[1], khp_pl.rec[1]);   // KH_DL_PRES_HIX.
    }
    if (khp_moved) ++g_dl_pres_epoch;   // KH_DL_PRES_MEMO: every entry stored before this is out of date.
    // KH_DL_PRES_MEMO: a run that moved nothing is a fixed point - re-run now, it gives this outcome - and is
    // remembered while the land grid read the same at its end (the game thread stages the cell once a session).
    if (!khp_mh && !khp_moved && kh_dl_fbits(kh_dl_grid_base()) == kh_dl_fbits(khp_cb) &&
        kh_dl_fbits(kh_dl_grid()) == kh_dl_fbits(khp_q0)) {
        khp_me.epoch = g_dl_pres_epoch;
        khp_me.seq = khp_seq;
        khp_me.total = khp_total;
        khp_me.origin = khp_origin ? 1 : 0;
        khp_me.ox = kh_dl_fbits(khp_ox);
        khp_me.oz = kh_dl_fbits(khp_oz);
        khp_me.rw = khp_rw ? 1 : 0;
        khp_me.cb = kh_dl_fbits(khp_cb);
        khp_me.q = kh_dl_fbits(khp_q0);
        khp_me.stale = khp_stale ? 1 : 0;
        khp_me.any_org = khp_any_org ? 1 : 0;
        khp_me.unknown = khp_unknown;
        khp_me.unpl = khp_unpl;
        khp_me.hits = khp_hits;
        khp_me.pick_n = khp_mp_n;
        for (uint32_t k = 0; k < khp_mp_n; ++k) khp_me.pick[k] = khp_mp[k];
        khp_me.rw_n = khp_rw_n;
        for (uint32_t k = 0; k < khp_rw_n; ++k) {
            khp_me.rw_pick[k] = khp_rw_pick[k];
            khp_me.rw_rec[k] = khp_rw_rec[k];
        }
        memcpy(khp_me.bytes, khp_ctl, KH_DL_CTL_BYTES);
        memcpy(khp_me.bytes + KH_DL_CTL_BYTES, khp_l, khp_total * KH_DL_LIGHT_BYTES);
    }

    // KH_DL_REF_SEEN: a list that re-sighted a record through its buffer's origin (of any age, whatever its rewrite
    // took) offers its ctl to the reference beside the placed windows (dynlights_merge_windows): those are what the
    // routine quota and the unknown lights happened to author, and a variant whose lists carried no new light went
    // unseen. The ctl is the pass's, not the origin's: a cube hit only says the list carries lights.
    if (khp_any_org && dl_finite(khp_c[12]) && dl_finite(khp_c[13]) && dl_finite(khp_c[14]) &&
        dl_finite(khp_c[8]) && khp_c[8] > 1.0e-6f && khp_c[8] < 1.0e6f) {
        if (g_dl.ref_seen_seq != khp_seq) {   // The span's first.
            g_dl.ref_seen_seq = khp_seq;
            g_dl.ref_seen_lum = -1.0f;
        }
        const float khp_lum = khp_c[12] + khp_c[13] + khp_c[14];
        if (dl_finite(khp_lum) && khp_lum > g_dl.ref_seen_lum) {
            g_dl.ref_seen_lum = khp_lum;
            g_dl.ref_seen[0] = khp_c[12];
            g_dl.ref_seen[1] = khp_c[13];
            g_dl.ref_seen[2] = khp_c[14];
            g_dl.ref_seen[3] = khp_c[8];
        }
    }

    khp_hit_n = khp_hits;

    return khp_unknown;
}

// KH_DL_WIN_VIEW: stamp a window with the view it is drawn under - the cycle's view (g_ro.cycle_pv: the clear's
// bridge fetch, a later retry's, or a held one) at the capture. No view: the derivation and the range backstop stand
// down for this window. KH_DL_PRES_CUR: view_live when it is the clear's own.
inline void kh_dl_meta_view(DlWindowMeta& khd_m) {
    khd_m.view_live = kh_dl_view_live() ? 1 : 0;   // KH_DL_PRES_CUR.
    if (!g_ro.cycle_pv_valid) {
        khd_m.view_ok = 0;
        return;
    }
    for (int khd_k = 0; khd_k < 3; ++khd_k) {
        khd_m.view[khd_k * 4 + 0] = g_ro.cycle_pv.view[0][khd_k];
        khd_m.view[khd_k * 4 + 1] = g_ro.cycle_pv.view[1][khd_k];
        khd_m.view[khd_k * 4 + 2] = g_ro.cycle_pv.view[2][khd_k];
        khd_m.view[khd_k * 4 + 3] = 0.0f;
    }
    kh_cam_of(g_ro.cycle_pv.view, khd_m.cam);
    khd_m.view_ok = 1;
}

// One draw's cb10 / cb11 / VS windows, read from the shadows into the span's CPU arena (same layout as the GPU arena,
// so the merge is shared). Upload key: (buffer, first, shadow generation). Identical content within a span is one. A
// draw the capture cannot see whole marks the side incomplete, which sends this harvest to the TTL sweep instead of
// presence.
inline void dynlights_sample_draw_cpu(ID3D11DeviceContext1* khd_c1, ID3D11Buffer* khd_b11,
                                      UINT khd_f11, UINT khd_n11) {
    const int khd_w = g_dl.arena_write;
    if (g_dl.win_count[khd_w] > 0 && g_dl.win_meta[khd_w][0].gen == 0) {
        // A mode switch mid-span: the side holds GPU-arena windows whose bytes are not in the CPU arena. Start over.
        g_dl.win_count[khd_w] = 0;
        memset(g_dl.win_meta[khd_w], 0, sizeof(g_dl.win_meta[khd_w]));
        g_dl.side_miss[khd_w] = 0;
        g_dl.side_uploads[khd_w] = 0;
    }
    kh_dl_want_note(khd_b11, KH_DL_KIND_CB11);   // KH_DL_WANT_GATE.
    KhCbShadow* khd_s11 = kh_cbs_find(khd_b11);
    if (!khd_s11) {
        g_dl.side_miss[khd_w] = 1;
        if (g_prof_ds_on) kh_fun(KHF_DL_MISS, 1u);   // KH_PROF_FUNNEL.
        return;
    }
    // Every upload from here to the next injection belongs to the harvest that injection runs. A first draw of the span
    // that binds the same upload as the last draw of the previous span is this span's evidence, not a duplicate.
    const uint64_t khd_seq = g_dl.harvest_seq + 1;

    // The same upload, already processed this span - one compare on the shadow, no list walk.
    if (khd_s11->pres_gen == khd_s11->gen && khd_s11->pres_first == khd_f11 &&
        khd_s11->pres_seq == khd_seq) {
        if (g_prof_ds_on) kh_fun(KHF_DL_SAME, 1u);   // KH_PROF_FUNNEL.
        return;
    }

    ID3D11Buffer* khd_b10 = nullptr;
    UINT khd_f10 = 0, khd_n10 = 0;
    khd_c1->PSGetConstantBuffers1(10, 1, &khd_b10, &khd_f10, &khd_n10);
    if (!khd_b10) {
        return;
    }
    kh_dl_want_note(khd_b10, KH_DL_KIND_CB10);   // KH_DL_WANT_GATE.
    const KhCbShadow* khd_s10 = kh_cbs_find(khd_b10);
    khd_b10->Release();   // Identity taken; the shadow is what is read.
    const uint32_t khd_off10 = khd_f10 * 16u;
    const uint32_t khd_off11 = khd_f11 * 16u;
    if (!khd_s10 || khd_off10 + KH_DL_CTL_BYTES > khd_s10->bytes) {
        g_dl.side_miss[khd_w] = 1;
        if (g_prof_ds_on) kh_fun(KHF_DL_MISS, 1u);   // KH_PROF_FUNNEL.
        return;
    }
    if (khd_off11 >= khd_s11->bytes) {
        g_dl.side_miss[khd_w] = 1;
        if (g_prof_ds_on) kh_fun(KHF_DL_MISS, 1u);   // KH_PROF_FUNNEL.
        return;
    }
    uint32_t khd_want11 = KH_DL_ARR_BYTES;
    if (khd_n11 > 0 && khd_n11 * 16u < khd_want11) khd_want11 = khd_n11 * 16u;
    if (khd_off11 + khd_want11 > khd_s11->bytes) khd_want11 = khd_s11->bytes - khd_off11;
    khd_want11 -= khd_want11 % KH_DL_LIGHT_BYTES;
    const uint8_t* khd_p10 = khd_s10->data.data() + khd_off10;
    const uint8_t* khd_p11 = khd_s11->data.data() + khd_off11;

    // Content: ctl + the counted records (the merge reads the counts the same way; an implausible count hashes the
    // whole copied window).
    uint32_t khd_hn = khd_want11;
    {
        int32_t khd_pc_i = 0, khd_sc_i = 0;
        memcpy(&khd_pc_i, khd_p10 + 0, 4);
        memcpy(&khd_sc_i, khd_p10 + 16, 4);
        if (khd_pc_i >= 0 && khd_pc_i <= 1024 && khd_sc_i >= 0 && khd_sc_i <= 1024) {
            const uint32_t khd_cn = (static_cast<uint32_t>(khd_pc_i) + static_cast<uint32_t>(khd_sc_i)) * KH_DL_LIGHT_BYTES;
            if (khd_cn < khd_hn) khd_hn = khd_cn;
        }
    }
    const uint64_t khd_h = kh_dl_chash(khd_p10, khd_p11, khd_hn);   // KH_DL_CHASH.
    if (g_prof_ds_on) {   // KH_PROF_FUNNEL.
        kh_fun(KHF_DL_LISTS, 1u);
        kh_fun(KHF_DL_HASH_B, KH_DL_CTL_BYTES + khd_hn);
    }
    // This upload is processed from here on, whatever becomes of it below.
    khd_s11->pres_gen = khd_s11->gen;
    khd_s11->pres_first = khd_f11;
    khd_s11->pres_seq = khd_seq;
    ++g_dl.side_uploads[khd_w];

    for (uint32_t i = 0; i < g_dl.win_count[khd_w]; ++i) {   // Identical content: its twin did the work.
        if (g_dl.win_meta[khd_w][i].seq == khd_seq && g_dl.win_meta[khd_w][i].chash == khd_h) {
            // KH_DL_REC_ORDER: the same content captured again later - its twin's content is as new as this capture
            // (presence does not run for it; the window, at the harvest, writes it as this capture's).
            if (g_dl.win_meta[khd_w][i].wseq != 0 && g_dl.win_meta[khd_w][i].wseq < khd_s11->wseq) {
                g_dl.win_meta[khd_w][i].wseq = khd_s11->wseq;
            }
            if (g_prof_ds_on) kh_fun(KHF_DL_TWIN, 1u);   // KH_PROF_FUNNEL.
            return;
        }
    }

    // KH_DL_PRESENCE: the records against the pool, no window needed.
    uint32_t khd_unpl = 0, khd_hit_n = 0;
    uint32_t khd_unknown = dynlights_presence_upload(static_cast<const void*>(khd_b11),
                                                     khd_p10, khd_p11, khd_want11,
                                                     khd_seq,
                                                     steady_now_ms(), khd_unpl, khd_hit_n,
                                                     khd_s11->wseq,   // KH_DL_REC_ORDER.
                                                     khd_h);   // KH_DL_PRES_MEMO.
    // A note may not starve a light of the window that could place it: an upload carrying two known lights clears the
    // vote's bar, so its noted records author as unknown ones do.
    if (khd_unpl > 0 && khd_hit_n >= 2) { khd_unknown += khd_unpl; khd_unpl = 0; }

    // A window authors on: the routine quota, a buffer identity the harvest has not placed yet, or a record the pool
    // does not know. KH_DL_UNPL_ORIGIN: a buffer that never places would otherwise author on every upload past the
    // notes, so the no-origin trigger yields to a standing note as well.
    float khd_ox = 0.0f, khd_oz = 0.0f;
    const bool khd_routine = g_dl.win_count[khd_w] < KH_DL_WIN_ROUTINE ||
                             (khd_unpl == 0 &&
                              !kh_dl_win_origin_of(static_cast<const void*>(khd_b11), khd_ox, khd_oz));
    if (!khd_routine && khd_unknown == 0) {
        return;
    }
    if (g_dl.win_count[khd_w] >= KH_DL_WIN_MAX_CPU) {
        // Refused: an unknown record waits for a window next span. Not an incomplete harvest - presence saw the upload.
        kh_stat(g_stats.dl_windows_refused);   // KH_DL_DIAG.
        return;
    }
    if (g_dl.cpu_arena[khd_w].size() < static_cast<size_t>(KH_DL_WIN_MAX_CPU) * KH_DL_WIN_BYTES) {
        g_dl.cpu_arena[khd_w].assign(static_cast<size_t>(KH_DL_WIN_MAX_CPU) * KH_DL_WIN_BYTES, static_cast<uint8_t>(0));
    }

    const uint32_t khd_slot = g_dl.win_count[khd_w];
    uint8_t* khd_dst = g_dl.cpu_arena[khd_w].data() + static_cast<size_t>(khd_slot) * KH_DL_WIN_BYTES;
    memcpy(khd_dst, khd_p10, KH_DL_CTL_BYTES);
    if (khd_want11 > 0) memcpy(khd_dst + KH_DL_CTL_BYTES, khd_p11, khd_want11);

    DlWindowMeta& khd_meta = g_dl.win_meta[khd_w][khd_slot];
    khd_meta.buf = static_cast<void*>(khd_b11);
    khd_meta.first = khd_f11;
    khd_meta.bytes11 = khd_want11;
    khd_meta.gen = khd_s11->gen;
    khd_meta.wseq = khd_s11->wseq;   // KH_DL_REC_ORDER.
    khd_meta.chash = khd_h;
    khd_meta.seq = khd_seq;
    kh_dl_meta_view(khd_meta);   // KH_DL_WIN_VIEW.

    ID3D11Buffer* khd_vsb[KH_DL_VS_SLOTS] = {};
    UINT khd_vsf[KH_DL_VS_SLOTS] = {}, khd_vsn[KH_DL_VS_SLOTS] = {};
    khd_c1->VSGetConstantBuffers1(0, KH_DL_VS_SLOTS, khd_vsb, khd_vsf, khd_vsn);
    uint8_t* khd_vs_dst = khd_dst + KH_DL_STAGE_BYTES;

    for (uint32_t khd_vs = 0; khd_vs < KH_DL_VS_SLOTS; ++khd_vs) {
        khd_meta.vs_bytes[khd_vs] = 0;

        if (khd_vsb[khd_vs]) {
            kh_dl_want_note(khd_vsb[khd_vs], KH_DL_KIND_VS);   // KH_DL_WANT_GATE.
            const KhCbShadow* khd_sv = kh_cbs_find(khd_vsb[khd_vs]);
            const uint32_t khd_voff = khd_vsf[khd_vs] * 16u;

            if (!khd_sv) {
            } else if (khd_voff < khd_sv->bytes) {
                uint32_t khd_vwant = KH_DL_VS_BYTES;
                if (khd_vsn[khd_vs] > 0 && khd_vsn[khd_vs] * 16u < khd_vwant) khd_vwant = khd_vsn[khd_vs] * 16u;
                if (khd_voff + khd_vwant > khd_sv->bytes) khd_vwant = khd_sv->bytes - khd_voff;
                khd_vwant &= ~15u;

                if (khd_vwant >= 64u) {   // At least one 4x4.
                    memcpy(khd_vs_dst, khd_sv->data.data() + khd_voff, khd_vwant);
                    khd_meta.vs_bytes[khd_vs] = static_cast<uint16_t>(khd_vwant);
                    if (g_prof_ds_on) kh_fun(KHF_DL_WIN_VS_B, khd_vwant);   // KH_PROF_FUNNEL.
                }
            }

            khd_vsb[khd_vs]->Release();
        }

        khd_vs_dst += KH_DL_VS_BYTES;
    }

    g_dl.win_count[khd_w] = khd_slot + 1;
    if (g_prof_ds_on) kh_fun(KHF_DL_WINDOWS, 1u);   // KH_PROF_FUNNEL.
}

inline void dynlights_sample_draw(ID3D11DeviceContext* ctx) {
    ID3D11DeviceContext1* khd_c1 = kh_ctx1(ctx);
    if (!khd_c1) return;
    ID3D11Buffer* khd_b11 = nullptr;
    UINT khd_f11 = 0, khd_n11 = 0;
    khd_c1->PSGetConstantBuffers1(11, 1, &khd_b11, &khd_f11, &khd_n11);

    if (!khd_b11) {
        if (g_prof_ds_on) kh_fun(KHF_DL_NOLIST, 1u);   // KH_PROF_FUNNEL.
        return;
    }

    {   // Census: the session's first distinct (buffer, offset) pairs (g_dl_cd_*).
        bool khd_seen = false;

        for (uint32_t i = 0; i < g_dl_cd_n; ++i) {
            if (g_dl_cd_bufs[i] == static_cast<void*>(khd_b11) && g_dl_cd_first[i] == khd_f11) {
                khd_seen = true;
                break;
            }
        }

        if (!khd_seen && g_dl_cd_n < 8) {
            g_dl_cd_bufs[g_dl_cd_n] = static_cast<void*>(khd_b11);
            g_dl_cd_first[g_dl_cd_n] = khd_f11;
            g_dl_cd_n++;
        }
    }

    if (kh_dl_cpu_wanted()) {   // KH_DL_CPU: the shadows, the span's arena.
        dynlights_sample_draw_cpu(khd_c1, khd_b11, khd_f11, khd_n11);
        khd_b11->Release();
        return;
    }

    // Arena capture: only when this window is new to the write side.
    const int khd_w = g_dl.arena_write;
    bool khd_have = false;
    if (g_dl.win_count[khd_w] > 0 && g_dl.win_meta[khd_w][0].gen != 0) {
        // A mode switch mid-span the other way: the side holds CPU windows whose bytes are not in the GPU arena. Start
        // over.
        g_dl.win_count[khd_w] = 0;
        memset(g_dl.win_meta[khd_w], 0, sizeof(g_dl.win_meta[khd_w]));
        g_dl.side_miss[khd_w] = 0;
        g_dl.side_uploads[khd_w] = 0;
    }

    for (uint32_t i = 0; i < g_dl.win_count[khd_w]; ++i) {
        if (g_dl.win_meta[khd_w][i].buf == static_cast<void*>(khd_b11) &&
            g_dl.win_meta[khd_w][i].first == khd_f11) {
            khd_have = true;
            break;
        }
    }

    if (khd_have) {
        khd_b11->Release();
        return;
    }

    if (g_dl.win_count[khd_w] >= KH_DL_WIN_MAX) {
        khd_b11->Release();
        return;
    }

    if (!g_dl.arena[khd_w]) {
        ID3D11Device* khd_dev = nullptr;
        ctx->GetDevice(&khd_dev);

        if (khd_dev) {
            D3D11_BUFFER_DESC khd_sd = {};
            khd_sd.ByteWidth = KH_DL_ARENA_BYTES;
            khd_sd.Usage = D3D11_USAGE_STAGING;
            khd_sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            khd_dev->CreateBuffer(&khd_sd, nullptr, &g_dl.arena[khd_w]);
            khd_dev->Release();
        }
    }

    ID3D11Buffer* khd_b10 = nullptr;
    UINT khd_f10 = 0, khd_n10 = 0;
    khd_c1->PSGetConstantBuffers1(10, 1, &khd_b10, &khd_f10, &khd_n10);
    ID3D11Buffer* khd_vsb[KH_DL_VS_SLOTS] = {};
    UINT khd_vsf[KH_DL_VS_SLOTS] = {}, khd_vsn[KH_DL_VS_SLOTS] = {};
    khd_c1->VSGetConstantBuffers1(0, KH_DL_VS_SLOTS, khd_vsb, khd_vsf, khd_vsn);

    if (g_dl.arena[khd_w] && khd_b10) {
        D3D11_BUFFER_DESC khd_d10 = {}, khd_d11 = {};
        khd_b10->GetDesc(&khd_d10);
        khd_b11->GetDesc(&khd_d11);
        const uint32_t khd_off10 = khd_f10 * 16u;
        const uint32_t khd_off11 = khd_f11 * 16u;
        uint32_t khd_want11 = KH_DL_ARR_BYTES;
        if (khd_n11 > 0 && khd_n11 * 16u < khd_want11) khd_want11 = khd_n11 * 16u;

        if (khd_off10 + KH_DL_CTL_BYTES <= khd_d10.ByteWidth && khd_off11 < khd_d11.ByteWidth) {
            if (khd_off11 + khd_want11 > khd_d11.ByteWidth) khd_want11 = khd_d11.ByteWidth - khd_off11;
            khd_want11 -= khd_want11 % KH_DL_LIGHT_BYTES;

            const uint32_t khd_slot = g_dl.win_count[khd_w];
            const uint32_t khd_dst = khd_slot * KH_DL_WIN_BYTES;
            D3D11_BOX khd_box10 = { khd_off10, 0, 0, khd_off10 + KH_DL_CTL_BYTES, 1, 1 };
            ctx->CopySubresourceRegion(g_dl.arena[khd_w], 0, khd_dst, 0, 0, khd_b10, 0, &khd_box10);

            if (khd_want11 > 0) {
                D3D11_BOX khd_box11 = { khd_off11, 0, 0, khd_off11 + khd_want11, 1, 1 };
                ctx->CopySubresourceRegion(g_dl.arena[khd_w], 0, khd_dst + KH_DL_CTL_BYTES, 0, 0,
                                           khd_b11, 0, &khd_box11);
            }

            DlWindowMeta& khd_meta = g_dl.win_meta[khd_w][khd_slot];
            khd_meta.buf = static_cast<void*>(khd_b11);
            khd_meta.first = khd_f11;
            khd_meta.bytes11 = khd_want11;
            // KH_DL_WIN_VIEW: the rotation reference and true-world camera for the origin subtraction, per window (the
            // view this draw was drawn under).
            kh_dl_meta_view(khd_meta);
            uint32_t khd_vs_dst = khd_dst + KH_DL_STAGE_BYTES;

            for (uint32_t khd_vs = 0; khd_vs < KH_DL_VS_SLOTS; ++khd_vs) {
                khd_meta.vs_bytes[khd_vs] = 0;

                if (khd_vsb[khd_vs]) {
                    D3D11_BUFFER_DESC khd_vd = {};
                    khd_vsb[khd_vs]->GetDesc(&khd_vd);
                    const uint32_t khd_voff = khd_vsf[khd_vs] * 16u;

                    if (khd_voff < khd_vd.ByteWidth) {
                        uint32_t khd_vwant = KH_DL_VS_BYTES;
                        if (khd_vsn[khd_vs] > 0 && khd_vsn[khd_vs] * 16u < khd_vwant) khd_vwant = khd_vsn[khd_vs] * 16u;
                        if (khd_voff + khd_vwant > khd_vd.ByteWidth) khd_vwant = khd_vd.ByteWidth - khd_voff;
                        khd_vwant &= ~15u;

                        if (khd_vwant >= 64u) {   // At least one 4x4.
                            D3D11_BOX khd_vbox = { khd_voff, 0, 0, khd_voff + khd_vwant, 1, 1 };
                            ctx->CopySubresourceRegion(g_dl.arena[khd_w], 0, khd_vs_dst, 0, 0,
                                                       khd_vsb[khd_vs], 0, &khd_vbox);
                            khd_meta.vs_bytes[khd_vs] = static_cast<uint16_t>(khd_vwant);
                        }
                    }
                }

                khd_vs_dst += KH_DL_VS_BYTES;
            }

            g_dl.win_count[khd_w] = khd_slot + 1;
        }
    }

    if (khd_b10) khd_b10->Release();
    for (uint32_t khd_vs = 0; khd_vs < KH_DL_VS_SLOTS; ++khd_vs) {
        if (khd_vsb[khd_vs]) khd_vsb[khd_vs]->Release();
    }
    khd_b11->Release();
}
}