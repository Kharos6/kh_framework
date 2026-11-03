#pragma once

// The shared head (rendering_integration.hpp): the device resources (struct Resources, g_res), the declarations
// Resources::release calls, and two declarations for parts that call them before their definitions
// (session_teardown, reorder_on_render_thread). Never include it on its own.
namespace RenderIntegration {

// SUN_LADDER: the sun ladder's validity flags, tier keys and input hash describe maps and pyramids Resources owns,
// so Resources::release drops them itself (ensure_resources releases on its own failure paths and does not call
// release_shadow_device_state). Declared here, ahead of Resources, because release() calls it from an inline member
// body. Defined in rendering/sun_shadow_maps.hpp; its other callers are render_sun_depth (an empty caster list, a
// failed ensure; rendering/sun_shadow_render.hpp) and release_shadow_device_state (rendering/shadow_scan.hpp).
inline void sun_ladder_forget();

// Forward: THM_DEVRESET - the heightfield's live flags, dropped by Resources::release (defined after
// thm_upload in rendering/terrain_heightmap.hpp).
inline void thm_device_lost();
// Forward: PSV - the reduced particle shaders' variants and our builtins' blobs, dropped by Resources::release
// (defined with the capture in rendering/see_through_shaders.hpp).
inline void psv_device_lost();
inline void session_teardown(bool clean);   // SESSION_RESET: both mission-edge teardowns (defined with the reset).

// Forward: the render-thread identity check lives with the reorder state in rendering/hook_table.hpp.
inline bool reorder_on_render_thread();

// A sound key: depth_srv holds a reference on the texture, so its address cannot be reused while the SRV lives; nulled
// wherever depth_srv is released. Written by ensure_depth_srv from one thread at a time (a park's flush, the render
// thread, or a chain-only flush inside a game-thread resolve).
static void* g_eds_memo_res = nullptr;
static UINT g_eds_memo_w = 0;   // The keyed texture's size (the fast path's answer).
static UINT g_eds_memo_h = 0;

struct Resources {
    ID3D11VertexShader*      vs = nullptr;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11VertexShader*      vs_fullscreen = nullptr;   // SV_VertexID fullscreen triangle.
    ID3D11PixelShader*       ps_effect = nullptr;   // Uber effect shader (meshes + fullscreen).
    // FX_LEAN_PASS: PSEffect built with KH_FX_LEAN 1 (cb.hlsl's note: KhWseeArmed() false, so none of the
    // see-through record's code or registers is compiled in), made with ps_effect at its sample count (non-fatal). A
    // draw whose constants leave the pass unarmed (fx_tex[1][2] <= 0.5: the lane the shader's arm reads, so the full
    // build would run none of that code either) takes it - the scene and UI chains' builtin passes. Absent, ps_effect.
    ID3D11PixelShader*       ps_effect_lean = nullptr;
    UINT                     ps_effect_samples = 0;   // Depth MSAA count it was compiled for.
    // FX_SPLIT / FX_LINZ (effect.hlsl's tail): the effect unit's chain-side entry points - the linear-depth
    // targets' builder, the SSGI gather and a-trous, dynamicLightFog's gather and filter - made with ps_effect at its
    // sample count (ensure_effect_shader); each consumer gates on its pointer.
    ID3D11PixelShader*       ps_fx_linz = nullptr;
    ID3D11PixelShader*       ps_ssgi_gather = nullptr;
    ID3D11PixelShader*       ps_ssgi_atrous = nullptr;
    ID3D11PixelShader*       ps_dlf_gather = nullptr;
    ID3D11PixelShader*       ps_dlf_filter = nullptr;
    // FS_SPLIT (effect.hlsl's PSFsSplitS): fog scatter's own entry for the scene chain where its side draw (28)
    // is armed, made with ps_effect likewise; absent, the pass draws ps_effect's branch.
    ID3D11PixelShader*       ps_fs_split = nullptr;
    // WSEE_EDGE_COST (effect.hlsl's PSFsSide): fog scatter's side value (28) as its own entry, made with ps_effect
    // likewise; absent, fx_side_draw draws it with ps_effect's branch.
    ID3D11PixelShader*       ps_fs_side = nullptr;
    // KH_FX_LEAN (cb.hlsl's note): PSFsSplitS and effect.hlsl's PSDlfComp built with KH_FX_LEAN 1 - no record
    // maths -, for an unlocalized, unbanded fog scatter / dynamicLightFog composite in the scene chain; made with
    // ps_effect likewise; absent, the full build (ps_fs_split) or ps_effect's branch.
    ID3D11PixelShader*       ps_fs_split_lean = nullptr;
    ID3D11PixelShader*       ps_dlf_comp = nullptr;
    // DLF_SPLIT (effect.hlsl's PSDlfClass note): dynamicLightFog's gather built with KH_FX_LEAN 1 (no record code)
    // and the entry that marks the grid texels a layer may cover, made with ps_effect likewise. With both and the
    // stencil (dlf_st_*: D24_UNORM_S8_UINT at the grid target's size, made at first use, remade on a size change,
    // released with the grid targets - release_fx_chain) an armed pass's gather draws twice under it (dss_dlf_mark
    // marks 1, dss_dlf_sel tests EQUAL: ref 0 the lean build, 1 the full one); an unarmed pass draws the lean build
    // alone. Without the lean build, or armed without the marker (made with the system built in alone) or the
    // stencil, the full build alone. A refused stencil latches at its size (dlf_st_fail_*; another size tries again);
    // the latch and the states go with the device.
    ID3D11PixelShader*       ps_dlf_gather_lean = nullptr;
    ID3D11PixelShader*       ps_dlf_class = nullptr;
    ID3D11Texture2D*         dlf_st_tex = nullptr;
    ID3D11DepthStencilView*  dlf_st_dsv = nullptr;
    UINT                     dlf_st_w = 0, dlf_st_h = 0;
    UINT                     dlf_st_fail_w = 0, dlf_st_fail_h = 0;   // The refused size (0 x 0: none).
    ID3D11DepthStencilState* dss_dlf_mark = nullptr;
    ID3D11DepthStencilState* dss_dlf_sel = nullptr;
    ID3D11VertexShader*      vs_composite = nullptr;   // injected-path VS (adds world position).
    ID3D11PixelShader*       ps_maskcast = nullptr;   // Analytic mask cast (single-sample t0).
    ID3D11PixelShader*       ps_rpmerge = nullptr;    // VOL_REPLAY: the replayed count and footprint, merged.
    ID3D11PixelShader*       ps_rpmergemir = nullptr;   // MIR_REPLAY: the replayed mirror count, merged.
    ID3D11PixelShader*       ps_dls_world = nullptr;   // DLS_WORLD: dynamic-light world receive
                                                       // (single-sample t0).
    ID3D11PixelShader*       ps_dls_world_fog = nullptr;   // DLSW_FOG: its dual-source twin.
    ID3D11PixelShader*       ps_seam_foot = nullptr;   // VOL_FOOT: the footprint's distance.
    ID3D11PixelShader*       ps_maskprime = nullptr;   // Writes lit over our footprint.
    ID3D11PixelShader*       ps_inj_depth_a = nullptr;   // FOOTPRINT_ALPHA: the clip-only alpha
                                                         // footprint (KH_TEXTURED).
    ID3D11PixelShader*       ps_composite = nullptr;   // PSMain + the opaque.
    ID3D11PixelShader*       ps_composite_arb = nullptr;   // Arb variant: + SV_Depth analytic
                                                           // terrain clamp.
    UINT                     ps_composite_samples = 0;   // comp_depth_samples it was compiled for (always 1; 0 = none).
    ID3D11Texture2D*          comp_depth_tex = nullptr;   // Owned mid-frame copy of the scene
                                                          // depth.
    ID3D11ShaderResourceView* comp_depth_srv = nullptr;   // The live depth is bound as a writable DSV during
                                                          // injection: sampling it directly is a bind hazard.
    UINT                     comp_depth_w = 0, comp_depth_h = 0, comp_depth_samples = 1;
    float                    comp_depth_time = -1.0f;   // Last snapshot copy (overlay freshness
                                                        // gate).
    ID3D11RenderTargetView*   snap_rtv = nullptr;   // RTV over comp_depth_tex.
    ID3D11PixelShader*        ps_depth_resolve = nullptr;
    UINT                      ps_resolve_samples = 0;   // Source MSAA count compiled for.
    // Analytic terrain heightfield (t10; see thmParams).
    ID3D11Texture2D*          thm_tex = nullptr;
    ID3D11ShaderResourceView* thm_srv = nullptr;
    ID3D11InputLayout*       input_layout = nullptr;
    // Deliberately its own shaders, layout, buffers and states: everything else ensure_resources builds sits below the
    // async gate the placeholder exists to escape.
    ID3D11VertexShader*      vs_white = nullptr;
    ID3D11PixelShader*       ps_white = nullptr;
    ID3D11InputLayout*       layout_white = nullptr;
    ID3D11Buffer*            white_obj_cb = nullptr;   // b0, CBObj slice.
    ID3D11Buffer*            white_frame_cb = nullptr;   // b1, CBFrame slice.
    // Same bias, same DepthClipEnable false, so the placeholder sits where the real mesh will.
    ID3D11RasterizerState*   rast_white = nullptr;
    ID3D11DepthStencilState* dss_white = nullptr;   // LESS_EQUAL, write all.
    bool                     white_tried = false;   // One compile attempt per device.
    bool                     white_ready = false;
    ID3D11RasterizerState*   rasterizer_cull = nullptr;   // Back-face-culling twin of 'rasterizer' (twoSided = false
                                                          // objects; same biases).
    // Front-culling twin: pass 1 of the two-sided-translucent ordered draw. Below: the KH_TEXTURED twins, the same
    // sources compiled with KH_TEXTURED=1.
    ID3D11RasterizerState*   rasterizer_front = nullptr;
    ID3D11VertexShader*      vs_tex = nullptr;   // VSMain twin (flush).
    ID3D11PixelShader*       ps_tex = nullptr;   // PSMain twin.
    ID3D11VertexShader*      vs_comp_tex = nullptr;   // VSComposite twin (injection).
    ID3D11PixelShader*       ps_comp_tex = nullptr;   // PSComposite twin.
    ID3D11PixelShader*       ps_comp_arb_tex = nullptr;   // Arb twin.
    ID3D11InputLayout*       layout_tex = nullptr;
    ID3D11VertexShader*      vs_inst = nullptr;
    ID3D11VertexShader*      vs_inst_tex = nullptr;
    ID3D11VertexShader*      vs_comp_inst = nullptr;
    ID3D11VertexShader*      vs_comp_inst_tex = nullptr;
    ID3D11InputLayout*       layout_inst = nullptr;
    ID3D11InputLayout*       layout_inst_tex = nullptr;
    // SKIN_POOL: the vertex-pulling twins (static.hlsl / composite.hlsl *Sk) - the colour buckets' four (the
    // composite pair is the composite unit's) and the depth runs' three (an alpha caster draws one instance at a time,
    // through the IA route) - and their one layout (VSInstSk: the lane alone, on slot 1). Non-fatal: the colour
    // buckets take their four whole (inst_sk_on), a depth route its own twin (cast_pool_of).
    ID3D11VertexShader*      vs_inst_sk = nullptr;
    ID3D11VertexShader*      vs_inst_sk_tex = nullptr;
    ID3D11VertexShader*      vs_comp_inst_sk = nullptr;
    ID3D11VertexShader*      vs_comp_inst_sk_tex = nullptr;
    ID3D11VertexShader*      vs_sundepth_sk = nullptr;
    ID3D11VertexShader*      vs_seam_inst_sk = nullptr;
    ID3D11VertexShader*      vs_mirror_inst_sk = nullptr;
    ID3D11InputLayout*       layout_inst_sk = nullptr;
    ID3D11Buffer*            inst_vb[2] = {};
    uint32_t                 inst_cursor[2] = {};
    bool                     inst_fresh[2] = { true, true };   // Next map must discard.
    ID3D11SamplerState*      mat_sampler = nullptr;
    ID3D11Buffer*             mat_sb = nullptr;   // MAT_TABLE: StructuredBuffer<KhGpuMat>, t38.
    ID3D11ShaderResourceView* mat_srv = nullptr;
    uint32_t                  mat_cap = 0;   // Entries the buffer holds.
    // OBJBUF: the persistent object record buffer (StructuredBuffer<KhObjRec>, VS t39), one record per live-scene
    // slot, DEFAULT usage, delta-updated by objbuf_sync from the CPU mirror (UpdateSubresource per dirty run).
    ID3D11Buffer*             obj_sb = nullptr;
    ID3D11ShaderResourceView* obj_srv = nullptr;
    uint32_t                  obj_cap = 0;   // Records the buffer holds.
    // OBJBUF_RETRY: objbuf_sync calls still to refuse at once after the device refused the buffer (0 = ask);
    // a new device (release) asks at once.
    uint32_t                  obj_wait = 0;
    // Release wall: a freed mesh's VB/IB/def wait two flushes so per-frame caster lists built before the release drain
    // first. Swept by mesh_grave_sweep.
    struct MeshGrave { ID3D11Buffer* vb; ID3D11Buffer* ib; MeshDef* def; uint64_t flush_at; };
    std::vector<MeshGrave>   mesh_grave;
    std::vector<ID3D11Buffer*> mesh_ib;   // MESH_INDEXED: the R32_UINT index buffer twin of
                                          // mesh_vb, same id, same lifetime (nullptr = tombstone).
    std::vector<ID3D11Buffer*> mesh_vb;   // One VB per published registry mesh (48-byte textured vertex format;
                                          // grows in step via ensure_mesh_vbs - the dynamic-registration contract).
    ID3D11Buffer*            constant_buffer = nullptr;   // Dynamic, per draw (the scene and UI flushes):
                                                          // CBObj b0.
    ID3D11Buffer*            composite_cb = nullptr;   // Dynamic, per draw (render-thread injection): CBObj b0.
    // CB_RING: the per-object slices of the two buffers above ride a large dynamic ring each - WRITE_DISCARD once
    // per wrap, WRITE_NO_OVERWRITE per slice, bound with the D3D11.1 offset setters.
    ID3D11Buffer*            cb_ring[2] = {};   // [0] = constant_buffer's ring, [1] =
                                                // composite_cb's.
    uint32_t                 cb_ring_cursor[2] = {};
    bool                     cb_ring_fresh[2] = { true, true };   // Next map must discard.
    // CB-split frame twins (b1): the flushes' and the injection's, the same split as the pair above.
    ID3D11Buffer*            frame_cb = nullptr;   // The scene flush + the UI flush.
    ID3D11Buffer*            composite_frame_cb = nullptr;   // Render thread (injection, sun depth,
                                                             // mask cast).
    ID3D11DepthStencilState* dss_test = nullptr;   // LESS_EQUAL, no write.
    ID3D11DepthStencilState* dss_test_write = nullptr;   // LESS_EQUAL, write.
    // SSAO_MARK: dss_test_write plus stencil = ref (1) where the depth test passes (our pixels, for the AO pass);
    // the eraser (depth off, stencil EQUAL -> ZERO); the apply's mask (depth off, stencil EQUAL, no writes; null = no
    // depth view).
    ID3D11DepthStencilState* dss_test_write_mark = nullptr;
    ID3D11DepthStencilState* dss_mark_clear = nullptr;
    ID3D11DepthStencilState* dss_mark_test = nullptr;
    ID3D11DepthStencilState* dss_off = nullptr;
    ID3D11BlendState*        blend_modes[6] = {};   // Normal, additive, multiply, screen, lighten, darken.
    ID3D11BlendState*        dls_world_blend = nullptr;
    ID3D11BlendState*        dls_world_fog_blend = nullptr;   // DLSW_FOG: src0 + dest * src1.
    // World-pass occlusion mask: R32_FLOAT linear view distance (m) of our nearest mesh per pixel; min_blend resolves
    // overlaps, no depth target.
    ID3D11Texture2D*          dlsw_tex = nullptr;
    ID3D11RenderTargetView*   dlsw_rtv = nullptr;
    ID3D11ShaderResourceView* dlsw_srv = nullptr;
    UINT                      dlsw_w = 0, dlsw_h = 0;
    ID3D11BlendState*         dlsw_min_blend = nullptr;   // Dest = min(src, dest).
    ID3D11VertexShader*       vs_dls_mask = nullptr;
    ID3D11PixelShader*        ps_dls_mask = nullptr;
    ID3D11PixelShader*        ps_dls_mask_a = nullptr;   // DLSW_MASK_ALPHA: the alpha-clipping twin.
                                                         // Also VOL_FOOT's alpha casters (the seam).
    // SSAO: the four pass shaders (at the live depth's sample count), the half grid's sample-0 depth in metres
    // (R32_FLOAT), the term and its ping-pong twin (R8_UNORM), and the pass's 80-byte b0 (SsaoCb).
    ID3D11PixelShader*        ps_ssao_depth = nullptr;
    ID3D11PixelShader*        ps_ssao_main = nullptr;
    ID3D11PixelShader*        ps_ssao_blur = nullptr;
    ID3D11PixelShader*        ps_ssao_apply = nullptr;
    UINT                      ps_ssao_samples = 0;
    ID3D11Texture2D*          ssao_dh_tex = nullptr;
    ID3D11RenderTargetView*   ssao_dh_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_dh_srv = nullptr;
    ID3D11Texture2D*          ssao_ao_tex = nullptr;
    ID3D11RenderTargetView*   ssao_ao_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_ao_srv = nullptr;
    ID3D11Texture2D*          ssao_ao2_tex = nullptr;
    ID3D11RenderTargetView*   ssao_ao2_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_ao2_srv = nullptr;
    UINT                      ssao_w = 0, ssao_h = 0;     // The main depth's size the targets were made for.
    UINT                      ssao_hw = 0, ssao_hh = 0;   // The half grid's.
    ID3D11Buffer*             ssao_cb = nullptr;
    // NEARZ_MARK: the near-plane marker (R32G32_FLOAT, the main depth's size: our near-z pixels' raw depth and
    // true distance), made on first need; its pass shader (ssao.hlsl).
    ID3D11PixelShader*        ps_ssao_nzmark = nullptr;
    ID3D11Texture2D*          nzm_tex = nullptr;
    ID3D11RenderTargetView*   nzm_rtv = nullptr;
    ID3D11ShaderResourceView* nzm_srv = nullptr;
    UINT                      nzm_w = 0, nzm_h = 0;
    // VM_SEE: the scene's depth behind the view model's translucent planes (R32_FLOAT: sample 0 of the main depth,
    // raw, at the cycle's first view-model translucent draw) and their transmittance (R8_UNORM: cleared to 1 there,
    // times 1 - alpha per view-model draw that blends over with it), both the main depth's size, made by the first
    // snapshot that needs them (vmsee_targets) and released when the main depth goes to one sample
    // (msaa_toggle_wipe: no late chain reads them there); the snapshot's pass shader (depth_resolve.hlsl's
    // PSDepthS0, at the main depth's sample count) and the transmittance's blend (ZERO / INV_SRC_ALPHA, red only),
    // made at the arm (vmsee_ensure). A refused make is not retried for the same size (targets) or count (shader)
    // until the device goes.
    ID3D11Texture2D*          vmsee_s_tex = nullptr;
    ID3D11RenderTargetView*   vmsee_s_rtv = nullptr;
    ID3D11ShaderResourceView* vmsee_s_srv = nullptr;
    ID3D11Texture2D*          vmsee_t_tex = nullptr;
    ID3D11RenderTargetView*   vmsee_t_rtv = nullptr;
    ID3D11ShaderResourceView* vmsee_t_srv = nullptr;
    UINT                      vmsee_w = 0, vmsee_h = 0;
    UINT                      vmsee_fail_w = 0, vmsee_fail_h = 0;
    ID3D11PixelShader*        ps_vmsee_s0 = nullptr;
    UINT                      ps_vmsee_samples = 0;
    UINT                      ps_vmsee_fail = 0;
    ID3D11BlendState*         bs_vmsee_t = nullptr;
    bool                      bs_vmsee_failed = false;
    // WSEE: the capture's targets (wsee_snapshot; all the main depth's size) - S (R32_FLOAT, single sample:
    // sample 0 of the main depth, raw), T (R8G8B8A8_UNORM at the main depth's sample count and quality: bound beside
    // depth_dsv_ro for the re-issues, so it must match that view's texture; cleared to (1, 0, 1, 1): R the fence-type
    // transmittance, G = 1 where a late opaque's re-issue drew, B / A the particle-type layers' before / after B2),
    // B (the scene target's view format, single sample: its resolve) and NW (wsee_n_*: R32_TYPELESS single sample, a
    // D32_FLOAT DSV and an R32_FLOAT SRV; cleared to 1: the nearest depth of the blended draws that write none). Made
    // by the first capture that needs them (wsee_targets, VMSEE_LAZY's rule), remade at a size, sample-count,
    // quality or format change. The chain's own: E (wsee_e_*: R32_UINT at the chain's frame size, the class word,
    // wsee_agree_draw) and C (wsee_c_srv: a Texture2DMS view of the scene target the chain captures from, in its
    // RTV's format, keyed by that texture and format - it holds a reference to the engine's texture until a rekey or
    // a release; wsee_scene_ms). All released when the main depth goes to one sample (msaa_toggle_wipe: 1x
    // captures nothing; WSEE_MSAA_FREE: at a change to another count the capture's targets and C alone, E staying),
    // after WSEE_IDLE_FREE cycles without a want (wsee_cycle_note) and with the device; C also at the
    // swapchain's resize (release_wsee_c). wsee_depth_id: the depth texture they were last captured against (an
    // identity, never dereferenced: wsee_valid's test against depth_res_identity). The capture's pass shader and
    // the blended re-issue's blend are VM_SEE's (ps_vmsee_s0, bs_vmsee_t), its depth state dss_test; the record's
    // own (wsee_ensure, kept until the device goes): ps_wsee_one (depth_resolve.hlsl PSWseeOne, the late-opaque
    // re-issue's store path), ps_wsee_copy (an inline copy, the chain's final write when its last pass read C),
    // dss_wsee_nw (LESS, write, no stencil: NW) and dss_wsee_eq (EQUAL, no write, no stencil: the late-opaque
    // re-issue); ps_wsee_agree (effect.hlsl PSWseeAgree) is the effect unit's (ensure_effect_shader: made and
    // released with it). WSEE_FAIL_RETRY: a refused make latches its key (wsee_fail_*) and cycle (wsee_fail_cycle);
    // the key is asked again WSEE_HOLD_FAIL_RETRY cycles later (a transient refusal - memory the engine freed
    // since - would otherwise hold the record off until the device goes); a refusal stamps it again, a success clears
    // the latch. The same for the split's (wsee_ld_fail_*), the one pass's (wsee_pb_fail_*) and D3m's
    // (wsee_d3m_fail_*) targets.
    ID3D11Texture2D*          wsee_s_tex = nullptr;
    ID3D11RenderTargetView*   wsee_s_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_s_srv = nullptr;
    ID3D11Texture2D*          wsee_t_tex = nullptr;
    ID3D11RenderTargetView*   wsee_t_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_t_srv = nullptr;
    ID3D11Texture2D*          wsee_b_tex = nullptr;
    ID3D11ShaderResourceView* wsee_b_srv = nullptr;
    UINT                      wsee_w = 0, wsee_h = 0, wsee_samples = 0, wsee_quality = 0;
    DXGI_FORMAT               wsee_b_fmt = DXGI_FORMAT_UNKNOWN;
    void*                     wsee_depth_id = nullptr;
    // The engine's depth view last found to be wsee_depth_id's texture (wsee_dsv_is_depth compares pointers
    // first), held by one reference so its address is no other view's while kept, with the identity it matched
    // (wsee_dsv_pin_id; a hit needs it to be wsee_depth_id's still). Released with the capture's targets, where the
    // capture takes another depth (wsee_snapshot) and at a clear with the identity moved or the capture unwanted
    // (wsee_cycle_note) - the one reference also keeps the engine's depth texture until then.
    ID3D11DepthStencilView*   wsee_dsv_pin = nullptr;
    void*                     wsee_dsv_pin_id = nullptr;
    UINT                      wsee_fail_w = 0, wsee_fail_h = 0, wsee_fail_s = 0, wsee_fail_q = 0;
    DXGI_FORMAT               wsee_fail_f = DXGI_FORMAT_UNKNOWN;
    uint64_t                  wsee_fail_cycle = 0u;   // WSEE_FAIL_RETRY.
    ID3D11Texture2D*          wsee_n_tex = nullptr;   // NW.
    ID3D11DepthStencilView*   wsee_n_dsv = nullptr;
    ID3D11ShaderResourceView* wsee_n_srv = nullptr;
    // B2 (wsee_b2_*: B's desc - the scene resolved at the cycle's first fence-type layer, wsee_b2_take; only in a
    // cycle that resolved B) and NWpost (wsee_n2_*: NW's desc - the nearest particle-type layer drawn after B2;
    // wsee_n_* is NWpre). Made and released with S, T, B and NW (wsee_targets, release_wsee_capture).
    ID3D11Texture2D*          wsee_b2_tex = nullptr;
    ID3D11ShaderResourceView* wsee_b2_srv = nullptr;
    ID3D11Texture2D*          wsee_n2_tex = nullptr;
    ID3D11DepthStencilView*   wsee_n2_dsv = nullptr;
    ID3D11ShaderResourceView* wsee_n2_srv = nullptr;
    // S2 - the main depth's sample 0, raw, at B2's moment (S's desc; wsee_b2_take): the depth the late opaques
    // drawn before B2 left there. Made and released with S, T, B and NW.
    ID3D11Texture2D*          wsee_s2_tex = nullptr;
    ID3D11RenderTargetView*   wsee_s2_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_s2_srv = nullptr;
    // D3 - the main depth's sample 0 raw at the last time a late opaque owned sample 0 (S's desc with a UAV and, from
    // wsee_d3m_targets, an RTV: the store path's replay stores it once a pixel from the main depth's view - its
    // sample-0 invocation, depth_resolve.hlsl PSWseeOne / PSWseeOneL -, the D3m path's resolve writes it). Made and
    // released with S, T, B and NW (wsee_targets, all or nothing; release_wsee_capture); 33 MB at 3840x2160.
    ID3D11Texture2D*           wsee_d3_tex = nullptr;
    ID3D11UnorderedAccessView* wsee_d3_uav = nullptr;
    ID3D11ShaderResourceView*  wsee_d3_srv = nullptr;
    // WSEE_OQ2: D3m - R32_FLOAT at T's sample count and quality (the post late opaques' replays write their own
    // depth there as a third target: depth_resolve.hlsl PSWseeOneZ / PSWseeOneZL; 265 MB at 3840x2160 8x) - and D3's
    // RTV (wsee_d3m_resolve writes D3 from D3m before a reader binds it). Made by the capture
    // (wsee_d3m_targets) where the main depth's view is 24- or 32-bit, at T's key plus that view's format; released
    // with the capture's targets; a refused make latches its key and cycle (wsee_d3m_fail_*, WSEE_FAIL_RETRY).
    // Absent, every post replay takes the store path (the UAV and the main depth's view). wsee_rs_pin: the engine's
    // rasterizer state last asked whether it biases or clamps depth (wsee_d3m_path), held by one reference so its
    // address is no other state's while kept, with the verdict; released with the pinned depth view.
    ID3D11RenderTargetView*   wsee_d3_rtv = nullptr;
    ID3D11Texture2D*          wsee_d3m_tex = nullptr;
    ID3D11RenderTargetView*   wsee_d3m_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_d3m_srv = nullptr;
    UINT                      wsee_d3m_w = 0, wsee_d3m_h = 0, wsee_d3m_s = 0, wsee_d3m_q = 0;
    UINT                      wsee_d3m_fail_w = 0, wsee_d3m_fail_h = 0, wsee_d3m_fail_s = 0, wsee_d3m_fail_q = 0;
    DXGI_FORMAT               wsee_d3m_dfmt = DXGI_FORMAT_UNKNOWN;    // The main depth's view format it was made at,
    DXGI_FORMAT               wsee_d3m_fail_f = DXGI_FORMAT_UNKNOWN;  // and the refused key's (both in the key).
    uint64_t                  wsee_d3m_fail_cycle = 0u;   // WSEE_FAIL_RETRY.
    ID3D11RasterizerState*    wsee_rs_pin = nullptr;
    bool                      wsee_rs_pin_plain = false;
    ID3D11RasterizerState*    wsee_rs_pin_psb = nullptr;   // PSB_RSPIN: psb_rs_ok's own pin, the same rule;
    bool                      wsee_rs_pin_psb_ok = false, wsee_rs_pin_psb_sc = false;   // its two verdicts.
    ID3D11Texture2D*          wsee_e_tex = nullptr;   // E.
    ID3D11RenderTargetView*   wsee_e_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_e_srv = nullptr;
    UINT                      wsee_e_w = 0, wsee_e_h = 0;
    bool                      wsee_e_failed = false;
    UINT                      wsee_e_fail_w = 0, wsee_e_fail_h = 0;   // WSEE_EFAIL_KEY: the refused size.
    // WSEE_EDGE_COST: E is R32_UINT (the class word, cb.hlsl KhWseeClassWord). Beside it a stencil of E's size
    // (wsee_es_*: R24G8_TYPELESS, a D24_UNORM_S8_UINT view), cleared to 0 and marked 1 by E's draw where the word is
    // nonzero (dss_wsee_smark: no depth, stencil ALWAYS / REPLACE), so a frame-size draw with a per-sample site can be
    // drawn twice under dss_wsee_ssel (no depth, stencil EQUAL / KEEP, ref 0 then 1) - the agreeing pixels and the edge
    // pixels in warps of their own (fx_side_draw, fx_chain_run's flush). Made and released with E (release_wsee_chain);
    // the states with the device. A refused make latches wsee_es_failed: E is drawn without the stencil and nothing is
    // split. WSEE_EFAIL_KEY: both refusal latches (E's wsee_e_failed too) hold at the refused size alone (wsee_e_fail_*
    // / wsee_es_fail_*): another size tries again; the latches go with the device.
    ID3D11Texture2D*          wsee_es_tex = nullptr;
    ID3D11DepthStencilView*   wsee_es_dsv = nullptr;
    bool                      wsee_es_failed = false;
    UINT                      wsee_es_fail_w = 0, wsee_es_fail_h = 0;   // WSEE_EFAIL_KEY: the refused size.
    ID3D11DepthStencilState*  dss_wsee_smark = nullptr;
    ID3D11DepthStencilState*  dss_wsee_ssel = nullptr;
    // C, its key (an identity) and its refused key. The refused key holds no reference, so a recycled address could
    // refuse another texture's view until release_wsee_c (a resize, a count change, 1x, the idle free) or the device
    // goes - it needs a CreateShaderResourceView of a valid desc to fail first.
    ID3D11ShaderResourceView* wsee_c_srv = nullptr;
    void*                     wsee_c_res = nullptr;
    DXGI_FORMAT               wsee_c_fmt = DXGI_FORMAT_UNKNOWN;
    UINT                      wsee_c_w = 0, wsee_c_h = 0, wsee_c_s = 0;   // The capture's key it was made against.
    void*                     wsee_c_fail_res = nullptr;
    DXGI_FORMAT               wsee_c_fail_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11PixelShader*        ps_wsee_one = nullptr;
    ID3D11PixelShader*        ps_wsee_copy = nullptr;
    ID3D11PixelShader*        ps_wsee_agree = nullptr;
    ID3D11DepthStencilState*  dss_wsee_nw = nullptr;
    ID3D11DepthStencilState*  dss_wsee_eq = nullptr;
    // The sorted tail's layer-aware composite (dual source: out = o0 + dest x o1, alpha alike); wsee_ensure's,
    // non-fatal (absent, the sorted tail takes the plain blend - mode 1 - and the injection defers nothing).
    ID3D11BlendState*         bs_wsee_sort = nullptr;
    bool                      bs_wsee_sort_failed = false;
    // The particle-type re-issues' T passes - Tpre in T's B (ZERO / INV_SRC_ALPHA, blue only) and Tpost in T's A
    // (the alpha factors ZERO / INV_SRC_ALPHA, alpha only); the fence-type pass keeps bs_vmsee_t (red only). Made by
    // wsee_ensure with the record's depth states (required).
    ID3D11BlendState*         bs_wsee_tpre = nullptr;
    ID3D11BlendState*         bs_wsee_tpost = nullptr;
    // Q - the scene's colour just before the flush's first sorted part (B's desc: the scene target's view format,
    // single sample - its resolve), the composite's divisor (cb.hlsl KhWseeMeshOut); made by wsee_defer_ok whenever
    // the injection weighs a deferral and by the flush's first compositing part (wsee_q_ensure), released with the
    // capture's targets. A refused make latches until the device goes.
    ID3D11Texture2D*          wsee_q_tex = nullptr;
    ID3D11ShaderResourceView* wsee_q_srv = nullptr;
    bool                      wsee_q_failed = false;
    // The geometry hold - one DEFAULT vertex buffer the injection copies the deformed vertex arrays of its deferred
    // parts into (wsee_hold_take; g_wsee.hold's note), wsee_hold_cap bytes; grown by doubling (the old contents
    // copied to the new buffer's start: no entry moves), released with the capture's targets (release_wsee_targets),
    // after WSEE_IDLE_FREE cycles without a copy using it (wsee_cycle_note) and with the device. wsee_hold_gen
    // steps at every make and release (a growth keeps it). WSEE_HOLD_FAIL: a refused CreateBuffer latches its size
    // (wsee_hold_fail_cap) - a growth to that size or past it is refused without the call (wsee_hold_reserve) until
    // the hold is released; 0 = nothing refused. WSEE_HOLD_FAIL_RETRY: the cycle it was refused in
    // (wsee_hold_fail_cycle) - WSEE_HOLD_FAIL_RETRY cycles later one ask at that size is let through again (a
    // transient refusal - memory the engine freed since - would otherwise hold the deferral off until the hold's idle
    // release, which a deferral asked for every cycle never reaches); a refusal stamps it again.
    ID3D11Buffer*             wsee_hold_vb = nullptr;
    UINT                      wsee_hold_cap = 0u;
    UINT                      wsee_hold_fail_cap = 0u;
    uint64_t                  wsee_hold_fail_cycle = 0u;
    uint64_t                  wsee_hold_gen = 0u;
    bool                      wsee_v2_failed = false;     // ps_wsee_one, a depth or a T blend state refused.
    // Tp - R8G8B8A8_UNORM at the main depth's sample count and quality (bound beside depth_dsv_ro, as T), cleared to
    // 1: the particle-type layers' transmittance counting only fragments NOT within KH_WSEE_NEAR_TOL of the surface
    // behind them (R pre, G post; their replay pushed back by the near shift) and the same for the KH_WSEE_NEAR_TOL2
    // band (B / A); 265 MB at 3840x2160 8x. Tf - R8G8_UNORM single sample, cleared to 1: the pre / post particle-type
    // fragments FARTHER than the cycle's distance D (wsee_tfd_*: its own D32 target, cleared to D's raw, tested
    // GREATER without a write). Made by the capture (wsee_ld_targets) at T's key; released with the capture's
    // targets and at the first clear after the split was refused (wsee_ld_failed); a refused make latches its key and
    // cycle (wsee_ld_fail_*, WSEE_FAIL_RETRY) - another key tries at once. The late-opaque replay's MRT twin
    // (depth_resolve.hlsl PSWseeOneL: T and Tp at once) is wsee_ensure's (compiled at the flush, never inside a
    // draw hook); the four band blend states (T's particle factors on red / green, blue / alpha) and the far depth
    // state are wsee_ld_ensure's; a refused shader or state latches the split off until the device goes.
    ID3D11Texture2D*          wsee_tp_tex = nullptr;
    ID3D11RenderTargetView*   wsee_tp_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_tp_srv = nullptr;
    ID3D11Texture2D*          wsee_tf_tex = nullptr;
    ID3D11RenderTargetView*   wsee_tf_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_tf_srv = nullptr;
    ID3D11Texture2D*          wsee_tfd_tex = nullptr;
    ID3D11DepthStencilView*   wsee_tfd_dsv = nullptr;
    UINT                      wsee_ld_w = 0, wsee_ld_h = 0, wsee_ld_s = 0, wsee_ld_q = 0;
    UINT                      wsee_ld_fail_w = 0, wsee_ld_fail_h = 0, wsee_ld_fail_s = 0, wsee_ld_fail_q = 0;
    uint64_t                  wsee_ld_fail_cycle = 0u;   // WSEE_FAIL_RETRY.
    ID3D11PixelShader*        ps_wsee_one_l = nullptr;
    // WSEE_OQ2: the post twins writing D3m (PSWseeOneZ / PSWseeOneZL) and D3's resolve (PSWseeD3Res) -
    // wsee_ensure's; a refusal latches the store path for every post replay until the device goes
    // (ps_wsee_oq2_failed), reported once.
    ID3D11PixelShader*        ps_wsee_one_z = nullptr;
    ID3D11PixelShader*        ps_wsee_one_zl = nullptr;
    ID3D11PixelShader*        ps_wsee_d3_res = nullptr;
    bool                      ps_wsee_oq2_failed = false;
    ID3D11BlendState*         bs_wsee_ldpre = nullptr;
    ID3D11BlendState*         bs_wsee_ldpost = nullptr;
    ID3D11BlendState*         bs_wsee_ldpre2 = nullptr;    // The second band's (Tp's B / A).
    ID3D11BlendState*         bs_wsee_ldpost2 = nullptr;
    ID3D11DepthStencilState*  dss_wsee_far = nullptr;
    bool                      wsee_ld_failed = false;
    // PSB: the one pass's record, a target pair a phase (PSB_PHASE) - P0 / N the pre phase's, P1 / N2 the post
    // one's: P (R8G8B8A8_UNORM) = (T, the 25 % band's, the 100 % band's, Tf's) of its phase, N (R32_FLOAT) its NW -
    // single sample, cleared to 1 by the capture; made there (psb_targets) while a one-pass variant exists,
    // released with the capture's targets (release_wsee_pb); a refused make latches its key and cycle
    // (wsee_pb_fail_*, WSEE_FAIL_RETRY). PSB_LORES: at ceil(size / s) a side, s = 1 << PSB_LS = 2 (33 MB at
    // 3840x2160); the one pass draws into them at that scale (psb_post) and every reader takes the texel px >>
    // PSB_LS (clamped at the frame's edge: PSB_LORES_EDGE). The blend states (psb_ensure): P takes ZERO /
    // INV_SRC_COLOR (the alpha's ZERO / INV_SRC_ALPHA), N MIN; [0] the layer-depth split standing (all four channels),
    // [1] not (T and the 25 % band alone; the other two keep their clear). No N-alone state: a light-adding draw keeps
    // its NW pass. The constant buffers: one per verdict set (pushed | test-off << 1 | Tf drawn << 2), four rows -
    // (m_i, Dq), (a_i, D), the masks and the read count, the block's addressing -, refilled when the cycle's Hi, D
    // or sample count differ from what one holds (psb_cb_key). A refused state or buffer latches the one pass off
    // until the device goes (psb_failed).
    ID3D11Texture2D*          wsee_p0_tex = nullptr;
    ID3D11RenderTargetView*   wsee_p0_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_p0_srv = nullptr;
    ID3D11Texture2D*          wsee_p1_tex = nullptr;
    ID3D11RenderTargetView*   wsee_p1_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_p1_srv = nullptr;
    ID3D11Texture2D*          wsee_nb_tex = nullptr;
    ID3D11RenderTargetView*   wsee_nb_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_nb_srv = nullptr;
    ID3D11Texture2D*          wsee_nb2_tex = nullptr;   // PSB_PHASE: N2 (NWpost).
    ID3D11RenderTargetView*   wsee_nb2_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_nb2_srv = nullptr;
    UINT                      wsee_pb_w = 0, wsee_pb_h = 0;
    UINT                      wsee_pb_fail_w = 0, wsee_pb_fail_h = 0;
    uint64_t                  wsee_pb_fail_cycle = 0u;   // WSEE_FAIL_RETRY.
    // PSB_ZB: Zb (R32G32_FLOAT: the largest of S over a texel's s x s block, the block pixel holding it) at the
    // record's size, made with P0 / P1 / N / N2 (psb_targets) and released with them; 17 MB at 3840x2160. PSPsbZb
    // at log2 s = PSB_LS (wsee_ensure; psb_zb_failed: refused, not retried until the device goes).
    // wsee_zb_refused: a refused make at the targets' key - no retry until the targets are released; the one pass
    // reads its block without the bound meanwhile.
    ID3D11Texture2D*          wsee_zb_tex = nullptr;
    ID3D11RenderTargetView*   wsee_zb_rtv = nullptr;
    ID3D11ShaderResourceView* wsee_zb_srv = nullptr;
    ID3D11PixelShader*        ps_psb_zb = nullptr;   // One build (PSB_LS).
    bool                      psb_zb_failed = false;
    bool                      wsee_zb_refused = false;   // PSB_ZB: Zb refused at this key.
    ID3D11BlendState*         bs_psb[2] = {};       // PSB_PHASE: the split on, off.
    ID3D11Buffer*             psb_cb[8] = {};       // pushed | test-off << 1 | Tf << 2.
    uint32_t                  psb_cb_key[8][4] = {};   // PSB_LORES: and the scale.
    bool                      psb_cb_set[8] = {};
    bool                      psb_failed = false;
    bool                      ps_wsee_copy_failed = false;
    // PIP_FX: the PIP pass's own scene capture (t0, copied per pass), its depth copied (t1; single-sample, so
    // PSEffect's MSAA_DEPTH 0 build reads it), and that build. Sized to the PIP the last fire saw.
    ID3D11PixelShader*        ps_effect_pip = nullptr;
    bool                      ps_effect_pip_tried = false;   // One compile attempt per device.
    ID3D11Texture2D*          pip_scene_tex = nullptr;
    ID3D11ShaderResourceView* pip_scene_srv = nullptr;
    // The chain's second buffer: passes ping-pong [0] -> [1] -> [0] and the last one writes the PIP target, so the PIP
    // is copied once per cycle.
    ID3D11Texture2D*          pip_scene_tex2 = nullptr;
    ID3D11ShaderResourceView* pip_scene_srv2 = nullptr;
    ID3D11RenderTargetView*   pip_scene_rtv[2] = {};
    UINT                      pip_scene_w = 0, pip_scene_h = 0;
    DXGI_FORMAT               pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11Texture2D*          pip_depth_tex = nullptr;
    ID3D11ShaderResourceView* pip_depth_srv = nullptr;
    UINT                      pip_depth_w = 0, pip_depth_h = 0;
    DXGI_FORMAT               pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11BlendState*        blend_ui_replace = nullptr;   // Blend off, RGB-only write: builtin UI-phase.
    // Custom user FX only: hardware-lerps the chain-composited output by coverage (their shaders cannot be assumed
    // coverage-aware).
    ID3D11BlendState*        blend_ui_masked = nullptr;
    // Custom user FX only, a pass with uiSpill (the builtin spill lane composites in-shader): the output plus the
    // frame's uncovered share, src + dst x (1 - dst.a).
    ID3D11BlendState*        blend_ui_spill = nullptr;
    ID3D11BlendState*        blend_alpha_zero = nullptr;   // Blend off, alpha-only write mask.
    ID3D11PixelShader*       ps_alpha_zero = nullptr;
    ID3D11PixelShader*       ps_premult_copy = nullptr;
    ID3D11RasterizerState*   rasterizer = nullptr;   // cullNone, MSAA, no depth bias, no depth clip.
    ID3D11Texture2D*          sun_tex = nullptr;
    ID3D11DepthStencilView*   sun_dsv = nullptr;
    ID3D11ShaderResourceView* sun_srv = nullptr;
    ID3D11Texture2D* sun2_tex = nullptr;
    ID3D11DepthStencilView* sun2_dsv = nullptr;
    ID3D11ShaderResourceView* sun2_srv = nullptr;
    ID3D11Texture2D* sun3_tex = nullptr;
    ID3D11DepthStencilView* sun3_dsv = nullptr;
    ID3D11ShaderResourceView* sun3_srv = nullptr;
    ID3D11Texture2D* sun4_tex = nullptr;
    ID3D11DepthStencilView* sun4_dsv = nullptr;
    ID3D11ShaderResourceView* sun4_srv = nullptr;
    ID3D11Texture2D* sun5_tex = nullptr;   // Far band (SUN_FAR_BAND).
    ID3D11DepthStencilView* sun5_dsv = nullptr;
    ID3D11ShaderResourceView* sun5_srv = nullptr;
    // [0]=hero [1]=mid [2]=outer.
    ID3D11Texture2D*          sun_pf_tex[4] = {};   // FAR_PF: [3] = the far tier's.
    ID3D11RenderTargetView*   sun_pf_rtv[4] = {};
    ID3D11ShaderResourceView* sun_pf_srv[4] = {};
    ID3D11RenderTargetView*   sun_pf_rtvm[4][12] = {};
    ID3D11Texture2D*          sun_pf_scr = nullptr;
    ID3D11ShaderResourceView* sun_pf_scr_srvm[12] = {};
    ID3D11PixelShader*        ps_sunpfm = nullptr;
    ID3D11BlendState*         blend_pf = nullptr;
    ID3D11SamplerState*       samp_pf = nullptr;   // linear-clamp; s1 (SHADOW_GATHER).
    ID3D11VertexShader*       vs_sunpf = nullptr;
    ID3D11PixelShader*        ps_sunpf = nullptr;
    // SKIN_GPU (skin_so_ensure): VSSkinSo, the geometry-stage object that carries its stream-output declaration,
    // the rest stream's layout and the palette's constant buffer.
    ID3D11VertexShader*       skin_so_vs = nullptr;
    ID3D11GeometryShader*     skin_so_gs = nullptr;
    ID3D11InputLayout*        skin_so_layout = nullptr;
    ID3D11Buffer*             skin_so_cb = nullptr;
    // SKIN_SO_BIG: the large class's VSSkinSo, its stream-output object and its CBSkin (KH_SKIN_SO_BONES bones,
    // 65,504 bytes). SKIN_SO_LAZY: made by skin_so_big_ensure only when a binding needs them; released here.
    ID3D11VertexShader*       skin_so_vs_l = nullptr;
    ID3D11GeometryShader*     skin_so_gs_l = nullptr;
    ID3D11Buffer*             skin_so_cb_l = nullptr;
    ID3D11Buffer*             uvs_obj_cb = nullptr;     // KH_USER_VS: the deform pass's own b0 / b1 (the white
    ID3D11Buffer*             uvs_frame_cb = nullptr;   // placeholder's no-borrowing contract).
    ID3D11RasterizerState*    rast_sun = nullptr;   // cullNone, 16 / 3.0 bias; not the ladder's (SUN_RS_TEXT).
    // rast_sun's twin at a lower bias (CullNone, as rast_sun): what sun_rs_pick returns for the union's pass
    // (clamp false); the tiers take its DepthClipEnable-false twin rast_sun_lo_pk where that exists (SUN_PANCAKE).
    ID3D11RasterizerState*    rast_sun_lo = nullptr;   // low-bias default (8 / 1.0).
    ID3D11RasterizerState*    rast_sun_pk = nullptr;
    ID3D11RasterizerState*    rast_sun_lo_pk = nullptr;
    // The injection's own rasterizer.
    ID3D11RasterizerState*    rast_inject = nullptr;
    ID3D11VertexShader*       vs_sundepth = nullptr;
    ID3D11InputLayout*        layout_sundepth = nullptr;   // Mesh slot 0 + per-instance slot 1.
    // CAST_ALPHA: the alpha-aware twins (the same instanced transform with the uv, a depth-only PS that clips on
    // the caster's alpha).
    ID3D11VertexShader*       vs_sundepth_a = nullptr;
    ID3D11PixelShader*        ps_sundepth_a = nullptr;
    ID3D11InputLayout*        layout_sundepth_a = nullptr;   // TEXCOORD0 (uv) on slot 0.
    ID3D11VertexShader*       vs_sundepth_ao = nullptr;      // CAST_ALPHA: VSSunDepthAObj (SUN_OBJ).
    ID3D11InputLayout*        layout_sundepth_ao = nullptr;  // Its layout: the mesh's own four lanes, slot 0 only.
    // One depth array for every casting light: a spot spends 1 slice, a point 6 (a cube unrolled).
    ID3D11RasterizerState*    dls_rast = nullptr;
    ID3D11Texture2D*          dls_tex = nullptr;
    ID3D11ShaderResourceView* dls_srv = nullptr;
    ID3D11DepthStencilView*   dls_dsv[DLS_SLICES_MAX] = {};
    UINT                      dls_slices = 0;
    UINT                      dls_size = 0;
    ID3D11Buffer*             sun_instance_vb = nullptr;
    UINT                      sun_instance_cap = 0;   // Instances the buffer holds.
    // Seam transport instanced twins; both take layout_sundepth's lane shape. Non-fatal: absent, both loops draw per
    // caster.
    ID3D11VertexShader*       vs_seam_inst = nullptr;     // VSSeamInst   (loop A footprint).
    ID3D11VertexShader*       vs_mirror_inst = nullptr;   // VSMirrorInst (loop B prepass).
    // One buffer, both loops: loop A's lanes at [0, nA), loop B's at [nA, nA + nB), one WRITE_DISCARD map per frame.
    ID3D11Buffer*             seam_instance_vb = nullptr;
    UINT                      seam_instance_cap = 0;   // Lanes the buffer holds.
    ID3D11Buffer*             locality_buf = nullptr;   // Extended (>16) caster reach list.
    ID3D11ShaderResourceView* locality_srv = nullptr;   // t2 at the mask cast fire.
    UINT                      locality_cap = 0;   // Casters the list holds.
    // CAST_OCC: the caster-occupancy grid at t35 - a world-XZ texture whose texel holds the min/max Y of every
    // caster reach volume covering that column, so PSMaskCast's "near a caster" gate is one Load instead of a
    // per-pixel loop over every caster.
    ID3D11Texture2D*          castocc_tex = nullptr;
    ID3D11ShaderResourceView* castocc_srv = nullptr;
    bool                     initialized = false;

    // Post-tonemap backbuffer capture (UI-affecting passes).
    ID3D11Texture2D*          bb_tex = nullptr;
    ID3D11ShaderResourceView* bb_srv = nullptr;
    ID3D11RenderTargetView*   bb_rtv = nullptr;   // The spill lane's premultiplying capture and the UI lane's chain
                                                  // (its ping-pong with ui_chain_rtv) draw into it.
    UINT                      bb_w = 0, bb_h = 0;
    DXGI_FORMAT               bb_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11Texture2D*          ui_alpha_stage = nullptr;   // 64x64 staging window for the UI alpha readback.
    ID3D11Texture2D*          ui_chain_tex = nullptr;
    ID3D11RenderTargetView*   ui_chain_rtv = nullptr;
    ID3D11ShaderResourceView* ui_chain_srv = nullptr;

    // Fullscreen-chain targets, single-sample. [0] / [1]: the ping-pong pair (scene format and size): one scene Resolve
    // per frame at any pass count. [2] / [3]: the SSGI grid (g_ssgi_w x g_ssgi_h, R16G16B16A16_FLOAT) - the gather and
    // its a-trous sibling, also dynamicLightFog's grid; [4]: the radiance seed (SSGI_SEED), same size and format.
    ID3D11Texture2D*          chain_tex[5] = {};
    ID3D11RenderTargetView*   chain_rtv[5] = {};
    ID3D11ShaderResourceView* chain_srv[5] = {};
    // Linear-clamp sampler at s2 for the ssgi side chain (the shared prefix owns s1): the radiance seed (26), the
    // gather's radiance taps (22) and the resolve's upsample (24) sample through it. Bound from the seed to the
    // resolve, the prior binding saved / restored (StateBackup covers s0-s1 only).
    ID3D11SamplerState*       sampler = nullptr;
    // GLOW_PYR: the glow passes' pyramids (GlowPyr, per route), their builders (effect.hlsl PSGlowSeed /
    // PSGlowDown, made once with the core set - absent, every glow takes its direct taps) and the anisotropic clamp
    // sampler the taps read through at s2 (bound only for a glow draw, the prior binding put back: GlowBind).
    GlowPyr                 glow_pyr[3];
    ID3D11PixelShader*        ps_glow_seed = nullptr;
    ID3D11PixelShader*        ps_glow_down = nullptr;
    ID3D11SamplerState*       glow_sampler = nullptr;
    // ANA_PYR: anamorphic's one-axis pyramids (AnaPyr, per glow_pyr route and per axis: [route][0] along x,
    // [route][1] along y) and their builders (effect.hlsl PSAnaSeed / PSAnaDown, made with the glow builders -
    // absent, anamorphic takes its direct taps). No sampler: its taps are Loads.
    AnaPyr                  ana_pyr[3][2];
    ID3D11PixelShader*        ps_ana_seed = nullptr;
    ID3D11PixelShader*        ps_ana_down = nullptr;
    // FX_SIDE (effect.hlsl's note): a pass's side value, drawn by the effect unit before the pass (fx_side_draw:
    // PSEffect; fog scatter's by PSFsSide where made): [0] fog scatter's per-pixel fog (R16_FLOAT, the frame's size),
    // [1] the sun flare's visibility and [2] the UI lane's coverage probe (R32_FLOAT, 1 x 1). Made at first use,
    // remade on a size change, released with the device.
    ID3D11Texture2D*          fxs_tex[3] = {};
    ID3D11RenderTargetView*   fxs_rtv[3] = {};
    ID3D11ShaderResourceView* fxs_srv[3] = {};
    UINT                      fxs_w[3] = {};
    UINT                      fxs_h[3] = {};
    // FX_LINZ (effect.hlsl's note): the frame's linear depth [0] and the SSGI / dynamicLightFog grids' copies [1] /
    // [2] (R32_FLOAT), drawn by fx_linz_draw. Made at first use, remade on a size change, released with the device.
    ID3D11Texture2D*          linz_tex[3] = {};
    ID3D11RenderTargetView*   linz_rtv[3] = {};
    ID3D11ShaderResourceView* linz_srv[3] = {};
    UINT                      linz_w[3] = {};
    UINT                      linz_h[3] = {};
    // Scene HDR colour capture (scene_capture_timed): the bound RTV cannot be sampled while bound, so the scene is
    // resolved (MSAA) or copied into an owned single-sample texture + SRV.
    ID3D11Texture2D*          scene_tex = nullptr;
    ID3D11ShaderResourceView* scene_srv = nullptr;
    UINT                      scene_w = 0, scene_h = 0;
    DXGI_FORMAT               scene_fmt = DXGI_FORMAT_UNKNOWN;
    // BLEND_MINMAX: a lighten / darken mesh's own copy of the scene (mm_capture), apart from scene_tex.
    ID3D11Texture2D*          mm_tex = nullptr;
    ID3D11ShaderResourceView* mm_srv = nullptr;
    UINT                      mm_w = 0, mm_h = 0;
    DXGI_FORMAT               mm_fmt = DXGI_FORMAT_UNKNOWN;

    // Engine depth buffer as shader resource.
    ID3D11ShaderResourceView* depth_srv = nullptr;
    // SSAO_MARK: its stencil plane (uint, .g), where the format has one; null on a stencil-less depth (then no AO).
    ID3D11ShaderResourceView* depth_sten_srv = nullptr;
    ID3D11DepthStencilView*   depth_dsv_ro = nullptr;   // Read-only view: lets the PS sample depth
                                                        // while depth-testing.
    void*                     depth_res_identity = nullptr;   // Identity only, never dereferenced.
    UINT                      depth_sample_count = 0;
    uint32_t                  depth_srv_gen = 0;   // PSB_ZB: steps at every release (Zb names the view it read).

    void release_bb_capture() {
        if (bb_tex) { bb_tex->Release(); bb_tex = nullptr; }
        if (bb_srv) { bb_srv->Release(); bb_srv = nullptr; }
        if (bb_rtv) { bb_rtv->Release(); bb_rtv = nullptr; }
        if (ui_alpha_stage) { ui_alpha_stage->Release(); ui_alpha_stage = nullptr; }
        KH_SAFE_RELEASE(ui_chain_tex);   // The chain twin shares the group.
        KH_SAFE_RELEASE(ui_chain_rtv);
        KH_SAFE_RELEASE(ui_chain_srv);
        bb_w = 0; bb_h = 0; bb_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_fx_chain() {
        for (int i = 0; i < 5; ++i) {   // [0] / [1] ping-pong; [2] gather; [3] its a-trous sibling; [4] radiance seed.
            KH_SAFE_RELEASE(chain_tex[i]);
            KH_SAFE_RELEASE(chain_rtv[i]);
            KH_SAFE_RELEASE(chain_srv[i]);
        }
        KH_SAFE_RELEASE(sampler);
        KH_SAFE_RELEASE(dlf_st_dsv);   // DLF_SPLIT: the grid's stencil goes with the grid.
        KH_SAFE_RELEASE(dlf_st_tex);
        dlf_st_w = dlf_st_h = 0;
    }

    void release_scene_capture() {
        KH_SAFE_RELEASE(scene_tex);
        KH_SAFE_RELEASE(scene_srv);
        scene_w = 0; scene_h = 0; scene_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_mm_capture() {   // BLEND_MINMAX.
        KH_SAFE_RELEASE(mm_srv);
        KH_SAFE_RELEASE(mm_tex);
        mm_w = 0; mm_h = 0; mm_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_depth_srv() {
        if (depth_srv) { depth_srv->Release(); depth_srv = nullptr; }
        KH_SAFE_RELEASE(depth_sten_srv);   // SSAO_MARK.
        if (depth_dsv_ro) { depth_dsv_ro->Release(); depth_dsv_ro = nullptr; }
        depth_res_identity = nullptr;
        depth_sample_count = 0;
        ++depth_srv_gen;   // PSB_ZB: a Zb made from the view released here is not read against its successor.
        g_eds_memo_res = nullptr;   // The fast-path key dies with the SRV.
    }

    // VM_SEE: the two targets - at a size change or a refused make (vmsee_targets), a switch to one sample
    // (msaa_toggle_wipe) and the device's release.
    void release_vmsee_targets() {
        KH_SAFE_RELEASE(vmsee_s_srv);
        KH_SAFE_RELEASE(vmsee_s_rtv);
        KH_SAFE_RELEASE(vmsee_s_tex);
        KH_SAFE_RELEASE(vmsee_t_srv);
        KH_SAFE_RELEASE(vmsee_t_rtv);
        KH_SAFE_RELEASE(vmsee_t_tex);
        vmsee_w = vmsee_h = 0;
    }

    // WSEE: the capture's targets and what hangs on them - at a key change or a refused make (wsee_targets), a
    // sample-count change (msaa_toggle_wipe: at 1x with the chain's own, at another count with C alone), a long
    // idle (wsee_cycle_note) and the device's release; the last two and 1x take E and C too (release_wsee_chain).
    void release_wsee_capture() {
        KH_SAFE_RELEASE(wsee_s_srv);
        KH_SAFE_RELEASE(wsee_s_rtv);
        KH_SAFE_RELEASE(wsee_s_tex);
        KH_SAFE_RELEASE(wsee_t_srv);
        KH_SAFE_RELEASE(wsee_t_rtv);
        KH_SAFE_RELEASE(wsee_t_tex);
        KH_SAFE_RELEASE(wsee_b_srv);
        KH_SAFE_RELEASE(wsee_b_tex);
        KH_SAFE_RELEASE(wsee_n_srv);
        KH_SAFE_RELEASE(wsee_b2_srv);
        KH_SAFE_RELEASE(wsee_b2_tex);
        KH_SAFE_RELEASE(wsee_s2_srv);
        KH_SAFE_RELEASE(wsee_s2_rtv);
        KH_SAFE_RELEASE(wsee_s2_tex);
        release_wsee_d3m();   // WSEE_OQ2: D3m and D3's RTV (on D3's texture) first.
        KH_SAFE_RELEASE(wsee_d3_srv);
        KH_SAFE_RELEASE(wsee_d3_uav);
        KH_SAFE_RELEASE(wsee_d3_tex);
        KH_SAFE_RELEASE(wsee_n2_srv);
        KH_SAFE_RELEASE(wsee_n2_dsv);
        KH_SAFE_RELEASE(wsee_n2_tex);
        KH_SAFE_RELEASE(wsee_n_dsv);
        KH_SAFE_RELEASE(wsee_n_tex);
        KH_SAFE_RELEASE(wsee_q_srv);
        KH_SAFE_RELEASE(wsee_q_tex);
        release_wsee_ld();
        release_wsee_pb();   // PSB.
        wsee_w = wsee_h = wsee_samples = wsee_quality = 0;
        wsee_b_fmt = DXGI_FORMAT_UNKNOWN;
        wsee_depth_id = nullptr;
        release_wsee_dsv_pin();   // No identity to match.
    }
    // The pinned depth view (its note above) and the pinned rasterizer states alone.
    void release_wsee_dsv_pin() {
        KH_SAFE_RELEASE(wsee_dsv_pin);
        wsee_dsv_pin_id = nullptr;
        KH_SAFE_RELEASE(wsee_rs_pin);   // WSEE_OQ2: the pinned rasterizer state - the same lifetime.
        wsee_rs_pin_plain = false;
        KH_SAFE_RELEASE(wsee_rs_pin_psb);   // PSB_RSPIN: likewise.
        wsee_rs_pin_psb_ok = wsee_rs_pin_psb_sc = false;
    }
    // WSEE_OQ2: D3m and D3's RTV alone (wsee_d3m_targets' rekey and refusal; the capture's release).
    void release_wsee_d3m() {
        KH_SAFE_RELEASE(wsee_d3m_srv);
        KH_SAFE_RELEASE(wsee_d3m_rtv);
        KH_SAFE_RELEASE(wsee_d3m_tex);
        KH_SAFE_RELEASE(wsee_d3_rtv);
        wsee_d3m_w = wsee_d3m_h = wsee_d3m_s = wsee_d3m_q = 0;
        wsee_d3m_dfmt = DXGI_FORMAT_UNKNOWN;
    }
    // Tp, Tf and Tf's depth alone (also at the first clear after the split was refused, wsee_ld_failed).
    void release_wsee_ld() {
        KH_SAFE_RELEASE(wsee_tp_srv);
        KH_SAFE_RELEASE(wsee_tp_rtv);
        KH_SAFE_RELEASE(wsee_tp_tex);
        KH_SAFE_RELEASE(wsee_tf_srv);
        KH_SAFE_RELEASE(wsee_tf_rtv);
        KH_SAFE_RELEASE(wsee_tf_tex);
        KH_SAFE_RELEASE(wsee_tfd_dsv);
        KH_SAFE_RELEASE(wsee_tfd_tex);
        wsee_ld_w = wsee_ld_h = wsee_ld_s = wsee_ld_q = 0;
    }
    // PSB: P0, P1, N, N2 (PSB_PHASE) and Zb with its refusal (PSB_ZB) alone (with the capture's targets).
    void release_wsee_pb() {
        KH_SAFE_RELEASE(wsee_p0_srv);
        KH_SAFE_RELEASE(wsee_p0_rtv);
        KH_SAFE_RELEASE(wsee_p0_tex);
        KH_SAFE_RELEASE(wsee_p1_srv);
        KH_SAFE_RELEASE(wsee_p1_rtv);
        KH_SAFE_RELEASE(wsee_p1_tex);
        KH_SAFE_RELEASE(wsee_nb_srv);
        KH_SAFE_RELEASE(wsee_nb_rtv);
        KH_SAFE_RELEASE(wsee_nb_tex);
        KH_SAFE_RELEASE(wsee_nb2_srv);   // PSB_PHASE.
        KH_SAFE_RELEASE(wsee_nb2_rtv);
        KH_SAFE_RELEASE(wsee_nb2_tex);
        KH_SAFE_RELEASE(wsee_zb_srv);   // PSB_ZB.
        KH_SAFE_RELEASE(wsee_zb_rtv);
        KH_SAFE_RELEASE(wsee_zb_tex);
        wsee_zb_refused = false;   // PSB_ZB: the next make may try it again.
        wsee_pb_w = wsee_pb_h = 0;
    }
    // C alone: also at a sample-count change (msaa_toggle_wipe) and at the swapchain's resize (hooked_resizebuffers,
    // PRESENT_RESIZE's rule) - a render-resolution change recreates the engine's scene target, and C's reference
    // would keep the old one alive (530 MB at 3840x2160 8x RGBA16F) until the next chain run rekeyed it.
    void release_wsee_c() {
        KH_SAFE_RELEASE(wsee_c_srv);   // The engine's scene texture goes back to its own references.
        wsee_c_res = nullptr;
        wsee_c_fmt = DXGI_FORMAT_UNKNOWN;
        wsee_c_w = wsee_c_h = wsee_c_s = 0;
        // The refusal's identity goes with the view: nothing of ours holds the refused texture, so the engine's next
        // scene target may take its address; the next chain run asks once more (wsee_scene_ms).
        wsee_c_fail_res = nullptr;
        wsee_c_fail_fmt = DXGI_FORMAT_UNKNOWN;
    }
    // The geometry hold. A release mid-cycle (a switch to one sample, the device) leaves the cycle's held copies no
    // buffer: the generation tells the flush (wsee_hold_ref), which drops them.
    void release_wsee_hold() {
        if (wsee_hold_vb) ++wsee_hold_gen;
        KH_SAFE_RELEASE(wsee_hold_vb);
        wsee_hold_cap = 0u;
        wsee_hold_fail_cap = 0u;   // WSEE_HOLD_FAIL: the next reserve may try that size again.
        wsee_hold_fail_cycle = 0u;
    }
    void release_wsee_chain() {
        KH_SAFE_RELEASE(wsee_e_srv);
        KH_SAFE_RELEASE(wsee_e_rtv);
        KH_SAFE_RELEASE(wsee_e_tex);
        wsee_e_w = wsee_e_h = 0;
        KH_SAFE_RELEASE(wsee_es_dsv);   // WSEE_EDGE_COST: E's stencil goes with it.
        KH_SAFE_RELEASE(wsee_es_tex);
        release_wsee_c();
    }
    void release_wsee_targets() {
        release_wsee_capture();
        release_wsee_chain();
        release_wsee_hold();
    }

    void release() {
        KH_SAFE_RELEASE(vs);
        KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(vs_fullscreen);
        KH_SAFE_RELEASE(ps_effect);
        KH_SAFE_RELEASE(ps_effect_lean);   // FX_LEAN_PASS.
        ps_effect_samples = 0;
        KH_SAFE_RELEASE(ps_fx_linz);   // FX_SPLIT / FX_LINZ.
        KH_SAFE_RELEASE(ps_ssgi_gather);
        KH_SAFE_RELEASE(ps_ssgi_atrous);
        KH_SAFE_RELEASE(ps_dlf_gather);
        KH_SAFE_RELEASE(ps_dlf_filter);
        KH_SAFE_RELEASE(ps_fs_split);   // FS_SPLIT.
        KH_SAFE_RELEASE(ps_fs_side);   // WSEE_EDGE_COST.
        KH_SAFE_RELEASE(ps_fs_split_lean);   // KH_FX_LEAN.
        KH_SAFE_RELEASE(ps_dlf_comp);
        KH_SAFE_RELEASE(ps_dlf_gather_lean);   // DLF_SPLIT (its stencil: release_fx_chain, below).
        KH_SAFE_RELEASE(ps_dlf_class);
        KH_SAFE_RELEASE(dss_dlf_mark);
        KH_SAFE_RELEASE(dss_dlf_sel);
        dlf_st_fail_w = dlf_st_fail_h = 0;
        KH_SAFE_RELEASE(vs_composite);
        KH_SAFE_RELEASE(ps_composite);
        KH_SAFE_RELEASE(ps_composite_arb);
        ps_composite_samples = 0;
        KH_SAFE_RELEASE(comp_depth_tex);
        KH_SAFE_RELEASE(comp_depth_srv);
        KH_SAFE_RELEASE(snap_rtv);
        KH_SAFE_RELEASE(ps_depth_resolve);
        ps_resolve_samples = 0;
        comp_depth_time = -1.0f;
        KH_SAFE_RELEASE(thm_tex);
        KH_SAFE_RELEASE(thm_srv);
        thm_device_lost();   // THM_DEVRESET: the live flags go with the texture.
        comp_depth_w = comp_depth_h = 0;
        comp_depth_samples = 1;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vs_tex);
        KH_SAFE_RELEASE(vs_inst);
        KH_SAFE_RELEASE(vs_inst_tex);
        KH_SAFE_RELEASE(vs_comp_inst);
        KH_SAFE_RELEASE(vs_comp_inst_tex);
        KH_SAFE_RELEASE(layout_inst);
        KH_SAFE_RELEASE(layout_inst_tex);
        KH_SAFE_RELEASE(vs_inst_sk);   // SKIN_POOL.
        KH_SAFE_RELEASE(vs_inst_sk_tex);
        KH_SAFE_RELEASE(vs_comp_inst_sk);
        KH_SAFE_RELEASE(vs_comp_inst_sk_tex);
        KH_SAFE_RELEASE(vs_sundepth_sk);
        KH_SAFE_RELEASE(vs_seam_inst_sk);
        KH_SAFE_RELEASE(vs_mirror_inst_sk);
        KH_SAFE_RELEASE(layout_inst_sk);
        for (int in_i = 0; in_i < 2; ++in_i) {
            KH_SAFE_RELEASE(inst_vb[in_i]);
            inst_cursor[in_i] = 0;
            inst_fresh[in_i] = true;
        }
        KH_SAFE_RELEASE(ps_tex);
        KH_SAFE_RELEASE(vs_comp_tex);
        KH_SAFE_RELEASE(ps_comp_tex);
        KH_SAFE_RELEASE(ps_comp_arb_tex);
        KH_SAFE_RELEASE(layout_tex);
        KH_SAFE_RELEASE(mat_sampler);
        KH_SAFE_RELEASE(mat_srv);
        KH_SAFE_RELEASE(mat_sb);
        mat_cap = 0;
        KH_SAFE_RELEASE(obj_srv);
        KH_SAFE_RELEASE(obj_sb);
        obj_cap = 0;
        obj_wait = 0;   // OBJBUF_RETRY.
        for (auto& mb : mesh_vb) KH_SAFE_RELEASE(mb);
        mesh_vb.clear();
        for (auto& mi : mesh_ib) KH_SAFE_RELEASE(mi);
        mesh_ib.clear();
        for (auto& mg : mesh_grave) {
            KH_SAFE_RELEASE(mg.vb);
            KH_SAFE_RELEASE(mg.ib);
            delete mg.def;
            mg.def = nullptr;
        }
        mesh_grave.clear();
        KH_SAFE_RELEASE(sun_tex);
        KH_SAFE_RELEASE(sun_dsv);
        KH_SAFE_RELEASE(sun_srv);
        KH_SAFE_RELEASE(sun2_srv);
        KH_SAFE_RELEASE(sun2_dsv);
        KH_SAFE_RELEASE(sun2_tex);
        KH_SAFE_RELEASE(sun3_srv);
        KH_SAFE_RELEASE(sun3_dsv);
        KH_SAFE_RELEASE(sun3_tex);
        KH_SAFE_RELEASE(sun4_srv);
        KH_SAFE_RELEASE(sun4_dsv);
        KH_SAFE_RELEASE(sun4_tex);
        KH_SAFE_RELEASE(sun5_srv);
        KH_SAFE_RELEASE(sun5_dsv);
        KH_SAFE_RELEASE(sun5_tex);
        // Released here: nothing else owns the pyramid resources.
        for (int pf_i = 0; pf_i < 4; ++pf_i) {
            KH_SAFE_RELEASE(sun_pf_srv[pf_i]);
            KH_SAFE_RELEASE(sun_pf_rtv[pf_i]);
            KH_SAFE_RELEASE(sun_pf_tex[pf_i]);
            for (int m = 0; m < 12; ++m)
                KH_SAFE_RELEASE(sun_pf_rtvm[pf_i][m]);
        }
        for (int m = 0; m < 12; ++m)
            KH_SAFE_RELEASE(sun_pf_scr_srvm[m]);
        KH_SAFE_RELEASE(sun_pf_scr);
        KH_SAFE_RELEASE(ps_sunpfm);
        KH_SAFE_RELEASE(ps_sunpf);
        KH_SAFE_RELEASE(vs_sunpf);
        KH_SAFE_RELEASE(uvs_frame_cb);   // KH_USER_VS.
        KH_SAFE_RELEASE(uvs_obj_cb);
        KH_SAFE_RELEASE(skin_so_cb_l);   // SKIN_SO_BIG.
        KH_SAFE_RELEASE(skin_so_gs_l);
        KH_SAFE_RELEASE(skin_so_vs_l);
        KH_SAFE_RELEASE(skin_so_cb);   // SKIN_GPU.
        KH_SAFE_RELEASE(skin_so_layout);
        KH_SAFE_RELEASE(skin_so_gs);
        KH_SAFE_RELEASE(skin_so_vs);
        KH_SAFE_RELEASE(blend_pf);
        KH_SAFE_RELEASE(samp_pf);
        KH_SAFE_RELEASE(rast_sun);
        KH_SAFE_RELEASE(rast_sun_lo);
        KH_SAFE_RELEASE(rast_sun_pk);   // SUN_PANCAKE twins.
        KH_SAFE_RELEASE(rast_sun_lo_pk);
        KH_SAFE_RELEASE(rast_inject);
        KH_SAFE_RELEASE(vs_sundepth);
        KH_SAFE_RELEASE(layout_sundepth);
        KH_SAFE_RELEASE(vs_sundepth_a);
        KH_SAFE_RELEASE(ps_sundepth_a);
        KH_SAFE_RELEASE(layout_sundepth_a);
        KH_SAFE_RELEASE(vs_sundepth_ao);
        KH_SAFE_RELEASE(layout_sundepth_ao);
        KH_SAFE_RELEASE(sun_instance_vb);
        sun_instance_cap = 0;
        KH_SAFE_RELEASE(vs_seam_inst);   // SEAM_INST.
        KH_SAFE_RELEASE(vs_mirror_inst);
        KH_SAFE_RELEASE(seam_instance_vb);
        seam_instance_cap = 0;
        KH_SAFE_RELEASE(dls_rast);
        for (int d_i = 0; d_i < DLS_SLICES_MAX; ++d_i) KH_SAFE_RELEASE(dls_dsv[d_i]);
        KH_SAFE_RELEASE(dls_srv);
        KH_SAFE_RELEASE(dls_tex);
        dls_slices = 0;
        dls_size = 0;
        KH_SAFE_RELEASE(locality_srv);
        KH_SAFE_RELEASE(locality_buf);
        locality_cap = 0;
        KH_SAFE_RELEASE(castocc_srv);
        KH_SAFE_RELEASE(castocc_tex);
        KH_SAFE_RELEASE(constant_buffer);
        KH_SAFE_RELEASE(composite_cb);
        for (int cr_i = 0; cr_i < 2; ++cr_i) {
            KH_SAFE_RELEASE(cb_ring[cr_i]);
            cb_ring_cursor[cr_i] = 0;
            cb_ring_fresh[cr_i] = true;
        }
        KH_SAFE_RELEASE(frame_cb);
        KH_SAFE_RELEASE(composite_frame_cb);
        KH_SAFE_RELEASE(dss_test);
        KH_SAFE_RELEASE(dss_test_write);
        KH_SAFE_RELEASE(dss_test_write_mark);   // SSAO_MARK.
        KH_SAFE_RELEASE(dss_mark_clear);
        KH_SAFE_RELEASE(dss_mark_test);
        KH_SAFE_RELEASE(dss_off);
        for (int i = 0; i < 6; ++i) KH_SAFE_RELEASE(blend_modes[i]);
        KH_SAFE_RELEASE(dls_world_blend);
        KH_SAFE_RELEASE(dls_world_fog_blend);
        KH_SAFE_RELEASE(dlsw_srv);
        KH_SAFE_RELEASE(dlsw_rtv);
        KH_SAFE_RELEASE(dlsw_tex);
        KH_SAFE_RELEASE(dlsw_min_blend);
        KH_SAFE_RELEASE(vs_dls_mask);
        KH_SAFE_RELEASE(ps_dls_mask);
        KH_SAFE_RELEASE(ps_dls_mask_a);
        KH_SAFE_RELEASE(ps_ssao_depth);   // SSAO.
        KH_SAFE_RELEASE(ps_ssao_main);
        KH_SAFE_RELEASE(ps_ssao_blur);
        KH_SAFE_RELEASE(ps_ssao_apply);
        ps_ssao_samples = 0;
        KH_SAFE_RELEASE(ssao_dh_srv);
        KH_SAFE_RELEASE(ssao_dh_rtv);
        KH_SAFE_RELEASE(ssao_dh_tex);
        KH_SAFE_RELEASE(ssao_ao_srv);
        KH_SAFE_RELEASE(ssao_ao_rtv);
        KH_SAFE_RELEASE(ssao_ao_tex);
        KH_SAFE_RELEASE(ssao_ao2_srv);
        KH_SAFE_RELEASE(ssao_ao2_rtv);
        KH_SAFE_RELEASE(ssao_ao2_tex);
        ssao_w = ssao_h = 0;
        ssao_hw = ssao_hh = 0;
        KH_SAFE_RELEASE(ssao_cb);
        KH_SAFE_RELEASE(ps_ssao_nzmark);   // NEARZ_MARK.
        KH_SAFE_RELEASE(nzm_srv);
        KH_SAFE_RELEASE(nzm_rtv);
        KH_SAFE_RELEASE(nzm_tex);
        nzm_w = nzm_h = 0;
        release_vmsee_targets();   // VM_SEE.
        vmsee_fail_w = vmsee_fail_h = 0;
        KH_SAFE_RELEASE(ps_vmsee_s0);
        ps_vmsee_samples = 0;
        ps_vmsee_fail = 0;
        KH_SAFE_RELEASE(bs_vmsee_t);
        bs_vmsee_failed = false;
        release_wsee_targets();   // WSEE.
        wsee_fail_w = wsee_fail_h = wsee_fail_s = wsee_fail_q = 0;
        wsee_fail_f = DXGI_FORMAT_UNKNOWN;
        wsee_fail_cycle = 0u;   // WSEE_FAIL_RETRY: the four stamps go with their keys.
        wsee_e_failed = false;
        wsee_es_failed = false;   // WSEE_EDGE_COST: E's stencil and its states.
        wsee_e_fail_w = wsee_e_fail_h = wsee_es_fail_w = wsee_es_fail_h = 0;   // WSEE_EFAIL_KEY.
        KH_SAFE_RELEASE(dss_wsee_smark);
        KH_SAFE_RELEASE(dss_wsee_ssel);
        KH_SAFE_RELEASE(ps_wsee_one);
        // PSV: every variant of the engine's (and our tail's) pixel shaders, the queries, and our six builtins'
        // registered blobs (their shaders die here too).
        psv_device_lost();
        KH_SAFE_RELEASE(ps_wsee_copy);
        KH_SAFE_RELEASE(ps_wsee_agree);
        KH_SAFE_RELEASE(dss_wsee_nw);
        KH_SAFE_RELEASE(dss_wsee_eq);
        KH_SAFE_RELEASE(bs_wsee_tpre);
        KH_SAFE_RELEASE(bs_wsee_tpost);
        KH_SAFE_RELEASE(bs_wsee_sort);
        bs_wsee_sort_failed = false;
        KH_SAFE_RELEASE(ps_wsee_one_l);
        KH_SAFE_RELEASE(ps_wsee_one_z);   // WSEE_OQ2.
        KH_SAFE_RELEASE(ps_wsee_one_zl);
        KH_SAFE_RELEASE(ps_wsee_d3_res);
        ps_wsee_oq2_failed = false;
        wsee_d3m_fail_w = wsee_d3m_fail_h = wsee_d3m_fail_s = wsee_d3m_fail_q = 0;
        wsee_d3m_fail_f = DXGI_FORMAT_UNKNOWN;
        wsee_d3m_fail_cycle = 0u;
        KH_SAFE_RELEASE(bs_wsee_ldpre);
        KH_SAFE_RELEASE(bs_wsee_ldpost);
        KH_SAFE_RELEASE(bs_wsee_ldpre2);
        KH_SAFE_RELEASE(bs_wsee_ldpost2);
        KH_SAFE_RELEASE(dss_wsee_far);
        wsee_ld_failed = false;
        for (ID3D11BlendState*& b : bs_psb) KH_SAFE_RELEASE(b);   // PSB: its states and buffers.
        for (int rb_i = 0; rb_i < 8; ++rb_i) {
            KH_SAFE_RELEASE(psb_cb[rb_i]);
            psb_cb_set[rb_i] = false;
        }
        psb_failed = false;
        KH_SAFE_RELEASE(ps_psb_zb);   // PSB_ZB.
        psb_zb_failed = false;
        wsee_pb_fail_w = wsee_pb_fail_h = 0;
        wsee_pb_fail_cycle = 0u;
        wsee_ld_fail_w = wsee_ld_fail_h = wsee_ld_fail_s = wsee_ld_fail_q = 0;
        wsee_ld_fail_cycle = 0u;
        wsee_q_failed = false;
        wsee_v2_failed = false;
        ps_wsee_copy_failed = false;
        KH_SAFE_RELEASE(ps_effect_pip);   // PIP_FX.
        ps_effect_pip_tried = false;
        KH_SAFE_RELEASE(pip_scene_srv);
        KH_SAFE_RELEASE(pip_scene_tex);
        KH_SAFE_RELEASE(pip_scene_srv2);
        KH_SAFE_RELEASE(pip_scene_tex2);
        KH_SAFE_RELEASE(pip_scene_rtv[0]);
        KH_SAFE_RELEASE(pip_scene_rtv[1]);
        pip_scene_w = pip_scene_h = 0;
        pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
        KH_SAFE_RELEASE(pip_depth_srv);
        KH_SAFE_RELEASE(pip_depth_tex);
        pip_depth_w = pip_depth_h = 0;
        pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
        dlsw_w = dlsw_h = 0;
        KH_SAFE_RELEASE(blend_ui_replace);
        KH_SAFE_RELEASE(blend_ui_masked);
        KH_SAFE_RELEASE(blend_ui_spill);
        KH_SAFE_RELEASE(blend_alpha_zero);
        KH_SAFE_RELEASE(ps_alpha_zero);
        KH_SAFE_RELEASE(ps_premult_copy);
        KH_SAFE_RELEASE(ps_inj_depth_a);
        KH_SAFE_RELEASE(ps_maskprime);
        KH_SAFE_RELEASE(ps_maskcast);
        KH_SAFE_RELEASE(ps_rpmerge);   // VOL_REPLAY.
        KH_SAFE_RELEASE(ps_rpmergemir);   // MIR_REPLAY.
        KH_SAFE_RELEASE(ps_dls_world);
        KH_SAFE_RELEASE(ps_dls_world_fog);
        KH_SAFE_RELEASE(ps_seam_foot);

        KH_SAFE_RELEASE(rasterizer);
        KH_SAFE_RELEASE(rasterizer_cull);
        KH_SAFE_RELEASE(rasterizer_front);
        KH_SAFE_RELEASE(vs_white);
        KH_SAFE_RELEASE(ps_white);
        KH_SAFE_RELEASE(layout_white);
        KH_SAFE_RELEASE(white_obj_cb);
        KH_SAFE_RELEASE(white_frame_cb);
        KH_SAFE_RELEASE(rast_white);
        KH_SAFE_RELEASE(dss_white);
        white_tried = false;   // The device died; the next ensure may retry.
        white_ready = false;
        release_bb_capture();
        release_fx_chain();
        release_scene_capture();
        release_mm_capture();   // BLEND_MINMAX.
        release_depth_srv();
        for (GlowPyr& p : glow_pyr) p.release();   // GLOW_PYR.
        KH_SAFE_RELEASE(ps_glow_seed);
        KH_SAFE_RELEASE(ps_glow_down);
        KH_SAFE_RELEASE(glow_sampler);
        for (AnaPyr (&r)[2] : ana_pyr) for (AnaPyr& p : r) p.release();   // ANA_PYR.
        KH_SAFE_RELEASE(ps_ana_seed);
        KH_SAFE_RELEASE(ps_ana_down);
        for (int fs_i = 0; fs_i < 3; ++fs_i) {   // FX_SIDE.
            KH_SAFE_RELEASE(fxs_srv[fs_i]);
            KH_SAFE_RELEASE(fxs_rtv[fs_i]);
            KH_SAFE_RELEASE(fxs_tex[fs_i]);
            fxs_w[fs_i] = fxs_h[fs_i] = 0;
        }
        for (int lz_i = 0; lz_i < 3; ++lz_i) {   // FX_LINZ.
            KH_SAFE_RELEASE(linz_srv[lz_i]);
            KH_SAFE_RELEASE(linz_rtv[lz_i]);
            KH_SAFE_RELEASE(linz_tex[lz_i]);
            linz_w[lz_i] = linz_h[lz_i] = 0;
        }
        // The sun maps, their pyramids and the tier keys' subjects died above; the flags and the input hash that claim
        // them live outside Resources. render_sun_depth's hash early return is reached from mask_cast_engine with no
        // ensure_resources ahead of it: a hash left standing here would skip the rebuild for as long as it matched.
        sun_ladder_forget();
        initialized = false;
    }
};

static Resources g_res;
static bool g_reset_hook_installed = false;
}