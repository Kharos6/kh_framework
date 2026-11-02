#pragma once

// The feature half (rendering_integration_features.hpp): KH_DLS, the dynamic-light shadows' selection: slots, reach,
// faces and views. Never include it on its own.
namespace RenderIntegration {

// KH_DL_SHADOW stage 1 - which lights cast. A slot holds its light by the pool light's id (KH_DL_POOL_ID), by position
// only for a light that left the pool and came back under a new id - never by pool index (the pool is TTL-swept and
// compacted). The lights rank by their brightness as it reaches the camera (kh_dls_score: luminance attenuated over the
// camera distance), so the casting set is the lights whose shadows are on screen; the distance to the nearest caster is
// the admission and hold gate (kh_dls_select), and ranks only while no view is published (kh_dls_camera).
static constexpr uint32_t KH_DLS_MAX = 8;             // shadow-casting lights (the cap).
static constexpr float    KH_DLS_MATCH_M = 1.0f;      // same-light radius; mirrors KH_DL_MATCH_M.
static constexpr float    KH_DLS_OUST_FRAC = 1.25f;   // A challenger must beat the sitting light by this.

struct KhDlsSlot {
    float    pos[3] = {};      // Engine axes - the slot's identity when no pool light carries its light_id.
    uint32_t light_id = 0;     // KH_DL_POOL_ID: the pool entry this slot was admitted for
                               // (DlPoolLight::id); 0 = unknown.
    float    score = 0.0f;     // kh_dls_score it holds the slot on (luminance attenuated over the camera distance).
    float    reach = 0.0f;     // kh_dls_reach (m; hard fade, attenuation floor, ceiling); stage 2 sizes the map.
    uint8_t  spot = 0;         // 0 = point (omni), 1 = spot (single frustum).
    uint8_t  live = 0;
};

static KhDlsSlot g_dls[KH_DLS_MAX];
static uint64_t  g_dls_frame = 0;       // once-per-frame guard (twin call sites).
static uint64_t  g_dls_sel_ms = 0;      // KH_DLS_POOL_AGE: steady ms of kh_dls_select's last pass (0 = none).

// The slot table (g_dls) is behaviour state, not reset with the stats: clearing it would re-admit from scratch at every
// reset.

// rec.709 luminance of the diffuse lobe plus the per-light ambient - what the light spends at source (the ambient lane
// is what makes A3 lights read on away-facing surfaces). What it delivers is below.
inline float kh_dls_lum(const DlPoolLight& khs_l) {
    const float khs_d = 0.2126f * khs_l.rec[8]  + 0.7152f * khs_l.rec[9]  + 0.0722f * khs_l.rec[10];
    const float khs_a = 0.2126f * khs_l.rec[12] + 0.7152f * khs_l.rec[13] + 0.0722f * khs_l.rec[14];
    return khs_d + khs_a;
}

// DynLights' distance attenuation, verbatim (twin contract with the shader kernel): 1/(a0 + a1*d + a2*d^2) on the
// offset, scaled distance, then the hard range fade. TWIN (five copies; this one without the cone): DynLights,
// KhDynLightsPBRE and KhDlsWorldFactor (cb.hlsl), KhDlfAtt (effect.hlsl).
inline float kh_dls_atten(const DlPoolLight& kha_l, float kha_dist, float kha_scale) {
    const float kha_sd = kha_dist * kha_scale;
    const float kha_d  = kha_sd - kha_l.rec[16] > 0.0f ? kha_sd - kha_l.rec[16] : 0.0f;
    float kha_a = 1.0f / (kha_l.rec[17] + kha_l.rec[18] * kha_d
                        + kha_l.rec[19] * kha_d * kha_d + 1.0e-4f);
    if (kha_a > 1.0f) kha_a = 1.0f;
    if (kha_a < 0.0f) kha_a = 0.0f;
    float kha_f = (kha_sd - kha_l.rec[20]) * kha_l.rec[21];
    if (kha_f > 1.0f) kha_f = 1.0f;
    if (kha_f < 0.0f) kha_f = 0.0f;
    return kha_a * (1.0f - kha_f);
}

// The distance is camera -> light: ranking by caster distance can spend the whole budget on lights behind the player.
inline float kh_dls_score(const DlPoolLight& khs_l, float khs_dist, float khs_scale) {
    const float khs_lum = kh_dls_lum(khs_l);
    if (khs_dist < 0.0f) return khs_lum;   // No distance available: rank on source alone.
    return khs_lum * kh_dls_atten(khs_l, khs_dist, khs_scale);
}

// Camera on the render thread, engine axes. False when the engine has not published a view - kh_dls_select then ranks
// on the light-to-caster distance (source luminance for a light with no caster) rather than a zeroed camera (a camera
// at the origin would rank confidently wrong).
inline bool kh_dls_camera(float khc_out[3]) {
    if (!g_ro.cycle_pv_valid) return false;
    kh_cam_of(g_ro.cycle_pv.view, khc_out);
    return khc_out[0] == khc_out[0] && khc_out[1] == khc_out[1] && khc_out[2] == khc_out[2];
}

// The hard-fade term is rec[20] + 1/rec[21], and a light authored with no range fade leaves the inverse width at
// ~1e-20, so that returns 7e19 metres - and the far plane, the near plane and the face cull all break quietly. Bound it
// by what the light can actually light.
static constexpr float KH_DLS_REACH_MIN_M = 1.0f;   // A degenerate reach never yields a zero far
static constexpr float KH_DLS_REACH_MAX_M = 600.0f;
static constexpr float KH_DLS_ATTEN_FLOOR = 255.0f;   // 1/255 = one 8-bit level.

// Price the floor against what the light delivers: kh_dls_lum times the same two global lanes the shader multiplies by,
// falling below one level.
inline float kh_dls_reach_peak(const DlPoolLight& khp_l) {
    // The two global lanes DynLights applies (dlGlobal.xyz to each light's diffuse term, dlGlobal.w to the sum), both
    // priced here on the whole kh_dls_lum; rec.709 through kh_dls_lum so this cannot disagree with the score.
    const float khp_gd = (g_dl.main_ref_valid &&
                          (g_dl.main_gdiff[0] > 0.0f || g_dl.main_gdiff[1] > 0.0f ||
                           g_dl.main_gdiff[2] > 0.0f))
                       ? (0.2126f * g_dl.main_gdiff[0] + 0.7152f * g_dl.main_gdiff[1] +
                          0.0722f * g_dl.main_gdiff[2])
                       : 1.0f;
    const uint32_t khp_bits = g_dl_intensity_bits.load(std::memory_order_relaxed);
    float khp_int = 1.0f;
    memcpy(&khp_int, &khp_bits, sizeof(khp_int));
    if (!(khp_int > 0.0f) || khp_int != khp_int) khp_int = 1.0f;
    const float khp_p = kh_dls_lum(khp_l) * khp_gd * khp_int;
    // Never below 1: a dim light must not get a shorter reach than its far.
    return (khp_p > 1.0f && khp_p == khp_p) ? khp_p : 1.0f;
}

// The distance past which nothing else in the frame is casting either.
inline float kh_dls_reach_ceiling() {
    const float khc_s = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    return khc_s < KH_DLS_REACH_MAX_M ? khc_s : KH_DLS_REACH_MAX_M;
}

inline float kh_dls_reach(const DlPoolLight& khs_l) {
    const float khs_inv = khs_l.rec[21];
    const float khs_ceil = kh_dls_reach_ceiling();
    // The hard fade in metres: it runs on the scaled distance (kh_dls_atten), so its end divides by the global
    // distance scale - as the attenuation floor's solve below and kh_dl_select's reach do.
    const float khs_sc = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f) ? g_dl.main_scale : 1.0f;
    // KH_DL_NO_FADE: a light with no hard fade (inverse width ~1e-20) is bounded by the ceiling and the attenuation
    // floor below, as kh_dl_select takes it for every mesh - the fade start alone would cut its slot's reach, far plane
    // and casters where it still lights.
    float khs_r = khs_inv > 1.0e-6f ? (khs_l.rec[20] + 1.0f / khs_inv) / khs_sc : khs_ceil;
    if (!(khs_r > 0.0f) || khs_r != khs_r) khs_r = khs_ceil;

    const float khs_peak = kh_dls_reach_peak(khs_l);
    const float khs_a0 = khs_l.rec[17], khs_a1 = khs_l.rec[18], khs_a2 = khs_l.rec[19];
    const float khs_t = KH_DLS_ATTEN_FLOOR * khs_peak - khs_a0;
    if (khs_t > 0.0f) {
        float khs_d = -1.0f;
        if (khs_a2 > 1.0e-9f) {
            khs_d = (-khs_a1 + sqrtf(khs_a1 * khs_a1 + 4.0f * khs_a2 * khs_t)) / (2.0f * khs_a2);
        } else if (khs_a1 > 1.0e-9f) {
            khs_d = khs_t / khs_a1;
        }
        if (khs_d > 0.0f) {
            // The solve is in attenuated distance; undo the offset and the global distance scale to get metres, exactly
            // as DynLights does.
            const float khs_m = (khs_d + khs_l.rec[16]) / khs_sc;
            if (khs_m > 0.0f && khs_m < khs_r) khs_r = khs_m;
        }
    }
    if (khs_r > khs_ceil) khs_r = khs_ceil;
    if (khs_r < KH_DLS_REACH_MIN_M) khs_r = KH_DLS_REACH_MIN_M;
    return khs_r;
}

// The hold and the lookup share one rule (kh_dls_slot_of): the slot naming the light's id (KH_DL_POOL_ID), else the
// nearest orphan slot within KH_DLS_MATCH_M, a matching spot flag preferred, not required (a hold rewrites the slot's
// spot flag to its light's).

// Our meshes are the only occluders this feature draws, so a light with none in range cannot darken anything - its
// map is guaranteed-empty depth.
static constexpr float KH_DLS_HOLD_SLACK = 1.10f;   // Hold margin on the caster test.

// Nearest surface distance from this light to any of our meshes, or -1 when none. One walk per light per pass, cached
// by the caller: the admission gate and the hold test read the same number (the score too while no view is
// published: kh_dls_camera).
inline float kh_dls_caster_dist(const float* khr_lp) {
    float khr_best = -1.0f;
    kh_scene_sync();   // KH_SCENE.
    for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
        if (!g_scene.alive[khsc_i]) continue;
        const RenderObject& khr_o = g_scene.objs[khsc_i];
        if (!kh_shadow_active(khr_o)) continue;   // What kh_dls_frame draws into a map (an overlay casts nothing).
        float khr_he[3];
        kh_world_half_extents(khr_o, khr_he);   // Engine axes.
        // renderObject::pos is SQF [x, y, zASL]; engine is [x, zASL, y] - the same swizzle fill_dynlights_cb applies.
        const float khr_c[3] = { khr_o.pos[0], khr_o.pos[2], khr_o.pos[1] };
        // Surface distance, not centre distance: a large mesh whose centre is out of range can still have a corner lit,
        // and that corner casts.
        const float khr_dx = fmaxf(fabsf(khr_c[0] - khr_lp[0]) - khr_he[0], 0.0f);
        const float khr_dy = fmaxf(fabsf(khr_c[1] - khr_lp[1]) - khr_he[1], 0.0f);
        const float khr_dz = fmaxf(fabsf(khr_c[2] - khr_lp[2]) - khr_he[2], 0.0f);
        const float khr_d2 = khr_dx * khr_dx + khr_dy * khr_dy + khr_dz * khr_dz;
        if (khr_best < 0.0f || khr_d2 < khr_best) khr_best = khr_d2;
    }
    return khr_best < 0.0f ? -1.0f : sqrtf(khr_best);
}

// KH_DL_POOL_ID: does a light in the pool carry this id (a slot naming it is that light's)?
inline bool kh_dls_id_held(uint32_t khih_id) {
    for (uint32_t khih_i = 0; khih_i < g_dl.pool_n; ++khih_i) {
        if (g_dl.pool[khih_i].id == khih_id) return true;
    }
    return false;
}

// Eligible to hold a slot? khr_d is the cached nearest-caster distance.
inline bool kh_dls_relevant(float khr_d, float khr_reach) {
    return khr_d >= 0.0f && khr_d <= khr_reach;
}

// Called immediately ahead of render_sun_depth - not inside it, which returns early without a sun, while dynamic lights
// matter most at night. The frame guard makes the second call free.
inline void kh_dls_select() {
    if (g_dls_frame == g_topo_cycles) return;   // Once per frame, whoever arrives first.
    g_dls_frame = g_topo_cycles;

    const uint64_t khs_now = steady_now_ms();
    const uint64_t khs_prev = g_dls_sel_ms;   // KH_DLS_POOL_AGE: the previous pass's time (below), a
    g_dls_sel_ms = khs_now;                   // disallowed one included.

    // KH_DLS_ALLOW: disallowed = no slot. kh_dls_frame then takes its stand-down branch (no map, no caster list) and
    // the world pass returns at its first test. The per-light skip keys stay: nothing renders into the slices while
    // off, so a key that matches on return names a valid slice.
    if (!g_dls_allowed.load(std::memory_order_relaxed) || kh_shadows_off()) {   // KH_SHADOW_OFF too.
        for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) g_dls[khs_s].live = 0;
        g_dls_n = 0;
        return;
    }

    // A pool that stopped harvesting is empty here, so it releases every slot below. KH_DLS_POOL_AGE: the injection
    // runs this ahead of its own harvest (dynlights_acquire), so the stamp here is at least a frame old - the consumers
    // (kh_dl_select, the world pass) read it after. Do not stale the pool on a wall-clock gap alone (a hitch, alt-tab,
    // a frame over KH_DL_POOL_STALE_MS): it releases every slot for a frame while the meshes still take the lights. The
    // pool is stale only when it was ALREADY past the limit at the previous pass: a harvest that stopped leaves it
    // aging across passes and it goes a pass later, while a gap between two passes (whatever else came between them)
    // ages it at this pass alone. Nothing here counts depth clears or cycles, whose rate per frame is the engine's. The
    // first pass of a session has no previous one and takes the stamp's age alone.
    const bool khs_prev_old = khs_prev == 0u ||
                              (khs_prev > g_dl.pool_stamp && khs_prev - g_dl.pool_stamp > KH_DL_POOL_STALE_MS);
    const bool khs_stale = khs_now - g_dl.pool_stamp > KH_DL_POOL_STALE_MS && khs_prev_old;
    const uint32_t khs_pn = khs_stale ? 0u : g_dl.pool_n;

    // 1. EXPIRE. The pool is the slot table's only source and carries the engine's own expiry; a slot whose light is no
    // longer in the pool releases below, after the re-match (step 2) has given every pool light its chance.
    // KH_DLS_POOL_AGE: that release is the only one. Do not give a slot a wall-clock TTL of its own (500 ms since this
    // pass last re-matched it): it fires on a gap between passes alone, so a hitch releases every slot after the pool
    // rule above kept the pool, and the lights come back in the same pass, brightest first into the lowest free slots,
    // without their holds' slack or the oust margin, and every map re-renders.

    static std::vector<uint8_t> khs_used;   // render-thread scratch (the khco_r idiom).
    khs_used.assign(static_cast<size_t>(khs_pn), 0);
    // Two distances per light: khs_cdist = CAMERA to light, the ranking metric (the nearest to the viewer are the ones
    // whose shadows are on screen); khs_dist = light to nearest CASTER, the admission and hold gate (and the ranking
    // metric when no camera is published: kh_dls_camera). Both computed once per light per pass.
    static std::vector<float> khs_dist, khs_cdist, khs_reach;
    khs_dist.assign(static_cast<size_t>(khs_pn), -1.0f);
    khs_cdist.assign(static_cast<size_t>(khs_pn), -1.0f);
    khs_reach.assign(static_cast<size_t>(khs_pn), KH_DLS_REACH_MIN_M);
    float khs_cam[3];
    const bool khs_cam_ok = kh_dls_camera(khs_cam);
    for (uint32_t khs_i = 0; khs_i < khs_pn; ++khs_i) {
        const DlPoolLight& khs_pl = g_dl.pool[khs_i];
        khs_dist[khs_i] = kh_dls_caster_dist(khs_pl.rec);
        khs_reach[khs_i] = kh_dls_reach(khs_pl);
        if (khs_cam_ok) {
            const float khs_dx = khs_pl.rec[0] - khs_cam[0];
            const float khs_dy = khs_pl.rec[1] - khs_cam[1];
            const float khs_dz = khs_pl.rec[2] - khs_cam[2];
            khs_cdist[khs_i] = sqrtf(khs_dx * khs_dx + khs_dy * khs_dy + khs_dz * khs_dz);
        } else {
            khs_cdist[khs_i] = khs_dist[khs_i];
        }
    }
    const float khs_scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                          ? g_dl.main_scale : 1.0f;
    uint8_t  khs_filled[KH_DLS_MAX] = {};
    // KH_DL_POOL_ID: the live slots no pool light names by id - their light left the pool (a light seen again is
    // minted a new id); the position fallback below may hand one of those on, and no other.
    uint8_t  khs_orphan[KH_DLS_MAX] = {};
    for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
        if (g_dls[khs_s].live && !kh_dls_id_held(g_dls[khs_s].light_id)) khs_orphan[khs_s] = 1u;
    }

    // 2. RE-MATCH IN PLACE: a light still there refreshes the slot it already holds, so the slot -> light binding
    // survives a pool sweep.
    for (uint32_t khs_i = 0; khs_i < khs_pn; ++khs_i) {
        const DlPoolLight& khs_l = g_dl.pool[khs_i];
        // The hold takes its own slot, not the first in range: kh_dls_slot_of's rule (nearest live unfilled orphan
        // slot, matching spot flag preferred), so the two agree by construction.
        uint32_t khs_pick = KH_DLS_MAX;
        float    khs_pd = 0.0f;
        bool     khs_pflag = false;
        // A slot that names this light by id is its slot, whatever the positions say. The position rule below is the
        // fallback for a slot whose light left the pool and came back under a new id: it takes an orphan alone,
        // never a slot another pool light names.
        if (khs_l.id != 0) {
            for (uint32_t khs_c = 0; khs_c < KH_DLS_MAX; ++khs_c) {
                if (!g_dls[khs_c].live || khs_filled[khs_c]) continue;
                if (g_dls[khs_c].light_id == khs_l.id) { khs_pick = khs_c; break; }
            }
        }
        const bool khs_by_id = khs_pick != KH_DLS_MAX;
        for (uint32_t khs_c = 0; khs_c < KH_DLS_MAX && !khs_by_id; ++khs_c) {
            if (!g_dls[khs_c].live || khs_filled[khs_c] || !khs_orphan[khs_c]) continue;
            const float khs_cx = khs_l.rec[0] - g_dls[khs_c].pos[0];
            const float khs_cy = khs_l.rec[1] - g_dls[khs_c].pos[1];
            const float khs_cz = khs_l.rec[2] - g_dls[khs_c].pos[2];
            const float khs_cd = khs_cx * khs_cx + khs_cy * khs_cy + khs_cz * khs_cz;
            if (khs_cd > KH_DLS_MATCH_M * KH_DLS_MATCH_M) continue;
            const bool khs_cflag = g_dls[khs_c].spot == khs_l.spot;
            if (khs_pick == KH_DLS_MAX || (khs_cflag && !khs_pflag) ||
                (khs_cflag == khs_pflag && khs_cd < khs_pd)) {
                khs_pick = khs_c;
                khs_pd = khs_cd;
                khs_pflag = khs_cflag;
            }
        }
        for (uint32_t khs_s = khs_pick; khs_s < KH_DLS_MAX; ++khs_s) {   // The chosen slot, or none.
            g_dls[khs_s].pos[0] = khs_l.rec[0];
            g_dls[khs_s].pos[1] = khs_l.rec[1];
            g_dls[khs_s].pos[2] = khs_l.rec[2];
            g_dls[khs_s].score = kh_dls_score(khs_l, khs_cdist[khs_i], khs_scale);
            g_dls[khs_s].reach = khs_reach[khs_i];
            g_dls[khs_s].spot = khs_l.spot;
            g_dls[khs_s].light_id = khs_l.id;   // KH_DL_POOL_ID: a position hold learns the id.
            // KH_DLS_RELEVANCE: hold out to the slack radius.
            if (!kh_dls_relevant(khs_dist[khs_i],
                                 khs_reach[khs_i] * KH_DLS_HOLD_SLACK)) {
                g_dls[khs_s].live = 0;
                khs_used[khs_i] = 1;
                break;
            }
            khs_filled[khs_s] = 1;
            khs_used[khs_i] = 1;
            break;
        }
    }

    // A live slot no pool light re-matched this pass holds an expired light (step 2 released any past the hold radius).
    for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
        if (g_dls[khs_s].live && !khs_filled[khs_s]) {
            g_dls[khs_s].live = 0;
        }
    }
    // 3. ADMIT / EVICT, brightest first (max-per-round, allocation-free): it stops a dim light taking the last slot and
    // then having to be ousted while a brighter one waits. Bounded by the POOL, not the slot count: a light skipped for
    // having nothing to cast must not consume one of the chances to fill a slot.
    uint32_t khs_admitted = 0;
    for (uint32_t khs_r = 0; khs_r < khs_pn && khs_admitted < KH_DLS_MAX; ++khs_r) {
        uint32_t khs_best = 0xFFFFFFFFu;
        float    khs_bs = -1.0f;
        for (uint32_t khs_i = 0; khs_i < khs_pn; ++khs_i) {
            if (khs_used[khs_i]) continue;
            const float khs_sc = kh_dls_score(g_dl.pool[khs_i], khs_cdist[khs_i], khs_scale);
            if (khs_sc > khs_bs) { khs_bs = khs_sc; khs_best = khs_i; }
        }
        if (khs_best == 0xFFFFFFFFu) break;
        khs_used[khs_best] = 1;

        const DlPoolLight& khs_l = g_dl.pool[khs_best];
        // Nothing in range to cast -> not a candidate; skip, not break.
        if (!kh_dls_relevant(khs_dist[khs_best], khs_reach[khs_best])) {
            continue;
        }
        uint32_t khs_free = KH_DLS_MAX;
        for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
            if (!g_dls[khs_s].live) { khs_free = khs_s; break; }
        }

        uint32_t khs_take = KH_DLS_MAX;
        if (khs_free < KH_DLS_MAX) {
            khs_take = khs_free;
        } else {
            uint32_t khs_w = 0;
            for (uint32_t khs_s = 1; khs_s < KH_DLS_MAX; ++khs_s) {
                if (g_dls[khs_s].score < g_dls[khs_w].score) khs_w = khs_s;
            }
            if (khs_bs > g_dls[khs_w].score * KH_DLS_OUST_FRAC) {
                khs_take = khs_w;
            } else {
                // Release the claim before breaking.
                khs_used[khs_best] = 0;
                break;   // Everything after this is dimmer still.
            }
        }

        g_dls[khs_take].pos[0] = khs_l.rec[0];
        g_dls[khs_take].pos[1] = khs_l.rec[1];
        g_dls[khs_take].pos[2] = khs_l.rec[2];
        g_dls[khs_take].score = khs_bs;
        g_dls[khs_take].reach = khs_reach[khs_best];
        g_dls[khs_take].spot = khs_l.spot;
        g_dls[khs_take].light_id = khs_l.id;
        g_dls[khs_take].live = 1;
        khs_filled[khs_take] = 1;
        khs_admitted++;
    }

    // 4. The held count (g_dls_n, the output).
    uint32_t khs_n = 0;
    for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
        if (g_dls[khs_s].live) khs_n++;
    }
    g_dls_n = khs_n;
}

// Which shadow slot this pool light holds; -1 for none. Each light gets its own slot: a spot's frustum applied to a
// point beside it answers lit outside the cone.
inline int kh_dls_slot_of(const DlPoolLight& khs_l) {
    if (khs_l.id != 0) {   // KH_DL_POOL_ID: twin of the hold's first pass.
        for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
            if (g_dls[khs_s].live && g_dls[khs_s].light_id == khs_l.id) return static_cast<int>(khs_s);
        }
    }
    int   khs_best = -1;
    float khs_bd = 0.0f;
    bool  khs_bflag = false;
    for (uint32_t khs_s = 0; khs_s < KH_DLS_MAX; ++khs_s) {
        if (!g_dls[khs_s].live) continue;
        const float khs_dx = khs_l.rec[0] - g_dls[khs_s].pos[0];
        const float khs_dy = khs_l.rec[1] - g_dls[khs_s].pos[1];
        const float khs_dz = khs_l.rec[2] - g_dls[khs_s].pos[2];
        const float khs_d2 = khs_dx * khs_dx + khs_dy * khs_dy + khs_dz * khs_dz;
        if (khs_d2 > KH_DLS_MATCH_M * KH_DLS_MATCH_M) continue;
        if (kh_dls_id_held(g_dls[khs_s].light_id)) continue;   // KH_DL_POOL_ID: another light's slot (the hold's rule).
        const bool khs_flag = g_dls[khs_s].spot == khs_l.spot;
        if (khs_best < 0 || (khs_flag && !khs_bflag) ||
            (khs_flag == khs_bflag && khs_d2 < khs_bd)) {
            khs_best = static_cast<int>(khs_s);
            khs_bd = khs_d2;
            khs_bflag = khs_flag;
        }
    }
    return khs_best;
}

// Stage 2a - the geometry: face matrices, per-face culling, the skip hash. Anchored at the light: the matrices expect
// light-relative positions (wpos - slot.pos), since world-scale fp32 spacing (~8 mm) reads as acne in a depth compare.
// A cube face rotation is an axis permutation with signs, which makes the cull exact: an axis-aligned box maps to an
// axis-aligned box.
static constexpr int   KH_DLS_FACES = 6;
// Holding far/near at KH_DLS_NEAR_RATIO gives the lights it holds for (far 15 m - 75 m) one depth quality; below
// that span the floor sets near, above it the cap (KH_DLS_NEAR_CAP_M), so over the reach clamps far/near runs from
// 20 to 2400. The floor still applies: a light closer than the near plane to a caster clips it.
static constexpr float KH_DLS_NEAR_M = 0.05f;      // Absolute floor (5 cm).
static constexpr float KH_DLS_NEAR_RATIO = 300.0f;   // Far / near between the floor and the cap.

// One near-plane rule for the face matrices, the spot matrix and the cull: a map built to a different near than the
// cull tests against is a caster refused at a plane the raster would have accepted. Near must sit below the smallest
// caster-to-light distance the system can produce, hence the cap.
static constexpr float KH_DLS_NEAR_CAP_M = 0.25f;
inline float kh_dls_near(float khn_far) {
    float khn_r = khn_far / KH_DLS_NEAR_RATIO;
    if (khn_r > KH_DLS_NEAR_CAP_M) khn_r = KH_DLS_NEAR_CAP_M;
    return khn_r > KH_DLS_NEAR_M ? khn_r : KH_DLS_NEAR_M;
}
// Raising it is not obviously a win (six good slices may beat one bad one); settle by measurement - render one field
// spot both ways - before touching the constant.
static constexpr float KH_DLS_SPOT_FOV_MAX = 2.792527f;   // 160 deg; wider falls back to the cube.

// Cube basis, D3D face order (+X -X +Y -Y +Z -Z), right/up/forward per face. Written out so the table can be read
// against the D3D docs; a sign error here is a shadow on the wrong side of the world.
static const float KH_DLS_BASIS[6][9] = {
    {  0.0f, 0.0f,-1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
    {  0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,-1.0f },
};

// Row-vector world(light-relative) -> clip, 90 deg square perspective. Same convention as sunVP (v * VP, translation in
// row 3); translation is zero because the input is already light-relative.
inline void kh_dls_face_vp(int khf_face, float khf_far, float khf_out[16]) {
    const float* khf_b = KH_DLS_BASIS[khf_face < 0 ? 0 : (khf_face > 5 ? 5 : khf_face)];
    const float  khf_n = kh_dls_near(khf_far);
    const float  khf_f = khf_far > khf_n * 2.0f ? khf_far : khf_n * 2.0f;
    const float  khf_a = khf_f / (khf_f - khf_n);
    const float  khf_c = -khf_n * khf_f / (khf_f - khf_n);
    for (int khf_i = 0; khf_i < 3; ++khf_i) {
        khf_out[khf_i * 4 + 0] = khf_b[0 + khf_i];
        khf_out[khf_i * 4 + 1] = khf_b[3 + khf_i];
        khf_out[khf_i * 4 + 2] = khf_b[6 + khf_i] * khf_a;
        khf_out[khf_i * 4 + 3] = khf_b[6 + khf_i];
    }
    khf_out[12] = 0.0f; khf_out[13] = 0.0f; khf_out[14] = khf_c; khf_out[15] = 0.0f;
}

// A spot needs one frustum, not six. Half-angle is acos(cone cos-threshold), the lane DynLights compares against, so
// the map cannot cover less than the cone that samples it. False when the cone is too wide for a single frustum; the
// caller falls back to the cube.
inline bool kh_dls_spot_vp(const float* khs_dir, float khs_cone_cos, float khs_far,
                           float khs_out[16], float* khs_fov_out) {
    const float khs_l = sqrtf(khs_dir[0] * khs_dir[0] + khs_dir[1] * khs_dir[1] + khs_dir[2] * khs_dir[2]);
    if (!(khs_l > 1.0e-4f)) return false;
    const float khs_cc = khs_cone_cos < -0.999f ? -0.999f : (khs_cone_cos > 0.999f ? 0.999f : khs_cone_cos);
    // Reject on the raw cone; let the guard band yield to the cap when it does not fit (a slightly tight guard costs
    // the outermost texel row; a needless cube costs six passes).
    const float khs_raw = 2.0f * acosf(khs_cc);
    if (khs_raw > KH_DLS_SPOT_FOV_MAX) return false;   // Genuinely too wide for one frustum.
    float khs_fov = khs_raw * 1.15f;
    if (khs_fov > KH_DLS_SPOT_FOV_MAX) khs_fov = KH_DLS_SPOT_FOV_MAX;
    if (khs_fov < 0.12f) khs_fov = 0.12f;
    if (khs_fov_out) *khs_fov_out = khs_fov;

    // Forward = the spot axis; any stable perpendicular for right/up.
    const float khs_fx = khs_dir[0] / khs_l, khs_fy = khs_dir[1] / khs_l, khs_fz = khs_dir[2] / khs_l;
    float khs_ux = 0.0f, khs_uy = 1.0f, khs_uz = 0.0f;
    if (fabsf(khs_fy) > 0.99f) { khs_ux = 1.0f; khs_uy = 0.0f; khs_uz = 0.0f; }
    // Right = up x forward, then up = forward x right (both re-orthonormalized).
    float khs_rx = khs_uy * khs_fz - khs_uz * khs_fy;
    float khs_ry = khs_uz * khs_fx - khs_ux * khs_fz;
    float khs_rz = khs_ux * khs_fy - khs_uy * khs_fx;
    const float khs_rl = sqrtf(khs_rx * khs_rx + khs_ry * khs_ry + khs_rz * khs_rz);
    if (!(khs_rl > 1.0e-5f)) return false;
    khs_rx /= khs_rl; khs_ry /= khs_rl; khs_rz /= khs_rl;
    khs_ux = khs_fy * khs_rz - khs_fz * khs_ry;
    khs_uy = khs_fz * khs_rx - khs_fx * khs_rz;
    khs_uz = khs_fx * khs_ry - khs_fy * khs_rx;

    const float khs_n = kh_dls_near(khs_far);
    const float khs_f = khs_far > khs_n * 2.0f ? khs_far : khs_n * 2.0f;
    const float khs_s = 1.0f / tanf(khs_fov * 0.5f);
    const float khs_a = khs_f / (khs_f - khs_n);
    const float khs_c = -khs_n * khs_f / (khs_f - khs_n);
    const float khs_r3[3] = { khs_rx, khs_ry, khs_rz };
    const float khs_u3[3] = { khs_ux, khs_uy, khs_uz };
    const float khs_f3[3] = { khs_fx, khs_fy, khs_fz };
    for (int khs_i = 0; khs_i < 3; ++khs_i) {
        khs_out[khs_i * 4 + 0] = khs_r3[khs_i] * khs_s;
        khs_out[khs_i * 4 + 1] = khs_u3[khs_i] * khs_s;
        khs_out[khs_i * 4 + 2] = khs_f3[khs_i] * khs_a;
        khs_out[khs_i * 4 + 3] = khs_f3[khs_i];
    }
    khs_out[12] = 0.0f; khs_out[13] = 0.0f; khs_out[14] = khs_c; khs_out[15] = 0.0f;
    return true;
}

// Per-face cull. khc_c / khc_he are the caster's centre and half-extents in engine axes; khc_lp is the light. Exact for
// the cube (the basis is an axis permutation). Frustum in face space: |x| <= z, |y| <= z, near <= z <= far. A box is
// refused only when wholly outside one plane.
inline bool kh_dls_face_keep(int khc_face, const float* khc_lp,
                             const float* khc_c, const float* khc_he, float khc_far) {
    const float* khc_b = KH_DLS_BASIS[khc_face];
    const float khc_rel[3] = { khc_c[0] - khc_lp[0], khc_c[1] - khc_lp[1], khc_c[2] - khc_lp[2] };
    float khc_lo[3], khc_hi[3];
    for (int khc_k = 0; khc_k < 3; ++khc_k) {
        const float* khc_ax = khc_b + khc_k * 3;
        const float  khc_ctr = khc_rel[0] * khc_ax[0] + khc_rel[1] * khc_ax[1] + khc_rel[2] * khc_ax[2];
        // |axis| dotted with the half-extents: the box's own extent on this axis.
        const float  khc_ext = fabsf(khc_ax[0]) * khc_he[0]
                             + fabsf(khc_ax[1]) * khc_he[1]
                             + fabsf(khc_ax[2]) * khc_he[2];
        khc_lo[khc_k] = khc_ctr - khc_ext;
        khc_hi[khc_k] = khc_ctr + khc_ext;
    }
    // FP32 pad on the two depth planes; unpadded, the cull can refuse a caster the raster would accept. The pad only
    // keeps casters past the light's zero-contribution reach, which cannot darken anything.
    const float khc_pad = khc_far * 1.0e-3f + 0.01f;
    if (khc_hi[2] < kh_dls_near(khc_far) - khc_pad) return false;   // Wholly behind the near plane.
    if (khc_lo[2] > khc_far + khc_pad) return false;         // Wholly past the reach.
    if (khc_lo[0] > khc_hi[2]) return false;
    if (khc_hi[0] < -khc_hi[2]) return false;
    if (khc_lo[1] > khc_hi[2]) return false;
    if (khc_hi[1] < -khc_hi[2]) return false;
    return true;
}

// A light that moves changes the hash and forces a re-render, exactly as a moving mesh does for the union. Carries the
// direction and cone too, so a spot that turns in place re-renders.
inline uint64_t kh_dls_light_hash(const KhDlsSlot& khh_s, uint32_t khh_face_n,
                                  uint32_t khh_slice0, uint8_t khh_mask,
                                  uint64_t khh_casters, const float* khh_spot_vp) {
    uint64_t khh_k = CryptoGenerator::fnv1a64_update(CryptoGenerator::FNV1A64_OFFSET,
                                                     &khh_casters, sizeof(khh_casters));
    if (khh_spot_vp) khh_k = CryptoGenerator::fnv1a64_update(khh_k, khh_spot_vp, sizeof(float) * 16);
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, &khh_slice0, sizeof(khh_slice0));
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, &khh_mask, sizeof(khh_mask));
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, khh_s.pos, sizeof(khh_s.pos));
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, &khh_s.reach, sizeof(khh_s.reach));
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, &khh_s.spot, sizeof(khh_s.spot));
    khh_k = CryptoGenerator::fnv1a64_update(khh_k, &khh_face_n, sizeof(khh_face_n));
    if (khh_k == 0) khh_k = 1;   // 0 is the empty sentinel.
    return khh_k;
}

// The far plane a slot's maps are built to; floored so a degenerate reach never mints a zero far plane.
inline float kh_dls_far(const KhDlsSlot& khf_s) {
    return khf_s.reach > KH_DLS_REACH_MIN_M ? khf_s.reach : KH_DLS_REACH_MIN_M;
}

static uint64_t g_dls_keys[KH_DLS_MAX] = {};   // per-slot skip key (state, not a lane).
}