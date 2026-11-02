#pragma once

// The feature half (rendering_integration_features.hpp): the camera helpers: the viewport rebase, the camera position
// and the view rows. Never include it on its own.
namespace RenderIntegration {

// The camera folded into view_proj's row 3 in double (off within 1 m of the origin: false, nothing changes).
// KH_DDX_REBASE: the one rebase of a CPU-side view - its error is the view's translation held as a float, which no
// rebuild of the product recovers; only the engine's own camera (kh_rebase_vp_engcam) is exact.
inline bool kh_rebase_vp(ConstantData& cbd, const float* cam3) {
    if (cam3[0] * cam3[0] + cam3[1] * cam3[1] + cam3[2] * cam3[2] < 1.0f) return false;
    const double khr_r[3] = { cam3[0], cam3[1], cam3[2] };
    for (int j = 0; j < 4; ++j) {
        const double khr3 = static_cast<double>(cbd.view_proj[3][j])
                          + khr_r[0] * static_cast<double>(cbd.view_proj[0][j])
                          + khr_r[1] * static_cast<double>(cbd.view_proj[1][j])
                          + khr_r[2] * static_cast<double>(cbd.view_proj[2][j]);
        cbd.view_proj[3][j] = static_cast<float>(khr3);
    }
    return true;
}

// Per-object rebase twin: center minus the pass camera, subtracted in double. Requires center_size already filled
// with the engine-axes center. khfr_res: the object's KH_POS_RES residual (engine axes), or null - added in double,
// so the relative centre is the exact one.
inline void kh_fill_center_rel(ConstantData& cbd, const float* cam3, const float* khfr_res) {
    for (int khfr_k = 0; khfr_k < 3; ++khfr_k) {
        cbd.center_rel[khfr_k] = static_cast<float>(static_cast<double>(cbd.center_size[khfr_k]) -
                                                    static_cast<double>(cam3[khfr_k]) +
                                                    (khfr_res ? static_cast<double>(khfr_res[khfr_k]) : 0.0));
    }
    cbd.center_rel[3] = 1.0f;   // Armed (matches the pass's rebased viewProj).
}

inline bool kh_rebase_vp_engcam(ConstantData& khe_cbd, const float khe_view[4][4],
                               const float khe_proj[4][4]) {
    double khe_v[4][4];

    for (int khe_r = 0; khe_r < 4; ++khe_r)
        for (int khe_c = 0; khe_c < 4; ++khe_c)
            khe_v[khe_r][khe_c] = static_cast<double>(khe_view[khe_r][khe_c]);

    // The handed t never enters the transform at all.
    khe_v[3][0] = 0.0;
    khe_v[3][1] = 0.0;
    khe_v[3][2] = 0.0;

    for (int khe_r = 0; khe_r < 4; ++khe_r) {
        for (int khe_c = 0; khe_c < 4; ++khe_c) {
            const double khe_s = khe_v[khe_r][0] * static_cast<double>(khe_proj[0][khe_c])
                               + khe_v[khe_r][1] * static_cast<double>(khe_proj[1][khe_c])
                               + khe_v[khe_r][2] * static_cast<double>(khe_proj[2][khe_c])
                               + khe_v[khe_r][3] * static_cast<double>(khe_proj[3][khe_c]);
            khe_cbd.view_proj[khe_r][khe_c] = static_cast<float>(khe_s);
        }
    }

    return true;
}

// Camera position in engine space from the view matrix (row-vector convention: eye_i = -sum_j T_j * R[i][j]).
inline void extract_camera_pos(const float view[4][4], float out[3]) {
    for (int i = 0; i < 3; ++i) {
        out[i] = -(view[3][0] * view[i][0] +
                   view[3][1] * view[i][1] +
                   view[3][2] * view[i][2]);
    }
}

inline bool kh_view_camera_exact(const float khcx_view[4][4], float khcx_out[3],
                                 float* khcx_out_err = nullptr) {
    const double a = khcx_view[0][0], b = khcx_view[0][1], c = khcx_view[0][2];
    const double d = khcx_view[1][0], e = khcx_view[1][1], f = khcx_view[1][2];
    const double g = khcx_view[2][0], h = khcx_view[2][1], i = khcx_view[2][2];

    // Cofactors of R, so inv(R) = adj(R) / det.
    const double A =  (e * i - f * h);
    const double B = -(d * i - f * g);
    const double C =  (d * h - e * g);
    const double det = a * A + b * B + c * C;

    if (!(det == det) || fabs(det) < 1.0e-12) return false;

    const double D = -(b * i - c * h);
    const double E =  (a * i - c * g);
    const double F = -(a * h - b * g);
    const double G =  (b * f - c * e);
    const double H = -(a * f - c * d);
    const double I2 = (a * e - b * d);
    const double inv = 1.0 / det;

    // inv(R)[row][col] = adj / det, adj = transpose of the cofactor matrix.
    const double m[3][3] = {
        { A * inv, D * inv, G * inv },
        { B * inv, E * inv, H * inv },
        { C * inv, F * inv, I2 * inv }
    };

    const double t0 = khcx_view[3][0], t1 = khcx_view[3][1], t2 = khcx_view[3][2];

    // c = -t. inv(R) (row-vector convention, matching the rebuild).
    double khcx_c[3];
    for (int k = 0; k < 3; ++k) {
        khcx_c[k] = -(t0 * m[0][k] + t1 * m[1][k] + t2 * m[2][k]);
        if (!(khcx_c[k] == khcx_c[k])) return false;
    }

    if (khcx_out_err) {
        float khcx_t[3];
        extract_camera_pos(khcx_view, khcx_t);
        const double ex = khcx_c[0] - khcx_t[0];
        const double ey = khcx_c[1] - khcx_t[1];
        const double ez = khcx_c[2] - khcx_t[2];
        *khcx_out_err = static_cast<float>(sqrt(ex * ex + ey * ey + ez * ez));
    }

    khcx_out[0] = static_cast<float>(khcx_c[0]);
    khcx_out[1] = static_cast<float>(khcx_c[1]);
    khcx_out[2] = static_cast<float>(khcx_c[2]);
    return true;
}

inline void kh_cam_of(const float khco_v[4][4], float khco_out[3]) {
    if (!kh_view_camera_exact(khco_v, khco_out)) {
        extract_camera_pos(khco_v, khco_out);
    }
}

inline void kh_cam_of_site(const float khco_v[4][4], float khco_out[3],
                           int khco_site) {
    kh_cam_of(khco_v, khco_out);
}

// KH_CAM_HONEST: a view's inverse rotation laid out for KhCastWorld's row dots - castMat[j][c] = cofactor[j][c] / det,
// the freeze's own layout (g_fire_cast_inv) - in double. False (the caller keeps the transpose) for a rotation whose
// determinant is 0.1 or more off 1, the freeze's own refusal.
inline bool kh_view_inv_rows(const float khvi_v[4][4], float khvi_out[3][3]) {
    const double khvi_r[3][3] = {
        { khvi_v[0][0], khvi_v[0][1], khvi_v[0][2] },
        { khvi_v[1][0], khvi_v[1][1], khvi_v[1][2] },
        { khvi_v[2][0], khvi_v[2][1], khvi_v[2][2] } };
    const double khvi_c[3][3] = {
        { khvi_r[1][1] * khvi_r[2][2] - khvi_r[1][2] * khvi_r[2][1],
          khvi_r[1][2] * khvi_r[2][0] - khvi_r[1][0] * khvi_r[2][2],
          khvi_r[1][0] * khvi_r[2][1] - khvi_r[1][1] * khvi_r[2][0] },
        { khvi_r[0][2] * khvi_r[2][1] - khvi_r[0][1] * khvi_r[2][2],
          khvi_r[0][0] * khvi_r[2][2] - khvi_r[0][2] * khvi_r[2][0],
          khvi_r[0][1] * khvi_r[2][0] - khvi_r[0][0] * khvi_r[2][1] },
        { khvi_r[0][1] * khvi_r[1][2] - khvi_r[0][2] * khvi_r[1][1],
          khvi_r[0][2] * khvi_r[1][0] - khvi_r[0][0] * khvi_r[1][2],
          khvi_r[0][0] * khvi_r[1][1] - khvi_r[0][1] * khvi_r[1][0] } };
    const double khvi_det = khvi_r[0][0] * khvi_c[0][0] + khvi_r[0][1] * khvi_c[0][1] + khvi_r[0][2] * khvi_c[0][2];
    if (!(fabs(khvi_det - 1.0) < 0.1)) return false;
    for (int khvi_j = 0; khvi_j < 3; ++khvi_j) {
        for (int khvi_k = 0; khvi_k < 3; ++khvi_k) {
            khvi_out[khvi_j][khvi_k] = static_cast<float>(khvi_c[khvi_j][khvi_k] / khvi_det);
        }
    }
    return true;
}
}