#pragma once

// The shared head (rendering_integration.hpp): the engine-side (injection) functions the feature half calls ahead of
// their definitions. Never include it on its own.
namespace RenderIntegration {
// SNAP_RECT: the pixel rectangle the visible meshes project to under pv, plus SNAP_RECT_MARGIN (empty when no
// mesh is visible); false = a corner is behind the camera or the target size is unknown. ssao_rect pads it by
// SSAO_RECT_PAD into a drawer's SSAO pass rectangle (the whole target on false). Defined with the snapshot in
// rendering/snapshot.hpp.
inline bool snapshot_rect(const RVExtBridge::ProjectionViewTransform& pv, UINT w, UINT h, D3D11_RECT& out);

// Engine-side (injection) functions the feature half calls ahead of their definitions in the injection half
// (rendering_integration_injection.hpp's parts), which rendering_integration.hpp includes after
// rendering_integration_features.hpp.
static void __stdcall on_engine_reset();
inline void skel_centre_d(const RenderObject& o, double pos[3]);
inline void attach_skel_place(RenderObject& o, const double root[3], bool& moved);
inline void attach_moved_note(const RenderObject& o, bool moved, bool rt);
inline std::string scene_capture_timed(ID3D11Device* dev, ID3D11DeviceContext* ctx);
inline std::string ensure_depth_srv(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                    UINT* out_width, UINT* out_height);
inline void fx_depth_pair_guard(ConstantData& cbd, uint64_t serial);
inline bool is_composite_eligible(const RenderObject& o);
inline bool sky_static();
inline const float* shadow_sun();
inline bool sun_axis_witnessed();
inline bool adopt_frame_view(RVExtBridge::ProjectionViewTransform& pv);
inline bool vmsee_valid();
inline bool wsee_valid();   // WSEE.
inline bool wsee_ensure(ID3D11Device* dev);
inline bool wsee_d3m_resolve(ID3D11DeviceContext* ctx, uint64_t c);   // WSEE_OQ2.
inline void psv_own(int slot, const void* bc, SIZE_T n, ID3D11PixelShader* ps);   // PSV.

}