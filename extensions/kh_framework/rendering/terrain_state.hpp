#pragma once

// The shared head (rendering_integration.hpp): the terrain heightmap's data (g_thm). Never include it on its own.
namespace RenderIntegration {
// THM_EXACT - the engine's triangle split for the staging grid, as KhThmHeight reads it (thmParams.w - 1): class
// k's split (cloth_ground_prepare's g_ground_split: 0, 1, or 2 = bilinear) in base-3 digit k. A grid not at the
// matrix's native spacing, or a split not measured, is bilinear throughout (THM_SPLIT_SMOOTH). The autobuild writes
// it with the grid; the upload publishes it (g_thml.split) with the grid's other live meta.
static constexpr int         THM_SPLIT_SMOOTH = 2 + 2 * 3 + 2 * 9 + 2 * 27;
// The analytic terrain heightfield: the CPU copy and its grid, and the upload flags.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Also on the game thread: data, w, h, origin, cell, split, dirty, valid, reupload.
// Session reset: session_globals_reset resets every member.
struct TerrainHeightState {
    std::vector<float>    data;
    UINT w = 0;
    UINT h = 0;
    float                 origin[2] = {};
    float                 cell = 0.0f;
    int                   split = THM_SPLIT_SMOOTH;
    // NO_PARK_THM: hand-off between the autobuild (game thread writes the staging grid, then sets it) and
    // thm_upload (the flush reads the grid, then clears it). The builder waits while it is set, so the grid is
    // frozen until the upload's last read.
    std::atomic<bool>     dirty{ false };
    // Live texture usable. Written by thm_upload_body (flush_locked's housekeeping: the render thread or a park) and
    // by the device release (thm_device_lost); read by the drawers on those same threads, and by FX_LATE's
    // chain-only flush (fill_occ), which may be the game thread's inside a resolve the engine issues there.
    std::atomic<bool>     valid{ false };
    // THM_DEVRESET: a Resources::release dropped a live heightfield texture (render thread, or the game thread
    // under the graphics lock). thm_autobuild_step (game thread) turns it into a g_thm.dirty raise once the grid is
    // complete - the flag stays the autobuild -> flush hand-off, never raised beside the autobuild's writes.
    std::atomic<bool>     reupload{ false };
};
static TerrainHeightState g_thm;
}