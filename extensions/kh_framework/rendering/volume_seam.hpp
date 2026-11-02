#pragma once

// The injection half (rendering_integration_injection.hpp): the volume seam: the camera take, kh_volume_seam_inject,
// its tracking, the prime mask and the pump. Never include it on its own.
namespace RenderIntegration {

inline void kh_rp_foreign(int khrf_kind);                    // KH_VOL_REPLAY (defined with the replay).
inline void kh_rp_snap_engine(ID3D11DeviceContext* ctx);     // KH_VOL_REPLAY (defined with the replay).
inline bool kh_vmir_wanted(const float khvw_cam[3], const float khvw_proj[4][4]);   // KH_MIR_GATE: defined in
                                                                                     // rendering/replay.hpp.

// Defined in rendering/depth_identity.hpp (declared here for the uses below).
inline void* reorder_dsv_identity(ID3D11DepthStencilView* dsv);
// Defined in rendering/replay.hpp (declared here for the uses below).
inline void kh_rp_pass_end(ID3D11DeviceContext* ctx);


// The seam's camera take: the engine's camera projection for this frame, pre-clear, where no latch of this frame exists
// yet - the live bridge sample reconciled against the latch and the encode-pair witnesses. Shared verbatim with the
// view-model seam (kh_infront_seam_inject), which calls it only when the world seam did not run this window: the step
// trackers it advances are per window.
inline bool kh_svs_take_pv(ID3D11DeviceContext* ctx, RVExtBridge::ProjectionViewTransform& khv_pv) {
    bool khv_have = false;

    if (g_ro.cycle_pv_valid) { khv_pv = g_ro.cycle_pv; khv_have = true; }
    float khv_live_near = -1.0f;
    float khv_latch_cam[3] = {};
    bool  khv_latch_cam_ok = false;
    if (g_ro.cycle_pv_valid) {
        kh_cam_of_site(g_ro.cycle_pv.view, khv_latch_cam, 2);
        khv_latch_cam_ok = (khv_latch_cam[0] == khv_latch_cam[0]) &&
                           (khv_latch_cam[1] == khv_latch_cam[1]) &&
                           (khv_latch_cam[2] == khv_latch_cam[2]);
    }
    else if (RVExtBridge::get_projection_view_transform(khv_pv)) { khv_have = true; }

    if (!khv_have) {  return false; }

    // Adopt the frame view's rotation while it is frame-fresh (kh_adopt_frame_view); otherwise the taken view's own
    // stands (the latch's, else the live fetch's).
    kh_adopt_frame_view(khv_pv);

    // The depth pair (m22, m32) the footprint actually rasterises with, per frame: the measured engine pair here, then
    // the live take's arbitration and the CB census below, each overriding where it passes (a later one ranks above an
    // earlier one because it runs after it).

    if (g_ro.engine_proj_valid) {
        khv_pv.projection[2][2] = g_ro.engine_m22;
        khv_pv.projection[3][2] = g_ro.engine_m32;
    }

    RVExtBridge::ProjectionViewTransform khlt_live = {};

    if (RVExtBridge::get_projection_view_transform(khlt_live)) {
        float khlt_ac[3];
        float khlt_lc[3];   // The translation anchor the take builds on.
        float khlt_vc[3];   // The camera the bridge holds at this instant.
        // The honest inverse on all three.
        kh_cam_of_site(khv_pv.view, khlt_ac, 2);
        kh_cam_of_site(khlt_live.view, khlt_vc, 2);

        if (khv_latch_cam_ok) {
            khlt_lc[0] = khv_latch_cam[0];
            khlt_lc[1] = khv_latch_cam[1];
            khlt_lc[2] = khv_latch_cam[2];
        } else {
            khlt_lc[0] = khlt_ac[0];   // No latch: absolute form.
            khlt_lc[1] = khlt_ac[1];
            khlt_lc[2] = khlt_ac[2];
        }
        float khlt_cc[3];   // The candidate camera the take arbitrates.

        if (g_khlt_lp_ok) {
            khlt_cc[0] = khlt_lc[0] + (khlt_vc[0] - g_khlt_lp[0]);
            khlt_cc[1] = khlt_lc[1] + (khlt_vc[1] - g_khlt_lp[1]);
            khlt_cc[2] = khlt_lc[2] + (khlt_vc[2] - g_khlt_lp[2]);
        } else {
            khlt_cc[0] = khlt_vc[0];   // Cold: absolute form.
            khlt_cc[1] = khlt_vc[1];
            khlt_cc[2] = khlt_vc[2];
        }

        g_khlt_lp[0] = khlt_vc[0];   // The live feed's own trajectory.
        g_khlt_lp[1] = khlt_vc[1];   // Updated on every successful fetch.
        g_khlt_lp[2] = khlt_vc[2];   // Regardless of the take's verdict.
        g_khlt_lp_ok = true;

        const float khlt_dx = khlt_cc[0] - khlt_lc[0];
        const float khlt_dy = khlt_cc[1] - khlt_lc[1];
        const float khlt_dz = khlt_cc[2] - khlt_lc[2];
        float khlt_d = sqrtf(khlt_dx * khlt_dx + khlt_dy * khlt_dy +
                                   khlt_dz * khlt_dz);

        if (khlt_d <= KH_LATCH_JUMP_M) {
            // A foreign one-frame flicker never confirms.
            bool khlt_take = true;
            {
                float khlt_sm = 0.0f;

                if (g_khlt_prev_ok) {
                    const float khlt_sx = khlt_lc[0] - g_khlt_prev[0];
                    const float khlt_sy = khlt_lc[1] - g_khlt_prev[1];
                    const float khlt_sz = khlt_lc[2] - g_khlt_prev[2];
                    khlt_sm = sqrtf(khlt_sx * khlt_sx +
                                    khlt_sy * khlt_sy + khlt_sz * khlt_sz);

                    float khlt_recent = 0.0f;

                    for (int khlt_k = 0; khlt_k < 4; ++khlt_k) {
                        if (g_khlt_step_ring[khlt_k] > khlt_recent) {
                            khlt_recent = g_khlt_step_ring[khlt_k];
                        }
                    }

                    const float khlt_ref0 = khlt_sm > g_khlt_prev_step
                        ? khlt_sm : g_khlt_prev_step;
                    const float khlt_ref = khlt_recent > khlt_ref0 ? khlt_recent : khlt_ref0;
                    const float khlt_mul = 100.0f;
                    const float khlt_stl = 0.005f;
                    const bool  khlt_still = khlt_ref < khlt_stl;
                    const float khlt_ratio_b = khlt_mul * khlt_ref;
                    const float khlt_bnd = khlt_still ? 0.30f : (khlt_ratio_b > 0.30f ? khlt_ratio_b : 0.30f);

                    if (khlt_d > khlt_bnd) {
                        khlt_take = false;

                        if (g_khlt_ref_ok) {
                            const float khlt_cx = khlt_cc[0] - g_khlt_ref_cam[0];
                            const float khlt_cy = khlt_cc[1] - g_khlt_ref_cam[1];
                            const float khlt_cz = khlt_cc[2] - g_khlt_ref_cam[2];
                            const float khlt_cd = sqrtf(
                                khlt_cx * khlt_cx + khlt_cy * khlt_cy +
                                khlt_cz * khlt_cz);
                            const float khlt_ctol = 0.5f * khlt_d > 0.5f
                                                  ? 0.5f * khlt_d : 0.5f;

                            if (khlt_cd <= khlt_ctol) {
                                khlt_take = true;
                            }
                        }

                        if (!khlt_take && khlt_still) {
                            float khwg_cn = -1.0f;

                            if (kh_cbc_on()) {
                                ID3D11Buffer* khwg_vs[4] = {};
                                ID3D11Buffer* khwg_ps[4] = {};
                                ctx->VSGetConstantBuffers(0, 4, khwg_vs);
                                ctx->PSGetConstantBuffers(0, 4, khwg_ps);
                                const KhCbCensusEntry* khwg_best = nullptr;

                                for (int khwg_i = 0; khwg_i < 8; ++khwg_i) {
                                    ID3D11Buffer* khwg_b = khwg_i < 4
                                        ? khwg_vs[khwg_i] : khwg_ps[khwg_i - 4];
                                    if (!khwg_b) continue;
                                    const KhCbCensusEntry* khwg_e = kh_cbc_find(khwg_b);
                                    if (khwg_e && (!khwg_best ||
                                                   khwg_e->ms > khwg_best->ms)) {
                                        khwg_best = khwg_e;
                                    }
                                }

                                for (int khwg_i = 0; khwg_i < 4; ++khwg_i) {
                                    KH_SAFE_RELEASE(khwg_vs[khwg_i]);
                                    KH_SAFE_RELEASE(khwg_ps[khwg_i]);
                                }

                                if (khwg_best &&
                                    fabsf(khwg_best->m22) > 1.0e-9f) {
                                    khwg_cn = -khwg_best->m32 / khwg_best->m22;
                                }
                            }

                            const float khwg_fn = kh_enc_pair_near(
                                khlt_live.projection[2][2],
                                khlt_live.projection[3][2]);
                            const float khwg_sn = (g_ro.engine_proj_valid &&
                                fabsf(g_ro.engine_m22) > 1.0e-9f)
                                ? -g_ro.engine_m32 / g_ro.engine_m22 : -1.0f;

                            if (khwg_cn > 0.0f && khwg_sn > 0.0f &&
                                khwg_fn == khwg_fn && khwg_fn > 0.0f) {
                                const bool khwg_cf =
                                    fabsf(khwg_cn - khwg_fn) <= 0.10f * khwg_fn;
                                const bool khwg_cs =
                                    fabsf(khwg_cn - khwg_sn) <= 0.10f * khwg_sn;

                                if (khwg_cf && khwg_cs) {
                                    khlt_take = true;
                                    g_khlt_ref_ok = false;   // A take is a take.
                                }
                            }
                        }

                        if (!khlt_take) {
                            g_khlt_ref_cam[0] = khlt_cc[0];   // The refused candidate.
                            g_khlt_ref_cam[1] = khlt_cc[1];
                            g_khlt_ref_cam[2] = khlt_cc[2];
                            g_khlt_ref_ok = true;
                        }
                    } else {
                        g_khlt_ref_ok = false;
                    }
                }

                g_khlt_prev[0] = khlt_lc[0];
                g_khlt_prev[1] = khlt_lc[1];
                g_khlt_prev[2] = khlt_lc[2];
                g_khlt_prev_step = khlt_sm;
                g_khlt_prev_ok = true;
                // The applied-step ring, same unconditional bookkeeping as prev_step - refusal streaks and genuine rest
                // both write zeros.
                g_khlt_step_ring[g_khlt_step_ring_i & 3u] = khlt_sm;
                g_khlt_step_ring_i++;
            }

            const float khlt_ud = khlt_take ? khlt_d : 0.0f;
            bool khlr_basis = false;

            bool khlr_sane = true;

            for (int khlr_r = 0; khlr_r < 3 && khlr_sane; ++khlr_r) {
                const float khlr_n =
                    khlt_live.view[khlr_r][0] * khlt_live.view[khlr_r][0] +
                    khlt_live.view[khlr_r][1] * khlt_live.view[khlr_r][1] +
                    khlt_live.view[khlr_r][2] * khlt_live.view[khlr_r][2];
                if (!(khlr_n > 0.81f && khlr_n < 1.21f)) khlr_sane = false;
            }

            if (khlr_sane) {
                for (int khlr_r = 0; khlr_r < 3; ++khlr_r) {
                    khv_pv.view[khlr_r][0] = khlt_live.view[khlr_r][0];
                    khv_pv.view[khlr_r][1] = khlt_live.view[khlr_r][1];
                    khv_pv.view[khlr_r][2] = khlt_live.view[khlr_r][2];
                    khv_pv.view[khlr_r][3] = 0.0f;
                }
                khlr_basis = true;
            }

            // Rebuild whenever the basis moved or the camera did, from the
            // bounded camera through the current basis, double precision.
            if (khlr_basis || khlt_ud > 1.0e-6f) {
                const double khlt_cam[3] = {
                    static_cast<double>(khlt_take ? khlt_cc[0] : khlt_lc[0]),
                    static_cast<double>(khlt_take ? khlt_cc[1] : khlt_lc[1]),
                    static_cast<double>(khlt_take ? khlt_cc[2] : khlt_lc[2]) };

                for (int khlt_c = 0; khlt_c < 3; ++khlt_c) {
                    khv_pv.view[3][khlt_c] = static_cast<float>(
                        -(khlt_cam[0] * khv_pv.view[0][khlt_c] +
                          khlt_cam[1] * khv_pv.view[1][khlt_c] +
                          khlt_cam[2] * khv_pv.view[2][khlt_c]));
                }

                khv_pv.view[3][3] = 1.0f;
            }

            const float khlf_sx = khlt_live.projection[0][0];
            const float khlf_sy = khlt_live.projection[1][1];
            const float khlf_lx = khv_pv.projection[0][0];
            const float khlf_ly = khv_pv.projection[1][1];
            float khlf_r = -1.0f;
            if (khlf_sx > 1.0e-6f && khlf_sy > 1.0e-6f &&
                khlf_lx > 1.0e-6f && khlf_ly > 1.0e-6f) {
                const float khlf_rx = khlf_sx > khlf_lx ? khlf_sx / khlf_lx
                                                        : khlf_lx / khlf_sx;
                const float khlf_ry = khlf_sy > khlf_ly ? khlf_sy / khlf_ly
                                                        : khlf_ly / khlf_sy;
                khlf_r = khlf_rx > khlf_ry ? khlf_rx : khlf_ry;
            }

            const bool khlf_sane =
                khlf_sx == khlf_sx && khlf_sy == khlf_sy &&
                khlf_r > 0.0f && khlf_r < 1.30f;

            if (khlf_sane) {
                khv_pv.projection[0][0] = khlf_sx;
                khv_pv.projection[1][1] = khlf_sy;
            }

            // Mid-frame publications carry other windows' projections; the colour pass filters them, and an ungated
            // take here feeds them straight into the footprint's depth encode.
            const float khlp_n = kh_enc_pair_near(
                khlt_live.projection[2][2], khlt_live.projection[3][2]);
            const bool khlp_sane =
                khlp_n == khlp_n && khlp_n > 0.0f &&
                fabsf(khlt_live.projection[2][2]) > 1.0e-9f &&
                kh_enc_far(khlt_live.projection[2][2],
                           khlt_live.projection[3][2]) <= KH_ENC_FAR_MAX;
            if (khlp_sane) khv_live_near = khlp_n;

            const bool khlp_comm_ok =
                g_inj_enc_near > 0.0f && g_inj_enc_ms != 0 &&
                steady_now_ms() - g_inj_enc_ms < 250;
            const float khlp_sn = (g_ro.engine_proj_valid &&
                                   fabsf(g_ro.engine_m22) > 1.0e-9f)
                ? kh_enc_pair_near(g_ro.engine_m22, g_ro.engine_m32)
                : -1.0f;
            const bool khlp_sniff_ok =
                khlp_sn == khlp_sn && khlp_sn > 0.0f &&
                kh_enc_far(g_ro.engine_m22, g_ro.engine_m32) <= KH_ENC_FAR_MAX;

            if (khlp_comm_ok && g_inj_enc_ms != g_khpe_ms) {
                g_khpe_ms = g_inj_enc_ms;
                const float khpe_c = g_inj_enc_near;

                if (g_khbl_prev > 0.0f &&
                    fabsf(khpe_c - g_khbl_prev) <= 0.02f * g_khbl_prev) {
                    g_khbl_stable = khpe_c;
                }
                g_khbl_prev = khpe_c;

                const bool khpe_spike =
                    g_khpe_p1 > 0.0f &&
                    (khpe_c > 2.0f * g_khpe_p1 ||
                     khpe_c < 0.5f * g_khpe_p1);

                if (!khpe_spike) {
                    g_khpe_p2 = g_khpe_p1;
                    g_khpe_p1 = khpe_c;
                    g_khpe_m22 = g_inj_enc_m22;
                    g_khpe_upd_ms = steady_now_ms();
                    g_khpe_skip1 = -1.0f;
                } else if (g_khpe_skip1 > 0.0f && khpe_c <= 2.0f * g_khpe_skip1 && khpe_c >= 0.5f * g_khpe_skip1) {
                    g_khpe_p2 = -1.0f;
                    g_khpe_p1 = khpe_c;
                    g_khpe_m22 = g_inj_enc_m22;
                    g_khpe_upd_ms = steady_now_ms();
                    g_khpe_skip1 = -1.0f;
                } else {
                    g_khpe_skip1 = khpe_c;
                }   // A lone spiking commit is skipped: base and slope hold.
            }

            float khco_ref = g_inj_enc_near;

            if (g_khpe_p1 > 0.0f && khco_ref > 0.0f && (khco_ref > 2.0f * g_khpe_p1 ||
                 khco_ref < 0.5f * g_khpe_p1)) {
                khco_ref = g_khpe_p1;
            }

            const bool khlp_corr = khlp_sane && khlp_comm_ok &&
                khlp_n >= 0.5f * khco_ref &&
                khlp_n <= 12.0f * khco_ref;
            const bool khlp_base = khlp_sane && khlp_comm_ok && !khlp_corr && g_khbl_stable > 0.0f && fabsf(khlp_n - g_khbl_stable) <= 0.10f * g_khbl_stable;

            if (khlp_corr || khlp_base) {
                khv_pv.projection[2][2] = khlt_live.projection[2][2];
                khv_pv.projection[3][2] = khlt_live.projection[3][2];
            } else if (g_khpe_p1 > 0.0f && fabsf(g_khpe_m22) > 1.0e-9f &&
                        steady_now_ms() - g_khpe_upd_ms < 500) {
                float khpe_vel = 0.0f;

                if (g_khpe_p2 > 0.0f) {
                    const bool khpe_smooth = g_khpe_p1 <= 1.25f * g_khpe_p2 && g_khpe_p1 >= 0.8f * g_khpe_p2;
                    if (khpe_smooth) khpe_vel = g_khpe_p1 - g_khpe_p2;
                }

                float khpe_n = g_khpe_p1 + khpe_vel;
                if (khpe_n > 2.0f * g_khpe_p1) khpe_n = 2.0f * g_khpe_p1;
                if (khpe_n < 0.5f * g_khpe_p1) khpe_n = 0.5f * g_khpe_p1;
                khv_pv.projection[2][2] = g_khpe_m22;
                khv_pv.projection[3][2] = -khpe_n * g_khpe_m22;
            } else if (khlp_sniff_ok) {
                khv_pv.projection[2][2] = g_ro.engine_m22;
                khv_pv.projection[3][2] = g_ro.engine_m32;
            }
        }
    }

    // The engine uploads frame k+1's projection into the bound b2 before our pre-clear seam runs; identity reads it
    // where value arbitration structurally cannot.
    if (kh_cbc_on()) {
        ID3D11Buffer* khcb_vs[4] = {};
        ID3D11Buffer* khcb_ps[4] = {};
        ctx->VSGetConstantBuffers(0, 4, khcb_vs);
        ctx->PSGetConstantBuffers(0, 4, khcb_ps);

        const KhCbCensusEntry* khcb_best = nullptr;

        for (int khcb_i = 0; khcb_i < 8; ++khcb_i) {
            ID3D11Buffer* khcb_b =
                khcb_i < 4 ? khcb_vs[khcb_i] : khcb_ps[khcb_i - 4];
            if (!khcb_b) continue;
            const KhCbCensusEntry* khcb_e = kh_cbc_find(khcb_b);

            if (khcb_e) {
                if (!khcb_best || khcb_e->ms > khcb_best->ms) khcb_best = khcb_e;
            }
        }

        for (int khcb_i = 0; khcb_i < 4; ++khcb_i) {
            KH_SAFE_RELEASE(khcb_vs[khcb_i]);
            KH_SAFE_RELEASE(khcb_ps[khcb_i]);
        }

        if (khcb_best) {
            const float khcb_near = fabsf(khcb_best->m22) > 1.0e-9f
                ? -khcb_best->m32 / khcb_best->m22 : -1.0f;

            const float khcb_age = static_cast<float>(
                steady_now_ms() - khcb_best->ms);

            // Age-bounded so a stalled entry can never encode stale; a cold gate must not block the first encode.
            bool khcb_class_ok = true;

            if (khcb_near > 0.0f) {
                bool khcb_any_wit = false;
                bool khcb_agree = false;

                if (khv_live_near > 0.0f) {
                    khcb_any_wit = true;
                    if (khcb_near >= 0.5f * khv_live_near &&
                        khcb_near <= 12.0f * khv_live_near) khcb_agree = true;
                }

                if (!khcb_agree && g_inj_enc_near > 0.0f && g_inj_enc_ms != 0 &&
                    steady_now_ms() - g_inj_enc_ms < 250) {
                    khcb_any_wit = true;
                    if (khcb_near >= 0.5f * g_inj_enc_near &&
                        khcb_near <= 12.0f * g_inj_enc_near) khcb_agree = true;
                }

                // A pulse return agrees with the stable baseline by definition; the zoom poison sits at 0.14x of it and
                // fails the floor. Same 10 percent agreement bar the chain's baseline acceptance uses.
                if (!khcb_agree && g_khbl_stable > 0.0f) {
                    khcb_any_wit = true;
                    if (fabsf(khcb_near - g_khbl_stable) <=
                        0.10f * g_khbl_stable) khcb_agree = true;
                }

                if (khcb_any_wit && !khcb_agree) {
                    khcb_class_ok = false;
                }
            }

            if (khcb_class_ok && khcb_near > 0.0f && khcb_age <= 100.0f && kh_enc_far(khcb_best->m22, khcb_best->m32) <= KH_ENC_FAR_MAX) {
                khv_pv.projection[2][2] = khcb_best->m22;
                khv_pv.projection[3][2] = khcb_best->m32;
            }
        }
    }
    return true;
}
inline void kh_volume_seam_inject(ID3D11DeviceContext* ctx, uint32_t khv_w, uint32_t khv_h) {
    if (!ctx) return;
    kh_rp_foreign(2);   // KH_VOL_REPLAY: our footprint written into a pass already being recorded.

    std::vector<KhSvCaster>& khv_list = g_svs_caster_list;
    khv_list.clear();
    // KH_INFRONT: the view-model casters, drawn per caster into the world count and the mirror prepass below (never
    // into the instanced footprint or the KH_VOL_FOOT mask).
    static std::vector<KhSvCaster> khv_front;
    khv_front.clear();

    {
        // The consume point for THIS pass: the footprint lands under the mesh, not one frame behind it. The engine
        // counts its stencil volumes against the footprint, so one drawn at a stale transform hands our mesh a stencil
        // computed for where it WAS - registered against the camera and every engine caster, wrong only about us. Same
        // rule as the injection and the cast fire; this pass reads the scene to build draws exactly as they do.
        {
            // KH_SEAM_SKIN: with the pass's context, so the footprint is this frame's SHAPE as well as its place -
            // the skeletal skin, a cloth's pinned part and a chain's end (KH_CHAIN_END_DRAW) are written only by a
            // step with a context - and the vertex stages over them, as the cast fire and the injection pair them.
            // Our own buffer writes, under the flag: the hooks stand aside.
            KhInjectionFlagScope khv_flag;
            g_ro.in_injection = true;
            kh_attach_step(ctx);
            kh_uvs_step(ctx);
        }
        kh_scene_sync();   // KH_SCENE: live walk by reference, no copy.
        for (uint32_t khsc_i = 0; khsc_i < g_scene.objs.size(); ++khsc_i) {
            if (!g_scene.alive[khsc_i]) continue;
            const RenderObject& o = g_scene.objs[khsc_i];
            // The footprint serves our receivers' stencil term: visible, lit,
            // world-space, depth-participating geometry only.
            // KH_SHADOW_SWITCH: participation, not casting - a mesh that casts nothing still owns its pixels here and
            // may still receive. (The DLS world pass does not read this footprint: it keeps its own mask,
            // kh_dlsw_mask_render - KH_DLSW_OWN.) KH_SHOW_INFRONT: drawn.
            if (!kh_obj_shown(o) || !kh_shadow_part(o)) continue;
            KhSvCaster c;
            c.slot = khsc_i;   // KH_SEAM_INST: khObjs index for the instanced twins.
            memcpy(c.pos, o.pos, sizeof(c.pos));
            kh_pos_res(o, c.res);   // KH_POS_RES.
            memcpy(c.size, o.size, sizeof(c.size));
            kh_bounds_size_of(o, c.bsize);   // KH_CAST_REFIT.
            memcpy(c.rot, o.rot_m, sizeof(c.rot));
            c.rotated = o.rotated;
            c.mesh = mesh_id_clamp(o.mesh);
            c.lod = 0;   // KH_SHADOW_LOD_DRAWN: picked below, once khv_cam exists.
            c.lod_lock = kh_lod_locked(o);   // KH_LOD_SIM_SLOT.
            c.lod_r = kh_lod_radius(o);   // KH_SKEL_LOD_R.
            c.a_vis = true;   // KH_SEAM_CULL: decided below, once the pass matrix is final.
            {   // KH_FOOTPRINT_ALPHA (the sun census's recipe).
                bool khsv_exp = false;
                const float khsv_env = lifetime_envelope(o, effect_time_seconds(), khsv_exp);
                float khsv_a = khsv_exp ? 0.0f : o.color[3] * khsv_env;
                khsv_a = khsv_a < 0.0f ? 0.0f : (khsv_a > 1.0f ? 1.0f : khsv_a);
                c.alpha = floorf(khsv_a * 64.0f + 0.5f) / 64.0f;
                c.mats = kh_mat_set_has_alpha(kh_obj_textured(o)) ? o.materials : nullptr;
                c.alpha_caster = kh_footprint_alpha_on() && (c.alpha < 0.999f || c.mats);
            }
            c.no_foot = o.blend_mode != 0;   // KH_FOOT_BLEND.
            c.two_sided = o.two_sided;   // KH_SEAM_FACE.
            if (o.in_front) {   // KH_INFRONT: its own list; whole-object translucents write no depth.
                if (!(c.alpha_caster && c.alpha < 0.999f)) khv_front.push_back(c);
                continue;
            }
            khv_list.push_back(c);
        }
    }

    if (khv_list.empty() && khv_front.empty()) {  return; }
    if (!g_rp_seam_b) kh_rp_snap_engine(ctx);   // KH_VOL_REPLAY: the engine's own depth, before any footprint of ours.

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);
    const bool khv_res_ok = dev && kh_ensure_ok("seam resources", ensure_resources(dev));
    if (dev) dev->Release();
    if (!khv_res_ok || !g_res.input_layout || !g_res.vs ||
        !g_res.composite_cb || !g_res.composite_frame_cb || !g_res.dss_test_write) {
        return;
    }

    // The engine's camera projection for this frame.
    RVExtBridge::ProjectionViewTransform khv_pv = {};
    if (g_rp_seam_b) {   // KH_VOL_REPLAY: at the injection this frame's latch is the pass's camera; no tracker advances.
        if (!g_ro.cycle_pv_valid) {  return; }
        khv_pv = g_ro.cycle_pv;
        // KH_FX_FRAME_ADOPT: the injection's own frame (it adopts this latch), so the footprint lies where the
        // meshes that read it (t24 / t33) are drawn, and on the engine's picture.
        kh_adopt_frame_view(khv_pv);
        if (g_ro.engine_proj_valid) {
            khv_pv.projection[2][2] = g_ro.engine_m22;
            khv_pv.projection[3][2] = g_ro.engine_m32;
        }
        kh_rp_rast_apply(khv_pv.projection);   // KH_REPLAY_RASTER: the copy's raster when the latch's parts from it.
    } else if (!kh_svs_take_pv(ctx, khv_pv)) {  return; }

    float khv_vp_m[4][4] = {};
    mul_4x4(khv_pv.view, khv_pv.projection, khv_vp_m);
    // The priming pass runs later in the frame and must rasterize with this
    // exact transform, or its depth test rejects our own footprint.
    memcpy(g_svs_prime_vp, khv_vp_m, sizeof(g_svs_prime_vp));

    const bool khv_prev_inj = g_ro.in_injection;
    g_ro.in_injection = true;

    StateBackup backup;
    backup.capture(ctx);
    KhBindScope khv_bc(ctx);   // KH_BIND_FILTER.
    KhOmSave khv_om;   // Full OM set (8 RTVs + DSV); the DSV is reused below.
    khv_om.capture(ctx);
    ID3D11DepthStencilView* const khv_old_dsv = khv_om.dsv;
    UINT khv_old_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khv_old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&khv_old_nvp, khv_old_vps);

    // Depth target only - the engine's own, exactly as it bound it. Dropping the render target keeps the paired
    // linear-depth colour buffer (which the cascade resolves read) out of this entirely.
    const bool khv_withhold = kh_rp_seam_withhold();   // KH_SEAM_WITHHOLD: no depth target - the footprint draws go nowhere.
    if (khv_withhold) g_rp_withheld_seq = g_svs_frame_seq;
    ctx->OMSetRenderTargets(0, nullptr, khv_withhold ? nullptr : khv_old_dsv);

    float khv_vp_lo = g_ro.trig_vp_valid ? g_ro.trig_vp_min : g_scene_vp_min_d;
    float khv_vp_hi = g_ro.trig_vp_valid ? g_ro.trig_vp_max : g_scene_vp_max_d;

    if (!(khv_vp_lo >= 0.0f && khv_vp_hi <= 1.0f && khv_vp_lo < khv_vp_hi)) {
        khv_vp_lo = g_scene_vp_min_d;
        khv_vp_hi = g_scene_vp_max_d;
    }

    D3D11_VIEWPORT khv_vp = {};
    khv_vp.TopLeftX = 0.0f;
    khv_vp.TopLeftY = 0.0f;
    khv_vp.Width = static_cast<float>(khv_w);
    khv_vp.Height = static_cast<float>(khv_h);
    khv_vp.MinDepth = khv_vp_lo;
    khv_vp.MaxDepth = khv_vp_hi;
    ctx->RSSetViewports(1, &khv_vp);
    // KH_VOL_WITNESS: the depth-plane witness's encode (the fallback when the copy has no KH_VOL_FOOT mask), for the
    // copy this seam frame ends in. It is an estimate, not the writer's: the footprint's z is written through the
    // engine's own view block (KhVsCore's seam path, stenVol2.z = 3, remaps z by engBlk's pair), which the CPU never
    // sees - equal at rest, off while the near plane moves. KH_VOL_FOOT's mask makes the witness independent of it.
    kh_scene_depth_pair(khv_pv.projection, g_svs_seam_enc[0], g_svs_seam_enc[1]);
    g_svs_seam_enc[2] = khv_vp_lo;
    g_svs_seam_enc[3] = khv_vp_hi;
    g_svs_seam_enc_seq = g_svs_frame_seq;
    g_svs_seam_proj_ok = kh_sten_proj_terms(khv_pv.projection, g_svs_seam_proj);   // KH_VOL_ZOOM.

    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(nullptr, nullptr, 0);   // Depth-only: zero render targets.
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    ctx->OMSetDepthStencilState(g_res.dss_test_write, 0);
    const bool khv_clamp_want = g_res.rast_inject != nullptr;
    ID3D11RasterizerState* khv_rs = khv_clamp_want ? g_res.rast_inject : g_res.rast_sun;
    const bool khv_rs_have = khv_rs != nullptr;
    const bool khv_rs_own = khv_rs_have;
    if (khv_rs_own) ctx->RSSetState(khv_rs);
    // KH_SEAM_FACE: the colour passes draw a single-sided mesh's front faces only (o.two_sided ? rasterizer :
    // rasterizer_cull; a two-sided translucent's ordered pair draws both), so the footprint, its KH_VOL_FOOT mask and
    // the mirror prepass take the same faces per caster - khv_rs for a two-sided one, its CULL_BACK twin for a
    // single-sided one. g_res.rasterizer_cull is that twin: rasterizer and rast_inject are minted from one desc (solid,
    // no bias, no depth clip, ensure_resources), and the hands seam binds the pair for its footprint the same way.
    // Without rast_inject (the rast_sun fallback) every caster keeps khv_rs.
    ID3D11RasterizerState* const khv_rs_cull =
        (khv_rs_own && khv_rs == g_res.rast_inject && g_res.rasterizer_cull) ? g_res.rasterizer_cull : khv_rs;
    ID3D11RasterizerState* khv_rs_bound = khv_rs;
    auto khv_rs_face = [&](bool khrf_two) {
        ID3D11RasterizerState* const khrf_r = khrf_two ? khv_rs : khv_rs_cull;
        if (khv_rs_own && khrf_r != khv_rs_bound) { ctx->RSSetState(khrf_r); khv_rs_bound = khrf_r; }
    };

    // Our passes never touch b2+, so the engine's binds there are never disturbed.
    ID3D11Buffer* khv_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khv_cbs);
    ctx->PSSetConstantBuffers(0, 2, khv_cbs);

    ConstantData khv_cbf = {};
    memcpy(khv_cbf.view_proj, khv_vp_m, sizeof(khv_cbf.view_proj));
    // The vertex-path selector: 3 = KhVsCore's seam path, the footprint's z written through the engine's own view block
    // (engBlk's pair - the witness note above). VSMirror does not read it (KhMirClip rebuilds its own near). The flush
    // mesh pass never sets it.
    khv_cbf.sten_vol2[2] = 3.0f;
    float khv_cam[3] = {};
    float khv_cam_rt = -1.0f;
    if (!kh_view_camera_exact(khv_pv.view, khv_cam, &khv_cam_rt)) {
        extract_camera_pos(khv_pv.view, khv_cam);
    }
    const bool khv_engcam = kh_engcam_consume(khv_pv.view, khv_cam);
    // KH_SHADOW_LOD_DRAWN: the level the injection draws at this camera. KH_SEAM_LOD_CAM: picked after the consume,
    // as the injection picks after its own, so both take the camera the consume settled on.
    for (KhSvCaster& khsl_c : khv_list) {
        khsl_c.lod = kh_drawn_lod(khsl_c.mesh, khsl_c.lod_r, khsl_c.pos, khsl_c.size, khsl_c.rot, khsl_c.rotated,
                                  khsl_c.lod_lock, khv_cam);
    }
    const bool khv_rebase_on = khv_engcam ? kh_rebase_vp_engcam(khv_cbf, khv_pv.view, khv_pv.projection)
                                          : kh_rebase_vp(khv_cbf, khv_cam);
    // KH_DDX_REBASE: the fold is kh_rebase_vp's at every site (the wrapper around it only reached its first line).
    // The rebase mutates view_proj in place, so cache the post-rebase matrix.
    memcpy(g_svs_prime_vp, khv_cbf.view_proj, sizeof(g_svs_prime_vp));
    {   // KH_SEAM_CULL: the footprint loop's visibility, through the final matrix.
        KhCullSet khsv_cull;
        kh_cull_build(khv_cbf.view_proj, khv_cam, khsv_cull);
        for (KhSvCaster& khsv_c : khv_list) {
            const float khsv_ctr[3] = { khsv_c.pos[0], khsv_c.pos[2], khsv_c.pos[1] };   // Engine axes.
            khsv_c.a_vis = kh_mesh_visible(khsv_cull, khsv_ctr,
                                           kh_lod_radius_of(khsv_c.bsize, khsv_c.rot, khsv_c.rotated));   // KH_CAST_REFIT.
        }
    }
    memcpy(g_svs_prime_cam, khv_cam, sizeof(g_svs_prime_cam));
    g_svs_prime_rebase = khv_rebase_on;
    // The priming pass's viewport depth range (kh_svs_prime_mask), published beside its transform; ungated.
    g_svs_prime_vp_lo = khv_vp_lo;
    g_svs_prime_vp_hi = khv_vp_hi;
    // The rebase arm for the RECORD path: the instanced entries read khPass, which nothing fills per mesh, so it is a
    // frame lane. Left zeroed, KhVsCore takes the absolute branch against a rebased viewProj and displaces every
    // instance by the camera vector. MUST STAY ABOVE kh_upload_frame_cb: the frame half is copied once per pass and the
    // mirror prepass re-uploads this same struct verbatim.
    khv_cbf.kh_pass[0] = khv_cam[0];
    khv_cbf.kh_pass[1] = khv_cam[1];
    khv_cbf.kh_pass[2] = khv_cam[2];
    khv_cbf.kh_pass[3] = khv_rebase_on ? 1.0f : 0.0f;
    const bool khv_frame_ok = kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khv_cbf);

    // KH_SVS_SKIP: the signature, both caster lists (world, then in-front) in list order.
    {
        uint64_t khsk_h = CryptoGenerator::FNV1A64_OFFSET;   // House FNV-1a 64 (see
                                                             // kh_shader_cache_hash).
        const uintptr_t khsk_dsv = reinterpret_cast<uintptr_t>(khv_old_dsv);
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_dsv, sizeof(khsk_dsv));
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &g_depth_clear_serial, sizeof(g_depth_clear_serial));
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khv_cbf.view_proj, sizeof(khv_cbf.view_proj));
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khv_rebase_on, sizeof(khv_rebase_on));
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khv_vp_lo, sizeof(khv_vp_lo));
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khv_vp_hi, sizeof(khv_vp_hi));
        // KH_SEAM_SIG_FULL: the in-front list draws in this pass too (the world count), so it is hashed.
        const uint64_t khsk_n[2] = { khv_list.size(), khv_front.size() };
        khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khsk_n, sizeof(khsk_n));
        bool khsk_tex_alpha = false;   // KH_TEX_LAND: a caster draws through its material's maps.
        bool khsk_uv_anim = false;   // KH_UV_ANIM: an alpha caster's coverage moves with the clock.
        for (const std::vector<KhSvCaster>* khsk_l : { &khv_list, &khv_front })
        for (const KhSvCaster& khsk_c : *khsk_l) {
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khsk_c.pos, sizeof(khsk_c.pos));
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khsk_c.res, sizeof(khsk_c.res));   // KH_POS_RES.
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khsk_c.size, sizeof(khsk_c.size));
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, khsk_c.rot, sizeof(khsk_c.rot));
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_c.mesh, sizeof(khsk_c.mesh));
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_c.lod, sizeof(khsk_c.lod));   // KH_SHADOW_LOD_DRAWN.
            const KhVbRef khsk_vr = kh_mesh_vb_ref(khsk_c.mesh, khsk_c.slot);
            const uintptr_t khsk_vb = reinterpret_cast<uintptr_t>(khsk_vr.vb);
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_vb, sizeof(khsk_vb));
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_vr.off, sizeof(khsk_vr.off));   // KH_SKIN_POOL.
            // KH_CLOTH: a cloth instance keeps one buffer for its whole life and rewrites it every frame, so the
            // pointer above says nothing about whether the geometry moved. Without this the footprint would be
            // rasterised once and then reused while the cloth swung.
            const uint32_t khsk_cg = kh_cloth_gen_of(khsk_c.slot);
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_cg, sizeof(khsk_cg));
            // KH_SEAM_SIG_FULL: these pick loop A or the alpha-tested route, and what the alpha one samples.
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_c.alpha, sizeof(khsk_c.alpha));
            const uint64_t khsk_mh = khsk_c.mats ? khsk_c.mats->hash : 0ull;
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_mh, sizeof(khsk_mh));
            const uint8_t khsk_fl = static_cast<uint8_t>((khsk_c.alpha_caster ? 1 : 0) | (khsk_c.no_foot ? 2 : 0) |
                                                         (khsk_c.two_sided ? 4 : 0));   // KH_SEAM_FACE.
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_fl, 1);
            khsk_tex_alpha |= khsk_c.alpha_caster && khsk_c.mats;
            khsk_uv_anim |= khsk_c.alpha_caster && khsk_c.mats && khsk_c.mats->uv_anim_alpha;   // KH_UV_ANIM.
        }
        if (khsk_tex_alpha) {   // KH_TEX_LAND.
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &g_tex_land_serial, sizeof(g_tex_land_serial));
        }
        if (khsk_uv_anim) {   // KH_UV_ANIM: the union hash's rule.
            const uint64_t khsk_ac = kh_mat_anim_cycle_key();
            khsk_h = CryptoGenerator::fnv1a64_update(khsk_h, &khsk_ac, sizeof(khsk_ac));
        }
        if (!g_rp_seam_b && g_svs_skip_valid && g_svs_skip_sig == khsk_h) {   // KH_VOL_REPLAY: replay B always draws.
            khv_om.restore(ctx);
            if (khv_old_nvp > 0) ctx->RSSetViewports(khv_old_nvp, khv_old_vps);
            backup.restore(ctx);
            khv_om.release();
            g_ro.in_injection = khv_prev_inj;
            return;
        }
        g_svs_skip_sig = khsk_h;
        g_svs_skip_valid = false;   // Proven again by the completed pass below.
    }

    // The instance plan for both loops, built once: loop A the opaque footprint casters (alpha ones need a material per
    // submesh and stay per object), loop B every caster. Both sorted mesh-major so a run of one mesh (level, face rule
    // - KH_SEAM_FACE) is one DrawIndexedInstanced. Safe: both loops are depth-only LESS_EQUAL with no blending, and the
    // skip signature was hashed in list order.
    bool     khv_inst_ok = false;
    uint32_t khv_inst_a = 0;   // Loop A's lane count.
    uint32_t khv_inst_b = 0;   // Loop B's lane count.
    {
        khv_inst_ok = g_res.vs_seam_inst && g_res.vs_mirror_inst &&
                      g_res.layout_sundepth && g_res.obj_srv && !khv_list.empty();

        if (khv_inst_ok) {
            // The records the instanced entries read at t39. kh_scene_sync ran at the scan above; this brings the GPU
            // copy up to it. The seam cannot rely on the sun ladder having synced first.
            khv_inst_ok = kh_objbuf_sync(ctx);
        }

        if (khv_inst_ok) {
            const uint32_t khv_n = static_cast<uint32_t>(khv_list.size());
            g_svs_inst_ord.clear();
            g_svs_inst_ord.reserve(static_cast<size_t>(khv_n) * 2u);

            for (uint32_t khv_i = 0; khv_i < khv_n; ++khv_i) {
                if (!khv_list[khv_i].alpha_caster && khv_list[khv_i].a_vis &&
                    !khv_list[khv_i].no_foot) g_svs_inst_ord.push_back(khv_i);   // KH_SEAM_CULL, KH_FOOT_BLEND.
            }

            khv_inst_a = static_cast<uint32_t>(g_svs_inst_ord.size());
            for (uint32_t khv_i = 0; khv_i < khv_n; ++khv_i) g_svs_inst_ord.push_back(khv_i);
            khv_inst_b = khv_n;
            const std::vector<KhSvCaster>& khv_src = khv_list;
            // (mesh, level, face): KH_SHADOW_LOD_DRAWN, KH_SEAM_FACE.
            auto khv_bymesh = [&khv_src](uint32_t khv_x, uint32_t khv_y) {
                if (khv_src[khv_x].mesh != khv_src[khv_y].mesh) return khv_src[khv_x].mesh < khv_src[khv_y].mesh;
                if (khv_src[khv_x].lod != khv_src[khv_y].lod) return khv_src[khv_x].lod < khv_src[khv_y].lod;
                return !khv_src[khv_x].two_sided && khv_src[khv_y].two_sided;   // KH_SEAM_FACE: a run per face rule.
            };
            // KH_SKIN_POOL: with a chunk installed, a chunk's casters together within their (mesh, level, face), for
            // its runs (the order is free: depth and MIN only).
            auto khv_bypool = [&](uint32_t khv_x, uint32_t khv_y) {
                if (khv_src[khv_x].mesh != khv_src[khv_y].mesh) return khv_src[khv_x].mesh < khv_src[khv_y].mesh;
                if (khv_src[khv_x].lod != khv_src[khv_y].lod) return khv_src[khv_x].lod < khv_src[khv_y].lod;
                if (khv_src[khv_x].two_sided != khv_src[khv_y].two_sided) {   // KH_SEAM_FACE.
                    return !khv_src[khv_x].two_sided && khv_src[khv_y].two_sided;
                }
                return kh_cast_key_at(khv_src[khv_x].slot) < kh_cast_key_at(khv_src[khv_y].slot);
            };
            if (g_skin_pool_any) {
                for (const KhSvCaster& khv_kc : khv_list) kh_cast_key_put(khv_kc.mesh, khv_kc.slot);
                std::stable_sort(g_svs_inst_ord.begin(), g_svs_inst_ord.begin() + khv_inst_a, khv_bypool);
                std::stable_sort(g_svs_inst_ord.begin() + khv_inst_a, g_svs_inst_ord.end(), khv_bypool);
            } else {
                std::stable_sort(g_svs_inst_ord.begin(), g_svs_inst_ord.begin() + khv_inst_a, khv_bymesh);
                std::stable_sort(g_svs_inst_ord.begin() + khv_inst_a, g_svs_inst_ord.end(), khv_bymesh);
            }

            const UINT khv_need = static_cast<UINT>(khv_inst_a + khv_inst_b);

            if (khv_need > g_res.seam_instance_cap) {   // Grow-only, doubling (the sun buffer's shape).
                ID3D11Device* khv_idev = nullptr;
                ctx->GetDevice(&khv_idev);

                if (khv_idev) {
                    KH_SAFE_RELEASE(g_res.seam_instance_vb);
                    UINT khv_cap = g_res.seam_instance_cap > 128 ? g_res.seam_instance_cap : 128;
                    while (khv_cap < khv_need) khv_cap *= 2;
                    D3D11_BUFFER_DESC khv_ibd = {};
                    khv_ibd.ByteWidth = khv_cap * KH_INST_STRIDE;
                    khv_ibd.Usage = D3D11_USAGE_DYNAMIC;
                    khv_ibd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                    khv_ibd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                    if (SUCCEEDED(khv_idev->CreateBuffer(&khv_ibd, nullptr, &g_res.seam_instance_vb))) {
                        g_res.seam_instance_cap = khv_cap;
                    } else {
                        g_res.seam_instance_cap = 0;
                    }

                    khv_idev->Release();
                }
            }

            khv_inst_ok = g_res.seam_instance_vb && khv_need <= g_res.seam_instance_cap;
        }

        if (khv_inst_ok) {   // One WRITE_DISCARD for both loops' lanes.
            D3D11_MAPPED_SUBRESOURCE khv_im = {};

            if (SUCCEEDED(ctx->Map(g_res.seam_instance_vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &khv_im))) {
                KhInstLane* khv_lane = static_cast<KhInstLane*>(khv_im.pData);

                for (uint32_t khv_i = 0; khv_i < khv_inst_a + khv_inst_b; ++khv_i) {
                    const KhSvCaster& khv_c = khv_list[g_svs_inst_ord[khv_i]];
                    khv_lane->slot = khv_c.slot;   // KH_OBJBUF: the record index.
                    khv_lane->dither = 0.0f;       // Unread by both seam entry points.
                    khv_lane->alpha = khv_c.alpha;
                    khv_lane->mat_base = 0.0f;
                    ++khv_lane;
                }

                ctx->Unmap(g_res.seam_instance_vb, 0);
            } else {
                khv_inst_ok = false;   // Both loops fall back to their per-caster path.
            }
        }
    }

    if (khv_frame_ok) {
        UINT khv_stride = sizeof(MeshVertex);
        KhVbRef khv_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.

        // The alpha casters take the textured VS + layout and the clip-only PS, per submesh with the material bound;
        // the whole-object translucent ones are skipped. State flips per caster (khv_alpha_bound) and the opaque
        // footprint state returns at the end.
        bool khv_alpha_bound = false;
        ID3D11Device* khv_dev = nullptr;
        ctx->GetDevice(&khv_dev);

        // Hoisted out of the per-caster loop (ConstantData is over 5 KB). Every field the body reads is rewritten each
        // iteration; mat_ctl is written only by kh_bind_material (the alpha path), and the alpha path zeroes it again
        // on its way out (the memset after its submesh loop).
        ConstantData khv_obj = {};

        // KH_SEAM_WITHHOLD_DRAW: a withheld seam binds neither a depth nor a render target, so its footprint draws
        // land nowhere: none is issued - the instanced runs are skipped whole (binds and draws), and the per-caster
        // paths bind, upload (a set's first table fill keeps its place; a failed upload still ends the loop) and
        // draw nothing. The mirror prepass and the KH_VOL_FOOT mask below bind their own state and still draw.
        if (khv_inst_ok && !khv_withhold) {   // KH_SEAM_INST: one draw per mesh run, not per caster.
            ID3D11Buffer* khv_ivb[2] = { nullptr, g_res.seam_instance_vb };
            const UINT khv_istr[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
            UINT khv_ioff[2] = { 0, 0 };
            ctx->IASetInputLayout(g_res.layout_sundepth);   // VSInSeam mirrors VSInSun.
            ctx->VSSetShader(g_res.vs_seam_inst, nullptr, 0);
            kh_objbuf_bind(ctx);   // KH_OBJBUF: t39, the records the lane indexes.
            uint32_t khv_first = 0;
            KhDepthSk khv_sk = { g_res.vs_seam_inst, g_res.layout_sundepth, g_res.vs_seam_inst_sk, nullptr, nullptr };

            while (khv_first < khv_inst_a) {
                const int khv_mid = khv_list[g_svs_inst_ord[khv_first]].mesh;
                const int khv_lod = khv_list[g_svs_inst_ord[khv_first]].lod;   // KH_SHADOW_LOD_DRAWN.
                uint32_t khv_last = khv_first + 1;
                const KhVbRef khv_rv = kh_mesh_vb_ref(khv_mid, khv_list[g_svs_inst_ord[khv_first]].slot);   // KH_CLOTH.
                const KhSkinPoolChunk* const khv_pc =   // KH_SKIN_POOL.
                    kh_cast_pool_of(khv_mid, khv_list[g_svs_inst_ord[khv_first]].slot, g_res.vs_seam_inst_sk);

                const bool khv_two = khv_list[g_svs_inst_ord[khv_first]].two_sided;   // KH_SEAM_FACE.
                while (khv_last < khv_inst_a &&
                       khv_list[g_svs_inst_ord[khv_last]].mesh == khv_mid &&
                       khv_list[g_svs_inst_ord[khv_last]].lod == khv_lod &&
                       khv_list[g_svs_inst_ord[khv_last]].two_sided == khv_two &&
                       kh_cast_run_joins(khv_rv, khv_pc, khv_list[g_svs_inst_ord[khv_last]].mesh,
                                         khv_list[g_svs_inst_ord[khv_last]].slot, g_res.vs_seam_inst_sk)) ++khv_last;

                khv_rs_face(khv_two);
                khv_sk.use(ctx, khv_pc);
                khv_ivb[0] = khv_rv.vb;
                khv_ioff[0] = khv_rv.off;   // KH_SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, khv_ivb, khv_istr, khv_ioff);
                ctx->IASetIndexBuffer(g_res.mesh_ib[khv_mid], DXGI_FORMAT_R32_UINT, 0);
                // The run's level range, exactly as the per-caster draw below takes it (the level the injection drew).
                UINT khv_is = 0, khv_ic = 0;
                mesh_lod_range_of(khv_mid, khv_lod, khv_is, khv_ic);
                ctx->DrawIndexedInstanced(khv_ic, khv_last - khv_first, khv_is, 0, khv_first);
                khv_first = khv_last;
            }
            khv_sk.end(ctx);   // KH_SKIN_POOL.

            // Back to the opaque per-object footprint state for the alpha casters.
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
        }

        for (const auto& c : khv_list) {
            // The opaque set went out instanced above; the partition is exact (alpha_caster is false for every lane
            // written). A see-through blend mode is in neither (KH_FOOT_BLEND).
            if (!c.a_vis) continue;   // KH_SEAM_CULL: outside the footprint's frustum.
            if (c.no_foot) continue;   // KH_FOOT_BLEND: the world behind shows through.
            if (khv_inst_ok && !c.alpha_caster) continue;
            // Writes no depth in the colour pass: not drawn.
            if (c.alpha_caster && c.alpha < 0.999f) {  continue; }
            const bool khv_want_alpha = c.alpha_caster && c.mats && khv_dev;
            khv_rs_face(c.two_sided);   // KH_SEAM_FACE.

            if (khv_want_alpha != khv_alpha_bound) {
                ctx->IASetInputLayout(khv_want_alpha ? g_res.layout_tex : g_res.input_layout);
                ctx->VSSetShader(khv_want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                ctx->PSSetShader(khv_want_alpha ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
                if (khv_want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                khv_alpha_bound = khv_want_alpha;
            }

            const KhVbRef khv_want = kh_mesh_vb_ref(c.mesh, c.slot);   // KH_CLOTH.
            if (khv_want != khv_bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &khv_want.vb, &khv_stride, &khv_want.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                khv_bound_vb = khv_want;
            }

            khv_obj.depth_params[2] = khv_vp_lo;
            khv_obj.depth_params[3] = khv_vp_hi;
            khv_obj.fx_meta[0] = 0.0f;
            khv_obj.fx_meta[1] = 0.0f;
            khv_obj.center_size[0] = c.pos[0];
            khv_obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            khv_obj.center_size[2] = c.pos[1];
            // A footprint drawn a few percent larger covers the visible box under any displacement up to that margin
            // (only the engine's ground shading reads those texels). This is 1.0 and the instanced twin reads the
            // record's extents with no grow lane; raising it needs a matching lane on the instance stream.
            float khv_grow = 1.0f;
            khv_obj.size_axes[0] = c.size[0] * khv_grow;
            khv_obj.size_axes[1] = c.size[2] * khv_grow;
            khv_obj.size_axes[2] = c.size[1] * khv_grow;
            kh_fill_obj_rot(khv_obj, c.rot);
            if (khv_rebase_on) kh_fill_center_rel(khv_obj, khv_cam, c.res);   // KH_POS_RES.

            if (khv_want_alpha) {   // KH_FOOTPRINT_ALPHA: per submesh, the material's alpha lanes +
                                    // maps bound.
                const MeshDef& khv_md = mesh_def(c.mesh);
                const std::vector<MeshSubmesh>& khv_tab = mesh_lod_submeshes(khv_md, c.lod);   // KH_SHADOW_LOD_DRAWN.
                bool khv_ok = true;

                for (size_t khv_s = 0; khv_s < khv_tab.size() && khv_ok; ++khv_s) {
                    const MeshSubmesh& khv_sm = khv_tab[khv_s];
                    if (khv_sm.index_count == 0) continue;
                    kh_bind_material(ctx, khv_dev, khv_obj, c.mats, khv_s);
                    khv_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khv_obj);
                    if (!khv_ok) break;
                    if (!khv_withhold) {   // KH_SEAM_WITHHOLD_DRAW.
                        ctx->DrawIndexed(khv_sm.index_count, khv_sm.index_start, 0);
                    }
                }
                memset(khv_obj.mat_ctl, 0, sizeof(khv_obj.mat_ctl));   // KH_SEAM_CB_HOIST: the
                                                                       // only lanes this path dirties.

                if (!khv_ok) {  break; }
                continue;
            }

            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khv_obj)) {
                break;
            }

            {   // KH_SHADOW_LOD_DRAWN.
                UINT khsl_is = 0, khsl_ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, khsl_is, khsl_ic);
                if (!khv_withhold) ctx->DrawIndexed(khsl_ic, khsl_is, 0);   // KH_SEAM_WITHHOLD_DRAW.
            }
        }

        if (khv_alpha_bound) {   // The opaque footprint state again (the mirror prepass below
                                 // rebinds its own).
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
        }
        // KH_INFRONT: the view-model casters join the WORLD count. The engine's hands recount (kh_infront_seam_inject's
        // window) redraws only the volumes that reach the camera, and the depth partition hides every world-range
        // volume from a hands-range depth, so a shadow from a vehicle or a building reaches an inFront mesh only
        // through this draw: the same footprint state, the world range, the seam's own transform, under the depth clamp
        // (rast_inject, or its KH_SEAM_FACE twin) - a fragment nearer than the world near plane lands on it, which is
        // the right count there since the engine clips its volumes' front faces short of it. The stencil is untouched,
        // and the hands-seam draw keeps it, so this count survives to the resolve (the recount adds its near subset on
        // top; a shadowed count stays nonzero and small).
        for (const auto& c : khv_front) {
            if (c.no_foot) continue;   // KH_FOOT_BLEND: the world behind shows through.
            const bool khvf_want_alpha = c.alpha_caster && c.mats && khv_dev;
            khv_rs_face(c.two_sided);   // KH_SEAM_FACE.
            if (khvf_want_alpha != khv_alpha_bound) {
                ctx->IASetInputLayout(khvf_want_alpha ? g_res.layout_tex : g_res.input_layout);
                ctx->VSSetShader(khvf_want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                ctx->PSSetShader(khvf_want_alpha ? g_res.ps_inj_depth_a : nullptr, nullptr, 0);
                if (khvf_want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                khv_alpha_bound = khvf_want_alpha;
            }
            const KhVbRef khvf_vb = kh_mesh_vb_ref(c.mesh, c.slot);   // KH_CLOTH.
            if (khvf_vb != khv_bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &khvf_vb.vb, &khv_stride, &khvf_vb.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                khv_bound_vb = khvf_vb;
            }
            khv_obj.depth_params[2] = khv_vp_lo;
            khv_obj.depth_params[3] = khv_vp_hi;
            khv_obj.fx_meta[0] = 0.0f;
            khv_obj.fx_meta[1] = 0.0f;
            khv_obj.center_size[0] = c.pos[0];
            khv_obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
            khv_obj.center_size[2] = c.pos[1];
            khv_obj.size_axes[0] = c.size[0];
            khv_obj.size_axes[1] = c.size[2];
            khv_obj.size_axes[2] = c.size[1];
            kh_fill_obj_rot(khv_obj, c.rot);
            if (khv_rebase_on) kh_fill_center_rel(khv_obj, khv_cam, c.res);   // KH_POS_RES.
            bool khvf_ok = true;
            if (khvf_want_alpha) {
                const MeshDef& khvf_md = mesh_def(c.mesh);
                const std::vector<MeshSubmesh>& khvf_tab = mesh_lod_submeshes(khvf_md, 0);
                for (size_t khvf_s = 0; khvf_s < khvf_tab.size() && khvf_ok; ++khvf_s) {
                    const MeshSubmesh& khvf_sm = khvf_tab[khvf_s];
                    if (khvf_sm.index_count == 0) continue;
                    kh_bind_material(ctx, khv_dev, khv_obj, c.mats, khvf_s);
                    khvf_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khv_obj);
                    if (!khvf_ok) break;
                    if (!khv_withhold) {   // KH_SEAM_WITHHOLD_DRAW.
                        ctx->DrawIndexed(khvf_sm.index_count, khvf_sm.index_start, 0);
                    }
                }
                memset(khv_obj.mat_ctl, 0, sizeof(khv_obj.mat_ctl));
            } else {
                khvf_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khv_obj);
                if (khvf_ok) {
                    UINT khvf_is = 0, khvf_ic = 0;
                    mesh_lod_range_of(c.mesh, 0, khvf_is, khvf_ic);
                    if (!khv_withhold) ctx->DrawIndexed(khvf_ic, khvf_is, 0);   // KH_SEAM_WITHHOLD_DRAW.
                }
            }
            if (!khvf_ok) break;
        }
        if (khv_alpha_bound) {
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            ctx->PSSetShader(nullptr, nullptr, 0);
        }
        if (khv_dev) khv_dev->Release();

        if (!g_rp_seam_b) g_svs_injected_frame = true;   // KH_VOL_REPLAY: replay B is not this frame's seam.

        g_svs_prime_ready = true;   // Transform + list valid.
    }

    // Grafted at the instant the footprint already draws at, inside the same state envelope - the tail below restores
    // everything this block touches. Runs whether or not the footprint's frame CB landed: the mirror uploads its own
    // frame slice.
    {
        // KH_INFRONT; KH_MIR_REPLAY: for B only when kh_rp_replay armed mirror B.
        bool khvm_go = (!khv_list.empty() || !khv_front.empty()) && (!g_rp_seam_b || g_rp_mir_want);
        // KH_MIR_GATE: the real mirror only when a mesh can read it next cycle; skipped, it is marked absent, so
        // every reader takes mirMeta.x = 0 (no mirror) rather than an older one.
        if (khvm_go && !g_rp_seam_b && !kh_vmir_wanted(khv_cam, khv_pv.projection)) {
            khvm_go = false;
            g_vmir_mask_time = -1.0f;
        }
        if (khvm_go) {
            ID3D11Device* khvm_dev = nullptr;
            ctx->GetDevice(&khvm_dev);
            bool khvm_ok = false;
            if (khvm_dev) {
                {
                    khvm_ok = kh_vmir_ensure_targets(khvm_dev) && kh_vmir_ensure_gpu(khvm_dev);
                }
                khvm_dev->Release();
            }
            // KH_MIR_REPLAY: in B mode the prepass is mirror B's, at this cycle's pose (b2 is the pass's first draw's,
            // bound by kh_rp_replay_b, as the real prepass read it); the real mirror stays as the pass counted it.
            ID3D11DepthStencilView* const khvm_dsv = g_rp_seam_b ? g_rp_mirb_dsv : g_vmir_dsv;
            if (khvm_ok && g_vmir_vs && khvm_dsv) {
                // The frame CB uploads verbatim - VSMirror takes x/y/w from
                // this matrix and remaps z through the live-b2 low pair itself.
                if (kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khv_cbf)) {
                    ctx->ClearDepthStencilView(khvm_dsv,
                        D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
                    ctx->OMSetRenderTargets(0, nullptr, khvm_dsv);
                    D3D11_VIEWPORT khvm_vp = {};
                    khvm_vp.Width = static_cast<float>(g_vmir_w);
                    khvm_vp.Height = static_cast<float>(g_vmir_h);
                    khvm_vp.MinDepth = 0.0f;
                    khvm_vp.MaxDepth = 1.0f;
                    ctx->RSSetViewports(1, &khvm_vp);
                    ctx->PSSetShader(nullptr, nullptr, 0);

                    if (khv_inst_ok) {   // KH_SEAM_INST: one draw per mesh run, not per caster.
                        ID3D11Buffer* khvm_ivb[2] = { nullptr, g_res.seam_instance_vb };
                        const UINT khvm_istr[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
                        UINT khvm_ioff[2] = { 0, 0 };
                        ctx->IASetInputLayout(g_res.layout_sundepth);
                        ctx->VSSetShader(g_res.vs_mirror_inst, nullptr, 0);
                        kh_objbuf_bind(ctx);   // KH_OBJBUF: t39, the records the lane indexes.
                        uint32_t khvm_first = 0;
                        KhDepthSk khvm_sk = { g_res.vs_mirror_inst, g_res.layout_sundepth, g_res.vs_mirror_inst_sk,
                                              nullptr, nullptr };   // KH_SKIN_POOL.

                        while (khvm_first < khv_inst_b) {
                            // Loop B's lanes sit at [khv_inst_a, khv_inst_a + khv_inst_b) in the one buffer, so every
                            // run index carries that base.
                            const int khvm_mid = khv_list[g_svs_inst_ord[khv_inst_a + khvm_first]].mesh;
                            const int khvm_lod = khv_list[g_svs_inst_ord[khv_inst_a + khvm_first]].lod;   // KH_SHADOW_LOD_DRAWN.
                            uint32_t khvm_last = khvm_first + 1;
                            const KhVbRef khvm_rv = kh_mesh_vb_ref(khvm_mid,
                                khv_list[g_svs_inst_ord[khv_inst_a + khvm_first]].slot);   // KH_CLOTH.
                            const KhSkinPoolChunk* const khvm_pc = kh_cast_pool_of(khvm_mid,   // KH_SKIN_POOL.
                                khv_list[g_svs_inst_ord[khv_inst_a + khvm_first]].slot, g_res.vs_mirror_inst_sk);

                            // KH_SEAM_FACE: a run is one face rule too.
                            const bool khvm_two = khv_list[g_svs_inst_ord[khv_inst_a + khvm_first]].two_sided;
                            while (khvm_last < khv_inst_b &&
                                   khv_list[g_svs_inst_ord[khv_inst_a + khvm_last]].mesh == khvm_mid &&
                                   khv_list[g_svs_inst_ord[khv_inst_a + khvm_last]].lod == khvm_lod &&
                                   khv_list[g_svs_inst_ord[khv_inst_a + khvm_last]].two_sided == khvm_two &&
                                   kh_cast_run_joins(khvm_rv, khvm_pc,
                                                     khv_list[g_svs_inst_ord[khv_inst_a + khvm_last]].mesh,
                                                     khv_list[g_svs_inst_ord[khv_inst_a + khvm_last]].slot,
                                                     g_res.vs_mirror_inst_sk)) ++khvm_last;

                            khv_rs_face(khvm_two);   // KH_SEAM_FACE.
                            khvm_sk.use(ctx, khvm_pc);
                            khvm_ivb[0] = khvm_rv.vb;
                            khvm_ioff[0] = khvm_rv.off;   // KH_SKIN_POOL.
                            ctx->IASetVertexBuffers(0, 2, khvm_ivb, khvm_istr, khvm_ioff);
                            ctx->IASetIndexBuffer(g_res.mesh_ib[khvm_mid], DXGI_FORMAT_R32_UINT, 0);
                            UINT khvm_is = 0, khvm_ic = 0;
                            mesh_lod_range_of(khvm_mid, khvm_lod, khvm_is, khvm_ic);   // KH_SHADOW_LOD_DRAWN.
                            ctx->DrawIndexedInstanced(khvm_ic, khvm_last - khvm_first, khvm_is, 0,
                                                      khv_inst_a + khvm_first);
                            khvm_first = khvm_last;
                        }
                        khvm_sk.end(ctx);   // KH_SKIN_POOL.
                    } else {
                        ctx->VSSetShader(g_vmir_vs, nullptr, 0);
                        UINT khvm_stride = sizeof(MeshVertex);
                        KhVbRef khvm_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
                        // KH_SEAM_CB_HOIST: see the twin above.
                        ConstantData khvm_obj = {};
                        for (const auto& c : khv_list) {
                            khv_rs_face(c.two_sided);   // KH_SEAM_FACE.
                            const KhVbRef khvm_want = kh_mesh_vb_ref(c.mesh, c.slot);   // KH_CLOTH.
                            if (khvm_want != khvm_bound_vb) {
                                ctx->IASetVertexBuffers(0, 1, &khvm_want.vb,
                                                        &khvm_stride, &khvm_want.off);
                                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                                khvm_bound_vb = khvm_want;
                            }
                            khvm_obj.center_size[0] = c.pos[0];
                            khvm_obj.center_size[1] = c.pos[2];   // SQF -> engine axes.
                            khvm_obj.center_size[2] = c.pos[1];
                            khvm_obj.size_axes[0] = c.size[0];
                            khvm_obj.size_axes[1] = c.size[2];
                            khvm_obj.size_axes[2] = c.size[1];
                            kh_fill_obj_rot(khvm_obj, c.rot);
                            if (khv_rebase_on) kh_fill_center_rel(khvm_obj, khv_cam, c.res);   // KH_POS_RES.
                            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khvm_obj)) break;
                            {   // KH_SHADOW_LOD_DRAWN.
                                UINT khsl_is = 0, khsl_ic = 0;
                                mesh_lod_range_of(c.mesh, c.lod, khsl_is, khsl_ic);
                                ctx->DrawIndexed(khsl_ic, khsl_is, 0);
                            }
                        }
                    }   // KH_SEAM_INST: the per-caster fallback.
                    // KH_INFRONT: the view-model casters into the mirror, per caster, the world casters' own recipe
                    // (VSMirror: x / y / w from the seam's matrix, z on the 0.05 near). The colour pass reads the
                    // mirror over the whole mesh (mirMeta.x = 2), so the near collapse the mirror exists for never
                    // reaches it; the volume-buffer draws stay as the fallback where the mirror is not built.
                    if (!khv_front.empty()) {
                        ctx->IASetInputLayout(g_res.input_layout);
                        ctx->VSSetShader(g_vmir_vs, nullptr, 0);
                        UINT khvf_stride = sizeof(MeshVertex);
                        KhVbRef khvf_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
                        ConstantData khvf_obj = {};
                        for (const auto& c : khv_front) {
                            khv_rs_face(c.two_sided);   // KH_SEAM_FACE.
                            const KhVbRef khvf_want = kh_mesh_vb_ref(c.mesh, c.slot);
                            if (khvf_want != khvf_bound_vb) {
                                ctx->IASetVertexBuffers(0, 1, &khvf_want.vb, &khvf_stride, &khvf_want.off);
                                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                                khvf_bound_vb = khvf_want;
                            }
                            khvf_obj.center_size[0] = c.pos[0];
                            khvf_obj.center_size[1] = c.pos[2];   // SQF -> engine axes.
                            khvf_obj.center_size[2] = c.pos[1];
                            khvf_obj.size_axes[0] = c.size[0];
                            khvf_obj.size_axes[1] = c.size[2];
                            khvf_obj.size_axes[2] = c.size[1];
                            kh_fill_obj_rot(khvf_obj, c.rot);
                            if (khv_rebase_on) kh_fill_center_rel(khvf_obj, khv_cam, c.res);   // KH_POS_RES.
                            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khvf_obj)) break;
                            UINT khvf_is = 0, khvf_ic = 0;
                            mesh_lod_range_of(c.mesh, 0, khvf_is, khvf_ic);
                            ctx->DrawIndexed(khvf_ic, khvf_is, 0);
                        }
                    }
                    if (g_rp_seam_b) {
                        g_rp_mirb_drawn = true;   // KH_MIR_REPLAY: mirror B; the real mirror's stamps stand.
                    } else {
                        g_vmir_prepass_stamp = static_cast<uint32_t>(g_svs_frame_seq);
                        g_vmir_mask_time = effect_time_seconds();
                        g_vmir_prepass_src = g_svs_vol_src;
                    }
                }
            }
        }
    }

    // KH_VOL_FOOT: the footprint's view distance into the unpublished mask - the world casters (khv_list) with the same
    // transforms, levels, frustum cut and alpha clip as the footprint above, with no depth test (the engine's depth is
    // what this replaces) and a MIN blend (the nearest of our surfaces per pixel). The view-model casters the footprint
    // adds to the world count (khv_front) are not in it: their own receivers read no witness (the slice zeroes
    // stenVol4), and a world surface of ours they cover is hidden under the slice. The tail below restores every state
    // this touches.
    if (khv_frame_ok && g_res.ps_seam_foot && g_res.dlsw_min_blend && g_res.dss_off &&
        kh_svs_foot_ensure(ctx, khv_w, khv_h)) {
        const int khvp_ix = g_svs_foot_pub == 0 ? 1 : 0;
        const FLOAT khvp_far4[4] = { 1.0e30f, 1.0e30f, 1.0e30f, 1.0e30f };
        ctx->OMSetRenderTargets(1, &g_svs_foot_rtv[khvp_ix], nullptr);
        ctx->ClearRenderTargetView(g_svs_foot_rtv[khvp_ix], khvp_far4);
        ctx->RSSetViewports(1, &khv_vp);
        if (khv_rs_own) ctx->RSSetState(khv_rs);
        khv_rs_bound = khv_rs;   // KH_SEAM_FACE.
        const FLOAT khvp_bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        ctx->OMSetBlendState(g_res.dlsw_min_blend, khvp_bf, 0xFFFFFFFF);
        ctx->OMSetDepthStencilState(g_res.dss_off, 0);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 2, khv_cbs);
        ctx->PSSetConstantBuffers(0, 2, khv_cbs);
        ctx->PSSetShader(g_res.ps_seam_foot, nullptr, 0);
        // The mirror prepass above may have re-uploaded the frame half; this is the footprint's own.
        bool khvp_ok = kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khv_cbf);
        if (khvp_ok && khv_inst_ok) {   // Loop A's runs, as the footprint forms them.
            ID3D11Buffer* khvp_ivb[2] = { nullptr, g_res.seam_instance_vb };
            const UINT khvp_istr[2] = { sizeof(MeshVertex), KH_INST_STRIDE };
            UINT khvp_ioff[2] = { 0, 0 };
            ctx->IASetInputLayout(g_res.layout_sundepth);
            ctx->VSSetShader(g_res.vs_seam_inst, nullptr, 0);
            kh_objbuf_bind(ctx);
            uint32_t khvp_first = 0;
            KhDepthSk khvp_sk = { g_res.vs_seam_inst, g_res.layout_sundepth, g_res.vs_seam_inst_sk, nullptr, nullptr };
            while (khvp_first < khv_inst_a) {
                const KhSvCaster& khvp_c0 = khv_list[g_svs_inst_ord[khvp_first]];
                const KhVbRef khvp_vb0 = kh_mesh_vb_ref(khvp_c0.mesh, khvp_c0.slot);
                const KhSkinPoolChunk* const khvp_pc =   // KH_SKIN_POOL: loop A's runs, as the footprint forms them.
                    kh_cast_pool_of(khvp_c0.mesh, khvp_c0.slot, g_res.vs_seam_inst_sk);
                uint32_t khvp_last = khvp_first + 1;
                while (khvp_last < khv_inst_a &&
                       khv_list[g_svs_inst_ord[khvp_last]].mesh == khvp_c0.mesh &&
                       khv_list[g_svs_inst_ord[khvp_last]].lod == khvp_c0.lod &&
                       khv_list[g_svs_inst_ord[khvp_last]].two_sided == khvp_c0.two_sided &&   // KH_SEAM_FACE.
                       kh_cast_run_joins(khvp_vb0, khvp_pc, khv_list[g_svs_inst_ord[khvp_last]].mesh,
                                         khv_list[g_svs_inst_ord[khvp_last]].slot, g_res.vs_seam_inst_sk)) ++khvp_last;
                khv_rs_face(khvp_c0.two_sided);   // KH_SEAM_FACE.
                khvp_sk.use(ctx, khvp_pc);
                khvp_ivb[0] = khvp_vb0.vb;
                khvp_ioff[0] = khvp_vb0.off;   // KH_SKIN_POOL.
                ctx->IASetVertexBuffers(0, 2, khvp_ivb, khvp_istr, khvp_ioff);
                ctx->IASetIndexBuffer(g_res.mesh_ib[khvp_c0.mesh], DXGI_FORMAT_R32_UINT, 0);
                UINT khvp_is = 0, khvp_ic = 0;
                mesh_lod_range_of(khvp_c0.mesh, khvp_c0.lod, khvp_is, khvp_ic);
                ctx->DrawIndexedInstanced(khvp_ic, khvp_last - khvp_first, khvp_is, 0, khvp_first);
                khvp_first = khvp_last;
            }
            khvp_sk.end(ctx);   // KH_SKIN_POOL.
        }
        if (khvp_ok) {   // The per-object casters (the alpha ones, or all without instancing).
            ctx->IASetInputLayout(g_res.input_layout);
            ctx->VSSetShader(g_res.vs, nullptr, 0);
            const UINT khvp_stride = sizeof(MeshVertex);
            KhVbRef khvp_bound;   // KH_SKIN_POOL: the pair.
            ConstantData khvp_obj = {};
            // KH_FOOTPRINT_ALPHA's rule here too: a cutout hole or a blend material's see-through texel is not in the
            // footprint, so it must not be in the mask - drawn, it would hold the see-through surface's distance where
            // the engine counted what is behind it, and our surface there (an interior through glass, a mesh behind a
            // cutout) would fail the witness and go lit. An alpha caster the footprint drew per submesh draws the same
            // way, through the textured VS and PSDlsMaskA: PSInjDepthA's clip, then SV_Position.w (its LOD dither lane
            // reads this pass's zero blendCtl.w - off). Absent twin: the caster masks whole, as the dlsw mask does.
            ID3D11Device* khvp_dev = nullptr;
            ctx->GetDevice(&khvp_dev);
            const bool khvp_alpha_ok = khvp_dev && g_res.ps_dls_mask_a && g_res.vs_tex && g_res.layout_tex;
            bool khvp_alpha_bound = false;
            for (const auto& c : khv_list) {
                if (!c.a_vis) continue;                              // As the footprint.
                if (c.no_foot) continue;                             // KH_FOOT_BLEND: as the footprint.
                if (khv_inst_ok && !c.alpha_caster) continue;        // Drawn instanced above.
                if (c.alpha_caster && c.alpha < 0.999f) continue;    // Writes no depth: no footprint.
                // The footprint's own test (khv_want_alpha): per submesh of the drawn
                // level (c.lod, the injection's), else that level whole.
                const bool khvp_fp_alpha = c.alpha_caster && c.mats && khvp_dev;
                khv_rs_face(c.two_sided);   // KH_SEAM_FACE.
                const bool khvp_want_alpha = khvp_fp_alpha && khvp_alpha_ok;
                if (khvp_want_alpha != khvp_alpha_bound) {
                    ctx->IASetInputLayout(khvp_want_alpha ? g_res.layout_tex : g_res.input_layout);
                    ctx->VSSetShader(khvp_want_alpha ? g_res.vs_tex : g_res.vs, nullptr, 0);
                    ctx->PSSetShader(khvp_want_alpha ? g_res.ps_dls_mask_a : g_res.ps_seam_foot, nullptr, 0);
                    if (khvp_want_alpha && g_res.mat_sampler) ctx->PSSetSamplers(0, 1, &g_res.mat_sampler);
                    khvp_alpha_bound = khvp_want_alpha;
                }
                const KhVbRef khvp_vb = kh_mesh_vb_ref(c.mesh, c.slot);
                if (khvp_vb != khvp_bound) {
                    ctx->IASetVertexBuffers(0, 1, &khvp_vb.vb, &khvp_stride, &khvp_vb.off);
                    ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                    khvp_bound = khvp_vb;
                }
                khvp_obj.depth_params[2] = khv_vp_lo;
                khvp_obj.depth_params[3] = khv_vp_hi;
                khvp_obj.center_size[0] = c.pos[0];
                khvp_obj.center_size[1] = c.pos[2];   // SQF [x,y,zASL] -> engine [x,zASL,y].
                khvp_obj.center_size[2] = c.pos[1];
                khvp_obj.size_axes[0] = c.size[0];
                khvp_obj.size_axes[1] = c.size[2];
                khvp_obj.size_axes[2] = c.size[1];
                kh_fill_obj_rot(khvp_obj, c.rot);
                if (khv_rebase_on) kh_fill_center_rel(khvp_obj, khv_cam, c.res);   // KH_POS_RES.
                if (khvp_want_alpha) {   // The footprint's submeshes, each with its material.
                    const MeshDef& khvp_md = mesh_def(c.mesh);
                    const std::vector<MeshSubmesh>& khvp_tab = mesh_lod_submeshes(khvp_md, c.lod);
                    for (size_t khvp_s = 0; khvp_s < khvp_tab.size() && khvp_ok; ++khvp_s) {
                        const MeshSubmesh& khvp_sm = khvp_tab[khvp_s];
                        if (khvp_sm.index_count == 0) continue;
                        kh_bind_material(ctx, khvp_dev, khvp_obj, c.mats, khvp_s);
                        khvp_ok = kh_upload_obj_cb(ctx, g_res.composite_cb, khvp_obj);
                        if (!khvp_ok) break;
                        ctx->DrawIndexed(khvp_sm.index_count, khvp_sm.index_start, 0);
                    }
                    memset(khvp_obj.mat_ctl, 0, sizeof(khvp_obj.mat_ctl));   // The only lanes it dirties.
                    if (!khvp_ok) break;
                    continue;
                }
                if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khvp_obj)) { khvp_ok = false; break; }
                // The level the footprint drew: the injection's (KH_SHADOW_LOD_DRAWN), whole or per submesh alike.
                UINT khvp_is = 0, khvp_ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, khvp_is, khvp_ic);
                ctx->DrawIndexed(khvp_ic, khvp_is, 0);
            }
            if (khvp_dev) khvp_dev->Release();
        }
        if (khvp_ok) {
            g_svs_foot_seam_ix = khvp_ix;
            g_svs_foot_seam_seq = g_svs_frame_seq;
        }
    }

    g_svs_skip_valid = !khv_withhold;   // KH_SVS_SKIP: this signature's pass completed (KH_SEAM_WITHHOLD: a withheld one drew nothing).
    khv_om.restore(ctx);
    if (khv_old_nvp > 0) ctx->RSSetViewports(khv_old_nvp, khv_old_vps);
    backup.restore(ctx);
    khv_om.release();
    g_ro.in_injection = khv_prev_inj;
}

// Runs on every OM depth bind, before the live-shadow gate: the seam has
// nothing to do with whether cascades are wanted.
inline void kh_volume_seam_track(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv,
                                 bool dsv_is_main, UINT n, ID3D11RenderTargetView* const* rtvs) {
    g_svs_vol_dsv_now = false;
    void* khv_id = dsv ? reorder_dsv_identity(dsv) : nullptr;
    g_svs_vol_dsv_bound = g_svs_vol_src && khv_id && khv_id == g_svs_vol_src_id;   // KH_INFRONT: any bind of it.
    const bool khv_depth_only = dsv && (n == 0 || !rtvs || !rtvs[0]);
    const bool khv_has_rtv = dsv && n > 0 && rtvs && rtvs[0];

    // One GetDesc per identity per frame for the first 8 identities (the
    // cache); their repeats are a pointer compare, a 9th pays it every bind.
    if (khv_id) {
        int khv_cls = -1;

        for (uint32_t khv_i = 0; khv_i < g_svs_cls_n; ++khv_i) {
            if (g_svs_cls_id[khv_i] == khv_id) { khv_cls = g_svs_cls_srv[khv_i]; break; }
        }

        if (khv_cls < 0) {
            ID3D11Resource* khv_r = nullptr;
            dsv->GetResource(&khv_r);

            if (khv_r) {
                ID3D11Texture2D* khv_t = nullptr;
                khv_r->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khv_t));
                khv_r->Release();

                if (khv_t) {
                    D3D11_TEXTURE2D_DESC khv_d = {};
                    khv_t->GetDesc(&khv_d);
                    khv_t->Release();
                    khv_cls = (khv_d.BindFlags & D3D11_BIND_SHADER_RESOURCE) ? 1 : 0;

                    if (g_svs_cls_n < 8) {
                        g_svs_cls_id[g_svs_cls_n] = khv_id;
                        g_svs_cls_srv[g_svs_cls_n] = static_cast<uint8_t>(khv_cls);
                        g_svs_cls_n++;
                    }
                }
            }
        }

        // Mask candidate latch.
        if (khv_cls == 0) {

            if (khv_has_rtv && rtvs && rtvs[0]) {
                ID3D11Resource* khv_mr = nullptr;
                rtvs[0]->GetResource(&khv_mr);

                if (khv_mr) {
                    ID3D11Texture2D* khv_mt = nullptr;
                    khv_mr->QueryInterface(__uuidof(ID3D11Texture2D),
                                           reinterpret_cast<void**>(&khv_mt));
                    khv_mr->Release();

                    if (khv_mt) {
                        D3D11_TEXTURE2D_DESC khv_md = {};
                        khv_mt->GetDesc(&khv_md);
                        if (g_svs_mask_id &&
                            static_cast<void*>(khv_mt) == g_svs_mask_id) {
                            g_svs_bracket = 1;   // Pre at the next draw.
                            kh_svs_vol_latch(dsv, khv_id);
                        }
                        const bool khv_ok =
                            (khv_md.BindFlags & D3D11_BIND_SHADER_RESOURCE) != 0 &&
                            g_main_depth_w && khv_md.Width == g_main_depth_w &&
                            khv_md.Height == g_main_depth_h;

                        if (khv_ok) {
                            if (g_svs_mask_cand) g_svs_mask_cand->Release();
                            g_svs_mask_cand = khv_mt;
                        } else {
                            khv_mt->Release();
                        }
                    }
                }
            }
        }
    }

    if (khv_has_rtv && khv_id) {
        bool khv_known = false;

        for (uint32_t khv_i = 0; khv_i < g_svs_rtv_seen_n; ++khv_i) {
            if (g_svs_rtv_seen[khv_i] == khv_id) { khv_known = true; break; }
        }

        if (!khv_known && g_svs_rtv_seen_n < 8) g_svs_rtv_seen[g_svs_rtv_seen_n++] = khv_id;
    }

    bool khv_seen_rtv = false;

    for (uint32_t khv_i = 0; khv_i < g_svs_rtv_seen_n; ++khv_i) {
        if (g_svs_rtv_seen[khv_i] == khv_id) { khv_seen_rtv = true; break; }
    }

    const bool khv_seam = khv_depth_only && khv_id && khv_seen_rtv;

    if (!khv_seam) return;

    // Shape, fetched only on a seam (once or twice a frame), never per bind.
    ID3D11Resource* khv_res = nullptr;
    dsv->GetResource(&khv_res);
    if (!khv_res) return;
    ID3D11Texture2D* khv_tex = nullptr;
    khv_res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&khv_tex));
    khv_res->Release();
    if (!khv_tex) return;
    D3D11_TEXTURE2D_DESC khv_td = {};
    khv_tex->GetDesc(&khv_td);
    khv_tex->Release();

    // The scene prepass's target is the main depth's size. KH_SEAM_SQUARE: only while that is not yet known is a
    // square target taken for a cascade. Do not test the square rule first: it refuses a square main depth (a square
    // window's), and every seam, footprint mask and mirror with it. A cascade the main depth's size is still
    // refused below: it is SRV-readable.
    if (g_main_depth_w && g_main_depth_h) {
        if (khv_td.Width != g_main_depth_w || khv_td.Height != g_main_depth_h) return;
    } else if (khv_td.Width == khv_td.Height) {
        return;
    }

    // The shader-resource discriminator: the colour pass's depth is typeless and SRV-readable; the volume buffer is
    // write/test-only and never read as an SRV anywhere in the frame. An ordinal cannot identify a resource.
    if (khv_td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        return;
    }

    {
        // Mask without our depth in the buffer would answer nothing.
        if (!kh_svs_inject_armed()) {
            return;
        }
    }
    // This DSV is the volume buffer, so the draws that follow are counting draws until the next bind.
    g_svs_vol_dsv_now = true;

    if (g_svs_injected_frame) {
          return;

    }

    g_svs_pending = true;
    g_svs_pend_w = khv_td.Width;
    g_svs_pend_h = khv_td.Height;
}

inline void shadow_track_targets(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv, bool dsv_is_main,
                                 UINT n, ID3D11RenderTargetView* const* rtvs) {
    // Seam tracking runs first and unconditionally - independent of
    // shadow_live_wanted, which gates the cascade machinery below.
    kh_volume_seam_track(ctx, dsv, dsv_is_main, n, rtvs);
    if (!shadow_live_wanted()) {
        if (g_sr.phase_active) {
            g_sr.phase_active = false;
            g_sr.target_identity = nullptr;
        }

        g_ls.phase_on_atlas = false;
        g_ls.pending_valid = false;
        g_ls.pending_bytes = 0;
        g_ls.pending_vp_valid = false;   // KH_LS_VP_LATCH: the atlas phase ends here with no finalize.
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

// KH_FOG_PROBE: the probes' demand on the funnel when no mesh opens it (kh_cbc_on). Locked, a probe takes its own
// buffer only; unlocked, the light probe hunts every map inside its attempt window (the ~120 ms of each second
// locator_note_upload scans in) and the sky probe every map until it locks.
inline bool kh_fog_probe_admits(ID3D11Resource* khfp_res) {
    if (!g_fogsc_wanted.load(std::memory_order_relaxed)) return false;
    const void* khfp_r = static_cast<void*>(khfp_res);
    if (g_sky_probe.valid ? khfp_r == g_sky_probe.buf : true) return true;
    if (g_light_probe.valid) return khfp_r == g_light_probe.buf;
    const float khfp_now = effect_time_seconds();
    return khfp_now - floorf(khfp_now) < 0.12f;
}

inline void kh_svs_bracket_post(ID3D11DeviceContext* self) {
    // Context test first: g_svs_bracket is a render-thread byte, and this runs after every draw of the four core draw
    // hooks (not DrawAuto or the indirect draws) on every thread using the vtable.
    if (self != g_reorder_target_ctx.load(std::memory_order_relaxed)) return;
    if (g_svs_bracket != 2) return;
    if (g_ro.in_injection) return;
    g_svs_bracket = 0;
    if (kh_svs_snap_wanted()) kh_svs_mask_snap(self, true);
}

inline void kh_svs_prime_mask(ID3D11DeviceContext* ctx) {
    KH_PROF_SCOPE(KHP_SVS_PRIME);
    KH_GPU_SCOPE(ctx, KHG_SVS_PRIME);   // KH_PROF.
    if (!ctx || g_ro.in_injection) return;

    // No mode test here.
    if (!g_res.ps_maskprime || !g_res.input_layout || !g_res.vs) {  return; }
    if (!g_svs_prime_ready) {  return; }
    if (g_svs_caster_list.empty()) {  return; }
    if (!g_mask.engine_mask_rtv) {  return; }

    ID3D11Resource* khp_res = nullptr;
    g_mask.engine_mask_rtv->GetResource(&khp_res);
    const bool khp_same = khp_res && g_svs_mask_id &&
                          static_cast<void*>(khp_res) == g_svs_mask_id;
    if (khp_res) khp_res->Release();
    if (!khp_same) {  return; }

    ID3D11Device* khp_dev = nullptr;
    ctx->GetDevice(&khp_dev);
    const bool khp_st = kh_svs_prime_states(khp_dev);
    if (khp_dev) khp_dev->Release();
    if (!khp_st) {  return; }

    const bool khp_prev = g_ro.in_injection;
    g_ro.in_injection = true;
    StateBackup khp_bk;
    khp_bk.capture(ctx);
    KhOmSave khp_om;   // Full OM set (8 RTVs + DSV + the UAVs above them); the DSV is rebound below.
    khp_om.capture(ctx);
    ID3D11DepthStencilView* const khp_old_dsv = khp_om.dsv;
    UINT khp_nvp = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT khp_old_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    ctx->RSGetViewports(&khp_nvp, khp_old_vps);

    // The engine's own depth target stays bound: it still holds the depth the injection wrote, so the LESS_EQUAL test
    // is meant to land the write on exactly our surface (KH_SVS_OFF_UNMAINT: it does not - the z path and rasterizer
    // below are not the footprint's).
    ctx->OMSetRenderTargets(1, &g_mask.engine_mask_rtv, khp_old_dsv);
    D3D11_VIEWPORT khp_vp = {};
    khp_vp.Width = static_cast<float>(g_svs_mask_w);
    khp_vp.Height = static_cast<float>(g_svs_mask_h);
    // Use the latched priming range: reading g_ro.trig_vp live here puts the priming pass in a different window-z space
    // than the depth it tests against.
    khp_vp.MinDepth = g_svs_prime_vp_lo;
    khp_vp.MaxDepth = g_svs_prime_vp_hi;
    ctx->RSSetViewports(1, &khp_vp);

    ctx->IASetInputLayout(g_res.input_layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(g_res.vs, nullptr, 0);
    ctx->PSSetShader(g_res.ps_maskprime, nullptr, 0);
    ctx->GSSetShader(nullptr, nullptr, 0);
    ctx->HSSetShader(nullptr, nullptr, 0);
    ctx->DSSetShader(nullptr, nullptr, 0);
    if (g_res.rast_sun) ctx->RSSetState(g_res.rast_sun);
    ctx->OMSetDepthStencilState(g_mask.prime_dss, 0);
    const FLOAT khp_bf[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ctx->OMSetBlendState(g_mask.prime_blend, khp_bf, 0xFFFFFFFF);

    ID3D11Buffer* khp_cbs[2] = { g_res.composite_cb, g_res.composite_frame_cb };
    ctx->VSSetConstantBuffers(0, 2, khp_cbs);
    ctx->PSSetConstantBuffers(0, 2, khp_cbs);

    ConstantData khp_cbf = {};
    memcpy(khp_cbf.view_proj, g_svs_prime_vp, sizeof(khp_cbf.view_proj));

    // Drain the slot about to be reused - three frames old, so donotflush never stalls.
    const int khp_slot = g_mask.prime_occl_cursor;
    bool khp_q = !g_mask.prime_occl_failed && g_mask.prime_occl[khp_slot] != nullptr;

    if (khp_q && g_mask.prime_occl_inflight[khp_slot]) {
        UINT64 khp_px = 0;

        if (ctx->GetData(g_mask.prime_occl[khp_slot], &khp_px, sizeof(khp_px),
                         D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK) {
            g_mask.prime_occl_inflight[khp_slot] = false;
        } else {
            khp_q = false;
        }
    }

    if (kh_upload_frame_cb(ctx, g_res.composite_frame_cb, khp_cbf)) {
        UINT khp_stride = sizeof(MeshVertex);
        KhVbRef khp_bound_vb;   // KH_CLOTH; KH_SKIN_POOL: the pair.
        if (khp_q) ctx->Begin(g_mask.prime_occl[khp_slot]);

        for (const auto& c : g_svs_caster_list) {
            const KhVbRef khp_want = (c.mesh >= 0 &&
                static_cast<size_t>(c.mesh) < g_res.mesh_vb.size())
                ? kh_mesh_vb_ref(c.mesh, c.slot) : KhVbRef();   // KH_CLOTH.
            if (!khp_want) {
                continue;
            }

            if (khp_want != khp_bound_vb) {
                ctx->IASetVertexBuffers(0, 1, &khp_want.vb, &khp_stride, &khp_want.off);
                ctx->IASetIndexBuffer(g_res.mesh_ib[c.mesh], DXGI_FORMAT_R32_UINT, 0);
                khp_bound_vb = khp_want;
            }

            ConstantData khp_obj = {};
            khp_obj.center_size[0] = c.pos[0];
            khp_obj.center_size[1] = c.pos[2];
            khp_obj.center_size[2] = c.pos[1];
            khp_obj.size_axes[0] = c.size[0];
            khp_obj.size_axes[1] = c.size[2];
            khp_obj.size_axes[2] = c.size[1];
            kh_fill_obj_rot(khp_obj, c.rot);
            if (g_svs_prime_rebase) kh_fill_center_rel(khp_obj, g_svs_prime_cam, c.res);   // KH_POS_RES.

            if (!kh_upload_obj_cb(ctx, g_res.composite_cb, khp_obj)) {
                break;
            }

            {   // KH_SHADOW_LOD_DRAWN.
                UINT khsl_is = 0, khsl_ic = 0;
                mesh_lod_range_of(c.mesh, c.lod, khsl_is, khsl_ic);
                ctx->DrawIndexed(khsl_ic, khsl_is, 0);
            }
        }

        if (khp_q) {
            ctx->End(g_mask.prime_occl[khp_slot]);
            g_mask.prime_occl_inflight[khp_slot] = true;
            g_mask.prime_occl_cursor = (khp_slot + 1) % 3;
        }

    }

    khp_om.restore(ctx);
    if (khp_nvp > 0) ctx->RSSetViewports(khp_nvp, khp_old_vps);
    khp_bk.restore(ctx);
    khp_om.release();
    g_ro.in_injection = khp_prev;
}

inline void kh_svs_vol_copy(ID3D11DeviceContext* khc_ctx) {
    KH_PROF_SCOPE(KHP_SVS_VOL_COPY);
    if (g_svs_vol_seq == g_svs_frame_seq) {
        return;
    }
    // KH_PROF_ONCE: past the return above (this seam frame's copy is taken; the seam frame is the GPU zone's cycle -
    // both turn at the main-depth clear), as kh_dls_frame's: a later call's empty return no longer replaces it.
    KH_GPU_SCOPE(khc_ctx, KHG_SVS_VOL_COPY);

    if (!khc_ctx || g_ro.in_injection) {  return; }
    if (!kh_svs_vol_on()) {  return; }
    if (!g_svs_vol_src) {  return; }
    // No visible mesh, no reader: the copy is refused and the primed flag dropped, so kh_svs_vol_ready cannot hand a
    // mesh that appears next frame a copy from before it existed.
    if (!g_svs_mesh_wanted.load(std::memory_order_relaxed) || kh_shadows_off()) {   // KH_SHADOW_OFF: no reader.
        g_svs_vol_primed = false;
        return;
    }

    ID3D11Device* khc_dev = nullptr;
    khc_ctx->GetDevice(&khc_dev);
    const bool khc_ok = khc_dev && kh_svs_vol_ensure(khc_dev);
    if (khc_dev) khc_dev->Release();
    if (!khc_ok) {  return; }

    // The bracket state is not touched here: kh_svs_bracket_post consumes it.
    const bool khc_prev = g_ro.in_injection;
    g_ro.in_injection = true;

    // KH_OM_SAVE_VOL: the engine's full OM set - render targets, depth and the unordered-access views behind them -
    // saved and put back through KhOmSave, as every target swap is (a plain get / set pair dropped the UAVs of the
    // engine draw this runs ahead of).
    KhOmSave khc_om;
    khc_om.capture(khc_ctx);
    // KH_PRED_ASIDE: a predicate the engine left bound at this draw would skip the copy, and the seam and replay
    // would read the last frame's counts (g_svs_vol_seq is stamped below regardless).
    KhPredAside khc_pa;
    khc_pa.begin(khc_ctx);

    khc_ctx->OMSetRenderTargets(0, nullptr, nullptr);
    khc_ctx->CopyResource(g_svs_vol_tex, g_svs_vol_src);
    khc_pa.end();
    khc_om.restore(khc_ctx);
    khc_om.release();
    g_ro.in_injection = khc_prev;   // Cleared after the restore, deliberately.
    g_svs_vol_primed = true;
    // The epoch this copy belongs to.
    g_svs_vol_seq = g_svs_frame_seq;
    // KH_VOL_WITNESS: the encode of this seam frame's footprint, or none.
    g_svs_vol_enc_ok = g_svs_seam_enc_seq == g_svs_frame_seq &&
                       g_svs_seam_enc[3] > g_svs_seam_enc[2] && g_svs_seam_enc[1] != 0.0f;
    memcpy(g_svs_vol_enc, g_svs_seam_enc, sizeof(g_svs_vol_enc));
    // KH_VOL_ZOOM: that seam frame's projection, or none (identity).
    g_svs_vol_proj_ok = g_svs_seam_enc_seq == g_svs_frame_seq && g_svs_seam_proj_ok;
    memcpy(g_svs_vol_proj, g_svs_seam_proj, sizeof(g_svs_vol_proj));
    // KH_VOL_FOOT: the mask this seam frame drew is this copy's; otherwise no mask (the witness then decodes the depth
    // plane). Same pixel grid as the copy, too: the witness addresses both with one pixel (a resize can land the seam's
    // mask and the copy on different sizes for a frame).
    g_svs_vol_foot_ok = g_svs_foot_seam_seq == g_svs_frame_seq && g_svs_foot_seam_ix >= 0 &&
                        g_svs_foot_seam_ix <= 1 && g_svs_foot_srv[g_svs_foot_seam_ix] != nullptr &&
                        g_svs_foot_w == g_svs_vol_w && g_svs_foot_h == g_svs_vol_h;
    if (g_svs_vol_foot_ok) g_svs_foot_pub = g_svs_foot_seam_ix;
}

inline void kh_volume_seam_pump(ID3D11DeviceContext* ctx) {
    if (g_ro.in_injection) return;

    // The copy is pumped separately from the injection: a mask hunt gated behind the injection arming never finds it.
    if (g_svs_bracket == 1) {
        g_svs_bracket = 2;

        // The volume copy first and on its own arm: same position requirement as pre (after the counting draws, before
        // the resolve), separate demand gate, separate lanes.
        kh_rp_pass_end(ctx);   // KH_VOL_REPLAY: the rewritable buffers, before anything rewrites them.
        kh_svs_vol_copy(ctx);
        // The bracket is the stencil resolve, a 4-vertex fullscreen quad with no view matrix to read.

        if (kh_svs_snap_wanted()) {
            // Prime first, then snapshot: the only point in the frame that is after the engine's mask clear and all
            // eight cascade resolves, and before pre is read.
            if (kh_svs_prime_wanted()) kh_svs_prime_mask(ctx);
            kh_svs_mask_snap(ctx, false);
        }
    }

    if (!g_svs_pending) return;
    g_svs_pending = false;
    kh_volume_seam_inject(ctx, g_svs_pend_w, g_svs_pend_h);
}
}