#pragma once

// The injection half (rendering_integration_injection.hpp): DLS_WORLD, the world pass of our dynamic-light shadows.
// Never include it on its own.
namespace RenderIntegration {

// DLS_WORLD stage 4: the game environment receives our meshes' dynamic-light shadows - one fullscreen multiply into
// the scene colour at the resolve, emitting KhDlsWorldFactor (how much of each light's contribution our meshes blocked
// at that pixel, divided by everything lighting it; the albedo cancels, which is what makes this possible without a
// G-buffer - see the kernel's note in cb.hlsl).
static uint64_t g_dls_world_cycle = 0;        // once-per-cycle guard (state, not a lane).

// The caller resolves the matrix once and hands in the engine's own view and projection for this frame, so the mask
// clips exactly where the engine's draw of the same geometry clipped. The value written is the clip w, not the clip z,
// so the z terms decide only what is clipped, never what is stored.
inline bool dlsw_mask_render(ID3D11DeviceContext* ctx,
                             const float view[16], const float proj_in[16],
                             UINT w, UINT h) {
    if (!ctx || w < 2 || h < 2) return false;
    if (!g_res.vs_dls_mask || !g_res.ps_dls_mask || !g_res.dlsw_min_blend ||
        !g_res.input_layout || !g_res.dss_off || !g_res.rasterizer ||
        !g_res.composite_cb || !g_res.composite_frame_cb) return false;
    if (g_dlsw_casters.empty()) return false;
    if (!proj_in) return false;
    // A degenerate projection scales the mesh to nothing or to infinity; a
    // wrong mask is the failure this path exists to remove.
    if (!(fabsf(proj_in[0]) > 1.0e-6f) || !(fabsf(proj_in[5]) > 1.0e-6f)) return false;
    // Refused; the caller treats it as a mask failure (mir_meta[0] stays 0) and the pass runs unmasked.
    if (!(proj_in[11] > 0.0f)) return false;

    if (!g_res.dlsw_tex || g_res.dlsw_w != w || g_res.dlsw_h != h) {
        KH_SAFE_RELEASE(g_res.dlsw_srv);
        KH_SAFE_RELEASE(g_res.dlsw_rtv);
        KH_SAFE_RELEASE(g_res.dlsw_tex);
        g_res.dlsw_w = g_res.dlsw_h = 0;
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) return false;
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R32_FLOAT;   // Metres, not a depth format.
        td.SampleDesc.Count = 1;             // single-sample: the mask is a per-pixel question,
                                                 // And the world pass reads it per pixel.
        td.Usage = D3D11_USAGE_DEFAULT;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        bool ok = SUCCEEDED(dev->CreateTexture2D(&td, nullptr, &g_res.dlsw_tex));
        if (ok) ok = SUCCEEDED(dev->CreateRenderTargetView(g_res.dlsw_tex, nullptr, &g_res.dlsw_rtv));
        if (ok) ok = SUCCEEDED(dev->CreateShaderResourceView(g_res.dlsw_tex, nullptr, &g_res.dlsw_srv));
        dev->Release();
        if (!ok) {
            KH_SAFE_RELEASE(g_res.dlsw_srv);
            KH_SAFE_RELEASE(g_res.dlsw_rtv);
            KH_SAFE_RELEASE(g_res.dlsw_tex);
            return false;
        }
        g_res.dlsw_w = w;
        g_res.dlsw_h = h;
    }

    // View * proj, row-vector, with the engine's own projection (we store clip w: see the head note).
    float proj[4][4];
    memcpy(proj, proj_in, sizeof(proj));
    float v[4][4];
    memcpy(v, view, sizeof(v));
    float vp[4][4];
    mul_4x4(v, proj, vp);

    ConstantData frm = {};
    memcpy(frm.view_proj, vp, sizeof(frm.view_proj));

    // Rebase: the mask must adopt centerRel like every other consumer of that geometry.
    float cam[3] = { 0.0f, 0.0f, 0.0f };
    cam_of(reinterpret_cast<const float(*)[4]>(view), cam);
    // DLSW_ENGCAM: the meshes this mask must cover were drawn through the engine's own camera whenever the lock held
    // (the injection's engcam, the flush's engcam); the fold's float camera alone puts the mask beside the
    // drawn silhouette far from the map origin (1 - 6 px at 2 m, 15 - 30 km). The same choice here, through
    // ENGCAM_PEEK: the frame's latched value when this view agrees with it, else the value the consume would take
    // for this view - with none of the consume's side effects (this pass can run before the flush, whose view differs
    // on a sparse cycle that has not landed), so no drawer's verdict changes. False: the fold.
    const bool eng = engcam_peek(reinterpret_cast<const float(*)[4]>(view), cam);
    const bool reb = eng ? rebase_vp_engcam(frm, v, proj) : rebase_vp(frm, cam);

    // StateBackup owes the restore: without it the world pass's own capture would record our shaders and min-blend as
    // the engine's and hand them to every engine draw after the resolve.
    StateBackup bk;
    bk.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSave om;
    om.capture(ctx);
    UINT nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT ovp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&nvp, ovp);

    // The sentinel is the clear: every pixel starts further away than anything can be, so min-blending a caster in
    // always wins and an untouched pixel reads as "no mesh". Uploaded inside the bracket: upload_frame_cb rewrites
    // composite_frame_cb, and the bracket's restore puts the engine's binding back.
    if (!upload_frame_cb(ctx, g_res.composite_frame_cb, frm)) {
        om.restore(ctx);
        om.release();
        bk.restore(ctx);
        return false;
    }

    const FLOAT far4[4] = { 1.0e30f, 1.0e30f, 1.0e30f, 1.0e30f };
    ctx->OMSetRenderTargets(1, &g_res.dlsw_rtv, nullptr);
    ctx->ClearRenderTargetView(g_res.dlsw_rtv, far4);
    D3D11_VIEWPORT vp2 = {};
    vp2.Width = static_cast<FLOAT>(w);
    vp2.Height = static_cast<FLOAT>(h);
    vp2.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp2);

    // b0 the object CB, b1 the frame CB (viewProj), to both stages.
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_dls_mask, nullptr, 0);
    ctx->PSSetShader(g_res.ps_dls_mask, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    const FLOAT bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_res.dlsw_min_blend, bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);

    const UINT stride = sizeof(MeshVertex);
    VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
    uint32_t drawn = 0;
    // The device for the material binds; the alpha twin is optional (absent, an alpha caster masks whole).
    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool alpha_ok = dev && g_res.ps_dls_mask_a && g_res.vs_tex && g_res.layout_tex;
    bool alpha_bound = false;
    // INFRONT: a view-model mesh sits in the slice's depth, and the world projection clips it short of the world
    // near plane, so it would leave the mask and take the world-receive multiply on top of its own DLS term. It is
    // masked in a second pass under the hands pair (column 2 of the same rebased matrix): x / y / w are the world's, z
    // lands in [0, 1] from 0.01 m, and the mask is metres (clip w) either way.
    bool front_any = false;
    for (const DlswCaster& c0 : g_dlsw_casters) {
        if (c0.in_front && c0.visible && c0.alpha >= 0.999f && !c0.no_foot) front_any = true;
    }
    for (int pass = 0; pass < 2; ++pass) {
    if (pass == 1) {
        if (!front_any) break;
        float m22 = 0.0f, m32 = 0.0f;
        if (!infront_pair(m22, m32)) break;
        ConstantData frm2_m = frm;
        for (int r = 0; r < 3; ++r) frm2_m.view_proj[r][2] = m22 * frm2_m.view_proj[r][3];
        frm2_m.view_proj[3][2] = m22 * frm2_m.view_proj[3][3] + m32;
        if (!upload_frame_cb(ctx, g_res.composite_frame_cb, frm2_m)) break;
    }
    for (size_t i = 0; i < g_dlsw_casters.size(); ++i) {
        const DlswCaster& c = g_dlsw_casters[i];
        if (c.in_front != (pass == 1)) continue;   // INFRONT: its own pass.
        // Only what owns pixels is masked. An invisible (casterOnly) object and a whole translucent one write no depth,
        // so the surface at their pixels is the world behind them, which must receive the shadow they cast. A
        // see-through blend mode (FOOT_BLEND) may write depth and still shows the world behind it at every texel:
        // the same rule.
        if (!c.visible || c.alpha < 0.999f) continue;
        if (c.no_foot) continue;   // FOOT_BLEND: the world behind shows through.
        // The size test is not optional: c.mesh is bounded against the published registry (mesh_count), not against
        // mesh_vb, and those two are equal only while ensure_mesh_vbs keeps up (it returns early on an empty mesh or a
        // failed CreateBuffer after the published count was already raised).
        if (c.mesh < 0 ||
            static_cast<size_t>(c.mesh) >= g_res.mesh_vb.size() ||
            !g_res.mesh_vb[c.mesh]) continue;
        const VbRef want = mesh_vb_ref(c.mesh, c.slot);   // CLOTH.
        if (!want) continue;
        ConstantData obj = {};
        // SQF [x, y, zASL] -> engine [x, zASL, y].
        obj.center_size[0] = c.pos[0];
        obj.center_size[1] = c.pos[2];
        obj.center_size[2] = c.pos[1];
        obj.size_axes[0] = c.size[0];
        obj.size_axes[1] = c.size[2];
        obj.size_axes[2] = c.size[1];
        // Always the rotation, never null: fill_obj_rot dereferences unconditionally, and an unrotated caster
        // carries an identity in rot[] anyway.
        fill_obj_rot(obj, c.rot);
        // Armed only when the matrix above actually rebased: the two must never disagree, or the vertex would be
        // camera-relative against an absolute matrix.
        if (reb) fill_center_rel(obj, cam, c.res);   // POS_RES.
        if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) break;
        // An alpha caster with a material set masks through the textured VS and the clipping twin, its material bound
        // per submesh. Colour-alpha-only casters that reached here carry alpha >= 0.999: the plain pair.
        const bool want_alpha = c.alpha_caster && c.mats && alpha_ok;
        if (want_alpha != alpha_bound) {
            ctx->IASetInputLayout(want_alpha ? g_res.layout_tex : g_res.input_layout);
            ctx->VSSetShader(want_alpha ? g_res.vs_tex : g_res.vs_dls_mask, nullptr, 0);
            ctx->PSSetShader(want_alpha ? g_res.ps_dls_mask_a : g_res.ps_dls_mask, nullptr, 0);
            if (want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
            alpha_bound = want_alpha;
        }
        if (want != bound_vb) {
            ctx->IASetVertexBuffers(0, 1, &want.vb, &stride, &want.off);
            ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
            bound_vb = want;
        }
        // The mask covers whatever the colour pass drew: the level lod_pick chose, and during a fade both levels,
        // each cut by the colour pass's dither (DLSW_MASK_DITHER), so the mask at a pixel holds the level that pixel
        // shows. Same pick, same inputs on a cycle the injection drew (it published g_lod.proj_px before this resolve).
        // Known residual: a cycle whose meshes the flush draws late publishes after this pass (flush_locked runs after
        // dls_world_pass), so in a frame whose projection changed (a zoom, a FOV change) the pick here used the last
        // value: the mask's level - or, during a fade, its dither - can differ from the colour pass's, and the mask
        // then holds another level's outline than the one shown, for that frame. Not the shadow LOD. An inFront mesh is
        // the slice's, drawn at level 0 only (INFRONT_LOD).
        {
            const MeshDef& md = mesh_def(c.mesh);
            int   lvl = 0;
            float lt = 0.0f;
            if (!c.lod_lock && !c.in_front) {
                lod_pick(md, c.lod_r,   // SKEL_LOD_R.
                         sqrtf(mesh_dist_sq_of(c.pos, c.size, c.rot, c.rotated, cam)),
                         lvl, lt);
            }
            const int iters = lt > 0.0f ? 2 : 1;
            bool ok = true;
            for (int it = 0; it < iters && ok; ++it) {
                const int lv = mesh_lod_clamp(md, lvl + it);
                // DLSW_MASK_DITHER: a fading level takes the dither lane the injection's crossfade loop gives it
                // (lod_dither: the finer level first), which PSDlsMask (blendCtl.w) and PSDlsMaskA (its object lane)
                // cut by. Do not mask the union of both levels: it claims the world's pixels inside the hidden level's
                // outline as ours, and KhDlsMaskCov keeps our dynamic-light shadow off them for the fade. The alpha
                // route uploads obj per submesh below; the plain route uploads it here. obj is this caster's
                // own, so the lane leaks into no other caster.
                if (iters == 2) {
                    obj.blend_ctl[3] = lod_dither(lt, it != 0);
                    if (!want_alpha) {
                        ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                        if (!ok) break;
                    }
                }
                if (want_alpha) {   // Per submesh of the level, the material's lanes + maps bound.
                    const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, lv);
                    for (size_t s = 0; s < tab.size() && ok; ++s) {
                        const MeshSubmesh& sm = tab[s];
                        if (sm.index_count == 0) continue;
                        bind_material(ctx, dev, obj, c.mats, s);
                        ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                        if (!ok) break;
                        ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                    }
                } else {
                    UINT ls = 0, lc = 0;
                    mesh_lod_range(md, lv, ls, lc);
                    if (lc == 0) continue;
                    ctx->DrawIndexed(lc, ls, 0);
                }
            }
            if (!ok) break;
        }
        drawn++;
    }
    }   // INFRONT: the two mask passes.
    if (dev) dev->Release();

    if (nvp > 0) ctx->RSSetViewports(nvp, ovp);
    om.restore(ctx);
    om.release();
    bk.restore(ctx);   // Shaders, layout, blend, dss, raster, CBs.
    return drawn > 0;
}

// DLSW_FOG_FILL: the world pass's fog lanes on a cycle whose mesh frame template is not this cycle's. The injection
// publishes the template, but it can bail after the dynamic-light select (the near-gap partition, an anomalous cycle);
// the flush then carries the cycle and publishes the template - after the world pass, which runs ahead of it at the
// same resolve. This is the flush's fog-lane fill (fx_frame_template) verbatim, on the same live sources at the same
// instant, so the lanes are the ones the flush's meshes fog with this cycle. Deliberate difference: fogBelow.z is not
// written (the cast fire's lane; the template copy never writes it). TWIN: the injection's and the flush's fog fills -
// an edit there is an edit here.
inline void dlsw_fog_lanes(ConstantData& cbf, const float* cam) {
    const float cam_y =
        (g_ls.cam[0] * g_ls.cam[0] + g_ls.cam[1] * g_ls.cam[1] +
         g_ls.cam[2] * g_ls.cam[2] >= 1.0f) ? g_ls.cam[1]
      : (cam[0] * cam[0] + cam[1] * cam[1] + cam[2] * cam[2] >= 1.0f)
         ? cam[1] : 0.0f;
    // The TRUE camera altitude, unclamped. The shaders arm their below-layer branch on camY < fogSkyCol.w (static.hlsl,
    // composite.hlsl), so a lane clamped at the layer would arm that branch nowhere: the ray is split shader-side, not
    // here.
    const float fog_cam_y = cam_y;
    // fogBelow: x = the engine's below-layer extinction (block nb[51]), y = armed. z is not written here (the mesh
    // fills write 0; it is the cast fire's lane, which the world pass never carries); w is never written.
    cbf.fog_below[0] = (g_light_probe.hits > 0 && g_light_probe.meta == 40)   // PROBE_META40.
                     ? g_light_probe.nb[51] : 0.0f;
    cbf.fog_below[1] = 1.0f;
    const bool fog_on = g_fog_valid && g_fog[0] > 1e-4f;
    const bool eng_ok = g_light_probe.hits > 0 && g_light_probe.meta == 40;
    fill_haze(cbf);

    if (fog_on || cbf.haze_pars[3] >= 0.5f || eng_ok) {
        cbf.fog_params[0] = fog_on ? g_fog[0] : 0.0f;
        cbf.fog_params[1] = g_fog[1];
        cbf.fog_params[2] = g_fog[2];
        cbf.fog_params[3] = fog_on ? 1.0f : 0.0f;

        // Fog is decoupled from the block lock, so a degenerate staged decay or the pre-lock cold window does not leave
        // the mesh unfogged in a fogged world.
        if (g_light_probe.hits > 0 && g_light_probe.meta == 40) {
            const float* e = g_light_probe.nb + 36;

            for (int c = 0; c < 3; ++c) cbf.fog_color[c] = e[c];

            // Engine transmittance terms from the block mirror: the density scale, the linear ramp's end + inverse
            // range.
            cbf.fog_engine[0] = g_light_probe.nb[41];
            cbf.fog_engine[1] = g_light_probe.nb[48];
            cbf.fog_engine[2] = g_light_probe.nb[49];
            const bool dec_fresh =
                g_light_probe.hits > 0 &&
                (effect_time_seconds() - g_light_probe.mirror_confirm) < 0.5f;   // DEC_MIRROR_CLOCK.
            const bool dec_eng = dec_fresh && g_light_probe.nb[40] >= 0.0f && g_light_probe.nb[40] < 1.0f;
            if (dec_eng) {
                cbf.fog_params[1] = g_light_probe.nb[40];
            }
            const bool terms = g_light_probe.nb[48] > 1.0f &&
                               g_light_probe.nb[49] > 0.0f &&
                               g_light_probe.nb[41] >= 0.0f;
            const float rp_end = g_light_probe.nb[48];
            const float rp_inv = g_light_probe.nb[49];
            const float rp_prod = rp_end * rp_inv;
            const bool rp_incoh = terms && rp_prod < 1.0f;
            const bool ramp_off = rp_incoh;
            cbf.fog_engine[3] = !terms ? 0.0f : (ramp_off ? 2.0f : 1.0f);
        } else {
            cbf.fog_color[0] = 1.0f;
            cbf.fog_color[1] = 1.0f;
            cbf.fog_color[2] = 1.0f;
        }

        if (g_sky_probe.hits > 0) {
            cbf.fog_sky[0] = 1.0f;
            cbf.fog_sky[1] = 1.0f;
            cbf.fog_sky[2] = 1.0f;
            cbf.fog_sky[3] = 1.0f;
            cbf.fog_sky_col[0] = g_sky_probe.nb[4];
            cbf.fog_sky_col[1] = g_sky_probe.nb[5];
            cbf.fog_sky_col[2] = g_sky_probe.nb[6];
            // The below-layer convergence colour: nb[28..30] = the sky fog colour, nb[68..70] = its elevation gradient.
            // Pure captures, inert unless the shader is below the layer.
            cbf.fog_uw[0] = g_sky_probe.nb[28];
            cbf.fog_uw[1] = g_sky_probe.nb[29];
            cbf.fog_uw[2] = g_sky_probe.nb[30];
            cbf.fog_uw[3] =
                1.0f;
            cbf.fog_uw_grad[0] = g_sky_probe.nb[68];
            cbf.fog_uw_grad[1] = g_sky_probe.nb[69];
            cbf.fog_uw_grad[2] = g_sky_probe.nb[70];
        }

        // Camera altitude (engine Y-up), unclamped - see fog_cam_y.
        cbf.fog_color[3] = fog_cam_y;
    }
}

inline void dls_world_pass_body(ID3D11DeviceContext* ctx) {

    // A light must hold a slot and have a map, or the pass is a fullscreen multiply by 1.
    // DLS_SRV_GATE: no map view (released with the device) = no map (dls_published).
    if (g_dls_n == 0 || g_dls_map_valid == 0ull || !g_res.dls_srv) {  return; }
    // The maps, the slots and the mask's owners are dls_frame's, run by the injection (this cycle) or by the flush
    // after this pass (the previous one's then). Older, no mesh pass has run it since - a hidden or deleted last
    // caster's shadow would stay painted, drawn with casters that may be gone.
    if (g_dls_frame_cycle == ~0ull || g_topo_cycles - g_dls_frame_cycle > 1u) return;

    if (!g_res.ps_dls_world || !g_res.vs_fullscreen || !g_res.rasterizer ||
        !g_res.dss_off || !g_res.dls_world_blend || !g_res.composite_cb ||
        !g_res.composite_frame_cb) {
        return;
    }

    ID3D11RenderTargetView* rtv = nullptr;
    ctx->OMGetRenderTargets(1, &rtv, nullptr);
    if (!rtv) {  return; }
    rtv->Release();

    // g_mask.cast_depth is the engine's own live resolved linear depth, adopted by identity at the resolve sweep and
    // validated there as a screen-sized single-channel float.
    if (!g_mask.cast_depth || g_mask.cast_dims[0] < 2.0f || g_mask.cast_dims[1] < 2.0f) {
        return;
    }

    ConstantData obj = {};
    ConstantData frm = {};

    dls_fill_cb(frm);   // dls_meta / dls_ctl / dls_face_slice / dls_spot_vp.

    // Engine sun + sky, for KhDlsWorldFactor's denominator. These are FRAME-half lanes (past view_proj) and the frame
    // CB uploads from frm, so they MUST be written to frm - on obj they land in the never-uploaded tail,
    // Ltotal sums with zero sun and sky, and the shadow paints near-black in daylight. lighting0[2]/[3] are OBJECT-half
    // and stay on obj. The else arm twins fill_lighting_frame_cb's fallback (white, fails toward under-darkening).
    if (g_pub_block_valid) {
        frm.lighting2[0] = g_pub_block_sun[0];
        frm.lighting2[1] = g_pub_block_sun[1];
        frm.lighting2[2] = g_pub_block_sun[2];
        frm.light_amb[0] = g_pub_block_amb[0];
        frm.light_amb[1] = g_pub_block_amb[1];
        frm.light_amb[2] = g_pub_block_amb[2];
        frm.light_amb[3] = 1.0f;
    } else {
        frm.lighting2[0] = 1.0f;
        frm.lighting2[1] = 1.0f;
        frm.lighting2[2] = 1.0f;
        frm.light_amb[0] = 1.0f;
        frm.light_amb[1] = 1.0f;
        frm.light_amb[2] = 1.0f;
        frm.light_amb[3] = 0.0f;
    }
    // DLSW_GEN_AMB: never below the brightest lit world cluster of the last world frame, per channel (see
    // g_blko.gen_valid): the denominator stays generous whichever cluster the publish picked for this camera.
    if (g_blko.gen_valid) {
        for (int c = 0; c < 3; ++c) {
            frm.lighting2[c] = fmaxf(frm.lighting2[c], g_blko.gen_sun[c]);
            frm.light_amb[c] = fmaxf(frm.light_amb[c], g_blko.gen_amb[c]);
        }
    }
    obj.lighting0[2] = 1.0f;   // Ambient scale: the world's own, not an object's.
    obj.lighting0[3] = 1.0f;
    frm.lighting1[0] = g_sun_dir_engine[0];
    frm.lighting1[1] = g_sun_dir_engine[1];
    frm.lighting1[2] = g_sun_dir_engine[2];

    // Do not paint on our own meshes. The mask projects world -> pixel with the camera our meshes were drawn with and
    // the reconstruction maps pixel -> world; frozen picks only the reconstruction's inputs (the fire's frozen
    // view, inverse and fov below), and the mask is drawn on frozen and live frames alike. DLSW_LATCH: that camera
    // is the one the flush takes. A cycle that has landed uses the landing's own view (g_inj_pub.view, fresh), which
    // carries the injection's jump fallback; otherwise the cycle latch (LATCH_FOLD), or the live view without one,
    // with the engine's frame view adopted. One exception: on a sparse cycle that has not landed (flush_locked's
    // sparse_live) the flush draws with the live view, and this pass keeps the latch, the camera of the engine's
    // depth it reconstructs. The live read keeps only its projection (the injection takes the x and y scales live too).
    // Do not take the live view where a landing or the cycle latch exists: it is a later camera than the one the meshes
    // and the engine's depth were drawn with, so the mask and the reconstruction slide off them while the camera turns.
    RVExtBridge::ProjectionViewTransform pv = {};
    bool pv_ok = RVExtBridge::get_projection_view_transform(pv);
    if (g_ro.cycle_pv_valid) {
        if (pv_ok) memcpy(&pv.view[0][0], &g_ro.cycle_pv.view[0][0], sizeof(pv.view));
        else           pv = g_ro.cycle_pv;
        pv_ok = true;
    }
    if (!pv_ok) {  return; }
    if (g_composite_land_cycle == g_topo_cycles &&
        g_inj_pub.view_ms != 0 && steady_now_ms() - g_inj_pub.view_ms < 100) {
        memcpy(&pv.view[0][0], g_inj_pub.view, sizeof(g_inj_pub.view));
    } else {
        adopt_frame_view(pv);
    }

    const bool frozen = g_fire.lock_valid && g_fire.cast_inv_valid;
    float fov[2] = { 0.0f, 0.0f };
    bool  fov_ok = false;
    if (frozen && g_fire.fov2[0] > 1.0e-6f && g_fire.fov2[1] > 1.0e-6f) {
        fov[0] = g_fire.fov2[0];
        fov[1] = g_fire.fov2[1];
        fov_ok = true;
    } else if (g_mask.cast_fov_valid && g_mask.cast_fov[0] > 1.0e-6f &&
               g_mask.cast_fov[1] > 1.0e-6f) {
        fov[0] = g_mask.cast_fov[0];
        fov[1] = g_mask.cast_fov[1];
        fov_ok = true;
    }

    // The mask registers with the framebuffer, not with the reconstruction: the reconstruction turns a depth texel into
    // a world point, the mask rasterizes geometry - they share a view but not a job. Sharing one fov gives a white rim.
    frm.mir_meta[0] = 0.0f;
    float mv[16] = {};
    float mp[16] = {};
    bool  mok = false;

    // The meshes' camera (DLSW_LATCH above).
    memcpy(mv, &pv.view[0][0], sizeof(mv));
    memcpy(mp, &pv.projection[0][0], sizeof(mp));
    mok = true;

    if (mok &&
        dlsw_mask_render(ctx, mv, mp,
                         static_cast<UINT>(g_mask.cast_dims[0]),
                         static_cast<UINT>(g_mask.cast_dims[1]))) {
        frm.mir_meta[0] = 1.0f;
        frm.mir_meta[1] = static_cast<float>(g_res.dlsw_w);
        frm.mir_meta[2] = static_cast<float>(g_res.dlsw_h);
    }

    {
        float cam[3] = { 0.0f, 0.0f, 0.0f };
        cam_of(pv.view, cam);   // CAM_HONEST (re-derived below from castMat, the same point).
        float inv[3][3];
        const bool inv_ok = !frozen && view_inv_rows(pv.view, inv);

        for (int r = 0; r < 3; ++r) {
            frm.cast_view[0][r] = frozen ? g_fire.view2[12 + r]
                                                     : pv.view[3][r];
            if (frozen) {
                // The cast's own inverse, cofactor-derived in double and laid out for KhCastWorld's row dots.
                frm.cast_mat[r][0] = g_fire.cast_inv[r * 3 + 0];
                frm.cast_mat[r][1] = g_fire.cast_inv[r * 3 + 1];
                frm.cast_mat[r][2] = g_fire.cast_inv[r * 3 + 2];
            } else {
                // CAM_HONEST: the view's cofactor inverse rotation, as the freeze takes its own; the transpose
                // (the inverse of an exactly orthonormal rotation) only where that is refused.
                frm.cast_mat[r][0] = inv_ok ? inv[r][0] : pv.view[r][0];
                frm.cast_mat[r][1] = inv_ok ? inv[r][1] : pv.view[r][1];
                frm.cast_mat[r][2] = inv_ok ? inv[r][2] : pv.view[r][2];
            }
            frm.cast_mat[r][3] = 0.0f;   // CAST_OCC lane hygiene.
        }
        frm.cast_view[0][3] = 1.0f;
        // KhCastWorld multiplies NDC x by castView[1].x, so this lane is tan(fovX/2): the fire's frozen fov on a frozen
        // frame, else the engine's own constant block (g_mask.cast_fov); derived from the projection only when neither
        // is valid.
        if (fov_ok) {
            frm.cast_view[1][0] = fov[0];
            frm.cast_view[1][1] = fov[1];
        } else {
            frm.cast_view[1][0] = (pv.projection[0][0] != 0.0f)
                                ? 1.0f / pv.projection[0][0] : 1.0f;
            frm.cast_view[1][1] = (pv.projection[1][1] != 0.0f)
                                ? 1.0f / pv.projection[1][1] : 1.0f;
        }
        frm.cast_view[1][2] = g_mask.cast_dims[0];
        frm.cast_view[1][3] = g_mask.cast_dims[1];
        // The camera the normal's facing test uses must come from the same view the reconstruction did. Derived by the
        // reconstruction's own algebra: with zl = 0, KhCastWorld reduces to (-translation) dotted through the inverse
        // rotation. CAM_HONEST: in both branches, so the camera is castMat's whichever it holds.
        for (int r = 0; r < 3; ++r) {
            cam[r] = -(frm.cast_view[0][0] * frm.cast_mat[r][0] +
                       frm.cast_view[0][1] * frm.cast_mat[r][1] +
                       frm.cast_view[0][2] * frm.cast_mat[r][2]);
        }
        frm.cast_view[2][0] = cam[0];
        frm.cast_view[2][1] = cam[1];
        frm.cast_view[2][2] = cam[2];
    }

    if (frm.dls_range[3] > 0.0f) {
        frm.dls_range[0] = frm.cast_view[2][0];
        frm.dls_range[1] = frm.cast_view[2][1];
        frm.dls_range[2] = frm.cast_view[2][2];
    }

    // DLSW_FOG: the engine fogs every pixel in its forward pass, so the colour this pass multiplies already carries
    // the fog and haze in front of the surface, and a bare multiply darkens those too - a shadow darker than the unlit
    // ground beside it, by fog x (1 - transmittance) x the blocked share: worst at dusk, growing with distance and
    // changing with the view ray's elevation. The fog lanes are the mesh passes' own (this cycle's frame template: the
    // numbers our meshes fog with), and PSDlsWorldFog puts the in-scattered share back through a dual-source blend,
    // dest * F + fog * (1 - T) * (1 - F), which is the fogged shadowed surface exactly. No template this cycle (the
    // injection bailed; the flush that publishes one runs after this pass): the flush's own fill, now
    // (DLSW_FOG_FILL). Fog disarmed in the lanes: the added term is 0, the plain multiply.
    if (g_pip.tpl_valid && g_pip.tpl_cycle == g_topo_cycles) {
        memcpy(frm.fog_params, g_pip.tpl.fog_params, sizeof(frm.fog_params));
        memcpy(frm.fog_color, g_pip.tpl.fog_color, sizeof(frm.fog_color));
        memcpy(frm.fog_engine, g_pip.tpl.fog_engine, sizeof(frm.fog_engine));
        memcpy(frm.fog_sky, g_pip.tpl.fog_sky, sizeof(frm.fog_sky));
        memcpy(frm.fog_sky_col, g_pip.tpl.fog_sky_col, sizeof(frm.fog_sky_col));
        memcpy(frm.haze_pars, g_pip.tpl.haze_pars, sizeof(frm.haze_pars));
        memcpy(frm.fog_uw, g_pip.tpl.fog_uw, sizeof(frm.fog_uw));
        memcpy(frm.fog_uw_grad, g_pip.tpl.fog_uw_grad, sizeof(frm.fog_uw_grad));
        frm.fog_below[0] = g_pip.tpl.fog_below[0];   // [2] stays the cast fire's alone.
        frm.fog_below[1] = g_pip.tpl.fog_below[1];
    } else {
        dlsw_fog_lanes(frm, frm.cast_view[2]);   // DLSW_FOG_FILL.
    }

    StateBackup bk;
    bk.capture(ctx);
    // The engine's dynamic-light pool, in the same lanes DynLights reads, copied from the last mesh fill's source
    // rather than re-derived: one source, so the world cannot disagree with the meshes about which lights exist or
    // cast. After the capture: the ring append binds PS t40 (DL_RING), and this pass runs on the engine's state with
    // no bracket around it, so the bind must fall inside this one's restore. Nothing that can block (no published
    // casting light, a stale pool, a failed append): the factor is 1 at every pixel, so nothing is drawn.
    if (!dls_fill_dl_world(ctx, obj)) {
        bk.restore(ctx);
        return;
    }
    ID3D11ShaderResourceView* old_t37 = nullptr;   // Inside StateBackup's t0..t69 too; the explicit put-back stays.
    ctx->PSGetShaderResources(37, 1, &old_t37);

    // b0 and b1 bound as (0, 2), before the uploads; on a device without the 11.1 ring it never binds. StateBackup
    // saved both slots offset-aware and restores both.
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);
    if (!upload_obj_cb(ctx, g_res.composite_cb, obj) ||
        !upload_frame_cb(ctx, g_res.composite_frame_cb, frm)) {
        KH_SAFE_RELEASE(old_t37);
        bk.restore(ctx);   // The bracket is open: close it.
        return;
    }

    ctx->PSSetShaderResources(0, 1, &g_mask.cast_depth);
    if (g_res.dls_srv) ctx->PSSetShaderResources(36, 1, &g_res.dls_srv);
    ctx->PSSetSamplers(1, 1, &g_res.samp_pf);   // SHADOW_GATHER (bk restores s1).
    if (frm.mir_meta[0] >= 0.5f && g_res.dlsw_srv) {
        ctx->PSSetShaderResources(37, 1, &g_res.dlsw_srv);   // DLSW_MASK, metres.
    }
    ctx->IASetInputLayout(nullptr);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs_fullscreen, nullptr, 0);
    // DLSW_FOG: the dual-source form needs its shader, its blend state and render target 0 alone (a second source
    // blends into RT0 only; with another target bound its SV_Target1 would be written there); the fog lanes are this
    // cycle's on every path above. Anything missing: the plain multiply.
    bool fog = g_res.ps_dls_world_fog && g_res.dls_world_fog_blend;
    if (fog) {
        ID3D11RenderTargetView* rts[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rts, nullptr);
        for (UINT k = 0; k < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++k) {
            if (k > 0 && rts[k]) fog = false;
            KH_SAFE_RELEASE(rts[k]);
        }
    }
    ctx->PSSetShader(fog ? g_res.ps_dls_world_fog : g_res.ps_dls_world, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_off, 0);
    // Dest * src with a unit blend factor, so SRC_COLOR is the multiplier.
    const FLOAT bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(fog ? g_res.dls_world_fog_blend : g_res.dls_world_blend, bf, 0xFFFFFFFF);
    ctx->RSSetState(g_res.rasterizer);
    ctx->Draw(3, 0);

    // t0 held the depth SRV; the engine rebinds that resource as a DSV immediately after the resolve, and a lingering
    // SRV would be force-nulled with a debug-layer warning storm.
    ID3D11ShaderResourceView* null = nullptr;
    ctx->PSSetShaderResources(0, 1, &null);
    ctx->PSSetShaderResources(37, 1, &old_t37);   // Explicit put-back (StateBackup's restore covers it too)
    KH_SAFE_RELEASE(old_t37);   // Exactly what was bound (t35 pattern).
    ctx->PSSetShaderResources(36, 1, &null);   // Inside it; explicit like t0 and t37.
    bk.restore(ctx);
}

// The resolve hook evaluates !g_ro.in_injection before calling here, so the flag is raised inside. Without it the
// mask's state sets and draws would flow through the hooks as engine draws.
inline void dls_world_pass(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_DLS_WORLD);
    KH_GPU_SCOPE(ctx, GPU_DLS_WORLD);   // PROF.
    if (!ctx) return;
    const bool pinj = g_ro.in_injection;
    g_ro.in_injection = true;
    dls_world_pass_body(ctx);
    g_ro.in_injection = pinj;
}
}