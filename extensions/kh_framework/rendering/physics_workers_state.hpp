#pragma once

// The feature half (rendering_integration_features.hpp): the state (types, globals and constants) of the physics worker
// threads (cloth and skin jobs) and the fork's workers. Never include it on its own.
namespace RenderIntegration {
// Pending jobs, taken oldest first (KH_POOL_FIFO, kh_cloth_worker).
static std::deque<std::shared_ptr<KhClothInst>> g_cloth_q;
static std::condition_variable g_cloth_cv;
static std::condition_variable g_skin_cv;    // KH_POOL_SPLIT: the skin pool waits here, on the same g_cloth_mu.
static std::atomic<bool> g_cloth_stop{ false };
// Never destructed: a detached worker may outlive static teardown, and std::thread's static destructor is
// std::terminate under the loader lock (g_khmw_thr and g_khtl_thr are declared this way for the same reason).
static std::vector<std::thread>& g_cloth_thr = *(new std::vector<std::thread>());
// KH_POOL_SPLIT - two pools on one lock. The SOLVER pool (g_cloth_thr) takes the cloth and chain steps (g_cloth_q);
// the SKIN pool (g_skin_thr) the skinnings and their collider refits (g_skin_q). Each waits on its own condition
// variable and pops its own queue only, so a frame of many cloth steps cannot starve the skinning that draws the
// bodies and refits their colliders (one pool: 13 capes on 13 skinned colliders skipped the skin jobs every frame).
// A worker of either pool with nothing queued helps a posted fork (KH_CLOTH_FORK / KH_SKIN_FORK / KH_INJ_FORK) and
// yields to its own queue. Sizes: kh_pool_sizes.
static std::vector<std::thread>& g_skin_thr = *(new std::vector<std::thread>());
// KH_CLOTH_FORK: the pool's thread count as a plain atomic, written by kh_cloth_workers_start after its threads
// exist (zeroed by kh_cloth_workers_stop after they are joined) and read through kh_cloth_fork_workers on any thread
// that runs or sizes a fork (the gather on a worker, the render thread's skin and per-object forks, flush_frame's
// start test); the vector itself is never read off the game thread.
static std::atomic<uint32_t> g_cloth_thr_n{ 0 };
static std::atomic<uint32_t> g_skin_thr_n{ 0 };   // KH_POOL_SPLIT: the skin pool's, the same way.
// Buffers whose instance is gone. COM release belongs to flush_locked's housekeeping (kh_cloth_upload: the render
// thread's scene flush, or the game thread under a park) or to kh_cloth_release_all, never to a worker, so removal
// parks the pointer here.
static std::vector<ID3D11Buffer*> g_cloth_grave;

static std::atomic<uint32_t> g_cloth_skipped{ 0 };   // Frames a cloth was still busy from the last one.
}