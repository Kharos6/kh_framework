#pragma once

// The shared head (rendering_integration.hpp): the terrain heightmap's data (g_thm_*). Never include it on its own.
namespace RenderIntegration {
static std::vector<float>    g_thm_data;
static UINT g_thm_w = 0;
static UINT g_thm_h = 0;
static float                 g_thm_origin[2] = {};
static float                 g_thm_cell = 0.0f;
// KH_THM_EXACT - the engine's triangle split for the staging grid, as KhThmHeight reads it (thmParams.w - 1): class
// k's split (kh_cloth_ground_prepare's g_ground_split: 0, 1, or 2 = bilinear) in base-3 digit k. A grid not at the
// matrix's native spacing, or a split not measured, is bilinear throughout (KH_THM_SPLIT_SMOOTH). The autobuild writes
// it with the grid; the upload publishes it (g_thml_split) with the grid's other live meta.
static constexpr int         KH_THM_SPLIT_SMOOTH = 2 + 2 * 3 + 2 * 9 + 2 * 27;
static int                   g_thm_split = KH_THM_SPLIT_SMOOTH;
// KH_NO_PARK_THM: hand-off between the autobuild (game thread writes the staging grid, then sets it) and
// kh_thm_upload (the flush reads the grid, then clears it). The builder waits while it is set, so the grid is frozen
// until the upload's last read.
static std::atomic<bool>     g_thm_dirty{ false };
// Live texture usable. Written by kh_thm_upload_body (flush_locked's housekeeping: the render thread or a park) and
// by the device release (kh_thm_device_lost); read by the drawers on those same threads, and by KH_FX_LATE's
// chain-only flush (kh_fill_occ), which may be the game thread's inside a resolve the engine issues there.
static std::atomic<bool>     g_thm_valid{ false };
// KH_THM_DEVRESET: a Resources::release dropped a live heightfield texture (render thread, or the game thread
// under the graphics lock). kh_thm_autobuild_step (game thread) turns it into a g_thm_dirty raise once the grid is
// complete - the flag stays the autobuild -> flush hand-off, never raised beside the autobuild's writes.
static std::atomic<bool>     g_thm_reupload{ false };
}