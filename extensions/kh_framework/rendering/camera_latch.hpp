#pragma once

// The injection half (rendering_integration_injection.hpp): the engine camera latch: the encode pair, the boundary
// camera and the adopted frame view. Never include it on its own.
namespace RenderIntegration {
static constexpr float CAM_NEAR_MIN = 0.05f;
static constexpr float CAM_NEAR_MAX = 15.0f;

static constexpr float ENC_FAR_MAX = 50000.0f;

// Near from a forward-encode pair (ndc = m22 + m32/d): near = -m32/m22.
inline float enc_pair_near(float m22, float m32) {
    return fabsf(m22) > 1e-9f ? (-m32 / m22) : -1.0f;
}

// Far plane from a forward-encode pair (ndc = m22 + m32/d): far is where ndc reaches 1. Degenerate and non-physical
// pairs return 1e9 - always above the ceiling, i.e. refused.
inline float enc_far(float m22, float m32) {
    const float d = 1.0f - m22;
    if (fabsf(d) < 1e-7f) return 1e9f;
    const float f = m32 / d;
    return f > 0.0f ? f : 1e9f;
}
static float    g_latch_cam[3] = {};   // Last boundary camera.
static bool     g_latch_cam_valid = false;
static float    g_cam_step_m = -1.0f;   // Boundary-camera step this frame (m).
static float    g_boundary_t = -1.0f;


inline bool adopt_frame_view(RVExtBridge::ProjectionViewTransform& pv) {
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
    float rot = 0.0f;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            rot += fabsf(fv[r * 4 + c] - pv.view[r][c]);
        }
    }

    // A refused flick may itself be a wrong-lock publish; the bar is unproven.
    if (!(rot <= 0.75f)) {   // VIEW_NAN: positive form, a NaN frame view is never adopted.
        return false;
    }

    if (rot < 1.0e-5f) {
        return true;
    }

    // Latch camera first, extracted before any mutation, through the honest inverse (not the transpose: |t| is the
    // absolute map position, so the transpose is decimetres off).
    float camf[3];
    cam_of(pv.view, camf);
    const double cam[3] = { camf[0], camf[1], camf[2] };

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) pv.view[r][c] = fv[r * 4 + c];
        pv.view[r][3] = 0.0f;   // Enforce the affine column (harvested lane hygiene).
    }

    for (int c = 0; c < 3; ++c) {
        pv.view[3][c] = static_cast<float>(
            -(cam[0] * fv[0 * 4 + c] +
              cam[1] * fv[1 * 4 + c] +
              cam[2] * fv[2 * 4 + c]));
    }

    pv.view[3][3] = 1.0f;
    return true;
}
}