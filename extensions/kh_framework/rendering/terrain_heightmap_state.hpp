#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of the terrain
// heightmap (THM): its lanes and upload. Never include it on its own.
namespace RenderIntegration {

static constexpr float THM_BIAS_M = 0.35f;
static constexpr float THM_MIN_DIST_M = 1000.0f;   // March gate (endpoint test is ungated).
// THM_EXACT - the widest grid a side the heightfield texture takes (D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION): the
// matrix is taken whole up to it, point-sampled past it (maps finer than 16384 nodes a side).
static constexpr size_t      THM_MAX_SIDE = 16384;
// The heightfield's layout as last uploaded (origin, cell, split, size).
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets every member.
struct TerrainLayoutState {
    float                 origin[2] = {};   // Live meta (published by the flush).
    float                 cell = 0.0f;
    int                   split = THM_SPLIT_SMOOTH;   // THM_EXACT: g_thm.split, live.
    UINT w = 0;
    UINT h = 0;
};
static TerrainLayoutState g_thml;
// The heightfield's automatic build (terrain_autobuild): its state, cursor and retry.
// Owner: the game thread (thm_autobuild_step from flush_frame, ahead of the graphics lock).
// Session reset: session_globals_reset resets every member.
struct TerrainAutoState {
    // Zero-setup terrain acquisition: at the flush prelude (game thread, pre-lock) the builder acquires the heightfield
    // itself.
    int                   state = 0;   // 0 idle / 1 refining / 2 done / 3 failed.
    size_t                cursor = 0;
    int                   retry = 0;
};
static TerrainAutoState g_thm_auto;
static UINT                  g_thmf_dim = 0;   // fine-pass dims/cell (state 1).
static float                 g_thmf_cell = 0.0f;
}