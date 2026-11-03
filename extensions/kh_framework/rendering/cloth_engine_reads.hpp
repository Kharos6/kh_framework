#pragma once

// The injection half (rendering_integration_injection.hpp): the cloth's engine reads: the wind sample and the ground
// patch preparation. Never include it on its own.
namespace RenderIntegration {

// SQF wind is [x, y, z] in m/s on the SQF axes (z up); the simulation runs in engine order (east, up, north), the swap
// cloth_engine_pos makes for a position. Game thread only (an engine call).
inline void cloth_wind_sample() {
    try {
        const vector3 w = sqf::wind();
        const float v[3] = { static_cast<float>(w.x),
                             static_cast<float>(w.y),
                             static_cast<float>(w.z) };
        for (int k = 0; k < 3; ++k) if (!(v[k] == v[k])) return;   // A NaN is not a wind.
        g_cloth_wind[0] = v[0];
        g_cloth_wind[1] = v[2];
        g_cloth_wind[2] = v[1];
    } catch (...) {
        // Wind unavailable this frame: the last sample stands.
    }
}

// Game thread, holding no lock, outside the park: engine calls (at most 32, once per matrix and mission -
// reset_retained_state re-arms it).
inline void cloth_ground_prepare() {
    const size_t n = g_terrain_matrix.size();
    const float c = g_terrain_grid_width;
    if (n < 2 || !(c > 0.0f)) { g_ground_ready = false; return; }
    if (g_ground_tried && c == g_ground_cell && n == g_ground_n) return;
    g_ground_tried = true;
    g_ground_ready = false;
    g_ground_cell = c;
    g_ground_n = n;
    for (size_t r = 0; r < n; ++r) {
        if (g_terrain_matrix[r].size() != n) return;   // Not square: not the matrix described above.
    }
    // A cell answers only if its diagonals disagree by more than the engine's own float noise can hide: the two
    // candidates then stand at least half of GROUND_TELL apart at the centre, and the engine's answer lands on one.
    constexpr float GROUND_TELL = 0.05f;   // m: |(h00 + h11) - (h10 + h01)|, twice the diagonals' mid-heights' gap.
    constexpr float GROUND_HIT = 0.01f;    // m: the engine's answer on a candidate.
    constexpr int   GROUND_ASK = 8;        // Cells asked per class.
    int votes[4][3] = {};                // Per class: diagonal 0, diagonal 1, neither.
    int asked[4] = {};
    const size_t stride = n / 48u + 1u;
    try {
        for (size_t j = 0; j + 2 < n; j += stride) {
            for (size_t i = 0; i + 2 < n; i += stride) {
                for (int q = 0; q < 4; ++q) {   // The 2 x 2 block here holds one cell of each class.
                    const size_t ci = i + static_cast<size_t>(q & 1);
                    const size_t cj = j + static_cast<size_t>(q >> 1);
                    const int cls = static_cast<int>((ci & 1u) | ((cj & 1u) << 1));
                    if (asked[cls] >= GROUND_ASK) continue;
                    const float h00 = g_terrain_matrix[cj][ci], h10 = g_terrain_matrix[cj][ci + 1];
                    const float h01 = g_terrain_matrix[cj + 1][ci], h11 = g_terrain_matrix[cj + 1][ci + 1];
                    if (!(fabsf((h00 + h11) - (h10 + h01)) > GROUND_TELL)) continue;
                    // The framework sampled the nodes at float x * spacing; the centre is asked the same way.
                    const float x = (static_cast<float>(ci) + 0.5f) * c;
                    const float y = (static_cast<float>(cj) + 0.5f) * c;
                    const float hc = static_cast<float>(sqf::atl_to_asl(vector3(x, y, 0.0f)).z);
                    const float d0 = fabsf(hc - 0.5f * (h00 + h11));
                    const float d1 = fabsf(hc - 0.5f * (h10 + h01));
                    ++asked[cls];
                    if (d0 < GROUND_HIT && d1 > 2.0f * GROUND_HIT) ++votes[cls][0];
                    else if (d1 < GROUND_HIT && d0 > 2.0f * GROUND_HIT) ++votes[cls][1];
                    else ++votes[cls][2];
                }
            }
        }
    } catch (...) {
        report_error_once_safe("KH RenderIntegration: cloth ground: the terrain height query failed - cloth does not "
                               "land on the terrain this mission");
        return;
    }
    // A class decides when every cell asked agreed. One with no cell to ask (flat country) takes the decided classes'
    // answer if they agree; its own cells are planar or near it, so either split is exact or within GROUND_TELL / 2.
    int dec[4];
    for (int k = 0; k < 4; ++k) {
        const int* v = votes[k];
        dec[k] = asked[k] == 0 ? -1
               : (v[0] == asked[k] ? 0 : (v[1] == asked[k] ? 1 : 2));
    }
    int common = -1;
    bool agree = true;
    for (int k = 0; k < 4; ++k) {
        if (dec[k] < 0) continue;
        if (common < 0) common = dec[k];
        else if (common != dec[k]) agree = false;
    }
    bool smooth = false;
    for (int k = 0; k < 4; ++k) {
        int d = dec[k];
        if (d < 0) d = agree && common >= 0 ? common : (common < 0 ? 0 : 2);
        if (d == 2) smooth = true;
        g_ground_split[k] = static_cast<uint8_t>(d);
    }
    if (smooth) {   // Information, not a fault (game thread: a plain log line, once per matrix).
        sqf::diag_log("KH RenderIntegration: cloth ground: the terrain's triangle split was not recognised (grid " +
                      std::to_string(c) + " m) - cloth lands, and meshes are clipped, on a smooth "
                      "fit of the terrain, which departs from it on uneven ground");
    }
    g_ground_ready = true;
}
}