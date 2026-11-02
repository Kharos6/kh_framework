#pragma once

// The shared head (rendering_integration.hpp): the engine's sun and published lighting block as both halves read them.
// Never include it on its own.
namespace RenderIntegration {

static bool  g_sun_valid = false;
static float g_sun_dir_engine[3] = { 0.0f, 1.0f, 0.0f };   // Unit vector toward the light, engine axes.
static bool g_atlas_frame_cur = false;
static bool g_atlas_frame_prev = false;

static bool  g_pub_block_valid = false;

static float g_pub_block_amb[3] = {};
static float g_pub_block_sun[3] = {};
}