#pragma once

// The injection half (rendering_integration_injection.hpp): the cast occlusion (KH_OCC): the sweep, the stamps and the
// build. Never include it on its own.
namespace RenderIntegration {

// KH_CAST_OCC: the caster-occupancy grid. PSMaskCast's reach test ("is this world point within reach of any caster")
// reads one cell instead of looping every caster per pixel. A cell holds the union of the casters' stamped sweeps over
// its area, so it refuses only points no caster's shadow reaches - no shadow is lost (SunShadowOcclusion decides), only
// more pixels reach the expensive path. It is not a superset of that loop's inflated reach (below).
static constexpr int KH_OCC_N = 128;
// KH_CASTOCC_DOMAIN: the grid covers the RECEIVER domain (the pixels KhSunRangeFade leaves non-zero: a vertical
// cylinder of radius 0.995 * range about the camera, +-0.995 * range tall), not the caster set. floor = camera.y
// - range - slack (the deepest receiver, wherever the camera moves within its slack). Each sweep is clipped to the
// domain disc before stamping, so the extent is ~2 * (range + slack) whatever the caster set. The grid is stamped for a
// BAND of suns (elevation [syq - 1/64, syq + 2/64], +-0.5 deg azimuth) so a slowly moving sun does not force a rebuild
// at every bucket edge, and the camera may move 0.5 R (clamped to 40 - 250 m) from the built centre before one; a
// range longer than the one it was built for rebuilds too (KH_CASTOCC_RANGE). The <= 16 caster loop (KhCastReach)
// keeps its own inflated reach; the grid is a superset of the truth, not of that loop.
static constexpr float KH_OCC_SY_Q = 64.0f;
static constexpr float KH_OCC_SLACK_M = 6.0f;        // Around the exact sweep, lateral and vertical.
static constexpr float KH_OCC_AZ_TAN = 0.0087269f;   // tan(0.5 deg): widening and hysteresis.
static constexpr float KH_OCC_AZ_COS = 0.9999619f;   // cos(0.5 deg).
static constexpr float KH_OCC_CAM_SLACK = 0.50f;     // Of the range; clamped [40, 250] m.
static uint64_t g_castocc_key = 0;   // FNV-1a over (locals, range) of the last build (0 = none).
static float    g_castocc_lo[2] = {};   // Retained grid origin XZ (engine axes).
static float    g_castocc_inv = 0.0f;   // Retained 1 / cell.
static constexpr int KH_OCC_ADD_MAX = 256;      // Additions worth stamping one by one.
static constexpr int KH_OCC_STALE = 4;          // Rebuild past built > n + n/KH_OCC_STALE.
static constexpr float KH_OCC_EXTENT_PAD = 0.10f;
static std::vector<float>    g_castocc_grid;    // (minY, maxY) per texel, retained.
static std::vector<uint64_t> g_castocc_set;     // Sorted per-caster hashes of the built set.
static std::vector<uint32_t> g_castocc_addi;    // Scratch: indices of the additions.
static float    g_castocc_cell = 0.0f;
// The sun band the grid was stamped for, derived once per build from the fire's frozen sun and retained with the grid.
struct KhOccSun {
    float sy_lo, sy_hi;       // Elevation band, sun.y.
    float k_hi;               // Metres down-sun per metre of drop at the SHALLOWEST sun.
    float slope_lo, slope_hi; // Drop per metre down-sun: shallowest, steepest.
    float ux, uz;             // Down-sun unit (xz) at the band centre.
};
// The built envelope: what the retained grid still covers.
static KhOccSun g_castocc_sunb = {};
static float    g_castocc_dom[3] = {};      // Domain centre (engine axes) the grid was built about.
static float    g_castocc_slack = 0.0f;     // Camera slack (m); 0 = no retained grid.
static float    g_castocc_rdom = 0.0f;      // Disc radius the sweeps were clipped to.
static float    g_castocc_rng = 0.0f;       // KH_CASTOCC_RANGE: the shadow range the disc and floor were built for.
static float    g_castocc_floor = 0.0f;     // Receiver floor the sweeps were marched to.

inline uint64_t kh_castocc_hash1(const float* khch_e) {
    return CryptoGenerator::fnv1a64_raw(khch_e, 6u * sizeof(float));
}

inline KhOccSun kh_castocc_sun(const float* khos_sun) {   // Toward the light.
    KhOccSun khos_o;
    const float khos_sy = fabsf(khos_sun[1]);
    const float khos_hl = sqrtf(khos_sun[0] * khos_sun[0] + khos_sun[2] * khos_sun[2]);
    const float khos_syq = floorf(khos_sy * KH_OCC_SY_Q) / KH_OCC_SY_Q;
    khos_o.sy_lo = khos_syq - 1.0f / KH_OCC_SY_Q;
    if (khos_o.sy_lo < 0.02f) khos_o.sy_lo = 0.02f;
    khos_o.sy_hi = khos_syq + 2.0f / KH_OCC_SY_Q;
    if (khos_o.sy_hi > 1.0f) khos_o.sy_hi = 1.0f;
    const float khos_hl_hi = sqrtf(fmaxf(1.0f - khos_o.sy_lo * khos_o.sy_lo, 0.0f));
    const float khos_hl_lo = sqrtf(fmaxf(1.0f - khos_o.sy_hi * khos_o.sy_hi, 0.0f));
    khos_o.k_hi = khos_hl_hi / khos_o.sy_lo;
    khos_o.slope_lo = khos_o.sy_lo / fmaxf(khos_hl_hi, 1.0e-6f);
    khos_o.slope_hi = khos_o.sy_hi / fmaxf(khos_hl_lo, 1.0e-6f);
    khos_o.ux = khos_hl > 1.0e-4f ? (-khos_sun[0] / khos_hl) : 0.0f;
    khos_o.uz = khos_hl > 1.0e-4f ? (-khos_sun[2] / khos_hl) : 0.0f;
    return khos_o;
}

inline float kh_castocc_slack_of(float khcs_r) {
    float khcs_s = KH_OCC_CAM_SLACK * khcs_r;
    if (khcs_s < 40.0f) khcs_s = 40.0f;
    if (khcs_s > 250.0f) khcs_s = 250.0f;
    return khcs_s;
}

// One caster's sweep, clipped to the domain disc. on = false means no part of the swept shadow can reach a receiver the
// fade keeps: neither stamped nor counted in the extent.
struct KhOccSweep { float hu, top, base, ta, tb, w0; bool on; };
// KH_CASTOCC_PRE: a full rebuild fires the moment the camera leaves the built slack disc (a hitch frame). The rebuild
// is a pure function of (locals, range, domain, sun band, floor), so once the camera is past KH_OCC_PRE_START of the
// slack a second grid is built about the CURRENT camera, KH_OCC_PRE_PER_FIRE casters per fire, and committed at the
// next fire its envelope covers. A camera that outruns it takes the synchronous rebuild; a pending grid whose inputs no
// longer fit is dropped.
static constexpr float KH_OCC_PRE_START = 0.5f;   // Of the slack: start the pre-build here.
static constexpr int   KH_OCC_PRE_PER_FIRE = 192; // Casters stamped per fire (~0.5 ms at 3,000).
struct KhOccPending {
    bool     active = false;
    bool     done = false;
    uint64_t key = 0;
    int      n = 0;
    int      next = 0;
    KhOccSun sb = {};
    float    dom[3] = {};
    float    slack = 0.0f, rdom = 0.0f, floor = 0.0f, rng = 0.0f;
    float    lo[2] = {};
    float    cell = 0.0f, inv = 0.0f;
    std::vector<KhOccSweep> sw;
    std::vector<float>      grid;
};
static KhOccPending g_castocc_pre;
static bool     g_castocc_pre_noreach = false;   // Phase 0 found no sweep in the pending domain...
static float    g_castocc_pre_noreach_dom[3] = {};   // ...about this domain; retry once the camera moves.

inline KhOccSweep kh_castocc_sweep(const float* khcw_e, const KhOccSun& khcw_s,
                                   const float* khcw_dom, float khcw_rdom, float khcw_floor) {
    KhOccSweep khcw_o = {};
    const float khcw_hx = khcw_e[3], khcw_hy = khcw_e[4], khcw_hz = khcw_e[5];
    khcw_o.hu = fabsf(khcw_s.ux) * khcw_hx + fabsf(khcw_s.uz) * khcw_hz;   // Box half-extent along u.
    khcw_o.top = khcw_e[1] + khcw_hy;
    khcw_o.base = khcw_e[1] - khcw_hy;
    // The top ray reaches the floor at (top - floor) * k, plus the box's own down-sun face.
    const float khcw_drop = khcw_o.top - khcw_floor;
    const float khcw_al = (khcw_drop > 0.0f ? khcw_drop : 0.0f) * khcw_s.k_hi + khcw_o.hu;
    khcw_o.w0 = (khcw_hx > khcw_hz ? khcw_hx : khcw_hz) + KH_OCC_SLACK_M;
    const float khcw_wmax = khcw_o.w0 + (khcw_al + khcw_o.hu) * KH_OCC_AZ_TAN;
    // Disc clip: |c + u t - dom| <= rdom + wmax * sqrt(2), a quadratic in t.
    const float khcw_qx = khcw_e[0] - khcw_dom[0];
    const float khcw_qz = khcw_e[2] - khcw_dom[2];
    const float khcw_rc = khcw_rdom + khcw_wmax * 1.4142136f;
    const float khcw_b = 2.0f * (khcw_qx * khcw_s.ux + khcw_qz * khcw_s.uz);
    const float khcw_c = khcw_qx * khcw_qx + khcw_qz * khcw_qz - khcw_rc * khcw_rc;
    const float khcw_disc = khcw_b * khcw_b - 4.0f * khcw_c;
    if (khcw_disc < 0.0f) return khcw_o;   // The sweep line misses the domain.
    const float khcw_rt = sqrtf(khcw_disc);
    const float khcw_tin = (-khcw_b - khcw_rt) * 0.5f;
    const float khcw_tout = (-khcw_b + khcw_rt) * 0.5f;
    khcw_o.ta = -khcw_o.hu > khcw_tin ? -khcw_o.hu : khcw_tin;
    khcw_o.tb = khcw_al < khcw_tout ? khcw_al : khcw_tout;
    khcw_o.on = khcw_o.ta <= khcw_o.tb;
    return khcw_o;
}

// The XZ box a sweep's stamps will touch, for the extent and the fit test.
inline void kh_castocc_box(const float* khcb_e, const KhOccSun& khcb_s, const KhOccSweep& khcb_w,
                           float khcb_lo[2], float khcb_hi[2]) {
    const float khcb_wa = khcb_w.w0 + (khcb_w.tb + khcb_w.hu) * KH_OCC_AZ_TAN;
    const float khcb_ex = khcb_e[3] + KH_OCC_SLACK_M + khcb_wa;
    const float khcb_ez = khcb_e[5] + KH_OCC_SLACK_M + khcb_wa;
    const float khcb_ax = khcb_e[0] + khcb_s.ux * khcb_w.ta, khcb_az = khcb_e[2] + khcb_s.uz * khcb_w.ta;
    const float khcb_bx = khcb_e[0] + khcb_s.ux * khcb_w.tb, khcb_bz = khcb_e[2] + khcb_s.uz * khcb_w.tb;
    khcb_lo[0] = (khcb_ax < khcb_bx ? khcb_ax : khcb_bx) - khcb_ex;
    khcb_hi[0] = (khcb_ax > khcb_bx ? khcb_ax : khcb_bx) + khcb_ex;
    khcb_lo[1] = (khcb_az < khcb_bz ? khcb_az : khcb_bz) - khcb_ez;
    khcb_hi[1] = (khcb_az > khcb_bz ? khcb_az : khcb_bz) + khcb_ez;
}

inline bool kh_castocc_fits(const float* khcf_e, const KhOccSun& khcf_s, const KhOccSweep& khcf_w,
                            const float* khcf_lo, float khcf_cell, float khcf_inv) {
    if (!khcf_w.on) return true;   // Nothing to stamp always fits.
    float khcf_b0[2], khcf_b1[2];
    kh_castocc_box(khcf_e, khcf_s, khcf_w, khcf_b0, khcf_b1);
    const float khcf_gx = (khcf_b0[0] - khcf_cell - khcf_lo[0]) * khcf_inv;
    const float khcf_gz = (khcf_b0[1] - khcf_cell - khcf_lo[1]) * khcf_inv;
    const float khcf_gx1 = (khcf_b1[0] + khcf_cell - khcf_lo[0]) * khcf_inv;
    const float khcf_gz1 = (khcf_b1[1] + khcf_cell - khcf_lo[1]) * khcf_inv;
    return khcf_gx >= 0.0f && khcf_gz >= 0.0f &&
           khcf_gx1 <= static_cast<float>(KH_OCC_N) - 1.0f &&
           khcf_gz1 <= static_cast<float>(KH_OCC_N) - 1.0f;
}

// Stamp one clipped sweep. Steps every max(w0, cell) along u from ta to tb; each step writes a block of +-(h + slack
// + pad + widening) cells whose y window is the exact shadow band over the step's t range at the band's extreme slopes.
// The last step is open-bottomed: past the floor there is no receiver.
inline void kh_castocc_stamp(const float* khcs_e, const KhOccSun& khcs_s, const KhOccSweep& khcs_w,
                             const float* khcs_lo, float khcs_cell, float khcs_inv,
                             float* khcs_grid) {   // KH_CASTOCC_PRE: the retained or the pending grid.
    if (!khcs_w.on) return;
    const float khcs_hx = khcs_e[3], khcs_hz = khcs_e[5];
    const float khcs_dtw = khcs_w.w0 > khcs_cell ? khcs_w.w0 : khcs_cell;   // KH_OCC_COST: one cell per step.
    int khcs_ns = static_cast<int>(ceilf((khcs_w.tb - khcs_w.ta) / (khcs_dtw > 1.0e-4f ? khcs_dtw : 1.0e-4f)));
    if (khcs_ns < 0) khcs_ns = 0;
    if (khcs_ns > 4 * KH_OCC_N) khcs_ns = 4 * KH_OCC_N;
    const float khcs_dt = khcs_ns > 0 ? ((khcs_w.tb - khcs_w.ta) / static_cast<float>(khcs_ns)) : 0.0f;
    const float khcs_pad = khcs_cell > 0.5f * khcs_dt ? khcs_cell : 0.5f * khcs_dt;

    for (int khcs_si = 0; khcs_si <= khcs_ns; ++khcs_si) {
        const float khcs_t = khcs_w.ta + khcs_dt * static_cast<float>(khcs_si);
        float khcs_wid = (khcs_t + khcs_dt + khcs_pad + khcs_w.hu) * KH_OCC_AZ_TAN;
        if (khcs_wid < 0.0f) khcs_wid = 0.0f;
        const float khcs_cx = khcs_e[0] + khcs_s.ux * khcs_t;
        const float khcs_cz = khcs_e[2] + khcs_s.uz * khcs_t;
        const float khcs_rx = khcs_hx + KH_OCC_SLACK_M + khcs_pad + khcs_wid;
        const float khcs_rz = khcs_hz + KH_OCC_SLACK_M + khcs_pad + khcs_wid;
        int khcs_x0 = static_cast<int>(floorf((khcs_cx - khcs_rx - khcs_lo[0]) * khcs_inv));
        int khcs_x1 = static_cast<int>(floorf((khcs_cx + khcs_rx - khcs_lo[0]) * khcs_inv));
        int khcs_z0 = static_cast<int>(floorf((khcs_cz - khcs_rz - khcs_lo[1]) * khcs_inv));
        int khcs_z1 = static_cast<int>(floorf((khcs_cz + khcs_rz - khcs_lo[1]) * khcs_inv));
        if (khcs_x0 < 0) khcs_x0 = 0;
        if (khcs_z0 < 0) khcs_z0 = 0;
        if (khcs_x1 > KH_OCC_N - 1) khcs_x1 = KH_OCC_N - 1;
        if (khcs_z1 > KH_OCC_N - 1) khcs_z1 = KH_OCC_N - 1;
        if (khcs_x1 < khcs_x0 || khcs_z1 < khcs_z0) continue;
        const float khcs_y0 = (khcs_si == khcs_ns)
                            ? -1.0e30f
                            : khcs_w.base - (khcs_t + khcs_dt + khcs_pad + khcs_w.hu) * khcs_s.slope_hi
                              - KH_OCC_SLACK_M;
        float khcs_tt = khcs_t - khcs_pad - khcs_w.hu;
        if (khcs_tt < 0.0f) khcs_tt = 0.0f;
        const float khcs_y1 = khcs_w.top - khcs_tt * khcs_s.slope_lo + KH_OCC_SLACK_M + 1.0e-3f;

        for (int khcs_z = khcs_z0; khcs_z <= khcs_z1; ++khcs_z) {
            float* khcs_row = khcs_grid + (static_cast<size_t>(khcs_z) * KH_OCC_N) * 2u;

            for (int khcs_x = khcs_x0; khcs_x <= khcs_x1; ++khcs_x) {
                if (khcs_y0 < khcs_row[khcs_x * 2]) khcs_row[khcs_x * 2] = khcs_y0;
                if (khcs_y1 > khcs_row[khcs_x * 2 + 1]) khcs_row[khcs_x * 2 + 1] = khcs_y1;
            }
        }
    }
}

inline bool kh_castocc_upload(ID3D11DeviceContext* khcu_ctx) {
    D3D11_MAPPED_SUBRESOURCE khcu_m = {};
    if (FAILED(khcu_ctx->Map(g_res.castocc_tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &khcu_m))) return false;

    for (int khcu_z = 0; khcu_z < KH_OCC_N; ++khcu_z) {
        memcpy(static_cast<uint8_t*>(khcu_m.pData) + static_cast<size_t>(khcu_z) * khcu_m.RowPitch,
               g_castocc_grid.data() + static_cast<size_t>(khcu_z) * KH_OCC_N * 2u,
               static_cast<size_t>(KH_OCC_N) * 2u * sizeof(float));
    }

    khcu_ctx->Unmap(g_res.castocc_tex, 0);
    return true;
}

// Every failure return leaves NO retained grid (a rewritten grid under stale lo/inv/cell is a live hazard).
inline bool kh_castocc_fail() {
    g_castocc_pre.active = false;   // KH_CASTOCC_PRE.
    g_castocc_key = 0;
    g_castocc_set.clear();
    g_castocc_slack = 0.0f;
    return false;
}

// One fire's share of the pre-build, on the hit path. Starts a pending grid once the camera is past KH_OCC_PRE_START of
// the slack, stamps KH_OCC_PRE_PER_FIRE casters per call, and commits the completed grid on the first fire its own
// envelope covers. Every quantity is the rebuild's own arithmetic about the pending domain.
inline void kh_castocc_pre_step(ID3D11DeviceContext* khpp_ctx, int khpp_n, uint64_t khpp_key,
                                const float* khpp_dom, const float* khpp_sun, float khpp_rng) {
    KhOccPending& khpp_p = g_castocc_pre;

    if (khpp_p.active && (khpp_p.key != khpp_key || khpp_p.n != khpp_n)) {   // The locals moved.
        khpp_p.active = false;
    }

    if (!khpp_p.active) {
        const float khpp_dx = khpp_dom[0] - g_castocc_dom[0];
        const float khpp_dz = khpp_dom[2] - g_castocc_dom[2];
        const float khpp_far = KH_OCC_PRE_START * g_castocc_slack;
        if (khpp_dx * khpp_dx + khpp_dz * khpp_dz <= khpp_far * khpp_far &&
            khpp_dom[1] >= g_castocc_dom[1] - khpp_far) {
            return;   // The retained grid has room to spare.
        }
        if (g_castocc_pre_noreach) {   // A no-reach attempt is not retried until the camera moves.
            const float khpp_nx = khpp_dom[0] - g_castocc_pre_noreach_dom[0];
            const float khpp_ny = khpp_dom[1] - g_castocc_pre_noreach_dom[1];
            const float khpp_nz = khpp_dom[2] - g_castocc_pre_noreach_dom[2];
            const float khpp_nr = 0.1f * g_castocc_slack;
            if (khpp_nx * khpp_nx + khpp_ny * khpp_ny + khpp_nz * khpp_nz <= khpp_nr * khpp_nr) return;
            g_castocc_pre_noreach = false;
        }

        // Phase 0: the sweeps and the extent about the pending domain.
        khpp_p.key = khpp_key;
        khpp_p.n = khpp_n;
        khpp_p.sb = kh_castocc_sun(khpp_sun);
        khpp_p.slack = kh_castocc_slack_of(khpp_rng);
        khpp_p.rdom = 0.995f * khpp_rng + khpp_p.slack;
        khpp_p.floor = khpp_dom[1] - khpp_rng - khpp_p.slack;
        khpp_p.rng = khpp_rng;
        khpp_p.dom[0] = khpp_dom[0]; khpp_p.dom[1] = khpp_dom[1]; khpp_p.dom[2] = khpp_dom[2];
        float khpp_lo[2] = { 1.0e30f, 1.0e30f };
        float khpp_hi[2] = { -1.0e30f, -1.0e30f };
        khpp_p.sw.resize(static_cast<size_t>(khpp_n));

        for (int i = 0; i < khpp_n; ++i) {
            const float* e = g_sun_locals.data() + static_cast<size_t>(i) * 6u;
            khpp_p.sw[static_cast<size_t>(i)] = kh_castocc_sweep(e, khpp_p.sb, khpp_p.dom, khpp_p.rdom, khpp_p.floor);
            const KhOccSweep& khpp_w = khpp_p.sw[static_cast<size_t>(i)];
            if (!khpp_w.on) continue;
            float khpp_b0[2], khpp_b1[2];
            kh_castocc_box(e, khpp_p.sb, khpp_w, khpp_b0, khpp_b1);
            if (khpp_b0[0] < khpp_lo[0]) khpp_lo[0] = khpp_b0[0];
            if (khpp_b0[1] < khpp_lo[1]) khpp_lo[1] = khpp_b0[1];
            if (khpp_b1[0] > khpp_hi[0]) khpp_hi[0] = khpp_b1[0];
            if (khpp_b1[1] > khpp_hi[1]) khpp_hi[1] = khpp_b1[1];
        }

        if (khpp_lo[0] > khpp_hi[0]) {   // No sweep reaches the pending domain: nothing to pre-build.
            g_castocc_pre_noreach = true;
            g_castocc_pre_noreach_dom[0] = khpp_dom[0]; g_castocc_pre_noreach_dom[1] = khpp_dom[1]; g_castocc_pre_noreach_dom[2] = khpp_dom[2];
            return;
        }
        {   // KH_CASTOCC_PAD, as the rebuild pads.
            const float khpp_g0 = (khpp_hi[0] - khpp_lo[0]) * KH_OCC_EXTENT_PAD;
            const float khpp_g1 = (khpp_hi[1] - khpp_lo[1]) * KH_OCC_EXTENT_PAD;
            khpp_lo[0] -= khpp_g0; khpp_hi[0] += khpp_g0;
            khpp_lo[1] -= khpp_g1; khpp_hi[1] += khpp_g1;
        }
        const float khpp_sp0 = khpp_hi[0] - khpp_lo[0];
        const float khpp_sp1 = khpp_hi[1] - khpp_lo[1];
        // KH_CASTOCC_SPAN: the rebuild's guard (below).
        if (!(khpp_sp0 > 0.0f && khpp_sp0 <= 1.0e30f) || !(khpp_sp1 > 0.0f && khpp_sp1 <= 1.0e30f)) return;
        float khpp_cell = (khpp_sp0 > khpp_sp1 ? khpp_sp0 : khpp_sp1) / KH_OCC_N;
        if (!(khpp_cell > 1.0e-4f)) khpp_cell = 1.0e-4f;
        khpp_p.lo[0] = khpp_lo[0]; khpp_p.lo[1] = khpp_lo[1];
        khpp_p.cell = khpp_cell;
        khpp_p.inv = 1.0f / khpp_cell;
        khpp_p.grid.resize(static_cast<size_t>(KH_OCC_N) * KH_OCC_N * 2u);
        for (size_t g = 0; g < khpp_p.grid.size(); g += 2) { khpp_p.grid[g] = 1.0e30f; khpp_p.grid[g + 1] = -1.0e30f; }
        khpp_p.next = 0;
        khpp_p.done = false;
        khpp_p.active = true;
        return;   // The stamping begins next fire.
    }

    if (!khpp_p.done) {   // Phase 1: a slice of the stamps.
        int khpp_end = khpp_p.next + KH_OCC_PRE_PER_FIRE;
        if (khpp_end > khpp_p.n) khpp_end = khpp_p.n;

        for (int i = khpp_p.next; i < khpp_end; ++i) {
            const float* e = g_sun_locals.data() + static_cast<size_t>(i) * 6u;
            kh_castocc_stamp(e, khpp_p.sb, khpp_p.sw[static_cast<size_t>(i)], khpp_p.lo,
                             khpp_p.cell, khpp_p.inv, khpp_p.grid.data());
        }

        khpp_p.next = khpp_end;
        if (khpp_p.next >= khpp_p.n) khpp_p.done = true;
        if (!khpp_p.done) return;
    }

    // Complete: commit only into an envelope that covers THIS fire.
    {
        float khpp_sy = fabsf(khpp_sun[1]);
        if (khpp_sy < 0.02f) khpp_sy = 0.02f;   // KH_CASTOCC_LOWSUN (the rebuild's test below).
        const float khpp_shl = sqrtf(khpp_sun[0] * khpp_sun[0] + khpp_sun[2] * khpp_sun[2]);
        const float khpp_ux = khpp_shl > 1.0e-4f ? (-khpp_sun[0] / khpp_shl) : 0.0f;
        const float khpp_uz = khpp_shl > 1.0e-4f ? (-khpp_sun[2] / khpp_shl) : 0.0f;
        const bool khpp_bucket = khpp_sy >= khpp_p.sb.sy_lo && khpp_sy <= khpp_p.sb.sy_hi;
        const bool khpp_az = (khpp_ux * khpp_p.sb.ux + khpp_uz * khpp_p.sb.uz) >= KH_OCC_AZ_COS;
        const float khpp_dx = khpp_dom[0] - khpp_p.dom[0];
        const float khpp_dz = khpp_dom[2] - khpp_p.dom[2];
        const bool khpp_cam = khpp_dx * khpp_dx + khpp_dz * khpp_dz <= khpp_p.slack * khpp_p.slack;
        const bool khpp_floor = khpp_dom[1] >= khpp_p.dom[1] - khpp_p.slack;

        if (!(khpp_bucket && khpp_az && khpp_cam && khpp_floor)) {
            khpp_p.active = false;
            return;
        }
    }

    g_castocc_grid.swap(khpp_p.grid);   // The pending grid becomes the retained one.
    khpp_p.active = false;

    if (!kh_castocc_upload(khpp_ctx)) {   // The rebuild's own failure rule: no retained grid.
        kh_castocc_fail();
        return;
    }

    g_castocc_lo[0] = khpp_p.lo[0];
    g_castocc_lo[1] = khpp_p.lo[1];
    g_castocc_inv = khpp_p.inv;
    g_castocc_cell = khpp_p.cell;
    g_castocc_sunb = khpp_p.sb;
    g_castocc_dom[0] = khpp_p.dom[0]; g_castocc_dom[1] = khpp_p.dom[1]; g_castocc_dom[2] = khpp_p.dom[2];
    g_castocc_slack = khpp_p.slack;
    g_castocc_rdom = khpp_p.rdom;
    g_castocc_rng = khpp_p.rng;   // KH_CASTOCC_RANGE.
    g_castocc_floor = khpp_p.floor;
    g_castocc_key = khpp_p.key;
    g_castocc_set.clear();
    g_castocc_set.reserve(static_cast<size_t>(khpp_p.n));

    for (int khpp_s = 0; khpp_s < khpp_p.n; ++khpp_s) {
        g_castocc_set.push_back(kh_castocc_hash1(g_sun_locals.data() + static_cast<size_t>(khpp_s) * 6u));
    }

    std::sort(g_castocc_set.begin(), g_castocc_set.end());
}

// KH_SESSION_RESET: at file scope, not in the function below, so the session reset reaches it.
static uint64_t g_castocc_kc = 0;
static uint64_t g_castocc_kc_ser = ~0ull;
static float    g_castocc_kc_rng = -1.0f;
static int      g_castocc_kc_n = -1;
inline bool kh_cast_occ_build(ID3D11DeviceContext* khco_ctx, ConstantData& khco_cb) {
    const int khco_n = static_cast<int>(khco_cb.locality_meta[0]);
    if (khco_n <= 0 || static_cast<size_t>(khco_n) * 6u > g_sun_locals.size()) return false;
    // The domain: sunOrigin / mirMeta.w's sources (what the fill hands), not the CB. PSMaskCast fades about castCam
    // (the fire camera), a frame of motion away at most; the grid's camera slack and disc margin absorb the gap.
    const float khco_rng = fminf(fmaxf(g_sun_range.load(std::memory_order_relaxed), 8.0f), 1000.0f);
    const float khco_dom[3] = { g_sun_anchor_now[0], g_sun_anchor_now[1], g_sun_anchor_now[2] };
    const float* khco_sun = khco_cb.cast_view[2];   // The fire's frozen sun.
    // KH_CASTOCC_LOWSUN: kh_castocc_sun's floor. Every band built at |sun.y| < 0.02 has sy_lo = 0.02, so a fire below
    // it tested unclamped fails every band and rebuilds each frame; tested at 0.02 it keeps a band whose stamps cover
    // the rebuild's (same sy_lo, an sy_hi at least as high). A NaN stays NaN and still fails.
    float khco_sy = fabsf(khco_sun[1]);
    if (khco_sy < 0.02f) khco_sy = 0.02f;
    const float khco_shl = sqrtf(khco_sun[0] * khco_sun[0] + khco_sun[2] * khco_sun[2]);
    const float khco_ux = khco_shl > 1.0e-4f ? (-khco_sun[0] / khco_shl) : 0.0f;
    const float khco_uz = khco_shl > 1.0e-4f ? (-khco_sun[2] / khco_shl) : 0.0f;
    uint64_t khco_k;
    {
        if (g_castocc_kc_ser == g_sun_locals_serial && g_castocc_kc_rng == khco_rng && g_castocc_kc_n == khco_n) {
            khco_k = g_castocc_kc;
        } else {
            khco_k = CryptoGenerator::fnv1a64_raw(g_sun_locals.data(),
                                                  static_cast<size_t>(khco_n) * 6u * sizeof(float));
            khco_k = CryptoGenerator::fnv1a64_update(khco_k, &khco_rng, sizeof(khco_rng));
            if (khco_k == 0) khco_k = 1;   // 0 is the empty sentinel.
            g_castocc_kc = khco_k;
            g_castocc_kc_ser = g_sun_locals_serial;
            g_castocc_kc_rng = khco_rng;
            g_castocc_kc_n = khco_n;
        }
    }
    // KH_CASTOCC_DOMAIN: does the retained envelope still cover this fire?
    const bool khcw_state = g_res.castocc_tex && g_res.castocc_srv && !g_castocc_set.empty() &&
                            g_castocc_slack > 0.0f && g_castocc_inv > 0.0f && g_castocc_cell > 0.0f &&
                            g_castocc_grid.size() == static_cast<size_t>(KH_OCC_N) * KH_OCC_N * 2u;
    const bool khcw_bucket = khco_sy >= g_castocc_sunb.sy_lo && khco_sy <= g_castocc_sunb.sy_hi;
    const bool khcw_az = (khco_ux * g_castocc_sunb.ux + khco_uz * g_castocc_sunb.uz) >= KH_OCC_AZ_COS;
    const float khcw_dx = khco_dom[0] - g_castocc_dom[0];
    const float khcw_dz = khco_dom[2] - g_castocc_dom[2];
    const bool khcw_cam = khcw_dx * khcw_dx + khcw_dz * khcw_dz <= g_castocc_slack * g_castocc_slack;
    const bool khcw_floor = khco_dom[1] >= g_castocc_dom[1] - g_castocc_slack;
    // KH_CASTOCC_RANGE: the disc and the floor were built for a range; a longer one reaches receivers past them.
    const bool khcw_rng = khco_rng <= g_castocc_rng;
    const bool khcw_env = khcw_state && khcw_bucket && khcw_az && khcw_cam && khcw_floor && khcw_rng;

    if (khcw_env && khco_k == g_castocc_key) {
        kh_castocc_pre_step(khco_ctx, khco_n, khco_k, khco_dom, khco_sun, khco_rng);   // KH_CASTOCC_PRE: the next grid, a slice at a time; may commit.
        khco_cb.cast_mat[0][3] = g_castocc_inv;   // The arm (> 0), with the fill's localityMeta.w.
        khco_cb.cast_mat[1][3] = g_castocc_lo[0];
        khco_cb.cast_mat[2][3] = g_castocc_lo[1];
        return true;
    }
    g_castocc_key = khco_k;   // Provisional; cleared below on any failure.

    const bool khcw_fresh = g_castocc_set.size() <= static_cast<size_t>(khco_n) +
                                                    static_cast<size_t>(khco_n) / KH_OCC_STALE;
    if (khcw_env && khcw_fresh) {
        g_castocc_addi.clear();

        for (int khci_i = 0; khci_i < khco_n; ++khci_i) {
            const uint64_t khci_h = kh_castocc_hash1(g_sun_locals.data() + static_cast<size_t>(khci_i) * 6u);

            if (!std::binary_search(g_castocc_set.begin(), g_castocc_set.end(), khci_h)) {
                g_castocc_addi.push_back(static_cast<uint32_t>(khci_i));
            }
        }

        if (g_castocc_addi.size() <= static_cast<size_t>(KH_OCC_ADD_MAX)) {
            // Additions are swept in the BUILT envelope: its sun band, its domain disc, its floor.
            bool khci_ok = true;

            for (size_t khci_j = 0; khci_j < g_castocc_addi.size() && khci_ok; ++khci_j) {
                const float* khci_e = g_sun_locals.data() + static_cast<size_t>(g_castocc_addi[khci_j]) * 6u;
                const KhOccSweep khci_w = kh_castocc_sweep(khci_e, g_castocc_sunb, g_castocc_dom,
                                                           g_castocc_rdom, g_castocc_floor);
                khci_ok = kh_castocc_fits(khci_e, g_castocc_sunb, khci_w, g_castocc_lo,
                                          g_castocc_cell, g_castocc_inv);
            }

            if (khci_ok) {
                for (size_t khci_j = 0; khci_j < g_castocc_addi.size(); ++khci_j) {
                    const float* khci_e = g_sun_locals.data() + static_cast<size_t>(g_castocc_addi[khci_j]) * 6u;
                    const KhOccSweep khci_w = kh_castocc_sweep(khci_e, g_castocc_sunb, g_castocc_dom,
                                                               g_castocc_rdom, g_castocc_floor);
                    kh_castocc_stamp(khci_e, g_castocc_sunb, khci_w, g_castocc_lo,
                                     g_castocc_cell, g_castocc_inv, g_castocc_grid.data());
                }

                if (g_castocc_addi.empty() || kh_castocc_upload(khco_ctx)) {
                    const size_t khci_was = g_castocc_set.size();

                    for (size_t khci_j = 0; khci_j < g_castocc_addi.size(); ++khci_j) {
                        g_castocc_set.push_back(
                            kh_castocc_hash1(g_sun_locals.data() + static_cast<size_t>(g_castocc_addi[khci_j]) * 6u));
                    }

                    std::sort(g_castocc_set.begin() + khci_was, g_castocc_set.end());
                    std::inplace_merge(g_castocc_set.begin(),
                                       g_castocc_set.begin() + khci_was, g_castocc_set.end());

                    khco_cb.cast_mat[0][3] = g_castocc_inv;
                    khco_cb.cast_mat[1][3] = g_castocc_lo[0];
                    khco_cb.cast_mat[2][3] = g_castocc_lo[1];
                    return true;
                }

                return kh_castocc_fail();
            }
        }
    }
    // Full rebuild about THIS fire's envelope.
    if (g_castocc_pre.active) {   // KH_CASTOCC_PRE: outrun; the rebuild is the truth now.
        g_castocc_pre.active = false;
    }
    const KhOccSun khco_sb = kh_castocc_sun(khco_sun);
    const float khco_slack = kh_castocc_slack_of(khco_rng);
    const float khco_rdom = 0.995f * khco_rng + khco_slack;
    const float khco_floor = khco_dom[1] - khco_rng - khco_slack;   // KH_CASTOCC_FLOOR: the deepest receiver.
    float khco_lo[2] = { 1.0e30f, 1.0e30f };
    float khco_hi[2] = { -1.0e30f, -1.0e30f };
    static std::vector<KhOccSweep> khco_sw;   // Per caster (render-thread scratch).
    khco_sw.resize(static_cast<size_t>(khco_n));
    for (int i = 0; i < khco_n; ++i) {
        const float* e = g_sun_locals.data() + static_cast<size_t>(i) * 6u;
        khco_sw[static_cast<size_t>(i)] = kh_castocc_sweep(e, khco_sb, khco_dom, khco_rdom, khco_floor);
        const KhOccSweep& khco_w = khco_sw[static_cast<size_t>(i)];
        if (!khco_w.on) continue;
        float khco_b0[2], khco_b1[2];
        kh_castocc_box(e, khco_sb, khco_w, khco_b0, khco_b1);
        if (khco_b0[0] < khco_lo[0]) khco_lo[0] = khco_b0[0];
        if (khco_b0[1] < khco_lo[1]) khco_lo[1] = khco_b0[1];
        if (khco_b1[0] > khco_hi[0]) khco_hi[0] = khco_b1[0];
        if (khco_b1[1] > khco_hi[1]) khco_hi[1] = khco_b1[1];
    }
    if (khco_lo[0] > khco_hi[0]) return kh_castocc_fail();   // No sweep reaches the domain: the
                                                             // shader's grid arm stays 0.
    {   // KH_CASTOCC_PAD: rim headroom so an arriving caster can still be an
        // addition instead of a rebuild.
        const float khco_g0 = (khco_hi[0] - khco_lo[0]) * KH_OCC_EXTENT_PAD;
        const float khco_g1 = (khco_hi[1] - khco_lo[1]) * KH_OCC_EXTENT_PAD;
        khco_lo[0] -= khco_g0; khco_hi[0] += khco_g0;
        khco_lo[1] -= khco_g1; khco_hi[1] += khco_g1;
    }
    const float khco_sp0 = khco_hi[0] - khco_lo[0];
    const float khco_sp1 = khco_hi[1] - khco_lo[1];
    // KH_CASTOCC_SPAN: an infinite span (a caster size near FLT_MAX overflows its sweep) makes cell inf and inv 0, and
    // every stamp converts NaN to int. Below this bound each stamp term is a bounded number of cells.
    if (!(khco_sp0 > 0.0f && khco_sp0 <= 1.0e30f) || !(khco_sp1 > 0.0f && khco_sp1 <= 1.0e30f)) {
        return kh_castocc_fail();
    }
    float khco_cell = (khco_sp0 > khco_sp1 ? khco_sp0 : khco_sp1) / KH_OCC_N;
    if (!(khco_cell > 1.0e-4f)) khco_cell = 1.0e-4f;
    const float khco_inv = 1.0f / khco_cell;

    g_castocc_grid.resize(static_cast<size_t>(KH_OCC_N) * KH_OCC_N * 2u);   // KH_CASTOCC_INCR: retained.
    for (size_t g = 0; g < g_castocc_grid.size(); g += 2) { g_castocc_grid[g] = 1.0e30f; g_castocc_grid[g + 1] = -1.0e30f; }
    for (int i = 0; i < khco_n; ++i) {
        const float* e = g_sun_locals.data() + static_cast<size_t>(i) * 6u;
        kh_castocc_stamp(e, khco_sb, khco_sw[static_cast<size_t>(i)], khco_lo, khco_cell, khco_inv,
                         g_castocc_grid.data());
    }

    if (!g_res.castocc_tex) {
        ID3D11Device* khco_dev = nullptr;
        khco_ctx->GetDevice(&khco_dev);
        if (!khco_dev) return kh_castocc_fail();
        D3D11_TEXTURE2D_DESC khco_td = {};
        khco_td.Width = KH_OCC_N;
        khco_td.Height = KH_OCC_N;
        khco_td.MipLevels = 1;
        khco_td.ArraySize = 1;
        khco_td.Format = DXGI_FORMAT_R32G32_FLOAT;
        khco_td.SampleDesc.Count = 1;
        khco_td.Usage = D3D11_USAGE_DYNAMIC;
        khco_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        khco_td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        const bool khco_ok =
            SUCCEEDED(khco_dev->CreateTexture2D(&khco_td, nullptr, &g_res.castocc_tex)) &&
            SUCCEEDED(khco_dev->CreateShaderResourceView(g_res.castocc_tex, nullptr, &g_res.castocc_srv));
        if (!khco_ok) { KH_SAFE_RELEASE(g_res.castocc_srv); KH_SAFE_RELEASE(g_res.castocc_tex); }
        khco_dev->Release();
        if (!g_res.castocc_srv) return kh_castocc_fail();
    }
    if (!kh_castocc_upload(khco_ctx)) return kh_castocc_fail();

    khco_cb.cast_mat[0][3] = khco_inv;
    khco_cb.cast_mat[1][3] = khco_lo[0];
    khco_cb.cast_mat[2][3] = khco_lo[1];
    g_castocc_lo[0] = khco_lo[0];
    g_castocc_lo[1] = khco_lo[1];
    g_castocc_inv = khco_inv;
    g_castocc_cell = khco_cell;
    g_castocc_sunb = khco_sb;   // The envelope, committed with the grid.
    g_castocc_dom[0] = khco_dom[0]; g_castocc_dom[1] = khco_dom[1]; g_castocc_dom[2] = khco_dom[2];
    g_castocc_slack = khco_slack;
    g_castocc_rdom = khco_rdom;
    g_castocc_rng = khco_rng;   // KH_CASTOCC_RANGE.
    g_castocc_floor = khco_floor;
    g_castocc_set.clear();
    g_castocc_set.reserve(static_cast<size_t>(khco_n));

    for (int khco_s = 0; khco_s < khco_n; ++khco_s) {
        g_castocc_set.push_back(kh_castocc_hash1(g_sun_locals.data() + static_cast<size_t>(khco_s) * 6u));
    }

    std::sort(g_castocc_set.begin(), g_castocc_set.end());
    return true;
}
}