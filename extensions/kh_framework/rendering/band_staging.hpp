#pragma once

// The injection half (rendering_integration_injection.hpp): the shadow band staging pool and its commit. Never include
// it on its own.
namespace RenderIntegration {

// One staging back buffer per band: with fewer, per-frame reseals fill the pool at the first resolves and the other
// bands keep a seal whose window no longer matches their capture. A seal taken on a view one frame apart is offset by
// exactly that camera step; absent beats offset. The cold escape hatch lets a slot take its first seal on a dead view
// source.
static constexpr int KH_BAND_STAGE_POOL = 8;
inline int kh_band_pool_n() {
    return KH_BAND_STAGE_POOL;
}

struct KhBandStageTex {
    ID3D11Texture2D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    bool                      busy = false;
};

static KhBandStageTex g_band_pool[KH_BAND_STAGE_POOL];

// Also clears every slot's staging: a staged capture whose back buffer just
// died must not commit. Called on atlas desc change and device reset.
inline void kh_band_pool_release() {
    for (int khbp_k = 0; khbp_k < KH_BAND_STAGE_POOL; ++khbp_k) {
        if (g_band_pool[khbp_k].srv) { g_band_pool[khbp_k].srv->Release(); g_band_pool[khbp_k].srv = nullptr; }
        if (g_band_pool[khbp_k].tex) { g_band_pool[khbp_k].tex->Release(); g_band_pool[khbp_k].tex = nullptr; }
        g_band_pool[khbp_k].busy = false;
    }

    for (int khbp_b = 0; khbp_b < 8; ++khbp_b) {
        g_ls.band[khbp_b].stage_valid = false;
        g_ls.band[khbp_b].stage_pool = -1;
    }

    g_ls.stage_any = false;
}

// The view a copy of the adopted atlas is read through - its typeless format's depth plane, as the atlas's own view
// (shadow_live_ensure_srv) - or UNKNOWN for a format adoption refuses. Every band and staging copy takes it: a copy
// is made from the atlas's desc (CopyResource demands that), so an R32_FLOAT view fits only an R32 atlas.
inline DXGI_FORMAT kh_atlas_view_fmt(uint32_t khav_fmt) {
    switch (static_cast<DXGI_FORMAT>(khav_fmt)) {
        case DXGI_FORMAT_R32_TYPELESS:      return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_R16_TYPELESS:      return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}
// Desc comes from the live atlas; non-fatal, a failed create leaves the slot null and the miss path stands.
inline void kh_band_pool_ensure_n(int khbe_want) {
    if (!g_ls.atlas_tex) return;
    if (khbe_want > KH_BAND_STAGE_POOL) khbe_want = KH_BAND_STAGE_POOL;
    bool khbe_missing = false;
    for (int khbe_k = 0; khbe_k < khbe_want; ++khbe_k) {
        if (!g_band_pool[khbe_k].tex) { khbe_missing = true; break; }
    }
    if (!khbe_missing) return;
    D3D11_TEXTURE2D_DESC khbe_td = {};
    g_ls.atlas_tex->GetDesc(&khbe_td);
    const DXGI_FORMAT khbe_vf = kh_atlas_view_fmt(g_ls.atlas_fmt);
    if (khbe_vf == DXGI_FORMAT_UNKNOWN) return;
    khbe_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    khbe_td.Usage = D3D11_USAGE_DEFAULT;
    khbe_td.CPUAccessFlags = 0;
    khbe_td.MiscFlags = 0;
    ID3D11Device* khbe_dev = nullptr;
    g_ls.atlas_tex->GetDevice(&khbe_dev);
    if (!khbe_dev) return;
    for (int khbe_k = 0; khbe_k < khbe_want; ++khbe_k) {
        if (g_band_pool[khbe_k].tex) continue;
        if (SUCCEEDED(khbe_dev->CreateTexture2D(&khbe_td, nullptr, &g_band_pool[khbe_k].tex)) &&
            g_band_pool[khbe_k].tex) {
            D3D11_SHADER_RESOURCE_VIEW_DESC khbe_sd = {};
            khbe_sd.Format = khbe_vf;   // The atlas's own format (a band is its copy).
            khbe_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            khbe_sd.Texture2D.MipLevels = 1;
            if (FAILED(khbe_dev->CreateShaderResourceView(g_band_pool[khbe_k].tex,
                                                          &khbe_sd, &g_band_pool[khbe_k].srv))) {
                g_band_pool[khbe_k].srv = nullptr;
                g_band_pool[khbe_k].tex->Release();
                g_band_pool[khbe_k].tex = nullptr;
            }
        }
        g_band_pool[khbe_k].busy = false;
    }
    khbe_dev->Release();
}

// A candidate seal view whose camera sits farther than KH_LATCH_JUMP_M-class distance from the cycle latch camera is a
// foreign render and must never be committed into a band seal. true = ok.
inline bool kh_band_view_native(const float* khfv_v, bool khfv_count) {
    if (!g_ro.cycle_pv_valid) return true;   // No reference: cannot judge, admit.
    float khfv_c[3], khfv_r[3];
    if (!kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(khfv_v), khfv_c)) return true;
    if (!kh_view_camera_exact(g_ro.cycle_pv.view, khfv_r)) return true;
    const float khfv_dx = khfv_c[0] - khfv_r[0];
    const float khfv_dy = khfv_c[1] - khfv_r[1];
    const float khfv_dz = khfv_c[2] - khfv_r[2];
    const float khfv_d = sqrtf(khfv_dx * khfv_dx + khfv_dy * khfv_dy + khfv_dz * khfv_dz);
    if (!(khfv_d == khfv_d) || khfv_d <= 50.0f) return true;
    return false;
}
// The capture-time view (vcol column layout) against the welding view (row layout): the distance between their
// cameras, -1 when unjudgeable (either camera undecodable). No bar here: its one caller (shadow_view_scan's
// native-view stage pass) sets a stage's stage_pair_ok when the gap passes 0.5 m (same source within the 25 ms window
// is centimetres, cross-source is metres); the 50 m pair verdict is kh_band_pair_coherent (below).
inline float kh_band_source_gap(const float* khss_vc, const float* khss_fv) {
    float khss_m[4][4] = {};   // KH_CAM_HONEST: the row layout, for the honest inverse.
    for (int khss_j = 0; khss_j < 3; ++khss_j) {
        const float* khss_v = &khss_vc[khss_j * 4];
        const float khss_n = sqrtf(khss_v[0] * khss_v[0] +
                                   khss_v[1] * khss_v[1] +
                                   khss_v[2] * khss_v[2]);
        if (!(khss_n > 0.9f && khss_n < 1.1f)) return -1.0f;
        for (int khss_i = 0; khss_i < 4; ++khss_i) khss_m[khss_i][khss_j] = khss_v[khss_i];
    }
    khss_m[3][3] = 1.0f;
    float khss_a[3];
    kh_cam_of(khss_m, khss_a);
    float khss_b[3];
    if (!kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(khss_fv), khss_b)) return -1.0f;
    const float khss_dx = khss_a[0] - khss_b[0];
    const float khss_dy = khss_a[1] - khss_b[1];
    const float khss_dz = khss_a[2] - khss_b[2];
    const float khss_d = sqrtf(khss_dx * khss_dx + khss_dy * khss_dy + khss_dz * khss_dz);
    if (!(khss_d == khss_d)) return -1.0f;
    return khss_d;
}

static uint64_t g_band_fall_flush = ~0ull;   // Flush of the last allowed fall-through.
// The verdict form: true = within 50 m or unjudgeable (admit), false = refuse.
inline bool kh_band_pair_coherent(const float* khpc_vc, const float* khpc_fv) {
    float khpc_m[4][4] = {};   // KH_CAM_HONEST: the row layout, for the honest inverse.
    for (int khpc_j = 0; khpc_j < 3; ++khpc_j) {   // Decode the capture camera.
        const float* khpc_v = &khpc_vc[khpc_j * 4];
        const float khpc_n = sqrtf(khpc_v[0] * khpc_v[0] +
                                   khpc_v[1] * khpc_v[1] +
                                   khpc_v[2] * khpc_v[2]);
        if (!(khpc_n > 0.9f && khpc_n < 1.1f)) return true;
        for (int khpc_i = 0; khpc_i < 4; ++khpc_i) khpc_m[khpc_i][khpc_j] = khpc_v[khpc_i];
    }
    khpc_m[3][3] = 1.0f;
    float khpc_a[3];
    kh_cam_of(khpc_m, khpc_a);
    float khpc_b[3];
    if (!kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(khpc_fv), khpc_b)) return true;
    const float khpc_dx = khpc_a[0] - khpc_b[0];
    const float khpc_dy = khpc_a[1] - khpc_b[1];
    const float khpc_dz = khpc_a[2] - khpc_b[2];
    const float khpc_d = sqrtf(khpc_dx * khpc_dx + khpc_dy * khpc_dy + khpc_dz * khpc_dz);
    if (!(khpc_d == khpc_d) || khpc_d <= 50.0f) return true;
    return false;
}
// Gated on a plain bool, so the steady-state cost is one load.
inline void kh_band_stage_commit(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_BAND_COMMIT);   // KH_PROF.
    if (!g_ls.stage_any) return;
    if (!ctx || !g_ls.atlas_tex) return;
    if (!g_ls.frame_view_valid || g_ls.frame_view_time < 0.0f) return;
    // The pairing view is this cycle's publish; a staged capture from this cycle is still waiting for the next one.
    if (g_boundary_t >= 0.0f && g_ls.frame_view_time < g_boundary_t) return;

    const float khsc_now = effect_time_seconds();
    bool khsc_any = false;

    for (int khsc_i = 0; khsc_i < 8; ++khsc_i) {
        auto& b = g_ls.band[khsc_i];
        if (!b.stage_valid) continue;

        const int khsc_p = b.stage_pool;
        const bool khsc_dead = !b.valid || !b.tex || !b.srv ||
                               khsc_p < 0 || khsc_p >= KH_BAND_STAGE_POOL ||
                               !g_band_pool[khsc_p].tex || !g_band_pool[khsc_p].srv;

        if (khsc_dead) {   // Wiped underneath us: release and reseal clean.
            if (khsc_p >= 0 && khsc_p < KH_BAND_STAGE_POOL) g_band_pool[khsc_p].busy = false;
            b.stage_valid = false;
            b.stage_pool = -1;
            b.last_time = 0.0f;
            continue;
        }

        if (g_boundary_t >= 0.0f && b.stage_since >= g_boundary_t) {
            // Staged this cycle, always: the engine resolves its cascades at the end of the cycle, after our injection,
            // for the frame that follows, so the commit at the next cycle's first draws pairs that atlas with the next
            // cycle's view. An in-cycle commit pairs it with the CURRENT view and offsets the received shadows.
            khsc_any = true;
            continue;
        }

        const bool khsc_aged = khsc_now - b.stage_since > 0.05f;

        // The family and camera identity of the pairing view are tested below (kh_band_view_native, the pair
        // coherence); no rotation-agreement bar against the bridge PV (it never fired). The rare flick frame that
        // remains is the view publish landing after the injection (the early return above), which no bar can close.

        if (khsc_aged) {
            g_band_pool[khsc_p].busy = false;
            b.stage_valid = false;
            b.stage_pool = -1;
            b.last_time = 0.0f;   // Let the next sweep re-stage.
            continue;
        }

        if (!kh_band_view_native(g_ls.frame_view, true)) {
            khsc_any = true;
            continue;
        }
        // A mixed stage can never become coherent with any future publish, so it drops via the aged-drop mechanics and
        // the next sweep re-stages clean.
        if (b.stage_view_ok &&
            !kh_band_pair_coherent(b.stage_view, g_ls.frame_view)) {
            g_band_pool[khsc_p].busy = false;
            b.stage_valid = false;
            b.stage_pool = -1;
            b.last_time = 0.0f;
            continue;
        }
        // The front band adopts the stage's matrices only here, once the stage is known to land: adopting above the two
        // tests leaves a refused stage's sm on a band whose texture never swapped.
        memcpy(b.sm, b.stage_sm, sizeof(b.sm));
        memcpy(b.border, b.stage_border, sizeof(b.border));

        if (b.stage_first_ok) {
            memcpy(b.vcol, b.stage_first, sizeof(b.vcol));
        } else {
            for (int c = 0; c < 3; ++c) {
                for (int r = 0; r < 4; ++r) {
                    b.vcol[c * 4 + r] = g_ls.frame_view[r * 4 + c];
                }
            }
        }

        // Swap, do not copy: the back buffer was filled at the resolve that paired this sm. The retiring front texture
        // becomes the pool's next back buffer.
        {
            ID3D11Texture2D* khsc_t = b.tex;
            ID3D11ShaderResourceView* khsc_s = b.srv;
            b.tex = g_band_pool[khsc_p].tex;
            b.srv = g_band_pool[khsc_p].srv;
            g_band_pool[khsc_p].tex = khsc_t;
            g_band_pool[khsc_p].srv = khsc_s;
            g_band_pool[khsc_p].busy = false;
            b.stage_pool = -1;
        }

        b.pending_view = false;
        b.vcol_bridge = false;
        b.stage_valid = false;
        b.stage_view_ok = false;
        b.stage_pair_ok = false;
        b.stage_first_ok = false;
        b.last_time = khsc_now;
        if (g_ls.first_capture_time < 0.0f) g_ls.first_capture_time = khsc_now;
    }

    g_ls.stage_any = khsc_any;
}
}