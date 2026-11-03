#pragma once

// The injection half (rendering_integration_injection.hpp): the snapshot rectangle and take. Never include it on its
// own.
namespace RenderIntegration {

// The occlusion snapshot and its stamps (pair, camera, folded view*proj, viewport depth range), taken by the flush with
// the main depth bound and the frame's pv. Stamps are read on the render thread.
// KH_SNAP_RECT: the pixel rectangle the visible meshes project to under pv (their boxes' corners, plus a margin) -
// KH_SSAO's pass rectangle (kh_ssao_rect). False = a corner is behind the camera or the target size is unknown: the
// caller takes the whole target.
static constexpr LONG KH_SNAP_RECT_MARGIN = 16;
inline bool kh_snapshot_rect(const RVExtBridge::ProjectionViewTransform& pv, UINT khsr_w, UINT khsr_h, D3D11_RECT& khsr_out) {
    if (khsr_w == 0 || khsr_h == 0) return false;
    float khsr_vp[4][4];
    mul_4x4(pv.view, pv.projection, khsr_vp);
    float khsr_x0 = 1.0e9f, khsr_y0 = 1.0e9f, khsr_x1 = -1.0e9f, khsr_y1 = -1.0e9f;
    for (uint32_t khsr_i = 0; khsr_i < g_scene.objs.size(); ++khsr_i) {
        if (!g_scene.alive[khsr_i]) continue;
        const RenderObject& khsr_o = g_scene.objs[khsr_i];
        // KH_INFRONT: no snapshot reader. KH_SHOW_INFRONT: drawn (a head hidden around the camera would put a corner
        // behind it and the AO passes over the whole target).
        if (!kh_obj_shown(khsr_o) || khsr_o.fullscreen || kh_in_front(khsr_o)) continue;
        float khsr_c[3];
        kh_obj_center_engine(khsr_o, khsr_c);
        float khsr_he[3];
        kh_world_half_extents(khsr_o, khsr_he);   // KH_LOD_BOUNDS: everything it may draw, engine axes.
        for (int khsr_k = 0; khsr_k < 8; ++khsr_k) {
            const float khsr_p[3] = { khsr_c[0] + ((khsr_k & 1) ? khsr_he[0] : -khsr_he[0]),
                                      khsr_c[1] + ((khsr_k & 2) ? khsr_he[1] : -khsr_he[1]),
                                      khsr_c[2] + ((khsr_k & 4) ? khsr_he[2] : -khsr_he[2]) };
            float khsr_cl[4];
            for (int khsr_j = 0; khsr_j < 4; ++khsr_j) {
                khsr_cl[khsr_j] = khsr_p[0] * khsr_vp[0][khsr_j] + khsr_p[1] * khsr_vp[1][khsr_j] +
                                  khsr_p[2] * khsr_vp[2][khsr_j] + khsr_vp[3][khsr_j];
            }
            if (khsr_cl[3] <= 1.0e-3f) return false;   // At or behind the camera: the hull rule fails.
            const float khsr_px = (khsr_cl[0] / khsr_cl[3] * 0.5f + 0.5f) * static_cast<float>(khsr_w);
            const float khsr_py = (0.5f - khsr_cl[1] / khsr_cl[3] * 0.5f) * static_cast<float>(khsr_h);
            if (!(khsr_px == khsr_px) || !(khsr_py == khsr_py)) return false;
            khsr_x0 = khsr_px < khsr_x0 ? khsr_px : khsr_x0;
            khsr_y0 = khsr_py < khsr_y0 ? khsr_py : khsr_y0;
            khsr_x1 = khsr_px > khsr_x1 ? khsr_px : khsr_x1;
            khsr_y1 = khsr_py > khsr_y1 ? khsr_py : khsr_y1;
        }
    }
    if (khsr_x1 < khsr_x0) {   // No visible mesh: nothing reads the snapshot; an empty rect stamps only.
        khsr_out = D3D11_RECT{ 0, 0, 0, 0 };
        return true;
    }
    const float khsr_lim = 1.0e7f;   // Far off-screen projections clamp before the LONG cast.
    auto khsr_cv = [khsr_lim](float khsr_v) { return static_cast<LONG>(khsr_v < -khsr_lim ? -khsr_lim : (khsr_v > khsr_lim ? khsr_lim : khsr_v)); };
    khsr_out.left = khsr_cv(khsr_x0) - KH_SNAP_RECT_MARGIN;
    khsr_out.top = khsr_cv(khsr_y0) - KH_SNAP_RECT_MARGIN;
    khsr_out.right = khsr_cv(khsr_x1) + KH_SNAP_RECT_MARGIN + 1;
    khsr_out.bottom = khsr_cv(khsr_y1) + KH_SNAP_RECT_MARGIN + 1;
    return true;
}

inline void kh_snapshot_take(ID3D11Device* dev, ID3D11DeviceContext* ctx, const RVExtBridge::ProjectionViewTransform& pv) {
    KH_PROF_SCOPE(KHP_SNAPSHOT);
    KH_GPU_SCOPE(ctx, KHG_SNAPSHOT);   // KH_PROF.
    std::string khs_err = snapshot_composite_depth(dev, ctx);

    if (!khs_err.empty()) {
        report_error_once_safe("KH RenderIntegration: occlusion snapshot: " + khs_err);
    } else {
        g_snap_serial++;
        g_snap_ms = steady_now_ms();
        if (g_ro.engine_proj_valid) {
            g_snap_pair[0] = g_ro.engine_m22; g_snap_pair[1] = g_ro.engine_m32; g_snap_pair_src = 1;
        } else if (g_inj_dp_valid) {
            g_snap_pair[0] = g_inj_dp[0]; g_snap_pair[1] = g_inj_dp[1]; g_snap_pair_src = 2;
        } else {
            g_snap_pair_src = 0;
        }
        kh_cam_of(pv.view, g_snap_cam);
        // view*proj of this frame folded in double and rebased to g_snap_cam (row3 += cam . rows), so the shader
        // multiplies a camera-relative offset - exact algebra whichever point c is; c near the true camera only keeps
        // magnitudes small.
        {
            double khsv_v[4][4];
            for (int khsv_r = 0; khsv_r < 4; ++khsv_r)
                for (int khsv_c = 0; khsv_c < 4; ++khsv_c)
                    khsv_v[khsv_r][khsv_c] = static_cast<double>(pv.view[khsv_r][khsv_c]);
            for (int khsv_c = 0; khsv_c < 4; ++khsv_c) {
                khsv_v[3][khsv_c] += static_cast<double>(g_snap_cam[0]) * khsv_v[0][khsv_c]
                                   + static_cast<double>(g_snap_cam[1]) * khsv_v[1][khsv_c]
                                   + static_cast<double>(g_snap_cam[2]) * khsv_v[2][khsv_c];
            }
            for (int khsv_r = 0; khsv_r < 4; ++khsv_r) {
                for (int khsv_c = 0; khsv_c < 4; ++khsv_c) {
                    const double khsv_s = khsv_v[khsv_r][0] * static_cast<double>(pv.projection[0][khsv_c])
                                        + khsv_v[khsv_r][1] * static_cast<double>(pv.projection[1][khsv_c])
                                        + khsv_v[khsv_r][2] * static_cast<double>(pv.projection[2][khsv_c])
                                        + khsv_v[khsv_r][3] * static_cast<double>(pv.projection[3][khsv_c]);
                    g_snap_vp[khsv_r][khsv_c] = static_cast<float>(khsv_s);
                }
            }
            // The viewport range the resolved depth was written into: the pair the injection last filled (guarded or
            // routed; kept across frames until a session reset), else the scene's.
            if (g_inj_dp_valid) { g_snap_vp_lo = g_inj_dp[2]; g_snap_vp_hi = g_inj_dp[3]; }
            else { g_snap_vp_lo = g_scene_vp_min_d; g_snap_vp_hi = g_scene_vp_max_d; }
            g_snap_vp_valid = (g_snap_pair_src != 0) &&
                              fabsf(pv.projection[2][2]) > 1.0e-6f &&
                              g_snap_vp_hi > g_snap_vp_lo;
        }
    }
}
}