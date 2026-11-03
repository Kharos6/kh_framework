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
    float t[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const float tl = sqrtf(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);

    if (tl > 1e-8f) {
        t[0] /= tl; t[1] /= tl; t[2] /= tl;
    } else {
        t[0] = 1.0f; t[1] = 0.0f; t[2] = 0.0f;
    }

    const float bx[3] = {
        n[1] * t[2] - n[2] * t[1],
        n[2] * t[0] - n[0] * t[2],
        n[0] * t[1] - n[1] * t[0],
    };
    const float w = (bx[0] * (d[0] - a[0]) + bx[1] * (d[1] - a[1]) +
                     bx[2] * (d[2] - a[2])) >= 0.0f ? 1.0f : -1.0f;

    for (int i = 0; i < 6; ++i) {
        MeshVertex mv = {};
        memcpy(mv.pos, corners[i], sizeof(mv.pos));
        memcpy(mv.nrm, n, sizeof(mv.nrm));
        mv.uv[0] = uvs[uvi[i]][0];
        mv.uv[1] = uvs[uvi[i]][1];
        memcpy(mv.tan, t, sizeof(t));
        mv.tan[3] = w;
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
    const float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const float e2[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
    const float du1 = ub[0] - ua[0], dv1 = ub[1] - ua[1];
    const float du2 = uc[0] - ua[0], dv2 = uc[1] - ua[1];
    const float r = du1 * dv2 - du2 * dv1;
    float t[3];
    float w = 1.0f;

    if (r > 1e-10f || r < -1e-10f) {
        const float ir = 1.0f / r;
        t[0] = (e1[0] * dv2 - e2[0] * dv1) * ir;
        t[1] = (e1[1] * dv2 - e2[1] * dv1) * ir;
        t[2] = (e1[2] * dv2 - e2[2] * dv1) * ir;
        const float bitan[3] = {
            (e2[0] * du1 - e1[0] * du2) * ir,
            (e2[1] * du1 - e1[1] * du2) * ir,
            (e2[2] * du1 - e1[2] * du2) * ir,
        };
        const float cx[3] = {
            na[1] * t[2] - na[2] * t[1],
            na[2] * t[0] - na[0] * t[2],
            na[0] * t[1] - na[1] * t[0],
        };
        w = (cx[0] * bitan[0] + cx[1] * bitan[1] + cx[2] * bitan[2]) >= 0.0f ? 1.0f : -1.0f;
    } else {
        // uv-degenerate: any perpendicular to the first normal.
        if (na[0] * na[0] < 0.9f) { t[0] = 0.0f; t[1] = -na[2]; t[2] = na[1]; }
        else                      { t[0] = -na[2]; t[1] = 0.0f; t[2] = na[0]; }
    }

    const float l = sqrtf(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);

    if (l > 1e-8f) {
        t[0] /= l; t[1] /= l; t[2] /= l;
    } else {
        t[0] = 1.0f; t[1] = 0.0f; t[2] = 0.0f;
    }

    for (int i = 0; i < 3; ++i) {
        MeshVertex mv = {};
        memcpy(mv.pos, p[i], sizeof(mv.pos));
        memcpy(mv.nrm, n[i], sizeof(mv.nrm));
        mv.uv[0] = u[i][0];
        mv.uv[1] = u[i][1];
        memcpy(mv.tan, t, sizeof(t));
        mv.tan[3] = w;
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
    const float pi = 3.14159265358979f;

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
                // OCC_BOX: watertight - the seam's closing column takes the first
                // column's angle and a pole is exact (float 2 pi and pi / 2 are not), so
                // every shared corner is one position bit for bit, but for the sign of a
                // zero (a pole's 0 * cos / 0 * sin gives -0.0f on some slices; OCC_BOX's weld adds 0.0f, which folds
                // it to +0.0f).
                const int s = st + ci[k][1];
                const int w = (sl + ci[k][0]) % slices;
                const float la = (static_cast<float>(s) / static_cast<float>(stacks) - 0.5f) * pi;
                const float lo = static_cast<float>(w) / static_cast<float>(slices) * 2.0f * pi;
                const float cl = (s == 0 || s == stacks) ? 0.0f : cosf(la);
                const float sa = s == 0 ? -1.0f : (s == stacks ? 1.0f : sinf(la));
                n[k][0] = cl * cosf(lo);
                n[k][1] = cl * sinf(lo);
                n[k][2] = sa;
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
    const float pi = 3.14159265358979f;
    const float top[3] = { 0.0f, 0.0f,  0.5f }, bot[3] = { 0.0f, 0.0f, -0.5f };
    const float nzp[3] = { 0.0f, 0.0f,  1.0f }, nzn[3] = { 0.0f, 0.0f, -1.0f };
    const float ucc[2] = { 0.5f, 0.5f };

    for (int sl = 0; sl < slices; ++sl) {
        const float s0 = static_cast<float>(sl) / static_cast<float>(slices);
        const float s1 = static_cast<float>(sl + 1) / static_cast<float>(slices);
        // OCC_BOX: the closing slice ends on the first one's angle exactly
        // (float 2 pi does not), so the seam's corners are one position.
        const float s1w = static_cast<float>((sl + 1) % slices) / static_cast<float>(slices);
        const float a0 = s0 * 2.0f * pi, a1 = s1w * 2.0f * pi;
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
    const float pi = 3.14159265358979f;
    const float apex[3] = { 0.0f, 0.0f, 0.5f }, bot[3] = { 0.0f, 0.0f, -0.5f };
    const float nzn[3] = { 0.0f, 0.0f, -1.0f };
    const float ucc[2] = { 0.5f, 0.5f };
    const float l = sqrtf(1.25f);

    for (int sl = 0; sl < slices; ++sl) {
        const float s0 = static_cast<float>(sl) / static_cast<float>(slices);
        const float s1 = static_cast<float>(sl + 1) / static_cast<float>(slices);
        const float a0 = s0 * 2.0f * pi, a1 = s1 * 2.0f * pi;
        const float am = 0.5f * (a0 + a1);
        // OCC_BOX: the closing slice's corner on the first one's angle exactly.
        const float a1w = static_cast<float>((sl + 1) % slices) / static_cast<float>(slices) * 2.0f * pi;
        const float p0[3] = { cosf(a0) * 0.5f, sinf(a0) * 0.5f, -0.5f };
        const float p1[3] = { cosf(a1w) * 0.5f, sinf(a1w) * 0.5f, -0.5f };
        const float n0[3] = { cosf(a0) / l, sinf(a0) / l, 0.5f / l };
        const float n1[3] = { cosf(a1w) / l, sinf(a1w) / l, 0.5f / l };
        const float na[3] = { cosf(am) / l, sinf(am) / l, 0.5f / l };
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
    const float l = sqrtf(1.25f);
    const float ns[4][3] = { {  0.0f,          -1.0f / l, 0.5f / l },
                             {  1.0f / l,  0.0f,          0.5f / l },
                             {  0.0f,           1.0f / l, 0.5f / l },
                             { -1.0f / l,  0.0f,          0.5f / l } };
    const float u0[2] = { 0.0f, 0.0f }, u1[2] = { 1.0f, 0.0f }, ua[2] = { 0.5f, 1.0f };

    for (int f = 0; f < 4; ++f) {
        tri(v, b[f], ns[f], u0, b[(f + 1) & 3], ns[f], u1, apex, ns[f], ua);
    }
}

// Bake an arma-axes mesh into the engine layout: arma [x, y, z] (east, north,
// up) -> engine [x, z, y] (the swap every SQF position/size fill performs),
// then reorder each triangle so cross(b-a, c-a) points along its stored normal
// (the swap flips handedness).
// ext: the per-axis extents the positions were squashed by, in the ENGINE
// frame (the import's d.native_ext), or null for a mesh authored in the unit
// box (the builtins). The winding test is weighted by 1 / ext^2: squashing by
// diag(1 / ext) turns the geometric cross into diag(ext) . n_geom and
// IMPORT_NRM stores diag(ext) . n, so the plain dot is weighted by ext^2 and
// a thin mesh's smoothed normals decide its sign by their tiny in-plane parts;
// the weighting recovers sign(n_geom . n), the authored winding, never a
// back-facing hole. Null weights are exactly 1.0.
inline void bake(std::vector<MeshVertex>& v, const float* ext = nullptr) {
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
        float w[3] = { 1.0f, 1.0f, 1.0f };
        if (ext) {
            for (int k = 0; k < 3; ++k) {
                const float e = fabsf(ext[k]) > 1.0e-6f ? ext[k] : 1.0f;
                w[k] = 1.0f / (e * e);
            }
        }

        if (g[0] * n[0] * w[0] + g[1] * n[1] * w[1] + g[2] * n[2] * w[2] < 0.0f) {
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
// the mesh cache (MESH_CACHE_VERSION).
static constexpr uint32_t MCO_CACHE   = 32u;   // Modelled LRU cache depth.
inline float mco_cache_score(uint32_t p) {
    if (p >= MCO_CACHE) return 0.0f;
    if (p < 3u) return 0.75f;   // The last triangle's own vertices: a flat score, so
                                     // a strip does not chase itself.
    const float s = 1.0f - (static_cast<float>(p) - 3.0f) / (static_cast<float>(MCO_CACHE) - 3.0f);
    return powf(s, 1.5f);
}
inline float mco_valence_score(uint32_t n) {
    if (n == 0u) return -1.0f;
    return 2.0f * powf(static_cast<float>(n), -0.5f);
}
// One index range: the mo_c indices at ix (a multiple of 3).
inline void mco_range(uint32_t* ix, uint32_t mo_c, uint32_t vn) {
    const uint32_t tn = mo_c / 3u;
    if (tn < 2u || vn == 0u) return;
    for (uint32_t i = 0; i < mo_c; ++i) if (ix[i] >= vn) return;   // Never touch a bad stream.
    std::vector<uint32_t> deg(vn, 0u);          // Remaining triangles per vertex.
    for (uint32_t i = 0; i < mo_c; ++i) ++deg[ix[i]];
    std::vector<uint32_t> off(vn + 1u, 0u);     // Vertex -> its triangle list.
    for (uint32_t v = 0; v < vn; ++v) off[v + 1u] = off[v] + deg[v];
    std::vector<uint32_t> adj(mo_c);
    {
        std::vector<uint32_t> fill(off.begin(), off.end() - 1);
        for (uint32_t t = 0; t < tn; ++t)
            for (uint32_t k = 0; k < 3u; ++k) adj[fill[ix[t * 3u + k]]++] = t;
    }
    std::vector<uint32_t> cpos(vn, MCO_CACHE);   // Cache position (>= CACHE = out).
    std::vector<float>    vsc(vn, 0.0f);
    for (uint32_t v = 0; v < vn; ++v)
        vsc[v] = mco_cache_score(MCO_CACHE) + mco_valence_score(deg[v]);
    std::vector<float>   tsc(tn, 0.0f);
    std::vector<uint8_t> done(tn, 0u);
    for (uint32_t t = 0; t < tn; ++t)
        tsc[t] = vsc[ix[t * 3u]] + vsc[ix[t * 3u + 1u]] + vsc[ix[t * 3u + 2u]];
    // MCO_TREE - the fallback's pick without a scan of every triangle left: a max tree over the triangles' scores,
    // a done one at -inf, a tie to the lower index. The fallback runs only when no triangle left touches the cache, so
    // each vertex of one left is out of it and its vsc is the out-of-cache score it took on leaving (or at the
    // start), unchanged since: only an emit moves a valence, and an emit puts the vertex in the cache. An emit marks
    // its vertices stale; the fallback first re-scores the stale vertices' triangles from vsc with the scan's own
    // sum, so every leaf holds what the scan would compute and the pick is the scan's - the first of the best. One
    // scan per exhausted region was quadratic in a mesh of many regions (a flat-shaded one: every face is one); a pick
    // is now O(log n), and the stale triangles' refresh O(log n) each.
    uint32_t tp = 1u;
    while (tp < tn) tp <<= 1;
    const float inf = std::numeric_limits<float>::infinity();
    std::vector<float> tv(2u * static_cast<size_t>(tp), -inf);
    std::vector<uint32_t> ti(2u * static_cast<size_t>(tp), 0u);
    auto tfix = [&](uint32_t nd) {   // The nodes above a leaf, again.
        for (nd >>= 1; nd >= 1u; nd >>= 1) {
            const uint32_t l = 2u * nd;
            const bool lw = tv[l] >= tv[l + 1u];
            tv[nd] = lw ? tv[l] : tv[l + 1u];
            ti[nd] = lw ? ti[l] : ti[l + 1u];
        }
    };
    for (uint32_t t = 0; t < tn; ++t) { tv[tp + t] = tsc[t]; ti[tp + t] = t; }
    for (uint32_t nd = tp - 1u; nd >= 1u; --nd) {
        const uint32_t l = 2u * nd;
        const bool lw = tv[l] >= tv[l + 1u];
        tv[nd] = lw ? tv[l] : tv[l + 1u];
        ti[nd] = lw ? ti[l] : ti[l + 1u];
    }
    std::vector<uint8_t>  stale(vn, 0u);
    std::vector<uint32_t> stl;   // The stale vertices, each once.
    std::vector<uint32_t> out;
    out.reserve(mo_c);
    std::vector<uint32_t> cache;   // Most recent first.
    cache.reserve(MCO_CACHE + 3u);
    int64_t  best = -1;
    float    best_s = -1.0e30f;
    for (uint32_t t = 0; t < tn; ++t) if (tsc[t] > best_s) { best_s = tsc[t]; best = t; }
    for (uint32_t emit = 0; emit < tn; ++emit) {
        if (best < 0) {   // Nothing in the cache neighbourhood: the best-scoring triangle left (MCO_TREE).
            for (uint32_t mo_v : stl) {
                stale[mo_v] = 0u;
                for (uint32_t mo_a = off[mo_v]; mo_a < off[mo_v + 1u]; ++mo_a) {
                    const uint32_t u = adj[mo_a];
                    tv[tp + u] = done[u] ? -inf
                               : vsc[ix[u * 3u]] + vsc[ix[u * 3u + 1u]] +
                                 vsc[ix[u * 3u + 2u]];
                    tfix(tp + u);
                }
            }
            stl.clear();
            if (!(tv[1] > -inf)) break;   // None left.
            best = ti[1];
        }
        const uint32_t t = static_cast<uint32_t>(best);
        done[t] = 1u;
        for (uint32_t k = 0; k < 3u; ++k) {
            const uint32_t v = ix[t * 3u + k];
            out.push_back(v);
            --deg[v];
            if (!stale[v]) { stale[v] = 1u; stl.push_back(v); }   // MCO_TREE.
            // Move v to the cache front (an LRU list of positions, the model of the paper).
            size_t at = cache.size();
            for (size_t c = 0; c < cache.size(); ++c) if (cache[c] == v) { at = c; break; }
            if (at < cache.size()) cache.erase(cache.begin() + static_cast<std::ptrdiff_t>(at));
            cache.insert(cache.begin(), v);
        }
        // Re-score every vertex in the cache (positions moved), drop the overflow.
        for (size_t c = 0; c < cache.size(); ++c) {
            const uint32_t v = cache[c];
            cpos[v] = static_cast<uint32_t>(c);
            vsc[v] = mco_cache_score(cpos[v]) + mco_valence_score(deg[v]);
        }
        while (cache.size() > MCO_CACHE) {
            const uint32_t v = cache.back();
            cache.pop_back();
            cpos[v] = MCO_CACHE;
            vsc[v] = mco_cache_score(MCO_CACHE) + mco_valence_score(deg[v]);
        }
        // Re-score the triangles touching the cache and pick the next best among them.
        best = -1; best_s = -1.0e30f;
        for (size_t c = 0; c < cache.size(); ++c) {
            const uint32_t v = cache[c];
            for (uint32_t a = off[v]; a < off[v + 1u]; ++a) {
                const uint32_t u = adj[a];
                if (done[u]) continue;
                const float s = vsc[ix[u * 3u]] + vsc[ix[u * 3u + 1u]] + vsc[ix[u * 3u + 2u]];
                tsc[u] = s;
                if (s > best_s) { best_s = s; best = u; }
            }
        }
    }
    if (out.size() != mo_c) return;   // Defensive: leave the stream as it was.
    memcpy(ix, out.data(), mo_c * sizeof(uint32_t));
}
inline void mesh_cache_order(MeshDef& d) {
    if (d.indices.empty() || d.verts.empty()) return;
    const uint32_t vn = static_cast<uint32_t>(d.verts.size());
    const uint32_t in = static_cast<uint32_t>(d.indices.size());
    // The submesh ranges of every level, each ordered on its own.
    for (int l = 0; l <= static_cast<int>(d.lod_n); ++l) {
        const std::vector<MeshSubmesh>& tab = l == 0 ? d.submeshes : d.lod_sub[l - 1];
        for (const MeshSubmesh& sm : tab) {
            if (sm.index_count < 6u || sm.index_count % 3u != 0u) continue;
            if (sm.index_start > in || sm.index_count > in - sm.index_start) continue;
            mco_range(d.indices.data() + sm.index_start, sm.index_count, vn);
        }
    }
    // First-use vertex order: the vertex fetch walks forward with the index stream.
    std::vector<uint32_t> map(vn, 0xFFFFFFFFu);
    uint32_t next = 0;
    for (uint32_t i = 0; i < in; ++i) {
        const uint32_t v = d.indices[i];
        if (v >= vn) return;   // A bad stream: the triangle order above still stands.
        if (map[v] == 0xFFFFFFFFu) map[v] = next++;
    }
    for (uint32_t v = 0; v < vn; ++v) if (map[v] == 0xFFFFFFFFu) map[v] = next++;   // Unreferenced verts last.
    std::vector<MeshVertex> nv(vn);
    for (uint32_t v = 0; v < vn; ++v) nv[map[v]] = d.verts[v];
    d.verts.swap(nv);
    for (uint32_t i = 0; i < in; ++i) d.indices[i] = map[d.indices[i]];
}

inline void mesh_weld(MeshDef& d) {
    if (!d.indices.empty() || d.verts.empty()) return;
    struct MwKey {
        MeshVertex v;
        bool operator==(const MwKey& o) const { return memcmp(&v, &o.v, sizeof(MeshVertex)) == 0; }
    };
    struct MwHash {
        size_t operator()(const MwKey& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(&k.v, sizeof(MeshVertex)));
        }
    };
    std::unordered_map<MwKey, uint32_t, MwHash> map;
    map.reserve(d.verts.size() / 2 + 16);
    std::vector<MeshVertex> unique;
    unique.reserve(d.verts.size() / 2 + 16);
    d.indices.resize(d.verts.size());

    for (size_t i = 0; i < d.verts.size(); ++i) {
        MwKey mw_k;
        mw_k.v = d.verts[i];
        auto it = map.find(mw_k);
        if (it == map.end()) {
            const uint32_t id = static_cast<uint32_t>(unique.size());
            unique.push_back(mw_k.v);
            it = map.emplace(mw_k, id).first;
        }
        d.indices[i] = it->second;
    }

    d.verts.swap(unique);
    d.verts.shrink_to_fit();
    mesh_cache_order(d);   // MESH_CACHE_ORDER: the same triangles, in cache order.
}

// OCC_BOX - every mesh's occluder for VIS_OCC, builtin or imported: a box
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
// inflated by OCC_EPS, so the gap stays (far above the float rounding of the
// stored box). occ_ok false = no box: a level with no closed piece, pieces
// inside out, the seed outside their solid or on its surface. occ_hmn /
// occ_hmx bound every level's vertices, open pieces included (a decimated
// level's optimum can leave the unit box): what the camera must stay out of,
// the drawn extent the burial check reads, and - OCC_TARGET - the extent
// every bounds test gives an object (bounds_size_of, LOD_BOUNDS); made
// before the proof, for every mesh whose level tables are sound, whatever it
// decides. Game thread: the builtins in mesh_store, every import before its
// cache write (a cache load takes the sealed box instead) and every generated
// mesh (gen_build).
static constexpr double OCC_EPS = 1.0e-6;
static constexpr int    OCC_ITERS = 30;   // Bisection steps per face: 2^-30 of its reach.
static constexpr int    OCC_ROUNDS = 2;   // Passes over the six faces.
// Separating-axis test of a triangle against an axis-aligned box (centre c,
// half extents h): false only when an axis separates them.
inline bool occ_tri_box(const double* c, const double* h, const double* p0, const double* p1, const double* p2) {
    const double v[3][3] = { { p0[0] - c[0], p0[1] - c[1], p0[2] - c[2] },
                             { p1[0] - c[0], p1[1] - c[1], p1[2] - c[2] },
                             { p2[0] - c[0], p2[1] - c[1], p2[2] - c[2] } };
    const double e[3][3] = { { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] },
                             { v[2][0] - v[1][0], v[2][1] - v[1][1], v[2][2] - v[1][2] },
                             { v[0][0] - v[2][0], v[0][1] - v[2][1], v[0][2] - v[2][2] } };
    auto sep = [&](const double* u) -> bool {
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
        if (sep(u)) return false;
        for (int j = 0; j < 3; ++j) {   // The box axis crossed with each edge.
            const double a[3] = { u[1] * e[j][2] - u[2] * e[j][1], u[2] * e[j][0] - u[0] * e[j][2],
                                  u[0] * e[j][1] - u[1] * e[j][0] };
            if (sep(a)) return false;
        }
    }
    const double n[3] = { e[0][1] * e[1][2] - e[0][2] * e[1][1], e[0][2] * e[1][0] - e[0][0] * e[1][2],
                          e[0][0] * e[1][1] - e[0][1] * e[1][0] };
    return !sep(n);
}
// Squared distance from the origin to a segment and to a triangle (Ericson's
// regions; a triangle with no area falls back to its three edges).
inline double occ_seg_d2(const double* a, const double* b) {
    const double d[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const double l2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    double t = l2 > 0.0 ? -(a[0] * d[0] + a[1] * d[1] + a[2] * d[2]) / l2 : 0.0;
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);
    const double q[3] = { a[0] + t * d[0], a[1] + t * d[1], a[2] + t * d[2] };
    return q[0] * q[0] + q[1] * q[1] + q[2] * q[2];
}
inline double occ_tri_d2(const double* a, const double* b, const double* c) {
    auto dot = [](const double* x, const double* y) { return x[0] * y[0] + x[1] * y[1] + x[2] * y[2]; };
    const double ab[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
    const double ac[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };
    const double pa[3] = { -a[0], -a[1], -a[2] }, pb[3] = { -b[0], -b[1], -b[2] }, pc[3] = { -c[0], -c[1], -c[2] };
    const double d1 = dot(ab, pa), d2 = dot(ac, pa);
    if (d1 <= 0.0 && d2 <= 0.0) return dot(a, a);
    const double d3 = dot(ab, pb), d4 = dot(ac, pb);
    if (d3 >= 0.0 && d4 <= d3) return dot(b, b);
    const double d5 = dot(ab, pc), d6 = dot(ac, pc);
    if (d6 >= 0.0 && d5 <= d6) return dot(c, c);
    const double vc = d1 * d4 - d3 * d2, vb = d5 * d2 - d1 * d6, va = d3 * d6 - d5 * d4;
    if (!(va + vb + vc > 0.0)) {
        const double s0 = occ_seg_d2(a, b), s1 = occ_seg_d2(b, c), s2 = occ_seg_d2(c, a);
        return s0 < s1 ? (s0 < s2 ? s0 : s2) : (s1 < s2 ? s1 : s2);
    }
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) return occ_seg_d2(a, b);
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) return occ_seg_d2(a, c);
    if (va <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0) return occ_seg_d2(b, c);
    const double inv = 1.0 / (va + vb + vc), v = vb * inv, w = vc * inv;
    const double q[3] = { a[0] + ab[0] * v + ac[0] * w, a[1] + ab[1] * v + ac[1] * w, a[2] + ab[2] * v + ac[2] * w };
    return dot(q, q);
}
inline void mesh_occ_prep(MeshDef& d) {
    d.occ_ok = false;
    const size_t nv = d.verts.size(), ni = d.indices.size();
    // The levels' index ranges, 0 .. lod_n (level 0 ends where level 1 begins).
    std::vector<std::pair<size_t, size_t>> lv;
    lv.emplace_back(static_cast<size_t>(0), d.lod_n ? static_cast<size_t>(d.lod_istart[0]) : ni);
    for (int l = 0; l < static_cast<int>(d.lod_n) && l < LOD_MAX; ++l) {
        lv.emplace_back(d.lod_istart[l], d.lod_icount[l]);
    }
    for (const auto& r : lv) {
        if (r.first > ni || r.second > ni - r.first) return;
    }
    // OCC_TARGET: the bounds of everything every level draws, open pieces and
    // triangles with no area included, made before the proof and kept whatever it
    // decides (-0 read as +0, as the weld below reads it). Every index is checked
    // here, so the proof's own walk over these ranges stays in range.
    double mn[3] = { 1.0e300, 1.0e300, 1.0e300 }, mx[3] = { -1.0e300, -1.0e300, -1.0e300 };
    for (const auto& r : lv) {
        for (size_t t = r.first; t < r.first + r.second; ++t) {
            const uint32_t ix = d.indices[t];
            if (ix >= nv) return;
            for (int op_k = 0; op_k < 3; ++op_k) {
                const float f = d.verts[ix].pos[op_k];
                if (!(f > -1.0e6f && f < 1.0e6f)) return;   // NaN or absurd.
                const double x = static_cast<double>(f + 0.0f);
                if (x < mn[op_k]) mn[op_k] = x;
                if (x > mx[op_k]) mx[op_k] = x;
            }
        }
    }
    if (!(mn[0] <= mx[0] && mn[1] <= mx[1] && mn[2] <= mx[2])) return;
    for (int op_k = 0; op_k < 3; ++op_k) {
        d.occ_hmn[op_k] = static_cast<float>(mn[op_k]);   // Exact: float positions.
        d.occ_hmx[op_k] = static_cast<float>(mx[op_k]);
    }
    if (nv < 4u || ni < 12u || ni % 3u != 0u) return;
    for (const auto& r : lv) {
        if (r.second < 12u || r.second % 3u != 0u) return;
    }
    // Positions welded by their exact bits (-0 read as +0): an edge two
    // triangles share is two positions they share, whatever else differs.
    struct OpKey {
        float p[3];
        bool operator==(const OpKey& o) const { return memcmp(p, o.p, sizeof(p)) == 0; }
    };
    struct OpHash {
        size_t operator()(const OpKey& k) const {
            return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(k.p, sizeof(k.p)));
        }
    };
    std::unordered_map<OpKey, uint32_t, OpHash> map;
    map.reserve(nv);
    std::vector<uint32_t> pid(nv);
    std::vector<double> op_p;   // 3 per position.
    op_p.reserve(nv * 3u);
    for (size_t v = 0; v < nv; ++v) {
        OpKey op_k;
        for (int op_c = 0; op_c < 3; ++op_c) {
            const float x = d.verts[v].pos[op_c];
            if (!(x > -1.0e6f && x < 1.0e6f)) return;   // NaN or absurd.
            op_k.p[op_c] = x + 0.0f;
        }
        auto it = map.find(op_k);
        if (it == map.end()) {
            it = map.emplace(op_k, static_cast<uint32_t>(op_p.size() / 3u)).first;
            for (int op_c = 0; op_c < 3; ++op_c) op_p.push_back(op_k.p[op_c]);
        }
        pid[v] = it->second;
    }
    // Every level: its closed pieces' triangles with area kept.
    std::vector<uint32_t> op_tri;   // Position ids, 3 per triangle, level after level.
    std::vector<size_t> lt;      // Where each level's triangles begin in op_tri, then the end.
    struct OpEdge {
        int32_t bal;    // Crossings one way minus the other.
        uint32_t tri;   // A triangle using it: the edge's piece.
    };
    std::unordered_map<uint64_t, OpEdge> edge;
    std::vector<uint32_t> lvt;   // This level's triangles with area.
    std::vector<uint32_t> par;   // Union-find over them: the pieces.
    std::vector<uint8_t> open;   // Per piece root: an edge of it does not balance.
    for (const auto& r : lv) {
        lt.push_back(op_tri.size());
        edge.clear();
        lvt.clear();
        for (size_t t = r.first; t < r.first + r.second; t += 3u) {
            uint32_t q[3];
            for (int op_c = 0; op_c < 3; ++op_c) {
                q[op_c] = pid[d.indices[t + static_cast<size_t>(op_c)]];
            }
            if (q[0] == q[1] || q[1] == q[2] || q[0] == q[2]) continue;   // No area.
            lvt.insert(lvt.end(), q, q + 3);
        }
        const uint32_t ln = static_cast<uint32_t>(lvt.size() / 3u);
        par.resize(ln);
        for (uint32_t i = 0; i < ln; ++i) par[i] = i;
        auto find = [&](uint32_t x) -> uint32_t {
            while (par[x] != x) {
                par[x] = par[par[x]];
                x = par[x];
            }
            return x;
        };
        for (uint32_t i = 0; i < ln; ++i) {
            for (int op_c = 0; op_c < 3; ++op_c) {
                const uint32_t op_a = lvt[i * 3u + static_cast<uint32_t>(op_c)];
                const uint32_t op_b = lvt[i * 3u + static_cast<uint32_t>((op_c + 1) % 3)];
                const uint64_t key = op_a < op_b ? (static_cast<uint64_t>(op_b) << 32) | op_a
                                                          : (static_cast<uint64_t>(op_a) << 32) | op_b;
                const int32_t dir = op_a < op_b ? 1 : -1;
                auto it = edge.find(key);
                if (it == edge.end()) {
                    edge.emplace(key, OpEdge{ dir, i });
                } else {
                    it->second.bal += dir;
                    const uint32_t ra = find(i), rb = find(it->second.tri);
                    if (ra != rb) par[ra] = rb;
                }
            }
        }
        // A piece is open when any of its edges does not balance (a hole, a
        // crack, a sheet); each edge belongs to exactly one piece.
        open.assign(ln, 0u);
        for (const auto& kv : edge) {
            if (kv.second.bal != 0) open[find(kv.second.tri)] = 1u;
        }
        for (uint32_t i = 0; i < ln; ++i) {
            if (open[find(i)]) continue;
            op_tri.insert(op_tri.end(), lvt.begin() + i * 3u, lvt.begin() + i * 3u + 3u);
        }
        if (op_tri.size() == lt.back()) return;   // No closed piece at this level.
    }
    lt.push_back(op_tri.size());
    const size_t nt = op_tri.size() / 3u;
    if (nt < 4u) return;
    auto pt = [&](size_t t, int op_c) -> const double* {
        return &op_p[static_cast<size_t>(op_tri[t * 3u + static_cast<size_t>(op_c)]) * 3u];
    };
    // The seed inside every level's closed pieces (winding at least 1, +g
    // outward) and clear of every one of their triangles.
    for (size_t l = 0; l + 1u < lt.size(); ++l) {
        double w = 0.0;
        for (size_t t = lt[l] / 3u; t < lt[l + 1u] / 3u; ++t) {
            const double* a = pt(t, 0);
            const double* b = pt(t, 1);
            const double* c = pt(t, 2);
            const double la = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
            const double lb = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
            const double lc = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
            if (!(la > 0.0 && lb > 0.0 && lc > 0.0)) return;   // The seed on a vertex.
            const double det = a[0] * (b[1] * c[2] - b[2] * c[1]) + a[1] * (b[2] * c[0] - b[0] * c[2]) +
                               a[2] * (b[0] * c[1] - b[1] * c[0]);
            const double den = la * lb * lc + (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) * lc +
                               (a[0] * c[0] + a[1] * c[1] + a[2] * c[2]) * lb +
                               (b[0] * c[0] + b[1] * c[1] + b[2] * c[2]) * la;
            w += 2.0 * atan2(det, den);   // The solid angle (Van Oosterom - Strackee).
        }
        w /= 4.0 * 3.14159265358979323846;
        const double wr = floor(w + 0.5);
        if (!(wr >= 1.0) || !(fabs(w - wr) < 0.25)) return;   // Outside the solid, or inside out.
    }
    double d2 = 1.0e300;
    for (size_t t = 0; t < nt; ++t) {
        const double e = occ_tri_d2(pt(t, 0), pt(t, 1), pt(t, 2));
        if (e < d2) d2 = e;
    }
    const double r0 = (sqrt(d2) - OCC_EPS) / sqrt(3.0);   // Its corners OCC_EPS inside the ball.
    if (!(r0 > OCC_EPS)) return;
    double lo[3] = { -r0, -r0, -r0 }, hi[3] = { r0, r0, r0 };
    // A uniform grid of the closed pieces' triangles over the bounds, for the
    // slab queries.
    int g = static_cast<int>(cbrt(static_cast<double>(nt) * 0.5));
    g = g < 1 ? 1 : (g > 64 ? 64 : g);
    double inv[3];
    for (int op_k = 0; op_k < 3; ++op_k) {
        const double ext = mx[op_k] - mn[op_k];
        inv[op_k] = ext > 0.0 ? static_cast<double>(g) / ext : 0.0;
    }
    auto cell = [&](double x, int op_k) -> int {
        const double f = floor((x - mn[op_k]) * inv[op_k]);
        return f < 0.0 ? 0 : (f > static_cast<double>(g - 1) ? g - 1 : static_cast<int>(f));
    };
    const size_t gn = static_cast<size_t>(g) * static_cast<size_t>(g) * static_cast<size_t>(g);
    std::vector<uint32_t> cs(gn + 1u, 0u), cl;
    for (int pass = 0; pass < 2; ++pass) {   // Count, then fill (CSR).
        if (pass == 1) {
            for (size_t i = 1; i <= gn; ++i) cs[i] += cs[i - 1u];
            cl.assign(cs[gn], 0u);
        }
        std::vector<uint32_t> op_fill;
        if (pass == 1) op_fill.assign(cs.begin(), cs.end() - 1);
        for (size_t t = 0; t < nt; ++t) {
            int i0[3], i1[3];
            for (int op_k = 0; op_k < 3; ++op_k) {
                double op_a = pt(t, 0)[op_k], op_b = op_a;
                for (int op_c = 1; op_c < 3; ++op_c) {
                    const double x = pt(t, op_c)[op_k];
                    if (x < op_a) op_a = x;
                    if (x > op_b) op_b = x;
                }
                i0[op_k] = cell(op_a - OCC_EPS, op_k);
                i1[op_k] = cell(op_b + OCC_EPS, op_k);
            }
            for (int z = i0[2]; z <= i1[2]; ++z)
                for (int y = i0[1]; y <= i1[1]; ++y)
                    for (int x = i0[0]; x <= i1[0]; ++x) {
                        const size_t ci = (static_cast<size_t>(z) * g + y) * g + x;
                        if (pass == 0) ++cs[ci + 1u];
                        else cl[op_fill[ci]++] = static_cast<uint32_t>(t);
                    }
        }
    }
    std::vector<uint32_t> seen(nt, 0u);
    uint32_t stamp = 0u;
    // Whether any closed piece's triangle meets the box [bmn, bmx] inflated by
    // OCC_EPS.
    auto hits = [&](const double* bmn, const double* bmx) -> bool {
        if (++stamp == 0u) { std::fill(seen.begin(), seen.end(), 0u); stamp = 1u; }
        double op_c[3], h[3];
        int i0[3], i1[3];
        for (int op_k = 0; op_k < 3; ++op_k) {
            op_c[op_k] = 0.5 * (bmn[op_k] + bmx[op_k]);
            h[op_k] = 0.5 * (bmx[op_k] - bmn[op_k]) + OCC_EPS;
            i0[op_k] = cell(bmn[op_k] - OCC_EPS, op_k);
            i1[op_k] = cell(bmx[op_k] + OCC_EPS, op_k);
        }
        for (int z = i0[2]; z <= i1[2]; ++z)
            for (int y = i0[1]; y <= i1[1]; ++y)
                for (int x = i0[0]; x <= i1[0]; ++x) {
                    const size_t ci = (static_cast<size_t>(z) * g + y) * g + x;
                    for (uint32_t j = cs[ci]; j < cs[ci + 1u]; ++j) {
                        const uint32_t t = cl[j];
                        if (seen[t] == stamp) continue;
                        seen[t] = stamp;
                        if (occ_tri_box(op_c, h, pt(t, 0), pt(t, 1),
                                        pt(t, 2))) return true;
                    }
                }
        return false;
    };
    // Growth: each face in turn, as far as a slab of the growth stays clear.
    // The solid ends inside the bounds, so a face never passes them.
    for (int rd = 0; rd < OCC_ROUNDS; ++rd) {
        for (int f = 0; f < 6; ++f) {
            const int ax = f >> 1;
            const bool up = (f & 1) == 0;
            double op_a = up ? hi[ax] : lo[ax];   // Clear so far.
            double op_b = up ? mx[ax] : mn[ax];
            if (up ? !(op_b > op_a) : !(op_b < op_a)) continue;
            for (int it = 0; it < OCC_ITERS; ++it) {
                const double m = 0.5 * (op_a + op_b);
                double smn[3] = { lo[0], lo[1], lo[2] };
                double smx[3] = { hi[0], hi[1], hi[2] };
                if (up) { smn[ax] = op_a; smx[ax] = m; }
                else         { smn[ax] = m; smx[ax] = op_a; }
                if (hits(smn, smx)) op_b = m; else op_a = m;
            }
            if (up) hi[ax] = op_a; else lo[ax] = op_a;
        }
    }
    for (int op_k = 0; op_k < 3; ++op_k) {
        d.occ_c[op_k] = static_cast<float>(0.5 * (lo[op_k] + hi[op_k]));
        d.occ_h[op_k] = static_cast<float>(0.5 * (hi[op_k] - lo[op_k]));
    }
    d.occ_ok = true;
}
// OCC_BOX - the mesh cache's box chunk (OCC_CACHE_MAGIC): the verdict, the
// box and the bounds as a 12-float record (occ_c, occ_h, occ_hmn, occ_hmx), and
// a seal - FNV-1a 64 over the verdict, the record and everything the proof read
// (positions, indices, the level tables) - so a cached box is taken only for
// exactly the triangles it was proven on. Any other byte: a cache miss, and
// the mesh imports again.
inline void mesh_occ_record(const MeshDef& d, float r[12]) {
    for (int k = 0; k < 3; ++k) {
        r[k] = d.occ_c[k];
        r[3 + k] = d.occ_h[k];
        r[6 + k] = d.occ_hmn[k];
        r[9 + k] = d.occ_hmx[k];
    }
}
inline uint64_t mesh_occ_seal(const MeshDef& d, uint32_t ok, const float r[12]) {
    uint64_t h = CryptoGenerator::FNV1A64_OFFSET;
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(&ok), 4);
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(r), 48);
    for (const MeshVertex& v : d.verts) {
        h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(v.pos), 12);
    }
    if (!d.indices.empty()) {
        h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(d.indices.data()),
                                            d.indices.size() * sizeof(uint32_t));
    }
    const uint32_t ln = d.lod_n;
    h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(&ln), 4);
    if (ln) {
        h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(d.lod_istart),
                                            ln * 4u);
        h = CryptoGenerator::fnv1a64_update(h, reinterpret_cast<const char*>(d.lod_icount),
                                            ln * 4u);
    }
    return h;
}
// A read chunk, taken when its seal matches this geometry and the record is
// sane (finite; a box inside its bounds). False = a cache miss.
inline bool mesh_occ_accept(MeshDef& d, uint32_t ok, const float r[12], uint64_t seal) {
    if (ok > 1u) return false;
    for (int k = 0; k < 12; ++k) if (!(r[k] > -1.0e6f && r[k] < 1.0e6f)) return false;
    if (ok) {
        for (int k = 0; k < 3; ++k) {
            if (!(r[3 + k] > 0.0f) || r[k] - r[3 + k] < r[6 + k] ||
                r[k] + r[3 + k] > r[9 + k]) return false;
        }
    }
    if (mesh_occ_seal(d, ok, r) != seal) return false;
    for (int k = 0; k < 3; ++k) {
        d.occ_c[k] = r[k];
        d.occ_h[k] = r[3 + k];
        d.occ_hmn[k] = r[6 + k];
        d.occ_hmx[k] = r[9 + k];
    }
    d.occ_ok = ok != 0u;
    return true;
}
}