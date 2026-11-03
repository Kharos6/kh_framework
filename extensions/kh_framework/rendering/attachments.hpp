#pragma once

// The injection half (rendering_integration_injection.hpp): ATTACH: attachments and proxies, the skeleton reads and
// the engine visual-state reads. Never include it on its own.
namespace RenderIntegration {

// ATTACH_MAN_HELPER - a character's origin helper, for a lane about to name gv: the pool's (shared by every
// lane on that character), or none for anything that is not a Man, or when the helper cannot be made (the lane then
// reads the character itself). GAME THREAD, holding no lock (SQF calls); defined after pxy_pool_get.
// The caller owns the reference and files it: into a lane (attach_org_put) or the delete queue. from: the
// first sample that saw the helper (its pool entry's made_seq + 1), 0 when not known.
inline void attach_org_make(const game_value& gv, game_value& out, uint64_t& from);

// The read gate: true only when every page spanning [p, p + n) is committed and readable, so an address that is not
// one can be rejected instead of faulted on. Do not delete it: it is load-bearing on the live path. attach_vs_read
// gates BOTH of its dereferences on it (raw reads at measured offsets into engine memory, every frame a mesh follows
// an object). It lives in the injection half because that is the one that provably has windows.h.
inline bool probe_readable(const void* p, size_t n) {
    if (!p || n == 0) return false;
    const uint8_t* a = static_cast<const uint8_t*>(p);
    const uint8_t* end = a + n;
    if (end < a) return false;   // Wrapped: not an address.
    while (a < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(a, &mbi, sizeof(mbi)) != sizeof(mbi)) return false;
        if (mbi.State != MEM_COMMIT) return false;
        if (mbi.Protect & PAGE_GUARD) return false;
        const DWORD pr = mbi.Protect & 0xFFu;
        if (pr != PAGE_READONLY && pr != PAGE_READWRITE && pr != PAGE_WRITECOPY &&
            pr != PAGE_EXECUTE_READ && pr != PAGE_EXECUTE_READWRITE &&
            pr != PAGE_EXECUTE_WRITECOPY) return false;
        const uint8_t* next = static_cast<const uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
        if (next <= a) return false;   // No progress: stop rather than spin.
        a = next;
    }
    return true;
}

// ATTACH - a mesh bound to a game object's transform. The objects are read once a frame at Draw3D, on the game
// thread (ATTACH_GT_SNAP), and the mesh is placed at the points the scene is consumed - the injection and the cast
// fire on the render thread, the flush when a cycle takes the late path (the render thread's flush, or a park's on
// the game thread) - every one of them drawing the cycle's one pick (gts_for_cycle): the frame's Draw3D sample, or,
// while it has not landed, the render thread's own read of the objects (RT_PAINT_READ). Position and rotation
// attach independently and may name different objects; an empty lane keeps whatever the script last set there.
// ONE reader, attach_raw, takes the transform straight out of the engine's visual state at a MEASURED offset: no
// SQF call anywhere on this path, which is the point of the feature (the cost of following many objects). A lane
// whose read fails its own checks leaves the mesh where it is. It reads the RENDER copy where the object has one -
// the transform the engine draws the object from, not the one the simulation has already advanced to, so a moving
// vehicle's mesh sits on the vehicle instead of ahead of it (attach_raw has the numbers); in the Eden editor the
// SIMULATION copy alone (ATTACH_EDEN).
// ANCHOR: a mesh sits at the visual state's own origin, which is the object's WORLD position (getPosWorld: the model
// centre) in the same [x, y, zASL] frame as every mesh position - above getPosASL (the land contact) by a per-model
// constant (0 m on a man, 0.503 m on a house, 1.797 m and 2.430 m on two vehicles; measured). Nothing here reads
// getPosASL. That origin, the point the engine draws the model from, is the only anchor the object's own memory
// defines. Do not cancel the offset by measuring it against getPosASL: it costs one SQF read per lane and a "wait
// until the object holds still" gate to take it safely.
// The attachment lives beside the draw list, not inside RenderObject: RenderObject is copied wholesale on every scene
// sync, staging copy and flush pass, several of them on the render thread, and a game_value member would put a
// refcount touch on all of them. Here the objects are held once, by handle, under g_draw_list_mutex.
// THREADS: the table's entries and lanes are WRITTEN only by the game thread (the SQF commands via attach_set and
// its bone / skeletal / toggle siblings, the draw-list erase sites via attach_drop). It is READ by both:
// attach_step runs on the render thread from the injection, the cast fire and the other drawers (writing only an
// entry's own render-thread fields: its picks, reads, stamps and skin scratch), and on the game thread from
// park_run. The step therefore only ever COPIES a sampled transform and SKIPS orphans - it must never erase,
// because releasing a game_value on the render thread touches the SQF allocator off its own thread. If that function
// ever gains an erase, that is the bug.
#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
#define KH_ATTACH_VS_RENDER 0x1A0u
#define KH_ATTACH_VS_SIM    0xD0u
#else
#define KH_ATTACH_VS_RENDER 0u
#define KH_ATTACH_VS_SIM    0u
#endif
// The Draw3D sampler's serial (attach_gt_snap): each sample's GtSnap::seq. Game thread only. Reset with the
// objects (session_objects_reset), not the session: the bindings hold its values (their rings and picks, the
// helper stamps Attach::org_*_seq and PxyPoolEnt::made_seq), and the deferred teardown leaves the next mission's
// bindings standing.
static uint64_t g_gts_seq = 0;
static uint32_t g_skel_gen_serial = 0;   // SKEL: game thread only (the commands).

// ATTACH_DEAD - handles whose followed object the engine has emptied, waiting for the game thread to remove them.
// std::string only: the step that fills this runs on the render thread, and a handle is the one thing about an
// attachment that can be copied there without touching the SQF allocator. Drained by attach_reap, game thread only.
static std::vector<std::string> g_attach_dead;   // Under g_draw_list_mutex.
// The reap's gate, the twin of g_attach_n: a session where nothing ever dies pays one relaxed load per frame and takes
// no lock.
static std::atomic<uint32_t> g_attach_dead_n{ 0 };

// ATTACH_BONE - proxies whose attachment has ended, waiting for the game thread to deleteVehicle them. game_value
// and not a handle: once the Attach entry goes, this vector is the only thing that knows the object exists.
// RETIRING and DELETING a proxy have different requirements: a retire happens wherever an attachment ends, under
// g_draw_list_mutex (in a park, under the graphics lock too); the delete is sqf::delete_vehicle, which may not run
// under either (an SQF command re-entering on this thread would take the mutex again). So the retire only moves a
// reference into this vector, and attach_proxy_reap drains it from flush_frame holding nothing, releasing one pool
// reference per entry.
// EXIT_STATICS: never destructed (g_attach's rule - SQF references must not be released at static teardown).
static std::vector<game_value>& g_attach_proxy_dead = *(new std::vector<game_value>());   // Under g_draw_list_mutex.
// The drain's gate, the twin of g_attach_dead_n: a session that never attaches to a bone pays one relaxed load per
// frame and takes no lock.
static std::atomic<uint32_t> g_attach_proxy_dead_n{ 0 };

// The memory-point proxy's class: a local simple object the engine attaches and maintains a transform for. It is never
// drawn (PXY_HIDE) and the transform is read from its visual state, so its model is not a contract: any class the
// engine will create locally and attach will do.
static const char* const ATTACH_BONE_SHAPE = "KH_HelperRender";

// ATTACH_DIAG - what the Draw3D sampler's object reads (gts_read) came back as, for getRenderStats
// (attachRepaired / attachRefused / attachSkew). Written only by the sampler, on the game thread under
// g_draw_list_mutex; atomics because getRenderStats and resetRenderStats read and zero them without it. Collected
// only while stats are armed, like every g_stats counter.
struct AttachDiag {
    float skew = 0.0f;       // Largest |row length - 1| of the basis read.
    bool  repaired = false;  // A row outside 1 % of unit length: re-orthonormalised.
};
static std::atomic<uint64_t> g_attach_repaired{ 0 };   // Reads taken only because the basis was repaired.
static std::atomic<uint64_t> g_attach_refused{ 0 };    // Reads refused (what follows: attach_gt_snap).
static std::atomic<uint32_t> g_attach_skew_bits{ 0 };  // Float bits: the largest skew seen.

// One test for "is this slot an object to follow", used by every site that asks. is_nil FIRST: nil is a documented
// value in both slots (a nil rotation is the identity), and every parser beside this one orders the two tests this way.
inline bool attach_is_obj(const game_value& gv) {
    return !gv.is_nil() && gv.type_enum() == game_data_type::OBJECT;
}

// PXY_POOL - one proxy per (parent, memory point), shared by every binding on that point, attached with zero
// offset - following the bone for a memory point, at the origin for none (ATTACH_MAN_HELPER). g_pxy_pool (by
// proxy) counts the holders; attach_proxy_reap releases one per ended proxy and deletes at the last.
// g_pxy_pool_key (parent | lower-case memory point) finds a live proxy to share; an entry with a dead parent or proxy
// is not shared. A helper carries no scale of its own: it is hidden (PXY_HIDE), so nothing has to tell its matrix
// from a followed object's. GAME THREAD ONLY, but for the render thread's estimator step (PXY_SMOOTH: a find and
// the step under g_pxy_pool_mu). g_pxy_pool holds game_values and g_pxy_pool_key the proxy bases (both never
// destructed, like g_attach); the mission edge empties them.
struct PxyPoolEnt {
    game_value  proxy;
    game_value  parent;
    uint32_t    refs = 0;
    std::string key;
    uint64_t    made_seq = 0;   // g_gts_seq when it was made and attached (ATTACH_MAN_HELPER's stamp).
    // PXY_SMOOTH: a memory-point proxy (bone) and its estimator, per SQF axis - the last reading, the target and
    // which way it last moved, the tracker's output and velocity (m/s); the sample it last ran for, that sample's
    // stamp and delta (output minus reading). Stepped under g_pxy_pool_mu (pxy_smooth_note) by the game thread
    // (the sampler, a chain's end read) and the render thread (its reads, RT_PAINT_READ); ends with the entry.
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
    // PXY_REPLAY: the estimator as it stood before sm_seq's step, so a later reading of that sample steps again
    // from there (pxy_smooth).
    bool        smp_ok = false;
    int64_t     smp_q = 0;
    int8_t      smp_dir[3] = {};
    float       smp_r[3] = {};
    double      smp_t[3] = {};
    double      smp_x[3] = {};
    double      smp_v[3] = {};
};
static std::unordered_map<uintptr_t, PxyPoolEnt>& g_pxy_pool =
    *(new std::unordered_map<uintptr_t, PxyPoolEnt>());
static std::unordered_map<std::string, uintptr_t>& g_pxy_pool_key =
    *(new std::unordered_map<std::string, uintptr_t>());
static std::atomic<uint32_t> g_pxy_pool_n{ 0 };
// PXY_SMOOTH: the estimator is stepped from the game thread (the sampler, a chain's end) and the render thread
// (its reads, RT_PAINT_READ): every step, and every insert into or erase from g_pxy_pool (game thread), holds
// this. Held for map operations and the step alone - no engine call, no other lock taken inside it.
static std::mutex g_pxy_pool_mu;
// ATTACH_ROW_MAX - the longest basis row a visual-state read admits (attach_vs_eval). A followed object the
// script scaled carries its scale in these rows, so the band is generous: a little over 2x was the widest measured,
// and nothing beyond 4 has ever been asked for.
static constexpr float ATTACH_ROW_MAX = 4.0f;
// SESSION_RESET / VS_MEMO: the read memo's epoch, bumped by every session teardown (never reset, so no thread's
// stamp can match a later epoch by accident); a thread whose memo is from an earlier epoch empties it at its next read.
static std::atomic<uint32_t> g_vs_memo_epoch{ 0 };
// An object's identity for the caches and the sample ring: its engine object's address, 0 for none.
inline uintptr_t attach_base_of(const game_value& gv) {
    if (!attach_is_obj(gv)) return 0;
    const game_data_object* gd = static_cast<const game_data_object*>(gv.data.get());
    if (!gd || !gd->object || !gd->object->object) return 0;
    return reinterpret_cast<uintptr_t>(gd->object->object);
}
// One pool entry's end: its key if it still names it, and the entry.
inline void pxy_pool_erase(std::unordered_map<uintptr_t, PxyPoolEnt>::iterator it) {
    PxyPoolEnt& e = it->second;
    const auto kit = g_pxy_pool_key.find(e.key);
    if (kit != g_pxy_pool_key.end() && kit->second == it->first) g_pxy_pool_key.erase(kit);
    {
        std::lock_guard<std::mutex> l(g_pxy_pool_mu);   // PXY_SMOOTH: the render thread steps entries.
        g_pxy_pool.erase(it);
    }
    g_pxy_pool_n.store(static_cast<uint32_t>(g_pxy_pool.size()), std::memory_order_relaxed);
}
// The reap's release: true = no holder left (or never pooled), the caller deletes. A proxy the engine already deleted
// reads base 0, so its entry is found by reference instead.
inline bool pxy_pool_release(const game_value& gv) {
    const uintptr_t b = attach_base_of(gv);
    auto it = b != 0 ? g_pxy_pool.find(b) : g_pxy_pool.end();
    if (b == 0 && !gv.is_nil()) {
        for (auto s = g_pxy_pool.begin(); s != g_pxy_pool.end(); ++s) {
            if (s->second.proxy.data.get() == gv.data.get()) { it = s; break; }
        }
    }
    if (it == g_pxy_pool.end()) return true;
    PxyPoolEnt& e = it->second;
    if (e.refs > 1u) { --e.refs; return false; }
    pxy_pool_erase(it);
    return true;
}

// ATTACH_DEAD - has the engine emptied this lane's object, as opposed to the read merely having failed this frame?
// A true answer DESTROYS a mesh, so it is given only on positive evidence that the holder is empty, never on any form
// of 'could not read it this time' (a failed page probe, a failed basis check, an offset yet to be chosen): each of
// those is a transient that must leave the mesh where it is, which attach_raw returning false already does. The
// test is aimed at the harmless failure direction: MISSING a death leaves a mesh pinned to a dead object; inventing
// one deletes a mesh the script still owns and cannot get back. The holder chain is the same liveness test
// get_unit_yaw_speed_sqf and update_unit_states use to decide an object is not worth reading, and the one
// attach_raw refuses to proceed past; here it is treated as a DEATH rather than a skip.
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
inline bool attach_obj_dead(const game_value& gv) {
    if (gv.is_nil()) return false;
    if (gv.type_enum() != game_data_type::OBJECT) return false;
    // Our own game_value holds the reference, so the game_data_object itself cannot have been freed under us; only
    // the links below it empty out.
    const game_data_object* gd = static_cast<const game_data_object*>(gv.data.get());
    if (!gd) return true;
    if (!gd->object) return true;
    return gd->object->object == nullptr;
}

// ATTACH_RAW - the object's transform straight out of the engine's visual state, which is what makes attaching
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
// 0x1A0 is not universal (a house reports only 0xD0), so the copy is chosen per object (ATTACH_COPY,
// attach_raw): 0x1A0 wherever it reads, locked at the first read that lands, and 0xD0 - which every object tested
// carries - for an object with no 0x1A0. In the Eden editor, 0xD0 alone (ATTACH_EDEN).

// VS_MEMO - two entries per set, the set picked by the state pointer: the 14 floats last validated for a state and
// the answer. An entry is used only when both the pointer and every byte of the floats match, so a hit is the
// evaluation's own result. A miss replaces the state's own entry when the set holds one, else the set's older
// entry. 8,192 entries (~1.1 MB) per thread that reads (the
// game thread: the Draw3D sampler, the commands, the chains, the crew table; the render thread: the paint read,
// RT_PAINT_READ) keep a scene of a few thousand followed objects and helpers resident; heap-allocated on the
// thread's first read and kept for the process, zeroed (v = 0 never matches a state), and emptied again at the
// thread's first read after a session teardown (g_vs_memo_epoch).
struct VsMemoEnt {
    uintptr_t    v = 0;
    float        f[14] = {};
    bool         ok = false;
    AttachDiag diag;   // attach_vs_eval sets it on every path, so every filled entry (v != 0) holds one.
    float        pos[3] = {}, rot[9] = {}, len[3] = {};
};
static constexpr uint32_t VS_MEMO_SETS = 4096u;   // Power of two.
struct VsMemo {
    VsMemoEnt e[VS_MEMO_SETS][2];
    uint8_t     old[VS_MEMO_SETS] = {};   // The way a miss replaces next.
};
// g_vs_memo_epoch is defined with ATTACH_ROW_MAX above.
inline VsMemo* vs_memo() {
    static thread_local VsMemo* m = nullptr;
    static thread_local uint32_t e = 0;
    const uint32_t now = g_vs_memo_epoch.load(std::memory_order_relaxed);
    if (!m) {
        m = new (std::nothrow) VsMemo();   // Null = no memo on this thread; every read evaluates.
        e = now;
    } else if (e != now) {
        reinit(*m);
        e = now;
    }
    return m;
}
// VS_MEMO - the validation of one visual-state read, a pure function of the 14 floats read (the row ceiling is a
// constant): the rows' lengths and skew (the diag, set on every path), the band, finite and square tests, and the
// re-orthonormalisation. pos / rot / len are written only on success. attach_vs_read memoises it per thread
// (VsMemo), so a read whose bytes match the last read of the same state costs a compare instead of the square roots
// and divides - every helper shared by many bindings, every parent the draw sweep reads, every still object - and
// returns exactly what the evaluation would.
inline bool attach_vs_eval(const float* f, float pos[3], float rot[9],
                           AttachDiag* diag, float* len_out) {
    // The result is checked, not assumed: three rows of plausible length that stand well apart, and a finite
    // position. A read that fails this leaves the lane where it is for the frame.
    // The rows need NOT be orthonormal - the contract with bones. A plain object's are (to 7.5e-05 on the three
    // objects measured), but a ATTACH_BONE proxy is attached with the bone's rotation, and the engine hands it the
    // bone's ANIMATED matrix, which blended poses shorten and shear as the pose bends. Do not require 1 % unit rows:
    // that refuses those reads and the mesh holds its last transform until the pose comes back. So a basis is accepted
    // within a generous band and re-orthonormalised - forward (dir) kept, up made square to it, aside square to both
    // with its own sign - and a mesh only ever takes a rotation. On an orthonormal read this changes nothing beyond
    // float rounding, and a still object still hands back the same floats every frame.
    float len[3];
    float skew = 0.0f;
    bool  fix = false;
    for (int r = 0; r < 3; ++r) {
        const float* row = f + r * 3;
        const float l2 = row[0] * row[0] + row[1] * row[1] + row[2] * row[2];
        len[r] = sqrtf(l2);
        if (!(l2 > 0.98f && l2 < 1.02f)) fix = true;   // Outside the old unit test.
        const float dev = fabsf(len[r] - 1.0f);
        if (dev > skew) skew = dev;
    }
    if (diag) { diag->skew = skew; diag->repaired = fix; }
    const float vs_max = ATTACH_ROW_MAX;
    for (int r = 0; r < 3; ++r) {
        if (!(len[r] >= 0.5f && len[r] <= vs_max)) return false;   // NaN fails too.
    }
    for (int i = 9; i < 12; ++i) {
        if (f[i] != f[i] || fabsf(f[i]) > 1.0e7f) return false;
    }
    const float* a = f;       // aside
    const float* u = f + 3;   // up
    const float* d = f + 6;   // dir
    // Pairwise within 30 degrees of square (|cos| under 0.5), which a sheared pose stays well inside and an unrelated
    // triple of floats almost never is.
    if (fabsf(cloth_v3_dot(a, u)) >= 0.5f * len[0] * len[1] ||
        fabsf(cloth_v3_dot(a, d)) >= 0.5f * len[0] * len[2] ||
        fabsf(cloth_v3_dot(u, d)) >= 0.5f * len[1] * len[2]) return false;
    float o[9];
    for (int k = 0; k < 3; ++k) o[6 + k] = d[k] / len[2];
    const float ud = cloth_v3_dot(u, o + 6);
    for (int k = 0; k < 3; ++k) o[3 + k] = u[k] - ud * o[6 + k];
    const float ul = cloth_v3_len(o + 3);   // >= 0.866 of up's length, by the test above.
    for (int k = 0; k < 3; ++k) o[3 + k] /= ul;
    const float ad = cloth_v3_dot(a, o + 6);
    const float au = cloth_v3_dot(a, o + 3);
    for (int k = 0; k < 3; ++k) o[k] = a[k] - ad * o[6 + k] - au * o[3 + k];
    const float al = cloth_v3_len(o);
    if (!(al >= 0.5f * len[0])) return false;   // aside nearly in the plane of the other two.
    for (int k = 0; k < 3; ++k) o[k] /= al;
    memcpy(rot, o, sizeof(o));   // aside / up / dir = rot_m rows 0 / 1 / 2.
    pos[0] = f[9];    // east.
    pos[1] = f[11];   // north = SQF y.
    pos[2] = f[10];   // up = SQF zASL.
    if (len_out) memcpy(len_out, len, sizeof(len));   // Before the re-orthonormalisation.
    return true;
}

// One visual state, structure only - no reference, nothing assumed but the measured layout recorded above. pos
// comes back in RenderObject::pos's [x, y, zASL] and IS the anchor - no correction toward another anchor (getPosASL)
// is applied to it afterwards. vb caches the last state pointer confirmed readable, and it stays confirmed only
// while reads through it validate: a pointer whose bytes do not read as a state (0x1A0 on an object class nobody
// measured) is probed again on its next read (ATTACH_VS_CACHE). bb is the last object base whose slot at
// off was confirmed.
// BOTH dereferences are page-gated. This is the one site in either header that dereferences gd->object->object rather
// than using it as an opaque identity key, and the offsets are measured, not declared: nothing guarantees a given
// object extends to 0x1A0 (a house carries no render copy there), so the slot read is gated like the state read. Both
// gates are keyed on their pointer, so a followed object costs the page walk when it changes and not per frame.
inline bool attach_vs_read(uintptr_t base, uint32_t off, uintptr_t& vb,
                           uintptr_t& bb, float pos[3], float rot[9],
                           AttachDiag* diag = nullptr, float* len_out = nullptr) {
    if (off == 0u) return false;
    if (base != bb) {   // Changed (or first sight): confirm the slot before reading it.
        if (!probe_readable(reinterpret_cast<const void*>(base + off),
                            sizeof(uintptr_t))) return false;
        bb = base;
    }
    uintptr_t v = 0;
    memcpy(&v, reinterpret_cast<const void*>(base + off), sizeof(v));
    if (v == 0 || (v % sizeof(uintptr_t)) != 0) return false;
    if (v != vb) {   // Changed (or first sight): confirm the pages before trusting it.
        if (!probe_readable(reinterpret_cast<const void*>(v + sizeof(uintptr_t)),
                            sizeof(float) * 14)) return false;
        vb = v;
    }
    float f[14];
    memcpy(f, reinterpret_cast<const void*>(v + sizeof(uintptr_t)), sizeof(f));
    // VS_MEMO: the same 14 floats validate to the same answer.
    VsMemo* const m = vs_memo();
    VsMemoEnt* e = nullptr;
    if (m) {
        const uint32_t s = static_cast<uint32_t>(((v >> 4) ^ (v >> 16)) & (VS_MEMO_SETS - 1u));
        for (uint32_t w = 0; w < 2u; ++w) {
            VsMemoEnt& c = m->e[s][w];
            if (c.v == v && memcmp(c.f, f, sizeof(f)) == 0) {
                m->old[s] = static_cast<uint8_t>(w ^ 1u);   // The other way ages.
                if (diag) *diag = c.diag;   // A hit is a filled entry (v != 0).
                if (!c.ok) { vb = 0; return false; }   // Not a state: confirmed no longer.
                memcpy(rot, c.rot, sizeof(c.rot));
                memcpy(pos, c.pos, sizeof(c.pos));
                if (len_out) memcpy(len_out, c.len, sizeof(c.len));
                return true;
            }
        }
        // A miss: this state's own entry if it has one (its bytes moved on), else the set's older way.
        uint32_t w = m->old[s];
        if (m->e[s][0].v == v) w = 0u;
        else if (m->e[s][1].v == v) w = 1u;
        e = &m->e[s][w];
        m->old[s] = static_cast<uint8_t>(w ^ 1u);
    }
    AttachDiag dg;
    float ep[3] = {}, er[9] = {}, el[3] = {};
    const bool ok = attach_vs_eval(f, ep, er, &dg, el);
    if (e) {
        e->v = v;
        memcpy(e->f, f, sizeof(f));
        e->ok = ok;
        e->diag = dg;
        memcpy(e->pos, ep, sizeof(ep));
        memcpy(e->rot, er, sizeof(er));
        memcpy(e->len, el, sizeof(el));
    }
    if (diag) *diag = dg;
    if (!ok) { vb = 0; return false; }   // Not a state: confirmed no longer.
    memcpy(rot, er, sizeof(er));
    memcpy(pos, ep, sizeof(ep));
    if (len_out) memcpy(len_out, el, sizeof(el));
    return true;
}

// ATTACH_EDEN - the Eden editor does not keep the visual copy (KH_ATTACH_VS_RENDER) of some entities, so while it
// runs every read takes the simulation copy (KH_ATTACH_VS_SIM) alone: attach_raw, which every pose read of a
// followed object goes through (the objects, memory-point proxies and origin helpers of every binding, the chains'
// ends, the crew table's helper and vehicle), and the crew table's own pose commands (crew_sync). framework's
// g_is_eden belongs to the game thread; this is its copy for every thread that reads (the render thread's paint and
// chain-end reads too), stored by the game thread at every Draw3D (flush_frame's head, ahead of its first read) and
// at every command's read (attach_read) - checked each frame, since the editor has no mission start to check it
// once at. Not reset with the session (session_globals_reset's keep list): it copies the framework's state and
// is stored again at the next Draw3D or command read.
static std::atomic<bool> g_attach_eden{ false };
inline bool attach_eden() { return g_attach_eden.load(std::memory_order_relaxed); }
inline void attach_eden_note() {   // Game thread.
    g_attach_eden.store(static_cast<bool>(::g_is_eden), std::memory_order_relaxed);
}

// ATTACH_COPY - the copy of an object's visual state a read takes: the VISUAL copy (KH_ATTACH_VS_RENDER) - the
// engine's own drawn state, the one a Draw3D icon at an object's visual position is placed from - wherever the object
// carries one; the simulation copy only for an object that carries none (a house). The first visual read that lands
// locks the copy for good; until one does, each read tries it and takes the simulation copy for that read, so a slot
// not yet readable at the bind cannot demote an object that has one. No comparison between the copies - with one
// exception, ATTACH_VS_STALE: a visual copy the engine never writes (an object with no model to draw) keeps its
// default state, exactly the world origin, and such a read neither locks nor stands while the simulation copy puts
// the object anywhere else; that read is taken instead, and the next read asks the visual copy again. It locks only
// when the simulation copy reads the origin too; while that copy cannot be read, the read returns false, nothing
// locks, and the next read asks both again. off is the caller's record of the choice (KH_ATTACH_OFF_NONE: none
// yet), vb / bb the page caches of that offset. ATTACH_VS_CACHE: avb / abb are the visual
// attempt's own page caches while no visual read has locked (they become vb / bb when one does), so an
// object whose visual copy is absent or never written pays no page probe per read once its pages are confirmed -
// probed again when the object or the state pointer changes. False: not readable this time.
// Every read of a followed object comes here: the Draw3D sampler (proxies, parents, a rotation lane's own object),
// the one-shot reads at the commands, the chains, the crew table (CREW_LAG) and the render thread's paint read
// (RT_PAINT_READ). ATTACH_EDEN: in the Eden editor no copy is chosen - the simulation copy alone, and a
// visual lock taken before ends there (the first read outside the editor asks the visual copy again, and locks as
// above).
inline bool attach_raw(const game_value& gv, uint32_t& off, uintptr_t& vb,
                       uintptr_t& bb, uintptr_t& avb, uintptr_t& abb, float pos[3], float rot[9],
                       AttachDiag* diag = nullptr) {
    if (KH_ATTACH_VS_RENDER == 0u || KH_ATTACH_VS_SIM == 0u) return false;
    if (!attach_is_obj(gv)) return false;
    const game_data_object* gd = static_cast<const game_data_object*>(gv.data.get());
    // The holder chain get_unit_yaw_speed_sqf walks for its identity key.
    if (!gd || !gd->object || !gd->object->object) return false;
    const uintptr_t base = reinterpret_cast<uintptr_t>(gd->object->object);
    if (attach_eden()) {   // ATTACH_EDEN: the simulation copy alone, whatever an earlier read chose.
        if (off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
            off = KH_ATTACH_VS_SIM;
            vb = 0;
            bb = 0;
        }
        return attach_vs_read(base, KH_ATTACH_VS_SIM, vb, bb, pos, rot, diag);
    }
    if (off == KH_ATTACH_VS_RENDER) {   // Locked: the visual copy, and nothing else (outside the editor).
        return attach_vs_read(base, KH_ATTACH_VS_RENDER, vb, bb, pos, rot, diag);
    }
    if (attach_vs_read(base, KH_ATTACH_VS_RENDER, avb, abb, pos, rot, diag)) {   // It has one.
        if (pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f) {   // ATTACH_VS_STALE.
            float sp[3], sr[9];
            AttachDiag sd;
            if (off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
                off = KH_ATTACH_VS_SIM;
                vb = 0;
                bb = 0;
            }
            // Nothing to compare with: where the object is stays unknown, so this read is not readable (the
            // lanes hold their last pose) and nothing locks - an origin the simulation copy has not confirmed is
            // no proof the visual copy is real, and a lock is for good (outside the editor).
            if (!attach_vs_read(base, KH_ATTACH_VS_SIM, vb, bb, sp, sr, &sd)) return false;
            if (!(sp[0] == 0.0f && sp[1] == 0.0f && sp[2] == 0.0f)) {
                memcpy(pos, sp, sizeof(sp));
                memcpy(rot, sr, sizeof(sr));
                if (diag) *diag = sd;
                return true;
            }
        }
        off = KH_ATTACH_VS_RENDER;
        vb = avb;   // The page caches become the visual copy's.
        bb = abb;
        return true;
    }
    if (off != KH_ATTACH_VS_SIM) {   // The caches held another offset's pages.
        off = KH_ATTACH_VS_SIM;
        vb = 0;
        bb = 0;
    }
    return attach_vs_read(base, KH_ATTACH_VS_SIM, vb, bb, pos, rot, diag);
}

// The one-shot form, for the command sites that have no lane to cache in: addRender3D's position and rotation slots
// (the object named, or the parent a pair form binds to), and updateRender3D's properties - the attach itself
// (attach_apply), the parent of a new memory-point or skeletal binding (sqf_integration.hpp) and the re-seed of a
// mesh already attached (attach_reseed).
inline bool attach_read(const game_value& gv, float pos[3], float rot[9]) {
    attach_eden_note();   // ATTACH_EDEN: the commands' reads (game thread) take this call's flag.
    uint32_t  off = KH_ATTACH_OFF_NONE;
    uintptr_t vb = 0;
    uintptr_t bb = 0;
    uintptr_t avb = 0, abb = 0;
    return attach_raw(gv, off, vb, bb, avb, abb, pos, rot);
}

// rotated is the skip flag the bounds and the vertex path read, so it follows the matrix actually stored - a basis is
// not identity just because no euler was typed.
inline bool attach_rotated(const float m[9]) {
    static const float id[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    return memcmp(m, id, sizeof(id)) != 0;
}

// ATTACH_OFFSET - the composition, and the ONLY place it happens (with the position's double twin below,
// attach_offset_pos_d - POS_RES), so the axis convention and the multiplication order are each stated once.
// FIVE call sites each, across four functions: attach_apply here, attach_step (rendering/attachment_steps.hpp),
// attach_reseed below, and the bone seed and the skeletal seed inside kh_apply_render3d_prop in
// sqf_integration.hpp - attach_step's position through the double twin. Each has just taken a raw transform and
// is about to write it into a mesh.
// basis is the FOLLOWED object's rotation rows exactly as attach_vs_read hands them back - aside / up / dir,
// engine axes - the frame the offset is expressed in. pos is that object's position in SQF order [x, y, zASL],
// advanced in place. Neither reads the mesh's own rot_m: an offset is relative to the ATTACH POINT, not to whatever
// the mesh is currently rotated to.
inline void attach_offset_pos(const RenderObject& o, const float basis[9],
                              float pos[3]) {
    if (!o.attach_pos_on) return;
    // SQF [x, y, z] -> the object's engine-ordered local axes [side, up, forward]: the p = (0, 2, 1) permutation
    // set_rotation and objrec_fill use, its own inverse. Row 0 (aside) is the object's RIGHT, row 1 its up,
    // row 2 its forward.
    const float l[3] = { o.attach_pos[0], o.attach_pos[2], o.attach_pos[1] };
    float d[3] = { 0.0f, 0.0f, 0.0f };   // Engine axes (east, up, north).
    for (int k = 0; k < 3; ++k) {
        d[0] += l[k] * basis[k * 3 + 0];
        d[1] += l[k] * basis[k * 3 + 1];
        d[2] += l[k] * basis[k * 3 + 2];
    }
    pos[0] += d[0];   // east.
    pos[1] += d[2];   // north = SQF y.
    pos[2] += d[1];   // up = SQF zASL.
}

// POS_RES: the same offset onto a double position (the step's), so it adds no world-scale rounding.
inline void attach_offset_pos_d(const RenderObject& o, const float basis[9], double pos[3]) {
    if (!o.attach_pos_on) return;
    const double l[3] = { o.attach_pos[0], o.attach_pos[2], o.attach_pos[1] };
    double d[3] = { 0.0, 0.0, 0.0 };   // Engine axes (east, up, north).
    for (int k = 0; k < 3; ++k) {
        d[0] += l[k] * static_cast<double>(basis[k * 3 + 0]);
        d[1] += l[k] * static_cast<double>(basis[k * 3 + 1]);
        d[2] += l[k] * static_cast<double>(basis[k * 3 + 2]);
    }
    pos[0] += d[0];   // east.
    pos[1] += d[2];   // north = SQF y.
    pos[2] += d[1];   // up = SQF zASL.
}

// ATTACH_OFFSET - the rotation twin, in place on the rows just read. Row-vector convention throughout (world =
// centre + local.x * R0 + local.y * R1 + local.z * R2), so a mesh-local vector reaches world as v * attach_rot * basis
// and the product is attach_rot * basis IN THAT ORDER. The other order would turn the mesh about world axes instead
// of the attach point's.
inline void attach_offset_rot(const RenderObject& o, float rot[9]) {
    if (!o.attach_rot_on) return;
    float m[9];
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            m[r * 3 + c] =
                o.attach_rot[r * 3 + 0] * rot[0 * 3 + c] +
                o.attach_rot[r * 3 + 1] * rot[1 * 3 + c] +
                o.attach_rot[r * 3 + 2] * rot[2 * 3 + c];
        }
    }
    memcpy(rot, m, sizeof(m));
}

// SKEL - where a skeletal mesh is DRAWN from, given its root. pos arrives as the parent's origin with
// attachPosition already composed (the root, SQF order) and leaves as the draw centre: the root plus skel_ctr - the
// centre of the box the drawn buffer is stored in, in the authored frame - turned by the mesh's own rotation. So the
// object's box (pos and size, which every cull and caster bound reads) is the box of the pose being drawn, and a mesh
// drawing its rest buffer (no skin, or before its first upload; skel_ctr is then the import's own box centre) sits at
// its authored place on the parent. Runs after the rotation is final. Game thread (attach_reseed, addRender3D and
// the skeletal seed); the step's twin, skel_centre_d, runs on either thread (attach_step runs on both).
inline void skel_centre(const RenderObject& o, float pos[3]) {
    const float* c = o.skel_ctr;   // Engine axes.
    float d[3] = { c[0], c[1], c[2] };
    if (o.rotated) {   // Row vectors: the offset reaches world as c * rot_m.
        for (int k = 0; k < 3; ++k) {
            d[k] = c[0] * o.rot_m[0 * 3 + k] + c[1] * o.rot_m[1 * 3 + k] +
                   c[2] * o.rot_m[2 * 3 + k];
        }
    }
    pos[0] += d[0];   // east.
    pos[1] += d[2];   // north = SQF y.
    pos[2] += d[1];   // up = SQF zASL.
}
// POS_RES: the same centre onto a double root (the step's).
inline void skel_centre_d(const RenderObject& o, double pos[3]) {
    const float* c = o.skel_ctr;   // Engine axes.
    double d[3] = { c[0], c[1], c[2] };
    if (o.rotated) {
        for (int k = 0; k < 3; ++k) {
            d[k] = static_cast<double>(c[0]) * o.rot_m[0 * 3 + k] +
                   static_cast<double>(c[1]) * o.rot_m[1 * 3 + k] +
                   static_cast<double>(c[2]) * o.rot_m[2 * 3 + k];
        }
    }
    pos[0] += d[0];
    pos[1] += d[2];
    pos[2] += d[1];
}

// SKEL - the box of the REST buffer: the import's own size and centre. For the sites where the rest buffer is what
// draws next - a new skeletal binding and a new mesh under one (the substitute's mesh test refuses the old skinned
// buffer the moment the mesh changes). GAME THREAD, on the staged copy a command commits. Staged lookup, as
// apply_native_size.
inline void skel_rest_box(RenderObject& o) {
    const MeshDef* sd = mesh_def_staged(o.mesh);
    const MeshDef& d = sd ? *sd : mesh_def(0);
    for (int k = 0; k < 3; ++k) {
        o.size[k] = d.native_size[k];
        o.skel_ctr[k] = d.native_ctr[k];
    }
}

// SKEL - a mesh's skeleton as skel_make's filter: the lower-cased bone names, each bone's parent, whether any
// vertex takes weight from it (geom), and the bones that must keep a proxy whatever their geometry (keep: the chain's
// start and end bones, SKEL_PROXY_CULL). Built by skel_bone_names, GAME THREAD (staged lookup).
struct SkelBones {
    std::vector<std::string> name;
    std::vector<int32_t>     parent;
    std::vector<uint8_t>     geom;
    std::vector<std::string> keep;
};

// ATTACH_BONE - hand this entry's proxy to the delete queue and clear both bone lanes. Under g_draw_list_mutex.
// Moves out of the entry, so calling it twice on the same entry is a no-op rather than a double delete. Copies a
// game_value and makes no engine call, which is what lets it run from the park; the delete is attach_proxy_reap's.
inline void attach_proxy_retire(Attach& a) {
    // SKEL: a skeletal binding's proxies, every one of them ours as well.
    for (const game_value& s : a.skel_proxy) {
        if (!s.is_nil()) g_attach_proxy_dead.push_back(s);
    }
    a.skel_proxy.clear();
    a.skel_mem.clear();
    a.skel = false;
    if (!a.proxy.is_nil()) g_attach_proxy_dead.push_back(a.proxy);
    g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()),
                                std::memory_order_relaxed);
    a.proxy = game_value();
    a.bone_parent = game_value();
}

// Is the rotation lane the BINDING's - following the single proxy, or the parent under a skeletal binding - rather
// than an object the script pointed it at through the "rotation" property? The lanes are independent by contract, so
// only a binding's lane is a binding's to re-point or clear. Under g_draw_list_mutex.
inline bool attach_rot_is_binding(const Attach& a) {
    if (a.obj_rot.is_nil()) return false;
    if (!a.proxy.is_nil() && a.obj_rot.data.get() == a.proxy.data.get()) return true;
    return a.skel && !a.bone_parent.is_nil() &&
           a.obj_rot.data.get() == a.bone_parent.data.get();
}
// SKEL_ROT_OWN - does the rotation lane follow the POSITION lane's object (the plain "rotation true" lane,
// ATTACH_PLAIN_ROT)? attach_bone_rot's plain-arm test, which applies with no binding only: the same value, or
// two values of one object (separately evaluated lanes). Under g_draw_list_mutex.
inline bool attach_rot_is_plain(const Attach& a) {
    if (a.skel || !a.proxy.is_nil()) return false;   // A binding's lanes are the binding rule's.
    if (a.obj_rot.is_nil() || a.obj_pos.is_nil()) return false;
    if (a.obj_rot.data.get() == a.obj_pos.data.get()) return true;
    const uintptr_t b = attach_base_of(a.obj_rot);
    return b != 0u && b == attach_base_of(a.obj_pos);
}
// Queue one helper reference for attach_proxy_reap (release, delete at the last). Under g_draw_list_mutex.
inline void attach_org_drop(game_value& h) {
    if (h.is_nil()) return;
    g_attach_proxy_dead.push_back(h);
    g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()), std::memory_order_relaxed);
    h = game_value();
}
// A lane's new helper (nil: none), replacing the one it had, stamped with from (attach_org_make). The same
// helper again (the pool's, for the same character) keeps its stamp. Under g_draw_list_mutex.
inline void attach_org_put(Attach& a, bool rot, game_value& h, const game_value& op_for,
                           uint64_t from) {
    game_value& slot = rot ? a.org_rot : a.org_pos;
    const bool same = !slot.is_nil() && !h.is_nil() && slot.data.get() == h.data.get();
    attach_org_drop(slot);
    slot = h;
    h = game_value();
    (rot ? a.org_rot_for : a.org_pos_for) = slot.is_nil() ? nullptr : op_for.data.get();
    if (!same) (rot ? a.org_rot_seq : a.org_pos_seq) = from;
}
// Retire every helper that no longer stands in for its lane: the position helper serves a plain binding's position
// object or a skeletal binding's parent (both the position lane's object; a memory-point binding places from its
// own helper and reads its parent only as a check); the rotation helper serves the rotation lane's object when that
// is not the position lane's (whose read already has it). Under g_draw_list_mutex; after every lane change.
inline void attach_org_sync(Attach& a) {
    const bool pos_ok = a.skel || a.bone_parent.is_nil();
    if (!a.org_pos.is_nil() &&
        (!pos_ok || a.obj_pos.is_nil() || a.obj_pos.data.get() != a.org_pos_for)) {
        attach_org_drop(a.org_pos);
        a.org_pos_for = nullptr;
    }
    if (!a.org_rot.is_nil() &&
        (a.obj_rot.is_nil() || a.obj_rot.data.get() != a.org_rot_for ||
         a.obj_rot.data.get() == a.obj_pos.data.get())) {
        attach_org_drop(a.org_rot);
        a.org_rot_for = nullptr;
    }
}
// Both helpers, when the entry ends. Under g_draw_list_mutex.
inline void attach_org_retire(Attach& a) {
    attach_org_drop(a.org_pos);
    attach_org_drop(a.org_rot);
    a.org_pos_for = nullptr;
    a.org_rot_for = nullptr;
}

// Set or clear one lane. A value that is not a live-typed OBJECT clears it; an entry with both lanes clear is erased,
// so g_attach_n counts exactly the meshes the step walks and the objects are released as soon as they stop being
// followed. PLAIN_ROT_CARRY: end_plain (a position lane's detach) also clears a plain "rotation true" lane
// (attach_rot_is_plain, on the entry as it stood), under the same lock - as a second call clearing the
// rotation lane would.
inline void attach_set(const std::string& h, const game_value& gv, bool rot,
                       bool end_plain = false) {
    game_value org;   // ATTACH_MAN_HELPER: made before the mutex (SQF calls); every path below files it.
    uint64_t   from = 0u;
    if (attach_is_obj(gv) && !attach_obj_dead(gv)) attach_org_make(gv, org, from);
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const bool on = attach_is_obj(gv);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) {
        if (!on) { attach_org_drop(org); return; }
        it = g_attach.emplace(h, Attach()).first;
    }
    const bool plain = end_plain && !rot && !attach_rot_is_binding(it->second) &&
                       attach_rot_is_plain(it->second);   // PLAIN_ROT_CARRY.
    // ATTACH_BONE: the proxy exists to BE the position lane's object, so pointing that lane anywhere else ends the
    // bone attachment. Guarded on identity rather than on the flag, because attach_bone_set installs the proxy
    // through its own path and must not be undone here. The rotation lane is deliberately not covered: a script
    // setting a plain rotation while the position still follows a bone keeps the attachment and stops reading its rows.
    if (!rot && !it->second.proxy.is_nil() &&
        gv.data.get() != it->second.proxy.data.get()) {
        // The rotation lane goes with it only when it followed the proxy: one the script pointed at an object of
        // its own is independent (the skeletal branch below, attach_bone_set's rule).
        const bool pb = attach_rot_is_binding(it->second);
        attach_proxy_retire(it->second);
        if (pb) it->second.obj_rot = game_value();
    }
    // SKEL: a skeletal binding's position lane follows the parent itself, so identity cannot tell the binding from
    // a plain attach to the same object - and the SQF side clears RenderObject::skel on every position that is not the
    // binding. Any position set through here therefore ends it, with the rotation lane when that was the binding's.
    if (!rot && it->second.skel) {
        const bool rb = attach_rot_is_binding(it->second);
        attach_proxy_retire(it->second);
        if (rb) it->second.obj_rot = game_value();
    }
    game_value& lane = rot ? it->second.obj_rot : it->second.obj_pos;
    lane = on ? gv : game_value();
    attach_org_put(it->second, rot, org, gv, from);   // ATTACH_MAN_HELPER (nil: none).
    attach_org_sync(it->second);
    if (plain) {   // PLAIN_ROT_CARRY: the rotation lane cleared, as attach_set(h, nil, true) clears it.
        game_value none;
        it->second.obj_rot = game_value();
        attach_org_put(it->second, true, none, none, 0u);
        attach_org_sync(it->second);
    }
    if (it->second.obj_pos.is_nil() && it->second.obj_rot.is_nil()) {
        // ATTACH_BONE: the erase drops the only reference to the entry's proxy, so retire first or the object leaks
        // into the world with nothing left that knows its name. The guard above already retires on every path that
        // can empty the position lane, so this is unreachable today; keep it: a leak here would be silent and
        // permanent, and a later edit adding a rotation-side clear would not think to look at this erase.
        attach_proxy_retire(it->second);
        attach_org_retire(it->second);   // ATTACH_MAN_HELPER.
        g_attach.erase(it);
    }
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// Under g_draw_list_mutex, at every site that erases a draw-list entry: the attachment dies with the mesh. Nothing else
// drops it - the step skips an orphan and never erases (g_attach's THREADS note) - so a site that left this out would
// hold the object (and any proxy, standing on its parent) until the next attach_drop_all.
inline void attach_drop(const std::string& h) {
    if (g_attach.empty()) return;
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return;
    attach_proxy_retire(it->second);   // ATTACH_BONE: ours to delete.
    attach_org_retire(it->second);     // ATTACH_MAN_HELPER: likewise.
    g_attach.erase(it);
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// retire distinguishes the two callers. clear_render_objects is a SCRIPT clearing its meshes mid-mission: its
// proxies are live game objects that nothing but us will delete, so they must be queued. reset_retained_state is a
// MISSION EDGE: the engine is about to destroy every object in the world, the drain will not run again, and queueing
// there would hold SQF references to process exit (g_attach's rule).
inline void attach_drop_all(bool retire) {   // Under g_draw_list_mutex.
    if (retire) {
        for (auto& kv : g_attach) {
            attach_proxy_retire(kv.second);
            attach_org_retire(kv.second);   // ATTACH_MAN_HELPER.
        }
    } else {
        g_attach_proxy_dead.clear();
        g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> l(g_pxy_pool_mu);   // PXY_SMOOTH.
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

// ATTACH_DEAD, the queue side. Under g_draw_list_mutex, from EITHER thread - which is why it only ever appends a
// string. Deduplicated because the step runs several times a frame: at most one entry per doomed mesh however long
// the reap takes to arrive.
inline void attach_dead_queue(const std::string& h) {
    for (const std::string& s : g_attach_dead) if (s == h) return;
    g_attach_dead.push_back(h);
    g_attach_dead_n.store(static_cast<uint32_t>(g_attach_dead.size()), std::memory_order_relaxed);
}

// ATTACH_DEAD, the drain: GAME THREAD ONLY, once per flush_frame (the Draw3D handler), and the only place a mesh
// dies of its object's death - the thread the SQF commands mutate g_attach from, so the game_values attach_drop
// releases are released where they were taken. The erase is the one remove_render_object performs, in the same
// order, so a reaped mesh leaves the scene exactly as one removed by removeRenderHandler does.
// ATTACH_DEAD_RECHECK: the queue holds a verdict a step took on the binding as it stood then; a script may since
// have detached the mesh or pointed it at a live object (updateRender3D between the object's deletion and this
// Draw3D). The reap re-takes the step's own test on the binding as it stands now, under the same lock the commands
// write it under, and a mesh whose binding no longer follows a dead object stays - the one direction the note above
// ATTACH_DEAD forbids getting wrong.
inline void attach_reap() {
    if (g_attach_dead_n.load(std::memory_order_relaxed) == 0) return;
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    for (const std::string& h : g_attach_dead) {
        const auto at = g_attach.find(h);
        if (at == g_attach.end()) continue;   // Detached since: nothing follows a dead object.
        const Attach& a = at->second;
        if (!attach_obj_dead(a.obj_pos) && !attach_obj_dead(a.obj_rot) &&
            !attach_obj_dead(a.bone_parent)) continue;   // Re-pointed since at live objects.
        auto it = g_draw_list.find(h);
        // The mesh went first (a script or a mission edge removed it); the attachment still has to go, or the object
        // is held to teardown.
        if (it == g_draw_list.end()) { attach_drop(h); continue; }
        scene_stage_remove(it->second);
        attach_drop(h);
        g_draw_list.erase(it);
    }
    g_attach_dead.clear();
    g_attach_dead_n.store(0, std::memory_order_relaxed);
}

// ATTACH_BONE, the delete side: GAME THREAD ONLY, once per flush_frame (the Draw3D handler), and the only place a
// proxy is destroyed. It sits BEFORE the park because sqf::delete_vehicle is an engine call. The queue is swapped out
// under the mutex and drained outside it: deleteVehicle can re-enter our own SQF commands (an event handler on the
// proxy's death calling removeRenderHandler), and those take g_draw_list_mutex, which is not recursive.
inline void attach_proxy_reap() {
    if (g_attach_proxy_dead_n.load(std::memory_order_relaxed) == 0) return;
    std::vector<game_value> take;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        take.swap(g_attach_proxy_dead);
        g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
    }
    for (game_value& p : take) {
        if (p.is_nil()) continue;
        // A proxy whose parent was deleted may already be gone; deleting a null object is a no-op in SQF. The try is
        // the renderer's rule for every engine call made from a frame path.
        if (!pxy_pool_release(p)) continue;   // Another binding still follows it.
        try { sqf::delete_vehicle(static_cast<object>(p)); } catch (...) {}
    }
}

// ATTACH_BONE - queue a proxy that never reached a Attach entry. In every command that attaches to a memory
// point there is a window - attach_bone_make has created the object and attachTo has bound it to the parent, but
// attach_bone_set has not yet put it in a lane - in which NOTHING names it (not g_attach, not the draw list, not
// the dead queue), so a command that returns there leaves a ATTACH_BONE_SHAPE standing on the parent's memory
// point for the rest of the mission, invisible to every removal path (attach_drop, attach_drop_all).
// GAME THREAD, holding NO lock: from a command entry point (ProxyOwn, SkelOwn; kh_rv_chain_sim directly) and from
// flush_frame's crew_sync (a CREW_LAG entry's helper reference) and chain_prepare (a removed chain's proxy).
// It takes g_draw_list_mutex only to append; the delete still happens in attach_proxy_reap, outside every lock
// (g_attach_proxy_dead's note). Do not delete here: that would be safe at today's call sites and would stop being safe
// the first time one of them moves.
inline void attach_proxy_orphan(game_value& p) {
    if (p.is_nil()) return;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        g_attach_proxy_dead.push_back(p);
        g_attach_proxy_dead_n.store(static_cast<uint32_t>(g_attach_proxy_dead.size()),
                                    std::memory_order_relaxed);
    }
    p = game_value();
}

// The owner of that window, so the guarantee is STRUCTURAL rather than one exit at a time: addRender3D has nine
// returns after the make and before the set, plus two catch blocks. With a live proxy only the mesh parse's return
// is reachable: the make's own failure return holds no proxy, and the other seven are unreachable only because of
// the shape of an if/else chain (and the khr_skel test), which a later edit can change without noticing. Every path
// out of the command runs this destructor instead. Declare it only where g_draw_list_mutex is NOT held: the
// destructor takes that mutex and it is not recursive. All three declaration sites hold nothing: addRender3D, and
// kh_apply_render3d_prop and kh_rv_chain_sim, which updateRender3D reaches only after releasing the lock.
struct ProxyOwn {
    game_value proxy;
    ProxyOwn() {}
    ProxyOwn(const ProxyOwn&) = delete;
    ProxyOwn& operator=(const ProxyOwn&) = delete;
    ~ProxyOwn() { attach_proxy_orphan(proxy); }
    // Hands ownership on, so an installed proxy is never double-queued.
    game_value release() {
        game_value p = proxy;
        proxy = game_value();
        return p;
    }
};

// PXY_POOL - the proxy for (parent, memory point): the pool's live one, or a new one made and attached. GAME
// THREAD ONLY. False with a caller-facing sentence in err; the caller owns the reference (it ends in
// g_attach_proxy_dead).
inline bool pxy_pool_get(const game_value& parent, const std::string& mem, game_value& out,
                         const char* article, std::string& err) {
    const uintptr_t pb = attach_base_of(parent);
    std::string low = mem;
    low = kh_lower_copy(std::move(low));
    const std::string key = std::to_string(static_cast<unsigned long long>(pb)) + "|" + low;
    const auto kit = g_pxy_pool_key.find(key);
    if (kit != g_pxy_pool_key.end()) {
        const auto eit = g_pxy_pool.find(kit->second);
        if (eit != g_pxy_pool.end() && pb != 0 && attach_base_of(eit->second.parent) == pb &&
            !attach_obj_dead(eit->second.parent) && !attach_obj_dead(eit->second.proxy)) {
            ++eit->second.refs;
            out = eit->second.proxy;
            return true;
        }
        g_pxy_pool_key.erase(kit);   // Not shareable; its entry drains through the reap.
    }
    const object s = sqf::create_vehicle_local(ATTACH_BONE_SHAPE, vector3(0.0f, 0.0f, 0.0f));
    sqf::enable_simulation(s, false);
    sqf::set_physics_collision_flag(s, false);
    // PXY_HIDE - the helper exists for the transform the engine maintains for it (read from its visual state, not
    // from any draw). Hiding it drops its submission and every constant-buffer upload of its matrix. MEASURED: 420
    // game-thread samples whose parent moved, NONE with a helper that did not move with it - the engine refreshes a
    // hidden attached child exactly as it does a drawn one.
    sqf::hide_object(s, true);
    if (sqf::is_null(s)) {
        err = std::string("could not create ") + article + " memory-point proxy (" +
              ATTACH_BONE_SHAPE + " missing from the loaded mods?)";
        return false;
    }
    // The spawn position is discarded by the attach on the next simulation step.
    sqf::attach_to(s, static_cast<object>(parent), vector3(0.0f, 0.0f, 0.0f), mem, !mem.empty());
    PxyPoolEnt e;
    e.proxy = s;
    e.parent = parent;
    e.refs = 1u;
    e.made_seq = g_gts_seq;
    e.bone = !mem.empty();   // PXY_SMOOTH: an origin helper is not estimated.
    e.key = key;
    out = e.proxy;
    const uintptr_t xb = attach_base_of(e.proxy);
    if (xb != 0 && pb != 0) {
        // A filed entry here is a deleted, unreaped proxy whose address was reused: end it now, or its key could hand
        // this proxy out for its own memory point.
        const auto old = g_pxy_pool.find(xb);
        if (old != g_pxy_pool.end()) pxy_pool_erase(old);
        {
            std::lock_guard<std::mutex> l(g_pxy_pool_mu);   // PXY_SMOOTH.
            g_pxy_pool[xb] = e;
        }
        g_pxy_pool_key[key] = xb;
        g_pxy_pool_n.store(static_cast<uint32_t>(g_pxy_pool.size()), std::memory_order_relaxed);
    }
    return true;
}

// ATTACH_MAN_HELPER (declared above attach_set). The helper is attached at the character's origin with no
// memory point - where its visual state is placed, and what a plain lane reads - and pooled under that empty point.
inline void attach_org_make(const game_value& gv, game_value& out, uint64_t& from) {
    out = game_value();
    from = 0u;
    try {
        if (!sqf::is_kind_of(static_cast<object>(gv), "Man")) return;
        std::string err;
        if (!pxy_pool_get(gv, std::string(), out, "a character's", err)) out = game_value();
    } catch (...) {
        out = game_value();
    }
    if (out.is_nil()) return;
    const auto it = g_pxy_pool.find(attach_base_of(out));
    if (it != g_pxy_pool.end()) from = it->second.made_seq + 1u;
}

// NULL_SILENT - an attach target that is null (or whose transform cannot be read) is no fault the script hears
// of: the command still fails - nothing created, the property not applied - and reports nothing. A function below
// that finds one puts this marker in err instead of a sentence (attach_bone_make, skel_make,
// attach_apply); sqf_integration.hpp sets it at its own such sites, and every report site there says nothing
// for it (err_silent).
static constexpr const char* ERR_SILENT = "KH_ERR_SILENT";
inline bool err_silent(const std::string& e) { return e == ERR_SILENT; }

// ATTACH_BONE - the memory-point proxy. GAME THREAD ONLY (SQF calls). False with a caller-facing sentence in err
// (ERR_SILENT for a null parent). follow_bone is always true: the proxy tracks the full transform, and the
// script's rotation flag only selects whether the rotation lane reads it, so toggling rotation never re-attaches.
inline bool attach_bone_make(const game_value& parent, const std::string& mem,
                             game_value& proxy, std::string& err) {
    if (!attach_is_obj(parent)) {
        err = "position [object, memoryPoint] needs a game object in the first slot";
        return false;
    }
    if (attach_obj_dead(parent)) {
        err = ERR_SILENT;   // NULL_SILENT.
        return false;
    }
    if (mem.empty()) {
        err = "position [object, memoryPoint] needs a non-empty memory point name";
        return false;
    }
    return pxy_pool_get(parent, mem, proxy, "the", err);
}

// SKEL - the owner of a skeletal binding's proxies between their creation and attach_skel_set (ProxyOwn's
// reason and rule: declare it only where g_draw_list_mutex is NOT held).
struct SkelOwn {
    std::vector<game_value> proxies;
    SkelOwn() {}
    SkelOwn(const SkelOwn&) = delete;
    SkelOwn& operator=(const SkelOwn&) = delete;
    ~SkelOwn() { for (game_value& p : proxies) attach_proxy_orphan(p); }
    std::vector<game_value> release() {
        std::vector<game_value> r;
        r.swap(proxies);
        return r;
    }
};

// SKEL_PROXY_CULL - which of the parent's memory points get a proxy. The memory points a bone of the mesh names
// are matched (case-insensitively, a repeated name once); of those, a proxy is made only for a bone the skin would
// DRIVE something through: skin_drive moves every bone by the nearest matched bone up its chain (itself included),
// so the matched bones that matter are exactly the nearest matched ancestor-or-self of each bone that carries vertex
// weight (geom) or sits in the keep set (a chain's start and end, which the chain follows through their memory
// points - or their nearest named ancestor's - whether or not they skin a vertex). Any other matched bone drives
// nothing a vertex reads, and its proxy would be a game object made, attached and read every frame for no pixel.
// Bit-identical for the drawn pose, the cloth guide (read through vertex influences) and the chain's start and end:
// for a bone with geometry the nearest kept bone up its chain IS its nearest matched one, since the kept set is a
// subset of the matched set that contains it. Bones with no geometry and no keep entry may be driven from farther up
// the chain - nothing reads their map but the chain's reset placement of a body without geometry, which the
// simulation takes over on the next step. out is the memory points, lower-cased, in the parent's list order,
// with their names as the list spells them (raw, for attachTo). One SQF call (the selection list). GAME THREAD.
inline bool skel_plan(const object& p, const SkelBones& b,
                      std::vector<std::string>& out, std::vector<std::string>& raw) {
    out.clear();
    raw.clear();
    const size_t nb = b.name.size();
    if (nb == 0u) return true;   // No skeleton: nothing to follow, nothing to spawn.
    const auto names = sqf::selection_names(p, sqf::rv_selection_lods::Memory);
    // The matched bones, and the list entry each matched through.
    std::vector<int32_t> at(nb, -1);
    std::vector<std::string> low(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
        std::string l = names[i];
        l = kh_lower_copy(std::move(l));
        low[i] = l;
        if (l.empty()) continue;
        for (size_t k = 0; k < nb; ++k) {
            if (at[k] < 0 && b.name[k] == l) at[k] = static_cast<int32_t>(i);
        }
    }
    // The bones that drive something: up each geometric bone's chain to its nearest matched bone (bounded by the bone
    // count, as skin_drive is).
    std::vector<uint8_t> need(nb, 0u);
    for (size_t k = 0; k < nb; ++k) {
        const bool keep = std::find(b.keep.begin(), b.keep.end(), b.name[k]) != b.keep.end();
        if (!(k < b.geom.size() && b.geom[k]) && !keep) continue;
        int32_t c = static_cast<int32_t>(k);
        for (size_t s = 0; s <= nb && c >= 0 && static_cast<size_t>(c) < nb; ++s) {
            if (at[static_cast<size_t>(c)] >= 0) { need[static_cast<size_t>(c)] = 1u; break; }
            c = c < static_cast<int32_t>(b.parent.size()) ? b.parent[static_cast<size_t>(c)] : -1;
        }
    }
    // In the parent's list order, the entry each needed bone matched through.
    for (size_t i = 0; i < names.size(); ++i) {
        bool any = false;
        for (size_t k = 0; k < nb && !any; ++k) {
            if (need[k] && at[k] == static_cast<int32_t>(i)) any = true;
        }
        if (!any) continue;
        out.push_back(low[i]);
        raw.push_back(names[i]);
    }
    return true;
}

// SKEL - build a skeletal binding's proxies: one per memory point of the parent's Memory LOD that skel_plan
// keeps (SKEL_PROXY_CULL), each made and attached exactly as attach_bone_make makes its one (follow-bone, so
// the proxy carries the bone's animated rotation). GAME THREAD ONLY: one SQF call for the list and six per kept
// memory point that needs a new proxy (pxy_pool_get; none where the pool shares a live one), so every caller is a
// command entry point holding no lock. False with a caller-facing sentence in err (ERR_SILENT for a null parent);
// every proxy made before a failure is already in proxies, where the caller's SkelOwn queues it for deletion.
inline bool skel_make(const game_value& parent, const SkelBones& bones,
                      std::vector<game_value>& proxies, std::vector<std::string>& mems,
                      std::string& err) {
    if (!attach_is_obj(parent)) {
        err = "position [object, true] needs a game object in the first slot";
        return false;
    }
    if (attach_obj_dead(parent)) {
        err = ERR_SILENT;   // NULL_SILENT.
        return false;
    }
    const object p = static_cast<object>(parent);
    std::vector<std::string> plan, raw;
    skel_plan(p, bones, plan, raw);
    for (size_t i = 0; i < plan.size(); ++i) {
        game_value x;
        if (!pxy_pool_get(parent, raw[i], x, "a", err)) return false;
        proxies.push_back(x);   // Owned from here, whatever happens next.
        mems.push_back(plan[i]);
    }
    return true;
}

// ATTACH_BONE - install a finished proxy on a handle, both lanes at once: they name the same proxy, and setting
// them one at a time through attach_set would retire the proxy between the two calls.
inline void attach_bone_set(const std::string& h, const game_value& proxy,
                            const game_value& parent, bool rot) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) it = g_attach.emplace(h, Attach()).first;
    // A rotation lane following something OTHER than the OLD proxy is the script's own (the "rotation" property), and
    // the lanes are independent by contract - so a position change hands it back untouched. Only a lane that was
    // following the old proxy is the bone's to re-point.
    const bool was_bone = attach_rot_is_binding(it->second);   // SKEL: either kind.
    const game_value keep = was_bone ? game_value() : it->second.obj_rot;
    // A re-pointed bone attachment's old proxy stops being anything's lane here and is ours to delete.
    attach_proxy_retire(it->second);
    it->second.proxy = proxy;
    it->second.bone_parent = parent;
    it->second.obj_pos = proxy;
    const game_value new_rot = rot ? proxy : keep;
    it->second.obj_rot = new_rot;
    attach_org_sync(it->second);   // ATTACH_MAN_HELPER.
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// SKEL - install a skeletal binding on a handle: its proxies, the parent in the position lane, and the parent in
// the rotation lane when rot (the root follows the parent's rotation). Replaces any binding the handle had,
// single or skeletal, retiring its proxies; a rotation lane the script pointed at its own object is handed back
// untouched when rot is false, as attach_bone_set does.
inline void attach_skel_set(const std::string& h, std::vector<game_value>&& proxies,
                            const std::vector<std::string>& mems,
                            const game_value& parent, bool rot) {
    game_value org;   // ATTACH_MAN_HELPER: the parent's, made before the mutex (SQF calls); filed below.
    uint64_t   from = 0u;
    if (attach_is_obj(parent) && !attach_obj_dead(parent)) attach_org_make(parent, org, from);
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) it = g_attach.emplace(h, Attach()).first;
    Attach& a = it->second;
    const game_value keep = attach_rot_is_binding(a) ? game_value() : a.obj_rot;
    attach_proxy_retire(a);
    a.skel = true;
    a.skel_proxy = std::move(proxies);
    a.skel_mem = mems;
    a.skel_gen = ++g_skel_gen_serial;
    a.bone_parent = parent;
    a.obj_pos = parent;
    a.obj_rot = rot ? parent : keep;
    attach_org_put(a, false, org, parent, from);   // ATTACH_MAN_HELPER (nil: none).
    attach_org_sync(a);
    // A stash taken this cycle was the old binding's: the first render-thread step with a sample of this binding that
    // reads its pose re-stashes it (SKEL_SAMPLE's rule), and a drawer's step re-skins it.
    a.skel_smp_cycle = ~0ull;
    a.skin_culled = false;   // SKIN_CULL: the old binding's; the next step decides for this one.
    g_attach_n.store(static_cast<uint32_t>(g_attach.size()), std::memory_order_relaxed);
}

// ATTACH_BONE - the rotation toggle. Points the rotation lane at the proxy the position lane already follows, or
// clears it, WITHOUT touching the attachment (why the proxy is attached follow-bone once and never again). With no
// binding it names the object the position lane follows (ATTACH_PLAIN_ROT below). False when the handle has no
// attachment, or no binding and a position lane that follows nothing - the boolean then names no object.
inline bool attach_bone_rot(const std::string& h, bool on) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return false;
    // SKEL: under a skeletal binding the boolean is whether the ROOT follows the parent's rotation; the bones
    // follow their proxies either way.
    const bool skel = it->second.skel;
    const bool bind = skel || !it->second.proxy.is_nil();
    // ATTACH_PLAIN_ROT - with no binding the boolean's subject is the object the POSITION lane follows: true turns
    // the mesh with it, false stops, as the two binding forms mean (a script would otherwise have to spell a second
    // copy of the object). Refused only where the position lane follows nothing.
    if (!bind && it->second.obj_pos.is_nil()) return false;
    if (!on) {
        // FALSE means "stop following the bone", and nothing more: a rotation lane the script pointed at some other
        // object is not this command's to clear (bone_set's independence). The plain lane reads by the same rule,
        // against the position's object.
        const bool mine = bind
            ? attach_rot_is_binding(it->second)
            : (!it->second.obj_rot.is_nil() &&
               (it->second.obj_rot.data.get() == it->second.obj_pos.data.get() ||
                // ATTACH_PLAIN_ROT: two values of one object are one object (separately evaluated lanes).
                (attach_base_of(it->second.obj_rot) != 0u &&
                 attach_base_of(it->second.obj_rot) == attach_base_of(it->second.obj_pos))));
        if (!mine) return true;
        it->second.obj_rot = game_value();
    } else if (bind) {
        it->second.obj_rot = skel ? it->second.bone_parent : it->second.proxy;
    } else {
        it->second.obj_rot = it->second.obj_pos;   // ATTACH_PLAIN_ROT.
    }
    attach_org_sync(it->second);   // ATTACH_MAN_HELPER.
    return true;
}

// ATTACH_BONE - does this handle's rotation lane currently follow its bone? Read by the "position" property so
// that re-pointing an existing bone attachment at a new object or memory point PRESERVES the follow-rotation state.
// False for a handle with no bone attachment (also the right answer for a fresh one). The test is IDENTITY against
// the proxy, not a non-nil lane: a script may point the rotation lane at a different object while the position
// follows a bone, and answering true for that lane would let the position property overwrite it.
inline bool attach_bone_rot_state(const std::string& h) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return false;
    return attach_rot_is_binding(it->second);   // SKEL: either kind of binding.
}

// SKEL - does this handle carry a binding of either kind? Read by the "position" property: a new skeletal binding
// starts with its root following the parent's rotation, unless it replaces a binding, whose state it keeps, or the
// rotation lane follows an object of the script's own (attach_rot_own), which it keeps.
inline bool attach_has_binding(const std::string& h) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return false;
    return it->second.skel || !it->second.proxy.is_nil();
}

// SKEL_ROT_OWN - does this handle's rotation lane follow an object of the script's own (set through "rotation"),
// rather than a binding's or the position lane's (a plain "rotation true")? A new position never changes such a lane,
// so a new skeletal binding leaves it in place (attach_skel_set keeps it when told not to follow the parent), as
// re-pointing a bone does.
inline bool attach_rot_own(const std::string& h) {
    if (g_attach_n.load(std::memory_order_relaxed) == 0u) return false;   // Game thread: no entry, no lane.
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return false;
    const Attach& a = it->second;
    // A plain "rotation true" lane (ATTACH_PLAIN_ROT) follows the POSITION's object: the position's, not the
    // script's own - a new binding takes it over (attach_rot_is_plain).
    return !a.obj_rot.is_nil() && !attach_rot_is_binding(a) && !attach_rot_is_plain(a);
}
// SKEL_ROT_OWN - is this handle's rotation lane the plain "rotation true" one (it follows the position lane's
// object)? A new single-bone binding carries that true over - the mesh turns with the new proxy (the bone), as a
// skeletal binding's root does by default - instead of keeping the lane as an object of the script's own, which
// "rotation false" under the binding could then never clear. PLAIN_ROT_CARRY: a new object position carries
// it too (the lane follows the new object).
inline bool attach_rot_plain(const std::string& h) {
    if (g_attach_n.load(std::memory_order_relaxed) == 0u) return false;   // Game thread: no entry, no lane.
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    return it != g_attach.end() && !attach_rot_is_binding(it->second) &&
           attach_rot_is_plain(it->second);
}

// Attach a lane AND take the transform now, so the mesh is in place on the frame the command runs. False when the
// object cannot be read (null, or not readable now), with ERR_SILENT in err: the property fails without a report
// (NULL_SILENT), and nothing is attached.
inline bool attach_apply(const std::string& h, const game_value& gv,
                         bool rot, RenderObject& o, std::string& err) {
    float p[3], r[9];
    if (!attach_read(gv, p, r)) {
        err = ERR_SILENT;   // NULL_SILENT.
        return false;
    }
    if (rot) {
        attach_offset_rot(o, r);   // ATTACH_OFFSET: inside the frame just read.
        memcpy(o.rot_m, r, sizeof(o.rot_m));
        o.rotated = attach_rotated(r);
    } else {
        attach_offset_pos(o, r, p);   // ATTACH_OFFSET.
        o.pos[0] = p[0];
        o.pos[1] = p[1];
        o.pos[2] = p[2];
    }
    attach_set(h, gv, rot);
    return true;
}

// ATTACH_OFFSET - this handle's two lane objects, copied out. GAME THREAD ONLY: copying a game_value touches the
// SQF allocator (the rule that keeps attach_step to strings). A handle with no attachment leaves both out-params
// nil, which makes the re-seed below a no-op for it. The copy is taken under the mutex and USED outside it, so the raw
// reads and their page walks never run with g_draw_list_mutex held from a command.
// ATTACH_SEED_PARENT - placed (the re-seed's reads): a memory-point proxy not yet past the sample that first
// sees it (attach_org_read's rule, made_seq + 1) still stands at its spawn point, so a lane naming it is handed the
// parent instead, as the addRender3D bone seed reads.
inline void attach_lanes(const std::string& h, game_value& pos, game_value& rot,
                         bool placed = false) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_attach.find(h);
    if (it == g_attach.end()) return;
    pos = it->second.obj_pos;
    rot = it->second.obj_rot;
    const Attach& a = it->second;
    if (placed && !a.skel && !a.proxy.is_nil() && !a.bone_parent.is_nil()) {
        const auto pe = g_pxy_pool.find(attach_base_of(a.proxy));   // The game thread's own read.
        if (pe != g_pxy_pool.end() && g_gts_seq <= pe->second.made_seq + 1u) {
            const void* const x = a.proxy.data.get();
            if (pos.data.get() == x) pos = a.bone_parent;
            if (!rot.is_nil() && rot.data.get() == x) rot = a.bone_parent;
        }
    }
}

// ATTACH_OFFSET - re-take this mesh's transform from whatever it already follows, so a changed offset lands on the
// frame the command runs. GAME THREAD ONLY (attach_lanes), takes no lock itself past that copy, and makes NO SQF
// call - the reads are the step's raw, page-gated ones, with fresh caches (a command has no lane to cache in). A lane
// that is not attached, or whose read fails this instant, is left as it is: the step corrects it on its next pass,
// and the mesh never jumps somewhere neither value says.
inline void attach_reseed(const std::string& h, RenderObject& o) {
    game_value lp, lr;
    attach_lanes(h, lp, lr, true);   // ATTACH_SEED_PARENT.
    float p[3], r[9];
    float root[3] = { 0.0f, 0.0f, 0.0f };
    bool  centre = false;   // SKEL: the draw centre waits for the rotation below.
    if (!lp.is_nil() && attach_read(lp, p, r)) {
        attach_offset_pos(o, r, p);
        if (o.skel) {
            memcpy(root, p, sizeof(root));
            centre = true;
        } else {
            o.pos[0] = p[0];
            o.pos[1] = p[1];
            o.pos[2] = p[2];
        }
    }
    if (!lr.is_nil() && attach_read(lr, p, r)) {
        attach_offset_rot(o, r);
        memcpy(o.rot_m, r, sizeof(o.rot_m));
        o.rotated = attach_rotated(r);   // The COMPOSED matrix, not the raw one.
    }
    if (centre) {
        skel_centre(o, root);
        o.pos[0] = root[0];
        o.pos[1] = root[1];
        o.pos[2] = root[2];
    }
}

// ATTACH_DIAG, one object read's outcome. Under g_draw_list_mutex (the Draw3D sampler).
inline void attach_diag_note(bool ok, const AttachDiag& d) {
    if (!stats_on()) return;
    if (!ok) g_attach_refused.fetch_add(1u, std::memory_order_relaxed);
    else if (d.repaired) g_attach_repaired.fetch_add(1u, std::memory_order_relaxed);
    float dn_max = 0.0f;
    const uint32_t bits = g_attach_skew_bits.load(std::memory_order_relaxed);
    memcpy(&dn_max, &bits, sizeof(dn_max));
    if (d.skew > dn_max) {
        uint32_t nb = 0;
        memcpy(&nb, &d.skew, sizeof(nb));
        g_attach_skew_bits.store(nb, std::memory_order_relaxed);
    }
}
}