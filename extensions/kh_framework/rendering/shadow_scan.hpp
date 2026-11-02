#pragma once

// The injection half (rendering_integration_injection.hpp): the engine shadow scan: the live atlas, the view scan, the
// register reads, the cascade binding and the device-state release. Never include it on its own.
namespace RenderIntegration {

// Releases every shadow/mask/fire device object living outside Resources (defined later).
inline void release_shadow_device_state();
inline void kh_hook_desc_cache_release();   // KH_HOOK_DESC_CACHE (defined with the OM hooks).
// Forward: KH_PIP_DEVRESET - the PIP state is declared with the PIP block (rendering/pip.hpp).
inline void kh_pip_reset();
// Forward: KH_INFRONT - the slice's deferred colour tail holds the pass's targets; declared with the slice.
inline void kh_infront_tail_release();

// The engine-camera locator's weak-identity reset (its g_engcam_* state block, before the volume seam inject);
// called from release_shadow_device_state like g_proj_locator.
inline void kh_engcam_device_reset();
inline void locator_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes);

// Forward: defined with the live-capture group below.
inline void shadow_live_consider_atlas(ID3D11Texture2D* tex, const D3D11_TEXTURE2D_DESC& td);

inline void kh_rp_release_all();   // KH_VOL_REPLAY (defined with the replay).

// Defined in rendering/present_hook.hpp (declared here for the uses below).
inline void kh_present_rtv_release();
// Defined in rendering/upload_funnel.hpp (declared here for the uses below).
inline void kh_upload_copy_range(void* khur_dst, const void* khur_d, uint32_t khur_off, uint32_t khur_n);
inline void kh_upload_width_cache_release();


// Both receive gates consume this constant.
static constexpr float KH_RECEIVE_WARMUP_S = 2.0f;


// KH_REG_SLICE - the shadow registry's one reader (resolve_pair_capture, and band_capture from it) reads a record
// only when it holds KH_REG_READ_MIN floats or more, and then floats 176..199 alone: the cast fov (176..179), the
// sampling matrix (180..191) and the band border (196..199). A change to what either reads changes these.
static constexpr uint32_t KH_REG_READ_MIN = 200u;
static constexpr uint32_t KH_REG_SLICE_LO = 176u;
static constexpr uint32_t KH_REG_SLICE_N = 24u;
// KH_FOG_PROBE: the cycle (g_topo_cycles) whose camera g_ls.cam holds. The injection records it; the scene flush
// records its own on a cycle the injection did not, so a session with no injected mesh (translucent only, effects only)
// still hands the fog probe its camera altitude - without one the probe never locks and the engine-fog lanes stay cold
// for the session.
static uint64_t g_ls_cam_cycle = ~0ull;
static constexpr uint64_t KH_RECV_HEALTH_DWELL_MS = 500ull;
static bool     g_recv_health_ok = false;   // one-way latch (session state).
static uint64_t g_recv_health_since_ms = 0;   // Dwell start (0 = not running).

// A one-way latch: mid-session lock churn must not blink the world shadow out,
// and the forced-drop path wipes the tables itself. Absent beats foreign.
inline bool kh_recv_health() {
    if (g_recv_health_ok) return true;
    const bool khrh_ok = g_ls.view_src_valid &&
                         g_ls.pub_fresh_ms != 0 &&
                         steady_now_ms() - g_ls.pub_fresh_ms <= 1000 &&
                         g_sun_valid;

    if (!khrh_ok) { g_recv_health_since_ms = 0; return false; }

    const uint64_t khrh_now = steady_now_ms();

    if (g_recv_health_since_ms == 0) { g_recv_health_since_ms = khrh_now; return false; }
    if (khrh_now - g_recv_health_since_ms < KH_RECV_HEALTH_DWELL_MS) return false;

    g_recv_health_ok = true;

    for (int khrh_b = 0; khrh_b < 8; ++khrh_b) {
        if (!g_ls.band[khrh_b].valid) continue;
        g_ls.band[khrh_b].valid = false;
        g_ls.band[khrh_b].pending_view = false;
        g_ls.band[khrh_b].vcol_bridge = false;
    }

    return true;
}

// Shadow phase tracking (render thread only, like ReorderState).
static constexpr float    KH_SHADOW_AXIS_COS_TOL   = 0.995f;   // ~5.7 degrees off the sun vector.
static constexpr float    KH_SHADOW_ORTHO_TOL      = 0.02f;   // Orthonormality slack for the 3x3
                                                              // test.

struct ShadowReconState {
    bool     phase_active = false;   // depth-only, non-main DSV currently bound.
    void*    target_identity = nullptr;
};
static ShadowReconState g_sr;

// Absolute cosine between a (possibly scaled) axis and the published sun.
inline float shadow_axis_abs_cos(float ax, float ay, float az) {
    const float len2 = ax * ax + ay * ay + az * az;
    if (len2 < 1e-12f) return 0.0f;
    const float d = ax * g_sun_dir_engine[0] + ay * g_sun_dir_engine[1] + az * g_sun_dir_engine[2];
    return fabsf(d) / sqrtf(len2);
}

// Cleared at every frame boundary and on atlas adoption, so a destroyed view whose address is reused can never inherit
// a stale classification beyond the frame it died in.
static void* g_atlas_srv_cache[8] = {};
static uint32_t g_atlas_srv_count = 0;

// Every band slot's device objects and its seal, then the staging pool.
inline void kh_band_slots_release() {
    for (int b = 0; b < 8; ++b) {
        auto& bb = g_ls.band[b];
        if (bb.srv) { bb.srv->Release(); bb.srv = nullptr; }
        if (bb.tex) { bb.tex->Release(); bb.tex = nullptr; }
        bb.valid = false;
        bb.pending_view = false;
        memset(bb.sm, 0, sizeof(bb.sm));
        memset(bb.border, 0, sizeof(bb.border));
        bb.last_time = 0.0f;
    }
    kh_band_pool_release();
}

inline void shadow_live_consider_atlas(ID3D11Texture2D* tex, const D3D11_TEXTURE2D_DESC& td) {
    uint32_t khsc_clause = 0;

    if ((td.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0) {
        khsc_clause = 1;
    } else if (td.Width != td.Height || td.Width < 1024) {
        khsc_clause = 2;
    } else if (td.SampleDesc.Count > 1) {
        khsc_clause = 3;
    } else if (td.ArraySize != 1) {
        khsc_clause = 4;
    } else if (td.Format != DXGI_FORMAT_R32_TYPELESS && td.Format != DXGI_FORMAT_R24G8_TYPELESS &&
               td.Format != DXGI_FORMAT_R16_TYPELESS && td.Format != DXGI_FORMAT_R32G8X24_TYPELESS) {
        khsc_clause = 5;
    }

    if (khsc_clause != 0) return;

    void* id = static_cast<void*>(tex);
    if (id == g_ls.atlas_identity) {  return; }
    // Grow-or-recover adoption: 'keep the largest' alone is a one-way latch - a mid-session shadow-quality change
    // recreates the engine atlas and a same-size-or-smaller successor could never displace the held one.
    const float now_at = effect_time_seconds();
    const bool held_dead = g_ls.atlas_tex && (now_at - g_ls.atlas_last_seen > 3.0f);
    if (g_ls.atlas_tex && td.Width <= g_ls.atlas_size && !held_dead) {
        return;   // Keep the largest live atlas.
    }

    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); }
    tex->AddRef();
    g_ls.atlas_tex = tex;

    g_ls.atlas_identity = id;
    g_ls.atlas_size = td.Width;
    g_ls.atlas_fmt = static_cast<uint32_t>(td.Format);
    g_atlas_srv_count = 0;
    g_ls.atlas_last_seen = now_at;   // Adoption grace: displaceable only after 3 s unseen.
    g_ls.srv_failed = false;
    g_ls.cache_valid = false;   // New atlas: relearn the upload locator.
    g_atlas_srv_count = 0;

    // Drop the textures; band_capture's batch-create rebuilds all eight at the adopted desc on the first reseal.
    kh_band_slots_release();
}

inline bool shadow_live_test_window(const float* w) {
    const float* r0 = w + 0;
    const float* r1 = w + 4;
    const float* r2 = w + 8;
    const float n0 = r0[0] * r0[0] + r0[1] * r0[1] + r0[2] * r0[2];
    const float n1 = r1[0] * r1[0] + r1[1] * r1[1] + r1[2] * r1[2];
    const float n2 = r2[0] * r2[0] + r2[1] * r2[1] + r2[2] * r2[2];
    if (n0 < 1e-12f || n1 < 1e-12f || n2 < 1e-12f) return false;

    // Scale window: every genuine sampling transform observed sits at 1/160 .. 1/24 per metre (plus a 1/600 long-range
    // one); [~1/3160, 1/8] keeps that family with margin and rejects the 1/2.2-scale look-alikes.
    if (n0 > 0.015625f || n1 > 0.015625f) {
        return false; }   // uv scale <= 1/8.
    if (n0 < 1e-7f || n1 < 1e-7f) {
        return false; }   // Low sun stretches extents; uv scale >= ~1/3160 (norm^2 1e-7).
    // Depth scale within a factor ~20 of the uv scale (observed ~0.5-2x).
    if (n2 < 0.0025f * n0 || n2 > 400.0f * n0) {
        return false; }
    // u/v isotropy (norms-squared within ~10% = norms within ~5%).
    if (fabsf(n1 - n0) > 0.1f * n0) {
        return false; }
    if (w[3] < -8.0f || w[3] > 9.0f) {
        return false; }
    if (w[7] < -8.0f || w[7] > 9.0f) {  return false; }
    if (w[11] < -8.0f || w[11] > 9.0f) {  return false; }
    const float d01 = r0[0] * r1[0] + r0[1] * r1[1] + r0[2] * r1[2];
    const float d02 = r0[0] * r2[0] + r0[1] * r2[1] + r0[2] * r2[2];
    const float d12 = r1[0] * r2[0] + r1[1] * r2[1] + r1[2] * r2[2];

    if (fabsf(d01) > KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n1) ||
        fabsf(d02) > KH_SHADOW_ORTHO_TOL * sqrtf(n0 * n2) ||
        fabsf(d12) > KH_SHADOW_ORTHO_TOL * sqrtf(n1 * n2)) {
        return false;
    }

    if (fabsf(r0[0] * r0[0] - n0) < KH_SHADOW_ORTHO_TOL * n0 &&
        fabsf(r1[1] * r1[1] - n1) < KH_SHADOW_ORTHO_TOL * n1 &&
        fabsf(r2[2] * r2[2] - n2) < KH_SHADOW_ORTHO_TOL * n2) {
        return false;
    }

    for (int a = 0; a < 6; ++a) {
        float x, y, z;
        if (a < 3)  { const float* r = w + a * 4; x = r[0]; y = r[1]; z = r[2]; }
        else        { const int c = a - 3; x = r0[c]; y = r1[c]; z = r2[c]; }
        if (shadow_axis_abs_cos(x, y, z) >= KH_SHADOW_AXIS_COS_TOL) {

            return true;
        }
    }

    return false;
}

inline bool shadow_view_shaped(const float* w) {
    if (fabsf(w[15] - 1.0f) > 1e-3f) return false;
    const float n0 = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
    const float n1 = w[4] * w[4] + w[5] * w[5] + w[6] * w[6];
    if (n0 < 0.9f || n0 > 1.1f || n1 < 0.9f || n1 > 1.1f) return false;
    const float d01 = w[0] * w[4] + w[1] * w[5] + w[2] * w[6];
    return fabsf(d01) < 0.1f;
}

inline void shadow_view_scan(ID3D11Resource* res, const void* data, uint32_t bytes, uint32_t base_off = 0) {
    // base_off (floats): nonzero when the data is a window into the buffer (the binding-readback path); offsets stored
    // and matched below are canonical buffer offsets, so both supply paths feed one lock.
    if (!reorder_on_render_thread() || bytes < 64 || !res) return;

    if (g_acq_hold.load(std::memory_order_relaxed)) {   return; }

    // Locked fast path: with a source locked, the publish branch requires res == view_src_res and the admission branch
    // requires no lock, so for every foreign resource the offset loop is a guaranteed no-op.
    if (g_ls.view_src_valid && res != g_ls.view_src_res) {  return; }
    const float* f = static_cast<const float*>(data);
    const uint32_t nf = bytes / 4;
    kh_upload_need(data, (nf < 208u ? nf : 208u) * 4u);   // KH_UPLOAD_LAZY: offsets 0..192, 16 floats each.

    for (uint32_t off = 0; off + 16 <= nf && off <= 192; off += 4) {
        const float* w = f + off;

        // A genuinely wrong first lock never passes it.
        if (!g_ls.view_src_valid && res == g_ls.view_src_res &&
            off + base_off == g_ls.view_src_off &&
            g_ls.view_src_relative && g_ro.cycle_pv_valid) {
            float pf[16];

            if (g_ls.view_src_orient == 1) {
                memcpy(pf, w, sizeof(pf));
            } else {
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        pf[r * 4 + c] = w[c * 4 + r];
                    }
                }
            }

            const float khr_tmag = fabsf(pf[12]) + fabsf(pf[13]) + fabsf(pf[14]);

            if (khr_tmag <= 0.05f) {
                const float* bv = &g_ro.cycle_pv.view[0][0];
                float khr_er = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) {
                        khr_er += fabsf(pf[r * 4 + c] - bv[r * 4 + c]);
                    }
                }

                bool khr_take = khr_er <= 1e-3f;

                if (!khr_take && g_view_drop_path == 1 && g_view_drop_bar > 0.0f && khr_er <= g_view_drop_bar) {
                    if (g_view_relock_wide_cycle != 0 &&
                        g_view_relock_wide_cycle + 1 == g_topo_cycles) {
                        khr_take = true;
                    }

                    g_view_relock_wide_cycle = g_topo_cycles;
                }

                if (khr_take) {
                    g_ls.view_src_valid = true;
                    kh_view_lock_acquired();
                    g_ls.view_src_miss = 0;
                    g_ls.pub_rej_streak = 0;
                    g_ls.pub_ok_since_lock = false;
                    g_ls.count = 0;
                    g_ls.newest = -1;
                    g_lock_churn_ms_v = steady_now_ms();   // lock-settle: hold the cast through the
                                                           // rebuild.
                    g_ls.pub_exact_ms = steady_now_ms();
                    g_ls.pub_fresh_ms = g_ls.pub_exact_ms;
                }
            }
        }

        if (g_ls.view_src_valid && res == g_ls.view_src_res && off + base_off == g_ls.view_src_off) {
            float fv[16];

            if (g_ls.view_src_orient == 1) {
                memcpy(fv, w, sizeof(fv));
            } else {
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        fv[r * 4 + c] = w[c * 4 + r];
                    }
                }
            }

            if (g_ls.view_src_relative) {
                const float tmag = fabsf(fv[12]) + fabsf(fv[13]) + fabsf(fv[14]);

                if (!(tmag <= 0.05f)) {   // An object, not the view. KH_VIEW_NAN: positive form, NaN refused.

                    continue;
                }

                if (g_ro.cycle_pv_valid) {
                    const float* bv = &g_ro.cycle_pv.view[0][0];
                    float e_rot = 0.0f;

                    for (int r = 0; r < 3; ++r) {
                        for (int c = 0; c < 3; ++c) {
                            e_rot += fabsf(fv[r * 4 + c] - bv[r * 4 + c]);
                        }
                    }

                    // Family filter only: non-view families differ by >> 1. A tighter bar is speed-fragile (the bridge
                    // lags a frame; a fast spin exceeds it and the health check evicts the lock).
                    if (!(e_rot <= 0.35f)) {   // KH_VIEW_NAN: positive form, a NaN rotation is refused.

                        continue;
                    }

                    g_ls.last_publish_rot_err = e_rot;   // Cold fire quality gate.
                    if (e_rot <= KH_LIT_FRESH_ERR) g_ls.pub_fresh_ms = steady_now_ms();
                    if (e_rot <= 1e-3f) g_ls.pub_exact_ms = steady_now_ms();
                }
            }

            if (g_ls.view_src_relative && g_ro.cycle_pv_valid) {
                const float* bv = &g_ro.cycle_pv.view[0][0];
                float khpv_rd = 0.0f;

                for (int r = 0; r < 3; ++r) {
                    for (int c = 0; c < 3; ++c) khpv_rd += fabsf(fv[r * 4 + c] - bv[r * 4 + c]);
                }

                if (khpv_rd < 1.0e-5f) {
                    fv[12] = bv[12];
                    fv[13] = bv[13];
                    fv[14] = bv[14];
                } else {
                    float khpv_hc[3];
                    kh_cam_of(g_ro.cycle_pv.view, khpv_hc);   // KH_CAM_HONEST: bv's camera, not its transpose.
                    const double cam[3] = { khpv_hc[0], khpv_hc[1], khpv_hc[2] };

                    for (int c = 0; c < 3; ++c) {
                        fv[12 + c] = static_cast<float>(
                            -(cam[0] * fv[0 * 4 + c] + cam[1] * fv[1 * 4 + c] + cam[2] * fv[2 * 4 + c]));
                    }
                }

                fv[15] = 1.0f;
            }

            {
                const float pub_now = effect_time_seconds();
                if (g_pub_first < 0.0f) g_pub_first = pub_now;

                if (pub_now - g_pub_first < 3.0f) {
                    RVExtBridge::ProjectionViewTransform bpv = {};

                    if (RVExtBridge::get_projection_view_transform(bpv)) {
                        float bvv[16];
                        memcpy(bvv, &bpv.view, sizeof(bvv));
                        float rd = 0.0f;
                        float md = 0.0f;   // Rotation moved since the last accepted publish.

                        for (int rr = 0; rr < 3; ++rr) {
                            for (int cc = 0; cc < 3; ++cc) {
                                rd += fabsf(fv[rr * 4 + cc] - bvv[rr * 4 + cc]);
                                if (g_ls.prev_view_valid) md += fabsf(fv[rr * 4 + cc] - g_ls.prev_view[rr * 4 + cc]);
                            }
                        }

                        const float khpk_barv = 0.02f + (md < 0.3f ? md : 0.3f);

                        if (rd > khpk_barv) {

                            const uint32_t khpk_bar = g_ls.pub_ok_since_lock ? 40u : 8u;

                            if (++g_ls.pub_rej_streak >= khpk_bar && g_ls.view_src_valid) {
                                const bool khpk_hard =
                                    rd > khpk_barv * KH_PUB_DROP_HYST;
                                const uint64_t khpk_now = steady_now_ms();

                                if (!khpk_hard) {
                                    if (g_view_drop_hyst_since == 0) {
                                        g_view_drop_hyst_since = khpk_now;
                                    }

                                    const uint64_t khpk_held =
                                        khpk_now - g_view_drop_hyst_since;

                                    if (khpk_held < KH_PUB_DROP_HOLD_MS) {
                                        return;   // Full no-op: the lock survives.
                                    }

                                }

                                g_view_drop_path = 1;
                                g_view_drop_bar = khpk_barv;
                                g_view_relock_wide_cycle = 0;
                                g_ring_best_valid = false;
                                g_ls.view_src_valid = false;
                                g_ls.vc_n = 0;
                                g_ls.pub_rej_streak = 0;
                                g_view_drop_hyst_since = 0;
                                g_lock_churn_ms_v = steady_now_ms();   // lock-settle: the dropped
                                                                       // lock's.
                                g_ls.count = 0;
                                g_ls.newest = -1;

                                for (int khpk_b = 0; khpk_b < 8; ++khpk_b) {
                                    g_ls.band[khpk_b].valid = false;
                                    g_ls.band[khpk_b].pending_view = false;
                                }

                            }

                            return;   // Full no-op: no publish state was touched.
                        }

                        g_ls.pub_rej_streak = 0;
                        g_view_drop_hyst_since = 0;   // The run is broken.
                        g_ls.pub_ok_since_lock = true;
                    }
                }
            }

            if (g_ls.frame_view_valid) {
                memcpy(g_ls.prev_view, g_ls.frame_view, sizeof(g_ls.prev_view));
                g_ls.prev_view_valid = true;
                // When the incoming publish crosses sources, the outgoing frame_view is the other source's freshest
                // statement - latch it.
                float khav_a[3], khav_b[3];
                if (kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(g_ls.frame_view), khav_a) &&
                    kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(fv), khav_b)) {
                    const float khav_dx = khav_a[0] - khav_b[0];
                    const float khav_dy = khav_a[1] - khav_b[1];
                    const float khav_dz = khav_a[2] - khav_b[2];
                    const float khav_d = sqrtf(khav_dx * khav_dx + khav_dy * khav_dy +
                                               khav_dz * khav_dz);
                    if (khav_d == khav_d && khav_d > 0.5f) {
                        memcpy(g_ls.alt_view, g_ls.frame_view, sizeof(g_ls.alt_view));
                        g_ls.alt_view_valid = true;
                        g_ls.alt_view_time = g_ls.frame_view_time;
                    }
                }
            }

            memcpy(g_ls.frame_view, fv, sizeof(g_ls.frame_view));
            g_ls.frame_view_valid = true;
            g_ls.frame_view_time = effect_time_seconds();
            g_ls.view_published_this_frame = true;

            // Every publish arms the fire (once per frame: not after it has fired).
            if (!g_mask_cast_fired) {
                g_mask_cast_arm = true;   // Once per frame (engine-mask path).
            }

            // Complete any seals waiting for this frame's view.
            for (int bi = 0; bi < 8; ++bi) {
                auto& bs = g_ls.band[bi];
                if (!bs.valid || !bs.pending_view) continue;

                if (g_ls.last_publish_rot_err > 1e-3f) {
                    continue;
                }

                // Epoch guard: a pending seal older than this frame would pair a cross-frame view - a permanent receive
                // offset. Invalidate; the next resolve reseals with a same-frame pair.
                if (g_ls.frame_view_time - bs.pending_since > 0.05f) {
                    bs.valid = false;
                    bs.pending_view = false;
                    bs.last_time = 0.0f;
                    continue;
                }

                if (!kh_band_view_native(fv, true)) {   // Foreign publish never completes a seal.
                    continue;
                }
                // Absent beats offset: invalidate and the next resolve reseals coherent. A completing publish > 50 m
                // from the capture's own view would weld sm(A) to view(B).
                if (!bs.vcol_bridge &&
                    !kh_band_pair_coherent(bs.vcol, fv)) {
                    bs.valid = false;
                    bs.pending_view = false;
                    continue;
                }
                // Bridge provisionals are exempt: they cannot finalize and never serve unfinalized, so refusing would
                // only starve them.
                for (int c = 0; c < 3; ++c) {
                    for (int r = 0; r < 4; ++r) {
                        bs.vcol[c * 4 + r] = fv[r * 4 + c];
                    }
                }

                bs.pending_view = false;
                bs.vcol_bridge = false;
            }

            if (g_ls.last_publish_rot_err <= 1e-3f && kh_band_view_native(fv, false)) {
                for (int khsf_i = 0; khsf_i < 8; ++khsf_i) {
                    auto& khsf_b = g_ls.band[khsf_i];
                    if (!khsf_b.stage_valid || khsf_b.stage_first_ok) continue;
                    for (int c = 0; c < 3; ++c) {
                        for (int r = 0; r < 4; ++r) {
                            khsf_b.stage_first[c * 4 + r] = fv[r * 4 + c];
                        }
                    }
                    khsf_b.stage_first_ok = true;
                }
            }
            if (g_ls.last_publish_rot_err <= 1e-3f && kh_band_view_native(fv, false)) {
                for (int khfl_i = 0; khfl_i < 8; ++khfl_i) {
                    auto& khfl_b = g_ls.band[khfl_i];
                    if (!khfl_b.stage_valid || khfl_b.stage_pair_ok ||
                        !khfl_b.stage_view_ok) continue;
                    if (g_boundary_t >= 0.0f &&
                        khfl_b.stage_since >= g_boundary_t) continue;
                    const float khfl_d = kh_band_source_gap(khfl_b.stage_view, fv);
                    if (khfl_d > 0.5f) {
                        khfl_b.stage_pair_ok = true;
                    }
                }
            }

            return;
        }

        if (!g_ls.view_src_valid && shadow_view_shaped(w)) {
            // A tab-out stalls uploads past the health limit; the true view must be able to relearn from the ring
            // afterwards.
            const float tmag_r = fabsf(w[12]) + fabsf(w[13]) + fabsf(w[14]);
            const float tmag_c = fabsf(w[3]) + fabsf(w[7]) + fabsf(w[11]);

            if (tmag_r < 10.0f || tmag_c < 10.0f) {
                const uint32_t i = g_ls.vc_n & 15;
                g_ls.vc_ring[i].res = res;
                g_ls.vc_ring[i].off = off + base_off;
                memcpy(g_ls.vc_ring[i].m, w, 64);
                g_ls.vc_n++;

                // The ring keeps the last sixteen; this keeps the best one, so the injection's position stops deciding
                // what the hunt gets to see. Both orientations are scored because the convention is not known until the
                // lock is taken.
                if (g_ro.cycle_pv_valid) {
                    const float* khb_t = &g_ro.cycle_pv.view[0][0];

                    for (int khb_o = 1; khb_o <= 2; ++khb_o) {
                        float khb_e = 0.0f;

                        for (int r = 0; r < 3; ++r) {
                            for (int c = 0; c < 3; ++c) {
                                const float khb_v = (khb_o == 1) ? w[r * 4 + c]
                                                                 : w[c * 4 + r];
                                khb_e += fabsf(khb_v - khb_t[r * 4 + c]);
                            }
                        }

                        if (!kh_ring_best_fresh() || khb_e < g_ring_best_er) {   // KH_RING_BEST_AGE.
                            g_ring_best_valid = true;
                            g_ring_best_er = khb_e;
                            g_ring_best_res = static_cast<const void*>(res);
                            g_ring_best_off = off + base_off;
                            g_ring_best_orient = static_cast<uint32_t>(khb_o);
                            g_ring_best_cycle = g_topo_cycles;
                            memcpy(g_ring_best_m, w, 64);
                        }
                    }
                }
            }
        }
    }
}

// KH_REG_SLICE: every upload of 48 bytes or more is recorded (its buffer, size and recency, and its place in the ring),
// but a record copies only the floats its reader reads, straight from the upload (kh_upload_copy_range, which extends
// no capture), and a record its reader never reads copies nothing.
inline void shadow_register_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 48) return;   // No size cap: big CBs hold the maps.
    auto khsr_keep = [&](LiveShadowState::CbRecord& khsr_r) {
        if (khsr_r.floats < KH_REG_READ_MIN) return;   // Never read: bytes >= 4 * KH_REG_READ_MIN below.
        kh_upload_copy_range(khsr_r.data + KH_REG_SLICE_LO, data, KH_REG_SLICE_LO * 4u, KH_REG_SLICE_N * 4u);
    };

    for (uint32_t i = 0; i < 16; ++i) {
        if (g_ls.cb_reg[i].buf == res) {
            g_ls.cb_reg[i].floats = bytes / 4 <= 512 ? bytes / 4 : 512;
            g_ls.cb_reg[i].seq = ++g_ls.cb_reg_seq;
            khsr_keep(g_ls.cb_reg[i]);
            return;
        }
    }

    {
        auto& r = g_ls.cb_reg[g_ls.cb_reg_next];
        g_ls.cb_reg_next = (g_ls.cb_reg_next + 1) & 15;
        r.seq = ++g_ls.cb_reg_seq;
        r.buf = res;
        r.floats = bytes / 4 <= 512 ? bytes / 4 : 512;
        khsr_keep(r);
    }
}

inline void locator_note_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!res || !data || bytes < 64) return;   // The block is 56+ floats.
    // Never self-learn: our own CBs carry the staged fog and camera too.
    if (res == static_cast<ID3D11Resource*>(g_res.composite_cb)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.constant_buffer)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.frame_cb)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.composite_frame_cb)) return;

    const float* f = static_cast<const float*>(data);
    const uint32_t nf_full = bytes / 4;
    const uint32_t nf = nf_full > 4096 ? 4096 : nf_full;

    // References: decay from the staged fog (nonzero even with fog off - RV's default is 0.014), altitude from the
    // injection-recorded render camera.
    const float decay = g_fog[1];
    const float cam_alt = g_ls.cam[1];
    const bool ref_ok = g_fog_valid && decay > 1.0e-4f && cam_alt != 0.0f;
    // Discovery and relocation are untouched.
    const bool khz_hold = !ref_ok && g_fog_valid && cam_alt != 0.0f && !(decay > 1.0e-4f) && g_light_probe.valid;

    if (g_light_probe.valid && res == g_light_probe.buf && (ref_ok || khz_hold)) {
        // KH_LOC_LAZY: a confirm at the locked offset reads below float max(off, 40) + 16 - the anchor (off .. off +
        // 15), the mode lane (off - 25), the block note and the fog mirror (off - 40 .. off + 15, or 0 .. 55 below
        // offset 40); only the re-find walks the whole buffer, so only it copies the whole buffer.
        kh_upload_need(data, ((g_light_probe.off > 40u ? g_light_probe.off : 40u) + 16u) * 4u);   // KH_UPLOAD_LAZY.
        bool ok = ref_ok
                ? locator_light_anchor(f, g_light_probe.off, nf, decay, cam_alt)
                : locator_zero_anchor(f, g_light_probe.off, nf, cam_alt);

        if (!ok && ref_ok) {
            kh_upload_need(data, nf * 4u);   // KH_UPLOAD_LAZY: the re-find.
            for (uint32_t i = 0; i + 16 <= nf; ++i) {
                if (locator_light_anchor(f, i, nf, decay, cam_alt)) {
                    g_light_probe.off = i;
                    ok = true;
                    break;
                }
            }
        }

        if (ok) {
            g_light_probe.hits++;
            g_light_probe.last_confirm = effect_time_seconds();
            // Confirms still stamp (the buffer is the block), but the mirror only refreshes on the standard layout, so
            // every consumer keeps the last-known standard lanes.
            const uint32_t mode_i = g_light_probe.off - 25;

            // Every standard-layout upload is an admitted upload - recorded and given its ownership span.
            if (mode_i < nf && f[mode_i] == 1.0f &&
                g_light_probe.off >= 40 && g_light_probe.off - 40 + 18 < nf) {
                kh_blk_note_admitted(f + (g_light_probe.off - 40), nf - (g_light_probe.off - 40), kh_probe_world_cycle());
            }

            // KH_LOC_MODE_GUARD: the same bound its neighbours take - mode_i wraps (uint32) when the lock sits below
            // offset 25.
            const bool khml_adm = mode_i < nf && f[mode_i] == 1.0f;

            if (mode_i < nf && khml_adm && kh_probe_world_cycle()) {
                kh_probe_mirror_refresh(g_light_probe, f, nf);   // The fog-lane mirror.
            }
        }
    }

    const float now = static_cast<float>(kh_fast_effect_seconds_d());   // KH_FAST_CLOCK: every captured upload.

    if (nf >= 72) {
        kh_upload_need(data, (nf < 96u ? nf : 96u) * 4u);   // KH_UPLOAD_LAZY: the anchor and locator_capture's 96.
        if (g_sky_probe.valid && now - g_sky_probe.last_confirm > 60.0f) {
            g_sky_probe.valid = false;   // Freshness flag only: the mirror and hits persist -
                                         // consumption is mirror-based (hits > 0).
        }

        if (locator_sky_anchor(f, nf)) {
            g_sky_probe.buf = res;
            g_sky_probe.off = 0;
            g_sky_probe.floats = nf_full;
            g_sky_probe.valid = true;
            g_sky_probe.hits++;
            g_sky_probe.last_confirm = now;
            locator_capture(g_sky_probe, f, nf, 96);
        }
    }

    // Attempt window (~120 ms each second): expiry, then the unlocked light scan.
    if (now - floorf(now) >= 0.12f) return;

    if (g_light_probe.valid && now - g_light_probe.last_confirm > 30.0f) {
        g_light_probe.valid = false;
        g_light_probe.buf = nullptr;
    }

    const bool want_light = !g_light_probe.valid && ref_ok;
    if (!want_light) return;
    kh_upload_need(data, nf * 4u);   // KH_UPLOAD_LAZY: the unlocked hunt.

    for (uint32_t i = 0; i + 16 <= nf; ++i) {
        if (locator_light_anchor(f, i, nf, decay, cam_alt)) {
            g_light_probe.buf = res;
            g_light_probe.off = i;
            g_light_probe.floats = nf_full;
            g_light_probe.valid = true;
            g_light_probe.hits = 1;
            g_light_probe.last_confirm = now;
            const uint32_t mode_i2 = g_light_probe.off - 25;
            if (mode_i2 < nf && f[mode_i2] == 1.0f &&
                g_light_probe.off >= 40 && g_light_probe.off - 40 + 18 < nf) {   // KH_LIGHT_BASE twin.
                kh_blk_note_admitted(f + (g_light_probe.off - 40), nf - (g_light_probe.off - 40), kh_probe_world_cycle());
            }

            if (mode_i2 < nf && f[mode_i2] == 1.0f && kh_probe_world_cycle()) {
                kh_probe_mirror_refresh(g_light_probe, f, nf);
            }
            break;
        }
    }
}

inline void shadow_live_upload(const void* data, uint32_t bytes) {
    if (!shadow_live_wanted() || !g_ls.phase_on_atlas) return;
    if (bytes < 12 * sizeof(float)) return;
    if (bytes > 65536) bytes = 65536;
    const float* f = static_cast<const float*>(data);
    const uint32_t nfloats = bytes / 4;

    // KH_LIVE_DEAD: a latched cycle keeps only its lazy copy and returns; the small sampling block's branch keeps only
    // its lazy copy (the scratch holds the same bytes for every reader after) and falls through to the hunt below:
    // shadow_live_test_window writes no state (it reads the window and the published sun; nothing reads errno or the FP
    // status), so a loop over it that keeps nothing would be dead.
    if (g_ls.cycle_latched) {
        kh_upload_need(data, (nfloats < 140u ? nfloats : 140u) * 4u);   // KH_UPLOAD_LAZY: offsets 0..128, 12 floats.
        return;
    }

    if (bytes <= 256) {
        kh_upload_need(data, bytes);   // KH_UPLOAD_LAZY (inside the eager copy).
    }

    if (g_ls.pending_valid && bytes < g_ls.pending_bytes) return;   // Never downgrade (same size:
                                                                    // last upload wins).
    // KH_LIVE_LAZY: the cache test and the hunt read the whole buffer; an upload refused above reads none of it.
    kh_upload_need(data, bytes);   // KH_UPLOAD_LAZY.

    if (g_ls.cache_valid && bytes == g_ls.cache_bytes && g_ls.cache_offset + 12 <= nfloats) {
        if (shadow_live_test_window(f + g_ls.cache_offset)) {
            memcpy(g_ls.pending_m, f + g_ls.cache_offset, sizeof(g_ls.pending_m));
            g_ls.pending_valid = true;
            g_ls.pending_bytes = static_cast<uint32_t>(bytes);
            return;
        }
    }

    for (uint32_t off = 0; off + 12 <= nfloats; off += 4) {
        if (shadow_live_test_window(f + off)) {
            g_ls.cache_valid = true;
            g_ls.cache_bytes = static_cast<uint32_t>(bytes);
            g_ls.cache_offset = off;
            memcpy(g_ls.pending_m, f + off, sizeof(g_ls.pending_m));
            g_ls.pending_valid = true;
            g_ls.pending_bytes = static_cast<uint32_t>(bytes);
            return;
        }
    }
}

inline void shadow_live_on_draw(ID3D11DeviceContext* ctx) {
    if (!g_ls.pending_valid || g_ls.pending_vp_valid || !g_ls.phase_on_atlas || !ctx || g_ls.atlas_size == 0) return;
    UINT nvp = 1;
    D3D11_VIEWPORT vp = {};
    ctx->RSGetViewports(&nvp, &vp);
    if (nvp == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f) return;
    g_ls.pending_vp[0] = vp.TopLeftX;
    g_ls.pending_vp[1] = vp.TopLeftY;
    g_ls.pending_vp[2] = vp.Width;
    g_ls.pending_vp[3] = vp.Height;
    g_ls.pending_vp_valid = true;
}

// Cycle exit: the fully-upgraded pending latch becomes a table entry.
inline void shadow_live_finalize_cycle() {
    if (!g_ls.pending_valid || !g_ls.pending_vp_valid || g_ls.atlas_size == 0) {
        // Any failing term drops the latch. KH_LS_VP_LATCH: every reset of the upload latch drops the viewport latch
        // with it, so no term carries a stale tile across cycles.
        g_ls.pending_valid = false;
        g_ls.pending_bytes = 0;
        g_ls.pending_vp_valid = false;
        return;
    }

    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_ls.pending_vp_valid = false;

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = g_ls.pending_vp[0];
    vp.TopLeftY = g_ls.pending_vp[1];
    vp.Width = g_ls.pending_vp[2];
    vp.Height = g_ls.pending_vp[3];

    // Refuse the commit and keep the table. The derivation still samples every window (band_capture): if the published
    // sun ever locked wrong, derivation keeps measuring truth and the snap + jump arbiter recover it.
    {
        const float* khlx_r2 = g_ls.pending_m + 8;
        const float khlx_now = effect_time_seconds();
        const bool khlx_fv = g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
                             khlx_now - g_ls.frame_view_time < 0.05f;

        if (!(khlx_fv || g_ro.cycle_pv_valid)) {
            // No view source: the commit is refused (the table keeps its entries); the entry's camera below
            // would rest on g_ls.cam alone.
            return;
        } else if (kh_shadow_sun()) {
            // KH_LIVE_AXIS_RAW: this matrix is world-axis - it maps camera-relative world positions (the table
            // fill folds only the entry's camera into its translation, ShadowMapSample takes absolute world
            // positions, shadow_live_test_window admitted it by a raw row or column near the published axis) - so its
            // depth row is the light axis as it stands. A fold through the view (band_capture's, whose resolve
            // matrix is view-space) refuses every commit whose view rotation turns the sun axis by more than
            // KH_SUN_AXIS_MAX_DEG. Sign-free like the admission (the depth row may point either way along the
            // axis); positive form, so a NaN row is refused. Fail-open while no sun is published, as
            // kh_sun_axis_foreign.
            const float khlx_c = shadow_axis_abs_cos(khlx_r2[0], khlx_r2[1], khlx_r2[2]);

            if (!(khlx_c >= cosf(KH_SUN_AXIS_MAX_DEG * 0.01745329252f))) {
                return;
            }
        }
    }

    // Slot selection: same cascade = same scale (each cascade's extent is distinct); a re-render replaces its entry in
    // place. New scales append; a full table evicts the stalest.
    const float* pm = g_ls.pending_m;
    const float pn0 = pm[0] * pm[0] + pm[1] * pm[1] + pm[2] * pm[2];
    if (g_ls.wipe_deferred) {   // KH_LIVE_WIPE_DEFER: the deferred wipe lands here.
        g_ls.count = 0;
        g_ls.newest = -1;
        g_ls.wipe_deferred = false;
    }
    int slot = -1;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        const float* m = g_ls.entries[i].m;
        const float n0 = m[0] * m[0] + m[1] * m[1] + m[2] * m[2];
        if (fabsf(n0 - pn0) <= 0.2f * n0) { slot = static_cast<int>(i); break; }
    }

    if (slot < 0) {
        if (g_ls.count < KH_LIVE_MAX_CASCADES) {
            slot = static_cast<int>(g_ls.count++);
        } else {
            uint64_t oldest = ~0ull;

            for (uint32_t i = 0; i < g_ls.count; ++i) {
                if (g_ls.entries[i].stamp < oldest) { oldest = g_ls.entries[i].stamp; slot = static_cast<int>(i); }
            }
        }
    }

    const float inv = 1.0f / static_cast<float>(g_ls.atlas_size);
    const float texel = inv;
    LiveShadowEntry& e = g_ls.entries[slot];
    memcpy(e.m, g_ls.pending_m, sizeof(e.m));
    e.tile[0] = vp.TopLeftX * inv + texel;
    e.tile[1] = vp.TopLeftY * inv + texel;
    e.tile[2] = (vp.TopLeftX + vp.Width) * inv - texel;
    e.tile[3] = (vp.TopLeftY + vp.Height) * inv - texel;
    if (e.tile[2] <= e.tile[0] || e.tile[3] <= e.tile[1]) return;
    float khlp_cam[3] = { g_ls.cam[0], g_ls.cam[1], g_ls.cam[2] };
    const float khlp_now = effect_time_seconds();
    float khlp_c[3];
    if (g_ls.frame_view_valid && g_ls.frame_view_time >= 0.0f &&
        khlp_now - g_ls.frame_view_time < 0.05f &&
        kh_view_camera_exact(reinterpret_cast<const float(*)[4]>(g_ls.frame_view), khlp_c)) {
        khlp_cam[0] = khlp_c[0]; khlp_cam[1] = khlp_c[1]; khlp_cam[2] = khlp_c[2];
    } else if (g_ro.cycle_pv_valid &&
               kh_view_camera_exact(g_ro.cycle_pv.view, khlp_c)) {
        khlp_cam[0] = khlp_c[0]; khlp_cam[1] = khlp_c[1]; khlp_cam[2] = khlp_c[2];
    }
    e.cam[0] = khlp_cam[0];
    e.cam[1] = khlp_cam[1];
    e.cam[2] = khlp_cam[2];
    e.stamp = ++g_ls.stamp_counter;
    e.time = effect_time_seconds();
    g_ls.newest = slot;
    g_ls.cycle_latched = true;

}

// Cascade binding harvest: the cascade-family upload never reaches the general upload hooks, so it is harvested from
// the bound buffers instead.
struct CascBindProbe {
    ID3D11Buffer* staging[4] = {};
    uint32_t      bytes[4] = {};
    uint8_t       pending = 0;
};
static CascBindProbe g_cascbind;

// Chooses the four buffers the staging copy reads from: VS slots 0-3 as bound (both callers pass khcs_wide = false).
// The wide scan (the first four bound windows of 64 B or more across VS 0-7 then PS 0-7) is unused.
inline void kh_cascbind_select(ID3D11Buffer* (&khcs_vs)[8], UINT (&khcs_vf)[8], UINT (&khcs_vn)[8],
                               ID3D11Buffer* (&khcs_ps)[8], UINT (&khcs_pf)[8], UINT (&khcs_pn)[8],
                               bool khcs_wide,
                               ID3D11Buffer* (&khcs_out)[4], UINT (&khcs_of)[4], UINT (&khcs_on)[4]) {
    int khcs_n = 0;
    if (!khcs_wide) {
        for (int khcs_i = 0; khcs_i < 4; ++khcs_i) { khcs_out[khcs_i] = khcs_vs[khcs_i]; khcs_of[khcs_i] = khcs_vf[khcs_i]; khcs_on[khcs_i] = khcs_vn[khcs_i]; khcs_vs[khcs_i] = nullptr; }
        khcs_n = 4;
    } else {
        for (int khcs_p = 0; khcs_p < 16 && khcs_n < 4; ++khcs_p) {
            ID3D11Buffer*& khcs_b = khcs_p < 8 ? khcs_vs[khcs_p] : khcs_ps[khcs_p - 8];
            const UINT khcs_f = khcs_p < 8 ? khcs_vf[khcs_p] : khcs_pf[khcs_p - 8];
            const UINT khcs_c = khcs_p < 8 ? khcs_vn[khcs_p] : khcs_pn[khcs_p - 8];
            if (!khcs_b) continue;
            if (khcs_c != 0 && khcs_c * 16u < 64u) continue;
            khcs_out[khcs_n] = khcs_b; khcs_of[khcs_n] = khcs_f; khcs_on[khcs_n] = khcs_c;
            khcs_b = nullptr;
            ++khcs_n;
        }
    }
    for (int khcs_i = 0; khcs_i < 8; ++khcs_i) { KH_SAFE_RELEASE(khcs_vs[khcs_i]); KH_SAFE_RELEASE(khcs_ps[khcs_i]); }
}
static uint32_t g_cascharv_ctr = 0;

inline void cascbind_release() {
    for (int i = 0; i < 4; ++i) {
        KH_SAFE_RELEASE(g_cascbind.staging[i]);
        g_cascbind.bytes[i] = 0;
    }
    g_cascbind.pending = 0;
}

inline void cascbind_step(ID3D11DeviceContext* ctx) {
    for (int i = 0; i < 4; ++i) {
        if (!(g_cascbind.pending & (1u << i)) || !g_cascbind.staging[i]) continue;
        D3D11_MAPPED_SUBRESOURCE ms = {};
        const HRESULT hr = ctx->Map(g_cascbind.staging[i], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &ms);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) continue;
        g_cascbind.pending &= static_cast<uint8_t>(~(1u << i));
        if (FAILED(hr) || !ms.pData) continue;
        g_cascbind_feed_t = effect_time_seconds();
        shadow_live_upload(ms.pData, g_cascbind.bytes[i]);
        ctx->Unmap(g_cascbind.staging[i], 0);
    }

    // Idle while some live entry is fresh AND a harvest staging read succeeded within 3 s (g_cascbind_feed_t is stamped
    // at every successful read, whether or not shadow_live_upload took anything from it).
    const float now_c = effect_time_seconds();

    for (uint32_t i = 0; i < g_ls.count && i < KH_LIVE_MAX_CASCADES; ++i) {
        const auto& e = g_ls.entries[i];
        if (e.tile[2] > 0.0f && e.stamp != 0 && now_c - e.time < 3.0f &&
            g_cascbind_feed_t > 0.0f && now_c - g_cascbind_feed_t < 3.0f) {  return; }
    }

    ID3D11Buffer* bufs[4] = {};
    UINT first16[4] = {};
    UINT num16[4] = {};
    ID3D11DeviceContext1* ctx1 = kh_ctx1(ctx);

    if (ctx1) {
        ID3D11Buffer* khcw_vs[8] = {}; UINT khcw_vf[8] = {}; UINT khcw_vn[8] = {};
        ID3D11Buffer* khcw_ps[8] = {}; UINT khcw_pf[8] = {}; UINT khcw_pn[8] = {};
        const bool khcw_wide = false;
        ctx1->VSGetConstantBuffers1(0, 8, khcw_vs, khcw_vf, khcw_vn);
        if (khcw_wide) ctx1->PSGetConstantBuffers1(0, 8, khcw_ps, khcw_pf, khcw_pn);
        kh_cascbind_select(khcw_vs, khcw_vf, khcw_vn, khcw_ps, khcw_pf, khcw_pn, khcw_wide, bufs, first16, num16);
    } else {
        ID3D11Buffer* khcw_vs[8] = {}; UINT khcw_vf[8] = {}; UINT khcw_vn[8] = {};
        ID3D11Buffer* khcw_ps[8] = {}; UINT khcw_pf[8] = {}; UINT khcw_pn[8] = {};
        const bool khcw_wide = false;
        ctx->VSGetConstantBuffers(0, 8, khcw_vs);
        if (khcw_wide) ctx->PSGetConstantBuffers(0, 8, khcw_ps);
        kh_cascbind_select(khcw_vs, khcw_vf, khcw_vn, khcw_ps, khcw_pf, khcw_pn, khcw_wide, bufs, first16, num16);
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);

    for (int i = 0; i < 4; ++i) {
        if (!bufs[i]) continue;

        if (dev && !(g_cascbind.pending & (1u << i))) {
            D3D11_BUFFER_DESC bd = {};
            bufs[i]->GetDesc(&bd);
            const uint32_t off_b = first16[i] * 16u;
            const uint32_t span_b = num16[i] > 0 ? num16[i] * 16u
                                                 : (bd.ByteWidth > off_b ? bd.ByteWidth - off_b : 0u);

            if (span_b >= 64 && off_b + 64 <= bd.ByteWidth) {
                uint32_t win = span_b < 65536u ? span_b : 65536u;
                if (off_b + win > bd.ByteWidth) win = bd.ByteWidth - off_b;

                if (g_cascbind.staging[i] && g_cascbind.bytes[i] != win) {
                    KH_SAFE_RELEASE(g_cascbind.staging[i]);
                    g_cascbind.bytes[i] = 0;
                }

                if (!g_cascbind.staging[i]) {
                    D3D11_BUFFER_DESC sd = {};
                    sd.ByteWidth = win;
                    sd.Usage = D3D11_USAGE_STAGING;
                    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

                    if (SUCCEEDED(dev->CreateBuffer(&sd, nullptr, &g_cascbind.staging[i]))) {
                        g_cascbind.bytes[i] = win;
                    }
                }

                if (g_cascbind.staging[i] && g_cascbind.bytes[i] == win) {
                    if (win == bd.ByteWidth && off_b == 0) {
                        ctx->CopyResource(g_cascbind.staging[i], bufs[i]);
                    } else {
                        D3D11_BOX box = {};
                        box.left = off_b;
                        box.right = off_b + win;
                        box.bottom = 1;
                        box.back = 1;
                        ctx->CopySubresourceRegion(g_cascbind.staging[i], 0, 0, 0, 0,
                                                   bufs[i], 0, &box);
                    }

                    g_cascbind.pending |= static_cast<uint8_t>(1u << i);
                }
            }
        }

        bufs[i]->Release();
    }

    if (dev) dev->Release();
}

inline bool shadow_probe_target(ID3D11DepthStencilView* dsv) {
    if (!dsv) return false;
    ID3D11Resource* res = nullptr;
    dsv->GetResource(&res);
    if (!res) return false;

    ID3D11Texture2D* tex = nullptr;
    res->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&tex));
    res->Release();
    if (!tex) return false;

    D3D11_TEXTURE2D_DESC td = {};
    tex->GetDesc(&td);
    shadow_live_consider_atlas(tex, td);
    const bool is_atlas = static_cast<void*>(tex) == g_ls.atlas_identity;
    tex->Release();   // The desc is already copied out.

    return is_atlas;
}

// Frame boundary (main depth clear): prune the table's stale entries, expire stale band seals, drop the cycle's upload
// and viewport latches and the SRV cache.
inline void shadow_live_frame_reset() {
    // The table persists - entries stay valid until their cascade re-renders and replaces them - with one exception: a
    // cascade-layout change (sun elevation swings alter the count; 8 -> 5 observed) orphans slots that will never
    // re-render, and shadows drift and never heal.
    const float now_fr = effect_time_seconds();

    float freshest = -1e9f;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        if (g_ls.entries[i].time > freshest) freshest = g_ls.entries[i].time;
    }

    const bool table_active = g_ls.count > 0 && now_fr - freshest < 1.0f;
    uint32_t keep = 0;
    int newest = -1;
    uint64_t newest_stamp = 0;

    for (uint32_t i = 0; i < g_ls.count; ++i) {
        if (table_active && g_ls.entries[i].time > 0.0f && now_fr - g_ls.entries[i].time > 3.0f) continue;

        if (keep != i) g_ls.entries[keep] = g_ls.entries[i];

        if (g_ls.entries[keep].stamp >= newest_stamp) {
            newest_stamp = g_ls.entries[keep].stamp;
            newest = static_cast<int>(keep);
        }

        ++keep;
    }

    g_ls.count = keep;
    g_ls.newest = newest;

    const float khak_bound = 4.0f;
    for (int b = 0; b < 8; ++b) {
        if (g_ls.band[b].valid && now_fr - g_ls.band[b].last_time > khak_bound) {
            g_ls.band[b].valid = false;
            g_ls.band[b].pending_view = false;
        }
    }

    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    // KH_LS_VP_LATCH: a draw's viewport latched this cycle is this cycle's; left set, shadow_live_on_draw would never
    // take the next cascade's (it keeps the first) and the finalize would pair that cascade with this tile.
    g_ls.pending_vp_valid = false;
    g_atlas_srv_count = 0;   // per-frame SRV relearn (see the cache note).
}
inline void release_shadow_device_state() {
    kh_rp_release_all();   // KH_VOL_REPLAY: every engine object the records hold, our targets and copies.
    // Mask SRV, the latched mask candidate, the pre-resolve snapshot + SRV, the pending-copy arm and every weak seam
    // identity die with the device. kh_svs_mask_release's swap half is what mask adoption calls, and it deliberately
    // does none of this.
    kh_svs_mask_release();
    kh_ctx1_release();   // Cached context1 interface the device.
    kh_present_rtv_release();   // KH_PRESENT_UI: RTVs on the old swapchain's buffers.
    g_rt0_desc_n = 0;   // RT0 desc cache = weak identities; wholesale evict.
    g_rt0_desc_next = 0;
    memset(g_rt0_desc, 0, sizeof(g_rt0_desc));

    // Dynamic-lights mirror machinery (device objects + weak identities; the pod censuses survive until the session
    // reset).
    for (int khd_s = 0; khd_s < 2; ++khd_s) {
        if (g_dl.staging[khd_s]) { g_dl.staging[khd_s]->Release(); g_dl.staging[khd_s] = nullptr; }
        g_dl.inflight[khd_s] = 0;
    }
    kh_dlr_release();   // KH_DL_RING.

    for (int khd_s = 0; khd_s < 2; ++khd_s) {
        if (g_dl.arena[khd_s]) { g_dl.arena[khd_s]->Release(); g_dl.arena[khd_s] = nullptr; }
        g_dl.arena_inflight[khd_s] = 0;
        g_dl.win_count[khd_s] = 0;
        g_dl.side_miss[khd_s] = 0;   // KH_DL_CPU (the meta is wiped below).
        g_dl.side_uploads[khd_s] = 0;
    }
    kh_cbs_clear();   // KH_DL_CPU: the shadows are keyed on device buffers.

    memset(g_dl.win_meta, 0, sizeof(g_dl.win_meta));
    g_dl.pool_n = 0;   // Pool positions are pod but the stamp must not lie.
    kh_dl_pmemo_clear();   // KH_DL_PRES_MEMO: the pool it indexes is gone (the span count is not reset here).
    g_dl.pool_stamp = 0;
    g_dl.valid = false;   // getRenderStats reports it (dlValid); a fresh harvest re-arms.

    // screen-mask machinery (cast write).
    if (g_mask.engine_mask_rtv) { g_mask.engine_mask_rtv->Release(); g_mask.engine_mask_rtv = nullptr; }
    if (g_mask.cast_depth) { g_mask.cast_depth->Release(); g_mask.cast_depth = nullptr; }
    if (g_mask.min_blend) { g_mask.min_blend->Release(); g_mask.min_blend = nullptr; }
    if (g_mask.cast_rs_scissor) { g_mask.cast_rs_scissor->Release(); g_mask.cast_rs_scissor = nullptr; }   // KH_CAST_SCISSOR.
    g_mask.cast_rs_scissor_failed = false;
    if (g_mask.prime_blend) { g_mask.prime_blend->Release(); g_mask.prime_blend = nullptr; }
    if (g_mask.prime_dss) { g_mask.prime_dss->Release(); g_mask.prime_dss = nullptr; }
    g_mask.prime_states_failed = false;
    for (int khpq_i = 0; khpq_i < 3; ++khpq_i) {
        if (g_mask.prime_occl[khpq_i]) {
            g_mask.prime_occl[khpq_i]->Release();
            g_mask.prime_occl[khpq_i] = nullptr;
        }
        g_mask.prime_occl_inflight[khpq_i] = false;
    }
    g_mask.prime_occl_cursor = 0;
    g_mask.prime_occl_failed = false;
    if (g_mask.cast_dss) { g_mask.cast_dss->Release(); g_mask.cast_dss = nullptr; }
    g_mask.rt_is_resolve = false;
    g_topo_scene_tex_id = nullptr;
    g_topo_scene_seen_cycle = 0;   // KH_SCENE_NAME.
    g_mask.atlas_bound = false;
    g_mask.engine_mask_failed = false;
    g_mask.cast_states_failed = false;
    g_mask.cast_fov_valid = false;
    g_mask.cast_dims[0] = g_mask.cast_dims[1] = 0.0f;
    g_mask.sweep_settle = 0;
    g_mask.sweep_need = 10;   // A reset is a capture gap.
    g_mask.sweep_last_time = -1.0f;

    if (g_ls.atlas_srv) { g_ls.atlas_srv->Release(); g_ls.atlas_srv = nullptr; }
    if (g_ls.atlas_tex) { g_ls.atlas_tex->Release(); g_ls.atlas_tex = nullptr; }
    g_ls.atlas_identity = nullptr;
    g_ls.atlas_size = 0;
    g_ls.atlas_fmt = 0;
    g_ls.atlas_last_seen = 0.0f;
    g_ls.first_capture_time = -1.0f;
    g_ls.srv_failed = false;
    g_ls.phase_on_atlas = false;
    g_ls.pending_valid = false;
    g_ls.pending_bytes = 0;
    g_ls.pending_vp_valid = false;
    g_ls.cycle_latched = false;
    g_ls.cache_valid = false;
    g_ls.count = 0;
    g_ls.newest = -1;
    g_ls.resolve_window = 0;
    g_ls.resolve_seen_since_cast = false;

    kh_band_slots_release();

    for (uint32_t i = 0; i < 16; ++i) {
        g_ls.cb_reg[i].buf = nullptr;   // Weak keys into a dead device's allocations.
        g_ls.cb_reg[i].floats = 0;
    }

    memset(g_ls.vc_ring, 0, sizeof(g_ls.vc_ring));
    g_ls.vc_n = 0;
    g_ls.view_src_res = nullptr;
    g_ls.view_src_valid = false;
    g_ls.view_src_miss = 0;
    g_ls.frame_view_valid = false;
    g_ls.frame_view_time = -1.0f;
    g_ls.prev_view_valid = false;
    g_ls.alt_view_valid = false;   // The alt latch the session.
    g_ls.receive_warmed = false;   // Next session warms up again.
    g_band_fall_flush = ~0ull;   // Cap latch the session.
    g_ls.alt_view_time = -1.0f;
    g_ls.view_published_this_frame = false;

    // Frozen fire + private sun map.
    if (g_fire_lock) { g_fire_lock->Release(); g_fire_lock = nullptr; }
    g_fire_lock_valid = false;
    g_fire_refreeze_pending = false;

    g_mask_cast_arm = false;
    g_mask_cast_fired = false;
    kh_sun_ladder_forget();   // The ladder's flags, tier keys and input hash.
    g_sun_map_rendered_frame = false;
    g_sun_map_no_local = false;
    g_sun_map_render_time = -1.0f;
    g_sun_mat_chash = 0;
    g_sun_mat_sun[0] = g_sun_mat_sun[1] = g_sun_mat_sun[2] = 0.0f;
    g_sun_local_count = 0;
    g_sun_locals.clear();
    g_sun_locals.shrink_to_fit();
    ++g_sun_locals_serial;   // KH_CASTOCC_KEYSER: every rewrite bumps it, so the
                             // key memo never rests on a second invariant.

    // Weak identities the render hooks compare against.
    g_main_depth_identity = nullptr;
    g_atlas_srv_count = 0;
    // KH_RING_BEST_DEVRESET: the ring best and the prewarm / cold-adoption candidates are keys into the dead
    // device's buffers (a recycled address would match); cleared as the session reset clears them.
    g_ring_best_valid = false;
    g_ring_best_res = nullptr;
    g_prewarm_cand_res = nullptr;
    g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr;
    g_coldlock_cand_off = 0;
    kh_hook_desc_cache_release();   // KH_HOOK_DESC_CACHE: the pinned OM states.
    kh_upload_width_cache_release();   // KH_UPLOAD_WIDTH_CACHE: the pinned buffers.
    g_proj_locator = ProjLocator{};
    kh_engcam_device_reset();
    // The PIP classification cache keys on depth identities and RTV pointers, and the template belongs to the old
    // device's frame; a recycled address after a device reset would fire the injection into a foreign pass. g_pip_seen
    // re-arms at the next classified pass.
    kh_pip_reset();
    kh_infront_tail_release();   // KH_INFRONT: the slice's held scene target and depth view die with the device.
    for (int p = 0; p < 8; ++p) g_proj_pending[p] = ProjPendingMap{};
    g_light_probe = CbColorProbe{};
    g_sky_probe = CbColorProbe{};
    skybind_release();
    cascbind_release();
    kh_gpu_prof_release();   // KH_PROF: the timestamp queries go with the session.
}

// Per draw: while an upload latch is pending on the atlas, the first draw's viewport is latched with it
// (shadow_live_on_draw, KH_LS_VP_LATCH).
inline void shadow_note_draw(ID3D11DeviceContext* ctx) {
    if (g_ls.pending_valid) shadow_live_on_draw(ctx);
    return;   // Live latching runs above.
}

// The upload funnel's admission (kh_upload_hook_wanted, rendering/upload_funnel.hpp), shared by the Map and
// UpdateSubresource hooks (Unmap keeps its own gate): render thread, tracked context, outside the injection, while the
// hooks track anything (g_kh_track_wanted), and some consumer wants the bytes (the CB pointer census, the dynamic-light
// buffer shadows (KH_DL_CPU), the engine-camera locator, the projection hunt and locator, the live-shadow scanners, the
// mirror patch's b2 (KH_MIR_CPU), the fog probes (KH_FOG_PROBE), or the PIP harvest once a PIP pass has been classified
// this session).
static bool g_pip_seen = false;

// Game-thread context use never overlaps this: a park stalls the render thread, and other game-thread uses run inside
// the engine's calls. Do not take the graphics lock from the render thread: it deadlocks the game.
inline void shadow_view_prewarm() {
    if (g_ls.view_src_valid || !g_ro.cycle_pv_valid) return;
    const uint32_t vcn = g_ls.vc_n < 16 ? g_ls.vc_n : 16;
    if (vcn == 0) return;
    const float* truth = &g_ro.cycle_pv.view[0][0];
    int best_i = -1, best_k = 0;
    float best_rot = 1e9f;

    for (uint32_t i = 0; i < vcn; ++i) {
        const float* m = g_ls.vc_ring[i].m;

        for (int orient = 1; orient <= 2; ++orient) {
            float e_rot = 0.0f;

            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) {
                    const float mv = orient == 1 ? m[r * 4 + c] : m[c * 4 + r];
                    e_rot += fabsf(mv - truth[r * 4 + c]);
                }
            }

            if (e_rot < best_rot) {
                best_rot = e_rot;
                best_i = static_cast<int>(i);
                best_k = orient;
            }
        }
    }

    if (best_i < 0 || best_rot >= 0.03f) return;   // Impostors score ~0.05-0.13, the truth ~0.00.
    const float* m = g_ls.vc_ring[best_i].m;
    float tmag = 0.0f;

    for (int c = 0; c < 3; ++c) {
        const float tv = best_k == 1 ? m[12 + c] : m[c * 4 + 3];
        tmag += fabsf(tv);
    }

    if (tmag >= 10.0f) {
        return;   // Relative only (same rule as the injection).
    }

    // The same candidate must win two consecutive boundary attempts.
    if (g_ls.vc_ring[best_i].res != g_prewarm_cand_res ||
        g_ls.vc_ring[best_i].off != g_prewarm_cand_off) {

        g_prewarm_cand_res = g_ls.vc_ring[best_i].res;
        g_prewarm_cand_off = g_ls.vc_ring[best_i].off;
        return;   // First sighting: confirm next boundary.
    }

    g_ls.view_src_res = g_ls.vc_ring[best_i].res;
    g_ls.view_src_off = g_ls.vc_ring[best_i].off;
    g_ls.view_src_orient = best_k;
    g_ls.view_src_relative = true;
    g_ls.view_src_valid = true;
    kh_view_lock_acquired();
    g_ls.view_src_miss = 0;
    g_ls.pub_ok_since_lock = false;
    g_ls.view_locks++;
    g_lock_churn_ms_v = steady_now_ms();   // lock-settle: fresh lock, hold the cast a beat.

        g_ls.wipe_deferred = true;

}
}