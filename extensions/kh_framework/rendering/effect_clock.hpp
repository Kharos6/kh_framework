#pragma once

// The feature half (rendering_integration_features.hpp): the effect clock (effect_time_seconds, the fast clock). Never
// include it on its own.
namespace RenderIntegration {

// Monotonic seconds for effect animation (grain, distortion, pulse). The session clock in double: float seconds
// since its origin lose a millisecond of resolution after a few hours, so the origin of anything a shader animates
// on (fx_t0) and the differences taken against it are formed here; the float form below serves the envelopes,
// whose granularity is harmless at that scale.
// KH_SESSION_CLOCK: the origin is taken at the first read and dropped by the clean session reset
// (kh_session_objects_reset), so every session's clock starts at zero as a fresh process's does. The deferred
// teardown keeps it: the live objects' birth_time / fx_t0 were stamped on it. steady_clock ticks; 0 = not taken yet.
// Real time: it runs on through the pause menu and ignores setAccTime. Every duration (a mesh's or a pass's, re-armed
// from the update that sets it; an affector's lifetime), the uv clock (KH_UV_ANIM) and the cloth / chain steps run on
// it - scripts time them with uiSleep, not sleep. Moving any of them to game time breaks that.
static std::atomic<int64_t> g_effect_t0{ 0 };
inline double effect_time_seconds_d() {
    int64_t khet_now = static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    int64_t khet_t0 = g_effect_t0.load(std::memory_order_relaxed);
    if (khet_t0 == 0) {
        int64_t khet_z = 0;
        khet_t0 = g_effect_t0.compare_exchange_strong(khet_z, khet_now, std::memory_order_relaxed) ? khet_now : khet_z;
    }
    // A thread that read the clock just before another took the origin reads zero, never a negative time.
    if (khet_now < khet_t0) khet_now = khet_t0;
    return std::chrono::duration<double>(std::chrono::steady_clock::duration(khet_now - khet_t0)).count();
}
inline float effect_time_seconds() {
    return static_cast<float>(effect_time_seconds_d());
}
// KH_FAST_CLOCK - effect_time_seconds_d() for the upload funnel: the locator takes the time on every captured upload
// (~38,000 a frame in a heavy interior), and steady_clock's QPC is the runtime's calls into ntdll around an RDTSCP. The
// TSC instead - one RDTSC - carried from an anchor (a steady_clock read paired with the TSC reads around it, both on
// one core: RDTSCP's TSC_AUX) by the TSC's rate, measured between anchors; a new anchor every KH_FAST_CLOCK_SPAN_S of
// TSC time, and whenever the TSC reads behind the anchor. Only where Windows itself counts QPC from the TSC (an
// invariant TSC, RDTSCP, and QPC at 10 MHz - the frequency it reports for a TSC source, whose cores it keeps in step);
// elsewhere, before the first rate, and on a session reset's new clock origin until it re-anchors, steady_clock
// itself. The value tracks a steady_clock read at the same instant within half the anchor pairing (the
// narrowest of up to three; typically tens of nanoseconds) plus the rate's error over one span - typically well
// under a microsecond - and never runs backwards within a clock origin: a decision against a threshold differs from
// the old one only when the threshold falls inside that error, as if the upload came that much earlier or later.
// Worst cases: three preempted pairings in a row (about the narrowest one's width, for up to two spans), a resume
// that restored the TSC across a sleep (the sleep is missing until the span runs out: at most KH_FAST_CLOCK_SPAN_S of
// TSC time), and a change of the TSC's own rate (a VM moved live to a host whose TSC runs at another rate; not on
// bare metal with an invariant TSC): up to the change's share of KH_FAST_CLOCK_SPAN_S until the span runs out. An
// anchor whose TSC and steady_clock intervals disagree by more than KH_FAST_CLOCK_AGREE drops the rate and measures
// it again. Render thread only (the upload funnel); plain state.
static constexpr double KH_FAST_CLOCK_SPAN_S = 0.25;    // A fresh anchor this often (TSC time).
static constexpr double KH_FAST_CLOCK_FIRST_S = 0.05;   // A rate: anchors at least this far apart.
static constexpr double KH_FAST_CLOCK_PAIR_S = 2.0e-6;  // A pairing at most this wide ends the retakes.
static constexpr double KH_FAST_CLOCK_AGREE = 0.01;     // A span whose two clocks disagree more: the rate is dropped.
struct KhFastClock {
    int8_t   usable = 0;       // 0 = not asked yet, 1 = the TSC is QPC's source, -1 = steady_clock only.
    int64_t  t0 = 0;           // g_effect_t0 at the anchor: a different origin (a session reset) re-anchors.
    double   base_s = 0.0;     // effect_time_seconds_d() at the anchor.
    uint64_t base_tsc = 0;     // The TSC at the anchor (the middle of its pairing); 0 = no anchor.
    double   tsc_per_s = 0.0;  // The TSC's rate; 0 = not measured (steady_clock is read).
    double   s_per_tsc = 0.0;  // Its inverse.
    double   last = 0.0;       // The last value returned (this origin's): the clock never runs backwards.
};
static KhFastClock g_fast_clock;
inline bool kh_tsc_is_qpc() {
    int khti_r[4] = { 0, 0, 0, 0 };
    __cpuid(khti_r, static_cast<int>(0x80000000u));
    if (static_cast<uint32_t>(khti_r[0]) < 0x80000007u) return false;
    __cpuid(khti_r, static_cast<int>(0x80000001u));
    if ((khti_r[3] & (1 << 27)) == 0) return false;   // EDX bit 27: RDTSCP.
    __cpuid(khti_r, static_cast<int>(0x80000007u));
    if ((khti_r[3] & (1 << 8)) == 0) return false;   // EDX bit 8: invariant TSC.
    LARGE_INTEGER khti_f = {};
    return QueryPerformanceFrequency(&khti_f) && khti_f.QuadPart == 10000000;   // QPC from the TSC.
}
inline double kh_fast_clock_out(double khfo_v) {
    KhFastClock& khfo_c = g_fast_clock;
    if (khfo_v < khfo_c.last) khfo_v = khfo_c.last;
    khfo_c.last = khfo_v;
    return khfo_v;
}
// steady_clock, held to the clock's last value while the origin is the one it was taken on.
inline double kh_fast_clock_steady() {
    const double khfs_v = effect_time_seconds_d();
    return g_effect_t0.load(std::memory_order_relaxed) == g_fast_clock.t0 ? kh_fast_clock_out(khfs_v) : khfs_v;
}
inline double kh_fast_effect_seconds_d() {
    KhFastClock& khfc_c = g_fast_clock;
    if (khfc_c.usable <= 0) {
        if (khfc_c.usable < 0) return effect_time_seconds_d();
        khfc_c.usable = kh_tsc_is_qpc() ? 1 : -1;
        if (khfc_c.usable < 0) return effect_time_seconds_d();
    }
    const uint64_t khfc_tsc = __rdtsc();
    const int64_t khfc_t0 = g_effect_t0.load(std::memory_order_relaxed);
    if (khfc_c.tsc_per_s > 0.0 && khfc_c.base_tsc != 0 && khfc_t0 == khfc_c.t0 && khfc_tsc >= khfc_c.base_tsc) {
        // The interval is far below 2^63 ticks: the signed conversion is one instruction.
        const double khfc_dt = static_cast<double>(static_cast<int64_t>(khfc_tsc - khfc_c.base_tsc)) * khfc_c.s_per_tsc;
        if (khfc_dt < KH_FAST_CLOCK_SPAN_S) return kh_fast_clock_out(khfc_c.base_s + khfc_dt);
    }
    if (khfc_c.tsc_per_s <= 0.0 && khfc_c.base_tsc != 0 && khfc_t0 == khfc_c.t0) {
        // No rate yet: steady_clock until the anchor is KH_FAST_CLOCK_FIRST_S old.
        const double khfc_v = effect_time_seconds_d();
        if (khfc_v - khfc_c.base_s < KH_FAST_CLOCK_FIRST_S &&
            g_effect_t0.load(std::memory_order_relaxed) == khfc_c.t0) return kh_fast_clock_out(khfc_v);
    }
    // An anchor: steady_clock between two TSC reads on one core; the narrowest of up to three pairs (a preemption
    // between the reads widens one, a move to another core spoils one), done at the first narrow one once the rate is
    // known.
    double khfc_s = 0.0;
    uint64_t khfc_mid = 0, khfc_w = ~0ull;
    for (int khfc_k = 0; khfc_k < 3; ++khfc_k) {
        unsigned int khfc_xa = 0, khfc_xb = 0;
        const uint64_t khfc_a = __rdtscp(&khfc_xa);
        const double khfc_v = effect_time_seconds_d();
        const uint64_t khfc_b = __rdtscp(&khfc_xb);
        if (khfc_xa != khfc_xb || khfc_b < khfc_a) continue;   // Another core, or backwards: no pairing.
        if (khfc_b - khfc_a < khfc_w) {
            khfc_w = khfc_b - khfc_a;
            khfc_s = khfc_v;
            khfc_mid = khfc_a + khfc_w / 2u;
        }
        if (khfc_c.tsc_per_s > 0.0 && static_cast<double>(khfc_w) <= khfc_c.tsc_per_s * KH_FAST_CLOCK_PAIR_S) break;
    }
    if (khfc_w == ~0ull) return kh_fast_clock_steady();   // No pairing: steady_clock, the anchor kept.
    const int64_t khfc_t1 = g_effect_t0.load(std::memory_order_relaxed);   // The origin the reads were made on.
    if (khfc_t1 != khfc_c.t0) {   // A new origin (the first use, a session reset): its values start over.
        khfc_c.t0 = khfc_t1;
        khfc_c.last = 0.0;
        khfc_c.base_tsc = 0;
    }
    if (khfc_c.base_tsc != 0 && khfc_mid > khfc_c.base_tsc) {
        const double khfc_ds = khfc_s - khfc_c.base_s;
        const double khfc_dn = static_cast<double>(khfc_mid - khfc_c.base_tsc);   // TSC ticks over the same interval.
        if (khfc_ds >= KH_FAST_CLOCK_FIRST_S) {
            const double khfc_rate = khfc_dn / khfc_ds;
            const bool khfc_part = khfc_c.tsc_per_s > 0.0 &&
                                   fabs(khfc_dn * khfc_c.s_per_tsc - khfc_ds) > KH_FAST_CLOCK_AGREE * khfc_ds;
            // The clocks parted over the span (a resume, a rate change): no rate - measured again from this anchor.
            const bool khfc_ok = !khfc_part && khfc_rate > 1.0e8 && khfc_rate < 2.0e10;
            khfc_c.tsc_per_s = khfc_ok ? khfc_rate : 0.0;
            khfc_c.s_per_tsc = khfc_ok ? 1.0 / khfc_rate : 0.0;
        }
    }
    khfc_c.base_s = khfc_s;
    khfc_c.base_tsc = khfc_mid;
    return kh_fast_clock_out(khfc_s);
}
// The shader's time lane (fxMeta.y): seconds since the object was created, formed in double and handed over as
// float, so an effect's animation keeps its resolution however long the process has been up and never wraps.
inline float kh_fx_time(const RenderObject& khft_o, double khft_now_d) {
    const double khft_t = khft_now_d - khft_o.fx_t0;
    return khft_t > 0.0 ? static_cast<float>(khft_t) : 0.0f;
}
}