#pragma once

// The injection half (rendering_integration_injection.hpp): the terrain heightmap's automatic build from the engine
// (kh_thm_autobuild_step). Never include it on its own.
namespace RenderIntegration {

// Game thread, flush prelude, pre-lock. The coarse seed is synchronous and
// one-time; refinement is budgeted at 4096 samples per call.
inline void kh_thm_autobuild_step() {
    KH_PROF_SCOPE(KHP_THM_AUTOBUILD);   // KH_PROF.
    if (g_thm_dirty) return;
    if (g_thm_auto_state == 2) {
        // KH_THM_DEVRESET: the grid is complete and no longer written - the flush uploads it again.
        if (g_thm_reupload.exchange(false, std::memory_order_relaxed)) g_thm_dirty = true;
        return;
    }
    // Any other state ends in an upload of its own (a build, a refinement, a retry).
    g_thm_reupload.store(false, std::memory_order_relaxed);

    if (g_thm_auto_state == 3) {   // Failed: re-attempt (~5 s cadence).
        if (++g_thm_auto_retry < 300) return;
        g_thm_auto_retry = 0;
        g_thm_auto_state = 0;
    }

    try {
        if (g_thm_auto_state == 0) {
            // The framework's terrain matrix, first: the cloth lands on it at full resolution (KH_CLOTH_GROUND) and
            // path A below takes this heightfield from it. Built once per world and cached across missions by the
            // framework; the first build is synchronous, one engine height query per terrain-grid node, on this thread.
            ::initialize_terrain_matrix();
            // Path A: the framework's native-resolution terrain matrix, whole (KH_THM_EXACT: point-sampled only past
            // KH_THM_MAX_SIDE nodes a side), with the engine's triangle split; nothing to refine.
            if (!g_terrain_matrix.empty() && g_terrain_grid_width > 0.0f) {
                // KH_THM_EXACT: the split, measured on this matrix (engine calls: this thread, holding no lock, as
                // the matrix's own build; once per matrix and mission, shared with the cloth ground).
                kh_cloth_ground_prepare();
                const size_t khb_src = g_terrain_matrix.size();
                const size_t khb_stride = (khb_src + KH_THM_MAX_SIDE - 1) / KH_THM_MAX_SIDE > 0
                                        ? (khb_src + KH_THM_MAX_SIDE - 1) / KH_THM_MAX_SIDE : 1;
                const size_t khb_ow = (khb_src + khb_stride - 1) / khb_stride;
                g_thm_origin[0] = 0.0f;
                g_thm_origin[1] = 0.0f;
                g_thm_cell = g_terrain_grid_width * static_cast<float>(khb_stride);
                g_thm_w = static_cast<UINT>(khb_ow);
                g_thm_h = static_cast<UINT>(khb_ow);
                g_thm_data.assign(khb_ow * khb_ow, 0.0f);

                for (size_t khb_oy = 0; khb_oy < khb_ow; ++khb_oy) {
                    const size_t khb_y = khb_oy * khb_stride < khb_src ? khb_oy * khb_stride : khb_src - 1;
                    const auto& khb_row = g_terrain_matrix[khb_y];

                    for (size_t khb_ox = 0; khb_ox < khb_ow; ++khb_ox) {
                        const size_t khb_x = khb_ox * khb_stride < khb_row.size()
                                           ? khb_ox * khb_stride : (khb_row.size() ? khb_row.size() - 1 : 0);
                        g_thm_data[khb_oy * khb_ow + khb_ox] = khb_row.empty() ? 0.0f : khb_row[khb_x];
                    }
                }

                // KH_THM_EXACT: the split applies to the matrix's own cells alone - taken whole, and measured on it.
                g_thm_split = KH_THM_SPLIT_SMOOTH;
                if (khb_stride == 1 && g_ground_ready && g_ground_n == khb_src &&
                    g_ground_cell == g_terrain_grid_width) {
                    g_thm_split = g_ground_split[0] + 3 * g_ground_split[1] + 9 * g_ground_split[2] +
                                  27 * g_ground_split[3];
                }
                g_thm_dirty = true;
                g_thm_auto_state = 2;
                return;
            }

            // Path B, step 1: synchronous coarse seed (65 x 65); terrain occlusion goes live at the next flush.
            const float khb_ws = static_cast<float>(sqf::world_size());

            if (khb_ws <= 1.0f) {
                g_thm_auto_state = 3;
                return;
            }

            const UINT khb_cd = 65;
            const float khb_cc = khb_ws / 64.0f;
            g_thm_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT: sampled, not the engine's nodes - a bilinear fit.
            g_thm_origin[0] = 0.0f;
            g_thm_origin[1] = 0.0f;
            g_thm_cell = khb_cc;
            g_thm_w = khb_cd;
            g_thm_h = khb_cd;
            g_thm_data.assign(static_cast<size_t>(khb_cd) * khb_cd, 0.0f);

            for (UINT khb_iy = 0; khb_iy < khb_cd; ++khb_iy) {
                for (UINT khb_ix = 0; khb_ix < khb_cd; ++khb_ix) {
                    const vector3 khb_asl = sqf::atl_to_asl(vector3(
                        static_cast<float>(khb_ix) * khb_cc,
                        static_cast<float>(khb_iy) * khb_cc, 0.0f));
                    g_thm_data[static_cast<size_t>(khb_iy) * khb_cd + khb_ix] = khb_asl.z;
                }
            }

            g_thm_dirty = true;   // Coarse uploads at this flush.

            // Step 2 parameters: full-resolution refinement.
            float khb_fc = khb_ws / 1024.0f;
            if (khb_fc < 10.0f) khb_fc = 10.0f;
            if (khb_fc > 200.0f) khb_fc = 200.0f;
            g_thmf_dim = static_cast<UINT>(khb_ws / khb_fc) + 1;
            g_thmf_cell = khb_fc;
            g_thm_auto_cursor = 0;
            g_thm_auto_state = 1;
            return;
        }

        if (g_thm_auto_state == 1) {
            // Path B, step 2: background refinement into staging (the coarse grid stays live until the swap at the
            // flush).
            if (g_thm_w != g_thmf_dim) {
                g_thm_cell = g_thmf_cell;
                g_thm_w = g_thmf_dim;
                g_thm_h = g_thmf_dim;
                g_thm_data.assign(static_cast<size_t>(g_thmf_dim) * g_thmf_dim, 0.0f);
                g_thm_auto_cursor = 0;
            }

            const size_t khb_total = static_cast<size_t>(g_thm_w) * g_thm_h;
            size_t khb_budget = 4096;

            while (khb_budget-- != 0 && g_thm_auto_cursor < khb_total) {
                const size_t khb_iy = g_thm_auto_cursor / g_thm_w;
                const size_t khb_ix = g_thm_auto_cursor % g_thm_w;
                const vector3 khb_asl = sqf::atl_to_asl(vector3(
                    static_cast<float>(khb_ix) * g_thm_cell,
                    static_cast<float>(khb_iy) * g_thm_cell, 0.0f));
                g_thm_data[g_thm_auto_cursor] = khb_asl.z;
                g_thm_auto_cursor++;
            }

            if (g_thm_auto_cursor >= khb_total) {
                g_thm_dirty = true;   // full-res hot-swap at the next flush.
                g_thm_auto_state = 2;
            }
        }
    } catch (...) {
        g_thm_auto_state = 3;
        g_thm_auto_retry = 0;
        // KH_EA_ERR_CLASS: transient, retried (~5 s) - a warning, logged once a session (never thrown).
        kh_log_once_safe("KH RenderIntegration: terrain auto-acquisition: engine query failed (will retry)");
    }
}
}