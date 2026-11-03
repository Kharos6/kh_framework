#pragma once

// The injection half (rendering_integration_injection.hpp): the shadow band staging pool and its commit. Never include
// it on its own.
namespace RenderIntegration {

// One staging back buffer per band: with fewer, per-frame reseals fill the pool at the first resolves and the other
// bands keep a seal whose window no longer matches their capture. A seal taken on a view one frame apart is offset by
// exactly that camera step; absent beats offset. The cold escape hatch lets a slot take its first seal on a dead view
// source.
static constexpr int BAND_STAGE_POOL = 8;
inline int band_pool_n() {
    return BAND_STAGE_POOL;
}

struct BandStageTex {
    ID3D11Texture2D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    bool                      busy = false;
};

static BandStageTex g_band_pool[BAND_STAGE_POOL];

// Also clears every slot's staging: a staged capture whose back buffer just
// died must not commit. Called on atlas desc change and device reset.
inline void band_pool_release() {
    for (int k = 0; k < BAND_STAGE_POOL; ++k) {
        if (g_band_pool[k].srv) { g_band_pool[k].srv->Release(); g_band_pool[k].srv = nullptr; }
        if (g_band_pool[k].tex) { g_band_pool[k].tex->Release(); g_band_pool[k].tex = nullptr; }
        g_band_pool[k].busy = false;
    }

    for (int b = 0; b < 8; ++b) {
        g_ls.band[b].stage_valid = false;
        g_ls.band[b].stage_pool = -1;
    }

    g_ls.stage_any = false;
}

// The view a copy of the adopted atlas is read through - its typeless format's depth plane, as the atlas's own view
// (shadow_live_ensure_srv) - or UNKNOWN for a format adoption refuses. Every band and staging copy takes it: a copy
// is made from the atlas's desc (CopyResource demands that), so an R32_FLOAT view fits only an R32 atlas.
inline DXGI_FORMAT atlas_view_fmt(uint32_t fmt) {
    switch (static_cast<DXGI_FORMAT>(fmt)) {
        case DXGI_FORMAT_R32_TYPELESS:      return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:    return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_R16_TYPELESS:      return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        default:                            return DXGI_FORMAT_UNKNOWN;
    }
}
// Desc comes from the live atlas; non-fatal, a failed create leaves the slot null and the miss path stands.
inline void band_pool_ensure_n(int want) {
    if (!g_ls.atlas_tex) return;
    if (want > BAND_STAGE_POOL) want = BAND_STAGE_POOL;
    bool missing = false;
    for (int k = 0; k < want; ++k) {
        if (!g_band_pool[k].tex) { missing = true; break; }
    }
    if (!missing) return;
    D3D11_TEXTURE2D_DESC td = {};
    g_ls.atlas_tex->GetDesc(&td);
    const DXGI_FORMAT vf = atlas_view_fmt(g_ls.atlas_fmt);
    if (vf == DXGI_FORMAT_UNKNOWN) return;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;
    ID3D11Device* dev = nullptr;
    g_ls.atlas_tex->GetDevice(&dev);
    if (!dev) return;
    for (int k = 0; k < want; ++k) {
        if (g_band_pool[k].tex) continue;
        if (SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_band_pool[k].tex)) &&
            g_band_pool[k].tex) {
            D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
            sd.Format = vf;   // The atlas's own format (a band is its copy).
            sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            sd.Texture2D.MipLevels = 1;
            if (FAILED(dev->CreateShaderResourceView(g_band_pool[k].tex,
                                                     &sd, &g_band_pool[k].srv))) {
                g_band_pool[k].srv = nullptr;
                g_band_pool[k].tex->Release();
                g_band_pool[k].tex = nullptr;
            }
        }
        g_band_pool[k].busy = false;
    }
    dev->Release();
}

// A candidate seal view whose camera sits farther than LATCH_JUMP_M-class distance from the cycle latch camera is a
// foreign render and must never be committed into a band seal. true = ok.
inline bool band_view_native(const float* v) {
    if (!g_ro.cycle_pv_valid) return true;   // No reference: cannot judge, admit.
    float c[3], r[3];
    if (!view_camera_exact(reinterpret_cast<const float(*)[4]>(v), c)) return true;
    if (!view_camera_exact(g_ro.cycle_pv.view, r)) return true;
    const float dx = c[0] - r[0];
    const float dy = c[1] - r[1];
    const float dz = c[2] - r[2];
    const float d = sqrtf(dx * dx + dy * dy + dz * dz);
    if (!(d == d) || d <= 50.0f) return true;
    return false;
}
// The capture-time view (vcol column layout) against the welding view (row layout): the distance between their
// cameras, -1 when unjudgeable (either camera undecodable, or a NaN distance). No bar here: two callers -
// shadow_view_scan's native-view stage pass sets a stage's stage_pair_ok when the gap passes 0.5 m (same source within
// the 25 ms window is centimetres, cross-source is metres), and band_pair_coherent (below) is the 50 m pair verdict
// (-1 admits).
inline float band_source_gap(const float* vc, const float* fv) {
    float m[4][4] = {};   // CAM_HONEST: the row layout, for the honest inverse.
    for (int j = 0; j < 3; ++j) {
        const float* v = &vc[j * 4];
        const float n = sqrtf(v[0] * v[0] +
                              v[1] * v[1] +
                              v[2] * v[2]);
        if (!(n > 0.9f && n < 1.1f)) return -1.0f;
        for (int i = 0; i < 4; ++i) m[i][j] = v[i];
    }
    m[3][3] = 1.0f;
    float a[3];
    cam_of(m, a);
    float b[3];
    if (!view_camera_exact(reinterpret_cast<const float(*)[4]>(fv), b)) return -1.0f;
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    const float d = sqrtf(dx * dx + dy * dy + dz * dz);
    if (!(d == d)) return -1.0f;
    return d;
}

static uint64_t g_band_fall_flush = ~0ull;   // Flush of the last allowed fall-through.
// The verdict form: true = within 50 m or unjudgeable (admit), false = refuse. The gap's -1 (unjudgeable: either
// camera undecodable, or a NaN distance) admits.
inline bool band_pair_coherent(const float* vc, const float* fv) {
    return !(band_source_gap(vc, fv) > 50.0f);
}
// Gated on a plain bool, so the steady-state cost is one load.
inline void band_stage_commit(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_BAND_COMMIT);   // PROF.
    if (!g_ls.stage_any) return;
    if (!ctx || !g_ls.atlas_tex) return;
    if (!g_ls.frame_view_valid || g_ls.frame_view_time < 0.0f) return;
    // The pairing view is this cycle's publish; a staged capture from this cycle is still waiting for the next one.
    if (g_boundary_t >= 0.0f && g_ls.frame_view_time < g_boundary_t) return;

    const float now = effect_time_seconds();
    bool any = false;

    for (int i = 0; i < 8; ++i) {
        auto& b = g_ls.band[i];
        if (!b.stage_valid) continue;

        const int p = b.stage_pool;
        const bool dead = !b.valid || !b.tex || !b.srv ||
                          p < 0 || p >= BAND_STAGE_POOL ||
                          !g_band_pool[p].tex || !g_band_pool[p].srv;

        if (dead) {   // Wiped underneath us: release and reseal clean.
            if (p >= 0 && p < BAND_STAGE_POOL) g_band_pool[p].busy = false;
            b.stage_valid = false;
            b.stage_pool = -1;
            b.last_time = 0.0f;
            continue;
        }

        if (g_boundary_t >= 0.0f && b.stage_since >= g_boundary_t) {
            // Staged this cycle, always: the engine resolves its cascades at the end of the cycle, after our injection,
            // for the frame that follows, so the commit at the next cycle's first draws pairs that atlas with the next
            // cycle's view. An in-cycle commit pairs it with the CURRENT view and offsets the received shadows.
            any = true;
            continue;
        }

        const bool aged = now - b.stage_since > 0.05f;

        // The family and camera identity of the pairing view are tested below (band_view_native, the pair
        // coherence); no rotation-agreement bar against the bridge PV (it never fired). The rare flick frame that
        // remains is the view publish landing after the injection (the early return above), which no bar can close.

        if (aged) {
            g_band_pool[p].busy = false;
            b.stage_valid = false;
            b.stage_pool = -1;
            b.last_time = 0.0f;   // Let the next sweep re-stage.
            continue;
        }

        if (!band_view_native(g_ls.frame_view)) {
            any = true;
            continue;
        }
        // A mixed stage can never become coherent with any future publish, so it drops via the aged-drop mechanics and
        // the next sweep re-stages clean.
        if (b.stage_view_ok &&
            !band_pair_coherent(b.stage_view, g_ls.frame_view)) {
            g_band_pool[p].busy = false;
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
            ID3D11Texture2D* t = b.tex;
            ID3D11ShaderResourceView* s = b.srv;
            b.tex = g_band_pool[p].tex;
            b.srv = g_band_pool[p].srv;
            g_band_pool[p].tex = t;
            g_band_pool[p].srv = s;
            g_band_pool[p].busy = false;
            b.stage_pool = -1;
        }

        b.pending_view = false;
        b.vcol_bridge = false;
        b.stage_valid = false;
        b.stage_view_ok = false;
        b.stage_pair_ok = false;
        b.stage_first_ok = false;
        b.last_time = now;
        if (g_ls.first_capture_time < 0.0f) g_ls.first_capture_time = now;
    }

    g_ls.stage_any = any;
}
}