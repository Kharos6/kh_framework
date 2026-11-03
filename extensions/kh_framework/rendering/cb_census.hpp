#pragma once

// The injection half (rendering_integration_injection.hpp): the constant-buffer census (CBC). Never include it on
// its own.
namespace RenderIntegration {

inline bool shadow_live_wanted();   // Defined in rendering/mask_cast_state.hpp (the view-stage gate).
// Keyed by the buffer object pointer, compared opaquely (never dereferenced or AddRef'd: a destroyed buffer stops
// matching; a recycled pointer re-records only when an upload to it probes as a camera slot, and until then resolves to
// the old pair - cbc_find checks no age).
struct CbCensusEntry {
    void*    buf = nullptr;
    float    m22 = 0.0f;
    float    m32 = 0.0f;
    uint64_t ms = 0;   // Steady ms of the recording upload.
};
static constexpr int CBC_N = 16;
static CbCensusEntry g_cbc[CBC_N];

// DO NOT TRY TO NARROW THIS. It was measured, in full, and it cannot be.
//
// cbc_on() is term 1 of upload_hook_wanted's consumer list, and it is a blanket: with a lit mesh in the scene it
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
inline bool cbc_on() {
    return shadow_live_wanted() || g_svs_mesh_wanted.load(std::memory_order_relaxed);
}

inline const CbCensusEntry* cbc_find(void* buf) {
    for (int i = 0; i < CBC_N; ++i) {
        if (g_cbc[i].buf == buf) return &g_cbc[i];
    }
    return nullptr;
}

inline void cbc_note(void* res, float m22, float m32) {
    int slot = -1;
    uint64_t oldest = ~0ull;
    int oldest_i = 0;

    for (int i = 0; i < CBC_N; ++i) {
        if (g_cbc[i].buf == res) { slot = i; break; }
        if (!g_cbc[i].buf) { if (slot < 0) slot = i; continue; }
        if (g_cbc[i].ms < oldest) {
            oldest = g_cbc[i].ms;
            oldest_i = i;
        }
    }

    if (slot < 0) {
        slot = oldest_i;
        g_cbc[slot] = CbCensusEntry{};
    }

    g_cbc[slot].buf = res;
    g_cbc[slot].m22 = m22;
    g_cbc[slot].m32 = m32;
    g_cbc[slot].ms = steady_now_ms();
}
}