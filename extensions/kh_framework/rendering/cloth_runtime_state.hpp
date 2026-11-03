#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of CLOTH runtime:
// the collider BVH registry, cloth building and write-back, the shape notes, the collider gather and the ground patch.
// Never include it on its own.
namespace RenderIntegration {

// Exact-bit key over a normalized mesh position. The weights are carried from the expanded triangle list to the welded
// vertex array by POSITION, not by index: three stages between them permute the array (meshgen::bake reorders triangle
// corners, lod_build appends whole levels, mesh_weld dedups and re-orders for the vertex cache) and a parallel
// array would have to be maintained in all three. None of them changes the float bits, so an exact key is exact.
struct ClothPosKey {
    float p[3];
    bool operator==(const ClothPosKey& o) const { return memcmp(p, o.p, sizeof(p)) == 0; }
};
struct ClothPosHash {
    size_t operator()(const ClothPosKey& k) const {
        return static_cast<size_t>(CryptoGenerator::fnv1a64_raw(k.p, sizeof(k.p)));
    }
};

// CLOTH runtime - everything the simulation core does not know about: meshes, objects, threads and the device.
//
// THE THREAD MAP. The game thread publishes inputs and consumes outputs; a pool of cloth workers owns the stepping.
// An instance is handed to a worker by setting 'busy', and the game thread touches nothing a worker reads or writes
// inside a busy instance (it may mark one retired, under g_cloth_mu, and carry its owed time in dt_owed - fields no
// worker touches): that flag is the whole ownership protocol, and there is no lock on the step path. The deformed
// vertex array is double buffered and published by a release store, so the upload never blocks and never tears; the
// buffers keep their size once built (a resize to the same size does not reallocate), which makes a reader outside
// the park a stale read rather than a use-after-free.

// A collider mesh's BVH, built at a shape (physics_bvh_shape) and shared by every collider object drawing that
// mesh at those proportions. Keyed by mesh id in the high 32 bits and the shape's key in the low. The stamp is the
// gather that last used it: a collider whose proportions change leaves its old shapes behind, and
// physics_gather_colliders drops any unused for PHYSICS_BVH_IDLE gathers - a worker still stepping against one
// holds its own reference (in_col_own).
struct PhysicsBvhEntry { std::shared_ptr<const PhysicsBvh> bvh; uint32_t used; };
static std::mutex g_physics_bvh_mu;
static std::unordered_map<uint64_t, PhysicsBvhEntry> g_physics_bvh;
static uint32_t g_physics_bvh_frame = 0;   // Game thread: the gather's own counter.
static constexpr uint32_t PHYSICS_BVH_IDLE = 600u;
// Why a cloth is NOT simulating is reported (the RPT): clothInstances alone cannot tell an object that asked and was
// refused from one that never asked, and the two have different fixes. One report per mesh id, so a fixed and
// re-imported model reports again.
static std::vector<int> g_cloth_warned;

// The cosine two render vertices on one particle must agree to within to share a normal after deformation. A UV
// seam's copies carry the same authored normal to within export rounding; a hard edge worth keeping is tens of degrees.
static constexpr float CLOTH_NRM_JOIN = 0.999f;

// CLOTH_SHAPE - a key per drawn cloth shape, for the passes that cache on "has this draw changed"
// (g_cloth_gen_slot). gen cannot serve them: it moves on every step, so a cloth hanging at rest would re-render every
// shadow map every frame. A new key is taken only when some vertex has moved more than CLOTH_SHAPE_M along any
// axis from the shape that took the CURRENT key (not last frame's, so a slow creep still takes one once it adds up);
// the bound is half the finest sun tier's texel (SUN_HERO_HALF 2 m over SUN_HERO_BASE 4096, ~1 mm), under
// anything a shadow map can show. Keys come from one process-wide serial and are never 0 (0 = no cloth on the slot),
// so no two shapes of any two instances share one (32-bit: until it wraps, 2^32 shapes on).
static constexpr float CLOTH_SHAPE_M = 0.0005f;
static std::atomic<uint32_t> g_cloth_shape_serial{ 0 };
// The parallel of g_cloth_objs: each copied object's handle, for the chain's settings (cloth_sync).
static std::vector<std::string> g_cloth_objs_h;

// Gather this frame's colliders once, then hand each cloth the ones within reach. The broadphase is a sphere test
// against the cloth's own bounds - a collider on the far side of the map costs one distance compare.
struct PhysicsColSrc {
    PhysicsColliderView view;
    std::shared_ptr<const PhysicsBvh> own;
};

static std::vector<PhysicsColSrc> g_physics_col;   // Game thread only.
// Its size, stored by the game thread wherever it changes, for cloth_stats_publish on the render thread (which
// may not read the vector itself).
static std::atomic<uint64_t> g_physics_col_n{ 0 };
// cloth_sync's copy of every object with either cloth role, from g_draw_list under its mutex. Game thread only.
static std::vector<RenderObject> g_cloth_objs;
// THE FLOATING ORIGIN. A float resolves 0.49 mm at 6 km and 2 mm at 25 km, Arma's coordinates reach tens of km,
// and every constraint is a difference of two positions, so absolute world coordinates turn rounding into motion
// no setting can settle. Each instance simulates about its own origin, kept within this many metres of its
// carrier: when the carrier strays further the origin moves to it and everything stored shifts by the same amount.
static constexpr double CLOTH_ORIGIN_M = 4.0;
static bool     g_ground_tried = false;              // Measured for the matrix below, successfully or not.
}