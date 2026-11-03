#pragma once

// The feature half (rendering_integration_features.hpp): the constant-buffer uploads (the object / frame CB mirrors and
// rings). Never include it on its own.
namespace RenderIntegration {

// CB-split slice geometry: the object block ends where view_proj (the first frame field) begins, and goes up whole
// (the light records are in the light ring, DL_RING, not in it).

static constexpr size_t CBOBJ_BYTES      = offsetof(ConstantData, view_proj);
static constexpr size_t CBFRAME_BYTES    = sizeof(ConstantData) - CBOBJ_BYTES;
// EA_CB_ALIGN: both are constant buffers' ByteWidths (and the frame block's start is a float4 lane's).
// The HLSL CBObj/CBFrame declarations and this struct are a hand-kept mirror; ensure_resources' full build
// (rendering/device_resources.hpp; not its initialized fast path) reflects the compiled PSMain's CBObj / CBFrame
// sizes - sizes only, not field offsets - and reports a mismatch.
static_assert(CBOBJ_BYTES % 16u == 0u && CBFRAME_BYTES % 16u == 0u, "CBObj / CBFrame are whole float4 rows");

// The one object-CB rotation fill (every mesh pass's object fill and the vertex stage's). rot_m = engine-axes rows
// (RenderObject::rot_m or a caster record's rot).
inline void fill_obj_rot(ConstantData& cbd, const float* rot_m) {
    for (int rr = 0; rr < 3; ++rr) {
        cbd.obj_rot[rr][0] = rot_m[rr * 3 + 0];
        cbd.obj_rot[rr][1] = rot_m[rr * 3 + 1];
        cbd.obj_rot[rr][2] = rot_m[rr * 3 + 2];
    }

    cbd.obj_rot[0][3] = 1.0f;   // Rows filled: KhRotate / KhObjRows apply them (a zeroed block reads as identity).
}

// A NO_OVERWRITE map into a region the GPU is not reading is a pointer return. Slice = the object block rounded to
// 256 B (numConstants must be a multiple of 16).
static constexpr uint32_t CBR_STRIDE = static_cast<uint32_t>((CBOBJ_BYTES + 255u) & ~static_cast<size_t>(255u));
static constexpr uint32_t CBR_SLICES = 1024u;
static constexpr uint32_t CBR_CONSTS = CBR_STRIDE / 16u;
static bool     g_cbr_supported = false;   // Feature pair present on this device

// The object block goes up whole. Its light records are not in it (DL_RING): dl_first names them in the light ring.
inline bool upload_obj_cb(ID3D11DeviceContext* ctx, ID3D11Buffer* buf, const ConstantData& cbd) {
    const size_t bytes = CBOBJ_BYTES;

    // Ring route: the two per-object buffers only (the white placeholder keeps its own, by its no-borrowing contract).
    const int ri = (buf == g_res.constant_buffer) ? 0 : (buf == g_res.composite_cb) ? 1 : -1;
    ID3D11DeviceContext1* c1 = nullptr;

    if (ri >= 0 && g_cbr_supported && g_res.cb_ring[ri] && (c1 = as_ctx1(ctx)) != nullptr) {
        ID3D11Buffer* ring = g_res.cb_ring[ri];
        uint32_t& cur = g_res.cb_ring_cursor[ri];
        bool& fresh = g_res.cb_ring_fresh[ri];

        if (cur >= CBR_SLICES) { cur = 0; fresh = true; }   // Wrap: rename, never stall.
        D3D11_MAPPED_SUBRESOURCE m = {};
        const D3D11_MAP mode = fresh ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

        if (SUCCEEDED(ctx->Map(ring, 0, mode, 0, &m))) {
            fresh = false;
            memcpy(static_cast<uint8_t*>(m.pData) + static_cast<size_t>(cur) * CBR_STRIDE,
                   &cbd, bytes);
            ctx->Unmap(ring, 0);
            const UINT first = cur * CBR_CONSTS;
            const UINT num = CBR_CONSTS;
            c1->VSSetConstantBuffers1(0, 1, &ring, &first, &num);
            c1->PSSetConstantBuffers1(0, 1, &ring, &first, &num);
            if (g_ctx1_null_first) {
                ID3D11Buffer* null = nullptr;
                ctx->VSSetConstantBuffers(0, 1, &null);
                ctx->PSSetConstantBuffers(0, 1, &null);
                c1->VSSetConstantBuffers1(0, 1, &ring, &first, &num);
                c1->PSSetConstantBuffers1(0, 1, &ring, &first, &num);
            }
            cur++;
            return true;
        }
        // The ring re-DISCARDs on its next use rather than trusting a region it never wrote.
        fresh = true;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    memcpy(mapped.pData, &cbd, bytes);
    ctx->Unmap(buf, 0);
    // The plain buffer must be what b0 reads: a ring slice may still be bound there from the previous upload.
    if (ri >= 0 && g_cbr_supported && g_res.cb_ring[ri]) {
        ctx->VSSetConstantBuffers(0, 1, &buf);
        ctx->PSSetConstantBuffers(0, 1, &buf);
    }
    return true;
}

// CB_DERIVED: the frame lanes formed from the frame's own matrices, written straight into the mapped upload at
// their offsets past the object block (every frame upload comes through upload_frame_cb, which takes the struct
// const). ConstantData's note has the layout.
inline void cb_derived(uint8_t* dst, const ConstantData& c) {
    float col[3][4] = {};
    const float (*const m[5])[4] = { c.sun_vp, c.sun_vp2, c.sun_vp3, c.sun_vp4, c.sun_vp5 };
    for (int i = 0; i < 5; ++i) {
        const float (*const v)[4] = m[i];
        float* const o = &col[i >> 1][(i & 1) * 2];
        o[0] = sqrtf(v[0][0] * v[0][0] + v[1][0] * v[1][0] + v[2][0] * v[2][0]);
        o[1] = sqrtf(v[0][2] * v[0][2] + v[1][2] * v[1][2] + v[2][2] * v[2][2]);
    }
    float cam[4] = {};
    for (int r = 0; r < 3; ++r) {
        cam[r] = -(c.cast_view[0][0] * c.cast_mat[r][0] +
                   c.cast_view[0][1] * c.cast_mat[r][1] +
                   c.cast_view[0][2] * c.cast_mat[r][2]);
    }
    memcpy(dst + (offsetof(ConstantData, sun_col) - CBOBJ_BYTES), col, sizeof(col));
    memcpy(dst + (offsetof(ConstantData, cast_cam) - CBOBJ_BYTES), cam, sizeof(cam));
}

inline bool upload_frame_cb(ID3D11DeviceContext* ctx, ID3D11Buffer* buf, const ConstantData& cbd) {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    memcpy(mapped.pData, reinterpret_cast<const uint8_t*>(&cbd) + CBOBJ_BYTES, CBFRAME_BYTES);
    cb_derived(static_cast<uint8_t*>(mapped.pData), cbd);   // CB_DERIVED.
    ctx->Unmap(buf, 0);
    return true;
}

// depthParams.zw: the trigger draw's own viewport depth range when seen this cycle, else the scene's; a poisoned
// range garbles the linearization for every pixel, so it is sanitised here, for every fill site (the mesh / chain
// fills of the injection, the flush and the UI lane, FX_PAIR_HOLD's two guard temporaries and WSEE's decode
// temporary in flush_locked). EA_WORLD_RANGE: the rule itself is world_depth_range, which WSEE's CPU decode
// (see_through.hpp) reads too.
inline void world_depth_range(float& lo, float& hi) {
    lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
    if (!(lo >= 0.0f && hi <= 1.0f && lo < hi)) {
        lo = g_scene_vp_min_d;
        hi = g_scene_vp_max_d;
    }
}
inline void fill_depth_range_cb(ConstantData& cbd) {
    world_depth_range(cbd.depth_params[2], cbd.depth_params[3]);
}
}