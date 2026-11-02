#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of the terrain
// heightmap (KH_THM): its lanes and upload. Never include it on its own.
namespace RenderIntegration {

static constexpr float KH_THM_BIAS_M = 0.35f;
static constexpr float KH_THM_MIN_DIST_M = 1000.0f;   // March gate (endpoint test is ungated).
// KH_THM_EXACT - the widest grid a side the heightfield texture takes (D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION): the
// matrix is taken whole up to it, point-sampled past it (maps finer than 16384 nodes a side).
static constexpr size_t      KH_THM_MAX_SIDE = 16384;
static float                 g_thml_origin[2] = {};   // Live meta (published by the flush).
static float                 g_thml_cell = 0.0f;
static int                   g_thml_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT: g_thm_split, live.
static UINT g_thml_w = 0;
static UINT g_thml_h = 0;
// Zero-setup terrain acquisition: at the flush prelude (game thread, pre-lock) the builder acquires the heightfield
// itself.
static int                   g_thm_auto_state = 0;   // 0 idle / 1 refining / 2 done / 3 failed.
static size_t                g_thm_auto_cursor = 0;
static int                   g_thm_auto_retry = 0;
static UINT                  g_thmf_dim = 0;   // fine-pass dims/cell (state 1).
static float                 g_thmf_cell = 0.0f;
}