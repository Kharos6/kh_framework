#pragma once

// The feature half (rendering_integration_features.hpp): KH_AFFECTOR, the script-side physics affectors. Never include
// it on its own.
namespace RenderIntegration {

inline double effect_time_seconds_d();   // Defined with the effect clock (rendering/effect_clock.hpp).

// The frame delta the simulation steps on, from the process clock the effect envelopes use (steady wall time: the
// cloth keeps moving in the pause menu and does not follow setAccTime). The first frame of a session reports the
// clock's age at that call (the clock starts at its first read by anything, so 0 only when this is it), a gap over
// 10 s reports 0; the core advances nothing for a zero dt (an unprimed instance places its rest pose on its first
// step whatever dt says) and clamps any other long frame to KH_CLOTH_DT_MAX.
inline double effect_time_seconds_d();   // Defined with the effect clock (rendering/effect_clock.hpp).

// KH_AFFECTOR - the script-side record (addPhysicsAffector, updatePhysicsAffector, removeRenderHandler). GAME
// THREAD ONLY, unlocked: the SQF commands, kh_cloth_sync and the mission reset all run on it, and neither the render
// thread nor a worker reads this table - a worker sees only the views kh_cloth_sync copies into its instance.
struct KhAffector {
    uint8_t  type = KH_AFF_NONE;
    float    pos[3] = {};                      // SQF [x, y, zASL].
    float    dir[3] = { 0.0f, 1.0f, 0.0f };    // SQF vectorDir, unit; the unrotated types ignore it.
    // Half-extents (m) along the affector's right, forward and up - world east, north and up for a type without a
    // direction. All zero = unbounded.
    float    scale[3] = {};
    float    p[4] = {};                        // The type's parameters (KhClothAffView::p).
    float    falloff = 0.25f;                  // Outer fraction of the region faded to zero (0..1).
    // The lifetime, [fadeIn, hold, fadeOut] on the process clock; timed false = permanent. birth re-arms when the
    // script sets a new lifetime; t0 never does, so turbulence keeps flowing.
    bool     timed = false;
    float    fade_in = 0.0f, hold = 0.0f, fade_out = 0.0f;
    double   birth = 0.0;
    double   t0 = 0.0;
    uint64_t seq = 0;                          // Creation stamp; turbulence's seed.
};
static std::unordered_map<std::string, KhAffector> g_affectors;
// Its size, stored by the game thread wherever the table changes, for kh_cloth_stats_publish on the render thread.
static std::atomic<uint64_t> g_affectors_n{ 0 };
inline void kh_aff_count_note() { g_affectors_n.store(g_affectors.size(), std::memory_order_relaxed); }
static uint64_t g_aff_next_seq = 0;
// This frame's affectors with a live envelope, world centre in double, built once per frame and copied into each
// instance by kh_cloth_sync. reach is the region's bounding radius, negative for an unbounded one.
struct KhAffFrame { double c[3]; float reach; KhClothAffView v; };
static std::vector<KhAffFrame> g_aff_frame;

inline std::string make_affector_uid() {
    return std::string("khpa_") + UIDGenerator::generate();
}

inline bool kh_aff_rotated(uint8_t khar_t) {
    return khar_t == KH_AFF_WIND || khar_t == KH_AFF_VORTEX || khar_t == KH_AFF_FORCE;
}

// The renderer's lifetime_envelope, over the same [fadeIn, hold, fadeOut].
inline float kh_aff_envelope(const KhAffector& khae_a, double khae_now, bool& khae_expired) {
    khae_expired = false;
    if (!khae_a.timed) return 1.0f;
    double age = khae_now - khae_a.birth;
    if (age < 0.0) age = 0.0;
    if (age < khae_a.fade_in) return static_cast<float>(age / (khae_a.fade_in > 0.001f ? khae_a.fade_in : 0.001f));
    age -= khae_a.fade_in;
    if (age < khae_a.hold) return 1.0f;
    age -= khae_a.hold;
    if (age < khae_a.fade_out) return 1.0f - static_cast<float>(age / (khae_a.fade_out > 0.001f ? khae_a.fade_out : 0.001f));
    khae_expired = true;
    return 0.0f;
}

// Every entry point that looks an affector up reaps first, so an expired handle reads as removed everywhere at once.
inline void kh_aff_reap(double khar_now) {
    for (auto it = g_affectors.begin(); it != g_affectors.end(); ) {
        bool khar_x = false;
        kh_aff_envelope(it->second, khar_now, khar_x);
        if (khar_x) it = g_affectors.erase(it);
        else ++it;
    }
    kh_aff_count_note();
}

inline std::string kh_aff_add(KhAffector khaa_a) {
    if (!kh_render_on()) return std::string();   // KH_PLAYER_ONLY.
    khaa_a.seq = ++g_aff_next_seq;
    khaa_a.birth = khaa_a.t0 = effect_time_seconds_d();
    const std::string khaa_h = make_affector_uid();
    g_affectors[khaa_h] = khaa_a;
    kh_aff_count_note();
    return khaa_h;
}

inline bool kh_aff_remove(const std::string& khar_h) {
    const bool khar_r = g_affectors.erase(khar_h) != 0;
    kh_aff_count_note();
    return khar_r;
}
inline void kh_aff_clear() { g_affectors.clear(); kh_aff_count_note(); }
// KH_ALL_HANDLES: every affector handle, appended in creation order (seq). Game thread only; the caller reaps first
// if it wants only live ones.
inline void kh_aff_handles(std::vector<std::string>& khah_out) {
    std::vector<std::pair<uint64_t, std::string>> khah_v;
    khah_v.reserve(g_affectors.size());
    for (const auto& kv : g_affectors) khah_v.emplace_back(kv.second.seq, kv.first);
    std::sort(khah_v.begin(), khah_v.end());
    for (auto& khah_p : khah_v) khah_out.push_back(std::move(khah_p.second));
}

// The affector's axes as engine-axes rows: right, forward, up - Arma's model frame (x right, y forward, z up) with
// vectorDir as forward and the world's up as the reference, built in SQF axes then swapped (east, up, north) like a
// position. A vertical vectorDir takes north as its up. A type without a direction keeps east, north, up.
inline void kh_aff_basis(const KhAffector& khab_a, float* khab_ax) {
    float r[3] = { 1.0f, 0.0f, 0.0f }, f[3] = { 0.0f, 1.0f, 0.0f }, u[3] = { 0.0f, 0.0f, 1.0f };
    if (kh_aff_rotated(khab_a.type)) {
        for (int k = 0; k < 3; ++k) f[k] = khab_a.dir[k];
        const float ref[3] = { 0.0f, fabsf(f[2]) > 0.999f ? 1.0f : 0.0f, fabsf(f[2]) > 0.999f ? 0.0f : 1.0f };
        r[0] = f[1] * ref[2] - f[2] * ref[1];
        r[1] = f[2] * ref[0] - f[0] * ref[2];
        r[2] = f[0] * ref[1] - f[1] * ref[0];
        const float rl = kh_cloth_v3_len(r);
        for (int k = 0; k < 3; ++k) r[k] /= rl;
        u[0] = r[1] * f[2] - r[2] * f[1];
        u[1] = r[2] * f[0] - r[0] * f[2];
        u[2] = r[0] * f[1] - r[1] * f[0];
    }
    const float* khab_rows[3] = { r, f, u };
    for (int w = 0; w < 3; ++w) {
        khab_ax[w * 3 + 0] = khab_rows[w][0];
        khab_ax[w * 3 + 1] = khab_rows[w][2];
        khab_ax[w * 3 + 2] = khab_rows[w][1];
    }
}

// splitmix64: turbulence's modes are a pure function of the affector's seed.
inline uint64_t kh_aff_rng(uint64_t& khar_s) {
    uint64_t z = (khar_s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}
inline float kh_aff_u01(uint64_t& khau_s) {
    return static_cast<float>(static_cast<double>(kh_aff_rng(khau_s) >> 40) * (1.0 / 16777216.0));
}

// Reaps, then builds this frame's view of every affector with a live envelope. Game thread, once per frame, from
// kh_cloth_sync.
inline void kh_aff_frame_build(double khaf_now) {
    g_aff_frame.clear();
    kh_aff_reap(khaf_now);
    const double khaf_tau = 6.283185307179586;
    for (const auto& khaf_kv : g_affectors) {
        const KhAffector& a = khaf_kv.second;
        bool khaf_x = false;
        const float khaf_gain = kh_aff_envelope(a, khaf_now, khaf_x);
        if (!(khaf_gain > 0.0f)) continue;
        KhAffFrame fr{};
        fr.c[0] = static_cast<double>(a.pos[0]);   // SQF [x, y, zASL] -> engine [x, zASL, y].
        fr.c[1] = static_cast<double>(a.pos[2]);
        fr.c[2] = static_cast<double>(a.pos[1]);
        KhClothAffView& v = fr.v;
        v.type = a.type;
        kh_aff_basis(a, v.ax);
        v.bounded = a.scale[0] > 0.0f && a.scale[1] > 0.0f && a.scale[2] > 0.0f;
        fr.reach = -1.0f;
        if (v.bounded) {
            for (int k = 0; k < 3; ++k) v.inv_r[k] = 1.0f / a.scale[k];
            fr.reach = fmaxf(a.scale[0], fmaxf(a.scale[1], a.scale[2]));
        }
        v.edge = a.falloff < 0.0f ? 0.0f : (a.falloff > 1.0f ? 1.0f : a.falloff);
        v.gain = khaf_gain;
        for (int k = 0; k < 4; ++k) v.p[k] = a.p[k];
        if (a.type == KH_AFF_VORTEX) {
            // The core: as set, else a quarter of the region's narrower cross-section, else 1 m when unbounded.
            float khaf_core = a.p[3];
            if (!(khaf_core > 0.0f)) khaf_core = v.bounded ? 0.25f * fminf(a.scale[0], a.scale[2]) : 1.0f;
            v.p[3] = khaf_core > 0.01f ? khaf_core : 0.01f;
        }
        if (a.type == KH_AFF_TURBULENCE) {
            // Eight modes: four at the eddy size and four an octave finer at half the amplitude, directions and
            // phases from the seed, scaled so the field's RMS speed is the script's strength (each unit mode is RMS
            // 1/sqrt(2); the weights' squares sum to 5).
            const float khaf_size = a.p[1] > 0.01f ? a.p[1] : 0.01f;
            const float khaf_rate = a.p[2] > 0.0f ? a.p[2] : 0.0f;
            const float khaf_amp = a.p[0] / sqrtf(2.5f);
            const double khaf_age = khaf_now - a.t0;
            uint64_t khaf_s = a.seq * 0xD1B54A32D192ED03ull + 0x5851F42D4C957F2Dull;
            for (uint32_t m = 0; m < KH_AFF_MODES; ++m) {
                const float khaf_w = m < KH_AFF_MODES / 2 ? 1.0f : 0.5f;
                const float khaf_z = 2.0f * kh_aff_u01(khaf_s) - 1.0f;
                const float khaf_az = static_cast<float>(khaf_tau) * kh_aff_u01(khaf_s);
                const float khaf_rz = sqrtf(fmaxf(0.0f, 1.0f - khaf_z * khaf_z));
                const float khaf_kh[3] = { khaf_rz * cosf(khaf_az), khaf_z, khaf_rz * sinf(khaf_az) };
                const float khaf_km = static_cast<float>(khaf_tau) / khaf_size *
                                      (m < KH_AFF_MODES / 2 ? 1.0f : 2.0f) * (0.75f + 0.5f * kh_aff_u01(khaf_s));
                // Any direction perpendicular to the wave: the cross with a random vector, retried when degenerate.
                float khaf_a[3] = { 0.0f, 0.0f, 0.0f };
                for (int khaf_t = 0; khaf_t < 4; ++khaf_t) {
                    const float khaf_q[3] = { 2.0f * kh_aff_u01(khaf_s) - 1.0f, 2.0f * kh_aff_u01(khaf_s) - 1.0f,
                                              2.0f * kh_aff_u01(khaf_s) - 1.0f };
                    khaf_a[0] = khaf_kh[1] * khaf_q[2] - khaf_kh[2] * khaf_q[1];
                    khaf_a[1] = khaf_kh[2] * khaf_q[0] - khaf_kh[0] * khaf_q[2];
                    khaf_a[2] = khaf_kh[0] * khaf_q[1] - khaf_kh[1] * khaf_q[0];
                    if (kh_cloth_v3_len(khaf_a) > 0.1f) break;
                }
                const float khaf_al = kh_cloth_v3_len(khaf_a);
                for (int k = 0; k < 3; ++k) {
                    v.mk[m][k] = khaf_kh[k] * khaf_km;
                    v.ma[m][k] = khaf_al > 1.0e-6f ? khaf_a[k] / khaf_al * khaf_amp * khaf_w : 0.0f;
                }
                v.mw[m] = static_cast<float>(khaf_tau) * khaf_rate * (0.5f + kh_aff_u01(khaf_s));
                const double khaf_ph0 = khaf_tau * static_cast<double>(kh_aff_u01(khaf_s));
                v.mph[m] = static_cast<float>(fmod(static_cast<double>(v.mw[m]) * khaf_age + khaf_ph0, khaf_tau));
            }
        }
        g_aff_frame.push_back(fr);
    }
}
}