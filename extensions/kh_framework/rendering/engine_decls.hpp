#pragma once

// The shared head (rendering_integration.hpp): the engine-side (injection) functions the feature half calls ahead of
// their definitions. Never include it on its own.
namespace RenderIntegration {
// KH_SNAP_RECT: the pixel rectangle the visible meshes project to under pv, plus KH_SNAP_RECT_MARGIN (empty when no
// mesh is visible); false = a corner is behind the camera or the target size is unknown. kh_ssao_rect pads it by
// KH_SSAO_RECT_PAD into a drawer's SSAO pass rectangle (the whole target on false). Defined with the snapshot in
// rendering/snapshot.hpp.
inline bool kh_snapshot_rect(const RVExtBridge::ProjectionViewTransform& pv, UINT khsr_w, UINT khsr_h, D3D11_RECT& khsr_out);

// Engine-side (injection) functions the feature half calls ahead of their definitions in the injection half
// (rendering_integration_injection.hpp's parts), which rendering_integration.hpp includes after
// rendering_integration_features.hpp.
static void __stdcall on_engine_reset();
inline void kh_skel_centre_d(const RenderObject& khsd_o, double khsd_pos[3]);
inline void kh_attach_skel_place(RenderObject& khsp_o, const double khsp_root[3], bool& khsp_moved);
inline void kh_attach_moved_note(const RenderObject& khmn_o, bool khmn_moved, bool khmn_rt);
inline std::string kh_scene_capture_timed(ID3D11Device* dev, ID3D11DeviceContext* ctx);
inline std::string ensure_depth_srv(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                    UINT* out_width, UINT* out_height);
inline void kh_fx_depth_pair_guard(ConstantData& cbd, uint64_t khfx_serial);
inline bool is_composite_eligible(const RenderObject& o);
inline bool kh_sky_static();
inline const float* kh_shadow_sun();
inline bool kh_sun_axis_witnessed();
inline bool kh_adopt_frame_view(RVExtBridge::ProjectionViewTransform& pv);
inline bool kh_vmsee_valid();
inline bool kh_wsee_valid();   // KH_WSEE.
inline bool kh_wsee_ensure(ID3D11Device* dev);
inline bool kh_wsee_d3m_resolve(ID3D11DeviceContext* ctx, uint64_t khdr_c);   // KH_WSEE_OQ2.
inline void kh_psv_own(int khpo_slot, const void* khpo_bc, SIZE_T khpo_n, ID3D11PixelShader* khpo_ps);   // KH_PSV.

}