#pragma once

// The injection half (rendering_integration_injection.hpp): the shadow locators: the colour probes, the sky binding,
// the ring best and the rescue window. Never include it on its own.
namespace RenderIntegration {
inline void shadow_view_scan(ID3D11Resource* res, const void* data, uint32_t bytes, uint32_t base_off);

// The sky block's anchor (nf >= 72): row 17 exactly (0.25, 0.75, 3.5), or structurally - row 15 positive and small,
// row 17 ascending and bounded (< 32), row 14 sane.

inline bool locator_sky_anchor(const float* f, uint32_t nf) {
    if (nf < 72) return false;

    // Should a map or config ever move them, the structural path below still stands.
    if (f[68] == 0.25f && f[69] == 0.75f && f[70] == 3.5f) return true;

    const float* ray = f + 60;
    if (!(ray[0] > 0.0f && ray[1] > 0.0f && ray[2] > 0.0f && ray[0] < 1.0f && ray[2] < 1.0f)) return false;

    const float* grad = f + 68;
    if (!(grad[0] >= 0.0f && grad[1] >= grad[0] && grad[2] > grad[1] && grad[2] < 32.0f)) return false;

    const float* lay = f + 56;
    if (!(fabsf(lay[0]) < 5000.0f && lay[1] >= 0.0f && lay[2] > 0.0f && lay[2] < 0.5f)) return false;

    return true;
}
// KH_RING_BEST_AGE: a best the injection may still take - admitted within KH_RING_BEST_MAX_CYCLES. The hunt's
// admission asks the same: an aged best is no best there either, so any new candidate replaces it (it could
// otherwise be displaced only by a strictly lower error, which a still camera never produces).
inline bool kh_ring_best_fresh() {
    return g_ring_best_valid && g_topo_cycles >= g_ring_best_cycle &&
           g_topo_cycles - g_ring_best_cycle <= KH_RING_BEST_MAX_CYCLES;
}

// Functional, not diagnostic: gates the sky-span trigger path and the injection's rescue take. Healthy frames are never
// carried; a rescue early in the repainting partition writes world-range depth.
inline bool kh_rescue_window_open() {
    const uint64_t khr_cfs = g_cc_flush_serial.load(std::memory_order_relaxed);

    if (g_carry_pending_serial.load(std::memory_order_relaxed) != khr_cfs) {
        return false;
    }

    if (g_cc_flush_opaques.load(std::memory_order_relaxed) == 0xFFFFFFFFu) {
        return false;
    }

    const bool khr_keep_ok = g_slot_keep_near > 0.0f && g_slot_keep_ms != 0 &&
                             steady_now_ms() - g_slot_keep_ms < 250 &&
                             g_slot_keep_near >= KH_CAM_NEAR_MIN &&
                             g_slot_keep_near <= KH_CAM_NEAR_MAX;

    if (!khr_keep_ok) {
        return false;
    }

    return true;
}

inline void skybind_release() {
    for (int i = 0; i < 16; ++i) {
        KH_SAFE_RELEASE(g_skybind.staging[i]);
        g_skybind.bytes[i] = 0;
    }
    g_skybind.pending = 0;
}

inline void skybind_step(ID3D11DeviceContext* ctx) {
    // Phase one: harvest last frame's copies.
    for (int i = 0; i < 16; ++i) {
        if (!(g_skybind.pending & (1u << i)) || !g_skybind.staging[i]) continue;
        D3D11_MAPPED_SUBRESOURCE ms = {};
        const HRESULT hr = ctx->Map(g_skybind.staging[i], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &ms);
        if (hr == DXGI_ERROR_WAS_STILL_DRAWING) continue;
        g_skybind.pending &= static_cast<uint16_t>(~(1u << i));
        if (FAILED(hr) || !ms.pData) continue;
        const float* f = static_cast<const float*>(ms.pData);
        const uint32_t nf = g_skybind.bytes[i] / 4;

        if (nf >= 72 && locator_sky_anchor(f, nf)) {
            g_sky_probe.floats = nf;
            g_sky_probe.valid = true;
            g_sky_probe.hits++;
            g_sky_probe.last_confirm = effect_time_seconds();
            locator_capture(g_sky_probe, f, nf, 96);
        }

        // VS-side windows carry the world draws' view matrix: feed the same scan the upload hooks feed - ring admission
        // when unlocked. VS copies are staged only while the view is unlocked, so after the lock only copies already in
        // flight arrive here. KH_VIEW_STAGED_LOCKED: those are not fed - a copy staged at an earlier opaque sample
        // would be published as this moment's frame view (the locked path stamps it now); locked, the scan's only use
        // of a copy is that publish (with its disagreement count against the bridge).
        if (i >= 8 && g_skybind.src[i] && !g_ls.view_src_valid) {
            if (g_ro.in_injection) {
            } else {
                shadow_view_scan(g_skybind.src[i], f, g_skybind.bytes[i], g_skybind.base_fl[i]);
            }
        }

        ctx->Unmap(g_skybind.staging[i], 0);
    }

    // Phase two: stage fresh copies. Two independent wants: a stale sky mirror (fog) or an unlocked view (the harvest).
    // Idles fully when both are satisfied.
    const bool want_sky_stage = !(g_sky_probe.valid &&
        effect_time_seconds() - g_sky_probe.last_confirm < 5.0f);
    // The view lock's only consumer is the lit shadow cast; demand-gate the
    // stage so unlit sessions stop staging VS copies forever.
    const bool want_view_stage = !g_ls.view_src_valid && shadow_live_wanted();
    if (!want_sky_stage && !want_view_stage) return;

    ID3D11Buffer* bufs[16] = {};
    UINT first16[16] = {};
    UINT num16[16] = {};
    ID3D11DeviceContext1* ctx1 = kh_ctx1(ctx);

    if (ctx1) {
        ctx1->PSGetConstantBuffers1(0, 8, bufs, first16, num16);
        ctx1->VSGetConstantBuffers1(0, 8, bufs + 8, first16 + 8, num16 + 8);
    } else {
        ctx->PSGetConstantBuffers(0, 8, bufs);
        ctx->VSGetConstantBuffers(0, 8, bufs + 8);
    }

    ID3D11Device* dev = nullptr;
    ctx->GetDevice(&dev);

    for (int i = 0; i < 16; ++i) {
        if (!bufs[i]) continue;

        // VS slots: never at the injection (our own CB is bound there - never self-learn), and only while the view
        // wants feeding; PS slots only while the sky wants feeding.
        const bool stage_ok = i < 8 ? want_sky_stage
                                    : (want_view_stage && !g_ro.in_injection);

        if (stage_ok && dev && !(g_skybind.pending & (1u << i))) {
            D3D11_BUFFER_DESC bd = {};
            bufs[i]->GetDesc(&bd);

            const uint32_t off_b = first16[i] * 16u;   // Zero on the 11.0 path.
            const uint32_t span_b = num16[i] > 0 ? num16[i] * 16u
                                                 : (bd.ByteWidth > off_b ? bd.ByteWidth - off_b : 0u);

            if (span_b >= 288 && off_b + 288 <= bd.ByteWidth) {
                uint32_t win = span_b < 4096u ? span_b : 4096u;
                if (off_b + win > bd.ByteWidth) win = bd.ByteWidth - off_b;

                if (g_skybind.staging[i] && g_skybind.bytes[i] != win) {
                    KH_SAFE_RELEASE(g_skybind.staging[i]);
                    g_skybind.bytes[i] = 0;
                }

                if (!g_skybind.staging[i]) {
                    D3D11_BUFFER_DESC sd = {};
                    sd.ByteWidth = win;
                    sd.Usage = D3D11_USAGE_STAGING;
                    sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

                    if (SUCCEEDED(dev->CreateBuffer(&sd, nullptr, &g_skybind.staging[i]))) {
                        g_skybind.bytes[i] = win;
                    }
                }

                if (g_skybind.staging[i] && g_skybind.bytes[i] == win) {
                    g_skybind.src[i] = bufs[i];
                    g_skybind.base_fl[i] = off_b / 4u;

                    if (win == bd.ByteWidth && off_b == 0) {
                        ctx->CopyResource(g_skybind.staging[i], bufs[i]);
                    } else {
                        D3D11_BOX box = {};
                        box.left = off_b;
                        box.right = off_b + win;
                        box.bottom = 1;
                        box.back = 1;
                        ctx->CopySubresourceRegion(g_skybind.staging[i], 0, 0, 0, 0,
                                                   bufs[i], 0, &box);
                    }

                    g_skybind.pending |= static_cast<uint16_t>(1u << i);
                }
            }
        }

        bufs[i]->Release();   // PSGetConstantBuffers adds a reference.
    }

    if (dev) dev->Release();
}

// KH_ANCHOR_NAN: both lane tests are in positive form, so a NaN lane (an int -1 / all-ones mask reads as NaN)
// fails them; wherever d and tol are not NaN, !(d <= tol) is d > tol, the same verdict.
inline bool locator_light_anchor(const float* f, uint32_t i, uint32_t nf, float decay, float cam_alt) {
    if (i + 16 > nf) return false;
    if (!(fabsf(f[i] - decay) <= 1.0e-4f + 5.0e-3f * decay)) return false;
    if (!(fabsf(f[i + 12] + cam_alt) <= 2.5f)) return false;
    return true;
}

inline bool locator_zero_anchor(const float* f, uint32_t i, uint32_t nf,
                                float cam_alt) {
    if (i + 16 > nf) return false;
    if (!(fabsf(f[i + 12] + cam_alt) <= 2.5f)) return false;   // nb[52] = -(camera altitude). KH_ANCHOR_NAN.
    uint32_t khza_mode = 0;
    memcpy(&khza_mode, &f[i + 4], sizeof(khza_mode));   // nb[44] = fog-model selector.
    return khza_mode == 1u || khza_mode == 3u;
}

// meta is the caller's field (kh_probe_mirror_refresh stores the light probe's off - nb_base there; KH_PROBE_META40).
inline void locator_capture(CbColorProbe& pr, const float* f, uint32_t nf, uint32_t win) {
    if (win > 96) win = 96;

    if (nf <= win) {
        pr.nb_base = 0;   // Small CB: the whole block, head included.
    } else {
        pr.nb_base = pr.off >= 40 ? pr.off - 40 : 0;
        if (pr.nb_base + win > nf) pr.nb_base = nf - win;
    }

    const uint32_t n = nf - pr.nb_base < win ? nf - pr.nb_base : win;
    memcpy(pr.nb, f + pr.nb_base, n * sizeof(float));

    // Sky-probe refreshes also update the engine-sun reference (floats 8-10 of the sky block, normalized, unflipped):
    // the sky lane kh_sun_raw_frame and kh_sun_axis_lane_fresh / _ok read, and the motion window kh_sky_static reads.
    if (&pr == &g_sky_probe && nf >= 11) {
        const float sl = sqrtf(f[8] * f[8] + f[9] * f[9] + f[10] * f[10]);

        if (sl > 0.5f && sl < 2.0f) {
            g_skysun_ref[0] = f[8] / sl;
            g_skysun_ref[1] = f[9] / sl;
            g_skysun_ref[2] = f[10] / sl;
            g_skysun_ref_valid = true;
            g_skysun_ref_ms = steady_now_ms();

            if (g_skysun_mot_ref_ms == 0) {
                g_skysun_mot_ref[0] = g_skysun_ref[0];
                g_skysun_mot_ref[1] = g_skysun_ref[1];
                g_skysun_mot_ref[2] = g_skysun_ref[2];
                g_skysun_mot_ref_ms = g_skysun_ref_ms;
            } else if (g_skysun_ref_ms - g_skysun_mot_ref_ms >= 1000) {
                float khsm_d = g_skysun_ref[0] * g_skysun_mot_ref[0] +
                               g_skysun_ref[1] * g_skysun_mot_ref[1] +
                               g_skysun_ref[2] * g_skysun_mot_ref[2];
                khsm_d = khsm_d > 1.0f ? 1.0f : (khsm_d < -1.0f ? -1.0f : khsm_d);
                g_skysun_mot_deg = acosf(khsm_d) * 57.29578f;
                g_skysun_mot_ref[0] = g_skysun_ref[0];
                g_skysun_mot_ref[1] = g_skysun_ref[1];
                g_skysun_mot_ref[2] = g_skysun_ref[2];
                g_skysun_mot_ref_ms = g_skysun_ref_ms;
            }
        }
    }
}
}