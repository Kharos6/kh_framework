#pragma once

// The injection half (rendering_integration_injection.hpp): the dynamic-light harvest, part 2: the structural tests,
// the windows, origin derivation, the anchors and the pool. Never include it on its own.
namespace RenderIntegration {

// Structural affine test shared by the derivation. Form codes: 0 = 4x4 row-vector ((0,0,0,1) column checked); 1 = 4x4
// transposed / HLSL column-major (lane checked); 2 = 4x3 in 3 registers, rows [a, b, c, t] (the shadow_mats
// convention); 3 = 4 registers, each one row's xyz (w lanes ignored, none checked). The full scan tries all four.
inline uint32_t dl_form_floats(uint8_t form) { return form == 2 ? 12u : 16u; }

inline bool dl_cand_structure(const float* mf, uint8_t form, float (&v)[4][4]) {
    if (form <= 1) {
        for (int i = 0; i < 4; ++i) {
            for (int k = 0; k < 4; ++k) {
                v[i][k] = form == 0 ? mf[i * 4 + k] : mf[k * 4 + i];
            }
        }

        if (fabsf(v[0][3]) > 0.001f || fabsf(v[1][3]) > 0.001f ||
            fabsf(v[2][3]) > 0.001f || fabsf(v[3][3] - 1.0f) > 0.001f) {
            return false;
        }
    } else if (form == 2) {
        for (int k = 0; k < 3; ++k) {
            v[0][k] = mf[k * 4 + 0];
            v[1][k] = mf[k * 4 + 1];
            v[2][k] = mf[k * 4 + 2];
            v[3][k] = mf[k * 4 + 3];
            v[k][3] = 0.0f;
        }

        v[3][3] = 1.0f;
    } else {
        for (int i = 0; i < 4; ++i) {
            v[i][0] = mf[i * 4 + 0];
            v[i][1] = mf[i * 4 + 1];
            v[i][2] = mf[i * 4 + 2];
            v[i][3] = 0.0f;
        }

        v[3][3] = 1.0f;
    }

    for (int r = 0; r < 3; ++r) {
        const float n2 = v[r][0] * v[r][0] + v[r][1] * v[r][1] +
                         v[r][2] * v[r][2];
        if (n2 < 0.96f || n2 > 1.04f) return false;
    }

    for (int a = 0; a < 3; ++a) {
        for (int b = a + 1; b < 3; ++b) {
            const float d = v[a][0] * v[b][0] + v[a][1] * v[b][1] +
                            v[a][2] * v[b][2];
            if (fabsf(d) > 0.02f) return false;
        }
    }

    return true;
}

inline bool dl_rot_matches(const float (&v)[4][4], const float* cols) {
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            if (fabsf(v[j][k] - cols[k * 4 + j]) > DL_DERIVE_ROT_TOL) return false;
        }
    }

    return true;
}

inline bool dl_rot_identity(const float (&v)[4][4]) {
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            const float want = j == k ? 1.0f : 0.0f;
            if (fabsf(v[j][k] - want) > DL_DERIVE_ROT_TOL) return false;
        }
    }

    return true;
}

// The frame-structure proof shared by every route: bare triples must be near-exact - a random constant must not fake a
// camera.
inline bool dl_origin_validate(float ex, float ey, float ez,
                               float ytol, float gtol, float q,
                               float& ox, float& oz, float& res) {
    if (!(q > 0.0f)) return false;   // DL_ORIGIN_GRID: no grid known yet.
    if (fabsf(ey) > ytol) {
        return false;
    }

    const float sx = floorf(ex / q + 0.5f) * q;
    const float sz = floorf(ez / q + 0.5f) * q;
    const float rx = fabsf(ex - sx);
    const float rz = fabsf(ez - sz);

    if (rx > gtol || rz > gtol) {
        return false;
    }

    ox = sx;
    oz = sz;
    res = rx > rz ? rx : rz;
    return true;
}

// These three sit together; keep them together. dynlights_derive_origin, below, uses dl_win_slot and
// dl_win_origin_of; the presence pass and the sampler (rendering/dynamic_lights_capture.hpp) reach
// dl_win_origin_of through its declaration above dl_view_live. Nothing frees an entry (only the session reset),
// so once the engine has cycled more buffers than the table holds a lookup recycles one: the entry failing longest, and
// among equals the one looked up longest ago (DL_SLOT_LRU: do not break ties to slot 0 - a table full of buffers the
// engine has stopped using, each at a run of 0, hands slot 0 to every new buffer, evicting the one before it, a buffer
// placed this same harvest included).
inline uint32_t dl_win_slot(const void* buf) {
    const uint32_t lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buf));
    const uint64_t use = ++g_dl.win_use_n;   // DL_SLOT_LRU.

    for (uint32_t i = 0; i < DL_WIN_SLOTS; ++i) {
        if (g_dl.win_fail_lo[i] == lo) {
            g_dl.win_use[i] = use;
            return i;
        }
    }

    for (uint32_t i = 0; i < DL_WIN_SLOTS; ++i) {
        if (g_dl.win_fail_lo[i] == 0) {
            g_dl.win_fail_lo[i] = lo;
            g_dl.win_fail_run[i] = 0;
            g_dl.win_use[i] = use;
            return i;
        }
    }

    uint32_t worst = 0;

    for (uint32_t i = 1; i < DL_WIN_SLOTS; ++i) {
        const uint32_t r = g_dl.win_fail_run[i], rw = g_dl.win_fail_run[worst];
        if (r > rw || (r == rw && g_dl.win_use[i] < g_dl.win_use[worst])) worst = i;
    }

    g_dl.win_fail_lo[worst] = lo;
    g_dl.win_fail_run[worst] = 0;
    g_dl.win_use[worst] = use;
    // DL_SLOT_RECYCLE: the slot now names another buffer. The origin and hints resolved for the old one are not this
    // buffer's - dl_win_origin_of would hand them out as the origin this buffer last resolved to.
    g_dl.win_ox[worst] = 0.0f;
    g_dl.win_oz[worst] = 0.0f;
    g_dl.win_oset[worst] = 0;
    g_dl.win_oseq[worst] = 0;
    g_dl.win_hint_set[worst] = 0;
    g_dl.win_hint_slot[worst] = -1;
    g_dl.win_hint_wslot[worst] = -1;
    return worst;
}

// Read-only: the origin this buffer last resolved to, at whatever harvest (dl_win_origin_seq says which).
// Deliberately not dl_win_slot, which allocates - the ranking must ask without changing the table.
inline bool dl_win_origin_of(const void* buf, float& ox, float& oz) {
    const uint32_t lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buf));
    if (lo == 0) return false;

    for (uint32_t i = 0; i < DL_WIN_SLOTS; ++i) {
        if (g_dl.win_fail_lo[i] == lo && g_dl.win_oset[i]) {
            ox = g_dl.win_ox[i];
            oz = g_dl.win_oz[i];
            return true;
        }
    }

    return false;
}

// DL_PRES_FRESH: the harvest (g_dl.harvest_seq) that set this buffer's origin; 0 = none. Read-only, as
// dl_win_origin_of.
inline uint64_t dl_win_origin_seq(const void* buf) {
    const uint32_t lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buf));
    if (lo == 0) return 0;

    for (uint32_t i = 0; i < DL_WIN_SLOTS; ++i) {
        if (g_dl.win_fail_lo[i] == lo && g_dl.win_oset[i]) return g_dl.win_oseq[i];
    }

    return 0;
}

// DL_PRES_CUR: have two cameras (x, z) crossed no line of the land grid's cells, nor a half-cell line, between
// them? The engine's origins sit on whole cells (DL_ORIGIN_GRID); DL_PRES_CUR's premise is that they move only as
// the render camera crosses one of those lines - the cell's edge or its middle, of the engine's step or a multiple of
// it - so that a camera that crossed none kept its origin. Not measured in the field. No cell read yet: no.
inline bool dl_same_cells(float ax, float az, float bx, float bz) {
    const float c = dl_grid_base();
    if (!(c > 0.0f)) return false;
    auto one = [c](float a, float b) {
        return floorf(a / c) == floorf(b / c) &&
               floorf(a / c + 0.5f) == floorf(b / c + 0.5f);
    };
    return one(ax, bx) && one(az, bz);
}

// DL_PRES_CUR: may an upload of span seq rewrite through the origin (ox, oz) its buffer resolved? When
// the last harvest set that origin (DL_PRES_FRESH), or placed another window at the same origin (within the vote's
// residual) and the camera now is on the same cells as that window's capture camera (dl_same_cells): the frame's
// lists sit at one origin, which moves only as the camera crosses cells, so a buffer that got no window is placed by
// its neighbours'. A window is authored only on the routine quota (the span's first uploads), an unknown light or a
// buffer with no origin, and a harvest places few of those - without this a moving light whose lists come later in the
// frame is never rewritten: it freezes until it leaves the 1 m match cube and is sighted as new. The camera test keeps
// a harvest that placed windows on both sides of a crossing (a span holds two frames) from vouching for the old cell,
// and it vouches only for the one origin whose camera is on the camera's cells now: two such (a window placed at a
// wrong origin after the crossing) vouch for neither, nor does a harvest that placed a window it could not weigh (no
// live camera, or past DL_CUR_N origins: cur_amb). Only the clear's own view is a camera (dl_view_live). Not
// covered, for every buffer at the origin and one span: a rebase its premise does not see (dl_same_cells - a rule
// with hysteresis or a radius, an origin keyed on something other than the render camera, a view out of step with
// the lists) and a window placed wrong at an origin no window of the harvest contradicts - DL_PRES_FRESH's own
// residual, which reached only that harvest's windows' buffers; DL_PRES_STALE stands behind both.
inline bool dl_pres_fresh(const void* buf, float ox, float oz, uint64_t seq) {
    if (dl_win_origin_seq(buf) + 1 == seq) return true;
    if (g_dl.cur_oseq == 0 || g_dl.cur_oseq + 1 != seq || g_dl.cur_amb || !dl_view_live()) return false;
    int hit = -1;
    for (uint32_t i = 0; i < g_dl.cur_n && hit < 0; ++i) {
        if (fabsf(g_dl.cur_o[i][0] - ox) <= DL_ORIGIN_RES_MAX &&
            fabsf(g_dl.cur_o[i][1] - oz) <= DL_ORIGIN_RES_MAX) {
            hit = static_cast<int>(i);
        }
    }
    if (hit < 0) return false;
    float cam[3];
    cam_of(g_ro.cycle_pv.view, cam);
    for (uint32_t i = 0; i < g_dl.cur_n; ++i) {   // The hit's camera here, and no other origin's.
        const bool here = dl_same_cells(cam[0], cam[2], g_dl.cur_o[i][2], g_dl.cur_o[i][3]);
        if (here != (static_cast<int>(i) == hit)) return false;
    }
    return true;
}

inline bool dynlights_derive_origin(const uint8_t* win, const DlWindowMeta& m,
                                    const float* recs, uint32_t rec_n,
                                    float& d_ox, float& d_oz, float& d_res,
                                    int32_t& slot_out, int32_t& wslot_out) {
    if (!m.view_ok) {   // DL_WIN_VIEW: the view this window was drawn under.
        return false;
    }

    const float* bcols = m.view;
    const float* bcam = m.cam;
    const uint8_t* vs_base = win + DL_STAGE_BYTES;

    auto block = [&](int32_t s) {
        return reinterpret_cast<const float*>(vs_base + s * DL_VS_BYTES);
    };

    // Applied at candidate selection and in the hint, so impostors die at candidacy. The invariant: the engine culls
    // its light list to the camera, so a correct origin places at least one of a window's lights near the capture
    // camera and a wrong one places them all far away. No 'record 0 is near' shortcut: the list is not distance-sorted.
    float rxz[DL_RANGE_PROBE][3];   // x, y, z - Y is needed to match anchors.
    uint32_t rn = 0;

    for (uint32_t i = 0; i < rec_n && rn < DL_RANGE_PROBE; ++i) {
        const float* rr = recs + i * 24;

        if (dl_finite(rr[0]) && dl_finite(rr[1]) && dl_finite(rr[2])) {
            rxz[rn][0] = rr[0];
            rxz[rn][1] = rr[1];
            rxz[rn][2] = rr[2];
            rn++;
        }
    }

    // How near this origin puts the window's closest light to the capture camera. Negative when the window carries no
    // usable record.
    auto near_of = [&](float cox, float coz) {
        float best = -1.0f;

        for (uint32_t i = 0; i < rn; ++i) {
            const float rx = cox + rxz[i][0] - bcam[0];
            const float rz = coz + rxz[i][2] - bcam[2];
            const float d2 = rx * rx + rz * rz;
            if (best < 0.0f || d2 < best) best = d2;
        }

        return best < 0.0f ? -1.0f : sqrtf(best);
    };

    auto range_ok = [&](float cox, float coz) {
        if (rn == 0) return true;
        const float nr = near_of(cox, coz);
        if (nr >= 0.0f && nr <= DL_ORIGIN_RANGE_M) return true;
        return false;
    };

    // Residual is the rank where proximity (DL_NEAR_MARGIN_M, in offer) does not decide - never the route's
    // kind (do not prefer matrices over triples: that let 0.29 m impostors outrank 0.002 m truth); the authorship
    // ceiling gates every offer; zero-origin tracked separately (the degeneracy bin).
    float b_ox = 0.0f, b_oz = 0.0f, b_res = 1.0e9f;
    int32_t b_slot = -1, b_wslot = -1;
    uint32_t b_off = 0, b_woff = 0;
    uint8_t b_form = 0, b_wform = 0, b_kind = 0;
    bool have_nz = false, have_z = false;
    float z_res = 1.0e9f;

    // A strict '<' would let slot order choose among equal residuals, which changes with the camera. Ties break by
    // evidence: 1. CONTINUITY - the origin this window last resolved to (only candidates offered THIS frame can
    // win). 2. CORROBORATION - how many of the window's records land on a known anchor. 3. Neither: the strictly
    // lower residual, else the first offered.
    float prev_ox = 0.0f, prev_oz = 0.0f;
    const bool prev_ok = dl_win_origin_of(m.buf, prev_ox, prev_oz);

    auto corrob = [&](float ox, float oz) {
        uint32_t n = 0;

        for (uint32_t i = 0; i < rn; ++i) {
            const float x = ox + rxz[i][0];
            const float y = rxz[i][1];
            const float z = oz + rxz[i][2];

            for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
                if (fabsf(g_dl.anchor_pos[a][0] - x) < DL_MATCH_M &&
                    fabsf(g_dl.anchor_pos[a][1] - y) < DL_MATCH_M &&
                    fabsf(g_dl.anchor_pos[a][2] - z) < DL_MATCH_M) {
                    n++;
                    break;
                }
            }
        }

        return n;
    };

    auto offer = [&](float ox, float oz, float res, uint8_t kind,
                     int32_t slot, uint32_t off, uint8_t form,
                     int32_t wslot, uint32_t woff, uint8_t wform) {
        // A wandering structure is not an origin, and its residual says so (impostors land at 0.008-0.020 against the
        // true candidate at 0.000).
        if (res > DL_ORIGIN_RES_MAX) {
            return;   // Never authors.
        }

        if (!range_ok(ox, oz)) return;   // Structural impostors die here.

        if (ox == 0.0f && oz == 0.0f) {
            have_z = true;
            if (res < z_res) z_res = res;
            return;
        }

        // The engine culls its light list to the camera, so a correct origin must place at least one of the window's
        // lights near it and a wrong one displaces every light. Proximity is the primary key, residual the tiebreak,
        // and the margin keeps noise from flapping the choice.
        bool take = false;
        const float nr_new = near_of(ox, oz);
        const float nr_old = have_nz ? near_of(b_ox, b_oz) : -1.0f;
        const bool near_decides =
            have_nz && nr_new >= 0.0f && nr_old >= 0.0f &&
            fabsf(nr_new - nr_old) > DL_NEAR_MARGIN_M;

        if (!have_nz) {
            take = true;
        } else if (ox == b_ox && oz == b_oz) {
            return;   // Same origin by another route: nothing to decide.
        } else if (near_decides) {
            take = nr_new < nr_old;
        } else if (res < b_res - DL_RES_TIE) {
            take = true;
        } else if (res <= b_res + DL_RES_TIE) {
            const bool cont_new = prev_ok && ox == prev_ox && oz == prev_oz;
            const bool cont_old = prev_ok &&
                                  b_ox == prev_ox && b_oz == prev_oz;

            if (cont_new != cont_old) {
                take = cont_new;
                have_nz = true;
                if (take) {
                    b_ox = ox; b_oz = oz; b_res = res;
                    b_kind = kind;
                    b_slot = slot; b_off = off; b_form = form;
                    b_wslot = wslot; b_woff = woff; b_wform = wform;
                }
                return;
            }

            {
                const uint32_t cn = corrob(ox, oz);
                const uint32_t co = corrob(b_ox, b_oz);

                if (cn != co) {
                    take = cn > co;
                } else {
                    take = res < b_res;
                }
            }

        }

        if (take) {
            have_nz = true;
            b_ox = ox; b_oz = oz; b_res = res;
            b_kind = kind;
            b_slot = slot; b_off = off; b_form = form;
            b_wslot = wslot; b_woff = woff; b_wform = wform;
        }
    };

    float tox, toz, tres;

    // Memoized fast path (non-zero winners only): this window's memo, not the session's.
    const uint32_t ws = dl_win_slot(m.buf);
    const uint8_t  h_kind  = g_dl.win_hint_set[ws] ? g_dl.win_hint_kind[ws]  : 0;
    const int32_t  h_slot  = g_dl.win_hint_set[ws] ? g_dl.win_hint_slot[ws]  : -1;
    const uint32_t h_off   = g_dl.win_hint_set[ws] ? g_dl.win_hint_off[ws]   : 0;
    const uint8_t  h_form  = g_dl.win_hint_set[ws] ? g_dl.win_hint_form[ws]  : 0;
    const int32_t  h_wslot = g_dl.win_hint_set[ws] ? g_dl.win_hint_wslot[ws] : -1;
    const uint32_t h_woff  = g_dl.win_hint_set[ws] ? g_dl.win_hint_woff[ws]  : 0;
    const uint8_t  h_wform = g_dl.win_hint_set[ws] ? g_dl.win_hint_wform[ws] : 0;

    if (h_slot >= 0 && h_slot < static_cast<int32_t>(DL_VS_SLOTS)) {
        const uint32_t mfl = m.vs_bytes[h_slot] / 4u;
        bool hit = false;

        if (h_kind == 2) {
            if (h_off + 4 <= mfl) {
                const float* d_t = block(h_slot) + h_off;

                if (dl_finite(d_t[0]) && dl_finite(d_t[1]) && dl_finite(d_t[2]) && dl_origin_validate(bcam[0] - d_t[0], bcam[1] - d_t[1],
                                       bcam[2] - d_t[2], 0.5f, 0.25f, dl_grid(),
                                       tox, toz, tres) && tres <= DL_ORIGIN_RES_MAX && range_ok(tox, toz) && !(tox == 0.0f && toz == 0.0f)) {
                    hit = true;
                }
            }
        } else if (h_kind == 3 && h_wslot >= 0 &&
                   h_wslot < static_cast<int32_t>(DL_VS_SLOTS)) {
            const uint32_t wfl = m.vs_bytes[h_wslot] / 4u;

            if (h_off + 4 <= mfl &&
                h_woff + dl_form_floats(h_wform) <= wfl) {
                const float* d_t = block(h_slot) + h_off;
                float vw[4][4];

                if (dl_finite(d_t[0]) && dl_finite(d_t[1]) && dl_finite(d_t[2]) && dl_cand_structure(block(h_wslot) + h_woff,
                                      h_wform, vw) && dl_rot_identity(vw) && dl_origin_validate(vw[3][0] - d_t[0], vw[3][1] - d_t[1],
                                       vw[3][2] - d_t[2], 0.5f, 0.25f, dl_grid(),
                                       tox, toz, tres) && tres <= DL_ORIGIN_RES_MAX && range_ok(tox, toz) && !(tox == 0.0f && toz == 0.0f)) {
                    hit = true;
                }
            }
        } else if (h_kind == 1 && h_wslot >= 0 &&
                   h_wslot < static_cast<int32_t>(DL_VS_SLOTS)) {
            const uint32_t wfl = m.vs_bytes[h_wslot] / 4u;

            if (h_off + dl_form_floats(h_form) <= mfl &&
                h_woff + dl_form_floats(h_wform) <= wfl) {
                float vm[4][4], vw[4][4];

                if (dl_cand_structure(block(h_slot) + h_off,
                                      h_form, vm) &&
                    dl_rot_matches(vm, bcols) &&
                    dl_cand_structure(block(h_wslot) + h_woff,
                                      h_wform, vw) &&
                    dl_rot_identity(vw)) {
                    float d_cam[3];
                    cam_of(vm, d_cam);

                    if (dl_finite(d_cam[0]) && dl_finite(d_cam[1]) && dl_finite(d_cam[2]) && dl_origin_validate(bcam[0] - d_cam[0] - vw[3][0],
                                           bcam[1] - d_cam[1] - vw[3][1],
                                           bcam[2] - d_cam[2] - vw[3][2],
                                           DL_DERIVE_Y_TOL, DL_DERIVE_GRID_TOL, dl_grid_base(),
                                           tox, toz, tres) && tres <= DL_ORIGIN_RES_MAX && range_ok(tox, toz) && !(tox == 0.0f && toz == 0.0f)) {
                        hit = true;
                    }
                }
            }
        }

        // A drifting hit is not a hit: a real origin does not wander. If the memo produces an origin other than the
        // one this window holds, it has stopped describing this window.
        if (hit) {
            float px = 0.0f, pz = 0.0f;

            if (dl_win_origin_of(m.buf, px, pz) &&
                (px != tox || pz != toz)) {
                hit = false;   // Fall through to the full scan.
            }
        }

        if (hit) {
            if (h_kind == 1) dl_grid_learn(tox, toz);   // DL_ORIGIN_GRID: a strong route's.
            d_ox = tox;
            d_oz = toz;
            d_res = tres;
            slot_out = h_slot;
            wslot_out = h_wslot;
            return true;
        }
    }

    // Full scan: matrices first.
    struct DlMvCand { float cam[3]; int32_t slot; uint32_t off; uint8_t form; };
    struct DlWCand { float t[3]; int32_t slot; uint32_t off; uint8_t form; };
    DlMvCand mv[8];
    DlWCand wm[8];
    uint32_t mv_n = 0, wm_n = 0;

    for (uint32_t s = 0; s < DL_VS_SLOTS; ++s) {
        const uint32_t fl = m.vs_bytes[s] / 4u;
        if (fl < 12) continue;
        const float* base = block(static_cast<int32_t>(s));

        for (uint32_t d_off = 0; d_off + 12 <= fl; d_off += 4) {
            {   // DL_SCAN_PRE - each form reads its rotation from the same nine floats here (as rows or as columns:
                // [0..2], [4..6], [8..10]) and dl_cand_structure fails it when a row's squared norm is outside
                // [0.96, 1.04]. One of the nine past 1.03 in magnitude puts its row past 1.04 in either reading - a
                // sum of squares only grows, rounded or not; all nine under 0.56 keep every row under 0.95. A NaN
                // in a row makes its norm NaN, which the test passes, so a NaN among the nine leaves the offset to
                // the forms. Such an offset fails every form, and only a passing form runs anything below: it is
                // skipped here, in offset order. Most offsets of an engine buffer hold no rotation (the scan's bulk).
                // Edit with dl_cand_structure's bounds.
                static constexpr uint32_t rot9[9] = { 0, 1, 2, 4, 5, 6, 8, 9, 10 };
                bool pre_nan = false, pre_big = false, pre_small = true;
                for (const uint32_t pre_k : rot9) {
                    const float pre_x = base[d_off + pre_k];
                    if (!(pre_x == pre_x)) pre_nan = true;
                    if (fabsf(pre_x) > 1.03f) pre_big = true;
                    if (!(fabsf(pre_x) < 0.56f)) pre_small = false;
                }
                if ((pre_big || pre_small) && !pre_nan) continue;
            }
            for (uint8_t d_form = 0; d_form < 4; ++d_form) {
                if (d_off + dl_form_floats(d_form) > fl) continue;
                float v[4][4];
                if (!dl_cand_structure(base + d_off, d_form, v)) continue;

                if (dl_rot_matches(v, bcols)) {
                    float d_cam[3];
                    cam_of(v, d_cam);

                    if (!dl_finite(d_cam[0]) || !dl_finite(d_cam[1]) || !dl_finite(d_cam[2])) {
                        continue;
                    }

                    if (dl_origin_validate(bcam[0] - d_cam[0], bcam[1] - d_cam[1],
                                           bcam[2] - d_cam[2],
                                           DL_DERIVE_Y_TOL, DL_DERIVE_GRID_TOL, dl_grid_base(),
                                           tox, toz, tres)) {
                        offer(tox, toz, tres, static_cast<uint8_t>(0),
                              static_cast<int32_t>(s), d_off, d_form, -1, 0, 0);
                    }

                    if (mv_n < 8) {
                        mv[mv_n].cam[0] = d_cam[0];
                        mv[mv_n].cam[1] = d_cam[1];
                        mv[mv_n].cam[2] = d_cam[2];
                        mv[mv_n].slot = static_cast<int32_t>(s);
                        mv[mv_n].off = d_off;
                        mv[mv_n].form = d_form;
                        mv_n++;
                    }
                } else if (dl_rot_identity(v)) {
                    if (dl_finite(v[3][0]) && dl_finite(v[3][1]) && dl_finite(v[3][2]) &&
                        fabsf(v[3][0]) < 1.0e7f && fabsf(v[3][1]) < 1.0e7f &&
                        fabsf(v[3][2]) < 1.0e7f && wm_n < 8) {
                        wm[wm_n].t[0] = v[3][0];
                        wm[wm_n].t[1] = v[3][1];
                        wm[wm_n].t[2] = v[3][2];
                        wm[wm_n].slot = static_cast<int32_t>(s);
                        wm[wm_n].off = d_off;
                        wm[wm_n].form = d_form;
                        wm_n++;
                    }
                }
            }
        }
    }

    // MV x W pairs (W' light-frame yields the origin; W true yields the tracked (0,0) degeneracy).
    for (uint32_t a = 0; a < mv_n; ++a) {
        for (uint32_t b = 0; b < wm_n; ++b) {

            if (dl_origin_validate(bcam[0] - mv[a].cam[0] - wm[b].t[0],
                                   bcam[1] - mv[a].cam[1] - wm[b].t[1],
                                   bcam[2] - mv[a].cam[2] - wm[b].t[2],
                                   DL_DERIVE_Y_TOL, DL_DERIVE_GRID_TOL, dl_grid_base(),
                                   tox, toz, tres)) {
                offer(tox, toz, tres, 1,
                      mv[a].slot, mv[a].off, mv[a].form,
                      wm[b].slot, wm[b].off, wm[b].form);
            }
        }
    }

    // Kinds 2/3: bare float triples (tight tolerances - a round constant must not fake a camera or an object position).
    for (uint32_t s = 0; s < DL_VS_SLOTS; ++s) {
        const uint32_t fl = m.vs_bytes[s] / 4u;
        if (fl < 4) continue;
        const float* base = block(static_cast<int32_t>(s));

        for (uint32_t d_off = 0; d_off + 4 <= fl; d_off += 4) {
            const float* d_t = base + d_off;

            if (!dl_finite(d_t[0]) || !dl_finite(d_t[1]) || !dl_finite(d_t[2])) continue;
            if (fabsf(d_t[0]) > 1.0e7f || fabsf(d_t[1]) > 1.0e7f || fabsf(d_t[2]) > 1.0e7f) continue;

            // t = camera in the light frame.
            if (dl_origin_validate(bcam[0] - d_t[0], bcam[1] - d_t[1],
                                   bcam[2] - d_t[2], 0.5f, 0.25f, dl_grid(),
                                   tox, toz, tres)) {
                offer(tox, toz, tres, 2,
                      static_cast<int32_t>(s), d_off, 0, -1, 0, 0);
            }

            // t = object position in the light frame, against each true-world W translation.
            for (uint32_t b = 0; b < wm_n; ++b) {
                if (dl_origin_validate(wm[b].t[0] - d_t[0], wm[b].t[1] - d_t[1],
                                       wm[b].t[2] - d_t[2], 0.5f, 0.25f, dl_grid(),
                                       tox, toz, tres)) {
                    offer(tox, toz, tres, 3,
                          static_cast<int32_t>(s), d_off, 0,
                          wm[b].slot, wm[b].off, wm[b].form);
                }
            }
        }
    }

    if (have_nz) {
        d_ox = b_ox;
        d_oz = b_oz;
        d_res = b_res;
        slot_out = b_slot;
        wslot_out = b_wslot;

        if (b_kind <= 1) dl_grid_learn(b_ox, b_oz);   // DL_ORIGIN_GRID: routes 0 / 1 only.
        g_dl.win_hint_set[ws] = 1;
        g_dl.win_hint_kind[ws] = b_kind;
        g_dl.win_hint_slot[ws] = b_slot;
        g_dl.win_hint_off[ws] = b_off;
        g_dl.win_hint_form[ws] = b_form;
        g_dl.win_hint_wslot[ws] = b_wslot;
        g_dl.win_hint_woff[ws] = b_woff;
        g_dl.win_hint_wform[ws] = b_wform;

        return true;
    }

    if (have_z && range_ok(0.0f, 0.0f)) {
        // The true-world degeneracy: accepted only alone, never memoized, and only when it places this list near the
        // camera like any other candidate.
        d_ox = 0.0f;
        d_oz = 0.0f;
        d_res = z_res;
        slot_out = -1;
        wslot_out = -1;
        return true;
    }

    return false;
}

// Anchor-table upsert: exact-derivation lights become long-lived surveying knowledge. Match-in-place refresh; oldest
// evicted when full.
inline void dynlights_anchor_upsert(float x, float y, float z, uint64_t now) {
    int oldest = 0;

    for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
        if (fabsf(g_dl.anchor_pos[a][0] - x) < DL_MATCH_M &&
            fabsf(g_dl.anchor_pos[a][1] - y) < DL_MATCH_M &&
            fabsf(g_dl.anchor_pos[a][2] - z) < DL_MATCH_M) {
            g_dl.anchor_pos[a][0] = x;
            g_dl.anchor_pos[a][1] = y;
            g_dl.anchor_pos[a][2] = z;
            g_dl.anchor_stamp[a] = now;
            return;
        }

        if (g_dl.anchor_stamp[a] < g_dl.anchor_stamp[oldest]) oldest = static_cast<int>(a);
    }

    uint32_t slot;

    if (g_dl.anchor_n < DL_ANCHOR_N) {
        slot = g_dl.anchor_n++;
    } else {
        slot = static_cast<uint32_t>(oldest);
    }

    g_dl.anchor_pos[slot][0] = x;
    g_dl.anchor_pos[slot][1] = y;
    g_dl.anchor_pos[slot][2] = z;
    g_dl.anchor_stamp[slot] = now;
}

// A pool record is re-sighted by every harvest that carries its light; an anchor is refreshed only when an exact
// derivation lands on it again, so a wrong one stands until its TTL unless this test convicts it. Drop the anchor,
// never the pool.

inline bool dynlights_anchor_ghost(const float* a) {
    const float q = dl_grid();   // DL_ORIGIN_GRID: no grid learned, no conviction.
    if (!(q > 0.0f)) return false;
    for (uint32_t p = 0; p < g_dl.pool_n; ++p) {
        const DlPoolLight& e = g_dl.pool[p];
        // Only a corroborated light may convict an anchor: otherwise a bad origin reaching both the pool and the table
        // lets the sweep drop the real anchor and the ghost alike, taking the vote out entirely.
        if (e.sightings < DL_DENSE_SIGHTS) continue;
        const float* r = e.rec;
        if (fabsf(r[1] - a[1]) > DL_MATCH_M) continue;   // Different height: different
                                                                    // light.
        const float dx = r[0] - a[0];
        const float dz = r[2] - a[2];
        const float sx = floorf(dx / q + 0.5f) * q;
        const float sz = floorf(dz / q + 0.5f) * q;
        if (sx == 0.0f && sz == 0.0f) continue;   // Same place: this is the light.
        if (fabsf(dx - sx) > DL_MATCH_M) continue;
        if (fabsf(dz - sz) > DL_MATCH_M) continue;
        return true;
    }

    return false;
}

// The anchor vote (a window dynlights_derive_origin could not place): each finite record against each anchor at its
// height (within 0.25 m) a whole number of learned-grid steps away (within the vote's residual) votes for that origin
// (8 candidates at most). It wins with at least two votes and two more than the runner-up, or as the only candidate at
// a residual within 5 mm. Reads only: it writes nothing but ox / oz.
inline bool dynlights_pool_vote(const float* l, uint32_t total,
                                float& ox, float& oz) {
    if (g_dl.anchor_n == 0 || total == 0) return false;
    const float q = dl_grid();   // DL_ORIGIN_GRID: no grid learned, no vote.
    if (!(q > 0.0f)) return false;
    float cox[8], coz[8], cres[8];
    uint32_t cv[8], cn = 0;

    for (uint32_t i = 0; i < total; ++i) {
        const float* r = l + i * 24;
        if (!dl_finite(r[0]) || !dl_finite(r[1]) || !dl_finite(r[2])) continue;

        for (uint32_t d = 0; d < g_dl.anchor_n; ++d) {
            // Anchors are derivation-written only - exogenous truth, never the vote's own output.
            if (fabsf(g_dl.anchor_pos[d][1] - r[1]) > 0.25f) continue;   // Same light: identical Y.
            const float ex = g_dl.anchor_pos[d][0] - r[0];
            const float ez = g_dl.anchor_pos[d][2] - r[2];
            const float sx = floorf(ex / q + 0.5f) * q;
            const float sz = floorf(ez / q + 0.5f) * q;
            const float rx = fabsf(ex - sx);
            const float rz = fabsf(ez - sz);
            if (rx > DL_ORIGIN_RES_MAX || rz > DL_ORIGIN_RES_MAX) continue;
            const float res = rx > rz ? rx : rz;
            bool found = false;

            for (uint32_t c = 0; c < cn; ++c) {
                if (cox[c] == sx && coz[c] == sz) {
                    cv[c]++;
                    if (res < cres[c]) cres[c] = res;
                    found = true;
                    break;
                }
            }

            if (!found && cn < 8) {
                cox[cn] = sx;
                coz[cn] = sz;
                cres[cn] = res;
                cv[cn] = 1;
                cn++;
            }
        }
    }

    int best = -1;

    for (uint32_t c = 0; c < cn; ++c) {
        if (best < 0 ||
            cv[c] > cv[best] ||
            (cv[c] == cv[best] && cres[c] < cres[best])) {
            best = static_cast<int>(c);
        }
    }

    if (best < 0) return false;
    uint32_t runner = 0;

    for (uint32_t c = 0; c < cn; ++c) {
        if (static_cast<int>(c) != best && cv[c] > runner) runner = cv[c];
    }

    const bool clear = cv[best] >= 2 && cv[best] >= runner + 2;
    const bool lone = cn == 1 && cres[best] <= 0.005f;

    if (!clear && !lone) {
        return false;
    }

    ox = cox[best];
    oz = coz[best];
    return true;
}

// The TTL sweep: compact out entries not re-sighted in time. The fallback for a harvest the sampler knows was
// incomplete.
inline void dynlights_pool_sweep_ttl(uint64_t now) {
    uint32_t keep = 0;

    for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
        const DlPoolLight& e = g_dl.pool[i];
        uint64_t ttl;
        bool fast = false;

        if (e.gap_max_ms == 0) {
            ttl = DL_TTL_FLOOR_MS;   // Single sighting: no cadence evidence yet.
        } else {
            ttl = dl_gap_mult() * static_cast<uint64_t>(e.gap_max_ms);
            fast = dl_dense(e.sightings);
            // Not gated on decode failures elsewhere: forcing every light onto the slow floor while any window is
            // undecoded answers the wrong question.
            const uint64_t floor = fast ? DL_TTL_DENSE_FLOOR_MS
                                                : DL_TTL_FLOOR_MS;
            if (ttl < floor) ttl = floor;
            if (ttl > DL_TTL_CAP_MS) ttl = DL_TTL_CAP_MS;
        }

        if (now - e.stamp > ttl) {
            // Expired: compacted out (a window that is down, DL_WINPROV, does not extend it).
            continue;
        }

        if (keep != i) g_dl.pool[keep] = g_dl.pool[i];
        keep++;
    }

    g_dl.pool_n = keep;
}

// DL_DIAG (CN-4): whether reference b (main_gdiff rgb, main_scale) lights a light brighter than a by more than
// 10 %: a diffuse channel up by more than a tenth of a's (of 0.02 at least - the merge's own floor), or the distance
// scale down by more than a tenth of b's (attenuation reads distance x scale). Dimmer is brighter(b, a).
inline bool dl_ref_brighter(const float* a, const float* b) {
    for (int c = 0; c < 3; ++c) {
        if (b[c] - a[c] > 0.1f * fmaxf(fabsf(a[c]), 0.02f)) return true;
    }
    return a[3] - b[3] > 0.1f * fabsf(b[3]);
}
// DL_REF_MEDIAN: a harvest's brightest candidate (r: gdiff rgb, scale) into the history, DL_DIAG's
// moves counted against the pushes before it, and the reference published: each component's median of the three
// held, or the newest while fewer are (the session's first harvests). No age limit: after a pause in the harvests the
// first one back publishes the reference held before it once, then the new one - the lag of any lasting change; a
// reset there would pass the next excursion unfiltered, on the harvest where lights come back into view.
inline void dl_ref_push(const float* r) {
    if (g_dl.ref_hist_n == 3) {
        memmove(g_dl.ref_hist[0], g_dl.ref_hist[1], sizeof(g_dl.ref_hist[0]) * 2u);
        g_dl.ref_hist_n = 2;
    }
    memcpy(g_dl.ref_hist[g_dl.ref_hist_n], r, sizeof(g_dl.ref_hist[0]));
    ++g_dl.ref_hist_n;
    const uint32_t n = g_dl.ref_hist_n;
    if (n >= 2) {
        const float* const p = g_dl.ref_hist[n - 2];
        if (dl_ref_brighter(p, r)) stat_inc(g_stats.dl_ref_up);
        if (dl_ref_brighter(r, p)) stat_inc(g_stats.dl_ref_down);
    }
    float m[4];
    memcpy(m, r, sizeof(m));
    if (n == 3) {
        const float* const o = g_dl.ref_hist[0];
        const float* const c = g_dl.ref_hist[1];
        // The middle one an excursion this one took back.
        const bool back = !dl_ref_brighter(o, r) && !dl_ref_brighter(r, o);
        if (back && dl_ref_brighter(o, c)) stat_inc(g_stats.dl_ref_spike_up);
        if (back && dl_ref_brighter(c, o)) stat_inc(g_stats.dl_ref_spike_down);
        for (int k = 0; k < 4; ++k) {
            const float a = o[k], b = c[k];
            m[k] = fmaxf(fminf(a, b), fminf(fmaxf(a, b), r[k]));
        }
    }
    g_dl.main_gdiff[0] = m[0];
    g_dl.main_gdiff[1] = m[1];
    g_dl.main_gdiff[2] = m[2];
    g_dl.main_scale = m[3];
    g_dl.main_ref_valid = 1;
}
// DL_DIAG (CN-4): whether pool entry e repeats an older entry's light a nonzero whole number of land-grid cells away.
// Every origin sits on whole cells (DL_ORIGIN_GRID), so a window whose origin comes out wrong places its lights exactly
// that far off, as new entries; a lamp of the same model grid-aligned with another looks the same. The test: an entry
// the merge created earlier, at the same height and a whole number of cells off in x and z (both within 1 cm; TWIN of
// dynlights_presence_upload's elsewhere, DL_PRES_STALE, tighter), with the same record in every lane but the position -
// the attenuation, fade and cone shape exactly (their own relative 1e-4), the cone axis within 0.05 and its cut within
// 1e-3, the colour within half (a fire flickers from one upload to the next). No land grid read: no.
inline bool dl_copy_of(uint32_t e) {
    const float cell = dl_grid_base();
    if (!(cell > 0.0f)) return false;
    const DlPoolLight& a = g_dl.pool[e];
    auto c_near = [](float x, float y, float tol) { return fabsf(x - y) <= tol; };
    for (uint32_t j = 0; j < g_dl.pool_n; ++j) {
        const DlPoolLight& b = g_dl.pool[j];
        if (j == e || b.first_seq >= a.first_seq || b.spot != a.spot) continue;
        if (!c_near(a.rec[1], b.rec[1], 0.01f)) continue;
        const float dx = a.rec[0] - b.rec[0];
        const float dz = a.rec[2] - b.rec[2];
        if (!c_near(dx, cell * floorf(dx / cell + 0.5f), 0.01f) ||
            !c_near(dz, cell * floorf(dz / cell + 0.5f), 0.01f)) continue;
        if (!(fabsf(dx) > 0.5f * cell || fabsf(dz) > 0.5f * cell)) continue;   // The same place.
        bool same = true;
        static const int exact[] = { 11, 15, 16, 17, 18, 19, 20, 21 };
        for (int k : exact) {
            const float m = fmaxf(1.0f, fmaxf(fabsf(a.rec[k]), fabsf(b.rec[k])));
            same = same && c_near(a.rec[k], b.rec[k], 1.0e-4f * m);
        }
        for (int k = 4; k < 7; ++k) {
            same = same && c_near(a.rec[k], b.rec[k], 0.05f);
        }
        same = same && c_near(a.rec[7], b.rec[7], 1.0e-3f);
        static const int col[] = { 8, 9, 10, 12, 13, 14 };
        for (int k : col) {
            const float m = fmaxf(fabsf(a.rec[k]), fabsf(b.rec[k]));
            same = same && c_near(a.rec[k], b.rec[k], 0.5f * m + 1.0e-3f);
        }
        if (same) return true;
    }
    return false;
}

// Origin recovery + persistent union merge for one harvested arena side.
inline void dynlights_merge_windows(const uint8_t* base, int side, bool bridge) {
    const uint64_t now = steady_now_ms();
    if (g_dl.pool_stamp != 0 && now > g_dl.pool_stamp && now - g_dl.pool_stamp > DL_POOL_STALE_MS) {
        g_dlcap.stall_a = g_dl.pool_stamp;   // DL_STALL_GAP: this harvest ends a stall.
        g_dlcap.stall_b = now;
    }
    // Presence sweeps after the merge (a re-sight must land on its own entry). The TTL sweep runs only on an incomplete
    // harvest; pres is true whenever this runs.
    const bool pres = dl_cpu_wanted();
    float best_lum = -1.0f;
    float best_g[3] = { 1.0f, 1.0f, 1.0f };
    float best_s = 1.0f;

    {   // anchor-table sweep: long TTL, movers-only concern, and a reconciliation against the pool
        // - see dynlights_anchor_ghost.
        uint32_t ak = 0;

        for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
            if (now - g_dl.anchor_stamp[a] > DL_ANCHOR_TTL_MS) continue;

            if (dynlights_anchor_ghost(g_dl.anchor_pos[a])) {
                continue;
            }

            if (ak != a) {
                g_dl.anchor_pos[ak][0] = g_dl.anchor_pos[a][0];
                g_dl.anchor_pos[ak][1] = g_dl.anchor_pos[a][1];
                g_dl.anchor_pos[ak][2] = g_dl.anchor_pos[a][2];
                g_dl.anchor_stamp[ak] = g_dl.anchor_stamp[a];
            }

            ak++;
        }

        g_dl.anchor_n = ak;
    }

    const uint32_t wn = g_dl.win_count[side];
    const uint32_t cap = dl_cpu_wanted() ? DL_WIN_MAX_CPU : DL_WIN_MAX;

    for (uint32_t w = 0; w < wn && w < cap; ++w) {
        const DlWindowMeta& m = g_dl.win_meta[side][w];
        const uint8_t* p = base + w * DL_WIN_BYTES;
        float ctl[16];
        memcpy(ctl, p, sizeof(ctl));
        int32_t pc_i = 0, sc_i = 0;
        memcpy(&pc_i, p + 0, 4);
        memcpy(&sc_i, p + 16, 4);

        if (pc_i < 0 || pc_i > 1024 || sc_i < 0 || sc_i > 1024 ||
            !dl_finite(ctl[8]) || ctl[8] <= 1.0e-6f || ctl[8] >= 1.0e6f) {
            continue;
        }

        uint32_t pc = static_cast<uint32_t>(pc_i);
        uint32_t sc = static_cast<uint32_t>(sc_i);
        uint32_t total = pc + sc;

        if (total > DL_MAX_LIGHTS) {
            if (pc > DL_MAX_LIGHTS) pc = DL_MAX_LIGHTS;
            sc = DL_MAX_LIGHTS - pc;
            total = DL_MAX_LIGHTS;
        }

        const uint32_t avail = m.bytes11 / DL_LIGHT_BYTES;

        if (total > avail) {
            if (pc > avail) pc = avail;
            sc = avail - pc;
            total = avail;
        }

        const float lum = ctl[12] + ctl[13] + ctl[14];

        if (total == 0) continue;
        const float* l = reinterpret_cast<const float*>(p + DL_CTL_BYTES);

        float dox = 0.0f, doz = 0.0f, dres = -1.0f;
        int32_t dslot = -1, dwslot = -1;
        const bool derived_ok = dynlights_derive_origin(p, m,
                                                        l, total,
                                                        dox, doz, dres,
                                                        dslot, dwslot);

        float o_x = dox;
        float o_z = doz;

        if (!derived_ok) {
            // Propagation: vote this window's records against the pool.
            if (!dynlights_pool_vote(l, total, o_x, o_z)) {
                {   // DL_WINPROV: this window is down, per window.
                    const uint32_t ws = dl_win_slot(m.buf);
                    if (g_dl.win_fail_run[ws] < 0xFFFFFFFFu) g_dl.win_fail_run[ws]++;
                }
                if (pres) {
                    dl_unpl_note(l, total, g_dl.harvest_seq);   // DL_UNPL.
                }
                continue;
            }
        }

        // Range backstop: visible-light lists are camera-culled by construction, so a decoded record landing kilometres
        // from the capture camera convicts the origin, whatever route produced it. If any of the window's lights lands
        // near the camera the origin is plausible; only if none does is it convicted.
        if (total > 0 && m.view_ok) {   // DL_WIN_VIEW: this window's capture camera.
            bool d_near = false;
            uint32_t probed = 0;

            for (uint32_t d_i = 0; d_i < total &&
                                   probed < DL_RANGE_PROBE && !d_near; ++d_i) {
                const float* rr = l + d_i * 24;
                if (!dl_finite(rr[0]) || !dl_finite(rr[2])) continue;
                probed++;
                const float r0x = o_x + rr[0] - m.cam[0];
                const float r0z = o_z + rr[2] - m.cam[2];

                if (r0x * r0x + r0z * r0z <=
                    DL_ORIGIN_RANGE_M * DL_ORIGIN_RANGE_M) {
                    d_near = true;
                }
            }

            if (probed > 0 && !d_near) {
                if (pres) {
                    dl_unpl_note(l, total, g_dl.harvest_seq);   // DL_UNPL.
                }
                continue;
            }
        }

        // Main-pass reference candidates: brightest ctl this harvest, among the windows the harvest placed
        // (DL_REF_SEEN adds the span's uploads re-sighted through their buffer's origin, after the loop). A window
        // whose camera could not be placed is not the main pass.
        {
            if (dl_finite(lum) && lum > best_lum) {
                best_lum = lum;
                best_g[0] = ctl[12];
                best_g[1] = ctl[13];
                best_g[2] = ctl[14];
                best_s = ctl[8];
            }
        }

        // A window that keeps anchoring but moves its origin by a grid multiple is the last capture's shape: it never
        // counts as unanchored and puts its lights where they are not.
        {
            const uint32_t ws = dl_win_slot(m.buf);
            g_dl.win_fail_run[ws] = 0;
            g_dl.win_ox[ws] = o_x;
            g_dl.win_oz[ws] = o_z;
            g_dl.win_oset[ws] = 1;
            g_dl.win_oseq[ws] = g_dl.harvest_seq;   // DL_PRES_FRESH.
            if (g_dl.cur_oseq != g_dl.harvest_seq) {   // DL_PRES_CUR: this harvest's first placement.
                g_dl.cur_oseq = g_dl.harvest_seq;
                g_dl.cur_n = 0;
                g_dl.cur_amb = 0;
            }
            if (!m.view_live) g_dl.cur_amb = 1;   // A window no camera weighs: the harvest vouches for none.
            if (m.view_live) {   // Its capture camera vouches for the origin.
                uint32_t d_c = 0;
                while (d_c < g_dl.cur_n && !(fabsf(g_dl.cur_o[d_c][0] - o_x) <= DL_ORIGIN_RES_MAX &&
                                             fabsf(g_dl.cur_o[d_c][1] - o_z) <= DL_ORIGIN_RES_MAX)) {
                    ++d_c;
                }
                if (d_c == g_dl.cur_n && g_dl.cur_n < DL_CUR_N) {
                    ++g_dl.cur_n;
                } else if (d_c == g_dl.cur_n) {
                    g_dl.cur_amb = 1;   // An origin past the cap: unweighed.
                }
                if (d_c < g_dl.cur_n) {   // New, or the same origin's later capture (window order).
                    g_dl.cur_o[d_c][0] = o_x;
                    g_dl.cur_o[d_c][1] = o_z;
                    g_dl.cur_o[d_c][2] = m.cam[0];
                    g_dl.cur_o[d_c][3] = m.cam[2];
                }
            }
        }

        // The trigger-window mirror is main-pass by construction (the injection triggers inside the main pass); a
        // window whose globals diverge from the main-pass reference (DL_REF_MEDIAN: the median of the harvests'
        // brightest candidates, not that mirror) is aux: it keeps a pool light it sights alive (stamp, sightings)
        // without rewriting it - a light it sights first is still written, verbatim, as any first sight is. TWIN:
        // dl_ctl_main (the presence pass's rewrite test, DL_PRES_REC).
        bool main = true;

        if (g_dl.main_ref_valid) {   // vs the reference the last harvest left (DL_REF_MEDIAN);
                                     // Brightness moves slowly.
            for (int d_k = 0; d_k < 3 && main; ++d_k) {
                const float mref = g_dl.main_gdiff[d_k];
                const float tol = 0.5f * (fabsf(mref) > 0.02f ? fabsf(mref) : 0.02f);
                if (fabsf(ctl[12 + d_k] - mref) > tol) main = false;
            }

            if (main && fabsf(ctl[8] - g_dl.main_scale) > 0.25f * fabsf(g_dl.main_scale)) {
                main = false;
            }
        }

        // Slots this list has already claimed. Reset per window - another pass's list carries the same lights again.
        int      claim[DL_MAX_LIGHTS];
        uint32_t claim_n = 0;

        for (uint32_t i = 0; i < total; ++i) {
            const float* r = l + i * 24;
            const float wx = o_x + r[0];
            const float wy = r[1];
            const float wz = o_z + r[2];
            if (!dl_finite(wx) || !dl_finite(wy) || !dl_finite(wz)) continue;
            int hit = -1;
            float hit_cost = 0.0f;

            for (uint32_t d = 0; d < g_dl.pool_n; ++d) {
                if (fabsf(g_dl.pool[d].rec[0] - wx) < DL_MATCH_M &&
                    fabsf(g_dl.pool[d].rec[1] - wy) < DL_MATCH_M &&
                    fabsf(g_dl.pool[d].rec[2] - wz) < DL_MATCH_M) {
                    bool taken = false;
                    for (uint32_t c = 0; c < claim_n; ++c) {
                        if (claim[c] == static_cast<int>(d)) { taken = true; break; }
                    }
                    if (taken) continue;   // An earlier record of this list is that light.
                    // Nearest record: position plus every non-position lane.
                    float cost = fabsf(g_dl.pool[d].rec[0] - wx) +
                                 fabsf(g_dl.pool[d].rec[1] - wy) +
                                 fabsf(g_dl.pool[d].rec[2] - wz);
                    for (int k = 3; k < 24; ++k) cost += fabsf(g_dl.pool[d].rec[k] - r[k]);
                    if (hit < 0 || cost < hit_cost) {
                        hit = static_cast<int>(d);
                        hit_cost = cost;
                    }
                }
            }

            const bool d_new = hit < 0;
            if (d_new) {
                if (g_dl.pool_n >= static_cast<uint32_t>(g_dl.pool.size())) {
                    g_dl.pool.push_back(DlPoolLight{});   // Uncapped growth.
                }

                hit = static_cast<int>(g_dl.pool_n++);
                g_dl.pool[hit] = DlPoolLight{};   // The slot may hold a compacted-out ghost's
                                                      // stamps.
                g_dl.pool[hit].id = g_dlcap.pool_next_id++;
                g_dl.pool[hit].first_seq = g_dl.harvest_seq;   // DL_DIAG (CN-4).
                if (g_dlcap.pool_next_id == 0) g_dlcap.pool_next_id = 1;
            }
            if (claim_n < DL_MAX_LIGHTS) claim[claim_n++] = hit;
            if (d_new) {
                // First sight: falls through to the verbatim write below.
            } else if (!main) {
                DlPoolLight& ax = g_dl.pool[hit];
                const uint64_t gap = now - ax.stamp;
                ax.gap_max_ms = dl_gap_mark_at(ax.gap_max_ms, gap, ax.stamp, now);
                if (ax.sightings < 0xFFFF) ax.sightings++;
                ax.stamp = now;
                if (m.seq > ax.seq) ax.seq = m.seq;   // DL_SEQ.
                continue;
            }

            // Re-sight writes the whole record verbatim - live colour/attenuation (animated fires keep flickering),
            // freshest position, no folded globals (baking any pass's cb10 into shared records makes the pass
            // alternation flap per light).
            DlPoolLight& pl = g_dl.pool[hit];
            const uint8_t spot_new = i >= pc ? 1 : 0;
            // DL_REC_ORDER: a window older than the content the record holds (one of the span's first uploads - the
            // previous frame's lists, or a buffer sampled again unrewritten - under a record presence or a later
            // window wrote) re-sights as any main window does - cadence, anchor, stamp - and leaves the newer record.
            const bool older = !d_new && m.wseq != 0 && pl.rec_wseq > m.wseq;

            if (pl.stamp != 0) {
                const uint64_t gap = now - pl.stamp;
                pl.gap_max_ms = dl_gap_mark_at(pl.gap_max_ms, gap, pl.stamp, now);
                if (pl.sightings < 0xFFFF) pl.sightings++;
            } else {
                pl.gap_max_ms = 0;
                pl.sightings = 1;
            }
            if (!older) {   // DL_REC_ORDER.
                memcpy(pl.rec, r, sizeof(pl.rec));
                pl.rec[0] = wx;   // +0 rewritten to absolute world.
                pl.rec[1] = wy;
                pl.rec[2] = wz;
                pl.spot = spot_new;
                pl.rec_wseq = m.wseq;
            }
            if (derived_ok) {

                if (dres >= 0.0f && dres <= DL_ANCHOR_RES) {
                    dynlights_anchor_upsert(wx, wy, wz, now);
                }
            }

            pl.stamp = now;
            if (m.seq > pl.seq) pl.seq = m.seq;   // DL_SEQ.
        }
    }

    // DL_REF_MEDIAN - the main-pass reference: the diffuse multiplier and distance scale every dynamic light, the
    // fog's and our meshes', is weighted and attenuated with (dl_fill_ctl), what a main-pass list is told from an
    // aux one by (main above, dl_ctl_main) and what the light maps' reach and score read. Its evidence is the
    // brightest ctl among the windows a harvest places and (DL_REF_SEEN) among the span's uploads re-sighted
    // through their buffer's origin - the windows alone follow what the routine quota and the new lights authored: a
    // variant whose lists carried no new light went unseen, and came back on the harvests where one did (gunfire),
    // the reference flipping with it. Each component is the median of the last three harvests' (dl_ref_push):
    // once three are held, an excursion of one harvest never reaches it and a lasting change does a harvest later. A
    // harvest with no lit candidate keeps the last reference.
    if (g_dl.ref_seen_seq == g_dl.harvest_seq && g_dl.ref_seen_lum > best_lum) {   // DL_REF_SEEN.
        best_lum = g_dl.ref_seen_lum;
        best_g[0] = g_dl.ref_seen[0];
        best_g[1] = g_dl.ref_seen[1];
        best_g[2] = g_dl.ref_seen[2];
        best_s = g_dl.ref_seen[3];
    }
    if (best_lum > 0.0f) {
        const float best_r[4] = { best_g[0], best_g[1], best_g[2], best_s };
        dl_ref_push(best_r);
    }

    if (pres) {
        // Presence: this harvest holds every upload drawn since the last injection, tagged with its serial. When the
        // capture was complete, a pool light no upload of the span carried is gone at this injection (a light last seen
        // in the previous frame's post-injection draws lasts one harvest more: those draws are in this span). An
        // incomplete harvest takes the TTL sweep. An unplaced window is counted, not a gate. DL_SPAN_BRIDGE
        // (bridge): the first injected harvest after a flush harvest also keeps what that flush's span saw (seq one
        // below), so a light drawn only after the trigger is not dropped for a frame; a light that died there lasts one
        // harvest more instead.
        const uint64_t d_c = g_dl.harvest_seq;
        const bool complete = !g_dl.side_miss[side] && g_dl.side_uploads[side] > 0;

        if (!complete) {
            stat_inc(g_stats.dl_harvests_incomplete);   // DL_DIAG.
            dynlights_pool_sweep_ttl(now);
        } else {
            uint32_t keep = 0;

            for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
                const DlPoolLight& e = g_dl.pool[i];

                if (e.seq != d_c && !(bridge && e.seq + 1u == d_c)) {
                    if (e.seq == e.first_seq) stat_inc(g_stats.dl_blinks);   // DL_DIAG (CN-4).
                    continue;
                }

                if (keep != i) g_dl.pool[keep] = g_dl.pool[i];
                keep++;
            }

            g_dl.pool_n = keep;
        }
    }
    if (stats_on()) {   // DL_DIAG (CN-4): this harvest's new entries that repeat an older one elsewhere.
        for (uint32_t d_i = 0; d_i < g_dl.pool_n; ++d_i) {
            if (g_dl.pool[d_i].first_seq == g_dl.harvest_seq && dl_copy_of(d_i)) stat_inc(g_stats.dl_copies);
        }
    }
    {   // DL_IDLE: harvests in a row with nothing in the pool.
        const uint32_t idle = g_dlcap.idle.load(std::memory_order_relaxed);
        g_dlcap.idle.store(g_dl.pool_n == 0 ? (idle < 0xFFFFFFFFu ? idle + 1u : idle) : 0u,
            std::memory_order_relaxed);
    }

    g_dl.pool_stamp = now;
}

// Arena lifecycle at the injection (or its trigger, DL_TRIGGER_HARVEST; the flush's fallback otherwise). Every
// caller runs only under dl_cpu_wanted, so the live path merges the write side's CPU windows with no Map and no
// flip. The GPU-arena path below (harvest the previous frame's captures, DO_NOT_WAIT - a stall retries next injection
// without flipping - then flip sides) runs only if the demand drops between the caller's test and this one.
inline void dynlights_harvest_arena(ID3D11DeviceContext* ctx, bool bridge) {
    ++g_dlcap.pres_epoch;   // DL_PRES_MEMO: the merge and the sweep rewrite the pool, the origins and the notes.
    if (dl_cpu_wanted()) {
        // The windows drawn since the last harvest (an injection's, a trigger's or the flush's) are CPU-resident: merge
        // them now. No flip, no Map. A side holding GPU-arena windows (a mode switch mid-span) is skipped.
        const int w = g_dl.arena_write;
        const bool gpu_side = g_dl.win_count[w] > 0 && g_dl.win_meta[w][0].gen == 0;
        ++g_dl.harvest_seq;   // DL_SEQ: this span's serial; the uploads were tagged with it.

        if (!gpu_side) {
            dynlights_merge_windows(g_dl.cpu_arena[w].empty() ? nullptr : g_dl.cpu_arena[w].data(), w, bridge);
        }

        g_dl.win_count[w] = 0;
        memset(g_dl.win_meta[w], 0, sizeof(g_dl.win_meta[w]));
        g_dl.side_miss[w] = 0;
        g_dl.side_uploads[w] = 0;
        return;
    }

    const int r = g_dl.arena_write ^ 1;

    if (g_dl.arena_inflight[r] && g_dl.arena[r]) {
        D3D11_MAPPED_SUBRESOURCE m = {};
        const HRESULT hr = ctx->Map(g_dl.arena[r], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &m);

        if (FAILED(hr)) {
            return;   // Retry next injection; no flip.
        }

        dynlights_merge_windows(static_cast<const uint8_t*>(m.pData), r, bridge);
        ctx->Unmap(g_dl.arena[r], 0);
        g_dl.arena_inflight[r] = 0;
    }

    // Flip: the side the sampler filled this frame goes in flight; the drained side becomes the new write target.
    g_dl.arena_inflight[g_dl.arena_write] = g_dl.win_count[g_dl.arena_write] > 0 ? 1 : 0;
    g_dl.arena_write = r;
    g_dl.win_count[r] = 0;
    memset(g_dl.win_meta[r], 0, sizeof(g_dl.win_meta[r]));
}

// Parse + validate + publish one ctl + lights window: the trigger window staged from the CPU shadows, or a completed
// staging readback.
inline void dynlights_publish(const uint8_t* p, int s) {
    float ctl[16];
    memcpy(ctl, p, sizeof(ctl));
    int32_t pc_i = 0, sc_i = 0;
    memcpy(&pc_i, p + 0, 4);
    memcpy(&sc_i, p + 16, 4);
    uint32_t pc = 0, sc = 0;

    if (pc_i >= 0 && pc_i <= 1024 && sc_i >= 0 && sc_i <= 1024) {
        pc = static_cast<uint32_t>(pc_i);
        sc = static_cast<uint32_t>(sc_i);
    } else {
        const float pf = ctl[0], sf = ctl[4];

        if (dl_finite(pf) && dl_finite(sf) &&
            pf >= 0.0f && pf <= 1024.0f && sf >= 0.0f && sf <= 1024.0f &&
            pf == floorf(pf) && sf == floorf(sf)) {
            pc = static_cast<uint32_t>(pf);
            sc = static_cast<uint32_t>(sf);
        } else {
            return;
        }
    }

    // Scale / global-diffuse sanity: a poisoned scale garbles every light's attenuation - refuse the harvest rather
    // than shade with it.
    if (!dl_finite(ctl[8]) || ctl[8] <= 1.0e-6f || ctl[8] >= 1.0e6f ||
        !dl_finite(ctl[12]) || !dl_finite(ctl[13]) || !dl_finite(ctl[14])) {
        return;
    }

    uint32_t total = pc + sc;

    if (total > DL_MAX_LIGHTS) {
        if (pc > DL_MAX_LIGHTS) pc = DL_MAX_LIGHTS;
        sc = DL_MAX_LIGHTS - pc;
        total = DL_MAX_LIGHTS;
    }

    const uint32_t avail = g_dl.slot_bytes11[s] / DL_LIGHT_BYTES;

    if (total > avail) {   // The copied window undercuts the count.
        if (pc > avail) pc = avail;
        sc = avail - pc;
        total = avail;
    }

    const float* l = reinterpret_cast<const float*>(p + DL_CTL_BYTES);

    if (total > 0 &&
        (!dl_finite(l[0]) || !dl_finite(l[1]) || !dl_finite(l[2]))) {
        return;
    }

    // Publish (render thread): what getRenderStats reports (dlValid, dlViewValid, the counts, the capture camera and
    // the light list).
    g_dl.point_n = pc;
    g_dl.spot_n = sc;
    if (total > 0) memcpy(g_dl.lights, l, static_cast<size_t>(total) * DL_LIGHT_BYTES);
    g_dl.view_valid = g_dl.pending_view_ok[s] != 0;
    memcpy(g_dl.cam, g_dl.pending_cam[s], sizeof(g_dl.cam));
    g_dl.valid = true;
}

// Runs inside an accepted injection on the render thread (never on anomalous/rescue cycles) - or at its trigger when it
// has nothing to draw (DL_TRIGGER_HARVEST). Returns at once unless dl_cpu_wanted. Harvest first (the span's
// windows into the pool); then the trigger window publishes straight from the CPU shadows, before this frame's cbd
// fills read the mirror. The staging path (the copy into a free staging slot after the CPU branch, which step 1 reads
// back the next call) runs only if the demand drops between the two dl_cpu_wanted tests.
inline void dynlights_acquire(ID3D11DeviceContext* ctx, const float view[4][4]) {
    KH_PROF_SCOPE(PROF_DL_ACQUIRE);   // PROF.
    if (!dl_cpu_wanted()) return;   // DLF_DEMAND: a lit mesh or a caster, or a dynamicLightFog pass.

    // (0) merge the span's per-draw window captures into the pool (the CPU path does not flip).
    dynlights_harvest_arena(ctx, g_dlcap.harvest_flush_last);   // DL_SPAN_BRIDGE.
    g_dlcap.harvest_flush_last = false;
    g_dlcap.harvest_cycle = g_topo_cycles;   // DL_FALLBACK_HARVEST: the flush need not.
    stat_inc(g_stats.dl_harvests_inject);   // DL_DIAG.

    // (1) harvest completed copies (DO_NOT_WAIT: a stalled copy retries next injection).
    for (int s = 0; s < 2; ++s) {
        if (!g_dl.inflight[s] || !g_dl.staging[s]) continue;
        D3D11_MAPPED_SUBRESOURCE m = {};
        const HRESULT hr = ctx->Map(g_dl.staging[s], 0, D3D11_MAP_READ,
                                    D3D11_MAP_FLAG_DO_NOT_WAIT, &m);
        if (FAILED(hr)) {  continue; }
        dynlights_publish(static_cast<const uint8_t*>(m.pData), s);
        ctx->Unmap(g_dl.staging[s], 0);
        g_dl.inflight[s] = 0;
    }

    // (2) live slot bindings + this frame's copy.
    ID3D11DeviceContext1* c1 = as_ctx1(ctx);
    if (!c1) {  return; }
    ID3D11Buffer* b10 = nullptr;
    UINT f10 = 0, n10 = 0;
    ID3D11Buffer* b11 = nullptr;
    UINT f11 = 0, n11 = 0;
    c1->PSGetConstantBuffers1(10, 1, &b10, &f10, &n10);
    c1->PSGetConstantBuffers1(11, 1, &b11, &f11, &n11);

    if (!b10 || !b11) {
        if (b10) b10->Release();
        if (b11) b11->Release();
        return;
    }

    if (dl_cpu_wanted()) {
        // The trigger window publishes from the shadows, not from a staging copy read back a frame later.
        const CbShadow* s10 = cbs_find(b10);
        const CbShadow* s11 = cbs_find(b11);
        const uint32_t off10 = f10 * 16u;
        const uint32_t off11 = f11 * 16u;
        b10->Release();
        b11->Release();
        if (!s10 || !s11 ||
            off10 + DL_CTL_BYTES > s10->bytes || off11 >= s11->bytes) {
            return;
        }
        uint32_t want11 = DL_ARR_BYTES;
        if (n11 > 0 && n11 * 16u < want11) want11 = n11 * 16u;
        if (off11 + want11 > s11->bytes) want11 = s11->bytes - off11;
        want11 -= want11 % DL_LIGHT_BYTES;
        static uint8_t stage[DL_STAGE_BYTES];   // Render thread only.
        memcpy(stage, s10->data.data() + off10, DL_CTL_BYTES);
        if (want11 > 0) memcpy(stage + DL_CTL_BYTES, s11->data.data() + off11, want11);
        g_dl.slot_bytes11[0] = want11;

        if (view) {
            cam_of(view, g_dl.pending_cam[0]);
            g_dl.pending_view_ok[0] = 1;
        } else {
            g_dl.pending_view_ok[0] = 0;
        }

        dynlights_publish(stage, 0);
        return;
    }

    int w = -1;
    for (int s = 0; s < 2; ++s) {
        if (!g_dl.inflight[s]) { w = s; break; }
    }

    if (w < 0) {
        b10->Release();
        b11->Release();
        return;
    }

    if (!g_dl.staging[w]) {
        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);

        if (dev) {
            D3D11_BUFFER_DESC sd = {};
            sd.ByteWidth = DL_STAGE_BYTES;
            sd.Usage = D3D11_USAGE_STAGING;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            dev->CreateBuffer(&sd, nullptr, &g_dl.staging[w]);
            dev->Release();
        }

        if (!g_dl.staging[w]) {
            b10->Release();
            b11->Release();
            return;
        }
    }

    D3D11_BUFFER_DESC d10 = {}, d11 = {};
    b10->GetDesc(&d10);
    b11->GetDesc(&d11);
    const uint32_t off10 = f10 * 16u;   // firstConstant is in 16-byte constants.
    const uint32_t off11 = f11 * 16u;
    uint32_t want11 = DL_ARR_BYTES;

    if (n11 > 0 && n11 * 16u < want11) {
        want11 = n11 * 16u;   // The engine bound a smaller window.
    }

    if (off10 + DL_CTL_BYTES > d10.ByteWidth || off11 >= d11.ByteWidth) {
        b10->Release();
        b11->Release();
        return;
    }

    if (off11 + want11 > d11.ByteWidth) want11 = d11.ByteWidth - off11;
    want11 -= want11 % DL_LIGHT_BYTES;

    D3D11_BOX box10 = { off10, 0, 0, off10 + DL_CTL_BYTES, 1, 1 };
    ctx->CopySubresourceRegion(g_dl.staging[w], 0, 0, 0, 0, b10, 0, &box10);

    if (want11 > 0) {
        D3D11_BOX box11 = { off11, 0, 0, off11 + want11, 1, 1 };
        ctx->CopySubresourceRegion(g_dl.staging[w], 0, DL_CTL_BYTES, 0, 0, b11, 0, &box11);
    }

    g_dl.slot_bytes11[w] = want11;
    g_dl.inflight[w] = 1;

    // The camera captured with the list (getRenderStats reports it with the list).
    if (view) {
        cam_of(view, g_dl.pending_cam[w]);
        g_dl.pending_view_ok[w] = 1;
    } else {
        g_dl.pending_view_ok[w] = 0;
    }

    b10->Release();
    b11->Release();
}

// Session reset for the mirror: clears the PODs and the censuses (device objects are
// release_shadow_device_state's job).
inline void dynlights_reset_session() {
    g_dlcap.idle.store(0, std::memory_order_relaxed);   // DL_IDLE.
    g_dlcap.keep_n = 0u;   // DL_WANT_GATE.
    memset(g_dl_keep_ix, 0, sizeof(g_dl_keep_ix));   // DL_WANT_INDEX.
    memset(g_dlcap.keep_id, 0, sizeof(g_dlcap.keep_id));
    memset(g_dlcap.keep_cycle, 0, sizeof(g_dlcap.keep_cycle));
    g_dl.valid = false;
    g_dl.point_n = g_dl.spot_n = 0;
    g_dl.view_valid = false;
    g_dl.cam[0] = g_dl.cam[1] = g_dl.cam[2] = 0.0f;
    g_dl.inflight[0] = g_dl.inflight[1] = 0;
    g_dl.slot_bytes11[0] = g_dl.slot_bytes11[1] = 0;
    g_dl.pending_view_ok[0] = g_dl.pending_view_ok[1] = 0;

    // Arena / pool / origin machinery (device objects stay with release_shadow_device_state).
    g_dl.arena_inflight[0] = g_dl.arena_inflight[1] = 0;
    g_dl.arena_write = 0;
    memset(g_dl.win_meta, 0, sizeof(g_dl.win_meta));
    g_dl.win_count[0] = g_dl.win_count[1] = 0;
    for (int s = 0; s < 2; ++s) {   // DL_CPU.
        std::vector<uint8_t>().swap(g_dl.cpu_arena[s]);
        g_dl.side_miss[s] = 0;
        g_dl.side_uploads[s] = 0;
    }
    g_dl.harvest_seq = 0;
    dl_pmemo_clear();   // DL_PRES_MEMO: the span count restarts with the pool.
    memset(g_dl.unpl_q, 0, sizeof(g_dl.unpl_q));   // DL_UNPL.
    memset(g_dl.unpl_seq, 0, sizeof(g_dl.unpl_seq));
    cbs_clear();
    g_dl.pool.clear();
    g_dl.pool.shrink_to_fit();
    g_dl.pool_n = 0;
    g_dl.pool_stamp = 0;
    g_dl.main_gdiff[0] = g_dl.main_gdiff[1] = g_dl.main_gdiff[2] = 1.0f;
    g_dl.main_scale = 1.0f;
    g_dl.main_ref_valid = 0;
    memset(g_dl.ref_hist, 0, sizeof(g_dl.ref_hist));   // DL_REF_MEDIAN.
    g_dl.ref_hist_n = 0;

    memset(g_dl.anchor_pos, 0, sizeof(g_dl.anchor_pos));
    memset(g_dl.anchor_stamp, 0, sizeof(g_dl.anchor_stamp));
    g_dl.anchor_n = 0;

    memset(g_dl.win_ox, 0, sizeof(g_dl.win_ox));
    memset(g_dl.win_oz, 0, sizeof(g_dl.win_oz));
    memset(g_dl.win_oset, 0, sizeof(g_dl.win_oset));
    memset(g_dl.win_oseq, 0, sizeof(g_dl.win_oseq));   // DL_PRES_FRESH.
    g_dl.cur_n = 0;   // DL_PRES_CUR.
    g_dl.cur_oseq = 0;
    g_dl.cur_amb = 0;
    g_dl.ref_seen_lum = -1.0f;   // DL_REF_SEEN.
    g_dl.ref_seen_seq = 0;

    memset(g_dl.win_fail_lo, 0, sizeof(g_dl.win_fail_lo));
    memset(g_dl.win_fail_run, 0, sizeof(g_dl.win_fail_run));
    memset(g_dl.win_use, 0, sizeof(g_dl.win_use));   // DL_SLOT_LRU.
    g_dl.win_use_n = 0;

    memset(g_dl.win_hint_set, 0, sizeof(g_dl.win_hint_set));
    memset(g_dl.win_hint_kind, 0, sizeof(g_dl.win_hint_kind));
    memset(g_dl.win_hint_off, 0, sizeof(g_dl.win_hint_off));
    memset(g_dl.win_hint_form, 0, sizeof(g_dl.win_hint_form));
    memset(g_dl.win_hint_woff, 0, sizeof(g_dl.win_hint_woff));
    memset(g_dl.win_hint_wform, 0, sizeof(g_dl.win_hint_wform));
    for (uint32_t i = 0; i < DL_WIN_SLOTS; ++i) {
        g_dl.win_hint_slot[i] = -1;
        g_dl.win_hint_wslot[i] = -1;
    }

}
}