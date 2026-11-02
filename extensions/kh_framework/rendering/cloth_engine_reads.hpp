#pragma once

// The injection half (rendering_integration_injection.hpp): the cloth's engine reads: the wind sample and the ground
// patch preparation. Never include it on its own.
namespace RenderIntegration {

// SQF wind is [x, y, z] in m/s on the SQF axes (z up); the simulation runs in engine order (east, up, north), the swap
// kh_cloth_engine_pos makes for a position. Game thread only (an engine call).
inline void kh_cloth_wind_sample() {
    try {
        const vector3 khcw_w = sqf::wind();
        const float khcw_v[3] = { static_cast<float>(khcw_w.x),
                                  static_cast<float>(khcw_w.y),
                                  static_cast<float>(khcw_w.z) };
        for (int k = 0; k < 3; ++k) if (!(khcw_v[k] == khcw_v[k])) return;   // A NaN is not a wind.
        g_cloth_wind[0] = khcw_v[0];
        g_cloth_wind[1] = khcw_v[2];
        g_cloth_wind[2] = khcw_v[1];
    } catch (...) {
        // Wind unavailable this frame: the last sample stands.
    }
}

// Game thread, holding no lock, outside the park: engine calls (at most 32, once per matrix and mission -
// reset_retained_state re-arms it).
inline void kh_cloth_ground_prepare() {
    const size_t khgp_n = g_terrain_matrix.size();
    const float khgp_c = g_terrain_grid_width;
    if (khgp_n < 2 || !(khgp_c > 0.0f)) { g_ground_ready = false; return; }
    if (g_ground_tried && khgp_c == g_ground_cell && khgp_n == g_ground_n) return;
    g_ground_tried = true;
    g_ground_ready = false;
    g_ground_cell = khgp_c;
    g_ground_n = khgp_n;
    for (size_t khgp_r = 0; khgp_r < khgp_n; ++khgp_r) {
        if (g_terrain_matrix[khgp_r].size() != khgp_n) return;   // Not square: not the matrix described above.
    }
    // A cell answers only if its diagonals disagree by more than the engine's own float noise can hide: the two
    // candidates then stand at least half of KH_GROUND_TELL apart at the centre, and the engine's answer lands on one.
    constexpr float KH_GROUND_TELL = 0.05f;   // m: |(h00 + h11) - (h10 + h01)|, twice the diagonals' mid-heights' gap.
    constexpr float KH_GROUND_HIT = 0.01f;    // m: the engine's answer on a candidate.
    constexpr int   KH_GROUND_ASK = 8;        // Cells asked per class.
    int khgp_votes[4][3] = {};                // Per class: diagonal 0, diagonal 1, neither.
    int khgp_asked[4] = {};
    const size_t khgp_stride = khgp_n / 48u + 1u;
    try {
        for (size_t khgp_j = 0; khgp_j + 2 < khgp_n; khgp_j += khgp_stride) {
            for (size_t khgp_i = 0; khgp_i + 2 < khgp_n; khgp_i += khgp_stride) {
                for (int khgp_q = 0; khgp_q < 4; ++khgp_q) {   // The 2 x 2 block here holds one cell of each class.
                    const size_t khgp_ci = khgp_i + static_cast<size_t>(khgp_q & 1);
                    const size_t khgp_cj = khgp_j + static_cast<size_t>(khgp_q >> 1);
                    const int khgp_cls = static_cast<int>((khgp_ci & 1u) | ((khgp_cj & 1u) << 1));
                    if (khgp_asked[khgp_cls] >= KH_GROUND_ASK) continue;
                    const float h00 = g_terrain_matrix[khgp_cj][khgp_ci], h10 = g_terrain_matrix[khgp_cj][khgp_ci + 1];
                    const float h01 = g_terrain_matrix[khgp_cj + 1][khgp_ci], h11 = g_terrain_matrix[khgp_cj + 1][khgp_ci + 1];
                    if (!(fabsf((h00 + h11) - (h10 + h01)) > KH_GROUND_TELL)) continue;
                    // The framework sampled the nodes at float x * spacing; the centre is asked the same way.
                    const float khgp_x = (static_cast<float>(khgp_ci) + 0.5f) * khgp_c;
                    const float khgp_y = (static_cast<float>(khgp_cj) + 0.5f) * khgp_c;
                    const float khgp_hc = static_cast<float>(sqf::atl_to_asl(vector3(khgp_x, khgp_y, 0.0f)).z);
                    const float khgp_d0 = fabsf(khgp_hc - 0.5f * (h00 + h11));
                    const float khgp_d1 = fabsf(khgp_hc - 0.5f * (h10 + h01));
                    ++khgp_asked[khgp_cls];
                    if (khgp_d0 < KH_GROUND_HIT && khgp_d1 > 2.0f * KH_GROUND_HIT) ++khgp_votes[khgp_cls][0];
                    else if (khgp_d1 < KH_GROUND_HIT && khgp_d0 > 2.0f * KH_GROUND_HIT) ++khgp_votes[khgp_cls][1];
                    else ++khgp_votes[khgp_cls][2];
                }
            }
        }
    } catch (...) {
        report_error_once_safe("KH cloth ground: the terrain height query failed - cloth does not land on the terrain this mission");
        return;
    }
    // A class decides when every cell asked agreed. One with no cell to ask (flat country) takes the decided classes'
    // answer if they agree; its own cells are planar or near it, so either split is exact or within KH_GROUND_TELL / 2.
    int khgp_dec[4];
    for (int khgp_k = 0; khgp_k < 4; ++khgp_k) {
        const int* v = khgp_votes[khgp_k];
        khgp_dec[khgp_k] = khgp_asked[khgp_k] == 0 ? -1
                         : (v[0] == khgp_asked[khgp_k] ? 0 : (v[1] == khgp_asked[khgp_k] ? 1 : 2));
    }
    int khgp_common = -1;
    bool khgp_agree = true;
    for (int khgp_k = 0; khgp_k < 4; ++khgp_k) {
        if (khgp_dec[khgp_k] < 0) continue;
        if (khgp_common < 0) khgp_common = khgp_dec[khgp_k];
        else if (khgp_common != khgp_dec[khgp_k]) khgp_agree = false;
    }
    bool khgp_smooth = false;
    for (int khgp_k = 0; khgp_k < 4; ++khgp_k) {
        int d = khgp_dec[khgp_k];
        if (d < 0) d = khgp_agree && khgp_common >= 0 ? khgp_common : (khgp_common < 0 ? 0 : 2);
        if (d == 2) khgp_smooth = true;
        g_ground_split[khgp_k] = static_cast<uint8_t>(d);
    }
    if (khgp_smooth) {   // Information, not a fault (game thread: a plain log line, once per matrix).
        sqf::diag_log("KH cloth ground: the terrain's triangle split was not recognised (grid " +
                      std::to_string(khgp_c) + " m) - cloth lands, and meshes are clipped, on a smooth "
                      "fit of the terrain, which departs from it on uneven ground");
    }
    g_ground_ready = true;
}
}