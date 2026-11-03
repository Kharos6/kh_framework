#pragma once

// The feature half (rendering_integration_features.hpp): frustum culling, the scene grid queries and the local cull.
// Never include it on its own.
namespace RenderIntegration {

// Frustum culling. Nothing here is published or shared: every cull_build call fills a set the CALLER owns as a
// local (or a member of its pass object: InjectPass, FlushPass), at five sites on both threads - volume_seam_inject,
// pip_inject, the injection twice (InjectPass: the grid pre-cull and the colour pass) and the flush (FlushPass). A set
// never outlives the pass that built it, so it needs no lock and carries no cross-thread contract.

// The apex is recoverable from the planes (three intersect there); its offset from the camera is the rebase origin.
static constexpr float CULL_APEX_TOL   = 1.0f;   // 4th-plane residual at the apex, m.
static constexpr float CULL_ORIGIN_TOL = 4.0f;   // Recovered origin must be 0 or the camera.
static constexpr float CULL_PAD        = 2.0f;   // Radius pad: fp32 at 7.5 km, and cheap.

// Apex: the point on all of planes 0, 1, 2 (Cramer). A singular system is a degenerate frustum; the caller stands down.
inline bool cull_apex(const CullSet& s, float a[3]) {
    const float m[3][3] = {
        { s.p[0][0], s.p[0][1], s.p[0][2] },
        { s.p[1][0], s.p[1][1], s.p[1][2] },
        { s.p[2][0], s.p[2][1], s.p[2][2] },
    };
    const float r[3] = { -s.p[0][3], -s.p[1][3], -s.p[2][3] };
    const float det =
        m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
        m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
        m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    if (!(det > 1.0e-9f || det < -1.0e-9f)) return false;
    const float inv = 1.0f / det;
    a[0] = inv * (r[0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                  m[0][1] * (r[1] * m[2][2] - m[1][2] * r[2]) +
                  m[0][2] * (r[1] * m[2][1] - m[1][1] * r[2]));
    a[1] = inv * (m[0][0] * (r[1] * m[2][2] - m[1][2] * r[2]) -
                  r[0] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                  m[0][2] * (m[1][0] * r[2] - r[1] * m[2][0]));
    a[2] = inv * (m[0][0] * (m[1][1] * r[2] - r[1] * m[2][1]) -
                  m[0][1] * (m[1][0] * r[2] - r[1] * m[2][0]) +
                  r[0] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));
    return true;
}

inline void cull_build(const float vp[4][4], const float cam[3], CullSet& o) {
    KH_PROF_SCOPE(PROF_CULL_BUILD);   // PROF.
    o.valid = false;

    for (int i = 0; i < 4; ++i) {
        const int r = i >> 1;   // 0 = x column, 1 = y column.
        const float s = (i & 1) ? -1.0f : 1.0f;

        for (int c = 0; c < 4; ++c)
            o.p[i][c] = vp[c][3] + s * vp[c][r];

        const float l = sqrtf(o.p[i][0] * o.p[i][0] +
                              o.p[i][1] * o.p[i][1] +
                              o.p[i][2] * o.p[i][2]);
        if (!(l > 1.0e-9f)) {  return; }

        for (int c = 0; c < 4; ++c) o.p[i][c] /= l;
    }

    // The apex, then the fourth plane must pass through it - a matrix that gives none is not a frustum to cull against.
    float ap[3];
    if (!cull_apex(o, ap)) {  return; }
    const float res = fabsf(o.p[3][0] * ap[0] + o.p[3][1] * ap[1] +
                            o.p[3][2] * ap[2] + o.p[3][3]);
    if (!(res <= CULL_APEX_TOL)) {  return; }
    {   // CULL_BACK: forward from the inward side normals; apex is the eye.
        float f[3] = { 0.0f, 0.0f, 0.0f };
        for (int i = 0; i < 4; ++i)
            for (int c = 0; c < 3; ++c) f[c] += o.p[i][c];
        const float fl = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
        if (fl > 1.0e-6f) {
            for (int c = 0; c < 3; ++c) o.fwd[c] = f[c] / fl;
            o.apex[0] = ap[0]; o.apex[1] = ap[1]; o.apex[2] = ap[2];
            o.back_ok = true;
        } else {
            o.back_ok = false;   // Degenerate: the back half stays unculled.
        }
    }
    // The recovered origin: only none and the camera are accepted, so a cycle latch or a foreign view cannot cull.
    const float ac[3] = { ap[0] - cam[0], ap[1] - cam[1],
                          ap[2] - cam[2] };
    const float dabs = sqrtf(ac[0] * ac[0] + ac[1] * ac[1] +
                             ac[2] * ac[2]);
    const float dreb = sqrtf(ap[0] * ap[0] + ap[1] * ap[1] +
                             ap[2] * ap[2]);

    if (dabs <= CULL_ORIGIN_TOL) {
        o.bias[0] = o.bias[1] = o.bias[2] = 0.0f;
    } else if (dreb <= CULL_ORIGIN_TOL) {
        o.bias[0] = ac[0];
        o.bias[1] = ac[1];
        o.bias[2] = ac[2];
    } else {  return; }

    o.valid = true;
}

// True = draw it. Fails open in every direction: an unbuilt set, a zero radius, a stood-down pass and anything
// non-finite all return true.
inline bool mesh_visible(const CullSet& s, const float ctr[3], float r) {
    if (!s.valid) return true;
    if (!(r > 0.0f)) return true;

    // Into the matrix's own space, then a padded sphere test.
    const float q[3] = { ctr[0] + s.bias[0], ctr[1] + s.bias[1],
                         ctr[2] + s.bias[2] };
    const float b = r + CULL_PAD;

    for (int i = 0; i < 4; ++i) {
        const float d = s.p[i][0] * q[0] + s.p[i][1] * q[1] +
                        s.p[i][2] * q[2] + s.p[i][3];
        if (d < -b) return false;
    }

    if (s.back_on && s.back_ok) {   // CULL_BACK: wholly behind the apex plane.
        const float dn = s.fwd[0] * (q[0] - s.apex[0]) +
                         s.fwd[1] * (q[1] - s.apex[1]) +
                         s.fwd[2] * (q[2] - s.apex[2]);
        if (dn < -b) {  return false; }
    }

    return true;
}

// Grid query: every live mesh slot in a cell whose loose bounds (footprint, y span, padded by its largest object
// radius + CULL_PAD) can meet the frustum. Conservative; mesh_visible still runs per object. An invalid cull
// set returns everything.
inline void scene_grid_query(const CullSet& s, std::vector<uint32_t>& out) {
    out.clear();
    for (const auto& kv : g_scene.grid) {
        const SceneCell& c = kv.second;
        if (s.valid) {
            const int32_t cx = static_cast<int32_t>(kv.first >> 32);
            const int32_t cz = static_cast<int32_t>(kv.first & 0xFFFFFFFFu);
            const float pad = c.rmax + CULL_PAD;
            const float mn[3] = { cx * SCENE_CELL_M - pad + s.bias[0],
                                  c.ymin - pad + s.bias[1],
                                  cz * SCENE_CELL_M - pad + s.bias[2] };
            const float mx[3] = { (cx + 1) * SCENE_CELL_M + pad + s.bias[0],
                                      c.ymax + pad + s.bias[1],
                                      (cz + 1) * SCENE_CELL_M + pad + s.bias[2] };
            bool in = true;
            for (int i = 0; i < 4 && in; ++i) {   // P-vertex test per plane.
                const float* p = s.p[i];
                const float d = p[0] * (p[0] >= 0.0f ? mx[0] : mn[0]) +
                                p[1] * (p[1] >= 0.0f ? mx[1] : mn[1]) +
                                p[2] * (p[2] >= 0.0f ? mx[2] : mn[2]) + p[3];
                if (d < 0.0f) in = false;
            }
            if (in && s.back_on && s.back_ok) {
                const float d = s.fwd[0] * ((s.fwd[0] >= 0.0f ? mx[0] : mn[0]) - s.apex[0]) +
                                s.fwd[1] * ((s.fwd[1] >= 0.0f ? mx[1] : mn[1]) - s.apex[1]) +
                                s.fwd[2] * ((s.fwd[2] >= 0.0f ? mx[2] : mn[2]) - s.apex[2]);
                if (d < 0.0f) in = false;
            }
            if (!in) continue;
        }
        out.insert(out.end(), c.slots.begin(), c.slots.end());
    }
}

// Every live mesh slot in a cell whose loose bounds can meet the engine-axes box (same padding rule). Conservative.
inline void scene_grid_query_box(const float mn[3], const float mx[3], std::vector<uint32_t>& out) {
    out.clear();
    for (const auto& kv : g_scene.grid) {
        const SceneCell& c = kv.second;
        const int32_t cx = static_cast<int32_t>(kv.first >> 32);
        const int32_t cz = static_cast<int32_t>(kv.first & 0xFFFFFFFFu);
        const float pad = c.rmax + CULL_PAD;
        if (cx * SCENE_CELL_M - pad > mx[0] || (cx + 1) * SCENE_CELL_M + pad < mn[0]) continue;
        if (c.ymin - pad > mx[1] || c.ymax + pad < mn[1]) continue;
        if (cz * SCENE_CELL_M - pad > mx[2] || (cz + 1) * SCENE_CELL_M + pad < mn[2]) continue;
        out.insert(out.end(), c.slots.begin(), c.slots.end());
    }
}
// vp: the row-vector clip matrix (clip = p x vp) of the camera-relative frame; cam: fxCam (w armed).
inline bool local_cull_make(LocalCull& c, const float vp[4][4], const float cam[4],
                            bool pair_check, float m22, float m32) {
    c.on = false;
    if (!(cam[3] > 0.5f)) return false;   // Absolute frames are not culled.
    static const int ax[4] = { 0, 0, 1, 1 };
    static const double sg[4] = { 1.0, -1.0, 1.0, -1.0 };   // w + x, w - x, w + y, w - y.
    for (int p = 0; p < 4; ++p) {
        double e[4];
        for (int k = 0; k < 4; ++k) {
            e[k] = static_cast<double>(vp[k][3]) +
                   sg[p] * static_cast<double>(vp[k][ax[p]]);
        }
        const double n = sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
        if (!(n > 1.0e-12 && n < 1.0e30)) return false;
        for (int k = 0; k < 4; ++k) c.plane[p][k] = e[k] / n;
    }
    for (int k = 0; k < 3; ++k) c.cam[k] = static_cast<double>(cam[k]);
    c.pair_check = pair_check;
    c.pair[0] = m22;
    c.pair[1] = m32;
    c.on = true;
    return true;
}
inline bool local_culled(const LocalCull& c, const RenderObject& o) {
    if (!c.on || !o.localized || o.local_inverse || o.effect <= 0 ||
        o.effect == EFFECT_CUSTOM) return false;
    if (c.pair_check && (!g_ro.engine_proj_valid || g_ro.engine_m22 != c.pair[0] ||
                         g_ro.engine_m32 != c.pair[1])) return false;
    // fill_local_band_cb's lanes, as the shader reads them (TWIN): centre and radii in engine axes, the
    // falloff lane over the mean radius.
    const float r[3] = { o.local_radius[0], o.local_radius[2], o.local_radius[1] };
    const float mean = (o.local_radius[0] + o.local_radius[1] + o.local_radius[2]) / 3.0f;
    const float fl = o.local_falloff / (mean > 0.01f ? mean : 0.01f);
    const double f = 1.0 + static_cast<double>(fl > 0.001f ? fl : 0.001f);
    const double ctr[3] = { static_cast<double>(o.pos[0]) - c.cam[0],
                            static_cast<double>(o.pos[2]) - c.cam[1],
                            static_cast<double>(o.pos[1]) - c.cam[2] };
    double h[3];
    for (int k = 0; k < 3; ++k) {
        h[k] = static_cast<double>(r[k] > 0.01f ? r[k] : 0.01f) * f;
        if (!(h[k] < 1.0e30) || !(fabs(ctr[k]) < 1.0e30)) return false;   // Non-finite.
    }
    for (int p = 0; p < 4; ++p) {
        const double* pl = c.plane[p];
        const double s = pl[0] * ctr[0] + pl[1] * ctr[1] +
                         pl[2] * ctr[2] + pl[3];
        const double e = fabs(pl[0]) * h[0] + fabs(pl[1]) * h[1] +
                         fabs(pl[2]) * h[2];
        if (s + e < 0.0) return true;
    }
    return false;
}
}