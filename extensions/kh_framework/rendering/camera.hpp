#pragma once

// The feature half (rendering_integration_features.hpp): the camera helpers: the viewport rebase, the camera position
// and the view rows. Never include it on its own.
namespace RenderIntegration {

// The camera folded into view_proj's row 3 in double (off within 1 m of the origin: false, nothing changes).
// DDX_REBASE: the one rebase of a CPU-side view - its error is the view's translation held as a float, which no
// rebuild of the product recovers; only the engine's own camera (rebase_vp_engcam) is exact.
inline bool rebase_vp(ConstantData& cbd, const float* cam3) {
    if (cam3[0] * cam3[0] + cam3[1] * cam3[1] + cam3[2] * cam3[2] < 1.0f) return false;
    const double r[3] = { cam3[0], cam3[1], cam3[2] };
    for (int j = 0; j < 4; ++j) {
        const double r3 = static_cast<double>(cbd.view_proj[3][j])
                        + r[0] * static_cast<double>(cbd.view_proj[0][j])
                        + r[1] * static_cast<double>(cbd.view_proj[1][j])
                        + r[2] * static_cast<double>(cbd.view_proj[2][j]);
        cbd.view_proj[3][j] = static_cast<float>(r3);
    }
    return true;
}

// Per-object rebase twin: center minus the pass camera, subtracted in double. Requires center_size already filled
// with the engine-axes center. res: the object's POS_RES residual (engine axes), or null - added in double,
// so the relative centre is the exact one.
inline void fill_center_rel(ConstantData& cbd, const float* cam3, const float* res) {
    for (int k = 0; k < 3; ++k) {
        cbd.center_rel[k] = static_cast<float>(static_cast<double>(cbd.center_size[k]) -
                                               static_cast<double>(cam3[k]) +
                                               (res ? static_cast<double>(res[k]) : 0.0));
    }
    cbd.center_rel[3] = 1.0f;   // Armed (matches the pass's rebased viewProj).
}

inline bool rebase_vp_engcam(ConstantData& cbd, const float view[4][4],
                               const float proj[4][4]) {
    double v[4][4];

    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            v[r][c] = static_cast<double>(view[r][c]);

    // The handed t never enters the transform at all.
    v[3][0] = 0.0;
    v[3][1] = 0.0;
    v[3][2] = 0.0;

    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            const double s = v[r][0] * static_cast<double>(proj[0][c])
                           + v[r][1] * static_cast<double>(proj[1][c])
                           + v[r][2] * static_cast<double>(proj[2][c])
                           + v[r][3] * static_cast<double>(proj[3][c]);
            cbd.view_proj[r][c] = static_cast<float>(s);
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

inline bool view_camera_exact(const float view[4][4], float out[3],
                              float* out_err = nullptr) {
    const double a = view[0][0], b = view[0][1], c = view[0][2];
    const double d = view[1][0], e = view[1][1], f = view[1][2];
    const double g = view[2][0], h = view[2][1], i = view[2][2];

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

    const double t0 = view[3][0], t1 = view[3][1], t2 = view[3][2];

    // c = -t. inv(R) (row-vector convention, matching the rebuild).
    double cx_c[3];
    for (int k = 0; k < 3; ++k) {
        cx_c[k] = -(t0 * m[0][k] + t1 * m[1][k] + t2 * m[2][k]);
        if (!(cx_c[k] == cx_c[k])) return false;
    }

    if (out_err) {
        float t[3];
        extract_camera_pos(view, t);
        const double ex = cx_c[0] - t[0];
        const double ey = cx_c[1] - t[1];
        const double ez = cx_c[2] - t[2];
        *out_err = static_cast<float>(sqrt(ex * ex + ey * ey + ez * ez));
    }

    out[0] = static_cast<float>(cx_c[0]);
    out[1] = static_cast<float>(cx_c[1]);
    out[2] = static_cast<float>(cx_c[2]);
    return true;
}

inline void cam_of(const float v[4][4], float out[3]) {
    if (!view_camera_exact(v, out)) {
        extract_camera_pos(v, out);
    }
}

// CAM_HONEST: a view's inverse rotation laid out for KhCastWorld's row dots - castMat[j][c] = cofactor[j][c] / det,
// the freeze's own layout (g_fire.cast_inv) - in double. False (the caller keeps the transpose) for a rotation whose
// determinant is 0.1 or more off 1, the freeze's own refusal.
inline bool view_inv_rows(const float v[4][4], float out[3][3]) {
    const double r[3][3] = {
        { v[0][0], v[0][1], v[0][2] },
        { v[1][0], v[1][1], v[1][2] },
        { v[2][0], v[2][1], v[2][2] } };
    const double c[3][3] = {
        { r[1][1] * r[2][2] - r[1][2] * r[2][1],
          r[1][2] * r[2][0] - r[1][0] * r[2][2],
          r[1][0] * r[2][1] - r[1][1] * r[2][0] },
        { r[0][2] * r[2][1] - r[0][1] * r[2][2],
          r[0][0] * r[2][2] - r[0][2] * r[2][0],
          r[0][1] * r[2][0] - r[0][0] * r[2][1] },
        { r[0][1] * r[1][2] - r[0][2] * r[1][1],
          r[0][2] * r[1][0] - r[0][0] * r[1][2],
          r[0][0] * r[1][1] - r[0][1] * r[1][0] } };
    const double det = r[0][0] * c[0][0] + r[0][1] * c[0][1] + r[0][2] * c[0][2];
    if (!(fabs(det - 1.0) < 0.1)) return false;
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            out[j][k] = static_cast<float>(c[j][k] / det);
        }
    }
    return true;
}
}