#pragma once

// The injection half (rendering_integration_injection.hpp): the engine camera scan (ENGCAM). Never include it on its
// own.
namespace RenderIntegration {

// The lock is (buffer identity, float offset), compared opaquely and never dereferenced.
struct EngCamCand {
    float   v[3] = {};   // Sighted value (bit-exact key).
    void*   res = nullptr;   // First sighting's buffer identity (opaque).
    int32_t off = -1;
    uint32_t sights = 0;   // Sightings of this exact value this frame.
};
// The engine camera scan (ENGCAM): the candidate locations, the values read there, the frame's value and the
// reference.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct EngCamState {
    EngCamCand cand[32];
    uint32_t cand_n = 0;
    uint64_t cand_seq = ~0ull;   // g_svs.frame_seq of the table.
    void*    res = nullptr;
    int32_t  off = -1;   // Locked float offset (-1 = none).
    // The locked buffer is a ring CB serving several shader layouts: a single last-sighting slot lets a foreign
    // layout's upload evict the camera row after it arrived this frame.
    float    vals[4][3] = {};   // Distinct sightings at the lock, this frame.
    uint32_t vals_n = 0;
    uint64_t val_seq = ~0ull;   // Frame stamp of the set.
    float    frame_val[3] = {};   // The frame's latched verdict.
    uint64_t frame_seq = ~0ull;
    bool     frame_taken = false;
    float    ref[3] = {};   // Last recovered camera (coarse).
    bool     ref_ok = false;   // Scan gate only - never decides.
};
static EngCamState g_engcam;

inline bool engcam_on() {
    // The feature toggle: the scan and the consume run whenever the funnel does (g_track_wanted gates it upstream).
    // What the locator asks the funnel to CAPTURE is engcam_wanted below, not this.
    return true;
}

// ENGCAM_GATE: the locator's demand on the upload funnel. Unlocked it hunts every constant-buffer map; locked, only
// a map of the locked buffer is wanted (losing the lock reopens the hunt). The other consumers keep their own terms. In
// practice the lock lands on the engine's per-draw buffer, so this saves nothing; it is kept because it is correct and
// equivalent. Render thread.
inline bool engcam_wanted(ID3D11Resource* res) {
    if (!engcam_on()) return false;
    return !g_engcam.res || static_cast<void*>(res) == g_engcam.res;
}

inline bool engcam_agree4(const float* a, const float* b) {

    float m = fabsf(a[0]);
    if (fabsf(a[1]) > m) m = fabsf(a[1]);
    if (fabsf(a[2]) > m) m = fabsf(a[2]);
    const float eps = 8.0f * (nextafterf(m, 3.0e38f) - m);
    return fabsf(a[0] - b[0]) <= eps &&
           fabsf(a[1] - b[1]) <= eps &&
           fabsf(a[2] - b[2]) <= eps;
}

inline bool engcam_plausible(const float* r) {
    if (!(r[0] == r[0]) || !(r[1] == r[1]) ||
        !(r[2] == r[2])) return false;
    const float ax = fabsf(r[0]);
    const float ay = fabsf(r[1]);
    const float az = fabsf(r[2]);
    const float m = ax > ay ? (ax > az ? ax : az)
                                        : (ay > az ? ay : az);
    return m >= 1.0f && m <= 60000.0f;
}

// ENGCAM_SILENT: frames (g_svs.frame_seq) a lock may go without a sighting before it is let go (engcam_eval).
static constexpr uint64_t ENGCAM_SILENT_FRAMES = 8;

inline void engcam_device_reset() {
    // Weak identities die with the device/session (the g_proj_locator rule).
    g_engcam.res = nullptr;
    g_engcam.off = -1;
    g_engcam.val_seq = ~0ull;
    g_engcam.vals_n = 0;
    g_engcam.frame_seq = ~0ull;
    g_engcam.frame_taken = false;
    g_engcam.cand_seq = ~0ull;
    g_engcam.cand_n = 0;
    g_engcam.ref_ok = false;
}

// The scan half: called beside locator_note_upload on every tracked upload. Render thread. Read-only on the upload.
inline void engcam_scan(ID3D11Resource* res, const void* data,
                        uint32_t bytes) {
    if (!engcam_on() || !res || !data || bytes < 16) return;
    // Never self-learn: locator_note_upload's own exclusion list.
    if (res == static_cast<ID3D11Resource*>(g_res.composite_cb)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.constant_buffer)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.frame_cb)) return;
    if (res == static_cast<ID3D11Resource*>(g_res.composite_frame_cb)) return;

    const uint64_t cur = g_svs.frame_seq;
    const float* f = static_cast<const float*>(data);

    // A structural miss here is not a drop - ring CBs legitimately carry other layouts between camera uploads; the
    // consume-side confirm is the arbiter. Locked fast path: identity compare + one 16-byte read.
    if (g_engcam.res) {
        if (res == g_engcam.res && g_engcam.off >= 0) {
            const uint32_t o = static_cast<uint32_t>(g_engcam.off);
            if ((o + 4u) * 4u <= bytes) {
                upload_need(data, (o + 4u) * 4u);   // UPLOAD_LAZY.
                const float* r = f + o;
                if (r[3] == 1.0f && engcam_plausible(r)) {
                    if (g_engcam.val_seq != cur) {
                        g_engcam.vals_n = 0;
                        g_engcam.val_seq = cur;
                    }

                    bool have = false;

                    for (uint32_t k = 0; k < g_engcam.vals_n; ++k) {
                        if (memcmp(g_engcam.vals[k], r, 12) == 0) {
                            have = true;
                            break;
                        }
                    }

                    if (!have) {
                        if (g_engcam.vals_n < 4u) {
                            g_engcam.vals[g_engcam.vals_n][0] = r[0];
                            g_engcam.vals[g_engcam.vals_n][1] = r[1];
                            g_engcam.vals[g_engcam.vals_n][2] = r[2];
                            g_engcam.vals_n++;
                        }
                    }
                }
            }
        }
        return;
    }

    // Unlocked: bounded row scan into the per-frame value-deduped table.
    if (g_engcam.cand_seq != cur) {
        g_engcam.cand_n = 0;
        g_engcam.cand_seq = cur;
    }

    // Coarse proximity gate (table hygiene only; the consume decides): 2 m plus 8x the accepted latch step covers
    // any legitimate motion including teleport frames.
    const float step = g_cam_step_m > 0.0f ? g_cam_step_m : 0.0f;
    const float gate = 2.0f + 8.0f * step;

    uint32_t rows = bytes / 16u;
    if (rows > 64u) rows = 64u;
    upload_need(data, rows * 16u);   // UPLOAD_LAZY.

    for (uint32_t i = 0; i < rows; ++i) {
        const float* r = f + i * 4u;
        if (r[3] != 1.0f) continue;
        if (!engcam_plausible(r)) continue;
        if (g_engcam.ref_ok &&
            (fabsf(r[0] - g_engcam.ref[0]) > gate ||
             fabsf(r[1] - g_engcam.ref[1]) > gate ||
             fabsf(r[2] - g_engcam.ref[2]) > gate)) continue;

        bool seen = false;

        for (uint32_t k = 0; k < g_engcam.cand_n; ++k) {
            if (memcmp(g_engcam.cand[k].v, r, 12) == 0) {
                g_engcam.cand[k].sights++;
                seen = true;
                break;
            }
        }

        if (!seen) {
            if (g_engcam.cand_n < 32u) {
                EngCamCand& c = g_engcam.cand[g_engcam.cand_n++];
                c.v[0] = r[0];
                c.v[1] = r[1];
                c.v[2] = r[2];
                c.res = static_cast<void*>(res);
                c.off = static_cast<int32_t>(i * 4u);
                c.sights = 1;
            }
        }
    }
}

// ENGCAM_PEEK: the consume's verdict with or without its side effects. commit true is
// engcam_consume (it moves the scan's proximity gate, adopts or drops the lock, latches the frame);
// false is engcam_peek, which writes nothing and returns the value the consume would return now.
inline bool engcam_eval(const float view[4][4], float cam[3], bool commit) {
    if (!engcam_on()) return false;

    const uint64_t cur = g_svs.frame_seq;

    if (g_engcam.frame_seq == cur) {
        if (!g_engcam.frame_taken) return false;

        float lrec[3];
        cam_of(view, lrec);

        if (!engcam_agree4(g_engcam.frame_val, lrec)) {
            return false;
        }

        cam[0] = g_engcam.frame_val[0];
        cam[1] = g_engcam.frame_val[1];
        cam[2] = g_engcam.frame_val[2];
        return true;
    }

    float rec[3];
    cam_of(view, rec);   // CAM_HONEST.

    // Near-origin stand-down, parity with the rebase family.
    if (rec[0] * rec[0] + rec[1] * rec[1] +
        rec[2] * rec[2] < 1.0f) return false;

    if (commit) {   // ENGCAM_PEEK.
        g_engcam.ref[0] = rec[0];
        g_engcam.ref[1] = rec[1];
        g_engcam.ref[2] = rec[2];
        g_engcam.ref_ok = true;
    }

    const float* v = nullptr;

    if (g_engcam.res) {
        if (g_engcam.val_seq != cur || g_engcam.vals_n == 0) {
            // ENGCAM_SILENT: a lock whose buffer has shown no camera row for ENGCAM_SILENT_FRAMES
            // frames is let go and the hunt reopens (engcam_wanted). Without it only a disagreeing sighting
            // drops the lock, so a buffer the engine stopped writing the row to would hold every consumer on the fold.
            if (commit && cur - g_engcam.val_seq > ENGCAM_SILENT_FRAMES) {
                g_engcam.res = nullptr;
                g_engcam.off = -1;
            }
            return false;
        }

        for (uint32_t k = 0; k < g_engcam.vals_n; ++k) {
            if (engcam_agree4(g_engcam.vals[k], rec)) {
                v = g_engcam.vals[k];
                break;
            }
        }

        if (!v) {
            if (commit) {   // ENGCAM_PEEK.
                g_engcam.res = nullptr;
                g_engcam.off = -1;
            }
            return false;
        }
    } else {
        if (g_engcam.cand_seq != cur) return false;

        const EngCamCand* best = nullptr;
        uint32_t agree = 0;

        for (uint32_t k = 0; k < g_engcam.cand_n; ++k) {
            if (!engcam_agree4(g_engcam.cand[k].v, rec)) continue;
            agree++;
            if (!best || g_engcam.cand[k].sights > best->sights) {
                best = &g_engcam.cand[k];
            }
        }

        if (!best) return false;

        if (agree > 1u) {
            // Two distinct values both within 8 ulp of the recovery: the
            // one-value-per-frame property is violated and no pick is safe.
            return false;
        }

        if (best->sights < 2u) return false;

        if (!commit) {   // ENGCAM_PEEK: the value the consume would lock.
            cam[0] = best->v[0];
            cam[1] = best->v[1];
            cam[2] = best->v[2];
            return true;
        }

        g_engcam.res = best->res;
        g_engcam.off = best->off;
        g_engcam.vals[0][0] = best->v[0];
        g_engcam.vals[0][1] = best->v[1];
        g_engcam.vals[0][2] = best->v[2];
        g_engcam.vals_n = 1;
        g_engcam.val_seq = cur;
        v = g_engcam.vals[0];
    }

    if (!commit) {   // ENGCAM_PEEK: the locked value the consume would latch.
        cam[0] = v[0];
        cam[1] = v[1];
        cam[2] = v[2];
        return true;
    }

    g_engcam.frame_seq = cur;
    g_engcam.frame_val[0] = v[0];
    g_engcam.frame_val[1] = v[1];
    g_engcam.frame_val[2] = v[2];
    g_engcam.frame_taken = true;

    cam[0] = g_engcam.frame_val[0];
    cam[1] = g_engcam.frame_val[1];
    cam[2] = g_engcam.frame_val[2];
    return true;
}

inline bool engcam_consume(const float view[4][4], float cam[3]) {
    return engcam_eval(view, cam, true);
}

// ENGCAM_PEEK: for a reader that must not move the drawers' verdict (the DLS world pass's self-mask, which can run
// before the flush, whose view may differ).
inline bool engcam_peek(const float view[4][4], float cam[3]) {
    return engcam_eval(view, cam, false);
}
}