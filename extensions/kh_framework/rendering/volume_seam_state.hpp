#pragma once

// The injection half (rendering_integration_injection.hpp): the volume seam (SVS): its targets, the footprint, the
// stencil reprojection and the scene depth pair. Never include it on its own.
namespace RenderIntegration {
inline void vmir_release();   // VOL_MIRROR (defined with its statics).

inline void rp_release_if_held();   // SHADOW_OFF: defined with the replay's records.
inline void rp_release_dead();   // RP_DEAD_RELEASE: likewise.

// Defined in rendering/replay_state.hpp (declared here for the uses below).
inline bool rp_use_merged();
inline ID3D11ShaderResourceView* svs_foot_bound();



// Only the footprint loop reads these: the mirror prepass and the prime draw the list whole.
struct SvCaster {
    float pos[3]; float size[3]; float rot[9]; bool rotated; int mesh;
    float res[3];   // POS_RES (obj_pos_res at the build).
    float bsize[3];   // CAST_REFIT: the cull's size (bounds_size_of); size draws.
    int  lod;        // SHADOW_LOD_DRAWN: the level the footprint draws (the injection's own pick,
                     // taken once the pass camera is known, below the list build).
    bool lod_lock;   // LOD_LOCK, carried for that pick.
    float lod_r;     // SKEL_LOD_R: lod_radius at the build, that pick's radius.
    // Inside the footprint's own frustum (the pass camera's, padded sphere): the footprint loop is a screen-space
    // depth write through that matrix, so an object outside its frustum contributes nothing there.
    bool a_vis;
    // The live-scene slot this caster was built from - the index the instanced entry points read khObjs at. The record
    // carries centre, extents and rotation, so the lane needs nothing else.
    uint32_t slot;
    float alpha;
    const MaterialSet* mats;   // Pool-owned (MAT_POOL); the grave wall outlives this list.
    bool alpha_caster;
    // FOOT_BLEND: a non-normal blend mode (additive, multiply, screen, lighten, darken) combines with what is behind
    // it, so the world behind shows through at every texel, full alpha or not: the engine's count there belongs to what
    // is behind. Not in the footprint or its mask (the mirror prepass still draws it, as it draws the whole translucent
    // ones); its own receivers read no witness (PSMain / PSComposite, bm != 0).
    bool no_foot;
    bool two_sided;   // SEAM_FACE: the object's twoSided - which faces its colour pass draws.
};
// The volume seam (SVS): the engine shadow-mask targets it watches (the seen RTVs, the class, the mask, the pre-
// and post-resolve snapshots), the caster list and its instance order, the prime pass's frame, the volume copy and its
// stencil, the encode pairs, the footprint.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// The volume copy's depth view (vol_depth_srv, VOL_WITNESS) and the footprint's targets (foot_*, VOL_FOOT) are
// released by svs_vol_release before the session reset nulls them.
// Session reset: session_globals_reset resets every member. session_scratch_reset resets 1 of them.
struct SeamState {
    // Seam-target stability. The set is tiny (a full set takes no new target), cleared only by svs_mask_release (a
    // device release, an MSAA sample-count change) and the session reset, and holds identity tokens that are compared
    // and never dereferenced.
    void*    rtv_seen[8] = {};
    uint32_t rtv_seen_n = 0;
    // Accepted limitation: this engine records on deferred contexts, so a prepass bind landing on one would go
    // unmarked. Every tested session marks its prepass through the immediate context.
    void*    cls_id[8] = {};
    uint8_t  cls_srv[8] = {};
    uint32_t cls_n = 0;
    // The injection is deferred out of the OM hook body.
    bool     pending = false;
    uint32_t pend_w = 0;
    uint32_t pend_h = 0;
    void*    mask_id = nullptr;
    ID3D11ShaderResourceView* mask_srv = nullptr;
    uint32_t mask_w = 0;
    uint32_t mask_h = 0;
    uint32_t mask_fmt = 0;
    // Mask capture is decoupled from injection.
    ID3D11Texture2D* mask_cand = nullptr;
    // The mask has two writers - eight cascade resolves and one stencil-volume resolve - and only the second knows
    // about our box (our injection writes depth only; the cascade half is resolved against the linear-depth target we
    // do not touch). The pre-resolve snapshot.
    ID3D11Texture2D*          pre_tex = nullptr;
    ID3D11ShaderResourceView* pre_srv = nullptr;
    uint32_t pre_w = 0;
    uint32_t pre_h = 0;
    uint32_t pre_fmt = 0;
    ID3D11Texture2D*          post_tex = nullptr;
    ID3D11ShaderResourceView* post_srv = nullptr;
    uint64_t post_made = 0;
    uint64_t skip_sig = 0;         // Signature of the last completed inject.
    bool     skip_valid = false;
    uint64_t frame_seq = 0;   // Bumped once per seam frame boundary.
    uint8_t  bracket = 0;
    std::vector<SvCaster> caster_list;
    // The seam's instance plan, kept across frames so the two loops never allocate. g_svs.inst_ord holds indices into
    // the caster list, mesh-major: [0, inst_a) is loop A's opaque footprint set and [inst_a, inst_a + inst_b) is loop
    // B's (every caster). The lanes are written into the mapped buffer in this order, so the index into this array IS
    // the StartInstanceLocation.
    std::vector<uint32_t>   inst_ord;
    float    prime_vp[4][4] = {};
    float    prime_cam[3] = {};
    bool     prime_rebase = false;
    float    prime_vp_lo = 0.0f;
    float    prime_vp_hi = 1.0f;
    ID3D11Texture2D*          vol_src = nullptr;
    void*                     vol_src_id = nullptr;
    ID3D11Texture2D*          vol_tex = nullptr;
    ID3D11ShaderResourceView* vol_sten_srv = nullptr;
    uint32_t vol_w = 0;
    uint32_t vol_h = 0;
    bool     vol_primed = false;
    // Live epoch latch.
    uint64_t vol_seq = 0;
    // VOL_WITNESS: the copy's depth plane (t23), and the encode that wrote it. A mesh reads the copy a frame late
    // (the injection precedes the engine's counting draws), and at a pixel its surface did not cover then the count
    // belongs to whatever did - the engine body behind a swinging cape, in its own shadow. The witness keeps a count
    // only where the copy's depth is this fragment's surface. The encode (m22, m32, viewport min, max) is the seam's,
    // taken where it draws the footprint the engine counts against, and latched with the copy of the same seam frame; a
    // copy with no seam behind it carries none and the witness stands down.
    ID3D11ShaderResourceView* vol_depth_srv = nullptr;
    float    seam_enc[4] = {};
    uint64_t seam_enc_seq = ~0ull;   // The g_svs.frame_seq it was taken in.
    float    vol_enc[4] = {};
    bool     vol_enc_ok = false;
    // VOL_ZOOM: the seam frame's projection terms (P00, P11, P20, P21, each over P23 - sten_proj_terms), taken
    // with the encode and latched with the copy of the same seam frame: the projection the copy's pixels were drawn
    // under.
    float    seam_proj[4] = {};
    bool     seam_proj_ok = false;
    float    vol_proj[4] = {};
    bool     vol_proj_ok = false;
    // VOL_FOOT: the witness's own record of where our surface was. The copy's depth plane is the engine's volume
    // buffer, whose depth the engine builds with its own pass (it is single-sample, the scene's is not); while the near
    // plane moves (a zoom, a fast approach) that pass encodes our surface with terms we never see, the decode lands a
    // few percent off, and past the 5 cm floor every witness failed - the shadow dropped out. The seam also draws the
    // footprint's view distance (m, SV_Position.w) into an R32F mask of its own, no depth test, MIN blend over a 1e30
    // clear, and the witness compares against that. Two masks: the seam draws into the one not published; the copy
    // latch publishes it with the copy of the same seam frame.
    ID3D11Texture2D*          foot_tex[2] = {};
    ID3D11RenderTargetView*   foot_rtv[2] = {};
    ID3D11ShaderResourceView* foot_srv[2] = {};
    uint32_t foot_w = 0;
    uint32_t foot_h = 0;
    int      foot_pub = -1;        // The mask the copy owns (bound at t33).
    int      foot_seam_ix = -1;    // The mask the last seam drew.
    uint64_t foot_seam_seq = ~0ull;   // The seam frame it drew in.
    bool     vol_foot_ok = false;  // The published mask is this copy's.
    bool     prime_ready = false;
    bool     vol_dsv_now = false;
    // INFRONT: the volume buffer is the bound DSV, a render target beside it or
    // not (the engine's hands prepass into it binds one). Set at every OM bind.
    bool     vol_dsv_bound = false;
    bool     injected_frame = false;
};
static SeamState g_svs;
// SVS_SKIP (SVS_SKIP_NOTE): the signature of a world seam pass's inputs - target, clear serial, view, both caster
// lists with their alpha, material, verdict and blend flag, and the maps' readiness (TEX_LAND); a pass matching the
// last one marked completed is skipped. It never matches a pass that drew: a world seam that draws marks the frame
// (g_svs.injected_frame - no second seam before the main-depth clear) and every engine depth clear moves the serial;
// replay B never skips, and a withheld pass marks none. It matches only a re-bind, in the same clear span, after a pass
// whose frame constant upload failed (frame_ok false: no footprint drawn, yet marked completed) - that retry is
// skipped, so the frame keeps no footprint. A retry would redraw the mirror prepass, clearing any counts re-issued into
// it since.
static uint64_t g_depth_clear_serial = 0;

inline bool footprint_alpha_on() {
    return g_res.ps_inj_depth_a && g_res.vs_tex && g_res.layout_tex;
}


inline void svs_foot_release() {
    for (int i = 0; i < 2; ++i) {
        KH_SAFE_RELEASE(g_svs.foot_srv[i]);
        KH_SAFE_RELEASE(g_svs.foot_rtv[i]);
        KH_SAFE_RELEASE(g_svs.foot_tex[i]);
    }
    g_svs.foot_w = 0;
    g_svs.foot_h = 0;
    g_svs.foot_pub = -1;
    g_svs.foot_seam_ix = -1;
    g_svs.foot_seam_seq = ~0ull;
    g_svs.vol_foot_ok = false;
}
inline bool svs_foot_ensure(ID3D11DeviceContext* ctx, uint32_t w, uint32_t h) {
    if (g_svs.foot_rtv[0] && g_svs.foot_rtv[1] && g_svs.foot_srv[0] && g_svs.foot_srv[1] &&
        g_svs.foot_w == w && g_svs.foot_h == h) return true;
    svs_foot_release();
    if (!ctx || w == 0 || h == 0) return false;
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    if (!dev) return false;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w;
    td.Height = h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32_FLOAT;   // Metres, not a depth format.
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    bool ok = true;
    for (int i = 0; i < 2 && ok; ++i) {
        ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_svs.foot_tex[i])) &&
             SUCCEEDED(dev->CreateRenderTargetView(g_svs.foot_tex[i], nullptr,
                      &g_svs.foot_rtv[i])) &&
             SUCCEEDED(dev->CreateShaderResourceView(g_svs.foot_tex[i], nullptr,
                      &g_svs.foot_srv[i]));
    }
    dev->Release();
    if (!ok) {
        svs_foot_release();
        return false;
    }
    g_svs.foot_w = w;
    g_svs.foot_h = h;
    return true;
}
// Deliberate toggle: constant true, kept as a function so the volume transport can be switched off in one place.
// SVS_OFF_UNMAINT: the path it would hand back to - the priming pass (svs_prime_mask), the mask snapshots and
// KhStenTerm - is unmaintained and does not work as it stands: the prime draws through a zeroed frame block (stenVol2.z
// = 0: plain z through g_svs.prime_vp) under rast_sun's bias with depth clip on, while the footprint it must land on
// writes z through the engine's view block (stenVol2.z = 3) under rast_inject, so its LESS_EQUAL test rejects most of
// our own surface. Update it first.
inline bool svs_vol_on() {
    return true;
}

inline void svs_vol_release() {
    if (g_svs.vol_sten_srv) { g_svs.vol_sten_srv->Release(); g_svs.vol_sten_srv = nullptr; }
    if (g_svs.vol_depth_srv) { g_svs.vol_depth_srv->Release(); g_svs.vol_depth_srv = nullptr; }   // VOL_WITNESS.
    g_svs.vol_enc_ok = false;
    g_svs.vol_proj_ok = false;   // VOL_ZOOM.
    svs_foot_release();   // VOL_FOOT: the masks follow the copy's shape and device.
    if (g_svs.vol_tex) { g_svs.vol_tex->Release(); g_svs.vol_tex = nullptr; }
    g_svs.vol_w = 0; g_svs.vol_h = 0;
    g_svs.vol_primed = false;   // Readiness cannot outlive the resources it describes.
}

inline void svs_vol_src_release() {
    if (g_svs.vol_src) { g_svs.vol_src->Release(); g_svs.vol_src = nullptr; }
    g_svs.vol_src_id = nullptr;
}

// The latch runs whenever volume_seam_track recognises the mask target (g_svs.mask_id) and costs one compare while
// the identity stands; a new identity, or a source released since (svs_vol_src_release), latches again, and the
// copy's shape follows.
inline void svs_vol_latch(ID3D11DepthStencilView* dsv, void* id) {
    if (!dsv || !id) return;
    if (g_svs.vol_src && g_svs.vol_src_id == id) return;   // Steady state: one compare.
    ID3D11Resource* r = nullptr;
    dsv->GetResource(&r);
    if (!r) return;
    ID3D11Texture2D* t = nullptr;
    r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t));
    r->Release();
    if (!t) return;
    if (g_svs.vol_src) { g_svs.vol_src->Release();  }
    g_svs.vol_src = t;
    g_svs.vol_src_id = id;
    svs_vol_release();   // The copy's shape follows the source's.
}

// The copy destination, created from the source's own desc. Rebuilt on any shape change, released on device reset.
inline bool svs_vol_ensure(ID3D11Device* dev) {
    if (!dev || !g_svs.vol_src) return false;
    D3D11_TEXTURE2D_DESC sd = {};
    g_svs.vol_src->GetDesc(&sd);

    if (g_svs.vol_tex && g_svs.vol_sten_srv &&
        g_svs.vol_w == sd.Width && g_svs.vol_h == sd.Height) {
        return true;
    }

    svs_vol_release();

    // MSAA would force Texture2DMS in the shader and forbid the copy to a
    // non-MS destination. Refused (silently: no copy is made).
    if (sd.SampleDesc.Count != 1) {  return false; }
    if (sd.ArraySize != 1 || sd.MipLevels != 1) {  return false; }

    if (sd.Format != DXGI_FORMAT_D24_UNORM_S8_UINT &&
        sd.Format != DXGI_FORMAT_R24G8_TYPELESS &&
        sd.Format != DXGI_FORMAT_R24_UNORM_X8_TYPELESS &&
        sd.Format != DXGI_FORMAT_X24_TYPELESS_G8_UINT) {
        return false;
    }

    D3D11_TEXTURE2D_DESC td = sd;
    td.Format = DXGI_FORMAT_R24G8_TYPELESS;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    if (FAILED(dev->CreateTexture2D(&td, nullptr, &g_svs.vol_tex))) {
        g_svs.vol_tex = nullptr;
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC vd = {};
    vd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    vd.Texture2D.MipLevels = 1;
    vd.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;   // The stencil plane (g).

    if (FAILED(dev->CreateShaderResourceView(g_svs.vol_tex, &vd,
                                             &g_svs.vol_sten_srv))) {
        g_svs.vol_sten_srv = nullptr;
        svs_vol_release();
        return false;
    }
    // VOL_WITNESS: the depth plane (r). Optional: without it the witness stands down (fill_sten_reproj) and
    // the count is read as it stands.
    vd.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    if (FAILED(dev->CreateShaderResourceView(g_svs.vol_tex, &vd,
                                             &g_svs.vol_depth_srv))) {
        g_svs.vol_depth_srv = nullptr;
    }

    g_svs.vol_w = td.Width;
    g_svs.vol_h = td.Height;
    return true;
}

inline bool svs_vol_ready() {
    if (g_svs.vol_sten_srv == nullptr) return false;
    if (!g_svs.vol_primed) {  return false; }
    // No same-frame gate here, deliberately: the copy runs at the engine's stencil-mask RTV bind and the reader is
    // inject_composited_meshes off the opaque trigger - opposite sides of the frame. Cross-frame consumption is the
    // transport's contract (g_svs.vol_primed is sticky; svs_vol_copy's no-mesh-wanted guard drops it). Do not add
    // a same-frame gate: it silently drops maskMeta.w, the stenVol2.x arm and the t24 bind whenever the mask bind
    // lands after the trigger. If staleness is ever observed, bound it against a measured epoch lag, not equality.

    if (!g_main_depth_w || !g_main_depth_h ||
        g_svs.vol_w != static_cast<uint32_t>(g_main_depth_w) ||
        g_svs.vol_h != static_cast<uint32_t>(g_main_depth_h)) {
        return false;
    }

    return true;
}

inline void svs_mask_swap_release() {
    if (g_svs.mask_srv) { g_svs.mask_srv->Release(); g_svs.mask_srv = nullptr; }
    g_svs.mask_id = nullptr;
    g_svs.mask_w = 0; g_svs.mask_h = 0; g_svs.mask_fmt = 0;
}

// The latched mask candidate holds an AddRef'd texture between the bind that latched it and the frame boundary that
// consumes it; it must be released here or a device reset keeps a dead device alive.
inline void svs_mask_release(bool scrub_vmir) {
    svs_mask_swap_release();
    // The volume transport's too: the copy, both views and the AddRef'd source.
    svs_vol_release();
    if (scrub_vmir) vmir_release();
    svs_vol_src_release();
    if (g_svs.mask_cand) { g_svs.mask_cand->Release(); g_svs.mask_cand = nullptr; }
    if (g_svs.pre_srv) { g_svs.pre_srv->Release(); g_svs.pre_srv = nullptr; }
    if (g_svs.pre_tex) { g_svs.pre_tex->Release(); g_svs.pre_tex = nullptr; }
    if (g_svs.post_srv) { g_svs.post_srv->Release(); g_svs.post_srv = nullptr; }
    if (g_svs.post_tex) { g_svs.post_tex->Release(); g_svs.post_tex = nullptr; }
    g_svs.pre_w = 0; g_svs.pre_h = 0; g_svs.pre_fmt = 0;
    g_svs.bracket = 0;
    g_svs.rtv_seen_n = 0;   // Weak identities die with the device.
    g_svs.cls_n = 0;
}

inline bool svs_sten_ready() {
    if (g_svs.mask_srv == nullptr || g_svs.pre_srv == nullptr ||
        g_svs.post_srv == nullptr) {
        return false;
    }

    if (g_svs.post_made == 0) {  return false; }

    if (!g_main_depth_w || !g_main_depth_h ||
        g_svs.mask_w != static_cast<uint32_t>(g_main_depth_w) ||
        g_svs.mask_h != static_cast<uint32_t>(g_main_depth_h)) {
        return false;
    }

    return true;
}

// Demand gate: deliberate toggle, constant true, kept as a function so the feature can be switched off in one place.
// Gated on the mode, not on readiness: the snapshot textures are created inside svs_pre_ensure, which only runs from
// svs_mask_snap, so a readiness gate would never open.
inline bool svs_feature_on() {
    return true;
}

// The two artifacts get separate arms: the multiply reads a mask our depth
// never touched, so injection, snapshots and multiply arm independently.
inline bool svs_inject_armed() {
    if (shadows_off()) return false;   // SHADOW_OFF: nothing reads what the seam and the pass it arms serve.
    if (svs_feature_on()) return true;
    return   // A mode must be wired into every site it needs.
           false;
}

inline bool svs_snap_wanted() {
    // Only our mesh draws read the snapshots (t21 / t22), so a frame with no visible
    // mesh copies nothing (the volume copy is gated the same way).
    // EA_SNAP_DEAD: and only through KhStenTerm, which the volume transport replaces (maskMeta.w and stenVol2.x are
    // both its readiness then, so KhStenUnit never takes it): no snapshot, as no prime (svs_prime_wanted).
    if (svs_vol_on()) return false;
    if (shadows_off()) return false;   // SHADOW_OFF: no reader.
    if (svs_feature_on()) return g_svs_mesh_wanted.load(std::memory_order_relaxed);
    return false;
}

// The snapshot ratio's readiness (KhStenTerm, the t21 / t22 binds). maskMeta.w is svs_unit_on's, which takes this
// only with the volume transport off (svs_vol_on).
inline bool svs_sten_on() {
    if (!svs_sten_ready()) return false;
    // Off with the feature toggle (svs_feature_on), as the injection's arm is (svs_inject_armed).
    if (svs_feature_on()) return true;
    return false;
}

inline bool svs_unit_on() {
    if (svs_vol_on()) return svs_vol_ready();
    return svs_sten_on();
}

// Destination for the snapshot: a clone of the mask desc minus the render target bind. Rebuilt whenever the mask's
// shape or format moves (the mask identity is weak).
inline bool svs_clone_one(ID3D11Device* dev, ID3D11Texture2D** tex,
                          ID3D11ShaderResourceView** srv) {
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = g_svs.mask_w;
    td.Height = g_svs.mask_h;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = static_cast<DXGI_FORMAT>(g_svs.mask_fmt);
    td.SampleDesc.Count = 1;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;   // Read only - never an RT.
    td.CPUAccessFlags = 0;
    td.MiscFlags = 0;

    if (FAILED(dev->CreateTexture2D(&td, nullptr, tex))) {
        *tex = nullptr;
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = td.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;

    if (FAILED(dev->CreateShaderResourceView(*tex, &sd, srv))) {
        *srv = nullptr;
        (*tex)->Release();
        *tex = nullptr;
        return false;
    }

    return true;
}

// Both ends of the bracket, created and dropped together: a ratio with only one snapshot alive is not a ratio, and
// svs_sten_ready refuses on either being null.
inline bool svs_pre_ensure(ID3D11Device* dev) {
    if (!dev || !g_svs.mask_w || !g_svs.mask_h) return false;

    if (g_svs.pre_tex && g_svs.pre_srv && g_svs.post_tex && g_svs.post_srv &&
        g_svs.pre_w == g_svs.mask_w && g_svs.pre_h == g_svs.mask_h &&
        g_svs.pre_fmt == g_svs.mask_fmt) {
        return true;
    }

    if (g_svs.pre_srv) { g_svs.pre_srv->Release(); g_svs.pre_srv = nullptr; }
    if (g_svs.pre_tex) { g_svs.pre_tex->Release(); g_svs.pre_tex = nullptr; }
    if (g_svs.post_srv) { g_svs.post_srv->Release(); g_svs.post_srv = nullptr; }
    if (g_svs.post_tex) { g_svs.post_tex->Release(); g_svs.post_tex = nullptr; }
    g_svs.pre_w = 0; g_svs.pre_h = 0; g_svs.pre_fmt = 0;

    if (!svs_clone_one(dev, &g_svs.pre_tex, &g_svs.pre_srv)) {
        return false;
    }

    if (!svs_clone_one(dev, &g_svs.post_tex, &g_svs.post_srv)) {
        if (g_svs.pre_srv) { g_svs.pre_srv->Release(); g_svs.pre_srv = nullptr; }
        if (g_svs.pre_tex) { g_svs.pre_tex->Release(); g_svs.pre_tex = nullptr; }
        return false;
    }

    g_svs.pre_w = g_svs.mask_w;
    g_svs.pre_h = g_svs.mask_h;
    g_svs.pre_fmt = g_svs.mask_fmt;
    return true;
}

// EA_VOL_COPY: src copied whole into dst ahead of the engine's draw - the engine's full OM set (OM_SAVE_VOL:
// OmSave, its UAVs with it) and its predicate (PRED_ASIDE) set aside around the copy, so a depth target bound as
// src is unbound while copied and the copy is never skipped; our hooks stand aside (in_injection, cleared after the
// restore). The one bracket of the volume copy (svs_vol_copy) and the replay's two engine snapshots
// (rp_pass_begin, rp_snap_engine).
inline void svs_copy_aside(ID3D11DeviceContext* ctx, ID3D11Resource* dst, ID3D11Resource* src) {
    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    OmSave om;
    om.capture(ctx);
    PredAside pa;
    pa.begin(ctx);
    ctx->OMSetRenderTargets(0, nullptr, nullptr);
    ctx->CopyResource(dst, src);
    pa.end();
    om.restore(ctx);
    om.release();
    g_ro.in_injection = prev;
}

// Source is whatever the adopted mask SRV views, so there is no second weak pointer to keep alive.
inline void svs_mask_snap(ID3D11DeviceContext* ctx, bool is_post) {
    if (!ctx || !g_svs.mask_srv) {
        return;
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = dev && svs_pre_ensure(dev);
    if (dev) dev->Release();

    if (!ok) {
        return;
    }

    ID3D11Resource* src = nullptr;
    g_svs.mask_srv->GetResource(&src);

    if (!src) {
        return;
    }

    {
        PredAside pa;   // PRED_ASIDE: the seam's mask snapshot, not skipped by the engine's predicate.
        pa.begin(ctx);
        ctx->CopyResource(is_post ? g_svs.post_tex : g_svs.pre_tex, src);
    }
    src->Release();

    if (is_post) {
        g_svs.post_made++;
    }
}
inline void volume_seam_frame_reset() {
    g_svs.injected_frame = false;
    if (g_svs.pending) { g_svs.pending = false;  }
    // A copy armed at a resolve bind that no draw followed.
    if (g_svs.bracket != 0) { g_svs.bracket = 0;  }
    g_svs.vol_dsv_now = false;
    g_svs.prime_ready = false;
    g_svs.frame_seq++;   // Epoch clock for the snapshot age.
    rp_release_dead();   // RP_DEAD_RELEASE: after the epoch moved.
    // SHADOW_OFF: every frame shadows are off, nothing of the stencil path stays ready for their return - the
    // volume copy unprimed, the snapshots unmade, the mirror absent (MIR_GATE's rule for a skipped mirror: every
    // reader then takes none rather than an older one) - and the last recorded pass lets its engine buffers go
    // (the epoch above has moved, so no pass is open). Here and not at the engine's mask bracket, which a frame
    // without the engine's shadow passes never reaches.
    if (shadows_off()) {
        g_svs.vol_primed = false;
        g_svs.post_made = 0;
        g_vmir.mask_time = -1.0f;
        rp_release_if_held();
    }
    if (g_svs.mask_cand) {
        void* cid = static_cast<void*>(g_svs.mask_cand);

        if (cid != g_svs.mask_id) {
            D3D11_TEXTURE2D_DESC cd = {};
            g_svs.mask_cand->GetDesc(&cd);
            ID3D11Device* cdev = nullptr;
            g_svs.mask_cand->GetDevice(&cdev);

            if (cdev) {
                // The swap release, not the teardown.
                svs_mask_swap_release();
                D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
                sd.Format = cd.Format;
                sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                sd.Texture2D.MipLevels = 1;

                if (SUCCEEDED(cdev->CreateShaderResourceView(
                        g_svs.mask_cand, &sd, &g_svs.mask_srv))) {
                    g_svs.mask_id = cid;
                    g_svs.mask_w = cd.Width;
                    g_svs.mask_h = cd.Height;
                    g_svs.mask_fmt = static_cast<uint32_t>(cd.Format);
                } else {
                    g_svs.mask_srv = nullptr;
                }

                cdev->Release();
            }
        }

        g_svs.mask_cand->Release();
        g_svs.mask_cand = nullptr;
    }
    g_svs.cls_n = 0;   // Classification cache is per frame too.
}

inline bool svs_prime_states(ID3D11Device* dev) {
    if (!dev || g_mask.prime_states_failed) return false;

    // Ahead of the early-out below: the states are created once and this must still get its chance on that first call.
    if (!g_mask.prime_occl_failed && !g_mask.prime_occl[0]) {
        D3D11_QUERY_DESC qd = {};
        qd.Query = D3D11_QUERY_OCCLUSION;

        for (int i = 0; i < 3; ++i) {
            if (FAILED(dev->CreateQuery(&qd, &g_mask.prime_occl[i]))) {
                for (int j = 0; j < 3; ++j) {
                    if (g_mask.prime_occl[j]) {
                        g_mask.prime_occl[j]->Release();
                        g_mask.prime_occl[j] = nullptr;
                    }
                    g_mask.prime_occl_inflight[j] = false;
                }
                g_mask.prime_occl_failed = true;
                break;
            }
        }
    }

    if (g_mask.prime_blend && g_mask.prime_dss) return true;

    if (!g_mask.prime_dss) {
        D3D11_DEPTH_STENCIL_DESC dd = {};
        dd.DepthEnable = TRUE;
        dd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dd.StencilEnable = FALSE;

        if (FAILED(dev->CreateDepthStencilState(&dd, &g_mask.prime_dss))) {
            g_mask.prime_states_failed = true;
            return false;
        }
    }

    if (!g_mask.prime_blend) {
        D3D11_BLEND_DESC bd = {};
        bd.RenderTarget[0].BlendEnable = FALSE;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;

        if (FAILED(dev->CreateBlendState(&bd, &g_mask.prime_blend))) {
            g_mask.prime_states_failed = true;
            return false;
        }
    }

    return g_mask.prime_blend != nullptr && g_mask.prime_dss != nullptr;
}

inline bool svs_prime_wanted() {
    // The volume transport (svs_vol_on, constant true) disables the priming pass (cascade cut absent).
    if (svs_vol_on()) return false;
    return svs_snap_wanted();
}

// VOL_ZOOM: a perspective projection's screen terms, row-vector convention: ndc.x = (x / z) * P00' + P20', ndc.y =
// (y / z) * P11' + P21', with ' = over P23 (the w column). False for anything else (a skewed or affine projection): the
// map then stands down to the identity.
inline bool sten_proj_terms(const float p[4][4], float out[4]) {
    const float w = p[2][3];
    if (!(w > 1.0e-6f) || !(fabsf(p[3][3]) < 1.0e-6f) ||
        !(fabsf(p[0][1]) < 1.0e-6f) || !(fabsf(p[1][0]) < 1.0e-6f) ||
        !(fabsf(p[3][0]) < 1.0e-6f) || !(fabsf(p[3][1]) < 1.0e-6f) ||
        !(fabsf(p[0][3]) < 1.0e-6f) || !(fabsf(p[1][3]) < 1.0e-6f)) return false;
    out[0] = p[0][0] / w;
    out[1] = p[1][1] / w;
    out[2] = p[2][0] / w;
    out[3] = p[2][1] / w;
    for (int i = 0; i < 4; ++i) {
        if (!(out[i] == out[i]) || fabsf(out[i]) > 1.0e6f) return false;
    }
    return fabsf(out[0]) > 1.0e-6f && fabsf(out[1]) > 1.0e-6f;
}

// VOL_ZOOM: the raster map (HLSL stenVol3) from a pass drawing under proj onto the volume copy's seam-frame
// raster - copy = raster * xy + zw per axis - or 0 (the identity) with no measured projection on either side, or
// one of the other handedness. Every lookup into an image drawn in the copy's raster takes it: the count (t24,
// KhVolTerm) and the mirror (t28, KhMirUnit; MIR_ZOOM). Equal projections give s = 1 and o = 0 exactly.
inline void sten_zoom_map(float out[4], const float (*proj)[4]) {
    out[0] = out[1] = out[2] = out[3] = 0.0f;
    float now[4];
    if (g_svs.vol_proj_ok && proj && g_svs.vol_w > 0 && g_svs.vol_h > 0 &&
        sten_proj_terms(proj, now)) {
        const float sx = g_svs.vol_proj[0] / now[0];
        const float sy = g_svs.vol_proj[1] / now[1];
        const float ox = g_svs.vol_proj[2] - now[2] * sx;
        const float oy = g_svs.vol_proj[3] - now[3] * sy;
        const float w = static_cast<float>(g_svs.vol_w);
        const float h = static_cast<float>(g_svs.vol_h);
        if (sx > 0.0f && sy > 0.0f) {   // Same handedness; a flip is not a zoom.
            out[0] = sx;
            out[1] = sy;
            out[2] = 0.5f * w * (1.0f - sx + ox);
            out[3] = 0.5f * h * (1.0f - sy - oy);
        }
    }
}

// Reprojection fill, one truth for both CB editions. A per-pixel read offset makes smf itself noisy (a halo on the
// stencil shadow and on our mesh); do not build another mode on this. VOL_WITNESS is not one: it reads the count at
// the fragment's own pixel and only asks whether the copy's surface there is the fragment's (its ring fallback reads
// neighbours that pass the same test). Lanes: sten_vol.xy = the copy's (m22, m32), sten_vol2.yw = its viewport (min,
// max); all zero = the witness stands down.
// proj = the projection this pass draws our meshes under (the injection's
// and the flush's pv.projection); nullptr = no map.
inline void fill_sten_reproj(ConstantData& cbd, const float (*proj)[4]) {
    const bool vol = svs_vol_on() && svs_vol_ready();
    const bool wit = vol && g_svs.vol_depth_srv != nullptr && g_svs.vol_enc_ok;
    cbd.sten_vol[0] = wit ? g_svs.vol_enc[0] : 0.0f;
    cbd.sten_vol[1] = wit ? g_svs.vol_enc[1] : 0.0f;
    cbd.sten_vol[2] = static_cast<float>(g_svs.vol_w);
    cbd.sten_vol[3] = static_cast<float>(g_svs.vol_h);
    cbd.sten_vol2[0] = vol ? 1.0f : 0.0f;
    cbd.sten_vol2[1] = wit ? g_svs.vol_enc[2] : 0.0f;
    cbd.sten_vol2[3] = wit ? g_svs.vol_enc[3] : 0.0f;
    // VOL_FOOT: armed exactly when the pass binds a mask at t33: the merged one on a merged pass
    // (rp_use_merged, g_rp.mfoot_srv), else the copy's (svs_foot_bound).
    cbd.sten_vol4[0] = (vol && (rp_use_merged() || svs_foot_bound())) ? 1.0f : 0.0f;   // VOL_REPLAY.
    cbd.sten_vol4[1] = cbd.sten_vol4[2] = cbd.sten_vol4[3] = 0.0f;
    // VOL_ZOOM: ndc_copy = ndc_pass * s + o per axis (s = the copy's scale over the pass's, o = the copy's offset
    // less the pass's times s), carried to pixels: the copy and this pass share the target's size (the lookup's own
    // premise).
    // Measured projections only - no motion is assumed. VOL_REPLAY: a merged frame maps the same way: replay B
    // redraws the copy's pass in the copy's raster (its footprint under the cycle latch, or under the copy's own
    // projection terms where the two part - REPLAY_RASTER), while this pass draws under proj, whose scale the
    // injection takes from the live fetch - the two differ while the view zooms.
    sten_zoom_map(cbd.sten_vol3, vol ? proj : nullptr);
}

// VOL_WITNESS: the scene depth's encode pair, by the flush guard's precedence (the pass projection, then the
// measured engine pair, then the live slot pair when its near and far are sane).
inline void scene_depth_pair(const float proj[4][4], float& m22, float& m32) {
    m22 = proj[2][2];
    m32 = proj[3][2];
    if (g_ro.engine_proj_valid) {
        m22 = g_ro.engine_m22;
        m32 = g_ro.engine_m32;
    }
    if (g_ro.slot_near_live >= CAM_NEAR_MIN &&
        g_ro.slot_near_live <= CAM_NEAR_MAX &&
        enc_far(g_ro.slot_m22, g_ro.slot_m32) <= ENC_FAR_MAX) {
        m22 = g_ro.slot_m22;
        m32 = g_ro.slot_m32;
    }
}
}