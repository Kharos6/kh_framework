#pragma once

// The shared head (rendering_integration.hpp): the attachment records (KhAttach, KhGtSnap, KhSkinDrawJob) and g_attach.
// Never include it on its own.
namespace RenderIntegration {
struct KhSkinInst;
struct KhSkinRest;   // KH_SKIN_CULL: KhAttach::cull_rest (defined with the skin in rendering/skinning_state.hpp).
struct KhAttach;
#define KH_ATTACH_OFF_NONE  0xFFFFFFFFu
struct KhSkinXf { float q[9]; float t[3]; };   // A proxy against its parent: rows Q, position t (KH_SKEL; defined here for KhAttach's scratch).
// KH_ATTACH_GT_SNAP - ONE game-thread sample of a binding: everything it follows, read TOGETHER from the object's
// chosen copy at Draw3D, the instant the engine's own drawn state is coherent (measured: a helper attached to a bone
// reads there exactly where the engine has it for the frame being drawn, standing and moving alike). It holds under
// KH_PXY_HIDE, which changes whether a helper is SUBMITTED, not when the engine refreshes it (beside its parent either
// way: KH_PXY_HIDE's numbers). Do not make a render-thread read at the cycle's first step the pose's source: the
// engine refreshes an attached child DURING the frame, after that step - 54 % of such reads changed again before the
// injection (up to 76 mm), which a parent-relative placement cancels while the unit travels and shows in full when a
// bone moves against a standing parent (the in-place animation lag). The render thread reads the objects itself only
// to stand in for a frame's own sample that has not landed (KH_RT_PAINT_READ).
//   par  = the object the binding is placed RELATIVE to: the parent of a skeletal or memory-point binding, the
//          followed object itself for a plain one (and for a binding that follows a rotation alone, that object -
//          kh_attach_parent_gv). 12 floats: position (SQF order), then the basis rows.
//   px   = the helpers, the same 12 floats each: a skeletal binding's proxies in order, or a memory-point
//          binding's one proxy (a refused one carried from the previous sample - KH_GTS_HOLD). Empty for a plain
//          binding.
//   rot  = the ROTATION lane's own object, when it is neither par nor the one helper (a script's own object);
//          rot_of names that object (its base, KH_ATTACH_ROT_OF): the step takes rot only while the rotation lane
//          still follows it, so a lane re-pointed since the sample never turns with the old object.
// gen / xb name the binding the sample was taken for (a skeletal binding's skel_gen; the position lane's object, or
// the rotation lane's where there is no position lane - kh_attach_key_gv), so a re-bind cannot be served a sample of
// what it followed before. seq / qpc order the ring and place it against the cycle's depth clear. Written by the game
// thread and read by the render thread's step, both under g_draw_list_mutex.
static constexpr uint32_t KH_GTS_N = 4u;   // Samples kept: the drawn frame is at most one or two behind the newest.
struct KhGtSnap {
    bool      ok = false;
    bool      rot_ok = false;
    uint64_t  seq = 0;
    int64_t   qpc = 0;
    uint32_t  gen = 0;
    uintptr_t xb = 0;
    float     par[12] = {};
    float     rot[12] = {};
    uintptr_t rot_of = 0;   // KH_ATTACH_ROT_OF: the base of the object rot was read for (0: none).
    std::vector<float> px;
    std::vector<float> pxd;   // KH_PXY_SMOOTH: per helper 3 floats (SQF order), estimator output minus px's reading.
};
struct KhAttach {
    // KH_ATTACH_GT_SNAP: the ring, and the page caches and copy choices of the reads that fill it (the Draw3D
    // sampler's, game thread).
    KhGtSnap  gts[KH_GTS_N];
    uint32_t  gts_w = 0;
    uint64_t  gts_cyc = ~0ull, gts_pick_seq = 0;   // The cycle last picked for, and its sample (kh_gts_for_cycle).
    uint32_t  gts_gen = 0;
    uintptr_t gts_xb = 0, gts_pb = 0, gts_rb = 0;
    uint32_t  gts_par_off = KH_ATTACH_OFF_NONE, gts_rot_off = KH_ATTACH_OFF_NONE;
    uintptr_t gts_par_vb = 0, gts_par_bb = 0, gts_rot_vb = 0, gts_rot_bb = 0;
    uintptr_t gts_par_avb = 0, gts_par_abb = 0, gts_rot_avb = 0, gts_rot_abb = 0;   // KH_ATTACH_VS_CACHE.
    std::vector<uint32_t>  gts_off;
    std::vector<uintptr_t> gts_vb, gts_bb, gts_avb, gts_abb;
    // KH_RT_PAINT_READ: the render thread's own read of what the sampler last read (rt_par: the parent or its helper;
    // rt_rot: the rotation object, nil when the lane has none of its own), with its own page caches, for the cast
    // fire's paint and the final step's mesh when this frame's own sample has not landed. rt_snap is that read,
    // rt_cyc the cycle it is for.
    game_value rt_par, rt_rot;
    uint32_t  rt_par_off = KH_ATTACH_OFF_NONE, rt_rot_off = KH_ATTACH_OFF_NONE;
    uintptr_t rt_par_vb = 0, rt_par_bb = 0, rt_rot_vb = 0, rt_rot_bb = 0, rt_pb = 0, rt_rb = 0, rt_xb = 0;
    uintptr_t rt_par_avb = 0, rt_par_abb = 0, rt_rot_avb = 0, rt_rot_abb = 0;   // KH_ATTACH_VS_CACHE.
    uint32_t  rt_gen = 0;
    std::vector<uint32_t>  rt_off;
    std::vector<uintptr_t> rt_vb, rt_bb, rt_avb, rt_abb;
    KhGtSnap  rt_snap;
    uint64_t  rt_cyc = ~0ull;
    // KH_ATTACH_ROT_OF: what the sampler named rt_par / rt_rot for - the binding's gen and key (kh_attach_key_gv's
    // base) and the rotation lane's object (its base; 0 while rt_rot is nil). The paint read reads a binding only
    // while it is still that one (one re-bound since - another key object, or a new skel_gen - reads nothing until
    // the next Draw3D names its objects), and rt_rot only while the rotation lane still follows what it was named
    // for.
    uint32_t  rt_for_gen = 0;
    uintptr_t rt_for_xb = 0, rt_rot_for = 0;
    // KH_RT_PAINT_READ: the read the final step drew for the mesh (the frame's own sample not landed), kept for its
    // check against that sample at the next cycle's final step (kh_gts_for_cycle).
    KhGtSnap  rt_chk;
    uint64_t  rt_chk_cyc = ~0ull;
    bool      rt_crew = false;   // rt_snap placed by the crew prediction (KH_CREW_LAG): the repick's alone.
    // KH_ATTACH_MAN_HELPER: a character's own visual state is not written while it rides in a vehicle (the engine
    // draws the crew from the vehicle, and the state keeps the pose it had when the character got in), but a helper
    // attached to it is: attachTo places it from the character's drawn transform. A lane naming a Man therefore gets
    // a pooled helper attached at the character's origin (kh_pxy_pool_get, memory point ""), and the Draw3D sampler
    // reads the helper wherever it would have read the character (kh_attach_org_read) - a skeletal binding's parent
    // included, whose frozen state the bones were made relative to (cancelled until a bone lay more than
    // KH_SKEL_REACH_M from it, and never under rotation false). One helper per character, shared through the pool by
    // every lane and binding on it and deleted with the last (kh_attach_proxy_reap). org_*_for is the object the
    // helper stands in for (kh_attach_org_sync retires one that no longer does); org_*_seq is the sample that first
    // saw the helper after it was made (the pool's made_seq + 1; 0: not known, the first sample of the lane), which
    // the sampler skips, because attachTo places a new helper at the next simulation step - one made frames ago is
    // read from the lane's first sample.
    // Game thread, under g_draw_list_mutex; the render thread never reads them.
    game_value  org_pos, org_rot;
    const void* org_pos_for = nullptr;
    const void* org_rot_for = nullptr;
    uint64_t    org_pos_seq = 0, org_rot_seq = 0;
    game_value obj_pos;   // Nil = position not attached.
    game_value obj_rot;   // Nil = rotation not attached.
    // KH_ATTACH_BONE. proxy: a LOCAL simple object (KH_ATTACH_BONE_SHAPE) the engine keeps attached to a memory point
    // of bone_parent, so the two lanes above read a bone's transform through the ordinary raw path with no new engine
    // assumption: obj_pos (and obj_rot, when the script follows the bone's rotation) hold THIS proxy, not the parent.
    // Nil = no bone attachment; the lanes keep their plain meaning. bone_parent is held for the death test and is the
    // parent the Draw3D sampler reads beside the proxy (kh_attach_parent_gv). attachTo does not destroy the proxy when
    // its parent dies (a mesh following a bone of a deleted vehicle would hang at the parent's last position forever),
    // so kh_attach_step tests both and either one empty kills the mesh. The proxy is OURS (pooled: shared by every
    // binding on the same parent and memory point) and must be deleted when its last binding ends, or it leaks a game
    // object. Attachments end under g_draw_list_mutex (and, in a park, under the graphics lock), where no SQF call may
    // be made, so the deletion is queued (g_attach_proxy_dead).
    game_value proxy;
    game_value bone_parent;
    // KH_SKEL - a skeletal binding ([object, true]). skel_proxy holds one KH_ATTACH_BONE_SHAPE per memory point of
    // bone_parent that a bone of the mesh names AND the skin drives something through (kh_skel_plan,
    // KH_SKEL_PROXY_CULL), attached follow-bone like the single proxy above and OURS in the same way; skel_mem: their
    // lower-cased names in the same order. The two lanes follow the PARENT here, not a proxy: obj_pos is bone_parent,
    // and obj_rot is bone_parent while the script's rotation boolean is true. The Draw3D sampler reads the proxies
    // beside the parent (KH_ATTACH_GT_SNAP); the render thread's kh_attach_step stashes that pair for kh_skin_sync
    // (KH_SKEL_SAMPLE). skel_gen: a process-wide serial taken at every binding, so the skin's per-proxy read caches
    // never outlive the proxies they were for.
    bool skel = false;
    std::vector<game_value> skel_proxy;
    std::vector<std::string> skel_mem;
    uint32_t skel_gen = 0;
    // KH_SKEL_SAMPLE - the proxies and the parent they were read beside, stashed raw (before the attach offsets) by
    // kh_attach_step from the cycle's Draw3D sample (KH_ATTACH_GT_SNAP). kh_skin_sync builds its poses from the stash
    // while it is fresh (KH_SKEL_SAMPLE_KEEP_MS); a stale one gives it no new pose. Per proxy 12 floats: position (SQF
    // order) then the basis rows; the parent the same 12. Written under g_draw_list_mutex by the step, read under it
    // by the sync.
    std::vector<float>     skel_smp;
    float                  skel_smp_par[12] = {};
    std::vector<float>     skel_smp_d;   // KH_PXY_SMOOTH: per proxy 3 floats, the sample's pxd (zero: none).
    bool                   skel_smp_ok = false;
    uint32_t               skel_smp_gen = 0;    // The skel_gen the stash was taken for.
    uint64_t               skel_smp_ms = 0;     // Its steady stamp (KH_SKEL_SAMPLE_KEEP_MS).
    // KH_SKEL_DRAW: the cycle (g_topo_cycles) whose render-thread step took this stash and decided its skin - usually
    // the cast fire's; stamped too when the maps fail or the pool's chunk throws (the last pose stands), not by a step
    // that culled it, nor when the chunk throws for a binding whose buffer stands culled (KH_SKIN_CULL: every later
    // step of the cycle decides again). Later steps of that cycle keep both unless a later pick takes another sample
    // (skel_smp_seq: the sample stashed; skel_draw_seq: the sample skinned) - the frame's own landing, or a
    // render-thread read of a changed pose (KH_RT_READ_SER) - when it stashes and skins again, so the injection and
    // everything after it draw one pose, the one the rigid bindings draw.
    uint64_t               skel_smp_cycle = ~0ull;
    uint64_t               skel_smp_seq = 0;
    uint64_t               skel_draw_seq = 0;
    // KH_ATTACH_CYCLE: the cycle (g_topo_cycles) in which the render thread's step last wrote this entry's lanes into
    // the mesh - the consume point's sample, the one that draws. A park's step (kh_park_run) leaves alone an entry the
    // render thread sampled this cycle, so a park's late draw lands where the injection drew; an entry the render
    // thread has not reached this cycle takes the park's step.
    uint64_t               smp_cycle = ~0ull;
    // KH_SKEL_DRAW - the skinning AT THE CONSUME POINT. The worker pipeline (kh_skin_sync at Draw3D, a pool job,
    // kh_skin_upload in the flush) hands the drawers a buffer skinned from an older sample (a skeletal mesh a frame
    // behind its body), so the render-thread step skins the stash it just took into the same buffer the upload installs
    // and draws that (kh_skin_draw_prep / _verts / _finish). skin: the instance kh_skin_sync keeps for this handle (its
    // drive tables and rest snapshot; set under g_draw_list_mutex). The draw_* vectors: this step's scratch, under
    // g_draw_list_mutex and render thread only, except draw_out / draw_met, which kh_skin_draw_verts fills on whichever
    // pool thread takes the binding's chunk (KH_SKIN_FORK) while the render thread holds the mutex and waits for it.
    std::shared_ptr<KhSkinInst> skin;
    std::vector<KhSkinXf>   draw_pose;
    std::vector<uint8_t>    draw_ok;
    std::vector<float>      draw_a;
    std::vector<MeshVertex> draw_out;
    std::vector<float>      draw_met;
    // KH_SKEL_SHAPE: the shape key of the pose the step last wrote and the
    // metric positions that took it. Render thread, under g_draw_list_mutex.
    std::vector<float>      draw_shape_ref;
    uint32_t                draw_shape_key = 0u;
    bool                    draw_shape_gpu = false;   // KH_SKIN_GPU: draw_shape_ref holds a palette (kh_skin_so_shape_note), not positions.
    // KH_SKIN_CULL: the buffer holds an older pose than the box the binding carries (a step culled it: no pass could
    // draw it); the next step that skins it, or a catch-up (kh_skin_cull_catchup), clears it. And the last bound the
    // cull took, by its inputs (the rest snapshot and the maps): the bound is a pure function of them, so equal inputs
    // reuse it.
    bool                    skin_culled = false;
    std::shared_ptr<const KhSkinRest> cull_rest;
    std::vector<float>      cull_a;
    float                   cull_bc[3] = { 0.0f, 0.0f, 0.0f };
    float                   cull_bs[3] = { 0.0f, 0.0f, 0.0f };
};
static constexpr uint64_t KH_SKEL_SAMPLE_KEEP_MS = 250;   // A stash older than this gives the sync no new pose.
// The render cycle counter (bumped at the main depth clear): the frame topology census's, here for the stash's stamp.
static uint64_t  g_topo_cycles = 0;
// Published copy for game-thread readers (among them kh_park_run's step, kh_skin_sync, kh_fx_late_gt and its
// chain-only flush, flush_frame's park test, and the material animation clock under the park - kh_mat_anim_*).
// Stored and zeroed with g_topo_cycles.
static std::atomic<uint64_t> g_topo_cycles_pub{ 0 };
// Deliberately never destructed (the shader pool's idiom; g_pxy_pool, g_crew, g_chain_end and the delete queue
// g_attach_proxy_dead, drained every Draw3D, are kept the same way): it owns SQF references, and a game_value's
// release at static teardown would touch the engine's allocator after the engine is done with it. Both mission edges
// empty it (reset_retained_state), normally long before teardown.
static std::unordered_map<std::string, KhAttach>& g_attach =
    *(new std::unordered_map<std::string, KhAttach>());   // Under g_draw_list_mutex.
// The step's gate: a session with no attachment pays one relaxed load per frame and takes no lock. Stored from the
// size under the mutex.
static std::atomic<uint32_t> g_attach_n{ 0 };
// KH_SKIN_FORK - one binding whose vertices this step skins across the pool: its loop tail (the upload, the box, the
// draw centre, the scene mark) waits for the vertices; the rest of the binding (its stamps, its cycle sample) stays
// where it was in the loop.
struct KhSkinDrawJob {
    const std::string* h;
    KhAttach*          a;
    RenderObject*      o;
    double             root[3];   // KH_POS_RES: the skeletal root, exact.
    float              bc[3], bs[3];
    bool               moved;
    bool               ok;
    bool               rt;   // reorder_on_render_thread() at the binding (the restep's gate).
    // KH_SKIN_GPU: the buffer kh_skin_so_batch skinned this binding into on the GPU (bc / bs then hold the pose's
    // bound); null = the pool skins it. KH_SKIN_POOL: the pair - a chunk's slice, or the binding's own at 0.
    KhVbRef            so_vb;
    // KH_SKIN_CULL: culled (so_vb is the binding's buffer, bc / bs the bound, nothing is drawn into it); bnd = bc / bs
    // already hold the bound (kh_skin_cull_pass), which the batch then takes; the stamps the step overwrote, handed
    // back to a culled binding.
    bool               culled;
    bool               bnd;
    uint64_t           prev_smp_cycle;
    uint64_t           prev_draw_seq;
};
}