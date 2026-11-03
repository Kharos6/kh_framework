#pragma once

// The shared head (rendering_integration.hpp): the cloth / chain instances and the chain config (ClothInst,
// ChainCfg), the ground patch and the skin pool chunk. Never include it on its own.
namespace RenderIntegration {
struct SkinPoolChunk;   // SKIN_POOL (defined below; the pool itself is in rendering/skinning_state.hpp).
inline ChainParams chain_defaults();   // Defined in rendering/chain_solver.hpp.

// One simulated instance. Owned by shared_ptr so a worker holding a job keeps it alive past the script removing the
// mesh.
struct ClothInst {
    ClothState st;
    // Render vertex -> particle. Many-to-one: a UV or smoothing seam splits a render vertex without splitting the
    // cloth (otherwise the sheet tears along every seam).
    std::vector<uint32_t> vmap;
    std::vector<MeshVertex> rest;      // The mesh's own vertices, the fallback and the LOD tail.
    std::vector<MeshVertex> out[2];    // Double buffered deformed array.
    std::vector<float> nrm_acc;        // Normal accumulator, 3 per render vertex.
    std::vector<float> tan_acc;        // Tangent accumulator, 3 per render vertex (cloth_writeback).
    // Render vertex -> the render vertex whose accumulator it shares: the first one on the same particle with the
    // same authored normal (cloth_build). Written at build, read by the writeback.
    std::vector<uint32_t> nrm_rep;
    // Build-time only, deliberately not in ClothPart (which the solver touches every substep): the authored normal
    // of each particle, 3 per particle, used once to decide which surfaces face each other.
    std::vector<float> part_nrm;
    // SKEL, written at build. part_inf: each particle's four bone influences, from the first render vertex on it
    // (every render vertex at one position carries the same ones); empty when the mesh has no vertex groups.
    // auth_ctr / auth_ext: the mesh's native_ctr / native_ext, so a stored rest position r was authored at
    // r * auth_ext + auth_ctr.
    std::vector<SkinInf> part_inf;
    float auth_ctr[3] = { 0.0f, 0.0f, 0.0f };
    float auth_ext[3] = { 1.0f, 1.0f, 1.0f };
    uint32_t mesh_edges = 0;           // Stretch entries that are real mesh edges; shell links follow.
    float    shell_built = -1.0f;      // The 'shell' the current links were built for.
    // CLOTH_OWN_IDX: level 0's index stream, copied at cloth_build - the writeback (a pool worker) reads this
    // and never the MeshDef, as the skin pool reads SkinRest and a chain its own inf / lvl0. Empty for a chain.
    std::vector<uint32_t> lvl0_idx;
    int      mesh = -1;
    uint32_t slot = 0xFFFFFFFFu;
    // The object this instance simulates, by its creation stamp (RenderObject::seq, unique per object within a
    // mission and carried through every update). Slot and mesh alone do not name an object: a slot is reused, and a
    // new object on it with the same mesh would take over the old drape.
    uint64_t seq = 0;
    // Written by the game thread under g_cloth_mu (the upload reads it under the same lock). Dead, but a worker
    // still held it when cloth_sync came to retire it: never stepped, never installed in a slot again, and erased
    // the first frame it is free. Also set on the erase itself (CLOTH_ERASE_RETIRE), for the upload's snapshot.
    bool retired = false;
    // THE FLOATING ORIGIN (engine axes, metres, double): the particles, the carrier frame and the colliders' views
    // are all stored RELATIVE to it. Game thread only, on a non-busy instance (cloth_sync).
    double org[3] = { 0.0, 0.0, 0.0 };
    bool   org_on = false;
    ClothParams par = cloth_defaults();

    // The frame's inputs, written by the game thread while 'busy' is false.
    float in_centre[3] = {};
    float in_rot[9] = {};
    bool  in_rotated = false;
    float in_size[3] = { 1.0f, 1.0f, 1.0f };
    float in_dt = 0.0f;
    float in_wind[3] = {};
    // Every collider in reach, however many.
    std::vector<PhysicsColliderView> in_col;
    // The BVHs the views point at, held for the duration of the step so a collider mesh released mid-step cannot
    // free the geometry under a worker.
    std::vector<std::shared_ptr<const PhysicsBvh>> in_col_own;
    // AFFECTOR: the affectors in reach this frame, already in this instance's frame, written with the inputs.
    std::vector<ClothAffView> in_aff;
    // Game thread only: the frame time that passed while a worker still held this instance, owed to its next step -
    // a skipped frame is late, not lost.
    float dt_owed = 0.0f;
    // CLOTH_GROUND: the terrain under everything this cloth can reach this frame, written with the inputs
    // (cloth_ground_patch).
    ClothGround in_ground;
    // SKEL: the skeleton's pose, one affine map per bone in skin_affine's layout (empty = no guide), and
    // whether the object is a skeletal binding: one steps in its REST box at the model's origin (cloth_sync) and
    // has its buffers written in the box of the drawn shape (cloth_box_out).
    std::vector<float> in_guide;
    bool in_boxed = false;
    // SIM_LOD (game thread, cloth_sync, written with the inputs): the iterations and substeps this step runs
    // at - the object's own dials scaled by the LOD share - and, standing between frames, the hysteresis's memory of
    // the last choice. 0 = not chosen yet (the job takes the dials themselves). Read by cloth_run.
    uint16_t in_iter = 0;
    uint16_t in_sub = 0;

    std::atomic<bool>     busy{ false };
    // CHAIN_END_RACE: a worker took this job off the queue (set under g_cloth_mu at the pop) and is writing its
    // back buffers; cleared after the publish. busy is also true while the job only waits in the queue.
    std::atomic<bool>     running{ false };
    std::atomic<uint32_t> front{ 0 };   // Which out[] the upload may read.
    std::atomic<uint32_t> gen{ 0 };     // Bumped on every publish; the upload's dirty test.
    uint32_t uploaded_gen = 0xFFFFFFFFu;
    // CLOTH_SHAPE: the shape key of each out[] buffer (cloth_shape_note) and of the one the upload last wrote,
    // which is what g_cloth_gen_slot publishes. shape_of[b] is written by the worker before front's release store
    // and read by the upload after its acquire load, like out[b].
    uint32_t shape_of[2] = { 0u, 0u };
    uint32_t uploaded_shape = 0u;
    // SKEL: when box_on[b], out[b] is stored in the box box_ctr[b] (its centre in the authored frame) /
    // box_size[b] (edge lengths), engine axes, metres; otherwise in the frame's own rest box. Written by the worker
    // before front's release store and copied by the upload with the buffer, like shape_of.
    bool  box_on[2] = { false, false };
    float box_ctr[2][3] = {};
    float box_size[2][3] = {};
    bool  uploaded_box_on = false;
    float uploaded_box_ctr[3] = {};
    float uploaded_box_size[3] = {};
    // CAST_REFIT (a plain instance, neither boxed nor a chain): the edge lengths out[b]'s level-0 vertices reach
    // about the centre, in the frame's own space (engine axes like st.size, metres, >= the frame's size). Published
    // as box_* are; the upload hands the object the uploaded buffer's.
    float cast_size[2][3] = {};
    float uploaded_cast_size[3] = {};
    // Worker-private: a boxed instance's writeback target in the frame's own space, which out[] is then mapped from.
    std::vector<MeshVertex> sim_out;
    // Worker-private: the mesh-local positions (3 per render vertex) of the shape that took the current key, and
    // that key.
    std::vector<float> shape_ref;
    uint32_t shape_key = 0u;
    // Created by cloth_upload (flush_locked's housekeeping: the render thread's scene flush, or the game thread
    // under a park); retired to g_cloth_grave by cloth_sync; released by the upload's grave drain or
    // cloth_release_all. Never touched by a worker.
    ID3D11Buffer* vb = nullptr;
    bool built = false;
    // CHAIN: this instance is a chain (chain_build), not a cloth. It keeps the carrier's frame, the substep
    // clock and the counters in st, which the shared plumbing reads (cloth_sync, cloth_ground_patch,
    // cloth_origin_shift, cloth_upload), and its bodies in ch. cpar and in_tg are inputs like the rest
    // (written while busy is false); ch_start / ch_end are the bone names it was built for, so new ones rebuild it.
    // sim_met is worker-private: the skinned positions, carrier-local metres.
    bool chain = false;
    ChainState ch;
    ChainParams cpar = chain_defaults();
    ChainTarget in_tg;
    std::string ch_start;
    std::string ch_end;
    bool ch_start_head = false;   // CHAIN_POINT: the anchor and hold points it
    bool ch_end_tail = false;     // was built for; a change rebuilds it too.
    std::vector<float> sim_met;
    // CLOTH_PIN_FRESH - what the guide alone decides (the cloth's hard pins, the chain's kinematic bodies and
    // guide-mapped bones) is re-placed from this frame's guide into the front buffer's frame on every upload, whether
    // or not a job landed, so the bound part never lags a slow simulation. ch_mat[b]: the per-bone maps
    // chain_writeback skinned out[b] from (12 floats a bone), written with out[b]. pin_out / pin_mat: scratch for
    // whichever re-places the bound part (cloth_upload or the consume-point step), under g_cloth_mu. pin_guide:
    // the upload's copy of the Draw3D guide.
    // CLOTH_PIN_STEP - pin_step_cycle: the cycle whose consume-point step re-placed the bound part and wrote vb;
    // the upload leaves that cycle's bytes, key and box standing.
    uint64_t pin_step_cycle = ~0ull;
    uint32_t pin_front = 0;   // CHAIN_END_DRAW: the out[] buffer that step placed (its pin_mat's).
    // CHAIN_END_DRAW - per out[] buffer, written by the writeback before front's release like box_*: the end's
    // position and rotation targets the job held, carrier-local (false: that lane had none). end_mat / end_out /
    // end_met: chain_end_draw's scratch; end_shape_ref / end_shape_key: the shape that took the key of the bytes
    // it writes (CLOTH_SHAPE's rule, cloth_step_shape). Render thread, under g_cloth_mu.
    bool  end_job_p[2] = { false, false };
    bool  end_job_r[2] = { false, false };
    float end_job_x[2][3] = {};
    float end_job_q[2][4] = {};
    std::vector<float>      end_mat;
    std::vector<MeshVertex> end_out;
    std::vector<float>      end_met;
    std::vector<float>      end_shape_ref;
    uint32_t                end_shape_key = 0u;
    // The same targets in the world (engine axes; a rotation as its quaternion), for an end fixed there: its frame
    // target is where the job held it (CHE_WORLD, CHE_HOLD). end_anc: chain_end_fix's scratch.
    double end_job_w[2][3] = {};
    float  end_job_wq[2][4] = {};
    std::vector<int32_t>    end_anc;
    // CHAIN_END_DRAW (C3) - vb_wr counts every write of vb (cloth_upload, cloth_pin_draw_step,
    // chain_end_draw); the draw's last write: vb_wr then (end_wr), the buffer, front and generation, the bone
    // maps it skinned and the front's box - equal again, with no write between, the bytes would be the same, and
    // the draw keeps them (end_be: the box they were stored in). Render thread, under g_cloth_mu.
    uint32_t                vb_wr = 0u;
    uint32_t                end_wr = 0u;
    bool                    end_wr_ok = false;
    ID3D11Buffer*           end_vb = nullptr;
    uint32_t                end_f = 0u, end_gen = 0u;
    std::vector<float>      end_mat_last;
    float                   end_box[6] = {};
    float                   end_be[3] = {};
    // CHAIN_END_NOTE: the cycle (g_topo_cycles) of the draw's last write - kept bytes written in this one
    // are still news to a final or repick step (chain_end_draw's note).
    uint64_t                end_new_cyc = ~0ull;
    // CLOTH_STEP_SHAPE - pin_g: the guide cloth_pin_place last re-placed a chain from (chain_end_draw reads
    // the pin step's: an end held on a bone). pin_met: cloth_pin_seal's scratch; pin_shape_ref / pin_shape_key: the
    // shape that took the key of the re-placement's bytes. Under g_cloth_mu (render thread, or a park).
    std::vector<float>      pin_g;
    std::vector<float>      pin_met;
    std::vector<float>      pin_shape_ref;
    uint32_t                pin_shape_key = 0u;
    // CLOTH_PIN_GRADE - out_guide[b]: the guide the job solved out[b] against (its in_guide, copied with out[b]).
    // Without it the re-placement is binary - a hard pin takes this frame's bones, a particle painted 2 % sim keeps the
    // pose an older guide gave it - while the solver's gradient is continuous, so the cloth tears along the pin
    // boundary by however far the bones moved since the job's guide (a frame or more: the job is asynchronous, and
    // SIM_LOD spaces its steps). With it cloth_pin_place carries a gradient particle along by (1 - its paint
    // weight) of what its OWN target moved since - 1 at the pinned end, 0 at the simulated end, so it meets both
    // neighbours. Empty or of another bone count = no grading.
    // CLOTH_HELD_REC: what the job solved against is the guide as the step HELD it - SUB_CLOCK's substeps end
    // sub_t of the way across the frame, the pins swept from the last step's held guide to this frame's
    // (cloth_guide_prep's lane a, cloth_target) - so out_guide[b] is guide_held, the bone matrices blended the
    // same way (cloth_guide_at is affine in them: the held pin positions exactly), kept by the job across steps;
    // the frame's end as given fell (1 - sub_t) of the bones' travel short at frame rates whose frames do not divide
    // into substeps (90 / 144 fps; none at 60 / 120). The chain's twin is c.guide_prev (CHAIN_HELD_REC).
    std::vector<float> out_guide[2];
    std::vector<float> guide_held;   // Job thread only.
    std::vector<float> ch_mat[2];
    std::vector<MeshVertex> pin_out;
    std::vector<float> pin_guide;
    std::vector<float> pin_mat;
};

static std::mutex g_cloth_mu;                                            // Guards the table and the queue.
static std::unordered_map<uint32_t, std::shared_ptr<ClothInst>> g_cloth;   // By scene slot.

// Slot -> substitute vertex buffer, consulted by every draw pass. Resized only by the uploads in flush_locked's
// housekeeping (cloth_upload, skin_upload, uvs_upload); elements written only there, by
// cloth_release_all, or by cloth_slot_forget (scene_sync, uvs_release_all) - never while a render-thread
// reader is mid-read. A buffer is released and its element cleared in the same window.
static std::vector<ID3D11Buffer*> g_cloth_vb_slot;
// SKIN_POOL: some element of g_cloth_pool_slot may name a chunk - skin_upload sets it after its installs,
// skin_release_all and the session clear it; under the tables' rules. The depth routes' tie-break and test
// (cast_pool_key, cast_pool_of) skip the tables without one.
static bool g_skin_pool_any = false;
// The published SHAPE KEY per slot (CLOTH_SHAPE; 0 = no cloth), beside the buffer and under the same lifetime
// rules - except that the render thread's consume-point writers (skin_draw_finish, cloth_pin_draw_step,
// chain_end_draw, uvs_step) also write an element, the key following the bytes they wrote. A cloth instance's
// BUFFER POINTER never changes while its CONTENTS change every frame, so any pass that caches on "has this draw
// changed" has to hash this and not the pointer - otherwise a moving cloth keeps a footprint or a shadow map
// rendered from a shape it has left. The key moves only when the drawn shape does, so a cloth at rest costs those
// passes nothing. Hashed by volume_seam_inject, render_sun_depth and dls_render; uvs_upload keys a vertex
// stage's source on it (r.src_key).
static std::vector<uint32_t> g_cloth_gen_slot;

// CHAIN - what the script set with chainSimulation, by handle. GAME THREAD ONLY: written by the command, read by
// chain_prepare, cloth_sync and the game thread's other lookups (chain_end_publish, chain_refused,
// chain_nobuild_prune, skel_bone_names, crew_chain_*), never by the render thread. It lives here rather
// than in RenderObject because it holds game objects - an end that follows an object or a memory point - and a
// RenderObject is copied to the render thread, where no game_value may be copied; the one bit the render thread needs,
// RenderObject::chain_sim, is there. Like the cloth's parameters it OUTLIVES the simulation (switching the chain off
// and on keeps the tuning and the end), and it dies with its object: chain_prepare drops an entry whose handle is
// gone or names another object, and hands its proxy to the delete queue; a mission's end clears the table
// (reset_retained_state).
//
// The END's two lanes are independent, as a mesh's position and rotation are. pos: FREE (not held), BONE (true: the
// end bone's place on the carrier - its memory point on a bound skeleton, else its authored place in the mesh's
// frame), HOLD (false: where it stood when the chain started), WORLD ([x, y, zASL]), OBJ (an object's position), MEM
// ([object, memoryPoint], through a ATTACH_BONE proxy, read as a mesh attached to one is). rot: the same kinds;
// true resolves at the command to the rotation of whatever the position follows (MEM, OBJ) or BONE.
enum ChainEndKind : uint8_t { CHE_FREE = 0, CHE_BONE, CHE_HOLD, CHE_WORLD, CHE_OBJ, CHE_MEM };
struct ChainRead { uint32_t off = KH_ATTACH_OFF_NONE; uintptr_t vb = 0; uintptr_t bb = 0; uintptr_t avb = 0; uintptr_t abb = 0; };
struct ChainCfg {
    uint64_t seq = 0;                   // The object this entry belongs to (RenderObject::seq).
    ChainParams par = chain_defaults();
    std::string start;                  // Lower-case bone names; "" = the default (ChainState).
    std::string end;
    // CHAIN_POINT - which point of each named bone the chain uses. The start's ANCHOR: false (default) = its
    // tail, the first simulated bone's head; true = the start bone's own head. The end's HELD point: false (default)
    // = its head, the joint anchor; true = its tail.
    bool start_head = false;
    bool end_tail = false;
    uint8_t pos = CHE_FREE;
    uint8_t rot = CHE_FREE;
    bool rot_true = false;              // rot was given as true: resolved against pos (chain_cfg_resolve).
    float pos_w[3] = { 0.0f, 0.0f, 0.0f };   // WORLD: SQF [x, y, zASL].
    float rot_w[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };   // WORLD: rot_m rows, engine axes.
    game_value pos_obj;                 // OBJ: the object. MEM: the memory point's owner.
    game_value proxy;                   // MEM: the proxy on the memory point; ours to delete.
    game_value rot_obj;                 // rot OBJ: the object.
    ChainRead rd_pos, rd_rot, rd_par; // Raw-read caches (attach_raw).
    // The last transform each followed lane read, and whether it has read one. A raw read can refuse for a frame
    // (the page gate, a basis that fails the checks) while the object is alive; the lane then HOLDS this rather than
    // going free, as attach_raw's other callers do with a refused read - a socketed end must not fall out of its
    // chute for one frame and creep back at the approach rate. A lane re-pointed at something else drops it, and a dead
    // object still frees the lane.
    bool   read_p = false, read_r = false;
    double last_pos[3] = { 0.0, 0.0, 0.0 };
    float  last_rot[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // This frame's reads (chain_prepare): world, engine axes.
    bool   live_p = false;
    bool   live_r = false;
    double live_pos[3] = { 0.0, 0.0, 0.0 };
    float  live_rot[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // CHAIN_REFUSED_END: the chain builds nothing - its mesh, loaded, carries no skeleton (chain_mesh_ok), or
    // its build was refused (chain_refused). Written by chain_prepare's pass ahead of chain_end_publish,
    // its one reader, in the same pass. Its ends are still read there: the lanes' hold (read_p /
    // last_pos) stays current for the frame the chain builds again.
    bool   nobuild = false;
};
// EXIT_STATICS: never destructed (g_attach's rule): the configs hold SQF references (ChainCfg's game_values),
// and a quit that skips mission_ended (Alt+F4, the window closed from Windows) would release them at the DLL's
// static teardown, after the engine is gone. The mission edges clear it (reset_retained_state).
static std::unordered_map<std::string, ChainCfg>& g_chain_cfg = *(new std::unordered_map<std::string, ChainCfg>());

// The world wind, ENGINE axes (east, up, north), sampled once per Draw3D by cloth_wind_sample and read by the
// sync. One frame of age is not worth a second engine call: wind is a slow field and the cloth it drives is already
// published a frame behind the draw.
static float g_cloth_wind[3] = { 0.0f, 0.0f, 0.0f };

// CLOTH_GROUND - the terrain the cloth lands on. The source is the framework's terrain matrix (g_terrain_matrix,
// built by initialize_terrain_matrix, which thm_autobuild_step calls from flush_frame ahead of the cloth step, on
// a mission's first frame with anything to draw): the engine's own height at every terrain-grid node, rows north and
// columns east from the map's corner, every g_terrain_grid_width metres. Between the nodes the engine's ground is
// two flat triangles per cell, split along one diagonal or the other; which one is MEASURED here, once per matrix
// and mission, by asking the engine for the height at the centre of a few cells whose two diagonals disagree (the
// centre lies on both, and only one answer is the engine's). The four parity classes of a cell are measured apart, so a
// checkerboard or row-alternating split is found as well as a uniform one. A class that answers neither way falls back
// to a smooth (bilinear) fit and says so once. Game thread only; the workers see only each cloth's own copy of the
// nodes under it. The heightfield our meshes are clipped against takes the same split (THM_EXACT:
// thm_autobuild_step measures it here too, with or without a cloth).
static bool     g_ground_ready = false;              // A matrix and its split are known.
static float    g_ground_cell = 0.0f;                // The matrix measured: its spacing...
static size_t   g_ground_n = 0;                      // ...and its nodes per side.
static uint8_t  g_ground_split[4] = { 2, 2, 2, 2 };  // ClothGround::split, by class.

// SKEL runtime - a skeletal mesh's vertices, skinned from its parent's memory-point proxies and published through
// the cloth's slot tables, so no pass or shader knows skinning exists.
//
// THE MODEL. Everything is in the parent's frame (engine axes, metres, the parent's visual origin). A proxy carries
// its bone's animated transform relative to the parent: rotation Q and position t. A bone matching a proxy's memory
// point maps v to (v - head) * Q + t (identity at rest: the mesh's bones sit on the memory points). An unmatched
// bone takes its nearest matched ancestor's map (or the root's identity). Vertices blend four influences (linear
// blend skinning); missing weight goes to the root.
//
// THREADS - the cloth's protocol on the cloth's pool. skin_sync (game thread) reads the proxies under
// g_draw_list_mutex, builds one affine map per bone and hands the instance to a worker ('busy'). The worker
// (skin_job) skins into the back buffer from the instance's immutable rest snapshot, writes that buffer's box,
// shape key and gen (gen_of), release-stores front and gen, and clears busy last. skin_upload acquires gen
// (anything new?), then front, and installs that buffer with its own box, key and gen_of - a gen read just before a
// job's front store names the result before it (SKIN_GEN_OF). One job per instance at most, and the upload walks
// with g_cloth_mu held, under which every job leaves the queue, so no job can start on the buffer it reads (the one
// a running job writes is the other). g_skin_gpu is under g_skin_mu (the engine-reset release can run on the render
// thread).
//
// THE BOX. A skinned buffer is stored in its own pose's box, and the object's size and skel_ctr are set only where
// the buffer is installed (the upload and the consume-point steps).
//
// NORMALS. n / native_ext is the authored normal; written back as n * box it is what KhVsCore's normalize(n / size)
// expects. It is skinned by the blended map's inverse transpose (SKIN_NRM_COF: skin_nrm_cof, skin.hlsl's twin).
//
// CLOTH. A binding whose mesh simulates is the cloth's to draw: no skinning job runs; skin_sync keeps its pose as
// per-bone maps (SkinInst::guide) for cloth_sync, whose bones place the pinned part. skin_upload hands the
// object the box of whichever buffer its slot ends up with.
//
// COLLIDER. The job that skins a pose also builds its BVH (physics_bvh_build, metres about the pose's box centre)
// and publishes it with the buffer; skin_upload records the installed result's BVH (g_skin_col) for
// physics_gather_colliders. Each pose carries its corners' motion since the previous pose (PhysicsBvh::vd; none
// when the chain was cut), stamped at its first install (SkinGpu::serial), so exactly the next gather sweeps it
// across its substeps. The sweep is discrete: a limb moving more than half its thickness in one substep can tunnel.
// A hidden collider still collides.
static constexpr float SKEL_REACH_M = 500.0f;   // A proxy farther than this from its parent is not yet attached.

// SKIN_POOL - where the GPU-skinned poses live: per (mesh, slice size) a pool of chunks, each one DEFAULT buffer
// (VERTEX_BUFFER | STREAM_OUTPUT | SHADER_RESOURCE, with a R32G32B32A32_UINT view - three elements a MeshVertex) of
// 'used.size()' equal slices, one per binding (SkinGpu::chunk / slice / off). The GPU skins into the slice (stream
// output at its offset), the CPU writes it with a box, every pass binds it with the IA offset, and the instanced
// routes read it through the view at the base the record carries (KhObjRec::res.w) - many bindings' poses, one
// buffer, one draw. The memory follows the population within a factor: a flush's new slices are reserved at once
// (skin_pool_reserve: the last chunk grown to fit, then whole chunks); a take past them grows the last chunk in
// place, doubling (skin_pool_grow: a new buffer, the old one copied to its start, so no slice moves) up to
// SKIN_POOL_CHUNK_BYTES (or one slice when a slice is larger), then a new one-slice chunk starts; a pool whose
// live slices fall to a quarter of its slices is repacked into half that (skin_pool_trim: fresh chunks, every
// slice copied, every base new); a chunk goes when its last slice does, or at the next trim when nothing took it.
// So a pool holds between one and four times its live slices (rounded up to a whole chunk), and a mesh's bindings
// share as few chunks - instanced draws - as 64 MB allows. A binding whose mesh changes at the same byte size leaves
// its slice (skin_upload), so a chunk holds one mesh's poses. Only a stream-output buffer is pooled (the CPU's
// DYNAMIC one never is); a chunk whose buffer or view the device refuses leaves the binding a buffer of its own for
// as long as that buffer stands (the reservation counts it as housed: it rejoins a pool only once the buffer goes -
// its binding ends or changes byte size, the device or the session goes) - and when the device has made no pool
// buffer yet, it refuses the kind itself, which stops further pooling until the device goes (SKIN_POOL_MADE: a
// refusal after one stood is that take's alone); a refused growth or repack keeps what stands. Under g_skin_mu;
// created, grown, repacked and released only by skin_upload (the flush's housekeeping, after cloth_upload
// emptied the slot tables - a growth there re-points the elements the walk installed already: skin_pool_rebind)
// and skin_release_all (beside cloth_release_all), so no table element names a released buffer.
struct SkinPoolChunk {
    ID3D11Buffer* vb = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    uint64_t key = 0u;                  // Its pool's (g_skin_pool).
    uint32_t serial = 0u;               // Unique for the session and never 0: the instanced routes' key lane.
    std::vector<uint8_t> used;          // Per slice.
    uint32_t live = 0u;                 // Slices in use.
};
}