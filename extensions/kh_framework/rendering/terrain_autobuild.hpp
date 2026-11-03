#pragma once

// The injection half (rendering_integration_injection.hpp): the terrain heightmap's automatic build from the engine
// (thm_autobuild_step). Never include it on its own.
namespace RenderIntegration {

// Game thread, flush prelude, pre-lock. The coarse seed is synchronous and
// one-time; refinement is budgeted at 4096 samples per call.
inline void thm_autobuild_step() {
    KH_PROF_SCOPE(PROF_THM_AUTOBUILD);   // PROF.
    if (g_thm.dirty) return;
    if (g_thm_auto.state == 2) {
        // THM_DEVRESET: the grid is complete and no longer written - the flush uploads it again.
        if (g_thm.reupload.exchange(false, std::memory_order_relaxed)) g_thm.dirty = true;
        return;
    }
    // Any other state ends in an upload of its own (a build, a refinement, a retry).
    g_thm.reupload.store(false, std::memory_order_relaxed);

    if (g_thm_auto.state == 3) {   // Failed: re-attempt (~5 s cadence).
        if (++g_thm_auto.retry < 300) return;
        g_thm_auto.retry = 0;
        g_thm_auto.state = 0;
    }

    try {
        if (g_thm_auto.state == 0) {
            // The framework's terrain matrix, first: the cloth lands on it at full resolution (CLOTH_GROUND) and
            // path A below takes this heightfield from it. Built once per world and cached across missions by the
            // framework; the first build is synchronous, one engine height query per terrain-grid node, on this thread.
            ::initialize_terrain_matrix();
            // Path A: the framework's native-resolution terrain matrix, whole (THM_EXACT: point-sampled only past
            // THM_MAX_SIDE nodes a side), with the engine's triangle split; nothing to refine.
            if (!g_terrain_matrix.empty() && g_terrain_grid_width > 0.0f) {
                // THM_EXACT: the split, measured on this matrix (engine calls: this thread, holding no lock, as
                // the matrix's own build; once per matrix and mission, shared with the cloth ground).
                cloth_ground_prepare();
                const size_t src = g_terrain_matrix.size();
                const size_t stride = (src + THM_MAX_SIDE - 1) / THM_MAX_SIDE > 0
                                    ? (src + THM_MAX_SIDE - 1) / THM_MAX_SIDE : 1;
                const size_t ow = (src + stride - 1) / stride;
                g_thm.origin[0] = 0.0f;
                g_thm.origin[1] = 0.0f;
                g_thm.cell = g_terrain_grid_width * static_cast<float>(stride);
                g_thm.w = static_cast<UINT>(ow);
                g_thm.h = static_cast<UINT>(ow);
                g_thm.data.assign(ow * ow, 0.0f);

                for (size_t oy = 0; oy < ow; ++oy) {
                    const size_t y = oy * stride < src ? oy * stride : src - 1;
                    const auto& row = g_terrain_matrix[y];

                    for (size_t ox = 0; ox < ow; ++ox) {
                        const size_t x = ox * stride < row.size()
                                       ? ox * stride : (row.size() ? row.size() - 1 : 0);
                        g_thm.data[oy * ow + ox] = row.empty() ? 0.0f : row[x];
                    }
                }

                // THM_EXACT: the split applies to the matrix's own cells alone - taken whole, and measured on it.
                g_thm.split = THM_SPLIT_SMOOTH;
                if (stride == 1 && g_ground_ready && g_ground_n == src &&
                    g_ground_cell == g_terrain_grid_width) {
                    g_thm.split = g_ground_split[0] + 3 * g_ground_split[1] + 9 * g_ground_split[2] +
                                  27 * g_ground_split[3];
                }
                g_thm.dirty = true;
                g_thm_auto.state = 2;
                return;
            }

            // Path B, step 1: synchronous coarse seed (65 x 65); terrain occlusion goes live at the next flush.
            const float ws = static_cast<float>(sqf::world_size());

            if (ws <= 1.0f) {
                g_thm_auto.state = 3;
                return;
            }

            const UINT cd = 65;
            const float cc = ws / 64.0f;
            g_thm.split = THM_SPLIT_SMOOTH;   // THM_EXACT: sampled, not the engine's nodes - a bilinear fit.
            g_thm.origin[0] = 0.0f;
            g_thm.origin[1] = 0.0f;
            g_thm.cell = cc;
            g_thm.w = cd;
            g_thm.h = cd;
            g_thm.data.assign(static_cast<size_t>(cd) * cd, 0.0f);

            for (UINT iy = 0; iy < cd; ++iy) {
                for (UINT ix = 0; ix < cd; ++ix) {
                    const vector3 asl = sqf::atl_to_asl(vector3(
                        static_cast<float>(ix) * cc,
                        static_cast<float>(iy) * cc, 0.0f));
                    g_thm.data[static_cast<size_t>(iy) * cd + ix] = asl.z;
                }
            }

            g_thm.dirty = true;   // Coarse uploads at this flush.

            // Step 2 parameters: full-resolution refinement.
            float fc = ws / 1024.0f;
            if (fc < 10.0f) fc = 10.0f;
            if (fc > 200.0f) fc = 200.0f;
            g_thmf_dim = static_cast<UINT>(ws / fc) + 1;
            g_thmf_cell = fc;
            g_thm_auto.cursor = 0;
            g_thm_auto.state = 1;
            return;
        }

        if (g_thm_auto.state == 1) {
            // Path B, step 2: background refinement into staging (the coarse grid stays live until the swap at the
            // flush).
            if (g_thm.w != g_thmf_dim) {
                g_thm.cell = g_thmf_cell;
                g_thm.w = g_thmf_dim;
                g_thm.h = g_thmf_dim;
                g_thm.data.assign(static_cast<size_t>(g_thmf_dim) * g_thmf_dim, 0.0f);
                g_thm_auto.cursor = 0;
            }

            const size_t total = static_cast<size_t>(g_thm.w) * g_thm.h;
            size_t budget = 4096;

            while (budget-- != 0 && g_thm_auto.cursor < total) {
                const size_t iy = g_thm_auto.cursor / g_thm.w;
                const size_t ix = g_thm_auto.cursor % g_thm.w;
                const vector3 asl = sqf::atl_to_asl(vector3(
                    static_cast<float>(ix) * g_thm.cell,
                    static_cast<float>(iy) * g_thm.cell, 0.0f));
                g_thm.data[g_thm_auto.cursor] = asl.z;
                g_thm_auto.cursor++;
            }

            if (g_thm_auto.cursor >= total) {
                g_thm.dirty = true;   // full-res hot-swap at the next flush.
                g_thm_auto.state = 2;
            }
        }
    } catch (...) {
        g_thm_auto.state = 3;
        g_thm_auto.retry = 0;
        // EA_ERR_CLASS: transient, retried (~5 s) - a warning, logged once a session (never thrown).
        log_once_safe("KH RenderIntegration: terrain auto-acquisition: engine query failed (will retry)");
    }
}
}