#pragma once

// The feature half (rendering_integration_features.hpp): frustum culling, the scene grid queries and the local cull.
// Never include it on its own.
namespace RenderIntegration {

// Frustum culling. Nothing here is published or shared: every kh_cull_build call fills a set the CALLER owns as a
// local, at five sites on both threads - kh_volume_seam_inject, kh_pip_inject, inject_composited_meshes twice (the
// grid pre-cull and the colour pass) and flush_locked. A set never outlives the pass that built it, so it needs no
// lock and carries no cross-thread contract.

// The apex is recoverable from the planes (three intersect there); its offset from the camera is the rebase origin.
static constexpr float KH_CULL_APEX_TOL   = 1.0f;   // 4th-plane residual at the apex, m.
static constexpr float KH_CULL_ORIGIN_TOL = 4.0f;   // Recovered origin must be 0 or the camera.
static constexpr float KH_CULL_PAD        = 2.0f;   // Radius pad: fp32 at 7.5 km, and cheap.

// Apex: the point on all of planes 0, 1, 2 (Cramer). A singular system is a degenerate frustum; the caller stands down.
inline bool kh_cull_apex(const KhCullSet& khc_s, float khc_a[3]) {
    const float khc_m[3][3] = {
        { khc_s.p[0][0], khc_s.p[0][1], khc_s.p[0][2] },
        { khc_s.p[1][0], khc_s.p[1][1], khc_s.p[1][2] },
        { khc_s.p[2][0], khc_s.p[2][1], khc_s.p[2][2] },
    };
    const float khc_r[3] = { -khc_s.p[0][3], -khc_s.p[1][3], -khc_s.p[2][3] };
    const float khc_det =
        khc_m[0][0] * (khc_m[1][1] * khc_m[2][2] - khc_m[1][2] * khc_m[2][1]) -
        khc_m[0][1] * (khc_m[1][0] * khc_m[2][2] - khc_m[1][2] * khc_m[2][0]) +
        khc_m[0][2] * (khc_m[1][0] * khc_m[2][1] - khc_m[1][1] * khc_m[2][0]);
    if (!(khc_det > 1.0e-9f || khc_det < -1.0e-9f)) return false;
    const float khc_inv = 1.0f / khc_det;
    khc_a[0] = khc_inv * (khc_r[0] * (khc_m[1][1] * khc_m[2][2] - khc_m[1][2] * khc_m[2][1]) -
                          khc_m[0][1] * (khc_r[1] * khc_m[2][2] - khc_m[1][2] * khc_r[2]) +
                          khc_m[0][2] * (khc_r[1] * khc_m[2][1] - khc_m[1][1] * khc_r[2]));
    khc_a[1] = khc_inv * (khc_m[0][0] * (khc_r[1] * khc_m[2][2] - khc_m[1][2] * khc_r[2]) -
                          khc_r[0] * (khc_m[1][0] * khc_m[2][2] - khc_m[1][2] * khc_m[2][0]) +
                          khc_m[0][2] * (khc_m[1][0] * khc_r[2] - khc_r[1] * khc_m[2][0]));
    khc_a[2] = khc_inv * (khc_m[0][0] * (khc_m[1][1] * khc_r[2] - khc_r[1] * khc_m[2][1]) -
                          khc_m[0][1] * (khc_m[1][0] * khc_r[2] - khc_r[1] * khc_m[2][0]) +
                          khc_r[0] * (khc_m[1][0] * khc_m[2][1] - khc_m[1][1] * khc_m[2][0]));
    return true;
}

inline void kh_cull_build(const float khc_vp[4][4], const float khc_cam[3], KhCullSet& khc_o) {
    KH_PROF_SCOPE(KHP_CULL_BUILD);   // KH_PROF.
    khc_o.valid = false;

    for (int khc_i = 0; khc_i < 4; ++khc_i) {
        const int khc_r = khc_i >> 1;   // 0 = x column, 1 = y column.
        const float khc_s = (khc_i & 1) ? -1.0f : 1.0f;

        for (int khc_c = 0; khc_c < 4; ++khc_c)
            khc_o.p[khc_i][khc_c] = khc_vp[khc_c][3] + khc_s * khc_vp[khc_c][khc_r];

        const float khc_l = sqrtf(khc_o.p[khc_i][0] * khc_o.p[khc_i][0] +
                                  khc_o.p[khc_i][1] * khc_o.p[khc_i][1] +
                                  khc_o.p[khc_i][2] * khc_o.p[khc_i][2]);
        if (!(khc_l > 1.0e-9f)) {  return; }

        for (int khc_c = 0; khc_c < 4; ++khc_c) khc_o.p[khc_i][khc_c] /= khc_l;
    }

    // The apex, then the fourth plane must pass through it - a matrix that gives none is not a frustum to cull against.
    float khc_ap[3];
    if (!kh_cull_apex(khc_o, khc_ap)) {  return; }
    const float khc_res = fabsf(khc_o.p[3][0] * khc_ap[0] + khc_o.p[3][1] * khc_ap[1] +
                                khc_o.p[3][2] * khc_ap[2] + khc_o.p[3][3]);
    if (!(khc_res <= KH_CULL_APEX_TOL)) {  return; }
    {   // KH_CULL_BACK: forward from the inward side normals; apex is the eye.
        float khc_f[3] = { 0.0f, 0.0f, 0.0f };
        for (int khc_i = 0; khc_i < 4; ++khc_i)
            for (int khc_c = 0; khc_c < 3; ++khc_c) khc_f[khc_c] += khc_o.p[khc_i][khc_c];
        const float khc_fl = sqrtf(khc_f[0] * khc_f[0] + khc_f[1] * khc_f[1] + khc_f[2] * khc_f[2]);
        if (khc_fl > 1.0e-6f) {
            for (int khc_c = 0; khc_c < 3; ++khc_c) khc_o.fwd[khc_c] = khc_f[khc_c] / khc_fl;
            khc_o.apex[0] = khc_ap[0]; khc_o.apex[1] = khc_ap[1]; khc_o.apex[2] = khc_ap[2];
            khc_o.back_ok = true;
        } else {
            khc_o.back_ok = false;   // Degenerate: the back half stays unculled.
        }
    }
    // The recovered origin: only none and the camera are accepted, so a cycle latch or a foreign view cannot cull.
    const float khc_ac[3] = { khc_ap[0] - khc_cam[0], khc_ap[1] - khc_cam[1],
                              khc_ap[2] - khc_cam[2] };
    const float khc_dabs = sqrtf(khc_ac[0] * khc_ac[0] + khc_ac[1] * khc_ac[1] +
                                 khc_ac[2] * khc_ac[2]);
    const float khc_dreb = sqrtf(khc_ap[0] * khc_ap[0] + khc_ap[1] * khc_ap[1] +
                                 khc_ap[2] * khc_ap[2]);

    if (khc_dabs <= KH_CULL_ORIGIN_TOL) {
        khc_o.bias[0] = khc_o.bias[1] = khc_o.bias[2] = 0.0f;
    } else if (khc_dreb <= KH_CULL_ORIGIN_TOL) {
        khc_o.bias[0] = khc_ac[0];
        khc_o.bias[1] = khc_ac[1];
        khc_o.bias[2] = khc_ac[2];
    } else {  return; }

    khc_o.valid = true;
}

// True = draw it. Fails open in every direction: an unbuilt set, a zero radius, a stood-down pass and anything
// non-finite all return true.
inline bool kh_mesh_visible(const KhCullSet& khc_s, const float khc_ctr[3], float khc_r) {
    if (!khc_s.valid) return true;
    if (!(khc_r > 0.0f)) return true;

    // Into the matrix's own space, then a padded sphere test.
    const float khc_q[3] = { khc_ctr[0] + khc_s.bias[0], khc_ctr[1] + khc_s.bias[1],
                             khc_ctr[2] + khc_s.bias[2] };
    const float khc_b = khc_r + KH_CULL_PAD;

    for (int khc_i = 0; khc_i < 4; ++khc_i) {
        const float khc_d = khc_s.p[khc_i][0] * khc_q[0] + khc_s.p[khc_i][1] * khc_q[1] +
                            khc_s.p[khc_i][2] * khc_q[2] + khc_s.p[khc_i][3];
        if (khc_d < -khc_b) return false;
    }

    if (khc_s.back_on && khc_s.back_ok) {   // KH_CULL_BACK: the mirror half of the double cone.
        const float khc_dn = khc_s.fwd[0] * (khc_q[0] - khc_s.apex[0]) +
                             khc_s.fwd[1] * (khc_q[1] - khc_s.apex[1]) +
                             khc_s.fwd[2] * (khc_q[2] - khc_s.apex[2]);
        if (khc_dn < -khc_b) {  return false; }
    }

    return true;
}

// Grid query: every live mesh slot in a cell whose loose bounds (footprint, y span, padded by its largest object
// radius + KH_CULL_PAD) can meet the frustum. Conservative; kh_mesh_visible still runs per object. An invalid cull
// set returns everything.
inline void kh_scene_grid_query(const KhCullSet& khq_s, std::vector<uint32_t>& khq_out) {
    khq_out.clear();
    for (const auto& khq_kv : g_scene.grid) {
        const KhSceneCell& khq_c = khq_kv.second;
        if (khq_s.valid) {
            const int32_t khq_cx = static_cast<int32_t>(khq_kv.first >> 32);
            const int32_t khq_cz = static_cast<int32_t>(khq_kv.first & 0xFFFFFFFFu);
            const float khq_pad = khq_c.rmax + KH_CULL_PAD;
            const float khq_mn[3] = { khq_cx * KH_SCENE_CELL_M - khq_pad + khq_s.bias[0],
                                      khq_c.ymin - khq_pad + khq_s.bias[1],
                                      khq_cz * KH_SCENE_CELL_M - khq_pad + khq_s.bias[2] };
            const float khq_mx[3] = { (khq_cx + 1) * KH_SCENE_CELL_M + khq_pad + khq_s.bias[0],
                                      khq_c.ymax + khq_pad + khq_s.bias[1],
                                      (khq_cz + 1) * KH_SCENE_CELL_M + khq_pad + khq_s.bias[2] };
            bool khq_in = true;
            for (int khq_i = 0; khq_i < 4 && khq_in; ++khq_i) {   // P-vertex test per plane.
                const float* khq_p = khq_s.p[khq_i];
                const float khq_d = khq_p[0] * (khq_p[0] >= 0.0f ? khq_mx[0] : khq_mn[0]) +
                                    khq_p[1] * (khq_p[1] >= 0.0f ? khq_mx[1] : khq_mn[1]) +
                                    khq_p[2] * (khq_p[2] >= 0.0f ? khq_mx[2] : khq_mn[2]) + khq_p[3];
                if (khq_d < 0.0f) khq_in = false;
            }
            if (khq_in && khq_s.back_on && khq_s.back_ok) {
                const float khq_d = khq_s.fwd[0] * ((khq_s.fwd[0] >= 0.0f ? khq_mx[0] : khq_mn[0]) - khq_s.apex[0]) +
                                    khq_s.fwd[1] * ((khq_s.fwd[1] >= 0.0f ? khq_mx[1] : khq_mn[1]) - khq_s.apex[1]) +
                                    khq_s.fwd[2] * ((khq_s.fwd[2] >= 0.0f ? khq_mx[2] : khq_mn[2]) - khq_s.apex[2]);
                if (khq_d < 0.0f) khq_in = false;
            }
            if (!khq_in) continue;
        }
        khq_out.insert(khq_out.end(), khq_c.slots.begin(), khq_c.slots.end());
    }
}

// Every live mesh slot in a cell whose loose bounds can meet the engine-axes box (same padding rule). Conservative.
inline void kh_scene_grid_query_box(const float khb_mn[3], const float khb_mx[3], std::vector<uint32_t>& khb_out) {
    khb_out.clear();
    for (const auto& khb_kv : g_scene.grid) {
        const KhSceneCell& khb_c = khb_kv.second;
        const int32_t khb_cx = static_cast<int32_t>(khb_kv.first >> 32);
        const int32_t khb_cz = static_cast<int32_t>(khb_kv.first & 0xFFFFFFFFu);
        const float khb_pad = khb_c.rmax + KH_CULL_PAD;
        if (khb_cx * KH_SCENE_CELL_M - khb_pad > khb_mx[0] || (khb_cx + 1) * KH_SCENE_CELL_M + khb_pad < khb_mn[0]) continue;
        if (khb_c.ymin - khb_pad > khb_mx[1] || khb_c.ymax + khb_pad < khb_mn[1]) continue;
        if (khb_cz * KH_SCENE_CELL_M - khb_pad > khb_mx[2] || (khb_cz + 1) * KH_SCENE_CELL_M + khb_pad < khb_mn[2]) continue;
        khb_out.insert(khb_out.end(), khb_c.slots.begin(), khb_c.slots.end());
    }
}
// khlc_vp: the row-vector clip matrix (clip = p x vp) of the camera-relative frame; khlc_cam: fxCam (w armed).
inline bool kh_local_cull_make(KhLocalCull& khlc_c, const float khlc_vp[4][4], const float khlc_cam[4],
                               bool khlc_pair_check, float khlc_m22, float khlc_m32) {
    khlc_c.on = false;
    if (!(khlc_cam[3] > 0.5f)) return false;   // Absolute frames are not culled.
    static const int khlc_ax[4] = { 0, 0, 1, 1 };
    static const double khlc_sg[4] = { 1.0, -1.0, 1.0, -1.0 };   // w + x, w - x, w + y, w - y.
    for (int khlc_p = 0; khlc_p < 4; ++khlc_p) {
        double khlc_e[4];
        for (int khlc_k = 0; khlc_k < 4; ++khlc_k) {
            khlc_e[khlc_k] = static_cast<double>(khlc_vp[khlc_k][3]) +
                             khlc_sg[khlc_p] * static_cast<double>(khlc_vp[khlc_k][khlc_ax[khlc_p]]);
        }
        const double khlc_n = sqrt(khlc_e[0] * khlc_e[0] + khlc_e[1] * khlc_e[1] + khlc_e[2] * khlc_e[2]);
        if (!(khlc_n > 1.0e-12 && khlc_n < 1.0e30)) return false;
        for (int khlc_k = 0; khlc_k < 4; ++khlc_k) khlc_c.plane[khlc_p][khlc_k] = khlc_e[khlc_k] / khlc_n;
    }
    for (int khlc_k = 0; khlc_k < 3; ++khlc_k) khlc_c.cam[khlc_k] = static_cast<double>(khlc_cam[khlc_k]);
    khlc_c.pair_check = khlc_pair_check;
    khlc_c.pair[0] = khlc_m22;
    khlc_c.pair[1] = khlc_m32;
    khlc_c.on = true;
    return true;
}
inline bool kh_local_culled(const KhLocalCull& khlu_c, const RenderObject& khlu_o) {
    if (!khlu_c.on || !khlu_o.localized || khlu_o.local_inverse || khlu_o.effect <= 0 ||
        khlu_o.effect == KH_EFFECT_CUSTOM) return false;
    if (khlu_c.pair_check && (!g_ro.engine_proj_valid || g_ro.engine_m22 != khlu_c.pair[0] ||
                              g_ro.engine_m32 != khlu_c.pair[1])) return false;
    // kh_fill_local_band_cb's lanes, as the shader reads them (TWIN): centre and radii in engine axes, the
    // falloff lane over the mean radius.
    const float khlu_r[3] = { khlu_o.local_radius[0], khlu_o.local_radius[2], khlu_o.local_radius[1] };
    const float khlu_mean = (khlu_o.local_radius[0] + khlu_o.local_radius[1] + khlu_o.local_radius[2]) / 3.0f;
    const float khlu_fl = khlu_o.local_falloff / (khlu_mean > 0.01f ? khlu_mean : 0.01f);
    const double khlu_f = 1.0 + static_cast<double>(khlu_fl > 0.001f ? khlu_fl : 0.001f);
    const double khlu_ctr[3] = { static_cast<double>(khlu_o.pos[0]) - khlu_c.cam[0],
                                 static_cast<double>(khlu_o.pos[2]) - khlu_c.cam[1],
                                 static_cast<double>(khlu_o.pos[1]) - khlu_c.cam[2] };
    double khlu_h[3];
    for (int khlu_k = 0; khlu_k < 3; ++khlu_k) {
        khlu_h[khlu_k] = static_cast<double>(khlu_r[khlu_k] > 0.01f ? khlu_r[khlu_k] : 0.01f) * khlu_f;
        if (!(khlu_h[khlu_k] < 1.0e30) || !(fabs(khlu_ctr[khlu_k]) < 1.0e30)) return false;   // Non-finite.
    }
    for (int khlu_p = 0; khlu_p < 4; ++khlu_p) {
        const double* khlu_pl = khlu_c.plane[khlu_p];
        const double khlu_s = khlu_pl[0] * khlu_ctr[0] + khlu_pl[1] * khlu_ctr[1] +
                              khlu_pl[2] * khlu_ctr[2] + khlu_pl[3];
        const double khlu_e = fabs(khlu_pl[0]) * khlu_h[0] + fabs(khlu_pl[1]) * khlu_h[1] +
                              fabs(khlu_pl[2]) * khlu_h[2];
        if (khlu_s + khlu_e < 0.0) return true;
    }
    return false;
}
}