#pragma once

// The injection half (rendering_integration_injection.hpp): KH_ATTACH: attachments and proxies, the skeleton reads and
// the engine visual-state reads. Never include it on its own.
namespace RenderIntegration {

// KH_ATTACH_MAN_HELPER - a character's origin helper, for a lane about to name khom_gv: the pool's (shared by every
// lane on that character), or none for anything that is not a Man, or when the helper cannot be made (the lane then
// reads the character itself). GAME THREAD, holding no lock (SQF calls); defined after kh_pxy_pool_get.
// The caller owns the reference and files it: into a lane (kh_attach_org_put) or the delete queue. khom_from: the
// first sample that saw the helper (its pool entry's made_seq + 1), 0 when not known.
inline void kh_attach_org_make(const game_value& khom_gv, game_value& khom_out, uint64_t& khom_from);

// The read gate: true only when every page spanning [p, p + n) is committed and readable, so an address that is not
// one can be rejected instead of faulted on. Do not delete it: it is load-bearing on the live path. kh_attach_vs_read
// gates BOTH of its dereferences on it (raw reads at measured offsets into engine memory, every frame a mesh follows
// an object). It lives in the injection half because that is the one that provably has windows.h.
inline bool kh_probe_readable(const void* khpr_p, size_t khpr_n) {
    if (!khpr_p || khpr_n == 0) return false;
    const uint8_t* khpr_a = static_cast<const uint8_t*>(khpr_p);
    const uint8_t* khpr_end = khpr_a + khpr_n;
    if (khpr_end < khpr_a) return false;   // Wrapped: not an address.
    while (khpr_a < khpr_end) {
        MEMORY_BASIC_INFORMATION khpr_mbi;
        if (VirtualQuery(khpr_a, &khpr_mbi, sizeof(khpr_mbi)) != sizeof(khpr_mbi)) return false;
        if (khpr_mbi.State != MEM_COMMIT) return false;
        if (khpr_mbi.Protect & PAGE_GUARD) return false;
        const DWORD khpr_pr = khpr_mbi.Protect & 0xFFu;
        if (khpr_pr != PAGE_READONLY && khpr_pr != PAGE_READWRITE && khpr_pr != PAGE_WRITECOPY &&
            khpr_pr != PAGE_EXECUTE_READ && khpr_pr != PAGE_EXECUTE_READWRITE &&
            khpr_pr != PAGE_EXECUTE_WRITECOPY) return false;
        const uint8_t* khpr_next = static_cast<const uint8_t*>(khpr_mbi.BaseAddress) + khpr_mbi.RegionSize;
        if (khpr_next <= khpr_a) return false;   // No progress: stop rather than spin.
        khpr_a = khpr_next;
    }
    return true;
}

// KH_ATTACH - a mesh bound to a game object's transform. The objects are read once a frame at Draw3D, on the game
// thread (KH_ATTACH_GT_SNAP), and the mesh is placed at the points the scene is consumed - the injection and the cast
// fire on the render thread, the flush when a cycle takes the late path (the render thread's flush, or a park's on
// the game thread) - every one of them drawing the cycle's one pick (kh_gts_for_cycle): the frame's Draw3D sample, or,
// while it has not landed, the render thread's own read of the objects (KH_RT_PAINT_READ). Position and rotation
// attach independently and may name different objects; an empty lane keeps whatever the script last set there.
// ONE reader, kh_attach_raw, takes the transform straight out of the engine's visual state at a MEASURED offset: no
// SQF call anywhere on this path, which is the point of the feature (the cost of following many objects). A lane
// whose read fails its own checks leaves the mesh where it is. It reads the RENDER copy where the object has one -
// the transform the engine draws the object from, not the one the simulation has already advanced to, so a moving
// vehicle's mesh sits on the vehicle instead of ahead of it (kh_attach_raw has the numbers); in the Eden editor the
// SIMULATION copy alone (KH_ATTACH_EDEN).
// ANCHOR: a mesh sits at the visual state's own origin, which is the object's WORLD position (getPosWorld: the model
// centre) in the same [x, y, zASL] frame as every mesh position - above getPosASL (the land contact) by a per-model
// constant (0 m on a man, 0.503 m on a house, 1.797 m and 2.430 m on two vehicles; measured). Nothing here reads
// getPosASL. That origin, the point the engine draws the model from, is the only anchor the object's own memory
// defines. Do not cancel the offset by measuring it against getPosASL: it costs one SQF read per lane and a "wait
// until the object holds still" gate to take it safely.
// The attachment lives beside the draw list, not inside RenderObject: RenderObject is copied wholesale on every scene
// sync, staging copy and flush pass, several of them on the render thread, and a game_value member would put a
// refcount touch on all of them. Here the objects are held once, by handle, under g_draw_list_mutex.
// THREADS: the table is WRITTEN only by the game thread (the SQF commands via kh_attach_set / kh_attach_drop, and the
// draw-list erase sites). It is READ by both: kh_attach_step runs on the render thread from the injection, the cast
// fire and the other drawers, and on the game thread from kh_park_run. The step therefore only ever COPIES a sampled
// transform and SKIPS orphans - it must never erase, because releasing a game_value on the render thread touches the
// SQF allocator off its own thread. If that function ever gains an erase, that is the bug.
#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
#define KH_ATTACH_VS_RENDER 0x1A0u
#define KH_ATTACH_VS_SIM    0xD0u
#else
#define KH_ATTACH_VS_RENDER 0u
#define KH_ATTACH_VS_SIM    0u
#endif
// The Draw3D sampler's serial (kh_attach_gt_snap): each sample's KhGtSnap::seq. Game thread only. Reset with the
// objects (kh_session_objects_reset), not the session: the bindings hold its values (their rings and picks, the
// helper stamps KhAttach::org_*_seq and KhPxyPoolEnt::made_seq), and the deferred teardown leaves the next mission's
// bindings standing.
static uint64_t g_gts_seq = 0;
static uint32_t g_skel_gen_serial = 0;   // KH_SKEL: game thread only (the commands).

// KH_ATTACH_DEAD - handles whose followed object the engine has emptied, waiting for the game thread to remove them.
// std::string only: the step that fills this runs on the render thread, and a handle is the one thing about an
// attachment that can be copied there without touching the SQF allocator. Drained by kh_attach_reap, game thread only.
static std::vector<std::string> g_attach_dead;   // Under g_draw_list_mutex.
// The reap's gate, the twin of g_attach_n: a session where nothing ever dies pays one relaxed load per frame and takes
// no lock.
static std::atomic<uint32_t> g_attach_dead_n{ 0 };

// KH_ATTACH_BONE - proxies whose attachment has ended, waiting for the game thread to deleteVehicle them. game_value
// and not a handle: once the KhAttach entry goes, this vector is the only thing that knows the object exists.
// RETIRING and DELETING a proxy have different requirements: a retire happens wherever an attachment ends, under
// g_draw_list_mutex (in a park, under the graphics lock too); the delete is sqf::delete_vehicle, which may not run
// under either (an SQF command re-entering on this thread would take the mutex again). So the retire only moves a
// reference into this vector, and kh_attach_proxy_reap drains it from flush_frame holding nothing, releasing one pool
// reference per entry.
// KH_EXIT_STATICS: never destructed (g_attach's rule - SQF references must not be released at static teardown).
static std::vector<game_value>& g_attach_proxy_dead = *(new std::vector<game_value>());   // Under g_draw_list_mutex.
// The drain's gate, the twin of g_attach_dead_n: a session that never attaches to a bone pays one relaxed load per
// frame and takes no lock.
static std::atomic<uint32_t> g_attach_proxy_dead_n{ 0 };

// The memory-point proxy's class: a local simple object the engine attaches and maintains a transform for. It is never
// drawn (KH_PXY_HIDE) and the transform is read from its visual state, so its model is not a contract: any class the
// engine will create locally and attach will do.
static const char* const KH_ATTACH_BONE_SHAPE = "KH_HelperRender";

// KH_ATTACH_DIAG - what the Draw3D sampler's object reads (kh_gts_read) came back as, for getRenderStats
// (attachRepaired / attachRefused / attachSkew). Written only by the sampler, on the game thread under
// g_draw_list_mutex; atomics because getRenderStats and resetRenderStats read and zero them without it. Collected
// only while stats are armed, like every g_stats counter.
struct KhAttachDiag {
    float skew = 0.0f;       // Largest |row length - 1| of the basis read.
    bool  repaired = false;  // A row outside 1 % of unit length: re-orthonormalised.
};
static std::atomic<uint64_t> g_attach_repaired{ 0 };   // Reads taken only because the basis was repaired.
static std::atomic<uint64_t> g_attach_refused{ 0 };    // Reads refused (what follows: kh_attach_gt_snap).
static std::atomic<uint32_t> g_attach_skew_bits{ 0 };  // Float bits: the largest skew seen.

// One test for "is this slot an object to follow", used by every site that asks. is_nil FIRST: nil is a documented
// value in both slots (a nil rotation is the identity), and every parser beside this one orders the two tests this way.
inline bool kh_attach_is_obj(const game_value& khao_gv) {
    return !khao_gv.is_nil() && khao_gv.type_enum() == game_data_type::OBJECT;
}

// KH_PXY_POOL - one proxy per (parent, memory point), shared by every binding on that point, attached with zero
// offset - following the bone for a memory point, at the origin for none (KH_ATTACH_MAN_HELPER). g_pxy_pool (by
// proxy) counts the holders; kh_attach_proxy_reap releases one per ended proxy and deletes at the last.
// g_pxy_pool_key (parent | lower-case memory point) finds a live proxy to share; an entry with a dead parent or proxy
// is not shared. A helper carries no scale of its own: it is hidden (KH_PXY_HIDE), so nothing has to tell its matrix
// from a followed object's. GAME THREAD ONLY, but for the render thread's estimator step (KH_PXY_SMOOTH: a find and
// the step under g_pxy_pool_mu). g_pxy_pool holds game_values and g_pxy_pool_key the proxy bases (both never
// destructed, like g_attach); the mission edge empties them.
struct KhPxyPoolEnt {
    game_value  proxy;
    game_value  parent;
    uint32_t    refs = 0;
    std::string key;
    uint64_t    made_seq = 0;   // g_gts_seq when it was made and attached (KH_ATTACH_MAN_HELPER's stamp).
    // KH_PXY_SMOOTH: a memory-point proxy (bone) and its estimator, per SQF axis - the last reading, the target and
    // which way it last moved, the tracker's output and velocity (m/s); the sample it last ran for, that sample's
    // stamp and delta (output minus reading). Stepped under g_pxy_pool_mu (kh_pxy_smooth_note) by the game thread
    // (the sampler, a chain's end read) and the render thread (its reads, KH_RT_PAINT_READ); ends with the entry.
    bool        bone = false;
    bool        sm_ok = false;
    uint64_t    sm_seq = 0;
    int64_t     sm_q = 0;
    int8_t      sm_dir[3] = {};   // The way each axis's target last moved: +1, -1, 0 = not since a restart.
    float       sm_r[3] = {};
    double      sm_t[3] = {};
    double      sm_x[3] = {};
    double      sm_v[3] = {};
    float       sm_d[3] = {};
    // KH_PXY_REPLAY: the estimator as it stood before sm_seq's step, so a later reading of that sample steps again
    // from there (kh_pxy_smooth).
    bool        smp_ok = false;
    int64_t     smp_q = 0;
    int8_t      smp_dir[3] = {};
    float       smp_r[3] = {};
    double      smp_t[3] = {};
    double      smp_x[3] = {};
    double      smp_v[3] = {};
};
static std::unordered_map<uintptr_t, KhPxyPoolEnt>& g_pxy_pool =
    *(new std::unordered_map<uintptr_t, KhPxyPoolEnt>());
static std::unordered_map<std::string, uintptr_t>& g_pxy_pool_key =
    *(new std::unordered_map<std::string, uintptr_t>());
static std::atomic<uint32_t> g_pxy_pool_n{ 0 };
// KH_PXY_SMOOTH: the estimator is stepped from the game thread (the sampler, a chain's end) and the render thread
// (its reads, KH_RT_PAINT_READ): every step, and every insert into or erase from g_pxy_pool (game thread), holds
// this. Held for map operations and the step alone - no engine call, no other lock taken inside it.
static std::mutex g_pxy_pool_mu;
// KH_ATTACH_ROW_MAX - the longest basis row a visual-state read admits (kh_attach_vs_eval). A followed object the
// script scaled carries its scale in these rows, so the band is generous: a little over 2x was the widest measured,
// and nothing beyond 4 has ever been asked for.
static constexpr float KH_ATTACH_ROW_MAX = 4.0f;
// KH_SESSION_RESET / KH_VS_MEMO: the read memo's epoch, bumped by every session teardown (never reset, so no thread's
// stamp can match a later epoch by accident); a thread whose memo is from an earlier epoch empties it at its next read.
static std::atomic<uint32_t> g_vs_memo_epoch{ 0 };
// An object's identity for the caches and the sample ring: its engine object's address, 0 for none.
inline uintptr_t kh_attach_base_of(const game_value& khab_gv) {
    if (!kh_attach_is_obj(khab_gv)) return 0;
    const game_data_object* khab_gd = static_cast<const game_data_object*>(khab_gv.data.get());
    if (!khab_gd || !khab_gd->object || !khab_gd->object->object) return 0;
    return reinterpret_cast<uintptr_t>(khab_gd->object->object);
}
// One pool entry's end: its key if it still names it, and the entry.
inline void kh_pxy_pool_erase(std::unordered_map<uintptr_t, KhPxyPoolEnt>::iterator khpr_it) {
    KhPxyPoolEnt& khpr_e = khpr_it->second;
    const auto khpr_kit = g_pxy_pool_key.find(khpr_e.key);
    if (khpr_kit != g_pxy_pool_key.end() && khpr_kit->second == khpr_it->first) g_pxy_pool_key.erase(khpr_kit);
    {
        std::lock_guard<std::mutex> khpr_l(g_pxy_pool_mu);   // KH_PXY_SMOOTH: the render thread steps entries.
        g_pxy_pool.erase(khpr_it);
    }
    g_pxy_pool_n.store(static_cast<uint32_t>(g_pxy_pool.size()), std::memory_order_relaxed);
}
// The reap's release: true = no holder left (or never pooled), the caller deletes. A proxy the engine already deleted
// reads base 0, so its entry is found by reference instead.
inline bool kh_pxy_pool_release(const game_value& khpr_gv) {
    const uintptr_t khpr_b = kh_attach_base_of(khpr_gv);
    auto khpr_it = khpr_b != 0 ? g_pxy_pool.find(khpr_b) : g_pxy_pool.end();
    if (khpr_b == 0 && !khpr_gv.is_nil()) {
        for (auto khpr_s = g_pxy_pool.begin(); khpr_s != g_pxy_pool.end(); ++khpr_s) {
            if (khpr_s->second.proxy.data.get() == khpr_gv.data.get()) { khpr_it = khpr_s; break; }
        }
    }
    if (khpr_it == g_pxy_pool.end()) return true;
    KhPxyPoolEnt& khpr_e = khpr_it->second;
    if (khpr_e.refs > 1u) { --khpr_e.refs; return false; }
    kh_pxy_pool_erase(khpr_it);
    return true;
}

// KH_ATTACH_DEAD - has the engine emptied this lane's object, as opposed to the read merely having failed this frame?
// A true answer DESTROYS a mesh, so it is given only on positive evidence that the holder is empty, never on any form
// of 'could not read it this time' (a failed page probe, a failed basis check, an offset yet to be chosen): each of
// those is a transient that must leave the mesh where it is, which kh_attach_raw returning false already does. The
// test is aimed at the harmless failure direction: MISSING a death leaves a mesh pinned to a dead object; inventing
// one deletes a mesh the script still owns and cannot get back. The holder chain is the same liveness test
// get_unit_yaw_speed_sqf and update_unit_states use to decide an object is not worth reading, and the one
// kh_attach_raw refuses to proceed past; here it is treated as a DEATH rather than a skip.
// MEASURED, one object, before and after deleteVehicle:
//
//                       live          deleted
//     sqfIsNull         0             1
//     gd                ...4ca198     ...4ca198   (the SAME allocation)
//     gd->object        non-null      non-null
//     ->object          ...4dd1c0     0x0         <== the signal
//
// The engine empties the third link and leaves the first two standing: our own reference keeps the game_data_object
// alive (its first 0x40 bytes did not change by a single qword) and keeps the holder alive, and the entity pointer
// inside the holder becomes exactly nullptr, in lockstep with the engine's own isNull - a clean nullptr rather than a
// freed pointer, safe to read because the two upper links are intact. The order of the three tests below is the
// measured order: the first two protect the third's dereference and, on this build, never fire.
// A nil lane is unattached and a non-object lane was never followed: neither is a death.
inline bool kh_attach_obj_dead(const game_value& khod_gv) {
    if (khod_gv.is_nil()) return false;
    if (khod_gv.type_enum() != game_data_type::OBJECT) return false;
    // Our own game_value holds the reference, so the game_data_object itself cannot have been freed under us; only
    // the links below it empty out.
    const game_data_object* khod_gd = static_cast<const game_data_object*>(khod_gv.data.get());
    if (!khod_gd) return true;
    if (!khod_gd->object) return true;
    return khod_gd->object->object == nullptr;
}

// KH_ATTACH_RAW - the object's transform straight out of the engine's visual state, which is what makes attaching
// many meshes affordable: no SQF call at all, against four per mesh per frame for a read through SQF commands.
// EVERY NUMBER HERE WAS MEASURED (a memory sweep on a player, a vehicle and a house). The simulation copy's state
// pointer is at object + 0xD0 on a 64-bit build, not 0xA0: 0xA0 holds two packed 32-bit values (0x0000000c000000d9 on
// the player), which game_data_object::get_position_matrix dereferences with no build branch - why that call crashes;
// do not use it. visState1 starts one pointer into the state that pointer names (*(object + 0xD0) + 8) and is laid out
// aside / up / dir / position / scale / maxScale, as types.hpp declares. The axis order is (east, up, north) - our
// engine axes - so the three basis vectors ARE rot_m's rows with no conversion: measured error against vectorDir /
// vectorUp was 0.0000 on all three objects, and the bases were orthonormal to 7.5e-05.
// The position is the object's WORLD position (getPosWorld's model centre), not getPosASL's land contact (the ANCHOR
// paragraph above is the whole of the position contract). Expressed in each object's own axes the offset between the
// two is almost pure model-up ([-0.003, 1.797, -0.006] for the vehicle): a per-model constant in MODEL space, not a
// world offset. It is NOT cancelled.
// An object carries up to TWO states. 0xD0 matched vectorDir / vectorUp to 0.0000 and carries the WORLD position: it
// IS the simulation state. 0x1A0 is the VISUAL state, and it lags 0xD0 by a tick: getPosASLVisual and getPosASL differ
// on a moving object by an amount that grows with speed, and reading 0x1A0 makes a followed mesh sit where the engine
// draws its object instead of a tick ahead of it. Do not conclude "no lag" from the two agreeing at one instant: that
// cannot tell agreement from identity. 0x1A0 is the copy every POSE is read from; 0xD0 is the fallback and the only
// copy that carries an object's scale.
// 0x1A0 is not universal (a house reports only 0xD0), so the copy is chosen per object (KH_ATTACH_COPY,
// kh_attach_raw): 0x1A0 wherever it reads, locked at the first read that lands, and 0xD0 - which every object tested
// carries - for an object with no 0x1A0. In the Eden editor, 0xD0 alone (KH_ATTACH_EDEN).

// KH_VS_MEMO - two entries per set, the set picked by the state pointer: the 14 floats last validated for a state and
// the answer. An entry is used only when both the pointer and every byte of the floats match, so a hit is the
// evaluation's own result. A miss replaces the state's own entry when the set holds one, else the set's older
// entry. 8,192 entries (~1.1 MB) per thread that reads (the
// game thread: the Draw3D sampler, the commands, the chains, the crew table; the render thread: the paint read,
// KH_RT_PAINT_READ) keep a scene of a few thousand followed objects and helpers resident; heap-allocated on the
// thread's first read and kept for the process, zeroed (v = 0 never matches a state), and emptied again at the
// thread's first read after a session teardown (g_vs_memo_epoch).
struct KhVsMemoEnt {
    uintptr_t    v = 0;
    float        f[14] = {};
    bool         ok = false;
    bool         reached = false;
    KhAttachDiag diag;
    float        pos[3] = {}, rot[9] = {}, len[3] = {};
};
static constexpr uint32_t KH_VS_MEMO_SETS = 4096u;   // Power of two.
struct KhVsMemo {
    KhVsMemoEnt e[KH_VS_MEMO_SETS][2];
    uint8_t     old[KH_VS_MEMO_SETS] = {};   // The way a miss replaces next.
};
// g_vs_memo_epoch is defined with KH_ATTACH_ROW_MAX above.
inline KhVsMemo* kh_vs_memo() {
    static thread_local KhVsMemo* khvm_m = nullptr;
    static thread_local uint32_t khvm_e = 0;
    const uint32_t khvm_now = g_vs_memo_epoch.load(std::memory_order_relaxed);
    if (!khvm_m) {
        khvm_m = new (std::nothrow) KhVsMemo();   // Null = no memo on this thread; every read evaluates.
        khvm_e = khvm_now;
    } else if (khvm_e != khvm_now) {
        kh_reinit(*khvm_m);
        khvm_e = khvm_now;
    }
    return khvm_m;
}
// KH_VS_MEMO - the validation of one visual-state read, a pure function of the 14 floats read (the row ceiling is a
// constant): the rows' lengths and skew (the diag, set on every path), the band, finite and square tests, and the
// re-orthonormalisation. pos / rot / len are written only on success. kh_attach_vs_read memoises it per thread
// (KhVsMemo), so a read whose bytes match the last read of the same state costs a compare instead of the square roots
// and divides - every helper shared by many bindings, every parent the draw sweep reads, every still object - and
// returns exactly what the evaluation would.
inline bool kh_attach_vs_eval(const float* khvs_f, float khvs_pos[3], float khvs_rot[9],
                              KhAttachDiag* khvs_diag, float* khvs_len_out) {
    // The result is checked, not assumed: three rows of plausible length that stand well apart, and a finite
    // position. A read that fails this leaves the lane where it is for the frame.
    // The rows need NOT be orthonormal - the contract with bones. A plain object's are (to 7.5e-05 on the three
    // objects measured), but a KH_ATTACH_BONE proxy is attached with the bone's rotation, and the engine hands it the
    // bone's ANIMATED matrix, which blended poses shorten and shear as the pose bends. Do not require 1 % unit rows:
    // that refuses those reads and the mesh holds its last transform until the pose comes back. So a basis is accepted
    // within a generous band and re-orthonormalised - forward (dir) kept, up made square to it, aside square to both
    // with its own sign - and a mesh only ever takes a rotation. On an orthonormal read this changes nothing beyond
    // float rounding, and a still object still hands back the same floats every frame.
    float khvs_len[3];
    float khvs_skew = 0.0f;
    bool  khvs_fix = false;
    for (int khvs_r = 0; khvs_r < 3; ++khvs_r) {
        const float* khvs_row = khvs_f + khvs_r * 3;
        const float khvs_l2 = khvs_row[0] * khvs_row[0] + khvs_row[1] * khvs_row[1] + khvs_row[2] * khvs_row[2];
        khvs_len[khvs_r] = sqrtf(khvs_l2);
        if (!(khvs_l2 > 0.98f && khvs_l2 < 1.02f)) khvs_fix = true;   // Outside the old unit test.
        const float khvs_dev = fabsf(khvs_len[khvs_r] - 1.0f);
        if (khvs_dev > khvs_skew) khvs_skew = khvs_dev;
    }
    if (khvs_diag) { khvs_diag->skew = khvs_skew; khvs_diag->repaired = khvs_fix; }
    const float khvs_max = KH_ATTACH_ROW_MAX;
    for (int khvs_r = 0; khvs_r < 3; ++khvs_r) {
        if (!(khvs_len[khvs_r] >= 0.5f && khvs_len[khvs_r] <= khvs_max)) return false;   // NaN fails too.
    }
    for (int khvs_i = 9; khvs_i < 12; ++khvs_i) {
        if (khvs_f[khvs_i] != khvs_f[khvs_i] || fabsf(khvs_f[khvs_i]) > 1.0e7f) return false;
    }
    const float* khvs_a = khvs_f;       // aside
    const float* khvs_u = khvs_f + 3;   // up
    const float* khvs_d = khvs_f + 6;   // dir
    // Pairwise within 30 degrees of square (|cos| under 0.5), which a sheared pose stays well inside and an unrelated
    // triple of floats almost never is.
    if (fabsf(kh_cloth_v3_dot(khvs_a, khvs_u)) >= 0.5f * khvs_len[0] * khvs_len[1] ||
        fabsf(kh_cloth_v3_dot(khvs_a, khvs_d)) >= 0.5f * khvs_len[0] * khvs_len[2] ||
        fabsf(kh_cloth_v3_dot(khvs_u, khvs_d)) >= 0.5f * khvs_len[1] * khvs_len[2]) return false;
    float khvs_o[9];
    for (int k = 0; k < 3; ++k) khvs_o[6 + k] = khvs_d[k] / khvs_len[2];
    const float khvs_ud = kh_cloth_v3_dot(khvs_u, khvs_o + 6);
    for (int k = 0; k < 3; ++k) khvs_o[3 + k] = khvs_u[k] - khvs_ud * khvs_o[6 + k];
    const float khvs_ul = kh_cloth_v3_len(khvs_o + 3);   // >= 0.866 of up's length, by the test above.
    for (int k = 0; k < 3; ++k) khvs_o[3 + k] /= khvs_ul;
    const float khvs_ad = kh_cloth_v3_dot(khvs_a, khvs_o + 6);
    const float khvs_au = kh_cloth_v3_dot(khvs_a, khvs_o + 3);
    for (int k = 0; k < 3; ++k) khvs_o[k] = khvs_a[k] - khvs_ad * khvs_o[6 + k] - khvs_au * khvs_o[3 + k];
    const float khvs_al = kh_cloth_v3_len(khvs_o);
    if (!(khvs_al >= 0.5f * khvs_len[0])) return false;   // aside nearly in the plane of the other two.
    for (int k = 0; k < 3; ++k) khvs_o[k] /= khvs_al;
    memcpy(khvs_rot, khvs_o, sizeof(khvs_o));   // aside / up / dir = rot_m rows 0 / 1 / 2.
    khvs_pos[0] = khvs_f[9];    // east.
    khvs_pos[1] = khvs_f[11];   // north = SQF y.
    khvs_pos[2] = khvs_f[10];   // up = SQF zASL.
    if (khvs_len_out) memcpy(khvs_len_out, khvs_len, sizeof(khvs_len));   // Before the re-orthonormalisation.
    return true;
}

// One visual state, structure only - no reference, nothing assumed but the measured layout recorded above. khvs_pos
// comes back in RenderObject::pos's [x, y, zASL] and IS the anchor - no correction toward another anchor (getPosASL)
// is applied to it afterwards. khvs_vb caches the last state pointer confirmed readable, and it stays confirmed only
// while reads through it validate: a pointer whose bytes do not read as a state (0x1A0 on an object class nobody
// measured) is probed again on its next read (KH_ATTACH_VS_CACHE). khvs_bb is the last object base whose slot at
// khvs_off was confirmed.
// BOTH dereferences are page-gated. This is the one site in either header that dereferences gd->object->object rather
// than using it as an opaque identity key, and the offsets are measured, not declared: nothing guarantees a given
// object extends to 0x1A0 (a house carries no render copy there), so the slot read is gated like the state read. Both
// gates are keyed on their pointer, so a followed object costs the page walk when it changes and not per frame.
inline bool kh_attach_vs_read(uintptr_t khvs_base, uint32_t khvs_off, uintptr_t& khvs_vb,
                              uintptr_t& khvs_bb, float khvs_pos[3], float khvs_rot[9],
                              KhAttachDiag* khvs_diag = nullptr, float* khvs_len_out = nullptr) {
    if (khvs_off == 0u) return false;
    if (khvs_base != khvs_bb) {   // Changed (or first sight): confirm the slot before reading it.
        if (!kh_probe_readable(reinterpret_cast<const void*>(khvs_base + khvs_off),
                               sizeof(uintptr_t))) return false;
        khvs_bb = khvs_base;
    }
    uintptr_t khvs_v = 0;
    memcpy(&khvs_v, reinterpret_cast<const void*>(khvs_base + khvs_off), sizeof(khvs_v));
    if (khvs_v == 0 || (khvs_v % sizeof(uintptr_t)) != 0) return false;
    if (khvs_v != khvs_vb) {   // Changed (or first sight): confirm the pages before trusting it.
        if (!kh_probe_readable(reinterpret_cast<const void*>(khvs_v + sizeof(uintptr_t)),
                               sizeof(float) * 14)) return false;
        khvs_vb = khvs_v;
    }
    float khvs_f[14];
    memcpy(khvs_f, reinterpret_cast<const void*>(khvs_v + sizeof(uintptr_t)), sizeof(khvs_f));
    // KH_VS_MEMO: the same 14 floats validate to the same answer.
    KhVsMemo* const khvs_m = kh_vs_memo();
    KhVsMemoEnt* khvs_e = nullptr;
    if (khvs_m) {
        const uint32_t khvs_s = static_cast<uint32_t>(((khvs_v >> 4) ^ (khvs_v >> 16)) & (KH_VS_MEMO_SETS - 1u));
        for (uint32_t khvs_w = 0; khvs_w < 2u; ++khvs_w) {
            KhVsMemoEnt& khvs_c = khvs_m->e[khvs_s][khvs_w];
            if (khvs_c.v == khvs_v && memcmp(khvs_c.f, khvs_f, sizeof(khvs_f)) == 0) {
                khvs_m->old[khvs_s] = static_cast<uint8_t>(khvs_w ^ 1u);   // The other way ages.
                if (khvs_c.reached && khvs_diag) *khvs_diag = khvs_c.diag;
                if (!khvs_c.ok) { khvs_vb = 0; return false; }   // Not a state: confirmed no longer.
                memcpy(khvs_rot, khvs_c.rot, sizeof(khvs_c.rot));
                memcpy(khvs_pos, khvs_c.pos, sizeof(khvs_c.pos));
                if (khvs_len_out) memcpy(khvs_len_out, khvs_c.len, sizeof(khvs_c.len));
                return true;
            }
        }
        // A miss: this state's own entry if it has one (its bytes moved on), else the set's older way.
        uint32_t khvs_w = khvs_m->old[khvs_s];
        if (khvs_m->e[khvs_s][0].v == khvs_v) khvs_w = 0u;
        else if (khvs_m->e[khvs_s][1].v == khvs_v) khvs_w = 1u;
        khvs_e = &khvs_m->e[khvs_s][khvs_w];
        khvs_m->old[khvs_s] = static_cast<uint8_t>(khvs_w ^ 1u);
    }
    KhAttachDiag khvs_dg;
    float khvs_ep[3] = {}, khvs_er[9] = {}, khvs_el[3] = {};
    const bool khvs_ok = kh_attach_vs_eval(khvs_f, khvs_ep, khvs_er, &khvs_dg, khvs_el);
    if (khvs_e) {
        khvs_e->v = khvs_v;
        memcpy(khvs_e->f, khvs_f, sizeof(khvs_f));
        khvs_e->ok = khvs_ok;
        khvs_e->reached = true;   // kh_attach_vs_eval sets its diag on every path.
        khvs_e->diag = khvs_dg;
        memcpy(khvs_e->pos, khvs_ep, sizeof(khvs_ep));
        memcpy(khvs_e->rot, khvs_er, sizeof(khvs_er));
        memcpy(khvs_e->len, khvs_el, sizeof(khvs_el));
    }
    if (khvs_diag) *khvs_diag = khvs_dg;
    if (!khvs_ok) { khvs_vb = 0; return false; }   // Not a state: confirmed no longer.
    memcpy(khvs_rot, khvs_er, sizeof(khvs_er));
    memcpy(khvs_pos, khvs_ep, sizeof(khvs_ep));
    if (khvs_len_out) memcpy(khvs_len_out, khvs_el, sizeof(khvs_el));
    return true;
}

// KH_ATTACH_EDEN - the Eden editor does not keep the visual copy (KH_ATTACH_VS_RENDER) of some entities, so while it
// runs every read takes the simulation copy (KH_ATTACH_VS_SIM) alone: kh_attach_raw, which every pose read of a
// followed object goes through (the objects, memory-point proxies and origin helpers of every binding, the chains'
// ends, the crew table's helper and vehicle), and the crew table's own pose commands (kh_crew_sync). framework's
// g_is_eden belongs to the game thread; this is its copy for every thread that reads (the render thread's paint and
// chain-end reads too), stored by the game thread at every Draw3D (flush_frame's head, ahead of its first read) and
// at every command's read (kh_attach_read) - checked each frame, since the editor has no mission start to check it
// once at. Not reset with the session (kh_session_globals_reset's keep list): it copies the framework's state and
// is stored again at the next Draw3D or command read.
static std::atomic<bool> g_attach_eden{ false };
inline bool kh_attach_eden() { return g_attach_eden.load(std::memory_order_relaxed); }
inline void kh_attach_eden_note() {   // Game thread.
    g_attach_eden.store(static_cast<bool>(::g_is_eden), std::memory_order_relaxed);
}

// KH_ATTACH_COPY - the copy of an object's visual state a read takes: the VISUAL copy (KH_ATTACH_VS_RENDER) - the
// engine's own drawn state, the one a Draw3D icon at an object's visual position is placed from - wherever the object
// carries one; the simulation copy only for an object that carries none (a house). The first visual read that lands
// locks the copy for good; until one does, each read tries it and takes the simulation copy for that read, so a slot
// not yet readable at the bind cannot demote an object that has one. No comparison between the copies - with one
// exception, KH_ATTACH_VS_STALE: a visual copy the engine never writes (an object with no model to draw) keeps its
// default state, exactly the world origin, and such a read neither locks nor stands while the simulation copy puts
// the object anywhere else; that read is taken instead, and the next read asks the visual copy again. It locks only
// when the simulation copy reads the origin too; while that copy cannot be read, the read returns false, nothing
// locks, and the next read asks both again. khrw_off is the caller's record of the choice (KH_ATTACH_OFF_NONE: none
// yet), khrw_vb / khrw_bb the page caches of that offset. KH_ATTACH_VS_CACHE: khrw_avb / khrw_abb are the visual
// attempt's own page caches while no visual read has locked (they become khrw_vb / khrw_bb when one does), so an
// object whose visual copy is absent or never written pays no page probe per read once its pages are confirmed -
// probed again when the object or the state pointer changes. False: not readable this time.
// Every read of a followed object comes here: the Draw3D sampler (proxies, parents, a rotation lane's own object),
// the one-shot reads at the commands, the chains, the crew table (KH_CREW_LAG) and the render thread's paint read
// (KH_RT_PAINT_READ). KH_ATTACH_EDEN: in the Eden editor no copy is chosen - the simulation copy alone, and a
// visual lock taken before ends there (the first read outside the editor asks the visual copy again, and locks as
// above).
inline bool kh_attach_raw(const game_value& khrw_gv, uint32_t& khrw_off, uintptr_t& khrw_vb,
                          uintptr_t& khrw_bb, uintptr_t& khrw_avb, uintptr_t& khrw_abb, float khrw_pos[3], float khrw_rot[9],
                          KhAttachDiag* khrw_diag = nullptr) {
    if (KH_ATTACH_VS_RENDER == 0u || KH_ATTACH_VS_SIM == 0u) return false;
    if (!kh_attach_is_obj(khrw_gv)) return false;
    const game_data_object* khrw_gd = static_cast<const game_data_object*>(khrw_gv.data.get());
    // The holder chain get_unit_yaw_speed_sqf walks for its identity key.
    if (!khrw_gd || !khrw_gd->object || !khrw_gd->object->object) return false;
    const uintptr_t khrw_base = reinterpret_cast<uintptr_t>(khrw_gd->object->object);
    if (kh_attach_eden()) {   // KH_ATTACH_EDEN: the simulation copy alone, whatever an earlier read chose.
        if (khrw_off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
            khrw_off = KH_ATTACH_VS_SIM;
            khrw_vb = 0;
            khrw_bb = 0;
        }
        return kh_attach_vs_read(khrw_base, KH_ATTACH_VS_SIM, khrw_vb, khrw_bb, khrw_pos, khrw_rot, khrw_diag);
    }
    if (khrw_off == KH_ATTACH_VS_RENDER) {   // Locked: the visual copy, and nothing else (outside the editor).
        return kh_attach_vs_read(khrw_base, KH_ATTACH_VS_RENDER, khrw_vb, khrw_bb, khrw_pos, khrw_rot, khrw_diag);
    }
    if (kh_attach_vs_read(khrw_base, KH_ATTACH_VS_RENDER, khrw_avb, khrw_abb, khrw_pos, khrw_rot, khrw_diag)) {   // It has one.
        if (khrw_pos[0] == 0.0f && khrw_pos[1] == 0.0f && khrw_pos[2] == 0.0f) {   // KH_ATTACH_VS_STALE.
            float khrw_sp[3], khrw_sr[9];
            KhAttachDiag khrw_sd;
            if (khrw_off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
                khrw_off = KH_ATTACH_VS_SIM;
                khrw_vb = 0;
                khrw_bb = 0;
            }
            // Nothing to compare with: where the object is stays unknown, so this read is not readable (the
            // lanes hold their last pose) and nothing locks - an origin the simulation copy has not confirmed is
            // no proof the visual copy is real, and a lock is for good (outside the editor).
            if (!kh_attach_vs_read(khrw_base, KH_ATTACH_VS_SIM, khrw_vb, khrw_bb, khrw_sp, khrw_sr, &khrw_sd)) return false;
            if (!(khrw_sp[0] == 0.0f && khrw_sp[1] == 0.0f && khrw_sp[2] == 0.0f)) {
                memcpy(khrw_pos, khrw_sp, sizeof(khrw_sp));
                memcpy(khrw_rot, khrw_sr, sizeof(khrw_sr));
                if (khrw_diag) *khrw_diag = khrw_sd;
                return true;
            }
        }
        khrw_off = KH_ATTACH_VS_RENDER;
        khrw_vb = khrw_avb;   // The page caches become the visual copy's.
        khrw_bb = khrw_abb;
        return true;
    }
    if (khrw_off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
        khrw_off = KH_ATTACH_VS_SIM;
        khrw_vb = 0;
        khrw_bb = 0;
    }
    return kh_attach_vs_read(khrw_base, KH_ATTACH_VS_SIM, khrw_vb, khrw_bb, khrw_pos, khrw_rot, khrw_diag);
}

// The one-shot form, for the command sites that have no lane to cache in: addRender3D's position and rotation slots
// (the object named, or the parent a pair form binds to), and updateRender3D's properties - the attach itself
// (kh_attach_apply), the parent of a new memory-point or skeletal binding (sqf_integration.hpp) and the re-seed of a
// mesh already attached (kh_attach_reseed).
inline bool kh_attach_read(const game_value& khar_gv, float khar_pos[3], float khar_rot[9]) {
    kh_attach_eden_note();   // KH_ATTACH_EDEN: the commands' reads (game thread) take this call's flag.
    uint32_t  khar_off = KH_ATTACH_OFF_NONE;
    uintptr_t khar_vb = 0;
    uintptr_t khar_bb = 0;
    uintptr_t khar_avb = 0, khar_abb = 0;
    return kh_attach_raw(khar_gv, khar_off, khar_vb, khar_bb, khar_avb, khar_abb, khar_pos, khar_rot);
}

// rotated is the skip flag the bounds and the vertex path read, so it follows the matrix actually stored - a basis is
// not identity just because no euler was typed.
inline bool kh_attach_rotated(const float khat_m[9]) {
    static const float khat_id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    return memcmp(khat_m, khat_id, sizeof(khat_id)) != 0;
}

// KH_ATTACH_OFFSET - the composition, and the ONLY place it happens (with the position's double twin below,
// kh_attach_offset_pos_d - KH_POS_RES), so the axis convention and the multiplication order are each stated once.
// FIVE call sites each, across four functions: kh_attach_apply here, kh_attach_step (rendering/attachment_steps.hpp),
// kh_attach_reseed below, and the bone seed and the skeletal seed inside kh_apply_render3d_prop in
// sqf_integration.hpp - kh_attach_step's position through the double twin. Each has just taken a raw transform and
// is about to write it into a mesh.
// khoq_basis is the FOLLOWED object's rotation rows exactly as kh_attach_vs_read hands them back - aside / up / dir,
// engine axes - the frame the offset is expressed in. khoq_pos is that object's position in SQF order [x, y, zASL],
// advanced in place. Neither reads the mesh's own rot_m: an offset is relative to the ATTACH POINT, not to whatever
// the mesh is currently rotated to.
inline void kh_attach_offset_pos(const RenderObject& khoq_o, const float khoq_basis[9],
                                 float khoq_pos[3]) {
    if (!khoq_o.attach_pos_on) return;
    // SQF [x, y, z] -> the object's engine-ordered local axes [side, up, forward]: the p = (0, 2, 1) permutation
    // kh_set_rotation and kh_objrec_fill use, its own inverse. Row 0 (aside) is the object's RIGHT, row 1 its up,
    // row 2 its forward.
    const float khoq_l[3] = { khoq_o.attach_pos[0], khoq_o.attach_pos[2], khoq_o.attach_pos[1] };
    float khoq_d[3] = { 0.0f, 0.0f, 0.0f };   // Engine axes (east, up, north).
    for (int khoq_k = 0; khoq_k < 3; ++khoq_k) {
        khoq_d[0] += khoq_l[khoq_k] * khoq_basis[khoq_k * 3 + 0];
        khoq_d[1] += khoq_l[khoq_k] * khoq_basis[khoq_k * 3 + 1];
        khoq_d[2] += khoq_l[khoq_k] * khoq_basis[khoq_k * 3 + 2];
    }
    khoq_pos[0] += khoq_d[0];   // east.
    khoq_pos[1] += khoq_d[2];   // north = SQF y.
    khoq_pos[2] += khoq_d[1];   // up = SQF zASL.
}

// KH_POS_RES: the same offset onto a double position (the step's), so it adds no world-scale rounding.
inline void kh_attach_offset_pos_d(const RenderObject& khod_o, const float khod_basis[9], double khod_pos[3]) {
    if (!khod_o.attach_pos_on) return;
    const double khod_l[3] = { khod_o.attach_pos[0], khod_o.attach_pos[2], khod_o.attach_pos[1] };
    double khod_d[3] = { 0.0, 0.0, 0.0 };   // Engine axes (east, up, north).
    for (int khod_k = 0; khod_k < 3; ++khod_k) {
        khod_d[0] += khod_l[khod_k] * static_cast<double>(khod_basis[khod_k * 3 + 0]);
        khod_d[1] += khod_l[khod_k] * static_cast<double>(khod_basis[khod_k * 3 + 1]);
        khod_d[2] += khod_l[khod_k] * static_cast<double>(khod_basis[khod_k * 3 + 2]);
    }
    khod_pos[0] += khod_d[0];   // east.
    khod_pos[1] += khod_d[2];   // north = SQF y.
    khod_pos[2] += khod_d[1];   // up = SQF zASL.
}

// KH_ATTACH_OFFSET - the rotation twin, in place on the rows just read. Row-vector convention throughout (world =
// centre + local.x * R0 + local.y * R1 + local.z * R2), so a mesh-local vector reaches world as v * attach_rot * basis
// and the product is attach_rot * basis IN THAT ORDER. The other order would turn the mesh about world axes instead
// of the attach point's.
inline void kh_attach_offset_rot(const RenderObject& khor_o, float khor_rot[9]) {
    if (!khor_o.attach_rot_on) return;
    float khor_m[9];
    for (int khor_r = 0; khor_r < 3; ++khor_r) {
        for (int khor_c = 0; khor_c < 3; ++khor_c) {
            khor_m[khor_r * 3 + khor_c] =
                khor_o.attach_rot[khor_r * 3 + 0] * khor_rot[0 * 3 + khor_c] +
                khor_o.attach_rot[khor_r * 3 + 1] * khor_rot[1 * 3 + khor_c] +
                khor_o.attach_rot[khor_r * 3 + 2] * khor_rot[2 * 3 + khor_c];
        }
    }
    memcpy(khor_rot, khor_m, sizeof(khor_m));
}

// KH_SKEL - where a skeletal mesh is DRAWN from, given its root. khsc_pos arrives as the parent's origin with
// attachPosition already composed (the root, SQF order) and leaves as the draw centre: the root plus skel_ctr - the
// centre of the box the drawn buffer is stored in, in the authored frame - turned by the mesh's own rotation. So the
// object's box (pos and size, which every cull and caster bound reads) is the box of the pose being drawn, and a mesh
// drawing its rest buffer (no skin, or before its first upload; skel_ctr is then the import's own box centre) sits at
// its authored place on the parent. Runs after the rotation is final. Game thread (kh_attach_reseed, addRender3D and
// the skeletal seed); the step's twin, kh_skel_centre_d, runs on either thread (kh_attach_step runs on both).
inline void kh_skel_centre(const RenderObject& khsc_o, float khsc_pos[3]) {
    const float* khsc_c = khsc_o.skel_ctr;   // Engine axes.
    float khsc_d[3] = { khsc_c[0], khsc_c[1], khsc_c[2] };
    if (khsc_o.rotated) {   // Row vectors: the offset reaches world as c * rot_m.
        for (int k = 0; k < 3; ++k) {
            khsc_d[k] = khsc_c[0] * khsc_o.rot_m[0 * 3 + k] + khsc_c[1] * khsc_o.rot_m[1 * 3 + k] +
                        khsc_c[2] * khsc_o.rot_m[2 * 3 + k];
        }
    }
    khsc_pos[0] += khsc_d[0];   // east.
    khsc_pos[1] += khsc_d[2];   // north = SQF y.
    khsc_pos[2] += khsc_d[1];   // up = SQF zASL.
}
// KH_POS_RES: the same centre onto a double root (the step's).
inline void kh_skel_centre_d(const RenderObject& khsd_o, double khsd_pos[3]) {
    const float* khsd_c = khsd_o.skel_ctr;   // Engine axes.
    double khsd_d[3] = { khsd_c[0], khsd_c[1], khsd_c[2] };
    if (khsd_o.rotated) {
        for (int k = 0; k < 3; ++k) {
            khsd_d[k] = static_cast<double>(khsd_c[0]) * khsd_o.rot_m[0 * 3 + k] +
                        static_cast<double>(khsd_c[1]) * khsd_o.rot_m[1 * 3 + k] +
                        static_cast<double>(khsd_c[2]) * khsd_o.rot_m[2 * 3 + k];
        }
    }
    khsd_pos[0] += khsd_d[0];
    khsd_pos[1] += khsd_d[2];
    khsd_pos[2] += khsd_d[1];
}

// KH_SKEL - the box of the REST buffer: the import's own size and centre. For the sites where the rest buffer is what
// draws next - a new skeletal binding and a new mesh under one (the substitute's mesh test refuses the old skinned
// buffer the moment the mesh changes). GAME THREAD, on the staged copy a command commits. Staged lookup, as
// kh_apply_native_size.
inline void kh_skel_rest_box(RenderObject& khrb_o) {
    const MeshDef* khrb_sd = kh_mesh_def_staged(khrb_o.mesh);
    const MeshDef& khrb_d = khrb_sd ? *khrb_sd : mesh_def(0);
    for (int k = 0; k < 3; ++k) {
        khrb_o.size[k] = khrb_d.native_size[k];
        khrb_o.skel_ctr[k] = khrb_d.native_ctr[k];
    }
}

// KH_SKEL - a mesh's skeleton as kh_skel_make's filter: the lower-cased bone names, each bone's parent, whether any
// vertex takes weight from it (geom), and the bones that must keep a proxy whatever their geometry (keep: the chain's
// start and end bones, KH_SKEL_PROXY_CULL). Built by kh_skel_bone_names, GAME THREAD (staged lookup).
struct KhSkelBones {
    std::vector<std::string> name;
    std::vector<int32_t>     parent;
    std::vector<uint8_t>     geom;
    std::vector<std::string> keep;
};

// KH_ATTACH_BONE - hand this entry's proxy to the delete queue and clear both bone lanes. Under g_draw_list_mutex.
// Moves out of the entry, so calling it twice on the same entry is a no-op rather than a double delete. Copies a
// game_value and makes no engine call, which is what lets it run from the park; the delete is kh_attach_proxy_reap's.
inline void kh_attach_proxy_retire(KhAttach& khpx_a) {
    // KH_SKEL: a skeletal binding's proxies, every one of them ours as well.
    for (const game_value& khpx_s : khpx_a.skel_proxy) {
        if (!khpx_s.is_nil()) g_attach_proxy_dead.push_back(khpx_s);
    }
    khpx_a.skel_proxy.clear();
    khpx_a.skel_mem.clear();
    khpx_a.skel = false;
    if (!khpx_a.proxy.is_nil()) g_attach_proxy_dead.push_back(khpx_a.proxy);
    g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()),
                                std::memory_order_relaxed);
    khpx_a.proxy = game_value();
    khpx_a.bone_parent = game_value();
}

// Is the rotation lane the BINDING's - following the single proxy, or the parent under a skeletal binding - rather
// than an object the script pointed it at through the "rotation" property? The lanes are independent by contract, so
// only a binding's lane is a binding's to re-point or clear. Under g_draw_list_mutex.
inline bool kh_attach_rot_is_binding(const KhAttach& khrb_a) {
    if (khrb_a.obj_rot.is_nil()) return false;
    if (!khrb_a.proxy.is_nil() && khrb_a.obj_rot.data.get() == khrb_a.proxy.data.get()) return true;
    return khrb_a.skel && !khrb_a.bone_parent.is_nil() &&
           khrb_a.obj_rot.data.get() == khrb_a.bone_parent.data.get();
}
// KH_SKEL_ROT_OWN - does the rotation lane follow the POSITION lane's object (the plain "rotation true" lane,
// KH_ATTACH_PLAIN_ROT)? kh_attach_bone_rot's plain-arm test, which applies with no binding only: the same value, or
// two values of one object (separately evaluated lanes). Under g_draw_list_mutex.
inline bool kh_attach_rot_is_plain(const KhAttach& khrp_a) {
    if (khrp_a.skel || !khrp_a.proxy.is_nil()) return false;   // A binding's lanes are the binding rule's.
    if (khrp_a.obj_rot.is_nil() || khrp_a.obj_pos.is_nil()) return false;
    if (khrp_a.obj_rot.data.get() == khrp_a.obj_pos.data.get()) return true;
    const uintptr_t khrp_b = kh_attach_base_of(khrp_a.obj_rot);
    return khrp_b != 0u && khrp_b == kh_attach_base_of(khrp_a.obj_pos);
}
// Queue one helper reference for kh_attach_proxy_reap (release, delete at the last). Under g_draw_list_mutex.
inline void kh_attach_org_drop(game_value& khod_h) {
    if (khod_h.is_nil()) return;
    g_attach_proxy_dead.push_back(khod_h);
    g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()), std::memory_order_relaxed);
    khod_h = game_value();
}
// A lane's new helper (nil: none), replacing the one it had, stamped with khop_from (kh_attach_org_make). The same
// helper again (the pool's, for the same character) keeps its stamp. Under g_draw_list_mutex.
inline void kh_attach_org_put(KhAttach& khop_a, bool khop_rot, game_value& khop_h, const game_value& khop_for,
                              uint64_t khop_from) {
    game_value& khop_slot = khop_rot ? khop_a.org_rot : khop_a.org_pos;
    const bool khop_same = !khop_slot.is_nil() && !khop_h.is_nil() && khop_slot.data.get() == khop_h.data.get();
    kh_attach_org_drop(khop_slot);
    khop_slot = khop_h;
    khop_h = game_value();
    (khop_rot ? khop_a.org_rot_for : khop_a.org_pos_for) = khop_slot.is_nil() ? nullptr : khop_for.data.get();
    if (!khop_same) (khop_rot ? khop_a.org_rot_seq : khop_a.org_pos_seq) = khop_from;
}
// Retire every helper that no longer stands in for its lane: the position helper serves a plain binding's position
// object or a skeletal binding's parent (both the position lane's object; a memory-point binding places from its
// own helper and reads its parent only as a check); the rotation helper serves the rotation lane's object when that
// is not the position lane's (whose read already has it). Under g_draw_list_mutex; after every lane change.
inline void kh_attach_org_sync(KhAttach& khos_a) {
    const bool khos_pos_ok = khos_a.skel || khos_a.bone_parent.is_nil();
    if (!khos_a.org_pos.is_nil() &&
        (!khos_pos_ok || khos_a.obj_pos.is_nil() || khos_a.obj_pos.data.get() != khos_a.org_pos_for)) {
        kh_attach_org_drop(khos_a.org_pos);
        khos_a.org_pos_for = nullptr;
    }
    if (!khos_a.org_rot.is_nil() &&
        (khos_a.obj_rot.is_nil() || khos_a.obj_rot.data.get() != khos_a.org_rot_for ||
         khos_a.obj_rot.data.get() == khos_a.obj_pos.data.get())) {
        kh_attach_org_drop(khos_a.org_rot);
        khos_a.org_rot_for = nullptr;
    }
}
// Both helpers, when the entry ends. Under g_draw_list_mutex.
inline void kh_attach_org_retire(KhAttach& khor_a) {
    kh_attach_org_drop(khor_a.org_pos);
    kh_attach_org_drop(khor_a.org_rot);
    khor_a.org_pos_for = nullptr;
    khor_a.org_rot_for = nullptr;
}

// Set or clear one lane. A value that is not a live-typed OBJECT clears it; an entry with both lanes clear is erased,
// so g_attach_n counts exactly the meshes the step walks and the objects are released as soon as they stop being
// followed. KH_PLAIN_ROT_CARRY: khas_end_plain (a position lane's detach) also clears a plain "rotation true" lane
// (kh_attach_rot_is_plain, on the entry as it stood), under the same lock - as a second call clearing the
// rotation lane would.
inline void kh_attach_set(const std::string& khas_h, const game_value& khas_gv, bool khas_rot,
                          bool khas_end_plain = false) {
    game_value khas_org;   // KH_ATTACH_MAN_HELPER: made before the mutex (SQF calls); every path below files it.
    uint64_t   khas_from = 0u;
    if (kh_attach_is_obj(khas_gv) && !kh_attach_obj_dead(khas_gv)) kh_attach_org_make(khas_gv, khas_org, khas_from);
    std::lock_guard<std::mutex> khas_g(g_draw_list_mutex);
    const bool khas_on = kh_attach_is_obj(khas_gv);
    auto khas_it = g_attach.find(khas_h);
    if (khas_it == g_attach.end()) {
        if (!khas_on) { kh_attach_org_drop(khas_org); return; }
        khas_it = g_attach.emplace(khas_h, KhAttach()).first;
    }
    const bool khas_plain = khas_end_plain && !khas_rot && !kh_attach_rot_is_binding(khas_it->second) &&
                            kh_attach_rot_is_plain(khas_it->second);   // KH_PLAIN_ROT_CARRY.
    // KH_ATTACH_BONE: the proxy exists to BE the position lane's object, so pointing that lane anywhere else ends the
    // bone attachment. Guarded on identity rather than on the flag, because kh_attach_bone_set installs the proxy
    // through its own path and must not be undone here. The rotation lane is deliberately not covered: a script
    // setting a plain rotation while the position still follows a bone keeps the attachment and stops reading its rows.
    if (!khas_rot && !khas_it->second.proxy.is_nil() &&
        khas_gv.data.get() != khas_it->second.proxy.data.get()) {
        // The rotation lane goes with it only when it followed the proxy: one the script pointed at an object of
        // its own is independent (the skeletal branch below, kh_attach_bone_set's rule).
        const bool khas_pb = kh_attach_rot_is_binding(khas_it->second);
        kh_attach_proxy_retire(khas_it->second);
        if (khas_pb) khas_it->second.obj_rot = game_value();
    }
    // KH_SKEL: a skeletal binding's position lane follows the parent itself, so identity cannot tell the binding from
    // a plain attach to the same object - and the SQF side clears RenderObject::skel on every position that is not the
    // binding. Any position set through here therefore ends it, with the rotation lane when that was the binding's.
    if (!khas_rot && khas_it->second.skel) {
        const bool khas_rb = kh_attach_rot_is_binding(khas_it->second);
        kh_attach_proxy_retire(khas_it->second);
        if (khas_rb) khas_it->second.obj_rot = game_value();
    }
    game_value& khas_lane = khas_rot ? khas_it->second.obj_rot : khas_it->second.obj_pos;
    khas_lane = khas_on ? khas_gv : game_value();
    kh_attach_org_put(khas_it->second, khas_rot, khas_org, khas_gv, khas_from);   // KH_ATTACH_MAN_HELPER (nil: none).
    kh_attach_org_sync(khas_it->second);
    if (khas_plain) {   // KH_PLAIN_ROT_CARRY: the rotation lane cleared, as kh_attach_set(h, nil, true) clears it.
        game_value khas_none;
        khas_it->second.obj_rot = game_value();
        kh_attach_org_put(khas_it->second, true, khas_none, khas_none, 0u);
        kh_attach_org_sync(khas_it->second);
    }
    if (khas_it->second.obj_pos.is_nil() && khas_it->second.obj_rot.is_nil()) {
        // KH_ATTACH_BONE: the erase drops the only reference to the entry's proxy, so retire first or the object leaks
        // into the world with nothing left that knows its name. The guard above already retires on every path that
        // can empty the position lane, so this is unreachable today; keep it: a leak here would be silent and
        // permanent, and a later edit adding a rotation-side clear would not think to look at this erase.
        kh_attach_proxy_retire(khas_it->second);
        kh_attach_org_retire(khas_it->second);   // KH_ATTACH_MAN_HELPER.
        g_attach.erase(khas_it);
    }
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// Under g_draw_list_mutex, at every site that erases a draw-list entry: the attachment dies with the mesh. Nothing else
// drops it - the step skips an orphan and never erases (g_attach's THREADS note) - so a site that left this out would
// hold the object (and any proxy, standing on its parent) until the next kh_attach_drop_all.
inline void kh_attach_drop(const std::string& khad_h) {
    if (g_attach.empty()) return;
    auto khad_it = g_attach.find(khad_h);
    if (khad_it == g_attach.end()) return;
    kh_attach_proxy_retire(khad_it->second);   // KH_ATTACH_BONE: ours to delete.
    kh_attach_org_retire(khad_it->second);     // KH_ATTACH_MAN_HELPER: likewise.
    g_attach.erase(khad_it);
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// khad_retire distinguishes the two callers. clear_render_objects is a SCRIPT clearing its meshes mid-mission: its
// proxies are live game objects that nothing but us will delete, so they must be queued. reset_retained_state is a
// MISSION EDGE: the engine is about to destroy every object in the world, the drain will not run again, and queueing
// there would hold SQF references to process exit (g_attach's rule).
inline void kh_attach_drop_all(bool khad_retire) {   // Under g_draw_list_mutex.
    if (khad_retire) {
        for (auto& khad_kv : g_attach) {
            kh_attach_proxy_retire(khad_kv.second);
            kh_attach_org_retire(khad_kv.second);   // KH_ATTACH_MAN_HELPER.
        }
    } else {
        g_attach_proxy_dead.clear();
        g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> khpc_l(g_pxy_pool_mu);   // KH_PXY_SMOOTH.
            g_pxy_pool.clear();   // The mission edge (game thread).
        }
        g_pxy_pool_key.clear();
        g_pxy_pool_n.store(0, std::memory_order_relaxed);
    }
    g_attach.clear();
    g_attach_n.store(0, std::memory_order_relaxed);
    // The handles the queue names are about to stop existing.
    g_attach_dead.clear();
    g_attach_dead_n.store(0, std::memory_order_relaxed);
}

// KH_ATTACH_DEAD, the queue side. Under g_draw_list_mutex, from EITHER thread - which is why it only ever appends a
// string. Deduplicated because the step runs several times a frame: at most one entry per doomed mesh however long
// the reap takes to arrive.
inline void kh_attach_dead_queue(const std::string& khdq_h) {
    for (const std::string& khdq_s : g_attach_dead) if (khdq_s == khdq_h) return;
    g_attach_dead.push_back(khdq_h);
    g_attach_dead_n.store(static_cast<uint32_t>(g_attach_dead.size()), std::memory_order_relaxed);
}

// KH_ATTACH_DEAD, the drain: GAME THREAD ONLY, once per flush_frame (the Draw3D handler), and the only place a mesh
// dies of its object's death - the thread the SQF commands mutate g_attach from, so the game_values kh_attach_drop
// releases are released where they were taken. The erase is the one remove_render_object performs, in the same
// order, so a reaped mesh leaves the scene exactly as one removed by removeRenderHandler does.
// KH_ATTACH_DEAD_RECHECK: the queue holds a verdict a step took on the binding as it stood then; a script may since
// have detached the mesh or pointed it at a live object (updateRender3D between the object's deletion and this
// Draw3D). The reap re-takes the step's own test on the binding as it stands now, under the same lock the commands
// write it under, and a mesh whose binding no longer follows a dead object stays - the one direction the note above
// KH_ATTACH_DEAD forbids getting wrong.
inline void kh_attach_reap() {
    if (g_attach_dead_n.load(std::memory_order_relaxed) == 0) return;
    std::lock_guard<std::mutex> khar_g(g_draw_list_mutex);
    for (const std::string& khar_h : g_attach_dead) {
        const auto khar_at = g_attach.find(khar_h);
        if (khar_at == g_attach.end()) continue;   // Detached since: nothing follows a dead object.
        const KhAttach& khar_a = khar_at->second;
        if (!kh_attach_obj_dead(khar_a.obj_pos) && !kh_attach_obj_dead(khar_a.obj_rot) &&
            !kh_attach_obj_dead(khar_a.bone_parent)) continue;   // Re-pointed since at live objects.
        auto khar_it = g_draw_list.find(khar_h);
        // The mesh went first (a script or a mission edge removed it); the attachment still has to go, or the object
        // is held to teardown.
        if (khar_it == g_draw_list.end()) { kh_attach_drop(khar_h); continue; }
        kh_scene_stage_remove(khar_it->second);
        kh_attach_drop(khar_h);
        g_draw_list.erase(khar_it);
    }
    g_attach_dead.clear();
    g_attach_dead_n.store(0, std::memory_order_relaxed);
}

// KH_ATTACH_BONE, the delete side: GAME THREAD ONLY, once per flush_frame (the Draw3D handler), and the only place a
// proxy is destroyed. It sits BEFORE the park because sqf::delete_vehicle is an engine call. The queue is swapped out
// under the mutex and drained outside it: deleteVehicle can re-enter our own SQF commands (an event handler on the
// proxy's death calling removeRenderHandler), and those take g_draw_list_mutex, which is not recursive.
inline void kh_attach_proxy_reap() {
    if (g_attach_proxy_dead_n.load(std::memory_order_relaxed) == 0) return;
    std::vector<game_value> khpr_take;
    {
        std::lock_guard<std::mutex> khpr_g(g_draw_list_mutex);
        khpr_take.swap(g_attach_proxy_dead);
        g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
    }
    for (game_value& khpr_p : khpr_take) {
        if (khpr_p.is_nil()) continue;
        // A proxy whose parent was deleted may already be gone; deleting a null object is a no-op in SQF. The try is
        // the renderer's rule for every engine call made from a frame path.
        if (!kh_pxy_pool_release(khpr_p)) continue;   // Another binding still follows it.
        try { sqf::delete_vehicle(static_cast<object>(khpr_p)); } catch (...) {}
    }
}

// KH_ATTACH_BONE - queue a proxy that never reached a KhAttach entry. In every command that attaches to a memory
// point there is a window - kh_attach_bone_make has created the object and attachTo has bound it to the parent, but
// kh_attach_bone_set has not yet put it in a lane - in which NOTHING names it (not g_attach, not the draw list, not
// the dead queue), so a command that returns there leaves a KH_ATTACH_BONE_SHAPE standing on the parent's memory
// point for the rest of the mission, invisible to every removal path (kh_attach_drop, kh_attach_drop_all).
// GAME THREAD, holding NO lock: from a command entry point (KhProxyOwn, KhSkelOwn; kh_rv_chain_sim directly) and from
// flush_frame's kh_crew_sync (a KH_CREW_LAG entry's helper reference) and kh_chain_prepare (a removed chain's proxy).
// It takes g_draw_list_mutex only to append; the delete still happens in kh_attach_proxy_reap, outside every lock
// (g_attach_proxy_dead's note). Do not delete here: that would be safe at today's call sites and would stop being safe
// the first time one of them moves.
inline void kh_attach_proxy_orphan(game_value& khpo_p) {
    if (khpo_p.is_nil()) return;
    {
        std::lock_guard<std::mutex> khpo_g(g_draw_list_mutex);
        g_attach_proxy_dead.push_back(khpo_p);
        g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()),
                                    std::memory_order_relaxed);
    }
    khpo_p = game_value();
}

// The owner of that window, so the guarantee is STRUCTURAL rather than one exit at a time: addRender3D has nine
// returns after the make and before the set, plus two catch blocks. With a live proxy only the mesh parse's return
// is reachable: the make's own failure return holds no proxy, and the other seven are unreachable only because of
// the shape of an if/else chain (and the khr_skel test), which a later edit can change without noticing. Every path
// out of the command runs this destructor instead. Declare it only where g_draw_list_mutex is NOT held: the
// destructor takes that mutex and it is not recursive. All three declaration sites hold nothing: addRender3D, and
// kh_apply_render3d_prop and kh_rv_chain_sim, which updateRender3D reaches only after releasing the lock.
struct KhProxyOwn {
    game_value proxy;
    KhProxyOwn() {}
    KhProxyOwn(const KhProxyOwn&) = delete;
    KhProxyOwn& operator=(const KhProxyOwn&) = delete;
    ~KhProxyOwn() { kh_attach_proxy_orphan(proxy); }
    // Hands ownership on, so an installed proxy is never double-queued.
    game_value release() {
        game_value khpn_p = proxy;
        proxy = game_value();
        return khpn_p;
    }
};

// KH_PXY_POOL - the proxy for (parent, memory point): the pool's live one, or a new one made and attached. GAME
// THREAD ONLY. False with a caller-facing sentence in err; the caller owns the reference (it ends in
// g_attach_proxy_dead).
inline bool kh_pxy_pool_get(const game_value& khpg_parent, const std::string& khpg_mem, game_value& khpg_out,
                            const char* khpg_article, std::string& err) {
    const uintptr_t khpg_pb = kh_attach_base_of(khpg_parent);
    std::string khpg_low = khpg_mem;
    khpg_low = kh_lower_copy(std::move(khpg_low));
    const std::string khpg_key = std::to_string(static_cast<unsigned long long>(khpg_pb)) + "|" + khpg_low;
    const auto khpg_kit = g_pxy_pool_key.find(khpg_key);
    if (khpg_kit != g_pxy_pool_key.end()) {
        const auto khpg_eit = g_pxy_pool.find(khpg_kit->second);
        if (khpg_eit != g_pxy_pool.end() && khpg_pb != 0 && kh_attach_base_of(khpg_eit->second.parent) == khpg_pb &&
            !kh_attach_obj_dead(khpg_eit->second.parent) && !kh_attach_obj_dead(khpg_eit->second.proxy)) {
            ++khpg_eit->second.refs;
            khpg_out = khpg_eit->second.proxy;
            return true;
        }
        g_pxy_pool_key.erase(khpg_kit);   // Not shareable; its entry drains through the reap.
    }
    const object khpg_s = sqf::create_vehicle_local(KH_ATTACH_BONE_SHAPE, vector3(0.0f, 0.0f, 0.0f));
    sqf::enable_simulation(khpg_s, false);
    sqf::set_physics_collision_flag(khpg_s, false);
    // KH_PXY_HIDE - the helper exists for the transform the engine maintains for it (read from its visual state, not
    // from any draw). Hiding it drops its submission and every constant-buffer upload of its matrix. MEASURED: 420
    // game-thread samples whose parent moved, NONE with a helper that did not move with it - the engine refreshes a
    // hidden attached child exactly as it does a drawn one.
    sqf::hide_object(khpg_s, true);
    if (sqf::is_null(khpg_s)) {
        err = std::string("could not create ") + khpg_article + " memory-point proxy (" +
              KH_ATTACH_BONE_SHAPE + " missing from the loaded mods?)";
        return false;
    }
    // The spawn position is discarded by the attach on the next simulation step.
    sqf::attach_to(khpg_s, static_cast<object>(khpg_parent), vector3(0.0f, 0.0f, 0.0f), khpg_mem, !khpg_mem.empty());
    KhPxyPoolEnt khpg_e;
    khpg_e.proxy = khpg_s;
    khpg_e.parent = khpg_parent;
    khpg_e.refs = 1u;
    khpg_e.made_seq = g_gts_seq;
    khpg_e.bone = !khpg_mem.empty();   // KH_PXY_SMOOTH: an origin helper is not estimated.
    khpg_e.key = khpg_key;
    khpg_out = khpg_e.proxy;
    const uintptr_t khpg_xb = kh_attach_base_of(khpg_e.proxy);
    if (khpg_xb != 0 && khpg_pb != 0) {
        // A filed entry here is a deleted, unreaped proxy whose address was reused: end it now, or its key could hand
        // this proxy out for its own memory point.
        const auto khpg_old = g_pxy_pool.find(khpg_xb);
        if (khpg_old != g_pxy_pool.end()) kh_pxy_pool_erase(khpg_old);
        {
            std::lock_guard<std::mutex> khpg_l(g_pxy_pool_mu);   // KH_PXY_SMOOTH.
            g_pxy_pool[khpg_xb] = khpg_e;
        }
        g_pxy_pool_key[khpg_key] = khpg_xb;
        g_pxy_pool_n.store(static_cast<uint32_t>(g_pxy_pool.size()), std::memory_order_relaxed);
    }
    return true;
}

// KH_ATTACH_MAN_HELPER (declared above kh_attach_set). The helper is attached at the character's origin with no
// memory point - where its visual state is placed, and what a plain lane reads - and pooled under that empty point.
inline void kh_attach_org_make(const game_value& khom_gv, game_value& khom_out, uint64_t& khom_from) {
    khom_out = game_value();
    khom_from = 0u;
    try {
        if (!sqf::is_kind_of(static_cast<object>(khom_gv), "Man")) return;
        std::string khom_err;
        if (!kh_pxy_pool_get(khom_gv, std::string(), khom_out, "a character's", khom_err)) khom_out = game_value();
    } catch (...) {
        khom_out = game_value();
    }
    if (khom_out.is_nil()) return;
    const auto khom_it = g_pxy_pool.find(kh_attach_base_of(khom_out));
    if (khom_it != g_pxy_pool.end()) khom_from = khom_it->second.made_seq + 1u;
}

// KH_NULL_SILENT - an attach target that is null (or whose transform cannot be read) is no fault the script hears
// of: the command still fails - nothing created, the property not applied - and reports nothing. A function below
// that finds one puts this marker in err instead of a sentence (kh_attach_bone_make, kh_skel_make,
// kh_attach_apply); sqf_integration.hpp sets it at its own such sites, and every report site there says nothing
// for it (kh_err_silent).
static constexpr const char* KH_ERR_SILENT = "KH_ERR_SILENT";
inline bool kh_err_silent(const std::string& khes_e) { return khes_e == KH_ERR_SILENT; }

// KH_ATTACH_BONE - the memory-point proxy. GAME THREAD ONLY (SQF calls). False with a caller-facing sentence in err
// (KH_ERR_SILENT for a null parent). follow_bone is always true: the proxy tracks the full transform, and the
// script's rotation flag only selects whether the rotation lane reads it, so toggling rotation never re-attaches.
inline bool kh_attach_bone_make(const game_value& khbm_parent, const std::string& khbm_mem,
                                game_value& khbm_proxy, std::string& err) {
    if (!kh_attach_is_obj(khbm_parent)) {
        err = "position [object, memoryPoint] needs a game object in the first slot";
        return false;
    }
    if (kh_attach_obj_dead(khbm_parent)) {
        err = KH_ERR_SILENT;   // KH_NULL_SILENT.
        return false;
    }
    if (khbm_mem.empty()) {
        err = "position [object, memoryPoint] needs a non-empty memory point name";
        return false;
    }
    return kh_pxy_pool_get(khbm_parent, khbm_mem, khbm_proxy, "the", err);
}

// KH_SKEL - the owner of a skeletal binding's proxies between their creation and kh_attach_skel_set (KhProxyOwn's
// reason and rule: declare it only where g_draw_list_mutex is NOT held).
struct KhSkelOwn {
    std::vector<game_value> proxies;
    KhSkelOwn() {}
    KhSkelOwn(const KhSkelOwn&) = delete;
    KhSkelOwn& operator=(const KhSkelOwn&) = delete;
    ~KhSkelOwn() { for (game_value& khso_p : proxies) kh_attach_proxy_orphan(khso_p); }
    std::vector<game_value> release() {
        std::vector<game_value> khso_r;
        khso_r.swap(proxies);
        return khso_r;
    }
};

// KH_SKEL_PROXY_CULL - which of the parent's memory points get a proxy. The memory points a bone of the mesh names
// are matched (case-insensitively, a repeated name once); of those, a proxy is made only for a bone the skin would
// DRIVE something through: kh_skin_drive moves every bone by the nearest matched bone up its chain (itself included),
// so the matched bones that matter are exactly the nearest matched ancestor-or-self of each bone that carries vertex
// weight (geom) or sits in the keep set (a chain's start and end, which the chain follows through their memory
// points - or their nearest named ancestor's - whether or not they skin a vertex). Any other matched bone drives
// nothing a vertex reads, and its proxy would be a game object made, attached and read every frame for no pixel.
// Bit-identical for the drawn pose, the cloth guide (read through vertex influences) and the chain's start and end:
// for a bone with geometry the nearest kept bone up its chain IS its nearest matched one, since the kept set is a
// subset of the matched set that contains it. Bones with no geometry and no keep entry may be driven from farther up
// the chain - nothing reads their map but the chain's reset placement of a body without geometry, which the
// simulation takes over on the next step. khsp_out is the memory points, lower-cased, in the parent's list order,
// with their names as the list spells them (khsp_raw, for attachTo). One SQF call (the selection list). GAME THREAD.
inline bool kh_skel_plan(const object& khsp_p, const KhSkelBones& khsp_b,
                         std::vector<std::string>& khsp_out, std::vector<std::string>& khsp_raw) {
    khsp_out.clear();
    khsp_raw.clear();
    const size_t khsp_nb = khsp_b.name.size();
    if (khsp_nb == 0u) return true;   // No skeleton: nothing to follow, nothing to spawn.
    const auto khsp_names = sqf::selection_names(khsp_p, sqf::rv_selection_lods::Memory);
    // The matched bones, and the list entry each matched through.
    std::vector<int32_t> khsp_at(khsp_nb, -1);
    std::vector<std::string> khsp_low(khsp_names.size());
    for (size_t khsp_i = 0; khsp_i < khsp_names.size(); ++khsp_i) {
        std::string khsp_l = khsp_names[khsp_i];
        khsp_l = kh_lower_copy(std::move(khsp_l));
        khsp_low[khsp_i] = khsp_l;
        if (khsp_l.empty()) continue;
        for (size_t khsp_k = 0; khsp_k < khsp_nb; ++khsp_k) {
            if (khsp_at[khsp_k] < 0 && khsp_b.name[khsp_k] == khsp_l) khsp_at[khsp_k] = static_cast<int32_t>(khsp_i);
        }
    }
    // The bones that drive something: up each geometric bone's chain to its nearest matched bone (bounded by the bone
    // count, as kh_skin_drive is).
    std::vector<uint8_t> khsp_need(khsp_nb, 0u);
    for (size_t khsp_k = 0; khsp_k < khsp_nb; ++khsp_k) {
        const bool khsp_keep = std::find(khsp_b.keep.begin(), khsp_b.keep.end(), khsp_b.name[khsp_k]) != khsp_b.keep.end();
        if (!(khsp_k < khsp_b.geom.size() && khsp_b.geom[khsp_k]) && !khsp_keep) continue;
        int32_t khsp_c = static_cast<int32_t>(khsp_k);
        for (size_t khsp_s = 0; khsp_s <= khsp_nb && khsp_c >= 0 && static_cast<size_t>(khsp_c) < khsp_nb; ++khsp_s) {
            if (khsp_at[static_cast<size_t>(khsp_c)] >= 0) { khsp_need[static_cast<size_t>(khsp_c)] = 1u; break; }
            khsp_c = khsp_c < static_cast<int32_t>(khsp_b.parent.size()) ? khsp_b.parent[static_cast<size_t>(khsp_c)] : -1;
        }
    }
    // In the parent's list order, the entry each needed bone matched through.
    for (size_t khsp_i = 0; khsp_i < khsp_names.size(); ++khsp_i) {
        bool khsp_any = false;
        for (size_t khsp_k = 0; khsp_k < khsp_nb && !khsp_any; ++khsp_k) {
            if (khsp_need[khsp_k] && khsp_at[khsp_k] == static_cast<int32_t>(khsp_i)) khsp_any = true;
        }
        if (!khsp_any) continue;
        khsp_out.push_back(khsp_low[khsp_i]);
        khsp_raw.push_back(khsp_names[khsp_i]);
    }
    return true;
}

// KH_SKEL - build a skeletal binding's proxies: one per memory point of the parent's Memory LOD that kh_skel_plan
// keeps (KH_SKEL_PROXY_CULL), each made and attached exactly as kh_attach_bone_make makes its one (follow-bone, so
// the proxy carries the bone's animated rotation). GAME THREAD ONLY: one SQF call for the list and six per kept
// memory point that needs a new proxy (kh_pxy_pool_get; none where the pool shares a live one), so every caller is a
// command entry point holding no lock. False with a caller-facing sentence in err (KH_ERR_SILENT for a null parent);
// every proxy made before a failure is already in khsm_proxies, where the caller's KhSkelOwn queues it for deletion.
inline bool kh_skel_make(const game_value& khsm_parent, const KhSkelBones& khsm_bones,
                         std::vector<game_value>& khsm_proxies, std::vector<std::string>& khsm_mems,
                         std::string& err) {
    if (!kh_attach_is_obj(khsm_parent)) {
        err = "position [object, true] needs a game object in the first slot";
        return false;
    }
    if (kh_attach_obj_dead(khsm_parent)) {
        err = KH_ERR_SILENT;   // KH_NULL_SILENT.
        return false;
    }
    const object khsm_p = static_cast<object>(khsm_parent);
    std::vector<std::string> khsm_plan, khsm_raw;
    kh_skel_plan(khsm_p, khsm_bones, khsm_plan, khsm_raw);
    for (size_t khsm_i = 0; khsm_i < khsm_plan.size(); ++khsm_i) {
        game_value khsm_x;
        if (!kh_pxy_pool_get(khsm_parent, khsm_raw[khsm_i], khsm_x, "a", err)) return false;
        khsm_proxies.push_back(khsm_x);   // Owned from here, whatever happens next.
        khsm_mems.push_back(khsm_plan[khsm_i]);
    }
    return true;
}

// KH_ATTACH_BONE - install a finished proxy on a handle, both lanes at once: they name the same proxy, and setting
// them one at a time through kh_attach_set would retire the proxy between the two calls.
inline void kh_attach_bone_set(const std::string& khbs_h, const game_value& khbs_proxy,
                               const game_value& khbs_parent, bool khbs_rot) {
    std::lock_guard<std::mutex> khbs_g(g_draw_list_mutex);
    auto khbs_it = g_attach.find(khbs_h);
    if (khbs_it == g_attach.end()) khbs_it = g_attach.emplace(khbs_h, KhAttach()).first;
    // A rotation lane following something OTHER than the OLD proxy is the script's own (the "rotation" property), and
    // the lanes are independent by contract - so a position change hands it back untouched. Only a lane that was
    // following the old proxy is the bone's to re-point.
    const bool khbs_was_bone = kh_attach_rot_is_binding(khbs_it->second);   // KH_SKEL: either kind.
    const game_value khbs_keep = khbs_was_bone ? game_value() : khbs_it->second.obj_rot;
    // A re-pointed bone attachment's old proxy stops being anything's lane here and is ours to delete.
    kh_attach_proxy_retire(khbs_it->second);
    khbs_it->second.proxy = khbs_proxy;
    khbs_it->second.bone_parent = khbs_parent;
    khbs_it->second.obj_pos = khbs_proxy;
    const game_value khbs_new_rot = khbs_rot ? khbs_proxy : khbs_keep;
    khbs_it->second.obj_rot = khbs_new_rot;
    kh_attach_org_sync(khbs_it->second);   // KH_ATTACH_MAN_HELPER.
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// KH_SKEL - install a skeletal binding on a handle: its proxies, the parent in the position lane, and the parent in
// the rotation lane when khss_rot (the root follows the parent's rotation). Replaces any binding the handle had,
// single or skeletal, retiring its proxies; a rotation lane the script pointed at its own object is handed back
// untouched when khss_rot is false, as kh_attach_bone_set does.
inline void kh_attach_skel_set(const std::string& khss_h, std::vector<game_value>&& khss_proxies,
                               const std::vector<std::string>& khss_mems,
                               const game_value& khss_parent, bool khss_rot) {
    game_value khss_org;   // KH_ATTACH_MAN_HELPER: the parent's, made before the mutex (SQF calls); filed below.
    uint64_t   khss_from = 0u;
    if (kh_attach_is_obj(khss_parent) && !kh_attach_obj_dead(khss_parent)) kh_attach_org_make(khss_parent, khss_org, khss_from);
    std::lock_guard<std::mutex> khss_g(g_draw_list_mutex);
    auto khss_it = g_attach.find(khss_h);
    if (khss_it == g_attach.end()) khss_it = g_attach.emplace(khss_h, KhAttach()).first;
    KhAttach& khss_a = khss_it->second;
    const game_value khss_keep = kh_attach_rot_is_binding(khss_a) ? game_value() : khss_a.obj_rot;
    kh_attach_proxy_retire(khss_a);
    khss_a.skel = true;
    khss_a.skel_proxy = std::move(khss_proxies);
    khss_a.skel_mem = khss_mems;
    khss_a.skel_gen = ++g_skel_gen_serial;
    khss_a.bone_parent = khss_parent;
    khss_a.obj_pos = khss_parent;
    khss_a.obj_rot = khss_rot ? khss_parent : khss_keep;
    kh_attach_org_put(khss_a, false, khss_org, khss_parent, khss_from);   // KH_ATTACH_MAN_HELPER (nil: none).
    kh_attach_org_sync(khss_a);
    // A stash taken this cycle was the old binding's: the first render-thread step with a sample of this binding that
    // reads its pose re-stashes it (KH_SKEL_SAMPLE's rule), and a drawer's step re-skins it.
    khss_a.skel_smp_cycle = ~0ull;
    khss_a.skin_culled = false;   // KH_SKIN_CULL: the old binding's; the next step decides for this one.
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// KH_ATTACH_BONE - the rotation toggle. Points the rotation lane at the proxy the position lane already follows, or
// clears it, WITHOUT touching the attachment (why the proxy is attached follow-bone once and never again). With no
// binding it names the object the position lane follows (KH_ATTACH_PLAIN_ROT below). False when the handle has no
// attachment, or no binding and a position lane that follows nothing - the boolean then names no object.
inline bool kh_attach_bone_rot(const std::string& khbr_h, bool khbr_on) {
    std::lock_guard<std::mutex> khbr_g(g_draw_list_mutex);
    auto khbr_it = g_attach.find(khbr_h);
    if (khbr_it == g_attach.end()) return false;
    // KH_SKEL: under a skeletal binding the boolean is whether the ROOT follows the parent's rotation; the bones
    // follow their proxies either way.
    const bool khbr_skel = khbr_it->second.skel;
    const bool khbr_bind = khbr_skel || !khbr_it->second.proxy.is_nil();
    // KH_ATTACH_PLAIN_ROT - with no binding the boolean's subject is the object the POSITION lane follows: true turns
    // the mesh with it, false stops, as the two binding forms mean (a script would otherwise have to spell a second
    // copy of the object). Refused only where the position lane follows nothing.
    if (!khbr_bind && khbr_it->second.obj_pos.is_nil()) return false;
    if (!khbr_on) {
        // FALSE means "stop following the bone", and nothing more: a rotation lane the script pointed at some other
        // object is not this command's to clear (bone_set's independence). The plain lane reads by the same rule,
        // against the position's object.
        const bool khbr_mine = khbr_bind
            ? kh_attach_rot_is_binding(khbr_it->second)
            : (!khbr_it->second.obj_rot.is_nil() &&
               (khbr_it->second.obj_rot.data.get() == khbr_it->second.obj_pos.data.get() ||
                // KH_ATTACH_PLAIN_ROT: two values of one object are one object (separately evaluated lanes).
                (kh_attach_base_of(khbr_it->second.obj_rot) != 0u &&
                 kh_attach_base_of(khbr_it->second.obj_rot) == kh_attach_base_of(khbr_it->second.obj_pos))));
        if (!khbr_mine) return true;
        khbr_it->second.obj_rot = game_value();
    } else if (khbr_bind) {
        khbr_it->second.obj_rot = khbr_skel ? khbr_it->second.bone_parent : khbr_it->second.proxy;
    } else {
        khbr_it->second.obj_rot = khbr_it->second.obj_pos;   // KH_ATTACH_PLAIN_ROT.
    }
    kh_attach_org_sync(khbr_it->second);   // KH_ATTACH_MAN_HELPER.
    return true;
}

// KH_ATTACH_BONE - does this handle's rotation lane currently follow its bone? Read by the "position" property so
// that re-pointing an existing bone attachment at a new object or memory point PRESERVES the follow-rotation state.
// False for a handle with no bone attachment (also the right answer for a fresh one). The test is IDENTITY against
// the proxy, not a non-nil lane: a script may point the rotation lane at a different object while the position
// follows a bone, and answering true for that lane would let the position property overwrite it.
inline bool kh_attach_bone_rot_state(const std::string& khbq_h) {
    std::lock_guard<std::mutex> khbq_g(g_draw_list_mutex);
    auto khbq_it = g_attach.find(khbq_h);
    if (khbq_it == g_attach.end()) return false;
    return kh_attach_rot_is_binding(khbq_it->second);   // KH_SKEL: either kind of binding.
}

// KH_SKEL - does this handle carry a binding of either kind? Read by the "position" property: a new skeletal binding
// starts with its root following the parent's rotation, unless it replaces a binding, whose state it keeps, or the
// rotation lane follows an object of the script's own (kh_attach_rot_own), which it keeps.
inline bool kh_attach_has_binding(const std::string& khhb_h) {
    std::lock_guard<std::mutex> khhb_g(g_draw_list_mutex);
    auto khhb_it = g_attach.find(khhb_h);
    if (khhb_it == g_attach.end()) return false;
    return khhb_it->second.skel || !khhb_it->second.proxy.is_nil();
}

// KH_SKEL_ROT_OWN - does this handle's rotation lane follow an object of the script's own (set through "rotation"),
// rather than a binding's or the position lane's (a plain "rotation true")? A new position never changes such a lane,
// so a new skeletal binding leaves it in place (kh_attach_skel_set keeps it when told not to follow the parent), as
// re-pointing a bone does.
inline bool kh_attach_rot_own(const std::string& khro_h) {
    if (g_attach_n.load(std::memory_order_relaxed) == 0u) return false;   // Game thread: no entry, no lane.
    std::lock_guard<std::mutex> khro_g(g_draw_list_mutex);
    auto khro_it = g_attach.find(khro_h);
    if (khro_it == g_attach.end()) return false;
    const KhAttach& khro_a = khro_it->second;
    // A plain "rotation true" lane (KH_ATTACH_PLAIN_ROT) follows the POSITION's object: the position's, not the
    // script's own - a new binding takes it over (kh_attach_rot_is_plain).
    return !khro_a.obj_rot.is_nil() && !kh_attach_rot_is_binding(khro_a) && !kh_attach_rot_is_plain(khro_a);
}
// KH_SKEL_ROT_OWN - is this handle's rotation lane the plain "rotation true" one (it follows the position lane's
// object)? A new single-bone binding carries that true over - the mesh turns with the new proxy (the bone), as a
// skeletal binding's root does by default - instead of keeping the lane as an object of the script's own, which
// "rotation false" under the binding could then never clear. KH_PLAIN_ROT_CARRY: a new object position carries
// it too (the lane follows the new object).
inline bool kh_attach_rot_plain(const std::string& khrp_h) {
    if (g_attach_n.load(std::memory_order_relaxed) == 0u) return false;   // Game thread: no entry, no lane.
    std::lock_guard<std::mutex> khrp_g(g_draw_list_mutex);
    auto khrp_it = g_attach.find(khrp_h);
    return khrp_it != g_attach.end() && !kh_attach_rot_is_binding(khrp_it->second) &&
           kh_attach_rot_is_plain(khrp_it->second);
}

// Attach a lane AND take the transform now, so the mesh is in place on the frame the command runs. False when the
// object cannot be read (null, or not readable now), with KH_ERR_SILENT in err: the property fails without a report
// (KH_NULL_SILENT), and nothing is attached.
inline bool kh_attach_apply(const std::string& khaa_h, const game_value& khaa_gv,
                            bool khaa_rot, RenderObject& khaa_o, std::string& err) {
    float khaa_p[3], khaa_r[9];
    if (!kh_attach_read(khaa_gv, khaa_p, khaa_r)) {
        err = KH_ERR_SILENT;   // KH_NULL_SILENT.
        return false;
    }
    if (khaa_rot) {
        kh_attach_offset_rot(khaa_o, khaa_r);   // KH_ATTACH_OFFSET: inside the frame just read.
        memcpy(khaa_o.rot_m, khaa_r, sizeof(khaa_o.rot_m));
        khaa_o.rotated = kh_attach_rotated(khaa_r);
    } else {
        kh_attach_offset_pos(khaa_o, khaa_r, khaa_p);   // KH_ATTACH_OFFSET.
        khaa_o.pos[0] = khaa_p[0];
        khaa_o.pos[1] = khaa_p[1];
        khaa_o.pos[2] = khaa_p[2];
    }
    kh_attach_set(khaa_h, khaa_gv, khaa_rot);
    return true;
}

// KH_ATTACH_OFFSET - this handle's two lane objects, copied out. GAME THREAD ONLY: copying a game_value touches the
// SQF allocator (the rule that keeps kh_attach_step to strings). A handle with no attachment leaves both out-params
// nil, which makes the re-seed below a no-op for it. The copy is taken under the mutex and USED outside it, so the raw
// reads and their page walks never run with g_draw_list_mutex held from a command.
// KH_ATTACH_SEED_PARENT - khal_placed (the re-seed's reads): a memory-point proxy not yet past the sample that first
// sees it (kh_attach_org_read's rule, made_seq + 1) still stands at its spawn point, so a lane naming it is handed the
// parent instead, as the addRender3D bone seed reads.
inline void kh_attach_lanes(const std::string& khal_h, game_value& khal_pos, game_value& khal_rot,
                            bool khal_placed = false) {
    std::lock_guard<std::mutex> khal_g(g_draw_list_mutex);
    auto khal_it = g_attach.find(khal_h);
    if (khal_it == g_attach.end()) return;
    khal_pos = khal_it->second.obj_pos;
    khal_rot = khal_it->second.obj_rot;
    const KhAttach& khal_a = khal_it->second;
    if (khal_placed && !khal_a.skel && !khal_a.proxy.is_nil() && !khal_a.bone_parent.is_nil()) {
        const auto khal_pe = g_pxy_pool.find(kh_attach_base_of(khal_a.proxy));   // The game thread's own read.
        if (khal_pe != g_pxy_pool.end() && g_gts_seq <= khal_pe->second.made_seq + 1u) {
            const void* const khal_x = khal_a.proxy.data.get();
            if (khal_pos.data.get() == khal_x) khal_pos = khal_a.bone_parent;
            if (!khal_rot.is_nil() && khal_rot.data.get() == khal_x) khal_rot = khal_a.bone_parent;
        }
    }
}

// KH_ATTACH_OFFSET - re-take this mesh's transform from whatever it already follows, so a changed offset lands on the
// frame the command runs. GAME THREAD ONLY (kh_attach_lanes), takes no lock itself past that copy, and makes NO SQF
// call - the reads are the step's raw, page-gated ones, with fresh caches (a command has no lane to cache in). A lane
// that is not attached, or whose read fails this instant, is left as it is: the step corrects it on its next pass,
// and the mesh never jumps somewhere neither value says.
inline void kh_attach_reseed(const std::string& khrs_h, RenderObject& khrs_o) {
    game_value khrs_lp, khrs_lr;
    kh_attach_lanes(khrs_h, khrs_lp, khrs_lr, true);   // KH_ATTACH_SEED_PARENT.
    float khrs_p[3], khrs_r[9];
    float khrs_root[3] = { 0.0f, 0.0f, 0.0f };
    bool  khrs_centre = false;   // KH_SKEL: the draw centre waits for the rotation below.
    if (!khrs_lp.is_nil() && kh_attach_read(khrs_lp, khrs_p, khrs_r)) {
        kh_attach_offset_pos(khrs_o, khrs_r, khrs_p);
        if (khrs_o.skel) {
            memcpy(khrs_root, khrs_p, sizeof(khrs_root));
            khrs_centre = true;
        } else {
            khrs_o.pos[0] = khrs_p[0];
            khrs_o.pos[1] = khrs_p[1];
            khrs_o.pos[2] = khrs_p[2];
        }
    }
    if (!khrs_lr.is_nil() && kh_attach_read(khrs_lr, khrs_p, khrs_r)) {
        kh_attach_offset_rot(khrs_o, khrs_r);
        memcpy(khrs_o.rot_m, khrs_r, sizeof(khrs_o.rot_m));
        khrs_o.rotated = kh_attach_rotated(khrs_r);   // The COMPOSED matrix, not the raw one.
    }
    if (khrs_centre) {
        kh_skel_centre(khrs_o, khrs_root);
        khrs_o.pos[0] = khrs_root[0];
        khrs_o.pos[1] = khrs_root[1];
        khrs_o.pos[2] = khrs_root[2];
    }
}

// KH_ATTACH_DIAG, one object read's outcome. Under g_draw_list_mutex (the Draw3D sampler).
inline void kh_attach_diag_note(bool khdn_ok, const KhAttachDiag& khdn_d) {
    if (!kh_stats_on()) return;
    if (!khdn_ok) g_attach_refused.fetch_add(1u, std::memory_order_relaxed);
    else if (khdn_d.repaired) g_attach_repaired.fetch_add(1u, std::memory_order_relaxed);
    float khdn_max = 0.0f;
    const uint32_t khdn_bits = g_attach_skew_bits.load(std::memory_order_relaxed);
    memcpy(&khdn_max, &khdn_bits, sizeof(khdn_max));
    if (khdn_d.skew > khdn_max) {
        uint32_t khdn_nb = 0;
        memcpy(&khdn_nb, &khdn_d.skew, sizeof(khdn_nb));
        g_attach_skew_bits.store(khdn_nb, std::memory_order_relaxed);
    }
}
}