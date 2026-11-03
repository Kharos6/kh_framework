#pragma once

// The shared head (rendering_integration.hpp): the view-model slice and mirror flags both halves read. Never include it
// on its own.
namespace RenderIntegration {

// The view-model mirror (VMIR): its size and mask time, its targets, the b2 patch, the prepass and the bracket's
// saved state.
// Owner: the render thread (and a park's flush on the game thread, the render thread stalled).
// Session reset: session_globals_reset resets 31 of them. session_scratch_reset resets 1 of them.
struct VmirState {
    ID3D11VertexShader* vs = nullptr;
    // VOL_MIRROR - the three PODs the mesh fill reads (mirMeta).
    uint32_t w = 0;
    uint32_t h = 0;   // Mask dims (engine vol DSV size).
    float    mask_time = -1.0f;   // Effect time of the last prepass.
    ID3D11Texture2D*          tex = nullptr;
    ID3D11DepthStencilView*   dsv = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11Buffer* b2_src = nullptr;   // 64B raw SRV: engine b2 copy.
    ID3D11ShaderResourceView* b2_src_srv = nullptr;
    // MIR_DRAW: the patch's draw target, four R32G32B32A32_FLOAT elements (one
    // per matrix row), copied whole into g_vmir.b2_patch.
    ID3D11Buffer* b2_out = nullptr;
    ID3D11RenderTargetView* b2_out_rtv = nullptr;
    ID3D11Buffer* b2_patch = nullptr;   // 64B CB: what the re-issue binds.
    ID3D11VertexShader* b2_vs = nullptr;   // VSMirB2 (vmir_cs.hlsl).
    ID3D11PixelShader*  b2_ps = nullptr;   // PSMirB2.
    bool     b2_failed = false;   // Compile once; a failure stands the pass down.
    bool     pending = false;   // Set per counting draw, consumed by the hook.
    bool     frame_ok = false;   // This frame's patch + prepass both landed.
    uint32_t mark2 = 0;
    uint32_t prepass_stamp = 0;   // Frame seq of the last prepass.
    // Depth clamp closes the near-plane hole: with DepthClipEnable false the sub-near cap fragments clamp to z = 0,
    // fail the GREATER_EQUAL test and the balance is exact - the textbook z-fail companion. VMIR_RS_KEY: each key
    // holds a reference (AddRef at caching, Release at eviction and in vmir_release), as rp_rs_mir's does, so the
    // engine's state cannot die and a new state of another desc take its address while the clone is cached. Null = the
    // default state (no reference).
    ID3D11RasterizerState* rs_src[2] = {};   // Engine RS the clone shadows.
    ID3D11RasterizerState* rs_clamp[2] = {};   // Same desc, DepthClipEnable false.
    ID3D11RasterizerState* sv_rs = nullptr;
    // The secondary session (a volume source other than g_vmir.prepass_src) then counts for the engine exactly as it
    // would with no mirror pass, and never touches the mirror.
    ID3D11Texture2D* prepass_src = nullptr;   // Weak identity, compare only.
    // Engine dss / shaders / IA are never touched - the stencil arithmetic stays the engine's own; the rasterizer is
    // swapped for a DepthClipEnable = FALSE clone of the engine's own (vmir_begin) and handed back at the end.
    ID3D11RenderTargetView* sv_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView* sv_dsv = nullptr;
    D3D11_VIEWPORT sv_vps[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    UINT sv_nvp = 0;
    ID3D11Buffer* sv_b2 = nullptr;
    UINT sv_b2_first = 0;
    UINT sv_b2_num = 0;
    bool sv_b2_off_ok = false;
    uint64_t want_cycle = ~0ull;   // The last cycle that wanted the mirror (~0 = none).
};
static VmirState g_vmir;

// The one rule for whether an object takes part in shadowing (shadow_part: the seam footprints, which are also
// our pixel-ownership lists) and whether it casts (shadow_active: the sun / DLS maps, the world cast, caster
// demand). An unlit mesh neither receives nor casts (effect meshes are unlit by construction). casterOnly admits an
// invisible object as a caster; SHADOW_SWITCH: castShadow false makes it no caster at all, casterOnly or not, and
// so does every shadow off (SHADOW_OFF): cast_on. Every caster gather and demand census reads these -
// cast_on where it has taken the part test already - never the fields directly.
// INFRONT: whether the cycle that last ended drew the engine's view-model slice (a hands prepass or colour pass
// was seen). Written by the render thread at the main depth clear (infront_frame_reset), read by every caster
// gather and demand census on either thread; atomic because the flush's pre-park census reads it outside the park.
// An inFront mesh casts only while this holds - in third person there is no slice, so no shadow from a mesh that is
// not drawn.
// Measured on foot with a weapon only; any other view counts by whether the game draws the slice in it - expected (not
// measured): none in third person, Zeus or a scripted camera; a first-person view of another unit draws that unit's
// slice.
static std::atomic<bool> g_vm_slice_live{false};
// SHOW_INFRONT: the verdict above is a measurement - true when the detector ran through the whole cycle that
// last ended (armed at its start, g_infront.watch_cycle). A cycle no detector watched says nothing about first person.
static std::atomic<bool> g_vm_slice_known{false};
}