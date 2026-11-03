#pragma once

// The injection half (rendering_integration_injection.hpp): the engine camera scan (KH_ENGCAM). Never include it on its
// own.
namespace RenderIntegration {

// The lock is (buffer identity, float offset), compared opaquely and never dereferenced.
struct KhEngCamCand {
    float   v[3] = {};   // Sighted value (bit-exact key).
    void*   res = nullptr;   // First sighting's buffer identity (opaque).
    int32_t off = -1;
    uint32_t sights = 0;   // Sightings of this exact value this frame.
};
static KhEngCamCand g_engcam_cand[32];
static uint32_t g_engcam_cand_n = 0;
static uint64_t g_engcam_cand_seq = ~0ull;   // g_svs_frame_seq of the table.
static void*    g_engcam_res = nullptr;
static int32_t  g_engcam_off = -1;   // Locked float offset (-1 = none).
// The locked buffer is a ring CB serving several shader layouts: a single last-sighting slot lets a foreign layout's
// upload evict the camera row after it arrived this frame.
static float    g_engcam_vals[4][3] = {};   // Distinct sightings at the lock, this frame.
static uint32_t g_engcam_vals_n = 0;
static uint64_t g_engcam_val_seq = ~0ull;   // Frame stamp of the set.
static float    g_engcam_frame_val[3] = {};   // The frame's latched verdict.
static uint64_t g_engcam_frame_seq = ~0ull;
static bool     g_engcam_frame_taken = false;
static float    g_engcam_ref[3] = {};   // Last recovered camera (coarse).
static bool     g_engcam_ref_ok = false;   // Scan gate only - never decides.

inline bool kh_engcam_on() {
    // The feature toggle: the scan and the consume run whenever the funnel does (g_kh_track_wanted gates it upstream).
    // What the locator asks the funnel to CAPTURE is kh_engcam_wanted below, not this.
    return true;
}

// KH_ENGCAM_GATE: the locator's demand on the upload funnel. Unlocked it hunts every constant-buffer map; locked, only
// a map of the locked buffer is wanted (losing the lock reopens the hunt). The other consumers keep their own terms. In
// practice the lock lands on the engine's per-draw buffer, so this saves nothing; it is kept because it is correct and
// equivalent. Render thread.
inline bool kh_engcam_wanted(ID3D11Resource* khew_res) {
    if (!kh_engcam_on()) return false;
    return !g_engcam_res || static_cast<void*>(khew_res) == g_engcam_res;
}

inline bool kh_engcam_agree4(const float* khg_a, const float* khg_b) {

    float khg_m = fabsf(khg_a[0]);
    if (fabsf(khg_a[1]) > khg_m) khg_m = fabsf(khg_a[1]);
    if (fabsf(khg_a[2]) > khg_m) khg_m = fabsf(khg_a[2]);
    const float khg_eps = 8.0f * (nextafterf(khg_m, 3.0e38f) - khg_m);
    return fabsf(khg_a[0] - khg_b[0]) <= khg_eps &&
           fabsf(khg_a[1] - khg_b[1]) <= khg_eps &&
           fabsf(khg_a[2] - khg_b[2]) <= khg_eps;
}

inline bool kh_engcam_plausible(const float* khp_r) {
    if (!(khp_r[0] == khp_r[0]) || !(khp_r[1] == khp_r[1]) ||
        !(khp_r[2] == khp_r[2])) return false;
    const float khp_ax = fabsf(khp_r[0]);
    const float khp_ay = fabsf(khp_r[1]);
    const float khp_az = fabsf(khp_r[2]);
    const float khp_m = khp_ax > khp_ay ? (khp_ax > khp_az ? khp_ax : khp_az)
                                        : (khp_ay > khp_az ? khp_ay : khp_az);
    return khp_m >= 1.0f && khp_m <= 60000.0f;
}

// KH_ENGCAM_SILENT: frames (g_svs_frame_seq) a lock may go without a sighting before it is let go (kh_engcam_eval).
static constexpr uint64_t KH_ENGCAM_SILENT_FRAMES = 8;

inline void kh_engcam_device_reset() {
    // Weak identities die with the device/session (the g_proj_locator rule).
    g_engcam_res = nullptr;
    g_engcam_off = -1;
    g_engcam_val_seq = ~0ull;
    g_engcam_vals_n = 0;
    g_engcam_frame_seq = ~0ull;
    g_engcam_frame_taken = false;
    g_engcam_cand_seq = ~0ull;
    g_engcam_cand_n = 0;
    g_engcam_ref_ok = false;
}

// The scan half: called beside locator_note_upload on every tracked upload. Render thread. Read-only on the upload.
inline void kh_engcam_scan(ID3D11Resource* khes_res, const void* khes_data,
                           uint32_t khes_bytes) {
    if (!kh_engcam_on() || !khes_res || !khes_data || khes_bytes < 16) return;
    // Never self-learn: locator_note_upload's own exclusion list.
    if (khes_res == static_cast<ID3D11Resource*>(g_res.composite_cb)) return;
    if (khes_res == static_cast<ID3D11Resource*>(g_res.constant_buffer)) return;
    if (khes_res == static_cast<ID3D11Resource*>(g_res.frame_cb)) return;
    if (khes_res == static_cast<ID3D11Resource*>(g_res.composite_frame_cb)) return;

    const uint64_t khes_cur = g_svs_frame_seq;
    const float* khes_f = static_cast<const float*>(khes_data);

    // A structural miss here is not a drop - ring CBs legitimately carry other layouts between camera uploads; the
    // consume-side confirm is the arbiter. Locked fast path: identity compare + one 16-byte read.
    if (g_engcam_res) {
        if (khes_res == g_engcam_res && g_engcam_off >= 0) {
            const uint32_t khes_o = static_cast<uint32_t>(g_engcam_off);
            if ((khes_o + 4u) * 4u <= khes_bytes) {
                kh_upload_need(khes_data, (khes_o + 4u) * 4u);   // KH_UPLOAD_LAZY.
                const float* khes_r = khes_f + khes_o;
                if (khes_r[3] == 1.0f && kh_engcam_plausible(khes_r)) {
                    if (g_engcam_val_seq != khes_cur) {
                        g_engcam_vals_n = 0;
                        g_engcam_val_seq = khes_cur;
                    }

                    bool khes_have = false;

                    for (uint32_t khes_k = 0; khes_k < g_engcam_vals_n; ++khes_k) {
                        if (memcmp(g_engcam_vals[khes_k], khes_r, 12) == 0) {
                            khes_have = true;
                            break;
                        }
                    }

                    if (!khes_have) {
                        if (g_engcam_vals_n < 4u) {
                            g_engcam_vals[g_engcam_vals_n][0] = khes_r[0];
                            g_engcam_vals[g_engcam_vals_n][1] = khes_r[1];
                            g_engcam_vals[g_engcam_vals_n][2] = khes_r[2];
                            g_engcam_vals_n++;
                        }
                    }
                }
            }
        }
        return;
    }

    // Unlocked: bounded row scan into the per-frame value-deduped table.
    if (g_engcam_cand_seq != khes_cur) {
        g_engcam_cand_n = 0;
        g_engcam_cand_seq = khes_cur;
    }

    // Coarse proximity gate (table hygiene only; the consume decides): 2 m plus 8x the accepted latch step covers
    // any legitimate motion including teleport frames.
    const float khes_step = g_cam_step_m > 0.0f ? g_cam_step_m : 0.0f;
    const float khes_gate = 2.0f + 8.0f * khes_step;

    uint32_t khes_rows = khes_bytes / 16u;
    if (khes_rows > 64u) khes_rows = 64u;
    kh_upload_need(khes_data, khes_rows * 16u);   // KH_UPLOAD_LAZY.

    for (uint32_t khes_i = 0; khes_i < khes_rows; ++khes_i) {
        const float* khes_r = khes_f + khes_i * 4u;
        if (khes_r[3] != 1.0f) continue;
        if (!kh_engcam_plausible(khes_r)) continue;
        if (g_engcam_ref_ok &&
            (fabsf(khes_r[0] - g_engcam_ref[0]) > khes_gate ||
             fabsf(khes_r[1] - g_engcam_ref[1]) > khes_gate ||
             fabsf(khes_r[2] - g_engcam_ref[2]) > khes_gate)) continue;

        bool khes_seen = false;

        for (uint32_t khes_k = 0; khes_k < g_engcam_cand_n; ++khes_k) {
            if (memcmp(g_engcam_cand[khes_k].v, khes_r, 12) == 0) {
                g_engcam_cand[khes_k].sights++;
                khes_seen = true;
                break;
            }
        }

        if (!khes_seen) {
            if (g_engcam_cand_n < 32u) {
                KhEngCamCand& khes_c = g_engcam_cand[g_engcam_cand_n++];
                khes_c.v[0] = khes_r[0];
                khes_c.v[1] = khes_r[1];
                khes_c.v[2] = khes_r[2];
                khes_c.res = static_cast<void*>(khes_res);
                khes_c.off = static_cast<int32_t>(khes_i * 4u);
                khes_c.sights = 1;
            }
        }
    }
}

// KH_ENGCAM_PEEK: the consume's verdict with or without its side effects. khec_commit true is
// kh_engcam_consume (it moves the scan's proximity gate, adopts or drops the lock, latches the frame);
// false is kh_engcam_peek, which writes nothing and returns the value the consume would return now.
inline bool kh_engcam_eval(const float khec_view[4][4], float khec_cam[3], bool khec_commit) {
    if (!kh_engcam_on()) return false;

    const uint64_t khec_cur = g_svs_frame_seq;

    if (g_engcam_frame_seq == khec_cur) {
        if (!g_engcam_frame_taken) return false;

        float khec_lrec[3];
        kh_cam_of(khec_view, khec_lrec);

        if (!kh_engcam_agree4(g_engcam_frame_val, khec_lrec)) {
            return false;
        }

        khec_cam[0] = g_engcam_frame_val[0];
        khec_cam[1] = g_engcam_frame_val[1];
        khec_cam[2] = g_engcam_frame_val[2];
        return true;
    }

    float khec_rec[3];
    kh_cam_of(khec_view, khec_rec);   // KH_CAM_HONEST.

    // Near-origin stand-down, parity with the rebase family.
    if (khec_rec[0] * khec_rec[0] + khec_rec[1] * khec_rec[1] +
        khec_rec[2] * khec_rec[2] < 1.0f) return false;

    if (khec_commit) {   // KH_ENGCAM_PEEK.
        g_engcam_ref[0] = khec_rec[0];
        g_engcam_ref[1] = khec_rec[1];
        g_engcam_ref[2] = khec_rec[2];
        g_engcam_ref_ok = true;
    }

    const float* khec_v = nullptr;

    if (g_engcam_res) {
        if (g_engcam_val_seq != khec_cur || g_engcam_vals_n == 0) {
            // KH_ENGCAM_SILENT: a lock whose buffer has shown no camera row for KH_ENGCAM_SILENT_FRAMES
            // frames is let go and the hunt reopens (kh_engcam_wanted). Without it only a disagreeing sighting
            // drops the lock, so a buffer the engine stopped writing the row to would hold every consumer on the fold.
            if (khec_commit && khec_cur - g_engcam_val_seq > KH_ENGCAM_SILENT_FRAMES) {
                g_engcam_res = nullptr;
                g_engcam_off = -1;
            }
            return false;
        }

        for (uint32_t khec_k = 0; khec_k < g_engcam_vals_n; ++khec_k) {
            if (kh_engcam_agree4(g_engcam_vals[khec_k], khec_rec)) {
                khec_v = g_engcam_vals[khec_k];
                break;
            }
        }

        if (!khec_v) {
            if (khec_commit) {   // KH_ENGCAM_PEEK.
                g_engcam_res = nullptr;
                g_engcam_off = -1;
            }
            return false;
        }
    } else {
        if (g_engcam_cand_seq != khec_cur) return false;

        const KhEngCamCand* khec_best = nullptr;
        uint32_t khec_agree = 0;

        for (uint32_t khec_k = 0; khec_k < g_engcam_cand_n; ++khec_k) {
            if (!kh_engcam_agree4(g_engcam_cand[khec_k].v, khec_rec)) continue;
            khec_agree++;
            if (!khec_best || g_engcam_cand[khec_k].sights > khec_best->sights) {
                khec_best = &g_engcam_cand[khec_k];
            }
        }

        if (!khec_best) return false;

        if (khec_agree > 1u) {
            // Two distinct values both within 8 ulp of the recovery: the
            // one-value-per-frame property is violated and no pick is safe.
            return false;
        }

        if (khec_best->sights < 2u) return false;

        if (!khec_commit) {   // KH_ENGCAM_PEEK: the value the consume would lock.
            khec_cam[0] = khec_best->v[0];
            khec_cam[1] = khec_best->v[1];
            khec_cam[2] = khec_best->v[2];
            return true;
        }

        g_engcam_res = khec_best->res;
        g_engcam_off = khec_best->off;
        g_engcam_vals[0][0] = khec_best->v[0];
        g_engcam_vals[0][1] = khec_best->v[1];
        g_engcam_vals[0][2] = khec_best->v[2];
        g_engcam_vals_n = 1;
        g_engcam_val_seq = khec_cur;
        khec_v = g_engcam_vals[0];
    }

    if (!khec_commit) {   // KH_ENGCAM_PEEK: the locked value the consume would latch.
        khec_cam[0] = khec_v[0];
        khec_cam[1] = khec_v[1];
        khec_cam[2] = khec_v[2];
        return true;
    }

    g_engcam_frame_seq = khec_cur;
    g_engcam_frame_val[0] = khec_v[0];
    g_engcam_frame_val[1] = khec_v[1];
    g_engcam_frame_val[2] = khec_v[2];
    g_engcam_frame_taken = true;

    khec_cam[0] = g_engcam_frame_val[0];
    khec_cam[1] = g_engcam_frame_val[1];
    khec_cam[2] = g_engcam_frame_val[2];
    return true;
}

inline bool kh_engcam_consume(const float khec_view[4][4], float khec_cam[3]) {
    return kh_engcam_eval(khec_view, khec_cam, true);
}

// KH_ENGCAM_PEEK: for a reader that must not move the drawers' verdict (the DLS world pass's self-mask, which can run
// before the flush, whose view may differ).
inline bool kh_engcam_peek(const float khec_view[4][4], float khec_cam[3]) {
    return kh_engcam_eval(khec_view, khec_cam, false);
}
}