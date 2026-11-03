#pragma once

// The injection half (rendering_integration_injection.hpp): the snapshot rectangle and take. Never include it on its
// own.
namespace RenderIntegration {

// The occlusion snapshot and its stamps (pair, camera, folded view*proj, viewport depth range), taken by the flush with
// the main depth bound and the frame's pv. Stamps are read on the render thread.
// SNAP_RECT: the pixel rectangle the visible meshes project to under pv (their boxes' corners, plus a margin) -
// SSAO's pass rectangle (ssao_rect). False = a corner is behind the camera or the target size is unknown: the
// caller takes the whole target.
static constexpr LONG SNAP_RECT_MARGIN = 16;
inline bool snapshot_rect(const RVExtBridge::ProjectionViewTransform& pv, UINT w, UINT h, D3D11_RECT& out) {
    if (w == 0 || h == 0) return false;
    float vp[4][4];
    mul_4x4(pv.view, pv.projection, vp);
    float x0 = 1.0e9f, y0 = 1.0e9f, x1 = -1.0e9f, y1 = -1.0e9f;
    for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
        if (!g_scene.alive[i]) continue;
        const RenderObject& o = g_scene.objs[i];
        // INFRONT: no snapshot reader. SHOW_INFRONT: drawn (a head hidden around the camera would put a corner
        // behind it and the AO passes over the whole target).
        if (!obj_shown(o) || o.fullscreen || obj_in_front(o)) continue;
        float c[3];
        obj_center_engine(o, c);
        float he[3];
        world_half_extents(o, he);   // LOD_BOUNDS: everything it may draw, engine axes.
        for (int k = 0; k < 8; ++k) {
            const float p[3] = { c[0] + ((k & 1) ? he[0] : -he[0]),
                                 c[1] + ((k & 2) ? he[1] : -he[1]),
                                 c[2] + ((k & 4) ? he[2] : -he[2]) };
            float cl[4];
            for (int j = 0; j < 4; ++j) {
                cl[j] = p[0] * vp[0][j] + p[1] * vp[1][j] +
                        p[2] * vp[2][j] + vp[3][j];
            }
            if (cl[3] <= 1.0e-3f) return false;   // At or behind the camera: the hull rule fails.
            const float px = (cl[0] / cl[3] * 0.5f + 0.5f) * static_cast<float>(w);
            const float py = (0.5f - cl[1] / cl[3] * 0.5f) * static_cast<float>(h);
            if (!(px == px) || !(py == py)) return false;
            x0 = px < x0 ? px : x0;
            y0 = py < y0 ? py : y0;
            x1 = px > x1 ? px : x1;
            y1 = py > y1 ? py : y1;
        }
    }
    if (x1 < x0) {   // No visible mesh: nothing reads the snapshot; an empty rect stamps only.
        out = D3D11_RECT{ 0, 0, 0, 0 };
        return true;
    }
    const float lim = 1.0e7f;   // Far off-screen projections clamp before the LONG cast.
    auto cv = [lim](float v) { return static_cast<LONG>(v < -lim ? -lim : (v > lim ? lim : v)); };
    out.left = cv(x0) - SNAP_RECT_MARGIN;
    out.top = cv(y0) - SNAP_RECT_MARGIN;
    out.right = cv(x1) + SNAP_RECT_MARGIN + 1;
    out.bottom = cv(y1) + SNAP_RECT_MARGIN + 1;
    return true;
}

inline void snapshot_take(ID3D11Device* dev, ID3D11DeviceContext* ctx, const RVExtBridge::ProjectionViewTransform& pv) {
    KH_PROF_SCOPE(PROF_SNAPSHOT);
    KH_GPU_SCOPE(ctx, GPU_SNAPSHOT);   // PROF.
    std::string err = snapshot_composite_depth(dev, ctx);

    if (!err.empty()) {
        report_error_once_safe("KH RenderIntegration: occlusion snapshot: " + err);
    } else {
        g_snap.serial++;
        g_snap.ms = steady_now_ms();
        if (g_ro.engine_proj_valid) {
            g_snap.pair[0] = g_ro.engine_m22; g_snap.pair[1] = g_ro.engine_m32; g_snap.pair_src = 1;
        } else if (g_inj_pub.dp_valid) {
            g_snap.pair[0] = g_inj_pub.dp[0]; g_snap.pair[1] = g_inj_pub.dp[1]; g_snap.pair_src = 2;
        } else {
            g_snap.pair_src = 0;
        }
        cam_of(pv.view, g_snap.cam);
        // view*proj of this frame folded in double and rebased to g_snap.cam (row3 += cam . rows), so the shader
        // multiplies a camera-relative offset - exact algebra whichever point c is; c near the true camera only keeps
        // magnitudes small.
        {
            double v[4][4];
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    v[r][c] = static_cast<double>(pv.view[r][c]);
            for (int c = 0; c < 4; ++c) {
                v[3][c] += static_cast<double>(g_snap.cam[0]) * v[0][c]
                                   + static_cast<double>(g_snap.cam[1]) * v[1][c]
                                   + static_cast<double>(g_snap.cam[2]) * v[2][c];
            }
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    const double s = v[r][0] * static_cast<double>(pv.projection[0][c])
                                   + v[r][1] * static_cast<double>(pv.projection[1][c])
                                   + v[r][2] * static_cast<double>(pv.projection[2][c])
                                   + v[r][3] * static_cast<double>(pv.projection[3][c]);
                    g_snap.vp[r][c] = static_cast<float>(s);
                }
            }
            // The viewport range the resolved depth was written into: the pair the injection last filled (guarded or
            // routed; kept across frames until a session reset), else the scene's.
            if (g_inj_pub.dp_valid) { g_snap.vp_lo = g_inj_pub.dp[2]; g_snap.vp_hi = g_inj_pub.dp[3]; }
            else { g_snap.vp_lo = g_scene_vp_min_d; g_snap.vp_hi = g_scene_vp_max_d; }
            g_snap.vp_valid = (g_snap.pair_src != 0) &&
                              fabsf(pv.projection[2][2]) > 1.0e-6f &&
                              g_snap.vp_hi > g_snap.vp_lo;
        }
    }
}
}