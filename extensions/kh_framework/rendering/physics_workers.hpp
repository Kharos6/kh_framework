#pragma once

// The feature half (rendering_integration_features.hpp): the physics worker threads (cloth and skin jobs) and the
// fork's workers. Never include it on its own.
namespace RenderIntegration {
inline void kh_skin_job(const std::shared_ptr<KhSkinInst>& khsj_in);

// KH_CLOTH_FORK: the pool's size, for the gather's participant scratch, and the wake a posted fork sends the idle
// workers. The wake takes and drops g_cloth_mu before notifying so a worker between its predicate and its wait cannot
// miss it; the posting job holds only g_cloth_fork_mu while it is taken (kh_cloth_fork_run), which nothing waits on -
// every taker try-locks it.
inline uint32_t kh_cloth_fork_workers() {
    const uint32_t khfw_n = g_cloth_thr_n.load(std::memory_order_relaxed) + g_skin_thr_n.load(std::memory_order_relaxed);   // KH_POOL_SPLIT: both pools help.
    return khfw_n > 0u ? khfw_n : 1u;
}
inline void kh_cloth_fork_wake() {
    { std::lock_guard<std::mutex> khfw_g(g_cloth_mu); }
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // KH_POOL_SPLIT.
}
// KH_SKIN_FORK - the render thread's wake: it does not wait on g_cloth_mu, which the game thread holds across a chain
// build. That spares the wake's own wait only - the same render-thread step does block on g_cloth_mu in
// kh_cloth_pin_draw_step and kh_chain_end_draw. The lock pass only closes the window of a worker that has tested its
// wait condition and not yet slept; when the lock is busy that worker may sleep through this notify, and the fork then
// simply has one helper fewer - the initiator runs every chunk no helper takes, so no fork ever depends on a wake.
inline void kh_cloth_fork_wake_try() {
    if (g_cloth_mu.try_lock()) g_cloth_mu.unlock();
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // KH_POOL_SPLIT.
}

// KH_POOL_SPLIT: khcw_skin_pool = a skin worker (g_skin_q, g_skin_cv), else a solver worker (g_cloth_q, g_cloth_cv). A
// job of the worker's own queue comes first; a posted fork after it: an idle worker helps a running job, a worker with
// a job of its own runs that.
inline void kh_cloth_worker(bool khcw_skin_pool) {
    uint32_t khcw_served = 0;   // KH_CLOTH_FORK: the fork generation this worker last helped.
    std::condition_variable& khcw_cv = khcw_skin_pool ? g_skin_cv : g_cloth_cv;
    for (;;) {
        std::shared_ptr<KhClothInst> khcw_job;
        std::shared_ptr<KhSkinInst> khcw_skin;
        {
            std::unique_lock<std::mutex> khcw_l(g_cloth_mu);
            khcw_cv.wait(khcw_l, [khcw_served, khcw_skin_pool] {
                const uint32_t khcw_g = g_cloth_fork.gen_posted.load(std::memory_order_acquire);
                return g_cloth_stop.load(std::memory_order_relaxed) || (khcw_skin_pool ? !g_skin_q.empty() : !g_cloth_q.empty()) ||
                       (khcw_g != 0u && khcw_g != khcw_served);
            });
            if (g_cloth_stop.load(std::memory_order_relaxed) && (khcw_skin_pool ? g_skin_q.empty() : g_cloth_q.empty())) return;
            // KH_POOL_FIFO: the oldest job first, so every queued instance steps in turn. Do not take the newest
            // first: a busy instance is not queued again and the game thread queues in the same order every frame, so
            // a pool behind its work never reaches the ones queued first (a cape frozen, a body's collider still).
            if (!khcw_skin_pool && !g_cloth_q.empty()) {
                khcw_job = std::move(g_cloth_q.front());
                g_cloth_q.pop_front();
                // KH_CHAIN_END_RACE: under g_cloth_mu, so a reader holding it sees a job either queued (it cannot
                // start) or running.
                khcw_job->running.store(true, std::memory_order_release);
                g_cloth_q_n.store(static_cast<uint32_t>(g_cloth_q.size()), std::memory_order_relaxed);
            } else if (khcw_skin_pool && !g_skin_q.empty()) {
                khcw_skin = std::move(g_skin_q.front());
                g_skin_q.pop_front();
                g_skin_q_n.store(static_cast<uint32_t>(g_skin_q.size()), std::memory_order_relaxed);
            } else {
                khcw_l.unlock();
                khcw_served = kh_cloth_fork_help(khcw_served, khcw_skin_pool);   // KH_CLOTH_FORK: holding nothing.
                continue;
            }
        }
        if (khcw_skin) {
            kh_skin_job(khcw_skin);
            continue;
        }
        // An exception here would strand 'busy' true and the cloth would never
        // step again, so the flag is cleared on every path out.
        try { kh_cloth_run(khcw_job); }
        catch (...) {
            khcw_job->running.store(false, std::memory_order_release);
            khcw_job->busy.store(false, std::memory_order_release);
        }
    }
}

// KH_POOL_SPLIT: the two pools' sizes for a machine of khps_hw hardware threads. The solver pool takes half the
// machine, 2 to 8; the skin pool a quarter, 1 to 4 - on the 12-thread machines the users mostly have, 6 + 3 beside the
// engine's render and game threads (4 in all leaves cores idle under a dozen cloths). The floors (2 and 1) hold on any
// machine: both pools run work the render thread would otherwise wait for; the ceilings keep a wide machine's box for
// the engine.
inline void kh_pool_sizes(unsigned khps_hw, unsigned& khps_solver, unsigned& khps_skin) {
    if (khps_hw < 2u) khps_hw = 2u;
    khps_solver = khps_hw / 2u;
    if (khps_solver < 2u) khps_solver = 2u;
    if (khps_solver > 8u) khps_solver = 8u;
    khps_skin = khps_hw / 4u;
    if (khps_skin < 1u) khps_skin = 1u;
    if (khps_skin > 4u) khps_skin = 4u;
}
inline void kh_cloth_workers_start() {
    std::lock_guard<std::mutex> khcs_g(g_cloth_mu);
    // Each pool is made when it has no thread: a start that made one pool and not a thread of the other is finished
    // by the next call, not taken for done.
    if ((!g_cloth_thr.empty() && !g_skin_thr.empty()) || g_cloth_stop.load(std::memory_order_relaxed)) return;
    unsigned khcs_n = 0u, khcs_sn = 0u;
    kh_pool_sizes(std::thread::hardware_concurrency(), khcs_n, khcs_sn);
    if (g_cloth_thr.empty()) {
        for (unsigned khcs_i = 0; khcs_i < khcs_n; ++khcs_i) {
            try { g_cloth_thr.emplace_back(kh_cloth_worker, false); } catch (...) { break; }
        }
    }
    if (g_skin_thr.empty()) {   // KH_POOL_SPLIT.
        for (unsigned khcs_i = 0; khcs_i < khcs_sn; ++khcs_i) {
            try { g_skin_thr.emplace_back(kh_cloth_worker, true); } catch (...) { break; }
        }
    }
    g_cloth_thr_n.store(static_cast<uint32_t>(g_cloth_thr.size()), std::memory_order_relaxed);   // KH_CLOTH_FORK.
    g_skin_thr_n.store(static_cast<uint32_t>(g_skin_thr.size()), std::memory_order_relaxed);
}

// KH_SESSION_RESET - the pool's end, at the session teardown (park held, so the render thread is outside every fork).
// The queues are emptied first, so a worker finishes the job it holds and returns; nothing else is waited for. The
// dropped jobs' instances go with kh_cloth_drop_all / kh_skin_drop_all right after. kh_cloth_workers_start makes a new
// pool on the next session's demand.
inline void kh_cloth_workers_stop() {
    {
        std::lock_guard<std::mutex> khcx_g(g_cloth_mu);
        g_cloth_q.clear();
        g_cloth_q_n.store(0u, std::memory_order_relaxed);
        g_skin_q.clear();
        g_skin_q_n.store(0u, std::memory_order_relaxed);
        g_cloth_stop.store(true, std::memory_order_relaxed);
    }
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // KH_POOL_SPLIT.
    for (std::thread& khcx_t : g_cloth_thr) {
        if (khcx_t.joinable()) khcx_t.join();
    }
    g_cloth_thr.clear();
    g_cloth_thr_n.store(0u, std::memory_order_relaxed);
    for (std::thread& khcx_t : g_skin_thr) {   // KH_POOL_SPLIT.
        if (khcx_t.joinable()) khcx_t.join();
    }
    g_skin_thr.clear();
    g_skin_thr_n.store(0u, std::memory_order_relaxed);
    kh_reinit(g_cloth_fork);   // No fork can be posted: every participant is gone.
    g_cloth_stop.store(false, std::memory_order_relaxed);
}
}