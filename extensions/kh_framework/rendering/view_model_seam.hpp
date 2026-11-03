#pragma once

// The injection half (rendering_integration_injection.hpp): the view-model seam (infront_seam_inject) and its
// pre-draw. Never include it on its own.
namespace RenderIntegration {
inline void infront_seam_inject(ID3D11DeviceContext* ctx, const D3D11_VIEWPORT& vp) {
    if (!g_res.vs || !g_res.composite_cb || !g_res.composite_frame_cb || !g_res.input_layout ||
        !g_res.rasterizer || !g_res.rasterizer_cull || g_res.mesh_vb.empty()) return;
    ID3D11Device* dev = RVExtBridge::get_d3d_device();
    if (!dev) return;
    float m22 = 0.0f, m32 = 0.0f;
    if (!infront_pair(m22, m32)) return;
    const bool pinj = g_ro.in_injection;
    g_ro.in_injection = true;

    // The casters: live, at the consume point (the world seam's rule), their shapes
    // too (SEAM_SKIN: the pass's context; the flag is raised above).
    attach_step(ctx);
    uvs_step(ctx);
    scene_sync();
    static std::vector<RenderObject> list;
    list.clear();
    const float now = effect_time_seconds();
    for (uint32_t s = 0; s < g_scene.objs.size(); ++s) {
        if (!g_scene.alive[s]) continue;
        const RenderObject& o = g_scene.objs[s];
        if (!o.in_front || !obj_shown(o) || !shadow_part(o)) continue;   // SHADOW_SWITCH: the seam's rule.
        if (o.blend_mode != 0 || o.color[3] < 0.999f) continue;   // Writes no depth in the colour pass: not drawn.
        bool exp = false;
        const float env = lifetime_envelope(o, now, exp);
        if (exp || o.color[3] * env < 0.999f) continue;
        const int vs_mid = mesh_id_clamp(o.mesh);
        if (vs_mid < 0 || static_cast<size_t>(vs_mid) >= g_res.mesh_vb.size()) continue;
        if (!mesh_vb_ref(vs_mid, o.slot)) continue;
        list.push_back(o);
    }
    if (list.empty()) { g_ro.in_injection = pinj; return; }

    // The pass transform: the world seam's this window, else the same take.
    float vpm[4][4];
    float cam[3];
    bool  rebase;
    if (g_svs.injected_frame || g_rp.seam_b) {   // VOL_REPLAY: in B mode, the transform the B world seam just took.
        memcpy(vpm, g_svs.prime_vp, sizeof(vpm));
        cam[0] = g_svs.prime_cam[0]; cam[1] = g_svs.prime_cam[1]; cam[2] = g_svs.prime_cam[2];
        rebase = g_svs.prime_rebase;
    } else {
        RVExtBridge::ProjectionViewTransform pv = {};
        if (!svs_take_pv(ctx, pv)) { g_ro.in_injection = pinj; return; }
        cam_of(pv.view, cam);
        const bool engcam = engcam_consume(pv.view, cam);
        ConstantData tmp = {};
        mul_4x4(pv.view, pv.projection, tmp.view_proj);
        rebase = engcam
               ? rebase_vp_engcam(tmp, pv.view, pv.projection)
               : rebase_vp(tmp, cam);   // DDX_REBASE.
        memcpy(vpm, tmp.view_proj, sizeof(vpm));
    }
    // Column 2 from the hands pair: VP[r][2] = m22 * V[r][2] = m22 * VP[r][3] (r < 3), VP[3][2] = m22 * VP[3][3] +
    // m32 - the view's affine last column survives the rebase, which moves only its translation row.
    for (int r = 0; r < 3; ++r) vpm[r][2] = m22 * vpm[r][3];
    vpm[3][2] = m22 * vpm[3][3] + m32;

    StateBackup bk;
    bk.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSave om;
    om.capture(ctx);
    ctx->OMSetRenderTargets(0, nullptr, om.dsv);
    ConstantData cbf = {};
    memcpy(cbf.view_proj, vpm, sizeof(cbf.view_proj));
    cbf.pass_cam[0] = cam[0]; cbf.pass_cam[1] = cam[1]; cbf.pass_cam[2] = cam[2];
    cbf.pass_cam[3] = rebase ? 1.0f : 0.0f;
    if (!upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
        g_ro.in_injection = pinj;
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
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);   // Stencil untouched: the world count survives.
    const FLOAT bf[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(g_res.blend_modes[0], bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ID3D11RasterizerState* bound_rs = g_res.rasterizer;
    VbRef bound_vb;   // SKIN_POOL: the pair.
    bool alpha_bound = false;
    const bool alpha_ok = footprint_alpha_on();
    ConstantData cbd;
    for (const RenderObject& o : list) {
        const int mid = mesh_id_clamp(o.mesh);
        const MeshDef& md = mesh_def(mid);
        cbd = cbf;
        cbd.center_size[0] = o.pos[0];
        cbd.center_size[1] = o.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
        cbd.center_size[2] = o.pos[1];
        float res[3];   // POS_RES: the object's live residual.
        obj_pos_res(o, res);
        if (rebase) fill_center_rel(cbd, cam, res);
        cbd.size_axes[0] = o.size[0];
        cbd.size_axes[1] = o.size[2];
        cbd.size_axes[2] = o.size[1];
        cbd.size_axes[3] = 0.0f;
        fill_obj_rot(cbd, o.rot_m);
        cbd.depth_params[0] = m22;
        cbd.depth_params[1] = m32;
        cbd.depth_params[2] = vp.MinDepth;
        cbd.depth_params[3] = vp.MaxDepth;
        ID3D11RasterizerState* want_rs = o.two_sided ? g_res.rasterizer : g_res.rasterizer_cull;
        if (want_rs != bound_rs) { ctx->RSSetState(want_rs); bound_rs = want_rs; }
        const VbRef vb = mesh_vb_ref(mid, o.slot);   // CLOTH.
        if (vb != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = vb;
        }
        const MaterialSet* txm = obj_textured(o);
        const bool want_a = txm && alpha_ok && mat_set_has_alpha(txm);
        if (want_a != alpha_bound) {
            ctx->IASetInputLayout(want_a ? g_res.layout_tex : g_res.input_layout);
            ctx->VSSetShader(want_a ? g_res.vs_tex : g_res.vs, nullptr, 0);
            ctx->PSSetShader(want_a ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
            alpha_bound = want_a;
        }
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
            }
            if (!ok) break;
        } else {
            if (!upload_obj_cb(ctx, g_res.composite_cb, cbd)) break;
            UINT ls = 0, lc = 0;
            mesh_lod_range(md, 0, ls, lc);
            if (lc == 0) continue;
            ctx->DrawIndexed(lc, ls, 0);
        }
    }
    om.restore(ctx);
    om.release();
    bk.restore(ctx);
    g_ro.in_injection = pinj;
}

// The hook's side: every tracked draw on the main depth while the detector is armed (an inFront mesh, or a
// showInFront false one - SHOW_INFRONT) reads the viewport; the draw that first enters the slice's range this cycle
// marks the prepass (no render target bound) or the colour pass (the scene target bound) seen - the cycle's verdict -
// and, for a drawn inFront mesh (the colour pass also for a cycle whose prepass drew: INFRONT_PRE_COL), fires that
// pass's injection, each once per cycle.
inline void infront_pre_draw(ID3D11DeviceContext* ctx) {
    UINT n = 1;
    D3D11_VIEWPORT vp = {};
    ctx->RSGetViewports(&n, &vp);
    const bool vd_near = n >= 1 && infront_vp_is_slice(vp);
    const bool start = vd_near && !g_infront.prev_near;
    g_infront.prev_near = vd_near;
    if (!start) return;
    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    const bool colour = rtv != nullptr;
    if (rtv) rtv->Release();
    if (colour) {
        if (g_infront.col_done) return;
        g_infront.col_done = true;
    } else {
        if (g_infront.pre_done) return;
        g_infront.pre_done = true;
    }
    // SHOW_INFRONT: the verdict is taken (the flags above) for a showInFront false mesh too; the slice is drawn
    // into only for an inFront mesh that is drawn - or, INFRONT_PRE_COL, for the colour pass of a cycle whose
    // prepass drew (the census may have dropped the mesh between the two).
    if (!g_infront.wanted.load(std::memory_order_relaxed) && !g_infront.pre_injected) return;
    infront_inject(ctx, vp, colour);
}
}