#pragma once

// The shared head (rendering_integration.hpp): the cloth, collider and chain types (ClothState, PhysicsBvh,
// ChainState and their parts). Never include it on its own.
namespace RenderIntegration {

// One simulated point. Render vertices map onto these many-to-one: a UV seam splits a render vertex, not the cloth.
struct ClothPart {
    // Positions in the simulation's frame: engine axes, metres, the world less the instance's floating origin
    // (cloth_sync) - never world-absolute.
    float p[3];      // Current.
    float pp[3];     // At the start of the substep (Verlet's previous).
    float rest[3];   // Mesh-local rest position, the rigid-follow target.
    float w;         // Inverse mass. 0 = pinned, 1 / mass everywhere else.
    // Compliance (N^-1 m) of the attachment holding this particle at its rigid-follow rest position: larger is looser,
    // ZERO = no attachment (not 'welded': a zero-initialised particle must not read as welded, and the only paint
    // weight the curve maps to zero compliance is the fully pinned one, whose inverse mass is zero and which the
    // attachment pass skips; a fully simulated vertex takes zero as the sentinel). It carries the whole pin/sim
    // gradient and inverse mass none of it: cloth_build.
    float att;
    // CLOTH long-range attachment: 'anchor' is the nearest pinned particle measured ALONG the mesh, 'tether' that
    // path's rest length. A per-edge stretch limit propagates one edge per pass and cannot hold a long chain in one
    // sweep; a bound straight to a fixed anchor enforces globally in one pass and is what stops a yanked cloth
    // becoming string.
    uint32_t anchor;
    float    tether;   // <= 0 = no anchor reachable (a cloth with no pins at all).
};

// One simulated face, carried for the wind and for self-collision (ClothSelfC::t): a sheet edge-on to the wind
// catches nothing, and without faces the solver has no orientation, only points.
struct ClothTri {
    uint32_t a, b, c;
};

// A distance constraint. Stretch edges and bend pairs share the type (only the compliance differs): one solver loop.
struct ClothDist {
    uint32_t a, b;
    float    rest;   // Metric rest length (m in the box the cloth steps in: the object's size, or a skeletal
                     // binding's or a chain's rest box; cloth_rest_lengths).
};

// Flat AABB BVH over a collider mesh's level-0 triangles. A rigid collider's: MESH-LOCAL normalized space (times its
// bake), built once per mesh id and bake shape (physics_bvh_for's key) and shared by every collider instance using
// both (only the transform differs). A skeletal binding's (SKEL, skin_run): its own, in metres about the pose
// box's centre - per pose a refitted copy of a template built once (PhysicsColliderView's SKEL note).
struct PhysicsBvhNode {
    float    mn[3];
    float    mx[3];
    uint32_t left;    // Interior: child index. Leaf: first triangle in tri[].
    uint32_t count;   // 0 = interior node.
};

struct PhysicsBvh {
    std::vector<PhysicsBvhNode> nodes;
    // Indices of the KEPT triangles into vp / pn (stream order), permuted by the build into the tree's leaf order.
    // Dropped: an index out of range, a non-finite corner, a triangle with no area.
    std::vector<uint32_t>       tri;
    std::vector<float>          vp;     // Triangle corner positions, 9 floats per triangle, tri-indexed.
    // Per triangle, 18 floats: the pseudonormals of corners a, b, c, then of edges ab, bc, ca (physics_bvh_build).
    // Same indexing as vp.
    std::vector<float>          pn;
    // SKEL: a DEFORMING collider's motion - each corner's displacement from the previous pose to this one, 9 floats
    // per triangle like vp, same space. A deforming view (PhysicsColliderView::deform) queries the corners at
    // vp + s * vd for s in [-1, 0], and the node boxes then hold both poses. Empty = rigid: readers take vp alone.
    std::vector<float>          vd;
    // COL_CONVEX: the mesh as convex PIECES (its position-welded connected components), built for every collider
    // and read for one the script marked convex or a pose refitted for the planes alone (cvx_only); a particle's
    // distance to a piece is the largest of its planes' (physics_convex_one). Built with the BVH
    // (physics_convex_build), refitted with it (physics_convex_refit); the planes are in vp's space.
    std::vector<uint32_t> cvx_ptri;    // Per kept triangle (vp order - the kept stream): its piece.
    std::vector<uint32_t> cvx_pstart;  // Per piece: its first plane; pieces + 1 entries.
    std::vector<uint32_t> cvx_tri;     // Per plane: the triangle it was taken from (vp index).
    std::vector<float>    cvx_pl;      // Per plane 4: unit outward normal n, offset d (n . x = d on it), this pose.
    std::vector<float>    cvx_pl0;     // ...at the previous pose (vp - vd); empty = rigid, as vd.
    std::vector<float>    cvx_pv;      // Per plane 3: its triangle's mean corner motion (vd); empty with cvx_pl0.
    std::vector<float>    cvx_sph;     // Per piece 4: bounding sphere centre and radius, both poses.
    // The pose was refitted for the planes alone (physics_bvh_refit's planes arm): the root box holds, the nodes
    // below it and pn are the template's. physics_collide_one takes the convex path on such a pose whatever the
    // view asks.
    bool cvx_only = false;
    // COL_CLEAR: this pose's identity (unique in the process, never 0 once built or refitted), the identity of the
    // pose vd sweeps FROM (0 = none: no vd, or a cut chain), and the pose's largest corner sweep |vd| in vp's units:
    // the bound on how far any surface point moves between two sweep fractions, or from the previous pose to this.
    uint32_t pose_seq = 0u;
    uint32_t prev_seq = 0u;
    float    vd_max = 0.0f;
    bool empty() const { return nodes.empty() || tri.empty(); }
    bool cvx_valid() const { return cvx_pstart.size() >= 2u && cvx_pl.size() == cvx_tri.size() * 4u && !cvx_tri.empty(); }
};

// A rigid frame: centre, rotation rows (engine axes, rows as basis vectors), edge lengths - exactly what cloth_l2w
// applies. Carries the cloth, and each collider.
struct ClothFrame {
    float centre[3];
    float rot[9];
    float size[3];
    bool  rotated;
};

// A collider as the sim thread sees it for one frame: the shared BVH plus this instance's transform, snapshotted by
// the game thread. The BVH lives in the collider mesh's normalized local space times 'bake' (physics_bvh_shape);
// l2w / w2l carry the rest of the size, the rotation and the centre. Every size here is in metres;
// physics_collide_one divides the bake out itself.
struct PhysicsColliderView {
    const PhysicsBvh* bvh;
    // The proportions the BVH was built at; zero = none (1: the plain normalized mesh).
    float bake[3];
    float rot[9];        // The collider object's rot_m, engine axes, rows as basis vectors.
    float centre[3];     // Centre: world in the gather's views, the cloth's own frame in its copies (cloth_sync).
    float res[3] = {};   // POS_RES: a world view's centre residual (engine axes); zero in the frame copies.
    float size[3];       // Engine-axes edge lengths; the normalized mesh spans [-0.5, 0.5].
    float inv_size[3];
    float bs_c[3];       // Bounding sphere, the broadphase - in the same frame as centre.
    float bs_r;
    float margin;        // Surface offset (m) added to the cloth's own thickness.
    float friction;      // The collider's half of the contact's Coulomb coefficient (0..1).
    bool  rotated;       // False = identity, skip the matrix entirely.
    bool  inside;        // True keeps cloth INSIDE the collider (a bag) instead of outside.
    bool  convex;        // COL_CONVEX: the script marked the collider convex (physics_convex_one).
    // The pose one step EARLIER: the previous frame's on the view the game thread hands a cloth, the substep start's
    // on the copy cloth_step makes per substep. All-zero (a size of zero) = none: the collider is taken as still.
    ClothFrame prev;
    uint32_t slot;       // The collider object's scene slot and mesh id: what matches a view to
    int      mesh;       // the one this cloth saw last frame.
    // SKEL: the BVH is a skeletal binding's skinned pose (skin_run), in metres about the pose box's centre,
    // which sits at skin_ctr in the model's authored frame (engine axes). cloth_sync reads skinned to move the
    // previous pose rigidly with the root (through root_off, below); the core reads skin_ctr alone, to put a
    // particle in the model's frame for COL_CLEAR (physics_collide_one).
    bool     skinned;
    float    skin_ctr[3];
    // SKEL: the BVH's own motion this frame (its vd) applies. def0 / def1: how much of it stands behind the
    // substep's start and end (the substep's fractions less one, cloth_step): the corners are at vp + def * vd.
    // Zero, and unread, on a view that does not deform.
    bool     deform;
    float    def0;
    float    def1;
    // The centre's velocity (m/s) over the cloth's last step, for cloth_sync's teleport prediction; zero on a view
    // seen for the first time. Game-side only - the core never reads it.
    float    vel[3];
    // SKIN_COL_PREV: where the centre sits from the object's root, in the model's frame (engine axes): a skinned
    // view's BVH box centre (skin_ctr), a skeletal binding's rest mesh's own centre (its skel_ctr), zero for anything
    // else. cloth_sync's previous-pose rebuild takes last frame's root from it. Game-side only.
    float    root_off[3] = { 0.0f, 0.0f, 0.0f };
};
struct ClothGround {
    bool on = false;
    float x0 = 0.0f, z0 = 0.0f;       // Node (0, 0): engine axes 0 (east) and 2 (north), the instance's frame.
    float cell = 1.0f;                // Node spacing (m).
    uint32_t nx = 0, nz = 0;          // Nodes east / north.
    uint32_t px = 0, pz = 0;          // Parity of node (0, 0)'s indices in the whole matrix: the split's class.
    // By (i & 1) | (j & 1) << 1 of a cell's absolute indices: 0 = the diagonal from (i, j) to (i + 1, j + 1), 1 = the
    // one from (i + 1, j) to (i, j + 1), 2 = bilinear.
    uint8_t split[4] = { 2, 2, 2, 2 };
    std::vector<float> h;             // nx * nz heights on the up axis, the instance's frame; row j = north.
};

// Per-object tuning: metric quantities or normalized 0..1 dials; no raw compliance value reaches the script surface.
struct ClothParams {
    float stretch;        // 0..1 dial; 1 = inextensible.
    float bend;           // 0..1 dial; 0 = rag, 1 = card.
    // 0..1 MATERIAL damping per CLOTH_H: removes that fraction of the rate at which each constraint stretches or
    // folds, nothing else. A rigid motion changes no constraint, so a swing, a spin and the carrier's own motion are
    // left to the air - 'drag'.
    float damping;
    // CLOTH_DRAG: the air's drag per point (kg/s) against its motion relative to the air, in full through its
    // faces and CLOTH_SKIN of it along them; acceleration = this / mass. cloth_substep.
    float drag;
    float mass;           // Particle mass (kg). Scales response to gravity vs constraints, and to the media's drag.
    float gravity;        // Multiplier on standard gravity; 0 = weightless.
    float max_stretch;    // Hard post-solve limit as a multiple of rest length (>= 1).
    float thickness;      // Cloth collision radius (m).
    float friction;       // Cloth-side half of the contact's Coulomb coefficient (0..1).
    // Per-particle speed clamp (m/s) against the CARRIER's motion: a particle faster than this in the world AND
    // relative to the carrier where it is is held to this much faster than the carrier (a flag on a vehicle travels
    // at the vehicle's speed; one the vehicle leaves is not pulled).
    float max_speed;
    // A carrier (or a collider, cloth_sync) whose centre lands further than this (m) from where its own velocity
    // would have taken it in one frame jumped; fast travel is motion.
    float teleport;
    // Collider and affector reach past the cloth's own extent (m; 0 reads as the default, 1): the larger of its rest
    // box and how far it actually reached at its last step.
    float range;
    float sleep;          // Mean particle speed under this parks the instance's velocity (m/s).
    // THICK CLOTH. A solidified mesh (front, back, rim) is two sheets to the position weld (front and back share no
    // edge but the rim). The largest gap (m) across which two OPPOSING surfaces are bound together, so a shell behaves
    // as one piece; set it a little above the mesh's own thickness. 0 = off: right, and free, for single-sided cloth.
    float shell;
    // Multiplier on the world wind the game reports; 0 = this object ignores the world's wind and the air around it is
    // still air, which still resists its motion. Wind acts through the drag (CLOTH_DRAG): a cloth edge-on to it is
    // barely pushed, and it scales as 1 / mass.
    float wind;
    // Upward acceleration underwater as a multiple of STANDARD gravity (not of the 'gravity' dial): 0 sinks exactly as
    // in air; at gravity 1, 1 is neutrally buoyant and holds its depth, above 1 rises. A cloth settles where buoyancy
    // balances gravity: 'gravity' divided by this is the fraction of CLOTH_WATER_BAND it floats at.
    float buoyancy;
    // CLOTH_DRAG: the water's drag per point (kg/s), the twin of 'drag' in still water, taken in over the same band
    // in proportion. Water is not air: a cape entering it slows hard rather than keeps swinging.
    float water_drag;
    // SELF-COLLISION (clothSimulation's selfCollision / selfThickness / selfFriction keys), off by default. On: no
    // particle comes nearer than self_thickness to a face of the same cloth, from the side it approached, and no edge
    // nearer than that to another edge - or, for a pair already nearer in the rest pose along the surface's normal,
    // nearer than it was there, so an authored fold or a shell's two surfaces keep their own gap. self_friction: the
    // Coulomb coefficient between the two layers; 'friction' is the cloth's half of a COLLIDER contact and is not read
    // here. cloth_self_gather.
    bool  self_collide;
    float self_thickness;   // Separation held between layers (m).
    float self_friction;    // Coulomb coefficient between layers (0..1).
    uint16_t iterations;  // Constraint iterations per substep.
    // Substeps per 1/60 s, NOT per frame: the substep length is fixed and must not follow the frame rate
    // (cloth_step); a slow frame runs more of them, a fast frame fewer. The backlog one step may simulate is bounded
    // in TIME (CLOTH_DT_MAX), never in substeps, so this dial sets how finely the solve runs and not the frame rate
    // below which the cloth runs slow: that floor is the stall clamp's, 10 fps, at every setting.
    uint16_t substeps;
};
struct ColClear {        // One particle against one collider column. d <= 0 = nothing known.
    float l[3];            // Where the particle stood, the collider's model frame.
    float d;               // Its distance from the surface then (the BVH's units), found outside.
    float acc;             // The column's surface-motion sum then.
};
struct ColClearCol {     // One collider column's pose chain.
    uint32_t seq = 0u;     // The pose last used...
    float    s = 0.0f;     // ...at this sweep fraction (def1; 0 = the pose itself)...
    float    vd_max = 0.0f;   // ...and its largest corner sweep.
    float    acc = 0.0f;   // Surface motion summed over the chain (the BVH's units).
    bool     on = false;   // The column's records may be read and written.
};

// ClothState (below): everything one instance needs to step, and nothing that resizes while a worker holds it. The
// particle array is sized once at build; 'stretch' is re-sized only by cloth_build_shell, on the game thread and
// only on an instance no worker holds; the solver's own arrays are sized on the worker. A reader off the worker sees
// a stale value at worst, never a freed one.
// The self-collision pairs (cloth_self_gather): a particle against a face of its own cloth, and an edge against an
// edge, that can reach each other this substep.
struct ClothSelfC {
    uint32_t q;      // The particle.
    uint32_t t;      // The face, an index into ClothState::tri.
    float    side;   // +1 / -1: the side of the face's plane the particle is on (started on, until it goes round).
    // The separation held (cloth_self_gather's sep): self_thickness; a pair within twice that at rest holds
    // its rest distance if nearer (a layer) or half its rest distance (a neighbour along the surface).
    float    sep;
    float    dep;    // Separation restored so far this substep (m) - the friction cone's normal term.
    float    s0;     // Its distance from the face's plane at the substep's start (m, >= 0).
};
struct ClothSelfEdge {
    uint32_t a, b;   // A surface edge's particles.
    uint32_t f;      // A face it borders, whose normal says which way the surface faces there.
};
struct ClothSelfE {
    uint32_t e0, e1;   // Indices into ClothState::self_edges, e0 < e1.
    float    n[3];     // Unit direction from e1's nearest point to e0's: the start's, until they go round.
    float    sep;
    float    dep;
    float    d0;       // The nearest points' distance at the substep's start (m).
};

struct ClothState {
    std::vector<ClothPart> part;
    // Mesh edges, then the shell links appended after them (cloth_build_shell). All distance constraints, so one
    // set serves both: the shell links take the same compliance, ride the same per-edge stretch limit (what bounds how
    // far a thickness may change) and join the tether graph, where a path through the shell is a real path.
    std::vector<ClothDist> stretch;
    std::vector<ClothDist> bend;      // Opposite corners across a shared edge.
    std::vector<ClothTri>  tri;       // Level-0 faces: the wind normal and self-collision's faces.
    // SUB_CLOCK: the carrier's frame at the SIMULATION'S time - where the last substep ended, short of the frame's
    // end by the substep accumulator's leftover (under one substep). The particles stand against it, the writeback is
    // relative to it and the next step's substeps run from it; fend is the frame's end (the step's input), which the
    // teleport, jolt and sleep tests read as a frame-to-frame motion. Equal when nothing is left over (60 / 120 fps at
    // the default two substeps).
    float    centre[3];
    float    rot[9];
    float    size[3];
    bool     rotated;
    ClothFrame fend;
    // SUB_CLOCK: the fraction of the last step's span (the frame above to its end) the substeps covered: 1 when
    // nothing was left over, 0 when none ran. cloth_sync places each collider's previous pose by it and
    // cloth_guide_prep the skeleton's, so both start where the simulation stands; chain_step places its own lanes
    // (the skeleton guide and the end's held position / rotation) by the same fraction (its cov, which it stores here).
    float    sub_t;
    bool     primed;                    // False until the first step has placed the particles.
    // Sea level on the up axis of the frame the particles live in: the core is frame-agnostic and this is its one
    // absolute fact; the runtime simulates about a floating origin and sets it. Zero = 'the frame is the world', the
    // right answer for a zero-initialised state.
    float    sea;
    float    accum;                     // Substep accumulator (s).
    // SIM_LOD: the substep length the particles' displacement (p - pp) was last integrated over; 0 = none yet. A
    // step at another length rescales the displacement first (cloth_step): the integrator reads it back as the
    // velocity over ONE substep.
    float    h_last;
    float    speed_mean;                // Mean particle speed at the last step (m/s), the sleep test.
    // XPBD Lagrange multipliers, one per constraint, reset at the start of every substep; sized there to their sets,
    // re-sized there whenever a set's size changes (a rebuilt shell changes stretch's). Worker-private: nothing
    // outside cloth_substep reads them, so they are not part of the published state.
    std::vector<float> lam_stretch;
    std::vector<float> lam_bend;
    std::vector<float> lam_att;
    // Wind's facing, rebuilt each substep: 7 floats per particle, the sum over the faces around it of
    // |n| nhat nhat^T (xx xy xz yy yz zz) and then of |n| - their ratio is the area-weighted mean of nhat nhat^T.
    std::vector<float> nrm;
    // SELF-COLLISION, rebuilt on every substep that has it on. Worker-private scratch, resized as the gather needs.
    std::vector<ClothSelfC> self_c;   // This substep's particle / face pairs.
    std::vector<ClothSelfE> self_e;   // This substep's edge / edge pairs.
    std::vector<ClothSelfEdge> self_edges;   // The surface's edges, built once per face set.
    uint32_t              self_edges_nt;   // The face count self_edges was built from; zero = never.
    std::vector<float>    self_rest;    // Metric rest positions, 3 per particle: the rest separations.
    float                 self_rest_size[3];   // The size self_rest was built at.
    float                 self_edge;    // Mean metric rest edge length, the hash cell's floor.
    std::vector<float>    self_box;     // Per face, then per edge: its box over the substep (6 floats).
    std::vector<uint32_t> self_wide;    // CLOTH_SELF_WIDE: this substep's wide elements (usually none).
    std::vector<uint32_t> self_head;    // Hash bucket starts, one past the table.
    std::vector<uint32_t> self_ent;     // Element indices (self_box's: faces, then edges), bucket by bucket.
    std::vector<uint8_t>  self_tang;    // Per particle: 1 = at an edge-through-face intersection at the substep's start.
    // CLOTH_FORK: the gather's queries run in chunks across the pool when the cloth is large (cloth_fork_run).
    // Per participant (the job's own worker and each helper): its own 'seen' stamps (per element, the query that last
    // met it, plus one) and tangle marks; per chunk: the pairs it listed, concatenated in chunk order afterwards.
    // Worker-private scratch; a helper touches only its own participant's and its chunk's entries.
    std::vector<std::vector<uint32_t>>     self_seen_par;
    std::vector<std::vector<uint8_t>>      self_tang_par;
    std::vector<std::vector<ClothSelfC>> self_c_par;
    std::vector<std::vector<ClothSelfE>> self_e_par;
    // The jolt stand-down (cloth_step): seconds left with self-collision off, and the velocity of each corner of
    // the carrier's box last frame.
    float                 self_hold;
    float                 self_v[8][3];
    bool                  self_vok;        // self_v holds a frame; false = no velocity yet.
    // The carrier centre's velocity over the last frame (m/s), for the teleport test's prediction (cloth_step);
    // carrier_vok false = none yet, which is the plain distance test. Recorded on a rebased frame too: otherwise a
    // carrier past the teleport distance per frame would rebase for ever, never having a velocity to predict with.
    float                 carrier_v[3];
    bool                  carrier_vok;
    // TELEPORT_ONCE: carrier_v was taken across a rebase (the jump's own travel), so the next test also takes no
    // jump from where the carrier stood.
    bool                  carrier_vreb;
    // How far (m) the farthest particle stood from the carrier's centre (the state's frame, SUB_CLOCK) at the end
    // of the last step, or 0 before one: cloth_sync's reach for colliders and affectors. A scalar, written by the
    // worker, read on a non-busy instance.
    float                 reach;
    // Worker-private: the per-substep collider views (cloth_step).
    std::vector<PhysicsColliderView> col_sub;
    // CLOTH_WARM: per collider (in col_sub's order) per particle, the triangle its last query hit
    // (CLOTH_WARM_NONE = none), and per collider the slot / mesh the column was filled for: a column whose collider
    // changed identity starts over. Worker-private.
    std::vector<uint32_t> col_warm;
    std::vector<uint32_t> col_warm_key;
    std::vector<ColClear>    col_clear;       // COL_CLEAR: col_warm's layout, a record per particle per column.
    std::vector<ColClearCol> col_clear_col;   // ...and each column's pose chain.
    // SKEL: a skeletal binding's guide for the rigid-follow targets (cloth_target), 3 floats per particle,
    // mesh-local like 'rest': where the skeleton put each particle at the simulation's time (a - the last substep's
    // end, SUB_CLOCK) and where it puts it at the frame's end (b). A simulated particle's entry is its rest
    // position: the bones move only what is pinned, wholly or in part. Both empty = no guide, every target 'rest'.
    // Filled on the worker ahead of the step (cloth_guide_prep). guide_t0 / guide_t1: the fractions of the step's
    // span (SUB_CLOCK: guide_a stands at the simulation's time) the running substep starts and ends at, so guide_t1
    // is where the frame in 'centre' stands (cloth_step sets both).
    std::vector<float>    guide_a;
    std::vector<float>    guide_b;
    float                 guide_t0;
    float                 guide_t1;
};
// INJ_FORK: fork_each (rendering/cloth_solver.hpp) runs fn(i) for every i in [0, n), in chunks across
// the pool with the caller helping (cloth_fork_run: one fork at a time; a fork already up, no workers, or a small
// n runs the plain loop). fn reads only what is frozen for the call and writes only its own index's slot; it takes no
// lock and touches no device object. Chunk order is unspecified, so nothing fn writes may depend on another index.
static constexpr uint32_t FORK_EACH_MIN = 512u;   // Below this the fork's wake costs more than it saves.

// AFFECTOR - PHYSICS AFFECTORS (addPhysicsAffector), the core's half: a scripted field that pushes cloth in one of
// three ways, which separates the types. AIR (wind, turbulence, vortex) moves the air: it adds to the world wind at the
// particle and acts through exactly that term - the drag (CLOTH_DRAG), through the particle's own facing, against
// its own motion, as 1 / mass, and not under water - so cloth edge-on to it is barely pushed, and the cloth's 'wind'
// dial (world wind only) does not scale it. FORCE (force, radial) accelerates every simulated particle alike, as
// gravity does, whatever its facing or mass. DRAG (drag) thickens the air: its density is 1 + the strengths summed,
// which scales all of the air's drag (CLOTH_DRAG) and none of the water's. Each is confined to an ellipsoid in its
// own axes (or acts everywhere), faded to zero across the outer 'edge' fraction of it, and scaled by its lifetime
// envelope. cloth_sync builds the views in the instance's own frame; the core only evaluates them, and with none
// the step is bit-identical to one without affectors.
static constexpr uint32_t AFF_MODES = 8u;       // Turbulence's Fourier modes.

// One affector as a cloth step sees it: engine axes, in the instance's frame. p: the type's parameters as
// aff_frame_build resolved them - wind: speed (m/s); turbulence: unused (its modes carry it); vortex: swirl, inflow,
// updraft (m/s), core (m, > 0); force and radial: strength (m/s^2); drag: strength (the air thickened by it,
// 0..AFF_DRAG_MAX: CLOTH_DRAG).
struct ClothAffView {
    uint8_t type;
    bool    bounded;                 // False = acts everywhere; inv_r unread.
    float   c[3];                    // Centre.
    float   ax[9];                   // Rows: right, forward (the script's vectorDir), up.
    float   inv_r[3];                // 1 / half-extent along each row.
    float   edge;                    // Outer fraction of the ellipsoid faded to zero (0 = hard edge).
    float   gain;                    // Lifetime envelope this frame (0..1).
    float   p[4];
    // Turbulence: each mode's wave vector (rad/m), amplitude (m/s, already perpendicular to the wave vector), angular
    // frequency (rad/s) and phase at the END of this frame (rad, wrapped to one turn).
    float   mk[AFF_MODES][3];
    float   ma[AFF_MODES][3];
    float   mw[AFF_MODES];
    float   mph[AFF_MODES];
};

// CHAIN - BONE-DRIVEN RIGID BODIES: a rope, a chain, an ammo belt. The mesh's own skeleton is the simulation: every
// bone from the chain's START bone down is a rigid body (the start itself kinematic, pinned where the carrier or the
// bound skeleton puts it), each joined to its parent at its own head, and the mesh is skinned from those bodies
// exactly as a skeletal binding skins from its proxies. What collides is the GEOMETRY: every level-0 vertex whose
// strongest influence is a simulated bone is a contact point on that bone's body, against the terrain and every
// physics collider (a flat belt lands flat, a chain drapes over what it touches). The END bone may be held too - its
// head at a position, its orientation at a rotation - each followed, fixed or free (ChainTarget).
// THE SOLVER: extended position-based dynamics for rigid bodies (Mueller, Macklin, Chentanez, Jeschke, Kim 2020). Per
// substep: integrate each free body; project the joints (position at the shared head; swing and twist against the
// rest orientation, each with its own stiffness and optional limit), the end's hold and the contacts - positional
// corrections applied at a point through each body's generalised inverse mass; take the velocities from the motion;
// damp the joints' relative spin. The substep is the cloth's, fixed at 1 / (60 * substeps) s, with the cloth's
// backlog and stall rules (cloth_step); the carrier, the skeleton and the colliders are interpolated across the
// substeps the same way.
// FRAMES: the rest pose lives in CARRIER-LOCAL metres, engine axes about the rest box's centre (the mesh's
// native_ctr), the object drawn at its authored size - a simulated chain ignores the size property, as a skeletal
// binding does, because the bones' lengths are the model's. Every body frame IS the carrier-local frame at rest, so
// the rest relative rotation of every joint is the identity. The state lives in the instance's floating-origin frame
// like the cloth's particles; a body's orientation is a unit quaternion (x, y, z, w) taking body vectors to that frame.
struct ChainParams {
    float stretch;        // 0..1: the joints' positional stiffness; 1 (the default) holds them exactly.
    float bend;           // 0..1: swing stiffness toward the rest angle; 0 = limp rope, 1 = rigid.
    float twist;          // 0..1: twist stiffness about each bone's axis.
    float damping;        // 0..1 of the joints' relative spin removed per CLOTH_H.
    // 0..1 of a body's motion RELATIVE TO ITS CARRIER removed per CLOTH_H: the loss 'damping' cannot give (a chain
    // swinging as one has almost no relative spin between its links). Relative to the carrier, not the world, so a
    // belt on a moving vehicle is damped toward travelling WITH it and never lags; on a still carrier the two are the
    // same. 0 = off, and the step is then bit-identical to one without these dials.
    float lin_damping;
    float ang_damping;    // ...and of its spin relative to the carrier's own turn.
    float drag;           // CLOTH_DRAG: kg/s, the whole chain's drag against motion relative to the air.
    float mass;           // kg, the whole simulated chain, shared by each bone's length.
    float gravity;        // Multiplier on standard gravity.
    float thickness;      // Contact offset (m) held between the geometry and the ground or a collider.
    float friction;       // The chain's half of a contact's Coulomb coefficient (0..1).
    float swing_limit;    // Degrees a joint may bend from its rest angle; 180 = no limit.
    float twist_limit;    // Degrees a joint may twist from its rest angle; 180 = no limit.
    float max_speed;      // m/s against the carrier, the cloth's guard (ClothParams::max_speed).
    float teleport;       // As the cloth's: a carrier or collider missing its predicted place by this (m) jumped.
    float range;          // As the cloth's: collider and affector reach past the chain's own (m; 0 reads as 1).
    float sleep;          // Mean body speed (m/s) under which a chain on a still carrier parks.
    float wind;           // Multiplier on the world wind the game reports.
    float buoyancy;       // Upward acceleration underwater as a multiple of standard gravity (the cloth's rule).
    float water_drag;     // CLOTH_DRAG: kg/s, the twin of 'drag' in still water.
    uint16_t iterations;  // Constraint passes per substep.
    uint16_t substeps;    // Substeps per 1/60 s, as the cloth's.
};

// One simulated bone. Rest members: carrier-local metres and, for the vectors that belong to the body, its own frame
// (the carrier-local axes at rest); state members: the instance's frame.
struct ChainBody {
    float x0[3];          // Rest centre of mass.
    float inv_m;          // 0 = kinematic (the start bone).
    float ib_inv[9];      // Inverse inertia about the centre of mass, body frame, symmetric.
    float ib[9];          // ...and the inertia itself (chain_joint_solve's mass block).
    float radius;         // Farthest contact point from the centre of mass (m): the broadphase.
    int32_t bone;         // MeshDef::skin_bones index.
    int32_t parent;       // Parent body, or -1 for the start.
    uint32_t s0, sn;      // Its contact points: ChainState::samp[s0 .. s0 + sn).
    float x[3], q[4];     // Now.
    float v[3], w[3];     // Linear (m/s) and angular (rad/s) velocity.
    float xp[3], qp[4];   // At the substep's start.
};
// A joint at the child's head: ra / rb the head from each body's centre of mass in its own frame; u the twist axis
// and e a vector across it, in the child's frame (the parent's too - the rest relative rotation is the identity).
struct ChainJoint {
    uint32_t a, b;
    float ra[3], rb[3];
    float u[3], e[3];
};
// What the end is held by this frame, in the instance's frame. Kinds: 0 free; 1 the bone's own place on the carrier
// (the bound skeleton's pose, or its authored place in the mesh's frame); 2 this position / these axes.
struct ChainTarget {
    uint8_t pos = 0;
    uint8_t rot = 0;
    bool    pos_hold = false;   // Kind 2 comes from the chain's own pose when it first stood (false).
    bool    rot_hold = false;
    float   p[3] = { 0.0f, 0.0f, 0.0f };
    float   r[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };   // Rows: the images of the body axes.
};
struct ChainState {
    std::vector<ChainBody>  body;    // Parents before children; body 0 is the start.
    std::vector<ChainJoint> joint;   // Parents before children, one per body but the start.
    std::vector<float>        samp;    // 3 per contact point: its offset from its body's centre of mass, body frame.
    std::vector<int32_t>      body_of; // Per bone: its body, or -1 (the bone follows the carrier or the skeleton).
    int32_t end_body = -1;             // The end bone's body, or -1 (none named and none implied).
    // CHAIN_POINT - which point of the START and END bones the chain is anchored and held at (the script's
    // startPoint / endPoint). A bone's HEAD is its own origin; its TAIL is the far end - its first simulated child's
    // head, or, on a tip bone with no child to end it, its own geometry's reach down the joint's axis.
    // end_pt: the point the end's hold acts on, in the END body's frame. The default is that bone's head, which IS
    // joint[end_body - 1].rb.
    float   end_pt[3] = { 0.0f, 0.0f, 0.0f };
    // start_off: 3 per body, the carrier-local translation the REST placement takes, so the pose the joints want is
    // the pose the reset lays down. Empty with startPoint "tail" (the default), where the anchor already IS the rest
    // pose's. With "head" the free subtree is placed one start bone shorter; without start_off the first step would
    // close that gap as a snap and read it back as a frame of velocity.
    std::vector<float> start_off;
    float   auth_ctr[3] = { 0.0f, 0.0f, 0.0f };   // MeshDef::native_ctr: carrier-local = authored - this.
    // Skinning, immutable after the build: the worker reads these, never the MeshDef.
    std::vector<SkinInf> inf;        // Per render vertex, or empty.
    std::vector<uint8_t>   lvl0;       // Per render vertex: 1 = level 0 draws it (the box), or empty = all.
    float ext[3] = { 1.0f, 1.0f, 1.0f };   // MeshDef::native_ext: a stored position times this is carrier-local.
    // The skeleton's pose where the last step's substeps ended (skin_affine's layout; SUB_CLOCK: its frame's end
    // when nothing was left over), the start of this step's interpolation. Empty = none.
    std::vector<float> guide_prev;
    // GUIDE_END: the pose at the last step's frame END, as given (bit for bit) - the held_moved test's reference:
    // guide_prev is a blend at a leftover (not bit-exact even for a still pose), so a comparison against it would
    // read a still skeleton as moving. Empty = none.
    std::vector<float> guide_end;
    // The end's targets where the last step's substeps ended (SUB_CLOCK, as above), the start of this step's
    // interpolation of a kind-2 target (the kind it was, so a change is not swept).
    uint8_t prev_pos = 0, prev_rot = 0;
    float   prev_p[3] = { 0.0f, 0.0f, 0.0f };
    float   prev_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // A 'false' target: the end's pose when the chain first stood, held for the instance's life (the floating origin
    // carries it: cloth_origin_shift).
    bool    hold_on = false;
    float   hold_p[3] = { 0.0f, 0.0f, 0.0f };
    float   hold_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // How far the end's held point can get from where the chain hangs off the start (the bone lengths along the path,
    // plus the end bone's own length when endPoint holds its tail - CHAIN_POINT), and the body whose joint is that
    // pin. A position target further away cannot be met together with the joints (both are rigid rows) and the solve
    // would alternate between them forever; clamping the target to this radius gives a taut chain instead. The path
    // exists when end_root > 0 (CHAIN_REACH_ZERO): its reach may be 0 - an end bone hanging straight off the start,
    // held at its head, IS the pin.
    float   end_reach = 0.0f;
    int32_t end_root = -1;
    // The end's APPROACH: where a hold engages away from where the end stands (a target set across the map, a bone
    // the chain cannot reach), the gap is closed at CHAIN_SEEK_MS / CHAIN_SEEK_RAD rather than in one substep (a
    // correction taken in one substep is read back as that distance over one substep of velocity). seek_p / seek_q:
    // what is still to close, added to the target, shrinking every substep.
    bool    seek_p_on = false;
    bool    seek_q_on = false;
    float   seek_p[3] = { 0.0f, 0.0f, 0.0f };
    float   seek_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // Worker-private: 6 multipliers per joint then 6 for the end's hold, reset per substep.
    std::vector<float> lam;
    // Which of each joint's three rotation rows stood at its limit on the last pass (3 per joint): a row's multiplier
    // belongs to the constraint it was accumulated against, so it is dropped whenever that changes.
    std::vector<uint8_t> lam_lim;
    // Worker-private: the direct solve's scratch (chain_joint_solve).
    std::vector<double> solve_wk;
    std::vector<int32_t> solve_meta;
    std::vector<int32_t> solve_at;
    float    mass_built = 0.0f;        // The 'mass' the bodies were distributed at.
    // CHAIN_RUNAWAY: seconds the rigid joints have stood open, less the steps they stood closed (chain_step).
    float    runaway_t = 0.0f;
};
}