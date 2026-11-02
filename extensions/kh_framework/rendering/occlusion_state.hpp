#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of the visibility
// occluders (KH_VOC) and the occluder floor. Never include it on its own.
namespace RenderIntegration {
static constexpr int   KH_VOC_MAX = 16;           // Occluders per pass.
static constexpr int   KH_VOC_PX_BUDGET = 20000;  // Covered-pixel writes cap (partial stays conservative).
static constexpr int   KH_VOC_TEST_PX = 1024;     // A larger test rect reads visible (cost bound).
static constexpr float KH_VOC_MIN_SCORE = 5.0e-4f;   // r^2/d^2 occluder floor.
static constexpr float KH_VOC_EPS_M = 0.05f;
static constexpr float KH_VOC_FACE_COS = 0.10f;   // Grazing faces skipped (ClipEdgeSliver could eat them).
static constexpr float KH_VOC_NEAR_M = 0.30f;     // ClipOwnNear (0.05 m) with margin.
static constexpr float KH_VOC_INSET = 0.99f;      // Faces pulled inward: cover less, sit deeper
                                                   // (a drawn surface's box; a declared one is exact).
// KH_VOC_BURIAL (kh_voc_floor): no floor - the terrain lane's endpoint test cannot reach the occluder (its march is
// not bounded), or no heightfield is live (the lanes are inert). A declared occluder (KH_OCC_FORCE) carries it too;
// an object refused as an occluder gets KH_VOC_FLOOR_REFUSE (rendering/culling_state.hpp).
static constexpr float KH_VOC_FLOOR_NONE = -3.0e38f;

// KH_VOC_NEAR_CLIP: every face is clipped at view depth KH_VOC_W_MIN before it is projected, so a face reaching
// behind the camera keeps its part in front (dropping the whole face lost an occluder as soon as a corner went behind
// the camera: a wall beside the view with its centre off the screen). A ray through the clipped face's projection
// meets the face at a point of the clipped part: it is written at that part's farthest corner. A clip point is made
// once per box edge, from its lower corner index, so two faces meeting at an edge share it to the bit.
static constexpr float KH_VOC_W_MIN = 0.06f;   // Above kh_voc_project's 0.05 floor.
// KH_VOC_UNION: the cells a shared edge between two of an occluder's drawn front faces crosses. Inner-conservative
// per face, such a cell is inside neither face (it straddles the edge), though the faces together cover it whole - a
// column of uncovered cells down every edge between two faces the camera sees, however deep inside the box's
// outline it runs. Here a cell the edge crosses counts when the drawn faces cover it together: the edge's two faces
// (khvb_pair: the cell cut by the edge's line, each side strictly inside its face's other edges), or at a corner all
// three (khvb_cover: the cell's square minus each face in turn, its outside kept edge by edge as convex pieces,
// leaves nothing). Off the edge's line the union on either side is one face, so no other cell can need it. The faces
// are built from points projected once (the eight corners and the near clip's points), so two faces meeting at an
// edge share its two projected points and its line (to the bit, negated): nothing of the true union falls between
// them. A cell is written at the farthest-corner distance of the farthest face that meets it (every point of the
// cell looks through one of them). khvb_cover's emptiness threshold is 1e-6 of a cell (float slivers along the clip
// lines); at a silhouette that is far below the drawn surface's own margin past a drawn occluder's inset box.
struct KhVocPoly { float x[16], y[16]; int n; };
static constexpr float KH_VOC_UNION_EPS = 1.0e-6f;   // Cell areas; see KH_VOC_UNION.
static constexpr int   KH_VOC_UNION_PIECES = 32;

// KH_VOC_BURIAL: with the heightfield lanes armed PSComposite discards fragments whose terrain clearance sits below
// -thmMeta.z - a fragment's own height over the terrain at its footprint (the endpoint test, at every distance)
// and, from view depth KH_THM_MIN_DIST_M, the lowest its camera ray's march finds (KhThmClearance) - so an occluder
// partly below the terrain can lose pixels the occlusion grid counted as depth-written. The check reads snapshots
// of exactly the uploaded grid (g_voc_thm_*, published by the upload in the flush - the render thread's or the
// park's - never the autobuild's staging): its nodes, and their max per KH_VOC_THM_DS-node cell. It takes the max
// over every node the footprint's support touches (a
// sample's cell corners: its height is a convex mix of them, whichever triangle or the bilinear fit KhThmHeight
// takes - KH_THM_EXACT) - node by node while that is at most KH_VOC_THM_FINE_CAP nodes, else by the coarse cells -
// and answers the line (engine y) above which the endpoint test cannot discard a fragment of the occluder: its
// floor (kh_voc_floor). The cull keeps only the part of the box above it (kh_vis_occ_build). Every unknown answers
// 'refuse' = not an occluder (under-cull only). The march is not bounded: a fragment it discards has its camera ray
// pass under the heightfield on the way, and a mesh behind that pixel lies farther along the same ray, behind the
// same dip - so the cull hides nothing the heightfield shows. Against an unculled frame it can still differ: the
// hidden mesh's own march samples its longer ray at other points and may miss a narrow dip the occluder's caught.
static constexpr UINT KH_VOC_THM_DS = 8;      // Fine nodes per coarse max cell.
static constexpr int  KH_VOC_THM_CAP = 4096;  // Coarse cells per query; wider refuses.
static constexpr int  KH_VOC_THM_FINE_CAP = 4096;   // Nodes read one by one per query.
static std::vector<float> g_voc_thm_max;
static std::vector<float> g_voc_thm_fine;     // The uploaded grid's nodes, g_voc_thm_fw per row.
static UINT  g_voc_thm_w = 0;                 // Coarse width (the height is implied by the vector).
static UINT  g_voc_thm_fw = 0;                // Fine dims (the texture's).
static UINT  g_voc_thm_fh = 0;
static float g_voc_thm_origin[2] = {};
static float g_voc_thm_cell = 0.0f;           // Fine cell (the texture's).
static bool  g_voc_thm_valid = false;
}