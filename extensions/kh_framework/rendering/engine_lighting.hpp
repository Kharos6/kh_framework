#pragma once

// The injection half (rendering_integration_injection.hpp): the latched engine lighting: the fog staging, the sky sun,
// the world-lighting owner, the raw sun axis and the probes. Never include it on its own.
namespace RenderIntegration {
// Defined later in this file (declared here for the LIGHT_BASE owner, which uses them first).
inline bool probe_lum_band(float a, float b);
inline bool probe_world_cycle();
// Defined in rendering/shadow_locators.hpp (declared here for the uses below).
inline void locator_capture(CbColorProbe& pr, const float* f, uint32_t nf, uint32_t win);

inline bool sun_axis_lane_fresh(uint64_t now) {
    return g_skysun.ref_valid && (now - g_skysun.ref_ms) < SUN_AXIS_SKY_FRESH_MS;
}
inline bool sun_axis_lane_ok(uint64_t now) {
    return sun_axis_lane_fresh(now) && g_skysun.ref[1] > SUN_AXIS_LANE_MIN_Y;
}
// Sky-static predicate for the witness consumers: fresh mirror (the lane's own freshness), an established motion
// window, and sub-0.3 deg sky movement across it.
inline bool sky_static() {
    return sun_axis_lane_fresh(steady_now_ms()) &&
           g_skysun.mot_deg >= 0.0f && g_skysun.mot_deg < 0.3f;
}

inline void blk_owner_close_span() {
    if (!g_blko.open) return;
    g_blko.open = false;
    if (g_blko.open_i < 0 || g_blko.open_i >= g_blko.n) return;
    const uint64_t d64 = g_blko.draws >= g_blko.open_draws ? g_blko.draws - g_blko.open_draws : 0;
    const uint32_t d = static_cast<uint32_t>(d64 > 0xFFFFFFFFull ? 0xFFFFFFFFull : d64);
    BlkOwn& e = g_blko.cur[g_blko.open_i];
    e.draws += d;
    if (d > e.rep_draws || e.members == 1) {   // The member that owned the most itself represents.
        e.rep_draws = d;
        memcpy(e.amb, g_blko.open_amb, sizeof(e.amb));
        memcpy(e.sun, g_blko.open_sun, sizeof(e.sun));
    }
}
// Every admitted upload: the previous span closes, this upload joins its
// cluster (or opens one) and owns from here. State only.
inline void blk_owner_note(const float* b, uint8_t flag) {
    blk_owner_close_span();
    const float al = b[8] + b[9] + b[10];
    const float sl = b[16] + b[17] + b[18];
    int i = -1;
    for (int k = 0; k < g_blko.n; ++k) {
        if (((g_blko.cur[k].flag ^ flag) & 2u) != 0u) continue;   // Zero never merges with lit.
        if (probe_lum_band(sl, g_blko.cur[k].sl) &&
            probe_lum_band(al, g_blko.cur[k].al)) { i = k; break; }
    }
    if (i < 0) {
        if (g_blko.n >= BLKO_MAX) { g_blko.open_i = -1; return; }
        i = g_blko.n++;
        BlkOwn& e = g_blko.cur[i];
        memset(&e, 0, sizeof(e));
        e.flag = flag;
        e.al = al; e.sl = sl;   // The cluster's band is anchored on its first member.
    }
    BlkOwn& c = g_blko.cur[i];
    ++c.members;
    c.flag |= flag;
    for (int k = 0; k < 3; ++k) { g_blko.open_amb[k] = b[8 + k];
        g_blko.open_sun[k] = b[16 + k]; }
    g_blko.open_i = i;
    g_blko.open_draws = g_blko.draws;
    g_blko.open = true;
}
// One admitted upload of the locked block (b = the block base: lanes 8-10
// ambient, 16-18 sun, 36-38 fog; avail = floats readable from it).
inline void blk_note_admitted(const float* b, uint32_t avail, bool world) {
    const float al = b[8] + b[9] + b[10];
    const float sl = b[16] + b[17] + b[18];
    const float fl = avail > 38 ? b[36] + b[37] + b[38] : 1.0f;
    // BLK_FINITE: an ambient or sun sum that is NaN or beyond +-1e30 (inf; no colour) is flagged with the clears:
    // never a base (once picked, no '>' could displace a NaN cluster, and the frame fill copies the pick raw into
    // lighting2 / lightAmb). Sums within +-1e30: unchanged.
    const bool zero = (al < 1.0e-6f && sl < 1.0e-6f && fl < 1.0e-6f) ||
                      !(fabsf(al) <= 1.0e30f && fabsf(sl) <= 1.0e30f);
    const uint8_t flag = static_cast<uint8_t>((world ? 1u : 0u) | (zero ? 2u : 0u));
    blk_owner_note(b, flag);
}
inline void blk_owner_frame() {
    blk_owner_close_span();
    g_blko.open_i = -1;
    if (g_blko.n == 0) return;
    uint32_t owned = 0;
    uint32_t memb = 0;   // BLKO_MEMBER: lit members, both shares' denominator.
    int of_max = -1; uint32_t max_d = 0;   // The draw-count winner: the fallback when no cluster clears the bar.
    for (int i = 0; i < g_blko.n; ++i) {
        if (g_blko.cur[i].flag & 2u) continue;
        owned += g_blko.cur[i].draws;
        memb += g_blko.cur[i].members;
        if (of_max < 0 || g_blko.cur[i].draws > max_d) { of_max = i;
            max_d = g_blko.cur[i].draws; }
    }
    int pick = -1;
    if (owned > 0) {
        const float bar_m = BLKO_BASE_SHARE * static_cast<float>(memb);
        for (int i = 0; i < g_blko.n; ++i) {
            const BlkOwn& e = g_blko.cur[i];
            if (e.flag & 2u) continue;   // All-zero is a clear, never a base.
            if (static_cast<float>(e.members) < bar_m) continue;
            if (pick < 0) { pick = i; continue; }
            const BlkOwn& p = g_blko.cur[pick];
            const float hi = e.sl > p.sl ? e.sl : p.sl;
            if (fabsf(e.sl - p.sl) <= 0.01f * hi + 1.0e-3f) {   // The same sun (an ambient-only variant): the one that draws more is the lighting, not the lift.
                if (e.draws > p.draws || (e.draws == p.draws && e.sl > p.sl)) pick = i;
            } else if (e.sl > p.sl) {
                pick = i;
            }
        }
        if (g_pub_block_valid) {   // Admission hold: a standing base at least as bright stays down to BLKO_BASE_KEEP.
            // The first cluster in table order in band with the standing publish is the one judged; two clusters can
            // both be in band with it while distinct from each other.
            const float pal = g_pub_block_amb[0] + g_pub_block_amb[1] + g_pub_block_amb[2];
            const float psl = g_pub_block_sun[0] + g_pub_block_sun[1] + g_pub_block_sun[2];
            for (int i = 0; i < g_blko.n; ++i) {
                const BlkOwn& e = g_blko.cur[i];
                if (e.flag & 2u) continue;
                if (!probe_lum_band(psl, e.sl) || !probe_lum_band(pal, e.al)) continue;
                if (i != pick && static_cast<float>(e.members) >= BLKO_BASE_KEEP * static_cast<float>(memb) &&
                    (pick < 0 || e.sl >= g_blko.cur[pick].sl)) { pick = i; }
                break;
            }
        }
        if (pick < 0) pick = of_max;   // Nothing clears the member bar (many near-equal clusters): the largest owner.
    }
    const bool world = probe_world_cycle();
    // DLSW_GEN_AMB: the frame's brightest lit world clusters. A sparse frame (fewer lit draws owned than
    // LITE_CONF_DRAWS - the publish's own evidence bar, LIT_EVIDENCE) keeps the last value: one stray upload
    // cannot set the floor.
    if (world && owned >= LITE_CONF_DRAWS) {
        bool gany = false;
        float gamb[3] = { 0.0f, 0.0f, 0.0f }, gsun[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < g_blko.n; ++i) {
            const BlkOwn& e = g_blko.cur[i];
            if ((e.flag & 2u) || !(e.flag & 1u) || e.draws == 0) continue;
            for (int c = 0; c < 3; ++c) {
                if (e.amb[c] > gamb[c]) gamb[c] = e.amb[c];
                if (e.sun[c] > gsun[c]) gsun[c] = e.sun[c];
            }
            gany = true;
        }
        if (gany) {
            memcpy(g_blko.gen_amb, gamb, sizeof(g_blko.gen_amb));
            memcpy(g_blko.gen_sun, gsun, sizeof(g_blko.gen_sun));
            g_blko.gen_valid = true;
        }
    }
    if (pick >= 0 && world) {
        const BlkOwn& w = g_blko.cur[pick];
        bool admit = true;
        if (g_pub_block_valid && owned < LITE_CONF_DRAWS) {   // LIT_EVIDENCE.
            const float psl = g_pub_block_sun[0] + g_pub_block_sun[1] + g_pub_block_sun[2];
            const float pal = g_pub_block_amb[0] + g_pub_block_amb[1] + g_pub_block_amb[2];
            if (!(probe_lum_band(psl, w.sl) && probe_lum_band(pal, w.al))) {
                if (g_lite_cand.n > 0 && probe_lum_band(g_lite_cand.sl, w.sl) &&
                    probe_lum_band(g_lite_cand.al, w.al)) {
                    ++g_lite_cand.n;
                } else {
                    g_lite_cand.n = 1; g_lite_cand.sl = w.sl; g_lite_cand.al = w.al;
                }
                admit = g_lite_cand.n >= LITE_CONF_CLOSES;
            } else {
                g_lite_cand.n = 0;
            }
        } else {
            g_lite_cand.n = 0;
        }
        if (admit) {
            for (int c = 0; c < 3; ++c) { g_pub_block_amb[c] = w.amb[c]; g_pub_block_sun[c] = w.sun[c]; }
            g_pub_block_valid = true;
        }
    }
    g_blko.n = 0;
}
// SUN_RAW: the sun direction publish. The engine states its light axis twice per frame: the cascade sampling
// matrices' depth row (the direction it actually shadows with; the moon at night) and the sky CB's sun lane (world
// space, day only). band_capture samples the cascade axis of every capture; at the main depth clear sun_raw_frame
// takes the cycle's median (world cycles only) and publishes: the SKY LANE when fresh and within SRAW_LANE_WIT_DEG
// of the consensus (it lands the frame the sun moves; the cascade axis converges over ~0.7 s and departs during camera
// moves); the STANDING LANE (Src 4) when the lane did not step since the previous close that named it (<=
// SRAW_LANE_STILL_DEG) but the consensus left it (on a flick or camera switch the cascade axis leaves an unmoved
// lane by tens of degrees for 1-3 frames); the CASCADE CONSENSUS otherwise (night); the sky lane alone after
// SRAW_SKY_FALLBACK_MS with no cascade (shadows off). A step past SRAW_JUMP_DEG resets the union's map hash.
// Nothing here blends: the consensus published is its median over a ring of closes (before the ring fills, the close's
// own median), and only while that median is still or the lane witnesses the sun moving (SRAW_SETTLE), when nothing
// stands yet (the first publish), or once an unsettled run reaches SRAW_HOLD_CAP_MS; after a lane jump the lane
// stands until it settles. Otherwise the engine's own convergence, its rotation departures and its per-close spikes
// never replace a standing publish.
inline float rc_median(float* v, int n) {   // Insertion sort: n <= SRAW_RING.
    for (int i = 1; i < n; ++i) {
        const float x = v[i];
        int j = i - 1;
        while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; --j; }
        v[j + 1] = x;
    }
    return v[n / 2];
}
inline void sraw_ring_reset() { g_sraw.ring_n = 0; g_sraw.ring_i = 0; g_sraw.med_prev_valid = false; }

inline float sraw_angle_deg(const float* a, const float* b) {
    float d = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    d = d > 1.0f ? 1.0f : (d < -1.0f ? -1.0f : d);
    return acosf(d) * 57.29578f;
}
// One raw cascade axis (world, already skyward and unit) from the capture site.
inline void sun_raw_note(const float* w) {
    g_sraw.last_sample_ms = steady_now_ms();
    if (g_sraw.cyc_n >= SRAW_CYC) return;
    g_sraw.cyc[g_sraw.cyc_n][0] = w[0];
    g_sraw.cyc[g_sraw.cyc_n][1] = w[1];
    g_sraw.cyc[g_sraw.cyc_n][2] = w[2];
    ++g_sraw.cyc_n;
}
// At the main depth clear (render thread), before g_ro.cycle_vp_* resets (the ended cycle's viewport is the world test)
// and before the sun-jump block consumes g_sun_jump_pending.
inline void sun_raw_frame() {
    const uint64_t now = steady_now_ms();
    const bool world = probe_world_cycle();
    float m[3] = { 0.0f, 0.0f, 0.0f };
    uint8_t src = 0;
    const int n = g_sraw.cyc_n;
    g_sraw.cyc_n = 0;
    if (n > 0) {
        float x[SRAW_CYC], y[SRAW_CYC], z[SRAW_CYC];
        for (int i = 0; i < n; ++i) {
            x[i] = g_sraw.cyc[i][0];
            y[i] = g_sraw.cyc[i][1];
            z[i] = g_sraw.cyc[i][2];
        }
        m[0] = rc_median(x, n);
        m[1] = rc_median(y, n);
        m[2] = rc_median(z, n);
        const float l = sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
        if (l > 1.0e-6f) {
            m[0] /= l; m[1] /= l; m[2] /= l;
            if (m[1] < 0.0f) { m[0] = -m[0]; m[1] = -m[1]; m[2] = -m[2]; }
            src = 1;
        }
    } else if (sun_axis_lane_ok(now) &&
               (g_sraw.last_sample_ms == 0 || now - g_sraw.last_sample_ms >= SRAW_SKY_FALLBACK_MS)) {
        m[0] = g_skysun.ref[0]; m[1] = g_skysun.ref[1]; m[2] = g_skysun.ref[2];
        src = 2;   // A lane above the floor. SRAW_FLOOR_HOLD: at or below it (the sun ~5.7 deg up or lower)
                        // or with no fresh lane, the standing publish stays - no clock - until a cascade sample or a
                        // lane above the floor.
    }
    if (src == 0) return;   // Nothing this cycle: the standing publish stays.
    if (src == 1) {
        // SRAW_SETTLE (see the statics). The ring takes world closes only;
        // a refused close's sample is a foreign pass's axis.
        bool settled = false;
        if (world) {
            g_sraw.ring[g_sraw.ring_i][0] = m[0]; g_sraw.ring[g_sraw.ring_i][1] = m[1];
            g_sraw.ring[g_sraw.ring_i][2] = m[2];
            g_sraw.ring_i = (g_sraw.ring_i + 1) % SRAW_RING;
            if (g_sraw.ring_n < SRAW_RING) ++g_sraw.ring_n;
        }
        if (g_sraw.ring_n >= SRAW_RING) {
            float rx[SRAW_RING], ry[SRAW_RING], rz[SRAW_RING];
            for (int i = 0; i < SRAW_RING; ++i) {
                rx[i] = g_sraw.ring[i][0]; ry[i] = g_sraw.ring[i][1];
                rz[i] = g_sraw.ring[i][2];
            }
            float md[3] = { rc_median(rx, SRAW_RING), rc_median(ry, SRAW_RING), rc_median(rz, SRAW_RING) };
            const float ml = sqrtf(md[0] * md[0] + md[1] * md[1] + md[2] * md[2]);
            if (ml > 1.0e-6f) {
                md[0] /= ml; md[1] /= ml; md[2] /= ml;
                if (md[1] < 0.0f) { md[0] = -md[0]; md[1] = -md[1]; md[2] = -md[2]; }
                float step_deg = 0.0f;
                if (g_sraw.med_prev_valid) step_deg = sraw_angle_deg(md, g_sraw.med_prev);
                g_sraw.med_prev[0] = md[0]; g_sraw.med_prev[1] = md[1]; g_sraw.med_prev[2] = md[2];
                g_sraw.med_prev_valid = true;
                const float sy = md[1];   // sin(elevation): skyward and unit.
                float bar = 0.02f * sy * sy * 57.29578f;
                if (bar < SRAW_SETTLE_MIN_DEG) bar = SRAW_SETTLE_MIN_DEG;
                if (bar > SRAW_SETTLE_MAX_DEG) bar = SRAW_SETTLE_MAX_DEG;
                if (step_deg > bar) g_sraw.cons_moved_ms = now;
                settled = g_sraw.cons_moved_ms == 0 || now - g_sraw.cons_moved_ms >= SRAW_SETTLE_MS;
                m[0] = md[0]; m[1] = md[1]; m[2] = md[2];   // The median is the consensus from here.
            }
        }
        if (settled) {
            g_sraw.unsettled_ms = 0;
        } else if (g_sraw.unsettled_ms == 0) {
            g_sraw.unsettled_ms = now;   // The cap's clock: opened at the first unsettled close.
        }
        const bool capped = g_sraw.unsettled_ms != 0 && now - g_sraw.unsettled_ms >= SRAW_HOLD_CAP_MS;
        bool lane_moved = false;   // The sun moved (the lane stepped): the median publishes unheld.
        // The fresh sky lane: its own step since the previous close (the witness) and the consensus's gap to it.
        g_sraw.sky_gap_deg = -1.0f;
        if (sun_axis_lane_fresh(now)) {
            const float s[3] = { g_skysun.ref[0], g_skysun.ref[1], g_skysun.ref[2] };   // Raw: no flip, as stored.
            const bool  s_ok = s[1] > SUN_AXIS_LANE_MIN_Y;
            // At night: the moon against the set sun (> 90 deg).
            g_sraw.sky_gap_deg = sraw_angle_deg(m, s);
            g_sraw.sky_step_deg = g_sraw.sky_prev_valid ? sraw_angle_deg(s, g_sraw.sky_prev) : 0.0f;
            g_sraw.sky_prev[0] = s[0]; g_sraw.sky_prev[1] = s[1]; g_sraw.sky_prev[2] = s[2];
            g_sraw.sky_prev_valid = true;
            lane_moved = s_ok && g_sraw.sky_step_deg > SRAW_LANE_STILL_DEG;
            // SRAW_JUMP_HOLD: the lane jumped - the sun moved - the lane stands in for the consensus until the
            // median settles on the new sun; the ring restarts so the old sun never blends into it.
            if (s_ok && g_sraw.sky_step_deg > SRAW_JUMP_DEG) {
                g_sraw.hold_until_ms = now + SRAW_HOLD_CAP_MS;
                g_sraw.cons_moved_ms = now;
                g_sraw.unsettled_ms = now;
                settled = false;
                lane_moved = false;
                sraw_ring_reset();
            }
            // The witnessed lane is the axis, fold-free.
            if (g_sraw.sky_gap_deg <= SRAW_LANE_WIT_DEG && s_ok) {
                m[0] = s[0]; m[1] = s[1]; m[2] = s[2];
                src = 3;
                g_sraw.hold_until_ms = 0;   // The consensus reached the lane: nothing left to hold against.
            } else if (s_ok && g_sraw.hold_until_ms != 0 && !settled && now < g_sraw.hold_until_ms) {
                m[0] = s[0]; m[1] = s[1]; m[2] = s[2];
                src = 4;   // The lane stands through the convergence.
            } else if (s_ok &&
                       (g_sraw.last_src == 3 || g_sraw.last_src == 4) &&
                       g_sraw.sky_step_deg >= 0.0f && g_sraw.sky_step_deg <= SRAW_LANE_STILL_DEG &&
                       !settled) {
                // Standing-lane hold: the lane stood still since the previous close that named it (g_sraw.last_src is
                // stored before the world test, so a refused close counts); the consensus left it, the sun did not (Src
                // 4). A settled median that left the lane is the engine's own axis and publishes.
                m[0] = s[0]; m[1] = s[1]; m[2] = s[2];
                src = 4;
            } else {
                g_sraw.hold_until_ms = 0;
            }
        } else {
            g_sraw.sky_step_deg = -1.0f;
            g_sraw.sky_prev_valid = false;
            g_sraw.hold_until_ms = 0;   // No lane to stand in (night): the last publish holds instead.
        }
        // The consensus publishes settled, or witnessed moving, or capped, or
        // when nothing stands yet; unsettled it holds the standing publish.
        if (src == 1 && !settled && !lane_moved && !capped && g_pub_valid) {
            g_sraw.last_src = src;
            g_sraw.pub_gap_deg = sraw_angle_deg(m, g_pub_dir);
            return;   // Held: the standing publish stays.
        }
    }
    g_sraw.last_src = src;
    g_sraw.pub_gap_deg = g_pub_valid ? sraw_angle_deg(m, g_pub_dir) : -1.0f;
    if (src != 2 && !world) {   // Refused: the consensus stands off the publish until a world cycle lands.
        return;
    }
    // Publish. The step against the standing publish is the jump test.
    const float step = g_pub_valid ? g_sraw.pub_gap_deg : 0.0f;
    g_pub_dir[0] = m[0]; g_pub_dir[1] = m[1]; g_pub_dir[2] = m[2];
    g_sun_dir_engine[0] = m[0]; g_sun_dir_engine[1] = m[1]; g_sun_dir_engine[2] = m[2];
    g_pub_valid = true;
    g_sun_valid = true;
    if (step > SRAW_JUMP_DEG) {
        g_sun_jump_pending = true;   // The clear hook's jump block consumes it right after this call.
    }
}
// SUN_RAW end.

// The engine does not state the moon direction in the sky block (every float triple of the sky upload was tested
// against the moon consensus; no lane). At night the cascade consensus IS the axis, departures included.

inline const float* shadow_sun() {
    if (g_sun_valid) return g_sun_dir_engine;
    return nullptr;
}

// A fresh sky lane above the horizon. It carries nothing on its own: its three callers pair it with shadow_sun(),
// which is null exactly while !g_sun_valid (SUN_AXIS_FOLD: the maps render from the publish itself) -
// render_sun_depth and the mask cast's readiness test `!g_sun_valid && !sun_axis_witnessed()` and then return on
// `!shadow_sun()`; skin_cull_pass takes the sun only when `(g_sun_valid || sun_axis_witnessed()) &&
// shadow_sun() != nullptr`. So before the first publish neither the maps nor the cast run; the sky lane reaches
// them only through a publish (sun_raw_frame's sky-lane sources). In the first two the witness only picks which of
// the two returns is taken.
inline bool sun_axis_witnessed() {
    return sun_axis_lane_ok(steady_now_ms());   // The lane's freshness test includes g_skysun.ref_valid.
}

// Caller supplies the axis already rotated into world space; fail-open wherever the comparison is not trustworthy
// (no published sun). The bar is SUN_AXIS_MAX_DEG.
inline bool sun_axis_foreign(float x, float y, float z, float* out_deg) {
    const float* ps = shadow_sun();
    if (!ps) return false;   // Nothing published yet: nothing to compare against.

    const float n = sqrtf(x * x + y * y + z * z);
    if (n < 1e-6f) return false;
    x /= n; y /= n; z /= n;
    if (y < 0.0f) { x = -x; y = -y; z = -z; }   // Skyward, like the derivation.
    float d = x * ps[0] + y * ps[1] + z * ps[2];
    d = d > 1.0f ? 1.0f : (d < -1.0f ? -1.0f : d);
    const float deg = acosf(d) * 57.29578f;
    if (out_deg) *out_deg = deg;
    if (deg > SUN_AXIS_MAX_DEG) {
        return true;
    }

    return false;
}

// Once per scene flush: the direction is already published (sun_raw_frame at the clear); the flush carries the fog
// and mirrors validity. Do not add a maturity, hold, cooldown or glide: each was pure shadow lag.
inline void publish_world_lighting() {
    g_sun_valid = g_pub_valid;
    // NO_PARK: the render-thread flush publishes unparked, so the staged
    // copy is read under the mutex stage_world_lighting writes it under.
    std::lock_guard<std::mutex> l(g_fog_stage_mu);
    g_fog_valid = g_fog_staged_valid;
    g_fog[0] = g_fog_staged[0];
    g_fog[1] = g_fog_staged[1];
    g_fog[2] = g_fog_staged[2];
}

inline bool probe_world_cycle() {
    return !g_ro.cycle_vp_valid ||
           (g_ro.cycle_vp_min <= 0.3f && g_ro.cycle_vp_max >= 0.7f);
}

// 15% luminance band with an absolute 0.02 floor: LIGHT_BASE's cluster test, its admission hold and
// LIT_EVIDENCE's band test (night-small values must not ratio-flap into separate clusters).
inline bool probe_lum_band(float a, float b) {
    const float m = a > b ? a : b;
    const float eps = 0.15f * m > 0.02f ? 0.15f * m : 0.02f;
    return fabsf(a - b) <= eps;
}

// The block mirror (nb): every admitted non-zero standard-layout upload of a world cycle (probe_world_cycle)
// refreshes it in full. Its readers are the fog / haze lanes (40+); amb / sun are published by LIGHT_BASE from the
// uploads themselves. An all-zero block (amb, sun, fog) is a clear, not lighting, and is refused.
inline void probe_mirror_refresh(CbColorProbe& p, const float* f, uint32_t nf) {
    if (p.off >= 40 && p.off - 40 + 38 < nf) {
        const float* b = f + (p.off - 40);
        const float al = b[8] + b[9] + b[10];
        const float sl = b[16] + b[17] + b[18];
        const float fl = b[36] + b[37] + b[38];
        if (al < 1.0e-6f && sl < 1.0e-6f && fl < 1.0e-6f) return;
    }
    locator_capture(p, f, nf, 56);
    p.meta = static_cast<int>(p.off - p.nb_base);
    p.mirror_confirm = p.last_confirm;   // DEC_MIRROR_CLOCK: both callers stamp last_confirm first.
}
}