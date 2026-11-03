#pragma once

// The injection half (rendering_integration_injection.hpp): the view-model seam (kh_infront_seam_inject) and its
// pre-draw. Never include it on its own.
namespace RenderIntegration {
inline void kh_infront_seam_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& khvs_vp) {
    if (!g_res.vs || !g_res.composite_cb || !g_res.composite_frame_cb || !g_res.input_layout ||
        !g_res.rasterizer || !g_res.rasterizer_cull || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    float khvs_m22 = 0.0f, khvs_m32 = 0.0f;
    if (!kh_infront_pair(khvs_m22, khvs_m32)) return;
    const bool khvs_pinj = g_ro.in_injection;
    g_ro.in_injection = true;

    // The casters: live, at the consume point (the world seam's rule), their shapes
    // too (KH_SEAM_SKIN: the pass's context; the flag is raised above).
    kh_attach_step(ctx);
    kh_uvs_step(ctx);
    kh_scene_sync();
    static std::vector<RenderObject> khvs_list;
    khvs_list.clear();
    const float khvs_now = effect_time_seconds();
    for (uint32_t khvs_s = 0; khvs_s < g_scene.objs.size(); ++khvs_s) {
        if (!g_scene.alive[khvs_s]) continue;
        const RenderObject& o = g_scene.objs[khvs_s];
        if (!o.in_front || !kh_obj_shown(o) || !kh_shadow_part(o)) continue;   // KH_SHADOW_SWITCH: the seam's rule.
        if (o.blend_mode != 0 || o.color[3] < 0.999f) continue;   // Writes no depth in the colour pass: not drawn.
        bool khvs_exp = false;
        const float khvs_env = lifetime_envelope(o, khvs_now, khvs_exp);
        if (khvs_exp || o.color[3] * khvs_env < 0.999f) continue;
        const int khvs_mid = mesh_id_clamp(o.mesh);
        if (khvs_mid < 0 || static_cast<size_t>(khvs_mid) >= g_res.mesh_vb.size()) continue;
        if (!kh_mesh_vb_ref(khvs_mid, o.slot)) continue;
        khvs_list.push_back(o);
    }
    if (khvs_list.empty()) { g_ro.in_injection = khvs_pinj; return; }

    // The pass transform: the world seam's this window, else the same take.
    float khvs_vpm[4][4];
    float khvs_cam[3];
    bool  khvs_rebase;
    if (g_svs_injected_frame || g_rp_seam_b) {   // KH_VOL_REPLAY: in B mode, the transform the B world seam just took.
        memcpy(khvs_vpm, g_svs_prime_vp, sizeof(khvs_vpm));
        khvs_cam[0] = g_svs_prime_cam[0]; khvs_cam[1] = g_svs_prime_cam[1]; khvs_cam[2] = g_svs_prime_cam[2];
        khvs_rebase = g_svs_prime_rebase;
    } else {
        RVExtBridge::ProjectionViewTransform khvs_pv = {};
        if (!kh_svs_take_pv(ctx, khvs_pv)) { g_ro.in_injection = khvs_pinj; return; }
        kh_cam_of(khvs_pv.view, khvs_cam);
        const bool khvs_engcam = kh_engcam_consume(khvs_pv.view, khvs_cam);
        ConstantData khvs_tmp = {};
        mul_4x4(khvs_pv.view, khvs_pv.projection, khvs_tmp.view_proj);
        khvs_rebase = khvs_engcam
                    ? kh_rebase_vp_engcam(khvs_tmp, khvs_pv.view, khvs_pv.projection)
                    : kh_rebase_vp(khvs_tmp, khvs_cam);   // KH_DDX_REBASE.
        memcpy(khvs_vpm, khvs_tmp.view_proj, sizeof(khvs_vpm));
    }
    // Column 2 from the hands pair: VP[r][2] = m22 * V[r][2] = m22 * VP[r][3] (r < 3), VP[3][2] = m22 * VP[3][3] +
    // m32 - the view's affine last column survives the rebase, which moves only its translation row.
    for (int khvs_r = 0; khvs_r < 3; ++khvs_r) khvs_vpm[khvs_r][2] = khvs_m22 * khvs_vpm[khvs_r][3];
    khvs_vpm[3][2] = khvs_m22 * khvs_vpm[3][3] + khvs_m32;

    StateBackup khvs_bk;
    khvs_bk.capture(ctx);
    KhBindScope khvs_bc(ctx);   // KH_BIND_FILTER.
    KhOmSave khvs_om;
    khvs_om.capture(ctx);
    ctx->OMSetRenderTargets(0, nullptr, khvs_om.dsv);
    ConstantData khvs_cbf = {};
    memcpy(khvs_cbf.view_proj, khvs_vpm, sizeof(khvs_cbf.view_proj));
    khvs_cbf.kh_pass[0] = khvs_cam[0]; khvs_cbf.kh_pass[1] = khvs_cam[1]; khvs_cbf.kh_pass[2] = khvs_cam[2];
    khvs_cbf.kh_pass[3] = khvs_rebase ? 1.0f : 0.0f;
    if (!kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khvs_cbf)) {
        khvs_om.restore(ctx);
        khvs_om.release();
        khvs_bk.restore(ctx);
        g_ro.in_injection = khvs_pinj;
        return;
    }
    // The volume buffer, depth-only, under the engine's own hands-range viewport: the engine's hands prepass binds a
    // render target beside it, which is not written here.
    const UINT stride = sizeof(MeshVertex);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    if (g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
    ID3D11Buffer* khvs_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khvs_cbs);
    ctx->PSSetConstantBuffers(0, 2, khvs_cbs);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);   // Stencil untouched: the world count survives.
    const FLOAT khvs_bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], khvs_bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* khvs_bound_rs = g_res.rasterizer;
    KhVbRef khvs_bound_vb;   // KH_SKIN_POOL: the pair.
    bool khvs_alpha_bound = false;
    const bool khvs_alpha_ok = kh_footprint_alpha_on();
    ConstantData cbd;
    for (const RenderObject& o : khvs_list) {
        const int mid = mesh_id_clamp(o.mesh);
        const MeshDef& md = mesh_def(mid);
        cbd = khvs_cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float khvs_res[3];   // KH_POS_RES: the object's live residual.
        kh_pos_res(o, khvs_res);
        if (khvs_rebase) kh_fill_center_rel(cbd, khvs_cam, khvs_res);
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = 0.0f;
        kh_fill_obj_rot(cbd, o.rot_m);
        cbd.depth_params[0] = khvs_m22;
        cbd.depth_params[1] = khvs_m32;
        cbd.depth_params[2] = khvs_vp.MinDepth;
        cbd.depth_params[3] = khvs_vp.MaxDepth;
        ID3D11RasterizerState* khvs_want_rs = o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull;
        if (khvs_want_rs != khvs_bound_rs) { ctx->RSSetState(khvs_want_rs); khvs_bound_rs = khvs_want_rs; }
        const KhVbRef khvs_vb = kh_mesh_vb_ref(mid, o.slot);   // KH_CLOTH.
        if (khvs_vb != khvs_bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &khvs_vb.vb, &stride, &khvs_vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            khvs_bound_vb = khvs_vb;
        }
        const KhMaterialSet* khvs_txm = kh_obj_textured(o);
        const bool khvs_want_a = khvs_txm && khvs_alpha_ok && kh_mat_set_has_alpha(khvs_txm);
        if (khvs_want_a != khvs_alpha_bound) {
            ctx->IASetInputLayout(khvs_want_a ? g_res.layout_tex : g_res.input_layout);
            ctx->VSSetShader(khvs_want_a ? g_res.vs_tex : g_res.vs, nullptr, 0);
            ctx->PSSetShader(khvs_want_a ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
            khvs_alpha_bound = khvs_want_a;
        }
        if (khvs_want_a) {
            const std::vector<MeshSubmesh>& khvs_tab = mesh_lod_submeshes(md, 0);
            bool khvs_ok = true;
            for (size_t khvs_si = 0; khvs_si < khvs_tab.size() && khvs_ok; ++khvs_si) {
                const MeshSubmesh& khvs_sm = khvs_tab[khvs_si];
                if (khvs_sm.index_count == 0) continue;
                kh_bind_material(ctx, dev, cbd, khvs_txm, khvs_si);
                khvs_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, cbd);
                if (!khvs_ok) break;
                ctx->DrawIndexed(khvs_sm.index_count, khvs_sm.index_start, 0);
            }
            if (!khvs_ok) break;
        } else {
            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            UINT khvs_ls = 0, khvs_lc = 0;
            mesh_lod_range(md, 0, khvs_ls, khvs_lc);
            if (khvs_lc == 0) continue;
            ctx->DrawIndexed(khvs_lc, khvs_ls, 0);
        }
    }
    khvs_om.restore(ctx);
    khvs_om.release();
    khvs_bk.restore(ctx);
    g_ro.in_injection = khvs_pinj;
}

// The hook's side: every tracked draw on the main depth while the detector is armed (an inFront mesh, or a
// showInFront false one - KH_SHOW_INFRONT) reads the viewport; the draw that first enters the slice's range this cycle
// marks the prepass (no render target bound) or the colour pass (the scene target bound) seen - the cycle's verdict -
// and, for a drawn inFront mesh (the colour pass also for a cycle whose prepass drew: KH_INFRONT_PRE_COL), fires that
// pass's injection, each once per cycle.
inline void kh_infront_pre_draw(ID3D11DeviceContext* ctx) {
    UINT khvd_n = 1;
    D3D11_VIEWPORT khvd_vp = {};
    ctx->RSGetViewports(&khvd_n, &khvd_vp);
    const bool khvd_near = khvd_n >= 1 && kh_infront_vp_is_slice(khvd_vp);
    const bool khvd_start = khvd_near && !g_vm_prev_near;
    g_vm_prev_near = khvd_near;
    if (!khvd_start) return;
    ID3D11RenderTargetView* khvd_rtv = nullptr;
    ctx->OMGetRenderTargets(1, &khvd_rtv, nullptr);
    const bool khvd_colour = khvd_rtv != nullptr;
    if (khvd_rtv) khvd_rtv->Release();
    if (khvd_colour) {
        if (g_vm_col_done) return;
        g_vm_col_done = true;
    } else {
        if (g_vm_pre_done) return;
        g_vm_pre_done = true;
    }
    // KH_SHOW_INFRONT: the verdict is taken (the flags above) for a showInFront false mesh too; the slice is drawn
    // into only for an inFront mesh that is drawn - or, KH_INFRONT_PRE_COL, for the colour pass of a cycle whose
    // prepass drew (the census may have dropped the mesh between the two).
    if (!g_infront_wanted.load(std::memory_order_relaxed) && !g_vm_pre_injected) return;
    kh_infront_inject(ctx, khvd_vp, khvd_colour);
}
}