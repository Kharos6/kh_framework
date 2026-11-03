#pragma once

// The feature half (rendering_integration_features.hpp): DLS, the dynamic-light shadows' selection: slots, reach,
// faces and views. Never include it on its own.
namespace RenderIntegration {

// DL_SHADOW stage 1 - which lights cast. A slot holds its light by the pool light's id (DL_POOL_ID), by position
// only for a light that left the pool and came back under a new id - never by pool index (the pool is TTL-swept and
// compacted). The lights rank by their brightness as it reaches the camera (dls_score: luminance attenuated over the
// camera distance), so the casting set is the lights whose shadows are on screen; the distance to the nearest caster is
// the admission and hold gate (dls_select), and ranks only while no view is published (dls_camera).
static constexpr uint32_t DLS_MAX = 8;             // shadow-casting lights (the cap).
static constexpr float    DLS_MATCH_M = 1.0f;      // same-light radius; mirrors DL_MATCH_M.
static constexpr float    DLS_OUST_FRAC = 1.25f;   // A challenger must beat the sitting light by this.

struct DlsSlot {
    float    pos[3] = {};      // Engine axes - the slot's identity when no pool light carries its light_id.
    uint32_t light_id = 0;     // DL_POOL_ID: the pool entry this slot was admitted for
                               // (DlPoolLight::id); 0 = unknown.
    float    score = 0.0f;     // dls_score it holds the slot on (luminance attenuated over the camera distance).
    float    reach = 0.0f;     // dls_reach (m; hard fade, attenuation floor, ceiling); stage 2 sizes the map.
    uint8_t  spot = 0;         // 0 = point (omni), 1 = spot (single frustum).
    uint8_t  live = 0;
};

static DlsSlot g_dls[DLS_MAX];
static uint64_t  g_dls_frame = 0;       // once-per-frame guard (twin call sites).
static uint64_t  g_dls_sel_ms = 0;      // DLS_POOL_AGE: steady ms of dls_select's last pass (0 = none).

// The slot table (g_dls) is behaviour state, not reset with the stats: clearing it would re-admit from scratch at every
// reset.

// rec.709 luminance of the diffuse lobe plus the per-light ambient - what the light spends at source (the ambient lane
// is what makes A3 lights read on away-facing surfaces). What it delivers is below.
inline float dls_lum(const DlPoolLight& l) {
    const float d = 0.2126f * l.rec[8]  + 0.7152f * l.rec[9]  + 0.0722f * l.rec[10];
    const float a = 0.2126f * l.rec[12] + 0.7152f * l.rec[13] + 0.0722f * l.rec[14];
    return d + a;
}

// DynLights' distance attenuation, verbatim (twin contract with the shader kernel): 1/(a0 + a1*d + a2*d^2) on the
// offset, scaled distance, then the hard range fade. TWIN (five copies; this one without the cone): DynLights,
// KhDynLightsPBRE and KhDlsWorldFactor (cb.hlsl), KhDlfAtt (effect.hlsl).
inline float dls_atten(const DlPoolLight& l, float dist, float scale) {
    const float sd = dist * scale;
    const float d  = sd - l.rec[16] > 0.0f ? sd - l.rec[16] : 0.0f;
    float a = 1.0f / (l.rec[17] + l.rec[18] * d
                        + l.rec[19] * d * d + 1.0e-4f);
    if (a > 1.0f) a = 1.0f;
    if (a < 0.0f) a = 0.0f;
    float f = (sd - l.rec[20]) * l.rec[21];
    if (f > 1.0f) f = 1.0f;
    if (f < 0.0f) f = 0.0f;
    return a * (1.0f - f);
}

// The distance is camera -> light: ranking by caster distance can spend the whole budget on lights behind the player.
inline float dls_score(const DlPoolLight& l, float dist, float scale) {
    const float lum = dls_lum(l);
    if (dist < 0.0f) return lum;   // No distance available: rank on source alone.
    return lum * dls_atten(l, dist, scale);
}

// Camera on the render thread, engine axes. False when the engine has not published a view - dls_select then ranks
// on the light-to-caster distance (source luminance for a light with no caster) rather than a zeroed camera (a camera
// at the origin would rank confidently wrong).
inline bool dls_camera(float out[3]) {
    if (!g_ro.cycle_pv_valid) return false;
    cam_of(g_ro.cycle_pv.view, out);
    return out[0] == out[0] && out[1] == out[1] && out[2] == out[2];
}

// The hard-fade term is rec[20] + 1/rec[21], and a light authored with no range fade leaves the inverse width at
// ~1e-20, so that returns 7e19 metres - and the far plane, the near plane and the face cull all break quietly. Bound it
// by what the light can actually light.
static constexpr float DLS_REACH_MIN_M = 1.0f;   // A degenerate reach never yields a zero far
static constexpr float DLS_REACH_MAX_M = 600.0f;
static constexpr float DLS_ATTEN_FLOOR = 255.0f;   // 1/255 = one 8-bit level.

// Price the floor against what the light delivers: dls_lum times the same two global lanes the shader multiplies by,
// falling below one level.
inline float dls_reach_peak(const DlPoolLight& l) {
    // The two global lanes DynLights applies (dlGlobal.xyz to each light's diffuse term, dlGlobal.w to the sum), both
    // priced here on the whole dls_lum; rec.709 through dls_lum so this cannot disagree with the score.
    const float gd = (g_dl.main_ref_valid &&
                      (g_dl.main_gdiff[0] > 0.0f || g_dl.main_gdiff[1] > 0.0f ||
                       g_dl.main_gdiff[2] > 0.0f))
                   ? (0.2126f * g_dl.main_gdiff[0] + 0.7152f * g_dl.main_gdiff[1] +
                      0.0722f * g_dl.main_gdiff[2])
                   : 1.0f;
    float int_p = dl_intensity();   // The lane the shader's dlGlobal.w takes (dl_fill_ctl).
    if (!(int_p > 0.0f) || int_p != int_p) int_p = 1.0f;
    const float p = dls_lum(l) * gd * int_p;
    // Never below 1: a dim light must not get a shorter reach than its far.
    return (p > 1.0f && p == p) ? p : 1.0f;
}

// The distance past which nothing else in the frame is casting either.
inline float dls_reach_ceiling() {
    const float s = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    return s < DLS_REACH_MAX_M ? s : DLS_REACH_MAX_M;
}

inline float dls_reach(const DlPoolLight& l) {
    const float inv = l.rec[21];
    const float ceil = dls_reach_ceiling();
    // The hard fade in metres: it runs on the scaled distance (dls_atten), so its end divides by the global
    // distance scale - as the attenuation floor's solve below and dl_select's reach do.
    const float sc = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f) ? g_dl.main_scale : 1.0f;
    // DL_NO_FADE: a light with no hard fade (inverse width ~1e-20) is bounded by the ceiling and the attenuation
    // floor below, as dl_select takes it for every mesh - the fade start alone would cut its slot's reach, far plane
    // and casters where it still lights.
    float r = inv > 1.0e-6f ? (l.rec[20] + 1.0f / inv) / sc : ceil;
    if (!(r > 0.0f) || r != r) r = ceil;

    const float peak = dls_reach_peak(l);
    const float a0 = l.rec[17], a1 = l.rec[18], a2 = l.rec[19];
    const float t = DLS_ATTEN_FLOOR * peak - a0;
    if (t > 0.0f) {
        float d = -1.0f;
        if (a2 > 1.0e-9f) {
            d = (-a1 + sqrtf(a1 * a1 + 4.0f * a2 * t)) / (2.0f * a2);
        } else if (a1 > 1.0e-9f) {
            d = t / a1;
        }
        if (d > 0.0f) {
            // The solve is in attenuated distance; undo the offset and the global distance scale to get metres, exactly
            // as DynLights does.
            const float m = (d + l.rec[16]) / sc;
            if (m > 0.0f && m < r) r = m;
        }
    }
    if (r > ceil) r = ceil;
    if (r < DLS_REACH_MIN_M) r = DLS_REACH_MIN_M;
    return r;
}

// The hold and the lookup share one rule (dls_slot_of): the slot naming the light's id (DL_POOL_ID), else the
// nearest orphan slot within DLS_MATCH_M, a matching spot flag preferred, not required (a hold rewrites the slot's
// spot flag to its light's).

// Our meshes are the only occluders this feature draws, so a light with none in range cannot darken anything - its
// map is guaranteed-empty depth.
static constexpr float DLS_HOLD_SLACK = 1.10f;   // Hold margin on the caster test.

// Nearest surface distance from this light to any of our meshes, or -1 when none. One walk per light per pass, cached
// by the caller: the admission gate and the hold test read the same number (the score too while no view is
// published: dls_camera).
inline float dls_caster_dist(const float* lp) {
    float best = -1.0f;
    scene_sync();   // SCENE.
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        if (!shadow_active(o)) continue;   // What dls_frame draws into a map (an overlay casts nothing).
        float he[3];
        world_half_extents(o, he);   // Engine axes.
        // renderObject::pos is SQF [x, y, zASL]; engine is [x, zASL, y] - the same swizzle fill_dynlights_cb applies.
        const float c[3] = { o.pos[0], o.pos[2], o.pos[1] };
        // Surface distance, not centre distance: a large mesh whose centre is out of range can still have a corner lit,
        // and that corner casts.
        const float dx = fmaxf(fabsf(c[0] - lp[0]) - he[0], 0.0f);
        const float dy = fmaxf(fabsf(c[1] - lp[1]) - he[1], 0.0f);
        const float dz = fmaxf(fabsf(c[2] - lp[2]) - he[2], 0.0f);
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (best < 0.0f || d2 < best) best = d2;
    }
    return best < 0.0f ? -1.0f : sqrtf(best);
}

// DL_POOL_ID: does a light in the pool carry this id (a slot naming it is that light's)?
inline bool dls_id_held(uint32_t id) {
    for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
        if (g_dl.pool[i].id == id) return true;
    }
    return false;
}

// Eligible to hold a slot? d is the cached nearest-caster distance.
inline bool dls_relevant(float d, float reach) {
    return d >= 0.0f && d <= reach;
}

// Called immediately ahead of render_sun_depth - not inside it, which returns early without a sun, while dynamic lights
// matter most at night. The frame guard makes the second call free.
inline void dls_select() {
    if (g_dls_frame == g_topo_cycles) return;   // Once per frame, whoever arrives first.
    g_dls_frame = g_topo_cycles;

    const uint64_t now = steady_now_ms();
    const uint64_t prev = g_dls_sel_ms;   // DLS_POOL_AGE: the previous pass's time (below), a
    g_dls_sel_ms = now;                   // disallowed one included.

    // DLS_ALLOW: disallowed = no slot. dls_frame then takes its stand-down branch (no map, no caster list) and
    // the world pass returns at its first test. The per-light skip keys stay: nothing renders into the slices while
    // off, so a key that matches on return names a valid slice.
    if (!g_dls_allowed.load(std::memory_order_relaxed) || shadows_off()) {   // SHADOW_OFF too.
        for (uint32_t s = 0; s < DLS_MAX; ++s) g_dls[s].live = 0;
        g_dls_n = 0;
        return;
    }

    // A pool that stopped harvesting is empty here, so it releases every slot below. DLS_POOL_AGE: the injection
    // runs this ahead of its own harvest (dynlights_acquire), so the stamp here is at least a frame old - the consumers
    // (dl_select, the world pass) read it after. Do not stale the pool on a wall-clock gap alone (a hitch, alt-tab,
    // a frame over DL_POOL_STALE_MS): it releases every slot for a frame while the meshes still take the lights. The
    // pool is stale only when it was ALREADY past the limit at the previous pass: a harvest that stopped leaves it
    // aging across passes and it goes a pass later, while a gap between two passes (whatever else came between them)
    // ages it at this pass alone. Nothing here counts depth clears or cycles, whose rate per frame is the engine's. The
    // first pass of a session has no previous one and takes the stamp's age alone.
    const bool prev_old = prev == 0u ||
                          (prev > g_dl.pool_stamp && prev - g_dl.pool_stamp > DL_POOL_STALE_MS);
    const bool stale = now - g_dl.pool_stamp > DL_POOL_STALE_MS && prev_old;
    const uint32_t pn = stale ? 0u : g_dl.pool_n;

    // 1. EXPIRE. The pool is the slot table's only source and carries the engine's own expiry; a slot whose light is no
    // longer in the pool releases below, after the re-match (step 2) has given every pool light its chance.
    // DLS_POOL_AGE: that release is the only one. Do not give a slot a wall-clock TTL of its own (500 ms since this
    // pass last re-matched it): it fires on a gap between passes alone, so a hitch releases every slot after the pool
    // rule above kept the pool, and the lights come back in the same pass, brightest first into the lowest free slots,
    // without their holds' slack or the oust margin, and every map re-renders.

    static std::vector<uint8_t> used;   // render-thread scratch (static: its capacity stays).
    used.assign(static_cast<size_t>(pn), 0);
    // Two distances per light: cdist = CAMERA to light, the ranking metric (the nearest to the viewer are the ones
    // whose shadows are on screen); dist = light to nearest CASTER, the admission and hold gate (and the ranking
    // metric when no camera is published: dls_camera). Both computed once per light per pass.
    static std::vector<float> dist, cdist, reach;
    dist.assign(static_cast<size_t>(pn), -1.0f);
    cdist.assign(static_cast<size_t>(pn), -1.0f);
    reach.assign(static_cast<size_t>(pn), DLS_REACH_MIN_M);
    float cam[3];
    const bool cam_ok = dls_camera(cam);
    for (uint32_t i = 0; i < pn; ++i) {
        const DlPoolLight& pl = g_dl.pool[i];
        dist[i] = dls_caster_dist(pl.rec);
        reach[i] = dls_reach(pl);
        if (cam_ok) {
            const float dx = pl.rec[0] - cam[0];
            const float dy = pl.rec[1] - cam[1];
            const float dz = pl.rec[2] - cam[2];
            cdist[i] = sqrtf(dx * dx + dy * dy + dz * dz);
        } else {
            cdist[i] = dist[i];
        }
    }
    const float scale = (g_dl.main_ref_valid && g_dl.main_scale > 1.0e-6f)
                      ? g_dl.main_scale : 1.0f;
    uint8_t  filled[DLS_MAX] = {};
    // DL_POOL_ID: the live slots no pool light names by id - their light left the pool (a light seen again is
    // minted a new id); the position fallback below may hand one of those on, and no other.
    uint8_t  orphan[DLS_MAX] = {};
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        if (g_dls[s].live && !dls_id_held(g_dls[s].light_id)) orphan[s] = 1u;
    }

    // 2. RE-MATCH IN PLACE: a light still there refreshes the slot it already holds, so the slot -> light binding
    // survives a pool sweep.
    for (uint32_t i = 0; i < pn; ++i) {
        const DlPoolLight& l = g_dl.pool[i];
        // The hold takes its own slot, not the first in range: dls_slot_of's rule (nearest live unfilled orphan
        // slot, matching spot flag preferred), so the two agree by construction.
        uint32_t pick = DLS_MAX;
        float    pd = 0.0f;
        bool     pflag = false;
        // A slot that names this light by id is its slot, whatever the positions say. The position rule below is the
        // fallback for a slot whose light left the pool and came back under a new id: it takes an orphan alone,
        // never a slot another pool light names.
        if (l.id != 0) {
            for (uint32_t c = 0; c < DLS_MAX; ++c) {
                if (!g_dls[c].live || filled[c]) continue;
                if (g_dls[c].light_id == l.id) { pick = c; break; }
            }
        }
        const bool by_id = pick != DLS_MAX;
        for (uint32_t c = 0; c < DLS_MAX && !by_id; ++c) {
            if (!g_dls[c].live || filled[c] || !orphan[c]) continue;
            const float cx = l.rec[0] - g_dls[c].pos[0];
            const float cy = l.rec[1] - g_dls[c].pos[1];
            const float cz = l.rec[2] - g_dls[c].pos[2];
            const float cd = cx * cx + cy * cy + cz * cz;
            if (cd > DLS_MATCH_M * DLS_MATCH_M) continue;
            const bool cflag = g_dls[c].spot == l.spot;
            if (pick == DLS_MAX || (cflag && !pflag) ||
                (cflag == pflag && cd < pd)) {
                pick = c;
                pd = cd;
                pflag = cflag;
            }
        }
        for (uint32_t s = pick; s < DLS_MAX; ++s) {   // The chosen slot, or none.
            g_dls[s].pos[0] = l.rec[0];
            g_dls[s].pos[1] = l.rec[1];
            g_dls[s].pos[2] = l.rec[2];
            g_dls[s].score = dls_score(l, cdist[i], scale);
            g_dls[s].reach = reach[i];
            g_dls[s].spot = l.spot;
            g_dls[s].light_id = l.id;   // DL_POOL_ID: a position hold learns the id.
            // DLS_RELEVANCE: hold out to the slack radius.
            if (!dls_relevant(dist[i],
                              reach[i] * DLS_HOLD_SLACK)) {
                g_dls[s].live = 0;
                used[i] = 1;
                break;
            }
            filled[s] = 1;
            used[i] = 1;
            break;
        }
    }

    // A live slot no pool light re-matched this pass holds an expired light (step 2 released any past the hold radius).
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        if (g_dls[s].live && !filled[s]) {
            g_dls[s].live = 0;
        }
    }
    // 3. ADMIT / EVICT, brightest first (max-per-round, allocation-free): it stops a dim light taking the last slot and
    // then having to be ousted while a brighter one waits. Bounded by the POOL, not the slot count: a light skipped for
    // having nothing to cast must not consume one of the chances to fill a slot.
    uint32_t admitted = 0;
    for (uint32_t r = 0; r < pn && admitted < DLS_MAX; ++r) {
        uint32_t best = 0xFFFFFFFFu;
        float    bs = -1.0f;
        for (uint32_t i = 0; i < pn; ++i) {
            if (used[i]) continue;
            const float sc = dls_score(g_dl.pool[i], cdist[i], scale);
            if (sc > bs) { bs = sc; best = i; }
        }
        if (best == 0xFFFFFFFFu) break;
        used[best] = 1;

        const DlPoolLight& l = g_dl.pool[best];
        // Nothing in range to cast -> not a candidate; skip, not break.
        if (!dls_relevant(dist[best], reach[best])) {
            continue;
        }
        uint32_t free_s = DLS_MAX;
        for (uint32_t s = 0; s < DLS_MAX; ++s) {
            if (!g_dls[s].live) { free_s = s; break; }
        }

        uint32_t take = DLS_MAX;
        if (free_s < DLS_MAX) {
            take = free_s;
        } else {
            uint32_t w = 0;
            for (uint32_t s = 1; s < DLS_MAX; ++s) {
                if (g_dls[s].score < g_dls[w].score) w = s;
            }
            if (bs > g_dls[w].score * DLS_OUST_FRAC) {
                take = w;
            } else {
                // Release the claim before breaking.
                used[best] = 0;
                break;   // Everything after this is dimmer still.
            }
        }

        g_dls[take].pos[0] = l.rec[0];
        g_dls[take].pos[1] = l.rec[1];
        g_dls[take].pos[2] = l.rec[2];
        g_dls[take].score = bs;
        g_dls[take].reach = reach[best];
        g_dls[take].spot = l.spot;
        g_dls[take].light_id = l.id;
        g_dls[take].live = 1;
        filled[take] = 1;
        admitted++;
    }

    // 4. The held count (g_dls_n, the output).
    uint32_t n = 0;
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        if (g_dls[s].live) n++;
    }
    g_dls_n = n;
}

// Which shadow slot this pool light holds; -1 for none. Each light gets its own slot: a spot's frustum applied to a
// point beside it answers lit outside the cone.
inline int dls_slot_of(const DlPoolLight& l) {
    if (l.id != 0) {   // DL_POOL_ID: twin of the hold's first pass.
        for (uint32_t s = 0; s < DLS_MAX; ++s) {
            if (g_dls[s].live && g_dls[s].light_id == l.id) return static_cast<int>(s);
        }
    }
    int   best = -1;
    float bd = 0.0f;
    bool  bflag = false;
    for (uint32_t s = 0; s < DLS_MAX; ++s) {
        if (!g_dls[s].live) continue;
        const float dx = l.rec[0] - g_dls[s].pos[0];
        const float dy = l.rec[1] - g_dls[s].pos[1];
        const float dz = l.rec[2] - g_dls[s].pos[2];
        const float d2 = dx * dx + dy * dy + dz * dz;
        if (d2 > DLS_MATCH_M * DLS_MATCH_M) continue;
        if (dls_id_held(g_dls[s].light_id)) continue;   // DL_POOL_ID: another light's slot (the hold's rule).
        const bool flag = g_dls[s].spot == l.spot;
        if (best < 0 || (flag && !bflag) ||
            (flag == bflag && d2 < bd)) {
            best = static_cast<int>(s);
            bd = d2;
            bflag = flag;
        }
    }
    return best;
}

// Stage 2a - the geometry: face matrices, per-face culling, the skip hash. Anchored at the light: the matrices expect
// light-relative positions (wpos - slot.pos), since world-scale fp32 spacing (~8 mm) reads as acne in a depth compare.
// A cube face rotation is an axis permutation with signs, which makes the cull exact: an axis-aligned box maps to an
// axis-aligned box.
static constexpr int   DLS_FACES = 6;
// Holding far/near at KH_DLS_NEAR_RATIO gives the lights it holds for (far 15 m - 75 m) one depth quality; below
// that span the floor sets near, above it the cap (KH_DLS_NEAR_CAP_M), so over the reach clamps far/near runs from
// 20 to 2400. The floor still applies: a light closer than the near plane to a caster clips it.
static constexpr float KH_DLS_NEAR_M = 0.05f;      // Absolute floor (5 cm).
static constexpr float KH_DLS_NEAR_RATIO = 300.0f;   // Far / near between the floor and the cap.

// One near-plane rule for the face matrices, the spot matrix and the cull: a map built to a different near than the
// cull tests against is a caster refused at a plane the raster would have accepted. Near must sit below the smallest
// caster-to-light distance the system can produce, hence the cap.
static constexpr float KH_DLS_NEAR_CAP_M = 0.25f;
inline float dls_near(float n_far) {
    float r = n_far / KH_DLS_NEAR_RATIO;
    if (r > KH_DLS_NEAR_CAP_M) r = KH_DLS_NEAR_CAP_M;
    return r > KH_DLS_NEAR_M ? r : KH_DLS_NEAR_M;
}
// Raising it is not obviously a win (six good slices may beat one bad one); settle by measurement - render one field
// spot both ways - before touching the constant.
static constexpr float DLS_SPOT_FOV_MAX = 2.792527f;   // 160 deg; wider falls back to the cube.

// Cube basis, D3D face order (+X -X +Y -Y +Z -Z), right/up/forward per face. Written out so the table can be read
// against the D3D docs; a sign error here is a shadow on the wrong side of the world.
static const float DLS_BASIS[6][9] = {
    {  0.0f, 0.0f,-1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f },
    {  0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f,-1.0f, 0.0f },
    {  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,-1.0f },
};

// Row-vector world(light-relative) -> clip, 90 deg square perspective. Same convention as sunVP (v * VP, translation in
// row 3); translation is zero because the input is already light-relative.
inline void dls_face_vp(int face, float f_far, float out[16]) {
    const float* b = DLS_BASIS[face < 0 ? 0 : (face > 5 ? 5 : face)];
    const float  n = dls_near(f_far);
    const float  f = f_far > n * 2.0f ? f_far : n * 2.0f;
    const float  a = f / (f - n);
    const float  c = -n * f / (f - n);
    for (int i = 0; i < 3; ++i) {
        out[i * 4 + 0] = b[0 + i];
        out[i * 4 + 1] = b[3 + i];
        out[i * 4 + 2] = b[6 + i] * a;
        out[i * 4 + 3] = b[6 + i];
    }
    out[12] = 0.0f; out[13] = 0.0f; out[14] = c; out[15] = 0.0f;
}

// A spot needs one frustum, not six. Half-angle is acos(cone cos-threshold), the lane DynLights compares against, so
// the map cannot cover less than the cone that samples it. False when the cone is too wide for a single frustum; the
// caller falls back to the cube.
inline bool dls_spot_view_proj(const float* dir, float cone_cos, float far_s,
                               float out[16], float* fov_out) {
    const float l = sqrtf(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (!(l > 1.0e-4f)) return false;
    const float cc = cone_cos < -0.999f ? -0.999f : (cone_cos > 0.999f ? 0.999f : cone_cos);
    // Reject on the raw cone; let the guard band yield to the cap when it does not fit (a slightly tight guard costs
    // the outermost texel row; a needless cube costs six passes).
    const float raw = 2.0f * acosf(cc);
    if (raw > DLS_SPOT_FOV_MAX) return false;   // Genuinely too wide for one frustum.
    float fov = raw * 1.15f;
    if (fov > DLS_SPOT_FOV_MAX) fov = DLS_SPOT_FOV_MAX;
    if (fov < 0.12f) fov = 0.12f;
    if (fov_out) *fov_out = fov;

    // Forward = the spot axis; any stable perpendicular for right/up.
    const float fx = dir[0] / l, fy = dir[1] / l, fz = dir[2] / l;
    float ux = 0.0f, uy = 1.0f, uz = 0.0f;
    if (fabsf(fy) > 0.99f) { ux = 1.0f; uy = 0.0f; uz = 0.0f; }
    // Right = up x forward, then up = forward x right (both re-orthonormalized).
    float rx = uy * fz - uz * fy;
    float ry = uz * fx - ux * fz;
    float rz = ux * fy - uy * fx;
    const float rl = sqrtf(rx * rx + ry * ry + rz * rz);
    if (!(rl > 1.0e-5f)) return false;
    rx /= rl; ry /= rl; rz /= rl;
    ux = fy * rz - fz * ry;
    uy = fz * rx - fx * rz;
    uz = fx * ry - fy * rx;

    const float n = dls_near(far_s);
    const float f = far_s > n * 2.0f ? far_s : n * 2.0f;
    const float s = 1.0f / tanf(fov * 0.5f);
    const float a = f / (f - n);
    const float c = -n * f / (f - n);
    const float r3[3] = { rx, ry, rz };
    const float u3[3] = { ux, uy, uz };
    const float f3[3] = { fx, fy, fz };
    for (int i = 0; i < 3; ++i) {
        out[i * 4 + 0] = r3[i] * s;
        out[i * 4 + 1] = u3[i] * s;
        out[i * 4 + 2] = f3[i] * a;
        out[i * 4 + 3] = f3[i];
    }
    out[12] = 0.0f; out[13] = 0.0f; out[14] = c; out[15] = 0.0f;
    return true;
}

// Per-face cull. c / he are the caster's centre and half-extents in engine axes; lp is the light. Exact for
// the cube (the basis is an axis permutation). Frustum in face space: |x| <= z, |y| <= z, near <= z <= far. A box is
// refused only when wholly outside one plane.
inline bool dls_face_keep(int face, const float* lp,
                          const float* c, const float* he, float c_far) {
    const float* b = DLS_BASIS[face];
    const float rel[3] = { c[0] - lp[0], c[1] - lp[1], c[2] - lp[2] };
    float lo[3], hi[3];
    for (int k = 0; k < 3; ++k) {
        const float* ax = b + k * 3;
        const float  ctr = rel[0] * ax[0] + rel[1] * ax[1] + rel[2] * ax[2];
        // |axis| dotted with the half-extents: the box's own extent on this axis.
        const float  ext = fabsf(ax[0]) * he[0]
                         + fabsf(ax[1]) * he[1]
                         + fabsf(ax[2]) * he[2];
        lo[k] = ctr - ext;
        hi[k] = ctr + ext;
    }
    // FP32 pad on the two depth planes; unpadded, the cull can refuse a caster the raster would accept. The pad only
    // keeps casters past the light's zero-contribution reach, which cannot darken anything.
    const float pad = c_far * 1.0e-3f + 0.01f;
    if (hi[2] < dls_near(c_far) - pad) return false;   // Wholly behind the near plane.
    if (lo[2] > c_far + pad) return false;         // Wholly past the reach.
    if (lo[0] > hi[2]) return false;
    if (hi[0] < -hi[2]) return false;
    if (lo[1] > hi[2]) return false;
    if (hi[1] < -hi[2]) return false;
    return true;
}

// A light that moves changes the hash and forces a re-render, exactly as a moving mesh does for the union. Carries the
// direction and cone too, so a spot that turns in place re-renders.
inline uint64_t dls_light_hash(const DlsSlot& s, uint32_t face_n,
                               uint32_t slice0, uint8_t mask,
                               uint64_t casters, const float* spot_vp) {
    uint64_t k = CryptoGenerator::fnv1a64_update(CryptoGenerator::FNV1A64_OFFSET,
                                                 &casters, sizeof(casters));
    if (spot_vp) k = CryptoGenerator::fnv1a64_update(k, spot_vp, sizeof(float) * 16);
    k = CryptoGenerator::fnv1a64_update(k, &slice0, sizeof(slice0));
    k = CryptoGenerator::fnv1a64_update(k, &mask, sizeof(mask));
    k = CryptoGenerator::fnv1a64_update(k, s.pos, sizeof(s.pos));
    k = CryptoGenerator::fnv1a64_update(k, &s.reach, sizeof(s.reach));
    k = CryptoGenerator::fnv1a64_update(k, &s.spot, sizeof(s.spot));
    k = CryptoGenerator::fnv1a64_update(k, &face_n, sizeof(face_n));
    if (k == 0) k = 1;   // 0 is the empty sentinel.
    return k;
}

// The far plane a slot's maps are built to; floored so a degenerate reach never mints a zero far plane.
inline float dls_far(const DlsSlot& s) {
    return s.reach > DLS_REACH_MIN_M ? s.reach : DLS_REACH_MIN_M;
}

static uint64_t g_dls_keys[DLS_MAX] = {};   // per-slot skip key (state, not a lane).
}