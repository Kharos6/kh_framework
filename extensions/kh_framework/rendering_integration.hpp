#pragma once

#define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

// ufbx, mikktspace, stb_image, <fstream>, <memory>, <intrin.h> and <smmintrin.h> (WC_COPY's __cpuid, _mm_mfence and
// _mm_stream_load_si128) are included by the framework header ahead of this file.

// The integration headers are CRLF-only, tab-free ASCII. Shaders are plain .hlsl RCDATA resources (kh_shaders.rc).

// ===========================================================================
// SQF COMMAND NOTES - maintainer notes on the renderer's script surface (the three headers). What each command,
// property and key accepts, defaults to, clamps to and returns is documented for script authors in a separate
// reference: a parser change a script can see (sqf_integration.hpp: add_render3d_sqf, kh_apply_render3d_prop,
// update_post_fx_sqf, add_physics_affector_sqf, ...) is an edit there too. User .hlsl: cb.hlsl's USER SHADER CONTRACT.
// getRenderStats' fields: get_render_stats_sqf (sqf_integration.hpp).
//
// RANGE RULE, every parser alike: a number outside its key's range is clamped to it silently (the solvers and passes
// are stable over those ranges). Refused with a report instead: a blend mode, a depth mode (mode), an effect, a mask
// shape or a mesh index given as a number, a material's fresnel pair, every texture-animation key, vertexBound, a
// simulationLod pair out of order, a physics affector's lifetime, position, vectorDir or scale number that is not
// finite or is 1e30 or more in size (kh_aff_finite), and an [x, y, z] scale half-extent of 0 or below. A number for
// one of the first five is truncated toward zero first (gv_int: blend 5.9 is 5, -0.5 is 0). A NaN is refused with
// a report by every parser, and so is an infinity in a material key (MAT_FINITE) or where the key has no range to
// clamp it to (a position, size, radius, falloff, band, rotation, lit pair or effect params: KH_RV_FINITE,
// FX_FINITE); elsewhere an infinity is a number past its range (a duration part's is accepted: DURATION_NAN).
//
// PLAYER_ONLY: graphics only where there is an interface (hasInterface, framework.hpp's g_is_player). On a
// dedicated server or a headless client every command returns its empty value at once ('' / false / []): no parse, no
// report, nothing created (kh_gfx_off, sqf_integration.hpp; render_on below).
//
// MISSION RESET: every mission start clears the last mission's meshes, passes and physics affectors and puts the
// settings back to their defaults, in Intercept's pre-init (a CBA XEH preInit handler; rendering_integration_reset). A
// command run before it at the same start (a preInit function the game calls ahead of Intercept's) is cleared with the
// rest, unreported: the first commands must come from postInit or later.
//
// SHADER_FIRST_RUN - cold-cache cost: some 130 units (plus the placeholder's two), about 18 minutes of one-core
// compile work (d3dcompiler_47, one core of a loaded machine): the six mesh and injection shaders 35 - 45 s each, the
// four full effect shaders 2 - 4 min each, the rest seconds. One worker per hardware thread, and the white placeholder
// (no post-processing) holds until the last unit lands, so the first start takes at least the longest unit's time:
// about 9.5 - 10 min on 2 threads, 5 on 4, 4 - 4.5 on 8 (the queue simulated with those times). Warm (khs_*.khsc in
// Documents or a mod's 'cache'): a frame or more of placeholder at each mission start and device reset. A change to
// any file a unit is composed of (cb.hlsl is in most) recompiles that unit on every player's machine.
// ===========================================================================

// The shared head, in parts in rendering/, one feature each, in this order: a part may use what the parts above it
// declare, so the order is load-bearing. The reader's guide is rendering/README.md: the layout, the threads, the
// cycle, the naming, the abbreviations, the state map and the limitations - start reading there.
#include "rendering/profiling.hpp"
#include "rendering/mesh_types.hpp"
#include "rendering/physics_types.hpp"
#include "rendering/material_types.hpp"
#include "rendering/bloom_types.hpp"
#include "rendering/render_object.hpp"
#include "rendering/scene_state.hpp"
#include "rendering/attachment_types.hpp"
#include "rendering/stats.hpp"
#include "rendering/see_through_state.hpp"
#include "rendering/constant_data.hpp"
#include "rendering/core.hpp"
#include "rendering/physics_state.hpp"
#include "rendering/culling_state.hpp"
#include "rendering/frame_state.hpp"
#include "rendering/terrain_state.hpp"
#include "rendering/reorder_state.hpp"
#include "rendering/engine_light_state.hpp"
#include "rendering/live_shadow_state.hpp"
#include "rendering/view_model_state.hpp"
#include "rendering/sun_shadow_state.hpp"
#include "rendering/dynamic_light_state.hpp"
#include "rendering/gpu_resources.hpp"
#include "rendering/instancing_types.hpp"
#include "rendering/state_backup.hpp"
#include "rendering/effect_state.hpp"
#include "rendering/engine_decls.hpp"

// The renderer in two halves, between the shared head above and the session lifecycle below: the feature header
// (our renderer: meshes, materials, physics, shaders, effects, lighting and shadows) first, then the injection
// header (the bridge to the engine: the D3D hooks, the scanners of the engine's uploads and memory, the injection
// sites and the drivers), which builds on both.
#include "rendering_integration_features.hpp"
#include "rendering_integration_injection.hpp"

// The shared tail: the session lifecycle and the render-object API.
#include "rendering/session_lifecycle.hpp"
#include "rendering/render_object_api.hpp"

#undef KH_SAFE_RELEASE