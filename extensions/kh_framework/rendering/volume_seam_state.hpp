#pragma once

// The injection half (rendering_integration_injection.hpp): the volume seam (KH_SVS): its targets, the footprint, the
// stencil reprojection and the scene depth pair. Never include it on its own.
namespace RenderIntegration {
inline void kh_vmir_release();   // KH_VOL_MIRROR (defined with its statics).

inline void kh_rp_release_if_held();   // KH_SHADOW_OFF: defined with the replay's records.
inline void kh_rp_release_dead();   // KH_RP_DEAD_RELEASE: likewise.

// Defined in rendering/replay_state.hpp (declared here for the uses below).
inline bool kh_rp_use_merged();
inline ID3D11ShaderResourceView* kh_svs_foot_bound();



// Seam-target stability. The set is tiny (a full set takes no new target), cleared only by kh_svs_mask_release (a
// device release, an MSAA sample-count change) and the session reset, and holds identity tokens that are compared and
// never dereferenced.
static void*    g_svs_rtv_seen[8] = {};
static uint32_t g_svs_rtv_seen_n = 0;
// Accepted limitation: this engine records on deferred contexts, so a prepass bind landing on one would go unmarked.
// Every tested session marks its prepass through the immediate context.
static void*    g_svs_cls_id[8] = {};
static uint8_t  g_svs_cls_srv[8] = {};
static uint32_t g_svs_cls_n = 0;
// The injection is deferred out of the OM hook body.
static bool     g_svs_pending = false;
static uint32_t g_svs_pend_w = 0;
static uint32_t g_svs_pend_h = 0;
static void*    g_svs_mask_id = nullptr;
static ID3D11ShaderResourceView* g_svs_mask_srv = nullptr;
static uint32_t g_svs_mask_w = 0;
static uint32_t g_svs_mask_h = 0;
static uint32_t g_svs_mask_fmt = 0;
// Mask capture is decoupled from injection.
static ID3D11Texture2D* g_svs_mask_cand = nullptr;
// The mask has two writers - eight cascade resolves and one stencil-volume resolve - and only the second knows about
// our box (our injection writes depth only; the cascade half is resolved against the linear-depth target we do not
// touch). The pre-resolve snapshot.
static ID3D11Texture2D*          g_svs_pre_tex = nullptr;
static ID3D11ShaderResourceView* g_svs_pre_srv = nullptr;
static uint32_t g_svs_pre_w = 0;
static uint32_t g_svs_pre_h = 0;
static uint32_t g_svs_pre_fmt = 0;
static ID3D11Texture2D*          g_svs_post_tex = nullptr;
static ID3D11ShaderResourceView* g_svs_post_srv = nullptr;
static uint64_t g_svs_post_made = 0;
// KH_SVS_SKIP (KH_SVS_SKIP_NOTE): the signature of a world seam pass's inputs - target, clear serial, view, both caster
// lists with their alpha, material, verdict and blend flag, and the maps' readiness (KH_TEX_LAND); a pass matching the
// last one marked completed is skipped. It never matches a pass that drew: a world seam that draws marks the frame
// (g_svs_injected_frame - no second seam before the main-depth clear) and every engine depth clear moves the serial;
// replay B never skips, and a withheld pass marks none. It matches only a re-bind, in the same clear span, after a pass
// whose frame constant upload failed (khv_frame_ok false: no footprint drawn, yet marked completed) - that retry is
// skipped, so the frame keeps no footprint. A retry would redraw the mirror prepass, clearing any counts re-issued into
// it since.
static uint64_t g_depth_clear_serial = 0;
static uint64_t g_svs_skip_sig = 0;         // Signature of the last completed inject.
static bool     g_svs_skip_valid = false;
static uint64_t g_svs_frame_seq = 0;   // Bumped once per seam frame boundary.
static uint8_t  g_svs_bracket = 0;
// Only the footprint loop reads these: the mirror prepass and the prime draw the list whole.
struct KhSvCaster {
    float pos[3]; float size[3]; float rot[9]; bool rotated; int mesh;
    float res[3];   // KH_POS_RES (kh_pos_res at the build).
    float bsize[3];   // KH_CAST_REFIT: the cull's size (kh_bounds_size_of); size draws.
    int  lod;        // KH_SHADOW_LOD_DRAWN: the level the footprint draws (the injection's own pick,
                     // taken once the pass camera is known, below the list build).
    bool lod_lock;   // KH_LOD_LOCK, carried for that pick.
    float lod_r;     // KH_SKEL_LOD_R: kh_lod_radius at the build, that pick's radius.
    // Inside the footprint's own frustum (the pass camera's, padded sphere): the footprint loop is a screen-space
    // depth write through that matrix, so an object outside its frustum contributes nothing there.
    bool a_vis;
    // The live-scene slot this caster was built from - the index the instanced entry points read khObjs at. The record
    // carries centre, extents and rotation, so the lane needs nothing else.
    uint32_t slot;
    float alpha;
    const KhMaterialSet* mats;   // Pool-owned (KH_MAT_POOL); the grave wall outlives this list.
    bool alpha_caster;
    // KH_FOOT_BLEND: a non-normal blend mode (additive, multiply, screen, lighten, darken) combines with what is behind
    // it, so the world behind shows through at every texel, full alpha or not: the engine's count there belongs to what
    // is behind. Not in the footprint or its mask (the mirror prepass still draws it, as it draws the whole translucent
    // ones); its own receivers read no witness (PSMain / PSComposite, bm != 0).
    bool no_foot;
    bool two_sided;   // KH_SEAM_FACE: the object's twoSided - which faces its colour pass draws.
};

inline bool kh_footprint_alpha_on() {
    return g_res.ps_inj_depth_a && g_res.vs_tex && g_res.layout_tex;
}
static std::vector<KhSvCaster> g_svs_caster_list;
// The seam's instance plan, kept across frames so the two loops never allocate. g_svs_inst_ord holds indices into the
// caster list, mesh-major: [0, inst_a) is loop A's opaque footprint set and [inst_a, inst_a + inst_b) is loop B's
// (every caster). The lanes are written into the mapped buffer in this order, so the index into this array IS the
// StartInstanceLocation.
static std::vector<uint32_t>   g_svs_inst_ord;
static float    g_svs_prime_vp[4][4] = {};

static float    g_khpe_p1 = -1.0f;   // Predictor base (spike-guarded committed near).
static float    g_khpe_p2 = -1.0f;   // Its predecessor (-1 = slope unknown).
static float    g_khpe_m22 = 0.0f;
static uint64_t g_khpe_ms = 0;   // g_inj_enc_ms of the last intaken commit.
static uint64_t g_khpe_upd_ms = 0;   // Steady ms of the last base update (freshness).
static float    g_khpe_skip1 = -1.0f;   // Last spike-skipped commit (the rebase escape).
static float    g_khbl_stable = -1.0f;   // Stable committed baseline (2 consecutive within 2%).
static float    g_khbl_prev = -1.0f;
// The live take's trackers (kh_svs_take_pv): its previous take, its step
// history and its references. They die with the session.
static float    g_khlt_prev[3] = {};
static bool     g_khlt_prev_ok = false;
static float    g_khlt_prev_step = 0.0f;
static float    g_khlt_step_ring[4] = {};
static uint32_t g_khlt_step_ring_i = 0;
static float    g_khlt_ref_cam[3] = {};   // Last bound-refused live camera.
static bool     g_khlt_ref_ok = false;

static float    g_svs_prime_cam[3] = {};
static bool     g_svs_prime_rebase = false;
static float    g_svs_prime_vp_lo = 0.0f;
static float    g_svs_prime_vp_hi = 1.0f;
static ID3D11Texture2D*          g_svs_vol_src = nullptr;
static void*                     g_svs_vol_src_id = nullptr;
static ID3D11Texture2D*          g_svs_vol_tex = nullptr;
static ID3D11ShaderResourceView* g_svs_vol_sten_srv = nullptr;
static uint32_t g_svs_vol_w = 0;
static uint32_t g_svs_vol_h = 0;
static bool     g_svs_vol_primed = false;
// Live epoch latch.
static uint64_t g_svs_vol_seq = 0;
// KH_VOL_WITNESS: the copy's depth plane (t23), and the encode that wrote it. A mesh reads the copy a frame late (the
// injection precedes the engine's counting draws), and at a pixel its surface did not cover then the count belongs to
// whatever did - the engine body behind a swinging cape, in its own shadow. The witness keeps a count only where the
// copy's depth is this fragment's surface. The encode (m22, m32, viewport min, max) is the seam's, taken where it draws
// the footprint the engine counts against, and latched with the copy of the same seam frame; a copy with no seam behind
// it carries none and the witness stands down.
static ID3D11ShaderResourceView* g_svs_vol_depth_srv = nullptr;
static float    g_svs_seam_enc[4] = {};
static uint64_t g_svs_seam_enc_seq = ~0ull;   // The g_svs_frame_seq it was taken in.
static float    g_svs_vol_enc[4] = {};
static bool     g_svs_vol_enc_ok = false;
// KH_VOL_ZOOM: the seam frame's projection terms (P00, P11, P20, P21, each over P23 - kh_sten_proj_terms), taken with
// the encode and latched with the copy of the same seam frame: the projection the copy's pixels were drawn under.
static float    g_svs_seam_proj[4] = {};
static bool     g_svs_seam_proj_ok = false;
static float    g_svs_vol_proj[4] = {};
static bool     g_svs_vol_proj_ok = false;
// KH_VOL_FOOT: the witness's own record of where our surface was. The copy's depth plane is the engine's volume buffer,
// whose depth the engine builds with its own pass (it is single-sample, the scene's is not); while the near plane moves
// (a zoom, a fast approach) that pass encodes our surface with terms we never see, the decode lands a few percent off,
// and past the 5 cm floor every witness failed - the shadow dropped out. The seam also draws the footprint's view
// distance (m, SV_Position.w) into an R32F mask of its own, no depth test, MIN blend over a 1e30 clear, and the witness
// compares against that. Two masks: the seam draws into the one not published; the copy latch publishes it with the
// copy of the same seam frame.
static ID3D11Texture2D*          g_svs_foot_tex[2] = {};
static ID3D11RenderTargetView*   g_svs_foot_rtv[2] = {};
static ID3D11ShaderResourceView* g_svs_foot_srv[2] = {};
static uint32_t g_svs_foot_w = 0;
static uint32_t g_svs_foot_h = 0;
static int      g_svs_foot_pub = -1;        // The mask the copy owns (bound at t33).
static int      g_svs_foot_seam_ix = -1;    // The mask the last seam drew.
static uint64_t g_svs_foot_seam_seq = ~0ull;   // The seam frame it drew in.
static bool     g_svs_vol_foot_ok = false;  // The published mask is this copy's.
inline void kh_svs_foot_release() {
    for (int khfr_i = 0; khfr_i < 2; ++khfr_i) {
        KH_SAFE_RELEASE(g_svs_foot_srv[khfr_i]);
        KH_SAFE_RELEASE(g_svs_foot_rtv[khfr_i]);
        KH_SAFE_RELEASE(g_svs_foot_tex[khfr_i]);
    }
    g_svs_foot_w = 0;
    g_svs_foot_h = 0;
    g_svs_foot_pub = -1;
    g_svs_foot_seam_ix = -1;
    g_svs_foot_seam_seq = ~0ull;
    g_svs_vol_foot_ok = false;
}
inline bool kh_svs_foot_ensure(ID3D11DeviceContext* khfe_ctx, uint32_t khfe_w, uint32_t khfe_h) {
    if (g_svs_foot_rtv[0] && g_svs_foot_rtv[1] && g_svs_foot_srv[0] && g_svs_foot_srv[1] &&
        g_svs_foot_w == khfe_w && g_svs_foot_h == khfe_h) return true;
    kh_svs_foot_release();
    if (!khfe_ctx || khfe_w == 0 || khfe_h == 0) return false;
    ID3D11Device* khfe_dev = nullptr;
    khfe_ctx->GetDevice(&khfe_dev);
    if (!khfe_dev) return false;
    D3D11_TEXTURE2D_DESC khfe_td = {};
    khfe_td.Width = khfe_w;
    khfe_td.Height = khfe_h;
    khfe_td.MipLevels = 1;
    khfe_td.ArraySize = 1;
    khfe_td.Format = DXGI_FORMAT_R32_FLOAT;   // Metres, not a depth format.
    khfe_td.SampleDesc.Count = 1;
    khfe_td.Usage = D3D11_USAGE_DEFAULT;
    khfe_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool khfe_ok = true;
    for (int khfe_i = 0; khfe_i < 2 && khfe_ok; ++khfe_i) {
        khfe_ok = SUCCEEDED(khfe_dev->CreateTexture2D(&khfe_td, nullptr, &g_svs_foot_tex[khfe_i])) &&
                  SUCCEEDED(khfe_dev->CreateRenderTargetView(g_svs_foot_tex[khfe_i], nullptr, &g_svs_foot_rtv[khfe_i])) &&
                  SUCCEEDED(khfe_dev->CreateShaderResourceView(g_svs_foot_tex[khfe_i], nullptr, &g_svs_foot_srv[khfe_i]));
    }
    khfe_dev->Release();
    if (!khfe_ok) {
        kh_svs_foot_release();
        return false;
    }
    g_svs_foot_w = khfe_w;
    g_svs_foot_h = khfe_h;
    return true;
}
// Deliberate toggle: constant true, kept as a function so the volume transport can be switched off in one place.
// KH_SVS_OFF_UNMAINT: the path it would hand back to - the priming pass (kh_svs_prime_mask), the mask snapshots and
// KhStenTerm - is unmaintained and does not work as it stands: the prime draws through a zeroed frame block (stenVol2.z
// = 0: plain z through g_svs_prime_vp) under rast_sun's bias with depth clip on, while the footprint it must land on
// writes z through the engine's view block (stenVol2.z = 3) under rast_inject, so its LESS_EQUAL test rejects most of
// our own surface. Update it first.
inline bool kh_svs_vol_on() {
    return true;
}

inline void kh_svs_vol_release() {
    if (g_svs_vol_sten_srv) { g_svs_vol_sten_srv->Release(); g_svs_vol_sten_srv = nullptr; }
    if (g_svs_vol_depth_srv) { g_svs_vol_depth_srv->Release(); g_svs_vol_depth_srv = nullptr; }   // KH_VOL_WITNESS.
    g_svs_vol_enc_ok = false;
    g_svs_vol_proj_ok = false;   // KH_VOL_ZOOM.
    kh_svs_foot_release();   // KH_VOL_FOOT: the masks follow the copy's shape and device.
    if (g_svs_vol_tex) { g_svs_vol_tex->Release(); g_svs_vol_tex = nullptr; }
    g_svs_vol_w = 0; g_svs_vol_h = 0;
    g_svs_vol_primed = false;   // Readiness cannot outlive the resources it describes.
}

inline void kh_svs_vol_src_release() {
    if (g_svs_vol_src) { g_svs_vol_src->Release(); g_svs_vol_src = nullptr; }
    g_svs_vol_src_id = nullptr;
}

// The latch runs whenever kh_volume_seam_track recognises the mask target (g_svs_mask_id) and costs one compare while
// the identity stands; a new identity, or a source released since (kh_svs_vol_src_release), latches again, and the
// copy's shape follows.
inline void kh_svs_vol_latch(ID3D11DepthStencilView* khl_dsv, void* khl_id) {
    if (!khl_dsv || !khl_id) return;
    if (g_svs_vol_src && g_svs_vol_src_id == khl_id) return;   // Steady state: one compare.
    ID3D11Resource* khl_r = nullptr;
    khl_dsv->GetResource(&khl_r);
    if (!khl_r) return;
    ID3D11Texture2D* khl_t = nullptr;
    khl_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khl_t));
    khl_r->Release();
    if (!khl_t) return;
    if (g_svs_vol_src) { g_svs_vol_src->Release();  }
    g_svs_vol_src = khl_t;
    g_svs_vol_src_id = khl_id;
    kh_svs_vol_release();   // The copy's shape follows the source's.
}

// The copy destination, created from the source's own desc. Rebuilt on any shape change, released on device reset.
inline bool kh_svs_vol_ensure(ID3D11Device* khe_dev) {
    if (!khe_dev || !g_svs_vol_src) return false;
    D3D11_TEXTURE2D_DESC khe_sd = {};
    g_svs_vol_src->GetDesc(&khe_sd);

    if (g_svs_vol_tex && g_svs_vol_sten_srv &&
        g_svs_vol_w == khe_sd.Width && g_svs_vol_h == khe_sd.Height) {
        return true;
    }

    kh_svs_vol_release();

    // MSAA would force Texture2DMS in the shader and forbid the copy to a
    // non-MS destination. Refused (silently: no copy is made).
    if (khe_sd.SampleDesc.Count != 1) {  return false; }
    if (khe_sd.ArraySize != 1 || khe_sd.MipLevels != 1) {  return false; }

    if (khe_sd.Format != DXGI_FORMAT_D24_UNORM_S8_UINT &&
        khe_sd.Format != DXGI_FORMAT_R24G8_TYPELESS &&
        khe_sd.Format != DXGI_FORMAT_R24_UNORM_X8_TYPELESS &&
        khe_sd.Format != DXGI_FORMAT_X24_TYPELESS_G8_UINT) {
        return false;
    }

    D3D11_TEXTURE2D_DESC khe_td = khe_sd;
    khe_td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    khe_td.Usage = D3D11_USAGE_DEFAULT;
    khe_td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    khe_td.CPUAccessFlags = 0;
    khe_td.MiscFlags = 0;

    if (FAILED(khe_dev->CreateTexture2D(&khe_td, nullptr, &g_svs_vol_tex))) {
        g_svs_vol_tex = nullptr;
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC khe_vd = {};
    khe_vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    khe_vd.Texture2D.MipLevels = 1;
    khe_vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;   // The stencil plane (g).

    if (FAILED(khe_dev->CreateShaderResourceView(g_svs_vol_tex, &khe_vd,
                                                 &g_svs_vol_sten_srv))) {
        g_svs_vol_sten_srv = nullptr;
        kh_svs_vol_release();
        return false;
    }
    // KH_VOL_WITNESS: the depth plane (r). Optional: without it the witness stands down (kh_fill_sten_reproj) and
    // the count is read as it stands.
    khe_vd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    if (FAILED(khe_dev->CreateShaderResourceView(g_svs_vol_tex, &khe_vd,
                                                 &g_svs_vol_depth_srv))) {
        g_svs_vol_depth_srv = nullptr;
    }

    g_svs_vol_w = khe_td.Width;
    g_svs_vol_h = khe_td.Height;
    return true;
}

inline bool kh_svs_vol_ready() {
    if (g_svs_vol_sten_srv == nullptr) return false;
    if (!g_svs_vol_primed) {  return false; }
    // No same-frame gate here, deliberately: the copy runs at the engine's stencil-mask RTV bind and the reader is
    // inject_composited_meshes off the opaque trigger - opposite sides of the frame. Cross-frame consumption is the
    // transport's contract (g_svs_vol_primed is sticky; kh_svs_vol_copy's no-mesh-wanted guard drops it). Do not add
    // a same-frame gate: it silently drops maskMeta.w, the stenVol2.x arm and the t24 bind whenever the mask bind
    // lands after the trigger. If staleness is ever observed, bound it against a measured epoch lag, not equality.

    if (!g_main_depth_w || !g_main_depth_h ||
        g_svs_vol_w != static_cast<uint32_t>(g_main_depth_w) ||
        g_svs_vol_h != static_cast<uint32_t>(g_main_depth_h)) {
        return false;
    }

    return true;
}

static bool     g_svs_prime_ready = false;

inline void kh_svs_mask_swap_release() {
    if (g_svs_mask_srv) { g_svs_mask_srv->Release(); g_svs_mask_srv = nullptr; }
    g_svs_mask_id = nullptr;
    g_svs_mask_w = 0; g_svs_mask_h = 0; g_svs_mask_fmt = 0;
}

// The latched mask candidate holds an AddRef'd texture between the bind that latched it and the frame boundary that
// consumes it; it must be released here or a device reset keeps a dead device alive.
inline void kh_svs_mask_release(bool khm_scrub_vmir) {
    kh_svs_mask_swap_release();
    // The volume transport's too: the copy, both views and the AddRef'd source.
    kh_svs_vol_release();
    if (khm_scrub_vmir) kh_vmir_release();
    kh_svs_vol_src_release();
    if (g_svs_mask_cand) { g_svs_mask_cand->Release(); g_svs_mask_cand = nullptr; }
    if (g_svs_pre_srv) { g_svs_pre_srv->Release(); g_svs_pre_srv = nullptr; }
    if (g_svs_pre_tex) { g_svs_pre_tex->Release(); g_svs_pre_tex = nullptr; }
    if (g_svs_post_srv) { g_svs_post_srv->Release(); g_svs_post_srv = nullptr; }
    if (g_svs_post_tex) { g_svs_post_tex->Release(); g_svs_post_tex = nullptr; }
    g_svs_pre_w = 0; g_svs_pre_h = 0; g_svs_pre_fmt = 0;
    g_svs_bracket = 0;
    g_svs_rtv_seen_n = 0;   // Weak identities die with the device.
    g_svs_cls_n = 0;
}

inline bool kh_svs_sten_ready() {
    if (g_svs_mask_srv == nullptr || g_svs_pre_srv == nullptr ||
        g_svs_post_srv == nullptr) {
        return false;
    }

    if (g_svs_post_made == 0) {  return false; }

    if (!g_main_depth_w || !g_main_depth_h ||
        g_svs_mask_w != static_cast<uint32_t>(g_main_depth_w) ||
        g_svs_mask_h != static_cast<uint32_t>(g_main_depth_h)) {
        return false;
    }

    return true;
}

// Demand gate: deliberate toggle, constant true, kept as a function so the feature can be switched off in one place.
// Gated on the mode, not on readiness: the snapshot textures are created inside kh_svs_pre_ensure, which only runs from
// kh_svs_mask_snap, so a readiness gate would never open.
inline bool kh_svs_feature_on() {
    return true;
}

// The two artifacts get separate arms: the multiply reads a mask our depth
// never touched, so injection, snapshots and multiply arm independently.
inline bool kh_svs_inject_armed() {
    if (kh_shadows_off()) return false;   // KH_SHADOW_OFF: nothing reads what the seam and the pass it arms serve.
    if (kh_svs_feature_on()) return true;
    return   // A mode must be wired into every site it needs.
           false;
}

inline bool kh_svs_snap_wanted() {
    // Only our mesh draws read the snapshots (t21 / t22), so a frame with no visible
    // mesh copies nothing (the volume copy is gated the same way).
    if (kh_shadows_off()) return false;   // KH_SHADOW_OFF: no reader.
    if (kh_svs_feature_on()) return g_svs_mesh_wanted.load(std::memory_order_relaxed);
    return false;
}

// The snapshot ratio's readiness (KhStenTerm, the t21 / t22 binds). maskMeta.w is kh_svs_unit_on's, which takes this
// only with the volume transport off (kh_svs_vol_on).
inline bool kh_svs_sten_on() {
    if (!kh_svs_sten_ready()) return false;
    // Off with the feature toggle (kh_svs_feature_on), as the injection's arm is (kh_svs_inject_armed).
    if (kh_svs_feature_on()) return true;
    return false;
}

inline bool kh_svs_unit_on() {
    if (kh_svs_vol_on()) return kh_svs_vol_ready();
    return kh_svs_sten_on();
}

// Destination for the snapshot: a clone of the mask desc minus the render target bind. Rebuilt whenever the mask's
// shape or format moves (the mask identity is weak).
inline bool kh_svs_clone_one(ID3D11Device* khv_dev, ID3D11Texture2D** khv_tex,
                             ID3D11ShaderResourceView** khv_srv) {
    D3D11_TEXTURE2D_DESC khv_td = {};
    khv_td.Width = g_svs_mask_w;
    khv_td.Height = g_svs_mask_h;
    khv_td.MipLevels = 1;
    khv_td.ArraySize = 1;
    khv_td.Format = static_cast<DXGI_FORMAT>(g_svs_mask_fmt);
    khv_td.SampleDesc.Count = 1;
    khv_td.SampleDesc.Quality = 0;
    khv_td.Usage = D3D11_USAGE_DEFAULT;
    khv_td.BindFlags = D3D11_BIND_SHADER_RESOURCE;   // Read only - never an RT.
    khv_td.CPUAccessFlags = 0;
    khv_td.MiscFlags = 0;

    if (FAILED(khv_dev->CreateTexture2D(&khv_td, nullptr, khv_tex))) {
        *khv_tex = nullptr;
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC khv_sd = {};
    khv_sd.Format = khv_td.Format;
    khv_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    khv_sd.Texture2D.MipLevels = 1;

    if (FAILED(khv_dev->CreateShaderResourceView(*khv_tex, &khv_sd, khv_srv))) {
        *khv_srv = nullptr;
        (*khv_tex)->Release();
        *khv_tex = nullptr;
        return false;
    }

    return true;
}

// Both ends of the bracket, created and dropped together: a ratio with only one snapshot alive is not a ratio, and
// kh_svs_sten_ready refuses on either being null.
inline bool kh_svs_pre_ensure(ID3D11Device* khv_dev) {
    if (!khv_dev || !g_svs_mask_w || !g_svs_mask_h) return false;

    if (g_svs_pre_tex && g_svs_pre_srv && g_svs_post_tex && g_svs_post_srv &&
        g_svs_pre_w == g_svs_mask_w && g_svs_pre_h == g_svs_mask_h &&
        g_svs_pre_fmt == g_svs_mask_fmt) {
        return true;
    }

    if (g_svs_pre_srv) { g_svs_pre_srv->Release(); g_svs_pre_srv = nullptr; }
    if (g_svs_pre_tex) { g_svs_pre_tex->Release(); g_svs_pre_tex = nullptr; }
    if (g_svs_post_srv) { g_svs_post_srv->Release(); g_svs_post_srv = nullptr; }
    if (g_svs_post_tex) { g_svs_post_tex->Release(); g_svs_post_tex = nullptr; }
    g_svs_pre_w = 0; g_svs_pre_h = 0; g_svs_pre_fmt = 0;

    if (!kh_svs_clone_one(khv_dev, &g_svs_pre_tex, &g_svs_pre_srv)) {
        return false;
    }

    if (!kh_svs_clone_one(khv_dev, &g_svs_post_tex, &g_svs_post_srv)) {
        if (g_svs_pre_srv) { g_svs_pre_srv->Release(); g_svs_pre_srv = nullptr; }
        if (g_svs_pre_tex) { g_svs_pre_tex->Release(); g_svs_pre_tex = nullptr; }
        return false;
    }

    g_svs_pre_w = g_svs_mask_w;
    g_svs_pre_h = g_svs_mask_h;
    g_svs_pre_fmt = g_svs_mask_fmt;
    return true;
}

// Source is whatever the adopted mask SRV views, so there is no second weak pointer to keep alive.
inline void kh_svs_mask_snap(ID3D11DeviceContext* khv_ctx, bool khv_is_post) {
    if (!khv_ctx || !g_svs_mask_srv) {
        return;
    }

    ID3D11Device* khv_dev = nullptr;
    khv_ctx->GetDevice(&khv_dev);
    const bool khv_ok = khv_dev && kh_svs_pre_ensure(khv_dev);
    if (khv_dev) khv_dev->Release();

    if (!khv_ok) {
        return;
    }

    ID3D11Resource* khv_src = nullptr;
    g_svs_mask_srv->GetResource(&khv_src);

    if (!khv_src) {
        return;
    }

    {
        KhPredAside khv_pa;   // KH_PRED_ASIDE: the seam's mask snapshot, not skipped by the engine's predicate.
        khv_pa.begin(khv_ctx);
        khv_ctx->CopyResource(khv_is_post ? g_svs_post_tex : g_svs_pre_tex, khv_src);
    }
    khv_src->Release();

    if (khv_is_post) {
        g_svs_post_made++;
    }
}
static bool     g_svs_vol_dsv_now = false;
// KH_INFRONT: the volume buffer is the bound DSV, a render target beside it or
// not (the engine's hands prepass into it binds one). Set at every OM bind.
static bool     g_svs_vol_dsv_bound = false;
static bool     g_svs_injected_frame = false;
inline void kh_volume_seam_frame_reset() {
    g_svs_injected_frame = false;
    if (g_svs_pending) { g_svs_pending = false;  }
    // A copy armed at a resolve bind that no draw followed.
    if (g_svs_bracket != 0) { g_svs_bracket = 0;  }
    g_svs_vol_dsv_now = false;
    g_svs_prime_ready = false;
    g_svs_frame_seq++;   // Epoch clock for the snapshot age.
    kh_rp_release_dead();   // KH_RP_DEAD_RELEASE: after the epoch moved.
    // KH_SHADOW_OFF: every frame shadows are off, nothing of the stencil path stays ready for their return - the
    // volume copy unprimed, the snapshots unmade, the mirror absent (KH_MIR_GATE's rule for a skipped mirror: every
    // reader then takes none rather than an older one) - and the last recorded pass lets its engine buffers go
    // (the epoch above has moved, so no pass is open). Here and not at the engine's mask bracket, which a frame
    // without the engine's shadow passes never reaches.
    if (kh_shadows_off()) {
        g_svs_vol_primed = false;
        g_svs_post_made = 0;
        g_vmir_mask_time = -1.0f;
        kh_rp_release_if_held();
    }
    if (g_svs_mask_cand) {
        void* khv_cid = static_cast<void*>(g_svs_mask_cand);

        if (khv_cid != g_svs_mask_id) {
            D3D11_TEXTURE2D_DESC khv_cd = {};
            g_svs_mask_cand->GetDesc(&khv_cd);
            ID3D11Device* khv_cdev = nullptr;
            g_svs_mask_cand->GetDevice(&khv_cdev);

            if (khv_cdev) {
                // The swap release, not the teardown.
                kh_svs_mask_swap_release();
                D3D11_SHADER_RESOURCE_VIEW_DESC khv_sd = {};
                khv_sd.Format = khv_cd.Format;
                khv_sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                khv_sd.Texture2D.MipLevels = 1;

                if (SUCCEEDED(khv_cdev->CreateShaderResourceView(
                        g_svs_mask_cand, &khv_sd, &g_svs_mask_srv))) {
                    g_svs_mask_id = khv_cid;
                    g_svs_mask_w = khv_cd.Width;
                    g_svs_mask_h = khv_cd.Height;
                    g_svs_mask_fmt = static_cast<uint32_t>(khv_cd.Format);
                } else {
                    g_svs_mask_srv = nullptr;
                }

                khv_cdev->Release();
            }
        }

        g_svs_mask_cand->Release();
        g_svs_mask_cand = nullptr;
    }
    g_svs_cls_n = 0;   // Classification cache is per frame too.
}

inline bool kh_svs_prime_states(ID3D11Device* khp_dev) {
    if (!khp_dev || g_mask.prime_states_failed) return false;

    // Ahead of the early-out below: the states are created once and this must still get its chance on that first call.
    if (!g_mask.prime_occl_failed && !g_mask.prime_occl[0]) {
        D3D11_QUERY_DESC khp_qd = {};
        khp_qd.Query = D3D11_QUERY_OCCLUSION;

        for (int khp_i = 0; khp_i < 3; ++khp_i) {
            if (FAILED(khp_dev->CreateQuery(&khp_qd, &g_mask.prime_occl[khp_i]))) {
                for (int khp_j = 0; khp_j < 3; ++khp_j) {
                    if (g_mask.prime_occl[khp_j]) {
                        g_mask.prime_occl[khp_j]->Release();
                        g_mask.prime_occl[khp_j] = nullptr;
                    }
                    g_mask.prime_occl_inflight[khp_j] = false;
                }
                g_mask.prime_occl_failed = true;
                break;
            }
        }
    }

    if (g_mask.prime_blend && g_mask.prime_dss) return true;

    if (!g_mask.prime_dss) {
        D3D11_DEPTH_STENCIL_DESC khp_dd = {};
        khp_dd.DepthEnable = TRUE;
        khp_dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        khp_dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        khp_dd.StencilEnable = FALSE;

        if (FAILED(khp_dev->CreateDepthStencilState(&khp_dd, &g_mask.prime_dss))) {
            g_mask.prime_states_failed = true;
            return false;
        }
    }

    if (!g_mask.prime_blend) {
        D3D11_BLEND_DESC khp_bd = {};
        khp_bd.RenderTarget[0].BlendEnable = FALSE;
        khp_bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;

        if (FAILED(khp_dev->CreateBlendState(&khp_bd, &g_mask.prime_blend))) {
            g_mask.prime_states_failed = true;
            return false;
        }
    }

    return g_mask.prime_blend != nullptr && g_mask.prime_dss != nullptr;
}

inline bool kh_svs_prime_wanted() {
    // The volume transport (kh_svs_vol_on, constant true) disables the priming pass (cascade cut absent).
    if (kh_svs_vol_on()) return false;
    return kh_svs_snap_wanted();
}

// KH_VOL_ZOOM: a perspective projection's screen terms, row-vector convention: ndc.x = (x / z) * P00' + P20', ndc.y =
// (y / z) * P11' + P21', with ' = over P23 (the w column). False for anything else (a skewed or affine projection): the
// map then stands down to the identity.
inline bool kh_sten_proj_terms(const float khpt_p[4][4], float khpt_out[4]) {
    const float khpt_w = khpt_p[2][3];
    if (!(khpt_w > 1.0e-6f) || !(fabsf(khpt_p[3][3]) < 1.0e-6f) ||
        !(fabsf(khpt_p[0][1]) < 1.0e-6f) || !(fabsf(khpt_p[1][0]) < 1.0e-6f) ||
        !(fabsf(khpt_p[3][0]) < 1.0e-6f) || !(fabsf(khpt_p[3][1]) < 1.0e-6f) ||
        !(fabsf(khpt_p[0][3]) < 1.0e-6f) || !(fabsf(khpt_p[1][3]) < 1.0e-6f)) return false;
    khpt_out[0] = khpt_p[0][0] / khpt_w;
    khpt_out[1] = khpt_p[1][1] / khpt_w;
    khpt_out[2] = khpt_p[2][0] / khpt_w;
    khpt_out[3] = khpt_p[2][1] / khpt_w;
    for (int khpt_i = 0; khpt_i < 4; ++khpt_i) {
        if (!(khpt_out[khpt_i] == khpt_out[khpt_i]) || fabsf(khpt_out[khpt_i]) > 1.0e6f) return false;
    }
    return fabsf(khpt_out[0]) > 1.0e-6f && fabsf(khpt_out[1]) > 1.0e-6f;
}

// KH_VOL_ZOOM: the raster map (HLSL stenVol3) from a pass drawing under khzm_proj onto the volume copy's seam-frame
// raster - copy = raster * xy + zw per axis - or 0 (the identity) with no measured projection on either side, or
// one of the other handedness. Every lookup into an image drawn in the copy's raster takes it: the count (t24,
// KhVolTerm) and the mirror (t28, KhMirUnit; KH_MIR_ZOOM). Equal projections give s = 1 and o = 0 exactly.
inline void kh_sten_zoom_map(float khzm_out[4], const float (*khzm_proj)[4]) {
    khzm_out[0] = khzm_out[1] = khzm_out[2] = khzm_out[3] = 0.0f;
    float khs_now[4];
    if (g_svs_vol_proj_ok && khzm_proj && g_svs_vol_w > 0 && g_svs_vol_h > 0 &&
        kh_sten_proj_terms(khzm_proj, khs_now)) {
        const float khs_sx = g_svs_vol_proj[0] / khs_now[0];
        const float khs_sy = g_svs_vol_proj[1] / khs_now[1];
        const float khs_ox = g_svs_vol_proj[2] - khs_now[2] * khs_sx;
        const float khs_oy = g_svs_vol_proj[3] - khs_now[3] * khs_sy;
        const float khs_w = static_cast<float>(g_svs_vol_w);
        const float khs_h = static_cast<float>(g_svs_vol_h);
        if (khs_sx > 0.0f && khs_sy > 0.0f) {   // Same handedness; a flip is not a zoom.
            khzm_out[0] = khs_sx;
            khzm_out[1] = khs_sy;
            khzm_out[2] = 0.5f * khs_w * (1.0f - khs_sx + khs_ox);
            khzm_out[3] = 0.5f * khs_h * (1.0f - khs_sy - khs_oy);
        }
    }
}

// Reprojection fill, one truth for both CB editions. A per-pixel read offset makes smf itself noisy (a halo on the
// stencil shadow and on our mesh); do not build another mode on this. KH_VOL_WITNESS is not one: it reads the count at
// the fragment's own pixel and only asks whether the copy's surface there is the fragment's (its ring fallback reads
// neighbours that pass the same test). Lanes: sten_vol.xy = the copy's (m22, m32), sten_vol2.yw = its viewport (min,
// max); all zero = the witness stands down.
// khs_proj = the projection this pass draws our meshes under (the injection's
// and the flush's pv.projection); nullptr = no map.
inline void kh_fill_sten_reproj(ConstantData& khs_cbd, const float (*khs_proj)[4]) {
    const bool khs_vol = kh_svs_vol_on() && kh_svs_vol_ready();
    const bool khs_wit = khs_vol && g_svs_vol_depth_srv != nullptr && g_svs_vol_enc_ok;
    khs_cbd.sten_vol[0] = khs_wit ? g_svs_vol_enc[0] : 0.0f;
    khs_cbd.sten_vol[1] = khs_wit ? g_svs_vol_enc[1] : 0.0f;
    khs_cbd.sten_vol[2] = static_cast<float>(g_svs_vol_w);
    khs_cbd.sten_vol[3] = static_cast<float>(g_svs_vol_h);
    khs_cbd.sten_vol2[0] = khs_vol ? 1.0f : 0.0f;
    khs_cbd.sten_vol2[1] = khs_wit ? g_svs_vol_enc[2] : 0.0f;
    khs_cbd.sten_vol2[3] = khs_wit ? g_svs_vol_enc[3] : 0.0f;
    // KH_VOL_FOOT: armed exactly when the pass binds a mask at t33: the merged one on a merged pass
    // (kh_rp_use_merged, g_rp_mfoot_srv), else the copy's (kh_svs_foot_bound).
    khs_cbd.sten_vol4[0] = (khs_vol && (kh_rp_use_merged() || kh_svs_foot_bound())) ? 1.0f : 0.0f;   // KH_VOL_REPLAY.
    khs_cbd.sten_vol4[1] = khs_cbd.sten_vol4[2] = khs_cbd.sten_vol4[3] = 0.0f;
    // KH_VOL_ZOOM: ndc_copy = ndc_pass * s + o per axis (s = the copy's scale over the pass's, o = the copy's offset
    // less the pass's times s), carried to pixels: the copy and this pass share the target's size (the lookup's own
    // premise).
    // Measured projections only - no motion is assumed. KH_VOL_REPLAY: a merged frame maps the same way: replay B
    // redraws the copy's pass in the copy's raster (its footprint under the cycle latch, or under the copy's own
    // projection terms where the two part - KH_REPLAY_RASTER), while this pass draws under khs_proj, whose scale the
    // injection takes from the live fetch - the two differ while the view zooms.
    kh_sten_zoom_map(khs_cbd.sten_vol3, khs_vol ? khs_proj : nullptr);
}

// KH_VOL_WITNESS: the scene depth's encode pair, by the flush guard's precedence (the pass projection, then the
// measured engine pair, then the live slot pair when its near and far are sane).
inline void kh_scene_depth_pair(const float khsp_proj[4][4], float& khsp_m22, float& khsp_m32) {
    khsp_m22 = khsp_proj[2][2];
    khsp_m32 = khsp_proj[3][2];
    if (g_ro.engine_proj_valid) {
        khsp_m22 = g_ro.engine_m22;
        khsp_m32 = g_ro.engine_m32;
    }
    if (g_ro.slot_near_live >= KH_CAM_NEAR_MIN &&
        g_ro.slot_near_live <= KH_CAM_NEAR_MAX &&
        kh_enc_far(g_ro.slot_m22, g_ro.slot_m32) <= KH_ENC_FAR_MAX) {
        khsp_m22 = g_ro.slot_m22;
        khsp_m32 = g_ro.slot_m32;
    }
}
}