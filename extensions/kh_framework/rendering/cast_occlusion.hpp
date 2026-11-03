#pragma once

// The injection half (rendering_integration_injection.hpp): the cast occlusion (OCC): the sweep, the stamps and the
// build. Never include it on its own.
namespace RenderIntegration {

// CAST_OCC: the caster-occupancy grid. PSMaskCast's reach test ("is this world point within reach of any caster")
// reads one cell instead of looping every caster per pixel. A cell holds the union of the casters' stamped sweeps over
// its area, so it refuses only points no caster's shadow reaches - no shadow is lost (SunShadowOcclusion decides), only
// more pixels reach the expensive path. It is not a superset of that loop's inflated reach (below).
static constexpr int OCC_N = 128;
// CASTOCC_DOMAIN: the grid covers the RECEIVER domain (the pixels KhSunRangeFade leaves non-zero: a vertical
// cylinder of radius 0.995 * range about the camera, +-0.995 * range tall), not the caster set. floor = camera.y
// - range - slack (the deepest receiver, wherever the camera moves within its slack). Each sweep is clipped to the
// domain disc before stamping, so the extent is ~2 * (range + slack) whatever the caster set. The grid is stamped for a
// BAND of suns (elevation [syq - 1/64, syq + 2/64], +-0.5 deg azimuth) so a slowly moving sun does not force a rebuild
// at every bucket edge, and the camera may move 0.5 R (clamped to 40 - 250 m) from the built centre before one; a
// range longer than the one it was built for rebuilds too (CASTOCC_RANGE). The <= 16 caster loop (KhCastReach)
// keeps its own inflated reach; the grid is a superset of the truth, not of that loop.
static constexpr float OCC_SY_Q = 64.0f;
static constexpr float OCC_SLACK_M = 6.0f;        // Around the exact sweep, lateral and vertical.
static constexpr float OCC_AZ_TAN = 0.0087269f;   // tan(0.5 deg): widening and hysteresis.
static constexpr float OCC_AZ_COS = 0.9999619f;   // cos(0.5 deg).
static constexpr float OCC_CAM_SLACK = 0.50f;     // Of the range; clamped [40, 250] m.
// One caster's sweep, clipped to the domain disc. on = false means no part of the swept shadow can reach a receiver the
// fade keeps: neither stamped nor counted in the extent.
struct OccSweep { float hu, top, base, ta, tb, w0; bool on; };
// The sun band the grid was stamped for, derived once per build from the fire's frozen sun and retained with the grid.
struct OccSun {
    float sy_lo, sy_hi;       // Elevation band, sun.y.
    float k_hi;               // Metres down-sun per metre of drop at the SHALLOWEST sun.
    float slope_lo, slope_hi; // Drop per metre down-sun: shallowest, steepest.
    float ux, uz;             // Down-sun unit (xz) at the band centre.
};
struct OccPending {
    bool     active = false;
    bool     done = false;
    uint64_t key = 0;
    int      n = 0;
    int      next = 0;
    OccSun sb = {};
    float    dom[3] = {};
    float    slack = 0.0f, rdom = 0.0f, floor = 0.0f, rng = 0.0f;
    float    lo[2] = {};
    float    cell = 0.0f, inv = 0.0f;
    std::vector<OccSweep> sw;
    std::vector<float>      grid;
};
// The cast occlusion (OCC): the sweep's grid and set, the cells and the sun basis, the pre-pass and the key cache.
// Owner: the render thread.
// Session reset: session_globals_reset resets every member. session_scratch_reset resets 4 of them.
struct CastOccState {
    uint64_t key = 0;   // FNV-1a over (locals, range) of the last build (0 = none).
    float    lo[2] = {};   // Retained grid origin XZ (engine axes).
    float    inv = 0.0f;   // Retained 1 / cell.
    std::vector<float>    grid;    // (minY, maxY) per texel, retained.
    std::vector<uint64_t> set;     // Sorted per-caster hashes of the built set.
    std::vector<uint32_t> addi;    // Scratch: indices of the additions.
    float    cell = 0.0f;
    // The built envelope: what the retained grid still covers.
    OccSun sunb = {};
    float    dom[3] = {};      // Domain centre (engine axes) the grid was built about.
    float    slack = 0.0f;     // Camera slack (m); 0 = no retained grid.
    float    rdom = 0.0f;      // Disc radius the sweeps were clipped to.
    float    rng = 0.0f;       // CASTOCC_RANGE: the shadow range the disc and floor were built for.
    float    floor = 0.0f;     // Receiver floor the sweeps were marched to.
    OccPending pre;
    bool     pre_noreach = false;   // Phase 0 found no sweep in the pending domain...
    float    pre_noreach_dom[3] = {};   // ...about this domain; retry once the camera moves.
    // SESSION_RESET: at file scope, not in the function below, so the session reset reaches it.
    uint64_t kc = 0;
    uint64_t kc_ser = ~0ull;
    float    kc_rng = -1.0f;
    int      kc_n = -1;
};
static CastOccState g_castocc;
static constexpr int OCC_ADD_MAX = 256;      // Additions worth stamping one by one.
static constexpr int OCC_STALE = 4;          // Rebuild past built > n + n/OCC_STALE.
static constexpr float OCC_EXTENT_PAD = 0.10f;

inline uint64_t castocc_hash1(const float* e) {
    return CryptoGenerator::fnv1a64_raw(e, 6u * sizeof(float));
}

inline OccSun castocc_sun(const float* sun) {   // Toward the light.
    OccSun o;
    const float sy = fabsf(sun[1]);
    const float hl = sqrtf(sun[0] * sun[0] + sun[2] * sun[2]);
    const float syq = floorf(sy * OCC_SY_Q) / OCC_SY_Q;
    o.sy_lo = syq - 1.0f / OCC_SY_Q;
    if (o.sy_lo < 0.02f) o.sy_lo = 0.02f;
    o.sy_hi = syq + 2.0f / OCC_SY_Q;
    if (o.sy_hi > 1.0f) o.sy_hi = 1.0f;
    const float hl_hi = sqrtf(fmaxf(1.0f - o.sy_lo * o.sy_lo, 0.0f));
    const float hl_lo = sqrtf(fmaxf(1.0f - o.sy_hi * o.sy_hi, 0.0f));
    o.k_hi = hl_hi / o.sy_lo;
    o.slope_lo = o.sy_lo / fmaxf(hl_hi, 1.0e-6f);
    o.slope_hi = o.sy_hi / fmaxf(hl_lo, 1.0e-6f);
    o.ux = hl > 1.0e-4f ? (-sun[0] / hl) : 0.0f;
    o.uz = hl > 1.0e-4f ? (-sun[2] / hl) : 0.0f;
    return o;
}

inline float castocc_slack_of(float r) {
    float s = OCC_CAM_SLACK * r;
    if (s < 40.0f) s = 40.0f;
    if (s > 250.0f) s = 250.0f;
    return s;
}

// CASTOCC_PRE: a full rebuild fires the moment the camera leaves the built slack disc (a hitch frame). The rebuild
// is a pure function of (locals, range, domain, sun band, floor), so once the camera is past OCC_PRE_START of the
// slack a second grid is built about the CURRENT camera, OCC_PRE_PER_FIRE casters per fire, and committed at the
// next fire its envelope covers. A camera that outruns it takes the synchronous rebuild; a pending grid whose inputs no
// longer fit is dropped.
static constexpr float OCC_PRE_START = 0.5f;   // Of the slack: start the pre-build here.
static constexpr int   OCC_PRE_PER_FIRE = 192; // Casters stamped per fire (~0.5 ms at 3,000).

inline OccSweep castocc_sweep(const float* e, const OccSun& s,
                              const float* dom, float rdom, float floor) {
    OccSweep o = {};
    const float hx = e[3], hy = e[4], hz = e[5];
    o.hu = fabsf(s.ux) * hx + fabsf(s.uz) * hz;   // Box half-extent along u.
    o.top = e[1] + hy;
    o.base = e[1] - hy;
    // The top ray reaches the floor at (top - floor) * k, plus the box's own down-sun face.
    const float drop = o.top - floor;
    const float al = (drop > 0.0f ? drop : 0.0f) * s.k_hi + o.hu;
    o.w0 = (hx > hz ? hx : hz) + OCC_SLACK_M;
    const float wmax = o.w0 + (al + o.hu) * OCC_AZ_TAN;
    // Disc clip: |c + u t - dom| <= rdom + wmax * sqrt(2), a quadratic in t.
    const float qx = e[0] - dom[0];
    const float qz = e[2] - dom[2];
    const float rc = rdom + wmax * 1.4142136f;
    const float b = 2.0f * (qx * s.ux + qz * s.uz);
    const float c = qx * qx + qz * qz - rc * rc;
    const float disc = b * b - 4.0f * c;
    if (disc < 0.0f) return o;   // The sweep line misses the domain.
    const float rt = sqrtf(disc);
    const float tin = (-b - rt) * 0.5f;
    const float tout = (-b + rt) * 0.5f;
    o.ta = -o.hu > tin ? -o.hu : tin;
    o.tb = al < tout ? al : tout;
    o.on = o.ta <= o.tb;
    return o;
}

// The XZ box a sweep's stamps will touch, for the extent and the fit test.
inline void castocc_box(const float* e, const OccSun& s, const OccSweep& w,
                        float lo[2], float hi[2]) {
    const float wa = w.w0 + (w.tb + w.hu) * OCC_AZ_TAN;
    const float ex = e[3] + OCC_SLACK_M + wa;
    const float ez = e[5] + OCC_SLACK_M + wa;
    const float ax = e[0] + s.ux * w.ta, az = e[2] + s.uz * w.ta;
    const float bx = e[0] + s.ux * w.tb, bz = e[2] + s.uz * w.tb;
    lo[0] = (ax < bx ? ax : bx) - ex;
    hi[0] = (ax > bx ? ax : bx) + ex;
    lo[1] = (az < bz ? az : bz) - ez;
    hi[1] = (az > bz ? az : bz) + ez;
}

inline bool castocc_fits(const float* e, const OccSun& s, const OccSweep& w,
                         const float* lo, float cell, float inv) {
    if (!w.on) return true;   // Nothing to stamp always fits.
    float b0[2], b1[2];
    castocc_box(e, s, w, b0, b1);
    const float gx = (b0[0] - cell - lo[0]) * inv;
    const float gz = (b0[1] - cell - lo[1]) * inv;
    const float gx1 = (b1[0] + cell - lo[0]) * inv;
    const float gz1 = (b1[1] + cell - lo[1]) * inv;
    return gx >= 0.0f && gz >= 0.0f &&
           gx1 <= static_cast<float>(OCC_N) - 1.0f &&
           gz1 <= static_cast<float>(OCC_N) - 1.0f;
}

// Stamp one clipped sweep. Steps every max(w0, cell) along u from ta to tb; each step writes a block of +-(h + slack
// + pad + widening) cells whose y window is the exact shadow band over the step's t range at the band's extreme slopes.
// The last step is open-bottomed: past the floor there is no receiver.
inline void castocc_stamp(const float* e, const OccSun& s, const OccSweep& w,
                          const float* lo, float cell, float inv,
                          float* grid) {   // CASTOCC_PRE: the retained or the pending grid.
    if (!w.on) return;
    const float hx = e[3], hz = e[5];
    const float dtw = w.w0 > cell ? w.w0 : cell;   // OCC_COST: one cell per step.
    int ns = static_cast<int>(ceilf((w.tb - w.ta) / (dtw > 1.0e-4f ? dtw : 1.0e-4f)));
    if (ns < 0) ns = 0;
    if (ns > 4 * OCC_N) ns = 4 * OCC_N;
    const float dt = ns > 0 ? ((w.tb - w.ta) / static_cast<float>(ns)) : 0.0f;
    const float pad = cell > 0.5f * dt ? cell : 0.5f * dt;

    for (int si = 0; si <= ns; ++si) {
        const float t = w.ta + dt * static_cast<float>(si);
        float wid = (t + dt + pad + w.hu) * OCC_AZ_TAN;
        if (wid < 0.0f) wid = 0.0f;
        const float cx = e[0] + s.ux * t;
        const float cz = e[2] + s.uz * t;
        const float rx = hx + OCC_SLACK_M + pad + wid;
        const float rz = hz + OCC_SLACK_M + pad + wid;
        int x0 = static_cast<int>(floorf((cx - rx - lo[0]) * inv));
        int x1 = static_cast<int>(floorf((cx + rx - lo[0]) * inv));
        int z0 = static_cast<int>(floorf((cz - rz - lo[1]) * inv));
        int z1 = static_cast<int>(floorf((cz + rz - lo[1]) * inv));
        if (x0 < 0) x0 = 0;
        if (z0 < 0) z0 = 0;
        if (x1 > OCC_N - 1) x1 = OCC_N - 1;
        if (z1 > OCC_N - 1) z1 = OCC_N - 1;
        if (x1 < x0 || z1 < z0) continue;
        const float y0 = (si == ns)
                       ? -1.0e30f
                       : w.base - (t + dt + pad + w.hu) * s.slope_hi
                         - OCC_SLACK_M;
        float tt = t - pad - w.hu;
        if (tt < 0.0f) tt = 0.0f;
        const float y1 = w.top - tt * s.slope_lo + OCC_SLACK_M + 1.0e-3f;

        for (int z = z0; z <= z1; ++z) {
            float* row = grid + (static_cast<size_t>(z) * OCC_N) * 2u;

            for (int x = x0; x <= x1; ++x) {
                if (y0 < row[x * 2]) row[x * 2] = y0;
                if (y1 > row[x * 2 + 1]) row[x * 2 + 1] = y1;
            }
        }
    }
}

inline bool castocc_upload(ID3D11DeviceContext* ctx) {
    D3D11_MAPPED_SUBRESOURCE m = {};
    if (FAILED(ctx->Map(g_res.castocc_tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &m))) return false;

    for (int z = 0; z < OCC_N; ++z) {
        memcpy(static_cast<uint8_t*>(m.pData) + static_cast<size_t>(z) * m.RowPitch,
               g_castocc.grid.data() + static_cast<size_t>(z) * OCC_N * 2u,
               static_cast<size_t>(OCC_N) * 2u * sizeof(float));
    }

    ctx->Unmap(g_res.castocc_tex, 0);
    return true;
}

// Every failure return leaves NO retained grid (a rewritten grid under stale lo/inv/cell is a live hazard).
inline bool castocc_fail() {
    g_castocc.pre.active = false;   // CASTOCC_PRE.
    g_castocc.key = 0;
    g_castocc.set.clear();
    g_castocc.slack = 0.0f;
    return false;
}

// One fire's share of the pre-build, on the hit path. Starts a pending grid once the camera is past OCC_PRE_START of
// the slack, stamps OCC_PRE_PER_FIRE casters per call, and commits the completed grid on the first fire its own
// envelope covers. Every quantity is the rebuild's own arithmetic about the pending domain.
inline void castocc_pre_step(ID3D11DeviceContext* ctx, int n, uint64_t key,
                             const float* dom, const float* sun, float rng) {
    OccPending& p = g_castocc.pre;

    if (p.active && (p.key != key || p.n != n)) {   // The locals moved.
        p.active = false;
    }

    if (!p.active) {
        const float dx = dom[0] - g_castocc.dom[0];
        const float dz = dom[2] - g_castocc.dom[2];
        const float pp_far = OCC_PRE_START * g_castocc.slack;
        if (dx * dx + dz * dz <= pp_far * pp_far &&
            dom[1] >= g_castocc.dom[1] - pp_far) {
            return;   // The retained grid has room to spare.
        }
        if (g_castocc.pre_noreach) {   // A no-reach attempt is not retried until the camera moves.
            const float nx = dom[0] - g_castocc.pre_noreach_dom[0];
            const float ny = dom[1] - g_castocc.pre_noreach_dom[1];
            const float nz = dom[2] - g_castocc.pre_noreach_dom[2];
            const float nr = 0.1f * g_castocc.slack;
            if (nx * nx + ny * ny + nz * nz <= nr * nr) return;
            g_castocc.pre_noreach = false;
        }

        // Phase 0: the sweeps and the extent about the pending domain.
        p.key = key;
        p.n = n;
        p.sb = castocc_sun(sun);
        p.slack = castocc_slack_of(rng);
        p.rdom = 0.995f * rng + p.slack;
        p.floor = dom[1] - rng - p.slack;
        p.rng = rng;
        p.dom[0] = dom[0]; p.dom[1] = dom[1]; p.dom[2] = dom[2];
        float lo[2] = { 1.0e30f, 1.0e30f };
        float hi[2] = { -1.0e30f, -1.0e30f };
        p.sw.resize(static_cast<size_t>(n));

        for (int i = 0; i < n; ++i) {
            const float* e = g_sun_maps.sun_locals.data() + static_cast<size_t>(i) * 6u;
            p.sw[static_cast<size_t>(i)] = castocc_sweep(e, p.sb, p.dom, p.rdom, p.floor);
            const OccSweep& w = p.sw[static_cast<size_t>(i)];
            if (!w.on) continue;
            float b0[2], b1[2];
            castocc_box(e, p.sb, w, b0, b1);
            if (b0[0] < lo[0]) lo[0] = b0[0];
            if (b0[1] < lo[1]) lo[1] = b0[1];
            if (b1[0] > hi[0]) hi[0] = b1[0];
            if (b1[1] > hi[1]) hi[1] = b1[1];
        }

        if (lo[0] > hi[0]) {   // No sweep reaches the pending domain: nothing to pre-build.
            g_castocc.pre_noreach = true;
            g_castocc.pre_noreach_dom[0] = dom[0]; g_castocc.pre_noreach_dom[1] = dom[1];
            g_castocc.pre_noreach_dom[2] = dom[2];
            return;
        }
        {   // CASTOCC_PAD, as the rebuild pads.
            const float g0 = (hi[0] - lo[0]) * OCC_EXTENT_PAD;
            const float g1 = (hi[1] - lo[1]) * OCC_EXTENT_PAD;
            lo[0] -= g0; hi[0] += g0;
            lo[1] -= g1; hi[1] += g1;
        }
        const float sp0 = hi[0] - lo[0];
        const float sp1 = hi[1] - lo[1];
        // CASTOCC_SPAN: the rebuild's guard (below).
        if (!(sp0 > 0.0f && sp0 <= 1.0e30f) || !(sp1 > 0.0f && sp1 <= 1.0e30f)) return;
        float cell = (sp0 > sp1 ? sp0 : sp1) / OCC_N;
        if (!(cell > 1.0e-4f)) cell = 1.0e-4f;
        p.lo[0] = lo[0]; p.lo[1] = lo[1];
        p.cell = cell;
        p.inv = 1.0f / cell;
        p.grid.resize(static_cast<size_t>(OCC_N) * OCC_N * 2u);
        for (size_t g = 0; g < p.grid.size(); g += 2) { p.grid[g] = 1.0e30f; p.grid[g + 1] = -1.0e30f; }
        p.next = 0;
        p.done = false;
        p.active = true;
        return;   // The stamping begins next fire.
    }

    if (!p.done) {   // Phase 1: a slice of the stamps.
        int end = p.next + OCC_PRE_PER_FIRE;
        if (end > p.n) end = p.n;

        for (int i = p.next; i < end; ++i) {
            const float* e = g_sun_maps.sun_locals.data() + static_cast<size_t>(i) * 6u;
            castocc_stamp(e, p.sb, p.sw[static_cast<size_t>(i)], p.lo,
                          p.cell, p.inv, p.grid.data());
        }

        p.next = end;
        if (p.next >= p.n) p.done = true;
        if (!p.done) return;
    }

    // Complete: commit only into an envelope that covers THIS fire.
    {
        float sy = fabsf(sun[1]);
        if (sy < 0.02f) sy = 0.02f;   // CASTOCC_LOWSUN (the rebuild's test below).
        const float shl = sqrtf(sun[0] * sun[0] + sun[2] * sun[2]);
        const float ux = shl > 1.0e-4f ? (-sun[0] / shl) : 0.0f;
        const float uz = shl > 1.0e-4f ? (-sun[2] / shl) : 0.0f;
        const bool bucket = sy >= p.sb.sy_lo && sy <= p.sb.sy_hi;
        const bool az = (ux * p.sb.ux + uz * p.sb.uz) >= OCC_AZ_COS;
        const float dx = dom[0] - p.dom[0];
        const float dz = dom[2] - p.dom[2];
        const bool cam = dx * dx + dz * dz <= p.slack * p.slack;
        const bool floor = dom[1] >= p.dom[1] - p.slack;

        if (!(bucket && az && cam && floor)) {
            p.active = false;
            return;
        }
    }

    g_castocc.grid.swap(p.grid);   // The pending grid becomes the retained one.
    p.active = false;

    if (!castocc_upload(ctx)) {   // The rebuild's own failure rule: no retained grid.
        castocc_fail();
        return;
    }

    g_castocc.lo[0] = p.lo[0];
    g_castocc.lo[1] = p.lo[1];
    g_castocc.inv = p.inv;
    g_castocc.cell = p.cell;
    g_castocc.sunb = p.sb;
    g_castocc.dom[0] = p.dom[0]; g_castocc.dom[1] = p.dom[1]; g_castocc.dom[2] = p.dom[2];
    g_castocc.slack = p.slack;
    g_castocc.rdom = p.rdom;
    g_castocc.rng = p.rng;   // CASTOCC_RANGE.
    g_castocc.floor = p.floor;
    g_castocc.key = p.key;
    g_castocc.set.clear();
    g_castocc.set.reserve(static_cast<size_t>(p.n));

    for (int s = 0; s < p.n; ++s) {
        g_castocc.set.push_back(castocc_hash1(g_sun_maps.sun_locals.data() + static_cast<size_t>(s) * 6u));
    }

    std::sort(g_castocc.set.begin(), g_castocc.set.end());
}

inline bool cast_occ_build(ID3D11DeviceContext* ctx, ConstantData& cb) {
    const int n = static_cast<int>(cb.locality_meta[0]);
    if (n <= 0 || static_cast<size_t>(n) * 6u > g_sun_maps.sun_locals.size()) return false;
    // The domain: sunOrigin / mirMeta.w's sources (what the fill hands), not the CB. PSMaskCast fades about castCam
    // (the fire camera), a frame of motion away at most; the grid's camera slack and disc margin absorb the gap.
    const float co_rng = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    const float dom[3] = { g_sun_maps.sun_anchor_now[0], g_sun_maps.sun_anchor_now[1],
        g_sun_maps.sun_anchor_now[2] };
    const float* sun = cb.cast_view[2];   // The fire's frozen sun.
    // CASTOCC_LOWSUN: castocc_sun's floor. Every band built at |sun.y| < 0.02 has sy_lo = 0.02, so a fire below
    // it tested unclamped fails every band and rebuilds each frame; tested at 0.02 it keeps a band whose stamps cover
    // the rebuild's (same sy_lo, an sy_hi at least as high). A NaN stays NaN and still fails.
    float sy = fabsf(sun[1]);
    if (sy < 0.02f) sy = 0.02f;
    const float shl = sqrtf(sun[0] * sun[0] + sun[2] * sun[2]);
    const float ux = shl > 1.0e-4f ? (-sun[0] / shl) : 0.0f;
    const float uz = shl > 1.0e-4f ? (-sun[2] / shl) : 0.0f;
    uint64_t k;
    {
        if (g_castocc.kc_ser == g_sun_maps.sun_locals_serial && g_castocc.kc_rng == co_rng &&
            g_castocc.kc_n == n) {
            k = g_castocc.kc;
        } else {
            k = CryptoGenerator::fnv1a64_raw(g_sun_maps.sun_locals.data(),
                                             static_cast<size_t>(n) * 6u * sizeof(float));
            k = CryptoGenerator::fnv1a64_update(k, &co_rng, sizeof(co_rng));
            if (k == 0) k = 1;   // 0 is the empty sentinel.
            g_castocc.kc = k;
            g_castocc.kc_ser = g_sun_maps.sun_locals_serial;
            g_castocc.kc_rng = co_rng;
            g_castocc.kc_n = n;
        }
    }
    // CASTOCC_DOMAIN: does the retained envelope still cover this fire?
    const bool state = g_res.castocc_tex && g_res.castocc_srv && !g_castocc.set.empty() &&
                       g_castocc.slack > 0.0f && g_castocc.inv > 0.0f && g_castocc.cell > 0.0f &&
                       g_castocc.grid.size() == static_cast<size_t>(OCC_N) * OCC_N * 2u;
    const bool bucket = sy >= g_castocc.sunb.sy_lo && sy <= g_castocc.sunb.sy_hi;
    const bool az = (ux * g_castocc.sunb.ux + uz * g_castocc.sunb.uz) >= OCC_AZ_COS;
    const float dx = dom[0] - g_castocc.dom[0];
    const float dz = dom[2] - g_castocc.dom[2];
    const bool cam = dx * dx + dz * dz <= g_castocc.slack * g_castocc.slack;
    const bool cw_floor = dom[1] >= g_castocc.dom[1] - g_castocc.slack;
    // CASTOCC_RANGE: the disc and the floor were built for a range; a longer one reaches receivers past them.
    const bool cw_rng = co_rng <= g_castocc.rng;
    const bool env = state && bucket && az && cam && cw_floor && cw_rng;

    if (env && k == g_castocc.key) {
        castocc_pre_step(ctx, n, k, dom, sun, co_rng);   // CASTOCC_PRE: the next grid, a slice at a time; may commit.
        cb.cast_mat[0][3] = g_castocc.inv;   // The arm (> 0), with the fill's localityMeta.w.
        cb.cast_mat[1][3] = g_castocc.lo[0];
        cb.cast_mat[2][3] = g_castocc.lo[1];
        return true;
    }
    g_castocc.key = k;   // Provisional; cleared below on any failure.

    const bool fresh = g_castocc.set.size() <= static_cast<size_t>(n) +
                                               static_cast<size_t>(n) / OCC_STALE;
    if (env && fresh) {
        g_castocc.addi.clear();

        for (int ci_i = 0; ci_i < n; ++ci_i) {
            const uint64_t h = castocc_hash1(g_sun_maps.sun_locals.data() + static_cast<size_t>(ci_i) * 6u);

            if (!std::binary_search(g_castocc.set.begin(), g_castocc.set.end(), h)) {
                g_castocc.addi.push_back(static_cast<uint32_t>(ci_i));
            }
        }

        if (g_castocc.addi.size() <= static_cast<size_t>(OCC_ADD_MAX)) {
            // Additions are swept in the BUILT envelope: its sun band, its domain disc, its floor.
            bool ok = true;

            for (size_t j = 0; j < g_castocc.addi.size() && ok; ++j) {
                const float* ci_e = g_sun_maps.sun_locals.data() + static_cast<size_t>(g_castocc.addi[j]) * 6u;
                const OccSweep w = castocc_sweep(ci_e, g_castocc.sunb, g_castocc.dom,
                                                 g_castocc.rdom, g_castocc.floor);
                ok = castocc_fits(ci_e, g_castocc.sunb, w, g_castocc.lo,
                                  g_castocc.cell, g_castocc.inv);
            }

            if (ok) {
                for (size_t j = 0; j < g_castocc.addi.size(); ++j) {
                    const float* ci_e =
                        g_sun_maps.sun_locals.data() + static_cast<size_t>(g_castocc.addi[j]) * 6u;
                    const OccSweep w = castocc_sweep(ci_e, g_castocc.sunb, g_castocc.dom,
                                                     g_castocc.rdom, g_castocc.floor);
                    castocc_stamp(ci_e, g_castocc.sunb, w, g_castocc.lo,
                                  g_castocc.cell, g_castocc.inv, g_castocc.grid.data());
                }

                if (g_castocc.addi.empty() || castocc_upload(ctx)) {
                    const size_t was = g_castocc.set.size();

                    for (size_t j = 0; j < g_castocc.addi.size(); ++j) {
                        g_castocc.set.push_back(
                            castocc_hash1(g_sun_maps.sun_locals.data() +
                                          static_cast<size_t>(g_castocc.addi[j]) * 6u));
                    }

                    std::sort(g_castocc.set.begin() + was, g_castocc.set.end());
                    std::inplace_merge(g_castocc.set.begin(),
                                       g_castocc.set.begin() + was, g_castocc.set.end());

                    cb.cast_mat[0][3] = g_castocc.inv;
                    cb.cast_mat[1][3] = g_castocc.lo[0];
                    cb.cast_mat[2][3] = g_castocc.lo[1];
                    return true;
                }

                return castocc_fail();
            }
        }
    }
    // Full rebuild about THIS fire's envelope.
    if (g_castocc.pre.active) {   // CASTOCC_PRE: outrun; the rebuild is the truth now.
        g_castocc.pre.active = false;
    }
    const OccSun sb = castocc_sun(sun);
    const float slack = castocc_slack_of(co_rng);
    const float rdom = 0.995f * co_rng + slack;
    const float co_floor = dom[1] - co_rng - slack;   // CASTOCC_FLOOR: the deepest receiver.
    float lo[2] = { 1.0e30f, 1.0e30f };
    float hi[2] = { -1.0e30f, -1.0e30f };
    static std::vector<OccSweep> sw;   // Per caster (render-thread scratch).
    sw.resize(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        const float* e = g_sun_maps.sun_locals.data() + static_cast<size_t>(i) * 6u;
        sw[static_cast<size_t>(i)] = castocc_sweep(e, sb, dom, rdom, co_floor);
        const OccSweep& w = sw[static_cast<size_t>(i)];
        if (!w.on) continue;
        float b0[2], b1[2];
        castocc_box(e, sb, w, b0, b1);
        if (b0[0] < lo[0]) lo[0] = b0[0];
        if (b0[1] < lo[1]) lo[1] = b0[1];
        if (b1[0] > hi[0]) hi[0] = b1[0];
        if (b1[1] > hi[1]) hi[1] = b1[1];
    }
    if (lo[0] > hi[0]) return castocc_fail();   // No sweep reaches the domain: the
                                                             // shader's grid arm stays 0.
    {   // CASTOCC_PAD: rim headroom so an arriving caster can still be an
        // addition instead of a rebuild.
        const float g0 = (hi[0] - lo[0]) * OCC_EXTENT_PAD;
        const float g1 = (hi[1] - lo[1]) * OCC_EXTENT_PAD;
        lo[0] -= g0; hi[0] += g0;
        lo[1] -= g1; hi[1] += g1;
    }
    const float sp0 = hi[0] - lo[0];
    const float sp1 = hi[1] - lo[1];
    // CASTOCC_SPAN: an infinite span (a caster size near FLT_MAX overflows its sweep) makes cell inf and inv 0, and
    // every stamp converts NaN to int. Below this bound each stamp term is a bounded number of cells.
    if (!(sp0 > 0.0f && sp0 <= 1.0e30f) || !(sp1 > 0.0f && sp1 <= 1.0e30f)) {
        return castocc_fail();
    }
    float cell = (sp0 > sp1 ? sp0 : sp1) / OCC_N;
    if (!(cell > 1.0e-4f)) cell = 1.0e-4f;
    const float inv = 1.0f / cell;

    g_castocc.grid.resize(static_cast<size_t>(OCC_N) * OCC_N * 2u);   // CASTOCC_INCR: retained.
    for (size_t g = 0; g < g_castocc.grid.size(); g += 2) { g_castocc.grid[g] = 1.0e30f;
        g_castocc.grid[g + 1] = -1.0e30f; }
    for (int i = 0; i < n; ++i) {
        const float* e = g_sun_maps.sun_locals.data() + static_cast<size_t>(i) * 6u;
        castocc_stamp(e, sb, sw[static_cast<size_t>(i)], lo, cell, inv,
                      g_castocc.grid.data());
    }

    if (!g_res.castocc_tex) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return castocc_fail();
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = OCC_N;
        td.Height = OCC_N;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32G32_FLOAT;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_DYNAMIC;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        const bool ok =
            SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.castocc_tex)) &&
            SUCCEEDED(dev->CreateShaderResourceView(g_res.castocc_tex, nullptr, &g_res.castocc_srv));
        if (!ok) { KH_SAFE_RELEASE(g_res.castocc_srv); KH_SAFE_RELEASE(g_res.castocc_tex); }
        dev->Release();
        if (!g_res.castocc_srv) return castocc_fail();
    }
    if (!castocc_upload(ctx)) return castocc_fail();

    cb.cast_mat[0][3] = inv;
    cb.cast_mat[1][3] = lo[0];
    cb.cast_mat[2][3] = lo[1];
    g_castocc.lo[0] = lo[0];
    g_castocc.lo[1] = lo[1];
    g_castocc.inv = inv;
    g_castocc.cell = cell;
    g_castocc.sunb = sb;   // The envelope, committed with the grid.
    g_castocc.dom[0] = dom[0]; g_castocc.dom[1] = dom[1]; g_castocc.dom[2] = dom[2];
    g_castocc.slack = slack;
    g_castocc.rdom = rdom;
    g_castocc.rng = co_rng;   // CASTOCC_RANGE.
    g_castocc.floor = co_floor;
    g_castocc.set.clear();
    g_castocc.set.reserve(static_cast<size_t>(n));

    for (int s = 0; s < n; ++s) {
        g_castocc.set.push_back(castocc_hash1(g_sun_maps.sun_locals.data() + static_cast<size_t>(s) * 6u));
    }

    std::sort(g_castocc.set.begin(), g_castocc.set.end());
    return true;
}
}