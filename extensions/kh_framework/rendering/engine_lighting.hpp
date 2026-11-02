#pragma once

// The injection half (rendering_integration_injection.hpp): the latched engine lighting: the fog staging, the sky sun,
// the world-lighting owner, the raw sun axis and the probes. Never include it on its own.
namespace RenderIntegration {
// Luminance-band agreement with an absolute epsilon so night-small values
// cannot ratio-flap: |a - b| <= max(15% of the larger, 0.02).

inline bool kh_probe_lum_band(float a, float b);   // Defined with the locator (the 15% band).
// The main depth clear: close the frame, publish the brightest substantial cluster's representative.
inline bool kh_probe_world_cycle();   // Defined with the locator.
inline bool kh_probe_world_cycle();   // Defined with the locator.

inline void locator_capture(CbColorProbe& pr, const float* f, uint32_t nf, uint32_t win);

// Sky-static predicate for the witness consumers: fresh mirror, an established
// motion window, and sub-0.3 deg sky movement across it.
inline bool kh_sky_static() {
    return g_skysun_ref_valid &&
           steady_now_ms() - g_skysun_ref_ms < 1000 &&
           g_skysun_mot_deg >= 0.0f && g_skysun_mot_deg < 0.3f;
}
inline bool kh_sun_axis_lane_fresh(uint64_t khlf_now) {
    return g_skysun_ref_valid && (khlf_now - g_skysun_ref_ms) < KH_SUN_AXIS_SKY_FRESH_MS;
}
inline bool kh_sun_axis_lane_ok(uint64_t khlo_now) {
    return kh_sun_axis_lane_fresh(khlo_now) && g_skysun_ref[1] > KH_SUN_AXIS_LANE_MIN_Y;
}

inline void kh_blk_owner_close_span() {
    if (!g_blko_open) return;
    g_blko_open = false;
    if (g_blko_open_i < 0 || g_blko_open_i >= g_blko_n) return;
    const uint64_t khoc_d64 = g_blko_draws >= g_blko_open_draws ? g_blko_draws - g_blko_open_draws : 0;
    const uint32_t khoc_d = static_cast<uint32_t>(khoc_d64 > 0xFFFFFFFFull ? 0xFFFFFFFFull : khoc_d64);
    KhBlkOwn& khoc_e = g_blko_cur[g_blko_open_i];
    khoc_e.draws += khoc_d;
    if (khoc_d > khoc_e.rep_draws || khoc_e.members == 1) {   // The member that owned the most itself represents.
        khoc_e.rep_draws = khoc_d;
        memcpy(khoc_e.amb, g_blko_open_amb, sizeof(khoc_e.amb));
        memcpy(khoc_e.sun, g_blko_open_sun, sizeof(khoc_e.sun));
    }
}
// Every admitted upload: the previous span closes, this upload joins its
// cluster (or opens one) and owns from here. State only.
inline void kh_blk_owner_note(const float* khon_b, uint8_t khon_flag) {
    kh_blk_owner_close_span();
    const float khon_al = khon_b[8] + khon_b[9] + khon_b[10];
    const float khon_sl = khon_b[16] + khon_b[17] + khon_b[18];
    int khon_i = -1;
    for (int khon_k = 0; khon_k < g_blko_n; ++khon_k) {
        if (((g_blko_cur[khon_k].flag ^ khon_flag) & 2u) != 0u) continue;   // Zero never merges with lit.
        if (kh_probe_lum_band(khon_sl, g_blko_cur[khon_k].sl) && kh_probe_lum_band(khon_al, g_blko_cur[khon_k].al)) { khon_i = khon_k; break; }
    }
    if (khon_i < 0) {
        if (g_blko_n >= KH_BLKO_MAX) { g_blko_open_i = -1; return; }
        khon_i = g_blko_n++;
        KhBlkOwn& khon_e = g_blko_cur[khon_i];
        memset(&khon_e, 0, sizeof(khon_e));
        khon_e.at = static_cast<uint32_t>(g_blko_draws); khon_e.flag = khon_flag;
        khon_e.al = khon_al; khon_e.sl = khon_sl;   // The cluster's band is anchored on its first member.
    }
    KhBlkOwn& khon_c = g_blko_cur[khon_i];
    ++khon_c.members;
    khon_c.flag |= khon_flag;
    for (int khon_k = 0; khon_k < 3; ++khon_k) { g_blko_open_amb[khon_k] = khon_b[8 + khon_k]; g_blko_open_sun[khon_k] = khon_b[16 + khon_k]; }
    g_blko_open_i = khon_i;
    g_blko_open_draws = g_blko_draws;
    g_blko_open = true;
}
// One admitted upload of the locked block (khba_b = the block base: lanes 8-10
// ambient, 16-18 sun, 36-38 fog; khba_avail = floats readable from it).
inline void kh_blk_note_admitted(const float* khba_b, uint32_t khba_avail, bool khba_world) {
    const float khba_al = khba_b[8] + khba_b[9] + khba_b[10];
    const float khba_sl = khba_b[16] + khba_b[17] + khba_b[18];
    const float khba_fl = khba_avail > 38 ? khba_b[36] + khba_b[37] + khba_b[38] : 1.0f;
    // KH_BLK_FINITE: an ambient or sun sum that is NaN or beyond +-1e30 (inf; no colour) is flagged with the clears:
    // never a base (once picked, no '>' could displace a NaN cluster, and the frame fill copies the pick raw into
    // lighting2 / lightAmb). Sums within +-1e30: unchanged.
    const bool khba_zero = (khba_al < 1.0e-6f && khba_sl < 1.0e-6f && khba_fl < 1.0e-6f) ||
                           !(fabsf(khba_al) <= 1.0e30f && fabsf(khba_sl) <= 1.0e30f);
    const uint8_t khba_flag = static_cast<uint8_t>((khba_world ? 1u : 0u) | (khba_zero ? 2u : 0u));
    kh_blk_owner_note(khba_b, khba_flag);
}
inline void kh_blk_owner_frame() {
    kh_blk_owner_close_span();
    g_blko_open_i = -1;
    if (g_blko_n == 0) return;
    uint32_t khof_owned = 0;
    uint32_t khof_memb = 0;   // KH_BLKO_MEMBER: lit members, both shares' denominator.
    int khof_max = -1; uint32_t khof_max_d = 0;   // The draw-count winner: the fallback when no cluster clears the bar.
    for (int khof_i = 0; khof_i < g_blko_n; ++khof_i) {
        if (g_blko_cur[khof_i].flag & 2u) continue;
        khof_owned += g_blko_cur[khof_i].draws;
        khof_memb += g_blko_cur[khof_i].members;
        if (khof_max < 0 || g_blko_cur[khof_i].draws > khof_max_d) { khof_max = khof_i; khof_max_d = g_blko_cur[khof_i].draws; }
    }
    int khof_pick = -1;
    if (khof_owned > 0) {
        const float khof_bar_m = KH_BLKO_BASE_SHARE * static_cast<float>(khof_memb);
        for (int khof_i = 0; khof_i < g_blko_n; ++khof_i) {
            const KhBlkOwn& khof_e = g_blko_cur[khof_i];
            if (khof_e.flag & 2u) continue;   // All-zero is a clear, never a base.
            if (static_cast<float>(khof_e.members) < khof_bar_m) continue;
            if (khof_pick < 0) { khof_pick = khof_i; continue; }
            const KhBlkOwn& khof_p = g_blko_cur[khof_pick];
            const float khof_hi = khof_e.sl > khof_p.sl ? khof_e.sl : khof_p.sl;
            if (fabsf(khof_e.sl - khof_p.sl) <= 0.01f * khof_hi + 1.0e-3f) {   // The same sun (an ambient-only variant): the one that draws more is the lighting, not the lift.
                if (khof_e.draws > khof_p.draws || (khof_e.draws == khof_p.draws && khof_e.sl > khof_p.sl)) khof_pick = khof_i;
            } else if (khof_e.sl > khof_p.sl) {
                khof_pick = khof_i;
            }
        }
        if (g_pub_block_valid) {   // Admission hold: a standing base at least as bright stays down to KH_BLKO_BASE_KEEP.
            // The first cluster in table order in band with the standing publish is the one judged; two clusters can
            // both be in band with it while distinct from each other.
            const float khof_pal = g_pub_block_amb[0] + g_pub_block_amb[1] + g_pub_block_amb[2];
            const float khof_psl = g_pub_block_sun[0] + g_pub_block_sun[1] + g_pub_block_sun[2];
            for (int khof_i = 0; khof_i < g_blko_n; ++khof_i) {
                const KhBlkOwn& khof_e = g_blko_cur[khof_i];
                if (khof_e.flag & 2u) continue;
                if (!kh_probe_lum_band(khof_psl, khof_e.sl) || !kh_probe_lum_band(khof_pal, khof_e.al)) continue;
                if (khof_i != khof_pick && static_cast<float>(khof_e.members) >= KH_BLKO_BASE_KEEP * static_cast<float>(khof_memb) &&
                    (khof_pick < 0 || khof_e.sl >= g_blko_cur[khof_pick].sl)) { khof_pick = khof_i; }
                break;
            }
        }
        if (khof_pick < 0) khof_pick = khof_max;   // Nothing clears the member bar (many near-equal clusters): the largest owner.
    }
    const bool khof_world = kh_probe_world_cycle();
    // KH_DLSW_GEN_AMB: the frame's brightest lit world clusters. A sparse frame (fewer lit draws owned than
    // KH_LITE_CONF_DRAWS - the publish's own evidence bar, KH_LIT_EVIDENCE) keeps the last value: one stray upload
    // cannot set the floor.
    if (khof_world && khof_owned >= KH_LITE_CONF_DRAWS) {
        bool khof_gany = false;
        float khof_gamb[3] = { 0.0f, 0.0f, 0.0f }, khof_gsun[3] = { 0.0f, 0.0f, 0.0f };
        for (int khof_i = 0; khof_i < g_blko_n; ++khof_i) {
            const KhBlkOwn& khof_e = g_blko_cur[khof_i];
            if ((khof_e.flag & 2u) || !(khof_e.flag & 1u) || khof_e.draws == 0) continue;
            for (int khof_c = 0; khof_c < 3; ++khof_c) {
                if (khof_e.amb[khof_c] > khof_gamb[khof_c]) khof_gamb[khof_c] = khof_e.amb[khof_c];
                if (khof_e.sun[khof_c] > khof_gsun[khof_c]) khof_gsun[khof_c] = khof_e.sun[khof_c];
            }
            khof_gany = true;
        }
        if (khof_gany) {
            memcpy(g_blko_gen_amb, khof_gamb, sizeof(g_blko_gen_amb));
            memcpy(g_blko_gen_sun, khof_gsun, sizeof(g_blko_gen_sun));
            g_blko_gen_valid = true;
        }
    }
    if (khof_pick >= 0 && khof_world) {
        const KhBlkOwn& khof_w = g_blko_cur[khof_pick];
        bool khof_admit = true;
        if (g_pub_block_valid && khof_owned < KH_LITE_CONF_DRAWS) {   // KH_LIT_EVIDENCE.
            const float khof_psl = g_pub_block_sun[0] + g_pub_block_sun[1] + g_pub_block_sun[2];
            const float khof_pal = g_pub_block_amb[0] + g_pub_block_amb[1] + g_pub_block_amb[2];
            if (!(kh_probe_lum_band(khof_psl, khof_w.sl) && kh_probe_lum_band(khof_pal, khof_w.al))) {
                if (g_lite_cand_n > 0 && kh_probe_lum_band(g_lite_cand_sl, khof_w.sl) &&
                    kh_probe_lum_band(g_lite_cand_al, khof_w.al)) {
                    ++g_lite_cand_n;
                } else {
                    g_lite_cand_n = 1; g_lite_cand_sl = khof_w.sl; g_lite_cand_al = khof_w.al;
                }
                khof_admit = g_lite_cand_n >= KH_LITE_CONF_CLOSES;
            } else {
                g_lite_cand_n = 0;
            }
        } else {
            g_lite_cand_n = 0;
        }
        if (khof_admit) {
            for (int khof_c = 0; khof_c < 3; ++khof_c) { g_pub_block_amb[khof_c] = khof_w.amb[khof_c]; g_pub_block_sun[khof_c] = khof_w.sun[khof_c]; }
            g_pub_block_valid = true;
        }
    }
    g_blko_n = 0;
}
// KH_SUN_RAW: the sun direction publish. The engine states its light axis twice per frame: the cascade sampling
// matrices' depth row (the direction it actually shadows with; the moon at night) and the sky CB's sun lane (world
// space, day only). band_capture samples the cascade axis of every capture; at the main depth clear kh_sun_raw_frame
// takes the cycle's median (world cycles only) and publishes: the SKY LANE when fresh and within KH_SRAW_LANE_WIT_DEG
// of the consensus (it lands the frame the sun moves; the cascade axis converges over ~0.7 s and departs during camera
// moves); the STANDING LANE (Src 4) when the lane did not step since the previous close that named it (<=
// KH_SRAW_LANE_STILL_DEG) but the consensus left it (on a flick or camera switch the cascade axis leaves an unmoved
// lane by tens of degrees for 1-3 frames); the CASCADE CONSENSUS otherwise (night); the sky lane alone after
// KH_SRAW_SKY_FALLBACK_MS with no cascade (shadows off). A step past KH_SRAW_JUMP_DEG resets the union's map hash.
// Nothing here blends: the consensus published is its median over a ring of closes (before the ring fills, the close's
// own median), and only while that median is still or the lane witnesses the sun moving (KH_SRAW_SETTLE), when nothing
// stands yet (the first publish), or once an unsettled run reaches KH_SRAW_HOLD_CAP_MS; after a lane jump the lane
// stands until it settles. Otherwise the engine's own convergence, its rotation departures and its per-close spikes
// never replace a standing publish.
inline float kh_rc_median(float* khrm_v, int khrm_n) {   // Insertion sort: n <= KH_SRAW_RING.
    for (int khrm_i = 1; khrm_i < khrm_n; ++khrm_i) {
        const float khrm_x = khrm_v[khrm_i];
        int khrm_j = khrm_i - 1;
        while (khrm_j >= 0 && khrm_v[khrm_j] > khrm_x) { khrm_v[khrm_j + 1] = khrm_v[khrm_j]; --khrm_j; }
        khrm_v[khrm_j + 1] = khrm_x;
    }
    return khrm_v[khrm_n / 2];
}
inline void kh_sraw_ring_reset() { g_sraw_ring_n = 0; g_sraw_ring_i = 0; g_sraw_med_prev_valid = false; }

inline float kh_sraw_angle_deg(const float* khsa_a, const float* khsa_b) {
    float khsa_d = khsa_a[0] * khsa_b[0] + khsa_a[1] * khsa_b[1] + khsa_a[2] * khsa_b[2];
    khsa_d = khsa_d > 1.0f ? 1.0f : (khsa_d < -1.0f ? -1.0f : khsa_d);
    return acosf(khsa_d) * 57.29578f;
}
// One raw cascade axis (world, already skyward and unit) from the capture site.
inline void kh_sun_raw_note(const float* khsn_w) {
    g_sraw_last_sample_ms = steady_now_ms();
    if (g_sraw_cyc_n >= KH_SRAW_CYC) return;
    g_sraw_cyc[g_sraw_cyc_n][0] = khsn_w[0];
    g_sraw_cyc[g_sraw_cyc_n][1] = khsn_w[1];
    g_sraw_cyc[g_sraw_cyc_n][2] = khsn_w[2];
    ++g_sraw_cyc_n;
}
// At the main depth clear (render thread), before g_ro.cycle_vp_* resets (the ended cycle's viewport is the world test)
// and before the sun-jump block consumes g_sun_jump_pending.
inline void kh_sun_raw_frame() {
    const uint64_t khsf_now = steady_now_ms();
    const bool khsf_world = kh_probe_world_cycle();
    float khsf_m[3] = { 0.0f, 0.0f, 0.0f };
    uint8_t khsf_src = 0;
    const int khsf_n = g_sraw_cyc_n;
    g_sraw_cyc_n = 0;
    if (khsf_n > 0) {
        float khsf_x[KH_SRAW_CYC], khsf_y[KH_SRAW_CYC], khsf_z[KH_SRAW_CYC];
        for (int khsf_i = 0; khsf_i < khsf_n; ++khsf_i) {
            khsf_x[khsf_i] = g_sraw_cyc[khsf_i][0];
            khsf_y[khsf_i] = g_sraw_cyc[khsf_i][1];
            khsf_z[khsf_i] = g_sraw_cyc[khsf_i][2];
        }
        khsf_m[0] = kh_rc_median(khsf_x, khsf_n);
        khsf_m[1] = kh_rc_median(khsf_y, khsf_n);
        khsf_m[2] = kh_rc_median(khsf_z, khsf_n);
        const float khsf_l = sqrtf(khsf_m[0] * khsf_m[0] + khsf_m[1] * khsf_m[1] + khsf_m[2] * khsf_m[2]);
        if (khsf_l > 1.0e-6f) {
            khsf_m[0] /= khsf_l; khsf_m[1] /= khsf_l; khsf_m[2] /= khsf_l;
            if (khsf_m[1] < 0.0f) { khsf_m[0] = -khsf_m[0]; khsf_m[1] = -khsf_m[1]; khsf_m[2] = -khsf_m[2]; }
            khsf_src = 1;
        }
    } else if (kh_sun_axis_lane_ok(khsf_now) &&
               (g_sraw_last_sample_ms == 0 || khsf_now - g_sraw_last_sample_ms >= KH_SRAW_SKY_FALLBACK_MS)) {
        khsf_m[0] = g_skysun_ref[0]; khsf_m[1] = g_skysun_ref[1]; khsf_m[2] = g_skysun_ref[2];
        khsf_src = 2;   // A lane above the floor. KH_SRAW_FLOOR_HOLD: at or below it (the sun ~5.7 deg up or lower)
                        // or with no fresh lane, the standing publish stays - no clock - until a cascade sample or a
                        // lane above the floor.
    }
    if (khsf_src == 0) return;   // Nothing this cycle: the standing publish stays.
    if (khsf_src == 1) {
        // KH_SRAW_SETTLE (see the statics). The ring takes world closes only;
        // a refused close's sample is a foreign pass's axis.
        bool khsf_settled = false;
        if (khsf_world) {
            g_sraw_ring[g_sraw_ring_i][0] = khsf_m[0]; g_sraw_ring[g_sraw_ring_i][1] = khsf_m[1]; g_sraw_ring[g_sraw_ring_i][2] = khsf_m[2];
            g_sraw_ring_i = (g_sraw_ring_i + 1) % KH_SRAW_RING;
            if (g_sraw_ring_n < KH_SRAW_RING) ++g_sraw_ring_n;
        }
        if (g_sraw_ring_n >= KH_SRAW_RING) {
            float khsf_rx[KH_SRAW_RING], khsf_ry[KH_SRAW_RING], khsf_rz[KH_SRAW_RING];
            for (int khsf_i = 0; khsf_i < KH_SRAW_RING; ++khsf_i) {
                khsf_rx[khsf_i] = g_sraw_ring[khsf_i][0]; khsf_ry[khsf_i] = g_sraw_ring[khsf_i][1]; khsf_rz[khsf_i] = g_sraw_ring[khsf_i][2];
            }
            float khsf_md[3] = { kh_rc_median(khsf_rx, KH_SRAW_RING), kh_rc_median(khsf_ry, KH_SRAW_RING), kh_rc_median(khsf_rz, KH_SRAW_RING) };
            const float khsf_ml = sqrtf(khsf_md[0] * khsf_md[0] + khsf_md[1] * khsf_md[1] + khsf_md[2] * khsf_md[2]);
            if (khsf_ml > 1.0e-6f) {
                khsf_md[0] /= khsf_ml; khsf_md[1] /= khsf_ml; khsf_md[2] /= khsf_ml;
                if (khsf_md[1] < 0.0f) { khsf_md[0] = -khsf_md[0]; khsf_md[1] = -khsf_md[1]; khsf_md[2] = -khsf_md[2]; }
                float khsf_step_deg = 0.0f;
                if (g_sraw_med_prev_valid) khsf_step_deg = kh_sraw_angle_deg(khsf_md, g_sraw_med_prev);
                g_sraw_med_prev[0] = khsf_md[0]; g_sraw_med_prev[1] = khsf_md[1]; g_sraw_med_prev[2] = khsf_md[2];
                g_sraw_med_prev_valid = true;
                const float khsf_sy = khsf_md[1];   // sin(elevation): skyward and unit.
                float khsf_bar = 0.02f * khsf_sy * khsf_sy * 57.29578f;
                if (khsf_bar < KH_SRAW_SETTLE_MIN_DEG) khsf_bar = KH_SRAW_SETTLE_MIN_DEG;
                if (khsf_bar > KH_SRAW_SETTLE_MAX_DEG) khsf_bar = KH_SRAW_SETTLE_MAX_DEG;
                if (khsf_step_deg > khsf_bar) g_sraw_cons_moved_ms = khsf_now;
                khsf_settled = g_sraw_cons_moved_ms == 0 || khsf_now - g_sraw_cons_moved_ms >= KH_SRAW_SETTLE_MS;
                khsf_m[0] = khsf_md[0]; khsf_m[1] = khsf_md[1]; khsf_m[2] = khsf_md[2];   // The median is the consensus from here.
            }
        }
        if (khsf_settled) {
            g_sraw_unsettled_ms = 0;
        } else if (g_sraw_unsettled_ms == 0) {
            g_sraw_unsettled_ms = khsf_now;   // The cap's clock: opened at the first unsettled close.
        }
        const bool khsf_capped = g_sraw_unsettled_ms != 0 && khsf_now - g_sraw_unsettled_ms >= KH_SRAW_HOLD_CAP_MS;
        bool khsf_lane_moved = false;   // The sun moved (the lane stepped): the median publishes unheld.
        // The fresh sky lane: its own step since the previous close (the witness) and the consensus's gap to it.
        g_sraw_sky_gap_deg = -1.0f;
        if (kh_sun_axis_lane_fresh(khsf_now)) {
            const float khsf_s[3] = { g_skysun_ref[0], g_skysun_ref[1], g_skysun_ref[2] };   // Raw: no flip, as stored.
            const bool  khsf_s_ok = khsf_s[1] > KH_SUN_AXIS_LANE_MIN_Y;
            g_sraw_sky_gap_deg = kh_sraw_angle_deg(khsf_m, khsf_s);   // At night: the moon against the set sun (> 90 deg).
            g_sraw_sky_step_deg = g_sraw_sky_prev_valid ? kh_sraw_angle_deg(khsf_s, g_sraw_sky_prev) : 0.0f;
            g_sraw_sky_prev[0] = khsf_s[0]; g_sraw_sky_prev[1] = khsf_s[1]; g_sraw_sky_prev[2] = khsf_s[2];
            g_sraw_sky_prev_valid = true;
            khsf_lane_moved = khsf_s_ok && g_sraw_sky_step_deg > KH_SRAW_LANE_STILL_DEG;
            // KH_SRAW_JUMP_HOLD: the lane jumped - the sun moved - the lane stands in for the consensus until the
            // median settles on the new sun; the ring restarts so the old sun never blends into it.
            if (khsf_s_ok && g_sraw_sky_step_deg > KH_SRAW_JUMP_DEG) {
                g_sraw_hold_until_ms = khsf_now + KH_SRAW_HOLD_CAP_MS;
                g_sraw_cons_moved_ms = khsf_now;
                g_sraw_unsettled_ms = khsf_now;
                khsf_settled = false;
                khsf_lane_moved = false;
                kh_sraw_ring_reset();
            }
            // The witnessed lane is the axis, fold-free.
            if (g_sraw_sky_gap_deg <= KH_SRAW_LANE_WIT_DEG && khsf_s_ok) {
                khsf_m[0] = khsf_s[0]; khsf_m[1] = khsf_s[1]; khsf_m[2] = khsf_s[2];
                khsf_src = 3;
                g_sraw_hold_until_ms = 0;   // The consensus reached the lane: nothing left to hold against.
            } else if (khsf_s_ok && g_sraw_hold_until_ms != 0 && !khsf_settled && khsf_now < g_sraw_hold_until_ms) {
                khsf_m[0] = khsf_s[0]; khsf_m[1] = khsf_s[1]; khsf_m[2] = khsf_s[2];
                khsf_src = 4;   // The lane stands through the convergence.
            } else if (khsf_s_ok &&
                       (g_sraw_last_src == 3 || g_sraw_last_src == 4) &&
                       g_sraw_sky_step_deg >= 0.0f && g_sraw_sky_step_deg <= KH_SRAW_LANE_STILL_DEG &&
                       !khsf_settled) {
                // Standing-lane hold: the lane stood still since the previous close that named it (g_sraw_last_src is
                // stored before the world test, so a refused close counts); the consensus left it, the sun did not (Src
                // 4). A settled median that left the lane is the engine's own axis and publishes.
                khsf_m[0] = khsf_s[0]; khsf_m[1] = khsf_s[1]; khsf_m[2] = khsf_s[2];
                khsf_src = 4;
            } else {
                g_sraw_hold_until_ms = 0;
            }
        } else {
            g_sraw_sky_step_deg = -1.0f;
            g_sraw_sky_prev_valid = false;
            g_sraw_hold_until_ms = 0;   // No lane to stand in (night): the last publish holds instead.
        }
        // The consensus publishes settled, or witnessed moving, or capped, or
        // when nothing stands yet; unsettled it holds the standing publish.
        if (khsf_src == 1 && !khsf_settled && !khsf_lane_moved && !khsf_capped && g_pub_valid) {
            g_sraw_last_src = khsf_src;
            g_sraw_pub_gap_deg = kh_sraw_angle_deg(khsf_m, g_pub_dir);
            return;   // Held: the standing publish stays.
        }
    }
    g_sraw_last_src = khsf_src;
    g_sraw_pub_gap_deg = g_pub_valid ? kh_sraw_angle_deg(khsf_m, g_pub_dir) : -1.0f;
    if (khsf_src != 2 && !khsf_world) {   // Refused: the consensus stands off the publish until a world cycle lands.
        return;
    }
    // Publish. The step against the standing publish is the jump test.
    const float khsf_step = g_pub_valid ? g_sraw_pub_gap_deg : 0.0f;
    g_pub_dir[0] = khsf_m[0]; g_pub_dir[1] = khsf_m[1]; g_pub_dir[2] = khsf_m[2];
    g_sun_dir_engine[0] = khsf_m[0]; g_sun_dir_engine[1] = khsf_m[1]; g_sun_dir_engine[2] = khsf_m[2];
    g_pub_valid = true;
    g_sun_valid = true;
    if (khsf_step > KH_SRAW_JUMP_DEG) {
        g_sun_jump_pending = true;   // The clear hook's jump block consumes it right after this call.
    }
}
// KH_SUN_RAW end.

// The engine does not state the moon direction in the sky block (every float triple of the sky upload was tested
// against the moon consensus; no lane). At night the cascade consensus IS the axis, departures included.

inline const float* kh_shadow_sun() {
    if (g_sun_valid) return g_sun_dir_engine;
    return nullptr;
}

// A fresh sky lane above the horizon. It carries nothing on its own: its three callers pair it with kh_shadow_sun(),
// which is null exactly while !g_sun_valid (KH_SUN_AXIS_FOLD: the maps render from the publish itself) -
// render_sun_depth and the mask cast's readiness test `!g_sun_valid && !kh_sun_axis_witnessed()` and then return on
// `!kh_shadow_sun()`; kh_skin_cull_pass takes the sun only when `(g_sun_valid || kh_sun_axis_witnessed()) &&
// kh_shadow_sun() != nullptr`. So before the first publish neither the maps nor the cast run; the sky lane reaches
// them only through a publish (kh_sun_raw_frame's sky-lane sources). In the first two the witness only picks which of
// the two returns is taken.
inline bool kh_sun_axis_witnessed() {
    return g_skysun_ref_valid && kh_sun_axis_lane_ok(steady_now_ms());
}

// Caller supplies the axis already rotated into world space; fail-open wherever the comparison is not trustworthy
// (no published sun). The bar is KH_SUN_AXIS_MAX_DEG.
inline bool kh_sun_axis_foreign(float khsx_x, float khsx_y, float khsx_z, float* khsx_out_deg) {
    const float* khsx_ps = kh_shadow_sun();
    if (!khsx_ps) return false;   // Nothing published yet: nothing to compare against.

    const float khsx_n = sqrtf(khsx_x * khsx_x + khsx_y * khsx_y + khsx_z * khsx_z);
    if (khsx_n < 1e-6f) return false;
    khsx_x /= khsx_n; khsx_y /= khsx_n; khsx_z /= khsx_n;
    if (khsx_y < 0.0f) { khsx_x = -khsx_x; khsx_y = -khsx_y; khsx_z = -khsx_z; }   // Skyward, like the derivation.
    float khsx_d = khsx_x * khsx_ps[0] + khsx_y * khsx_ps[1] + khsx_z * khsx_ps[2];
    khsx_d = khsx_d > 1.0f ? 1.0f : (khsx_d < -1.0f ? -1.0f : khsx_d);
    const float khsx_deg = acosf(khsx_d) * 57.29578f;
    if (khsx_out_deg) *khsx_out_deg = khsx_deg;
    if (khsx_deg > KH_SUN_AXIS_MAX_DEG) {
        return true;
    }

    return false;
}

// Once per scene flush: the direction is already published (kh_sun_raw_frame at the clear); the flush carries the fog
// and mirrors validity. Do not add a maturity, hold, cooldown or glide: each was pure shadow lag.
inline void publish_world_lighting() {
    g_sun_valid = g_pub_valid;
    // KH_NO_PARK: the render-thread flush publishes unparked, so the staged
    // copy is read under the mutex stage_world_lighting writes it under.
    std::lock_guard<std::mutex> khpw_l(g_fog_stage_mu);
    g_fog_valid = g_fog_staged_valid;
    g_fog[0] = g_fog_staged[0];
    g_fog[1] = g_fog_staged[1];
    g_fog[2] = g_fog_staged[2];
}

inline bool kh_probe_world_cycle() {
    return !g_ro.cycle_vp_valid ||
           (g_ro.cycle_vp_min <= 0.3f && g_ro.cycle_vp_max >= 0.7f);
}

// 15% luminance band with an absolute 0.02 floor: KH_LIGHT_BASE's cluster test, its admission hold and
// KH_LIT_EVIDENCE's band test (night-small values must not ratio-flap into separate clusters).
inline bool kh_probe_lum_band(float a, float b) {
    const float khp_m = a > b ? a : b;
    const float khp_eps = 0.15f * khp_m > 0.02f ? 0.15f * khp_m : 0.02f;
    return fabsf(a - b) <= khp_eps;
}

// The block mirror (nb): every admitted non-zero standard-layout upload of a world cycle (kh_probe_world_cycle)
// refreshes it in full. Its readers are the fog / haze lanes (40+); amb / sun are published by KH_LIGHT_BASE from the
// uploads themselves. An all-zero block (amb, sun, fog) is a clear, not lighting, and is refused.
inline void kh_probe_mirror_refresh(CbColorProbe& khp, const float* f, uint32_t nf) {
    if (khp.off >= 40 && khp.off - 40 + 38 < nf) {
        const float* khpz_b = f + (khp.off - 40);
        const float khpz_al = khpz_b[8] + khpz_b[9] + khpz_b[10];
        const float khpz_sl = khpz_b[16] + khpz_b[17] + khpz_b[18];
        const float khpz_fl = khpz_b[36] + khpz_b[37] + khpz_b[38];
        if (khpz_al < 1.0e-6f && khpz_sl < 1.0e-6f && khpz_fl < 1.0e-6f) return;
    }
    locator_capture(khp, f, nf, 56);
    khp.meta = static_cast<int>(khp.off - khp.nb_base);
    khp.mirror_confirm = khp.last_confirm;   // KH_DEC_MIRROR_CLOCK: both callers stamp last_confirm first.
}
}