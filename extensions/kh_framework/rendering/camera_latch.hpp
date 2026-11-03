#pragma once

// The injection half (rendering_integration_injection.hpp): the engine camera latch: the encode pair, the boundary
// camera and the adopted frame view. Never include it on its own.
namespace RenderIntegration {
static constexpr float KH_CAM_NEAR_MIN = 0.05f;
static constexpr float KH_CAM_NEAR_MAX = 15.0f;

static constexpr float KH_ENC_FAR_MAX = 50000.0f;

// Near from a forward-encode pair (ndc = m22 + m32/d): near = -m32/m22.
inline float kh_enc_pair_near(float khn_m22, float khn_m32) {
    return fabsf(khn_m22) > 1e-9f ? (-khn_m32 / khn_m22) : -1.0f;
}

// Far plane from a forward-encode pair (ndc = m22 + m32/d): far is where ndc reaches 1. Degenerate and non-physical
// pairs return 1e9 - always above the ceiling, i.e. refused.
inline float kh_enc_far(float m22, float m32) {
    const float khd = 1.0f - m22;
    if (fabsf(khd) < 1e-7f) return 1e9f;
    const float khf = m32 / khd;
    return khf > 0.0f ? khf : 1e9f;
}
static float    g_latch_cam[3] = {};   // Last boundary camera.
static bool     g_latch_cam_valid = false;
static float    g_cam_step_m = -1.0f;   // Boundary-camera step this frame (m).
static float    g_boundary_t = -1.0f;

// The last proceeding injection attempt's view (after adoption) and its stamp: the flush and the DLS world pass take it
// on a cycle that landed, within 100 ms.
// Rotation only (kh_adopt_frame_view, below): the mesh passes adopt fv's rotation whenever it is frame-fresh; the latch
// remains the authority when it is not. The supervisor's truth/health reference stays the pre-adoption latch (a wrong
// lock must not self-certify through its own adopted publishes).
static float    g_inj_view[16] = {};
static uint64_t g_inj_view_ms = 0;

inline bool kh_adopt_frame_view(RVExtBridge::ProjectionViewTransform& pv) {
    if (!g_ls.frame_view_valid || g_ls.frame_view_time < 0.0f) return false;

    if (g_ls.pub_fresh_ms == 0 ||
        steady_now_ms() - g_ls.pub_fresh_ms > 1000) {
        return false;
    }

    if (g_boundary_t >= 0.0f && g_ls.frame_view_time < g_boundary_t) {
        return false;
    }

    if (effect_time_seconds() - g_ls.frame_view_time > 0.15f) {
        return false;
    }

    const float* fv = g_ls.frame_view;
    float khav_rot = 0.0f;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            khav_rot += fabsf(fv[r * 4 + c] - pv.view[r][c]);
        }
    }

    // A refused flick may itself be a wrong-lock publish; the bar is unproven.
    if (!(khav_rot <= 0.75f)) {   // KH_VIEW_NAN: positive form, a NaN frame view is never adopted.
        return false;
    }

    if (khav_rot < 1.0e-5f) {
        return true;
    }

    // Latch camera first, extracted before any mutation, through the honest inverse (not the transpose: |t| is the
    // absolute map position, so the transpose is decimetres off).
    float khav_camf[3];
    kh_cam_of(pv.view, khav_camf);
    const double khav_cam[3] = { khav_camf[0], khav_camf[1], khav_camf[2] };

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) pv.view[r][c] = fv[r * 4 + c];
        pv.view[r][3] = 0.0f;   // Enforce the affine column (harvested lane hygiene).
    }

    for (int c = 0; c < 3; ++c) {
        pv.view[3][c] = static_cast<float>(
            -(khav_cam[0] * fv[0 * 4 + c] +
              khav_cam[1] * fv[1 * 4 + c] +
              khav_cam[2] * fv[2 * 4 + c]));
    }

    pv.view[3][3] = 1.0f;
    return true;
}
}