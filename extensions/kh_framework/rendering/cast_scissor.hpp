#pragma once

// The injection half (rendering_integration_injection.hpp): the cast scissor (CSC). Never include it on its own.
namespace RenderIntegration {

// CAST_SCISSOR - the fire's screen rectangle. PSMaskCast writes shade = 1 - hit * saturate(castView[2].w) under a
// MIN blend into the engine's mask, so a pixel with hit = 0 writes 1 and, on a UNORM mask (every value already <= 1),
// changes nothing: drawing it or not is the same image. A float mask (R16_FLOAT) could hold a value above 1 that the
// MIN clamps, so there the fire stays whole. hit > 0 needs, at the pixel's reconstructed (terrain-snapped) point pw:
//   - the range fade (mirMeta.w = R >= 0.5): |pw - fc|.xz and |pw.y - fc.y|
//     under 0.995 R, fc the frozen fire camera (castMat * -castView[0]);
//   - near_ok: the occupancy grid's cell (xz inside the grid's N cells), or
//     KhCastReach for some caster of the list (CB or t2).
// KhCastReach's set is the caster's shadow volume (CAST_PRISM): p passes its lateral test exactly when p = b + s d
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
// view depth CSC_EPS: every chord between two vertices lies inside the hull, so its crossing does too, and the
// hull's own edges are among the chords. A pixel whose depth is under CSC_EPS reconstructs within |M| EPS sqrt(1 +
// fx^2 + fy^2) of the camera; a prism that close stands the rectangle down. The occupancy grid keeps its xz domain and
// the whole band (its cells' own y spans are on the GPU).
// CAST_MAPS - the second gate, and the one that bites: hit > 0 also needs SunShadowOcclusion(pw) > 0, which admits a
// point only inside a sun map's window - the union's (sunVP: |clip.xy| < 1 within its 0.001 guard, clip.z > 0,
// unbounded down-sun) or a tier's (sunVP2..5, sunMeta*.x >= 0.5: |clip.xy| < 1 within 0.002, 0 < clip.z < 1); a tier's
// carried verdict resolves only where that tier admitted the point, and the union answers 0 outside its own window. So
// the pixels a fire can change lie in the union of those windows' world regions, cut to the fade band: for a tier, its
// box's corners and edge crossings; for the union, its near-plane cap's corners and edges cut to the band, each cap
// corner swept down-sun (clip +z) to the band's floor, entering at the top when it starts above. The snap moves pw.y by
// at most thmMeta.z: the regions are taken at both offsets. The windows come from the same CB lanes the shader reads
// (fill_sun_tiers_cb on this fill), rebased to sunOrigin as the shader rebases. The rectangle is the intersection of
// the reach bound's rectangle and the windows'. Anything the shader could take another way (the combined fallback, no
// fade, a t2 count past the list, a window whose down-sun axis does not descend) draws whole. 0 = whole,
// 1 = inside rect, 2 = no pixel can change (the draw is skipped).
static constexpr double CSC_EPS = 0.01;      // View depth (m) the boxes are clipped at.
static constexpr double CSC_PAD_M = 0.5;    // World padding (m): the shader's float reconstruction.
static constexpr long   CSC_PAD_PX = 2;     // Screen padding (px).
static constexpr int    CSC_VMAX = 96;      // Vertices per polytope (a cut tier box's 12 edges x 4 at most).
// CSC_MANY: past this many list casters each prism's vertices go into one world box that is projected once (a
// superset of the union: identical safety, one polytope instead of thousands), below it each prism projects
// on its own (tighter).
static constexpr int    CSC_MANY = 32;
inline int cast_scissor(const ConstantData& cb, bool unorm, D3D11_RECT& rect) {
    if (!unorm) return 0;
    if (!(cb.cast_view[2][3] > 0.0f)) return 2;   // saturate(<= 0) = 0: shade is 1 everywhere.
    if (cb.sun_meta[0] < 0.5f) return 0;         // No map (every fire fills one; EA_SLAB_DEAD): whole.
    const double R = cb.mir_meta[3];
    if (!(R >= 0.5)) return 0;                    // No range fade: nothing bounds the reach.
    const double W = cb.cast_view[1][2], H = cb.cast_view[1][3];
    const double fx = cb.cast_view[1][0], fy = cb.cast_view[1][1];
    if (!(W >= 1.0 && H >= 1.0 && W < 1.0e6 && H < 1.0e6 &&
          fx > 1.0e-6 && fy > 1.0e-6 && fx < 1.0e6 && fy < 1.0e6)) return 0;
    double m[3][3], t[3];
    double frob = 0.0;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) { m[r][c] = cb.cast_mat[r][c]; frob += m[r][c] * m[r][c]; }
        t[r] = cb.cast_view[0][r];
    }
    frob = sqrt(frob);
    const double cs_det =
        m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    if (!(fabs(cs_det) > 1.0e-6) || !(frob < 1.0e3)) return 0;
    double mi[3][3];   // castMat^-1: the shader forms pw = castMat * (v - castView[0]).
    mi[0][0] =  (m[1][1] * m[2][2] - m[1][2] * m[2][1]) / cs_det;
    mi[0][1] = -(m[0][1] * m[2][2] - m[0][2] * m[2][1]) / cs_det;
    mi[0][2] =  (m[0][1] * m[1][2] - m[0][2] * m[1][1]) / cs_det;
    mi[1][0] = -(m[1][0] * m[2][2] - m[1][2] * m[2][0]) / cs_det;
    mi[1][1] =  (m[0][0] * m[2][2] - m[0][2] * m[2][0]) / cs_det;
    mi[1][2] = -(m[0][0] * m[1][2] - m[0][2] * m[1][0]) / cs_det;
    mi[2][0] =  (m[1][0] * m[2][1] - m[1][1] * m[2][0]) / cs_det;
    mi[2][1] = -(m[0][0] * m[2][1] - m[0][1] * m[2][0]) / cs_det;
    mi[2][2] =  (m[0][0] * m[1][1] - m[0][1] * m[1][0]) / cs_det;
    double fc[3];   // The shader's khfc: dot(-castView[0].xyz, castMat[i].xyz).
    for (int r = 0; r < 3; ++r) fc[r] = -(t[0] * m[r][0] + t[1] * m[r][1] + t[2] * m[r][2]);
    const double rr = 0.995 * R;
    const double tol = (cb.thm_params[3] >= 0.5f && cb.thm_meta[2] > 0.0f) ? cb.thm_meta[2] : 0.0;
    const double ylo = fc[1] - rr - tol - CSC_PAD_M;   // The band's floor, widened.
    const double yhi = fc[1] + rr + tol + CSC_PAD_M;   // Its top, widened.
    const double cs_near = frob * CSC_EPS * sqrt(1.0 + fx * fx + fy * fy) + 0.01;
    // Two screen bounds: the reach hull's and the map windows' (CAST_MAPS); the rectangle is their intersection.
    // Each stands down to the whole screen on its own.
    struct CscBox { double s0[2] = { 1.0e300, 1.0e300 }; double s1[2] = { -1.0e300, -1.0e300 }; bool any = false; bool whole = false; };
    CscBox reach, maps;
    // One convex world polytope (engine axes) given by its vertices, into a bound: stands it down when the camera is
    // within cs_near of the polytope's bounds, else projects every vertex in front of the clip plane and every
    // chord's crossing of it.
    auto poly = [&](CscBox& bx, const double (*pv)[3], int pn) {
        bool& cs_whole = bx.whole;
        bool& cs_any = bx.any;
        double* cs_s0 = bx.s0;
        double* cs_s1 = bx.s1;
        if (cs_whole || pn <= 0) return;
        double b0[3] = { 1.0e300, 1.0e300, 1.0e300 }, b1[3] = { -1.0e300, -1.0e300, -1.0e300 };
        for (int i = 0; i < pn; ++i) {
            for (int a = 0; a < 3; ++a) {
                if (!(pv[i][a] == pv[i][a])) { cs_whole = true; return; }
                b0[a] = fmin(b0[a], pv[i][a]); b1[a] = fmax(b1[a], pv[i][a]);
            }
        }
        double d2 = 0.0;   // The camera's distance to the bounds.
        for (int a = 0; a < 3; ++a) {
            const double g = fmax(fmax(b0[a] - fc[a], fc[a] - b1[a]), 0.0);
            d2 += g * g;
        }
        if (d2 <= cs_near * cs_near) { cs_whole = true; return; }
        double v[CSC_VMAX][3];
        for (int i = 0; i < pn; ++i) {
            for (int r = 0; r < 3; ++r) {
                v[i][r] = mi[r][0] * pv[i][0] + mi[r][1] * pv[i][1] + mi[r][2] * pv[i][2] + t[r];
            }
        }
        auto pt = [&](const double q[3]) {
            const double sx = (q[0] / (q[2] * fx) + 1.0) * 0.5 * W;
            const double sy = (1.0 - q[1] / (q[2] * fy)) * 0.5 * H;
            if (!(sx == sx && sy == sy)) { cs_whole = true; return; }
            cs_s0[0] = fmin(cs_s0[0], sx); cs_s1[0] = fmax(cs_s1[0], sx);
            cs_s0[1] = fmin(cs_s0[1], sy); cs_s1[1] = fmax(cs_s1[1], sy);
            cs_any = true;
        };
        for (int i = 0; i < pn && !cs_whole; ++i) {   // Vertices in front of the clip plane...
            if (v[i][2] >= CSC_EPS) pt(v[i]);
            for (int j = i + 1; j < pn && !cs_whole; ++j) {   // ...and every chord that crosses it.
                const double za = v[i][2] - CSC_EPS, zb = v[j][2] - CSC_EPS;
                if ((za < 0.0) == (zb < 0.0)) continue;
                const double f = za / (za - zb);
                const double q[3] = { v[i][0] + (v[j][0] - v[i][0]) * f, v[i][1] + (v[j][1] - v[i][1]) * f, CSC_EPS };
                pt(q);
            }
        }
    };
    // The sweep: w = (k ux, -1, k uz) per metre of drop (KhCastReach's k and u; a vertical sun has k = 0, a
    // straight-down column).
    const double sd[3] = { cb.cast_view[2][0], cb.cast_view[2][1], cb.cast_view[2][2] };
    const double hl = sqrt(sd[0] * sd[0] + sd[2] * sd[2]);
    const double cs_k = hl / fmax(fabs(sd[1]), 0.02);
    const double ux = hl > 1.0e-4 ? -sd[0] / hl : 0.0;
    const double uz = hl > 1.0e-4 ? -sd[2] / hl : 0.0;
    const double w[3] = { cs_k * ux, -1.0, cs_k * uz };
    const double stretch = 2.0 + 3.0 / fmax(fabs(sd[1]), 0.15);
    // One caster of a KhCastReach list (c, h engine axes): the padded box swept to the band's floor and cut at its top
    // (CAST_PRISM).
    double u0[3] = { 1.0e300, 1.0e300, 1.0e300 }, u1[3] = { -1.0e300, -1.0e300, -1.0e300 };   // CSC_MANY: the union box.
    bool uany = false;
    auto caster = [&](const float* c, const float* h, bool many) {
        CscBox& cs_bx = reach;
        if (cs_bx.whole) return;
        const double hx = fabs(h[0]), hy = fabs(h[1]), hz = fabs(h[2]);
        const double hlen = sqrt(hx * hx + hy * hy + hz * hz);
        const double lr = fmin(hlen * stretch, fmax(600.0, hlen * 24.0));
        const double ex = hx + lr + CSC_PAD_M, ey = hy + lr + tol + CSC_PAD_M, ez = hz + lr + CSC_PAD_M;
        if (!(c[0] == c[0] && c[1] == c[1] && c[2] == c[2])) { cs_bx.whole = true; return; }
        const double cy0 = c[1] - ey, cy1 = c[1] + ey;
        double pv[CSC_VMAX][3];
        int pn = 0;
        auto add = [&](double x, double y, double z) { if (pn < CSC_VMAX) { pv[pn][0] = x; pv[pn][1] = y; pv[pn][2] = z; ++pn; } };
        auto sweep = [&](double x, double y, double z) {   // The corner's sweep, from y down to the floor.
            const double dl = y - ylo;
            if (dl > 0.0) add(x + w[0] * dl, ylo, z + w[2] * dl);
        };
        for (int kx = 0; kx < 2; ++kx) {
            for (int kz = 0; kz < 2; ++kz) {
                const double x = kx ? c[0] + ex : c[0] - ex, z = kz ? c[2] + ez : c[2] - ez;
                // The column (x, [cy0, +inf), z) cut to the band: from the higher of the box's bottom and the floor up
                // to the top.
                const double ya = fmax(cy0, ylo), yb = yhi;
                if (ya <= yb) { add(x, ya, z); if (yb > ya) add(x, yb, z); }
                // The sweep edges from both corners: a corner above the top enters the band at its crossing of the top
                // (x + w dl, yhi, z + w dl); every sweep ends at the floor.
                for (int ky = 0; ky < 2; ++ky) {
                    double x0 = x, y0 = ky ? cy1 : cy0, z0 = z;
                    if (y0 > yhi) { const double dl = y0 - yhi; x0 += w[0] * dl; z0 += w[2] * dl; y0 = yhi; add(x0, y0, z0); }
                    sweep(x0, y0, z0);
                }
            }
        }
        if (many) {   // CSC_MANY: into the union box, projected once below.
            for (int i = 0; i < pn; ++i) {
                for (int a = 0; a < 3; ++a) {
                    if (!(pv[i][a] == pv[i][a])) { cs_bx.whole = true; return; }
                    u0[a] = fmin(u0[a], pv[i][a]); u1[a] = fmax(u1[a], pv[i][a]);
                }
            }
            if (pn > 0) uany = true;
            return;
        }
        poly(cs_bx, pv, pn);
    };
    auto cs_union = [&]() {   // CSC_MANY: the one projection.
        if (reach.whole || !uany) return;
        double pv[8][3];
        for (int k = 0; k < 8; ++k) { pv[k][0] = (k & 1) ? u1[0] : u0[0]; pv[k][1] = (k & 2) ? u1[1] : u0[1]; pv[k][2] = (k & 4) ? u1[2] : u0[2]; }
        poly(reach, pv, 8);
    };
    const int lc = static_cast<int>(cb.locality_meta[0]);
    if (cb.locality_meta[1] >= 0.5f) {
        // The occupancy grid, armed as PSMaskCast arms it (an unwritten edge lane: the t2 list below): xz
        // inside its N cells, the whole band.
        if (cb.cast_mat[0][3] > 0.0f && cb.locality_meta[3] >= 1.0f) {
            int n = static_cast<int>(cb.locality_meta[3]);
            const double ext = static_cast<double>(n) / cb.cast_mat[0][3];
            const double lx = cb.cast_mat[1][3], lz = cb.cast_mat[2][3];
            double x0 = fmax(lx, fc[0] - rr), x1 = fmin(lx + ext, fc[0] + rr);   // The fade's square.
            double z0 = fmax(lz, fc[2] - rr), z1 = fmin(lz + ext, fc[2] + rr);
            if (x0 <= x1 && z0 <= z1) {
                x0 -= CSC_PAD_M; x1 += CSC_PAD_M; z0 -= CSC_PAD_M; z1 += CSC_PAD_M;
                double pv[8][3];
                for (int k = 0; k < 8; ++k) { pv[k][0] = (k & 1) ? x1 : x0; pv[k][1] = (k & 2) ? yhi : ylo; pv[k][2] = (k & 4) ? z1 : z0; }
                poly(reach, pv, 8);
            }
        } else {   // The t2 list the fire uploaded (upload_locality_ext: g_sun_maps.sun_locals).
            if (lc < 0 || static_cast<size_t>(lc) * 6u > g_sun_maps.sun_locals.size()) return 0;
            const bool many = lc > CSC_MANY;
            for (int li = 0; li < lc && !reach.whole; ++li) {
                const float* e = g_sun_maps.sun_locals.data() + static_cast<size_t>(li) * 6u;
                caster(e, e + 3, many);
            }
            if (many) cs_union();
        }
    } else if (cb.locality_meta[0] >= 0.5f && cb.locality_meta[0] <= 16.5f) {
        for (int li = 0; li < lc && !reach.whole; ++li) {
            caster(cb.locality[li * 2], cb.locality[li * 2 + 1], false);   // <= 16: each on its own.
        }
    } else {
        return 0;   // The combined-bounds fallback.
    }
    // CAST_MAPS: one sun map's window region, cut to the band, at a y offset (the snap's). vp is the row-major
    // world->clip the shader multiplies (row vector), r = p - sunOrigin. far_m = the window is a box (a tier, 0 < z <
    // 1) rather than a prism (the union, z > 0).
    auto map = [&](const float (*vp)[4], bool far_m, double dy) {
        if (maps.whole) return;
        double L[3][3], tt[3];
        for (int r = 0; r < 3; ++r) { for (int c = 0; c < 3; ++c) L[r][c] = vp[r][c]; tt[r] = vp[3][r]; }
        const double det = L[0][0] * (L[1][1] * L[2][2] - L[1][2] * L[2][1]) - L[0][1] * (L[1][0] * L[2][2] - L[1][2] * L[2][0]) + L[0][2] * (L[1][0] * L[2][1] - L[1][1] * L[2][0]);
        if (!(fabs(det) > 1.0e-30) || !(det == det)) { maps.whole = true; return; }
        double Li[3][3];   // clip = r * L + t  =>  r = (clip - t) * Li.
        Li[0][0] =  (L[1][1] * L[2][2] - L[1][2] * L[2][1]) / det; Li[0][1] = -(L[0][1] * L[2][2] - L[0][2] * L[2][1]) / det; Li[0][2] =  (L[0][1] * L[1][2] - L[0][2] * L[1][1]) / det;
        Li[1][0] = -(L[1][0] * L[2][2] - L[1][2] * L[2][0]) / det; Li[1][1] =  (L[0][0] * L[2][2] - L[0][2] * L[2][0]) / det; Li[1][2] = -(L[0][0] * L[1][2] - L[0][2] * L[1][0]) / det;
        Li[2][0] =  (L[1][0] * L[2][1] - L[1][1] * L[2][0]) / det; Li[2][1] = -(L[0][0] * L[2][1] - L[0][1] * L[2][0]) / det; Li[2][2] =  (L[0][0] * L[1][1] - L[0][1] * L[1][0]) / det;
        const double o[3] = { cb.sun_origin[0], cb.sun_origin[1] + dy, cb.sun_origin[2] };
        auto world = [&](double cx, double cy, double cz, double out[3]) {
            const double q[3] = { cx - tt[0], cy - tt[1], cz - tt[2] };
            for (int a = 0; a < 3; ++a) out[a] = o[a] + q[0] * Li[0][a] + q[1] * Li[1][a] + q[2] * Li[2][a];
        };
        const double m_g = 1.0 + 1.0e-3;   // Past the shader's uv guard.
        double pv[CSC_VMAX][3];
        int pn = 0;
        auto add = [&](const double p[3]) { if (pn < CSC_VMAX) { pv[pn][0] = p[0]; pv[pn][1] = p[1]; pv[pn][2] = p[2]; ++pn; } };
        // A segment a-b cut to the band: its ends inside and its crossings.
        auto seg = [&](const double a[3], const double b[3]) {
            const double ya = a[1], yb = b[1];
            if (ya >= ylo && ya <= yhi) add(a);
            if (yb >= ylo && yb <= yhi) add(b);
            for (int e = 0; e < 2; ++e) {
                const double yp = e ? yhi : ylo;
                if ((ya < yp) == (yb < yp)) continue;
                const double f = (yp - ya) / (yb - ya);
                const double p[3] = { a[0] + (b[0] - a[0]) * f, yp, a[2] + (b[2] - a[2]) * f };
                add(p);
            }
        };
        if (far_m) {   // A tier: the box, 12 edges.
            double v[8][3];
            for (int k = 0; k < 8; ++k) world((k & 1) ? m_g : -m_g, (k & 2) ? m_g : -m_g, (k & 4) ? 1.0 + 1.0e-3 : -1.0e-3, v[k]);
            for (int k = 0; k < 8; ++k) for (int a = 1; a < 8; a <<= 1) { const int j = k | a; if (j != k) seg(v[k], v[j]); }
        } else {   // The union: the near cap swept down-sun to the band's floor.
            double dz[3] = { Li[2][0], Li[2][1], Li[2][2] };   // World per unit clip z.
            if (!(dz[1] < -1.0e-9)) { maps.whole = true; return; }   // Not descending: unbounded in the band.
            double v[4][3];
            for (int k = 0; k < 4; ++k) world((k & 1) ? m_g : -m_g, (k & 2) ? m_g : -m_g, -1.0e-3, v[k]);
            for (int k = 0; k < 4; ++k) {
                seg(v[k], v[k ^ 1]); seg(v[k], v[k ^ 2]);   // The cap's edges (each twice: harmless).
                double a[3] = { v[k][0], v[k][1], v[k][2] };
                if (a[1] > yhi) { const double s = (a[1] - yhi) / -dz[1]; for (int q = 0; q < 3; ++q) a[q] += dz[q] * s; }   // Entry at the top.
                if (a[1] >= ylo) {
                    const double s = (a[1] - ylo) / -dz[1];
                    const double b[3] = { a[0] + dz[0] * s, ylo, a[2] + dz[2] * s };   // The floor.
                    seg(a, b);
                }
            }
        }
        poly(maps, pv, pn);
    };
    for (int o_m = 0; o_m < 2 && !maps.whole; ++o_m) {
        const double dy = o_m ? tol : -tol;
        map(cb.sun_vp, false, dy);   // sunMeta.x >= 0.5 holds here.
        if (cb.sun_meta2[0] >= 0.5f) map(cb.sun_vp2, true, dy);
        if (cb.sun_meta3[0] >= 0.5f) map(cb.sun_vp3, true, dy);
        if (cb.sun_meta4[0] >= 0.5f) map(cb.sun_vp4, true, dy);
        if (cb.sun_meta5[0] >= 0.5f) map(cb.sun_vp5, true, dy);
        if (tol == 0.0) break;
    }
    if (reach.whole && maps.whole) return 0;
    if ((!reach.whole && !reach.any) || (!maps.whole && !maps.any)) return 2;   // A gate no pixel passes.
    double cs_s0[2], cs_s1[2];   // The intersection of the two bounds (a whole one does not narrow).
    for (int a = 0; a < 2; ++a) {
        cs_s0[a] = fmax(reach.whole ? -1.0e300 : reach.s0[a], maps.whole ? -1.0e300 : maps.s0[a]);
        cs_s1[a] = fmin(reach.whole ?  1.0e300 : reach.s1[a], maps.whole ?  1.0e300 : maps.s1[a]);
    }
    const long wl = static_cast<long>(ceil(W)), hl2 = static_cast<long>(ceil(H));
    const double cs_x0 = fmax(floor(cs_s0[0]) - CSC_PAD_PX, 0.0), cs_x1 = fmin(ceil(cs_s1[0]) + CSC_PAD_PX, static_cast<double>(wl));
    const double cs_y0 = fmax(floor(cs_s0[1]) - CSC_PAD_PX, 0.0), y1 = fmin(ceil(cs_s1[1]) + CSC_PAD_PX, static_cast<double>(hl2));
    if (!(cs_x0 < cs_x1 && cs_y0 < y1)) return 2;   // All of it off screen.
    if (cs_x0 <= 0.0 && cs_y0 <= 0.0 && cs_x1 >= wl && y1 >= hl2) return 0;   // The whole screen anyway.
    rect.left = static_cast<LONG>(cs_x0);
    rect.top = static_cast<LONG>(cs_y0);
    rect.right = static_cast<LONG>(cs_x1);
    rect.bottom = static_cast<LONG>(y1);
    return 1;
}
// The fire's rasterizer with the scissor on: the default state otherwise (the fire runs on RSSetState(nullptr)).
// Created once per device; a failure leaves the fire whole.
inline bool cast_scissor_rs(ID3D11DeviceContext* ctx) {
    if (g_mask.cast_rs_scissor) return true;
    if (g_mask.cast_rs_scissor_failed) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    D3D11_RASTERIZER_DESC d = {};
    d.FillMode = D3D11_FILL_SOLID;
    d.CullMode = D3D11_CULL_BACK;
    d.DepthClipEnable = TRUE;
    d.ScissorEnable = TRUE;
    if (FAILED(dev->CreateRasterizerState(&d, &g_mask.cast_rs_scissor))) {
        g_mask.cast_rs_scissor = nullptr;
        g_mask.cast_rs_scissor_failed = true;
    }
    dev->Release();
    return g_mask.cast_rs_scissor != nullptr;
}
}