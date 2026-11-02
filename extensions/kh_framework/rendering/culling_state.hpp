#pragma once

// The shared head (rendering_integration.hpp): the culling, LOD and occluder state (KhCullSet, KhVisOcc, KhLocalCull).
// Never include it on its own.
namespace RenderIntegration {
// Two writers: flush_locked and inject_composited_meshes both publish it - on the render thread, in KH_FX_LATE's
// chain-only flush (either thread's, inside the engine's resolve), or in a park's flush on the game thread with the
// render thread held (a frame the flush never publishes on would otherwise draw every object at level 0), with the
// same formula from their own pass. KH_LOD_PARK_KEEP: a park publishes only at the injection's last pass height, or
// while the injection has not published within the last two cycles; else the last value stands. Read lock-free by
// kh_lod_pick. KH_PIP_LOD: a pass with a camera and a target of its own (the PIP) hands kh_lod_pick its own value
// instead - this one is the main view's. KH_LOD_ONE_SCALE: both writers go through kh_lod_publish, the one
// definition (the projection's P11 and the height of the viewport the pass draws our meshes with), which writes
// g_lod_zoom with it.
static float g_lod_proj_px = 0.0f;
// KH_LOD_ZOOM: the main view's zoom for the level pick's range term (kh_lod_zoom_of: 1 = unzoomed), published
// with g_lod_proj_px by the same writers, read with it.
static float g_lod_zoom = 1.0f;

struct KhCullSet {
    float p[4][4] = {};   // Left, right, bottom, top - normalised.
    float bias[3] = {};   // Add to a world centre to reach the matrix's space.
    bool  valid = false;
    // The four side planes pass through the apex, so they bound a double cone - the frustum and its mirror behind.
    // "Is this object behind the camera" must not be asked with a near plane from the matrix's column 2 (depth
    // encoding is unrelated).
    float fwd[3] = {};
    float apex[3] = {};
    bool  back_ok = false;
    bool  back_on = false;
};

// KH_VIS_OCC: conservative CPU occlusion culling for the injection colour pass (the one pass that depth-writes every
// solid). Occluders are the opaque solids of any mesh with a KH_OCC_BOX (kh_mesh_occ_prep) that draw that mesh as it
// was proven (kh_occ_drawn_as_built), plus the objects a script declared occluders (KH_OCC_FORCE, drawn or not).
// Their boxes' front faces, each clipped at the near depth (KH_VOC_NEAR_CLIP), rasterize inner-conservatively into a
// small grid of occluder depths at their farthest-corner camera distance, and a cell an edge between two of them
// crosses counts when the two together cover it (KH_VOC_UNION). An object is hidden when every cell its projected
// AABB touches holds an occluder strictly nearer than its nearest possible point. Every guard fails toward
// 'visible'. Occluders enter regardless of their own verdict (hiding is transitive).
static constexpr int   KH_VOC_W = 96;
static constexpr int   KH_VOC_H = 54;
static constexpr float KH_VOC_FLOOR_REFUSE = 3.0e38f;

struct KhVisOcc {
    float z[KH_VOC_W * KH_VOC_H];   // Farthest-corner camera distance of the nearest covering face.
    float vp[4][4];
    float bias[3];   // KhCullSet::bias - world into the matrix's space.
    float cam[3];
    float margin = 0.0f;   // Euclidean metres of extra separation the cull demands: the far-arb clamp's view-depth
                           // cap times the frustum-corner secant (call site).
    bool  on = false;
};

// KH_LOCAL_CULL - a localized pass whose volume no reconstructed point can reach is skipped whole. The localization
// mask (cb.hlsl KhFxFinish) is 0 wherever the point KhWorldPosRelFenced rebuilds from the depth lies outside the
// volume and its falloff - a box of half extents max(r, 0.01) x (1 + max(falloff lane, 0.001)) about the centre,
// the ellipsoid inside it - and every such point lies inside the four side planes of the matrix the pass's inverse
// inverts: a pixel centre is inside the viewport and its depth (> 0: KH_NEAR_RECON's 1 mm floor, through the encode
// pair the matrix itself carries) in front of the camera. Outside one side plane, then, the mask is 0 at every pixel
// and the pass leaves the picture as it was (KH_LOCAL_BLEND: under every blend), so it is skipped as a zero-opacity
// pass is, with that skip's two side effects: the passes either side of it may fuse (one rounding to the scene's
// 16F format fewer) and, every pass skipped, the target keeps the engine's alpha. Not an inverse volume (its mask is
// 1 there), not a .hlsl effect (it applies the mask only if it calls KhFxFinish). Planes in double; no margin: a
// pixel centre lies half a pixel inside the view's edge, some thousand times the float reconstruction's error
// (hx_dd/h4). The scene chain (the flush's camera-relative inverse, while the passes linearise with the pair it was
// built with) and the PIP's localized chain (its own view, pair and camera).
struct KhLocalCull {
    bool   on = false;
    bool   pair_check = false;   // The scene chain: g_ro's engine pair must still be the planes' (pair).
    float  pair[2] = { 0.0f, 0.0f };
    double plane[4][4] = {};      // Inward unit normals and offsets, camera-relative.
    double cam[3] = { 0.0, 0.0, 0.0 };   // fxCam: the shader takes the centre relative to it.
};
static uint64_t g_skin_cull_pip_cycle = ~0ull;   // The cycle a PIP last reached its draws (render thread).
}