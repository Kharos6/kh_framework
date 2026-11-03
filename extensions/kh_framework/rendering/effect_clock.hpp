#pragma once

// The feature half (rendering_integration_features.hpp): the effect clock (effect_time_seconds, the fast clock). Never
// include it on its own.
namespace RenderIntegration {

// Monotonic seconds for effect animation (grain, distortion, pulse). The session clock in double: float seconds
// since its origin lose a millisecond of resolution after a few hours, so the origin of anything a shader animates
// on (fx_t0) and the differences taken against it are formed here; the float form below serves the envelopes,
// whose granularity is harmless at that scale.
// SESSION_CLOCK: the origin is taken at the first read and dropped by the clean session reset
// (session_objects_reset), so every session's clock starts at zero as a fresh process's does. The deferred
// teardown keeps it: the live objects' birth_time / fx_t0 were stamped on it. steady_clock ticks; 0 = not taken yet.
// Real time: it runs on through the pause menu and ignores setAccTime. Every duration (a mesh's or a pass's, re-armed
// from the update that sets it; an affector's lifetime), the uv clock (UV_ANIM) and the cloth / chain steps run on
// it - scripts time them with uiSleep, not sleep. Moving any of them to game time breaks that.
static std::atomic<int64_t> g_effect_t0{ 0 };
inline double effect_time_seconds_d() {
    int64_t et_now = static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    int64_t t0 = g_effect_t0.load(std::memory_order_relaxed);
    if (t0 == 0) {
        int64_t z = 0;
        t0 = g_effect_t0.compare_exchange_strong(z, et_now, std::memory_order_relaxed) ? et_now : z;
    }
    // A thread that read the clock just before another took the origin reads zero, never a negative time.
    if (et_now < t0) et_now = t0;
    return std::chrono::duration<double>(std::chrono::steady_clock::duration(et_now - t0)).count();
}
inline float effect_time_seconds() {
    return static_cast<float>(effect_time_seconds_d());
}
// FAST_CLOCK - effect_time_seconds_d() for the upload funnel: the locator takes the time on every captured upload
// (~38,000 a frame in a heavy interior), and steady_clock's QPC is the runtime's calls into ntdll around an RDTSCP. The
// TSC instead - one RDTSC - carried from an anchor (a steady_clock read paired with the TSC reads around it, both on
// one core: RDTSCP's TSC_AUX) by the TSC's rate, measured between anchors; a new anchor every FAST_CLOCK_SPAN_S of
// TSC time, and whenever the TSC reads behind the anchor. Only where Windows itself counts QPC from the TSC (an
// invariant TSC, RDTSCP, and QPC at 10 MHz - the frequency it reports for a TSC source, whose cores it keeps in step);
// elsewhere, before the first rate, and on a session reset's new clock origin until it re-anchors, steady_clock
// itself. The value tracks a steady_clock read at the same instant within half the anchor pairing (the
// narrowest of up to three; typically tens of nanoseconds) plus the rate's error over one span - typically well
// under a microsecond - and never runs backwards within a clock origin: a decision against a threshold differs from
// the old one only when the threshold falls inside that error, as if the upload came that much earlier or later.
// Worst cases: three preempted pairings in a row (about the narrowest one's width, for up to two spans), a resume
// that restored the TSC across a sleep (the sleep is missing until the span runs out: at most FAST_CLOCK_SPAN_S of
// TSC time), and a change of the TSC's own rate (a VM moved live to a host whose TSC runs at another rate; not on
// bare metal with an invariant TSC): up to the change's share of FAST_CLOCK_SPAN_S until the span runs out. An
// anchor whose TSC and steady_clock intervals disagree by more than FAST_CLOCK_AGREE drops the rate and measures
// it again. Render thread only (the upload funnel); plain state.
static constexpr double FAST_CLOCK_SPAN_S = 0.25;    // A fresh anchor this often (TSC time).
static constexpr double FAST_CLOCK_FIRST_S = 0.05;   // A rate: anchors at least this far apart.
static constexpr double FAST_CLOCK_PAIR_S = 2.0e-6;  // A pairing at most this wide ends the retakes.
static constexpr double FAST_CLOCK_AGREE = 0.01;     // A span whose two clocks disagree more: the rate is dropped.
struct FastClock {
    int8_t   usable = 0;       // 0 = not asked yet, 1 = the TSC is QPC's source, -1 = steady_clock only.
    int64_t  t0 = 0;           // g_effect_t0 at the anchor: a different origin (a session reset) re-anchors.
    double   base_s = 0.0;     // effect_time_seconds_d() at the anchor.
    uint64_t base_tsc = 0;     // The TSC at the anchor (the middle of its pairing); 0 = no anchor.
    double   tsc_per_s = 0.0;  // The TSC's rate; 0 = not measured (steady_clock is read).
    double   s_per_tsc = 0.0;  // Its inverse.
    double   last = 0.0;       // The last value returned (this origin's): the clock never runs backwards.
};
static FastClock g_fast_clock;
inline bool tsc_is_qpc() {
    int r[4] = { 0, 0, 0, 0 };
    __cpuid(r, static_cast<int>(0x80000000u));
    if (static_cast<uint32_t>(r[0]) < 0x80000007u) return false;
    __cpuid(r, static_cast<int>(0x80000001u));
    if ((r[3] & (1 << 27)) == 0) return false;   // EDX bit 27: RDTSCP.
    __cpuid(r, static_cast<int>(0x80000007u));
    if ((r[3] & (1 << 8)) == 0) return false;   // EDX bit 8: invariant TSC.
    LARGE_INTEGER f = {};
    return QueryPerformanceFrequency(&f) && f.QuadPart == 10000000;   // QPC from the TSC.
}
inline double fast_clock_out(double v) {
    FastClock& c = g_fast_clock;
    if (v < c.last) v = c.last;
    c.last = v;
    return v;
}
// steady_clock, held to the clock's last value while the origin is the one it was taken on.
inline double fast_clock_steady() {
    const double v = effect_time_seconds_d();
    return g_effect_t0.load(std::memory_order_relaxed) == g_fast_clock.t0 ? fast_clock_out(v) : v;
}
inline double fast_effect_seconds_d() {
    FastClock& c = g_fast_clock;
    if (c.usable <= 0) {
        if (c.usable < 0) return effect_time_seconds_d();
        c.usable = tsc_is_qpc() ? 1 : -1;
        if (c.usable < 0) return effect_time_seconds_d();
    }
    const uint64_t tsc = __rdtsc();
    const int64_t t0 = g_effect_t0.load(std::memory_order_relaxed);
    if (c.tsc_per_s > 0.0 && c.base_tsc != 0 && t0 == c.t0 && tsc >= c.base_tsc) {
        // The interval is far below 2^63 ticks: the signed conversion is one instruction.
        const double dt = static_cast<double>(static_cast<int64_t>(tsc - c.base_tsc)) * c.s_per_tsc;
        if (dt < FAST_CLOCK_SPAN_S) return fast_clock_out(c.base_s + dt);
    }
    if (c.tsc_per_s <= 0.0 && c.base_tsc != 0 && t0 == c.t0) {
        // No rate yet: steady_clock until the anchor is FAST_CLOCK_FIRST_S old.
        const double v = effect_time_seconds_d();
        if (v - c.base_s < FAST_CLOCK_FIRST_S &&
            g_effect_t0.load(std::memory_order_relaxed) == c.t0) return fast_clock_out(v);
    }
    // An anchor: steady_clock between two TSC reads on one core; the narrowest of up to three pairs (a preemption
    // between the reads widens one, a move to another core spoils one), done at the first narrow one once the rate is
    // known.
    double s = 0.0;
    uint64_t mid = 0, w = ~0ull;
    for (int k = 0; k < 3; ++k) {
        unsigned int xa = 0, xb = 0;
        const uint64_t a = __rdtscp(&xa);
        const double v = effect_time_seconds_d();
        const uint64_t b = __rdtscp(&xb);
        if (xa != xb || b < a) continue;   // Another core, or backwards: no pairing.
        if (b - a < w) {
            w = b - a;
            s = v;
            mid = a + w / 2u;
        }
        if (c.tsc_per_s > 0.0 && static_cast<double>(w) <= c.tsc_per_s * FAST_CLOCK_PAIR_S) break;
    }
    if (w == ~0ull) return fast_clock_steady();   // No pairing: steady_clock, the anchor kept.
    const int64_t t1 = g_effect_t0.load(std::memory_order_relaxed);   // The origin the reads were made on.
    if (t1 != c.t0) {   // A new origin (the first use, a session reset): its values start over.
        c.t0 = t1;
        c.last = 0.0;
        c.base_tsc = 0;
    }
    if (c.base_tsc != 0 && mid > c.base_tsc) {
        const double ds = s - c.base_s;
        const double dn = static_cast<double>(mid - c.base_tsc);   // TSC ticks over the same interval.
        if (ds >= FAST_CLOCK_FIRST_S) {
            const double rate = dn / ds;
            const bool part = c.tsc_per_s > 0.0 &&
                              fabs(dn * c.s_per_tsc - ds) > FAST_CLOCK_AGREE * ds;
            // The clocks parted over the span (a resume, a rate change): no rate - measured again from this anchor.
            const bool ok = !part && rate > 1.0e8 && rate < 2.0e10;
            c.tsc_per_s = ok ? rate : 0.0;
            c.s_per_tsc = ok ? 1.0 / rate : 0.0;
        }
    }
    c.base_s = s;
    c.base_tsc = mid;
    return fast_clock_out(s);
}
// The shader's time lane (fxMeta.y): seconds since the object was created, formed in double and handed over as
// float, so an effect's animation keeps its resolution however long the process has been up and never wraps.
inline float fx_time(const RenderObject& o, double now_d) {
    const double t = now_d - o.fx_t0;
    return t > 0.0 ? static_cast<float>(t) : 0.0f;
}
}