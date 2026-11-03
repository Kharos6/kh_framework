#pragma once

// The feature half (rendering_integration_features.hpp): the built-in mesh generator (meshgen), vertex-cache ordering,
// welding and the occluder prep. Never include it on its own.
namespace RenderIntegration {

namespace meshgen {

// One quad = two triangles with an explicit outward normal. Corners may arrive
// in any perimeter order; bake normalizes the winding. UVs are face-local
// (a->b->c->d = (0,0)(1,0)(1,1)(0,1)): flat builtins are texture-ready per face.
inline void quad(std::vector<MeshVertex>& v,
                 const float a[3], const float b[3],
                 const float c[3], const float d[3], const float n[3]) {
    static const float uvs[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f },
                                     { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    const float* corners[6] = { a, b, c, a, c, d };
    const int    uvi[6]     = { 0, 1, 2, 0, 2, 3 };

    // Analytic tangent: T = normalize(b - a) (the uv u axis); handedness = sign
    // of cross(N, T) . (d - a).
    float khq_t[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const float khq_tl = sqrtf(khq_t[0] * khq_t[0] + khq_t[1] * khq_t[1] + khq_t[2] * khq_t[2]);

    if (khq_tl > 1e-8f) {
        khq_t[0] /= khq_tl; khq_t[1] /= khq_tl; khq_t[2] /= khq_tl;
    } else {
        khq_t[0] = 1.0f; khq_t[1] = 0.0f; khq_t[2] = 0.0f;
    }

    const float khq_bx[3] = {
        n[1] * khq_t[2] - n[2] * khq_t[1],
        n[2] * khq_t[0] - n[0] * khq_t[2],
        n[0] * khq_t[1] - n[1] * khq_t[0],
    };
    const float khq_w = (khq_bx[0] * (d[0] - a[0]) + khq_bx[1] * (d[1] - a[1]) +
                         khq_bx[2] * (d[2] - a[2])) >= 0.0f ? 1.0f : -1.0f;

    for (int i = 0; i < 6; ++i) {
        MeshVertex mv = {};
        memcpy(mv.pos, corners[i], sizeof(mv.pos));
        memcpy(mv.nrm, n, sizeof(mv.nrm));
        mv.uv[0] = uvs[uvi[i]][0];
        mv.uv[1] = uvs[uvi[i]][1];
        memcpy(mv.tan, khq_t, sizeof(khq_t));
        mv.tan[3] = khq_w;
        v.push_back(mv);
    }
}

// Triangle with per-vertex normals and UVs (the smooth generators). bake's
// winding test reads the first vertex's normal.
inline void tri(std::vector<MeshVertex>& v,
                const float a[3], const float na[3], const float ua[2],
                const float b[3], const float nb[3], const float ub[2],
                const float c[3], const float nc[3], const float uc[2]) {
    const float* p[3] = { a, b, c };
    const float* n[3] = { na, nb, nc };
    const float* u[3] = { ua, ub, uc };

    // Per-face uv-delta tangent (the standard derivation), shared by all three
    // vertices; degenerate uv area falls back to any vector perpendicular to
    // the first normal.
    const float khe1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const float khe2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
    const float khdu1 = ub[0] - ua[0], khdv1 = ub[1] - ua[1];
    const float khdu2 = uc[0] - ua[0], khdv2 = uc[1] - ua[1];
    const float khr = khdu1 * khdv2 - khdu2 * khdv1;
    float kht_t[3];
    float kht_w = 1.0f;

    if (khr > 1e-10f || khr < -1e-10f) {
        const float khir = 1.0f / khr;
        kht_t[0] = (khe1[0] * khdv2 - khe2[0] * khdv1) * khir;
        kht_t[1] = (khe1[1] * khdv2 - khe2[1] * khdv1) * khir;
        kht_t[2] = (khe1[2] * khdv2 - khe2[2] * khdv1) * khir;
        const float khb[3] = {
            (khe2[0] * khdu1 - khe1[0] * khdu2) * khir,
            (khe2[1] * khdu1 - khe1[1] * khdu2) * khir,
            (khe2[2] * khdu1 - khe1[2] * khdu2) * khir,
        };
        const float khcx[3] = {
            na[1] * kht_t[2] - na[2] * kht_t[1],
            na[2] * kht_t[0] - na[0] * kht_t[2],
            na[0] * kht_t[1] - na[1] * kht_t[0],
        };
        kht_w = (khcx[0] * khb[0] + khcx[1] * khb[1] + khcx[2] * khb[2]) >= 0.0f ? 1.0f : -1.0f;
    } else {
        // uv-degenerate: any perpendicular to the first normal.
        if (na[0] * na[0] < 0.9f) { kht_t[0] = 0.0f; kht_t[1] = -na[2]; kht_t[2] = na[1]; }
        else                      { kht_t[0] = -na[2]; kht_t[1] = 0.0f; kht_t[2] = na[0]; }
    }

    const float kht_l = sqrtf(kht_t[0] * kht_t[0] + kht_t[1] * kht_t[1] + kht_t[2] * kht_t[2]);

    if (kht_l > 1e-8f) {
        kht_t[0] /= kht_l; kht_t[1] /= kht_l; kht_t[2] /= kht_l;
    } else {
        kht_t[0] = 1.0f; kht_t[1] = 0.0f; kht_t[2] = 0.0f;
    }

    for (int i = 0; i < 3; ++i) {
        MeshVertex mv = {};
        memcpy(mv.pos, p[i], sizeof(mv.pos));
        memcpy(mv.nrm, n[i], sizeof(mv.nrm));
        mv.uv[0] = u[i][0];
        mv.uv[1] = u[i][1];
        memcpy(mv.tan, kht_t, sizeof(kht_t));
        mv.tan[3] = kht_w;
        v.push_back(mv);
    }
}

// Axis-aligned block between mn and mx: 6 quads with exact face normals.
inline void block(std::vector<MeshVertex>& v, const float mn[3], const float mx[3]) {
    const float p000[3] = { mn[0], mn[1], mn[2] };
    const float p001[3] = { mn[0], mn[1], mx[2] };
    const float p010[3] = { mn[0], mx[1], mn[2] };
    const float p011[3] = { mn[0], mx[1], mx[2] };
    const float p100[3] = { mx[0], mn[1], mn[2] };
    const float p101[3] = { mx[0], mn[1], mx[2] };
    const float p110[3] = { mx[0], mx[1], mn[2] };
    const float p111[3] = { mx[0], mx[1], mx[2] };
    const float nxp[3] = { 1.0f, 0.0f, 0.0f }, nxn[3] = { -1.0f, 0.0f, 0.0f };
    const float nyp[3] = { 0.0f, 1.0f, 0.0f }, nyn[3] = { 0.0f, -1.0f, 0.0f };
    const float nzp[3] = { 0.0f, 0.0f, 1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
    quad(v, p100, p110, p111, p101, nxp);
    quad(v, p000, p001, p011, p010, nxn);
    quad(v, p010, p011, p111, p110, nyp);
    quad(v, p000, p100, p101, p001, nyn);
    quad(v, p001, p101, p111, p011, nzp);
    quad(v, p000, p010, p110, p100, nzn);
}

// Parametric builtins (arma axes: x east, y north, z up; every one fits
// [-0.5, 0.5]^3 exactly, the size contract). Smooth surfaces carry per-vertex
// normals, caps and flat faces carry face normals; UVs are the conventional wraps.

// UV sphere, radius 0.5. Pole rows emit one zero-area triangle per slice;
// harmless, never rasterized.
inline void uv_sphere(std::vector<MeshVertex>& v, int stacks, int slices) {
    const float khms_pi = 3.14159265358979f;

    for (int st = 0; st < stacks; ++st) {
        const float t0 = static_cast<float>(st) / static_cast<float>(stacks);
        const float t1 = static_cast<float>(st + 1) / static_cast<float>(stacks);

        for (int sl = 0; sl < slices; ++sl) {
            const float s0 = static_cast<float>(sl) / static_cast<float>(slices);
            const float s1 = static_cast<float>(sl + 1) / static_cast<float>(slices);
            const float ts[2] = { t0, t1 }, ss[2] = { s0, s1 };
            // (s0,t0) (s1,t0) (s1,t1) (s0,t1).
            const int ci[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
            float p[4][3], n[4][3], u[4][2];

            for (int k = 0; k < 4; ++k) {
                // KH_OCC_BOX: watertight - the seam's closing column takes the first
                // column's angle and a pole is exact (float 2 pi and pi / 2 are not), so
                // every shared corner is one position bit for bit, but for the sign of a
                // zero (a pole's 0 * cos / 0 * sin gives -0.0f on some slices; KH_OCC_BOX's weld adds 0.0f, which folds
                // it to +0.0f).
                const int khms_s = st + ci[k][1];
                const int khms_w = (sl + ci[k][0]) % slices;
                const float la = (static_cast<float>(khms_s) / static_cast<float>(stacks) - 0.5f) * khms_pi;
                const float lo = static_cast<float>(khms_w) / static_cast<float>(slices) * 2.0f * khms_pi;
                const float khms_cl = (khms_s == 0 || khms_s == stacks) ? 0.0f : cosf(la);
                const float khms_sa = khms_s == 0 ? -1.0f : (khms_s == stacks ? 1.0f : sinf(la));
                n[k][0] = khms_cl * cosf(lo);
                n[k][1] = khms_cl * sinf(lo);
                n[k][2] = khms_sa;
                p[k][0] = n[k][0] * 0.5f;
                p[k][1] = n[k][1] * 0.5f;
                p[k][2] = n[k][2] * 0.5f;
                u[k][0] = ss[ci[k][0]];
                u[k][1] = ts[ci[k][1]];
            }

            tri(v, p[0], n[0], u[0], p[1], n[1], u[1], p[2], n[2], u[2]);
            tri(v, p[0], n[0], u[0], p[2], n[2], u[2], p[3], n[3], u[3]);
        }
    }
}

// Cylinder: axis z-up, radius 0.5, height 1, capped. Smooth radial side
// normals, flat cap fans (disc UVs).
inline void cylinder(std::vector<MeshVertex>& v, int slices) {
    const float khms_pi = 3.14159265358979f;
    const float top[3] = { 0.0f, 0.0f,  0.5f }, bot[3] = { 0.0f, 0.0f, -0.5f };
    const float nzp[3] = { 0.0f, 0.0f,  1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
    const float ucc[2] = { 0.5f, 0.5f };

    for (int sl = 0; sl < slices; ++sl) {
        const float s0 = static_cast<float>(sl) / static_cast<float>(slices);
        const float s1 = static_cast<float>(sl + 1) / static_cast<float>(slices);
        // KH_OCC_BOX: the closing slice ends on the first one's angle exactly
        // (float 2 pi does not), so the seam's corners are one position.
        const float s1w = static_cast<float>((sl + 1) % slices) / static_cast<float>(slices);
        const float a0 = s0 * 2.0f * khms_pi, a1 = s1w * 2.0f * khms_pi;
        const float r0[3] = { cosf(a0), sinf(a0), 0.0f };
        const float r1[3] = { cosf(a1), sinf(a1), 0.0f };
        const float p00[3] = { r0[0] * 0.5f, r0[1] * 0.5f, -0.5f };
        const float p10[3] = { r1[0] * 0.5f, r1[1] * 0.5f, -0.5f };
        const float p01[3] = { r0[0] * 0.5f, r0[1] * 0.5f,  0.5f };
        const float p11[3] = { r1[0] * 0.5f, r1[1] * 0.5f,  0.5f };
        const float u00[2] = { s0, 0.0f }, u10[2] = { s1, 0.0f };
        const float u01[2] = { s0, 1.0f }, u11[2] = { s1, 1.0f };
        tri(v, p00, r0, u00, p10, r1, u10, p11, r1, u11);
        tri(v, p00, r0, u00, p11, r1, u11, p01, r0, u01);
        const float ud0[2] = { 0.5f + r0[0] * 0.5f, 0.5f + r0[1] * 0.5f };
        const float ud1[2] = { 0.5f + r1[0] * 0.5f, 0.5f + r1[1] * 0.5f };
        tri(v, top, nzp, ucc, p01, nzp, ud0, p11, nzp, ud1);
        tri(v, bot, nzn, ucc, p10, nzn, ud1, p00, nzn, ud0);
    }
}

// Cone: apex at z = +0.5, base disc radius 0.5 at z = -0.5. Smooth slant
// normals normalize(cos, sin, r/h) with r/h = 0.5; the apex takes the face's
// mid-angle normal (the standard degenerate-pole treatment).
inline void cone(std::vector<MeshVertex>& v, int slices) {
    const float khms_pi = 3.14159265358979f;
    const float apex[3] = { 0.0f, 0.0f, 0.5f }, bot[3] = { 0.0f, 0.0f, -0.5f };
    const float nzn[3] = { 0.0f, 0.0f, -1.0f };
    const float ucc[2] = { 0.5f, 0.5f };
    const float khms_l = sqrtf(1.25f);

    for (int sl = 0; sl < slices; ++sl) {
        const float s0 = static_cast<float>(sl) / static_cast<float>(slices);
        const float s1 = static_cast<float>(sl + 1) / static_cast<float>(slices);
        const float a0 = s0 * 2.0f * khms_pi, a1 = s1 * 2.0f * khms_pi;
        const float am = 0.5f * (a0 + a1);
        // KH_OCC_BOX: the closing slice's corner on the first one's angle exactly.
        const float a1w = static_cast<float>((sl + 1) % slices) / static_cast<float>(slices) * 2.0f * khms_pi;
        const float p0[3] = { cosf(a0) * 0.5f, sinf(a0) * 0.5f, -0.5f };
        const float p1[3] = { cosf(a1w) * 0.5f, sinf(a1w) * 0.5f, -0.5f };
        const float n0[3] = { cosf(a0) / khms_l, sinf(a0) / khms_l, 0.5f / khms_l };
        const float n1[3] = { cosf(a1w) / khms_l, sinf(a1w) / khms_l, 0.5f / khms_l };
        const float na[3] = { cosf(am) / khms_l, sinf(am) / khms_l, 0.5f / khms_l };
        const float u0[2] = { s0, 0.0f }, u1[2] = { s1, 0.0f };
        const float ua[2] = { 0.5f * (s0 + s1), 1.0f };
        tri(v, p0, n0, u0, p1, n1, u1, apex, na, ua);
        const float ud0[2] = { 0.5f + cosf(a0) * 0.5f, 0.5f + sinf(a0) * 0.5f };
        const float ud1[2] = { 0.5f + cosf(a1) * 0.5f, 0.5f + sinf(a1) * 0.5f };
        tri(v, bot, nzn, ucc, p1, nzn, ud1, p0, nzn, ud0);
    }
}

// Square pyramid: base at z = -0.5, apex at (0, 0, +0.5).
inline void pyramid(std::vector<MeshVertex>& v) {
    const float apex[3] = { 0.0f, 0.0f, 0.5f };
    const float b[4][3] = { { -0.5f, -0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f },
                            {  0.5f,  0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f } };
    const float nzn[3] = { 0.0f, 0.0f, -1.0f };
    quad(v, b[0], b[1], b[2], b[3], nzn);
    // Side face normals: normalize(outward edge normal, rise) - the cross
    // product of (base edge, base corner -> apex) is (edge_out, 0.5)/L.
    const float khms_l = sqrtf(1.25f);
    const float ns[4][3] = { {  0.0f,          -1.0f / khms_l, 0.5f / khms_l },
                             {  1.0f / khms_l,  0.0f,          0.5f / khms_l },
                             {  0.0f,           1.0f / khms_l, 0.5f / khms_l },
                             { -1.0f / khms_l,  0.0f,          0.5f / khms_l } };
    const float u0[2] = { 0.0f, 0.0f }, u1[2] = { 1.0f, 0.0f }, ua[2] = { 0.5f, 1.0f };

    for (int f = 0; f < 4; ++f) {
        tri(v, b[f], ns[f], u0, b[(f + 1) & 3], ns[f], u1, apex, ns[f], ua);
    }
}

// Bake an arma-axes mesh into the engine layout: arma [x, y, z] (east, north,
// up) -> engine [x, z, y] (the swap every SQF position/size fill performs),
// then reorder each triangle so cross(b-a, c-a) points along its stored normal
// (the swap flips handedness).
// khb_ext: the per-axis extents the positions were squashed by, in the ENGINE
// frame (the import's d.native_ext), or null for a mesh authored in the unit
// box (the builtins). The winding test is weighted by 1 / ext^2: squashing by
// diag(1 / ext) turns the geometric cross into diag(ext) . n_geom and
// KH_IMPORT_NRM stores diag(ext) . n, so the plain dot is weighted by ext^2 and
// a thin mesh's smoothed normals decide its sign by their tiny in-plane parts;
// the weighting recovers sign(n_geom . n), the authored winding, never a
// back-facing hole. Null weights are exactly 1.0.
inline void bake(std::vector<MeshVertex>& v, const float* khb_ext = nullptr) {
    for (auto& mv : v) {
        const float py = mv.pos[1]; mv.pos[1] = mv.pos[2]; mv.pos[2] = py;
        const float ny = mv.nrm[1]; mv.nrm[1] = mv.nrm[2]; mv.nrm[2] = ny;
        const float ty = mv.tan[1]; mv.tan[1] = mv.tan[2]; mv.tan[2] = ty;
        // The y/z swap has det -1, so the shader's bitangent B = w * cross(N,
        // T) needs w negated. Applies to builtin and mikktspace tangents alike
        // (both author pre-bake).
        mv.tan[3] = -mv.tan[3];
    }

    for (size_t i = 0; i + 2 < v.size(); i += 3) {
        const float* a = v[i].pos;
        const float* b = v[i + 1].pos;
        const float* c = v[i + 2].pos;
        const float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
        const float e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
        const float g[3] = {
            e1[1] * e2[2] - e1[2] * e2[1],
            e1[2] * e2[0] - e1[0] * e2[2],
            e1[0] * e2[1] - e1[1] * e2[0],
        };
        const float* n = v[i].nrm;
        float khb_w[3] = { 1.0f, 1.0f, 1.0f };
        if (khb_ext) {
            for (int k = 0; k < 3; ++k) {
                const float khb_e = fabsf(khb_ext[k]) > 1.0e-6f ? khb_ext[k] : 1.0f;
                khb_w[k] = 1.0f / (khb_e * khb_e);
            }
        }

        if (g[0] * n[0] * khb_w[0] + g[1] * n[1] * khb_w[1] + g[2] * n[2] * khb_w[2] < 0.0f) {
            const MeshVertex tmp = v[i + 1];
            v[i + 1] = v[i + 2];
            v[i + 2] = tmp;
        }
    }
}

}

// Weld key: an exact 48-byte match (position, normal, uv, tangent), so shading
// is unchanged; the index stream mirrors input order, so every triangle-vertex
// range is already the index range. Idempotent. Then Forsyth's vertex-cache
// order per submesh range per level plus a first-use vertex remap, baked into
// the mesh cache (KH_MESH_CACHE_VERSION).
static constexpr uint32_t KH_MCO_CACHE   = 32u;   // Modelled LRU cache depth.
inline float kh_mco_cache_score(uint32_t khmc_p) {
    if (khmc_p >= KH_MCO_CACHE) return 0.0f;
    if (khmc_p < 3u) return 0.75f;   // The last triangle's own vertices: a flat score, so
                                     // a strip does not chase itself.
    const float khmc_s = 1.0f - (static_cast<float>(khmc_p) - 3.0f) / (static_cast<float>(KH_MCO_CACHE) - 3.0f);
    return powf(khmc_s, 1.5f);
}
inline float kh_mco_valence_score(uint32_t khmv_n) {
    if (khmv_n == 0u) return -1.0f;
    return 2.0f * powf(static_cast<float>(khmv_n), -0.5f);
}
// One index range: the khmo_c indices at khmo_ix (a multiple of 3).
inline void kh_mco_range(uint32_t* khmo_ix, uint32_t khmo_c, uint32_t khmo_vn) {
    const uint32_t khmo_tn = khmo_c / 3u;
    if (khmo_tn < 2u || khmo_vn == 0u) return;
    for (uint32_t i = 0; i < khmo_c; ++i) if (khmo_ix[i] >= khmo_vn) return;   // Never touch a bad stream.
    std::vector<uint32_t> khmo_deg(khmo_vn, 0u);          // Remaining triangles per vertex.
    for (uint32_t i = 0; i < khmo_c; ++i) ++khmo_deg[khmo_ix[i]];
    std::vector<uint32_t> khmo_off(khmo_vn + 1u, 0u);     // Vertex -> its triangle list.
    for (uint32_t v = 0; v < khmo_vn; ++v) khmo_off[v + 1u] = khmo_off[v] + khmo_deg[v];
    std::vector<uint32_t> khmo_adj(khmo_c);
    {
        std::vector<uint32_t> khmo_fill(khmo_off.begin(), khmo_off.end() - 1);
        for (uint32_t t = 0; t < khmo_tn; ++t)
            for (uint32_t k = 0; k < 3u; ++k) khmo_adj[khmo_fill[khmo_ix[t * 3u + k]]++] = t;
    }
    std::vector<uint32_t> khmo_cpos(khmo_vn, KH_MCO_CACHE);   // Cache position (>= CACHE = out).
    std::vector<float>    khmo_vsc(khmo_vn, 0.0f);
    for (uint32_t v = 0; v < khmo_vn; ++v)
        khmo_vsc[v] = kh_mco_cache_score(KH_MCO_CACHE) + kh_mco_valence_score(khmo_deg[v]);
    std::vector<float>   khmo_tsc(khmo_tn, 0.0f);
    std::vector<uint8_t> khmo_done(khmo_tn, 0u);
    for (uint32_t t = 0; t < khmo_tn; ++t)
        khmo_tsc[t] = khmo_vsc[khmo_ix[t * 3u]] + khmo_vsc[khmo_ix[t * 3u + 1u]] + khmo_vsc[khmo_ix[t * 3u + 2u]];
    // KH_MCO_TREE - the fallback's pick without a scan of every triangle left: a max tree over the triangles' scores,
    // a done one at -inf, a tie to the lower index. The fallback runs only when no triangle left touches the cache, so
    // each vertex of one left is out of it and its khmo_vsc is the out-of-cache score it took on leaving (or at the
    // start), unchanged since: only an emit moves a valence, and an emit puts the vertex in the cache. An emit marks
    // its vertices stale; the fallback first re-scores the stale vertices' triangles from khmo_vsc with the scan's own
    // sum, so every leaf holds what the scan would compute and the pick is the scan's - the first of the best. One
    // scan per exhausted region was quadratic in a mesh of many regions (a flat-shaded one: every face is one); a pick
    // is now O(log n), and the stale triangles' refresh O(log n) each.
    uint32_t khmo_tp = 1u;
    while (khmo_tp < khmo_tn) khmo_tp <<= 1;
    const float khmo_inf = std::numeric_limits<float>::infinity();
    std::vector<float> khmo_tv(2u * static_cast<size_t>(khmo_tp), -khmo_inf);
    std::vector<uint32_t> khmo_ti(2u * static_cast<size_t>(khmo_tp), 0u);
    auto khmo_tfix = [&](uint32_t khmo_nd) {   // The nodes above a leaf, again.
        for (khmo_nd >>= 1; khmo_nd >= 1u; khmo_nd >>= 1) {
            const uint32_t khmo_l = 2u * khmo_nd;
            const bool khmo_lw = khmo_tv[khmo_l] >= khmo_tv[khmo_l + 1u];
            khmo_tv[khmo_nd] = khmo_lw ? khmo_tv[khmo_l] : khmo_tv[khmo_l + 1u];
            khmo_ti[khmo_nd] = khmo_lw ? khmo_ti[khmo_l] : khmo_ti[khmo_l + 1u];
        }
    };
    for (uint32_t t = 0; t < khmo_tn; ++t) { khmo_tv[khmo_tp + t] = khmo_tsc[t]; khmo_ti[khmo_tp + t] = t; }
    for (uint32_t khmo_nd = khmo_tp - 1u; khmo_nd >= 1u; --khmo_nd) {
        const uint32_t khmo_l = 2u * khmo_nd;
        const bool khmo_lw = khmo_tv[khmo_l] >= khmo_tv[khmo_l + 1u];
        khmo_tv[khmo_nd] = khmo_lw ? khmo_tv[khmo_l] : khmo_tv[khmo_l + 1u];
        khmo_ti[khmo_nd] = khmo_lw ? khmo_ti[khmo_l] : khmo_ti[khmo_l + 1u];
    }
    std::vector<uint8_t>  khmo_stale(khmo_vn, 0u);
    std::vector<uint32_t> khmo_stl;   // The stale vertices, each once.
    std::vector<uint32_t> khmo_out;
    khmo_out.reserve(khmo_c);
    std::vector<uint32_t> khmo_cache;   // Most recent first.
    khmo_cache.reserve(KH_MCO_CACHE + 3u);
    int64_t  khmo_best = -1;
    float    khmo_best_s = -1.0e30f;
    for (uint32_t t = 0; t < khmo_tn; ++t) if (khmo_tsc[t] > khmo_best_s) { khmo_best_s = khmo_tsc[t]; khmo_best = t; }
    for (uint32_t khmo_emit = 0; khmo_emit < khmo_tn; ++khmo_emit) {
        if (khmo_best < 0) {   // Nothing in the cache neighbourhood: the best-scoring triangle left (KH_MCO_TREE).
            for (uint32_t khmo_v : khmo_stl) {
                khmo_stale[khmo_v] = 0u;
                for (uint32_t khmo_a = khmo_off[khmo_v]; khmo_a < khmo_off[khmo_v + 1u]; ++khmo_a) {
                    const uint32_t u = khmo_adj[khmo_a];
                    khmo_tv[khmo_tp + u] = khmo_done[u] ? -khmo_inf
                                         : khmo_vsc[khmo_ix[u * 3u]] + khmo_vsc[khmo_ix[u * 3u + 1u]] +
                                           khmo_vsc[khmo_ix[u * 3u + 2u]];
                    khmo_tfix(khmo_tp + u);
                }
            }
            khmo_stl.clear();
            if (!(khmo_tv[1] > -khmo_inf)) break;   // None left.
            khmo_best = khmo_ti[1];
        }
        const uint32_t t = static_cast<uint32_t>(khmo_best);
        khmo_done[t] = 1u;
        for (uint32_t k = 0; k < 3u; ++k) {
            const uint32_t v = khmo_ix[t * 3u + k];
            khmo_out.push_back(v);
            --khmo_deg[v];
            if (!khmo_stale[v]) { khmo_stale[v] = 1u; khmo_stl.push_back(v); }   // KH_MCO_TREE.
            // Move v to the cache front (an LRU list of positions, the model of the paper).
            size_t khmo_at = khmo_cache.size();
            for (size_t c = 0; c < khmo_cache.size(); ++c) if (khmo_cache[c] == v) { khmo_at = c; break; }
            if (khmo_at < khmo_cache.size()) khmo_cache.erase(khmo_cache.begin() + static_cast<std::ptrdiff_t>(khmo_at));
            khmo_cache.insert(khmo_cache.begin(), v);
        }
        // Re-score every vertex in the cache (positions moved), drop the overflow.
        for (size_t c = 0; c < khmo_cache.size(); ++c) {
            const uint32_t v = khmo_cache[c];
            khmo_cpos[v] = static_cast<uint32_t>(c);
            khmo_vsc[v] = kh_mco_cache_score(khmo_cpos[v]) + kh_mco_valence_score(khmo_deg[v]);
        }
        while (khmo_cache.size() > KH_MCO_CACHE) {
            const uint32_t v = khmo_cache.back();
            khmo_cache.pop_back();
            khmo_cpos[v] = KH_MCO_CACHE;
            khmo_vsc[v] = kh_mco_cache_score(KH_MCO_CACHE) + kh_mco_valence_score(khmo_deg[v]);
        }
        // Re-score the triangles touching the cache and pick the next best among them.
        khmo_best = -1; khmo_best_s = -1.0e30f;
        for (size_t c = 0; c < khmo_cache.size(); ++c) {
            const uint32_t v = khmo_cache[c];
            for (uint32_t a = khmo_off[v]; a < khmo_off[v + 1u]; ++a) {
                const uint32_t u = khmo_adj[a];
                if (khmo_done[u]) continue;
                const float s = khmo_vsc[khmo_ix[u * 3u]] + khmo_vsc[khmo_ix[u * 3u + 1u]] + khmo_vsc[khmo_ix[u * 3u + 2u]];
                khmo_tsc[u] = s;
                if (s > khmo_best_s) { khmo_best_s = s; khmo_best = u; }
            }
        }
    }
    if (khmo_out.size() != khmo_c) return;   // Defensive: leave the stream as it was.
    memcpy(khmo_ix, khmo_out.data(), khmo_c * sizeof(uint32_t));
}
inline void kh_mesh_cache_order(MeshDef& khmo_d) {
    if (khmo_d.indices.empty() || khmo_d.verts.empty()) return;
    const uint32_t khmo_vn = static_cast<uint32_t>(khmo_d.verts.size());
    const uint32_t khmo_in = static_cast<uint32_t>(khmo_d.indices.size());
    // The submesh ranges of every level, each ordered on its own.
    for (int khmo_l = 0; khmo_l <= static_cast<int>(khmo_d.lod_n); ++khmo_l) {
        const std::vector<MeshSubmesh>& khmo_tab = khmo_l == 0 ? khmo_d.submeshes : khmo_d.lod_sub[khmo_l - 1];
        for (const MeshSubmesh& khmo_sm : khmo_tab) {
            if (khmo_sm.index_count < 6u || khmo_sm.index_count % 3u != 0u) continue;
            if (khmo_sm.index_start > khmo_in || khmo_sm.index_count > khmo_in - khmo_sm.index_start) continue;
            kh_mco_range(khmo_d.indices.data() + khmo_sm.index_start, khmo_sm.index_count, khmo_vn);
        }
    }
    // First-use vertex order: the vertex fetch walks forward with the index stream.
    std::vector<uint32_t> khmo_map(khmo_vn, 0xFFFFFFFFu);
    uint32_t khmo_next = 0;
    for (uint32_t i = 0; i < khmo_in; ++i) {
        const uint32_t v = khmo_d.indices[i];
        if (v >= khmo_vn) return;   // A bad stream: the triangle order above still stands.
        if (khmo_map[v] == 0xFFFFFFFFu) khmo_map[v] = khmo_next++;
    }
    for (uint32_t v = 0; v < khmo_vn; ++v) if (khmo_map[v] == 0xFFFFFFFFu) khmo_map[v] = khmo_next++;   // Unreferenced verts last.
    std::vector<MeshVertex> khmo_nv(khmo_vn);
    for (uint32_t v = 0; v < khmo_vn; ++v) khmo_nv[khmo_map[v]] = khmo_d.verts[v];
    khmo_d.verts.swap(khmo_nv);
    for (uint32_t i = 0; i < khmo_in; ++i) khmo_d.indices[i] = khmo_map[khmo_d.indices[i]];
}

inline void kh_mesh_weld(MeshDef& khmw_d) {
    if (!khmw_d.indices.empty() || khmw_d.verts.empty()) return;
    struct KhMwKey {
        MeshVertex v;
        bool operator==(const KhMwKey& o) const { return memcmp(&v, &o.v, sizeof(MeshVertex)) == 0; }
    };
    struct KhMwHash {
        size_t operator()(const KhMwKey& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(&k.v, sizeof(MeshVertex)));
        }
    };
    std::unordered_map<KhMwKey, uint32_t, KhMwHash> khmw_map;
    khmw_map.reserve(khmw_d.verts.size() / 2 + 16);
    std::vector<MeshVertex> khmw_unique;
    khmw_unique.reserve(khmw_d.verts.size() / 2 + 16);
    khmw_d.indices.resize(khmw_d.verts.size());

    for (size_t khmw_i = 0; khmw_i < khmw_d.verts.size(); ++khmw_i) {
        KhMwKey khmw_k;
        khmw_k.v = khmw_d.verts[khmw_i];
        auto khmw_it = khmw_map.find(khmw_k);
        if (khmw_it == khmw_map.end()) {
            const uint32_t khmw_id = static_cast<uint32_t>(khmw_unique.size());
            khmw_unique.push_back(khmw_k.v);
            khmw_it = khmw_map.emplace(khmw_k, khmw_id).first;
        }
        khmw_d.indices[khmw_i] = khmw_it->second;
    }

    khmw_d.verts.swap(khmw_unique);
    khmw_d.verts.shrink_to_fit();
    kh_mesh_cache_order(khmw_d);   // KH_MESH_CACHE_ORDER: the same triangles, in cache order.
}

// KH_OCC_BOX - every mesh's occluder for KH_VIS_OCC, builtin or imported: a box
// in normalised mesh space (engine axes; a vertex p is drawn at centre +
// R (p * size)) that every level of the mesh covers from any camera outside that
// level's bounds. The proof, per level, runs over the level's closed pieces
// (triangles joined through shared position-welded edges, every edge crossed as
// often one way as the other) and leaves its open pieces (a decal, an open
// tube, a sheet) out: whatever else the object draws can only put nearer depth in front of the proof's, never take it
// away.
// With each triangle's drawn front (+g, index order: the winding meshgen::bake
// fixes against the normals, as the back-face-culled builtin box draws) as
// outward, the winding number at the box's centre is at least 1 and no closed
// triangle meets the box, so it is at least 1 all through it; a ray from a
// camera outside the bounds (winding 0) to any point of the box crosses more
// front-facing triangles than back-facing - at least one, drawn, nearer than
// the point. The box: the cube inside the ball about the seed (the normalised
// origin) that no closed piece's triangle of any level enters, grown face by
// face while the slab it gains meets none - the triangle-box test with the box
// inflated by KH_OCC_EPS, so the gap stays (far above the float rounding of the
// stored box). occ_ok false = no box: a level with no closed piece, pieces
// inside out, the seed outside their solid or on its surface. occ_hmn /
// occ_hmx bound every level's vertices, open pieces included (a decimated
// level's optimum can leave the unit box): what the camera must stay out of,
// the drawn extent the burial check reads, and - KH_OCC_TARGET - the extent
// every bounds test gives an object (kh_bounds_size_of, KH_LOD_BOUNDS); made
// before the proof, for every mesh whose level tables are sound, whatever it
// decides. Game thread: the builtins in mesh_store, and every import before
// its cache write (a cache load takes the sealed box instead).
static constexpr double KH_OCC_EPS = 1.0e-6;
static constexpr int    KH_OCC_ITERS = 30;   // Bisection steps per face: 2^-30 of its reach.
static constexpr int    KH_OCC_ROUNDS = 2;   // Passes over the six faces.
// Separating-axis test of a triangle against an axis-aligned box (centre c,
// half extents h): false only when an axis separates them.
inline bool kh_occ_tri_box(const double* c, const double* h, const double* p0, const double* p1, const double* p2) {
    const double v[3][3] = { { p0[0] - c[0], p0[1] - c[1], p0[2] - c[2] },
                             { p1[0] - c[0], p1[1] - c[1], p1[2] - c[2] },
                             { p2[0] - c[0], p2[1] - c[1], p2[2] - c[2] } };
    const double e[3][3] = { { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] },
                             { v[2][0] - v[1][0], v[2][1] - v[1][1], v[2][2] - v[1][2] },
                             { v[0][0] - v[2][0], v[0][1] - v[2][1], v[0][2] - v[2][2] } };
    auto khot_sep = [&](const double* u) -> bool {
        const double r = h[0] * fabs(u[0]) + h[1] * fabs(u[1]) + h[2] * fabs(u[2]);
        double mn = 1.0e300, mx = -1.0e300;
        for (int i = 0; i < 3; ++i) {
            const double p = v[i][0] * u[0] + v[i][1] * u[1] + v[i][2] * u[2];
            if (p < mn) mn = p;
            if (p > mx) mx = p;
        }
        return mn > r || mx < -r;
    };
    for (int k = 0; k < 3; ++k) {
        const double u[3] = { k == 0 ? 1.0 : 0.0, k == 1 ? 1.0 : 0.0, k == 2 ? 1.0 : 0.0 };
        if (khot_sep(u)) return false;
        for (int j = 0; j < 3; ++j) {   // The box axis crossed with each edge.
            const double a[3] = { u[1] * e[j][2] - u[2] * e[j][1], u[2] * e[j][0] - u[0] * e[j][2],
                                  u[0] * e[j][1] - u[1] * e[j][0] };
            if (khot_sep(a)) return false;
        }
    }
    const double n[3] = { e[0][1] * e[1][2] - e[0][2] * e[1][1], e[0][2] * e[1][0] - e[0][0] * e[1][2],
                          e[0][0] * e[1][1] - e[0][1] * e[1][0] };
    return !khot_sep(n);
}
// Squared distance from the origin to a segment and to a triangle (Ericson's
// regions; a triangle with no area falls back to its three edges).
inline double kh_occ_seg_d2(const double* a, const double* b) {
    const double d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const double l2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    double t = l2 > 0.0 ? -(a[0] * d[0] + a[1] * d[1] + a[2] * d[2]) / l2 : 0.0;
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    const double q[3] = { a[0] + t * d[0], a[1] + t * d[1], a[2] + t * d[2] };
    return q[0] * q[0] + q[1] * q[1] + q[2] * q[2];
}
inline double kh_occ_tri_d2(const double* a, const double* b, const double* c) {
    auto khod_dot = [](const double* x, const double* y) { return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]; };
    const double ab[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const double ac[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
    const double pa[3] = { -a[0], -a[1], -a[2] }, pb[3] = { -b[0], -b[1], -b[2] }, pc[3] = { -c[0], -c[1], -c[2] };
    const double d1 = khod_dot(ab, pa), d2 = khod_dot(ac, pa);
    if (d1 <= 0.0 && d2 <= 0.0) return khod_dot(a, a);
    const double d3 = khod_dot(ab, pb), d4 = khod_dot(ac, pb);
    if (d3 >= 0.0 && d4 <= d3) return khod_dot(b, b);
    const double d5 = khod_dot(ab, pc), d6 = khod_dot(ac, pc);
    if (d6 >= 0.0 && d5 <= d6) return khod_dot(c, c);
    const double vc = d1 * d4 - d3 * d2, vb = d5 * d2 - d1 * d6, va = d3 * d6 - d5 * d4;
    if (!(va + vb + vc > 0.0)) {
        const double s0 = kh_occ_seg_d2(a, b), s1 = kh_occ_seg_d2(b, c), s2 = kh_occ_seg_d2(c, a);
        return s0 < s1 ? (s0 < s2 ? s0 : s2) : (s1 < s2 ? s1 : s2);
    }
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) return kh_occ_seg_d2(a, b);
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) return kh_occ_seg_d2(a, c);
    if (va <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0) return kh_occ_seg_d2(b, c);
    const double inv = 1.0 / (va + vb + vc), v = vb * inv, w = vc * inv;
    const double q[3] = { a[0] + ab[0] * v + ac[0] * w, a[1] + ab[1] * v + ac[1] * w, a[2] + ab[2] * v + ac[2] * w };
    return khod_dot(q, q);
}
inline void kh_mesh_occ_prep(MeshDef& khop_d) {
    khop_d.occ_ok = false;
    const size_t khop_nv = khop_d.verts.size(), khop_ni = khop_d.indices.size();
    // The levels' index ranges, 0 .. lod_n (level 0 ends where level 1 begins).
    std::vector<std::pair<size_t, size_t>> khop_lv;
    khop_lv.emplace_back(static_cast<size_t>(0), khop_d.lod_n ? static_cast<size_t>(khop_d.lod_istart[0]) : khop_ni);
    for (int khop_l = 0; khop_l < static_cast<int>(khop_d.lod_n) && khop_l < KH_LOD_MAX; ++khop_l) {
        khop_lv.emplace_back(khop_d.lod_istart[khop_l], khop_d.lod_icount[khop_l]);
    }
    for (const auto& khop_r : khop_lv) {
        if (khop_r.first > khop_ni || khop_r.second > khop_ni - khop_r.first) return;
    }
    // KH_OCC_TARGET: the bounds of everything every level draws, open pieces and
    // triangles with no area included, made before the proof and kept whatever it
    // decides (-0 read as +0, as the weld below reads it). Every index is checked
    // here, so the proof's own walk over these ranges stays in range.
    double khop_mn[3] = { 1.0e300, 1.0e300, 1.0e300 }, khop_mx[3] = { -1.0e300, -1.0e300, -1.0e300 };
    for (const auto& khop_r : khop_lv) {
        for (size_t khop_t = khop_r.first; khop_t < khop_r.first + khop_r.second; ++khop_t) {
            const uint32_t khop_ix = khop_d.indices[khop_t];
            if (khop_ix >= khop_nv) return;
            for (int khop_k = 0; khop_k < 3; ++khop_k) {
                const float khop_f = khop_d.verts[khop_ix].pos[khop_k];
                if (!(khop_f > -1.0e6f && khop_f < 1.0e6f)) return;   // NaN or absurd.
                const double khop_x = static_cast<double>(khop_f + 0.0f);
                if (khop_x < khop_mn[khop_k]) khop_mn[khop_k] = khop_x;
                if (khop_x > khop_mx[khop_k]) khop_mx[khop_k] = khop_x;
            }
        }
    }
    if (!(khop_mn[0] <= khop_mx[0] && khop_mn[1] <= khop_mx[1] && khop_mn[2] <= khop_mx[2])) return;
    for (int khop_k = 0; khop_k < 3; ++khop_k) {
        khop_d.occ_hmn[khop_k] = static_cast<float>(khop_mn[khop_k]);   // Exact: float positions.
        khop_d.occ_hmx[khop_k] = static_cast<float>(khop_mx[khop_k]);
    }
    if (khop_nv < 4u || khop_ni < 12u || khop_ni % 3u != 0u) return;
    for (const auto& khop_r : khop_lv) {
        if (khop_r.second < 12u || khop_r.second % 3u != 0u) return;
    }
    // Positions welded by their exact bits (-0 read as +0): an edge two
    // triangles share is two positions they share, whatever else differs.
    struct KhOpKey {
        float p[3];
        bool operator==(const KhOpKey& o) const { return memcmp(p, o.p, sizeof(p)) == 0; }
    };
    struct KhOpHash {
        size_t operator()(const KhOpKey& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(k.p, sizeof(k.p)));
        }
    };
    std::unordered_map<KhOpKey, uint32_t, KhOpHash> khop_map;
    khop_map.reserve(khop_nv);
    std::vector<uint32_t> khop_pid(khop_nv);
    std::vector<double> khop_p;   // 3 per position.
    khop_p.reserve(khop_nv * 3u);
    for (size_t khop_v = 0; khop_v < khop_nv; ++khop_v) {
        KhOpKey khop_k;
        for (int khop_c = 0; khop_c < 3; ++khop_c) {
            const float khop_x = khop_d.verts[khop_v].pos[khop_c];
            if (!(khop_x > -1.0e6f && khop_x < 1.0e6f)) return;   // NaN or absurd.
            khop_k.p[khop_c] = khop_x + 0.0f;
        }
        auto khop_it = khop_map.find(khop_k);
        if (khop_it == khop_map.end()) {
            khop_it = khop_map.emplace(khop_k, static_cast<uint32_t>(khop_p.size() / 3u)).first;
            for (int khop_c = 0; khop_c < 3; ++khop_c) khop_p.push_back(khop_k.p[khop_c]);
        }
        khop_pid[khop_v] = khop_it->second;
    }
    // Every level: its closed pieces' triangles with area kept.
    std::vector<uint32_t> khop_tri;   // Position ids, 3 per triangle, level after level.
    std::vector<size_t> khop_lt;      // Where each level's triangles begin in khop_tri, then the end.
    struct KhOpEdge {
        int32_t bal;    // Crossings one way minus the other.
        uint32_t tri;   // A triangle using it: the edge's piece.
    };
    std::unordered_map<uint64_t, KhOpEdge> khop_edge;
    std::vector<uint32_t> khop_lvt;   // This level's triangles with area.
    std::vector<uint32_t> khop_par;   // Union-find over them: the pieces.
    std::vector<uint8_t> khop_open;   // Per piece root: an edge of it does not balance.
    for (const auto& khop_r : khop_lv) {
        khop_lt.push_back(khop_tri.size());
        khop_edge.clear();
        khop_lvt.clear();
        for (size_t khop_t = khop_r.first; khop_t < khop_r.first + khop_r.second; khop_t += 3u) {
            uint32_t khop_q[3];
            for (int khop_c = 0; khop_c < 3; ++khop_c) {
                khop_q[khop_c] = khop_pid[khop_d.indices[khop_t + static_cast<size_t>(khop_c)]];
            }
            if (khop_q[0] == khop_q[1] || khop_q[1] == khop_q[2] || khop_q[0] == khop_q[2]) continue;   // No area.
            khop_lvt.insert(khop_lvt.end(), khop_q, khop_q + 3);
        }
        const uint32_t khop_ln = static_cast<uint32_t>(khop_lvt.size() / 3u);
        khop_par.resize(khop_ln);
        for (uint32_t khop_i = 0; khop_i < khop_ln; ++khop_i) khop_par[khop_i] = khop_i;
        auto khop_find = [&](uint32_t khop_x) -> uint32_t {
            while (khop_par[khop_x] != khop_x) {
                khop_par[khop_x] = khop_par[khop_par[khop_x]];
                khop_x = khop_par[khop_x];
            }
            return khop_x;
        };
        for (uint32_t khop_i = 0; khop_i < khop_ln; ++khop_i) {
            for (int khop_c = 0; khop_c < 3; ++khop_c) {
                const uint32_t khop_a = khop_lvt[khop_i * 3u + static_cast<uint32_t>(khop_c)];
                const uint32_t khop_b = khop_lvt[khop_i * 3u + static_cast<uint32_t>((khop_c + 1) % 3)];
                const uint64_t khop_key = khop_a < khop_b ? (static_cast<uint64_t>(khop_b) << 32) | khop_a
                                                          : (static_cast<uint64_t>(khop_a) << 32) | khop_b;
                const int32_t khop_dir = khop_a < khop_b ? 1 : -1;
                auto khop_it = khop_edge.find(khop_key);
                if (khop_it == khop_edge.end()) {
                    khop_edge.emplace(khop_key, KhOpEdge{ khop_dir, khop_i });
                } else {
                    khop_it->second.bal += khop_dir;
                    const uint32_t khop_ra = khop_find(khop_i), khop_rb = khop_find(khop_it->second.tri);
                    if (khop_ra != khop_rb) khop_par[khop_ra] = khop_rb;
                }
            }
        }
        // A piece is open when any of its edges does not balance (a hole, a
        // crack, a sheet); each edge belongs to exactly one piece.
        khop_open.assign(khop_ln, 0u);
        for (const auto& khop_kv : khop_edge) {
            if (khop_kv.second.bal != 0) khop_open[khop_find(khop_kv.second.tri)] = 1u;
        }
        for (uint32_t khop_i = 0; khop_i < khop_ln; ++khop_i) {
            if (khop_open[khop_find(khop_i)]) continue;
            khop_tri.insert(khop_tri.end(), khop_lvt.begin() + khop_i * 3u, khop_lvt.begin() + khop_i * 3u + 3u);
        }
        if (khop_tri.size() == khop_lt.back()) return;   // No closed piece at this level.
    }
    khop_lt.push_back(khop_tri.size());
    const size_t khop_nt = khop_tri.size() / 3u;
    if (khop_nt < 4u) return;
    auto khop_pt = [&](size_t khop_t, int khop_c) -> const double* {
        return &khop_p[static_cast<size_t>(khop_tri[khop_t * 3u + static_cast<size_t>(khop_c)]) * 3u];
    };
    // The seed inside every level's closed pieces (winding at least 1, +g
    // outward) and clear of every one of their triangles.
    for (size_t khop_l = 0; khop_l + 1u < khop_lt.size(); ++khop_l) {
        double khop_w = 0.0;
        for (size_t khop_t = khop_lt[khop_l] / 3u; khop_t < khop_lt[khop_l + 1u] / 3u; ++khop_t) {
            const double* a = khop_pt(khop_t, 0);
            const double* b = khop_pt(khop_t, 1);
            const double* c = khop_pt(khop_t, 2);
            const double la = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
            const double lb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
            const double lc = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
            if (!(la > 0.0 && lb > 0.0 && lc > 0.0)) return;   // The seed on a vertex.
            const double khop_det = a[0] * (b[1] * c[2] - b[2] * c[1]) + a[1] * (b[2] * c[0] - b[0] * c[2]) +
                                    a[2] * (b[0] * c[1] - b[1] * c[0]);
            const double khop_den = la * lb * lc + (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) * lc +
                                    (a[0] * c[0] + a[1] * c[1] + a[2] * c[2]) * lb +
                                    (b[0] * c[0] + b[1] * c[1] + b[2] * c[2]) * la;
            khop_w += 2.0 * atan2(khop_det, khop_den);   // The solid angle (Van Oosterom - Strackee).
        }
        khop_w /= 4.0 * 3.14159265358979323846;
        const double khop_wr = floor(khop_w + 0.5);
        if (!(khop_wr >= 1.0) || !(fabs(khop_w - khop_wr) < 0.25)) return;   // Outside the solid, or inside out.
    }
    double khop_d2 = 1.0e300;
    for (size_t khop_t = 0; khop_t < khop_nt; ++khop_t) {
        const double khop_e = kh_occ_tri_d2(khop_pt(khop_t, 0), khop_pt(khop_t, 1), khop_pt(khop_t, 2));
        if (khop_e < khop_d2) khop_d2 = khop_e;
    }
    const double khop_r0 = (sqrt(khop_d2) - KH_OCC_EPS) / sqrt(3.0);   // Its corners KH_OCC_EPS inside the ball.
    if (!(khop_r0 > KH_OCC_EPS)) return;
    double khop_lo[3] = { -khop_r0, -khop_r0, -khop_r0 }, khop_hi[3] = { khop_r0, khop_r0, khop_r0 };
    // A uniform grid of the closed pieces' triangles over the bounds, for the
    // slab queries.
    int khop_g = static_cast<int>(cbrt(static_cast<double>(khop_nt) * 0.5));
    khop_g = khop_g < 1 ? 1 : (khop_g > 64 ? 64 : khop_g);
    double khop_inv[3];
    for (int khop_k = 0; khop_k < 3; ++khop_k) {
        const double khop_ext = khop_mx[khop_k] - khop_mn[khop_k];
        khop_inv[khop_k] = khop_ext > 0.0 ? static_cast<double>(khop_g) / khop_ext : 0.0;
    }
    auto khop_cell = [&](double khop_x, int khop_k) -> int {
        const double khop_f = floor((khop_x - khop_mn[khop_k]) * khop_inv[khop_k]);
        return khop_f < 0.0 ? 0 : (khop_f > static_cast<double>(khop_g - 1) ? khop_g - 1 : static_cast<int>(khop_f));
    };
    const size_t khop_gn = static_cast<size_t>(khop_g) * static_cast<size_t>(khop_g) * static_cast<size_t>(khop_g);
    std::vector<uint32_t> khop_cs(khop_gn + 1u, 0u), khop_cl;
    for (int khop_pass = 0; khop_pass < 2; ++khop_pass) {   // Count, then fill (CSR).
        if (khop_pass == 1) {
            for (size_t khop_i = 1; khop_i <= khop_gn; ++khop_i) khop_cs[khop_i] += khop_cs[khop_i - 1u];
            khop_cl.assign(khop_cs[khop_gn], 0u);
        }
        std::vector<uint32_t> khop_fill;
        if (khop_pass == 1) khop_fill.assign(khop_cs.begin(), khop_cs.end() - 1);
        for (size_t khop_t = 0; khop_t < khop_nt; ++khop_t) {
            int khop_i0[3], khop_i1[3];
            for (int khop_k = 0; khop_k < 3; ++khop_k) {
                double khop_a = khop_pt(khop_t, 0)[khop_k], khop_b = khop_a;
                for (int khop_c = 1; khop_c < 3; ++khop_c) {
                    const double khop_x = khop_pt(khop_t, khop_c)[khop_k];
                    if (khop_x < khop_a) khop_a = khop_x;
                    if (khop_x > khop_b) khop_b = khop_x;
                }
                khop_i0[khop_k] = khop_cell(khop_a - KH_OCC_EPS, khop_k);
                khop_i1[khop_k] = khop_cell(khop_b + KH_OCC_EPS, khop_k);
            }
            for (int khop_z = khop_i0[2]; khop_z <= khop_i1[2]; ++khop_z)
                for (int khop_y = khop_i0[1]; khop_y <= khop_i1[1]; ++khop_y)
                    for (int khop_x = khop_i0[0]; khop_x <= khop_i1[0]; ++khop_x) {
                        const size_t khop_ci = (static_cast<size_t>(khop_z) * khop_g + khop_y) * khop_g + khop_x;
                        if (khop_pass == 0) ++khop_cs[khop_ci + 1u];
                        else khop_cl[khop_fill[khop_ci]++] = static_cast<uint32_t>(khop_t);
                    }
        }
    }
    std::vector<uint32_t> khop_seen(khop_nt, 0u);
    uint32_t khop_stamp = 0u;
    // Whether any closed piece's triangle meets the box [bmn, bmx] inflated by
    // KH_OCC_EPS.
    auto khop_hits = [&](const double* khop_bmn, const double* khop_bmx) -> bool {
        if (++khop_stamp == 0u) { std::fill(khop_seen.begin(), khop_seen.end(), 0u); khop_stamp = 1u; }
        double khop_c[3], khop_h[3];
        int khop_i0[3], khop_i1[3];
        for (int khop_k = 0; khop_k < 3; ++khop_k) {
            khop_c[khop_k] = 0.5 * (khop_bmn[khop_k] + khop_bmx[khop_k]);
            khop_h[khop_k] = 0.5 * (khop_bmx[khop_k] - khop_bmn[khop_k]) + KH_OCC_EPS;
            khop_i0[khop_k] = khop_cell(khop_bmn[khop_k] - KH_OCC_EPS, khop_k);
            khop_i1[khop_k] = khop_cell(khop_bmx[khop_k] + KH_OCC_EPS, khop_k);
        }
        for (int khop_z = khop_i0[2]; khop_z <= khop_i1[2]; ++khop_z)
            for (int khop_y = khop_i0[1]; khop_y <= khop_i1[1]; ++khop_y)
                for (int khop_x = khop_i0[0]; khop_x <= khop_i1[0]; ++khop_x) {
                    const size_t khop_ci = (static_cast<size_t>(khop_z) * khop_g + khop_y) * khop_g + khop_x;
                    for (uint32_t khop_j = khop_cs[khop_ci]; khop_j < khop_cs[khop_ci + 1u]; ++khop_j) {
                        const uint32_t khop_t = khop_cl[khop_j];
                        if (khop_seen[khop_t] == khop_stamp) continue;
                        khop_seen[khop_t] = khop_stamp;
                        if (kh_occ_tri_box(khop_c, khop_h, khop_pt(khop_t, 0), khop_pt(khop_t, 1),
                                           khop_pt(khop_t, 2))) return true;
                    }
                }
        return false;
    };
    // Growth: each face in turn, as far as a slab of the growth stays clear.
    // The solid ends inside the bounds, so a face never passes them.
    for (int khop_rd = 0; khop_rd < KH_OCC_ROUNDS; ++khop_rd) {
        for (int khop_f = 0; khop_f < 6; ++khop_f) {
            const int khop_ax = khop_f >> 1;
            const bool khop_up = (khop_f & 1) == 0;
            double khop_a = khop_up ? khop_hi[khop_ax] : khop_lo[khop_ax];   // Clear so far.
            double khop_b = khop_up ? khop_mx[khop_ax] : khop_mn[khop_ax];
            if (khop_up ? !(khop_b > khop_a) : !(khop_b < khop_a)) continue;
            for (int khop_it = 0; khop_it < KH_OCC_ITERS; ++khop_it) {
                const double khop_m = 0.5 * (khop_a + khop_b);
                double khop_smn[3] = { khop_lo[0], khop_lo[1], khop_lo[2] };
                double khop_smx[3] = { khop_hi[0], khop_hi[1], khop_hi[2] };
                if (khop_up) { khop_smn[khop_ax] = khop_a; khop_smx[khop_ax] = khop_m; }
                else         { khop_smn[khop_ax] = khop_m; khop_smx[khop_ax] = khop_a; }
                if (khop_hits(khop_smn, khop_smx)) khop_b = khop_m; else khop_a = khop_m;
            }
            if (khop_up) khop_hi[khop_ax] = khop_a; else khop_lo[khop_ax] = khop_a;
        }
    }
    for (int khop_k = 0; khop_k < 3; ++khop_k) {
        khop_d.occ_c[khop_k] = static_cast<float>(0.5 * (khop_lo[khop_k] + khop_hi[khop_k]));
        khop_d.occ_h[khop_k] = static_cast<float>(0.5 * (khop_hi[khop_k] - khop_lo[khop_k]));
    }
    khop_d.occ_ok = true;
}
// KH_OCC_BOX - the mesh cache's box chunk (KH_OCC_CACHE_MAGIC): the verdict, the
// box and the bounds as a 12-float record (occ_c, occ_h, occ_hmn, occ_hmx), and
// a seal - FNV-1a 64 over the verdict, the record and everything the proof read
// (positions, indices, the level tables) - so a cached box is taken only for
// exactly the triangles it was proven on. Any other byte: a cache miss, and
// the mesh imports again.
inline void kh_mesh_occ_record(const MeshDef& khor_d, float khor_r[12]) {
    for (int k = 0; k < 3; ++k) {
        khor_r[k] = khor_d.occ_c[k];
        khor_r[3 + k] = khor_d.occ_h[k];
        khor_r[6 + k] = khor_d.occ_hmn[k];
        khor_r[9 + k] = khor_d.occ_hmx[k];
    }
}
inline uint64_t kh_mesh_occ_seal(const MeshDef& khos_d, uint32_t khos_ok, const float khos_r[12]) {
    uint64_t khos_h = CryptoGenerator::FNV1A64_OFFSET;
    khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(&khos_ok), 4);
    khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(khos_r), 48);
    for (const MeshVertex& khos_v : khos_d.verts) {
        khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(khos_v.pos), 12);
    }
    if (!khos_d.indices.empty()) {
        khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(khos_d.indices.data()),
                                                 khos_d.indices.size() * sizeof(uint32_t));
    }
    const uint32_t khos_ln = khos_d.lod_n;
    khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(&khos_ln), 4);
    if (khos_ln) {
        khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(khos_d.lod_istart),
                                                 khos_ln * 4u);
        khos_h = CryptoGenerator::fnv1a64_update(khos_h, reinterpret_cast<const char*>(khos_d.lod_icount),
                                                 khos_ln * 4u);
    }
    return khos_h;
}
// A read chunk, taken when its seal matches this geometry and the record is
// sane (finite; a box inside its bounds). False = a cache miss.
inline bool kh_mesh_occ_accept(MeshDef& khoa_d, uint32_t khoa_ok, const float khoa_r[12], uint64_t khoa_seal) {
    if (khoa_ok > 1u) return false;
    for (int k = 0; k < 12; ++k) if (!(khoa_r[k] > -1.0e6f && khoa_r[k] < 1.0e6f)) return false;
    if (khoa_ok) {
        for (int k = 0; k < 3; ++k) {
            if (!(khoa_r[3 + k] > 0.0f) || khoa_r[k] - khoa_r[3 + k] < khoa_r[6 + k] ||
                khoa_r[k] + khoa_r[3 + k] > khoa_r[9 + k]) return false;
        }
    }
    if (kh_mesh_occ_seal(khoa_d, khoa_ok, khoa_r) != khoa_seal) return false;
    for (int k = 0; k < 3; ++k) {
        khoa_d.occ_c[k] = khoa_r[k];
        khoa_d.occ_h[k] = khoa_r[3 + k];
        khoa_d.occ_hmn[k] = khoa_r[6 + k];
        khoa_d.occ_hmx[k] = khoa_r[9 + k];
    }
    khoa_d.occ_ok = khoa_ok != 0u;
    return true;
}
}