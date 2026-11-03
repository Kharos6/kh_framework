#pragma once

// The injection half (rendering_integration_injection.hpp): INFRONT, the view-model slice. Never include it on its
// own.
namespace RenderIntegration {
inline void rp_mir_finish(ID3D11DeviceContext* ctx);   // Defined with the replay.

// VM_SEE (defined with it in rendering/view_model_see_through.hpp): the deferred tail's transmittance arm and the
// re-issue brackets; and its scene-RT0 test (EA_SCENE_RTV), the tail arm's.
inline bool infront_vmsee_take(ID3D11DeviceContext* ctx);
inline bool vmsee_t_begin(ID3D11DeviceContext* ctx);
inline void vmsee_t_end(ID3D11DeviceContext* ctx);
inline bool rtv_is_scene(ID3D11RenderTargetView* rtv);

// INFRONT, the view-model slice: its demand, the slice's frame (view, projection, camera, pair), the prepass /
// colour flags of the cycle, the kept pair, its meshes and the deferred tail.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: watch_wanted, wanted.
// The deferred tail (tail_*) is armed on the render thread and taken at either thread's resolve
// (infront_tail_take): tail_armed publishes the rest with release.
// Session reset: session_globals_reset resets 21 of them.
struct InFrontState {
    // INFRONT - the engine's view-model slice. MEASURED (RenderDoc, 3840x2160, one frame): after the main depth
    // clear the engine draws the world's depth prepass in the viewport range [0.011, 0.999], then the first-person
    // weapon and hands DEPTH PREPASS in [0.00301, 0.01] - 21 draws, no render target, LESS_EQUAL + write, the
    // projection slot rewritten with near 0.01 / far 500 at the world's own sx / sy - then the world's colour passes in
    // [0.011, 0.999], then the hands COLOUR pass in [0.00301, 0.01] into the HDR scene target, EQUAL + write against
    // its own prepass, then post-processing. No depth clear and no fullscreen draw into the scene target sit between
    // the world and the slice. Because the slice's range lies below the world's, the prepass culls every world colour
    // draw under the hands and the mid-frame depth readers see them. An inFront mesh joins both slice draws: at the
    // first draw of the depth prepass it writes its depth (depth-only, LESS_EQUAL + write - its solid texels only,
    // PSInjDepthA's verdict), and at the first draw of the colour pass it shades under the same viewport, the same view
    // and the same hands projection with LESS_EQUAL + write. Ordering against the engine's own hands then falls out of
    // the depth test: where ours is nearer it wins and their EQUAL draw fails; where it is behind, its test fails
    // against their prepass depth. Both draws use ONE set of matrices and one mesh snapshot per cycle (g_infront.*), so
    // the colour draw meets its own prepass depth exactly, as the engine's EQUAL draw meets its own. The parts that
    // write no depth cannot take that route: an EQUAL draw after them passes wherever the depth is still its prepass
    // value - everywhere they drew - and paints over them. They are drawn after the engine's last slice draw instead
    // (the deferred tail, g_infront.tail_*), under the same targets, viewport and matrices. The hands pair (m22 / m32)
    // is the engine's own: the projection slot the locator watches receives the view-model upload before each slice
    // (proj_slot_probe admits it - same sx / sy, m22 within 2 % - and g_ro.slot_near_live carries its near, below
    // CAM_NEAR_MIN, which is why the world keep filters it). A slice whose upload the locator missed takes the last
    // confirmed pair (a fixed property of the engine; VM_KEEP_MS). The viewport is the engine's own, read at the
    // trigger draw and left bound. The view is the cycle latch under adopt_frame_view, the recipe the world
    // injection uses; the frame lanes are the published frame template (g_pip.tpl - the PIP's reuse: the injection's,
    // or the flush's where the injection published none, INFRONT_TPL), with the terrain lane and the snapshot stood
    // down and the sun / band / light / stencil terms kept; the stencil term reads the mirror over the whole mesh
    // (mirMeta.x = 2, the near-collapse fix), the volume-buffer count where no mirror was built. A caster like any mesh
    // while the engine draws a slice (shadow_active, g_vm_slice_live; of the seam's draws the VOL_FOOT mask alone
    // leaves it out), outside SSAO (the world injection's pass), never in the world colour passes
    // (is_composite_eligible / the censuses). Level 0 only. Prepass and colour agree on every discard but
    // ClipEdgeSliver (stands down under 10 m, where a view-model mesh lives). A cloth / skin substitute installed by a
    // park landing between the two slice draws leaves that frame's colour draw short of its prepass; the next frame
    // repairs it.
    std::atomic<bool> wanted{false};   // flush_frame census: any drawn inFront mesh.
    // SHOW_INFRONT: the slice detector's demand (infront_pre_draw) - g_infront.wanted, or a visible mesh with
    // showInFront false (its test reads the detector's verdict, g_vm_slice_live). flush_frame census.
    std::atomic<bool> watch_wanted{false};
    // Render-thread state (the hook's thread), reset at the main depth clear.
    bool     prev_near = false;   // The previous tracked draw sat in the slice's range.
    bool     pre_done = false;    // This cycle's slice prepass was seen (and injected into: inFront).
    bool     col_done = false;    // This cycle's slice colour pass was seen (likewise).
    // INFRONT_PRE_COL: this cycle's prepass injection reached its draws (our solids' depth may stand in the slice).
    // The colour pass then injects whatever the census (g_infront.wanted) reads by then: that depth culls the world's
    // and the hands' colour under it (EQUAL), so only our colour can fill it. It draws the prepass's snapshot.
    bool     pre_injected = false;
    // SHOW_INFRONT: the detector was armed (g_infront.watch_wanted) when this cycle began - the cycle's verdict will
    // be a measurement (g_vm_slice_known). Render thread (the clear's thread, as the flags above).
    bool     watch_cycle = false;
    bool     seam_done = false;   // This window's view-model seam injection ran (the volume buffer).
    bool     frame_valid = false; // g_infront.view_proj / cam / pair / meshes are this cycle's.
    uint64_t frame_cycle = 0;
    float    view_proj[4][4] = {};   // The slice's rebased view-projection (pass_cam.w = g_infront.rebase).
    float    proj[4][4] = {};   // MIR_ZOOM: its projection (the latch's, the hands pair in m22 / m32).
    float    cam[3] = {};
    float    rebase = 0.0f;
    float    m22 = 0.0f, m32 = 0.0f;   // The hands pair this cycle drew with.
    float    keep_m22 = 0.0f, keep_m32 = 0.0f;   // The last confirmed pair.
    uint64_t keep_ms = 0;
    std::vector<RenderObject> meshes;   // The cycle's snapshot (both draws read it).
    // INFRONT - the colour pass's deferred tail: the parts that write no depth (infront_inject's loop 2 - a
    // translucent or non-normal-blend object, a blend material's translucent texels, a depth-Off overlay). Drawn at the
    // pass's first draw, the engine's later hands draws (EQUAL + write against the prepass depth) would cover them
    // wherever they overlap. So they are drawn once the engine's slice draws are all in, under the pass's own scene RT0
    // and main depth view (held, AddRef, from the pass's first draw), its viewport and the cycle's matrices
    // (g_infront.*), tested LESS_EQUAL: over the hands where ours is nearer, over a plane that wrote no depth, behind
    // one that wrote its depth nearer. The pass's end is the FIRST of: the render thread's OM bind that leaves the held
    // pair (RT0 or the depth view changes; the OM hooks, pip_track_targets' leave rule) or a resolve of the named
    // scene texture past the pass (FX_LATE's, either thread). The arm is one-shot (no double draw); the next main
    // depth clear releases an undrawn pair, the device / session the rest. Armed only when RT0 is the named scene
    // texture and a depth view is bound (infront_tail_arm); else the parts draw at the pass's start. Armed on the
    // render thread (the hooks'); taken on either (the OM hooks or the resolve).
    // Atomic: the game thread's resolve reads it too (the arm publishes the pair with release, a take acquires).
    std::atomic<bool>       tail_armed{false};
    ID3D11RenderTargetView* tail_rtv = nullptr;   // The pass's RT0 (the scene texture) and main depth view.
    ID3D11DepthStencilView* tail_dsv = nullptr;
    D3D11_VIEWPORT          tail_vp = {};         // The slice viewport the pass drew under.
};
static InFrontState g_infront;
static constexpr uint64_t VM_KEEP_MS = 2000;

inline void infront_tail_release() {
    g_infront.tail_armed.store(false, std::memory_order_relaxed);
    KH_SAFE_RELEASE(g_infront.tail_rtv);
    KH_SAFE_RELEASE(g_infront.tail_dsv);
}

// At the colour pass's first draw (infront_inject, slot 0 alone bound): the pass's RT0 and depth view held and
// the tail deferred - true - when RT0 is the named scene texture and a depth view is bound.
inline bool infront_tail_arm(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& vp) {
    infront_tail_release();
    if (!g_topo_scene_tex_id) return false;
    ID3D11RenderTargetView* rtv = nullptr;
    ID3D11DepthStencilView* dsv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, &dsv);
    const bool scene = rtv && rtv_is_scene(rtv);   // EA_SCENE_RTV.
    if (!scene || !dsv) {
        KH_SAFE_RELEASE(rtv);
        KH_SAFE_RELEASE(dsv);
        return false;
    }
    g_infront.tail_rtv = rtv;
    g_infront.tail_dsv = dsv;
    g_infront.tail_vp = vp;
    g_infront.tail_armed.store(true, std::memory_order_release);
    return true;
}

inline void infront_frame_reset() {
    // The ended cycle's verdict, published before the flags clear (the session
    // reset lands here too and publishes false).
    g_vm_slice_live.store(g_infront.pre_done || g_infront.col_done, std::memory_order_relaxed);
    // SHOW_INFRONT: a measurement only if the detector watched the whole cycle; the next one is watched if it is
    // armed now.
    g_vm_slice_known.store(g_infront.watch_cycle, std::memory_order_relaxed);
    g_infront.watch_cycle = g_infront.watch_wanted.load(std::memory_order_relaxed);
    g_infront.prev_near = false;
    g_infront.pre_done = false;
    g_infront.col_done = false;
    g_infront.pre_injected = false;   // INFRONT_PRE_COL.
    g_infront.seam_done = false;
    g_infront.frame_valid = false;
    infront_tail_release();   // INFRONT: a tail still armed drew nowhere - its frame is gone.
}

// The slice's range: a viewport that never reaches the far side (the trigger's
// rule for the weapon / hands slice) and starts near.
inline bool infront_vp_is_slice(const D3D11_VIEWPORT& vp) {
    return vp.MaxDepth < 0.7f && vp.MinDepth <= 0.3f &&
           vp.MaxDepth > vp.MinDepth &&
           vp.Width >= 2.0f && vp.Height >= 2.0f;
}

// The hands projection pair: the slot's live pair when it is not the world's (its near below half the world's, or below
// CAM_NEAR_MIN with no world pair latched), else the keep.
inline bool infront_pair(float& m22, float& m32) {
    const uint64_t now = steady_now_ms();
    const float sn = g_ro.slot_near_live;
    if (sn > 0.0f && fabsf(g_ro.slot_m22) > 1.0e-9f && g_ro.slot_m32 < 0.0f) {
        bool hands;
        if (g_ro.world_pair_valid && fabsf(g_ro.world_m22) > 1.0e-9f) {
            const float wn = -g_ro.world_m32 / g_ro.world_m22;
            hands = sn < 0.5f * wn;
        } else {
            hands = sn < CAM_NEAR_MIN;
        }
        if (hands) {
            m22 = g_ro.slot_m22;
            m32 = g_ro.slot_m32;
            g_infront.keep_m22 = m22;
            g_infront.keep_m32 = m32;
            g_infront.keep_ms = now;
            return true;
        }
    }
    if (g_infront.keep_ms != 0 && now - g_infront.keep_ms < VM_KEEP_MS && fabsf(g_infront.keep_m22) > 1.0e-9f) {
        m22 = g_infront.keep_m22;
        m32 = g_infront.keep_m32;
        return true;
    }
    return false;
}

// Once per cycle, at the first slice draw of either kind: the matrices and the mesh snapshot both slice draws share.
inline bool infront_frame_prepare(ID3D11DeviceContext* ctx) {
    if (g_infront.frame_valid && g_infront.frame_cycle == g_topo_cycles) return true;
    g_infront.frame_valid = false;
    float m22 = 0.0f, m32 = 0.0f;
    if (!infront_pair(m22, m32)) return false;
    RVExtBridge::ProjectionViewTransform pv = {};
    if (g_ro.cycle_pv_valid) {
        pv = g_ro.cycle_pv;
    } else if (!RVExtBridge::get_projection_view_transform(pv)) {
        return false;
    }
    if (fabsf(pv.projection[2][2]) < 1.0e-6f || fabsf(pv.projection[0][0]) < 1.0e-6f) return false;
    adopt_frame_view(pv);
    pv.projection[2][2] = m22;
    pv.projection[3][2] = m32;
    float cam[3];
    cam_of(pv.view, cam);
    const bool engcam = engcam_consume(pv.view, cam);
    ConstantData tmp;
    mul_4x4(pv.view, pv.projection, tmp.view_proj);
    const bool rebase = engcam
                      ? rebase_vp_engcam(tmp, pv.view, pv.projection)
                      : rebase_vp(tmp, cam);   // DDX_REBASE.
    memcpy(g_infront.view_proj, tmp.view_proj, sizeof(g_infront.view_proj));
    memcpy(g_infront.proj, pv.projection, sizeof(g_infront.proj));   // MIR_ZOOM.
    g_infront.cam[0] = cam[0]; g_infront.cam[1] = cam[1]; g_infront.cam[2] = cam[2];
    g_infront.rebase = rebase ? 1.0f : 0.0f;
    g_infront.m22 = m22;
    g_infront.m32 = m32;

    // The snapshot: the attach lanes sampled here, once, and the objects copied out so the second slice draw reads what
    // the first drew. SEAM_SKIN: with the pass's context (its caller raised the flag), so the shapes are this
    // frame's too, and the vertex stages over them.
    attach_step(ctx);
    uvs_step(ctx);
    scene_sync();
    const float now = effect_time_seconds();
    g_infront.meshes.clear();
    for (uint32_t s = 0; s < g_scene.objs.size(); ++s) {
        if (!g_scene.alive[s]) continue;
        const RenderObject& o = g_scene.objs[s];
        if (!o.in_front || !obj_shown(o) || o.fullscreen || o.effect != 0) continue;   // SHOW_INFRONT.
        bool exp = false;
        const float env = lifetime_envelope(o, now, exp);
        if (exp || env <= 0.0f) continue;
        const int mid = mesh_id_clamp(o.mesh);
        if (mid < 0 || static_cast<size_t>(mid) >= g_res.mesh_vb.size()) continue;
        if (!mesh_vb_ref(mid, o.slot)) continue;   // CLOTH: no buffer, nothing to draw.
        g_infront.meshes.push_back(o);
        g_infront.meshes.back().color[3] *= env;
    }
    g_infront.frame_cycle = g_topo_cycles;
    g_infront.frame_valid = true;
    return true;
}

// colour = false: the depth prepass (no render target bound). true: the colour pass. late (INFRONT, colour
// only): the deferred tail - loop 2 alone, at the pass's end, under the held targets and viewport (g_infront.tail_*,
// vp is that viewport); every exit leaves the engine's state as found and the caller releases the pair.
inline void infront_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& vp, bool colour,
                           bool late = false) {
    if (!g_pip.tpl_valid) return;   // No frame lanes to shade under yet.
    if (!g_res.vs || !g_res.ps || !g_res.composite_cb || !g_res.composite_frame_cb ||
        !g_res.input_layout || !g_res.dss_test_write || !g_res.dss_test || !g_res.dss_off ||
        !g_res.rasterizer || !g_res.rasterizer_cull || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    // Our own draws: the hooks stand aside. EA_VM_FLAG: restored by the scope on every exit, a throw included -
    // the tail can run on the game thread (infront_tail_take at its resolve), whose hook_except leaves the flag
    // alone, so a hand restore skipped by a throw there would leave the render thread's hooks standing aside.
    InjectionFlagScope fl;
    g_ro.in_injection = true;
    // INFRONT: the tail draws the pass's own snapshot and matrices or nothing (never a re-prepare).
    if ((late && (!g_infront.frame_valid || g_infront.frame_cycle != g_topo_cycles || !g_infront.tail_rtv ||
        !g_infront.tail_dsv)) ||
        !infront_frame_prepare(ctx) || g_infront.meshes.empty()) {
        return;
    }
    const float* cam = g_infront.cam;
    // REPLAY_SCISSOR: the slice's meshes are drawn under the hands projection, which the replay's rectangles do not
    // bound - the merged textures are the slice's only when the replay drew the whole target.
    if (colour) rp_mir_finish(ctx);   // MIR_REPLAY: an inFront mesh reads the mirror whole.
    RpPassScope rp(g_rp.sc_all.full, g_rp.sc_mir.full);
    StateBackup bk;
    bk.capture(ctx);
    // INFRONT: the tail binds the held pass targets and viewport (the engine has moved on: the OM leave or the
    // resolve), put back before the state (pip_run_fx's order: declared after the backup, restored ahead of it).
    OmSave om;
    if (late) {
        om.capture(ctx);
        ctx->OMSetRenderTargets(1, &g_infront.tail_rtv, g_infront.tail_dsv);
        ctx->RSSetViewports(1, &vp);
    }
    BindScope bc(ctx);   // BIND_FILTER.
    OmSlot0 s0;   // MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    s0.open(ctx);

    ConstantData cbf = g_pip.tpl;
    memcpy(cbf.view_proj, g_infront.view_proj, sizeof(cbf.view_proj));
    fill_user_cam_cb(cbf, cam, nullptr);   // USER_LANES: the slice's camera, the main view's axes.
    cbf.snap_cam[3] = 0.0f;                                   // No scene snapshot.
    // The stencil unit (mask_meta.w, sten_vol, sten_vol2.x) stays: the engine counts its shadow volumes in its volume
    // buffer before the main depth clear, and an inFront mesh's footprint is in that count twice - the world seam's and
    // the hands seam's (INFRONT, infront_seam_inject) - so the count at a slice pixel is taken against our own
    // depth there, as its own hands receive theirs. The vertex-path selector (sten_vol2.z) is cleared: the plain path
    // (the witness, zoom-map and footprint lanes are set below).
    cbf.sten_vol2[2] = 0.0f;
    // VOL_WITNESS stands down here (sten_vol2.y = .w = 0): the slice draws under its own projection and t23 is not
    // bound for it, so the count is read as it stands.
    cbf.sten_vol2[1] = 0.0f;
    cbf.sten_vol2[3] = 0.0f;
    // VOL_ZOOM / MIR_ZOOM: the slice's own map - the template's is the main pass's projection onto the copy's;
    // the slice draws under g_infront.proj, whose screen terms are the latch's (only m22 / m32 differ). The count and
    // the mirror are in the copy's raster - our in-front footprint too, since the hands seam takes the world seam's
    // transform - so both take it.
    sten_zoom_map(cbf.sten_vol3, (svs_vol_on() && svs_vol_ready()) ? g_infront.proj : nullptr);
    memset(cbf.sten_vol4, 0, sizeof(cbf.sten_vol4));   // VOL_FOOT likewise.
    // The mirror answers the whole mesh (mirMeta.x = 2) when this frame built it: the world seam drew the mesh into it,
    // so its count is taken against the mesh with the volumes standing on a 0.05 near - the near-collapse fix the world
    // receivers take at the near plane, here everywhere. Without a mirror the volume-buffer count (the world seam + the
    // hands seam) answers.
    cbf.mir_meta[0] = (colour && cbf.mir_meta[0] >= 0.5f && g_vmir.srv &&
                       g_vmir.mask_time >= 0.0f) ? 2.0f : 0.0f;
    cbf.thm_params[3] = 0.0f;                                 // The terrain never occludes the slice.
    cbf.fx_meta[2] = vp.Width;
    cbf.fx_meta[3] = vp.Height;
    cbf.pass_cam[0] = cam[0]; cbf.pass_cam[1] = cam[1]; cbf.pass_cam[2] = cam[2];
    cbf.pass_cam[3] = g_infront.rebase;
    cbf.fog_color[3] = cam[1];
    // PF_STAGES: the engine's GS / HS / DS nulled before the convert (pip_inject's twin).
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    sun_pf_convert(ctx, &cbf, &g_infront.meshes);   // PF_NEED: the slice's snapshot is what it draws.
    fill_sun_pf_lanes(cbf);   // PF_NEED: the arms say what the binds below bind.
    if (!upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
        s0.close(ctx);   // MESH_O1: under the hooks' exclusion.
        if (late) { om.restore(ctx); om.release(); }   // INFRONT: the engine's targets back.
        bk.restore(ctx);
        return;
    }

    const UINT stride = sizeof(MeshVertex);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(colour ? g_res.ps : nullptr, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    if (colour) {   // The flush's PSMain binds: the received atlas / bands, the sun ladder, the lights.
        ID3D11ShaderResourceView* atlas = shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
        ctx->PSSetShaderResources(1, 1, &atlas);
        for (UINT vi_b = 0; vi_b < 8; ++vi_b) {
            if (g_ls.band[vi_b].valid && g_ls.band[vi_b].srv) {
                const UINT t = vi_b < 6 ? 4 + vi_b : 12 + (vi_b - 6);
                ctx->PSSetShaderResources(t, 1, &g_ls.band[vi_b].srv);
            }
        }
        if (g_res.dls_srv) ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
        if (g_sun_maps.sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
        if (g_sun_maps.sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
        if (g_sun_maps.sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
        if (g_sun_maps.sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
        if (g_sun_maps.sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
        if (g_sun_maps.sun2_map_valid && g_sun_maps.sun_pf_valid[0] &&
            g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29, 1, &g_res.sun_pf_srv[0]);
        if (g_sun_maps.sun3_map_valid && g_sun_maps.sun_pf_valid[1] &&
            g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30, 1, &g_res.sun_pf_srv[1]);
        if (g_sun_maps.sun4_map_valid && g_sun_maps.sun_pf_valid[2] &&
            g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31, 1, &g_res.sun_pf_srv[2]);
        if (g_sun_maps.sun5_map_valid && g_sun_maps.sun_pf_valid[3] &&
            g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20, 1, &g_res.sun_pf_srv[3]);
        ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // SHADOW_GATHER: null = the default state, also clamp.
        // The engine's stencil evidence, as the injection binds it: the pre /
        // post pair (KhStenTerm) and the volume copy (KhVolTerm).
        if (svs_sten_on()) {
            if (g_svs.pre_srv) ctx->PSSetShaderResources(21, 1, &g_svs.pre_srv);
            if (g_svs.post_srv) ctx->PSSetShaderResources(22, 1, &g_svs.post_srv);
        }
        if (svs_vol_ready()) {   // VOL_REPLAY: the replayed count when this cycle has one.
            ID3D11ShaderResourceView* const sten = rp_use_merged() ? g_rp.msten_srv : g_svs.vol_sten_srv;
            ctx->PSSetShaderResources(24, 1, &sten);
        }
        if (cbf.mir_meta[0] >= 1.5f) {   // The mirror (MIR_REPLAY: this cycle's merge when it has one).
            ID3D11ShaderResourceView* const mir = rp_mir_srv();
            ctx->PSSetShaderResources(28, 1, &mir);
        }
    }
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* bound_rs = g_res.rasterizer;
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], bf, 0xFFFFFFFF);
    int bound_bm = 0;
    VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
    ID3D11VertexShader* bound_vs = g_res.vs;
    ID3D11InputLayout*  bound_il = g_res.input_layout;
    ID3D11PixelShader*  bound_ps = colour ? g_res.ps : nullptr;
    ID3D11DepthStencilState* bound_dss = g_res.dss_test_write;
    const bool alpha_ok = footprint_alpha_on();   // PSInjDepthA + the textured VS / layout.
    const bool tex_ok = g_res.layout_tex && g_res.vs_tex && g_res.ps_tex && g_res.mat_sampler;

    auto solid = [](const RenderObject& o) -> bool {
        return o.mode != DepthMode::Off && o.blend_mode == 0 && o.color[3] >= 0.999f;
    };
    auto fill = [&](const RenderObject& o, ConstantData& cbd) {
        cbd = cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float res[3];   // POS_RES: the object's live residual.
        obj_pos_res(o, res);
        if (g_infront.rebase > 0.5f) fill_center_rel(cbd, cam, res);
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        fill_obj_rot(cbd, o.rot_m);
        cbd.blend_ctl[0] = 0.0f;
        // CUTOUT_A2C: the slice keeps a cutout's hard clip. Its depth prepass (PSInjDepthA) clips at the cutoff
        // and culls the world colour under it, so a prepass sample the colour pass's alpha to coverage left out
        // would show no world behind it; with the clip both cover the same samples (cb.hlsl KhCutoutCover).
        cbd.blend_ctl[2] = 1.0f;
        cbd.blend_ctl[3] = 0.0f;
        memcpy(cbd.color, o.color, sizeof(cbd.color));
        cbd.fx0[0] = cam[0]; cbd.fx0[1] = cam[1]; cbd.fx0[2] = cam[2];
        cbd.fx1[0] = 1e9f;   // No punch-through guard: the slice has no scene snapshot.
        cbd.fx1[1] = 0.0f;
        cbd.fx_meta[0] = 0.0f;
        cbd.fx_meta[1] = 0.0f;
        cbd.depth_params[0] = g_infront.m22;   // The hands pair and the slice's own range.
        cbd.depth_params[1] = g_infront.m32;
        cbd.depth_params[2] = vp.MinDepth;
        cbd.depth_params[3] = vp.MaxDepth;
        fill_local_band_cb(cbd, o);
        fill_lighting_obj_cb(ctx, cbd, o);
        cbd.shadow_meta2[1] = 0.0f;   // No object view-distance cut (the prepass has none either).
    };
    auto set_dss = [&](ID3D11DepthStencilState* s) {
        if (s != bound_dss) { ctx->OMSetDepthStencilState(s, 0); bound_dss = s; }
    };
    auto bind_vb = [&](int vb_mid, uint32_t slot) {
        const VbRef vb = mesh_vb_ref(vb_mid, slot);   // CLOTH.
        if (vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[vb_mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = vb;
        }
    };
    auto bind_pipe = [&](bool tex, ID3D11PixelShader* ps) {
        ID3D11VertexShader* vs = tex ? g_res.vs_tex : g_res.vs;
        ID3D11InputLayout*  il = tex ? g_res.layout_tex : g_res.input_layout;
        if (vs != bound_vs) { ctx->VSSetShader(vs, nullptr, 0); bound_vs = vs; }
        if (il != bound_il) { ctx->IASetInputLayout(il); bound_il = il; }
        if (ps != bound_ps) { ctx->PSSetShader(ps, nullptr, 0); bound_ps = ps; }
    };
    auto set_rs = [&](ID3D11RasterizerState* s) {
        if (s != bound_rs) { ctx->RSSetState(s); bound_rs = s; }
    };
    auto set_bm = [&](int bm) {
        if (bm != bound_bm) { ctx->OMSetBlendState(g_res.blend_modes[bm], bf, 0xFFFFFFFF); bound_bm = bm; }
    };

    ConstantData cbd;
    uint32_t drawn = 0;
    if (!colour) g_infront.pre_injected = true;   // INFRONT_PRE_COL: the colour pass owes this snapshot.

    // Loop 1: the solids - the depth prepass draws these alone; the colour pass draws them first (LESS_EQUAL + write),
    // a blend material's solid part included. INFRONT: the deferred tail (late) skips them - the pass's start
    // drew them.
    for (const RenderObject& o : g_infront.meshes) {
        if (late || !solid(o)) continue;
        const int mid = mesh_id_clamp(o.mesh);
        const MeshDef& md = mesh_def(mid);
        fill(o, cbd);
        set_dss(g_res.dss_test_write);
        set_rs(o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull);
        bind_vb(mid, o.slot);
        const MaterialSet* txm = obj_textured(o);
        if (!colour) {
            // Depth only. A material with cutout / blend texels takes the clip-only alpha footprint, per submesh with
            // its maps bound - the colour pass's own texel verdict; everything else the bare VS.
            const bool want_a = txm && alpha_ok && mat_set_has_alpha(txm);
            bind_pipe(want_a, want_a ? g_res.ps_inj_depth_a : nullptr);
            if (want_a) {
                const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, 0);
                bool ok = true;
                for (size_t si = 0; si < tab.size() && ok; ++si) {
                    const MeshSubmesh& sm = tab[si];
                    if (sm.index_count == 0) continue;
                    bind_material(ctx, dev, cbd, txm, si);
                    ok = upload_obj_cb(ctx, g_res.composite_cb, cbd);
                    if (!ok) break;
                    ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                    ++drawn;
                }
                if (!ok) break;
            } else {
                if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
                UINT ls = 0, lc = 0;
                mesh_lod_range(md, 0, ls, lc);
                if (lc == 0) continue;
                ctx->DrawIndexed(lc, ls, 0);
                ++drawn;
            }
            continue;
        }
        set_bm(0);
        const bool tx = txm && tex_ok;
        bind_pipe(tx, tx ? g_res.ps_tex : g_res.ps);
        if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
        if (tx) {
            drawn += draw_textured(ctx, dev, cbd, g_res.composite_cb, *txm, mid, false,
                                   bound_rs, 0, g_res.ps_tex, 0, 1);
        } else {
            UINT ls = 0, lc = 0;
            mesh_lod_range(md, 0, ls, lc);
            if (lc == 0) continue;
            ctx->DrawIndexed(lc, ls, 0);
            ++drawn;
        }
    }

    // Loop 2 (colour only): what writes no depth - a blend material's translucent part, a translucent or
    // non-normal-blend object, a depth-Off overlay - far to near, tested against what loop 1 wrote. INFRONT: at the
    // pass's end (late), after the engine's own slice draws - the deferred tail; at the pass's start only where
    // the tail cannot be armed.
    if (colour) {
        static std::vector<std::pair<float, uint32_t>> tail;
        tail.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(g_infront.meshes.size()); ++i) {
            const RenderObject& o = g_infront.meshes[i];
            const bool part = solid(o) && obj_has_blend(o) && obj_textured(o) && tex_ok;
            if (!solid(o) || part) tail.emplace_back(mesh_dist_sq(o, cam), i);
        }
        std::stable_sort(tail.begin(), tail.end(),
                         [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) {
                             return a.first > b.first;
                         });
        // INFRONT: the pass's start - the pair held, the parts wait for the pass's end (infront_tail_take).
        if (!late && !tail.empty() && infront_tail_arm(ctx, vp)) tail.clear();
        // VM_SEE: at the pass's end, past the engine's planes and the cycle's snapshot, each normal-blend part is
        // drawn once more into the transmittance as the engine's planes are (vmsee_pre_draw's rule: over-blend
        // only; bs_vmsee_t takes the straight alpha PSMain / its textured twin write on this route - blendCtl.x 0,
        // KhWseeMeshOut's plain output). The snapshot is taken here when the engine's planes took none - and only
        // where a normal-blend part is in the tail (nothing would be re-issued otherwise: no snapshot for nothing).
        bool nb = false;
        for (const auto& t : tail) nb = nb || g_infront.meshes[t.second].blend_mode == 0;
        const bool vmsee = late && nb && infront_vmsee_take(ctx);
        for (const auto& t : tail) {
            const RenderObject& o = g_infront.meshes[t.second];
            const int mid = mesh_id_clamp(o.mesh);
            const MeshDef& md = mesh_def(mid);
            const bool part = solid(o);   // The blend material's translucent tail.
            fill(o, cbd);
            set_dss(o.mode == DepthMode::Off ? g_res.dss_off : g_res.dss_test);
            set_bm(o.blend_mode);
            bind_vb(mid, o.slot);
            const MaterialSet* txm = obj_textured(o);
            const bool tx = txm && tex_ok;
            // VM_PART_BOTH: a non-solid object's blend material draws both parts here, its solid texels (part 1)
            // then its glass (part 2), under the object's one depth state and blend - loop 1 drew neither.
            const bool both = !part && tx && obj_has_blend(o);
            // A two-sided translucent - a blend material's part included (VM_PART_TS: the injection's
            // tail and the flush do the same) - draws its interior first, then its exterior over it (the
            // ordered contract of both colour passes).
            const bool ts = o.two_sided && g_res.rasterizer_front != nullptr;
            bind_pipe(tx, tx ? g_res.ps_tex : g_res.ps);
            if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            // VM_SEE: the part's draws, once into the scene and once more into T (the rasterizer set at each
            // start: the ordered two-sided draw leaves the cull state bound).
            auto emit = [&]() {
                set_rs(ts ? g_res.rasterizer_front : o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull);
                if (tx) {
                    if (both) {   // VM_PART_BOTH: the ordered draw restarts per submesh in each call.
                        drawn += draw_textured(ctx, dev, cbd, g_res.composite_cb, *txm, mid, ts,
                                               bound_rs, 0, g_res.ps_tex, 0, 1);
                    }
                    drawn += draw_textured(ctx, dev, cbd, g_res.composite_cb, *txm, mid, ts,
                                           bound_rs, 0, g_res.ps_tex, 0,
                                           (part || both) ? 2 : 1);
                } else {
                    UINT ls = 0, lc = 0;
                    mesh_lod_range(md, 0, ls, lc);
                    if (lc == 0) return;
                    ctx->DrawIndexed(lc, ls, 0);
                    ++drawn;
                    if (ts) {
                        set_rs(g_res.rasterizer_cull);
                        ctx->DrawIndexed(lc, ls, 0);
                        ++drawn;
                    }
                }
            };
            emit();
            if (vmsee && o.blend_mode == 0 && vmsee_t_begin(ctx)) {
                emit();
                vmsee_t_end(ctx);
            }
        }
    }
    (void)drawn;
    s0.close(ctx);   // MESH_O1: under the hooks' exclusion.
    if (late) { om.restore(ctx); om.release(); }   // INFRONT: the engine's targets back.
    bk.restore(ctx);
}

// INFRONT: the deferred tail's draw at the pass's end (g_infront.tail_*: the sites, above), once - disarmed ahead of
// the draw, so a throw inside leaves that frame's parts undrawn, never drawn twice; the held pair goes with a normal
// return, and at the next main depth clear after a throw (infront_frame_reset).
// Under the flush flag too: the game thread's resolve may host it (fx_late_gt's rule - its draws stay out of
// every thread's hooks), and the render thread's hooks stand aside on in_injection, as for the pass's start.
inline void infront_tail_take(ID3D11DeviceContext* ctx) {
    if (!g_infront.tail_armed.load(std::memory_order_acquire)) return;
    g_infront.tail_armed.store(false, std::memory_order_relaxed);
    {
        FlushActiveScope active;
        infront_inject(ctx, g_infront.tail_vp, true, true);
    }
    infront_tail_release();
}
}