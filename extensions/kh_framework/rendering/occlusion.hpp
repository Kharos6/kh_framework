#pragma once

// The feature half (rendering_integration_features.hpp): the visibility occluders (KH_VOC) and the occluder floor.
// Never include it on its own.
namespace RenderIntegration {

inline bool kh_voc_project(const KhVisOcc& khvp_o, const float khvp_w[3], float& khvp_x, float& khvp_y) {
    const float khvp_q[3] = { khvp_w[0] + khvp_o.bias[0], khvp_w[1] + khvp_o.bias[1],
                              khvp_w[2] + khvp_o.bias[2] };
    float khvp_c[4];
    for (int khvp_j = 0; khvp_j < 4; ++khvp_j) {
        khvp_c[khvp_j] = khvp_o.vp[3][khvp_j];
        for (int khvp_i = 0; khvp_i < 3; ++khvp_i) khvp_c[khvp_j] += khvp_q[khvp_i] * khvp_o.vp[khvp_i][khvp_j];
    }
    if (!(khvp_c[3] > 0.05f)) return false;   // Behind / at the near plane: fail toward visible.
    const float khvp_iw = 1.0f / khvp_c[3];
    khvp_x = (khvp_c[0] * khvp_iw * 0.5f + 0.5f) * static_cast<float>(KH_VOC_W);
    khvp_y = (0.5f - khvp_c[1] * khvp_iw * 0.5f) * static_cast<float>(KH_VOC_H);
    if (!(khvp_x == khvp_x) || !(khvp_y == khvp_y)) return false;   // NaN-safe.
    return true;
}

// Inner-conservative rasterization of a convex polygon (kh_voc_box's faces, at most six corners): a cell counts only
// when all four of its corners are inside every edge. Edge signs normalized against the centroid, so winding never
// matters; a near-degenerate polygon stands the face down.
inline void kh_voc_face(KhVisOcc& khvf_o, const float khvf_p[][2], int khvf_n, float khvf_z, int& khvf_budget) {
    if (khvf_n < 3 || khvf_n > 6) return;
    float khvf_cx = 0.0f, khvf_cy = 0.0f;
    float khvf_mnx = khvf_p[0][0], khvf_mxx = khvf_p[0][0];
    float khvf_mny = khvf_p[0][1], khvf_mxy = khvf_p[0][1];
    const float khvf_in = 1.0f / static_cast<float>(khvf_n);
    for (int khvf_i = 0; khvf_i < khvf_n; ++khvf_i) {
        khvf_cx += khvf_p[khvf_i][0] * khvf_in;
        khvf_cy += khvf_p[khvf_i][1] * khvf_in;
        if (khvf_p[khvf_i][0] < khvf_mnx) khvf_mnx = khvf_p[khvf_i][0];
        if (khvf_p[khvf_i][0] > khvf_mxx) khvf_mxx = khvf_p[khvf_i][0];
        if (khvf_p[khvf_i][1] < khvf_mny) khvf_mny = khvf_p[khvf_i][1];
        if (khvf_p[khvf_i][1] > khvf_mxy) khvf_mxy = khvf_p[khvf_i][1];
    }
    float khvf_a[6], khvf_b[6], khvf_c[6];
    for (int khvf_e = 0; khvf_e < khvf_n; ++khvf_e) {
        const float* khvf_p0 = khvf_p[khvf_e];
        const float* khvf_p1 = khvf_p[(khvf_e + 1) % khvf_n];
        khvf_a[khvf_e] = khvf_p0[1] - khvf_p1[1];
        khvf_b[khvf_e] = khvf_p1[0] - khvf_p0[0];
        khvf_c[khvf_e] = khvf_p0[0] * khvf_p1[1] - khvf_p1[0] * khvf_p0[1];
        const float khvf_ec = khvf_a[khvf_e] * khvf_cx + khvf_b[khvf_e] * khvf_cy + khvf_c[khvf_e];
        if (!(khvf_ec > 1.0e-4f || khvf_ec < -1.0e-4f)) return;   // Degenerate: stand down.
        if (khvf_ec < 0.0f) { khvf_a[khvf_e] = -khvf_a[khvf_e]; khvf_b[khvf_e] = -khvf_b[khvf_e]; khvf_c[khvf_e] = -khvf_c[khvf_e]; }
    }
    // The bounds clamped to the grid (one cell of margin) before the casts, on both sides: a corner projected near
    // the camera plane lands far off it.
    const float khvf_gw = static_cast<float>(KH_VOC_W), khvf_gh = static_cast<float>(KH_VOC_H);
    int khvf_x0 = static_cast<int>(floorf(fminf(fmaxf(khvf_mnx, -1.0f), khvf_gw))); if (khvf_x0 < 0) khvf_x0 = 0;
    int khvf_y0 = static_cast<int>(floorf(fminf(fmaxf(khvf_mny, -1.0f), khvf_gh))); if (khvf_y0 < 0) khvf_y0 = 0;
    int khvf_x1 = static_cast<int>(floorf(fminf(fmaxf(khvf_mxx, -1.0f), khvf_gw)));
    if (khvf_x1 > KH_VOC_W - 1) khvf_x1 = KH_VOC_W - 1;
    int khvf_y1 = static_cast<int>(floorf(fminf(fmaxf(khvf_mxy, -1.0f), khvf_gh)));
    if (khvf_y1 > KH_VOC_H - 1) khvf_y1 = KH_VOC_H - 1;
    for (int khvf_iy = khvf_y0; khvf_iy <= khvf_y1; ++khvf_iy) {
        for (int khvf_ix = khvf_x0; khvf_ix <= khvf_x1; ++khvf_ix) {
            bool khvf_inside = true;
            for (int khvf_e = 0; khvf_e < khvf_n && khvf_inside; ++khvf_e) {
                const float khvf_mn = khvf_a[khvf_e] * static_cast<float>(khvf_ix) +
                                      khvf_b[khvf_e] * static_cast<float>(khvf_iy) + khvf_c[khvf_e] +
                                      (khvf_a[khvf_e] < 0.0f ? khvf_a[khvf_e] : 0.0f) +
                                      (khvf_b[khvf_e] < 0.0f ? khvf_b[khvf_e] : 0.0f);
                if (!(khvf_mn > 0.0f)) khvf_inside = false;
            }
            if (!khvf_inside) continue;
            float& khvf_slot = khvf_o.z[khvf_iy * KH_VOC_W + khvf_ix];
            if (khvf_z < khvf_slot) khvf_slot = khvf_z;
            if (++khvf_budget > KH_VOC_PX_BUDGET) return;
        }
    }
}
inline float kh_voc_poly_area(const KhVocPoly& khpa_p) {
    float khpa_a = 0.0f;
    for (int khpa_i = 0; khpa_i < khpa_p.n; ++khpa_i) {
        const int khpa_j = (khpa_i + 1) % khpa_p.n;
        khpa_a += khpa_p.x[khpa_i] * khpa_p.y[khpa_j] - khpa_p.x[khpa_j] * khpa_p.y[khpa_i];
    }
    return 0.5f * fabsf(khpa_a);
}
// The part of khpc_in where a x + b y + c >= 0 (coordinates relative to the cell, c already shifted to them).
// Sutherland-Hodgman against one line.
inline void kh_voc_poly_clip(const KhVocPoly& khpc_in, float khpc_a, float khpc_b, float khpc_c, KhVocPoly& khpc_out) {
    khpc_out.n = 0;
    for (int khpc_i = 0; khpc_i < khpc_in.n; ++khpc_i) {
        const int khpc_j = (khpc_i + 1) % khpc_in.n;
        const float khpc_v0 = khpc_a * khpc_in.x[khpc_i] + khpc_b * khpc_in.y[khpc_i] + khpc_c;
        const float khpc_v1 = khpc_a * khpc_in.x[khpc_j] + khpc_b * khpc_in.y[khpc_j] + khpc_c;
        if (khpc_v0 >= 0.0f && khpc_out.n < 16) {
            khpc_out.x[khpc_out.n] = khpc_in.x[khpc_i]; khpc_out.y[khpc_out.n] = khpc_in.y[khpc_i]; ++khpc_out.n;
        }
        if ((khpc_v0 >= 0.0f) != (khpc_v1 >= 0.0f) && khpc_out.n < 16) {
            const float khpc_t = khpc_v0 / (khpc_v0 - khpc_v1);
            khpc_out.x[khpc_out.n] = khpc_in.x[khpc_i] + khpc_t * (khpc_in.x[khpc_j] - khpc_in.x[khpc_i]);
            khpc_out.y[khpc_out.n] = khpc_in.y[khpc_i] + khpc_t * (khpc_in.y[khpc_j] - khpc_in.y[khpc_i]);
            ++khpc_out.n;
        }
    }
}
// One occluder box into the grid (centre khvb_ctr, half-vectors khvb_hv, engine axes): its front faces that are not
// grazing (KH_VOC_FACE_COS), each clipped at the near depth (KH_VOC_NEAR_CLIP), rasterized one by one (kh_voc_face),
// then the cells along the edges between them (KH_VOC_UNION).
inline void kh_voc_box(KhVisOcc& khvb_o, const float khvb_ctr[3], const float khvb_hv[3][3],
                       const float khvb_cam[3], int& khvb_budget) {
    // Points 0..7 the corners (bit k = the sign along axis k), 8..19 the near clip on box edge 8 + 4 ax + (the other
    // two axes' bits).
    float khvb_w[20][3], khvb_dp[20], khvb_ds[20], khvb_p[20][2];
    bool khvb_have[20] = {}, khvb_proj[20] = {};
    auto khvb_depth = [&](const float* khvb_x) {   // The projection's w (kh_voc_project's).
        float khvb_c = khvb_o.vp[3][3];
        for (int khvb_i = 0; khvb_i < 3; ++khvb_i) {
            khvb_c += (khvb_x[khvb_i] + khvb_o.bias[khvb_i]) * khvb_o.vp[khvb_i][3];
        }
        return khvb_c;
    };
    auto khvb_finish = [&](int khvb_id) {
        float khvb_dd = 0.0f;
        for (int khvb_c = 0; khvb_c < 3; ++khvb_c) {
            const float khvb_r = khvb_w[khvb_id][khvb_c] - khvb_cam[khvb_c];
            khvb_dd += khvb_r * khvb_r;
        }
        khvb_ds[khvb_id] = sqrtf(khvb_dd);
        khvb_proj[khvb_id] = khvb_dp[khvb_id] >= KH_VOC_W_MIN &&
                             kh_voc_project(khvb_o, khvb_w[khvb_id], khvb_p[khvb_id][0], khvb_p[khvb_id][1]);
        khvb_have[khvb_id] = true;
    };
    for (int khvb_s = 0; khvb_s < 8; ++khvb_s) {
        for (int khvb_c = 0; khvb_c < 3; ++khvb_c) {
            khvb_w[khvb_s][khvb_c] = khvb_ctr[khvb_c] + ((khvb_s & 1) ? 1.0f : -1.0f) * khvb_hv[0][khvb_c] +
                                     ((khvb_s & 2) ? 1.0f : -1.0f) * khvb_hv[1][khvb_c] +
                                     ((khvb_s & 4) ? 1.0f : -1.0f) * khvb_hv[2][khvb_c];
        }
        khvb_dp[khvb_s] = khvb_depth(khvb_w[khvb_s]);
        khvb_finish(khvb_s);
    }
    auto khvb_front = [&](int khvb_s) { return khvb_dp[khvb_s] >= KH_VOC_W_MIN; };
    // The clip point of the edge between corners s0 and s1 (one bit apart).
    auto khvb_clip = [&](int khvb_s0, int khvb_s1) -> int {
        if (khvb_s0 > khvb_s1) { const int khvb_t = khvb_s0; khvb_s0 = khvb_s1; khvb_s1 = khvb_t; }
        const int khvb_ax = (khvb_s0 ^ khvb_s1) == 1 ? 0 : ((khvb_s0 ^ khvb_s1) == 2 ? 1 : 2);
        const int khvb_u = (khvb_ax + 1) % 3, khvb_v = (khvb_ax + 2) % 3;
        const int khvb_id = 8 + 4 * khvb_ax + ((khvb_s0 >> khvb_u) & 1) + 2 * ((khvb_s0 >> khvb_v) & 1);
        if (!khvb_have[khvb_id]) {
            const float khvb_t = (KH_VOC_W_MIN - khvb_dp[khvb_s0]) / (khvb_dp[khvb_s1] - khvb_dp[khvb_s0]);
            for (int khvb_c = 0; khvb_c < 3; ++khvb_c) {
                const float khvb_e = khvb_w[khvb_s1][khvb_c] - khvb_w[khvb_s0][khvb_c];
                khvb_w[khvb_id][khvb_c] = khvb_w[khvb_s0][khvb_c] + khvb_t * khvb_e;
            }
            khvb_dp[khvb_id] = KH_VOC_W_MIN;   // By construction; the projection re-derives its own w.
            khvb_finish(khvb_id);
        }
        return khvb_id;
    };
    // The faces: point ids in ring order, their edge lines (inside-positive, the centroid decides as kh_voc_face) and
    // the farthest point's distance.
    struct KhVbFace { int ax, sg, n, id[6]; float a[6], b[6], c[6], z; };
    KhVbFace khvb_f[6];
    int khvb_nf = 0;
    static const int khvb_ring[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
    for (int khvb_ax = 0; khvb_ax < 3 && khvb_budget <= KH_VOC_PX_BUDGET; ++khvb_ax) {
        for (int khvb_si = 0; khvb_si < 2 && khvb_budget <= KH_VOC_PX_BUDGET; ++khvb_si) {
            const float khvb_fs = khvb_si ? 1.0f : -1.0f;
            float khvb_nl2 = 0.0f, khvb_vl2 = 0.0f, khvb_nv = 0.0f;
            for (int khvb_c = 0; khvb_c < 3; ++khvb_c) {
                const float khvb_fc = khvb_ctr[khvb_c] + khvb_fs * khvb_hv[khvb_ax][khvb_c];
                const float khvb_vw = khvb_fc - khvb_cam[khvb_c];
                khvb_nl2 += khvb_hv[khvb_ax][khvb_c] * khvb_hv[khvb_ax][khvb_c];
                khvb_vl2 += khvb_vw * khvb_vw;
                khvb_nv += khvb_vw * khvb_fs * khvb_hv[khvb_ax][khvb_c];
            }
            if (!(khvb_nl2 > 1.0e-12f) || !(khvb_vl2 > 1.0e-6f)) continue;
            if (!(khvb_nv / (sqrtf(khvb_vl2) * sqrtf(khvb_nl2)) < -KH_VOC_FACE_COS)) continue;
            const int khvb_u = (khvb_ax + 1) % 3, khvb_v = (khvb_ax + 2) % 3;
            int khvb_q[4];
            for (int khvb_k = 0; khvb_k < 4; ++khvb_k) {
                khvb_q[khvb_k] = (khvb_si << khvb_ax) | ((khvb_ring[khvb_k][0] > 0 ? 1 : 0) << khvb_u) |
                                 ((khvb_ring[khvb_k][1] > 0 ? 1 : 0) << khvb_v);
            }
            KhVbFace& khvb_g = khvb_f[khvb_nf];
            khvb_g.n = 0;
            for (int khvb_k = 0; khvb_k < 4; ++khvb_k) {
                const int khvb_a0 = khvb_q[khvb_k], khvb_a1 = khvb_q[(khvb_k + 1) & 3];
                if (khvb_front(khvb_a0)) khvb_g.id[khvb_g.n++] = khvb_a0;
                if (khvb_front(khvb_a0) != khvb_front(khvb_a1)) khvb_g.id[khvb_g.n++] = khvb_clip(khvb_a0, khvb_a1);
            }
            if (khvb_g.n < 3) continue;   // Wholly behind the near depth.
            bool khvb_ok = true;
            float khvb_pts[6][2];
            khvb_g.z = 0.0f;
            for (int khvb_k = 0; khvb_k < khvb_g.n; ++khvb_k) {
                const int khvb_id = khvb_g.id[khvb_k];
                if (!khvb_proj[khvb_id]) khvb_ok = false;
                khvb_pts[khvb_k][0] = khvb_p[khvb_id][0];
                khvb_pts[khvb_k][1] = khvb_p[khvb_id][1];
                if (khvb_ds[khvb_id] > khvb_g.z) khvb_g.z = khvb_ds[khvb_id];
            }
            if (!khvb_ok) continue;
            kh_voc_face(khvb_o, khvb_pts, khvb_g.n, khvb_g.z, khvb_budget);
            // The same lines for the union (kh_voc_face's arithmetic on the same points).
            float khvb_cx = 0.0f, khvb_cy = 0.0f;
            const float khvb_in = 1.0f / static_cast<float>(khvb_g.n);
            for (int khvb_k = 0; khvb_k < khvb_g.n; ++khvb_k) {
                khvb_cx += khvb_pts[khvb_k][0] * khvb_in;
                khvb_cy += khvb_pts[khvb_k][1] * khvb_in;
            }
            for (int khvb_e = 0; khvb_e < khvb_g.n && khvb_ok; ++khvb_e) {
                const float* khvb_p0 = khvb_pts[khvb_e];
                const float* khvb_p1 = khvb_pts[(khvb_e + 1) % khvb_g.n];
                float khvb_a = khvb_p0[1] - khvb_p1[1], khvb_b = khvb_p1[0] - khvb_p0[0];
                float khvb_c = khvb_p0[0] * khvb_p1[1] - khvb_p1[0] * khvb_p0[1];
                const float khvb_ec = khvb_a * khvb_cx + khvb_b * khvb_cy + khvb_c;
                if (!(khvb_ec > 1.0e-4f || khvb_ec < -1.0e-4f)) khvb_ok = false;   // Degenerate: no union.
                if (khvb_ec < 0.0f) { khvb_a = -khvb_a; khvb_b = -khvb_b; khvb_c = -khvb_c; }
                khvb_g.a[khvb_e] = khvb_a; khvb_g.b[khvb_e] = khvb_b; khvb_g.c[khvb_e] = khvb_c;
            }
            if (!khvb_ok) continue;
            khvb_g.ax = khvb_ax; khvb_g.sg = khvb_si;
            ++khvb_nf;
        }
    }
    if (khvb_nf < 2 || khvb_budget > KH_VOC_PX_BUDGET) return;
    uint8_t khvb_seen[KH_VOC_W * KH_VOC_H] = {};
    // Is the cell (ix, iy) inside the faces' union? Its farthest meeting face's distance in khvb_zc.
    auto khvb_cover = [&](int khvb_ix, int khvb_iy, float& khvb_zc) -> bool {
        KhVocPoly khvb_pc[KH_VOC_UNION_PIECES], khvb_nx[KH_VOC_UNION_PIECES];
        int khvb_np = 1, khvb_nn = 0;
        khvb_pc[0].n = 4;
        khvb_pc[0].x[0] = 0.0f; khvb_pc[0].y[0] = 0.0f; khvb_pc[0].x[1] = 1.0f; khvb_pc[0].y[1] = 0.0f;
        khvb_pc[0].x[2] = 1.0f; khvb_pc[0].y[2] = 1.0f; khvb_pc[0].x[3] = 0.0f; khvb_pc[0].y[3] = 1.0f;
        khvb_zc = 0.0f;
        for (int khvb_i = 0; khvb_i < khvb_nf && khvb_np > 0; ++khvb_i) {
            const KhVbFace& khvb_g = khvb_f[khvb_i];
            khvb_nn = 0;
            bool khvb_meets = false;
            for (int khvb_j = 0; khvb_j < khvb_np; ++khvb_j) {
                KhVocPoly khvb_cur = khvb_pc[khvb_j];
                for (int khvb_e = 0; khvb_e < khvb_g.n && khvb_cur.n > 0; ++khvb_e) {
                    // The line in the cell's own coordinates (x - ix, y - iy).
                    const float khvb_cc = khvb_g.c[khvb_e] + khvb_g.a[khvb_e] * static_cast<float>(khvb_ix) +
                                          khvb_g.b[khvb_e] * static_cast<float>(khvb_iy);
                    KhVocPoly khvb_out;
                    kh_voc_poly_clip(khvb_cur, -khvb_g.a[khvb_e], -khvb_g.b[khvb_e], -khvb_cc, khvb_out);
                    if (khvb_out.n >= 3 && kh_voc_poly_area(khvb_out) > KH_VOC_UNION_EPS) {
                        if (khvb_nn == KH_VOC_UNION_PIECES) return false;   // Out of room: not covered.
                        khvb_nx[khvb_nn++] = khvb_out;
                    }
                    KhVocPoly khvb_in;
                    kh_voc_poly_clip(khvb_cur, khvb_g.a[khvb_e], khvb_g.b[khvb_e], khvb_cc, khvb_in);
                    khvb_cur = khvb_in;
                    if (khvb_cur.n < 3 || !(kh_voc_poly_area(khvb_cur) > KH_VOC_UNION_EPS)) khvb_cur.n = 0;
                }
                if (khvb_cur.n > 0) khvb_meets = true;
            }
            if (khvb_meets && khvb_g.z > khvb_zc) khvb_zc = khvb_g.z;
            for (int khvb_j = 0; khvb_j < khvb_nn; ++khvb_j) khvb_pc[khvb_j] = khvb_nx[khvb_j];
            khvb_np = khvb_nn;
        }
        return khvb_np == 0 && khvb_zc > 0.0f;
    };
    // The fast exact test for a cell the edge between faces g and h crosses (edge ge of g, he of h).
    auto khvb_pair = [&](const KhVbFace& khvb_g, int khvb_ge, const KhVbFace& khvb_h, int khvb_he,
                         int khvb_ix, int khvb_iy) -> bool {
        // The lines in the cell's own coordinates; g's shared line is h's negated.
        float khvb_gc[6], khvb_hc[6];
        for (int khvb_e = 0; khvb_e < khvb_g.n; ++khvb_e) {
            khvb_gc[khvb_e] = khvb_g.c[khvb_e] + khvb_g.a[khvb_e] * static_cast<float>(khvb_ix) +
                              khvb_g.b[khvb_e] * static_cast<float>(khvb_iy);
        }
        for (int khvb_e = 0; khvb_e < khvb_h.n; ++khvb_e) {
            khvb_hc[khvb_e] = khvb_h.c[khvb_e] + khvb_h.a[khvb_e] * static_cast<float>(khvb_ix) +
                              khvb_h.b[khvb_e] * static_cast<float>(khvb_iy);
        }
        auto khvb_in = [&](const KhVbFace& khvb_f2, const float* khvb_cs, int khvb_se, float khvb_x, float khvb_y) {
            for (int khvb_e = 0; khvb_e < khvb_f2.n; ++khvb_e) {
                if (khvb_e == khvb_se) continue;
                if (!(khvb_f2.a[khvb_e] * khvb_x + khvb_f2.b[khvb_e] * khvb_y + khvb_cs[khvb_e] > 0.0f)) return false;
            }
            return true;
        };
        // The vertices of the two sides of the cut: the square's corners on each side and the points where the line
        // crosses the square's edges.
        static const float khvb_sx[4] = { 0.0f, 1.0f, 1.0f, 0.0f }, khvb_sy[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        float khvb_v[4];
        for (int khvb_k = 0; khvb_k < 4; ++khvb_k) {
            khvb_v[khvb_k] = khvb_g.a[khvb_ge] * khvb_sx[khvb_k] + khvb_g.b[khvb_ge] * khvb_sy[khvb_k] +
                             khvb_gc[khvb_ge];
        }
        for (int khvb_k = 0; khvb_k < 4; ++khvb_k) {
            const float khvb_cx2 = khvb_sx[khvb_k], khvb_cy2 = khvb_sy[khvb_k];
            if (khvb_v[khvb_k] >= 0.0f && !khvb_in(khvb_g, khvb_gc, khvb_ge, khvb_cx2, khvb_cy2)) return false;
            if (khvb_v[khvb_k] <= 0.0f && !khvb_in(khvb_h, khvb_hc, khvb_he, khvb_cx2, khvb_cy2)) return false;
            const int khvb_n2 = (khvb_k + 1) & 3;
            if ((khvb_v[khvb_k] >= 0.0f) != (khvb_v[khvb_n2] >= 0.0f)) {
                const float khvb_t2 = khvb_v[khvb_k] / (khvb_v[khvb_k] - khvb_v[khvb_n2]);
                const float khvb_px = khvb_sx[khvb_k] + khvb_t2 * (khvb_sx[khvb_n2] - khvb_sx[khvb_k]);
                const float khvb_py = khvb_sy[khvb_k] + khvb_t2 * (khvb_sy[khvb_n2] - khvb_sy[khvb_k]);
                if (!khvb_in(khvb_g, khvb_gc, khvb_ge, khvb_px, khvb_py) ||
                    !khvb_in(khvb_h, khvb_hc, khvb_he, khvb_px, khvb_py)) return false;
            }
        }
        return true;
    };
    auto khvb_edge_of = [](const KhVbFace& khvb_f2, int khvb_a0, int khvb_a1) -> int {
        for (int khvb_e = 0; khvb_e < khvb_f2.n; ++khvb_e) {
            const int khvb_p0 = khvb_f2.id[khvb_e], khvb_p1 = khvb_f2.id[(khvb_e + 1) % khvb_f2.n];
            if ((khvb_p0 == khvb_a0 && khvb_p1 == khvb_a1) || (khvb_p0 == khvb_a1 && khvb_p1 == khvb_a0)) return khvb_e;
        }
        return -1;
    };
    // Every edge two of the faces share (its part in front of the near depth): the cells its projected segment
    // crosses, clipped to the grid first.
    for (int khvb_i = 0; khvb_i < khvb_nf; ++khvb_i) {
        for (int khvb_j = khvb_i + 1; khvb_j < khvb_nf; ++khvb_j) {
            const KhVbFace& khvb_g = khvb_f[khvb_i];
            const KhVbFace& khvb_h = khvb_f[khvb_j];
            if (khvb_g.ax == khvb_h.ax) continue;   // Opposite faces share nothing.
            const int khvb_t = 3 - khvb_g.ax - khvb_h.ax;
            const int khvb_s0 = (khvb_g.sg << khvb_g.ax) | (khvb_h.sg << khvb_h.ax);
            const int khvb_s1 = khvb_s0 | (1 << khvb_t);
            if (!khvb_front(khvb_s0) && !khvb_front(khvb_s1)) continue;   // Wholly behind the near depth.
            const int khvb_e0 = khvb_front(khvb_s0) ? khvb_s0 : khvb_clip(khvb_s0, khvb_s1);
            const int khvb_e1 = khvb_front(khvb_s1) ? khvb_s1 : khvb_clip(khvb_s0, khvb_s1);
            const int khvb_ge = khvb_edge_of(khvb_g, khvb_e0, khvb_e1);
            const int khvb_he = khvb_edge_of(khvb_h, khvb_e0, khvb_e1);
            if (khvb_ge < 0 || khvb_he < 0) continue;
            const float khvb_x0 = khvb_p[khvb_e0][0], khvb_y0 = khvb_p[khvb_e0][1];
            const float khvb_dx = khvb_p[khvb_e1][0] - khvb_x0, khvb_dy = khvb_p[khvb_e1][1] - khvb_y0;
            float khvb_t0 = 0.0f, khvb_t1 = 1.0f;   // Liang-Barsky against the grid, one cell of margin.
            const float khvb_lim[4] = { -1.0f - khvb_x0, static_cast<float>(KH_VOC_W) + 1.0f - khvb_x0,
                                        -1.0f - khvb_y0, static_cast<float>(KH_VOC_H) + 1.0f - khvb_y0 };
            const float khvb_dir[4] = { -khvb_dx, khvb_dx, -khvb_dy, khvb_dy };
            bool khvb_in = true;
            for (int khvb_k = 0; khvb_k < 4 && khvb_in; ++khvb_k) {
                const float khvb_pk = khvb_dir[khvb_k];
                const float khvb_qk = (khvb_k & 1) ? khvb_lim[khvb_k] : -khvb_lim[khvb_k];
                if (khvb_pk == 0.0f) { if (khvb_qk < 0.0f) khvb_in = false; continue; }
                const float khvb_r = khvb_qk / khvb_pk;
                if (khvb_pk < 0.0f) { if (khvb_r > khvb_t0) khvb_t0 = khvb_r; }
                else if (khvb_r < khvb_t1) khvb_t1 = khvb_r;
                if (khvb_t0 > khvb_t1) khvb_in = false;
            }
            if (!khvb_in) continue;
            const float khvb_len = (khvb_t1 - khvb_t0) * sqrtf(khvb_dx * khvb_dx + khvb_dy * khvb_dy);
            const int khvb_steps = static_cast<int>(fminf(khvb_len * 2.0f, 1024.0f)) + 1;
            for (int khvb_k = 0; khvb_k <= khvb_steps; ++khvb_k) {
                const float khvb_tt = khvb_t0 + (khvb_t1 - khvb_t0) * static_cast<float>(khvb_k) /
                                                static_cast<float>(khvb_steps);
                // KH_VOC_CAST_CLAMP: the clip keeps the point in [-1, W + 1] only up to float cancellation (a
                // corner far off the grid); clamped two cells out (NaN too) before the casts, where every
                // neighbour below is still off the grid and skipped.
                const int khvb_cx = static_cast<int>(floorf(fminf(fmaxf(khvb_x0 + khvb_tt * khvb_dx, -2.0f),
                                                                  static_cast<float>(KH_VOC_W) + 2.0f)));
                const int khvb_cy = static_cast<int>(floorf(fminf(fmaxf(khvb_y0 + khvb_tt * khvb_dy, -2.0f),
                                                                  static_cast<float>(KH_VOC_H) + 2.0f)));
                for (int khvb_oy = -1; khvb_oy <= 1; ++khvb_oy) {
                    for (int khvb_ox = -1; khvb_ox <= 1; ++khvb_ox) {
                        const int khvb_ix = khvb_cx + khvb_ox, khvb_iy = khvb_cy + khvb_oy;
                        if (khvb_ix < 0 || khvb_iy < 0 || khvb_ix >= KH_VOC_W || khvb_iy >= KH_VOC_H) continue;
                        uint8_t& khvb_m = khvb_seen[khvb_iy * KH_VOC_W + khvb_ix];
                        if (khvb_m) continue;
                        khvb_m = 1;
                        // Only a cell the edge itself crosses can be in the union and in neither face: off the
                        // edge's line the union on each side is one face. The segment against the cell's square
                        // (1e-3 wider).
                        {
                            float khvb_a0 = khvb_t0, khvb_a1 = khvb_t1;
                            const float khvb_bx[4] = { static_cast<float>(khvb_ix) - 1.0e-3f - khvb_x0,
                                                       static_cast<float>(khvb_ix) + 1.001f - khvb_x0,
                                                       static_cast<float>(khvb_iy) - 1.0e-3f - khvb_y0,
                                                       static_cast<float>(khvb_iy) + 1.001f - khvb_y0 };
                            bool khvb_hit = true;
                            for (int khvb_b = 0; khvb_b < 4 && khvb_hit; ++khvb_b) {
                                const float khvb_pb = khvb_dir[khvb_b];
                                const float khvb_qb = (khvb_b & 1) ? khvb_bx[khvb_b] : -khvb_bx[khvb_b];
                                if (khvb_pb == 0.0f) { if (khvb_qb < 0.0f) khvb_hit = false; continue; }
                                const float khvb_rb = khvb_qb / khvb_pb;
                                if (khvb_pb < 0.0f) { if (khvb_rb > khvb_a0) khvb_a0 = khvb_rb; }
                                else if (khvb_rb < khvb_a1) khvb_a1 = khvb_rb;
                                if (khvb_a0 > khvb_a1) khvb_hit = false;
                            }
                            if (!khvb_hit) continue;
                        }
                        float khvb_zc = khvb_g.z > khvb_h.z ? khvb_g.z : khvb_h.z;
                        if (!khvb_pair(khvb_g, khvb_ge, khvb_h, khvb_he, khvb_ix, khvb_iy) &&
                            !(khvb_nf > 2 && khvb_cover(khvb_ix, khvb_iy, khvb_zc))) continue;
                        float& khvb_slot = khvb_o.z[khvb_iy * KH_VOC_W + khvb_ix];
                        if (khvb_zc < khvb_slot) khvb_slot = khvb_zc;
                        if (++khvb_budget > KH_VOC_PX_BUDGET) return;
                    }
                }
            }
        }
    }
}
// KH_OCC_BOX / KH_OCC_FORCE: the box the cull rasterizes for an object and the bounds the camera must stay out of,
// normalised (engine axes): the mesh's proven box and its every level's bounds, or for a declared occluder its unit
// box for both. The transform is the vertex stage's: centre + R (p * size).
inline void kh_occ_box_of(const RenderObject& o, bool khob_force, float khob_c[3], float khob_h[3],
                          float khob_mn[3], float khob_mx[3]) {
    const MeshDef& khob_md = mesh_def(mesh_id_clamp(o.mesh));
    for (int k = 0; k < 3; ++k) {
        khob_c[k] = khob_force ? 0.0f : khob_md.occ_c[k];
        khob_h[k] = khob_force ? 0.5f : khob_md.occ_h[k];
        khob_mn[k] = khob_force ? -0.5f : khob_md.occ_hmn[k];
        khob_mx[k] = khob_force ? 0.5f : khob_md.occ_hmx[k];
    }
}
// Squared distance (metres^2) from the camera to those bounds, along the object's own axes. KH_OCC_ROW_SCALE: a row
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
inline float kh_occ_bounds_dist_sq(const RenderObject& o, const float khbd_mn[3], const float khbd_mx[3],
                                   const float cam[3]) {
    for (int i = 0; i < 3; ++i) {   // KH_OCC_ROW_SCALE: the orthogonality the measure needs.
        const float* const khbd_ri = o.rot_m + i * 3;
        const float* const khbd_rj = o.rot_m + ((i + 1) % 3) * 3;
        const float khbd_d = khbd_ri[0] * khbd_rj[0] + khbd_ri[1] * khbd_rj[1] + khbd_ri[2] * khbd_rj[2];
        const float khbd_ni = khbd_ri[0] * khbd_ri[0] + khbd_ri[1] * khbd_ri[1] + khbd_ri[2] * khbd_ri[2];
        const float khbd_nj = khbd_rj[0] * khbd_rj[0] + khbd_rj[1] * khbd_rj[1] + khbd_rj[2] * khbd_rj[2];
        if (khbd_d * khbd_d > 1.0e-6f * khbd_ni * khbd_nj) return 0.0f;
    }
    float khbd_c[3];
    kh_obj_center_engine(o, khbd_c);
    const float khbd_v[3] = { cam[0] - khbd_c[0], cam[1] - khbd_c[1], cam[2] - khbd_c[2] };
    const float khbd_se[3] = { o.size[0], o.size[2], o.size[1] };   // Engine edge lengths.
    float khbd_acc = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float* const khbd_r = o.rot_m + k * 3;
        const float khbd_n2 = khbd_r[0] * khbd_r[0] + khbd_r[1] * khbd_r[1] + khbd_r[2] * khbd_r[2];
        const float khbd_l = (khbd_v[0] * khbd_r[0] + khbd_v[1] * khbd_r[1] + khbd_v[2] * khbd_r[2]) / khbd_n2;
        const float khbd_a = khbd_mn[k] * khbd_se[k], khbd_b = khbd_mx[k] * khbd_se[k];
        const float khbd_e = khbd_l < khbd_a ? khbd_a - khbd_l : (khbd_l > khbd_b ? khbd_l - khbd_b : 0.0f);
        khbd_acc += khbd_e * khbd_e * khbd_n2;
    }
    return khbd_acc;
}
// The world-axis box (centre, half extents) around everything an object's mesh draws, at any level: the burial
// check's footprint.
inline void kh_occ_hull_world(const RenderObject& o, float khhw_c[3], float khhw_he[3]) {
    const MeshDef& khhw_md = mesh_def(mesh_id_clamp(o.mesh));
    kh_obj_center_engine(o, khhw_c);
    const float khhw_se[3] = { o.size[0], o.size[2], o.size[1] };
    khhw_he[0] = khhw_he[1] = khhw_he[2] = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float khhw_m = 0.5f * (khhw_md.occ_hmn[k] + khhw_md.occ_hmx[k]) * khhw_se[k];
        const float khhw_x = 0.5f * (khhw_md.occ_hmx[k] - khhw_md.occ_hmn[k]) * khhw_se[k];
        for (int c = 0; c < 3; ++c) {
            khhw_c[c] += khhw_m * o.rot_m[k * 3 + c];
            khhw_he[c] += khhw_x * fabsf(o.rot_m[k * 3 + c]);
        }
    }
}
// KH_OCC_BOX: whether this object draws its mesh as kh_mesh_occ_prep proved it. A skeletal binding, a cloth and a
// chain move their vertices every frame (the pass draws the skinned or simulated buffer - or the rest pose until that
// exists), and a KH_USER_VS vertex stage deforms them on the GPU: none is the geometry the box was proven on,
// whatever the buffer. Beyond those, the pass must bind the mesh's own buffer (any substitution kh_mesh_vb_ref
// makes), and the transform must keep the winding (positive sizes, a proper rotation - an attached basis may be
// mirrored).
inline bool kh_occ_drawn_as_built(const RenderObject& o) {
    if (o.skel || o.cloth_sim || o.chain_sim) return false;
    if (o.materials && o.materials->vertex_any) return false;
    const int khdb_mid = mesh_id_clamp(o.mesh);
    if (!mesh_def(khdb_mid).occ_ok) return false;
    if (khdb_mid < 0 || static_cast<size_t>(khdb_mid) >= g_res.mesh_vb.size() || !g_res.mesh_vb[khdb_mid] ||
        kh_mesh_vb_ref(khdb_mid, o.slot).vb != g_res.mesh_vb[khdb_mid]) return false;
    if (!(o.size[0] > 0.0f && o.size[1] > 0.0f && o.size[2] > 0.0f)) return false;
    const float* r = o.rot_m;
    const float khdb_det = r[0] * (r[4] * r[8] - r[5] * r[7]) - r[1] * (r[3] * r[8] - r[5] * r[6]) +
                           r[2] * (r[3] * r[7] - r[4] * r[6]);
    return khdb_det > 0.5f;
}

// Occluder pick + rasterize. khvo_m is the pass's own draw list, khvo_x the declared occluders it does not draw (one
// index space, khvo_m first). khvo_margin covers passes whose PS may write nearer than their geometry (far-arb
// terrain clamp); khvo_far / khvo_cut bound an occluder to where its far contract cannot discard it. khvo_floor = the
// height (engine y) below which this object may fail to depth-write a pixel the grid would count, the terrain
// march's discards aside (KH_VOC_BURIAL says why they are safe): KH_VOC_FLOOR_REFUSE for the SV_Depth routes and any
// occluder the endpoint discard cannot be bounded for, KH_VOC_FLOOR_NONE where no endpoint discard reaches it, else
// kh_voc_floor's certified line - the box is then clipped to its part above it, and taken only with the camera above
// it too. A declared occluder takes none of the drawn-surface tests: its box is the script's promise, whatever is
// drawn there - taken exactly (no inset), at any size on screen, and ranked ahead of every automatic occluder.
template <class KhVocFloor>
inline void kh_vis_occ_build(KhVisOcc& khvo, const std::vector<RenderObject>& khvo_m,
                             const std::vector<RenderObject>& khvo_x,
                             const float khvo_vp[4][4], const KhCullSet& khvo_cull,
                             const float khvo_cam[3], float khvo_margin,
                             float khvo_far, float khvo_cut, KhVocFloor khvo_floor) {
    KH_PROF_SCOPE(KHP_VIS_OCC);   // KH_PROF.
    khvo.on = false;
    if (!khvo_cull.valid) return;
    memcpy(khvo.vp, khvo_vp, sizeof(khvo.vp));
    khvo.bias[0] = khvo_cull.bias[0]; khvo.bias[1] = khvo_cull.bias[1]; khvo.bias[2] = khvo_cull.bias[2];
    khvo.cam[0] = khvo_cam[0]; khvo.cam[1] = khvo_cam[1]; khvo.cam[2] = khvo_cam[2];
    khvo.margin = khvo_margin;
    double khvo_sc[KH_VOC_MAX];   // Rank keys (see below).
    uint32_t khvo_id[KH_VOC_MAX];
    float khvo_fl[KH_VOC_MAX];   // KH_VOC_BURIAL: each pick's floor.
    int khvo_n = 0;
    const size_t khvo_mn = khvo_m.size();
    for (uint32_t khvo_i = 0; khvo_i < khvo_mn + khvo_x.size(); ++khvo_i) {
        const RenderObject& o = khvo_i < khvo_mn ? khvo_m[khvo_i] : khvo_x[khvo_i - khvo_mn];
        if (o.fullscreen) continue;
        const bool khvo_force = o.occluder;   // KH_OCC_FORCE.
        float khvo_f = KH_VOC_FLOOR_NONE;
        if (!khvo_force) {
            if (o.effect != 0 || o.mode == DepthMode::Off) continue;
            if (!kh_occ_drawn_as_built(o)) continue;   // KH_OCC_BOX: every mesh by one rule.
            if (o.blend_mode != 0 || o.color[3] < 0.999f) continue;
            if (o.localized || o.banded) continue;
            if (o.materials && o.materials->any) {   // Cutout / blend / user PS may discard pixels.
                bool khvo_solid = true;
                for (const KhMaterial& khvo_mt : o.materials->slots) {
                    if (khvo_mt.used && (khvo_mt.alpha_mode != 0 || khvo_mt.shader == 1)) {
                        khvo_solid = false;
                        break;
                    }
                }
                if (!khvo_solid) continue;
            }
            khvo_f = khvo_floor(o);
            if (!(khvo_f < KH_VOC_FLOOR_REFUSE)) continue;   // NaN refuses too.
            // KH_VOC_BURIAL: a ray from the camera to the clipped box meets the drawn front face on the way, between
            // two points above the floor - above it too, so never discarded by the endpoint test - only while the
            // camera is above the floor.
            if (khvo_f > KH_VOC_FLOOR_NONE && !(khvo_cam[1] > khvo_f)) continue;
        }
        // Camera inside / touching the bounds (outside a closed mesh's it has winding 0).
        {
            float khvo_bc[3], khvo_bh[3], khvo_bmn[3], khvo_bmx[3];
            kh_occ_box_of(o, khvo_force, khvo_bc, khvo_bh, khvo_bmn, khvo_bmx);
            if (!(kh_occ_bounds_dist_sq(o, khvo_bmn, khvo_bmx, khvo_cam) > 0.04f)) continue;
        }
        float khvo_he[3];
        kh_world_half_extents(o, khvo_he);
        const float khvo_r2 = khvo_he[0] * khvo_he[0] + khvo_he[1] * khvo_he[1] + khvo_he[2] * khvo_he[2];
        const float khvo_d2 = kh_mesh_dist_sq(o, khvo_cam);
        if (!(khvo_d2 > 1.0e-4f)) continue;
        const float khvo_s = khvo_r2 / khvo_d2;
        // A declared occluder has no size floor. NaN fails here too.
        if (!(khvo_s >= (khvo_force ? 0.0f : KH_VOC_MIN_SCORE))) continue;
        // KH_OCC_FORCE: the rank key puts every declared occluder ahead of every automatic one, each kind by its
        // score (s / (1 + s) keeps the order in [0, 1); a declared one adds 2).
        const double khvo_k = (khvo_force ? 2.0 : 0.0) +
                              static_cast<double>(khvo_s) / (1.0 + static_cast<double>(khvo_s));
        int khvo_at = khvo_n < KH_VOC_MAX ? khvo_n : KH_VOC_MAX - 1;
        if (khvo_n == KH_VOC_MAX && !(khvo_k > khvo_sc[khvo_at])) continue;
        while (khvo_at > 0 && khvo_sc[khvo_at - 1] < khvo_k) {
            khvo_sc[khvo_at] = khvo_sc[khvo_at - 1];
            khvo_id[khvo_at] = khvo_id[khvo_at - 1];
            khvo_fl[khvo_at] = khvo_fl[khvo_at - 1];
            --khvo_at;
        }
        khvo_sc[khvo_at] = khvo_k;
        khvo_id[khvo_at] = khvo_i;
        khvo_fl[khvo_at] = khvo_f;
        if (khvo_n < KH_VOC_MAX) ++khvo_n;
    }
    if (khvo_n == 0) return;
    for (int khvo_i = 0; khvo_i < KH_VOC_W * KH_VOC_H; ++khvo_i) khvo.z[khvo_i] = 1.0e18f;
    int khvo_budget = 0;
    uint64_t khvo_drawn = 0;   // KH_OCC_STATS: boxes that covered a cell.
    for (int khvo_k = 0; khvo_k < khvo_n && khvo_budget <= KH_VOC_PX_BUDGET; ++khvo_k) {
        const RenderObject& o = khvo_id[khvo_k] < khvo_mn ? khvo_m[khvo_id[khvo_k]]
                                                          : khvo_x[khvo_id[khvo_k] - khvo_mn];
        const bool khvo_force = o.occluder;
        float khvo_bc[3], khvo_bh[3], khvo_bmn[3], khvo_bmx[3];
        kh_occ_box_of(o, khvo_force, khvo_bc, khvo_bh, khvo_bmn, khvo_bmx);
        const float khvo_se[3] = { o.size[0], o.size[2], o.size[1] };   // Engine edge lengths.
        float khvo_ctr[3];
        kh_obj_center_engine(o, khvo_ctr);
        // KH_VOC_BURIAL: the part of the box above the floor. World height is linear in the box's normalised
        // coordinates (y = ctr.y + sum a_k p_k); the axis with the steepest rise is cut at the height its lowest
        // corner must keep with every other axis at its lowest - a box standing upright loses its buried slab
        // exactly; nothing left, no occluder.
        if (khvo_fl[khvo_k] > KH_VOC_FLOOR_NONE) {
            float khvo_a[3];
            int khvo_ku = 0;
            for (int khvo_rr = 0; khvo_rr < 3; ++khvo_rr) {
                khvo_a[khvo_rr] = khvo_se[khvo_rr] * o.rot_m[khvo_rr * 3 + 1];
                if (fabsf(khvo_a[khvo_rr]) > fabsf(khvo_a[khvo_ku])) khvo_ku = khvo_rr;
            }
            if (!(fabsf(khvo_a[khvo_ku]) > 1.0e-6f)) continue;
            float khvo_low = khvo_ctr[1];   // The lowest the other axes can put it.
            for (int khvo_rr = 0; khvo_rr < 3; ++khvo_rr) {
                if (khvo_rr == khvo_ku) continue;
                khvo_low += khvo_a[khvo_rr] * khvo_bc[khvo_rr] - fabsf(khvo_a[khvo_rr]) * khvo_bh[khvo_rr];
            }
            const float khvo_fc = (khvo_fl[khvo_k] - khvo_low) / khvo_a[khvo_ku];
            float khvo_lo = khvo_bc[khvo_ku] - khvo_bh[khvo_ku], khvo_hi = khvo_bc[khvo_ku] + khvo_bh[khvo_ku];
            if (khvo_a[khvo_ku] > 0.0f) khvo_lo = fmaxf(khvo_lo, khvo_fc);
            else                        khvo_hi = fminf(khvo_hi, khvo_fc);
            if (!(khvo_hi - khvo_lo > 1.0e-4f)) continue;
            khvo_bc[khvo_ku] = 0.5f * (khvo_lo + khvo_hi);
            khvo_bh[khvo_ku] = 0.5f * (khvo_hi - khvo_lo);
        }
        // The box's half-vectors (rows scaled by the engine edge lengths and its half extents), its centre moved onto
        // its own. A declared box is taken as it is: the inset keeps a drawn surface's box off that surface.
        const float khvo_inset = khvo_force ? 1.0f : KH_VOC_INSET;
        float khvo_hv[3][3];
        for (int khvo_rr = 0; khvo_rr < 3; ++khvo_rr) {
            for (int khvo_c = 0; khvo_c < 3; ++khvo_c) {
                const float khvo_ax = khvo_se[khvo_rr] * o.rot_m[khvo_rr * 3 + khvo_c];
                khvo_hv[khvo_rr][khvo_c] = khvo_inset * khvo_bh[khvo_rr] * khvo_ax;
                khvo_ctr[khvo_c] += khvo_bc[khvo_rr] * khvo_ax;
            }
        }
        {   // KH_POS_RES: drawn at pos plus its residual - every half-vector pulled in by the residual's length
            // (the rows are orthogonal), so the box lies inside the drawn one wherever the residual points; an
            // axis that would vanish takes the occluder out.
            float khvo_rs[3];
            kh_pos_res(o, khvo_rs);
            const float khvo_rl = sqrtf(khvo_rs[0] * khvo_rs[0] + khvo_rs[1] * khvo_rs[1] + khvo_rs[2] * khvo_rs[2]);
            bool khvo_gone = false;
            if (khvo_rl > 0.0f) {
                for (int khvo_rr = 0; khvo_rr < 3; ++khvo_rr) {
                    float* const khvo_h = khvo_hv[khvo_rr];
                    const float khvo_len = sqrtf(khvo_h[0] * khvo_h[0] + khvo_h[1] * khvo_h[1] + khvo_h[2] * khvo_h[2]);
                    if (!(khvo_len > khvo_rl)) { khvo_gone = true; break; }
                    const float khvo_f = 1.0f - khvo_rl / khvo_len;
                    khvo_h[0] *= khvo_f; khvo_h[1] *= khvo_f; khvo_h[2] *= khvo_f;
                }
            }
            if (khvo_gone) continue;
        }
        float khvo_dmn = 1.0e18f, khvo_dmx = 0.0f;
        for (int khvo_s8 = 0; khvo_s8 < 8; ++khvo_s8) {
            const float khvo_sx = (khvo_s8 & 1) ? 1.0f : -1.0f;
            const float khvo_sy = (khvo_s8 & 2) ? 1.0f : -1.0f;
            const float khvo_sz = (khvo_s8 & 4) ? 1.0f : -1.0f;
            float khvo_d2c = 0.0f;
            for (int khvo_c = 0; khvo_c < 3; ++khvo_c) {
                const float khvo_w = khvo_ctr[khvo_c] + khvo_sx * khvo_hv[0][khvo_c] +
                                     khvo_sy * khvo_hv[1][khvo_c] + khvo_sz * khvo_hv[2][khvo_c] - khvo_cam[khvo_c];
                khvo_d2c += khvo_w * khvo_w;
            }
            const float khvo_dc = sqrtf(khvo_d2c);
            if (khvo_dc < khvo_dmn) khvo_dmn = khvo_dc;
            if (khvo_dc > khvo_dmx) khvo_dmx = khvo_dc;
        }
        if (!(khvo_dmn > KH_VOC_NEAR_M)) continue;
        if (!khvo_force) {   // Its own far contract may discard it: keep it well inside.
            float khvo_lim = khvo_far;
            if (khvo_cut > 0.0f && khvo_cut < khvo_lim) khvo_lim = khvo_cut;
            if (khvo_lim > 0.0f && !(khvo_dmx < 0.98f * khvo_lim)) continue;
        }
        const int khvo_b0 = khvo_budget;
        kh_voc_box(khvo, khvo_ctr, khvo_hv, khvo_cam, khvo_budget);   // KH_VOC_NEAR_CLIP, KH_VOC_UNION.
        if (khvo_budget > khvo_b0) ++khvo_drawn;
    }
    kh_stat_add(g_stats.vis_occluders, khvo_drawn);
    khvo.on = khvo_budget > 0;
}

// True = provably hidden. Every failure direction is 'visible'. KH_LOD_BOUNDS: the object is taken at its bounds
// size, which holds everything any of its levels draws, so no drawn corner falls outside the cells tested or nearer
// than the distance they are tested at.
inline bool kh_vis_occ_hidden(const KhVisOcc& khvh_o, const RenderObject& o) {
    if (!khvh_o.on) return false;
    float khvh_ctr[3];
    kh_obj_center_engine(o, khvh_ctr);
    float khvh_he[3];
    kh_world_half_extents(o, khvh_he);
    const float khvh_r = sqrtf(khvh_he[0] * khvh_he[0] + khvh_he[1] * khvh_he[1] + khvh_he[2] * khvh_he[2]);
    float khvh_d2 = 0.0f;
    for (int khvh_c = 0; khvh_c < 3; ++khvh_c) {
        const float khvh_w = khvh_ctr[khvh_c] - khvh_o.cam[khvh_c];
        khvh_d2 += khvh_w * khvh_w;
    }
    const float khvh_mn = sqrtf(khvh_d2) - khvh_r - KH_VOC_EPS_M;   // Nearest possible point.
    if (!(khvh_mn > KH_VOC_NEAR_M)) return false;
    float khvh_x0 = 1.0e18f, khvh_x1 = -1.0e18f, khvh_y0 = 1.0e18f, khvh_y1 = -1.0e18f;
    for (int khvh_s = 0; khvh_s < 8; ++khvh_s) {
        const float khvh_wc[3] = {
            khvh_ctr[0] + ((khvh_s & 1) ? khvh_he[0] : -khvh_he[0]),
            khvh_ctr[1] + ((khvh_s & 2) ? khvh_he[1] : -khvh_he[1]),
            khvh_ctr[2] + ((khvh_s & 4) ? khvh_he[2] : -khvh_he[2]) };
        float khvh_px, khvh_py;
        if (!kh_voc_project(khvh_o, khvh_wc, khvh_px, khvh_py)) return false;
        if (khvh_px < khvh_x0) khvh_x0 = khvh_px;
        if (khvh_px > khvh_x1) khvh_x1 = khvh_px;
        if (khvh_py < khvh_y0) khvh_y0 = khvh_py;
        if (khvh_py > khvh_y1) khvh_y1 = khvh_py;
    }
    // KH_VOC_CAST_CLAMP: clamped to the grid (one cell of margin) before the casts, as kh_voc_face does - a huge
    // finite corner converted out of int range (UB). A clamped bound still fails the range test below.
    const float khvh_gw = static_cast<float>(KH_VOC_W), khvh_gh = static_cast<float>(KH_VOC_H);
    const int khvh_ix0 = static_cast<int>(floorf(fminf(fmaxf(khvh_x0 - 0.01f, -1.0f), khvh_gw)));
    const int khvh_iy0 = static_cast<int>(floorf(fminf(fmaxf(khvh_y0 - 0.01f, -1.0f), khvh_gh)));
    const int khvh_ix1 = static_cast<int>(floorf(fminf(fmaxf(khvh_x1 + 0.01f, -1.0f), khvh_gw)));
    const int khvh_iy1 = static_cast<int>(floorf(fminf(fmaxf(khvh_y1 + 0.01f, -1.0f), khvh_gh)));
    if (khvh_ix0 < 0 || khvh_iy0 < 0 || khvh_ix1 >= KH_VOC_W || khvh_iy1 >= KH_VOC_H) return false;
    if ((khvh_ix1 - khvh_ix0 + 1) * (khvh_iy1 - khvh_iy0 + 1) > KH_VOC_TEST_PX) return false;
    for (int khvh_iy = khvh_iy0; khvh_iy <= khvh_iy1; ++khvh_iy) {
        for (int khvh_ix = khvh_ix0; khvh_ix <= khvh_ix1; ++khvh_ix) {
            // An uncovered cell holds 1e18 and reads visible here too.
            if (khvh_mn <= khvh_o.z[khvh_iy * KH_VOC_W + khvh_ix] + khvh_o.margin) return false;
        }
    }
    return true;
}

inline float kh_voc_floor(const RenderObject& khvb_o) {
    if (!g_thm_valid) return KH_VOC_FLOOR_NONE;   // Discard lanes inert (kh_fill_occ): nothing to defend.
    if (!g_voc_thm_valid || !(g_voc_thm_cell > 0.0f) ||
        g_voc_thm_fw == 0 || g_voc_thm_fh == 0) {
        return KH_VOC_FLOOR_REFUSE;   // Armed discard, no certificate: refuse.
    }
    float khvb_c[3], khvb_he[3];
    kh_occ_hull_world(khvb_o, khvb_c, khvb_he);   // KH_OCC_BOX: everything the occluder draws.
    const float khvb_inv = 1.0f / g_voc_thm_cell;
    const float khvb_gx0 = (khvb_c[0] - khvb_he[0] - g_voc_thm_origin[0]) * khvb_inv;
    const float khvb_gx1 = (khvb_c[0] + khvb_he[0] - g_voc_thm_origin[0]) * khvb_inv;
    const float khvb_gz0 = (khvb_c[2] - khvb_he[2] - g_voc_thm_origin[1]) * khvb_inv;
    const float khvb_gz1 = (khvb_c[2] + khvb_he[2] - g_voc_thm_origin[1]) * khvb_inv;
    if (!(khvb_gx0 <= khvb_gx1) || !(khvb_gz0 <= khvb_gz1) ||
        !(khvb_gx1 - khvb_gx0 < 1.0e7f) || !(khvb_gz1 - khvb_gz0 < 1.0e7f)) {
        return KH_VOC_FLOOR_REFUSE;   // NaN / absurd extents: refuse before any int cast.
    }
    // KhThmHeight answers -1e6 outside the grid (no discard evidence), so a footprint fully outside cannot be eaten.
    if (khvb_gx1 < -1.0f || khvb_gz1 < -1.0f ||
        khvb_gx0 > static_cast<float>(g_voc_thm_fw) ||
        khvb_gz0 > static_cast<float>(g_voc_thm_fh)) {
        return KH_VOC_FLOOR_NONE;
    }
    // Fine-node range under the footprint plus one node each way (the support of a sample at g - its cell's corners -
    // touches floor(g) and floor(g) + 1), clamped in float so the casts stay in range.
    const float khvb_fx = static_cast<float>(g_voc_thm_fw - 1);
    const float khvb_fz = static_cast<float>(g_voc_thm_fh - 1);
    const int khvb_x0 = static_cast<int>(fmaxf(floorf(khvb_gx0) - 1.0f, 0.0f));
    const int khvb_x1 = static_cast<int>(fminf(floorf(khvb_gx1) + 1.0f, khvb_fx));
    const int khvb_z0 = static_cast<int>(fmaxf(floorf(khvb_gz0) - 1.0f, 0.0f));
    const int khvb_z1 = static_cast<int>(fminf(floorf(khvb_gz1) + 1.0f, khvb_fz));
    float khvb_hmax = -1.0e6f;
    if ((khvb_x1 - khvb_x0 + 1) * (khvb_z1 - khvb_z0 + 1) <= KH_VOC_THM_FINE_CAP) {
        for (int khvb_z = khvb_z0; khvb_z <= khvb_z1; ++khvb_z) {
            const float* khvb_row = g_voc_thm_fine.data() + static_cast<size_t>(khvb_z) * g_voc_thm_fw;
            for (int khvb_x = khvb_x0; khvb_x <= khvb_x1; ++khvb_x) {
                if (khvb_row[khvb_x] > khvb_hmax) khvb_hmax = khvb_row[khvb_x];
            }
        }
    } else {
        const int khvb_cx0 = khvb_x0 / static_cast<int>(KH_VOC_THM_DS);
        const int khvb_cx1 = khvb_x1 / static_cast<int>(KH_VOC_THM_DS);
        const int khvb_cz0 = khvb_z0 / static_cast<int>(KH_VOC_THM_DS);
        const int khvb_cz1 = khvb_z1 / static_cast<int>(KH_VOC_THM_DS);
        if ((khvb_cx1 - khvb_cx0 + 1) * (khvb_cz1 - khvb_cz0 + 1) > KH_VOC_THM_CAP) {
            return KH_VOC_FLOOR_REFUSE;   // Too wide to certify at this budget: refuse.
        }
        for (int khvb_cz = khvb_cz0; khvb_cz <= khvb_cz1; ++khvb_cz) {
            const float* khvb_row = g_voc_thm_max.data() +
                                    static_cast<size_t>(khvb_cz) * g_voc_thm_w;
            for (int khvb_cx = khvb_cx0; khvb_cx <= khvb_cx1; ++khvb_cx) {
                if (khvb_row[khvb_cx] > khvb_hmax) khvb_hmax = khvb_row[khvb_cx];
            }
        }
    }
    // The discard line anywhere under the footprint, with 5 cm of headroom over the shader's strict compare: a floor
    // only when the box's lowest point could sit below it.
    const float khvb_fl = khvb_hmax - KH_THM_BIAS_M + 0.05f;
    return khvb_c[1] - khvb_he[1] < khvb_fl ? khvb_fl : KH_VOC_FLOOR_NONE;
}

// Fills the heightfield lanes (thm_params, thm_meta) of the guard CB tail while a heightfield is live; untouched
// (zeroed = inert) otherwise. Called at three fills: the injection's frame CB (inject_composited_meshes), the
// flush's frame template (kh_fx_frame_template) and the engine cast mask's (mask_cast_engine).
inline void kh_fill_occ(ConstantData& cbd) {
    if (!g_thm_valid) return;   // Heightfield lanes stay zeroed = lane inert.
    cbd.thm_params[0] = g_thml_origin[0];
    cbd.thm_params[1] = g_thml_origin[1];
    cbd.thm_params[2] = g_thml_cell;
    cbd.thm_params[3] = 1.0f + static_cast<float>(g_thml_split);   // KH_THM_EXACT: enabled, and the split.
    cbd.thm_meta[0] = static_cast<float>(g_thml_w);
    cbd.thm_meta[1] = static_cast<float>(g_thml_h);
    cbd.thm_meta[2] = KH_THM_BIAS_M;
    cbd.thm_meta[3] = KH_THM_MIN_DIST_M;
}
}