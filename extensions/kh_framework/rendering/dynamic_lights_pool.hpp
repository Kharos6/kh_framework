#pragma once

// The injection half (rendering_integration_injection.hpp): the dynamic-light harvest, part 2: the structural tests,
// the windows, origin derivation, the anchors and the pool. Never include it on its own.
namespace RenderIntegration {

// Structural affine test shared by the derivation. Form codes: 0 = 4x4 row-vector ((0,0,0,1) column checked); 1 = 4x4
// transposed / HLSL column-major (lane checked); 2 = 4x3 in 3 registers, rows [a, b, c, t] (the shadow_mats
// convention); 3 = 4 registers, each one row's xyz (w lanes ignored, none checked). The full scan tries all four.
inline uint32_t dl_form_floats(uint8_t khd_form) { return khd_form == 2 ? 12u : 16u; }

inline bool dl_cand_structure(const float* khd_mf, uint8_t khd_form, float (&khd_v)[4][4]) {
    if (khd_form <= 1) {
        for (int i = 0; i < 4; ++i) {
            for (int k = 0; k < 4; ++k) {
                khd_v[i][k] = khd_form == 0 ? khd_mf[i * 4 + k] : khd_mf[k * 4 + i];
            }
        }

        if (fabsf(khd_v[0][3]) > 0.001f || fabsf(khd_v[1][3]) > 0.001f ||
            fabsf(khd_v[2][3]) > 0.001f || fabsf(khd_v[3][3] - 1.0f) > 0.001f) {
            return false;
        }
    } else if (khd_form == 2) {
        for (int k = 0; k < 3; ++k) {
            khd_v[0][k] = khd_mf[k * 4 + 0];
            khd_v[1][k] = khd_mf[k * 4 + 1];
            khd_v[2][k] = khd_mf[k * 4 + 2];
            khd_v[3][k] = khd_mf[k * 4 + 3];
            khd_v[k][3] = 0.0f;
        }

        khd_v[3][3] = 1.0f;
    } else {
        for (int i = 0; i < 4; ++i) {
            khd_v[i][0] = khd_mf[i * 4 + 0];
            khd_v[i][1] = khd_mf[i * 4 + 1];
            khd_v[i][2] = khd_mf[i * 4 + 2];
            khd_v[i][3] = 0.0f;
        }

        khd_v[3][3] = 1.0f;
    }

    for (int r = 0; r < 3; ++r) {
        const float khd_n2 = khd_v[r][0] * khd_v[r][0] + khd_v[r][1] * khd_v[r][1] +
                             khd_v[r][2] * khd_v[r][2];
        if (khd_n2 < 0.96f || khd_n2 > 1.04f) return false;
    }

    for (int a = 0; a < 3; ++a) {
        for (int b = a + 1; b < 3; ++b) {
            const float khd_d = khd_v[a][0] * khd_v[b][0] + khd_v[a][1] * khd_v[b][1] +
                                khd_v[a][2] * khd_v[b][2];
            if (fabsf(khd_d) > 0.02f) return false;
        }
    }

    return true;
}

inline bool dl_rot_matches(const float (&khd_v)[4][4], const float* khd_cols) {
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            if (fabsf(khd_v[j][k] - khd_cols[k * 4 + j]) > KH_DL_DERIVE_ROT_TOL) return false;
        }
    }

    return true;
}

inline bool dl_rot_identity(const float (&khd_v)[4][4]) {
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            const float khd_want = j == k ? 1.0f : 0.0f;
            if (fabsf(khd_v[j][k] - khd_want) > KH_DL_DERIVE_ROT_TOL) return false;
        }
    }

    return true;
}

// The frame-structure proof shared by every route: bare triples must be near-exact - a random constant must not fake a
// camera.
inline bool dl_origin_validate(float khd_ex, float khd_ey, float khd_ez,
                               float khd_ytol, float khd_gtol, float khd_q,
                               float& khd_ox, float& khd_oz, float& khd_res) {
    if (!(khd_q > 0.0f)) return false;   // KH_DL_ORIGIN_GRID: no grid known yet.
    if (fabsf(khd_ey) > khd_ytol) {
        return false;
    }

    const float khd_sx = floorf(khd_ex / khd_q + 0.5f) * khd_q;
    const float khd_sz = floorf(khd_ez / khd_q + 0.5f) * khd_q;
    const float khd_rx = fabsf(khd_ex - khd_sx);
    const float khd_rz = fabsf(khd_ez - khd_sz);

    if (khd_rx > khd_gtol || khd_rz > khd_gtol) {
        return false;
    }

    khd_ox = khd_sx;
    khd_oz = khd_sz;
    khd_res = khd_rx > khd_rz ? khd_rx : khd_rz;
    return true;
}

// These three sit together; keep them together. dynlights_derive_origin, below, uses kh_dl_win_slot and
// kh_dl_win_origin_of; the presence pass and the sampler (rendering/dynamic_lights_capture.hpp) reach
// kh_dl_win_origin_of through its declaration above kh_dl_view_live. Nothing frees an entry (only the session reset),
// so once the engine has cycled more buffers than the table holds a lookup recycles one: the entry failing longest, and
// among equals the one looked up longest ago (KH_DL_SLOT_LRU: do not break ties to slot 0 - a table full of buffers the
// engine has stopped using, each at a run of 0, hands slot 0 to every new buffer, evicting the one before it, a buffer
// placed this same harvest included).
inline uint32_t kh_dl_win_slot(const void* khw_buf) {
    const uint32_t khw_lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(khw_buf));
    const uint64_t khw_use = ++g_dl.win_use_n;   // KH_DL_SLOT_LRU.

    for (uint32_t khw_i = 0; khw_i < KH_DL_WIN_SLOTS; ++khw_i) {
        if (g_dl.win_fail_lo[khw_i] == khw_lo) {
            g_dl.win_use[khw_i] = khw_use;
            return khw_i;
        }
    }

    for (uint32_t khw_i = 0; khw_i < KH_DL_WIN_SLOTS; ++khw_i) {
        if (g_dl.win_fail_lo[khw_i] == 0) {
            g_dl.win_fail_lo[khw_i] = khw_lo;
            g_dl.win_fail_run[khw_i] = 0;
            g_dl.win_use[khw_i] = khw_use;
            return khw_i;
        }
    }

    uint32_t khw_worst = 0;

    for (uint32_t khw_i = 1; khw_i < KH_DL_WIN_SLOTS; ++khw_i) {
        const uint32_t khw_r = g_dl.win_fail_run[khw_i], khw_rw = g_dl.win_fail_run[khw_worst];
        if (khw_r > khw_rw || (khw_r == khw_rw && g_dl.win_use[khw_i] < g_dl.win_use[khw_worst])) khw_worst = khw_i;
    }

    g_dl.win_fail_lo[khw_worst] = khw_lo;
    g_dl.win_fail_run[khw_worst] = 0;
    g_dl.win_use[khw_worst] = khw_use;
    // KH_DL_SLOT_RECYCLE: the slot now names another buffer. The origin and hints resolved for the old one are not this
    // buffer's - kh_dl_win_origin_of would hand them out as the origin this buffer last resolved to.
    g_dl.win_ox[khw_worst] = 0.0f;
    g_dl.win_oz[khw_worst] = 0.0f;
    g_dl.win_oset[khw_worst] = 0;
    g_dl.win_oseq[khw_worst] = 0;
    g_dl.win_hint_set[khw_worst] = 0;
    g_dl.win_hint_slot[khw_worst] = -1;
    g_dl.win_hint_wslot[khw_worst] = -1;
    return khw_worst;
}

// Read-only: the origin this buffer last resolved to, at whatever harvest (kh_dl_win_origin_seq says which).
// Deliberately not kh_dl_win_slot, which allocates - the ranking must ask without changing the table.
inline bool kh_dl_win_origin_of(const void* khp_buf, float& khp_ox, float& khp_oz) {
    const uint32_t khp_lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(khp_buf));
    if (khp_lo == 0) return false;

    for (uint32_t khp_i = 0; khp_i < KH_DL_WIN_SLOTS; ++khp_i) {
        if (g_dl.win_fail_lo[khp_i] == khp_lo && g_dl.win_oset[khp_i]) {
            khp_ox = g_dl.win_ox[khp_i];
            khp_oz = g_dl.win_oz[khp_i];
            return true;
        }
    }

    return false;
}

// KH_DL_PRES_FRESH: the harvest (g_dl.harvest_seq) that set this buffer's origin; 0 = none. Read-only, as
// kh_dl_win_origin_of.
inline uint64_t kh_dl_win_origin_seq(const void* khp_buf) {
    const uint32_t khp_lo = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(khp_buf));
    if (khp_lo == 0) return 0;

    for (uint32_t khp_i = 0; khp_i < KH_DL_WIN_SLOTS; ++khp_i) {
        if (g_dl.win_fail_lo[khp_i] == khp_lo && g_dl.win_oset[khp_i]) return g_dl.win_oseq[khp_i];
    }

    return 0;
}

// KH_DL_PRES_CUR: have two cameras (x, z) crossed no line of the land grid's cells, nor a half-cell line, between
// them? The engine's origins sit on whole cells (KH_DL_ORIGIN_GRID); KH_DL_PRES_CUR's premise is that they move only as
// the render camera crosses one of those lines - the cell's edge or its middle, of the engine's step or a multiple of
// it - so that a camera that crossed none kept its origin. Not measured in the field. No cell read yet: no.
inline bool kh_dl_same_cells(float khs_ax, float khs_az, float khs_bx, float khs_bz) {
    const float khs_c = kh_dl_grid_base();
    if (!(khs_c > 0.0f)) return false;
    auto khs_one = [khs_c](float khs_a, float khs_b) {
        return floorf(khs_a / khs_c) == floorf(khs_b / khs_c) &&
               floorf(khs_a / khs_c + 0.5f) == floorf(khs_b / khs_c + 0.5f);
    };
    return khs_one(khs_ax, khs_bx) && khs_one(khs_az, khs_bz);
}

// KH_DL_PRES_CUR: may an upload of span khc_seq rewrite through the origin (khc_ox, khc_oz) its buffer resolved? When
// the last harvest set that origin (KH_DL_PRES_FRESH), or placed another window at the same origin (within the vote's
// residual) and the camera now is on the same cells as that window's capture camera (kh_dl_same_cells): the frame's
// lists sit at one origin, which moves only as the camera crosses cells, so a buffer that got no window is placed by
// its neighbours'. A window is authored only on the routine quota (the span's first uploads), an unknown light or a
// buffer with no origin, and a harvest places few of those - without this a moving light whose lists come later in the
// frame is never rewritten: it freezes until it leaves the 1 m match cube and is sighted as new. The camera test keeps
// a harvest that placed windows on both sides of a crossing (a span holds two frames) from vouching for the old cell,
// and it vouches only for the one origin whose camera is on the camera's cells now: two such (a window placed at a
// wrong origin after the crossing) vouch for neither, nor does a harvest that placed a window it could not weigh (no
// live camera, or past KH_DL_CUR_N origins: cur_amb). Only the clear's own view is a camera (kh_dl_view_live). Not
// covered, for every buffer at the origin and one span: a rebase its premise does not see (kh_dl_same_cells - a rule
// with hysteresis or a radius, an origin keyed on something other than the render camera, a view out of step with
// the lists) and a window placed wrong at an origin no window of the harvest contradicts - KH_DL_PRES_FRESH's own
// residual, which reached only that harvest's windows' buffers; KH_DL_PRES_STALE stands behind both.
inline bool kh_dl_pres_fresh(const void* khc_buf, float khc_ox, float khc_oz, uint64_t khc_seq) {
    if (kh_dl_win_origin_seq(khc_buf) + 1 == khc_seq) return true;
    if (g_dl.cur_oseq == 0 || g_dl.cur_oseq + 1 != khc_seq || g_dl.cur_amb || !kh_dl_view_live()) return false;
    int khc_hit = -1;
    for (uint32_t khc_i = 0; khc_i < g_dl.cur_n && khc_hit < 0; ++khc_i) {
        if (fabsf(g_dl.cur_o[khc_i][0] - khc_ox) <= KH_DL_ORIGIN_RES_MAX &&
            fabsf(g_dl.cur_o[khc_i][1] - khc_oz) <= KH_DL_ORIGIN_RES_MAX) {
            khc_hit = static_cast<int>(khc_i);
        }
    }
    if (khc_hit < 0) return false;
    float khc_cam[3];
    kh_cam_of(g_ro.cycle_pv.view, khc_cam);
    for (uint32_t khc_i = 0; khc_i < g_dl.cur_n; ++khc_i) {   // The hit's camera here, and no other origin's.
        const bool khc_here = kh_dl_same_cells(khc_cam[0], khc_cam[2], g_dl.cur_o[khc_i][2], g_dl.cur_o[khc_i][3]);
        if (khc_here != (static_cast<int>(khc_i) == khc_hit)) return false;
    }
    return true;
}

inline bool dynlights_derive_origin(const uint8_t* khd_win, const DlWindowMeta& khd_m,
                                    const float* khd_recs, uint32_t khd_rec_n,
                                    float& khd_ox, float& khd_oz, float& khd_res,
                                    int32_t& khd_slot_out, int32_t& khd_wslot_out) {
    if (!khd_m.view_ok) {   // KH_DL_WIN_VIEW: the view this window was drawn under.
        return false;
    }

    const float* khd_bcols = khd_m.view;
    const float* khd_bcam = khd_m.cam;
    const uint8_t* khd_vs_base = khd_win + KH_DL_STAGE_BYTES;

    auto khd_block = [&](int32_t khd_s) {
        return reinterpret_cast<const float*>(khd_vs_base + khd_s * KH_DL_VS_BYTES);
    };

    // Applied at candidate selection and in the hint, so impostors die at candidacy. The invariant: the engine culls
    // its light list to the camera, so a correct origin places at least one of a window's lights near the capture
    // camera and a wrong one places them all far away. No 'record 0 is near' shortcut: the list is not distance-sorted.
    float khd_rxz[KH_DL_RANGE_PROBE][3];   // x, y, z - Y is needed to match anchors.
    uint32_t khd_rn = 0;

    for (uint32_t i = 0; i < khd_rec_n && khd_rn < KH_DL_RANGE_PROBE; ++i) {
        const float* khd_rr = khd_recs + i * 24;

        if (dl_finite(khd_rr[0]) && dl_finite(khd_rr[1]) && dl_finite(khd_rr[2])) {
            khd_rxz[khd_rn][0] = khd_rr[0];
            khd_rxz[khd_rn][1] = khd_rr[1];
            khd_rxz[khd_rn][2] = khd_rr[2];
            khd_rn++;
        }
    }

    // How near this origin puts the window's closest light to the capture camera. Negative when the window carries no
    // usable record.
    auto khd_near_of = [&](float khd_cox, float khd_coz) {
        float khd_best = -1.0f;

        for (uint32_t i = 0; i < khd_rn; ++i) {
            const float khd_rx = khd_cox + khd_rxz[i][0] - khd_bcam[0];
            const float khd_rz = khd_coz + khd_rxz[i][2] - khd_bcam[2];
            const float khd_d2 = khd_rx * khd_rx + khd_rz * khd_rz;
            if (khd_best < 0.0f || khd_d2 < khd_best) khd_best = khd_d2;
        }

        return khd_best < 0.0f ? -1.0f : sqrtf(khd_best);
    };

    auto khd_range_ok = [&](float khd_cox, float khd_coz) {
        if (khd_rn == 0) return true;
        const float khd_nr = khd_near_of(khd_cox, khd_coz);
        if (khd_nr >= 0.0f && khd_nr <= KH_DL_ORIGIN_RANGE_M) return true;
        return false;
    };

    // Residual is the rank where proximity (KH_DL_NEAR_MARGIN_M, in khd_offer) does not decide - never the route's
    // kind (do not prefer matrices over triples: that let 0.29 m impostors outrank 0.002 m truth); the authorship
    // ceiling gates every offer; zero-origin tracked separately (the degeneracy bin).
    float khd_b_ox = 0.0f, khd_b_oz = 0.0f, khd_b_res = 1.0e9f;
    int32_t khd_b_slot = -1, khd_b_wslot = -1;
    uint32_t khd_b_off = 0, khd_b_woff = 0;
    uint8_t khd_b_form = 0, khd_b_wform = 0, khd_b_kind = 0;
    bool khd_have_nz = false, khd_have_z = false;
    float khd_z_res = 1.0e9f;

    // A strict '<' would let slot order choose among equal residuals, which changes with the camera. Ties break by
    // evidence: 1. CONTINUITY - the origin this window last resolved to (only candidates offered THIS frame can
    // win). 2. CORROBORATION - how many of the window's records land on a known anchor. 3. Neither: the strictly
    // lower residual, else the first offered.
    float khd_prev_ox = 0.0f, khd_prev_oz = 0.0f;
    const bool khd_prev_ok = kh_dl_win_origin_of(khd_m.buf, khd_prev_ox, khd_prev_oz);

    auto khd_corrob = [&](float ox, float oz) {
        uint32_t khc_n = 0;

        for (uint32_t i = 0; i < khd_rn; ++i) {
            const float khc_x = ox + khd_rxz[i][0];
            const float khc_y = khd_rxz[i][1];
            const float khc_z = oz + khd_rxz[i][2];

            for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
                if (fabsf(g_dl.anchor_pos[a][0] - khc_x) < KH_DL_MATCH_M &&
                    fabsf(g_dl.anchor_pos[a][1] - khc_y) < KH_DL_MATCH_M &&
                    fabsf(g_dl.anchor_pos[a][2] - khc_z) < KH_DL_MATCH_M) {
                    khc_n++;
                    break;
                }
            }
        }

        return khc_n;
    };

    auto khd_offer = [&](float ox, float oz, float res, uint8_t kind,
                         int32_t slot, uint32_t off, uint8_t form,
                         int32_t wslot, uint32_t woff, uint8_t wform) {
        // A wandering structure is not an origin, and its residual says so (impostors land at 0.008-0.020 against the
        // true candidate at 0.000).
        if (res > KH_DL_ORIGIN_RES_MAX) {
            return;   // Never authors.
        }

        if (!khd_range_ok(ox, oz)) return;   // Structural impostors die here.

        if (ox == 0.0f && oz == 0.0f) {
            khd_have_z = true;
            if (res < khd_z_res) khd_z_res = res;
            return;
        }

        // The engine culls its light list to the camera, so a correct origin must place at least one of the window's
        // lights near it and a wrong one displaces every light. Proximity is the primary key, residual the tiebreak,
        // and the margin keeps noise from flapping the choice.
        bool khd_take = false;
        const float khd_nr_new = khd_near_of(ox, oz);
        const float khd_nr_old = khd_have_nz ? khd_near_of(khd_b_ox, khd_b_oz) : -1.0f;
        const bool khd_near_decides =
            khd_have_nz && khd_nr_new >= 0.0f && khd_nr_old >= 0.0f &&
            fabsf(khd_nr_new - khd_nr_old) > KH_DL_NEAR_MARGIN_M;

        if (!khd_have_nz) {
            khd_take = true;
        } else if (ox == khd_b_ox && oz == khd_b_oz) {
            return;   // Same origin by another route: nothing to decide.
        } else if (khd_near_decides) {
            khd_take = khd_nr_new < khd_nr_old;
        } else if (res < khd_b_res - KH_DL_RES_TIE) {
            khd_take = true;
        } else if (res <= khd_b_res + KH_DL_RES_TIE) {
            const bool khd_cont_new = khd_prev_ok && ox == khd_prev_ox && oz == khd_prev_oz;
            const bool khd_cont_old = khd_prev_ok &&
                                      khd_b_ox == khd_prev_ox && khd_b_oz == khd_prev_oz;

            if (khd_cont_new != khd_cont_old) {
                khd_take = khd_cont_new;
                khd_have_nz = true;
                if (khd_take) {
                    khd_b_ox = ox; khd_b_oz = oz; khd_b_res = res;
                    khd_b_kind = kind;
                    khd_b_slot = slot; khd_b_off = off; khd_b_form = form;
                    khd_b_wslot = wslot; khd_b_woff = woff; khd_b_wform = wform;
                }
                return;
            }

            {
                const uint32_t khd_cn = khd_corrob(ox, oz);
                const uint32_t khd_co = khd_corrob(khd_b_ox, khd_b_oz);

                if (khd_cn != khd_co) {
                    khd_take = khd_cn > khd_co;
                } else {
                    khd_take = res < khd_b_res;
                }
            }

        }

        if (khd_take) {
            khd_have_nz = true;
            khd_b_ox = ox; khd_b_oz = oz; khd_b_res = res;
            khd_b_kind = kind;
            khd_b_slot = slot; khd_b_off = off; khd_b_form = form;
            khd_b_wslot = wslot; khd_b_woff = woff; khd_b_wform = wform;
        }
    };

    float khd_tox, khd_toz, khd_tres;

    // Memoized fast path (non-zero winners only): this window's memo, not the session's.
    const uint32_t khd_ws = kh_dl_win_slot(khd_m.buf);
    const uint8_t  khd_h_kind  = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_kind[khd_ws]  : 0;
    const int32_t  khd_h_slot  = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_slot[khd_ws]  : -1;
    const uint32_t khd_h_off   = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_off[khd_ws]   : 0;
    const uint8_t  khd_h_form  = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_form[khd_ws]  : 0;
    const int32_t  khd_h_wslot = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_wslot[khd_ws] : -1;
    const uint32_t khd_h_woff  = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_woff[khd_ws]  : 0;
    const uint8_t  khd_h_wform = g_dl.win_hint_set[khd_ws] ? g_dl.win_hint_wform[khd_ws] : 0;

    if (khd_h_slot >= 0 && khd_h_slot < static_cast<int32_t>(KH_DL_VS_SLOTS)) {
        const uint32_t khd_mfl = khd_m.vs_bytes[khd_h_slot] / 4u;
        bool khd_hit = false;

        if (khd_h_kind == 2) {
            if (khd_h_off + 4 <= khd_mfl) {
                const float* khd_t = khd_block(khd_h_slot) + khd_h_off;

                if (dl_finite(khd_t[0]) && dl_finite(khd_t[1]) && dl_finite(khd_t[2]) && dl_origin_validate(khd_bcam[0] - khd_t[0], khd_bcam[1] - khd_t[1],
                                       khd_bcam[2] - khd_t[2], 0.5f, 0.25f, kh_dl_grid(),
                                       khd_tox, khd_toz, khd_tres) && khd_tres <= KH_DL_ORIGIN_RES_MAX && khd_range_ok(khd_tox, khd_toz) && !(khd_tox == 0.0f && khd_toz == 0.0f)) {
                    khd_hit = true;
                }
            }
        } else if (khd_h_kind == 3 && khd_h_wslot >= 0 &&
                   khd_h_wslot < static_cast<int32_t>(KH_DL_VS_SLOTS)) {
            const uint32_t khd_wfl = khd_m.vs_bytes[khd_h_wslot] / 4u;

            if (khd_h_off + 4 <= khd_mfl &&
                khd_h_woff + dl_form_floats(khd_h_wform) <= khd_wfl) {
                const float* khd_t = khd_block(khd_h_slot) + khd_h_off;
                float khd_vw[4][4];

                if (dl_finite(khd_t[0]) && dl_finite(khd_t[1]) && dl_finite(khd_t[2]) && dl_cand_structure(khd_block(khd_h_wslot) + khd_h_woff,
                                      khd_h_wform, khd_vw) && dl_rot_identity(khd_vw) && dl_origin_validate(khd_vw[3][0] - khd_t[0], khd_vw[3][1] - khd_t[1],
                                       khd_vw[3][2] - khd_t[2], 0.5f, 0.25f, kh_dl_grid(),
                                       khd_tox, khd_toz, khd_tres) && khd_tres <= KH_DL_ORIGIN_RES_MAX && khd_range_ok(khd_tox, khd_toz) && !(khd_tox == 0.0f && khd_toz == 0.0f)) {
                    khd_hit = true;
                }
            }
        } else if (khd_h_kind == 1 && khd_h_wslot >= 0 &&
                   khd_h_wslot < static_cast<int32_t>(KH_DL_VS_SLOTS)) {
            const uint32_t khd_wfl = khd_m.vs_bytes[khd_h_wslot] / 4u;

            if (khd_h_off + dl_form_floats(khd_h_form) <= khd_mfl &&
                khd_h_woff + dl_form_floats(khd_h_wform) <= khd_wfl) {
                float khd_vm[4][4], khd_vw[4][4];

                if (dl_cand_structure(khd_block(khd_h_slot) + khd_h_off,
                                      khd_h_form, khd_vm) &&
                    dl_rot_matches(khd_vm, khd_bcols) &&
                    dl_cand_structure(khd_block(khd_h_wslot) + khd_h_woff,
                                      khd_h_wform, khd_vw) &&
                    dl_rot_identity(khd_vw)) {
                    float khd_cam[3];
                    kh_cam_of(khd_vm, khd_cam);

                    if (dl_finite(khd_cam[0]) && dl_finite(khd_cam[1]) && dl_finite(khd_cam[2]) && dl_origin_validate(khd_bcam[0] - khd_cam[0] - khd_vw[3][0],
                                           khd_bcam[1] - khd_cam[1] - khd_vw[3][1],
                                           khd_bcam[2] - khd_cam[2] - khd_vw[3][2],
                                           KH_DL_DERIVE_Y_TOL, KH_DL_DERIVE_GRID_TOL, kh_dl_grid_base(),
                                           khd_tox, khd_toz, khd_tres) && khd_tres <= KH_DL_ORIGIN_RES_MAX && khd_range_ok(khd_tox, khd_toz) && !(khd_tox == 0.0f && khd_toz == 0.0f)) {
                        khd_hit = true;
                    }
                }
            }
        }

        // A drifting hit is not a hit: a real origin does not wander. If the memo produces an origin other than the
        // one this window holds, it has stopped describing this window.
        if (khd_hit) {
            float khh_px = 0.0f, khh_pz = 0.0f;

            if (kh_dl_win_origin_of(khd_m.buf, khh_px, khh_pz) &&
                (khh_px != khd_tox || khh_pz != khd_toz)) {
                khd_hit = false;   // Fall through to the full scan.
            }
        }

        if (khd_hit) {
            if (khd_h_kind == 1) kh_dl_grid_learn(khd_tox, khd_toz);   // KH_DL_ORIGIN_GRID: a strong route's.
            khd_ox = khd_tox;
            khd_oz = khd_toz;
            khd_res = khd_tres;
            khd_slot_out = khd_h_slot;
            khd_wslot_out = khd_h_wslot;
            return true;
        }
    }

    // Full scan: matrices first.
    struct DlMvCand { float cam[3]; int32_t slot; uint32_t off; uint8_t form; };
    struct DlWCand { float t[3]; int32_t slot; uint32_t off; uint8_t form; };
    DlMvCand khd_mv[8];
    DlWCand khd_wm[8];
    uint32_t khd_mv_n = 0, khd_wm_n = 0;

    for (uint32_t khd_s = 0; khd_s < KH_DL_VS_SLOTS; ++khd_s) {
        const uint32_t khd_fl = khd_m.vs_bytes[khd_s] / 4u;
        if (khd_fl < 12) continue;
        const float* khd_base = khd_block(static_cast<int32_t>(khd_s));

        for (uint32_t khd_off = 0; khd_off + 12 <= khd_fl; khd_off += 4) {
            {   // KH_DL_SCAN_PRE - each form reads its rotation from the same nine floats here (as rows or as columns:
                // [0..2], [4..6], [8..10]) and dl_cand_structure fails it when a row's squared norm is outside
                // [0.96, 1.04]. One of the nine past 1.03 in magnitude puts its row past 1.04 in either reading - a
                // sum of squares only grows, rounded or not; all nine under 0.56 keep every row under 0.95. A NaN
                // in a row makes its norm NaN, which the test passes, so a NaN among the nine leaves the offset to
                // the forms. Such an offset fails every form, and only a passing form runs anything below: it is
                // skipped here, in offset order. Most offsets of an engine buffer hold no rotation (the scan's bulk).
                // Edit with dl_cand_structure's bounds.
                static constexpr uint32_t khd_rot9[9] = { 0, 1, 2, 4, 5, 6, 8, 9, 10 };
                bool khd_pre_nan = false, khd_pre_big = false, khd_pre_small = true;
                for (const uint32_t khd_pre_k : khd_rot9) {
                    const float khd_pre_x = khd_base[khd_off + khd_pre_k];
                    if (!(khd_pre_x == khd_pre_x)) khd_pre_nan = true;
                    if (fabsf(khd_pre_x) > 1.03f) khd_pre_big = true;
                    if (!(fabsf(khd_pre_x) < 0.56f)) khd_pre_small = false;
                }
                if ((khd_pre_big || khd_pre_small) && !khd_pre_nan) continue;
            }
            for (uint8_t khd_form = 0; khd_form < 4; ++khd_form) {
                if (khd_off + dl_form_floats(khd_form) > khd_fl) continue;
                float khd_v[4][4];
                if (!dl_cand_structure(khd_base + khd_off, khd_form, khd_v)) continue;

                if (dl_rot_matches(khd_v, khd_bcols)) {
                    float khd_cam[3];
                    kh_cam_of(khd_v, khd_cam);

                    if (!dl_finite(khd_cam[0]) || !dl_finite(khd_cam[1]) || !dl_finite(khd_cam[2])) {
                        continue;
                    }

                    if (dl_origin_validate(khd_bcam[0] - khd_cam[0], khd_bcam[1] - khd_cam[1],
                                           khd_bcam[2] - khd_cam[2],
                                           KH_DL_DERIVE_Y_TOL, KH_DL_DERIVE_GRID_TOL, kh_dl_grid_base(),
                                           khd_tox, khd_toz, khd_tres)) {
                        khd_offer(khd_tox, khd_toz, khd_tres, static_cast<uint8_t>(0),
                                  static_cast<int32_t>(khd_s), khd_off, khd_form, -1, 0, 0);
                    }

                    if (khd_mv_n < 8) {
                        khd_mv[khd_mv_n].cam[0] = khd_cam[0];
                        khd_mv[khd_mv_n].cam[1] = khd_cam[1];
                        khd_mv[khd_mv_n].cam[2] = khd_cam[2];
                        khd_mv[khd_mv_n].slot = static_cast<int32_t>(khd_s);
                        khd_mv[khd_mv_n].off = khd_off;
                        khd_mv[khd_mv_n].form = khd_form;
                        khd_mv_n++;
                    }
                } else if (dl_rot_identity(khd_v)) {
                    if (dl_finite(khd_v[3][0]) && dl_finite(khd_v[3][1]) && dl_finite(khd_v[3][2]) &&
                        fabsf(khd_v[3][0]) < 1.0e7f && fabsf(khd_v[3][1]) < 1.0e7f &&
                        fabsf(khd_v[3][2]) < 1.0e7f && khd_wm_n < 8) {
                        khd_wm[khd_wm_n].t[0] = khd_v[3][0];
                        khd_wm[khd_wm_n].t[1] = khd_v[3][1];
                        khd_wm[khd_wm_n].t[2] = khd_v[3][2];
                        khd_wm[khd_wm_n].slot = static_cast<int32_t>(khd_s);
                        khd_wm[khd_wm_n].off = khd_off;
                        khd_wm[khd_wm_n].form = khd_form;
                        khd_wm_n++;
                    }
                }
            }
        }
    }

    // MV x W pairs (W' light-frame yields the origin; W true yields the tracked (0,0) degeneracy).
    for (uint32_t a = 0; a < khd_mv_n; ++a) {
        for (uint32_t b = 0; b < khd_wm_n; ++b) {

            if (dl_origin_validate(khd_bcam[0] - khd_mv[a].cam[0] - khd_wm[b].t[0],
                                   khd_bcam[1] - khd_mv[a].cam[1] - khd_wm[b].t[1],
                                   khd_bcam[2] - khd_mv[a].cam[2] - khd_wm[b].t[2],
                                   KH_DL_DERIVE_Y_TOL, KH_DL_DERIVE_GRID_TOL, kh_dl_grid_base(),
                                   khd_tox, khd_toz, khd_tres)) {
                khd_offer(khd_tox, khd_toz, khd_tres, 1,
                          khd_mv[a].slot, khd_mv[a].off, khd_mv[a].form,
                          khd_wm[b].slot, khd_wm[b].off, khd_wm[b].form);
            }
        }
    }

    // Kinds 2/3: bare float triples (tight tolerances - a round constant must not fake a camera or an object position).
    for (uint32_t khd_s = 0; khd_s < KH_DL_VS_SLOTS; ++khd_s) {
        const uint32_t khd_fl = khd_m.vs_bytes[khd_s] / 4u;
        if (khd_fl < 4) continue;
        const float* khd_base = khd_block(static_cast<int32_t>(khd_s));

        for (uint32_t khd_off = 0; khd_off + 4 <= khd_fl; khd_off += 4) {
            const float* khd_t = khd_base + khd_off;

            if (!dl_finite(khd_t[0]) || !dl_finite(khd_t[1]) || !dl_finite(khd_t[2])) continue;
            if (fabsf(khd_t[0]) > 1.0e7f || fabsf(khd_t[1]) > 1.0e7f || fabsf(khd_t[2]) > 1.0e7f) continue;

            // t = camera in the light frame.
            if (dl_origin_validate(khd_bcam[0] - khd_t[0], khd_bcam[1] - khd_t[1],
                                   khd_bcam[2] - khd_t[2], 0.5f, 0.25f, kh_dl_grid(),
                                   khd_tox, khd_toz, khd_tres)) {
                khd_offer(khd_tox, khd_toz, khd_tres, 2,
                          static_cast<int32_t>(khd_s), khd_off, 0, -1, 0, 0);
            }

            // t = object position in the light frame, against each true-world W translation.
            for (uint32_t b = 0; b < khd_wm_n; ++b) {
                if (dl_origin_validate(khd_wm[b].t[0] - khd_t[0], khd_wm[b].t[1] - khd_t[1],
                                       khd_wm[b].t[2] - khd_t[2], 0.5f, 0.25f, kh_dl_grid(),
                                       khd_tox, khd_toz, khd_tres)) {
                    khd_offer(khd_tox, khd_toz, khd_tres, 3,
                              static_cast<int32_t>(khd_s), khd_off, 0,
                              khd_wm[b].slot, khd_wm[b].off, khd_wm[b].form);
                }
            }
        }
    }

    if (khd_have_nz) {
        khd_ox = khd_b_ox;
        khd_oz = khd_b_oz;
        khd_res = khd_b_res;
        khd_slot_out = khd_b_slot;
        khd_wslot_out = khd_b_wslot;

        if (khd_b_kind <= 1) kh_dl_grid_learn(khd_b_ox, khd_b_oz);   // KH_DL_ORIGIN_GRID: routes 0 / 1 only.
        g_dl.win_hint_set[khd_ws] = 1;
        g_dl.win_hint_kind[khd_ws] = khd_b_kind;
        g_dl.win_hint_slot[khd_ws] = khd_b_slot;
        g_dl.win_hint_off[khd_ws] = khd_b_off;
        g_dl.win_hint_form[khd_ws] = khd_b_form;
        g_dl.win_hint_wslot[khd_ws] = khd_b_wslot;
        g_dl.win_hint_woff[khd_ws] = khd_b_woff;
        g_dl.win_hint_wform[khd_ws] = khd_b_wform;

        return true;
    }

    if (khd_have_z && khd_range_ok(0.0f, 0.0f)) {
        // The true-world degeneracy: accepted only alone, never memoized, and only when it places this list near the
        // camera like any other candidate.
        khd_ox = 0.0f;
        khd_oz = 0.0f;
        khd_res = khd_z_res;
        khd_slot_out = -1;
        khd_wslot_out = -1;
        return true;
    }

    return false;
}

// Anchor-table upsert: exact-derivation lights become long-lived surveying knowledge. Match-in-place refresh; oldest
// evicted when full.
inline void dynlights_anchor_upsert(float khd_x, float khd_y, float khd_z, uint64_t khd_now) {
    int khd_oldest = 0;

    for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
        if (fabsf(g_dl.anchor_pos[a][0] - khd_x) < KH_DL_MATCH_M &&
            fabsf(g_dl.anchor_pos[a][1] - khd_y) < KH_DL_MATCH_M &&
            fabsf(g_dl.anchor_pos[a][2] - khd_z) < KH_DL_MATCH_M) {
            g_dl.anchor_pos[a][0] = khd_x;
            g_dl.anchor_pos[a][1] = khd_y;
            g_dl.anchor_pos[a][2] = khd_z;
            g_dl.anchor_stamp[a] = khd_now;
            return;
        }

        if (g_dl.anchor_stamp[a] < g_dl.anchor_stamp[khd_oldest]) khd_oldest = static_cast<int>(a);
    }

    uint32_t khd_slot;

    if (g_dl.anchor_n < KH_DL_ANCHOR_N) {
        khd_slot = g_dl.anchor_n++;
    } else {
        khd_slot = static_cast<uint32_t>(khd_oldest);
    }

    g_dl.anchor_pos[khd_slot][0] = khd_x;
    g_dl.anchor_pos[khd_slot][1] = khd_y;
    g_dl.anchor_pos[khd_slot][2] = khd_z;
    g_dl.anchor_stamp[khd_slot] = khd_now;
}

// A pool record is re-sighted by every harvest that carries its light; an anchor is refreshed only when an exact
// derivation lands on it again, so a wrong one stands until its TTL unless this test convicts it. Drop the anchor,
// never the pool.

inline bool dynlights_anchor_ghost(const float* khg_a) {
    const float khg_q = kh_dl_grid();   // KH_DL_ORIGIN_GRID: no grid learned, no conviction.
    if (!(khg_q > 0.0f)) return false;
    for (uint32_t khg_p = 0; khg_p < g_dl.pool_n; ++khg_p) {
        const DlPoolLight& khg_e = g_dl.pool[khg_p];
        // Only a corroborated light may convict an anchor: otherwise a bad origin reaching both the pool and the table
        // lets the sweep drop the real anchor and the ghost alike, taking the vote out entirely.
        if (khg_e.sightings < KH_DL_DENSE_SIGHTS) continue;
        const float* khg_r = khg_e.rec;
        if (fabsf(khg_r[1] - khg_a[1]) > KH_DL_MATCH_M) continue;   // Different height: different
                                                                    // light.
        const float khg_dx = khg_r[0] - khg_a[0];
        const float khg_dz = khg_r[2] - khg_a[2];
        const float khg_sx = floorf(khg_dx / khg_q + 0.5f) * khg_q;
        const float khg_sz = floorf(khg_dz / khg_q + 0.5f) * khg_q;
        if (khg_sx == 0.0f && khg_sz == 0.0f) continue;   // Same place: this is the light.
        if (fabsf(khg_dx - khg_sx) > KH_DL_MATCH_M) continue;
        if (fabsf(khg_dz - khg_sz) > KH_DL_MATCH_M) continue;
        return true;
    }

    return false;
}

// The anchor vote (a window dynlights_derive_origin could not place): each finite record against each anchor at its
// height (within 0.25 m) a whole number of learned-grid steps away (within the vote's residual) votes for that origin
// (8 candidates at most). It wins with at least two votes and two more than the runner-up, or as the only candidate at
// a residual within 5 mm. Reads only: it writes nothing but khd_ox / khd_oz.
inline bool dynlights_pool_vote(const float* khd_l, uint32_t khd_total,
                                float& khd_ox, float& khd_oz) {
    if (g_dl.anchor_n == 0 || khd_total == 0) return false;
    const float khd_q = kh_dl_grid();   // KH_DL_ORIGIN_GRID: no grid learned, no vote.
    if (!(khd_q > 0.0f)) return false;
    float khd_cox[8], khd_coz[8], khd_cres[8];
    uint32_t khd_cv[8], khd_cn = 0;

    for (uint32_t i = 0; i < khd_total; ++i) {
        const float* khd_r = khd_l + i * 24;
        if (!dl_finite(khd_r[0]) || !dl_finite(khd_r[1]) || !dl_finite(khd_r[2])) continue;

        for (uint32_t d = 0; d < g_dl.anchor_n; ++d) {
            // Anchors are derivation-written only - exogenous truth, never the vote's own output.
            if (fabsf(g_dl.anchor_pos[d][1] - khd_r[1]) > 0.25f) continue;   // Same light: identical Y.
            const float khd_ex = g_dl.anchor_pos[d][0] - khd_r[0];
            const float khd_ez = g_dl.anchor_pos[d][2] - khd_r[2];
            const float khd_sx = floorf(khd_ex / khd_q + 0.5f) * khd_q;
            const float khd_sz = floorf(khd_ez / khd_q + 0.5f) * khd_q;
            const float khd_rx = fabsf(khd_ex - khd_sx);
            const float khd_rz = fabsf(khd_ez - khd_sz);
            if (khd_rx > KH_DL_ORIGIN_RES_MAX || khd_rz > KH_DL_ORIGIN_RES_MAX) continue;
            const float khd_res = khd_rx > khd_rz ? khd_rx : khd_rz;
            bool khd_found = false;

            for (uint32_t c = 0; c < khd_cn; ++c) {
                if (khd_cox[c] == khd_sx && khd_coz[c] == khd_sz) {
                    khd_cv[c]++;
                    if (khd_res < khd_cres[c]) khd_cres[c] = khd_res;
                    khd_found = true;
                    break;
                }
            }

            if (!khd_found && khd_cn < 8) {
                khd_cox[khd_cn] = khd_sx;
                khd_coz[khd_cn] = khd_sz;
                khd_cres[khd_cn] = khd_res;
                khd_cv[khd_cn] = 1;
                khd_cn++;
            }
        }
    }

    int khd_best = -1;

    for (uint32_t c = 0; c < khd_cn; ++c) {
        if (khd_best < 0 ||
            khd_cv[c] > khd_cv[khd_best] ||
            (khd_cv[c] == khd_cv[khd_best] && khd_cres[c] < khd_cres[khd_best])) {
            khd_best = static_cast<int>(c);
        }
    }

    if (khd_best < 0) return false;
    uint32_t khd_runner = 0;

    for (uint32_t c = 0; c < khd_cn; ++c) {
        if (static_cast<int>(c) != khd_best && khd_cv[c] > khd_runner) khd_runner = khd_cv[c];
    }

    const bool khd_clear = khd_cv[khd_best] >= 2 && khd_cv[khd_best] >= khd_runner + 2;
    const bool khd_lone = khd_cn == 1 && khd_cres[khd_best] <= 0.005f;

    if (!khd_clear && !khd_lone) {
        return false;
    }

    khd_ox = khd_cox[khd_best];
    khd_oz = khd_coz[khd_best];
    return true;
}

// The TTL sweep: compact out entries not re-sighted in time. The fallback for a harvest the sampler knows was
// incomplete.
inline void dynlights_pool_sweep_ttl(uint64_t khd_now) {
    uint32_t khd_keep = 0;

    for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
        const DlPoolLight& khd_e = g_dl.pool[i];
        uint64_t khd_ttl;
        bool khd_fast = false;

        if (khd_e.gap_max_ms == 0) {
            khd_ttl = KH_DL_TTL_FLOOR_MS;   // Single sighting: no cadence evidence yet.
        } else {
            khd_ttl = kh_dl_gap_mult() * static_cast<uint64_t>(khd_e.gap_max_ms);
            khd_fast = kh_dl_dense(khd_e.sightings);
            // Not gated on decode failures elsewhere: forcing every light onto the slow floor while any window is
            // undecoded answers the wrong question.
            const uint64_t khd_floor = khd_fast ? KH_DL_TTL_DENSE_FLOOR_MS
                                                : KH_DL_TTL_FLOOR_MS;
            if (khd_ttl < khd_floor) khd_ttl = khd_floor;
            if (khd_ttl > KH_DL_TTL_CAP_MS) khd_ttl = KH_DL_TTL_CAP_MS;
        }

        if (khd_now - khd_e.stamp > khd_ttl) {
            // Expired: compacted out (a window that is down, KH_DL_WINPROV, does not extend it).
            continue;
        }

        if (khd_keep != i) g_dl.pool[khd_keep] = g_dl.pool[i];
        khd_keep++;
    }

    g_dl.pool_n = khd_keep;
}

// KH_DL_DIAG (CN-4): whether reference b (main_gdiff rgb, main_scale) lights a light brighter than a by more than
// 10 %: a diffuse channel up by more than a tenth of a's (of 0.02 at least - the merge's own floor), or the distance
// scale down by more than a tenth of b's (attenuation reads distance x scale). Dimmer is brighter(b, a).
inline bool kh_dl_ref_brighter(const float* khrb_a, const float* khrb_b) {
    for (int khrb_c = 0; khrb_c < 3; ++khrb_c) {
        if (khrb_b[khrb_c] - khrb_a[khrb_c] > 0.1f * fmaxf(fabsf(khrb_a[khrb_c]), 0.02f)) return true;
    }
    return khrb_a[3] - khrb_b[3] > 0.1f * fabsf(khrb_b[3]);
}
// KH_DL_REF_MEDIAN: a harvest's brightest candidate (khrp_r: gdiff rgb, scale) into the history, KH_DL_DIAG's
// moves counted against the pushes before it, and the reference published: each component's median of the three
// held, or the newest while fewer are (the session's first harvests). No age limit: after a pause in the harvests the
// first one back publishes the reference held before it once, then the new one - the lag of any lasting change; a
// reset there would pass the next excursion unfiltered, on the harvest where lights come back into view.
inline void kh_dl_ref_push(const float* khrp_r) {
    if (g_dl.ref_hist_n == 3) {
        memmove(g_dl.ref_hist[0], g_dl.ref_hist[1], sizeof(g_dl.ref_hist[0]) * 2u);
        g_dl.ref_hist_n = 2;
    }
    memcpy(g_dl.ref_hist[g_dl.ref_hist_n], khrp_r, sizeof(g_dl.ref_hist[0]));
    ++g_dl.ref_hist_n;
    const uint32_t khrp_n = g_dl.ref_hist_n;
    if (khrp_n >= 2) {
        const float* const khrp_p = g_dl.ref_hist[khrp_n - 2];
        if (kh_dl_ref_brighter(khrp_p, khrp_r)) kh_stat(g_stats.dl_ref_up);
        if (kh_dl_ref_brighter(khrp_r, khrp_p)) kh_stat(g_stats.dl_ref_down);
    }
    float khrp_m[4];
    memcpy(khrp_m, khrp_r, sizeof(khrp_m));
    if (khrp_n == 3) {
        const float* const khrp_o = g_dl.ref_hist[0];
        const float* const khrp_c = g_dl.ref_hist[1];
        // The middle one an excursion this one took back.
        const bool khrp_back = !kh_dl_ref_brighter(khrp_o, khrp_r) && !kh_dl_ref_brighter(khrp_r, khrp_o);
        if (khrp_back && kh_dl_ref_brighter(khrp_o, khrp_c)) kh_stat(g_stats.dl_ref_spike_up);
        if (khrp_back && kh_dl_ref_brighter(khrp_c, khrp_o)) kh_stat(g_stats.dl_ref_spike_down);
        for (int khrp_k = 0; khrp_k < 4; ++khrp_k) {
            const float khrp_a = khrp_o[khrp_k], khrp_b = khrp_c[khrp_k];
            khrp_m[khrp_k] = fmaxf(fminf(khrp_a, khrp_b), fminf(fmaxf(khrp_a, khrp_b), khrp_r[khrp_k]));
        }
    }
    g_dl.main_gdiff[0] = khrp_m[0];
    g_dl.main_gdiff[1] = khrp_m[1];
    g_dl.main_gdiff[2] = khrp_m[2];
    g_dl.main_scale = khrp_m[3];
    g_dl.main_ref_valid = 1;
}
// KH_DL_DIAG (CN-4): whether pool entry khc_e repeats an older entry's light a nonzero whole number of land-grid
// cells away. Every origin sits on whole cells (KH_DL_ORIGIN_GRID), so a window whose origin comes out wrong places
// its lights exactly that far off, as new entries; a lamp of the same model grid-aligned with another looks the same.
// The test: an entry the merge created earlier, at the same height and a whole number of cells off in x and z (both
// within 1 cm; TWIN of presence's khp_elsewhere, KH_DL_PRES_STALE, tighter), with the same record in every lane but
// the position - the attenuation, fade and cone shape exactly (their own relative 1e-4), the cone axis within 0.05 and
// its cut within 1e-3, the colour within half (a fire flickers from one upload to the next). No land grid read: no.
inline bool kh_dl_copy_of(uint32_t khc_e) {
    const float khc_cell = kh_dl_grid_base();
    if (!(khc_cell > 0.0f)) return false;
    const DlPoolLight& khc_a = g_dl.pool[khc_e];
    auto khc_near = [](float khc_x, float khc_y, float khc_tol) { return fabsf(khc_x - khc_y) <= khc_tol; };
    for (uint32_t khc_j = 0; khc_j < g_dl.pool_n; ++khc_j) {
        const DlPoolLight& khc_b = g_dl.pool[khc_j];
        if (khc_j == khc_e || khc_b.first_seq >= khc_a.first_seq || khc_b.spot != khc_a.spot) continue;
        if (!khc_near(khc_a.rec[1], khc_b.rec[1], 0.01f)) continue;
        const float khc_dx = khc_a.rec[0] - khc_b.rec[0];
        const float khc_dz = khc_a.rec[2] - khc_b.rec[2];
        if (!khc_near(khc_dx, khc_cell * floorf(khc_dx / khc_cell + 0.5f), 0.01f) ||
            !khc_near(khc_dz, khc_cell * floorf(khc_dz / khc_cell + 0.5f), 0.01f)) continue;
        if (!(fabsf(khc_dx) > 0.5f * khc_cell || fabsf(khc_dz) > 0.5f * khc_cell)) continue;   // The same place.
        bool khc_same = true;
        static const int khc_exact[] = { 11, 15, 16, 17, 18, 19, 20, 21 };
        for (int khc_k : khc_exact) {
            const float khc_m = fmaxf(1.0f, fmaxf(fabsf(khc_a.rec[khc_k]), fabsf(khc_b.rec[khc_k])));
            khc_same = khc_same && khc_near(khc_a.rec[khc_k], khc_b.rec[khc_k], 1.0e-4f * khc_m);
        }
        for (int khc_k = 4; khc_k < 7; ++khc_k) {
            khc_same = khc_same && khc_near(khc_a.rec[khc_k], khc_b.rec[khc_k], 0.05f);
        }
        khc_same = khc_same && khc_near(khc_a.rec[7], khc_b.rec[7], 1.0e-3f);
        static const int khc_col[] = { 8, 9, 10, 12, 13, 14 };
        for (int khc_k : khc_col) {
            const float khc_m = fmaxf(fabsf(khc_a.rec[khc_k]), fabsf(khc_b.rec[khc_k]));
            khc_same = khc_same && khc_near(khc_a.rec[khc_k], khc_b.rec[khc_k], 0.5f * khc_m + 1.0e-3f);
        }
        if (khc_same) return true;
    }
    return false;
}

// Origin recovery + persistent union merge for one harvested arena side.
inline void dynlights_merge_windows(const uint8_t* khd_base, int khd_side, bool khd_bridge) {
    const uint64_t khd_now = steady_now_ms();
    if (g_dl.pool_stamp != 0 && khd_now > g_dl.pool_stamp && khd_now - g_dl.pool_stamp > KH_DL_POOL_STALE_MS) {
        g_dl_stall_a = g_dl.pool_stamp;   // KH_DL_STALL_GAP: this harvest ends a stall.
        g_dl_stall_b = khd_now;
    }
    // Presence sweeps after the merge (a re-sight must land on its own entry). The TTL sweep runs only on an incomplete
    // harvest; khd_pres is true whenever this runs.
    const bool khd_pres = kh_dl_cpu_wanted();
    float khd_best_lum = -1.0f;
    float khd_best_g[3] = { 1.0f, 1.0f, 1.0f };
    float khd_best_s = 1.0f;

    {   // anchor-table sweep: long TTL, movers-only concern, and a reconciliation against the pool
        // - see dynlights_anchor_ghost.
        uint32_t khd_ak = 0;

        for (uint32_t a = 0; a < g_dl.anchor_n; ++a) {
            if (khd_now - g_dl.anchor_stamp[a] > KH_DL_ANCHOR_TTL_MS) continue;

            if (dynlights_anchor_ghost(g_dl.anchor_pos[a])) {
                continue;
            }

            if (khd_ak != a) {
                g_dl.anchor_pos[khd_ak][0] = g_dl.anchor_pos[a][0];
                g_dl.anchor_pos[khd_ak][1] = g_dl.anchor_pos[a][1];
                g_dl.anchor_pos[khd_ak][2] = g_dl.anchor_pos[a][2];
                g_dl.anchor_stamp[khd_ak] = g_dl.anchor_stamp[a];
            }

            khd_ak++;
        }

        g_dl.anchor_n = khd_ak;
    }

    const uint32_t khd_wn = g_dl.win_count[khd_side];
    const uint32_t khd_cap = kh_dl_cpu_wanted() ? KH_DL_WIN_MAX_CPU : KH_DL_WIN_MAX;

    for (uint32_t w = 0; w < khd_wn && w < khd_cap; ++w) {
        const DlWindowMeta& khd_m = g_dl.win_meta[khd_side][w];
        const uint8_t* khd_p = khd_base + w * KH_DL_WIN_BYTES;
        float khd_ctl[16];
        memcpy(khd_ctl, khd_p, sizeof(khd_ctl));
        int32_t khd_pc_i = 0, khd_sc_i = 0;
        memcpy(&khd_pc_i, khd_p + 0, 4);
        memcpy(&khd_sc_i, khd_p + 16, 4);

        if (khd_pc_i < 0 || khd_pc_i > 1024 || khd_sc_i < 0 || khd_sc_i > 1024 ||
            !dl_finite(khd_ctl[8]) || khd_ctl[8] <= 1.0e-6f || khd_ctl[8] >= 1.0e6f) {
            continue;
        }

        uint32_t khd_pc = static_cast<uint32_t>(khd_pc_i);
        uint32_t khd_sc = static_cast<uint32_t>(khd_sc_i);
        uint32_t khd_total = khd_pc + khd_sc;

        if (khd_total > KH_DL_MAX_LIGHTS) {
            if (khd_pc > KH_DL_MAX_LIGHTS) khd_pc = KH_DL_MAX_LIGHTS;
            khd_sc = KH_DL_MAX_LIGHTS - khd_pc;
            khd_total = KH_DL_MAX_LIGHTS;
        }

        const uint32_t khd_avail = khd_m.bytes11 / KH_DL_LIGHT_BYTES;

        if (khd_total > khd_avail) {
            if (khd_pc > khd_avail) khd_pc = khd_avail;
            khd_sc = khd_avail - khd_pc;
            khd_total = khd_avail;
        }

        const float khd_lum = khd_ctl[12] + khd_ctl[13] + khd_ctl[14];

        if (khd_total == 0) continue;
        const float* khd_l = reinterpret_cast<const float*>(khd_p + KH_DL_CTL_BYTES);

        float khd_dox = 0.0f, khd_doz = 0.0f, khd_dres = -1.0f;
        int32_t khd_dslot = -1, khd_dwslot = -1;
        const bool khd_derived_ok = dynlights_derive_origin(khd_p, khd_m,
                                                            khd_l, khd_total,
                                                            khd_dox, khd_doz, khd_dres,
                                                            khd_dslot, khd_dwslot);

        float khd_o_x = khd_dox;
        float khd_o_z = khd_doz;

        if (!khd_derived_ok) {
            // Propagation: vote this window's records against the pool.
            if (!dynlights_pool_vote(khd_l, khd_total, khd_o_x, khd_o_z)) {
                {   // KH_DL_WINPROV: this window is down, per window.
                    const uint32_t khd_ws = kh_dl_win_slot(khd_m.buf);
                    if (g_dl.win_fail_run[khd_ws] < 0xFFFFFFFFu) g_dl.win_fail_run[khd_ws]++;
                }
                if (khd_pres) {
                    kh_dl_unpl_note(khd_l, khd_total, g_dl.harvest_seq);   // KH_DL_UNPL.
                }
                continue;
            }
        }

        // Range backstop: visible-light lists are camera-culled by construction, so a decoded record landing kilometres
        // from the capture camera convicts the origin, whatever route produced it. If any of the window's lights lands
        // near the camera the origin is plausible; only if none does is it convicted.
        if (khd_total > 0 && khd_m.view_ok) {   // KH_DL_WIN_VIEW: this window's capture camera.
            bool khd_near = false;
            uint32_t khd_probed = 0;

            for (uint32_t khd_i = 0; khd_i < khd_total &&
                                     khd_probed < KH_DL_RANGE_PROBE && !khd_near; ++khd_i) {
                const float* khd_rr = khd_l + khd_i * 24;
                if (!dl_finite(khd_rr[0]) || !dl_finite(khd_rr[2])) continue;
                khd_probed++;
                const float khd_r0x = khd_o_x + khd_rr[0] - khd_m.cam[0];
                const float khd_r0z = khd_o_z + khd_rr[2] - khd_m.cam[2];

                if (khd_r0x * khd_r0x + khd_r0z * khd_r0z <=
                    KH_DL_ORIGIN_RANGE_M * KH_DL_ORIGIN_RANGE_M) {
                    khd_near = true;
                }
            }

            if (khd_probed > 0 && !khd_near) {
                if (khd_pres) {
                    kh_dl_unpl_note(khd_l, khd_total, g_dl.harvest_seq);   // KH_DL_UNPL.
                }
                continue;
            }
        }

        // Main-pass reference candidates: brightest ctl this harvest, among the windows the harvest placed
        // (KH_DL_REF_SEEN adds the span's uploads re-sighted through their buffer's origin, after the loop). A window
        // whose camera could not be placed is not the main pass.
        {
            if (dl_finite(khd_lum) && khd_lum > khd_best_lum) {
                khd_best_lum = khd_lum;
                khd_best_g[0] = khd_ctl[12];
                khd_best_g[1] = khd_ctl[13];
                khd_best_g[2] = khd_ctl[14];
                khd_best_s = khd_ctl[8];
            }
        }

        // A window that keeps anchoring but moves its origin by a grid multiple is the last capture's shape: it never
        // counts as unanchored and puts its lights where they are not.
        {
            const uint32_t khd_ws = kh_dl_win_slot(khd_m.buf);
            g_dl.win_fail_run[khd_ws] = 0;
            g_dl.win_ox[khd_ws] = khd_o_x;
            g_dl.win_oz[khd_ws] = khd_o_z;
            g_dl.win_oset[khd_ws] = 1;
            g_dl.win_oseq[khd_ws] = g_dl.harvest_seq;   // KH_DL_PRES_FRESH.
            if (g_dl.cur_oseq != g_dl.harvest_seq) {   // KH_DL_PRES_CUR: this harvest's first placement.
                g_dl.cur_oseq = g_dl.harvest_seq;
                g_dl.cur_n = 0;
                g_dl.cur_amb = 0;
            }
            if (!khd_m.view_live) g_dl.cur_amb = 1;   // A window no camera weighs: the harvest vouches for none.
            if (khd_m.view_live) {   // Its capture camera vouches for the origin.
                uint32_t khd_c = 0;
                while (khd_c < g_dl.cur_n && !(fabsf(g_dl.cur_o[khd_c][0] - khd_o_x) <= KH_DL_ORIGIN_RES_MAX &&
                                               fabsf(g_dl.cur_o[khd_c][1] - khd_o_z) <= KH_DL_ORIGIN_RES_MAX)) {
                    ++khd_c;
                }
                if (khd_c == g_dl.cur_n && g_dl.cur_n < KH_DL_CUR_N) {
                    ++g_dl.cur_n;
                } else if (khd_c == g_dl.cur_n) {
                    g_dl.cur_amb = 1;   // An origin past the cap: unweighed.
                }
                if (khd_c < g_dl.cur_n) {   // New, or the same origin's later capture (window order).
                    g_dl.cur_o[khd_c][0] = khd_o_x;
                    g_dl.cur_o[khd_c][1] = khd_o_z;
                    g_dl.cur_o[khd_c][2] = khd_m.cam[0];
                    g_dl.cur_o[khd_c][3] = khd_m.cam[2];
                }
            }
        }

        // The trigger-window mirror is main-pass by construction (the injection triggers inside the main pass); a
        // window whose globals diverge from the main-pass reference (KH_DL_REF_MEDIAN: the median of the harvests'
        // brightest candidates, not that mirror) is aux: it keeps a pool light it sights alive (stamp, sightings)
        // without rewriting it - a light it sights first is still written, verbatim, as any first sight is. TWIN:
        // kh_dl_ctl_main (the presence pass's rewrite test, KH_DL_PRES_REC).
        bool khd_main = true;

        if (g_dl.main_ref_valid) {   // vs the reference the last harvest left (KH_DL_REF_MEDIAN);
                                     // Brightness moves slowly.
            for (int khd_k = 0; khd_k < 3 && khd_main; ++khd_k) {
                const float khd_mref = g_dl.main_gdiff[khd_k];
                const float khd_tol = 0.5f * (fabsf(khd_mref) > 0.02f ? fabsf(khd_mref) : 0.02f);
                if (fabsf(khd_ctl[12 + khd_k] - khd_mref) > khd_tol) khd_main = false;
            }

            if (khd_main && fabsf(khd_ctl[8] - g_dl.main_scale) > 0.25f * fabsf(g_dl.main_scale)) {
                khd_main = false;
            }
        }

        // Slots this list has already claimed. Reset per window - another pass's list carries the same lights again.
        int      khd_claim[KH_DL_MAX_LIGHTS];
        uint32_t khd_claim_n = 0;

        for (uint32_t i = 0; i < khd_total; ++i) {
            const float* khd_r = khd_l + i * 24;
            const float khd_wx = khd_o_x + khd_r[0];
            const float khd_wy = khd_r[1];
            const float khd_wz = khd_o_z + khd_r[2];
            if (!dl_finite(khd_wx) || !dl_finite(khd_wy) || !dl_finite(khd_wz)) continue;
            int khd_hit = -1;
            float khd_hit_cost = 0.0f;

            for (uint32_t d = 0; d < g_dl.pool_n; ++d) {
                if (fabsf(g_dl.pool[d].rec[0] - khd_wx) < KH_DL_MATCH_M &&
                    fabsf(g_dl.pool[d].rec[1] - khd_wy) < KH_DL_MATCH_M &&
                    fabsf(g_dl.pool[d].rec[2] - khd_wz) < KH_DL_MATCH_M) {
                    bool khd_taken = false;
                    for (uint32_t c = 0; c < khd_claim_n; ++c) {
                        if (khd_claim[c] == static_cast<int>(d)) { khd_taken = true; break; }
                    }
                    if (khd_taken) continue;   // An earlier record of this list is that light.
                    // Nearest record: position plus every non-position lane.
                    float khd_cost = fabsf(g_dl.pool[d].rec[0] - khd_wx) +
                                     fabsf(g_dl.pool[d].rec[1] - khd_wy) +
                                     fabsf(g_dl.pool[d].rec[2] - khd_wz);
                    for (int k = 3; k < 24; ++k) khd_cost += fabsf(g_dl.pool[d].rec[k] - khd_r[k]);
                    if (khd_hit < 0 || khd_cost < khd_hit_cost) {
                        khd_hit = static_cast<int>(d);
                        khd_hit_cost = khd_cost;
                    }
                }
            }

            const bool khd_new = khd_hit < 0;
            if (khd_new) {
                if (g_dl.pool_n >= static_cast<uint32_t>(g_dl.pool.size())) {
                    g_dl.pool.push_back(DlPoolLight{});   // Uncapped growth.
                }

                khd_hit = static_cast<int>(g_dl.pool_n++);
                g_dl.pool[khd_hit] = DlPoolLight{};   // The slot may hold a compacted-out ghost's
                                                      // stamps.
                g_dl.pool[khd_hit].id = g_dl_pool_next_id++;
                g_dl.pool[khd_hit].first_seq = g_dl.harvest_seq;   // KH_DL_DIAG (CN-4).
                if (g_dl_pool_next_id == 0) g_dl_pool_next_id = 1;
            }
            if (khd_claim_n < KH_DL_MAX_LIGHTS) khd_claim[khd_claim_n++] = khd_hit;
            if (khd_new) {
                // First sight: falls through to the verbatim write below.
            } else if (!khd_main) {
                DlPoolLight& khd_ax = g_dl.pool[khd_hit];
                const uint64_t khd_gap = khd_now - khd_ax.stamp;
                khd_ax.gap_max_ms = kh_dl_gap_mark_at(khd_ax.gap_max_ms, khd_gap, khd_ax.stamp, khd_now);
                if (khd_ax.sightings < 0xFFFF) khd_ax.sightings++;
                khd_ax.stamp = khd_now;
                if (khd_m.seq > khd_ax.seq) khd_ax.seq = khd_m.seq;   // KH_DL_SEQ.
                continue;
            }

            // Re-sight writes the whole record verbatim - live colour/attenuation (animated fires keep flickering),
            // freshest position, no folded globals (baking any pass's cb10 into shared records makes the pass
            // alternation flap per light).
            DlPoolLight& khd_pl = g_dl.pool[khd_hit];
            const uint8_t khd_spot_new = i >= khd_pc ? 1 : 0;
            // KH_DL_REC_ORDER: a window older than the content the record holds (one of the span's first uploads - the
            // previous frame's lists, or a buffer sampled again unrewritten - under a record presence or a later
            // window wrote) re-sights as any main window does - cadence, anchor, stamp - and leaves the newer record.
            const bool khd_older = !khd_new && khd_m.wseq != 0 && khd_pl.rec_wseq > khd_m.wseq;

            if (khd_pl.stamp != 0) {
                const uint64_t khd_gap = khd_now - khd_pl.stamp;
                khd_pl.gap_max_ms = kh_dl_gap_mark_at(khd_pl.gap_max_ms, khd_gap, khd_pl.stamp, khd_now);
                if (khd_pl.sightings < 0xFFFF) khd_pl.sightings++;
            } else {
                khd_pl.gap_max_ms = 0;
                khd_pl.sightings = 1;
            }
            if (!khd_older) {   // KH_DL_REC_ORDER.
                memcpy(khd_pl.rec, khd_r, sizeof(khd_pl.rec));
                khd_pl.rec[0] = khd_wx;   // +0 rewritten to absolute world.
                khd_pl.rec[1] = khd_wy;
                khd_pl.rec[2] = khd_wz;
                khd_pl.spot = khd_spot_new;
                khd_pl.rec_wseq = khd_m.wseq;
            }
            if (khd_derived_ok) {

                if (khd_dres >= 0.0f && khd_dres <= KH_DL_ANCHOR_RES) {
                    dynlights_anchor_upsert(khd_wx, khd_wy, khd_wz, khd_now);
                }
            }

            khd_pl.stamp = khd_now;
            if (khd_m.seq > khd_pl.seq) khd_pl.seq = khd_m.seq;   // KH_DL_SEQ.
        }
    }

    // KH_DL_REF_MEDIAN - the main-pass reference: the diffuse multiplier and distance scale every dynamic light, the
    // fog's and our meshes', is weighted and attenuated with (kh_dl_fill_ctl), what a main-pass list is told from an
    // aux one by (khd_main above, kh_dl_ctl_main) and what the light maps' reach and score read. Its evidence is the
    // brightest ctl among the windows a harvest places and (KH_DL_REF_SEEN) among the span's uploads re-sighted
    // through their buffer's origin - the windows alone follow what the routine quota and the new lights authored: a
    // variant whose lists carried no new light went unseen, and came back on the harvests where one did (gunfire),
    // the reference flipping with it. Each component is the median of the last three harvests' (kh_dl_ref_push):
    // once three are held, an excursion of one harvest never reaches it and a lasting change does a harvest later. A
    // harvest with no lit candidate keeps the last reference.
    if (g_dl.ref_seen_seq == g_dl.harvest_seq && g_dl.ref_seen_lum > khd_best_lum) {   // KH_DL_REF_SEEN.
        khd_best_lum = g_dl.ref_seen_lum;
        khd_best_g[0] = g_dl.ref_seen[0];
        khd_best_g[1] = g_dl.ref_seen[1];
        khd_best_g[2] = g_dl.ref_seen[2];
        khd_best_s = g_dl.ref_seen[3];
    }
    if (khd_best_lum > 0.0f) {
        const float khd_best_r[4] = { khd_best_g[0], khd_best_g[1], khd_best_g[2], khd_best_s };
        kh_dl_ref_push(khd_best_r);
    }

    if (khd_pres) {
        // Presence: this harvest holds every upload drawn since the last injection, tagged with its serial. When the
        // capture was complete, a pool light no upload of the span carried is gone at this injection (a light last seen
        // in the previous frame's post-injection draws lasts one harvest more: those draws are in this span). An
        // incomplete harvest takes the TTL sweep. An unplaced window is counted, not a gate. KH_DL_SPAN_BRIDGE
        // (khd_bridge): the first injected harvest after a flush harvest also keeps what that flush's span saw (seq one
        // below), so a light drawn only after the trigger is not dropped for a frame; a light that died there lasts one
        // harvest more instead.
        const uint64_t khd_c = g_dl.harvest_seq;
        const bool khd_complete = !g_dl.side_miss[khd_side] && g_dl.side_uploads[khd_side] > 0;

        if (!khd_complete) {
            kh_stat(g_stats.dl_harvests_incomplete);   // KH_DL_DIAG.
            dynlights_pool_sweep_ttl(khd_now);
        } else {
            uint32_t khd_keep = 0;

            for (uint32_t i = 0; i < g_dl.pool_n; ++i) {
                const DlPoolLight& khd_e = g_dl.pool[i];

                if (khd_e.seq != khd_c && !(khd_bridge && khd_e.seq + 1u == khd_c)) {
                    if (khd_e.seq == khd_e.first_seq) kh_stat(g_stats.dl_blinks);   // KH_DL_DIAG (CN-4).
                    continue;
                }

                if (khd_keep != i) g_dl.pool[khd_keep] = g_dl.pool[i];
                khd_keep++;
            }

            g_dl.pool_n = khd_keep;
        }
    }
    if (kh_stats_on()) {   // KH_DL_DIAG (CN-4): this harvest's new entries that repeat an older one elsewhere.
        for (uint32_t khd_i = 0; khd_i < g_dl.pool_n; ++khd_i) {
            if (g_dl.pool[khd_i].first_seq == g_dl.harvest_seq && kh_dl_copy_of(khd_i)) kh_stat(g_stats.dl_copies);
        }
    }
    {   // KH_DL_IDLE: harvests in a row with nothing in the pool.
        const uint32_t khd_idle = g_dl_idle.load(std::memory_order_relaxed);
        g_dl_idle.store(g_dl.pool_n == 0 ? (khd_idle < 0xFFFFFFFFu ? khd_idle + 1u : khd_idle) : 0u, std::memory_order_relaxed);
    }

    g_dl.pool_stamp = khd_now;
}

// Arena lifecycle at the injection (or its trigger, KH_DL_TRIGGER_HARVEST; the flush's fallback otherwise). Every
// caller runs only under kh_dl_cpu_wanted, so the live path merges the write side's CPU windows with no Map and no
// flip. The GPU-arena path below (harvest the previous frame's captures, DO_NOT_WAIT - a stall retries next injection
// without flipping - then flip sides) runs only if the demand drops between the caller's test and this one.
inline void dynlights_harvest_arena(ID3D11DeviceContext* ctx, bool khd_bridge) {
    ++g_dl_pres_epoch;   // KH_DL_PRES_MEMO: the merge and the sweep rewrite the pool, the origins and the notes.
    if (kh_dl_cpu_wanted()) {
        // The windows drawn since the last harvest (an injection's, a trigger's or the flush's) are CPU-resident: merge
        // them now. No flip, no Map. A side holding GPU-arena windows (a mode switch mid-span) is skipped.
        const int khd_w = g_dl.arena_write;
        const bool khd_gpu_side = g_dl.win_count[khd_w] > 0 && g_dl.win_meta[khd_w][0].gen == 0;
        ++g_dl.harvest_seq;   // KH_DL_SEQ: this span's serial; the uploads were tagged with it.

        if (!khd_gpu_side) {
            dynlights_merge_windows(g_dl.cpu_arena[khd_w].empty() ? nullptr : g_dl.cpu_arena[khd_w].data(), khd_w, khd_bridge);
        }

        g_dl.win_count[khd_w] = 0;
        memset(g_dl.win_meta[khd_w], 0, sizeof(g_dl.win_meta[khd_w]));
        g_dl.side_miss[khd_w] = 0;
        g_dl.side_uploads[khd_w] = 0;
        return;
    }

    const int khd_r = g_dl.arena_write ^ 1;

    if (g_dl.arena_inflight[khd_r] && g_dl.arena[khd_r]) {
        D3D11_MAPPED_SUBRESOURCE khd_m = {};
        const HRESULT khd_hr = ctx->Map(g_dl.arena[khd_r], 0, D3D11_MAP_READ,
                                        D3D11_MAP_FLAG_DO_NOT_WAIT, &khd_m);

        if (FAILED(khd_hr)) {
            return;   // Retry next injection; no flip.
        }

        dynlights_merge_windows(static_cast<const uint8_t*>(khd_m.pData), khd_r, khd_bridge);
        ctx->Unmap(g_dl.arena[khd_r], 0);
        g_dl.arena_inflight[khd_r] = 0;
    }

    // Flip: the side the sampler filled this frame goes in flight; the drained side becomes the new write target.
    g_dl.arena_inflight[g_dl.arena_write] = g_dl.win_count[g_dl.arena_write] > 0 ? 1 : 0;
    g_dl.arena_write = khd_r;
    g_dl.win_count[khd_r] = 0;
    memset(g_dl.win_meta[khd_r], 0, sizeof(g_dl.win_meta[khd_r]));
}

// Parse + validate + publish one ctl + lights window: the trigger window staged from the CPU shadows, or a completed
// staging readback.
inline void dynlights_publish(const uint8_t* khd_p, int khd_s) {
    float khd_ctl[16];
    memcpy(khd_ctl, khd_p, sizeof(khd_ctl));
    int32_t khd_pc_i = 0, khd_sc_i = 0;
    memcpy(&khd_pc_i, khd_p + 0, 4);
    memcpy(&khd_sc_i, khd_p + 16, 4);
    uint32_t khd_pc = 0, khd_sc = 0;

    if (khd_pc_i >= 0 && khd_pc_i <= 1024 && khd_sc_i >= 0 && khd_sc_i <= 1024) {
        khd_pc = static_cast<uint32_t>(khd_pc_i);
        khd_sc = static_cast<uint32_t>(khd_sc_i);
    } else {
        const float khd_pf = khd_ctl[0], khd_sf = khd_ctl[4];

        if (dl_finite(khd_pf) && dl_finite(khd_sf) &&
            khd_pf >= 0.0f && khd_pf <= 1024.0f && khd_sf >= 0.0f && khd_sf <= 1024.0f &&
            khd_pf == floorf(khd_pf) && khd_sf == floorf(khd_sf)) {
            khd_pc = static_cast<uint32_t>(khd_pf);
            khd_sc = static_cast<uint32_t>(khd_sf);
        } else {
            return;
        }
    }

    // Scale / global-diffuse sanity: a poisoned scale garbles every light's attenuation - refuse the harvest rather
    // than shade with it.
    if (!dl_finite(khd_ctl[8]) || khd_ctl[8] <= 1.0e-6f || khd_ctl[8] >= 1.0e6f ||
        !dl_finite(khd_ctl[12]) || !dl_finite(khd_ctl[13]) || !dl_finite(khd_ctl[14])) {
        return;
    }

    uint32_t khd_total = khd_pc + khd_sc;

    if (khd_total > KH_DL_MAX_LIGHTS) {
        if (khd_pc > KH_DL_MAX_LIGHTS) khd_pc = KH_DL_MAX_LIGHTS;
        khd_sc = KH_DL_MAX_LIGHTS - khd_pc;
        khd_total = KH_DL_MAX_LIGHTS;
    }

    const uint32_t khd_avail = g_dl.slot_bytes11[khd_s] / KH_DL_LIGHT_BYTES;

    if (khd_total > khd_avail) {   // The copied window undercuts the count.
        if (khd_pc > khd_avail) khd_pc = khd_avail;
        khd_sc = khd_avail - khd_pc;
        khd_total = khd_avail;
    }

    const float* khd_l = reinterpret_cast<const float*>(khd_p + KH_DL_CTL_BYTES);

    if (khd_total > 0 &&
        (!dl_finite(khd_l[0]) || !dl_finite(khd_l[1]) || !dl_finite(khd_l[2]))) {
        return;
    }

    // Publish (render thread): what getRenderStats reports (dlValid, dlViewValid, the counts, the capture camera and
    // the light list).
    g_dl.point_n = khd_pc;
    g_dl.spot_n = khd_sc;
    if (khd_total > 0) memcpy(g_dl.lights, khd_l, static_cast<size_t>(khd_total) * KH_DL_LIGHT_BYTES);
    g_dl.view_valid = g_dl.pending_view_ok[khd_s] != 0;
    memcpy(g_dl.cam, g_dl.pending_cam[khd_s], sizeof(g_dl.cam));
    g_dl.valid = true;
}

// Runs inside an accepted injection on the render thread (never on anomalous/rescue cycles) - or at its trigger when it
// has nothing to draw (KH_DL_TRIGGER_HARVEST). Returns at once unless kh_dl_cpu_wanted. Harvest first (the span's
// windows into the pool); then the trigger window publishes straight from the CPU shadows, before this frame's cbd
// fills read the mirror. The staging path (the copy into a free staging slot after the CPU branch, which step 1 reads
// back the next call) runs only if the demand drops between the two kh_dl_cpu_wanted tests.
inline void dynlights_acquire(ID3D11DeviceContext* ctx, const float view[4][4]) {
    KH_PROF_SCOPE(KHP_DL_ACQUIRE);   // KH_PROF.
    if (!kh_dl_cpu_wanted()) return;   // KH_DLF_DEMAND: a lit mesh or a caster, or a dynamicLightFog pass.

    // (0) merge the span's per-draw window captures into the pool (the CPU path does not flip).
    dynlights_harvest_arena(ctx, g_dl_harvest_flush_last);   // KH_DL_SPAN_BRIDGE.
    g_dl_harvest_flush_last = false;
    g_dl_harvest_cycle = g_topo_cycles;   // KH_DL_FALLBACK_HARVEST: the flush need not.
    kh_stat(g_stats.dl_harvests_inject);   // KH_DL_DIAG.

    // (1) harvest completed copies (DO_NOT_WAIT: a stalled copy retries next injection).
    for (int khd_s = 0; khd_s < 2; ++khd_s) {
        if (!g_dl.inflight[khd_s] || !g_dl.staging[khd_s]) continue;
        D3D11_MAPPED_SUBRESOURCE khd_m = {};
        const HRESULT khd_hr = ctx->Map(g_dl.staging[khd_s], 0, D3D11_MAP_READ,
                                        D3D11_MAP_FLAG_DO_NOT_WAIT, &khd_m);
        if (FAILED(khd_hr)) {  continue; }
        dynlights_publish(static_cast<const uint8_t*>(khd_m.pData), khd_s);
        ctx->Unmap(g_dl.staging[khd_s], 0);
        g_dl.inflight[khd_s] = 0;
    }

    // (2) live slot bindings + this frame's copy.
    ID3D11DeviceContext1* khd_c1 = kh_ctx1(ctx);
    if (!khd_c1) {  return; }
    ID3D11Buffer* khd_b10 = nullptr;
    UINT khd_f10 = 0, khd_n10 = 0;
    ID3D11Buffer* khd_b11 = nullptr;
    UINT khd_f11 = 0, khd_n11 = 0;
    khd_c1->PSGetConstantBuffers1(10, 1, &khd_b10, &khd_f10, &khd_n10);
    khd_c1->PSGetConstantBuffers1(11, 1, &khd_b11, &khd_f11, &khd_n11);

    if (!khd_b10 || !khd_b11) {
        if (khd_b10) khd_b10->Release();
        if (khd_b11) khd_b11->Release();
        return;
    }

    if (kh_dl_cpu_wanted()) {
        // The trigger window publishes from the shadows, not from a staging copy read back a frame later.
        const KhCbShadow* khd_s10 = kh_cbs_find(khd_b10);
        const KhCbShadow* khd_s11 = kh_cbs_find(khd_b11);
        const uint32_t khd_off10 = khd_f10 * 16u;
        const uint32_t khd_off11 = khd_f11 * 16u;
        khd_b10->Release();
        khd_b11->Release();
        if (!khd_s10 || !khd_s11 ||
            khd_off10 + KH_DL_CTL_BYTES > khd_s10->bytes || khd_off11 >= khd_s11->bytes) {
            return;
        }
        uint32_t khd_want11 = KH_DL_ARR_BYTES;
        if (khd_n11 > 0 && khd_n11 * 16u < khd_want11) khd_want11 = khd_n11 * 16u;
        if (khd_off11 + khd_want11 > khd_s11->bytes) khd_want11 = khd_s11->bytes - khd_off11;
        khd_want11 -= khd_want11 % KH_DL_LIGHT_BYTES;
        static uint8_t khd_stage[KH_DL_STAGE_BYTES];   // Render thread only.
        memcpy(khd_stage, khd_s10->data.data() + khd_off10, KH_DL_CTL_BYTES);
        if (khd_want11 > 0) memcpy(khd_stage + KH_DL_CTL_BYTES, khd_s11->data.data() + khd_off11, khd_want11);
        g_dl.slot_bytes11[0] = khd_want11;

        if (view) {
            kh_cam_of(view, g_dl.pending_cam[0]);
            g_dl.pending_view_ok[0] = 1;
        } else {
            g_dl.pending_view_ok[0] = 0;
        }

        dynlights_publish(khd_stage, 0);
        return;
    }

    int khd_w = -1;
    for (int khd_s = 0; khd_s < 2; ++khd_s) {
        if (!g_dl.inflight[khd_s]) { khd_w = khd_s; break; }
    }

    if (khd_w < 0) {
        khd_b10->Release();
        khd_b11->Release();
        return;
    }

    if (!g_dl.staging[khd_w]) {
        ID3D11Device* khd_dev = nullptr;
        ctx->GetDevice(&khd_dev);

        if (khd_dev) {
            D3D11_BUFFER_DESC khd_sd = {};
            khd_sd.ByteWidth = KH_DL_STAGE_BYTES;
            khd_sd.Usage = D3D11_USAGE_STAGING;
            khd_sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            khd_dev->CreateBuffer(&khd_sd, nullptr, &g_dl.staging[khd_w]);
            khd_dev->Release();
        }

        if (!g_dl.staging[khd_w]) {
            khd_b10->Release();
            khd_b11->Release();
            return;
        }
    }

    D3D11_BUFFER_DESC khd_d10 = {}, khd_d11 = {};
    khd_b10->GetDesc(&khd_d10);
    khd_b11->GetDesc(&khd_d11);
    const uint32_t khd_off10 = khd_f10 * 16u;   // firstConstant is in 16-byte constants.
    const uint32_t khd_off11 = khd_f11 * 16u;
    uint32_t khd_want11 = KH_DL_ARR_BYTES;

    if (khd_n11 > 0 && khd_n11 * 16u < khd_want11) {
        khd_want11 = khd_n11 * 16u;   // The engine bound a smaller window.
    }

    if (khd_off10 + KH_DL_CTL_BYTES > khd_d10.ByteWidth || khd_off11 >= khd_d11.ByteWidth) {
        khd_b10->Release();
        khd_b11->Release();
        return;
    }

    if (khd_off11 + khd_want11 > khd_d11.ByteWidth) khd_want11 = khd_d11.ByteWidth - khd_off11;
    khd_want11 -= khd_want11 % KH_DL_LIGHT_BYTES;

    D3D11_BOX khd_box10 = { khd_off10, 0, 0, khd_off10 + KH_DL_CTL_BYTES, 1, 1 };
    ctx->CopySubresourceRegion(g_dl.staging[khd_w], 0, 0, 0, 0, khd_b10, 0, &khd_box10);

    if (khd_want11 > 0) {
        D3D11_BOX khd_box11 = { khd_off11, 0, 0, khd_off11 + khd_want11, 1, 1 };
        ctx->CopySubresourceRegion(g_dl.staging[khd_w], 0, KH_DL_CTL_BYTES, 0, 0, khd_b11, 0, &khd_box11);
    }

    g_dl.slot_bytes11[khd_w] = khd_want11;
    g_dl.inflight[khd_w] = 1;

    // The camera captured with the list (getRenderStats reports it with the list).
    if (view) {
        kh_cam_of(view, g_dl.pending_cam[khd_w]);
        g_dl.pending_view_ok[khd_w] = 1;
    } else {
        g_dl.pending_view_ok[khd_w] = 0;
    }

    khd_b10->Release();
    khd_b11->Release();
}

// Session reset for the mirror: clears the PODs and the censuses (device objects are
// release_shadow_device_state's job).
inline void dynlights_reset_session() {
    g_dl_idle.store(0, std::memory_order_relaxed);   // KH_DL_IDLE.
    g_dl_keep_n = 0u;   // KH_DL_WANT_GATE.
    memset(g_dl_keep_ix, 0, sizeof(g_dl_keep_ix));   // KH_DL_WANT_INDEX.
    memset(g_dl_keep_id, 0, sizeof(g_dl_keep_id));
    memset(g_dl_keep_cycle, 0, sizeof(g_dl_keep_cycle));
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
    for (int khd_s = 0; khd_s < 2; ++khd_s) {   // KH_DL_CPU.
        std::vector<uint8_t>().swap(g_dl.cpu_arena[khd_s]);
        g_dl.side_miss[khd_s] = 0;
        g_dl.side_uploads[khd_s] = 0;
    }
    g_dl.harvest_seq = 0;
    kh_dl_pmemo_clear();   // KH_DL_PRES_MEMO: the span count restarts with the pool.
    memset(g_dl.unpl_q, 0, sizeof(g_dl.unpl_q));   // KH_DL_UNPL.
    memset(g_dl.unpl_seq, 0, sizeof(g_dl.unpl_seq));
    kh_cbs_clear();
    g_dl.pool.clear();
    g_dl.pool.shrink_to_fit();
    g_dl.pool_n = 0;
    g_dl.pool_stamp = 0;
    g_dl.main_gdiff[0] = g_dl.main_gdiff[1] = g_dl.main_gdiff[2] = 1.0f;
    g_dl.main_scale = 1.0f;
    g_dl.main_ref_valid = 0;
    memset(g_dl.ref_hist, 0, sizeof(g_dl.ref_hist));   // KH_DL_REF_MEDIAN.
    g_dl.ref_hist_n = 0;

    memset(g_dl.anchor_pos, 0, sizeof(g_dl.anchor_pos));
    memset(g_dl.anchor_stamp, 0, sizeof(g_dl.anchor_stamp));
    g_dl.anchor_n = 0;

    memset(g_dl.win_ox, 0, sizeof(g_dl.win_ox));
    memset(g_dl.win_oz, 0, sizeof(g_dl.win_oz));
    memset(g_dl.win_oset, 0, sizeof(g_dl.win_oset));
    memset(g_dl.win_oseq, 0, sizeof(g_dl.win_oseq));   // KH_DL_PRES_FRESH.
    g_dl.cur_n = 0;   // KH_DL_PRES_CUR.
    g_dl.cur_oseq = 0;
    g_dl.cur_amb = 0;
    g_dl.ref_seen_lum = -1.0f;   // KH_DL_REF_SEEN.
    g_dl.ref_seen_seq = 0;

    memset(g_dl.win_fail_lo, 0, sizeof(g_dl.win_fail_lo));
    memset(g_dl.win_fail_run, 0, sizeof(g_dl.win_fail_run));
    memset(g_dl.win_use, 0, sizeof(g_dl.win_use));   // KH_DL_SLOT_LRU.
    g_dl.win_use_n = 0;

    memset(g_dl.win_hint_set, 0, sizeof(g_dl.win_hint_set));
    memset(g_dl.win_hint_kind, 0, sizeof(g_dl.win_hint_kind));
    memset(g_dl.win_hint_off, 0, sizeof(g_dl.win_hint_off));
    memset(g_dl.win_hint_form, 0, sizeof(g_dl.win_hint_form));
    memset(g_dl.win_hint_woff, 0, sizeof(g_dl.win_hint_woff));
    memset(g_dl.win_hint_wform, 0, sizeof(g_dl.win_hint_wform));
    for (uint32_t khh_i = 0; khh_i < KH_DL_WIN_SLOTS; ++khh_i) {
        g_dl.win_hint_slot[khh_i] = -1;
        g_dl.win_hint_wslot[khh_i] = -1;
    }

}
}