#pragma once

// The feature half (rendering_integration_features.hpp): the visibility occluders (VOC) and the occluder floor.
// Never include it on its own.
namespace RenderIntegration {

inline bool voc_project(const VisOcc& o, const float w[3], float& x, float& y) {
    const float q[3] = { w[0] + o.bias[0], w[1] + o.bias[1],
                         w[2] + o.bias[2] };
    float c[4];
    for (int j = 0; j < 4; ++j) {
        c[j] = o.vp[3][j];
        for (int i = 0; i < 3; ++i) c[j] += q[i] * o.vp[i][j];
    }
    if (!(c[3] > 0.05f)) return false;   // Behind / at the near plane: fail toward visible.
    const float iw = 1.0f / c[3];
    x = (c[0] * iw * 0.5f + 0.5f) * static_cast<float>(VOC_W);
    y = (0.5f - c[1] * iw * 0.5f) * static_cast<float>(VOC_H);
    if (!(x == x) || !(y == y)) return false;   // NaN-safe.
    return true;
}

// Inner-conservative rasterization of a convex polygon (voc_box's faces, at most six corners): a cell counts only
// when all four of its corners are inside every edge. Edge signs normalized against the centroid, so winding never
// matters; a near-degenerate polygon stands the face down.
inline void voc_face(VisOcc& o, const float p[][2], int n, float z, int& budget) {
    if (n < 3 || n > 6) return;
    float cx = 0.0f, cy = 0.0f;
    float mnx = p[0][0], mxx = p[0][0];
    float mny = p[0][1], mxy = p[0][1];
    const float in = 1.0f / static_cast<float>(n);
    for (int i = 0; i < n; ++i) {
        cx += p[i][0] * in;
        cy += p[i][1] * in;
        if (p[i][0] < mnx) mnx = p[i][0];
        if (p[i][0] > mxx) mxx = p[i][0];
        if (p[i][1] < mny) mny = p[i][1];
        if (p[i][1] > mxy) mxy = p[i][1];
    }
    float a[6], b[6], c[6];
    for (int e = 0; e < n; ++e) {
        const float* p0 = p[e];
        const float* p1 = p[(e + 1) % n];
        a[e] = p0[1] - p1[1];
        b[e] = p1[0] - p0[0];
        c[e] = p0[0] * p1[1] - p1[0] * p0[1];
        const float ec = a[e] * cx + b[e] * cy + c[e];
        if (!(ec > 1.0e-4f || ec < -1.0e-4f)) return;   // Degenerate: stand down.
        if (ec < 0.0f) { a[e] = -a[e]; b[e] = -b[e]; c[e] = -c[e]; }
    }
    // The bounds clamped to the grid (one cell of margin) before the casts, on both sides: a corner projected near
    // the camera plane lands far off it.
    const float gw = static_cast<float>(VOC_W), gh = static_cast<float>(VOC_H);
    int x0 = static_cast<int>(floorf(fminf(fmaxf(mnx, -1.0f), gw))); if (x0 < 0) x0 = 0;
    int y0 = static_cast<int>(floorf(fminf(fmaxf(mny, -1.0f), gh))); if (y0 < 0) y0 = 0;
    int x1 = static_cast<int>(floorf(fminf(fmaxf(mxx, -1.0f), gw)));
    if (x1 > VOC_W - 1) x1 = VOC_W - 1;
    int y1 = static_cast<int>(floorf(fminf(fmaxf(mxy, -1.0f), gh)));
    if (y1 > VOC_H - 1) y1 = VOC_H - 1;
    for (int iy = y0; iy <= y1; ++iy) {
        for (int ix = x0; ix <= x1; ++ix) {
            bool inside = true;
            for (int e = 0; e < n && inside; ++e) {
                const float mn = a[e] * static_cast<float>(ix) +
                                 b[e] * static_cast<float>(iy) + c[e] +
                                 (a[e] < 0.0f ? a[e] : 0.0f) +
                                 (b[e] < 0.0f ? b[e] : 0.0f);
                if (!(mn > 0.0f)) inside = false;
            }
            if (!inside) continue;
            float& slot = o.z[iy * VOC_W + ix];
            if (z < slot) slot = z;
            if (++budget > VOC_PX_BUDGET) return;
        }
    }
}
inline float voc_poly_area(const VocPoly& p) {
    float a = 0.0f;
    for (int i = 0; i < p.n; ++i) {
        const int j = (i + 1) % p.n;
        a += p.x[i] * p.y[j] - p.x[j] * p.y[i];
    }
    return 0.5f * fabsf(a);
}
// The part of in where a x + b y + c >= 0 (coordinates relative to the cell, c already shifted to them).
// Sutherland-Hodgman against one line.
inline void voc_poly_clip(const VocPoly& in, float a, float b, float c, VocPoly& out) {
    out.n = 0;
    for (int i = 0; i < in.n; ++i) {
        const int j = (i + 1) % in.n;
        const float v0 = a * in.x[i] + b * in.y[i] + c;
        const float v1 = a * in.x[j] + b * in.y[j] + c;
        if (v0 >= 0.0f && out.n < 16) {
            out.x[out.n] = in.x[i]; out.y[out.n] = in.y[i]; ++out.n;
        }
        if ((v0 >= 0.0f) != (v1 >= 0.0f) && out.n < 16) {
            const float t = v0 / (v0 - v1);
            out.x[out.n] = in.x[i] + t * (in.x[j] - in.x[i]);
            out.y[out.n] = in.y[i] + t * (in.y[j] - in.y[i]);
            ++out.n;
        }
    }
}
// One occluder box into the grid (centre ctr, half-vectors hv, engine axes): its front faces that are not
// grazing (VOC_FACE_COS), each clipped at the near depth (VOC_NEAR_CLIP), rasterized one by one (voc_face),
// then the cells along the edges between them (VOC_UNION).
inline void voc_box(VisOcc& o, const float ctr[3], const float hv[3][3],
                    const float cam[3], int& budget) {
    // Points 0..7 the corners (bit k = the sign along axis k), 8..19 the near clip on box edge 8 + 4 ax + (the other
    // two axes' bits).
    float w[20][3], dp[20], ds[20], p[20][2];
    bool have[20] = {}, proj[20] = {};
    auto depth = [&](const float* x) {   // The projection's w (voc_project's).
        float vb_c = o.vp[3][3];
        for (int i = 0; i < 3; ++i) {
            vb_c += (x[i] + o.bias[i]) * o.vp[i][3];
        }
        return vb_c;
    };
    auto finish = [&](int vb_id) {
        float dd = 0.0f;
        for (int vb_c = 0; vb_c < 3; ++vb_c) {
            const float r = w[vb_id][vb_c] - cam[vb_c];
            dd += r * r;
        }
        ds[vb_id] = sqrtf(dd);
        proj[vb_id] = dp[vb_id] >= VOC_W_MIN &&
                      voc_project(o, w[vb_id], p[vb_id][0], p[vb_id][1]);
        have[vb_id] = true;
    };
    for (int s = 0; s < 8; ++s) {
        for (int vb_c = 0; vb_c < 3; ++vb_c) {
            w[s][vb_c] = ctr[vb_c] + ((s & 1) ? 1.0f : -1.0f) * hv[0][vb_c] +
                         ((s & 2) ? 1.0f : -1.0f) * hv[1][vb_c] +
                         ((s & 4) ? 1.0f : -1.0f) * hv[2][vb_c];
        }
        dp[s] = depth(w[s]);
        finish(s);
    }
    auto front = [&](int s) { return dp[s] >= VOC_W_MIN; };
    // The clip point of the edge between corners s0 and s1 (one bit apart).
    auto clip = [&](int s0, int s1) -> int {
        if (s0 > s1) { const int t = s0; s0 = s1; s1 = t; }
        const int vb_ax = (s0 ^ s1) == 1 ? 0 : ((s0 ^ s1) == 2 ? 1 : 2);
        const int u = (vb_ax + 1) % 3, v = (vb_ax + 2) % 3;
        const int vb_id = 8 + 4 * vb_ax + ((s0 >> u) & 1) + 2 * ((s0 >> v) & 1);
        if (!have[vb_id]) {
            const float t = (VOC_W_MIN - dp[s0]) / (dp[s1] - dp[s0]);
            for (int vb_c = 0; vb_c < 3; ++vb_c) {
                const float e = w[s1][vb_c] - w[s0][vb_c];
                w[vb_id][vb_c] = w[s0][vb_c] + t * e;
            }
            dp[vb_id] = VOC_W_MIN;   // By construction; the projection re-derives its own w.
            finish(vb_id);
        }
        return vb_id;
    };
    // The faces: point ids in ring order, their edge lines (inside-positive, the centroid decides as voc_face) and
    // the farthest point's distance.
    struct VbFace { int ax, sg, n, id[6]; float a[6], b[6], c[6], z; };
    VbFace f[6];
    int nf = 0;
    static const int ring[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
    for (int vb_ax = 0; vb_ax < 3 && budget <= VOC_PX_BUDGET; ++vb_ax) {
        for (int si = 0; si < 2 && budget <= VOC_PX_BUDGET; ++si) {
            const float fs = si ? 1.0f : -1.0f;
            float nl2 = 0.0f, vl2 = 0.0f, nv = 0.0f;
            for (int vb_c = 0; vb_c < 3; ++vb_c) {
                const float fc = ctr[vb_c] + fs * hv[vb_ax][vb_c];
                const float vw = fc - cam[vb_c];
                nl2 += hv[vb_ax][vb_c] * hv[vb_ax][vb_c];
                vl2 += vw * vw;
                nv += vw * fs * hv[vb_ax][vb_c];
            }
            if (!(nl2 > 1.0e-12f) || !(vl2 > 1.0e-6f)) continue;
            if (!(nv / (sqrtf(vl2) * sqrtf(nl2)) < -VOC_FACE_COS)) continue;
            const int u = (vb_ax + 1) % 3, v = (vb_ax + 2) % 3;
            int q[4];
            for (int k = 0; k < 4; ++k) {
                q[k] = (si << vb_ax) | ((ring[k][0] > 0 ? 1 : 0) << u) |
                       ((ring[k][1] > 0 ? 1 : 0) << v);
            }
            VbFace& g = f[nf];
            g.n = 0;
            for (int k = 0; k < 4; ++k) {
                const int a0 = q[k], a1 = q[(k + 1) & 3];
                if (front(a0)) g.id[g.n++] = a0;
                if (front(a0) != front(a1)) g.id[g.n++] = clip(a0, a1);
            }
            if (g.n < 3) continue;   // Wholly behind the near depth.
            bool ok = true;
            float pts[6][2];
            g.z = 0.0f;
            for (int k = 0; k < g.n; ++k) {
                const int vb_id = g.id[k];
                if (!proj[vb_id]) ok = false;
                pts[k][0] = p[vb_id][0];
                pts[k][1] = p[vb_id][1];
                if (ds[vb_id] > g.z) g.z = ds[vb_id];
            }
            if (!ok) continue;
            voc_face(o, pts, g.n, g.z, budget);
            // The same lines for the union (voc_face's arithmetic on the same points).
            float cx = 0.0f, cy = 0.0f;
            const float in = 1.0f / static_cast<float>(g.n);
            for (int k = 0; k < g.n; ++k) {
                cx += pts[k][0] * in;
                cy += pts[k][1] * in;
            }
            for (int e = 0; e < g.n && ok; ++e) {
                const float* p0 = pts[e];
                const float* p1 = pts[(e + 1) % g.n];
                float vb_a = p0[1] - p1[1], vb_b = p1[0] - p0[0];
                float vb_c = p0[0] * p1[1] - p1[0] * p0[1];
                const float ec = vb_a * cx + vb_b * cy + vb_c;
                if (!(ec > 1.0e-4f || ec < -1.0e-4f)) ok = false;   // Degenerate: no union.
                if (ec < 0.0f) { vb_a = -vb_a; vb_b = -vb_b; vb_c = -vb_c; }
                g.a[e] = vb_a; g.b[e] = vb_b; g.c[e] = vb_c;
            }
            if (!ok) continue;
            g.ax = vb_ax; g.sg = si;
            ++nf;
        }
    }
    if (nf < 2 || budget > VOC_PX_BUDGET) return;
    uint8_t seen[VOC_W * VOC_H] = {};
    // Is the cell (ix, iy) inside the faces' union? Its farthest meeting face's distance in zc.
    auto cover = [&](int ix, int iy, float& zc) -> bool {
        VocPoly pc[VOC_UNION_PIECES], nx[VOC_UNION_PIECES];
        int np = 1, nn = 0;
        pc[0].n = 4;
        pc[0].x[0] = 0.0f; pc[0].y[0] = 0.0f; pc[0].x[1] = 1.0f; pc[0].y[1] = 0.0f;
        pc[0].x[2] = 1.0f; pc[0].y[2] = 1.0f; pc[0].x[3] = 0.0f; pc[0].y[3] = 1.0f;
        zc = 0.0f;
        for (int i = 0; i < nf && np > 0; ++i) {
            const VbFace& g = f[i];
            nn = 0;
            bool meets = false;
            for (int j = 0; j < np; ++j) {
                VocPoly cur = pc[j];
                for (int e = 0; e < g.n && cur.n > 0; ++e) {
                    // The line in the cell's own coordinates (x - ix, y - iy).
                    const float cc = g.c[e] + g.a[e] * static_cast<float>(ix) +
                                     g.b[e] * static_cast<float>(iy);
                    VocPoly out;
                    voc_poly_clip(cur, -g.a[e], -g.b[e], -cc, out);
                    if (out.n >= 3 && voc_poly_area(out) > VOC_UNION_EPS) {
                        if (nn == VOC_UNION_PIECES) return false;   // Out of room: not covered.
                        nx[nn++] = out;
                    }
                    VocPoly in;
                    voc_poly_clip(cur, g.a[e], g.b[e], cc, in);
                    cur = in;
                    if (cur.n < 3 || !(voc_poly_area(cur) > VOC_UNION_EPS)) cur.n = 0;
                }
                if (cur.n > 0) meets = true;
            }
            if (meets && g.z > zc) zc = g.z;
            for (int j = 0; j < nn; ++j) pc[j] = nx[j];
            np = nn;
        }
        return np == 0 && zc > 0.0f;
    };
    // The fast exact test for a cell the edge between faces g and h crosses (edge ge of g, he of h).
    auto pair = [&](const VbFace& g, int ge, const VbFace& h, int he,
                    int ix, int iy) -> bool {
        // The lines in the cell's own coordinates; g's shared line is h's negated.
        float gc[6], hc[6];
        for (int e = 0; e < g.n; ++e) {
            gc[e] = g.c[e] + g.a[e] * static_cast<float>(ix) +
                    g.b[e] * static_cast<float>(iy);
        }
        for (int e = 0; e < h.n; ++e) {
            hc[e] = h.c[e] + h.a[e] * static_cast<float>(ix) +
                    h.b[e] * static_cast<float>(iy);
        }
        auto in = [&](const VbFace& f2, const float* cs, int se, float x, float y) {
            for (int e = 0; e < f2.n; ++e) {
                if (e == se) continue;
                if (!(f2.a[e] * x + f2.b[e] * y + cs[e] > 0.0f)) return false;
            }
            return true;
        };
        // The vertices of the two sides of the cut: the square's corners on each side and the points where the line
        // crosses the square's edges.
        static const float sx[4] = { 0.0f, 1.0f, 1.0f, 0.0f }, sy[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        float v[4];
        for (int k = 0; k < 4; ++k) {
            v[k] = g.a[ge] * sx[k] + g.b[ge] * sy[k] +
                   gc[ge];
        }
        for (int k = 0; k < 4; ++k) {
            const float cx2 = sx[k], cy2 = sy[k];
            if (v[k] >= 0.0f && !in(g, gc, ge, cx2, cy2)) return false;
            if (v[k] <= 0.0f && !in(h, hc, he, cx2, cy2)) return false;
            const int n2 = (k + 1) & 3;
            if ((v[k] >= 0.0f) != (v[n2] >= 0.0f)) {
                const float t2 = v[k] / (v[k] - v[n2]);
                const float px = sx[k] + t2 * (sx[n2] - sx[k]);
                const float py = sy[k] + t2 * (sy[n2] - sy[k]);
                if (!in(g, gc, ge, px, py) ||
                    !in(h, hc, he, px, py)) return false;
            }
        }
        return true;
    };
    auto edge_of = [](const VbFace& f2, int a0, int a1) -> int {
        for (int e = 0; e < f2.n; ++e) {
            const int p0 = f2.id[e], p1 = f2.id[(e + 1) % f2.n];
            if ((p0 == a0 && p1 == a1) || (p0 == a1 && p1 == a0)) return e;
        }
        return -1;
    };
    // Every edge two of the faces share (its part in front of the near depth): the cells its projected segment
    // crosses, clipped to the grid first.
    for (int i = 0; i < nf; ++i) {
        for (int j = i + 1; j < nf; ++j) {
            const VbFace& g = f[i];
            const VbFace& h = f[j];
            if (g.ax == h.ax) continue;   // Opposite faces share nothing.
            const int t = 3 - g.ax - h.ax;
            const int s0 = (g.sg << g.ax) | (h.sg << h.ax);
            const int s1 = s0 | (1 << t);
            if (!front(s0) && !front(s1)) continue;   // Wholly behind the near depth.
            const int e0 = front(s0) ? s0 : clip(s0, s1);
            const int e1 = front(s1) ? s1 : clip(s0, s1);
            const int ge = edge_of(g, e0, e1);
            const int he = edge_of(h, e0, e1);
            if (ge < 0 || he < 0) continue;
            const float x0 = p[e0][0], y0 = p[e0][1];
            const float dx = p[e1][0] - x0, dy = p[e1][1] - y0;
            float t0 = 0.0f, t1 = 1.0f;   // Liang-Barsky against the grid, one cell of margin.
            const float lim[4] = { -1.0f - x0, static_cast<float>(VOC_W) + 1.0f - x0,
                                   -1.0f - y0, static_cast<float>(VOC_H) + 1.0f - y0 };
            const float dir[4] = { -dx, dx, -dy, dy };
            bool in = true;
            for (int k = 0; k < 4 && in; ++k) {
                const float pk = dir[k];
                const float qk = (k & 1) ? lim[k] : -lim[k];
                if (pk == 0.0f) { if (qk < 0.0f) in = false; continue; }
                const float r = qk / pk;
                if (pk < 0.0f) { if (r > t0) t0 = r; }
                else if (r < t1) t1 = r;
                if (t0 > t1) in = false;
            }
            if (!in) continue;
            const float len = (t1 - t0) * sqrtf(dx * dx + dy * dy);
            const int steps = static_cast<int>(fminf(len * 2.0f, 1024.0f)) + 1;
            for (int k = 0; k <= steps; ++k) {
                const float tt = t0 + (t1 - t0) * static_cast<float>(k) /
                                      static_cast<float>(steps);
                // VOC_CAST_CLAMP: the clip keeps the point in [-1, W + 1] only up to float cancellation (a
                // corner far off the grid); clamped two cells out (NaN too) before the casts, where every
                // neighbour below is still off the grid and skipped.
                const int cx = static_cast<int>(floorf(fminf(fmaxf(x0 + tt * dx, -2.0f),
                                                             static_cast<float>(VOC_W) + 2.0f)));
                const int cy = static_cast<int>(floorf(fminf(fmaxf(y0 + tt * dy, -2.0f),
                                                             static_cast<float>(VOC_H) + 2.0f)));
                for (int oy = -1; oy <= 1; ++oy) {
                    for (int ox = -1; ox <= 1; ++ox) {
                        const int ix = cx + ox, iy = cy + oy;
                        if (ix < 0 || iy < 0 || ix >= VOC_W || iy >= VOC_H) continue;
                        uint8_t& m = seen[iy * VOC_W + ix];
                        if (m) continue;
                        m = 1;
                        // Only a cell the edge itself crosses can be in the union and in neither face: off the
                        // edge's line the union on each side is one face. The segment against the cell's square
                        // (1e-3 wider).
                        {
                            float a0 = t0, a1 = t1;
                            const float bx[4] = { static_cast<float>(ix) - 1.0e-3f - x0,
                                                  static_cast<float>(ix) + 1.001f - x0,
                                                  static_cast<float>(iy) - 1.0e-3f - y0,
                                                  static_cast<float>(iy) + 1.001f - y0 };
                            bool hit = true;
                            for (int vb_b = 0; vb_b < 4 && hit; ++vb_b) {
                                const float pb = dir[vb_b];
                                const float qb = (vb_b & 1) ? bx[vb_b] : -bx[vb_b];
                                if (pb == 0.0f) { if (qb < 0.0f) hit = false; continue; }
                                const float rb = qb / pb;
                                if (pb < 0.0f) { if (rb > a0) a0 = rb; }
                                else if (rb < a1) a1 = rb;
                                if (a0 > a1) hit = false;
                            }
                            if (!hit) continue;
                        }
                        float zc = g.z > h.z ? g.z : h.z;
                        if (!pair(g, ge, h, he, ix, iy) &&
                            !(nf > 2 && cover(ix, iy, zc))) continue;
                        float& slot = o.z[iy * VOC_W + ix];
                        if (zc < slot) slot = zc;
                        if (++budget > VOC_PX_BUDGET) return;
                    }
                }
            }
        }
    }
}
// OCC_BOX / OCC_FORCE: the box the cull rasterizes for an object and the bounds the camera must stay out of,
// normalised (engine axes): the mesh's proven box and its every level's bounds, or for a declared occluder its unit
// box for both. The transform is the vertex stage's: centre + R (p * size).
inline void occ_box_of(const RenderObject& o, bool force, float c[3], float h[3],
                       float mn[3], float mx[3]) {
    const MeshDef& md = mesh_def(mesh_id_clamp(o.mesh));
    for (int k = 0; k < 3; ++k) {
        c[k] = force ? 0.0f : md.occ_c[k];
        h[k] = force ? 0.5f : md.occ_h[k];
        mn[k] = force ? -0.5f : md.occ_hmn[k];
        mx[k] = force ? 0.5f : md.occ_hmx[k];
    }
}
// Squared distance (metres^2) from the camera to those bounds, along the object's own axes. OCC_ROW_SCALE: a row
// may carry an attached parent's scale (length s); the camera's local coordinate is (v . row) / |row|^2 and an
// axis's gap counts |row| metres per local unit - exact for orthogonal rows of any lengths (bit-identical to the
// plain dot for |row|^2 == 1). A zero row's coordinate is 0 / 0 = NaN: gap 0 on that axis (both tests fail). A
// non-finite row makes the sum NaN (0 x inf); so does a row whose |row|^2 rounds to inf (|row| > ~1.8e19: +inf
// instead where the box lies off the object's centre on that axis - truly far) or to 0 (|row| < ~2.6e-23: gap 0
// instead where the camera's dot with it is 0). The one caller reads NaN as inside (camera inside or right beside
// the box - no occluder, the conservative side). A local gap past ~1.8e19 squares to +inf (outside): wrong only for
// |row| below ~1.1e-20, where a camera under the caller's 0.2 m from the box reads as outside - unreachable scales.
// Rows that are not orthogonal (a non-uniform parent scale under a rotation: a sheared map) have no such exact form:
// the camera counts as inside (0), as above.
inline float occ_bounds_dist_sq(const RenderObject& o, const float mn[3], const float mx[3],
                                const float cam[3]) {
    for (int i = 0; i < 3; ++i) {   // OCC_ROW_SCALE: the orthogonality the measure needs.
        const float* const ri = o.rot_m + i * 3;
        const float* const rj = o.rot_m + ((i + 1) % 3) * 3;
        const float d = ri[0] * rj[0] + ri[1] * rj[1] + ri[2] * rj[2];
        const float ni = ri[0] * ri[0] + ri[1] * ri[1] + ri[2] * ri[2];
        const float nj = rj[0] * rj[0] + rj[1] * rj[1] + rj[2] * rj[2];
        if (d * d > 1.0e-6f * ni * nj) return 0.0f;
    }
    float c[3];
    obj_center_engine(o, c);
    const float v[3] = { cam[0] - c[0], cam[1] - c[1], cam[2] - c[2] };
    const float se[3] = { o.size[0], o.size[2], o.size[1] };   // Engine edge lengths.
    float acc = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float* const r = o.rot_m + k * 3;
        const float n2 = r[0] * r[0] + r[1] * r[1] + r[2] * r[2];
        const float l = (v[0] * r[0] + v[1] * r[1] + v[2] * r[2]) / n2;
        const float a = mn[k] * se[k], b = mx[k] * se[k];
        const float e = l < a ? a - l : (l > b ? l - b : 0.0f);
        acc += e * e * n2;
    }
    return acc;
}
// The world-axis box (centre, half extents) around everything an object's mesh draws, at any level: the burial
// check's footprint.
inline void occ_hull_world(const RenderObject& o, float hw_c[3], float he[3]) {
    const MeshDef& md = mesh_def(mesh_id_clamp(o.mesh));
    obj_center_engine(o, hw_c);
    const float se[3] = { o.size[0], o.size[2], o.size[1] };
    he[0] = he[1] = he[2] = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float m = 0.5f * (md.occ_hmn[k] + md.occ_hmx[k]) * se[k];
        const float x = 0.5f * (md.occ_hmx[k] - md.occ_hmn[k]) * se[k];
        for (int c = 0; c < 3; ++c) {
            hw_c[c] += m * o.rot_m[k * 3 + c];
            he[c] += x * fabsf(o.rot_m[k * 3 + c]);
        }
    }
}
// OCC_BOX: whether this object draws its mesh as mesh_occ_prep proved it. A skeletal binding, a cloth and a
// chain move their vertices every frame (the pass draws the skinned or simulated buffer - or the rest pose until that
// exists), and a KH_USER_VS vertex stage deforms them on the GPU: none is the geometry the box was proven on,
// whatever the buffer. Beyond those, the pass must bind the mesh's own buffer (any substitution mesh_vb_ref
// makes), and the transform must keep the winding (positive sizes, a proper rotation - an attached basis may be
// mirrored).
inline bool occ_drawn_as_built(const RenderObject& o) {
    if (o.skel || o.cloth_sim || o.chain_sim) return false;
    if (o.materials && o.materials->vertex_any) return false;
    const int mid = mesh_id_clamp(o.mesh);
    if (!mesh_def(mid).occ_ok) return false;
    if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size() || !g_res.mesh_vb[mid] ||
        mesh_vb_ref(mid, o.slot).vb != g_res.mesh_vb[mid]) return false;
    if (!(o.size[0] > 0.0f && o.size[1] > 0.0f && o.size[2] > 0.0f)) return false;
    const float* r = o.rot_m;
    const float det = r[0] * (r[4] * r[8] - r[5] * r[7]) - r[1] * (r[3] * r[8] - r[5] * r[6]) +
                      r[2] * (r[3] * r[7] - r[4] * r[6]);
    return det > 0.5f;
}

// Occluder pick + rasterize. m is the pass's own draw list, x the declared occluders it does not draw (one
// index space, m first). margin covers passes whose PS may write nearer than their geometry (far-arb
// terrain clamp); vo_far / cut bound an occluder to where its far contract cannot discard it. floor = the
// height (engine y) below which this object may fail to depth-write a pixel the grid would count, the terrain
// march's discards aside (VOC_BURIAL says why they are safe): VOC_FLOOR_REFUSE for the SV_Depth routes and any
// occluder the endpoint discard cannot be bounded for, VOC_FLOOR_NONE where no endpoint discard reaches it, else
// voc_floor's certified line - the box is then clipped to its part above it, and taken only with the camera above
// it too. A declared occluder takes none of the drawn-surface tests: its box is the script's promise, whatever is
// drawn there - taken exactly (no inset), at any size on screen, and ranked ahead of every automatic occluder.
template <class VocFloor>
inline void vis_occ_build(VisOcc& vo, const std::vector<RenderObject>& m,
                          const std::vector<RenderObject>& x,
                          const float vp[4][4], const CullSet& cull,
                          const float cam[3], float margin,
                          float vo_far, float cut, VocFloor floor) {
    KH_PROF_SCOPE(PROF_VIS_OCC);   // PROF.
    vo.on = false;
    if (!cull.valid) return;
    memcpy(vo.vp, vp, sizeof(vo.vp));
    vo.bias[0] = cull.bias[0]; vo.bias[1] = cull.bias[1]; vo.bias[2] = cull.bias[2];
    vo.cam[0] = cam[0]; vo.cam[1] = cam[1]; vo.cam[2] = cam[2];
    vo.margin = margin;
    double sc[VOC_MAX];   // Rank keys (see below).
    uint32_t id[VOC_MAX];
    float fl[VOC_MAX];   // VOC_BURIAL: each pick's floor.
    int n = 0;
    const size_t mn = m.size();
    for (uint32_t i = 0; i < mn + x.size(); ++i) {
        const RenderObject& o = i < mn ? m[i] : x[i - mn];
        if (o.fullscreen) continue;
        const bool force = o.occluder;   // OCC_FORCE.
        float f = VOC_FLOOR_NONE;
        if (!force) {
            if (o.effect != 0 || o.mode == DepthMode::Off) continue;
            if (!occ_drawn_as_built(o)) continue;   // OCC_BOX: every mesh by one rule.
            if (o.blend_mode != 0 || o.color[3] < 0.999f) continue;
            if (o.localized || o.banded) continue;
            if (o.materials && o.materials->any) {   // Cutout / blend / user PS may discard pixels.
                bool solid = true;
                for (const Material& mt : o.materials->slots) {
                    if (mt.used && (mt.alpha_mode != 0 || mt.shader == 1)) {
                        solid = false;
                        break;
                    }
                }
                if (!solid) continue;
            }
            f = floor(o);
            if (!(f < VOC_FLOOR_REFUSE)) continue;   // NaN refuses too.
            // VOC_BURIAL: a ray from the camera to the clipped box meets the drawn front face on the way, between
            // two points above the floor - above it too, so never discarded by the endpoint test - only while the
            // camera is above the floor.
            if (f > VOC_FLOOR_NONE && !(cam[1] > f)) continue;
        }
        // Camera inside / touching the bounds (outside a closed mesh's it has winding 0).
        {
            float bc[3], bh[3], bmn[3], bmx[3];
            occ_box_of(o, force, bc, bh, bmn, bmx);
            if (!(occ_bounds_dist_sq(o, bmn, bmx, cam) > 0.04f)) continue;
        }
        float he[3];
        world_half_extents(o, he);
        const float r2 = he[0] * he[0] + he[1] * he[1] + he[2] * he[2];
        const float d2 = mesh_dist_sq(o, cam);
        if (!(d2 > 1.0e-4f)) continue;
        const float s = r2 / d2;
        // A declared occluder has no size floor. NaN fails here too.
        if (!(s >= (force ? 0.0f : VOC_MIN_SCORE))) continue;
        // OCC_FORCE: the rank key puts every declared occluder ahead of every automatic one, each kind by its
        // score (s / (1 + s) keeps the order in [0, 1); a declared one adds 2).
        const double k = (force ? 2.0 : 0.0) +
                         static_cast<double>(s) / (1.0 + static_cast<double>(s));
        int at = n < VOC_MAX ? n : VOC_MAX - 1;
        if (n == VOC_MAX && !(k > sc[at])) continue;
        while (at > 0 && sc[at - 1] < k) {
            sc[at] = sc[at - 1];
            id[at] = id[at - 1];
            fl[at] = fl[at - 1];
            --at;
        }
        sc[at] = k;
        id[at] = i;
        fl[at] = f;
        if (n < VOC_MAX) ++n;
    }
    if (n == 0) return;
    for (int i = 0; i < VOC_W * VOC_H; ++i) vo.z[i] = 1.0e18f;
    int budget = 0;
    uint64_t drawn = 0;   // OCC_STATS: boxes that covered a cell.
    for (int k = 0; k < n && budget <= VOC_PX_BUDGET; ++k) {
        const RenderObject& o = id[k] < mn ? m[id[k]]
                                                          : x[id[k] - mn];
        const bool force = o.occluder;
        float bc[3], bh[3], bmn[3], bmx[3];
        occ_box_of(o, force, bc, bh, bmn, bmx);
        const float se[3] = { o.size[0], o.size[2], o.size[1] };   // Engine edge lengths.
        float ctr[3];
        obj_center_engine(o, ctr);
        // VOC_BURIAL: the part of the box above the floor. World height is linear in the box's normalised
        // coordinates (y = ctr.y + sum a_k p_k); the axis with the steepest rise is cut at the height its lowest
        // corner must keep with every other axis at its lowest - a box standing upright loses its buried slab
        // exactly; nothing left, no occluder.
        if (fl[k] > VOC_FLOOR_NONE) {
            float a[3];
            int ku = 0;
            for (int rr = 0; rr < 3; ++rr) {
                a[rr] = se[rr] * o.rot_m[rr * 3 + 1];
                if (fabsf(a[rr]) > fabsf(a[ku])) ku = rr;
            }
            if (!(fabsf(a[ku]) > 1.0e-6f)) continue;
            float low = ctr[1];   // The lowest the other axes can put it.
            for (int rr = 0; rr < 3; ++rr) {
                if (rr == ku) continue;
                low += a[rr] * bc[rr] - fabsf(a[rr]) * bh[rr];
            }
            const float fc = (fl[k] - low) / a[ku];
            float lo = bc[ku] - bh[ku], hi = bc[ku] + bh[ku];
            if (a[ku] > 0.0f) lo = fmaxf(lo, fc);
            else                        hi = fminf(hi, fc);
            if (!(hi - lo > 1.0e-4f)) continue;
            bc[ku] = 0.5f * (lo + hi);
            bh[ku] = 0.5f * (hi - lo);
        }
        // The box's half-vectors (rows scaled by the engine edge lengths and its half extents), its centre moved onto
        // its own. A declared box is taken as it is: the inset keeps a drawn surface's box off that surface.
        const float inset = force ? 1.0f : VOC_INSET;
        float hv[3][3];
        for (int rr = 0; rr < 3; ++rr) {
            for (int c = 0; c < 3; ++c) {
                const float ax = se[rr] * o.rot_m[rr * 3 + c];
                hv[rr][c] = inset * bh[rr] * ax;
                ctr[c] += bc[rr] * ax;
            }
        }
        {   // POS_RES: drawn at pos plus its residual - every half-vector pulled in by the residual's length
            // (the rows are orthogonal), so the box lies inside the drawn one wherever the residual points; an
            // axis that would vanish takes the occluder out.
            float rs[3];
            obj_pos_res(o, rs);
            const float rl = sqrtf(rs[0] * rs[0] + rs[1] * rs[1] + rs[2] * rs[2]);
            bool gone = false;
            if (rl > 0.0f) {
                for (int rr = 0; rr < 3; ++rr) {
                    float* const h = hv[rr];
                    const float len = sqrtf(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
                    if (!(len > rl)) { gone = true; break; }
                    const float f = 1.0f - rl / len;
                    h[0] *= f; h[1] *= f; h[2] *= f;
                }
            }
            if (gone) continue;
        }
        float dmn = 1.0e18f, dmx = 0.0f;
        for (int s8 = 0; s8 < 8; ++s8) {
            const float sx = (s8 & 1) ? 1.0f : -1.0f;
            const float sy = (s8 & 2) ? 1.0f : -1.0f;
            const float sz = (s8 & 4) ? 1.0f : -1.0f;
            float d2c = 0.0f;
            for (int c = 0; c < 3; ++c) {
                const float w = ctr[c] + sx * hv[0][c] +
                                sy * hv[1][c] + sz * hv[2][c] - cam[c];
                d2c += w * w;
            }
            const float dc = sqrtf(d2c);
            if (dc < dmn) dmn = dc;
            if (dc > dmx) dmx = dc;
        }
        if (!(dmn > VOC_NEAR_M)) continue;
        if (!force) {   // Its own far contract may discard it: keep it well inside.
            float lim = vo_far;
            if (cut > 0.0f && cut < lim) lim = cut;
            if (lim > 0.0f && !(dmx < 0.98f * lim)) continue;
        }
        const int b0 = budget;
        voc_box(vo, ctr, hv, cam, budget);   // VOC_NEAR_CLIP, VOC_UNION.
        if (budget > b0) ++drawn;
    }
    stat_add(g_stats.vis_occluders, drawn);
    vo.on = budget > 0;
}

// True = provably hidden. Every failure direction is 'visible'. LOD_BOUNDS: the object is taken at its bounds
// size, which holds everything any of its levels draws, so no drawn corner falls outside the cells tested or nearer
// than the distance they are tested at.
inline bool vis_occ_hidden(const VisOcc& vh_o, const RenderObject& o) {
    if (!vh_o.on) return false;
    float ctr[3];
    obj_center_engine(o, ctr);
    float he[3];
    world_half_extents(o, he);
    const float r = sqrtf(he[0] * he[0] + he[1] * he[1] + he[2] * he[2]);
    float d2 = 0.0f;
    for (int c = 0; c < 3; ++c) {
        const float w = ctr[c] - vh_o.cam[c];
        d2 += w * w;
    }
    const float mn = sqrtf(d2) - r - VOC_EPS_M;   // Nearest possible point.
    if (!(mn > VOC_NEAR_M)) return false;
    float x0 = 1.0e18f, x1 = -1.0e18f, y0 = 1.0e18f, y1 = -1.0e18f;
    for (int s = 0; s < 8; ++s) {
        const float wc[3] = {
            ctr[0] + ((s & 1) ? he[0] : -he[0]),
            ctr[1] + ((s & 2) ? he[1] : -he[1]),
            ctr[2] + ((s & 4) ? he[2] : -he[2]) };
        float px, py;
        if (!voc_project(vh_o, wc, px, py)) return false;
        if (px < x0) x0 = px;
        if (px > x1) x1 = px;
        if (py < y0) y0 = py;
        if (py > y1) y1 = py;
    }
    // VOC_CAST_CLAMP: clamped to the grid (one cell of margin) before the casts, as voc_face does - a huge
    // finite corner converted out of int range (UB). A clamped bound still fails the range test below.
    const float gw = static_cast<float>(VOC_W), gh = static_cast<float>(VOC_H);
    const int ix0 = static_cast<int>(floorf(fminf(fmaxf(x0 - 0.01f, -1.0f), gw)));
    const int iy0 = static_cast<int>(floorf(fminf(fmaxf(y0 - 0.01f, -1.0f), gh)));
    const int ix1 = static_cast<int>(floorf(fminf(fmaxf(x1 + 0.01f, -1.0f), gw)));
    const int iy1 = static_cast<int>(floorf(fminf(fmaxf(y1 + 0.01f, -1.0f), gh)));
    if (ix0 < 0 || iy0 < 0 || ix1 >= VOC_W || iy1 >= VOC_H) return false;
    if ((ix1 - ix0 + 1) * (iy1 - iy0 + 1) > VOC_TEST_PX) return false;
    for (int iy = iy0; iy <= iy1; ++iy) {
        for (int ix = ix0; ix <= ix1; ++ix) {
            // An uncovered cell holds 1e18 and reads visible here too.
            if (mn <= vh_o.z[iy * VOC_W + ix] + vh_o.margin) return false;
        }
    }
    return true;
}

inline float voc_floor(const RenderObject& o) {
    if (!g_thm.valid) return VOC_FLOOR_NONE;   // Discard lanes inert (fill_occ): nothing to defend.
    if (!g_voc_thm.valid || !(g_voc_thm.cell > 0.0f) ||
        g_voc_thm.fw == 0 || g_voc_thm.fh == 0) {
        return VOC_FLOOR_REFUSE;   // Armed discard, no certificate: refuse.
    }
    float c[3], he[3];
    occ_hull_world(o, c, he);   // OCC_BOX: everything the occluder draws.
    const float inv = 1.0f / g_voc_thm.cell;
    const float gx0 = (c[0] - he[0] - g_voc_thm.origin[0]) * inv;
    const float gx1 = (c[0] + he[0] - g_voc_thm.origin[0]) * inv;
    const float gz0 = (c[2] - he[2] - g_voc_thm.origin[1]) * inv;
    const float gz1 = (c[2] + he[2] - g_voc_thm.origin[1]) * inv;
    if (!(gx0 <= gx1) || !(gz0 <= gz1) ||
        !(gx1 - gx0 < 1.0e7f) || !(gz1 - gz0 < 1.0e7f)) {
        return VOC_FLOOR_REFUSE;   // NaN / absurd extents: refuse before any int cast.
    }
    // KhThmHeight answers -1e6 outside the grid (no discard evidence), so a footprint fully outside cannot be eaten.
    if (gx1 < -1.0f || gz1 < -1.0f ||
        gx0 > static_cast<float>(g_voc_thm.fw) ||
        gz0 > static_cast<float>(g_voc_thm.fh)) {
        return VOC_FLOOR_NONE;
    }
    // Fine-node range under the footprint plus one node each way (the support of a sample at g - its cell's corners -
    // touches floor(g) and floor(g) + 1), clamped in float so the casts stay in range.
    const float fx = static_cast<float>(g_voc_thm.fw - 1);
    const float fz = static_cast<float>(g_voc_thm.fh - 1);
    const int x0 = static_cast<int>(fmaxf(floorf(gx0) - 1.0f, 0.0f));
    const int x1 = static_cast<int>(fminf(floorf(gx1) + 1.0f, fx));
    const int z0 = static_cast<int>(fmaxf(floorf(gz0) - 1.0f, 0.0f));
    const int z1 = static_cast<int>(fminf(floorf(gz1) + 1.0f, fz));
    float hmax = -1.0e6f;
    if ((x1 - x0 + 1) * (z1 - z0 + 1) <= VOC_THM_FINE_CAP) {
        for (int z = z0; z <= z1; ++z) {
            const float* row = g_voc_thm.fine.data() + static_cast<size_t>(z) * g_voc_thm.fw;
            for (int x = x0; x <= x1; ++x) {
                if (row[x] > hmax) hmax = row[x];
            }
        }
    } else {
        const int cx0 = x0 / static_cast<int>(VOC_THM_DS);
        const int cx1 = x1 / static_cast<int>(VOC_THM_DS);
        const int cz0 = z0 / static_cast<int>(VOC_THM_DS);
        const int cz1 = z1 / static_cast<int>(VOC_THM_DS);
        if ((cx1 - cx0 + 1) * (cz1 - cz0 + 1) > VOC_THM_CAP) {
            return VOC_FLOOR_REFUSE;   // Too wide to certify at this budget: refuse.
        }
        for (int cz = cz0; cz <= cz1; ++cz) {
            const float* row = g_voc_thm.max_grid.data() +
                               static_cast<size_t>(cz) * g_voc_thm.w;
            for (int cx = cx0; cx <= cx1; ++cx) {
                if (row[cx] > hmax) hmax = row[cx];
            }
        }
    }
    // The discard line anywhere under the footprint, with 5 cm of headroom over the shader's strict compare: a floor
    // only when the box's lowest point could sit below it.
    const float fl = hmax - THM_BIAS_M + 0.05f;
    return c[1] - he[1] < fl ? fl : VOC_FLOOR_NONE;
}

// Fills the heightfield lanes (thm_params, thm_meta) of the guard CB tail while a heightfield is live; untouched
// (zeroed = inert) otherwise. Called at three fills: the injection's frame CB (inject_composited_meshes), the
// flush's frame template (fx_frame_template) and the engine cast mask's (mask_cast_engine).
inline void fill_occ(ConstantData& cbd) {
    if (!g_thm.valid) return;   // Heightfield lanes stay zeroed = lane inert.
    cbd.thm_params[0] = g_thml.origin[0];
    cbd.thm_params[1] = g_thml.origin[1];
    cbd.thm_params[2] = g_thml.cell;
    cbd.thm_params[3] = 1.0f + static_cast<float>(g_thml.split);   // THM_EXACT: enabled, and the split.
    cbd.thm_meta[0] = static_cast<float>(g_thml.w);
    cbd.thm_meta[1] = static_cast<float>(g_thml.h);
    cbd.thm_meta[2] = THM_BIAS_M;
    cbd.thm_meta[3] = THM_MIN_DIST_M;
}
}