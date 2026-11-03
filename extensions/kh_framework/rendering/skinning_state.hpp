#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of SKIN_GPU /
// SKEL: skinning on the CPU and the GPU (stream output), the skin pools, the sync and upload. Never include it on
// its own.
namespace RenderIntegration {

// SKEL: the skinning jobs run on the skin pool (POOL_SPLIT), which shares the solver pool's ownership protocol
// (busy) and the unload path's stop. Their queue is guarded by g_cloth_mu with the cloth's; the skin pool waits on its
// own condition (g_skin_cv). skin_job (defined in rendering/skinning.hpp) clears its own busy on every path out.
struct SkinInst;
// getRenderStats: the skins of the last complete cycle, by path - the GPU's (skinGpu) and the pool's (skinCpu). A
// cycle's steps each skin what they must (the cast fire's most bindings, the injection's the few that waited or
// changed), so the counts are summed over the cycle and published when the next cycle's first skinning step arrives;
// a binding skinned twice in a cycle counts twice. Render thread (only a step handed the context skins).
// SKIN_CULL: and the steps' culls (skinCulled; a binding culled at several steps of a cycle counts at each).
static std::atomic<uint32_t> g_skin_so_n{ 0 }, g_skin_cpu_n{ 0 }, g_skin_cull_n{ 0 };
static uint64_t g_skin_stat_cycle = ~0ull;
static uint32_t g_skin_stat_gpu = 0u, g_skin_stat_cpu = 0u, g_skin_stat_cull = 0u;

// SKIN_NRM_COF - a skinned normal by the inverse transpose of the blended map, for both linear-blend skins
// (skin_verts, chain_skin_vert). r: the blended map B's three rows in the palette's layout (row c =
// (B[c], B[3 + c], B[6 + c]) of skin_affine_of's 12 floats: sum w M over the influences, the root's share on the
// diagonal); nrm: the authored normal. nn (out): n . cof(B) - the inverse transpose up to the factor det(B) -
// turned by det(B)'s sign and scaled to the length of the rows of the blend, n . B (sum w n M + rest n reassociated),
// which every reader normalises. It stays perpendicular to every tangent the same map carries (the covariant tt);
// n . B does not under a non-rigid blend (two rotations' mean, a scale). Neither sees the weights' gradient, the true
// surface Jacobian's other term. A cofactor of squared length outside (1e-30, 1e30), or an n . B of squared length
// 1e30 or more, gives n . B itself: a zero map, a rank-one one whose cofactor rounds to 0 (a rounded one's cofactor
// is rounding noise and passes - its triangles have no area), a full-rank map scaled past ~3e7 or below ~3e-8, most
// non-finite ones. The result is finite wherever n . B is.
// TWIN: KhSkinNrmCof (skin.hlsl), term for term.
static constexpr float SKIN_NRM_LC_MIN = 1.0e-30f;
static constexpr float SKIN_NRM_L_MAX = 1.0e30f;
static std::deque<std::shared_ptr<SkinInst>> g_skin_q;   // Under g_cloth_mu. Oldest first (POOL_FIFO).
// SKEL: the collider BVH of each skeletal binding's installed skinned result, by creation stamp, with the box it was
// stored in. Rebuilt by skin_upload, read through a copy by physics_gather_colliders. Pending motion is a stamp,
// not a flag: uploads may run several times per frame or not at all.
struct SkinCol {
    int mesh = -1;
    float ctr[3] = { 0.0f, 0.0f, 0.0f };    // skel_ctr: engine axes, metres.
    float size[3] = { 1.0f, 1.0f, 1.0f };   // SQF order, as RenderObject::size.
    std::shared_ptr<const PhysicsBvh> bvh;
    uint64_t serial = 0;   // The install stamp of this pose (SkinGpu::serial); 0 = none.
};
using SkinColMap = std::unordered_map<uint64_t, SkinCol>;
static std::shared_ptr<const SkinColMap> g_skin_col;   // Under g_skin_col_mu; a published table is never written again.
// Published by skin_upload on the flush's thread, copied by the cloth sync
// on the game thread: every access holds this mutex.
static std::mutex g_skin_col_mu;
// SKIN_COL_PUB - skin_upload runs at any point of the game thread's frame, so the table, the object boxes and the
// freshness stamp must change together: the upload builds the table aside and publishes it, with the highest stamp it
// can hold, in the g_draw_list_mutex scope that writes the boxes; the cloth sync copies the table and stamp in the
// scope that copies the objects; and a stamp is fresh for exactly the gather after it was published - the window (lo,
// hi], advanced at every cloth sync.
static uint64_t g_skin_col_stamp = 0;   // skin_upload's thread: the last stamp handed out.
static uint64_t g_skin_col_pub = 0;     // Under g_skin_col_mu: the highest stamp the published table can hold.
static uint64_t g_skin_col_lo = 0, g_skin_col_hi = 0;   // Game thread: this frame's fresh stamps are (lo, hi].
static std::shared_ptr<const SkinColMap> g_skin_col_snap;   // Game thread: the table as the cloth sync's object copy saw it.
static constexpr uint64_t SKIN_STEP_OWN = 8u;   // Cycles a step's skin keeps the buffer from the worker.
// SKIN_GPU - the pose's box without the pose's vertices. A linear-blend vertex is a convex combination of its
// bones' rigid images of it (and of itself, for the weight left to the root), so the box of every bone's image of
// ITS vertices (the drawn vertices it carries weight on) contains the skinned mesh, and equals its exact box
// wherever the extreme vertex follows one bone (a hand, a foot, a head). Where a BLENDED vertex is the extreme the
// bound exceeds the exact box by at most that vertex's swing between its bones' images of it, and is never smaller than
// the exact box; skinBoundWorst in getRenderStats (armed) is the largest ratio measured against a worker's exact box
// for the same pose. Each bone's vertices are a small k-d tree (authored metres); the extreme of a bone's image along a
// world axis is the support of its point set along that axis taken back through the bone's map, an exact
// branch-and-bound query. Up to six per bone per pose, on the render thread (SKIN_BOUND_PRUNE skips a bone whose
// root box cannot move the extreme): tens of microseconds a pose at 20k-60k vertices and 100-300 bones (-O2; 2-25x
// less than the hundreds of microseconds querying every bone costs).
// The vertices whose weight to the root is more than SKIN_SO_ROOT_W stand in the box at rest (id_*); less than
// that moves a vertex by under a third of a millimetre while its bones' images stay within about 3 m of its rest
// place (as a body's do), which SKIN_SO_PAD_M covers with the float slack of a weight sum a hair over one. Built
// once per mesh by skin_rest_of (game thread), immutable after.
static constexpr float    SKIN_SO_ROOT_W = 1.0e-4f;
static constexpr float    SKIN_SO_PAD_M = 0.001f;
static constexpr uint32_t SKIN_SUP_LEAF = 8u;
struct SkinSupNode {
    float    mn[3], mx[3];
    uint32_t left;    // Interior: the first child (the second is left + 1).
    uint32_t first;   // Leaf: its first point...
    uint32_t count;   // ...and how many; 0 = interior.
};
struct SkinSup {
    std::vector<float>         pt;     // Points, 3 floats each, in node order.
    std::vector<SkinSupNode> node;
    std::vector<uint32_t>      root;   // Per bone: its tree's root node, or 0xFFFFFFFF (it carries no drawn vertex).
    // SKIN_SO_PAL: the rest stream's palette - the bones some vertex's weight names (skin_sup_prepare), in
    // bone order. Row j of CBSkin is bone pal[j]'s map; the stream stores j. Its size is what KH_SKIN_SO_BONES caps.
    std::vector<uint32_t>      pal;
    float id_mn[3] = { 0.0f, 0.0f, 0.0f }, id_mx[3] = { 0.0f, 0.0f, 0.0f };
    bool  id_any = false;
    bool  ok = false;
};
// A mesh's rest data as the jobs need it, copied once per mesh on the game thread and immutable after: the workers
// read this and never the MeshDef, whose release the park owns. Shared by every instance of the mesh and held by a
// job for as long as it runs.
struct SkinRest {
    std::vector<MeshVertex> verts;
    std::vector<SkinInf> inf;   // Parallel to verts, or empty.
    // LOD_SKIN: 1 for a vertex some LOD level draws; the box and the bound's trees are taken over these alone.
    // Every level skins (a decimated vertex carries the influences its collapses blended) and a binding at range
    // draws a coarser level, so the box holds every level's vertices, not level 0's alone. Empty = count every
    // vertex (the rule: the weld leaves no vertex no level draws). SKIN_LVL0 (retired - it skinned level 0's prefix
    // alone on the GPU for a locked binding): do not skin a prefix alone on any path - the lock is the script's to
    // clear, and a draw after a clear in the same cycle reads the tail's older bytes.
    std::vector<uint8_t> drawn;
    std::vector<uint32_t> idx0;   // Level 0's triangle list: the faces a collider's BVH is built from.
    float ctr[3] = { 0.0f, 0.0f, 0.0f };   // MeshDef::native_ctr / native_ext.
    float ext[3] = { 1.0f, 1.0f, 1.0f };
    SkinSup sup;   // SKIN_GPU: the bound's trees (ok = built; the GPU path stands down without them).
    // SKIN_GPU: per bone, the rest box of the vertices it carries (mn xyz, mx xyz, authored metres; mn > mx =
    // none) - the palette shape key's reach.
    std::vector<float> bone_box;
};
struct SkinInst {
    // GAME THREAD ONLY.
    uint64_t seq = 0;
    uint32_t slot = 0xFFFFFFFFu;
    int mesh = -1;
    uint32_t skel_gen = 0;
    std::shared_ptr<const SkinRest> rest_draw;   // SKEL_DRAW: the rest snapshot, for the consume-point skinning.
    std::vector<SkinXf> pose;        // Per proxy: the last read that passed. Never read = the rest.
    std::vector<uint8_t> pose_ok;
    std::vector<SkinXf> done;        // The pose last handed to a job, and its flags.
    std::vector<uint8_t> done_ok;
    bool dirty = true;                 // The next job must run whatever the pose says.
    int32_t drive_mesh = -1;           // The (mesh, skel_gen) the drive table was built for.
    uint32_t drive_gen = 0;
    std::vector<int32_t> drive;        // Per bone: the matched bone whose proxy moves it, or -1 (the root).
    std::vector<int32_t> drive_px;     // Per bone: that proxy's index, or -1.
    uint32_t bones_driven = 0;         // Bones whose own name matched a memory point.
    bool seen = false;
    bool cloth = false;                // The cloth draws this mesh (CLOTH above); set by skin_sync.
    std::vector<float> guide;          // That cloth's pose, skin_affine's layout, this frame's.
    bool want_collide = false;         // The object is a physics collider (COLLIDER above).
    bool done_collide = false;         // ...as it was when the last job was handed over.
    bool want_convex = false;          // COL_CONVEX: ...marked convex (the pose refits its planes alone).
    bool done_convex = false;
    // The next collider pose must not be swept from the last one: the pose left the chain (a new skeleton or mesh,
    // a proxy's first good read) since the last job. Handed over as in_cut. A distance per pose cannot tell a fast
    // limb from a snap (it depends on the frame rate), so the chain is cut only where the code knows it broke.
    bool cut = true;
    // THE JOB'S INPUTS: written by the game thread only while busy is false.
    std::shared_ptr<const SkinRest> in_rest;
    std::vector<float> in_a;           // Per bone: rows then translation, v' = v * M + T.
    bool in_collide = false;           // Build the pose's collider BVH too.
    bool in_convex = false;            // COL_CONVEX: ...for its planes alone.
    bool in_cut = false;               // This pose carries no motion (cut above).
    // SKIN_GPU: the job skins the drawn vertices (out). False while the render thread's step owns the buffer
    // (step_cycle_pub within SKIN_STEP_OWN cycles): skin_upload leaves such a buffer's bytes and box to the
    // step, so out would be thrown away. The job then computes the positions alone for a collider, and nothing for
    // a mesh that is not one.
    bool in_verts = true;
    // WORKER-PRIVATE.
    std::vector<float> met;            // The skinned positions, metres, authored frame.
    std::vector<float> shape_ref;      // Those positions when the current shape key was taken.
    uint32_t shape_key = 0;
    // COLLIDER: the rest pose's BVH and its topology record, built on the first collider job; every pose after is a
    // copy refitted to it.
    std::shared_ptr<const PhysicsBvh> col_tpl;
    PhysicsBvhTopo col_topo;
    std::vector<float> col_scr;
    std::vector<float> col_prev;       // The last collider job's positions (met), the next one's motion base.
    uint32_t col_pose_seq = 0u;        // COL_CLEAR: the last collider pose's identity (PhysicsBvh::pose_seq), the next one's prev_seq.
    // PUBLISHED: buffer b, its box and its key are written by the worker before front's release store, and read by
    // the upload after its acquire load.
    std::vector<MeshVertex> out[2];
    float box_ctr[2][3] = {};          // The box out[b] is stored in: engine axes, metres.
    float box_size[2][3] = {};
    uint32_t key_of[2] = { 0u, 0u };
    uint32_t gen_of[2] = { 0u, 0u };   // SKIN_GEN_OF: the gen published with out[b] (skin_upload installs it).
    bool out_ok[2] = { false, false };   // SKIN_GPU: out[b] holds this result's vertices (the job ran with in_verts).
    std::shared_ptr<const PhysicsBvh> bvh_of[2];   // out[b]'s collider BVH, or none (COLLIDER above).
    std::atomic<uint32_t> front{ 0 };
    std::atomic<uint32_t> gen{ 0 };    // 0 = nothing published yet; else from g_skin_gen_serial.
    std::atomic<bool> busy{ false };
    // SKIN_GPU: the cycle (g_topo_cycles) the render thread's step last wrote this binding's buffer
    // (skin_draw_finish, either path), for the game thread's in_verts; and the upload's request for a full job
    // (it needed the worker's vertices - no step within SKIN_STEP_OWN cycles - and the front result had none).
    std::atomic<uint64_t> step_cycle_pub{ ~0ull };
    std::atomic<bool> need_full{ false };
    // SKIN_HIDDEN_COL - the object is hidden (skin_sync writes, the upload reads; SKIN_CASTER: a
    // casterOnly one that casts is not - the maps draw it; SKIN_1X_HIDDEN: at FSAA 1x every binding is). A hidden
    // mesh that is not drawn and no collider asks for no pose at all (one hidden by 1x alone still asks, its job
    // taking no vertices). A hidden COLLIDER (an invisible collision body, the usual way to give a cloth a body)
    // still needs its positions and BVH every frame; no step skins a mesh that is not drawn (nor a culled one's
    // SKIN_CULL bytes), so it is never 'owned', and without this rule its worker skinned the full vertices and
    // the flush uploaded the whole array every frame for bytes nothing draws (five bodies: skinUploadUs 13 -> 127).
    // Hidden, its job is positions-only once its buffer holds a pose, and the upload takes the result's record
    // (gen, key, box, BVH) and leaves the bytes. Shown again, the first visible step skins the buffer before the
    // cycle's draw and the next job is a full one; a park's late draw in that one frame (with the PIP, the drawers
    // no ctx step precedes) shows the pose the buffer last held in the box of the record last taken, which the
    // hidden frames moved on, so it is stretched by the difference for that one frame.
    std::atomic<bool> hidden{ false };
};
// SKIN_MAP - written by the game thread (skin_sync, skin_drop_all) and read on the render thread by
// skin_upload and by skin_guide_of (cloth_upload via cloth_pin_fresh): the map's structure and the
// instance fields those reads take (slot, cloth, want_collide, seq, mesh; seen and guide for skin_guide_of) are
// written under g_skin_mu, and both iterate under it. Lock order: g_draw_list_mutex -> g_skin_mu,
// g_cloth_mu -> g_skin_mu, g_skin_mu -> g_skin_col_mu; nothing takes g_cloth_mu or g_draw_list_mutex under
// g_skin_mu. The game thread's own reads need no lock.
static std::unordered_map<std::string, std::shared_ptr<SkinInst>> g_skin;
static std::unordered_map<int, std::shared_ptr<const SkinRest>> g_skin_rest;   // Game thread only.
// A published result's gen, unique across every instance a handle ever has (32-bit: until it wraps), so a rebuilt
// instance does not repeat a value its handle's buffer holds.
static std::atomic<uint32_t> g_skin_gen_serial{ 0 };
static std::atomic<uint32_t> g_skin_skipped{ 0 };   // Changed poses that waited on a job still running.
static std::atomic<uint32_t> g_skin_pool_full{ 0 };   // skinPoolFull: the workers' full CPU skins (armed only).
struct SkinGpu {
    ID3D11Buffer* vb = nullptr;
    UINT bytes = 0;
    uint32_t gen = 0;      // The published result the buffer holds...
    uint32_t key = 0;      // ...its shape key...
    float box_ctr[3] = { 0.0f, 0.0f, 0.0f };   // ...and its box.
    float box_size[3] = { 1.0f, 1.0f, 1.0f };
    std::shared_ptr<const PhysicsBvh> bvh;     // ...and its collider BVH, if it was built with one.
    uint64_t serial = 0;   // The stamp when gen was first installed (a re-upload keeps it).
    // SKEL_DRAW: the cycle (g_topo_cycles) whose consume-point step last wrote vb (skin_draw_finish). The
    // park's install leaves vb alone while this is the cycle in flight: on a park between the cycle's first step
    // and its injection, writing the worker's older pose here would hand the late draw and the injection a pose
    // the sun ladder did not cast, and the two drawers a pose apart; the put leaves the object's box alone with it
    // (the vertices are normalised to their box). gen / key / box / bvh still take the worker's result (the collider
    // and the shape key are its consumers, the draw is not). ~0 = never stepped, or vb replaced (SKIN_GPU_MOVE
    // keeps it: the step's bytes are copied into the new buffer, by_step and step_key with them).
    uint64_t step_cycle = ~0ull;
    uint32_t step_key = 0u;   // SKEL_SHAPE: the shape key of the bytes the step wrote (with by_step).
    // Whose bytes vb holds: true from the step's Unmap until the park's next Map. The put follows THIS, not
    // step_cycle: a cycle the render thread did not step, with no new gen to install, leaves the step's bytes in vb,
    // and the worker's box over them would deform the late draw.
    bool by_step = false;
    bool valid = false;
    // SKIN_GPU: vb is a DEFAULT buffer with the stream-output bind (the GPU skins into it); every CPU write to it
    // is an UpdateSubresource, not a Map.
    bool so = false;
    // SKIN_GPU_MOVE: the cycle the device refused this binding a stream-output buffer (its pool slice and one of
    // its own) - its DYNAMIC buffer is offered one again SKIN_GPU_RETRY cycles on (with no buffer at all, the
    // DYNAMIC one refused too, the first install asks again every flush). ~0 = not refused.
    uint64_t so_refused = ~0ull;
    // SKIN_POOL: vb is this pool chunk (not owned - the slice is), off the slice's byte offset in it (bytes
    // long); null = vb is the binding's own, off 0. Every reader takes (vb, off): skin_gpu_ref.
    SkinPoolChunk* chunk = nullptr;
    uint32_t slice = 0u;
    UINT off = 0u;
};
static std::unordered_map<std::string, SkinGpu> g_skin_gpu;   // Under g_skin_mu.
static std::mutex g_skin_mu;

// SKIN_GPU - the skeletal draw, skinned on the GPU. The consume-point step (attach_step) hands the GPU a
// palette (skin_so_batch: the per-bone maps the step builds, three float4 a bone) and draws the mesh's REST
// STREAM through VSSkinSo (skin.hlsl) with stream output into the binding's buffer. What every pass binds, and every
// shader reads, is the binding's buffer of MeshVertex in the pose's own box - SKIN_POOL: that buffer is the
// binding's slice of its mesh's skin pool chunk (SkinPoolChunk), bound at its offset (VbRef), and the instanced
// routes read the same bytes through the chunk's view (the Sk twins). The box is the trees' bound
// (skin_so_bound), the shape key the palette's (skin_so_shape_note).
// The worker path stays for what reads it: a collider's positions and BVH (positions alone while the step owns the
// buffer - SkinInst::in_verts), the full vertices whenever no step has written the buffer lately (a binding's
// first result, a span the render thread does not flush in - FSAA 1x excepted, which draws none of them:
// SKIN_1X_HIDDEN), so a collider switched on or off never touches the drawn bytes.
// STANDS DOWN, per binding, to the pool's CPU skin when: the objects below could not be made (reported once;
// SKIN_SO_RETRY: a memory refusal is asked again SKIN_GPU_RETRY cycles on and a new device is asked at once,
// any other failure stands until a new device or the session's end), the mesh has more than KH_SKIN_SO_BONES
// weighted bones (SKIN_SO_BIG) or no influences, or the device refused the binding a stream-output buffer or its
// mesh's rest stream - SKIN_GPU_MOVE: a refusal is asked again SKIN_GPU_RETRY cycles on, and a CPU-skinned
// buffer then moves to stream output with its bytes (skin_gpu_move).
// SKIN_GPU_ON false compiles the path out of use.
static constexpr bool     SKIN_GPU_ON = true;
// SKIN_SO_BIG: the bones the GPU path takes are the mesh's palette (SkinSup::pal, SKIN_SO_PAL: the bones a
// weight names, not the unweighted ancestors skin_bones also holds), in one of two classes: up to
// SKIN_SO_BONES_S, VSSkinSo as skin.hlsl declares it (CBSkin 770 float4, the 12,320-byte Map); up to
// KH_SKIN_SO_BONES, VSSkinSo compiled with SKIN_SO_DEFS_L (CBSkin 4094 float4 - (4096 - 2) / 3 bones, the most one
// cbuffer holds). A bigger palette stays on the CPU path. HLSL twin: skin.hlsl's KH_SKIN_SO_BONES (its default is the
// small class; the large class's text is KH_SKIN_SO_BONES_L_TXT). The two, CBSkin's ByteWidths and the refusal sites
// (skin_rest_gpu_ensure, skin_gpu_movable, skin_upload's reserve and first install, skin_so_target)
// change as one. SKIN_SO_LAZY: the large class's objects are made on demand, apart from the small class's
// (skin_so_big_ensure); until they stand a palette above SKIN_SO_BONES_S stays on the CPU path.
static constexpr uint32_t SKIN_SO_BONES_S = 256u;
static constexpr uint32_t KH_SKIN_SO_BONES = 1364u;
#define KH_SKIN_SO_BONES_L_TXT "1364"
static const D3D_SHADER_MACRO SKIN_SO_DEFS_L[] = {
    { "KH_SKIN_SO_BONES", KH_SKIN_SO_BONES_L_TXT }, { nullptr, nullptr } };
constexpr uint32_t skin_so_dec(const char* t, uint32_t v = 0u) {
    return *t ? skin_so_dec(t + 1, v * 10u + static_cast<uint32_t>(*t - '0')) : v;
}
static_assert(skin_so_dec(KH_SKIN_SO_BONES_L_TXT) == KH_SKIN_SO_BONES, "the large class's define is its count");
static_assert(2u + KH_SKIN_SO_BONES * 3u <= 4096u, "CBSkin's large class fits one cbuffer");
static_assert(SKIN_SO_BONES_S < KH_SKIN_SO_BONES, "the small class is the smaller");
// SKIN_GPU_MOVE: the cycles (g_topo_cycles) before a stream-output buffer or a rest stream the device refused is
// asked for again - about two seconds at 60 fps, so a device short of memory is not asked every frame.
static constexpr uint64_t SKIN_GPU_RETRY = 120u;
static std::atomic<bool>     g_skin_so_failed{ false };   // The objects could not be made (skinGpuOff).
// SKIN_SO_RETRY: the cycle (g_topo_cycles) the device refused memory for the objects while g_skin_so_failed stands -
// asked again SKIN_GPU_RETRY cycles on, as a refused buffer is. ~0 = not asked again: any other failure (a compile
// error, an invalid argument) stands until skin_release_all (a new device) or the session reset. Written by
// skin_so_ensure on the flush's thread and by those two; read there.
static uint64_t g_skin_so_retry_at = ~0ull;
// SKIN_SO_LAZY: the large class's objects (g_res.skin_so_vs_l / _gs_l / _cb_l), made apart from skin_so_ensure's small
// class and only when some binding's palette needs them (above SKIN_SO_BONES_S, at most KH_SKIN_SO_BONES): a session
// with no such skeleton never makes them (the arming prewarm still queues the unit's compile, PREWARM_ALL). Never
// compiled here while that prewarm is in flight or not yet armed (g_shbatch_state != 2): compile_shader would take the
// batch's not-yet-taken job and compile it on the flush's thread (about 2 s under fxc on a cold cache) - such an ask is
// "not now", with no latch and no stat; the binding keeps the CPU path and SKIN_GPU_MOVE moves it to stream output at
// the first flush the class is ready (skin_so_class_ok turns true in skin_gpu_movable). After the batch the unit is a
// memo or a disk hit (a later batch's finish releases the memo - ShaderMtScope's composite batch, keep false - and the
// disk serves it), except where the pool refused or had nothing to do (shader_async_gate sets state 2 with no memo: the
// first ask compiles inline), or the memo was released and the cache folder is unwritable: a cold compile then, on the
// flush's thread - never a draw hook. Its own failure latch, so a refusal here never stands the small class down: a
// memory refusal is asked again SKIN_GPU_RETRY cycles on, any other failure stands until skin_release_all (a new
// device) or the session reset. Flush's thread (skin_upload), holding no lock; made by skin_so_big_ensure (after
// skin_so_ensure).
static bool     g_skin_so_big_failed = false;
static uint64_t g_skin_so_big_retry_at = ~0ull;
// getRenderStats (armed only): the largest bound-to-exact-box edge ratio a worker measured (skin_run, for a
// pose it has the positions of), as float bits (positive floats order as their bits do).
static std::atomic<uint32_t> g_skin_bound_worst_bits{ 0 };
// The rest stream's vertex (HLSL twin VSInSkin): skin_verts' authored inputs, computed once.
struct SkinSoVert { float p[3]; float n[3]; float uv[2]; float t[4]; uint32_t b[4]; float w[4]; };
static_assert(sizeof(SkinSoVert) == 80u, "skin.hlsl's VSInSkin is 80 bytes");
static_assert(sizeof(MeshVertex) == 48u, "VSSkinSo streams out one MeshVertex");
struct SkinPool {
    std::vector<std::unique_ptr<SkinPoolChunk>> chunks;
    // A refused repack: the slice count it was refused at and the cycle (g_topo_cycles) - not retried at that size
    // for SKIN_POOL_TRIM_RETRY cycles.
    uint64_t trim_refused = 0u;
    uint64_t trim_refused_at = 0u;
};
static constexpr uint64_t SKIN_POOL_TRIM_RETRY = 600u;
static std::unordered_map<uint64_t, SkinPool> g_skin_pool;   // (mesh << 32) | slice bytes. Under g_skin_mu.
static uint32_t g_skin_pool_serial = 0u;      // Under g_skin_mu.
static bool     g_skin_pool_failed = false;   // Under g_skin_mu; cleared with the device (skin_release_all).
// SKIN_POOL_MADE: the device has made a pool buffer (skin_pool_buffer) - it takes the kind, so a later refusal
// is one take's (memory, a slice past its limits), not a reason to stop pooling. Under g_skin_mu; cleared with the
// device, as g_skin_pool_failed is.
static bool     g_skin_pool_made = false;
// 64 MB: within D3D11's guaranteed resource size (128 MB), and a slice's base vertex (offset / 48) stays far
// below 2^24, exact in the record's float lane.
static constexpr UINT     SKIN_POOL_CHUNK_BYTES = 64u << 20;
// A mesh's rest stream on the device, by snapshot. Under g_skin_mu; made and swept by skin_upload, released
// with the device (skin_release_all).
struct SkinRestGpu {
    std::shared_ptr<const SkinRest> rest;   // Keeps the key's address its own.
    ID3D11Buffer* vb = nullptr;
    UINT nv = 0;
    bool seen = false;
    uint64_t fail_at = ~0ull;   // SKIN_GPU_MOVE: the cycle its create failed (~0 = it has not).
};
static std::unordered_map<const SkinRest*, SkinRestGpu> g_skin_rest_gpu;
// SKIN_GPU (skin_so_batch, rendering/skinning.hpp) - the GPU skinning of attach_step and skin_cull_catchup:
// every binding of the step whose buffer takes stream output, in one state bracket. Under g_draw_list_mutex (the
// caller's): the render thread, from a step that was handed the context, or the catch-up (render thread, or the park).
// Per binding: the pose's bound from the trees (into the job's bc / bs), the palette into CBSkin,
// the mesh's rest stream drawn as a point list through VSSkinSo with the binding's buffer as the stream-output target -
// one MeshVertex per vertex, in order, from the target's offset (SKIN_POOL: the binding's slice) - and so_vb set. A
// binding it cannot take (no stream-output buffer, no rest stream, a palette that does not fit, a Map that fails) is
// left for the pool, untouched.
// THE BRACKET: StateBackup holds everything the pass sets that it captures (layout, topology, IA slot 0, VS / GS / PS /
// HS / DS, VS b0); the stream-output target it does not capture is saved and handed back here, slot 0 only -
// SOSetTargets(1, ...) leaves slots 1-3 null (offset -1: an engine target, were there ever one, keeps appending).
// g_ro.in_injection is raised for the pass in its own scope (its render-thread callers raise it too, STEP_OWN_FLAG,
// but the pass does not rely on them), so the palette's Map is not read as an engine upload by the scanners and the
// draw is not tracked as an engine draw; the draw goes to the original entry when the hook is installed. Rasterisation
// is off in the stream-output object itself (D3D11_SO_NO_RASTERIZED_STREAM), so no target, depth or blend state is
// touched or read.
// One binding's buffer (SKIN_POOL: the pair), its mesh's rest stream and vertex count.
struct SbTarget { VbRef vb; ID3D11Buffer* rest; UINT nv; };
}