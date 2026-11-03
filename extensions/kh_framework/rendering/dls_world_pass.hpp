#pragma once

// The injection half (rendering_integration_injection.hpp): KH_DLS_WORLD, the world pass of our dynamic-light shadows.
// Never include it on its own.
namespace RenderIntegration {

// KH_DLS_WORLD stage 4: the game environment receives our meshes' dynamic-light shadows - one fullscreen multiply into
// the scene colour at the resolve, emitting KhDlsWorldFactor (how much of each light's contribution our meshes blocked
// at that pixel, divided by everything lighting it; the albedo cancels, which is what makes this possible without a
// G-buffer - see the kernel's note in cb.hlsl).
static uint64_t g_dls_world_cycle = 0;        // once-per-cycle guard (state, not a lane).

// The caller resolves the matrix once and hands in the engine's own view and projection for this frame, so the mask
// clips exactly where the engine's draw of the same geometry clipped. The value written is the clip w, not the clip z,
// so the z terms decide only what is clipped, never what is stored.
inline bool kh_dlsw_mask_render(ID3D11DeviceContext* khm_ctx,
                                const float khm_view[16], const float khm_proj_in[16],
                                UINT khm_w, UINT khm_h) {
    if (!khm_ctx || khm_w < 2 || khm_h < 2) return false;
    if (!g_res.vs_dls_mask || !g_res.ps_dls_mask || !g_res.dlsw_min_blend ||
        !g_res.input_layout || !g_res.dss_off || !g_res.rasterizer ||
        !g_res.composite_cb || !g_res.composite_frame_cb) return false;
    if (g_dlsw_casters.empty()) return false;
    if (!khm_proj_in) return false;
    // A degenerate projection scales the mesh to nothing or to infinity; a
    // wrong mask is the failure this path exists to remove.
    if (!(fabsf(khm_proj_in[0]) > 1.0e-6f) || !(fabsf(khm_proj_in[5]) > 1.0e-6f)) return false;
    // Refused; the caller treats it as a mask failure (mir_meta[0] stays 0) and the pass runs unmasked.
    if (!(khm_proj_in[11] > 0.0f)) return false;

    if (!g_res.dlsw_tex || g_res.dlsw_w != khm_w || g_res.dlsw_h != khm_h) {
        KH_SAFE_RELEASE(g_res.dlsw_srv);
        KH_SAFE_RELEASE(g_res.dlsw_rtv);
        KH_SAFE_RELEASE(g_res.dlsw_tex);
        g_res.dlsw_w = g_res.dlsw_h = 0;
        ID3D11Device* khm_dev = nullptr;
        khm_ctx->GetDevice(&khm_dev);
        if (!khm_dev) return false;
        D3D11_TEXTURE2D_DESC khm_td = {};
        khm_td.Width = khm_w;
        khm_td.Height = khm_h;
        khm_td.MipLevels = 1;
        khm_td.ArraySize = 1;
        khm_td.Format = DXGI_FORMAT_R32_FLOAT;   // Metres, not a depth format.
        khm_td.SampleDesc.Count = 1;             // single-sample: the mask is a per-pixel question,
                                                 // And the world pass reads it per pixel.
        khm_td.Usage = D3D11_USAGE_DEFAULT;
        khm_td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bool khm_ok = SUCCEEDED(khm_dev->CreateTexture2D(&khm_td, nullptr, &g_res.dlsw_tex));
        if (khm_ok) khm_ok = SUCCEEDED(khm_dev->CreateRenderTargetView(g_res.dlsw_tex, nullptr, &g_res.dlsw_rtv));
        if (khm_ok) khm_ok = SUCCEEDED(khm_dev->CreateShaderResourceView(g_res.dlsw_tex, nullptr, &g_res.dlsw_srv));
        khm_dev->Release();
        if (!khm_ok) {
            KH_SAFE_RELEASE(g_res.dlsw_srv);
            KH_SAFE_RELEASE(g_res.dlsw_rtv);
            KH_SAFE_RELEASE(g_res.dlsw_tex);
            return false;
        }
        g_res.dlsw_w = khm_w;
        g_res.dlsw_h = khm_h;
    }

    // View * proj, row-vector, with the engine's own projection (we store clip w: see the head note).
    float khm_proj[4][4];
    memcpy(khm_proj, khm_proj_in, sizeof(khm_proj));
    float khm_v[4][4];
    memcpy(khm_v, khm_view, sizeof(khm_v));
    float khm_vp[4][4];
    mul_4x4(khm_v, khm_proj, khm_vp);

    ConstantData khm_frm = {};
    memcpy(khm_frm.view_proj, khm_vp, sizeof(khm_frm.view_proj));

    // Rebase: the mask must adopt centerRel like every other consumer of that geometry.
    float khm_cam[3] = { 0.0f, 0.0f, 0.0f };
    kh_cam_of(reinterpret_cast<const float(*)[4]>(khm_view), khm_cam);
    // KH_DLSW_ENGCAM: the meshes this mask must cover were drawn through the engine's own camera whenever the lock held
    // (the injection's khcs_engcam, the flush's khf_engcam); the fold's float camera alone puts the mask beside the
    // drawn silhouette far from the map origin (1 - 6 px at 2 m, 15 - 30 km). The same choice here, through
    // KH_ENGCAM_PEEK: the frame's latched value when this view agrees with it, else the value the consume would take
    // for this view - with none of the consume's side effects (this pass can run before the flush, whose view differs
    // on a sparse cycle that has not landed), so no drawer's verdict changes. False: the fold.
    const bool khm_eng = kh_engcam_peek(reinterpret_cast<const float(*)[4]>(khm_view), khm_cam);
    const bool khm_reb = khm_eng ? kh_rebase_vp_engcam(khm_frm, khm_v, khm_proj) : kh_rebase_vp(khm_frm, khm_cam);

    // StateBackup owes the restore: without it the world pass's own capture would record our shaders and min-blend as
    // the engine's and hand them to every engine draw after the resolve.
    StateBackup khm_bk;
    khm_bk.capture(khm_ctx);
    KhBindScope khm_bc(khm_ctx);   // KH_BIND_FILTER.
    KhOmSave khm_om;
    khm_om.capture(khm_ctx);
    UINT khm_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khm_ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    khm_ctx->RSGetViewports(&khm_nvp, khm_ovp);

    // The sentinel is the clear: every pixel starts further away than anything can be, so min-blending a caster in
    // always wins and an untouched pixel reads as "no mesh". Uploaded inside the bracket: kh_upload_frame_cb rewrites
    // composite_frame_cb, and the bracket's restore puts the engine's binding back.
    if (!kh_upload_frame_cb(khm_ctx, g_res.composite_frame_cb, khm_frm)) {
        khm_om.restore(khm_ctx);
        khm_om.release();
        khm_bk.restore(khm_ctx);
        return false;
    }

    const FLOAT khm_far4[4] = { 1.0e30f, 1.0e30f, 1.0e30f, 1.0e30f };
    khm_ctx->OMSetRenderTargets(1, &g_res.dlsw_rtv, nullptr);
    khm_ctx->ClearRenderTargetView(g_res.dlsw_rtv, khm_far4);
    D3D11_VIEWPORT khm_vp2 = {};
    khm_vp2.Width = static_cast<FLOAT>(khm_w);
    khm_vp2.Height = static_cast<FLOAT>(khm_h);
    khm_vp2.MaxDepth = 1.0f;
    khm_ctx->RSSetViewports(1, &khm_vp2);

    // b0 the object CB, b1 the frame CB (viewProj), to both stages.
    ID3D11Buffer* khm_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    khm_ctx->VSSetConstantBuffers(0, 2, khm_cbs);
    khm_ctx->PSSetConstantBuffers(0, 2, khm_cbs);
    khm_ctx->IASetInputLayout(g_res.input_layout);
    khm_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    khm_ctx->VSSetShader(g_res.vs_dls_mask, nullptr, 0);
    khm_ctx->PSSetShader(g_res.ps_dls_mask, nullptr, 0);
    khm_ctx->GSSetShader(nullptr, nullptr, 0);
    khm_ctx->HSSetShader(nullptr, nullptr, 0);
    khm_ctx->DSSetShader(nullptr, nullptr, 0);
    khm_ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT khm_bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    khm_ctx->OMSetBlendState(g_res.dlsw_min_blend, khm_bf, 0xFFFFFFFF);
    khm_ctx->RSSetState(g_res.rasterizer);

    const UINT khm_stride = sizeof(MeshVertex);
    KhVbRef khm_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
    uint32_t khm_drawn = 0;
    // The device for the material binds; the alpha twin is optional (absent, an alpha caster masks whole).
    ID3D11Device* khm_dev = nullptr;
    khm_ctx->GetDevice(&khm_dev);
    const bool khm_alpha_ok = khm_dev && g_res.ps_dls_mask_a && g_res.vs_tex && g_res.layout_tex;
    bool khm_alpha_bound = false;
    // KH_INFRONT: a view-model mesh sits in the slice's depth, and the world projection clips it short of the world
    // near plane, so it would leave the mask and take the world-receive multiply on top of its own DLS term. It is
    // masked in a second pass under the hands pair (column 2 of the same rebased matrix): x / y / w are the world's, z
    // lands in [0, 1] from 0.01 m, and the mask is metres (clip w) either way.
    bool khm_front_any = false;
    for (const KhDlswCaster& khm_c0 : g_dlsw_casters) {
        if (khm_c0.in_front && khm_c0.visible && khm_c0.alpha >= 0.999f && !khm_c0.no_foot) khm_front_any = true;
    }
    for (int khm_pass = 0; khm_pass < 2; ++khm_pass) {
    if (khm_pass == 1) {
        if (!khm_front_any) break;
        float khm_m22 = 0.0f, khm_m32 = 0.0f;
        if (!kh_infront_pair(khm_m22, khm_m32)) break;
        ConstantData khm_frm2 = khm_frm;
        for (int khm_r = 0; khm_r < 3; ++khm_r) khm_frm2.view_proj[khm_r][2] = khm_m22 * khm_frm2.view_proj[khm_r][3];
        khm_frm2.view_proj[3][2] = khm_m22 * khm_frm2.view_proj[3][3] + khm_m32;
        if (!kh_upload_frame_cb(khm_ctx, g_res.composite_frame_cb, khm_frm2)) break;
    }
    for (size_t khm_i = 0; khm_i < g_dlsw_casters.size(); ++khm_i) {
        const KhDlswCaster& khm_c = g_dlsw_casters[khm_i];
        if (khm_c.in_front != (khm_pass == 1)) continue;   // KH_INFRONT: its own pass.
        // Only what owns pixels is masked. An invisible (casterOnly) object and a whole translucent one write no depth,
        // so the surface at their pixels is the world behind them, which must receive the shadow they cast. A
        // see-through blend mode (KH_FOOT_BLEND) may write depth and still shows the world behind it at every texel:
        // the same rule.
        if (!khm_c.visible || khm_c.alpha < 0.999f) continue;
        if (khm_c.no_foot) continue;   // KH_FOOT_BLEND: the world behind shows through.
        // The size test is not optional: khm_c.mesh is bounded against the published registry (mesh_count), not against
        // mesh_vb, and those two are equal only while ensure_mesh_vbs keeps up (it returns early on an empty mesh or a
        // failed CreateBuffer after the published count was already raised).
        if (khm_c.mesh < 0 ||
            static_cast<size_t>(khm_c.mesh) >= g_res.mesh_vb.size() ||
            !g_res.mesh_vb[khm_c.mesh]) continue;
        const KhVbRef khm_want = kh_mesh_vb_ref(khm_c.mesh, khm_c.slot);   // KH_CLOTH.
        if (!khm_want) continue;
        ConstantData khm_obj = {};
        // SQF [x, y, zASL] -> engine [x, zASL, y].
        khm_obj.center_size[0] = khm_c.pos[0];
        khm_obj.center_size[1] = khm_c.pos[2];
        khm_obj.center_size[2] = khm_c.pos[1];
        khm_obj.size_axes[0] = khm_c.size[0];
        khm_obj.size_axes[1] = khm_c.size[2];
        khm_obj.size_axes[2] = khm_c.size[1];
        // Always the rotation, never null: kh_fill_obj_rot dereferences unconditionally, and an unrotated caster
        // carries an identity in rot[] anyway.
        kh_fill_obj_rot(khm_obj, khm_c.rot);
        // Armed only when the matrix above actually rebased: the two must never disagree, or the vertex would be
        // camera-relative against an absolute matrix.
        if (khm_reb) kh_fill_center_rel(khm_obj, khm_cam, khm_c.res);   // KH_POS_RES.
        if (!kh_upload_obj_cb(khm_ctx, g_res.composite_cb, khm_obj)) break;
        // An alpha caster with a material set masks through the textured VS and the clipping twin, its material bound
        // per submesh. Colour-alpha-only casters that reached here carry alpha >= 0.999: the plain pair.
        const bool khm_want_alpha = khm_c.alpha_caster && khm_c.mats && khm_alpha_ok;
        if (khm_want_alpha != khm_alpha_bound) {
            khm_ctx->IASetInputLayout(khm_want_alpha ? g_res.layout_tex : g_res.input_layout);
            khm_ctx->VSSetShader(khm_want_alpha ? g_res.vs_tex : g_res.vs_dls_mask, nullptr, 0);
            khm_ctx->PSSetShader(khm_want_alpha ? g_res.ps_dls_mask_a : g_res.ps_dls_mask, nullptr, 0);
            if (khm_want_alpha && g_res.mat_sampler) khm_ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
            khm_alpha_bound = khm_want_alpha;
        }
        if (khm_want != khm_bound_vb) {
            khm_ctx->IASetVertexBuffers(0, 1, &khm_want.vb, &khm_stride, &khm_want.off);
            khm_ctx->IASetIndexBuffer(g_res.mesh_ib[khm_c.mesh], DXGI_FORMAT_R32_UINT, 0);
            khm_bound_vb = khm_want;
        }
        // The mask covers whatever the colour pass drew: the level kh_lod_pick chose, and during a fade both levels,
        // each cut by the colour pass's dither (KH_DLSW_MASK_DITHER), so the mask at a pixel holds the level that pixel
        // shows. Same pick, same inputs on a cycle the injection drew (it published g_lod_proj_px before this resolve).
        // Known residual: a cycle whose meshes the flush draws late publishes after this pass (flush_locked runs after
        // kh_dls_world_pass), so in a frame whose projection changed (a zoom, a FOV change) the pick here used the last
        // value: the mask's level - or, during a fade, its dither - can differ from the colour pass's, and the mask
        // then holds another level's outline than the one shown, for that frame. Not the shadow LOD. An inFront mesh is
        // the slice's, drawn at level 0 only (KH_INFRONT_LOD).
        {
            const MeshDef& khm_md = mesh_def(khm_c.mesh);
            int   khm_lvl = 0;
            float khm_lt = 0.0f;
            if (!khm_c.lod_lock && !khm_c.in_front) {
                kh_lod_pick(khm_md, khm_c.lod_r,   // KH_SKEL_LOD_R.
                            sqrtf(kh_mesh_dist_sq_of(khm_c.pos, khm_c.size, khm_c.rot, khm_c.rotated, khm_cam)),
                            khm_lvl, khm_lt);
            }
            const int khm_iters = khm_lt > 0.0f ? 2 : 1;
            bool khm_ok = true;
            for (int khm_it = 0; khm_it < khm_iters && khm_ok; ++khm_it) {
                const int khm_lv = mesh_lod_clamp(khm_md, khm_lvl + khm_it);
                // KH_DLSW_MASK_DITHER: a fading level takes the dither lane the injection's crossfade loop gives it
                // (kh_lod_dither: the finer level first), which PSDlsMask (blendCtl.w) and PSDlsMaskA (its object lane)
                // cut by. Do not mask the union of both levels: it claims the world's pixels inside the hidden level's
                // outline as ours, and KhDlsMaskCov keeps our dynamic-light shadow off them for the fade. The alpha
                // route uploads khm_obj per submesh below; the plain route uploads it here. khm_obj is this caster's
                // own, so the lane leaks into no other caster.
                if (khm_iters == 2) {
                    khm_obj.blend_ctl[3] = kh_lod_dither(khm_lt, khm_it != 0);
                    if (!khm_want_alpha) {
                        khm_ok = kh_upload_obj_cb(khm_ctx, g_res.composite_cb, khm_obj);
                        if (!khm_ok) break;
                    }
                }
                if (khm_want_alpha) {   // Per submesh of the level, the material's lanes + maps bound.
                    const std::vector<MeshSubmesh>& khm_tab = mesh_lod_submeshes(khm_md, khm_lv);
                    for (size_t khm_s = 0; khm_s < khm_tab.size() && khm_ok; ++khm_s) {
                        const MeshSubmesh& khm_sm = khm_tab[khm_s];
                        if (khm_sm.index_count == 0) continue;
                        kh_bind_material(khm_ctx, khm_dev, khm_obj, khm_c.mats, khm_s);
                        khm_ok = kh_upload_obj_cb(khm_ctx, g_res.composite_cb, khm_obj);
                        if (!khm_ok) break;
                        khm_ctx->DrawIndexed(khm_sm.index_count, khm_sm.index_start, 0);
                    }
                } else {
                    UINT khm_ls = 0, khm_lc = 0;
                    mesh_lod_range(khm_md, khm_lv, khm_ls, khm_lc);
                    if (khm_lc == 0) continue;
                    khm_ctx->DrawIndexed(khm_lc, khm_ls, 0);
                }
            }
            if (!khm_ok) break;
        }
        khm_drawn++;
    }
    }   // KH_INFRONT: the two mask passes.
    if (khm_dev) khm_dev->Release();

    if (khm_nvp > 0) khm_ctx->RSSetViewports(khm_nvp, khm_ovp);
    khm_om.restore(khm_ctx);
    khm_om.release();
    khm_bk.restore(khm_ctx);   // Shaders, layout, blend, dss, raster, CBs.
    return khm_drawn > 0;
}

// KH_DLSW_FOG_FILL: the world pass's fog lanes on a cycle whose mesh frame template is not this cycle's. The injection
// publishes the template, but it can bail after the dynamic-light select (the near-gap partition, an anomalous cycle);
// the flush then carries the cycle and publishes the template - after the world pass, which runs ahead of it at the
// same resolve. This is the flush's fog-lane fill (kh_fx_frame_template) verbatim, on the same live sources at the same
// instant, so the lanes are the ones the flush's meshes fog with this cycle. Deliberate difference: fogBelow.z is not
// written (the cast fire's lane; the template copy never writes it). TWIN: the injection's and the flush's fog fills -
// an edit there is an edit here.
inline void kh_dlsw_fog_lanes(ConstantData& khdf_cbf, const float* cam) {
    const float khdf_cam_y =
        (g_ls.cam[0] * g_ls.cam[0] + g_ls.cam[1] * g_ls.cam[1] +
         g_ls.cam[2] * g_ls.cam[2] >= 1.0f) ? g_ls.cam[1]
      : (cam[0] * cam[0] + cam[1] * cam[1] + cam[2] * cam[2] >= 1.0f)
         ? cam[1] : 0.0f;
    // The TRUE camera altitude, unclamped. The shaders arm their below-layer branch on camY < fogSkyCol.w (static.hlsl,
    // composite.hlsl), so a lane clamped at the layer would arm that branch nowhere: the ray is split shader-side, not
    // here.
    const float khdf_fog_cam_y = khdf_cam_y;
    // fogBelow: x = the engine's below-layer extinction (block nb[51]), y = armed. z is not written here (the mesh
    // fills write 0; it is the cast fire's lane, which the world pass never carries); w is never written.
    khdf_cbf.fog_below[0] = (g_light_probe.hits > 0 && g_light_probe.meta == 40)   // KH_PROBE_META40.
                          ? g_light_probe.nb[51] : 0.0f;
    khdf_cbf.fog_below[1] = 1.0f;
    const bool khdf_fog_on = g_fog_valid && g_fog[0] > 1e-4f;
    const bool khdf_eng_ok = g_light_probe.hits > 0 && g_light_probe.meta == 40;
    kh_fill_haze(khdf_cbf);

    if (khdf_fog_on || khdf_cbf.haze_pars[3] >= 0.5f || khdf_eng_ok) {
        khdf_cbf.fog_params[0] = khdf_fog_on ? g_fog[0] : 0.0f;
        khdf_cbf.fog_params[1] = g_fog[1];
        khdf_cbf.fog_params[2] = g_fog[2];
        khdf_cbf.fog_params[3] = khdf_fog_on ? 1.0f : 0.0f;

        // Fog is decoupled from the block lock, so a degenerate staged decay or the pre-lock cold window does not leave
        // the mesh unfogged in a fogged world.
        if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
            const float* e = g_light_probe.nb + 36;

            for (int c = 0; c < 3; ++c) khdf_cbf.fog_color[c] = e[c];

            // Engine transmittance terms from the block mirror: the density scale, the linear ramp's end + inverse
            // range.
            khdf_cbf.fog_engine[0] = g_light_probe.nb[41];
            khdf_cbf.fog_engine[1] = g_light_probe.nb[48];
            khdf_cbf.fog_engine[2] = g_light_probe.nb[49];
            const bool khdf_dec_fresh =
                g_light_probe.hits > 0 &&
                (effect_time_seconds() - g_light_probe.mirror_confirm) < 0.5f;   // KH_DEC_MIRROR_CLOCK.
            const bool khdf_dec_eng = khdf_dec_fresh && g_light_probe.nb[40] >= 0.0f && g_light_probe.nb[40] < 1.0f;
            if (khdf_dec_eng) {
                khdf_cbf.fog_params[1] = g_light_probe.nb[40];
            }
            const bool khdf_terms = g_light_probe.nb[48] > 1.0f &&
                                    g_light_probe.nb[49] > 0.0f &&
                                    g_light_probe.nb[41] >= 0.0f;
            const float khdf_rp_end = g_light_probe.nb[48];
            const float khdf_rp_inv = g_light_probe.nb[49];
            const float khdf_rp_prod = khdf_rp_end * khdf_rp_inv;
            const bool khdf_rp_incoh = khdf_terms && khdf_rp_prod < 1.0f;
            const bool khdf_ramp_off = khdf_rp_incoh;
            khdf_cbf.fog_engine[3] = !khdf_terms ? 0.0f : (khdf_ramp_off ? 2.0f : 1.0f);
        } else {
            khdf_cbf.fog_color[0] = 1.0f;
            khdf_cbf.fog_color[1] = 1.0f;
            khdf_cbf.fog_color[2] = 1.0f;
        }

        if (g_sky_probe.hits > 0) {
            khdf_cbf.fog_sky[0] = 1.0f;
            khdf_cbf.fog_sky[1] = 1.0f;
            khdf_cbf.fog_sky[2] = 1.0f;
            khdf_cbf.fog_sky[3] = 1.0f;
            khdf_cbf.fog_sky_col[0] = g_sky_probe.nb[4];
            khdf_cbf.fog_sky_col[1] = g_sky_probe.nb[5];
            khdf_cbf.fog_sky_col[2] = g_sky_probe.nb[6];
            // The below-layer convergence colour: nb[28..30] = the sky fog colour, nb[68..70] = its elevation gradient.
            // Pure captures, inert unless the shader is below the layer.
            khdf_cbf.fog_uw[0] = g_sky_probe.nb[28];
            khdf_cbf.fog_uw[1] = g_sky_probe.nb[29];
            khdf_cbf.fog_uw[2] = g_sky_probe.nb[30];
            khdf_cbf.fog_uw[3] =
                1.0f;
            khdf_cbf.fog_uw_grad[0] = g_sky_probe.nb[68];
            khdf_cbf.fog_uw_grad[1] = g_sky_probe.nb[69];
            khdf_cbf.fog_uw_grad[2] = g_sky_probe.nb[70];
        }

        // Camera altitude (engine Y-up), unclamped - see khdf_fog_cam_y.
        khdf_cbf.fog_color[3] = khdf_fog_cam_y;
    }
}

inline void kh_dls_world_pass_body(ID3D11DeviceContext* khw_ctx) {

    // A light must hold a slot and have a map, or the pass is a fullscreen multiply by 1.
    // KH_DLS_SRV_GATE: no map view (released with the device) = no map (kh_dls_published).
    if (g_dls_n == 0 || g_dls_map_valid == 0ull || !g_res.dls_srv) {  return; }
    // The maps, the slots and the mask's owners are kh_dls_frame's, run by the injection (this cycle) or by the flush
    // after this pass (the previous one's then). Older, no mesh pass has run it since - a hidden or deleted last
    // caster's shadow would stay painted, drawn with casters that may be gone.
    if (g_dls_frame_cycle == ~0ull || g_topo_cycles - g_dls_frame_cycle > 1u) return;

    if (!g_res.ps_dls_world || !g_res.vs_fullscreen || !g_res.rasterizer ||
        !g_res.dss_off || !g_res.dls_world_blend || !g_res.composite_cb ||
        !g_res.composite_frame_cb) {
        return;
    }

    ID3D11RenderTargetView* khw_rtv = nullptr;
    khw_ctx->OMGetRenderTargets(1, &khw_rtv, nullptr);
    if (!khw_rtv) {  return; }
    khw_rtv->Release();

    // g_mask.cast_depth is the engine's own live resolved linear depth, adopted by identity at the resolve sweep and
    // validated there as a screen-sized single-channel float.
    if (!g_mask.cast_depth || g_mask.cast_dims[0] < 2.0f || g_mask.cast_dims[1] < 2.0f) {
        return;
    }

    ConstantData khw_obj = {};
    ConstantData khw_frm = {};

    kh_dls_fill_cb(khw_frm);   // dls_meta / dls_ctl / dls_face_slice / dls_spot_vp.

    // Engine sun + sky, for KhDlsWorldFactor's denominator. These are FRAME-half lanes (past view_proj) and the frame
    // CB uploads from khw_frm, so they MUST be written to khw_frm - on khw_obj they land in the never-uploaded tail,
    // Ltotal sums with zero sun and sky, and the shadow paints near-black in daylight. lighting0[2]/[3] are OBJECT-half
    // and stay on khw_obj. The else arm twins fill_lighting_frame_cb's fallback (white, fails toward under-darkening).
    if (g_pub_block_valid) {
        khw_frm.lighting2[0] = g_pub_block_sun[0];
        khw_frm.lighting2[1] = g_pub_block_sun[1];
        khw_frm.lighting2[2] = g_pub_block_sun[2];
        khw_frm.light_amb[0] = g_pub_block_amb[0];
        khw_frm.light_amb[1] = g_pub_block_amb[1];
        khw_frm.light_amb[2] = g_pub_block_amb[2];
        khw_frm.light_amb[3] = 1.0f;
    } else {
        khw_frm.lighting2[0] = 1.0f;
        khw_frm.lighting2[1] = 1.0f;
        khw_frm.lighting2[2] = 1.0f;
        khw_frm.light_amb[0] = 1.0f;
        khw_frm.light_amb[1] = 1.0f;
        khw_frm.light_amb[2] = 1.0f;
        khw_frm.light_amb[3] = 0.0f;
    }
    // KH_DLSW_GEN_AMB: never below the brightest lit world cluster of the last world frame, per channel (see
    // g_blko_gen_valid): the denominator stays generous whichever cluster the publish picked for this camera.
    if (g_blko_gen_valid) {
        for (int khw_c = 0; khw_c < 3; ++khw_c) {
            khw_frm.lighting2[khw_c] = fmaxf(khw_frm.lighting2[khw_c], g_blko_gen_sun[khw_c]);
            khw_frm.light_amb[khw_c] = fmaxf(khw_frm.light_amb[khw_c], g_blko_gen_amb[khw_c]);
        }
    }
    khw_obj.lighting0[2] = 1.0f;   // Ambient scale: the world's own, not an object's.
    khw_obj.lighting0[3] = 1.0f;
    khw_frm.lighting1[0] = g_sun_dir_engine[0];
    khw_frm.lighting1[1] = g_sun_dir_engine[1];
    khw_frm.lighting1[2] = g_sun_dir_engine[2];

    // Do not paint on our own meshes. The mask projects world -> pixel with the camera our meshes were drawn with and
    // the reconstruction maps pixel -> world; khw_frozen picks only the reconstruction's inputs (the fire's frozen
    // view, inverse and fov below), and the mask is drawn on frozen and live frames alike.
    // KH_DLSW_LATCH: that camera is the one the flush takes. A cycle that has landed uses the landing's own view
    // (g_inj_view, fresh), which carries the injection's jump fallback; otherwise the cycle latch (KH_LATCH_FOLD), or
    // the live view without one, with the engine's frame view adopted. One exception: on a sparse cycle that has not
    // landed (flush_locked's khf_sparse_live) the flush draws with the live view, and this pass keeps the latch, the
    // camera of the engine's depth it reconstructs. The live read keeps only its projection (the injection takes the x
    // and y scales live too). Do not take the live view where a landing or the cycle latch exists: it is a later camera
    // than the one the meshes and the engine's depth were drawn with, so the mask and the reconstruction slide off them
    // while the camera turns.
    RVExtBridge::ProjectionViewTransform khw_pv = {};
    bool khw_pv_ok = RVExtBridge::get_projection_view_transform(khw_pv);
    if (g_ro.cycle_pv_valid) {
        if (khw_pv_ok) memcpy(&khw_pv.view[0][0], &g_ro.cycle_pv.view[0][0], sizeof(khw_pv.view));
        else           khw_pv = g_ro.cycle_pv;
        khw_pv_ok = true;
    }
    if (!khw_pv_ok) {  return; }
    if (g_composite_land_cycle == g_topo_cycles &&
        g_inj_view_ms != 0 && steady_now_ms() - g_inj_view_ms < 100) {
        memcpy(&khw_pv.view[0][0], g_inj_view, sizeof(g_inj_view));
    } else {
        kh_adopt_frame_view(khw_pv);
    }

    const bool khw_frozen = g_fire_lock_valid && g_fire_cast_inv_valid;
    float khw_fov[2] = { 0.0f, 0.0f };
    bool  khw_fov_ok = false;
    if (khw_frozen && g_fire_fov2[0] > 1.0e-6f && g_fire_fov2[1] > 1.0e-6f) {
        khw_fov[0] = g_fire_fov2[0];
        khw_fov[1] = g_fire_fov2[1];
        khw_fov_ok = true;
    } else if (g_mask.cast_fov_valid && g_mask.cast_fov[0] > 1.0e-6f &&
               g_mask.cast_fov[1] > 1.0e-6f) {
        khw_fov[0] = g_mask.cast_fov[0];
        khw_fov[1] = g_mask.cast_fov[1];
        khw_fov_ok = true;
    }

    // The mask registers with the framebuffer, not with the reconstruction: the reconstruction turns a depth texel into
    // a world point, the mask rasterizes geometry - they share a view but not a job. Sharing one fov gives a white rim.
    khw_frm.mir_meta[0] = 0.0f;
    float khw_mv[16] = {};
    float khw_mp[16] = {};
    bool  khw_mok = false;

    // The meshes' camera (KH_DLSW_LATCH above).
    memcpy(khw_mv, &khw_pv.view[0][0], sizeof(khw_mv));
    memcpy(khw_mp, &khw_pv.projection[0][0], sizeof(khw_mp));
    khw_mok = true;

    if (khw_mok &&
        kh_dlsw_mask_render(khw_ctx, khw_mv, khw_mp,
                            static_cast<UINT>(g_mask.cast_dims[0]),
                            static_cast<UINT>(g_mask.cast_dims[1]))) {
        khw_frm.mir_meta[0] = 1.0f;
        khw_frm.mir_meta[1] = static_cast<float>(g_res.dlsw_w);
        khw_frm.mir_meta[2] = static_cast<float>(g_res.dlsw_h);
    }

    {
        float khw_cam[3] = { 0.0f, 0.0f, 0.0f };
        kh_cam_of(khw_pv.view, khw_cam);   // KH_CAM_HONEST (re-derived below from castMat, the same point).
        float khw_inv[3][3];
        const bool khw_inv_ok = !khw_frozen && kh_view_inv_rows(khw_pv.view, khw_inv);

        for (int khw_r = 0; khw_r < 3; ++khw_r) {
            khw_frm.cast_view[0][khw_r] = khw_frozen ? g_fire_view2[12 + khw_r]
                                                     : khw_pv.view[3][khw_r];
            if (khw_frozen) {
                // The cast's own inverse, cofactor-derived in double and laid out for KhCastWorld's row dots.
                khw_frm.cast_mat[khw_r][0] = g_fire_cast_inv[khw_r * 3 + 0];
                khw_frm.cast_mat[khw_r][1] = g_fire_cast_inv[khw_r * 3 + 1];
                khw_frm.cast_mat[khw_r][2] = g_fire_cast_inv[khw_r * 3 + 2];
            } else {
                // KH_CAM_HONEST: the view's cofactor inverse rotation, as the freeze takes its own; the transpose
                // (the inverse of an exactly orthonormal rotation) only where that is refused.
                khw_frm.cast_mat[khw_r][0] = khw_inv_ok ? khw_inv[khw_r][0] : khw_pv.view[khw_r][0];
                khw_frm.cast_mat[khw_r][1] = khw_inv_ok ? khw_inv[khw_r][1] : khw_pv.view[khw_r][1];
                khw_frm.cast_mat[khw_r][2] = khw_inv_ok ? khw_inv[khw_r][2] : khw_pv.view[khw_r][2];
            }
            khw_frm.cast_mat[khw_r][3] = 0.0f;   // KH_CAST_OCC lane hygiene.
        }
        khw_frm.cast_view[0][3] = 1.0f;
        // KhCastWorld multiplies NDC x by castView[1].x, so this lane is tan(fovX/2): the fire's frozen fov on a frozen
        // frame, else the engine's own constant block (g_mask.cast_fov); derived from the projection only when neither
        // is valid.
        if (khw_fov_ok) {
            khw_frm.cast_view[1][0] = khw_fov[0];
            khw_frm.cast_view[1][1] = khw_fov[1];
        } else {
            khw_frm.cast_view[1][0] = (khw_pv.projection[0][0] != 0.0f)
                                    ? 1.0f / khw_pv.projection[0][0] : 1.0f;
            khw_frm.cast_view[1][1] = (khw_pv.projection[1][1] != 0.0f)
                                    ? 1.0f / khw_pv.projection[1][1] : 1.0f;
        }
        khw_frm.cast_view[1][2] = g_mask.cast_dims[0];
        khw_frm.cast_view[1][3] = g_mask.cast_dims[1];
        // The camera the normal's facing test uses must come from the same view the reconstruction did. Derived by the
        // reconstruction's own algebra: with zl = 0, KhCastWorld reduces to (-translation) dotted through the inverse
        // rotation. KH_CAM_HONEST: in both branches, so the camera is castMat's whichever it holds.
        for (int khw_r = 0; khw_r < 3; ++khw_r) {
            khw_cam[khw_r] = -(khw_frm.cast_view[0][0] * khw_frm.cast_mat[khw_r][0] +
                               khw_frm.cast_view[0][1] * khw_frm.cast_mat[khw_r][1] +
                               khw_frm.cast_view[0][2] * khw_frm.cast_mat[khw_r][2]);
        }
        khw_frm.cast_view[2][0] = khw_cam[0];
        khw_frm.cast_view[2][1] = khw_cam[1];
        khw_frm.cast_view[2][2] = khw_cam[2];
    }

    if (khw_frm.dls_range[3] > 0.0f) {
        khw_frm.dls_range[0] = khw_frm.cast_view[2][0];
        khw_frm.dls_range[1] = khw_frm.cast_view[2][1];
        khw_frm.dls_range[2] = khw_frm.cast_view[2][2];
    }

    // KH_DLSW_FOG: the engine fogs every pixel in its forward pass, so the colour this pass multiplies already carries
    // the fog and haze in front of the surface, and a bare multiply darkens those too - a shadow darker than the unlit
    // ground beside it, by fog x (1 - transmittance) x the blocked share: worst at dusk, growing with distance and
    // changing with the view ray's elevation. The fog lanes are the mesh passes' own (this cycle's frame template: the
    // numbers our meshes fog with), and PSDlsWorldFog puts the in-scattered share back through a dual-source blend,
    // dest * F + fog * (1 - T) * (1 - F), which is the fogged shadowed surface exactly. No template this cycle (the
    // injection bailed; the flush that publishes one runs after this pass): the flush's own fill, now
    // (KH_DLSW_FOG_FILL). Fog disarmed in the lanes: the added term is 0, the plain multiply.
    if (g_pip.tpl_valid && g_pip.tpl_cycle == g_topo_cycles) {
        memcpy(khw_frm.fog_params, g_pip.tpl.fog_params, sizeof(khw_frm.fog_params));
        memcpy(khw_frm.fog_color, g_pip.tpl.fog_color, sizeof(khw_frm.fog_color));
        memcpy(khw_frm.fog_engine, g_pip.tpl.fog_engine, sizeof(khw_frm.fog_engine));
        memcpy(khw_frm.fog_sky, g_pip.tpl.fog_sky, sizeof(khw_frm.fog_sky));
        memcpy(khw_frm.fog_sky_col, g_pip.tpl.fog_sky_col, sizeof(khw_frm.fog_sky_col));
        memcpy(khw_frm.haze_pars, g_pip.tpl.haze_pars, sizeof(khw_frm.haze_pars));
        memcpy(khw_frm.fog_uw, g_pip.tpl.fog_uw, sizeof(khw_frm.fog_uw));
        memcpy(khw_frm.fog_uw_grad, g_pip.tpl.fog_uw_grad, sizeof(khw_frm.fog_uw_grad));
        khw_frm.fog_below[0] = g_pip.tpl.fog_below[0];   // [2] stays the cast fire's alone.
        khw_frm.fog_below[1] = g_pip.tpl.fog_below[1];
    } else {
        kh_dlsw_fog_lanes(khw_frm, khw_frm.cast_view[2]);   // KH_DLSW_FOG_FILL.
    }

    StateBackup khw_bk;
    khw_bk.capture(khw_ctx);
    // The engine's dynamic-light pool, in the same lanes DynLights reads, copied from the last mesh fill's source
    // rather than re-derived: one source, so the world cannot disagree with the meshes about which lights exist or
    // cast. After the capture: the ring append binds PS t40 (KH_DL_RING), and this pass runs on the engine's state with
    // no bracket around it, so the bind must fall inside this one's restore. Nothing that can block (no published
    // casting light, a stale pool, a failed append): the factor is 1 at every pixel, so nothing is drawn.
    if (!kh_dls_fill_dl_world(khw_ctx, khw_obj)) {
        khw_bk.restore(khw_ctx);
        return;
    }
    ID3D11ShaderResourceView* khw_old_t37 = nullptr;   // Inside StateBackup's t0..t69 too; the explicit put-back stays.
    khw_ctx->PSGetShaderResources(37, 1, &khw_old_t37);

    // b0 and b1 bound as (0, 2), before the uploads; on a device without the 11.1 ring it never binds. StateBackup
    // saved both slots offset-aware and restores both.
    ID3D11Buffer* khw_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    khw_ctx->VSSetConstantBuffers(0, 2, khw_cbs);
    khw_ctx->PSSetConstantBuffers(0, 2, khw_cbs);
    if (!kh_upload_obj_cb(khw_ctx, g_res.composite_cb, khw_obj) ||
        !kh_upload_frame_cb(khw_ctx, g_res.composite_frame_cb, khw_frm)) {
        KH_SAFE_RELEASE(khw_old_t37);
        khw_bk.restore(khw_ctx);   // The bracket is open: close it.
        return;
    }

    khw_ctx->PSSetShaderResources(0, 1, &g_mask.cast_depth);
    if (g_res.dls_srv) khw_ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    khw_ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // KH_SHADOW_GATHER (khw_bk restores s1).
    if (khw_frm.mir_meta[0] >= 0.5f && g_res.dlsw_srv) {
        khw_ctx->PSSetShaderResources(37, 1, &g_res.dlsw_srv);   // KH_DLSW_MASK, metres.
    }
    khw_ctx->IASetInputLayout(nullptr);
    khw_ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    khw_ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    // KH_DLSW_FOG: the dual-source form needs its shader, its blend state and render target 0 alone (a second source
    // blends into RT0 only; with another target bound its SV_Target1 would be written there); the fog lanes are this
    // cycle's on every path above. Anything missing: the plain multiply.
    bool khw_fog = g_res.ps_dls_world_fog && g_res.dls_world_fog_blend;
    if (khw_fog) {
        ID3D11RenderTargetView* khw_rts[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        khw_ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, khw_rts, nullptr);
        for (UINT khw_k = 0; khw_k < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khw_k) {
            if (khw_k > 0 && khw_rts[khw_k]) khw_fog = false;
            KH_SAFE_RELEASE(khw_rts[khw_k]);
        }
    }
    khw_ctx->PSSetShader(khw_fog ? g_res.ps_dls_world_fog : g_res.ps_dls_world, nullptr, 0);
    khw_ctx->GSSetShader(nullptr, nullptr, 0);
    khw_ctx->HSSetShader(nullptr, nullptr, 0);
    khw_ctx->DSSetShader(nullptr, nullptr, 0);
    khw_ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    // Dest * src with a unit blend factor, so SRC_COLOR is the multiplier.
    const FLOAT khw_bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    khw_ctx->OMSetBlendState(khw_fog ? g_res.dls_world_fog_blend : g_res.dls_world_blend, khw_bf, 0xFFFFFFFF);
    khw_ctx->RSSetState(g_res.rasterizer);
    khw_ctx->Draw(3, 0);

    // t0 held the depth SRV; the engine rebinds that resource as a DSV immediately after the resolve, and a lingering
    // SRV would be force-nulled with a debug-layer warning storm.
    ID3D11ShaderResourceView* khw_null = nullptr;
    khw_ctx->PSSetShaderResources(0, 1, &khw_null);
    khw_ctx->PSSetShaderResources(37, 1, &khw_old_t37);   // Explicit put-back (StateBackup's restore covers it too)
    KH_SAFE_RELEASE(khw_old_t37);   // Exactly what was bound (t35 pattern).
    khw_ctx->PSSetShaderResources(36, 1, &khw_null);   // Inside it; explicit like t0 and t37.
    khw_bk.restore(khw_ctx);
}

// The resolve hook evaluates !g_ro.in_injection before calling here, so the flag is raised inside. Without it the
// mask's state sets and draws would flow through the hooks as engine draws.
inline void kh_dls_world_pass(ID3D11DeviceContext* khw_ctx) {
    KH_PROF_SCOPE(KHP_DLS_WORLD);
    KH_GPU_SCOPE(khw_ctx, KHG_DLS_WORLD);   // KH_PROF.
    if (!khw_ctx) return;
    const bool khw_pinj = g_ro.in_injection;
    g_ro.in_injection = true;
    kh_dls_world_pass_body(khw_ctx);
    g_ro.in_injection = khw_pinj;
}
}