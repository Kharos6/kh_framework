#pragma once

// The injection half (rendering_integration_injection.hpp): the volume seam: the camera take, volume_seam_inject,
// its tracking, the prime mask and the pump. Never include it on its own.
namespace RenderIntegration {

inline void rp_foreign(int kind);                    // VOL_REPLAY (defined with the replay).
inline void rp_snap_engine(ID3D11DeviceContext* ctx);     // VOL_REPLAY (defined with the replay).
inline bool vmir_wanted(const float cam[3], const float proj[4][4]);   // MIR_GATE: defined in
                                                                                     // rendering/replay.hpp.

// Defined in rendering/depth_identity.hpp (declared here for the uses below).
inline void* reorder_dsv_identity(ID3D11DepthStencilView* dsv);
// Defined in rendering/replay.hpp (declared here for the uses below).
inline void rp_pass_end(ID3D11DeviceContext* ctx);


// EA_CBC_BEST: the census entry (cbc_find) of the freshest upload among the buffers bound at VS / PS b0 - b3,
// or none - the take's two census reads (the still-camera rescue and the depth pair) ask the same question.
inline const CbCensusEntry* svs_cbc_best(ID3D11DeviceContext* ctx) {
    ID3D11Buffer* b[8] = {};
    ctx->VSGetConstantBuffers(0, 4, b);
    ctx->PSGetConstantBuffers(0, 4, b + 4);
    const CbCensusEntry* best = nullptr;
    for (int i = 0; i < 8; ++i) {
        if (!b[i]) continue;
        const CbCensusEntry* const e = cbc_find(b[i]);
        if (e && (!best || e->ms > best->ms)) best = e;
    }
    for (ID3D11Buffer*& r : b) KH_SAFE_RELEASE(r);
    return best;
}

// The seam's camera take: the engine's camera projection for this frame, pre-clear, where no latch of this frame exists
// yet - the live bridge sample reconciled against the latch and the encode-pair witnesses. Shared verbatim with the
// view-model seam (infront_seam_inject), which calls it only when the world seam did not run this window: the step
// trackers it advances are per window.
inline bool svs_take_pv(ID3D11DeviceContext* ctx, RVExtBridge::ProjectionViewTransform& pv) {
    bool have = false;

    if (g_ro.cycle_pv_valid) { pv = g_ro.cycle_pv; have = true; }
    float live_near = -1.0f;
    float latch_cam[3] = {};
    bool  latch_cam_ok = false;
    if (g_ro.cycle_pv_valid) {
        cam_of(g_ro.cycle_pv.view, latch_cam);
        latch_cam_ok = (latch_cam[0] == latch_cam[0]) &&
                       (latch_cam[1] == latch_cam[1]) &&
                       (latch_cam[2] == latch_cam[2]);
    }
    else if (RVExtBridge::get_projection_view_transform(pv)) { have = true; }

    if (!have) {  return false; }

    // Adopt the frame view's rotation while it is frame-fresh (adopt_frame_view); otherwise the taken view's own
    // stands (the latch's, else the live fetch's).
    adopt_frame_view(pv);

    // The depth pair (m22, m32) the footprint actually rasterises with, per frame: the measured engine pair here, then
    // the live take's arbitration and the CB census below, each overriding where it passes (a later one ranks above an
    // earlier one because it runs after it).

    if (g_ro.engine_proj_valid) {
        pv.projection[2][2] = g_ro.engine_m22;
        pv.projection[3][2] = g_ro.engine_m32;
    }

    RVExtBridge::ProjectionViewTransform live = {};

    if (RVExtBridge::get_projection_view_transform(live)) {
        float ac[3];
        float lc[3];   // The translation anchor the take builds on.
        float vc[3];   // The camera the bridge holds at this instant.
        // The honest inverse on all three.
        cam_of(pv.view, ac);
        cam_of(live.view, vc);

        if (latch_cam_ok) {
            lc[0] = latch_cam[0];
            lc[1] = latch_cam[1];
            lc[2] = latch_cam[2];
        } else {
            lc[0] = ac[0];   // No latch: absolute form.
            lc[1] = ac[1];
            lc[2] = ac[2];
        }
        float cc[3];   // The candidate camera the take arbitrates.

        if (g_seam_take.live_lp_ok) {
            cc[0] = lc[0] + (vc[0] - g_seam_take.live_lp[0]);
            cc[1] = lc[1] + (vc[1] - g_seam_take.live_lp[1]);
            cc[2] = lc[2] + (vc[2] - g_seam_take.live_lp[2]);
        } else {
            cc[0] = vc[0];   // Cold: absolute form.
            cc[1] = vc[1];
            cc[2] = vc[2];
        }

        g_seam_take.live_lp[0] = vc[0];   // The live feed's own trajectory.
        g_seam_take.live_lp[1] = vc[1];   // Updated on every successful fetch.
        g_seam_take.live_lp[2] = vc[2];   // Regardless of the take's verdict.
        g_seam_take.live_lp_ok = true;

        const float dx = cc[0] - lc[0];
        const float dy = cc[1] - lc[1];
        const float dz = cc[2] - lc[2];
        float d = sqrtf(dx * dx + dy * dy +
                                   dz * dz);

        if (d <= LATCH_JUMP_M) {
            // A foreign one-frame flicker never confirms.
            bool take = true;
            {
                float sm = 0.0f;

                if (g_seam_take.live_prev_ok) {
                    const float lt_sx = lc[0] - g_seam_take.live_prev[0];
                    const float lt_sy = lc[1] - g_seam_take.live_prev[1];
                    const float sz = lc[2] - g_seam_take.live_prev[2];
                    sm = sqrtf(lt_sx * lt_sx +
                               lt_sy * lt_sy + sz * sz);

                    float recent = 0.0f;

                    for (int k = 0; k < 4; ++k) {
                        if (g_seam_take.live_step_ring[k] > recent) {
                            recent = g_seam_take.live_step_ring[k];
                        }
                    }

                    const float ref0 = sm > g_seam_take.live_prev_step
                        ? sm : g_seam_take.live_prev_step;
                    const float lt_ref = recent > ref0 ? recent : ref0;
                    const float mul = 100.0f;
                    const float stl = 0.005f;
                    const bool  still = lt_ref < stl;
                    const float ratio_b = mul * lt_ref;
                    const float bnd = still ? 0.30f : (ratio_b > 0.30f ? ratio_b : 0.30f);

                    if (d > bnd) {
                        take = false;

                        if (g_seam_take.live_ref_ok) {
                            const float cx = cc[0] - g_seam_take.live_ref_cam[0];
                            const float cy = cc[1] - g_seam_take.live_ref_cam[1];
                            const float cz = cc[2] - g_seam_take.live_ref_cam[2];
                            const float cd = sqrtf(
                                cx * cx + cy * cy +
                                cz * cz);
                            const float ctol = 0.5f * d > 0.5f
                                             ? 0.5f * d : 0.5f;

                            if (cd <= ctol) {
                                take = true;
                            }
                        }

                        if (!take && still) {
                            float cn = -1.0f;

                            if (cbc_on()) {
                                const CbCensusEntry* const best = svs_cbc_best(ctx);

                                if (best &&
                                    fabsf(best->m22) > 1.0e-9f) {
                                    cn = -best->m32 / best->m22;
                                }
                            }

                            const float fn = enc_pair_near(
                                live.projection[2][2],
                                live.projection[3][2]);
                            const float wg_sn = (g_ro.engine_proj_valid &&
                                fabsf(g_ro.engine_m22) > 1.0e-9f)
                                ? -g_ro.engine_m32 / g_ro.engine_m22 : -1.0f;

                            if (cn > 0.0f && wg_sn > 0.0f &&
                                fn == fn && fn > 0.0f) {
                                const bool cf =
                                    fabsf(cn - fn) <= 0.10f * fn;
                                const bool cs =
                                    fabsf(cn - wg_sn) <= 0.10f * wg_sn;

                                if (cf && cs) {
                                    take = true;
                                    g_seam_take.live_ref_ok = false;   // A take is a take.
                                }
                            }
                        }

                        if (!take) {
                            g_seam_take.live_ref_cam[0] = cc[0];   // The refused candidate.
                            g_seam_take.live_ref_cam[1] = cc[1];
                            g_seam_take.live_ref_cam[2] = cc[2];
                            g_seam_take.live_ref_ok = true;
                        }
                    } else {
                        g_seam_take.live_ref_ok = false;
                    }
                }

                g_seam_take.live_prev[0] = lc[0];
                g_seam_take.live_prev[1] = lc[1];
                g_seam_take.live_prev[2] = lc[2];
                g_seam_take.live_prev_step = sm;
                g_seam_take.live_prev_ok = true;
                // The applied-step ring, same unconditional bookkeeping as prev_step - refusal streaks and genuine rest
                // both write zeros.
                g_seam_take.live_step_ring[g_seam_take.live_step_ring_i & 3u] = sm;
                g_seam_take.live_step_ring_i++;
            }

            const float ud = take ? d : 0.0f;
            bool basis = false;

            bool lr_sane = true;

            for (int lr_r = 0; lr_r < 3 && lr_sane; ++lr_r) {
                const float lr_n =
                    live.view[lr_r][0] * live.view[lr_r][0] +
                    live.view[lr_r][1] * live.view[lr_r][1] +
                    live.view[lr_r][2] * live.view[lr_r][2];
                if (!(lr_n > 0.81f && lr_n < 1.21f)) lr_sane = false;
            }

            if (lr_sane) {
                for (int lr_r = 0; lr_r < 3; ++lr_r) {
                    pv.view[lr_r][0] = live.view[lr_r][0];
                    pv.view[lr_r][1] = live.view[lr_r][1];
                    pv.view[lr_r][2] = live.view[lr_r][2];
                    pv.view[lr_r][3] = 0.0f;
                }
                basis = true;
            }

            // Rebuild whenever the basis moved or the camera did, from the
            // bounded camera through the current basis, double precision.
            if (basis || ud > 1.0e-6f) {
                const double cam[3] = {
                    static_cast<double>(take ? cc[0] : lc[0]),
                    static_cast<double>(take ? cc[1] : lc[1]),
                    static_cast<double>(take ? cc[2] : lc[2]) };

                for (int c = 0; c < 3; ++c) {
                    pv.view[3][c] = static_cast<float>(
                        -(cam[0] * pv.view[0][c] +
                          cam[1] * pv.view[1][c] +
                          cam[2] * pv.view[2][c]));
                }

                pv.view[3][3] = 1.0f;
            }

            const float lf_sx = live.projection[0][0];
            const float lf_sy = live.projection[1][1];
            const float lx = pv.projection[0][0];
            const float ly = pv.projection[1][1];
            float lf_r = -1.0f;
            if (lf_sx > 1.0e-6f && lf_sy > 1.0e-6f &&
                lx > 1.0e-6f && ly > 1.0e-6f) {
                const float rx = lf_sx > lx ? lf_sx / lx
                                                        : lx / lf_sx;
                const float ry = lf_sy > ly ? lf_sy / ly
                                                        : ly / lf_sy;
                lf_r = rx > ry ? rx : ry;
            }

            const bool lf_sane =
                lf_sx == lf_sx && lf_sy == lf_sy &&
                lf_r > 0.0f && lf_r < 1.30f;

            if (lf_sane) {
                pv.projection[0][0] = lf_sx;
                pv.projection[1][1] = lf_sy;
            }

            // Mid-frame publications carry other windows' projections; the colour pass filters them, and an ungated
            // take here feeds them straight into the footprint's depth encode.
            const float lp_n = enc_pair_near(
                live.projection[2][2], live.projection[3][2]);
            const bool lp_sane =
                lp_n == lp_n && lp_n > 0.0f &&
                fabsf(live.projection[2][2]) > 1.0e-9f &&
                enc_far(live.projection[2][2],
                        live.projection[3][2]) <= ENC_FAR_MAX;
            if (lp_sane) live_near = lp_n;

            const bool comm_ok =
                g_inj_pub.enc_near > 0.0f && g_inj_pub.enc_ms != 0 &&
                steady_now_ms() - g_inj_pub.enc_ms < 250;
            const float lp_sn = (g_ro.engine_proj_valid &&
                                 fabsf(g_ro.engine_m22) > 1.0e-9f)
                ? enc_pair_near(g_ro.engine_m22, g_ro.engine_m32)
                : -1.0f;
            const bool sniff_ok =
                lp_sn == lp_sn && lp_sn > 0.0f &&
                enc_far(g_ro.engine_m22, g_ro.engine_m32) <= ENC_FAR_MAX;

            if (comm_ok && g_inj_pub.enc_ms != g_seam_take.pred_ms) {
                g_seam_take.pred_ms = g_inj_pub.enc_ms;
                const float c = g_inj_pub.enc_near;

                if (g_seam_take.base_prev > 0.0f &&
                    fabsf(c - g_seam_take.base_prev) <= 0.02f * g_seam_take.base_prev) {
                    g_seam_take.base_stable = c;
                }
                g_seam_take.base_prev = c;

                const bool spike =
                    g_seam_take.pred_p1 > 0.0f &&
                    (c > 2.0f * g_seam_take.pred_p1 ||
                     c < 0.5f * g_seam_take.pred_p1);

                if (!spike) {
                    g_seam_take.pred_p2 = g_seam_take.pred_p1;
                    g_seam_take.pred_p1 = c;
                    g_seam_take.pred_m22 = g_inj_pub.enc_m22;
                    g_seam_take.pred_upd_ms = steady_now_ms();
                    g_seam_take.pred_skip1 = -1.0f;
                } else if (g_seam_take.pred_skip1 > 0.0f && c <= 2.0f * g_seam_take.pred_skip1 &&
                    c >= 0.5f * g_seam_take.pred_skip1) {
                    g_seam_take.pred_p2 = -1.0f;
                    g_seam_take.pred_p1 = c;
                    g_seam_take.pred_m22 = g_inj_pub.enc_m22;
                    g_seam_take.pred_upd_ms = steady_now_ms();
                    g_seam_take.pred_skip1 = -1.0f;
                } else {
                    g_seam_take.pred_skip1 = c;
                }   // A lone spiking commit is skipped: base and slope hold.
            }

            float co_ref = g_inj_pub.enc_near;

            if (g_seam_take.pred_p1 > 0.0f && co_ref > 0.0f && (co_ref > 2.0f * g_seam_take.pred_p1 ||
                 co_ref < 0.5f * g_seam_take.pred_p1)) {
                co_ref = g_seam_take.pred_p1;
            }

            const bool corr = lp_sane && comm_ok &&
                lp_n >= 0.5f * co_ref &&
                lp_n <= 12.0f * co_ref;
            const bool base = lp_sane && comm_ok && !corr && g_seam_take.base_stable > 0.0f &&
                fabsf(lp_n - g_seam_take.base_stable) <= 0.10f * g_seam_take.base_stable;

            if (corr || base) {
                pv.projection[2][2] = live.projection[2][2];
                pv.projection[3][2] = live.projection[3][2];
            } else if (g_seam_take.pred_p1 > 0.0f && fabsf(g_seam_take.pred_m22) > 1.0e-9f &&
                        steady_now_ms() - g_seam_take.pred_upd_ms < 500) {
                float vel = 0.0f;

                if (g_seam_take.pred_p2 > 0.0f) {
                    const bool smooth = g_seam_take.pred_p1 <= 1.25f * g_seam_take.pred_p2 &&
                        g_seam_take.pred_p1 >= 0.8f * g_seam_take.pred_p2;
                    if (smooth) vel = g_seam_take.pred_p1 - g_seam_take.pred_p2;
                }

                float pe_n = g_seam_take.pred_p1 + vel;
                if (pe_n > 2.0f * g_seam_take.pred_p1) pe_n = 2.0f * g_seam_take.pred_p1;
                if (pe_n < 0.5f * g_seam_take.pred_p1) pe_n = 0.5f * g_seam_take.pred_p1;
                pv.projection[2][2] = g_seam_take.pred_m22;
                pv.projection[3][2] = -pe_n * g_seam_take.pred_m22;
            } else if (sniff_ok) {
                pv.projection[2][2] = g_ro.engine_m22;
                pv.projection[3][2] = g_ro.engine_m32;
            }
        }
    }

    // The engine uploads frame k+1's projection into the bound b2 before our pre-clear seam runs; identity reads it
    // where value arbitration structurally cannot.
    if (cbc_on()) {
        const CbCensusEntry* const best = svs_cbc_best(ctx);

        if (best) {
            const float cb_near = fabsf(best->m22) > 1.0e-9f
                ? -best->m32 / best->m22 : -1.0f;

            const float age = static_cast<float>(
                steady_now_ms() - best->ms);

            // Age-bounded so a stalled entry can never encode stale; a cold gate must not block the first encode.
            bool class_ok = true;

            if (cb_near > 0.0f) {
                bool any_wit = false;
                bool agree = false;

                if (live_near > 0.0f) {
                    any_wit = true;
                    if (cb_near >= 0.5f * live_near &&
                        cb_near <= 12.0f * live_near) agree = true;
                }

                if (!agree && g_inj_pub.enc_near > 0.0f && g_inj_pub.enc_ms != 0 &&
                    steady_now_ms() - g_inj_pub.enc_ms < 250) {
                    any_wit = true;
                    if (cb_near >= 0.5f * g_inj_pub.enc_near &&
                        cb_near <= 12.0f * g_inj_pub.enc_near) agree = true;
                }

                // A pulse return agrees with the stable baseline by definition; the zoom poison sits at 0.14x of it and
                // fails the floor. Same 10 percent agreement bar the chain's baseline acceptance uses.
                if (!agree && g_seam_take.base_stable > 0.0f) {
                    any_wit = true;
                    if (fabsf(cb_near - g_seam_take.base_stable) <=
                        0.10f * g_seam_take.base_stable) agree = true;
                }

                if (any_wit && !agree) {
                    class_ok = false;
                }
            }

            if (class_ok && cb_near > 0.0f && age <= 100.0f && enc_far(best->m22, best->m32) <= ENC_FAR_MAX) {
                pv.projection[2][2] = best->m22;
                pv.projection[3][2] = best->m32;
            }
        }
    }
    return true;
}
inline void volume_seam_inject(ID3D11DeviceContext* ctx, uint32_t w, uint32_t v_h) {
    if (!ctx) return;
    rp_foreign(2);   // VOL_REPLAY: our footprint written into a pass already being recorded.

    std::vector<SvCaster>& list = g_svs.caster_list;
    list.clear();
    // INFRONT: the view-model casters, drawn per caster into the world count and the mirror prepass below (never
    // into the instanced footprint or the VOL_FOOT mask).
    static std::vector<SvCaster> front;
    front.clear();

    {
        // The consume point for THIS pass: the footprint lands under the mesh, not one frame behind it. The engine
        // counts its stencil volumes against the footprint, so one drawn at a stale transform hands our mesh a stencil
        // computed for where it WAS - registered against the camera and every engine caster, wrong only about us. Same
        // rule as the injection and the cast fire; this pass reads the scene to build draws exactly as they do.
        {
            // SEAM_SKIN: with the pass's context, so the footprint is this frame's SHAPE as well as its place -
            // the skeletal skin, a cloth's pinned part and a chain's end (CHAIN_END_DRAW) are written only by a
            // step with a context - and the vertex stages over them, as the cast fire and the injection pair them.
            // Our own buffer writes, under the flag: the hooks stand aside.
            InjectionFlagScope flag;
            g_ro.in_injection = true;
            attach_step(ctx);
            uvs_step(ctx);
        }
        scene_sync();   // SCENE: live walk by reference, no copy.
        for (uint32_t i = 0; i < g_scene.objs.size(); ++i) {
            if (!g_scene.alive[i]) continue;
            const RenderObject& o = g_scene.objs[i];
            // The footprint serves our receivers' stencil term: visible, lit,
            // world-space, depth-participating geometry only.
            // SHADOW_SWITCH: participation, not casting - a mesh that casts nothing still owns its pixels here and
            // may still receive. (The DLS world pass does not read this footprint: it keeps its own mask,
            // dlsw_mask_render - DLSW_OWN.) SHOW_INFRONT: drawn.
            if (!obj_shown(o) || !shadow_part(o)) continue;
            SvCaster c;
            c.slot = i;   // SEAM_INST: khObjs index for the instanced twins.
            memcpy(c.pos, o.pos, sizeof(c.pos));
            obj_pos_res(o, c.res);   // POS_RES.
            memcpy(c.size, o.size, sizeof(c.size));
            bounds_size_of(o, c.bsize);   // CAST_REFIT.
            memcpy(c.rot, o.rot_m, sizeof(c.rot));
            c.rotated = o.rotated;
            c.mesh = mesh_id_clamp(o.mesh);
            c.lod = 0;   // SHADOW_LOD_DRAWN: picked below, once cam exists.
            c.lod_lock = lod_locked(o);   // LOD_SIM_SLOT.
            c.lod_r = lod_radius(o);   // SKEL_LOD_R.
            c.a_vis = true;   // SEAM_CULL: decided below, once the pass matrix is final.
            {   // FOOTPRINT_ALPHA (the sun census's recipe).
                bool exp = false;
                const float env = lifetime_envelope(o, effect_time_seconds(), exp);
                float a = exp ? 0.0f : o.color[3] * env;
                a = a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a);
                c.alpha = floorf(a * 64.0f + 0.5f) / 64.0f;
                c.mats = mat_set_has_alpha(obj_textured(o)) ? o.materials : nullptr;
                c.alpha_caster = footprint_alpha_on() && (c.alpha < 0.999f || c.mats);
            }
            c.no_foot = o.blend_mode != 0;   // FOOT_BLEND.
            c.two_sided = o.two_sided;   // SEAM_FACE.
            if (o.in_front) {   // INFRONT: its own list; whole-object translucents write no depth.
                if (!(c.alpha_caster && c.alpha < 0.999f)) front.push_back(c);
                continue;
            }
            list.push_back(c);
        }
    }

    if (list.empty() && front.empty()) {  return; }
    if (!g_rp.seam_b) rp_snap_engine(ctx);   // VOL_REPLAY: the engine's own depth, before any footprint of ours.

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool res_ok = dev && ensure_ok("seam resources", ensure_resources(dev));
    if (dev) dev->Release();
    if (!res_ok || !g_res.input_layout || !g_res.vs ||
        !g_res.composite_cb || !g_res.composite_frame_cb || !g_res.dss_test_write) {
        return;
    }

    // The engine's camera projection for this frame.
    RVExtBridge::ProjectionViewTransform pv = {};
    // VOL_REPLAY: at the injection this frame's latch is the pass's camera; no tracker advances.
    if (g_rp.seam_b) {
        if (!g_ro.cycle_pv_valid) {  return; }
        pv = g_ro.cycle_pv;
        // FX_FRAME_ADOPT: the injection's own frame (it adopts this latch), so the footprint lies where the
        // meshes that read it (t24 / t33) are drawn, and on the engine's picture.
        adopt_frame_view(pv);
        if (g_ro.engine_proj_valid) {
            pv.projection[2][2] = g_ro.engine_m22;
            pv.projection[3][2] = g_ro.engine_m32;
        }
        rp_rast_apply(pv.projection);   // REPLAY_RASTER: the copy's raster when the latch's parts from it.
    } else if (!svs_take_pv(ctx, pv)) {  return; }

    float vp_m[4][4] = {};
    mul_4x4(pv.view, pv.projection, vp_m);
    // The priming pass runs later in the frame and must rasterize with this
    // exact transform, or its depth test rejects our own footprint.
    memcpy(g_svs.prime_vp, vp_m, sizeof(g_svs.prime_vp));

    const bool prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;

    StateBackup backup;
    backup.capture(ctx);
    BindScope bc(ctx);   // BIND_FILTER.
    OmSave om;   // Full OM set (8 RTVs + DSV); the DSV is reused below.
    om.capture(ctx);
    ID3D11DepthStencilView* const old_dsv = om.dsv;
    UINT old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&old_nvp, old_vps);

    // Depth target only - the engine's own, exactly as it bound it. Dropping the render target keeps the paired
    // linear-depth colour buffer (which the cascade resolves read) out of this entirely.
    const bool withhold = rp_seam_withhold();   // SEAM_WITHHOLD: no depth target - the footprint draws go nowhere.
    if (withhold) g_rp.withheld_seq = g_svs.frame_seq;
    ctx->OMSetRenderTargets(0, nullptr, withhold ? nullptr : old_dsv);

    float vp_lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    float vp_hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;

    if (!(vp_lo >= 0.0f && vp_hi <= 1.0f && vp_lo < vp_hi)) {
        vp_lo = g_scene_vp_min_d;
        vp_hi = g_scene_vp_max_d;
    }

    D3D11_VIEWPORT v_vp = {};
    v_vp.TopLeftX = 0.0f;
    v_vp.TopLeftY = 0.0f;
    v_vp.Width = static_cast<float>(w);
    v_vp.Height = static_cast<float>(v_h);
    v_vp.MinDepth = vp_lo;
    v_vp.MaxDepth = vp_hi;
    ctx->RSSetViewports(1, &v_vp);
    // VOL_WITNESS: the depth-plane witness's encode (the fallback when the copy has no VOL_FOOT mask), for the
    // copy this seam frame ends in. It is an estimate, not the writer's: the footprint's z is written through the
    // engine's own view block (KhVsCore's seam path, stenVol2.z = 3, remaps z by engBlk's pair), which the CPU never
    // sees - equal at rest, off while the near plane moves. VOL_FOOT's mask makes the witness independent of it.
    scene_depth_pair(pv.projection, g_svs.seam_enc[0], g_svs.seam_enc[1]);
    g_svs.seam_enc[2] = vp_lo;
    g_svs.seam_enc[3] = vp_hi;
    g_svs.seam_enc_seq = g_svs.frame_seq;
    g_svs.seam_proj_ok = sten_proj_terms(pv.projection, g_svs.seam_proj);   // VOL_ZOOM.

    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);   // Depth-only: zero render targets.
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    const bool clamp_want = g_res.rast_inject != nullptr;
    ID3D11RasterizerState* rs = clamp_want ? g_res.rast_inject : g_res.rast_sun;
    const bool rs_have = rs != nullptr;
    const bool rs_own = rs_have;
    if (rs_own) ctx->RSSetState(rs);
    // SEAM_FACE: the colour passes draw a single-sided mesh's front faces only (o.two_sided ? rasterizer :
    // rasterizer_cull; a two-sided translucent's ordered pair draws both), so the footprint, its VOL_FOOT mask and
    // the mirror prepass take the same faces per caster - rs for a two-sided one, its D3D11_CULL_BACK twin for a
    // single-sided one. g_res.rasterizer_cull is that twin: rasterizer and rast_inject are minted from one desc (solid,
    // no bias, no depth clip, ensure_resources), and the hands seam binds the pair for its footprint the same way.
    // Without rast_inject (the rast_sun fallback) every caster keeps rs.
    ID3D11RasterizerState* const rs_cull =
        (rs_own && rs == g_res.rast_inject && g_res.rasterizer_cull) ? g_res.rasterizer_cull : rs;
    ID3D11RasterizerState* rs_bound = rs;
    auto rs_face = [&](bool two) {
        ID3D11RasterizerState* const r = two ? rs : rs_cull;
        if (rs_own && r != rs_bound) { ctx->RSSetState(r); rs_bound = r; }
    };

    // Our passes never touch b2+, so the engine's binds there are never disturbed.
    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);

    ConstantData cbf = {};
    memcpy(cbf.view_proj, vp_m, sizeof(cbf.view_proj));
    // The vertex-path selector: 3 = KhVsCore's seam path, the footprint's z written through the engine's own view block
    // (engBlk's pair - the witness note above). VSMirror does not read it (KhMirClip rebuilds its own near). The flush
    // mesh pass never sets it.
    cbf.sten_vol2[2] = 3.0f;
    float cam[3] = {};
    float cam_rt = -1.0f;
    if (!view_camera_exact(pv.view, cam, &cam_rt)) {
        extract_camera_pos(pv.view, cam);
    }
    const bool engcam = engcam_consume(pv.view, cam);
    // SHADOW_LOD_DRAWN: the level the injection draws at this camera. SEAM_LOD_CAM: picked after the consume,
    // as the injection picks after its own, so both take the camera the consume settled on.
    for (SvCaster& sl_c : list) {
        sl_c.lod = drawn_lod(sl_c.mesh, sl_c.lod_r, sl_c.pos, sl_c.size, sl_c.rot, sl_c.rotated,
                             sl_c.lod_lock, cam);
    }
    const bool rebase_on = engcam ? rebase_vp_engcam(cbf, pv.view, pv.projection)
                                          : rebase_vp(cbf, cam);
    // DDX_REBASE: the fold is rebase_vp's at every site (the wrapper around it only reached its first line).
    // The rebase mutates view_proj in place, so cache the post-rebase matrix.
    memcpy(g_svs.prime_vp, cbf.view_proj, sizeof(g_svs.prime_vp));
    {   // SEAM_CULL: the footprint loop's visibility, through the final matrix.
        CullSet cull;
        cull_build(cbf.view_proj, cam, cull);
        for (SvCaster& sv_c : list) {
            const float ctr[3] = { sv_c.pos[0], sv_c.pos[2], sv_c.pos[1] };   // Engine axes.
            sv_c.a_vis = mesh_visible(cull, ctr,
                                      lod_radius_of(sv_c.bsize, sv_c.rot, sv_c.rotated));   // CAST_REFIT.
        }
    }
    memcpy(g_svs.prime_cam, cam, sizeof(g_svs.prime_cam));
    g_svs.prime_rebase = rebase_on;
    // The priming pass's viewport depth range (svs_prime_mask), published beside its transform; ungated.
    g_svs.prime_vp_lo = vp_lo;
    g_svs.prime_vp_hi = vp_hi;
    // The rebase arm for the RECORD path: the instanced entries read khPass, which nothing fills per mesh, so it is a
    // frame lane. Left zeroed, KhVsCore takes the absolute branch against a rebased viewProj and displaces every
    // instance by the camera vector. MUST STAY ABOVE upload_frame_cb: the frame half is copied once per pass and the
    // mirror prepass re-uploads this same struct verbatim.
    cbf.pass_cam[0] = cam[0];
    cbf.pass_cam[1] = cam[1];
    cbf.pass_cam[2] = cam[2];
    cbf.pass_cam[3] = rebase_on ? 1.0f : 0.0f;
    const bool frame_ok = upload_frame_cb(ctx, g_res.composite_frame_cb, cbf);

    // SVS_SKIP: the signature, both caster lists (world, then in-front) in list order.
    {
        uint64_t sk_h = CryptoGenerator::FNV1A64_OFFSET;   // House FNV-1a 64 (see
                                                             // shader_cache_hash).
        const uintptr_t dsv = reinterpret_cast<uintptr_t>(old_dsv);
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, &dsv, sizeof(dsv));
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, &g_depth_clear_serial, sizeof(g_depth_clear_serial));
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, cbf.view_proj, sizeof(cbf.view_proj));
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, &rebase_on, sizeof(rebase_on));
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, &vp_lo, sizeof(vp_lo));
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, &vp_hi, sizeof(vp_hi));
        // SEAM_SIG_FULL: the in-front list draws in this pass too (the world count), so it is hashed.
        const uint64_t n[2] = { list.size(), front.size() };
        sk_h = CryptoGenerator::fnv1a64_update(sk_h, n, sizeof(n));
        bool tex_alpha = false;   // TEX_LAND: a caster draws through its material's maps.
        bool uv_anim = false;   // UV_ANIM: an alpha caster's coverage moves with the clock.
        for (const std::vector<SvCaster>* l : { &list, &front })
        for (const SvCaster& sk_c : *l) {
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, sk_c.pos, sizeof(sk_c.pos));
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, sk_c.res, sizeof(sk_c.res));   // POS_RES.
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, sk_c.size, sizeof(sk_c.size));
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, sk_c.rot, sizeof(sk_c.rot));
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &sk_c.mesh, sizeof(sk_c.mesh));
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &sk_c.lod, sizeof(sk_c.lod));   // SHADOW_LOD_DRAWN.
            const VbRef vr = mesh_vb_ref(sk_c.mesh, sk_c.slot);
            const uintptr_t vb = reinterpret_cast<uintptr_t>(vr.vb);
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &vb, sizeof(vb));
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &vr.off, sizeof(vr.off));   // SKIN_POOL.
            // CLOTH: a cloth instance keeps one buffer for its whole life and rewrites it every frame, so the
            // pointer above says nothing about whether the geometry moved. Without this the footprint would be
            // rasterised once and then reused while the cloth swung.
            const uint32_t cg = cloth_gen_of(sk_c.slot);
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &cg, sizeof(cg));
            // SEAM_SIG_FULL: these pick loop A or the alpha-tested route, and what the alpha one samples.
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &sk_c.alpha, sizeof(sk_c.alpha));
            const uint64_t mh = sk_c.mats ? sk_c.mats->hash : 0ull;
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &mh, sizeof(mh));
            const uint8_t fl = static_cast<uint8_t>((sk_c.alpha_caster ? 1 : 0) | (sk_c.no_foot ? 2 : 0) |
                                                    (sk_c.two_sided ? 4 : 0));   // SEAM_FACE.
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &fl, 1);
            tex_alpha |= sk_c.alpha_caster && sk_c.mats;
            uv_anim |= sk_c.alpha_caster && sk_c.mats && sk_c.mats->uv_anim_alpha;   // UV_ANIM.
        }
        if (tex_alpha) {   // TEX_LAND.
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &g_tex_land_serial, sizeof(g_tex_land_serial));
        }
        if (uv_anim) {   // UV_ANIM: the union hash's rule.
            const uint64_t ac = mat_anim_cycle_key();
            sk_h = CryptoGenerator::fnv1a64_update(sk_h, &ac, sizeof(ac));
        }
        if (!g_rp.seam_b && g_svs.skip_valid && g_svs.skip_sig == sk_h) {   // VOL_REPLAY: replay B always draws.
            om.restore(ctx);
            if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
            backup.restore(ctx);
            om.release();
            g_ro.in_injection = prev_inj;
            return;
        }
        g_svs.skip_sig = sk_h;
        g_svs.skip_valid = false;   // Proven again by the completed pass below.
    }

    // The instance plan for both loops, built once: loop A the opaque footprint casters (alpha ones need a material per
    // submesh and stay per object), loop B every caster. Both sorted mesh-major so a run of one mesh (level, face rule
    // - SEAM_FACE) is one DrawIndexedInstanced. Safe: both loops are depth-only LESS_EQUAL with no blending, and the
    // skip signature was hashed in list order.
    bool     inst_ok = false;
    uint32_t inst_a = 0;   // Loop A's lane count.
    uint32_t inst_b = 0;   // Loop B's lane count.
    {
        inst_ok = g_res.vs_seam_inst && g_res.vs_mirror_inst &&
                  g_res.layout_sundepth && g_res.obj_srv && !list.empty();

        if (inst_ok) {
            // The records the instanced entries read at t39. scene_sync ran at the scan above; this brings the GPU
            // copy up to it. The seam cannot rely on the sun ladder having synced first.
            inst_ok = objbuf_sync(ctx);
        }

        if (inst_ok) {
            const uint32_t n = static_cast<uint32_t>(list.size());
            g_svs.inst_ord.clear();
            g_svs.inst_ord.reserve(static_cast<size_t>(n) * 2u);

            for (uint32_t i = 0; i < n; ++i) {
                if (!list[i].alpha_caster && list[i].a_vis &&
                    !list[i].no_foot) g_svs.inst_ord.push_back(i);   // SEAM_CULL, FOOT_BLEND.
            }

            inst_a = static_cast<uint32_t>(g_svs.inst_ord.size());
            for (uint32_t i = 0; i < n; ++i) g_svs.inst_ord.push_back(i);
            inst_b = n;
            const std::vector<SvCaster>& src = list;
            // (mesh, level, face): SHADOW_LOD_DRAWN, SEAM_FACE.
            auto bymesh = [&src](uint32_t x, uint32_t y) {
                if (src[x].mesh != src[y].mesh) return src[x].mesh < src[y].mesh;
                if (src[x].lod != src[y].lod) return src[x].lod < src[y].lod;
                return !src[x].two_sided && src[y].two_sided;   // SEAM_FACE: a run per face rule.
            };
            // SKIN_POOL: with a chunk installed, a chunk's casters together within their (mesh, level, face), for
            // its runs (the order is free: depth and MIN only).
            auto bypool = [&](uint32_t x, uint32_t y) {
                if (src[x].mesh != src[y].mesh) return src[x].mesh < src[y].mesh;
                if (src[x].lod != src[y].lod) return src[x].lod < src[y].lod;
                if (src[x].two_sided != src[y].two_sided) {   // SEAM_FACE.
                    return !src[x].two_sided && src[y].two_sided;
                }
                return cast_key_at(src[x].slot) < cast_key_at(src[y].slot);
            };
            if (g_skin_pool_any) {
                for (const SvCaster& kc : list) cast_key_put(kc.mesh, kc.slot);
                std::stable_sort(g_svs.inst_ord.begin(), g_svs.inst_ord.begin() + inst_a, bypool);
                std::stable_sort(g_svs.inst_ord.begin() + inst_a, g_svs.inst_ord.end(), bypool);
            } else {
                std::stable_sort(g_svs.inst_ord.begin(), g_svs.inst_ord.begin() + inst_a, bymesh);
                std::stable_sort(g_svs.inst_ord.begin() + inst_a, g_svs.inst_ord.end(), bymesh);
            }

            const UINT need = static_cast<UINT>(inst_a + inst_b);

            if (need > g_res.seam_instance_cap) {   // Grow-only, doubling (the sun buffer's shape).
                ID3D11Device* idev = nullptr;
                ctx->GetDevice(&idev);

                if (idev) {
                    KH_SAFE_RELEASE(g_res.seam_instance_vb);
                    UINT cap = g_res.seam_instance_cap > 128 ? g_res.seam_instance_cap : 128;
                    while (cap < need) cap *= 2;
                    D3D11_BUFFER_DESC ibd = {};
                    ibd.ByteWidth = cap * INST_STRIDE;
                    ibd.Usage = D3D11_USAGE_DYNAMIC;
                    ibd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                    ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                    if (SUCCEEDED(idev->CreateBuffer(&ibd, nullptr, &g_res.seam_instance_vb))) {
                        g_res.seam_instance_cap = cap;
                    } else {
                        g_res.seam_instance_cap = 0;
                    }

                    idev->Release();
                }
            }

            inst_ok = g_res.seam_instance_vb && need <= g_res.seam_instance_cap;
        }

        if (inst_ok) {   // One WRITE_DISCARD for both loops' lanes.
            D3D11_MAPPED_SUBRESOURCE im = {};

            if (SUCCEEDED(ctx->Map(g_res.seam_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) {
                InstLane* lane = static_cast<InstLane*>(im.pData);

                for (uint32_t i = 0; i < inst_a + inst_b; ++i) {
                    const SvCaster& v_c = list[g_svs.inst_ord[i]];
                    lane->slot = v_c.slot;   // OBJBUF: the record index.
                    lane->dither = 0.0f;       // Unread by both seam entry points.
                    lane->alpha = v_c.alpha;
                    lane->mat_base = 0.0f;
                    ++lane;
                }

                ctx->Unmap(g_res.seam_instance_vb, 0);
            } else {
                inst_ok = false;   // Both loops fall back to their per-caster path.
            }
        }
    }

    if (frame_ok) {
        UINT stride = sizeof(MeshVertex);
        VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.

        // The alpha casters take the textured VS + layout and the clip-only PS, per submesh with the material bound;
        // the whole-object translucent ones are skipped. State flips per caster (alpha_bound) and the opaque
        // footprint state returns at the end.
        bool alpha_bound = false;
        ID3D11Device* v_dev = nullptr;
        ctx->GetDevice(&v_dev);

        // Hoisted out of the per-caster loop (ConstantData is over 5 KB). Every field the body reads is rewritten each
        // iteration; mat_ctl is written only by bind_material (the alpha path), and the alpha path zeroes it again
        // on its way out (the memset after its submesh loop).
        ConstantData obj = {};

        // SEAM_WITHHOLD_DRAW: a withheld seam binds neither a depth nor a render target, so its footprint draws
        // land nowhere: none is issued - the instanced runs are skipped whole (binds and draws), and the per-caster
        // paths bind, upload (a set's first table fill keeps its place; a failed upload still ends the loop) and
        // draw nothing. The mirror prepass and the VOL_FOOT mask below bind their own state and still draw.
        if (inst_ok && !withhold) {   // SEAM_INST: one draw per mesh run, not per caster.
            ID3D11Buffer* ivb[2] = { nullptr, g_res.seam_instance_vb };
            const UINT istr[2] = { sizeof(MeshVertex), INST_STRIDE };
            UINT ioff[2] = { 0, 0 };
            ctx->IASetInputLayout(g_res.layout_sundepth);   // VSInSeam mirrors VSInSun.
            ctx->VSSetShader(g_res.vs_seam_inst, nullptr, 0);
            objbuf_bind(ctx);   // OBJBUF: t39, the records the lane indexes.
            uint32_t first = 0;
            DepthSk sk = { g_res.vs_seam_inst, g_res.layout_sundepth, g_res.vs_seam_inst_sk, nullptr, nullptr };

            while (first < inst_a) {
                const int mid = list[g_svs.inst_ord[first]].mesh;
                const int lod = list[g_svs.inst_ord[first]].lod;   // SHADOW_LOD_DRAWN.
                uint32_t last = first + 1;
                const VbRef rv = mesh_vb_ref(mid, list[g_svs.inst_ord[first]].slot);   // CLOTH.
                const SkinPoolChunk* const pc =   // SKIN_POOL.
                    cast_pool_of(mid, list[g_svs.inst_ord[first]].slot, g_res.vs_seam_inst_sk);

                const bool two = list[g_svs.inst_ord[first]].two_sided;   // SEAM_FACE.
                while (last < inst_a &&
                       list[g_svs.inst_ord[last]].mesh == mid &&
                       list[g_svs.inst_ord[last]].lod == lod &&
                       list[g_svs.inst_ord[last]].two_sided == two &&
                       cast_run_joins(rv, pc, list[g_svs.inst_ord[last]].mesh,
                                      list[g_svs.inst_ord[last]].slot, g_res.vs_seam_inst_sk)) ++last;

                rs_face(two);
                sk.use(ctx, pc);
                ivb[0] = rv.vb;
                ioff[0] = rv.off;   // SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, ivb, istr, ioff);
                ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                // The run's level range, exactly as the per-caster draw below takes it (the level the injection drew).
                UINT is = 0, ic = 0;
                mesh_lod_range_of(mid, lod, is, ic);
                ctx->DrawIndexedInstanced(ic, last - first, is, 0, first);
                first = last;
            }
            sk.end(ctx);   // SKIN_POOL.

            // Back to the opaque per-object footprint state for the alpha casters.
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
        }

        for (const auto& c : list) {
            // The opaque set went out instanced above; the partition is exact (alpha_caster is false for every lane
            // written). A see-through blend mode is in neither (FOOT_BLEND).
            if (!c.a_vis) continue;   // SEAM_CULL: outside the footprint's frustum.
            if (c.no_foot) continue;   // FOOT_BLEND: the world behind shows through.
            if (inst_ok && !c.alpha_caster) continue;
            // Writes no depth in the colour pass: not drawn.
            if (c.alpha_caster && c.alpha < 0.999f) {  continue; }
            const bool want_alpha = c.alpha_caster && c.mats && v_dev;
            rs_face(c.two_sided);   // SEAM_FACE.

            if (want_alpha != alpha_bound) {
                ctx->IASetInputLayout(want_alpha ? g_res.layout_tex : g_res.input_layout);
                ctx->VSSetShader(want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                ctx->PSSetShader(want_alpha ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
                if (want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                alpha_bound = want_alpha;
            }

            const VbRef want = mesh_vb_ref(c.mesh, c.slot);   // CLOTH.
            if (want != bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &want.vb, &stride, &want.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                bound_vb = want;
            }

            obj.depth_params[2] = vp_lo;
            obj.depth_params[3] = vp_hi;
            obj.fx_meta[0] = 0.0f;
            obj.fx_meta[1] = 0.0f;
            obj.center_size[0] = c.pos[0];
            obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            obj.center_size[2] = c.pos[1];
            // A footprint drawn a few percent larger covers the visible box under any displacement up to that margin
            // (only the engine's ground shading reads those texels). This is 1.0 and the instanced twin reads the
            // record's extents with no grow lane; raising it needs a matching lane on the instance stream.
            float grow = 1.0f;
            obj.size_axes[0] = c.size[0] * grow;
            obj.size_axes[1] = c.size[2] * grow;
            obj.size_axes[2] = c.size[1] * grow;
            fill_obj_rot(obj, c.rot);
            if (rebase_on) fill_center_rel(obj, cam, c.res);   // POS_RES.

            if (want_alpha) {   // FOOTPRINT_ALPHA: per submesh, the material's alpha lanes +
                                    // maps bound.
                const MeshDef& md = mesh_def(c.mesh);
                const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, c.lod);   // SHADOW_LOD_DRAWN.
                bool ok = true;

                for (size_t s = 0; s < tab.size() && ok; ++s) {
                    const MeshSubmesh& sm = tab[s];
                    if (sm.index_count == 0) continue;
                    bind_material(ctx, v_dev, obj, c.mats, s);
                    ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                    if (!ok) break;
                    if (!withhold) {   // SEAM_WITHHOLD_DRAW.
                        ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                    }
                }
                memset(obj.mat_ctl, 0, sizeof(obj.mat_ctl));   // SEAM_CB_HOIST: the
                                                                       // only lanes this path dirties.

                if (!ok) {  break; }
                continue;
            }

            if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) {
                break;
            }

            {   // SHADOW_LOD_DRAWN.
                UINT is = 0, ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, is, ic);
                if (!withhold) ctx->DrawIndexed(ic, is, 0);   // SEAM_WITHHOLD_DRAW.
            }
        }

        if (alpha_bound) {   // The opaque footprint state again (the mirror prepass below
                                 // rebinds its own).
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
        }
        // INFRONT: the view-model casters join the WORLD count. The engine's hands recount (infront_seam_inject's
        // window) redraws only the volumes that reach the camera, and the depth partition hides every world-range
        // volume from a hands-range depth, so a shadow from a vehicle or a building reaches an inFront mesh only
        // through this draw: the same footprint state, the world range, the seam's own transform, under the depth clamp
        // (rast_inject, or its SEAM_FACE twin) - a fragment nearer than the world near plane lands on it, which is
        // the right count there since the engine clips its volumes' front faces short of it. The stencil is untouched,
        // and the hands-seam draw keeps it, so this count survives to the resolve (the recount adds its near subset on
        // top; a shadowed count stays nonzero and small).
        for (const auto& c : front) {
            if (c.no_foot) continue;   // FOOT_BLEND: the world behind shows through.
            const bool want_alpha = c.alpha_caster && c.mats && v_dev;
            rs_face(c.two_sided);   // SEAM_FACE.
            if (want_alpha != alpha_bound) {
                ctx->IASetInputLayout(want_alpha ? g_res.layout_tex : g_res.input_layout);
                ctx->VSSetShader(want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                ctx->PSSetShader(want_alpha ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
                if (want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                alpha_bound = want_alpha;
            }
            const VbRef vb = mesh_vb_ref(c.mesh, c.slot);   // CLOTH.
            if (vb != bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                bound_vb = vb;
            }
            obj.depth_params[2] = vp_lo;
            obj.depth_params[3] = vp_hi;
            obj.fx_meta[0] = 0.0f;
            obj.fx_meta[1] = 0.0f;
            obj.center_size[0] = c.pos[0];
            obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            obj.center_size[2] = c.pos[1];
            obj.size_axes[0] = c.size[0];
            obj.size_axes[1] = c.size[2];
            obj.size_axes[2] = c.size[1];
            fill_obj_rot(obj, c.rot);
            if (rebase_on) fill_center_rel(obj, cam, c.res);   // POS_RES.
            bool ok = true;
            if (want_alpha) {
                const MeshDef& md = mesh_def(c.mesh);
                const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, 0);
                for (size_t s = 0; s < tab.size() && ok; ++s) {
                    const MeshSubmesh& sm = tab[s];
                    if (sm.index_count == 0) continue;
                    bind_material(ctx, v_dev, obj, c.mats, s);
                    ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                    if (!ok) break;
                    if (!withhold) {   // SEAM_WITHHOLD_DRAW.
                        ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                    }
                }
                memset(obj.mat_ctl, 0, sizeof(obj.mat_ctl));
            } else {
                ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                if (ok) {
                    UINT is = 0, ic = 0;
                    mesh_lod_range_of(c.mesh, 0, is, ic);
                    if (!withhold) ctx->DrawIndexed(ic, is, 0);   // SEAM_WITHHOLD_DRAW.
                }
            }
            if (!ok) break;
        }
        if (alpha_bound) {
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
        }
        if (v_dev) v_dev->Release();

        if (!g_rp.seam_b) g_svs.injected_frame = true;   // VOL_REPLAY: replay B is not this frame's seam.

        g_svs.prime_ready = true;   // Transform + list valid.
    }

    // Grafted at the instant the footprint already draws at, inside the same state envelope - the tail below restores
    // everything this block touches. Runs whether or not the footprint's frame CB landed: the mirror uploads its own
    // frame slice.
    {
        // INFRONT; MIR_REPLAY: for B only when rp_replay armed mirror B.
        bool go = (!list.empty() || !front.empty()) && (!g_rp.seam_b || g_rp.mir_want);
        // MIR_GATE: the real mirror only when a mesh can read it next cycle; skipped, it is marked absent, so
        // every reader takes mirMeta.x = 0 (no mirror) rather than an older one.
        if (go && !g_rp.seam_b && !vmir_wanted(cam, pv.projection)) {
            go = false;
            g_vmir.mask_time = -1.0f;
        }
        if (go) {
            ID3D11Device* vm_dev = nullptr;
            ctx->GetDevice(&vm_dev);
            bool ok = false;
            if (vm_dev) {
                {
                    ok = vmir_ensure_targets(vm_dev) && vmir_ensure_gpu(vm_dev);
                }
                vm_dev->Release();
            }
            // MIR_REPLAY: in B mode the prepass is mirror B's, at this cycle's pose (b2 is the pass's first draw's,
            // bound by rp_replay_b, as the real prepass read it); the real mirror stays as the pass counted it.
            ID3D11DepthStencilView* const dsv = g_rp.seam_b ? g_rp.mirb_dsv : g_vmir.dsv;
            if (ok && g_vmir.vs && dsv) {
                // The frame CB uploads verbatim - VSMirror takes x/y/w from
                // this matrix and remaps z through the live-b2 low pair itself.
                if (upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
                    ctx->ClearDepthStencilView(dsv,
                        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                    ctx->OMSetRenderTargets(0, nullptr, dsv);
                    D3D11_VIEWPORT vm_vp = {};
                    vm_vp.Width = static_cast<float>(g_vmir.w);
                    vm_vp.Height = static_cast<float>(g_vmir.h);
                    vm_vp.MinDepth = 0.0f;
                    vm_vp.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &vm_vp);
                    ctx->PSSetShader(nullptr, nullptr, 0);

                    if (inst_ok) {   // SEAM_INST: one draw per mesh run, not per caster.
                        ID3D11Buffer* ivb[2] = { nullptr, g_res.seam_instance_vb };
                        const UINT istr[2] = { sizeof(MeshVertex), INST_STRIDE };
                        UINT ioff[2] = { 0, 0 };
                        ctx->IASetInputLayout(g_res.layout_sundepth);
                        ctx->VSSetShader(g_res.vs_mirror_inst, nullptr, 0);
                        objbuf_bind(ctx);   // OBJBUF: t39, the records the lane indexes.
                        uint32_t first = 0;
                        DepthSk sk = { g_res.vs_mirror_inst, g_res.layout_sundepth, g_res.vs_mirror_inst_sk,
                                       nullptr, nullptr };   // SKIN_POOL.

                        while (first < inst_b) {
                            // Loop B's lanes sit at [inst_a, inst_a + inst_b) in the one buffer, so every
                            // run index carries that base.
                            const int mid = list[g_svs.inst_ord[inst_a + first]].mesh;
                            // SHADOW_LOD_DRAWN.
                            const int lod = list[g_svs.inst_ord[inst_a + first]].lod;
                            uint32_t last = first + 1;
                            const VbRef rv = mesh_vb_ref(mid,
                                list[g_svs.inst_ord[inst_a + first]].slot);   // CLOTH.
                            const SkinPoolChunk* const pc = cast_pool_of(mid,   // SKIN_POOL.
                                list[g_svs.inst_ord[inst_a + first]].slot, g_res.vs_mirror_inst_sk);

                            // SEAM_FACE: a run is one face rule too.
                            const bool two = list[g_svs.inst_ord[inst_a + first]].two_sided;
                            while (last < inst_b &&
                                   list[g_svs.inst_ord[inst_a + last]].mesh == mid &&
                                   list[g_svs.inst_ord[inst_a + last]].lod == lod &&
                                   list[g_svs.inst_ord[inst_a + last]].two_sided == two &&
                                   cast_run_joins(rv, pc,
                                                  list[g_svs.inst_ord[inst_a + last]].mesh,
                                                  list[g_svs.inst_ord[inst_a + last]].slot,
                                                  g_res.vs_mirror_inst_sk)) ++last;

                            rs_face(two);   // SEAM_FACE.
                            sk.use(ctx, pc);
                            ivb[0] = rv.vb;
                            ioff[0] = rv.off;   // SKIN_POOL.
                            ctx->IASetVertexBuffers(0, 2, ivb, istr, ioff);
                            ctx->IASetIndexBuffer(g_res.mesh_ib[mid], DXGI_FORMAT_R32_UINT, 0);
                            UINT is = 0, ic = 0;
                            mesh_lod_range_of(mid, lod, is, ic);   // SHADOW_LOD_DRAWN.
                            ctx->DrawIndexedInstanced(ic, last - first, is, 0,
                                                      inst_a + first);
                            first = last;
                        }
                        sk.end(ctx);   // SKIN_POOL.
                    } else {
                        ctx->VSSetShader(g_vmir.vs, nullptr, 0);
                        UINT stride = sizeof(MeshVertex);
                        VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
                        // SEAM_CB_HOIST: see the twin above.
                        ConstantData obj = {};
                        for (const auto& c : list) {
                            rs_face(c.two_sided);   // SEAM_FACE.
                            const VbRef want = mesh_vb_ref(c.mesh, c.slot);   // CLOTH.
                            if (want != bound_vb) {
                                ctx->IASetVertexBuffers(0, 1, &want.vb,
                                                        &stride, &want.off);
                                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                                bound_vb = want;
                            }
                            obj.center_size[0] = c.pos[0];
                            obj.center_size[1] = c.pos[2];   // SQF -> engine axes.
                            obj.center_size[2] = c.pos[1];
                            obj.size_axes[0] = c.size[0];
                            obj.size_axes[1] = c.size[2];
                            obj.size_axes[2] = c.size[1];
                            fill_obj_rot(obj, c.rot);
                            if (rebase_on) fill_center_rel(obj, cam, c.res);   // POS_RES.
                            if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) break;
                            {   // SHADOW_LOD_DRAWN.
                                UINT is = 0, ic = 0;
                                mesh_lod_range_of(c.mesh, c.lod, is, ic);
                                ctx->DrawIndexed(ic, is, 0);
                            }
                        }
                    }   // SEAM_INST: the per-caster fallback.
                    // INFRONT: the view-model casters into the mirror, per caster, the world casters' own recipe
                    // (VSMirror: x / y / w from the seam's matrix, z on the 0.05 near). The colour pass reads the
                    // mirror over the whole mesh (mirMeta.x = 2), so the near collapse the mirror exists for never
                    // reaches it; the volume-buffer draws stay as the fallback where the mirror is not built.
                    if (!front.empty()) {
                        ctx->IASetInputLayout(g_res.input_layout);
                        ctx->VSSetShader(g_vmir.vs, nullptr, 0);
                        UINT stride = sizeof(MeshVertex);
                        VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
                        ConstantData obj = {};
                        for (const auto& c : front) {
                            rs_face(c.two_sided);   // SEAM_FACE.
                            const VbRef want = mesh_vb_ref(c.mesh, c.slot);
                            if (want != bound_vb) {
                                ctx->IASetVertexBuffers(0, 1, &want.vb, &stride, &want.off);
                                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                                bound_vb = want;
                            }
                            obj.center_size[0] = c.pos[0];
                            obj.center_size[1] = c.pos[2];   // SQF -> engine axes.
                            obj.center_size[2] = c.pos[1];
                            obj.size_axes[0] = c.size[0];
                            obj.size_axes[1] = c.size[2];
                            obj.size_axes[2] = c.size[1];
                            fill_obj_rot(obj, c.rot);
                            if (rebase_on) fill_center_rel(obj, cam, c.res);   // POS_RES.
                            if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) break;
                            UINT is = 0, ic = 0;
                            mesh_lod_range_of(c.mesh, 0, is, ic);
                            ctx->DrawIndexed(ic, is, 0);
                        }
                    }
                    if (g_rp.seam_b) {
                        g_rp.mirb_drawn = true;   // MIR_REPLAY: mirror B; the real mirror's stamps stand.
                    } else {
                        g_vmir.prepass_stamp = static_cast<uint32_t>(g_svs.frame_seq);
                        g_vmir.mask_time = effect_time_seconds();
                        g_vmir.prepass_src = g_svs.vol_src;
                    }
                }
            }
        }
    }

    // VOL_FOOT: the footprint's view distance into the unpublished mask - the world casters (list) with the same
    // transforms, levels, frustum cut and alpha clip as the footprint above, with no depth test (the engine's depth is
    // what this replaces) and a MIN blend (the nearest of our surfaces per pixel). The view-model casters the footprint
    // adds to the world count (front) are not in it: their own receivers read no witness (the slice zeroes
    // stenVol4), and a world surface of ours they cover is hidden under the slice. The tail below restores every state
    // this touches.
    if (frame_ok && g_res.ps_seam_foot && g_res.dlsw_min_blend && g_res.dss_off &&
        svs_foot_ensure(ctx, w, v_h)) {
        const int ix = g_svs.foot_pub == 0 ? 1 : 0;
        const FLOAT far4[4] = { 1.0e30f, 1.0e30f, 1.0e30f, 1.0e30f };
        ctx->OMSetRenderTargets(1, &g_svs.foot_rtv[ix], nullptr);
        ctx->ClearRenderTargetView(g_svs.foot_rtv[ix], far4);
        ctx->RSSetViewports(1, &v_vp);
        if (rs_own) ctx->RSSetState(rs);
        rs_bound = rs;   // SEAM_FACE.
        const FLOAT bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ctx->OMSetBlendState(g_res.dlsw_min_blend, bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 2, cbs);
        ctx->PSSetConstantBuffers(0, 2, cbs);
        ctx->PSSetShader(g_res.ps_seam_foot, nullptr, 0);
        // The mirror prepass above may have re-uploaded the frame half; this is the footprint's own.
        bool ok = upload_frame_cb(ctx, g_res.composite_frame_cb, cbf);
        if (ok && inst_ok) {   // Loop A's runs, as the footprint forms them.
            ID3D11Buffer* ivb[2] = { nullptr, g_res.seam_instance_vb };
            const UINT istr[2] = { sizeof(MeshVertex), INST_STRIDE };
            UINT ioff[2] = { 0, 0 };
            ctx->IASetInputLayout(g_res.layout_sundepth);
            ctx->VSSetShader(g_res.vs_seam_inst, nullptr, 0);
            objbuf_bind(ctx);
            uint32_t first = 0;
            DepthSk sk = { g_res.vs_seam_inst, g_res.layout_sundepth, g_res.vs_seam_inst_sk, nullptr, nullptr };
            while (first < inst_a) {
                const SvCaster& c0 = list[g_svs.inst_ord[first]];
                const VbRef vb0 = mesh_vb_ref(c0.mesh, c0.slot);
                const SkinPoolChunk* const pc =   // SKIN_POOL: loop A's runs, as the footprint forms them.
                    cast_pool_of(c0.mesh, c0.slot, g_res.vs_seam_inst_sk);
                uint32_t last = first + 1;
                while (last < inst_a &&
                       list[g_svs.inst_ord[last]].mesh == c0.mesh &&
                       list[g_svs.inst_ord[last]].lod == c0.lod &&
                       list[g_svs.inst_ord[last]].two_sided == c0.two_sided &&   // SEAM_FACE.
                       cast_run_joins(vb0, pc, list[g_svs.inst_ord[last]].mesh,
                                      list[g_svs.inst_ord[last]].slot, g_res.vs_seam_inst_sk)) ++last;
                rs_face(c0.two_sided);   // SEAM_FACE.
                sk.use(ctx, pc);
                ivb[0] = vb0.vb;
                ioff[0] = vb0.off;   // SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, ivb, istr, ioff);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c0.mesh], DXGI_FORMAT_R32_UINT, 0);
                UINT is = 0, ic = 0;
                mesh_lod_range_of(c0.mesh, c0.lod, is, ic);
                ctx->DrawIndexedInstanced(ic, last - first, is, 0, first);
                first = last;
            }
            sk.end(ctx);   // SKIN_POOL.
        }
        if (ok) {   // The per-object casters (the alpha ones, or all without instancing).
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            const UINT stride = sizeof(MeshVertex);
            VbRef bound;   // SKIN_POOL: the pair.
            ConstantData obj = {};
            // FOOTPRINT_ALPHA's rule here too: a cutout hole or a blend material's see-through texel is not in the
            // footprint, so it must not be in the mask - drawn, it would hold the see-through surface's distance where
            // the engine counted what is behind it, and our surface there (an interior through glass, a mesh behind a
            // cutout) would fail the witness and go lit. An alpha caster the footprint drew per submesh draws the same
            // way, through the textured VS and PSDlsMaskA: PSInjDepthA's clip, then SV_Position.w (its LOD dither lane
            // reads this pass's zero blendCtl.w - off). Absent twin: the caster masks whole, as the dlsw mask does.
            ID3D11Device* vp_dev = nullptr;
            ctx->GetDevice(&vp_dev);
            const bool alpha_ok = vp_dev && g_res.ps_dls_mask_a && g_res.vs_tex && g_res.layout_tex;
            bool alpha_bound = false;
            for (const auto& c : list) {
                if (!c.a_vis) continue;                              // As the footprint.
                if (c.no_foot) continue;                             // FOOT_BLEND: as the footprint.
                if (inst_ok && !c.alpha_caster) continue;        // Drawn instanced above.
                if (c.alpha_caster && c.alpha < 0.999f) continue;    // Writes no depth: no footprint.
                // The footprint's own test (want_alpha): per submesh of the drawn
                // level (c.lod, the injection's), else that level whole.
                const bool fp_alpha = c.alpha_caster && c.mats && vp_dev;
                rs_face(c.two_sided);   // SEAM_FACE.
                const bool want_alpha = fp_alpha && alpha_ok;
                if (want_alpha != alpha_bound) {
                    ctx->IASetInputLayout(want_alpha ? g_res.layout_tex : g_res.input_layout);
                    ctx->VSSetShader(want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                    ctx->PSSetShader(want_alpha ? g_res.ps_dls_mask_a : g_res.ps_seam_foot, nullptr, 0);
                    if (want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                    alpha_bound = want_alpha;
                }
                const VbRef vb = mesh_vb_ref(c.mesh, c.slot);
                if (vb != bound) {
                    ctx->IASetVertexBuffers(0, 1, &vb.vb, &stride, &vb.off);
                    ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                    bound = vb;
                }
                obj.depth_params[2] = vp_lo;
                obj.depth_params[3] = vp_hi;
                obj.center_size[0] = c.pos[0];
                obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
                obj.center_size[2] = c.pos[1];
                obj.size_axes[0] = c.size[0];
                obj.size_axes[1] = c.size[2];
                obj.size_axes[2] = c.size[1];
                fill_obj_rot(obj, c.rot);
                if (rebase_on) fill_center_rel(obj, cam, c.res);   // POS_RES.
                if (want_alpha) {   // The footprint's submeshes, each with its material.
                    const MeshDef& md = mesh_def(c.mesh);
                    const std::vector<MeshSubmesh>& tab = mesh_lod_submeshes(md, c.lod);
                    for (size_t s = 0; s < tab.size() && ok; ++s) {
                        const MeshSubmesh& sm = tab[s];
                        if (sm.index_count == 0) continue;
                        bind_material(ctx, vp_dev, obj, c.mats, s);
                        ok = upload_obj_cb(ctx, g_res.composite_cb, obj);
                        if (!ok) break;
                        ctx->DrawIndexed(sm.index_count, sm.index_start, 0);
                    }
                    memset(obj.mat_ctl, 0, sizeof(obj.mat_ctl));   // The only lanes it dirties.
                    if (!ok) break;
                    continue;
                }
                if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) { ok = false; break; }
                // The level the footprint drew: the injection's (SHADOW_LOD_DRAWN), whole or per submesh alike.
                UINT is = 0, ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, is, ic);
                ctx->DrawIndexed(ic, is, 0);
            }
            if (vp_dev) vp_dev->Release();
        }
        if (ok) {
            g_svs.foot_seam_ix = ix;
            g_svs.foot_seam_seq = g_svs.frame_seq;
        }
    }

    // SVS_SKIP: this signature's pass completed (SEAM_WITHHOLD: a withheld one drew nothing).
    g_svs.skip_valid = !withhold;
    om.restore(ctx);
    if (old_nvp > 0) ctx->RSSetViewports(old_nvp, old_vps);
    backup.restore(ctx);
    om.release();
    g_ro.in_injection = prev_inj;
}

// Runs on every OM depth bind, before the live-shadow gate: the seam has
// nothing to do with whether cascades are wanted.
inline void volume_seam_track(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv,
                              bool dsv_is_main, UINT n, ID3D11RenderTargetView* const* rtvs) {
    g_svs.vol_dsv_now = false;
    void* id = dsv ? reorder_dsv_identity(dsv) : nullptr;
    g_svs.vol_dsv_bound = g_svs.vol_src && id && id == g_svs.vol_src_id;   // INFRONT: any bind of it.
    const bool depth_only = dsv && (n == 0 || !rtvs || !rtvs[0]);
    const bool has_rtv = dsv && n > 0 && rtvs && rtvs[0];

    // One GetDesc per identity per frame for the first 8 identities (the
    // cache); their repeats are a pointer compare, a 9th pays it every bind.
    if (id) {
        int cls = -1;

        for (uint32_t i = 0; i < g_svs.cls_n; ++i) {
            if (g_svs.cls_id[i] == id) { cls = g_svs.cls_srv[i]; break; }
        }

        if (cls < 0) {
            ID3D11Resource* r = nullptr;
            dsv->GetResource(&r);

            if (r) {
                ID3D11Texture2D* t = nullptr;
                r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&t));
                r->Release();

                if (t) {
                    D3D11_TEXTURE2D_DESC d = {};
                    t->GetDesc(&d);
                    t->Release();
                    cls = (d.BindFlags & D3D11_BIND_SHADER_RESOURCE) ? 1 : 0;

                    if (g_svs.cls_n < 8) {
                        g_svs.cls_id[g_svs.cls_n] = id;
                        g_svs.cls_srv[g_svs.cls_n] = static_cast<uint8_t>(cls);
                        g_svs.cls_n++;
                    }
                }
            }
        }

        // Mask candidate latch.
        if (cls == 0) {

            if (has_rtv && rtvs && rtvs[0]) {
                ID3D11Resource* mr = nullptr;
                rtvs[0]->GetResource(&mr);

                if (mr) {
                    ID3D11Texture2D* mt = nullptr;
                    mr->QueryInterface(__uuidof(ID3D11Texture2D),
                                       reinterpret_cast<void**>(&mt));
                    mr->Release();

                    if (mt) {
                        D3D11_TEXTURE2D_DESC md = {};
                        mt->GetDesc(&md);
                        if (g_svs.mask_id &&
                            static_cast<void*>(mt) == g_svs.mask_id) {
                            g_svs.bracket = 1;   // Pre at the next draw.
                            svs_vol_latch(dsv, id);
                        }
                        const bool ok =
                            (md.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
                            g_main_depth_w && md.Width == g_main_depth_w &&
                            md.Height == g_main_depth_h;

                        if (ok) {
                            if (g_svs.mask_cand) g_svs.mask_cand->Release();
                            g_svs.mask_cand = mt;
                        } else {
                            mt->Release();
                        }
                    }
                }
            }
        }
    }

    if (has_rtv && id) {
        bool known = false;

        for (uint32_t i = 0; i < g_svs.rtv_seen_n; ++i) {
            if (g_svs.rtv_seen[i] == id) { known = true; break; }
        }

        if (!known && g_svs.rtv_seen_n < 8) g_svs.rtv_seen[g_svs.rtv_seen_n++] = id;
    }

    bool seen_rtv = false;

    for (uint32_t i = 0; i < g_svs.rtv_seen_n; ++i) {
        if (g_svs.rtv_seen[i] == id) { seen_rtv = true; break; }
    }

    const bool seam = depth_only && id && seen_rtv;

    if (!seam) return;

    // Shape, fetched only on a seam (once or twice a frame), never per bind.
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return;
    ID3D11Texture2D* tex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (!tex) return;
    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    tex->Release();

    // The scene prepass's target is the main depth's size. SEAM_SQUARE: only while that is not yet known is a
    // square target taken for a cascade. Do not test the square rule first: it refuses a square main depth (a square
    // window's), and every seam, footprint mask and mirror with it. A cascade the main depth's size is still
    // refused below: it is SRV-readable.
    if (g_main_depth_w && g_main_depth_h) {
        if (td.Width != g_main_depth_w || td.Height != g_main_depth_h) return;
    } else if (td.Width == td.Height) {
        return;
    }

    // The shader-resource discriminator: the colour pass's depth is typeless and SRV-readable; the volume buffer is
    // write/test-only and never read as an SRV anywhere in the frame. An ordinal cannot identify a resource.
    if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        return;
    }

    {
        // Mask without our depth in the buffer would answer nothing.
        if (!svs_inject_armed()) {
            return;
        }
    }
    // This DSV is the volume buffer, so the draws that follow are counting draws until the next bind.
    g_svs.vol_dsv_now = true;

    if (g_svs.injected_frame) return;

    g_svs.pending = true;
    g_svs.pend_w = td.Width;
    g_svs.pend_h = td.Height;
}

inline void shadow_track_targets(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, bool dsv_is_main,
                                 UINT n, ID3D11RenderTargetView* const* rtvs) {
    // Seam tracking runs first and unconditionally - independent of
    // shadow_live_wanted, which gates the cascade machinery below.
    volume_seam_track(ctx, dsv, dsv_is_main, n, rtvs);
    if (!shadow_live_wanted()) {
        if (g_sr.phase_active) {
            g_sr.phase_active = false;
            g_sr.target_identity = nullptr;
        }

        g_ls.phase_on_atlas = false;
        g_ls.pending_valid = false;
        g_ls.pending_bytes = 0;
        g_ls.pending_vp_valid = false;   // LS_VP_LATCH: the atlas phase ends here with no finalize.
        return;
    }

    const bool depth_only = dsv && !dsv_is_main && (n == 0 || !rtvs || !rtvs[0]);
    void* id = depth_only ? reorder_dsv_identity(dsv) : nullptr;
    bool is_atlas = false;

    if (depth_only && (!g_sr.phase_active || g_sr.target_identity != id)) {
        is_atlas = shadow_probe_target(dsv);
    } else if (depth_only) {
        is_atlas = g_ls.phase_on_atlas;
    }

    const bool atlas_cycle_start =
        is_atlas && (!g_ls.phase_on_atlas || id != g_sr.target_identity);

    // Cycle boundary on the atlas: commit the cycle's fully-upgraded latch then
    // grab per-cycle state; the cascade's draws are complete here.
    if (g_ls.phase_on_atlas && (!is_atlas || atlas_cycle_start)) {
        shadow_live_finalize_cycle();

    }

    g_sr.phase_active = depth_only;
    g_sr.target_identity = id;
    g_ls.phase_on_atlas = is_atlas;

    if (atlas_cycle_start) {
        g_ls.cycle_latched = false;   // New cascade cycle, new latch.
        g_atlas_frame_cur = true;   // Atlas stream liveness.
    }
    if (!is_atlas) { g_ls.pending_valid = false; g_ls.pending_bytes = 0; g_ls.pending_vp_valid = false; }
}

// FOG_PROBE: the probes' demand on the funnel when no mesh opens it (cbc_on). Locked, a probe takes its own
// buffer only; unlocked, the light probe hunts every map inside its attempt window (the ~120 ms of each second
// locator_note_upload scans in) and the sky probe every map until it locks.
inline bool fog_probe_admits(ID3D11Resource* res) {
    if (!g_fogsc_wanted.load(std::memory_order_relaxed)) return false;
    const void* r = static_cast<void*>(res);
    if (g_sky_probe.valid ? r == g_sky_probe.buf : true) return true;
    if (g_light_probe.valid) return r == g_light_probe.buf;
    const float now = effect_time_seconds();
    return now - floorf(now) < 0.12f;
}

inline void svs_bracket_post(ID3D11DeviceContext* self) {
    // Context test first: g_svs.bracket is a render-thread byte, and this runs after every draw of the four core draw
    // hooks (not DrawAuto or the indirect draws) on every thread using the vtable.
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    if (g_svs.bracket != 2) return;
    if (g_ro.in_injection) return;
    g_svs.bracket = 0;
    if (svs_snap_wanted()) svs_mask_snap(self, true);
}

inline void svs_prime_mask(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_SVS_PRIME);
    KH_GPU_SCOPE(ctx, GPU_SVS_PRIME);   // PROF.
    if (!ctx || g_ro.in_injection) return;

    // No mode test here.
    if (!g_res.ps_maskprime || !g_res.input_layout || !g_res.vs) {  return; }
    if (!g_svs.prime_ready) {  return; }
    if (g_svs.caster_list.empty()) {  return; }
    if (!g_mask.engine_mask_rtv) {  return; }

    ID3D11Resource* res = nullptr;
    g_mask.engine_mask_rtv->GetResource(&res);
    const bool same = res && g_svs.mask_id &&
                      static_cast<void*>(res) == g_svs.mask_id;
    if (res) res->Release();
    if (!same) {  return; }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool st = svs_prime_states(dev);
    if (dev) dev->Release();
    if (!st) {  return; }

    const bool prev = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup bk;
    bk.capture(ctx);
    OmSave om;   // Full OM set (8 RTVs + DSV + the UAVs above them); the DSV is rebound below.
    om.capture(ctx);
    ID3D11DepthStencilView* const old_dsv = om.dsv;
    UINT nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&nvp, old_vps);

    // The engine's own depth target stays bound: it still holds the depth the injection wrote, so the LESS_EQUAL test
    // is meant to land the write on exactly our surface (SVS_OFF_UNMAINT: it does not - the z path and rasterizer
    // below are not the footprint's).
    ctx->OMSetRenderTargets(1, &g_mask.engine_mask_rtv, old_dsv);
    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(g_svs.mask_w);
    vp.Height = static_cast<float>(g_svs.mask_h);
    // Use the latched priming range: reading g_ro.trig_vp live here puts the priming pass in a different window-z space
    // than the depth it tests against.
    vp.MinDepth = g_svs.prime_vp_lo;
    vp.MaxDepth = g_svs.prime_vp_hi;
    ctx->RSSetViewports(1, &vp);

    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskprime, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    if (g_res.rast_sun) ctx->RSSetState(g_res.rast_sun);
    ctx->OMSetDepthStencilState(g_mask.prime_dss, 0);
    const FLOAT bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.prime_blend, bf, 0xFFFFFFFF);

    ID3D11Buffer* cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, cbs);
    ctx->PSSetConstantBuffers(0, 2, cbs);

    ConstantData cbf = {};
    memcpy(cbf.view_proj, g_svs.prime_vp, sizeof(cbf.view_proj));

    // Drain the slot about to be reused - three frames old, so donotflush never stalls.
    const int slot = g_mask.prime_occl_cursor;
    bool q = !g_mask.prime_occl_failed && g_mask.prime_occl[slot] != nullptr;

    if (q && g_mask.prime_occl_inflight[slot]) {
        UINT64 px = 0;

        if (ctx->GetData(g_mask.prime_occl[slot], &px, sizeof(px),
                         D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK) {
            g_mask.prime_occl_inflight[slot] = false;
        } else {
            q = false;
        }
    }

    if (upload_frame_cb(ctx, g_res.composite_frame_cb, cbf)) {
        UINT stride = sizeof(MeshVertex);
        VbRef bound_vb;   // CLOTH; SKIN_POOL: the pair.
        if (q) ctx->Begin(g_mask.prime_occl[slot]);

        for (const auto& c : g_svs.caster_list) {
            const VbRef want = (c.mesh >= 0 &&
                static_cast<size_t>(c.mesh) < g_res.mesh_vb.size())
                ? mesh_vb_ref(c.mesh, c.slot) : VbRef();   // CLOTH.
            if (!want) {
                continue;
            }

            if (want != bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &want.vb, &stride, &want.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                bound_vb = want;
            }

            ConstantData obj = {};
            obj.center_size[0] = c.pos[0];
            obj.center_size[1] = c.pos[2];
            obj.center_size[2] = c.pos[1];
            obj.size_axes[0] = c.size[0];
            obj.size_axes[1] = c.size[2];
            obj.size_axes[2] = c.size[1];
            fill_obj_rot(obj, c.rot);
            if (g_svs.prime_rebase) fill_center_rel(obj, g_svs.prime_cam, c.res);   // POS_RES.

            if (!upload_obj_cb(ctx, g_res.composite_cb, obj)) {
                break;
            }

            {   // SHADOW_LOD_DRAWN.
                UINT is = 0, ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, is, ic);
                ctx->DrawIndexed(ic, is, 0);
            }
        }

        if (q) {
            ctx->End(g_mask.prime_occl[slot]);
            g_mask.prime_occl_inflight[slot] = true;
            g_mask.prime_occl_cursor = (slot + 1) % 3;
        }

    }

    om.restore(ctx);
    if (nvp > 0) ctx->RSSetViewports(nvp, old_vps);
    bk.restore(ctx);
    om.release();
    g_ro.in_injection = prev;
}

inline void svs_vol_copy(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(PROF_SVS_VOL_COPY);
    if (g_svs.vol_seq == g_svs.frame_seq) {
        return;
    }
    // PROF_ONCE: past the return above (this seam frame's copy is taken; the seam frame is the GPU zone's cycle -
    // both turn at the main-depth clear), as dls_frame's: a later call's empty return no longer replaces it.
    KH_GPU_SCOPE(ctx, GPU_SVS_VOL_COPY);

    if (!ctx || g_ro.in_injection) {  return; }
    if (!svs_vol_on()) {  return; }
    if (!g_svs.vol_src) {  return; }
    // No visible mesh, no reader: the copy is refused and the primed flag dropped, so svs_vol_ready cannot hand a
    // mesh that appears next frame a copy from before it existed.
    if (!g_svs_mesh_wanted.load(std::memory_order_relaxed) || shadows_off()) {   // SHADOW_OFF: no reader.
        g_svs.vol_primed = false;
        return;
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool ok = dev && svs_vol_ensure(dev);
    if (dev) dev->Release();
    if (!ok) {  return; }

    // The bracket state is not touched here: svs_bracket_post consumes it.
    // OM_SAVE_VOL: the engine's full OM set - render targets, depth and the unordered-access views behind them -
    // saved and put back through OmSave, as every target swap is (a plain get / set pair dropped the UAVs of the
    // engine draw this runs ahead of). PRED_ASIDE: a predicate the engine left bound at this draw would skip the
    // copy, and the seam and replay would read the last frame's counts (g_svs.vol_seq is stamped below regardless).
    svs_copy_aside(ctx, g_svs.vol_tex, g_svs.vol_src);   // EA_VOL_COPY.
    g_svs.vol_primed = true;
    // The epoch this copy belongs to.
    g_svs.vol_seq = g_svs.frame_seq;
    // VOL_WITNESS: the encode of this seam frame's footprint, or none.
    g_svs.vol_enc_ok = g_svs.seam_enc_seq == g_svs.frame_seq &&
                       g_svs.seam_enc[3] > g_svs.seam_enc[2] && g_svs.seam_enc[1] != 0.0f;
    memcpy(g_svs.vol_enc, g_svs.seam_enc, sizeof(g_svs.vol_enc));
    // VOL_ZOOM: that seam frame's projection, or none (identity).
    g_svs.vol_proj_ok = g_svs.seam_enc_seq == g_svs.frame_seq && g_svs.seam_proj_ok;
    memcpy(g_svs.vol_proj, g_svs.seam_proj, sizeof(g_svs.vol_proj));
    // VOL_FOOT: the mask this seam frame drew is this copy's; otherwise no mask (the witness then decodes the depth
    // plane). Same pixel grid as the copy, too: the witness addresses both with one pixel (a resize can land the seam's
    // mask and the copy on different sizes for a frame).
    g_svs.vol_foot_ok = g_svs.foot_seam_seq == g_svs.frame_seq && g_svs.foot_seam_ix >= 0 &&
                        g_svs.foot_seam_ix <= 1 && g_svs.foot_srv[g_svs.foot_seam_ix] != nullptr &&
                        g_svs.foot_w == g_svs.vol_w && g_svs.foot_h == g_svs.vol_h;
    if (g_svs.vol_foot_ok) g_svs.foot_pub = g_svs.foot_seam_ix;
}

inline void volume_seam_pump(ID3D11DeviceContext* ctx) {
    if (g_ro.in_injection) return;

    // The copy is pumped separately from the injection: a mask hunt gated behind the injection arming never finds it.
    if (g_svs.bracket == 1) {
        g_svs.bracket = 2;

        // The volume copy first and on its own arm: same position requirement as pre (after the counting draws, before
        // the resolve), separate demand gate, separate lanes.
        rp_pass_end(ctx);   // VOL_REPLAY: the rewritable buffers, before anything rewrites them.
        svs_vol_copy(ctx);
        // The bracket is the stencil resolve, a 4-vertex fullscreen quad with no view matrix to read.

        if (svs_snap_wanted()) {
            // Prime first, then snapshot: the only point in the frame that is after the engine's mask clear and all
            // eight cascade resolves, and before pre is read.
            if (svs_prime_wanted()) svs_prime_mask(ctx);
            svs_mask_snap(ctx, false);
        }
    }

    if (!g_svs.pending) return;
    g_svs.pending = false;
    volume_seam_inject(ctx, g_svs.pend_w, g_svs.pend_h);
}
}