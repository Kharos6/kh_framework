#pragma once

// The shared head (rendering_integration.hpp): the reorder hook's state (ReorderState g_ro, InjectionFlagScope,
// g_orig_draw). Never include it on its own.
namespace RenderIntegration {
typedef void (STDMETHODCALLTYPE* FnDraw)(ID3D11DeviceContext*, UINT, UINT);
static FnDraw                    g_orig_draw = nullptr;

struct ReorderState {
    bool injected = false;   // This scene frame already got its injection.
    bool in_injection = false;   // Our own draws in flight: bypass tracking.
    bool blend_translucent = false;   // Current blend state alpha-blends.
    bool dss_nowrite = false;   // Current dss tests but does not write depth.
    bool dss_write = true;   // Current dss tests and really writes depth (DepthEnable, mask ALL).
    bool dss_stencil = false;   // Current dss tests stencil (StencilEnable).
    bool dsv_main = false;   // The main scene depth is bound.
    uint32_t opaque_draws = 0;   // Opaque main-scene draws since the last main depth clear.
    uint32_t inject_attempts = 0;   // Injection attempts this scene frame (retry bound).
    float cycle_vp_min = 0.0f;
    float cycle_vp_max = 1.0f;
    bool  cycle_vp_valid = false;
    float trig_vp_min = 0.0f;
    float trig_vp_max = 1.0f;
    bool  trig_vp_valid = false;
    RVExtBridge::ProjectionViewTransform cycle_pv = {};
    bool  cycle_pv_valid = false;
    bool  cycle_pv_stale = false;   // This cycle's fetch failed; pv is the kept.
    float engine_m22 = 0.0f;
    float engine_m32 = 0.0f;
    bool  engine_proj_valid = false;
    // Read-only: it never touches the lock, adoption, or re-arm machinery.
    float slot_near_live = -1.0f;
    float slot_m22 = 0.0f;
    float slot_m32 = 0.0f;
    float world_m22 = 0.0f;
    float world_m32 = 0.0f;
    bool  world_pair_valid = false;
    uint64_t frame_cycles = 0;   // Main depth-clear cycles since the session reset (CAST_FRAME64: 64-bit - at 16
                                 // bits it wrapped every ~4.5 min at 240 fps, and the cast latch compares it with !=).
    uint32_t opaques_since_inject = 0;
    // Set when an anomalous (slot-silent) cycle is seen at an injection attempt; consumed by the flush, which then
    // also draws the meshes late that frame - identical overdraw if the injection survived, fill-in if a foreign
    // pass erased it.
    bool anomaly_seen = false;
};
static ReorderState g_ro;
// Restores g_ro.in_injection on every exit, exception included. Its users: the parked flush (park_run - its
// save/restore pairs are skipped by a throw; game thread, inside the park's lock scope), skin_so_batch and
// uvs_step (the render thread's steps - and, for uvs_step, the flush's thread at uvs_upload's end - which
// declare it ahead of their StateBackup so that it unwinds after the restore), flush_ui_frame's lock path
// (PARK_INJ, as the parked flush), the seam footprint's step (volume_seam_inject) and the volume replay's
// (rp_replay, RP_PRESYNC), the mirror patch's fast-path upload (vmir_patch_b2, VMIR_FAST_OWN), the
// mirror re-issue's two OM binds (vmir_begin / vmir_end, VMIR_OM_OWN), VM_SEE's snapshot
// (vmsee_snapshot, ahead of its StateBackup) and transmittance re-issue's OM binds (vmsee_t_begin /
// vmsee_t_end), STEP_OWN_FLAG's steps: the cast fire's and its two repicks (mask_cast_engine) and the
// skin-cull catch-up (skin_cull_catchup), and WSEE's capture, B2 moment and D3 resolve (wsee_snapshot,
// wsee_b2_take, wsee_d3m_resolve, ahead of their saves), its re-issues' binds (wsee_re_set,
// wsee_re_restore) and the one pass (psb_post), and the in-front slice's injection (infront_inject,
// EA_VM_FLAG: its deferred tail runs on either thread).
struct InjectionFlagScope {
    bool prev;
    InjectionFlagScope() : prev(g_ro.in_injection) {}
    ~InjectionFlagScope() { g_ro.in_injection = prev; }
    InjectionFlagScope(const InjectionFlagScope&) = delete;
    InjectionFlagScope& operator=(const InjectionFlagScope&) = delete;
};
}