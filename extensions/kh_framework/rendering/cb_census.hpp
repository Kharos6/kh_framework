#pragma once

// The injection half (rendering_integration_injection.hpp): the constant-buffer census (KH_CBC). Never include it on
// its own.
namespace RenderIntegration {

inline bool shadow_live_wanted();   // Defined in rendering/mask_cast_state.hpp (the view-stage gate).
// Keyed by the buffer object pointer, compared opaquely (never dereferenced or AddRef'd: a destroyed buffer stops
// matching; a recycled pointer re-records only when an upload to it probes as a camera slot, and until then resolves to
// the old pair - kh_cbc_find checks no age).
struct KhCbCensusEntry {
    void*    buf = nullptr;
    float    m22 = 0.0f;
    float    m32 = 0.0f;
    uint64_t ms = 0;   // Steady ms of the recording upload.
};
static constexpr int KH_CBC_N = 16;
static KhCbCensusEntry g_cbc[KH_CBC_N];

// DO NOT TRY TO NARROW THIS. It was measured, in full, and it cannot be.
//
// kh_cbc_on() is term 1 of kh_upload_hook_wanted's consumer list, and it is a blanket: with a lit mesh in the scene it
// admits EVERY engine constant-buffer map - ~8,800 a cycle, ~3 ms of render-thread time. Narrowing it looks like an
// obvious win (the census it feeds, g_cbc, holds 16 slots and the other locators all narrow to one buffer once they
// lock).
// Measured per term, per consumer and per map, two passes 2,891 cycles apart on a lit-mesh scene:
//
//   the census IS stable      0 new slots in 5,579,840 notes; 2 slots live;
//                             ~849 of the ~8,800 admissions a cycle reach it.
//   shadow_register_upload    16 / 16 slots FULL, taking 22.1 NEW slots a
//   is NOT                    cycle - it recycles its whole table ~1.4x every
//                             frame. It has no working set; the stream is its
//                             working set, which is what its own 'no size cap:
//                             big CBs hold the maps' is telling you.
//
// The candidate tried was the union of what term 1's consumers actually hold - census + cb_reg + view_src + the two
// colour probes - with discovery left open while any table is unpopulated. As a shadow predicate against live traffic
// it refused 21 % of admissions and would have COST 16.2 cb_reg slots a cycle, 74 % of every new one. The prize was
// ~0.6 ms; the price was three quarters of the shadow register's intake.
//
// A starved consumer here does not fall back and does not flicker: it holds its last value. Shadow view recovery would
// degrade silently ('shadows sometimes look wrong'), with nothing pointing back here. The same shape already bit the
// two colour probes in locator_note_upload when a different blanket term was closed: they had no term of their own and
// were riding that one.
//
// The only consumer that can be locked is the census, at 10 % of the traffic. Anything else needs
// shadow_register_upload to stop being a rolling recorder, which is a change to what it is FOR, not to this gate.
inline bool kh_cbc_on() {
    return shadow_live_wanted() || g_svs_mesh_wanted.load(std::memory_order_relaxed);
}

inline const KhCbCensusEntry* kh_cbc_find(void* khcb_buf) {
    for (int khcb_i = 0; khcb_i < KH_CBC_N; ++khcb_i) {
        if (g_cbc[khcb_i].buf == khcb_buf) return &g_cbc[khcb_i];
    }
    return nullptr;
}

inline void kh_cbc_note(void* khcb_res, float khcb_m22, float khcb_m32) {
    int khcb_slot = -1;
    uint64_t khcb_oldest = ~0ull;
    int khcb_oldest_i = 0;

    for (int khcb_i = 0; khcb_i < KH_CBC_N; ++khcb_i) {
        if (g_cbc[khcb_i].buf == khcb_res) { khcb_slot = khcb_i; break; }
        if (!g_cbc[khcb_i].buf) { if (khcb_slot < 0) khcb_slot = khcb_i; continue; }
        if (g_cbc[khcb_i].ms < khcb_oldest) {
            khcb_oldest = g_cbc[khcb_i].ms;
            khcb_oldest_i = khcb_i;
        }
    }

    if (khcb_slot < 0) {
        khcb_slot = khcb_oldest_i;
        g_cbc[khcb_slot] = KhCbCensusEntry{};
    }

    g_cbc[khcb_slot].buf = khcb_res;
    g_cbc[khcb_slot].m22 = khcb_m22;
    g_cbc[khcb_slot].m32 = khcb_m32;
    g_cbc[khcb_slot].ms = steady_now_ms();
}
}