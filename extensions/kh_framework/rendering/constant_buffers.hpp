#pragma once

// The feature half (rendering_integration_features.hpp): the constant-buffer uploads (the object / frame CB mirrors and
// rings). Never include it on its own.
namespace RenderIntegration {

// CB-split slice geometry: the object block ends where view_proj (the first frame field) begins, and goes up whole
// (the light records are in the light ring, KH_DL_RING, not in it).

static constexpr size_t KH_CBOBJ_BYTES      = offsetof(ConstantData, view_proj);
static constexpr size_t KH_CBFRAME_BYTES    = sizeof(ConstantData) - KH_CBOBJ_BYTES;
// The HLSL CBObj/CBFrame declarations and this struct are a hand-kept mirror; ensure_resources' full build (not its
// initialized fast path) reflects the compiled PSMain's CBObj / CBFrame sizes - sizes only, not field offsets - and
// reports a mismatch.
static uint32_t g_cb_mirror_obj_b = 0;
static uint32_t g_cb_mirror_frame_b = 0;
static bool     g_cb_mirror_bad = false;

// The one mesh-CB rotation fill (injection, flush, sun fallback). rot_m = engine-axes rows (RenderObject::rot_m or
// SunCaster::rot).
inline void kh_fill_obj_rot(ConstantData& cbd, const float* rot_m) {
    for (int rr = 0; rr < 3; ++rr) {
        cbd.obj_rot[rr][0] = rot_m[rr * 3 + 0];
        cbd.obj_rot[rr][1] = rot_m[rr * 3 + 1];
        cbd.obj_rot[rr][2] = rot_m[rr * 3 + 2];
    }

    cbd.obj_rot[0][3] = 1.0f;   // KhRotate engages identity matrix when the object is unrotated.
}

// A NO_OVERWRITE map into a region the GPU is not reading is a pointer return. Slice = the object block rounded to
// 256 B (numConstants must be a multiple of 16).
static constexpr uint32_t KH_CBR_STRIDE = static_cast<uint32_t>((KH_CBOBJ_BYTES + 255u) & ~static_cast<size_t>(255u));
static constexpr uint32_t KH_CBR_SLICES = 1024u;
static constexpr uint32_t KH_CBR_CONSTS = KH_CBR_STRIDE / 16u;
static bool     g_cbr_supported = false;   // Feature pair present on this device

// The object block goes up whole. Its light records are not in it (KH_DL_RING): dl_first names them in the light ring.
inline bool kh_upload_obj_cb(ID3D11DeviceContext* ctx, ID3D11Buffer* buf, const ConstantData& cbd) {
    const size_t khu_bytes = KH_CBOBJ_BYTES;

    // Ring route: the two per-object buffers only (the white placeholder keeps its own, by its no-borrowing contract).
    const int khu_ri = (buf == g_res.constant_buffer) ? 0 : (buf == g_res.composite_cb) ? 1 : -1;
    ID3D11DeviceContext1* khu_c1 = nullptr;

    if (khu_ri >= 0 && g_cbr_supported && g_res.cb_ring[khu_ri] && (khu_c1 = kh_ctx1(ctx)) != nullptr) {
        ID3D11Buffer* khu_ring = g_res.cb_ring[khu_ri];
        uint32_t& khu_cur = g_res.cb_ring_cursor[khu_ri];
        bool& khu_fresh = g_res.cb_ring_fresh[khu_ri];

        if (khu_cur >= KH_CBR_SLICES) { khu_cur = 0; khu_fresh = true; }   // Wrap: rename, never stall.
        D3D11_MAPPED_SUBRESOURCE khu_m = {};
        const D3D11_MAP khu_mode = khu_fresh ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

        if (SUCCEEDED(ctx->Map(khu_ring, 0, khu_mode, 0, &khu_m))) {
            khu_fresh = false;
            memcpy(static_cast<uint8_t*>(khu_m.pData) + static_cast<size_t>(khu_cur) * KH_CBR_STRIDE,
                   &cbd, khu_bytes);
            ctx->Unmap(khu_ring, 0);
            const UINT khu_first = khu_cur * KH_CBR_CONSTS;
            const UINT khu_num = KH_CBR_CONSTS;
            khu_c1->VSSetConstantBuffers1(0, 1, &khu_ring, &khu_first, &khu_num);
            khu_c1->PSSetConstantBuffers1(0, 1, &khu_ring, &khu_first, &khu_num);
            if (g_kh_ctx1_null_first) {
                ID3D11Buffer* khu_null = nullptr;
                ctx->VSSetConstantBuffers(0, 1, &khu_null);
                ctx->PSSetConstantBuffers(0, 1, &khu_null);
                khu_c1->VSSetConstantBuffers1(0, 1, &khu_ring, &khu_first, &khu_num);
                khu_c1->PSSetConstantBuffers1(0, 1, &khu_ring, &khu_first, &khu_num);
            }
            khu_cur++;
            return true;
        }
        // The ring re-DISCARDs on its next use rather than trusting a region it never wrote.
        khu_fresh = true;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    memcpy(mapped.pData, &cbd, khu_bytes);
    ctx->Unmap(buf, 0);
    // The plain buffer must be what b0 reads: a ring slice may still be bound there from the previous upload.
    if (khu_ri >= 0 && g_cbr_supported && g_res.cb_ring[khu_ri]) {
        ctx->VSSetConstantBuffers(0, 1, &buf);
        ctx->PSSetConstantBuffers(0, 1, &buf);
    }
    return true;
}

// KH_CB_DERIVED: the frame lanes formed from the frame's own matrices, written straight into the mapped upload at
// their offsets past the object block (every frame upload comes through kh_upload_frame_cb, which takes the struct
// const). ConstantData's note has the layout.
inline void kh_cb_derived(uint8_t* khcd_dst, const ConstantData& khcd_c) {
    float khcd_col[3][4] = {};
    const float (*const khcd_m[5])[4] = { khcd_c.sun_vp, khcd_c.sun_vp2, khcd_c.sun_vp3, khcd_c.sun_vp4, khcd_c.sun_vp5 };
    for (int khcd_i = 0; khcd_i < 5; ++khcd_i) {
        const float (*const khcd_v)[4] = khcd_m[khcd_i];
        float* const khcd_o = &khcd_col[khcd_i >> 1][(khcd_i & 1) * 2];
        khcd_o[0] = sqrtf(khcd_v[0][0] * khcd_v[0][0] + khcd_v[1][0] * khcd_v[1][0] + khcd_v[2][0] * khcd_v[2][0]);
        khcd_o[1] = sqrtf(khcd_v[0][2] * khcd_v[0][2] + khcd_v[1][2] * khcd_v[1][2] + khcd_v[2][2] * khcd_v[2][2]);
    }
    float khcd_cam[4] = {};
    for (int khcd_r = 0; khcd_r < 3; ++khcd_r) {
        khcd_cam[khcd_r] = -(khcd_c.cast_view[0][0] * khcd_c.cast_mat[khcd_r][0] +
                             khcd_c.cast_view[0][1] * khcd_c.cast_mat[khcd_r][1] +
                             khcd_c.cast_view[0][2] * khcd_c.cast_mat[khcd_r][2]);
    }
    memcpy(khcd_dst + (offsetof(ConstantData, sun_col) - KH_CBOBJ_BYTES), khcd_col, sizeof(khcd_col));
    memcpy(khcd_dst + (offsetof(ConstantData, cast_cam) - KH_CBOBJ_BYTES), khcd_cam, sizeof(khcd_cam));
}

inline bool kh_upload_frame_cb(ID3D11DeviceContext* ctx, ID3D11Buffer* buf, const ConstantData& cbd) {
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(ctx->Map(buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    memcpy(mapped.pData, reinterpret_cast<const uint8_t*>(&cbd) + KH_CBOBJ_BYTES, KH_CBFRAME_BYTES);
    kh_cb_derived(static_cast<uint8_t*>(mapped.pData), cbd);   // KH_CB_DERIVED.
    ctx->Unmap(buf, 0);
    return true;
}

// depthParams.zw: the trigger draw's own viewport depth range when seen this cycle, else the scene's; a poisoned
// range garbles the linearization for every pixel, so it is sanitised here, for every fill site (the mesh / chain
// fills of the injection, the flush and the UI lane, KH_FX_PAIR_HOLD's two guard temporaries and KH_WSEE's decode
// temporary in flush_locked).
inline void kh_fill_depth_range_cb(ConstantData& cbd) {
    cbd.depth_params[2] = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    cbd.depth_params[3] = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;
    if (!(cbd.depth_params[2] >= 0.0f && cbd.depth_params[3] <= 1.0f &&
          cbd.depth_params[2] < cbd.depth_params[3])) {
        cbd.depth_params[2] = g_scene_vp_min_d;
        cbd.depth_params[3] = g_scene_vp_max_d;
    }
}
}