#pragma once

// The injection half (rendering_integration_injection.hpp): the projection locator (ProjLocator) and its upload scan.
// Never include it on its own.
namespace RenderIntegration {

inline void upload_need(const void* d, uint32_t n);   // UPLOAD_LAZY: defined in
                                                                  // rendering/upload_funnel.hpp.
// The projection slot's last good pair (the poison shield the injection and the flush fall back on while the slot is
// silent).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct SlotKeepState {
    // Render-thread writes; read by the injection and the flush.
    float    m22 = 0.0f;
    float    m32 = 0.0f;
    float    near_m = -1.0f;
    uint64_t ms = 0;
};
static SlotKeepState g_slot_keep;

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
    bool discard = false;   // MAP_MIX: the map was WRITE_DISCARD (armed counters; MAP_REDIRECT takes no other).
    bool redir = false;     // MAP_REDIRECT: data is the cached buffer handed to the engine, not the driver's.
};
alignas(64) static ProjPendingMap g_proj_pending[8];   // HOT_ALIGN (192 B: 3 lines, not 4).

inline bool proj_near_zero(float v) { return fabsf(v) < 1e-6f; }

// Persisted last captured m32: the continuity anchor for measurement-mode acceptance.
static float g_proj_last_m32 = 0.0f;

// The row-vector D3D perspective layout [ sx 0 0 0 / 0 sy 0 0 / jx jy m22 1 / 0 0 m32 0 ] (t false) or its
// transpose (HLSL column_major, t true): the structure, and sx, sy and m22 within 2 % of the latch's. Returns the
// window index of the m32 lane (14, or 11 transposed), or -1. The one structural test of the probe and the hunt below.
inline int proj_layout(const float* w, bool t, float sx, float sy, float m22_l) {
    auto close = [](float a, float b, float rel) {
        return fabsf(a - b) <= rel * (fabsf(a) > fabsf(b) ? fabsf(a) : fabsf(b)) + 1e-5f;
    };

    if (!t) {
        if (proj_near_zero(w[1]) && proj_near_zero(w[2]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[12]) && proj_near_zero(w[13]) && proj_near_zero(w[15]) &&
            fabsf(w[11] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f)) {
            return 14;
        }
    } else {
        if (proj_near_zero(w[1]) && proj_near_zero(w[3]) &&
            proj_near_zero(w[4]) && proj_near_zero(w[6]) && proj_near_zero(w[7]) &&
            proj_near_zero(w[8]) && proj_near_zero(w[9]) && proj_near_zero(w[12]) && proj_near_zero(w[13]) &&
            fabsf(w[14] - 1.0f) < 1e-4f &&
            close(w[0], sx, 0.02f) && close(w[5], sy, 0.02f) && close(w[10], m22_l, 0.02f)) {
            return 11;
        }
    }

    return -1;
}

// Tests a 16-float window in the locked slot's own orientation (proj_layout) against the latch, m32 of the latch's
// sign. Returns the window's near (-m32 / m22) with the pair in out_m22 / out_m32, or -1.
inline float proj_slot_probe(const float* w, float* out_m22, float* out_m32) {
    const float sx = g_ro.cycle_pv.projection[0][0];
    const float sy = g_ro.cycle_pv.projection[1][1];
    const float m22_l = g_ro.cycle_pv.projection[2][2];
    const float m32_l = g_ro.cycle_pv.projection[3][2];
    if (fabsf(m32_l) < 1e-9f) return -1.0f;

    const int i = proj_layout(w, g_proj_locator.transposed, sx, sy, m22_l);
    if (i >= 0 && w[i] != 0.0f && w[i] * m32_l > 0.0f && fabsf(w[10]) > 1e-9f) {
        *out_m22 = w[10];
        *out_m32 = w[i];
        return -w[i] / w[10];
    }

    return -1.0f;
}

// The hunt's test (either orientation, the plain layout first): m32 within 10 % of the latch's, or off the known slot
// (discovery false) within 25 % of the last latched m32. Latches the engine pair on a match. No mid-frame near-shift
// re-arm here: every call sits behind proj_scan_upload's engine_proj_valid return, so the pair latches once per cycle;
// the partition re-arm is inject_composited_meshes' (PART_NEAR, the anomalous cycle).
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

    for (int t = 0; t < 2; ++t) {   // The plain layout, then the transposed upload.
        const int i = proj_layout(w, t != 0, sx, sy, m22_l);
        if (i < 0 || !m32_ok(w[i])) continue;
        g_ro.engine_m22 = w[10];
        g_ro.engine_m32 = w[i];
        g_proj_last_m32 = w[i];
        g_ro.engine_proj_valid = true;
        g_proj_locator.transposed = t != 0;
        return true;
    }

    return false;
}
inline void proj_scan_upload(ID3D11Resource* res, const void* data, uint32_t bytes) {
    if (!g_ro.cycle_pv_valid) return;

    // Foreign windows are recorded deliberately: identity is the discriminator the values cannot provide.
    if (cbc_on() && g_proj_locator.valid && bytes >= 16 * sizeof(float)) {
        const float* cb_f = static_cast<const float*>(data);
        const uint32_t n = (bytes > 16384 ? 16384 : bytes) / 4;
        upload_need(data, (g_proj_locator.float_offset + 16u) * 4u);   // UPLOAD_LAZY: that offset and offset 0.
        float m22 = 0.0f, m32 = 0.0f;
        float cb_near = -1.0f;

        if (g_proj_locator.float_offset + 16 <= n) {
            cb_near = proj_slot_probe(
                cb_f + g_proj_locator.float_offset, &m22, &m32);
        }

        if (cb_near <= 0.0f && g_proj_locator.float_offset != 0 && n >= 16) {
            cb_near = proj_slot_probe(cb_f, &m22, &m32);
        }

        // Foreign buffers (a near outside the camera band) never enter the table and resolve as misses.
        if (cb_near >= CAM_NEAR_MIN && cb_near <= CAM_NEAR_MAX) {
            cbc_note(res, m22, m32);
        }
    }

    // Partition telltale: uploads to the known slot refresh the live pair even while the lock idles.
    if (g_proj_locator.valid && res == g_proj_locator.buf && bytes >= 16 * sizeof(float)) {
        const uint32_t pn = (bytes > 16384 ? 16384 : bytes) / 4;
        upload_need(data, (g_proj_locator.float_offset + 16u) * 4u);   // UPLOAD_LAZY.

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
                if (pnear >= CAM_NEAR_MIN && pnear <= CAM_NEAR_MAX &&
                    enc_far(p22, p32) <= ENC_FAR_MAX) {
                    g_slot_keep.m22 = p22;
                    g_slot_keep.m32 = p32;
                    g_slot_keep.near_m = pnear;
                    g_slot_keep.ms = steady_now_ms();
                }
            }
        }
    }

    if (g_ro.engine_proj_valid) return;
    if (bytes < 16 * sizeof(float)) return;
    if (bytes > 16384) bytes = 16384;   // per-frame CBs are small; cap the scan.
    upload_need(data, bytes);   // UPLOAD_LAZY: the hunt reads all of it.
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