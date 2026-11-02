#pragma once

// The shared head (rendering_integration.hpp): the view-model slice and mirror flags both halves read. Never include it
// on its own.
namespace RenderIntegration {

static ID3D11VertexShader* g_vmir_vs = nullptr;

// The one rule for whether an object takes part in shadowing (kh_shadow_part: the seam footprints, which are also
// our pixel-ownership lists) and whether it casts (kh_shadow_active: the sun / DLS maps, the world cast, caster
// demand). An unlit mesh neither receives nor casts (effect meshes are unlit by construction). casterOnly admits an
// invisible object as a caster; KH_SHADOW_SWITCH: castShadow false makes it no caster at all, casterOnly or not, and
// so does every shadow off (KH_SHADOW_OFF): kh_cast_on. Every caster gather and demand census reads these -
// kh_cast_on where it has taken the part test already - never the fields directly.
// KH_INFRONT: whether the cycle that last ended drew the engine's view-model slice (a hands prepass or colour pass
// was seen). Written by the render thread at the main depth clear (kh_infront_frame_reset), read by every caster
// gather and demand census on either thread; atomic because the flush's pre-park census reads it outside the park.
// An inFront mesh casts only while this holds - in third person there is no slice, so no shadow from a mesh that is
// not drawn.
// Measured on foot with a weapon only; any other view counts by whether the game draws the slice in it - expected (not
// measured): none in third person, Zeus or a scripted camera; a first-person view of another unit draws that unit's
// slice.
static std::atomic<bool> g_vm_slice_live{false};
// KH_SHOW_INFRONT: the verdict above is a measurement - true when the detector ran through the whole cycle that
// last ended (armed at its start, g_vm_watch_cycle). A cycle no detector watched says nothing about first person.
static std::atomic<bool> g_vm_slice_known{false};
// KH_VOL_MIRROR - the three PODs the mesh fill reads (mirMeta).
static uint32_t g_vmir_w = 0;
static uint32_t g_vmir_h = 0;   // Mask dims (engine vol DSV size).
static float    g_vmir_mask_time = -1.0f;   // Effect time of the last prepass.
}