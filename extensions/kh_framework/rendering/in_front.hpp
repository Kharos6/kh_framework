#pragma once

// The injection half (rendering_integration_injection.hpp): KH_INFRONT, the view-model slice. Never include it on its
// own.
namespace RenderIntegration {
inline void kh_rp_mir_finish(ID3D11DeviceContext* ctx);   // Defined with the replay.

// KH_VM_SEE (defined with it in rendering/view_model_see_through.hpp): the deferred tail's transmittance arm and the
// re-issue brackets.
inline bool kh_infront_vmsee_take(ID3D11DeviceContext* khvt_ctx);
inline bool kh_vmsee_t_begin(ID3D11DeviceContext* ctx);
inline void kh_vmsee_t_end(ID3D11DeviceContext* ctx);

// KH_INFRONT - the engine's view-model slice. MEASURED (RenderDoc, 3840x2160, one frame): after the main depth clear
// the engine draws the world's depth prepass in the viewport range [0.011, 0.999], then the first-person weapon and
// hands DEPTH PREPASS in [0.00301, 0.01] - 21 draws, no render target, LESS_EQUAL + write, the projection slot
// rewritten with near 0.01 / far 500 at the world's own sx / sy - then the world's colour passes in [0.011, 0.999],
// then the hands COLOUR pass in [0.00301, 0.01] into the HDR scene target, EQUAL + write against its own prepass, then
// post-processing. No depth clear and no fullscreen draw into the scene target sit between the world and the slice.
// Because the slice's range lies below the world's, the prepass culls every world colour draw under the hands and the
// mid-frame depth readers see them.
//
// An inFront mesh joins both slice draws: at the first draw of the depth prepass it writes its depth (depth-only,
// LESS_EQUAL + write - its solid texels only, PSInjDepthA's verdict), and at the first draw of the colour pass it
// shades under the same viewport, the same view and the same hands projection with LESS_EQUAL + write. Ordering against
// the engine's own hands then falls out of the depth test: where ours is nearer it wins and their EQUAL draw fails;
// where it is behind, its test fails against their prepass depth. Both draws use ONE set of matrices and one mesh
// snapshot per cycle (g_vm_*), so the colour draw meets its own prepass depth exactly, as the engine's EQUAL draw meets
// its own. The parts that write no depth cannot take that route: an EQUAL draw after them passes wherever the depth is
// still its prepass value - everywhere they drew - and paints over them. They are drawn after the engine's last slice
// draw instead (the deferred tail, g_vm_tail_*), under the same targets, viewport and matrices.
//
// The hands pair (m22 / m32) is the engine's own: the projection slot the locator watches receives the view-model
// upload before each slice (proj_slot_probe admits it - same sx / sy, m22 within 2 % - and g_ro.slot_near_live carries
// its near, below KH_CAM_NEAR_MIN, which is why the world keep filters it). A slice whose upload the locator missed
// takes the last confirmed pair (a fixed property of the engine; KH_VM_KEEP_MS). The viewport is the engine's own, read
// at the trigger draw and left bound. The view is the cycle latch under kh_adopt_frame_view, the recipe the world
// injection uses; the frame lanes are the published frame template (g_pip.tpl - the PIP's reuse: the injection's, or
// the flush's where the injection published none, KH_INFRONT_TPL), with the terrain lane and the snapshot stood down
// and the sun / band / light / stencil terms kept; the stencil term reads the mirror over the whole mesh
// (mirMeta.x = 2, the near-collapse fix), the volume-buffer count where no mirror was built.
//
// A caster like any mesh while the engine draws a slice (kh_shadow_active, g_vm_slice_live; of the seam's draws the
// KH_VOL_FOOT mask alone leaves it out), outside KH_SSAO (the world injection's pass), never in the world colour passes
// (is_composite_eligible / the censuses). Level 0 only. Prepass and colour agree on every discard but ClipEdgeSliver
// (stands down under 10 m, where a view-model mesh lives). A cloth / skin substitute installed by a park landing
// between the two slice draws leaves that frame's colour draw short of its prepass; the next frame repairs it.
static std::atomic<bool> g_infront_wanted{false};   // flush_frame census: any drawn inFront mesh.
// KH_SHOW_INFRONT: the slice detector's demand (kh_infront_pre_draw) - g_infront_wanted, or a visible mesh with
// showInFront false (its test reads the detector's verdict, g_vm_slice_live). flush_frame census.
static std::atomic<bool> g_vm_watch_wanted{false};
// Render-thread state (the hook's thread), reset at the main depth clear.
static bool     g_vm_prev_near = false;   // The previous tracked draw sat in the slice's range.
static bool     g_vm_pre_done = false;    // This cycle's slice prepass was seen (and injected into: inFront).
static bool     g_vm_col_done = false;    // This cycle's slice colour pass was seen (likewise).
// KH_INFRONT_PRE_COL: this cycle's prepass injection reached its draws (our solids' depth may stand in the slice).
// The colour pass then injects whatever the census (g_infront_wanted) reads by then: that depth culls the world's
// and the hands' colour under it (EQUAL), so only our colour can fill it. It draws the prepass's snapshot.
static bool     g_vm_pre_injected = false;
// KH_SHOW_INFRONT: the detector was armed (g_vm_watch_wanted) when this cycle began - the cycle's verdict will be a
// measurement (g_vm_slice_known). Render thread (the clear's thread, as the flags above).
static bool     g_vm_watch_cycle = false;
static bool     g_vm_seam_done = false;   // This window's view-model seam injection ran (the volume buffer).
static bool     g_vm_frame_valid = false; // g_vm_view_proj / cam / pair / meshes are this cycle's.
static uint64_t g_vm_frame_cycle = 0;
static float    g_vm_view_proj[4][4] = {};   // The slice's rebased view-projection (kh_pass.w = g_vm_rebase).
static float    g_vm_proj[4][4] = {};   // KH_MIR_ZOOM: its projection (the latch's, the hands pair in m22 / m32).
static float    g_vm_cam[3] = {};
static float    g_vm_rebase = 0.0f;
static float    g_vm_m22 = 0.0f, g_vm_m32 = 0.0f;   // The hands pair this cycle drew with.
static float    g_vm_keep_m22 = 0.0f, g_vm_keep_m32 = 0.0f;   // The last confirmed pair.
static uint64_t g_vm_keep_ms = 0;
static constexpr uint64_t KH_VM_KEEP_MS = 2000;
static std::vector<RenderObject> g_vm_meshes;   // The cycle's snapshot (both draws read it).
// KH_INFRONT - the colour pass's deferred tail: the parts that write no depth (kh_infront_inject's loop 2 - a
// translucent or non-normal-blend object, a blend material's translucent texels, a depth-Off overlay). Drawn at the
// pass's first draw, the engine's later hands draws (EQUAL + write against the prepass depth) would cover them
// wherever they overlap. So they are drawn once the engine's slice draws are all in, under the pass's own scene RT0
// and main depth view (held, AddRef, from the pass's first draw), its viewport and the cycle's matrices (g_vm_*),
// tested LESS_EQUAL: over the hands where ours is nearer, over a plane that wrote no depth, behind one that wrote its
// depth nearer. The pass's end is the FIRST of: the render thread's OM bind that leaves the held pair (RT0 or the
// depth view changes; the OM hooks, kh_pip_track_targets' leave rule) or a resolve of the named scene texture past
// the pass (KH_FX_LATE's, either thread). The arm is one-shot (no double draw); the next main depth clear releases an
// undrawn pair, the device / session the rest. Armed only when RT0 is the named scene texture and a depth view is
// bound (kh_infront_tail_arm); else the parts draw at the pass's start. Armed on the render thread (the hooks'); taken
// on either (the OM hooks or the resolve).
static std::atomic<bool>       g_vm_tail_armed{false};   // Atomic: the game thread's resolve reads it too (the arm
                                                           // publishes the pair with release, a take acquires).
static ID3D11RenderTargetView* g_vm_tail_rtv = nullptr;   // The pass's RT0 (the scene texture) and main depth view.
static ID3D11DepthStencilView* g_vm_tail_dsv = nullptr;
static D3D11_VIEWPORT          g_vm_tail_vp = {};         // The slice viewport the pass drew under.

inline void kh_infront_tail_release() {
    g_vm_tail_armed.store(false, std::memory_order_relaxed);
    KH_SAFE_RELEASE(g_vm_tail_rtv);
    KH_SAFE_RELEASE(g_vm_tail_dsv);
}

// At the colour pass's first draw (kh_infront_inject, slot 0 alone bound): the pass's RT0 and depth view held and
// the tail deferred - true - when RT0 is the named scene texture and a depth view is bound.
inline bool kh_infront_tail_arm(ID3D11DeviceContext* khta_ctx, const D3D11_VIEWPORT& khta_vp) {
    kh_infront_tail_release();
    if (!g_topo_scene_tex_id) return false;
    ID3D11RenderTargetView* khta_rtv = nullptr;
    ID3D11DepthStencilView* khta_dsv = nullptr;
    khta_ctx->OMGetRenderTargets(1, &khta_rtv, &khta_dsv);
    bool khta_scene = false;
    if (khta_rtv) {
        ID3D11Resource* khta_res = nullptr;
        khta_rtv->GetResource(&khta_res);
        khta_scene = khta_res != nullptr && static_cast<void*>(khta_res) == g_topo_scene_tex_id;
        if (khta_res) khta_res->Release();
    }
    if (!khta_scene || !khta_dsv) {
        KH_SAFE_RELEASE(khta_rtv);
        KH_SAFE_RELEASE(khta_dsv);
        return false;
    }
    g_vm_tail_rtv = khta_rtv;
    g_vm_tail_dsv = khta_dsv;
    g_vm_tail_vp = khta_vp;
    g_vm_tail_armed.store(true, std::memory_order_release);
    return true;
}

inline void kh_infront_frame_reset() {
    // The ended cycle's verdict, published before the flags clear (the session
    // reset lands here too and publishes false).
    g_vm_slice_live.store(g_vm_pre_done || g_vm_col_done, std::memory_order_relaxed);
    // KH_SHOW_INFRONT: a measurement only if the detector watched the whole cycle; the next one is watched if it is
    // armed now.
    g_vm_slice_known.store(g_vm_watch_cycle, std::memory_order_relaxed);
    g_vm_watch_cycle = g_vm_watch_wanted.load(std::memory_order_relaxed);
    g_vm_prev_near = false;
    g_vm_pre_done = false;
    g_vm_col_done = false;
    g_vm_pre_injected = false;   // KH_INFRONT_PRE_COL.
    g_vm_seam_done = false;
    g_vm_frame_valid = false;
    kh_infront_tail_release();   // KH_INFRONT: a tail still armed drew nowhere - its frame is gone.
}

// The slice's range: a viewport that never reaches the far side (the trigger's
// rule for the weapon / hands slice) and starts near.
inline bool kh_infront_vp_is_slice(const D3D11_VIEWPORT& khvs_vp) {
    return khvs_vp.MaxDepth < 0.7f && khvs_vp.MinDepth <= 0.3f &&
           khvs_vp.MaxDepth > khvs_vp.MinDepth &&
           khvs_vp.Width >= 2.0f && khvs_vp.Height >= 2.0f;
}

// The hands projection pair: the slot's live pair when it is not the world's (its near below half the world's, or below
// KH_CAM_NEAR_MIN with no world pair latched), else the keep.
inline bool kh_infront_pair(float& khvp_m22, float& khvp_m32) {
    const uint64_t khvp_now = steady_now_ms();
    const float khvp_sn = g_ro.slot_near_live;
    if (khvp_sn > 0.0f && fabsf(g_ro.slot_m22) > 1.0e-9f && g_ro.slot_m32 < 0.0f) {
        bool khvp_hands;
        if (g_ro.world_pair_valid && fabsf(g_ro.world_m22) > 1.0e-9f) {
            const float khvp_wn = -g_ro.world_m32 / g_ro.world_m22;
            khvp_hands = khvp_sn < 0.5f * khvp_wn;
        } else {
            khvp_hands = khvp_sn < KH_CAM_NEAR_MIN;
        }
        if (khvp_hands) {
            khvp_m22 = g_ro.slot_m22;
            khvp_m32 = g_ro.slot_m32;
            g_vm_keep_m22 = khvp_m22;
            g_vm_keep_m32 = khvp_m32;
            g_vm_keep_ms = khvp_now;
            return true;
        }
    }
    if (g_vm_keep_ms != 0 && khvp_now - g_vm_keep_ms < KH_VM_KEEP_MS && fabsf(g_vm_keep_m22) > 1.0e-9f) {
        khvp_m22 = g_vm_keep_m22;
        khvp_m32 = g_vm_keep_m32;
        return true;
    }
    return false;
}

// Once per cycle, at the first slice draw of either kind: the matrices and the mesh snapshot both slice draws share.
inline bool kh_infront_frame_prepare(ID3D11DeviceContext* khvf_ctx) {
    if (g_vm_frame_valid && g_vm_frame_cycle == g_topo_cycles) return true;
    g_vm_frame_valid = false;
    float khvf_m22 = 0.0f, khvf_m32 = 0.0f;
    if (!kh_infront_pair(khvf_m22, khvf_m32)) return false;
    RVExtBridge::ProjectionViewTransform khvf_pv = {};
    if (g_ro.cycle_pv_valid) {
        khvf_pv = g_ro.cycle_pv;
    } else if (!RVExtBridge::get_projection_view_transform(khvf_pv)) {
        return false;
    }
    if (fabsf(khvf_pv.projection[2][2]) < 1.0e-6f || fabsf(khvf_pv.projection[0][0]) < 1.0e-6f) return false;
    kh_adopt_frame_view(khvf_pv);
    khvf_pv.projection[2][2] = khvf_m22;
    khvf_pv.projection[3][2] = khvf_m32;
    float khvf_cam[3];
    if (!kh_view_camera_exact(khvf_pv.view, khvf_cam)) extract_camera_pos(khvf_pv.view, khvf_cam);
    const bool khvf_engcam = kh_engcam_consume(khvf_pv.view, khvf_cam);
    ConstantData khvf_tmp;
    mul_4x4(khvf_pv.view, khvf_pv.projection, khvf_tmp.view_proj);
    const bool khvf_rebase = khvf_engcam
                           ? kh_rebase_vp_engcam(khvf_tmp, khvf_pv.view, khvf_pv.projection)
                           : kh_rebase_vp(khvf_tmp, khvf_cam);   // KH_DDX_REBASE.
    memcpy(g_vm_view_proj, khvf_tmp.view_proj, sizeof(g_vm_view_proj));
    memcpy(g_vm_proj, khvf_pv.projection, sizeof(g_vm_proj));   // KH_MIR_ZOOM.
    g_vm_cam[0] = khvf_cam[0]; g_vm_cam[1] = khvf_cam[1]; g_vm_cam[2] = khvf_cam[2];
    g_vm_rebase = khvf_rebase ? 1.0f : 0.0f;
    g_vm_m22 = khvf_m22;
    g_vm_m32 = khvf_m32;

    // The snapshot: the attach lanes sampled here, once, and the objects copied out so the second slice draw reads what
    // the first drew. KH_SEAM_SKIN: with the pass's context (its caller raised the flag), so the shapes are this
    // frame's too, and the vertex stages over them.
    kh_attach_step(khvf_ctx);
    kh_uvs_step(khvf_ctx);
    kh_scene_sync();
    const float khvf_now = effect_time_seconds();
    g_vm_meshes.clear();
    for (uint32_t khvf_s = 0; khvf_s < g_scene.objs.size(); ++khvf_s) {
        if (!g_scene.alive[khvf_s]) continue;
        const RenderObject& o = g_scene.objs[khvf_s];
        if (!o.in_front || !kh_obj_shown(o) || o.fullscreen || o.effect != 0) continue;   // KH_SHOW_INFRONT.
        bool khvf_exp = false;
        const float khvf_env = lifetime_envelope(o, khvf_now, khvf_exp);
        if (khvf_exp || khvf_env <= 0.0f) continue;
        const int khvf_mid = mesh_id_clamp(o.mesh);
        if (khvf_mid < 0 || static_cast<size_t>(khvf_mid) >= g_res.mesh_vb.size()) continue;
        if (!kh_mesh_vb_ref(khvf_mid, o.slot)) continue;   // KH_CLOTH: no buffer, nothing to draw.
        g_vm_meshes.push_back(o);
        g_vm_meshes.back().color[3] *= khvf_env;
    }
    g_vm_frame_cycle = g_topo_cycles;
    g_vm_frame_valid = true;
    return true;
}

// khvi_colour = false: the depth prepass (no render target bound). true: the colour pass. khvi_late (KH_INFRONT, colour
// only): the deferred tail - loop 2 alone, at the pass's end, under the held targets and viewport (g_vm_tail_*, khvi_vp
// is that viewport); every exit leaves the engine's state as found and the caller releases the pair.
inline void kh_infront_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& khvi_vp, bool khvi_colour,
                              bool khvi_late = false) {
    if (!g_pip.tpl_valid) return;   // No frame lanes to shade under yet.
    if (!g_res.vs || !g_res.ps || !g_res.composite_cb || !g_res.composite_frame_cb ||
        !g_res.input_layout || !g_res.dss_test_write || !g_res.dss_test || !g_res.dss_off ||
        !g_res.rasterizer || !g_res.rasterizer_cull || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    const bool khvi_pinj = g_ro.in_injection;
    g_ro.in_injection = true;   // Our own draws: the hooks stand aside.
    // KH_INFRONT: the tail draws the pass's own snapshot and matrices or nothing (never a re-prepare).
    if ((khvi_late && (!g_vm_frame_valid || g_vm_frame_cycle != g_topo_cycles || !g_vm_tail_rtv || !g_vm_tail_dsv)) ||
        !kh_infront_frame_prepare(ctx) || g_vm_meshes.empty()) {
        g_ro.in_injection = khvi_pinj;
        return;
    }
    const float* cam = g_vm_cam;
    // KH_REPLAY_SCISSOR: the slice's meshes are drawn under the hands projection, which the replay's rectangles do not
    // bound - the merged textures are the slice's only when the replay drew the whole target.
    if (khvi_colour) kh_rp_mir_finish(ctx);   // KH_MIR_REPLAY: an inFront mesh reads the mirror whole.
    KhRpPassScope khvi_rp(g_rp_sc_all.full, g_rp_sc_mir.full);
    StateBackup khvi_bk;
    khvi_bk.capture(ctx);
    // KH_INFRONT: the tail binds the held pass targets and viewport (the engine has moved on: the OM leave or the
    // resolve), put back before the state (kh_pip_fx's order: declared after the backup, restored ahead of it).
    KhOmSave khvi_om;
    if (khvi_late) {
        khvi_om.capture(ctx);
        ctx->OMSetRenderTargets(1, &g_vm_tail_rtv, g_vm_tail_dsv);
        ctx->RSSetViewports(1, &khvi_vp);
    }
    KhBindScope khvi_bc(ctx);   // KH_BIND_FILTER.
    KhOmSlot0 khvi_s0;   // KH_MESH_O1: slot 0 alone where the engine left more bound (the scope's end restores it).
    khvi_s0.open(ctx);

    ConstantData khvi_cbf = g_pip.tpl;
    memcpy(khvi_cbf.view_proj, g_vm_view_proj, sizeof(khvi_cbf.view_proj));
    kh_fill_user_cam_cb(khvi_cbf, cam, nullptr);   // KH_USER_LANES: the slice's camera, the main view's axes.
    khvi_cbf.snap_cam[3] = 0.0f;                                   // No scene snapshot.
    // The stencil unit (mask_meta.w, sten_vol, sten_vol2.x) stays: the engine counts its shadow volumes in its volume
    // buffer before the main depth clear, and an inFront mesh's footprint is in that count twice - the world seam's and
    // the hands seam's (KH_INFRONT, kh_infront_seam_inject) - so the count at a slice pixel is taken against our own
    // depth there, as its own hands receive theirs. The vertex-path selector (sten_vol2.z) is cleared: the plain path
    // (the witness, zoom-map and footprint lanes are set below).
    khvi_cbf.sten_vol2[2] = 0.0f;
    // KH_VOL_WITNESS stands down here (sten_vol2.y = .w = 0): the slice draws under its own projection and t23 is not
    // bound for it, so the count is read as it stands.
    khvi_cbf.sten_vol2[1] = 0.0f;
    khvi_cbf.sten_vol2[3] = 0.0f;
    // KH_VOL_ZOOM / KH_MIR_ZOOM: the slice's own map - the template's is the main pass's projection onto the copy's;
    // the slice draws under g_vm_proj, whose screen terms are the latch's (only m22 / m32 differ). The count and the
    // mirror are in the copy's raster - our in-front footprint too, since the hands seam takes the world seam's
    // transform - so both take it.
    kh_sten_zoom_map(khvi_cbf.sten_vol3, (kh_svs_vol_on() && kh_svs_vol_ready()) ? g_vm_proj : nullptr);
    memset(khvi_cbf.sten_vol4, 0, sizeof(khvi_cbf.sten_vol4));   // KH_VOL_FOOT likewise.
    // The mirror answers the whole mesh (mirMeta.x = 2) when this frame built it: the world seam drew the mesh into it,
    // so its count is taken against the mesh with the volumes standing on a 0.05 near - the near-collapse fix the world
    // receivers take at the near plane, here everywhere. Without a mirror the volume-buffer count (the world seam + the
    // hands seam) answers.
    khvi_cbf.mir_meta[0] = (khvi_colour && khvi_cbf.mir_meta[0] >= 0.5f && g_vmir_srv &&
                            g_vmir_mask_time >= 0.0f) ? 2.0f : 0.0f;
    khvi_cbf.thm_params[3] = 0.0f;                                 // The terrain never occludes the slice.
    khvi_cbf.fx_meta[2] = khvi_vp.Width;
    khvi_cbf.fx_meta[3] = khvi_vp.Height;
    khvi_cbf.kh_pass[0] = cam[0]; khvi_cbf.kh_pass[1] = cam[1]; khvi_cbf.kh_pass[2] = cam[2];
    khvi_cbf.kh_pass[3] = g_vm_rebase;
    khvi_cbf.fog_color[3] = cam[1];
    // KH_PF_STAGES: the engine's GS / HS / DS nulled before the convert (kh_pip_inject's twin).
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    kh_sun_pf_convert(ctx, &khvi_cbf, &g_vm_meshes);   // KH_PF_NEED: the slice's snapshot is what it draws.
    kh_fill_sun_pf_lanes(khvi_cbf);   // KH_PF_NEED: the arms say what the binds below bind.
    if (!kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khvi_cbf)) {
        khvi_s0.close(ctx);   // KH_MESH_O1: under the hooks' exclusion.
        if (khvi_late) { khvi_om.restore(ctx); khvi_om.release(); }   // KH_INFRONT: the engine's targets back.
        khvi_bk.restore(ctx);
        g_ro.in_injection = khvi_pinj;
        return;
    }

    const UINT stride = sizeof(MeshVertex);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(khvi_colour ? g_res.ps : nullptr, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* khvi_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khvi_cbs);
    ctx->PSSetConstantBuffers(0, 2, khvi_cbs);
    if (khvi_colour) {   // The flush's PSMain binds: the received atlas / bands, the sun ladder, the lights.
        ID3D11ShaderResourceView* khvi_atlas = shadow_live_ensure_srv() ? g_ls.atlas_srv : nullptr;
        ctx->PSSetShaderResources(1, 1, &khvi_atlas);
        for (UINT khvi_b = 0; khvi_b < 8; ++khvi_b) {
            if (g_ls.band[khvi_b].valid && g_ls.band[khvi_b].srv) {
                const UINT khvi_t = khvi_b < 6 ? 4 + khvi_b : 12 + (khvi_b - 6);
                ctx->PSSetShaderResources(khvi_t, 1, &g_ls.band[khvi_b].srv);
            }
        }
        if (g_res.dls_srv) ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
        if (g_sun_map_valid && g_res.sun_srv) ctx->PSSetShaderResources(11, 1, &g_res.sun_srv);
        if (g_sun2_map_valid && g_res.sun2_srv) ctx->PSSetShaderResources(25, 1, &g_res.sun2_srv);
        if (g_sun3_map_valid && g_res.sun3_srv) ctx->PSSetShaderResources(26, 1, &g_res.sun3_srv);
        if (g_sun4_map_valid && g_res.sun4_srv) ctx->PSSetShaderResources(27, 1, &g_res.sun4_srv);
        if (g_sun5_map_valid && g_res.sun5_srv) ctx->PSSetShaderResources(32, 1, &g_res.sun5_srv);
        if (g_sun2_map_valid && g_sun_pf_valid[0] && g_res.sun_pf_srv[0]) ctx->PSSetShaderResources(29, 1, &g_res.sun_pf_srv[0]);
        if (g_sun3_map_valid && g_sun_pf_valid[1] && g_res.sun_pf_srv[1]) ctx->PSSetShaderResources(30, 1, &g_res.sun_pf_srv[1]);
        if (g_sun4_map_valid && g_sun_pf_valid[2] && g_res.sun_pf_srv[2]) ctx->PSSetShaderResources(31, 1, &g_res.sun_pf_srv[2]);
        if (g_sun5_map_valid && g_sun_pf_valid[3] && g_res.sun_pf_srv[3]) ctx->PSSetShaderResources(20, 1, &g_res.sun_pf_srv[3]);
        ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER: null = the default state, also clamp.
        // The engine's stencil evidence, as the injection binds it: the pre /
        // post pair (KhStenTerm) and the volume copy (KhVolTerm).
        if (kh_svs_sten_on()) {
            if (g_svs_pre_srv) ctx->PSSetShaderResources(21, 1, &g_svs_pre_srv);
            if (g_svs_post_srv) ctx->PSSetShaderResources(22, 1, &g_svs_post_srv);
        }
        if (kh_svs_vol_ready()) {   // KH_VOL_REPLAY: the replayed count when this cycle has one.
            ID3D11ShaderResourceView* const khvi_sten = kh_rp_use_merged() ? g_rp_msten_srv : g_svs_vol_sten_srv;
            ctx->PSSetShaderResources(24, 1, &khvi_sten);
        }
        if (khvi_cbf.mir_meta[0] >= 1.5f) {   // The mirror (KH_MIR_REPLAY: this cycle's merge when it has one).
            ID3D11ShaderResourceView* const khvi_mir = kh_rp_mir_srv();
            ctx->PSSetShaderResources(28, 1, &khvi_mir);
        }
    }
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* khvi_bound_rs = g_res.rasterizer;
    const FLOAT khvi_bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], khvi_bf, 0xFFFFFFFF);
    int khvi_bound_bm = 0;
    KhVbRef khvi_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
    ID3D11VertexShader* khvi_bound_vs = g_res.vs;
    ID3D11InputLayout*  khvi_bound_il = g_res.input_layout;
    ID3D11PixelShader*  khvi_bound_ps = khvi_colour ? g_res.ps : nullptr;
    ID3D11DepthStencilState* khvi_bound_dss = g_res.dss_test_write;
    const bool khvi_alpha_ok = kh_footprint_alpha_on();   // PSInjDepthA + the textured VS / layout.
    const bool khvi_tex_ok = g_res.layout_tex && g_res.vs_tex && g_res.ps_tex && g_res.mat_sampler;

    auto khvi_solid = [](const RenderObject& o) -> bool {
        return o.mode != DepthMode::Off && o.blend_mode == 0 && o.color[3] >= 0.999f;
    };
    auto khvi_fill = [&](const RenderObject& o, ConstantData& cbd) {
        cbd = khvi_cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float khvi_res[3];   // KH_POS_RES: the object's live residual.
        kh_pos_res(o, khvi_res);
        if (g_vm_rebase > 0.5f) kh_fill_center_rel(cbd, cam, khvi_res);
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = static_cast<float>(o.blend_mode);
        kh_fill_obj_rot(cbd, o.rot_m);
        cbd.blend_ctl[0] = 0.0f;
        // KH_CUTOUT_A2C: the slice keeps a cutout's hard clip. Its depth prepass (PSInjDepthA) clips at the cutoff
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
        cbd.depth_params[0] = g_vm_m22;   // The hands pair and the slice's own range.
        cbd.depth_params[1] = g_vm_m32;
        cbd.depth_params[2] = khvi_vp.MinDepth;
        cbd.depth_params[3] = khvi_vp.MaxDepth;
        kh_fill_local_band_cb(cbd, o);
        fill_lighting_obj_cb(ctx, cbd, o);
        cbd.shadow_meta2[1] = 0.0f;   // No object view-distance cut (the prepass has none either).
    };
    auto khvi_set_dss = [&](ID3D11DepthStencilState* khvd_s) {
        if (khvd_s != khvi_bound_dss) { ctx->OMSetDepthStencilState(khvd_s, 0); khvi_bound_dss = khvd_s; }
    };
    auto khvi_bind_vb = [&](int khvb_mid, uint32_t khvb_slot) {
        const KhVbRef khvb_vb = kh_mesh_vb_ref(khvb_mid, khvb_slot);   // KH_CLOTH.
        if (khvb_vb != khvi_bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khvb_vb.vb, &stride, &khvb_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[khvb_mid], DXGI_FORMAT_R32_UINT, 0);
            khvi_bound_vb = khvb_vb;
        }
    };
    auto khvi_bind_pipe = [&](bool khvp_tex, ID3D11PixelShader* khvp_ps) {
        ID3D11VertexShader* khvp_vs = khvp_tex ? g_res.vs_tex : g_res.vs;
        ID3D11InputLayout*  khvp_il = khvp_tex ? g_res.layout_tex : g_res.input_layout;
        if (khvp_vs != khvi_bound_vs) { ctx->VSSetShader(khvp_vs, nullptr, 0); khvi_bound_vs = khvp_vs; }
        if (khvp_il != khvi_bound_il) { ctx->IASetInputLayout(khvp_il); khvi_bound_il = khvp_il; }
        if (khvp_ps != khvi_bound_ps) { ctx->PSSetShader(khvp_ps, nullptr, 0); khvi_bound_ps = khvp_ps; }
    };
    auto khvi_set_rs = [&](ID3D11RasterizerState* khvr_s) {
        if (khvr_s != khvi_bound_rs) { ctx->RSSetState(khvr_s); khvi_bound_rs = khvr_s; }
    };
    auto khvi_set_bm = [&](int khvm_bm) {
        if (khvm_bm != khvi_bound_bm) { ctx->OMSetBlendState(g_res.blend_modes[khvm_bm], khvi_bf, 0xFFFFFFFF); khvi_bound_bm = khvm_bm; }
    };

    ConstantData cbd;
    uint32_t khvi_drawn = 0;
    if (!khvi_colour) g_vm_pre_injected = true;   // KH_INFRONT_PRE_COL: the colour pass owes this snapshot.

    // Loop 1: the solids - the depth prepass draws these alone; the colour pass draws them first (LESS_EQUAL + write),
    // a blend material's solid part included. KH_INFRONT: the deferred tail (khvi_late) skips them - the pass's start
    // drew them.
    for (const RenderObject& o : g_vm_meshes) {
        if (khvi_late || !khvi_solid(o)) continue;
        const int mid = mesh_id_clamp(o.mesh);
        const MeshDef& md = mesh_def(mid);
        khvi_fill(o, cbd);
        khvi_set_dss(g_res.dss_test_write);
        khvi_set_rs(o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull);
        khvi_bind_vb(mid, o.slot);
        const KhMaterialSet* khvi_txm = kh_obj_textured(o);
        if (!khvi_colour) {
            // Depth only. A material with cutout / blend texels takes the clip-only alpha footprint, per submesh with
            // its maps bound - the colour pass's own texel verdict; everything else the bare VS.
            const bool khvi_want_a = khvi_txm && khvi_alpha_ok && kh_mat_set_has_alpha(khvi_txm);
            khvi_bind_pipe(khvi_want_a, khvi_want_a ? g_res.ps_inj_depth_a : nullptr);
            if (khvi_want_a) {
                const std::vector<MeshSubmesh>& khvi_tab = mesh_lod_submeshes(md, 0);
                bool khvi_ok = true;
                for (size_t khvi_si = 0; khvi_si < khvi_tab.size() && khvi_ok; ++khvi_si) {
                    const MeshSubmesh& khvi_sm = khvi_tab[khvi_si];
                    if (khvi_sm.index_count == 0) continue;
                    kh_bind_material(ctx, dev, cbd, khvi_txm, khvi_si);
                    khvi_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, cbd);
                    if (!khvi_ok) break;
                    ctx->DrawIndexed(khvi_sm.index_count, khvi_sm.index_start, 0);
                    ++khvi_drawn;
                }
                if (!khvi_ok) break;
            } else {
                if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
                UINT khvi_ls = 0, khvi_lc = 0;
                mesh_lod_range(md, 0, khvi_ls, khvi_lc);
                if (khvi_lc == 0) continue;
                ctx->DrawIndexed(khvi_lc, khvi_ls, 0);
                ++khvi_drawn;
            }
            continue;
        }
        khvi_set_bm(0);
        const bool khvi_tx = khvi_txm && khvi_tex_ok;
        khvi_bind_pipe(khvi_tx, khvi_tx ? g_res.ps_tex : g_res.ps);
        if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
        if (khvi_tx) {
            khvi_drawn += kh_draw_textured(ctx, dev, cbd, g_res.composite_cb, *khvi_txm, mid, false,
                                           khvi_bound_rs, 0, g_res.ps_tex, 0, 1);
        } else {
            UINT khvi_ls = 0, khvi_lc = 0;
            mesh_lod_range(md, 0, khvi_ls, khvi_lc);
            if (khvi_lc == 0) continue;
            ctx->DrawIndexed(khvi_lc, khvi_ls, 0);
            ++khvi_drawn;
        }
    }

    // Loop 2 (colour only): what writes no depth - a blend material's translucent part, a translucent or
    // non-normal-blend object, a depth-Off overlay - far to near, tested against what loop 1 wrote. KH_INFRONT: at the
    // pass's end (khvi_late), after the engine's own slice draws - the deferred tail; at the pass's start only where
    // the tail cannot be armed.
    if (khvi_colour) {
        static std::vector<std::pair<float, uint32_t>> khvi_tail;
        khvi_tail.clear();
        for (uint32_t khvi_i = 0; khvi_i < static_cast<uint32_t>(g_vm_meshes.size()); ++khvi_i) {
            const RenderObject& o = g_vm_meshes[khvi_i];
            const bool khvi_part = khvi_solid(o) && kh_obj_has_blend(o) && kh_obj_textured(o) && khvi_tex_ok;
            if (!khvi_solid(o) || khvi_part) khvi_tail.emplace_back(kh_mesh_dist_sq(o, cam), khvi_i);
        }
        std::stable_sort(khvi_tail.begin(), khvi_tail.end(),
                         [](const std::pair<float, uint32_t>& a, const std::pair<float, uint32_t>& b) {
                             return a.first > b.first;
                         });
        // KH_INFRONT: the pass's start - the pair held, the parts wait for the pass's end (kh_infront_tail_take).
        if (!khvi_late && !khvi_tail.empty() && kh_infront_tail_arm(ctx, khvi_vp)) khvi_tail.clear();
        // KH_VM_SEE: at the pass's end, past the engine's planes and the cycle's snapshot, each normal-blend part is
        // drawn once more into the transmittance as the engine's planes are (kh_vmsee_pre_draw's rule: over-blend
        // only; bs_vmsee_t takes the straight alpha PSMain / its textured twin write on this route - blendCtl.x 0,
        // KhWseeMeshOut's plain output). The snapshot is taken here when the engine's planes took none - and only
        // where a normal-blend part is in the tail (nothing would be re-issued otherwise: no snapshot for nothing).
        bool khvi_nb = false;
        for (const auto& khvi_t : khvi_tail) khvi_nb = khvi_nb || g_vm_meshes[khvi_t.second].blend_mode == 0;
        const bool khvi_vmsee = khvi_late && khvi_nb && kh_infront_vmsee_take(ctx);
        for (const auto& khvi_t : khvi_tail) {
            const RenderObject& o = g_vm_meshes[khvi_t.second];
            const int mid = mesh_id_clamp(o.mesh);
            const MeshDef& md = mesh_def(mid);
            const bool khvi_part = khvi_solid(o);   // The blend material's translucent tail.
            khvi_fill(o, cbd);
            khvi_set_dss(o.mode == DepthMode::Off ? g_res.dss_off : g_res.dss_test);
            khvi_set_bm(o.blend_mode);
            khvi_bind_vb(mid, o.slot);
            const KhMaterialSet* khvi_txm = kh_obj_textured(o);
            const bool khvi_tx = khvi_txm && khvi_tex_ok;
            // KH_VM_PART_BOTH: a non-solid object's blend material draws both parts here, its solid texels (part 1)
            // then its glass (part 2), under the object's one depth state and blend - loop 1 drew neither.
            const bool khvi_both = !khvi_part && khvi_tx && kh_obj_has_blend(o);
            // A two-sided translucent - a blend material's part included (KH_VM_PART_TS: the injection's
            // tail and the flush do the same) - draws its interior first, then its exterior over it (the
            // ordered contract of both colour passes).
            const bool khvi_ts = o.two_sided && g_res.rasterizer_front != nullptr;
            khvi_bind_pipe(khvi_tx, khvi_tx ? g_res.ps_tex : g_res.ps);
            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            // KH_VM_SEE: the part's draws, once into the scene and once more into T (the rasterizer set at each
            // start: the ordered two-sided draw leaves the cull state bound).
            auto khvi_emit = [&]() {
                khvi_set_rs(khvi_ts ? g_res.rasterizer_front : o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull);
                if (khvi_tx) {
                    if (khvi_both) {   // KH_VM_PART_BOTH: the ordered draw restarts per submesh in each call.
                        khvi_drawn += kh_draw_textured(ctx, dev, cbd, g_res.composite_cb, *khvi_txm, mid, khvi_ts,
                                                       khvi_bound_rs, 0, g_res.ps_tex, 0, 1);
                    }
                    khvi_drawn += kh_draw_textured(ctx, dev, cbd, g_res.composite_cb, *khvi_txm, mid, khvi_ts,
                                                   khvi_bound_rs, 0, g_res.ps_tex, 0,
                                                   (khvi_part || khvi_both) ? 2 : 1);
                } else {
                    UINT khvi_ls = 0, khvi_lc = 0;
                    mesh_lod_range(md, 0, khvi_ls, khvi_lc);
                    if (khvi_lc == 0) return;
                    ctx->DrawIndexed(khvi_lc, khvi_ls, 0);
                    ++khvi_drawn;
                    if (khvi_ts) {
                        khvi_set_rs(g_res.rasterizer_cull);
                        ctx->DrawIndexed(khvi_lc, khvi_ls, 0);
                        ++khvi_drawn;
                    }
                }
            };
            khvi_emit();
            if (khvi_vmsee && o.blend_mode == 0 && kh_vmsee_t_begin(ctx)) {
                khvi_emit();
                kh_vmsee_t_end(ctx);
            }
        }
    }
    (void)khvi_drawn;
    khvi_s0.close(ctx);   // KH_MESH_O1: under the hooks' exclusion.
    if (khvi_late) { khvi_om.restore(ctx); khvi_om.release(); }   // KH_INFRONT: the engine's targets back.
    khvi_bk.restore(ctx);
    g_ro.in_injection = khvi_pinj;
}

// KH_INFRONT: the deferred tail's draw at the pass's end (g_vm_tail_*: the sites, above), once - disarmed ahead of
// the draw, so a throw inside leaves that frame's parts undrawn, never drawn twice; the held pair goes with a normal
// return, and at the next main depth clear after a throw (kh_infront_frame_reset).
// Under the flush flag too: the game thread's resolve may host it (kh_fx_late_gt's rule - its draws stay out of
// every thread's hooks), and the render thread's hooks stand aside on in_injection, as for the pass's start.
inline void kh_infront_tail_take(ID3D11DeviceContext* khtk_ctx) {
    if (!g_vm_tail_armed.load(std::memory_order_acquire)) return;
    g_vm_tail_armed.store(false, std::memory_order_relaxed);
    {
        KhFlushActiveScope khtk_active;
        kh_infront_inject(khtk_ctx, g_vm_tail_vp, true, true);
    }
    kh_infront_tail_release();
}
}