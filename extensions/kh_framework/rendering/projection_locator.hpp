#pragma once

// The injection half (rendering_integration_injection.hpp): the projection locator (ProjLocator) and its upload scan.
// Never include it on its own.
namespace RenderIntegration {

inline void kh_upload_need(const void* khun_d, uint32_t khun_n);   // KH_UPLOAD_LAZY: defined in
                                                                  // rendering/upload_funnel.hpp.
// Render-thread writes; read by the injection and the flush.
static float    g_slot_keep_m22 = 0.0f;
static float    g_slot_keep_m32 = 0.0f;
static float    g_slot_keep_near = -1.0f;
static uint64_t g_slot_keep_ms = 0;

struct ProjLocator {
    ID3D11Resource* buf = nullptr;   // Identity only - never dereferenced.
    uint32_t float_offset = 0;
    bool transposed = false;
    bool valid = false;
};
static ProjLocator g_proj_locator;

struct ProjPendingMap {
    ID3D11Resource* res = nullptr;
    void* data = nullptr;
    uint32_t bytes = 0;
    bool discard = false;   // KH_MAP_MIX: the map was WRITE_DISCARD (armed counters; KH_MAP_REDIRECT takes no other).
    bool redir = false;     // KH_MAP_REDIRECT: data is the cached buffer handed to the engine, not the driver's.
};
alignas(64) static ProjPendingMap g_proj_pending[8];   // KH_HOT_ALIGN (192 B: 3 lines, not 4).

inline bool proj_near_zero(float v) { return fabsf(v) < 1e-6f; }

// Persisted last captured m32: the continuity anchor for measurement-mode acceptance.
static float g_proj_last_m32 = 0.0f;

// Tests a 16-float window for the row-vector D3D perspective layout [ sx 0 0 0 / 0 sy 0 0 / jx jy m22 1 / 0 0 m32 0 ]
// and its transpose. Structure, sx, sy and m22 must match the latch tightly.
inline float proj_slot_probe(const float* w, float* out_m22, float* out_m32) {
    const float sx = g_ro.cycle_pv.projection[0][0];
    const float sy = g_ro.cycle_pv.projection[1][1];
    const float m22_l = g_ro.cycle_pv.projection[2][2];
    const float m32_l = g_ro.cycle_pv.projection[3][2];
    if (fabsf(m32_l) < 1e-9f) return -1.0f;

    auto close = [](float a, float b, float rel) {
        return fabsf(a - b) <= rel * (fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b)) + 1e-5f;
    };

    if (!g_proj_locator.transposed) {
        if (proj_near_zero(w[1]) && proj_near_zero(w[2]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[15]) &&
            fabsf(w[11] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
            w[14] != 0.0f && w[14] * m32_l > 0.0f && fabsf(w[10]) > 1e-9f) {
            *out_m22 = w[10];
            *out_m32 = w[14];
            return -w[14] / w[10];
        }
    } else {
        if (proj_near_zero(w[1]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[8]) && proj_near_zero(w[9]) && proj_near_zero(w[12]) && proj_near_zero(w[13]) &&
            fabsf(w[14] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
            w[11] != 0.0f && w[11] * m32_l > 0.0f && fabsf(w[10]) > 1e-9f) {
            *out_m22 = w[10];
            *out_m32 = w[11];
            return -w[11] / w[10];
        }
    }

    return -1.0f;
}

inline bool proj_try_window(const float* w, bool discovery) {
    const float sx = g_ro.cycle_pv.projection[0][0];
    const float sy = g_ro.cycle_pv.projection[1][1];
    const float m22_l = g_ro.cycle_pv.projection[2][2];
    const float m32_l = g_ro.cycle_pv.projection[3][2];
    if (fabsf(m32_l) < 1e-9f) return false;

    auto close = [](float a, float b, float rel) {
        return fabsf(a - b) <= rel * (fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b)) + 1e-5f;
    };

    auto m32_ok = [&](float v) {
        if (close(v, m32_l, 0.10f)) return true;
        if (discovery) return false;
        return g_proj_last_m32 != 0.0f && close(v, g_proj_last_m32, 0.25f);
    };

    if (proj_near_zero(w[1]) && proj_near_zero(w[2]) && proj_near_zero(w[3]) &&
        proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
        proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[15]) &&
        fabsf(w[11] - 1.0f) < 1e-4f &&
        close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
        m32_ok(w[14])) {
        {
            const float n_prev = fabsf(g_ro.engine_m22) > 1e-9f ? (-g_ro.engine_m32 / g_ro.engine_m22) : 0.0f;
            const float n_new  = fabsf(w[10]) > 1e-9f ? (-w[14] / w[10]) : 0.0f;

            // Near-plane shift mid-frame. Never runs: every call sits behind proj_scan_upload's engine_proj_valid
            // return, so the pair latches once per cycle. The partition re-arm is done in inject_composited_meshes
            // (KH_PART_NEAR, the anomalous cycle).
            if (g_ro.engine_proj_valid && g_ro.injected &&
                n_prev > 1e-4f && fabsf(n_new - n_prev) > 0.05f * n_prev) {
                g_ro.injected = false;
            }
        }
        g_ro.engine_m22 = w[10];
        g_ro.engine_m32 = w[14];
        g_proj_last_m32 = w[14];
        g_ro.engine_proj_valid = true;
        g_proj_locator.transposed = false;
        return true;
    }

    // Transposed (HLSL column_major) upload.
    if (proj_near_zero(w[1]) && proj_near_zero(w[3]) &&
        proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
        proj_near_zero(w[8]) && proj_near_zero(w[9]) && proj_near_zero(w[12]) && proj_near_zero(w[13]) &&
        fabsf(w[14] - 1.0f) < 1e-4f &&
        close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f) &&
        m32_ok(w[11])) {
        {
            const float n_prev = fabsf(g_ro.engine_m22) > 1e-9f ? (-g_ro.engine_m32 / g_ro.engine_m22) : 0.0f;
            const float n_new  = fabsf(w[10]) > 1e-9f ? (-w[11] / w[10]) : 0.0f;

            if (g_ro.engine_proj_valid && g_ro.injected &&
                n_prev > 1e-4f && fabsf(n_new - n_prev) > 0.05f * n_prev) {
                g_ro.injected = false;
            }
        }
        g_ro.engine_m22 = w[10];
        g_ro.engine_m32 = w[11];
        g_proj_last_m32 = w[11];
        g_ro.engine_proj_valid = true;
        g_proj_locator.transposed = true;
        return true;
    }

    return false;
}
inline void proj_scan_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!g_ro.cycle_pv_valid) return;

    // Foreign windows are recorded deliberately: identity is the discriminator the values cannot provide.
    if (kh_cbc_on() && g_proj_locator.valid && bytes >= 16 * sizeof(float)) {
        const float* khcb_f = static_cast<const float*>(data);
        const uint32_t khcb_n = (bytes > 16384 ? 16384 : bytes) / 4;
        kh_upload_need(data, (g_proj_locator.float_offset + 16u) * 4u);   // KH_UPLOAD_LAZY: that offset and offset 0.
        float khcb_m22 = 0.0f, khcb_m32 = 0.0f;
        float khcb_near = -1.0f;

        if (g_proj_locator.float_offset + 16 <= khcb_n) {
            khcb_near = proj_slot_probe(
                khcb_f + g_proj_locator.float_offset, &khcb_m22, &khcb_m32);
        }


        if (khcb_near <= 0.0f && g_proj_locator.float_offset != 0 && khcb_n >= 16) {
            khcb_near = proj_slot_probe(khcb_f, &khcb_m22, &khcb_m32);
        }

        // Foreign buffers never enter the table and resolve as misses.
        if (khcb_near > 0.0f && !(khcb_near >= KH_CAM_NEAR_MIN && khcb_near <= KH_CAM_NEAR_MAX)) {
        } else if (khcb_near > 0.0f) {
            kh_cbc_note(res, khcb_m22, khcb_m32, nullptr);
        }
    }

    // Partition telltale: uploads to the known slot refresh the live pair even while the lock idles.
    if (g_proj_locator.valid && res == g_proj_locator.buf && bytes >= 16 * sizeof(float)) {
        const uint32_t pn = (bytes > 16384 ? 16384 : bytes) / 4;
        kh_upload_need(data, (g_proj_locator.float_offset + 16u) * 4u);   // KH_UPLOAD_LAZY.

        if (g_proj_locator.float_offset + 16 <= pn) {
            float p22 = 0.0f, p32 = 0.0f;
            const float pnear = proj_slot_probe(
                static_cast<const float*>(data) + g_proj_locator.float_offset, &p22, &p32);

            if (pnear > 0.0f) {

                g_ro.slot_near_live = pnear;
                g_ro.slot_m22 = p22;
                g_ro.slot_m32 = p32;

                // A late-frame viewmodel upload must not be cached into the
                // keep: it would feed every silent frame for 250 ms.
                if (pnear >= KH_CAM_NEAR_MIN && pnear <= KH_CAM_NEAR_MAX &&
                    kh_enc_far(p22, p32) <= KH_ENC_FAR_MAX) {
                    g_slot_keep_m22 = p22;
                    g_slot_keep_m32 = p32;
                    g_slot_keep_near = pnear;
                    g_slot_keep_ms = steady_now_ms();
                }
            }
        }
    }

    if (g_ro.engine_proj_valid) return;
    if (bytes < 16 * sizeof(float)) return;
    if (bytes > 16384) bytes = 16384;   // per-frame CBs are small; cap the scan.
    kh_upload_need(data, bytes);   // KH_UPLOAD_LAZY: the hunt reads all of it.
    const float* f = static_cast<const float*>(data);
    const uint32_t nfloats = bytes / 4;

    if (g_proj_locator.valid && g_proj_locator.buf == res) {
        if (g_proj_locator.float_offset + 16 <= nfloats && proj_try_window(f + g_proj_locator.float_offset, false)) {
            return;
        }

        g_proj_locator.valid = false;
    }

    for (uint32_t off = 0; off + 16 <= nfloats; off += 4) {
        if (proj_try_window(f + off, true)) {
            g_proj_locator.buf = res;
            g_proj_locator.float_offset = off;
            g_proj_locator.valid = true;
            g_proj_locator_ever = true;   // Persistent: the anomaly gate keys on this.
            return;
        }
    }
}
}