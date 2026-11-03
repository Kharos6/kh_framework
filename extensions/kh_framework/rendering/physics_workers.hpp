#pragma once

// The feature half (rendering_integration_features.hpp): the physics worker threads (cloth and skin jobs) and the
// fork's workers. Never include it on its own.
namespace RenderIntegration {
inline void skin_job(const std::shared_ptr<SkinInst>& in);

// CLOTH_FORK: the pool's size, for the gather's participant scratch, and the wake a posted fork sends the idle
// workers. The wake takes and drops g_cloth_mu before notifying so a worker between its predicate and its wait cannot
// miss it; the posting job holds only g_cloth_fork_mu while it is taken (cloth_fork_run), which nothing waits on -
// every taker try-locks it.
inline uint32_t cloth_fork_workers() {
    const uint32_t n = g_cloth_thr_n.load(std::memory_order_relaxed) + g_skin_thr_n.load(std::memory_order_relaxed);   // POOL_SPLIT: both pools help.
    return n > 0u ? n : 1u;
}
inline void cloth_fork_wake() {
    { std::lock_guard<std::mutex> g(g_cloth_mu); }
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // POOL_SPLIT.
}
// SKIN_FORK - the render thread's wake: it does not wait on g_cloth_mu, which the game thread holds across a chain
// build. That spares the wake's own wait only - the same render-thread step does block on g_cloth_mu in
// cloth_pin_draw_step and chain_end_draw. The lock pass only closes the window of a worker that has tested its
// wait condition and not yet slept; when the lock is busy that worker may sleep through this notify, and the fork then
// simply has one helper fewer - the initiator runs every chunk no helper takes, so no fork ever depends on a wake.
inline void cloth_fork_wake_try() {
    if (g_cloth_mu.try_lock()) g_cloth_mu.unlock();
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // POOL_SPLIT.
}

// POOL_SPLIT: skin_pool = a skin worker (g_skin_q, g_skin_cv), else a solver worker (g_cloth_q, g_cloth_cv). A
// job of the worker's own queue comes first; a posted fork after it: an idle worker helps a running job, a worker with
// a job of its own runs that.
inline void cloth_worker(bool skin_pool) {
    uint32_t served = 0;   // CLOTH_FORK: the fork generation this worker last helped.
    std::condition_variable& cv = skin_pool ? g_skin_cv : g_cloth_cv;
    for (;;) {
        std::shared_ptr<ClothInst> job;
        std::shared_ptr<SkinInst> skin;
        {
            std::unique_lock<std::mutex> l(g_cloth_mu);
            cv.wait(l, [served, skin_pool] {
                const uint32_t g = g_cloth_fork.gen_posted.load(std::memory_order_acquire);
                return g_cloth_stop.load(std::memory_order_relaxed) || (skin_pool ? !g_skin_q.empty() : !g_cloth_q.empty()) ||
                       (g != 0u && g != served);
            });
            if (g_cloth_stop.load(std::memory_order_relaxed) && (skin_pool ? g_skin_q.empty() : g_cloth_q.empty())) return;
            // POOL_FIFO: the oldest job first, so every queued instance steps in turn. Do not take the newest
            // first: a busy instance is not queued again and the game thread queues in the same order every frame, so
            // a pool behind its work never reaches the ones queued first (a cape frozen, a body's collider still).
            if (!skin_pool && !g_cloth_q.empty()) {
                job = std::move(g_cloth_q.front());
                g_cloth_q.pop_front();
                // CHAIN_END_RACE: under g_cloth_mu, so a reader holding it sees a job either queued (it cannot
                // start) or running.
                job->running.store(true, std::memory_order_release);
                g_cloth_q_n.store(static_cast<uint32_t>(g_cloth_q.size()), std::memory_order_relaxed);
            } else if (skin_pool && !g_skin_q.empty()) {
                skin = std::move(g_skin_q.front());
                g_skin_q.pop_front();
                g_skin_q_n.store(static_cast<uint32_t>(g_skin_q.size()), std::memory_order_relaxed);
            } else {
                l.unlock();
                served = cloth_fork_help(served, skin_pool);   // CLOTH_FORK: holding nothing.
                continue;
            }
        }
        if (skin) {
            skin_job(skin);
            continue;
        }
        // An exception here would strand 'busy' true and the cloth would never
        // step again, so the flag is cleared on every path out.
        try { cloth_run(job); }
        catch (...) {
            job->running.store(false, std::memory_order_release);
            job->busy.store(false, std::memory_order_release);
        }
    }
}

// POOL_SPLIT: the two pools' sizes for a machine of hw hardware threads. The solver pool takes half the
// machine, 2 to 8; the skin pool a quarter, 1 to 4 - on the 12-thread machines the users mostly have, 6 + 3 beside the
// engine's render and game threads (4 in all leaves cores idle under a dozen cloths). The floors (2 and 1) hold on any
// machine: both pools run work the render thread would otherwise wait for; the ceilings keep a wide machine's box for
// the engine.
inline void pool_sizes(unsigned hw, unsigned& solver, unsigned& skin) {
    if (hw < 2u) hw = 2u;
    solver = hw / 2u;
    if (solver < 2u) solver = 2u;
    if (solver > 8u) solver = 8u;
    skin = hw / 4u;
    if (skin < 1u) skin = 1u;
    if (skin > 4u) skin = 4u;
}
inline void cloth_workers_start() {
    std::lock_guard<std::mutex> g(g_cloth_mu);
    // Each pool is made when it has no thread: a start that made one pool and not a thread of the other is finished
    // by the next call, not taken for done.
    if ((!g_cloth_thr.empty() && !g_skin_thr.empty()) || g_cloth_stop.load(std::memory_order_relaxed)) return;
    unsigned n = 0u, sn = 0u;
    pool_sizes(std::thread::hardware_concurrency(), n, sn);
    if (g_cloth_thr.empty()) {
        for (unsigned i = 0; i < n; ++i) {
            try { g_cloth_thr.emplace_back(cloth_worker, false); } catch (...) { break; }
        }
    }
    if (g_skin_thr.empty()) {   // POOL_SPLIT.
        for (unsigned i = 0; i < sn; ++i) {
            try { g_skin_thr.emplace_back(cloth_worker, true); } catch (...) { break; }
        }
    }
    g_cloth_thr_n.store(static_cast<uint32_t>(g_cloth_thr.size()), std::memory_order_relaxed);   // CLOTH_FORK.
    g_skin_thr_n.store(static_cast<uint32_t>(g_skin_thr.size()), std::memory_order_relaxed);
}

// SESSION_RESET - the pool's end, at the session teardown (park held, so the render thread is outside every fork).
// The queues are emptied first, so a worker finishes the job it holds and returns; nothing else is waited for. The
// dropped jobs' instances go with cloth_drop_all / skin_drop_all right after. cloth_workers_start makes a new
// pool on the next session's demand.
inline void cloth_workers_stop() {
    {
        std::lock_guard<std::mutex> g(g_cloth_mu);
        g_cloth_q.clear();
        g_cloth_q_n.store(0u, std::memory_order_relaxed);
        g_skin_q.clear();
        g_skin_q_n.store(0u, std::memory_order_relaxed);
        g_cloth_stop.store(true, std::memory_order_relaxed);
    }
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // POOL_SPLIT.
    for (std::thread& t : g_cloth_thr) {
        if (t.joinable()) t.join();
    }
    g_cloth_thr.clear();
    g_cloth_thr_n.store(0u, std::memory_order_relaxed);
    for (std::thread& t : g_skin_thr) {   // POOL_SPLIT.
        if (t.joinable()) t.join();
    }
    g_skin_thr.clear();
    g_skin_thr_n.store(0u, std::memory_order_relaxed);
    reinit(g_cloth_fork);   // No fork can be posted: every participant is gone.
    g_cloth_stop.store(false, std::memory_order_relaxed);
}
}