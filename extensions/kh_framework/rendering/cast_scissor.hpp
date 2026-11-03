#pragma once

// The injection half (rendering_integration_injection.hpp): the cast scissor (KH_CSC). Never include it on its own.
namespace RenderIntegration {

// KH_CAST_SCISSOR - the fire's screen rectangle. PSMaskCast writes shade = 1 - hit * saturate(castView[2].w) under a
// MIN blend into the engine's mask, so a pixel with hit = 0 writes 1 and, on a UNORM mask (every value already <= 1),
// changes nothing: drawing it or not is the same image. A float mask (R16_FLOAT) could hold a value above 1 that the
// MIN clamps, so there the fire stays whole. hit > 0 needs, at the pixel's reconstructed (terrain-snapped) point pw:
//   - the range fade (mirMeta.w = R >= 0.5): |pw - fc|.xz and |pw.y - fc.y|
//     under 0.995 R, fc the frozen fire camera (castMat * -castView[0]);
//   - near_ok: the occupancy grid's cell (xz inside the grid's N cells), or
//     KhCastReach for some caster of the list (CB or t2).
// KhCastReach's set is the caster's shadow volume (KH_CAST_PRISM): p passes its lateral test exactly when p = b + s d
// for some b in the box padded by lr in xz and some s >= 0, d the down-sun direction dropping 1 / k per metre (t0 / t1
// are that s at the box's bottom and top faces) - or, above the box, where t0 = t1 = 0 clamp the sweep away, when p.xz
// alone is within the padded box (the column above it); its 3D test passes only within lr of the box. So the reach set
// lies inside the convex hull of the column above the padded box and the prism swept from it along d, cut to the fade's
// altitude band: a receiver outside the band cannot pass the fade, so the column stops at its top and the sweep at its
// floor. The snap moves pw.y by at most thmMeta.z, never xz, so the band and the box are widened by that. The vertices
// are the padded box's xz corners at the band's top and at the higher of its bottom and the band's floor, plus each box
// corner swept to the floor (entering at the top when the corner is above it) - the exact vertices of that hull inside
// the band (an inside vertex or an edge's crossing). Seen from the frozen view the shader reconstructs with (v =
// castMat^-1 * pw + castView[0]), every point of the prism projects inside the rectangle of its vertices clipped at
// view depth KH_CSC_EPS: every chord between two vertices lies inside the hull, so its crossing does too, and the
// hull's own edges are among the chords. A pixel whose depth is under KH_CSC_EPS reconstructs within |M| EPS sqrt(1 +
// fx^2 + fy^2) of the camera; a prism that close stands the rectangle down. The occupancy grid keeps its xz domain and
// the whole band (its cells' own y spans are on the GPU).
// KH_CAST_MAPS - the second gate, and the one that bites: hit > 0 also needs SunShadowOcclusion(pw) > 0, which admits a
// point only inside a sun map's window - the union's (sunVP: |clip.xy| < 1 within its 0.001 guard, clip.z > 0,
// unbounded down-sun) or a tier's (sunVP2..5, sunMeta*.x >= 0.5: |clip.xy| < 1 within 0.002, 0 < clip.z < 1); a tier's
// carried verdict resolves only where that tier admitted the point, and the union answers 0 outside its own window. So
// the pixels a fire can change lie in the union of those windows' world regions, cut to the fade band: for a tier, its
// box's corners and edge crossings; for the union, its near-plane cap's corners and edges cut to the band, each cap
// corner swept down-sun (clip +z) to the band's floor, entering at the top when it starts above. The snap moves pw.y by
// at most thmMeta.z: the regions are taken at both offsets. The windows come from the same CB lanes the shader reads
// (kh_fill_sun_tiers_cb on this fill), rebased to sunOrigin as the shader rebases. The rectangle is the intersection of
// the reach bound's rectangle and the windows'. Anything the shader could take another way (the combined fallback, no
// fade, a t2 count past the list, a window whose down-sun axis does not descend) draws whole. 0 = whole,
// 1 = inside khcs_rect, 2 = no pixel can change (the draw is skipped).
static constexpr double KH_CSC_EPS = 0.01;      // View depth (m) the boxes are clipped at.
static constexpr double KH_CSC_PAD_M = 0.5;    // World padding (m): the shader's float reconstruction.
static constexpr long   KH_CSC_PAD_PX = 2;     // Screen padding (px).
static constexpr int    KH_CSC_VMAX = 96;      // Vertices per polytope (a cut tier box's 12 edges x 4 at most).
// KH_CSC_MANY: past this many list casters each prism's vertices go into one world box that is projected once (a
// superset of the union: identical safety, one polytope instead of thousands), below it each prism projects
// on its own (tighter).
static constexpr int    KH_CSC_MANY = 32;
inline int kh_cast_scissor(const ConstantData& khcs_cb, bool khcs_unorm, D3D11_RECT& khcs_rect) {
    if (!khcs_unorm) return 0;
    if (!(khcs_cb.cast_view[2][3] > 0.0f)) return 2;   // saturate(<= 0) = 0: shade is 1 everywhere.
    if (khcs_cb.sun_meta[0] < 0.5f) return 0;         // No map (every fire fills one; KH_EA_SLAB_DEAD): whole.
    const double khcs_R = khcs_cb.mir_meta[3];
    if (!(khcs_R >= 0.5)) return 0;                    // No range fade: nothing bounds the reach.
    const double khcs_W = khcs_cb.cast_view[1][2], khcs_H = khcs_cb.cast_view[1][3];
    const double khcs_fx = khcs_cb.cast_view[1][0], khcs_fy = khcs_cb.cast_view[1][1];
    if (!(khcs_W >= 1.0 && khcs_H >= 1.0 && khcs_W < 1.0e6 && khcs_H < 1.0e6 &&
          khcs_fx > 1.0e-6 && khcs_fy > 1.0e-6 && khcs_fx < 1.0e6 && khcs_fy < 1.0e6)) return 0;
    double khcs_m[3][3], khcs_t[3];
    double khcs_frob = 0.0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { khcs_m[r][c] = khcs_cb.cast_mat[r][c]; khcs_frob += khcs_m[r][c] * khcs_m[r][c]; }
        khcs_t[r] = khcs_cb.cast_view[0][r];
    }
    khcs_frob = sqrt(khcs_frob);
    const double khcs_det =
        khcs_m[0][0] * (khcs_m[1][1] * khcs_m[2][2] - khcs_m[1][2] * khcs_m[2][1]) -
        khcs_m[0][1] * (khcs_m[1][0] * khcs_m[2][2] - khcs_m[1][2] * khcs_m[2][0]) +
        khcs_m[0][2] * (khcs_m[1][0] * khcs_m[2][1] - khcs_m[1][1] * khcs_m[2][0]);
    if (!(fabs(khcs_det) > 1.0e-6) || !(khcs_frob < 1.0e3)) return 0;
    double khcs_mi[3][3];   // castMat^-1: the shader forms pw = castMat * (v - castView[0]).
    khcs_mi[0][0] =  (khcs_m[1][1] * khcs_m[2][2] - khcs_m[1][2] * khcs_m[2][1]) / khcs_det;
    khcs_mi[0][1] = -(khcs_m[0][1] * khcs_m[2][2] - khcs_m[0][2] * khcs_m[2][1]) / khcs_det;
    khcs_mi[0][2] =  (khcs_m[0][1] * khcs_m[1][2] - khcs_m[0][2] * khcs_m[1][1]) / khcs_det;
    khcs_mi[1][0] = -(khcs_m[1][0] * khcs_m[2][2] - khcs_m[1][2] * khcs_m[2][0]) / khcs_det;
    khcs_mi[1][1] =  (khcs_m[0][0] * khcs_m[2][2] - khcs_m[0][2] * khcs_m[2][0]) / khcs_det;
    khcs_mi[1][2] = -(khcs_m[0][0] * khcs_m[1][2] - khcs_m[0][2] * khcs_m[1][0]) / khcs_det;
    khcs_mi[2][0] =  (khcs_m[1][0] * khcs_m[2][1] - khcs_m[1][1] * khcs_m[2][0]) / khcs_det;
    khcs_mi[2][1] = -(khcs_m[0][0] * khcs_m[2][1] - khcs_m[0][1] * khcs_m[2][0]) / khcs_det;
    khcs_mi[2][2] =  (khcs_m[0][0] * khcs_m[1][1] - khcs_m[0][1] * khcs_m[1][0]) / khcs_det;
    double khcs_fc[3];   // The shader's khfc: dot(-castView[0].xyz, castMat[i].xyz).
    for (int r = 0; r < 3; ++r) khcs_fc[r] = -(khcs_t[0] * khcs_m[r][0] + khcs_t[1] * khcs_m[r][1] + khcs_t[2] * khcs_m[r][2]);
    const double khcs_rr = 0.995 * khcs_R;
    const double khcs_tol = (khcs_cb.thm_params[3] >= 0.5f && khcs_cb.thm_meta[2] > 0.0f) ? khcs_cb.thm_meta[2] : 0.0;
    const double khcs_ylo = khcs_fc[1] - khcs_rr - khcs_tol - KH_CSC_PAD_M;   // The band's floor, widened.
    const double khcs_yhi = khcs_fc[1] + khcs_rr + khcs_tol + KH_CSC_PAD_M;   // Its top, widened.
    const double khcs_near = khcs_frob * KH_CSC_EPS * sqrt(1.0 + khcs_fx * khcs_fx + khcs_fy * khcs_fy) + 0.01;
    // Two screen bounds: the reach hull's and the map windows' (KH_CAST_MAPS); the rectangle is their intersection.
    // Each stands down to the whole screen on its own.
    struct KhCscBox { double s0[2] = { 1.0e300, 1.0e300 }; double s1[2] = { -1.0e300, -1.0e300 }; bool any = false; bool whole = false; };
    KhCscBox khcs_reach, khcs_maps;
    // One convex world polytope (engine axes) given by its vertices, into a bound: stands it down when the camera is
    // within khcs_near of the polytope's bounds, else projects every vertex in front of the clip plane and every
    // chord's crossing of it.
    auto khcs_poly = [&](KhCscBox& bx, const double (*pv)[3], int pn) {
        bool& khcs_whole = bx.whole;
        bool& khcs_any = bx.any;
        double* khcs_s0 = bx.s0;
        double* khcs_s1 = bx.s1;
        if (khcs_whole || pn <= 0) return;
        double b0[3] = { 1.0e300, 1.0e300, 1.0e300 }, b1[3] = { -1.0e300, -1.0e300, -1.0e300 };
        for (int i = 0; i < pn; ++i) {
            for (int a = 0; a < 3; ++a) {
                if (!(pv[i][a] == pv[i][a])) { khcs_whole = true; return; }
                b0[a] = fmin(b0[a], pv[i][a]); b1[a] = fmax(b1[a], pv[i][a]);
            }
        }
        double d2 = 0.0;   // The camera's distance to the bounds.
        for (int a = 0; a < 3; ++a) {
            const double g = fmax(fmax(b0[a] - khcs_fc[a], khcs_fc[a] - b1[a]), 0.0);
            d2 += g * g;
        }
        if (d2 <= khcs_near * khcs_near) { khcs_whole = true; return; }
        double v[KH_CSC_VMAX][3];
        for (int i = 0; i < pn; ++i) {
            for (int r = 0; r < 3; ++r) {
                v[i][r] = khcs_mi[r][0] * pv[i][0] + khcs_mi[r][1] * pv[i][1] + khcs_mi[r][2] * pv[i][2] + khcs_t[r];
            }
        }
        auto khcs_pt = [&](const double q[3]) {
            const double sx = (q[0] / (q[2] * khcs_fx) + 1.0) * 0.5 * khcs_W;
            const double sy = (1.0 - q[1] / (q[2] * khcs_fy)) * 0.5 * khcs_H;
            if (!(sx == sx && sy == sy)) { khcs_whole = true; return; }
            khcs_s0[0] = fmin(khcs_s0[0], sx); khcs_s1[0] = fmax(khcs_s1[0], sx);
            khcs_s0[1] = fmin(khcs_s0[1], sy); khcs_s1[1] = fmax(khcs_s1[1], sy);
            khcs_any = true;
        };
        for (int i = 0; i < pn && !khcs_whole; ++i) {   // Vertices in front of the clip plane...
            if (v[i][2] >= KH_CSC_EPS) khcs_pt(v[i]);
            for (int j = i + 1; j < pn && !khcs_whole; ++j) {   // ...and every chord that crosses it.
                const double za = v[i][2] - KH_CSC_EPS, zb = v[j][2] - KH_CSC_EPS;
                if ((za < 0.0) == (zb < 0.0)) continue;
                const double f = za / (za - zb);
                const double q[3] = { v[i][0] + (v[j][0] - v[i][0]) * f, v[i][1] + (v[j][1] - v[i][1]) * f, KH_CSC_EPS };
                khcs_pt(q);
            }
        }
    };
    // The sweep: w = (k ux, -1, k uz) per metre of drop (KhCastReach's k and u; a vertical sun has k = 0, a
    // straight-down column).
    const double khcs_sd[3] = { khcs_cb.cast_view[2][0], khcs_cb.cast_view[2][1], khcs_cb.cast_view[2][2] };
    const double khcs_hl = sqrt(khcs_sd[0] * khcs_sd[0] + khcs_sd[2] * khcs_sd[2]);
    const double khcs_k = khcs_hl / fmax(fabs(khcs_sd[1]), 0.02);
    const double khcs_ux = khcs_hl > 1.0e-4 ? -khcs_sd[0] / khcs_hl : 0.0;
    const double khcs_uz = khcs_hl > 1.0e-4 ? -khcs_sd[2] / khcs_hl : 0.0;
    const double khcs_w[3] = { khcs_k * khcs_ux, -1.0, khcs_k * khcs_uz };
    const double khcs_stretch = 2.0 + 3.0 / fmax(fabs(khcs_sd[1]), 0.15);
    // One caster of a KhCastReach list (c, h engine axes): the padded box swept to the band's floor and cut at its top
    // (KH_CAST_PRISM).
    double khcs_u0[3] = { 1.0e300, 1.0e300, 1.0e300 }, khcs_u1[3] = { -1.0e300, -1.0e300, -1.0e300 };   // KH_CSC_MANY: the union box.
    bool khcs_uany = false;
    auto khcs_caster = [&](const float* c, const float* h, bool khcs_many) {
        KhCscBox& khcs_bx = khcs_reach;
        if (khcs_bx.whole) return;
        const double hx = fabs(h[0]), hy = fabs(h[1]), hz = fabs(h[2]);
        const double hlen = sqrt(hx * hx + hy * hy + hz * hz);
        const double lr = fmin(hlen * khcs_stretch, fmax(600.0, hlen * 24.0));
        const double ex = hx + lr + KH_CSC_PAD_M, ey = hy + lr + khcs_tol + KH_CSC_PAD_M, ez = hz + lr + KH_CSC_PAD_M;
        if (!(c[0] == c[0] && c[1] == c[1] && c[2] == c[2])) { khcs_bx.whole = true; return; }
        const double cy0 = c[1] - ey, cy1 = c[1] + ey;
        double pv[KH_CSC_VMAX][3];
        int pn = 0;
        auto khcs_add = [&](double x, double y, double z) { if (pn < KH_CSC_VMAX) { pv[pn][0] = x; pv[pn][1] = y; pv[pn][2] = z; ++pn; } };
        auto khcs_sweep = [&](double x, double y, double z) {   // The corner's sweep, from y down to the floor.
            const double dl = y - khcs_ylo;
            if (dl > 0.0) khcs_add(x + khcs_w[0] * dl, khcs_ylo, z + khcs_w[2] * dl);
        };
        for (int kx = 0; kx < 2; ++kx) {
            for (int kz = 0; kz < 2; ++kz) {
                const double x = kx ? c[0] + ex : c[0] - ex, z = kz ? c[2] + ez : c[2] - ez;
                // The column (x, [cy0, +inf), z) cut to the band: from the higher of the box's bottom and the floor up
                // to the top.
                const double ya = fmax(cy0, khcs_ylo), yb = khcs_yhi;
                if (ya <= yb) { khcs_add(x, ya, z); if (yb > ya) khcs_add(x, yb, z); }
                // The sweep edges from both corners: a corner above the top enters the band at its crossing of the top
                // (x + w dl, yhi, z + w dl); every sweep ends at the floor.
                for (int ky = 0; ky < 2; ++ky) {
                    double x0 = x, y0 = ky ? cy1 : cy0, z0 = z;
                    if (y0 > khcs_yhi) { const double dl = y0 - khcs_yhi; x0 += khcs_w[0] * dl; z0 += khcs_w[2] * dl; y0 = khcs_yhi; khcs_add(x0, y0, z0); }
                    khcs_sweep(x0, y0, z0);
                }
            }
        }
        if (khcs_many) {   // KH_CSC_MANY: into the union box, projected once below.
            for (int i = 0; i < pn; ++i) {
                for (int a = 0; a < 3; ++a) {
                    if (!(pv[i][a] == pv[i][a])) { khcs_bx.whole = true; return; }
                    khcs_u0[a] = fmin(khcs_u0[a], pv[i][a]); khcs_u1[a] = fmax(khcs_u1[a], pv[i][a]);
                }
            }
            if (pn > 0) khcs_uany = true;
            return;
        }
        khcs_poly(khcs_bx, pv, pn);
    };
    auto khcs_union = [&]() {   // KH_CSC_MANY: the one projection.
        if (khcs_reach.whole || !khcs_uany) return;
        double pv[8][3];
        for (int k = 0; k < 8; ++k) { pv[k][0] = (k & 1) ? khcs_u1[0] : khcs_u0[0]; pv[k][1] = (k & 2) ? khcs_u1[1] : khcs_u0[1]; pv[k][2] = (k & 4) ? khcs_u1[2] : khcs_u0[2]; }
        khcs_poly(khcs_reach, pv, 8);
    };
    const int khcs_lc = static_cast<int>(khcs_cb.locality_meta[0]);
    if (khcs_cb.locality_meta[1] >= 0.5f) {
        // The occupancy grid, armed as PSMaskCast arms it (an unwritten edge lane: the t2 list below): xz
        // inside its N cells, the whole band.
        if (khcs_cb.cast_mat[0][3] > 0.0f && khcs_cb.locality_meta[3] >= 1.0f) {
            int khcs_n = static_cast<int>(khcs_cb.locality_meta[3]);
            const double khcs_ext = static_cast<double>(khcs_n) / khcs_cb.cast_mat[0][3];
            const double khcs_lx = khcs_cb.cast_mat[1][3], khcs_lz = khcs_cb.cast_mat[2][3];
            double x0 = fmax(khcs_lx, khcs_fc[0] - khcs_rr), x1 = fmin(khcs_lx + khcs_ext, khcs_fc[0] + khcs_rr);   // The fade's square.
            double z0 = fmax(khcs_lz, khcs_fc[2] - khcs_rr), z1 = fmin(khcs_lz + khcs_ext, khcs_fc[2] + khcs_rr);
            if (x0 <= x1 && z0 <= z1) {
                x0 -= KH_CSC_PAD_M; x1 += KH_CSC_PAD_M; z0 -= KH_CSC_PAD_M; z1 += KH_CSC_PAD_M;
                double pv[8][3];
                for (int k = 0; k < 8; ++k) { pv[k][0] = (k & 1) ? x1 : x0; pv[k][1] = (k & 2) ? khcs_yhi : khcs_ylo; pv[k][2] = (k & 4) ? z1 : z0; }
                khcs_poly(khcs_reach, pv, 8);
            }
        } else {   // The t2 list the fire uploaded (kh_upload_locality_ext: g_sun_locals).
            if (khcs_lc < 0 || static_cast<size_t>(khcs_lc) * 6u > g_sun_locals.size()) return 0;
            const bool khcs_many = khcs_lc > KH_CSC_MANY;
            for (int li = 0; li < khcs_lc && !khcs_reach.whole; ++li) {
                const float* e = g_sun_locals.data() + static_cast<size_t>(li) * 6u;
                khcs_caster(e, e + 3, khcs_many);
            }
            if (khcs_many) khcs_union();
        }
    } else if (khcs_cb.locality_meta[0] >= 0.5f && khcs_cb.locality_meta[0] <= 16.5f) {
        for (int li = 0; li < khcs_lc && !khcs_reach.whole; ++li) {
            khcs_caster(khcs_cb.locality[li * 2], khcs_cb.locality[li * 2 + 1], false);   // <= 16: each on its own.
        }
    } else {
        return 0;   // The combined-bounds fallback.
    }
    // KH_CAST_MAPS: one sun map's window region, cut to the band, at a y offset (the snap's). khm_vp is the row-major
    // world->clip the shader multiplies (row vector), r = p - sunOrigin. khm_far = the window is a box (a tier, 0 < z <
    // 1) rather than a prism (the union, z > 0).
    auto khcs_map = [&](const float (*khm_vp)[4], bool khm_far, double khm_dy) {
        if (khcs_maps.whole) return;
        double L[3][3], tt[3];
        for (int r = 0; r < 3; ++r) { for (int c = 0; c < 3; ++c) L[r][c] = khm_vp[r][c]; tt[r] = khm_vp[3][r]; }
        const double det = L[0][0] * (L[1][1] * L[2][2] - L[1][2] * L[2][1]) - L[0][1] * (L[1][0] * L[2][2] - L[1][2] * L[2][0]) + L[0][2] * (L[1][0] * L[2][1] - L[1][1] * L[2][0]);
        if (!(fabs(det) > 1.0e-30) || !(det == det)) { khcs_maps.whole = true; return; }
        double Li[3][3];   // clip = r * L + t  =>  r = (clip - t) * Li.
        Li[0][0] =  (L[1][1] * L[2][2] - L[1][2] * L[2][1]) / det; Li[0][1] = -(L[0][1] * L[2][2] - L[0][2] * L[2][1]) / det; Li[0][2] =  (L[0][1] * L[1][2] - L[0][2] * L[1][1]) / det;
        Li[1][0] = -(L[1][0] * L[2][2] - L[1][2] * L[2][0]) / det; Li[1][1] =  (L[0][0] * L[2][2] - L[0][2] * L[2][0]) / det; Li[1][2] = -(L[0][0] * L[1][2] - L[0][2] * L[1][0]) / det;
        Li[2][0] =  (L[1][0] * L[2][1] - L[1][1] * L[2][0]) / det; Li[2][1] = -(L[0][0] * L[2][1] - L[0][1] * L[2][0]) / det; Li[2][2] =  (L[0][0] * L[1][1] - L[0][1] * L[1][0]) / det;
        const double o[3] = { khcs_cb.sun_origin[0], khcs_cb.sun_origin[1] + khm_dy, khcs_cb.sun_origin[2] };
        auto khm_world = [&](double cx, double cy, double cz, double out[3]) {
            const double q[3] = { cx - tt[0], cy - tt[1], cz - tt[2] };
            for (int a = 0; a < 3; ++a) out[a] = o[a] + q[0] * Li[0][a] + q[1] * Li[1][a] + q[2] * Li[2][a];
        };
        const double khm_g = 1.0 + 1.0e-3;   // Past the shader's uv guard.
        double pv[KH_CSC_VMAX][3];
        int pn = 0;
        auto khm_add = [&](const double p[3]) { if (pn < KH_CSC_VMAX) { pv[pn][0] = p[0]; pv[pn][1] = p[1]; pv[pn][2] = p[2]; ++pn; } };
        // A segment a-b cut to the band: its ends inside and its crossings.
        auto khm_seg = [&](const double a[3], const double b[3]) {
            const double ya = a[1], yb = b[1];
            if (ya >= khcs_ylo && ya <= khcs_yhi) khm_add(a);
            if (yb >= khcs_ylo && yb <= khcs_yhi) khm_add(b);
            for (int e = 0; e < 2; ++e) {
                const double yp = e ? khcs_yhi : khcs_ylo;
                if ((ya < yp) == (yb < yp)) continue;
                const double f = (yp - ya) / (yb - ya);
                const double p[3] = { a[0] + (b[0] - a[0]) * f, yp, a[2] + (b[2] - a[2]) * f };
                khm_add(p);
            }
        };
        if (khm_far) {   // A tier: the box, 12 edges.
            double v[8][3];
            for (int k = 0; k < 8; ++k) khm_world((k & 1) ? khm_g : -khm_g, (k & 2) ? khm_g : -khm_g, (k & 4) ? 1.0 + 1.0e-3 : -1.0e-3, v[k]);
            for (int k = 0; k < 8; ++k) for (int a = 1; a < 8; a <<= 1) { const int j = k | a; if (j != k) khm_seg(v[k], v[j]); }
        } else {   // The union: the near cap swept down-sun to the band's floor.
            double dz[3] = { Li[2][0], Li[2][1], Li[2][2] };   // World per unit clip z.
            if (!(dz[1] < -1.0e-9)) { khcs_maps.whole = true; return; }   // Not descending: unbounded in the band.
            double v[4][3];
            for (int k = 0; k < 4; ++k) khm_world((k & 1) ? khm_g : -khm_g, (k & 2) ? khm_g : -khm_g, -1.0e-3, v[k]);
            for (int k = 0; k < 4; ++k) {
                khm_seg(v[k], v[k ^ 1]); khm_seg(v[k], v[k ^ 2]);   // The cap's edges (each twice: harmless).
                double a[3] = { v[k][0], v[k][1], v[k][2] };
                if (a[1] > khcs_yhi) { const double s = (a[1] - khcs_yhi) / -dz[1]; for (int q = 0; q < 3; ++q) a[q] += dz[q] * s; }   // Entry at the top.
                if (a[1] >= khcs_ylo) {
                    const double s = (a[1] - khcs_ylo) / -dz[1];
                    const double b[3] = { a[0] + dz[0] * s, khcs_ylo, a[2] + dz[2] * s };   // The floor.
                    khm_seg(a, b);
                }
            }
        }
        khcs_poly(khcs_maps, pv, pn);
    };
    for (int khm_o = 0; khm_o < 2 && !khcs_maps.whole; ++khm_o) {
        const double khm_dy = khm_o ? khcs_tol : -khcs_tol;
        khcs_map(khcs_cb.sun_vp, false, khm_dy);   // sunMeta.x >= 0.5 holds here.
        if (khcs_cb.sun_meta2[0] >= 0.5f) khcs_map(khcs_cb.sun_vp2, true, khm_dy);
        if (khcs_cb.sun_meta3[0] >= 0.5f) khcs_map(khcs_cb.sun_vp3, true, khm_dy);
        if (khcs_cb.sun_meta4[0] >= 0.5f) khcs_map(khcs_cb.sun_vp4, true, khm_dy);
        if (khcs_cb.sun_meta5[0] >= 0.5f) khcs_map(khcs_cb.sun_vp5, true, khm_dy);
        if (khcs_tol == 0.0) break;
    }
    if (khcs_reach.whole && khcs_maps.whole) return 0;
    if ((!khcs_reach.whole && !khcs_reach.any) || (!khcs_maps.whole && !khcs_maps.any)) return 2;   // A gate no pixel passes.
    double khcs_s0[2], khcs_s1[2];   // The intersection of the two bounds (a whole one does not narrow).
    for (int a = 0; a < 2; ++a) {
        khcs_s0[a] = fmax(khcs_reach.whole ? -1.0e300 : khcs_reach.s0[a], khcs_maps.whole ? -1.0e300 : khcs_maps.s0[a]);
        khcs_s1[a] = fmin(khcs_reach.whole ?  1.0e300 : khcs_reach.s1[a], khcs_maps.whole ?  1.0e300 : khcs_maps.s1[a]);
    }
    const long khcs_wl = static_cast<long>(ceil(khcs_W)), khcs_hl2 = static_cast<long>(ceil(khcs_H));
    const double khcs_x0 = fmax(floor(khcs_s0[0]) - KH_CSC_PAD_PX, 0.0), khcs_x1 = fmin(ceil(khcs_s1[0]) + KH_CSC_PAD_PX, static_cast<double>(khcs_wl));
    const double khcs_y0 = fmax(floor(khcs_s0[1]) - KH_CSC_PAD_PX, 0.0), khcs_y1 = fmin(ceil(khcs_s1[1]) + KH_CSC_PAD_PX, static_cast<double>(khcs_hl2));
    if (!(khcs_x0 < khcs_x1 && khcs_y0 < khcs_y1)) return 2;   // All of it off screen.
    if (khcs_x0 <= 0.0 && khcs_y0 <= 0.0 && khcs_x1 >= khcs_wl && khcs_y1 >= khcs_hl2) return 0;   // The whole screen anyway.
    khcs_rect.left = static_cast<LONG>(khcs_x0);
    khcs_rect.top = static_cast<LONG>(khcs_y0);
    khcs_rect.right = static_cast<LONG>(khcs_x1);
    khcs_rect.bottom = static_cast<LONG>(khcs_y1);
    return 1;
}
// The fire's rasterizer with the scissor on: the default state otherwise (the fire runs on RSSetState(nullptr)).
// Created once per device; a failure leaves the fire whole.
inline bool kh_cast_scissor_rs(ID3D11DeviceContext* khsr_ctx) {
    if (g_mask.cast_rs_scissor) return true;
    if (g_mask.cast_rs_scissor_failed) return false;
    ID3D11Device* khsr_dev = nullptr;
    khsr_ctx->GetDevice(&khsr_dev);
    if (!khsr_dev) return false;
    D3D11_RASTERIZER_DESC khsr_d = {};
    khsr_d.FillMode = D3D11_FILL_SOLID;
    khsr_d.CullMode = D3D11_CULL_BACK;
    khsr_d.DepthClipEnable = TRUE;
    khsr_d.ScissorEnable = TRUE;
    if (FAILED(khsr_dev->CreateRasterizerState(&khsr_d, &g_mask.cast_rs_scissor))) {
        g_mask.cast_rs_scissor = nullptr;
        g_mask.cast_rs_scissor_failed = true;
    }
    khsr_dev->Release();
    return g_mask.cast_rs_scissor != nullptr;
}
}